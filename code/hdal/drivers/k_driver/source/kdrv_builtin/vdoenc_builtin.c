
#if defined(__KERNEL__)

#define LVGL_BUILTIN_STAMP 1

#if LVGL_BUILTIN_STAMP

#define LVGL_BUILTIN_STAMP_TYPE_OSG 0
#define LVGL_BUILTIN_STAMP_TYPE_YUV 1
#define LVGL_BUILTIN_STAMP_TYPE LVGL_BUILTIN_STAMP_TYPE_OSG

#include "lv_user_font_conv.h" /* must before include linux/module.h */
#include <linux/rtc.h>
#include <linux/io.h>
#include <plat/rtc_reg.h>
#include <plat/rtc_int.h>
#include "kwrap/cpu.h"

#endif


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/math64.h>

#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include "kwrap/debug.h"
#include "kwrap/util.h"
#include "kwrap/spinlock.h"

#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videoenc/kdrv_videoenc_lmt.h"
#include "kdrv_ipp_builtin.h"
#include "vdoenc_builtin.h"
#include "nvtmpp_init.h"
#include "h26x_def.h"
#include "h26x.h"
#include "h264_def.h"
#include "h264enc_api.h"
#include "h265_def.h"
#include "h265enc_api.h"
#include "jpeg.h"
#include "jpg_enc.h"
#include <plat/top.h>
#include "bridge.h"
#include "osg/osg_internal.h"

#define FLG_VDOENC_BUILTIN_IDLE         	FLGPTN_BIT(0) //0x00000001
#define FLG_VDOENC_BUILTIN_ENCODE       	FLGPTN_BIT(1) //0x00000002
#define FLG_VDOENC_BUILTIN_STOP         	FLGPTN_BIT(2) //0x00000004
#define FLG_VDOENC_BUILTIN_STOP_DONE    	FLGPTN_BIT(3) //0x00000008

#define FLG_VDOENC_BUILTIN_DONE         	FLGPTN_BIT(4) //0x00000010

