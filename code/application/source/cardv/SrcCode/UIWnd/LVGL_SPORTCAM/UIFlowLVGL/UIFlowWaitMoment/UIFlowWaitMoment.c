#include "UIFlowLVGL/UIFlowWaitMoment/UIFlowWaitMoment.h"
#include "UIFlowLVGL/UIFlowLVGL.h"

#include "Resource/Plugin/lvgl_plugin.h"
/**********************
 *       WIDGETS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
lv_obj_t* container_1_scr_uiflowwaitmoment;
lv_obj_t* label_1_scr_uiflowwaitmoment;
lv_obj_t* spinner_1_scr_uiflowwaitmoment;

lv_obj_t* UIFlowWaitMoment_create(){
	lv_obj_t *parent = lv_plugin_scr_create();
	lv_obj_set_event_cb(parent, UIFlowWaitMomentEventCallback);

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
	lv_obj_set_pos(container_1, 5, 60);
	lv_obj_set_size(container_1, 311, 174);
	lv_cont_set_layout(container_1, LV_LAYOUT_COLUMN_MID);
	lv_obj_add_style(container_1, 0, &container_1_s0);

	container_1_scr_uiflowwaitmoment = container_1;


	static lv_style_t label_1_s0;
	lv_style_init(&label_1_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&label_1_s0, LV_STATE_DEFAULT, color);
	lv_style_set_text_font(&label_1_s0,LV_STATE_DEFAULT,&notosans_black_16_1bpp);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&label_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&label_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&label_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&label_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&label_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&label_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&label_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&label_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&label_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&label_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&label_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&label_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&label_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&label_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&label_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&label_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&label_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&label_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&label_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&label_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_text_color(&label_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_text_sel_color(&label_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x3a, 0x00, 0xb4, 0x95) ; lv_style_set_text_sel_bg_color(&label_1_s0, LV_STATE_DISABLED, color);
	lv_obj_t *label_1 = lv_label_create(container_1, NULL);
	lv_obj_set_hidden(label_1, false);
	lv_obj_set_click(label_1, false);
	lv_obj_set_drag(label_1, false);
	lv_plugin_label_allocate_ext_attr(label_1);
	lv_plugin_label_set_text(label_1, LV_PLUGIN_STRING_ID_STRID_NULL_);
	lv_plugin_label_allocate_ext_attr(label_1);
	lv_plugin_label_set_font_type(label_1, LV_PLUGIN_LANGUAGE_FONT_TYPE_0);
	lv_label_set_align(label_1, LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(label_1, LV_LABEL_LONG_CROP);
	lv_obj_set_pos(label_1, 50, 14);
	lv_obj_set_size(label_1, 210, 93);
	lv_obj_add_style(label_1, 0, &label_1_s0);

	label_1_scr_uiflowwaitmoment = label_1;


	static lv_style_t spinner_1_s0;
	lv_style_init(&spinner_1_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&spinner_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&spinner_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&spinner_1_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_line_color(&spinner_1_s0, LV_STATE_DEFAULT, color);
	lv_style_set_line_opa(&spinner_1_s0,LV_STATE_DEFAULT,0);
	lv_style_set_line_width(&spinner_1_s0,LV_STATE_DEFAULT,5);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&spinner_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&spinner_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&spinner_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_line_color(&spinner_1_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&spinner_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&spinner_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&spinner_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_line_color(&spinner_1_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&spinner_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&spinner_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&spinner_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_line_color(&spinner_1_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&spinner_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&spinner_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&spinner_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_line_color(&spinner_1_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&spinner_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&spinner_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&spinner_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_line_color(&spinner_1_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&spinner_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&spinner_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&spinner_1_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_line_color(&spinner_1_s0, LV_STATE_DISABLED, color);
	static lv_style_t spinner_1_s1;
	lv_style_init(&spinner_1_s1);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_DEFAULT, color);
	lv_style_set_line_opa(&spinner_1_s1,LV_STATE_DEFAULT,255);
	lv_style_set_line_width(&spinner_1_s1,LV_STATE_DEFAULT,7);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_DISABLED, color);
	lv_obj_t *spinner_1 = lv_spinner_create(container_1, NULL);
	lv_obj_set_hidden(spinner_1, false);
	lv_obj_set_click(spinner_1, true);
	lv_obj_set_drag(spinner_1, false);
	lv_obj_set_pos(spinner_1, 129, 119);
	lv_obj_set_size(spinner_1, 53, 52);
	lv_spinner_set_dir(spinner_1, LV_SPINNER_DIR_FORWARD);
	lv_spinner_set_spin_time(spinner_1, 1000);
	lv_spinner_set_arc_length(spinner_1, 30);
	lv_spinner_set_type(spinner_1, LV_SPINNER_TYPE_FILLSPIN_ARC);
	lv_obj_add_style(spinner_1, 0, &spinner_1_s0);
	lv_obj_add_style(spinner_1, 1, &spinner_1_s1);

	spinner_1_scr_uiflowwaitmoment = spinner_1;


	return parent;
}
