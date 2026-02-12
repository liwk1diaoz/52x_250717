#include "UIControlInternal.h"
#include "UIControl/UIControl.h"
#include "UIControl/UIControlExt.h"
#include "UIControl/UIControlEvent.h"
#include "UIControl/UICtrlMenuLib.h"
static PUSERSCROLL_FUNC p_user_scroll= 0;

/////////////////////////////////////////////////////////////////////////////
//        API
////////////////////////////////////////////////////////////////////////////////
static UINT32 UxMenu_GetStatusShowTableIndex(UINT32 status)
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
// for startIndex forward/backwark calculate enable item until page number
// retrun pStartIndex and pEndIndex
static UINT32 UxMenu_ScrollCalIndex(CTRL_MENU_DATA *menu, UINT32 *pStartIndex, UINT32 *pEndIndex,UINT32 startIndex,UINT32 bforward)
{
	UINT32 scrollStartIndex = 0;
	UINT32 scrollEndIndex = 0;
    INT32 k=0;
    UINT32 enableCount=0;

    DBG_IND("startIndex %d,bforward %d\r\n", startIndex,bforward);

    if(bforward) {
		//scoll to pre item,keep bar in the front
		scrollStartIndex = startIndex;
        for(k=(INT32)scrollStartIndex;k<(INT32)menu->totalItem;k++) {
            if (menu->item[k]->StatusFlag == STATUS_ENABLE) {
			    enableCount++;
                if(enableCount==menu->pageItem){
                    break;
                }
            }
        }
        if(enableCount==menu->pageItem)
    	    scrollEndIndex = k;
        else
    	    scrollEndIndex = menu->totalItem-1;
    } else {
		//scoll to next item,keep bar in the end
		scrollEndIndex = startIndex;
        for(k=(INT32)scrollEndIndex;k>=0;k--) {
            if (menu->item[k]->StatusFlag == STATUS_ENABLE) {
			    enableCount++;
                if(enableCount==menu->pageItem) {
                    break;
                }
            }
        }
        if(enableCount==menu->pageItem)
    	    scrollStartIndex = k;
        else
    	    scrollStartIndex = 0;
	}


	*pStartIndex = scrollStartIndex;
	*pEndIndex = scrollEndIndex;

    return enableCount;
}

//calculate start index and end index
static void UxMenu_ScrollOneItemCal(CTRL_MENU_DATA *menu, UINT32 *pStartIndex, UINT32 *pEndIndex)
{
    UINT32 enableCount=0;
	UINT32 scrollStartIndex = 0;
	UINT32 scrollEndIndex ;

    if ((menu->action != NVTEVT_NEXT_ITEM)&&(menu->currentItem==menu->totalItem-1)) {
        DBG_IND("from tail\r\n");
        UxMenu_ScrollCalIndex(menu,&scrollStartIndex,&scrollEndIndex,menu->totalItem-1,0);
    } else if ((menu->action != NVTEVT_PREVIOUS_ITEM)&&(menu->currentItem==0)) {
        DBG_IND("from head\r\n");
        UxMenu_ScrollCalIndex(menu,&scrollStartIndex,&scrollEndIndex,0,1);
    } else {

        enableCount=UxMenu_ScrollCalIndex(menu,&scrollStartIndex,&scrollEndIndex,menu->currentItem,0);

        if(enableCount<menu->pageItem){
            DBG_IND("first page enableCount %d\r\n",enableCount);
            UxMenu_ScrollCalIndex(menu,&scrollStartIndex,&scrollEndIndex,0,1);
        }
    }
	*pStartIndex = scrollStartIndex;
	*pEndIndex = scrollEndIndex;

    DBG_IND("start %d end %d currentItem %d\r\n",*pStartIndex,*pEndIndex,menu->currentItem);
	menu->action = 0;  //clear action flag every time
}
#if 0
//keep current in last
static void UxMenu_ScrollOneItemCal_2(CTRL_MENU_DATA *menu, UINT32 *pStartIndex, UINT32 *pEndIndex)
{
    UINT32 enableCount=0;
	UINT32 scrollStartIndex = 0;
	UINT32 scrollEndIndex ;

    if ((menu->action != NVTEVT_NEXT_ITEM)&&(menu->currentItem==menu->totalItem-1)) {
        DBG_IND("from tail\r\n");
        UxMenu_ScrollCalIndex(menu,&scrollStartIndex,&scrollEndIndex,menu->totalItem-1,0);
    } else if ((menu->action != NVTEVT_PREVIOUS_ITEM)&&(menu->currentItem==0)) {
        DBG_IND("from head\r\n");
        UxMenu_ScrollCalIndex(menu,&scrollStartIndex,&scrollEndIndex,0,1);
    } else {

        enableCount=UxMenu_ScrollCalIndex(menu,&scrollStartIndex,&scrollEndIndex,menu->currentItem,1);

        if(enableCount<menu->pageItem){
            DBG_IND("last page enableCount %d\r\n",enableCount);
            UxMenu_ScrollCalIndex(menu,&scrollStartIndex,&scrollEndIndex,menu->totalItem-1,0);
        }
    }
	*pStartIndex = scrollStartIndex;
	*pEndIndex = scrollEndIndex;

    DBG_IND("start %d end %d currentItem %d\r\n",*pStartIndex,*pEndIndex,menu->currentItem);
	menu->action = 0;  //clear action flag every time
}
#endif

