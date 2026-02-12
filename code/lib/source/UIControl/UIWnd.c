#include <kwrap/flag.h>
#include <kwrap/task.h>
#include "UIControl/UIControlWnd.h"
#include "UIControlID_int.h"
#include "UIWndInternal.h"
#include "NvtUser/NvtUser.h"
#include <stdarg.h>
#include <string.h>

//janice vos todo
// fake get_tid() function,alwayse is other task
static unsigned int get_tid (ID *p_tskid )
{
    *p_tskid = (ID)vos_task_get_handle();
    return 0;
}

#define MAX_CREATED_WINDOW    6
#define NO_USE_TASK_ID      0xffff

typedef struct _CREATE_WINDOW_ENTRY {
	VControl *pWnd;
	UINT8  NumOfMap;
} CREATE_WINDOW_ENTRY;

static CREATE_WINDOW_ENTRY CreatedWindowArray[MAX_CREATED_WINDOW] = {0};
static UINT8 CreatedWindowNumber = 0;
#if 0
NVTRET Ux_CloseWindowInternal(VControl *pWnd, UINT32 paramNum, UINT32 *paramArray);
NVTRET Ux_OpenWindowInternal(VControl *pWnd, UINT32 paramNum, UINT32 *paramArray);
#endif

extern ID gFrameworkRunningTskID;
extern ID gFrameworkCallingLevel;


