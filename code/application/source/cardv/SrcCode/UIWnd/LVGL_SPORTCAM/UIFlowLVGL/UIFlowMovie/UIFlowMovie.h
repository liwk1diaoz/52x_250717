#ifndef UIFLOWMOVIE_H
#define UIFLOWMOVIE_H

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
void UIFlowMovieEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* image_storage_scr_uiflowmovie;
extern lv_obj_t* image_mode_video_scr_uiflowmovie;
extern lv_obj_t* image_battery_scr_uiflowmovie;
extern lv_obj_t* label_date_scr_uiflowmovie;
extern lv_obj_t* label_time_scr_uiflowmovie;
extern lv_obj_t* image_ev_scr_uiflowmovie;
extern lv_obj_t* label_rec_time_scr_uiflowmovie;
extern lv_obj_t* image_rec_ellipse_scr_uiflowmovie;
extern lv_obj_t* image_pim_scr_uiflowmovie;
extern lv_obj_t* label_size_scr_uiflowmovie;
extern lv_obj_t* image_cyclic_rec_scr_uiflowmovie;
extern lv_obj_t* image_hdr_scr_uiflowmovie;
extern lv_obj_t* image_wifi_scr_uiflowmovie;
extern lv_obj_t* image_motiondetect_scr_uiflowmovie;
extern lv_obj_t* label_zoom_scr_uiflowmovie;
extern lv_obj_t* label_maxtime_scr_uiflowmovie;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowMovie_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWMOVIE_H*/