//calculate start index and end index
static void UxMenu_ScrollOnePageCal(CTRL_MENU_DATA *menu, UINT32 *pStartIndex, UINT32 *pEndIndex)
{
	UINT32 curItem = menu->currentItem ;
	*pEndIndex = menu->pageItem;

	if ((menu->style & MENU_DISABLE_MASK) & MENU_DISABLE_HIDE) {
		UINT32 index = 0;
		UINT32 enableCount = 0;
		if (menu->pageItem == 1) {
			*pStartIndex = menu->currentItem;
		} else {
			for (index = 0; index <= menu->currentItem; index++) {
				if (menu->item[index]->StatusFlag == STATUS_ENABLE) {
					enableCount++;
					if ((enableCount % menu->pageItem) == 1) {
						*pStartIndex = index;
					}

				}
			}
		}
		*pEndIndex = *pStartIndex + menu->pageItem;


	} else { //MENU_DISABLE_SHOW
		//scroll one page once
		if (curItem >= menu->pageItem) {
			UINT32 uiPage = 0;
			uiPage = menu->currentItem / menu->pageItem ;
			*pStartIndex = menu->pageItem * uiPage;
			*pEndIndex = menu->pageItem * (uiPage + 1);
		}
	}


	if (*pEndIndex > menu->totalItem) {
		*pEndIndex = menu->totalItem;
	}

}
static void UxMenu_ScrollCenterCal(CTRL_MENU_DATA *menu, UINT32 *pStartIndex, UINT32 *pEndIndex)
{

	if (menu->currentItem < menu->pageItem / 2) {
		*pStartIndex = menu->currentItem - menu->pageItem / 2 + menu->totalItem;
	} else {
		*pStartIndex = menu->currentItem - menu->pageItem / 2;
	}

	if (menu->currentItem + menu->pageItem / 2 < menu->totalItem) {
		*pEndIndex = menu->currentItem + menu->pageItem / 2;
	} else {
		*pEndIndex = menu->currentItem + menu->pageItem / 2 - menu->totalItem;
	}
}




