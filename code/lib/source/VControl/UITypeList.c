
#include "NvtUser/NVTEvent.h"
#include "VControl/VControl.h"
#include "VControl_int.h"
#include <stdarg.h>

///////////////////////////////////////////////////////////////////////////////////////
// Type APIs
////////////////////////////////////////////////////////////////////////////////////////

VControl **gUITypeList = NULL;
INT32 gUITypeCount = 0;

void Ux_SetUITypeList(VControl **pUITable, INT32 size)
{
	gUITypeList = pUITable;
	gUITypeCount = size;
}
VControl **Ux_GetUITypeList(void)
{
	return gUITypeList;
}
BOOL Ux_IsUIControl(VControl *pCtrl)
{
	if (!pCtrl) {
		return FALSE;
	}
	if ((pCtrl->wType >= CTRL_TYPE_MIN) && (pCtrl->wType < (CTRL_TYPE_MIN + gUITypeCount))) {
		return TRUE;
	}
	return FALSE;
}

VControl **gAppTypeList = NULL;
INT32 gAppTypeCount = 0;

void Ux_SetAppTypeList(VControl **pAppTable, INT32 size)
{
	gAppTypeList = pAppTable;
	gAppTypeCount = size;
}
VControl **Ux_GetAppTypeList(void)
{
	return gAppTypeList;
}
BOOL Ux_IsAppControl(VControl *pCtrl)
{
	if (!pCtrl) {
		return FALSE;
	}
	if ((pCtrl->wType >= APP_TYPE_MIN) && (pCtrl->wType < (APP_TYPE_MIN + gAppTypeCount))) {
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
// Active APIs
////////////////////////////////////////////////////////////////////////////////////////
VControl *gActiveApp = NULL;

void Ux_SetActiveApp(VControl *pApp)
{
	gActiveApp = pApp;
}
VControl *Ux_GetActiveApp(void)
{
	return gActiveApp;
}



///////////////////////////////////////////////////////////////////////////////////////
// Common APIs
////////////////////////////////////////////////////////////////////////////////////////

VControl *Ux_GetBaseType(VControl *pLocalCtrl)
{
	if (!pLocalCtrl) {
		DBG_ERR("null control  !! \r\n");
		return 0;
	}

	//no base control
	if (pLocalCtrl->wType == 0) {
		return 0;
	}

	if (Ux_IsUIControl(pLocalCtrl)) {
		VControl **table = Ux_GetUITypeList();
		if (!table) {
			return 0;
		}
		//change type to its base type
		return table[pLocalCtrl->wType - CTRL_TYPE_MIN];
	} else if (Ux_IsAppControl(pLocalCtrl)) {
		VControl **table = Ux_GetAppTypeList();
		if (!table) {
			return 0;
		}
		//change type to its base type
		return table[pLocalCtrl->wType - APP_TYPE_MIN];
	} else {
		DBG_ERR("unknown type %d!! \r\n", pLocalCtrl->wType);
	}
	return 0;
}


