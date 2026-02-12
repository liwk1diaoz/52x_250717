/**
* @file fbdev.c
*
*/

/*********************
*      INCLUDES
*********************/
#include "fbdev.h"
#include "hd_gfx.h"
#include "vendor_gfx.h"
#include "hd_common.h"
#include <kwrap/perf.h>
#include <kwrap/util.h>
#if defined(_UI_STYLE_LVGL_)
#include "PrjInc.h"
#endif
#if USE_FBDEV || USE_BSD_FBDEV
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#if !defined(__FREERTOS)
#include <sys/mman.h>
#include <sys/ioctl.h>
#endif

#if USE_BSD_FBDEV
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/consio.h>
#include <sys/fbio.h>
#else  /* USE_BSD_FBDEV */
#if !defined(__FREERTOS)
#include <linux/fb.h>
#endif
#endif /* USE_BSD_FBDEV */

/*********************
*      DEFINES
*********************/
#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#endif

//#define FBDEV_DBG
#ifdef FBDEV_DBG
#define fbdev_msg printf
#else
#define fbdev_msg(...)
#endif


//#define FBDEV_PERF
#ifdef FBDEV_PERF
#define fbdev_perf printf
#else
#define fbdev_perf(...)
#endif
/**********************
*      TYPEDEFS
**********************/

/**********************
*      STRUCTURES
**********************/
#if (USE_BSD_FBDEV || defined(__FREERTOS))
struct bsd_fb_var_info{
	int32_t xoffset;
	int32_t yoffset;
	int32_t xres;
	int32_t yres;
	int bits_per_pixel;
};

struct bsd_fb_fix_info{
	long int line_length;
	long int smem_len;
};
#endif

/**********************
*  Function Declaration
**********************/
HD_RESULT LVGL_rotate_fb(UINT32 dst_pa, UINT32 src_pa,lv_disp_drv_t * disp_drv,UINT32 fmt);

/**********************
*  STATIC VARIABLES
**********************/
#if USE_BSD_FBDEV
static struct bsd_fb_var_info vinfo;
static struct bsd_fb_fix_info finfo;
#else
#if !defined(__FREERTOS)
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
#else
#include "sys_mempool.h"
#include "vendor_videoout.h"
static struct bsd_fb_var_info vinfo;
//static struct bsd_fb_fix_info finfo;
#define FBIOPAN_DISPLAY  (0x1)
#endif
#endif /* USE_BSD_FBDEV */

char *fbp = 0;
long int screensize = 0;
static UINT32 buf1_pa;

#if LV_USE_GPU_NVT_DMA2D

#include "lvgl/src/lv_gpu/lv_gpu_nvt_dma2d.h"

#endif

static int fbfd = 0;
/**********************
*      MACROS
**********************/

/**********************
*   GLOBAL FUNCTIONS
**********************/

/*********************************************
 * IMPORTANT!
 *
 * Do not edit lv_user_rounder_callback
 *
 *********************************************/
void lv_user_rounder_callback(
    lv_disp_drv_t* disp_drv,
    lv_area_t* area)
{
#if LV_USE_GPU_NVT_DMA2D == 1

#if LV_USER_CFG_USE_ROTATE_BUFFER
	uint8_t alignment = 16;
#else
	uint8_t alignment = 4;
#endif

	area->x1 = ALIGN_FLOOR(area->x1, alignment);
	area->y1 = ALIGN_FLOOR(area->y1, alignment);

	lv_coord_t copy_w = lv_area_get_width(area);
	lv_coord_t copy_h = lv_area_get_height(area);
	uint8_t remainder_copy_w = copy_w % alignment;
	uint8_t remainder_copy_h = copy_h % alignment;

	if(area->x1 == area->x2){
		area->x2 += (alignment - 1);
	}
	else if(remainder_copy_w){
		area->x2 += (alignment - remainder_copy_w);
	}


	if( (area->y1 == area->y2)){
		area->y2 += (alignment - 1);
	}
	else if(remainder_copy_h){
		area->y2 += (alignment - remainder_copy_h);
	}

	/* check out of area, LV_MAX_RES_HOR & LV_MAX_RES_VER should be aligned too.  */
	if(area->x2 > (disp_drv->hor_res - 1))
		area->x2 = disp_drv->hor_res - 1;

	if(area->y2 > (disp_drv->ver_res - 1))
		area->y2 = disp_drv->ver_res - 1;

#endif
}

