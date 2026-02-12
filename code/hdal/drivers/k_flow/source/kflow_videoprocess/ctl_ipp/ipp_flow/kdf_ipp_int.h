/**
    IPL KDF Layer

    @file       kdf_ipp_int.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _KDF_IPP_INT_H
#define _KDF_IPP_INT_H

#include "kwrap/type.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/kdrv_ipp_builtin.h"
#include "kdrv_builtin/nvtmpp_init.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_videoprocess/ipp_event.h"
#include "ipp_debug_int.h"
#include "ctl_ipp_util_int.h"

/**********************************/
/*	kdf_ipl task ctrl			  */
/**********************************/
#define KDF_IPP_TASK_TIMEOUT_MS			(60000)
#define DATA_IN_NUM						(3)

ER kdf_ipp_open_tsk(void);
ER kdf_ipp_close_tsk(void);
ER kdf_ipp_set_resume(BOOL b_flush_evt);
ER kdf_ipp_set_pause(BOOL b_wait_end, BOOL b_flush_evt);
ER kdf_ipp_wait_pause_end(void);

/**********************************/
/*	kdf_ipl message queue		  */
/**********************************/
#define KDF_IPP_MSG_STS_FREE    0x00000000
#define KDF_IPP_MSG_STS_LOCK    0x00000001

#define KDF_IPP_MSG_IGNORE					0x00000000
#define KDF_IPP_MSG_PROCESS					0x00000001  //[0]: handle, [1]: header address, [2]: reserve
#define KDF_IPP_MSG_DROP					0x00000002  //[0]: handle, [1]: header address, [2]: reserve, for erase queue used
#define KDF_IPP_MSG_HWRST					0x00000003	//[0]: handle, [1]: header address, [2]: reserve
#define KDF_IPP_MSG_BUILTIN_EXIT			0x00000004	//[0]: reserve, [1]: reserve, [2]: reserve
#define KDF_IPP_MSG_PROCESS_PATTERN_PASTE	0x00000005  //[0]: handle, [1]: header address, [2]: reserve

typedef struct {
	UINT32 cmd;
	UINT32 param[5];
	UINT32 rev[2];
	UINT32 count;
	IPP_EVENT_FP drop_fp;
	CTL_IPP_LIST_HEAD list;
	UINT64 snd_time;
} KDF_IPP_MSG_EVENT;

ER kdf_ipp_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, IPP_EVENT_FP drop_fp, UINT32 count);
ER kdf_ipp_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, IPP_EVENT_FP *drop_fp, UINT64 *snd_t);
ER kdf_ipp_msg_flush(void);
ER kdf_ipp_erase_queue(UINT32 handle);
ER kdf_ipp_disable_path_dma_in_queue(UINT32 handle, UINT32 pid);
ER kdf_ipp_msg_reset_queue(void);
UINT32 kdf_ipp_get_free_queue_num(void);

/**********************************/
/*	kdf_ipl handle ctrl			  */
/**********************************/
#define KDF_IPP_HANDLE_FREE_FLAG_START			(1)									/* pool lock bit at first flag_id */
#define KDF_IPP_HANDLE_AVAILABLE_FLAG_BIT		(CTL_IPP_UTIL_FLAG_PAGE_SIZE - 1)	/* Timeout bit for each flag_id */

#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
#define KDF_IPP_HANDLE_LOCK_TIMEOUT_MS			(5000)
#else
#define KDF_IPP_HANDLE_LOCK_TIMEOUT_MS			(1000)
#endif

#define KDF_IPP_HANDLE_NAME_MAX				(16)

#define KDF_IPP_HANDLE_STS_MASK				(0x5A5A0000)
#define KDF_IPP_HANDLE_STS_FREE				(KDF_IPP_HANDLE_STS_MASK | 1)
#define KDF_IPP_HANDLE_STS_READY			(KDF_IPP_HANDLE_STS_MASK | 2)

#define KDF_IPP_HANDLE_KDRV_CH_NONE			(0xFFFF)

typedef void (*KDF_IPP_DUMP_FP)(char *fmt, ...);

typedef enum {
	KDF_IPP_FLOW_UNKNOWN = 0,
	KDF_IPP_FLOW_RAW,
	KDF_IPP_FLOW_DIRECT_RAW,
	KDF_IPP_FLOW_CCIR,
	KDF_IPP_FLOW_DIRECT_CCIR,
	KDF_IPP_FLOW_IME_D2D,
	KDF_IPP_FLOW_IPE_D2D,
	KDF_IPP_FLOW_DCE_D2D,
	KDF_IPP_FLOW_MAX
} KDF_IPP_FLOW_TYPE;

