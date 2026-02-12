/**
	@brief Source file of kflow_ai_net.

	@file kflow_ai_core.c

	@ingroup kflow_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/

#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/platform.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#if defined(_BSP_NA51090_)  || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
#include "kdrv_ai.h"
#endif

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#else
#include <kwrap/util.h>	
#endif

//=============================================================
#define __CLASS__ 				"[ai][kflow][schd]"
#include "kflow_ai_debug.h"
//=============================================================
#include "kflow_ai_net/kflow_ai_net_platform.h"
#include "kflow_ai_net/kflow_ai_core.h"
#include "kflow_ai_net/kflow_ai_core_task.h" //for KFLOW_CORE_TASK_FUNC
#include "kflow_ai_net/kflow_ai_net_queue.h" //for AI_PROC_QUEUE
#include "kflow_ai_net/kflow_ai_core_callback.h"

#include "kflow_cpu/kflow_cpu_callback.h"
#include "kflow_dsp/kflow_dsp_callback.h"

#include "debug_util/graph_debug_schedule.h"
#include "debug_util/graph_debug_log.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)

static UINT32 klog_mask = 0;
static UINT32 debug_proc_id = 0;
static UINT32 debug_func = 0;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#else
extern UINT32 still_wait_cpu;
#endif
#if 0
#define KLOG_OPEN(size)
#define KLOG_GET() 				(klog_mask)
#define KLOG_SET(mask) 			(klog_mask) = (mask)
#define KLOG_IF(mask) 			((mask) & (klog_mask))
#define KLOG_OUT(mask, fmt, args...) 	if ((mask) & (klog_mask)) DBG_DUMP(fmt, ##args)
#define KLOG_DUMP()
#define KLOG_CLOSE()
#else
#define KLOG_OPEN(size)			graph_debug_log_open(size)
#define KLOG_GET() 				(klog_mask)
#define KLOG_SET(mask) 			(klog_mask) = (mask)
#define KLOG_IF(mask) 			((mask) & (klog_mask))
#define KLOG_OUT(mask, fmt, args...) 	if ((mask) & (klog_mask)) graph_debug_log_print(fmt, ##args)
#define KLOG_DUMP()				graph_debug_log_dump()
#define KLOG_CLOSE()			graph_debug_log_close()
#endif

#define KFLOW_AI_DBG_CORE		0x00000000	//dump current jobs in wait queue, ready queue and run of all engines
#define KFLOW_AI_DBG_BIND		0x00000001	//dump net graph before net proc
#define KFLOW_AI_DBG_SCHD		0x00000002	//dump schedule log while proc each job
#define KFLOW_AI_DBG_CTX		0x00000004	//dump context after proc each job
#define KFLOW_AI_DBG_OBUF		0x00000008	//dump buffer after proc each job

#define KFLOW_AI_DBG_TIME		0x00000100	//dump time after proc
#define KFLOW_AI_DBG_TIMELINE	0x00000200 	//dump timeline after proc

#define KLOG_STAGE 	0x00001000	//graph stage
#define KLOG_BIND 	0x00000200	//bind job
#define KLOG_SETUP 	0x00000100	//setup job
#define KLOG_PULL 	0x00000040	//last job notify user pull
#define KLOG_NEXT 	0x00000020	//job notify next job
#define KLOG_PUSH 	0x00000010	//user push first job
#define KLOG_WAIT 	0x00000004	//add to wait job
#define KLOG_READY 	0x00000002	//add to ready job
#define KLOG_RUN 	0x00000001	//trigger to run job

#define ISR_TRIG 1

#define LOAD_BALANCE 1

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)

#include "kflow_common/nvtmpp.h"
#include "comm/hwclock.h" //for hwclock_get_longcounter()
#define _nvt_ai_get_counter() hwclock_get_longcounter()

#else //_BSP_NA51068_

#include <plat/nvt_jiffies.h> //for get_nvt_jiffies()
#include <linux/vmalloc.h>
#include <linux/slab.h>
#define _nvt_ai_get_counter() get_nvt_jiffies_us()

#endif


#if defined(_BSP_NA51055_)  //520, 528
#define CNN_ENG_CLK  600
#define NUE_ENG_CLK  600
#define NUE2_ENG_CLK 480
#define CNN_ENG_EV   60
#define NUE_ENG_EV   60
#define NUE2_ENG_EV  48
#endif
#if defined(_BSP_NA51089_)  //560
#define CNN_ENG_CLK  600
#define NUE_ENG_CLK  600
#define NUE2_ENG_CLK 480
#define CNN_ENG_EV   60
#define NUE_ENG_EV   60
#define NUE2_ENG_EV  48
#endif
#if defined(_BSP_NA51068_)  //321
#define CNN_ENG_CLK  600
#define NUE_ENG_CLK  600
#define NUE2_ENG_CLK 600
#define CNN_ENG_EV   60
#define NUE_ENG_EV   60
#define NUE2_ENG_EV  60
#endif
#if defined(_BSP_NA51090_)  //336
#define CNN_ENG_CLK  600
#define NUE_ENG_CLK  600
#define NUE2_ENG_CLK 600
#define CNN_ENG_EV   60
#define NUE_ENG_EV   60
#define NUE2_ENG_EV  60
#endif
#if defined(_BSP_NA51103_)  //331
#define CNN_ENG_CLK  600
#define NUE_ENG_CLK  600
#define NUE2_ENG_CLK 600
#define CNN_ENG_EV   60
#define NUE_ENG_EV   60
#define NUE2_ENG_EV  60
#endif
#if defined(_BSP_NA51102_)  //530
#define CNN_ENG_CLK  800
#define NUE_ENG_CLK  800
#define NUE2_ENG_CLK 800
#define CNN_ENG_EV   80
#define NUE_ENG_EV   80
#define NUE2_ENG_EV  80
#endif




/*
vendor_ai_net_open() -> vendor_ais_net_gen_init()
	vendor_ais_remap_kerl_mem() -> VENDOR_AIS_FLOW_IOC_REMAP_ADDR -> nvt_ai_init_mem()
	vendor_ais_flow_user_init() =>
		vendor_ai_drv_init()
			AI_IOC_OPENCFG => kdrv_ai_set KDRV_AI_PARAM_OPENCFG
			AI_IOC_OPENCFG => kdrv_ai_set KDRV_AI_PARAM_OPENCFG
			AI_IOC_OPENCFG => kdrv_ai_set KDRV_AI_PARAM_OPENCFG
			AI_IOC_SET_MAP_ADDR =>
				memcpy(&g_ai_drv_map_mem[nvt_ai_chk_net_id(ai_map_mem_info.net_id)], &ai_map_mem_info.mem, sizeof(AI_DRV_MAP_MEM));
			AI_IOC_ENG_INIT =>
				nue_init();
				cnn_init();
				nue2_init();
	vendor_ais_pars_kerl_mem() -> VENDOR_AIS_FLOW_IOC_PARS_MODEL -> nvt_ai_pars_net()
	///////////////// kflow_ai_net_open()

vendor_ai_net_set(VENDOR_AI_PARAM_TYPE_IN) -> _vendor_ai_net_set_input_img() -> vendor_ais_net_input_init()
	vendor_ais_proc_input_init() -> VENDOR_AIS_FLOW_IOC_INPUT_INIT -> nvt_ai_proc_input_init()
	vendor_ais_update_proclayer() -> VENDOR_AIS_FLOW_IOC_UPDATE_PARM -> nvt_ai_update_layer()

vendor_ai_net_proc()
	vendor_ais_proc_net()
		//-> VENDOR_AIS_FLOW_IOC_SWITCH_PROC -> nvt_ai_proc_net()
		vendor_ai_drv_open
			AI_IOC_OPEN => kdrv_ai_open
		vendor_ai_drv_set_param
			AI_IOC_SET_MODE => kdrv_ai_set KDRV_AI_PARAM_MODE_INFO
			AI_IOC_SET_APP => nvt_ai_drv_user2kerl_va() + kdrv_ai_set KDRV_AI_PARAM_APP_INFO
			AI_IOC_SET_LL => nvt_ai_drv_user2kerl_va() + kdrv_ai_set KDRV_AI_PARAM_LL_INFO
		vendor_ai_drv_get_param
			AI_IOC_GET_MODE => kdrv_ai_get KDRV_AI_PARAM_MODE_INFO
			AI_IOC_GET_APP => kdrv_ai_get KDRV_AI_PARAM_APP_INFO + nvt_ai_drv_kerl2user_va()
			AI_IOC_GET_LL => kdrv_ai_get KDRV_AI_PARAM_LL_INFO + nvt_ai_drv_kerl2user_va()
		vendor_ai_drv_trigger
			AI_IOC_TRIGGER => kdrv_ai_trigger
		vendor_ai_drv_waitdone
			AI_IOC_WAITDONE => kdrv_ai_waitdone
		vendor_ai_drv_close
			AI_IOC_CLOSE => kdrv_ai_close
	vendor_ais_net_input_uninit()
		vendor_ais_proc_input_uninit() -> VENDOR_AIS_FLOW_IOC_INPUT_UNINIT -> nvt_ai_proc_input_uninit()
	///////////////// kflow_ai_net_proc()

vendor_ai_net_close() -> vendor_ais_net_gen_uninit()
	vendor_ais_unmap_kerl_mem() -> VENDOR_AIS_FLOW_IOC_UNMAP_ADDR -> nvt_ai_uinit_mem()
	vendor_ais_flow_user_uninit() =>
		vendor_ai_drv_uninit
			AI_IOC_ENG_UNINIT =>
				nue_uninit();
				cnn_uninit();
				nue2_uninit();
	///////////////// kflow_ai_net_close()
*/


/*
static LIST_HEAD my_head;
static MY_STRUCT my_data;  //MY_STRUCT have member => LIST_HEAD list;

	unsigned long loc_cpu_flg;
	LIST_HEAD *p_this; //current entry
	LIST_HEAD *p_temp; //temp entry
	MY_STRUCT *p_data;

	loc_cpu(loc_cpu_flg);
	INIT_LIST_HEAD(&my_list);
	list_add(&my_data->list, &my_head);
	list_add_tail(&my_data->list, &my_head);
	list_del(&my_data->list);
	if (!list_empty(&my_head)) {
		list_for_each(p_this, &my_head) {
			p_data = list_entry(p_this, MY_STRUCT, list);
		}
		list_for_each_safe(p_this, p_temp, &my_head) {
			p_data = list_entry(p_this, MY_STRUCT, list);
		}
	}
	unl_cpu(loc_cpu_flg);
*/

/*

create()
	int vendor_ai_net_open(void);
	int vendor_ai_net_create(KFLOW_AI_NET* p_net, UINT32 job_cnt, UINT32 bind_cnt);
	int vendor_ai_net_init_job(KFLOW_AI_JOB* p_job, void* p_op_info);
	int vendor_ai_net_bind_job(KFLOW_AI_JOB* p_job, KFLOW_AI_JOB* p_next_job);

optimize()
	1. LL group --- limit to 1-in-1-out, some node is disabled
	2. calc linear order
	int vendor_ai_net_optimize (KFLOW_AI_NET* p_net);

enqueue()
	3. put by linear order
	int vendor_ai_net_enqueue (KFLOW_AI_NET* p_net);

destory()
	int vendor_ai_net_detory(KFLOW_AI_NET* p_net);
	int vendor_ai_net_close(void);

=> schedule() => trigger()



*/

#define SCHD_FAIR 0
#define SCHD_CAPACITY 1
#define SCHD_FIFO 2

#define MAX_ENGINE_ID		10
UINT32 kflow_init = 0;
UINT32 kflow_init_count = 0;
static UINT32 kflow_schd = 0; //0=FIFO, 1=FAIR, 2=CAPACITY
static UINT64 kflow_chk_interval = 0; //chk timer interval (ms)

static KFLOW_AI_ENGINE_CTX* kflow_engine_list[MAX_ENGINE_ID] = {0};

extern UINT32 g_ai_support_net_max;
#if defined (__LINUX)
#include <linux/vmalloc.h>
#include "linux/kflow_ai_net_proc.h"
#define mem_alloc	vmalloc
#define mem_free	vfree
#else
#include "rtos/kflow_ai_net_proc.h"
#include <malloc.h>
#define mem_alloc	malloc
#define mem_free	free
#endif

static KFLOW_AI_NET *g_kflow_net;
static AI_PROC_QUEUE *g_kflow_queue;
static SEM_HANDLE *g_kflow_queue_SEM_ID;
static SEM_HANDLE *g_kflow_data_SEM_ID;
static SEM_HANDLE *g_kflow_fifo_SEM_ID;

#if defined(_BSP_NA51055_)
UINT32 kflow_chip_old = 0;
#endif

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
#include "comm/timer.h"

static volatile UINT32 kflow_ai_core_chk_timer = {0};
static void kflow_ai_core_chk_callback(UINT32 nouse);
#else
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>

static struct hrtimer kflow_ai_core_chk_timer = {0};
static ktime_t kt_periode = {0};
static enum hrtimer_restart kflow_ai_core_chk_callback(struct hrtimer * timer);
#endif

static char kflow_ai_perf_level = 0;
static UINT32 kflow_perf_count = 0;
static UINT32 kflow_perf_sample = 1;
static int (*kflow_perf_dump)(const char *fmt, ...) = 0;


#define CLEAR_OUTBUF_BEFORE_TRIG 0

#if (CLEAR_OUTBUF_BEFORE_TRIG == 1)
#define MAX_BUF_NUM 1000
BOOL outbuf_is_clear[MAX_BUF_NUM] = {0};
#endif

#include "kflow_ai_net_int.h"
#include "kdrv_ai.h"
extern BOOL g_ai_net_path_open[AI_SUPPORT_NET_MAX];
extern BOOL g_ai_net_path_is_used[AI_SUPPORT_NET_MAX];
extern BOOL g_ai_net_path_need_reset[AI_SUPPORT_NET_MAX];
extern SEM_HANDLE g_kflow_net_sem_id;

KFLOW_AI_NET* kflow_ai_core_net(UINT32 proc_id)
{
	if (proc_id >= g_ai_support_net_max)
		return 0;
	return g_kflow_net + proc_id;
}

