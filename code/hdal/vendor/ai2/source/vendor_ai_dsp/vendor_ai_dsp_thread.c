/**
	@brief Source file of cpu1 processing flow.

	@file vendor_ai_cpu.c

	@ingroup vendor_ai_cpu

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#else
#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#endif
#include <string.h>
#include "hd_type.h"
#include "hd_common.h"

#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

#include "vendor_ai.h"
#include "vendor_ai_plugin.h"

#include "kflow_ai_net/kflow_ai_net.h"
#include "vendor_ai_comm_flow.h"

//=============================================================
#define __CLASS__ 				"[ai][lib][dsp]"
#include "vendor_ai_debug.h"
//=============================================================

#if (NET_TASK_HEAP == 1)
#include "vendor_ai_comm.h"
#else
typedef struct _VENDOR_AI_DSP_TASK {
	UINT32 proc_id;
	int en;
	pthread_t  thread_id;
} VENDOR_AI_DSP_TASK;
#endif

extern INT32 *vendor_ais_flow_fd;
extern UINT32 g_ai_support_net_max;

//////////////////////////////////////////////////////////////////////////////////
static INT g_ai_dsp_task_init_cnt = 0;
#if (NET_TASK_HEAP == 1)
#else
VENDOR_AI_DSP_TASK *_vendor_ai_dsp_task;
#endif

static void *_vendor_ai_dsp_callback_thread(void *arg);
static VENDOR_AI_ENG_CB _vendor_ai_dsp_callback = 0;

HD_RESULT vendor_ai_dsp_thread_init_task(VOID)
{
	if (g_ai_dsp_task_init_cnt == 0) {
#if (NET_TASK_HEAP == 1)
#else
		_vendor_ai_dsp_task = (VENDOR_AI_DSP_TASK *)vendor_ai_malloc(sizeof(VENDOR_AI_DSP_TASK) * g_ai_support_net_max);
		if (_vendor_ai_dsp_task == NULL) {
			return HD_ERR_NOMEM;
		}
		memset(_vendor_ai_dsp_task, 0x0, sizeof(VENDOR_AI_DSP_TASK) * g_ai_support_net_max);
#endif
		g_ai_dsp_task_init_cnt = 1;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dsp_thread_uninit_task(VOID)
{
	if (g_ai_dsp_task_init_cnt) {
#if (NET_TASK_HEAP == 1)
#else
		if (_vendor_ai_dsp_task) {
			vendor_ai_free(_vendor_ai_dsp_task, sizeof(VENDOR_AI_DSP_TASK) * g_ai_support_net_max);
		}
#endif
		g_ai_dsp_task_init_cnt = 0;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dsp_thread_reg_cb(VENDOR_AI_ENG_CB fp)
{
	_vendor_ai_dsp_callback = fp;
	return HD_OK;
}

HD_RESULT vendor_ai_dsp_thread_init(UINT32 proc_id)
{
	int ret;
#if (NET_TASK_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_DSP_TASK* p_cb_task = &(p_proc->dsp_task);
#else
	VENDOR_AI_DSP_TASK* p_cb_task = _vendor_ai_dsp_task + proc_id;
#endif
	p_cb_task->proc_id = proc_id;
	p_cb_task->en = 1;
	p_cb_task->thread_id = 0;
	ret = pthread_create(&p_cb_task->thread_id, NULL, _vendor_ai_dsp_callback_thread, (void *)p_cb_task);
	if (ret < 0) {
		//DBG_ERR("create dsp thread fail? %d\r\n", ret);
		return HD_ERR_FAIL;
	}
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#else
    {
        char thread_name[20] = {0};
        sprintf(thread_name, "ai_dsp_tsk[%d]", (int)proc_id);
        ret = pthread_setname_np(p_cb_task->thread_id, thread_name);
    }
    if (ret < 0) {
        //DBG_ERR("name thread fail? %d\r\n", ret);
        return HD_OK;
    }
#endif
	return HD_OK;
}

HD_RESULT vendor_ai_dsp_thread_exit(UINT32 proc_id)
{
#if (NET_TASK_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_DSP_TASK* p_cb_task = &(p_proc->dsp_task);
#else
	VENDOR_AI_DSP_TASK* p_cb_task = _vendor_ai_dsp_task + proc_id;
#endif

	if (p_cb_task->thread_id == 0) {
		//DBG_ERR("create dump thread fail? %d\r\n", r);
		return HD_ERR_FAIL;
	}
	// destroy thread
	pthread_join(p_cb_task->thread_id, NULL);

	return HD_OK;
}

static void *_vendor_ai_dsp_callback_thread(void *arg)
{
	int r;
	VENDOR_AI_DSP_TASK* p_cb_task = (VENDOR_AI_DSP_TASK*)arg;
	VENDOR_AIS_FLOW_DSP_WAI wai_cmd = {0};
	VENDOR_AIS_FLOW_DSP_SIG sig_cmd = {0};
	UINT32 proc_id = p_cb_task->proc_id;

	while(p_cb_task->en) {
		// wait log cmd
		wai_cmd.proc_id = proc_id;
		r = KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_WAI_DSP, &wai_cmd);
		if(r < 0) {
			DBG_ERR("callback wait fail?\r\n");
			break; //quit now!
		}
		if (wai_cmd.job_id == 0xffffffff) {
			p_cb_task->en = 0; //quit this loop!
			break;
		} else {
			sig_cmd.proc_id = proc_id;
			sig_cmd.job_id = wai_cmd.job_id;
			//printf("user: job[%d] dsp exec - begin \r\n", (int)wai_cmd.job_id);
			if (_vendor_ai_dsp_callback)
				_vendor_ai_dsp_callback(proc_id, wai_cmd.job_id);
			//printf("user: job[%d] dsp exec - end\r\n", (int)wai_cmd.job_id);
		}
		if (p_cb_task->en) {
			sig_cmd.proc_id = proc_id;
			sig_cmd.job_id = wai_cmd.job_id;
			// signal log cmd
			r = KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_SIG_DSP, &sig_cmd);
			if(r < 0) {
				DBG_ERR("callback signal fail?\r\n");
				break; //quit now!
			}
			wai_cmd.proc_id = proc_id;
			wai_cmd.job_id = 0;
		}
	}
	return 0;
}




