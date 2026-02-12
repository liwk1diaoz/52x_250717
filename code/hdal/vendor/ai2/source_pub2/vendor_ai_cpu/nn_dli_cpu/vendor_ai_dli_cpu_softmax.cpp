/**
	@brief Source file of DLI cpu softmax layer.

	@file vendor_ai_dli_cpu_softmax.cpp

	@ingroup dli_cpu_softmax

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <iostream>
#include <map>
#include "vendor_ai_util.h"
#include "vendor_ai_dli_cpu_softmax.hpp"
#include "vendor_ai_dli_cpu_utils.hpp"

#include <arm_compute/runtime/NEON/functions/NESoftmaxLayer.h> // for arm_compute::NESoftmaxLayer

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT dli_cpu_softmax_init(NN_DLI_SOFTMAX_PARM *p_parm)
{
	//DBG_IND("entry\r\n");
	//DBG_IND(" Beta: %f, Axis: %ld\n", p_parm->beta, p_parm->axis);

	UINTPTR input_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input_info_va));
	UINTPTR output_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va));

	auto p_layer = new arm_compute::NESoftmaxLayer();

	p_layer->configure(
		reinterpret_cast<arm_compute::ITensor *>(input_obj_ptr),
		reinterpret_cast<arm_compute::ITensor *>(output_obj_ptr),
		p_parm->beta,
		p_parm->axis);

	p_parm->obj_ptr = reinterpret_cast<UINTPTR>(p_layer);

	return HD_OK;
}

HD_RESULT dli_cpu_softmax_proc(NN_DLI_SOFTMAX_PARM *p_parm)
{
	//DBG_IND("entry\r\n");

	NN_DLI_TENSOR_INFO *p_input_info = reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input_info_va);
	NN_DLI_TENSOR_INFO *p_output_info = reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va);

	// Calculate temp buffer
	// Temp buffer is shared by inputs and outputs
	UINT32 input_tmp_size = sizeof(FLOAT) * get_tensor_num_element(p_input_info);
	p_input_info->trans_va = p_parm->temp_buf_va;
	p_output_info->trans_va = p_parm->temp_buf_va + input_tmp_size;

	// Dequantize input data: Fix to Float
	dequantize_tensor(p_input_info);

	// Import memory
	import_acl_tensor_f32only(p_input_info);
	import_acl_tensor_f32only(p_output_info);

	// Execute workload
	arm_compute::IFunction *p_layer = reinterpret_cast<arm_compute::IFunction *>(p_parm->obj_ptr);
	p_layer->run();

	// Quantize output data: Float to Fix
	quantize_tensor(p_output_info);

	// Debug: Show raw data
	//show_raw_data<INT16>(" %d", p_parm->output_info.data_va, get_tensor_num_element(&(p_parm->output_info)));

	return HD_OK;
}

HD_RESULT dli_cpu_softmax_uninit(NN_DLI_SOFTMAX_PARM *p_parm)
{
	//DBG_IND("entry\r\n");

	NN_DLI_TENSOR_INFO *p_input_info = reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input_info_va);
	NN_DLI_TENSOR_INFO *p_output_info = reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va);

	destroy_acl_tensor(p_input_info);
	destroy_acl_tensor(p_output_info);

	return HD_OK;
}