void kflow_ai_core_reset(void)
{
	UINT32 j, i;
	if (kflow_init > 0) {
		DBG_DUMP("kflow_ai_core - init: already init?\r\n");
		DBG_DUMP("kflow_ai_core - reset - begin\r\n");

		DBG_DUMP(" <-1> close timer!\r\n");
#if defined(_BSP_NA51055_)
		if(nvt_get_chip_id() == CHIP_NA51055) {
			kflow_chip_old = 1;
		} else {
			kflow_chip_old = 0;
		}
#endif
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
		if (kflow_ai_core_chk_timer) {
			timer_close(kflow_ai_core_chk_timer);
			kflow_ai_core_chk_timer = 0;
		}
#else
        hrtimer_cancel(& kflow_ai_core_chk_timer);
#endif
        //kflow_ai_perf_level = 0;
        //kflow_perf_count = 0;
        //kflow_perf_sample = 1;
        //kflow_perf_dump = 0;

		j = kflow_ai_core_tsk_get_cnt();
		if (j > 0) {
			DBG_DUMP(" <0> drop core task!\r\n");
			kflow_ai_core_tsk_reset();
		}
		// stop / close
		for (i = 0; i < MAX_ENGINE_ID; i++) {
			KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
			if (!p_eng) continue;
			for (j=0; j<p_eng->channel_count; j++) {
				KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
				if (p_ch->state == 2) { //start
					//force stop
					DBG_DUMP(" <1> engine[%s] ch[%s] force stop\r\n", p_eng->name, p_ch->name);
					if (p_ch->reset != 0) {
						p_ch->reset(p_ch);
					}
				}
				if (p_ch->state == 1) { //open
					DBG_DUMP(" <1> engine[%s] ch[%s] force close\r\n", p_eng->name, p_ch->name);
					// force close
					if (p_ch->close != 0) {
						p_ch->close(p_ch);
					}
				}
				p_ch->state = 0; //init
			}
			if (p_eng->state >= 1) { //init
				DBG_DUMP(" <1> engine[%s] force uninit\r\n", p_eng->name);
				//force uninit
				if (p_eng->uninit != 0) {
					p_eng->uninit(p_eng);
				}
				p_eng->state = 0;
			}
		}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#else
		while (still_wait_cpu == 1) { 
			vos_util_delay_ms(1);
			continue;
		}
#endif
		// clear graph
		for (i = 0; i < g_ai_support_net_max; i++) {
			KFLOW_AI_NET* p_net = kflow_ai_core_net(i);
			if (p_net == NULL) {
				DBG_ERR("NULL p_net with idx(%u)\r\n", i);
				return;
			}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			if (p_net->addr) {
				DBG_DUMP(" <2> proc[%d] clear graph and kill task\r\n", i);
				p_net->pool = 0; //NOTE: pool is already force reset by nvtmpp! ignore it.
				kflow_ai_net_destory(p_net);
			}
#else
			if (p_net->job) {
				DBG_DUMP(" <2> proc[%d] clear graph and kill task\r\n", i);
				p_net->pool = 0; //NOTE: pool is already force reset by nvtmpp! ignore it.
				kflow_ai_net_destory(p_net);
			}
#endif
		}
		DBG_DUMP(" <3> remove all engines\r\n");
		kflow_ai_core_reset_engine();
		// clear context
		DBG_DUMP(" <4> clear all proc context\r\n");
		for (i=0; i < g_ai_support_net_max; i++) {
			memset(&(g_kflow_net[i]), 0, sizeof(KFLOW_AI_NET));
			g_kflow_net[i].proc_id = i;
		}
		// uninit	(with force reset)
		j = kflow_ai_core_tsk_get_cnt();
		if (j > 0) {
			DBG_DUMP(" <5> kill core task!\r\n");
			for (i=0; i < j; i++) {
				kflow_ai_core_tsk_close();
			}
		}
		DBG_DUMP(" <6> kill core sem!\r\n");
		for (i = 0; i < g_ai_support_net_max; i++) {
			SEM_DESTROY(g_kflow_data_SEM_ID[i]);
			SEM_DESTROY(g_kflow_queue_SEM_ID[i]);
		}
		SEM_DESTROY(g_kflow_fifo_SEM_ID[0]);

		DBG_DUMP(" <-1> free mem!\r\n");
		if (g_kflow_fifo_SEM_ID) {
			mem_free(g_kflow_fifo_SEM_ID);
		}
		if (g_kflow_data_SEM_ID) {
			mem_free(g_kflow_data_SEM_ID);
		}
		if (g_kflow_queue_SEM_ID) {
			mem_free(g_kflow_queue_SEM_ID);
		}
		if (g_kflow_queue) {
			mem_free(g_kflow_queue);
		}
		if (g_kflow_net) {
			mem_free(g_kflow_net);
		}
		
		//DBG_DUMP(" <flag> dump - begin!\r\n");
		//vos_flag_dump(printk);
		//DBG_DUMP(" <flag> dump - end!\r\n");
		//DBG_DUMP(" <sem> dump - begin!\r\n");
		//vos_sem_dump(printk, 1);
		//DBG_DUMP(" <sem> dump - end!\r\n");
		
		DBG_DUMP("kflow_ai_core - reset - end\r\n");
		KLOG_SET(0);
		kflow_init = 0;
#if (CLEAR_OUTBUF_BEFORE_TRIG == 1)
		for (i = 0; i < MAX_BUF_NUM; i++) {
			outbuf_is_clear[i] = 0;
		}
#endif
	}
}

void kflow_ai_core_reset_path(UINT32 net_id)
{

	KFLOW_AI_NET* p_net = kflow_ai_core_net(net_id);

	if (p_net == NULL) {
		DBG_ERR("NULL p_net with idx(%u)\r\n", net_id);
		return;
	}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	if (p_net->addr) {
		DBG_DUMP(" <1> proc[%d] clear graph and kill task\r\n", net_id);
		p_net->pool = 0; //NOTE: pool is already force reset by nvtmpp! ignore it.
		kflow_ai_net_destory(p_net);
	}
#else
	if (p_net->job) {
		DBG_DUMP(" <1> proc[%d] clear graph and kill task\r\n", net_id);
		p_net->pool = 0; //NOTE: pool is already force reset by nvtmpp! ignore it.
		kflow_ai_net_destory(p_net);
	}
#endif

	SEM_DESTROY(g_kflow_data_SEM_ID[net_id]);
	SEM_DESTROY(g_kflow_queue_SEM_ID[net_id]);
	//SEM_DESTROY(g_kflow_fifo_SEM_ID[0]);
	memset(g_kflow_data_SEM_ID + net_id, 0x0, sizeof(SEM_HANDLE));
	memset(g_kflow_queue_SEM_ID + net_id, 0x0, sizeof(SEM_HANDLE));
	OS_CONFIG_SEMPHORE(g_kflow_data_SEM_ID[net_id], 0, 1, 1);
	OS_CONFIG_SEMPHORE(g_kflow_queue_SEM_ID[net_id], 0, 1, 1);
	
	memset(g_kflow_queue + net_id, 0x0, sizeof(AI_PROC_QUEUE));

	memset(g_kflow_net + net_id, 0x0, sizeof(KFLOW_AI_NET));

	g_kflow_net[net_id].proc_id = net_id;

	return;
}

ER kflow_ai_core_init(void)
{
	UINT32 j, i;

	KLOG_OUT(KLOG_STAGE, "kflow_ai_core - init - begin\r\n");
	
	//DBG_DUMP(" <+1> alloc mem!\r\n");
	g_kflow_net = (KFLOW_AI_NET *)mem_alloc(sizeof(KFLOW_AI_NET) * g_ai_support_net_max);
	if (g_kflow_net == NULL) {
		return E_NOMEM;
	}
	memset(g_kflow_net, 0x0, sizeof(KFLOW_AI_NET) * g_ai_support_net_max);

	g_kflow_queue = (AI_PROC_QUEUE *)mem_alloc(sizeof(AI_PROC_QUEUE) * g_ai_support_net_max);
	if (g_kflow_queue == NULL) {
		return E_NOMEM;
	}
	memset(g_kflow_queue, 0x0, sizeof(AI_PROC_QUEUE) * g_ai_support_net_max);

	g_kflow_queue_SEM_ID = (SEM_HANDLE *)mem_alloc(sizeof(SEM_HANDLE) * g_ai_support_net_max);
	if (g_kflow_queue_SEM_ID == NULL) {
		return E_NOMEM;
	}
	memset(g_kflow_queue_SEM_ID, 0x0, sizeof(SEM_HANDLE) * g_ai_support_net_max);

	g_kflow_data_SEM_ID = (SEM_HANDLE *)mem_alloc(sizeof(SEM_HANDLE) * g_ai_support_net_max);
	if (g_kflow_data_SEM_ID == NULL) {
		return E_NOMEM;
	}
	memset(g_kflow_data_SEM_ID, 0x0, sizeof(SEM_HANDLE) * g_ai_support_net_max);

	g_kflow_fifo_SEM_ID = (SEM_HANDLE *)mem_alloc(sizeof(SEM_HANDLE) * g_ai_support_net_max);
	if (g_kflow_fifo_SEM_ID == NULL) {
		return E_NOMEM;
	}
	memset(g_kflow_fifo_SEM_ID, 0x0, sizeof(SEM_HANDLE) * g_ai_support_net_max);

	OS_CONFIG_SEMPHORE(g_kflow_fifo_SEM_ID[0], 0, 1, 1);
	for (i = 0; i < g_ai_support_net_max; i++) {
		OS_CONFIG_SEMPHORE(g_kflow_data_SEM_ID[i], 0, 1, 1);
		OS_CONFIG_SEMPHORE(g_kflow_queue_SEM_ID[i], 0, 1, 1);
	}
	kflow_ai_core_tsk_open();
	for (i = 0; i < g_ai_support_net_max; i++) {
		g_kflow_net[i].proc_id = i;
	}
	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		if (p_eng->init != 0) {
			p_eng->geteng = kflow_ai_core_get_engine;
			p_eng->init(p_eng);
			p_eng->state = 1; //init
		}
		for (j=0; j < p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
			if (p_ch->state != 0) {
				DBG_ERR("- engine[%s] ch[%s] state != 0?\r\n", p_eng->name, p_ch->name);
			}
			if (p_ch->open != 0) {
				p_ch->open(p_ch);
			}
			
			p_ch->ts_perf_begin = 0;
			p_ch->ts_perf_end = 0;
			p_ch->ts_exec = 0;
			p_ch->ts_exec_hw = 0;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
			p_ch->ts_exec_macc = 0;
#endif			
			p_ch->ts_exec_ideal = 0;
			
			p_ch->state = 1; //open
			p_ch->onfinish = kflow_ai_core_onfinish_job; //hook
#if (LOAD_BALANCE == 1)
		    p_ch->ts_prev_end = _nvt_ai_get_counter();
		    p_ch->ts_ready_load = 0;
#endif
		}
	}

#if defined(_BSP_NA51055_)
	if(nvt_get_chip_id() == CHIP_NA51055) {
		kflow_chip_old = 1;
	} else {
		kflow_chip_old = 0;
	}
#endif
	//DBG_DUMP(" <+1> open timer!\r\n");
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
	if (timer_open((TIMER_ID *)&kflow_ai_core_chk_timer, kflow_ai_core_chk_callback) != E_OK) {
		DBG_ERR("kflow_ai_core_chk_timer init fail!\r\n");
		return E_SYS;
	}
	if (kflow_chk_interval == 0) {
		kflow_chk_interval = 1000;
	}
	
	timer_cfg(kflow_ai_core_chk_timer, kflow_chk_interval *1000, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
#else
	if (kflow_chk_interval == 0) {
		kflow_chk_interval = 1000;
	}
   
    hrtimer_init (& kflow_ai_core_chk_timer, CLOCK_REALTIME, HRTIMER_MODE_REL);
    kflow_ai_core_chk_timer.function = kflow_ai_core_chk_callback;
    kt_periode = ktime_set((kflow_chk_interval*1000)/1000000, ((kflow_chk_interval*1000)%1000000)*1000); //seconds,nanoseconds
    hrtimer_start(& kflow_ai_core_chk_timer, kt_periode, HRTIMER_MODE_REL);
#endif

	KLOG_OUT(KLOG_STAGE, "kflow_ai_core - init - end\r\n");

	kflow_init = 1;
	return E_OK;
}
void kflow_ai_core_cfgschd(UINT32 schd)
{
	kflow_schd = schd;
}
void kflow_ai_core_cfgchk(UINT32 chk_interval)
{
	kflow_chk_interval = chk_interval;
}

static void kflow_ai_core_reset_perf_ut(void);

void kflow_ai_perf(UINT32 func, UINT32 sample)
{
	//unsigned long flags;

	kflow_ai_perf_level = func;
    kflow_perf_sample = sample;
    kflow_perf_dump = 0;
	if (kflow_ai_perf_level == 3) {
		kflow_ai_core_reset_perf_ut();
	}		
}
ER kflow_ai_core_uninit(void)
{
	UINT32 j, i;

	KLOG_OUT(KLOG_STAGE, "kflow_ai - uninit - begin\r\n");

	//DBG_DUMP(" <-1> close timer!\r\n");
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
	if (kflow_ai_core_chk_timer) {
		timer_close(kflow_ai_core_chk_timer);
		kflow_ai_core_chk_timer = 0;
	}
#else
    hrtimer_cancel(& kflow_ai_core_chk_timer);
#endif

	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		for (j=0; j<p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
			if (p_ch->state != 1) {
				DBG_ERR("- engine[%s] ch[%s] state != 1?\r\n", p_eng->name, p_ch->name);
			}
			if (p_ch->close != 0) {
				p_ch->close(p_ch);
			}
			p_ch->state = 0; //init
		}
		if (p_eng->uninit != 0) {
			p_eng->uninit(p_eng);
			p_eng->state = 0; //n/a
		}
	}

	kflow_ai_core_tsk_close();
	for (i = 0; i < g_ai_support_net_max; i++) {
		SEM_DESTROY(g_kflow_data_SEM_ID[i]);
		SEM_DESTROY(g_kflow_queue_SEM_ID[i]);
	}
	SEM_DESTROY(g_kflow_fifo_SEM_ID[0]);
	
	//DBG_DUMP(" <-1> free mem!\r\n");
	if (g_kflow_fifo_SEM_ID) {
		mem_free(g_kflow_fifo_SEM_ID);
	}
	if (g_kflow_data_SEM_ID) {
		mem_free(g_kflow_data_SEM_ID);
	}
	if (g_kflow_queue_SEM_ID) {
		mem_free(g_kflow_queue_SEM_ID);
	}
	if (g_kflow_queue) {
		mem_free(g_kflow_queue);
	}
	if (g_kflow_net) {
		mem_free(g_kflow_net);
	}

	KLOG_OUT(KLOG_STAGE, "kflow_ai - init - end\r\n");
	
	kflow_init = 0;
#if (CLEAR_OUTBUF_BEFORE_TRIG == 1)
	for (i = 0; i < MAX_BUF_NUM; i++) {
		outbuf_is_clear[i] = 0;
	}
#endif
	return E_OK;
}

void kflow_ai_core_reset_engine(void)
{
	int i;
	for (i = 0; i < MAX_ENGINE_ID; i++)
		kflow_engine_list[i] = 0;
}

void kflow_ai_core_add_engine(UINT32 engine_id, KFLOW_AI_ENGINE_CTX* p_eng)
{
	if (engine_id >= MAX_ENGINE_ID) {
		return;
	}
	
	if (p_eng == NULL) {
		return;
	}
	
	kflow_engine_list[engine_id] = p_eng;
}

UINT32 kflow_ai_core_get_engine_cnt(void)
{
	return MAX_ENGINE_ID;
}

KFLOW_AI_ENGINE_CTX* kflow_ai_core_get_engine(UINT32 engine_id)
{
	return kflow_engine_list[engine_id];
}

UINT32 kflow_ai_core_get_channel_cnt(KFLOW_AI_ENGINE_CTX* p_eng)
{
	if (p_eng == NULL) {
		return 0;
	}
	
	return p_eng->channel_count;
}

KFLOW_AI_CHANNEL_CTX* kflow_ai_core_get_channel(KFLOW_AI_ENGINE_CTX* p_eng, UINT32 channel_id)
{
	if (p_eng == NULL) {
        return 0;
	}
	
	return p_eng->p_ch[channel_id];
}

