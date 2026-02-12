/**
    VPE_Ctrl Layer

    @file       ctl_vpe_int.h
    @ingroup    mIVPE_Ctrl
    @note

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _CTL_VPE_INT_H_
#define _CTL_VPE_INT_H_

#include "kwrap/cpu.h"
#include "kwrap/util.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/spinlock.h"
#include "kwrap/type.h"
#include "kwrap/list.h"
#include "kdrv_videoprocess/kdrv_vpe_if.h"
#include "kflow_videoprocess/ctl_vpe.h"
#include "kflow_videoprocess/ctl_vpe_isp.h"
#include "ctl_vpe_dbg.h"

#if defined(CONFIG_NVT_SMALL_HDAL)
#define CTL_VPE_MODULE_ENABLE	DISABLE
#else
#define CTL_VPE_MODULE_ENABLE	ENABLE
#endif

/**
	atomic operations
*/
#if defined(__LINUX)
/* use linux kernel atomic api */
#else
/* use gcc api */
typedef INT32 atomic_t;

#define ATOMIC_INIT(i) {(i)}
#define atomic_inc(x) __sync_add_and_fetch(x, 1)
#define atomic_dec(x) __sync_sub_and_fetch(x, 1)
#define atomic_read(x) __sync_add_and_fetch(x, 0)
#define atomic_set(x, v) __sync_bool_compare_and_swap(x, *x, v)
#endif

#define CTL_VPE_UTIL_OFFSETOF(TYPE, MEMBER)  ((UINT32)&((TYPE *)0)->MEMBER)

#define CTL_VPE_MSG_STS_FREE    0x00000000
#define CTL_VPE_MSG_STS_LOCK    0x00000001

#define CTL_VPE_MSG_IGNORE      0x00000000
/*
	process command param differ in different usage
	[0]: handle, [1]: header address, [2]: buf_id(for unit release buffer)
*/
#define CTL_VPE_MSG_PROCESS     0x00000001
#define CTL_VPE_MSG_DROP        0x00000002  //[0]: handle, [1]: header address, [2]: err, this cmd is for erase queue used

#define CTL_VPE_QUE_FLG_PROC	FLGPTN_BIT(0)

#define CTL_VPE_TASK_TIMEOUT_MS			(2000)
#define CTL_VPE_JOB_TIMEOUT_MS			(1000)

#define CTL_VPE_TASK_PAUSE				FLGPTN_BIT(0)
#define CTL_VPE_TASK_PAUSE_END			FLGPTN_BIT(1)
#define CTL_VPE_TASK_RESUME				FLGPTN_BIT(2)
#define CTL_VPE_TASK_RESUME_END			FLGPTN_BIT(3)
#define CTL_VPE_TASK_IDLE				FLGPTN_BIT(4)
#define CTL_VPE_TASK_RESTART			FLGPTN_BIT(5)
#define CTL_VPE_TASK_FLUSH				FLGPTN_BIT(6)
#define CTL_VPE_TASK_EXIT				FLGPTN_BIT(7)
#define CTL_VPE_TASK_CHK				FLGPTN_BIT(8)
#define CTL_VPE_TASK_TRIG				FLGPTN_BIT(9)
#define CTL_VPE_TASK_TRIG_END			FLGPTN_BIT(10)
#define CTL_VPE_TASK_EXIT_END			FLGPTN_BIT(11)
#define CTL_VPE_TASK_JOBDONE			FLGPTN_BIT(12)
#define CTL_VPE_TASK_LOCK				FLGPTN_BIT(13)
#define CTL_VPE_TASK_OOPS				FLGPTN_BIT(31)
#define CTL_VPE_PROC_TASK_INIT       	(CTL_VPE_TASK_PAUSE_END|CTL_VPE_TASK_TRIG_END|CTL_VPE_TASK_LOCK)
#define CTL_VPE_CB_TASK_INIT       		(CTL_VPE_TASK_PAUSE_END|CTL_VPE_TASK_TRIG_END|CTL_VPE_TASK_LOCK)

