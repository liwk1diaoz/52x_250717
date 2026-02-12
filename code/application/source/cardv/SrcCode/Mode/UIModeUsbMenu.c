////////////////////////////////////////////////////////////////////////////////
#include "PrjInc.h"
#include <kwrap/debug.h>
#include "kflow_common/nvtmpp.h"
////////////////////////////////////////////////////////////////////////////////

int PRIMARY_MODE_USBMENU = -1;

void ModeUsbMenu_Open(void);
void ModeUsbMenu_Close(void);

void ModeUsbMenu_Open(void)
{
	Input_ResetMask();
    #if defined(_UI_STYLE_LVGL_)
    lv_plugin_scr_open(UIFlowUSBMenu, NULL);
    #else
	Ux_OpenWindow(&UIMenuWndUSBCtrl, 0);
    #endif
}
void ModeUsbMenu_Close(void)
{
    #if defined(_UI_STYLE_LVGL_)
    lv_plugin_scr_close(UIFlowUSBMenu, NULL);
    #else
	Ux_CloseWindow(&UIMenuWndUSBCtrl, 0);
    #endif
}

SYS_MODE gModeUsbMenu = {
	"USBMenu",
	ModeUsbMenu_Open,
	ModeUsbMenu_Close,
	NULL,
	NULL,
	NULL,
	NULL
};
