/**
    @file       kdrv_ai.c
    @ingroup    Predefined_group_name

    @brief      ai device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <asm/io.h>
//#include <linux/uaccess.h>
//#include "mach/fmem.h"
#include "kwrap/semaphore.h"
//#include <linux/delay.h>
//#include <linux/timer.h>
//#include <linux/spinlock.h>

#include "ai_lib.h"
#include "kdrv_ai.h"
#include "cnn_lib.h"
#include "nue_lib.h"
#include "nue2_lib.h"
#include "kdrv_ai_int.h"
#include "kwrap/flag.h"

#if defined(__FREERTOS)
#include "kwrap/debug.h"
#include <string.h>
#include "ai_ioctl.h"
#else
#include "kdrv_ai_dbg.h"
#endif

#if LL_SUPPORT_ROI
#include "cnn_ll_cmd.h"
#endif

#include "kwrap/cpu.h"
#include "ai_api.h"
#if KDRV_AI_SINGLE_LAYER_CYCLE
#include "cnn_int.h"
#include "nue_int.h"
#include "nue2_int.h"
#endif
#define PROF                DISABLE
#if PROF
static struct timeval tstart, tend;
#define PROF_START()    do_gettimeofday(&tstart);
#define PROF_END(msg)   do_gettimeofday(&tend);     \
	printk("%s time (us): %lu\r\n", msg,        \
		   (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec));
#else
#define PROF_START()
#define PROF_END(msg)
#endif

/**
    ai tab selection
*/
typedef enum {
	KDRV_AI_TAB_HANDLE      = 0,
	KDRV_AI_TAB_PARM       	= 1,
	KDRV_AI_TAB_TIME,
	ENUM_DUMMY4WORD(KDRV_AI_TAB)
} KDRV_AI_TAB;
static UINT32 g_kdrv_ai_init_flg[AI_ENG_TOTAL] = {0};
static KDRV_AI_HANDLE *g_kdrv_ai_trig_hdl[AI_ENG_TOTAL] = {0};
static KDRV_AI_HANDLE g_kdrv_ai_handle_tab[AI_ENG_TOTAL][KDRV_AI_HANDLE_MAX_NUM] = {0};
static KDRV_AI_OPENCFG g_kdrv_ai_open_cfg[AI_ENG_TOTAL] = {0};
static INT32 g_kdrv_ai_open_times[AI_ENG_TOTAL] = {0};
static KDRV_AI_PRAM *g_kdrv_ai_trig_parm[AI_ENG_TOTAL] = {0};
static KDRV_AI_PRAM g_kdrv_ai_parm_tab[AI_ENG_TOTAL][KDRV_AI_HANDLE_MAX_NUM] = {0};
static KDRV_AI_TIME g_kdrv_ai_time_tab[AI_ENG_TOTAL][KDRV_AI_HANDLE_MAX_NUM] /*= {0}*/;

BOOL g_ai_isr_trig = 0;

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER kdrv_ai_app_set_parm(UINT32 id, KDRV_AI_APP_INFO *p_info);
ER kdrv_ai_app_get_parm(UINT32 id, KDRV_AI_APP_INFO *p_info);
ER kdrv_ai_app_trigger(UINT32 id);
#endif
ER kdrv_ai_ll_set_parm(UINT32 id, KDRV_AI_LL_INFO *p_info);
ER kdrv_ai_ll_get_parm(UINT32 id, KDRV_AI_LL_INFO *p_info);
ER kdrv_ai_ll_trigger(UINT32 id);
ER kdrv_ai_ll_done(UINT32 id);


#if (NEW_AI_FLOW == 1)
static int g_kdrv_ai_flow = 0;
void kdrv_ai_config_flow(UINT32 flow);
#endif

#if (NEW_AI_FLOW == 1)
void kdrv_ai_config_flow(UINT32 flow)
{
	g_kdrv_ai_flow = flow;
}
#endif

static KDRV_AI_ENG kdrv_ai_trans_id2eng(UINT32 eng_id)
{
	KDRV_AI_ENG ai_eng = AI_ENG_UNKNOWN;
	if (eng_id == KDRV_AI_ENGINE_CNN) {
		ai_eng = AI_ENG_CNN;
	} else if (eng_id == KDRV_AI_ENGINE_NUE) {
		ai_eng = AI_ENG_NUE;
	} else if (eng_id == KDRV_AI_ENGINE_NUE2) {
		ai_eng = AI_ENG_NUE2;
	} else if (eng_id == KDRV_AI_ENGINE_CNN2) {
		ai_eng = AI_ENG_CNN2;
	} else {
		ai_eng = AI_ENG_UNKNOWN;
	}
	return ai_eng;
}

#if KDRV_AI_FLG_IMPROVE
#else

static void kdrv_ai_lock(KDRV_AI_HANDLE *p_handle, BOOL flag)
{
	if (p_handle == NULL) {
		DBG_ERR("null handle...\r\n");
		return;
	}

	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}
#endif


static KDRV_AI_HANDLE *kdrv_ai_chk_handle(KDRV_AI_HANDLE *p_handle)
{
	UINT32 i, eng;
	if (p_handle == NULL) {
		return NULL;
	}

	for (eng = 0; eng < (UINT32)AI_ENG_TOTAL; eng++) {
		for (i = 0; i < KDRV_AI_HANDLE_MAX_NUM; i ++) {
			if (p_handle == &g_kdrv_ai_handle_tab[eng][i]) {
				return p_handle;
			}
		}
	}
	return NULL;
}

#if KDRV_AI_FLG_IMPROVE
static void kdrv_ai_handle_lock2(KDRV_AI_ENG eng)
{
	SEM_WAIT(ai_handle_sem_id[eng]);
}

static void kdrv_ai_handle_unlock2(KDRV_AI_ENG eng)
{
	SEM_SIGNAL(ai_handle_sem_id[eng]);
}
#else
static void kdrv_ai_handle_lock(KDRV_AI_ENG eng)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, FLG_ID_KDRV_AI[eng], KDRV_AI_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_ai_handle_unlock(KDRV_AI_ENG eng)
{
	set_flg(FLG_ID_KDRV_AI[eng], KDRV_AI_HDL_UNLOCK);
}
#endif

static ER kdrv_ai_handle_alloc(KDRV_AI_ENG eng)
{
	UINT32 i;
	ER rt = E_OK;

	if (eng < 0) {
		DBG_ERR("unknown engine : %d\r\n", (int)eng);
		return E_CTX;
	}
#if KDRV_AI_FLG_IMPROVE
	kdrv_ai_handle_lock2(eng);
#else
	kdrv_ai_handle_lock(eng);
#endif

	for (i = 0; i < KDRV_AI_HANDLE_MAX_NUM; i++) {
		if (!(g_kdrv_ai_init_flg[eng] & (1 << i))) {
			g_kdrv_ai_init_flg[eng] |= (1 << i);
			memset(&g_kdrv_ai_handle_tab[eng][i], 0, sizeof(KDRV_AI_HANDLE));
			g_kdrv_ai_handle_tab[eng][i].entry_id = i;
			g_kdrv_ai_handle_tab[eng][i].flag_id = FLG_ID_KDRV_AI[eng];
			g_kdrv_ai_handle_tab[eng][i].lock_bit = (1 << i);
			g_kdrv_ai_handle_tab[eng][i].sts |= KDRV_AI_HANDLE_LOCK;

		}
	}
#if KDRV_AI_FLG_IMPROVE
	kdrv_ai_handle_unlock2(eng);
#else
	kdrv_ai_handle_unlock(eng);
#endif

	return rt;
}

static ER kdrv_ai_handle_free(KDRV_AI_ENG eng)
{
	ER rt = E_OBJ;
	UINT32 ch = 0;
	KDRV_AI_HANDLE *p_handle;

	if (eng < 0) {
		DBG_ERR("unknown engine : %d\r\n", (int)eng);
		return E_CTX;
	}
#if KDRV_AI_FLG_IMPROVE
	kdrv_ai_handle_lock2(eng);
#else
	kdrv_ai_handle_lock(eng);
#endif
	for (ch = 0; ch < KDRV_AI_HANDLE_MAX_NUM; ch++) {
		p_handle = &(g_kdrv_ai_handle_tab[eng][ch]);
		p_handle->sts = 0;
		g_kdrv_ai_init_flg[eng] &= ~(1 << ch);
	}
	if (g_kdrv_ai_init_flg[eng] == 0) {
		rt = E_OK;
	}
#if KDRV_AI_FLG_IMPROVE
	kdrv_ai_handle_unlock2(eng);
#else
	kdrv_ai_handle_unlock(eng);
#endif

	return rt;
}

static VOID *kdrv_ai_id_conv2_tab(UINT32 id, KDRV_AI_TAB tab)

{
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);
	UINT32 engine = KDRV_DEV_ID_ENGINE(id);
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(engine);

	if (channel >= KDRV_AI_HANDLE_MAX_NUM) {
		DBG_ERR("chanel is overflow, should < %d\r\n", KDRV_AI_HANDLE_MAX_NUM);
		return NULL;
	} else if (ai_eng < 0) {
		DBG_ERR("unknown AI engine : %d\r\n", (int)ai_eng);
		return NULL;
	}

	switch (tab) {
	case KDRV_AI_TAB_HANDLE:
		return (VOID*)(&g_kdrv_ai_handle_tab[ai_eng][channel]);
	case KDRV_AI_TAB_PARM:
		return (VOID*)(&g_kdrv_ai_parm_tab[ai_eng][channel]);
	case KDRV_AI_TAB_TIME:
		return (VOID*)(&g_kdrv_ai_time_tab[ai_eng][channel]);
	default:
		DBG_ERR("unknown tab type : %d\r\n", (int)tab);

		return NULL;


	}
}

static KDRV_AI_HANDLE *kdrv_ai_id_conv2_handle(UINT32 id)
{
	return kdrv_ai_id_conv2_tab(id, KDRV_AI_TAB_HANDLE);
}

static KDRV_AI_PRAM *kdrv_ai_id_conv2_parm(UINT32 id)
{
	return kdrv_ai_id_conv2_tab(id, KDRV_AI_TAB_PARM);
}

#if (KDRV_AI_FLG_IMPROVE == 0)
static KDRV_AI_TIME *kdrv_ai_id_conv2_time(UINT32 id)
{
	return kdrv_ai_id_conv2_tab(id, KDRV_AI_TAB_TIME);
}
#endif

static void kdrv_ai_sem(SEM_HANDLE *p_handle, BOOL flag)
{
	if (p_handle == NULL) {
		DBG_ERR("null handle...\r\n");
		return;
	}

	if (flag == TRUE) {
		///< wait semaphore
		if (SEM_WAIT(*p_handle)) {
			return;
		}
	} else {
		///< release semaphore
		SEM_SIGNAL(*p_handle);
	}
}


#if (NEW_AI_FLOW == 1)
static void kdrv_ai_cnn_isr_cb2(UINT32 intstatus)
{
	KDRV_AI_PRAM *p_parm = g_kdrv_ai_trig_parm[AI_ENG_CNN];
	KDRV_AI_HANDLE *p_handle = g_kdrv_ai_trig_hdl[AI_ENG_CNN];
	if (((p_parm->mode == AI_TRIG_MODE_APP) && (intstatus & CNN_INT_FRM_DONE))
		|| ((p_parm->mode == AI_TRIG_MODE_LL) && (intstatus & CNN_INT_LLEND))) {
    	if (p_handle->isrcb_fp != NULL) {
    	    UINT32 exec_cycle = cnn_getcycle(0)*5; //cnn clk=600, clk ratio = 600/120 = 5
    		p_handle->isrcb_fp(exec_cycle, intstatus, AI_ENG_CNN, (void*)p_parm);
    	}
    }
}

