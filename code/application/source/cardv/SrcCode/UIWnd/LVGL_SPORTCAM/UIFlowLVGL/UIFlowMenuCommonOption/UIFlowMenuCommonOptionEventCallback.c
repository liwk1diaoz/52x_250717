
#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include "UIApp/Network/UIAppNetwork.h"
#include <kwrap/debug.h>

#define PAGE           4

static lv_group_t* gp = NULL;
static lv_obj_t* menu_item = NULL;
static lv_obj_t* label_menu_item = NULL;
static lv_obj_t* label_menu_option = NULL;
static TM_MENU *g_pOptionMenu = 0;

static void set_indev_keypad_group(lv_obj_t* obj)
{
	if(gp == NULL){
		gp = lv_group_create();
		lv_group_add_obj(gp, obj);
	}

	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_group(indev, gp);
}

static void MenuCommonOption_SetCurrentMenu(TM_MENU *pMenu)
{
	g_pOptionMenu = pMenu;
}

static TM_MENU *MenuCommonOption_GetCurrentMenu(void)
{
	return g_pOptionMenu;
}

static void MenuCommonOption_UpdateContent(TM_MENU *pMenu)
{
	TM_PAGE    *pPage;
	TM_ITEM    *pItem;
	TM_OPTION  *pOption;
	UINT32      i;
	UINT16      startIndex = 0;
	UINT16      itemIndex = 0;
	UINT16      SelOption = 0;

	pPage = &pMenu->pPages[pMenu->SelPage];
	pItem = &pPage->pItems[pPage->SelItem];
	SelOption = SysGetFlag(pItem->SysFlag);
	TM_CheckOptionStatus(pMenu, &SelOption, TRUE);
	SysSetFlag(pItem->SysFlag, SelOption); //SelOption might change

	pOption = &pItem->pOptions[SelOption];

	if (pItem->Count) {
		lv_plugin_label_set_text(label_menu_option, pOption->TextId);
		lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);

	} else {
		if (pItem->ItemId == IDM_VERSION) {
			lv_plugin_label_set_text(label_menu_option, LV_PLUGIN_RES_ID_NONE);
			lv_label_set_text(label_menu_option, Prj_GetVersionString());
			lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);

		} else {
			lv_plugin_label_set_text(label_menu_option, LV_PLUGIN_STRING_ID_STRID_NULL_);
			lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);
		}
	}

	lv_plugin_label_set_text(label_menu_item, pItem->TextId);
	lv_plugin_label_update_font(label_menu_item, LV_OBJ_PART_MAIN);

	//find startIndex
	TM_FindStartIndex(pMenu, PAGE, &startIndex);

	//draw item form startIndex
	itemIndex = startIndex;
	for (i = 0; i < PAGE; i++) {
		//check item if disable
		TM_CheckItemStatus(pMenu, &itemIndex, TRUE);
		if ((itemIndex == pPage->Count) && (i < PAGE)) {

			lv_plugin_menu_set_item_string_id(menu_item, i, LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM, pItem->TextId);
			lv_plugin_menu_set_item_img_id(menu_item, i, LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM, pItem->IconId);
			lv_plugin_menu_set_item_hidden(menu_item, i, true);

		} else {
			pItem = &pPage->pItems[itemIndex];

			lv_plugin_menu_set_item_string_id(menu_item, i, LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM, pItem->TextId);
			lv_plugin_menu_set_item_img_id(menu_item, i, LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM, pItem->IconId);
			lv_plugin_menu_set_item_hidden(menu_item, i, false);

			itemIndex++;
		}
	}
}

static void MenuItem_OnOpen(lv_obj_t* obj, TM_MENU *data)
{
	TM_MENU    *pMenu;
	TM_PAGE    *pPage;
	TM_ITEM    *pItem;
	TM_OPTION  *pOption;
	UINT16      SelOption = 0;


		if(NULL == data)
			return;

		MenuCommonOption_SetCurrentMenu(data);
		pMenu = MenuCommonOption_GetCurrentMenu();
		pMenu->Status = TMS_ON_OPTION;
		pMenu->SelPage = 0;

		pPage = &pMenu->pPages[pMenu->SelPage];
		pPage->SelItem = 0;
		//check item if disable
		TM_CheckItemStatus(pMenu, &pPage->SelItem, TRUE);
		pItem = &pPage->pItems[pPage->SelItem];
		if (pItem->Count > 0) {
			SelOption = SysGetFlag(pItem->SysFlag);
			//check option if disable
			TM_CheckOptionStatus(pMenu, &SelOption, TRUE);
			SysSetFlag(pItem->SysFlag, SelOption); //SelOption might change

			pOption = &pItem->pOptions[SelOption];

			lv_plugin_label_set_text(label_menu_option, pOption->TextId);
			lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);
		}

		/* check menu item is init */
		if(!lv_plugin_menu_item_cnt(menu_item)){
			/* allocate menu item */
			lv_plugin_menu_init_items(menu_item, PAGE);
		}

		MenuCommonOption_UpdateContent(pMenu);

		lv_plugin_menu_select_item(menu_item, 0);

        #if _TODO
