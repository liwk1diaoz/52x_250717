#include "SysCommon.h"
#include "AppDisp_PBView.h"
#include "PlaybackTsk.h"
#include "vf_gfx.h"
#include "GxVideo.h"
#include <kwrap/error_no.h>
#include "exif/Exif.h"
#include "exif/ExifDef.h"
#include "SizeConvert.h"
#include "kwrap/perf.h"
#include "PlaybackTsk.h"
#include "GxVideoFile.h"
#include "sys_mempool.h"
#include "UIWnd/UIFlow.h"
#include "lfqueue/lfqueue.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          PBVIEW
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
#define COLOR_RGB_BLACK             0x00000000

#if PLAY_DEWARP == ENABLE

typedef struct {
	PBVIEW_DEWARP_CB cb;
	BOOL is_set;
} PBVIEW_DEWARP_PARAM;

static PBVIEW_DEWARP_PARAM dewarp_param = {0};

#endif

static USIZE  aspect_ratio;
static ER (*gfpDrawCb)(HD_VIDEO_FRAME *pHdDecVdoFrame, PUSIZE dst_ratio);
static UINT32 gPbState = PB_VIEW_STATE_NONE;
static HD_VIDEO_FRAME g_LastVdoFrm = {0}; //keep last disp screen
#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
static HD_VIDEO_FRAME g_DispTmpFrm = {0};  // display tmp frame
static HD_VIDEO_FRAME g_ThumbFrm[FLOWPB_THUMB_H_NUM2*FLOWPB_THUMB_V_NUM2] = {0}; // thumbnail frame
#endif

static const char pbview_videoout_task_name[] = "PBView_Vout_Task";

typedef struct {
	THREAD_HANDLE  id;
	UINT run;
	UINT is_running;
	lfqueue_t lfqueue;
} PBView_videoout_task_param;


typedef struct {

	HD_PATH_ID		path_id;
	HD_VIDEO_FRAME	push_in_frame;

	/* buffers need to be released after frame pushed */
	PBVIEW_HD_COM_BUF relesse_view_buf;
	PBVIEW_HD_COM_BUF release_2pass_buf;

} lfqueue_videoout_param;

PBView_videoout_task_param* get_videoout_task_param(void)
{
	static PBView_videoout_task_param* param = NULL;

	if(param == NULL){

		DBG_DUMP("init lfqueue\r\n");

		param = (PBView_videoout_task_param*) malloc(sizeof(PBView_videoout_task_param));
		memset(param, 0, sizeof(PBView_videoout_task_param));
		lfqueue_init(&param->lfqueue);
	}

	return param;
}

THREAD_RETTYPE _PBView_videoout_task(void)
{
	HD_RESULT ret = HD_OK;
	PBView_videoout_task_param* task_param = NULL;
	lfqueue_videoout_param* lfqueue_param = NULL;
	UINT empty_queue_delay_ms = 1;

	THREAD_ENTRY();

	task_param = get_videoout_task_param();

	while(task_param->run)
	{
		if(!task_param->is_running)
			task_param->is_running = 1;

		/* sleep when queue is empty */
		if((lfqueue_param = lfqueue_deq(&task_param->lfqueue)) == NULL){
			vos_util_delay_ms(empty_queue_delay_ms);
			continue;
		}

		ret = hd_videoout_push_in_buf(
				lfqueue_param->path_id,
				&lfqueue_param->push_in_frame,
				NULL,
				0);

		if (ret != HD_OK && ret != HD_ERR_OVERRUN){
			DBG_ERR("failed to push in buf of videoout\r\n");
		}

		if(lfqueue_param->relesse_view_buf.blk && lfqueue_param->relesse_view_buf.blk!= HD_COMMON_MEM_VB_INVALID_BLK){

			hd_common_mem_munmap((void *)lfqueue_param->relesse_view_buf.va, lfqueue_param->relesse_view_buf.blk_size);

			ret = hd_common_mem_release_block(lfqueue_param->relesse_view_buf.blk);
			if (ret != HD_OK){
				DBG_ERR("release blk(0x%x) failed\r\n", lfqueue_param->relesse_view_buf.blk);
			}
		}


	    if (lfqueue_param->release_2pass_buf.blk && lfqueue_param->release_2pass_buf.blk!= HD_COMMON_MEM_VB_INVALID_BLK) {
		    hd_common_mem_munmap((void *)lfqueue_param->release_2pass_buf.va, lfqueue_param->release_2pass_buf.blk_size);
			ret = hd_common_mem_release_block(lfqueue_param->release_2pass_buf.blk);
			if (ret != HD_OK){
				DBG_ERR("release 2-pass blk(0x%x) failed\r\n", lfqueue_param->release_2pass_buf.blk);
			}
		}


	    if(lfqueue_param){
	    	free(lfqueue_param);
	    	lfqueue_param = NULL;
	    }
	}

	task_param->is_running = 0;

	THREAD_RETURN(0);
}

ER PBView_videoout_task_create(void)
{
	ER ret = E_OK;
	const char* task_name = pbview_videoout_task_name;
	const int priority = 9;
	const int stack_size = 2048;
	PBView_videoout_task_param* task_param = get_videoout_task_param();

	if(!task_param->id){

		if ((task_param->id = vos_task_create(_PBView_videoout_task, 0, task_name, priority, stack_size)) == 0) {
			DBG_ERR("%s create failed.\r\n", pbview_videoout_task_name);
			ret |= E_SYS;
		} else {
			DBG_DUMP("%s created\r\n", task_name);
			task_param->run = 1;
			vos_task_resume(task_param->id);
		}
	}

	return ret;
}

void PBView_videoout_task_destroy(void)
{
	const UINT timeout = 500;
	const UINT delay_us = 10*1000;
	UINT cnt = 0;
	PBView_videoout_task_param* task_param = get_videoout_task_param();

	task_param->run = 0;

	while(task_param->is_running && (cnt++ < timeout))
	{
		vos_util_delay_us(delay_us);
	}

	if(cnt >= timeout){
		DBG_ERR("wait PBView_videoout_task finish timeout(over %s ms)\r\n", (timeout*delay_us)/1000);
	}

	vos_task_destroy(task_param->id);

	task_param->id = 0;

	DBG_DUMP("%s destroyed\r\n", pbview_videoout_task_name);
}


UINT32 PBView_get_hd_phy_addr(void *va)
{
    if (va) {
        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
        vir_meminfo.va = va;
        if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) == HD_OK) {
                return vir_meminfo.pa;
        }
    }

    return 0;
}

HD_RESULT PBView_get_hd_common_buf(PPBVIEW_HD_COM_BUF p_hd_view_buf)
{
	// get memory
	p_hd_view_buf->blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, p_hd_view_buf->blk_size, DDR_ID0); // Get block from mem pool
	if (p_hd_view_buf->blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("config_vdo_frm: get blk fail, blk(0x%x)\n", p_hd_view_buf->blk );
		return HD_ERR_SYS;
	}

	p_hd_view_buf->pa = hd_common_mem_blk2pa(p_hd_view_buf->blk); // get physical addr
	if (p_hd_view_buf->pa == 0) {
		DBG_ERR("config_vdo_frm: blk2pa fail, blk(0x%x)\n", p_hd_view_buf->blk);
		return HD_ERR_SYS;
	}

	p_hd_view_buf->va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_hd_view_buf->pa, p_hd_view_buf->blk_size);
	if (p_hd_view_buf->va == 0) {
		DBG_ERR("Convert to VA failed for file buffer for decoded buffer!\r\n");
		return HD_ERR_SYS;
	}

	return HD_OK;
}

static void PBView_2PassScale(HD_VIDEO_FRAME *pVdoSrc, HD_VIDEO_FRAME *pVdo2PassSrc)
{
	VF_GFX_SCALE  gfx_scale = {0};

	memcpy((void *)&gfx_scale.src_img, (void *)pVdoSrc, sizeof(HD_VIDEO_FRAME));
	memcpy((void *)&gfx_scale.dst_img, (void *)pVdo2PassSrc, sizeof(HD_VIDEO_FRAME));

	gfx_scale.src_region.x = 0;
	gfx_scale.src_region.y = 0;
	gfx_scale.src_region.w = pVdoSrc->dim.w;
	gfx_scale.src_region.h = pVdoSrc->dim.h;
	gfx_scale.dst_region.x = 0;
	gfx_scale.dst_region.y = 0;
	gfx_scale.dst_region.w = pVdo2PassSrc->dim.w;
	gfx_scale.dst_region.h = pVdo2PassSrc->dim.h;
	gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	vf_gfx_scale(&gfx_scale, 0);
}

#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
void PBView_DrawSingleView(HD_VIDEO_FRAME *pVdoSrc)
{
    HD_RESULT ret = HD_OK;
	PBVIEW_HD_COM_BUF hd_view_buf = {0};
	PBVIEW_HD_COM_BUF hd_2pass_buf = {0};
	VF_GFX_SCALE  gfx_scale = {0};
	HD_VIDEOOUT_IN video_out_param = {0};
	HD_VIDEOOUT_WIN_ATTR video_out_win = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);
	HD_VIDEO_FRAME Vdo2PassSrc = {0}, *pVdoNewSrc = NULL;
	UINT32 disp_rotate = GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY);
	ISIZE  disp_size = GxVideo_GetDeviceSize(DOUT1);
	USIZE  dst_aspect_ratio = GxVideo_GetDeviceAspect(DOUT1);
	ISIZE  img_size;
	BOOL   scale_2pass = FALSE;

	disp_size.w /= 2; // single view only on the right half part of display
	dst_aspect_ratio.w /= 2; // single view only on the right half part of display

	// set scaling image info by original image and display aspect ratio
	if ((pVdoSrc->dim.w * dst_aspect_ratio.h) > (dst_aspect_ratio.w * pVdoSrc->dim.h)) {

		if ((pVdoSrc->dim.h / 2) > (UINT32)disp_size.h) {
			scale_2pass = TRUE;
			img_size.w = pVdoSrc->dim.w / 2;
			img_size.h = pVdoSrc->dim.h / 2;
		} else {
			img_size.w = pVdoSrc->dim.w;
			img_size.h = pVdoSrc->dim.h;
		}

		gfx_scale.engine = 0;
		gfx_scale.src_region.w = ALIGN_CEIL_4((img_size.h * dst_aspect_ratio.w) / dst_aspect_ratio.h);
		gfx_scale.src_region.h = img_size.h;
		gfx_scale.src_region.x = (img_size.w - gfx_scale.src_region.w) / 2;
		gfx_scale.src_region.y = 0;
		gfx_scale.dst_region.w = disp_size.w;
		gfx_scale.dst_region.h = disp_size.h;
		gfx_scale.dst_region.x = 0;
		gfx_scale.dst_region.y = 0;
		gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	} else {

		if ((pVdoSrc->dim.w / 2) > (UINT32)disp_size.w) {
			scale_2pass = TRUE;
			img_size.w = pVdoSrc->dim.w / 2;
			img_size.h = pVdoSrc->dim.h / 2;
		} else {
			img_size.w = pVdoSrc->dim.w;
			img_size.h = pVdoSrc->dim.h;
		}

		gfx_scale.engine = 0;
		gfx_scale.src_region.w = img_size.w;
		gfx_scale.src_region.h = ALIGN_CEIL_4((img_size.w * dst_aspect_ratio.h) / dst_aspect_ratio.w);
		gfx_scale.src_region.x = 0;
		gfx_scale.src_region.y = (img_size.h - gfx_scale.src_region.h) / 2;
		gfx_scale.dst_region.w = disp_size.w;
		gfx_scale.dst_region.h = disp_size.h;
		gfx_scale.dst_region.x = disp_size.w; // single view is on the right side of display
		gfx_scale.dst_region.y = 0;
		gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	}

	// do 2-pass scaling if necessary
	if (scale_2pass) {
		hd_2pass_buf.blk_size = img_size.w * img_size.h * 3 / 2;
		PBView_get_hd_common_buf(&hd_2pass_buf);

		pVdoNewSrc = &Vdo2PassSrc;
		loff[0] = img_size.w;
		addr[0] = hd_2pass_buf.pa;
		loff[1] = img_size.w;
		addr[1] = ALIGN_CEIL_4(addr[0] + img_size.w * img_size.h);
		ret = vf_init_ex(pVdoNewSrc, img_size.w, img_size.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);
		if (ret != HD_OK) {
			DBG_ERR("vf_init_ex: 2-pass src failed\r\n");
			return;
		}
		pVdoNewSrc->blk = hd_2pass_buf.blk;
		PBView_2PassScale(pVdoSrc, pVdoNewSrc);
	} else {
		pVdoNewSrc = pVdoSrc;
	}

	// scale image to display size
	disp_size.w *= 2; // recall original display width
	hd_view_buf.blk_size = disp_size.w * disp_size.h * 3 / 2;
	PBView_get_hd_common_buf(&hd_view_buf);
	loff[0] = disp_size.w;
	addr[0] = hd_view_buf.pa;
	loff[1] = disp_size.w;
	addr[1] = ALIGN_CEIL_4(addr[0] + disp_size.w * disp_size.h);
	ret = vf_init_ex(&gfx_scale.dst_img, disp_size.w, disp_size.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);
	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
		return;
	}

	#if 0 // clear buffer
	VF_GFX_DRAW_RECT  draw_rect = {0};
	memcpy((void *)&draw_rect.dst_img, (void *)&gfx_scale.dst_img, sizeof(HD_VIDEO_FRAME));
	draw_rect.color = COLOR_RGB_BLACK;
	draw_rect.rect.x = 0;
	draw_rect.rect.y = 0;
	draw_rect.rect.w = disp_size.w;
	draw_rect.rect.h = disp_size.h;
	draw_rect.type = HD_GFX_RECT_SOLID;
	draw_rect.thickness = 0; //Don't care for HD_GFX_RECT_SOLID
	draw_rect.engine = 0;
	vf_gfx_draw_rect(&draw_rect);
	#endif

	memcpy((void *)&gfx_scale.src_img, (void *)pVdoNewSrc, sizeof(HD_VIDEO_FRAME));
	vf_gfx_scale(&gfx_scale, 1);
	gfx_scale.dst_img.blk = hd_view_buf.blk; //must for HD_VIDEOOUT

	#if 1
	// copy left part of display tmp buffer (thumbnail layout) to display buffer
	VF_GFX_COPY gfx_copy;
	gfx_copy.src_img = g_DispTmpFrm;
	gfx_copy.dst_img = gfx_scale.dst_img;
	//if ((ret = vf_init(&gfx_copy.dst_img, gfx_scale.dst_img.dim.w, gfx_scale.dst_img.dim.h, gfx_scale.dst_img.pxlfmt, gfx_scale.dst_img.loff[0], hd_view_buf.pa, hd_view_buf.blk_size)) != HD_OK) {
	//	DBG_ERR("vf_init fail(%d)\r\n", ret);
	//}
	gfx_copy.src_region.x = 0;
	gfx_copy.src_region.y = 0;
	gfx_copy.src_region.w = gfx_scale.dst_img.dim.w/2;
	gfx_copy.src_region.h = gfx_scale.dst_img.dim.h;
	gfx_copy.dst_pos.x = 0;
	gfx_copy.dst_pos.y = 0;
	gfx_copy.colorkey = 0;
	gfx_copy.alpha = 255;
	gfx_copy.engine = 0;
	if ((ret = vf_gfx_copy(&gfx_copy)) != HD_OK) {
		DBG_ERR("vf_gfx_copy fail(%d)\r\n", ret);
	}
	#endif

	// set video out parameter
	video_out_param.dim.w = disp_size.w;
	video_out_param.dim.h = disp_size.h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = SysVideo_GetDirbyID(0);
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN failed\r\n");
		return;
	}

	// set video out window
	video_out_win.visible = TRUE;
	video_out_win.layer = HD_LAYER1;
	video_out_win.rect.x = 0;
	video_out_win.rect.y = 0;
	if (disp_rotate) {
		video_out_win.rect.w = disp_size.h;
		video_out_win.rect.h = disp_size.w;
	} else {
		video_out_win.rect.w = disp_size.w;
		video_out_win.rect.h = disp_size.h;
	}
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_win);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN_WIN_ATTR fail\r\n");
	}

	ret = hd_videoout_start(video_out_path);
	if(ret != HD_OK){
		DBG_ERR("hd_videoout_start failed\r\n");
		return;
	}

	// push in and release buffer
	ret = hd_videoout_push_in_buf(video_out_path, &gfx_scale.dst_img, NULL, 0); // only support non-blocking mode now
	if (ret != HD_OK) {
		DBG_ERR("failed to push in buf of videoout\r\n");
	} else {
	    hd_common_mem_munmap((void *)hd_view_buf.va, hd_view_buf.blk_size);
	    ret = hd_common_mem_release_block(hd_view_buf.blk);
		if (ret != HD_OK) {
			DBG_ERR("release blk(0x%x) failed\r\n", hd_view_buf.blk);
		}

		memcpy((void *)&g_LastVdoFrm, (void *)&gfx_scale.dst_img, sizeof(HD_VIDEO_FRAME));
	}

	// release 2-pass scaling buffer if necessary
    if (hd_2pass_buf.blk) {
	    hd_common_mem_munmap((void *)hd_2pass_buf.va, hd_2pass_buf.blk_size);
		ret = hd_common_mem_release_block(hd_2pass_buf.blk);
		if (ret != HD_OK) {
			DBG_ERR("release 2-pass blk(0x%x) failed\r\n", hd_2pass_buf.blk);
		}
	}
}
#elif PLAY_FULL_DISP // image crop for full display
void PBView_DrawSingleView(HD_VIDEO_FRAME *pVdoSrc)
{
    HD_RESULT ret = HD_OK;
	PBVIEW_HD_COM_BUF hd_view_buf = {0};
	PBVIEW_HD_COM_BUF hd_2pass_buf = {0};
	VF_GFX_SCALE  gfx_scale = {0};
	HD_VIDEOOUT_IN video_out_param = {0};
	HD_VIDEOOUT_WIN_ATTR video_out_win = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);
	HD_VIDEO_FRAME Vdo2PassSrc = {0}, *pVdoNewSrc = NULL;
	UINT32 disp_rotate = GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY);
	ISIZE  disp_size = GxVideo_GetDeviceSize(DOUT1);
	USIZE  dst_aspect_ratio = GxVideo_GetDeviceAspect(DOUT1);
	ISIZE  img_size;
	BOOL   scale_2pass = FALSE;

	// set scaling image info by original image and display aspect ratio
	if ((pVdoSrc->dim.w * dst_aspect_ratio.h) > (dst_aspect_ratio.w * pVdoSrc->dim.h)) {

		if ((pVdoSrc->dim.h / 2) > (UINT32)disp_size.h) {
			scale_2pass = TRUE;
			img_size.w = pVdoSrc->dim.w / 2;
			img_size.h = pVdoSrc->dim.h / 2;
		} else {
			img_size.w = pVdoSrc->dim.w;
			img_size.h = pVdoSrc->dim.h;
		}

		gfx_scale.engine = 0;
		gfx_scale.src_region.w = ALIGN_CEIL_4((img_size.h * dst_aspect_ratio.w) / dst_aspect_ratio.h);
		gfx_scale.src_region.h = img_size.h;
		gfx_scale.src_region.x = (img_size.w - gfx_scale.src_region.w) / 2;
		gfx_scale.src_region.y = 0;
		gfx_scale.dst_region.w = disp_size.w;
		gfx_scale.dst_region.h = disp_size.h;
		gfx_scale.dst_region.x = 0;
		gfx_scale.dst_region.y = 0;
		gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	} else {

		if ((pVdoSrc->dim.w / 2) > (UINT32)disp_size.w) {
			scale_2pass = TRUE;
			img_size.w = pVdoSrc->dim.w / 2;
			img_size.h = pVdoSrc->dim.h / 2;
		} else {
			img_size.w = pVdoSrc->dim.w;
			img_size.h = pVdoSrc->dim.h;
		}

		gfx_scale.engine = 0;
		gfx_scale.src_region.w = img_size.w;
		gfx_scale.src_region.h = ALIGN_CEIL_4((img_size.w * dst_aspect_ratio.h) / dst_aspect_ratio.w);
		gfx_scale.src_region.x = 0;
		gfx_scale.src_region.y = (img_size.h - gfx_scale.src_region.h) / 2;
		gfx_scale.dst_region.w = disp_size.w;
		gfx_scale.dst_region.h = disp_size.h;
		gfx_scale.dst_region.x = 0;
		gfx_scale.dst_region.y = 0;
		gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	}

	// do 2-pass scaling if necessary
	if (scale_2pass) {
		hd_2pass_buf.blk_size = img_size.w * img_size.h * 3 / 2;
		PBView_get_hd_common_buf(&hd_2pass_buf);

		pVdoNewSrc = &Vdo2PassSrc;
		loff[0] = img_size.w;
		addr[0] = hd_2pass_buf.pa;
		loff[1] = img_size.w;
		addr[1] = ALIGN_CEIL_4(addr[0] + img_size.w * img_size.h);
		ret = vf_init_ex(pVdoNewSrc, img_size.w, img_size.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);
		if (ret != HD_OK) {
			DBG_ERR("vf_init_ex: 2-pass src failed\r\n");
			return;
		}
		pVdoNewSrc->blk = hd_2pass_buf.blk;
		PBView_2PassScale(pVdoSrc, pVdoNewSrc);
	} else {
		pVdoNewSrc = pVdoSrc;
	}

	// scale image to display size
	hd_view_buf.blk_size = disp_size.w * disp_size.h * 3 / 2;
	PBView_get_hd_common_buf(&hd_view_buf);
	loff[0] = disp_size.w;
	addr[0] = hd_view_buf.pa;
	loff[1] = disp_size.w;
	addr[1] = ALIGN_CEIL_4(addr[0] + disp_size.w * disp_size.h);
	ret = vf_init_ex(&gfx_scale.dst_img, disp_size.w, disp_size.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);
	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
		return;
	}

	memcpy((void *)&gfx_scale.src_img, (void *)pVdoNewSrc, sizeof(HD_VIDEO_FRAME));

	vf_gfx_scale(&gfx_scale, 1);
	gfx_scale.dst_img.blk = hd_view_buf.blk; //must for HD_VIDEOOUT

	// set video out parameter
	video_out_param.dim.w = disp_size.w;
	video_out_param.dim.h = disp_size.h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = SysVideo_GetDirbyID(0);
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN failed\r\n");
		return;
	}

	// set video out window
	video_out_win.visible = TRUE;
	video_out_win.layer = HD_LAYER1;
	video_out_win.rect.x = 0;
	video_out_win.rect.y = 0;
	if (disp_rotate) {
		video_out_win.rect.w = disp_size.h;
		video_out_win.rect.h = disp_size.w;
	} else {
		video_out_win.rect.w = disp_size.w;
		video_out_win.rect.h = disp_size.h;
	}
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_win);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN_WIN_ATTR fail\r\n");
	}

	ret = hd_videoout_start(video_out_path);
	if(ret != HD_OK){
		DBG_ERR("hd_videoout_start failed\r\n");
		return;
	}

	// push in and release buffer
	ret = hd_videoout_push_in_buf(video_out_path, &gfx_scale.dst_img, NULL, 0); // only support non-blocking mode now
	if (ret != HD_OK) {
		DBG_ERR("failed to push in buf of videoout\r\n");
	} else {
	    hd_common_mem_munmap((void *)hd_view_buf.va, hd_view_buf.blk_size);
	    ret = hd_common_mem_release_block(hd_view_buf.blk);
		if (ret != HD_OK) {
			DBG_ERR("release blk(0x%x) failed\r\n", hd_view_buf.blk);
		}

		memcpy((void *)&g_LastVdoFrm, (void *)&gfx_scale.dst_img, sizeof(HD_VIDEO_FRAME));
	}

	// release 2-pass scaling buffer if necessary
    if (hd_2pass_buf.blk) {
	    hd_common_mem_munmap((void *)hd_2pass_buf.va, hd_2pass_buf.blk_size);
		ret = hd_common_mem_release_block(hd_2pass_buf.blk);
		if (ret != HD_OK) {
			DBG_ERR("release 2-pass blk(0x%x) failed\r\n", hd_2pass_buf.blk);
		}
	}
}
#else // image full view

#if PLAY_DEWARP == ENABLE

void PBView_DrawSingleView_Dewarp(HD_VIDEO_FRAME *pVdoSrc)
{
	if(dewarp_param.is_set == TRUE){

		HD_VIDEO_FRAME dewarp_frame = {0};

		if(dewarp_param.cb.push_in_buf(pVdoSrc, NULL) != HD_OK)
			return;

		if(dewarp_param.cb.pull_out_buf(&dewarp_frame) != HD_OK)
			return;

		PBView_DrawSingleView(&dewarp_frame);

		if(dewarp_param.cb.release_out_buf(&dewarp_frame) != HD_OK)
			return;
	}
	else{
		PBView_DrawSingleView(pVdoSrc);
	}
}

void PBView_SetDewarpCB(const PBVIEW_DEWARP_CB cb)
{
	if(cb.pull_out_buf && cb.push_in_buf && cb.release_out_buf){
		dewarp_param.cb = cb;
		dewarp_param.is_set = TRUE;
	}
	else{
		dewarp_param.is_set = FALSE;
	}
}

#endif

void PBView_DrawSingleView(HD_VIDEO_FRAME *pVdoSrc)
{
    HD_RESULT ret = HD_OK;
	PBVIEW_HD_COM_BUF hd_view_buf = {0};
	PBVIEW_HD_COM_BUF hd_2pass_buf = {0};
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps = &video_out_syscaps;
	VF_GFX_SCALE  gfx_scale = {0};
	HD_VIDEOOUT_IN video_out_param={0};
	HD_VIDEOOUT_WIN_ATTR video_out_win = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);
	HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
	HD_VIDEO_FRAME Vdo2PassSrc = {0}, *pVdoNewSrc = NULL;
	float SrcImgRatio, Img2PassRatio;
	UINT32 New2PassImgW, New2PassImgH= 0;
	SIZECONVERT_INFO     CovtInfo = {0};
	HD_URECT DstDispRect = {0};
	UINT32 disp_rotate = GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY);
	USIZE  dst_aspect_ratio = GxVideo_GetDeviceAspect(DOUT1);

    Img2PassRatio = (float)PBVIEW_2PASS_W/(float)PBVIEW_2PASS_H;
	Img2PassRatio = (INT32)(Img2PassRatio*100);
    SrcImgRatio = (float)pVdoSrc->dim.w/(float)pVdoSrc->dim.h;
    SrcImgRatio = (INT32)(SrcImgRatio*100);

	if (SrcImgRatio > Img2PassRatio){ //Source image is 16:9
		New2PassImgW = PBVIEW_2PASS_W;
    	New2PassImgH = ALIGN_CEIL_4(PBVIEW_2PASS_W*9/16);
	}else{
		New2PassImgW = PBVIEW_2PASS_W;
	    New2PassImgH = PBVIEW_2PASS_H;
    }

    if ((pVdoSrc->dim.w > New2PassImgW) && (pVdoSrc->dim.h > New2PassImgH)){
  	    hd_2pass_buf.blk_size = New2PassImgW*New2PassImgH*3/2;

  	    if(PBView_get_hd_common_buf(&hd_2pass_buf) != HD_OK)
  	    {
  	    	goto RELEASE_BLK;
  	    }


  	    pVdoNewSrc = &Vdo2PassSrc;
  	    loff[0] = New2PassImgW; //Besides rotate panel, the display device don't consider the line offset.
  	    addr[0] = hd_2pass_buf.pa;
  	    loff[1] = New2PassImgW; //Besides rotate panel, the display device don't consider the line offset.
  	    addr[1] = ALIGN_CEIL_4(addr[0] + New2PassImgW*New2PassImgH);
  	    ret = vf_init_ex(pVdoNewSrc, New2PassImgW, New2PassImgH, HD_VIDEO_PXLFMT_YUV420, loff, addr);
  	    if (ret != HD_OK) {
  		    DBG_ERR("vf_init_ex: 2-pass src failed\r\n");
			return;
  	    }
  	    pVdoNewSrc->blk = hd_2pass_buf.blk;
  	    PBView_2PassScale(pVdoSrc, pVdoNewSrc);
    }else{
  	    pVdoNewSrc = pVdoSrc;
    }

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
		return;
	}

	CovtInfo.uiSrcWidth  = pVdoNewSrc->dim.w;
	CovtInfo.uiSrcHeight = pVdoNewSrc->dim.h;
	if (disp_rotate) {
		CovtInfo.uiDstWidth  = p_video_out_syscaps->output_dim.h;
		CovtInfo.uiDstHeight = p_video_out_syscaps->output_dim.w;
	} else {
		CovtInfo.uiDstWidth  = p_video_out_syscaps->output_dim.w;
		CovtInfo.uiDstHeight = p_video_out_syscaps->output_dim.h;
	}
	CovtInfo.uiDstWRatio = dst_aspect_ratio.w;
	CovtInfo.uiDstHRatio = dst_aspect_ratio.h;
	CovtInfo.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
	DisplaySizeConvert(&CovtInfo);

	if (CovtInfo.uiOutWidth && CovtInfo.uiOutHeight) {
        if (disp_rotate) {
            CovtInfo.uiOutWidth = ALIGN_CEIL_8(CovtInfo.uiOutWidth);
            CovtInfo.uiOutHeight = ALIGN_CEIL_8(CovtInfo.uiOutHeight);
            CovtInfo.uiOutX = (p_video_out_syscaps->output_dim.h -  CovtInfo.uiOutWidth)>>1;
            CovtInfo.uiOutY = (p_video_out_syscaps->output_dim.w -  CovtInfo.uiOutHeight)>>1;
        }

		DstDispRect.x = CovtInfo.uiOutX;
		DstDispRect.y = CovtInfo.uiOutY;
		DstDispRect.w = CovtInfo.uiOutWidth;
		DstDispRect.h = CovtInfo.uiOutHeight;
	} else {
		DstDispRect.x = 0;
		DstDispRect.y = 0;
		DstDispRect.w = p_video_out_syscaps->output_dim.w;
		DstDispRect.h = p_video_out_syscaps->output_dim.h;
	}

	hd_view_buf.blk_size = DstDispRect.w*DstDispRect.h*3/2;


	if(PBView_get_hd_common_buf(&hd_view_buf) != HD_OK)
	{
		goto RELEASE_BLK;
	}

	loff[0] = DstDispRect.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[0] = hd_view_buf.pa;
	loff[1] = DstDispRect.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[1] = ALIGN_CEIL_4(addr[0] + DstDispRect.w*DstDispRect.h);
	ret = vf_init_ex(&gfx_scale.dst_img, DstDispRect.w, DstDispRect.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);
	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
		return;
	}

	memcpy((void *)&gfx_scale.src_img, (void *)pVdoNewSrc, sizeof(HD_VIDEO_FRAME));

	gfx_scale.engine = 0;
	gfx_scale.src_region.x = 0;
	gfx_scale.src_region.y = 0;
	gfx_scale.src_region.w = pVdoSrc->dim.w;
	gfx_scale.src_region.h = pVdoSrc->dim.h;
	gfx_scale.dst_region.x = 0; // dst frame buffer rather than IDE window.
	gfx_scale.dst_region.y = 0; // dst frame buffer rather than IDE window.
	gfx_scale.dst_region.w = DstDispRect.w;
	gfx_scale.dst_region.h = DstDispRect.h;
	gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;

	vf_gfx_scale(&gfx_scale, 0);
	gfx_scale.dst_img.blk = hd_view_buf.blk; //must for HD_VIDEOOUT

	video_out_param.dim.w = DstDispRect.w;
	video_out_param.dim.h = DstDispRect.h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = SysVideo_GetDirbyID(0);
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN failed\r\n");
		return;
	}

	#if 0
	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_get: HD_VIDEOOUT_PARAM_IN failed\r\n");
		return;
	}
	#endif

	video_out_win.visible = TRUE;
	if (disp_rotate) {
		video_out_win.rect.x = DstDispRect.y;
		video_out_win.rect.y = DstDispRect.x;
		video_out_win.rect.w = DstDispRect.h;
		video_out_win.rect.h = DstDispRect.w;
	} else {
		video_out_win.rect.x = DstDispRect.x;
		video_out_win.rect.y = DstDispRect.y;
		video_out_win.rect.w = DstDispRect.w;
		video_out_win.rect.h = DstDispRect.h;
	}
	video_out_win.layer = HD_LAYER1;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_win);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN_WIN_ATTR fail\r\n");
	}

	ret = hd_videoout_start(video_out_path);
	if(ret != HD_OK){
		DBG_ERR("hd_videoout_start failed\r\n");
		return;
	}

