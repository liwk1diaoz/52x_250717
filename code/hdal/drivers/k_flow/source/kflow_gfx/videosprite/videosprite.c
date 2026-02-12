#if defined(__LINUX)
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "kwrap/cmdsys.h"
#endif

#include "kwrap/semaphore.h"
#include "kwrap/spinlock.h"
#include "kwrap/cpu.h"
#include "kflow.h"
#include "kflow_common/isf_flow_def.h"
#include "videosprite/videosprite.h"
#include "videosprite/videosprite_internal.h"
#include "videosprite/videosprite_open.h"
#include "videosprite/videosprite_ime.h"
#include "videosprite/videosprite_enc.h"
#include "videosprite/videosprite_vo.h"
#include "kflow_common/nvtmpp.h"
#include <gximage/gximage.h>
#include "comm/hwclock.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__        videosprite
#define __DBGLVL__                NVT_DBG_WRN
#include "kwrap/debug.h"
unsigned int videosprite_debug_level = __DBGLVL__;
///////////////////////////////////////////////////////////////////////////////

static int stamp_img_cnt         = 0;
static int vds_max_coe_vid       = 0;
int vds_max_coe_stamp            = 0;
static int vds_max_coe_ext_vid   = 0;
static int vds_max_coe_ext_stamp = 0;
static int vds_max_coe_ext_mask  = 0;
static int vds_max_ime           = 0;
int vds_max_ime_stamp            = 0;
int vds_max_ime_mask             = 0;
static int vds_max_ime_ext       = 0;
static int vds_max_ime_ext_stamp = 0;
static int vds_max_ime_ext_mask  = 0;
static int vds_max_vo_vid        = 0;
static int vds_max_vo_stamp      = 0;
static int vds_max_vo_mask       = 0;

typedef struct _VDS_BUF {
	UINT32            valid;          ///< 1:initialized / 0:not used
	VDS_BUF_TYPE      type;           ///< ping pong / single
	UINT32            ref;            ///< reference count
	VDS_IMG_FMT       fmt[2];         ///< rgb565 / argb1555 / argb4444
	UINT32            p_addr[2];      ///< rgb buffer pointer
	UINT32            size[2];        ///< rgb buffer size in byte
	USIZE             img_size[2];    ///< image's width & height
	UINT32            swap;           ///< 0:buf[0] is for app,buf[1] is for hardware / 1:buf[0] is for hardware,buf[1] is for app
	UINT32            dirty_for_get;  ///< vds_get_stamp_img() should not copy image content to user space if image is not updated
} VDS_BUF;

typedef struct _VDS_RGN {
	UINT32            valid;          ///< 1:initialized / 0:not used
	UINT32            en;             ///< 1:display / 0:not display
	UINT32            is_mosaic;      ///< 1:mosaic / 0:mask or stamp
	UINT32            is_enc_mask;    ///< 1:osg mask / 0:osg stamp
	VDS_BUF           *buf;           ///< pointer to hold bitmap
	union{
		VDS_STAMP_ATTR    stamp;
		VDS_MASK_ATTR     mask;
		VDS_MOSAIC_ATTR   mosaic;
		VDS_ENC_MASK      enc_mask;
	}attr;
} VDS_RGN;

static int vo_stamp_cnt      = 0;
static int vo_mask_cnt       = 0;
static int coe_stamp_cnt     = 0;
static int coe_ext_stamp_cnt = 0;
static int coe_ext_mask_cnt  = 0;
static int ime_stamp_cnt     = 0;
static int ime_mask_cnt      = 0;
static int ime_ext_stamp_cnt = 0;
static int ime_ext_mask_cnt  = 0;

static VDS_BUF     *stamp_imgs     = NULL;
static VDS_RGN     *vo_stamps      = NULL;
static VDS_RGN     *vo_masks       = NULL;
static VDS_RGN     *coe_stamps     = NULL;
static VDS_RGN     *coe_ext_stamps = NULL;
static VDS_RGN     *coe_ext_masks  = NULL;
static VDS_RGN     *ime_stamps     = NULL;
static VDS_RGN     *ime_masks      = NULL;
static VDS_RGN     *ime_ext_stamps = NULL;
static VDS_RGN     *ime_ext_masks  = NULL;

static VDS_TO_IME_STAMP         ime_stamp     = {0};
static VDS_TO_IME_MASK          ime_mask      = {0};
static VDS_TO_IME_EXT_STAMP     ime_ext_stamp = {0};
static VDS_TO_IME_EXT_MASK      ime_ext_mask  = {0};
static VDS_TO_ENC_EXT_STAMP     enc_ext_stamp = {0};
static VDS_TO_ENC_EXT_MASK      enc_ext_mask  = {0};
static VDS_TO_ENC_COE_STAMP     enc_coe_stamp = {0};
static VDS_TO_ENC_COE_STAMP     enc_jpg_stamp = {0};
static VDS_TO_VO_STAMP          vo_stamp      = {0};
static VDS_TO_VO_MASK           vo_mask       = {0};

#define LOCK_RESOURCE_IME_STAMP        (1)
#define LOCK_RESOURCE_IME_MASK         (1<<1)
#define LOCK_RESOURCE_IME_EXT_STAMP    (1<<2)
#define LOCK_RESOURCE_IME_EXT_MASK     (1<<3)
#define LOCK_RESOURCE_ENC_EXT_STAMP    (1<<4)
#define LOCK_RESOURCE_ENC_EXT_MASK     (1<<5)
#define LOCK_RESOURCE_ENC_COE_STAMP    (1<<6)
#define LOCK_RESOURCE_ENC_JPG_STAMP    (1<<7)
#define LOCK_RESOURCE_VO_STAMP         (1<<8)
#define LOCK_RESOURCE_VO_MASK          (1<<9)
#define LOCK_RESOURCE_ALL              (LOCK_RESOURCE_IME_STAMP | \
									    LOCK_RESOURCE_IME_MASK | \
									    LOCK_RESOURCE_IME_EXT_STAMP | \
									    LOCK_RESOURCE_IME_EXT_MASK | \
									    LOCK_RESOURCE_ENC_EXT_STAMP | \
									    LOCK_RESOURCE_ENC_EXT_MASK | \
									    LOCK_RESOURCE_ENC_COE_STAMP | \
									    LOCK_RESOURCE_ENC_JPG_STAMP | \
									    LOCK_RESOURCE_VO_STAMP | \
									    LOCK_RESOURCE_VO_MASK )

SEM_HANDLE VDS_API_SEM_ID           = {0};
VK_DEFINE_SPINLOCK(VDS_IME_MASK_SEM_ID);
SEM_HANDLE VDS_IME_EXT_STAMP_SEM_ID = {0};
SEM_HANDLE VDS_IME_EXT_MASK_SEM_ID  = {0};
SEM_HANDLE VDS_ENC_EXT_STAMP_SEM_ID = {0};
SEM_HANDLE VDS_ENC_EXT_MASK_SEM_ID  = {0};
SEM_HANDLE VDS_ENC_COE_STAMP_SEM_ID = {0};
SEM_HANDLE VDS_ENC_JPG_STAMP_SEM_ID = {0};
SEM_HANDLE VDS_VO_STAMP_SEM_ID      = {0};
SEM_HANDLE VDS_VO_MASK_SEM_ID       = {0};

static UINT32         g_nvtmpp_addr     = 0;
static UINT32         g_nvtmpp_size     = 0;

static UINT32 *ime_palette = NULL;
static UINT32 *coe_palette = NULL;
static UINT32 *vo_palette  = NULL;
static char *render_coe_buffer = NULL;

//if app system("X"), another pair of api_proc_open and
//api_release would be called. We need a counter to
//prevent api_release from reseting videosprite.
static int            app_open_count    = 0;

typedef struct _LATENCY_RECORD {
	VDS_QUERY_STAGE    stage;
	int                vid;
	int                lock;
	UINT64             time;
} LATENCY_RECORD;

#ifdef CONFIG_NVT_SMALL_HDAL
#define MAX_LATENCY_RECORD 6
#else
#define MAX_LATENCY_RECORD 30
#endif
LATENCY_RECORD min_proc_stamp_latency[MAX_LATENCY_RECORD], max_proc_stamp_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_proc_mask_latency[MAX_LATENCY_RECORD], max_proc_mask_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_proc_ext_stamp_latency[MAX_LATENCY_RECORD], max_proc_ext_stamp_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_proc_ext_mask_latency[MAX_LATENCY_RECORD], max_proc_ext_mask_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_enc_stamp_latency[MAX_LATENCY_RECORD], max_enc_stamp_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_jpg_stamp_latency[MAX_LATENCY_RECORD], max_jpg_stamp_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_enc_ext_stamp_latency[MAX_LATENCY_RECORD], max_enc_ext_stamp_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_enc_ext_mask_latency[MAX_LATENCY_RECORD], max_enc_ext_mask_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_out_stamp_latency[MAX_LATENCY_RECORD], max_out_stamp_latency[MAX_LATENCY_RECORD];
LATENCY_RECORD min_out_mask_latency[MAX_LATENCY_RECORD], max_out_mask_latency[MAX_LATENCY_RECORD];
UINT64 avg_proc_stamp_latency, avg_proc_mask_latency, avg_proc_ext_stamp_latency, avg_proc_ext_mask_latency;
UINT64 avg_enc_stamp_latency, avg_jpg_stamp_latency, avg_enc_ext_stamp_latency, avg_enc_ext_mask_latency;
UINT64 avg_out_stamp_latency, avg_out_mask_latency;

VK_DEFINE_SPINLOCK(VDS_LATENCY_LOCK_ID);

//-----------------------------------------------------------------------------
// API
//-----------------------------------------------------------------------------

#define cst_prom0   21
#define cst_prom1   79
#define cst_prom2   29
#define cst_prom3   43
#define RGB_GET_Y(r,g,b)    (((INT32)g) + ((cst_prom1 * (((INT32)r)-((INT32)g))) >> 8) + ((cst_prom2 * (((INT32)b)-((INT32)g))) >> 8) )
#define RGB_GET_U(r,g,b)    (128 + ((cst_prom3 * (((INT32)g)-((INT32)r))) >> 8) + ((((INT32)b)-((INT32)g)) >> 1) )
#define RGB_GET_V(r,g,b)    (128 + ((cst_prom0 * (((INT32)g)-((INT32)b))) >> 8) + ((((INT32)r)-((INT32)g)) >> 1) )

static void vds_api_lock(void)
{
	SEM_WAIT(VDS_API_SEM_ID);
}

static void vds_api_unlock(void)
{
	SEM_SIGNAL(VDS_API_SEM_ID);
}

static void vds_res_lock(UINT32 res, UINT32 *ime_flag)
{
#if 0
	if(res & LOCK_RESOURCE_IME_STAMP)
		down(&ime_stamp_lock);
#endif
	if(res & LOCK_RESOURCE_ENC_EXT_STAMP)
		SEM_WAIT(VDS_ENC_EXT_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_ENC_EXT_MASK)
		SEM_WAIT(VDS_ENC_EXT_MASK_SEM_ID);
	if(res & LOCK_RESOURCE_ENC_COE_STAMP)
		SEM_WAIT(VDS_ENC_COE_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_ENC_JPG_STAMP)
		SEM_WAIT(VDS_ENC_JPG_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_IME_EXT_STAMP)
		SEM_WAIT(VDS_IME_EXT_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_IME_EXT_MASK)
		SEM_WAIT(VDS_IME_EXT_MASK_SEM_ID);
	if(res & LOCK_RESOURCE_VO_STAMP)
		SEM_WAIT(VDS_VO_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_VO_MASK)
		SEM_WAIT(VDS_VO_MASK_SEM_ID);
	if(res & LOCK_RESOURCE_IME_MASK){
		if(ime_flag){
			unsigned long flag;
			vk_spin_lock_irqsave(&VDS_IME_MASK_SEM_ID, flag);
			*ime_flag = flag;
		}else
			DBG_ERR("ime_flag can not be NULL\n");
	}
}

static void vds_res_unlock(UINT32 res, UINT32 ime_flag)
{
	if(res & LOCK_RESOURCE_IME_MASK)
		vk_spin_unlock_irqrestore(&VDS_IME_MASK_SEM_ID, ime_flag);
	if(res & LOCK_RESOURCE_VO_MASK)
		SEM_SIGNAL(VDS_VO_MASK_SEM_ID);
	if(res & LOCK_RESOURCE_VO_STAMP)
		SEM_SIGNAL(VDS_VO_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_IME_EXT_MASK)
		SEM_SIGNAL(VDS_IME_EXT_MASK_SEM_ID);
	if(res & LOCK_RESOURCE_IME_EXT_STAMP)
		SEM_SIGNAL(VDS_IME_EXT_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_ENC_JPG_STAMP)
		SEM_SIGNAL(VDS_ENC_JPG_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_ENC_COE_STAMP)
		SEM_SIGNAL(VDS_ENC_COE_STAMP_SEM_ID);
	if(res & LOCK_RESOURCE_ENC_EXT_MASK)
		SEM_SIGNAL(VDS_ENC_EXT_MASK_SEM_ID);
	if(res & LOCK_RESOURCE_ENC_EXT_STAMP)
		SEM_SIGNAL(VDS_ENC_EXT_STAMP_SEM_ID);
#if 0
	if(res & LOCK_RESOURCE_IME_STAMP)
		up(&ime_stamp_lock);
#endif
}

static void vds_clear(int free)
{
	if(ime_stamp.data && vds_max_ime_stamp){
		if(free)
			ime_stamp.data = NULL;
		else
			vds_memset(ime_stamp.data, 0, sizeof(VDS_INTERNAL_IME_STAMP) * vds_max_ime_stamp);
		ime_stamp.dirty = 0;
	}
	
	if(ime_mask.data && vds_max_ime_mask){
		if(free)
			ime_mask.data = NULL;
		else
			vds_memset(ime_mask.data, 0, sizeof(VDS_INTERNAL_IME_MASK) * vds_max_ime_mask);
		ime_mask.dirty = 0;
	}

	if(ime_ext_stamp.data.stamp && vds_max_ime_ext_stamp){
		if(free)
			ime_ext_stamp.data.stamp = NULL;
		else
			vds_memset(ime_ext_stamp.data.stamp, 0, sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_ime_ext_stamp);
		ime_ext_stamp.dirty = 0;
	}

	if(ime_ext_mask.data.mask && vds_max_ime_ext_mask){
		if(free)
			ime_ext_mask.data.mask = NULL;
		else
			vds_memset(ime_ext_mask.data.mask, 0, sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_ime_ext_mask);
		ime_ext_mask.dirty = 0;
	}

	if(enc_ext_stamp.stamp && vds_max_coe_ext_stamp){
		if(free)
			enc_ext_stamp.stamp = NULL;
		else
			vds_memset(enc_ext_stamp.stamp, 0, sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_coe_ext_stamp);
		enc_ext_stamp.dirty = 0;
	}

	if(enc_ext_mask.mask && vds_max_coe_ext_mask){
		if(free)
			enc_ext_mask.mask = NULL;
		else
			vds_memset(enc_ext_mask.mask, 0, sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_coe_ext_mask);
		enc_ext_mask.dirty = 0;
	}
	
	if(enc_coe_stamp.stamp && vds_max_coe_stamp){
		if(free)
			enc_coe_stamp.stamp = NULL;
		else
			vds_memset(enc_coe_stamp.stamp, 0, sizeof(VDS_INTERNAL_COE_STAMP_MASK) * vds_max_coe_stamp);
		enc_coe_stamp.dirty = 0;
	}
	
	if(enc_jpg_stamp.stamp && vds_max_coe_stamp){
		if(free)
			enc_jpg_stamp.stamp = NULL;
		else
			vds_memset(enc_jpg_stamp.stamp, 0, sizeof(VDS_INTERNAL_COE_STAMP_MASK) * vds_max_coe_stamp);
		enc_jpg_stamp.dirty = 0;
	}
	
	if(vo_stamp.stamp && vds_max_vo_stamp){
		if(free)
			vo_stamp.stamp = NULL;
		else
			vds_memset(vo_stamp.stamp, 0, sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_vo_stamp);
		vo_stamp.dirty = 0;
	}
	
	if(vo_mask.mask && vds_max_vo_mask){
		if(free)
			vo_mask.mask = NULL;
		else
			vds_memset(vo_mask.mask, 0, sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_vo_mask);
		vo_mask.dirty = 0;
	}
}

int vds_reset(void)
{
	UINT32 ime_flag = 0;
	
	vds_api_lock();
	vds_res_lock(LOCK_RESOURCE_ALL, &ime_flag);

	if(stamp_imgs && stamp_img_cnt)
		vds_memset(stamp_imgs, 0, sizeof(VDS_BUF) * stamp_img_cnt);
	
	if(vo_stamps && vo_stamp_cnt)
		vds_memset(vo_stamps, 0, sizeof(VDS_RGN) * vo_stamp_cnt);
	
	if(vo_masks && vo_mask_cnt)
		vds_memset(vo_masks, 0, sizeof(VDS_RGN) * vo_mask_cnt);
	
	if(coe_stamps && coe_stamp_cnt)
		vds_memset(coe_stamps, 0, sizeof(VDS_RGN) * coe_stamp_cnt);
	
	if(coe_ext_stamps && coe_ext_stamp_cnt)
		vds_memset(coe_ext_stamps, 0, sizeof(VDS_RGN) * coe_ext_stamp_cnt);
	
	if(coe_ext_masks && coe_ext_mask_cnt)
		vds_memset(coe_ext_masks, 0, sizeof(VDS_RGN) * coe_ext_mask_cnt);
	
	if(ime_stamps && ime_stamp_cnt)
		vds_memset(ime_stamps, 0, sizeof(VDS_RGN) * ime_stamp_cnt);
	
	if(ime_masks && ime_mask_cnt)
		vds_memset(ime_masks, 0, sizeof(VDS_RGN) * ime_mask_cnt);
	
	if(ime_ext_stamps && ime_ext_stamp_cnt)
		vds_memset(ime_ext_stamps, 0, sizeof(VDS_RGN) * ime_ext_stamp_cnt);

	if(ime_ext_masks && ime_ext_mask_cnt)
		vds_memset(ime_ext_masks, 0, sizeof(VDS_RGN) * ime_ext_mask_cnt);

	if(ime_palette && vds_max_ime)
		vds_memset(ime_palette, 0, 16 * sizeof(UINT32) * vds_max_ime);

	if(coe_palette && vds_max_coe_vid)
		vds_memset(coe_palette, 0, 16 * sizeof(UINT32) * vds_max_coe_vid);

	if(vo_palette && vds_max_vo_vid)
		vds_memset(vo_palette, 0, 16 * sizeof(UINT32) * vds_max_vo_vid);

	vds_clear(0);

	vds_res_unlock(LOCK_RESOURCE_ALL, ime_flag);
	vds_api_unlock();
	
	return ISF_OK;
}

static void _vds_reset(void)
{
	vo_stamp_cnt       = 0;
	vo_mask_cnt        = 0;
	coe_stamp_cnt      = 0;
	coe_ext_stamp_cnt  = 0;
	coe_ext_mask_cnt   = 0;
	ime_stamp_cnt      = 0;
	ime_mask_cnt       = 0;
	ime_ext_stamp_cnt  = 0;
	ime_ext_mask_cnt   = 0;

	stamp_imgs      = NULL;
	vo_stamps       = NULL;
	vo_masks        = NULL;
	coe_stamps      = NULL;
	coe_ext_stamps  = NULL;
	coe_ext_masks   = NULL;
	ime_stamps      = NULL;
	ime_masks       = NULL;
	ime_ext_stamps  = NULL;
	ime_ext_masks   = NULL;
	ime_palette     = NULL;
	coe_palette     = NULL;
	vo_palette      = NULL;
	render_coe_buffer = NULL;
	
	vds_clear(1);
	
	//if(g_nvtmpp_addr)
	//	vds_free((void*)g_nvtmpp_addr);
	//g_nvtmpp_addr = 0;	
}