//		UI_SetDisplayPalette(LAYER_OSD1, 0, 256, gDemoKit_PaletteOption_Palette);
        #endif
//		Ux_DefaultEvent(pCtrl, NVTEVT_OPEN_WINDOW, paramNum, paramArray);

}

static void MenuItem_OnNext(lv_obj_t* obj)
{
	TM_MENU    *pMenu;
	TM_PAGE    *pPage;
//	TM_ITEM    *pItem;


	pMenu = MenuCommonOption_GetCurrentMenu();
	pPage = &pMenu->pPages[pMenu->SelPage];
//	pItem = &pPage->pItems[pPage->SelItem];

	pPage->SelItem++;
	//check item if disable
	TM_CheckItemStatus(pMenu, &pPage->SelItem, TRUE);

	if (pPage->SelItem == pPage->Count) {
		lv_plugin_scr_close(obj, NULL);
	} else {
		MenuCommonOption_UpdateContent(pMenu);
		lv_plugin_menu_select_next_item(menu_item);
	}
}

static void MenuItem_OnPrev(lv_obj_t* obj)
{
	TM_MENU    *pMenu;
	TM_PAGE    *pPage;
//	TM_ITEM    *pItem;


	pMenu = MenuCommonOption_GetCurrentMenu();
	pPage = &pMenu->pPages[pMenu->SelPage];
//	pItem = &pPage->pItems[pPage->SelItem];

	if (pPage->SelItem == 0) {
		// Close current UI Window now
//		Ux_CloseWindow(&MenuCommonOptionCtrl, 2, pItem->ItemId, 0);
		lv_plugin_scr_close(obj, NULL);
	} else {
		pPage->SelItem--;
		//check item if disable
		TM_CheckItemStatus(pMenu, &pPage->SelItem, FALSE);
		if (pPage->SelItem == pPage->Count) {
			// Close current UI Window now
//			Ux_CloseWindow(&MenuCommonOptionCtrl, 2, pItem->ItemId, 0);
			lv_plugin_scr_close(obj, NULL);
		} else {
			MenuCommonOption_UpdateContent(pMenu);
//			MenuCommonOption_UpdatePosition();
//			Ux_SendEvent(pCtrl, NVTEVT_PREVIOUS_ITEM, 0);
			lv_plugin_menu_select_prev_item(menu_item);
		}
	}
}

static void MenuItem_OnSelected(lv_obj_t* obj)
{
	TM_MENU    *pMenu;
	TM_PAGE    *pPage;
	TM_ITEM    *pItem;
	TM_OPTION  *pOption;
	UINT16      SelOption = 0;


		pMenu = MenuCommonOption_GetCurrentMenu();
		pPage = &pMenu->pPages[pMenu->SelPage];
		pItem = &pPage->pItems[pPage->SelItem];


		if (pItem->Count != 0 && pItem->SysFlag != 0) {
			//#NT#2016/09/20#Bob Huang -begin
			//#NT#Support HDMI Display with 3DNR Out
			//Only support FHD p30 size, cannot change size when 3DNR Out enabled
#if (_3DNROUT_FUNC == ENABLE)
			if (pItem->SysFlag == FL_MOVIE_SIZE_MENU && gb3DNROut) {
				Ux_SendEvent(pCtrl, NVTEVT_PRESS, 0);
//				return NVTEVT_CONSUME;
			}
#endif
			//#NT#2016/09/20#Bob Huang -end
			SelOption = SysGetFlag(pItem->SysFlag);
			pMenu->Status = TMS_ON_OPTION;
			SelOption++;
			if (SelOption >= pItem->Count) {
				SelOption = 0;
			}
			TM_CheckOptionStatus(pMenu, &SelOption, TRUE);

			SysSetFlag(pItem->SysFlag, SelOption);
			// notify upper layer the Option had been confirmed
			TM_MENU_CALLBACK(pMenu, TMM_CONFIRM_OPTION, MAKE_LONG(pItem->ItemId, SelOption));

			pOption = &pItem->pOptions[SysGetFlag(pItem->SysFlag)];

			lv_plugin_label_set_text(label_menu_option, pOption->TextId);
			lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);

			if (pOption->TextId){

			}

			if (pItem->ItemId == IDM_LANGUAGE) {

			}

//			Ux_SendEvent(pCtrl, NVTEVT_PRESS, 0);
		} else if (pItem->pOptions != 0) {              // custom process
			pMenu->Status = TMS_ON_CUSTOM;
			TM_ITEM_CALLBACK(pItem, TMM_CONFIRM_OPTION, pItem->ItemId);              // execute custom pPage flow
		}
}