/**
* Flush a buffer to the marked area
* @param x1 left coordinate
* @param y1 top coordinate
* @param x2 right coordinate
* @param y2 bottom coordinate
* @param color_p an array of colors
*/
void fbdev_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{
	if(fbp == NULL) return;

	fbdev_msg("fbdev_flush : %d,%d,%d,%d   {%lu, %lu, %lu, %lu}\r\n",x1,y1,x2,y2, vinfo.xres, vinfo.xoffset, vinfo.yres, vinfo.yoffset);

	/*Return if the area is out the screen*/
	if(x2 < 0) return;
	if(y2 < 0) return;
	if(x1 > (int32_t)(vinfo.xres) - 1) return;
	if(y1 > (int32_t)(vinfo.yres) - 1) return;

	/*Truncate the area to the screen*/
	int32_t act_x1 = x1 < 0 ? 0 : x1;
	int32_t act_y1 = y1 < 0 ? 0 : y1;
	int32_t act_x2 = x2 > (int32_t)(vinfo.xres) - 1 ? (int32_t)(vinfo.xres) - 1 : x2;
	int32_t act_y2 = y2 > (int32_t)(vinfo.yres) - 1 ? (int32_t)(vinfo.yres) - 1 : y2;

	long int location = 0;

	/*32 or 24 bit per pixel*/
	if(vinfo.bits_per_pixel == 32) {

		uint32_t *fbp32 = (uint32_t*)fbp;

		for(int32_t y = act_y1; y <= act_y2; y++) {
			for(int32_t x = act_x1; x <= act_x2; x++) {

				location = x + y * vinfo.xres;
				fbp32[location] = lv_color_to32(*color_p);
				color_p++;
			}

			color_p += x2 - act_x2;
		}
	}
	/*16 bit per pixel*/
	else if(vinfo.bits_per_pixel == 16) {

		uint16_t *fbp16 = (uint16_t*)fbp;

		for(int32_t y = act_y1; y <= act_y2; y++) {
			for(int32_t x = act_x1; x <= act_x2; x++) {

				location = x + y * vinfo.xres;
				fbp16[location] = color_p->full;
				color_p++;
			}

			color_p += x2 - act_x2;
		}
	}
	/**********************************************************************************
	 * 24 bit per pixel
	 *
	 * IMPORTANT!!
	 *
	 * set_px_cb is set in LV_COLOR_DEPTH = 24, color_p is already a two plane buffer,
	 * do not get pixel data by accessing member of lv_color_t
	 * **********************************************************************************/
	else if(vinfo.bits_per_pixel == 24) {

		/* RGB plane*/
		uint16_t *fbp16 = (uint16_t*)fbp;
		uint16_t *color_p16 = (uint16_t*)color_p;

		/* Alpha plane, 565 = 2 bytes, shift to alpha address */
		uint8_t *fbp8 = (uint8_t*)fbp;
		uint8_t *color_p8 = (uint8_t*)color_p;

		fbp8 += (vinfo.yres * vinfo.xres * sizeof(color_p->full));
		color_p8 += (vinfo.yres * vinfo.xres * sizeof(color_p->full));

		for(int32_t y = act_y1 ; y <= act_y2 ; y++)
		{
			for( int32_t x = act_x1 ; x <= act_x2 ; x++)
			{
				location = x + y * vinfo.xres;
				fbp16[location] = color_p16[location];
				fbp8[location] = color_p8[location];
			}
		}
	}
	/*8 bit per pixel*/
	else if(vinfo.bits_per_pixel == 8) {

		uint8_t *fbp8 = (uint8_t*)fbp;

		for(int32_t y = act_y1 ; y <= act_y2 ; y++)
		{
			for(int32_t x = act_x1 ; x <= act_x2 ; x++)
			{
				location = x + y * vinfo.xres;
				fbp8[location] = color_p->full;
				color_p++;
			}

			color_p += x2 - act_x2;
		}

	} else {

		LV_LOG_ERROR("current vinfo.bits_per_pixel(%d) is not supported!", vinfo.bits_per_pixel);

	}
}

#if !defined(__FREERTOS)

void fbdev_init(void)
{
	// Open the file for reading and writing
	int count =1000;
    HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
	int ERcode;

	 while(count){
    	fbfd = open(FBDEV_PATH, O_RDWR);
    	if (fbfd<0)	{
            usleep(10);
            count--;
    	} else {
    	    break;
    	}
    }

	if(fbfd == -1) {
		perror("Error: cannot open framebuffer device");
		return;
	}
	fbdev_msg("The framebuffer device was opened successfully.~~~~\n");

	// Get fixed screen information
	if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error reading fixed information");
		return;
	}

	// Get variable screen information
	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return;
	}

	vinfo.xres_virtual = vinfo.xres;

