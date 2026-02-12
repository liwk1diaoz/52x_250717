/**
	@brief Source file of vendor ai net test code.

	@file ai_test.c

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_config.h"


/*-----------------------------------------------------------------------------*/
/* Input Functions                                                             */
/*-----------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////

UINT32 _calc_ai_buf_size(UINT32 loff, UINT32 h, UINT32 c, UINT32 t, UINT32 fmt)
{
	UINT size = 0;

	switch (fmt) {
	case HD_VIDEO_PXLFMT_YUV420:
		{
			size = loff * h * 3 / 2;
			//printf("size(%u) -> HD_VIDEO_PXLFMT_YUV420\n", size);
		}
		break;
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		{
			size = AI_RGB_BUFSIZE(loff, h);
			//printf("size(%u) -> HD_VIDEO_PXLFMT_RGB888_PLANAR\n", size);
		}
		break;
	case HD_VIDEO_PXLFMT_BGR888_PLANAR:
		{
			size = AI_RGB_BUFSIZE(loff, h);
			//printf("size(%u) -> HD_VIDEO_PXLFMT_BGR888_PLANAR\n", size);
		}
		break;
	default: // feature-in and float
		{
			size = loff * h * c * t;
			//printf("size(%u) -> feature-in\n", size);
		}
		break;
	}

	if (!size) {
		printf("ERROR!! ai_buf size = 0\n");
	}
	return size;
}

static INT32 _getsize_input(char* filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get input(%s) size fail\n", filename);
		return (-1);
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return bin_size;
}

static HD_RESULT _load_buf(MEM_PARM *mem_parm, CHAR *filename, VENDOR_AI_BUF *p_buf, UINT32 w, UINT32 h, UINT32 c, UINT32 loff, UINT32 fmt, UINT32 bit)
{
	INT32 file_len;

printf("image:%s\n", filename);
	file_len = ai_test_mem_load(mem_parm, filename);
	if (file_len < 0) {
		printf("load buf(%s) fail\r\n", filename);
		return HD_ERR_NG;
	}
	p_buf->width = w;
	p_buf->height = h;
	p_buf->channel = c;
	p_buf->line_ofs = loff;
	p_buf->fmt = fmt;
	p_buf->pa   = mem_parm->pa;
	p_buf->va   = mem_parm->va;
	p_buf->sign = MAKEFOURCC('A','B','U','F');
	//p_buf->size = loff * h * 3 / 2;
	p_buf->size = _calc_ai_buf_size(loff, h, c, bit/8, fmt);
	return HD_OK;
}

static int transform_float_to_fix(NET_PATH_ID net_path, VENDOR_AI_NET_PARAM_ID param_id, NET_IN_CONFIG *in_cfg, MEM_PARM *input_mem)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF layer0_in = {0};
	UINT32 fixed_size = 0;
	VOID *tmp = NULL;
	extern HD_RESULT vendor_ai_cpu_util_float2fixed (FLOAT *in_data, FLOAT in_scale_ratio, VOID *out_data, HD_VIDEO_PXLFMT out_fmt, INT32 data_size);
	
	if(!in_cfg || !input_mem){
		printf("in_cfg(%p) or input_mem(%p) is null\n", in_cfg, input_mem);
		return -1;
	}
	if(input_mem->size == 0 || !input_mem->va){
		printf("invalid input_mem size(%u) va(%x)\n", input_mem->size, input_mem->va);
		return -1;
	}

	ret = vendor_ai_net_get(net_path, param_id, &layer0_in);
	if (HD_OK != ret) {
		printf("net_path(%u) get layer0 inbuf fail !!\n", net_path);
		return HD_ERR_FAIL;
	}

	layer0_in.fmt = HD_VIDEO_PXLFMT_AI_SFIXED16(in_cfg->float_to_fix);
	
	fixed_size = in_cfg->w * in_cfg->h * in_cfg->c * sizeof(short);
	if(input_mem->size < fixed_size){
		printf("fixed size(%u) > input_mem->size(%u) of %s\n", fixed_size, input_mem->size, in_cfg->input_filename);
		return -1;
	}

	tmp = (VOID*)malloc(fixed_size);
	if(!tmp){
		printf("fail to allocate tmp buffer %d=u bytes\n", fixed_size);
		return -1;
	}

	ret = vendor_ai_cpu_util_float2fixed((FLOAT*)input_mem->va, 1.0f, tmp,  layer0_in.fmt, in_cfg->h * in_cfg->w * in_cfg->c);
	if(ret){
		printf("vendor_ai_cpu_util_float2fixed() fail\n");
		free(tmp);
		return -1;
	}

	memcpy((VOID*)input_mem->va, tmp, fixed_size);
	free(tmp);
	hd_common_mem_flush_cache((VOID*)input_mem->va, fixed_size);
	
	return 0;
}

HD_RESULT ai_test_input_open(NET_PATH_ID net_path, VENDOR_AI_NET_PARAM_ID param_id, NET_IN_CONFIG *in_cfg, MEM_PARM *input_mem, VENDOR_AI_BUF *src_img)
{
	HD_RESULT ret = HD_OK;
	UINT32 alloc_size = 0;
	UINT32 byte_align_size = 0;      

	if(!in_cfg || !input_mem || !src_img){
		printf("in_cfg(%p) or input_mem(%p) or src_img(%p) is null\n", in_cfg, input_mem, src_img);
		return -1;
	}

	input_mem->size = _getsize_input(in_cfg->input_filename);
	if(input_mem->size == (UINT32)-1){
		printf("fail to get size of %s\n", in_cfg->input_filename);
		return -1;
	}

	if (!in_cfg->is_comb_img) {
		byte_align_size = ALIGN_CEIL_64(input_mem->size);    
		alloc_size = byte_align_size * in_cfg->batch;
		//printf("allocate batch = %u\r\n", alloc_size);
	} else {
		byte_align_size = ALIGN_CEIL_64(input_mem->size+(64 * in_cfg->batch)); 
		alloc_size = byte_align_size;
		//printf("allocate = %u\r\n", alloc_size);
	}             

	ret = ai_test_mem_alloc(input_mem, "ai_in_buf", alloc_size);
	if (ret != HD_OK) {
		printf("alloc ai_in_buf fail\r\n");
		return HD_ERR_FAIL;
	}

	ret = _load_buf(input_mem,
		in_cfg->input_filename,
		src_img,
		in_cfg->w,
		in_cfg->h,
		in_cfg->c,
		in_cfg->loff,
		in_cfg->fmt,
		in_cfg->bit);
	src_img->ddr_id = input_mem->ddr_id;
	if (ret != HD_OK) {
		printf("input_open fail=%s\n", in_cfg->input_filename);
		ai_test_input_close(input_mem);
		memset(input_mem, 0, sizeof(MEM_PARM));
		memset(src_img, 0, sizeof(VENDOR_AI_BUF));
		return ret;
	}
	
	if(in_cfg->float_to_fix){
		ret = transform_float_to_fix(net_path, param_id, in_cfg, input_mem);
		if (ret != HD_OK) {
			printf("fail to transform %s from float to fix\n", in_cfg->input_filename);
			ai_test_input_close(input_mem);
			memset(input_mem, 0, sizeof(MEM_PARM));
			memset(src_img, 0, sizeof(VENDOR_AI_BUF));
			return ret;
		}
	}

	return HD_OK;
}

HD_RESULT ai_test_input_close(MEM_PARM *input_mem)
{
	if(!input_mem){
		printf("input_mem is null\n");
		return -1;
	}

	ai_test_mem_free(input_mem);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

