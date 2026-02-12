////////////////////////////////////////////////////////////////////////////////
#include "PrjInc.h"
#include "kwrap/debug.h"
////////////////////////////////////////////////////////////////////////////////

int PRIMARY_MODE_USBPCC = -1;     ///< USB PC camera

void ModeUsbCam_Open(void);
void ModeUsbCam_Close(void);

void ModeUsbCam_Open(void)
{
#if (USB_MODE == ENABLE)
    
	Input_ResetMask();
	USB_PlugInSetFunc();
	Ux_SetActiveApp(&CustomUSBPCCObjCtrl);
	Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);
    #if defined(_UI_STYLE_LVGL_)
	static UINT32 usbmodepcc = UIFlowWndUSB_PCC_MODE;
    lv_plugin_scr_open(UIFlowUSB, &usbmodepcc);
    #else
	Ux_OpenWindow((VControl *)(&UIFlowWndUSBCtrl), 1, UIFlowWndUSB_PCC_MODE);
    #endif
#endif
}
void ModeUsbCam_Close(void)
{
#if (USB_MODE == ENABLE)
	Ux_SendEvent(0, NVTEVT_EXE_CLOSE, 0);
#endif
}

SYS_MODE gModeUsbCam = {
	"USBCAM",
	ModeUsbCam_Open,
	ModeUsbCam_Close,
	NULL,
	NULL,
	NULL,
	NULL
};