THREAD_HANDLE                                   VDOENC_BUILTIN_TSK_ID_H26X = 0;
THREAD_HANDLE                                   VDOENC_BUILTIN_TSK_ID_JPEG = 0;
ID                                              FLG_ID_VDOENC_BUILTIN_H26X = 0;
ID                                              FLG_ID_VDOENC_BUILTIN_JPEG = 0;
ID                                              FLG_ID_VDOENC_BUILTIN_CHECK[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
SEM_HANDLE                                      VDOENC_BUILTIN_BS_SEM_ID = 0;

VDOENC_BUILTIN_OBJ gVdoEncBuiltinObj[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
VDOENC_BUILTIN_JOBQ gVdoEncBuiltinJobQ_H26X = {0};
VDOENC_BUILTIN_JOBQ gVdoEncBuiltinJobQ_JPEG = {0};
H26XENC_VAR  enc_var[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
H264ENC_INIT init_avc_obj[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
H264ENC_INFO info_avc_obj[BUILTIN_VDOENC_PATH_ID_MAX] = {0};

H265ENC_INIT init_hevc_obj[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
H265ENC_INFO info_hevc_obj[BUILTIN_VDOENC_PATH_ID_MAX] = {0};

static UINT32 gVdoEnc_en[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 gVdoEnc_CodecType[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 bsque_max[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 gFrameRate[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 gSec[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static int yuv_unlock_cnt[BUILTIN_VDOENC_PATH_ID_MAX] = {0}; // yuv_unlock_cnt == 2, set builtin_stop_encode as TRUE
static UINT32 builtin_stop_encode[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 gThisFrmIdx[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 gJpegEncIdx[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 gBSStart[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 gBSEnd[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 orig_yuv_width[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 orig_yuv_height[BUILTIN_VDOENC_PATH_ID_MAX] = {0};

// jpeg builtin dtsi
static UINT32 jpeg_quality[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 jpeg_fps[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 jpeg_width[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 jpeg_height[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
static UINT32 jpeg_max_mem_size[BUILTIN_VDOENC_PATH_ID_MAX] = {0};

// dynamic settings
static UINT32 dynamic_codec[BUILTIN_VDOENC_PATH_ID_MAX] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static UINT32 dynamic_rc_byterate[BUILTIN_VDOENC_PATH_ID_MAX] = {0};

// 3DNR
SIE_FB_ISR_FP vdoenc_builtin_get_sie_fp = NULL;
H26XEncTnrCfg h26x_enc_tnr[ISP_ID_MAX] = {0};

// Rate control
VDOENC_BUILTIN_RC_PARAM gRcParam[BUILTIN_VDOENC_PATH_ID_MAX] = {0};

// NALU size address
UINT32 *avc_nalu_size_addr = 0;
UINT32 *hevc_nalu_size_addr = 0;

// sensor name
//static UINT32 sensor1 = 0, sensor2 = 0;

/********************************************************
 * LVGL
 *******************************************************/
#if LVGL_BUILTIN_STAMP

#define LV_USER_FONT_CONV_ALIGN_W 8
#define LV_USER_FONT_CONV_ALIGN_H 2

#define LV_USER_FONT_CONV_DATETIME_FMT "%04ld/%02d/%02d %02d:%02d:%02d"
#define LV_USER_FONT_CONV_DATETIME_FMT_ARG(X) X.tm_year, X.tm_mon, X.tm_mday, X.tm_hour, X.tm_min, X.tm_sec

#define FLG_LVGL_BUILTIN_IDLE         	FLGPTN_BIT(0) //0x00000001
#define FLG_LVGL_BUILTIN_FONT_CONV      FLGPTN_BIT(1) //0x00000002
#define FLG_LVGL_BUILTIN_STOP         	FLGPTN_BIT(2) //0x00000004
#define FLG_LVGL_BUILTIN_STOP_DONE    	FLGPTN_BIT(3) //0x00000008

THREAD_HANDLE                                   LVGL_BUILTIN_TSK_ID = 0;
SEM_HANDLE                                      LVGL_BUILTIN_SEM_ID = 0;
ID                                              FLG_ID_LVGL_BUILTIN = 0;

extern lv_font_t LV_USER_CFG_STAMP_FONT_XXL;
extern lv_font_t LV_USER_CFG_STAMP_FONT_XL;
extern lv_font_t LV_USER_CFG_STAMP_FONT_LARGE;
extern lv_font_t LV_USER_CFG_STAMP_FONT_MEDIUM;
extern lv_font_t LV_USER_CFG_STAMP_FONT_SMALL;

#if	LVGL_BUILTIN_STAMP_TYPE == LVGL_BUILTIN_STAMP_TYPE_OSG
static OSG osg;
static OSG_STAMP stamp[1] = {0};
static uintptr_t osg_buffer = 0; /* ping pong */

int Lvgl_BuiltIn_Config_OSG(
		lv_user_font_conv_calc_buffer_size_result *mem_rst,
		lv_user_font_conv_mem_cfg* mem_cfg,
		UINT32 x,
		UINT32 y
);

#endif

static lv_user_font_conv_draw_cfg lv_draw_cfg = {0};
static lv_user_font_conv_calc_buffer_size_result lv_calc_mem_rst = {0};
static lv_user_font_conv_mem_cfg lv_stamp_mem_cfg = {0};
static char lv_datetime_string[64] = {'\0'};
static UINT32 stamp_pos_x = 0, stamp_pos_y = 0;

static int Lvgl_BuiltIn_Font_Conv(
		VDOENC_BUILTIN_YUV_INFO *yuv_info,
		lv_user_font_conv_calc_buffer_size_result *mem_rst,
		lv_user_font_conv_mem_cfg *mem_cfg,
		UINT32 dst_x, UINT32 dst_y
);

int Lvgl_BuiltIn_Calc_Pos(
		UINT32 img_width,
		UINT32 imag_height,
		lv_user_font_conv_calc_buffer_size_result *mem_rst,
		lv_user_font_conv_draw_cfg *draw_cfg,
		UINT32 *x,
		UINT32 *y
);

void Lvgl_BuiltIn_Init(
		UINT32 image_width,
		UINT32 image_height,
		lv_user_font_conv_draw_cfg *draw_cfg,
		lv_user_font_conv_calc_buffer_size_result *mem_rst,
		lv_user_font_conv_mem_cfg *mem_cfg
);

static int Lvgl_BuiltIn_TskStart(void);
static void Lvgl_BuiltIn_TskStop(void);
static int Lvgl_Builtin_rtc_init(void);
static int Lvgl_Builtin_rtc_read_time(struct tm *datetime);

static lv_font_t* Lvgl_BuiltIn_Select_Font(
		UINT32 image_width,
		UINT32 image_height
);

static const unsigned short rtc_ydays[2][13] = {
	/* Normal years */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	/* Leap years */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#define IOADDR_BASE                 0xF0000000
#define IOADDR_RTC_REG_BASE         (IOADDR_BASE+0x00060000)
static unsigned int _REGIOBASE = 0;

#endif

static VK_DEFINE_SPINLOCK(my_lock);

UINT32 VdoEnc_Builtin_spin_lock(void)
{
	unsigned long flags;
	vk_spin_lock_irqsave(&my_lock, flags);
	return flags;
}

void VdoEnc_Builtin_spin_unlock(UINT32 flags)
{
	vk_spin_unlock_irqrestore(&my_lock, flags);
}

UINT32 _VdoEnc_builtin_GetBytesPerSecond(UINT32 width, UINT32 height)
{
	UINT32 bytePerSec;

	bytePerSec = width * height / 5;

	return bytePerSec;
}

BOOL _VdoEnc_Builtin_PutYuv(UINT32 pathID, VDOENC_BUILTIN_YUV_INFO *pYuvInfo)
{
	VDOENC_BUILTIN_YUVQ *pObj;
	unsigned long         flags;

	flags = VdoEnc_Builtin_spin_lock();

	pObj = &(gVdoEncBuiltinObj[pathID].yuvQueue);

	DBG_IND("%s:(line%d) pathID=%d, f=%d, r=%d, full=%d\r\n", __func__, __LINE__, pathID, pObj->Front, pObj->Rear, pObj->bFull);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		VdoEnc_Builtin_spin_unlock(flags);
		return FALSE;
	} else {
		pObj->Queue[pObj->Rear].enable = pYuvInfo->enable;
		pObj->Queue[pObj->Rear].y_addr = pYuvInfo->y_addr;
		pObj->Queue[pObj->Rear].c_addr = pYuvInfo->c_addr;
		pObj->Queue[pObj->Rear].width = pYuvInfo->width;
		pObj->Queue[pObj->Rear].height = pYuvInfo->height;
		pObj->Queue[pObj->Rear].y_line_offset = pYuvInfo->y_line_offset;
		pObj->Queue[pObj->Rear].c_line_offset = pYuvInfo->c_line_offset;
		pObj->Queue[pObj->Rear].timestamp = pYuvInfo->timestamp;
		pObj->Queue[pObj->Rear].release_flag = pYuvInfo->release_flag;

		if (pObj->Queue[pObj->Rear].release_flag && pObj->Queue[pObj->Rear].enable) {
			DBG_IND("%s [%d] enable=%d,0x%08x\r\n",__func__,pathID,pYuvInfo->enable,pObj->Queue[pObj->Rear].y_addr);
			nvtmpp_lock_fastboot_blk(pObj->Queue[pObj->Rear].y_addr); //lock yuv blk
		}

		pObj->Rear = (pObj->Rear + 1) % VDOENC_BUILTIN_YUVQ_MAX;
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		VdoEnc_Builtin_spin_unlock(flags);
		return TRUE;
	}
}

BOOL _VdoEnc_Builtin_GetYuv(UINT32 pathID, VDOENC_BUILTIN_YUV_INFO *pYuvInfo)
{
	VDOENC_BUILTIN_YUVQ *pObj;
	unsigned long         flags;

	flags = VdoEnc_Builtin_spin_lock();

	pObj = &(gVdoEncBuiltinObj[pathID].yuvQueue);

	DBG_IND("%s:(line%d) pathID=%d, f=%d, r=%d, full=%d\r\n", __func__, __LINE__, pathID, pObj->Front, pObj->Rear, pObj->bFull);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		VdoEnc_Builtin_spin_unlock(flags);
		DBG_ERR("get yuv queue empty!\r\n");
		return FALSE;
	} else {
		pYuvInfo->enable = pObj->Queue[pObj->Front].enable;
		pYuvInfo->y_addr = pObj->Queue[pObj->Front].y_addr;
		pYuvInfo->c_addr = pObj->Queue[pObj->Front].c_addr;
		pYuvInfo->width = pObj->Queue[pObj->Front].width;
		pYuvInfo->height = pObj->Queue[pObj->Front].height;
		pYuvInfo->y_line_offset = pObj->Queue[pObj->Front].y_line_offset;
		pYuvInfo->c_line_offset = pObj->Queue[pObj->Front].c_line_offset;
		pYuvInfo->timestamp = pObj->Queue[pObj->Front].timestamp;
		pYuvInfo->release_flag = pObj->Queue[pObj->Front].release_flag;
		pObj->Front = (pObj->Front + 1) % VDOENC_BUILTIN_YUVQ_MAX;
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}
		VdoEnc_Builtin_spin_unlock(flags);
		return TRUE;
	}
}

UINT32 _VdoEnc_Builtin_HowManyInYUVQ(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	VDOENC_BUILTIN_YUVQ *pObj;

	//SEM_WAIT(VDOENC_BUILTIN_YUV_SEM_ID); //wai_sem(NMP_VDODEC_SEM_ID[pathID]);
	pObj = &(gVdoEncBuiltinObj[pathID].yuvQueue);
	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	//SEM_SIGNAL(VDOENC_BUILTIN_YUV_SEM_ID); //sig_sem(NMP_VDODEC_SEM_ID[pathID]);
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = VDOENC_BUILTIN_YUVQ_MAX - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = VDOENC_BUILTIN_YUVQ_MAX;
	} else {
		sq = 0;
	}

	DBG_IND("%s:(line%d) sq=%d\r\n", __func__, __LINE__, sq);

	return sq;
}

BOOL _VdoEnc_builtin_PutJob_H26X(UINT32 pathID)
{
	unsigned long         flags;

	flags = VdoEnc_Builtin_spin_lock();
	if ((gVdoEncBuiltinJobQ_H26X.Front == gVdoEncBuiltinJobQ_H26X.Rear) && (gVdoEncBuiltinJobQ_H26X.bFull == TRUE)) {
		VdoEnc_Builtin_spin_unlock(flags);
		return FALSE;
	} else {
		gVdoEncBuiltinJobQ_H26X.Queue[gVdoEncBuiltinJobQ_H26X.Rear].pathID = pathID;
		gVdoEncBuiltinJobQ_H26X.Rear = (gVdoEncBuiltinJobQ_H26X.Rear + 1) % VDOENC_BUILTIN_JOBQ_MAX;
		if (gVdoEncBuiltinJobQ_H26X.Front == gVdoEncBuiltinJobQ_H26X.Rear) { // Check Queue full
			gVdoEncBuiltinJobQ_H26X.bFull = TRUE;
		}
		VdoEnc_Builtin_spin_unlock(flags);
		return TRUE;
	}
}

BOOL _VdoEnc_builtin_GetJob_H26X(UINT32 *pPathID)
{
	unsigned long         flags;

	flags = VdoEnc_Builtin_spin_lock();
	if ((gVdoEncBuiltinJobQ_H26X.Front == gVdoEncBuiltinJobQ_H26X.Rear) && (gVdoEncBuiltinJobQ_H26X.bFull == FALSE)) {
		VdoEnc_Builtin_spin_unlock(flags);
		DBG_ERR("[VDOTRIG] Job Queue(H26x) is Empty!\r\n");
		return FALSE;
	} else {
		*pPathID = gVdoEncBuiltinJobQ_H26X.Queue[gVdoEncBuiltinJobQ_H26X.Front].pathID;
		gVdoEncBuiltinJobQ_H26X.Front = (gVdoEncBuiltinJobQ_H26X.Front + 1) % VDOENC_BUILTIN_JOBQ_MAX;
		if (gVdoEncBuiltinJobQ_H26X.Front == gVdoEncBuiltinJobQ_H26X.Rear) { // Check Queue full
			gVdoEncBuiltinJobQ_H26X.bFull = FALSE;
		}
		VdoEnc_Builtin_spin_unlock(flags);
		return TRUE;
	}
}

UINT32 _VdoEnc_builtin_GetJobCount_H26X(void)
{
	UINT32 front, rear, full, sq = 0;
	unsigned long         flags;

	flags = VdoEnc_Builtin_spin_lock();
	front = gVdoEncBuiltinJobQ_H26X.Front;
	rear = gVdoEncBuiltinJobQ_H26X.Rear;
	full = gVdoEncBuiltinJobQ_H26X.bFull;
	VdoEnc_Builtin_spin_unlock(flags);

	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (VDOENC_BUILTIN_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = VDOENC_BUILTIN_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}

BOOL _VdoEnc_builtin_PutJob_JPEG(UINT32 pathID)
{
	unsigned long		  flags;

	flags = VdoEnc_Builtin_spin_lock();
	if ((gVdoEncBuiltinJobQ_JPEG.Front == gVdoEncBuiltinJobQ_JPEG.Rear) && (gVdoEncBuiltinJobQ_JPEG.bFull == TRUE)) {
		VdoEnc_Builtin_spin_unlock(flags);
		return FALSE;
	} else {
		gVdoEncBuiltinJobQ_JPEG.Queue[gVdoEncBuiltinJobQ_JPEG.Rear].pathID = pathID;
		gVdoEncBuiltinJobQ_JPEG.Rear = (gVdoEncBuiltinJobQ_JPEG.Rear + 1) % VDOENC_BUILTIN_JOBQ_MAX;
		if (gVdoEncBuiltinJobQ_JPEG.Front == gVdoEncBuiltinJobQ_JPEG.Rear) { // Check Queue full
			gVdoEncBuiltinJobQ_JPEG.bFull = TRUE;
		}
		VdoEnc_Builtin_spin_unlock(flags);
		return TRUE;
	}
}

BOOL _VdoEnc_builtin_GetJob_JPEG(UINT32 *pPathID)
{
	unsigned long		  flags;

	flags = VdoEnc_Builtin_spin_lock();
	if ((gVdoEncBuiltinJobQ_JPEG.Front == gVdoEncBuiltinJobQ_JPEG.Rear) && (gVdoEncBuiltinJobQ_JPEG.bFull == FALSE)) {
		VdoEnc_Builtin_spin_unlock(flags);
		DBG_ERR("[VDOTRIG] Job Queue(H26x) is Empty!\r\n");
		return FALSE;
	} else {
		*pPathID = gVdoEncBuiltinJobQ_JPEG.Queue[gVdoEncBuiltinJobQ_JPEG.Front].pathID;
		gVdoEncBuiltinJobQ_JPEG.Front = (gVdoEncBuiltinJobQ_JPEG.Front + 1) % VDOENC_BUILTIN_JOBQ_MAX;
		if (gVdoEncBuiltinJobQ_JPEG.Front == gVdoEncBuiltinJobQ_JPEG.Rear) { // Check Queue full
			gVdoEncBuiltinJobQ_JPEG.bFull = FALSE;
		}
		VdoEnc_Builtin_spin_unlock(flags);
		return TRUE;
	}
}

UINT32 _VdoEnc_builtin_GetJobCount_JPEG(void)
{
	UINT32 front, rear, full, sq = 0;
	unsigned long		  flags;

	flags = VdoEnc_Builtin_spin_lock();
	front = gVdoEncBuiltinJobQ_JPEG.Front;
	rear = gVdoEncBuiltinJobQ_JPEG.Rear;
	full = gVdoEncBuiltinJobQ_JPEG.bFull;
	VdoEnc_Builtin_spin_unlock(flags);

	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (VDOENC_BUILTIN_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = VDOENC_BUILTIN_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}

void _VdoEnc_builtin_3DNR_Internal(UINT32 pathID, UINT32 p3DNRConfig)
{
	SIE_FB_ISR_FP fp = vdoenc_builtin_get_sie_fp;
	UINT32 isp_id   = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_ISP_ID);

	if (fp != NULL)  {
		fp(isp_id, BUILTIN_ISP_EVENT_ENC_TNR);   // iq will then call vdoenc_builtin_set() to update 3DNR parameters
	}
#if TNR_DBG
	DBG_DUMP("[VDOENC_BUILTIN][%d] isp_id = %d, SIE tnr = (%d, %d, %d, %d), fp = 0x%08x\r\n",
		pathID,
		isp_id,
		h26x_enc_tnr[isp_id].nr_3d_mode,
		h26x_enc_tnr[isp_id].nr_3d_adp_th_p2p[0],
		h26x_enc_tnr[isp_id].nr_3d_adp_th_p2p[1],
		h26x_enc_tnr[isp_id].nr_3d_adp_th_p2p[2],
		(UINT32)fp);
#endif

	// copy iq 3DNR parameters to H26x driver
	memcpy((void *)p3DNRConfig, &h26x_enc_tnr[isp_id], sizeof(H26XEncTnrCfg));
}

void _VdoEnc_builtin_Get_SrcYuv_Info(VDOENC_BUILTIN_FMD_INFO *pSrc, VDOENC_BUILTIN_YUV_INFO *pDst, UINT32 ipp_pathID)
{
	pDst->enable = pSrc->out_img[ipp_pathID].enable;
	pDst->y_addr = pSrc->out_img[ipp_pathID].addr[0];
	pDst->c_addr = pSrc->out_img[ipp_pathID].addr[1];
	pDst->width = pSrc->out_img[ipp_pathID].size.w;
	pDst->height = pSrc->out_img[ipp_pathID].size.h;
	pDst->y_line_offset = pSrc->out_img[ipp_pathID].loff[0];
	pDst->c_line_offset = pSrc->out_img[ipp_pathID].loff[1];
	pDst->timestamp = (UINT32)pSrc->out_img[ipp_pathID].timestamp;
	pDst->release_flag = pSrc->release_flg;
}

BOOL vdoenc_builtin_set(UINT32 id, BUILTIN_VDOENC_ISP_ITEM item, void *data)
{
	if (item == BUILTIN_VDOENC_ISP_ITEM_TNR) {
		memcpy(&h26x_enc_tnr[id], data, sizeof(H26XEncTnrCfg));
	}

	return TRUE;
}

BOOL vdoenc_builtin_evt_fp_reg(CHAR *name, SIE_FB_ISR_FP fp)
{
	vdoenc_builtin_get_sie_fp = fp;  // Let PQ team register callback function

	return TRUE;
}

BOOL vdoenc_builtin_evt_fp_unreg(CHAR *name)
{
	vdoenc_builtin_get_sie_fp = NULL;  // Let PQ team register callback function

	return TRUE;
}

BOOL VdoEnc_Builtin_AllocQueMem(UINT32 pathID, UINT32 bsq_max_num)
{
	UINT32 uiBufSize = sizeof(VDOENC_BUILTIN_BS_INFO) * bsq_max_num;
	PVDOENC_BUILTIN_BSQ pObj;

	if (uiBufSize == 0) {
		DBG_ERR("[VDOBUILTIN] AllocQueMem fail\r\n");
		return FALSE;
	}
	pObj = &(gVdoEncBuiltinObj[pathID].bsQueue);
	pObj->Queue = (VDOENC_BUILTIN_BS_INFO *)vmalloc(uiBufSize);

	return TRUE;
}

void VdoEnc_Builtin_FreeQueMem(UINT32 pathID)
{
	PVDOENC_BUILTIN_BSQ pObj;
	PVDOENC_BUILTIN_JOBQ pJobQH26X;
	PVDOENC_BUILTIN_JOBQ pJobQJPEG;

	pObj = &(gVdoEncBuiltinObj[pathID].bsQueue);
	if (pObj->Queue != NULL) {
		vfree(pObj->Queue);
	}

	pJobQH26X = &gVdoEncBuiltinJobQ_H26X;
	if (pJobQH26X->Queue != NULL) {
		vfree(pJobQH26X->Queue);
	}

	pJobQJPEG = &gVdoEncBuiltinJobQ_JPEG;
	if (pJobQJPEG->Queue != NULL) {
		vfree(pJobQJPEG->Queue);
	}
}

BOOL VdoEnc_Builtin_PutBS(UINT32 pathID, BOOL isKeyFrm, VDOENC_BUILTIN_PARAM *enc_param, H26XEncResultCfg *enc_result)
{
	PVDOENC_BUILTIN_BSQ pObj;

	if (enc_param == NULL) {
		DBG_ERR("[VDOBUILTIN] BS Lock Error enc_param is NULL\r\n");
		return FALSE;
	}

	if (enc_param->bs_addr_1 == 0 || enc_param->bs_size_1== 0) {
		DBG_ERR("[VDOBUILTIN] BS Lock Error Addr 0x%x Size is %d\r\n", enc_param->bs_addr_1, enc_param->bs_size_1);
		return FALSE;
	}

	SEM_WAIT(VDOENC_BUILTIN_BS_SEM_ID);

	pObj = &(gVdoEncBuiltinObj[pathID].bsQueue);

	if (gVdoEnc_CodecType[pathID] == BUILTIN_VDOENC_H264 || gVdoEnc_CodecType[pathID] == BUILTIN_VDOENC_H265) {
		if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
			//DBG_ERR("[VdoEncBuiltin] BS Lock Queue is Full!\r\n");
			SEM_SIGNAL(VDOENC_BUILTIN_BS_SEM_ID);
			return FALSE;
		} else {
			pObj->Queue[pObj->Rear].Addr = enc_param->bs_addr_1;
			pObj->Queue[pObj->Rear].Size = enc_param->bs_size_1;
			pObj->Queue[pObj->Rear].temproal_id = enc_result->uiSvcLable;
			pObj->Queue[pObj->Rear].re_encode_en = (enc_param->interrupt != H26X_FINISH_INT);
			pObj->Queue[pObj->Rear].timestamp = enc_param->timestamp;
			pObj->Queue[pObj->Rear].nxt_frm_type = enc_result->ucNxtPicType;
			pObj->Queue[pObj->Rear].base_qp = enc_result->ucQP;
			pObj->Queue[pObj->Rear].bs_size_1 = enc_result->uiBSLen;
			pObj->Queue[pObj->Rear].frm_type = enc_result->ucPicType;
			pObj->Queue[pObj->Rear].encode_time = enc_result->uiHwEncTime;
			pObj->Queue[pObj->Rear].isKeyFrame = isKeyFrm;
			pObj->Queue[pObj->Rear].nalu_num = enc_param->nalu_num;
			pObj->Queue[pObj->Rear].nalu_size_addr = enc_param->nalu_size_addr;
			pObj->Rear = (pObj->Rear + 1) % bsque_max[pathID];
			if (pObj->Front == pObj->Rear) { // Check Queue full
				pObj->bFull = TRUE;
			}
		}
	} else if (gVdoEnc_CodecType[pathID] == BUILTIN_VDOENC_MJPEG) {
		if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
			//DBG_ERR("[VdoEncBuiltin] BS Lock Queue is Full!\r\n");
			SEM_SIGNAL(VDOENC_BUILTIN_BS_SEM_ID);
			return FALSE;
		} else {
			pObj->Queue[pObj->Rear].Addr = enc_param->bs_addr_1;
			pObj->Queue[pObj->Rear].Size = enc_param->bs_size_1;
			pObj->Queue[pObj->Rear].temproal_id = pathID;
			pObj->Queue[pObj->Rear].re_encode_en = 0;
			pObj->Queue[pObj->Rear].timestamp = enc_param->timestamp;
			pObj->Queue[pObj->Rear].nxt_frm_type = 0;//enc_result->ucNxtPicType;
			pObj->Queue[pObj->Rear].base_qp = enc_param->base_qp;
			pObj->Queue[pObj->Rear].bs_size_1 = enc_param->bs_size_1;//enc_result->uiBSLen;
			pObj->Queue[pObj->Rear].frm_type = enc_param->frm_type;
			pObj->Queue[pObj->Rear].encode_time = 0;//uiHwEncTime;
			pObj->Queue[pObj->Rear].isKeyFrame = 0;
			pObj->Rear = (pObj->Rear + 1) % bsque_max[pathID];
			if (pObj->Front == pObj->Rear) { // Check Queue full
				pObj->bFull = TRUE;
			}
		}
	}
	SEM_SIGNAL(VDOENC_BUILTIN_BS_SEM_ID);
	return TRUE;
}

BOOL VdoEnc_Builtin_GetBS(UINT32 pathID, VDOENC_BUILTIN_BS_INFO *builtin_bs_info)
{
	VDOENC_BUILTIN_BSQ *pObj;

	SEM_WAIT(VDOENC_BUILTIN_BS_SEM_ID);

	pObj = &(gVdoEncBuiltinObj[pathID].bsQueue);

	DBG_IND("%s:(line%d) pathID=%d, f=%d, r=%d, full=%d\r\n", __func__, __LINE__, pathID, pObj->Front, pObj->Rear, pObj->bFull);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(VDOENC_BUILTIN_BS_SEM_ID);
		DBG_ERR("get bs queue empty!\r\n");
		return FALSE;
	} else {
		builtin_bs_info->Addr = pObj->Queue[pObj->Front].Addr;
		builtin_bs_info->Size = pObj->Queue[pObj->Front].Size;
		builtin_bs_info->temproal_id = pObj->Queue[pObj->Front].temproal_id;
		builtin_bs_info->re_encode_en = pObj->Queue[pObj->Front].re_encode_en;
		builtin_bs_info->timestamp = pObj->Queue[pObj->Front].timestamp;
		builtin_bs_info->nxt_frm_type = pObj->Queue[pObj->Front].nxt_frm_type;
		builtin_bs_info->base_qp = pObj->Queue[pObj->Front].base_qp;
		builtin_bs_info->bs_size_1 = pObj->Queue[pObj->Front].bs_size_1;
		builtin_bs_info->frm_type = pObj->Queue[pObj->Front].frm_type;
		builtin_bs_info->encode_time = pObj->Queue[pObj->Front].encode_time;
		builtin_bs_info->isKeyFrame = pObj->Queue[pObj->Front].isKeyFrame;
		builtin_bs_info->nalu_num = pObj->Queue[pObj->Front].nalu_num;
		builtin_bs_info->nalu_size_addr = pObj->Queue[pObj->Front].nalu_size_addr;
		pObj->Front = (pObj->Front + 1) % bsque_max[pathID];

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}

		SEM_SIGNAL(VDOENC_BUILTIN_BS_SEM_ID);
		return TRUE;
	}
}

UINT32 VdoEnc_Builtin_HowManyInBSQ(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	VDOENC_BUILTIN_BSQ *pObj;

	SEM_WAIT(VDOENC_BUILTIN_BS_SEM_ID);
	pObj = &(gVdoEncBuiltinObj[pathID].bsQueue);
	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	SEM_SIGNAL(VDOENC_BUILTIN_BS_SEM_ID);
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = bsque_max[pathID] - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = bsque_max[pathID];
	} else {
		sq = 0;
	}

	DBG_IND("%s:(line%d) sq=%d\r\n", __func__, __LINE__, sq);

	return sq;
}

BOOL VdoEnc_Builtin_GetEncVar(UINT32 pathID, void *kdrv_vdoenc_var)
{
	memcpy((H26XENC_VAR *)kdrv_vdoenc_var, &enc_var[pathID], sizeof(H26XENC_VAR));
	return TRUE;
}

UINT32 VdoEnc_Builtin_SetParam(UINT32 pathID, UINT32 Param, UINT32 Value)
{
	switch (Param) {
	case BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_CODEC:
		dynamic_codec[pathID] = Value;
		break;

	case BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_BYTERATE:
		dynamic_rc_byterate[pathID] = Value;
		break;

	case BUILTIN_VDOENC_INIT_PARAM_FREE_MEM: {
		UINT32 *pAddr = (UINT32 *)Value;
		vfree(pAddr);
		}
		break;

	default:
		DBG_ERR("[VDOENCBUILTIN] Get invalid param = %d\r\n", Param);
		return -1;
	}
	return 0;
}

UINT32 VdoEnc_Builtin_GetParam(UINT32 pathID, UINT32 Param, UINT32 *pValue)
{
	UINT32 codectype = 0;
	codectype = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_CODECTYPE);

	switch (Param) {
	case BUILTIN_VDOENC_INIT_PARAM_ENC_EN:
		*pValue = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_ENC_EN);
		break;

	case BUILTIN_VDOENC_INIT_PARAM_CODEC:
		*pValue = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_CODECTYPE);
		break;

	case BUILTIN_VDOENC_INIT_PARAM_DIRECT:
		*pValue = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_H26X_ROTATE);
		break;

	case BUILTIN_VDOENC_INIT_PARAM_WIDTH:
		if (codectype == BUILTIN_VDOENC_MJPEG) {
			*pValue = jpeg_width[pathID];
		} else {
			*pValue = orig_yuv_width[pathID];
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_HEIGHT:
		if (codectype == BUILTIN_VDOENC_MJPEG) {
			*pValue = jpeg_height[pathID];
		} else {
			*pValue = orig_yuv_height[pathID];
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_FRAME_RATE:
		*pValue = gFrameRate[pathID] * 1000;
		break;

	case BUILTIN_VDOENC_INIT_PARAM_PROFILE:
		if (init_avc_obj[pathID].eProfile == 100) {
			*pValue = 2;
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_LEVEL_IDC:
		if (codectype == BUILTIN_VDOENC_H265) {
			*pValue = init_hevc_obj[pathID].ucLevelIdc;
		} else if (codectype == BUILTIN_VDOENC_H264) {
			*pValue = init_avc_obj[pathID].ucLevelIdc;
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_GOP_NUM:
		if (codectype == BUILTIN_VDOENC_H265) {
			*pValue = init_hevc_obj[pathID].uiGopNum;
		} else if (codectype == BUILTIN_VDOENC_H264) {
			*pValue = init_avc_obj[pathID].uiGopNum;
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_LTR_INTERVAL:
		if (codectype == BUILTIN_VDOENC_H265) {
			*pValue = init_hevc_obj[pathID].uiLTRInterval;
		} else if (codectype == BUILTIN_VDOENC_H264) {
			*pValue = init_avc_obj[pathID].uiLTRInterval;
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_LTR_PRE_REF:
		if (codectype == BUILTIN_VDOENC_H265) {
			*pValue = init_hevc_obj[pathID].uiLTRPreRef;
		} else if (codectype == BUILTIN_VDOENC_H264) {
			*pValue = init_avc_obj[pathID].bLTRPreRef;
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_GRAY_EN:
		if (codectype == BUILTIN_VDOENC_H265) {
			*pValue = init_hevc_obj[pathID].bGrayEn;
		} else if (codectype == BUILTIN_VDOENC_H264) {
			*pValue = init_avc_obj[pathID].bGrayEn;
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_SRC_OUT:
		if (codectype == BUILTIN_VDOENC_H265) {
			*pValue = 0;
		} else if (codectype == BUILTIN_VDOENC_H264) {
			*pValue = info_avc_obj[pathID].bSrcOutEn;
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_SVC:
		if (codectype == BUILTIN_VDOENC_H265) {
			*pValue = init_hevc_obj[pathID].ucSVCLayer;
		} else if (codectype == BUILTIN_VDOENC_H264) {
			*pValue = init_avc_obj[pathID].ucSVCLayer;
		}
		break;

	case BUILTIN_VDOENC_INIT_PARAM_ENTROPY:
		*pValue = init_avc_obj[pathID].eEntropyMode;
		break;

	case BUILTIN_VDOENC_INIT_PARAM_BSQ_MAX:
		*pValue = bsque_max[pathID];
		break;

	case BUILTIN_VDOENC_INIT_PARAM_SEC:
		*pValue = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SEC);
		break;

	case BUILTIN_VDOENC_INIT_PARAM_JPEG_QUALITY:
		*pValue = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_JPEG_QUALITY);
		break;

	case BUILTIN_VDOENC_INIT_PARAM_JPEG_FPS:
		*pValue = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_JPEG_FPS);
		break;

	case BUILTIN_VDOENC_INIT_PARAM_JPEG_MAX_MEM_SIZE:
		*pValue = jpeg_max_mem_size[pathID];
		break;

	case BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_CODEC:
		*pValue = dynamic_codec[pathID];
		break;

	case BUILTIN_VDOENC_INIT_PARAM_BS_START:
		*pValue = gBSStart[pathID];
		break;

	case BUILTIN_VDOENC_INIT_PARAM_BS_END:
		*pValue = gBSEnd[pathID];
		break;

	case BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_BYTERATE:
		*pValue = dynamic_rc_byterate[pathID];
		break;

	case BUILTIN_VDOENC_INIT_PARAM_RATE_CONTROL:
		memcpy((VDOENC_BUILTIN_RC_PARAM *)pValue, &gRcParam[pathID], sizeof(VDOENC_BUILTIN_RC_PARAM));
		break;

	default:
		DBG_ERR("[VDOENCBUILTIN] Get invalid param = %d\r\n", Param);
		return -1;
	}
	return 0;
}

UINT32 VdoEnc_Builtin_CheckBuiltinStop(UINT32 pathID)
{
	FLGPTN uiFlag;
	ER ret = E_OK;
	UINT32 i = 0;

	for (i = 0; i < BUILTIN_VDOENC_PATH_ID_MAX; i++) {
		if (gVdoEnc_en[i]) {
			ret = vos_flag_wait_timeout(&uiFlag, FLG_ID_VDOENC_BUILTIN_CHECK[i], FLG_VDOENC_BUILTIN_DONE, TWF_ORW, vos_util_msec_to_tick(1000));
			if (ret != E_OK) {
				DBG_ERR("[%d] VdoEnc_Builtin_CheckBuiltinStop time out, pathID %d\r\n", i, pathID);
			}
		}
	}

	return 0;
}

int VdoEnc_builtin_get_dtsi_param(UINT32 pathID, BUILTIN_VDOENC_DTSI_PARAM param)
{
	UINT32 value = 0;

	struct device_node* of_node;
	const char *path[BUILTIN_VDOENC_PATH_ID_MAX] = {
		"/fastboot/venc/venc0",
		"/fastboot/venc/venc1",
		"/fastboot/venc/venc2",
		"/fastboot/venc/venc3",
		"/fastboot/venc/venc4",
		"/fastboot/venc/venc5",
	};

	of_node = of_find_node_by_path(path[pathID]);

	switch (param) {
		case BUILTIN_VDOENC_DTSI_PARAM_ENC_EN:
			if (of_node) {
				if (of_property_read_u32(of_node, "enable", &value) != 0) {
					DBG_ERR("cannot find %s/enable", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_CODECTYPE:
			if (of_node) {
				if (of_property_read_u32(of_node, "codectype", &value) != 0) {
					DBG_ERR("cannot find %s/codectype", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_SRC_VPRC_DEV:
			if (of_node) {
				if (of_property_read_u32(of_node, "vprc_src_dev", &value) != 0) {
					DBG_ERR("cannot find %s/vprc_src_dev", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_SRC_VPRC_PATH:
			if (of_node) {
				if (of_property_read_u32(of_node, "vprc_src_path", &value) != 0) {
					DBG_ERR("cannot find %s/vprc_src_path", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_INIT_BYTE_RATE:
			if (of_node) {
				if (of_property_read_u32(of_node, "init_byte_rate", &value) != 0) {
					DBG_ERR("cannot find %s/bitrate", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_FRAMERATE:
			if (of_node) {
				if (of_property_read_u32(of_node, "framerate", &value) != 0) {
					DBG_ERR("cannot find %s/framerate", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_SEC:
			if (of_node) {
				if (of_property_read_u32(of_node, "sec", &value) != 0) {
					DBG_ERR("cannot find %s/sec", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_GOP:
			if (of_node) {
				if (of_property_read_u32(of_node, "gop", &value) != 0) {
					DBG_ERR("cannot find %s/gop", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_QP:
			if (of_node) {
				if (of_property_read_u32(of_node, "qp", &value) != 0) {
					DBG_ERR("cannot find %s/qp", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_JPEG_QUALITY:
			if (of_node) {
				if (of_property_read_u32(of_node, "jpeg_quality", &value) != 0) {
					DBG_ERR("cannot find %s/jpeg_quality", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_JPEG_FPS:
			if (of_node) {
				if (of_property_read_u32(of_node, "jpeg_fps", &value) != 0) {
					DBG_ERR("cannot find %s/jpeg_fps", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_RC_MODE:
			if (of_node) {
				if (of_property_read_u32(of_node, "rc", &value) != 0) {
					pr_err("cannot find %s/rc", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP:
			if (of_node) {
				if (of_property_read_u32(of_node, "init_i_qp", &value) != 0) {
					pr_err("cannot find %s/init_i_qp", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_MIN_IQP:
			if (of_node) {
				if (of_property_read_u32(of_node, "min_i_qp", &value) != 0) {
					pr_err("cannot find %s/min_i_qp", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_MAX_IQP:
			if (of_node) {
				if (of_property_read_u32(of_node, "max_i_qp", &value) != 0) {
					pr_err("cannot find %s/max_i_qp", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP:
			if (of_node) {
				if (of_property_read_u32(of_node, "init_p_qp", &value) != 0) {
					pr_err("cannot find %s/init_p_qp", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_MIN_PQP:
			if (of_node) {
				if (of_property_read_u32(of_node, "min_p_qp", &value) != 0) {
					pr_err("cannot find %s/min_p_qp", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_MAX_PQP:
			if (of_node) {
				if (of_property_read_u32(of_node, "max_p_qp", &value) != 0) {
					pr_err("cannot find %s/max_p_qp", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_RC_BYTE_RATE:
			if (of_node) {
				if (of_property_read_u32(of_node, "rc_byte_rate", &value) != 0) {
					pr_err("cannot find %s/rc_byte_rate", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_STATIC_TIME:
			if (of_node) {
				if (of_property_read_u32(of_node, "static_time", &value) != 0) {
					pr_err("cannot find %s/static_time", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_IP_WEIGHT:
			if (of_node) {
				if (of_property_read_u32(of_node, "ip_weight", &value) != 0) {
					pr_err("cannot find %s/ip_weight", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_KEY_P_PERIOD:
			if (of_node) {
				if (of_property_read_u32(of_node, "key_p_period", &value) != 0) {
					pr_err("cannot find %s/key_p_period", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_KP_WEIGHT:
			if (of_node) {
				if (of_property_read_u32(of_node, "kp_weight", &value) != 0) {
					pr_err("cannot find %s/kp_weight", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_P2_WEIGHT:
			if (of_node) {
				if (of_property_read_u32(of_node, "p2_weight", &value) != 0) {
					pr_err("cannot find %s/p2_weight", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_P3_WEIGHT:
			if (of_node) {
				if (of_property_read_u32(of_node, "p3_weight", &value) != 0) {
					pr_err("cannot find %s/p3_weight", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_LT_WEIGHT:
			if (of_node) {
				if (of_property_read_u32(of_node, "lt_weight", &value) != 0) {
					pr_err("cannot find %s/lt_weight", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_MOTION_AQ_STR:
			if (of_node) {
				if (of_property_read_u32(of_node, "motion_aq_str", &value) != 0) {
					pr_err("cannot find %s/motion_aq_str", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_STILL_FRM_CND:
			if (of_node) {
				if (of_property_read_u32(of_node, "still_frm_cnd", &value) != 0) {
				pr_err("cannot find %s/still_frm_cnd", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_MOTION_RATIO_THD:
			if (of_node) {
				if (of_property_read_u32(of_node, "motion_ratio_thd", &value) != 0) {
					pr_err("cannot find %s/motion_ratio_thd", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_I_PSNR_CND:
			if (of_node) {
				if (of_property_read_u32(of_node, "i_psnr_cnd", &value) != 0) {
					pr_err("cannot find %s/i_psnr_cnd", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_P_PSNR_CND:
			if (of_node) {
				if (of_property_read_u32(of_node, "p_psnr_cnd", &value) != 0) {
					pr_err("cannot find %s/p_psnr_cnd", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_KP_PSNR_CND:
			if (of_node) {
				if (of_property_read_u32(of_node, "kp_psnr_cnd", &value) != 0) {
					pr_err("cannot find %s/kp_psnr_cnd", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_CHANGE_POSITION:
			if (of_node) {
				if (of_property_read_u32(of_node, "change_pos", &value) != 0) {
					pr_err("cannot find %s/change_pos", path[pathID]);
				}
			}
		break;

		case BUILTIN_VDOENC_DTSI_PARAM_SVC_LAYER:
			if (of_node) {
				if (of_property_read_u32(of_node, "svc_layer", &value) != 0) {
					DBG_ERR("cannot find %s/svc_layer", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_LTR_INTERVAL:
			if (of_node) {
				if (of_property_read_u32(of_node, "ltr_interval", &value) != 0) {
					DBG_ERR("cannot find %s/ltr_interval", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_D2D:
			if (of_node) {
				if (of_property_read_u32(of_node, "d2d", &value) != 0) {
					DBG_ERR("cannot find %s/d2d", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_GDC:
			if (of_node) {
				if (of_property_read_u32(of_node, "gdc", &value) != 0) {
					DBG_ERR("cannot find %s/gdc", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_COLMV:
			if (of_node) {
				if (of_property_read_u32(of_node, "colmv", &value) != 0) {
					DBG_ERR("cannot find %s/colmv", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_QUALITY_LV:
			if (of_node) {
				if (of_property_read_u32(of_node, "qualitylv", &value) != 0) {
					DBG_ERR("cannot find %s/qualitylv", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_ISP_ID:
			if (of_node) {
				if (of_property_read_u32(of_node, "isp_id", &value) != 0) {
					DBG_ERR("cannot find %s/isp_id", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_H26X_ROTATE:
			if (of_node) {
				if (of_property_read_u32(of_node, "h26x_rotate", &value) != 0) {
					pr_err("cannot find /fastboot/venc/h26x_rotate");
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_SRCOUT_SIZE:
			if (of_node) {
				if (of_property_read_u32(of_node, "srcout_size", &value) != 0) {
					DBG_ERR("cannot find %s/srcout_size", path[pathID]);
				}
			}
			break;

		case BUILTIN_VDOENC_DTSI_PARAM_SVC_WEIGHT_MODE:
			if (of_node) {
				if (of_property_read_u32(of_node, "svc_weight_mode", &value) != 0) {
					pr_err("cannot find %s/svc_weight_mode", path[pathID]);
				}
			}
		break;

		default:
		DBG_ERR("[VDOENCBUILTIN] Get invalid param = %d\r\n", param);
		value = -1;
	}

	return value;
}

void VdoEnc_builtin_set_rate_control(UINT32 pathID)
{
	UINT32 rc_mode = 0;

	rc_mode = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_RC_MODE);

	if (rc_mode == BUILTIN_VDOENC_RC_CBR) {
		H26XEncRCParam sRcParam = {0};

		gRcParam[pathID].uiEncId = sRcParam.uiEncId = pathID;
		gRcParam[pathID].uiRCMode = sRcParam.uiRCMode = H26X_RC_CBR;
		gRcParam[pathID].uiInitIQp = sRcParam.uiInitIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP);
		gRcParam[pathID].uiMinIQp = sRcParam.uiMinIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_IQP);
		gRcParam[pathID].uiMaxIQp = sRcParam.uiMaxIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_IQP);
		gRcParam[pathID].uiInitPQp = sRcParam.uiInitPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP);
		gRcParam[pathID].uiMinPQp = sRcParam.uiMinPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_PQP);
		gRcParam[pathID].uiMaxPQp = sRcParam.uiMaxPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_PQP);
		gRcParam[pathID].uiBitRate = sRcParam.uiBitRate = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_RC_BYTE_RATE)*8;
		gRcParam[pathID].uiFrameRateBase = sRcParam.uiFrameRateBase = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_FRAMERATE)*1000;
		gRcParam[pathID].uiFrameRateIncr = sRcParam.uiFrameRateIncr = 1000;
		gRcParam[pathID].uiGOP = sRcParam.uiGOP = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_GOP);
		gRcParam[pathID].uiRowLevelRCEnable = sRcParam.uiRowLevelRCEnable = 1;
		gRcParam[pathID].uiStaticTime = sRcParam.uiStaticTime = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_STATIC_TIME);
		gRcParam[pathID].iIPWeight = sRcParam.iIPWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_IP_WEIGHT);
		gRcParam[pathID].uiKeyPPeriod = sRcParam.uiKeyPPeriod = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_KEY_P_PERIOD);
		gRcParam[pathID].iKPWeight = sRcParam.iKPWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_KP_WEIGHT);
		gRcParam[pathID].iP2Weight = sRcParam.iP2Weight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_P2_WEIGHT);
		gRcParam[pathID].iP3Weight = sRcParam.iP3Weight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_P3_WEIGHT);
		gRcParam[pathID].iLTWeight = sRcParam.iLTWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_LT_WEIGHT);
		gRcParam[pathID].iMotionAQStrength = sRcParam.iMotionAQStrength = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MOTION_AQ_STR);
		gRcParam[pathID].uiSvcBAMode = sRcParam.uiSvcBAMode = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SVC_WEIGHT_MODE);

		h26XEnc_setRcInit(&enc_var[pathID], &sRcParam);
	} else if (rc_mode == BUILTIN_VDOENC_RC_EVBR) {
		H26XEncRCParam sRcParam = {0};

		gRcParam[pathID].uiEncId = sRcParam.uiEncId = pathID;
		gRcParam[pathID].uiRCMode = sRcParam.uiRCMode = H26X_RC_EVBR;
		gRcParam[pathID].uiInitIQp = sRcParam.uiInitIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP);
		gRcParam[pathID].uiMinIQp = sRcParam.uiMinIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_IQP);
		gRcParam[pathID].uiMaxIQp = sRcParam.uiMaxIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_IQP);
		gRcParam[pathID].uiInitPQp = sRcParam.uiInitPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP);
		gRcParam[pathID].uiMinPQp = sRcParam.uiMinPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_PQP);
		gRcParam[pathID].uiMaxPQp = sRcParam.uiMaxPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_PQP);
		gRcParam[pathID].uiBitRate = sRcParam.uiBitRate = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_RC_BYTE_RATE)*8;
		gRcParam[pathID].uiFrameRateBase = sRcParam.uiFrameRateBase = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_FRAMERATE)*1000;
		gRcParam[pathID].uiFrameRateIncr = sRcParam.uiFrameRateIncr = 1000;
		gRcParam[pathID].uiGOP = sRcParam.uiGOP = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_GOP);
		gRcParam[pathID].uiRowLevelRCEnable = sRcParam.uiRowLevelRCEnable = 1;
		gRcParam[pathID].uiStaticTime = sRcParam.uiStaticTime = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_STATIC_TIME);
		gRcParam[pathID].iIPWeight = sRcParam.iIPWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_IP_WEIGHT);
		gRcParam[pathID].iKPWeight = sRcParam.iKPWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_KP_WEIGHT);
		gRcParam[pathID].uiKeyPPeriod = sRcParam.uiKeyPPeriod = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_KEY_P_PERIOD);
		gRcParam[pathID].iMotionAQStrength = sRcParam.iMotionAQStrength = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MOTION_AQ_STR);
		gRcParam[pathID].uiStillFrameCnd = sRcParam.uiStillFrameCnd = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_STILL_FRM_CND);
		gRcParam[pathID].uiMotionRatioThd = sRcParam.uiMotionRatioThd = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MOTION_RATIO_THD);
		gRcParam[pathID].uiIPsnrCnd = sRcParam.uiIPsnrCnd = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_I_PSNR_CND);
		gRcParam[pathID].uiPPsnrCnd = sRcParam.uiPPsnrCnd = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_P_PSNR_CND);
		gRcParam[pathID].uiKeyPPsnrCnd = sRcParam.uiKeyPPsnrCnd = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_KP_PSNR_CND);
		gRcParam[pathID].uiMinStillIQp = sRcParam.uiMinStillIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_PQP);
		gRcParam[pathID].iP2Weight = sRcParam.iP2Weight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_P2_WEIGHT);
		gRcParam[pathID].iP3Weight = sRcParam.iP3Weight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_P3_WEIGHT);
		gRcParam[pathID].iLTWeight = sRcParam.iLTWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_LT_WEIGHT);
		gRcParam[pathID].uiSvcBAMode = sRcParam.uiSvcBAMode = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SVC_WEIGHT_MODE);

		h26XEnc_setRcInit(&enc_var[pathID], &sRcParam);
	} else if (rc_mode == BUILTIN_VDOENC_RC_VBR || rc_mode == BUILTIN_VDOENC_RC_VBR2) {
		H26XEncRCParam sRcParam = {0};

		gRcParam[pathID].uiEncId = sRcParam.uiEncId = pathID;
		if (rc_mode == BUILTIN_VDOENC_RC_VBR2) {
			gRcParam[pathID].uiRCMode = sRcParam.uiRCMode = H26X_RC_VBR2;
		} else {
			gRcParam[pathID].uiRCMode = sRcParam.uiRCMode = H26X_RC_VBR;
		}
		gRcParam[pathID].uiInitIQp = sRcParam.uiInitIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP);
		gRcParam[pathID].uiMinIQp = sRcParam.uiMinIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_IQP);
		gRcParam[pathID].uiMaxIQp = sRcParam.uiMaxIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_IQP);
		gRcParam[pathID].uiInitPQp = sRcParam.uiInitPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP);
		gRcParam[pathID].uiMinPQp = sRcParam.uiMinPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_PQP);
		gRcParam[pathID].uiMaxPQp = sRcParam.uiMaxPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_PQP);
		gRcParam[pathID].uiBitRate = sRcParam.uiBitRate = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_RC_BYTE_RATE)*8;
		gRcParam[pathID].uiFrameRateBase = sRcParam.uiFrameRateBase = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_FRAMERATE)*1000;
		gRcParam[pathID].uiFrameRateIncr = sRcParam.uiFrameRateIncr = 1000;
		gRcParam[pathID].uiGOP = sRcParam.uiGOP = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_GOP);
		gRcParam[pathID].uiRowLevelRCEnable = sRcParam.uiRowLevelRCEnable = 1;
		gRcParam[pathID].uiStaticTime = sRcParam.uiStaticTime = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_STATIC_TIME);
		gRcParam[pathID].iIPWeight = sRcParam.iIPWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_IP_WEIGHT);
		gRcParam[pathID].uiChangePos = sRcParam.uiChangePos = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_CHANGE_POSITION);
		gRcParam[pathID].uiKeyPPeriod = sRcParam.uiKeyPPeriod = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_KEY_P_PERIOD);
		gRcParam[pathID].iKPWeight = sRcParam.iKPWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_KP_WEIGHT);
		gRcParam[pathID].iP2Weight = sRcParam.iP2Weight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_P2_WEIGHT);
		gRcParam[pathID].iP3Weight = sRcParam.iP3Weight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_P3_WEIGHT);
		gRcParam[pathID].iLTWeight = sRcParam.iLTWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_LT_WEIGHT);
		gRcParam[pathID].iMotionAQStrength = sRcParam.iMotionAQStrength = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MOTION_AQ_STR);
		gRcParam[pathID].uiSvcBAMode = sRcParam.uiSvcBAMode = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SVC_WEIGHT_MODE);

		h26XEnc_setRcInit(&enc_var[pathID], &sRcParam);
	} else if (rc_mode == BUILTIN_VDOENC_RC_FIXQP) {
		H26XEncRCParam sRcParam = {0};

		gRcParam[pathID].uiEncId = sRcParam.uiEncId = pathID;
		gRcParam[pathID].uiRCMode = sRcParam.uiRCMode = H26X_RC_FixQp;
		gRcParam[pathID].uiMinIQp = sRcParam.uiMinIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP);
		gRcParam[pathID].uiFixIQp = sRcParam.uiFixIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP);
		gRcParam[pathID].uiMaxIQp = sRcParam.uiMaxIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP);
		gRcParam[pathID].uiMinPQp = sRcParam.uiMinPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP);
		gRcParam[pathID].uiFixPQp = sRcParam.uiFixPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP);
		gRcParam[pathID].uiMaxPQp = sRcParam.uiMaxPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP);
		gRcParam[pathID].uiFrameRateBase = sRcParam.uiFrameRateBase = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_FRAMERATE)*1000;
		gRcParam[pathID].uiFrameRateIncr = sRcParam.uiFrameRateIncr = 1000;

		h26XEnc_setRcInit(&enc_var[pathID], &sRcParam);
	}


	return;
}

UINT32 _vdoenc_builtin_nalu_num_max(void)
{
	// TODO : nvt_get_chip_id() to use different chip define for H26XE_SLICE_MAX_NUM
	return H26XE_SLICE_MAX_NUM;
}

UINT32 _vdoenc_builtin_nalu_size_info_max_size(void)
{
	return 64 + ALIGN_CEIL_64(sizeof(UINT32) + _vdoenc_builtin_nalu_num_max() *(sizeof(UINT32)+sizeof(UINT32)));   // nalu_num + 176 * (nalu_addr + nalu_size) , 176 is HW limit for 52x
}

int H264Enc_builtin_init(UINT32 pathID, VDOENC_BUILTIN_INIT_INFO *p_info)
{
	UINT32 width = 0, height = 0, rotate = 0;
	UINT32 svc_layer = 0, ltr_interval = 0, quality_lv = 0, d2d = 0, gdc = 0, colmv = 0;
	unsigned int venc_ctrl_0[2] = {0};

	/* H264 codec buffer */
	H26XEncMeminfo memInfo = {0};
	UINT32 codec_mem_addr = 0, codec_mem_size = 0;
	UINT32 sizePerSec = 0, uiMinIRatio = 150, uiMinPRatio = 100;
	UINT32 srcout_size = 0;

	VDOENC_BUILTIN_OBJ *p_obj = &gVdoEncBuiltinObj[pathID];

	memset(&gVdoEncBuiltinObj[pathID], 0, sizeof(VDOENC_BUILTIN_OBJ));
	memset(&enc_var[pathID], 0, sizeof(H26XENC_VAR));
	memset(&init_avc_obj[pathID], 0, sizeof(H264ENC_INIT));
	memset(&info_avc_obj[pathID], 0, sizeof(H264ENC_INFO));

	orig_yuv_width[pathID] = p_info[pathID].width;
	orig_yuv_height[pathID] = p_info[pathID].height;
	rotate = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_H26X_ROTATE);
	if (rotate == 0 || rotate == 3) {
		width = p_info[pathID].width;
		height = p_info[pathID].height;
	} else {
		width = p_info[pathID].height;
		height = p_info[pathID].width;
	}
	venc_ctrl_0[0] = p_info[pathID].max_blk_addr;
	venc_ctrl_0[1] = p_info[pathID].max_blk_size;
	if (width == 0 || height == 0) {
		DBG_ERR("error size width %d, height %d\r\n", (int)width, (int)height);
		return -1;
	}

	svc_layer = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SVC_LAYER);
	ltr_interval = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_LTR_INTERVAL);
	quality_lv = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_QUALITY_LV);
	d2d = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_D2D);
	gdc = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_GDC);
	colmv = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_COLMV);

	// get H264 codec buffer size
	{
		memInfo.uiWidth = width;
		memInfo.uiHeight = height;
		memInfo.ucSVCLayer = svc_layer;
		memInfo.uiLTRInterval = ltr_interval;
		memInfo.ucQualityLevel = quality_lv;
		memInfo.bTileEn= 0;
		memInfo.bD2dEn= d2d;
		memInfo.bGdcEn= gdc;
		memInfo.bColMvEn = colmv;
		memInfo.bCommReconFrmBuf = 0;

		codec_mem_addr = venc_ctrl_0[0];
		codec_mem_size = h264Enc_queryMemSize(&memInfo);
		if (codec_mem_size == 0) {
			DBG_ERR("H264Enc query error MemSize 0\r\n");
			return -1;
		}
	}

	p_obj->venc_param.codec_mem_addr = codec_mem_addr;
	p_obj->venc_param.codec_mem_size = codec_mem_size;
	p_obj->venc_param.bs_start_addr = p_obj->venc_param.codec_mem_addr + ALIGN_CEIL_4(p_obj->venc_param.codec_mem_size);

	srcout_size = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SRCOUT_SIZE);

	p_obj->venc_param.bs_end_addr = venc_ctrl_0[0] + venc_ctrl_0[1] - VDOENC_MD_MAP_MAX_SIZE - VDOENC_BUF_RESERVED_BYTES - srcout_size;
	gBSStart[pathID] = p_obj->venc_param.bs_start_addr;
	gBSEnd[pathID] = p_obj->venc_param.bs_end_addr;
	p_obj->venc_param.bs_addr_1 = p_obj->venc_param.bs_start_addr;
	p_obj->venc_param.bs_size_1 = p_obj->venc_param.bs_end_addr - p_obj->venc_param.bs_start_addr;
	sizePerSec = _VdoEnc_builtin_GetBytesPerSecond(width, height);
	p_obj->venc_param.bs_min_i_size = (UINT32)div_u64((UINT64)sizePerSec * (UINT64)uiMinIRatio, 100);
	p_obj->venc_param.bs_min_p_size = (UINT32)div_u64((UINT64)sizePerSec * (UINT64)uiMinPRatio, 100);

	DBG_IND("codec_addr 0x%x, codec_size 0x%x, bs_start_addr 0x%x, bs_end_addr 0x%x, bs_addr_1 0x%x, bs_size_1 0x%x\r\n",
			(unsigned int)p_obj->venc_param.codec_mem_addr,
			(unsigned int)p_obj->venc_param.codec_mem_size,
			(unsigned int)p_obj->venc_param.bs_start_addr,
			(unsigned int)p_obj->venc_param.bs_end_addr,
			(unsigned int)p_obj->venc_param.bs_addr_1,
			(unsigned int)p_obj->venc_param.bs_size_1);

	// Init encode engine
	{
		init_avc_obj[pathID].uiDisplayWidth = width;
		init_avc_obj[pathID].uiWidth  = ALIGN_CEIL_16(init_avc_obj[pathID].uiDisplayWidth);
		init_avc_obj[pathID].uiHeight = height;
		init_avc_obj[pathID].uiEncBufAddr = codec_mem_addr;
		init_avc_obj[pathID].uiEncBufSize = codec_mem_size;
		init_avc_obj[pathID].uiGopNum = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_GOP);
		init_avc_obj[pathID].ucDisLFIdc = 0 & 0x3; // remove 3'b100 (TileMode), which is H265 only
		init_avc_obj[pathID].cDBAlpha = 0;
		init_avc_obj[pathID].cDBBeta = 0;
		init_avc_obj[pathID].cChrmQPIdx = 0;
		init_avc_obj[pathID].cSecChrmQPIdx = 0;
		init_avc_obj[pathID].ucSVCLayer = svc_layer;
		init_avc_obj[pathID].uiLTRInterval = ltr_interval;
		init_avc_obj[pathID].bLTRPreRef = 0;
		init_avc_obj[pathID].uiBitRate = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_BYTE_RATE)*8;

		gFrameRate[pathID] = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_FRAMERATE);
		gSec[pathID] = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SEC);
		init_avc_obj[pathID].uiFrmRate = gFrameRate[pathID] * 1000;
		bsque_max[pathID] = gFrameRate[pathID] * gSec[pathID];
		if (!VdoEnc_Builtin_AllocQueMem(pathID, bsque_max[pathID])){
			DBG_ERR("VdoEnc Builtin AllocQueMem fail !!!\r\n");
			return -1;
		}

		init_avc_obj[pathID].ucIQP = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP);
		init_avc_obj[pathID].ucPQP = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP);
		init_avc_obj[pathID].ucMaxIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_IQP);
		init_avc_obj[pathID].ucMinIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_IQP);
		init_avc_obj[pathID].ucMaxPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_PQP);
		init_avc_obj[pathID].ucMinPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_PQP);
		if (VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_RC_MODE) == BUILTIN_VDOENC_RC_FIXQP) {
			init_avc_obj[pathID].iIPWeight = 0;
			init_avc_obj[pathID].uiStaticTime = 0;
		} else {
			init_avc_obj[pathID].iIPWeight = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_IP_WEIGHT);
			init_avc_obj[pathID].uiStaticTime = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_STATIC_TIME);
		}
		init_avc_obj[pathID].bFBCEn = 0;
		init_avc_obj[pathID].bGrayEn = 0;
		init_avc_obj[pathID].bFastSearchEn = 0;//pinfo->fast_search;	// nt98520 not support search range set to 1 : big range //
		init_avc_obj[pathID].bHwPaddingEn = 1;
		init_avc_obj[pathID].ucRotate = rotate;
		init_avc_obj[pathID].bD2dEn = d2d;
		init_avc_obj[pathID].bColMvEn = colmv;

		init_avc_obj[pathID].eProfile = PROFILE_HIGH;
		init_avc_obj[pathID].eEntropyMode = 1; //1: KDRV_VDOENC_CABAC, 0: KDRV_VDOENC_CAVLC
		if (init_avc_obj[pathID].uiWidth > 1920 && init_avc_obj[pathID].uiHeight > 1080) {
			init_avc_obj[pathID].ucLevelIdc = 51; //LEVEL_51
		} else {
			init_avc_obj[pathID].ucLevelIdc = 41; //LEVEL_51
		}
		init_avc_obj[pathID].bTrans8x8En = 1;
		init_avc_obj[pathID].bForwardRecChrmEn = 0;

		init_avc_obj[pathID].bVUIEn = 0;
		init_avc_obj[pathID].usSarWidth = width;
		init_avc_obj[pathID].usSarHeight = height;
		init_avc_obj[pathID].ucMatrixCoef = 0;
		init_avc_obj[pathID].ucTransferCharacteristics = 0;
		init_avc_obj[pathID].ucColourPrimaries = 0;
		init_avc_obj[pathID].ucVideoFormat = 0;
		init_avc_obj[pathID].ucColorRange = 0;
		init_avc_obj[pathID].bTimeingPresentFlag = 0;
		init_avc_obj[pathID].bRecBufComm = 0;
		init_avc_obj[pathID].uiRecBufAddr[0] = 0;
		init_avc_obj[pathID].uiRecBufAddr[1] = 0;
		init_avc_obj[pathID].uiRecBufAddr[2] = 0;

		h264Enc_initEncoder(&init_avc_obj[pathID], &enc_var[pathID]);

		{
			UINT32 desc_addr = 0;
			UINT32 desc_size = 0;
			UINT8 *ptr;

			h264Enc_getSeqHdr(&enc_var[pathID], &desc_addr, &desc_size);
			ptr = (UINT8 *)desc_addr;
			if (*(ptr+0) != 0x00 || *(ptr+1) != 0x00 || *(ptr+2) != 0x00 || *(ptr+3) != 0x01 || *(ptr+4) != 0x67) {
				DBG_ERR("H264 error descriptor\r\n");
				return -1;
			}
		}
	}

	return 0;
}

void H264Enc_builtin_encodeOne(UINT32 pathID)
{
	UINT32 width = 0, height = 0;
	UINT32 interrupt = 0;
	BOOL is_keyfrm = 0;
	H26XEncResultCfg sResult = {0};
	H26XEncNaluLenResult nalu_len_rslt = {0};
	VDOENC_BUILTIN_OBJ *p_obj = &gVdoEncBuiltinObj[pathID];
	VDOENC_BUILTIN_YUV_INFO yuv_info = {0};

	if (_VdoEnc_Builtin_HowManyInYUVQ(pathID) <= 0) {
		return;
	}

	_VdoEnc_Builtin_GetYuv(pathID, &yuv_info);


#if LVGL_BUILTIN_STAMP

	Lvgl_BuiltIn_Font_Conv(
			&yuv_info,
			&lv_calc_mem_rst,
			&lv_stamp_mem_cfg,
			stamp_pos_x,
			stamp_pos_y
	);

#endif


	if (p_obj->bsQueue.bFull) {
		//DBG_DUMP("Bs Queue Full, Not Encode Further\r\n");
		if (yuv_info.release_flag) {
			if (yuv_info.enable) {
				DBG_IND("%s_1 0x%08x\r\n",__func__,yuv_info.y_addr);
				nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
			}
			yuv_unlock_cnt[pathID]++;
			if (yuv_unlock_cnt[pathID] == 2) {
				builtin_stop_encode[pathID] = TRUE;
				set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
			}
		}
		return;
	}

	width = yuv_info.width;
	height = yuv_info.height;

	if (vdoenc_builtin_get_sie_fp != NULL) {
		H26XEncTnrCfg stCfg = {0};
		_VdoEnc_builtin_3DNR_Internal(pathID, (UINT32)&stCfg);
		h26XEnc_setTnrCfg(&enc_var[pathID], &stCfg);
	}

	// set encode param to H264ENC_INFO
	if (yuv_info.enable) {
		UINT32 uiBsOutBufSize = p_obj->venc_param.bs_end_addr - p_obj->venc_param.bs_addr_1;

		info_avc_obj[pathID].ePicType = h264Enc_getNxtPicType(&enc_var[pathID]);
		info_avc_obj[pathID].bGetSeqHdrEn = IS_ISLICE(info_avc_obj[pathID].ePicType);
		is_keyfrm = IS_ISLICE(info_avc_obj[pathID].ePicType);

		if (uiBsOutBufSize < (IS_ISLICE(info_avc_obj[pathID].ePicType) ? p_obj->venc_param.bs_min_i_size : p_obj->venc_param.bs_min_p_size)) {
			//DBG_DUMP("Bs Buffer Full, Not Encode Further\r\n");
			if (yuv_info.release_flag) {
				if (yuv_info.enable) {
					DBG_IND("%s_2 0x%08x\r\n",__func__,yuv_info.y_addr);
					nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
				}
				yuv_unlock_cnt[pathID]++;
				if (yuv_unlock_cnt[pathID] == 2) {
					builtin_stop_encode[pathID] = TRUE;
					set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
				}
			}
			return;
		}

		// reserve size for nalu info
		if (uiBsOutBufSize > _vdoenc_builtin_nalu_size_info_max_size()) {
			uiBsOutBufSize -= _vdoenc_builtin_nalu_size_info_max_size();
		} else {
			DBG_WRN("[VDOENC_BUILTIN][%d] nalu size not enough, drop frame\r\n", pathID);
			if (yuv_info.release_flag) { 
				nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
				yuv_unlock_cnt[pathID]++;
				if (yuv_unlock_cnt[pathID] == 2) {
					builtin_stop_encode[pathID] = TRUE;
					set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
				}
			}
			return;
		}

		if (builtin_stop_encode[pathID]) {
			return;
		}

		info_avc_obj[pathID].uiSrcYAddr = yuv_info.y_addr; //0: y addr, 1: uv addr
		info_avc_obj[pathID].uiSrcCAddr = yuv_info.c_addr; //0: y addr, 1: uv addr
		info_avc_obj[pathID].uiSrcYLineOffset = yuv_info.y_line_offset; //0: y addr, 1: uv addr;
		info_avc_obj[pathID].uiSrcCLineOffset = yuv_info.c_line_offset;
		info_avc_obj[pathID].bSrcOutEn = 0;
		info_avc_obj[pathID].SdeCfg.bEnable = 0;
		info_avc_obj[pathID].uiBsOutBufAddr = p_obj->venc_param.bs_addr_1;
		info_avc_obj[pathID].uiBsOutBufSize = uiBsOutBufSize;
//		info_obj.uiNalLenOutAddr = nalu_len_addr; //maybe no need to set

#if LVGL_BUILTIN_STAMP && (LVGL_BUILTIN_STAMP_TYPE == LVGL_BUILTIN_STAMP_TYPE_OSG)

		if (builtin_osg_setup_h26x_stamp(&enc_var[pathID], pathID)){
			DBG_ERR("fail to setup h26x stamp for encoding path(%u)\n", pathID);
			return;
		}

#endif

		// encode picture
		h264Enc_prepareOnePicture(&info_avc_obj[pathID], &enc_var[pathID]);

		{

			h26x_lock();
			h26x_enableClk();

			if (H26X_ENC_MODE)
				h26x_setEncDirectRegSet(h26xEnc_getVaAPBAddr(&enc_var[pathID]));
			else
				h26x_setEncLLRegSet(1, h26x_getPhyAddr(h26xEnc_getVaLLCAddr(&enc_var[pathID])));

			h26x_start();

			interrupt = h26x_waitINT();

			if ((interrupt & h26x_getIntEn()) != 0x00000001)  {
				DBG_ERR("encode error interrupt(0x%08x)\r\n", (int)interrupt);
			}

			h264Enc_getResult(&enc_var[pathID], H26X_ENC_MODE, &sResult, interrupt, 0, &info_avc_obj[pathID], 0);

			h26x_disableClk();
			h26x_unlock();

			if (yuv_info.release_flag) {
				if (yuv_info.enable) {
					DBG_IND("%s_3 0x%08x\r\n",__func__,yuv_info.y_addr);
					nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
				}
				yuv_unlock_cnt[pathID]++;
			}

#if 0
			{
				UINT8 *ptr;
				ptr = (UINT8 *)p_obj->venc_param.bs_addr_1;
				DBG_DUMP("[%d] BSAddr 0x%x, BSSize 0x%x\r\n", pathID, p_obj->venc_param.bs_addr_1, sResult.uiBSLen);
				DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
					*(ptr+0), *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7));
				DBG_DUMP("========================\r\n");
			}
#endif
			p_obj->venc_param.interrupt = interrupt;
			p_obj->venc_param.timestamp = yuv_info.timestamp;
			p_obj->venc_param.bs_size_1 = sResult.uiBSLen;

			// nalu len
			h26xEnc_getNaluLenResult(&enc_var[pathID], &nalu_len_rslt);

			avc_nalu_size_addr = (UINT32 *)vmalloc(sizeof(UINT32)*nalu_len_rslt.uiSliceNum);
			memcpy((void *)avc_nalu_size_addr, (void *)nalu_len_rslt.uiVaAddr, sizeof(UINT32)*nalu_len_rslt.uiSliceNum);

			p_obj->venc_param.nalu_num       = nalu_len_rslt.uiSliceNum;
			p_obj->venc_param.nalu_size_addr = (UINT32)avc_nalu_size_addr;
		}

		VdoEnc_Builtin_PutBS(pathID, is_keyfrm, &(p_obj->venc_param), &sResult);
		p_obj->venc_param.bs_addr_1 = ALIGN_CEIL_64(p_obj->venc_param.bs_addr_1 + p_obj->venc_param.bs_size_1);
		{
			UINT32 idx = 0;
			UINT32 nalu_num = p_obj->venc_param.nalu_num;
			UINT32 total_size = 0;

			total_size += sizeof(UINT32);
			for (idx=0; idx<nalu_num; idx++) {
				// nalu addr
				total_size += sizeof(UINT32);
				// nalu size
				total_size += sizeof(UINT32);
			}
			total_size = ALIGN_CEIL_64(total_size);
			// update BsNow
			p_obj->venc_param.bs_addr_1 += total_size;
		}

		if (yuv_unlock_cnt[pathID] == 2) {
			builtin_stop_encode[pathID] = TRUE;
			set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
		}
	}

	if (!yuv_info.enable && yuv_info.release_flag) {
		yuv_unlock_cnt[pathID]++;
		if (yuv_unlock_cnt[pathID] == 2) {
			builtin_stop_encode[pathID] = TRUE;
			set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
		}
	}

	return;
}

int H265Enc_builtin_init(UINT32 pathID, VDOENC_BUILTIN_INIT_INFO *p_info)
{
	UINT32 width = 0, height = 0, rotate= 0;
	UINT32 svc_layer = 0, ltr_interval = 0, quality_lv = 0, d2d = 0, gdc = 0, colmv = 0;
	unsigned int venc_ctrl_0[2] = {0};

	/* H264 codec buffer */
	H26XEncMeminfo memInfo = {0};
	UINT32 codec_mem_addr = 0, codec_mem_size = 0;
	UINT32 sizePerSec = 0, uiMinIRatio = 150, uiMinPRatio = 100;
	UINT32 chip_id = nvt_get_chip_id();
	UINT32 max_width_without_tile = 0;
	UINT32 srcout_size = 0;

	VDOENC_BUILTIN_OBJ *p_obj = &gVdoEncBuiltinObj[pathID];

	memset(&gVdoEncBuiltinObj[pathID], 0, sizeof(VDOENC_BUILTIN_OBJ));
	memset(&enc_var[pathID], 0, sizeof(H26XENC_VAR));
	memset(&init_hevc_obj[pathID], 0, sizeof(H265ENC_INIT));
	memset(&info_hevc_obj[pathID], 0, sizeof(H265ENC_INFO));

	orig_yuv_width[pathID] = p_info[pathID].width;
	orig_yuv_height[pathID] = p_info[pathID].height;
	rotate = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_H26X_ROTATE);
	if (rotate == 0 || rotate == 3) {
		width = p_info[pathID].width;
		height = p_info[pathID].height;
	} else {
		width = p_info[pathID].height;
		height = p_info[pathID].width;
	}
	venc_ctrl_0[0] = p_info[pathID].max_blk_addr;
	venc_ctrl_0[1] = p_info[pathID].max_blk_size;
	if (width == 0 || height == 0) {
		DBG_ERR("error size width %d, height %d\r\n", (int)width, (int)height);
		return -1;
	}

	if (chip_id == CHIP_NA51055) {
		max_width_without_tile = 2048;
	} else if (chip_id == CHIP_NA51084) {
		max_width_without_tile = 2176;
	}

	if (max_width_without_tile == 0) {
		DBG_ERR("error chip id 0x%x\r\n", chip_id);
		return -1;
	}

	svc_layer = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SVC_LAYER);
	ltr_interval = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_LTR_INTERVAL);
	quality_lv = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_QUALITY_LV);
	d2d = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_D2D);
	gdc = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_GDC);
	colmv = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_COLMV);

	// get H265 codec buffer size
	{
		memInfo.uiWidth = width;
		memInfo.uiHeight = height;
		memInfo.ucSVCLayer = svc_layer;
		memInfo.uiLTRInterval = ltr_interval;
		memInfo.ucQualityLevel = quality_lv;
		memInfo.bTileEn= 0;
		memInfo.bD2dEn= d2d;
		memInfo.bGdcEn= gdc;
		memInfo.bColMvEn = colmv;
		memInfo.bCommReconFrmBuf = 0;
		codec_mem_addr = venc_ctrl_0[0];
		codec_mem_size = h265Enc_queryMemSize(&memInfo);

		if (codec_mem_size == 0) {
			DBG_ERR("H265Enc query error MemSize 0\r\n");
			return -1;
		}
	}

	p_obj->venc_param.codec_mem_addr = codec_mem_addr;
	p_obj->venc_param.codec_mem_size = codec_mem_size;
	p_obj->venc_param.bs_start_addr = p_obj->venc_param.codec_mem_addr + ALIGN_CEIL_4(p_obj->venc_param.codec_mem_size);

	srcout_size = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SRCOUT_SIZE);

	p_obj->venc_param.bs_end_addr = venc_ctrl_0[0] + venc_ctrl_0[1] - VDOENC_MD_MAP_MAX_SIZE - VDOENC_BUF_RESERVED_BYTES - srcout_size;
	gBSStart[pathID] = p_obj->venc_param.bs_start_addr;
	gBSEnd[pathID] = p_obj->venc_param.bs_end_addr;
	p_obj->venc_param.bs_addr_1 = p_obj->venc_param.bs_start_addr;
	p_obj->venc_param.bs_size_1 = p_obj->venc_param.bs_end_addr - p_obj->venc_param.bs_start_addr;
	sizePerSec = _VdoEnc_builtin_GetBytesPerSecond(width, height);
	p_obj->venc_param.bs_min_i_size = (UINT32)div_u64((UINT64)sizePerSec * (UINT64)uiMinIRatio, 100);
	p_obj->venc_param.bs_min_p_size = (UINT32)div_u64((UINT64)sizePerSec * (UINT64)uiMinPRatio, 100);

	DBG_IND("codec_addr 0x%x, codec_size 0x%x, bs_start_addr 0x%x, bs_end_addr 0x%x, bs_addr_1 0x%x, bs_size_1 0x%x\r\n",
			(unsigned int)p_obj->venc_param.codec_mem_addr,
			(unsigned int)p_obj->venc_param.codec_mem_size,
			(unsigned int)p_obj->venc_param.bs_start_addr,
			(unsigned int)p_obj->venc_param.bs_end_addr,
			(unsigned int)p_obj->venc_param.bs_addr_1,
			(unsigned int)p_obj->venc_param.bs_size_1);

	// Init encode engine
	{
		init_hevc_obj[pathID].uiDisplayWidth = width;
		init_hevc_obj[pathID].uiWidth  = ALIGN_CEIL_16(init_hevc_obj[pathID].uiDisplayWidth);
		init_hevc_obj[pathID].uiHeight = height;

		init_hevc_obj[pathID].bTileEn = 0;
		init_hevc_obj[pathID].eQualityLvl = quality_lv;
		init_hevc_obj[pathID].uiEncBufAddr = codec_mem_addr;
		init_hevc_obj[pathID].uiEncBufSize = codec_mem_size;
		init_hevc_obj[pathID].uiGopNum = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_GOP);
		init_hevc_obj[pathID].uiUsrQpSize = 0;
		init_hevc_obj[pathID].ucDisableDB = 0;
		init_hevc_obj[pathID].cDBAlpha = 0;
		init_hevc_obj[pathID].cDBBeta = 0;
		init_hevc_obj[pathID].iQpCbOffset = 0;
		init_hevc_obj[pathID].iQpCrOffset= 0;
		init_hevc_obj[pathID].ucSVCLayer = svc_layer;
		init_hevc_obj[pathID].uiLTRInterval = ltr_interval;
		init_hevc_obj[pathID].uiLTRPreRef = 0;
		init_hevc_obj[pathID].uiSAO = 1;
		init_hevc_obj[pathID].uiSaoLumaFlag = 1;
		init_hevc_obj[pathID].uiSaoChromaFlag = 1;
		init_hevc_obj[pathID].uiBitRate = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_BYTE_RATE)*8;

		gFrameRate[pathID] = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_FRAMERATE);
		gSec[pathID] = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_SEC);
		init_hevc_obj[pathID].uiFrmRate = gFrameRate[pathID] * 1000;
		bsque_max[pathID] = gFrameRate[pathID] * gSec[pathID];
		if (!VdoEnc_Builtin_AllocQueMem(pathID, bsque_max[pathID])){
			DBG_ERR("VdoEnc Builtin AllocQueMem fail !!!\r\n");
			return -1;
		}

		init_hevc_obj[pathID].ucIQP = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_IQP);
		init_hevc_obj[pathID].ucPQP = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_INIT_PQP);
		init_hevc_obj[pathID].ucMaxIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_IQP);
		init_hevc_obj[pathID].ucMinIQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_IQP);
		init_hevc_obj[pathID].ucMaxPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MAX_PQP);
		init_hevc_obj[pathID].ucMinPQp = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_MIN_PQP);
		if (VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_RC_MODE) == BUILTIN_VDOENC_RC_FIXQP) {
			init_hevc_obj[pathID].iIPQPoffset = 0;
			init_hevc_obj[pathID].uiStaticTime = 0;
		} else {
			init_hevc_obj[pathID].iIPQPoffset = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_IP_WEIGHT);
			init_hevc_obj[pathID].uiStaticTime = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_STATIC_TIME);
		}

		init_hevc_obj[pathID].bFBCEn = 0;
		init_hevc_obj[pathID].bGrayEn = 0;
		init_hevc_obj[pathID].bFastSearchEn = 0;//pinfo->fast_search;	// nt98520 not support search range set to 1 : big range //
		init_hevc_obj[pathID].bHwPaddingEn = 1;
		init_hevc_obj[pathID].ucRotate = rotate;
		init_hevc_obj[pathID].bD2dEn = d2d;
		init_hevc_obj[pathID].bGdcEn = gdc;
		init_hevc_obj[pathID].bColMvEn = colmv;
		init_hevc_obj[pathID].uiMultiTLayer = 0;
		if (init_hevc_obj[pathID].uiWidth > 1920 && init_hevc_obj[pathID].uiHeight > 1080) {
			init_hevc_obj[pathID].ucLevelIdc = 150;
		} else {
			init_hevc_obj[pathID].ucLevelIdc = 123;
		}
		init_hevc_obj[pathID].bVUIEn = 0;
		init_hevc_obj[pathID].usSarWidth = width;
		init_hevc_obj[pathID].usSarHeight = height;
		init_hevc_obj[pathID].ucMatrixCoef = 2;
		init_hevc_obj[pathID].ucTransferCharacteristics = 2;
		init_hevc_obj[pathID].ucColourPrimaries = 2;
		init_hevc_obj[pathID].ucVideoFormat = 5;
		init_hevc_obj[pathID].ucColorRange = 0;
		init_hevc_obj[pathID].bTimeingPresentFlag = 0;
		init_hevc_obj[pathID].bRecBufComm = 0;
		init_hevc_obj[pathID].uiRecBufAddr[0] = 0;
		init_hevc_obj[pathID].uiRecBufAddr[1] = 0;
		init_hevc_obj[pathID].uiRecBufAddr[2] = 0;

		h265Enc_initEncoder(&init_hevc_obj[pathID], &enc_var[pathID]);
		{
			UINT32 desc_addr = 0;
			UINT32 desc_size = 0;
			UINT8 *ptr;

			h265Enc_getSeqHdr(&enc_var[pathID], &desc_addr, &desc_size);

			ptr = (UINT8 *)desc_addr;
			if (*(ptr+0) != 0x00 || *(ptr+1) != 0x00 || *(ptr+2) != 0x00 || *(ptr+3) != 0x01 || *(ptr+4) != 0x40) {
				DBG_ERR("H265 error descriptor\r\n");
				return -1;
			}
		}
	}

	return 0;
}

void H265Enc_builtin_encodeOne(UINT32 pathID)
{
	UINT32 width = 0, height = 0;
	UINT32 interrupt = 0;
	BOOL is_keyfrm = 0;
	H26XEncResultCfg sResult = {0};
	H26XEncNaluLenResult nalu_len_rslt = {0};
	VDOENC_BUILTIN_OBJ *p_obj = &gVdoEncBuiltinObj[pathID];
	VDOENC_BUILTIN_YUV_INFO yuv_info = {0};

	if (_VdoEnc_Builtin_HowManyInYUVQ(pathID) <= 0) {
		return;
	}

	_VdoEnc_Builtin_GetYuv(pathID, &yuv_info);

	if (p_obj->bsQueue.bFull) {
		//DBG_DUMP("Bs Queue Full, Not Encode Further\r\n");
		if (yuv_info.release_flag) {
			if (yuv_info.enable) {
				DBG_IND("%s_1 0x%08x\r\n",__func__,yuv_info.y_addr);
				nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
			}
			yuv_unlock_cnt[pathID]++;
			if (yuv_unlock_cnt[pathID] == 2) {
				builtin_stop_encode[pathID] = TRUE;
				set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
			}
		}
		return;
	}

	width = yuv_info.width;
	height = yuv_info.height;

	if (vdoenc_builtin_get_sie_fp != NULL) {
		H26XEncTnrCfg stCfg = {0};
		_VdoEnc_builtin_3DNR_Internal(pathID, (UINT32)&stCfg);
		h26XEnc_setTnrCfg(&enc_var[pathID], &stCfg);
	}

	// set encode param to H265ENC_INFO
	if (yuv_info.enable) {
		UINT32 uiBsOutBufSize = p_obj->venc_param.bs_end_addr - p_obj->venc_param.bs_addr_1;

		info_hevc_obj[pathID].ePicType = h265Enc_getNxtPicType(&enc_var[pathID]);
		info_hevc_obj[pathID].bGetSeqHdrEn = IS_ISLICE(info_hevc_obj[pathID].ePicType);
		is_keyfrm = IS_ISLICE(info_hevc_obj[pathID].ePicType);

		if (uiBsOutBufSize < (IS_ISLICE(info_hevc_obj[pathID].ePicType) ? p_obj->venc_param.bs_min_i_size : p_obj->venc_param.bs_min_p_size)) {
			//DBG_DUMP("Bs Buffer Full, Not Encode Further\r\n");
			if (yuv_info.release_flag) {
				if (yuv_info.enable) {
					DBG_IND("%s_2 0x%08x\r\n",__func__,yuv_info.y_addr);
					nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
				}
				yuv_unlock_cnt[pathID]++;
				if (yuv_unlock_cnt[pathID] == 2) {
					builtin_stop_encode[pathID] = TRUE;
					set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
				}
			}
			return;
		}

		// reserve size for nalu info
		if (uiBsOutBufSize > _vdoenc_builtin_nalu_size_info_max_size()) {
			uiBsOutBufSize -= _vdoenc_builtin_nalu_size_info_max_size();
		} else {
			DBG_WRN("[VDOENC_BUILTIN][%d] nalu size not enough, drop frame\r\n", pathID);
			if (yuv_info.release_flag) { 
				nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
				yuv_unlock_cnt[pathID]++;
				if (yuv_unlock_cnt[pathID] == 2) {
					builtin_stop_encode[pathID] = TRUE;
					set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
				}
			}
			return;
		}

		if (builtin_stop_encode[pathID]) {
			return;
		}

		info_hevc_obj[pathID].uiSrcYAddr = yuv_info.y_addr; //0: y addr, 1: uv addr
		info_hevc_obj[pathID].uiSrcCAddr = yuv_info.c_addr; //0: y addr, 1: uv addr
		info_hevc_obj[pathID].uiSrcYLineOffset = yuv_info.y_line_offset; //0: y addr, 1: uv addr;
		info_hevc_obj[pathID].uiSrcCLineOffset = yuv_info.c_line_offset;
		info_hevc_obj[pathID].bSrcOutEn = 0;
		info_hevc_obj[pathID].SdeCfg.bEnable = 0;
		info_hevc_obj[pathID].uiBsOutBufAddr = p_obj->venc_param.bs_addr_1;
		info_hevc_obj[pathID].uiBsOutBufSize = uiBsOutBufSize;
//		info_obj.uiNalLenOutAddr = nalu_len_addr; //maybe no need to set

#if LVGL_BUILTIN_STAMP && (LVGL_BUILTIN_STAMP_TYPE == LVGL_BUILTIN_STAMP_TYPE_OSG)

		if (builtin_osg_setup_h26x_stamp(&enc_var[pathID], pathID)){
			DBG_ERR("fail to setup h26x stamp for encoding path(%u)\n", pathID);
			return;
		}

#endif

		// encode picture
		h265Enc_prepareOnePicture(&info_hevc_obj[pathID], &enc_var[pathID]);

		{

			h26x_lock();
			h26x_enableClk();

			if (H26X_ENC_MODE)
				h26x_setEncDirectRegSet(h26xEnc_getVaAPBAddr(&enc_var[pathID]));
			else
				h26x_setEncLLRegSet(1, h26x_getPhyAddr(h26xEnc_getVaLLCAddr(&enc_var[pathID])));

			h26x_start();

			interrupt = h26x_waitINT();

			if ((interrupt & h26x_getIntEn()) != 0x00000001)  {
				DBG_ERR("encode error interrupt(0x%08x)\r\n", (int)interrupt);
			}

			h265Enc_getResult(&enc_var[pathID], H26X_ENC_MODE, &sResult, interrupt, 0, &info_hevc_obj[pathID], 0);


			h26x_disableClk();
			h26x_unlock();

			if (yuv_info.release_flag) {
				if (yuv_info.enable) {
					DBG_IND("%s_3 0x%08x\r\n",__func__,yuv_info.y_addr);
					nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
				}
				yuv_unlock_cnt[pathID]++;
			}
#if 0
			{
				UINT8 *ptr;
				ptr = (UINT8 *)p_obj->venc_param.bs_addr_1;
				DBG_DUMP("[%d] BSAddr 0x%x, BSSize 0x%x\r\n", pathID, p_obj->venc_param.bs_addr_1, sResult.uiBSLen);
				DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
					*(ptr+0), *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7));
				DBG_DUMP("========================\r\n");
			}
#endif
			p_obj->venc_param.interrupt = interrupt;
			p_obj->venc_param.timestamp = yuv_info.timestamp;
			p_obj->venc_param.bs_size_1 = sResult.uiBSLen;
			// nalu len
			h26xEnc_getNaluLenResult(&enc_var[pathID], &nalu_len_rslt);

			hevc_nalu_size_addr = (UINT32 *)vmalloc(sizeof(UINT32)*nalu_len_rslt.uiSliceNum);
			memcpy((void *)hevc_nalu_size_addr, (void *)nalu_len_rslt.uiVaAddr, sizeof(UINT32)*nalu_len_rslt.uiSliceNum);

			p_obj->venc_param.nalu_num		 = nalu_len_rslt.uiSliceNum;
			p_obj->venc_param.nalu_size_addr = (UINT32)hevc_nalu_size_addr;
		}

		VdoEnc_Builtin_PutBS(pathID, is_keyfrm, &(p_obj->venc_param), &sResult);
		p_obj->venc_param.bs_addr_1 = ALIGN_CEIL_64(p_obj->venc_param.bs_addr_1 + p_obj->venc_param.bs_size_1);
		{
			UINT32 idx = 0;
			UINT32 nalu_num = p_obj->venc_param.nalu_num;
			UINT32 total_size = 0;

			total_size += sizeof(UINT32);
			for (idx=0; idx<nalu_num; idx++) {
				// nalu addr
				total_size += sizeof(UINT32);
				// nalu size
				total_size += sizeof(UINT32);
			}
			total_size = ALIGN_CEIL_64(total_size);
			// update BsNow
			p_obj->venc_param.bs_addr_1 += total_size;
		}

		if (yuv_unlock_cnt[pathID] == 2) {
			builtin_stop_encode[pathID] = TRUE;
			set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
		}
	}

	if (!yuv_info.enable && yuv_info.release_flag) {
		yuv_unlock_cnt[pathID]++;
		if (yuv_unlock_cnt[pathID] == 2) {
			builtin_stop_encode[pathID] = TRUE;
			set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
		}
	}

	return;
}

int MJPGEnc_builtin_init(UINT32 pathID, VDOENC_BUILTIN_INIT_INFO *p_info)
{
	UINT32 width = 0, height = 0;
	unsigned int venc_ctrl_0[2] = {0};
	UINT32 sizePerSec = 0, uiMinIRatio = 150;
	VDOENC_BUILTIN_OBJ *p_obj = &gVdoEncBuiltinObj[pathID];

	memset(&gVdoEncBuiltinObj[pathID], 0, sizeof(VDOENC_BUILTIN_OBJ));

	jpeg_width[pathID] = p_info[pathID].width;
	jpeg_height[pathID] = p_info[pathID].height;
	venc_ctrl_0[0] = p_info[pathID].max_blk_addr;
	venc_ctrl_0[1] = p_info[pathID].max_blk_size;
	jpeg_max_mem_size[pathID] =p_info[pathID].max_blk_size;
	jpeg_quality[pathID] = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_JPEG_QUALITY);
	jpeg_fps[pathID] = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_JPEG_FPS);
	jpg_enc_setdata(pathID, JPG_TARGET_RATE, 0);
	jpg_enc_setdata(pathID, JPG_VBR_QUALITY, jpeg_quality[pathID]);
	bsque_max[pathID] = 90; // 30fps * 3s

	if (!VdoEnc_Builtin_AllocQueMem(pathID, bsque_max[pathID])){
		DBG_ERR("VdoEnc Builtin AllocQueMem fail !!!\r\n");
		return -1;
	}

	p_obj->venc_param.bs_start_addr = venc_ctrl_0[0];//(UINT32)vmalloc(3200);
	p_obj->venc_param.bs_end_addr = p_obj->venc_param.bs_start_addr + venc_ctrl_0[1];//3200;
	p_obj->venc_param.bs_addr_1 = p_obj->venc_param.bs_start_addr;
	p_obj->venc_param.bs_size_1 = p_obj->venc_param.bs_end_addr - p_obj->venc_param.bs_start_addr;
	sizePerSec = _VdoEnc_builtin_GetBytesPerSecond(width, height);
	p_obj->venc_param.bs_min_i_size = (UINT32)div_u64((UINT64)sizePerSec * (UINT64)uiMinIRatio, 100);

	return 0;
}

void MJPGEnc_builtin_encodeOne(UINT32 pathID)
{
	UINT32 width = 0, height = 0;
	UINT32 jpeg_fps = 0, bs_count = 0, framerate = 0;
	BOOL is_keyfrm = 0;
	H26XEncResultCfg sResult = {0};
	KDRV_VDOENC_PARAM jpgenc_param = {0};
	VDOENC_BUILTIN_YUV_INFO yuv_info = {0};
	VDOENC_BUILTIN_OBJ *p_obj = &gVdoEncBuiltinObj[pathID];

	if (_VdoEnc_Builtin_HowManyInYUVQ(pathID) <= 0) {
		return;
	}

	_VdoEnc_Builtin_GetYuv(pathID, &yuv_info);

	if (p_obj->bsQueue.bFull) {
		//DBG_DUMP("Bs Queue Full, Not Encode Further\r\n");
		if (yuv_info.release_flag) {
			if (yuv_info.enable) {
				DBG_IND("%s_1 0x%08x\r\n",__func__,yuv_info.y_addr);
				nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
			}
			yuv_unlock_cnt[pathID]++;
			if (yuv_unlock_cnt[pathID] == 2) {
				builtin_stop_encode[pathID] = TRUE;
				set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
			}
		}
		return;
	}

	if (p_obj->venc_param.bs_end_addr - p_obj->venc_param.bs_addr_1 < 0x64000/*p_obj->venc_param.bs_min_i_size*/) {
		//DBG_DUMP("Bs Buffer Full, Not Encode Further\r\n");
		if (yuv_info.release_flag) {
			if (yuv_info.enable) {
				DBG_IND("%s_2 0x%08x\r\n",__func__,yuv_info.y_addr);
				nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
			}
			yuv_unlock_cnt[pathID]++;
			if (yuv_unlock_cnt[pathID] == 2) {
				builtin_stop_encode[pathID] = TRUE;
				set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
			}
		}
		return;
	}

	if (builtin_stop_encode[pathID]) {
		return;
	}

	width = yuv_info.width;
	height = yuv_info.height;

	{
		if (gJpegEncIdx[pathID] == gThisFrmIdx[pathID]) {
			jpgenc_param.y_addr = yuv_info.y_addr;
			jpgenc_param.c_addr = yuv_info.c_addr;
			jpgenc_param.y_line_offset = yuv_info.y_line_offset;
			jpgenc_param.c_line_offset = yuv_info.c_line_offset;
			jpgenc_param.bs_addr_1 = p_obj->venc_param.bs_addr_1;
			jpgenc_param.bs_size_1 = p_obj->venc_param.bs_end_addr - p_obj->venc_param.bs_addr_1;
			jpgenc_param.quality = jpeg_quality[pathID];
			jpgenc_param.encode_width = width;
			jpgenc_param.encode_height = height;
			jpgenc_param.in_fmt = 1;

			DBG_IND("y_addr 0x%x, c_addr 0x%x, ylof %u, clof %u, bs_addr_1 0x%x, bs_size_1 0x%x, quality %u, w %u, h %u\r\n",
				jpgenc_param.y_addr,
				jpgenc_param.c_addr,
				jpgenc_param.y_line_offset,
				jpgenc_param.c_line_offset,
				jpgenc_param.bs_addr_1,
				jpgenc_param.bs_size_1,
				jpgenc_param.quality,
				jpgenc_param.encode_width,
				jpgenc_param.encode_height);
			jpeg_add_queue(pathID ,JPEG_CODEC_MODE_ENC,(void *)(&jpgenc_param), 0, 0);

			p_obj->venc_param.bs_size_1 = jpgenc_param.bs_size_1;
			p_obj->venc_param.base_qp = jpgenc_param.base_qp;
			p_obj->venc_param.frm_type = jpgenc_param.frm_type;
			p_obj->venc_param.timestamp = yuv_info.timestamp;
			VdoEnc_Builtin_PutBS(pathID, is_keyfrm, &(p_obj->venc_param), &sResult);
			p_obj->venc_param.bs_addr_1 = ALIGN_CEIL_64(p_obj->venc_param.bs_addr_1 + p_obj->venc_param.bs_size_1);
			{
				UINT32 total_size = 0;

				// nalu_num
				total_size += sizeof(UINT32);
				// nalu_addr
				total_size += sizeof(UINT32);
				// nalu_size
				total_size += sizeof(UINT32);

				total_size = ALIGN_CEIL_64(total_size);

				// update BsNow
				p_obj->venc_param.bs_addr_1 += total_size;
			}

			jpeg_fps = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_JPEG_FPS);
			bs_count = VdoEnc_Builtin_HowManyInBSQ(pathID);
//			if (VdoEnc_builtin_get_dtsi_param(BUILTIN_VDOENC_DTSI_PARAM_JPEG_YUV_SRC_PATH) == 0) {
//				framerate = VdoEnc_builtin_get_dtsi_param(BUILTIN_VDOENC_DTSI_PARAM_MAIN_FRAMERATE);
//			} else {
//				framerate = VdoEnc_builtin_get_dtsi_param(BUILTIN_VDOENC_DTSI_PARAM_SUB_FRAMERATE);
//			}
//			if (framerate == 0 && (bMainEn == 0 || bSubEn == 0)) {
				framerate = 30;
//			}
			if (jpeg_fps == 0 || framerate == 0) {
				DBG_ERR("Error !! jpeg_fps %u, framerate %u\r\n", jpeg_fps, framerate);
				return;
			}
			gJpegEncIdx[pathID] = ((((1000 / jpeg_fps) * bs_count * 100) / (100000 / framerate)) * 100 + 50) / 100;
		}

		gThisFrmIdx[pathID]++;
	}

	if (yuv_info.release_flag) {
		if (yuv_info.enable) {
			DBG_IND("%s_3 0x%08x\r\n",__func__,yuv_info.y_addr);
			nvtmpp_unlock_fastboot_blk(yuv_info.y_addr);
		}
		yuv_unlock_cnt[pathID]++;
		if (yuv_unlock_cnt[pathID] == 2) {
			builtin_stop_encode[pathID] = TRUE;
			set_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
		}
	}

	return;
}

void VdoEnc_BuiltIn_trig(VDOENC_BUILTIN_FMD_INFO *p_info, UINT32 reserved)
{
	VDOENC_BUILTIN_YUV_INFO yuv_info = {0};
	UINT32 ipp_dev = 0; // get p_info->name
	UINT32 vprc_src_dev = 0, vprc_src_path = 0; //venc dtsi
	UINT32 i = 0;

	if (strcmp(p_info->name, "vdoprc0") == 0) {
		ipp_dev = 0;
	} else if (strcmp(p_info->name, "vdoprc1") == 0) {
		ipp_dev = 1;
	}

	for (i = 0; i < BUILTIN_VDOENC_PATH_ID_MAX; i++) {
		if (gVdoEnc_en[i]) {
			vprc_src_dev = VdoEnc_builtin_get_dtsi_param(i, BUILTIN_VDOENC_DTSI_PARAM_SRC_VPRC_DEV);
			vprc_src_path = VdoEnc_builtin_get_dtsi_param(i, BUILTIN_VDOENC_DTSI_PARAM_SRC_VPRC_PATH);
			if (ipp_dev == vprc_src_dev) {
				_VdoEnc_builtin_Get_SrcYuv_Info(p_info, &yuv_info, vprc_src_path);
				_VdoEnc_Builtin_PutYuv(i, &yuv_info);
				if (gVdoEnc_CodecType[i] == BUILTIN_VDOENC_MJPEG) {
					_VdoEnc_builtin_PutJob_JPEG(i);
					set_flg(FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_ENCODE);
				} else {
					_VdoEnc_builtin_PutJob_H26X(i);
					iset_flg(FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_ENCODE);

#if LVGL_BUILTIN_STAMP
					iset_flg(FLG_ID_LVGL_BUILTIN, FLG_LVGL_BUILTIN_FONT_CONV);
#endif
				}
			}
		}
	}

	return;
}

THREAD_DECLARE(VdoEnc_BuiltIn_Tsk_H26X, arglist) //static void VdoEnc_BuiltIn_Tsk_H26X(void)
{
	FLGPTN			uiFlag = 0;
	UINT32			pathID = 0;
	UINT32			codectype = 0;
	DBG_DUMP("[VDOENC_BUILTIN] VdoEnc_BuiltIn_Tsk_H26X() start\r\n");

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_ENCODE | FLG_VDOENC_BUILTIN_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_IDLE);

		if (uiFlag & FLG_VDOENC_BUILTIN_STOP) {
			break;
		}

		if (uiFlag & FLG_VDOENC_BUILTIN_ENCODE) {
			if (_VdoEnc_builtin_GetJobCount_H26X() > 0) {
				_VdoEnc_builtin_GetJob_H26X(&pathID);
				codectype = VdoEnc_builtin_get_dtsi_param(pathID, BUILTIN_VDOENC_DTSI_PARAM_CODECTYPE);
				clr_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
				if (codectype == BUILTIN_VDOENC_H265) {
					H265Enc_builtin_encodeOne(pathID);
				} else if (codectype == BUILTIN_VDOENC_H264) {
					H264Enc_builtin_encodeOne(pathID);
				}
			}

			if (_VdoEnc_builtin_GetJobCount_H26X() > 0) { // continue to decode
				set_flg(FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_ENCODE);
			}
		}
	} // end of while loop

	set_flg(FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_STOP_DONE);

	THREAD_RETURN(0);
}

int VdoEnc_BuiltIn_TskStart_H26X(void)
{
	// start tsk
	THREAD_CREATE(VDOENC_BUILTIN_TSK_ID_H26X, VdoEnc_BuiltIn_Tsk_H26X, NULL, "VdoEnc_BuiltIn_Tsk_H26X"); //sta_tsk(VDOENC_BUILTIN_TSK_ID_H26X, 0);
	if (VDOENC_BUILTIN_TSK_ID_H26X == 0) {
		DBG_ERR("Invalid VDOENC_BUILTIN_TSK_ID_H26X\r\n");
		return -1;
	}
	THREAD_SET_PRIORITY(VDOENC_BUILTIN_TSK_ID_H26X, VDOENC_BUILTIN_TSK_PRI);
	THREAD_RESUME(VDOENC_BUILTIN_TSK_ID_H26X);
	DBG_IND("[VDOENC_BUILTIN] video encoding task start...\r\n");

	return 0;
}

int VdoEnc_BuiltIn_TskStop_H26X(void)
{
	FLGPTN uiFlag;

	if (FLG_ID_VDOENC_BUILTIN_H26X == 0) {
		return E_OK;
	}

	DBG_DUMP("[VDOENC_BUILTIN]tskstop wait idle ..\r\n");
	wai_flg(&uiFlag, FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_IDLE, TWF_ORW);
	DBG_DUMP("[VDOENC_BUILTIN]tskstop wait idle OK\r\n");

	// close decoder
	/*if (gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo) {
		(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_CLOSEBUS, 0, 0, 0);
	} else {
		DBG_ERR("MP decoder is NULL\r\n");
	}*/

//#if __KERNEL__
#if 1
	set_flg(FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_STOP);
	wai_flg(&uiFlag, FLG_ID_VDOENC_BUILTIN_H26X, FLG_VDOENC_BUILTIN_STOP_DONE, TWF_ORW | TWF_CLR);
#endif
	// Terminate Task
	//THREAD_DESTROY(NMP_VDODEC_TSK_ID); //ter_tsk(NMP_VDODEC_TSK_ID);

#if LVGL_BUILTIN_STAMP
	Lvgl_BuiltIn_TskStop();
#endif
	return E_OK;
}

THREAD_DECLARE(VdoEnc_BuiltIn_Tsk_JPEG, arglist) //static void VdoEnc_BuiltIn_Tsk_JPEG(void)
{
	FLGPTN			uiFlag = 0;
	UINT32			pathID = 0;
	DBG_DUMP("[VDOENC_BUILTIN] VdoEnc_BuiltIn_Tsk_JPEG() start\r\n");

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_ENCODE | FLG_VDOENC_BUILTIN_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_IDLE);

		if (uiFlag & FLG_VDOENC_BUILTIN_STOP) {
			break;
		}

		if (uiFlag & FLG_VDOENC_BUILTIN_ENCODE) {
			if (_VdoEnc_builtin_GetJobCount_JPEG() > 0) {
				_VdoEnc_builtin_GetJob_JPEG(&pathID);
				clr_flg(FLG_ID_VDOENC_BUILTIN_CHECK[pathID], FLG_VDOENC_BUILTIN_DONE);
				MJPGEnc_builtin_encodeOne(pathID);
			}

			if (_VdoEnc_builtin_GetJobCount_JPEG() > 0) { // continue to decode
				set_flg(FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_ENCODE);
			}
		}
	} // end of while loop

	set_flg(FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_STOP_DONE);

	THREAD_RETURN(0);
}

int VdoEnc_BuiltIn_TskStart_JPEG(void)
{
	// start tsk
	THREAD_CREATE(VDOENC_BUILTIN_TSK_ID_JPEG, VdoEnc_BuiltIn_Tsk_JPEG, NULL, "VdoEnc_BuiltIn_Tsk_JPEG"); //sta_tsk(VDOENC_BUILTIN_TSK_ID_JPEG, 0);
	if (VDOENC_BUILTIN_TSK_ID_JPEG == 0) {
		DBG_ERR("Invalid VDOENC_BUILTIN_TSK_ID_JPEG\r\n");
		return -1;
	}
	THREAD_SET_PRIORITY(VDOENC_BUILTIN_TSK_ID_JPEG, VDOENC_BUILTIN_TSK_PRI);
	THREAD_RESUME(VDOENC_BUILTIN_TSK_ID_JPEG);
	DBG_IND("[VDOENC_BUILTIN] video encoding task start...\r\n");

	return 0;
}

int VdoEnc_BuiltIn_TskStop_JPEG(void)
{
	FLGPTN uiFlag;

	if (FLG_ID_VDOENC_BUILTIN_JPEG == 0) {
		return E_OK;
	}

	DBG_DUMP("[VDOENC_BUILTIN]tskstop wait idle ..\r\n");
	wai_flg(&uiFlag, FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_IDLE, TWF_ORW);
	DBG_DUMP("[VDOENC_BUILTIN]tskstop wait idle OK\r\n");

	// close decoder
	/*if (gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo) {
		(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_CLOSEBUS, 0, 0, 0);
	} else {
		DBG_ERR("MP decoder is NULL\r\n");
	}*/

//#if __KERNEL__
#if 1
	set_flg(FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_STOP);
	wai_flg(&uiFlag, FLG_ID_VDOENC_BUILTIN_JPEG, FLG_VDOENC_BUILTIN_STOP_DONE, TWF_ORW | TWF_CLR);
#endif
	// Terminate Task
	//THREAD_DESTROY(NMP_VDODEC_TSK_ID); //ter_tsk(NMP_VDODEC_TSK_ID);

	return E_OK;
}


// install task
void VdoEnc_BuiltIn_Install_ID(void)
{
	UINT32 i = 0;

	OS_CONFIG_FLAG(FLG_ID_VDOENC_BUILTIN_H26X);
	OS_CONFIG_FLAG(FLG_ID_VDOENC_BUILTIN_JPEG);
	for (i = 0; i < BUILTIN_VDOENC_PATH_ID_MAX; i++) {
		OS_CONFIG_FLAG(FLG_ID_VDOENC_BUILTIN_CHECK[i]);
	}
	OS_CONFIG_SEMPHORE(VDOENC_BUILTIN_BS_SEM_ID, 0, 1, 1);
}

void VdoEnc_BuiltIn_Uninstall_ID(void)
{
	UINT32 i = 0;

	if (FLG_ID_VDOENC_BUILTIN_H26X) {
		rel_flg(FLG_ID_VDOENC_BUILTIN_H26X);
	}
	if (FLG_ID_VDOENC_BUILTIN_JPEG) {
		rel_flg(FLG_ID_VDOENC_BUILTIN_JPEG);
	}
	for (i = 0; i < BUILTIN_VDOENC_PATH_ID_MAX; i++) {
		if (FLG_ID_VDOENC_BUILTIN_CHECK[i]) {
			rel_flg(FLG_ID_VDOENC_BUILTIN_CHECK[i]);
		}
	}
	if (VDOENC_BUILTIN_BS_SEM_ID) {
		SEM_DESTROY(VDOENC_BUILTIN_BS_SEM_ID);
	}
}

int  VdoEnc_builtin_init(VDOENC_BUILTIN_INIT_INFO *p_info)
{
	UINT32 uiBufSize = 0;
	PVDOENC_BUILTIN_JOBQ pJobQ26X = &gVdoEncBuiltinJobQ_H26X;
	PVDOENC_BUILTIN_JOBQ pJobQJPEG = &gVdoEncBuiltinJobQ_JPEG;
	UINT32 i = 0;

#if LVGL_BUILTIN_STAMP

	Lvgl_BuiltIn_Init(
			p_info->width,
			p_info->height,
			&lv_draw_cfg,
			&lv_calc_mem_rst,
			&lv_stamp_mem_cfg
	);

	Lvgl_BuiltIn_Calc_Pos(
			p_info->width,
			p_info->height,
			&lv_calc_mem_rst,
			&lv_draw_cfg,
			&stamp_pos_x,
			&stamp_pos_y
			);


#if	LVGL_BUILTIN_STAMP_TYPE == LVGL_BUILTIN_STAMP_TYPE_OSG

	Lvgl_BuiltIn_Config_OSG(
			&lv_calc_mem_rst,
			&lv_stamp_mem_cfg,
			stamp_pos_x,
			stamp_pos_y
			);

#endif

	Lvgl_Builtin_rtc_init();

	Lvgl_BuiltIn_TskStart();

#endif

	VdoEnc_BuiltIn_Install_ID();
	VdoEnc_BuiltIn_TskStart_H26X();

	VdoEnc_BuiltIn_TskStart_JPEG();
	jpeg_open();

	uiBufSize = sizeof(VDOENC_BUILTIN_JOB_INFO) * VDOENC_BUILTIN_JOBQ_MAX;
	if (uiBufSize == 0) {
		DBG_ERR("[VDOBUILTIN] JobQue size fail\r\n");
		return -1;
	}
	pJobQ26X->Queue = (VDOENC_BUILTIN_JOB_INFO *)vmalloc(uiBufSize);
	if (pJobQ26X->Queue == NULL) {
		DBG_ERR("[VDOBUILTIN] AllocJobQueMem fail\r\n");
		return -1;
	}
	pJobQJPEG->Queue = (VDOENC_BUILTIN_JOB_INFO *)vmalloc(uiBufSize);
	if (pJobQJPEG->Queue == NULL) {
		DBG_ERR("[VDOBUILTIN] AllocJobQueMem fail\r\n");
		return -1;
	}

	for (i = 0; i < BUILTIN_VDOENC_PATH_ID_MAX; i++) {
		gVdoEnc_en[i] = VdoEnc_builtin_get_dtsi_param(i, BUILTIN_VDOENC_DTSI_PARAM_ENC_EN);
		if (gVdoEnc_en[i]) {
			gVdoEnc_CodecType[i] = VdoEnc_builtin_get_dtsi_param(i, BUILTIN_VDOENC_DTSI_PARAM_CODECTYPE);
			if (gVdoEnc_CodecType[i] == BUILTIN_VDOENC_H265) {
				H265Enc_builtin_init(i, p_info);
				VdoEnc_builtin_set_rate_control(i);
			} else if (gVdoEnc_CodecType[i] == BUILTIN_VDOENC_H264) {
				H264Enc_builtin_init(i, p_info);
				VdoEnc_builtin_set_rate_control(i);
			} else if (gVdoEnc_CodecType[i] == BUILTIN_VDOENC_MJPEG) {
				MJPGEnc_builtin_init(i, p_info);
			}
		}
	}
	
	

	kdrv_ipp_builtin_reg_fmd_cb(VdoEnc_BuiltIn_trig);

	return 0;
}

#if LVGL_BUILTIN_STAMP

lv_font_t* Lvgl_BuiltIn_Select_Font(
		UINT32 image_width,
		UINT32 image_height
)
{
	lv_font_t* font = NULL;

	if (image_width >= 3840) {
		font = &LV_USER_CFG_STAMP_FONT_XXL;
	}
	else if(image_width >=3600) {
		font = &LV_USER_CFG_STAMP_FONT_XXL;
	}
	else if(image_width >=3200) {
		font = &LV_USER_CFG_STAMP_FONT_XL;
	}
	else if(image_width >=2880) {
		font = &LV_USER_CFG_STAMP_FONT_XL;
	}
	else if(image_width >=1920) {
		font = &LV_USER_CFG_STAMP_FONT_LARGE;
	}
	else if(image_width >=1080) {
		font = &LV_USER_CFG_STAMP_FONT_MEDIUM;
	}
	else if(image_width >=640) {
		font = &LV_USER_CFG_STAMP_FONT_SMALL;
	}
	else if(image_width >=320) {
		font = &LV_USER_CFG_STAMP_FONT_SMALL;
	}
	else {
		DBG_WRN("image_width(%u) is too small!\n", image_width);
		font = &LV_USER_CFG_STAMP_FONT_SMALL;
	}

	return font;
}

void Lvgl_BuiltIn_Uninit(lv_user_font_conv_mem_cfg *mem_cfg)
{
	if(mem_cfg->working_buffer)
		vfree(mem_cfg->working_buffer);

	if(mem_cfg->output_buffer)
		vfree(mem_cfg->output_buffer);

#if	LVGL_BUILTIN_STAMP_TYPE == LVGL_BUILTIN_STAMP_TYPE_OSG
	if(osg_buffer)
		vfree((void*)osg_buffer);
#endif

	rel_flg(FLG_ID_LVGL_BUILTIN);
	SEM_DESTROY(LVGL_BUILTIN_SEM_ID);
}

void Lvgl_BuiltIn_Init(
		UINT32 image_width,
		UINT32 image_height,
		lv_user_font_conv_draw_cfg *draw_cfg,
		lv_user_font_conv_calc_buffer_size_result *mem_rst,
		lv_user_font_conv_mem_cfg *mem_cfg)
{
	static BOOL is_init = FALSE;
	struct tm datetime = {0};
	lv_color32_t color;

	if(is_init == TRUE)
		return;

	is_init = TRUE;

	lv_user_font_conv_draw_cfg_init(draw_cfg);

	sprintf(lv_datetime_string, LV_USER_FONT_CONV_DATETIME_FMT, LV_USER_FONT_CONV_DATETIME_FMT_ARG(datetime));

#if	LVGL_BUILTIN_STAMP_TYPE == LVGL_BUILTIN_STAMP_TYPE_OSG
	draw_cfg->fmt = LV_USER_FONT_CONV_FMT_ARGB4444;
#else
	draw_cfg->fmt = LV_USER_FONT_CONV_FMT_YUV420;
#endif

	draw_cfg->string.text = lv_datetime_string;
//	draw_cfg->string.font = &LV_USER_CFG_STAMP_FONT_XXL; /* allocate max font buffer */
	draw_cfg->string.font = Lvgl_BuiltIn_Select_Font(image_width, image_height);

	/* nvt-info.dtsi */
	color.full = LV_USER_CFG_STAMP_COLOR_TEXT;
	draw_cfg->string.color = LV_COLOR_MAKE(color.ch.red, color.ch.green, color.ch.blue);
	draw_cfg->string.opa = color.ch.alpha;

	color.full = LV_USER_CFG_STAMP_COLOR_BACKGROUND;
	draw_cfg->bg.color = LV_COLOR_MAKE(color.ch.red, color.ch.green, color.ch.blue);
	draw_cfg->bg.opa = color.ch.alpha;

	color.full = LV_USER_CFG_STAMP_COLOR_FRAME;
	draw_cfg->border.color = LV_COLOR_MAKE(color.ch.red, color.ch.green, color.ch.blue);
	draw_cfg->border.opa = color.ch.alpha;

	draw_cfg->align_w = LV_USER_FONT_CONV_ALIGN_W;
	draw_cfg->align_h = LV_USER_FONT_CONV_ALIGN_H;
	draw_cfg->string.letter_space = LV_USER_CFG_STAMP_LETTER_SPACE;
	draw_cfg->ext_w = LV_USER_CFG_STAMP_EXT_WIDTH;
	draw_cfg->ext_h = LV_USER_CFG_STAMP_EXT_HEIGHT;
	draw_cfg->border.width = LV_USER_CFG_STAMP_BORDER_WIDTH;
	draw_cfg->radius = LV_USER_CFG_STAMP_RADIUS;

	DBG_DUMP("[LVGL] align {%u, %u}, letter space = %u, ext {%u, %u}, border width = %u, radius = %u\n",
			draw_cfg->align_w,
			draw_cfg->align_h,
			draw_cfg->string.letter_space,
			draw_cfg->ext_w,
			draw_cfg->ext_h,
			draw_cfg->border.width,
			draw_cfg->radius
	);

	lv_user_font_conv_init(draw_cfg, mem_rst);

	mem_cfg->working_buffer = vmalloc(mem_rst->working_buffer_size);
	mem_cfg->working_buffer_size = mem_rst->working_buffer_size;

	if(mem_cfg->working_buffer == NULL){
		DBG_ERR("[LVGL] alloc working buffer failed!\n");
	}
	else{
		DBG_DUMP("[LVGL] alloc working buffer ok(%x)!\n", mem_cfg->working_buffer_size);
	}

	mem_cfg->output_buffer  = vmalloc(ALIGN_CEIL(mem_rst->output_buffer_size, 128));
	mem_cfg->output_buffer_size = mem_rst->output_buffer_size;

	if(mem_cfg->output_buffer == NULL){
		DBG_ERR("alloc output buffer failed!\n");
	}
	else{
		DBG_DUMP("[LVGL] alloc output buffer ok(%x)!\n", mem_cfg->output_buffer_size);
	}

	OS_CONFIG_SEMPHORE(LVGL_BUILTIN_SEM_ID, 0, 1, 1); /* output buffer lock */
	OS_CONFIG_FLAG(FLG_ID_LVGL_BUILTIN);
}

#if	LVGL_BUILTIN_STAMP_TYPE == LVGL_BUILTIN_STAMP_TYPE_OSG

int Lvgl_BuiltIn_Config_OSG(
		lv_user_font_conv_calc_buffer_size_result *mem_rst,
		lv_user_font_conv_mem_cfg* mem_cfg,
		UINT32 x,
		UINT32 y
)
{
	/***************************************************************
	 * OSG
	 **************************************************************/

	UINT32 size = ALIGN_CEIL(mem_cfg->output_buffer_size * 2, 128);

	osg_buffer = (uintptr_t)vmalloc(size);

	if(osg_buffer == 0){
		DBG_ERR("vmalloc(%x) failed!\n", size);
		return -1;
	}

	stamp[0].buf.type        = OSG_BUF_TYPE_PING_PONG;
	stamp[0].buf.p_addr = (uintptr_t)  osg_buffer;
	stamp[0].buf.size        = size; //should be 128 aligned
	stamp[0].buf.ddr_id      = 0;
	stamp[0].img.fmt         = OSG_PXLFMT_ARGB4444;
	stamp[0].img.dim.w       = mem_rst->width;
	stamp[0].img.dim.h       = mem_rst->height;
	stamp[0].attr.alpha      = 255;
	stamp[0].attr.position.x = x;
	stamp[0].attr.position.y = y;
	stamp[0].attr.layer      = 0;
	stamp[0].attr.region     = 0;

	osg.num = 1;
	osg.stamp = stamp;

	if(vds_set_early_osg(0, &osg)){
		DBG_ERR("vds_set_early_osg() fail\n");
	}

	return 0;
}

#endif

int Lvgl_BuiltIn_Calc_Pos(
		UINT32 img_width,
		UINT32 imag_height,
		lv_user_font_conv_calc_buffer_size_result *mem_rst,
		lv_user_font_conv_draw_cfg *draw_cfg,
		UINT32 *x,
		UINT32 *y
)
{
	if(x == NULL || y == NULL){
		DBG_ERR("x and y can'nt be null!\n");
		return -1;
	}

	UINT32 font_width = mem_rst->width / strlen(draw_cfg->string.text);

	*x = ALIGN_FLOOR(img_width - (font_width * strlen(draw_cfg->string.text)) - mem_rst->height, 2);
	*y = ALIGN_FLOOR(imag_height - mem_rst->height * 2, 2);

	return 0;
}

int Lvgl_BuiltIn_Font_Conv(
		VDOENC_BUILTIN_YUV_INFO *yuv_info,
		lv_user_font_conv_calc_buffer_size_result *mem_rst,
		lv_user_font_conv_mem_cfg *mem_cfg,
		UINT32 dst_x, UINT32 dst_y)
{
	SEM_WAIT(LVGL_BUILTIN_SEM_ID);

	if(dst_x + mem_rst->width > yuv_info->width){
		DBG_ERR("stamp pos out of range!(dst_x(%u) + width(%u) > yuv width(%u))\n", dst_x, mem_rst->width, yuv_info->width);
		return -1;
	}

	if(dst_y + mem_rst->height > yuv_info->height){
		DBG_ERR("stamp pos out of range!(dst_x(%u) + width(%u) > yuv width(%u))\n", dst_y, mem_rst->height, yuv_info->height);
		return -1;
	}

	DBG_DUMP("x = %u, y = %u\n", dst_x, dst_y);


#if LVGL_BUILTIN_STAMP_TYPE == LVGL_BUILTIN_STAMP_TYPE_OSG

	if(vds_update_early_osg(0, 0, (uintptr_t)mem_cfg->output_buffer)){
		DBG_ERR("vds_update_early_osg fail\n");
	}

#else

	UINT32 y_addr = 0, c_addr = 0;
	UINT32 ptr = 0;

	y_addr = yuv_info->y_addr + dst_x + dst_y * yuv_info->y_line_offset;
	c_addr = yuv_info->c_addr + dst_x + (dst_y / 2) * yuv_info->c_line_offset;
	ptr = (UINT32)mem_cfg->output_buffer;

	for(UINT32 i = 0 ; i < mem_rst->height ; i++)
	{
		memcpy((void*)y_addr, (void*)ptr, mem_rst->width);
		y_addr += yuv_info->y_line_offset;
		ptr +=  mem_rst->width;
	}

	vos_cpu_dcache_sync((VOS_ADDR)(yuv_info->y_addr + (dst_y * yuv_info->y_line_offset)), yuv_info->y_line_offset * mem_rst->height , VOS_DMA_TO_DEVICE_NON_ALIGN);

	ptr = (UINT32)mem_cfg->output_buffer + (mem_rst->width * mem_rst->height);
	for(UINT32 i = 0 ; i < (mem_rst->height / 2) ; i++)
	{
		memcpy((void*)c_addr, (void*)ptr, mem_rst->width);
		c_addr += yuv_info->c_line_offset;
		ptr +=  mem_rst->width;
	}

	vos_cpu_dcache_sync((VOS_ADDR)(yuv_info->c_addr + ((dst_y / 2) * yuv_info->c_line_offset)), yuv_info->c_line_offset * (mem_rst->height / 2 ), VOS_DMA_TO_DEVICE_NON_ALIGN);
#endif

	SEM_SIGNAL(LVGL_BUILTIN_SEM_ID);

	return 0;
}

THREAD_DECLARE(Lvgl_BuiltIn_Tsk, arglist)
{
	struct tm datetime_curr = {0};
	struct tm datetime_tmp = {0};
	FLGPTN			uiFlag = 0;

	DBG_DUMP("[LVGL_BUILTIN] Lvgl_BuiltIn_Tsk() start\r\n");

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(FLG_ID_LVGL_BUILTIN, FLG_LVGL_BUILTIN_IDLE);

	SEM_WAIT(LVGL_BUILTIN_SEM_ID);
	Lvgl_Builtin_rtc_read_time(&datetime_curr);
	sprintf(lv_datetime_string, LV_USER_FONT_CONV_DATETIME_FMT, LV_USER_FONT_CONV_DATETIME_FMT_ARG(datetime_curr));
	lv_user_font_conv(&lv_draw_cfg, &lv_stamp_mem_cfg);
	SEM_SIGNAL(LVGL_BUILTIN_SEM_ID);

	while(!THREAD_SHOULD_STOP)
	{
		set_flg(FLG_ID_LVGL_BUILTIN, FLG_LVGL_BUILTIN_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_LVGL_BUILTIN, FLG_LVGL_BUILTIN_FONT_CONV | FLG_LVGL_BUILTIN_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		if(uiFlag & FLG_LVGL_BUILTIN_STOP)
			break;

		if(uiFlag & FLG_LVGL_BUILTIN_FONT_CONV){

			Lvgl_Builtin_rtc_read_time(&datetime_tmp);

			if(datetime_curr.tm_sec != datetime_tmp.tm_sec ||
			   datetime_curr.tm_min != datetime_tmp.tm_min
			){
				sprintf(lv_datetime_string, LV_USER_FONT_CONV_DATETIME_FMT, LV_USER_FONT_CONV_DATETIME_FMT_ARG(datetime_tmp));

				datetime_curr = datetime_tmp;

				SEM_WAIT(LVGL_BUILTIN_SEM_ID);
				lv_user_font_conv(&lv_draw_cfg, &lv_stamp_mem_cfg);
				SEM_SIGNAL(LVGL_BUILTIN_SEM_ID);
			}
		}
	}

	set_flg(FLG_ID_LVGL_BUILTIN, FLG_LVGL_BUILTIN_STOP_DONE);

	THREAD_RETURN(0);
}

int Lvgl_BuiltIn_TskStart(void)
{
	THREAD_CREATE(LVGL_BUILTIN_TSK_ID, Lvgl_BuiltIn_Tsk, NULL, "Lvgl_BuiltIn_Tsk");
	if (LVGL_BUILTIN_TSK_ID == 0) {
		DBG_ERR("Invalid LVGL_BUILTIN_TSK_ID\r\n");
		return -1;
	}
	THREAD_SET_PRIORITY(LVGL_BUILTIN_TSK_ID, 4);
	THREAD_RESUME(LVGL_BUILTIN_TSK_ID);

	return 0;
}

void Lvgl_BuiltIn_TskStop(void)
{
	FLGPTN			uiFlag = 0;

	set_flg(FLG_ID_LVGL_BUILTIN, FLG_LVGL_BUILTIN_STOP);
	wai_flg(&uiFlag, FLG_ID_LVGL_BUILTIN, FLG_LVGL_BUILTIN_STOP_DONE, TWF_ORW | TWF_CLR);
}

static REGVALUE rtc_getreg(uint32_t offset)
{
	return nvt_readl(_REGIOBASE + offset);
}

static int Lvgl_Builtin_rtc_init(void)
{
	if(unlikely(_REGIOBASE) == 0){
		_REGIOBASE = (u32) ioremap(IOADDR_RTC_REG_BASE, 64);

		if(_REGIOBASE == 0){
			DBG_ERR("[LVGL] rtc ioremap failed!!\b");
			return -1;
		}
	}

	return 0;
}

int Lvgl_Builtin_rtc_read_time(struct tm *datetime)
{
	union RTC_TIMER_REG timer_reg;
	union RTC_DAYKEY_REG daykey_reg;
	uint32_t days, months, years, month_days;

	timer_reg.reg = rtc_getreg(RTC_TIMER_REG_OFS);
	daykey_reg.reg = rtc_getreg(RTC_DAYKEY_REG_OFS);
	days = daykey_reg.bit.day;

	for (years = 0; days >= rtc_ydays[is_leap_year(years + 1900)][12]; \
	years++) {
		days -= rtc_ydays[is_leap_year(years + 1900)][12];
	}

	for (months = 1; months < 13; months++) {
		if (days <= rtc_ydays[is_leap_year(years + 1900)][months]) {
			days -= rtc_ydays[is_leap_year(years + 1900)][months-1];
			months--;
			break;
		}
	}

	month_days = rtc_ydays[is_leap_year(years + 1900)][months+1] - \
		rtc_ydays[is_leap_year(years + 1900)][months];

	if (days == month_days) {
		months++;
		days = 1;
	} else
		days++; /*Align linux time format*/

	datetime->tm_sec  = timer_reg.bit.sec;
	datetime->tm_min  = timer_reg.bit.min;
	datetime->tm_hour = timer_reg.bit.hour;
	datetime->tm_mday = days;
	datetime->tm_mon  = months + 1;
	datetime->tm_year = years + 1900;

	return 0;
}
#endif












#endif
