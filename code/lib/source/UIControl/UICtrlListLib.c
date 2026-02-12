#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlListLib.h"

/////////////////////////////////////////////////////////////////////////////
//        API
////////////////////////////////////////////////////////////////////////////////
static UINT32 UxList_GetStatusShowTableIndex(UINT32 status)
{
	if ((status & STATUS_ENABLE_MASK) & STATUS_DISABLE) {
		if ((status & STATUS_FOCUS_MASK) & STATUS_FOCUS_BIT) {
			return STATUS_FOCUS_DISABLE;    //user define disable sel item show style
		} else {
			return STATUS_NORMAL_DISABLE;    //user define disable unsel item show style
		}

	} else { //enable  item
		if ((status & STATUS_FOCUS_MASK) & STATUS_FOCUS_BIT) {
			return STATUS_FOCUS;
		} else {
			return STATUS_NORMAL;
		}
	}
}
static void UxList_ScrollOneItemCal(CTRL_LIST_DATA *list, UINT32 *pStartIndex, UINT32 *pEndIndex)
{
	static UINT32 scrollStartIndex = 0, scrollEndIndex = 0;
	if (list->action == NVTEVT_NEXT_ITEM) {
		if (list->currentItem != 0) {
			if (list->currentItem >= scrollEndIndex) {
				//scoll to next item,keep bar in the end
				scrollEndIndex = list->currentItem + 1;
				scrollStartIndex = scrollEndIndex - list->pageItem ;
			}
		} else {
			//go to first item and bar stay in the front
			scrollStartIndex = 0 ;
			scrollEndIndex = list->pageItem;

		}
	} else if (list->action == NVTEVT_PREVIOUS_ITEM) {
		if (list->currentItem != list->totalItem - 1) {
			if (list->currentItem < scrollStartIndex) {
				//scoll to pre item,keep bar in the front
				scrollStartIndex = list->currentItem;
				scrollEndIndex = scrollStartIndex + list->pageItem ;
			}
		} else {
			//go to last item and bar stay in the end
			scrollEndIndex = list->totalItem;
			scrollStartIndex = scrollEndIndex - list->pageItem ;
		}

	} else if (list->action == 0) {
		//init state
		scrollEndIndex = list->pageItem;
	}
	*pStartIndex = scrollStartIndex;
	*pEndIndex = scrollEndIndex;
	list->action = 0;  //clear action flag every time
}

static void UxList_ScrollOnePageCal(CTRL_LIST_DATA *list, UINT32 *pStartIndex, UINT32 *pEndIndex)
{
	*pEndIndex = list->pageItem;
	//scroll one page once
	if (list->currentItem >= list->pageItem) {
		UINT32 uiPage = 0;
		uiPage = (list->currentItem - (list->currentItem % list->pageItem)) / list->pageItem ;
		*pStartIndex = list->pageItem * uiPage;
		*pEndIndex = list->pageItem * (uiPage + 1);
	}

	if (*pEndIndex > list->totalItem) {
		*pEndIndex = list->totalItem;
	}

}

///////////////////////////////////////////////////////////////////////////////
//
//      API called by internal
///////////////////////////////////////////////////////////////////////////////

