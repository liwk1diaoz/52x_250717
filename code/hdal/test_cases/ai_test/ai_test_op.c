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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"
#include "ai_test_int.h"


// platform dependent
#if defined(__LINUX)
#include <signal.h>
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/signal.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(ai_test, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

#define ALLOC_WORKBUF_BY_USER 1

///////////////////////////////////////////////////////////////////////////////

static OP_PROC g_op[16] = {0};

#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
static UINT32 mau_mat_outid = 0;
static VENDOR_AI_BUF mau_mat_out;
#endif

extern UINT32 op_config_mode;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
extern AI_OP_MAU_CONFIG_DATA op_mau_config_data;
#endif
extern AI_OP_PREPROC_CONFIG_DATA op_preproc_config_data;

UINT32 ai_test_operator_get_fmt(UINT32 io_type, UINT32 frac_bit)
{
	switch(io_type) {
		case AI_OP_INT8:
		{
			return HD_VIDEO_PXLFMT_AI_SFIXED8(frac_bit);
		}
		break;
		case AI_OP_UINT8:
		{
			return HD_VIDEO_PXLFMT_AI_UFIXED8(frac_bit);
		}
		break;
		case AI_OP_INT16:
		{
			return HD_VIDEO_PXLFMT_AI_SFIXED16(frac_bit);
		}
		break;
		case AI_OP_UINT16:
		{
			return HD_VIDEO_PXLFMT_AI_UFIXED16(frac_bit);
		}
		break;
		case AI_OP_FP16:
		{
			return HD_VIDEO_PXLFMT_AI_FLOAT16;
		}
		break;
		case AI_OP_FP32:
		{
			return HD_VIDEO_PXLFMT_AI_FLOAT32;
		}
		break;
		default:
		break;
	}
	return 0;
}
	
HD_RESULT ai_test_operator_set_config(NET_PATH_ID net_path, int in_op_opt)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + net_path;
    p_op->op_opt = in_op_opt;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
	if(in_op_opt == AI_OP_MAU_MAT_MUL) {
		op_mau_config_data.in_a_w = MAU_MAT_MULTI_A_W;
		op_mau_config_data.in_a_h = MAU_MAT_MULTI_A_H;
		op_mau_config_data.in_a_frac_bit = MAU_MAT_MULTI_A_FRAC_BIT;
		op_mau_config_data.in_a_bit_depth = MAU_MAT_MULTI_A_BIT_DEPTH;
		op_mau_config_data.in_a_fmt = MAU_MAT_MULTI_A_FMT;
		strncpy(op_mau_config_data.in_a_file_path, MAU_MAT_MULTI_A_FILE_PATH, 256-1);
		op_mau_config_data.in_a_file_path[256-1] = '\0';
		op_mau_config_data.in_b_w = MAU_MAT_MULTI_B_W;
		op_mau_config_data.in_b_h = MAU_MAT_MULTI_B_H;
		op_mau_config_data.in_b_frac_bit = MAU_MAT_MULTI_B_FRAC_BIT;
		op_mau_config_data.in_b_bit_depth = MAU_MAT_MULTI_B_BIT_DEPTH;
		op_mau_config_data.in_b_fmt = MAU_MAT_MULTI_B_FMT;
		strncpy(op_mau_config_data.in_b_file_path, MAU_MAT_MULTI_B_FILE_PATH, 256-1);
		op_mau_config_data.in_b_file_path[256-1] = '\0';
		op_mau_config_data.out_c_bit_depth = MAU_MAT_MULTI_C_BIT_DEPTH;
		op_mau_config_data.out_c_fmt = MAU_MAT_MULTI_C_FMT;
	} else if(in_op_opt == AI_OP_MAU_MAT_ADD) {
		op_mau_config_data.in_a_w = MAU_MAT_ADD_A_W;
		op_mau_config_data.in_a_h = MAU_MAT_ADD_A_H;
		op_mau_config_data.in_a_frac_bit = MAU_MAT_ADD_A_FRAC_BIT;
		op_mau_config_data.in_a_bit_depth = MAU_MAT_ADD_A_BIT_DEPTH;
		op_mau_config_data.in_a_fmt = MAU_MAT_ADD_A_FMT;
		strncpy(op_mau_config_data.in_a_file_path, MAU_MAT_ADD_A_FILE_PATH, 256-1);
		op_mau_config_data.in_a_file_path[256-1] = '\0';
		op_mau_config_data.in_b_w = MAU_MAT_ADD_B_W;
		op_mau_config_data.in_b_h = MAU_MAT_ADD_B_H;
		op_mau_config_data.in_b_frac_bit = MAU_MAT_ADD_B_FRAC_BIT;
		op_mau_config_data.in_b_bit_depth = MAU_MAT_ADD_B_BIT_DEPTH;
		op_mau_config_data.in_b_fmt = MAU_MAT_ADD_B_FMT;
		strncpy(op_mau_config_data.in_b_file_path, MAU_MAT_ADD_B_FILE_PATH, 256-1);
		op_mau_config_data.in_b_file_path[256-1] = '\0';
		op_mau_config_data.out_c_bit_depth = MAU_MAT_ADD_C_BIT_DEPTH;
		op_mau_config_data.out_c_fmt = MAU_MAT_ADD_C_FMT;
	} else if(in_op_opt == AI_OP_MAU_MAT_SUB) {
		op_mau_config_data.in_a_w = MAU_MAT_SUB_A_W;
		op_mau_config_data.in_a_h = MAU_MAT_SUB_A_H;
		op_mau_config_data.in_a_frac_bit = MAU_MAT_SUB_A_FRAC_BIT;
		op_mau_config_data.in_a_bit_depth = MAU_MAT_SUB_A_BIT_DEPTH;
		op_mau_config_data.in_a_fmt = MAU_MAT_SUB_A_FMT;
		strncpy(op_mau_config_data.in_a_file_path, MAU_MAT_SUB_A_FILE_PATH, 256-1);
		op_mau_config_data.in_a_file_path[256-1] = '\0';
		op_mau_config_data.in_b_w = MAU_MAT_SUB_B_W;
		op_mau_config_data.in_b_h = MAU_MAT_SUB_B_H;
		op_mau_config_data.in_b_frac_bit = MAU_MAT_SUB_B_FRAC_BIT;
		op_mau_config_data.in_b_bit_depth = MAU_MAT_SUB_B_BIT_DEPTH;
		op_mau_config_data.in_b_fmt = MAU_MAT_SUB_B_FMT;
		strncpy(op_mau_config_data.in_b_file_path, MAU_MAT_SUB_B_FILE_PATH, 256-1);
		op_mau_config_data.in_b_file_path[256-1] = '\0';
		op_mau_config_data.out_c_fmt = MAU_MAT_SUB_C_FRAC_BIT;
		op_mau_config_data.out_c_bit_depth = MAU_MAT_SUB_C_BIT_DEPTH;
		op_mau_config_data.out_c_fmt = MAU_MAT_SUB_C_FMT;
	} else if(in_op_opt == AI_OP_MAU_L2_NORM) {
		op_mau_config_data.in_a_w = MAU_L2_NORM_A_W;
		op_mau_config_data.in_a_h = MAU_L2_NORM_A_H;
		op_mau_config_data.in_a_frac_bit = MAU_L2_NORM_A_FRAC_BIT;
		op_mau_config_data.in_a_bit_depth = MAU_L2_NORM_A_BIT_DEPTH;
		op_mau_config_data.in_a_fmt = MAU_L2_NORM_A_FMT;
		strncpy(op_mau_config_data.in_a_file_path, MAU_L2_NORM_A_FILE_PATH, 256-1);
		op_mau_config_data.in_a_file_path[256-1] = '\0';
		op_mau_config_data.out_c_fmt = MAU_L2_NORM_C_FRAC_BIT;
		op_mau_config_data.out_c_bit_depth = MAU_L2_NORM_C_BIT_DEPTH;
		op_mau_config_data.out_c_fmt = MAU_L2_NORM_C_FMT;
		op_mau_config_data.mau_l2_norm_iter = MAU_L2_NORM_ITER;
	} else if(in_op_opt == AI_OP_MAU_L2_NORM_INV) {
		op_mau_config_data.in_a_w = MAU_L2_NORM_INV_A_W;
		op_mau_config_data.in_a_h = MAU_L2_NORM_INV_A_H;
		op_mau_config_data.in_a_frac_bit = MAU_L2_NORM_INV_A_FRAC_BIT;
		op_mau_config_data.in_a_bit_depth = MAU_L2_NORM_INV_A_BIT_DEPTH;
		op_mau_config_data.in_a_fmt = MAU_L2_NORM_INV_A_FMT;
		strncpy(op_mau_config_data.in_a_file_path, MAU_L2_NORM_INV_A_FILE_PATH, 256-1);
		op_mau_config_data.in_a_file_path[256-1] = '\0';
		op_mau_config_data.out_c_fmt = MAU_L2_NORM_INV_C_FRAC_BIT;
		op_mau_config_data.out_c_bit_depth = MAU_L2_NORM_INV_C_BIT_DEPTH;
		op_mau_config_data.out_c_fmt = MAU_L2_NORM_INV_C_FMT;
		op_mau_config_data.mau_l2_norm_iter = MAU_L2_NORM_INV_ITER;
	} else if(in_op_opt == AI_OP_MAU_VECTOR_NORMALIZAION) {
		op_mau_config_data.in_a_w = MAU_VECTOR_NORMALIZAION_A_W;
		op_mau_config_data.in_a_h = MAU_VECTOR_NORMALIZAION_A_H;
		op_mau_config_data.in_a_frac_bit = MAU_VECTOR_NORMALIZAION_A_FRAC_BIT;
		op_mau_config_data.in_a_bit_depth = MAU_VECTOR_NORMALIZAION_A_BIT_DEPTH;
		op_mau_config_data.in_a_fmt = MAU_VECTOR_NORMALIZAION_A_FMT;
		strncpy(op_mau_config_data.in_a_file_path, MAU_VECTOR_NORMALIZAION_A_FILE_PATH, 256-1);
		op_mau_config_data.in_a_file_path[256-1] = '\0';
		op_mau_config_data.out_c_fmt = MAU_VECTOR_NORMALIZAION_C_FRAC_BIT;
		op_mau_config_data.out_c_bit_depth = MAU_VECTOR_NORMALIZAION_C_BIT_DEPTH;
		op_mau_config_data.out_c_fmt = MAU_VECTOR_NORMALIZAION_C_FMT;
		op_mau_config_data.mau_l2_norm_iter = MAU_VECTOR_NORMALIZAION_ITER;
	} else if(in_op_opt == AI_OP_MAU_TOPN_SORT) {
		op_mau_config_data.in_a_w = MAU_TOPN_SORT_A_W;
		op_mau_config_data.in_a_h = MAU_TOPN_SORT_A_H;
		op_mau_config_data.in_a_frac_bit = MAU_TOPN_SORT_A_FRAC_BIT;
		op_mau_config_data.in_a_bit_depth = MAU_TOPN_SORT_A_BIT_DEPTH;
		op_mau_config_data.in_a_fmt = MAU_TOPN_SORT_A_FMT;
		strncpy(op_mau_config_data.in_a_file_path, MAU_TOPN_SORT_A_FILE_PATH, 256-1);
		op_mau_config_data.in_a_file_path[256-1] = '\0';
		op_mau_config_data.out_c_frac_bit = MAU_TOPN_SORT_C_FRAC_BIT;
		op_mau_config_data.out_c_bit_depth = MAU_TOPN_SORT_C_BIT_DEPTH;
		op_mau_config_data.out_c_fmt = MAU_TOPN_SORT_C_FMT;
		op_mau_config_data.sort_idx_bit_depth = MAU_TOPN_SORT_IDX_BIT_DEPTH;
		op_mau_config_data.sort_idx_fmt = MAU_TOPN_SORT_IDX_FMT;
		op_mau_config_data.mau_sort_top_n = MAU_TOPN_SORT_N;
	}
#endif
	return ret;
}

