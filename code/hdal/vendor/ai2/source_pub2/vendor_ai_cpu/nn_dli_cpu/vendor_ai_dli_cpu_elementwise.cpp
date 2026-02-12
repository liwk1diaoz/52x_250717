/**
	@brief Source file of DLI cpu elementwise layer.

	@file vendor_ai_dli_cpu_elementwise.cpp

	@ingroup dli_cpu_elementwise

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <iostream>
#include <map>
#include "vendor_ai_util.h"
#include "vendor_ai_dli_cpu_elementwise.hpp"
#include "vendor_ai_dli_cpu_utils.hpp"
#include <arm_compute/runtime/IFunction.h> // for arm_compute::IFunction
#include <arm_compute/runtime/NEON/functions/NEActivationLayer.h> // for arm_compute::NEActivationLayer
#include <arm_compute/runtime/NEON/functions/NEElementwiseUnaryLayer.h> // for arm_compute::NEElementwiseUnaryLayer
#include <arm_compute/runtime/NEON/functions/NEElementwiseOperations.h> // for arm_compute::NEElementwiseDivision
#include <arm_compute/runtime/NEON/functions/NEFloor.h> // for arm_compute::NEFloor

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
/* Workload function */
static VOID build_acl_activation_workload(NN_DLI_ElEMENTWISE_PARM *p_parm);
template <arm_compute::ElementWiseUnary op>
static VOID build_acl_elementwise_unary_workload(NN_DLI_ElEMENTWISE_PARM *p_parm);
template <class op>
static VOID build_acl_elementwise_binary_workload_f32only(NN_DLI_ElEMENTWISE_PARM *p_parm);

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
static VOID build_acl_activation_workload(NN_DLI_ElEMENTWISE_PARM *p_parm)
{
	//DBG_IND("entry\r\n");

	std::map<UINT32, arm_compute::ActivationLayerInfo::ActivationFunction> actMap = {
		{ NN_DLI_SQRT, arm_compute::ActivationLayerInfo::ActivationFunction::SQRT }
	};

	auto p_layer = new arm_compute::NEActivationLayer();
	arm_compute::ActivationLayerInfo activation_info(actMap[p_parm->mode]);

	UINTPTR input_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input1_info_va));
	UINTPTR output_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va));

	p_layer->configure(
		reinterpret_cast<arm_compute::ITensor *>(input_obj_ptr),
		reinterpret_cast<arm_compute::ITensor *>(output_obj_ptr),
		activation_info);

	p_parm->obj_ptr = reinterpret_cast<UINTPTR>(p_layer);
	return;
}

template <arm_compute::ElementWiseUnary op>
static VOID build_acl_elementwise_unary_workload(NN_DLI_ElEMENTWISE_PARM *p_parm)
{
	//DBG_IND("entry\r\n");
	auto p_layer = new arm_compute::NEElementwiseUnaryLayer<op>();

	UINTPTR input_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input1_info_va));
	UINTPTR output_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va));

	p_layer->configure(
		reinterpret_cast<arm_compute::ITensor *>(input_obj_ptr),
		reinterpret_cast<arm_compute::ITensor *>(output_obj_ptr));

	p_parm->obj_ptr = reinterpret_cast<UINTPTR>(p_layer);
	return;
}

template <class op>
static VOID build_acl_elementwise_binary_workload_f32only(NN_DLI_ElEMENTWISE_PARM *p_parm)
{
	//DBG_IND("entry\r\n");
	auto p_layer = new op();

	UINTPTR input1_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input1_info_va));
	UINTPTR input2_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input2_info_va));
	UINTPTR output_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va));

	p_layer->configure(
		reinterpret_cast<arm_compute::ITensor *>(input1_obj_ptr),
		reinterpret_cast<arm_compute::ITensor *>(input2_obj_ptr),
		reinterpret_cast<arm_compute::ITensor *>(output_obj_ptr));

	p_parm->obj_ptr = reinterpret_cast<UINTPTR>(p_layer);
	return;
}