#if 0
	DBG_ERR("MAKEFOURCC('V','F','R','M') = %d\r\n", MAKEFOURCC('V','F','R','M'));
	DBG_ERR("gfx_scale.dst_img.sign = %d\r\n", gfx_scale.dst_img.sign);
	DBG_ERR("gfx_scale.dst_img.ddr_id = %d\r\n", gfx_scale.dst_img.ddr_id);
	DBG_ERR("gfx_scale.dst_img.pxlfmt = 0x%x\r\n", gfx_scale.dst_img.pxlfmt);
	DBG_ERR("gfx_scale.dst_img.dim.w = %d\r\n", gfx_scale.dst_img.dim.w);
	DBG_ERR("gfx_scale.dst_img.dim.h = %d\r\n", gfx_scale.dst_img.dim.h);
	DBG_ERR("gfx_scale.dst_img.count = %d\r\n", gfx_scale.dst_img.count);
	DBG_ERR("gfx_scale.dst_img.timestamp = %d\r\n", gfx_scale.dst_img.timestamp);
	DBG_ERR("gfx_scale.dst_img.pw[HD_VIDEO_PINDEX_Y]	= %d\r\n", gfx_scale.dst_img.pw[HD_VIDEO_PINDEX_Y]);
	DBG_ERR("gfx_scale.dst_img.pw[HD_VIDEO_PINDEX_UV]	= %d\r\n", gfx_scale.dst_img.pw[HD_VIDEO_PINDEX_UV]);
	DBG_ERR("gfx_scale.dst_img.ph[HD_VIDEO_PINDEX_Y]	= %d\r\n", gfx_scale.dst_img.ph[HD_VIDEO_PINDEX_Y]);
	DBG_ERR("gfx_scale.dst_img.ph[HD_VIDEO_PINDEX_UV]	= %d\r\n", gfx_scale.dst_img.ph[HD_VIDEO_PINDEX_UV]);
	DBG_ERR("gfx_scale.dst_img.loff[HD_VIDEO_PINDEX_Y]	= %d\r\n", gfx_scale.dst_img.loff[HD_VIDEO_PINDEX_Y]);
	DBG_ERR("gfx_scale.dst_img.loff[HD_VIDEO_PINDEX_UV] = %d\r\n", gfx_scale.dst_img.loff[HD_VIDEO_PINDEX_UV]);
	DBG_ERR("gfx_scale.dst_img.phy_addr[HD_VIDEO_PINDEX_Y]	= 0x%x\r\n", gfx_scale.dst_img.phy_addr[HD_VIDEO_PINDEX_Y]);
	DBG_ERR("gfx_scale.dst_img.phy_addr[HD_VIDEO_PINDEX_UV] = 0x%x\r\n", gfx_scale.dst_img.phy_addr[HD_VIDEO_PINDEX_UV]);
	DBG_ERR("gfx_scale.dst_img.blk = %d\r\n", gfx_scale.dst_img.blk);
#endif


	PBView_videoout_task_param* task_param = get_videoout_task_param();

	if(task_param->is_running){

		lfqueue_videoout_param* lfqueue_param = (lfqueue_videoout_param*) malloc(sizeof(lfqueue_videoout_param));

		memset(lfqueue_param, 0, sizeof(lfqueue_videoout_param));

		lfqueue_param->path_id = video_out_path;
		lfqueue_param->push_in_frame = gfx_scale.dst_img;
		lfqueue_param->relesse_view_buf = hd_view_buf;
		lfqueue_param->release_2pass_buf = hd_2pass_buf;

		while (lfqueue_enq(&task_param->lfqueue, lfqueue_param) == -1) {
		    DBG_ERR("ENQ Full ?\r\n");
		}
	}
	else{
		// push in and release buffer
		ret = hd_videoout_push_in_buf(video_out_path, &gfx_scale.dst_img, NULL, 0); // only support non-blocking mode now
		if (ret != HD_OK) {
			DBG_ERR("failed to push in buf of videoout\r\n");
		}
		else{
			memcpy((void *)&g_LastVdoFrm, (void *)&gfx_scale.dst_img, sizeof(HD_VIDEO_FRAME));
		}

RELEASE_BLK:
		if(hd_view_buf.blk && hd_view_buf.blk != HD_COMMON_MEM_VB_INVALID_BLK){
			hd_common_mem_munmap((void *)hd_view_buf.va, hd_view_buf.blk_size);
			ret = hd_common_mem_release_block(hd_view_buf.blk);
			if (ret != HD_OK) {
				DBG_ERR("release blk(0x%x) failed\r\n", hd_view_buf.blk);
			}
		}

		// release 2-pass scaling buffer if necessary
		if (hd_2pass_buf.blk && hd_2pass_buf.blk != HD_COMMON_MEM_VB_INVALID_BLK) {
		    hd_common_mem_munmap((void *)hd_2pass_buf.va, hd_2pass_buf.blk_size);
			ret = hd_common_mem_release_block(hd_2pass_buf.blk);
			if (ret != HD_OK) {
				DBG_ERR("release 2-pass blk(0x%x) failed\r\n", hd_2pass_buf.blk);
			}
		}	

	}
}
#endif

