/**
    IPL Ctrl Layer

    @file       ctl_ipp_int.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_IPP_INT_H
#define _CTL_IPP_INT_H

#include "ctl_ipp_util_int.h"
#include "kflow_videoprocess/ctl_ipp.h"

#define CTL_IPP_BUF_LCA_NUM 2
#define CTL_IPP_BUF_3DNR_NUM 2
#define CTL_IPP_BUF_3DNR_STA_NUM 1
#define CTL_IPP_BUF_DFG_NUM 2
#define CTL_IPP_BUF_WDR_NUM 2
#define CTL_IPP_BUF_PM_NUM 3
#define CTL_IPP_BUF_YOUT_NUM 4
#define CTL_IPP_BUF_VA_NUM 2
#define CTL_IPP_BUF_DUMMY_UV_NUM 1

#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
#define CTL_IPP_HANDLE_LOCK_TIMEOUT_MS	(5000)
#else
#define CTL_IPP_HANDLE_LOCK_TIMEOUT_MS	(500)
#endif

/**********************************/
/*  ctl_ipl message queue         */
/**********************************/
#define CTL_IPP_MSG_STS_FREE    0x00000000
#define CTL_IPP_MSG_STS_LOCK    0x00000001

#define CTL_IPP_MSG_IGNORE					0x00000000
#define CTL_IPP_MSG_PROCESS					0x00000001  //[0]: handle, [1]: header address, [2]: buf_id(for unit release buffer)
#define CTL_IPP_MSG_DROP					0x00000002  //[0]: handle, [1]: header address, [2]: buf_id(for unit release buffer), this cmd is for erase queue used
#define CTL_IPP_MSG_PROCESS_PATTERN_PASTE	0x00000003  //[0]: handle, [1]: header address, [2]: buf_id(for unit release buffer)
#define CTL_IPP_MSG_PROCESS_DIRECT			0x00000004  //[0]: handle, [1]: header address, [2]: direct event

typedef struct {
	UINT32 cmd;
	UINT32 param[5];
	UINT32 rev[2];
	IPP_EVENT_FP drop_fp;
	CTL_IPP_LIST_HEAD list;
	UINT64 snd_time;
} CTL_IPP_MSG_EVENT;

ER ctl_ipp_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, IPP_EVENT_FP drop_fp);
ER ctl_ipp_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, IPP_EVENT_FP *drop_fp, UINT64 *snd_t);
ER ctl_ipp_msg_flush(void);
ER ctl_ipp_erase_queue(UINT32 handle);
ER ctl_ipp_msg_reset_queue(void);
UINT32 ctl_ipp_get_free_queue_num(void);


/**********************************/
/*  ctl_ipl handle, process       */
/**********************************/
#define CTL_IPP_HANDLE_NAME_MAX             (16)

#define CTL_IPP_HANDLE_STS_MASK             (0x55AA0000)
#define CTL_IPP_HANDLE_STS_FREE             (CTL_IPP_HANDLE_STS_MASK | 0x01)
#define CTL_IPP_HANDLE_STS_READY            (CTL_IPP_HANDLE_STS_MASK | 0x02)
#define CTL_IPP_HANDLE_STS_DIR_START        (CTL_IPP_HANDLE_STS_MASK | 0x11)
#define CTL_IPP_HANDLE_STS_DIR_READY        (CTL_IPP_HANDLE_STS_MASK | 0x12)
#define CTL_IPP_HANDLE_STS_DIR_STOP_S1      (CTL_IPP_HANDLE_STS_MASK | 0x13)
#define CTL_IPP_HANDLE_STS_DIR_STOP_S2      (CTL_IPP_HANDLE_STS_MASK | 0x14)
#define CTL_IPP_HANDLE_STS_DIR_SLEEP      	(CTL_IPP_HANDLE_STS_MASK | 0x15)
#define CTL_IPP_HANDLE_STS_DIR_STOP_S3      (CTL_IPP_HANDLE_STS_MASK | 0x16)
#define CTL_IPP_HANDLE_STS_DIR_TSK_IDLE     (CTL_IPP_HANDLE_STS_MASK | 0x17)
#define CTL_IPP_HANDLE_STS_DIR_TSK_PROC     (CTL_IPP_HANDLE_STS_MASK | 0x18)

