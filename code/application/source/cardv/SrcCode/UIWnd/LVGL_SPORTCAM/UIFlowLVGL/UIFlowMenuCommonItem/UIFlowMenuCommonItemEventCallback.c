
#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include "UIApp/Network/UIAppNetwork.h"
#include <kwrap/debug.h>

#define PAGE           4

#define MENU_KEY_PRESS_MASK        (FLGKEY_UP|FLGKEY_DOWN|FLGKEY_RIGHT|FLGKEY_SHUTTER2)
#define MENU_KEY_RELEASE_MASK      (FLGKEY_UP|FLGKEY_DOWN|FLGKEY_RIGHT|FLGKEY_SHUTTER2)
#define MENU_KEY_CONTINUE_MASK     (FLGKEY_UP|FLGKEY_DOWN|FLGKEY_RIGHT|FLGKEY_SHUTTER2)

static TM_MENU *g_pItemMenu = 0;
static lv_group_t* gp = NULL;
static lv_obj_t* menu_item = NULL;
static lv_obj_t* label_menu_item = NULL;
static lv_obj_t* label_menu_option = NULL;

static void set_indev_keypad_group(lv_obj_t* obj)
{
	if(gp == NULL){
		gp = lv_group_create();
		lv_group_add_obj(gp, obj);
	}

	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_group(indev, gp);
}

static void MenuCommonItem_SetCurrentMenu(TM_MENU *pMenu)
{
	g_pItemMenu = pMenu;
}
static TM_MENU *MenuCommonItem_GetCurrentMenu(void)
{
	return g_pItemMenu;
}

static void MenuCommonItem_UpdateContent(TM_MENU *pMenu);

static void MenuItem_OnNext(lv_obj_t* obj)
{

	TM_MENU    *pMenu;
	TM_PAGE    *pPage;

	pMenu = MenuCommonItem_GetCurrentMenu();
	pPage = &pMenu->pPages[pMenu->SelPage];
	pPage->SelItem++;
	//check item if disable
	TM_CheckItemStatus(pMenu, &pPage->SelItem, TRUE);
	if (pPage->SelItem == pPage->Count) {
		lv_plugin_scr_close(obj, NULL);
	} else {
		MenuCommonItem_UpdateContent(pMenu);
		lv_plugin_menu_select_next_item(menu_item);
	}
}

static void MenuItem_OnPrev(lv_obj_t* obj)
{
	TM_MENU    *pMenu;
	TM_PAGE    *pPage;


	pMenu = MenuCommonItem_GetCurrentMenu();
	pPage = &pMenu->pPages[pMenu->SelPage];
	if (pPage->SelItem == 0) {
		// Close current UI Window now
//		Ux_CloseWindow(&MenuCommonItemCtrl, 0);

		lv_plugin_scr_close(obj, NULL);


	} else {
		pPage->SelItem--;
		//check item if disable
		TM_CheckItemStatus(pMenu, &pPage->SelItem, FALSE);
		if (pPage->SelItem == pPage->Count) {
			lv_plugin_scr_close(obj, NULL);

		} else {
			MenuCommonItem_UpdateContent(pMenu);
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
	TM_MENU    *pNextMenu;
	UINT32      SelOption = 0 ;


	pMenu = MenuCommonItem_GetCurrentMenu();
	pPage = &pMenu->pPages[pMenu->SelPage];
	pItem = &pPage->pItems[pPage->SelItem];


	if (pItem->Count != 0 && pItem->SysFlag != 0 && pItem->ItemId != IDM_COMMON_CLOUD) {
		SelOption = SysGetFlag(pItem->SysFlag);

		SelOption++;
		if (SelOption >= pItem->Count) {
			SelOption = 0;
		}

		SysSetFlag(pItem->SysFlag, SelOption);

		// toggle icon's string
		pOption = &pItem->pOptions[SelOption];

		lv_plugin_label_set_text(label_menu_option, pOption->TextId);
		lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);

		TM_MENU_CALLBACK(pMenu, TMM_CONFIRM_OPTION, MAKE_LONG(pItem->ItemId, SelOption));
	} else {
		if (pItem->SysFlag == FL_COMMON_MODE) {
			// Enter 2nd level menu and pop up various memu item.
			lv_plugin_scr_open(UIFlowMenuCommonOption, &gModeMenu);

		} else if (pItem->SysFlag == FL_COMMON_MENU) {
#if (PHOTO_MODE == ENABLE)
			// Enter 2nd level menu and pop up current mode's menu lists
			if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO) {
				pNextMenu = &gPhotoMenu;
#if (PLAY_MODE == ENABLE)
			} else if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
				pNextMenu = &gPlaybackMenu;
#endif
			} else {
				pNextMenu = &gMovieMenu;
			}
#else
				pNextMenu = &gMovieMenu;
#endif

			lv_plugin_scr_open(UIFlowMenuCommonOption, pNextMenu);

		} else if (pItem->SysFlag == FL_COMMON_SETUP) {
			// Enter 2nd level menu and pop up various memu item.
			lv_plugin_scr_open(UIFlowMenuCommonOption, &gSetupMenu);

		} else if (pItem->SysFlag == FL_COMMON_EXT_SETUP) {
			#if _TODO
			// Enter 2nd level menu and pop up various memu item.
//			Ux_OpenWindow(&MenuCommonOptionCtrl, 1, &gExtSetupMenu);
			#endif
		} else {
			DBG_ERR("not supp %d\r\n", pItem->SysFlag);

		}
	}
}