static UINT32 UxMenu_FindItem(VControl *pCtrl, UINT32 x, UINT32 y)
{
	CTRL_MENU_DATA  *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	UINT32 itemIndex = 0, itemPosIndex = 0, shiftPos = 0, Postion = 0;
	UINT32 uiStartIndex = 0, uiEndIndex = 0;
	UINT32 bFound = FALSE;

	//0.calculate start and end index
	if ((menu->style & MENU_SCROLL_MASK) & MENU_SCROLL_NEXT_ITEM) {
		UxMenu_ScrollOneItemCal(menu, &uiStartIndex, &uiEndIndex);
	} else if ((menu->style & MENU_SCROLL_MASK) & MENU_SCROLL_CENTER) {
		UxMenu_ScrollCenterCal(menu, &uiStartIndex, &uiEndIndex);
	} else {
		UxMenu_ScrollOnePageCal(menu, &uiStartIndex, &uiEndIndex);
	}

	//calculate next item shift position
	if ((menu->style & MENU_LAYOUT_MASK) & MENU_LAYOUT_HORIZONTAL) {
		shiftPos = pCtrl->rect.x2 - pCtrl->rect.x1;
		Postion = x - pCtrl->rect.x1;
	} else {
		shiftPos = pCtrl->rect.y2 - pCtrl->rect.y1;
		Postion = y - pCtrl->rect.y1;
	}
	for (itemIndex = uiStartIndex, itemPosIndex = 0; itemPosIndex < menu->pageItem; itemPosIndex++) {
		//disable hide,need to check disable item
		if ((menu->style & MENU_DISABLE_MASK) & MENU_DISABLE_HIDE) {
			//check if current item is disalbe,if disable get next item
			CTRL_MENU_ITEM_DATA *item;
			item = menu->item[itemIndex];
			while (item->StatusFlag == STATUS_DISABLE) {
				item = menu->item[++itemIndex];
				if (itemIndex == menu->totalItem) {
					break;
				}
			}
		}

		DBG_IND("itemIndex %d %d\r\n", itemIndex, itemPosIndex);
		if (itemIndex == menu->totalItem) {
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
		return menu->currentItem;
	}

	return itemIndex;

}
void UxMenu_GetRange(VControl *pCtrl, Ux_RECT *pRange)
{
	CTRL_MENU_DATA *menu ;
	UINT32 rangeItemNum = 0;

	if (pCtrl->wType != CTRL_MENU) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	menu = ((CTRL_MENU_DATA *)(pCtrl->CtrlData));

	//use relative menu rect
	pRange->x1 = pCtrl->rect.x1;
	pRange->y1 = pCtrl->rect.y1;

	if (menu->totalItem > menu->pageItem) {
		rangeItemNum = menu->pageItem;
	} else {
		rangeItemNum = menu->totalItem;
	}

	if ((menu->style & MENU_LAYOUT_MASK) & MENU_LAYOUT_HORIZONTAL) {
		pRange->x2 = pRange->x1 + (pCtrl->rect.x2 - pCtrl->rect.x1) * rangeItemNum;
		pRange->y2 = pRange->y1 + (pCtrl->rect.y2 - pCtrl->rect.y1);
	} else {
		pRange->x2 = pRange->x1 + (pCtrl->rect.x2 - pCtrl->rect.x1);
		pRange->y2 = pRange->y1 + (pCtrl->rect.y2 - pCtrl->rect.y1) * rangeItemNum;
	}

	DBG_IND("x1  %d,x2 %d  y1 %d,y2 %d \r\n ", pRange->x1, pRange->x2, pRange->y1, pRange->y2);

}

void UxMenu_GetItemPos(VControl *pCtrl, UINT32 index, Ux_RECT *pRect)
{
	CTRL_MENU_DATA *menu ;
	UINT32 i = 0, count = 0;
	UINT32 OneItemRange = 0;
	UINT32 pageNum = 0;
	if (pCtrl->wType != CTRL_MENU) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}

	if (!pRect) {
		return ;
	}

	menu = ((CTRL_MENU_DATA *)(pCtrl->CtrlData));
	if (index == CURITEM_INDEX) {
		index = menu->currentItem;
	}
	pageNum = menu->pageItem;
	//disable hide,need to check disable item
	if ((menu->style & MENU_DISABLE_MASK) & MENU_DISABLE_HIDE) {
		for (i = 0; i < index ; i++) {
			if (UxMenu_GetItemData(pCtrl, i, MNUITM_STATUS) == STATUS_DISABLE) {
				count++;
			}
		}
	}
	DBG_IND("count %d\r\n", count);

	pRect->x1 = pCtrl->rect.x1;
	pRect->y1 = pCtrl->rect.y1;
	pRect->x2 = pCtrl->rect.x2;
	pRect->y1 = pCtrl->rect.y1;

	index = index % pageNum;
	if ((menu->style & MENU_LAYOUT_MASK) & MENU_LAYOUT_HORIZONTAL) {
		OneItemRange = pCtrl->rect.x2 - pCtrl->rect.x1;
		pRect->x1 = pCtrl->rect.x1 + OneItemRange * (index - count);
		pRect->x2 = pRect->x1 + OneItemRange;
	} else {
		OneItemRange = pCtrl->rect.y2 - pCtrl->rect.y1;
		pRect->y1 = pCtrl->rect.y1 + OneItemRange * (index - count);
		pRect->y2 = pRect->y1 + OneItemRange;
	}

	DBG_IND(" %d %d %d %d \r\n", pRect->x1, pRect->x2, pRect->y1, pRect->y2);
}
////////////////////////////////////////////////////////////////////////////////
//      API called by users
////////////////////////////////////////////////////////////////////////////////
void UxMenu_SetItemData(VControl *pCtrl, UINT32 index, MNUITM_DATA_SET attribute, UINT32 value)
{
	CTRL_MENU_DATA *menu ;
	CTRL_MENU_ITEM_DATA *item;

	if (pCtrl->wType != CTRL_MENU) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}

	menu = ((CTRL_MENU_DATA *)(pCtrl->CtrlData));
	if (index == CURITEM_INDEX) {
		index = menu->currentItem;
	}
	if (index > menu->totalItem - 1) {
		DBG_ERR("%s %d %s 0~%d  \r\n", pCtrl->Name, index, ERR_OUT_RANGE_MSG, menu->totalItem - 1);
		return ;
	}
	item = menu->item[index];
	switch (attribute) {
	case MNUITM_STRID:
		item->stringID = value;
		break;
	case MNUITM_ICONID:
		item->iconID = value;
		break;
	case MNUITM_STATUS:
		item->StatusFlag = value;
		//if set disable and hide item,need to update parent
		UxCtrl_SetDirty(pCtrl->pParent, TRUE);
		break;
	case MNUITM_EVENT:
		item->pExeEvent = value;
		break;
	case MNUITM_CTRL:
		item->pBindCtrl = (VControl *)value;
		break;
	case MNUITM_VALUE:
		item->value = value;
		break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}