#if LV_USER_CFG_USE_TWO_BUFFER
	vinfo.yres_virtual = vinfo.yres * 2;
#else
	vinfo.yres_virtual = vinfo.yres;
#endif

	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
		perror("Could not set variable screen info!\n");
		return;
	}

	fbdev_msg("The mem is :%d\n",finfo.smem_len);
	fbdev_msg("The line_length is :%d\n",finfo.line_length);
	fbdev_msg("The xres is :%d\n",vinfo.xres);
	fbdev_msg("The yres is :%d\n",vinfo.yres);
	fbdev_msg("bits_per_pixel is :%d\n",vinfo.bits_per_pixel);
	fbdev_msg("The yres_virtual is :%d\n",vinfo.yres_virtual);

	// Figure out the size of the screen in bytes
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	fbdev_msg("screen size = %u  xres = %u yres = %u bpp = %u\r\n", screensize, vinfo.xres , vinfo.yres , vinfo.bits_per_pixel);

	// Map the device to memory
	fbp = (char *)mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if((intptr_t)fbp == -1) {
		perror("Error: failed to map framebuffer device to memory");
		return;
	}
	memset(fbp, 0, screensize);

	fbdev_msg("The framebuffer device was mapped to memory successfully.\n");

	vir_meminfo.va = (void *)(fbp);
	ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
	if ( ERcode != HD_OK) {
		fbdev_msg("_DC_bltDestOffset map fail %d\n",ERcode);
		return;
	}
	buf1_pa = vir_meminfo.pa;

}

void fbdev_exit(void)
{
	close(fbfd);
}

#else  // __FREERTOS

#define ioctl fbdev_ioctl

int fbdev_ioctl(int f, unsigned int cmd, void *arg)
{
	int ret = 0;

	switch(cmd){
	case FBIOPAN_DISPLAY :
	{
        VENDOR_FB_INIT fb_init;
        struct bsd_fb_var_info *my_vinfo =arg;

        //assign bf buffer to FB_INIT
        fb_init.fb_id = HD_FB0;
        fb_init.pa_addr = (int) fbp + (my_vinfo->xres * my_vinfo->yoffset * (my_vinfo->bits_per_pixel / 8));
        fb_init.buf_len = (my_vinfo->xres * my_vinfo->yres * (my_vinfo->bits_per_pixel / 8));

    	fbdev_msg("pa_addr 0x%x buf_len 0x%x \r\n",fb_init.pa_addr,fb_init.buf_len);

    	ret = vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_INIT, &fb_init);
    	return ret;
    };
	break;
	default :
		printf("unsupport cmd = %d\r\n", cmd);
		break;
	};

	return ret;
}

void fbdev_init(void)
{
    screensize = OSD_SCREEN_SIZE;
    fbp = (char *)mempool_disp_osd1_pa;
    buf1_pa = mempool_disp_osd1_pa;

    memset(fbp,0,POOL_SIZE_OSD);
    memset((void *)&vinfo,0,sizeof(struct bsd_fb_var_info));
    vinfo.xoffset = 0;
    vinfo.yoffset = 0;
    vinfo.xres = DISPLAY_OSD_W;
    vinfo.yres = DISPLAY_OSD_H;
    vinfo.bits_per_pixel = LV_COLOR_DEPTH;

    fbfd =1;

	fbdev_msg("The xres is :%d\n",vinfo.xres);
	fbdev_msg("The yres is :%d\n",vinfo.yres);
	fbdev_msg("bits_per_pixel is :%d\n",vinfo.bits_per_pixel);
}

void fbdev_exit(void)
{
    fbfd =0;
}

#endif

#include "kwrap/debug.h"
#if LV_USE_GPU_NVT_DMA2D

	static UINT32 disp_get_dst_va(lv_disp_drv_t * disp_drv)
	{
		UINT32 dst_va;

	#if LV_USER_CFG_USE_TWO_BUFFER
		if(vinfo.yoffset == 0){
			dst_va = (UINT32)fbp + screensize;
		}
		else{
			dst_va = (UINT32)fbp;
		}
	#else
		dst_va = (UINT32)fbp;
	#endif

		return dst_va;
	}

	static void disp_set_pan_display(lv_disp_drv_t * disp_drv)
	{
	#if LV_USER_CFG_USE_TWO_BUFFER

		UINT32 fpb_scr, fpb_dst;
		const lv_area_t area = {0, 0, DISPLAY_OSD_W - 1, DISPLAY_OSD_H - 1};

		/* check is last chunk */
		if(disp_drv->buffer->flushing_last){

			if(vinfo.yoffset == 0){
				vinfo.yoffset = vinfo.yres;
				fpb_scr = (UINT32)fbp + screensize;
				fpb_dst = (UINT32)fbp;
			}
			else{
				vinfo.yoffset = 0;
				fpb_scr = (UINT32)fbp;
				fpb_dst = (UINT32)fbp + screensize;
			}

			if (ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo)) {
				fbdev_msg("Error to do pan display. %s\n",strerror(errno));
			}

			lv_gpu_nvt_dma2d_copy(
					(lv_color_t *) fpb_scr,
					DISPLAY_OSD_W,
					DISPLAY_OSD_H,
					&area,
					(lv_color_t *) fpb_dst,
					DISPLAY_OSD_W,
					DISPLAY_OSD_H,
					0,
					0,
					true
			);
		}

	#endif
	}


/* Flush the content of the internal buffer the specific area on the display
* You can use DMA or any hardware acceleration to do this operation in the background but
* 'lv_disp_flush_ready()' has to be called when finished. */

	#if (LV_COLOR_DEPTH == 24)

		#if LV_USER_CFG_USE_ROTATE_BUFFER

		void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
		{
			UINT32 dst_va = disp_get_dst_va(disp_drv);

			lv_gpu_nvt_dma2d_rotate(
					(lv_color_t *)color_p,
					disp_drv->hor_res,
					disp_drv->ver_res,
					area,
					(lv_color_t *)dst_va,
					DISPLAY_OSD_W,
					DISPLAY_OSD_H,
					DISPLAY_OSD_W_OFFSET,
					DISPLAY_OSD_H_OFFSET,
					VDO_ROTATE_DIR,
					true
			);

			disp_set_pan_display(disp_drv);

			lv_disp_flush_ready(disp_drv);
		}

		#else

		void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
		{
			UINT32 dst_va = disp_get_dst_va(disp_drv);

			lv_gpu_nvt_dma2d_copy(
					color_p,
					lv_area_get_width(area),
					lv_area_get_height(area),
					area,
					(lv_color_t *)dst_va,
					DISPLAY_OSD_W,
					DISPLAY_OSD_H,
					DISPLAY_OSD_W_OFFSET,
					DISPLAY_OSD_H_OFFSET,
					true
			);

			disp_set_pan_display(disp_drv);

			lv_disp_flush_ready(disp_drv);
		}

		#endif


	#else /* 32 & 8 */

		#if LV_USER_CFG_USE_ROTATE_BUFFER

		void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
		{
			UINT32 dst_va = disp_get_dst_va(disp_drv);

			lv_gpu_nvt_dma2d_rotate(
					(lv_color_t *)color_p,
					disp_drv->hor_res,
					disp_drv->ver_res,
					area,
					(lv_color_t *)dst_va,
					DISPLAY_OSD_W,
					DISPLAY_OSD_H,
					DISPLAY_OSD_W_OFFSET,
					DISPLAY_OSD_H_OFFSET,
					VDO_ROTATE_DIR,
					true
			);

			disp_set_pan_display(disp_drv);

			lv_disp_flush_ready(disp_drv);
		}

		#else

		void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
		{
			UINT32 dst_va = disp_get_dst_va(disp_drv);

			lv_gpu_nvt_dma2d_copy(
					color_p,
					lv_area_get_width(area),
					lv_area_get_height(area),
					area,
					(lv_color_t *)dst_va,
					DISPLAY_OSD_W,
					DISPLAY_OSD_H,
					DISPLAY_OSD_W_OFFSET,
					DISPLAY_OSD_H_OFFSET,
					true
			);

			disp_set_pan_display(disp_drv);

			lv_disp_flush_ready(disp_drv);
		}

		#endif

	#endif

#else
	/* Flush the content of the internal buffer the specific area on the display
	* You can use DMA or any hardware acceleration to do this operation in the background but
	* 'lv_disp_flush_ready()' has to be called when finished. */
	void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
	{
		fbdev_flush(area->x1,area->y1,area->x2,area->y2,color_p);
		lv_disp_flush_ready(disp_drv);
	}
#endif


#if LV_USE_GPU_NVT_DMA2D

#if LV_COLOR_DEPTH == 24

