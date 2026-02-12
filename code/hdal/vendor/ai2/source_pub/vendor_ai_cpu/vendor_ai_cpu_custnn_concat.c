/**
	@brief Source file of custom layers.

	@file net_cust_layer.c

	@ingroup custnn

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
#include <string.h>

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define CUST_CONCAT_PRINT_PARM      FALSE

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#if CUST_SUPPORT_MULTI_IO
HD_RESULT custnn_cpu_concat_process(UINTPTR bin_length, CHAR *p_prototxt_parm, CUSTNN_CONCAT_PARM* p_parm, UINTPTR temp_buf_addr,
	NN_DATA* input_info, NN_CUSTOM_DIM* input_dim, INT32 *p_isf_parm, UINT32 input_num,
	NN_DATA* output_info, NN_CUSTOM_DIM* output_dim, INT32 *p_osf_parm, UINT32 output_num)
{
	INT32 axis = 0;
	if (bin_length == 0) { //prototxt mode
		axis = *(INT32 *)p_prototxt_parm;
	} else {               //bin mode
		axis = (INT32)p_parm->axis;
	}

	if (axis > 3) {
		printf("not supported axis: %d\n", (int)axis);
		return HD_ERR_NOT_SUPPORT;
	}

#if CUST_CONCAT_PRINT_PARM
	DBG_DUMP("axis: %d\n", axis);
	DBG_DUMP("input_num: %d\n", input_num);
	for (UINT32 i = 0; i < input_num; i++) {
		DBG_DUMP("input idx: %d\n", i);
		DBG_DUMP("input_info.size: %d\n", input_info[i].size);
		DBG_DUMP("input_info.fmt.sign_bits: %d\n", input_info[i].fmt.sign_bits);
		DBG_DUMP("input_info.fmt.int_bits: %d\n", input_info[i].fmt.int_bits);
		DBG_DUMP("input_info.fmt.frac_bits: %d\n", input_info[i].fmt.frac_bits);
		DBG_DUMP("input_info.fmt.reserved: %d\n", input_info[i].fmt.reserved);
		// width
		DBG_DUMP("input_dim.dim[0]: %d\n", input_dim[i].dim[0]);
		// height
		DBG_DUMP("input_dim.dim[1]: %d\n", input_dim[i].dim[1]);
		// channel
		DBG_DUMP("input_dim.dim[2]: %d\n", input_dim[i].dim[2]);
		// batch
		DBG_DUMP("input_dim.dim[3]: %d\n", input_dim[i].dim[3]);
		// reverse
		DBG_DUMP("input_dim.dim[4]: %d\n", input_dim[i].dim[4]);
		// line ofs
		DBG_DUMP("input_dim.ofs[0]: %d\n", input_dim[i].ofs[0]);
		// channel ofs
		DBG_DUMP("input_dim.ofs[1]: %d\n", input_dim[i].ofs[1]);
		// batch ofs
		DBG_DUMP("input_dim.ofs[2]: %d\n", input_dim[i].ofs[2]);
		// reverse
		DBG_DUMP("input_dim.ofs[3]: %d\n", input_dim[i].ofs[3]);
	}
	DBG_DUMP("output_num: %d\n", output_num);
	for (UINT32 i = 0; i < output_num; i++) {
		DBG_DUMP("output idx: %d\n", i);
		DBG_DUMP("output_info.size: %d\n", output_info[i].size);
		DBG_DUMP("output_info.fmt.sign_bits: %d\n", output_info[i].fmt.sign_bits);
		DBG_DUMP("output_info.fmt.int_bits: %d\n", output_info[i].fmt.int_bits);
		DBG_DUMP("output_info.fmt.frac_bits: %d\n", output_info[i].fmt.frac_bits);
		DBG_DUMP("output_info.fmt.reserved: %d\n", output_info[i].fmt.reserved);
		DBG_DUMP("output_dim.dim[0]: %d\n", output_dim[i].dim[0]);
		DBG_DUMP("output_dim.dim[1]: %d\n", output_dim[i].dim[1]);
		DBG_DUMP("output_dim.dim[2]: %d\n", output_dim[i].dim[2]);
		DBG_DUMP("output_dim.dim[3]: %d\n", output_dim[i].dim[3]);
		DBG_DUMP("output_dim.dim[4]: %d\n", output_dim[i].dim[4]);
		DBG_DUMP("output_dim.ofs[0]: %d\n", output_dim[i].ofs[0]);
		DBG_DUMP("output_dim.ofs[1]: %d\n", output_dim[i].ofs[1]);
		DBG_DUMP("output_dim.ofs[2]: %d\n", output_dim[i].ofs[2]);
		DBG_DUMP("output_dim.ofs[3]: %d\n", output_dim[i].ofs[3]);
	}
#endif

	NN_SF dummy_sf = {1, 0};
	UINTPTR input_addr_i, input_addr_b, input_addr_c, input_addr_h;
	UINTPTR output0_addr_i, output0_addr_b, output0_addr_c, output0_addr_h;
	UINTPTR output0_addr = output_info[0].va;
	UINT32 output0_byte = custnn_get_type(output_info[0].fmt) < 2 ? 1 : 2;

	for (UINT32 input_idx = 0; input_idx < input_num; input_idx++) {

		input_addr_i = input_info[input_idx].va;
		output0_addr_i = output0_addr;

		for (UINT32 b_idx = 0; b_idx < input_dim[input_idx].dim[3]; b_idx++) {

			input_addr_b = input_addr_i;
			output0_addr_b = output0_addr_i;

			for (UINT32 c_idx = 0; c_idx < input_dim[input_idx].dim[2]; c_idx++) {

				input_addr_c = input_addr_b;
				output0_addr_c = output0_addr_b;

				for (UINT32 h_idx = 0; h_idx < input_dim[input_idx].dim[1]; h_idx++) {

					input_addr_h = input_addr_c;
					output0_addr_h = output0_addr_c;

					nvtnn_convert_fmt_c((VOID *)input_addr_h, (VOID *)output0_addr_h, input_dim[input_idx].dim[0],
						&input_info[input_idx].fmt, &output_info[0].fmt, &dummy_sf, &dummy_sf);

					input_addr_c += input_dim[input_idx].ofs[0];
					output0_addr_c += output_dim[0].ofs[0];

				}

				input_addr_b += input_dim[input_idx].ofs[1];
				output0_addr_b += output_dim[0].ofs[1];

			}

			input_addr_i += input_dim[input_idx].ofs[2];
			output0_addr_i += output_dim[0].ofs[2];

		}

		if (axis == 0) {
			output0_addr += output_dim[0].ofs[2] * input_dim[input_idx].dim[3];
		} else if (axis == 1) {
			output0_addr += output_dim[0].ofs[1] * input_dim[input_idx].dim[2];
		} else if (axis == 2) {
			output0_addr += output_dim[0].ofs[0] * input_dim[input_idx].dim[1];
		} else if (axis == 3){
			output0_addr += output0_byte * input_dim[input_idx].dim[0];
		} else {
			return HD_ERR_NOT_SUPPORT;
		}

	}


	if (output_num == 1)
		return HD_OK;

	UINTPTR outputi_addr, outputi_addr_b, outputi_addr_c, outputi_addr_h;

	for (UINT32 output_idx = 1; output_idx < output_num; output_idx++) {

		output0_addr = output_info[0].va;
		outputi_addr = output_info[output_idx].va;

		for (UINT32 b_idx = 0; b_idx < output_dim[output_idx].dim[3]; b_idx++) {

			output0_addr_b = output0_addr + b_idx * output_dim[0].ofs[2];
			outputi_addr_b = outputi_addr + b_idx * output_dim[output_idx].ofs[2];

			for (UINT32 c_idx = 0; c_idx < output_dim[output_idx].dim[2]; c_idx++) {

				output0_addr_c = output0_addr_b + c_idx * output_dim[0].ofs[1];
				outputi_addr_c = outputi_addr_b + c_idx * output_dim[output_idx].ofs[1];

				for (UINT32 h_idx = 0; h_idx < output_dim[output_idx].dim[1]; h_idx++) {

					output0_addr_h = output0_addr_c + h_idx * output_dim[0].ofs[0];
					outputi_addr_h = outputi_addr_c + h_idx * output_dim[output_idx].ofs[0];

					nvtnn_convert_fmt_c((VOID *)output0_addr_h, (VOID *)outputi_addr_h, output_dim[output_idx].dim[0],
						&output_info[0].fmt, &output_info[output_idx].fmt, &dummy_sf, &dummy_sf);
				}
			}
		}
	}

	return HD_OK;
}
#endif

