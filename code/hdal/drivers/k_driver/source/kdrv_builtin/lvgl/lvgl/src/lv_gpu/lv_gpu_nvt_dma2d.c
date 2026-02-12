/**
 * @file lv_gpu_nvt_dma2d.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../lv_misc/lv_math.h"
#include "../lv_hal/lv_hal_disp.h"
#include "../lv_core/lv_refr.h"

#include "lv_gpu_nvt_dma2d.h"
#include "hdal.h"
#include "vendor_gfx.h"

#if LV_USE_GPU_NVT_DMA2D

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_color_t* get_working_buffer(void);
static HD_VIDEO_PXLFMT color_depth_to_hd_video_pxlfmt(void);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_res_t lv_gpu_nvt_dma2d_init()
{
	/* test working buffer */
	if(!get_working_buffer()){
		LV_LOG_ERROR("can't get working buffer!");
		return LV_RES_INV;
	}

	return LV_RES_OK;
}

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
)
{

#if LV_COLOR_DEPTH == 24

	UINT32 dst_pa, src_pa;
	HD_GFX_COPY			param;
	HD_RESULT           ret;
	HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
	HD_VIDEO_PXLFMT     fmt;
	lv_coord_t 			copy_w = lv_area_get_width(area);
	lv_coord_t 			copy_h = lv_area_get_height(area);
	const static uint8_t pixel_size_rgb = sizeof(LV_COLOR_BLACK.full);
	const static uint8_t pixel_size_alpha = sizeof(LV_COLOR_BLACK.ext_ch.alpha);
	const uint32_t alpha_offset_src = LV_VER_RES_MAX * LV_HOR_RES_MAX * pixel_size_rgb;
	uint32_t alpha_offset_dst = dst_w * dst_h * pixel_size_rgb;

	LV_UNUSED(color_depth_to_hd_video_pxlfmt);

	vir_meminfo.va = (void *)(src);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		LV_LOG_ERROR("convert map failed!");
		return LV_RES_INV;
	}

	src_pa = vir_meminfo.pa;

	vir_meminfo.va = (void *)(dst);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		LV_LOG_ERROR("convert buf failed!");
		return LV_RES_INV;
	}

	dst_pa = vir_meminfo.pa;


	fmt = HD_VIDEO_PXLFMT_RGB565;
	memset(&param, 0, sizeof(HD_GFX_COPY));
	param.src_img.dim.w            = LV_HOR_RES_MAX;
	param.src_img.dim.h            = LV_VER_RES_MAX;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.lineoffset[0]    = LV_HOR_RES_MAX * pixel_size_rgb;
	param.src_img.format           = fmt;

	param.dst_img.dim.w            = dst_w;
	param.dst_img.dim.h            = dst_h;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.lineoffset[0]    = dst_w * pixel_size_rgb;
	param.dst_img.format           = fmt;

	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = copy_w;
	param.src_region.h             = copy_h;

	param.dst_pos.x                = dst_x + area->x1;
	param.dst_pos.y                = dst_y + area->y1;
	param.colorkey                 = 0;
	param.alpha                    = 255;

	if(flush){
		ret = hd_gfx_copy(&param);
	}
	else{
		ret = vendor_gfx_copy_no_flush(&param);
	}

	if(ret != HD_OK){
		LV_LOG_ERROR("ret = %d", ret);
		return LV_RES_INV;
	}


	fmt = HD_VIDEO_PXLFMT_I8;

	memset(&param, 0, sizeof(HD_GFX_COPY));
	param.src_img.dim.w            = LV_HOR_RES_MAX;
	param.src_img.dim.h            = LV_VER_RES_MAX;
	param.src_img.p_phy_addr[0]    = src_pa + alpha_offset_src;
	param.src_img.lineoffset[0]    = LV_HOR_RES_MAX * pixel_size_alpha;
	param.src_img.format           = fmt;

	param.dst_img.dim.w            = dst_w;
	param.dst_img.dim.h            = dst_h;
	param.dst_img.p_phy_addr[0]    = dst_pa + alpha_offset_dst;
	param.dst_img.lineoffset[0]    = dst_w * pixel_size_alpha;
	param.dst_img.format           = fmt;

	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = copy_w;
	param.src_region.h             = copy_h;

	param.dst_pos.x                = dst_x + area->x1;
	param.dst_pos.y                = dst_y + area->y1;
	param.colorkey                 = 0;
	param.alpha                    = 255;

	if(flush){
		ret = hd_gfx_copy(&param);
	}
	else{
		ret = vendor_gfx_copy_no_flush(&param);
	}

	if(ret != HD_OK){
		LV_LOG_ERROR("ret = %d", ret);
		return LV_RES_INV;
	}