typedef enum {
	KDF_IPP_CBEVT_PROCEND = 0,	/* call at isr context */
	KDF_IPP_CBEVT_DROP,			/* call at isr context */
	KDF_IPP_CBEVT_PRESET,		/* call at kdf_ipl task */
	KDF_IPP_CBEVT_ENG_RHE_ISR,	/* call at isr context */
	KDF_IPP_CBEVT_ENG_IFE_ISR,	/* call at isr context */
	KDF_IPP_CBEVT_ENG_DCE_ISR,	/* call at isr context */
	KDF_IPP_CBEVT_ENG_IPE_ISR,	/* call at isr context */
	KDF_IPP_CBEVT_ENG_IME_ISR,	/* call at isr context */
	KDF_IPP_CBEVT_ENG_IFE2_ISR,	/* call at isr context */
	KDF_IPP_CBEVT_PROC_TIME,	/* call at isr context */
	KDF_IPP_CBEVT_DATASTAMP,	/* call at isr context */
	KDF_IPP_CBEVT_PRIMASK,		/* call at isr context */
	KDF_IPP_CBEVT_ISP,			/* call at kdf_ipl task & isr context */
	KDF_IPP_CBEVT_PROCSTART,	/* call at isr context */
	KDF_IPP_CBEVT_FRAMESTART,	/* call at isr context */
	KDF_IPP_CBEVT_LOWDELAY,		/* call at isr context */
	KDF_IPP_CBEVT_MAX
} KDF_IPP_CBEVT_TYPE;

typedef struct {
	UINT32	data_addr;
	UINT32	count;
	UINT32	rev;
	INT32   err_msg;
} KDF_IPP_EVT;

typedef struct {
	UINT32	data_addr;
	UINT32	count;
	UINT32	rev;
	INT32   err_msg;
} KDF_IPP_EVT_PATTERN_PASTE;

typedef struct {
	KDF_IPP_CBEVT_TYPE cbevt;
	IPP_EVENT_FP fp;
} KDF_IPP_REG_CB_INFO;

typedef enum {
	KDF_IPP_ISP_CB_TYPE_IQ_TRIG = 0,
	KDF_IPP_ISP_CB_TYPE_IQ_GET,
	KDF_IPP_ISP_CB_TYPE_INT_EVT,
	KDF_IPP_ISP_CB_TYPE_MAX,
} KDF_IPP_ISP_CB_TYPE;

typedef struct {
	KDF_IPP_ISP_CB_TYPE type;	/* isp callback type */
	UINT32 item;				/* isp callback item for each type, isp_event for trig, isp_item for get */
	UINT32 base_info_addr;		/* baseinfo address */
	UINT32 data_addr;			/* data address for iq isp_api */
} KDF_IPP_ISP_CB_INFO;

typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
} KDF_IPP_FLUSH_CONFIG;

typedef struct {
	USIZE in_size;			/* input size */
	BOOL gdc_enable;		/* input gdc enable for stripe cal rule */

	UINT32 stripe_num;		/* output stripe number */
} KDF_IPP_STRIPE_NUM_INFO;

typedef enum {
	KDF_IPP_ITEM_FLOWTYPE = 0,	/* KDF Handle flow type.            GET-Only,   data_type: KDF_IPP_FLOW_TYPE    */
	KDF_IPP_ITEM_REG_CB,		/* KDF Callback function register.	GET/SET,	data_type: KDF_IPP_REG_CB_INFO	*/
	KDF_IPP_ITEM_FLUSH,			/* KDF flush all event in queue.	GET/SET,	data_type: NONE                 */
	KDF_IPP_ITEM_STRIPE_NUM,	/* KDF cal stripe number by dce.	GET-Only,	data_type: KDF_IPP_STRIPE_NUM_INFO */
	KDF_IPP_ITEM_FIRST_HDL,		/* KDF frist handle                 SET-Only    data_type: NULL                 */
	KDF_IPP_ITEM_MAX
} KDF_IPP_ITEM;

