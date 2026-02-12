#include "UIFlowLVGL/UIFlowWifiWait/UIFlowWifiWait.h"
#include "UIFlowLVGL/UIFlowLVGL.h"

#include "Resource/Plugin/lvgl_plugin.h"
/**********************
 *       WIDGETS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
lv_obj_t* container_1_scr_uiflowwifiwait;
lv_obj_t* spinner_1_scr_uiflowwifiwait;
lv_obj_t* container_2_scr_uiflowwifiwait;
lv_obj_t* image_cap_scr_uiflowwifiwait;
lv_obj_t* image_con_scr_uiflowwifiwait;
lv_obj_t* image_wifi_scr_uiflowwifiwait;

lv_obj_t* UIFlowWifiWait_create(){
	lv_obj_t *parent = lv_plugin_scr_create();
	lv_obj_set_event_cb(parent, UIFlowWifiWaitEventCallback);

	lv_color_t color = {0};
	STYLE_COLOR_PROP(0xd0, 0x23, 0x23, 0x23);
	_lv_obj_set_style_local_color(parent,0,LV_STYLE_BG_COLOR, color);

	if(color.full== LV_COLOR_TRANSP.full){
		_lv_obj_set_style_local_opa(parent,0,LV_STYLE_BG_OPA,0);
	}


	static lv_style_t container_1_s0;
	lv_style_init(&container_1_s0);
	lv_style_set_radius(&container_1_s0,LV_STATE_DEFAULT,0);
	lv_style_set_pad_inner(&container_1_s0,LV_STATE_DEFAULT,30);
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
	lv_obj_set_pos(container_1, 74, 94);
	lv_obj_set_size(container_1, 171, 57);
	lv_cont_set_layout(container_1, LV_LAYOUT_ROW_MID);
	lv_obj_add_style(container_1, 0, &container_1_s0);

	container_1_scr_uiflowwifiwait = container_1;


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
	lv_style_set_line_width(&spinner_1_s1,LV_STATE_DEFAULT,5);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xf8, 0xfd, 0xfd, 0xfd) ; lv_style_set_line_color(&spinner_1_s1, LV_STATE_DISABLED, color);
	lv_obj_t *spinner_1 = lv_spinner_create(parent, NULL);
	lv_obj_set_hidden(spinner_1, false);
	lv_obj_set_click(spinner_1, true);
	lv_obj_set_drag(spinner_1, false);
	lv_obj_set_pos(spinner_1, 282, 200);
	lv_obj_set_size(spinner_1, 30, 32);
	lv_spinner_set_dir(spinner_1, LV_SPINNER_DIR_FORWARD);
	lv_spinner_set_spin_time(spinner_1, 1000);
	lv_spinner_set_arc_length(spinner_1, 30);
	lv_spinner_set_type(spinner_1, LV_SPINNER_TYPE_FILLSPIN_ARC);
	lv_obj_add_style(spinner_1, 0, &spinner_1_s0);
	lv_obj_add_style(spinner_1, 1, &spinner_1_s1);

	spinner_1_scr_uiflowwifiwait = spinner_1;


	static lv_style_t container_2_s0;
	lv_style_init(&container_2_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_2_s0, LV_STATE_DEFAULT, color);
	lv_style_set_bg_opa(&container_2_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_2_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_2_s0, LV_STATE_DEFAULT, color);
	lv_style_set_border_opa(&container_2_s0,LV_STATE_DEFAULT,0);
	lv_style_set_border_width(&container_2_s0,LV_STATE_DEFAULT,0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_2_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_2_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_2_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_2_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_2_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_2_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_2_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_2_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_2_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_2_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_2_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_2_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_2_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_2_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_2_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&container_2_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&container_2_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xe9, 0xdd, 0xdd, 0xdd) ; lv_style_set_border_color(&container_2_s0, LV_STATE_DISABLED, color);
	lv_obj_t *container_2 = lv_cont_create(parent, NULL);
	lv_obj_set_hidden(container_2, false);
	lv_obj_set_click(container_2, true);
	lv_obj_set_drag(container_2, false);
	lv_obj_set_pos(container_2, 84, 102);
	lv_obj_set_size(container_2, 161, 50);
	lv_cont_set_layout(container_2, LV_LAYOUT_PRETTY_MID);
	lv_obj_add_style(container_2, 0, &container_2_s0);

	container_2_scr_uiflowwifiwait = container_2;


	static lv_style_t image_cap_s0;
	lv_style_init(&image_cap_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_cap_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_cap_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_cap_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_cap_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_cap_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_cap_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_cap_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_cap_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_cap_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_cap_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_cap_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_cap_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_cap_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_cap_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_cap_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_cap_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_cap_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_cap_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_cap_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_cap_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_cap_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_cap_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_cap_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_cap_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_cap_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_cap_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_cap_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_cap_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_cap_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_cap_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_cap_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_cap_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_cap_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_cap_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_cap_s0, LV_STATE_DISABLED, color);
	lv_obj_t *image_cap = lv_img_create(container_2, NULL);
	lv_obj_set_hidden(image_cap, false);
	lv_obj_set_click(image_cap, false);
	lv_obj_set_drag(image_cap, false);
	lv_obj_set_pos(image_cap, 14, 14);
	lv_obj_set_size(image_cap, 28, 28);
	lv_img_set_src(image_cap, &icon_mode_capture);
	lv_obj_add_style(image_cap, 0, &image_cap_s0);

	image_cap_scr_uiflowwifiwait = image_cap;


	static lv_style_t image_con_s0;
	lv_style_init(&image_con_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_con_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_con_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_con_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_con_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_con_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_con_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_con_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_con_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_con_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_con_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_con_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_con_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_con_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_con_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_con_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_con_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_con_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_con_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_con_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_con_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_con_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_con_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_con_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_con_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_con_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_con_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_con_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_con_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_con_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_con_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_con_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_con_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_con_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_con_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_con_s0, LV_STATE_DISABLED, color);
	lv_obj_t *image_con = lv_img_create(container_2, NULL);
	lv_obj_set_hidden(image_con, false);
	lv_obj_set_click(image_con, false);
	lv_obj_set_drag(image_con, false);
	lv_obj_set_pos(image_con, 66, 14);
	lv_obj_set_size(image_con, 28, 28);
	lv_img_set_src(image_con, &icon_wifi_connecting3);
	lv_obj_add_style(image_con, 0, &image_con_s0);

	image_con_scr_uiflowwifiwait = image_con;


	static lv_style_t image_wifi_s0;
	lv_style_init(&image_wifi_s0);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_wifi_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_wifi_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_wifi_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_wifi_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_wifi_s0, LV_STATE_DEFAULT, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_wifi_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_wifi_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_wifi_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_wifi_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_wifi_s0, LV_STATE_CHECKED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_wifi_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_wifi_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_wifi_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_wifi_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_wifi_s0, LV_STATE_FOCUSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_wifi_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_wifi_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_wifi_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_wifi_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_wifi_s0, LV_STATE_EDITED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_wifi_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_wifi_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_wifi_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_wifi_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_wifi_s0, LV_STATE_HOVERED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_wifi_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_wifi_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_wifi_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_wifi_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_wifi_s0, LV_STATE_PRESSED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_color(&image_wifi_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x02, 0xff, 0xff, 0xff) ; lv_style_set_bg_grad_color(&image_wifi_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_border_color(&image_wifi_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0x01, 0x00, 0x00, 0x00) ; lv_style_set_outline_color(&image_wifi_s0, LV_STATE_DISABLED, color);
	STYLE_COLOR_PROP(0xd3, 0x3c, 0x3c, 0x3c) ; lv_style_set_image_recolor(&image_wifi_s0, LV_STATE_DISABLED, color);
	lv_obj_t *image_wifi = lv_img_create(container_2, NULL);
	lv_obj_set_hidden(image_wifi, false);
	lv_obj_set_click(image_wifi, false);
	lv_obj_set_drag(image_wifi, false);
	lv_obj_set_pos(image_wifi, 118, 14);
	lv_obj_set_size(image_wifi, 28, 28);
	lv_img_set_src(image_wifi, &icon_wifi_on);
	lv_obj_add_style(image_wifi, 0, &image_wifi_s0);

	image_wifi_scr_uiflowwifiwait = image_wifi;


	return parent;
}