static void UIFlowMenuCommonOption_ScrClose(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);
}

static void UIFlowMenuCommonOption_ChildScrClose(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

	set_indev_keypad_group(obj);

}

static void UIFlowMenuCommonOption_ScrOpen(lv_obj_t* obj, const void *data)
{

	DBG_DUMP("%s\r\n", __func__);

	/***********************************************************************************
	 * Add Menu Screen into group and set group to keypad indev
	 ***********************************************************************************/
//	if(gp == NULL){
//		gp = lv_group_create();
//		lv_group_add_obj(gp, obj);
//	}
//
//	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
//	lv_indev_set_group(indev, gp);
//	lv_group_focus_obj(obj);

	set_indev_keypad_group(obj);


    /***********************************************************************************
     * create a plugin menu, the menu should contains below widgets :
     *
     *  container (parent)
     *
     *  	btn or imgbtn (item1)
     *  		label + img (item1's label and img)
     *
     *  	btn or imgbtn (item2)
     *  		label + img (item2's label and img)
     *
     *  	....
     *
     *	those widgets styles and number of buttons are configured in the builder,
     *	btn's label or img is not mandatory
     *
     **********************************************************************************/
	if(menu_item == NULL){
		menu_item = lv_plugin_menu_create(obj, container_main_menu_scr_uiflowmenucommonoption);
		lv_plugin_menu_set_wrap(menu_item, true);
	}

	if(label_menu_item == NULL)
		label_menu_item = label_menu_item_scr_uiflowmenucommonoption;

	if(label_menu_option == NULL)
		label_menu_option = label_menu_option_scr_uiflowmenucommonoption;

    MenuItem_OnOpen(obj, (TM_MENU *)data);

}

static void UIFlowMenuCommonOption_Key(lv_obj_t* obj, uint32_t key)
{

	switch(key)
	{

	case LV_USER_KEY_NEXT:
	{
		MenuItem_OnNext(obj);
		break;
	}

	case LV_USER_KEY_PREV:
	{
		MenuItem_OnPrev(obj);
		break;
	}

	case LV_USER_KEY_SELECT:
	{
		MenuItem_OnSelected(obj);
		break;
	}

	case LV_USER_KEY_SHUTTER2:
	{
		lv_plugin_scr_close(UIFlowMenuCommonItem, NULL);
		break;
	}


	}

}

void UIFlowMenuCommonOptionEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{
	case LV_PLUGIN_EVENT_SCR_OPEN:
	{
		UIFlowMenuCommonOption_ScrOpen(obj, lv_event_get_data());
		break;
	}

	case LV_PLUGIN_EVENT_SCR_CLOSE:
	{
		UIFlowMenuCommonOption_ScrClose(obj);
		break;
	}

	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
	{
		UIFlowMenuCommonOption_ChildScrClose(obj);
		break;
	}

	case LV_EVENT_PRESSED:
		lv_plugin_menu_set_selected_item_pressed(menu_item);
		break;

	case LV_EVENT_RELEASED:
		lv_plugin_menu_set_selected_item_released(menu_item);
		break;

	case LV_EVENT_CLICKED:
		MenuItem_OnSelected(obj);
		break;

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		/* handle key event */
		UIFlowMenuCommonOption_Key(obj, *key);


		/***********************************************************************************
		 * IMPORTANT!!
		 *
		 * calling lv_indev_wait_release to avoid duplicate event in long pressed key state
		 * the event will not be sent again until released
		 *
		 ***********************************************************************************/
		if(*key != LV_KEY_ENTER)
			lv_indev_wait_release(lv_indev_get_act());
		break;
	}

	default:
		break;

	}

}