static VOID build_acl_elementwise_floor_workload(NN_DLI_ElEMENTWISE_PARM *p_parm)
{
	//DBG_IND("entry\r\n");
	auto p_layer = new arm_compute::NEFloor();

	UINTPTR input_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input1_info_va));
	UINTPTR output_obj_ptr = build_acl_tensor_f32only(
		reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va));

	p_layer->configure(
		reinterpret_cast<arm_compute::ITensor *>(input_obj_ptr),
		reinterpret_cast<arm_compute::ITensor *>(output_obj_ptr));

	p_parm->obj_ptr = reinterpret_cast<UINTPTR>(p_layer);
	return;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT dli_cpu_elementwise_init(NN_DLI_ElEMENTWISE_PARM *p_parm)
{
	//DBG_IND("entry\r\n");

	switch(p_parm->mode) {
		case NN_DLI_SQRT:
			build_acl_activation_workload(p_parm);
			break;
		case NN_DLI_EXP:
			build_acl_elementwise_unary_workload<arm_compute::ElementWiseUnary::EXP>(p_parm);
			break;
		case NN_DLI_DIV:
			build_acl_elementwise_binary_workload_f32only<arm_compute::NEElementwiseDivision>(p_parm);
			break;
		case NN_DLI_LOG:
			build_acl_elementwise_unary_workload<arm_compute::ElementWiseUnary::LOG>(p_parm);
			break;
		case NN_DLI_POW:
			build_acl_elementwise_binary_workload_f32only<arm_compute::NEElementwisePower>(p_parm);
			break;
		case NN_DLI_SIN:
			build_acl_elementwise_unary_workload<arm_compute::ElementWiseUnary::SIN>(p_parm);
			break;
		case NN_DLI_FLOOR:
			build_acl_elementwise_floor_workload(p_parm);
			break;
		case NN_DLI_ROUND:
			build_acl_elementwise_unary_workload<arm_compute::ElementWiseUnary::ROUND>(p_parm);
			break;
		default:
			break;
	}
	return HD_OK;
}

HD_RESULT dli_cpu_elementwise_proc(NN_DLI_ElEMENTWISE_PARM *p_parm)
{
	//DBG_IND("entry\r\n");

	NN_DLI_TENSOR_INFO *p_input1_info = reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input1_info_va);
	NN_DLI_TENSOR_INFO *p_input2_info =
		(p_parm->input2_info_va == UINTPTR_MAX) ? nullptr : reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input2_info_va);
	NN_DLI_TENSOR_INFO *p_output_info = reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va);

	// Calculate temp buffer
	// Temp buffer is shared by inputs and outputs
	UINT32 input1_tmp_size = sizeof(FLOAT) * get_tensor_num_element(p_input1_info);
	UINT32 input2_tmp_size = 0;
	p_input1_info->trans_va = p_parm->temp_buf_va;
	if(p_input2_info != nullptr) {
		p_input2_info->trans_va = p_parm->temp_buf_va + input1_tmp_size;
		input2_tmp_size = sizeof(FLOAT) * get_tensor_num_element(p_input2_info);
	}
	p_output_info->trans_va = p_parm->temp_buf_va + input1_tmp_size + input2_tmp_size;

	// Dequantize input data: Fix to Float
	dequantize_tensor(p_input1_info);
	dequantize_tensor(p_input2_info);

	// Import memory
	import_acl_tensor_f32only(p_input1_info);
	import_acl_tensor_f32only(p_input2_info);
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

HD_RESULT dli_cpu_elementwise_uninit(NN_DLI_ElEMENTWISE_PARM *p_parm)
{
	//DBG_IND("entry\r\n");

	NN_DLI_TENSOR_INFO *p_input1_info = reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input1_info_va);
	NN_DLI_TENSOR_INFO *p_input2_info =
		(p_parm->input2_info_va == UINTPTR_MAX) ? nullptr : reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->input2_info_va);
	NN_DLI_TENSOR_INFO *p_output_info = reinterpret_cast<NN_DLI_TENSOR_INFO *>(p_parm->output_info_va);

	destroy_acl_tensor(p_input1_info);
	destroy_acl_tensor(p_input2_info);
	destroy_acl_tensor(p_output_info);

	return HD_OK;
}