HD_RESULT ai_test_operator_set_config_by_file(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + net_path;
	int in_op_opt = 0;

	if(op_config_mode == 0) {
		in_op_opt = AI_OP_PREPROC_USER;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
	} else {
		switch (op_mau_config_data.mau_cal_mode) {
			case VENDOR_AI_MAU_MAT_MUL:
			{
		    	in_op_opt = AI_OP_MAU_MAT_MUL;
			}
			break;
			case VENDOR_AI_MAU_MAT_ADD:
			{
		    	in_op_opt = AI_OP_MAU_MAT_ADD;
			}
			break;
			case VENDOR_AI_MAU_MAT_SUB:
			{
		    	in_op_opt = AI_OP_MAU_MAT_SUB;
			}
			break;
			case VENDOR_AI_MAU_L2_NORM:
			{
		    	in_op_opt = AI_OP_MAU_L2_NORM;
			}
			break;
			case VENDOR_AI_MAU_L2_NORM_INV:
			{
		    	in_op_opt = AI_OP_MAU_L2_NORM_INV;
			}
			break;
			case VENDOR_AI_MAU_VECTOR_NORMALIZAION:
			{
		    	in_op_opt = AI_OP_MAU_VECTOR_NORMALIZAION;
			}
			break;
			case VENDOR_AI_MAU_TOPN_SORT:
			{
		    	in_op_opt = AI_OP_MAU_TOPN_SORT;
			}
			break;
			default:
			break;
		}
#endif
	}
	p_op->op_opt = in_op_opt;
	return ret;
}

static HD_RESULT ai_test_operator_alloc_out_buf(NET_PATH_ID op_path, NET_PATH_ID in_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;
	NET_IN* p_in = g_in + in_path;
	UINT32 proc_id = p_op->proc_id;
	
	// alloc result buff
    switch (p_op->op_opt) {
	    case AI_OP_FC: //VENDOR_AI_OP_FC
	    case AI_OP_FC_LL_MODE:
        {
	        ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", SV_LENGTH*4);
	        if (ret != HD_OK) {
	       	    printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	      	    return HD_ERR_FAIL;
	        }
            else {
                printf("proc_id(%lu) alloc out_buf OK, size = %d\r\n", proc_id, SV_LENGTH*4);
            }
    	    ai_test_mem_fill(&p_op->output_mem, 1); 
	        ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./user_out_ori.bin");
    	}
		break;
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		{
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_RGB_BUFSIZE(SCALE_DIM_W, SCALE_DIM_H));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
            else {
                printf("proc_id(%lu) alloc out_buf OK, size = %d\r\n", proc_id, AI_RGB_BUFSIZE(SCALE_DIM_W, SCALE_DIM_H));
            }
		}
		break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_Y2Y_UV2UV:
        {
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_RGB_BUFSIZE(p_in->in_cfg.w, p_in->in_cfg.h));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_RGB_BUFSIZE(p_in->in_cfg.w, p_in->in_cfg.h));
            }
        }
        break;
		case AI_OP_PREPROC_USER:
		{
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_RGB_BUFSIZE(op_preproc_config_data.dst_0_width, op_preproc_config_data.dst_0_height));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_RGB_BUFSIZE(op_preproc_config_data.dst_0_width, op_preproc_config_data.dst_0_height));
            }
        }
        break;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_MUL:
        {
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_b_h, 32)); //always need h * w * 4bytes
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_b_h, 32));
            }

			ret = ai_test_mem_alloc(&p_op->output2_mem, "user_tmp_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_b_h, 32)); //always need h * w * 4bytes
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc tmp_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc tmp_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_b_h, 32));
            }
        }
        break;
		case AI_OP_MAU_MAT_ADD:
        {
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_a_w, op_mau_config_data.out_c_bit_depth));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_a_w, op_mau_config_data.out_c_bit_depth));
            }
        }
        break;
		case AI_OP_MAU_MAT_SUB:
        {
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_a_w, op_mau_config_data.out_c_bit_depth));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_a_w, op_mau_config_data.out_c_bit_depth));
            }
        }
        break;
		case AI_OP_MAU_L2_NORM:
        {
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, 1, op_mau_config_data.out_c_bit_depth));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, 1, op_mau_config_data.out_c_bit_depth));
            }
        }
        break;
		case AI_OP_MAU_L2_NORM_INV:
        {
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, 1, op_mau_config_data.out_c_bit_depth));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, 1, op_mau_config_data.out_c_bit_depth));
            }
        }
        break;
		case AI_OP_MAU_VECTOR_NORMALIZAION:
        {
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_a_w, op_mau_config_data.out_c_bit_depth));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_a_w, op_mau_config_data.out_c_bit_depth));
            }
        }
        break;
		case AI_OP_MAU_TOPN_SORT:
        {
			ret = ai_test_mem_alloc(&p_op->output_mem, "user_out_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.mau_sort_top_n, op_mau_config_data.out_c_bit_depth));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc out_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc out_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.mau_sort_top_n, op_mau_config_data.out_c_bit_depth));
            }

			ret = ai_test_mem_alloc(&p_op->output2_mem, "user_idx_buf", AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.mau_sort_top_n, op_mau_config_data.sort_idx_bit_depth));
			if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc tmp_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
			else {
                printf("proc_id(%lu) alloc tmp_buf OK, size = %lu\r\n", proc_id, AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.mau_sort_top_n, op_mau_config_data.sort_idx_bit_depth));
            }
        }
        break;
#endif
        default:
        break;
    }
	return ret;
}

#if ALLOC_WORKBUF_BY_USER
static HD_RESULT ai_test_operator_alloc_work_buf(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;
	UINT32 proc_id = p_op->proc_id;
	
	// alloc work buff
    ret = ai_test_mem_alloc(&p_op->work_mem, "op_work_buf", p_op->need_size);
    if (ret != HD_OK) {
   	    printf("proc_id(%lu) alloc work_buf fail\r\n", proc_id);
  	    return HD_ERR_FAIL;
    }
	else {
		printf("proc_id(%lu) alloc work_buf OK, size = %lu\r\n", proc_id, p_op->need_size);
	}
            
	return ret;
}

static HD_RESULT ai_test_operator_free_work_buf(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;
	
	// free work buff
	if(p_op->work_mem.size > 0) {
		ai_test_mem_free(&p_op->work_mem);
	}
	
	return ret;
}

#endif

static HD_RESULT ai_test_operator_free_out_buf(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;

	// free buffer
	switch (p_op->op_opt) {
		case AI_OP_FC: //VENDOR_AI_OP_FC
		case AI_OP_FC_LL_MODE:
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_USER:
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_ADD:
		case AI_OP_MAU_MAT_SUB:
		case AI_OP_MAU_L2_NORM:
		case AI_OP_MAU_L2_NORM_INV:
		case AI_OP_MAU_VECTOR_NORMALIZAION:
#endif
        {
            ai_test_mem_free(&p_op->output_mem);
        }
        break;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_MUL:
		case AI_OP_MAU_TOPN_SORT:
        {
            ai_test_mem_free(&p_op->output_mem);
			ai_test_mem_free(&p_op->output2_mem);
        }
        break;
#endif
        default:
        break;
	}
	
	return ret;
}
HD_RESULT ai_test_operator_init(void)
{
	HD_RESULT ret = HD_OK;
	int  i;	
	for (i = 0; i < 16; i++) {
		OP_PROC* p_op = g_op + i;
		p_op->proc_id = i;
	}
	return ret;
}

