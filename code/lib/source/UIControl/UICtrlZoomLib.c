
#include "UIControlInternal.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UIControlWnd.h"
#include "UIControl/UIControl.h"
#include "UIControl/UICtrlZoomLib.h"

#define ZOOM_NORMAL      0   //state only one state,always get normal state

////////////////////////////////////////////////////////////////////////////////
//      API called by users
/////////////////////h///////////////////////////////////////////////////////////
void UxZoom_SetPosition(VControl *pCtrl, ZOOM_POS_ATTRIBUTE_SET attribute, Ux_RECT *pRect)
{
	ITEM_BASE  **showTable;
	ITEM_BASE  *pStatusGroup;

	if (pCtrl->wType != CTRL_ZOOM) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[STATUS_NORMAL];
	showTable = ((ITEM_GROUP *)pStatusGroup)->ShowTable;

	switch (attribute) {
	case ZOOM_EXTERNAL_RECT:
	case ZOOM_INTERNAL_RECT:
		Ux_SetShowObjPos(showTable[attribute], pRect->x1, pRect->y1, pRect->x2, pRect->y2);
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
void UxZoom_SetData(VControl *pCtrl, ZOOM_DATA_SET attribute, UINT32 value)
{
	CTRL_ZOOM_DATA *zoomCtrl ;
	if (pCtrl->wType != CTRL_ZOOM) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	zoomCtrl = ((CTRL_ZOOM_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case ZOOM_VALUE:
		zoomCtrl->value = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxZoom_GetData(VControl *pCtrl, ZOOM_DATA_SET attribute)
{
	CTRL_ZOOM_DATA *zoomCtrl ;
	if (pCtrl->wType != CTRL_ZOOM) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	zoomCtrl = ((CTRL_ZOOM_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case ZOOM_VALUE:
		return zoomCtrl->value;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}
}

////////////////////////////////////////////////////////////////////////////////
//       Default  Zoom   Cmd
////////////////////////////////////////////////////////////////////////////////

//depend on status change showobj property,if showObj not assign status
//would only draw showob, not change staus
static INT32 UxZoom_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_ZOOM_DATA *Zoom = (CTRL_ZOOM_DATA *)pCtrl->CtrlData;
	UIScreen ScreenObj = *paramArray;
	ITEM_BASE     *pStatusGroup = NULL;

	pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[ZOOM_NORMAL];
	Ux_DrawItemByStatus(ScreenObj, pStatusGroup, Zoom->value, Zoom->value, Zoom->value);

	return NVTEVT_CONSUME;

}



EVENT_ENTRY DefaultZoomCmdMap[] = {
	{NVTEVT_REDRAW,                 UxZoom_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxZoomCtrl = {
	"UxZoomCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultZoomCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