static ER PBView_DrawMethod(HD_VIDEO_FRAME *pHdDecVdoFrame, PUSIZE dst_aspect_ratio)
{
	//--------------------Customer Draw Begin-------------------------------

#if PLAY_DEWARP == ENABLE
	PBView_DrawSingleView_Dewarp(pHdDecVdoFrame);
#else
	PBView_DrawSingleView(pHdDecVdoFrame);
#endif
	//--------------------Customer Draw End---------------------------------

	PB_SetPBFlag(PB_SET_FLG_DRAW_END);

	return E_OK;
}

static ER PBView_OnSingleDraw(HD_VIDEO_FRAME *pHdDecVdoFrame, PUSIZE dst_aspect_ratio)
{
	return PBView_DrawMethod(pHdDecVdoFrame, dst_aspect_ratio);
}

static void PBView_DrawThumbView(UINT32 i, HD_VIDEO_FRAME *pVdoSrc, HD_VIDEO_FRAME *pVdoDstDisp, PUSIZE dst_ratio)
{
	UINT32 SrcWidth, SrcHeight; //SrcStart_X, SrcStart_Y
	UINT32 DstWidth, DstHeight, DstX, DstY;
	UINT8  ucOrientation;
	HD_IRECT DstRegion = {0};
	PURECT idx_view;
	UINT32 width_array, height_array;
	BOOL  dec_err_array;
	BOOL   dec_err=TRUE, exif_exist;
	UINT32 exif_orient = 0;
	VF_GFX_DRAW_RECT  DrawRectDst = {0};
	VF_GFX_DRAW_RECT *pDrawRectDst = &DrawRectDst;
	VF_GFX_SCALE  gfx_scale = {0};
	HD_VIDEO_FRAME Vdo2PassSrc = {0}, *pVdoNewSrc = NULL;
	//INT32 i32VdoDstBlk = pVdoDst->blk;
	PBVIEW_HD_COM_BUF hd_2pass_buf = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_RESULT ret = HD_OK;

	PB_GetParam(PBPRMID_THUMB_RECT, (UINT32 *) &idx_view);
	PB_GetParam(PBPRMID_THUMB_WIDTH_ARRAY, (UINT32 *) &width_array);
	PB_GetParam(PBPRMID_THUMB_HEIGHT_ARRAY, (UINT32 *) &height_array);
	PB_GetParam(PBPRMID_THUMB_DEC_ARRAY, (UINT32 *) &dec_err_array);
	PB_GetParam(PBPRMID_EXIF_EXIST, (UINT32 *) &exif_exist);
	PB_GetParam(PBPRMID_EXIF_ORIENT, (UINT32 *) &exif_orient);
	dec_err = *((((BOOL *)dec_err_array) + i));

	SrcWidth  = *((((UINT32 *)width_array) + i));
	SrcHeight = *((((UINT32 *)height_array) + i));
	//SrcStart_X = 0;
	//SrcStart_Y = 0;
	DstWidth  = (idx_view + i)->w;
	DstHeight = (idx_view + i)->h;
	DstX = (idx_view + i)->x;
	DstY = (idx_view + i)->y;

	// clear BG
	if (i == 0) {
		memcpy((void *)&pDrawRectDst->dst_img, (void *)pVdoDstDisp, sizeof(HD_VIDEO_FRAME));
		pDrawRectDst->color = COLOR_RGB_BLACK;
		pDrawRectDst->rect.x = 0;
		pDrawRectDst->rect.y = 0;
		pDrawRectDst->rect.w = pVdoDstDisp->dim.w;
		pDrawRectDst->rect.h = pVdoDstDisp->dim.h;
		pDrawRectDst->type = HD_GFX_RECT_SOLID;
		pDrawRectDst->thickness = 0; //Don't care for HD_GFX_RECT_SOLID
		pDrawRectDst->engine = 0;
		vf_gfx_draw_rect(pDrawRectDst);
	}
	DstRegion.x = DstX;
	DstRegion.y = DstY;
	DstRegion.w = DstWidth;
	DstRegion.h = DstHeight;
	// decode error
	if (dec_err == TRUE) {
		memcpy((void *)&pDrawRectDst->rect, (void *)&DstRegion, sizeof(HD_IRECT));
		pDrawRectDst->rect.x = DstX;
		pDrawRectDst->rect.y = DstY;
		pDrawRectDst->rect.w = DstWidth;
		pDrawRectDst->rect.h = DstHeight;
		pDrawRectDst->color = COLOR_RGB_BLACK;
		pDrawRectDst->type = HD_GFX_RECT_SOLID;
		pDrawRectDst->thickness = 0; //Don't care for HD_GFX_RECT_SOLID
		pDrawRectDst->engine = 0;
		vf_gfx_draw_rect(pDrawRectDst);
	}
	// decode ok
	else {
		UINT16 usPBDisplayWidth = pVdoDstDisp->dim.w;
		PB_KEEPAR ImageRatioTrans;

		if ((SrcWidth == DstWidth) && (SrcHeight == DstHeight)) {
			ImageRatioTrans = PB_KEEPAR_NONE;
		} else {
			ImageRatioTrans = PB_GetImageRatioTrans(SrcWidth, SrcHeight, DstWidth*dst_ratio->w*pVdoDstDisp->dim.h/pVdoDstDisp->dim.w/dst_ratio->h, DstHeight);
		}

		if (ImageRatioTrans == PB_KEEPAR_LETTERBOXING) {
			UINT32 uiTmpHeight;
			UINT32 temp;

			// Clear buffer..
			memcpy((void *)&pDrawRectDst->dst_img, (void *)pVdoDstDisp, sizeof(HD_VIDEO_FRAME));
			pDrawRectDst->rect.x = DstX;
			pDrawRectDst->rect.y = DstY;
			pDrawRectDst->rect.w = DstWidth;
			pDrawRectDst->rect.h = DstHeight;
			pDrawRectDst->color = COLOR_RGB_BLACK;
			pDrawRectDst->type = HD_GFX_RECT_SOLID;
			pDrawRectDst->thickness = 0; //Don't care for HD_GFX_RECT_SOLID
			pDrawRectDst->engine = 0;
			vf_gfx_draw_rect(pDrawRectDst);

			uiTmpHeight = DstWidth * SrcHeight / SrcWidth * pVdoDstDisp->dim.h / usPBDisplayWidth * dst_ratio->w / dst_ratio->h;
			temp = ((DstHeight - uiTmpHeight) + 1) & 0xFFFFFFFE;
			DstHeight = uiTmpHeight;
			DstRegion.y = DstRegion.y + (temp >> 1);
		}

		if (exif_exist) {
			ucOrientation = exif_orient;
		} else {
			ucOrientation = JPEG_EXIF_ORI_DEFAULT;
		}
		if (ucOrientation != JPEG_EXIF_ORI_DEFAULT) {
			if (ucOrientation != JPEG_EXIF_ORI_ROTATE_180) {
				UINT32 ulTmpWidth;

				ulTmpWidth = DstHeight * SrcHeight / SrcWidth * usPBDisplayWidth / pVdoDstDisp->dim.h * dst_ratio->h / dst_ratio->w;
				if (ulTmpWidth > DstWidth) {
					// Prevent user from setting strange-ratio thumbnail window
					ulTmpWidth = DstWidth / 2;
				}
				DstRegion.x += ((DstWidth - ulTmpWidth) >> 1);
				DstRegion.w = ulTmpWidth;
				DstRegion.h = DstHeight;
			} else {
				DstRegion.w = DstWidth;
				DstRegion.h = DstHeight;
			}
		} else {
			DstRegion.w = DstWidth;
			DstRegion.h = DstHeight;
		}

        if ((pVdoSrc->dim.w > PBVIEW_2PASS_W) && (pVdoSrc->dim.h > PBVIEW_2PASS_H)){
			hd_2pass_buf.blk_size = PBVIEW_2PASS_W*PBVIEW_2PASS_H*3/2;
			PBView_get_hd_common_buf(&hd_2pass_buf);

			pVdoNewSrc = &Vdo2PassSrc;
			loff[0] = PBVIEW_2PASS_W; //Besides rotate panel, the display device don't consider the line offset.
			addr[0] = hd_2pass_buf.pa;
			loff[1] = PBVIEW_2PASS_W; //Besides rotate panel, the display device don't consider the line offset.
			addr[1] = ALIGN_CEIL_4(addr[0] + PBVIEW_2PASS_W*PBVIEW_2PASS_H);
			//ret = vf_init_ex(pVdoNewSrc, PBVIEW_2PASS_W, PBVIEW_2PASS_H, HD_VIDEO_PXLFMT_YUV420, loff, addr);
			ret = vf_init_ex(pVdoNewSrc, PBVIEW_2PASS_W, (pVdoSrc->dim.h*PBVIEW_2PASS_W)/pVdoSrc->dim.w, HD_VIDEO_PXLFMT_YUV420, loff, addr);
			if (ret != HD_OK) {
				DBG_ERR("vf_init_ex dst failed\r\n");
			}
			pVdoNewSrc->blk = hd_2pass_buf.blk;
			PBView_2PassScale(pVdoSrc, pVdoNewSrc);
        }else{
            pVdoNewSrc = pVdoSrc;
		}

		memcpy((void *)&gfx_scale.src_img, (void *)pVdoNewSrc, sizeof(HD_VIDEO_FRAME));
		memcpy((void *)&gfx_scale.dst_img, (void *)pVdoDstDisp, sizeof(HD_VIDEO_FRAME));	// display device

		gfx_scale.src_region.x = 0;
		gfx_scale.src_region.y = 0;
		gfx_scale.src_region.w = pVdoNewSrc->dim.w;
		gfx_scale.src_region.h = pVdoNewSrc->dim.h;

		gfx_scale.dst_region.x = DstRegion.x;
		gfx_scale.dst_region.y = DstRegion.y;
		gfx_scale.dst_region.w = ALIGN_CEIL_4(DstRegion.w);
		gfx_scale.dst_region.h = ALIGN_CEIL_4(DstRegion.h);
		gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
		vf_gfx_scale(&gfx_scale, 1);

		#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
		// copy thumbnail image to thumbnail frame buffer
		VF_GFX_COPY gfx_copy;

		gfx_copy.src_img = gfx_scale.src_img;
		//memcpy((void *)&gfx_copy.src_img, (void *)&gfx_scale.src_img, sizeof(HD_VIDEO_FRAME));
		if ((ret = vf_init(&gfx_copy.dst_img, gfx_scale.src_img.dim.w, gfx_scale.src_img.dim.h, gfx_scale.src_img.pxlfmt, gfx_scale.src_img.loff[0], mempool_thumb_frame[i], POOL_SIZE_THUMB_FRAME)) != HD_OK) {
			DBG_ERR("vf_init fail(%d)\r\n", ret);
		}
		gfx_copy.src_region.x = 0;
		gfx_copy.src_region.y = 0;
		gfx_copy.src_region.w = gfx_scale.src_img.dim.w;
		gfx_copy.src_region.h = gfx_scale.src_img.dim.h;
		gfx_copy.dst_pos.x = 0;
		gfx_copy.dst_pos.y = 0;
		gfx_copy.colorkey = 0;
		gfx_copy.alpha = 255;
		gfx_copy.engine = 0;
		if ((ret = vf_gfx_copy(&gfx_copy)) != HD_OK) {
			DBG_ERR("vf_gfx_copy fail(%d)\r\n", ret);
		}
		memcpy((void *)&g_ThumbFrm[i], (void *)&gfx_copy.dst_img, sizeof(HD_VIDEO_FRAME));
		#endif

        if (hd_2pass_buf.blk != 0){
			ret = hd_common_mem_munmap((void *)hd_2pass_buf.va, hd_2pass_buf.blk_size);
			if (ret != HD_OK){
				DBG_ERR("failed to memory munmap\r\n");
			}

		    ret = hd_common_mem_release_block(hd_2pass_buf.blk);
			if (ret != HD_OK){
				DBG_ERR("release blk(0x%x) failed\r\n", hd_2pass_buf.blk);
			}
        }
	}

}

