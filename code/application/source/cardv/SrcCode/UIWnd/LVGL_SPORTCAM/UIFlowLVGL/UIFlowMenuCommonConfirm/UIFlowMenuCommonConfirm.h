#ifndef UIFLOWMENUCOMMONCONFIRM_H
#define UIFLOWMENUCOMMONCONFIRM_H

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
void UIFlowMenuCommonConfirmEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* message_box_1_scr_uiflowmenucommonconfirm;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowMenuCommonConfirm_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWMENUCOMMONCONFIRM_H*/