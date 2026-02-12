/*
    USB MSDC Task ID/FLAG Install File

    The user must use Msdc_InstallID()
    to retrieve the system task ID and the flag ID for MSDC Task usage to keep the task normal working.

    @file       UsbStrgTsk_id.c
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "umsd.h"
#include "usbstrg_task.h"




void U2Msdc_InstallID(void)
{
	//OS_CONFIG_FLAG(FLG_ID_USBCLASS);
	//OS_CONFIG_TASK(USBSTRGTSK_ID,        PRI_USBSTRG,        STKSIZE_USBSTRGVND,   U2Msdc_StrgTsk);
	//OS_CONFIG_TASK(USBSTRGCACHETSK_ID,   PRI_USBSTRG_CACHE,  STKSIZE_USBSTRGCACHE, U2Msdc_CacheTsk);
}


