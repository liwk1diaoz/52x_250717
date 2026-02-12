
#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlStateLib.h"
#define STATE_NORMAL      0   //state only one state,always get normal state

////////////////////////////////////////////////////////////////////////////////
//      API called by users
////////////////////////////////////////////////////////////////////////////////
void UxState_SetItemData(VControl *pCtrl, UINT32 index, STATE_ITEM_DATA_SET attribute, UINT32 value)
{
	CTRL_STATE_DATA *state ;
	CTRL_STATE_ITEM_DATA *item;

	if (pCtrl->wType != CTRL_STATE) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}

	state = ((CTRL_STATE_DATA *)(pCtrl->CtrlData));
	if (index == CURITEM_INDEX) {
		index = state->currentItem;
	}

	if (index > state->totalItem - 1) {
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_OUT_RANGE_MSG);
		return ;
	}
	item = state->item[index];
	switch (attribute) {
	case STATE_ITEM_STRID:
		item->stringID = value;
		break;
	case STATE_ITEM_ICONID:
		item->iconID = value;
		break;
	case STATE_ITEM_EVENT:
		item->pExeEvent = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}

UINT32 UxState_GetItemData(VControl *pCtrl, UINT32 index, STATE_ITEM_DATA_SET attribute)
{
	CTRL_STATE_DATA *state ;
	CTRL_STATE_ITEM_DATA *item;

	if (pCtrl->wType != CTRL_STATE) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}

	state = ((CTRL_STATE_DATA *)(pCtrl->CtrlData));
	if (index == CURITEM_INDEX) {
		index = state->currentItem;
	}

	if (index > state->totalItem - 1) {
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_OUT_RANGE_MSG);
		return ERR_OUT_RANGE;
	}
	item = state->item[index];

	switch (attribute) {
	case STATE_ITEM_STRID:
		return item->stringID;
	case STATE_ITEM_ICONID:
		return item->iconID;
	case STATE_ITEM_EVENT:
		return item->pExeEvent;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}

void UxState_SetData(VControl *pCtrl, STATE_DATA_SET attribute, UINT32 value)
{
	CTRL_STATE_DATA *state ;

	if (pCtrl->wType != CTRL_STATE) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	state = ((CTRL_STATE_DATA *)(pCtrl->CtrlData));

	switch (attribute) {
	case STATE_CURITEM:
		state->currentItem = value;
		break;
#if 0
	case STATE_TOTITEM:
		state->totalItem = value;
		break;
#endif
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxState_GetData(VControl *pCtrl, STATE_DATA_SET attribute)
{
	CTRL_STATE_DATA *state ;

	if (pCtrl->wType != CTRL_STATE) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	state = ((CTRL_STATE_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case STATE_CURITEM:
		return state->currentItem;
	case STATE_TOTITEM:
		return state->totalItem;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}

////////////////////////////////////////////////////////////////////////////////
//       Default  State   Cmd
////////////////////////////////////////////////////////////////////////////////
static INT32 UxState_OnPrevItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uiStatus = UxState_GetData(pCtrl, STATE_CURITEM);

	DBG_IND("default UxState_OnPrevItem \r\n");
	if (uiStatus > 0) {
		UxState_SetData(pCtrl, STATE_CURITEM, uiStatus - 1);
	} else {
		UxState_SetData(pCtrl, STATE_CURITEM, UxState_GetData(pCtrl, STATE_TOTITEM) - 1);
	}

	UxCtrl_SetDirty(pCtrl,  TRUE);
	return NVTEVT_CONSUME;

}
static INT32 UxState_OnNextItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uiStatus = UxState_GetData(pCtrl, STATE_CURITEM);

	DBG_IND("default UxState_OnNextItem \r\n");
	if (uiStatus < UxState_GetData(pCtrl, STATE_TOTITEM) - 1) {
		UxState_SetData(pCtrl, STATE_CURITEM, uiStatus + 1);
	} else {
		UxState_SetData(pCtrl, STATE_CURITEM, 0);
	}

	UxCtrl_SetDirty(pCtrl,  TRUE);
	return NVTEVT_CONSUME;

}

static INT32 UxState_OnUnfocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UxState_SetData(pCtrl, STATE_CURITEM, STATUS_NORMAL);

	return NVTEVT_CONSUME;
}

static INT32 UxState_OnFocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UxState_SetData(pCtrl, STATE_CURITEM, STATUS_FOCUS);
	return NVTEVT_CONSUME;
}


//depend on status change showobj property,if showObj not assign status
//would only draw showob, not change staus
static INT32 UxState_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_STATE_DATA *state = (CTRL_STATE_DATA *)pCtrl->CtrlData;
	CTRL_STATE_ITEM_DATA *item =  NULL;
	UIScreen ScreenObj = *paramArray;
	ITEM_BASE     *pStatusGroup = NULL;

	if (state->currentItem == (UINT32) - 1) { //STATE_CURITEM is never set
		pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[STATE_NORMAL];

		Ux_DrawItemByStatus(ScreenObj, pStatusGroup, 0, 0, 0);
	} else {
		item =  state->item[state->currentItem];
		pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[STATE_NORMAL];

		Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, item->iconID, 0);
	}

	return NVTEVT_CONSUME;

}



EVENT_ENTRY DefaultStateCmdMap[] = {
	{NVTEVT_PREVIOUS_ITEM,          UxState_OnPrevItem},
	{NVTEVT_NEXT_ITEM,              UxState_OnNextItem},
	{NVTEVT_UNFOCUS,                UxState_OnUnfocus},
	{NVTEVT_FOCUS,                  UxState_OnFocus},
	{NVTEVT_CHANGE_STATE,           UxState_OnNextItem},
	{NVTEVT_REDRAW,                 UxState_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxStateCtrl = {
	"UxStateCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultStateCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

