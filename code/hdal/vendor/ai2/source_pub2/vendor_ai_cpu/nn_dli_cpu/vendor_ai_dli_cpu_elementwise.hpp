/**
	@brief Header file of definition of DLI elementwise layer.

	@file vendor_ai_dli_cpu_elementwise.hpp

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_DLI_CPU_ELEMETWISE_HPP_
#define _VENDOR_AI_DLI_CPU_ELEMETWISE_HPP_

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
HD_RESULT dli_cpu_elementwise_init(NN_DLI_ElEMENTWISE_PARM *p_parm);
HD_RESULT dli_cpu_elementwise_proc(NN_DLI_ElEMENTWISE_PARM *p_parm);
HD_RESULT dli_cpu_elementwise_uninit(NN_DLI_ElEMENTWISE_PARM *p_parm);

#endif  /* _VENDOR_AI_DLI_CPU_ELEMETWISE_HPP_ */
