#ifndef UIFLOWUSB_H
#define UIFLOWUSB_H

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
void UIFlowUSBEventCallback(lv_obj_t* obj, lv_event_t event);


extern lv_obj_t* container_1_scr_uiflowusb;
extern lv_obj_t* label_usbmode_scr_uiflowusb;
extern lv_obj_t* image_usb_scr_uiflowusb;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t* UIFlowUSB_create(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UIFLOWUSB_H*/