typedef enum {
	KDF_IPP_IOCTL_SNDEVT = 0,			/* Send kdf event.						data_type: KDF_IPP_EVT		*/
	KDF_IPP_IOCTL_DIRECT_START,			/* Direct mode, eng start.				data_type: TBD				*/
	KDF_IPP_IOCTL_DIRECT_STOP,			/* Direct mode, eng stop.				data_type: TBD				*/
	KDF_IPP_IOCTL_DIRECT_CFG,			/* Direct mode, set input param.		data_type: TBD				*/
	KDF_IPP_IOCTL_DIRECT_ONESHOT,		/* Direct mode, set one-shot output	data_type: TBD					*/
	KDF_IPP_IOCTL_FREE_QUEUE,			/* Direct mode, set one-shot output	data_type: TBD					*/
	KDF_IPP_IOCTL_DIRECT_WAKEUP,		/* Direct mode, eng wakeup.				data_type: TBD				*/
	KDF_IPP_IOCTL_DIRECT_SLEEP,			/* Direct mode, eng sleep.				data_type: TBD				*/
	KDF_IPP_IOCTL_SNDEVT_PATTERN_PASTE,	/* Send kdf event, pattern paste.		data_type: KDF_IPP_EVT_PATTERN_PASTE */
	KDF_IPP_IOCTL_MAX
} KDF_IPP_IOCTL;

typedef enum {
	KDF_IPP_DIRECT_MSG_START = 0,	/* Direct mode, eng start.				data_type: TBD				*/
	KDF_IPP_DIRECT_MSG_CFG,			/* Direct mode, eng start.				data_type: TBD				*/
	KDF_IPP_DIRECT_MSG_STOP,		/* Direct mode, eng start.				data_type: TBD				*/
	KDF_IPP_DIRECT_MSG_MAX
} KDF_IPP_DIRECT_MSG;

typedef struct {
	UINT32 flag;
	UINT32 ptn;
} KDF_IPP_HANDLE_LOCKINFO;

typedef enum {
	KDF_IPP_ENG_RHE = 0,
	KDF_IPP_ENG_IFE,
	KDF_IPP_ENG_DCE,
	KDF_IPP_ENG_IPE,
	KDF_IPP_ENG_IME,
	KDF_IPP_ENG_IFE2,
	KDF_IPP_ENG_MAX
} KDF_IPP_ENG;

typedef enum {
	KDF_IPP_ENG_BIT_RHE = (1 << KDF_IPP_ENG_RHE),
	KDF_IPP_ENG_BIT_IFE = (1 << KDF_IPP_ENG_IFE),
	KDF_IPP_ENG_BIT_DCE = (1 << KDF_IPP_ENG_DCE),
	KDF_IPP_ENG_BIT_IPE = (1 << KDF_IPP_ENG_IPE),
	KDF_IPP_ENG_BIT_IME = (1 << KDF_IPP_ENG_IME),
	KDF_IPP_ENG_BIT_IFE2 = (1 << KDF_IPP_ENG_IFE2),
} KDF_IPP_ENG_BIT;

typedef enum {
	KDF_IPP_DIR_LOAD_VD		 = 0X00000001,
	KDF_IPP_DIR_LOAD_FMS     = 0X00000002,
	KDF_IPP_DIR_LOAD_PAUSE   = 0X00000004,
	KDF_IPP_DIR_LOAD_PAUSE_E = 0X00000008,
} KDF_IPP_DIR_LOAD_BIT;

typedef enum {
	KDF_IPP_FRAME_START		 = 0X00000001,
	KDF_IPP_FRAME_END		 = 0X00000002,
	KDF_IPP_FRAME_BP		 = 0X00000004,
	KDF_IPP_FRAME_BP_END	 = 0X00000008,
	KDF_IPP_FRAME_BP_START	 = 0X00000010,
} KDF_IPP_PROC_STS;