#if (UX_POSTEVT)
NVTRET Ux_OpenWindow(VControl *pWnd, UINT32 paramNum, ...)
{
	UINT32 i;
	va_list ap;
	//NVTRET ret;
	UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];
	ID callerTskId = 0;

    DBG_IND("%s!\n\r", pWnd->Name);

	if (!Ux_IsOpen()) {
		DBG_ERR("Framework not open %s\n\r", pWnd->Name);
		return NVTRET_ERROR;
	}

	if (pWnd->wType != CTRL_WND) {
		DBG_ERR("Failed %s is not a wnd type\n\r", pWnd->Name);
		return NVTRET_ERROR;
	}

	if (paramNum) {
		if (paramNum > MAX_MESSAGE_PARAM_NUM) {
			DBG_ERR("parameter number exceeds %d. Only take %d\n\r", MAX_MESSAGE_PARAM_NUM, MAX_MESSAGE_PARAM_NUM);
            Ux_DumpStatus();
			paramNum = MAX_MESSAGE_PARAM_NUM;
		}
		va_start(ap, paramNum);
		for (i = 0; i < paramNum; i++) {
			pUIParamArray[i] = va_arg(ap, UINT32);
		}
		va_end(ap);
	}
	get_tid(&callerTskId);
	if (callerTskId != Ux_GetTaskID()) {
		DBG_IND("not by UI,post NVTEVT_OPEN_WINDOW %s!\n\r", pWnd->Name);
		if (paramNum == 0) {
			Ux_PostEvent(NVTEVT_OPEN_WINDOW, 1, pWnd);
		}
		if (paramNum == 1) {
			Ux_PostEvent(NVTEVT_OPEN_WINDOW, 2, pWnd, pUIParamArray[0]);
		}
		if (paramNum == 2) {
			Ux_PostEvent(NVTEVT_OPEN_WINDOW, 3, pWnd, pUIParamArray[0], pUIParamArray[1]);
		}
		if (paramNum == 3) {
			Ux_PostEvent(NVTEVT_OPEN_WINDOW, 4, pWnd, pUIParamArray[0], pUIParamArray[1], pUIParamArray[2]);
		}
	} else {
		DBG_IND("direct OPEN_WINDOW %s!\n\r", pWnd->Name);
		Ux_OpenWindowInternal(pWnd, paramNum, pUIParamArray);
	}

	return NVTRET_OK;
}
//New API
NVTRET Ux_OpenWindowInternal(VControl *pWnd, UINT32 paramNum, UINT32 *paramArray)
{
	if (CreatedWindowNumber >= MAX_CREATED_WINDOW) {
		DBG_ERR("exceeds MAX \n\r");
        Ux_DumpStatus();
		return NVTRET_ERROR;
	}

	CreatedWindowArray[CreatedWindowNumber].pWnd = pWnd;

#if 0
	// Check the number of event maps this window has
	i = 0;
	while (pWnd->EventList[i] != 0) {
		i++;
	}
	CreatedWindowArray[CreatedWindowNumber].NumOfMap = i;
#endif
	CreatedWindowNumber++;
	DBG_IND("goto level %d\n\r", CreatedWindowNumber);

	Ux_CreateRelation(pWnd);
	// Notify Create
	//#NT#2008/10/22@Steven wang begin -
	//#NT#Process user define evt mapping first and process default evt mapping table
	Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_OPEN_WINDOW, paramNum, paramArray);
	//#NT#2008/10/22@Steven wang end -

	// When the first window is created , set a flag to notify UI framework task
	// beginning wait event
	if (CreatedWindowNumber > 0) { //in first window may open window ,CreatedWindowNumber >1
		vos_flag_set(UICTRL_FLG_ID, FLGFRAMEWORK_WINDOW_CREATED);
	}

	return NVTRET_OK;
}
#else
NVTRET Ux_OpenWindow(VControl *pWnd, UINT32 paramNum, ...)
{
	UINT32 i;
	va_list ap;
	NVTRET ret;
	UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];
	ID callerTskId;

	if (!Ux_IsOpen()) {
		DBG_ERR("Framework not open %s\n\r", pWnd->Name);
		return NVTRET_ERROR;
	}

	if (pWnd->wType != CTRL_WND) {
		DBG_ERR("Failed %s is not a wnd type\n\r", pWnd->Name);
		return NVTRET_ERROR;
	}
	get_tid(&callerTskId);

	if (gFrameworkRunningTskID == NO_USE_TASK_ID ||
		callerTskId != gFrameworkRunningTskID) {
		vos_sem_wait(UICTRL_WND_SEM_ID);
		gFrameworkRunningTskID = callerTskId;
	}
	gFrameworkCallingLevel++;

	if (CreatedWindowNumber < MAX_CREATED_WINDOW) {
		CreatedWindowArray[CreatedWindowNumber].pWnd = pWnd;

#if 0
		// Check the number of event maps this window has
		i = 0;
		while (pWnd->EventList[i] != 0) {
			i++;
		}
		CreatedWindowArray[CreatedWindowNumber].NumOfMap = i;
#endif
		CreatedWindowNumber++;
		DBG_IND("goto level %d\n\r", CreatedWindowNumber);

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
		Ux_CreateRelation(pWnd);
		// Notify Create
		//#NT#2008/10/22@Steven wang begin -
		//#NT#Process user define evt mapping first and process default evt mapping table
		Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_OPEN_WINDOW, paramNum, pUIParamArray);
		//#NT#2008/10/22@Steven wang end -

		// When the first window is created , set a flag to notify UI framework task
		// beginning wait event
		if (CreatedWindowNumber > 0) { //in first window may open window ,CreatedWindowNumber >1
			vos_flag_set(UICTRL_FLG_ID, FLGFRAMEWORK_WINDOW_CREATED);
		}

		ret = NVTRET_OK;
	} else {
		DBG_ERR("exceeds MAX \n\r");
		ret = NVTRET_ERROR;
	}

	//auto Redraw,before sig sem
	Ux_Redraw();

	gFrameworkCallingLevel--;
	if (gFrameworkCallingLevel == 0) {
		gFrameworkRunningTskID = NO_USE_TASK_ID;
		vos_sem_sig(UICTRL_WND_SEM_ID);
	}
	return ret;
}
#endif

#define CLOSE_SEQUENCE_EVENT     ENABLE


