#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlSliderLib.h"

#define SLIDER_BACKGROUND   0
#define SLIDER_THUMBNAIL    1
//#NT#2009/02/26#Lily Kao -begin
#define SLIDER_SKIN_DEPTH       8    //pixcels

typedef enum {
	SLIDER_CURSOR_POSLEFT = 0x00,
	SLIDER_CURSOR_POSRIGHT
} SLIDER_CURSOR_POS;
//#NT#2009/02/26#Lily Kao -end


///////////////////////////////////////////////////////////////////////////////
//
//      API called by users
///////////////////////////////////////////////////////////////////////////////
void UxSlider_SetData(VControl *pCtrl, SLIDER_DATA_SET attribute, UINT32 value)
{
	CTRL_SLIDER_DATA *slider ;
	if (pCtrl->wType != CTRL_SLIDER) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	slider = ((CTRL_SLIDER_DATA *)(pCtrl->CtrlData));

	switch (attribute) {
	case SLIDER_TYPE:
		slider->sliderType = value;
		break;
	case SLIDER_CURSTP:
		slider->currentStep = value;
		break;
	case SLIDER_TOTSTP:
		slider->totalStep = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxSlider_GetData(VControl *pCtrl, SLIDER_DATA_SET attribute)
{
	CTRL_SLIDER_DATA *slider ;
	if (pCtrl->wType != CTRL_SLIDER) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	slider = ((CTRL_SLIDER_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case SLIDER_TYPE:
		return slider->sliderType;
	case SLIDER_CURSTP:
		return slider->currentStep;
	case SLIDER_TOTSTP:
		return slider->totalStep;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}
///////////////////////////////////////////////////////////////////////////////
//
//       Default  State   Cmd
///////////////////////////////////////////////////////////////////////////////
static INT32 UxSlider_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UxCtrl_SetDirty(pCtrl, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxSlider_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	//#NT#2009/03/23#Lily Kao -begin
	UINT32 curStep, totalStep;
	//#NT#2009/03/23#Lily Kao -end
	ITEM_BASE *pShowObj = NULL;
	ITEM_BASE *pShowObjBk = NULL;
	ITEM_BASE *pRectObj = NULL;
	ITEM_BASE *pThumbObj = NULL;
	UIScreen ScreenObj = *paramArray;
	IPOINT orgPos = Ux_GetOrigin(ScreenObj);
	//#NT#2009/02/26#Lily Kao -begin
	UINT8 thumbPos = SLIDER_CURSOR_POSLEFT;
	UINT16 thumbGapX, thumbGapY, rectGapX, rectGapY, tmpV;
	double tmpDouble;
	//#NT#2009/02/26#Lily Kao -end


	// Get first group
	pShowObj = (ITEM_BASE *)pCtrl->ShowList[0];

	pShowObjBk = ((ITEM_GROUP *)pShowObj)->ShowTable[SLIDER_BACKGROUND];
	pRectObj = ((ITEM_GROUP *)pShowObj)->ShowTable[SLIDER_THUMBNAIL];

	// Draw background group
	Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pShowObjBk)->ShowTable);

	pThumbObj = ((ITEM_GROUP *)pRectObj)->ShowTable[0];

	curStep = UxSlider_GetData(pCtrl, SLIDER_CURSTP);
	totalStep = UxSlider_GetData(pCtrl, SLIDER_TOTSTP);

	//#NT#2009/02/26#Lily Kao -begin
	thumbGapX = pThumbObj->x2 - pThumbObj->x1;
	thumbGapY = pThumbObj->y2 - pThumbObj->y1;
	rectGapX  = pRectObj->x2 - pRectObj->x1 ;//- SLIDER_SKIN_DEPTH;
	rectGapY  = pRectObj->y2 - pRectObj->y1 ;//- SLIDER_SKIN_DEPTH;

	// Slider type is horizontal
	if (UxSlider_GetData(pCtrl, SLIDER_TYPE) == SLIDER_HORIZONTAL) {
		if ((rectGapX / totalStep) > thumbGapX) {
			thumbPos = SLIDER_CURSOR_POSLEFT;
		} else {
			thumbPos = SLIDER_CURSOR_POSRIGHT;
		}
		//DBG_IND("rectGapX=%d,totalStep=%d,thumbGapX=%d,thumbPos=%d\r\n",rectGapX,totalStep,thumbGapX,thumbPos);
		if (curStep == 0) {
			Ux_SetOrigin(ScreenObj, (LVALUE)orgPos.x, (LVALUE)orgPos.y);
		} else {
			if (thumbPos == SLIDER_CURSOR_POSLEFT) {
				//slider Cursor left
				tmpDouble = curStep * rectGapX;
				tmpDouble /= totalStep;
				tmpV = (INT32)(tmpDouble + 0.5);
				tmpV += orgPos.x;
				Ux_SetOrigin(ScreenObj, (LVALUE)(tmpV - thumbGapX), (LVALUE)(orgPos.y));
			} else {
				//slider Cursor right
				if (rectGapX < thumbGapX) {
					DBG_ERR("SliderUI Fail:rectGapX=%d,thumbGapX=%d\r\n", rectGapX, thumbGapX);
				} else {
					rectGapX -= thumbGapX;
				}
				tmpDouble = curStep * rectGapX;
				tmpDouble /= totalStep;
				tmpV = (INT32)(tmpDouble + 0.5);
				tmpV += orgPos.x;
				Ux_SetOrigin(ScreenObj, (LVALUE)tmpV, (LVALUE)(orgPos.y));
			}
		}
	}
	// Slider type is vertical
	else {
		DBG_IND("rectGapY=%d,totalStep=%d,thumbGapY=%d\r\n", rectGapY, totalStep, thumbGapY);
		if ((rectGapY / totalStep) > thumbGapY) {
			thumbPos = SLIDER_CURSOR_POSLEFT;
		} else {
			thumbPos = SLIDER_CURSOR_POSRIGHT;
		}

		if (curStep == 0) {
			Ux_SetOrigin(ScreenObj, (LVALUE)orgPos.x, (LVALUE)orgPos.y);
		} else {
			if (thumbPos == SLIDER_CURSOR_POSLEFT) {
				//slider Cursor up
				tmpDouble = curStep * rectGapY;
				tmpDouble /= totalStep;
				tmpV = (INT32)(tmpDouble + 0.5);
				tmpV += orgPos.y;
				Ux_SetOrigin(ScreenObj, (LVALUE)(orgPos.x), (LVALUE)(tmpV - thumbGapY));
			} else {
				//slider Cursor down
				if (rectGapY < thumbGapY) {
					DBG_ERR("SliderUI Fail:rectGapY=%d,thumbGapY=%d\r\n", rectGapY, thumbGapY);
				} else {
					rectGapY -= thumbGapY;
				}
				tmpDouble = curStep * rectGapY;
				tmpDouble /= totalStep;
				tmpV = (INT32)(tmpDouble + 0.5);
				tmpV += orgPos.y;
				Ux_SetOrigin(ScreenObj, (LVALUE)(orgPos.x), (LVALUE)tmpV);
			}
		}
	}
	//#NT#2009/02/26#Lily Kao -end

	Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pThumbObj)->ShowTable);

	return NVTEVT_CONSUME;
}