#elif LV_COLOR_DEPTH == 32 || LV_COLOR_DEPTH == 8

	UINT32 dst_pa, src_pa;
	HD_GFX_COPY			param;
	HD_RESULT           ret;
	HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
	HD_VIDEO_PXLFMT		fmt = color_depth_to_hd_video_pxlfmt();
	lv_coord_t 			copy_w = lv_area_get_width(src_area);
	lv_coord_t 			copy_h = lv_area_get_height(src_area);


	vir_meminfo.va = (void *)(src);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		LV_LOG_ERROR("convert src failed!");
		return LV_RES_INV;
	}

	src_pa = vir_meminfo.pa;

	vir_meminfo.va = (void *)(dst);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		LV_LOG_ERROR("convert dst failed!");
		return LV_RES_INV;
	}

	dst_pa = vir_meminfo.pa;

	memset(&param, 0, sizeof(HD_GFX_COPY));
	param.src_img.dim.w            = src_w;
	param.src_img.dim.h            = src_h;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.lineoffset[0]    = src_w * sizeof(lv_color_t);
	param.src_img.format           = fmt;

	param.dst_img.dim.w            = dst_w;
	param.dst_img.dim.h            = dst_h;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.lineoffset[0]    = dst_w * sizeof(lv_color_t);
	param.dst_img.format           = fmt;

	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = copy_w;
	param.src_region.h             = copy_h;

	param.dst_pos.x                = dst_x + src_area->x1;
	param.dst_pos.y                = dst_y + src_area->y1;
	param.colorkey                 = 0;
	param.alpha                    = 255;

	if(flush){
		ret = hd_gfx_copy(&param);
	}
	else{
		ret = vendor_gfx_copy_no_flush(&param);
	}

	if(ret != HD_OK){
		LV_LOG_ERROR("ret = %d src { %lu, %lu , %lu, %lu}  dst{ %lu, %lu , %lu, %lu} x1 = %lu , x2 = %lu",
				ret, src_w, src_h, copy_w, copy_h, dst_w, dst_h, dst_x, dst_y,
				src_area->x1, src_area->y1
		);



		return LV_RES_INV;
	}

#endif

	return LV_RES_OK;
}

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
		bool flush)
{

#if LV_COLOR_DEPTH == 24

	UINT32 					dst_pa = 0;
	UINT32 					src_pa = 0;
	HD_GFX_ROTATE			param = {0};
	HD_RESULT           	ret = HD_OK;
	HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
	lv_coord_t 				copy_w = lv_area_get_width(src_area);
	lv_coord_t 				copy_h = lv_area_get_height(src_area);
	bool 					swap_xy = false;
	HD_IPOINT               dst_pos = {-1, -1};

	const uint8_t 			pixel_size_rgb = sizeof(LV_COLOR_BLACK.full);
	const uint8_t 			pixel_size_alpha = sizeof(LV_COLOR_BLACK.ext_ch.alpha);
	uint32_t 				alpha_offset_src = LV_VER_RES_MAX * LV_HOR_RES_MAX * pixel_size_rgb;
	uint32_t 				alpha_offset_dst = dst_w * dst_h * pixel_size_rgb;

	LV_UNUSED(color_depth_to_hd_video_pxlfmt);

	/* convert va to pa */
	vir_meminfo.va = (void *)(src);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		LV_LOG_ERROR("convert map failed!\n");
		return;
	}

	src_pa = vir_meminfo.pa;

	vir_meminfo.va = (void *)(dst);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		LV_LOG_ERROR("convert buf failed!\n");
		return;
	}

	dst_pa = vir_meminfo.pa;


	memset(&param, 0, sizeof(HD_GFX_ROTATE));

	/* check dir */

	switch(dir)
	{
	case HD_VIDEO_DIR_ROTATE_90:
		dst_pos = (HD_IPOINT){dst_h - src_area.y1 - copy_h - dst_y, src_area.x1 + dst_x};
		swap_xy = true;
		break;

	case HD_VIDEO_DIR_ROTATE_270:
		dst_pos = (HD_IPOINT){src_area.y1 + dst_y, dst_w - src_area.x1 - copy_w - dst_x};
		swap_xy = true;
		break;

	case HD_VIDEO_DIR_ROTATE_180:
		dst_pos = (HD_IPOINT){dst_w - src_area.x1 - copy_w - dst_x, dst_h - src_area.y1 - copy_h - dst_y};
		swap_xy = false;
		break;

	default:
		LV_LOG_ERROR("dir(%lx) is currently not supported", dir);
		return;
	}

	/* rotate rgb channels */

	param.src_img.dim.w            = LV_HOR_RES_MAX;
	param.src_img.dim.h            = LV_VER_RES_MAX;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.lineoffset[0]    = LV_HOR_RES_MAX * pixel_size_rgb;
	param.src_img.format           = HD_VIDEO_PXLFMT_RGB565;

	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = copy_w;
	param.src_region.h             = copy_h;

	param.dst_img.format           = HD_VIDEO_PXLFMT_RGB565;
	param.dst_img.dim.w            = swap_xy ? dst_h : dst_w;
	param.dst_img.dim.h            = swap_xy ? dst_w : dst_h;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.lineoffset[0]    = (swap_xy ? dst_h : dst_w) * pixel_size_rgb;
	param.dst_pos 				   = dst_pos;
	param.angle					   = dir;

	ret = hd_gfx_rotate(&param);

	if(ret != HD_OK){
		LV_LOG_ERROR("ret = %d\n", ret);
	}

	/* rotate alpha channel */
	param.src_img.dim.w            = LV_HOR_RES_MAX;
	param.src_img.dim.h            = LV_VER_RES_MAX;
	param.src_img.p_phy_addr[0]    = src_pa + alpha_offset_src;
	param.src_img.lineoffset[0]    = LV_HOR_RES_MAX * pixel_size_alpha;
	param.src_img.format           = HD_VIDEO_PXLFMT_I8;

	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = copy_w;
	param.src_region.h             = copy_h;

	param.dst_img.format           = HD_VIDEO_PXLFMT_I8;
	param.dst_img.dim.w            = swap_xy ? dst_h : dst_w;
	param.dst_img.dim.h            = swap_xy ? dst_w : dst_h;
	param.dst_img.p_phy_addr[0]    = dst_pa + alpha_offset_dst;
	param.dst_img.lineoffset[0]    = (swap_xy ? dst_h : dst_w) * pixel_size_alpha;
	param.dst_pos 				   = dst_pos;
	param.angle					   = dir;

	if(flush){
		ret = hd_gfx_rotate(&param);
	}
	else{
		ret = vendor_gfx_rotate_no_flush(&param);
	}

	if(ret != HD_OK){
		LV_LOG_ERROR("ret = %d\n", ret);
	}

