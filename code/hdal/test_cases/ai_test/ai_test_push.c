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
#include "ai_test_int.h"
#include "ai_test_util.h"
#include "hd_debug.h"


/*-----------------------------------------------------------------------------*/
/* Input Functions                                                             */
/*-----------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////

NET_IN g_in[16] = {0};

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

UINT32 _calc_ai_buf_size(UINT32 loff, UINT32 h, UINT32 c, UINT32 b, UINT32 bitdepth, UINT32 fmt)
{
	UINT size = 0;

	switch (fmt) {
	case HD_VIDEO_PXLFMT_YUV420:
		{
			size = loff * h * 3 / 2;
		}
		break;
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		{
			size = AI_RGB_BUFSIZE(loff, h);
		}
		break;
	case HD_VIDEO_PXLFMT_BGR888_PLANAR:
		{
			size = AI_RGB_BUFSIZE(loff, h);
		}
		break;
	default: // feature-in
		{
			size = loff * h * c * (bitdepth/8);
		}
		break;
	}

	if (!size) {
		printf("ERROR!! ai_buf size = 0\n");
	}
	return size;
}

static HD_RESULT _load_buf(MEM_PARM *mem_parm, CHAR *filename, VENDOR_AI_BUF *p_buf, UINT32 w, UINT32 h, UINT32 c, UINT32 b, UINT32 bitdepth, UINT32 loff, UINT32 fmt)
{
	INT32 file_len;
#if 0
	UINT32 key = 0;
#endif

	file_len = ai_test_mem_load(mem_parm, filename);
	if (file_len < 0) {
		printf("load buf(%s) fail\r\n", filename);
		return HD_ERR_NG;
	}
	printf("load buf(%s) ok\r\n", filename);
	p_buf->width = w;
	p_buf->height = h;
	p_buf->channel = c;
	p_buf->batch_num = b;
	p_buf->line_ofs = loff;
	p_buf->fmt = fmt;
#if 0
	if (p_buf->width == 0) {
		while (TRUE) {
			printf("Enter width = ?\r\n");
			if (scanf("%lu", &p_buf->width) != 1) {
				printf("Wrong input\n");
				continue;
			} else {
				break;
			}
		}
	}
	if (p_buf->height == 0) {
		while (TRUE) {
			printf("Enter height = ?\r\n");
			if (scanf("%lu", &p_buf->height) != 1) {
				printf("Wrong input\n");
				continue;
			} else {
				break;
			}
		}
	}
	if (p_buf->channel == 0) {
		while (TRUE) {
			printf("Enter channel = ?\r\n");
			if (scanf("%lu", &p_buf->channel) != 1) {
				printf("Wrong input\n");
				continue;
			} else {
				break;
			}
		}
	}
	if (p_buf->line_ofs == 0) {
		while (TRUE) {
			printf("Enter line offset = ?\r\n");
			if (scanf("%lu", &p_buf->line_ofs) != 1) {
				printf("Wrong input\n");
				continue;
			} else {
				break;
			}
		}
	}
	if (p_buf->fmt == 0) {
		while (TRUE) {
			printf("Enter image format = ?\r\n");
			printf("[%d] RGB888 (0x%08x)\r\n", NET_IMG_RGB_PLANE, HD_VIDEO_PXLFMT_RGB888_PLANAR);
			printf("[%d] YUV420 (0x%08x)\r\n", NET_IMG_YUV420, HD_VIDEO_PXLFMT_YUV420);
			printf("[%d] YUV422 (0x%08x)\r\n", NET_IMG_YUV422, HD_VIDEO_PXLFMT_YUV422);
			printf("[%d] Y only (0x%08x)\r\n", NET_IMG_YONLY, HD_VIDEO_PXLFMT_Y8);

			if (scanf("%lu", &key) != 1) {
				printf("Wrong input\n");
				continue;
			} else if (key == NET_IMG_YUV420) {
				p_buf->fmt = HD_VIDEO_PXLFMT_YUV420;
			} else if (key == NET_IMG_YUV422) {
				p_buf->fmt = HD_VIDEO_PXLFMT_YUV422;
			} else if (key == NET_IMG_RGB_PLANE) {
				p_buf->fmt = HD_VIDEO_PXLFMT_RGB888_PLANAR;
			} else if (key == NET_IMG_YONLY) {
				p_buf->fmt = HD_VIDEO_PXLFMT_Y8;
			} else {
				printf("Wrong enter format\n");
				continue;
			}
			break;
		}
	}
#endif
	p_buf->pa   = mem_parm->pa;
	p_buf->va   = mem_parm->va;
	p_buf->sign = MAKEFOURCC('A', 'B', 'U', 'F');
	p_buf->size = _calc_ai_buf_size(loff, h, c, b, bitdepth, fmt);
	if (p_buf->size == 0) {
		printf("load buf(%s) fail, p_buf->size = 0\r\n", filename);
		return HD_ERR_NG;
	}
	return HD_OK;
}

HD_RESULT ai_test_input_init(void)
{
	HD_RESULT ret = HD_OK;
	int  i;
	
	for (i = 0; i < 16; i++) {
		NET_IN* p_net = g_in + i;
		p_net->in_id = i;
	}
	return ret;
}

HD_RESULT ai_test_input_uninit(void)
{
	HD_RESULT ret = HD_OK;
	return ret;
}

INT32 ai_test_input_mem_config(NET_PATH_ID net_path, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, void* p_cfg, INT32 i)
{
	return i;
}

HD_RESULT ai_test_input_set_config(IN_PATH_ID in_path, NET_IN_CONFIG *p_in_cfg)
{
	HD_RESULT ret = HD_OK;
	NET_IN* p_in = g_in + in_path;
	UINT32 in_id = p_in->in_id;
	
	memcpy((void*)&p_in->in_cfg, (void*)p_in_cfg, sizeof(NET_IN_CONFIG));
	printf("in_id(%lu) set in_cfg: file(%s), buf=(%lu,%lu,%lu,%lu,%lu,%lu,0x%08lx,%lu)\r\n",
		in_id,
		p_in->in_cfg.input_filename,
		p_in->in_cfg.w,
		p_in->in_cfg.h,
		p_in->in_cfg.c,
		p_in->in_cfg.b,
		p_in->in_cfg.bitdepth,
		p_in->in_cfg.loff,
		p_in->in_cfg.fmt,
		p_in->in_cfg.is_comb_img);
	
	return ret;
}

HD_RESULT ai_test_input_open(IN_PATH_ID in_path)
{
	HD_RESULT ret = HD_OK;
	NET_IN* p_in = g_in + in_path;
	UINT32 in_id = p_in->in_id;
	p_in->input_mem.size = _getsize_input(p_in->in_cfg.input_filename);
	ret = ai_test_mem_alloc(&p_in->input_mem, "ai_in_buf", p_in->input_mem.size);
	if (ret != HD_OK) {
		printf("in_id(%lu) alloc ai_in_buf fail\r\n", in_id);
		return HD_ERR_FAIL;
	}

	// load in buf
	ret = _load_buf(&p_in->input_mem, 
		p_in->in_cfg.input_filename, 
		&p_in->src_img,
		p_in->in_cfg.w,
		p_in->in_cfg.h,
		p_in->in_cfg.c,
		p_in->in_cfg.b,
		p_in->in_cfg.bitdepth,
		p_in->in_cfg.loff,
		p_in->in_cfg.fmt);
	if (ret != HD_OK) {
		printf("in_id(%lu) input_open fail=%d\n", in_id, ret);
	}

	return ret;
}

HD_RESULT ai_test_input_close(IN_PATH_ID in_path)
{
	HD_RESULT ret = HD_OK;
	NET_IN *p_in = g_in + in_path;

	ai_test_mem_free(&p_in->input_mem);

	return ret;
}

HD_RESULT ai_test_input_start(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	return ret;
}

HD_RESULT ai_test_input_stop(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	return ret;
}

HD_RESULT ai_test_input_pull_buf(NET_PATH_ID net_path, VENDOR_AI_BUF *p_in, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	NET_IN *p_net = g_in + net_path;

	memcpy((void *)p_in, (void *) & (p_net->src_img), sizeof(VENDOR_AI_BUF));
	return ret;
}
///////////////////////////////////////////////////////////////////////////////

