
#include "kflow_cpu_dbg.h"
#include "kflow_cpu/kflow_cpu_callback.h"
#include "kdrv_ai.h"
#include "kflow_cpu/kflow_cpu_platform.h"

/////////////////////////////////////////////////////////////////////////////////////////

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

#else
#include <linux/wait.h>
#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

static ID g_ai_cpu_wq = 0; //wait user CB begin
static ID g_ai_cpu_wq2 = 0; //wait user CB end
static int g_ai_cpu_init = 0;
static int g_ai_cpu_begin = 0;
static int g_ai_cpu_end = 0;
static UINT32 g_ai_cpu_unit = 0;

#define FLG_ID_WQ_ALL		0xFFFFFFFF
#define FLG_ID_WQ_BEGIN	FLGPTN_BIT(0)
#define FLG_ID_WQ_END		FLGPTN_BIT(1)

int kflow_ai_cpu_init(UINT32 proc_id)
{
	g_ai_cpu_begin = 0;
	g_ai_cpu_end = 0;
	g_ai_cpu_unit = 0;
	//log_sfile = 0;
	//if (g_init == 0) {
		OS_CONFIG_FLAG(g_ai_cpu_wq);
		OS_CONFIG_FLAG(g_ai_cpu_wq2);
	//}
	clr_flg(g_ai_cpu_wq, FLG_ID_WQ_ALL);
	clr_flg(g_ai_cpu_wq2, FLG_ID_WQ_ALL);
	g_ai_cpu_init = 1;
	return 1;
}
int kflow_ai_cpu_exit(UINT32 proc_id)
{
	if(!g_ai_cpu_init)
		return 0;
//	log_begin = -1;
	set_flg(g_ai_cpu_wq, FLG_ID_WQ_BEGIN); //force quit from wait user CB begin?
	rel_flg(g_ai_cpu_wq);
	rel_flg(g_ai_cpu_wq2);
	g_ai_cpu_init = 0;
	return 1;
}

//kernel 硄 callback start / 单 callback end
void kflow_ai_cpu_cb(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	FLGPTN          flag = 0;
	if(!g_ai_cpu_init) {
		return;
	}
	g_ai_cpu_begin = (UINT32)p_job;
	set_flg(g_ai_cpu_wq, FLG_ID_WQ_BEGIN);
	///.....
	wai_flg(&flag, g_ai_cpu_wq2, FLG_ID_WQ_END, TWF_ORW|TWF_CLR);
	g_ai_cpu_end = 0;
}
//user 单 callback start
#if NN_DLI
KFLOW_AI_JOB* kflow_ai_cpu_wait(UINT32 proc_id, UINT32* p_event)
#else
KFLOW_AI_JOB* kflow_ai_cpu_wait(UINT32 proc_id)
#endif
{
	FLGPTN          flag = 0;
	if(!g_ai_cpu_init) {
		return 0;
	}
	wai_flg(&flag, g_ai_cpu_wq, FLG_ID_WQ_BEGIN, TWF_ORW|TWF_CLR);
	g_ai_cpu_unit = g_ai_cpu_begin;
	g_ai_cpu_begin = 0;
	return (KFLOW_AI_JOB*)g_ai_cpu_unit;
}
//user 硄 callback end
void kflow_ai_cpu_sig(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	if(!g_ai_cpu_init) {
		return;
	}
	g_ai_cpu_end = (UINT32)p_job;
	set_flg(g_ai_cpu_wq2, FLG_ID_WQ_END);
	g_ai_cpu_unit = 0;
}

#else

extern BOOL g_ai_net_path_open[AI_SUPPORT_NET_MAX];
extern BOOL g_ai_net_is_multi_process;

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#else
UINT32 still_wait_cpu = 0;
EXPORT_SYMBOL(still_wait_cpu);
#endif

typedef struct _CPU_CB {
	ID wq;
	ID wq2;
	int init;
	int begin;
	int end;
	UINT32 unit;
} CPU_CB;

#define FLG_ID_WQ_ALL		0xFFFFFFFF
#define FLG_ID_WQ_BEGIN		FLGPTN_BIT(0)
#define FLG_ID_WQ_END		FLGPTN_BIT(1)
#if NN_DLI
#define FLG_ID_WQ_BEGIN_2	FLGPTN_BIT(2)
#define FLG_ID_WQ_BEGIN_3	FLGPTN_BIT(3)
#endif

extern UINT32 kdrv_ai_drv_get_net_supported_num(VOID);
static INT g_ai_cpu_cb_init_cnt = 0;
static CPU_CB *g_ai_cpu_cb;

