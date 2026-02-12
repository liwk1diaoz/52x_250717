#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlButtonLib.h"

////////////////////////////////////////////////////////////////////////////////
//      API called by users
////////////////////////////////////////////////////////////////////////////////

void UxButton_SetItemData(VControl *pCtrl, UINT32 index, BTNITM_DATA_SET attribute, UINT32 value)
{
	CTRL_BUTTON_DATA *button ;
	CTRL_BUTTON_ITEM_DATA *item;

	if (pCtrl->wType != CTRL_BUTTON) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}

	button = ((CTRL_BUTTON_DATA *)(pCtrl->CtrlData));

	if (index > BUTTON_ONE) {
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_OUT_RANGE_MSG);
		return ;
	}

	item = button->item[index];
	switch (attribute) {
	case BTNITM_STRID:
		item->stringID = value;
		break;
	case BTNITM_ICONID:
		item->iconID = value;
		break;
	case BTNITM_STATUS:
		item->StatusFlag = value;
		break;
	case BTNITM_EVENT:
		item->pExeEvent = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);


}

UINT32 UxButton_GetItemData(VControl *pCtrl, UINT32 index, BTNITM_DATA_SET attribute)
{
	CTRL_BUTTON_DATA *button ;
	CTRL_BUTTON_ITEM_DATA *item;

	if (pCtrl->wType != CTRL_BUTTON) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}

	button = ((CTRL_BUTTON_DATA *)(pCtrl->CtrlData));

	if (index > BUTTON_ONE) {
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_OUT_RANGE_MSG);
		return ERR_OUT_RANGE;
	}

	item = button->item[index];
	switch (attribute) {
	case BTNITM_STRID:
		return item->stringID;
	case BTNITM_ICONID:
		return item->iconID;
	case BTNITM_STATUS:
		return item->StatusFlag;
	case BTNITM_EVENT:
		return item->pExeEvent;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;

	}

}