HD_RESULT ai_test_operator_open(NET_PATH_ID op_path, NET_PATH_ID in_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;
	NET_IN* p_in = g_in + in_path;
	UINT32 proc_id = p_op->proc_id;

	// alloc buffer
	switch (p_op->op_opt) {
		case AI_OP_FC: 
		case AI_OP_FC_LL_MODE:
        {
	        ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf", SV_FEA_LENGTH);
	        if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
	        ret = ai_test_mem_alloc(&p_op->weight_mem, "user_weight_buf", SV_LENGTH*SV_FEA_LENGTH);
	        if (ret != HD_OK) {
	        	printf("proc_id(%lu) alloc weight_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }

	        // fill buffer
	        ai_test_mem_fill(&p_op->input_mem, 1); 
	        ai_test_mem_fill(&p_op->weight_mem, 1); 
	        // save buffer
	        ai_test_mem_save(&p_op->input_mem, p_op->input_mem.size, "./user_in.bin"); 
	        ai_test_mem_save(&p_op->weight_mem, p_op->weight_mem.size, "./user_weight.bin");
		}
        break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_Y2Y_UV2UV:
		case AI_OP_PREPROC_USER:
        {
			if(p_op->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE)
            	ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf", 2*AI_RGB_BUFSIZE(p_in->in_cfg.w, p_in->in_cfg.h));
			else
				ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf", AI_RGB_BUFSIZE(p_in->in_cfg.w, p_in->in_cfg.h));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            INT32 file_len;
            file_len = ai_test_mem_load(&p_op->input_mem, p_in->in_cfg.input_filename);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", p_in->in_cfg.input_filename);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", p_in->in_cfg.input_filename, file_len);
        }
        break;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_MUL:
        {
			INT32 file_len;
			ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf_1", AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            if (strlen(op_mau_config_data.in_a_file_path) == 0) {
				// fill buffer
		        ai_test_mem_fill(&p_op->input_mem, 1); 
		        // save buffer
		        ai_test_mem_save(&p_op->input_mem, p_op->input_mem.size, "./mau_mat_mul_in_a.bin"); 
	        } else {
	            file_len = ai_test_mem_load(&p_op->input_mem, op_mau_config_data.in_a_file_path);
		        if (file_len < 0) {
			        printf("load buf(%s) fail\r\n", op_mau_config_data.in_a_file_path);
			        return HD_ERR_NG;
		        }
	        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_a_file_path, file_len);
            }
			ret = ai_test_mem_alloc(&p_op->input2_mem, "user_in_buf_2", AI_MAU_BUFSIZE(op_mau_config_data.in_b_w, op_mau_config_data.in_b_h, op_mau_config_data.in_b_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            if (strlen(op_mau_config_data.in_a_file_path) == 0) {
				// fill buffer
		        ai_test_mem_fill(&p_op->input2_mem, 1); 
		        // save buffer
		        ai_test_mem_save(&p_op->input2_mem, p_op->input2_mem.size, "./mau_mat_mul_in_b.bin"); 
	        } else {
	            file_len = ai_test_mem_load(&p_op->input2_mem, op_mau_config_data.in_b_file_path);
		        if (file_len < 0) {
			        printf("load buf(%s) fail\r\n", op_mau_config_data.in_b_file_path);
			        return HD_ERR_NG;
		        }
	        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_b_file_path, file_len);
	        }
        }
        break;
		case AI_OP_MAU_MAT_ADD:
        {
			INT32 file_len;
			ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf_1", AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            file_len = ai_test_mem_load(&p_op->input_mem, op_mau_config_data.in_a_file_path);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", op_mau_config_data.in_a_file_path);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_a_file_path, file_len);

			ret = ai_test_mem_alloc(&p_op->input2_mem, "user_in_buf_2", AI_MAU_BUFSIZE(op_mau_config_data.in_b_w, op_mau_config_data.in_b_h, op_mau_config_data.in_b_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            file_len = ai_test_mem_load(&p_op->input2_mem, op_mau_config_data.in_b_file_path);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", op_mau_config_data.in_b_file_path);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_b_file_path, file_len);
        }
        break;
		case AI_OP_MAU_MAT_SUB:
        {
			INT32 file_len;
			ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf_1", AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            file_len = ai_test_mem_load(&p_op->input_mem, op_mau_config_data.in_a_file_path);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", op_mau_config_data.in_a_file_path);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_a_file_path, file_len);

			ret = ai_test_mem_alloc(&p_op->input2_mem, "user_in_buf_2", AI_MAU_BUFSIZE(op_mau_config_data.in_b_w, op_mau_config_data.in_b_h, op_mau_config_data.in_b_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            file_len = ai_test_mem_load(&p_op->input2_mem, op_mau_config_data.in_b_file_path);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", op_mau_config_data.in_b_file_path);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_b_file_path, file_len);
        }
        break;
		case AI_OP_MAU_L2_NORM:
        {
			INT32 file_len;
			ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf_1", AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            file_len = ai_test_mem_load(&p_op->input_mem, op_mau_config_data.in_a_file_path);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", op_mau_config_data.in_a_file_path);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_a_file_path, file_len);
        }
        break;
		case AI_OP_MAU_L2_NORM_INV:
        {
			INT32 file_len;
			ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf_1", AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            file_len = ai_test_mem_load(&p_op->input_mem, op_mau_config_data.in_a_file_path);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", op_mau_config_data.in_a_file_path);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_a_file_path, file_len);
        }
        break;
		case AI_OP_MAU_VECTOR_NORMALIZAION:
        {
			INT32 file_len;
			ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf_1", AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            file_len = ai_test_mem_load(&p_op->input_mem, op_mau_config_data.in_a_file_path);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", op_mau_config_data.in_a_file_path);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_a_file_path, file_len);
        }
        break;
		case AI_OP_MAU_TOPN_SORT:
        {
			INT32 file_len;
			ret = ai_test_mem_alloc(&p_op->input_mem, "user_in_buf_1", AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            file_len = ai_test_mem_load(&p_op->input_mem, op_mau_config_data.in_a_file_path);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", op_mau_config_data.in_a_file_path);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %ld\r\n", op_mau_config_data.in_a_file_path, file_len);
        }
        break;
#endif
        default:
        {
            printf("Unknown op_opt\r\n");
	        return HD_ERR_LIMIT;
        }
        break;
	}
		
	ret = vendor_ai_op_open(op_path);
	
	return ret;
}

HD_RESULT ai_test_operator_close(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;

	// close	
	ret = vendor_ai_op_close(op_path);
    // free buffer
	switch (p_op->op_opt) {
		case AI_OP_FC: //VENDOR_AI_OP_FC
		case AI_OP_FC_LL_MODE:
        {
			ai_test_mem_free(&p_op->input_mem);
			ai_test_mem_free(&p_op->weight_mem);
		}
        break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_Y2Y_UV2UV:
		case AI_OP_PREPROC_USER:
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_L2_NORM:
		case AI_OP_MAU_L2_NORM_INV:
		case AI_OP_MAU_VECTOR_NORMALIZAION:
		case AI_OP_MAU_TOPN_SORT:
#endif
        {
            ai_test_mem_free(&p_op->input_mem);
        }
        break;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_MUL:
		case AI_OP_MAU_MAT_ADD:
		case AI_OP_MAU_MAT_SUB:
        {
            ai_test_mem_free(&p_op->input_mem);
			ai_test_mem_free(&p_op->input2_mem);
        }
        break;
#endif
        default:
        break;
	}
	return ret;
}