int kflow_ai_cpu_init_cb(VOID)
{
	UINT32 ai_support_net_max;

	ai_support_net_max = kdrv_ai_drv_get_net_supported_num();
	if (ai_support_net_max < 1) {
		return E_SYS;
	}
	if (g_ai_cpu_cb_init_cnt == 0) {
		UINT32 i;
		g_ai_cpu_cb = (CPU_CB *)nvt_ai_mem_alloc_cpu(sizeof(CPU_CB) * ai_support_net_max);
		if (g_ai_cpu_cb == NULL) {
			return E_NOMEM;
		}
		memset(g_ai_cpu_cb, 0x0, sizeof(CPU_CB) * ai_support_net_max);
		for(i = 0; i < ai_support_net_max; i++) {
			CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + i);
			OS_CONFIG_FLAG(p_cpu_cb->wq);
			OS_CONFIG_FLAG(p_cpu_cb->wq2);
		}
		g_ai_cpu_cb_init_cnt = 1;
	}
	return E_OK;
}
EXPORT_SYMBOL(kflow_ai_cpu_init_cb);

int kflow_ai_cpu_uninit_cb(VOID)
{
	UINT32 ai_support_net_max;

	ai_support_net_max = kdrv_ai_drv_get_net_supported_num();
	if (ai_support_net_max < 1) {
		return E_SYS;
	}
	if (g_ai_cpu_cb_init_cnt) {
		if (g_ai_cpu_cb) {
			UINT32 i;
			for(i = 0; i < ai_support_net_max; i++) {
				CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + i);
				rel_flg(p_cpu_cb->wq);
				rel_flg(p_cpu_cb->wq2);
			}
			nvt_ai_mem_free_cpu(g_ai_cpu_cb);
			g_ai_cpu_cb = 0;
		}
		g_ai_cpu_cb_init_cnt = 0;
	}
	return E_OK;
}
EXPORT_SYMBOL(kflow_ai_cpu_uninit_cb);

int kflow_ai_cpu_uninit_cb_path(UINT32 net_id)
{
	if (g_ai_cpu_cb) {
		CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + net_id);
		rel_flg(p_cpu_cb->wq);
		rel_flg(p_cpu_cb->wq2);
		memset(g_ai_cpu_cb + net_id, 0x0, sizeof(CPU_CB));
		OS_CONFIG_FLAG(p_cpu_cb->wq);
		OS_CONFIG_FLAG(p_cpu_cb->wq2);
	}
	return E_OK;
}
EXPORT_SYMBOL(kflow_ai_cpu_uninit_cb_path);

int kflow_ai_cpu_init(UINT32 proc_id)
{
	CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + proc_id);
	p_cpu_cb->begin = 0;
	p_cpu_cb->end = 0;
	p_cpu_cb->unit = 0;
	//OS_CONFIG_FLAG(p_cpu_cb->wq);
	//OS_CONFIG_FLAG(p_cpu_cb->wq2);
	clr_flg(p_cpu_cb->wq, FLG_ID_WQ_ALL); //BEGIN
	clr_flg(p_cpu_cb->wq2, FLG_ID_WQ_ALL); //END
	p_cpu_cb->init = 1;
	return 1;
}
EXPORT_SYMBOL(kflow_ai_cpu_init);
int kflow_ai_cpu_exit(UINT32 proc_id)
{
	CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + proc_id);
	if(!p_cpu_cb->init)
		return 0;
	//force to quit
	p_cpu_cb->begin = -1;
	set_flg(p_cpu_cb->wq, FLG_ID_WQ_BEGIN); //force quit from wait user CB begin?
	//rel_flg(p_cpu_cb->wq);
	//rel_flg(p_cpu_cb->wq2);
	p_cpu_cb->init = 0;
	return 1;
}
EXPORT_SYMBOL(kflow_ai_cpu_exit);

#if NN_DLI
//kernel callback start / callback end
void kflow_ai_cpu_cb2(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	FLGPTN          flag = 0;
	CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + proc_id);
	if(!p_cpu_cb->init) {
		return;
	}

	// trigger user space job processing
	p_cpu_cb->begin = (UINT32)p_job;
	set_flg(p_cpu_cb->wq, FLG_ID_WQ_BEGIN_2);

	// waiting user space job processing
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	vos_flag_wait(&flag, p_cpu_cb->wq2, FLG_ID_WQ_END, TWF_CLR);
#else
	still_wait_cpu = 1;
	vos_flag_wait(&flag, p_cpu_cb->wq2, FLG_ID_WQ_END, TWF_CLR);
	still_wait_cpu = 0;
