/**
	@brief Header file of Custom NN Neural Network functions operating using CPU.

	@file custnn_cpu.h

	@ingroup custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _CUSTNN_CPU_H_
#define _CUSTNN_CPU_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

#ifndef UINTPTR
typedef uintptr_t                       UINTPTR;
#endif

/********************************************************************
	TYPE DEFINITION
********************************************************************/
/**
    Pooling layer parameters.
*/
typedef struct {
	INT32 in_type;
	INT32 out_type;
	INT32 size_width;
	INT32 size_height;
	INT32 size_channel;
	INT32 batch_num;
	INT32 in_ofs_line_ofs;
	INT32 in_ofs_channel_ofs;
	INT32 in_ofs_batch_ofs;
	INT32 out_ofs_line_ofs;
	INT32 out_ofs_channel_ofs;
	INT32 out_ofs_batch_ofs;
	INT32 pool_mode;
	INT32 pool_local_ker_w;
	INT32 pool_local_ker_h;
	INT32 pool_local_ker_stridex;
	INT32 pool_local_ker_stridey;
	INT32 pool_local_pad_top_pad_num;
	INT32 pool_local_pad_bot_pad_num;
	INT32 pool_local_pad_left_pad_num;
	INT32 pool_local_pad_right_pad_num;
	INT32 pool_local_pad_pad_val;
	INT32 pool_global_avg_mul;
	INT32 pool_global_avg_shf;
	INT32 pool_pool_shf;
	INT32 norm_scale;
	INT32 norm_shift;
} CUSTNN_POOL_PARM;

typedef struct _CUSTNN_POOL_PROTO_PARM {
	INT32 pool_type;	// 0:MAX; 1:average
	INT32 kerl_sz;
	INT32 stride;
	INT32 pad;
} CUSTNN_POOL_PROTO_PARM;

/**
    Eltwise layer parameters.
*/
typedef struct {
	INT32 in0_type;
	INT32 in1_type;
	INT32 out_type;
	INT32 size_width;
	INT32 size_height;
	INT32 size_channel;
	INT32 batch_num;
	INT32 in_ofs_line_ofs;
	INT32 in_ofs_channel_ofs;
	INT32 in_ofs_batch_ofs;
	INT32 out_ofs_line_ofs;
	INT32 out_ofs_channel_ofs;
	INT32 out_ofs_batch_ofs;
	INT32 elt_mode;
	INT32 elt_coeff0;
	INT32 elt_coeff1;
	INT32 elt_shift0;
	INT32 elt_shift1;
	INT32 elt_out_shift;
} CUSTNN_ELT_PARM;

//yuchieh
typedef struct _CUSTNN_ELTWISE_PROTO_PARM {
	CHAR operation_name[1000];
	INT32 operation;//0:PROD; 1:SUM (default); 2:MAX
	FLOAT elt_coeff0;
	FLOAT elt_coeff1;
} CUSTNN_ELTWISE_PROTO_PARM;


/**
    Concat layer parameters.
*/
typedef struct {
	UINT32 axis;
} CUSTNN_CONCAT_PARM;

/**
    Slice layer parameters.
*/
typedef struct {
	UINT32 reserved;
} CUSTNN_SLICE_PARM;

typedef struct _CUSTNN_SLICE_PROTO_PARM {
	INT32 axis;
	INT32 slice_point_cnt;
	INT32 slice_point[256];
} CUSTNN_SLICE_PROTO_PARM;
/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/* scale shift function */
extern INT64 custnn_int64_rightShift(INT64 val, unsigned char shiftBits);
extern INT64 custnn_int64_leftShift(INT64 val, unsigned char shiftBits);
extern INT64 custnn_int64_clamp(INT64 prx, INT64 lb, INT64 ub);
extern INT32 custnn_r_scale_shift(INT64 ival, int ShiftDir, int Shift, int Scale, int bitdepth, int sign);
extern HD_RESULT custnn_cpu_scaleshift_process(UINTPTR in_addr, UINTPTR out_addr, UINT32 buf_size, UINT32 io_type, INT32 scale, INT32 shift);
/* get type function */
extern NN_AI_IO_TYPE custnn_get_type(NN_FMT format);
/* Pooling layer */
extern HD_RESULT custnn_cpu_pool_process(UINTPTR bin_length, CHAR *p_prototxt_parm, CUSTNN_POOL_PARM *p_parm, UINTPTR in_addr, UINTPTR out_addr,
 NN_DATA* input_info, NN_CUSTOM_DIM* input_dim, INT32 *p_isf_parm, UINT32 input_num,
 NN_DATA* output_info, NN_CUSTOM_DIM* output_dim, INT32 *p_osf_parm, UINT32 output_num);
/* Eltwise layer */
extern HD_RESULT custnn_cpu_elt_process(CUSTNN_ELT_PARM* p_parm,
 NN_DATA* input_info, NN_CUSTOM_DIM* input_dim,
 NN_DATA* output_info, NN_CUSTOM_DIM* output_dim, UINTPTR in0_temp_addr, UINTPTR in1_temp_addr);
/* concat layer */
#if CUST_SUPPORT_MULTI_IO
extern HD_RESULT custnn_cpu_concat_process(UINTPTR bin_length, CHAR *p_prototxt_parm, CUSTNN_CONCAT_PARM* p_parm,  UINTPTR temp_buf_addr,
 NN_DATA* input_info, NN_CUSTOM_DIM* input_dim, INT32 *p_isf_parm, UINT32 input_num,
 NN_DATA* output_info, NN_CUSTOM_DIM* output_dim, INT32 *p_osf_parm, UINT32 output_num);
extern HD_RESULT custnn_cpu_slice_process(UINTPTR bin_length, CHAR *p_prototxt_parm, CUSTNN_SLICE_PARM* p_parm, UINTPTR temp_buf_addr,
 NN_DATA* input_info, NN_CUSTOM_DIM* input_dim, INT32 *p_isf_parm, UINT32 input_num,
 NN_DATA* output_info, NN_CUSTOM_DIM* output_dim, INT32 *p_osf_parm, UINT32 output_num);
#endif
#ifdef __cplusplus
}
#endif

#endif  /* _CUSTNN_CPU_H_ */
