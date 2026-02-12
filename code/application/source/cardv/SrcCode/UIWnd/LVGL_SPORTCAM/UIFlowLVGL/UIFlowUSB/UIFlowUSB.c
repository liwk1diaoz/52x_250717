#include "UIFlowLVGL/UIFlowUSB/UIFlowUSB.h"
#include "UIFlowLVGL/UIFlowLVGL.h"

#include "Resource/Plugin/lvgl_plugin.h"
/**********************
 *       WIDGETS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
lv_obj_t* container_1_scr_uiflowusb;
lv_obj_t* label_usbmode_scr_uiflowusb;
lv_obj_t* image_usb_scr_uiflowusb;

lv_obj_t* UIFlowUSB_create(){
	lv_obj_t *parent = lv_plugin_scr_create();
	lv_obj_set_event_cb(parent, UIFlowUSBEventCallback);

	lv_color_t color = {0};
	STYLE_COLOR_PROP(0xd0, 0x23, 0x23, 0x23);
	_lv_obj_set_style_local_color(parent,0,LV_STYLE_BG_COLOR, color);

	if(color.full== LV_COLOR_TRANSP.full){
		_lv_obj_set_style_local_opa(parent,0,LV_STYLE_BG_OPA,0);
	}


	static lv_style_t container_1_s0;
	lv_style_init(&container_1_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_1_s0, LV_STATE_DEFAULT, color);
	lv_style_set_bg_opa(&container_1_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_1_s0, LV_STATE_DEFAULT, color);
	lv_style_set_border_opa(&container_1_s0,LV_STATE_DEFAULT,0);
	lv_style_set_border_width(&container_1_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_1_s0, LV_STATE_DISABLED, color);
	lv_obj_t *container_1 = lv_cont_create(parent, NULL);
	lv_obj_set_hidden(container_1, false);
	lv_obj_set_click(container_1, true);
	lv_obj_set_drag(container_1, false);
	lv_obj_set_pos(container_1, 6, 91);
	lv_obj_set_size(container_1, 306, 85);
	lv_cont_set_layout(container_1, LV_LAYOUT_COLUMN_MID);
	lv_obj_add_style(container_1, 0, &container_1_s0);

	container_1_scr_uiflowusb = container_1;


	static lv_style_t label_usbmode_s0;
	lv_style_init(&label_usbmode_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_usbmode_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_usbmode_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_usbmode_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_text_color(&label_usbmode_s0, LV_STATE_DEFAULT, color);
	lv_style_set_text_font(&label_usbmode_s0,LV_STATE_DEFAULT,&notosans_black_16_1bpp);
	STYLE_COLOR_PROP(0x2f, 0x00, 0x9e, 0x84) ; lv_style_set_text_sel_color(&label_usbmode_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_text_sel_bg_color(&label_usbmode_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_usbmode_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_usbmode_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_usbmode_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_text_color(&label_usbmode_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x2f, 0x00, 0x9e, 0x84) ; lv_style_set_text_sel_color(&label_usbmode_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_text_sel_bg_color(&label_usbmode_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_usbmode_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_usbmode_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_usbmode_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_text_color(&label_usbmode_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x2f, 0x00, 0x9e, 0x84) ; lv_style_set_text_sel_color(&label_usbmode_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_text_sel_bg_color(&label_usbmode_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_usbmode_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_usbmode_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_usbmode_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_text_color(&label_usbmode_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x2f, 0x00, 0x9e, 0x84) ; lv_style_set_text_sel_color(&label_usbmode_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_text_sel_bg_color(&label_usbmode_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_usbmode_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_usbmode_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_usbmode_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_text_color(&label_usbmode_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x2f, 0x00, 0x9e, 0x84) ; lv_style_set_text_sel_color(&label_usbmode_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_text_sel_bg_color(&label_usbmode_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_usbmode_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_usbmode_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_usbmode_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_text_color(&label_usbmode_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x2f, 0x00, 0x9e, 0x84) ; lv_style_set_text_sel_color(&label_usbmode_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_text_sel_bg_color(&label_usbmode_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_usbmode_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_usbmode_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_usbmode_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_text_color(&label_usbmode_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x2f, 0x00, 0x9e, 0x84) ; lv_style_set_text_sel_color(&label_usbmode_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_text_sel_bg_color(&label_usbmode_s0, LV_STATE_DISABLED, color);
	lv_obj_t *label_usbmode = lv_label_create(container_1, NULL);
	lv_obj_set_hidden(label_usbmode, false);
	lv_obj_set_click(label_usbmode, false);
	lv_obj_set_drag(label_usbmode, false);
	lv_plugin_label_allocate_ext_attr(label_usbmode);
	lv_plugin_label_set_text(label_usbmode, LV_PLUGIN_STRING_ID_STRID_MSDC);
	lv_label_set_text(label_usbmode,"Mass Storage");
	lv_plugin_label_allocate_ext_attr(label_usbmode);
	lv_plugin_label_set_font_type(label_usbmode, LV_PLUGIN_LANGUAGE_FONT_TYPE_0);
	lv_obj_set_pos(label_usbmode, 99, 14);
	lv_obj_set_size(label_usbmode, 109, 19);
	lv_obj_add_style(label_usbmode, 0, &label_usbmode_s0);

	label_usbmode_scr_uiflowusb = label_usbmode;


	static lv_style_t image_usb_s0;
	lv_style_init(&image_usb_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_usb_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_usb_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_usb_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_usb_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_usb_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_usb_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_usb_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_usb_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_usb_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_usb_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_usb_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_usb_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_usb_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_usb_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_usb_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_usb_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_usb_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_usb_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_usb_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_usb_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_usb_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_usb_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_usb_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_usb_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_usb_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_usb_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_usb_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_usb_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_usb_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_usb_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_usb_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_usb_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_usb_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_usb_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_usb_s0, LV_STATE_DISABLED, color);
	lv_obj_t *image_usb = lv_img_create(container_1, NULL);
	lv_obj_set_hidden(image_usb, false);
	lv_obj_set_click(image_usb, false);
	lv_obj_set_drag(image_usb, false);
	lv_obj_set_pos(image_usb, 139, 45);
	lv_obj_set_size(image_usb, 28, 28);
	lv_img_set_src(image_usb, &icon_usb_on);
	lv_obj_add_style(image_usb, 0, &image_usb_s0);

	image_usb_scr_uiflowusb = image_usb;


	return parent;
}
