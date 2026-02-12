#include "UIControl/UIControlWnd.h"
#include "UIWndInternal.h"
#include <stdarg.h>
#include "UIControl/UIControlExt.h" //for CTRL_EX_TYPE

#define CTRL_FOUND  1
#define CTRL_NOT_FOUND 0



static INT32 Ux_WndEventProc_LockedCtrl(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	VControl **controlList;
	BOOL result = CTRL_NOT_FOUND ;
	if (pCtrl == UxCtrl_GetLock()) {
		Ux_SendEventInternal(UxCtrl_GetLock(), evt, paramNum, paramArray);
		result = CTRL_FOUND;
	} else if ((pCtrl->wType == CTRL_WND) || (pCtrl->wType == CTRL_PANEL) || (pCtrl->wType == CTRL_TAB)) {
		//recursive send to  child control
		controlList = pCtrl->ControlList;
		if (controlList) {
			int i = 0;
			//control rect is relative to parent control,
			//so we need to calculate event relative position
			paramArray[0] = paramArray[0] - pCtrl->rect.x1;
			paramArray[1] = paramArray[1] - pCtrl->rect.y1;
			while (controlList[i] != 0) {
				result = Ux_WndEventProc_LockedCtrl(controlList[i], evt, paramNum, paramArray);
				i++;
			}
			//if not fond control,need to add rect back ,for some control overlay
			if (result == CTRL_NOT_FOUND) {
				paramArray[0] = paramArray[0] + pCtrl->rect.x1;
				paramArray[1] = paramArray[1] + pCtrl->rect.y1;
			}
		}
	}
	return result;
}

static INT32 Ux_WndEventProc_ByTouchPos(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 eventPosX = paramArray[0];// x is relative to parent x
	INT32 eventPosY = paramArray[1];// y is relative to parent y
	VControl **controlList;
	Ux_RECT SearchRange ;
	BOOL result = CTRL_NOT_FOUND ;

	//hide control ,not recesive TP event
	if (!UxCtrl_IsShow(pCtrl)) {
		return result;
	}


	//disable control ,not recesive TP event
	if (pCtrl->wType == CTRL_BUTTON) {
		if (UxButton_GetItemData(pCtrl, BUTTON_ONE, BTNITM_STATUS) == STATUS_DISABLE) {
			return result;
		}
	}

	//get some special control rect range
	if (pCtrl->wType == CTRL_MENU) {
		UxMenu_GetRange(pCtrl, &SearchRange);
	} else if (pCtrl->wType == CTRL_LIST) {
		UxList_GetRange(pCtrl, &SearchRange);
	} else {
		SearchRange = pCtrl->rect ;
	}
	if ((eventPosX >= SearchRange.x1) && (eventPosX <= SearchRange.x2) &&
		(eventPosY >= SearchRange.y1) && (eventPosY <= SearchRange.y2)) {
		DBG_IND("###in %s \r\n ", pCtrl->Name);
		//in TP tab control is the same panel
		if ((pCtrl->wType == CTRL_WND) || (pCtrl->wType == CTRL_PANEL) || (pCtrl->wType == CTRL_TAB)) {
			//recursive send to  child control
			controlList = pCtrl->ControlList;
			if (controlList) {
				int totalChild = 0;
				int i = 0;
				//find total child
				while (controlList[totalChild] != 0) {
					totalChild++;
				}
				//control rect is relative to parent control,
				//so we need to caluate event relative position
				paramArray[0] = paramArray[0] - pCtrl->rect.x1;
				paramArray[1] = paramArray[1] - pCtrl->rect.y1;
				DBG_IND("totalChild %d \r\n ", totalChild);
				//sent TP event to all child from top child
				for (i = totalChild - 1; i >= 0; i--) {
					result = Ux_WndEventProc_ByTouchPos(controlList[i], evt, paramNum, paramArray);
					if (result == CTRL_FOUND) {
						break;
					}
				}
				//if not fond control,need to add rect back ,for some control overlay
				if (result == CTRL_NOT_FOUND) {
					paramArray[0] = paramArray[0] + pCtrl->rect.x1;
					paramArray[1] = paramArray[1] + pCtrl->rect.y1;
				}
			}

			if (result == CTRL_NOT_FOUND) {
				//all child not in range,so tab,window,panel recesive
				INT32 evtResult = Ux_SendEventInternal(pCtrl, evt, paramNum, paramArray);
				if (evtResult == NVTEVT_CONSUME) {
					result = CTRL_FOUND;
				} else {
					result = CTRL_NOT_FOUND;
				}

			}
		} else {
			INT32 evtResult = Ux_SendEventInternal(pCtrl, evt, paramNum, paramArray);
			if (evtResult == NVTEVT_CONSUME) {
				result = CTRL_FOUND;
			} else {
				result = CTRL_NOT_FOUND;
			}
		}
	}
	DBG_IND("### %s event CTRL_FOUND %d ###\r\n", pCtrl->Name, result);
	return result;
}
static INT32 Ux_WndEventProc_TouchEvent(VControl *pWnd, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	if (UxCtrl_GetLock()) {
		//send to locked control of current wnd
		Ux_WndEventProc_LockedCtrl(pWnd, evt, paramNum, paramArray);
	} else {
#if 1
		VControl *pCurWnd = pWnd;
		//send to current wnd ans its control (by rect range)
		while (pCurWnd) {
			if (Ux_WndEventProc_ByTouchPos(pCurWnd, evt, paramNum, paramArray) == CTRL_NOT_FOUND) {
				//only send to cur wnd, do not need to send to default evt,because default evt would consume event
				if (Ux_SendEventInternal(pCurWnd, NVTEVT_OUT_RANGE, paramNum, paramArray) == NVTEVT_PASS) {
					//if pass event and cur wind be closed,send evt to new window
					VControl *pNewWnd = 0;
					//send to its parent wnd
					Ux_GetFocusedWindow(&pNewWnd);
					if (pCurWnd != pNewWnd) {
						pCurWnd = pNewWnd;
					} else {
						pCurWnd = 0;
					}
				} else {
					pCurWnd = 0;
				}
			} else {
				pCurWnd = 0;
			}
		}
#else
		//send to current wnd ans its control
		Ux_CtrlEventProcByPos(pWnd, evt, paramNum, paramArray);
#endif
	}
	return NVTEVT_CONSUME;
}

