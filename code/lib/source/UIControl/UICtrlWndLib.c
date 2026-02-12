#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UIControl.h"

////////////////////////////////////////////////////////////////////////////////
//       Wnd
////////////////////////////////////////////////////////////////////////////////
//gourp control would get unsel event
static INT32 UxWnd_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	//user can not createWnd in WndOnOpen,it would rescursive
	//only do the thing create wnd
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxWnd_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	//user can not closeWnd in WndOnClose,it would rescursive
	//only do the thing close wnd
	return NVTEVT_CONSUME;
}
static INT32 UxWnd_OnChildClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxWnd_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UIScreen ScreenObj = *paramArray;
	return Ux_DrawShowTable(ScreenObj, (ITEM_BASE **)pCtrl->ShowList);

}

EVENT_ENTRY DefaultWndCmdMap[] = {
	{NVTEVT_OPEN_WINDOW,            UxWnd_OnOpen},
	{NVTEVT_CLOSE_WINDOW,           UxWnd_OnClose},
	{NVTEVT_CHILD_CLOSE,            UxWnd_OnChildClose},
	{NVTEVT_REDRAW,                 UxWnd_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxWndCtrl = {
	"UxWndCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultWndCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};


