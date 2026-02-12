/**
	@brief Header file of vendor vpe module.\n
	This file contains the functions which is related to 3A in the chip.

	@file vendor_vpe.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_VPE_H_
#define _VENDOR_VPE_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
#define CFG_NAME_LENGTH  256
#define DTSI_NAME_LENGTH  256

#include "vpe_alg.h"
#include "vpe_api.h"
#include "vpet_api.h"
#include "vpet_api.h"

#if !defined(__FREERTOS)
#include "vpe_ioctl.h"
#endif

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_vpe_init(void);
extern HD_RESULT vendor_vpe_uninit(void);
extern HD_RESULT vendor_vpe_get_cmd(VPET_ITEM item, VOID *p_param);
extern HD_RESULT vendor_vpe_set_cmd(VPET_ITEM item, VOID *p_param);
#endif