void MenuItem_OnClose(lv_obj_t* obj)
{
	Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_DEFAULT);
	Input_SetKeyMask(KEY_RELEASE, FLGKEY_KEY_MASK_DEFAULT);
	Input_SetKeyMask(KEY_CONTINUE, FLGKEY_KEY_MASK_DEFAULT);
#if (MOVIE_MODE==ENABLE)
	//#NT#2016/08/19#Lincy Lin#[0106935] -begin
	//#NT# Support change WDR, SHDR, RSC setting will change mode after exit menu
	BOOL bReOpenMovie = FlowMovie_CheckReOpenItem();
	BOOL bReOpenPhoto = 0;
#if (PHOTO_MODE==ENABLE)
	bReOpenPhoto = FlowPhoto_CheckReOpenItem();
#endif
	if(bReOpenMovie)
		DBG_DUMP("RESTART_MODE_YES\r\n");
	else
		DBG_DUMP("RESTART_MODE_NO\r\n");

	if (bReOpenMovie || bReOpenPhoto)
		//#NT#2016/08/19#Lincy Lin -end
	{
		Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, System_GetState(SYS_STATE_CURRMODE));
	}
#endif

}

void MenuItem_OnOpen(lv_obj_t* obj)
{
	DBG_DUMP("MenuItem_OnOpen\r\n");

	TM_MENU    *pMenu = NULL;
	TM_PAGE    *pPage = NULL;
	TM_ITEM    *pItem = NULL;
	TM_OPTION  *pOption = NULL;
	TM_ITEM    *pModeItem = NULL;
#if (PHOTO_MODE==ENABLE)
	INT32      curMode = 0;
#endif
	Input_SetKeyMask(KEY_PRESS, MENU_KEY_PRESS_MASK);
	Input_SetKeyMask(KEY_RELEASE, MENU_KEY_RELEASE_MASK);
	Input_SetKeyMask(KEY_CONTINUE, MENU_KEY_CONTINUE_MASK);

#if(WIFI_FUNC==ENABLE)
	if (UI_GetData(FL_WIFI_LINK) == WIFI_LINK_OK && UI_GetData(FL_NetWorkMode) == NET_STATION_MODE) {
		SysSetFlag(FL_COMMON_CLOUD, CLOUD_ON);
	} else {
		SysSetFlag(FL_COMMON_CLOUD, CLOUD_OFF);
	}
#else
        SysSetFlag(FL_COMMON_CLOUD, CLOUD_OFF);
#endif

	if (System_GetEnableSensor() == (SENSOR_1 | SENSOR_2)) {
		TM_SetItemStatus(&gMovieMenu, IDM_MOVIE_DUAL_CAM, TM_ITEM_ENABLE);
#if (PHOTO_MODE==ENABLE)
		TM_SetItemStatus(&gPhotoMenu, IDM_DUAL_CAM, TM_ITEM_ENABLE);
#endif
#if (_BOARD_DRAM_SIZE_ == 0x04000000)
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_DUAL_1920x1080P30_1280x720P30, TM_OPTION_ENABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_DUAL_1920x1080P30_848x480P30, TM_OPTION_ENABLE);
#else
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_DUAL_1920x1080P30_1920x1080P30, TM_OPTION_ENABLE);
#endif
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_2880x2160P50, TM_OPTION_DISABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_3840x2160P30, TM_OPTION_DISABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_2704x2032P60, TM_OPTION_DISABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_2560x1440P80, TM_OPTION_DISABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_2560x1440P60, TM_OPTION_DISABLE);
	} else {
#if (SENSOR_CAPS_COUNT > 1)
		TM_SetItemStatus(&gMovieMenu, IDM_MOVIE_DUAL_CAM, TM_ITEM_DISABLE);
		TM_SetItemStatus(&gPhotoMenu, IDM_DUAL_CAM, TM_ITEM_DISABLE);
#endif
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_DUAL_1920x1080P30_1920x1080P30, TM_OPTION_DISABLE);
		//#NT#2016/08/12#Hideo Lin -begin
		//#NT#For small size clone movie
