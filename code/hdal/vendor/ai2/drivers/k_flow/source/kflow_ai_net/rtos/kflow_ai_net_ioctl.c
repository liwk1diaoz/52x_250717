/**
	@brief Source file of IO control of vendor net flow sample.

	@file net_flow_sample_ioctl.c

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#if defined(_BSP_NA51055_)
#include "rtos_na51055/top.h"
#elif defined(_BSP_NA51089_)
#include "rtos_na51089/top.h"
#endif
#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/nn_diff.h"
//#include "kflow_ai_net_ioctl.h"
//#include "kflow_ai_net_parm.h"
#include "../kflow_ai_net_int.h"
//=============================================================
#define __CLASS__ 				"[ai][kflow][ictl]"
#include "kflow_ai_debug.h"
//=============================================================

#include "kdrv_ai.h" //for NEW_AI_FLOW

#include "kflow_ai_net/kflow_ai_core.h"
#include "kflow_ai_net/kflow_ai_core_callback.h"

#include "kflow_cnn/kflow_cnn.h"
#include "kflow_nue/kflow_nue.h"
#include "kflow_nue2/kflow_nue2.h"
#include "kflow_cpu/kflow_cpu.h"
#include "kflow_dsp/kflow_dsp.h"

#include "kflow_cpu/kflow_cpu_callback.h"


/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

//static void *vendor_ais_buf_va_addr = NULL;     //???!! from structure changed to void

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/

extern unsigned int nvt_ai_user2kerl_va(unsigned int addr, UINT32 net_id);
extern unsigned int nvt_ai_kerl2user_va(unsigned int addr, UINT32 net_id);
void kflow_ai_install_cmd(void);
void kflow_ai_uninstall_cmd(void);

static void kflow_cpu_exec(KFLOW_AI_JOB* p_job)
{
	if (p_job->state == 13) {
		//force stop (cancel last CPU job for reset after ctrl-c)
		kflow_ai_cpu_sig(p_job->proc_id, p_job);
		return;
	}
	
	//call user space to do CPU exec
	kflow_ai_cpu_cb(p_job->proc_id, p_job);
}


int vendor_ais_flow_miscdev_ioctl(int fd, unsigned int cmd, void *arg)
{
	int ret = 0;

	KFLOW_AI_NET* p_net = NULL;
	KFLOW_AI_JOB* p_job = NULL;
	KFLOW_AI_JOB* p_next_job = NULL;

	switch (cmd) {
	case VENDOR_AIS_FLOW_IOC_NET_RESET:
		nvt_ai_reset_net();
		break;
			
	case VENDOR_AIS_FLOW_IOC_NET_INIT: {
			VENDOR_AIS_FLOW_ID *id_info  = (VENDOR_AIS_FLOW_ID *)arg;
			nvt_ai_init_net(id_info);
		}
		break;
		
	case VENDOR_AIS_FLOW_IOC_NET_UNINIT:
		nvt_ai_uninit_net();
		break;
		
	case VENDOR_AIS_FLOW_IOC_NET_LOCK: {
			VENDOR_AIS_FLOW_ID *id_info  = (VENDOR_AIS_FLOW_ID *)arg;
			nvt_ai_lock_net(id_info->net_id);
		}
		break;
			
	case VENDOR_AIS_FLOW_IOC_NET_UNLOCK: {
			VENDOR_AIS_FLOW_ID *id_info  = (VENDOR_AIS_FLOW_ID *)arg;
			nvt_ai_unlock_net(id_info->net_id);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_COMMON_LOCK:
		nvt_ai_comm_lock();	
		break;

	case VENDOR_AIS_FLOW_IOC_COMMON_UNLOCK:
		nvt_ai_comm_unlock();	
		break;	 

	case VENDOR_AIS_FLOW_IOC_REMAP_ADDR: {
			VENDOR_AIS_FLOW_MAP_MEM_INFO *map_mem_info = (VENDOR_AIS_FLOW_MAP_MEM_INFO *)arg;
			nvt_ai_open_net(&map_mem_info->parm, map_mem_info->net_id);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_UNMAP_ADDR: {
			VENDOR_AIS_FLOW_MAP_MEM_INFO *map_mem_info = (VENDOR_AIS_FLOW_MAP_MEM_INFO *)arg;
			nvt_ai_close_net(map_mem_info->net_id);
			nvt_ai_dynamic_batch_uninit(map_mem_info->net_id);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_PARS_MODEL: {
			VENDOR_AIS_FLOW_MAP_MEM_INFO *map_mem_info = (VENDOR_AIS_FLOW_MAP_MEM_INFO *)arg;
			if (nvt_ai_is_dynamic_batch(map_mem_info->net_id))
			{
				nvt_ai_dynamic_batch_init(map_mem_info->net_id);
			}
			nvt_ai_pars_net(&map_mem_info->parm, map_mem_info->net_id);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_CORE_RESET:
		kflow_ai_core_reset();
		break;
		
	case VENDOR_AIS_FLOW_IOC_CORE_INIT:
			kflow_ai_core_reset_engine();
			kflow_ai_core_add_engine(0, kflow_cnn_get_engine()); // 0 = VENDOR_AI_ENGINE_CNN
			kflow_ai_core_add_engine(1, kflow_nue_get_engine()); // 1 = VENDOR_AI_ENGINE_NUE
			kflow_ai_core_add_engine(2, kflow_nue2_get_engine()); // 2 = VENDOR_AI_ENGINE_NUE2
			kflow_ai_core_add_engine(8, kflow_cpu_get_engine()); // 8 = VENDOR_AI_ENGINE_CPU
			kflow_ai_core_add_engine(9, kflow_dsp_get_engine()); // 9 = VENDOR_AI_ENGINE_DSP
			kflow_cpu_reg_exec_cb(kflow_cpu_exec);
			{
				//KFLOW_AI_ENGINE_CTX* p_eng = kflow_cpu_get_engine();
				//p_eng->p_ch[0]->trigger = kflow_cpu_ch1_trig; //hook
			}
			kflow_ai_core_init();
		break;
	case VENDOR_AIS_FLOW_IOC_CORE_UNINIT:
		kflow_ai_core_uninit();
		break;
	case VENDOR_AIS_FLOW_IOC_CORE_CFGSCHD: {
			VENDOR_AIS_FLOW_CORE_CFG *core_cfg = (VENDOR_AIS_FLOW_CORE_CFG *)arg;
			kflow_ai_core_cfgschd(core_cfg->schd);
		}
		break;
	case VENDOR_AIS_FLOW_IOC_NEW_JOBLIST: {
			VENDOR_AIS_FLOW_JOBLIST_INFO *joblist_info = (VENDOR_AIS_FLOW_JOBLIST_INFO *)arg;
			p_net = kflow_ai_core_net(joblist_info->proc_id);
			kflow_ai_net_create(p_net, joblist_info->max_job_cnt, joblist_info->job_cnt, joblist_info->bind_cnt, joblist_info->ddr_id);
		}
		break;
	case VENDOR_AIS_FLOW_IOC_DUMP_JOBLIST: {
			VENDOR_AIS_FLOW_JOBLIST_INFO *joblist_info = (VENDOR_AIS_FLOW_JOBLIST_INFO *)arg;
			p_net = kflow_ai_core_net(joblist_info->proc_id);
			kflow_ai_net_dump(p_net, joblist_info->job_cnt); //job_cnt = info
		}
		break;
	case VENDOR_AIS_FLOW_IOC_DEL_JOBLIST: {
			VENDOR_AIS_FLOW_JOBLIST_INFO *joblist_info = (VENDOR_AIS_FLOW_JOBLIST_INFO *)arg;
			p_net = kflow_ai_core_net(joblist_info->proc_id);
			kflow_ai_net_destory(p_net);
		}
		break;
	case VENDOR_AIS_FLOW_IOC_CLR_JOB: {
		VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			p_net = kflow_ai_core_net(job_info->proc_id);
			p_job = kflow_ai_net_job(p_net, job_info->job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info->job_id);
				ret = 0;
				goto exit;
			}
			kflow_ai_core_clr_job(p_net, p_job);
		}
		break;
	case VENDOR_AIS_FLOW_IOC_SET_JOB: {
			//job_info.info is a modectrl ptr!, convert user va to kernel va
			VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			UINT32 info = nvt_ai_user2kerl_va((unsigned int)job_info->info, job_info->proc_id);
			UINT32 info2 = nvt_ai_user2kerl_va((unsigned int)job_info->info2, job_info->proc_id);
			INT32 wait_ms = job_info->wait_ms;
			p_net = kflow_ai_core_net(job_info->proc_id);
			p_job = kflow_ai_net_job(p_net, job_info->job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info->job_id);
				ret = 0;
				goto exit;
			}
			p_job->engine_id = job_info->engine_id;
			p_job->engine_op = job_info->engine_op;
			p_job->schd_parm = job_info->schd_parm;
			p_job->wait_ms = job_info->wait_ms;
			p_job->p_eng = kflow_ai_core_get_engine(p_job->engine_id); //assign this job's engine!
			if (p_job->p_eng == 0) {
				DBG_ERR("proc[%d] job[%d], invalid engine_id=%d?\r\n", p_net->proc_id, (int)job_info->job_id, (int)p_job->engine_id);
				ret = 0;
				goto exit;
			}
			//kflow_ai_core_set_job(p_net, p_job, job_info->engine_id, job_info->engine_op, job_info->schd_parm, (void*)info, (void*)info2, wait_ms);
			kflow_ai_core_set_job(p_net, p_job, (void*)info, (void*)info2, wait_ms);
		}
		break;
	case VENDOR_AIS_FLOW_IOC_SET_JOB2:	{
			//job_info.info is a modectrl ptr!, convert user va to kernel va
			VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			UINT32 info = nvt_ai_user2kerl_va((unsigned int)job_info->info, job_info->proc_id);
			//UINT32 info2 = nvt_ai_user2kerl_va((unsigned int)job_info->info2, job_info->proc_id);
			INT32 wait_ms = job_info->wait_ms;
			
			nvt_ai_copy_net_from_user(job_info->proc_id);
			
			p_net = kflow_ai_core_net(job_info->proc_id);
			p_job = kflow_ai_net_job(p_net, job_info->job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info->job_id);
				ret = 0;
				goto exit;
			}
			p_job->engine_id = job_info->engine_id;
			p_job->engine_op = job_info->engine_op;
			p_job->schd_parm = job_info->schd_parm;
			p_job->wait_ms = job_info->wait_ms;
			p_job->p_eng = kflow_ai_core_get_engine(p_job->engine_id); //assign this job's engine!
			if (p_job->p_eng == 0) {
				DBG_ERR("proc[%d] job[%d], invalid engine_id=%d?\r\n", p_net->proc_id, (int)job_info->job_id, (int)p_job->engine_id);
				ret = 0;
				goto exit;
			}
			//kflow_ai_core_set_job(p_net, p_job, job_info->engine_id, job_info->engine_op, (void*)info, (void*)info2, wait_ms);
			kflow_ai_core_set_job(p_net, p_job, (void*)info, 0, wait_ms);
			}
			break;
	case VENDOR_AIS_FLOW_IOC_BIND_JOB: {
			VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			p_net = kflow_ai_core_net(job_info->proc_id);
			p_job = kflow_ai_net_job(p_net, job_info->job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info->job_id);
				ret = 0;
				goto exit;
			}
			p_next_job = kflow_ai_net_job(p_net, job_info->info);
			if (p_next_job == NULL) {
				DBG_ERR("proc[%d] set end job_id=%d\r\n", p_net->proc_id, (int)job_info->job_id);
			}
			kflow_ai_core_bind_job(p_net, p_job, p_next_job);
		}
		break;
	case VENDOR_AIS_FLOW_IOC_UNBIND_JOB: {
			VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			p_net = kflow_ai_core_net(job_info->proc_id);
			kflow_ai_core_sum_job(p_net, &job_info->info, &job_info->info2);
		}
		break;
	case VENDOR_AIS_FLOW_IOC_LOCK_JOB: {
			VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			p_net = kflow_ai_core_net(job_info->proc_id);
			p_job = kflow_ai_net_job(p_net, job_info->job_id);
			if (p_job == 0) {
				job_info->wait_ms = -1;
			} else {
				kflow_ai_core_lock_job(p_net, p_job);
				job_info->wait_ms = 0;
			}
		}
		break;
	case VENDOR_AIS_FLOW_IOC_UNLOCK_JOB: {
			VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			p_net = kflow_ai_core_net(job_info->proc_id);
			p_job = kflow_ai_net_job(p_net, job_info->job_id);
			if (p_job == 0) {
				job_info->wait_ms = -1;
			} else {
				kflow_ai_core_unlock_job(p_net, p_job);
				job_info->wait_ms = 0;
			}
		}
		break;
	case VENDOR_AIS_FLOW_IOC_PUSH_JOB: {
			VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			p_net = kflow_ai_core_net(job_info->proc_id);
			if (job_info->job_id >= 0xf0000000) {
				if (job_info->job_id == 0xf0000001) {
					//BEGIN
					kflow_ai_core_push_begin(p_net);
				} else if (job_info->job_id == 0xf0000002) {
					//END
					kflow_ai_core_push_end(p_net);
				}
			} else {
				p_job = kflow_ai_net_job(p_net, job_info->job_id);
				if (p_job == 0) {
					job_info->wait_ms = -1;
				} else {
					kflow_ai_core_push_job(p_net, p_job);
					job_info->wait_ms = 0;
				}
			}
		}
		break;
	case VENDOR_AIS_FLOW_IOC_PULL_JOB: {
			VENDOR_AIS_FLOW_JOB_INFO *job_info = (VENDOR_AIS_FLOW_JOB_INFO *)arg;
			p_net = kflow_ai_core_net(job_info->proc_id);
			if (job_info->job_id >= 0xf0000000) {
				if (job_info->job_id == 0xf0000001) {
					//BEGIN
					kflow_ai_core_pull_begin(p_net);
				} else if (job_info->job_id == 0xf0000002) {
					//END
					kflow_ai_core_pull_end(p_net);
				} else if (job_info->job_id == 0xf0000003) {
					//ready
					p_job = 0;
					kflow_ai_core_pull_ready(p_net, p_job);
				} else if (job_info->job_id == 0xf0000004) {
					KFLOW_AI_JOB* pull_job;
					//ready
					pull_job = 0;
					job_info->rv = kflow_ai_core_pull_job(p_net, &pull_job);
					if (job_info->rv == 0) {
						//DBG_DUMP("->>> proc[%d] - IOCTL ok\r\n", (int)job_info->proc_id);
						job_info->wait_ms = 0;
						job_info->job_id = pull_job->job_id;
						job_info->info = 0; //HD_ERR_EOL
					} else if (job_info->rv > 0) {
						job_info->wait_ms = 0;
						job_info->job_id = pull_job->job_id;
						job_info->info = 1;
					} else if (job_info->rv == -2) {
						job_info->wait_ms = 0;
						DBG_ERR("proc[%d] PULL_JOB abort.\r\n", p_net->proc_id);
					} else {
						//DBG_DUMP("->>> proc[%d] - IOCTL cancel\r\n", (int)job_info->proc_id);
						job_info->wait_ms = -1;
					}
				}
			}
		}
		break;
/*
	case VENDOR_AIS_FLOW_IOC_WAI_JOB: {
			VENDOR_AIS_FLOW_JOB_WAI *p_cmd  = (VENDOR_AIS_FLOW_JOB_WAI *)arg;
			//DBG_IND("ioctl OUT_WAI: open\r\n");
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			p_job = kflow_ai_core_cb_wait(p_cmd->proc_id);
			if (p_job == NULL) {
				p_cmd->job_id = 0xffffffff;
			} else {
				p_cmd->job_id = kflow_ai_net_job_id(p_net, p_job);
				if (p_cmd->job_id == 0xffffffff) {
					DBG_ERR("proc[%d] invalid job_id?\r\n", p_net->proc_id);
				}
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_SIG_JOB: {
			VENDOR_AIS_FLOW_JOB_SIG *p_cmd  = (VENDOR_AIS_FLOW_JOB_SIG *)arg;
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			//DBG_IND("ioctl OUT_LOG: close\r\n");
			p_job = kflow_ai_net_job(p_net, p_cmd->job_id);
			if (p_job == 0)
			    p_job = (KFLOW_AI_JOB*)0xffffffff;
			kflow_ai_core_cb_sig(p_cmd->proc_id, p_job);
		}
		break;
*/
	case VENDOR_AIS_FLOW_IOC_WAI_CPU: {
			VENDOR_AIS_FLOW_CPU_WAI *p_cmd = (VENDOR_AIS_FLOW_CPU_WAI *)arg;
#if NN_DLI
			UINT32 event = 0;
#endif
			//DBG_IND("ioctl OUT_WAI: open\r\n");
			p_net = kflow_ai_core_net(p_cmd->proc_id);
#if NN_DLI
			p_job = kflow_ai_cpu_wait(p_cmd->proc_id, &event);
#else
			p_job = kflow_ai_cpu_wait(p_cmd->proc_id);
#endif
			if (p_job == NULL) {
				p_cmd->job_id = 0xffffffff;
			} else {
				p_cmd->job_id = kflow_ai_net_job_id(p_net, p_job);
				if (p_cmd->job_id == 0xffffffff) {
					DBG_ERR("proc[%d] invalid job_id?\r\n", p_net->proc_id);
				}
#if NN_DLI
				if (event == 0xff) {
					p_cmd->job_id |= 0xff000000; //start
				}
				if (event == 0xfe) {
					p_cmd->job_id |= 0xfe000000; //stop
				}
				//event == 0  //proc
#endif
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_SIG_CPU: {
			VENDOR_AIS_FLOW_CPU_SIG *p_cmd  = (VENDOR_AIS_FLOW_CPU_SIG *)arg;
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			//DBG_IND("ioctl OUT_LOG: close\r\n");
#if NN_DLI
			p_cmd->job_id &= ~0xff000000; //clean start/stop flag
#endif
			p_job = kflow_ai_net_job(p_net, p_cmd->job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] invalid job_id=%d?\r\n", p_net->proc_id, (int)p_cmd->job_id);
				ret = 0;
				goto exit;
			}
			kflow_ai_cpu_sig(p_cmd->proc_id, p_job);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_INPUT_INIT: {
			VENDOR_AIS_FLOW_PROC_INPUT_INFO *input_info = (VENDOR_AIS_FLOW_PROC_INPUT_INFO *)arg;
#if CNN_25_MATLAB
			nvt_ai_set_input(input_info->net_addr, &input_info->iomem, input_info->net_id);
#else	
			nvt_ai_set_input(input_info->net_addr, input_info->imem, NN_IMEM_NUM, input_info->net_id);
#endif
		}
		break;

	case VENDOR_AIS_FLOW_IOC_INPUT_UNINIT: {
			VENDOR_AIS_FLOW_PROC_INPUT_INFO *input_info = (VENDOR_AIS_FLOW_PROC_INPUT_INFO *)arg;
			nvt_ai_clr_input(input_info->net_addr, input_info->net_id);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_UPDATE_PARM: {
			VENDOR_AIS_FLOW_UPDATE_INFO *up_info = (VENDOR_AIS_FLOW_UPDATE_INFO *)arg;
			nvt_ai_update_layer(up_info->layer, up_info->net_id);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_LL_BASE:
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	{
		VENDOR_AIS_FLOW_LL_BASE *ll_base = (VENDOR_AIS_FLOW_LL_BASE *)arg;
		if(nvt_get_chip_id() == CHIP_NA51084) { // only 528 IC support
			nvt_ai_set_ll_base(ll_base);
		} else {
			DBG_ERR("VENDOR_AIS_FLOW_IOC_LL_BASE only support for 528\r\n");
		}
	}
#else
		DBG_ERR("VENDOR_AIS_FLOW_IOC_LL_BASE only support for 528\r\n");
#endif
		break;

	// only for FPGA test
	case VENDOR_AIS_FLOW_IOC_MEM_OFS: {
		VENDOR_AIS_FLOW_MEM_OFS *mem_ofs  = (VENDOR_AIS_FLOW_MEM_OFS *)arg;
		nvt_ai_set_mem_ofs(mem_ofs);
	}
		break;

	case VENDOR_AIS_FLOW_IOC_VERS:
		{
			VENDOR_AIS_FLOW_VERS *vers_info = (VENDOR_AIS_FLOW_VERS *)arg;
			vers_info->rv = nvt_ai_chk_vers(vers_info);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_PROC_UPDATE_INFO:
		{
			VENDOR_AIS_FLOW_UPDATE_NET_INFO *up_net_info = (VENDOR_AIS_FLOW_UPDATE_NET_INFO *)arg;
			nvt_ai_update_net_online(&up_net_info->map_parm, (NN_DIFF_MODEL_HEAD *)up_net_info->net_info_pa, up_net_info->model_id, up_net_info->net_id);
			break;
		}

	case VENDOR_AIS_FLOW_IOC_RESTORE_UPDATE_INFO:
		{
			VENDOR_AIS_FLOW_UPDATE_NET_INFO *up_net_info = (VENDOR_AIS_FLOW_UPDATE_NET_INFO *)arg;
			nvt_ai_restore_net_online(&up_net_info->map_parm, (NN_DIFF_MODEL_HEAD *)up_net_info->net_info_pa, up_net_info->model_id, up_net_info->net_id);
			break;
		}

	case KFLOW_AI_IOC_CMD_DMA_ABORT:
		{
			kdrv_ai_dma_abort(0, AI_ENG_TOTAL);
			break;
		}

	default :
		break;
	}

exit:

	return ret;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int vendor_ais_flow_miscdev_init(void)
{
	int ret = 0;

	kflow_ai_install_cmd();

	return ret;
}

void vendor_ais_flow_miscdev_exit(void)
{
	kflow_ai_uninstall_cmd();
}
