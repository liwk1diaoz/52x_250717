/**
 * @file lv_gpu_stm32_dma2d.h
 *
 */

#ifndef LV_GPU_STM32_DMA2D_H
#define LV_GPU_STM32_DMA2D_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_misc/lv_area.h"
#include "../lv_misc/lv_color.h"

/*********************
 *      DEFINES
 *********************/
#define LV_GPU_NVT_WORKING_BUFFER_SIZE 			(LV_COLOR_SIZE * 1024 * 1)

/* pixel size */
#define LV_GPU_NVT_FILL_WITH_ALPHA_SIZE_LIMIT 	32
#define LV_GPU_NVT_FILL_SIZE_LIMIT 				32
#define LV_GPU_NVT_BLEND_SIZE_LIMIT 			32
#define LV_GPU_NVT_COPY_SIZE_LIMIT 				32

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_res_t lv_gpu_nvt_dma2d_init(void);

lv_res_t lv_gpu_nvt_dma2d_copy(
		const lv_color_t * src,
		lv_coord_t src_w,
		lv_coord_t src_h,
		const lv_area_t* src_area,
		lv_color_t * dst,
		lv_coord_t dst_w,
		lv_coord_t dst_h,
		const lv_coord_t dst_x,
		const lv_coord_t dst_y,
		bool flush
);

void lv_gpu_nvt_dma2d_rotate(
		lv_color_t * src,
		lv_coord_t src_w,
		lv_coord_t src_h,
		const lv_area_t* src_area,
		lv_color_t * dst,
		lv_coord_t dst_w,
		lv_coord_t dst_h,
		const lv_coord_t dst_x,
		const lv_coord_t dst_y,
		uint32_t dir,
		bool flush);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_GPU_STM32_DMA2D_H*/
