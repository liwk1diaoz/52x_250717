#include "NvtUser/NVTEvent.h"
#include "NvtUser/NVTReturn.h"
#include "VControl/VControl.h"
#include "VControl_int.h"
#include <stdarg.h>


/**
    Send evnet to specific control

    This API would send  to user define event table first,and then sent to dufault lib.

    @param pWnd The pointer of the window instance
    @param paramNum How many parameters given to the created window. Acceptable value:0~3
    @param ... Variable number arguments according to paramNum
*/
INT32 Ux_SendEvent(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, ...)
{
	UINT32 i;
	va_list ap;
	UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];
	INT32 result = NVTEVT_PASS;

	DBG_FUNC_BEGIN("\r\n");
	DBG_IND("^GUx_SendEvent: %08x!\r\n", evt);

	if (paramNum) {
		if (paramNum > MAX_MESSAGE_PARAM_NUM) {
			DBG_ERR("parameter number exceeds %d. Only take %d\n\r", MAX_MESSAGE_PARAM_NUM, MAX_MESSAGE_PARAM_NUM);
			paramNum = MAX_MESSAGE_PARAM_NUM;
		}
		va_start(ap, paramNum);
		for (i = 0; i < paramNum; i++) {
			pUIParamArray[i] = va_arg(ap, UINT32);
		}
		va_end(ap);
	}

	if (!pCtrl) {
		pCtrl = Ux_GetActiveApp();
	}

	//if current Ctrl is a UI Ctrl, send to its related APP first
	if (Ux_IsUIControl(pCtrl)) {
		VControl *pAppCtrl = NULL;
		pAppCtrl = Ux_GetActiveApp();

		//1.SEND TO APP
		if ((IS_APP_EVENT(evt)) && pAppCtrl) {
			//UX_MSG(("MSG: Send %08x to App:%s\r\n", evt, pAppCtrl->Name));
			result = Ux_SendEventInternal(pAppCtrl, evt, paramNum, pUIParamArray);
			if (result == NVTEVT_CONSUME) {
				return result;
			}
		}
	}
	//2.SEND TO CTRL
	{
		//UX_MSG(("MSG: Send %08x to Ctrl:%s\r\n", evt, pCtrl->Name));
		result = Ux_SendEventInternal(pCtrl, evt, paramNum, pUIParamArray);
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////
// Internel APIs
////////////////////////////////////////////////////////////////////////////////////////

static NVTEVT_EVENT_FUNC_PTR Ux_FindEvent(EVENT_ENTRY *CmdMap, NVTEVT evt)
{
	NVTEVT_EVENT_FUNC_PTR pFunc = 0;
	register int i = 0;
	if (!CmdMap) {
		return 0;
	}
	//seek event handler in type
	while ((CmdMap[i].event != NVTEVT_NULL) && !pFunc) {
		if (evt == CmdMap[i].event) {
			pFunc = CmdMap[i].pfn;
		}
		i++;
	}
	return pFunc;
}

///////////////////////////////////////////////////////////////////////////////////////
// Common APIs
////////////////////////////////////////////////////////////////////////////////////////

/**
    Send evnet to specific control

    This API would send  to user define event table first,and then sent to dufault lib.
    paramArray is taken from variable number arguments "..."

    @param pWnd The pointer of the window instance
    @param paramNum How many parameters given to the created window. Acceptable value:0~3
    @param paramArray Variable number arguments according to paramNum
*/
INT32 Ux_SendEventInternal(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	EVENT_ENTRY *CmdMap = 0;
	NVTEVT_EVENT_FUNC_PTR pFunc = 0;
	VControl *pLocalCtrl = pCtrl;

	if (!pLocalCtrl) {
		DBG_ERR("null control  !! \r\n");
		return NVTEVT_PASS;
	}

	//find event handler in event-table of current Ctrl
	//if no match, try to find again in event-table of its base-class Ctrl
	while (pLocalCtrl && !pFunc) {
		//change to base control event table
		CmdMap = pLocalCtrl->EventList;
		//seek event handler in type
		pFunc = Ux_FindEvent(CmdMap, evt);
		if (pLocalCtrl && !pFunc) {
			//change type to its base type
			pLocalCtrl = Ux_GetBaseType(pLocalCtrl);
		}
	}

	//if we find matched event handler, execute it by this event
	if (pFunc) {
		if (paramNum) {
			return ((*pFunc)(pCtrl, paramNum, paramArray));
		} else {
			return ((*pFunc)(pCtrl, 0, 0));
		}
	}
	//cannot find any match event handler
	else {
		DBG_MSG("drop event %d !! \r\n", evt);
	}
	return NVTEVT_PASS;
}

///////////////////////////////////////////////////////////////////////////////////////
// Common APIs
////////////////////////////////////////////////////////////////////////////////////////

/**
    Send evnet to default lib directly

    Send to to dufault lib ,paramArray is get from variable number arguments "..."

    @param pWnd The pointer of the window instance
    @param paramNum How many parameters given to the created window. Acceptable value:0~3
    @param paramArray Variable number arguments according to paramNum
*/
INT32 Ux_DefaultEvent(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	EVENT_ENTRY *CmdMap = 0;
	NVTEVT_EVENT_FUNC_PTR pFunc = 0;
	VControl *pLocalCtrl = pCtrl;

	if (!pLocalCtrl) {
		DBG_ERR("null control  !! \r\n");
		return NVTEVT_CONSUME;
	}
	///////////////////////////////////////////////////////////////////
	//change type to its base type
	pLocalCtrl = Ux_GetBaseType(pLocalCtrl);
	///////////////////////////////////////////////////////////////////

	//find event handler in event-table of current Ctrl
	//if no match, try to find again in event-table of its base-class Ctrl
	while (pLocalCtrl && !pFunc) {
		//change to base control event table
		CmdMap = pLocalCtrl->EventList;
		//seek event handler in type
		pFunc = Ux_FindEvent(CmdMap, evt);
		if (pLocalCtrl && !pFunc) {
			//change type to its base type
			pLocalCtrl = Ux_GetBaseType(pLocalCtrl);
		}
	}

	//if we find matched event handler, execute it by this event
	if (pFunc) {
		if (paramNum) {
			return ((*pFunc)(pCtrl, paramNum, paramArray));
		} else {
			return ((*pFunc)(pCtrl, 0, 0));
		}
	}
	//cannot find any match event handler
	else {
		DBG_MSG("drop event %d !! \r\n", evt);
	}
	return NVTEVT_CONSUME;

}

