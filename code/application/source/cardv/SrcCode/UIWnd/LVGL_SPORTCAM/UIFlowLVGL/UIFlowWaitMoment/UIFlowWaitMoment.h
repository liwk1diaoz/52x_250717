#ifndef UIFLOWWAITMOMENT_H
#define UIFLOWWAITMOMENT_H

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
void UIFlowWaitMomentEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* container_1_scr_uiflowwaitmoment;
extern lv_obj_t* label_1_scr_uiflowwaitmoment;
extern lv_obj_t* spinner_1_scr_uiflowwaitmoment;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowWaitMoment_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWWAITMOMENT_H*/