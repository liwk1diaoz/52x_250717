#ifndef UIFLOWWIFILINK_H
#define UIFLOWWIFILINK_H

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
void UIFlowWifiLinkEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* container_select_scr_uiflowwifilink;
extern lv_obj_t* button_ref_scr_uiflowwifilink;
extern lv_obj_t* label_ref_scr_uiflowwifilink;
extern lv_obj_t* button_wifioff_scr_uiflowwifilink;
extern lv_obj_t* label_wifioff_scr_uiflowwifilink;
extern lv_obj_t* container_1_scr_uiflowwifilink;
extern lv_obj_t* label_1_scr_uiflowwifilink;
extern lv_obj_t* label_2_scr_uiflowwifilink;
extern lv_obj_t* label_ssid_scr_uiflowwifilink;
extern lv_obj_t* label_pwa2_scr_uiflowwifilink;
extern lv_obj_t* label_3_scr_uiflowwifilink;
extern lv_obj_t* label_wifimode_scr_uiflowwifilink;
extern lv_obj_t* image_wifista_scr_uiflowwifilink;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowWifiLink_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWWIFILINK_H*/