void kflow_ai_core_clr_bind(KFLOW_AI_NET* p_net, KFLOW_AI_BIND* p_bind)
{
	if (p_net == NULL) {
		return;
	}
	if (p_bind == NULL) {
		return;
	}
		
    //bind
	p_bind->p_job = 0;
	INIT_LIST_HEAD(&p_bind->list);
}

void kflow_ai_core_clr_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job)
{
	if (p_net == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
		
    //op
	p_job->engine_id = 0; //will be assign in kflow_ai_core_set_job()
	p_job->engine_op = 0; //will be assign in kflow_ai_core_set_job()
	p_job->p_op_info = 0; //will be assign in kflow_ai_core_set_job()
	p_job->p_io_info = 0; //will be assign in kflow_ai_core_set_job()
	p_job->wait_ms = -1; //will be assign in kflow_ai_core_set_job()
	p_job->debug_func = p_net->debug_func;
	p_job->p_eng = 0; //will be assign in ioctl VENDOR_AIS_FLOW_IOC_SET_JOB

    //bind
	p_job->parent_cnt = 0;
	p_job->child_cnt = 0;
	INIT_LIST_HEAD(&p_job->parent_list);
	INIT_LIST_HEAD(&p_job->child_list);

    //runtime
	p_job->state = 0; //free
	p_job->wait_cnt = 0;
	p_job->exec_cnt = 0;
	INIT_LIST_HEAD(&p_job->list);

	//engine dispatch channel
	p_job->p_ch = 0; //will be assign in kflow_ai_core_run_job()
}

//void kflow_ai_core_set_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job, UINT32 engine_id, UINT32 engine_op, void* p_op_info, void* p_io_info, INT32 wait_ms)
void kflow_ai_core_set_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job, void* p_op_info, void* p_io_info, INT32 wait_ms)
{
#if defined(_BSP_NA51090_) ||defined(_BSP_NA51102_) || defined(_BSP_NA51103_)	
	NN_GEN_MODE_CTRL *p_mctrl;
#endif
	if (p_net == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
		
    //op
	//p_job->engine_id = engine_id;
	//p_job->engine_op = engine_op;
	p_job->p_op_info = p_op_info;
	p_job->p_io_info = p_io_info;
	//p_job->schd_parm = schd_parm;
	if (wait_ms <= 0) {
		p_job->wait_ms = wait_ms;
	} else {
		p_job->wait_ms = wait_ms * 1000; //actually in us
	}

	//get ideal time of job
	p_job->ts_exec_predict = p_job->p_eng->getcfg(p_job->p_eng, p_job, 0); 
	if (kflow_ai_perf_level > 0 )	{
		p_job->ts_exec_ideal_cycle = p_job->p_eng->getcfg(p_job->p_eng, p_job, 1); 
	}
	
	//runtime
	p_job->wait_cnt = 0;
#if defined(_BSP_NA51090_) ||defined(_BSP_NA51102_) || defined(_BSP_NA51103_)	
	//set llcmd buffer pa
	p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
	if (p_mctrl->trig_src == NN_GEN_TRIG_LL_AI_DRV) {
		p_job->parm_addr= nvt_ai_va2pa(((KDRV_AI_LL_HEAD *)(p_mctrl->addr))->parm_addr);
	}
#endif
	KLOG_OUT(KLOG_SETUP, "proc[%d].job[%d] setup - engine_id=%d engine_op=%d p_op_info=%08x p_io_info=%08x wait_ms=%d (wait_cnt=%d)\r\n",
		(int)p_job->proc_id, (int)p_job->job_id,
		(int)p_job->engine_id, (int)p_job->engine_op,
		(int)p_job->p_op_info, (int)p_job->p_io_info,
		(int)p_job->wait_ms, (int)p_job->wait_cnt);

#if NN_DLI
	{
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[p_job->engine_id];
		KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[0];
		kflow_ai_core_start_job(p_ch, p_job);
	}
#endif

}

void kflow_ai_core_bind_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job, KFLOW_AI_JOB* p_next_job)
{
    KFLOW_AI_BIND* p_child_bind = 0;
    KFLOW_AI_BIND* p_parent_bind = 0;

	if (p_net == NULL) {
		return;
	}
	if ((p_job == NULL) || (p_next_job == NULL)) {
		return;
	}

    if (list_empty(&p_net->free_bind_list)) {
        DBG_ERR("proc[%d] bind is not enough!\r\n",
			(int)p_job->proc_id);
        return;
    }
    //get a bind from net
	p_child_bind = list_entry(p_net->free_bind_list.next, KFLOW_AI_BIND, list);
	list_del(&p_child_bind->list);
	p_child_bind->list.prev = 0;
	p_child_bind->list.next = 0;
    if (list_empty(&p_net->free_bind_list)) {
        DBG_ERR("proc[%d] bind is not enough!\r\n",
			(int)p_job->proc_id);
		return;
    }
    //get a bind from net
	p_parent_bind = list_entry(p_net->free_bind_list.next, KFLOW_AI_BIND, list);
	list_del(&p_parent_bind->list);
	p_parent_bind->list.prev = 0;
	p_parent_bind->list.next = 0;

	//add p_next_job as child of p_job
	p_child_bind->p_job = p_next_job;
	list_add_tail(&(p_child_bind->list), &p_job->child_list);  //add p_next_job as last sibling
	p_job->child_cnt++;

    KLOG_OUT(KLOG_BIND, "* proc[%d].job[%d] bind to child job[%d] ... (child=%d)\r\n", 
    	(int)p_job->proc_id, (int)p_job->job_id, (int)p_next_job->job_id, (int)p_job->child_cnt);

	//set p_job as parent of p_next_job
	p_parent_bind->p_job = p_job;
	list_add_tail(&(p_parent_bind->list), &p_next_job->parent_list);
	p_next_job->parent_cnt++;

    KLOG_OUT(KLOG_BIND, "* proc[%d].job[%d] bind to parent job[%d] ... (parent=%d)\r\n", 
    	(int)p_job->proc_id, (int)p_next_job->job_id, (int)p_job->job_id, (int)p_next_job->parent_cnt);

    return;
}

void kflow_ai_core_sum_job(KFLOW_AI_NET* p_net, UINT32* src_count, UINT32* dest_count)
{
	UINT32 i;
	UINT32 s = 0;
	UINT32 d = 0;
	if (p_net == NULL) {
		return;
	}

	if ((src_count == NULL) || (dest_count == NULL)) {
		return;
	}

	for (i = 0; i < p_net->job_cnt; i++) {
		KFLOW_AI_JOB* p_job = &(p_net->job[i]);
		if (p_job->p_eng != 0) {
			if (p_job->parent_cnt == 0) {
				//DBG_IND("* proc[%d].job[%d] is src job\r\n", int)p_job->proc_id, (int)p_job->job_id);
				s++;
			}
			if (p_job->child_cnt == 0) {
				//DBG_IND("* proc[%d].job[%d] is dest job\r\n", int)p_job->proc_id, (int)p_job->job_id);
				d++;
			}
		}
	}

	//DBG_IND("proc[%d] have %d job, %d src job, %d dest job\r\n", (int)p_net->proc_id, t, s, d);
	p_net->src_cnt = s;
	p_net->dest_cnt = d;
	src_count[0] = s;
	dest_count[0] = d;
}

void kflow_ai_core_push_begin(KFLOW_AI_NET* p_net)
{
	if (p_net == NULL) {
		return;
	}

	if (p_net->proc_id == debug_proc_id) {
		p_net->debug_func = debug_func;
	}

	if (p_net->debug_func & KFLOW_AI_DBG_BIND) {
		kflow_ai_net_dump(p_net, 0); //job bind
		p_net->debug_func &= ~KFLOW_AI_DBG_BIND;
	}
	
	KLOG_OUT(KLOG_PUSH, "kflow_ai_core - push - begin\r\n");
}

void kflow_ai_core_push_end(KFLOW_AI_NET* p_net)
{
	if (p_net == NULL) {
		return;
	}

	KLOG_OUT(KLOG_PUSH, "kflow_ai_core - push - end\r\n");

	//TODO: verify all proc job are done
	//TODO: verify all proc job are not remain in any engine queue
	/*
    {
		unsigned long flags;
    	UINT32 j, i;
    	//for all channels of all engines
    	for (i = 0; i < MAX_ENGINE_ID; i++) {
    		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
			if (!p_eng) continue;
			loc_cpu(flags);
			INIT_LIST_HEAD(p_eng->wait_queue);
    		for (j=0; j<p_eng->channel_count; j++) {
    			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
				INIT_LIST_HEAD(p_ch->ready_queue);
    		}
			unl_cpu(flags);
    	}
    }
    */
}

/*
int show_chk_ready = 0;

if (show_chk_ready) {
	DBG_ERR("- chk before trigger ~~~~ dump\r\n"); 
	DBG_ERR("- proc[%d].job[%d] engine[%s] ch[%s]\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name);
	kflow_ai_engine_dump(p_job->p_eng, 0x02);
}

if (p_ch->state != 1) {
	DBG_ERR("- proc[%d].job[%d] engine[%s] ch[%s] ch state = %d, ch state != 1???\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name, (int)p_ch->state);
	show_chk_ready = 1;
	kflow_ai_engine_dump(p_job->p_eng, 0x02);
	return -1;
}
{
	UINT32 find = 0;
	KFLOW_AI_JOB* p_chk_job = 0;
	LIST_HEAD *p_this; //current entry
	LIST_HEAD *p_temp; //temp entry
	list_for_each_safe(p_this, p_temp, p_ch->ready_queue) {
		p_chk_job = list_entry(p_this, KFLOW_AI_JOB, list);
		if (p_chk_job == p_job) { find = 1; break; }
	}
	if (!find) {
		DBG_ERR("- proc[%d].job[%d] engine[%s] ch[%s] state=%d: before trigger but NOT in ready queue???\r\n", 
			(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name, (int)p_job->state);
		kflow_ai_engine_dump(p_job->p_eng, 0x02);
		show_chk_ready = 1;
	}
}
*/

#if NN_DLI
void kflow_ai_core_start_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
	if (p_ch == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	
	if (p_ch->start != 0) {
		p_ch->start(p_ch, p_job); 
		//if non-DLA job, it will call this to dynamice create job context 
	}
}

void kflow_ai_core_stop_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
	if (p_ch == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	
	if (p_ch->stop != 0) {
		p_ch->stop(p_ch, p_job); 
		//if non-DLA job, it will call this to dynamice destory job context 
	}
}
#endif

