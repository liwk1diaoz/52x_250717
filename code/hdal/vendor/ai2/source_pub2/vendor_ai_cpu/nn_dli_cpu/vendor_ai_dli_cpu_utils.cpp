/**
	@brief Source file of DLI cpu utility functions.

	@file vendor_ai_dli_cpu_utils.cpp

	@ingroup dli_cpu_utils

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "vendor_ai_util.h"
#include "nvtnn/nvtnn_lib.h"
#include "vendor_ai_dli_cpu_utils.hpp"
#include <arm_compute/runtime/IFunction.h> // for arm_compute::IFunction
#include <arm_compute/runtime/Tensor.h> // for arm_compute::Tensor
#include <arm_compute/core/Types.h> // for arm_compute::DataLayout

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define DEBUG_INFO 0 // 0: off, 1: on

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
// Calculate original strides
static VOID calculate_strides(
	const UINT32 shape[NN_DLI_AXIS_NUM],
	UINT32 strides[NN_DLI_AXIS_NUM],
	UINT32 element_size)
{
	strides[0]=element_size;
	for(UINT32 i=1;i<NN_DLI_AXIS_NUM;i++) {
		if(shape[i]==0)
			break;
		strides[i]=shape[i-1]*strides[i-1];
	}
	return;
}

#if DEBUG_INFO
static VOID show_dli_quantization_info(NN_DLI_QUANTIZATION_INFO *p_info)
{
	if(p_info == nullptr)
		return;

	DBG_IND("ACL scale: %f, offset: %ld\r\n", p_info->scale, p_info->offset);
	DBG_IND("FMT frac: %d, int: %d, sign: %d\r\n", p_info->fmt.frac_bits, p_info->fmt.int_bits, p_info->fmt.sign_bits);
	DBG_IND("SF mul: %ld, shf: %d\r\n", p_info->sf.mul, p_info->sf.shf);

	return;
}
#endif

// Float to Fix
static VOID do_quantize(UINTPTR p_in /*FLOAT*/, UINTPTR p_out /*VOID*/, UINT32 dim,
	const UINT32 shape[NN_DLI_AXIS_NUM],
	const UINT32 in_strides[NN_DLI_AXIS_NUM],
	const UINT32 out_strides[NN_DLI_AXIS_NUM],
	NN_FMT *p_fmt, NN_SF *p_sf)
{
	if(shape[dim] == 0) {
		DBG_ERR("shape[%ld] should not be 0\n", dim);
		return;
	}

	if(dim == 0) {
		nvtnn_floattofix_neon(reinterpret_cast<FLOAT *>(p_in), reinterpret_cast<VOID *>(p_out), shape[0], p_fmt, p_sf);
		return;
	}

	for(UINT32 i=0;i<shape[dim];i++) {
		do_quantize(p_in,p_out,dim-1,shape,in_strides,out_strides,p_fmt,p_sf);
		p_in+=in_strides[dim];
		p_out+=out_strides[dim];
	}
	return;
}

