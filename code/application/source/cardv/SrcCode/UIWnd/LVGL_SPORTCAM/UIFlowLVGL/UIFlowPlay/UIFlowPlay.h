#ifndef UIFLOWPLAY_H
#define UIFLOWPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 *      INCLUDES
 **********************/

#include "lvgl/lvgl.h"

/**********************
 *       WIDGETS
 **********************/
void UIFlowPlayEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* image_mode_playback_scr_uiflowplay;
extern lv_obj_t* image_file_attri_scr_uiflowplay;
extern lv_obj_t* label_file_name_scr_uiflowplay;
extern lv_obj_t* image_file_ev_scr_uiflowplay;
extern lv_obj_t* image_file_wb_scr_uiflowplay;
extern lv_obj_t* image_file_flash_scr_uiflowplay;
extern lv_obj_t* label_play_time_scr_uiflowplay;
extern lv_obj_t* container_bt_bar_scr_uiflowplay;
extern lv_obj_t* button_up_bg_scr_uiflowplay;
extern lv_obj_t* image_5_scr_uiflowplay;
extern lv_obj_t* button_left_bg_scr_uiflowplay;
extern lv_obj_t* image_3_scr_uiflowplay;
extern lv_obj_t* button_select_bg_scr_uiflowplay;
extern lv_obj_t* image_1_scr_uiflowplay;
extern lv_obj_t* button_right_bg_scr_uiflowplay;
extern lv_obj_t* image_2_scr_uiflowplay;
extern lv_obj_t* button_down_bg_scr_uiflowplay;
extern lv_obj_t* image_4_scr_uiflowplay;
extern lv_obj_t* label_file_time_scr_uiflowplay;
extern lv_obj_t* label_file_date_scr_uiflowplay;
extern lv_obj_t* image_quality_scr_uiflowplay;
extern lv_obj_t* image_sharpness_scr_uiflowplay;
extern lv_obj_t* image_storage_scr_uiflowplay;
extern lv_obj_t* label_file_size_scr_uiflowplay;
extern lv_obj_t* image_battery_scr_uiflowplay;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowPlay_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWPLAY_H*/