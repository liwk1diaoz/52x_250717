/**
    @file       UIWndShow.c

    @ingroup    mIAPPExtUIFrmwk

    @brief      NVT UI framework API
                Implement functions of UI framework API

    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/
#include <kwrap/flag.h>
#include <kwrap/semaphore.h>
#include "UIControl/UIControlWnd.h"
#include "UIWndInternal.h"
#include "UIControl/UIControlExt.h"
#include "UIControlID_int.h"

/**
@addtogroup mIAPPExtUIFrmwk
@{

@name Display Control Functions
@{
*/


UIRender *_pUIRender = 0;

UINT32  _gShowErrorMsg = 1;

void Ux_SetRender(UIRender *pUIRender)
{
	_pUIRender = pUIRender;
}
void Ux_SetDebugMsg(UINT32 Show)
{
	_gShowErrorMsg = Show;
}


void UxCtrl_SetDirty(VControl *pCtrl, BOOL bDirty)
{
	VControl **controlList;

	if (!pCtrl) {
		return;
	}
	if (bDirty) {
		pCtrl->flag |= DIRTY_THIS;
	} else {
		pCtrl->flag &= ~DIRTY_THIS;
	}

	UxCtrl_SetChildDirty(pCtrl, -1, bDirty);

	controlList = pCtrl->ControlList;
	if (controlList && (pCtrl->flag & DIRTY_CHILD)) {
		int i = 0;
		while (controlList[i] != 0) {
			UxCtrl_SetDirty(controlList[i], bDirty);
			i++;
		}
	}

	//recursive dirty parent's child bit
	if (pCtrl->pParent) {
		VControl *Parent = pCtrl->pParent;
		while (Parent) {
			Parent->flag |= DIRTY_CHILD;
			Parent = Parent->pParent;
		}
	}
}


BOOL UxCtrl_IsDirty(VControl *pCtrl)
{
	if (!pCtrl) {
		return FALSE;
	}
	return pCtrl->flag & DIRTY_THIS;
}


void UxCtrl_SetShow(VControl *pCtrl, BOOL bShow)
{
	if (!pCtrl) {
		return;
	}
	if (bShow) {
		pCtrl->flag &= ~CTRL_HIDE;
		UxCtrl_SetDirty(pCtrl, TRUE);
	} else {
		pCtrl->flag |= CTRL_HIDE;
		//should redraw parent
		if (pCtrl->pParent) {
			UxCtrl_SetDirty(pCtrl->pParent, TRUE);
		} else {
			UxCtrl_SetDirty(pCtrl, TRUE);
		}
	}

}

BOOL UxCtrl_IsShow(VControl *pCtrl)
{
	if (!pCtrl) {
		return FALSE;
	}
	return !(pCtrl->flag & CTRL_HIDE);
}

static void UxCtrl_SetChildShow(VControl *pCtrl, BOOL bShow)
{
	VControl **controlList;
	UINT32 i = 0;

	if (!pCtrl) {
		return;
	}

	UxCtrl_SetShow(pCtrl, bShow);

	controlList = pCtrl->ControlList;
	if (controlList) {
		while (controlList[i] != 0) {
			UxCtrl_SetChildShow(controlList[i], bShow);
			i++;
		}
	}
}
void UxCtrl_SetAllChildShow(VControl *pCtrl, BOOL bShow)
{
	VControl **controlList;
	UINT32 i = 0;

	if (!pCtrl) {
		return;
	}
	controlList = pCtrl->ControlList;
	if (controlList) {
		while (controlList[i] != 0) {
			UxCtrl_SetChildShow(controlList[i], bShow);
			i++;
		}
	}
}




void UxCtrl_SetPos(VControl *pCtrl, Ux_RECT rect)
{
	if (!pCtrl) {
		return;
	}
	pCtrl->rect = rect;
	//should redraw parent
	if (pCtrl->pParent) {
		UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	} else {
		UxCtrl_SetDirty(pCtrl, TRUE);
	}
}