static UINT32 UxList_FindItem(VControl *pCtrl, UINT32 x, UINT32 y)
{
	CTRL_LIST_DATA  *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;
	UINT32 itemIndex = 0, itemPosIndex = 0, shiftPos = 0, Postion = 0;
	UINT32 shiftPosX = 0, shiftPosY = 0, PostionX = 0, PostionY = 0;
	UINT32 uiStartIndex = 0, uiEndIndex = 0;
	UINT32 totalRow = 0, rowIndex;
	UINT32 bFound = FALSE;

	shiftPosX = pCtrl->rect.x2 - pCtrl->rect.x1;
	PostionX = x - pCtrl->rect.x1;
	shiftPosY = pCtrl->rect.y2 - pCtrl->rect.y1;
	PostionY = y - pCtrl->rect.y1;

	//calculate next item shift position
	if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_VERTICAL) {
		Postion = PostionY;
		shiftPos = shiftPosY;
	} else { //LIST_LAYOUT_HORIZONTAL  and LIST_LAYOUT_ARRAY
		Postion = PostionX;
		shiftPos = shiftPosX;
	}

	if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_ARRAY) {
		totalRow = ((list->totalItem - 1) / list->pageItem) + 1;
	} else {
		totalRow = 1;
	}

	for (rowIndex = 0; rowIndex < totalRow; rowIndex++) {
		if ((PostionY > rowIndex * shiftPosY) && (PostionY < (rowIndex + 1)*shiftPosY)) {
			break;
		}
	}
	if (rowIndex >= totalRow) {
		rowIndex = 0;
	}

	DBG_IND("###  rowIndex %d\r\n", rowIndex);
	//calculate start and end index
	if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_ARRAY) {
		uiStartIndex = rowIndex * list->pageItem;
		if (uiStartIndex + list->pageItem < list->totalItem) {
			uiEndIndex = uiStartIndex + list->pageItem;
		} else {
			uiEndIndex = list->totalItem;
		}
	} else {
		if ((list->style & LIST_SCROLL_MASK) & LIST_SCROLL_NEXT_ITEM) {
			UxList_ScrollOneItemCal(list, &uiStartIndex, &uiEndIndex);
		} else {
			UxList_ScrollOnePageCal(list, &uiStartIndex, &uiEndIndex);
		}
	}
	itemIndex = uiStartIndex;
	for (itemPosIndex = 0; itemPosIndex <= uiEndIndex; itemPosIndex++) {
		//disable hide,need to check disable item
		if ((list->style & LIST_DISABLE_MASK) & LIST_DISABLE_HIDE) {
			//check if current item is disalbe,if disable get next item
			CTRL_LIST_ITEM_DATA *item;
			item = list->item[itemIndex];
			while (item->StatusFlag == STATUS_DISABLE) {
				item = list->item[++itemIndex];
				if (itemIndex == list->totalItem) {
					break;
				}
			}
		}

		DBG_IND("itemIndex %d %d\r\n", itemIndex, itemPosIndex);
		if (itemIndex == list->totalItem) {
			break;
		}

		//find match item
		if ((Postion > itemPosIndex * shiftPos) && (Postion < (itemPosIndex + 1)*shiftPos)) {
			bFound = TRUE;
			break;
		}
		itemIndex++;
	}


	DBG_IND("### bFound %d ,focus itemIndex %d\r\n", bFound, itemIndex);
	if (bFound) {
		return itemIndex;
	} else {
		return list->currentItem;
	}

}

void UxList_GetRange(VControl *pCtrl, Ux_RECT *pRange)
{
	CTRL_LIST_DATA *list ;
	UINT32 rangeItemNum = 0;

	if (pCtrl->wType != CTRL_LIST) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	list = ((CTRL_LIST_DATA *)(pCtrl->CtrlData));

	//use relative list rect
	pRange->x1 = pCtrl->rect.x1;
	pRange->y1 = pCtrl->rect.y1;

	if (list->totalItem > list->pageItem) {
		rangeItemNum = list->pageItem;
	} else {
		rangeItemNum = list->totalItem;
	}

	if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_HORIZONTAL) {
		pRange->x2 = pRange->x1 + (pCtrl->rect.x2 - pCtrl->rect.x1) * rangeItemNum;
		pRange->y2 = pRange->y1 + (pCtrl->rect.y2 - pCtrl->rect.y1);
	} else if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_VERTICAL) {
		pRange->x2 = pRange->x1 + (pCtrl->rect.x2 - pCtrl->rect.x1);
		pRange->y2 = pRange->y1 + (pCtrl->rect.y2 - pCtrl->rect.y1) * rangeItemNum;
	} else {
		UINT32 totalRow = ((list->totalItem - 1) / list->pageItem) + 1;

		pRange->x2 = pRange->x1 + (pCtrl->rect.x2 - pCtrl->rect.x1) * rangeItemNum;
		pRange->y2 = pRange->y1 + (pCtrl->rect.y2 - pCtrl->rect.y1) * totalRow;
	}

	DBG_IND("****list x1  %d,x2 %d  y1 %d,y2 %d \r\n ", pRange->x1, pRange->x2, pRange->y1, pRange->y2);

}