UINT32 UxMenu_GetItemData(VControl *pCtrl, UINT32 index, MNUITM_DATA_SET attribute)
{
	CTRL_MENU_DATA *menu ;
	CTRL_MENU_ITEM_DATA *item;

	if (pCtrl->wType != CTRL_MENU) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	menu = ((CTRL_MENU_DATA *)(pCtrl->CtrlData));
	if (index == CURITEM_INDEX) {
		index = menu->currentItem;
	}
	if (index > menu->totalItem - 1) {
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_OUT_RANGE_MSG);
		return ERR_OUT_RANGE;
	}
	item = menu->item[index];
	switch (attribute) {
	case MNUITM_STRID:
		return item->stringID;
	case MNUITM_ICONID:
		return item->iconID;
	case MNUITM_STATUS:
		return item->StatusFlag;
	case MNUITM_EVENT:
		return item->pExeEvent;
	case MNUITM_CTRL:
		return (UINT32)item->pBindCtrl;
	case MNUITM_VALUE:
		return item->value;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}

void UxMenu_SetData(VControl *pCtrl, MNU_DATA_SET attribute, UINT32 value)
{
	CTRL_MENU_DATA *menu ;
	if (pCtrl->wType != CTRL_MENU) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ;
	}
	menu = ((CTRL_MENU_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case MNU_CURITM:
		menu->currentItem = value;
		break;
	case MNU_TOTITM: {
			UINT32 total_alloc = (menu->reserved & MENU_REV_TOTITM_MASK);
			if (value > total_alloc) {
				DBG_ERR("%d > alloc %d\r\n", value, total_alloc);
				return ;
			}
			menu->totalItem = value;
		}
		break;
	case MNU_PAGEITEM:
		menu->pageItem = value;
		break;
	case MNU_STYLE:
		menu->style = value;
		break;
	case MNU_ACTION:
		menu->action = value;
		break;
	case MNU_RESERVED:
		menu->reserved = value;
		break;
    case MNU_USERCB:
        p_user_scroll = (PUSERSCROLL_FUNC)value;
        break;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		break;
	}
	UxCtrl_SetDirty(pCtrl,  TRUE);

}
UINT32 UxMenu_GetData(VControl *pCtrl, MNU_DATA_SET attribute)
{
	CTRL_MENU_DATA *menu ;
	if (pCtrl->wType != CTRL_MENU) {
		DBG_ERR("%s %s %d\r\n", pCtrl->Name, ERR_TYPE_MSG, pCtrl->wType);
		return ERR_TYPE;
	}
	menu = ((CTRL_MENU_DATA *)(pCtrl->CtrlData));
	switch (attribute) {
	case MNU_CURITM:
		return menu->currentItem;
	case MNU_TOTITM:
		return menu->totalItem;
	case MNU_PAGEITEM:
		return menu->pageItem;
	case MNU_STYLE:
		return menu->style;
	case MNU_ACTION:
		return menu->action;
	case MNU_RESERVED:
		return menu->reserved;
	default:
		DBG_ERR("%s %s\r\n", pCtrl->Name, ERR_ATTRIBUTE_MSG);
		return ERR_ATTRIBUTE;
	}

}


