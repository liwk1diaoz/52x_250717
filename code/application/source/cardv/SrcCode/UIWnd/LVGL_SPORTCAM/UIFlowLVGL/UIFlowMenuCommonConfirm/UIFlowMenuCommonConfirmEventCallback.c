#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include "UIApp/Play/UIPlayComm.h"
#include "UIFlowLVGL/UIFlowMenuCommonConfirm/UIFlowMenuCommonConfirmAPI.h"
#include "UIFlowLVGL/UIFlowWrnMsg/UIFlowWrnMsgAPI.h"
#include <kwrap/debug.h>


static UINT32 gBKGEvt = 0;
static UINT32 gExeEvt = 0;
static UINT32 gItemID = 0;
static uint32_t string_id_please_wait = FLOWWRNMSG_ISSUE_PLEASE_WAIT;
static lv_obj_t* wait_moment_scr = NULL;
static lv_group_t* gp = NULL;
static lv_obj_t* msgbox = NULL;
#define MAX_MSGBOX_BTN_TEXT_LEN 16

static const char* btn_map[3] = {NULL, NULL, ""};

static void set_indev_keypad_group(lv_obj_t* obj)
{
	if(gp == NULL){
		gp = lv_group_create();
	}

	lv_group_add_obj(gp, obj);
	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_group(indev, gp);
}


void  UIFlowMenuCommonConfirmAPI_Open(uint32_t itemID)
{
	if(msgbox){
		DBG_ERR("confirm message box is already opened by id(%lu)!, ignore request id(%lu)", gItemID, itemID);
		return;
	}


	/*************************************************************************************
	 * message box created by builder's generated code is a sample and should keep invisible,
	 * this api will create a duplicate obj of the sample
	 *************************************************************************************/
	 lv_obj_set_hidden(message_box_1_scr_uiflowmenucommonconfirm, true);

	 msgbox = lv_msgbox_create(UIFlowMenuCommonConfirm, message_box_1_scr_uiflowmenucommonconfirm);

	 /* remember to set duplicate obj visible */
	 lv_obj_set_hidden(msgbox, false);

	 /* anim time won't be copied by lvgl, config it manually */
	 uint16_t anim_time = lv_msgbox_get_anim_time(message_box_1_scr_uiflowmenucommonconfirm);
	 lv_msgbox_set_anim_time(msgbox, anim_time);


	/*******************************************************************
	 * IMPORTANT!!
	 *
	 * msgbox created in user code should call lv_plugin_msgbox_allocate_ext_attr
	 * before using plugin msgbox API
	 *
	 *******************************************************************/
	lv_plugin_msgbox_allocate_ext_attr(msgbox);

	DBG_DUMP("itemID=%u\r\n",itemID);

	UINT32 strID = 0;
	gBKGEvt = 0;
	gExeEvt = 0;

	gItemID = itemID;

	switch (gItemID) {
	case IDM_FORMAT:
		strID = LV_PLUGIN_STRING_ID_STRID_DELETE_WARNING;
		gBKGEvt = NVTEVT_BKW_FORMAT_CARD;
		break;
	case IDM_DEFAULT:
		strID = LV_PLUGIN_STRING_ID_STRID_RESET_WARNING;
		gExeEvt = NVTEVT_EXE_SYSRESET_NO_WIN;
		break;
	case IDM_FW_UPDATE:
		strID = LV_PLUGIN_STRING_ID_STRID_FW_UPDATE;
		gBKGEvt = NVTEVT_BKW_FW_UPDATE;
		break;
	case IDM_DELETE_ALL:
		strID = LV_PLUGIN_STRING_ID_STRID_ERASE_ALL;
		gBKGEvt = NVTEVT_BKW_DELALL;
		break;
	case IDM_PROTECT_ALL:
		strID = LV_PLUGIN_STRING_ID_STRID_LOCKALL;
		AppBKW_SetData(BKW_PROTECT_TYPE, SETUP_PROTECT_ALL);
		gBKGEvt = NVTEVT_BKW_SETPROTECT;
		break;
	case IDM_UNPROTECT_ALL:
		strID = LV_PLUGIN_STRING_ID_STRID_UNLOCKALL;
		AppBKW_SetData(BKW_PROTECT_TYPE, SETUP_UNPROTECT_ALL);
		gBKGEvt = NVTEVT_BKW_SETPROTECT;
		break;
	case IDM_PROTECT:
		strID = LV_PLUGIN_STRING_ID_STRID_LOCKONE;
		AppBKW_SetData(BKW_PROTECT_TYPE, SETUP_PROTECT_ONE);
		gBKGEvt = NVTEVT_BKW_SETPROTECT;
		break;
	case IDM_DELETE_THIS:
		strID = LV_PLUGIN_STRING_ID_STRID_ERASE_THIS;
		break;
	default:
		gBKGEvt = 0;
		DBG_ERR("error itemID %d\r\n", gItemID);
	}
	DBG_FUNC("gItemID=%d, gBKGEvt=%d\r\n",gItemID,gBKGEvt);


	/* set text by string id */
	lv_plugin_msgbox_set_text(msgbox, strID);

	/* update font of message part */
	lv_plugin_msgbox_update_font(msgbox, LV_MSGBOX_PART_BG);

	if (gItemID == IDM_PROTECT) {
		DBG_FUNC("IDM_PROTECT>>\r\n");

		btn_map[0] = lv_plugin_get_string(LV_PLUGIN_STRING_ID_STRID_LOCK)->ptr;
		btn_map[1] = lv_plugin_get_string(LV_PLUGIN_STRING_ID_STRID_UNLOCK)->ptr;

		DBG_FUNC("IDM_PROTECT<<\r\n");
	} else {
		DBG_FUNC("IDM_OKNO>>\r\n");

		btn_map[0] = lv_plugin_get_string(LV_PLUGIN_STRING_ID_STRID_OK)->ptr;
		btn_map[1] = lv_plugin_get_string(LV_PLUGIN_STRING_ID_STRID_NO)->ptr;

		DBG_FUNC("IDM_OKNO<<\r\n");
	}

	lv_msgbox_add_btns(msgbox, btn_map);
	lv_obj_t * btnmatrix = lv_msgbox_get_btnmatrix(msgbox);

	if(btnmatrix){
		lv_btnmatrix_set_focused_btn(btnmatrix, 0);

		/* update font of button part */
		lv_plugin_msgbox_update_font(msgbox, LV_MSGBOX_PART_BTN);
	}

	lv_plugin_scr_open(UIFlowMenuCommonConfirm, NULL);

}