void set_px_cb_8565(
		struct _disp_drv_t * disp_drv,
		uint8_t * buf,
		lv_coord_t buf_w,
		lv_coord_t x,
		lv_coord_t y,
		lv_color_t color,
		lv_opa_t opa)
{
	/* buf should be disp_drv->buffer->buf_act */
	if(buf == disp_drv->buffer->buf_act){

		static const uint32_t px_size_rgb = sizeof(color.full);
		static const uint32_t offset = LV_HOR_RES_MAX * LV_VER_RES_MAX  * px_size_rgb;
		uint32_t location = x  + y * vinfo.xres;
		lv_color_t color_buf;
		lv_color_t color_mix;

		color_buf.full = *(((uint16_t*)buf) + location);
		color_buf.ext_ch.alpha = *(buf + offset + location);

		lv_color_mix_with_alpha(
				color_buf, (lv_opa_t) color_buf.ext_ch.alpha,
				color, opa,
				&color_mix, (lv_opa_t*)&color_mix.ext_ch.alpha);

		/* RGB plane*/
		*(((uint16_t*)buf) + location) = color_mix.full;


		/* Alpha plane, 565 = 2 bytes, shift to alpha address */
		*(buf + offset + location) = color_mix.ext_ch.alpha;

	}
	else{
		LV_LOG_WARN("buf != disp_drv->buffer->buf_act");
	}
}

#endif

#include "kwrap/debug.h"
#include "FileSysTsk.h"

#endif


/**********************
*   Local Functions
**********************/

//#if LV_COLOR_DEPTH == 24
//
//HD_RESULT disp_gfx_rotate_8565(UINT32 dst_pa, UINT32 src_pa, lv_disp_drv_t * disp_drv)
//{
//	HD_GFX_ROTATE       param;
//	HD_RESULT           ret;
//	HD_VIDEO_PXLFMT  fmt;
//
//	fmt = HD_VIDEO_PXLFMT_RGB565;
//
//	fbdev_msg("disp_gfx_rotate: dst_pa = 0x%x,src_pa = 0x%x fmt 0x%x\n", dst_pa,src_pa,fmt);
//
//	memset(&param, 0, sizeof(HD_GFX_ROTATE));
//	param.src_img.dim.w            = disp_drv->hor_res;
//	param.src_img.dim.h            = disp_drv->ver_res;
//	param.src_img.p_phy_addr[0]    = src_pa;
//	param.src_img.lineoffset[0]    = disp_drv->hor_res * 2;
//	param.src_img.format           = fmt;
//
//	param.dst_img.dim.w            = disp_drv->ver_res;
//	param.dst_img.dim.h            = disp_drv->hor_res;
//	param.dst_img.p_phy_addr[0]    = dst_pa;
//	param.dst_img.lineoffset[0]    = disp_drv->ver_res * 2;
//	param.dst_img.format           = fmt;
//
//	param.src_region.x             = 0;
//	param.src_region.y             = 0;
//	param.src_region.w             = disp_drv->hor_res;
//	param.src_region.h             = disp_drv->ver_res;
//
//	param.dst_pos.x                = 0;
//	param.dst_pos.y                = 0;
//	param.angle                    = VDO_ROTATE_DIR;
//
//	ret = hd_gfx_rotate(&param);
//
//
//	fmt = HD_VIDEO_PXLFMT_I8;
//
//	memset(&param, 0, sizeof(HD_GFX_ROTATE));
//	param.src_img.dim.w            = disp_drv->hor_res;
//	param.src_img.dim.h            = disp_drv->ver_res;
//	param.src_img.p_phy_addr[0]    = src_pa + disp_drv->hor_res * disp_drv->ver_res * 2;
//	param.src_img.lineoffset[0]    = disp_drv->hor_res;
//	param.src_img.format           = fmt;
//
//	param.dst_img.dim.w            = disp_drv->ver_res;
//	param.dst_img.dim.h            = disp_drv->hor_res;
//	param.dst_img.p_phy_addr[0]    = dst_pa + disp_drv->hor_res * disp_drv->ver_res * 2;
//	param.dst_img.lineoffset[0]    = disp_drv->ver_res;
//	param.dst_img.format           = fmt;
//
//	param.src_region.x             = 0;
//	param.src_region.y             = 0;
//	param.src_region.w             = disp_drv->hor_res;
//	param.src_region.h             = disp_drv->ver_res;
//
//	param.dst_pos.x                = 0;
//	param.dst_pos.y                = 0;
//	param.angle                    = VDO_ROTATE_DIR;
//
//	ret = hd_gfx_rotate(&param);
//
//	return ret;
//}
//
//#endif


#endif /* USE_FBDEV || USE_BSD_FBDEV */
