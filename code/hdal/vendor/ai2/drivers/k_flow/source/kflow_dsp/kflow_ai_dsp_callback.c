
#include "kflow_dsp_dbg.h"
#include "kflow_dsp/kflow_dsp_callback.h"
#include "kdrv_ai.h"
#include "kflow_dsp/kflow_dsp_platform.h"

/////////////////////////////////////////////////////////////////////////////////////////

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

#else
#include <linux/wait.h>
#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

static ID g_ai_dsp_wq = 0; //wait user CB begin
static ID g_ai_dsp_wq2 = 0; //wait user CB end
static int g_ai_dsp_init = 0;
static int g_ai_dsp_begin = 0;
static int g_ai_dsp_end = 0;
static UINT32 g_ai_dsp_unit = 0;

#define FLG_ID_WQ_ALL		0xFFFFFFFF
#define FLG_ID_WQ_BEGIN	FLGPTN_BIT(0)
#define FLG_ID_WQ_END		FLGPTN_BIT(1)

int kflow_ai_dsp_init(UINT32 proc_id)
{
	g_ai_dsp_begin = 0;
	g_ai_dsp_end = 0;
	g_ai_dsp_unit = 0;
	//log_sfile = 0;
	//if (g_init == 0) {
		OS_CONFIG_FLAG(g_ai_dsp_wq);
		OS_CONFIG_FLAG(g_ai_dsp_wq2);
	//}
	clr_flg(g_ai_dsp_wq, FLG_ID_WQ_ALL);
	clr_flg(g_ai_dsp_wq2, FLG_ID_WQ_ALL);
	g_ai_dsp_init = 1;
	return 1;
}
int kflow_ai_dsp_exit(UINT32 proc_id)
{
	if(!g_ai_dsp_init)
		return 0;
//	log_begin = -1;
	set_flg(g_ai_dsp_wq, FLG_ID_WQ_BEGIN); //force quit from wait user CB begin?
	rel_flg(g_ai_dsp_wq);
	rel_flg(g_ai_dsp_wq2);
	g_ai_dsp_init = 0;
	return 1;
}

//kernel 硄 callback start / 单 callback end
void kflow_ai_dsp_cb(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	FLGPTN          flag = 0;
	if(!g_ai_dsp_init) {
		return;
	}
	g_ai_dsp_begin = (UINT32)p_job;
	set_flg(g_ai_dsp_wq, FLG_ID_WQ_BEGIN);
	///.....
	wai_flg(&flag, g_ai_dsp_wq2, FLG_ID_WQ_END, TWF_ORW|TWF_CLR);
	g_ai_dsp_end = 0;
}
//user 单 callback start
KFLOW_AI_JOB* kflow_ai_dsp_wait(UINT32 proc_id)
{
	FLGPTN          flag = 0;
	if(!g_ai_dsp_init) {
		return 0;
	}
	wai_flg(&flag, g_ai_dsp_wq, FLG_ID_WQ_BEGIN, TWF_ORW|TWF_CLR);
	g_ai_dsp_unit = g_ai_dsp_begin;
	g_ai_dsp_begin = 0;
	return (KFLOW_AI_JOB*)g_ai_dsp_unit;
}
//user 硄 callback end
void kflow_ai_dsp_sig(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	if(!g_ai_dsp_init) {
		return;
	}
	g_ai_dsp_end = (UINT32)p_job;
	set_flg(g_ai_dsp_wq2, FLG_ID_WQ_END);
	g_ai_dsp_unit = 0;
}

#else

extern BOOL g_ai_net_path_open[AI_SUPPORT_NET_MAX];
extern BOOL g_ai_net_is_multi_process;

typedef struct _DSP_CB {
	ID wq;
	ID wq2;
	int init;
	int begin;
	int end;
	UINT32 unit;
} DSP_CB;

#define FLG_ID_WQ_ALL		0xFFFFFFFF
#define FLG_ID_WQ_BEGIN		FLGPTN_BIT(0)
#define FLG_ID_WQ_END		FLGPTN_BIT(1)

extern UINT32 kdrv_ai_drv_get_net_supported_num(VOID);
static INT g_ai_dsp_cb_init_cnt = 0;
static DSP_CB *g_ai_dsp_cb;

int kflow_ai_dsp_init_cb(VOID)
{
	UINT32 ai_support_net_max;

	ai_support_net_max = kdrv_ai_drv_get_net_supported_num();
	if (ai_support_net_max < 1) {
		return E_SYS;
	}
	if (g_ai_dsp_cb_init_cnt == 0) {
		UINT32 i;
		g_ai_dsp_cb = (DSP_CB *)nvt_ai_mem_alloc_dsp(sizeof(DSP_CB) * ai_support_net_max);
		if (g_ai_dsp_cb == NULL) {
			return E_NOMEM;
		}
		memset(g_ai_dsp_cb, 0x0, sizeof(DSP_CB) * ai_support_net_max);
		for(i = 0; i < ai_support_net_max; i++) {
			DSP_CB* p_dsp_cb = (DSP_CB*)(g_ai_dsp_cb + i);
			OS_CONFIG_FLAG(p_dsp_cb->wq);
			OS_CONFIG_FLAG(p_dsp_cb->wq2);
		}
		g_ai_dsp_cb_init_cnt = 1;
	}
	return E_OK;
}
EXPORT_SYMBOL(kflow_ai_dsp_init_cb);