NVTRET Ux_CloseWindowInternal(VControl *pWnd, UINT32 paramNum, UINT32 *paramArray)
{
	UINT8 wndIndex;

	if (CreatedWindowNumber) {
		if (CreatedWindowArray[CreatedWindowNumber - 1].pWnd == pWnd) {
			// Notify Close
			//#NT#2008/10/22@Steven wang begin -
			//#NT#Process user define evt mapping first and process default evt mapping table
			Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_CLOSE_WINDOW, paramNum, paramArray);
			//#NT#2008/10/22@Steven wang end
			CreatedWindowNumber--;
			CreatedWindowArray[CreatedWindowNumber].pWnd = 0;
			DBG_IND("Ux_CloseWindow(): goto level %d\n\r", CreatedWindowNumber);

			if (CreatedWindowNumber && CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
				// Notify focusing window that child is closed.
				//#NT#2008/10/22@Steven wang begin -
				//#NT#Process user define evt mapping first and process default evt mapping table
				Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_CHILD_CLOSE, paramNum, paramArray);
				//#NT#2008/10/22@Steven wang end
			} else {
				// When the last window is closed, no window created,
				// clear the flag and let UI framework task wait until
				// the flag is set, that is a window created.
				vos_flag_clr(UICTRL_FLG_ID, FLGFRAMEWORK_WINDOW_CREATED);
			}
			return NVTRET_OK;
		} else { // pWnd is not the last window
			DBG_IND("is not the last.\n\r");
			// Make sure this object is created.
			for (wndIndex = CreatedWindowNumber; wndIndex > 0; wndIndex--) {
				if (CreatedWindowArray[wndIndex - 1].pWnd == pWnd) {
					break;
				}
			}
			if (wndIndex == 0) {
				DBG_ERR("%s not created.\n\r", pWnd->Name);
				return NVTRET_ERROR;
			}
#if (CLOSE_SEQUENCE_EVENT == ENABLE)
			// Destroy derivative windows
			vos_flag_set(UICTRL_FLG_ID, FLGFRAMEWORK_FORCE_CLOSE);   // notify not draw in forced closing
			do {
				// Call close event and destroy this window
				if (CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
					//#NT#2008/10/22@Steven wang begin -
					//#NT#Process user define evt mapping first and process default evt mapping table
					Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_CLOSE_WINDOW, 0, paramArray);
					//#NT#2008/10/22@Steven wang end
				}
				CreatedWindowNumber--;
				CreatedWindowArray[CreatedWindowNumber].pWnd = 0;
				DBG_IND("goto level %d\n\r", CreatedWindowNumber);
				// Notify child is closed
				if (CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
					//#NT#2008/10/22@Steven wang begin -
					//#NT#Process user define evt mapping first and process default evt mapping table
					Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_CHILD_CLOSE, 0, paramArray);
					//#NT#2008/10/22@Steven wang end -
				}
			} while (CreatedWindowNumber > wndIndex);
			// Close last window
			vos_flag_clr(UICTRL_FLG_ID, FLGFRAMEWORK_FORCE_CLOSE);
			//#NT#2009/09/08@Janice Huang begin -
			//avoid on parent NVTEVT_CHILD_CLOSE close parent
			if (CreatedWindowNumber != 0) {
				if (CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
					//#NT#2008/10/22@Steven wang begin -
					//#NT#Process user define evt mapping first and process default evt mapping table
					Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_CLOSE_WINDOW, paramNum, paramArray);
					//#NT#2008/10/22@Steven wang end -
					CreatedWindowNumber--;
					CreatedWindowArray[CreatedWindowNumber].pWnd = 0;
					DBG_IND("goto level %d\n\r", CreatedWindowNumber);
				}
			} else {
				DBG_ERR("close double!!\n\r");
			}
			//#NT#2009/09/08@Janice Huang end -
			// Notify child is closed
			if (CreatedWindowNumber && CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
				//#NT#2008/10/22@Steven wang begin -
				//#NT#Process user define evt mapping first and process default evt mapping table
				Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_CHILD_CLOSE, paramNum, paramArray);
				//#NT#2008/10/22@Steven wang end -
			}
#else
			do {
				CreatedWindowNumber--;
				CreatedWindowArray[CreatedWindowNumber].pWnd = 0;
				DBG_IND("goto level %d\n\r", CreatedWindowNumber);
			} while (CreatedWindowNumber > wndIndex);
			if (CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
				//#NT#2008/10/22@Steven wang begin -
				//#NT#Process user define evt mapping first and process default evt mapping table
				Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_CLOSE_WINDOW, paramNum, paramArray);
				//#NT#2008/10/22@Steven wang end -
			}
			CreatedWindowNumber--;
			CreatedWindowArray[CreatedWindowNumber].pWnd = 0;
			DBG_IND("goto level %d\n\r", CreatedWindowNumber);
			// Notify child is closed
			if (CreatedWindowNumber && CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
				//#NT#2008/10/22@Steven wang begin -
				//#NT#Process user define evt mapping first and process default evt mapping table
				Ux_SendEventInternal(CreatedWindowArray[CreatedWindowNumber - 1].pWnd, NVTEVT_CHILD_CLOSE, paramNum, paramArray);
				//#NT#2008/10/22@Steven wang end -
			}