#if PLAY_FULL_DISP // image crop for full display
void PBView_DrawErrorView(void)
{
	HD_RESULT ret = HD_OK;
	PBVIEW_HD_COM_BUF hd_view_buf = {0};
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps = &video_out_syscaps;
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);
	HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_VIDEO_FRAME VdoDstDisp = {0}; //Display size
	VF_GFX_DRAW_RECT  DrawRectDst = {0};
	VF_GFX_DRAW_RECT *pDrawRectDst = &DrawRectDst;
	UINT32 disp_rotate = GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY);
	UINT32 swap_w_h=0;

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
	}

	//#NT#2021/1/28#Philex Lin-begin
	// swap Width/Height if panel is rotated
	if (disp_rotate)
	{
		// swap w/h
		swap_w_h=p_video_out_syscaps->output_dim.w;
		p_video_out_syscaps->output_dim.w=p_video_out_syscaps->output_dim.h;
		p_video_out_syscaps->output_dim.h=swap_w_h;
	}
	//#NT#2021/1/28#Philex Lin-end
	hd_view_buf.blk_size = p_video_out_syscaps->output_dim.w*p_video_out_syscaps->output_dim.h*3/2; //YUV420
	PBView_get_hd_common_buf(&hd_view_buf);

	loff[0] = p_video_out_syscaps->output_dim.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[0] = hd_view_buf.pa;
	loff[1] = p_video_out_syscaps->output_dim.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[1] = ALIGN_CEIL_4(addr[0] + p_video_out_syscaps->output_dim.w*p_video_out_syscaps->output_dim.h);
	ret = vf_init_ex(&VdoDstDisp, p_video_out_syscaps->output_dim.w, p_video_out_syscaps->output_dim.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);
	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
	}
	VdoDstDisp.blk = hd_view_buf.blk;

	memcpy((void *)&pDrawRectDst->dst_img, (void *)&VdoDstDisp, sizeof(HD_VIDEO_FRAME));
	pDrawRectDst->color = COLOR_RGB_BLACK;
	pDrawRectDst->rect.x = 0;
	pDrawRectDst->rect.y = 0;
	pDrawRectDst->rect.w = VdoDstDisp.dim.w;
	pDrawRectDst->rect.h = VdoDstDisp.dim.h;
	pDrawRectDst->type = HD_GFX_RECT_SOLID;
	pDrawRectDst->thickness = 0; //Don't care for HD_GFX_RECT_SOLID
	pDrawRectDst->engine = 0;
	vf_gfx_draw_rect(pDrawRectDst);

	ret = hd_videoout_start(video_out_path);
	if(ret != HD_OK){
		DBG_ERR("hd_videoout_start failed\r\n");
		return;
	}

	ret = hd_videoout_push_in_buf(video_out_path, &VdoDstDisp, NULL, 0); // only support non-blocking mode now
	if (ret != HD_OK){
		DBG_ERR("failed to push in buf of videoout\r\n");
	}

    ret = hd_common_mem_munmap((void *)hd_view_buf.va, hd_view_buf.blk_size);
	if (ret != HD_OK){
		DBG_ERR("failed to memory munmap\r\n");
	}

	ret = hd_common_mem_release_block(hd_view_buf.blk);
	if (ret != HD_OK){
		DBG_ERR("release blk(0x%x) failed\r\n", hd_view_buf.blk);
	}

}
#else
void PBView_DrawErrorView(void)
{
	HD_RESULT ret = HD_OK;
	PBVIEW_HD_COM_BUF hd_view_buf = {0};
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps = &video_out_syscaps;
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);
	HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_VIDEO_FRAME VdoDstDisp = {0}; //Display size
	VF_GFX_DRAW_RECT  DrawRectDst = {0};
	VF_GFX_DRAW_RECT *pDrawRectDst = &DrawRectDst;
	UINT32 disp_rotate = GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY);
	USIZE  dst_aspect_ratio = GxVideo_GetDeviceAspect(DOUT1);
	SIZECONVERT_INFO     CovtInfo = {0};
	HD_URECT DstDispRect = {0};

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
	}

	CovtInfo.uiSrcWidth  = 2560; //hard code
	CovtInfo.uiSrcHeight = 1440;

	if (disp_rotate) {
		CovtInfo.uiDstWidth  = p_video_out_syscaps->output_dim.h;
		CovtInfo.uiDstHeight = p_video_out_syscaps->output_dim.w;
	} else {
		CovtInfo.uiDstWidth  = p_video_out_syscaps->output_dim.w;
		CovtInfo.uiDstHeight = p_video_out_syscaps->output_dim.h;
	}

	CovtInfo.uiDstWRatio = dst_aspect_ratio.w;
	CovtInfo.uiDstHRatio = dst_aspect_ratio.h;
	CovtInfo.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
	DisplaySizeConvert(&CovtInfo);

	if (CovtInfo.uiOutWidth && CovtInfo.uiOutHeight) {
		DstDispRect.x = CovtInfo.uiOutX;
		DstDispRect.y = CovtInfo.uiOutY;
		DstDispRect.w = CovtInfo.uiOutWidth;
		DstDispRect.h = CovtInfo.uiOutHeight;
	} else {
		DstDispRect.x = 0;
		DstDispRect.y = 0;
		DstDispRect.w = p_video_out_syscaps->output_dim.w;
		DstDispRect.h = p_video_out_syscaps->output_dim.h;
	}

	hd_view_buf.blk_size = DstDispRect.w*DstDispRect.h*3/2; //YUV420
	PBView_get_hd_common_buf(&hd_view_buf);
	loff[0] = DstDispRect.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[0] = hd_view_buf.pa;
	loff[1] = DstDispRect.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[1] = ALIGN_CEIL_4(addr[0] + DstDispRect.w*DstDispRect.h);
	ret = vf_init_ex(&VdoDstDisp, DstDispRect.w, DstDispRect.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);
	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
	}

	VdoDstDisp.blk = hd_view_buf.blk;
	memcpy((void *)&pDrawRectDst->dst_img, (void *)&VdoDstDisp, sizeof(HD_VIDEO_FRAME));
	pDrawRectDst->color = COLOR_RGB_BLACK;
	pDrawRectDst->rect.x = 0;
	pDrawRectDst->rect.y = 0;
	pDrawRectDst->rect.w = VdoDstDisp.dim.w;
	pDrawRectDst->rect.h = VdoDstDisp.dim.h;
	pDrawRectDst->type = HD_GFX_RECT_SOLID;
	pDrawRectDst->thickness = 0; //Don't care for HD_GFX_RECT_SOLID
	pDrawRectDst->engine = 0;
	vf_gfx_draw_rect(pDrawRectDst);

	ret = hd_videoout_start(video_out_path);
	if(ret != HD_OK){
		DBG_ERR("hd_videoout_start failed\r\n");
		return;
	}

	ret = hd_videoout_push_in_buf(video_out_path, &VdoDstDisp, NULL, 0); // only support non-blocking mode now
	if (ret != HD_OK){
		DBG_ERR("failed to push in buf of videoout\r\n");
	}

    ret = hd_common_mem_munmap((void *)hd_view_buf.va, hd_view_buf.blk_size);
	if (ret != HD_OK){
		DBG_ERR("failed to memory munmap\r\n");
	}

	ret = hd_common_mem_release_block(hd_view_buf.blk);
	if (ret != HD_OK){
		DBG_ERR("release blk(0x%x) failed\r\n", hd_view_buf.blk);
	}

}
#endif

