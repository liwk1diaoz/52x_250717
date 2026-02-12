
#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlScrollBarLib.h"

/**
scroll bar range is between Track obj x1~x2 (y1~y2)

*/
#define SCRBAR_NORMAL        0   //SCRBAR only one state,always get normal state

#define SCRBAR_BACKGROUND        0
#define SCRBAR_UPGROUP           1
#define SCRBAR_DOWNGROUP         3
#define SCRBAR_THUMBNAIL_RECT    2
#define SCRBAR_THUMBNAIL          0
///////////////////////////////////////////////////////////////////////////////
//
//      API called by internal
///////////////////////////////////////////////////////////////////////////////
void UxScrollBar_GetRange(VControl *pCtrl, Ux_RECT *pRange)
{
	ITEM_BASE *pShowObj = NULL;
	ITEM_BASE *pRectObj = NULL;

	// Get first group
	pShowObj = (ITEM_BASE *)pCtrl->ShowList[SCRBAR_NORMAL];

	pRectObj = ((ITEM_GROUP *)pShowObj)->ShowTable[SCRBAR_THUMBNAIL_RECT];

	pRange->x1 = pCtrl->rect.x1 + pRectObj->x1;
	pRange->x2 = pCtrl->rect.x1 + pRectObj->x2;
	pRange->y1 = pCtrl->rect.y1 + pRectObj->y1;
	pRange->y2 = pCtrl->rect.y1 + pRectObj->y2;

	DBG_IND("****scr x1  %d,x2 %d  y1 %d,y2 %d \r\n ", pRange->x1, pRange->x2, pRange->y1, pRange->y2);

}