static void kdrv_ai_cnn2_isr_cb2(UINT32 intstatus)
{
	KDRV_AI_PRAM *p_parm = g_kdrv_ai_trig_parm[AI_ENG_CNN2];
	KDRV_AI_HANDLE *p_handle = g_kdrv_ai_trig_hdl[AI_ENG_CNN2];
	if (((p_parm->mode == AI_TRIG_MODE_APP) && (intstatus & CNN_INT_FRM_DONE))
		|| ((p_parm->mode == AI_TRIG_MODE_LL) && (intstatus & CNN_INT_LLEND))) {
    	if (p_handle->isrcb_fp != NULL) {
    	    UINT32 exec_cycle = cnn_getcycle(1)*5; //cnn clk=600, clk ratio = 600/120 = 5
    		p_handle->isrcb_fp(exec_cycle, intstatus, AI_ENG_CNN2, (void*)p_parm);
    	}
	}
}

static void kdrv_ai_nue_isr_cb2(UINT32 intstatus)
{
	KDRV_AI_PRAM *p_parm = g_kdrv_ai_trig_parm[AI_ENG_NUE];
	KDRV_AI_HANDLE *p_handle = g_kdrv_ai_trig_hdl[AI_ENG_NUE];
	if (((p_parm->mode == AI_TRIG_MODE_APP) && (intstatus & NUE_INT_ROUTEND))
		|| ((p_parm->mode == AI_TRIG_MODE_LL) && (intstatus & NUE_INT_LLEND))) {
    	if (p_handle->isrcb_fp != NULL) {
    	    UINT32 exec_cycle = nue_getcycle(); //nue clk=600
    		p_handle->isrcb_fp(exec_cycle, intstatus, AI_ENG_NUE, (void*)p_parm);
    	}
	}
}

static void kdrv_ai_nue2_isr_cb2(UINT32 intstatus)
{
	KDRV_AI_PRAM *p_parm = g_kdrv_ai_trig_parm[AI_ENG_NUE2];
	KDRV_AI_HANDLE *p_handle = g_kdrv_ai_trig_hdl[AI_ENG_NUE2];
	if (((p_parm->mode == AI_TRIG_MODE_APP) && (intstatus & NUE2_INT_FRMEND))
		|| ((p_parm->mode == AI_TRIG_MODE_LL) && (intstatus & NUE2_INT_LLEND))) {
    	if (p_handle->isrcb_fp != NULL) {
    	    UINT32 exec_cycle = nue2_getcycle(); //nue2 clk=480
    		p_handle->isrcb_fp(exec_cycle, intstatus, AI_ENG_NUE2, (void*)p_parm);
    	}
	}
}
#endif


#if 0
static void kdrv_ai_frm_end(KDRV_AI_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		set_flg(p_handle->flag_id, KDRV_AI_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_AI_FMD);
	}
}
#endif

#if (KDRV_AI_FLG_IMPROVE == 0)
static void kdrv_ai_cnn_frm_end(KDRV_AI_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		set_flg(p_handle->flag_id, KDRV_AI_CNN_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_AI_CNN_FMD);
	}
}

static void kdrv_ai_cnn2_frm_end(KDRV_AI_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		set_flg(p_handle->flag_id, KDRV_AI_CNN2_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_AI_CNN2_FMD);
	}
}

static void kdrv_ai_nue_frm_end(KDRV_AI_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		set_flg(p_handle->flag_id, KDRV_AI_NUE_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_AI_NUE_FMD);
	}
}

static void kdrv_ai_nue2_frm_end(KDRV_AI_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		set_flg(p_handle->flag_id, KDRV_AI_NUE2_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_AI_NUE2_FMD);
	}
}

static void kdrv_ai_reset_eng(KDRV_AI_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		set_flg(p_handle->flag_id, KDRV_AI_RESET);
	} else {
		clr_flg(p_handle->flag_id, KDRV_AI_RESET);
	}
}

static void kdrv_ai_clear_timeout_flag(KDRV_AI_HANDLE *p_handle)
{
	clr_flg(p_handle->flag_id, KDRV_AI_TIMEOUT);
}


static void kdrv_ai_cnn_isr_cb(UINT32 intstatus)
{
	KDRV_AI_PRAM *p_parm = g_kdrv_ai_trig_parm[AI_ENG_CNN];
	KDRV_AI_HANDLE *p_handle = g_kdrv_ai_trig_hdl[AI_ENG_CNN];
	if (((p_parm->mode == AI_TRIG_MODE_APP) && (intstatus & CNN_INT_FRM_DONE))
		|| ((p_parm->mode == AI_TRIG_MODE_LL) && (intstatus & CNN_INT_LLEND))) {
		kdrv_ai_cnn_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, AI_ENG_CNN, NULL);
	}
}

static void kdrv_ai_cnn2_isr_cb(UINT32 intstatus)
{
	KDRV_AI_PRAM *p_parm = g_kdrv_ai_trig_parm[AI_ENG_CNN2];
	KDRV_AI_HANDLE *p_handle = g_kdrv_ai_trig_hdl[AI_ENG_CNN2];

	if (((p_parm->mode == AI_TRIG_MODE_APP) && (intstatus & CNN_INT_FRM_DONE))
		|| ((p_parm->mode == AI_TRIG_MODE_LL) && (intstatus & CNN_INT_LLEND))) {
		kdrv_ai_cnn2_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, AI_ENG_CNN2, NULL);
	}
}

static void kdrv_ai_nue_isr_cb(UINT32 intstatus)
{
	KDRV_AI_PRAM *p_parm = g_kdrv_ai_trig_parm[AI_ENG_NUE];
	KDRV_AI_HANDLE *p_handle = g_kdrv_ai_trig_hdl[AI_ENG_NUE];

	if (((p_parm->mode == AI_TRIG_MODE_APP) && (intstatus & NUE_INT_ROUTEND))
		|| ((p_parm->mode == AI_TRIG_MODE_LL) && (intstatus & NUE_INT_LLEND))) {
		kdrv_ai_nue_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, AI_ENG_NUE, NULL);
	}
}

static void kdrv_ai_nue2_isr_cb(UINT32 intstatus)
{
	KDRV_AI_PRAM *p_parm = g_kdrv_ai_trig_parm[AI_ENG_NUE2];
	KDRV_AI_HANDLE *p_handle = g_kdrv_ai_trig_hdl[AI_ENG_NUE2];

	if (((p_parm->mode == AI_TRIG_MODE_APP) && (intstatus & NUE2_INT_FRMEND))
		|| ((p_parm->mode == AI_TRIG_MODE_LL) && (intstatus & NUE2_INT_LLEND))) {
		kdrv_ai_nue2_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, AI_ENG_NUE2, NULL);
	}
}