#endif
	p_cpu_cb->end = 0;
}
EXPORT_SYMBOL(kflow_ai_cpu_cb2);

//kernel callback start / callback end
void kflow_ai_cpu_cb3(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	FLGPTN          flag = 0;
	CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + proc_id);
	if(!p_cpu_cb->init) {
		return;
	}

	// trigger user space job processing
	p_cpu_cb->begin = (UINT32)p_job;
	set_flg(p_cpu_cb->wq, FLG_ID_WQ_BEGIN_3);

	// waiting user space job processing
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	vos_flag_wait(&flag, p_cpu_cb->wq2, FLG_ID_WQ_END, TWF_CLR);
#else
	still_wait_cpu = 1;
	vos_flag_wait(&flag, p_cpu_cb->wq2, FLG_ID_WQ_END, TWF_CLR);
	still_wait_cpu = 0;
#endif
	p_cpu_cb->end = 0;
}
EXPORT_SYMBOL(kflow_ai_cpu_cb3);
#endif

//kernel 硄 callback start / 单 callback end
void kflow_ai_cpu_cb(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	FLGPTN          flag = 0;
	CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + proc_id);
	if(!p_cpu_cb->init) {
		return;
	}

	// trigger user space job processing
	p_cpu_cb->begin = (UINT32)p_job;
	set_flg(p_cpu_cb->wq, FLG_ID_WQ_BEGIN);

	if(!g_ai_net_is_multi_process || g_ai_net_path_open[proc_id] != 0) {
		// waiting user space job processing
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		vos_flag_wait(&flag, p_cpu_cb->wq2, FLG_ID_WQ_END, TWF_CLR);
#else
		still_wait_cpu = 1;
		vos_flag_wait(&flag, p_cpu_cb->wq2, FLG_ID_WQ_END, TWF_CLR);
		still_wait_cpu = 0;
#endif
	}
	p_cpu_cb->end = 0;
}
EXPORT_SYMBOL(kflow_ai_cpu_cb);

//user 单 callback start
#if NN_DLI
KFLOW_AI_JOB* kflow_ai_cpu_wait(UINT32 proc_id, UINT32* p_event)
#else
KFLOW_AI_JOB* kflow_ai_cpu_wait(UINT32 proc_id)
#endif
{
	FLGPTN          flag = 0;
	int             ret = 0;
	CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + proc_id);
	if(!p_cpu_cb->init) {
		return 0;
	}
	if(p_cpu_cb->begin != -1) {
#if NN_DLI
		ret = vos_flag_wait_interruptible(&flag, p_cpu_cb->wq, FLG_ID_WQ_BEGIN|FLG_ID_WQ_BEGIN_2|FLG_ID_WQ_BEGIN_3, TWF_CLR|TWF_ORW);
#else
		ret = vos_flag_wait_interruptible(&flag, p_cpu_cb->wq, FLG_ID_WQ_BEGIN, TWF_CLR);
#endif
	}
	if (ret == E_RLWAI) {
		return (KFLOW_AI_JOB*)0x0C; //interrupt by system (include ctrl-c or other signals)
	}
	p_cpu_cb->unit = p_cpu_cb->begin;
	p_cpu_cb->begin = 0;

	if (p_cpu_cb->unit == (UINT32)-1) {
		return 0; //stopped by user (normal case)
	}

#if NN_DLI
	if (flag & FLG_ID_WQ_BEGIN_2) {
		if (p_event) (*p_event) = 0xff; //start
		return (KFLOW_AI_JOB*)p_cpu_cb->unit;
	}
	
	if (flag & FLG_ID_WQ_BEGIN_3) {
		if (p_event) (*p_event) = 0xfe; //stop
		return (KFLOW_AI_JOB*)p_cpu_cb->unit;
	}

	if (p_event) (*p_event) = 0; //proc
#endif
	return (KFLOW_AI_JOB*)p_cpu_cb->unit;
}
EXPORT_SYMBOL(kflow_ai_cpu_wait);
//user 硄 callback end
void kflow_ai_cpu_sig(UINT32 proc_id, KFLOW_AI_JOB* p_job)
{
	CPU_CB* p_cpu_cb = (CPU_CB*)(g_ai_cpu_cb + proc_id);
	if(!p_cpu_cb->init) {
		return;
	}
	p_cpu_cb->end = (UINT32)p_job;
	set_flg(p_cpu_cb->wq2, FLG_ID_WQ_END);
	p_cpu_cb->unit = 0;
}
EXPORT_SYMBOL(kflow_ai_cpu_sig);
#endif

