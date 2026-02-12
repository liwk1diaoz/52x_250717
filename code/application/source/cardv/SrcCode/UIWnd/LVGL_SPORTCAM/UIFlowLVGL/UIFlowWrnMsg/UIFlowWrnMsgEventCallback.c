#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowWrnMsg/UIFlowWrnMsgAPI.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include <kwrap/debug.h>

/**************************************************************
 * static variables
 **************************************************************/
static lv_group_t* gp = NULL;
static lv_obj_t* msgbox = NULL;
static const char* btn_map[2] = {NULL, ""};

static NVTEVT evt = NVTEVT_NULL;

static void set_indev_keypad_group(lv_obj_t* obj)
{
	if(gp == NULL){
		gp = lv_group_create();
	}

	lv_group_add_obj(gp, obj);
	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_group(indev, gp);
}

void  UIFlowWrnMsgAPI_Open_StringID(lv_plugin_res_id id, uint16_t auto_close_time_ms)
{
	if(msgbox){
		DBG_WRN("warning message box is already opened and not closed yet, ignore request");
		return;
	}

	/*************************************************************************************
	 * message box created by builder's generated code is a sample and should keep invisible,
	 * this api will create a duplicate obj of the sample
	 *************************************************************************************/
	lv_obj_set_hidden(message_box_1_scr_uiflowwrnmsg, true);

	msgbox = lv_msgbox_create(UIFlowWrnMsg, message_box_1_scr_uiflowwrnmsg);

	 /* remember to set duplicate obj visible */
	 lv_obj_set_hidden(msgbox, false);

	 /* anim time won't be copied by lvgl, config it manually */
	 uint16_t anim_time = lv_msgbox_get_anim_time(message_box_1_scr_uiflowwrnmsg);
	 lv_msgbox_set_anim_time(msgbox, anim_time);

	/*******************************************************************
	 * IMPORTANT!!
	 *
	 * msgbox created in user code should call lv_plugin_msgbox_allocate_ext_attr
	 * before using plugin msgbox API
	 *
	 *******************************************************************/
	 lv_plugin_msgbox_allocate_ext_attr(msgbox);

	 lv_plugin_msgbox_set_text(msgbox, id);

	/* add button */
	btn_map[0] = lv_plugin_get_string(LV_PLUGIN_STRING_ID_STRID_OK)->ptr;
	lv_msgbox_add_btns(msgbox, btn_map);

	/* update font of text */
	lv_plugin_msgbox_update_font(msgbox, LV_MSGBOX_PART_BG);

	lv_obj_t * btnmatrix = lv_msgbox_get_btnmatrix(msgbox);

	if(btnmatrix){
		lv_btnmatrix_set_focused_btn(btnmatrix, 0);

		/* update font of button text */
		lv_plugin_msgbox_update_font(msgbox, LV_MSGBOX_PART_BTN);
	}


	if(auto_close_time_ms){
		lv_msgbox_start_auto_close(msgbox, auto_close_time_ms);
	}

	set_indev_keypad_group(msgbox);

	lv_plugin_scr_open(UIFlowWrnMsg, NULL);
}

void UIFlowWrnMsgEventCallback_OnOpen(lv_obj_t* obj)
{



}

void UIFlowWrnMsgEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
		UIFlowWrnMsgEventCallback_OnOpen(obj);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
		break;

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

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

void  UIFlowWrnMsg_CloseScr(NVTEVT e)
{
	evt = e;
	lv_msgbox_start_auto_close(msgbox, 0);
}

void message_box_wrnmsg_OnKeyMenu(lv_obj_t* msgbox)
{
	UIFlowWrnMsg_CloseScr(NVTRET_ENTER_MENU);
}

void message_box_wrnmsg_OnKey(lv_obj_t* msgbox, uint32_t key)
{
	switch(key)
	{
	case LV_USER_KEY_MENU:
		message_box_wrnmsg_OnKeyMenu(msgbox);
		break;

	default:
		break;

	}

}

void message_box_wrnmsg_event_callback(lv_obj_t* obj, lv_event_t event)
{
    switch (event)
    {
		case LV_EVENT_KEY:
		{
			uint32_t* key = (uint32_t*)lv_event_get_data();

			message_box_wrnmsg_OnKey(obj, *key);

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

    	case LV_EVENT_DELETE:
    	{
    		/* reset pointer of msgbox */
    		msgbox = NULL;

    		/* parent is UIFlowWrnMsg */
    		if(evt != NVTEVT_NULL){
    			lv_plugin_scr_close(lv_obj_get_parent(obj), gen_nvtmsg_data(evt, 0));
    		}
    		else{
    			lv_plugin_scr_close(lv_obj_get_parent(obj), NULL);
    		}
    		break;
    	}

    	/* triggered by button release */
		case LV_EVENT_VALUE_CHANGED:
		{
			uint32_t value = *(uint32_t*)lv_event_get_data();

			if (value != LV_BTNMATRIX_BTN_NONE) {
				UIFlowWrnMsg_CloseScr(NVTEVT_NULL);
			}

			break;
		}

    }
}