void UxList_GetItemPos(VControl *pCtrl, UINT32 index, Ux_RECT *pRect)
{
	CTRL_LIST_DATA *list ;
	UINT32 i = 0, count = 0;
	UINT32 OneItemRange = 0;
	UINT32 pageNum = 0;
	if (pCtrl->wType != CTRL_LIST) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}

	if (!pRect) {
		return ;
	}

	list = ((CTRL_LIST_DATA *)(pCtrl->CtrlData));
	if (index == CURITEM_INDEX) {
		index = list->currentItem;
	}
	pageNum = list->pageItem;

	if (!pageNum) {
		return ;
	}

	//disable hide,need to check disable item
	if ((list->style & LIST_DISABLE_MASK) & LIST_DISABLE_HIDE) {
		for (i = 0; i < index ; i++) {
			if (UxList_GetItemData(pCtrl, i, MNUITM_STATUS) == STATUS_DISABLE) {
				count++;
			}
		}
	}
	DBG_IND("count %d\r\n", count);

	pRect->x1 = pCtrl->rect.x1;
	pRect->y1 = pCtrl->rect.y1;
	pRect->x2 = pCtrl->rect.x2;
	pRect->y1 = pCtrl->rect.y1;


	if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_HORIZONTAL) {
		OneItemRange = pCtrl->rect.x2 - pCtrl->rect.x1;
		pRect->x1 = pCtrl->rect.x1 + OneItemRange * ((index - count) % pageNum);
		pRect->x2 = pRect->x1 + OneItemRange;
	} else if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_VERTICAL) {
		OneItemRange = pCtrl->rect.y2 - pCtrl->rect.y1;
		pRect->y1 = pCtrl->rect.y1 + OneItemRange * ((index - count) % pageNum);
		pRect->y2 = pRect->y1 + OneItemRange;
	} else if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_ARRAY) {
		OneItemRange = pCtrl->rect.x2 - pCtrl->rect.x1;
		pRect->x1 = pCtrl->rect.x1 + OneItemRange * ((index - count) % pageNum);
		pRect->x2 = pRect->x1 + OneItemRange;
		OneItemRange = pCtrl->rect.y2 - pCtrl->rect.y1;
		pRect->y1 = pCtrl->rect.y1 + OneItemRange * ((index - count) / pageNum);
		pRect->y2 = pRect->y1 + OneItemRange;

	} else {
		DBG_ERR("not support \r\n");
	}
	DBG_IND(" %d %d %d %d \r\n", pRect->x1, pRect->x2, pRect->y1, pRect->y2);
}

////////////////////////////////////////////////////////////////////////////////
//      API called by users
////////////////////////////////////////////////////////////////////////////////
void UxList_SetItemData(VControl *pCtrl, UINT32 index, LSTITM_DATA_SET attribute, UINT32 value)
{
	CTRL_LIST_DATA *list ;
	CTRL_LIST_ITEM_DATA *item;

	if (pCtrl->wType != CTRL_LIST) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	list = ((CTRL_LIST_DATA *)(pCtrl->CtrlData));
	if (index == CURITEM_INDEX) {
		index = list->currentItem;
	}
	if (index > list->totalItem - 1) {
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_OUT_RANGE_MSG);
		return ;
	}
	item = list->item[index];
	switch (attribute) {
	case LSTITM_STRID:
		item->stringID = value;
		break;
	case LSTITM_ICONID:
		item->iconID = value;
		break;
	case LSTITM_STATUS:
		item->StatusFlag = value;
		//if set disable and hide item,need to update parent
		UxCtrl_SetDirty(pCtrl->pParent, TRUE);
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}

UINT32 UxList_GetItemData(VControl *pCtrl, UINT32 index, LSTITM_DATA_SET attribute)
{
	CTRL_LIST_DATA *list ;
	CTRL_LIST_ITEM_DATA *item;

	if (pCtrl->wType != CTRL_LIST) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}

	list = ((CTRL_LIST_DATA *)(pCtrl->CtrlData));
	if (index == CURITEM_INDEX) {
		index = list->currentItem;
	}

	if (index > list->totalItem - 1) {
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_OUT_RANGE_MSG);
		return ERR_OUT_RANGE;
	}
	item = list->item[index];

	switch (attribute) {
	case LSTITM_STRID:
		return item->stringID;
	case LSTITM_ICONID:
		return item->iconID;
	case LSTITM_STATUS:
		return item->StatusFlag;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}