void kflow_ai_core_run_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
	if (p_ch == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	
	KLOG_OUT(KLOG_RUN, "---- proc[%d].job[%d] trigger to engine[%s] ch[%s]\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name);

	p_ch->state = 2; //start
	p_ch->run_job = p_job;
	p_job->p_ch = p_ch; //assign current ch

	p_job->state = 3; //exec
	p_job->ts_exec_begin = _nvt_ai_get_counter();
	p_job->ts_ready_end = p_job->ts_exec_begin;
#if (LOAD_BALANCE == 1)
	p_ch->ts_ready_load -= p_job->ts_exec_predict;
	p_ch->ts_this_begin = p_job->ts_exec_begin;
	if ((int)p_ch->ts_ready_load < (int)0 ) {
		//DBG_IND("p[%d].j[%d] e[%s] ch[%s] ld=%d - %d (?????)\r\n", 
		//	(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name, p_ch->ts_ready_load, p_job->ts_exec_predict);
		DBG_WRN(" - proc[%d].job[%d] engine[%s] ch[%s] invalid ready_load? (%d)\r\n", 
			(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name, p_ch->ts_ready_load);
		p_ch->ts_ready_load = 0;
	}
#endif
	p_job->exec_cnt = 0;
}

void kflow_ai_core_trig_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
	if (p_ch == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	
	if (p_ch->trigger != 0) {
		p_ch->trigger(p_ch, p_job); 
		//if non-DLA job, it will call p_ch->onfinish() inside, 
		// 1. p_job will be done, remove from ready_queue
		// 2. will dispatch to any first job in ready queue of all engine's all channel
		// 3. will dispatch to all next job in wait queue of all engine's, to some of its channel

	}
}

static UINT32 _kflow_ai_core_div64(UINT64 dividend, UINT64 divisor)
{
	if (divisor == 0) {
		return 0;
	}
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return (UINT32)(dividend / divisor);
#else
	do_div(dividend, divisor);
	return (UINT32)(dividend);
#endif
}

#define PERF_MM_SIZE	128
#define PERF_LOG_SIZE	1024
static char _ai_perf_mm[PERF_MM_SIZE] ={0};
static char _ai_perf_log[PERF_LOG_SIZE] = {0};

void  kflow_ai_core_chk_timeout(KFLOW_AI_ENGINE_CTX* p_eng)
{
	UINT32 j;
	unsigned long flags;

	if (p_eng == NULL) {
		return;
	}

	loc_cpu(flags);

	for (j=0; j<p_eng->channel_count; j++) {
		KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
		
		if ((kflow_ai_perf_level > 0 ) && (p_ch->state > 0)) {
			p_ch->ts_perf_end = _nvt_ai_get_counter();
			if ((p_ch->ts_perf_begin > 0) && (p_ch->ts_perf_end > p_ch->ts_perf_begin)) {
				UINT32 dt = (INT32)(p_ch->ts_perf_end - p_ch->ts_perf_begin);
				UINT32 bv, kv;
				UINT32 ut, ut_i, ut_f;
				UINT32 ut_hw, ut_hw_i, ut_hw_f;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
				UINT32 ut_macc, ut_macc_i, ut_macc_f;
#endif
				UINT32 ut_ideal, ut_ideal_i, ut_ideal_f;
				kv =0; bv = 0;
				if (strcmp(p_eng->name, "cnn") == 0) {kv = CNN_ENG_EV; bv = CNN_ENG_EV;}  //kv=bv
				if (strcmp(p_eng->name, "nue") == 0) {kv = NUE_ENG_EV; bv = NUE_ENG_EV;}  //kv=bv
				if (strcmp(p_eng->name, "nue2") == 0) {kv = NUE2_ENG_EV; bv = NUE2_ENG_EV;}  //kv=bv
				if (strcmp(p_eng->name, "cpu") == 0) {kv = 0; bv = 1;}
				if (strcmp(p_eng->name, "dsp") == 0) {kv = 0; bv = 0;}
				if ((bv > 1) && (kflow_ai_perf_level > 1 )) {
					//DBG_DUMP(" (%u %u %u) %u %u\r\n", p_ch->ts_exec, p_ch->ts_exec_hw, p_ch->ts_exec_ideal, bv, dt);
					ut = _kflow_ai_core_div64(((UINT64)p_ch->ts_exec)*10000, (UINT64)dt);
					ut_hw = _kflow_ai_core_div64(((UINT64)p_ch->ts_exec_hw)*1000, ((UINT64)dt)*kv);
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
					ut_macc = _kflow_ai_core_div64(((UINT64)p_ch->ts_exec_macc)*1000, ((UINT64)dt)*kv);
#endif
					ut_ideal = _kflow_ai_core_div64(((UINT64)p_ch->ts_exec_ideal)*1000, ((UINT64)dt)*bv);
					ut_i = ut/100; ut_f = ut - ut_i*100;
					ut_hw_i = ut_hw/100; ut_hw_f = ut_hw - ut_hw_i*100;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
					ut_macc_i = ut_macc/100; ut_macc_f = ut_macc - ut_macc_i*100;
#endif
					ut_ideal_i = ut_ideal/100; ut_ideal_f = ut_ideal - ut_ideal_i*100;
                    if (kflow_perf_dump) {
                        p_ch->last_ut.ut_i = ut_i;
                        p_ch->last_ut.ut_f = ut_f;
                        p_ch->last_ut.ut_hw_i = ut_hw_i;
                        p_ch->last_ut.ut_hw_f = ut_hw_f;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                        p_ch->last_ut.ut_macc_i = ut_macc_i;
                        p_ch->last_ut.ut_macc_f = ut_macc_f;
#endif
                        p_ch->last_ut.ut_ideal_i = ut_ideal_i;
                        p_ch->last_ut.ut_ideal_f = ut_ideal_f;
                    } else {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
					snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u%%,%3u.%02u%%,%3u.%02u%%,%3u.%02u%%", 
#else
					snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u%%,%3u.%02u%%,%3u.%02u%%", 
#endif
						p_ch->name, 
						(unsigned int)ut_i, (unsigned int)ut_f, 
						(unsigned int)ut_hw_i, (unsigned int)ut_hw_f, 
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
						(unsigned int)ut_macc_i, (unsigned int)ut_macc_f, 
#endif
						(unsigned int)ut_ideal_i, (unsigned int)ut_ideal_f);
					strncat(_ai_perf_log, _ai_perf_mm, PERF_LOG_SIZE - strlen(_ai_perf_log));
                    }
				} else if ((bv > 1) && (kflow_ai_perf_level > 0 )) {
					//DBG_DUMP(" (%u %u %u) %u %u\r\n", p_ch->ts_exec, p_ch->ts_exec_hw, p_ch->ts_exec_ideal, bv, dt);
					ut = _kflow_ai_core_div64(((UINT64)p_ch->ts_exec)*10000, (UINT64)dt);
					ut_i = ut/100; ut_f = ut - ut_i*100;
                    if (kflow_perf_dump) {
                        p_ch->last_ut.ut_i = ut_i;
                        p_ch->last_ut.ut_f = ut_f;
                        p_ch->last_ut.ut_hw_i = 0;
                        p_ch->last_ut.ut_hw_f = 0;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                        p_ch->last_ut.ut_macc_i = 0;
                        p_ch->last_ut.ut_macc_f = 0;
#endif
                        p_ch->last_ut.ut_ideal_i = 0;
                        p_ch->last_ut.ut_ideal_f = 0;
                    } else {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
					snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u%%", 
#else
					snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u%%", 
#endif
						p_ch->name, 
						(unsigned int)ut_i, (unsigned int)ut_f);
					strncat(_ai_perf_log, _ai_perf_mm, PERF_LOG_SIZE - strlen(_ai_perf_log));
                    }
				} else if ((bv > 0) && (kflow_ai_perf_level <= 2 )) {
					ut = _kflow_ai_core_div64(((UINT64)p_ch->ts_exec)*100, (UINT64)dt);
					ut_i = ut/100; ut_f = ut - ut_i*100;
                    if (kflow_perf_dump) {
                        p_ch->last_ut.ut_i = ut_i;
                        p_ch->last_ut.ut_f = ut_f;
                        p_ch->last_ut.ut_hw_i = 0;
                        p_ch->last_ut.ut_hw_f = 0;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                        p_ch->last_ut.ut_macc_i = 0;
                        p_ch->last_ut.ut_macc_f = 0;
#endif
                        p_ch->last_ut.ut_ideal_i = 0;
                        p_ch->last_ut.ut_ideal_f = 0;
                    } else {
					snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u%%", 
						p_ch->name, 
						(unsigned int)ut_i, (unsigned int)ut_f);
					strncat(_ai_perf_log, _ai_perf_mm, PERF_LOG_SIZE - strlen(_ai_perf_log));
                    }
				}
			}
			p_ch->ts_perf_begin = p_ch->ts_perf_end;
			p_ch->ts_exec = 0;
			p_ch->ts_exec_hw = 0;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
			p_ch->ts_exec_macc = 0;
#endif
			p_ch->ts_exec_ideal = 0;
		}
		
		//check busy
		if (p_ch->run_job != 0) {
			KFLOW_AI_JOB* run_job = p_ch->run_job;
			if ((run_job->wait_ms > 0) && (p_ch->reset2)) { //enable timeout
				UINT32 ts_current = _nvt_ai_get_counter();
				UINT32 ts_this_end =  p_ch->ts_this_begin + run_job->ts_exec_predict;
				UINT32 ts_timeout = run_job->wait_ms;
				//DBG_ERR(" - proc[%d].job[%d] engine[%s] ch[%s] check (%d > %d)....\r\n", 
				//	(int)run_job->proc_id, (int)run_job->job_id, run_job->p_eng->name, p_ch->name, 
				//	(int)ts_current-(int)ts_this_end, (int)ts_timeout);
				if ((int)ts_current-(int)ts_this_end > (int)ts_timeout) {
					//DBG_ERR("- proc[%d].job[%d] engine[%s] ch[%s] timeout (%d - %d = %d > 500000)!\r\n", 
					DBG_ERR(" - proc[%d].job[%d] engine[%s] ch[%s] timeout (%d > %d)!\r\n", 
						(int)run_job->proc_id, (int)run_job->job_id, run_job->p_eng->name, p_ch->name, 
						//ts_current, ts_this_end, (int)ts_current-(int)ts_this_end);
						(int)ts_current-(int)ts_this_end, (int)ts_timeout);
					//DBG_IND("- do engine reset!\r\n");
					unl_cpu(flags);
					if (p_ch->reset2) {
						p_ch->reset2(p_ch);
					}
					loc_cpu(flags);
				}	
			}
		}

	}
	unl_cpu(flags);

}

void kflow_ai_cat_perf(UINT32 func, int (*dump)(const char *fmt, ...))
{
	//unsigned long flags;

	kflow_ai_perf_level = func;
    kflow_perf_dump = dump;
	if (kflow_ai_perf_level == 3) {
		kflow_ai_core_reset_perf_ut();
	}

    MSLEEP((kflow_perf_sample*1000)+1000);
    
    {
    UINT32 i;
    
    if (kflow_ai_perf_level == 1 )  {snprintf(_ai_perf_log, PERF_LOG_SIZE, " perf-ut:");}
    if (kflow_ai_perf_level == 2 )  {snprintf(_ai_perf_log, PERF_LOG_SIZE, " perf-all:");}
    for (i = 0; i < MAX_ENGINE_ID; i++) {
        KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
        UINT32 j;
        unsigned long flags;
        if (!p_eng) continue;
        
        loc_cpu(flags);
        for (j=0; j<p_eng->channel_count; j++) {
            KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
        
            if ((kflow_ai_perf_level > 0 ) && (p_ch->state > 0)) {
                if (kflow_ai_perf_level > 1) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                    //snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u%%,%3u.%02u%%,%3u.%02u%%,%3u.%02u%%",
                    snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u,%3u.%02u,%3u.%02u,%3u.%02u",
#else
                    //snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u%%,%3u.%02u%%,%3u.%02u%%",
                    snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u,%3u.%02u,%3u.%02u",
#endif
                        p_ch->name,
                        (unsigned int)p_ch->last_ut.ut_i, (unsigned int)p_ch->last_ut.ut_f,
                        (unsigned int)p_ch->last_ut.ut_hw_i, (unsigned int)p_ch->last_ut.ut_hw_f,
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                        (unsigned int)p_ch->last_ut.ut_macc_i, (unsigned int)p_ch->last_ut.ut_macc_f,
#endif
                        (unsigned int)p_ch->last_ut.ut_ideal_i, (unsigned int)p_ch->last_ut.ut_ideal_f);
                    strncat(_ai_perf_log, _ai_perf_mm, PERF_LOG_SIZE - strlen(_ai_perf_log));
                } else { // kflow_ai_perf_level == 1
                    //snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u%%",
                    snprintf(_ai_perf_mm, PERF_MM_SIZE, " [%s]%3u.%02u",
                        p_ch->name,
                        (unsigned int)p_ch->last_ut.ut_i, (unsigned int)p_ch->last_ut.ut_f);
                    strncat(_ai_perf_log, _ai_perf_mm, PERF_LOG_SIZE - strlen(_ai_perf_log));
                }
            }
        }
        unl_cpu(flags);
    }

    //printk("_ai_perf_log=%s\r\n", _ai_perf_log);
    if (kflow_ai_perf_level == 1 )  {dump("%s\r\n", _ai_perf_log);}
    if (kflow_ai_perf_level == 2 )  {dump("%s\r\n", _ai_perf_log);}
    kflow_ai_perf_level = 0;
    kflow_perf_dump = 0;
    }
}

static void kflow_ai_core_reset_perf_ut(void)
{
	unsigned long flags;
	UINT32 j, i;
	
	//reset related counter
	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		loc_cpu(flags);
		for (j=0; j < p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
			p_ch->ts_perf_begin = _nvt_ai_get_counter();
			p_ch->ts_perf_end = p_ch->ts_perf_begin;
			p_ch->ts_exec = 0;
			p_ch->ts_exec_hw = 0;
			p_ch->ts_exec_ideal = 0;
			p_ch->lts_exec_all = 0;
			p_ch->lts_exec = 0;
		}
		unl_cpu(flags);
	}
}

void  kflow_ai_core_get_perf_ut(KFLOW_AI_PERF_UT* p_perf_ut)
{
	UINT32 core_id = 0;
	UINT32 i,j;
	unsigned long flags;

	//DBG_DUMP("=> api: getgetget\r\n");
	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		loc_cpu(flags);
		for (j=0; j<p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
			//KFLOW_AI_CORE_UT* p_core_ut =  &(p_perf_ut->core[core_id+1]);
			KFLOW_AI_CORE_UT* p_core_ut =  &(p_perf_ut->core[core_id+1]);
			//kflow_ai_perf_get_ch_ut(p_eng, p_ch, &(p_perf_ut->core[core_id]));
			p_ch->ts_perf_end = _nvt_ai_get_counter();
			if ((p_ch->ts_perf_end > p_ch->ts_perf_begin)) {
				p_ch->lts_exec_all += p_ch->ts_perf_end - p_ch->ts_perf_begin;
			}
			//////////////////////////////////////
			//DBG_DUMP("sum1: [%s] time = %llu(us), util = %u(%%)\r\n", p_ch->name, p_ch->lts_exec, _kflow_ai_core_div64(((UINT64)p_ch->lts_exec)*100, (UINT64)(p_ch->lts_exec_all)));
			strncpy(p_core_ut->name, p_ch->name, 8-1);
			p_core_ut->time = p_ch->lts_exec;
			p_core_ut->util = _kflow_ai_core_div64(((UINT64)p_ch->lts_exec)*10000, (UINT64)(p_ch->lts_exec_all));
			//////////////////////////////////////
			if (core_id == 0) {
				KFLOW_AI_CORE_UT* p_core_ut =  &(p_perf_ut->core[0]);
				strncpy(p_core_ut->name, "total", 8-1);
				p_core_ut->time = p_ch->lts_exec_all;
				p_core_ut->util = 10000;
			}
			core_id ++;
		}
		unl_cpu(flags);
	}
	//p_perf_ut->core_count = core_id;
	p_perf_ut->core_count = 1+core_id;
	//DBG_DUMP("=> api: getgetget2\r\n");
}

