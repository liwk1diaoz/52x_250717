#ifndef UIFLOWWIFILINKOK_H
#define UIFLOWWIFILINKOK_H

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
void UIFlowWifiLinkOKEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* button_select_msg_scr_uiflowwifilinkok;
extern lv_obj_t* label_select_msg_scr_uiflowwifilinkok;
extern lv_obj_t* container_1_scr_uiflowwifilinkok;
extern lv_obj_t* label_2_scr_uiflowwifilinkok;
extern lv_obj_t* label_mac_scr_uiflowwifilinkok;
extern lv_obj_t* label_wifista_scr_uiflowwifilinkok;
extern lv_obj_t* label_1_scr_uiflowwifilinkok;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowWifiLinkOK_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWWIFILINKOK_H*/