// Fix to Float
static VOID do_dequantize(UINTPTR p_in /*FLOAT*/, UINTPTR p_out /*VOID*/, UINT32 dim,
	const UINT32 shape[NN_DLI_AXIS_NUM],
	const UINT32 in_strides[NN_DLI_AXIS_NUM],
	const UINT32 out_strides[NN_DLI_AXIS_NUM],
	NN_FMT *p_fmt, NN_SF *p_sf)
{
	if(shape[dim] == 0) {
		DBG_ERR("shape[%ld] should not be 0\n", dim);
		return;
	}

	if(dim == 0) {
		nvtnn_fixtofloat_neon(reinterpret_cast<VOID *>(p_in), reinterpret_cast<FLOAT *>(p_out), shape[0], p_fmt, p_sf);
		return;
	}

	for(UINT32 i=0;i<shape[dim];i++) {
		do_dequantize(p_in,p_out,dim-1,shape,in_strides,out_strides,p_fmt,p_sf);
		p_in+=in_strides[dim];
		p_out+=out_strides[dim];
	}
	return;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
BOOL is_quantized_type_all(UINT32 type)
{
	if( type == NN_DLI_DATA_TYPE_QASYMM16 ||
		type == NN_DLI_DATA_TYPE_QASYMM8 ||
		type == NN_DLI_DATA_TYPE_QASYMM8_SIGNED ||
		type == NN_DLI_DATA_TYPE_QSYMM16 ||
		type == NN_DLI_DATA_TYPE_QSYMM8 ) {
		return TRUE;
	} else if (type == NN_DLI_DATA_TYPE_QSYMM8_PER_CHANNEL) {
		// TODO
		return FALSE;
	} else {
		// Do nothing
	}
	return FALSE;
}

BOOL is_quantized_type_16bit(UINT32 type)
{
	if( type == NN_DLI_DATA_TYPE_QASYMM16 ||
		type == NN_DLI_DATA_TYPE_QSYMM16 ) {
		return TRUE;
	}
	return FALSE;
}

BOOL is_quantized_type_8bit(UINT32 type)
{
	if( type == NN_DLI_DATA_TYPE_QASYMM8 ||
		type == NN_DLI_DATA_TYPE_QASYMM8_SIGNED ||
		type == NN_DLI_DATA_TYPE_QSYMM8 ) {
		return TRUE;
	} else if (type == NN_DLI_DATA_TYPE_QSYMM8_PER_CHANNEL) {
		// TODO
		return FALSE;
	} else {
		// Do nothing
	}
	return FALSE;
}

// Get tensor's number of element.
UINT32 get_tensor_num_element(NN_DLI_TENSOR_INFO *p_info)
{
	if(p_info == nullptr || p_info->data_type == NN_DLI_DATA_TYPE_UNKNOWN)
		return 0;

	UINT32 ret = 1;
	for(INT32 i = 0; i < NN_DLI_AXIS_NUM; i++) {
		if(p_info->shape[i] == 0)
			break;
		ret *= p_info->shape[i];
	}
	return ret;
}

// Get tensor's size. It is without strides.
UINT32 get_tensor_size(NN_DLI_TENSOR_INFO *p_info)
{
	if(p_info == nullptr || p_info->data_type == NN_DLI_DATA_TYPE_UNKNOWN)
		return 0;
	return (p_info->strides[0] * get_tensor_num_element(p_info));
}

// Get tensor's total size. It is with strides.
UINT32 get_tensor_total_size(NN_DLI_TENSOR_INFO *p_info)
{
	if(p_info == nullptr || p_info->data_type == NN_DLI_DATA_TYPE_UNKNOWN)
		return 0;

	UINT32 ret = 0;
	for(INT32 i = NN_DLI_AXIS_NUM-1; i >= 0; i--) {
		if(p_info->shape[i] != 0) {
			ret = p_info->shape[i] * p_info->strides[i];
			break;
		}
	}
	return ret;
}

// Check tensor has stride or not
BOOL is_tensor_stride(NN_DLI_TENSOR_INFO *p_info)
{
	return (get_tensor_size(p_info) != get_tensor_total_size(p_info));
}

VOID show_dli_tensor_info(NN_DLI_TENSOR_INFO *p_info)
{
#if DEBUG_INFO
	if(p_info == nullptr)
		return;

	DBG_IND("Data type: %d, Fromat: %d, Data layout: %d, Trans_Enable: %d\n",
		p_info->data_type, p_info->format, p_info->data_layout, p_info->trans_enable);
	DBG_IND("Shape: [%ld,%ld,%ld,%ld,%ld,%ld]\n",
		p_info->shape[0],p_info->shape[1],p_info->shape[2],
		p_info->shape[3],p_info->shape[4],p_info->shape[5]);
	DBG_IND("Stride: [%ld,%ld,%ld,%ld,%ld,%ld]\n",
		p_info->strides[0],p_info->strides[1],p_info->strides[2],
		p_info->strides[3],p_info->strides[4],p_info->strides[5]);
	DBG_IND("Quantization Info Num: %ld\n", p_info->quant_info_nums);
	DBG_IND("Quantization Info va: 0x%x\n", p_info->quant_info_va);
	DBG_IND("Quantization Info pa: 0x%x\n", p_info->quant_info_pa);
	DBG_IND("Data va: 0x%x\n", p_info->data_va);
	DBG_IND("Data pa: 0x%x\n", p_info->data_pa);
	DBG_IND("Trans va: 0x%x\n", p_info->trans_va);
	DBG_IND("Trans pa: 0x%x\n", p_info->trans_pa);
	DBG_IND("Obj ptr: 0x%x\n", p_info->obj_ptr);
	DBG_IND("f32 obj ptr: 0x%x\n", p_info->f32_obj_ptr);

	NN_DLI_QUANTIZATION_INFO *p_quant_info =
		reinterpret_cast<NN_DLI_QUANTIZATION_INFO *>(p_info->quant_info_va);
	for(UINT32 i = 0; i < p_info->quant_info_nums; i++) {
		DBG_IND("Quantization Info [%ld]\n", i);
		show_dli_quantization_info(&(p_quant_info[i]));
	}
#endif

	return;
}

VOID quantize_tensor(NN_DLI_TENSOR_INFO *p_info)
{
	if(p_info == nullptr)
		return;
	if(is_quantized_type_all(p_info->data_type) == FALSE)
		return;
	if(p_info->quant_info_nums == 0)
		return;

	NN_DLI_QUANTIZATION_INFO *p_quant_info =
		reinterpret_cast<NN_DLI_QUANTIZATION_INFO *>(p_info->quant_info_va);

	if(is_tensor_stride(p_info)) {
		UINT32 in_strides[NN_DLI_AXIS_NUM]={};
		INT32 num_dim=NN_DLI_AXIS_NUM;

		calculate_strides(p_info->shape, in_strides, sizeof(FLOAT));
		while(num_dim > 1 && p_info->shape[num_dim-1] == 0) {
			num_dim--;
		}

		do_quantize(p_info->trans_va, p_info->data_va, num_dim-1,
			p_info->shape, in_strides, p_info->strides,
			&(p_quant_info->fmt), &(p_quant_info->sf));
	} else {
		nvtnn_floattofix_neon(reinterpret_cast<FLOAT *>(p_info->trans_va),
			reinterpret_cast<VOID *>(p_info->data_va),
			get_tensor_num_element(p_info),
			&(p_quant_info->fmt), &(p_quant_info->sf));
	}

	return;
}

VOID dequantize_tensor(NN_DLI_TENSOR_INFO *p_info)
{
	if(p_info == nullptr)
		return;
	if(is_quantized_type_all(p_info->data_type) == FALSE)
		return;
	if(p_info->quant_info_nums == 0)
		return;

	NN_DLI_QUANTIZATION_INFO *p_quant_info =
		reinterpret_cast<NN_DLI_QUANTIZATION_INFO *>(p_info->quant_info_va);

	if(is_tensor_stride(p_info)) {
		UINT32 out_strides[NN_DLI_AXIS_NUM]={};
		INT32 num_dim=NN_DLI_AXIS_NUM;

		calculate_strides(p_info->shape, out_strides, sizeof(FLOAT));
		while(num_dim > 1 && p_info->shape[num_dim-1]==0) {
			num_dim--;
		}

		do_dequantize(p_info->data_va, p_info->trans_va, num_dim-1,
			p_info->shape, p_info->strides, out_strides,
			&(p_quant_info->fmt), &(p_quant_info->sf));
	} else {
		nvtnn_fixtofloat_neon(reinterpret_cast<VOID *>(p_info->data_va),
			reinterpret_cast<FLOAT *>(p_info->trans_va),
			get_tensor_num_element(p_info),
			&(p_quant_info->fmt), &(p_quant_info->sf));
	}
	return;
}

static arm_compute::DataType convert_nn_dli_data_type(UINT32 data_type)
{
	arm_compute::DataType ret = arm_compute::DataType::UNKNOWN;
	switch(data_type)
	{
		case NN_DLI_DATA_TYPE_QSYMM8:
			ret = arm_compute::DataType::QSYMM8;
			break;
		case NN_DLI_DATA_TYPE_QASYMM8:
			ret = arm_compute::DataType::QASYMM8;
			break;
		case NN_DLI_DATA_TYPE_QASYMM8_SIGNED:
			ret = arm_compute::DataType::QASYMM8_SIGNED;
			break;
		case NN_DLI_DATA_TYPE_QSYMM8_PER_CHANNEL:
			ret = arm_compute::DataType::QSYMM8_PER_CHANNEL;
			break;
		case NN_DLI_DATA_TYPE_QSYMM16:
			ret = arm_compute::DataType::QSYMM16;
			break;
		case NN_DLI_DATA_TYPE_QASYMM16:
			ret = arm_compute::DataType::QASYMM16;
			break;
		case NN_DLI_DATA_TYPE_F32:
			ret = arm_compute::DataType::F32;
			break;
		default:
			DBG_ERR("Unknown data type (%ld)\n", data_type);
			break;
	}
	return ret;
}

// Debug
#if 0
static VOID show_acl_tensor_info(const arm_compute::ITensor* p_acl_tensor)
{
	size_t tot_size = p_acl_tensor->info()->total_size();
	const arm_compute::TensorShape& shape = p_acl_tensor->info()->tensor_shape();
	const arm_compute::Strides& strides = p_acl_tensor->info()->strides_in_bytes();
	const arm_compute::QuantizationInfo& quantizationInfo = p_acl_tensor->info()->quantization_info();
	const arm_compute::DataLayout& dataLayout = p_acl_tensor->info()->data_layout();

	DBG_IND("Total size: %d, BufferPtr: 0x%p\n", tot_size, reinterpret_cast<const void*>(p_acl_tensor->buffer()));

	DBG_IND("Shape:\n");
	DBG_IND("");
	for(size_t i = 0; i < p_acl_tensor->info()->num_dimensions(); i++) {
		printf(" %d", shape[i]);
	}
	printf("\n");

	DBG_IND("Strides_in_bytes:\n");
	DBG_IND("");
	for(size_t i = 0; i < p_acl_tensor->info()->num_dimensions(); i++) {
		printf(" %d", strides[i]);
	}
	printf("\n");

	DBG_IND("Has_padding: %d\n", p_acl_tensor->info()->has_padding());

	DBG_IND("Quantization scale:\n");
	DBG_IND("");
	for(auto&& scale : quantizationInfo.scale()) {
		printf(" %lf", scale);
	}
	printf("\n");

	DBG_IND("Quantization offset:\n");
	DBG_IND("");
	for(auto&& offset : quantizationInfo.offset()) {
		printf(" %d", offset);
	}
	printf("\n");

	DBG_IND("Data_layout: %ld\n", static_cast<UINT32>(dataLayout));

	return;
}
#endif

static arm_compute::TensorShape build_acl_tensor_shape(const UINT32 shape[NN_DLI_AXIS_NUM])
{
	arm_compute::TensorShape acl_shape;

	// arm_compute tensors are (width, height, channels, batch).
	for (UINT32 i = 0; i < NN_DLI_AXIS_NUM; i++)
	{
		if(shape[i] == 0)
			break;

		// Note that our dimensions are stored in the same order to ACL's.
		acl_shape.set(i, shape[i], false);

		// TensorShape::set() flattens leading ones, so that batch size 1 cannot happen.
		// arm_compute tensors expect this.
	}

	return acl_shape;
}

VOID build_acl_tensor(NN_DLI_TENSOR_INFO *p_info, BOOL is_float)
{
	if(p_info == nullptr)
		return;
	if(p_info->data_type == NN_DLI_DATA_TYPE_UNKNOWN)
		return;

	if(is_float && p_info->f32_obj_ptr != 0) {
		// ACL Tensor is created.
		return;
	} else if (!is_float && p_info->obj_ptr != 0) {
		// ACL Tensor is created.
		return;
	}

	// Debug
	//show_dli_tensor_info(p_info);

	arm_compute::Tensor *p_tensor = new arm_compute::Tensor;
	arm_compute::DataType data_type = convert_nn_dli_data_type(p_info->data_type);
	if(is_float) {
		data_type = arm_compute::DataType::F32;
	}
	arm_compute::TensorInfo acl_tensor_info(build_acl_tensor_shape(p_info->shape), 1, data_type);
	p_tensor->allocator()->init(acl_tensor_info);
	if(is_float) {
		p_info->f32_obj_ptr = reinterpret_cast<UINTPTR>(p_tensor);
	} else {
		p_info->obj_ptr = reinterpret_cast<UINTPTR>(p_tensor);
	}

	// Debug
	//show_acl_tensor_info(p_tensor);

	return;
}

UINTPTR build_acl_tensor_f32only(NN_DLI_TENSOR_INFO *p_info)
{
	if(p_info == nullptr)
		return 0;

	// Check data type
	BOOL is_float = is_quantized_type_all(p_info->data_type);
	build_acl_tensor(p_info, is_float);
	UINTPTR obj_ptr = p_info->obj_ptr;
	if(is_float) {
		obj_ptr = p_info->f32_obj_ptr;
	}
	return obj_ptr;
}

VOID destroy_acl_tensor(NN_DLI_TENSOR_INFO *p_info)
{
	if(p_info == nullptr)
		return;
	if(p_info->data_type == NN_DLI_DATA_TYPE_UNKNOWN)
		return;

	if(p_info->obj_ptr != 0) {
		arm_compute::Tensor *p_tensor = reinterpret_cast<arm_compute::Tensor *>(p_info->obj_ptr);
		delete p_tensor;
		p_info->obj_ptr = 0;
	}

	if(p_info->f32_obj_ptr != 0) {
		arm_compute::Tensor *p_tensor = reinterpret_cast<arm_compute::Tensor *>(p_info->f32_obj_ptr);
		delete p_tensor;
		p_info->f32_obj_ptr = 0;
	}

	return;
}

VOID import_acl_tensor(NN_DLI_TENSOR_INFO *p_info, BOOL is_float)
{
	if(p_info == nullptr)
		return;
	if(p_info->data_type == NN_DLI_DATA_TYPE_UNKNOWN)
		return;

	arm_compute::Tensor *p_tensor = reinterpret_cast<arm_compute::Tensor *>(p_info->obj_ptr);
	VOID *ptr = reinterpret_cast<VOID *>(p_info->data_va);
	if(is_float) {
		p_tensor = reinterpret_cast<arm_compute::Tensor *>(p_info->f32_obj_ptr);
		ptr = reinterpret_cast<VOID *>(p_info->trans_va);
	}

	p_tensor->allocator()->import_memory(ptr);
	return;
}

VOID import_acl_tensor_f32only(NN_DLI_TENSOR_INFO *p_info)
{
	if(p_info == nullptr)
		return;

	// Check data type
	BOOL is_float = is_quantized_type_all(p_info->data_type);
	import_acl_tensor(p_info, is_float);

	return;
}

UINT32 get_acl_tensor_total_size(NN_DLI_TENSOR_INFO *p_info, BOOL is_float)
{
	if(p_info == nullptr) {
		return 0;
	}
	if(p_info->data_type == NN_DLI_DATA_TYPE_UNKNOWN) {
		return 0;
	}

	arm_compute::ITensor *p_tensor = nullptr;
	if(is_float) {
		p_tensor = reinterpret_cast<arm_compute::ITensor *>(p_info->f32_obj_ptr);
	} else {
		p_tensor = reinterpret_cast<arm_compute::ITensor *>(p_info->obj_ptr);
	}

	if(p_tensor == nullptr) {
		return 0;
	}
	return p_tensor->info()->total_size();
}

VOID destroy_acl_workload(NN_DLI_LAYER_PARM *p_layer)
{
	if(p_layer == nullptr)
		return;
	if(p_layer->obj_ptr == 0 || p_layer->obj_ptr == UINTPTR_MAX)
		return;

	//DBG_IND("p_layer->obj_ptr %p\r\n", p_layer->obj_ptr);
	arm_compute::IFunction *p_workload = reinterpret_cast<arm_compute::IFunction *>(p_layer->obj_ptr);
	delete p_workload;
	p_layer->obj_ptr = 0;

	return;
}