void UxButton_SetData(VControl *pCtrl, BTN_DATA_SET attribute, UINT32 value)
{
	CTRL_BUTTON_DATA *button ;
	if (pCtrl->wType != CTRL_BUTTON) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	button = ((CTRL_BUTTON_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case BTN_STYLE:
		button->style = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxButton_GetData(VControl *pCtrl, BTN_DATA_SET attribute)
{
	CTRL_BUTTON_DATA *button ;
	if (pCtrl->wType != CTRL_BUTTON) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	button = ((CTRL_BUTTON_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case BTN_STYLE:
		return button->style;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}

static UINT32 UxButton_GetStatusShowTableIndex(UINT32 status)
{
	if ((status & STATUS_ENABLE_MASK) & STATUS_DISABLE) {
		if ((status & STATUS_FOCUS_MASK) & STATUS_FOCUS_BIT) {
			return STATUS_FOCUS_DISABLE;    //user define disable sel item show style
		} else {
			return STATUS_NORMAL_DISABLE;    //user define disable unsel item show style
		}

	} else { //enable  item
		if ((status & STATUS_FOCUS_MASK) & STATUS_FOCUS_BIT) {
			return STATUS_FOCUS;
		} else {
			return STATUS_NORMAL;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//       Defaul Button Cmd
////////////////////////////////////////////////////////////////////////////////

static INT32 UxButton_OnUnfocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_BUTTON_DATA *button = (CTRL_BUTTON_DATA *)pCtrl->CtrlData;

#if 0// Janice V1.0 button draw focus satus
	button->item[BUTTON_ONE]->StatusFlag = (button->item[BUTTON_ONE]->StatusFlag & ~STATUS_ENABLE_MASK) | STATUS_DISABLE;
#else
	button->item[BUTTON_ONE]->StatusFlag = (button->item[BUTTON_ONE]->StatusFlag & ~STATUS_FOCUS_MASK) | STATUS_NORMAL_BIT;
#endif

	//some button bg is in the parent dirty button background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	UxCtrl_SetLock(pCtrl, FALSE);

	return NVTEVT_CONSUME;
}

static INT32 UxButton_OnFocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_BUTTON_DATA *button = (CTRL_BUTTON_DATA *)pCtrl->CtrlData;

#if 0// Janice V1.0 button draw focus satus
	button->item[BUTTON_ONE]->StatusFlag = (button->item[BUTTON_ONE]->StatusFlag & ~STATUS_ENABLE_MASK) | STATUS_ENABLE;
#else
	button->item[BUTTON_ONE]->StatusFlag = (button->item[BUTTON_ONE]->StatusFlag & ~STATUS_FOCUS_MASK) | STATUS_FOCUS_BIT;
#endif

	//some button bg is in the parent dirty button background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxButton_OnPressItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_BUTTON_DATA *button = (CTRL_BUTTON_DATA *)pCtrl->CtrlData;
	CTRL_BUTTON_ITEM_DATA *item =  button->item[BUTTON_ONE];
	INT32 ret = NVTEVT_PASS;

	//Auto trigger event
	//button->action = NVTEVT_PRESS_ITEM;
	if (item->pExeEvent) {
		ret = Ux_SendEvent(0, item->pExeEvent, 0);
	}

	return ret;
}

static INT32 UxButton_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_BUTTON_DATA *button = (CTRL_BUTTON_DATA *)pCtrl->CtrlData;
	CTRL_BUTTON_ITEM_DATA *item =  button->item[BUTTON_ONE];
	UIScreen ScreenObj = *paramArray;
	UINT32 uiStatus, StatusShowTableIndex;

	ITEM_BASE     *pStatusGroup = NULL;

	//3.according to current list item status get corresponding show table
	uiStatus = item->StatusFlag;

#if 0// Janice V1.0 button draw focus satus
	if (BUTTON_ONE == button->currentItem) {
		uiStatus |= STATUS_FOCUS_BIT;
	}
#endif
	StatusShowTableIndex = UxButton_GetStatusShowTableIndex(uiStatus);

	if (StatusShowTableIndex >= STATUS_SETTIMG_MAX) {
		DBG_ERR("sta err %d\r\n", StatusShowTableIndex);
		return NVTEVT_CONSUME;
	}

	if ((UxCtrl_GetLock() == pCtrl) && (StatusShowTableIndex == STATUS_FOCUS)) {
		StatusShowTableIndex = STATUS_FOCUS_PRESS;
	}

	pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[StatusShowTableIndex];
	if ((button->style & BTN_DRAW_MASK) & BTN_DRAW_IMAGE_LIST) {
		Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, item->iconID + StatusShowTableIndex, 0);
	} else

	{
		Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, item->iconID, 0);
	}

	return NVTEVT_CONSUME;

}
static INT32 UxButton_OnChange(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;

}
static INT32 UxButton_OnPress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UxButton_OnFocus(pCtrl, paramNum, paramArray);
	UxCtrl_SetLock(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}
static INT32 UxButton_OnRelease(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UxButton_OnUnfocus(pCtrl, paramNum, paramArray);
	UxCtrl_SetLock(pCtrl, FALSE);
	return NVTEVT_CONSUME;
}
EVENT_ENTRY DefaultButtonCmdMap[] = {
	{NVTEVT_UNFOCUS,                UxButton_OnUnfocus},
	{NVTEVT_FOCUS,                  UxButton_OnFocus},
	{NVTEVT_PRESS_ITEM,             UxButton_OnPressItem},
	{NVTEVT_REDRAW,                 UxButton_OnDraw},
	{NVTEVT_CHANGE_STATE,           UxButton_OnChange},
	{NVTEVT_PRESS,                  UxButton_OnPress},
	{NVTEVT_RELEASE,                UxButton_OnRelease},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxButtonCtrl = {
	"UxButtonCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultButtonCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