void  kflow_ai_core_add_ready(KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job, UINT32 this_eng_attr, unsigned long flags)
{
	UINT32 ts_current;
	UINT32 j;
#if (LOAD_BALANCE == 1)
	UINT32 max_idle = 0;
	KFLOW_AI_CHANNEL_CTX* p_max_idle_ch = 0;
	UINT32 min_busy = 0xffffffff;
	KFLOW_AI_CHANNEL_CTX* p_min_busy_ch = 0;
#endif

	if (p_eng == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	
    p_job->ts_ready_begin = _nvt_ai_get_counter();
    p_job->ts_wait_end = p_job->ts_ready_begin;
	ts_current = _nvt_ai_get_counter();

#if (LOAD_BALANCE == 1)
    {
		//UINT32 max_idle = 0;
		//KFLOW_AI_CHANNEL_CTX* p_max_idle_ch = 0;
		//UINT32 min_busy = 0xffffffff;
		//KFLOW_AI_CHANNEL_CTX* p_min_busy_ch = 0;
		if ((p_job->schd_parm & 0xff) < p_eng->channel_count) {
			/////////////////////////////////////////////////////
			// force select user assign channel!
			/////////////////////////////////////////////////////
    		KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[(p_job->schd_parm & 0xff)];
    		if (list_empty(p_ch->ready_queue)) {
				// this channel is "zero" ready job
				UINT32 this_idle = ts_current - p_ch->ts_prev_end;
				
				KLOG_OUT(KLOG_READY, "---- proc[%d].job[%d] force engine[%s] ch[%s] this_idle=%u\r\n", 
					(int)p_job->proc_id, (int)p_job->job_id, p_eng->name, p_ch->name, this_idle);
				p_max_idle_ch = p_ch;
    		} else {
				// this channel is "least" ready jobs
				UINT32 this_run = (ts_current - p_ch->ts_this_begin);
				UINT32 this_busy;
				if (this_run < p_job->ts_exec_predict) {
					this_busy = p_ch->ts_ready_load + (p_job->ts_exec_predict - this_run);
				} else {
					this_busy = p_ch->ts_ready_load + (0);
				}
				
				KLOG_OUT(KLOG_READY, "--- proc[%d].job[%d] force engine[%s] ch[%s] this_busy=%u\r\n", 
					(int)p_job->proc_id, (int)p_job->job_id, p_eng->name, p_ch->name, this_busy);
				p_min_busy_ch = p_ch;
    		}
		} else {
			/////////////////////////////////////////////////////
			// auto select from engine's all channels!
			/////////////////////////////////////////////////////
	    	for (j=0; j<p_eng->channel_count; j++) {
	    		KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
				// pick 1 channel with "zero" ready job, and with "max-idle-time"
	    		if (list_empty(p_ch->ready_queue)) {
		            UINT32 this_idle = ts_current - p_ch->ts_prev_end;
					
	    			KLOG_OUT(KLOG_READY, "---- proc[%d].job[%d] eval engine[%s] ch[%s] this_idle=%u max_idle=%u\r\n", 
	    				(int)p_job->proc_id, (int)p_job->job_id, p_eng->name, p_ch->name, this_idle, max_idle);

	    			KLOG_OUT(KLOG_READY, "---..... current=%u prev_end=%u\r\n", ts_current, p_ch->ts_prev_end);

	                if (this_idle >= max_idle) { //NOTE: compare with >= here, because this_idle may be 0!
	                    max_idle = this_idle;
	                    p_max_idle_ch = p_ch;
	                }
	    		}
				// pick 1 channel with "least" ready jobs, and with "mini-busy-time"
				if (!list_empty(p_ch->ready_queue)) {
					UINT32 this_run = (ts_current - p_ch->ts_this_begin);
					UINT32 this_busy;
					if (this_run < p_job->ts_exec_predict) {
						this_busy = p_ch->ts_ready_load + (p_job->ts_exec_predict - this_run);
					} else {
						this_busy = p_ch->ts_ready_load + (0);
					}
					
					KLOG_OUT(KLOG_READY, "--- proc[%d].job[%d] eval engine[%s] ch[%s] this_busy=%u min_busy=%u\r\n", 
						(int)p_job->proc_id, (int)p_job->job_id, p_eng->name, p_ch->name, this_busy, min_busy);
					
					KLOG_OUT(KLOG_READY, "---..... ready_load=%u this_ideal=%u current=%u this_begin=%u\r\n", p_ch->ts_ready_load, p_job->ts_exec_predict, ts_current, p_ch->ts_this_begin);

					if (this_busy < min_busy) {
						min_busy = this_busy;
						p_min_busy_ch = p_ch;
					}
				}
	    	}
		}
    	if (p_max_idle_ch != 0) {

			KLOG_OUT(KLOG_READY, "--- proc[%d].job[%d] add to ready of engine[%s] ch[%s]\r\n", 
				(int)p_job->proc_id, (int)p_job->job_id, p_eng->name, p_max_idle_ch->name);

            p_max_idle_ch->ts_ready_load += p_job->ts_exec_predict;

			KLOG_OUT(KLOG_READY, "---..... ready_load=%u (+%u)\r\n", p_max_idle_ch->ts_ready_load, p_job->ts_exec_predict);

			// add job to this channel's ready queue
			p_job->state = 2; //ready
			INIT_LIST_HEAD(&(p_job->list)); //reset for add
			list_add_tail(&(p_job->list), p_max_idle_ch->ready_queue);
			// trigger job as run JOB
			if (p_job->p_eng->attr == 0) { //DLA job
				if (p_job->wait_ms >= 0) { //under async
					kflow_ai_core_run_job(p_max_idle_ch, p_job);
					kflow_ai_core_trig_job(p_max_idle_ch, p_job);
				} else { //under sync
					kflow_ai_core_run_job(p_max_idle_ch, p_job);
					unl_cpu(flags); //unlock to support kdrv wait_done inside trig
					kflow_ai_core_trig_job(p_max_idle_ch, p_job);
					loc_cpu(flags);
				}
			} else {//non-DLA job
				kflow_ai_core_run_job(p_max_idle_ch, p_job);
				unl_cpu(flags); //unlock for trigger callback
				kflow_ai_core_trig_job(p_max_idle_ch, p_job);
				loc_cpu(flags);
			}
			return; //quit
		}
    	if (p_min_busy_ch != 0) {

			KLOG_OUT(KLOG_READY, "--- proc[%d].job[%d] add to ready of engine[%s] ch[%s]\r\n", 
				(int)p_job->proc_id, (int)p_job->job_id, p_eng->name, p_min_busy_ch->name);

            p_min_busy_ch->ts_ready_load += p_job->ts_exec_predict;

			KLOG_OUT(KLOG_READY, "---..... ready_load=%u (+%u)\r\n", p_min_busy_ch->ts_ready_load, p_job->ts_exec_predict);

			// add job to this channel's ready queue
			p_job->state = 2; //ready
			INIT_LIST_HEAD(&(p_job->list)); //reset for add
			list_add_tail(&(p_job->list), p_min_busy_ch->ready_queue);
			return; //quit
		}
    }
#else
	{
		KFLOW_AI_CHANNEL_CTX* p_zero_ch = 0;
		KFLOW_AI_CHANNEL_CTX* p_small_ch = 0;
		// from engine's all channels,
		for (j=0; j<p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
			// pick 1 channel with "zero" ready job
			if (list_empty(p_ch->ready_queue)) {
				p_zero_ch = p_ch;
			}
			// pick 1 channel with "least" ready jobs
			if (!list_empty(p_ch->ready_queue)) {
				p_small_ch = p_ch;
			}
		}
		if (p_zero_ch) {
			// add job to this channel's ready queue
			p_job->state = 2; //ready
			INIT_LIST_HEAD(&(p_job->list)); //reset for add
			list_add_tail(&(p_job->list), p_zero_ch->ready_queue);
			// trigger job as run JOB
			kflow_ai_core_run_job(p_zero_ch, p_job);
			kflow_ai_core_trig_job(p_zero_ch, p_job);
			return; //quit
		}
		if (p_small_ch) {
			// add job to this channel's ready queue
			p_job->state = 2; //ready
			INIT_LIST_HEAD(&(p_job->list)); //reset for add
			list_add_tail(&(p_job->list), p_small_ch->ready_queue);
			return; //quit
		}
	}
#endif

    DBG_ERR("--- proc[%d].job[%d] add to ready of engine[%s] fail!\r\n", 
    	(int)p_job->proc_id, (int)p_job->job_id, p_eng->name);
}

static void kflow_ai_core_del_ready_run(KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job)
{
	KFLOW_AI_CHANNEL_CTX* p_ch;

	if (p_eng == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	

	p_ch = p_job->p_ch;
	// remove job from ready queue
	//DBG_DUMP("--- proc[%d].job[%d] remove from ready of engine[%s]\r\n", 
	//	(int)p_job->proc_id, (int)p_ready_job->job_id, p_eng->name);
	list_del(&(p_job->list));
	//INIT_LIST_HEAD(&(p_ready_job->list));
	//p_ready_job->list.prev = 0;
	//p_ready_job->list.next = 0;
	
	//remove JOB from run
	p_ch->run_job = 0;
	p_ch->state = 1; //open
	p_job->state = 4; //done
}

static void kflow_ai_core_add_wait(KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job)
{
	if (p_eng == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	
	//DBG_DUMP("proc[%d].job[%d] push with wait_cnt=%d\r\n", 
	//	(int)p_job->proc_id, (int)p_job->job_id, (int)p_job->wait_cnt);
    p_job->ts_wait_begin = _nvt_ai_get_counter();

	//add JOB to wait list of related engine
	if (KLOG_IF(KLOG_WAIT)) {
		KLOG_OUT(KLOG_WAIT, "- proc[%d].job[%d] add to wait of engine[%s] - child_cnt=%d parent_cnt=%d\r\n", 
			(int)p_job->proc_id, (int)p_job->job_id, p_eng->name, (int)p_job->child_cnt, (int)p_job->parent_cnt);
		if (p_job->parent_cnt > 0) {
		    LIST_HEAD *p_this; //current entry
		    LIST_HEAD *p_temp; //temp entry
		    KFLOW_AI_BIND* p_bind = 0;
			list_for_each_safe(p_this, p_temp, &p_job->parent_list) {
				p_bind = list_entry(p_this, KFLOW_AI_BIND, list);
		        KLOG_OUT(KLOG_WAIT, "        p:: job[%d]\r\n", (int)p_bind->p_job->job_id);
			}
		}
		if (p_job->child_cnt > 0) {
		    LIST_HEAD *p_this; //current entry
		    LIST_HEAD *p_temp; //temp entry
		    KFLOW_AI_BIND* p_bind = 0;
			list_for_each_safe(p_this, p_temp, &p_job->child_list) {
				p_bind = list_entry(p_this, KFLOW_AI_BIND, list);
		        KLOG_OUT(KLOG_WAIT, "        c:: job[%d]\r\n", (int)p_bind->p_job->job_id);
			}
		}
	}

	p_job->state = 1; //wait
	INIT_LIST_HEAD(&(p_job->list)); //reset for add
	list_add_tail(&(p_job->list), p_eng->wait_queue);
}

static void kflow_ai_core_del_wait(KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job)
{
	if (p_eng == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	
	
	//DBG_DUMP("--- proc[%d].child-job[%d] remove from wait of engine[%s]\r\n", 
	//	(int)p_job->proc_id, (int)p_wait_job->job_id, p_wait_job->p_eng->name);
	list_del(&(p_job->list));
	//INIT_LIST_HEAD(&(p_wait_job->list));
	//p_wait_job->list.prev = 0;
	//p_wait_job->list.next = 0;
}

static void kflow_ai_core_chk_ready(KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job, unsigned long flags)
{
	if (p_eng == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	
	//if this JOB is not waiting to any resource,
	if (p_job->wait_cnt == 0) {
		//remove JOB from wait list
		kflow_ai_core_del_wait(p_job->p_eng, p_job);
		//add JOB to ready queue of related engine

		KLOG_OUT(KLOG_NEXT, "--- proc[%d].job[%d] add to ready of engine[%s]\r\n", 
			(int)p_job->proc_id, (int)p_job->job_id, p_eng->name);

		kflow_ai_core_add_ready(p_job->p_eng, p_job, p_job->p_eng->attr, flags);
	}
}

static void kflow_ai_core_done_job(KFLOW_AI_JOB* p_job)
{
	KFLOW_AI_ENGINE_CTX* p_eng;
	KFLOW_AI_CHANNEL_CTX* p_ch = 0;

	if (p_job == NULL) {
		return;
	}

	p_ch = p_job->p_ch;
	p_eng = p_job->p_eng;

	KLOG_OUT(KLOG_NEXT, "----- proc[%d].job[%d] done, remove from engine[%s] ch[%s] ready queue\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id, p_eng->name, p_ch->name);

	//remove JOB from ready queue and run
	kflow_ai_core_del_ready_run(p_eng, p_job);
}

static void kflow_ai_core_this_job(KFLOW_AI_JOB* p_job, UINT32 this_eng_attr, unsigned long flags)
{
//	KFLOW_AI_ENGINE_CTX* p_eng;
	KFLOW_AI_JOB* p_ready_job = 0;
	KFLOW_AI_JOB* p_trig_job = 0;
	KFLOW_AI_CHANNEL_CTX* p_ch = 0;
	
	if (p_job == NULL) {
		return;
	}
	
	p_ch = p_job->p_ch;

	KLOG_OUT(KLOG_NEXT, "- proc[%d].job[%d] check next job from ready queue\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id);

	//////////////////////////////////////
	//
	// try to trigger this ch!
	//
	//////////////////////////////////////
	if (p_ch->run_job != 0) {
		//if this channel is under busy
		goto trigger_all;
	}
	//if this channel's ready queue is not empty
	p_trig_job = 0;
	if (!list_empty(p_ch->ready_queue)) {
		//pick on early JOB in ready queue
		p_ready_job = list_first_entry(p_ch->ready_queue, KFLOW_AI_JOB, list);
		if (p_ready_job) {
			if (this_eng_attr == p_ready_job->p_eng->attr) {
				p_trig_job = p_ready_job;
				if ((this_eng_attr == 0) && (p_ready_job->wait_ms < 0)) { //DLA job under sync
					p_trig_job = 0; //ignore DLA job under sync (in ISR)
				}
			} else if ((this_eng_attr == 1) && (p_ready_job->wait_ms < 0)) { //any job under sync
				p_trig_job = p_ready_job; //pick any job under sync (in kernal task)
			}
		}
	}
	if (p_trig_job) {

		if (p_trig_job->p_eng->attr == 0) { //DLA job
			//trigger it as a run JOB
			if (p_trig_job->wait_ms >= 0) { //under async
				kflow_ai_core_run_job(p_ch, p_trig_job);
				kflow_ai_core_trig_job(p_ch, p_trig_job);
			} else { //under sync
				kflow_ai_core_run_job(p_ch, p_trig_job);
				unl_cpu(flags); //unlock to support kdrv wait_done inside trig
				kflow_ai_core_trig_job(p_ch, p_trig_job);
				loc_cpu(flags);
			}
		} else { //non-DLA job
			kflow_ai_core_run_job(p_ch, p_trig_job);
			unl_cpu(flags); //unlock to support kdrv wait_done inside trig
			kflow_ai_core_trig_job(p_ch, p_trig_job);
			loc_cpu(flags);
		}
		p_trig_job = 0;
	} else {
	}

trigger_all:
	//////////////////////////////////////
	//
	// try to trigger all engine's all ch!
	//
	//////////////////////////////////////
	{
		KFLOW_AI_CHANNEL_CTX* p_this_ch = p_ch;
		UINT32 j, i;

		//for all channels of all engines
		for (i = 0; i < MAX_ENGINE_ID; i++) {
    		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
			if (!p_eng) continue;
			p_ready_job = 0;
			for (j=0; j<p_eng->channel_count; j++) {
    			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
				if (p_ch == p_this_ch)
					continue;
            	//if this channel is under busy
    			if (p_ch->run_job != 0) {
					continue;
    			}
            	//if this channel's ready queue is not empty
            	if (!list_empty(p_ch->ready_queue)) {
            		//pick on early JOB in ready queue
            		p_ready_job = list_first_entry(p_ch->ready_queue, KFLOW_AI_JOB, list);
            		//trigger it as a run JOB
            		if (this_eng_attr == p_ready_job->p_eng->attr) {
						p_trig_job = p_ready_job;
						if ((this_eng_attr == 0) && (p_ready_job->wait_ms < 0)) { //DLA job under sync
							p_trig_job = 0; //ignore DLA job under sync (in ISR)
						}
					} else if ((this_eng_attr == 1) && (p_ready_job->wait_ms < 0)) { //any job under sync
						p_trig_job = p_ready_job; //pick any job under sync (in kernel task)
					}
				}
				if (p_trig_job) {
					if (p_trig_job->p_eng->attr == 0) { //DLA job
						if (p_trig_job->wait_ms >= 0) { //under async
							kflow_ai_core_run_job(p_ch, p_trig_job);
							kflow_ai_core_trig_job(p_ch, p_trig_job);
						} else { //under sync
							kflow_ai_core_run_job(p_ch, p_trig_job);
							unl_cpu(flags); //unlock to support kdrv wait_done inside trig
							kflow_ai_core_trig_job(p_ch, p_trig_job);
							loc_cpu(flags);
						}
					} else { //non-DLA job
						kflow_ai_core_run_job(p_ch, p_trig_job);
						unl_cpu(flags); //unlock to support kdrv wait_done inside trig
						kflow_ai_core_trig_job(p_ch, p_trig_job);
						loc_cpu(flags);
					}
					p_trig_job = 0;
				} else {
				}
			}
    	}
    }
}

static void kflow_ai_core_next_job(KFLOW_AI_JOB* p_job, UINT32 this_eng_attr, unsigned long flags)
{
	if (p_job == NULL) {
		return;
	}
	
	KLOG_OUT(KLOG_NEXT, "-- proc[%d].job[%d] notify its %d child\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id, (int)p_job->child_cnt);

	//if this JOB has next JOBs, notify these JOBs
	if (p_job->child_cnt > 0) {
	    LIST_HEAD *p_this; //current entry
	    LIST_HEAD *p_temp; //temp entry
	    KFLOW_AI_BIND* p_bind = 0;
	    KFLOW_AI_JOB* p_next_job = 0;

		//notify each next JOB to minus its waiting count
		list_for_each_safe(p_this, p_temp, &p_job->child_list) {
			p_bind = list_entry(p_this, KFLOW_AI_BIND, list);
			p_next_job = p_bind->p_job;
			if (p_next_job->p_eng->attr == this_eng_attr) { //0: DLA job, 1: non-DLA job
			
			    KLOG_OUT(KLOG_NEXT, "-- proc[%d].child-job[%d] update wait_cnt (%d => %d)\r\n", 
			    	(int)p_next_job->proc_id, (int)p_next_job->job_id, (int)p_next_job->wait_cnt, (int)p_next_job->wait_cnt-1);

				p_next_job->wait_cnt--;
				//if waiting count of JOB is minus to "zero",
				if (p_next_job->wait_cnt == 0) {
					//remove JOB from wait list
					kflow_ai_core_del_wait(p_next_job->p_eng, p_next_job);
					//add JOB to ready queue of engine

					KLOG_OUT(KLOG_NEXT, "--- proc[%d].child-job[%d] add to ready of engine[%s]\r\n", 
						(int)p_next_job->proc_id, (int)p_next_job->job_id, p_next_job->p_eng->name);

					kflow_ai_core_add_ready(p_next_job->p_eng, p_next_job, p_next_job->p_eng->attr, flags);
				}
			}
		}
	}
}

INT32 kflow_ai_core_lock_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job)
{
	UINT32 i  ;
	if (p_net == NULL) {
		return -1;
	}
	if (p_job == NULL) {
		return -1;
	}

	for (i = 0; i < p_net->job_cnt; i++) {
		KFLOW_AI_JOB* p_job = &(p_net->job[i]);
		if (p_job->p_eng != 0) {
			if (p_job->parent_cnt == 0) {		
				 p_job->wait_cnt++;
				 KLOG_OUT(KLOG_PUSH, "- proc[%d] - job[%d] lock\r\n",(int)p_job->proc_id, (int)p_job->job_id);
			}
		}
	}

	p_net->rv = 0; //OK
	
	
	
	return 0;
}

//static UINT32 isr_pcnt_max = 0;
//static UINT32 isr_pcnt = 0;

INT32 kflow_ai_core_unlock_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job)
{
	unsigned long flags;
	UINT32 i  ;
	if (p_net == NULL) {
		return -1;
	}
	if (p_job == NULL) {
		return -1;
	}
	if (p_job->p_eng == NULL) {
		return -1;
	}

	if (kflow_schd == SCHD_FIFO) {
		if (SEM_WAIT_INTERRUPTIBLE(g_kflow_fifo_SEM_ID[0])) {
			//Ignore
			return -2;
		}
	}
	 
	for (i = 0; i < p_net->job_cnt; i++) {
		KFLOW_AI_JOB* p_job = &(p_net->job[i]);
		if (p_job->p_eng != 0) {
			if (p_job->parent_cnt == 0) {		
				loc_cpu(flags);
				//isr_pcnt_max = 0;
				p_job->wait_cnt--;
				p_net->src_wait_cnt = p_net->src_cnt;
				p_net->dest_wait_cnt = p_net->dest_cnt;

				//DBG_DUMP("proc_id(%d) --- running - begin !!\n", (int)p_net->proc_id);

				KLOG_OUT(KLOG_PUSH, "- proc[%d] - job[%d] unlock\r\n",
					(int)p_job->proc_id, (int)p_job->job_id);

				kflow_ai_core_chk_ready(p_job->p_eng, p_job, flags);
				unl_cpu(flags);
			}
		}
	}
	return 0;
}

void kflow_ai_core_push_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job)
{
	unsigned long flags;
	if (p_net == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	if (p_job->p_eng == NULL) {
		return;
	}
	
	loc_cpu(flags);
	p_job->state = 0; //free
	p_job->wait_cnt += p_job->parent_cnt;
	
	kflow_ai_core_add_wait(p_job->p_eng, p_job);
	kflow_ai_core_chk_ready(p_job->p_eng, p_job, flags);
	unl_cpu(flags);
}

void kflow_ai_core_pull_begin(KFLOW_AI_NET* p_net)
{
	if (p_net == NULL) {
		return;
	}

	KLOG_OUT(KLOG_PULL, "- proc[%d] - pull open\r\n", 
		(int)p_net->proc_id);

	nvt_ai_proc_queue_open(&(g_kflow_queue[p_net->proc_id]), 
		p_net->proc_id, 
		&(g_kflow_data_SEM_ID[p_net->proc_id]),
		&(g_kflow_queue_SEM_ID[p_net->proc_id]));
}

void kflow_ai_core_pull_end(KFLOW_AI_NET* p_net)
{
	if (p_net == NULL) {
		return;
	}

	KLOG_OUT(KLOG_PULL, "- proc[%d] - pull close\r\n", 
		(int)p_net->proc_id);

	nvt_ai_proc_queue_close(&(g_kflow_queue[p_net->proc_id]));
}

void kflow_ai_core_pull_ready(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job)
{
	if (p_net == NULL) {
		return;
	}

	KLOG_OUT(KLOG_PULL, "- proc[%d] - pull ready\r\n", 
		(int)p_net->proc_id);

	nvt_ai_proc_queue_put(&(g_kflow_queue[p_net->proc_id]), (UINT32)p_job);
}

INT32 kflow_ai_core_pull_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB** p_pulljob)
{
	int r;
	UINT32 data;
	if (p_net == NULL) {
		return -1;
	}

	KLOG_OUT(KLOG_PULL, "- proc[%d] - pull job - begin\r\n", 
		(int)p_net->proc_id);

	r = nvt_ai_proc_queue_get(&(g_kflow_queue[p_net->proc_id]), &data, -1);

	KLOG_OUT(KLOG_PULL, "- proc[%d] - pull job - end\r\n", 
		(int)p_net->proc_id);

	//DBG_DUMP("proc_id(%d) --- running - end !!\n", (int)p_net->proc_id);
	if (kflow_schd == SCHD_FIFO) {
		SEM_SIGNAL(g_kflow_fifo_SEM_ID[0]);
	}
	
	if (r == E_RLWAI) {
		p_net->rv = -3; //ABORT
		return -2;
	}
	if (r != E_OK) {
		return -1;
	}

	p_pulljob[0] = (KFLOW_AI_JOB*)data;
	if (p_net->rv != 0) {
		return -3;
	}
	/*
	p_net->dest_wait_cnt--;
	if (p_net->dest_wait_cnt > 0) {
		return 1;
	}*/
	
	return 0;
}

void kflow_ai_core_onlast_job(KFLOW_AI_JOB* p_job)
{
	//kflow_ai_job_cb(p_job->proc_id, p_job);
	KFLOW_AI_NET* p_net = NULL;

	if (p_job == NULL) {
		return;
	}
	
	p_net = kflow_ai_core_net(p_job->proc_id);
	if (p_net == NULL) {
		return;
	}

	p_net->dest_wait_cnt--;
	
	KLOG_OUT(KLOG_PULL, "- proc[%d] - job[%d] finish\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id);

	//DBG_IND("- proc[%d] - job[%d] finish! max isr_pcnt=%d ", (int)p_job->proc_id, (int)p_job->job_id, isr_pcnt_max);
	
	if (p_net->dest_wait_cnt == 0) {
		
		p_net->rv = 0; //OK
		
		if (KLOG_GET() > 0) {
			DBG_DUMP("kflow_ai - log dump:\r\n");
			KLOG_DUMP();
			DBG_DUMP("kflow_ai - log end:\r\n");
			KLOG_CLOSE();
			KLOG_SET(0); //clear
		}
		if (p_net->proc_id == debug_proc_id) {
			p_net->debug_func = debug_func;
		}
#if (DBG_TIME_DUMP == 1)
		p_net->debug_func |= KFLOW_AI_DBG_TIME;
#endif
		if (p_net->debug_func & KFLOW_AI_DBG_TIME) {
			kflow_ai_net_dump(p_net, 1); //job result
			p_net->debug_func &= ~KFLOW_AI_DBG_TIME;
		}
		if (p_net->debug_func & KFLOW_AI_DBG_TIMELINE) {
			kflow_ai_net_dump(p_net, 2); //job core  .... html
			p_net->debug_func &= ~KFLOW_AI_DBG_TIMELINE;
		}

		//this will notify user proc() finish, and net will be destory by user stop() immediately
		kflow_ai_core_pull_ready(p_net, p_job);
		
	}
}

static GRAPH_DEBUG_SCHEDULE_HANDLER graph_debug_hdl = (-1);
static CHAR filepath[64] = "/mnt/sd/kflow_ai_timeline.html";
static CHAR blk_name[20];

void kflow_ai_core_dump_job(UINT32 info, UINT32 act, KFLOW_AI_JOB* p_job)
{
	static CHAR na_str[2] = "-";
	//bind info
    if (info == 0) {
        if (act == 2) {
            if (p_job->p_eng != 0) {
                static CHAR parent_str[256] = "";
                static CHAR child_str[256] = "";
                CHAR* str;
        	    str = parent_str;
        	    str[0] = 0;
            	if (p_job->parent_cnt > 0) {
            	    LIST_HEAD *p_this; //current entry
					LIST_HEAD *p_temp; //temp entry
            	    KFLOW_AI_BIND* p_bind = 0;
            		list_for_each_safe(p_this, p_temp, &p_job->parent_list) {
            			p_bind = list_entry(p_this, KFLOW_AI_BIND, list);
            			snprintf(str, 256, "%d ", (int)p_bind->p_job->job_id);
            			str += strlen(str);
            		}
            	}
         	    str = child_str;
        	    str[0] = 0;
            	if (p_job->child_cnt > 0) {
            	    LIST_HEAD *p_this; //current entry
					LIST_HEAD *p_temp; //temp entry
            	    KFLOW_AI_BIND* p_bind = 0;
           		    list_for_each_safe(p_this, p_temp, &p_job->child_list) {
            			p_bind = list_entry(p_this, KFLOW_AI_BIND, list);
            			snprintf(str, 256, "%d ", (int)p_bind->p_job->job_id);
            			str += strlen(str);
            		}
                }
             	DBG_DUMP("  \\_ proc[%d].job[%d] engine[%s]: parent_cnt=%d child_cnt=%d parent=[%s] child=[%s]\r\n",
            	    (int)p_job->proc_id, (int)p_job->job_id, 
            	    p_job->p_eng->name,
            	    (int)p_job->parent_cnt, (int)p_job->child_cnt, parent_str, child_str);
            }
        }
    }

	//wait info
    if (info == 3) {
        if (act == 2) {
            if (p_job->p_eng != 0) {
             	DBG_DUMP("  \\_ proc[%d].job[%d]: state=%u, wait_cnt=%d, ts_ideal=%u, ts_wait_in=%u\r\n",
            	    (int)p_job->proc_id, (int)p_job->job_id, 
            	    p_job->state, (int)p_job->wait_cnt,
            	    p_job->ts_exec_predict, p_job->ts_wait_begin);
            }
        }
    }
	
	//ready info
    if (info == 4) {
        if (act == 2) {
            if (p_job->p_eng != 0) {
             	DBG_DUMP("  \\_ proc[%d].job[%d]: state=%u, wait_cnt=%d, ts_ideal=%u, ts_ready_in=%u\r\n",
            	    (int)p_job->proc_id, (int)p_job->job_id, 
            	    p_job->state, (int)p_job->wait_cnt,
            	    p_job->ts_exec_predict, p_job->ts_ready_begin);
           }
        }
    }
	
	//result info
    if (info == 1) {
        if (act == 2) {
            if (p_job->p_ch != 0) {
                UINT32 exec_hw = 0;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                UINT32 exec_macc = 0;
#endif
                CHAR* eng_name = p_job->p_eng->name;
                CHAR* ch_name = ((p_job->p_ch == 0)?(na_str):(p_job->p_ch->name));
                if (p_job->ts_exec_cycle > 0) {
                    if (strcmp(p_job->p_ch->name, "cnn1") == 0) exec_hw = p_job->ts_exec_cycle/CNN_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "cnn2") == 0) exec_hw = p_job->ts_exec_cycle/CNN_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "cnn3") == 0) exec_hw = p_job->ts_exec_cycle/CNN_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "nue1") == 0) exec_hw = p_job->ts_exec_cycle/NUE_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "nue2") == 0) exec_hw = p_job->ts_exec_cycle/NUE2_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "cpu") == 0) exec_hw = 0;
                }
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                if (p_job->ts_exec_macc_cycle > 0) {
                    if (strcmp(p_job->p_ch->name, "cnn1") == 0) exec_macc = p_job->ts_exec_macc_cycle/CNN_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "cnn2") == 0) exec_macc = p_job->ts_exec_macc_cycle/CNN_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "cnn3") == 0) exec_macc = p_job->ts_exec_macc_cycle/CNN_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "nue1") == 0) exec_macc = p_job->ts_exec_macc_cycle/NUE_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "nue2") == 0) exec_macc = p_job->ts_exec_macc_cycle/NUE2_ENG_CLK;
                    if (strcmp(p_job->p_ch->name, "cpu") == 0) exec_macc = 0;
                }
