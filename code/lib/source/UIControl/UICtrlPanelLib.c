#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"

////////////////////////////////////////////////////////////////////////////////
//       Panel
////////////////////////////////////////////////////////////////////////////////
//gourp control would get unsel event
static INT32 UxPanel_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxPanel_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}

static INT32 UxPanel_OnUnfocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 i = 0;
	VControl *childControl ;

	while (pCtrl->ControlList[i]) {
		childControl = UxCtrl_GetChild(pCtrl, i);
		Ux_SendEvent(childControl, NVTEVT_UNFOCUS, 0);
		UxCtrl_SetChildDirty(pCtrl, i, TRUE);
		i++;
	}

	return NVTEVT_CONSUME;
}

static INT32 UxPanel_OnFocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 i = 0;
	VControl *childControl ;

	while (pCtrl->ControlList[i]) {
		childControl = UxCtrl_GetChild(pCtrl, i);
		Ux_SendEvent(childControl, NVTEVT_FOCUS, 0);
		UxCtrl_SetChildDirty(pCtrl, i, TRUE);
		i++;
	}
	return NVTEVT_CONSUME;

}

static INT32 UxPanel_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UIScreen ScreenObj = *paramArray;
	return Ux_DrawShowTable(ScreenObj, (ITEM_BASE **)pCtrl->ShowList);

}

EVENT_ENTRY DefaultPanelCmdMap[] = {
	{NVTEVT_OPEN_WINDOW,            UxPanel_OnOpen},
	{NVTEVT_CLOSE_WINDOW,           UxPanel_OnClose},
	{NVTEVT_CHILD_CLOSE,            UxPanel_OnOpen},
	{NVTEVT_UNFOCUS,                UxPanel_OnUnfocus},
	{NVTEVT_FOCUS,                  UxPanel_OnFocus},
	{NVTEVT_REDRAW,                 UxPanel_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};


VControl UxPanelCtrl = {
	"UxPanelCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultPanelCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