#endif

			// When the last window is closed, no window created,
			// clear the flag and let UI framework task wait until
			// the flag is set, that is a window created.
			if (CreatedWindowNumber == 0) {
				vos_flag_clr(UICTRL_FLG_ID, FLGFRAMEWORK_WINDOW_CREATED);
			}

			return NVTRET_OK;
		}
	} else {
		DBG_ERR("WindowNumber 0\n\r");
		return NVTRET_ERROR;
	}
}

void Ux_PostCloseWindowInternal(void)
{
	//#NT#2010/03/04@Janice Huang begin -
	//child window may share control,so need to create relation again
	if (CreatedWindowNumber && CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
		Ux_CreateRelation(CreatedWindowArray[CreatedWindowNumber - 1].pWnd);
	}
	//#NT#2010/03/04@Janice Huang end -
}

#if (UX_POSTEVT)
NVTRET Ux_CloseWindow(VControl *pWnd, UINT32 paramNum, ...)
{
	va_list ap;
	UINT8 i;
	//NVTRET ret;
	ID callerTskId = 0;
	UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];

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

	get_tid(&callerTskId);
	if (callerTskId != Ux_GetTaskID()) {
		//DBG_WRN("not by UI,post NVTEVT_CLOSE_WINDOW %s!\n\r", pWnd->Name);
		if (paramNum == 0) {
			Ux_PostEvent(NVTEVT_CLOSE_WINDOW, 1, pWnd);
		}
		if (paramNum == 1) {
			Ux_PostEvent(NVTEVT_CLOSE_WINDOW, 2, pWnd, pUIParamArray[0]);
		}
		if (paramNum == 2) {
			Ux_PostEvent(NVTEVT_CLOSE_WINDOW, 3, pWnd, pUIParamArray[0], pUIParamArray[1]);
		}
		if (paramNum == 3) {
			Ux_PostEvent(NVTEVT_CLOSE_WINDOW, 4, pWnd, pUIParamArray[0], pUIParamArray[1], pUIParamArray[2]);
		}
	} else {
		DBG_IND("direct CLOSE_WINDOW %s!\n\r", pWnd->Name);
		Ux_CloseWindowInternal(pWnd, paramNum, pUIParamArray);
		Ux_PostCloseWindowInternal();
	}

	return NVTRET_OK;
}
#else
NVTRET Ux_CloseWindow(VControl *pWnd, UINT32 paramNum, ...)
{
	va_list ap;
	UINT8 i;
	NVTRET ret;
	ID callerTskId;
	UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];

	get_tid(&callerTskId);

	if (gFrameworkRunningTskID == NO_USE_TASK_ID ||
		callerTskId != gFrameworkRunningTskID) {
		vos_sem_wait(UICTRL_WND_SEM_ID);
		gFrameworkRunningTskID = callerTskId;
	}
	gFrameworkCallingLevel++;

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

	ret = Ux_CloseWindowInternal(pWnd, paramNum, pUIParamArray);
	//#NT#2010/03/04@Janice Huang begin -
	//child window may share control,so need to create relation again
	if (CreatedWindowNumber && CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
		Ux_CreateRelation(CreatedWindowArray[CreatedWindowNumber - 1].pWnd);
	}
	//#NT#2010/03/04@Janice Huang end -
	gFrameworkCallingLevel--;
	if (gFrameworkCallingLevel == 0) {
		gFrameworkRunningTskID = NO_USE_TASK_ID;
		vos_sem_sig(UICTRL_WND_SEM_ID);
	}

	return ret;
}
#endif


