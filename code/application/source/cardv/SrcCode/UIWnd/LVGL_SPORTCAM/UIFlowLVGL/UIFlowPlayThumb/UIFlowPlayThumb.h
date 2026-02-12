#ifndef UIFLOWPLAYTHUMB_H
#define UIFLOWPLAYTHUMB_H

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
void UIFlowPlayThumbEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* button_thumb_item1_scr_uiflowplaythumb;
extern lv_obj_t* image_item1_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item1_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* button_thumb_item2_scr_uiflowplaythumb;
extern lv_obj_t* image_item2_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item2_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* button_thumb_item3_scr_uiflowplaythumb;
extern lv_obj_t* image_item3_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item3_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* button_thumb_item4_scr_uiflowplaythumb;
extern lv_obj_t* image_item4_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item4_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* button_thumb_item5_scr_uiflowplaythumb;
extern lv_obj_t* image_item5_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item5_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* button_thumb_item6_scr_uiflowplaythumb;
extern lv_obj_t* image_item6_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item6_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* button_thumb_item7_scr_uiflowplaythumb;
extern lv_obj_t* image_item7_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item7_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* button_thumb_item8_scr_uiflowplaythumb;
extern lv_obj_t* image_item8_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item8_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* button_thumb_item9_scr_uiflowplaythumb;
extern lv_obj_t* image_item9_lock_scr_uiflowplaythumb;
extern lv_obj_t* image_item9_filefmt_scr_uiflowplaythumb;
extern lv_obj_t* label_page_info_scr_uiflowplaythumb;
extern lv_obj_t* label_file_name_scr_uiflowplaythumb;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowPlayThumb_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWPLAYTHUMB_H*/