#else

	UINT32 					dst_pa = 0;
	UINT32 					src_pa = 0;
	HD_GFX_ROTATE			param = {0};
	HD_RESULT           	ret = HD_OK;
	HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
	HD_VIDEO_PXLFMT			fmt = color_depth_to_hd_video_pxlfmt();
	lv_coord_t 				copy_w = lv_area_get_width(src_area);
	lv_coord_t 				copy_h = lv_area_get_height(src_area);
	bool 					swap_xy = false;
	HD_IPOINT               dst_pos = {-1, -1};

	/* convert va to pa */
	vir_meminfo.va = (void *)(src);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		LV_LOG_ERROR("convert map failed!\n");
		return;
	}

	src_pa = vir_meminfo.pa;

	vir_meminfo.va = (void *)(dst);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		LV_LOG_ERROR("convert buf failed!\n");
		return;
	}

	dst_pa = vir_meminfo.pa;


	memset(&param, 0, sizeof(HD_GFX_ROTATE));

	/* check dir */
	switch(dir)
	{
	case HD_VIDEO_DIR_ROTATE_90:
		dst_pos = (HD_IPOINT){dst_h - src_area->y1 - copy_h - dst_y, src_area->x1 + dst_x};
		swap_xy = true;
		break;

	case HD_VIDEO_DIR_ROTATE_270:
		dst_pos = (HD_IPOINT){src_area->y1 + dst_y, dst_w - src_area->x1 - copy_w - dst_x};
		swap_xy = true;
		break;

	case HD_VIDEO_DIR_ROTATE_180:
		dst_pos = (HD_IPOINT){dst_w - src_area->x1 - copy_w - dst_x, dst_h - src_area->y1 - copy_h - dst_y};
		swap_xy = false;
		break;

	default:
		LV_LOG_ERROR("dir(%lx) is currently not supported", dir);
		return;
	}

	/* rotate rgb channels */

	param.src_img.dim.w            = copy_w;
	param.src_img.dim.h            = copy_h;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.lineoffset[0]    = copy_w * sizeof(lv_color_t);
	param.src_img.format           = fmt;

	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = copy_w;
	param.src_region.h             = copy_h;

	param.dst_img.format           = fmt;
	param.dst_img.dim.w            = swap_xy ? dst_h : dst_w;
	param.dst_img.dim.h            = swap_xy ? dst_w : dst_h;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.lineoffset[0]    = (swap_xy ? dst_h : dst_w) * sizeof(lv_color_t);
	param.dst_pos 				   = dst_pos;
	param.angle					   = dir;

	if(flush){
		ret = hd_gfx_rotate(&param);
	}
	else{
		ret = vendor_gfx_rotate_no_flush(&param);
	}

	if(ret != HD_OK){
		LV_LOG_ERROR("ret = %d\n", ret);
	}

#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static lv_color_t* get_working_buffer(void)
{
	static lv_color_t* working_buffer = NULL;

	if(working_buffer == NULL){

		extern UINT32 mempool_lv_temp;
		working_buffer = (lv_color_t *)mempool_lv_temp;
	}

	return working_buffer;
}

static HD_VIDEO_PXLFMT color_depth_to_hd_video_pxlfmt(void)
{
#if LV_COLOR_DEPTH == 8

	return HD_VIDEO_PXLFMT_I8;

#elif LV_COLOR_DEPTH == 24

	return HD_VIDEO_PXLFMT_ARGB8565;

#elif LV_COLOR_DEPTH == 32

	return HD_VIDEO_PXLFMT_ARGB8888;

#else

	LV_LOG_ERROR("LV_COLOR_DEPTH(%u) not support!", LV_COLOR_DEPTH);
	return HD_VIDEO_PXLFMT_NONE;

#endif

}


#endif
