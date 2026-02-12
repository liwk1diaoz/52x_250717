#include "UIFlowLVGL/UIFlowSetupDateTime/UIFlowSetupDateTime.h"
#include "UIFlowLVGL/UIFlowLVGL.h"

#include "Resource/Plugin/lvgl_plugin.h"
/**********************
 *       WIDGETS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
lv_obj_t* button_matrix_datetime_scr_uiflowsetupdatetime;

lv_obj_t* UIFlowSetupDateTime_create(){
	lv_obj_t *parent = lv_plugin_scr_create();
	lv_obj_set_event_cb(parent, UIFlowSetupDateTimeEventCallback);

	lv_color_t color = {0};
	STYLE_COLOR_PROP(0x00, 0x55, 0x1f, 0x57);
	_lv_obj_set_style_local_color(parent,0,LV_STYLE_BG_COLOR, color);

	if(color.full== LV_COLOR_TRANSP.full){
		_lv_obj_set_style_local_opa(parent,0,LV_STYLE_BG_OPA,0);
	}


	static lv_style_t button_matrix_datetime_s0;
	lv_style_init(&button_matrix_datetime_s0);
	lv_style_set_radius(&button_matrix_datetime_s0,LV_STATE_DEFAULT,0);
	lv_style_set_pad_top(&button_matrix_datetime_s0,LV_STATE_DEFAULT,3);
	lv_style_set_pad_bottom(&button_matrix_datetime_s0,LV_STATE_DEFAULT,3);
	lv_style_set_pad_left(&button_matrix_datetime_s0,LV_STATE_DEFAULT,3);
	lv_style_set_pad_right(&button_matrix_datetime_s0,LV_STATE_DEFAULT,3);
	lv_style_set_pad_inner(&button_matrix_datetime_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&button_matrix_datetime_s0, LV_STATE_DEFAULT, color);
	lv_style_set_bg_opa(&button_matrix_datetime_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&button_matrix_datetime_s0, LV_STATE_DEFAULT, color);
	lv_style_set_border_width(&button_matrix_datetime_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&button_matrix_datetime_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&button_matrix_datetime_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&button_matrix_datetime_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&button_matrix_datetime_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&button_matrix_datetime_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&button_matrix_datetime_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&button_matrix_datetime_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&button_matrix_datetime_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&button_matrix_datetime_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&button_matrix_datetime_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xcc, 0x02, 0x62, 0xb6) ; lv_style_set_bg_color(&button_matrix_datetime_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&button_matrix_datetime_s0, LV_STATE_DISABLED, color);
	static lv_style_t button_matrix_datetime_s1;
	lv_style_init(&button_matrix_datetime_s1);
	lv_style_set_radius(&button_matrix_datetime_s1,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0xd2, 0x30, 0x30, 0x30) ; lv_style_set_bg_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_border_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	lv_style_set_border_width(&button_matrix_datetime_s1,LV_STATE_DEFAULT,0);
	lv_style_set_border_side(&button_matrix_datetime_s1,LV_STATE_DEFAULT,1);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_value_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xdc, 0xb5, 0xb5, 0xb5) ; lv_style_set_text_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	lv_style_set_text_font(&button_matrix_datetime_s1,LV_STATE_DEFAULT,&notosans_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_text_sel_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x3a, 0x01, 0xa2, 0xb1) ; lv_style_set_text_sel_bg_color(&button_matrix_datetime_s1, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xd2, 0x30, 0x30, 0x30) ; lv_style_set_bg_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_border_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	lv_style_set_border_width(&button_matrix_datetime_s1,LV_STATE_CHECKED,0);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_value_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xdc, 0xb5, 0xb5, 0xb5) ; lv_style_set_text_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	lv_style_set_text_font(&button_matrix_datetime_s1,LV_STATE_CHECKED,&notosanscjksc_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_text_sel_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x3a, 0x01, 0xa2, 0xb1) ; lv_style_set_text_sel_bg_color(&button_matrix_datetime_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd5, 0x57, 0x57, 0x57) ; lv_style_set_bg_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_border_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_value_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xf3, 0xf2, 0xf2, 0xf2) ; lv_style_set_text_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	lv_style_set_text_font(&button_matrix_datetime_s1,LV_STATE_FOCUSED,&notosanscjksc_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_text_sel_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x3a, 0x01, 0xa2, 0xb1) ; lv_style_set_text_sel_bg_color(&button_matrix_datetime_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd2, 0x30, 0x30, 0x30) ; lv_style_set_bg_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_border_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_value_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xdc, 0xb5, 0xb5, 0xb5) ; lv_style_set_text_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	lv_style_set_text_font(&button_matrix_datetime_s1,LV_STATE_EDITED,&notosanscjksc_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_text_sel_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x3a, 0x01, 0xa2, 0xb1) ; lv_style_set_text_sel_bg_color(&button_matrix_datetime_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd2, 0x30, 0x30, 0x30) ; lv_style_set_bg_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_border_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_value_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xdc, 0xb5, 0xb5, 0xb5) ; lv_style_set_text_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	lv_style_set_text_font(&button_matrix_datetime_s1,LV_STATE_HOVERED,&notosanscjksc_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_text_sel_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x3a, 0x01, 0xa2, 0xb1) ; lv_style_set_text_sel_bg_color(&button_matrix_datetime_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x83, 0x56, 0x56, 0x56) ; lv_style_set_bg_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_border_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_value_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xf3, 0xf2, 0xf2, 0xf2) ; lv_style_set_text_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	lv_style_set_text_font(&button_matrix_datetime_s1,LV_STATE_PRESSED,&notosanscjksc_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_text_sel_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x3a, 0x01, 0xa2, 0xb1) ; lv_style_set_text_sel_bg_color(&button_matrix_datetime_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd2, 0x30, 0x30, 0x30) ; lv_style_set_bg_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x0e, 0x00, 0x00, 0xff) ; lv_style_set_border_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_shadow_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_pattern_recolor(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_value_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xdc, 0xb5, 0xb5, 0xb5) ; lv_style_set_text_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	lv_style_set_text_font(&button_matrix_datetime_s1,LV_STATE_DISABLED,&notosanscjksc_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3b, 0x3e, 0x42) ; lv_style_set_text_sel_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x3a, 0x01, 0xa2, 0xb1) ; lv_style_set_text_sel_bg_color(&button_matrix_datetime_s1, LV_STATE_DISABLED, color);
	lv_obj_t *button_matrix_datetime = lv_btnmatrix_create(parent, NULL);
	lv_obj_set_hidden(button_matrix_datetime, false);
	lv_obj_set_click(button_matrix_datetime, true);
	lv_obj_set_drag(button_matrix_datetime, false);
	lv_obj_set_pos(button_matrix_datetime, 24, 43);
	lv_obj_set_size(button_matrix_datetime, 268, 152);
	static const char* button_matrix_datetime_LVGLPropertyBtnmatrixButtonsText[] = {"1","9","8","6","/","0","7","/","2","1","\n"," "," "," ","2","3",":","5","9"," "," "," ","",""};
	lv_btnmatrix_set_map(button_matrix_datetime, button_matrix_datetime_LVGLPropertyBtnmatrixButtonsText);
	lv_btnmatrix_set_one_check(button_matrix_datetime, false);
	lv_btnmatrix_set_align(button_matrix_datetime, LV_LABEL_ALIGN_CENTER);
	lv_btnmatrix_set_focused_btn(button_matrix_datetime,0);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 4, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 7, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 10, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 11, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 12, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 15, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 18, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 19, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_btn_ctrl(button_matrix_datetime, 20, LV_BTNMATRIX_CTRL_DISABLED);
	lv_btnmatrix_set_one_check(button_matrix_datetime, true);
	lv_obj_add_style(button_matrix_datetime, 0, &button_matrix_datetime_s0);
	lv_obj_add_style(button_matrix_datetime, 1, &button_matrix_datetime_s1);

	button_matrix_datetime_scr_uiflowsetupdatetime = button_matrix_datetime;


	return parent;
}