static int _vds_init(void)
{
	int ret = ISF_ERR_NEW_FAIL;
	int stamp_imgs_buf_size, vo_stamps_buf_size, vo_masks_buf_size, coe_stamps_buf_size, coe_ext_stamps_buf_size;
	int coe_ext_masks_buf_size, ime_stamps_buf_size, ime_masks_buf_size, ime_ext_stamps_buf_size, ime_ext_masks_buf_size;
	int ime_stamp_data_buf_size, ime_mask_data_buf_size, ime_ext_stamp_data_buf_size, ime_ext_mask_data_buf_size;
	int enc_ext_stamp_buf_size, enc_ext_mask_buf_size, enc_coe_stamp_buf_size, enc_jpg_stamp_buf_size;
	int vo_stamp_buf_size, vo_mask_buf_size, ime_palette_buf_size, coe_palette_buf_size, vo_palette_buf_size, render_coe_buf_size;
	int total_buf_size = 0;
	char *p_buf;

	if(vds_max_ime_ext == -1)
		vds_max_ime_ext = (vds_max_ime * 4);

	if(stamp_img_cnt == -1){
		stamp_img_cnt  = (vds_max_ime * vds_max_ime_stamp +
							vds_max_ime * vds_max_ime_ext * vds_max_ime_ext_stamp +
							vds_max_coe_vid * vds_max_coe_stamp +
							vds_max_coe_ext_vid * vds_max_coe_ext_stamp +
							vds_max_vo_vid * vds_max_vo_stamp);
	}
	vo_stamp_cnt       = vds_max_vo_vid      * vds_max_vo_stamp;
	vo_mask_cnt        = vds_max_vo_vid      * vds_max_vo_mask;
	coe_stamp_cnt      = vds_max_coe_vid     * vds_max_coe_stamp;
	coe_ext_stamp_cnt  = vds_max_coe_ext_vid * vds_max_coe_ext_stamp;
	coe_ext_mask_cnt   = vds_max_coe_ext_vid * vds_max_coe_ext_mask;
	ime_stamp_cnt      = vds_max_ime         * vds_max_ime_stamp;
	ime_mask_cnt       = vds_max_ime         * vds_max_ime_mask;
	ime_ext_stamp_cnt  = vds_max_ime * vds_max_ime_ext * vds_max_ime_ext_stamp;
	ime_ext_mask_cnt   = vds_max_ime * vds_max_ime_ext * vds_max_ime_ext_mask;

	stamp_imgs_buf_size = sizeof(VDS_BUF) * stamp_img_cnt;
	total_buf_size += stamp_imgs_buf_size;
	vo_stamps_buf_size = sizeof(VDS_RGN) * vo_stamp_cnt;
	total_buf_size += vo_stamps_buf_size;
	vo_masks_buf_size = sizeof(VDS_RGN) * vo_mask_cnt;
	total_buf_size += vo_masks_buf_size;
	coe_stamps_buf_size = sizeof(VDS_RGN) * coe_stamp_cnt;
	total_buf_size += coe_stamps_buf_size;
	coe_ext_stamps_buf_size = sizeof(VDS_RGN) * coe_ext_stamp_cnt;
	total_buf_size += coe_ext_stamps_buf_size;
	coe_ext_masks_buf_size = sizeof(VDS_RGN) * coe_ext_mask_cnt;
	total_buf_size += coe_ext_masks_buf_size;
	ime_stamps_buf_size = sizeof(VDS_RGN) * ime_stamp_cnt;
	total_buf_size += ime_stamps_buf_size;
	ime_masks_buf_size = sizeof(VDS_RGN) * ime_mask_cnt;
	total_buf_size += ime_masks_buf_size;
	ime_ext_stamps_buf_size = sizeof(VDS_RGN) * ime_ext_stamp_cnt;
	total_buf_size += ime_ext_stamps_buf_size;
	ime_ext_masks_buf_size = sizeof(VDS_RGN) * ime_ext_mask_cnt;
	total_buf_size += ime_ext_masks_buf_size;
	ime_stamp_data_buf_size = sizeof(VDS_INTERNAL_IME_STAMP) * vds_max_ime_stamp;
	total_buf_size += ime_stamp_data_buf_size;
	ime_mask_data_buf_size = sizeof(VDS_INTERNAL_IME_MASK) * vds_max_ime_mask;
	total_buf_size += ime_mask_data_buf_size;
	ime_ext_stamp_data_buf_size = sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_ime_ext_stamp;
	total_buf_size += ime_ext_stamp_data_buf_size;
	ime_ext_mask_data_buf_size = sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_ime_ext_mask;
	total_buf_size += ime_ext_mask_data_buf_size;
	enc_ext_stamp_buf_size = sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_coe_ext_stamp;
	total_buf_size += enc_ext_stamp_buf_size;
	enc_ext_mask_buf_size = sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_coe_ext_mask;
	total_buf_size += enc_ext_mask_buf_size;
	enc_coe_stamp_buf_size = sizeof(VDS_INTERNAL_COE_STAMP_MASK) * vds_max_coe_stamp;
	total_buf_size += enc_coe_stamp_buf_size;
	enc_jpg_stamp_buf_size = sizeof(VDS_INTERNAL_COE_STAMP_MASK) * vds_max_coe_stamp;
	total_buf_size += enc_jpg_stamp_buf_size;
	vo_stamp_buf_size = sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_vo_stamp;
	total_buf_size += vo_stamp_buf_size;
	vo_mask_buf_size = sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_vo_mask;
	total_buf_size += vo_mask_buf_size;
	ime_palette_buf_size = 16 * sizeof(UINT32) * vds_max_ime;
	total_buf_size += ime_palette_buf_size;
	coe_palette_buf_size = 16 * sizeof(UINT32) * vds_max_coe_vid;
	total_buf_size += coe_palette_buf_size;
	vo_palette_buf_size = 16 * sizeof(UINT32) * vds_max_vo_vid;
	total_buf_size += vo_palette_buf_size;
	if(sizeof(VDS_INTERNAL_EXT_STAMP) > sizeof(VDS_INTERNAL_EXT_MASK))
		render_coe_buf_size = sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_coe_stamp;
	else
		render_coe_buf_size = sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_coe_stamp;
	total_buf_size += render_coe_buf_size;

	if(total_buf_size){					
		if(g_nvtmpp_size && (int)g_nvtmpp_size != total_buf_size){
			DBG_ERR("can't support re-init with different fdt setting:old(%d) new(%d)\n", g_nvtmpp_size, total_buf_size);
			goto out;
		}
		if(!g_nvtmpp_addr)
			g_nvtmpp_addr = (UINT32)vds_alloc(total_buf_size);
		if (g_nvtmpp_addr == 0) {
			DBG_ERR("fail to vds_alloc(%d) \n", total_buf_size);
			goto out;
		}
		g_nvtmpp_size = total_buf_size;
	}
	p_buf = (char*)g_nvtmpp_addr;
	if (p_buf == NULL) {
		DBG_ERR("fail to allocate vds buffer size 0x%x\r\n", total_buf_size);
		goto out;
	}
	vds_memset(p_buf, 0, total_buf_size);
	if (stamp_img_cnt){
		stamp_imgs = (void*)p_buf;
		p_buf += stamp_imgs_buf_size;
	} else {
		stamp_imgs = NULL;
	}
	DBG_IND("stamp_imgs = 0x%x\r\n", (int)stamp_imgs);
	if (vo_stamp_cnt){
		vo_stamps = (void*)p_buf;
		p_buf += vo_stamps_buf_size;
	} else {
		vo_stamps = NULL;
	}
	DBG_IND("vo_stamps = 0x%x\r\n", (int)vo_stamps);
	if (vo_mask_cnt){
		vo_masks = (void*)p_buf;
		p_buf += vo_masks_buf_size;
	} else {
		vo_masks = NULL;
	}
	DBG_IND("vo_masks = 0x%x\r\n", (int)vo_masks);
	if (coe_stamp_cnt){
		coe_stamps = (void*)p_buf;
		p_buf += coe_stamps_buf_size;
	} else {
		coe_stamps = NULL;
	}
	DBG_IND("coe_stamps = 0x%x\r\n", (int)coe_stamps);
	if (coe_ext_stamp_cnt){
		coe_ext_stamps = (void*)p_buf;
		p_buf += coe_ext_stamps_buf_size;
	} else {
		coe_ext_stamps = NULL;
	}
	DBG_IND("coe_ext_stamps = 0x%x\r\n", (int)coe_ext_stamps);
	if (coe_ext_mask_cnt){
		coe_ext_masks = (void*)p_buf;
		p_buf += coe_ext_masks_buf_size;
	} else {
		coe_ext_masks = NULL;
	}
	DBG_IND("coe_ext_masks = 0x%x\r\n", (int)coe_ext_masks);
	if (ime_stamp_cnt){
		ime_stamps = (void*)p_buf;
		p_buf += ime_stamps_buf_size;
	} else {
		ime_stamps = NULL;
	}
	DBG_IND("ime_stamps = 0x%x\r\n", (int)ime_stamps);
	if (ime_mask_cnt){
		ime_masks = (void*)p_buf;
		p_buf += ime_masks_buf_size;
	} else {
		ime_masks = NULL;
	}
	DBG_IND("ime_masks = 0x%x\r\n", (int)ime_masks);
	if (ime_ext_stamp_cnt){
		ime_ext_stamps = (void*)p_buf;
		p_buf += ime_ext_stamps_buf_size;
	} else {
		ime_ext_stamps = NULL;
	}
	DBG_IND("ime_ext_stamps = 0x%x\r\n", (int)ime_ext_stamps);
	if (ime_ext_mask_cnt){
		ime_ext_masks = (void*)p_buf;
		p_buf += ime_ext_masks_buf_size;
	} else {
		ime_ext_masks = NULL;
	}
	DBG_IND("ime_ext_masks = 0x%x\r\n", (int)ime_ext_masks);
	if (vds_max_ime_stamp){
		ime_stamp.data = (void*)p_buf;
		p_buf += ime_stamp_data_buf_size;
	} else {
		ime_stamp.data = NULL;
	}
	DBG_IND("ime_stamp.data = 0x%x\r\n", (int)ime_stamp.data);
	if (vds_max_ime_mask){
		ime_mask.data = (void*)p_buf;
		p_buf += ime_mask_data_buf_size;
	} else {
		ime_mask.data = NULL;
	}
	DBG_IND("ime_mask.data = 0x%x\r\n", (int)ime_mask.data);
	if (vds_max_ime_ext_stamp){
		ime_ext_stamp.data.stamp = (void*)p_buf;
		p_buf += ime_ext_stamp_data_buf_size;
	} else {
		ime_ext_stamp.data.stamp = NULL;
	}
	DBG_IND("ime_ext_stamp.data.stamp = 0x%x\r\n", (int)ime_ext_stamp.data.stamp);
	if (vds_max_ime_ext_mask){
		ime_ext_mask.data.mask = (void*)p_buf;
		p_buf += ime_ext_mask_data_buf_size;
	} else {
		ime_ext_mask.data.mask = NULL;
	}
	DBG_IND("ime_ext_mask.data.mask = 0x%x\r\n", (int)ime_ext_mask.data.mask);
	if (vds_max_coe_ext_stamp){
		enc_ext_stamp.stamp = (void*)p_buf;
		p_buf += enc_ext_stamp_buf_size;
	} else {
		enc_ext_stamp.stamp = NULL;
	}
	DBG_IND("enc_ext_stamp.stamp = 0x%x\r\n", (int)enc_ext_stamp.stamp);
	if (vds_max_coe_ext_mask){
		enc_ext_mask.mask = (void*)p_buf;
		p_buf += enc_ext_mask_buf_size;
	} else {
		enc_ext_mask.mask = NULL;
	}
	DBG_IND("enc_ext_mask.mask = 0x%x\r\n", (int)enc_ext_mask.mask);
	if (vds_max_coe_stamp){
		enc_coe_stamp.stamp = (void*)p_buf;
		p_buf += enc_coe_stamp_buf_size;
	} else {
		enc_coe_stamp.stamp = NULL;
	}
	DBG_IND("enc_coe_stamp.stamp = 0x%x\r\n", (int)enc_coe_stamp.stamp);
	if (vds_max_coe_stamp){
		enc_jpg_stamp.stamp = (void*)p_buf;
		p_buf += enc_jpg_stamp_buf_size;
	} else {
		enc_jpg_stamp.stamp = NULL;
	}
	DBG_IND("enc_jpg_stamp.stamp = 0x%x\r\n", (int)enc_jpg_stamp.stamp);
	if (vds_max_vo_stamp){
		vo_stamp.stamp = (void*)p_buf;
		p_buf += vo_stamp_buf_size;
	} else {
		vo_stamp.stamp = NULL;
	}
	DBG_IND("vo_stamp.stamp = 0x%x\r\n", (int)vo_stamp.stamp);
	if(vds_max_vo_mask){
		vo_mask.mask = (void*)p_buf;
		p_buf += vo_mask_buf_size;
	} else {
		vo_mask.mask = NULL;
	}
	DBG_IND("vo_mask.mask = 0x%x\r\n", (int)vo_mask.mask);
	if(vds_max_ime){
		ime_palette = (void*)p_buf;
		p_buf += ime_palette_buf_size;
	} else {
		ime_palette = NULL;
	}
	DBG_IND("ime_palette = 0x%x\r\n", (int)ime_palette);
	if (vds_max_coe_vid){
		coe_palette = (void*)p_buf;
		p_buf += coe_palette_buf_size;
	} else {
		coe_palette = NULL;
	}
	DBG_IND("coe_palette = 0x%x\r\n", (int)coe_palette);
	if (vds_max_vo_vid){
		vo_palette = (void*)p_buf;
		p_buf += vo_palette_buf_size;
	} else {
		vo_palette = NULL;
	}
	DBG_IND("vo_palette = 0x%x\r\n", (int)vo_palette);
	render_coe_buffer = p_buf;
	DBG_IND("render_coe_buffer = 0x%x\r\n", (int)render_coe_buffer);
	//coverity[assigned_pointer]
	p_buf += render_coe_buf_size;

	ret = ISF_OK;
out:	
	if(ret != ISF_OK && ret != ISF_ERR_INVALID_STATE)
		_vds_reset();

	return ret;
}

int vds_init(void)
{
	vds_api_lock();
	app_open_count++;
	vds_api_unlock();
	
	return ISF_OK;
}

int vds_uninit(void)
{
	UINT32 ime_flag = 0;
	vds_api_lock();

	app_open_count--;
	if(app_open_count > 0){
		vds_api_unlock();
		return ISF_OK;
	}

	vds_res_lock(LOCK_RESOURCE_ALL, &ime_flag);
	
	_vds_reset();

	vds_res_unlock(LOCK_RESOURCE_ALL, ime_flag);
	vds_api_unlock();
	
	return ISF_OK;
}

static VDS_RGN* find_region(VDS_PHASE phase, UINT32 rgn, UINT32 vid, UINT32 *lock)
{
	UINT32 ime_dev, ime_port;

	if(phase == VDS_PHASE_IME_STAMP){
		if((int)rgn >= vds_max_ime_stamp){
			DBG_ERR("invalid ime stamp(%d) vid(%d). max stamp(%d) vid(%d)\n", rgn, vid, vds_max_ime_stamp, vds_max_ime);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_ime_stamp); 
			return NULL;
		}else if((int)vid >= vds_max_ime){
			DBG_ERR("invalid ime stamp(%d) vid(%d). max stamp(%d) vid(%d)\n", rgn, vid, vds_max_ime_stamp, vds_max_ime);
			DBG_ERR("port_id=%d is larger then max=%d!\r\n", vid, vds_max_ime); 
			return NULL;
		}

		if(lock)
			*lock = LOCK_RESOURCE_IME_STAMP;

		return &(ime_stamps[vds_max_ime_stamp*vid + rgn]);
	}
	else if(phase == VDS_PHASE_IME_MASK || phase == VDS_PHASE_IME_MOSAIC){
		if((int)rgn >= vds_max_ime_mask){
			DBG_ERR("invalid ime mask(%d) vid(%d). max mask(%d) vid(%d)\n", rgn, vid, vds_max_ime_mask, vds_max_ime);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_ime_mask); 
			return NULL;
		}else if((int)vid >= vds_max_ime){
			DBG_ERR("invalid ime mask(%d) vid(%d). max mask(%d) vid(%d)\n", rgn, vid, vds_max_ime_mask, vds_max_ime);
			DBG_ERR("port_id=%d is larger then max=%d!\r\n", vid, vds_max_ime); 
			return NULL;
		}

		if(lock)
			*lock = LOCK_RESOURCE_IME_MASK;

		return &(ime_masks[vds_max_ime_mask*vid + rgn]);
	}
	else if(phase == VDS_PHASE_IME_EXT_STAMP){
		ime_dev = ((vid & 0xFFFF0000) >> 16);
		ime_port = (vid & 0x0FFFF);
		vid = (ime_dev * vds_max_ime_ext + ime_port);

		if((int)rgn >= vds_max_ime_ext_stamp){
			DBG_ERR("invalid ime ext stamp(%d) vid(%d). max ext stamp(%d) vid(%d)\n", rgn, vid, vds_max_ime_ext_stamp, vds_max_ime_ext);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_ime_ext_stamp); 
			return NULL;
		}else if((int)ime_dev >= vds_max_ime){
			DBG_ERR("invalid ime dev id(%d), max (%d)\n", (int)ime_dev, vds_max_ime);
			return NULL;
		}else if((int)ime_port >= vds_max_ime_ext){
			DBG_ERR("invalid ime outport(%d), max (%d)\n", (int)ime_port, vds_max_ime_ext);
			return NULL;
		}

		if(lock)
			*lock = LOCK_RESOURCE_IME_EXT_STAMP;

		return &(ime_ext_stamps[vds_max_ime_ext_stamp*vid + rgn]);
	}
	else if(phase == VDS_PHASE_IME_EXT_MASK){
		ime_dev = ((vid & 0xFFFF0000) >> 16);
		ime_port = (vid & 0x0FFFF);
		vid = (ime_dev * vds_max_ime_ext + ime_port);

		if((int)rgn >= vds_max_ime_ext_mask){
			DBG_ERR("invalid ime ext mask(%d) vid(%d). max ext mask(%d) vid(%d)\n", rgn, vid, vds_max_ime_ext_mask, vds_max_ime_ext);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_ime_ext_mask); 
			return NULL;
		}else if((int)ime_dev >= vds_max_ime){
			DBG_ERR("invalid ime dev id(%d), max (%d)\n", (int)ime_dev, vds_max_ime);
			return NULL;
		}else if((int)ime_port >= vds_max_ime_ext){
			DBG_ERR("invalid ime outport(%d), max (%d)\n", (int)ime_port, vds_max_ime_ext);
			return NULL;
		}

		if(lock)
			*lock = LOCK_RESOURCE_IME_EXT_MASK;

		return &(ime_ext_masks[vds_max_ime_ext_mask*vid + rgn]);
	}
	else if(phase == VDS_PHASE_COE_STAMP){
		if((int)rgn >= vds_max_coe_stamp){
			DBG_ERR("invalid coe stamp(%d) vid(%d). max stamp(%d) vid(%d)\n", rgn, vid, vds_max_coe_stamp, vds_max_coe_vid);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_coe_stamp); 
			return NULL;
		}else if((int)vid >= vds_max_coe_vid){
			DBG_ERR("invalid coe stamp(%d) vid(%d). max stamp(%d) vid(%d)\n", rgn, vid, vds_max_coe_stamp, vds_max_coe_vid);
			DBG_ERR("port_id=%d is larger then max=%d!\r\n", vid, vds_max_coe_vid); 
			return NULL;
		}

		if(lock)
			*lock = (LOCK_RESOURCE_ENC_COE_STAMP | LOCK_RESOURCE_ENC_JPG_STAMP);

		return &(coe_stamps[vds_max_coe_stamp*vid + rgn]);
	}
	else if(phase == VDS_PHASE_COE_EXT_STAMP){
		if((int)rgn >= vds_max_coe_ext_stamp){
			DBG_ERR("invalid coe ext stamp(%d) vid(%d). max ext stamp(%d) vid(%d)\n", rgn, vid, vds_max_coe_ext_stamp, vds_max_coe_ext_vid);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_coe_ext_stamp); 
			return NULL;
		}else if((int)vid >= vds_max_coe_ext_vid){
			DBG_ERR("invalid coe ext stamp(%d) vid(%d). max ext stamp(%d) vid(%d)\n", rgn, vid, vds_max_coe_ext_stamp, vds_max_coe_ext_vid);
			DBG_ERR("port_id=%d is larger then max=%d!\r\n", vid, vds_max_coe_ext_vid); 
			return NULL;
		}

		if(lock)
			*lock = LOCK_RESOURCE_ENC_EXT_STAMP;

		return &(coe_ext_stamps[vds_max_coe_ext_stamp*vid + rgn]);
	}
	else if(phase == VDS_PHASE_COE_EXT_MASK){
		if((int)rgn >= vds_max_coe_ext_mask){
			DBG_ERR("invalid coe ext mask(%d) vid(%d). max ext mask(%d) vid(%d)\n", rgn, vid, vds_max_coe_ext_mask, vds_max_coe_ext_vid);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_coe_ext_mask); 
			return NULL;
		}else if((int)vid >= vds_max_coe_ext_vid){
			DBG_ERR("invalid coe ext mask(%d) vid(%d). max ext mask(%d) vid(%d)\n", rgn, vid, vds_max_coe_ext_mask, vds_max_coe_ext_vid);
			DBG_ERR("port_id=%d is larger then max=%d!\r\n", vid, vds_max_coe_ext_vid); 
			return NULL;
		}

		if(lock)
			*lock = LOCK_RESOURCE_ENC_EXT_MASK;

		return &(coe_ext_masks[vds_max_coe_ext_mask*vid + rgn]);
	}
	else if(phase == VDS_PHASE_VO_STAMP){
		if((int)rgn >= vds_max_vo_stamp){
			DBG_ERR("invalid vo stamp(%d) vid(%d). max vo(%d) vid(%d)\n", rgn, vid, vds_max_vo_stamp, vds_max_vo_vid);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_vo_stamp); 
			return NULL;
		}else if((int)vid >= vds_max_vo_vid){
			DBG_ERR("invalid vo stamp(%d) vid(%d). max vo(%d) vid(%d)\n", rgn, vid, vds_max_vo_stamp, vds_max_vo_vid);
			DBG_ERR("port_id=%d is larger then max=%d!\r\n", vid, vds_max_vo_vid); 
			return NULL;
		}

		if(lock)
			*lock = LOCK_RESOURCE_VO_STAMP;

		return &(vo_stamps[vds_max_vo_stamp*vid + rgn]);
	}
	else if(phase == VDS_PHASE_VO_MASK){
		if((int)rgn >= vds_max_vo_mask){
			DBG_ERR("invalid vo mask(%d) vid(%d). max vo mask(%d) vid(%d)\n", rgn, vid, vds_max_vo_mask, vds_max_vo_vid);
			DBG_ERR("path_id=%d is larger then max=%d!\r\n", rgn, vds_max_vo_mask); 
			return NULL;
		}else if((int)vid >= vds_max_vo_vid){
			DBG_ERR("invalid vo mask(%d) vid(%d). max vo mask(%d) vid(%d)\n", rgn, vid, vds_max_vo_mask, vds_max_vo_vid);
			DBG_ERR("port_id=%d is larger then max=%d!\r\n", vid, vds_max_vo_vid); 
			return NULL;
		}

		if(lock)
			*lock = LOCK_RESOURCE_VO_MASK;

		return &(vo_masks[vds_max_vo_mask*vid + rgn]);
	}
	else
		DBG_ERR("invalid phase(%d)\n", phase);

	return NULL;
}