#if (SMALL_CLONE_MOVIE == DISABLE)
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_2880x2160P50, TM_OPTION_DISABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_3840x2160P30, TM_OPTION_DISABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_2704x2032P60, TM_OPTION_DISABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_2560x1440P80, TM_OPTION_DISABLE);
		TM_SetOptionStatus(&gMovieMenu, IDM_MOVIE_SIZE, MOVIE_SIZE_FRONT_2560x1440P60, TM_OPTION_DISABLE);
#endif
		//#NT#2016/08/12#Hideo Lin -end
	}

	MenuCommonItem_SetCurrentMenu(&gCommonMenu);
	pMenu = MenuCommonItem_GetCurrentMenu();

	pMenu->Status = TMS_ON_ITEM;
	pMenu->SelPage = 0;         // reset page to 0

	pPage = &pMenu->pPages[pMenu->SelPage];
	pPage->SelItem = 0;           // reset item to 0
	//check item if disable
	TM_CheckItemStatus(pMenu, &pPage->SelItem, TRUE);

	pItem = &pPage->pItems[pPage->SelItem];

	if(pItem->Count){
		pOption = &pItem->pOptions[SysGetFlag(pItem->SysFlag)];
		lv_plugin_label_set_text(label_menu_option, pOption->TextId);
		lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);
	}

#if (PHOTO_MODE==ENABLE)
	curMode = System_GetState(SYS_STATE_CURRMODE);
	if (curMode == PRIMARY_MODE_PHOTO) {
		pModeItem = &pPage->pItems[1];
		pModeItem->IconId = LV_PLUGIN_IMG_ID_ICON_MODE_CAPTURE_M;
		pModeItem->TextId = LV_PLUGIN_STRING_ID_STRID_CAP_MODE;
	} else if (curMode == PRIMARY_MODE_MOVIE) {
		pModeItem = &pPage->pItems[1];
		pModeItem->IconId = LV_PLUGIN_IMG_ID_ICON_MODE_VIDEO_M;
		pModeItem->TextId = LV_PLUGIN_STRING_ID_STRID_MOVIE;
#if (PLAY_MODE == ENABLE)
	} else if (curMode == PRIMARY_MODE_PLAYBACK) {
		pModeItem = &pPage->pItems[1];
		pModeItem->IconId = LV_PLUGIN_IMG_ID_ICON_MODE_PLAYBACK_M;
		pModeItem->TextId = LV_PLUGIN_STRING_ID_STRID_PLAYBACK;
#endif
	}
#else
		pModeItem = &pPage->pItems[1];
		pModeItem->IconId = ICON_MODE_VIDEO_M;
		pModeItem->TextId = STRID_MOVIE;

#endif


	/* check menu item is init */
	if(!lv_plugin_menu_item_cnt(menu_item)){
		/* allocate menu item */
		lv_plugin_menu_init_items(menu_item, PAGE);
	}

	MenuCommonItem_UpdateContent(pMenu);
	lv_plugin_menu_select_item(menu_item, 0);

	Input_SetKeyMask(KEY_PRESS, MENU_KEY_PRESS_MASK);
	Input_SetKeyMask(KEY_RELEASE, MENU_KEY_PRESS_MASK);
	Input_SetKeyMask(KEY_CONTINUE, MENU_KEY_PRESS_MASK);
	Input_SetKeySoundMask(KEY_PRESS, MENU_KEY_PRESS_MASK);

}

static void MenuCommonItem_UpdateContent(TM_MENU *pMenu)
{
	TM_PAGE    *pPage;
	TM_ITEM    *pItem;
	TM_OPTION  *pOption;
	UINT32      i;
	UINT16      startIndex = 0;
	UINT16      itemIndex = 0;

	pPage = &pMenu->pPages[pMenu->SelPage];
	pItem = &pPage->pItems[pPage->SelItem];
	pOption = &pItem->pOptions[SysGetFlag(pItem->SysFlag)];

	if (pItem->Count) {
		lv_plugin_label_set_text(label_menu_option, pOption->TextId);
		lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);
	} else if (pItem->ItemId == IDM_COMMON_MENU) {
		lv_plugin_label_set_text(label_menu_option, LV_PLUGIN_STRING_ID_STRID_SETUP);
		lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);
	} else {
		lv_plugin_label_set_text(label_menu_option, LV_PLUGIN_STRING_ID_STRID_NULL_);
		lv_plugin_label_update_font(label_menu_option, LV_OBJ_PART_MAIN);
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



static void UIFlowMenuCommonItem_ScrOpen(lv_obj_t* obj)
{
	DBG_DUMP("UIFlowMenuCommonItem_ScrOpen\r\n");


	/***********************************************************************************
	 * Add Menu Screen into group and set group to keypad indev
	 ***********************************************************************************/
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
		menu_item = lv_plugin_menu_create(obj, container_main_menu_scr_uiflowmenucommonitem);
		lv_plugin_menu_set_wrap(menu_item, true);
	}

	if(label_menu_item == NULL)
		label_menu_item = label_menu_item_scr_uiflowmenucommonitem;

	if(label_menu_option == NULL)
		label_menu_option = label_menu_option_scr_uiflowmenucommonitem;

    MenuItem_OnOpen(obj);

}

static void UIFlowMenuCommonItem_Key(lv_obj_t* obj, uint32_t key)
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
		lv_plugin_scr_close(obj, NULL);
		break;
	}

	}

}

static void UIFlowMenuCommonItem_ScrClose(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

	MenuItem_OnClose(obj);
}


static void UIFlowMenuCommonItem_ChildScrClose(lv_obj_t* obj,const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	DBG_DUMP("%s\r\n", __func__);

	set_indev_keypad_group(obj);
}


void container_main_menu_callback(lv_obj_t* obj, lv_event_t event)
{
	DBG_DUMP("%s\r\n", __func__);
}

void UIFlowMenuCommonItemEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{
	case LV_PLUGIN_EVENT_SCR_OPEN:
	{
		UIFlowMenuCommonItem_ScrOpen(obj);
		break;
	}

	case LV_PLUGIN_EVENT_SCR_CLOSE:
	{
		UIFlowMenuCommonItem_ScrClose(obj);
		break;
	}

	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
	{
		UIFlowMenuCommonItem_ChildScrClose(obj,(const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data());

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
		UIFlowMenuCommonItem_Key(obj, *key);

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