static UINT32 UxScrollBar_FindItem(VControl *pCtrl, UINT32 curX, UINT32 curY)
{
	UINT16 curStep, totalStep, shape;
	ITEM_BASE *pShowObj = NULL;
	ITEM_BASE *pRectObj = NULL;
	ITEM_BASE *pThumbObj = NULL;
	UINT16 thumbGapX, thumbGapY, rectGapX, rectGapY;
	INT32 relativeX, relativeY;

	// Get first group
	pShowObj = (ITEM_BASE *)pCtrl->ShowList[SCRBAR_NORMAL];

	pRectObj = ((ITEM_GROUP *)pShowObj)->ShowTable[SCRBAR_THUMBNAIL_RECT];

	pThumbObj = ((ITEM_GROUP *)pRectObj)->ShowTable[SCRBAR_THUMBNAIL];

	totalStep =  UxScrollBar_GetData(pCtrl, SCRBAR_TOTSTP);
	shape =  UxScrollBar_GetData(pCtrl, SCRBAR_SHAPE);

	rectGapX  = pRectObj->x2 - pRectObj->x1 ;
	rectGapY  = pRectObj->y2 - pRectObj->y1 ;
	if (shape == SCRBAR_FIXSIZE) {
		thumbGapX = pThumbObj->x2 - pThumbObj->x1;
		thumbGapY = pThumbObj->y2 - pThumbObj->y1;
	} else {
		thumbGapX = rectGapX / totalStep;
		thumbGapY = rectGapY / totalStep;

	}
	relativeX = curX - pCtrl->rect.x1;
	relativeY = curY - pCtrl->rect.y1;

	if (UxScrollBar_GetData(pCtrl, SCRBAR_TYPE) == SCRBAR_HORIZONTAL) {
		if (relativeX < pRectObj->x1) {
			curStep = 0;
		} else if (relativeX + thumbGapX > pRectObj->x2) {
			curStep = totalStep - 1;
		} else {
			curStep = (relativeX - pRectObj->x1) * (totalStep - 1) / (rectGapX - thumbGapX);
		}
	} else {
		if (relativeY < pRectObj->y1) {
			curStep = 0;
		} else if (relativeY + thumbGapY > pRectObj->y2) {
			curStep = totalStep - 1;
		} else {
			curStep = (relativeY - pRectObj->y1) * (totalStep - 1) / (rectGapY - thumbGapY);
		}
	}
	return curStep;
}
///////////////////////////////////////////////////////////////////////////////
//
//      API called by users
///////////////////////////////////////////////////////////////////////////////
void UxScrollBar_SetData(VControl *pCtrl, SCRBAR_DATA_SET attribute, UINT32 value)
{
	CTRL_SCRBAR_DATA *scrollbar ;
	if (pCtrl->wType != CTRL_SCROLLBAR) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	scrollbar = ((CTRL_SCRBAR_DATA *)(pCtrl->CtrlData));


	switch (attribute) {
	case SCRBAR_TYPE:
		scrollbar->scrlBarType = value;
		break;
	case SCRBAR_SHAPE:
		scrollbar->scrlBarShape = value;
		break;
	case SCRBAR_CURSTP:
		if (value < scrollbar->totalStep) {
			scrollbar->currentStep = value;
		} else {
			DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_VALUE_MSG);
		}
		break;
	case SCRBAR_TOTSTP:
		if (value > 0) {
			scrollbar->totalStep = value;
		} else {
			DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_VALUE_MSG);
		}
		break;
	case SCRBAR_PAGESTP:
		if ((value <= scrollbar->totalStep) &&
			(scrollbar->scrlBarShape == SCRBAR_STEP_AVG)) {
			scrollbar->pageStep = value;
		} else {
			DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_VALUE_MSG);
		}
		break;
	case SCRBAR_THUMB:
        {
        	ITEM_BASE *pShowObj = NULL;
        	ITEM_BASE *pRectObj = NULL;
        	ITEM_BASE *pThumbObj = NULL;
            ITEM_BASE *pThumbShowObj = NULL;

        	// Get first group
        	pShowObj = (ITEM_BASE *)pCtrl->ShowList[SCRBAR_NORMAL];
        	pRectObj = ((ITEM_GROUP *)pShowObj)->ShowTable[SCRBAR_THUMBNAIL_RECT];
        	pThumbObj = ((ITEM_GROUP *)pRectObj)->ShowTable[SCRBAR_THUMBNAIL];

            pThumbShowObj = (((ITEM_GROUP *)pThumbObj)->ShowTable)[0];
            if((pThumbShowObj->ItemType & CMD_MASK)==CMD_Rectangle) {

        		if (scrollbar->scrlBarType == SCRBAR_HORIZONTAL) {
        			pThumbShowObj->x2=pThumbShowObj->x1+value;
        		} else {
        			pThumbShowObj->y2=pThumbShowObj->y1+value;
        		}
            }

    		if (scrollbar->scrlBarType == SCRBAR_HORIZONTAL) {
    			pThumbObj->x2=pThumbObj->x1+value;
    		} else {
    			pThumbObj->y2=pThumbObj->y1+value;
    		}
    	}
        break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxScrollBar_GetData(VControl *pCtrl, SCRBAR_DATA_SET attribute)
{
	CTRL_SCRBAR_DATA *scrollbar ;
	ITEM_BASE *pShowObj = NULL;
	ITEM_BASE *pRectObj = NULL;
	ITEM_BASE *pThumbObj = NULL;
	UINT16 thumbGapX, thumbGapY, rectGapX, rectGapY;

	if (pCtrl->wType != CTRL_SCROLLBAR) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	scrollbar = ((CTRL_SCRBAR_DATA *)(pCtrl->CtrlData));

	// Get first group
	pShowObj = (ITEM_BASE *)pCtrl->ShowList[SCRBAR_NORMAL];

	pRectObj = ((ITEM_GROUP *)pShowObj)->ShowTable[SCRBAR_THUMBNAIL_RECT];

	pThumbObj = ((ITEM_GROUP *)pRectObj)->ShowTable[SCRBAR_THUMBNAIL];

	thumbGapX = pThumbObj->x2 - pThumbObj->x1;
	thumbGapY = pThumbObj->y2 - pThumbObj->y1;
	rectGapX  = pRectObj->x2 - pRectObj->x1 ;
	rectGapY  = pRectObj->y2 - pRectObj->y1 ;

	switch (attribute) {
	case SCRBAR_TYPE:
		return scrollbar->scrlBarType;
	case SCRBAR_SHAPE:
		return scrollbar->scrlBarShape;
	case SCRBAR_CURSTP:
		return scrollbar->currentStep;
	case SCRBAR_TOTSTP:
		return scrollbar->totalStep;
	case SCRBAR_PAGESTP:
		return scrollbar->pageStep;
	case SCRBAR_RECT:
		if (scrollbar->scrlBarType == SCRBAR_HORIZONTAL) {
			return rectGapX;
		} else {
			return rectGapY;
		}
	case SCRBAR_THUMB:
		if (scrollbar->scrlBarType == SCRBAR_HORIZONTAL) {
			return thumbGapX;
		} else {
			return thumbGapY;
		}
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}

///////////////////////////////////////////////////////////////////////////////
//
//       Default  State   Cmd
///////////////////////////////////////////////////////////////////////////////
static INT32 UxScrollbar_OnNextStep(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 curStep, totalStep, pageStep;

	pageStep = UxScrollBar_GetData(pCtrl, SCRBAR_PAGESTP);
	curStep = UxScrollBar_GetData(pCtrl, SCRBAR_CURSTP);
	totalStep =  UxScrollBar_GetData(pCtrl, SCRBAR_TOTSTP);

	if (pageStep) {
		totalStep =  totalStep - (pageStep - 1);
	}

	if (curStep < totalStep - 1) {
		curStep ++;
	} else {
		curStep = 0 ;
	}

	UxScrollBar_SetData(pCtrl, SCRBAR_CURSTP, curStep);
	Ux_SendEvent(pCtrl, NVTEVT_CHANGE_STATE, 0);
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxScrollbar_OnPreviousStep(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 curStep, totalStep, pageStep;

	pageStep = UxScrollBar_GetData(pCtrl, SCRBAR_PAGESTP);
	curStep = UxScrollBar_GetData(pCtrl, SCRBAR_CURSTP);
	totalStep =  UxScrollBar_GetData(pCtrl, SCRBAR_TOTSTP);

	if (pageStep) {
		totalStep =  totalStep - (pageStep - 1);
	}

	if (curStep > 0) {
		curStep --;
	} else {
		curStep = totalStep - 1;
	}

	UxScrollBar_SetData(pCtrl, SCRBAR_CURSTP, curStep);
	Ux_SendEvent(pCtrl, NVTEVT_CHANGE_STATE, 0);
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxScrollbar_OnChange(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}
static INT32 UxScrollbar_OnPress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_SCRBAR_DATA  *scrbar = (CTRL_SCRBAR_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		scrbar->currentStep = UxScrollBar_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}
	UxCtrl_SetLock(pCtrl, TRUE);
	UxCtrl_SetDirty(pCtrl, TRUE);

	return NVTEVT_CONSUME;
}

static INT32 UxScrollbar_OnMove(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_SCRBAR_DATA  *scrbar = (CTRL_SCRBAR_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		scrbar->currentStep = UxScrollBar_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;

}
static INT32 UxScrollbar_OnRelease(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_SCRBAR_DATA  *scrbar = (CTRL_SCRBAR_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		scrbar->currentStep = UxScrollBar_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}
	UxCtrl_SetLock(pCtrl, FALSE);
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxScrollbar_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT16 curStep, totalStep, pageStep;
	UINT32 type, shape = 0;
	//double stepValue;
	ITEM_BASE *pShowObj = NULL;
	ITEM_BASE *pShowObjBk = NULL;
	ITEM_BASE *pShowObjUp = NULL;
	ITEM_BASE *pShowObjDown = NULL;
	ITEM_BASE *pRectObj = NULL;
	ITEM_BASE *pThumbObj = NULL;
	ITEM_BASE **ShowTable = NULL;
	UIScreen ScreenObj = *paramArray;
	IPOINT orgPos = Ux_GetOrigin(ScreenObj);
	INT32 thumbGapX, thumbGapY, rectGapX, rectGapY;
	INT32 newX1, newY1;


	// Get first group
	pShowObj = (ITEM_BASE *)pCtrl->ShowList[SCRBAR_NORMAL];

	pShowObjBk = ((ITEM_GROUP *)pShowObj)->ShowTable[SCRBAR_BACKGROUND];
	// Draw background group
	Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pShowObjBk)->ShowTable);

	pShowObjUp = ((ITEM_GROUP *)pShowObj)->ShowTable[SCRBAR_UPGROUP];
	// Draw up group
	Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pShowObjUp)->ShowTable);

	pShowObjDown = ((ITEM_GROUP *)pShowObj)->ShowTable[SCRBAR_DOWNGROUP];
	// Draw down group
	Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pShowObjDown)->ShowTable);

	pRectObj = ((ITEM_GROUP *)pShowObj)->ShowTable[SCRBAR_THUMBNAIL_RECT];
	pThumbObj = ((ITEM_GROUP *)pRectObj)->ShowTable[SCRBAR_THUMBNAIL];
	ShowTable = ((ITEM_GROUP *)pThumbObj)->ShowTable;
	if (ShowTable) {
		pShowObj = ShowTable[0];
	}

	pageStep = UxScrollBar_GetData(pCtrl, SCRBAR_PAGESTP);
	curStep = UxScrollBar_GetData(pCtrl, SCRBAR_CURSTP);
	totalStep =  UxScrollBar_GetData(pCtrl, SCRBAR_TOTSTP);

	type = UxScrollBar_GetData(pCtrl, SCRBAR_TYPE);
	shape =  UxScrollBar_GetData(pCtrl, SCRBAR_SHAPE);

	rectGapX  = pRectObj->x2 - pRectObj->x1 ;
	rectGapY  = pRectObj->y2 - pRectObj->y1 ;

	if (shape == SCRBAR_FIXSIZE) {
		thumbGapX = pThumbObj->x2 - pThumbObj->x1;
		thumbGapY = pThumbObj->y2 - pThumbObj->y1;
	} else {
		if (pageStep) {
			thumbGapX = rectGapX * pageStep / totalStep;
			thumbGapY = rectGapY * pageStep / totalStep;
			totalStep =  totalStep - (pageStep - 1);
		} else

		{
			thumbGapX = rectGapX / totalStep;
			thumbGapY = rectGapY / totalStep;
		}
	}

	//if(pageStep)


	if (type == SCRBAR_HORIZONTAL) {
		if (curStep >= totalStep - 1) { // if there is SCRBAR_PAGESTP curStep > totalStep
			newX1 = (orgPos.x + pRectObj->x2 - thumbGapX - pThumbObj->x1);
			newY1 = orgPos.y ;
		} else {
			//#NT#2010/05/26#Janice Huang#[0010589] -begin
			//stepValue = (rectGapX - thumbGapX) / (totalStep-1 );
			newX1 = (orgPos.x + pRectObj->x1 + (UINT16)(curStep * (rectGapX - thumbGapX) / (totalStep - 1)) - pThumbObj->x1);
			newY1 = orgPos.y ;
			//#NT#2010/05/26#Janice Huang#[0010589] -end
		}
	} else {
		if (curStep >= totalStep - 1) { // if there is SCRBAR_PAGESTP curStep > totalStep
			newX1 = orgPos.x;
			newY1 = (orgPos.y + pRectObj->y2 - thumbGapY - pThumbObj->y1) ;
		} else {
			//#NT#2010/05/26#Janice Huang#[0010589] -begin
			//stepValue = (rectGapY -thumbGapY) / (totalStep-1 );
			newX1 = orgPos.x;
			newY1 = (orgPos.y + pRectObj->y1 + (UINT16)(curStep * (rectGapY - thumbGapY) / (totalStep - 1)) - pThumbObj->y1) ;
			//#NT#2010/05/26#Janice Huang#[0010589] -end
		}
	}

	if (shape == SCRBAR_STEP_AVG) {
		if (type == SCRBAR_HORIZONTAL) {
			Ux_SetShowObjPos(pShowObj, pThumbObj->x1, pThumbObj->y1, (INT32)(pThumbObj->x1 + thumbGapX), pThumbObj->y2);
		} else {
			Ux_SetShowObjPos(pShowObj, pThumbObj->x1, pThumbObj->y1, pThumbObj->x2, (INT32)(pThumbObj->y1 + thumbGapY));
		}
	}

	Ux_SetOrigin(ScreenObj, (LVALUE)newX1, (LVALUE)newY1);
	Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pThumbObj)->ShowTable);

	return NVTEVT_CONSUME;
}

EVENT_ENTRY DefaultScrollbarCmdMap[] = {
	{NVTEVT_NEXT_STEP,                  UxScrollbar_OnNextStep},
	{NVTEVT_PREVIOUS_STEP,              UxScrollbar_OnPreviousStep},
	{NVTEVT_CHANGE_STATE,               UxScrollbar_OnChange},
	{NVTEVT_PRESS,                      UxScrollbar_OnPress},
	{NVTEVT_MOVE,                       UxScrollbar_OnMove},
	{NVTEVT_RELEASE,                    UxScrollbar_OnRelease},
	{NVTEVT_REDRAW,                     UxScrollbar_OnDraw},
	{NVTEVT_NULL,                       0},  //End of Command Map
};


VControl UxScrollbarCtrl = {
	"UxScrollbarCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultScrollbarCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};