#endif
#if (CNN_NUE_EXCLUSIVE == 1)
                //check 0x00008000 for NUE mode
                if (p_job->engine_op & 0x00008000) {
                    //should be a NUE job, replace to NUE engine & NUE channel for dump
                    KFLOW_AI_ENGINE_CTX *p_nue_eng = p_job->p_eng->geteng(1); // 1 = VENDOR_AI_ENGINE_NUE
                    KFLOW_AI_CHANNEL_CTX *p_nue_ch = p_nue_eng->p_ch[0];
                    eng_name = p_nue_eng->name;
                    ch_name = p_nue_ch->name;
                    if (p_job->ts_exec_cycle > 0) {
                        exec_hw = p_job->ts_exec_cycle/NUE_ENG_CLK;
                    }
                }
#endif
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                DBG_DUMP("  \\_ proc[%d].job[%d] engine[%s] ch[%s]: state=%u, ts_wait=%u, ts_ready=%u, ts_exec=%u, ts_exec_hw=%u, ts_exec_macc=%u, ts_ideal=%u\r\n",
                    (int)p_job->proc_id, (int)p_job->job_id,
            	    eng_name, ch_name, p_job->state, 
                    p_job->ts_wait_end - p_job->ts_wait_begin,
                    p_job->ts_ready_end - p_job->ts_ready_begin,
                    p_job->ts_exec_end - p_job->ts_exec_begin, //actual sw time
                    exec_hw, //actual hw time
                    exec_macc,  //actual hw macc time
                    p_job->ts_exec_predict  //ideal predict time (provided by gentool)
                    );
#else
                DBG_DUMP("  \\_ proc[%d].job[%d] engine[%s] ch[%s]: state=%u, ts_wait=%u, ts_ready=%u, ts_exec=%u, ts_exec_hw=%u, ts_ideal=%u\r\n",
            	    (int)p_job->proc_id, (int)p_job->job_id,
            	    eng_name, ch_name, p_job->state, 
            	    p_job->ts_wait_end - p_job->ts_wait_begin,
            	    p_job->ts_ready_end - p_job->ts_ready_begin,
            	    p_job->ts_exec_end - p_job->ts_exec_begin, //actual sw time
                    exec_hw, //actual hw time
                    p_job->ts_exec_predict  //ideal predict time (provided by gentool)
                    );
#endif
            }
        }
    }

	//result timeline file
    if (info == 2) {
        if (act == 0) { //open
            graph_debug_schedule_open(filepath, &graph_debug_hdl);
            if (graph_debug_hdl == (-1)) {
                DBG_ERR("[kflow_ai] open file (%s) fail ...\r\n", filepath);
                return;
            }
        } else if (act == 1) { //close
            graph_debug_schedule_close(graph_debug_hdl);
            DBG_DUMP("[kflow_ai] save file (%s) ok\r\n", filepath);
            graph_debug_hdl = -1;
        } else if (act == 2) {
            UINT32 eng = SCHE_ENG_CPU;
            if (p_job->p_ch != 0) {
                snprintf(blk_name, 20, "J[%d] %lu us", (int)p_job->job_id, (unsigned long)(p_job->ts_exec_end - p_job->ts_exec_begin));

                if (strcmp(p_job->p_ch->name, "cnn1") == 0) eng = SCHE_ENG_CNN;
                if (strcmp(p_job->p_ch->name, "cnn2") == 0) eng = SCHE_ENG_CNN2;
#if defined(_BSP_NA51090_)
                if (strcmp(p_job->p_ch->name, "cnn3") == 0) eng = SCHE_ENG_CNN3;
#endif
                if (strcmp(p_job->p_ch->name, "nue1") == 0) eng = SCHE_ENG_NUE;
                if (strcmp(p_job->p_ch->name, "nue2") == 0) eng = SCHE_ENG_NUE2;
                if (strcmp(p_job->p_ch->name, "cpu") == 0) eng = SCHE_ENG_CPU;
                graph_debug_schedule_add_block(graph_debug_hdl, eng, p_job->ts_exec_begin, p_job->ts_exec_end, blk_name, 20);
            }

        } else if (act == 3) {
            UINT32 eng = SCHE_ENG_CPU;
            UINT32 parent_eng = SCHE_ENG_CPU;
            //UINT32 child_eng = SCHE_ENG_CPU;
            if (p_job->p_ch != 0) {
                if (strcmp(p_job->p_ch->name, "cnn1") == 0) eng = SCHE_ENG_CNN;
                if (strcmp(p_job->p_ch->name, "cnn2") == 0) eng = SCHE_ENG_CNN2;
#if defined(_BSP_NA51090_)
                if (strcmp(p_job->p_ch->name, "cnn3") == 0) eng = SCHE_ENG_CNN3;
#endif
                if (strcmp(p_job->p_ch->name, "nue1") == 0) eng = SCHE_ENG_NUE;
                if (strcmp(p_job->p_ch->name, "nue2") == 0) eng = SCHE_ENG_NUE2;
                if (strcmp(p_job->p_ch->name, "cpu") == 0) eng = SCHE_ENG_CPU;
            	if (p_job->parent_cnt > 0) {
            	    LIST_HEAD *p_this; //current entry
					LIST_HEAD *p_temp; //temp entry
            	    KFLOW_AI_BIND* p_bind = 0;
            		list_for_each_safe(p_this, p_temp, &p_job->parent_list) {
            			p_bind = list_entry(p_this, KFLOW_AI_BIND, list);
                        if (strcmp(p_bind->p_job->p_ch->name, "cnn1") == 0) parent_eng = SCHE_ENG_CNN;
                        if (strcmp(p_bind->p_job->p_ch->name, "cnn2") == 0) parent_eng = SCHE_ENG_CNN2;
#if defined(_BSP_NA51090_)
                        if (strcmp(p_bind->p_job->p_ch->name, "cnn3") == 0) parent_eng = SCHE_ENG_CNN3;
#endif
                        if (strcmp(p_bind->p_job->p_ch->name, "nue1") == 0) parent_eng = SCHE_ENG_NUE;
                        if (strcmp(p_bind->p_job->p_ch->name, "nue2") == 0) parent_eng = SCHE_ENG_NUE2;
                        if (strcmp(p_bind->p_job->p_ch->name, "cpu") == 0) parent_eng = SCHE_ENG_CPU;
                        graph_debug_schedule_add_connection(graph_debug_hdl, parent_eng, p_bind->p_job->ts_exec_end, eng, p_job->ts_exec_begin);
            		}
            	}
        	}
        }
    }
}

static void kflow_ai_core_schd_job(void* p_param)
{
	unsigned long flags;
	KFLOW_AI_JOB* p_job = 0;
	KFLOW_AI_CHANNEL_CTX* p_ch = 0;
	UINT32 info = 0;

	if (p_param == NULL) {
		return;
	}
	
	p_job = (KFLOW_AI_JOB*)p_param;
	p_ch = p_job->p_ch;
	if (p_ch == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	if (p_job->p_eng == NULL) {
		return;
	}
	
	KLOG_OUT(KLOG_NEXT, "- proc[%d].job[%d] schedule - begin\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id);

#if (ISR_TRIG == 1)
    if (p_job->wait_ms < 0) { //under sync, support load debug_func
		// set debug info
		info = p_job->debug_func;
#if (DBG_OUT_DUMP == 1)
		info |= KFLOW_AI_DBG_OBUF;
#endif
#if (DBG_REG_DUMP == 1)
		info |= KFLOW_AI_DBG_CTX;
#endif
		if (info != 0) {
			if (p_ch == NULL) {
				DBG_ERR("%s:%d p_ch == NULL\n", __func__, __LINE__);
				goto debug_end;
			}
			if (p_ch->debug == NULL) {
				DBG_ERR("%s:%d p_ch->debug NULL\n", __func__, __LINE__);
				goto debug_end;
			}
			// call engine debug
			p_ch->debug(p_ch, p_job, info);

			// clear debug func
			p_job->debug_func = 0;
		}
    }
	
debug_end:
	loc_cpu(flags);
    if (p_job->wait_ms < 0) { //under sync, only trigger other job at task
		kflow_ai_core_this_job(p_job, 0, flags); //dispatch any DLA job in ready queue ... to engine's ch
    }
    if (p_job->wait_ms < 0) { //under sync, only trigger next job at task
	    kflow_ai_core_next_job(p_job, 0, flags); //dispatch DLA job in wait queue ... to ready queue
    }
#else
	kflow_ai_core_done_job(p_job); //done
	kflow_ai_core_this_job(p_job, 0, flags); //dispatch any DLA job in ready queue ... to engine's ch
	kflow_ai_core_next_job(p_job, 0, flags); //dispatch DLA job in wait queue ... to ready queue
#endif
	kflow_ai_core_this_job(p_job, 1, flags); //dispatch any non-DLA job in ready queue ... to engine's ch
	kflow_ai_core_next_job(p_job, 1, flags); //dispatch non-DLA job in wait queue ... to ready queue
	unl_cpu(flags);

	if (p_job->child_cnt == 0) {

		KLOG_OUT(KLOG_NEXT, "--- proc[%d].job[%d] notify USER!\r\n", 
			(int)p_job->proc_id, (int)p_job->job_id);
		
		kflow_ai_core_onlast_job(p_job); //notify last job is finished
	}
	
	KLOG_OUT(KLOG_NEXT, "- proc[%d].job[%d] schedule - end\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id);
}

#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
void kflow_ai_core_onfinish_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job, UINT32 cycle, UINT32 macc)
#else
void kflow_ai_core_onfinish_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job, UINT32 cycle)
#endif
{
	unsigned long flags;

	if (p_ch == NULL) {
		return;
	}
	if (p_job == NULL) {
		return;
	}
	if (p_job->p_eng == 0) {
		return;
	}

	loc_cpu(flags);	
	//isr_pcnt ++;
	//if (isr_pcnt > isr_pcnt_max)
	//	isr_pcnt_max = isr_pcnt;
    p_job->ts_exec_end = _nvt_ai_get_counter();
	if (p_job->ts_exec_end > p_job->ts_exec_begin) {
		p_job->ts_exec = (p_job->ts_exec_end - p_job->ts_exec_begin);
	} else {
		p_job->ts_exec = 0;
	}
    p_job->ts_exec_cycle = cycle;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
    p_job->ts_exec_macc_cycle = macc;
#endif
	if (1) {
		p_ch->lts_exec += (UINT64)p_job->ts_exec;
	}

	if (kflow_ai_perf_level > 0 )	{
		p_ch->ts_exec += (p_job->ts_exec);
		p_ch->ts_exec_hw += (p_job->ts_exec_cycle);
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
		p_ch->ts_exec_macc += (p_job->ts_exec_macc_cycle);
#endif	
		p_ch->ts_exec_ideal += (p_job->ts_exec_ideal_cycle);
	//DBG_DUMP(" ++ proc[%d].job[%d] engine[%s] ch[%s] (%llu %llu %llu %llu)\r\n",
	//	(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name, p_ch->ts_exec, p_ch->ts_exec_hw, p_ch->ts_exec_macc, p_ch->ts_exec_ideal);
	//DBG_DUMP(" ++engine[%s] ch[%s] (%u %u %u)\r\n",
	//	p_job->p_eng->name, p_ch->name, p_ch->ts_exec, p_ch->ts_exec_hw, p_ch->ts_exec_ideal);
    //	if (strcmp(p_job->p_ch->name, "nue1") == 0) {
    //	DBG_DUMP(" ++ proc[%d].job[%d] engine[%s] ch[%s] (%u %u %u %u)\r\n",
    //		(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name, (p_job->ts_exec_end - p_job->ts_exec_begin), p_job->ts_exec_cycle, p_job->ts_exec_macc_cycle, p_job->ts_exec_ideal_cycle);
    //	DBG_DUMP(" ++engine[%s] ch[%s] (%llu %llu %llu %llu)\r\n",
    //		p_job->p_eng->name, p_ch->name, p_ch->ts_exec, p_ch->ts_exec_hw, p_ch->ts_exec_macc, p_ch->ts_exec_ideal);
    //   }
	}

	if (cycle == 0xffff0000) {
		KFLOW_AI_NET* p_net = NULL;
		p_job->state = 11; //done but error
		DBG_ERR(" - proc[%d].job[%d] engine[%s] ch[%s] failed!\r\n", 
			(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name);
		p_net = kflow_ai_core_net(p_job->proc_id);
		if (p_net) {
			p_net->rv = -1; //FAILED
		}
	}
	if (cycle == 0xfffe0000) {
		KFLOW_AI_NET* p_net = NULL;
		p_job->state = 12; //timeout error
		DBG_ERR(" - proc[%d].job[%d] engine[%s] ch[%s] timeout!\r\n", 
			(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name);
		p_net = kflow_ai_core_net(p_job->proc_id);
		if (p_net) {
			p_net->rv = -2; //TIMEOUT
		}
	}

	KLOG_OUT(KLOG_NEXT, "- proc[%d].job[%d] done by engine[%s] ch[%s]\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_job->p_ch->name);

#if (LOAD_BALANCE == 1)
    p_ch->ts_prev_end = p_job->ts_exec_end;

	KLOG_OUT(KLOG_READY, "---..... prev_end=%u\r\n", p_ch->ts_prev_end);

#endif

#if (ISR_TRIG == 1)
	kflow_ai_core_done_job(p_job); //done
    if (p_job->wait_ms >= 0) { //under async, support trigger other job at isr
		kflow_ai_core_this_job(p_job, 0, flags); //dispatch any DLA job in ready queue ... to engine's ch
    }
    if (p_job->wait_ms >= 0) { //under async, support trigger next job at isr
	    kflow_ai_core_next_job(p_job, 0, flags); //dispatch DLA job in wait queue ... to ready queue
    }
#else
    //do nothing
#endif

	//deferred "schd_job" to task
	p_job->deferred.list.prev = 0;
	p_job->deferred.list.next = 0;
	p_job->deferred.p_param = 0;
	p_job->deferred.p_exec = 0;
	kflow_ai_core_tsk_init_func(&(p_job->deferred), kflow_ai_core_schd_job, p_job);
	unl_cpu(flags);
	kflow_ai_core_tsk_put_func(&(p_job->deferred));

	//isr_pcnt --;
	KLOG_OUT(KLOG_NEXT, "- proc[%d].job[%d] schedule - deferred\r\n", 
		(int)p_job->proc_id, (int)p_job->job_id);
}

void kflow_ai_net_create(KFLOW_AI_NET* p_net, UINT32 max_job_cnt, UINT32 job_cnt, UINT32 bind_cnt, UINT32 ddr_id)
{

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	UINT32 blk_size, blk_cnt;
	static CHAR pool_name[34] = "";
	NVTMPP_ER          ret;
	NVTMPP_MODULE      module = MAKE_NVTMPP_MODULE('V', 'e', 'n', 'd', 'e', 'r', 'A', 'I');
	UINT32 map_size, job_size, bind_size;
	UINT8* addr;

	if (p_net == NULL) {
		return;
	}
	
	snprintf(pool_name, 24, "ainet.proc[%u].job", (UINT)p_net->proc_id);

	ret = nvtmpp_vb_add_module(module);
	if (NVTMPP_ER_OK != ret) {
	}

	KLOG_OUT(KLOG_STAGE, "- proc[%d] - create - begin\r\n", (int)p_net->proc_id);

	p_net->max_job_cnt = max_job_cnt;
    p_net->job_cnt = job_cnt;
    p_net->bind_cnt = bind_cnt;
	//DBG_IND("xxx-1 %d %d\r\n", (int)p_net->job_cnt, (int)p_net->bind_cnt);
	//DBG_IND("xxx-2 %08x %08x\r\n", (int)sizeof(KFLOW_AI_JOB), (int)sizeof(KFLOW_AI_BIND));
	//DBG_IND("xxx-3 %08x %08x\r\n", (int)sizeof(KFLOW_AI_JOB) * job_cnt, (int)sizeof(KFLOW_AI_BIND) * bind_cnt);

	map_size = 0;
	if (max_job_cnt > job_cnt) {
		map_size = sizeof(UINT32) * max_job_cnt;
	}
	job_size = sizeof(KFLOW_AI_JOB) * job_cnt;
	bind_size = sizeof(KFLOW_AI_BIND) * bind_cnt;
	
	blk_size = map_size + job_size + bind_size;
	blk_cnt = 1;
	//DBG_IND("xxx-4 size=%08x\r\n", (int)blk_size);

	p_net->pool = nvtmpp_vb_create_pool(pool_name, blk_size, blk_cnt, ddr_id);
	if ((INT32)p_net->pool == NVTMPP_VB_INVALID_POOL) {
		return;
	}
	p_net->addr = 0;
	p_net->map = 0;
	p_net->job = 0;
	p_net->bind = 0;
	p_net->map_id = 0; //free map_id from here
	
	p_net->blk_id = nvtmpp_vb_get_block(module, p_net->pool, blk_size, ddr_id);
	if (NVTMPP_VB_INVALID_BLK == (INT32)p_net->blk_id) {
		return;
	}
	p_net->addr = (void* )nvtmpp_vb_blk2va(p_net->blk_id);
	if (p_net->addr == 0) {
		return;
	}
	
	memset(p_net->addr, 0, blk_size);

	addr = (UINT8*)(p_net->addr);
	if (max_job_cnt > job_cnt) {
		p_net->map = (UINT32*)addr;
		addr += map_size;
	}
	if (1) {
		p_net->job = (KFLOW_AI_JOB*)addr;
		addr += job_size;
	}
	if (1) {
		p_net->bind = (KFLOW_AI_BIND*)addr;
	}

#else
	UINT32 map_size = 0, job_size = 0, bind_size = 0;

	if (p_net == NULL) {
        return;
	}
	
	KLOG_OUT(KLOG_STAGE, "- proc[%d] - create - begin\r\n", (int)p_net->proc_id);

	p_net->max_job_cnt = max_job_cnt;
    p_net->job_cnt = job_cnt;
    p_net->bind_cnt = bind_cnt;
	p_net->src_cnt = 1;
	p_net->src_wait_cnt = 0;
	p_net->dest_cnt = 0;
	p_net->dest_wait_cnt = 0;
    p_net->addr = 0;

	map_size = 0;
	p_net->map = 0;
	p_net->map_id = 0; //free map_id from here
	if (max_job_cnt > job_cnt) {
		map_size = sizeof(UINT32) * max_job_cnt;
		p_net->map = vmalloc(map_size);
		if (p_net->map == 0) {
			return;
		}
		memset(p_net->map, 0, map_size);
	}
	
	if (job_cnt > 0) {
		job_size = sizeof(KFLOW_AI_JOB) * job_cnt;
		p_net->job = vmalloc(job_size);
		if (p_net->job == 0) {
			return;
		}
		memset(p_net->job, 0, job_size);
	}

	if (bind_cnt > 0) {
		bind_size = sizeof(KFLOW_AI_BIND) * bind_cnt;
		p_net->bind = vmalloc(bind_size);
		if (p_net->bind == 0) {
			return;
		}
		memset(p_net->bind, 0, bind_size);
	}
#endif

	
	//KLOG_OUT(KLOG_MEM, "- proc[%d] - job_cnt=%u, job_mem_size=%08x\r\n", (int)p_net->proc_id, job_cnt, sizeof(KFLOW_AI_JOB) * job_cnt);
	//KLOG_OUT(KLOG_MEM, "- proc[%d] - bind_cnt=%u, bind_mem_size=%08x\r\n", (int)p_net->proc_id, bind_cnt, sizeof(KFLOW_AI_BIND) * bind_cnt);

	INIT_LIST_HEAD(&p_net->free_job_list);
	{
		UINT32 i;
		for (i = 0; i < job_cnt; i++) {
			p_net->job[i].proc_id = p_net->proc_id;
			//p_net->job[i].job_id = i;
			kflow_ai_core_clr_job(p_net, &p_net->job[i]);
		}
		// clear debug bitmap
		if (p_net->proc_id == debug_proc_id) {
			p_net->debug_func = debug_func;
		}
		if (p_net->debug_func & KFLOW_AI_DBG_CTX) {
			p_net->debug_func &= ~KFLOW_AI_DBG_CTX;
		}
		if (p_net->debug_func & KFLOW_AI_DBG_OBUF) {
			p_net->debug_func &= ~KFLOW_AI_DBG_OBUF;
		}
	}
	INIT_LIST_HEAD(&p_net->free_bind_list);
	{
		UINT32 i;
		for (i = 0; i < bind_cnt; i++) {
			kflow_ai_core_clr_bind(p_net, &p_net->bind[i]);
			list_add_tail(&(p_net->bind[i].list), &p_net->free_bind_list);
		}
	}

	//kflow_ai_core_cb_init(p_net->proc_id);
	kflow_ai_cpu_init(p_net->proc_id);
	kflow_ai_dsp_init(p_net->proc_id);
	
	p_net->rv = 0;
	
	KLOG_OUT(KLOG_STAGE, "- proc[%d] - create - end\r\n", (int)p_net->proc_id);
}

void kflow_ai_net_debug(UINT32 proc_id, UINT32 func)
{
	debug_proc_id = proc_id;
	debug_func |= func;
}

void kflow_ai_net_flow(UINT32 mask)
{
	if (mask > 0) {
		if (KLOG_GET() > 0) {
			DBG_DUMP("kflow_ai - log already begin, change: %04x\r\n", mask);
			KLOG_SET(mask);
		} else {
			KLOG_OPEN(1000*1000);
			DBG_DUMP("kflow_ai - log begin: %04x\r\n", mask);
			KLOG_SET(mask);
		}
	}
	if (mask == 0) {
		if (KLOG_GET() > 0) {
			KLOG_SET(mask);
			DBG_DUMP("kflow_ai - log dump:\r\n");
			KLOG_DUMP();
			DBG_DUMP("kflow_ai - log end:\r\n");
			KLOG_CLOSE();
		} else {
			DBG_DUMP("kflow_ai - log not begin yet, error!\r\n");
		}
	}
}

void kflow_ai_net_dump(KFLOW_AI_NET* p_net, UINT32 info)
{
	UINT32 i;
	if (p_net == NULL) {
		return;
	}
	
	kflow_ai_core_dump_job(info, 0, 0); //open
	for (i = 0; i < p_net->job_cnt; i++) {
		kflow_ai_core_dump_job(info, 2, &p_net->job[i]); //job
	}
	if (info == 2) {
	for (i = 0; i < p_net->job_cnt; i++) {
		kflow_ai_core_dump_job(info, 3, &p_net->job[i]); //job
	}
	}
	kflow_ai_core_dump_job(info, 1, 0); //close
}

void kflow_ai_engine_dump(KFLOW_AI_ENGINE_CTX* p_eng, UINT32 info)
{
	KFLOW_AI_JOB* p_job = 0;
	UINT32 j;
	LIST_HEAD *p_this; //current entry
	LIST_HEAD *p_temp; //temp entry

	if (p_eng == NULL) {
		return;
	}

	DBG_DUMP("kflow_ai - core dump:\r\n");
	if (info & 0x01) {
		DBG_DUMP(" <1> engine[%s] wait_queue:\r\n", p_eng->name);
		list_for_each_safe(p_this, p_temp, p_eng->wait_queue) {
			p_job = list_entry(p_this, KFLOW_AI_JOB, list);
			kflow_ai_core_dump_job(3, 2, p_job); //wait info=3
		}
	}
	if (info & 0x02) {
		//for all channels
		for (j=0; j<p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
			DBG_DUMP(" <2> engine[%s] ch[%s] ready_queue:\r\n", p_eng->name, p_ch->name);
			list_for_each_safe(p_this, p_temp, p_ch->ready_queue) {
				p_job = list_entry(p_this, KFLOW_AI_JOB, list);
				kflow_ai_core_dump_job(4, 2, p_job); //ready info=4
	    	}
		}
		for (j=0; j<p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
			p_job = p_ch->run_job;
			if (p_job) {
				DBG_DUMP(" <3> engine[%s] ch[%s] running ...\r\n", p_eng->name, p_ch->name);
				kflow_ai_core_dump_job(4, 2, p_job); //ready info=4
			} else {
				DBG_DUMP(" <3> engine[%s] ch[%s] idle\r\n", p_eng->name, p_ch->name);
			}
		}
	}
}

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
static void kflow_ai_core_chk_callback(UINT32 nouse)
{
	UINT32 i;
    kflow_perf_count++;
    if (kflow_perf_count < kflow_perf_sample) {
        return;
    }
    kflow_perf_count = 0;

    if (kflow_perf_dump) {
    } else {
    	if (kflow_ai_perf_level == 1 )	{snprintf(_ai_perf_log, PERF_LOG_SIZE, " perf-ut:");}
    	if (kflow_ai_perf_level == 2 )	{snprintf(_ai_perf_log, PERF_LOG_SIZE, " perf-all:");}
    }
	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		kflow_ai_core_chk_timeout(p_eng);
	}
    if (kflow_perf_dump) {
    } else {
    	if (kflow_ai_perf_level == 1 )	{DBG_DUMP("%s\r\n", _ai_perf_log);}
    	if (kflow_ai_perf_level == 2 )	{DBG_DUMP("%s\r\n", _ai_perf_log);}
    }
}
#else
static enum hrtimer_restart kflow_ai_core_chk_callback(struct hrtimer * timer)
{
	UINT32 i;
    kflow_perf_count++;
    if (kflow_perf_count < kflow_perf_sample) {
        return HRTIMER_RESTART;
    }
    kflow_perf_count = 0;

    if (kflow_perf_dump) {
    } else {
    	if (kflow_ai_perf_level == 1 )	{snprintf(_ai_perf_log, PERF_LOG_SIZE, " perf-ut:");}
    	if (kflow_ai_perf_level == 2 )	{snprintf(_ai_perf_log, PERF_LOG_SIZE, " perf-all:");}
    }
	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		kflow_ai_core_chk_timeout(p_eng);
	}
    if (kflow_perf_dump) {
    } else {
    	if (kflow_ai_perf_level == 1 )	{DBG_DUMP("%s\r\n", _ai_perf_log);}
    	if (kflow_ai_perf_level == 2 )	{DBG_DUMP("%s\r\n", _ai_perf_log);}
    }
    hrtimer_forward_now(timer, kt_periode);
    return HRTIMER_RESTART;
}
#endif

void kflow_ai_core_dump(void)
{
	UINT32 i;
	unsigned long flags;
	
	//for all engines
	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		loc_cpu(flags);
		kflow_ai_engine_dump(p_eng, 0x03);
		unl_cpu(flags);
	}
}

void kflow_ai_engine_reg_dump(KFLOW_AI_CHANNEL_CTX* p_ch, UINT32 info)
{
	if (!p_ch) {
		DBG_ERR("p_ch is NULL\n");
		return;
	}
	if (!p_ch->debug) {
		DBG_ERR("p_ch->debug is NULL\n");
		return;
	}

	// call engine debug
	p_ch->debug(p_ch, 0, info);
}

void kflow_ai_reg_dump(void)
{
	UINT32 i, j;
	
	//for all engines
	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		//for all channels
		for (j=0; j<p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];

			if (p_ch != 0) {
				kflow_ai_engine_reg_dump(p_ch, KFLOW_AI_DBG_CTX);
			}
		}
	}
}

void kflow_ai_net_destory(KFLOW_AI_NET* p_net)
{
	if (p_net == NULL) {
		return;
	}

#if NN_DLI
	if (p_net->job == 0)
		return;

	// p_net->pool is 0 ==> already forced to reset 0 in kflow_ai_core - reset flow
	// In reset flow, _vendor_ai_cpu_callback_thread was deleted by Ctrl + C ==> skip job stop   
	if(p_net->pool != 0){
		UINT32 i;
		for (i = 0; i < p_net->job_cnt; i++) {
			KFLOW_AI_JOB* p_job = &p_net->job[i];
			KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[p_job->engine_id];
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[0];
			
			KLOG_OUT(KLOG_SETUP, "proc[%d].job[%d] clear - engine_id=%d engine_op=%d p_op_info=%08x p_io_info=%08x wait_ms=%d (wait_cnt=%d)\r\n",
				(int)p_job->proc_id, (int)p_job->job_id,
				(int)p_job->engine_id, (int)p_job->engine_op,
				(int)p_job->p_op_info, (int)p_job->p_io_info,
				(int)p_job->wait_ms, (int)p_job->wait_cnt);
			
			kflow_ai_core_stop_job(p_ch, p_job);
		}
	}
#endif

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	if (p_net->addr == 0) {
		return;
	}

	KLOG_OUT(KLOG_STAGE, "- proc[%d] - destory - begin\r\n", (int)p_net->proc_id);
	
	//force cancel pull (for async porc)
	//nvt_ai_proc_queue_cancel(&(g_kflow_queue[p_net->proc_id]));

	//kflow_ai_core_cb_exit(p_net->proc_id);
	kflow_ai_cpu_exit(p_net->proc_id);
	kflow_ai_dsp_exit(p_net->proc_id);

	if (p_net->pool) {
		NVTMPP_ER mr;
		mr = nvtmpp_vb_destroy_pool(p_net->pool);
		if (mr != NVTMPP_ER_OK) {
			DBG_ERR("destory pool failed\n");
		}
	}
#else

#if NN_DLI
#else
	if (p_net->job == 0)
		return;
#endif

	KLOG_OUT(KLOG_STAGE, "- proc[%d] - destory - begin\r\n", (int)p_net->proc_id);

	//force cancel pull (for async porc)
	//nvt_ai_proc_queue_cancel(&(g_kflow_queue[p_net->proc_id]));

	//kflow_ai_core_cb_exit(p_net->proc_id);
	kflow_ai_cpu_exit(p_net->proc_id);
	kflow_ai_dsp_exit(p_net->proc_id);

	if (p_net->job) {
		vfree(p_net->job);
	}
	if (p_net->bind) {
		vfree(p_net->bind);
	}
	if (p_net->map) {
		vfree(p_net->map);
	}
#endif

	p_net->addr = 0;
    p_net->job = 0;
    p_net->bind = 0;
    p_net->blk_id = 0;
    p_net->blk_sz = 0;
    p_net->pool = 0;
    p_net->map = 0;
	
	KLOG_OUT(KLOG_STAGE, "- proc[%d] - destory - end\r\n", (int)p_net->proc_id);
}

KFLOW_AI_JOB* kflow_ai_net_add_job(KFLOW_AI_NET* p_net, UINT32 job_id)
{
	if (p_net == NULL) {
		return 0;
	}

	if (p_net->job) {
		KFLOW_AI_JOB* p_new_job = 0;
		if (p_net->map) {
			// all-map-used
			p_net->map[job_id] = p_net->map_id;
			p_new_job = p_net->job + p_net->map_id;
			p_new_job->job_id = job_id;
			p_net->map_id++;
			return p_new_job;
		}
		// 1-map-1
		p_new_job = p_net->job + job_id;
		p_new_job->job_id = job_id;
		return p_new_job;
	}
	return 0;
}

KFLOW_AI_JOB* kflow_ai_net_job(KFLOW_AI_NET* p_net, UINT32 job_id)
{
	if (p_net == NULL) {
		return 0;
	}

	if (p_net->job) {
		// all-map-used
		if (p_net->map) {
			return p_net->job + p_net->map[job_id];
		}
		// 1-map-1
		return p_net->job + job_id;
	}
	return 0;
}

UINT32 kflow_ai_net_job_id(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job)
{
	if (p_net == NULL) {
		return 0xffffffff;
	}
	
	if (p_net->job) {
		//return (UINT32)(p_job - p_net->job);
		return (UINT32)(p_job->job_id);
	}
	return 0xffffffff;
}

void kflow_ai_reset_net_path(UINT32 net_id)
{
	UINT32 j, i;
	KFLOW_AI_NET* p_net = NULL;
	p_net = kflow_ai_core_net(net_id);
	if (p_net == NULL) {
		return;
	}
	g_ai_net_path_open[net_id] = 0;
	for (i = 0; i < MAX_ENGINE_ID; i++) {
		KFLOW_AI_ENGINE_CTX* p_eng = kflow_engine_list[i];
		if (!p_eng) continue;
		for (j=0; j<p_eng->channel_count; j++) {
			KFLOW_AI_CHANNEL_CTX* p_ch = p_eng->p_ch[j];
			if (!strcmp(p_eng->name, "cpu")) {
				if (p_ch->state == 2) { //start
					//force stop
					DBG_DUMP(" <0> engine[%s] ch[%s] reset_path\r\n", p_eng->name, p_ch->name);
					kflow_cpu_ch1_reset_path(p_ch, net_id);
				}
			}
			if (!strcmp(p_eng->name, "dsp")) {
				if (p_ch->state == 2) { //start
					//force stop
					DBG_DUMP(" <0> engine[%s] ch[%s] reset_path\r\n", p_eng->name, p_ch->name);
					kflow_dsp_ch1_reset_path(p_ch, net_id);
				}
			}
		}
	}

	g_ai_net_path_need_reset[net_id] = 1;
	g_ai_net_path_is_used[net_id] = 0;
}
