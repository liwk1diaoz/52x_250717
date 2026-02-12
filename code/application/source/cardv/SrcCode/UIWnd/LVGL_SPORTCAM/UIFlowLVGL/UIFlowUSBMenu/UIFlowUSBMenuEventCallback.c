#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include <kwrap/debug.h>
static lv_group_t* gp_btns = NULL;

typedef enum {
	BTN_USB_MSDC = 0,
	BTN_USB_PCC,
} WIFI_LINK_BTN_FLAG;

static uint32_t btn_sta = BTN_USB_MSDC;

static void set_indev_keypad_group(lv_obj_t* obj)
{
    if(gp_btns == NULL)
    {
        gp_btns = lv_group_create();

        lv_group_add_obj(gp_btns, button_msdc_scr_uiflowusbmenu);
        lv_group_add_obj(gp_btns, button_pcc_scr_uiflowusbmenu);

        lv_group_set_wrap(gp_btns, true);
        lv_group_focus_obj(button_msdc_scr_uiflowusbmenu);
    }
        
    lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_group(indev, gp_btns);
}

static void UIFlowUSBMenu_OnKeyEnter(lv_obj_t* obj, uint32_t key)
{
    DBG_DUMP("UIFlowUSBMenu_ScrKeyENTER\r\n");
    if (lv_group_get_focused(gp_btns) == button_pcc_scr_uiflowusbmenu) {
        btn_sta = BTN_USB_PCC;
    }
    else if (lv_group_get_focused(gp_btns) == button_msdc_scr_uiflowusbmenu) {
        btn_sta = BTN_USB_MSDC;
    }
    else {
    }
    DBG_IND("menu item index %x\r\n",btn_sta);
    if (btn_sta == BTN_USB_MSDC) {
        //msdc mode
        DBG_DUMP("UIFlowUSBMenu_ScrKeyENTER MSDC\r\n");
        Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_USBMSDC);
    }
    else {
        //pcc mode
        DBG_DUMP("UIFlowUSBMenu_ScrKeyENTER PCC\r\n");
        Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_USBPCC);
    }
}

static void UIFlowUSBMenu_Key(lv_obj_t* obj, uint32_t key)
{
	switch(key)
	{
        case LV_USER_KEY_NEXT:
        {
            lv_group_focus_next(gp_btns);
            break;
        }

        case LV_USER_KEY_PREV:
        {
            lv_group_focus_prev(gp_btns);
            break;
        }

        case LV_USER_KEY_SHUTTER2:
        {
            UIFlowUSBMenu_OnKeyEnter(obj, key);
            break;
        }
	}
}

static void lv_plugin_obj_iterator_set_state_callback(lv_obj_t* obj)
{
	lv_obj_t* parent = lv_obj_get_parent(obj);
	lv_state_t state = lv_obj_get_state(parent, LV_OBJ_PART_MAIN);

	if(state & LV_STATE_FOCUSED){
		lv_obj_set_state(obj, LV_STATE_FOCUSED);
	}
	else{
		lv_obj_set_state(obj, LV_STATE_DEFAULT);
	}
}

static void UIFlowUSBMenuBtnEventCallback(lv_obj_t* obj, lv_event_t event)
{
    DBG_IND("%s:%d\r\n", __func__, event);

    switch(event)
    {

        case LV_EVENT_CLICKED:
        {
            uint32_t* key = (uint32_t*)lv_event_get_data();

            UIFlowUSBMenu_OnKeyEnter(obj, *key);
            break;
        }

        case LV_EVENT_KEY:
        {
            uint32_t* key = (uint32_t*)lv_event_get_data();

            DBG_DUMP("UIFlowUSBMenu_ScrKey is %x\r\n", *key);

            UIFlowUSBMenu_Key(obj,*key);

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
            //LV_UNUSED(key); /* avoid compile error, please remove it manually */
            //break;
        }

        case LV_EVENT_FOCUSED:
        {
        	lv_plugin_obj_iterator(obj, lv_plugin_obj_iterator_set_state_callback, false);
        	break;
        }

        case LV_EVENT_DEFOCUSED:
        {
        	lv_plugin_obj_iterator(obj, lv_plugin_obj_iterator_set_state_callback, false);
        	break;
        }
    }
}

void btn_msdc_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowUSBMenuBtnEventCallback(obj, event);
}

void btn_pcc_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowUSBMenuBtnEventCallback(obj, event);
}

void UIFlowUSBMenuEventCallback(lv_obj_t* obj, lv_event_t event)
{
	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
        DBG_DUMP("UIFlowUSBMenu_ScrOpen\r\n");
        set_indev_keypad_group(obj);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
        DBG_DUMP("UIFlowUSBMenu_ScrClose\r\n");
		break;

	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
		
		break;
	/* handle nvt event except key event */
	case LV_USER_EVENT_NVTMSG:
	{
		break;
	}

	default:
		break;

	}

}

