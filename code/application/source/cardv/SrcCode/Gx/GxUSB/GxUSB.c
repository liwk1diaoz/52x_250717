/**
    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.

    @file       GxUSB.c
    @ingroup    mIPRJAPKeyIO

    @brief      Detect USB is plugging in or unplugged
                Detect USB is plugging in or unplugged

    @note       Nothing.

    @date       2005/12/15
*/

/** \addtogroup mIPRJAPKeyIO */
//@{

#include "kwrap/type.h"
#include "GxUSB.h"
#include "usb2dev.h"
#include "DxUSB.h"
#include "DxApi.h"

//#include "FileSysTsk.h" //abnormal
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxUsb
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>


GX_CALLBACK_PTR   g_fpUSBCB = NULL;


static BOOL bLastUSBDet     = FALSE;
static BOOL bLastUSBStatus  = FALSE;
static UINT32 uiUSBCurrType = 0;
static UINT32 g_uiUSBDevID = 0;
static void xUsbChargeCB(UINT32 uiEvent)
{
	UINT32 uiChargeCurrent = USB_CHARGE_CURRENT_2P5MA;
	switch (uiEvent) {
	case USB_CHARGE_EVENT_2P5MA:
		DBG_DUMP("^YUSB 2.5mA\r\n");
		break;
	case USB_CHARGE_EVENT_100MA:
		DBG_DUMP("^YUSB 100mA\r\n");
		uiChargeCurrent = USB_CHARGE_CURRENT_100MA;
		break;
	case USB_CHARGE_EVENT_500MA:
		DBG_DUMP("^YUSB 500mA\r\n");
		uiChargeCurrent = USB_CHARGE_CURRENT_500MA;
		break;
	case USB_CHARGE_EVENT_1P5A:
		DBG_DUMP("^YUSB 1.5A\r\n");
		uiChargeCurrent = USB_CHARGE_CURRENT_1500MA;
		break;
	default:
		break;
	}
	if (g_fpUSBCB) {
		g_fpUSBCB(USB_CB_CHARGE, uiChargeCurrent, 0);
	}
}
void GxUSB_Init(UINT32 DevID, BOOL bPowerOnSrc)
{
	g_uiUSBDevID = DevID;

	if (bPowerOnSrc) {
		bLastUSBDet     = TRUE;
		bLastUSBStatus  = FALSE;
	} else {
		bLastUSBDet     = FALSE;
		bLastUSBStatus  = FALSE;
	}
	usb2dev_set_callback(USB_CALLBACK_CHARGING_EVENT, xUsbChargeCB);

}

void GxUSB_Exit(void)
{
}


/**
  Detect USB is plugging in or unplugged

  Detect USB is plugging in or unplugged and change corresponding mode
  [KeyScan internal API]

  @param void
  @return void
*/

void GxUSB_DetConnect(void)
{
	BOOL        bCurUSBDet, bCurUSBStatus;

	bCurUSBDet = GxUSB_GetIsUSBPlug();

	DBG_MSG("^BUSB DetPlug > lastDet = %d, currDet = %d\r\n", bLastUSBDet, bCurUSBDet);

	bCurUSBStatus = (BOOL)(bCurUSBDet & bLastUSBDet);

	if (bCurUSBStatus != bLastUSBStatus) {
		DBG_MSG("^BUSB DetPlug > last = %d, curr = %d\r\n", bLastUSBStatus, bCurUSBStatus);
		GxUSB_UpdateConnectType();
		//debounce: 兩次都插入才算真正插入
		if (bCurUSBStatus == TRUE) {
#if 0
			uiUSBCurrType = GxUSB_GetConnectType();
			if (uiUSBCurrType != USB_CONNECT_UNKNOWN) {
				DBG_DUMP("^BUSB Plug\r\n");
				if (g_fpUSBCB) {
					g_fpUSBCB(USB_CB_PLUG, uiUSBCurrType, 0);
				}
			} else {
				//USB already unplugged while call GxUSB_UpdateConnectType()
				DBG_DUMP("^BUSB Fast Unplug\r\n");
				if (g_fpUSBCB) {
					g_fpUSBCB(USB_CB_UNPLUG, uiUSBCurrType, 0);
				}
				bCurUSBDet = FALSE;
				bCurUSBStatus = (BOOL)(bCurUSBDet & bLastUSBDet);
			}
#else
			if (g_fpUSBCB) {
				g_fpUSBCB(USB_CB_PLUG, uiUSBCurrType, 0);
			}
#endif
		}
		//debounce: 兩次都移除才算真正移除
		else {
			//uiUSBCurrType = USB_CONNECT_NONE;
			if (1) {
				DBG_DUMP("^BUSB Unplug\r\n");
				//Need feedback from callback
				if (g_fpUSBCB) {
					g_fpUSBCB(USB_CB_UNPLUG, 0, 0);
				}
			}
		}
	}
	//debounce: 一次移除也算真正移除
	else if ((bLastUSBDet == TRUE) && (bCurUSBDet == FALSE)) {
		//USB already unplugged
		DBG_MSG("^BUSB Fast Unplug 2\r\n");
		if (g_fpUSBCB) {
			g_fpUSBCB(USB_CB_UNPLUG, 0, 0);
		}
		bCurUSBDet = FALSE;
		bCurUSBStatus = (BOOL)(bCurUSBDet & bLastUSBDet);
	}

	bLastUSBDet     = bCurUSBDet;
	bLastUSBStatus  = bCurUSBStatus;

}

/**
  Get if usb is plug / unplug.
  It cab be used before DetectSrvTsk starts.

  @param void
  @return TRUE : plug in FALSE : unplug
*/

BOOL GxUSB_GetIsUSBPlug(void)
{
	if (g_uiUSBDevID == 0) {
		DBG_ERR("GxUSB_Init should be invoked before any other GxUSB APIs.\r\n");
		return FALSE;
	}
	if (Dx_Getcaps((DX_HANDLE)g_uiUSBDevID, DETUSB_CAPS_BASE, 0) & DETUSB_BF_DETPLUG) {
		return Dx_Getcaps((DX_HANDLE)g_uiUSBDevID, DETUSB_CAPS_PLUG, 0);    //detect current plug status
	} else {
		return FALSE;
	}
}

void GxUSB_RegCB(GX_CALLBACK_PTR fpUSBCB)
{
	g_fpUSBCB = fpUSBCB;
}

void GxUSB_UpdateConnectType(void)
{
	if (g_uiUSBDevID == 0) {
		DBG_ERR("GxUSB_Init should be invoked before any other GxUSB APIs.\r\n");
		return;
	}
	if (Dx_Getcaps((DX_HANDLE)g_uiUSBDevID, DETUSB_CAPS_BASE, 0) & DETUSB_BF_CONNTYPE) {
		uiUSBCurrType = Dx_Getcaps((DX_HANDLE)g_uiUSBDevID, DETUSB_CAPS_CONN_TYPE, 0);
	}
}

UINT32 GxUSB_GetConnectType(void)
{
	return uiUSBCurrType;
}

void GxUSB_SetChargerType(BOOL bIsStandardCharger)
{
	if (g_uiUSBDevID == 0) {
		DBG_ERR("GxUSB_Init should be invoked before any other GxUSB APIs.\r\n");
		return;
	}
	Dx_Setconfig((DX_HANDLE)g_uiUSBDevID, DETUSB_CFG_STANDARD_CHARGER, bIsStandardCharger);
}