void UxList_SetData(VControl *pCtrl, LST_DATA_SET attribute, UINT32 value)
{
	CTRL_LIST_DATA *list ;
	if (pCtrl->wType != CTRL_LIST) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	list = ((CTRL_LIST_DATA *)(pCtrl->CtrlData));

	switch (attribute) {
	case LST_CURITM:
		list->currentItem = value;
		break;
	case LST_TOTITM: {
			UINT32 total_alloc = (list->reserved & LIST_REV_TOTITM_MASK);
			if (value > total_alloc) {
				DBG_ERR("%d > alloc %d\r\n", value, total_alloc);
				return ;
			}
			list->totalItem = value;
		}
		break;
	case LST_PAGEITEM:
		list->pageItem = value;
		break;
	case LST_STYLE:
		list->style = value;
		break;
	case LST_ACTION:
		list->action = value;
		break;
	case LST_RESERVED:
		list->reserved = value;
		break;
	case LST_EVENT:
		list->pExeEvent = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxList_GetData(VControl *pCtrl, LST_DATA_SET attribute)
{
	CTRL_LIST_DATA *list ;
	if (pCtrl->wType != CTRL_LIST) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	list = ((CTRL_LIST_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case LST_CURITM:
		return list->currentItem;
	case LST_TOTITM:
		return list->totalItem;
	case LST_PAGEITEM:
		return list->pageItem;
	case LST_STYLE:
		return list->style;
	case LST_ACTION:
		return list->action;
	case LST_RESERVED:
		return list->reserved;
	case LST_EVENT:
		return list->pExeEvent;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}


////////////////////////////////////////////////////////////////////////////////
//       Default List Cmd
////////////////////////////////////////////////////////////////////////////////


static INT32 UxList_OnPrevItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;

	if ((list->style & LIST_DISABLE_SCRL_MASK) & LIST_DISABLE_SKIP) {
		//#NT#2010/02/18#Lincy Lin -begin
		//#NT#Fix all status disable will cause infinite loop
		/*
		do
		{
		    if(list->currentItem == 0)
		    {
		        list->currentItem = list->totalItem -1;
		    }
		    else
		    {
		        list->currentItem--;
		    }
		    DBG_IND("1 list->currentItem = %d\r\n",list->currentItem);
		} //check if current item is disalbe,if disable get next item
		while(list->item[list->currentItem]->StatusFlag == STATUS_DISABLE);
		*/
		UINT32 i = 0;
		for (i = 0; i < list->totalItem ; i++) {
			if (list->currentItem == 0) {
				list->currentItem = list->totalItem - 1;
			} else {
				list->currentItem--;
			}
			//check if current item is disalbe,if disable get next item
			if (list->item[list->currentItem]->StatusFlag != STATUS_DISABLE) {
				break;
			}
		}
		//#NT#2010/02/18#Lincy Lin -end
	} else { //user can stay on disable item
		if (list->currentItem == 0) {
			if ((list->style & LIST_SCROLL_END_MASK) & LIST_SCROLL_CYCLE) {
				list->currentItem = list->totalItem - 1;
			}
		} else {
			list->currentItem--;
		}
	}

	list->action =  NVTEVT_PREVIOUS_ITEM;
	//some list bg is in the parent dirty list background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	return NVTEVT_CONSUME;
}
static INT32 UxList_OnNextItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;

	if ((list->style & LIST_DISABLE_SCRL_MASK) & LIST_DISABLE_SKIP) {
		UINT32 i = 0;
		for (i = 0; i < list->totalItem ; i++) {
			if (list->currentItem == list->totalItem - 1) {
				list->currentItem = 0;
			} else {
				list->currentItem++;
			}
			//check if current item is disalbe,if disable get next item
			if (list->item[list->currentItem]->StatusFlag != STATUS_DISABLE) {
				break;
			}
		}
	} else { //user can stay on disable item
		if (list->currentItem == list->totalItem - 1) {
			if ((list->style & LIST_SCROLL_END_MASK) & LIST_SCROLL_CYCLE) {
				list->currentItem = 0;
			}
		} else {
			list->currentItem++;
		}
	}

	list->action = NVTEVT_NEXT_ITEM;
	//some list bg is in the parent dirty list background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxList_OnUnfocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;
	UINT32 i = 0;

	for (i = 0; i < list->totalItem; i++) {
		list->item[i]->StatusFlag = (list->item[i]->StatusFlag & ~STATUS_ENABLE_MASK) | STATUS_DISABLE;
	}
	//some list bg is in the parent dirty list background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);

	return NVTEVT_CONSUME;
}

static INT32 UxList_OnFocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;
	UINT32 i = 0;

	for (i = 0; i < list->totalItem; i++) {
		list->item[i]->StatusFlag = (list->item[i]->StatusFlag & ~STATUS_ENABLE_MASK) | STATUS_ENABLE;
	}
	//some list bg is in the parent dirty list background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	return NVTEVT_CONSUME;
}
// list on press would send event to parent event table to search exe function
static INT32 UxList_OnPressItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA *list = ((CTRL_LIST_DATA *)(pCtrl->CtrlData));
	INT32 ret = NVTEVT_PASS;

	//Auto trigger event
	list->action = NVTEVT_PRESS_ITEM;
	if (list->pExeEvent) {
		ret = Ux_SendEvent(0, list->pExeEvent, 0);
	}

	return ret;
}

