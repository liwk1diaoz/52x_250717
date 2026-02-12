////////////////////////////////////////////////////////////////////////////////
#include "PrjInc.h"
#include <kwrap/debug.h>
#include "kflow_common/nvtmpp.h"
////////////////////////////////////////////////////////////////////////////////

int PRIMARY_MODE_USBMSDC = -1;    ///< USB MSDC

void ModeUsbDisk_Open(void);
void ModeUsbDisk_Close(void);

void ModeUsbDisk_Open(void)
{
#if (USB_MODE == ENABLE)
	INT32 retV;


	Input_ResetMask();
	USB_PlugInSetFunc();
	Ux_SetActiveApp(&CustomMSDCObjCtrl);
	retV = Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);
	if (retV != E_TMOUT) {
        #if defined(_UI_STYLE_LVGL_)
		static UINT32 usbmodetmp = UIFlowWndUSB_MSDC_MODE;
        lv_plugin_scr_open(UIFlowUSB, &usbmodetmp);
        #else
		Ux_OpenWindow((VControl *)(&UIFlowWndUSBCtrl), 1, UIFlowWndUSB_MSDC_MODE);
        #endif
	} else {
		DBG_IND("MSDCOpen TO:%d\r\n", retV);
	}
#endif
}

void ModeUsbDisk_Close(void)
{
#if (USB_MODE == ENABLE)
    #if defined(_UI_STYLE_LVGL_)
    lv_plugin_scr_close(UIFlowUSB, NULL);
    #else
	Ux_CloseWindow((VControl *)(&UIFlowWndUSBCtrl), 0);
    #endif
	Ux_SendEvent(0, NVTEVT_EXE_CLOSE, 0);
#endif
}

SYS_MODE gModeUsbDisk = {
	"USBDISK",
	ModeUsbDisk_Open,
	ModeUsbDisk_Close,
	NULL,
	NULL,
	NULL,
	NULL
};