static INT32 Ux_WndEventProc_KeyEvent(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 result = NVTEVT_PASS;
	VControl **controlList;

	if ((pCtrl->wType == CTRL_WND) || (pCtrl->wType == CTRL_PANEL)) {
		//recursive send to all child control
		controlList = pCtrl->ControlList;
		if (controlList) {
			int i = 0;
			while (controlList[i] != 0) {
				result = Ux_WndEventProc_KeyEvent(controlList[i], evt, paramNum, paramArray);
				i++;
			}
		}
		//send to self
		result = Ux_SendEventInternal(pCtrl, evt, paramNum, paramArray);
	} else if (pCtrl->wType == CTRL_TAB) {
		//send event from  leaf control to current tab control
		VControl *leafCtrl = UxCtrl_GetLeafFocus(pCtrl);
		if (leafCtrl) {
			result = Ux_WndEventProc_KeyEvent(leafCtrl, evt, paramNum, paramArray);
		}
		//send to self
		result = Ux_SendEventInternal(pCtrl, evt, paramNum, paramArray);
	} else { //other type
		//send to self
		result = Ux_SendEventInternal(pCtrl, evt, paramNum, paramArray);
	}
	//all control would not receive this evt
	DBG_IND("### %s NO normal event 0x%08x ###\r\n", pCtrl->Name, evt);
	DBG_IND("### %s event CTRL_FOUND %d ###\r\n", pCtrl->Name, result);

	return result;
}


static INT32 Ux_WndEventProc_CmdEvent(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 result = NVTEVT_PASS;
	VControl **controlList;

	//recursive send to all child control
	controlList = pCtrl->ControlList;
	if (controlList) {
		int i = 0;
		while (controlList[i] != 0) {
			Ux_WndEventProc_CmdEvent(controlList[i], evt, paramNum, paramArray);
			i++;
		}
	}
	//send to self
	result = Ux_SendEventInternal(pCtrl, evt, paramNum, paramArray);
	//all control would not receive this evt
	DBG_IND("### %s NO normal event 0x%08x ###\r\n", pCtrl->Name, evt);
	DBG_IND("### %s event CTRL_FOUND %d ###\r\n", pCtrl->Name, result);

	return result;
}


INT32 Ux_WndEventProc(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 result = NVTEVT_PASS;
	VControl *pWnd = pCtrl;

	if (IS_TOUCH_EVENT(evt)) { //TOUCH event class
		//dispatch to current wnd and its touched/locked control
		Ux_WndEventProc_TouchEvent(pWnd, evt, paramNum, paramArray);
	} else if (IS_KEY_EVENT(evt)) { //KEY event class
		//dispatch to current wnd and its focused control
		result = Ux_WndEventProc_KeyEvent(pWnd, evt, paramNum, paramArray);
	} else { //other event classes
		//dispatch to current wnd and all of its control
		result = Ux_WndEventProc_CmdEvent(pWnd, evt, paramNum, paramArray);
	}
	return result;
}

INT32 Ux_WndDispatchMessage(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 result = NVTEVT_PASS;

	if (evt == NVTEVT_OPEN_WINDOW) {
		VControl *pWnd = (VControl *)paramArray[0];
		DBG_IND("get event NVTEVT_OPEN_WINDOW %s!\r\n", pWnd->Name);
		result = Ux_OpenWindowInternal(pWnd, paramNum - 1, paramArray + 1);
	} else if (evt == NVTEVT_CLOSE_WINDOW) {
		VControl *pWnd = (VControl *)paramArray[0];
		DBG_IND("get event NVTEVT_CLOSE_WINDOW %s!\r\n", pWnd->Name);
		result = Ux_CloseWindowInternal(pWnd, paramNum - 1, paramArray + 1);
		Ux_PostCloseWindowInternal();
	} else if (evt == NVTEVT_CLOSE_WINDOW_CLEAR) {
		VControl *pWnd = (VControl *)paramArray[0];
		DBG_IND("get event NVTEVT_CLOSE_WINDOW %s!\r\n", pWnd->Name);
		result = Ux_CloseWindowInternal(pWnd, paramNum - 1, paramArray + 1);
		Ux_PostCloseWindowInternal();
		if (result == NVTRET_OK) {
			if (_pUIRender) {
				UINT32 ScreenObj = 0;

				ScreenObj = _pUIRender->pfn_BeginScreen();
				_pUIRender->pfn_ClearScreen(ScreenObj);
				_pUIRender->pfn_EndScreen(ScreenObj);
			}
		}
	} else {
		VControl *pCtrl = 0;
		UINT8 wndIndex;
		Ux_GetFocusedWindowWithIndex(&pCtrl, &wndIndex);
		if (!pCtrl) {
			DBG_WRN("Try to get a window when no one is created. cmd: 0x%08x\r\n", evt);
		}
		if (pCtrl) {
			result = Ux_WndEventProc(pCtrl, evt, paramNum, paramArray);
		}
	}
	return result;
}



