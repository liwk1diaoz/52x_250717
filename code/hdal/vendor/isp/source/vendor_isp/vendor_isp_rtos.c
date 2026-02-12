/**
	@brief Header file of vendor isp module.\n
	This file contains the functions which is related to IQ/3A in the chip.

	@file vendor_isp_rtos.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "vendor_isp.h"
#include "isp_rtos_inc.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define VENDOR_AF_READY  0

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern ER ispt_api_get_cmd(ISPT_ITEM item, UINT32 addr);
extern ER ispt_api_set_cmd(ISPT_ITEM item, UINT32 addr);
extern ER aet_api_get_cmd(AET_ITEM item, UINT32 addr);
extern ER aet_api_set_cmd(AET_ITEM item, UINT32 addr);
extern ER aft_api_get_cmd(AFT_ITEM item, UINT32 addr);
extern ER aft_api_set_cmd(AFT_ITEM item, UINT32 addr);
extern ER awbt_api_get_cmd(AWBT_ITEM item, UINT32 addr);
extern ER awbt_api_set_cmd(AWBT_ITEM item, UINT32 addr);
extern ER iqt_api_get_cmd(IQT_ITEM item, UINT32 addr);
extern ER iqt_api_set_cmd(IQT_ITEM item, UINT32 addr);

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
HD_RESULT vendor_isp_init(void)
{
	return HD_OK;
}

HD_RESULT vendor_isp_uninit(void)
{
	return HD_OK;
}

HD_RESULT vendor_isp_get_ae(AET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = aet_api_get_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT vendor_isp_set_ae(AET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = aet_api_set_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT vendor_isp_get_af(AFT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	#if (VENDOR_AF_READY)
	ret = aft_api_get_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}
	#endif

	return ret;
}

HD_RESULT vendor_isp_set_af(AFT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	#if (VENDOR_AF_READY)
	ret = aft_api_set_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}
	#endif

	return ret;
}

HD_RESULT vendor_isp_get_awb(AWBT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = awbt_api_get_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT vendor_isp_set_awb(AWBT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = awbt_api_set_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT vendor_isp_get_iq(IQT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = iqt_api_get_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT vendor_isp_set_iq(IQT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = iqt_api_set_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT vendor_isp_get_common(ISPT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = ispt_api_get_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT vendor_isp_set_common(ISPT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	ret = ispt_api_set_cmd(item, (UINT32)p_param);
	if (ret < 0) {
		ret = HD_ERR_NG;
	}

	return ret;
}

