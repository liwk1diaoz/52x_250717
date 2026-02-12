/**
	@brief Header file of vendor sde module.\n
	This file contains the functions which is related to 3A in the chip.

	@file vendor_sde.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_SDE_H_
#define _VENDOR_SDE_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include <kwrap/type.h>
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

#include "sde_alg.h"
#include "sde_api.h"
#include "sdet_api.h"
#include "sdet_api.h"

#if !defined(__FREERTOS)
#include "sde_ioctl.h"
#endif

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_sde_init(void);
extern HD_RESULT vendor_sde_uninit(void);
extern HD_RESULT vendor_sde_get_cmd(SDET_ITEM item, VOID *p_param);
extern HD_RESULT vendor_sde_set_cmd(SDET_ITEM item, VOID *p_param);
#endif


