/**
	@brief Source file of custom layers.

	@file vendor_ai_cpu_custnn.c

	@ingroup vendor_ai_cpu_custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"
#include "vendor_ai_cpu_custnn.h"         // NN_CUSTOMER
#include "vendor_ai_cpu_custnn_sample.h"
#include "hd_common.h"
#include "nvtnn/nvtnn_lib.h"

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define CUST_ELT_PRINT_PARM         FALSE


/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT custnn_cpu_elt_process(CUSTNN_ELT_PARM* p_parm,
	NN_DATA* input_info, NN_CUSTOM_DIM* input_dim,
	NN_DATA* output_info, NN_CUSTOM_DIM* output_dim,
	UINTPTR in0_temp_addr, UINTPTR in1_temp_addr)
{
	INT32 size_width = input_dim[0].dim[0];
	INT32 size_height = input_dim[0].dim[1];
	INT32 size_channel = input_dim[0].dim[2];
	INT32 batch_num = input_dim[0].dim[3];

	INT32 in_ofs_line_ofs = input_dim[0].ofs[0];
	INT32 in_ofs_channel_ofs = input_dim[0].ofs[1];
	INT32 in_ofs_batch_ofs = input_dim[0].ofs[2];
	INT32 out_ofs_line_ofs = output_dim[0].ofs[0];
	INT32 out_ofs_channel_ofs = output_dim[0].ofs[1];
	INT32 out_ofs_batch_ofs = output_dim[0].ofs[2];
	INT32 elt_out_shift = p_parm->elt_out_shift;

	NN_SF dummy_sf = {1, 0};
	INT64 val0 = 0, val1 = 0, oval = 0;// scval=0, sfval=0;

	INT32 elt0_bit = ((p_parm->in0_type >> 1) > 0) ? 16 : 8;
	INT32 elt1_bit = ((p_parm->in1_type >> 1) > 0) ? 16 : 8;
	INT32 out_bit  = ((p_parm->out_type >> 1) > 0) ? 16 : 8;
	INT32 outBitDepth = (out_bit == 16) ? 1 : 0;

	INT32 elt0_sign = ((p_parm->in0_type & 0x1) > 0) ? 0 : 1;
	INT32 elt1_sign = ((p_parm->in1_type & 0x1) > 0) ? 0 : 1;
	INT32 out_sign  = ((p_parm->out_type & 0x1) > 0) ? 0 : 1;

	INT32 elt0_lob = -(1 << (elt0_bit - 1));
	INT32 elt0_upb = (1 << (elt0_bit - 1)) - 1;
	INT32 elt1_lob = -(1 << (elt1_bit - 1));
	INT32 elt1_upb = (1 << (elt1_bit - 1)) - 1;
	INT32 elt_olob = -(1 << (out_bit - 1));
	INT32 elt_oupb = (1 << (out_bit - 1)) - 1;

	INT64 elt_in_upb   = (((INT64)1) << (48 - 1)) - (INT64)1;
	INT64 elt_in_dnb   = -(((INT64)1) << (48 - 1));
	INT64 elt_out_upb   = (((INT64)1) << (32 - 1)) - (INT64)1;
	INT64 elt_out_dnb   = -(((INT64)1) << (32 - 1));

	INT32 Sout = 0;

#if CUST_ELT_PRINT_PARM
	DBG_DUMP("in1 addr = 0x%08X\n", (unsigned int)input_info[0].va);
	DBG_DUMP("in2 addr = 0x%08X\n", (unsigned int)input_info[1].va);
	DBG_DUMP("in0_type: %d\n", p_parm->in0_type);
	DBG_DUMP("in1_type: %d\n", p_parm->in1_type);
	DBG_DUMP("out_type: %d\n", p_parm->out_type);
	DBG_DUMP("size_width: %d\n", size_width);
	DBG_DUMP("size_height: %d\n", size_height);
	DBG_DUMP("size_channel: %d\n", size_channel);
	DBG_DUMP("batch_num: %d\n", batch_num);
	DBG_DUMP("in_ofs_line_ofs: %d\n", in_ofs_line_ofs);
	DBG_DUMP("in_ofs_channel_ofs: %d\n", in_ofs_channel_ofs);
	DBG_DUMP("in_ofs_batch_ofs: %d\n", in_ofs_batch_ofs);
	DBG_DUMP("out_ofs_line_ofs: %d\n", out_ofs_line_ofs);
	DBG_DUMP("out_ofs_channel_ofs: %d\n", out_ofs_channel_ofs);
	DBG_DUMP("out_ofs_batch_ofs: %d\n", out_ofs_batch_ofs);
	DBG_DUMP("elt_mode: %d\n", p_parm->elt_mode);
	DBG_DUMP("elt_coeff0: %d\n", p_parm->elt_coeff0);
	DBG_DUMP("elt_coeff1: %d\n", p_parm->elt_coeff1);
	DBG_DUMP("elt_shift0: %d\n", p_parm->elt_shift0);
	DBG_DUMP("elt_shift1: %d\n", p_parm->elt_shift1);
	DBG_DUMP("elt_out_shift: %d\n", elt_out_shift);
#endif

	UINTPTR in0_addr_b, in0_addr_c, in0_addr_h;
	UINTPTR in1_addr_b, in1_addr_c, in1_addr_h;
	UINTPTR in0_temp_addr_b, in0_temp_addr_c, in0_temp_addr_h;
	UINTPTR in1_temp_addr_b, in1_temp_addr_c, in1_temp_addr_h;
	UINTPTR out_addr_b, out_addr_c, out_addr_h;

	for (INT32 b_idx = 0; b_idx < batch_num; b_idx++) {

		in0_addr_b = input_info[0].va + b_idx * in_ofs_batch_ofs;
		in1_addr_b = input_info[1].va + b_idx * in_ofs_batch_ofs;
		in0_temp_addr_b = in0_temp_addr + b_idx * in_ofs_batch_ofs;
		in1_temp_addr_b = in1_temp_addr + b_idx * in_ofs_batch_ofs;
		out_addr_b = output_info[0].va + b_idx * out_ofs_batch_ofs;

		for (INT32 c_idx = 0; c_idx < size_channel; c_idx++) {

			in0_addr_c = in0_addr_b + c_idx * in_ofs_channel_ofs;
			in1_addr_c = in1_addr_b + c_idx * in_ofs_channel_ofs;
			in0_temp_addr_c = in0_temp_addr_b + c_idx * in_ofs_channel_ofs;
			in1_temp_addr_c = in1_temp_addr_b + c_idx * in_ofs_channel_ofs;
			out_addr_c = out_addr_b + c_idx * out_ofs_channel_ofs;

			for (INT32 h_idx = 0; h_idx < size_height; h_idx++) {

				in0_addr_h = in0_addr_c + h_idx * in_ofs_line_ofs;
				in1_addr_h = in1_addr_c + h_idx * in_ofs_line_ofs;
				in0_temp_addr_h = in0_temp_addr_c + h_idx * in_ofs_line_ofs;
				in1_temp_addr_h = in1_temp_addr_c + h_idx * in_ofs_line_ofs;
				out_addr_h = out_addr_c + h_idx * out_ofs_line_ofs;

				nvtnn_convert_fmt_c((VOID *)in0_addr_h, (VOID *)in0_temp_addr_h, size_width, &input_info[0].fmt, &output_info[0].fmt, &dummy_sf, &dummy_sf);
				nvtnn_convert_fmt_c((VOID *)in1_addr_h, (VOID *)in1_temp_addr_h, size_width, &input_info[1].fmt, &output_info[0].fmt, &dummy_sf, &dummy_sf);

				for (INT32 w_idx = 0; w_idx < size_width; w_idx++) {

					INT64 tmp_in0_val = 0;
					INT64 tmp_in1_val = 0;

					if (elt0_bit == 16) {
						if (elt0_sign)
							tmp_in0_val = ((INT16 *)in0_temp_addr_h)[w_idx];
						else
							tmp_in0_val = ((UINT16 *)in0_temp_addr_h)[w_idx];
					} else {
						if (elt0_sign)
							tmp_in0_val = ((INT8 *)in0_temp_addr_h)[w_idx];
						else
							tmp_in0_val = ((UINT8 *)in0_temp_addr_h)[w_idx];
					}

					if (elt1_bit == 16) {
						if (elt1_sign)
							tmp_in1_val = ((INT16 *)in1_temp_addr_h)[w_idx];
						else
							tmp_in1_val = ((UINT16 *)in1_temp_addr_h)[w_idx];
					} else {
						if (elt1_sign)
							tmp_in1_val = ((INT8 *)in1_temp_addr_h)[w_idx];
						else
							tmp_in1_val = ((UINT8 *)in1_temp_addr_h)[w_idx];
					}

					val0 = (INT64)(tmp_in0_val * (INT64)p_parm->elt_coeff0);
					val1 = (INT64)(tmp_in1_val * (INT64)p_parm->elt_coeff1);

					if (p_parm->elt_shift0 < 0) {
						val0 = custnn_int64_leftShift(val0, (unsigned char)((-1)*p_parm->elt_shift0));
					} else {
						val0 = custnn_int64_rightShift(val0, (unsigned char)(p_parm->elt_shift0));
					}

					if (p_parm->elt_shift1 < 0) {
						val1 = custnn_int64_leftShift(val1, (unsigned char)((-1)*p_parm->elt_shift1));
					} else {
						val1 = custnn_int64_rightShift(val1, (unsigned char)(p_parm->elt_shift1));
					}

					val0 = custnn_int64_clamp(val0, elt_in_dnb, elt_in_upb);
					val1 = custnn_int64_clamp(val1, elt_in_dnb, elt_in_upb);

					if (p_parm->elt_mode == 0) {
						//add
						oval = val0 + val1;
						oval = custnn_int64_rightShift(oval, elt_out_shift);
					} else if (p_parm->elt_mode == 1) {
						//multiply
						val0 = custnn_int64_clamp(val0, elt0_lob, elt0_upb);
						val1 = custnn_int64_clamp(val1, elt1_lob, elt1_upb);
						oval = val0 * val1;
						oval = custnn_int64_rightShift(oval, elt_out_shift);
					} else {
						//max
						oval = (val0 > val1) ? val0 : val1;
						oval = custnn_int64_rightShift(oval, elt_out_shift);
					}

					oval = custnn_int64_clamp(oval, elt_out_dnb, elt_out_upb);
					Sout = (INT32)custnn_int64_clamp((INT64)oval, elt_olob, elt_oupb);

					if (outBitDepth){
						if (out_sign)
							((INT16 *)out_addr_h)[w_idx] = (INT16)Sout;
						else
							((UINT16 *)out_addr_h)[w_idx] = (UINT16)Sout;
					} else {
						if (out_sign)
							((INT8 *)out_addr_h)[w_idx] = (INT8)Sout;
						else
							((UINT8 *)out_addr_h)[w_idx] = (UINT8)Sout;
					}
				}
				//nvtnn_convert_fmt_c((VOID *)out_addr_h, (VOID *)out_addr_h, size_width, &output_info[0].fmt, &output_info[0].fmt, &dummy_sf, &dummy_sf);
			}
		}
	}

	return HD_OK;
}

