#include "UIFlowLVGL/UIFlowLVGL.h"
#include "UIFlowWndUSBAPI.h"
#include <kwrap/debug.h>
static lv_group_t* gp = NULL;

static void set_indev_keypad_group(lv_obj_t* obj)
{
	if(gp == NULL){
		gp = lv_group_create();
		lv_group_add_obj(gp, obj);
	}

	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_group(indev, gp);
}

void UIFlowUSBEventCallback(lv_obj_t* obj, lv_event_t event)
{
    uint32_t* usbmode;
	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
	{
		lv_obj_t* img_usb = image_usb_scr_uiflowusb;

        DBG_DUMP("UIFlowUSB_ScrOpen\r\n");

        set_indev_keypad_group(obj);

        usbmode = (uint32_t*)lv_event_get_data();
        DBG_DUMP("UIFlowUSB_ScrOpen usbmode %x\r\n",*usbmode);
        if (*usbmode == UIFlowWndUSB_MSDC_MODE) {
            DBG_DUMP("UIFlowUSB_ScrOpen MSDC\r\n");
            lv_plugin_label_set_text(label_usbmode_scr_uiflowusb, LV_PLUGIN_STRING_ID_STRID_MSDC);
			lv_plugin_label_update_font(label_usbmode_scr_uiflowusb, 0);
			lv_plugin_img_set_src(img_usb, LV_PLUGIN_IMG_ID_ICON_USB_MSDC);
        }
        else {
            DBG_DUMP("UIFlowUSB_ScrOpen PCC\r\n");
            lv_plugin_label_set_text(label_usbmode_scr_uiflowusb, LV_PLUGIN_STRING_ID_STRID_PCC);
			lv_plugin_label_update_font(label_usbmode_scr_uiflowusb, 0);
			lv_plugin_img_set_src(img_usb, LV_PLUGIN_IMG_ID_ICON_USB_UVC);
        }
		break;
	}
	case LV_PLUGIN_EVENT_SCR_CLOSE:
        DBG_DUMP("UIFlowUSB_ScrClose\r\n");
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