#define CTL_VPE_TASK_PRIORITY			(3)

#define CTL_VPE_DBG_TS_NODE_MAX_NUM (16)
#define CTL_VPE_SCALE_RATIO_MAX 	(1599)
#define CTL_VPE_SCALE_RATIO_BASE 	(100)

typedef struct vos_list_head CTL_VPE_LIST_HEAD;

typedef enum {
	CTL_VPE_DBG_TS_SNDEVT,
	CTL_VPE_DBG_TS_ALLOC,
	CTL_VPE_DBG_TS_CONFIG_ED,
	CTL_VPE_DBG_TS_END_ISR,
	CTL_VPE_DBG_TS_CB_TSK_ST,
	CTL_VPE_DBG_TS_CB_TSK_ED,
	CTL_VPE_DBG_TS_MAX,
} CTL_VPE_DBG_TIMESTAMP;

typedef struct {
	UINT32 handle;
	UINT32 input_frm_count;
	UINT32 ts_flow[CTL_VPE_DBG_TS_MAX];
	INT32 err;
	UINT8 status;
	CTL_VPE_LIST_HEAD list;
} CTL_VPE_DBG_TS_NODE;

typedef struct {
	UINT8 enable;
	CHAR name[16];
	CHAR path[16];
	UINT32 save_input;
	UINT32 save_output;
} CTL_VPE_DBG_SAVEYUV_CFG;

typedef struct {
	UINT8 dump_info;
	UINT8 dump_file;
	UINT32 timeout_ms;
} CTL_VPE_DBG_ENG_HANG;

typedef struct {
	UINT32 cmd;
	UINT32 param[5];
	UINT32 rev[2];
	UINT32 timestamp;
	CTL_VPE_LIST_HEAD list;
} CTL_VPE_MSG_EVENT;

typedef struct {
	ISIZE size;
	UINT32 src_img_h_align;
} CTL_VPE_SRC_IMG;

typedef struct {
	CTL_VPE_SRC_IMG src_img;	/* debug only, information from input vdo frame */
	CTL_VPE_IN_CROP in_crop;
	USIZE dce_out_sz;
	CTL_VPE_OUT_PATH out_path[CTL_VPE_OUT_PATH_ID_MAX];
	UINT32 out_path_h_align[CTL_VPE_OUT_PATH_ID_MAX];
	CTL_VPE_DCS_MODE dcs_mode;
	UINT32 reserved;
} CTL_VPE_BASEINFO;

typedef struct {
	UINT32 tag;
	KDRV_VPE_JOB_LIST kdrv_job;
	KDRV_VPE_CONFIG kdrv_cfg;
	void *owner;
	CTL_VPE_EVT in_evt;
	CTL_VPE_OUT_BUF_INFO buf_info[CTL_VPE_OUT_PATH_ID_MAX];
	CTL_VPE_DBG_TS_NODE *p_dbg_node;
	CTL_VPE_LIST_HEAD list;
} CTL_VPE_JOB;

/* special case for 2dlut isp param
	base on CTL_VPE_ISP_DCE_2DLUT_PARAM, but lut change to pointer
*/
typedef struct {
	CTL_VPE_ISP_2DLUT_SZ lut_sz;
	UINT32 lut_paddr;
} CTL_VPE_INT_DCE_USER_2DLUT_PARAM;

/* special case for 2dlut isp param
	base on CTL_VPE_ISP_DCE_2DLUT_PARAM, but lut change to pointer
*/
typedef struct {
	UINT8 xofs_i;
	UINT32 xofs_f;
	UINT8 yofs_i;
	UINT32 yofs_f;
	CTL_VPE_ISP_2DLUT_SZ lut_sz;
	UINT32 *p_lut;
	UINT32 lut_buf_sz;
	USIZE 	out_size;	//2dlut output size
} CTL_VPE_ISP_INT_DCE_2DLUT_PARAM;

