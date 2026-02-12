#ifndef UIFLOWPHOTO_H
#define UIFLOWPHOTO_H

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
void UIFlowPhotoEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* image_mode_photo_scr_uiflowphoto;
extern lv_obj_t* image_iso_scr_uiflowphoto;
extern lv_obj_t* image_ev_scr_uiflowphoto;
extern lv_obj_t* image_wb_scr_uiflowphoto;
extern lv_obj_t* image_battery_scr_uiflowphoto;
extern lv_obj_t* image_storage_scr_uiflowphoto;
extern lv_obj_t* image_quality_scr_uiflowphoto;
extern lv_obj_t* label_size_scr_uiflowphoto;
extern lv_obj_t* image_self_timer_scr_uiflowphoto;
extern lv_obj_t* label_free_pic_scr_uiflowphoto;
extern lv_obj_t* image_hdr_scr_uiflowphoto;
extern lv_obj_t* image_bust_scr_uiflowphoto;
extern lv_obj_t* label_self_timer_cnt_scr_uiflowphoto;
extern lv_obj_t* image_fd_scr_uiflowphoto;
extern lv_obj_t* container_fd_frame_scr_uiflowphoto;
extern lv_obj_t* label_dzoom_scr_uiflowphoto;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowPhoto_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWPHOTO_H*/