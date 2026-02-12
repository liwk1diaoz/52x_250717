/**
	@brief Source file of vendor ai net test code.

	@file ai_test_op.h

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _AI_TEST_OP_H_
#define _AI_TEST_OP_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_int.h"
#include "ai_test_util.h"
/*-----------------------------------------------------------------------------*/
/* Operator Functions                                                           */
/*-----------------------------------------------------------------------------*/

#define SV_LENGTH 		10240
#define SV_FEA_LENGTH 	256

#define SCALE_DIM_W 384
#define SCALE_DIM_H 282

#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
#define AI_MAU_BUFSIZE(w, h, bit_depth)    ((w) * (h) * (bit_depth/8))

#define MAU_MAT_MULTI_A_W 291
#define MAU_MAT_MULTI_A_H 23
#define MAU_MAT_MULTI_A_FRAC_BIT 11
#define MAU_MAT_MULTI_A_BIT_DEPTH 16
#define MAU_MAT_MULTI_A_FMT HD_VIDEO_PXLFMT_AI_SFIXED16(MAU_MAT_MULTI_A_FRAC_BIT)
#define MAU_MAT_MULTI_A_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg15701/DI0_15701.bin"

#define MAU_MAT_MULTI_B_W 291
#define MAU_MAT_MULTI_B_H 284
#define MAU_MAT_MULTI_B_FRAC_BIT 11
#define MAU_MAT_MULTI_B_BIT_DEPTH 16
#define MAU_MAT_MULTI_B_FMT HD_VIDEO_PXLFMT_AI_SFIXED16(MAU_MAT_MULTI_B_FRAC_BIT)
#define MAU_MAT_MULTI_B_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg15701/DI1_15701.bin"

#define MAU_MAT_MULTI_C_BIT_DEPTH 32
#define MAU_MAT_MULTI_C_FMT HD_VIDEO_PXLFMT_AI_FLOAT32


#define MAU_MAT_ADD_A_W 2
#define MAU_MAT_ADD_A_H 1
#define MAU_MAT_ADD_A_FRAC_BIT 0
#define MAU_MAT_ADD_A_BIT_DEPTH 16
#define MAU_MAT_ADD_A_FMT HD_VIDEO_PXLFMT_AI_FLOAT16
#define MAU_MAT_ADD_A_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg15001/DI0_15001.bin"

#define MAU_MAT_ADD_B_W 2
#define MAU_MAT_ADD_B_H 1
#define MAU_MAT_ADD_B_FRAC_BIT 0
#define MAU_MAT_ADD_B_BIT_DEPTH 16
#define MAU_MAT_ADD_B_FMT HD_VIDEO_PXLFMT_AI_SFIXED16(MAU_MAT_ADD_B_FRAC_BIT)
#define MAU_MAT_ADD_B_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg15001/DI1_15001.bin"

#define MAU_MAT_ADD_C_BIT_DEPTH 32
#define MAU_MAT_ADD_C_FMT HD_VIDEO_PXLFMT_AI_FLOAT32


#define MAU_MAT_SUB_A_W 36
#define MAU_MAT_SUB_A_H 146
#define MAU_MAT_SUB_A_FRAC_BIT 0
#define MAU_MAT_SUB_A_BIT_DEPTH 32
#define MAU_MAT_SUB_A_FMT HD_VIDEO_PXLFMT_AI_FLOAT32
#define MAU_MAT_SUB_A_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg16501/DI0_16501.bin"

#define MAU_MAT_SUB_B_W 36
#define MAU_MAT_SUB_B_H 146
#define MAU_MAT_SUB_B_FRAC_BIT 0
#define MAU_MAT_SUB_B_BIT_DEPTH 32
#define MAU_MAT_SUB_B_FMT HD_VIDEO_PXLFMT_AI_FLOAT32
#define MAU_MAT_SUB_B_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg16501/DI1_16501.bin"

#define MAU_MAT_SUB_C_BIT_DEPTH 8
#define MAU_MAT_SUB_C_FRAC_BIT 134
#define MAU_MAT_SUB_C_FMT HD_VIDEO_PXLFMT_AI_SFIXED8(MAU_MAT_SUB_C_FRAC_BIT)


#define MAU_L2_NORM_A_W 54
#define MAU_L2_NORM_A_H 30
#define MAU_L2_NORM_A_FRAC_BIT 61
#define MAU_L2_NORM_A_BIT_DEPTH 16
#define MAU_L2_NORM_A_FMT HD_VIDEO_PXLFMT_AI_UFIXED16(MAU_L2_NORM_A_FRAC_BIT)
#define MAU_L2_NORM_A_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg16701/DI0_16701.bin"