INT32 UxCtrl_GetPos(VControl *pCtrl, Ux_RECT *pRect)
{
	if (!pCtrl || !pRect) {
		return NVTRET_ERROR;
	}
	memcpy(pRect, &pCtrl->rect, sizeof(Ux_RECT));
	return NVTRET_OK;

}

void UxCtrl_SetLock(VControl *pCtrl, BOOL bLock)
{
	if (bLock) {
		pLockedCtrl = pCtrl;
	} else {
		pLockedCtrl = NULL;
	}

}

VControl *pLockedCtrl = NULL;

VControl *UxCtrl_GetLock()
{
	return pLockedCtrl;
}
static void Ux_RedrawControl(VControl *pCtrl, UINT32 ScreenObj)
{
	DBG_IND("MSG: Ctrl %s flag:%08x child:%d\r\n", pCtrl->Name, pCtrl->flag, pCtrl->ControlList);
	if (!(pCtrl->flag & CTRL_HIDE)
		&& (pCtrl->flag & DIRTY_MASK)) {
		VControl **controlList;

		if (_pUIRender) {
			_pUIRender->pfn_BeginControl(pCtrl->pParent, pCtrl, ScreenObj);
		}

		Ux_SendEvent(pCtrl, NVTEVT_BEGIN_CTRL, 1, ScreenObj);

		if (pCtrl->flag & DIRTY_THIS) {
			DBG_IND("MSG: Draw this\r\n");
			Ux_SendEvent(pCtrl, NVTEVT_REDRAW, 1, ScreenObj);
			DBG_IND("MSG: Draw ok\r\n");
		}

		//resursive call child redraw
		controlList = pCtrl->ControlList;
		if (controlList && (pCtrl->flag & DIRTY_CHILD)) {
			int i = 0;
			while (controlList[i] != 0) {
				DBG_IND("MSG: Draw child[%d] %s\r\n", i, controlList[i]->Name);
				Ux_RedrawControl(controlList[i], ScreenObj);
				DBG_IND("MSG: Draw child ok\r\n");
				i++;
			}
		}

		Ux_SendEvent(pCtrl, NVTEVT_END_CTRL, 1, ScreenObj);

		if (_pUIRender) {
			_pUIRender->pfn_EndControl(pCtrl->pParent, pCtrl, ScreenObj);
		}

	}
	pCtrl->flag &= ~DIRTY_MASK;



}


NVTRET Ux_Redraw(void)
{
	VControl *pWnd = 0;
	UINT32 ScreenObj = 0;
	NVTRET ret = NVTRET_OK;

	vos_sem_wait(UICTRL_DRW_SEM_ID);

	// Skip draw functions when the windows are closed forcely.
	if (vos_flag_chk(UICTRL_FLG_ID, FLGFRAMEWORK_FORCE_CLOSE) == FLGFRAMEWORK_FORCE_CLOSE) {
		DBG_IND(" ... Skip OnDraw\n\r");
		goto Draw_end;
	}
	//get Window
	Ux_GetFocusedWindow(&pWnd);
	if (!pWnd) {
		ret = NVTRET_ERROR;
		goto Draw_end;
	}
	DBG_IND("%s Ux_Redraw\n\r",pWnd->Name);

	if (!_pUIRender) {
		ret = NVTRET_ERROR;
		if(_gShowErrorMsg){
			DBG_ERR("Not Assign Renderer!\r\n");
		}
		goto Draw_end;
	}


	ScreenObj = _pUIRender->pfn_BeginScreen();

	Ux_RedrawControl(pWnd, ScreenObj);

	_pUIRender->pfn_EndScreen(ScreenObj);


Draw_end:
    vos_sem_sig(UICTRL_DRW_SEM_ID);
	return ret;
}



