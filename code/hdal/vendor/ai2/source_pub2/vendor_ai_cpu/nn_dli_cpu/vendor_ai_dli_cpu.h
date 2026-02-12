/**
	@brief Header file of definition of DLI cpu.

	@file vendor_ai_dli_cpu.h

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_DLI_CPU_H_
#define _VENDOR_AI_DLI_CPU_H_

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h" // for HD_RESULT

#include "vendor_ai_net/nn_dli.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT dli_cpu_init(NN_DLI_LAYER_PARM *p_parm);
extern HD_RESULT dli_cpu_proc(NN_DLI_LAYER_PARM *p_parm);
extern HD_RESULT dli_cpu_uninit(NN_DLI_LAYER_PARM *p_parm);

#ifdef __cplusplus
}
#endif

#endif  /* _VENDOR_AI_DLI_CPU_H_ */