HD_RESULT ai_test_operator_get_workbuf_size(NET_PATH_ID op_path)
{
#if ALLOC_WORKBUF_BY_USER
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;
	UINT32 proc_id = p_op->proc_id;
	VENDOR_AI_OP_CFG_MAX wmax = {0};

	switch (p_op->op_opt) {
		case AI_OP_FC: 
        {
			wmax.op = VENDOR_AI_OP_FC;
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_USER:
		{
			wmax.op = VENDOR_AI_OP_PREPROC;
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
		case AI_OP_FC_LL_MODE:
		{
			wmax.op = VENDOR_AI_OP_LIST;
			wmax.max_param[0] = SV_LENGTH;
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_MUL:
		{
			wmax.op = VENDOR_AI_OP_MAU;
			wmax.max_param[0] = VENDOR_AI_MAU_MAT_MUL; //mau_cal_mode
			wmax.max_param[1] = op_mau_config_data.in_a_w; //input A(intput 1) width, only needed when doing Matrix multiplication
			wmax.max_param[2] = op_mau_config_data.in_a_h; //input A(intput 1) height, only needed when doing Matrix multiplication
			wmax.max_param[3] = op_mau_config_data.in_a_bit_depth/8; //input A(intput 1) byte, only needed when doing Matrix multiplication
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
		case AI_OP_MAU_MAT_ADD:
		{
			wmax.op = VENDOR_AI_OP_MAU;
			wmax.max_param[0] = VENDOR_AI_MAU_MAT_ADD; //mau_cal_mode
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
		case AI_OP_MAU_MAT_SUB:
		{
			wmax.op = VENDOR_AI_OP_MAU;
			wmax.max_param[0] = VENDOR_AI_MAU_MAT_SUB; //mau_cal_mode
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
		case AI_OP_MAU_L2_NORM:
		{
			wmax.op = VENDOR_AI_OP_MAU;
			wmax.max_param[0] = VENDOR_AI_MAU_L2_NORM; //mau_cal_mode
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
		case AI_OP_MAU_L2_NORM_INV:
		{
			wmax.op = VENDOR_AI_OP_MAU;
			wmax.max_param[0] = VENDOR_AI_MAU_L2_NORM_INV; //mau_cal_mode
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
		case AI_OP_MAU_VECTOR_NORMALIZAION:
		{
			wmax.op = VENDOR_AI_OP_MAU;
			wmax.max_param[0] = VENDOR_AI_MAU_VECTOR_NORMALIZAION; //mau_cal_mode
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
		case AI_OP_MAU_TOPN_SORT:
		{
			wmax.op = VENDOR_AI_OP_MAU;
			wmax.max_param[0] = VENDOR_AI_MAU_TOPN_SORT; //mau_cal_mode
			ret = vendor_ai_op_get(proc_id, VENDOR_AI_OP_PARAM_CFG_MAX, &wmax);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) get work_buf fail\r\n", proc_id);
		  	    return HD_ERR_FAIL;
		    }
			else {
				printf("proc_id(%lu) work_buf size = %lu\r\n", proc_id, wmax.size);
			}
		}
		break;
#endif
	}
	p_op->need_size = wmax.size;
	return ret;
#endif
}

HD_RESULT ai_test_operator_set_workbuf(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
#if ALLOC_WORKBUF_BY_USER
	OP_PROC* p_op = g_op + op_path;
	UINT32 proc_id = p_op->proc_id;
	
	VENDOR_AI_OP_CFG_WORKBUF wbuf = {0};

	//alloc work buffer
	ret = ai_test_operator_alloc_work_buf(op_path);

	//set work buffer
	wbuf.pa = (&p_op->work_mem)->pa;
	wbuf.va = (&p_op->work_mem)->va;
	wbuf.size = (&p_op->work_mem)->size;
	switch (p_op->op_opt) {
		case AI_OP_FC: 
        {
			wbuf.op = VENDOR_AI_OP_FC;
		}
		break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
		case AI_OP_PREPROC_USER:
        {
			wbuf.op = VENDOR_AI_OP_PREPROC;
		}
		break;
		case AI_OP_FC_LL_MODE:
        {
			wbuf.op = VENDOR_AI_OP_LIST;
		}
		break;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_MUL:
		case AI_OP_MAU_MAT_ADD:
		case AI_OP_MAU_MAT_SUB:
		case AI_OP_MAU_L2_NORM:
		case AI_OP_MAU_L2_NORM_INV:
		case AI_OP_MAU_VECTOR_NORMALIZAION:
		case AI_OP_MAU_TOPN_SORT:
        {
			wbuf.op = VENDOR_AI_OP_MAU;
		}
		break;
#endif
	}
	ret = vendor_ai_op_set(proc_id, VENDOR_AI_OP_PARAM_CFG_WORKBUF, &wbuf);
#endif
	return ret;
}

HD_RESULT ai_test_operator_clear_workbuf(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
#if ALLOC_WORKBUF_BY_USER
	//free work buf
	ret = ai_test_operator_free_work_buf(op_path);
#endif
	return ret;
}

static VOID *operator_user_thread(VOID *arg);

HD_RESULT ai_test_operator_user_start(TEST_NETWORK *p_stream)
{
	HD_RESULT ret = HD_OK;
	UINT32 proc_id = p_stream->op_path;

	p_stream->op_start = 0;
	p_stream->op_exit = 0;
	p_stream->op_oneshot = 0;

	ret = vendor_ai_op_start(proc_id);
	
	ret = pthread_create(&p_stream->op_thread_id, NULL, operator_user_thread, (VOID*)(p_stream));
	if (ret < 0) {
		return HD_ERR_FAIL;
	}

	p_stream->op_start = 1;
	p_stream->op_exit = 0;
	p_stream->op_oneshot = 0;
	
	return ret;
}

HD_RESULT ai_test_operator_user_oneshot(TEST_NETWORK *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->op_oneshot = 1;
	return ret;
}

HD_RESULT ai_test_operator_user_stop(TEST_NETWORK *p_stream)
{
	HD_RESULT ret = HD_OK;
	UINT32 proc_id = p_stream->op_path;
	p_stream->op_exit = 1;
	
	pthread_join(p_stream->op_thread_id, NULL);

	ret = vendor_ai_op_stop(proc_id);

	ai_test_operator_clear_workbuf(proc_id);

	return ret;
}

HD_RESULT ai_test_operator_add_llcmd(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;

	switch (p_op->op_opt) {
		case AI_OP_FC_LL_MODE: 
		{
		    // 2. flush input
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->weight_mem)->va, (&p_op->weight_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
							
			VENDOR_AI_BUF src[2] = {0};
			VENDOR_AI_BUF dest[1] = {0};
			MEM_PARM* in_buf = &p_op->input_mem;
			MEM_PARM* out_buf = &p_op->output_mem;
			MEM_PARM* weight_buf = &p_op->weight_mem;
			//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
		    
				//set src1 as 1d tensor
			src[0].sign = MAKEFOURCC('A','B','U','F');
			src[0].ddr_id = 0;
			src[0].va = in_buf->va; //< input address	 (size = SV_FEA_LENGTH bytes)
			src[0].pa = in_buf->pa;
			src[0].size = SV_FEA_LENGTH;
			src[0].fmt = HD_VIDEO_PXLFMT_AI_SFIXED8(0); //fixpoint s7.0
			src[0].width = SV_FEA_LENGTH;
			src[0].height = 1;
			src[0].channel = 1;
			src[0].batch_num = 1;
			
			//set src2 as 2d tensor
			src[1].sign = MAKEFOURCC('A','B','U','F');
			src[1].ddr_id = 0;
			src[1].va = weight_buf->va; //< sv weight address (size = SV_LENGTH*SV_FEA_LENGTH bytes)
			src[1].pa = weight_buf->pa;
			src[1].size = SV_FEA_LENGTH*SV_LENGTH;
			src[1].fmt = HD_VIDEO_PXLFMT_AI_SFIXED8(0); //fixpoint s7.0
			src[1].width = SV_FEA_LENGTH;
			src[1].height = SV_LENGTH;
			src[1].channel = 1;
			src[1].batch_num = 1;
			
			//set dest as 1d tensor
			dest[0].sign = MAKEFOURCC('A','B','U','F');
			dest[0].ddr_id = 0;
			dest[0].va = out_buf->va; //< output address	 (size = SV_LENGTH*4 bytes)
			dest[0].pa = out_buf->pa;
			dest[0].size = SV_LENGTH*4;
			dest[0].fmt = HD_VIDEO_PXLFMT_AI_SFIXED32(0); //fixpoint s31.0
			dest[0].width = SV_LENGTH;
			dest[0].height = 1;
			dest[0].channel = 1;
			dest[0].batch_num = 1;

			ret = vendor_ai_op_add(op_path, VENDOR_AI_OP_LIST, NULL, 2, src, 1, dest);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) vendor_ai_op_add fail\r\n", op_path);
		    }
			
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->work_mem)->va, (&p_op->work_mem)->size);
			if(HD_OK != ret) {
		    	printf("flush cache failed.\n");
			}
		}
		break;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_MUL: 
		{
			VENDOR_AI_OP_MAU_PARAM p_parm = {0};
		    // 2. flush input
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input2_mem)->va, (&p_op->input2_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output2_mem)->va, (&p_op->output2_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
							
			VENDOR_AI_BUF src[2] = {0};
			VENDOR_AI_BUF dest[2] = {0};
			MEM_PARM* in_A_buf = &p_op->input_mem;
			MEM_PARM* in_B_buf = &p_op->input2_mem;
			MEM_PARM* out_buf = &p_op->output_mem;
			MEM_PARM* tmp_buf = &p_op->output2_mem;
			
			//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
		    
			//set src1 as 1d tensor
			src[0].sign = MAKEFOURCC('A','B','U','F');
			src[0].ddr_id = 0;
			src[0].va = in_A_buf->va; //< input address
			src[0].pa = in_A_buf->pa;
			src[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth);
			src[0].fmt = op_mau_config_data.in_a_fmt;
			src[0].width = op_mau_config_data.in_a_w;
			src[0].height = op_mau_config_data.in_a_h;
			src[0].channel = 1;
			src[0].batch_num = 1;
			src[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.in_a_bit_depth/8;
			
			//set src2 as 2d tensor
			src[1].sign = MAKEFOURCC('A','B','U','F');
			src[1].ddr_id = 0;
			src[1].va = in_B_buf->va; //< input address
			src[1].pa = in_B_buf->pa;
			src[1].size = AI_MAU_BUFSIZE(op_mau_config_data.in_b_w, op_mau_config_data.in_b_h, op_mau_config_data.in_b_bit_depth);
			src[1].fmt = op_mau_config_data.in_b_fmt;
			src[1].width = op_mau_config_data.in_b_w;
			src[1].height = op_mau_config_data.in_b_h;
			src[1].channel = 1;
			src[1].batch_num = 1;
			src[1].line_ofs = op_mau_config_data.in_b_w * op_mau_config_data.in_b_bit_depth/8;
			
			//set dest1 as 2d tensor
			dest[0].sign = MAKEFOURCC('A','B','U','F');
			dest[0].ddr_id = 0;
			dest[0].va = out_buf->va;
			dest[0].pa = out_buf->pa;
			dest[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_b_h, 32); //always need h * w * 4bytes
			dest[0].fmt = op_mau_config_data.out_c_fmt;
			dest[0].width = op_mau_config_data.in_b_h;
			dest[0].height = op_mau_config_data.in_a_h;
			dest[0].channel = 1;
			dest[0].batch_num = 1;
			dest[0].line_ofs = op_mau_config_data.in_b_h * op_mau_config_data.out_c_bit_depth/8;

			//set dest2 as 2d tensor
			dest[1].sign = MAKEFOURCC('A','B','U','F');
			dest[1].ddr_id = 0;
			dest[1].va = tmp_buf->va;
			dest[1].pa = tmp_buf->pa;
			dest[1].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_h, op_mau_config_data.in_b_h, 32); //always need h * w * 4bytes
			dest[1].fmt = op_mau_config_data.out_c_fmt;
			dest[1].width = op_mau_config_data.in_b_h;
			dest[1].height = op_mau_config_data.in_a_h;
			dest[1].channel = 1;
			dest[1].batch_num = 1;
			dest[1].line_ofs = op_mau_config_data.in_b_h * op_mau_config_data.out_c_bit_depth/8;

			p_parm.mau_cal_mode = VENDOR_AI_MAU_MAT_MUL;

			ret = vendor_ai_op_add(op_path, VENDOR_AI_OP_MAU, &p_parm, 2, src, 2, dest);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) vendor_ai_op_add fail\r\n", op_path);
		    }

			mau_mat_outid = p_parm.mau_mat_outid;
			memcpy(&mau_mat_out, p_parm.mau_mat_out, sizeof(VENDOR_AI_BUF));
				
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->work_mem)->va, (&p_op->work_mem)->size);
			if(HD_OK != ret) {
		    	printf("flush cache failed.\n");
			}
		}
		break;
		case AI_OP_MAU_MAT_ADD: 
		{
			VENDOR_AI_OP_MAU_PARAM p_parm = {0};
		    // 2. flush input
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input2_mem)->va, (&p_op->input2_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
							
			VENDOR_AI_BUF src[2] = {0};
			VENDOR_AI_BUF dest[1] = {0};
			MEM_PARM* in_A_buf = &p_op->input_mem;
			MEM_PARM* in_B_buf = &p_op->input2_mem;
			MEM_PARM* out_buf = &p_op->output_mem;
			
			//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
		    
			//set src1 as 1d tensor
			src[0].sign = MAKEFOURCC('A','B','U','F');
			src[0].ddr_id = 0;
			src[0].va = in_A_buf->va; //< input address
			src[0].pa = in_A_buf->pa;
			src[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth);
			src[0].fmt = op_mau_config_data.in_a_fmt;
			src[0].width = op_mau_config_data.in_a_w;
			src[0].height = op_mau_config_data.in_a_h;
			src[0].channel = 1;
			src[0].batch_num = 1;
			src[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.in_a_bit_depth/8;
			
			//set src2 as 2d tensor
			src[1].sign = MAKEFOURCC('A','B','U','F');
			src[1].ddr_id = 0;
			src[1].va = in_B_buf->va; //< input address
			src[1].pa = in_B_buf->pa;
			src[1].size = AI_MAU_BUFSIZE(op_mau_config_data.in_b_w, op_mau_config_data.in_b_h, op_mau_config_data.in_b_bit_depth);
			src[1].fmt = op_mau_config_data.in_b_fmt;
			src[1].width = op_mau_config_data.in_b_w;
			src[1].height = op_mau_config_data.in_b_h;
			src[1].channel = 1;
			src[1].batch_num = 1;
			src[1].line_ofs = op_mau_config_data.in_b_w * op_mau_config_data.in_b_bit_depth/8;
			
			//set dest1 as 2d tensor
			dest[0].sign = MAKEFOURCC('A','B','U','F');
			dest[0].ddr_id = 0;
			dest[0].va = out_buf->va;
			dest[0].pa = out_buf->pa;
			dest[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.out_c_bit_depth);
			dest[0].fmt = op_mau_config_data.out_c_fmt;
			dest[0].width = op_mau_config_data.in_a_w;
			dest[0].height = op_mau_config_data.in_a_h;
			dest[0].channel = 1;
			dest[0].batch_num = 1;
			dest[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.out_c_bit_depth/8;

			p_parm.mau_cal_mode = VENDOR_AI_MAU_MAT_ADD;

			ret = vendor_ai_op_add(op_path, VENDOR_AI_OP_MAU, &p_parm, 2, src, 1, dest);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) vendor_ai_op_add fail\r\n", op_path);
		    }
			
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->work_mem)->va, (&p_op->work_mem)->size);
			if(HD_OK != ret) {
		    	printf("flush cache failed.\n");
			}
		}
		break;
		case AI_OP_MAU_MAT_SUB: 
		{
			VENDOR_AI_OP_MAU_PARAM p_parm = {0};
		    // 2. flush input
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input2_mem)->va, (&p_op->input2_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
							
			VENDOR_AI_BUF src[2] = {0};
			VENDOR_AI_BUF dest[1] = {0};
			MEM_PARM* in_A_buf = &p_op->input_mem;
			MEM_PARM* in_B_buf = &p_op->input2_mem;
			MEM_PARM* out_buf = &p_op->output_mem;
			
			//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
		    
			//set src1 as 1d tensor
			src[0].sign = MAKEFOURCC('A','B','U','F');
			src[0].ddr_id = 0;
			src[0].va = in_A_buf->va; //< input address
			src[0].pa = in_A_buf->pa;
			src[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth);
			src[0].fmt = op_mau_config_data.in_a_fmt;
			src[0].width = op_mau_config_data.in_a_w;
			src[0].height = op_mau_config_data.in_a_h;
			src[0].channel = 1;
			src[0].batch_num = 1;
			src[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.in_a_bit_depth/8;
			
			//set src2 as 2d tensor
			src[1].sign = MAKEFOURCC('A','B','U','F');
			src[1].ddr_id = 0;
			src[1].va = in_B_buf->va; //< input address
			src[1].pa = in_B_buf->pa;
			src[1].size = AI_MAU_BUFSIZE(op_mau_config_data.in_b_w, op_mau_config_data.in_b_h, op_mau_config_data.in_b_bit_depth);
			src[1].fmt = op_mau_config_data.in_b_fmt;
			src[1].width = op_mau_config_data.in_b_w;
			src[1].height = op_mau_config_data.in_b_h;
			src[1].channel = 1;
			src[1].batch_num = 1;
			src[1].line_ofs = op_mau_config_data.in_b_w * op_mau_config_data.in_b_bit_depth/8;
			
			//set dest1 as 2d tensor
			dest[0].sign = MAKEFOURCC('A','B','U','F');
			dest[0].ddr_id = 0;
			dest[0].va = out_buf->va;
			dest[0].pa = out_buf->pa;
			dest[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.out_c_bit_depth);
			dest[0].fmt = op_mau_config_data.out_c_fmt;
			dest[0].width = op_mau_config_data.in_a_w;
			dest[0].height = op_mau_config_data.in_a_h;
			dest[0].channel = 1;
			dest[0].batch_num = 1;
			dest[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.out_c_bit_depth/8;

			p_parm.mau_cal_mode = VENDOR_AI_MAU_MAT_SUB;

			ret = vendor_ai_op_add(op_path, VENDOR_AI_OP_MAU, &p_parm, 2, src, 1, dest);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) vendor_ai_op_add fail\r\n", op_path);
		    }
			
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->work_mem)->va, (&p_op->work_mem)->size);
			if(HD_OK != ret) {
		    	printf("flush cache failed.\n");
			}
		}
		break;
		case AI_OP_MAU_L2_NORM: 
		{
			VENDOR_AI_OP_MAU_PARAM p_parm = {0};
		    // 2. flush input
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
							
			VENDOR_AI_BUF src[1] = {0};
			VENDOR_AI_BUF dest[1] = {0};
			MEM_PARM* in_A_buf = &p_op->input_mem;
			MEM_PARM* out_buf = &p_op->output_mem;
			
			//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
		    
			//set src1 as 1d tensor
			src[0].sign = MAKEFOURCC('A','B','U','F');
			src[0].ddr_id = 0;
			src[0].va = in_A_buf->va; //< input address
			src[0].pa = in_A_buf->pa;
			src[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth);
			src[0].fmt = op_mau_config_data.in_a_fmt;
			src[0].width = op_mau_config_data.in_a_w;
			src[0].height = op_mau_config_data.in_a_h;
			src[0].channel = 1;
			src[0].batch_num = 1;
			src[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.in_a_bit_depth/8;
			
			//set dest1 as 2d tensor
			dest[0].sign = MAKEFOURCC('A','B','U','F');
			dest[0].ddr_id = 0;
			dest[0].va = out_buf->va;
			dest[0].pa = out_buf->pa;
			dest[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, 1, op_mau_config_data.out_c_bit_depth);
			dest[0].fmt = op_mau_config_data.out_c_fmt;
			dest[0].width = op_mau_config_data.in_a_w;
			dest[0].height = 1;
			dest[0].channel = 1;
			dest[0].batch_num = 1;
			dest[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.out_c_bit_depth/8;

			p_parm.mau_cal_mode = VENDOR_AI_MAU_L2_NORM;
			p_parm.mau_l2_norm_iter = op_mau_config_data.mau_l2_norm_iter;

			ret = vendor_ai_op_add(op_path, VENDOR_AI_OP_MAU, &p_parm, 1, src, 1, dest);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) vendor_ai_op_add fail\r\n", op_path);
		    }
			
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->work_mem)->va, (&p_op->work_mem)->size);
			if(HD_OK != ret) {
		    	printf("flush cache failed.\n");
			}
		}
		break;
		case AI_OP_MAU_L2_NORM_INV: 
		{
			VENDOR_AI_OP_MAU_PARAM p_parm = {0};
		    // 2. flush input
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
							
			VENDOR_AI_BUF src[1] = {0};
			VENDOR_AI_BUF dest[1] = {0};
			MEM_PARM* in_A_buf = &p_op->input_mem;
			MEM_PARM* out_buf = &p_op->output_mem;
			
			//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
		    
			//set src1 as 1d tensor
			src[0].sign = MAKEFOURCC('A','B','U','F');
			src[0].ddr_id = 0;
			src[0].va = in_A_buf->va; //< input address
			src[0].pa = in_A_buf->pa;
			src[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth);
			src[0].fmt = op_mau_config_data.in_a_fmt;
			src[0].width = op_mau_config_data.in_a_w;
			src[0].height = op_mau_config_data.in_a_h;
			src[0].channel = 1;
			src[0].batch_num = 1;
			src[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.in_a_bit_depth/8;
			
			//set dest1 as 2d tensor
			dest[0].sign = MAKEFOURCC('A','B','U','F');
			dest[0].ddr_id = 0;
			dest[0].va = out_buf->va;
			dest[0].pa = out_buf->pa;
			dest[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, 1, op_mau_config_data.out_c_bit_depth);
			dest[0].fmt = op_mau_config_data.out_c_fmt;
			dest[0].width = op_mau_config_data.in_a_w;
			dest[0].height = 1;
			dest[0].channel = 1;
			dest[0].batch_num = 1;
			dest[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.out_c_bit_depth/8;

			p_parm.mau_cal_mode = VENDOR_AI_MAU_L2_NORM_INV;
			p_parm.mau_l2_norm_iter = op_mau_config_data.mau_l2_norm_iter;

			ret = vendor_ai_op_add(op_path, VENDOR_AI_OP_MAU, &p_parm, 1, src, 1, dest);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) vendor_ai_op_add fail\r\n", op_path);
		    }
			
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->work_mem)->va, (&p_op->work_mem)->size);
			if(HD_OK != ret) {
		    	printf("flush cache failed.\n");
			}
		}
		break;
		case AI_OP_MAU_VECTOR_NORMALIZAION: 
		{
			VENDOR_AI_OP_MAU_PARAM p_parm = {0};
		    // 2. flush input
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
							
			VENDOR_AI_BUF src[1] = {0};
			VENDOR_AI_BUF dest[1] = {0};
			MEM_PARM* in_A_buf = &p_op->input_mem;
			MEM_PARM* out_buf = &p_op->output_mem;
			
			//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
		    
			//set src1 as 1d tensor
			src[0].sign = MAKEFOURCC('A','B','U','F');
			src[0].ddr_id = 0;
			src[0].va = in_A_buf->va; //< input address
			src[0].pa = in_A_buf->pa;
			src[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth);
			src[0].fmt = op_mau_config_data.in_a_fmt;
			src[0].width = op_mau_config_data.in_a_w;
			src[0].height = op_mau_config_data.in_a_h;
			src[0].channel = 1;
			src[0].batch_num = 1;
			src[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.in_a_bit_depth/8;
			
			//set dest1 as 2d tensor
			dest[0].sign = MAKEFOURCC('A','B','U','F');
			dest[0].ddr_id = 0;
			dest[0].va = out_buf->va;
			dest[0].pa = out_buf->pa;
			dest[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.out_c_bit_depth);
			dest[0].fmt = op_mau_config_data.out_c_fmt;
			dest[0].width = op_mau_config_data.in_a_w;
			dest[0].height = op_mau_config_data.in_a_h;
			dest[0].channel = 1;
			dest[0].batch_num = 1;
			dest[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.out_c_bit_depth/8;

			p_parm.mau_cal_mode = VENDOR_AI_MAU_VECTOR_NORMALIZAION;
			p_parm.mau_l2_norm_iter = op_mau_config_data.mau_l2_norm_iter;

			ret = vendor_ai_op_add(op_path, VENDOR_AI_OP_MAU, &p_parm, 1, src, 1, dest);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) vendor_ai_op_add fail\r\n", op_path);
		    }
			
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->work_mem)->va, (&p_op->work_mem)->size);
			if(HD_OK != ret) {
		    	printf("flush cache failed.\n");
			}
		}
		break;
		case AI_OP_MAU_TOPN_SORT: 
		{
			VENDOR_AI_OP_MAU_PARAM p_parm = {0};
		    // 2. flush input
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->output2_mem)->va, (&p_op->output2_mem)->size);
		    if(HD_OK != ret) {
		        printf("flush cache failed.\n");
		    }
							
			VENDOR_AI_BUF src[1] = {0};
			VENDOR_AI_BUF dest[2] = {0};
			MEM_PARM* in_A_buf = &p_op->input_mem;
			MEM_PARM* out_buf = &p_op->output_mem;
			MEM_PARM* idx_buf = &p_op->output2_mem;
			
			//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
		    
			//set src1 as 1d tensor
			src[0].sign = MAKEFOURCC('A','B','U','F');
			src[0].ddr_id = 0;
			src[0].va = in_A_buf->va; //< input address
			src[0].pa = in_A_buf->pa;
			src[0].size = AI_MAU_BUFSIZE(op_mau_config_data.in_a_w, op_mau_config_data.in_a_h, op_mau_config_data.in_a_bit_depth);
			src[0].fmt = op_mau_config_data.in_a_fmt;
			src[0].width = op_mau_config_data.in_a_w;
			src[0].height = op_mau_config_data.in_a_h;
			src[0].channel = 1;
			src[0].batch_num = 1;
			src[0].line_ofs = op_mau_config_data.in_a_w * op_mau_config_data.in_a_bit_depth/8;
			
			//set dest1 as 2d tensor
			dest[0].sign = MAKEFOURCC('A','B','U','F');
			dest[0].ddr_id = 0;
			dest[0].va = out_buf->va;
			dest[0].pa = out_buf->pa;
			dest[0].size = AI_MAU_BUFSIZE(op_mau_config_data.mau_sort_top_n, op_mau_config_data.in_a_h, op_mau_config_data.out_c_bit_depth);
			dest[0].fmt = op_mau_config_data.out_c_fmt;
			dest[0].width = op_mau_config_data.mau_sort_top_n;
			dest[0].height = op_mau_config_data.in_a_h;
			dest[0].channel = 1;
			dest[0].batch_num = 1;
			dest[0].line_ofs = op_mau_config_data.mau_sort_top_n * op_mau_config_data.out_c_bit_depth/8;

			//set dest2 as 2d tensor
			dest[1].sign = MAKEFOURCC('A','B','U','F');
			dest[1].ddr_id = 0;
			dest[1].va = idx_buf->va;
			dest[1].pa = idx_buf->pa;
			dest[1].size = AI_MAU_BUFSIZE(op_mau_config_data.mau_sort_top_n, op_mau_config_data.in_a_h, op_mau_config_data.sort_idx_bit_depth);
			dest[1].fmt = op_mau_config_data.sort_idx_fmt;
			dest[1].width = op_mau_config_data.mau_sort_top_n;
			dest[1].height = op_mau_config_data.in_a_h;
			dest[1].channel = 1;
			dest[1].batch_num = 1;
			dest[1].line_ofs = op_mau_config_data.mau_sort_top_n * op_mau_config_data.sort_idx_bit_depth/8;
			
			p_parm.mau_cal_mode = VENDOR_AI_MAU_TOPN_SORT;
			p_parm.mau_sort_top_n = op_mau_config_data.mau_sort_top_n;

			ret = vendor_ai_op_add(op_path, VENDOR_AI_OP_MAU, &p_parm, 1, src, 2, dest);
			if (ret != HD_OK) {
		   	    printf("proc_id(%lu) vendor_ai_op_add fail\r\n", op_path);
		    }
			
			ret = hd_common_mem_flush_cache((VOID *)(&p_op->work_mem)->va, (&p_op->work_mem)->size);
			if(HD_OK != ret) {
		    	printf("flush cache failed.\n");
			}
		}
		break;
