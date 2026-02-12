
//=============================================================
#define __CLASS__ 				"[ai][kflow][core]"
#include "kflow_ai_debug.h"
//=============================================================
#include "kflow_ai_net/kflow_ai_core_callback.h"
#include "kflow_ai_net/kflow_ai_net_platform.h"

/////////////////////////////////////////////////////////////////////////////////////////

#if 0

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

#else
#include <linux/wait.h>
#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

static ID g_ai_core_cb_wq = 0; //wait user CB begin
static ID g_ai_core_cb_wq2 = 0; //wait user CB end
static int g_ai_core_cb_init = 0;
static int g_ai_core_cb_begin = 0;
static int g_ai_core_cb_end = 0;
static UINT32 g_ai_core_cb_unit = 0;

#define FLG_ID_WQ_ALL		0xFFFFFFFF
#define FLG_ID_WQ_BEGIN	FLGPTN_BIT(0)
#define FLG_ID_WQ_END		FLGPTN_BIT(1)

int kflow_ai_core_cb_init(UINT32 proc_id)
{
	g_ai_core_cb_begin = 0;
	g_ai_core_cb_end = 0;
	g_ai_core_cb_unit = 0;
	//log_sfile = 0;
	//if (g_init == 0) {
		OS_CONFIG_FLAG(g_ai_core_cb_wq);
		OS_CONFIG_FLAG(g_ai_core_cb_wq2);
	//}
	clr_flg(g_ai_core_cb_wq, FLG_ID_WQ_ALL);
	clr_flg(g_ai_core_cb_wq2, FLG_ID_WQ_ALL);
	g_ai_core_cb_init = 1;
	return 1;
}
int kflow_ai_core_cb_exit(UINT32 proc_id)
{
	if(!g_ai_core_cb_init)
		return 0;
//	log_begin = -1;
	set_flg(g_ai_core_cb_wq, FLG_ID_WQ_BEGIN); //force quit from wait user CB begin?
	rel_flg(g_ai_core_cb_wq);
	rel_flg(g_ai_core_cb_wq2);
	g_ai_core_cb_init = 0;
	return 1;
}

//kernel 硄 callback start / 单 callback end
void kflow_ai_job_cb(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	FLGPTN          flag = 0;
	if(!g_ai_core_cb_init) {
		return;
	}
	g_ai_core_cb_begin = (UINT32)p_job;
	set_flg(g_ai_core_cb_wq, FLG_ID_WQ_BEGIN);
	///.....
	wai_flg(&flag, g_ai_core_cb_wq2, FLG_ID_WQ_END, TWF_ORW|TWF_CLR);
	g_ai_core_cb_end = 0;
}
//user 单 callback start
KFLOW_AI_JOB* kflow_ai_core_cb_wait(UINT32 proc_id)
{
	FLGPTN          flag = 0;
	if(!g_ai_core_cb_init) {
		return 0;
	}
	wai_flg(&flag, g_ai_core_cb_wq, FLG_ID_WQ_BEGIN, TWF_ORW|TWF_CLR);
	g_ai_core_cb_unit = g_ai_core_cb_begin;
	g_ai_core_cb_begin = 0;
	return (KFLOW_AI_JOB*)g_ai_core_cb_unit;
}
//user 硄 callback end
void kflow_ai_core_cb_sig(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	if(!g_ai_core_cb_init) {
		return;
	}
	g_ai_core_cb_end = (UINT32)p_job;
	set_flg(g_ai_core_cb_wq2, FLG_ID_WQ_END);
	g_ai_core_cb_unit = 0;
}

#else

typedef struct _JOB_CB {
	ID wq;
	ID wq2;
	int init;
	int begin;
	int end;
	UINT32 unit;
} JOB_CB;

extern UINT32 g_ai_support_net_max;
#define FLG_ID_WQ_ALL		0xFFFFFFFF
#define FLG_ID_WQ_BEGIN		FLGPTN_BIT(0)
#define FLG_ID_WQ_END		FLGPTN_BIT(1)

static INT g_ai_job_cb_init = 0;
static JOB_CB *g_ai_job_cb;

