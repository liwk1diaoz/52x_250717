#ifndef _IPP_DEBUG_INT_H_
#define _IPP_DEBUG_INT_H_
/**
    ipp_debug_int.h


    @file       ipp_debug_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "kwrap/cpu.h"
#include "comm/ddr_arb.h"
#include "kflow_videoprocess/ctl_ipp_util.h"
#include "ctl_ipp_dbg.h"
#include "ctl_ipp_int.h"
#include "ctl_ipp_buf_int.h"
#include "kdrv_videoprocess/kdrv_ipp.h"

#if defined(__LINUX)
#define CTL_IPP_INT_ROOT_PATH "//mnt//sd//"
#define CTL_IPP_INT_PATH_SPLT "//"
#elif defined(__FREERTOS)
#define CTL_IPP_INT_ROOT_PATH "A:\\"
#define CTL_IPP_INT_PATH_SPLT "\\"
#endif

int ctl_ipp_int_printf(const char *fmtstr, ...);

/**
	time stamp log
*/
#define CTL_IPP_DBG_TS_ENABLE (1)
#define CTL_IPP_DBG_TS_LOG_NUM (32)
#define CTL_IPP_DBG_TS_LOG_FASTBOOT_NUM (4)

typedef enum {
	CTL_IPP_DBG_TS_SND = 0,
	CTL_IPP_DBG_TS_RCV,
	CTL_IPP_DBG_TS_KDF_SND,
	CTL_IPP_DBG_TS_KDF_RCV,
	CTL_IPP_DBG_TS_PROCSTART,
	CTL_IPP_DBG_TS_TRIG,
	CTL_IPP_DBG_TS_BP,
	CTL_IPP_DBG_TS_PROCEND,
	CTL_IPP_DBG_TS_MAX
} CTL_IPP_DBG_TS_TYPE;

typedef struct {
	UINT32 handle;
	UINT32 cnt;
	UINT64 ts[CTL_IPP_DBG_TS_MAX];
	INT32 err_msg;
} CTL_IPP_DBG_TS_LOG;

typedef struct {
	CHAR name[CTL_IPP_HANDLE_NAME_MAX];
	CTL_IPP_FLOW_TYPE flow;
	UINT32 ctl_snd_evt_cnt;		/* ctl send event count */
	UINT32 ctl_frm_str_cnt;		/* ctl ife frame start count */
	UINT32 ctl_proc_end_cnt;	/* ctl process end count */
	UINT32 ctl_drop_cnt;		/* ctl drop frame count, include ctl_drop, kdf_drop */
	UINT32 kdf_snd_evt_cnt;		/* kdf send event count */
	UINT32 kdf_frm_str_cnt;		/* kdf ife frame start count */
	UINT32 kdf_proc_end_cnt;	/* kdf process end count */
	UINT32 kdf_drop_cnt;		/* kdf drop frame count, include ctl_drop, kdf_drop */
	UINT32 kdf_reset_cnt;		/* kdf drop frame count, include ctl_drop, kdf_drop */
	UINT32 in_buf_re_cb_cnt;	/* direct mode input buffer release count */
	UINT32 in_buf_re_cnt;		/* direct mode input buffer release count */
	UINT32 in_buf_drop_cnt;		/* direct mode input buffer drop count */
	UINT32 in_pro_skip_cnt;		/* direct mode skip process cfg count */
} CTL_IPP_DBG_BASE_INFO;