void UxCtrl_SetChildDirty(VControl *pCtrl, INT32 nChildIndex, BOOL bDirty)
{
	VControl **controlList;
	VControl *pControl;
	if (!pCtrl) {
		return;
	}
	controlList = pCtrl->ControlList;
	if (!controlList) {
		return;
	}
	if (nChildIndex == -1) {
		int i;
		i = 0;
		while (controlList[i] != 0) {
			pControl = controlList[i];
			if (bDirty) {
				pControl->flag |= DIRTY_THIS;
			} else {
				pControl->flag &= ~DIRTY_THIS;
			}
			i++;
		}
		if (bDirty) {
			pCtrl->flag |= DIRTY_CHILD;
		} else {
			pCtrl->flag &= ~DIRTY_CHILD;
		}
	} else {
		pControl = controlList[nChildIndex];
		if (!pControl) {
			return ;
		}

		if (bDirty) {
			pControl->flag |= DIRTY_THIS;
			pCtrl->flag |= DIRTY_CHILD;
		} else {
			pControl->flag &= ~DIRTY_MASK;
			{
				int i;
				BOOL bDrawAnyChild = FALSE;
				i = 0;
				while (controlList[i] != 0) {
					pControl = controlList[i];
					if (pControl->flag & DIRTY_MASK) {
						bDrawAnyChild = TRUE;
					}
				}
				if (!bDrawAnyChild) {
					pCtrl->flag &= ~DIRTY_CHILD;
				}
			}
		}
	}
}

NVTRET Ux_RedrawAllWind(void)
{
	VControl *pWnd = 0;
	UINT32 ScreenObj = 0;
	NVTRET ret = E_OK;
	UINT8 wndIndex, i;

	vos_sem_wait(UICTRL_DRW_SEM_ID);

	// Skip draw functions when the windows are closed forcely.
	if (vos_flag_chk(UICTRL_FLG_ID, FLGFRAMEWORK_FORCE_CLOSE) == FLGFRAMEWORK_FORCE_CLOSE) {
		DBG_IND(" ... Skip OnDraw\n\r");
		goto Draw_end;
	}

	if (_pUIRender) {
		ScreenObj = _pUIRender->pfn_BeginScreen();
	}

	//get current Window index
	Ux_GetFocusedWindowWithIndex(&pWnd, &wndIndex);

	//draw window from root (index 0) window
	for (i = 0; i < wndIndex; i++) {
		Ux_GetWindowByIndex(&pWnd, i);
		if (pWnd) {
			UxCtrl_SetDirty(pWnd, 1);
			Ux_RedrawControl(pWnd, ScreenObj);
		}

	}

	if (_pUIRender) {
		_pUIRender->pfn_EndScreen(ScreenObj);
	}

Draw_end:
    vos_sem_sig(UICTRL_DRW_SEM_ID);

	return ret;
}


VControl *UxCtrl_GetChild(VControl *pCtrl, UINT32 index)
{
#if 0
	//#NT#2008/09/16@Steven Wang begin -
	//#NT#Record parent control
	if (pCtrl->ControlList[index]) {
		pCtrl->ControlList[index]->pParent = pCtrl;
	}
	//#NT#2008/09/16@Steven Wang end -
#endif
	return (pCtrl->ControlList[index]);
}


void UxCtrl_SetChild(VControl *pParent, UINT32 index, VControl *pCtrl)
{
	pParent->ControlList[index] = pCtrl;
	pCtrl->pParent = pParent;
}


VControl *UxCtrl_GetLeafFocus(VControl *pCtrl)
{
	VControl *pChildCtrl = pCtrl;
	CTRL_TAB_DATA *ctrlTabData;
	while ((pChildCtrl) && (pChildCtrl->wType == CTRL_TAB)) {
		ctrlTabData = (CTRL_TAB_DATA *)pChildCtrl->CtrlData;
		pChildCtrl = UxCtrl_GetChild(pChildCtrl, ctrlTabData->focus);
	}
	return pChildCtrl;
}


void UxCtrl_SetShowTable(VControl *pCtrl, ITEM_BASE **showTable)
{
	pCtrl->ShowList = showTable ;
}

ITEM_BASE **UxCtrl_GetShowTable(VControl *pCtrl)
{
	return pCtrl->ShowList;
}

//@}

