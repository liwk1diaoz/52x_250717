/**
	@brief Source file of DLI cpu.

	@file vendor_ai_dli_cpu.c

	@ingroup dli_cpu

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_util.h"

#include "vendor_ai_dli_cpu.h"
#include "vendor_ai_dli_cpu_layers.hpp"
#include "vendor_ai_dli_cpu_utils.hpp"

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
HD_RESULT dli_cpu_init(NN_DLI_LAYER_PARM *p_parm)
{
	HD_RESULT result = HD_OK;

	//DBG_IND("Sign: 0x%lX, Mode: 0x%lX, Size: %ld\n", p_parm->sign, p_parm->mode, p_parm->parm_size);

	switch(p_parm->mode) {
		case NN_MODE::NN_DLI_SQRT:
		case NN_MODE::NN_DLI_EXP:
		case NN_MODE::NN_DLI_DIV:
		case NN_MODE::NN_DLI_LOG:
		case NN_MODE::NN_DLI_POW:
		case NN_MODE::NN_DLI_SIN:
		case NN_MODE::NN_DLI_FLOOR:
		case NN_MODE::NN_DLI_ROUND:
			result = dli_cpu_elementwise_init(reinterpret_cast<NN_DLI_ElEMENTWISE_PARM *>(p_parm));
			break;
		case NN_MODE::NN_DLI_RESIZE:
			result = dli_cpu_resize_init(reinterpret_cast<NN_DLI_RESIZE_PARM *>(p_parm));
			break;
		case NN_MODE::NN_DLI_SOFTMAX:
			result = dli_cpu_softmax_init(reinterpret_cast<NN_DLI_SOFTMAX_PARM *>(p_parm));
			break;
		default:
			result = HD_ERR_NOT_SUPPORT;
			break;
	}

	if(result != HD_OK) {
		DBG_ERR("err, Mode 0x%lX\n", p_parm->mode);
		p_parm->obj_ptr = UINTPTR_MAX; // mark as init fail
	}

	return result;
}

HD_RESULT dli_cpu_proc(NN_DLI_LAYER_PARM *p_parm)
{
	HD_RESULT result = HD_OK;

	//DBG_IND("Sign: 0x%lX, Mode: 0x%lX, Size: %ld\n", p_parm->sign, p_parm->mode, p_parm->parm_size);

	if(p_parm->obj_ptr != UINTPTR_MAX) {
		switch(p_parm->mode) {
			case NN_MODE::NN_DLI_SQRT:
			case NN_MODE::NN_DLI_EXP:
			case NN_MODE::NN_DLI_DIV:
			case NN_MODE::NN_DLI_LOG:
			case NN_MODE::NN_DLI_POW:
			case NN_MODE::NN_DLI_SIN:
			case NN_MODE::NN_DLI_FLOOR:
			case NN_MODE::NN_DLI_ROUND:
				result = dli_cpu_elementwise_proc(reinterpret_cast<NN_DLI_ElEMENTWISE_PARM *>(p_parm));
				break;
			case NN_MODE::NN_DLI_RESIZE:
				result = dli_cpu_resize_proc(reinterpret_cast<NN_DLI_RESIZE_PARM *>(p_parm));
				break;
			case NN_MODE::NN_DLI_SOFTMAX:
				result = dli_cpu_softmax_proc(reinterpret_cast<NN_DLI_SOFTMAX_PARM *>(p_parm));
				break;
			default:
				result = HD_ERR_NOT_SUPPORT;
				break;
		}
		if(result != HD_OK) {
			DBG_ERR("err, Mode 0x%lX\n", p_parm->mode);
		}
	}

	return result;
}

HD_RESULT dli_cpu_uninit(NN_DLI_LAYER_PARM *p_parm)
{
	HD_RESULT result = HD_OK;

	//DBG_IND("Sign: 0x%lX, Mode: 0x%lX, Size: %ld\n", p_parm->sign, p_parm->mode, p_parm->parm_size);

	if(p_parm->obj_ptr != UINTPTR_MAX) {
		// Destroy ACL Workload
		destroy_acl_workload(p_parm);

		// Destory layer's ACL Tensors.
		switch(p_parm->mode) {
			case NN_MODE::NN_DLI_SQRT:
			case NN_MODE::NN_DLI_EXP:
			case NN_MODE::NN_DLI_DIV:
			case NN_MODE::NN_DLI_LOG:
			case NN_MODE::NN_DLI_POW:
			case NN_MODE::NN_DLI_SIN:
			case NN_MODE::NN_DLI_FLOOR:
			case NN_MODE::NN_DLI_ROUND:
				result = dli_cpu_elementwise_uninit(reinterpret_cast<NN_DLI_ElEMENTWISE_PARM *>(p_parm));
				break;
			case NN_MODE::NN_DLI_RESIZE:
				result = dli_cpu_resize_uninit(reinterpret_cast<NN_DLI_RESIZE_PARM *>(p_parm));
				break;
			case NN_MODE::NN_DLI_SOFTMAX:
				result = dli_cpu_softmax_uninit(reinterpret_cast<NN_DLI_SOFTMAX_PARM *>(p_parm));
				break;
			default:
				result = HD_ERR_NOT_SUPPORT;
				break;
		}
		if(result != HD_OK) {
			DBG_ERR("err, Mode 0x%lX\n", p_parm->mode);
		}
	}

	return result;
}
