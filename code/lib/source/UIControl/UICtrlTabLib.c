#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlTabLib.h"

////////////////////////////////////////////////////////////////////////////////
//      API called by users
////////////////////////////////////////////////////////////////////////////////
void UxTab_SetData(VControl *pCtrl, TAB_DATA_SET attribute, UINT32 value)
{
	CTRL_TAB_DATA *tab ;

	if (pCtrl->wType != CTRL_TAB) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	tab = ((CTRL_TAB_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case TAB_FOCUS: {
			UINT32 i = 0;
			VControl *childControl;
			tab->focus = value; //set tab focus
			for (i = 0; i < tab->total; i++) {
				childControl = UxCtrl_GetChild(pCtrl, i);
				if (i == tab->focus) {
					Ux_SendEvent(childControl, NVTEVT_FOCUS, 0);
				} else {
					Ux_SendEvent(childControl, NVTEVT_UNFOCUS, 0);
				}
				UxCtrl_SetChildDirty(pCtrl, i, TRUE);
			}
		}
		break;
#if 0
	case TAB_TOTAL:
		tab->total = value;
		break;
#endif
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxTab_GetData(VControl *pCtrl, TAB_DATA_SET attribute)
{
	CTRL_TAB_DATA *tab ;
	if (pCtrl->wType != CTRL_TAB) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	tab = ((CTRL_TAB_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case TAB_FOCUS:
		return tab->focus;
	case TAB_TOTAL:
		return tab->total;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}



////////////////////////////////////////////////////////////////////////////////
//       Default  Tab   Cmd
////////////////////////////////////////////////////////////////////////////////
static INT32 UxTab_OnUnfocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_TAB_DATA *data = (CTRL_TAB_DATA *)pCtrl->CtrlData;
	VControl *childControl = UxCtrl_GetChild(pCtrl, data->focus);

	Ux_SendEvent(childControl, NVTEVT_UNFOCUS, 0);
	UxCtrl_SetChildDirty(pCtrl, data->focus, TRUE);
	return NVTEVT_CONSUME;

}

static INT32 UxTab_OnFocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_TAB_DATA *data = (CTRL_TAB_DATA *)pCtrl->CtrlData;
	VControl *childControl = UxCtrl_GetChild(pCtrl, data->focus);

	Ux_SendEvent(childControl, NVTEVT_FOCUS, 0);
	UxCtrl_SetChildDirty(pCtrl, data->focus, TRUE);
	return NVTEVT_CONSUME;

}
#if 0 //not use now
//#NT#2008/09/22#Steven Wang -begin
//#NT#TAB component scroll to first item and previous event occurred again
static INT32 UxTab_OnFirstItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	VControl *childControl;
	CTRL_TAB_DATA *data = (CTRL_TAB_DATA *)pCtrl->CtrlData;
	if (data->total > 1) {
		childControl = UxCtrl_GetChild(pCtrl, data->focus);
		Ux_SendEvent(childControl, NVTEVT_UNFOCUS, 0);

		if (data->focus == 0) {
			data->focus = data->total - 1;
		} else {
			data->focus--;
		}

		childControl = UxCtrl_GetChild(pCtrl, data->focus);

		Ux_SendEvent(childControl, NVTEVT_FOCUS, 0);
		UxCtrl_SetChildDirty(pCtrl, data->focus, TRUE);
		return NVTEVT_CONSUME;
	} else {
		return NVTEVT_PASS;
	}
}

//#NT#TAB component scroll to last item and next event occurred again
static INT32 UxTab_OnLastItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	VControl *childControl;
	CTRL_TAB_DATA *data = (CTRL_TAB_DATA *)pCtrl->CtrlData;
	if (data->total > 1) {
		childControl = UxCtrl_GetChild(pCtrl, data->focus);
		if (pCtrl->pParent) {
			if (pCtrl->pParent->wType == CTRL_TAB) {
				Ux_SendEvent(pCtrl->pParent, NVTEVT_LAST_ITEM, 0);
				return NVTEVT_CONSUME;
			} else {
				Ux_SendEvent(childControl, NVTEVT_UNFOCUS, 0);
				UxCtrl_SetChildDirty(pCtrl, data->focus, TRUE);
			}
		}


		Ux_SendEvent(childControl, NVTEVT_UNFOCUS, 0);

		if (data->focus == data->total - 1) {
			data->focus = 0;
		} else {
			data->focus++;
		}

		childControl = UxCtrl_GetChild(pCtrl, data->focus);
		Ux_SendEvent(childControl, NVTEVT_FOCUS, 0);
		UxCtrl_SetChildDirty(pCtrl, data->focus, TRUE);
		return NVTEVT_CONSUME;
	} else {
		return NVTEVT_PASS;
	}
}
//#NT#2008/09/22#Steven Wang -end
#endif
static INT32 UxTab_OnPrevItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_TAB_DATA *tab = (CTRL_TAB_DATA *)pCtrl->CtrlData;
	VControl *childControl ;

	// set next focus unsel
	childControl = UxCtrl_GetChild(pCtrl, tab->focus);
	Ux_SendEvent(childControl, NVTEVT_UNFOCUS, 0);
	UxCtrl_SetChildDirty(pCtrl, tab->focus, TRUE);


	// set next focus
	if (tab->focus == 0) {
		tab->focus = tab->total - 1;
	} else {
		tab->focus--;
	}
	// set next focus select state
	childControl = UxCtrl_GetChild(pCtrl, tab->focus);
	Ux_SendEvent(childControl, NVTEVT_FOCUS, 0);
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}
static INT32 UxTab_OnNextItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_TAB_DATA *tab = (CTRL_TAB_DATA *)pCtrl->CtrlData;
	VControl *childControl ;

	// set next focus unsel
	childControl = UxCtrl_GetChild(pCtrl, tab->focus);
	Ux_SendEvent(childControl, NVTEVT_UNFOCUS, 0);
	UxCtrl_SetChildDirty(pCtrl, tab->focus, TRUE);

	// set next focus
	if (tab->focus == tab->total - 1) {
		tab->focus = 0;
	} else {
		tab->focus++;
	}

	childControl = UxCtrl_GetChild(pCtrl, tab->focus);
	Ux_SendEvent(childControl, NVTEVT_FOCUS, 0);
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}
static INT32 UxTab_OnPress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_TAB_DATA *tab = (CTRL_TAB_DATA *)pCtrl->CtrlData;
	VControl *childControl ;

	childControl = UxCtrl_GetChild(pCtrl, tab->focus);
	Ux_SendEvent(childControl, NVTEVT_PRESS_ITEM, 0);
	UxCtrl_SetChildDirty(pCtrl, tab->focus, TRUE);
	return NVTEVT_CONSUME;

}
static INT32 UxTab_OnChange(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_TAB_DATA *tab = (CTRL_TAB_DATA *)pCtrl->CtrlData;
	VControl *childControl ;

	childControl = UxCtrl_GetChild(pCtrl, tab->focus);
	Ux_SendEvent(childControl, NVTEVT_CHANGE_STATE, 0);
	UxCtrl_SetChildDirty(pCtrl, tab->focus, TRUE);
	return NVTEVT_CONSUME;

}



static INT32 UxTab_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UIScreen ScreenObj = *paramArray;
	return Ux_DrawShowTable(ScreenObj, (ITEM_BASE **)pCtrl->ShowList);

}


EVENT_ENTRY DefaultTabCmdMap[] = {
	{NVTEVT_PREVIOUS_ITEM,          UxTab_OnPrevItem},
	{NVTEVT_NEXT_ITEM,              UxTab_OnNextItem},
	{NVTEVT_UNFOCUS,                UxTab_OnUnfocus},
	{NVTEVT_FOCUS,                  UxTab_OnFocus},
	//{NVTEVT_FIRST_ITEM,             UxTab_OnFirstItem},
	//{NVTEVT_LAST_ITEM,              UxTab_OnLastItem},
	{NVTEVT_PRESS_ITEM,             UxTab_OnPress},
	{NVTEVT_CHANGE_STATE,           UxTab_OnChange},
	{NVTEVT_REDRAW,                 UxTab_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxTabCtrl = {
	"UxTabCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultTabCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