#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
void PBView_DrawThumbFrame(UINT32 idx, UINT32 mode)
{
	HD_RESULT ret = HD_OK;
	PBVIEW_HD_COM_BUF hd_view_buf = {0};
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	ISIZE  disp_size = GxVideo_GetDeviceSize(DOUT1);
	USIZE  dst_aspect_ratio = GxVideo_GetDeviceAspect(DOUT1);
	UINT32 thumb_w, thumb_h, buff_addr;
	VF_GFX_SCALE gfx_scale = {0};
	VF_GFX_COPY gfx_copy;

	thumb_w = g_ThumbFrm[idx].dim.w;
	thumb_h = g_ThumbFrm[idx].dim.h;

	disp_size.w /= 2; // single view only on the right half part of display
	dst_aspect_ratio.w /= 2; // single view only on the right half part of display

	// scale and crop image from thumbnail frame to right side of display buffer
	// the method should be the same as PBView_DrawSingleView except 2 pass scaling
	if ((thumb_w * dst_aspect_ratio.h) > (dst_aspect_ratio.w * thumb_h)) {
		gfx_scale.engine = 0;
		gfx_scale.src_region.w = ALIGN_CEIL_4((thumb_h * dst_aspect_ratio.w) / dst_aspect_ratio.h);
		gfx_scale.src_region.h = thumb_h;
		gfx_scale.src_region.x = (thumb_w - gfx_scale.src_region.w) / 2;
		gfx_scale.src_region.y = 0;
		gfx_scale.dst_region.w = disp_size.w;
		gfx_scale.dst_region.h = disp_size.h;
		gfx_scale.dst_region.x = 0;
		gfx_scale.dst_region.y = 0;
		gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	} else {
		gfx_scale.engine = 0;
		gfx_scale.src_region.w = thumb_w;
		gfx_scale.src_region.h = ALIGN_CEIL_4((thumb_w * dst_aspect_ratio.h) / dst_aspect_ratio.w);
		gfx_scale.src_region.x = 0;
		gfx_scale.src_region.y = (thumb_h - gfx_scale.src_region.h) / 2;
		gfx_scale.dst_region.w = disp_size.w;
		gfx_scale.dst_region.h = disp_size.h;
		gfx_scale.dst_region.x = disp_size.w; // single view is on the right side of display
		gfx_scale.dst_region.y = 0;
		gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	}

	disp_size.w *= 2; // recall original display width

	// get new display buffer
	hd_view_buf.blk_size = disp_size.w * disp_size.h * 3 / 2;
	PBView_get_hd_common_buf(&hd_view_buf);

	if (mode == THUMB_DRAW_TMP_BUFFER) { // use display tmp buffer

		// scale image to display tmp buffer firstly, then copy to new display buffer
		buff_addr = mempool_disp_tmp;

	} else { // use new display buffer

		// scale image to new display buffer directly
		buff_addr = hd_view_buf.pa;
	}

	// scale image to display buffer
	loff[0] = disp_size.w;
	addr[0] = buff_addr;
	loff[1] = disp_size.w;
	addr[1] = ALIGN_CEIL_4(addr[0] + disp_size.w * disp_size.h);
	ret = vf_init_ex(&gfx_scale.dst_img, disp_size.w, disp_size.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);
	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
		//return;
	}

	memcpy((void *)&gfx_scale.src_img, (void *)&g_ThumbFrm[idx], sizeof(HD_VIDEO_FRAME));
	vf_gfx_scale(&gfx_scale, 1);

	if (mode == THUMB_DRAW_TMP_BUFFER) { // use display tmp buffer

		// copy whole image from display tmp buffer to new display buffer then push in
		gfx_copy.src_img = g_DispTmpFrm;
		if ((ret = vf_init(&gfx_copy.dst_img, disp_size.w, disp_size.h, g_DispTmpFrm.pxlfmt, disp_size.w, hd_view_buf.pa, (disp_size.w*disp_size.h*3)/2)) != HD_OK) {
			DBG_ERR("vf_init fail(%d)\r\n", ret);
		}
		gfx_copy.src_region.w = disp_size.w; // copy whole image

	} else { // use new display buffer

		// copy left part of last video frame buffer (thumbnail layout) to new display buffer
		gfx_copy.src_img = g_LastVdoFrm;
		if ((ret = vf_init(&gfx_copy.dst_img, disp_size.w, disp_size.h, g_LastVdoFrm.pxlfmt, disp_size.w, hd_view_buf.pa, (disp_size.w*disp_size.h*3)/2)) != HD_OK) {
			DBG_ERR("vf_init fail(%d)\r\n", ret);
		}
		gfx_copy.src_region.w = disp_size.w/2; // copy half image
	}

	gfx_copy.src_region.x = 0;
	gfx_copy.src_region.y = 0;
	gfx_copy.src_region.h = disp_size.h;
	gfx_copy.dst_pos.x = 0;
	gfx_copy.dst_pos.y = 0;
	gfx_copy.colorkey = 0;
	gfx_copy.alpha = 255;
	gfx_copy.engine = 0;
	if ((ret = vf_gfx_copy(&gfx_copy)) != HD_OK) {
		DBG_ERR("vf_gfx_copy fail(%d)\r\n", ret);
	}

	ret = hd_videoout_start(video_out_path);
	if (ret != HD_OK){
		DBG_ERR("hd_videoout_start failed\r\n");
		//return;
	}

	// push in and release buffer
	gfx_copy.dst_img.blk = hd_view_buf.blk;
	ret = hd_videoout_push_in_buf(video_out_path, &gfx_copy.dst_img, NULL, 0); // only support non-blocking mode now
	if (ret != HD_OK) {
		DBG_ERR("failed to push in buf of videoout, %d\r\n", ret);
	} else {
		hd_common_mem_munmap((void *)hd_view_buf.va, hd_view_buf.blk_size);
		ret = hd_common_mem_release_block(hd_view_buf.blk);
		if (ret != HD_OK) {
			DBG_ERR("release blk(0x%x) failed\r\n", hd_view_buf.blk);
		}

		memcpy((void *)&g_LastVdoFrm, (void *)&gfx_copy.dst_img, sizeof(HD_VIDEO_FRAME));
	}
}
#endif

#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
ER PBView_OnThumbDraw(HD_VIDEO_FRAME *pHdDecVdoFrame, PUSIZE dst_ratio)
{
	ER     er;
	UINT32 uiIdx;
	UINT32 i, thumb_num=0;
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps = &video_out_syscaps;
	HD_VIDEO_FRAME VdoSrc = {0};
	HD_VIDEO_FRAME VdoDstDisp = {0}; //Display size
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);
	HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
	HD_URECT DstDispRect = {0};
	UINT32 disp_rotate = GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY);

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
	}

	DstDispRect.x = 0;
	DstDispRect.y = 0;
	if (disp_rotate) {
		DstDispRect.w = p_video_out_syscaps->output_dim.h;
		DstDispRect.h = p_video_out_syscaps->output_dim.w;
	} else {
		DstDispRect.w = p_video_out_syscaps->output_dim.w;
		DstDispRect.h = p_video_out_syscaps->output_dim.h;
	}
	loff[0] = DstDispRect.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[0] = mempool_disp_tmp;
	loff[1] = DstDispRect.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[1] = ALIGN_CEIL_4(addr[0] + DstDispRect.w * DstDispRect.h);
	ret = vf_init_ex(&VdoDstDisp, DstDispRect.w, DstDispRect.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);

	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
	}

	PB_GetParam(PBPRMID_THUMB_CURR_NUM, (UINT32 *) &thumb_num);

	for (i = 0; i < thumb_num; i++) {
		uiIdx = (i & 0x1);
		if ((er = PB_LockThumb(uiIdx)) != E_OK) {
			//locked fail indicate skip this draw
			PB_SetPBFlag(PB_SET_FLG_BROWSER_END);
			return er;
		}
		PB_GetParam(PBPRMID_INFO_IMG, (UINT32 *)&VdoSrc);
		PBView_DrawThumbView(i, &VdoSrc, &VdoDstDisp, dst_ratio);
        uiIdx = ((i+1) & 0x1);
		PB_UnlockThumb(uiIdx);
	}

	memcpy((void *)&g_DispTmpFrm, (void *)&VdoDstDisp, sizeof(HD_VIDEO_FRAME));

