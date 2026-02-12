/**
 * @file fbdev.h
 *
 */

#ifndef FBDEV_H
#define FBDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifndef LV_DRV_NO_CONF
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "../lv_drv_conf.h"
#else
#include "../lv_drv_conf.h"
#endif
#endif

#if USE_FBDEV || USE_BSD_FBDEV

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void fbdev_init(void);
void fbdev_exit(void);
void fbdev_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p);
void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
void lv_user_rounder_callback(lv_disp_drv_t* disp_drv,lv_area_t* area);

#if LV_USE_GPU_NVT_DMA2D

#if LV_COLOR_DEPTH == 24


void set_px_cb_8565(
		struct _disp_drv_t * disp_drv,
		uint8_t * buf, lv_coord_t buf_w,
		lv_coord_t x, lv_coord_t y,
		lv_color_t color,
		lv_opa_t opa);


#endif

#endif

typedef void (*fbdev_flush_callback)(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

extern char *fbp;
extern long int screensize;
/**********************
 *      MACROS
 **********************/

#endif  /*USE_FBDEV*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*FBDEV_H*/