void ctl_ipp_dbg_ts_set(CTL_IPP_DBG_TS_LOG ts);
void ctl_ipp_dbg_ts_dump(int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_ts_dump_fastboot(void);
void ctl_ipp_dbg_set_baseinfo(UINT32 hdl, BOOL flag);

/**
	handle debug information
*/
void ctl_ipp_dbg_hdl_head_set(CTL_IPP_LIST_HEAD *hdl_used_head);
void ctl_ipp_dbg_hdl_stripe_set(UINT32 kdf_hdl, KDRV_DCE_STRIPE_INFO *p_strp_info);
void ctl_ipp_dbg_hdl_dump_all(int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_hdl_dump_buf(UINT32 hdl, int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_baseinfo_dump(UINT32 baseinfo, BOOL is_from_pool, int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_savefile(CHAR *f_name, UINT32 addr, UINT32 size);
void ctl_ipp_dbg_saveyuv(UINT32 handle, CTL_IPP_OUT_BUF_INFO *p_buf);
void ctl_ipp_dbg_saveyuv_cfg(CHAR *handle_name, CHAR *filepath, UINT32 count);
void ctl_ipp_dbg_save_vdofrm_image(VDO_FRAME *p_vdofrm, CHAR *f_name);
void ctl_ipp_dbg_dump_dtsi2file(UINT32 kdf_hdl);
void ctl_ipp_dbg_dump_dtsi(CHAR *f_name);
void ctl_ipp_dbg_dtsi_init(void);

/**
	debug common buffer infor
*/
#define CTL_IPP_DBG_BUF_LOG_ENABLE (1)
#define CTL_IPP_DBG_BUF_LOG_NUM (16)

typedef struct {
	UINT32 handle;
	UINT32 pid;
	UINT32 type;
	UINT32 ts_start;
	UINT32 ts_end;
	UINT32 buf_addr;
} CTL_IPP_DBG_BUF_CBTIME_LOG;

void ctl_ipp_dbg_outbuf_cbtime_dump(int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_outbuf_log_set(UINT32 hdl, CTL_IPP_BUF_IO_CFG io_type, CTL_IPP_OUT_BUF_INFO *p_buf, UINT64 ts_start, UINT64 ts_end);
void ctl_ipp_dbg_inbuf_log_set(UINT32 hdl, UINT32 buf_addr, CTL_IPP_CBEVT_IN_BUF_MSG type, UINT64 ts_start, UINT64 ts_end);
void ctl_ipp_dbg_inbuf_cbtime_dump(int (*dump)(const char *fmt, ...));


/**
	debug context buffer log
*/
#define CTL_IPP_DBG_CTX_BUF_NAME_MAX (16)

typedef enum {
	CTL_IPP_DBG_CTX_BUF_QUERY = 0,
	CTL_IPP_DBG_CTX_BUF_ALLOC,
	CTL_IPP_DBG_CTX_BUF_FREE,
	CTL_IPP_DBG_CTX_BUF_OP_MAX
} CTL_IPP_DBG_CTX_BUF_OP;

typedef struct {
	CHAR name[CTL_IPP_DBG_CTX_BUF_NAME_MAX];
	CTL_IPP_DBG_CTX_BUF_OP op;
	UINT32 size;
	UINT32 addr;
	UINT32 cfg_num;
} CTL_IPP_DBG_CTX_BUF_LOG;

void ctl_ipp_dbg_ctxbuf_log_set(CHAR *name, CTL_IPP_DBG_CTX_BUF_OP op, UINT32 size, UINT32 addr, UINT32 n);
void ctl_ipp_dbg_ctxbuf_log_dump(int (*dump)(const char *fmt, ...));


/**
	debug isr sequence in direct mode
*/
#define CTL_IPP_DBG_DIRECT_ISR_ENABLE (0)
#define CTL_IPP_DBG_DIRECT_ISR_NUM 32
#define CTL_IPP_DBG_DIR_ISR_NAME_MAX	(16)

typedef struct {
	CHAR name[CTL_IPP_DBG_DIR_ISR_NAME_MAX];
	UINT32 cnt;
	UINT32 addr;
} CTL_IPP_DBG_DIR_ISR_SEQ;


void ctl_ipp_dbg_dump_direct_isr_sequence(UINT32 hdl);
void ctl_ipp_dbg_set_direct_isr_sequence(UINT32 hdl, CHAR* name, UINT32 addr);

/**
	debug isp mode
*/
void ctl_ipp_dbg_set_isp_mode(UINT32 mode);
UINT32 ctl_ipp_dbg_get_isp_mode(void);

/**
	debug privcay mask
*/
#define CTL_IPP_DBG_PRIMASK_ENABLE (1)
#define CTL_IPP_DBG_PRIMASK_COLOR_POOL (8)

typedef struct {
	UINT8 enable;
	UINT8 color[CTL_IPP_DBG_PRIMASK_COLOR_POOL][3];
	USIZE msk_size;
} CTL_IPP_DBG_PRIMASK_CTL;

typedef enum {
	CTL_IPP_DBG_PRIMASK_CFG_SIZE = 2,	/* data_type: USIZE */
	CTL_IPP_DBG_PRIMASK_CFG_MAX,
} CTL_IPP_DBG_PRIMASK_CFG;

INT32 ctl_ipp_dbg_primask_cb(UINT32 msg, void *in, void *out);
void ctl_ipp_dbg_primask_en(UINT32 en);
void ctl_ipp_dbg_primask_cfg(UINT32 cfg, void *data);


#define CTL_IPP_DBG_DMA_WP_BUF_DISABLE 				0
#define CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 				((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 0)
#define CTL_IPP_DBG_DMA_WP_BUF_IFE_IN1 				((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 1)
#define CTL_IPP_DBG_DMA_WP_BUF_DCE_IN 				((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 2)
#define CTL_IPP_DBG_DMA_WP_BUF_IPE_IN 				((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 3)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_IN 				((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 4)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_REF_IN			((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 5)

#define CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBIN_WDR 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 6)
#define CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBIN_DEFOG 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 7)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_SUBIN_LCA 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 8)

#define CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT 				((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 9)
#define CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT 				((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 10)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT 			((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 11)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_P2_OUT 			((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 12)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_P3_OUT 			((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 13)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_P4_OUT 			((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 14)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_P5_OUT 			((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 15)

#define CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBOUT_WDR 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 16)
#define CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_DEFOG 	((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 17)
#define CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_VA 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 18)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_LCA 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 19)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MV 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 20)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 21)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS_ROI 	((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 22)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_PM 		((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 23)
#define CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_3DNR_STA 	((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 24)

#define CTL_IPP_DBG_DMA_WP_BUF_ALL					(((CTL_IPP_DBG_DMA_WP_BUF_TYPE)1 << 25) - 1)
typedef UINT64 CTL_IPP_DBG_DMA_WP_BUF_TYPE;	// bit field define by CTL_IPP_DBG_DMA_WP_BUF_XXX (see above)

typedef enum {
	CTL_IPP_DBG_DMA_WP_LEVEL_UNWRITE = DMA_WPLEL_UNWRITE,
	CTL_IPP_DBG_DMA_WP_LEVEL_DETECT = DMA_WPLEL_DETECT,
	CTL_IPP_DBG_DMA_WP_LEVEL_UNREAD = DMA_RPLEL_UNREAD,
	CTL_IPP_DBG_DMA_WP_LEVEL_UNRW = DMA_RWPLEL_UNRW,
	CTL_IPP_DBG_DMA_WP_LEVEL_AUTO = 0xFF, // set protect level by different channel
} CTL_IPP_DBG_DMA_WP_LEVEL;

typedef struct {
	BOOL one_buf_mode_en; // one buffer mode share yuv buffer with codec
	CTL_IPP_FLOW_TYPE flow; // LCA and 3DNR subin/subout in direct mode reference same buffer
} CTL_IPP_DBG_DMA_WP_BUF_EXT;

ER ctl_ipp_dbg_dma_wp_set_enable(BOOL en, CTL_IPP_DBG_DMA_WP_BUF_TYPE type);
CTL_IPP_DBG_DMA_WP_BUF_TYPE ctl_ipp_dbg_dma_wp_get_enable(void);
ER ctl_ipp_dbg_dma_wp_set_prot_level(CTL_IPP_DBG_DMA_WP_LEVEL level);
ER ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_TYPE type, CTL_IPP_BUF_ADDR addr, UINT32 size, CTL_IPP_DBG_DMA_WP_BUF_EXT *ext_data);
ER ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_TYPE type, CTL_IPP_BUF_ADDR addr);
ER ctl_ipp_dbg_dma_wp_proc_start(CTL_IPP_INFO_LIST_ITEM *ctrl_info);
ER ctl_ipp_dbg_dma_wp_proc_end(CTL_IPP_INFO_LIST_ITEM *ctrl_info, BOOL update_ref);
ER ctl_ipp_dbg_dma_wp_proc_end_ref(CTL_IPP_INFO_LIST_ITEM *last_ctrl_info);
ER ctl_ipp_dbg_dma_wp_proc_end_push(CTL_IPP_INFO_LIST_ITEM *ctrl_info, BOOL update_ref);
ER ctl_ipp_dbg_dma_wp_task_start(CTRL_IPP_BUF_ITEM item, CTL_IPP_BUF_ADDR addr, UINT32 size);
ER ctl_ipp_dbg_dma_wp_task_end(CTRL_IPP_BUF_ITEM item, CTL_IPP_BUF_ADDR addr);
ER ctl_ipp_dbg_dma_wp_drop(CTL_IPP_INFO_LIST_ITEM *ctrl_info);
ER ctl_ipp_dbg_dma_wp_clear(void);
void ctl_ipp_dbg_dma_wp_dump(int (*dump)(const char *fmt, ...));

#endif //_IPP_DEBUG_INT_H_
