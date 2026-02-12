#ifndef UIFLOWMENUCOMMONITEM_H
#define UIFLOWMENUCOMMONITEM_H

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
void UIFlowMenuCommonItemEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* container_main_menu_scr_uiflowmenucommonitem;
extern lv_obj_t* image_button_1_scr_uiflowmenucommonitem;
extern lv_obj_t* image_1_scr_uiflowmenucommonitem;
extern lv_obj_t* image_button_2_scr_uiflowmenucommonitem;
extern lv_obj_t* image_2_scr_uiflowmenucommonitem;
extern lv_obj_t* image_button_3_scr_uiflowmenucommonitem;
extern lv_obj_t* image_3_scr_uiflowmenucommonitem;
extern lv_obj_t* image_button_4_scr_uiflowmenucommonitem;
extern lv_obj_t* image_4_scr_uiflowmenucommonitem;
extern lv_obj_t* container_1_scr_uiflowmenucommonitem;
extern lv_obj_t* label_menu_item_scr_uiflowmenucommonitem;
extern lv_obj_t* label_menu_option_scr_uiflowmenucommonitem;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowMenuCommonItem_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWMENUCOMMONITEM_H*/