#define CTL_IPP_EVT_TAG_NONE					(0x00000000)
#define CTL_IPP_EVT_TAG_SUBOUT_FOR_CAP			(0x00000001)
#define CTL_IPP_EVT_TAG_MAINFRM_FOR_CAP			(0x00000002)
#define CTL_IPP_EVT_TAG_SKIP_ISP_FOR_FASTBOOT	(0x00000003)

#define CTL_IPP_TPLNR_REF_BUF_WAIT_PUSH		(0x1)
#define CTL_IPP_TPLNR_REF_BUF_WAIT_RELEASE	(0x2)

typedef struct {
	UINT32 flag;
	UINT32 ptn;
} CTL_IPP_HANDLE_LOCKINFO;

typedef struct {
	CTL_IPP_BASEINFO info;
	CTL_IPP_LIST_HEAD list;
	UINT32 owner;
	UINT32 lock_cnt;
	UINT32 evt_tag;

	UINT32 input_header_addr;
	UINT32 input_buf_id;
	UINT32 tplnr_ref_buf_sts;
	CTL_IPP_OUT_BUF_INFO buf_info[CTL_IPP_OUT_PATH_ID_MAX];

	UINT64 ctl_snd_evt_time;
	UINT64 ctl_rev_evt_time;
	UINT32 kdf_evt_count;

	UINT32 reserved;
} CTL_IPP_INFO_LIST_ITEM;

typedef struct {
	CTL_IPP_LIST_HEAD free_head;
	CTL_IPP_LIST_HEAD used_head;
	UINT32 count;
	CTL_IPP_INFO_LIST_ITEM *info;
} CTL_IPP_INFO_POOL;

typedef struct {
	UINT32 lofs;
	UINT32 size;
	CHAR *name;
} CTL_IPP_BUF_PRI_LOF_INFO;

