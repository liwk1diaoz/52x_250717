/**
	@brief Source file of vendor ai op test code.

	@file ai_test_op.c

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_config.h"
#include "ai_test_op.h"

static unsigned int calculate_input_buf_size(int op_opt, UINT32 fmt, UINT32 w, UINT32 h)
{
	unsigned int size = 0;
	
	if(fmt == 0x520c0420)//HD_VIDEO_PXLFMT_YUV420
		size = w*h*3/2;
	else if(fmt == 0x51080400)//HD_VIDEO_PXLFMT_Y8
		size = w*h;
	else if(fmt == 0x23180888)//HD_VIDEO_PXLFMT_RGB888_PLANAR
		size = AI_RGB_BUFSIZE(w, h);
	else{
		printf("unknown format %x\n", fmt);
		return 0;
	}

	if(op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE)
		size += AI_RGB_BUFSIZE(w, h);
	
	return size;
}

static unsigned int calculate_output_buf_size(UINT32 fmt, UINT32 w, UINT32 h)
{
	unsigned int size = 0;
	
	if(fmt == 0x520c0420)//HD_VIDEO_PXLFMT_YUV420
		size = w*h*3/2;
	else if(fmt == 0x51080400)//HD_VIDEO_PXLFMT_Y8
		size = w*h;
	else if(fmt == 0x23180888)//HD_VIDEO_PXLFMT_RGB888_PLANAR
		size = AI_RGB_BUFSIZE(w, h);
	else{
		printf("unknown format %x\n", fmt);
		return 0;
	}
	
	return size;
}

static HD_RESULT ai_test_operator_alloc_input_buf(int op_opt, MEM_PARM *input_mem, MEM_PARM *weight_mem, NET_IN_OUT_CONFIG *in_out_cfg1, NET_IN_OUT_CONFIG *in_out_cfg2)
{
	HD_RESULT ret = HD_OK;
	
	if(!input_mem || !weight_mem){
		printf("input_mem(%p) or weight_mem(%p) is NULL\n", input_mem, weight_mem);
		return HD_ERR_FAIL;
	}

	// alloc buffer
	switch (op_opt) {
		case AI_OP_FC: 
		case AI_OP_FC_LL_MODE:
        {
			if(!in_out_cfg1 || !in_out_cfg2){
				printf("in_out_cfg1(%p) or in_out_cfg2(%p) is NULL\n", in_out_cfg1, in_out_cfg2);
				return HD_ERR_FAIL;
			}

			ret = ai_test_mem_alloc(input_mem, "user_in_buf", in_out_cfg1->in_w*in_out_cfg1->in_h);
	        if (ret != HD_OK) {
	        	printf("alloc in_buf fail\r\n");
	        	return HD_ERR_FAIL;
	        }
			ret = ai_test_mem_alloc(weight_mem, "user_weight_buf", in_out_cfg2->in_w*in_out_cfg2->in_h);
	        if (ret != HD_OK) {
	        	printf("alloc weight_buf fail\r\n");
	        	return HD_ERR_FAIL;
	        }
		}
        break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_Y2Y_UV2UV:
        {	
			unsigned int size;

			if(!in_out_cfg1){
				printf("in_out_cfg1 is NULL\n");
				return HD_ERR_FAIL;
			}

			size = calculate_input_buf_size(op_opt, in_out_cfg1->in_fmt, in_out_cfg1->in_w, in_out_cfg1->in_h);
			if(size == 0){
	    	    printf("fail to calculate input size\r\n");
	    	    return HD_ERR_FAIL;
			}

			ret = ai_test_mem_alloc(input_mem, "user_in_buf", size);
			if (ret != HD_OK) {
	    	    printf("alloc in_buf fail\r\n");
	    	    return HD_ERR_FAIL;
	        }
        }
        break;
        default:
        {
            printf("Unknown op_opt\r\n");
	        return HD_ERR_LIMIT;
        }
        break;
	}
	
	return 0;
}

static HD_RESULT ai_test_operator_load_input(int op_opt, MEM_PARM *mem, NET_IN_OUT_CONFIG *in_out_cfg)
{
	INT32 file_len;
			
	if(!mem || !in_out_cfg){
		printf("mem(%p) or in_out_cfg(%p) is NULL\r\n", mem, in_out_cfg);
		return HD_ERR_NG;
	}
	
printf("load image:%s\n", 	in_out_cfg->input_filename);
	if(op_opt == AI_OP_FC || op_opt == AI_OP_FC_LL_MODE){
		ai_test_mem_fill(mem, 1);
	}else{
		file_len = ai_test_mem_load(mem, in_out_cfg->input_filename);
		if (file_len < 0) {
			printf("load buf(%s) fail\r\n", in_out_cfg->input_filename);
			return HD_ERR_NG;
		}
	}
	
	return 0;
}

static int ai_test_free_fourth_buf(FOURTH_LOOP_DATA *fourth_loop_data, MEM_PARM *input_mem, MEM_PARM* weight_mem, MEM_PARM* output_mem){

	GOLDEN_SAMPLE_RESULT *golden_sample;

	if(!fourth_loop_data || !input_mem || !weight_mem || !output_mem){
		printf("fourth_loop_data(%p) or input_mem(%p) or weight_mem(%p) or output_mem(%p) is null\n", fourth_loop_data, input_mem, weight_mem, output_mem);
		return -1;
	}

	golden_sample = &fourth_loop_data->golden_sample;
	if(golden_sample->data)
		free(golden_sample->data);
	memset(golden_sample, 0, sizeof(GOLDEN_SAMPLE_RESULT));

	if(input_mem->pa)
		ai_test_mem_free(input_mem);
	memset(input_mem, 0, sizeof(MEM_PARM));

	if(weight_mem->pa)
		ai_test_mem_free(weight_mem);
	memset(weight_mem, 0, sizeof(MEM_PARM));

	if(output_mem->pa)
		ai_test_mem_free(output_mem);
	memset(output_mem, 0, sizeof(MEM_PARM));

	return 0;
}

static HD_RESULT ai_test_operator_alloc_out_buf(int op_opt, MEM_PARM *output_mem, NET_IN_OUT_CONFIG *in_out_cfg1, NET_IN_OUT_CONFIG *in_out_cfg2)
{
	HD_RESULT ret = HD_ERR_FAIL;

	if(!output_mem){
        printf("output_mem is NULL\r\n");
        return HD_ERR_NG;
	}

	// alloc result buff
    switch (op_opt) {
	    case AI_OP_FC:
	    case AI_OP_FC_LL_MODE:
        {
			unsigned int size1, size2;
			if(!in_out_cfg1 || !in_out_cfg2){
				printf("in_out_cfg1(%p) or in_out_cfg2(%p) is null\n", in_out_cfg1, in_out_cfg2);
				return HD_ERR_NG;
			}
			
			size1 = calculate_output_buf_size(in_out_cfg1->out_fmt, in_out_cfg1->in_w, in_out_cfg1->in_h);
			if(size1 == 0){
	    	    printf("fail to calculate input size1\r\n");
	    	    return HD_ERR_FAIL;
			} 
			size2 = calculate_output_buf_size(in_out_cfg2->out_fmt, in_out_cfg2->in_w, in_out_cfg2->in_h);
			if(size2 == 0){
	    	    printf("fail to calculate input size2\r\n");
	    	    return HD_ERR_FAIL;
			} 

	        ret = ai_test_mem_alloc(output_mem, "user_out_buf", 4*size2/size1);
	        if (ret != HD_OK) {
	       	    printf("alloc out_buf fail\r\n");
	      	    return HD_ERR_FAIL;
	        }
    	}
		break;
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_Y2Y_UV2UV:
        {
			unsigned int size;
			if(!in_out_cfg1){
				printf("in_out_cfg1 is null\n");
				return HD_ERR_NG;
			}
			
			size = calculate_output_buf_size(in_out_cfg1->out_fmt, in_out_cfg1->out_w, in_out_cfg1->out_h);
			if(size == 0){
	    	    printf("fail to calculate input size\r\n");
	    	    return HD_ERR_FAIL;
			}

			ret = ai_test_mem_alloc(output_mem, "user_out_buf", size);
			if (ret != HD_OK) {
	        	printf("alloc out_buf fail\r\n");
	        	return HD_ERR_FAIL;
	        }
        }
        break;
        default:
        break;
    }

	return ret;
}

static int compare_with_golden_file(void *out_buf, UINT32 size, GOLDEN_SAMPLE_RESULT *golden_sample, char *filename)
{
	FILE *fd;
	UINT32 file_size = 0;

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}

	fseek(fd, 0, SEEK_END);
	file_size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	fclose(fd);

	if(file_size != size){
		printf("file(%s) size(%u) != output size(%u)\n", filename, (int)file_size, (int)size);
		return -1;
	}

	if(golden_sample->data && size != golden_sample->size){
		free(golden_sample->data);
		golden_sample->size = 0;
		golden_sample->data = malloc(size);
		if(golden_sample->data){
			printf("fail to allocate bytes %d for golden sample\n", (int)size);
			return -1;
		}
		golden_sample->size = size;
	}

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}
	file_size = fread (golden_sample->data, 1, size, fd);
	fclose(fd);
	if (size != file_size) {
		printf("size mismatch, real = %u, idea = %u\r\n", (int)file_size, (int)size);
		return -1;
	}

	if(memcmp(out_buf, golden_sample->data, size)){
		printf("output != golden sample\n");
		return -1;
	}

	return 0;
}

static int verify_result(void *out_buf, UINT32 size, NET_IN_OUT_CONFIG *in_out_cfg, FOURTH_LOOP_DATA *fourth_loop_data)
{
	GOLDEN_SAMPLE_RESULT *golden_sample;

	if(!in_out_cfg || !fourth_loop_data || !out_buf || !size){
		printf("in_out_cfg(%p) or fourth_loop_data(%p) or out_buf(%p) or size(%u)is null\n",
					in_out_cfg, fourth_loop_data, out_buf, (int)size);
		return -1;
	}

	golden_sample = &(fourth_loop_data->golden_sample);
	if(!golden_sample){
		printf("golden_sample is null\n");
		return -1;
	}

	if(in_out_cfg->verify_result == 0)
		return 0;
	else if(strlen(in_out_cfg->golden_sample_filename)){
		if(compare_with_golden_file(out_buf, size, golden_sample, in_out_cfg->golden_sample_filename)){
			printf("output != previous output\n");
			return -1;
		}
	}else{
		if(fourth_loop_data->golden_sample.data == NULL){
			fourth_loop_data->golden_sample.data = malloc(size);
			if(fourth_loop_data->golden_sample.data == NULL){
				printf("fail to allocate %d bytes for verify\n", size);
				return -1;
			}else{
				fourth_loop_data->golden_sample.size = size;
				memcpy(fourth_loop_data->golden_sample.data, out_buf, size);
			}
		}else{
			if(fourth_loop_data->golden_sample.size != size){
				printf("output size(%u) != last output size(%u)\n", size, fourth_loop_data->golden_sample.size);
				return -1;
			}else if(memcmp(fourth_loop_data->golden_sample.data, out_buf, size) != 0){
				printf("output != last output\n");
				return -1;
			}
		}
	}

	return 0;
}

static int setup_ai_buf(VENDOR_AI_BUF *ai_buf, NET_IN_OUT_CONFIG *in_out_cfg, int input, uintptr_t pa, uintptr_t va)
{
	int w, h, c, fmt, loff;

	if(!ai_buf || !in_out_cfg){
		printf("ai_buf(%p) or in_out_cfg(%p) is null\n", ai_buf, in_out_cfg);
		return -1;
	}
	
	if(input){
		w = in_out_cfg->in_w;
		h = in_out_cfg->in_h;
		c = in_out_cfg->in_c;
		loff = in_out_cfg->in_loff;
		fmt = in_out_cfg->in_fmt;
	}else{
		w = in_out_cfg->out_w;
		h = in_out_cfg->out_h;
		c = in_out_cfg->out_c;
		loff = in_out_cfg->out_loff;
		fmt = in_out_cfg->out_fmt;
	}

	ai_buf[0].sign = MAKEFOURCC('A','B','U','F');
	ai_buf[0].ddr_id = 0;
	ai_buf[0].va = va; //< input address	 
	ai_buf[0].pa = pa;
	ai_buf[0].size = loff*h;
	ai_buf[0].width = w;
	ai_buf[0].height = h;
	ai_buf[0].line_ofs = loff;
	ai_buf[0].channel = c;
	ai_buf[0].batch_num = 1;
	if(fmt == 0x520c0420 || fmt == 0x51080400)
		ai_buf[0].fmt = HD_VIDEO_PXLFMT_Y8;
	else if(fmt == 0x23180888)
		ai_buf[0].fmt = HD_VIDEO_PXLFMT_R8;

	if(fmt == 0x520c0420 || fmt == 0x23180888){
		ai_buf[1].sign = MAKEFOURCC('A','B','U','F');
		ai_buf[1].ddr_id = 0;
		ai_buf[1].va = va + ai_buf[0].size;
		ai_buf[1].pa = pa + ai_buf[0].size;
		ai_buf[1].size = loff*h;
		ai_buf[1].width = w;
		ai_buf[1].height = h;
		ai_buf[1].line_ofs = loff;
		ai_buf[1].channel = c;
		ai_buf[1].batch_num = 1;
		if(fmt == 0x520c0420)
			ai_buf[1].fmt = HD_VIDEO_PXLFMT_UV;
		else
			ai_buf[1].fmt = HD_VIDEO_PXLFMT_G8;
	}

	if(fmt == 0x23180888){
		ai_buf[2].sign = MAKEFOURCC('A','B','U','F');
		ai_buf[2].ddr_id = 0;
		ai_buf[2].va = va + ai_buf[0].size + ai_buf[1].size;
		ai_buf[2].pa = pa + ai_buf[0].size + ai_buf[1].size;
		ai_buf[2].size = loff*h;
		ai_buf[2].width = w;
		ai_buf[2].height = h;
		ai_buf[2].line_ofs = loff;
		ai_buf[2].channel = c;
		ai_buf[2].batch_num = 1;
		ai_buf[2].fmt = HD_VIDEO_PXLFMT_B8;
	}
	
	return 0;
}

static UINT32 _calc_image_size(UINT32 loff, UINT32 h, UINT32 fmt)
{
	UINT32 size = 0;

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
	case HD_VIDEO_PXLFMT_Y8:
		{
			size = loff * h;
		}
		break;
	default: // feature-in
		{
			size = 0;
		}
		break;
	}

	if (!size) {
		printf("ERROR!! format(%x) ai_buf size = 0\n", fmt);
	}
	return size;
}

static int ai_test_op_set_unaligned_start_address(UINT32 in_cnt, VENDOR_AI_BUF *ai_buf)
{
	UINT32 fmt;
	int align = 1;

	fmt = ai_buf[0].fmt;
	switch (fmt) {
	case HD_VIDEO_PXLFMT_UV:
		{
			align = 2;
			if ((ai_buf[0].pa) && !(ai_buf[0].pa % align)) {
				ai_buf[0].pa += 1;
			}
			if ((ai_buf[0].va) && !(ai_buf[0].va % align)) {
				ai_buf[0].va += 1;
			}
		}
		break;
	case HD_VIDEO_PXLFMT_Y8:
		{
			if (ai_buf[1].fmt == HD_VIDEO_PXLFMT_UV && in_cnt == 2) {
				align = 2;
				if ((ai_buf[1].pa) && !(ai_buf[1].pa % align)) {
					ai_buf[1].pa += 1;
				}
				if ((ai_buf[1].va) && !(ai_buf[1].va % align)) {
					ai_buf[1].va += 1;
				}
			} else {
				ai_buf[0].pa += 1;
				ai_buf[0].va += 1;
			}
		}
		break;
	case HD_VIDEO_PXLFMT_R8:
	case HD_VIDEO_PXLFMT_G8:
	case HD_VIDEO_PXLFMT_B8:
		{
			ai_buf[0].pa += 1;
			ai_buf[0].va += 1;
		}
		break;
	default: // feature-in
		break;
	}

	return 0;
}

static int ai_test_op_check_unaligned_start_address(UINT32 in_cnt, VENDOR_AI_BUF *ai_buf, HD_RESULT ret)
{
	UINT32 fmt;
	int result = ret;

	fmt = ai_buf[0].fmt;
	switch (fmt) {
	case HD_VIDEO_PXLFMT_UV:
		{
			result = ((ret) ? 0 : -1);
		}
		break;
	case HD_VIDEO_PXLFMT_Y8:
		{
			if (ai_buf[1].fmt == HD_VIDEO_PXLFMT_UV && in_cnt == 2) {
				result = ((ret) ? 0 : -1);
			}
		}
		break;
	case HD_VIDEO_PXLFMT_R8:
	case HD_VIDEO_PXLFMT_G8:
	case HD_VIDEO_PXLFMT_B8:
	default: // feature-in
		break;
	}

	return result;
}

static int fourth_loop(FIRST_LOOP_DATA *first_loop_data, OPERATOR_CONFIG *op_cfg)
{
	HD_RESULT ret = HD_OK;
	int i = 0, in_file_cfg_idx = 0;
	int loop_count, in_file_count;
	NET_PATH_ID net_path;
	MEM_PARM input_mem = {0}, weight_mem = {0}, output_mem = {0};
	NET_IN_OUT_CONFIG *in_out_cfg = NULL;
	NET_IN_FILE_CONFIG *in_file_cfg;
	UINT32 verify_size = 0;
	VENDOR_AI_BUF src[3] = {0};
	VENDOR_AI_BUF dest[3] = {0};
	VENDOR_AI_OP_PREPROC_PARAM p_parm = {0};
	UINT32 input_img_size = 0;

	if(!first_loop_data || !op_cfg){
		printf("first_loop_data(%p) or op_cfg(%p) is null\n", first_loop_data, op_cfg);
		return -1;
	}

	loop_count = first_loop_data->second_loop_data.third_loop_data.fourth_loop_data.loop_count;
	in_file_cfg = op_cfg->in_file_cfg;
	in_file_count = op_cfg->in_file_cfg_count;
	net_path = first_loop_data->net_path;

	for(in_file_cfg_idx = 0 ; in_file_cfg_idx < in_file_count ; in_file_cfg_idx++){
		
		memset(&input_mem, 0, sizeof(MEM_PARM));
		memset(&weight_mem, 0, sizeof(MEM_PARM));
		memset(&output_mem, 0, sizeof(MEM_PARM));
		
		in_out_cfg = in_file_cfg[in_file_cfg_idx].in_out_cfg;

		if(in_file_cfg[in_file_cfg_idx].input_num > 1)
			ret = ai_test_operator_alloc_input_buf(op_cfg->op_opt, &input_mem, &weight_mem, &(in_out_cfg[0]), &(in_out_cfg[1]));
		else
			ret = ai_test_operator_alloc_input_buf(op_cfg->op_opt, &input_mem, &weight_mem, &(in_out_cfg[0]), NULL);
		if(ret){
			printf("net_path(%lu) alloc input fail !!\n", net_path);
			goto exit;
		}
		
		ret = ai_test_operator_load_input(op_cfg->op_opt, &input_mem, &(in_out_cfg[0]));
		if(ret){
			printf("net_path(%lu) load input fail !!\n", net_path);
			goto exit;
		}
		
		if(in_file_cfg[in_file_cfg_idx].input_num > 1){
			ret = ai_test_operator_load_input(op_cfg->op_opt, &weight_mem, &(in_out_cfg[1]));
			if(ret){
				printf("net_path(%lu) load weight fail !!\n", net_path);
				goto exit;
			}
		}

		if(in_file_cfg[in_file_cfg_idx].input_num > 1)
			ret = ai_test_operator_alloc_out_buf(op_cfg->op_opt, &output_mem, &(in_out_cfg[0]), &(in_out_cfg[1]));
		else
			ret = ai_test_operator_alloc_out_buf(op_cfg->op_opt, &output_mem, &(in_out_cfg[0]), NULL);
		if (HD_OK != ret) {
			printf("net_path(%lu) alloc output fail !!\n", net_path);
			goto exit;
		}

		for(i = 0 ; i < loop_count ; ++i){
			
			memset(src, 0, sizeof(src));
			memset(dest, 0, sizeof(dest));
			memset(&p_parm, 0, sizeof(VENDOR_AI_OP_PREPROC_PARAM));
			ai_test_mem_fill(&output_mem, i);
			
			//prepare src and dest
			if(op_cfg->op_opt == AI_OP_FC || op_cfg->op_opt == AI_OP_FC_LL_MODE){
				if(in_file_cfg[in_file_cfg_idx].input_num <= 1){
					printf("AI_OP_FC must have two input. current=%d\n", in_file_cfg[in_file_cfg_idx].input_num);
					goto exit;
				}
				// 2. flush input
				ret = hd_common_mem_flush_cache((VOID *)input_mem.va, input_mem.size);
				if(HD_OK != ret) {
					printf("flush cache failed.\n");
					goto exit;
				}
				ret = hd_common_mem_flush_cache((VOID *)weight_mem.va, weight_mem.size);
				if(HD_OK != ret) {
					printf("flush cache failed.\n");
					goto exit;
				}
				ret = hd_common_mem_flush_cache((VOID *)output_mem.va, output_mem.size);
				if(HD_OK != ret) {
					printf("flush cache failed.\n");
					goto exit;
				}
				// 3. run OP
				{								
					//set src1 as 1d tensor
					src[0].sign = MAKEFOURCC('A','B','U','F');
					src[0].ddr_id = 0;
					src[0].va = (uintptr_t)input_mem.va; //< input address	 (size = SV_FEA_LENGTH bytes)
					src[0].pa = (uintptr_t)input_mem.pa;
					src[0].size = input_mem.size;;
					src[0].fmt = HD_VIDEO_PXLFMT_AI_SFIXED8(0); //fixpoint s7.0
					src[0].width = in_out_cfg[0].in_w;
					src[0].height = in_out_cfg[0].in_h;
					src[0].channel = in_out_cfg[0].in_c;
					src[0].batch_num = 1;
								
					//set src2 as 2d tensor
					src[1].sign = MAKEFOURCC('A','B','U','F');
					src[1].ddr_id = 0;
					src[1].va = (uintptr_t)weight_mem.va; //< sv weight address (size = SV_LENGTH*SV_FEA_LENGTH bytes)
					src[1].pa = (uintptr_t)weight_mem.pa;
					src[1].size = weight_mem.size;
					src[1].fmt = HD_VIDEO_PXLFMT_AI_SFIXED8(0); //fixpoint s7.0
					src[1].width = in_out_cfg[1].in_w;
					src[1].height = in_out_cfg[1].in_h;
					src[1].channel = in_out_cfg[1].in_c;
					src[1].batch_num = 1;
								
					//set dest as 1d tensor
					dest[0].sign = MAKEFOURCC('A','B','U','F');
					dest[0].ddr_id = 0;
					dest[0].va = (uintptr_t)output_mem.va; //< output address	 (size = SV_LENGTH*4 bytes)
					dest[0].pa = (uintptr_t)output_mem.pa;
					dest[0].size =  output_mem.size;
					dest[0].fmt = HD_VIDEO_PXLFMT_AI_SFIXED32(0); //fixpoint s31.0
					dest[0].width = in_out_cfg[0].out_w;
					dest[0].height = in_out_cfg[0].out_h;
					dest[0].channel = in_out_cfg[0].out_c;
					dest[0].batch_num = 1;
				}
			}else{
				ret = hd_common_mem_flush_cache((VOID *)input_mem.va, input_mem.size);
				if(HD_OK != ret) {
					printf("flush cache failed.\n");
					goto exit;
				}
				ret = hd_common_mem_flush_cache((VOID *)output_mem.va, output_mem.size);
				if(HD_OK != ret) {
					printf("flush cache failed.\n");
					goto exit;
				}

				if(setup_ai_buf(src, in_out_cfg, 1, (uintptr_t)input_mem.pa, (uintptr_t)input_mem.va)){
					printf("fail to setup input ai_buf\n");
					goto exit;
				}
				if(setup_ai_buf(dest, in_out_cfg, 0, (uintptr_t)output_mem.pa, (uintptr_t)output_mem.va)){
					printf("fail to setup output ai_buf\n");
					goto exit;
				}
			}
			
			//do vendor_ai_op_proc()
			if(op_cfg->op_opt == AI_OP_FC){
				ret = vendor_ai_op_proc(net_path, VENDOR_AI_OP_FC, NULL, 2, src, 1, dest);
				if (ret != 0) {
					printf("op inference fail\n");
					goto exit;
				}

				ret = hd_common_mem_flush_cache((VOID *)output_mem.va, output_mem.size);
				if(HD_OK != ret) {
					printf("flush cache failed.\n");
					goto exit;
				}
				
				verify_size = output_mem.size;
			}else if(op_cfg->op_opt == AI_OP_FC_LL_MODE){
				ret = vendor_ai_op_add(net_path, VENDOR_AI_OP_LIST, NULL, 2, src, 1, dest);
				if (ret != HD_OK) {
					printf("net_path(%u) vendor_ai_op_add fail\r\n", net_path);
					goto exit;
				}
								
				ret = hd_common_mem_flush_cache((VOID *)first_loop_data->work_mem.va, first_loop_data->work_mem.size);
				if(HD_OK != ret) {
					printf("flush cache failed.\n");
					goto exit;
				}
	
				ret = vendor_ai_op_proc(net_path, VENDOR_AI_OP_LIST, NULL, 0, NULL, 0, NULL);
				if (ret != HD_OK) {
					printf("net_path(%u) vendor_ai_op_proc for run fc ll fail\r\n", net_path);
					goto exit;
				}

				ret = hd_common_mem_flush_cache((VOID *)output_mem.va, output_mem.size);
				if(HD_OK != ret) {
					printf("flush cache failed.\n");
					goto exit;
				}
				
				verify_size = output_mem.size;
			}else{
				
				if(in_out_cfg[0].in_fmt == 0x520c0420 && in_out_cfg[0].out_fmt == 0x520c0420){
					src[1].width /= 2;
					src[1].height /= 2;
					dest[1].width /= 2;
					dest[1].height /= 2;
					dest[1].size /= 2;
					p_parm.scale_dim.w = in_out_cfg[0].out_w;
					p_parm.scale_dim.h = in_out_cfg[0].out_h;
					ret = vendor_ai_op_proc(net_path, VENDOR_AI_OP_PREPROC, &p_parm, 1, src, 1, dest);						
                    if (ret != 0) {
		        		printf("op inference y plane fail\n");
		        		goto exit;
		        	}

					if (in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr) {
						ai_test_op_set_unaligned_start_address(1, src+1);
					}

					p_parm.scale_dim.w /= 2;
					p_parm.scale_dim.h /= 2;
					ret = vendor_ai_op_proc(net_path, VENDOR_AI_OP_PREPROC, &p_parm, 1, src+1, 1, dest+1);
					if(in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr){
						if (ret) {
							if (0 != ai_test_op_check_unaligned_start_address(1, src+1, ret)) {
								printf("fail to detect unaligned addr buf\n");
								goto exit;
							} else {
								ret = 0;
								goto exit;
							}
						} else {
							if (0 != ai_test_op_check_unaligned_start_address(1, src+1, ret)) {
								ret = -1;
								printf("fail to detect unaligned addr buf\n");
								goto exit;
							}
						}
					} else {
						if (ret != 0) {
							printf("op inference uv plane fail\n");
							goto exit;
						}
					}

					verify_size = (in_out_cfg[0].out_w * in_out_cfg[0].out_h * 3 / 2);
				}else{
					int src_plane_num, dest_plane_num;
					// set func parameter
					if (in_out_cfg[0].in_w != in_out_cfg[0].out_w || in_out_cfg[0].in_h != in_out_cfg[0].out_h) {
						p_parm.scale_dim.w = in_out_cfg[0].out_w;
						p_parm.scale_dim.h = in_out_cfg[0].out_h;
					}				
								
					// plane mode
					if (op_cfg->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE) {
						input_img_size = _calc_image_size(in_out_cfg[0].in_loff, in_out_cfg[0].in_h, in_out_cfg[0].in_fmt);
						if(input_img_size == 0){
							printf("input image size is 0\n");
							goto exit;
						}
						memset((VOID *)(input_mem.va + input_img_size), 0x80808080, in_out_cfg[0].in_w*in_out_cfg[0].in_h*3);   //clear buffer for sub
						ret = hd_common_mem_flush_cache((VOID *)(input_mem.va + input_img_size), in_out_cfg[0].in_w*in_out_cfg[0].in_h*3);
						if(HD_OK != ret) {
							printf("flush cache failed.\n");
							goto exit;
						}
						p_parm.p_out_sub.pa = (uintptr_t)input_mem.pa + input_img_size;
						p_parm.p_out_sub.va = (uintptr_t)input_mem.va + input_img_size;
						p_parm.p_out_sub.width = in_out_cfg[0].out_w;
						p_parm.p_out_sub.height = in_out_cfg[0].out_h;
						p_parm.p_out_sub.line_ofs = in_out_cfg[0].out_w*3;
					}

					// dc mode
					if (op_cfg->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_DC) {                 
						p_parm.out_sub_color[0] = 128;
						p_parm.out_sub_color[1] = 127;
						p_parm.out_sub_color[2] = 126;
					}
					
					if(in_out_cfg[0].in_fmt == 0x520c0420)
						src_plane_num = 2;
					else if(in_out_cfg[0].in_fmt == 0x51080400)
						src_plane_num = 1;
					else if(in_out_cfg[0].in_fmt == 0x23180888)
						src_plane_num = 3;
					else{
		        		printf("unknown input format %x\n", in_out_cfg[0].in_fmt);
		        		goto exit;
		        	}
					
					if(in_out_cfg[0].out_fmt == 0x520c0420){
						dest_plane_num = 2;
						verify_size = (in_out_cfg[0].out_w * in_out_cfg[0].out_h * 3 / 2);
					}else if(in_out_cfg[0].out_fmt == 0x51080400){
						dest_plane_num = 1;
						verify_size = (in_out_cfg[0].out_w * in_out_cfg[0].out_h);
					}else if(in_out_cfg[0].out_fmt == 0x23180888){
						dest_plane_num = 3;
						verify_size = (in_out_cfg[0].out_w * in_out_cfg[0].out_h * 3);
					}else{
		        		printf("unknown input format %x\n", in_out_cfg[0].out_fmt);
		        		goto exit;
		        	}

					if (in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr) {
						ai_test_op_set_unaligned_start_address(src_plane_num, src);
					}

					ret = vendor_ai_op_proc(net_path, VENDOR_AI_OP_PREPROC, &p_parm, src_plane_num, src, dest_plane_num, dest);
					if(in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr){
						if (ret) {
							if (0 != ai_test_op_check_unaligned_start_address(src_plane_num, src, ret)) {
								printf("fail to detect unaligned addr buf\n");
								goto exit;
							} else {
								ret = 0;
								goto exit;
							}
						} else {
							if (0 != ai_test_op_check_unaligned_start_address(src_plane_num, src, ret)) {
								ret = -1;
								printf("fail to detect unaligned addr buf\n");
								goto exit;
							}
						}
					}else {
						if (ret != 0) {
							printf("inference fail\n");
							goto exit;
						}
					}
				}
		        	
				ret = hd_common_mem_flush_cache((void*)output_mem.va, output_mem.size);
				if(HD_OK != ret) {
					printf("flush output mem failed.\n");
					goto exit;
				}
			}

			ret = verify_result((void*)output_mem.va,
								verify_size,
								&(in_out_cfg[0]),
								&(first_loop_data->second_loop_data.third_loop_data.fourth_loop_data));
			if (HD_OK != ret) {
				printf("net_path(%u) verify fail !!\n", net_path);
				goto exit;
			}
		}

		if(ai_test_free_fourth_buf(&(first_loop_data->second_loop_data.third_loop_data.fourth_loop_data), 
									&input_mem, &weight_mem, &output_mem)){
			printf("net_path(%lu) fail to release input/output/weight buffer\n", net_path);
			goto exit;
		}
	}

exit:
	if(ai_test_free_fourth_buf(&(first_loop_data->second_loop_data.third_loop_data.fourth_loop_data), 
								&input_mem, &weight_mem, &output_mem)){
		printf("net_path(%lu) fail to release input/output/weight buffer\n", net_path);
	}

	return ret;
}

static int third_loop(FIRST_LOOP_DATA *first_loop_data, OPERATOR_CONFIG *op_cfg)
{
	int ret = 0, i;

	if(!first_loop_data || !op_cfg){
		printf("first_loop_data(%p) or model_cfg(%p) is null\n", first_loop_data, op_cfg);
		return -1;
	}

	for(i = 0 ; i < first_loop_data->second_loop_data.third_loop_data.loop_count ; ++i){

		ret = vendor_ai_op_start(first_loop_data->net_path);
		if (HD_OK != ret) {
			printf("first_loop_data->net_path(%lu) start fail !!\n", first_loop_data->net_path);
			return ret;
		}

		ret = fourth_loop(first_loop_data, op_cfg);
		if (ret != HD_OK) {
			printf("fourth_loop() fail=%d, net_path=%d\n", ret, (int)first_loop_data->net_path);
			vendor_ai_net_stop(first_loop_data->net_path);
			return -1;
		}

		ret = vendor_ai_op_stop(first_loop_data->net_path);
		if (HD_OK != ret) {
			printf("net_path(%lu) stop fail !!\n", first_loop_data->net_path);
			return -1;
		}
	}

	return 0;
}

static HD_RESULT ai_test_operator_alloc_work_buf(NET_PATH_ID net_path, int op_opt, unsigned int max_fc_ll_work_buf_size, MEM_PARM *work_mem)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_OP_CFG_MAX wmax;
	VENDOR_AI_OP_CFG_WORKBUF wbuf = {0};
	
	if(work_mem == NULL){
		printf("net_path(%u) work_mem is NULL\r\n", net_path);
		return HD_ERR_FAIL;
	}
	memset(work_mem, 0, sizeof(MEM_PARM));

	switch (op_opt) {
		case AI_OP_FC: 
        {
			wmax.op = VENDOR_AI_OP_FC;
			ret = vendor_ai_op_get(net_path, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("net_path(%u) get work_buf fail\r\n", net_path);
		  	    return HD_ERR_FAIL;
		    }
		}
		break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_Y2Y_UV2UV:
		{
			wmax.op = VENDOR_AI_OP_PREPROC;
			ret = vendor_ai_op_get(net_path, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("net_path(%u) get work_buf fail\r\n", net_path);
		  	    return HD_ERR_FAIL;
		    }
//printf("net_path(%u) work_buf size = %lu\r\n", net_path, wmax.size);
		}
		break;
		case AI_OP_FC_LL_MODE:
		{
			if(max_fc_ll_work_buf_size == 0){
				printf("max_fc_ll_work_buf_size is 0\n");
				return HD_ERR_NG;
			}

			wmax.op = VENDOR_AI_OP_LIST;
			wmax.max_param[0] = max_fc_ll_work_buf_size;
			ret = vendor_ai_op_get(net_path, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("net_path(%u) get work_buf fail\r\n", net_path);
		  	    return HD_ERR_FAIL;
		    }
		}
		break;
		default:
			printf("net_path(%u) unknown op_opt(%d)\r\n", net_path, op_opt);
			return HD_ERR_FAIL;
		break;
	}
	
	// alloc work buff
    ret = ai_test_mem_alloc(work_mem, "op_work_buf", wmax.size);
    if (ret != HD_OK) {
   	    printf("net_path(%u) alloc work_buf fail\r\n", net_path);
		memset(work_mem, 0, sizeof(MEM_PARM));
  	    return HD_ERR_FAIL;
    }            

	//set work buffer
	wbuf.pa = (uintptr_t)work_mem->pa;
	wbuf.va = (uintptr_t)work_mem->va;
	wbuf.size = work_mem->size;
	switch (op_opt) {
		case AI_OP_FC: 
        {
			wbuf.op = VENDOR_AI_OP_FC;
		}
		break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_Y2Y_UV2UV:
        {
			wbuf.op = VENDOR_AI_OP_PREPROC;
		}
		break;
		case AI_OP_FC_LL_MODE:
        {
			wbuf.op = VENDOR_AI_OP_LIST;
		}
		break;
	}
	ret = vendor_ai_op_set(net_path, VENDOR_AI_OP_PARAM_CFG_WORKBUF, &wbuf);
    if (ret != HD_OK) {
   	    printf("net_path(%u) set work_buf fail\r\n", net_path);
  	    return HD_ERR_FAIL;
    }
	
	return ret;
}

static int second_loop(FIRST_LOOP_DATA *first_loop_data, OPERATOR_CONFIG *op_cfg, unsigned int max_fc_ll_work_buf_size)
{
	int ret = 0, final_ret = 0, i;

	if(!first_loop_data || !op_cfg){
		printf("first_loop_data(%p) or op_cfg(%p) is null\n", first_loop_data, op_cfg);
		return -1;
	}

	memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));
	for(i = 0 ; i < first_loop_data->second_loop_data.loop_count ; ++i){

		ret = vendor_ai_op_open(first_loop_data->net_path);
		if (ret != HD_OK) {
			printf("net open error=%d\n", ret);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}

		//alloc work buffer
		ret = ai_test_operator_alloc_work_buf(first_loop_data->net_path, op_cfg->op_opt, max_fc_ll_work_buf_size, &(first_loop_data->work_mem));
		if (ret != HD_OK) {
			printf("net_path(%d) alloc work buf error=%d\n", (int)first_loop_data->net_path, ret);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}

		ret = third_loop(first_loop_data, op_cfg);
		if (ret != HD_OK) {
			printf("third_loop() fail=%d, net_path=%d\n", ret, (int)first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}

next:
		ret = vendor_ai_op_close(first_loop_data->net_path);
		if (ret != HD_OK) {
			printf("net close error=%d\n", ret);
			global_fail = 1;
			final_ret = -1;
		}

		if(first_loop_data->work_mem.pa)
			ai_test_mem_free(&(first_loop_data->work_mem));
		memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));
	}

	if(first_loop_data->work_mem.pa)
		ai_test_mem_free(&(first_loop_data->work_mem));
	memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));

	return final_ret;
}

static int find_FC_LL_max_work_buf_size(FIRST_LOOP_DATA *first_loop_data)
{
	OPERATOR_CONFIG *op_cfg = NULL;
	unsigned int size = 0, size1, size2, size3;
	int i, in_file_cfg_idx = 0, in_file_count;
	NET_IN_OUT_CONFIG *in_out_cfg = NULL;
	NET_IN_FILE_CONFIG *in_file_cfg;

	if(!first_loop_data){
		printf("operator path_id(%d):first_loop_data is null\n", first_loop_data->net_path);
		return -1;
	}
	if(!first_loop_data->op_cfg){
		printf("operator path_id(%d):first_loop_data->op_cfg is null\n", first_loop_data->net_path);
		return -1;
	}

	for(i = 0 ; i < first_loop_data->second_cfg_count ; ++i){

		op_cfg = &(first_loop_data->op_cfg[i]);
		if(!op_cfg){
			printf("operator path_id(%d):op_cfg[%d] is null\n", first_loop_data->net_path, i);
			return 0;
		}
		if(op_cfg->op_opt != AI_OP_FC_LL_MODE)
			continue;
		in_file_cfg = op_cfg->in_file_cfg;
		in_file_count = op_cfg->in_file_cfg_count;
				
		for(in_file_cfg_idx = 0 ; in_file_cfg_idx < in_file_count ; in_file_cfg_idx++){

			if(in_file_cfg[in_file_cfg_idx].input_num != 2){
				printf("operator path_id(%d):AI_OP_FC_LL_MODE doesn't have 2 input image. current=%d\n", first_loop_data->net_path, in_file_cfg[in_file_cfg_idx].input_num);
				return 0;
			}
			in_out_cfg = in_file_cfg[in_file_cfg_idx].in_out_cfg;
			if(!in_out_cfg){
				printf("operator path_id(%d):in_file_cfg[%d] is null\n", first_loop_data->net_path, in_file_cfg_idx);
				return 0;
			}
			
			size1 = in_out_cfg[0].in_w * in_out_cfg[0].in_h;
			size2 = in_out_cfg[1].in_w * in_out_cfg[1].in_h;
			size3 = size2/size1;
			if(size3 > size)
				size = size3;
		}
	}
	
	printf("operator path_id(%d):max FC LL work buf size=%d\n", first_loop_data->net_path, size);
	return size;
}

int op_first_loop(FIRST_LOOP_DATA *first_loop_data)
{
	int ret = 0, final_ret = 0, op_idx = 0, i, fail_cnt = 0;
	unsigned int max_fc_ll_work_buf_size;

	if(!first_loop_data){
		printf("operator path_id(%d):first_loop_data is null\n", first_loop_data->net_path);
		return -1;
	}
	
	max_fc_ll_work_buf_size = find_FC_LL_max_work_buf_size(first_loop_data);

	for(i = 0 ; i < first_loop_data->loop_count ; ++i){

printf("operator(%d) start, path_id(%d)\n", first_loop_data->op_cfg[op_idx].op_opt, first_loop_data->net_path);

		ret = second_loop(first_loop_data, &(first_loop_data->op_cfg[op_idx]), max_fc_ll_work_buf_size);
		if(ret){
			printf("second_loop() fail=%d, path_id=%d\n", ret, (int)first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			fail_cnt++;
		}

printf("operator(%d) end, path_id(%d)\n", first_loop_data->op_cfg[op_idx].op_opt, first_loop_data->net_path);
		op_idx++;
		if(op_idx >= first_loop_data->second_cfg_count)
			op_idx = 0;
	}

	ai_test_lock_and_sync();
	if(final_ret == 0)
		first_loop_data->test_success = 1;
	else{
		first_loop_data->test_success = -1;
		printf("operator(%d) has %d failure\n", first_loop_data->op_cfg[op_idx].op_opt, fail_cnt);
	}
	ai_test_unlock();

	return final_ret;
}