#ifdef __KERNEL__
static enum hrtimer_restart kdrv_ai_cnn_syscall_timercb(struct hrtimer *timer)
{
	set_flg(g_kdrv_ai_trig_hdl[AI_ENG_CNN]->flag_id, KDRV_AI_TIMEOUT);
	DBG_IND("kdrv ai time out\r\n");
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart kdrv_ai_nue_syscall_timercb(struct hrtimer *timer)
{
	set_flg(g_kdrv_ai_trig_hdl[AI_ENG_NUE]->flag_id, KDRV_AI_TIMEOUT);
	DBG_IND("kdrv ai time out\r\n");
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart kdrv_ai_nue2_syscall_timercb(struct hrtimer *timer)
{
	set_flg(g_kdrv_ai_trig_hdl[AI_ENG_NUE2]->flag_id, KDRV_AI_TIMEOUT);
	DBG_IND("kdrv ai time out\r\n");
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart kdrv_ai_cnn2_syscall_timercb(struct hrtimer *timer)
{
	set_flg(g_kdrv_ai_trig_hdl[AI_ENG_CNN2]->flag_id, KDRV_AI_TIMEOUT);
	DBG_IND("kdrv ai time out\r\n");
	return HRTIMER_NORESTART;
}
#else
static void kdrv_ai_cnn_timeout_cb(UINT32 timer_id)
{
	set_flg(g_kdrv_ai_trig_hdl[AI_ENG_CNN]->flag_id, KDRV_AI_TIMEOUT);
}

static void kdrv_ai_nue_timeout_cb(UINT32 timer_id)
{
	set_flg(g_kdrv_ai_trig_hdl[AI_ENG_NUE]->flag_id, KDRV_AI_TIMEOUT);
}

static void kdrv_ai_nue2_timeout_cb(UINT32 timer_id)
{
	set_flg(g_kdrv_ai_trig_hdl[AI_ENG_NUE2]->flag_id, KDRV_AI_TIMEOUT);
}

static void kdrv_ai_cnn2_timeout_cb(UINT32 timer_id)
{
	set_flg(g_kdrv_ai_trig_hdl[AI_ENG_CNN2]->flag_id, KDRV_AI_TIMEOUT);
}
#endif

ER kdrv_ai_time_start(UINT32 id, UINT32 time_out_ms)
{
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
	KDRV_AI_TIME *p_timer = kdrv_ai_id_conv2_time(id);

	if (p_timer == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);

		return E_ID;
	}

#ifdef __KERNEL__
	p_timer->kt_periode = ktime_set(0, time_out_ms * 1000000);      //seconds,nanoseconds
	hrtimer_init(&p_timer->htimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
	if (ai_eng == AI_ENG_CNN) {
		p_timer->htimer.function = kdrv_ai_cnn_syscall_timercb;
	} else if (ai_eng == AI_ENG_NUE) {
		p_timer->htimer.function = kdrv_ai_nue_syscall_timercb;
	} else if (ai_eng == AI_ENG_NUE2) {
		p_timer->htimer.function = kdrv_ai_nue2_syscall_timercb;
	} else if (ai_eng == AI_ENG_CNN2) {
		p_timer->htimer.function = kdrv_ai_cnn2_syscall_timercb;
	}
	hrtimer_start(&p_timer->htimer, p_timer->kt_periode, HRTIMER_MODE_REL);
#else
	if (ai_eng == AI_ENG_CNN) {
		p_timer->timer_cb = kdrv_ai_cnn_timeout_cb;
	} else if (ai_eng == AI_ENG_NUE) {
		p_timer->timer_cb = kdrv_ai_nue_timeout_cb;
	} else if (ai_eng == AI_ENG_NUE2) {
		p_timer->timer_cb = kdrv_ai_nue2_timeout_cb;
	} else if (ai_eng == AI_ENG_CNN2) {
		p_timer->timer_cb = kdrv_ai_cnn2_timeout_cb;
	}

	if (SwTimer_Open(&timer_id, p_timer->timer_cb) != E_OK) {
		DBG_ERR("open timer fail\r\n");
		p_timer->is_open_timer = FALSE;
	} else {
		SwTimer_Cfg(p_timer->timer_id, time_out_ms, SWTIMER_MODE_ONE_SHOT);
		SwTimer_Start(p_timer->timer_id);
	}
#endif

	return E_OK;
}

ER kdrv_ai_time_stop(UINT32 id)
{
	KDRV_AI_TIME *p_timer = kdrv_ai_id_conv2_time(id);

	if (p_timer == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);

		return E_ID;
	}

#ifdef __KERNEL__
	hrtimer_cancel(&p_timer->htimer);
#else
	if (p_timer->is_open_timer) {
		SwTimer_Stop(p_timer->timer_id);
		SwTimer_Close(p_timer->timer_id);
	}
#endif

	return E_OK;
}
#endif

INT32 kdrv_ai_open(UINT32 chip, UINT32 engine)
{
	ER er_code = E_OK;
	int err = 0;
#if KDRV_AI_FLG_IMPROVE
#else
	KDRV_AI_HANDLE *p_handle;
#endif
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(engine);

	if (ai_eng < 0) {
		DBG_ERR("unknown AI engine : %d\r\n", (int)ai_eng);
		return E_ID;
	}

	kdrv_ai_sem(&SEMID_KDRV_AI[ai_eng], TRUE);

	if (g_kdrv_ai_init_flg[ai_eng] == 0) {
		if (kdrv_ai_handle_alloc(ai_eng) != E_OK) {
			DBG_WRN("KDRV AI: no free handle, max handle num = %d\r\n", KDRV_AI_HANDLE_MAX_NUM);
			er_code = E_SYS;
			goto open_fail;
		}
	}



	if ((g_kdrv_ai_open_times[ai_eng] < KDRV_AI_HANDLE_MAX_NUM) && (g_kdrv_ai_open_times[ai_eng] > 0)) {
	} else if (g_kdrv_ai_open_times[ai_eng] == 0) {
#if (KDRV_AI_FLG_IMPROVE == 0)
		KDRV_AI_OPENCFG kdrv_ai_open_obj = g_kdrv_ai_open_cfg[ai_eng];
#endif
		INT32 ch;
		for (ch = 0; ch < KDRV_AI_HANDLE_MAX_NUM; ch++) {
#if KDRV_AI_FLG_IMPROVE
			SEM_WAIT(ai_sem_id[ai_eng][ch]);
#else
			p_handle = (KDRV_AI_HANDLE *)&g_kdrv_ai_handle_tab[ai_eng][ch];
			kdrv_ai_lock(p_handle, TRUE);
#endif
		}

#if (NEW_AI_FLOW == 1)
		if (g_kdrv_ai_flow == 1) {

		if (ai_eng == AI_ENG_CNN) {
			CNN_OPENOBJ cnn_drv_obj;
			err = cnn_init(0);
			if (err != 0) {
				kdrv_ai_handle_free(ai_eng);
				DBG_ERR("AI_KDRV_AI_API: Error, cnn_init failed.\r\n");
				er_code = E_SYS;
            	goto open_fail;
			}
			cnn_drv_obj.clk_sel     = 0;
			cnn_drv_obj.fp_isr_cb   = kdrv_ai_cnn_isr_cb2;
			if (cnn_open(0, &cnn_drv_obj) != E_OK) {
				kdrv_ai_handle_free(ai_eng);
				DBG_WRN("KDRV AI: cnn_open failed\r\n");
				er_code = E_SYS;
				goto open_fail;
			}
			//DBG_WRN("KDRV AI: cnn opened\r\n");
		} else if (ai_eng == AI_ENG_NUE) {
			NUE_OPENOBJ nue_drv_obj;
			err = nue_init();
			if (err != 0) {
				kdrv_ai_handle_free(ai_eng);
                DBG_ERR("AI_KDRV_AI_API: Error, nue_init failed.\r\n");
                er_code = E_SYS;
                goto open_fail;
            }
			nue_drv_obj.clk_sel     = 0;
			nue_drv_obj.fp_isr_cb   = kdrv_ai_nue_isr_cb2;
			if (nue_open(&nue_drv_obj) != E_OK) {
				kdrv_ai_handle_free(ai_eng);
				DBG_WRN("KDRV AI: nue_open failed\r\n");
				er_code = E_SYS;
				goto open_fail;
			}
			//DBG_WRN("KDRV AI: nue opened\r\n");
		} else if (ai_eng == AI_ENG_NUE2) {
			NUE2_OPENOBJ nue2_drv_obj;
			err = nue2_init();
			if (err != 0) {
				kdrv_ai_handle_free(ai_eng);
                DBG_ERR("AI_KDRV_AI_API: Error, nue2_init failed.\r\n");
                er_code = E_SYS;
                goto open_fail;
            }
			nue2_drv_obj.clk_sel     = 0;
			nue2_drv_obj.fp_isr_cb   = kdrv_ai_nue2_isr_cb2;
			if (nue2_open(&nue2_drv_obj) != E_OK) {
				kdrv_ai_handle_free(ai_eng);
				DBG_WRN("KDRV AI: nue2_open failed\r\n");
				er_code = E_SYS;
				goto open_fail;
			}
			//DBG_WRN("KDRV AI: nue2 opened\r\n");
		} else if (ai_eng == AI_ENG_CNN2) {
			CNN_OPENOBJ cnn_drv_obj;
			err = cnn_init(1);
			if (err != 0) {
                kdrv_ai_handle_free(ai_eng);
                DBG_ERR("AI_KDRV_AI_API: Error, cnn2_init failed.\r\n");
                er_code = E_SYS;
                goto open_fail;
            }
			cnn_drv_obj.clk_sel     = 0;
			cnn_drv_obj.fp_isr_cb   = kdrv_ai_cnn2_isr_cb2;
			if (cnn_open(1, &cnn_drv_obj) != E_OK) {
				kdrv_ai_handle_free(ai_eng);
				DBG_WRN("KDRV AI: cnn_open failed\r\n");
				er_code = E_SYS;
				goto open_fail;
			}
			//DBG_WRN("KDRV AI: cnn2 opened\r\n");
		} else {
			DBG_WRN("KDRV AI: unknown engine!!\r\n");
		}

		} else { //g_kdr_ai_flow == 0
#endif
		if (ai_eng == AI_ENG_CNN) {
#if (KDRV_AI_FLG_IMPROVE == 0)
			CNN_OPENOBJ cnn_drv_obj;
			cnn_drv_obj.clk_sel     = kdrv_ai_open_obj.clock_sel;
			cnn_drv_obj.fp_isr_cb   = kdrv_ai_cnn_isr_cb;
			if (cnn_open(0, &cnn_drv_obj) != E_OK) {
#else
			if (cnn_open(0, NULL) != E_OK) {
#endif
				kdrv_ai_handle_free(ai_eng);
				DBG_WRN("KDRV AI: cnn_open failed\r\n");
				er_code = E_SYS;
				goto open_fail;
			}
			//DBG_WRN("KDRV AI: cnn opened\r\n");
		} else if (ai_eng == AI_ENG_NUE) {
#if (KDRV_AI_FLG_IMPROVE == 0)
			NUE_OPENOBJ nue_drv_obj;
			nue_drv_obj.clk_sel     = kdrv_ai_open_obj.clock_sel;
			nue_drv_obj.fp_isr_cb   = kdrv_ai_nue_isr_cb;
			if (nue_open(&nue_drv_obj) != E_OK) {
#else
			if (nue_open(NULL) != E_OK) {
#endif
				kdrv_ai_handle_free(ai_eng);
				DBG_WRN("KDRV AI: nue_open failed\r\n");
				er_code = E_SYS;
				goto open_fail;
			}
			//DBG_WRN("KDRV AI: nue opened\r\n");
		} else if (ai_eng == AI_ENG_NUE2) {
#if (KDRV_AI_FLG_IMPROVE == 0)
			NUE2_OPENOBJ nue2_drv_obj;
			nue2_drv_obj.clk_sel     = kdrv_ai_open_obj.clock_sel;
			nue2_drv_obj.fp_isr_cb   = kdrv_ai_nue2_isr_cb;
			if (nue2_open(&nue2_drv_obj) != E_OK) {
#else
			if (nue2_open(NULL) != E_OK) {
#endif
				kdrv_ai_handle_free(ai_eng);
				DBG_WRN("KDRV AI: nue2_open failed\r\n");
				er_code = E_SYS;
				goto open_fail;
			}
			//DBG_WRN("KDRV AI: nue2 opened\r\n");
		} else if (ai_eng == AI_ENG_CNN2) {
#if (KDRV_AI_FLG_IMPROVE == 0)
			CNN_OPENOBJ cnn_drv_obj;
			cnn_drv_obj.clk_sel     = kdrv_ai_open_obj.clock_sel;
			cnn_drv_obj.fp_isr_cb   = kdrv_ai_cnn2_isr_cb;
			if (cnn_open(1, &cnn_drv_obj) != E_OK) {
#else
			if (cnn_open(1, NULL) != E_OK) {
#endif
				kdrv_ai_handle_free(ai_eng);
				DBG_WRN("KDRV AI: cnn_open failed\r\n");
				er_code = E_SYS;
				goto open_fail;
			}
			//DBG_WRN("KDRV AI: cnn2 opened\r\n");
		} else {
			DBG_WRN("KDRV AI: unknown engine!!\r\n");
		}
#if (NEW_AI_FLOW == 1)
		}
#endif

		for (ch = 0; ch < KDRV_AI_HANDLE_MAX_NUM; ch++) {
			memset((void *)&g_kdrv_ai_parm_tab[ai_eng][ch], 0, sizeof(KDRV_AI_PRAM));
#if KDRV_AI_FLG_IMPROVE
			SEM_SIGNAL(ai_sem_id[ai_eng][ch]);
#else
			p_handle = (KDRV_AI_HANDLE *)&g_kdrv_ai_handle_tab[ai_eng][ch];
			kdrv_ai_lock(p_handle, FALSE);
#endif
		}
	} else if (g_kdrv_ai_open_times[ai_eng] > KDRV_AI_HANDLE_MAX_NUM) {

		DBG_ERR("open too many times!\r\n");
		g_kdrv_ai_open_times[ai_eng] = KDRV_AI_HANDLE_MAX_NUM;
		er_code = E_SYS;
		goto open_fail;
	}

	g_kdrv_ai_open_times[ai_eng]++;

open_fail:
	if (er_code != E_OK) {
		kdrv_ai_sem(&SEMID_KDRV_AI[ai_eng], FALSE);
	}

	return er_code;
}

INT32 kdrv_ai_close(UINT32 chip, UINT32 engine)
{
#if KDRV_AI_FLG_IMPROVE
#else
	KDRV_AI_HANDLE *p_handle;
#endif
	UINT32 ch = 0;
	ER rt = E_OK;
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(engine);

	if (ai_eng < 0) {
		DBG_ERR("unknown AI engine : %d\r\n", (int)ai_eng);
		return E_ID;
	}

	if (g_kdrv_ai_init_flg[ai_eng] == 0) {
		DBG_ERR("should open kdrv ai first\r\n");
		return E_SYS;
	}
	g_kdrv_ai_open_times[ai_eng]--;

	if (g_kdrv_ai_open_times[ai_eng] > 0) {
	} else if (g_kdrv_ai_open_times[ai_eng] == 0) {
		for (ch = 0; ch < KDRV_AI_HANDLE_MAX_NUM; ch++) {
#if KDRV_AI_FLG_IMPROVE
			SEM_WAIT(ai_sem_id[ai_eng][ch]);
#else
			p_handle = (KDRV_AI_HANDLE *)&g_kdrv_ai_handle_tab[ai_eng][ch];
			kdrv_ai_lock(p_handle, TRUE);
#endif
		}

		if (ai_eng == AI_ENG_CNN) {
			if (kdrv_ai_handle_free(ai_eng) == E_OK) {
				rt = cnn_close(0);
			}
		} else if (ai_eng == AI_ENG_NUE) {
			if (kdrv_ai_handle_free(ai_eng) == E_OK) {
				rt = nue_close();
			}
		} else if (ai_eng == AI_ENG_NUE2) {
			if (kdrv_ai_handle_free(ai_eng) == E_OK) {
				rt = nue2_close();
			}
		} else if (ai_eng == AI_ENG_CNN2) {
			if (kdrv_ai_handle_free(ai_eng) == E_OK) {
				rt = cnn_close(1);
			}
		}

		for (ch = 0; ch < KDRV_AI_HANDLE_MAX_NUM; ch++) {
#if KDRV_AI_FLG_IMPROVE
			SEM_SIGNAL(ai_sem_id[ai_eng][ch]);
#else
			p_handle = (KDRV_AI_HANDLE *)&g_kdrv_ai_handle_tab[ai_eng][ch];
			kdrv_ai_lock(p_handle, FALSE);
#endif
		}
	} else if (g_kdrv_ai_open_times[ai_eng] < 0) {
		DBG_ERR("close too many times!\r\n");
		g_kdrv_ai_open_times[ai_eng] = 0;
		rt = E_PAR;
	}

	kdrv_ai_sem(&SEMID_KDRV_AI[ai_eng], FALSE);

	return rt;
}

#if LL_SUPPORT_ROI
#define TEST_MAX_JOB_NUM   AI_MAX_ROI_NUM
#define TEST_MAX_IO_NUM    AI_MAX_IN_NUM
#define LINK_MAX_LAYER_NUM 20
UINT32 g_kdrv_ai_input_addr[AI_SUPPORT_NET_MAX][LINK_MAX_LAYER_NUM][TEST_MAX_JOB_NUM][TEST_MAX_IO_NUM] = {0};
UINT32 g_kdrv_ai_output_addr[AI_SUPPORT_NET_MAX][LINK_MAX_LAYER_NUM][TEST_MAX_JOB_NUM][TEST_MAX_IO_NUM] = {0};
UINT32 g_ai_mem_info_addr[AI_SUPPORT_NET_MAX] = {0};
UINT32 g_ai_job_num[AI_SUPPORT_NET_MAX] = {0};
UINT32 g_ai_total_layer_num[AI_SUPPORT_NET_MAX] = {0};
UINT32 g_ai_net_link_num[AI_SUPPORT_NET_MAX] = {0};
UINT32 g_ai_ll_input_ofs[AI_SUPPORT_NET_MAX][LINK_MAX_LAYER_NUM][TEST_MAX_IO_NUM] = {0};
UINT32 g_ai_ll_output_ofs[AI_SUPPORT_NET_MAX][LINK_MAX_LAYER_NUM][TEST_MAX_IO_NUM] = {0};
UINT32 g_ai_net_link_single_size[AI_SUPPORT_NET_MAX] = {0};
UINT32 g_ai_net_nextll_pos[AI_SUPPORT_NET_MAX][LINK_MAX_LAYER_NUM] = {0};
UINT32 g_ai_net_end_pos[AI_SUPPORT_NET_MAX] = {0};
UINT64 g_ai_net_restore_cmd[AI_SUPPORT_NET_MAX] = {0};

ER kdrv_ai_set_mem_info(UINT32 addr, UINT32 id)
{
	if (id > AI_SUPPORT_NET_MAX - 1 || addr == 0)
		return 1;
	g_ai_mem_info_addr[id] = addr;

	return 0;
}
/*
static ER kdrv_ai_link_ll_cmd_preproc(UINT32 cmd_addr, UINT32 net_id)
{
	// used only for tmp model
	ER er_code = E_OK;
	UINT64* ll_cmd = (UINT64*)cmd_addr;
	UINT32 cmd_pos = 0;
	UINT32 null_flg = 0;
	UINT32 null_pos = 0;
	if (g_ai_net_link_num[net_id] > 0) {

	} else {
		while(1) {
			if (null_flg == 0 && ll_cmd[cmd_pos] == 0) {
				null_flg = 1;
				null_pos = cmd_pos;
			}
			if (null_flg == 1 && ((ll_cmd[cmd_pos] >> 60) == 8)) {
				ll_cmd[null_pos] = cnn_ll_nextll_cmd(vos_cpu_get_phy_addr((UINT32)(&ll_cmd[cmd_pos])), 1);
				break;
			}
			cmd_pos++;
		}
	}
	return er_code;
}
*/
ER kdrv_ai_link_ll_update_addr(AI_USR_LAYER_INFO* usr_layer_info, UINT32 net_id)
{
	ER er_code = E_OK;
	UINT32 i = 0, j = 0, k = 0;
	//UINT32 addr_size = sizeof(UINT32)*TEST_MAX_LAYER_NUM*TEST_MAX_JOB_NUM*TEST_MAX_IO_NUM;
	//printk("kdrv_ai_link_ll_update_addr 0/ net_id = %d, roi = %d\n", (int)net_id, (int)usr_layer_info->roi_num);
	//memset((VOID*)g_kdrv_ai_input_addr[net_id], 0, addr_size);
	//memset((VOID*)g_kdrv_ai_output_addr[net_id], 0, addr_size);
	//printk("In kdrv_ai_link_ll_update_addr net id = %d, roi_num = %d\n", (int)net_id, (int)usr_layer_info->roi_num);
	for (i = 0; i < usr_layer_info->in_layer_num; i++) {
		for (j = 0; j < usr_layer_info->roi_num; j++) {
			for (k = 0; k < AI_MAX_IN_NUM; k++) {
				g_kdrv_ai_input_addr[net_id][usr_layer_info->in_layer_idx[i]][j][k] = usr_layer_info->in_layer_addr[i][j][k];
			}
		}
	}

	for (i = 0; i < usr_layer_info->out_layer_num; i++) {
		for (j = 0; j < usr_layer_info->roi_num; j++) {
			for (k = 0; k < AI_MAX_OUT_NUM; k++) {
				g_kdrv_ai_output_addr[net_id][usr_layer_info->out_layer_idx[i]][j][k] = usr_layer_info->out_layer_addr[i][j][k];
			}
		}
	}

	g_ai_job_num[net_id] = usr_layer_info->roi_num;
	g_ai_total_layer_num[net_id] = usr_layer_info->total_layer_num;

	return er_code;
}

ER kdrv_ai_link_uninit(UINT32 net_id)
{
	//UINT32 i,j,k;
	g_ai_net_link_num[net_id] = 0;
	g_ai_net_link_single_size[net_id] = 0;
	//memset((VOID*)(kdrv_ai_get_drv_base_va(net_id)), 0, kdrv_ai_get_drv_io_buf_size());
	return E_OK;
}



static UINT32 kdrv_ai_calc_link_job_num(UINT32 id, UINT32 cmd_addr, UINT32 net_id)
{
	UINT32 single_job_cmd_size = 0;
	UINT32 job_num = 0;
	//UINT32 drv_cmd_ofs = 0;
	UINT64* drv_cmd = NULL;
	UINT64* model_cmd = (UINT64*)cmd_addr;
	UINT32 drv_cmd_pos = 0;
	UINT32 cmd_pos = 0;
	UINT32 i = 0;
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
	UINT32 input_reg_ofs[TEST_MAX_IO_NUM] = {0};
	UINT32 output_reg_ofs[TEST_MAX_IO_NUM] = {0};
	UINT32 layer_idx = 0, layer_num = 0;



	if (g_ai_net_link_num[net_id] > 0) {
		job_num = (g_ai_net_link_num[net_id] > g_ai_job_num[net_id])?g_ai_job_num[net_id]:g_ai_net_link_num[net_id];
	} else {
		if (ai_eng == AI_ENG_CNN || ai_eng == AI_ENG_CNN2) {
			input_reg_ofs[0] = 0x10;
			input_reg_ofs[1] = 0x3c;
			output_reg_ofs[0] = 0x44;
			output_reg_ofs[1] = 0x48;
		} else if (ai_eng == AI_ENG_NUE) {
		} else if (ai_eng == AI_ENG_NUE2) {
		}

		//drv_cmd_ofs = 0;
		drv_cmd = (UINT64*)(kdrv_ai_get_drv_base_va(net_id));
		// 1. copy model cmd to drv cmd addr & calculate cmd buffer size
		while (1) {
			drv_cmd[drv_cmd_pos] = model_cmd[cmd_pos];
			if ((model_cmd[cmd_pos] >> 60) == 2) {
				g_ai_net_nextll_pos[net_id][layer_idx++] = cmd_pos;
				drv_cmd[drv_cmd_pos] = cnn_ll_nextll_cmd(vos_cpu_get_phy_addr(((UINT32)drv_cmd) + (drv_cmd_pos+3)*sizeof(UINT64)), 2);
				cmd_pos += 2;
				drv_cmd_pos += 2;
			} else if ((model_cmd[cmd_pos] >> 60) == 0) {
				//printk("tmp model cmd = 0x%08X %08X\n", (unsigned int)(model_cmd[cmd_pos] & 0xFFFFFFFF), (unsigned int)(model_cmd[cmd_pos] >> 32));
				model_cmd[cmd_pos]   = cnn_ll_nextll_cmd(vos_cpu_get_phy_addr((UINT32)drv_cmd), 1);
				//printk("tmp model cmd = 0x%08X %08X\n", (unsigned int)(model_cmd[cmd_pos] & 0xFFFFFFFF), (unsigned int)(model_cmd[cmd_pos] >> 32));
				/*if (g_ai_job_num[net_id] == 2)
					drv_cmd[drv_cmd_pos] = cnn_ll_null_cmd(2);
				else
					drv_cmd[drv_cmd_pos] = cnn_ll_nextll_cmd(vos_cpu_get_phy_addr(((UINT32)drv_cmd) + (drv_cmd_pos+1)*sizeof(UINT64)), 2);*/
				drv_cmd[drv_cmd_pos] = cnn_ll_nextll_cmd(vos_cpu_get_phy_addr(((UINT32)drv_cmd) + (drv_cmd_pos+1)*sizeof(UINT64)), 2);
				drv_cmd_pos++;
				break;
			} else if ((model_cmd[cmd_pos] >> 60) == 8) {
				UINT32 reg_ofs = (model_cmd[cmd_pos] >> 32) & 0xFFF;
				if (reg_ofs == input_reg_ofs[0]) {
					g_ai_ll_input_ofs[net_id][layer_idx][0] = cmd_pos;
				} else if (reg_ofs == input_reg_ofs[1]) {
					g_ai_ll_input_ofs[net_id][layer_idx][1] = cmd_pos;
				} else if (reg_ofs == input_reg_ofs[2]) {
					g_ai_ll_input_ofs[net_id][layer_idx][2] = cmd_pos;
				} else if (reg_ofs == output_reg_ofs[0]) {
					g_ai_ll_output_ofs[net_id][layer_idx][0] = cmd_pos;
				} else if (reg_ofs == output_reg_ofs[1]) {
					g_ai_ll_output_ofs[net_id][layer_idx][1] = cmd_pos;
				} else if (reg_ofs == output_reg_ofs[2]) {
					g_ai_ll_output_ofs[net_id][layer_idx][2] = cmd_pos;
				}
			}
			cmd_pos++;
			drv_cmd_pos++;
		}
		single_job_cmd_size = drv_cmd_pos*sizeof(UINT64);

		if (single_job_cmd_size > kdrv_ai_get_drv_io_buf_size(net_id)) {
			DBG_IND("single cmd usage 0x%08X is larger than limit!\r\n", (unsigned int)single_job_cmd_size);
			return 0;
		}
		g_ai_net_link_single_size[net_id] = single_job_cmd_size;
		job_num = kdrv_ai_get_drv_io_buf_size(net_id) / single_job_cmd_size;
		job_num = (job_num > TEST_MAX_JOB_NUM)?TEST_MAX_JOB_NUM:job_num;
		g_ai_net_link_num[net_id] = job_num;
		//job_num = (job_num > g_ai_job_num[net_id])?g_ai_job_num[net_id]:job_num;
		layer_num = layer_idx;
		// 2. duplicate cmd & link all duplicated nets
		for (i = 1; i < job_num - 1; i++) {
			UINT64* p_tmp = (UINT64*)((UINT32)drv_cmd + single_job_cmd_size*(i+1) - sizeof(UINT64));
			memcpy((VOID*)((UINT32)drv_cmd + single_job_cmd_size*i), drv_cmd, single_job_cmd_size);
			if (i == job_num - 2) {
				// set null
				p_tmp[0] = cnn_ll_null_cmd((i+2) & 0xFF);
			} else {
				// set nextll
				p_tmp[0] = cnn_ll_nextll_cmd(vos_cpu_get_phy_addr(((UINT32)drv_cmd) + single_job_cmd_size*(i+1)), (i+2) & 0xFF);
			}
			for (layer_idx = 0; layer_idx < layer_num; layer_idx++) {
				p_tmp = (UINT64*)((UINT32)drv_cmd + single_job_cmd_size*i + g_ai_net_nextll_pos[net_id][layer_idx]*sizeof(UINT64));
				p_tmp[0] = cnn_ll_nextll_cmd(vos_cpu_get_phy_addr(((UINT32)p_tmp) + 3*sizeof(UINT64)), layer_idx & 0xFF);
			}

		}

		job_num = (job_num > g_ai_job_num[net_id])?g_ai_job_num[net_id]:job_num;
	}
	return job_num;
}

static ER kdrv_ai_link_ll_cmd(UINT32 id, UINT32 cmd_addr, UINT32 job_start_idx, UINT32 job_end_idx, UINT32 layer_num, UINT32 net_id)
{
	ER er_code = E_OK;
	UINT32 i = 0, j = 0, k = 0;
	UINT64* drv_cmd = NULL;
	UINT32 reg_ofs = 0;

	for (i = job_start_idx; i <= job_end_idx; i++) {
		if (i == job_start_idx) {
			drv_cmd = (UINT64*)cmd_addr;
		} else {
			drv_cmd = (UINT64*)(kdrv_ai_get_drv_base_va(net_id) + (i-job_start_idx-1)*g_ai_net_link_single_size[net_id]);
		}
		for (j = 0; j < layer_num; j++) {
			for (k = 0; k < TEST_MAX_IO_NUM; k++) {
				if (g_kdrv_ai_input_addr[net_id][j][i][k] > 0) {
					reg_ofs = (drv_cmd[g_ai_ll_input_ofs[net_id][j][k]] >> 32) & 0xFFF;
					drv_cmd[g_ai_ll_input_ofs[net_id][j][k]]  = cnn_ll_upd_cmd(0xF, reg_ofs, g_kdrv_ai_input_addr[net_id][j][i][k]);
				}
				if (g_kdrv_ai_output_addr[net_id][j][i][k] > 0) {
					reg_ofs = (drv_cmd[g_ai_ll_output_ofs[net_id][j][k]] >> 32) & 0xFFF;
					drv_cmd[g_ai_ll_output_ofs[net_id][j][k]] = cnn_ll_upd_cmd(0xF, reg_ofs, g_kdrv_ai_output_addr[net_id][j][i][k]);
				}
			}
		}

		if (i == job_end_idx && job_end_idx - job_start_idx + 1 < g_ai_net_link_num[net_id]) {
			UINT32 tmp_end_pos = (g_ai_net_link_single_size[net_id]/sizeof(UINT64)) - 1;

			g_ai_net_restore_cmd[net_id] = drv_cmd[tmp_end_pos];
			drv_cmd[tmp_end_pos] = cnn_ll_null_cmd(0);
			g_ai_net_end_pos[net_id] = (UINT32)(&drv_cmd[tmp_end_pos]);
		}

		if (i == job_start_idx) {
			//fmem_dcache_sync((VOID *)drv_cmd, g_ai_net_link_single_size[net_id], DMA_BIDIRECTIONAL);
			vos_cpu_dcache_sync((UINT32)drv_cmd, g_ai_net_link_single_size[net_id], VOS_DMA_TO_DEVICE);
		}
	}

	return er_code;
}

static ER kdrv_ai_link_restore_cmd(UINT32 net_id)
{
	if (g_ai_net_end_pos[net_id] > 0) {
		UINT64* tmp_pos = (UINT64*)(g_ai_net_end_pos[net_id]);
		tmp_pos[0] = g_ai_net_restore_cmd[net_id];
		g_ai_net_end_pos[net_id] = 0;
		g_ai_net_restore_cmd[net_id] = 0;
	}

	return E_OK;
}
#endif

INT32 kdrv_ai_trigger(UINT32 id, KDRV_AI_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data)
{
	ER rt = E_OK;
	KDRV_AI_HANDLE *p_handle;
	KDRV_AI_PRAM *p_ai_info;
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
	KDRV_AI_TRIG_MODE trig_mode;
	BOOL nonblock_en/*, time_out_en*/;
#if LL_SUPPORT_ROI
	UINT32 net_id = 0;
#endif

	p_handle = kdrv_ai_id_conv2_handle(id);
	p_ai_info = kdrv_ai_id_conv2_parm(id);
	if ((p_handle == NULL) || (p_ai_info == NULL)) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}

	trig_mode = p_ai_info->mode;
	nonblock_en = ((p_param != NULL) && (p_param->is_nonblock) && (trig_mode == AI_TRIG_MODE_LL)) ? TRUE : FALSE;
	//time_out_en = ((p_param != NULL) && (p_param->time_out_ms != 0)) ? TRUE : FALSE;

	g_ai_isr_trig = p_param->is_isr_trigger;
	//trigger ai start
	//kdrv_ai_frm_end(p_handle, FALSE);
#if (KDRV_AI_FLG_IMPROVE == 0)
	if (ai_eng == AI_ENG_CNN) {
		kdrv_ai_cnn_frm_end(p_handle, FALSE);
	} else if (ai_eng == AI_ENG_NUE) {
		kdrv_ai_nue_frm_end(p_handle, FALSE);
	} else if (ai_eng == AI_ENG_NUE2) {
		kdrv_ai_nue2_frm_end(p_handle, FALSE);
	} else if (ai_eng == AI_ENG_CNN2) {
		kdrv_ai_cnn2_frm_end(p_handle, FALSE);
	}

	kdrv_ai_clear_timeout_flag(p_handle);
#endif
	//update trig id and trig_cfg
	g_kdrv_ai_trig_hdl[ai_eng] = p_handle;
	g_kdrv_ai_trig_parm[ai_eng] = p_ai_info;
#if (KDRV_AI_FLG_IMPROVE == 0)
	// clear reset flag
	kdrv_ai_reset_eng(p_handle, FALSE);

	if (time_out_en) {
		kdrv_ai_time_start(id, p_param->time_out_ms);
	}
#endif

	//trigger ai start

	if (trig_mode == AI_TRIG_MODE_APP) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
		DBG_ERR("AI_FASTBOOT: Error, app_mode can't be called at fastboot.\r\n");
#else
		rt = kdrv_ai_app_trigger(id);
#endif
	} else if (trig_mode == AI_TRIG_MODE_LL) {
#if (NEW_AI_FLOW == 1)
		if (g_kdrv_ai_flow == 1) {
		    rt = kdrv_ai_ll_trigger(id);
		} else {
#endif
#if LL_SUPPORT_ROI
		AI_DRV_TRIGINFO* p_trig_info = (AI_DRV_TRIGINFO*)p_user_data;
		UINT32 link_job_num = 0;
		UINT32 link_idx = 0;
		UINT32 link_ofs = 0;
		UINT32 link_time = 0;

		net_id = p_trig_info->net_id;

		if (g_ai_job_num[net_id] > 0) {
			KDRV_AI_LL_HEAD *p_head;
			PROF_START();
			p_head = &p_ai_info->ll_parm;
			//kdrv_ai_link_ll_cmd_preproc(p_head->parm_addr, net_id);
			link_job_num = kdrv_ai_calc_link_job_num(id, p_head->parm_addr, net_id);

			link_time = (g_ai_job_num[net_id] / link_job_num) + (((g_ai_job_num[net_id] % link_job_num) > 0)? 1 : 0);
			for (link_idx = 0; link_idx < link_time; link_idx++) {
				if (link_idx == link_time - 1) {
					kdrv_ai_link_ll_cmd(id, p_head->parm_addr, link_ofs, g_ai_job_num[net_id] - 1, g_ai_total_layer_num[net_id], net_id);
				} else {
					kdrv_ai_link_ll_cmd(id, p_head->parm_addr, link_ofs, link_ofs + link_job_num - 1, g_ai_total_layer_num[net_id], net_id);
					link_ofs += link_job_num;
				}
				PROF_END("update_ll");
				rt = kdrv_ai_ll_trigger(id);
				if (link_idx < link_time - 1) {
					rt = kdrv_ai_ll_done(id);
				}
			}
		} else {
#endif
		rt = kdrv_ai_ll_trigger(id);
#if LL_SUPPORT_ROI
		}
#endif
#if (NEW_AI_FLOW == 1)
		}
#endif

	} else if (trig_mode == AI_TRIG_MODE_FC) {
		KDRV_AI_FC_PARM* p_input_parm;
		NUE_PARM nue_parm = {0};
		UINT32 run_idx = 0;
		UINT32 run_time = 0;
		UINT32 tmp_h = 0;
		KDRV_AI_APP_HEAD *p_cur;
		p_cur = &p_ai_info->app_parm;
		
		p_input_parm = (KDRV_AI_FC_PARM *)p_cur->parm_addr;
		/*if (copy_from_user(&input_parm, (void __user *)argc, sizeof(KDRV_AI_FC_PARM))) {
			err = -EFAULT;
			goto exit;
		}*/
		
		// init parameter
		p_input_parm->func_list[0] = KDRV_AI_FC_FULLY_EN;
		p_input_parm->func_list[1] = 0;
		p_input_parm->src_fmt  = AI_FC_SRC_FEAT;
		p_input_parm->out_type = AI_IO_INT8;
		p_input_parm->out_addr = 0;
		p_input_parm->size.width   = 1;
		p_input_parm->size.height  = 1;
		p_input_parm->size.channel = 256;
		p_input_parm->batch_num = 1;
		p_input_parm->in_interm_addr    = 0;
		p_input_parm->in_interm_dma_en  = 0;
		p_input_parm->out_interm_dma_en = 3;
		p_input_parm->in_interm_ofs.line_ofs = 1;              
		p_input_parm->in_interm_ofs.channel_ofs = 1;      
		p_input_parm->in_interm_ofs.batch_ofs = 256;
		p_input_parm->fc_ker.weight_w = 256;
		p_input_parm->fc_ker.sclshf.in_shift  = 0;
		p_input_parm->fc_ker.sclshf.out_shift = 0;
		p_input_parm->fc_ker.sclshf.in_scale  = 1;
		p_input_parm->fc_ker.sclshf.out_scale = 1;
		p_input_parm->fc_ker.bias_addr = 0;
		p_input_parm->act.mode = AI_ACT_RELU;
		p_input_parm->act.relu.leaky_val = 0;
		p_input_parm->act.relu.leaky_shf = 0;
		p_input_parm->act.neg_en = 0;
		p_input_parm->act.act_shf0 = 0;
		p_input_parm->act.act_shf1 = 0;
		p_input_parm->act.sclshf.in_shift  = 0;
		p_input_parm->act.sclshf.out_shift = 0;
		p_input_parm->act.sclshf.in_scale  = 1;
		p_input_parm->act.sclshf.out_scale = 1;
		p_input_parm->fcd.func_en = KDRV_AI_FCD_QUANTI_EN;
		p_input_parm->fcd.quanti_kmeans_addr = 0;
		p_input_parm->fcd.enc_bit_length = 0;
		p_input_parm->fcd.vlc_code_size = 0;
		p_input_parm->fcd.vlc_valid_size = 0;
		p_input_parm->fcd.vlc_ofs_size = 0;
		kdrv_ai_tran_nue_fc_parm(p_input_parm, &nue_parm, KDRV_AI_FC_FULLY_EN);
		
		nue_parm.dmaio_addr.drv_dma_not_sync[0] = 1;
		nue_parm.dmaio_addr.drv_dma_not_sync[2] = 1;
		nue_parm.dmaio_addr.drv_dma_not_sync[3] = 1;
		nue_parm.dmaio_addr.drv_dma_not_sync[4] = 1;
		nue_parm.dmaio_addr.drv_dma_not_sync[6] = 1;
		
		
		run_time = (nue_parm.insvm_size.sv_h / 256) + (((nue_parm.insvm_size.sv_h % 256) > 0)?1:0);
		tmp_h    = p_input_parm->fc_ker.weight_h;
		//nue_open(NULL);
		for (run_idx = 0; run_idx < run_time; run_idx++) {
			nue_parm.insvm_size.sv_h = (tmp_h < 256)?tmp_h:256;
			nue_parm.dmaio_addr.addr_out  = p_input_parm->out_interm_addr + run_idx*4*256;
			nue_parm.dmaio_addr.addrin_sv = p_input_parm->fc_ker.weight_addr + run_idx*nue_parm.insvm_size.sv_w*256;
			nue_parm.svm_parm.fcd_parm.fcd_enc_bit_length = 256*nue_parm.insvm_size.sv_h*8;
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
			DBG_ERR("AI_FASTBOOT: Error, trig_mode == AI_TRIG_MODE_FC can't be called at fastboot.\r\n");
#else
			if (nue_setmode(NUE_OPMODE_USERDEFINE, &nue_parm) == E_OK) {
				nue_start();
				nue_wait_frameend(FALSE);
			}
			nue_pause();
#endif
			tmp_h -= nue_parm.insvm_size.sv_h;
		}
		//nue_close();
	} else if (trig_mode == AI_TRIG_MODE_PREC) {
		KDRV_AI_PREPROC_PARM* g_nue2_input_parm;
		NUE2_PARM g_nue2_parm = {0};
		UINT32 func_en = 0;
		UINT32 func_idx = 0;
		KDRV_AI_APP_HEAD *p_cur;
		p_cur = &p_ai_info->app_parm;	

		g_nue2_input_parm = (KDRV_AI_PREPROC_PARM *)p_cur->parm_addr; 
		/*
		if (copy_from_user(&g_nue2_input_parm, (void __user *)argc, sizeof(KDRV_AI_PREPROC_PARM))) {
			err = -EFAULT;
			goto exit;
		}*/

		for (func_idx = 0; func_idx < KDRV_AI_PREPROC_FUNC_CNT; func_idx++) {
			if (g_nue2_input_parm->func_list[func_idx] == 0) {
				break;
			}
			func_en |= g_nue2_input_parm->func_list[func_idx];
		}
		
		kdrv_ai_tran_nue2_preproc_parm(g_nue2_input_parm, &g_nue2_parm, func_en);
		g_nue2_parm.dmaio_addr.dma_do_not_sync = 1;
		g_nue2_parm.dmaio_addr.is_pa = 1;
		/*
		er_return = nue2_open(NULL);
		if (er_return != E_OK) {
			err = -EFAULT;
			nvt_dbg(ERR, "AI_DRV: Error, nue2_open failed.\r\n");
		}
		*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
		nvt_dbg(ERR, "AI_FASTBOOT: Error, NUE2_IOC_RUN can't be called at fastboot.\n");
#else
		if (nue2_setmode(NUE2_OPMODE_USERDEFINE, &g_nue2_parm) == E_OK) {
			nue2_start();
			nue2_wait_frameend(FALSE);
		}
#endif
		nue2_pause();
		
		//nue2_close();
	}

	if (rt != E_OK) {
		DBG_ERR("k-driver ai trigger fail...\r\n");
		return rt;
	}

	//due to blocking process
	if (!nonblock_en) {
		if (trig_mode == AI_TRIG_MODE_APP) {
			//app trigger is blocking process
		} else if (trig_mode == AI_TRIG_MODE_LL) {
#if (NEW_AI_FLOW == 1)
			if (g_kdrv_ai_flow == 1) {
				rt = kdrv_ai_ll_done(id);
			} else {
#endif
			rt = kdrv_ai_ll_done(id);
#if LL_SUPPORT_ROI
			kdrv_ai_link_restore_cmd(net_id);
#endif
#if (NEW_AI_FLOW == 1)
			}
#endif
		}
#if (KDRV_AI_FLG_IMPROVE == 0)
		if (time_out_en) {
			if (kdrv_ai_time_stop(id) != E_OK) {
				DBG_WRN("timer close fail, eng=%d\r\n", ai_eng);
			}
		}
#endif
	}

	return rt;
}

INT32 kdrv_ai_waitdone(UINT32 id, KDRV_AI_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data)
{
	ER rt = E_OK;
	KDRV_AI_HANDLE *p_handle;
	KDRV_AI_PRAM *p_ai_info;
#if (KDRV_AI_FLG_IMPROVE == 0)
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
#endif
	KDRV_AI_TRIG_MODE trig_mode;
	BOOL nonblock_en/*, time_out_en*/;

	p_handle = kdrv_ai_id_conv2_handle(id);
	p_ai_info = kdrv_ai_id_conv2_parm(id);
	if ((p_handle == NULL) || (p_ai_info == NULL)) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}

	trig_mode = p_ai_info->mode;
	nonblock_en = ((p_param != NULL) && (p_param->is_nonblock) && (trig_mode == AI_TRIG_MODE_LL)) ? TRUE : FALSE;
	//time_out_en = ((p_param != NULL) && (p_param->time_out_ms != 0)) ? TRUE : FALSE;

	//due to non-blocking process
	if (!nonblock_en) {
		DBG_WRN("not support blocking process\r\n");
	} else {
		if (trig_mode == AI_TRIG_MODE_APP) {
			//app trigger is blocking process
		} else if (trig_mode == AI_TRIG_MODE_LL) {
			rt = kdrv_ai_ll_done(id);
		}
#if (KDRV_AI_FLG_IMPROVE == 0)
		if ((time_out_en) && (kdrv_ai_time_stop(id) != E_OK)) {
			DBG_WRN("timer close fail, eng = %d\r\n", ai_eng);

		}
#endif
	}
	return rt;
}

INT32 kdrv_ai_reset(UINT32 id, KDRV_AI_TRIGGER_PARAM *p_param)
{
	ER rt = E_OK;
	KDRV_AI_HANDLE *p_handle;
	KDRV_AI_PRAM *p_ai_info;
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
	KDRV_AI_TRIG_MODE trig_mode;

	p_handle = kdrv_ai_id_conv2_handle(id);
	p_ai_info = kdrv_ai_id_conv2_parm(id);
	if ((p_handle == NULL) || (p_ai_info == NULL)) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}

	trig_mode = p_ai_info->mode;
	if (ai_eng == AI_ENG_CNN) {
		cnn_dma_abort(0);
		cnn_reset(0);
		if (trig_mode == AI_TRIG_MODE_APP) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
			DBG_ERR("AI_FASTBOOT: Error, don't support app mode at kdrv_ai_reset(1).\r\n");		
#else
			rt = cnn_pause(0);
#endif
		} else if (trig_mode == AI_TRIG_MODE_LL) {
			rt = cnn_ll_pause(0);
		}
	} else if (ai_eng == AI_ENG_NUE) {
		nue_dma_abort();
		nue_reset();
		if (trig_mode == AI_TRIG_MODE_APP) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
			DBG_ERR("AI_FASTBOOT: Error, don't support app mode at kdrv_ai_reset(2).\r\n");	
#else
			rt = nue_pause();
#endif
		} else if (trig_mode == AI_TRIG_MODE_LL) {
			rt = nue_ll_pause();
		}
	} else if (ai_eng == AI_ENG_NUE2) {
		nue2_dma_abort();
		nue2_reset();
		if (trig_mode == AI_TRIG_MODE_APP) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
			DBG_ERR("AI_FASTBOOT: Error, don't support app mode at kdrv_ai_reset(3).\r\n");
#else
			rt = nue2_pause();
#endif
		} else if (trig_mode == AI_TRIG_MODE_LL) {
			rt = nue2_ll_pause();
		}
	} else if (ai_eng == AI_ENG_CNN2) {
		cnn_dma_abort(1);
		cnn_reset(1);
		if (trig_mode == AI_TRIG_MODE_APP) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
			DBG_ERR("AI_FASTBOOT: Error, don't support app mode at kdrv_ai_reset(4).\r\n");
#else
			rt = cnn_pause(1);
#endif
		} else if (trig_mode == AI_TRIG_MODE_LL) {
			rt = cnn_ll_pause(1);
		}
	}