#define MAU_L2_NORM_C_BIT_DEPTH 32
#define MAU_L2_NORM_C_FRAC_BIT 0
#define MAU_L2_NORM_C_FMT HD_VIDEO_PXLFMT_AI_FLOAT32

#define MAU_L2_NORM_ITER 0


#define MAU_L2_NORM_INV_A_W 183
#define MAU_L2_NORM_INV_A_H 31
#define MAU_L2_NORM_INV_A_FRAC_BIT 53
#define MAU_L2_NORM_INV_A_BIT_DEPTH 8
#define MAU_L2_NORM_INV_A_FMT HD_VIDEO_PXLFMT_AI_UFIXED8(MAU_L2_NORM_INV_A_FRAC_BIT)
#define MAU_L2_NORM_INV_A_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg16703/DI0_16703.bin"

#define MAU_L2_NORM_INV_C_BIT_DEPTH 32
#define MAU_L2_NORM_INV_C_FRAC_BIT 0
#define MAU_L2_NORM_INV_C_FMT HD_VIDEO_PXLFMT_AI_FLOAT32

#define MAU_L2_NORM_INV_ITER 1


#define MAU_VECTOR_NORMALIZAION_A_W 120
#define MAU_VECTOR_NORMALIZAION_A_H 27
#define MAU_VECTOR_NORMALIZAION_A_FRAC_BIT 0
#define MAU_VECTOR_NORMALIZAION_A_BIT_DEPTH 16
#define MAU_VECTOR_NORMALIZAION_A_FMT HD_VIDEO_PXLFMT_AI_FLOAT16
#define MAU_VECTOR_NORMALIZAION_A_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg17001/DI0_17001.bin"

#define MAU_VECTOR_NORMALIZAION_C_BIT_DEPTH 8
#define MAU_VECTOR_NORMALIZAION_C_FRAC_BIT 121
#define MAU_VECTOR_NORMALIZAION_C_FMT HD_VIDEO_PXLFMT_AI_UFIXED8(MAU_VECTOR_NORMALIZAION_C_FRAC_BIT)

#define MAU_VECTOR_NORMALIZAION_ITER 1


#define MAU_TOPN_SORT_A_W 512
#define MAU_TOPN_SORT_A_H 384
#define MAU_TOPN_SORT_A_FRAC_BIT 0
#define MAU_TOPN_SORT_A_BIT_DEPTH 16
#define MAU_TOPN_SORT_A_FMT HD_VIDEO_PXLFMT_AI_SFIXED16(MAU_TOPN_SORT_A_FRAC_BIT)
#define MAU_TOPN_SORT_A_FILE_PATH "/mnt/sd/jpg/MAU/NUEP/nueg17501/DI0_17501.bin"

#define MAU_TOPN_SORT_C_BIT_DEPTH 16
#define MAU_TOPN_SORT_C_FRAC_BIT 0
#define MAU_TOPN_SORT_C_FMT HD_VIDEO_PXLFMT_AI_SFIXED16(MAU_TOPN_SORT_C_FRAC_BIT)

#define MAU_TOPN_SORT_IDX_FMT HD_VIDEO_PXLFMT_AI_UINT16
#define MAU_TOPN_SORT_IDX_BIT_DEPTH 16
#define MAU_TOPN_SORT_N 128
#endif

typedef enum _AI_OP {
    AI_OP_FC                             =  0, 
	AI_OP_PREPROC_YUV2RGB                =  1,  
	AI_OP_PREPROC_YUV2RGB_SCALE          =  2,
	AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE  =  3,
	AI_OP_PREPROC_YUV2RGB_MEANSUB_DC     =  4,
	AI_OP_PREPROC_Y2Y_UV2UV              =  5,
	AI_OP_FC_LL_MODE					 =  6,
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
	AI_OP_MAU_MAT_MUL					 =  7,
	AI_OP_MAU_MAT_ADD					 =  8,
	AI_OP_MAU_MAT_SUB					 =  9,
	AI_OP_MAU_L2_NORM					 = 10,
	AI_OP_MAU_L2_NORM_INV				 = 11,
	AI_OP_MAU_VECTOR_NORMALIZAION		 = 12,
	AI_OP_MAU_TOPN_SORT					 = 13,
#endif

	AI_OP_PREPROC_USER					 = 99,
	
	ENUM_DUMMY4WORD(AI_OP)
} AI_OP;

typedef struct _OP_PROC {

	UINT32 proc_id;
	int op_opt;
	UINT32 need_size;
	MEM_PARM input_mem;
	MEM_PARM input2_mem;
	MEM_PARM weight_mem;
	MEM_PARM output_mem;
	MEM_PARM output2_mem;
	MEM_PARM work_mem;
		
} OP_PROC;