typedef struct {
	CHAR name[KDF_IPP_HANDLE_NAME_MAX];
	UINT32 sts;
	UINT32 lock;
	vk_spinlock_t spinlock;				/* spinlock for handle, current use to protect data_in_process when direct mode */
	CTL_IPP_LIST_HEAD list;

	KDF_IPP_FLOW_TYPE flow;
	IPP_EVENT_FP cb_fp[KDF_IPP_CBEVT_MAX];
	UINT32 proc_end_bit;
	UINT32 proc_end_mask;
	UINT32 frame_sta_bit;
	UINT32 frame_sta_mask;
	UINT32 dir_load_bit;                /* load register when vd and frame end */
	UINT32 data_addr_in_process[DATA_IN_NUM];		/* direct mode need pingpong address */
	UINT64 snd_evt_time;
	UINT64 rev_evt_time;
	UINT64 proc_st_time;
	UINT64 trig_ed_time;
	UINT64 ime_bp_time;
	UINT32 proc_sts;                    /**/
	UINT32 is_first_handle;				/* first handle, for fastboot flow control set by ctl_ipp */
	UINT32 engine_used_mask;

	/* ideal case
		snd_evt_cnt = proc_end_cnt + drop_cnt
	*/
	UINT32 snd_evt_cnt;
	UINT32 proc_end_cnt;
	UINT32 drop_cnt;
	UINT32 frame_sta_cnt;
	UINT32 reset_cnt;
	UINT32 fra_sta_with_sop_cnt;

	/* kdriver usage */
	UINT32 chip;
	UINT32 channel[KDF_IPP_ENG_MAX];

	UINT32 eng_err_sts[KDF_IPP_ENG_MAX];
	UINT32 eng_err_frm[KDF_IPP_ENG_MAX];
} KDF_IPP_HANDLE;

typedef struct {
	CTL_IPP_LIST_HEAD free_head;
	CTL_IPP_LIST_HEAD used_head;
	UINT32 flg_num;
	ID *flg_id;
	UINT32 handle_num;
	KDF_IPP_HANDLE *handle;
} KDF_IPP_HANDLE_POOL;

typedef enum {
	KDF_IPP_ENG_HANG_HWRST =		0x00000001,
	KDF_IPP_ENG_HANG_DUMP =			0x00000002,
	KDF_IPP_ENG_HANG_SAVE_INPUT =	0x00000004,
	KDF_IPP_ENG_HANG_HANDLE_MODE_MAX,
} KDF_IPP_ENG_HANG_HANDLE_MODE;

typedef struct {
	KDF_IPP_ENG_HANG_HANDLE_MODE mode;
	CHAR file_path[32];
} KDF_IPP_HWRST_CFG;

typedef INT32 (*KDF_IPP_IOCTL_FP)(KDF_IPP_HANDLE*, void*);
typedef INT32 (*KDF_IPP_SET_FP)(KDF_IPP_HANDLE*, void*);
typedef INT32 (*KDF_IPP_GET_FP)(KDF_IPP_HANDLE*, void*);

void kdf_ipp_eng_hang_config(KDF_IPP_ENG_HANG_HANDLE_MODE mode, CHAR *f_path);
void kdf_ipp_eng_hang_manual(KDF_IPP_ENG_HANG_HANDLE_MODE mode, CHAR *f_path);
void kdf_ipp_handle_reset(KDF_IPP_HANDLE *p_hdl);
INT32 kdf_ipp_handle_lock(UINT32 lock, BOOL b_timeout_en);
void kdf_ipp_handle_unlock(UINT32 lock);
INT32 kdf_ipp_handle_validate(KDF_IPP_HANDLE *p_hdl);
INT32 kdf_ipp_process(UINT32 hdl, UINT32 data_addr, UINT32 rev, UINT64 snd_time, UINT32 proc_cmd);
INT32 kdf_ipp_drop(UINT32 hdl, UINT32 data_addr, UINT32 rev, IPP_EVENT_FP drop_fp, INT32 err_msg);

UINT32 kdf_ipp_init(UINT32 num, UINT32 buf_addr, UINT32 is_query);
void kdf_ipp_uninit(void);
UINT32 kdf_ipp_open(CHAR *name, KDF_IPP_FLOW_TYPE flow);
INT32 kdf_ipp_close(UINT32 hdl);
INT32 kdf_ipp_set(UINT32 hdl, UINT32 item, void *data);
INT32 kdf_ipp_get(UINT32 hdl, UINT32 item, void *data);
INT32 kdf_ipp_ioctl(UINT32 hdl, UINT32 item, void *data);

INT32 kdf_ipp_rhe_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out);
INT32 kdf_ipp_ife_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out);
INT32 kdf_ipp_dce_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out);
INT32 kdf_ipp_ipe_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out);
INT32 kdf_ipp_ime_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out);
INT32 kdf_ipp_ife2_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out);

INT32 kdf_ipp_dir_wait_pause_end(void);
INT32 kdf_ipp_dir_eng_lock(KDF_IPP_HANDLE *p_hdl);


