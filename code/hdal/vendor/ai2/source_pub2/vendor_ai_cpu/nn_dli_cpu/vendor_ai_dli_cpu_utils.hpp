/**
	@brief Header file of definition of DLI cpu utility functions.

	@file vendor_ai_dli_cpu_utils.hpp

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_DLI_CPU_UTILS_HPP_
#define _VENDOR_AI_DLI_CPU_UTILS_HPP_

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	INCLUDE FILES
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "kwrap/type.h"
#include "vendor_ai_net/nn_dli.h"

#ifdef __cplusplus
}
#endif

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
// Check if tensor is quantized in 8 or 16 bit.
BOOL is_quantized_type_all(UINT32 type);
// Check if tensor is quantized in 16bit.
BOOL is_quantized_type_16bit(UINT32 type);
// Check if tensor is quantized in 8bit.
BOOL is_quantized_type_8bit(UINT32 type);
// Get tensor's number of element.
UINT32 get_tensor_num_element(NN_DLI_TENSOR_INFO *p_info);
// Get tensor's data size. It is without strides.
UINT32 get_tensor_size(NN_DLI_TENSOR_INFO *p_info);
// Get tensor's total data size. It is with strides.
UINT32 get_tensor_total_size(NN_DLI_TENSOR_INFO *p_info);
// Check if tensor has stride.
BOOL is_tensor_stride(NN_DLI_TENSOR_INFO *p_info);
// Show DLI tensor info
VOID show_dli_tensor_info(NN_DLI_TENSOR_INFO *p_info);
// Quantize tensor
VOID quantize_tensor(NN_DLI_TENSOR_INFO *p_info);
// Dequantize tensor
VOID dequantize_tensor(NN_DLI_TENSOR_INFO *p_info);
// Build ACL Tensor
VOID build_acl_tensor(NN_DLI_TENSOR_INFO *p_info, BOOL is_float);
// Build ACL Tensor in f32 and return it
UINTPTR build_acl_tensor_f32only(NN_DLI_TENSOR_INFO *p_info);
// Destroy ACL Tensor
VOID destroy_acl_tensor(NN_DLI_TENSOR_INFO *p_info);
// Import memory into ACL Tensor
VOID import_acl_tensor(NN_DLI_TENSOR_INFO *p_info, BOOL is_float);
// Import memory into ACL Tensor f32
VOID import_acl_tensor_f32only(NN_DLI_TENSOR_INFO *p_info);
// Get ACL Tensor's total size
UINT32 get_acl_tensor_total_size(NN_DLI_TENSOR_INFO *p_info, BOOL is_float);
// Destory ACL Workdload
VOID destroy_acl_workload(NN_DLI_LAYER_PARM *p_layer);

template<typename T>
static inline VOID show_raw_data(const char *fmt, UINTPTR ptr, UINT32 num_element)
{
	printf("\nraw data:\n");

	const T* const bufferPtr = reinterpret_cast<T *>(ptr);
	for(UINT32 i = 0; i < num_element; i++) {
		printf(fmt, bufferPtr[i]);
		if((i & 0xF) == 0xF)
			printf("\n");
	}
	printf("\n");

	return;
}

#endif /* _VENDOR_AI_DLI_CPU_UTILS_HPP_ */