#if 1 // set vout

	HD_VIDEOOUT_IN video_out_param={0};
	HD_VIDEOOUT_WIN_ATTR video_out_win = {0};

	video_out_param.dim.w = DstDispRect.w;
	video_out_param.dim.h = DstDispRect.h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = SysVideo_GetDirbyID(0);
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN failed\r\n");
		return E_SYS;
	}

	video_out_win.visible = TRUE;
	if (disp_rotate) {
		video_out_win.rect.x = DstDispRect.y;
		video_out_win.rect.y = DstDispRect.x;
		video_out_win.rect.w = DstDispRect.h;
		video_out_win.rect.h = DstDispRect.w;
	} else {
		video_out_win.rect.x = DstDispRect.x;
		video_out_win.rect.y = DstDispRect.y;
		video_out_win.rect.w = DstDispRect.w;
		video_out_win.rect.h = DstDispRect.h;
	}
	video_out_win.layer = HD_LAYER1;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_win);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN_WIN_ATTR fail\r\n");
	}

#endif

	PB_SetPBFlag(PB_SET_FLG_BROWSER_END);

	return E_OK;

}
#else
ER PBView_OnThumbDraw(HD_VIDEO_FRAME *pHdDecVdoFrame, PUSIZE dst_ratio)
{
	ER     er;
	UINT32 uiIdx;
	UINT32 i, thumb_num=0;
	HD_RESULT ret = HD_OK;
	PBVIEW_HD_COM_BUF hd_view_buf = {0};
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps = &video_out_syscaps;
	HD_VIDEO_FRAME VdoSrc = {0};
	HD_VIDEO_FRAME VdoDstDisp = {0}; //Display size
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);
	HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
	HD_URECT DstDispRect = {0};
	UINT32 disp_rotate = GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY);

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
	}

	hd_view_buf.blk_size = p_video_out_syscaps->output_dim.w*p_video_out_syscaps->output_dim.h*3/2; //YUV420
	PBView_get_hd_common_buf(&hd_view_buf);

	DstDispRect.x = 0;
	DstDispRect.y = 0;
	if (disp_rotate) {
		DstDispRect.w = p_video_out_syscaps->output_dim.h;
		DstDispRect.h = p_video_out_syscaps->output_dim.w;
	} else {
		DstDispRect.w = p_video_out_syscaps->output_dim.w;
		DstDispRect.h = p_video_out_syscaps->output_dim.h;
	}
	loff[0] = DstDispRect.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[0] = hd_view_buf.pa;
	loff[1] = DstDispRect.w; //Besides rotate panel, the display device don't consider the line offset.
	addr[1] = ALIGN_CEIL_4(addr[0] + DstDispRect.w * DstDispRect.h);
	ret = vf_init_ex(&VdoDstDisp, DstDispRect.w, DstDispRect.h, HD_VIDEO_PXLFMT_YUV420, loff, addr);

	if (ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
	}
	VdoDstDisp.blk = hd_view_buf.blk;

	PB_GetParam(PBPRMID_THUMB_CURR_NUM, (UINT32 *) &thumb_num);

	for (i = 0; i < thumb_num; i++) {
		uiIdx = (i & 0x1);
		if ((er = PB_LockThumb(uiIdx)) != E_OK) {
			//locked fail indicate skip this draw
			PB_SetPBFlag(PB_SET_FLG_BROWSER_END);
			return er;
		}
		PB_GetParam(PBPRMID_INFO_IMG, (UINT32 *)&VdoSrc);
		PBView_DrawThumbView(i, &VdoSrc, &VdoDstDisp, dst_ratio);
        uiIdx = ((i+1) & 0x1);
		PB_UnlockThumb(uiIdx);
	}

#if 1 // set vout

	HD_VIDEOOUT_IN video_out_param={0};
	HD_VIDEOOUT_WIN_ATTR video_out_win = {0};

	video_out_param.dim.w = DstDispRect.w;
	video_out_param.dim.h = DstDispRect.h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = SysVideo_GetDirbyID(0);
	DBG_DUMP("PBView_OnThumbDraw: video_out_param w %d, h %d\r\n", video_out_param.dim.w, video_out_param.dim.h);
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN failed\r\n");
		return E_SYS;
	}

	video_out_win.visible = TRUE;
	if (disp_rotate) {
		video_out_win.rect.x = DstDispRect.y;
		video_out_win.rect.y = DstDispRect.x;
		video_out_win.rect.w = DstDispRect.h;
		video_out_win.rect.h = DstDispRect.w;
	} else {
		video_out_win.rect.x = DstDispRect.x;
		video_out_win.rect.y = DstDispRect.y;
		video_out_win.rect.w = DstDispRect.w;
		video_out_win.rect.h = DstDispRect.h;
	}
	video_out_win.layer = HD_LAYER1;
	DBG_DUMP("PBView_OnThumbDraw: video_out x %d, y %d, w %d, h %d\r\n", video_out_win.rect.x, video_out_win.rect.y, video_out_win.rect.w, video_out_win.rect.h);
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_win);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set: HD_VIDEOOUT_PARAM_IN_WIN_ATTR fail\r\n");
	}

	ret = hd_videoout_start(video_out_path);
	if(ret != HD_OK){
		DBG_ERR("hd_videoout_start failed\r\n");
		return E_SYS;
	}

#endif

    VdoDstDisp.blk = hd_view_buf.blk;
	ret = hd_videoout_push_in_buf(video_out_path, &VdoDstDisp, NULL, 0); // only support non-blocking mode now
	if (ret != HD_OK){
		DBG_ERR("failed to push in buf of videoout\r\n");
	}else{
		ret = hd_common_mem_munmap((void *)hd_view_buf.va, hd_view_buf.blk_size);
		if (ret != HD_OK){
			DBG_ERR("failed to memory munmap\r\n");
		}

	    ret = hd_common_mem_release_block(hd_view_buf.blk);
		if (ret != HD_OK){
			DBG_ERR("release blk(0x%x) failed\r\n", hd_view_buf.blk);
		}

		memcpy((void *)&g_LastVdoFrm, (void *)&VdoDstDisp, sizeof(HD_VIDEO_FRAME));
	}

	PB_SetPBFlag(PB_SET_FLG_BROWSER_END);

	return E_OK;

}
#endif

void PBView_OnDrawCB(PB_VIEW_STATE view_state, HD_VIDEO_FRAME *pHdDecVdoFrame)
{
	UINT32 uiFileNum = 0;
	UINT32 u32CurrPbStatus = 0;

    PB_GetParam(PBPRMID_TOTAL_FILE_COUNT, &uiFileNum);
	PB_GetParam(PBPRMID_PLAYBACK_STATUS, &u32CurrPbStatus);

	if ((uiFileNum == 0) || (u32CurrPbStatus != PB_STA_DONE)){
		PBView_DrawErrorView();
		PB_SetPBFlag(PB_SET_FLG_DRAW_END);
		return;
	}

	if (view_state == PB_VIEW_STATE_SINGLE) {
		gfpDrawCb = PBView_OnSingleDraw;
	} else if (view_state == PB_VIEW_STATE_THUMB) {
		gfpDrawCb = PBView_OnThumbDraw;
	}

	gPbState = view_state;
	aspect_ratio = GxVideo_GetDeviceAspect(DOUT1);

    if (gPbState == PB_VIEW_STATE_THUMB) {
		gfpDrawCb(0, &aspect_ratio);
   	}else{
   		gfpDrawCb(pHdDecVdoFrame, &aspect_ratio);
   	}
}

#if _TODO //refer to NA51055-840 JIRA and using new method
void PBView_KeepLastView(void)
{
	HD_RESULT hd_ret = HD_OK;
    PBVIEW_HD_COM_BUF hd_view_buf = {0};
	HD_COMMON_MEM_DDR_ID ddr_id = 0;
	VF_GFX_COPY copy_param;
	HD_PATH_ID video_out_path = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_PATH);

	hd_view_buf.blk_size = g_LastVdoFrm.dim.w*g_LastVdoFrm.loff[0]*3/2;
	if ((hd_ret = hd_common_mem_alloc("NVTMPP_TEMP", &hd_view_buf.pa, (void **)&hd_view_buf.va, hd_view_buf.blk_size, ddr_id)) != HD_OK) {
		DBG_ERR("hd_common_mem_alloc fail(%d)\r\n", hd_ret);
		return;
	}

	copy_param.src_img = g_LastVdoFrm;
	if ((hd_ret = vf_init(&(copy_param.dst_img), g_LastVdoFrm.dim.w, g_LastVdoFrm.dim.h, g_LastVdoFrm.pxlfmt, g_LastVdoFrm.loff[0], hd_view_buf.pa, hd_view_buf.blk_size)) != HD_OK) {
		DBG_ERR("vf_init fail(%d)\r\n", hd_ret);
		return;
	}
	copy_param.src_region.x = 0;
	copy_param.src_region.y = 0;
	copy_param.src_region.w = g_LastVdoFrm.dim.w;
	copy_param.src_region.h = g_LastVdoFrm.dim.h;
	copy_param.dst_pos.x = 0;
	copy_param.dst_pos.y = 0;
	copy_param.colorkey = 0;
	copy_param.alpha = 255;
	copy_param.engine = 0;
	if ((hd_ret = vf_gfx_copy(&copy_param)) != HD_OK) {
		DBG_ERR("vf_gfx_copy fail(%d)\r\n", hd_ret);
	}
	copy_param.dst_img.blk = -2;
	if ((hd_ret = hd_videoout_push_in_buf(video_out_path, &(copy_param.dst_img), NULL, 500)) != HD_OK) {
		DBG_ERR("hd_videoout_push_in_buf fail(%d)\r\n", hd_ret);
	}
}
#endif