/**********************************/
/*	kdf_ipl process				  */
/**********************************/
//#define KDF_IPP_DFT_RHE_INTE (KDRV_RHE_INTE_FLDEND)
#define KDF_IPP_DFT_IFE_INTE (KDRV_IFE_INTE_FMD | KDRV_IFE_INT_DEC1_ERR | KDRV_IFE_INT_DEC2_ERR | KDRV_IFE_INT_LLEND | KDRV_IFE_INT_LLERR | KDRV_IFE_INT_LLERR2 | KDRV_IFE_INT_LLJOBEND | KDRV_IFE_INT_BUFOVFL | KDRV_IFE_INT_RING_BUF_ERR | KDRV_IFE_INT_FRAME_ERR| KDRV_IFE_INT_SIE_FRAME_START)
#define KDF_IPP_DFT_DCE_INTE (KDRV_DCE_INTE_ALL)
#define KDF_IPP_DFT_IPE_INTE (KDRV_IPE_INTE_ALL)
#define KDF_IPP_DFT_IME_INTE (KDRV_IME_INTE_FRM_END | KDRV_IME_INTE_FRM_START | KDRV_IME_INTE_BP3)
#define KDF_IPP_DFT_IFE2_INTE (KDRV_IFE2_INTE_FMD)

#define KDF_IPP_PROC_IFE_INTE (KDRV_IFE_INTE_FMD | KDRV_IFE_INT_SIE_FRAME_START)
#define KDF_IPP_PROC_DCE_INTE (CTL_IPP_DCE_INTE_FST | CTL_IPP_DCE_INTE_FMD)
#define KDF_IPP_PROC_IPE_INTE (CTL_IPP_IPE_INTE_FMS | CTL_IPP_IPE_INTE_FMD)
#define KDF_IPP_PROC_IME_INTE (KDRV_IME_INTE_FRM_END | KDRV_IME_INTE_FRM_START | KDRV_IME_INTE_BP3)
#define KDF_IPP_PROC_IFE2_INTE (KDRV_IFE2_INTE_FMD)

//#define KDF_IPP_DFT_RHE_FUNCEN	(KDRV_RHE_IQ_FUNC_CG | KDRV_RHE_IQ_FUNC_FILTER | KDRV_RHE_IQ_FUNC_BLEND | KDRV_RHE_IQ_FUNC_LTC | KDRV_RHE_IQ_FUNC_TONE_CRUVE | KDRV_RHE_IQ_FUNC_CSAT | KDRV_RHE_IQ_FUNC_CNS | KDRV_RHE_IQ_FUNC_CADJ)
#define KDF_IPP_DFT_IFE_FUNCEN	(KDRV_IFE_IQ_FUNC_ALL)
#define KDF_IPP_DFT_DCE_FUNCEN	(KDRV_DCE_IQ_FUNC_CFA | KDRV_DCE_IQ_FUNC_DC | KDRV_DCE_IQ_FUNC_CAC | KDRV_DCE_IQ_FUNC_2DLUT | KDRV_DCE_IQ_FUNC_WDR | KDRV_DCE_IQ_FUNC_TONECURVE | KDRV_DCE_IQ_FUNC_HISTOGRAM | KDRV_DCE_IQ_FUNC_FOV)
#define KDF_IPP_DFT_IPE_FUNCEN	(KDRV_IPE_IQ_FUNC_ALL)
#define KDF_IPP_DFT_IME_FUNCEN	(KDRV_IME_IQ_FUNC_ALL)
#define KDF_IPP_DFT_IFE2_FUNCEN	(KDRV_IFE2_IQ_FUNC_FILTER)

#define KDF_IPP_ENG_BIT_ALL	(KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | \
								KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME | KDF_IPP_ENG_BIT_IFE2)

#define KDF_IPP_RAW_ENG_MASK (KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | \
								KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME | KDF_IPP_ENG_BIT_IFE2)

#define KDF_IPP_CCIR_ENG_MASK (KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME | KDF_IPP_ENG_BIT_IFE2)


