#ifndef UIFLOWWIFIWAIT_H
#define UIFLOWWIFIWAIT_H

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
void UIFlowWifiWaitEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* container_1_scr_uiflowwifiwait;
extern lv_obj_t* spinner_1_scr_uiflowwifiwait;
extern lv_obj_t* container_2_scr_uiflowwifiwait;
extern lv_obj_t* image_cap_scr_uiflowwifiwait;
extern lv_obj_t* image_con_scr_uiflowwifiwait;
extern lv_obj_t* image_wifi_scr_uiflowwifiwait;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowWifiWait_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWWIFIWAIT_H*/