#if (UX_POSTEVT)
NVTRET Ux_CloseWindowClear(VControl *pWnd, UINT32 paramNum, ...)
{
	va_list ap;
	UINT8 i;
	//NVTRET ret;
	ID callerTskId = 0;
	UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];

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

	get_tid(&callerTskId);
	if (callerTskId != Ux_GetTaskID()) {
		//DBG_WRN("not by UI,post NVTEVT_CLOSE_WINDOW %s!\n\r", pWnd->Name);
		if (paramNum == 0) {
			Ux_PostEvent(NVTEVT_CLOSE_WINDOW_CLEAR, 1, pWnd);
		}
		if (paramNum == 1) {
			Ux_PostEvent(NVTEVT_CLOSE_WINDOW_CLEAR, 2, pWnd, pUIParamArray[0]);
		}
		if (paramNum == 2) {
			Ux_PostEvent(NVTEVT_CLOSE_WINDOW_CLEAR, 3, pWnd, pUIParamArray[0], pUIParamArray[1]);
		}
		if (paramNum == 3) {
			Ux_PostEvent(NVTEVT_CLOSE_WINDOW_CLEAR, 4, pWnd, pUIParamArray[0], pUIParamArray[1], pUIParamArray[2]);
		}
	} else {
		INT32 result = NVTEVT_PASS;
		DBG_IND("direct CLOSE_WINDOW %s!\n\r", pWnd->Name);
		result = Ux_CloseWindowInternal(pWnd, paramNum, pUIParamArray);
		Ux_PostCloseWindowInternal();
		if (result == NVTRET_OK) {
			if (_pUIRender) {
				UINT32 ScreenObj = 0;

				ScreenObj = _pUIRender->pfn_BeginScreen();
				_pUIRender->pfn_ClearScreen(ScreenObj);
				_pUIRender->pfn_EndScreen(ScreenObj);
			}
		}
	}

	return NVTRET_OK;
}
#else
NVTRET Ux_CloseWindowClear(VControl *pWnd, UINT32 paramNum, ...)
{
	NVTRET ret;
	UINT32 ScreenObj = 0;
	va_list ap;
	UINT8 i;
	ID callerTskId =0;
	UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];

	get_tid(&callerTskId);

	if (gFrameworkRunningTskID == NO_USE_TASK_ID ||
		callerTskId != gFrameworkRunningTskID) {
		vos_sem_wait(UICTRL_WND_SEM_ID);
		gFrameworkRunningTskID = callerTskId;
	}
	gFrameworkCallingLevel++;

	if (paramNum) {
		if (paramNum > MAX_MESSAGE_PARAM_NUM) {
			DBG_ERR("CloseWindowClear: parameter number exceeds %d. Only take %d\n\r", MAX_MESSAGE_PARAM_NUM, MAX_MESSAGE_PARAM_NUM);
			paramNum = MAX_MESSAGE_PARAM_NUM;
		}
		va_start(ap, paramNum);
		for (i = 0; i < paramNum; i++) {
			pUIParamArray[i] = va_arg(ap, UINT32);
		}
		va_end(ap);
	}

	ret = Ux_CloseWindowInternal(pWnd, paramNum, pUIParamArray);
	//#NT#2010/03/04@Janice Huang begin -
	//child window may share control,so need to create relation again
	if (CreatedWindowNumber && CreatedWindowArray[CreatedWindowNumber - 1].pWnd) {
		Ux_CreateRelation(CreatedWindowArray[CreatedWindowNumber - 1].pWnd);
	}
	//#NT#2010/03/04@Janice Huang end -
	if (ret == NVTRET_OK) {
		if (_pUIRender) {
			ScreenObj = _pUIRender->pfn_BeginScreen();
			_pUIRender->pfn_ClearScreen(ScreenObj);
			_pUIRender->pfn_EndScreen(ScreenObj);
		}
	}


	gFrameworkCallingLevel--;
	if (gFrameworkCallingLevel == 0) {
		gFrameworkRunningTskID = NO_USE_TASK_ID;
		vos_sem_sig(UICTRL_WND_SEM_ID);
	}

	return ret;
}
#endif

#if 0
/**
    Set the event map index of the window

    A window can have more than one event map. This function can select a event map
    by giving the index in the list.

    @param pWnd The pointer of the window instance
    @param index The index of the selected event map in the map list
*/
void Ux_SetWindowCommandMapIndex(VControl *pWnd, UINT8 index)
{
	pWnd->MapIndex = index;
}
#endif