typedef struct {
	CTL_VPE_ISP_SHARPEN_PARAM 		sharpen;
	CTL_VPE_ISP_DCE_CTL 			dce_ctl;
	CTL_VPE_ISP_DCE_GDC_PARAM 		dce_gdc_param;
	CTL_VPE_ISP_INT_DCE_2DLUT_PARAM	dce_2dlut_param;
	CTL_VPE_ISP_YUV_CVT_PARAM 		yuv_cvt_param;
	CTL_VPE_ISP_DCTG_PARAM 			dctg_param;
	CTL_VPE_ISP_FLIP_ROT_CTL 		flip_rot_ctl;
} CTL_VPE_ISP_PARAM;

typedef struct {
	BOOL dce_en;
	CTL_VPE_ISP_DCE_MODE dce_mode;
	CTL_VPE_ISP_2DLUT_SZ lut_sz;
} CTL_VPE_PRI_BUF_CFG_PARAM;

typedef struct {
	atomic_t err_sys;
	atomic_t err_par;
	atomic_t err_isp_par;
	atomic_t err_nomem;
	atomic_t err_indata;
	atomic_t err_kdrv_trig;
	atomic_t err_kdrv_drop;
} CTL_VPE_DBG_ERR_ACCU_INFO;

typedef struct {
	UINT32 tag;
	CHAR name[16];
	CTL_VPE_BASEINFO rtc_info;	/* info that keep setting from api, wait for set apply */
	CTL_VPE_BASEINFO ctl_info;	/* info that is currently used */
	CTL_VPE_EVENT_FP cb_fp[CTL_VPE_CBEVT_MAX];
	CTL_VPE_JOB *p_fired_job;
	UINT32 kdrv_id;
	UINT32 isp_id;
	UINT32 in_job_cnt;
	UINT32 out_job_cnt;
	UINT32 out_job_err_cnt;		/* number in out_job_cnt with (err_msg != E_OK) */
	UINT32 out_job_inq_cnt;		/* job number in cb queue */
	vk_spinlock_t lock;
	ID sem;						/* for vpe api racing protect(ex: close & sndevt at same time) */
	UINT32 sndevt_rwlock;		/* rwlock for sndevt, bit0: writer lock, bit4~30: reader count, bit31: protect bit */

	CTL_VPE_ISP_PARAM isp_param;
	CTL_VPE_PRI_BUF_CFG_PARAM pri_buf_cfg_param;
	vk_spinlock_t isp_lock;

	CTL_VPE_LIST_HEAD list;
	UINT32 reserved[8];
	CTL_VPE_DBG_ERR_ACCU_INFO dbg_err_accu_info;
} CTL_VPE_HANDLE;

typedef struct {
	CHAR name[16];
	CTL_VPE_LIST_HEAD free_list_head;
	CTL_VPE_LIST_HEAD used_list_head;
	UINT32 start_addr;
	UINT32 total_size;
	UINT32 blk_num;
	UINT32 blk_size;
	UINT32 cur_free_num;
	UINT32 max_used_num;
	vk_spinlock_t lock;
	ID flg_id;
} CTL_VPE_MEM_POOL;

typedef struct {
	vk_spinlock_t common_lock;
	CTL_VPE_MEM_POOL hdl_pool;
	CTL_VPE_MEM_POOL job_pool;
	CTL_VPE_MEM_POOL in_evt_que;
	CTL_VPE_MEM_POOL cb_evt_que;
	UINT32 kdrv_open_cnt;

	THREAD_HANDLE proc_tsk_id;
	ID proc_tsk_flg_id;
	vk_spinlock_t proc_tsk_lock;

	THREAD_HANDLE cb_tsk_id;
	ID cb_tsk_flg_id;
	vk_spinlock_t cb_tsk_lock;

	/* DEBUG */
	UINT32 dump_file_at_eng_hang;
	CTL_VPE_MEM_POOL dbg_ts_pool;
	CTL_VPE_DBG_SAVEYUV_CFG dbg_saveyuv_cfg;
	CTL_VPE_DBG_ENG_HANG dbg_eng_hang;
} CTL_VPE_CTL;