int kflow_ai_core_cb_init_cb(VOID)
{
	if (g_ai_job_cb_init == 0) {
		UINT32 i;
		g_ai_job_cb = (JOB_CB *)nvt_ai_mem_alloc(sizeof(JOB_CB) * g_ai_support_net_max);
		if (g_ai_job_cb == NULL) {
			return E_NOMEM;
		}
		memset(g_ai_job_cb, 0x0, sizeof(JOB_CB) * g_ai_support_net_max);
		for(i = 0; i < g_ai_support_net_max; i++) {
			JOB_CB* p_job_cb = (JOB_CB*)(g_ai_job_cb + i);
			OS_CONFIG_FLAG(p_job_cb->wq);
			OS_CONFIG_FLAG(p_job_cb->wq2);
		}
		g_ai_job_cb_init = 1;
	}
	return E_OK;
}

int kflow_ai_core_cb_uninit_cb(VOID)
{
	if (g_ai_job_cb_init) {
		if (g_ai_job_cb) {
			UINT32 i;
			for(i = 0; i < g_ai_support_net_max; i++) {
				JOB_CB* p_job_cb = (JOB_CB*)(g_ai_job_cb + i);
				rel_flg(p_job_cb->wq);
				rel_flg(p_job_cb->wq2);
			}
			nvt_ai_mem_free(g_ai_job_cb);
			g_ai_job_cb = 0;
		}
		g_ai_job_cb_init = 0;
	}
	return E_OK;
}

int kflow_ai_core_cb_init(UINT32 proc_id)
{
	JOB_CB* p_job_cb = (JOB_CB*)(g_ai_job_cb + proc_id);
	p_job_cb->begin = 0;
	p_job_cb->end = 0;
	p_job_cb->unit = 0;
	//OS_CONFIG_FLAG(p_job_cb->wq);
	//OS_CONFIG_FLAG(p_job_cb->wq2);
	clr_flg(p_job_cb->wq, FLG_ID_WQ_ALL);//BEGIN
	clr_flg(p_job_cb->wq2, FLG_ID_WQ_ALL);//END
	p_job_cb->init = 1;
	return 1;
}
int kflow_ai_core_cb_exit(UINT32 proc_id)
{
	JOB_CB* p_job_cb = (JOB_CB*)(g_ai_job_cb + proc_id);
	if(!p_job_cb->init)
		return 0;
	//force to quit
	p_job_cb->begin = -1;
	set_flg(p_job_cb->wq, FLG_ID_WQ_BEGIN); //force quit from wait user CB begin?
	//rel_flg(p_job_cb->wq);
	//rel_flg(p_job_cb->wq2);
	p_job_cb->init = 0;
	return 1;
}

//kernel 硄 callback start / 单 callback end
void kflow_ai_job_cb(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	FLGPTN          flag = 0;
	JOB_CB* p_job_cb = (JOB_CB*)(g_ai_job_cb + proc_id);
	if(!p_job_cb->init) {
		return;
	}
	p_job_cb->begin = (UINT32)p_job;
	set_flg(p_job_cb->wq, FLG_ID_WQ_BEGIN);
	///.....
	if(p_job_cb->end==0) {
		vos_flag_wait(&flag, p_job_cb->wq2, FLG_ID_WQ_END, TWF_CLR);
	}
	p_job_cb->end = 0;
}
EXPORT_SYMBOL(kflow_ai_job_cb);

//user 单 callback start
KFLOW_AI_JOB* kflow_ai_core_cb_wait(UINT32 proc_id)
{
	FLGPTN          flag = 0;
	JOB_CB* p_job_cb = (JOB_CB*)(g_ai_job_cb + proc_id);
	if(!p_job_cb->init) {
		return 0;
	}
	if(p_job_cb->begin != -1) {
		vos_flag_wait_interruptible(&flag, p_job_cb->wq, FLG_ID_WQ_BEGIN, TWF_CLR);
	}
	p_job_cb->unit = p_job_cb->begin;
	p_job_cb->begin = 0;
	
	if (p_job_cb->unit == (UINT32)-1) {
		return 0;
	}
	return (KFLOW_AI_JOB*)p_job_cb->unit;
}
//user 硄 callback end
void kflow_ai_core_cb_sig(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	JOB_CB* p_job_cb = (JOB_CB*)(g_ai_job_cb + proc_id);
	if(!p_job_cb->init) {
		return;
	}
	p_job_cb->end = (UINT32)p_job;
	set_flg(p_job_cb->wq2, FLG_ID_WQ_END);
	p_job_cb->unit = 0;
}
#endif

#endif

