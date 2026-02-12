
#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlProgressBarLib.h"

/**
progress bar range is between Thumb obj x1~x2 (y1~y2)

*/


#define PROBAR_NORMAL_STATE       0 //PROGRESSBAR only one state,always get normal state
#define PROBAR_BACKGROUND   0
#define PROBAR_THUMBNAIL    1
///////////////////////////////////////////////////////////////////////////////
//
//      API called by internal
///////////////////////////////////////////////////////////////////////////////

static UINT32 UxProgressBar_FindItem(VControl *pCtrl, UINT32 curX, UINT32 curY)
{
	UINT16 curStep;
	ITEM_BASE *pShowObj = NULL;
	ITEM_BASE *pThumbObj = NULL;
	double tmpDouble = 0;
	UINT16 thumbGapX, thumbGapY;
	INT32 relativeX, relativeY;

	UINT32 probarType = UxProgressBar_GetData(pCtrl, PROBAR_TYPE);
	UINT16 totalStep = UxProgressBar_GetData(pCtrl, PROBAR_TOTSTP);
	// Get first group
	pShowObj = (ITEM_BASE *)pCtrl->ShowList[PROBAR_NORMAL_STATE];

	pThumbObj = ((ITEM_GROUP *)pShowObj)->ShowTable[PROBAR_THUMBNAIL];

	thumbGapX = pThumbObj->x2 - pThumbObj->x1;
	thumbGapY = pThumbObj->y2 - pThumbObj->y1;

	relativeX = curX - pCtrl->rect.x1;
	relativeY = curY - pCtrl->rect.y1;
	if (UxProgressBar_GetData(pCtrl, PROBAR_TYPE)&PROBAR_HORIZONTAL) {
		if (relativeX < 0) {
			curStep = 0;
		} else if (relativeX > thumbGapX) {
			curStep = totalStep - 1;
		} else {
			tmpDouble = relativeX * (totalStep - 1);
			tmpDouble /= thumbGapX;
			curStep = (UINT16)(tmpDouble + 0.5) ;
		}
	} else {
		if (relativeY < 0) {
			curStep = 0;
		} else if (relativeY > thumbGapY) {
			curStep = totalStep - 1;
		} else {
			tmpDouble = relativeY * (totalStep - 1);
			tmpDouble /= thumbGapY;
			curStep = (UINT16)(tmpDouble + 0.5) ;
		}
	}
	if (probarType & PROBAR_INVERSE) {
		curStep = (totalStep - 1) - curStep;
	}
	return curStep;

}
///////////////////////////////////////////////////////////////////////////////
//
//      API called by users
///////////////////////////////////////////////////////////////////////////////
void UxProgressBar_SetData(VControl *pCtrl, PROBAR_DATA_SET attribute, UINT32 value)
{
	CTRL_PROBAR_DATA *progressbar ;
	if (pCtrl->wType != CTRL_PROGRESSBAR) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	progressbar = ((CTRL_PROBAR_DATA *)(pCtrl->CtrlData));

	switch (attribute) {
	case PROBAR_TYPE:
		progressbar->progressBarType = value;
		break;
	case PROBAR_CURSTP:
		progressbar->currentStep = value;
		break;
	case PROBAR_TOTSTP:
		progressbar->totalStep = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxProgressBar_GetData(VControl *pCtrl, PROBAR_DATA_SET attribute)
{
	CTRL_PROBAR_DATA *progressbar ;
	if (pCtrl->wType != CTRL_PROGRESSBAR) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	progressbar = ((CTRL_PROBAR_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case PROBAR_TYPE:
		return progressbar->progressBarType;
	case PROBAR_CURSTP:
		return progressbar->currentStep;
	case PROBAR_TOTSTP:
		return progressbar->totalStep;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}
///////////////////////////////////////////////////////////////////////////////
//
//       Default  State   Cmd
///////////////////////////////////////////////////////////////////////////////
/*
static INT32 UxProgressBar_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UxCtrl_SetDirty(pCtrl, TRUE);
    return NVTEVT_CONSUME;
}
*/
static INT32 UxProgressBar_OnPress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_PROBAR_DATA  *progressbar = (CTRL_PROBAR_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		progressbar->currentStep = UxProgressBar_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}

	DBG_IND("OnPress %d ", progressbar->currentStep);

	UxCtrl_SetLock(pCtrl, TRUE);
	UxCtrl_SetDirty(pCtrl, TRUE);

	return NVTEVT_CONSUME;
}
static INT32 UxProgressBar_OnMove(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_PROBAR_DATA  *progressbar = (CTRL_PROBAR_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		progressbar->currentStep = UxProgressBar_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}

	DBG_IND("OnMove %d ", progressbar->currentStep);

	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;

}
static INT32 UxProgressBar_OnRelease(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_PROBAR_DATA  *progressbar = (CTRL_PROBAR_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		progressbar->currentStep = UxProgressBar_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}

	DBG_IND("release %d ", progressbar->currentStep);

	UxCtrl_SetLock(pCtrl, FALSE);
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}


static INT32 UxProgressBar_OnNextProgress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 curStep, totalStep;

	curStep = UxProgressBar_GetData(pCtrl, PROBAR_CURSTP);
	totalStep = UxProgressBar_GetData(pCtrl, PROBAR_TOTSTP);

	if (curStep < totalStep) {
		curStep ++;
		UxProgressBar_SetData(pCtrl, PROBAR_CURSTP, curStep);
	} else {
		return NVTEVT_CONSUME;
	}

	Ux_SendEvent(pCtrl, NVTEVT_CHANGE_STATE, 0);

	return NVTEVT_CONSUME;
}

static INT32 UxProgressBar_OnPreviousProgress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 curStep;

	curStep = UxProgressBar_GetData(pCtrl, PROBAR_CURSTP);

	if (curStep > 0) {
		curStep --;
		UxProgressBar_SetData(pCtrl, PROBAR_CURSTP, curStep);
	} else {
		return NVTEVT_CONSUME;
	}

	Ux_SendEvent(pCtrl, NVTEVT_CHANGE_STATE, 0);

	return NVTEVT_CONSUME;
}

static INT32 UxProgressBar_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	//#NT#2009/09/07#Lily Kao -begin
	UINT32 curStep, totalStep;
	//#NT#2009/09/07#Lily Kao -end
	ITEM_BASE *pShowObj = NULL;
	ITEM_BASE *pShowObjBk = NULL;
	ITEM_BASE *pThumbObj = NULL;
	UIScreen ScreenObj = *paramArray;
	//#NT#2009/02/27#Lily Kao -begin
	ITEM_BASE **ShowTable = NULL;
	UINT32 i = 0;
	double tmpDouble = 0;
	INT32 orgX1, orgY1, orgX2, orgY2;
	INT32 start, end, ThumbEnd, ThumbRange;

	UINT32 probarType = UxProgressBar_GetData(pCtrl, PROBAR_TYPE);
	//#NT#2009/02/27#Lily Kao -end

	// Get first group
	pShowObj = (ITEM_BASE *)pCtrl->ShowList[PROBAR_NORMAL_STATE];

	pShowObjBk = ((ITEM_GROUP *)pShowObj)->ShowTable[PROBAR_BACKGROUND];
	pThumbObj = ((ITEM_GROUP *)pShowObj)->ShowTable[PROBAR_THUMBNAIL];

	// Draw background group
	Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pShowObjBk)->ShowTable);

	curStep = UxProgressBar_GetData(pCtrl, PROBAR_CURSTP);
	totalStep = UxProgressBar_GetData(pCtrl, PROBAR_TOTSTP);

	//#NT#2009/02/27#Lily Kao -begin
	ShowTable = ((ITEM_GROUP *)pThumbObj)->ShowTable;

	if (ShowTable) {
		pShowObj = ShowTable[0];
	} else {
		DBG_ERR("no showTbl\r\n");
		return NVTEVT_CONSUME;
	}

	while (pShowObj != NULL) {
		Ux_GetShowObjPos(pShowObj, &orgX1, &orgY1, &orgX2, &orgY2);
		if (probarType & PROBAR_HORIZONTAL) {
			// ProBar type is horizontal
			ThumbRange = pThumbObj->x2 - pThumbObj->x1;
			if (probarType & PROBAR_INVERSE) {
				start = pThumbObj->x2;
				ThumbEnd = pThumbObj->x1;
			} else {
				start = pThumbObj->x1;
				ThumbEnd = pThumbObj->x2;
			}
		} else {
			// ProBar type is vertical
			ThumbRange = pThumbObj->y2 - pThumbObj->y1;
			if (probarType & PROBAR_INVERSE) {
				start = pThumbObj->y2;
				ThumbEnd = pThumbObj->y1;
			} else {
				start = pThumbObj->y1;
				ThumbEnd = pThumbObj->y2;
			}
		}

		if (curStep >= totalStep - 1) {
			end = ThumbEnd - 1;
		} else if (curStep == 0) {
			end = start;
		} else {
			tmpDouble = curStep;
			tmpDouble /= (totalStep - 1);
			tmpDouble *= ThumbRange;
			if (probarType & PROBAR_INVERSE) {
				end = (INT32)(start - tmpDouble);
			} else {
				end = (INT32)(start + tmpDouble);
			}
		}
		if (probarType & PROBAR_HORIZONTAL) {
			if (probarType & PROBAR_INVERSE) {
				orgX1 = end;
				orgX2 = start;
			} else {
				orgX1 = start;
				orgX2 = end;
			}
		} else {
			if (probarType & PROBAR_INVERSE) {
				orgY1 = end;
				orgY2 = start;
			} else {
				orgY1 = start;
				orgY2 = end;
			}
		}

		Ux_SetShowObjPos(pShowObj, orgX1, orgY1, orgX2, orgY2);

		i++;
		pShowObj = ShowTable[i];
	}
	//#NT#2009/02/27#Lily Kao -end
	//#NT#2009/0/31#Janice ::if curStep =0 ,not draw
	if (curStep) {
		Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pThumbObj)->ShowTable);
	}

	return NVTEVT_CONSUME;
}
EVENT_ENTRY DefaultProgressBarCmdMap[] = {
	{NVTEVT_NEXT_STEP,                  UxProgressBar_OnNextProgress},
	{NVTEVT_PREVIOUS_STEP,              UxProgressBar_OnPreviousProgress},
	{NVTEVT_PRESS,                      UxProgressBar_OnPress},
	{NVTEVT_MOVE,                       UxProgressBar_OnMove},
	{NVTEVT_RELEASE,                    UxProgressBar_OnRelease},
	{NVTEVT_REDRAW,                     UxProgressBar_OnDraw},
	{NVTEVT_NULL,                       0},  //End of Command Map
};

VControl UxProgressbarCtrl = {
	"UxProgressbarCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultProgressBarCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};