typedef struct {
	CTL_IPP_BUF_PRI_LOF_INFO lca[CTL_IPP_BUF_LCA_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO mo[CTL_IPP_BUF_3DNR_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO mv[CTL_IPP_BUF_3DNR_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO defog_subout[CTL_IPP_BUF_DFG_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO pm[CTL_IPP_BUF_PM_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO ms[CTL_IPP_BUF_3DNR_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO ms_roi[CTL_IPP_BUF_3DNR_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO _3dnr_sta[CTL_IPP_BUF_3DNR_STA_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO wdr_subout[CTL_IPP_BUF_WDR_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO va[CTL_IPP_BUF_VA_NUM];
	CTL_IPP_BUF_PRI_LOF_INFO dummy_uv[CTL_IPP_BUF_DUMMY_UV_NUM];

	/* this is used to keep sie yout for isp flow */
	CTL_IPP_BUF_PRI_LOF_INFO yout[CTL_IPP_BUF_YOUT_NUM];
} CTL_IPP_BUF_PRI_ITEM_INFO;

typedef struct {
	CTL_IPP_PRIVATE_BUF buf_info;
	CTL_IPP_BUF_PRI_ITEM_INFO buf_item;
} CTL_IPP_BUF_PRI_INFO;

typedef struct {
	CHAR name[CTL_IPP_HANDLE_NAME_MAX];
	UINT32 sts;
	UINT32 dir_tsk_sts;				/* used for checking ctl_ipp_process is done. see CTL_IPP_HANDLE_STS_DIR_TSK_IDLE/CTL_IPP_HANDLE_STS_DIR_TSK_PROC. direct task mode only */
	UINT32 lock;
	UINT32 pro_end_lock;
	vk_spinlock_t spinlock;			/* spinlock for handle, current use to protect info between set_apply and direct_process */
	vk_spinlock_t dir_tsk_lock;		/* spinlock for direct mode cb and process, to protect dir_tsk_sts. separate from hdl->spinlock for efficiency. direct task mode only */
	CTL_IPP_LIST_HEAD list;

	CTL_IPP_FLOW_TYPE flow;
	UINT32 kdf_hdl;
	UINT32 ipp_evt_hdl;
	IPP_EVENT_FP evt_outbuf_fp;
	IPP_EVENT_FP evt_inbuf_fp;
	CTL_IPP_FUNC func_en;			/* keep func_en for get_cmd */
	CTL_IPP_BASEINFO ctrl_info;     /* info that is currently used */
	CTL_IPP_BASEINFO rtc_info;      /* info that keep settings from api, wait for set_apply */
	CTL_IPP_INFO_LIST_ITEM *p_last_rdy_ctrl_info;	/* last ready ctrl info, update when proc_en */
	CTL_IPP_INFO_LIST_ITEM *p_last_cfg_ctrl_info;	/* last cfg for direct mode, update when sie bp*/
	CTL_IPP_BUFCFG bufcfg;
	CTL_IPP_BUF_PRI_INFO private_buf;
	UINT8 buf_push_order[CTL_IPP_IME_PATH_ID_MAX];			/* push order of each path */
	UINT8 buf_push_pid_sequence[CTL_IPP_IME_PATH_ID_MAX];	/* push pid sequence sorted base on order */
	UINT32 buf_io_started[CTL_IPP_IME_PATH_ID_MAX];	/* 0 --> buf_io_start is not yet called, 1 --> buf_io_start already called */
	UINT32 is_first_handle;

	/* ideal case
		ctl_snd_evt_cnt = proc_end_cnt + drop_cnt
	*/
	UINT32 ctl_frm_str_cnt;		/* ife frame start count */
	UINT32 ctl_snd_evt_cnt;		/* ctl send event count */
	UINT32 proc_end_cnt;		/* process end count */
	UINT32 drop_cnt;			/* drop frame count, include ctl_drop, kdf_drop */
	UINT32 kdf_snd_evt_cnt;		/* kdf send event count, for predict apply frame */
	UINT32 buf_push_evt_cnt;	/* push event in buf task queue, for hdal to drop input */
	UINT32 buf_new_fail_cnt[CTL_IPP_OUT_PATH_ID_MAX];	/* new buffer failed count for each output path */
	UINT32 buf_tag_fail_cnt[CTL_IPP_OUT_PATH_ID_MAX];   /* check output buffer tag fail count */
	UINT32 in_buf_re_cb_cnt;	/* direct mode input buffer release count */
	UINT32 in_buf_re_cnt;		/* direct mode input buffer release count */
	UINT32 in_buf_drop_cnt;		/* direct mode input buffer drop count */
	UINT32 in_pro_skip_cnt;		/* direct mode skip process cfg count */

	UINT64 snd_evt_time;
	UINT64 rev_evt_time;

	UINT32 isp_id[CTL_IPP_ALGID_MAX];			/* isp id(iq, wdr_lib...) */

	/* Direct process bit*/
	UINT32 dir_proc_bit;

	/* debug info */
	UINT32 last_3dnr_disable_frm;	/* latest frame number that disable 3dnr */
	UINT32 last_3dnr_ref_shift_frm;	/* latest frame number that 3dnr reference is shifted(use previous reference output) */
	UINT32 _3dnr_ref_shift_cnt;		/* count how many times that previous reference output is keep for current frame */
	UINT32 max_3dnr_ref_shift_diff;	/* max frame difference of reference and current frame */
	UINT32 last_pattern_paste_frm;	/* latest frame number that enable pattern paste */
} CTL_IPP_HANDLE;

typedef struct {
	CTL_IPP_LIST_HEAD free_head;
	CTL_IPP_LIST_HEAD used_head;
	UINT32 flg_num;
	ID *flg_id;
	UINT32 pro_end_flg_num;
	ID *pro_end_flg_id;
	UINT32 handle_num;
	CTL_IPP_HANDLE *handle;
} CTL_IPP_HANDLE_POOL;

/**
	CTL IPP SIE ID
*/
typedef enum _CTL_IPP_SIE_ID {
	CTL_IPP_SIE_ID_1 = 0,	///< process id 0
	CTL_IPP_SIE_ID_2,	    ///< process id 1
	CTL_IPP_SIE_ID_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_IPP_SIE_ID)
} CTL_IPP_SIE_ID;

#define CTL_IPP_IFE_HDR_REF_SIE1 0x00000001	//reference channel: sie1
#define CTL_IPP_IFE_HDR_REF_SIE2 0x00000002	//reference channel: sie2
#define CTL_IPP_IFE_HDR_REF_SIE3 0x00000004	//reference channel: sie3

/* reserved[4] */
#define CTL_IPP_VDOFRM_RES_MSK_SIEID(reserved)		(((reserved) >> 0) & 0x000000ff)	// bit 0..7
#define CTL_IPP_VDOFRM_RES_MSK_IFECHK(reserved)		(((reserved) >> 8) & 0x00000001)	// bit 8

typedef INT32(*CTL_IPP_IOCTL_FP)(CTL_IPP_HANDLE *, void *);
typedef INT32(*CTL_IPP_SET_FP)(CTL_IPP_HANDLE *, void *);
typedef INT32(*CTL_IPP_GET_FP)(CTL_IPP_HANDLE *, void *);

void ctl_ipp_handle_reset(CTL_IPP_HANDLE *p_hdl);
void ctl_ipp_handle_lock(UINT32 lock);
void ctl_ipp_handle_unlock(UINT32 lock);
void ctl_ipp_handle_wait_proc_end(UINT32 lock, BOOL bclr_flag, BOOL b_timeout_en);
void ctl_ipp_handle_set_proc_end(UINT32 lock);
UINT32 ctl_ipp_handle_validate(CTL_IPP_HANDLE *p_hdl);
CTL_IPP_HANDLE *ctl_ipp_get_hdl_by_ispid(UINT32 id);
UINT32 ctl_ipp_get_hdl_sts_by_ispid(UINT32 id);
INT32 ctl_ipp_process(UINT32 hdl, UINT32 data_addr, UINT32 buf_id, UINT64 snd_time, UINT32 proc_cmd, UINT32 dir_evt);
INT32 ctl_ipp_drop(UINT32 hdl, UINT32 data_addr, UINT32 buf_id, IPP_EVENT_FP drop_fp, INT32 err_msg);
INT32 ctl_ipp_preset_cb(UINT32 msg, void *in, void *out);
INT32 ctl_ipp_isp_cb(UINT32 msg, void *in, void *out);

UINT32 ctl_ipp_info_pool_init(UINT32 num, UINT32 buf_addr, UINT32 is_query);
void ctl_ipp_info_pool_free(void);
void ctl_ipp_info_release(CTL_IPP_INFO_LIST_ITEM *p_info);
CTL_IPP_INFO_LIST_ITEM *ctl_ipp_info_get_entry_by_info_addr(UINT32 addr);
CTL_IPP_INFO_LIST_ITEM *ctl_ipp_info_alloc(UINT32 owner);
void ctl_ipp_info_lock(CTL_IPP_INFO_LIST_ITEM *p_info);
void ctl_ipp_info_dump(int (*dump)(const char *fmt, ...));

INT32 ctl_ipp_process_raw(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id);
INT32 ctl_ipp_process_ccir(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id);
INT32 ctl_ipp_process_ime_d2d(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id);
INT32 ctl_ipp_process_ipe_d2d(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id);
INT32 ctl_ipp_process_dce_d2d(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id);
INT32 ctl_ipp_process_vr360(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id);
INT32 ctl_ipp_process_direct(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id);
INT32 ctl_ipp_process_flow_pattern_paste(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id);

void ctl_ipp_direct_set_tsk_en(BOOL en);
BOOL ctl_ipp_direct_get_tsk_en(void);

#endif //_CTL_IPP_INT_H
