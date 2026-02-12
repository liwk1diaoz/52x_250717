/**
	@brief Header file of vendor sde module.\n
	This file contains the functions which is related to IQ/3A in the chip.

	@file vendor_sde_rtos.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "vendor_sde.h"
//#include "sde_rtos_inc.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern ER sdet_api_get_cmd(SDET_ITEM item, UINT32 addr);
extern ER sdet_api_set_cmd(SDET_ITEM item, UINT32 addr);

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_sde_init(void)
{
	return HD_OK;
}

HD_RESULT vendor_sde_uninit(void)
{
	return HD_OK;
}

HD_RESULT vendor_sde_get_cmd(SDET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = sdet_api_get_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT vendor_sde_set_cmd(SDET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = sdet_api_set_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}


