#ifndef UIFLOWWRNMSG_H
#define UIFLOWWRNMSG_H

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
void UIFlowWrnMsgEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* message_box_1_scr_uiflowwrnmsg;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowWrnMsg_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWWRNMSG_H*/