////////////////////////////////////////////////////////////////////////////////
//       Default Menu Cmd
////////////////////////////////////////////////////////////////////////////////


static INT32 UxMenu_OnPrevItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;

#if 0  //not change preitem behavior,user can not undersatand
	if ((pCtrl->pParent->wType == CTRL_TAB) && (menu->currentItem == 0)) {
		Ux_SendEvent(pCtrl->pParent, NVTEVT_FIRST_ITEM, 0);
		return NVTEVT_CONSUME;
	}

	/*
	if(item->pBindCtrl)
	    Ux_SendEvent(item->pBindCtrl,NVTEVT_UNFOCUS,0);
	*/
#endif
	if ((menu->style & MENU_DISABLE_SCRL_MASK) & MENU_DISABLE_SKIP) {
		UINT32 i = 0;
		for (i = 0; i < menu->totalItem ; i++) {
    		if (menu->currentItem == 0) {
    			if ((menu->style & MENU_SCROLL_END_MASK) & MENU_SCROLL_CYCLE) {
    				menu->currentItem = menu->totalItem - 1;
    			}
    		} else {
				menu->currentItem--;
			}
			//check if current item is disalbe,if disable get next item
			if (menu->item[menu->currentItem]->StatusFlag != STATUS_DISABLE) {
				break;
			}
		}
	} else { //user can stay on disable item
		if (menu->currentItem == 0) {
			if ((menu->style & MENU_SCROLL_END_MASK) & MENU_SCROLL_CYCLE) {
				menu->currentItem = menu->totalItem - 1;
			}
		} else {
			menu->currentItem--;
		}
	}

	/*
	if(item->pBindCtrl)
	    Ux_SendEvent(item->pBindCtrl,NVTEVT_FOCUS,0);
	*/
	menu->action =  NVTEVT_PREVIOUS_ITEM;
	//some menu bg is in the parent dirty menu background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	return NVTEVT_CONSUME;
}
static INT32 UxMenu_OnNextItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;

	if ((pCtrl->pParent->wType == CTRL_TAB) && (menu->currentItem == menu->totalItem - 1)) {
		Ux_SendEvent(pCtrl->pParent, NVTEVT_LAST_ITEM, 0);
		return NVTEVT_CONSUME;
	}
	/*
	if(item->pBindCtrl)
	    Ux_SendEvent(item->pBindCtrl,NVTEVT_UNFOCUS,0);
	*/
	if ((menu->style & MENU_DISABLE_SCRL_MASK) & MENU_DISABLE_SKIP) {
		UINT32 i = 0;
		for (i = 0; i < menu->totalItem ; i++) {
			if (menu->currentItem == menu->totalItem - 1) {
    			if ((menu->style & MENU_SCROLL_END_MASK) & MENU_SCROLL_CYCLE) {
    				menu->currentItem = 0;
    			}
			} else {
				menu->currentItem++;
			}
			//check if current item is disalbe,if disable get next item
			if (menu->item[menu->currentItem]->StatusFlag != STATUS_DISABLE) {
				break;
			}
		}
	} else { //user can stay on disable item
		if (menu->currentItem == menu->totalItem - 1) {
			if ((menu->style & MENU_SCROLL_END_MASK) & MENU_SCROLL_CYCLE) {
				menu->currentItem = 0;
			}
		} else {
			menu->currentItem++;
		}
	}
	/*
	if(item->pBindCtrl)
	    Ux_SendEvent(item->pBindCtrl,NVTEVT_FOCUS,0);
	*/
	menu->action = NVTEVT_NEXT_ITEM;
	//some menu bg is in the parent dirty menu background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	return NVTEVT_CONSUME;
}

static INT32 UxMenu_OnUnfocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	UINT32 i = 0;

	for (i = 0; i < menu->totalItem; i++) {
		menu->item[i]->StatusFlag = (menu->item[i]->StatusFlag & ~STATUS_ENABLE_MASK) | STATUS_DISABLE;
	}
	//some menu bg is in the parent dirty menu background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);

	return NVTEVT_CONSUME;
}

static INT32 UxMenu_OnFocus(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	UINT32 i = 0;

	for (i = 0; i < menu->totalItem; i++) {
		menu->item[i]->StatusFlag = (menu->item[i]->StatusFlag & ~STATUS_ENABLE_MASK) | STATUS_ENABLE;
	}
	//some menu bg is in the parent dirty menu background,ex:2nd page,only 3item
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	return NVTEVT_CONSUME;
}
// menu on press would send event to currnet event table to search exe function
static INT32 UxMenu_OnPressItem(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	CTRL_MENU_ITEM_DATA *item = menu->item[menu->currentItem];
	INT32 ret = NVTEVT_CONSUME;

	//Auto trigger event
	menu->action = NVTEVT_PRESS_ITEM;
	if (item->pExeEvent) {
		ret = Ux_SendEvent(0, item->pExeEvent, 0);
	}
	return ret;

}

static INT32 UxMenu_OnChangeState(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	DBG_IND("default Ux_StateOnChange \r\n");
	return NVTEVT_CONSUME;

}

static INT32 UxMenu_OnPress(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA  *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		menu->currentItem = UxMenu_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	UxCtrl_SetLock(pCtrl, TRUE);
	return NVTEVT_CONSUME;

}
static INT32 UxMenu_OnMove(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA  *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		UINT32 newCurItem = UxMenu_FindItem(pCtrl, paramArray[0], paramArray[1]);
		if (menu->currentItem  != newCurItem) {
			menu->currentItem  = newCurItem;
			UxCtrl_SetDirty(pCtrl->pParent, TRUE);
		}
	}
	return NVTEVT_CONSUME;

}
static INT32 UxMenu_OnRelease(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA  *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		menu->currentItem = UxMenu_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);
	UxCtrl_SetLock(pCtrl, FALSE);

	return NVTEVT_CONSUME;

}
static INT32 UxMenu_OnClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	CTRL_MENU_DATA  *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	if (paramNum == 2) {
		menu->currentItem = UxMenu_FindItem(pCtrl, paramArray[0], paramArray[1]);
	}
	UxCtrl_SetDirty(pCtrl->pParent, TRUE);

	return NVTEVT_CONSUME;
}


