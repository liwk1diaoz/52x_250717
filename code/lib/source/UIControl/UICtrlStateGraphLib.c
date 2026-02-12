#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlStateGraphLib.h"

#define STATEGRAPH_SKIN    0   //state graph first show table is skin, state from second
////////////////////////////////////////////////////////////////////////////////
//      API called by users
////////////////////////////////////////////////////////////////////////////////
void UxStateGraph_SetData(VControl *pCtrl, STATEGRAPH_DATA_SET attribute, UINT32 value)
{
	CTRL_STATEGRAPH_DATA *stategraph ;
	if (pCtrl->wType != CTRL_STATEGRAPH) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	stategraph = ((CTRL_STATEGRAPH_DATA *)(pCtrl->CtrlData));

	switch (attribute) {
	case STATEGRAPH_STATUS:
		stategraph->StatusFlag = value;
		break;
	case STATEGRAPH_EVENT:
		stategraph->pExeEvent = value;
		break;
#if 0
	case STATEGRAPH_TOTSTA:
		stategraph->totalChangeStatus = value;
		break;
#endif
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxStateGraph_GetData(VControl *pCtrl, STATEGRAPH_DATA_SET attribute)
{
	CTRL_STATEGRAPH_DATA *stategraph ;
	if (pCtrl->wType != CTRL_STATEGRAPH) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	stategraph = ((CTRL_STATEGRAPH_DATA *)(pCtrl->CtrlData));

	switch (attribute) {
	case STATEGRAPH_STATUS:
		return stategraph->StatusFlag;
	case STATEGRAPH_EVENT:
		return stategraph->pExeEvent;
	case STATEGRAPH_TOTSTA:
		return stategraph->totalChangeStatus;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}
/////////////////////////////////////////////////////////////////////////////
//        API
////////////////////////////////////////////////////////////////////////////////
#if 0 //not use now
static UINT32 UxStateGraph_GetStatusShowTableIndex(UINT32 status)
{
	if ((status & STATUS_ENABLE_MASK) & STATUS_DISABLE) {
		if ((status & STATUS_FOCUS_MASK) & STATUS_FOCUS_BIT) {
			return STATUS_FOCUS_DISABLE;    //user define disable sel item show style
		} else {
			return STATUS_NORMAL_DISABLE;    //user define disable unsel item show style
		}

	} else { //enalbe item
		if ((status & STATUS_FOCUS_MASK) & STATUS_FOCUS_BIT) {
			return STATUS_FOCUS;
		} else {
			return STATUS_NORMAL;
		}
	}
}
#endif
////////////////////////////////////////////////////////////////////////////////
//       Default StateGraph Cmd
////////////////////////////////////////////////////////////////////////////////
static INT32 UxStateGraph_OnPrevItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uiStatus = UxStateGraph_GetData(pCtrl, STATEGRAPH_STATUS);
	DBG_IND("default UxStateGraph_OnPrevItem \r\n");
	if (uiStatus > 0) {
		UxStateGraph_SetData(pCtrl, STATEGRAPH_STATUS, uiStatus - 1);
	} else {
		UxStateGraph_SetData(pCtrl, STATEGRAPH_STATUS, UxStateGraph_GetData(pCtrl, STATEGRAPH_TOTSTA) - 1);
	}

	UxCtrl_SetDirty(pCtrl,  TRUE);
	return NVTEVT_CONSUME;

}

static INT32 UxStateGraph_OnNextItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uiStatus = UxStateGraph_GetData(pCtrl, STATEGRAPH_STATUS);
	DBG_IND("default UxStateGraph_OnNextItem \r\n");
	if (uiStatus < UxStateGraph_GetData(pCtrl, STATEGRAPH_TOTSTA) - 1) {
		UxStateGraph_SetData(pCtrl, STATEGRAPH_STATUS, uiStatus + 1);
	} else {
		UxStateGraph_SetData(pCtrl, STATEGRAPH_STATUS, 0);
	}

	UxCtrl_SetDirty(pCtrl,  TRUE);
	return NVTEVT_CONSUME;

}

static INT32 UxStateGraph_OnUnfocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_STATEGRAPH_DATA   *stateGraphData;
	stateGraphData = (CTRL_STATEGRAPH_DATA *)pCtrl->CtrlData;

	stateGraphData->StatusFlag = STATUS_NORMAL;
	return NVTEVT_CONSUME;
}

static INT32 UxStateGraph_OnFocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_STATEGRAPH_DATA   *stateGraphData;
	stateGraphData = (CTRL_STATEGRAPH_DATA *)pCtrl->CtrlData;

	stateGraphData->StatusFlag = STATUS_FOCUS;
	return NVTEVT_CONSUME;
}
// stateGraph on press would send event to currnet event table to search exe function
static INT32 UxStateGraph_OnPress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_STATEGRAPH_DATA *stateGraph = (CTRL_STATEGRAPH_DATA *)pCtrl->CtrlData;
	VControl *pCurrnetWnd;

	Ux_GetFocusedWindow(&pCurrnetWnd);

	if (stateGraph->pExeEvent) {
		return Ux_SendEvent(pCurrnetWnd, stateGraph->pExeEvent, 0);
	} else {
		return NVTEVT_CONSUME;
	}

}

static INT32 UxStateGraph_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_STATEGRAPH_DATA *stateGraph = (CTRL_STATEGRAPH_DATA *)pCtrl->CtrlData;
	UIScreen ScreenObj = *paramArray;
	ITEM_BASE     *pSkinGroup = NULL;
	ITEM_BASE     *pStatusGroup = NULL;

	//first group is skin
	pSkinGroup = (ITEM_BASE *)pCtrl->ShowList[STATEGRAPH_SKIN];
	Ux_DrawShowObj(ScreenObj, (ITEM_BASE *)pSkinGroup);

	//shoud add skin group ,so +1
	pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[stateGraph->StatusFlag + 1];
	Ux_DrawItemByStatus(ScreenObj, pStatusGroup, 0, 0, 0);

	return NVTEVT_CONSUME;

}



EVENT_ENTRY DefaultStateGraphCmdMap[] = {
	{NVTEVT_PREVIOUS_ITEM,          UxStateGraph_OnPrevItem},
	{NVTEVT_NEXT_ITEM,              UxStateGraph_OnNextItem},
	{NVTEVT_UNFOCUS,                UxStateGraph_OnUnfocus},
	{NVTEVT_FOCUS,                  UxStateGraph_OnFocus},
	{NVTEVT_PRESS_ITEM,             UxStateGraph_OnPress},
	{NVTEVT_CHANGE_STATE,           UxStateGraph_OnNextItem},
	{NVTEVT_REDRAW,                 UxStateGraph_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxStateGraphCtrl = {
	"UxStateGraphCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultStateGraphCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