#endif
		default:
        break;
	}
	return ret;
}

static VOID *operator_user_thread(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	
	TEST_NETWORK *p_stream = (TEST_NETWORK*)arg;
	OP_PROC* p_op = g_op + p_stream->op_path;
	NET_IN* p_in = g_in + p_stream->in_path;

	printf("\r\n");
	while (p_stream->op_start == 0) sleep(1);

	printf("\r\n");
	ret = ai_test_operator_alloc_out_buf(p_stream->op_path, p_stream->in_path);
	if (HD_OK != ret) {
		printf("proc_id(%lu) alloc output fail !!\n", p_stream->op_path);
		goto skip;
	}

	printf("\r\n");
		
    switch (p_op->op_opt) {
		case AI_OP_FC: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
            

	                // 2. flush input
	        		ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
	        		ret = hd_common_mem_flush_cache((VOID *)(&p_op->weight_mem)->va, (&p_op->weight_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
	        		ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
		        	// 3. run OP
		        	{
		        		/*
		        		The code below is the flow of using FC 
		        		suppose the input feature size is 256 bytes (defined as SV_FEA_LENGTH)
		        		and the desired output length is 10240 (defined as SV_LENGTH)
		        		the following sample will transpose the input 256 bytes feature (1 byte per element) into 10240*4 bytes feature (4 bytes per element)
		        		
		        		fc_init_param is for setting parameter of FC
			        	user should set input/output/weight address
			        	*/
			        	VENDOR_AI_BUF src[2] = {0};
			        	VENDOR_AI_BUF dest[1] = {0};
			        	MEM_PARM* in_buf = &p_op->input_mem;
			        	MEM_PARM* out_buf = &p_op->output_mem;
			        	MEM_PARM* weight_buf = &p_op->weight_mem;
		        		//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			        	//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			        	//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
				        
		          		//set src1 as 1d tensor
			        	src[0].sign = MAKEFOURCC('A','B','U','F');
			        	src[0].ddr_id = 0;
			        	src[0].va = in_buf->va; //< input address	 (size = SV_FEA_LENGTH bytes)
			        	src[0].pa = in_buf->pa;
			        	src[0].size = SV_FEA_LENGTH;
			        	src[0].fmt = HD_VIDEO_PXLFMT_AI_SFIXED8(0); //fixpoint s7.0
			        	src[0].width = SV_FEA_LENGTH;
			        	src[0].height = 1;
			        	src[0].channel = 1;
			        	src[0].batch_num = 1;
			        	
		        		//set src2 as 2d tensor
		        		src[1].sign = MAKEFOURCC('A','B','U','F');
		        		src[1].ddr_id = 0;
		        		src[1].va = weight_buf->va; //< sv weight address (size = SV_LENGTH*SV_FEA_LENGTH bytes)
		        		src[1].pa = weight_buf->pa;
		        		src[1].size = SV_FEA_LENGTH*SV_LENGTH;
		        		src[1].fmt = HD_VIDEO_PXLFMT_AI_SFIXED8(0); //fixpoint s7.0
			        	src[1].width = SV_FEA_LENGTH;
			        	src[1].height = SV_LENGTH;
			        	src[1].channel = 1;
			        	src[1].batch_num = 1;
		        		
		        		//set dest as 1d tensor
			        	dest[0].sign = MAKEFOURCC('A','B','U','F');
		        		dest[0].ddr_id = 0;
			        	dest[0].va = out_buf->va; //< output address	 (size = SV_LENGTH*4 bytes)
			        	dest[0].pa = out_buf->pa;
			        	dest[0].size = SV_LENGTH*4;
			        	dest[0].fmt = HD_VIDEO_PXLFMT_AI_SFIXED32(0); //fixpoint s31.0
			        	dest[0].width = SV_LENGTH;
			        	dest[0].height = 1;
			        	dest[0].channel = 1;
			        	dest[0].batch_num = 1;

			        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_FC, NULL, 2, src, 1, dest);
		        	}
		        	if (ret != 0) {
		        		printf("op inference fail\n");
		        		return 0;
		        	}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
        
		        	ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./op_user_out.bin");
        
		        }
		        usleep(100);
	        }
		}
        break;
		case AI_OP_FC_LL_MODE: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
						
		        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_LIST, NULL, 0, NULL, 0, NULL);
					if (ret != HD_OK) {
				   	    printf("proc_id(%lu) vendor_ai_op_proc for run fc ll fail\r\n", p_stream->op_path);
					}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
        
		        	ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./AI_OP_FC_LL_MODE_out.bin");
        
		        }
		        usleep(100);
	        }		
		}
        break;
        case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC: 
        {
            while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
                    ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
                    ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
                    // 3. run OP
		        	{
                        VENDOR_AI_BUF src[2] = {0};
			        	VENDOR_AI_BUF dest[3] = {0};
			        	MEM_PARM* in_buf = &p_op->input_mem;
			        	MEM_PARM* out_buf = &p_op->output_mem;
                        NET_IN_CONFIG in_cfg = p_in->in_cfg;
                        VENDOR_AI_OP_PREPROC_PARAM p_parm = {0};

                        //set src1 as 1d tensor
						src[0].sign = MAKEFOURCC('A','B','U','F');
						src[0].ddr_id = 0;
						src[0].va = in_buf->va; //< input address	 
						src[0].pa = in_buf->pa;
						src[0].size = in_cfg.loff * in_cfg.h;
						src[0].fmt = HD_VIDEO_PXLFMT_Y8;
						src[0].width = in_cfg.w;
						src[0].height = in_cfg.h;
						src[0].line_ofs = in_cfg.loff;
						src[0].channel = 1;
						src[0].batch_num = 1;

                        //set src2 as 1d tensor
						src[1].sign = MAKEFOURCC('A','B','U','F');
						src[1].ddr_id = 0;
						src[1].va = in_buf->va + src[0].size; //< input address	 
						src[1].pa = in_buf->pa + src[0].size;
						src[1].size = in_cfg.loff * in_cfg.h;
						src[1].fmt = HD_VIDEO_PXLFMT_UV;
						src[1].width = in_cfg.w;
						src[1].height = in_cfg.h;
						src[1].line_ofs = in_cfg.loff;
						src[1].channel = 1;
						src[1].batch_num = 1;

						if(p_op->op_opt == AI_OP_PREPROC_YUV2RGB_SCALE) {
							//set dest1 as 1d tensor
							dest[0].sign = MAKEFOURCC('A','B','U','F');
							dest[0].ddr_id = 0;
							dest[0].va = out_buf->va; //< output address	 
							dest[0].pa = out_buf->pa;
							dest[0].size = SCALE_DIM_W * SCALE_DIM_H;
							dest[0].fmt = HD_VIDEO_PXLFMT_R8;
							dest[0].width = SCALE_DIM_W;
							dest[0].height = SCALE_DIM_H;
							dest[0].line_ofs = SCALE_DIM_W;
							dest[0].channel = 1;
							dest[0].batch_num = 1;

							//set dest2 as 1d tensor
							dest[1].sign = MAKEFOURCC('A','B','U','F');
							dest[1].ddr_id = 0;
							dest[1].va = out_buf->va + dest[0].size; //< output address	 
							dest[1].pa = out_buf->pa + dest[0].size;
							dest[1].size = SCALE_DIM_W * SCALE_DIM_H;
							dest[1].fmt = HD_VIDEO_PXLFMT_G8;
							dest[1].width = SCALE_DIM_W;
							dest[1].height = SCALE_DIM_H;
							dest[1].line_ofs = SCALE_DIM_W;
							dest[1].channel = 1;
							dest[1].batch_num = 1;

							//set dest3 as 1d tensor
							dest[2].sign = MAKEFOURCC('A','B','U','F');
							dest[2].ddr_id = 0;
							dest[2].va = out_buf->va + 2*dest[0].size; //< output address		 
							dest[2].pa = out_buf->pa + 2*dest[0].size;
							dest[2].size = SCALE_DIM_W * SCALE_DIM_H;
							dest[2].fmt = HD_VIDEO_PXLFMT_B8;
							dest[2].width = SCALE_DIM_W;
							dest[2].height = SCALE_DIM_H;
							dest[2].line_ofs = SCALE_DIM_W;
							dest[2].channel = 1;
							dest[2].batch_num = 1;
						}
						else {
							//set dest1 as 1d tensor
							dest[0].sign = MAKEFOURCC('A','B','U','F');
							dest[0].ddr_id = 0;
							dest[0].va = out_buf->va; //< output address	 
							dest[0].pa = out_buf->pa;
							dest[0].size = in_cfg.loff * in_cfg.h;
							dest[0].fmt = HD_VIDEO_PXLFMT_R8;
							dest[0].width = in_cfg.w;
							dest[0].height = in_cfg.h;
							dest[0].line_ofs = in_cfg.w;
							dest[0].channel = 1;
							dest[0].batch_num = 1;

							//set dest2 as 1d tensor
							dest[1].sign = MAKEFOURCC('A','B','U','F');
							dest[1].ddr_id = 0;
							dest[1].va = out_buf->va + dest[0].size; //< output address	 
							dest[1].pa = out_buf->pa + dest[0].size;
							dest[1].size = in_cfg.loff * in_cfg.h;
							dest[1].fmt = HD_VIDEO_PXLFMT_G8;
							dest[1].width = in_cfg.w;
							dest[1].height = in_cfg.h;
							dest[1].line_ofs = in_cfg.w;
							dest[1].channel = 1;
							dest[1].batch_num = 1;

							//set dest3 as 1d tensor
							dest[2].sign = MAKEFOURCC('A','B','U','F');
							dest[2].ddr_id = 0;
							dest[2].va = out_buf->va + 2*dest[0].size; //< output address		 
							dest[2].pa = out_buf->pa + 2*dest[0].size;
							dest[2].size = in_cfg.loff * in_cfg.h;
							dest[2].fmt = HD_VIDEO_PXLFMT_B8;
							dest[2].width = in_cfg.w;
							dest[2].height = in_cfg.h;
							dest[2].line_ofs = in_cfg.w;
							dest[2].channel = 1;
							dest[2].batch_num = 1;
						}

                        // set func parameter

						//scale
						if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_SCALE) {
							p_parm.scale_dim.w = SCALE_DIM_W;
							p_parm.scale_dim.h = SCALE_DIM_H;
						}				
						
                        // plane mode
                        if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE) {
							memset((VOID *)(in_buf->va + src[0].size*3), 0x80808080, src[0].size*3);   //clear buffer for sub
							ret = hd_common_mem_flush_cache((VOID *)(in_buf->va + src[0].size*3), src[0].size*3);
							if(HD_OK != ret) {
								printf("flush cache failed.\n");
							}
							p_parm.p_out_sub.pa = in_buf->pa + 3*src[0].size;
							p_parm.p_out_sub.va = in_buf->va + 3*src[0].size;                       
							p_parm.p_out_sub.width = in_cfg.w;
							p_parm.p_out_sub.height = in_cfg.h;
							p_parm.p_out_sub.line_ofs = in_cfg.w*3;
                        }

						// dc mode
						if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_DC) {                 
							p_parm.out_sub_color[0] = 128;
							p_parm.out_sub_color[1] = 127;
							p_parm.out_sub_color[2] = 126;
						}
         
                        ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_PREPROC, &p_parm, 2, src, 3, dest);

                    }
                    if (ret != 0) {
		        		printf("op inference fail\n");
		        		return 0;
		        	}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }

					if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB)
                    	ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./AI_OP_PREPROC_YUV2RGB_out.bin");
					else if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_SCALE)
                    	ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./AI_OP_PREPROC_YUV2RGB_SCALE_out.bin");
					else if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE)
                    	ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE_out.bin");
					else if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_DC)
                    	ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./AI_OP_PREPROC_YUV2RGB_MEANSUB_DC_out.bin");

	        	}
                usleep(100);
            }
        }
        break;
		case AI_OP_PREPROC_Y2Y_UV2UV: 
        {
            while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
                    ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
                    ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
                    // 3. run OP
		        	
                        VENDOR_AI_BUF src[2] = {0};
			        	VENDOR_AI_BUF dest[2] = {0};
			        	MEM_PARM* in_buf = &p_op->input_mem;
			        	MEM_PARM* out_buf = &p_op->output_mem;
                        NET_IN_CONFIG in_cfg = p_in->in_cfg;
                        VENDOR_AI_OP_PREPROC_PARAM p_parm = {0};

                        //set src1 as 1d tensor
						src[0].sign = MAKEFOURCC('A','B','U','F');
						src[0].ddr_id = 0;
						src[0].va = in_buf->va; //< input address	 
						src[0].pa = in_buf->pa;
						src[0].size = in_cfg.loff * in_cfg.h;
						src[0].fmt = HD_VIDEO_PXLFMT_Y8;
						src[0].width = in_cfg.w;
						src[0].height = in_cfg.h;
						src[0].line_ofs = in_cfg.loff;
						src[0].channel = 1;
						src[0].batch_num = 1;

                        //set src2 as 1d tensor
						src[1].sign = MAKEFOURCC('A','B','U','F');
						src[1].ddr_id = 0;
						src[1].va = in_buf->va + src[0].size; //< input address	 
						src[1].pa = in_buf->pa + src[0].size;
						src[1].size = in_cfg.loff * in_cfg.h;
						src[1].fmt = HD_VIDEO_PXLFMT_UV;
						src[1].width = in_cfg.w/2;
						src[1].height = in_cfg.h/2;
						src[1].line_ofs = in_cfg.w;
						src[1].channel = 1;
						src[1].batch_num = 1;

						//set dest1 as 1d tensor
						dest[0].sign = MAKEFOURCC('A','B','U','F');
						dest[0].ddr_id = 0;
						dest[0].va = out_buf->va; //< output address	 
						dest[0].pa = out_buf->pa;
						dest[0].size = SCALE_DIM_W * SCALE_DIM_H;
						dest[0].fmt = HD_VIDEO_PXLFMT_Y8;
						dest[0].width = SCALE_DIM_W;
						dest[0].height = SCALE_DIM_H;
						dest[0].line_ofs = SCALE_DIM_W;
						dest[0].channel = 1;
						dest[0].batch_num = 1;

						//set dest2 as 1d tensor
						dest[1].sign = MAKEFOURCC('A','B','U','F');
						dest[1].ddr_id = 0;
						dest[1].va = out_buf->va + dest[0].size; //< output address	 
						dest[1].pa = out_buf->pa + dest[0].size;
						dest[1].size = SCALE_DIM_W * SCALE_DIM_H / 2;
						dest[1].fmt = HD_VIDEO_PXLFMT_UV;
						dest[1].width = SCALE_DIM_W/2;
						dest[1].height = SCALE_DIM_H/2;
						dest[1].line_ofs = SCALE_DIM_W;
						dest[1].channel = 1;
						dest[1].batch_num = 1;

                        // set func parameter
                        
						//scale
						p_parm.scale_dim.w = SCALE_DIM_W;
						p_parm.scale_dim.h = SCALE_DIM_H;	 
         
                        ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_PREPROC, &p_parm, 1, src, 1, dest);

						p_parm.scale_dim.w = SCALE_DIM_W/2;
						p_parm.scale_dim.h = SCALE_DIM_H/2;
						
						ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_PREPROC, &p_parm, 1, src+1, 1, dest+1);

                    
                    if (ret != 0) {
		        		printf("op inference fail\n");
		        		return 0;
		        	}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }

                    ai_test_mem_save(&p_op->output_mem, SCALE_DIM_W*SCALE_DIM_H*3/2, "./AI_OP_PREPROC_Y2Y_UV2UV_out.bin");

	        	}
                usleep(100);
            }	
		}
		break;
		case AI_OP_PREPROC_USER: 
        {
            while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
                    ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
                    ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
                    // 3. run OP
		        	{
                        VENDOR_AI_BUF src[3] = {0};
			        	VENDOR_AI_BUF dest[3] = {0};
			        	MEM_PARM* in_buf = &p_op->input_mem;
			        	MEM_PARM* out_buf = &p_op->output_mem;
                        NET_IN_CONFIG in_cfg = p_in->in_cfg;
                        VENDOR_AI_OP_PREPROC_PARAM p_parm = {0};

                        //set src1 as 1d tensor
						src[0].sign = MAKEFOURCC('A','B','U','F');
						src[0].ddr_id = 0;
						src[0].va = in_buf->va; //< input address	 
						src[0].pa = in_buf->pa;
						src[0].size = in_cfg.loff * in_cfg.h;
						src[0].fmt = op_preproc_config_data.src_0_fmt;
						src[0].width = in_cfg.w;
						src[0].height = in_cfg.h;
						src[0].line_ofs = in_cfg.loff;
						src[0].channel = 1;
						src[0].batch_num = 1;

						if(op_preproc_config_data.in_cnt > 1) {
	                        //set src2 as 1d tensor
							src[1].sign = MAKEFOURCC('A','B','U','F');
							src[1].ddr_id = 0;
							src[1].va = in_buf->va + src[0].size; //< input address	 
							src[1].pa = in_buf->pa + src[0].size;
							src[1].size = in_cfg.loff * in_cfg.h;
							src[1].fmt = op_preproc_config_data.src_1_fmt;
							src[1].width = in_cfg.w;
							src[1].height = in_cfg.h;
							src[1].line_ofs = in_cfg.loff;
							src[1].channel = 1;
							src[1].batch_num = 1;
						}

						if(op_preproc_config_data.in_cnt > 2) {
	                        //set src2 as 1d tensor
							src[2].sign = MAKEFOURCC('A','B','U','F');
							src[2].ddr_id = 0;
							src[2].va = in_buf->va + 2*src[0].size; //< input address	 
							src[2].pa = in_buf->pa + 2*src[0].size;
							src[2].size = in_cfg.loff * in_cfg.h;
							src[2].fmt = op_preproc_config_data.src_2_fmt;
							src[2].width = in_cfg.w;
							src[2].height = in_cfg.h;
							src[2].line_ofs = in_cfg.loff;
							src[2].channel = 1;
							src[2].batch_num = 1;
						}

						//set dest1 as 1d tensor
						dest[0].sign = MAKEFOURCC('A','B','U','F');
						dest[0].ddr_id = 0;
						dest[0].va = out_buf->va; //< output address	 
						dest[0].pa = out_buf->pa;
						dest[0].size = op_preproc_config_data.dst_0_width * op_preproc_config_data.dst_0_height;
						dest[0].fmt = op_preproc_config_data.dst_0_fmt;
						dest[0].width = op_preproc_config_data.dst_0_width;
						dest[0].height = op_preproc_config_data.dst_0_height;
						dest[0].line_ofs = op_preproc_config_data.dst_0_line_ofs;
						dest[0].channel = 1;
						dest[0].batch_num = 1;

						if(op_preproc_config_data.out_cnt > 1) {
							//set dest2 as 1d tensor
							dest[1].sign = MAKEFOURCC('A','B','U','F');
							dest[1].ddr_id = 0;
							dest[1].va = out_buf->va + dest[0].size; //< output address	 
							dest[1].pa = out_buf->pa + dest[0].size;
							dest[1].size = op_preproc_config_data.dst_1_width * op_preproc_config_data.dst_1_height;
							dest[1].fmt = op_preproc_config_data.dst_1_fmt;
							dest[1].width = op_preproc_config_data.dst_1_width;
							dest[1].height = op_preproc_config_data.dst_1_height;
							dest[1].line_ofs = op_preproc_config_data.dst_1_line_ofs;
							dest[1].channel = 1;
							dest[1].batch_num = 1;
						}

						if(op_preproc_config_data.out_cnt > 2) {
							//set dest2 as 1d tensor
							dest[2].sign = MAKEFOURCC('A','B','U','F');
							dest[2].ddr_id = 0;
							dest[2].va = out_buf->va + 2*dest[0].size; //< output address	 
							dest[2].pa = out_buf->pa + 2*dest[0].size;
							dest[2].size = op_preproc_config_data.dst_2_width * op_preproc_config_data.dst_2_height;
							dest[2].fmt = op_preproc_config_data.dst_2_fmt;
							dest[2].width = op_preproc_config_data.dst_2_width;
							dest[2].height = op_preproc_config_data.dst_2_height;
							dest[2].line_ofs = op_preproc_config_data.dst_2_line_ofs;
							dest[2].channel = 1;
							dest[2].batch_num = 1;
						}
						

                        // set func parameter

						//scale
						if (op_preproc_config_data.scale_w != 0) {
							p_parm.scale_dim.w = op_preproc_config_data.scale_w;
							p_parm.scale_dim.h = op_preproc_config_data.scale_h;
						}				

						// dc mode
						if (op_preproc_config_data.out_sub_color_0 != 0) {                 
							p_parm.out_sub_color[0] = op_preproc_config_data.out_sub_color_0;
							p_parm.out_sub_color[1] = op_preproc_config_data.out_sub_color_1;
							p_parm.out_sub_color[2] = op_preproc_config_data.out_sub_color_2;
						}
         
                        ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_PREPROC, &p_parm, op_preproc_config_data.in_cnt, src, op_preproc_config_data.out_cnt, dest);

                    }
                    if (ret != 0) {
		        		printf("op inference fail\n");
		        		return 0;
		        	}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
					if (strlen(op_preproc_config_data.dst_file_path) == 0) {
                    	ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./AI_OP_PREPROC_USER.bin");
					} else {
						ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, op_preproc_config_data.dst_file_path);
					}
	        	}
                usleep(100);
            }
        }
        break;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		case AI_OP_MAU_MAT_MUL: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
						
		        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_MAU, NULL, 0, NULL, 0, NULL);
					if (ret != HD_OK) {
				   	    printf("proc_id(%lu) vendor_ai_op_proc for run MAU fail\r\n", p_stream->op_path);
					}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
					ret = hd_common_mem_flush_cache((VOID *)(&p_op->output2_mem)->va, (&p_op->output2_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }

					if (strlen(op_mau_config_data.out_c_file_path) == 0) {
						if(mau_mat_outid == 0) { //result is in output buff
			        		ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./mau_mat_multi_out.bin"); 
						} else { //result is in tmp buff
							ai_test_mem_save(&p_op->output2_mem, p_op->output2_mem.size, "./mau_mat_multi_tmp.bin");
						}
					} else {
						ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, op_mau_config_data.out_c_file_path); 
					}
					ai_test_mem_save_addr(mau_mat_out.va, mau_mat_out.size, "./mau_mat_multi_out_addr.bin");
        
		        }
		        usleep(100);
	        }		
		}
        break;
		case AI_OP_MAU_MAT_ADD: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
						
		        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_MAU, NULL, 0, NULL, 0, NULL);
					if (ret != HD_OK) {
				   	    printf("proc_id(%lu) vendor_ai_op_proc for run MAU fail\r\n", p_stream->op_path);
					}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
					if (strlen(op_mau_config_data.out_c_file_path) == 0) {
		        		ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./mau_mat_add_out.bin");
					} else {
						ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, op_mau_config_data.out_c_file_path); 
					}
        
		        }
		        usleep(100);
	        }		
		}
        break;
		case AI_OP_MAU_MAT_SUB: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
						
		        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_MAU, NULL, 0, NULL, 0, NULL);
					if (ret != HD_OK) {
				   	    printf("proc_id(%lu) vendor_ai_op_proc for run MAU fail\r\n", p_stream->op_path);
					}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
					
					if (strlen(op_mau_config_data.out_c_file_path) == 0) {
		        		ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./mau_mat_sub_out.bin");
					} else {
						ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, op_mau_config_data.out_c_file_path); 
					}
        
		        }
		        usleep(100);
	        }		
		}
        break;
		case AI_OP_MAU_L2_NORM: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
						
		        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_MAU, NULL, 0, NULL, 0, NULL);
					if (ret != HD_OK) {
				   	    printf("proc_id(%lu) vendor_ai_op_proc for run MAU fail\r\n", p_stream->op_path);
					}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
		        	
					if (strlen(op_mau_config_data.out_c_file_path) == 0) {
		        		ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./mau_l2_norm_out.bin");
					} else {
						ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, op_mau_config_data.out_c_file_path); 
					}
        
        
		        }
		        usleep(100);
	        }		
		}
        break;
		case AI_OP_MAU_L2_NORM_INV: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
						
		        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_MAU, NULL, 0, NULL, 0, NULL);
					if (ret != HD_OK) {
				   	    printf("proc_id(%lu) vendor_ai_op_proc for run MAU fail\r\n", p_stream->op_path);
					}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
		        	
					if (strlen(op_mau_config_data.out_c_file_path) == 0) {
		        		ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./mau_l2_norm_inv_out.bin");
					} else {
						ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, op_mau_config_data.out_c_file_path); 
					}
        
        
		        }
		        usleep(100);
	        }		
		}
        break;
		case AI_OP_MAU_VECTOR_NORMALIZAION: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
						
		        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_MAU, NULL, 0, NULL, 0, NULL);
					if (ret != HD_OK) {
				   	    printf("proc_id(%lu) vendor_ai_op_proc for run MAU fail\r\n", p_stream->op_path);
					}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
		        	
					if (strlen(op_mau_config_data.out_c_file_path) == 0) {
		        		ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./mau_vector_normalization_out.bin");
					} else {
						ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, op_mau_config_data.out_c_file_path); 
					}
        
		        }
		        usleep(100);
	        }		
		}
        break;
		case AI_OP_MAU_TOPN_SORT: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
						
		        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_MAU, NULL, 0, NULL, 0, NULL);
					if (ret != HD_OK) {
				   	    printf("proc_id(%lu) vendor_ai_op_proc for run MAU fail\r\n", p_stream->op_path);
					}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
					ret = hd_common_mem_flush_cache((VOID *)(&p_op->output2_mem)->va, (&p_op->output2_mem)->size);
                  	if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }

					if (strlen(op_mau_config_data.out_c_file_path) == 0) {
		        		ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, "./mau_topn_sort_out.bin");
					} else {
						ai_test_mem_save(&p_op->output_mem, p_op->output_mem.size, op_mau_config_data.out_c_file_path); 
					}

        			ai_test_mem_save(&p_op->output2_mem, p_op->output2_mem.size, "./mau_topn_sort_idx.bin"); 
		        }
		        usleep(100);
	        }		
		}
		break;
#endif
        default:
        break;
    }
	
skip:
	ret = ai_test_operator_free_out_buf(p_stream->op_path);
	if (HD_OK != ret) {
		printf("proc_id(%lu) free output fail !!\n", p_stream->op_path);
		goto skip;
	}
	
	return 0;
}