NVTRET Ux_GetWindowByIndex(VControl **ppWnd, UINT8 index)
{
	if (!ppWnd) {
		DBG_ERR("Null ppWnd\n\r");
		return NVTRET_ERROR;
	}
	if (index < CreatedWindowNumber) {
		*ppWnd  = CreatedWindowArray[index].pWnd;
		return NVTRET_OK;
	} else {
		DBG_ERR("wnd index Failed\n\r");
		*ppWnd = (VControl *)0;
		return NVTRET_ERROR;
	}
}
NVTRET Ux_GetFocusedWindowWithIndex(VControl **ppWnd, UINT8 *index)
{
	if (!ppWnd) {
		DBG_ERR("Null ppWnd\n\r");
		return NVTRET_ERROR;
	}
	if (CreatedWindowNumber) {
		*ppWnd = CreatedWindowArray[CreatedWindowNumber - 1].pWnd;
		*index = CreatedWindowNumber;
		return NVTRET_OK;
	} else {
		DBG_WRN("no open wnd\n\r");
		*ppWnd = (VControl *)0;
		*index = 0;
		return NVTRET_ERROR;
	}
}


NVTRET Ux_GetFocusedWindow(VControl **ppFocusedWnd)
{
	UINT8 index;
	return Ux_GetFocusedWindowWithIndex(ppFocusedWnd, &index);
}


NVTRET Ux_GetRootWindow(VControl **pRootWnd)
{
	if (!pRootWnd) {
		DBG_ERR("Null pRootWnd\n\r");
		return NVTRET_ERROR;
	}
	if (CreatedWindowNumber == 0) {
		DBG_ERR("wnd not created\n\r");
		*pRootWnd = (VControl *)0;
		return NVTRET_ERROR;
	}
	*pRootWnd = CreatedWindowArray[0].pWnd;
	return NVTRET_OK;
}


NVTRET Ux_GetParentWindow(VControl *pWnd, VControl **pParentWnd)
{
	UINT8 wndIndex;

	if (!pParentWnd || !pWnd) {
		DBG_ERR("Null pParentWnd or pWnd\n\r");
		return NVTRET_ERROR;
	}

	wndIndex = CreatedWindowNumber;
	while (wndIndex > 0 && CreatedWindowArray[wndIndex - 1].pWnd != pWnd) {
		wndIndex--;
	}
	if (wndIndex == 0) {
		DBG_IND("wnd not created");
		*pParentWnd = (VControl *)0;
		return NVTRET_ERROR;
	}
	wndIndex--;
	if (wndIndex) {
		*pParentWnd = CreatedWindowArray[wndIndex - 1].pWnd;
	} else {
		*pParentWnd = CreatedWindowArray[wndIndex].pWnd;
	}

	return NVTRET_OK;
}

INT32 Ux_CreateRelation(VControl *pCtrl)
{
	VControl **controlList = pCtrl->ControlList;
	int i = 0;
	if (!controlList) {
		return NVTEVT_PASS;
	}

	while (controlList[i] != 0) {
		controlList[i]->pParent = pCtrl;
		Ux_CreateRelation(controlList[i]);
		i++;
	}
	return NVTRET_OK;
}
BOOL Ux_IsForceCloseWindow()
{
	return (vos_flag_chk(UICTRL_FLG_ID, FLGFRAMEWORK_FORCE_CLOSE) == FLGFRAMEWORK_FORCE_CLOSE);
}

void Ux_DumpStatus(void)
{
	VControl *pCurrnetWnd;
	VControl *pRootWnd;
    UINT32    i=0;
    VControl *pWnd = NULL;

	Ux_GetFocusedWindow(&pCurrnetWnd);
	Ux_GetRootWindow(&pRootWnd);
	DBG_DUMP(" pCurrnetWnd = %s \r\n", pCurrnetWnd->Name);
	DBG_DUMP(" pRootWnd = %s \r\n", pRootWnd->Name);
    for(i=0;i<CreatedWindowNumber;i++)
    {
        if(CreatedWindowArray[i].pWnd){
    		pWnd = CreatedWindowArray[i].pWnd;
        	DBG_DUMP("index %d pWnd = %s \r\n",i, pWnd->Name);
        }

    }
}


