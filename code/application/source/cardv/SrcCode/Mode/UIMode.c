////////////////////////////////////////////////////////////////////////////////
#include "PrjInc.h"
#include "kwrap/debug.h"
#include "GxUSB.h"

#define UIDebug_usb    DBG_IND

void FlowMode_OnUsbUnplug(void)
{
	UIDebug_usb(("^BFlowMode_OnUsbUnplug begin\r\n"));
#if (USB_MODE==ENABLE)
	if (USB_GetIsMsdcNvt()) {
		// TODO
	} else {
		UIDebug_usb(("^B USB PlugOff,PwrOff 2\r\n"));
		System_PowerOff(SYS_POWEROFF_NORMAL);
	}
#endif
	UIDebug_usb(("^BFlowMode_OnUsbUnplug end\r\n"));
}

