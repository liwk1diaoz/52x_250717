#include "UIFlowLVGL/UIFlowMenuCommonConfirm/UIFlowMenuCommonConfirm.h"
#include "UIFlowLVGL/UIFlowLVGL.h"

#include "Resource/Plugin/lvgl_plugin.h"
/**********************
 *       WIDGETS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
lv_obj_t* message_box_1_scr_uiflowmenucommonconfirm;

lv_obj_t* UIFlowMenuCommonConfirm_create(){
	lv_obj_t *parent = lv_plugin_scr_create();
	lv_obj_set_event_cb(parent, UIFlowMenuCommonConfirmEventCallback);

	lv_color_t color = {0};
	STYLE_COLOR_PROP(0x00, 0x55, 0x1f, 0x57);
	_lv_obj_set_style_local_color(parent,0,LV_STYLE_BG_COLOR, color);

	if(color.full== LV_COLOR_TRANSP.full){
		_lv_obj_set_style_local_opa(parent,0,LV_STYLE_BG_OPA,0);
	}


	static lv_style_t message_box_1_s0;
	lv_style_init(&message_box_1_s0);
	lv_style_set_radius(&message_box_1_s0,LV_STATE_DEFAULT,10);
	STYLE_COLOR_PROP(0xcd, 0x1f, 0x48, 0x9a) ; lv_style_set_bg_color(&message_box_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&message_box_1_s0, LV_STATE_DEFAULT, color);
	lv_style_set_border_width(&message_box_1_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&message_box_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x09, 0xc0, 0xc0, 0xc0) ; lv_style_set_shadow_color(&message_box_1_s0, LV_STATE_DEFAULT, color);
	lv_style_set_shadow_opa(&message_box_1_s0,LV_STATE_DEFAULT,0);
	lv_style_set_shadow_width(&message_box_1_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&message_box_1_s0, LV_STATE_DEFAULT, color);
	lv_style_set_text_font(&message_box_1_s0,LV_STATE_DEFAULT,&notosans_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&message_box_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&message_box_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&message_box_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x09, 0xc0, 0xc0, 0xc0) ; lv_style_set_shadow_color(&message_box_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&message_box_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xcd, 0x1f, 0x48, 0x9a) ; lv_style_set_bg_color(&message_box_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&message_box_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&message_box_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x09, 0xc0, 0xc0, 0xc0) ; lv_style_set_shadow_color(&message_box_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&message_box_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&message_box_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&message_box_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&message_box_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x09, 0xc0, 0xc0, 0xc0) ; lv_style_set_shadow_color(&message_box_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&message_box_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&message_box_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&message_box_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&message_box_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x09, 0xc0, 0xc0, 0xc0) ; lv_style_set_shadow_color(&message_box_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&message_box_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xcd, 0x1f, 0x48, 0x9a) ; lv_style_set_bg_color(&message_box_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&message_box_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&message_box_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x09, 0xc0, 0xc0, 0xc0) ; lv_style_set_shadow_color(&message_box_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&message_box_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&message_box_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&message_box_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&message_box_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x09, 0xc0, 0xc0, 0xc0) ; lv_style_set_shadow_color(&message_box_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&message_box_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s0, LV_STATE_DISABLED, color);
	static lv_style_t message_box_1_s1;
	lv_style_init(&message_box_1_s1);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&message_box_1_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&message_box_1_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&message_box_1_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&message_box_1_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&message_box_1_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&message_box_1_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&message_box_1_s1, LV_STATE_DISABLED, color);
	static lv_style_t message_box_1_s2;
	lv_style_init(&message_box_1_s2);
	lv_style_set_radius(&message_box_1_s2,LV_STATE_DEFAULT,10);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_border_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	lv_style_set_border_width(&message_box_1_s2,LV_STATE_DEFAULT,3);
	STYLE_COLOR_PROP(0x06, 0xff, 0xff, 0x00) ; lv_style_set_outline_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	lv_style_set_outline_opa(&message_box_1_s2,LV_STATE_DEFAULT,255);
	lv_style_set_outline_width(&message_box_1_s2,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&message_box_1_s2, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xd3, 0x31, 0x40, 0x4f) ; lv_style_set_value_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	lv_style_set_value_font(&message_box_1_s2,LV_STATE_DEFAULT,&lv_font_montserrat_16);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	lv_style_set_text_font(&message_box_1_s2,LV_STATE_DEFAULT,&notosans_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s2, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x3a, 0x01, 0xa2, 0xb1) ; lv_style_set_bg_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_border_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	lv_style_set_border_width(&message_box_1_s2,LV_STATE_CHECKED,6);
	STYLE_COLOR_PROP(0x06, 0xff, 0xff, 0x00) ; lv_style_set_outline_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&message_box_1_s2, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_value_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	lv_style_set_text_font(&message_box_1_s2,LV_STATE_CHECKED,&lv_font_montserrat_16);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s2, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_border_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x06, 0xff, 0xff, 0x00) ; lv_style_set_outline_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	lv_style_set_outline_opa(&message_box_1_s2,LV_STATE_FOCUSED,255);
	lv_style_set_outline_width(&message_box_1_s2,LV_STATE_FOCUSED,2);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x31, 0x40, 0x4f) ; lv_style_set_value_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s2, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s2, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s2, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_border_color(&message_box_1_s2, LV_STATE_EDITED, color);
	lv_style_set_border_width(&message_box_1_s2,LV_STATE_EDITED,6);
	STYLE_COLOR_PROP(0x06, 0xff, 0xff, 0x00) ; lv_style_set_outline_color(&message_box_1_s2, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&message_box_1_s2, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&message_box_1_s2, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x31, 0x40, 0x4f) ; lv_style_set_value_color(&message_box_1_s2, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_color(&message_box_1_s2, LV_STATE_EDITED, color);
	lv_style_set_text_font(&message_box_1_s2,LV_STATE_EDITED,&lv_font_montserrat_16);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s2, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s2, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_border_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	lv_style_set_border_width(&message_box_1_s2,LV_STATE_HOVERED,6);
	STYLE_COLOR_PROP(0x06, 0xff, 0xff, 0x00) ; lv_style_set_outline_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&message_box_1_s2, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x31, 0x40, 0x4f) ; lv_style_set_value_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	lv_style_set_text_font(&message_box_1_s2,LV_STATE_HOVERED,&lv_font_montserrat_16);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s2, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xbd, 0xc7, 0xfb, 0xe7) ; lv_style_set_bg_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_border_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x06, 0xff, 0xff, 0x00) ; lv_style_set_outline_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	lv_style_set_outline_width(&message_box_1_s2,LV_STATE_PRESSED,2);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x31, 0x40, 0x4f) ; lv_style_set_value_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s2, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_border_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	lv_style_set_border_width(&message_box_1_s2,LV_STATE_DISABLED,6);
	STYLE_COLOR_PROP(0x06, 0xff, 0xff, 0x00) ; lv_style_set_outline_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&message_box_1_s2, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x9c, 0x88, 0x88, 0x88) ; lv_style_set_value_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	lv_style_set_text_font(&message_box_1_s2,LV_STATE_DISABLED,&lv_font_montserrat_16);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&message_box_1_s2, LV_STATE_DISABLED, color);
	lv_obj_t *message_box_1 = lv_msgbox_create(parent, NULL);
	lv_obj_set_hidden(message_box_1, false);
	lv_obj_set_click(message_box_1, true);
	lv_obj_set_drag(message_box_1, false);
	lv_obj_set_pos(message_box_1, 21, 63);
	lv_obj_set_size(message_box_1, 279, 118);
	static const char* message_box_1_LVGLPropertyMsgBoxBtnmatrixButtonsText[] = {"AAAAAA","BBBBBB","",""};
	lv_msgbox_add_btns(message_box_1, message_box_1_LVGLPropertyMsgBoxBtnmatrixButtonsText);
	lv_plugin_msgbox_allocate_ext_attr(message_box_1);
	lv_plugin_msgbox_set_text(message_box_1, LV_PLUGIN_STRING_ID_STRID_DELETE_WARNING);
	lv_msgbox_set_text(message_box_1,"All data will be deleted");
	lv_plugin_msgbox_set_font_type(message_box_1, LV_PLUGIN_LANGUAGE_FONT_TYPE_0, LV_MSGBOX_PART_BG);
	lv_plugin_msgbox_set_font_type(message_box_1, LV_PLUGIN_LANGUAGE_FONT_TYPE_0, LV_MSGBOX_PART_BTN);
	lv_plugin_msgbox_set_font_type(message_box_1, LV_PLUGIN_LANGUAGE_FONT_TYPE_0, LV_MSGBOX_PART_BTN_BG);
	lv_msgbox_set_anim_time(message_box_1, 500);
	lv_msgbox_set_recolor(message_box_1, false);
	lv_obj_add_style(message_box_1, 0, &message_box_1_s0);
	lv_obj_add_style(message_box_1, 64, &message_box_1_s1);
	lv_obj_add_style(message_box_1, 65, &message_box_1_s2);

	message_box_1_scr_uiflowmenucommonconfirm = message_box_1;
	lv_obj_set_event_cb(message_box_1_scr_uiflowmenucommonconfirm, message_box_confirm_msg_event_callback);


	return parent;
}