//depend on status change showobj property,if showObj not assign status
//would only draw showob, not change staus
static INT32 UxMenu_OnDraw(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UIScreen ScreenObj = *paramArray;
	CTRL_MENU_DATA  *menu = (CTRL_MENU_DATA *)pCtrl->CtrlData;
	CTRL_MENU_ITEM_DATA *item;
	ITEM_BASE     *pStatusGroup = NULL;
	UINT32 itemIndex = 0, itemPosIndex = 0, shiftPos = 0;
	UINT32 uiStatus, StatusShowTableIndex;
	UINT32 uiStartIndex = 0, uiEndIndex = 0;
	IPOINT orgPos = Ux_GetOrigin(ScreenObj);
    PALETTE_ITEM* orgFontColorTbl =0;

	//if  current item disable,move to next item
	if ((menu->item[menu->currentItem]->StatusFlag == STATUS_DISABLE)  &&
		((menu->style & MENU_DISABLE_SCRL_MASK) & MENU_DISABLE_SKIP)) {
		DBG_IND("%s cur %d is disable\r\n", pCtrl->Name, menu->currentItem);
		UxMenu_OnNextItem(pCtrl, paramNum, paramArray);
	}


	//0.calculate start and end index
	if ((menu->style & MENU_SCROLL_MASK) & MENU_SCROLL_NEXT_ITEM) {
		UxMenu_ScrollOneItemCal(menu, &uiStartIndex, &uiEndIndex);
	} else if ((menu->style & MENU_SCROLL_MASK) & MENU_SCROLL_CENTER) {
		UxMenu_ScrollCenterCal(menu, &uiStartIndex, &uiEndIndex);
	} else if ((menu->style & MENU_SCROLL_MASK) & MENU_SCROLL_USER) {
	    if((p_user_scroll)&&(p_user_scroll->scroll_cal)) {
		    p_user_scroll->scroll_cal(menu, &uiStartIndex, &uiEndIndex);
	    } else {
	        DBG_ERR("no MNU_USERCB \r\n");
	    }
	} else {
		UxMenu_ScrollOnePageCal(menu, &uiStartIndex, &uiEndIndex);
	}

	DBG_IND("tot %d cur %d uiStartIndex  %d ,uiEndIndex %d \r\n", menu->totalItem, menu->currentItem, uiStartIndex, uiEndIndex);

	//calculate next item shift position
	if ((menu->style & MENU_LAYOUT_MASK) & MENU_LAYOUT_HORIZONTAL) {
		shiftPos = pCtrl->rect.x2 - pCtrl->rect.x1;
	} else {
		shiftPos = pCtrl->rect.y2 - pCtrl->rect.y1;
	}

    if((menu->style & MENU_DRAW_MASK) & MENU_TEXTCOLOR_TABLE) {
        orgFontColorTbl = GxGfx_GetTextColorMapping();
    }
	for (itemIndex = uiStartIndex, itemPosIndex = 0; itemPosIndex < menu->pageItem; itemPosIndex++) {
		//1.set menu item shift position
		if ((menu->style & MENU_LAYOUT_MASK) & MENU_LAYOUT_HORIZONTAL) {
			Ux_SetOrigin(ScreenObj, (LVALUE)(orgPos.x + itemPosIndex * shiftPos), (LVALUE)(orgPos.y));
		} else {
			Ux_SetOrigin(ScreenObj, (LVALUE)(orgPos.x), (LVALUE)(orgPos.y + itemPosIndex * shiftPos));
		}

		//2.get really visible menu item
		item = menu->item[itemIndex];
		if ((menu->style & MENU_DISABLE_MASK) & MENU_DISABLE_HIDE) {
			//check if current item is disalbe,if disable get next item
			while (item->StatusFlag == STATUS_DISABLE) {
				item = menu->item[++itemIndex];
				if (itemIndex == menu->totalItem) {
					return NVTEVT_CONSUME;
				}
			}
		}

		//3.according to current menu item status get corresponding show table
		uiStatus = item->StatusFlag;
		if ((menu->style & MENU_DISABLE_SCRL_MASK) & MENU_DISABLE_SKIP) {
			//in skip mode currentItem would not be disable,except all item are disable
			if ((itemIndex == menu->currentItem) && (uiStatus != STATUS_DISABLE)) {
				uiStatus |= STATUS_FOCUS_BIT;
			}
		} else {
			if (itemIndex == menu->currentItem) {
				uiStatus |= STATUS_FOCUS_BIT;
			}

		}
		StatusShowTableIndex = UxMenu_GetStatusShowTableIndex(uiStatus);

		if (StatusShowTableIndex >= STATUS_SETTIMG_MAX) {
			DBG_ERR("sta err %d\r\n", StatusShowTableIndex);
			return NVTEVT_CONSUME;
		}

		if ((UxCtrl_GetLock() == pCtrl) && (StatusShowTableIndex == STATUS_FOCUS)) {
			StatusShowTableIndex = STATUS_FOCUS_PRESS;
		}

		pStatusGroup = (ITEM_BASE *)pCtrl->ShowList[StatusShowTableIndex];
		//4.draw all show obj of this status(any status show table must be a show obj of goup type)
		if ((menu->style & MENU_DRAW_MASK) & MENU_DRAW_IMAGE_LIST) {
			Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, item->iconID + StatusShowTableIndex, item->value);
		} else if ((menu->style & MENU_DRAW_MASK) & MENU_DRAW_IMAGE_TABLE) {
			UINT32 *iconTable = pCtrl->usrData;
			if (iconTable) {
				Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, *(iconTable + itemIndex * STATUS_SETTIMG_MAX + StatusShowTableIndex), item->value);
			} else {
				DBG_ERR("MENU_DRAW_IMAGE_TABLE ,plz assign table");
			}
		} else if ((menu->style & MENU_DRAW_MASK) & MENU_TEXTCOLOR_TABLE) {
            UINT32 **colorTable = pCtrl->usrData;
            if(colorTable)
            {
                GxGfx_SetTextColorMapping((PALETTE_ITEM*)(*(colorTable+StatusShowTableIndex)));
            }

            Ux_DrawItemByStatus(ScreenObj,pStatusGroup,item->stringID,item->iconID,item->value);
        } else {
			Ux_DrawItemByStatus(ScreenObj, pStatusGroup, item->stringID, item->iconID, item->value);
		}

		itemIndex++;
		if (itemIndex == menu->totalItem) {
			if ((menu->style & MENU_SCROLL_MASK) & MENU_SCROLL_CENTER) {
				itemIndex = 0;    //need to cycle from first item
			} else {
				break;    //donot need to draw others
			}
		}
	}

    //after draw menu,set orginal color table
    if((menu->style & MENU_DRAW_MASK) & MENU_TEXTCOLOR_TABLE)
    {
        GxGfx_SetTextColorMapping(orgFontColorTbl);
    }


	return NVTEVT_CONSUME;
}
EVENT_ENTRY DefaultMenuCmdMap[] = {
	{NVTEVT_PREVIOUS_ITEM,          UxMenu_OnPrevItem},
	{NVTEVT_NEXT_ITEM,              UxMenu_OnNextItem},
	{NVTEVT_UNFOCUS,                UxMenu_OnUnfocus},
	{NVTEVT_FOCUS,                  UxMenu_OnFocus},
	{NVTEVT_PRESS_ITEM,             UxMenu_OnPressItem},
	{NVTEVT_CHANGE_STATE,           UxMenu_OnChangeState},
	{NVTEVT_PRESS,                  UxMenu_OnPress},
	{NVTEVT_MOVE,                   UxMenu_OnMove},
	{NVTEVT_RELEASE,                UxMenu_OnRelease},
	{NVTEVT_CLICK,                  UxMenu_OnClick},
	{NVTEVT_REDRAW,                 UxMenu_OnDraw},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

VControl UxMenuCtrl = {
	"UxMenuCtrl",
	CTRL_BASE,
	0,  //child
	0,  //parent
	0,  //user
	DefaultMenuCmdMap, //event-table
	0,  //data-table
	0,  //show-table
	{0, 0, 0, 0},
	0
};