static INT32 UxList_OnChangeState(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	DBG_IND("default Ux_StateOnChange \r\n");
	return NVTEVT_CONSUME;

}
static INT32 UxList_OnPress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA  *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		list->currentItem = UxList_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	UxCtrl_SetLock(pCtrl, TRUE);

	return NVTEVT_CONSUME;

}
static INT32 UxList_OnMove(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA  *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;

	if (paramNum == 2) {
		UINT32 newCurItem = UxList_FindItem(pCtrl, paramArray[0], paramArray[1]);
		if (list->currentItem  != newCurItem) {
			list->currentItem  = newCurItem;
			UxCtrl_SetDirty(pCtrl->pParent, TRUE);
		}
	}
	return NVTEVT_CONSUME;

}
static INT32 UxList_OnRelease(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA  *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		list->currentItem = UxList_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	UxCtrl_SetLock(pCtrl, FALSE);
	return NVTEVT_CONSUME;

}
#if 0 //not use now
static INT32 UxList_OnClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_LIST_DATA  *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;

	list->currentItem = UxList_FindItem(pCtrl, paramArray[0], paramArray[1]);
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);

	return NVTEVT_CONSUME;
}
#endif

//depend on status change showobj property,if showObj not assign status
//would only draw showob, not change staus
static INT32 UxList_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UIScreen ScreenObj = *paramArray;
	CTRL_LIST_DATA  *list = (CTRL_LIST_DATA *)pCtrl->CtrlData;
	CTRL_LIST_ITEM_DATA *item;
	ITEM_BASE     *pStatusGroup = NULL;
	UINT32 itemIndex = 0, itemPosIndex = 0, shiftPosX = 0, shiftPosY = 0;
	UINT32 uiStatus, StatusShowTableIndex, rowIndex;
	UINT32 uiStartIndex = 0, uiEndIndex = 0;
	IPOINT orgPos = Ux_GetOrigin(ScreenObj);
	UINT32 totalRow = 0;
	//if  current item disable,move to next item
	if ((list->item[list->currentItem]->StatusFlag == STATUS_DISABLE)  &&
		((list->style & LIST_DISABLE_SCRL_MASK) & LIST_DISABLE_SKIP)) {
		UxList_OnNextItem(pCtrl, paramNum, paramArray);
	}

	//calculate next item shift position
	shiftPosX = pCtrl->rect.x2 - pCtrl->rect.x1;
	shiftPosY = pCtrl->rect.y2 - pCtrl->rect.y1;

	if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_ARRAY) {
		totalRow = ((list->totalItem - 1) / list->pageItem) + 1;
	} else {
		totalRow = 1;
	}

	for (rowIndex = 0; rowIndex < totalRow; rowIndex++) {
		//calculate start and end index
		if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_ARRAY) {
			uiStartIndex = rowIndex * list->pageItem;
			if (uiStartIndex + list->pageItem < list->totalItem) {
				uiEndIndex = uiStartIndex + list->pageItem;
			} else {
				uiEndIndex = list->totalItem;
			}
		} else {
			if ((list->style & LIST_SCROLL_MASK) & LIST_SCROLL_NEXT_ITEM) {
				UxList_ScrollOneItemCal(list, &uiStartIndex, &uiEndIndex);
			} else {
				UxList_ScrollOnePageCal(list, &uiStartIndex, &uiEndIndex);
			}

		}

		//draw one page itmes
		for (itemIndex = uiStartIndex, itemPosIndex = 0; itemIndex < uiEndIndex; itemIndex++, itemPosIndex++) {
			//1.set list item shift position
			if ((list->style & LIST_LAYOUT_MASK) == LIST_LAYOUT_VERTICAL) {
				Ux_SetOrigin(ScreenObj, (LVALUE)(orgPos.x + rowIndex * shiftPosX), (LVALUE)(orgPos.y + itemPosIndex * shiftPosY));
			} else { //LIST_LAYOUT_HORIZONTAL  and LIST_LAYOUT_ARRAY
				Ux_SetOrigin(ScreenObj, (LVALUE)(orgPos.x + itemPosIndex * shiftPosX), (LVALUE)(orgPos.y + rowIndex * shiftPosY));
			}

			//2.get really visible list item
			item = list->item[itemIndex];
			if ((list->style & LIST_DISABLE_MASK) & LIST_DISABLE_HIDE) {
				//check if current item is disalbe,if disable get next item
				while (item->StatusFlag == STATUS_DISABLE) {
					item = list->item[++itemIndex];
					if (itemIndex == list->totalItem) {
						Ux_SetOrigin(ScreenObj, (LVALUE)orgPos.x, (LVALUE)orgPos.y);
						return NVTEVT_CONSUME;
					}
				}
			}

			//3.according to current list item status get corresponding show table
			uiStatus = item->StatusFlag;
			if ((list->style & LIST_DISABLE_SCRL_MASK) & LIST_DISABLE_SKIP) {
				//in skip mode currentItem would not be disable,except all item are disable
				if ((itemIndex == list->currentItem) && (uiStatus != STATUS_DISABLE)) {
					uiStatus |= STATUS_FOCUS_BIT;
				}
			} else {
				if (itemIndex == list->currentItem) {
					uiStatus |= STATUS_FOCUS_BIT;
				}
			}
			StatusShowTableIndex = UxList_GetStatusShowTableIndex(uiStatus);

			if (StatusShowTableIndex >= STATUS_SETTIMG_MAX) {
				DBG_ERR("sta err %d\r\n", StatusShowTableIndex);
				return NVTEVT_CONSUME;
			}

			if ((UxCtrl_GetLock() == pCtrl) && (StatusShowTableIndex == STATUS_FOCUS)) {
				StatusShowTableIndex = STATUS_FOCUS_PRESS;
			}

			pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[StatusShowTableIndex];
			//4.draw all show obj of this status(any status show table must be a show obj of goup type)
			if ((list->style & LIST_DRAW_MASK) & LIST_DRAW_IMAGE_LIST) {
				Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, item->iconID + StatusShowTableIndex, 0);
			} else if ((list->style & LIST_DRAW_MASK) & LIST_DRAW_IMAGE_TABLE) {
				UINT32 *iconTable = pCtrl->usrData;
				if (iconTable) {
					Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, *(iconTable + itemIndex * STATUS_SETTIMG_MAX + StatusShowTableIndex), 0);
				} else {
					DBG_ERR("MENU_DRAW_IMAGE_TABLE ,plz assign table");
				}
			} else {
				Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, item->iconID, 0);
			}
		}
	}
	Ux_SetOrigin(ScreenObj, (LVALUE)orgPos.x, (LVALUE)orgPos.y);

	return NVTEVT_CONSUME;
}


EVENT_ENTRY DefaultListCmdMap[] = {
	{NVTEVT_PREVIOUS_ITEM,          UxList_OnPrevItem},
	{NVTEVT_NEXT_ITEM,              UxList_OnNextItem},
	{NVTEVT_UNFOCUS,                UxList_OnUnfocus},
	{NVTEVT_FOCUS,                  UxList_OnFocus},
	{NVTEVT_PRESS_ITEM,             UxList_OnPressItem},
	{NVTEVT_CHANGE_STATE,           UxList_OnChangeState},
	{NVTEVT_PRESS,                  UxList_OnPress},
	{NVTEVT_MOVE,                   UxList_OnMove},
	{NVTEVT_RELEASE,                UxList_OnRelease},
	{NVTEVT_REDRAW,                 UxList_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxListCtrl = {
	"UxListCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultListCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

