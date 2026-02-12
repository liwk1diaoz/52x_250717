////////////////////////////////////////////////////////////////////////////////
#include "PrjInc.h"
////////////////////////////////////////////////////////////////////////////////

int PRIMARY_MODE_USBSIDC = -1;    ///< USB SIDC/PTP

void ModeUsbPTP_Open(void);
void ModeUsbPTP_Close(void);

void ModeUsbPTP_Open(void)
{
#if _TODO
	INT32 retV;

	Input_ResetMask();
	USB_PlugInSetFunc();
	Ux_SetActiveApp(&CustomPrintObjCtrl);
	retV = Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);
	if (retV != E_TMOUT) {
	} else {
		debug_msg("PTPOpen:%d\r\n", retV);
	}
#endif
}
void ModeUsbPTP_Close(void)
{
#if _TODO
	Ux_SendEvent(0, NVTEVT_EXE_CLOSE, 0);//Ux_GetActiveApp()
#endif
}

SYS_MODE gModeUsbPTP = {
	"USBPTP",
	ModeUsbPTP_Open,
	ModeUsbPTP_Close,
	NULL,
	NULL,
	NULL,
	NULL
};