#if (KDRV_AI_FLG_IMPROVE == 0)
	kdrv_ai_reset_eng(p_handle, TRUE);
#endif

	return rt;
}

INT32 kdrv_ai_engine_reset(KDRV_AI_ENG ai_eng)
{
	ER rt = E_OK;

	if (ai_eng == AI_ENG_CNN) {
		cnn_dma_abort(0);
		cnn_reset(0);
		rt = cnn_ll_pause(0);	
	} else if (ai_eng == AI_ENG_NUE) {
		nue_dma_abort();
		nue_reset();
		rt = nue_ll_pause();
	} else if (ai_eng == AI_ENG_NUE2) {
		nue2_dma_abort();
		nue2_reset();
		rt = nue2_ll_pause();
	} else if (ai_eng == AI_ENG_CNN2) {
		cnn_dma_abort(1);
		cnn_reset(1);
		rt = cnn_ll_pause(1);
	}

	return rt;
}

static ER kdrv_ai_set_opencfg(UINT32 id, void *data)
{
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
	KDRV_AI_OPENCFG *open_param;
	if (ai_eng < 0) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	open_param = (KDRV_AI_OPENCFG *)data;
	g_kdrv_ai_open_cfg[ai_eng].clock_sel = open_param->clock_sel;

	return E_OK;
}