typedef struct {
	UINT32 hdl;
	UINT32 data;
	UINT32 buf_id;
	INT32 err;
	void *p_modify_info;
} CTL_VPE_MSG_MODIFY_CB_DATA;

/* api that use mem_pool as queue of CTL_VPE_MSG_EVENT */
typedef INT32 (*CTL_VPE_MSG_FLUSH_CB)(UINT32 hdl, UINT32 data, UINT32 buf_id, INT32 err);
typedef INT32 (*CTL_VPE_MSG_MODIFY_CB)(CTL_VPE_MSG_MODIFY_CB_DATA *p_data);
INT32 ctl_vpe_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, CTL_VPE_MEM_POOL *p_que);
INT32 ctl_vpe_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, UINT32 *time, CTL_VPE_MEM_POOL *p_que);
INT32 ctl_vpe_msg_flush(CTL_VPE_MEM_POOL *p_que, CTL_VPE_HANDLE *p_hdl, CTL_VPE_MSG_FLUSH_CB flush_cb);
INT32 ctl_vpe_msg_modify(CTL_VPE_MEM_POOL *p_que, CTL_VPE_HANDLE *p_hdl, CTL_VPE_MSG_MODIFY_CB modify_cb, void *p_data);
INT32 ctl_vpe_msg_init_queue(CTL_VPE_MEM_POOL *p_que);


#if 0
#endif

CTL_VPE_HANDLE *ctl_vpe_int_get_handle_by_isp_id(UINT32 isp_id);

UINT32 ctl_vpe_util_y2uvlof(VDO_PXLFMT fmt, UINT32 y_lof);
UINT32 ctl_vpe_util_y2uvwidth(VDO_PXLFMT fmt, UINT32 y_w);
UINT32 ctl_vpe_util_y2uvheight(VDO_PXLFMT fmt, UINT32 y_h);
UINT32 ctl_vpe_util_yuv_size(VDO_PXLFMT fmt, UINT32 y_width, UINT32 y_height);

INT32 ctl_vpe_process_d2d(CTL_VPE_HANDLE *p_hdl, CTL_VPE_JOB *p_job, void *cb_fp);
INT32 ctl_vpe_inbuf_cb_wrapper(CTL_VPE_EVENT_FP bufio_fp, CTL_VPE_EVT *p_evt, CTL_VPE_BUF_IO bufio);
INT32 ctl_vpe_outbuf_cb_wrapper(CTL_VPE_EVENT_FP bufio_fp, CTL_VPE_OUT_BUF_INFO *p_buf, CTL_VPE_BUF_IO bufio);

#if 0
#endif
void ctl_vpe_process_dbg_dump_kflow_isp_param(CTL_VPE_ISP_PARAM *p_isp, int (*dump)(const char *fmt, ...));
void ctl_vpe_process_dbg_dump_kflow_cfg(CTL_VPE_BASEINFO *p_base, int (*dump)(const char *fmt, ...));
void ctl_vpe_process_dbg_dump_kdrv_cfg(CTL_VPE_JOB *p_job, int (*dump)(const char *fmt, ...));
UINT32 ctl_vpe_process_dbg_dump_cfg(UINT32 op, UINT32 val);

void ctl_vpe_dump_job_ts(int (*dump)(const char *fmt, ...));
void ctl_vpe_save_yuv_cfg(CHAR *name, CHAR *path, UINT32 save_input, UINT32 save_output);
void ctl_vpe_dbg_eng_hang_cfg(CTL_VPE_DBG_ENG_HANG *p_eng_hang);

int ctl_vpe_int_printf(const char *fmtstr, ...);

#endif
