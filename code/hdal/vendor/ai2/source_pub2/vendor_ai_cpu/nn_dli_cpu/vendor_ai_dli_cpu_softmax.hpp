/**
	@brief Header file of definition of DLI softmax layer.

	@file vendor_ai_dli_cpu_softmax.hpp

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_DLI_CPU_SOFTMAX_HPP_
#define _VENDOR_AI_DLI_CPU_SOFTMAX_HPP_

/********************************************************************
 MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	INCLUDE FILES
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "hd_type.h"
#include "vendor_ai_net/nn_dli.h"

#ifdef __cplusplus
}
#endif


/********************************************************************
	TYPE DEFINITION
********************************************************************/

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT dli_cpu_softmax_init(NN_DLI_SOFTMAX_PARM *p_parm);
HD_RESULT dli_cpu_softmax_proc(NN_DLI_SOFTMAX_PARM *p_parm);
HD_RESULT dli_cpu_softmax_uninit(NN_DLI_SOFTMAX_PARM *p_parm);

#endif  /* _VENDOR_AI_DLI_CPU_SOFTMAX_HPP_ */