static ER kdrv_ai_set_modeinfo(UINT32 id, void *data)
{
	KDRV_AI_TRIG_MODE *p_mode_info;
	KDRV_AI_PRAM *p_ai_info;

	p_ai_info = kdrv_ai_id_conv2_parm(id);
	if (p_ai_info == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	p_mode_info = (KDRV_AI_TRIG_MODE *)data;
	p_ai_info->mode = *p_mode_info;

	return E_OK;

}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
static ER kdrv_ai_set_app_info(UINT32 id, void *data)
{
	ER rt = E_OK;

	DBG_ERR("AI_FASTBOOT: Error, kdrv_ai_set_app_info can't be called at fastboot.\r\n");

	return rt;
}
#else
static ER kdrv_ai_set_app_info(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_AI_APP_INFO *p_info;

	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	p_info = (KDRV_AI_APP_INFO *)data;
	rt = kdrv_ai_app_set_parm(id, p_info);

	return rt;
}
#endif

static ER kdrv_ai_set_ll_info(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_AI_LL_INFO *p_info;

	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	p_info = (KDRV_AI_LL_INFO *)data;
	rt = kdrv_ai_ll_set_parm(id, p_info);

	return rt;
}

static ER kdrv_ai_set_isr_cb(UINT32 id, void *data)
{
	KDRV_AI_HANDLE *p_handle = kdrv_ai_id_conv2_handle(id);

	if (p_handle == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	p_handle->isrcb_fp = (KDRV_AI_ISRCB)data;

	return E_OK;
}

KDRV_AI_SET_FP kdrv_ai_set_fp[KDRV_AI_PARAM_MAX] = {
	kdrv_ai_set_opencfg,

	kdrv_ai_set_modeinfo,
	kdrv_ai_set_app_info,
	kdrv_ai_set_ll_info,
	kdrv_ai_set_isr_cb,
};

INT32 kdrv_ai_set(UINT32 id, KDRV_AI_PARAM_ID param_id, void *p_param)
{
	ER rt = E_OK;
	KDRV_AI_HANDLE *p_handle;
	UINT32 ign_chk;
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
	UINT32 is_init = 0;
	INT32 open_times = 0;

	if (p_param == NULL) {
		DBG_ERR("null pointer...\r\n");
		return E_NOEXS;
	}
	p_handle = kdrv_ai_id_conv2_handle(id);
	if (kdrv_ai_chk_handle(p_handle) == NULL) {
		DBG_ERR("KDRV AI: illegal handle; id = %d\r\n", (int)id);
		return E_SYS;
	}

	if (ai_eng < 0) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}

	is_init 	= g_kdrv_ai_init_flg[ai_eng];
	open_times 	= g_kdrv_ai_open_times[ai_eng];

	if (is_init == 0) {
		if (kdrv_ai_handle_alloc(ai_eng) != E_OK) {
			DBG_WRN("KDRV AI: no free handle, max handle num = %d\r\n", KDRV_AI_HANDLE_MAX_NUM);
			return E_SYS;
		}
	}

	ign_chk = (KDRV_AI_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_AI_IGN_CHK);

#if (NEW_AI_FLOW == 1)
	if (g_kdrv_ai_flow == 1) {

	if ((open_times == 0) && (param_id != KDRV_AI_PARAM_OPENCFG) && (param_id != KDRV_AI_PARAM_ISR_CB)) {
		DBG_ERR("KDRV AI: engine(%d) is not opened. Only OPENCFG / ISR_CB can be set. ID = %d\r\n", (int)ai_eng, (int)param_id);
		return E_SYS;
	} else if ((open_times > 0) && ((param_id == KDRV_AI_PARAM_OPENCFG) || (param_id == KDRV_AI_PARAM_ISR_CB))) {
		DBG_ERR("KDRV AI: engine(%d) is opened. OPENCFG / ISR_CB cannot be set.\r\n", (int)ai_eng);
		return E_SYS;
	}
	} else {
#endif
	if ((open_times == 0) && (param_id != KDRV_AI_PARAM_OPENCFG)) {
		DBG_ERR("KDRV AI: engine(%d) is not opened. Only OPENCFG can be set. ID = %d\r\n", (int)ai_eng, (int)param_id);
		return E_SYS;
	} else if ((open_times > 0) && (param_id == KDRV_AI_PARAM_OPENCFG)) {
		DBG_ERR("KDRV AI: engine(%d) is opened. OPENCFG cannot be set.\r\n", (int)ai_eng);
		return E_SYS;
	}
#if (NEW_AI_FLOW == 1)
	}
#endif

	if ((p_handle->sts & KDRV_AI_HANDLE_LOCK) != KDRV_AI_HANDLE_LOCK) {
		DBG_ERR("KDRV AI: illegal handle sts 0x%.8x, param id=%d\r\n", (int)p_handle->sts, (int)param_id);
		return E_SYS;
	}

	if (!ign_chk) {
#if KDRV_AI_FLG_IMPROVE
		//printk("lock ai_sem_param_id%d\n", ai_eng);
		SEM_WAIT(ai_sem_param_id[ai_eng]);
		//printk("lock ai_sem_param_id%d done\n", ai_eng);
#else
		kdrv_ai_lock(p_handle, TRUE);
#endif
	}
	if (kdrv_ai_set_fp[param_id] != NULL) {
		rt = kdrv_ai_set_fp[param_id](id, p_param);
	}
	if (!ign_chk) {
#if KDRV_AI_FLG_IMPROVE

		SEM_SIGNAL(ai_sem_param_id[ai_eng]);
#else
		kdrv_ai_lock(p_handle, FALSE);
#endif
	}

	return rt;
}

static ER kdrv_ai_get_opencfg(UINT32 id, void *data)
{
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));

	if (ai_eng < 0) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	*(KDRV_AI_OPENCFG *)data = g_kdrv_ai_open_cfg[ai_eng];

	return E_OK;
}