typedef enum {
	AI_OP_INT8          = 0,
	AI_OP_UINT8         = 1,
	AI_OP_INT16         = 2,
	AI_OP_UINT16        = 3,
	AI_OP_FP16        	= 4,                   
	AI_OP_FP32			= 5,
	ENUM_DUMMY4WORD(AI_TEST_OP_IO_TYPE)
} AI_TEST_OP_IO_TYPE;

#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
typedef struct _AI_OP_MAU_CONFIG_DATA {
    VENDOR_AI_MAU_CAL_MODE      mau_cal_mode;		///< mau cal mode
	VENDOR_AI_MAU_SORT_OP       mau_sort_op;		///< mau sort operation, setup when mau_cal_mode = 6
	VENDOR_AI_MAU_IN_B_MODE     mau_in_b_mode;		///< mau input b mode, only valid when mau_cal_mode = 0~2
	FLOAT                       mau_mat_dc;			///< mau DC value (FP32 format) for input B, setup when mau_cal_mode = 0~2 and mau_in_b_mode = 1
	UINT32                      mau_sort_top_n;		///< top n number to sort, setup when mau_cal_mode = 6
	UINT32                      mau_l2_norm_iter;	///< iteration to refine the precision, setup when mau_cal_mode = 3~5
	UINT32                      in_a_w;
	UINT32                      in_a_h;
	UINT32                      in_a_frac_bit;
	UINT32                      in_a_bit_depth;
	UINT32                      in_a_fmt;
	CHAR                        in_a_file_path[256];
	UINT32                      in_b_w;
	UINT32                      in_b_h;
	UINT32                      in_b_frac_bit;
	UINT32                      in_b_bit_depth;
	UINT32                      in_b_fmt;
	CHAR                        in_b_file_path[256];
	UINT32                      out_c_frac_bit;
	UINT32                      out_c_bit_depth;
	UINT32                      out_c_fmt;
	CHAR                        out_c_file_path[256];
	UINT32                      sort_idx_bit_depth;
	UINT32                      sort_idx_fmt;
	
} AI_OP_MAU_CONFIG_DATA;
#endif

typedef struct _AI_OP_PREPROC_CONFIG_DATA {
    UINT32 in_cnt;
	UINT32 out_cnt;
	UINT32 src_0_width;
	UINT32 src_0_height;
	UINT32 src_0_fmt;
	UINT32 src_0_line_ofs;
	UINT32 src_1_width;
	UINT32 src_1_height;
	UINT32 src_1_fmt;
	UINT32 src_1_line_ofs;
	UINT32 src_2_width;
	UINT32 src_2_height;
	UINT32 src_2_fmt;
	UINT32 src_2_line_ofs;
	CHAR   src_file_path[256];
	UINT32 dst_0_width;
	UINT32 dst_0_height;
	UINT32 dst_0_fmt;
	UINT32 dst_0_line_ofs;
	UINT32 dst_1_width;
	UINT32 dst_1_height;
	UINT32 dst_1_fmt;
	UINT32 dst_1_line_ofs;
	UINT32 dst_2_width;
	UINT32 dst_2_height;
	UINT32 dst_2_fmt;
	UINT32 dst_2_line_ofs;
	UINT32 scale_w;
	UINT32 scale_h;
	UINT32 out_sub_color_0;
	UINT32 out_sub_color_1;
	UINT32 out_sub_color_2;
	CHAR   dst_file_path[256];
	
	
} AI_OP_PREPROC_CONFIG_DATA;

extern HD_RESULT ai_test_operator_init(void);
extern HD_RESULT ai_test_operator_set_config(NET_PATH_ID net_path, int in_op_opt);
extern HD_RESULT ai_test_operator_set_config_by_file(NET_PATH_ID net_path);
extern HD_RESULT ai_test_operator_open(NET_PATH_ID op_path, NET_PATH_ID in_path);
extern HD_RESULT ai_test_operator_close(NET_PATH_ID op_path);
extern HD_RESULT ai_test_operator_open(NET_PATH_ID op_path, NET_PATH_ID in_path);
extern HD_RESULT ai_test_operator_get_workbuf_size(NET_PATH_ID op_path);
extern HD_RESULT ai_test_operator_set_workbuf(NET_PATH_ID op_path);
extern HD_RESULT ai_test_operator_add_llcmd(NET_PATH_ID op_path);
extern UINT32 ai_test_operator_get_fmt(UINT32 io_type, UINT32 frac_bit);

#endif //_AI_TEST_OP_H_