static INT32 UxSlider_OnNextStep(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 curStep, totalStep;

	curStep = UxSlider_GetData(pCtrl, SLIDER_CURSTP);
	totalStep = UxSlider_GetData(pCtrl, SLIDER_TOTSTP);

	if (curStep < totalStep) {
		curStep ++;
		UxSlider_SetData(pCtrl, SLIDER_CURSTP, curStep);
	} else {
		return NVTEVT_CONSUME;
	}

	Ux_SendEvent(pCtrl, NVTEVT_CHANGE_STATE, 0);
	UxCtrl_SetDirty(pCtrl, TRUE);

	return NVTEVT_CONSUME;
}

static INT32 UxSlider_OnPreviousStep(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 curStep;

	curStep = UxSlider_GetData(pCtrl, SLIDER_CURSTP);

	if (curStep > 0) {
		curStep --;
		UxSlider_SetData(pCtrl, SLIDER_CURSTP, curStep);
	} else {
		return NVTEVT_CONSUME;
	}

	Ux_SendEvent(pCtrl, NVTEVT_CHANGE_STATE, 0);
	UxCtrl_SetDirty(pCtrl, TRUE);

	return NVTEVT_CONSUME;
}

static INT32 UxSlider_OnChange(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}


EVENT_ENTRY DefaultSliderCmdMap[] = {
	{NVTEVT_OPEN_WINDOW,                UxSlider_OnOpen},
	{NVTEVT_NEXT_STEP,                  UxSlider_OnNextStep},
	{NVTEVT_PREVIOUS_STEP,              UxSlider_OnPreviousStep},
	{NVTEVT_REDRAW,                     UxSlider_OnDraw},
	{NVTEVT_CHANGE_STATE,               UxSlider_OnChange},
	{NVTEVT_NULL,                       0},  //End of Command Map
};

VControl UxSliderCtrl = {
	"UxSliderCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultSliderCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};


