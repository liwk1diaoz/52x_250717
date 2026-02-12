#ifndef UIFLOWSETUPDATETIME_H
#define UIFLOWSETUPDATETIME_H

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
void UIFlowSetupDateTimeEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* button_matrix_datetime_scr_uiflowsetupdatetime;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowSetupDateTime_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWSETUPDATETIME_H*/