void UIFlowMenuCommonConfirm_OnOpen(lv_obj_t* obj)
{
	if(NULL == wait_moment_scr){
		wait_moment_scr = UIFlowWaitMoment;
	}


	set_indev_keypad_group(msgbox);

}

void UIFlowMenuCommonConfirm_CloseScr(void)
{
	lv_msgbox_start_auto_close(msgbox, 0);
}

void UIFlowMenuCommonConfirm_OnChildScrClose(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	if(msgbox == NULL){
		return;
	}

	set_indev_keypad_group(msgbox);

	if(msg){

		const char* msgbox_text = lv_msgbox_get_text(msgbox);
		const char* erase_text = lv_plugin_get_string(LV_PLUGIN_STRING_ID_STRID_ERASE_THIS)->ptr;

		if(msg->event == NVTRET_WAITMOMENT || strcmp(msgbox_text, erase_text) == 0){
			UIFlowMenuCommonConfirm_CloseScr();
		}
	}
}


/***************************************************************************
 *
 * Send control key to make message box change focused button
 *
 ***************************************************************************/
static void UIFlowMenuCommonConfirm_MessageBox_Key(lv_obj_t* obj, uint32_t key)
{

	lv_group_t *gp = lv_obj_get_group(obj);

	switch(key)
	{

	case LV_USER_KEY_NEXT:
	{
		if(gp){
			lv_group_send_data(gp, LV_KEY_RIGHT);
		}


		break;
	}

	case LV_USER_KEY_PREV:
	{
		if(gp){
			lv_group_send_data(gp, LV_KEY_LEFT);
		}

		break;
	}

	}
}

static void UIFlowMenuCommonConfirm_MessageBox_ValueChanged(lv_obj_t* obj, uint32_t* value)
{
	if(NULL == value)
		return;

	if (*value == 0) {

		const char* msgbox_text = lv_msgbox_get_text(obj);
		const char* erase_text = lv_plugin_get_string(LV_PLUGIN_STRING_ID_STRID_ERASE_THIS)->ptr;

		if(strcmp(msgbox_text, erase_text) == 0){
			#if(PLAY_MODE)
			UINT32 uiLockStatus;
			PB_GetParam(PBPRMID_FILE_ATTR_LOCK, &uiLockStatus);
			if (uiLockStatus) {

				UIFlowWrnMsgAPI_Open_StringID(FLOWWRNMSG_ISSUE_FILE_PROTECTED, 3000);
				return;

			}
			UIPlay_Delete(PB_DELETE_ONE);
			UIPlay_PlaySingle(PB_SINGLE_CURR);
			#endif

			UIFlowMenuCommonConfirm_CloseScr();


		} else if (gBKGEvt) {

			DBG_FUNC("gBKGEvt=%d\r\n",gBKGEvt);
			UIFlowWaitMomentAPI_Set_Text_Param param = (UIFlowWaitMomentAPI_Set_Text_Param) { .string_id = string_id_please_wait};
			lv_plugin_scr_open(wait_moment_scr, &param);
			BKG_PostEvent(gBKGEvt);

		} else {
			DBG_FUNC("gBKGEvt==>0 and call Ux_SendEvent(gExeEvt=%d)\r\n",gExeEvt);
			Ux_SendEvent(&UISetupObjCtrl, gExeEvt, 0);
			UIFlowMenuCommonConfirm_CloseScr();
		}
	} else {
		if (gItemID == IDM_PROTECT) {

			DBG_FUNC("gItemID == IDM_PROTECT\r\n");
			AppBKW_SetData(BKW_PROTECT_TYPE, SETUP_UNPROTECT_ONE);
			UIFlowWaitMomentAPI_Set_Text_Param param = (UIFlowWaitMomentAPI_Set_Text_Param) { .string_id = string_id_please_wait};
			lv_plugin_scr_open(wait_moment_scr, &param);
			BKG_PostEvent(gBKGEvt);

		} else {
			DBG_FUNC("gItemID != IDM_PROTECT\r\n");
			UIFlowMenuCommonConfirm_CloseScr();
		}
	}
}

void UIFlowMenuCommonConfirmEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
	{
		UIFlowMenuCommonConfirm_OnOpen(obj);
		break;
	}

	case LV_PLUGIN_EVENT_SCR_CLOSE:
		break;


	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
	{

		UIFlowMenuCommonConfirm_OnChildScrClose(obj, (const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data());
		break;
	}

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		LV_UNUSED(key); /* avoid compile error, please remove it manually */
		break;
	}

	default:
		break;

	}

}

void message_box_confirm_msg_event_callback(lv_obj_t* obj, lv_event_t event)
{
	switch(event)
	{

	/* this makes message box closing animation can be completed before closing screen */
	case LV_EVENT_DELETE:
	{
		/* reset pointer of msgbox */
		msgbox = NULL;

		/* parent is UIFlowMenuCommonConfirm */
		lv_plugin_scr_close(lv_obj_get_parent(obj), NULL);
		break;
	}

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		/* handle key event */
		UIFlowMenuCommonConfirm_MessageBox_Key(obj, *key);

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

	/* triggered by button release */
	case LV_EVENT_VALUE_CHANGED:
	{
		UIFlowMenuCommonConfirm_MessageBox_ValueChanged(obj, (uint32_t*)lv_event_get_data());
		break;
	}



	}

}