static int vds_open(VDS_PHASE phase, UINT32 pid, UINT32 vid)
{
	int     ret;
	UINT32  lock;
	VDS_RGN *rgn;

	vds_api_lock();

	rgn = find_region(phase, pid, vid, &lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_start(VDS_PHASE phase, UINT32 pid, UINT32 vid)
{
	int     ret;
	UINT32  lock, ime_flag = 0;
	VDS_RGN *rgn;

	vds_api_lock();

	rgn = find_region(phase, pid, vid, &lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	if(!rgn->valid){
		DBG_ERR("region(phase:%d,pid:%d,vid:%d) has empty attribute\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_STATE;
		goto out;
	}

	if(phase == VDS_PHASE_IME_STAMP || phase == VDS_PHASE_IME_EXT_STAMP ||
	   phase == VDS_PHASE_COE_STAMP || phase == VDS_PHASE_COE_EXT_STAMP ||
	   phase == VDS_PHASE_VO_STAMP){
		if(phase != VDS_PHASE_COE_STAMP || !rgn->is_enc_mask){
			if(!rgn->buf || !rgn->buf->valid){
				DBG_ERR("region(phase:%d,pid:%d,vid:%d) has empty buffer\r\n", phase, pid, vid);
				ret = ISF_ERR_INVALID_STATE;
				goto out;
			}
			if(!rgn->buf->img_size[0].w || !rgn->buf->img_size[0].h){
				DBG_ERR("region(phase:%d,pid:%d,vid:%d) has empty bitmap\r\n", phase, pid, vid);
				ret = ISF_ERR_INVALID_STATE;
				goto out;
			}
		}
	}

	vds_res_lock(lock, &ime_flag);
	rgn->en = 1;
	vds_res_unlock(lock, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_stop(VDS_PHASE phase, UINT32 pid, UINT32 vid)
{
	int     ret;
	UINT32  lock, ime_flag = 0;
	VDS_RGN *rgn;

	vds_api_lock();

	rgn = find_region(phase, pid, vid, &lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	vds_res_lock(lock, &ime_flag);
	rgn->en = 0;
	vds_res_unlock(lock, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_close(VDS_PHASE phase, UINT32 pid, UINT32 vid)
{
	int     ret;
	UINT32  lock, ime_flag = 0;
	VDS_RGN *rgn;
	VDS_BUF *buf;

	vds_api_lock();

	rgn = find_region(phase, pid, vid, &lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	buf = rgn->buf;

	vds_res_lock(lock, &ime_flag);
	vds_memset(rgn, 0, sizeof(VDS_RGN));
	vds_res_unlock(lock, ime_flag);

	if(buf){
		if(buf->ref == 0 || buf->ref == 1)
			vds_memset(buf, 0, sizeof(VDS_BUF));
		else
			--(buf->ref);
	}

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int setup_vds_buf(VDS_BUF *buf, VDS_BUF_TYPE type, UINT32 size, UINT32 p_addr)
{
	if(!buf){
		DBG_ERR("buf is NULL\n");
		return ISF_ERR_INVALID_PORT_ID;
	}

	vds_memset(buf, 0, sizeof(VDS_BUF));

	if(type == VDS_BUF_TYPE_SINGE){
		buf->p_addr[0] = p_addr;
		buf->size[0]   = size;
		buf->p_addr[1] = p_addr;
		buf->size[1]   = size;
	}
	else if(type == VDS_BUF_TYPE_PING_PONG){
		buf->p_addr[0] = p_addr;
		buf->size[0]   = (size >> 1);
		buf->p_addr[1] = (buf->p_addr[0] + buf->size[0]);
		buf->size[1]   = buf->size[0];
	}
	else{
		DBG_ERR("unknown buf type(%d)\n", type);
		return ISF_ERR_NOT_SUPPORT;
	}

	if(!VOS_IS_ALIGNED(buf->p_addr[0]) ||
	   !VOS_IS_ALIGNED(buf->p_addr[1]) ||
	   !VOS_IS_ALIGNED(buf->size[0]) ||
	   !VOS_IS_ALIGNED(buf->size[1])){

	   DBG_ERR("addr(%x,%x) size(%d,%d) is not cpu cache aligned\n", buf->p_addr[0], buf->p_addr[1], buf->size[0], buf->size[1]);
	   vds_memset(buf, 0, sizeof(VDS_BUF));
	   return ISF_ERR_INVALID_VALUE;
	}

	buf->type = type;
	buf->ref = 1;
	buf->valid = 1;

	return 0;
}

static VDS_BUF* alloc_vds_buf(VDS_BUF_TYPE type, UINT32 size, UINT32 p_addr)
{
	int i, ret;
	VDS_BUF *buf = NULL;

	for(i = 0 ; i < stamp_img_cnt ; ++i){
		if(!stamp_imgs[i].valid){
			buf = &(stamp_imgs[i]);
			break;
		}
	}

	if(!buf){
		DBG_ERR("run out of VDS_BUF\n");
		DBG_ERR("hd_osg runs out of stamp buffer. please increase stamp_maximg defined in device tree's nvt-na51055-mem-tbl.dtsi.(e.g. set stamp_maximg to the number of all osg images)\n");
		return NULL;
	}

	ret = setup_vds_buf(buf, type, size, p_addr);
	if(ret){
		DBG_ERR("setup_vds_buf(%d,%d,%x fail\n", type, size, p_addr);
		vds_memset(buf, 0, sizeof(VDS_BUF));
		return NULL;
	}

	return buf;
}

static VDS_BUF* find_vds_buf(VDS_BUF_TYPE type, UINT32 size, UINT32 p_addr)
{
	int i;

	for(i = 0 ; i < stamp_img_cnt ; ++i){
		if(!stamp_imgs[i].valid)
			continue;
		if(stamp_imgs[i].type != type)
			continue;

		if(type == VDS_BUF_TYPE_SINGE && stamp_imgs[i].size[0] == size && stamp_imgs[i].p_addr[0] == p_addr)
			return &(stamp_imgs[i]);
		if(type == VDS_BUF_TYPE_PING_PONG && (stamp_imgs[i].size[0] << 1) == size && stamp_imgs[i].p_addr[0] == p_addr)
			return &(stamp_imgs[i]);
	}

	return NULL;
}

static VDS_BUF* find_or_alloc_vds_buf(VDS_BUF_TYPE type, UINT32 size, UINT32 p_addr)
{
	VDS_BUF *buf = find_vds_buf(type, size, p_addr);

	if(buf){
		++(buf->ref);
		return buf;
	}

	return alloc_vds_buf(type, size, p_addr);
}

static int vds_set_stamp_buf(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_BUF_TYPE type, UINT32 size, UINT32 p_addr)
{
	int     ret  = ISF_ERR_INVALID_VALUE;
	UINT32  lock, ime_flag = 0;
	VDS_RGN *rgn;
	VDS_BUF *buf, temp;
	UINT32  va;

	va = nvtmpp_sys_pa2va(p_addr);
	if(!va){
		DBG_ERR("fail to map block(%d)\n", p_addr);
		return ret;
	}

	vds_api_lock();

	rgn = find_region(phase, pid, vid, &lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	buf = rgn->buf;
	vds_memset(&temp, 0, sizeof(VDS_BUF));

	//if first configure, try to re-use existing VDS_BUF if any. Or allocate a new VDS_BUF
	if(!buf)
		buf = find_or_alloc_vds_buf(type, size, va);
	//The VDS_BUF is not shared, try to find existing VDS_BUF with identical addr and size. Or allocate a new VDS_BUF
	else if(buf->ref == 1){
		vds_memset(buf, 0, sizeof(VDS_BUF));
		buf = find_or_alloc_vds_buf(type, size, va);
	} else if(buf->p_addr[0] == va) { //The VDS_BUF is shared and new addr/size are identical to existing VDS_BUF, do nothing, keep using existing VDS_BUF
		//check size is identical
		if (buf->type == VDS_BUF_TYPE_PING_PONG) {
			if ((buf->size[0] + buf->size[1]) != size) {
				DBG_ERR("pingpong share buffer can't be re-setup with different addr or size\n");
				DBG_ERR("new addr(0x%x) size(%d), old addr(0x%x) size(%d)\n", va, size, buf->p_addr[0], buf->size[0] + buf->size[1]);
				ret = ISF_ERR_INVALID_VALUE;
				goto out;
			}
		} else {
			if (buf->size[0] != size) {
				DBG_ERR("single share buffer can't be re-setup with different addr or size\n");
				DBG_ERR("new addr(0x%x) size(%d), old addr(0x%x) size(%d)\n", va, size, buf->p_addr[0], buf->size[0]);
				ret = ISF_ERR_INVALID_VALUE;
				goto out;
			}
		}
	} else { //The VDS_BUF is shared, and different addr/size are requested, find or allocate VDS_BUF
		buf->ref--;
		buf = find_or_alloc_vds_buf(type, size, va);
	}

	if(!buf){
		DBG_ERR("fail to allocate VDS_BUF for phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	vds_res_lock(lock, &ime_flag);
	rgn->buf = buf;
	if(temp.valid)
		vds_memcpy(rgn->buf, &temp, sizeof(VDS_BUF));
	vds_res_unlock(lock, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_get_stamp_buf(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_BUF_TYPE *type, UINT32 *size, UINT32 *p_addr)
{
	int     ret;
	VDS_RGN *rgn;
	VDS_BUF *buf;

	vds_api_lock();

	rgn = find_region(phase, pid, vid, NULL);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	buf = rgn->buf;
	if(!buf){
		DBG_ERR("NULL VDS_BUF for phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_STATE;
		goto out;
	}

	if(size){
		if(buf->type == VDS_BUF_TYPE_SINGE)
			*size = buf->size[0];
		else if(buf->type == VDS_BUF_TYPE_PING_PONG)
			*size = (buf->size[0] + buf->size[1]);
		else{
			DBG_ERR("unknown buf type(%d)\n", buf->type);
			ret = ISF_ERR_NOT_SUPPORT;
			goto out;
		}
	}

	if(type)
		*type = buf->type;

	if(p_addr)
		*p_addr = nvtmpp_sys_va2pa(buf->p_addr[0]);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_set_stamp_img(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_IMG_FMT fmt, UINT32 w, UINT32 h, UINT32 p_data)
{
	int ret = ISF_ERR_INVALID_VALUE;
	VDS_RGN *rgn;
	VDS_BUF *buf;
	int     size;
	UINT32  ime_flag;

	vds_api_lock();

	rgn = find_region(phase, pid, vid, NULL);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	buf = rgn->buf;
	if(!buf){
		DBG_ERR("NULL VDS_BUF for phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_STATE;
		goto out;
	}

	if(!w || !h || !p_data){
		DBG_ERR("invalid bitmap : w(%d) h(%d) p_data(%p) for phase(%d) pid(%d) vid(%d)\n",
			w, h, (void*)p_data, phase, pid, vid);
		goto out;
	}

	if(fmt == VDS_IMG_FMT_PICTURE_PALETTE1){
		if ((w>>3) & 0x1) {
			DBG_ERR("palette1 width(%d) not 2 aligned after shift 3\n", w);
			goto out;
		}
		size = ((w>>3) * h);
	}else if(fmt == VDS_IMG_FMT_PICTURE_PALETTE2){
		if ((w>>2) & 0x1) {
			DBG_ERR("palette1 width(%d) not 2 aligned after shift 2\n", w);
			goto out;
		}
		size = ((w>>2) * h);
	}else if(fmt == VDS_IMG_FMT_PICTURE_PALETTE4){
		if ((w>>1) & 0x1) {
			DBG_ERR("palette1 width(%d) not 2 aligned after shift 1\n", w);
			goto out;
		}
		size = ((w>>1) * h);
	}else{
		if (w & 0x1) {
			DBG_ERR("stamp width(%d) not 2 aligned\n", w);
			goto out;
		}
		if (fmt == VDS_IMG_FMT_PICTURE_ARGB8888) {
			size = (w * h * 4);
		} else {
			size = (w * h * 2);
		}
	}

	if((int)buf->size[buf->swap] < size){
		DBG_ERR("bitmap too large : w(%d) h(%d) vs buf size(%d) for phase(%d) pid(%d) vid(%d)\n",
			w, h, buf->size[buf->swap], phase, pid, vid);
		ret = ISF_ERR_COUNT_OVERFLOW;
		goto out;
	}

	//if single buffer, temporarily disable all stamps
	if(buf->type == VDS_BUF_TYPE_SINGE){
		vds_res_lock(LOCK_RESOURCE_ALL, &ime_flag);
		buf->valid = 0;
		vds_res_unlock(LOCK_RESOURCE_ALL, ime_flag);
	}

	//if p_data is the same with registered buffer,
	//it means programmers directly renders images on the registered buffer
	//no memory copy is required. simply swap buffer
	//otherwise it means p_data is a user mode buffer, memory copy is required
	if(p_data != nvtmpp_sys_va2pa(buf->p_addr[0]) && p_data != nvtmpp_sys_va2pa(buf->p_addr[1]))
		#if defined(__LINUX)
		if(vds_copy_from_user((void*)buf->p_addr[buf->swap], (void*)p_data, size)){
			DBG_ERR("vds_copy_from_user() fail\r\n");
			ret = ISF_ERR_COPY_FROM_USER;
			goto out;
		}
		#else
		vds_memcpy((void*)buf->p_addr[buf->swap], (void*)p_data, size);
		#endif
		
	vos_cpu_dcache_sync((VOS_ADDR)buf->p_addr[buf->swap], size, VOS_DMA_TO_DEVICE);

	vds_res_lock(LOCK_RESOURCE_ALL, &ime_flag);
	buf->img_size[buf->swap].w    = w;
	buf->img_size[buf->swap].h    = h;
	buf->fmt[buf->swap]           = fmt;
	buf->swap                     = (buf->swap ? 0 : 1);
	buf->valid                    = 1;
	buf->dirty_for_get            = 1;
	vds_res_unlock(LOCK_RESOURCE_ALL, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_get_stamp_img(VDS_PHASE phase, UINT32 pid, UINT32 vid,
								VDS_IMG_FMT *fmt, UINT32 *w, UINT32 *h, UINT32 *p_addr, VDS_COPY_HW copy)
{
	int     ret  = ISF_ERR_INVALID_VALUE;
	VDS_RGN *rgn;
	VDS_BUF *buf;
	void    *src, *dst;

	vds_api_lock();

	rgn = find_region(phase, pid, vid, NULL);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	buf = rgn->buf;
	if(!buf){
		DBG_ERR("NULL VDS_BUF for phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_STATE;
		goto out;
	}

	if(fmt)
		*fmt = buf->fmt[buf->swap];

	if(w)
		*w = buf->img_size[buf->swap].w;

	if(h)
		*h = buf->img_size[buf->swap].h;

	if(p_addr)
		*p_addr = nvtmpp_sys_va2pa(buf->p_addr[buf->swap]);

	if(buf->dirty_for_get == 0)
		ret = ISF_OK;
	else if(copy == VDS_COPY_HW_CPU || copy == VDS_COPY_HW_DMA){
		if(buf->type != VDS_BUF_TYPE_PING_PONG){
			DBG_ERR("Only ping pong buffer can copy image to user space\n");
			ret = ISF_ERR_INVALID_STATE;
			goto out;
		}
		if(!buf->p_addr[0] || !buf->p_addr[1] || !buf->size[0] || !buf->size[1]){
			DBG_ERR("invalid ping pong buffer : addr1(%x) addr2(%x) size1(%d) size2(%d)\n",
				buf->p_addr[0], buf->p_addr[1], buf->size[0], buf->size[1]);
			ret = ISF_ERR_INVALID_STATE;
			goto out;
		}

		src = buf->swap ? (void*)buf->p_addr[0] : (void*)buf->p_addr[1];
		dst = buf->swap ? (void*)buf->p_addr[1] : (void*)buf->p_addr[0];
		if(copy == VDS_COPY_HW_CPU){
			vds_memcpy(dst, src, buf->size[0]);
			ret = ISF_OK;
		}
		else if(copy == VDS_COPY_HW_DMA)
			DBG_ERR("dma copy is not supported\n");

		buf->dirty_for_get = 0;
	}
	else
		ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static INT32 check_encoder_osd_overlap(UINT32 pid, UINT32 vid, VDS_STAMP_ATTR *attr) {
	
	struct overlap_s{
		int en;
		int x;
		int y;
		int w;
		int h;
	};

	UINT32 i = 0;
	UINT32 j = 0;
	UINT32 k = 0;
	UINT32 pointx1, pointx2, pointy1, pointy2;
	UINT32 mb_st_x[2], mb_end_x[2], mb_st_y[2], mb_end_y[2];
	struct overlap_s overlap[32] = { 0 };
	int index, swap;
	VDS_RGN *rgn, *p_rgn;
	VDS_BUF         *p_buf;
	
	//sanity check	
	if(!attr){
		DBG_ERR("attr is NULL\n");
		return 1;
	}
		
	if(vds_max_coe_stamp == 0 || (int)vid >= vds_max_coe_vid || !coe_stamps){
		DBG_ERR("vds_max_coe_stamp(%d) vid(%d) coe_stamps(%p)\n", vds_max_coe_stamp, vds_max_coe_vid, coe_stamps);
		return 1;
	}

	//prepare overlap[] array for existing stamp
	p_rgn = &(coe_stamps[vds_max_coe_stamp * vid]);
	for(i = 0 ; (int)i < vds_max_coe_stamp ; ++i){
		if(!p_rgn[i].en || !p_rgn[i].valid || !p_rgn[i].buf || !p_rgn[i].buf->valid)
			continue;
		if(i == pid)
			continue;

		p_buf = p_rgn[i].buf;
		swap = (p_buf->swap ? 0 : 1);

		if(!p_buf->p_addr[swap] || !p_buf->img_size[swap].w || !p_buf->img_size[swap].h)
			continue;
			
		if(p_rgn[i].attr.stamp.layer == attr->layer && p_rgn[i].attr.stamp.region == attr->region){
			DBG_ERR("overlapped layer(%d) region(%d)\n", attr->layer, attr->region);
			return 1;
		}
			
		if(p_rgn[i].attr.stamp.layer >= 2 || p_rgn[i].attr.stamp.region >= 16){
			DBG_ERR("invalid layer(%d) region(%d)\n", p_rgn[i].attr.stamp.layer, p_rgn[i].attr.stamp.region);
			return 1;
		}

		index = ((p_rgn[i].attr.stamp.layer*16) + p_rgn[i].attr.stamp.region);
		overlap[index].en = 1;
		overlap[index].w  = p_buf->img_size[swap].w;
		overlap[index].h  = p_buf->img_size[swap].h;
		overlap[index].x  = p_rgn[i].attr.stamp.x;
		overlap[index].y  = p_rgn[i].attr.stamp.y;
	}
	
	//insert new stamp to overlap[] array
	rgn = find_region(VDS_PHASE_COE_STAMP, pid, vid, NULL);
	if(!rgn){
		DBG_ERR("invalid VDS_PHASE_COE_STAMP pid(%d) vid(%d)\n", pid, vid);
		return 1;
	}
	if(!rgn->buf || !rgn->buf->valid)
		return 1;

	p_buf = rgn->buf;
	swap = (p_buf->swap ? 0 : 1);

	if(!p_buf->p_addr[swap] || !p_buf->img_size[swap].w || !p_buf->img_size[swap].h)
		return 1;
	if(attr->layer >= 2 || attr->region >= 16){
		DBG_ERR("invalid layer(%d) region(%d)\n", attr->layer, attr->region);
		return 1;
	}

	index = ((attr->layer*16) + attr->region);
	overlap[index].en = 1;
	overlap[index].w  = p_buf->img_size[swap].w;
	overlap[index].h  = p_buf->img_size[swap].h;
	overlap[index].x  = attr->x;
	overlap[index].y  = attr->y;

	//check overlap[] array for overlapping
	{
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 16; j++) {
				if (overlap[j+i*16].en == 1) {
					pointx1 = overlap[j+i*16].x;
					pointx2 = pointx1 + overlap[j+i*16].w - 1;
					pointy1 = overlap[j+i*16].y;
					pointy2 = pointy1 + overlap[j+i*16].h - 1;

					mb_st_x[0] = pointx1 / 16;
					mb_end_x[0] = pointx2 / 16;
					mb_st_y[0] = pointy1 / 16;
					mb_end_y[0] = pointy2 / 16;

					for (k = j+1; k < 16; k++) {
						if (overlap[k+i*16].en == 1) {
							pointx1 = overlap[k+i*16].x;
							pointx2 = pointx1 + overlap[k+i*16].w - 1;
							pointy1 = overlap[k+i*16].y;
							pointy2 = pointy1 + overlap[k+i*16].h - 1;

							mb_st_x[1] = pointx1 / 16;
							mb_end_x[1] = pointx2 / 16;
							mb_st_y[1] = pointy1 / 16;
							mb_end_y[1] = pointy2 / 16;

							if (((mb_st_x[1] >= mb_st_x[0]) && (mb_st_x[1] <= mb_end_x[0])) || ((mb_end_x[1] >= mb_st_x[0]) && (mb_end_x[1] <= mb_end_x[0]))) {
								if (((mb_st_y[1] >= mb_st_y[0]) && (mb_st_y[1] <= mb_end_y[0])) || ((mb_end_y[1] >= mb_st_y[0]) && (mb_end_y[1] <= mb_end_y[0]))) {
									DBG_ERR("vencoder OSD are overlapping! layer%d, oldregion=%d, newregion=%d\r\n", (int)i, (int)j, (int)k);
									DBG_ERR("newregion:x(%d),y(%d),w(%d),h(%d)\r\n", 
												(int)overlap[k+i*16].x,(int)overlap[k+i*16].y,(int)overlap[k+i*16].w,(int)overlap[k+i*16].h);
									DBG_ERR("oldregion:x(%d),y(%d),w(%d),h(%d)\r\n", 
												(int)overlap[j+i*16].x,(int)overlap[j+i*16].y,(int)overlap[j+i*16].w,(int)overlap[j+i*16].h);
									return 1;
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

static int vds_set_stamp_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_STAMP_ATTR *attr)
{
	int ret = ISF_ERR_INVALID_VALUE, lock;
	VDS_RGN *rgn;
	UINT32  ime_flag = 0;
	
	//below qp and gcac fields are set by other API
	//this API should not touch them
	UINT8   lpm_mode;
	UINT8   tnr_mode;
	UINT8   fro_mode;
	UINT8   blk_num;
	UINT8   org_color_level;
	UINT8   inv_color_level;
	UINT8   nor_diff_th;
	UINT8   inv_diff_th;
	UINT8   sta_only_mode;
	UINT8   full_eval_mode;
	UINT8   eval_lum_targ;

	vds_api_lock();

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, (UINT32 *)&lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	vds_res_lock(lock, &ime_flag);
	
	if(phase == VDS_PHASE_COE_STAMP && check_encoder_osd_overlap(pid, vid, attr)){
		DBG_ERR("overlap check fail\n");
		ret = ISF_ERR_NOT_AVAIL;
	}else{
		lpm_mode        = rgn->attr.stamp.qp_lpm_mode;
		tnr_mode        = rgn->attr.stamp.qp_tnr_mode;
		fro_mode        = rgn->attr.stamp.qp_fro_mode;
		blk_num         = rgn->attr.stamp.gcac_blk_num;
		org_color_level = rgn->attr.stamp.gcac_org_color_level;
		inv_color_level = rgn->attr.stamp.gcac_inv_color_level;
		nor_diff_th     = rgn->attr.stamp.gcac_nor_diff_th;
		inv_diff_th     = rgn->attr.stamp.gcac_inv_diff_th;
		sta_only_mode   = rgn->attr.stamp.gcac_sta_only_mode;
		full_eval_mode  = rgn->attr.stamp.gcac_full_eval_mode;
		eval_lum_targ   = rgn->attr.stamp.gcac_eval_lum_targ;
		
		vds_memcpy(&(rgn->attr.stamp), attr, sizeof(VDS_STAMP_ATTR));

		rgn->attr.stamp.qp_lpm_mode          = lpm_mode;
		rgn->attr.stamp.qp_tnr_mode          = tnr_mode;
		rgn->attr.stamp.qp_fro_mode          = fro_mode;
		rgn->attr.stamp.gcac_blk_num         = blk_num;
		rgn->attr.stamp.gcac_org_color_level = org_color_level;
		rgn->attr.stamp.gcac_inv_color_level = inv_color_level;
		rgn->attr.stamp.gcac_nor_diff_th     = nor_diff_th;
		rgn->attr.stamp.gcac_inv_diff_th     = inv_diff_th;
		rgn->attr.stamp.gcac_sta_only_mode   = sta_only_mode;
		rgn->attr.stamp.gcac_full_eval_mode  = full_eval_mode;
		rgn->attr.stamp.gcac_eval_lum_targ   = eval_lum_targ;
		
		rgn->valid       = 1;
		rgn->is_enc_mask = 0;

		ret = ISF_OK;
	}
		
	vds_res_unlock(lock, ime_flag);

out:

	vds_api_unlock();

	return ret;
}

static int vds_get_stamp_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_STAMP_ATTR *attr)
{
	int ret = ISF_ERR_INVALID_VALUE;
	VDS_RGN *rgn;

	vds_api_lock();

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, NULL);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	if(attr)
		vds_memcpy(attr, &(rgn->attr.stamp), sizeof(VDS_STAMP_ATTR));

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_set_mask_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_MASK_ATTR *attr)
{
	int ret = ISF_ERR_INVALID_VALUE, lock;
	VDS_RGN *rgn;
	UINT32  ime_flag = 0;

	vds_api_lock();

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, (UINT32 *)&lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	vds_res_lock(lock, &ime_flag);
	vds_memcpy(&(rgn->attr.mask), attr, sizeof(VDS_MASK_ATTR));
	rgn->is_mosaic = 0;
	rgn->valid = 1;
	vds_res_unlock(lock, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_get_mask_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_MASK_ATTR *attr)
{
	int ret = ISF_ERR_INVALID_VALUE;
	VDS_RGN *rgn;

	vds_api_lock();

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, NULL);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	if(attr)
		vds_memcpy(attr, &(rgn->attr.mask), sizeof(VDS_MASK_ATTR));

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_set_mosaic_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_MOSAIC_ATTR *attr)
{
	int ret = ISF_ERR_INVALID_VALUE, lock;
	VDS_RGN *rgn;
	UINT32  ime_flag = 0;

	vds_api_lock();

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, (UINT32 *)&lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	vds_res_lock(lock, &ime_flag);
	vds_memcpy(&(rgn->attr.mosaic), attr, sizeof(VDS_MOSAIC_ATTR));
	rgn->is_mosaic = 1;
	rgn->valid = 1;
	vds_res_unlock(lock, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_get_mosaic_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_MOSAIC_ATTR *attr)
{
	int ret = ISF_ERR_INVALID_VALUE;
	VDS_RGN *rgn;

	vds_api_lock();

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, NULL);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	if(attr)
		vds_memcpy(attr, &(rgn->attr.mosaic), sizeof(VDS_MOSAIC_ATTR));

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_set_cfg_max(VDS_CFG_MAX *cfg)
{
	int ret = ISF_ERR_INVALID_VALUE;
	
	vds_api_lock();
	
	if(!cfg){
		DBG_ERR("cfg is NULL\n");
		goto out;
	}

	//subsequent _vds_init() calls vmalloc() and sleeps, so don't lock ime because it's spin lock
	vds_res_lock(LOCK_RESOURCE_ALL & ~LOCK_RESOURCE_IME_MASK, NULL);
	
	_vds_reset();

	stamp_img_cnt         = cfg->max_stamp_img;
	vds_max_coe_vid       = cfg->max_enc_path;
	vds_max_coe_stamp     = cfg->max_enc_stamp[0];
	vds_max_coe_ext_vid   = cfg->max_enc_path;
	vds_max_coe_ext_stamp = cfg->max_enc_stamp[1];
	vds_max_coe_ext_mask  = cfg->max_enc_mask[1];
	vds_max_ime           = cfg->max_prc_path;
	vds_max_ime_stamp     = cfg->max_prc_stamp[0];
	vds_max_ime_mask      = cfg->max_prc_mask[0];
	vds_max_ime_ext       = vds_max_ime_mask * 2;
	vds_max_ime_ext_stamp = cfg->max_prc_stamp[1];
	vds_max_ime_ext_mask  = cfg->max_prc_mask[1];
	vds_max_vo_vid        = cfg->max_out_path;
	vds_max_vo_stamp      = cfg->max_out_stamp[1];
	vds_max_vo_mask       = cfg->max_out_mask[1];
	
	_vds_init();

	vds_res_unlock(LOCK_RESOURCE_ALL & ~LOCK_RESOURCE_IME_MASK, 0);

	ret = ISF_OK;
	
out:		
		
	vds_api_unlock();
	
	return ret;
}

static int vds_set_stamp_qp(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_QP *qp)
{
	int ret = ISF_ERR_INVALID_VALUE, lock;
	VDS_RGN *rgn;
	UINT32  ime_flag = 0;

	vds_api_lock();

	if(!qp){
		DBG_ERR("qp is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, (UINT32 *)&lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	vds_res_lock(lock, &ime_flag);
	rgn->attr.stamp.qp_lpm_mode = qp->lpm_mode;
	rgn->attr.stamp.qp_tnr_mode = qp->tnr_mode;
	rgn->attr.stamp.qp_fro_mode = qp->fro_mode;
	vds_res_unlock(lock, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_set_stamp_color_invert(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_COLOR_INVERT *invert)
{
	int ret = ISF_ERR_INVALID_VALUE, lock;
	VDS_RGN *rgn;
	UINT32  ime_flag = 0;

	vds_api_lock();

	if(!invert){
		DBG_ERR("invert is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, (UINT32 *)&lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	vds_res_lock(lock, &ime_flag);
	rgn->attr.stamp.gcac_blk_num         = invert->blk_num;
	rgn->attr.stamp.gcac_org_color_level = invert->org_color_level;
	rgn->attr.stamp.gcac_inv_color_level = invert->inv_color_level;
	rgn->attr.stamp.gcac_nor_diff_th     = invert->nor_diff_th;
	rgn->attr.stamp.gcac_inv_diff_th     = invert->inv_diff_th;
	rgn->attr.stamp.gcac_sta_only_mode   = invert->sta_only_mode;
	rgn->attr.stamp.gcac_full_eval_mode  = invert->full_eval_mode;
	rgn->attr.stamp.gcac_eval_lum_targ   = invert->eval_lum_targ;
	vds_res_unlock(lock, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_set_enc_mask_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_ENC_MASK *attr)
{
	int ret = ISF_ERR_INVALID_VALUE, lock;
	VDS_RGN *rgn;
	UINT32  ime_flag;

	vds_api_lock();
	
	if(phase != VDS_PHASE_COE_STAMP){
		DBG_ERR("vds_set_enc_mask_attr is only valid for coe stamp, not %d\n", phase);
		goto out;
	}

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	rgn = find_region(phase, pid, vid, (UINT32 *)&lock);
	if(!rgn){
		DBG_ERR("invalid phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);
		ret = ISF_ERR_INVALID_PORT_ID;
		goto out;
	}

	vds_res_lock(lock, &ime_flag);	
	vds_memcpy(&(rgn->attr.enc_mask), attr, sizeof(VDS_ENC_MASK));
	rgn->valid       = 1;
	rgn->is_enc_mask = 1;
	vds_res_unlock(lock, ime_flag);

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_set_palette_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_PALETTE *attr)
{
	int ret = ISF_ERR_INVALID_VALUE;

	vds_api_lock();

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	if(!attr->p_addr){
		DBG_ERR("attr->p_addr is NULL\n");
		goto out;
	}
	
	if(attr->size != 16 * sizeof(UINT32)){
		DBG_ERR("invalid table size(%d)\n", attr->size);
		goto out;
	}
	
	if(phase == VDS_PHASE_IME_STAMP || phase == VDS_PHASE_IME_EXT_STAMP){
		if(!ime_palette || !vds_max_ime){
			DBG_ERR("ime palette(%p) or channel(%d) is NULL\n", ime_palette, vds_max_ime);
			goto out;
		}
		if((int)vid >= vds_max_ime){
			DBG_ERR("ime vid(%d) >= channel(%d)\n", vid, vds_max_ime);
			goto out;
		}
		#if defined(__LINUX)
		if(vds_copy_from_user(&ime_palette[vid*16], (void*)attr->p_addr, attr->size)){
			DBG_ERR("vds_copy_from_user() fail\r\n");
			goto out;
		}
		#else
		vds_memcpy(&ime_palette[vid*16], (void*)attr->p_addr, attr->size);
		#endif
	}else if(phase == VDS_PHASE_COE_STAMP || phase == VDS_PHASE_COE_EXT_STAMP){
		if(!coe_palette || !vds_max_coe_vid){
			DBG_ERR("coe palette(%p) or vid(%d) is NULL\n", coe_palette, vds_max_coe_vid);
			goto out;
		}
		if((int)vid >= vds_max_coe_vid){
			DBG_ERR("coe vid(%d) >= vid(%d)\n", vid, vds_max_coe_vid);
			goto out;
		}
		#if defined(__LINUX)
		if(vds_copy_from_user(&coe_palette[vid*16], (void*)attr->p_addr, attr->size)){
			DBG_ERR("vds_copy_from_user() fail\r\n");
			goto out;
		}
		#else
		vds_memcpy(&coe_palette[vid*16], (void*)attr->p_addr, attr->size);
		#endif
	}else if(phase == VDS_PHASE_VO_STAMP){
		if(!vo_palette || !vds_max_vo_vid){
			DBG_ERR("vo palette(%p) or vid(%d) is NULL\n", vo_palette, vds_max_vo_vid);
			goto out;
		}
		if((int)vid >= vds_max_vo_vid){
			DBG_ERR("vo vid(%d) >= vid(%d)\n", vid, vds_max_vo_vid);
			goto out;
		}
		#if defined(__LINUX)
		if(vds_copy_from_user(&vo_palette[vid*16], (void*)attr->p_addr, attr->size)){
			DBG_ERR("vds_copy_from_user() fail\r\n");
			goto out;
		}
		#else
		vds_memcpy(&vo_palette[vid*16], (void*)attr->p_addr, attr->size);
		#endif
	}else{
		DBG_ERR("phase(%d) is not supported\n", phase);
		goto out;
	}

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

static int vds_get_palette_attr(VDS_PHASE phase, UINT32 pid, UINT32 vid, VDS_PALETTE *attr)
{
	int ret = ISF_ERR_INVALID_VALUE;

	vds_api_lock();

	if(!attr){
		DBG_ERR("attr is NULL\n");
		goto out;
	}

	if(!attr->p_addr){
		DBG_ERR("attr->p_addr is NULL\n");
		goto out;
	}
	
	if(attr->size != 16 * sizeof(UINT32)){
		DBG_ERR("invalid table size(%d)\n", attr->size);
		goto out;
	}
	
	if(phase == VDS_PHASE_IME_STAMP){
		if(!ime_palette || !vds_max_ime){
			DBG_ERR("ime palette(%p) or channel(%d) is NULL\n", ime_palette, vds_max_ime);
			goto out;
		}
		if((int)vid >= vds_max_ime){
			DBG_ERR("ime vid(%d) >= channel(%d)\n", vid, vds_max_ime);
			goto out;
		}
		#if defined(__LINUX)
		if(vds_copy_to_user((void*)attr->p_addr, &ime_palette[vid*16], attr->size)){
			DBG_ERR("vds_copy_to_user() fail\r\n");
			goto out;
		}
		#else
		vds_memcpy((void*)attr->p_addr, &ime_palette[vid*16], attr->size);
		#endif
	}else if(phase == VDS_PHASE_COE_STAMP){
		if(!coe_palette || !vds_max_coe_vid){
			DBG_ERR("coe palette(%p) or vid(%d) is NULL\n", coe_palette, vds_max_coe_vid);
			goto out;
		}
		if((int)vid >= vds_max_coe_vid){
			DBG_ERR("coe vid(%d) >= vid(%d)\n", vid, vds_max_coe_vid);
			goto out;
		}
		#if defined(__LINUX)
		if(vds_copy_to_user((void*)attr->p_addr, &coe_palette[vid*16], attr->size)){
			DBG_ERR("vds_copy_to_user() fail\r\n");
			goto out;
		}
		#else
		vds_memcpy((void*)attr->p_addr, &coe_palette[vid*16], attr->size);
		#endif
	}else if(phase == VDS_PHASE_VO_STAMP){
		if(!vo_palette || !vds_max_vo_vid){
			DBG_ERR("vo palette(%p) or vid(%d) is NULL\n", vo_palette, vds_max_vo_vid);
			goto out;
		}
		if((int)vid >= vds_max_vo_vid){
			DBG_ERR("vo vid(%d) >= vid(%d)\n", vid, vds_max_vo_vid);
			goto out;
		}
		#if defined(__LINUX)
		if(vds_copy_to_user((void*)attr->p_addr, &vo_palette[vid*16], attr->size)){
			DBG_ERR("vds_copy_to_user() fail\r\n");
			goto out;
		}
		#else
		vds_memcpy((void*)attr->p_addr, &vo_palette[vid*16], attr->size);
		#endif
	}else{
		DBG_ERR("phase(%d) is not supported\n", phase);
		goto out;
	}

	ret = ISF_OK;

out:

	vds_api_unlock();

	return ret;
}

void* vds_lock_context(VDS_QUERY_STAGE stage, UINT32 id, UINT32 *ime_flag)
{
	int             i, num = 0, swap;
	void            *ret = NULL;
	VDS_RGN         *p_rgn;
	VDS_BUF         *p_buf;
	unsigned int    rgb_color;
	unsigned int    ime_dev, ime_port;
	UINT64          start = hwclock_get_longcounter();

	if(stage == VDS_QS_IME_STAMP){

		vds_res_lock(LOCK_RESOURCE_IME_STAMP, ime_flag);
		vds_save_latency(VDS_QS_IME_STAMP, id, 1, hwclock_get_longcounter() - start);

		if(ime_stamp.data && vds_max_ime_stamp && (int)id < vds_max_ime && ime_stamps){
			vds_memset(ime_stamp.data, 0, sizeof(VDS_INTERNAL_IME_STAMP) * vds_max_ime_stamp);
			p_rgn = &(ime_stamps[vds_max_ime_stamp * id]);
		}else
			return &ime_stamp;

		for(i = 0 ; i < vds_max_ime_stamp ; ++i){
			if(p_rgn[i].valid && p_rgn[i].en && p_rgn[i].buf && p_rgn[i].buf->valid){

				p_buf = p_rgn[i].buf;
				swap = (p_buf->swap ? 0 : 1);

				if(p_buf->p_addr[swap] && p_buf->img_size[swap].w && p_buf->img_size[swap].h){
					ime_stamp.dirty = 1;
					ime_stamp.data[num].en           = 1;
					ime_stamp.data[num].size.w       = p_buf->img_size[swap].w;
					ime_stamp.data[num].size.h       = p_buf->img_size[swap].h;
					ime_stamp.data[num].pos.x        = p_rgn[i].attr.stamp.x;
					ime_stamp.data[num].pos.y        = p_rgn[i].attr.stamp.y;
					ime_stamp.data[num].addr         = p_buf->p_addr[swap];
					ime_stamp.data[num].ckey_en      = p_rgn[i].attr.stamp.colorkey_en;
					ime_stamp.data[num].ckey_val     = p_rgn[i].attr.stamp.colorkey_val;

					if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_RGB565){
						ime_stamp.data[num].fmt      = VDO_PXLFMT_RGB565;
						ime_stamp.data[num].bweight0 = p_rgn[i].attr.stamp.fg_alpha;
						ime_stamp.data[num].bweight1 = p_rgn[i].attr.stamp.fg_alpha;
					}
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB1555){
						ime_stamp.data[num].fmt      = VDO_PXLFMT_ARGB1555;
						ime_stamp.data[num].bweight0 = (p_rgn[i].attr.stamp.fg_alpha & 0x0F);
						ime_stamp.data[num].bweight1 = ((p_rgn[i].attr.stamp.fg_alpha & 0x0F0) >> 4);
					}
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB4444){
						ime_stamp.data[num].fmt      = VDO_PXLFMT_ARGB4444;
						ime_stamp.data[num].bweight0 = p_rgn[i].attr.stamp.fg_alpha;
					}
					else if (p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB8888) {
						ime_stamp.data[num].fmt      = VDO_PXLFMT_ARGB8888;
						ime_stamp.data[num].bweight0 = (p_rgn[i].attr.stamp.fg_alpha & 0x0F);
						ime_stamp.data[num].bweight1 = ((p_rgn[i].attr.stamp.fg_alpha & 0x0F0) >> 4);
					}
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE1)
						ime_stamp.data[num].fmt      = VDO_PXLFMT_I1;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE2)
						ime_stamp.data[num].fmt      = VDO_PXLFMT_I2;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE4)
						ime_stamp.data[num].fmt      = VDO_PXLFMT_I4;
					else{
						DBG_ERR("VDS_QS_IME_STAMP : unknown format = %d\r\n", p_buf->fmt[swap]);
						break;
					}
				}
			}

			++num;
		}

		ret = (void*)&ime_stamp;

	}
	else if(stage == VDS_QS_IME_MASK){

		vds_res_lock(LOCK_RESOURCE_IME_MASK, ime_flag);
		vds_save_latency(VDS_QS_IME_MASK, id, 1, hwclock_get_longcounter() - start);

		if(ime_mask.data && vds_max_ime_mask && (int)id < vds_max_ime && ime_masks){
			vds_memset(ime_mask.data, 0, sizeof(VDS_INTERNAL_IME_MASK) * vds_max_ime_mask);
			p_rgn = &(ime_masks[vds_max_ime_mask * id]);
		}else
			return &ime_mask;

		for(i = 0 ; i < vds_max_ime_mask ; ++i){
			if(p_rgn[i].valid && p_rgn[i].en){

				ime_mask.dirty = 1;
				ime_mask.data[num].is_mosaic = p_rgn[i].is_mosaic;
				ime_mask.data[num].en = 1;

				if(p_rgn[i].is_mosaic){
					ime_mask.data[num].pos[0].x    = p_rgn[i].attr.mosaic.x[0];
					ime_mask.data[num].pos[0].y    = p_rgn[i].attr.mosaic.y[0];
					ime_mask.data[num].pos[1].x    = p_rgn[i].attr.mosaic.x[1];
					ime_mask.data[num].pos[1].y    = p_rgn[i].attr.mosaic.y[1];
					ime_mask.data[num].pos[2].x    = p_rgn[i].attr.mosaic.x[2];
					ime_mask.data[num].pos[2].y    = p_rgn[i].attr.mosaic.y[2];
					ime_mask.data[num].pos[3].x    = p_rgn[i].attr.mosaic.x[3];
					ime_mask.data[num].pos[3].y    = p_rgn[i].attr.mosaic.y[3];
					ime_mask.data[num].alpha       = 255;

					if(p_rgn[i].attr.mosaic.mosaic_blk_w <= 8)
						ime_mask.data[num].mosaic_blk_size = 8;
					else if(p_rgn[i].attr.mosaic.mosaic_blk_w > 8 && p_rgn[i].attr.mosaic.mosaic_blk_w <= 16)
						ime_mask.data[num].mosaic_blk_size = 16;
					else if(p_rgn[i].attr.mosaic.mosaic_blk_w > 16 && p_rgn[i].attr.mosaic.mosaic_blk_w <= 32)
						ime_mask.data[num].mosaic_blk_size = 32;
					else
						ime_mask.data[num].mosaic_blk_size = 64;
				}else{
					if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_SOLID)
						ime_mask.data[num].type      = VDS_INTERNAL_MASK_TYPE_SOLID;
					else if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_HOLLOW){
						ime_mask.data[num].type      = VDS_INTERNAL_MASK_TYPE_HOLLOW;
						ime_mask.data[num].thickness = p_rgn[i].attr.mask.thickness;
					}else{
						DBG_ERR("VDS_QS_IME_MASK : unknown type = %d\r\n", p_rgn[i].attr.mask.type);
						break;
					}
					rgb_color                      = p_rgn[i].attr.mask.color;
					ime_mask.data[num].pos[0].x    = p_rgn[i].attr.mask.data.normal.x[0];
					ime_mask.data[num].pos[0].y    = p_rgn[i].attr.mask.data.normal.y[0];
					ime_mask.data[num].pos[1].x    = p_rgn[i].attr.mask.data.normal.x[1];
					ime_mask.data[num].pos[1].y    = p_rgn[i].attr.mask.data.normal.y[1];
					ime_mask.data[num].pos[2].x    = p_rgn[i].attr.mask.data.normal.x[2];
					ime_mask.data[num].pos[2].y    = p_rgn[i].attr.mask.data.normal.y[2];
					ime_mask.data[num].pos[3].x    = p_rgn[i].attr.mask.data.normal.x[3];
					ime_mask.data[num].pos[3].y    = p_rgn[i].attr.mask.data.normal.y[3];
					ime_mask.data[num].color[0]    = RGB_GET_Y((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
					ime_mask.data[num].color[1]    = RGB_GET_U((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
					ime_mask.data[num].color[2]    = RGB_GET_V((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
					ime_mask.data[num].alpha       = p_rgn[i].attr.mask.data.normal.alpha;
				}
			}
			++num;
		}

		ret = (void*)&ime_mask;

	}
	else if(stage == VDS_QS_IME_EXT_STAMP){

		vds_res_lock(LOCK_RESOURCE_IME_EXT_STAMP, ime_flag);
		vds_save_latency(VDS_QS_IME_EXT_STAMP, id, 1, hwclock_get_longcounter() - start);

		ime_dev = ((id & 0xFFFF0000) >> 16);
		ime_port = (id & 0x0FFFF);
		id = (ime_dev * vds_max_ime_ext + ime_port);

		if(ime_ext_stamp.data.stamp && vds_max_ime_ext_stamp &&
		   (int)ime_dev < vds_max_ime && (int)ime_port < vds_max_ime_ext && ime_ext_stamps){
			vds_memset(ime_ext_stamp.data.stamp, 0, sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_ime_ext_stamp);
			p_rgn = &(ime_ext_stamps[vds_max_ime_ext_stamp * id]);
		}else
			return &ime_ext_stamp;

		for(i = 0 ; i < vds_max_ime_ext_stamp ; ++i)
			if(p_rgn[i].valid && p_rgn[i].en && p_rgn[i].buf && p_rgn[i].buf->valid){

				p_buf = p_rgn[i].buf;
				swap = (p_buf->swap ? 0 : 1);

				if(!p_buf->p_addr[swap] || !p_buf->img_size[swap].w || !p_buf->img_size[swap].h)
					continue;
				else
					ime_ext_stamp.dirty = 1;

				ime_ext_stamp.data.stamp[num].en        = 1;
				ime_ext_stamp.data.stamp[num].size.w    = p_buf->img_size[swap].w;
				ime_ext_stamp.data.stamp[num].size.h    = p_buf->img_size[swap].h;
				ime_ext_stamp.data.stamp[num].pos.x     = p_rgn[i].attr.stamp.x;
				ime_ext_stamp.data.stamp[num].pos.y     = p_rgn[i].attr.stamp.y;
				ime_ext_stamp.data.stamp[num].alpha     = p_rgn[i].attr.stamp.fg_alpha;
				ime_ext_stamp.data.stamp[num].addr      = p_buf->p_addr[swap];

				if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_RGB565)
					ime_ext_stamp.data.stamp[num].fmt = VDO_PXLFMT_RGB565;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB1555)
					ime_ext_stamp.data.stamp[num].fmt = VDO_PXLFMT_ARGB1555;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB4444)
					ime_ext_stamp.data.stamp[num].fmt = VDO_PXLFMT_ARGB4444;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB8888)
					ime_ext_stamp.data.stamp[num].fmt = VDO_PXLFMT_ARGB8888;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE1)
					ime_ext_stamp.data.stamp[num].fmt = VDO_PXLFMT_I1;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE2)
					ime_ext_stamp.data.stamp[num].fmt = VDO_PXLFMT_I2;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE4)
					ime_ext_stamp.data.stamp[num].fmt = VDO_PXLFMT_I4;
				else{
					DBG_ERR("VDS_QS_IME_OUT_RGN : unknown format = %d\r\n", p_buf->fmt[swap]);
					break;
				}

				++num;
			}

		ret = (void*)&ime_ext_stamp;

	}
	else if(stage == VDS_QS_IME_EXT_MASK){

		vds_res_lock(LOCK_RESOURCE_IME_EXT_MASK, ime_flag);
		vds_save_latency(VDS_QS_IME_EXT_MASK, id, 1, hwclock_get_longcounter() - start);

		ime_dev = ((id & 0xFFFF0000) >> 16);
		ime_port = (id & 0x0FFFF);
		id = (ime_dev * vds_max_ime_ext + ime_port);

		if(ime_ext_mask.data.mask && vds_max_ime_ext_mask &&
		   (int)ime_dev < vds_max_ime && (int)ime_port < vds_max_ime_ext && ime_ext_masks){
			vds_memset(ime_ext_mask.data.mask, 0, sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_ime_ext_mask);
			p_rgn = &(ime_ext_masks[vds_max_ime_ext_mask * id]);
		}else
			return &ime_ext_mask;

		for(i = 0 ; i < vds_max_ime_ext_mask ; ++i)
			if(p_rgn[i].valid && p_rgn[i].en){

				ime_ext_mask.dirty                      = 1;
				ime_ext_mask.data.mask[num].is_mosaic   = p_rgn[i].is_mosaic;
				ime_ext_mask.data.mask[num].en          = 1;

				rgb_color = p_rgn[i].attr.mask.color;
				ime_ext_mask.data.mask[num].color  = RGB_GET_V((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
				ime_ext_mask.data.mask[num].color <<= 8;
				ime_ext_mask.data.mask[num].color += RGB_GET_U((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
				ime_ext_mask.data.mask[num].color <<= 8;
				ime_ext_mask.data.mask[num].color += RGB_GET_Y((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);

				if(p_rgn[i].is_mosaic){
					ime_ext_mask.data.mask[num].data.normal.pos[0].x    = p_rgn[i].attr.mosaic.x[0];
					ime_ext_mask.data.mask[num].data.normal.pos[0].y    = p_rgn[i].attr.mosaic.y[0];
					ime_ext_mask.data.mask[num].data.normal.pos[1].x    = p_rgn[i].attr.mosaic.x[1];
					ime_ext_mask.data.mask[num].data.normal.pos[1].y    = p_rgn[i].attr.mosaic.y[1];
					ime_ext_mask.data.mask[num].data.normal.pos[2].x    = p_rgn[i].attr.mosaic.x[2];
					ime_ext_mask.data.mask[num].data.normal.pos[2].y    = p_rgn[i].attr.mosaic.y[2];
					ime_ext_mask.data.mask[num].data.normal.pos[3].x    = p_rgn[i].attr.mosaic.x[3];
					ime_ext_mask.data.mask[num].data.normal.pos[3].y    = p_rgn[i].attr.mosaic.y[3];
					ime_ext_mask.data.mask[num].data.normal.alpha       = 255;

					if(p_rgn[i].attr.mosaic.mosaic_blk_w <= 8)
						ime_ext_mask.data.mask[num].data.normal.mosaic_blk_size = 8;
					else if(p_rgn[i].attr.mosaic.mosaic_blk_w > 8 && p_rgn[i].attr.mosaic.mosaic_blk_w <= 16)
						ime_ext_mask.data.mask[num].data.normal.mosaic_blk_size = 16;
					else if(p_rgn[i].attr.mosaic.mosaic_blk_w > 16 && p_rgn[i].attr.mosaic.mosaic_blk_w <= 32)
						ime_ext_mask.data.mask[num].data.normal.mosaic_blk_size = 32;
					else
						ime_ext_mask.data.mask[num].data.normal.mosaic_blk_size = 64;
				}else if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_INCONTINUOUS){
					ime_ext_mask.data.mask[num].type                          = VDS_INTERNAL_MASK_TYPE_INCONTINUOUS;
					ime_ext_mask.data.mask[num].data.incontinuous.pos.x       = p_rgn[i].attr.mask.data.incontinuous.x;
					ime_ext_mask.data.mask[num].data.incontinuous.pos.y       = p_rgn[i].attr.mask.data.incontinuous.y;
					ime_ext_mask.data.mask[num].data.incontinuous.h_line_len  = p_rgn[i].attr.mask.data.incontinuous.h_line_len;
					ime_ext_mask.data.mask[num].data.incontinuous.h_hole_len  = p_rgn[i].attr.mask.data.incontinuous.h_hole_len;
					ime_ext_mask.data.mask[num].data.incontinuous.h_thickness = p_rgn[i].attr.mask.data.incontinuous.h_thickness;
					ime_ext_mask.data.mask[num].data.incontinuous.v_line_len  = p_rgn[i].attr.mask.data.incontinuous.v_line_len;
					ime_ext_mask.data.mask[num].data.incontinuous.v_hole_len  = p_rgn[i].attr.mask.data.incontinuous.v_hole_len;
					ime_ext_mask.data.mask[num].data.incontinuous.v_thickness = p_rgn[i].attr.mask.data.incontinuous.v_thickness;
				}else{
					if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_SOLID)
						ime_ext_mask.data.mask[num].type = VDS_INTERNAL_MASK_TYPE_SOLID;
					else if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_HOLLOW){
						ime_ext_mask.data.mask[num].type = VDS_INTERNAL_MASK_TYPE_HOLLOW;
						ime_ext_mask.data.mask[num].thickness = p_rgn[i].attr.mask.thickness;
					}else{
						DBG_ERR("VDS_QS_IME_EXT_MASK : unknown type = %d\r\n", p_rgn[i].attr.mask.type);
						break;
					}
					ime_ext_mask.data.mask[num].data.normal.pos[0].x    = p_rgn[i].attr.mask.data.normal.x[0];
					ime_ext_mask.data.mask[num].data.normal.pos[0].y    = p_rgn[i].attr.mask.data.normal.y[0];
					ime_ext_mask.data.mask[num].data.normal.pos[1].x    = p_rgn[i].attr.mask.data.normal.x[1];
					ime_ext_mask.data.mask[num].data.normal.pos[1].y    = p_rgn[i].attr.mask.data.normal.y[1];
					ime_ext_mask.data.mask[num].data.normal.pos[2].x    = p_rgn[i].attr.mask.data.normal.x[2];
					ime_ext_mask.data.mask[num].data.normal.pos[2].y    = p_rgn[i].attr.mask.data.normal.y[2];
					ime_ext_mask.data.mask[num].data.normal.pos[3].x    = p_rgn[i].attr.mask.data.normal.x[3];
					ime_ext_mask.data.mask[num].data.normal.pos[3].y    = p_rgn[i].attr.mask.data.normal.y[3];
					ime_ext_mask.data.mask[num].data.normal.alpha       = p_rgn[i].attr.mask.data.normal.alpha;
				}

				++num;
			}

		ret = (void*)&ime_ext_mask;

	}
	else if(stage == VDS_QS_ENC_EXT_STAMP){

		vds_res_lock(LOCK_RESOURCE_ENC_EXT_STAMP, ime_flag);
		vds_save_latency(VDS_QS_ENC_EXT_STAMP, id, 1, hwclock_get_longcounter() - start);

		if(enc_ext_stamp.stamp && vds_max_coe_ext_stamp && (int)id < vds_max_coe_ext_vid && coe_ext_stamps){
			vds_memset(enc_ext_stamp.stamp, 0, sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_coe_ext_stamp);
			p_rgn = &(coe_ext_stamps[vds_max_coe_ext_stamp * id]);
		}else
			return &enc_ext_stamp;

		for(i = 0 ; i < vds_max_coe_ext_stamp ; ++i)
			if(p_rgn[i].valid && p_rgn[i].en && p_rgn[i].buf && p_rgn[i].buf->valid){

				p_buf = p_rgn[i].buf;
				swap = (p_buf->swap ? 0 : 1);

				if(!p_buf->p_addr[swap] || !p_buf->img_size[swap].w || !p_buf->img_size[swap].h)
					continue;
				else
					enc_ext_stamp.dirty = 1;

				enc_ext_stamp.stamp[num].en        = 1;
				enc_ext_stamp.stamp[num].size.w    = p_buf->img_size[swap].w;
				enc_ext_stamp.stamp[num].size.h    = p_buf->img_size[swap].h;
				enc_ext_stamp.stamp[num].pos.x     = p_rgn[i].attr.stamp.x;
				enc_ext_stamp.stamp[num].pos.y     = p_rgn[i].attr.stamp.y;
				enc_ext_stamp.stamp[num].alpha     = p_rgn[i].attr.stamp.fg_alpha;
				enc_ext_stamp.stamp[num].addr      = p_buf->p_addr[swap];

				if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_RGB565)
					enc_ext_stamp.stamp[num].fmt = VDO_PXLFMT_RGB565;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB1555)
					enc_ext_stamp.stamp[num].fmt = VDO_PXLFMT_ARGB1555;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB4444)
					enc_ext_stamp.stamp[num].fmt = VDO_PXLFMT_ARGB4444;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB8888)
					enc_ext_stamp.stamp[num].fmt = VDO_PXLFMT_ARGB8888;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE1)
					enc_ext_stamp.stamp[num].fmt = VDO_PXLFMT_I1;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE2)
					enc_ext_stamp.stamp[num].fmt = VDO_PXLFMT_I2;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE4)
					enc_ext_stamp.stamp[num].fmt = VDO_PXLFMT_I4;
				else{
					DBG_ERR("VDS_QS_ENC_EXT_STAMP : unknown format = %d\r\n", p_buf->fmt[swap]);
					break;
				}

				++num;
			}

		ret = (void*)&enc_ext_stamp;
	}
	else if(stage == VDS_QS_ENC_EXT_MASK){

		vds_res_lock(LOCK_RESOURCE_ENC_EXT_MASK, ime_flag);
		vds_save_latency(VDS_QS_ENC_EXT_MASK, id, 1, hwclock_get_longcounter() - start);

		if(enc_ext_mask.mask && vds_max_coe_ext_mask && (int)id < vds_max_coe_ext_vid && coe_ext_masks){
			vds_memset(enc_ext_mask.mask, 0, sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_coe_ext_mask);
			p_rgn = &(coe_ext_masks[vds_max_coe_ext_mask * id]);
		}else
			return &enc_ext_mask;

		for(i = 0 ; i < vds_max_coe_ext_mask ; ++i)
			if(p_rgn[i].valid && p_rgn[i].en){

				enc_ext_mask.dirty                 = 1;
				enc_ext_mask.mask[num].is_mosaic   = p_rgn[i].is_mosaic;
				enc_ext_mask.mask[num].en          = 1;

				rgb_color = p_rgn[i].attr.mask.color;
				enc_ext_mask.mask[num].color  = RGB_GET_V((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
				enc_ext_mask.mask[num].color <<= 8;
				enc_ext_mask.mask[num].color += RGB_GET_U((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
				enc_ext_mask.mask[num].color <<= 8;
				enc_ext_mask.mask[num].color += RGB_GET_Y((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);

				if(p_rgn[i].is_mosaic){
					enc_ext_mask.mask[num].data.normal.pos[0].x    = p_rgn[i].attr.mosaic.x[0];
					enc_ext_mask.mask[num].data.normal.pos[0].y    = p_rgn[i].attr.mosaic.y[0];
					enc_ext_mask.mask[num].data.normal.pos[1].x    = p_rgn[i].attr.mosaic.x[1];
					enc_ext_mask.mask[num].data.normal.pos[1].y    = p_rgn[i].attr.mosaic.y[1];
					enc_ext_mask.mask[num].data.normal.pos[2].x    = p_rgn[i].attr.mosaic.x[2];
					enc_ext_mask.mask[num].data.normal.pos[2].y    = p_rgn[i].attr.mosaic.y[2];
					enc_ext_mask.mask[num].data.normal.pos[3].x    = p_rgn[i].attr.mosaic.x[3];
					enc_ext_mask.mask[num].data.normal.pos[3].y    = p_rgn[i].attr.mosaic.y[3];
					enc_ext_mask.mask[num].data.normal.alpha       = 255;

					if(p_rgn[i].attr.mosaic.mosaic_blk_w <= 8)
						enc_ext_mask.mask[num].data.normal.mosaic_blk_size = 8;
					else if(p_rgn[i].attr.mosaic.mosaic_blk_w > 8 && p_rgn[i].attr.mosaic.mosaic_blk_w <= 16)
						enc_ext_mask.mask[num].data.normal.mosaic_blk_size = 16;
					else if(p_rgn[i].attr.mosaic.mosaic_blk_w > 16 && p_rgn[i].attr.mosaic.mosaic_blk_w <= 32)
						enc_ext_mask.mask[num].data.normal.mosaic_blk_size = 32;
					else
						enc_ext_mask.mask[num].data.normal.mosaic_blk_size = 64;
				}else if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_INCONTINUOUS){
					enc_ext_mask.mask[num].type                    = VDS_INTERNAL_MASK_TYPE_INCONTINUOUS;
					enc_ext_mask.mask[num].data.incontinuous.pos.x       = p_rgn[i].attr.mask.data.incontinuous.x;
					enc_ext_mask.mask[num].data.incontinuous.pos.y       = p_rgn[i].attr.mask.data.incontinuous.y;
					enc_ext_mask.mask[num].data.incontinuous.h_line_len  = p_rgn[i].attr.mask.data.incontinuous.h_line_len;
					enc_ext_mask.mask[num].data.incontinuous.h_hole_len  = p_rgn[i].attr.mask.data.incontinuous.h_hole_len;
					enc_ext_mask.mask[num].data.incontinuous.h_thickness = p_rgn[i].attr.mask.data.incontinuous.h_thickness;
					enc_ext_mask.mask[num].data.incontinuous.v_line_len  = p_rgn[i].attr.mask.data.incontinuous.v_line_len;
					enc_ext_mask.mask[num].data.incontinuous.v_hole_len  = p_rgn[i].attr.mask.data.incontinuous.v_hole_len;
					enc_ext_mask.mask[num].data.incontinuous.v_thickness = p_rgn[i].attr.mask.data.incontinuous.v_thickness;
				}else{
					if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_SOLID)
						enc_ext_mask.mask[num].type = VDS_INTERNAL_MASK_TYPE_SOLID;
					else if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_HOLLOW){
						enc_ext_mask.mask[num].type    = VDS_INTERNAL_MASK_TYPE_HOLLOW;
						enc_ext_mask.mask[num].thickness = p_rgn[i].attr.mask.thickness;
					}else{
						DBG_ERR("VDS_QS_ENC_EXT_MASK : unknown type = %d\r\n", p_rgn[i].attr.mask.type);
						break;
					}
					enc_ext_mask.mask[num].data.normal.pos[0].x    = p_rgn[i].attr.mask.data.normal.x[0];
					enc_ext_mask.mask[num].data.normal.pos[0].y    = p_rgn[i].attr.mask.data.normal.y[0];
					enc_ext_mask.mask[num].data.normal.pos[1].x    = p_rgn[i].attr.mask.data.normal.x[1];
					enc_ext_mask.mask[num].data.normal.pos[1].y    = p_rgn[i].attr.mask.data.normal.y[1];
					enc_ext_mask.mask[num].data.normal.pos[2].x    = p_rgn[i].attr.mask.data.normal.x[2];
					enc_ext_mask.mask[num].data.normal.pos[2].y    = p_rgn[i].attr.mask.data.normal.y[2];
					enc_ext_mask.mask[num].data.normal.pos[3].x    = p_rgn[i].attr.mask.data.normal.x[3];
					enc_ext_mask.mask[num].data.normal.pos[3].y    = p_rgn[i].attr.mask.data.normal.y[3];
					enc_ext_mask.mask[num].data.normal.alpha       = p_rgn[i].attr.mask.data.normal.alpha;
				}

				++num;
			}

		ret = (void*)&enc_ext_mask;
	}
	else if(stage == VDS_QS_ENC_COE_STAMP){

		vds_res_lock(LOCK_RESOURCE_ENC_COE_STAMP, ime_flag);
		vds_save_latency(VDS_QS_ENC_COE_STAMP, id, 1, hwclock_get_longcounter() - start);

		if(enc_coe_stamp.stamp && vds_max_coe_stamp && (int)id < vds_max_coe_vid && coe_stamps){
			vds_memset(enc_coe_stamp.stamp, 0, sizeof(VDS_INTERNAL_COE_STAMP_MASK) * vds_max_coe_stamp);
			p_rgn = &(coe_stamps[vds_max_coe_stamp * id]);
		}else
			return &enc_coe_stamp;

		for(i = 0 ; i < vds_max_coe_stamp ; ++i)
			if(p_rgn[i].valid && p_rgn[i].en){

				if(p_rgn[i].is_enc_mask){
					enc_coe_stamp.dirty                          = 1;
					enc_coe_stamp.stamp[num].en                  = 1;
					enc_coe_stamp.stamp[num].is_mask             = 1;
					enc_coe_stamp.stamp[num].data.mask.alpha  = p_rgn[i].attr.enc_mask.alpha;
					enc_coe_stamp.stamp[num].data.mask.alpha  = p_rgn[i].attr.enc_mask.alpha;
					enc_coe_stamp.stamp[num].data.mask.x         = p_rgn[i].attr.enc_mask.x;
					enc_coe_stamp.stamp[num].data.mask.y         = p_rgn[i].attr.enc_mask.y;
					enc_coe_stamp.stamp[num].data.mask.w         = p_rgn[i].attr.enc_mask.w;
					enc_coe_stamp.stamp[num].data.mask.h         = p_rgn[i].attr.enc_mask.h;
					enc_coe_stamp.stamp[num].data.mask.thickness = p_rgn[i].attr.enc_mask.thickness;
					enc_coe_stamp.stamp[num].data.mask.layer     = p_rgn[i].attr.enc_mask.layer;
					enc_coe_stamp.stamp[num].data.mask.region    = p_rgn[i].attr.enc_mask.region;
					if(p_rgn[i].attr.enc_mask.type == VDS_MASK_TYPE_SOLID)
						enc_coe_stamp.stamp[num].data.mask.type = VDS_INTERNAL_MASK_TYPE_SOLID;
					else if(p_rgn[i].attr.enc_mask.type == VDS_MASK_TYPE_HOLLOW)
						enc_coe_stamp.stamp[num].data.mask.type = VDS_INTERNAL_MASK_TYPE_HOLLOW;
					else{
						DBG_ERR("VDS_QS_ENC_COE_STAMP : unknown mask type = %d\r\n", p_rgn[i].attr.enc_mask.type);
						break;
					}
					rgb_color = p_rgn[i].attr.enc_mask.color;
					enc_coe_stamp.stamp[num].data.mask.color  = RGB_GET_V((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
					enc_coe_stamp.stamp[num].data.mask.color <<= 8;
					enc_coe_stamp.stamp[num].data.mask.color += RGB_GET_U((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
					enc_coe_stamp.stamp[num].data.mask.color <<= 8;
					enc_coe_stamp.stamp[num].data.mask.color += RGB_GET_Y((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
				}else{
					if(!p_rgn[i].buf || !p_rgn[i].buf->valid)
						continue;

					p_buf = p_rgn[i].buf;
					swap = (p_buf->swap ? 0 : 1);

					if(!p_buf->p_addr[swap] || !p_buf->img_size[swap].w || !p_buf->img_size[swap].h)
						continue;
					else
						enc_coe_stamp.dirty = 1;

					enc_coe_stamp.stamp[num].en                              = 1;
					enc_coe_stamp.stamp[num].is_mask                         = 0;
					enc_coe_stamp.stamp[num].data.stamp.layer                = p_rgn[i].attr.stamp.layer;
					enc_coe_stamp.stamp[num].data.stamp.region               = p_rgn[i].attr.stamp.region;
					enc_coe_stamp.stamp[num].data.stamp.size.w               = p_buf->img_size[swap].w;
					enc_coe_stamp.stamp[num].data.stamp.size.h               = p_buf->img_size[swap].h;
					enc_coe_stamp.stamp[num].data.stamp.pos.x                = p_rgn[i].attr.stamp.x;
					enc_coe_stamp.stamp[num].data.stamp.pos.y                = p_rgn[i].attr.stamp.y;
					enc_coe_stamp.stamp[num].data.stamp.fg_alpha             = p_rgn[i].attr.stamp.fg_alpha;
					enc_coe_stamp.stamp[num].data.stamp.bg_alpha             = p_rgn[i].attr.stamp.bg_alpha;
					enc_coe_stamp.stamp[num].data.stamp.addr                 = p_buf->p_addr[swap];
					enc_coe_stamp.stamp[num].data.stamp.ckey_en              = p_rgn[i].attr.stamp.colorkey_en;
					enc_coe_stamp.stamp[num].data.stamp.ckey_val             = p_rgn[i].attr.stamp.colorkey_val;
					enc_coe_stamp.stamp[num].data.stamp.qp_en                = p_rgn[i].attr.stamp.qp_en;
					enc_coe_stamp.stamp[num].data.stamp.qp_fix               = p_rgn[i].attr.stamp.qp_fix;
					enc_coe_stamp.stamp[num].data.stamp.qp_val               = p_rgn[i].attr.stamp.qp_val;
					enc_coe_stamp.stamp[num].data.stamp.qp_lpm_mode          = p_rgn[i].attr.stamp.qp_lpm_mode;
					enc_coe_stamp.stamp[num].data.stamp.qp_tnr_mode          = p_rgn[i].attr.stamp.qp_tnr_mode;
					enc_coe_stamp.stamp[num].data.stamp.qp_fro_mode          = p_rgn[i].attr.stamp.qp_fro_mode;
					enc_coe_stamp.stamp[num].data.stamp.gcac_enable          = p_rgn[i].attr.stamp.gcac_enable;
					enc_coe_stamp.stamp[num].data.stamp.gcac_blk_width       = p_rgn[i].attr.stamp.gcac_blk_width;
					enc_coe_stamp.stamp[num].data.stamp.gcac_blk_height      = p_rgn[i].attr.stamp.gcac_blk_height;
					enc_coe_stamp.stamp[num].data.stamp.gcac_blk_num         = p_rgn[i].attr.stamp.gcac_blk_num;
					enc_coe_stamp.stamp[num].data.stamp.gcac_org_color_level = p_rgn[i].attr.stamp.gcac_org_color_level;
					enc_coe_stamp.stamp[num].data.stamp.gcac_inv_color_level = p_rgn[i].attr.stamp.gcac_inv_color_level;
					enc_coe_stamp.stamp[num].data.stamp.gcac_nor_diff_th     = p_rgn[i].attr.stamp.gcac_nor_diff_th;
					enc_coe_stamp.stamp[num].data.stamp.gcac_inv_diff_th     = p_rgn[i].attr.stamp.gcac_inv_diff_th;
					enc_coe_stamp.stamp[num].data.stamp.gcac_sta_only_mode   = p_rgn[i].attr.stamp.gcac_sta_only_mode;
					enc_coe_stamp.stamp[num].data.stamp.gcac_full_eval_mode  = p_rgn[i].attr.stamp.gcac_full_eval_mode;
					enc_coe_stamp.stamp[num].data.stamp.gcac_eval_lum_targ   = p_rgn[i].attr.stamp.gcac_eval_lum_targ;

					if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_RGB565)
						enc_coe_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_RGB565;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB1555)
						enc_coe_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_ARGB1555;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB4444)
						enc_coe_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_ARGB4444;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB8888)
						enc_coe_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_ARGB8888;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE1)
						enc_coe_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_I1;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE2)
						enc_coe_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_I2;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE4)
						enc_coe_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_I4;
					else{
						DBG_ERR("VDS_QS_ENC_COE_STAMP : unknown format = %d\r\n", p_buf->fmt[swap]);
						break;
					}
				}

				++num;
			}

		ret = (void*)&enc_coe_stamp;
	}
	else if(stage == VDS_QS_ENC_JPG_STAMP){

		vds_res_lock(LOCK_RESOURCE_ENC_JPG_STAMP, ime_flag);
		vds_save_latency(VDS_QS_ENC_JPG_STAMP, id, 1, hwclock_get_longcounter() - start);

		if(enc_jpg_stamp.stamp && vds_max_coe_stamp && (int)id < vds_max_coe_vid && coe_stamps){
			vds_memset(enc_jpg_stamp.stamp, 0, sizeof(VDS_INTERNAL_COE_STAMP_MASK) * vds_max_coe_stamp);
			p_rgn = &(coe_stamps[vds_max_coe_stamp * id]);
		}else
			return &enc_jpg_stamp;

		for(i = 0 ; i < vds_max_coe_stamp ; ++i)
			if(p_rgn[i].valid && p_rgn[i].en){

				if(p_rgn[i].is_enc_mask){
					enc_jpg_stamp.dirty                          = 1;
					enc_jpg_stamp.stamp[num].en                  = 1;
					enc_jpg_stamp.stamp[num].is_mask             = 1;
					enc_jpg_stamp.stamp[num].data.mask.alpha     = p_rgn[i].attr.enc_mask.alpha;
					enc_jpg_stamp.stamp[num].data.mask.x         = p_rgn[i].attr.enc_mask.x;
					enc_jpg_stamp.stamp[num].data.mask.y         = p_rgn[i].attr.enc_mask.y;
					enc_jpg_stamp.stamp[num].data.mask.w         = p_rgn[i].attr.enc_mask.w;
					enc_jpg_stamp.stamp[num].data.mask.h         = p_rgn[i].attr.enc_mask.h;
					enc_jpg_stamp.stamp[num].data.mask.thickness = p_rgn[i].attr.enc_mask.thickness;
					enc_jpg_stamp.stamp[num].data.mask.layer     = p_rgn[i].attr.enc_mask.layer;
					enc_jpg_stamp.stamp[num].data.mask.region    = p_rgn[i].attr.enc_mask.region;
					if(p_rgn[i].attr.enc_mask.type == VDS_MASK_TYPE_SOLID)
						enc_jpg_stamp.stamp[num].data.mask.type = VDS_INTERNAL_MASK_TYPE_SOLID;
					else if(p_rgn[i].attr.enc_mask.type == VDS_MASK_TYPE_HOLLOW)
						enc_jpg_stamp.stamp[num].data.mask.type = VDS_INTERNAL_MASK_TYPE_HOLLOW;
					else{
						DBG_ERR("VDS_QS_ENC_COE_STAMP : unknown mask type = %d\r\n", p_rgn[i].attr.enc_mask.type);
						break;
					}
					rgb_color = p_rgn[i].attr.enc_mask.color;
					enc_jpg_stamp.stamp[num].data.mask.color  = RGB_GET_V((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
					enc_jpg_stamp.stamp[num].data.mask.color <<= 8;
					enc_jpg_stamp.stamp[num].data.mask.color += RGB_GET_U((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
					enc_jpg_stamp.stamp[num].data.mask.color <<= 8;
					enc_jpg_stamp.stamp[num].data.mask.color += RGB_GET_Y((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
				}else{
					if(!p_rgn[i].buf || !p_rgn[i].buf->valid)
						continue;

					p_buf = p_rgn[i].buf;
					swap = (p_buf->swap ? 0 : 1);

					if(!p_buf->p_addr[swap] || !p_buf->img_size[swap].w || !p_buf->img_size[swap].h)
						continue;
					else
						enc_jpg_stamp.dirty = 1;

					enc_jpg_stamp.stamp[num].en                              = 1;
					enc_jpg_stamp.stamp[num].is_mask                         = 0;
					enc_jpg_stamp.stamp[num].data.stamp.layer                = p_rgn[i].attr.stamp.layer;
					enc_jpg_stamp.stamp[num].data.stamp.region               = p_rgn[i].attr.stamp.region;
					enc_jpg_stamp.stamp[num].data.stamp.size.w               = p_buf->img_size[swap].w;
					enc_jpg_stamp.stamp[num].data.stamp.size.h               = p_buf->img_size[swap].h;
					enc_jpg_stamp.stamp[num].data.stamp.pos.x                = p_rgn[i].attr.stamp.x;
					enc_jpg_stamp.stamp[num].data.stamp.pos.y                = p_rgn[i].attr.stamp.y;
					enc_jpg_stamp.stamp[num].data.stamp.fg_alpha             = p_rgn[i].attr.stamp.fg_alpha;
					enc_jpg_stamp.stamp[num].data.stamp.bg_alpha             = p_rgn[i].attr.stamp.bg_alpha;
					enc_jpg_stamp.stamp[num].data.stamp.addr                 = p_buf->p_addr[swap];
					enc_jpg_stamp.stamp[num].data.stamp.ckey_en              = p_rgn[i].attr.stamp.colorkey_en;
					enc_jpg_stamp.stamp[num].data.stamp.ckey_val             = p_rgn[i].attr.stamp.colorkey_val;
					enc_jpg_stamp.stamp[num].data.stamp.qp_en                = p_rgn[i].attr.stamp.qp_en;
					enc_jpg_stamp.stamp[num].data.stamp.qp_fix               = p_rgn[i].attr.stamp.qp_fix;
					enc_jpg_stamp.stamp[num].data.stamp.qp_val               = p_rgn[i].attr.stamp.qp_val;
					enc_jpg_stamp.stamp[num].data.stamp.qp_lpm_mode          = p_rgn[i].attr.stamp.qp_lpm_mode;
					enc_jpg_stamp.stamp[num].data.stamp.qp_tnr_mode          = p_rgn[i].attr.stamp.qp_tnr_mode;
					enc_jpg_stamp.stamp[num].data.stamp.qp_fro_mode          = p_rgn[i].attr.stamp.qp_fro_mode;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_enable          = p_rgn[i].attr.stamp.gcac_enable;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_blk_width       = p_rgn[i].attr.stamp.gcac_blk_width;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_blk_height      = p_rgn[i].attr.stamp.gcac_blk_height;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_blk_num         = p_rgn[i].attr.stamp.gcac_blk_num;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_org_color_level = p_rgn[i].attr.stamp.gcac_org_color_level;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_inv_color_level = p_rgn[i].attr.stamp.gcac_inv_color_level;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_nor_diff_th     = p_rgn[i].attr.stamp.gcac_nor_diff_th;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_inv_diff_th     = p_rgn[i].attr.stamp.gcac_inv_diff_th;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_sta_only_mode   = p_rgn[i].attr.stamp.gcac_sta_only_mode;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_full_eval_mode  = p_rgn[i].attr.stamp.gcac_full_eval_mode;
					enc_jpg_stamp.stamp[num].data.stamp.gcac_eval_lum_targ   = p_rgn[i].attr.stamp.gcac_eval_lum_targ;

					if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_RGB565)
						enc_jpg_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_RGB565;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB1555)
						enc_jpg_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_ARGB1555;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB4444)
						enc_jpg_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_ARGB4444;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB8888)
						enc_jpg_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_ARGB8888;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE1)
						enc_jpg_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_I1;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE2)
						enc_jpg_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_I2;
					else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE4)
						enc_jpg_stamp.stamp[num].data.stamp.fmt = VDO_PXLFMT_I4;
					else{
						DBG_ERR("VDS_QS_ENC_COE_STAMP : unknown format = %d\r\n", p_buf->fmt[swap]);
						break;
					}
				}

				++num;
			}

		ret = (void*)&enc_jpg_stamp;
	}
	else if(stage == VDS_QS_VO_STAMP){

		vds_res_lock(LOCK_RESOURCE_VO_STAMP, ime_flag);
		vds_save_latency(VDS_QS_VO_STAMP, id, 1, hwclock_get_longcounter() - start);

		if(vo_stamp.stamp && vds_max_vo_stamp && (int)id < vds_max_vo_vid && vo_stamps){
			vds_memset(vo_stamp.stamp, 0, sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_vo_stamp);
			p_rgn = &(vo_stamps[vds_max_vo_stamp * id]);
		}else
			return &vo_stamp;

		for(i = 0 ; i < vds_max_vo_stamp ; ++i)
			if(p_rgn[i].valid && p_rgn[i].en && p_rgn[i].buf && p_rgn[i].buf->valid){

				p_buf = p_rgn[i].buf;
				swap = (p_buf->swap ? 0 : 1);

				if(!p_buf->p_addr[swap] || !p_buf->img_size[swap].w || !p_buf->img_size[swap].h)
					continue;
				else
					vo_stamp.dirty = 1;

				vo_stamp.stamp[num].en        = 1;
				vo_stamp.stamp[num].size.w    = p_buf->img_size[swap].w;
				vo_stamp.stamp[num].size.h    = p_buf->img_size[swap].h;
				vo_stamp.stamp[num].pos.x     = p_rgn[i].attr.stamp.x;
				vo_stamp.stamp[num].pos.y     = p_rgn[i].attr.stamp.y;
				vo_stamp.stamp[num].alpha     = p_rgn[i].attr.stamp.fg_alpha;
				vo_stamp.stamp[num].addr      = p_buf->p_addr[swap];

				if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_RGB565)
					vo_stamp.stamp[num].fmt = VDO_PXLFMT_RGB565;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB1555)
					vo_stamp.stamp[num].fmt = VDO_PXLFMT_ARGB1555;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB4444)
					vo_stamp.stamp[num].fmt = VDO_PXLFMT_ARGB4444;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_ARGB8888)
					vo_stamp.stamp[num].fmt = VDO_PXLFMT_ARGB8888;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE1)
					vo_stamp.stamp[num].fmt = VDO_PXLFMT_I1;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE2)
					vo_stamp.stamp[num].fmt = VDO_PXLFMT_I2;
				else if(p_buf->fmt[swap] == VDS_IMG_FMT_PICTURE_PALETTE4)
					vo_stamp.stamp[num].fmt = VDO_PXLFMT_I4;
				else{
					DBG_ERR("VDS_QS_ENC_EXT_STAMP : unknown format = %d\r\n", p_buf->fmt[swap]);
					break;
				}

				++num;
			}

		ret = (void*)&vo_stamp;
	}
	else if(stage == VDS_QS_VO_MASK){

		vds_res_lock(LOCK_RESOURCE_VO_MASK, ime_flag);
		vds_save_latency(VDS_QS_VO_MASK, id, 1, hwclock_get_longcounter() - start);

		if(vo_mask.mask && vds_max_vo_mask && (int)id < vds_max_vo_vid && vo_masks){
			vds_memset(vo_mask.mask, 0, sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_vo_mask);
			p_rgn = &(vo_masks[vds_max_vo_mask * id]);
		}else
			return &vo_mask;

		for(i = 0 ; i < vds_max_vo_mask ; ++i)
			if(p_rgn[i].valid && p_rgn[i].en){

				vo_mask.dirty                 = 1;
				vo_mask.mask[num].is_mosaic   = p_rgn[i].is_mosaic;
				vo_mask.mask[num].en          = 1;

				rgb_color = p_rgn[i].attr.mask.color;
				vo_mask.mask[num].color  = RGB_GET_V((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
				vo_mask.mask[num].color <<= 8;
				vo_mask.mask[num].color += RGB_GET_U((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);
				vo_mask.mask[num].color <<= 8;
				vo_mask.mask[num].color += RGB_GET_Y((rgb_color & 0x0FF0000) >> 16, (rgb_color & 0x0FF00) >> 8, rgb_color & 0x0FF);

				if(p_rgn[i].is_mosaic){
					vo_mask.mask[num].data.normal.pos[0].x    = p_rgn[i].attr.mosaic.x[0];
					vo_mask.mask[num].data.normal.pos[0].y    = p_rgn[i].attr.mosaic.y[0];
					vo_mask.mask[num].data.normal.pos[1].x    = p_rgn[i].attr.mosaic.x[1];
					vo_mask.mask[num].data.normal.pos[1].y    = p_rgn[i].attr.mosaic.y[1];
					vo_mask.mask[num].data.normal.pos[2].x    = p_rgn[i].attr.mosaic.x[2];
					vo_mask.mask[num].data.normal.pos[2].y    = p_rgn[i].attr.mosaic.y[2];
					vo_mask.mask[num].data.normal.pos[3].x    = p_rgn[i].attr.mosaic.x[3];
					vo_mask.mask[num].data.normal.pos[3].y    = p_rgn[i].attr.mosaic.y[3];
					vo_mask.mask[num].data.normal.alpha       = 255;

					if(p_rgn[i].attr.mosaic.mosaic_blk_w <= 8)
						vo_mask.mask[num].data.normal.mosaic_blk_size = 8;
					else if(p_rgn[i].attr.mosaic.mosaic_blk_w > 8 && p_rgn[i].attr.mosaic.mosaic_blk_w <= 16)
						vo_mask.mask[num].data.normal.mosaic_blk_size = 16;
					else if(p_rgn[i].attr.mosaic.mosaic_blk_w > 16 && p_rgn[i].attr.mosaic.mosaic_blk_w <= 32)
						vo_mask.mask[num].data.normal.mosaic_blk_size = 32;
					else
						vo_mask.mask[num].data.normal.mosaic_blk_size = 64;
				}else if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_INCONTINUOUS){
					vo_mask.mask[num].type                              = VDS_INTERNAL_MASK_TYPE_INCONTINUOUS;
					vo_mask.mask[num].data.incontinuous.pos.x       = p_rgn[i].attr.mask.data.incontinuous.x;
					vo_mask.mask[num].data.incontinuous.pos.y       = p_rgn[i].attr.mask.data.incontinuous.y;
					vo_mask.mask[num].data.incontinuous.h_line_len  = p_rgn[i].attr.mask.data.incontinuous.h_line_len;
					vo_mask.mask[num].data.incontinuous.h_hole_len  = p_rgn[i].attr.mask.data.incontinuous.h_hole_len;
					vo_mask.mask[num].data.incontinuous.h_thickness = p_rgn[i].attr.mask.data.incontinuous.h_thickness;
					vo_mask.mask[num].data.incontinuous.v_line_len  = p_rgn[i].attr.mask.data.incontinuous.v_line_len;
					vo_mask.mask[num].data.incontinuous.v_hole_len  = p_rgn[i].attr.mask.data.incontinuous.v_hole_len;
					vo_mask.mask[num].data.incontinuous.v_thickness = p_rgn[i].attr.mask.data.incontinuous.v_thickness;
				}else{
					if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_SOLID)
						vo_mask.mask[num].type = VDS_INTERNAL_MASK_TYPE_SOLID;
					else if(p_rgn[i].attr.mask.type == VDS_MASK_TYPE_HOLLOW){
						vo_mask.mask[num].type = VDS_INTERNAL_MASK_TYPE_HOLLOW;
						vo_mask.mask[num].thickness = p_rgn[i].attr.mask.thickness;
					}else{
						DBG_ERR("VDS_QS_VO_MASK : unknown type = %d\r\n", p_rgn[i].attr.mask.type);
						break;
					}
					vo_mask.mask[num].data.normal.pos[0].x    = p_rgn[i].attr.mask.data.normal.x[0];
					vo_mask.mask[num].data.normal.pos[0].y    = p_rgn[i].attr.mask.data.normal.y[0];
					vo_mask.mask[num].data.normal.pos[1].x    = p_rgn[i].attr.mask.data.normal.x[1];
					vo_mask.mask[num].data.normal.pos[1].y    = p_rgn[i].attr.mask.data.normal.y[1];
					vo_mask.mask[num].data.normal.pos[2].x    = p_rgn[i].attr.mask.data.normal.x[2];
					vo_mask.mask[num].data.normal.pos[2].y    = p_rgn[i].attr.mask.data.normal.y[2];
					vo_mask.mask[num].data.normal.pos[3].x    = p_rgn[i].attr.mask.data.normal.x[3];
					vo_mask.mask[num].data.normal.pos[3].y    = p_rgn[i].attr.mask.data.normal.y[3];
					vo_mask.mask[num].data.normal.alpha       = p_rgn[i].attr.mask.data.normal.alpha;
				}

				++num;
			}

		ret = (void*)&vo_mask;
	}
	else
		DBG_ERR("unknown stage =%d\r\n", stage);

	return ret;
}

int vds_unlock_context(VDS_QUERY_STAGE stage, UINT32 id, UINT32 ime_flag)
{
	if(stage == VDS_QS_IME_STAMP)
		return ISF_OK;//vds_res_unlock(LOCK_RESOURCE_IME_STAMP);
	else if(stage == VDS_QS_IME_MASK)
		vds_res_unlock(LOCK_RESOURCE_IME_MASK, ime_flag);
	else if(stage == VDS_QS_IME_EXT_STAMP)
		vds_res_unlock(LOCK_RESOURCE_IME_EXT_STAMP, ime_flag);
	else if(stage == VDS_QS_IME_EXT_MASK)
		vds_res_unlock(LOCK_RESOURCE_IME_EXT_MASK, ime_flag);
	else if(stage == VDS_QS_ENC_EXT_STAMP)
		vds_res_unlock(LOCK_RESOURCE_ENC_EXT_STAMP, ime_flag);
	else if(stage == VDS_QS_ENC_EXT_MASK)
		vds_res_unlock(LOCK_RESOURCE_ENC_EXT_MASK, ime_flag);
	else if(stage == VDS_QS_ENC_COE_STAMP)
		vds_res_unlock(LOCK_RESOURCE_ENC_COE_STAMP, ime_flag);
	else if(stage == VDS_QS_ENC_JPG_STAMP)
		vds_res_unlock(LOCK_RESOURCE_ENC_JPG_STAMP, ime_flag);
	else if(stage == VDS_QS_VO_MASK)
		vds_res_unlock(LOCK_RESOURCE_VO_MASK, ime_flag);
	else if(stage == VDS_QS_VO_STAMP)
		vds_res_unlock(LOCK_RESOURCE_VO_STAMP, ime_flag);
	else{
		DBG_ERR("unknown stage =%d\r\n", stage);
		return ISF_ERR_INVALID_PORT_ID;
	}

	return ISF_OK;
}

static int render_graph_stamp(VDS_INTERNAL_EXT_STAMP* rgn, int cnt, UINT32 y, UINT32 uv, UINT32 w, UINT32 h, UINT32 *palette, UINT32 *lineoffset, VDO_PXLFMT fmt)
{
	VDO_FRAME                dst;
	UINT32                   loff[2] = {0}, addr[2] = {0}, size;
	int                      i;
	VDO_FRAME                r_img;
	IPOINT                   pt;

	if(lineoffset){
		loff[0] = lineoffset[0];
		loff[1] = lineoffset[1];
	}else{
		loff[0] = w;
		loff[1] = w;
	}

	if(fmt != VDO_PXLFMT_YUV420 && fmt != VDO_PXLFMT_YUV422){
		DBG_ERR("stream format(%x) is not supported. only yuv420 and yuv422 are supported\n", fmt);
		return ISF_ERR_INVALID_VALUE;
	}
	
	addr[0] = y;
	addr[1] = uv;

	if(!rgn || cnt <= 0)
		return ISF_OK;

	if(!y || !uv || !w || !h){
		DBG_ERR("invalid arguments : y(%p) uv(%p) w(%d) h(%d)\r\n", (void*)y, (void*)uv, w, h);
		return ISF_ERR_INVALID_VALUE;
	}

	vds_memset(&dst, 0, sizeof(VDO_FRAME));
	if(gximg_init_buf_ex(&dst, w, h, fmt, loff, addr) != ISF_OK){
		DBG_ERR("gximg_init_buf_ex() fail\r\n");
		return ISF_ERR_INVALID_VALUE;
	}

	for(i = 0 ; i < cnt ; ++i){

		if(!rgn[i].en)
			continue;

		if(!rgn[i].addr || !rgn[i].size.w || !rgn[i].size.h){
			DBG_ERR("invalid rgn[%d] buffer : addr(%p) w(%d) h(%d)\r\n", i, (void*)rgn[i].addr, rgn[i].size.w, rgn[i].size.h);
			return ISF_ERR_INVALID_STATE;
		}

		if(rgn[i].fmt != VDO_PXLFMT_RGB565   &&
		   rgn[i].fmt != VDO_PXLFMT_ARGB1555 &&
		   rgn[i].fmt != VDO_PXLFMT_ARGB4444 &&
		   rgn[i].fmt != VDO_PXLFMT_ARGB8888 &&
		   rgn[i].fmt != VDO_PXLFMT_I1       &&
		   rgn[i].fmt != VDO_PXLFMT_I2       &&
		   rgn[i].fmt != VDO_PXLFMT_I4){
			DBG_ERR("invalid rgn[%d] format(%d)\r\n", i, rgn[i].fmt);
			return ISF_ERR_INVALID_STATE;
		}

		if (rgn[i].fmt == VDO_PXLFMT_ARGB8888) {
			size = rgn[i].size.w * rgn[i].size.h * 4;
		} else {
			size = rgn[i].size.w * rgn[i].size.h * 2;
		}

		vds_memset(&r_img, 0, sizeof(VDO_FRAME));
		if (gximg_init_buf( &r_img, rgn[i].size.w, rgn[i].size.h, rgn[i].fmt, 0, rgn[i].addr, size) != ISF_OK) {
			DBG_ERR("gximg_init_buf() fail\r\n");
			return ISF_ERR_INVALID_VALUE;
		}

		pt.x = rgn[i].pos.x;
		pt.y = rgn[i].pos.y;
		if(gximg_argb_to_yuv_blend_no_flush(&r_img, GXIMG_REGION_MATCH_IMG, &dst, &pt, rgn[i].alpha, palette, 0) != ISF_OK){
			DBG_ERR("fail to render osg\r\n");
			return ISF_ERR_INVALID_VALUE;
		}
	}

	return ISF_OK;
}

static int render_incontinuous_mask(VDS_INTERNAL_EXT_MASK* mask,
									UINT32 y_addr, UINT32 uv_addr, UINT32 w, UINT32 loff, UINT32 h)
{
		unsigned char *p_y;
		unsigned char *p_u;
		unsigned char cv;
		unsigned char cu;
		unsigned char cy;
		int x, y, h_thickness, v_thickness, h_line_len, h_hole_len, v_line_len, v_hole_len;
		int i, j;

		if(!mask){
			DBG_ERR("mask is NULL\n");
			return -1;
		}

		if(w > loff){
			DBG_ERR("w(%d) can't be larger than loff(%d)\n", w, loff);
		}


		cv           = ((mask->color & 0xFF0000)>>16);
		cu           = ((mask->color & 0xFF00)>>8);
		cy           = (mask->color & 0xFF);
		x            = mask->data.incontinuous.pos.x;
		y            = mask->data.incontinuous.pos.y;
		h_thickness  = mask->data.incontinuous.h_thickness;
		v_thickness  = mask->data.incontinuous.v_thickness;
		h_line_len   = mask->data.incontinuous.h_line_len;
		h_hole_len   = mask->data.incontinuous.h_hole_len;
		v_line_len   = mask->data.incontinuous.v_line_len;
		v_hole_len   = mask->data.incontinuous.v_hole_len;

		if(x < 0 || y < 0){
			DBG_ERR("x(%d) or y(%d) < 0\n", x, y);
			return -1;
		}

		if(x >= (int)w || y >= (int)h){
			DBG_ERR("x(%d) >= w(%d) or y(%d) >= h(%d)\n", x, w, y, h);
			return -1;
		}

		if(h_line_len < 0 || h_hole_len < 0 || h_thickness < 0){
			DBG_ERR("invalid horizon len(%d) hole(%d) thick(%d)\n", h_line_len, h_hole_len, h_thickness);
			return -1;
		}

		if(h_line_len >= (int)w || h_hole_len >= (int)w || h_thickness >= (int)w){
			DBG_ERR("invalid horizon len(%d) or hole(%d) or thick(%d) >= w(%d)\n", h_line_len, h_hole_len, h_thickness, w);
			return -1;
		}

		if(v_line_len < 0 || v_hole_len < 0 || v_thickness < 0){
			DBG_ERR("invalid vertical len(%d) hole(%d) thick(%d)\n", v_line_len, v_hole_len, v_thickness);
			return -1;
		}

		if(v_line_len >= (int)h || v_hole_len >= (int)h || v_thickness >= (int)h){
			DBG_ERR("invalid vertical len(%d) or hole(%d) or thick(%d) >= h(%d)\n", v_line_len, v_hole_len, v_thickness, h);
			return -1;
		}

		if(x + h_line_len * 2 + h_hole_len > (int)w){
			DBG_ERR("x(%d), h line(%d), h hole(%d) > w(%d)\n",
				x, h_line_len, h_hole_len, h);
			return -1;
		}

		if(y + v_line_len * 2 + v_hole_len > (int)h){
			DBG_ERR("y(%d), v line(%d), v hole(%d) > h(%d)\n",
				y, v_line_len, v_hole_len, h);
			return -1;
		}

		if(h_thickness >= v_line_len || v_thickness >= h_line_len){
			DBG_ERR("h_thickness(%d) should be < v line(%d) and v_thickness(%d) should be < h line(%d)\n",
				h_thickness, v_line_len, v_thickness, h_line_len);
			return -1;
		}

		p_y = (unsigned char*)y_addr;
		p_y += (y * loff) + x;
		for(i = 0 ; i < h_thickness ; ++i){
			vds_memset(p_y, cy, h_line_len);
			vds_memset(p_y + h_line_len + h_hole_len, cy, h_line_len);
			vds_memset(p_y + (v_line_len*2 + v_hole_len - h_thickness)*loff, cy, h_line_len);
			vds_memset(p_y + (v_line_len*2 + v_hole_len - h_thickness)*loff + h_line_len + h_hole_len, cy, h_line_len);
			p_y += loff;
		}

		p_y = (unsigned char*)y_addr;
		p_y += (y * loff) + x;
		for(i = 0 ; i < v_line_len ; ++i)
			for(j = 0 ; j < v_thickness ; ++j){
				*(p_y + i*loff + j) = cy;
				*(p_y + i*loff + h_line_len*2 + h_hole_len + j - v_thickness) = cy;
				*(p_y + (i+v_line_len+v_hole_len)*loff + j) = cy;
				*(p_y + (i+v_line_len+v_hole_len)*loff + h_line_len*2 + h_hole_len + j - v_thickness) = cy;
			}

		p_u = (unsigned char*)uv_addr;
		p_u += ((y>>1) * loff) + x;
		for(i = 0 ; i < (h_thickness>>1) ; ++i){
			for(j = 0 ; j < (h_line_len>>1) ; ++j){
				*(p_u + i*loff + (j<<1)) = cu;
				*(p_u + i*loff + (j<<1) + 1) = cv;
				*(p_u + i*loff + (j<<1) + h_line_len + h_hole_len) = cu;
				*(p_u + i*loff + (j<<1) + h_line_len + h_hole_len + 1) = cv;
				*(p_u + (i + ((v_line_len*2 + v_hole_len - h_thickness)>>1))*loff + (j<<1)) = cu;
				*(p_u + (i + ((v_line_len*2 + v_hole_len - h_thickness)>>1))*loff + (j<<1) + 1) = cv;
				*(p_u + (i + ((v_line_len*2 + v_hole_len - h_thickness)>>1))*loff + (j<<1) + h_line_len + h_hole_len) = cu;
				*(p_u + (i + ((v_line_len*2 + v_hole_len - h_thickness)>>1))*loff + (j<<1) + h_line_len + h_hole_len + 1) = cv;
			}
		}

		p_u = (unsigned char*)uv_addr;
		p_u += ((y>>1) * loff) + x;
		for(i = 0 ; i < (v_line_len>>1) ; ++i)
			for(j = 0 ; j < (v_thickness>>1) ; ++j){
				*(p_u + i*loff + (j<<1)) = cu;
				*(p_u + i*loff + (j<<1) + 1) = cv;
				*(p_u + i*loff + (j<<1) + h_line_len*2 + h_hole_len - v_thickness) = cu;
				*(p_u + i*loff + (j<<1) + h_line_len*2 + h_hole_len - v_thickness + 1) = cv;
				*(p_u + (i+((v_line_len+v_hole_len)>>1))*loff + (j<<1)) = cu;
				*(p_u + (i+((v_line_len+v_hole_len)>>1))*loff + (j<<1) + 1) = cv;
				*(p_u + (i+((v_line_len+v_hole_len)>>1))*loff + (j<<1) + h_line_len*2 + h_hole_len - v_thickness) = cu;
				*(p_u + (i+((v_line_len+v_hole_len)>>1))*loff + (j<<1) + h_line_len*2 + h_hole_len - v_thickness + 1) = cv;
			}

	return 0;
}

static int render_graph_mask(VDS_INTERNAL_EXT_MASK* mask, int cnt, UINT32 y, UINT32 uv, UINT32 w, UINT32 h, UINT32 *lineoffset, VDO_PXLFMT fmt)
{
	VDO_FRAME                dst;
	UINT32                   loff[2] = {0}, addr[2] = {0};
	GXIMG_COVER_DESC         desc;
	IRECT                    area;
	int                      i;

	if(lineoffset){
		loff[0] = lineoffset[0];
		loff[1] = lineoffset[1];
	}else{
		loff[0] = w;
		loff[1] = w;
	}

	if(fmt != VDO_PXLFMT_YUV420 && fmt != VDO_PXLFMT_YUV422){
		DBG_ERR("stream format(%x) is not supported. only yuv420 and yuv422 are supported\n", fmt);
		return ISF_ERR_INVALID_VALUE;
	}
	
	addr[0] = y;
	addr[1] = uv;

	if(!mask || cnt <= 0)
		return ISF_OK;

	if(!y || !uv || !w || !h){
		DBG_ERR("invalid arguments : y(%p) uv(%p) w(%d) h(%d)\r\n", (void*)y, (void*)uv, w, h);
		return ISF_ERR_INVALID_VALUE;
	}

	vds_memset(&dst, 0, sizeof(VDO_FRAME));
	if(gximg_init_buf_ex(&dst, w, h, fmt, loff, addr) != ISF_OK){
		DBG_ERR("gximg_init_buf_ex() fail\r\n");
		return ISF_ERR_INVALID_VALUE;
	}

	for(i = 0 ; i < cnt ; ++i){

		if(!mask[i].en)
			continue;

		if(mask[i].type == VDS_INTERNAL_MASK_TYPE_SOLID ||
		   mask[i].type == VDS_INTERNAL_MASK_TYPE_HOLLOW ||
		   mask[i].is_mosaic){
			vds_memset(&desc, 0, sizeof(GXIMG_COVER_DESC));
			desc.top_left.x            = mask[i].data.normal.pos[0].x;
			desc.top_left.y            = mask[i].data.normal.pos[0].y;
			desc.top_right.x           = mask[i].data.normal.pos[1].x;
			desc.top_right.y           = mask[i].data.normal.pos[1].y;
			desc.bottom_right.x        = mask[i].data.normal.pos[2].x;
			desc.bottom_right.y        = mask[i].data.normal.pos[2].y;
			desc.bottom_left.x         = mask[i].data.normal.pos[3].x;
			desc.bottom_left.y         = mask[i].data.normal.pos[3].y;
			if(mask[i].is_mosaic){
				desc.yuva              = 0;
				desc.mosaic_blk_size.w = mask[i].data.normal.mosaic_blk_size;
				desc.mosaic_blk_size.h = mask[i].data.normal.mosaic_blk_size;
			}else{
				desc.yuva              = (((mask[i].data.normal.alpha & 0xff) << 24) | mask[i].color);
			}

			area.x = 0;
			area.y = 0;
			area.w = dst.size.w;
			area.h = dst.size.h;

			if(gximg_quad_cover_no_flush(&desc, &dst, &area, mask[i].type == VDS_INTERNAL_MASK_TYPE_HOLLOW, mask[i].thickness) != ISF_OK){
				DBG_ERR("gximg_quad_cover_no_flush() fail\r\n");
				return ISF_ERR_INVALID_VALUE;
			}
		}else if(mask[i].type == VDS_INTERNAL_MASK_TYPE_INCONTINUOUS){
			if(render_incontinuous_mask(&mask[i], y, uv, w, loff[0], h)){
				DBG_ERR("render_incontinuous_mask() fail\r\n");
				return ISF_ERR_INVALID_VALUE;
			}
		}
	}

	return ISF_OK;
}

int vds_render_ime_context(VDS_QUERY_STAGE stage, UINT32 y, UINT32 uv, void* p_data, UINT32 w, UINT32 h, UINT32 *palette, UINT32 *loff)
{
	if(stage == VDS_QS_IME_EXT_STAMP)
		return render_graph_stamp(((VDS_TO_IME_EXT_STAMP*)p_data)->data.stamp, vds_max_ime_ext_stamp, y, uv, w, h, palette, loff, VDO_PXLFMT_YUV420);
	else if(stage == VDS_QS_IME_EXT_MASK)
		return render_graph_mask(((VDS_TO_IME_EXT_MASK*)p_data)->data.mask, vds_max_ime_ext_mask, y, uv, w, h, loff, VDO_PXLFMT_YUV420);

	DBG_ERR("unknown stage = %d\r\n", stage);
	return ISF_ERR_INVALID_VALUE;
}

int vds_render_enc_ext(VDS_QUERY_STAGE stage, UINT32 y, UINT32 uv, void* p_data, UINT32 w, UINT32 h, UINT32 *palette, UINT32 *loff)
{
	if(stage == VDS_QS_ENC_EXT_STAMP)
		return render_graph_stamp(((VDS_TO_ENC_EXT_STAMP*)p_data)->stamp, vds_max_coe_ext_stamp, y, uv, w, h, palette, loff, VDO_PXLFMT_YUV420);
	else if(stage == VDS_QS_ENC_EXT_MASK)
		return render_graph_mask(((VDS_TO_ENC_EXT_MASK*)p_data)->mask, vds_max_coe_ext_mask, y, uv, w, h, loff, VDO_PXLFMT_YUV420);

	DBG_ERR("unknown stage = %d\r\n", stage);
	return ISF_ERR_INVALID_VALUE;
}

int vds_render_vo(VDS_QUERY_STAGE stage, UINT32 y, UINT32 uv, void* p_data, UINT32 w, UINT32 h, UINT32 *palette, UINT32 *loff, VDO_PXLFMT fmt)
{
	if(stage == VDS_QS_VO_STAMP)
		return render_graph_stamp(((VDS_TO_VO_STAMP*)p_data)->stamp, vds_max_vo_stamp, y, uv, w, h, palette, loff, fmt);
	else if(stage == VDS_QS_VO_MASK)
		return render_graph_mask(((VDS_TO_VO_MASK*)p_data)->mask, vds_max_vo_mask, y, uv, w, h, loff, fmt);

	DBG_ERR("unknown stage = %d\r\n", stage);
	return ISF_ERR_INVALID_VALUE;
}

int vds_render_coe_grh(UINT32 y, UINT32 uv, VDS_INTERNAL_COE_STAMP_MASK *p_data, UINT32 w, UINT32 h, UINT32 *palette, UINT32 *loff)
{
	int i, ret;
	VDS_INTERNAL_EXT_STAMP *rgn  = (void *)render_coe_buffer;
	VDS_INTERNAL_EXT_MASK  *mask = (void *)render_coe_buffer;

	if(!p_data || !w || !h){
		DBG_ERR("invalid arguments : image(%p) w(%d h(%d)\r\n", p_data, w, h);
		return ISF_ERR_INVALID_VALUE;
	}
	
	if(!rgn || !mask){
		DBG_ERR("internal buffer is NULL\r\n");
		return ISF_ERR_INVALID_VALUE;
	}

	vds_memset(rgn, 0, sizeof(VDS_INTERNAL_EXT_STAMP) * vds_max_coe_stamp);
	for(i = 0 ; i < vds_max_coe_stamp ; ++i){

		if(!p_data[i].en || p_data[i].is_mask)
			continue;

		rgn[i].en        = p_data[i].en;
		rgn[i].fmt       = p_data[i].data.stamp.fmt;
		rgn[i].size.w    = p_data[i].data.stamp.size.w;
		rgn[i].size.h    = p_data[i].data.stamp.size.h;
		rgn[i].pos.x     = p_data[i].data.stamp.pos.x;
		rgn[i].pos.y     = p_data[i].data.stamp.pos.y;
		rgn[i].alpha     = p_data[i].data.stamp.fg_alpha;
		rgn[i].addr      = p_data[i].data.stamp.addr;
	}
	ret = render_graph_stamp(rgn, vds_max_coe_stamp, y, uv, w, h, palette, loff, VDO_PXLFMT_YUV420);
	if(ret){
		DBG_ERR("fail to render encoder osg\n");
		return ret;
	}

	vds_memset(mask, 0, sizeof(VDS_INTERNAL_EXT_MASK) * vds_max_coe_stamp);
	for(i = 0 ; i < vds_max_coe_stamp ; ++i){

		if(!p_data[i].en || !p_data[i].is_mask)
			continue;

		mask[i].en                   = p_data[i].en;
		mask[i].type                 = p_data[i].data.mask.type;
		mask[i].color                = p_data[i].data.mask.color;
		mask[i].data.normal.pos[0].x = p_data[i].data.mask.x;
		mask[i].data.normal.pos[0].y = p_data[i].data.mask.y;
		mask[i].data.normal.pos[1].x = (mask[i].data.normal.pos[0].x + p_data[i].data.mask.w);
		mask[i].data.normal.pos[1].y = mask[i].data.normal.pos[0].y;
		mask[i].data.normal.pos[2].x = mask[i].data.normal.pos[1].x;
		mask[i].data.normal.pos[2].y = (mask[i].data.normal.pos[1].y + p_data[i].data.mask.h);
		mask[i].data.normal.pos[3].x = mask[i].data.normal.pos[0].x;
		mask[i].data.normal.pos[3].y = mask[i].data.normal.pos[2].y;
		mask[i].data.normal.alpha    = p_data[i].data.mask.alpha;
		mask[i].thickness            = p_data[i].data.mask.thickness;
	}
	ret = render_graph_mask(mask, vds_max_coe_stamp, y, uv, w, h, loff, VDO_PXLFMT_YUV420);
	if(ret){
		DBG_ERR("fail to render encoder mask\n");
		return ret;
	}

	return ret;
}

UINT32* vds_get_ime_palette(int vid)
{
	if(!ime_palette || !vds_max_ime){
		DBG_ERR("ime palette(%p) or channel(%d) is NULL\n", ime_palette, vds_max_ime);
		return NULL;
	}

	if(vid >= vds_max_ime){
		DBG_ERR("ime vid(%d) >= channel(%d)\n", vid, vds_max_ime);
		return NULL;
	}

	return &(ime_palette[vid*16]);
}

UINT32* vds_get_coe_palette(int vid)
{
	if(!coe_palette || !vds_max_coe_vid){
		//suppress this message for ctrl+c
		//because when ctrl+c is pressed, osg is uninit but videoenc is still running
		//DBG_ERR("coe palette(%p) or vid(%d) is NULL\n", coe_palette, vds_max_coe_vid);
		return NULL;
	}

	if(vid >= vds_max_coe_vid){
		DBG_ERR("coe vid(%d) >= vid(%d)\n", vid, vds_max_coe_vid);
		return NULL;
	}

	return &(coe_palette[vid*16]);
}

UINT32* vds_get_vo_palette(int vid)
{
	if(!vo_palette || !vds_max_vo_vid){
		DBG_ERR("vo palette(%p) or vid(%d) is NULL\n", vo_palette, vds_max_vo_vid);
		return NULL;
	}

	if(vid >= vds_max_vo_vid){
		DBG_ERR("vo vid(%d) >= vid(%d)\n", vid, vds_max_vo_vid);
		return NULL;
	}

	return &(vo_palette[vid*16]);
}

void vds_save_latency(VDS_QUERY_STAGE stage, UINT32 id, UINT32 lock, UINT64 time)
{
	unsigned long  flag;
	int            i, min = 0, max = 0;
	UINT64         *avg = NULL;
	LATENCY_RECORD *min_rec = NULL, *max_rec = NULL;
	
	//select min, avg, max record
	if(stage == VDS_QS_IME_STAMP){
		min_rec = min_proc_stamp_latency;
		max_rec = max_proc_stamp_latency;
		avg     = &avg_proc_stamp_latency;
	}else if(stage == VDS_QS_IME_MASK){
		min_rec = min_proc_mask_latency;
		max_rec = max_proc_mask_latency;
		avg     = &avg_proc_mask_latency;
	}else if(stage == VDS_QS_IME_EXT_STAMP){
		min_rec = min_proc_ext_stamp_latency;
		max_rec = max_proc_ext_stamp_latency;
		avg     = &avg_proc_ext_stamp_latency;
	}else if(stage == VDS_QS_IME_EXT_MASK){
		min_rec = min_proc_ext_mask_latency;
		max_rec = max_proc_ext_mask_latency;
		avg     = &avg_proc_ext_mask_latency;
	}else if(stage == VDS_QS_ENC_EXT_STAMP){
		min_rec = min_enc_ext_stamp_latency;
		max_rec = max_enc_ext_stamp_latency;
		avg     = &avg_enc_ext_stamp_latency;
	}else if(stage == VDS_QS_ENC_EXT_MASK){
		min_rec = min_enc_ext_mask_latency;
		max_rec = max_enc_ext_mask_latency;
		avg     = &avg_enc_ext_mask_latency;
	}else if(stage == VDS_QS_ENC_COE_STAMP){
		min_rec = min_enc_stamp_latency;
		max_rec = max_enc_stamp_latency;
		avg     = &avg_enc_stamp_latency;
	}else if(stage == VDS_QS_ENC_JPG_STAMP){
		min_rec = min_jpg_stamp_latency;
		max_rec = max_jpg_stamp_latency;
		avg     = &avg_jpg_stamp_latency;
	}else if(stage == VDS_QS_VO_STAMP){
		min_rec = min_out_stamp_latency;
		max_rec = max_out_stamp_latency;
		avg     = &avg_out_stamp_latency;
	}else if(stage == VDS_QS_VO_MASK){
		min_rec = min_out_mask_latency;
		max_rec = max_out_mask_latency;
		avg     = &avg_out_mask_latency;
	}else{
		DBG_ERR("stage(%d) is unknown\n", stage);
		return;
	}
	
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	
	//find minimum latency in max records
	for(i = 1 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_rec[i].time < max_rec[max].time)
			max = i;
	}
	//if minimum latency is smaller, replace it
	if(max_rec[max].time < time){
		max_rec[max].stage = stage;
		max_rec[max].vid   = id;
		max_rec[max].lock  = lock;
		max_rec[max].time  = time;
	}
	
	//find maximum latency in min records
	for(i = 1 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_rec[i].time == 0){
			min = i;
			break;
		}
		if(min_rec[i].time > min_rec[min].time)
			min = i;
	}
	//if maximum latency is larger, replace it
	if(min_rec[min].time > time || min_rec[min].time == 0){
		min_rec[min].stage = stage;
		min_rec[min].vid   = id;
		min_rec[min].lock  = lock;
		min_rec[min].time  = time;
	}
	
	*avg = ((*avg * (MAX_LATENCY_RECORD>>1) + time) >> 4);

	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
}

void vds_latency_show(void *m, void *v, int clear)
{
	unsigned long flag;
	int           i;
	char          msg[512];
	
	if(!m)
		goto end;
	
	//videoprocess stamp latency
	vds_seq_printf(m, "videoproc max stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_proc_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_proc_stamp_latency[i].time, 
					max_proc_stamp_latency[i].vid,
					max_proc_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "videoproc avg stamp:time(%llu) us\n", avg_proc_stamp_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "videoproc min stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_proc_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_proc_stamp_latency[i].time, 
					min_proc_stamp_latency[i].vid,
					min_proc_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//videoprocess mask latency
	vds_seq_printf(m, "videoproc max mask:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_proc_mask_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_proc_mask_latency[i].time, 
					max_proc_mask_latency[i].vid,
					max_proc_mask_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "videoproc avg mask:time(%llu) us\n", avg_proc_mask_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "videoproc min mask:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_proc_mask_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_proc_mask_latency[i].time, 
					min_proc_mask_latency[i].vid,
					min_proc_mask_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//videoprocess ext stamp latency
	vds_seq_printf(m, "videoproc max ext stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_proc_ext_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_proc_ext_stamp_latency[i].time, 
					max_proc_ext_stamp_latency[i].vid,
					max_proc_ext_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "videoproc avg ext stamp:time(%llu) us\n", avg_proc_ext_stamp_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "videoproc min ext stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_proc_ext_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_proc_ext_stamp_latency[i].time, 
					min_proc_ext_stamp_latency[i].vid,
					min_proc_ext_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//videoprocess ext mask latency
	vds_seq_printf(m, "videoproc max ext mask:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_proc_ext_mask_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_proc_ext_mask_latency[i].time, 
					max_proc_ext_mask_latency[i].vid,
					max_proc_ext_mask_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "videoproc avg ext mask:time(%llu) us\n", avg_proc_ext_mask_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "videoproc min ext mask:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_proc_ext_mask_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_proc_ext_mask_latency[i].time, 
					min_proc_ext_mask_latency[i].vid,
					min_proc_ext_mask_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//venc ext stamp latency
	vds_seq_printf(m, "venc max ext stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_enc_ext_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_enc_ext_stamp_latency[i].time, 
					max_enc_ext_stamp_latency[i].vid,
					max_enc_ext_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "venc avg ext stamp:time(%llu) us\n", avg_enc_ext_stamp_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "venc min ext stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_enc_ext_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_enc_ext_stamp_latency[i].time, 
					min_enc_ext_stamp_latency[i].vid,
					min_enc_ext_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//venc ext mask latency
	vds_seq_printf(m, "venc max ext mask:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_enc_ext_mask_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_enc_ext_mask_latency[i].time, 
					max_enc_ext_mask_latency[i].vid,
					max_enc_ext_mask_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "venc avg ext mask:time(%llu) us\n", avg_enc_ext_mask_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "venc min ext mask:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_enc_ext_mask_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_enc_ext_mask_latency[i].time, 
					min_enc_ext_mask_latency[i].vid,
					min_enc_ext_mask_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//venc stamp latency
	vds_seq_printf(m, "venc max stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_enc_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_enc_stamp_latency[i].time, 
					max_enc_stamp_latency[i].vid,
					max_enc_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "venc avg stamp:time(%llu) us\n", avg_enc_stamp_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "venc min stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_enc_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_enc_stamp_latency[i].time, 
					min_enc_stamp_latency[i].vid,
					min_enc_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//jpg stamp latency
	vds_seq_printf(m, "jpg max stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_jpg_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_jpg_stamp_latency[i].time, 
					max_jpg_stamp_latency[i].vid,
					max_jpg_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "jpg avg stamp:time(%llu) us\n", avg_jpg_stamp_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "jpg min stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_jpg_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_jpg_stamp_latency[i].time, 
					min_jpg_stamp_latency[i].vid,
					min_jpg_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//vout stamp latency
	vds_seq_printf(m, "vout max stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_out_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_out_stamp_latency[i].time, 
					max_out_stamp_latency[i].vid,
					max_out_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "vout avg stamp:time(%llu) us\n", avg_out_stamp_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "vout min stamp:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_out_stamp_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_out_stamp_latency[i].time, 
					min_out_stamp_latency[i].vid,
					min_out_stamp_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	
	//vout mask latency
	vds_seq_printf(m, "vout max mask:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(max_out_mask_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					max_out_mask_latency[i].time, 
					max_out_mask_latency[i].vid,
					max_out_mask_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	vds_snprintf(msg, sizeof(msg), "vout avg mask:time(%llu) us\n", avg_out_mask_latency);
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
	vds_seq_printf(m, msg);
	vds_seq_printf(m, "vout min mask:\n");
	for(i = 0 ; i < MAX_LATENCY_RECORD ; ++i){
		if(min_out_mask_latency[i].time == 0)
			continue;
		vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
		vds_snprintf(msg, sizeof(msg), "time(%llu) us, vid(%d), lock(%d)\n",
					min_out_mask_latency[i].time, 
					min_out_mask_latency[i].vid,
					min_out_mask_latency[i].lock);
		vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
		vds_seq_printf(m, msg);
	}

end:
	vk_spin_lock_irqsave(&VDS_LATENCY_LOCK_ID, flag);
	if(clear){	
		vds_memset(min_proc_stamp_latency, 0, sizeof(min_proc_stamp_latency));
		vds_memset(max_proc_stamp_latency, 0, sizeof(max_proc_stamp_latency));
		vds_memset(min_proc_mask_latency, 0, sizeof(min_proc_mask_latency));
		vds_memset(max_proc_mask_latency, 0, sizeof(max_proc_mask_latency));
		vds_memset(min_proc_ext_stamp_latency, 0, sizeof(min_proc_ext_stamp_latency));
		vds_memset(max_proc_ext_stamp_latency, 0, sizeof(max_proc_ext_stamp_latency));
		vds_memset(min_proc_ext_mask_latency, 0, sizeof(min_proc_ext_mask_latency));
		vds_memset(max_proc_ext_mask_latency, 0, sizeof(max_proc_ext_mask_latency));
		vds_memset(min_enc_stamp_latency, 0, sizeof(min_enc_stamp_latency));
		vds_memset(max_enc_stamp_latency, 0, sizeof(max_enc_stamp_latency));
		vds_memset(min_jpg_stamp_latency, 0, sizeof(min_jpg_stamp_latency));
		vds_memset(max_jpg_stamp_latency, 0, sizeof(max_jpg_stamp_latency));
		vds_memset(min_enc_ext_stamp_latency, 0, sizeof(min_enc_ext_stamp_latency));
		vds_memset(max_enc_ext_stamp_latency, 0, sizeof(max_enc_ext_stamp_latency));
		vds_memset(min_enc_ext_mask_latency, 0, sizeof(min_enc_ext_mask_latency));
		vds_memset(max_enc_ext_mask_latency, 0, sizeof(max_enc_ext_mask_latency));
		vds_memset(min_out_stamp_latency, 0, sizeof(min_out_stamp_latency));
		vds_memset(max_out_stamp_latency, 0, sizeof(max_out_stamp_latency));
		vds_memset(min_out_mask_latency, 0, sizeof(min_out_mask_latency));
		vds_memset(max_out_mask_latency, 0, sizeof(max_out_mask_latency));
		
		avg_proc_stamp_latency = 0;
		avg_proc_mask_latency = 0;
		avg_proc_ext_stamp_latency = 0;
		avg_proc_ext_mask_latency = 0;
		avg_enc_stamp_latency = 0;
		avg_jpg_stamp_latency = 0;
		avg_enc_ext_stamp_latency = 0;
		avg_enc_ext_mask_latency = 0;
		avg_out_stamp_latency = 0;
		avg_out_mask_latency = 0;
	}
	vk_spin_unlock_irqrestore(&VDS_LATENCY_LOCK_ID, flag);
}

int nvt_vds_ioctl (int fd, unsigned int cmd, void *arg)
{
	VDS_USER_DATA    user_data;
	int              ret = ISF_ERR_INVALID_VALUE;

#if defined(__LINUX)
	if(vds_copy_from_user(&user_data, arg, sizeof(user_data))){
		DBG_ERR("copy_from_user() fail\r\n");
		return ISF_ERR_COPY_FROM_USER;
	}
#else
	vds_memcpy(&user_data, arg, sizeof(user_data));
#endif

	if(user_data.version != VDS_USER_VERSION){
		DBG_ERR("mismatch version : lib =%d ; ko = %d\r\n", user_data.version, VDS_USER_VERSION);
		return ISF_ERR_INVALID_VALUE;
	}

	if(user_data.cmd <= VDS_USER_CMD_NULL || user_data.cmd >= VDS_USER_CMD_MAX){
		DBG_ERR("invalid command = %d\r\n", user_data.cmd);
		return ISF_ERR_INVALID_VALUE;
	}

	switch(user_data.cmd){

		case VDS_USER_CMD_START :

			ret = vds_start(user_data.phase, user_data.rgn, user_data.vid);

			break;

		case VDS_USER_CMD_STOP :

			ret = vds_stop(user_data.phase, user_data.rgn, user_data.vid);

			break;

		case VDS_USER_CMD_GET_STAMP_BUF :

			ret = vds_get_stamp_buf(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.buf.type, &user_data.data.buf.size, &user_data.data.buf.p_addr);
			if(ret == ISF_OK) {
#if defined(__LINUX)
				if (vds_copy_to_user((void*)arg, &user_data, sizeof(user_data)))
					ret = ISF_ERR_COPY_TO_USER;
#else
				vds_memcpy(arg, &user_data, sizeof(user_data));
#endif
			}
			break;

		case VDS_USER_CMD_SET_STAMP_BUF :

			ret = vds_set_stamp_buf(user_data.phase, user_data.rgn, user_data.vid, user_data.data.buf.type, user_data.data.buf.size, user_data.data.buf.p_addr);

			break;

		case VDS_USER_CMD_GET_STAMP_IMG :

			ret = vds_get_stamp_img(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.img.fmt,
									&user_data.data.img.w, &user_data.data.img.h, &user_data.data.img.p_addr, user_data.data.img.copy);
			if(ret == ISF_OK) {
#if defined(__LINUX)
				if (vds_copy_to_user((void*)arg, &user_data, sizeof(user_data)))
					ret = ISF_ERR_COPY_TO_USER;
#else
				vds_memcpy(arg, &user_data, sizeof(user_data));
#endif
			}
			break;

		case VDS_USER_CMD_SET_STAMP_IMG :

			ret = vds_set_stamp_img(user_data.phase, user_data.rgn, user_data.vid, user_data.data.img.fmt, user_data.data.img.w, user_data.data.img.h, user_data.data.img.p_addr);

			break;

		case VDS_USER_CMD_GET_STAMP_ATTR :

			ret = vds_get_stamp_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.stamp);
			if(ret == ISF_OK) {
#if defined(__LINUX)
				if (vds_copy_to_user((void*)arg, &user_data, sizeof(user_data)))
					ret = ISF_ERR_COPY_TO_USER;
#else
				vds_memcpy(arg, &user_data, sizeof(user_data));
#endif
			}
			break;

		case VDS_USER_CMD_SET_STAMP_ATTR :

			ret = vds_set_stamp_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.stamp);

			break;

		case VDS_USER_CMD_GET_MASK_ATTR :

			ret = vds_get_mask_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.mask);
			if(ret == ISF_OK) {
#if defined(__LINUX)
				if (vds_copy_to_user((void*)arg, &user_data, sizeof(user_data)))
					ret = ISF_ERR_COPY_TO_USER;
#else
				vds_memcpy(arg, &user_data, sizeof(user_data));
#endif
			}

			break;

		case VDS_USER_CMD_SET_MASK_ATTR :

			ret = vds_set_mask_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.mask);

			break;

		case VDS_USER_CMD_GET_MOSAIC_ATTR :

			ret = vds_get_mosaic_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.mosaic);
			if(ret == ISF_OK) {
#if defined(__LINUX)
				if (vds_copy_to_user((void*)arg, &user_data, sizeof(user_data)))
					ret = ISF_ERR_COPY_TO_USER;
#else
				vds_memcpy(arg, &user_data, sizeof(user_data));
#endif
			}
			break;

		case VDS_USER_CMD_SET_MOSAIC_ATTR :

			ret = vds_set_mosaic_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.mosaic);

			break;

		case VDS_USER_CMD_CLOSE :

			ret = vds_close(user_data.phase, user_data.rgn, user_data.vid);

			break;

		case VDS_USER_CMD_RESET :

			ret = vds_reset();

			break;
			
		case VDS_USER_CMD_SET_CFG_MAX :

			ret = vds_set_cfg_max(&user_data.data.cfg);
			
			break;

		case VDS_USER_CMD_SET_QP :

			ret = vds_set_stamp_qp(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.qp);

			break;

		case VDS_USER_CMD_SET_COLOR_INVERT :

			ret = vds_set_stamp_color_invert(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.color_invert);

			break;

		case VDS_USER_CMD_SET_ENCODER_BUILTIN_MASK :

			ret = vds_set_enc_mask_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.enc_mask);

			break;

		case VDS_USER_CMD_GET_PALETTE :

			ret = vds_get_palette_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.palette);

			break;

		case VDS_USER_CMD_SET_PALETTE :

			ret = vds_set_palette_attr(user_data.phase, user_data.rgn, user_data.vid, &user_data.data.palette);

			break;

		case VDS_USER_CMD_OPEN :

			ret = vds_open(user_data.phase, user_data.rgn, user_data.vid);

			break;

		default :
			DBG_ERR("unsupported command = %d\r\n", user_data.cmd);
			break;
	};

	return ret;
}

int vds_cmd_show(void *m, void *v) {

	vds_seq_printf(m, 
#if defined(__LINUX)
	"For more help, cat /proc/hdal/osg/help\n"
#else
	"For more help, execute \"osg help\" under \">\"\n"
#endif
	);

	return 0;
}

static char *get_buf_info_header(void)
{
	return "pid\ttype\tfmt\tw\th\taddr        size\tdraw\n";
}

static char *get_stamp_info_header(void)
{
	return "pid\tstart\tx\ty\talpha\tcken\tckval\tlayer\trgn\n";
}

static char *get_mask_info_header(void)
{
	return "pid\tstart\tx\ty\tw\th\tsolid\tthick\tcolor       \talpha\n";
}

static char *get_mosaic_info_header(void)
{
	return "pid\tstart\tx\ty\tw\th\tblkw\tblkh\n";
}

static char *get_fmt_str(VDS_IMG_FMT fmt)
{
	if(fmt == VDS_IMG_FMT_PICTURE_RGB565)
		return "565";
	else if(fmt == VDS_IMG_FMT_PICTURE_ARGB1555)
		return "1555";
	else if(fmt == VDS_IMG_FMT_PICTURE_ARGB4444)
		return "4444";
	else
		return "";
}

static int check_rgn_empty(VDS_RGN *rgn)
{
	int i;
	unsigned char *ptr;

	if(!rgn)
		return 1;

	ptr = (unsigned char*)rgn;
	for(i = 0 ; i < (int)sizeof(VDS_RGN) ; ++i)
		if(ptr[i])
			return 0;

	return 1;
}

static int vp_stamp_empty(int vid)
{
	int i;

	if(vid >= vds_max_ime)
		return 1;

	for(i = 0 ; i < vds_max_ime_stamp ; ++i)
		if(!check_rgn_empty(&ime_stamps[vid*vds_max_ime_stamp + i]))
			return 0;

	return 1;
}

static int vp_mask_empty(int vid)
{
	int i;

	if(vid >= vds_max_ime)
		return 1;

	for(i = 0 ; i < vds_max_ime_mask ; ++i){
		if(ime_masks[vid*vds_max_ime_mask + i].is_mosaic)
			continue;
		if(!check_rgn_empty(&ime_masks[vid*vds_max_ime_mask + i]))
			return 0;
	}

	return 1;
}

static int vp_mosaic_empty(int vid)
{
	int i;

	if(vid >= vds_max_ime)
		return 1;

	for(i = 0 ; i < vds_max_ime_mask ; ++i){
		if(!ime_masks[vid*vds_max_ime_mask + i].is_mosaic)
			continue;
		if(!check_rgn_empty(&ime_masks[vid*vds_max_ime_mask + i]))
			return 0;
	}

	return 1;
}

static int vp_ext_stamp_empty(int ime_dev, int ime_port)
{
	int i, vid;

	vid = (ime_dev * vds_max_ime + ime_port);

	if(vid >= vds_max_ime_ext)
		return 1;

	for(i = 0 ; i < vds_max_ime_ext_stamp ; ++i)
		if(!check_rgn_empty(&ime_ext_stamps[vid*vds_max_ime_ext_stamp + i]))
			return 0;

	return 1;
}

static int vp_ext_mask_empty(int ime_dev, int ime_port)
{
	int i, vid;

	vid = (ime_dev * vds_max_ime + ime_port);

	if(vid >= vds_max_ime_ext)
		return 1;

	for(i = 0 ; i < vds_max_ime_ext_mask ; ++i)
		if(!check_rgn_empty(&ime_ext_masks[vid*vds_max_ime_ext_mask + i]))
			return 0;

	return 1;
}

static int ve_stamp_empty(int vid)
{
	int i;

	if(vid >= vds_max_coe_vid)
		return 1;

	for(i = 0 ; i < vds_max_coe_stamp ; ++i)
		if(!check_rgn_empty(&coe_stamps[vid*vds_max_coe_stamp + i]))
			return 0;

	return 1;
}

static int ve_ext_stamp_empty(int vid)
{
	int i;

	if(vid >= vds_max_coe_ext_vid)
		return 1;

	for(i = 0 ; i < vds_max_coe_ext_stamp ; ++i)
		if(!check_rgn_empty(&coe_ext_stamps[vid*vds_max_coe_ext_stamp + i]))
			return 0;

	return 1;
}

static int ve_ext_mask_empty(int vid)
{
	int i;

	if(vid >= vds_max_coe_ext_vid)
		return 1;

	for(i = 0 ; i < vds_max_coe_ext_mask ; ++i)
		if(!check_rgn_empty(&coe_ext_masks[vid*vds_max_coe_ext_mask + i]))
			return 0;

	return 1;
}

static int vo_stamp_empty(int vid)
{
	int i;

	if(vid >= vds_max_vo_vid)
		return 1;

	for(i = 0 ; i < vds_max_vo_stamp ; ++i)
		if(!check_rgn_empty(&vo_stamps[vid*vds_max_vo_stamp + i]))
			return 0;

	return 1;
}

static int vo_mask_empty(int vid)
{
	int i;

	if(vid >= vds_max_vo_vid)
		return 1;

	for(i = 0 ; i < vds_max_vo_mask ; ++i)
		if(!check_rgn_empty(&vo_masks[vid*vds_max_vo_mask + i]))
			return 0;

	return 1;
}

static int dump_buf(void *m, VDS_RGN* rgn, int id){
	char              msg[512];
	VDS_BUF           *buf   = NULL;

	if(!rgn || check_rgn_empty(rgn))
		return 0;

	buf = rgn->buf;
	if(buf){
		if(buf->type == VDS_BUF_TYPE_SINGE){
			vds_snprintf(msg, sizeof(msg), "%d\t%s\t%s\t%d\t%d\t%-12x%d\t%d",
				id, "single", get_fmt_str(buf->fmt[0]),
				buf->img_size[0].w, buf->img_size[0].h,
				buf->valid && buf->p_addr[0] ? nvtmpp_sys_va2pa(buf->p_addr[0]) : 0,
				buf->size[0], 1);
			vds_seq_printf(m, msg);
		}
		else if(buf->type == VDS_BUF_TYPE_PING_PONG){
			vds_snprintf(msg, sizeof(msg), "%d\t%s\t%s\t%d\t%d\t%-12x%d\t%d\n",
				id, "pp", get_fmt_str(buf->fmt[0]),
				buf->img_size[0].w, buf->img_size[0].h,
				buf->valid && buf->p_addr[0] ? nvtmpp_sys_va2pa(buf->p_addr[0]) : 0,
				buf->size[0],
				buf->swap ? 1 : 0);
			vds_seq_printf(m, msg);
			vds_snprintf(msg, sizeof(msg), "%d\t%s\t%s\t%d\t%d\t%-12x%d\t%d\n",
				id, "pp", get_fmt_str(buf->fmt[1]),
				buf->img_size[1].w, buf->img_size[1].h,
				buf->valid && buf->p_addr[1] ? nvtmpp_sys_va2pa(buf->p_addr[1]) : 0,
				buf->size[1],
				buf->swap ? 0 : 1);
			vds_seq_printf(m, msg);
		}
	}

	vds_seq_printf(m, "\n");

	return 0;
}

static int dump_stamp(void *m, VDS_RGN* rgn, int id){
	char              msg[512];
	VDS_STAMP_ATTR    *stamp = NULL;

	if(!rgn || check_rgn_empty(rgn) || rgn->is_enc_mask)
		return 0;

	stamp = &rgn->attr.stamp;

	vds_snprintf(msg, sizeof(msg), "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t",
		id, rgn->en, stamp->x, stamp->y, stamp->fg_alpha,
		stamp->colorkey_en, stamp->colorkey_val,
		stamp->layer, stamp->region);
    vds_seq_printf(m, msg);

	vds_seq_printf(m, "\n");

	return 0;
}

static int dump_mask(void *m, VDS_RGN* rgn, int id){
	char              msg[512];
	VDS_MASK_ATTR     *mask  = NULL;

	if(!rgn || check_rgn_empty(rgn))
		return 0;

	mask = &rgn->attr.mask;

	if(mask->type == VDS_MASK_TYPE_SOLID || mask->type == VDS_MASK_TYPE_HOLLOW){
		vds_snprintf(msg, sizeof(msg), "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%-12x\t%d\n",
			id, rgn->en, mask->data.normal.x[0], mask->data.normal.y[0],
			mask->data.normal.x[1] - mask->data.normal.x[0], mask->data.normal.y[2] - mask->data.normal.y[1],
			mask->type == VDS_MASK_TYPE_SOLID ? 1 : 0,
			mask->type == VDS_MASK_TYPE_SOLID ? mask->thickness : 0,
			mask->color, mask->data.normal.alpha);
		vds_seq_printf(m, msg);
	}else if(mask->type == VDS_MASK_TYPE_INCONTINUOUS){
		vds_snprintf(msg, sizeof(msg), "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%-12x\t255\n",
			id, rgn->en, mask->data.incontinuous.x, mask->data.incontinuous.y,
			mask->data.incontinuous.h_line_len*2 + mask->data.incontinuous.h_hole_len,
			mask->data.incontinuous.v_line_len*2 + mask->data.incontinuous.v_hole_len,
			0, mask->data.incontinuous.v_thickness, mask->color);
		vds_seq_printf(m, msg);
	}

	return 0;
}

static int dump_mosaic(void *m, VDS_RGN* rgn, int id){
	char              msg[512];
	VDS_MOSAIC_ATTR   *mosaic  = NULL;

	if(!rgn || check_rgn_empty(rgn))
		return 0;

	mosaic = &rgn->attr.mosaic;

	vds_snprintf(msg, sizeof(msg), "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
		id, rgn->en, mosaic->x[0], mosaic->y[0], mosaic->x[1], mosaic->y[1],
		mosaic->x[2], mosaic->y[2], mosaic->x[3], mosaic->y[3],
		mosaic->mosaic_blk_w, mosaic->mosaic_blk_h);
    vds_seq_printf(m, msg);

	return 0;
}

static int dump_vp_show(void *m, void *v) {

	int  i, j, k, empty;
	char msg[128];

	vds_api_lock();

	if(ime_stamp_cnt && ime_stamps){
		empty = 1;
		for(i = 0 ; i < vds_max_ime ; ++i){
			if(!vp_stamp_empty(i)){
				empty = 0;
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOPROC %d IN BUFFER --------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_buf_info_header());
				for(j = 0 ; j < vds_max_ime_stamp ; ++j)
					dump_buf(m, &ime_stamps[i*vds_max_ime_stamp + j], j);
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOPROC %d IN STAMP ---------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_stamp_info_header());
				for(j = 0 ; j < vds_max_ime_stamp ; ++j)
					dump_stamp(m, &ime_stamps[i*vds_max_ime_stamp + j], j);
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOPROC has no stamp configured\n");
	}
	else
		vds_seq_printf(m, "videoprocess stamp is not enabled\n");

	if(ime_mask_cnt && ime_masks){
		empty = 1;
		for(i = 0 ; i < vds_max_ime ; ++i){
			if(!vp_mask_empty(i)){
				empty = 0;
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOPROC %d MASK -------------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_mask_info_header());
				for(j = 0 ; j < vds_max_ime_mask ; ++j)
					if(!ime_masks[i*vds_max_ime_mask + j].is_mosaic)
						dump_mask(m, &ime_masks[i*vds_max_ime_mask + j], j);
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOPROC has no mask configured\n");
		empty = 1;
		for(i = 0 ; i < vds_max_ime ; ++i){
			if(!vp_mosaic_empty(i)){
				empty = 0;
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOPROC %d MOSAIC -----------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_mosaic_info_header());
				for(j = 0 ; j < vds_max_ime_mask ; ++j)
					if(ime_masks[i*vds_max_ime_mask + j].is_mosaic)
						dump_mosaic(m, &ime_masks[i*vds_max_ime_mask + j], j);
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOPROC has no mosaic configured\n");
	}
	else
		vds_seq_printf(m, "videoprocess mask is not enabled\n");

	if(ime_ext_stamp_cnt && ime_ext_stamps){
		empty = 1;
		for(i = 0 ; i < vds_max_ime ; ++i){
			for(j = 0 ; j < 4 ; ++j){
				if(!vp_ext_stamp_empty(i, j)){
					empty = 0;
					vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOPROC %d:%d EXT BUFFER -------------------------------\n", i, j);
					vds_seq_printf(m, msg);
					vds_seq_printf(m, get_buf_info_header());
					for(k = 0 ; k < vds_max_ime_ext_stamp ; ++k)
						dump_buf(m, &ime_ext_stamps[(i * vds_max_ime + j)*vds_max_ime_ext_stamp + k], k);
					vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOPROC %d:%d EXT STAMP --------------------------------\n", i, j);
					vds_seq_printf(m, msg);
					vds_seq_printf(m, get_stamp_info_header());
					for(k = 0 ; k < vds_max_ime_ext_stamp ; ++k)
						dump_stamp(m, &ime_ext_stamps[(i * vds_max_ime + j)*vds_max_ime_ext_stamp + k], k);
				}
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOPROC has no ext stamp configured\n");
	}
	else
		vds_seq_printf(m, "videoprocess ext stamp is not enabled\n");

	if(ime_ext_mask_cnt && ime_ext_masks){
		empty = 1;
		for(i = 0 ; i < vds_max_ime ; ++i){
			for(j = 0 ; j < 4 ; ++j){
				if(!vp_ext_mask_empty(i, j)){
					empty = 0;
					vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOPROC %d:%d EXT MASK ---------------------------------\n", i, j);
					vds_seq_printf(m, msg);
					vds_seq_printf(m, get_mask_info_header());
					for(k = 0 ; k < vds_max_ime_ext_mask ; ++k)
						dump_mask(m, &ime_ext_masks[(i * vds_max_ime + j)*vds_max_ime_ext_mask + k], k);
				}
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOPROC has no ext mask configured\n");
	}
	else
		vds_seq_printf(m, "videoprocess ext mask is not enabled\n");

	vds_api_unlock();

	return 0;
}

static int dump_ve_show(void *m, void *v) {

	int  i, j, empty;
	char msg[128];

	vds_api_lock();

	if(coe_stamp_cnt && coe_stamps){
		empty = 1;
		for(i = 0 ; i < vds_max_coe_vid ; ++i){
			if(!ve_stamp_empty(i)){
				empty = 0;
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOENC %d BUFFER ------------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_buf_info_header());
				for(j = 0 ; j < vds_max_coe_stamp ; ++j)
					dump_buf(m, &coe_stamps[i*vds_max_coe_stamp + j], j);
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOENC %d STAMP -------------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_stamp_info_header());
				for(j = 0 ; j < vds_max_coe_stamp ; ++j)
					dump_stamp(m, &coe_stamps[i*vds_max_coe_stamp + j], j);
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOENC has no stamp configured\n");
	}
	else
		vds_seq_printf(m, "videoenc stamp is not enabled\n");

	if(coe_ext_stamp_cnt && coe_ext_stamps){
		empty = 1;
		for(i = 0 ; i < vds_max_coe_ext_vid ; ++i){
			if(!ve_ext_stamp_empty(i)){
				empty = 0;
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOENC %d EXT BUFFER --------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_buf_info_header());
				for(j = 0 ; j < vds_max_coe_ext_stamp ; ++j)
					dump_buf(m, &coe_ext_stamps[i*vds_max_coe_ext_stamp + j], j);
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOENC %d EXT STAMP ---------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_stamp_info_header());
				for(j = 0 ; j < vds_max_coe_ext_stamp ; ++j)
					dump_stamp(m, &coe_ext_stamps[i*vds_max_coe_ext_stamp + j], j);
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOENC has no ext stamp configured\n");
	}
	else
		vds_seq_printf(m, "videoenc ext stamp is not enabled\n");

	if(coe_ext_mask_cnt && coe_ext_masks){
		empty = 1;
		for(i = 0 ; i < vds_max_coe_ext_vid ; ++i){
			if(!ve_ext_mask_empty(i)){
				empty = 0;
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOENC %d EXT MASK ----------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_mask_info_header());
				for(j = 0 ; j < vds_max_coe_ext_mask ; ++j)
					dump_mask(m, &coe_ext_masks[i*vds_max_coe_ext_mask + j], j);
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOENC has no ext mask configured\n");
	}
	else
		vds_seq_printf(m, "videoenc ext mask is not enabled\n");

	vds_api_unlock();

	return 0;
}

static int dump_vo_show(void *m, void *v) {

	int  i, j, empty;
	char msg[128];

	vds_api_lock();

	if(vo_stamp_cnt && vo_stamps){
		empty = 1;
		for(i = 0 ; i < vds_max_vo_vid ; ++i){
			if(!vo_stamp_empty(i)){
				empty = 0;
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOOUT %d BUFFER ------------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_buf_info_header());
				for(j = 0 ; j < vds_max_vo_stamp ; ++j)
					dump_buf(m, &vo_stamps[i*vds_max_vo_stamp + j], j);
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOOUT %d STAMP -------------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_stamp_info_header());
				for(j = 0 ; j < vds_max_vo_stamp ; ++j)
					dump_stamp(m, &vo_stamps[i*vds_max_vo_stamp + j], j);
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOOUT has no stamp configured\n");
	}
	else
		vds_seq_printf(m, "videoout stamp is not enabled\n");

	if(vo_mask_cnt && vo_masks){
		empty = 1;
		for(i = 0 ; i < vds_max_vo_vid ; ++i){
			if(!vo_mask_empty(i)){
				empty = 0;
				vds_snprintf(msg, sizeof(msg), "-------------------------VIDEOOUT %d MASK --------------------------------------\n", i);
				vds_seq_printf(m, msg);
				vds_seq_printf(m, get_mask_info_header());
				for(j = 0 ; j < vds_max_vo_mask ; ++j)
					dump_mask(m, &vo_masks[i*vds_max_vo_mask + j], j);
			}
		}
		if(empty)
			vds_seq_printf(m, "VIDEOOUT has no mask configured\n");
	}
	else
		vds_seq_printf(m, "videoout mask is not enabled\n");

	vds_api_unlock();

	return 0;
}

int vds_info_show(void *m, void *v) {

	dump_vp_show(m, v);
	dump_ve_show(m, v);
	dump_vo_show(m, v);

	return 0;
}

void vds_cmd_write(char *cmd_line, int size)
{
	int len = size, pid = -1, io = -1;
	char target[16], osg[16];
	VDS_RGN *rgn;
	VDS_PHASE phase;
	UINT32 lock, ime_flag = 0;
	int start, x, y, w, h, alpha, cken, ckval, layer, region, solid, thick, color, blkw, blkh;

	if (len == 0) {
		DBG_ERR("Command length is 0!\n");
		return ;
	}

	cmd_line[len - 1] = '\0';
	vds_sscanf(cmd_line, "%s %s %d %d ", target, osg, &pid, &io);
	target[sizeof(target) - 1] = '\0';
	osg[sizeof(osg) - 1] = '\0';

	if(!vds_strcmp(target, "videoprocess")){
		if(!vds_strcmp(osg, "stamp"))
			phase = VDS_PHASE_IME_STAMP;
		else if(!vds_strcmp(osg, "mask"))
			phase = VDS_PHASE_IME_MASK;
		else if(!vds_strcmp(osg, "mosaic"))
			phase = VDS_PHASE_IME_MOSAIC;
		else if(!vds_strcmp(osg, "stamp_ex"))
			phase = VDS_PHASE_IME_EXT_STAMP;
		else if(!vds_strcmp(osg, "mask_ex"))
			phase = VDS_PHASE_IME_EXT_MASK;
		else{
			DBG_ERR("videoprocess doesn't support %s\n", osg);
			goto out;
		}
	} else if(!vds_strcmp(target, "videoenc")){
		if(!vds_strcmp(osg, "stamp"))
			phase = VDS_PHASE_COE_STAMP;
		else if(!vds_strcmp(osg, "stamp_ex"))
			phase = VDS_PHASE_COE_EXT_STAMP;
		else if(!vds_strcmp(osg, "mask_ex"))
			phase = VDS_PHASE_COE_EXT_MASK;
		else{
			DBG_ERR("videoenc doesn't support %s\n", osg);
			goto out;
		}
	} else if(!vds_strcmp(target, "videoout")){
		if(!vds_strcmp(osg, "stamp"))
			phase = VDS_PHASE_VO_STAMP;
		else if(!vds_strcmp(osg, "mask"))
			phase = VDS_PHASE_VO_MASK;
		else{
			DBG_ERR("videoout doesn't support %s\n", osg);
			goto out;
		}
	} else{
		DBG_ERR("unknown phase(%s)\n", target);
		goto out;
	}

	if(pid < 0 || io < 0){
		DBG_ERR("pid(%d) io(%d) is invalid\n", pid, io);
		goto out;
	}

	rgn = find_region(phase, pid, io, &lock);
	if(!rgn){
		DBG_ERR("%s %s %d %d is invalid\n", target, osg, pid, io);
		goto out;
	}

	if(!vds_strcmp(osg, "stamp") || !vds_strcmp(osg, "stamp_ex")){
		vds_sscanf(cmd_line, "%s %s %d %d %d %d %d %d %d %d %d %d",
			target, osg, &pid, &io, &start, &x, &y, &alpha, &cken, &ckval, &layer, &region);
		DBG_IND("set %s %s %d %d to start(%d) x(%d) y(%d) alpha(%d) cken(%d) ckval(%d) layer(%d) region(%d)\n",
			target, osg, pid, io, start, x, y, alpha, cken, ckval, layer, region);
	} else if(!vds_strcmp(osg, "mask") || !vds_strcmp(osg, "mask_ex")){
		vds_sscanf(cmd_line, "%s %s %d %d %d %d %d %d %d %d %d %x %d",
			target, osg, &pid, &io, &start, &x, &y, &w, &h, &solid, &thick, &color, &alpha);
		DBG_IND("set %s %s %d %d to start(%d) x(%d) y(%d) w(%d) h(%d) solid(%d) thickness(%d), color(%x) alpha(%d)\n",
			target, osg, pid, io, start, x, y, w, h, solid, thick, color, alpha);
	} else if(!vds_strcmp(osg, "mosaic")){
		vds_sscanf(cmd_line, "%s %s %d %d %d %d %d %d %d %d %d",
			target, osg, &pid, &io, &start, &x, &y, &w, &h, &blkw, &blkh);
		DBG_IND("set %s %s %d %d to start(%d) x(%d) y(%d) w(%d) h(%d) mosaic_blk_w(%d) mosaic_blk_h(%d)\n",
			target, osg, pid, io, start, x, y, w, h, blkw, blkh);
	}

	vds_res_lock(lock, &ime_flag);

	if(!vds_strcmp(osg, "stamp") || !vds_strcmp(osg, "stamp_ex")){
		rgn->en                       = start;
		rgn->attr.stamp.x             = x;
		rgn->attr.stamp.y             = y;
		rgn->attr.stamp.fg_alpha      = alpha;
		rgn->attr.stamp.colorkey_en   = cken;
		rgn->attr.stamp.colorkey_val  = ckval;
		rgn->attr.stamp.layer         = layer;
		rgn->attr.stamp.region        = region;
	} else if(!vds_strcmp(osg, "mask") || !vds_strcmp(osg, "mask_ex")){
		rgn->en                          = start;
		rgn->attr.mask.type              = solid ? VDS_MASK_TYPE_SOLID : VDS_MASK_TYPE_HOLLOW;
		rgn->attr.mask.color             = color;
		rgn->attr.mask.data.normal.alpha = alpha;
		rgn->attr.mask.data.normal.x[0]  = x;
		rgn->attr.mask.data.normal.y[0]  = y;
		rgn->attr.mask.data.normal.x[1]  = x + w;
		rgn->attr.mask.data.normal.y[1]  = y;
		rgn->attr.mask.data.normal.x[2]  = x + w;
		rgn->attr.mask.data.normal.y[2]  = y + h;
		rgn->attr.mask.data.normal.x[3]  = x;
		rgn->attr.mask.data.normal.y[3]  = y + h;
		rgn->attr.mask.thickness = thick;
	} else if(!vds_strcmp(osg, "mosaic")){
		rgn->en                       = start;
		rgn->attr.mosaic.x[0]         = x;
		rgn->attr.mosaic.y[0]         = y;
		rgn->attr.mosaic.x[1]         = x + w;
		rgn->attr.mosaic.y[1]         = y;
		rgn->attr.mosaic.x[2]         = x + w;
		rgn->attr.mosaic.y[2]         = y + h;
		rgn->attr.mosaic.x[3]         = x;
		rgn->attr.mosaic.y[3]         = y + h;
		rgn->attr.mosaic.mosaic_blk_w = blkw;
		rgn->attr.mosaic.mosaic_blk_h = blkh;
	}

	vds_res_unlock(lock, ime_flag);

out:

	return ;
}

void vds_save_write(int phase, int pid, int vid, char *directory)
{
	char filename[256];
	VDS_RGN *rgn;
	VDS_BUF *buf;
	int i, w, h;
	void *p_addr = NULL;

	DBG_ERR("saving images of phase(%d) pid(%d) vid(%d)\n", phase, pid, vid);

	vds_api_lock();

	rgn = find_region(phase, pid, vid, NULL);
	if(!rgn){
		DBG_ERR("phase(%d) pid(%d) vid(%d) is invalid\n", phase, pid, vid);
		goto out;
	}

	buf = rgn->buf;
	if(!buf){
		DBG_ERR("NULL VDS_BUF for phase(%d) pid(%d) vid(%d)\r\n", phase, pid, vid);
		goto out;
	}

	for(i = 0 ; i < 2 ; ++i){
		w = buf->img_size[i].w;
		h = buf->img_size[i].h;
		p_addr = (void*)buf->p_addr[i];
		if(p_addr == NULL){
			DBG_ERR("buf->p_addr[%d] is NULL\n", i);
			goto out;
		}
		if(buf->size[i] > 4096*4096*2 || buf->size[i] == 0){
			DBG_ERR("invalid image size(%d)\n", buf->size[i]);
			goto out;
		}
		if(i != (int)buf->swap)
			vds_sprintf(filename, "%s/%d_%d_%d_%dx%d_%d", directory, phase, pid, vid, w, h, 1);
		else
			vds_sprintf(filename, "%s/%d_%d_%d_%dx%d_%d", directory, phase, pid, vid, w, h, 0);
			
		if(vds_save_image(filename, p_addr, buf->size[i])){
			DBG_ERR("fail to save %dth buffer\n", i);
			goto out;
		}
	}

	DBG_ERR("images of phase(%d) pid(%d) vid(%d) are saved\n", phase, pid, vid);

out:

	vds_api_unlock();

	return ;
}

int nvt_vds_init(void)
{
	SEM_CREATE(VDS_API_SEM_ID, 1);
	//SEM_CREATE(VDS_IME_STAMP_SEM_ID, 1);
	SEM_CREATE(VDS_IME_EXT_STAMP_SEM_ID, 1);
	SEM_CREATE(VDS_IME_EXT_MASK_SEM_ID, 1);
	SEM_CREATE(VDS_ENC_EXT_STAMP_SEM_ID, 1);
	SEM_CREATE(VDS_ENC_EXT_MASK_SEM_ID, 1);
	SEM_CREATE(VDS_ENC_COE_STAMP_SEM_ID, 1);
	SEM_CREATE(VDS_ENC_JPG_STAMP_SEM_ID, 1);
	SEM_CREATE(VDS_VO_STAMP_SEM_ID, 1);
	SEM_CREATE(VDS_VO_MASK_SEM_ID, 1);
	
	vds_memset(min_proc_stamp_latency, 0, sizeof(min_proc_stamp_latency));
	vds_memset(max_proc_stamp_latency, 0, sizeof(max_proc_stamp_latency));
	vds_memset(min_proc_mask_latency, 0, sizeof(min_proc_mask_latency));
	vds_memset(max_proc_mask_latency, 0, sizeof(max_proc_mask_latency));
	vds_memset(min_proc_ext_stamp_latency, 0, sizeof(min_proc_ext_stamp_latency));
	vds_memset(max_proc_ext_stamp_latency, 0, sizeof(max_proc_ext_stamp_latency));
	vds_memset(min_proc_ext_mask_latency, 0, sizeof(min_proc_ext_mask_latency));
	vds_memset(max_proc_ext_mask_latency, 0, sizeof(max_proc_ext_mask_latency));
	vds_memset(min_enc_stamp_latency, 0, sizeof(min_enc_stamp_latency));
	vds_memset(max_enc_stamp_latency, 0, sizeof(max_enc_stamp_latency));
	vds_memset(min_jpg_stamp_latency, 0, sizeof(min_jpg_stamp_latency));
	vds_memset(max_jpg_stamp_latency, 0, sizeof(max_jpg_stamp_latency));
	vds_memset(min_enc_ext_stamp_latency, 0, sizeof(min_enc_ext_stamp_latency));
	vds_memset(max_enc_ext_stamp_latency, 0, sizeof(max_enc_ext_stamp_latency));
	vds_memset(min_enc_ext_mask_latency, 0, sizeof(min_enc_ext_mask_latency));
	vds_memset(max_enc_ext_mask_latency, 0, sizeof(max_enc_ext_mask_latency));
	vds_memset(min_out_stamp_latency, 0, sizeof(min_out_stamp_latency));
	vds_memset(max_out_stamp_latency, 0, sizeof(max_out_stamp_latency));
	vds_memset(min_out_mask_latency, 0, sizeof(min_out_mask_latency));
	vds_memset(max_out_mask_latency, 0, sizeof(max_out_mask_latency));
	
	avg_proc_stamp_latency = 0;
	avg_proc_mask_latency = 0;
	avg_proc_ext_stamp_latency = 0;
	avg_proc_ext_mask_latency = 0;
	avg_enc_stamp_latency = 0;
	avg_jpg_stamp_latency = 0;
	avg_enc_ext_stamp_latency = 0;
	avg_enc_ext_mask_latency = 0;
	avg_out_stamp_latency = 0;
	avg_out_mask_latency = 0;

	return 0;
}

void nvt_vds_exit(void)
{
	SEM_DESTROY(VDS_API_SEM_ID);
	//SEM_DESTROY(VDS_IME_STAMP_SEM_ID);
	SEM_DESTROY(VDS_IME_EXT_STAMP_SEM_ID);
	SEM_DESTROY(VDS_IME_EXT_MASK_SEM_ID);
	SEM_DESTROY(VDS_ENC_EXT_STAMP_SEM_ID);
	SEM_DESTROY(VDS_ENC_EXT_MASK_SEM_ID);
	SEM_DESTROY(VDS_ENC_COE_STAMP_SEM_ID);
	SEM_DESTROY(VDS_ENC_JPG_STAMP_SEM_ID);
	SEM_DESTROY(VDS_VO_STAMP_SEM_ID);
	SEM_DESTROY(VDS_VO_MASK_SEM_ID);
	
	_vds_reset();
}

#if defined(__LINUX)
#else

void vds_memset(void *buf, unsigned char val, int len)
{
	memset(buf, val, len);
}

void vds_memcpy(void *buf, void *src, int len)
{
	memcpy(buf, src, len);
}

void* vds_alloc(int size)
{
	return malloc(size);
}

void vds_free(void *buf)
{
	free(buf);
}

int vds_seq_printf(void *m, const char *fmtstr, ...)
{
	char    buf[512];
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);
	if (len > 0)
		DBG_DUMP(buf);
	return 0;
}

int vds_snprintf(char *buf, int size, const char *fmtstr, ...)
{
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, size, fmtstr, marker);
	va_end(marker);

	return len;
}

int vds_sprintf(char *buf, const char *fmtstr, ...)
{
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsprintf(buf, fmtstr, marker);
	va_end(marker);

	return len;
}

int vds_sscanf(char *buf, const char *fmtstr, ...)
{
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsscanf(buf, fmtstr, marker);
	va_end(marker);

	return len;
}

int vds_strcmp(char *s1, char *s2)
{
	return strcmp(s1, s2);
}

static int help_show(void) {

	vds_seq_printf(NULL, "execute \"osg info\" under \">\" to show all the osg configuration\n");

	return 0;
}

MAINFUNC_ENTRY(osg, argc, argv)
{
	if (argc < 1) {
		return -1;
	}
	
	if (strncmp(argv[1], "?", 2) == 0) {
		help_show();
		return 0;
	}
	
	if (strncmp(argv[1], "info", strlen(argv[1])) == 0) {
		dump_vp_show(NULL, NULL);
		dump_ve_show(NULL, NULL);
		dump_vo_show(NULL, NULL);
	}else if (strncmp(argv[1], "cmd", strlen(argv[1])) == 0) {
		vds_cmd_show(NULL, NULL);
	}else if (strncmp(argv[1], "help", strlen(argv[1])) == 0) {
		help_show();
	}else{
		DBG_ERR("Invalid CMD !!\r\n");
		help_show();
		return -1;
	}
	
	return 0;
}
#endif