static ER kdrv_ai_get_modeinfo(UINT32 id, void *data)
{
	KDRV_AI_PRAM *p_ai_info = kdrv_ai_id_conv2_parm(id);

	if (p_ai_info == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	*(KDRV_AI_TRIG_MODE *)data = p_ai_info->mode;

	return E_OK;
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
static ER kdrv_ai_get_app_info(UINT32 id, void *data)
{
	ER er_code = E_OK;

	DBG_ERR("AI_FASTBOOT: Error, kdrv_ai_get_app_info can't be called at fastboot.\r\n");

	return er_code;
}
#else
static ER kdrv_ai_get_app_info(UINT32 id, void *data)
{
	ER er_code = E_OK;
	KDRV_AI_APP_INFO *p_info;


	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	p_info = (KDRV_AI_APP_INFO *)data;
	er_code = kdrv_ai_app_get_parm(id, p_info);

	return er_code;
}
#endif

static ER kdrv_ai_get_ll_info(UINT32 id, void *data)
{
	ER er_code = E_OK;
	KDRV_AI_LL_INFO *p_info;

	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_PAR;
	}

	p_info = (KDRV_AI_LL_INFO *)data;
	er_code = kdrv_ai_ll_get_parm(id, p_info);

	return er_code;
}

static ER kdrv_ai_get_isr_cb(UINT32 id, void *data)
{
	KDRV_AI_HANDLE *p_handle = kdrv_ai_id_conv2_handle(id);

	if (p_handle == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (data == NULL) {
		DBG_ERR("id = %d, data param error 0x%.8x\r\n", (int)id, (int)data);
		return E_NOEXS;
	}

	*(KDRV_AI_ISRCB *)data = p_handle->isrcb_fp;

	return E_OK;
}

KDRV_AI_GET_FP kdrv_ai_get_fp[KDRV_AI_PARAM_MAX] = {
	kdrv_ai_get_opencfg,

	kdrv_ai_get_modeinfo,
	kdrv_ai_get_app_info,
	kdrv_ai_get_ll_info,
	kdrv_ai_get_isr_cb,
};

INT32 kdrv_ai_get(UINT32 id, KDRV_AI_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;
	KDRV_AI_HANDLE *p_handle;
	UINT32 ign_chk;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	if (p_param == NULL) {
		DBG_ERR("null pointer...\r\n");
		return E_NOEXS;
	}

	p_handle = kdrv_ai_id_conv2_handle(id);
	if (kdrv_ai_chk_handle(p_handle) == NULL) {
		DBG_ERR("KDRV AI: illegal handle; id = %d\r\n", (int)id);
		return E_SYS;
	}

	if ((p_handle->sts & KDRV_AI_HANDLE_LOCK) != KDRV_AI_HANDLE_LOCK) {
		DBG_ERR("KDRV AI: illegal handle sts 0x%.8x\r\n", (int)p_handle->sts);
		return E_SYS;
	}

	ign_chk = (KDRV_AI_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_AI_IGN_CHK);

	if (!ign_chk) {
#if KDRV_AI_FLG_IMPROVE
		SEM_WAIT(ai_sem_ch_id[channel]);
#else
		kdrv_ai_lock(p_handle, TRUE);
#endif
	}
	if (kdrv_ai_get_fp[parm_id] != NULL) {
		rt = kdrv_ai_get_fp[parm_id](channel, p_param);
	}
	if (!ign_chk) {
#if KDRV_AI_FLG_IMPROVE
		SEM_SIGNAL(ai_sem_ch_id[channel]);
#else
		kdrv_ai_lock(p_handle, FALSE);
#endif
	}

	return rt;
}


#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER kdrv_ai_app_set_parm(UINT32 id, KDRV_AI_APP_INFO *p_info)
{
	KDRV_AI_APP_HEAD *p_src_head, *p_dst_head;
	UINT32 *p_src_head_cnt, *p_dst_head_cnt;
	ER er_code = E_OK;
	KDRV_AI_PRAM *p_ai_info = kdrv_ai_id_conv2_parm(id);
	if (p_ai_info == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (p_info == NULL) {
		nvt_dbg(ERR, "input is NULL pointer\r\n");
		return E_OBJ;
	}

	p_dst_head = &p_ai_info->app_parm;
	p_dst_head_cnt = &p_ai_info->app_parm_cnt;
	p_src_head = p_info->p_head;
	p_src_head_cnt = &p_info->head_cnt;
	*p_dst_head_cnt = *p_src_head_cnt;

	//flush
	//memset(p_dst_head, 0, sizeof(KDRV_AI_APP_HEAD));

	//copy data to local
	memcpy(p_dst_head, p_src_head, sizeof(KDRV_AI_APP_HEAD));

	return er_code;
}

ER kdrv_ai_app_get_parm(UINT32 id, KDRV_AI_APP_INFO *p_info)
{
	KDRV_AI_APP_HEAD *p_src_head, *p_dst_head;
	UINT32 *p_src_head_cnt, *p_dst_head_cnt;
	KDRV_AI_PRAM *p_ai_info = kdrv_ai_id_conv2_parm(id);

	if (p_ai_info == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (p_info == NULL) {
		nvt_dbg(ERR, "output is NULL pointer\r\n");
		return E_OBJ;
	}

	p_src_head = &p_ai_info->app_parm;
	p_src_head_cnt = &p_ai_info->app_parm_cnt;
	p_dst_head = p_info->p_head;
	p_dst_head_cnt = &p_info->head_cnt;

	(*p_dst_head_cnt) = (*p_src_head_cnt);
	memcpy(p_dst_head, p_src_head, sizeof(KDRV_AI_APP_HEAD));

	return E_OK;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

ER kdrv_ai_ll_set_parm(UINT32 id, KDRV_AI_LL_INFO *p_info)
{
	KDRV_AI_LL_HEAD *p_src_head, *p_dst_head;
	UINT32 *p_src_head_cnt, *p_dst_head_cnt;
	ER er_code = E_OK;
	KDRV_AI_PRAM *p_ai_info = kdrv_ai_id_conv2_parm(id);

	if (p_ai_info == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (p_info == NULL) {
		nvt_dbg(ERR, "input is NULL pointer\r\n");
		return E_OBJ;
	}

	p_dst_head = &p_ai_info->ll_parm;
	p_dst_head_cnt = &p_ai_info->ll_parm_cnt;
	p_src_head = p_info->p_head;
	p_src_head_cnt = &p_info->head_cnt;
	*p_dst_head_cnt = *p_src_head_cnt;

	//flush
	//memset(p_dst_head, 0, sizeof(KDRV_AI_LL_HEAD));

	//copy data to local
	memcpy(p_dst_head, p_src_head, sizeof(KDRV_AI_LL_HEAD));

	return er_code;
}

ER kdrv_ai_ll_get_parm(UINT32 id, KDRV_AI_LL_INFO *p_info)
{
	KDRV_AI_LL_HEAD *p_src_head, *p_dst_head;
	UINT32 *p_src_head_cnt, *p_dst_head_cnt;
	KDRV_AI_PRAM *p_ai_info = kdrv_ai_id_conv2_parm(id);

	if (p_ai_info == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}
	if (p_info == NULL) {
		nvt_dbg(ERR, "output is NULL pointer\r\n");
		return E_OBJ;
	}

	p_src_head = &p_ai_info->ll_parm;
	p_src_head_cnt = &p_ai_info->ll_parm_cnt;
	p_dst_head = p_info->p_head;
	p_dst_head_cnt = &p_info->head_cnt;

	(*p_dst_head_cnt) = (*p_src_head_cnt);
	memcpy(p_dst_head, p_src_head, sizeof(KDRV_AI_LL_HEAD));

	return E_OK;
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER kdrv_ai_app_trigger(UINT32 id)
{
	KDRV_AI_APP_HEAD *p_first, *p_cur;
	UINT32 head_cnt = 0;
	UINT32 idx = 0;
	ER er_code = E_OK;
	KDRV_AI_HANDLE *p_handle = kdrv_ai_id_conv2_handle(id);
	KDRV_AI_PRAM *p_ai_info = kdrv_ai_id_conv2_parm(id);
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));

	if ((p_handle == NULL) || (p_ai_info == NULL)) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}

	PROF_START();

	p_first = &p_ai_info->app_parm;
	p_cur = p_first;
	head_cnt = p_ai_info->app_parm_cnt;
	for (idx = 0; idx < head_cnt; idx++) {
		if (p_cur->mode != p_first->mode) {
			DBG_WRN("stripe mode(%d) is different with others(%d)\r\n", p_cur->mode, p_first->mode);
		}

		switch (p_cur->mode) {
		BOOL cnn_id;
		case AI_MODE_NEURAL:
			if (ai_eng == AI_ENG_CNN) {
				cnn_id = 0;
			} else {
				cnn_id = 1;
			}
			er_code = kdrv_ai_neural_trig(cnn_id, (KDRV_AI_NEURAL_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_neural_done(cnn_id, (KDRV_AI_NEURAL_PARM *)p_cur->parm_addr, p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, cnn_getcycle(cnn_id)*5);
#endif
			break;
		case AI_MODE_ROIPOOL:
			er_code = kdrv_ai_roipool_trig((KDRV_AI_ROIPOOL_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_roipool_done(p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, nue_getcycle());
#endif
			break;
		case AI_MODE_SVM:
			er_code = kdrv_ai_svm_trig((KDRV_AI_SVM_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_svm_done((KDRV_AI_SVM_PARM *)p_cur->parm_addr, p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, nue_getcycle());
#endif
			break;
		case AI_MODE_FC:
			er_code = kdrv_ai_fc_trig((KDRV_AI_FC_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_fc_done((KDRV_AI_FC_PARM *)p_cur->parm_addr, p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, nue_getcycle());
#endif
			break;
		case AI_MODE_PERMUTE:
			er_code = kdrv_ai_permute_trig((KDRV_AI_PERMUTE_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_permute_done(p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, nue_getcycle());
#endif
			break;
		case AI_MODE_REORG:
			er_code = kdrv_ai_reorg_trig((KDRV_AI_REORG_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_reorg_done(p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, nue_getcycle());
#endif
			break;
		case AI_MODE_ANCHOR:
			er_code = kdrv_ai_anchor_trig((KDRV_AI_ANCHOR_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_anchor_done(p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, nue_getcycle());
#endif
			break;

		case AI_MODE_SOFTMAX:
			er_code = kdrv_ai_softmax_trig((KDRV_AI_SOFTMAX_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_softmax_done(p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, nue_getcycle());
#endif
			break;

		case AI_MODE_PREPROC:
			er_code = kdrv_ai_preproc_trig((KDRV_AI_PREPROC_PARM *)p_cur->parm_addr);
			if (er_code != E_OK) {
				return er_code;
			}
			er_code = kdrv_ai_preproc_done(p_handle);
			if (er_code != E_OK) {
				return er_code;
			}
#if KDRV_AI_SINGLE_LAYER_CYCLE
			kdrv_ai_set_layer_cycle(ai_eng, nue2_getcycle());
#endif
			break;
		default:
			break;
		}
		p_cur = (KDRV_AI_APP_HEAD *)p_cur->stripe_head_addr;
	}

	PROF_END("kdrv_ai_app");

	return E_OK;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

ER kdrv_ai_ll_trigger(UINT32 id)
{
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
	KDRV_AI_LL_HEAD *p_head;
	ER er_code = E_OK;
	KDRV_AI_HANDLE *p_handle = kdrv_ai_id_conv2_handle(id);
	KDRV_AI_PRAM *p_ai_info = kdrv_ai_id_conv2_parm(id);

	if ((p_handle == NULL) || (p_ai_info == NULL)) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}

	p_head = &p_ai_info->ll_parm;

	PROF_START();

	//trigger process
	if (ai_eng == AI_ENG_CNN) {
		er_code = kdrv_ai_cnn_ll_trig(0, vos_cpu_get_phy_addr(p_head->parm_addr));
	} else if (ai_eng == AI_ENG_NUE) {
		er_code = kdrv_ai_nue_ll_trig(vos_cpu_get_phy_addr(p_head->parm_addr));
	} else if (ai_eng == AI_ENG_NUE2) {
		er_code = kdrv_ai_nue2_ll_trig(vos_cpu_get_phy_addr(p_head->parm_addr));
	} else if (ai_eng == AI_ENG_CNN2) {
		er_code = kdrv_ai_cnn_ll_trig(1, vos_cpu_get_phy_addr(p_head->parm_addr));
	}



	return er_code;
}

ER kdrv_ai_ll_done(UINT32 id)
{
	KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(KDRV_DEV_ID_ENGINE(id));
	ER er_code = E_OK;
	KDRV_AI_HANDLE *p_handle = kdrv_ai_id_conv2_handle(id);
	if (p_handle == NULL) {
		DBG_ERR("unknown id : %d\r\n", (int)id);
		return E_ID;
	}


	if (ai_eng == AI_ENG_CNN) {
		er_code = kdrv_ai_cnn_ll_done(0, p_handle);
	} else if (ai_eng == AI_ENG_NUE) {
		er_code = kdrv_ai_nue_ll_done(p_handle);
	} else if (ai_eng == AI_ENG_NUE2) {
		er_code = kdrv_ai_nue2_ll_done(p_handle);
	} else if (ai_eng == AI_ENG_CNN2) {
		er_code = kdrv_ai_cnn_ll_done(1, p_handle);
	}

	PROF_END("kdrv_ai_ll");
	return er_code;
}

INT32 kdrv_ai_set_ll_base_addr(KDRV_AI_ENG eng, UINT32 addr)
{
	if ((eng > (AI_ENG_TOTAL-1)) || eng < 0) {
		DBG_ERR("unknown engine id : %d\r\n", (int)eng);
		return -1;
	}
	g_kdrv_ai_ll_base_addr[eng] = addr;
	
	return 0;
}

UINT32 kdrv_ai_get_eng_caps(KDRV_AI_ENG eng)
{
	UINT32 is_valid = 0;
	
	switch (eng) {
		case AI_ENG_CNN:
			is_valid = cnn_eng_valid(0);
			break;
		case AI_ENG_NUE:
		case AI_ENG_NUE2:
			is_valid = 1;
			break;
		case AI_ENG_CNN2:
			is_valid = cnn_eng_valid(1);
			break;
		default:
			break;
	}
	
	return is_valid;
}

VOID kdrv_ai_reset_status(VOID)
{
	kdrv_ai_uninstall_id();
	kdrv_ai_install_id();
	g_ai_isr_trig = 0;
	memset(g_kdrv_ai_init_flg, 0, AI_ENG_TOTAL*sizeof(UINT32));
	memset(g_kdrv_ai_open_times, 0, AI_ENG_TOTAL*sizeof(UINT32));
	memset(g_kdrv_ai_handle_tab, 0, AI_ENG_TOTAL*KDRV_AI_HANDLE_MAX_NUM*sizeof(KDRV_AI_HANDLE));
	memset(g_kdrv_ai_open_cfg, 0, AI_ENG_TOTAL*sizeof(KDRV_AI_OPENCFG));
	memset(g_kdrv_ai_parm_tab, 0, AI_ENG_TOTAL*KDRV_AI_HANDLE_MAX_NUM*sizeof(KDRV_AI_PRAM));
	
	cnn_reset_status();
	nue_reset_status();
	nue2_reset_status();
}

INT32 kdrv_ai_dma_abort(UINT32 chip, UINT32 engine)
{
	ER er_code = E_OK;
	//KDRV_AI_ENG ai_eng = kdrv_ai_trans_id2eng(engine);

	//if (AI_ENG_TOTAL == ai_eng) {
#if defined(_BSP_NA51089_)
		nue_dma_abort();
		cnn_dma_abort(0);
		nue2_dma_abort();
#else
		nue_dma_abort();
		cnn_dma_abort(0);
		cnn_dma_abort(1);
		nue2_dma_abort();
#endif
	//}

	return er_code;
}

#ifdef __KERNEL__
#if (NEW_AI_FLOW == 1)
EXPORT_SYMBOL(kdrv_ai_config_flow);
#endif

EXPORT_SYMBOL(kdrv_ai_open);
EXPORT_SYMBOL(kdrv_ai_close);
EXPORT_SYMBOL(kdrv_ai_trigger);
EXPORT_SYMBOL(kdrv_ai_waitdone);
EXPORT_SYMBOL(kdrv_ai_reset);
EXPORT_SYMBOL(kdrv_ai_set);
EXPORT_SYMBOL(kdrv_ai_get);
EXPORT_SYMBOL(kdrv_ai_set_ll_base_addr);
EXPORT_SYMBOL(kdrv_ai_get_eng_caps);
EXPORT_SYMBOL(kdrv_ai_reset_status);
EXPORT_SYMBOL(kdrv_ai_dma_abort);
EXPORT_SYMBOL(kdrv_ai_engine_reset); 

#endif