int kflow_ai_dsp_uninit_cb(VOID)
{
	UINT32 ai_support_net_max;
	ai_support_net_max = kdrv_ai_drv_get_net_supported_num();
	if (ai_support_net_max < 1) {
		return E_SYS;
	}
	if (g_ai_dsp_cb_init_cnt) {
		if (g_ai_dsp_cb) {
			UINT32 i;
			for(i = 0; i < ai_support_net_max; i++) {
				DSP_CB* p_dsp_cb = (DSP_CB*)(g_ai_dsp_cb + i);
				rel_flg(p_dsp_cb->wq);
				rel_flg(p_dsp_cb->wq2);
			}
			nvt_ai_mem_free_dsp(g_ai_dsp_cb);
			g_ai_dsp_cb = 0;
		}
		g_ai_dsp_cb_init_cnt = 0;
	}
	return E_OK;
}
EXPORT_SYMBOL(kflow_ai_dsp_uninit_cb);

int kflow_ai_dsp_uninit_cb_path(UINT32 net_id)
{
	if (g_ai_dsp_cb) {
		DSP_CB* p_dsp_cb = (DSP_CB*)(g_ai_dsp_cb + net_id);
		rel_flg(p_dsp_cb->wq);
		rel_flg(p_dsp_cb->wq2);
		memset(g_ai_dsp_cb + net_id, 0x0, sizeof(DSP_CB));
		OS_CONFIG_FLAG(p_dsp_cb->wq);
		OS_CONFIG_FLAG(p_dsp_cb->wq2);
	}
	return E_OK;
}
EXPORT_SYMBOL(kflow_ai_dsp_uninit_cb_path);

int kflow_ai_dsp_init(UINT32 proc_id)
{
	DSP_CB* p_dsp_cb = (DSP_CB*)(g_ai_dsp_cb + proc_id);
	p_dsp_cb->begin = 0;
	p_dsp_cb->end = 0;
	p_dsp_cb->unit = 0;
	//OS_CONFIG_FLAG(p_dsp_cb->wq);
	//OS_CONFIG_FLAG(p_dsp_cb->wq2);
	clr_flg(p_dsp_cb->wq, FLG_ID_WQ_ALL);
	clr_flg(p_dsp_cb->wq2, FLG_ID_WQ_ALL);
	p_dsp_cb->init = 1;
	return 1;
}
EXPORT_SYMBOL(kflow_ai_dsp_init);
int kflow_ai_dsp_exit(UINT32 proc_id)
{
	DSP_CB* p_dsp_cb = (DSP_CB*)(g_ai_dsp_cb + proc_id);
	if(!p_dsp_cb->init)
		return 0;
	//force to quit
	p_dsp_cb->begin = -1;
	set_flg(p_dsp_cb->wq, FLG_ID_WQ_BEGIN); //force quit from wait user CB begin?
	//rel_flg(p_dsp_cb->wq);
	//rel_flg(p_dsp_cb->wq2);
	p_dsp_cb->init = 0;

	return 1;
}
EXPORT_SYMBOL(kflow_ai_dsp_exit);

//kernel 硄 callback start / 单 callback end
void kflow_ai_dsp_cb(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	FLGPTN          flag = 0;
	DSP_CB* p_dsp_cb = (DSP_CB*)(g_ai_dsp_cb + proc_id);
	if(!p_dsp_cb->init) {
		return;
	}

	// trigger user space job processing
	p_dsp_cb->begin = (UINT32)p_job;
	set_flg(p_dsp_cb->wq, FLG_ID_WQ_BEGIN);

	// waiting user space job processing
	if(!g_ai_net_is_multi_process || g_ai_net_path_open[proc_id] != 0) {
		vos_flag_wait(&flag, p_dsp_cb->wq2, FLG_ID_WQ_END, TWF_CLR);
	}
	p_dsp_cb->end = 0;
}
EXPORT_SYMBOL(kflow_ai_dsp_cb);
//user 单 callback start
KFLOW_AI_JOB* kflow_ai_dsp_wait(UINT32 proc_id)
{
	FLGPTN          flag = 0;
	DSP_CB* p_dsp_cb = (DSP_CB*)(g_ai_dsp_cb + proc_id);
	if(!p_dsp_cb->init) {
		return 0;
	}
	if(p_dsp_cb->begin != -1) {
		vos_flag_wait_interruptible(&flag, p_dsp_cb->wq, FLG_ID_WQ_BEGIN, TWF_CLR);
	}
	p_dsp_cb->unit = p_dsp_cb->begin;
	p_dsp_cb->begin = 0;
	
	if (p_dsp_cb->unit == (UINT32)-1) {
		return 0;
	}
	return (KFLOW_AI_JOB*)p_dsp_cb->unit;
}
EXPORT_SYMBOL(kflow_ai_dsp_wait);
//user 硄 callback end
void kflow_ai_dsp_sig(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	DSP_CB* p_dsp_cb = (DSP_CB*)(g_ai_dsp_cb + proc_id);
	if(!p_dsp_cb->init) {
		return;
	}
	p_dsp_cb->end = (UINT32)p_job;
	set_flg(p_dsp_cb->wq2, FLG_ID_WQ_END);
	p_dsp_cb->unit = 0;
}
EXPORT_SYMBOL(kflow_ai_dsp_sig);
#endif

