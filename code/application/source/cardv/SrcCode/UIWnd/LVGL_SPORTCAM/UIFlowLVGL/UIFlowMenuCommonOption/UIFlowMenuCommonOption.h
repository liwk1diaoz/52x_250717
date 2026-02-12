#ifndef UIFLOWMENUCOMMONOPTION_H
#define UIFLOWMENUCOMMONOPTION_H

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
void UIFlowMenuCommonOptionEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* container_main_menu_scr_uiflowmenucommonoption;
extern lv_obj_t* image_button_1_scr_uiflowmenucommonoption;
extern lv_obj_t* image_1_scr_uiflowmenucommonoption;
extern lv_obj_t* image_button_2_scr_uiflowmenucommonoption;
extern lv_obj_t* image_2_scr_uiflowmenucommonoption;
extern lv_obj_t* image_button_3_scr_uiflowmenucommonoption;
extern lv_obj_t* image_3_scr_uiflowmenucommonoption;
extern lv_obj_t* image_button_4_scr_uiflowmenucommonoption;
extern lv_obj_t* image_4_scr_uiflowmenucommonoption;
extern lv_obj_t* container_1_scr_uiflowmenucommonoption;
extern lv_obj_t* label_menu_item_scr_uiflowmenucommonoption;
extern lv_obj_t* label_menu_option_scr_uiflowmenucommonoption;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowMenuCommonOption_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWMENUCOMMONOPTION_H*/