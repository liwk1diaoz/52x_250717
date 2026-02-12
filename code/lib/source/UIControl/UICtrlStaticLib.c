
#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlStaticLib.h"
#define STATIC_NORMAL      0   //state only one state,always get normal state

////////////////////////////////////////////////////////////////////////////////
//      API called by users
/////////////////////h///////////////////////////////////////////////////////////

void UxStatic_SetData(VControl *pCtrl, STATIC_DATA_SET attribute, UINT32 value)
{
	CTRL_STATIC_DATA *staticCtrl ;
	if (pCtrl->wType != CTRL_STATIC) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	staticCtrl = ((CTRL_STATIC_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case STATIC_VALUE:
		staticCtrl->value = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxStatic_GetData(VControl *pCtrl, STATIC_DATA_SET attribute)
{
	CTRL_STATIC_DATA *staticCtrl ;
	if (pCtrl->wType != CTRL_STATIC) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	staticCtrl = ((CTRL_STATIC_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case STATIC_VALUE:
		return staticCtrl->value;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}

////////////////////////////////////////////////////////////////////////////////
//       Default  Static   Cmd
////////////////////////////////////////////////////////////////////////////////

//depend on status change showobj property,if showObj not assign status
//would only draw showob, not change staus
static INT32 UxStatic_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_STATIC_DATA *Static = (CTRL_STATIC_DATA *)pCtrl->CtrlData;
	UIScreen ScreenObj = *paramArray;
	ITEM_BASE     *pStatusGroup = NULL;

	pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[STATIC_NORMAL];
	Ux_DrawItemByStatus(ScreenObj, pStatusGroup, Static->value, Static->value, Static->value);

	return NVTEVT_CONSUME;

}



EVENT_ENTRY DefaultStaticCmdMap[] = {
	{NVTEVT_REDRAW,                 UxStatic_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxStaticCtrl = {
	"UxStaticCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultStaticCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

