#ifndef UIFLOWUSBMENU_H
#define UIFLOWUSBMENU_H

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
void UIFlowUSBMenuEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* container_usbmenu_scr_uiflowusbmenu;
extern lv_obj_t* button_1_scr_uiflowusbmenu;
extern lv_obj_t* label_usbt_scr_uiflowusbmenu;
extern lv_obj_t* image_1_scr_uiflowusbmenu;
extern lv_obj_t* button_msdc_scr_uiflowusbmenu;
extern lv_obj_t* label_msdc_scr_uiflowusbmenu;
extern lv_obj_t* image_2_scr_uiflowusbmenu;
extern lv_obj_t* button_pcc_scr_uiflowusbmenu;
extern lv_obj_t* label_pcc_scr_uiflowusbmenu;
extern lv_obj_t* image_3_scr_uiflowusbmenu;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowUSBMenu_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWUSBMENU_H*/