KDF_IPP_HANDLE *kdf_ipp_get_trig_hdl(KDF_IPP_ENG eng);
void kdf_ipp_set_trig_hdl(KDF_IPP_HANDLE *p_hdl, KDF_IPP_ENG eng);
INT32 kdf_ipp_cb_proc(KDF_IPP_HANDLE *p_hdl, KDF_IPP_CBEVT_TYPE cbevt, UINT32 msg, void *in, void *out);
//void kdf_ipp_cal_vig_cen(KDRV_IFE_IN_IMG_INFO *in_img_info, UPOINT *ratio, UINT32 ratio_base, UPOINT *center);
void kdf_ipp_direct_load(KDF_IPP_HANDLE *p_hdl);
void kdf_ipp_direct_load_bit_set(UINT32 *load_bit, UINT32 bit);
void kdf_ipp_direct_load_bit_clr(UINT32 *load_bit, UINT32 bit);
void kdf_ipp_direct_clr_hdl(void);
UINT32 kdf_ipp_direct_datain_drop(KDF_IPP_HANDLE *p_hdl, UINT32 evt, UINT32 data_in);

INT32 kdf_ipp_process_raw(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev);
INT32 kdf_ipp_process_ccir(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev);
INT32 kdf_ipp_process_ime_d2d(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev);
INT32 kdf_ipp_process_ipe_d2d(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev);
INT32 kdf_ipp_process_dce_d2d(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev);
INT32 kdf_ipp_process_direct(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev);
INT32 kdf_ipp_process_flow_pattern_paste(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev);

/**********************************/
/*	kdf_ipl kdrv_api_wrapper	  */
/**********************************/
typedef INT32 (*KDF_IPP_KDRV_OPEN_FP)(UINT32 chip, UINT32 engine);
typedef INT32 (*KDF_IPP_KDRV_CLOSE_FP)(UINT32 chip, UINT32 engine);
typedef INT32 (*KDF_IPP_KDRV_TRIG_FP)(UINT32 id, void *param, KDRV_CALLBACK_FUNC *p_cb_func, void *p_user_data);
typedef INT32 (*KDF_IPP_KDRV_SET_FP)(UINT32 id, UINT32 param_id, void *p_param);
typedef INT32 (*KDF_IPP_KDRV_GET_FP)(UINT32 id, UINT32 param_id, void *p_param);
typedef INT32 (*KDF_IPP_KDRV_PAUSE_FP)(UINT32 chip, UINT32 engine);

#define KDF_IPP_KDRV_DBG_AVG_NUM	(30)
typedef struct {
	UINT32 open_cnt;
	UINT32 open_time_acc;
	UINT32 open_time_max;

	UINT32 close_cnt;
	UINT32 close_time_acc;
	UINT32 close_time_max;

	UINT32 trig_cnt;
	UINT32 trig_time_acc;
	UINT32 trig_time_max;

	UINT32 set_cnt;
	UINT32 set_time_acc;
	UINT32 set_time_max;
	UINT32 set_time_max_item;

	UINT32 get_cnt;
	UINT32 get_time_acc;
	UINT32 get_time_max;
	UINT32 get_time_max_item;

	UINT32 pause_cnt;
	UINT32 pause_time_acc;
	UINT32 pause_time_max;
} KDF_IPP_KDRV_DBG_INFO;

UINT32 kdf_ipp_kdrv_wrapper_init(UINT32 num, UINT32 buf_addr, UINT32 is_query);
void kdf_ipp_kdrv_wrapper_uninit(void);
void kdf_ipp_kdrv_wrapper_mount_api(BOOL dbg_mode);
INT32 kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl);
INT32 kdf_ipp_kdrv_close_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl);
INT32 kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl, void *param, void *p_user_data);
INT32 kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl, UINT32 param_id, void *data);
INT32 kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl, UINT32 param_id, void *data);
INT32 kdf_ipp_kdrv_pause_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl);
void kdf_ipp_kdrv_wrapper_debug_dump(int (*dump)(const char *fmt, ...));

/**********************************/
/*	kdf_ipl kdrv isr handle		  */
/**********************************/
#define KDF_IPP_KDRV_ISR_PRI_TABLE_MAX (4)
typedef enum {
	KDF_IPP_KDRV_ISR_STATE_UNKNOWN = 0,
	KDF_IPP_KDRV_ISR_STATE_START,
	KDF_IPP_KDRV_ISR_STATE_BP,
	KDF_IPP_KDRV_ISR_STATE_END,
	KDF_IPP_KDRV_ISR_STATE_MAX
} KDF_IPP_KDRV_ISR_STATE;

#endif //_KDF_IPP_INT_H
