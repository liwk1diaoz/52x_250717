/**
	@brief Source file of vendor user-space net flow sample.

	@file net_flow_user_sample.c

	@ingroup net_flow_user_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/platform.h"
#include <stdio.h>
#include <string.h>
#include "hd_type.h"
#include "hd_common.h"

#include "vendor_common.h"
#include "ai_ioctl.h"
#include "nvt_ai.h"
#include "kdrv_ai.h"
#include "vendor_ai_comm.h"
#include "vendor_ai_dla/vendor_ai_dla.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_dsp/vendor_ai_dsp.h"
#include "vendor_ai.h"
#include "vendor_ai_net_layer.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"
#include "kflow_ai_net/nn_net.h"
#include "kflow_ai_net/nn_parm.h"
#include "kflow_ai_net/nn_diff.h"
#include "vendor_ai_net_flow.h"
#include "vendor_ai_net_group.h"
#include "vendor_ai_net_debug.h"
#include "vendor_ai_net_cmd.h"
#include "vendor_ai_comm_flow.h"
#include "vendor_ai_net_gen.h"
#include "vendor_ai_version.h"

//=============================================================
#define __CLASS__ 				"[ai][lib][flow]"
#include "vendor_ai_debug.h"
//=============================================================

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define OUT_PATH            "output"
#define DEFAULT_BUFSZ   	0x8000

#define ENG_TIME_OUT        30000       //time (ms)
#define NN_RPOC_KERL_SWITCH     DISABLE
#define NN_DUMP_LAYER           DISABLE

#define PROF                    DISABLE
#if PROF
	static struct timeval tstart, tend;
	#define PROF_START()    gettimeofday(&tstart, NULL);
	#define PROF_END(msg)   gettimeofday(&tend, NULL);  \
			printf("%s time (us): %lu\r\n", msg,         \
					(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec));
#else
	#define PROF_START()
	#define PROF_END(msg)
#endif

#define ENABLE_IN_PADDING 		ENABLE

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

#if (NEW_AI_FLOW == 1)
UINT32 g_ai_support_net_max = 16;
INT32 *vendor_ais_flow_fd;
#else
static INT vendor_ais_flow_fd[NN_SUPPORT_NET_MAX] = {-1};
#endif
static VENDOR_AI_NET_PROC_CB g_ext_cb_fp[4] = {0};
static UINT32 vendor_ais_init_cnt = 0;

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_ais_set_results(NN_GEN_MODE_CTRL *p_last_mctrl, VENDOR_AIS_FLOW_MEM_PARM *p_rslt_mem, UINT32 net_id);

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern BOOL _vendor_ai_net_is_optimize_job(VENDOR_AI_NET_JOB_OPT job_opt);
extern BOOL _vendor_ai_net_is_linear_job(VENDOR_AI_NET_JOB_OPT job_opt);
extern BOOL _vendor_ai_net_is_graph_job(VENDOR_AI_NET_JOB_OPT job_opt);
extern BOOL is_multi_process;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#if CNN_AI_FASTBOOT
UINT32 vendor_ais_get_fastboot_dump_enable(UINT32 proc_id)
{
	UINT32 enable;

	if (vendor_ais_flow_fd[0] < 0) {
		DBG_ERR("%s: vendor ai ioctl not init...\r\n", __func__);
		return 0;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_GET_FBOOT_DUMP_EN, &enable) < 0) {
		//fprintf(stderr, "VENDOR_AIS_FLOW_IOC_SET_FBOOT_DUMP_EN ioctl failed\r\n");
		vendor_ai_errno_location();
		return 0;
	} else {
		return enable;
	}
}

HD_RESULT vendor_ais_get_fastboot_rslt(KFLOW_AI_BUILTIN_RSLT_INFO *p_rslt_info)
{
	UINT32 proc_id = 0;

	if (vendor_ais_flow_fd[proc_id+1] < 0) {
		DBG_ERR("%s: vendor ai ioctl not init...\r\n", __func__);
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_GET_FBOOT_RSLT, p_rslt_info) < 0) {
		//fprintf(stderr, "VENDOR_AIS_FLOW_IOC_KERL_START_MEM ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
}

HD_RESULT vendor_ais_get_kerl_start_mem(VENDOR_AIS_FLOW_KERL_START_MEM *p_kerl_start)
{
	UINT32 proc_id = p_kerl_start->proc_id;

	if (vendor_ais_flow_fd[proc_id+1] < 0) {
		DBG_ERR("%s: vendor ai ioctl not init...\r\n", __func__);
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_KERL_START_MEM, p_kerl_start) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_KERL_START_MEM ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
}

INT vendor_ais_is_fastboot(void)
{
	INT is_fastboot = 0;

	if (vendor_ais_flow_fd[0] <= 0) {
		DBG_ERR("%s: vendor ai ioctl not init...\r\n", __func__);
		return 0;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_IS_FASTBOOT, &is_fastboot) < 0) {
		//fprintf(stderr, "VENDOR_AIS_FLOW_IOC_IS_FASTBOOT ioctl failed\r\n");
		vendor_ai_errno_location();
		return 0;
	}
	return is_fastboot;
}

HD_RESULT vendor_ais_get_fastboot_builtin_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem)
{
	if (vendor_ais_flow_fd[0] <= 0) {
		DBG_ERR("%s: vendor ai ioctl not init...\r\n", __func__);
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_GET_BUILTIN_MEM, p_mem) < 0) {
		//fprintf(stderr, "VENDOR_AIS_FLOW_IOC_GET_BUILTIN_MEM ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}
#endif

HD_RESULT vendor_ais_set_ext_cb(UINT32 engine_id, VENDOR_AI_NET_PROC_CB fp)
{
	g_ext_cb_fp[engine_id] = fp;
	return HD_OK;
}

#if (NEW_AI_FLOW == 1)
//static HD_RESULT _vendor_ai_job_callback_thread_init(UINT32 proc_id);
//static HD_RESULT _vendor_ai_job_callback_thread_exit(UINT32 proc_id);
#endif

HD_RESULT vendor_ais_init_net(VOID)
{
	vendor_ai_net_gen_context_init();
	vendor_ai_cpu_thread_init_task();
	vendor_ai_dsp_thread_init_task();
	vendor_ai_net_debug_init();

	return HD_OK;
}

HD_RESULT vendor_ais_uninit_net(VOID)
{
	VENDOR_AIS_FLOW_ID id_info = {0};

	vendor_ai_cpu_thread_uninit_task();
	vendor_ai_dsp_thread_uninit_task();
	vendor_ai_net_gen_context_uninit();
	vendor_ai_net_debug_uninit();

	if (vendor_ais_flow_fd[0] != -1) {
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_NET_UNINIT, &id_info) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_NET_UNINIT ioctl failed\r\n");
			vendor_ai_errno_location();
			return HD_ERR_ABORT;
		}
	}
	return HD_OK;
}

static HD_RESULT _vendor_ais_open_fd(UINT32 net_id)
{
	CHAR device[256] = {0};
	if (is_multi_process) {
		snprintf(device, 256, DEFAULT_DEVICE"%lu", net_id+1);
		vendor_ais_flow_fd[net_id+1] = KFLOW_AI_OPEN(device, O_RDWR);
		if (vendor_ais_flow_fd[net_id+1] == -1) {
			fprintf(stderr, "open %s failed\r\n", device);
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	} else {
		snprintf(device, 256, DEFAULT_DEVICE"%i", MAX_PROC_CNT + 1);
		vendor_ais_flow_fd[net_id+1] = KFLOW_AI_OPEN(device, O_RDWR);
		if (vendor_ais_flow_fd[net_id+1] == -1) {
			fprintf(stderr, "open %s failed\r\n", device);
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}
	return HD_OK;
}

static HD_RESULT _vendor_ais_close_fd(UINT32 net_id)
{
	if (is_multi_process) {
		CHAR device[256] = {0};
		snprintf(device, 256, DEFAULT_DEVICE"%lu", net_id+1);
		if (vendor_ais_flow_fd[net_id+1] != -1) {
			//DBG_DUMP("close %s\r\n", device);
			if (KFLOW_AI_CLOSE(vendor_ais_flow_fd[net_id+1]) == -1) {
				fprintf(stderr, "close %s failed\r\n", device);
				vendor_ai_errno_location();
				return HD_ERR_NG;
			}
		}
	} else {
		if (vendor_ais_flow_fd[net_id+1] != -1) {
			//DBG_DUMP("close %s\r\n", device);
			if (KFLOW_AI_CLOSE(vendor_ais_flow_fd[net_id+1]) == -1) {
				fprintf(stderr, "close %s failed\r\n", DEFAULT_DEVICE);
				vendor_ai_errno_location();
				return HD_ERR_NG;
			}
		}
	}

	return HD_OK;
}

HD_RESULT vendor_ais_comm_lock(void)
{

	if (vendor_ais_init_cnt != 0)
		return HD_ERR_STATE;

	// ai net uninit
	if (vendor_ais_flow_fd[0] != -1) {
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0] , VENDOR_AIS_FLOW_IOC_COMMON_LOCK, 0) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_COMMON_LOCK ioctl failed\r\n");
			vendor_ai_errno_location();
			return HD_ERR_ABORT;
		}
	}
	return HD_OK;
}

HD_RESULT vendor_ais_comm_unlock(void)
{
	if (vendor_ais_init_cnt != 0)
		return HD_ERR_STATE;

	// ai net uninit
	if (vendor_ais_flow_fd[0] != -1) {
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0] , VENDOR_AIS_FLOW_IOC_COMMON_UNLOCK, 0) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_COMMON_UNLOCK ioctl failed\r\n");
			vendor_ai_errno_location();
			return HD_ERR_ABORT;
		}
	}
	return HD_OK;
}

HD_RESULT vendor_ais_lock_net(UINT32 net_id)
{
	VENDOR_AIS_FLOW_ID id_info = {0};
	if (vendor_ais_init_cnt != 0)
		return HD_ERR_STATE;

	// ai net uninit
	if (vendor_ais_flow_fd[0] != -1) {
		id_info.net_id = net_id;
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_NET_LOCK, &id_info) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_NET_LOCK ioctl failed\r\n");
			vendor_ai_errno_location();
			return HD_ERR_ABORT;
		}
	}
	return HD_OK;
}

HD_RESULT vendor_ais_unlock_net(UINT32 net_id)
{
	VENDOR_AIS_FLOW_ID id_info = {0};
	if (vendor_ais_init_cnt != 0)
		return HD_ERR_STATE;

	// ai net uninit
	if (vendor_ais_flow_fd[0] != -1) {
		id_info.net_id = net_id;
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_NET_UNLOCK, &id_info) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_NET_UNLOCK ioctl failed\r\n");
			vendor_ai_errno_location();
			return HD_ERR_ABORT;
		}
	}
	return HD_OK;
}

HD_RESULT vendor_ais_open_net(UINT32 net_id)
{
	
#if 0
	if (vendor_ais_flow_fd_cnt > NN_SUPPORT_NET_MAX) {
		DBG_DUMP("network number(%d) exceed limit(%d)...\r\n", (int)vendor_ais_flow_fd_cnt, NN_SUPPORT_NET_MAX);
		return HD_ERR_NG;
	} else if (vendor_ais_flow_fd_cnt < 0) {
		DBG_DUMP("network number(%d) < 0\r\n", (int)vendor_ais_flow_fd_cnt);
		return HD_ERR_NG;
	}

	if (vendor_ais_flow_fd_cnt == 0) {
		vendor_ais_flow_fd = KFLOW_AI_OPEN(DEFAULT_DEVICE, O_RDWR);
		if (vendor_ais_flow_fd == -1) {
			fprintf(stderr, "open %s failed\r\n", DEFAULT_DEVICE);
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}

	vendor_ais_flow_fd_cnt++;
#endif

#if (NEW_AI_FLOW == 1)
	// open fd
	_vendor_ais_open_fd(net_id);
#endif

	return HD_OK;
}

HD_RESULT vendor_ais_close_net(UINT32 net_id)
{
#if (NEW_AI_FLOW == 1)
	_vendor_ais_close_fd(net_id);
#endif

#if 0
	vendor_ais_flow_fd_cnt--;
	if (vendor_ais_flow_fd_cnt < 0) {
		DBG_DUMP("network number(%d) < 0\r\n", (int)vendor_ais_flow_fd_cnt);
		return HD_ERR_NG;
	}

	if (vendor_ais_flow_fd_cnt == 0) {
		if (vendor_ais_flow_fd != -1) {
			DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
			if (KFLOW_AI_CLOSE(vendor_ais_flow_fd) == -1) {
				fprintf(stderr, "close %s failed\r\n", DEFAULT_DEVICE);
				vendor_ai_errno_location();
				return HD_ERR_NG;
			}
		}
	}
#endif

	return HD_OK;
}

HD_RESULT vendor_ais_remap_kerl_mem(VENDOR_AIS_FLOW_MAP_MEM_INFO *p_info)
{
	UINT32 net_id = p_info->net_id;
	if (vendor_ais_flow_fd[net_id+1] < 0) {
		DBG_ERR("vendor ai ioctl not init...\r\n");
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_REMAP_ADDR, p_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_REMAP_ADDR ioctl failed\r\n");
		vendor_ai_errno_location();
        return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
}

HD_RESULT vendor_ais_unmap_kerl_mem(VENDOR_AIS_FLOW_MAP_MEM_INFO *p_info)
{
	UINT32 net_id = p_info->net_id;
	if (vendor_ais_flow_fd[net_id+1] < 0) {
		DBG_ERR("vendor ai ioctl not init...\r\n");
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_UNMAP_ADDR, p_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_UNMAP_ADDR ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
}

HD_RESULT vendor_ais_pars_kerl_mem(VENDOR_AIS_FLOW_MAP_MEM_INFO *p_info)
{
	UINT32 net_id = p_info->net_id;
	if (vendor_ais_flow_fd[net_id+1] < 0) {
		DBG_ERR("vendor ai ioctl not init...\r\n");
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_PARS_MODEL, p_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_PARS_MODEL ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
}

#if !CNN_25_MATLAB
HD_RESULT vendor_ais_unpars_kerl_mem(VENDOR_AIS_FLOW_MAP_MEM_INFO *p_info)
{
	UINT32 net_id = p_info->net_id;
	if (vendor_ais_flow_fd[net_id+1] < 0) {
		DBG_ERR("vendor ai ioctl not init...\r\n");
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_UNPARS_MODEL, p_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_UNPARS_MODEL ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
}
#endif

INT32 vendor_ais_flow_user_init(VENDOR_AIS_FLOW_MAP_MEM_PARM mem, UINT32 net_id)
{
#if (NEW_AI_FLOW == 1)
	return 0;
#else
	AI_DRV_MAP_MEM      drv_mem;
	drv_mem.user_pa = mem.user_parm.pa;
	drv_mem.user_va = mem.user_parm.va;
	drv_mem.kerl_pa = mem.kerl_parm.pa;
	drv_mem.kerl_va = mem.kerl_parm.va;

	/*
	    map memory with ai driver
	*/
	return vendor_ai_drv_init(drv_mem, net_id);
#endif
}

INT32 vendor_ais_flow_user_uninit(UINT32 net_id)
{
#if (NEW_AI_FLOW == 1)
	return 0;
#else
    return vendor_ai_drv_uninit(net_id);
#endif
}

#if CNN_25_MATLAB
ER vendor_ais_proc_assign_input_addr(UINT32 proc_layer_num, NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem)
{
	UINT32 process_index = 0, layer_index = 0;
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		layer_index = p_mctrl[process_index].layer_index;
		if (layer_index != 0) {
			continue;
		}

		if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_APP_AI_DRV) {
			VENDOR_AI_APP_HEAD *p_app_head = (VENDOR_AI_APP_HEAD *)p_mctrl[process_index].addr;
			switch (p_app_head->mode) {
			case AI_MODE_NEURAL: {
					VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
					//p_parm->elt.addr += p_io_mem->SAI[1].va;
				}
				break;
			case AI_MODE_ROIPOOL: {
					VENDOR_AI_ROIPOOL_PARM *p_parm = (VENDOR_AI_ROIPOOL_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
					p_parm->roi_ker.roi_addr += p_io_mem->SAI[5].va;
				}
				break;
			case AI_MODE_SVM: {
					VENDOR_AI_SVM_PARM *p_parm = (VENDOR_AI_SVM_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
			case AI_MODE_FC: {
					VENDOR_AI_FC_PARM *p_parm = (VENDOR_AI_FC_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
			case AI_MODE_PERMUTE: {
					VENDOR_AI_PERMUTE_PARM *p_parm = (VENDOR_AI_PERMUTE_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
			case AI_MODE_REORG: {
					VENDOR_AI_REORG_PARM *p_parm = (VENDOR_AI_REORG_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
            case AI_MODE_PREPROC: {
					VENDOR_PREPROC_PARM *p_parm = (VENDOR_PREPROC_PARM*)p_app_head->parm_addr;
					p_parm->in_addr[0] += p_io_mem->SAI[0].va;
                    p_parm->in_addr[1] += p_io_mem->SAI[1].va;
                    p_parm->in_addr[2] += p_io_mem->SAI[2].va;
				}
				break;
			default:
				DBG_ERR("unknown input mode(%d) in app\r\n", (int)p_mctrl[process_index].mode);
				break;
			}
		} else if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
			//========== for first layer linked list mode ==========
			VENDOR_AI_LL_HEAD *p_ll_head = (VENDOR_AI_LL_HEAD *)p_mctrl[process_index].addr;
			switch (p_ll_head->eng) {
			case AI_ENG_CNN:
			case AI_ENG_CNN2: {
					CNN_LL_PARM *p_parm = (CNN_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input.bit.addr += p_io_mem->SAI[0].va;
					//p_parm->input[1].bit.addr += p_sai[BUF_IN_IDX].va;
					//p_parm->input[2].bit.addr += p_sai[BUF_IN_IDX].va;
					//p_parm->interm_in.bit.addr += p_io_mem->SAI[1].va;
				}
				break;
			case AI_ENG_NUE: {
					NUE_LL_PARM *p_parm = (NUE_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input.bit.addr += p_io_mem->SAI[0].va;
					//p_parm->elt_in.bit.addr += p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr += p_sai[NUE_ROI_IDX].va;
				}
				break;
			case AI_ENG_NUE2: {
					NUE2_LL_PARM *p_parm = (NUE2_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input[0].bit.addr += p_io_mem->SAI[0].va;
					p_parm->input[1].bit.addr += p_io_mem->SAI[1].va;
					p_parm->input[2].bit.addr += p_io_mem->SAI[2].va;
					//p_parm->elt_in.bit.addr += p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr += p_sai[NUE_ROI_IDX].va;
				}
				break;
			default:
				DBG_ERR("unknown engine type(%d) in ll\r\n", (int)p_ll_head->eng);
				break;
			}
//========== by CCC 191004 ==========
#else
			DBG_ERR("first layer can't be linked list\r\n");
			return HD_ERR_ABORT;
#endif
		} else if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_COMMON) {
			const UINT32 parm_addr = p_mctrl[process_index].addr;
			switch (p_mctrl[process_index].mode) {
			case NN_POSTPROC: {
					NN_POSTPROC_PARM *p_parm = (NN_POSTPROC_PARM *)parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
			case NN_SOFTMAX: {
					NN_SOFTMAX_PARM *p_parm = (NN_SOFTMAX_PARM *)parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
                /*
			case NN_PREPROC: {
					NN_PRE_PARM *p_parm = (NN_PRE_PARM*)parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
				*/
			case NN_FC_POST: {
					NN_FC_POST_PARM *p_parm = (NN_FC_POST_PARM*)parm_addr;
					p_parm->addr_in += p_io_mem->SAI[0].va;
				}
				break;
			case NN_POOL: {
					NN_POOL_PARM *p_parm = (NN_POOL_PARM*)parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
			case NN_CUSTOMER: {
					NN_CUSTOM_PARM *p_parm = (NN_CUSTOM_PARM*)parm_addr;
					p_parm->input.va += p_io_mem->SAI[0].va;
					p_parm->input.pa += p_io_mem->SAI[0].pa;
				}
				break;
			case NN_BNSCALE: {
					NN_BNSCALE_PARM *p_parm = (NN_BNSCALE_PARM*)parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
			default:
				DBG_ERR("unknown input mode(%d) in common\r\n", (int)p_mctrl[process_index].mode);
				break;
			}
		} else {
			DBG_ERR("unknown first layer trigger source: %d\r\n", (int)p_mctrl[process_index].trig_src);
			return HD_ERR_ABORT;
		}
	}

	return E_OK;
}

HD_RESULT vendor_ais_proc_input_init(NN_MODE mode, NN_IOMEM *p_1st_mem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_IOMEM *p_io_mem, *p_mem_ofs;
	NN_DATA *p_in_data, *p_model_data;
	const UINT32 io_mem_cnt = sizeof(NN_IOMEM) / sizeof(NN_DATA);
	UINT32 idx = 0;
	VENDOR_AIS_FLOW_PROC_INPUT_INFO input_info = {0};
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;

	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	p_io_mem = net_info.p_io_mem;
#if CNN_MULTI_INPUT
	proc_layer_num = (vendor_ais_get_net_is_batch(net_id) == 1) ? 1 : p_head->mode_ctrl_num;
#else
	proc_layer_num = p_head->mode_ctrl_num;
#endif

	if (p_mctrl[0].mode != mode) {
		DBG_ERR("input mode(%d) isn't matched with first layer model mode(%d)\r\n", (int)mode, (int)p_mctrl[0].mode);
		return HD_ERR_ABORT;
	}

	///< update first layer io buffer
	p_in_data = (NN_DATA*)p_1st_mem;
	p_model_data = (NN_DATA*)(&p_io_mem[0]);
	for (idx = 0; idx < io_mem_cnt; idx++) {
		if (p_in_data[idx].size == 0) {
			continue;
		}
		memcpy(&p_model_data[idx], &p_in_data[idx], sizeof(NN_DATA));
	}

	///< update first layer address of parameter
	p_mem_ofs = p_1st_mem;
	vendor_ais_proc_assign_input_addr(proc_layer_num, p_mctrl, p_mem_ofs);


	input_info.net_addr = mem.va;
	memcpy(&input_info.iomem, p_mem_ofs, sizeof(NN_IOMEM));
	input_info.net_id = net_id;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_INPUT_INIT, &input_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_INPUT_INIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

HD_RESULT vendor_ais_proc_input_uninit(NN_IOMEM *p_1st_mem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_IOMEM *p_io_mem;
	NN_IOMEM first_mem_ofs;
	NN_DATA *p_model_data, *p_mem_ofs;
	const UINT32 io_mem_cnt = sizeof(NN_IOMEM) / sizeof(NN_DATA);
	UINT32 idx = 0;

	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AIS_FLOW_PROC_INPUT_INFO input_info;
	ER er = E_OK;

	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	p_io_mem = net_info.p_io_mem;
#if CNN_MULTI_INPUT
	proc_layer_num = (vendor_ais_get_net_is_batch(net_id) == 1) ? 1 : p_head->mode_ctrl_num;
#else
	proc_layer_num = p_head->mode_ctrl_num;
#endif
	input_info.net_addr = mem.va;
	input_info.net_id = net_id;



	///< restore first layer io buffer
	memcpy(&first_mem_ofs, p_1st_mem, sizeof(NN_IOMEM));
	p_model_data = (NN_DATA *)(&p_io_mem[0]);
	p_mem_ofs = (NN_DATA *)(&first_mem_ofs);
	for (idx = 0; idx < io_mem_cnt; idx++) {
		if (p_mem_ofs[idx].size == 0) {
			continue;
		}
		p_mem_ofs[idx].pa = -p_mem_ofs[idx].pa;
		p_mem_ofs[idx].va = -p_mem_ofs[idx].va;
		memset(&p_model_data[idx], 0, sizeof(NN_DATA));
	}
	memset(p_1st_mem, 0, sizeof(NN_IOMEM));

	///< restore first layer address of parameter
	vendor_ais_proc_assign_input_addr(proc_layer_num, p_mctrl, &first_mem_ofs);

	///< restore first layer address in kernel space
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_INPUT_UNINIT, &input_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_INPUT_UNINIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

#else
ER vendor_ais_proc_assign_input_addr(UINT32 proc_layer_num, NN_GEN_MODE_CTRL *p_mctrl, NN_DATA *p_imem_ofs)
{
	UINT32 process_index = 0, layer_index = 0;
	NN_DATA *p_sai;
	UINT32 tmp_layer_index = 0;

	if ((proc_layer_num == 0) || (p_mctrl == NULL) || (p_imem_ofs == NULL)) {
		DBG_ERR("null input...\r\n");
		return HD_ERR_ABORT;
	}

	p_sai = p_imem_ofs;
	layer_index = p_mctrl->layer_index;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		tmp_layer_index = p_mctrl[process_index].layer_index;
		if (tmp_layer_index != layer_index) {
			continue;
		}

		if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_APP_AI_DRV) {
			VENDOR_AI_APP_HEAD *p_app_head = (VENDOR_AI_APP_HEAD *)p_mctrl[process_index].addr;
			switch (p_app_head->mode) {
			case AI_MODE_NEURAL: {
					VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
					//p_parm->elt.addr += p_sai[NUE_ELT_IDX].va;
				}
				break;
			case AI_MODE_ROIPOOL: {
					VENDOR_AI_ROIPOOL_PARM *p_parm = (VENDOR_AI_ROIPOOL_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
					p_parm->roi_ker.roi_addr += p_sai[NUE_ROI_IDX].va;
				}
				break;
			case AI_MODE_SVM: {
					VENDOR_AI_SVM_PARM *p_parm = (VENDOR_AI_SVM_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case AI_MODE_FC: {
					VENDOR_AI_FC_PARM *p_parm = (VENDOR_AI_FC_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case AI_MODE_PERMUTE: {
					VENDOR_AI_PERMUTE_PARM *p_parm = (VENDOR_AI_PERMUTE_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case AI_MODE_REORG: {
					VENDOR_AI_REORG_PARM *p_parm = (VENDOR_AI_REORG_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
            case AI_MODE_PREPROC: {
					VENDOR_PREPROC_PARM *p_parm = (VENDOR_PREPROC_PARM*)p_app_head->parm_addr;
					p_parm->in_addr[0] += p_sai[NUE2_IN_IDX0].va;
                    p_parm->in_addr[1] += p_sai[NUE2_IN_IDX1].va;
                    p_parm->in_addr[2] += p_sai[NUE2_IN_IDX2].va;
				}
				break;
			default:
				DBG_ERR("unknown input mode(%d) in app\r\n", (int)p_mctrl[process_index].mode);
				break;
			}
		} else if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
			//========== for first layer linked list mode ==========
			VENDOR_AI_LL_HEAD *p_ll_head = (VENDOR_AI_LL_HEAD *)p_mctrl[process_index].addr;
			switch (p_ll_head->eng) {
			case AI_ENG_CNN:
			case AI_ENG_CNN2: {
					CNN_LL_PARM *p_parm = (CNN_LL_PARM *)p_ll_head->parm_addr;
#if defined(_BSP_NA51090_)
					p_parm->input.bit.addr += (UINT32)(p_sai[BUF_IN_IDX].pa & (0x00000000ffffffff));
					p_parm->input_MSB.bit.addr += (UINT32)((p_sai[BUF_IN_IDX].pa & (0xffffffff00000000))>>32);
#else
					p_parm->input.bit.addr += p_sai[BUF_IN_IDX].pa;
#endif
					//p_parm->input[1].bit.addr += p_sai[BUF_IN_IDX].va;
					//p_parm->input[2].bit.addr += p_sai[BUF_IN_IDX].va;
					//p_parm->interm_in.bit.addr += p_sai[NUE_ELT_IDX].pa;
				}
				break;
			case AI_ENG_NUE: {
					NUE_LL_PARM *p_parm = (NUE_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input.bit.addr += p_sai[BUF_IN_IDX].pa;
					//p_parm->elt_in.bit.addr += p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr += p_sai[NUE_ROI_IDX].va;
				}
				break;
			case AI_ENG_NUE2: {
					NUE2_LL_PARM *p_parm = (NUE2_LL_PARM *)p_ll_head->parm_addr;
#if defined(_BSP_NA51090_)
					p_parm->input[0].bit.addr += (UINT32)(p_sai[NUE2_IN_IDX0].pa & (0x00000000ffffffff));
					p_parm->input_MSB[0].bit.addr += (UINT32)((p_sai[NUE2_IN_IDX0].pa & (0xffffffff00000000))>>32);
					p_parm->input[1].bit.addr += (UINT32)(p_sai[NUE2_IN_IDX1].pa & (0x00000000ffffffff));
					p_parm->input_MSB[1].bit.addr += (UINT32)((p_sai[NUE2_IN_IDX1].pa & (0xffffffff00000000))>>32);
					p_parm->input[2].bit.addr += (UINT32)(p_sai[NUE2_IN_IDX2].pa & (0x00000000ffffffff));
					p_parm->input_MSB[2].bit.addr += (UINT32)((p_sai[NUE2_IN_IDX2].pa & (0xffffffff00000000))>>32);
#else
					p_parm->input[0].bit.addr += p_sai[NUE2_IN_IDX0].pa;
					p_parm->input[1].bit.addr += p_sai[NUE2_IN_IDX1].pa;
					p_parm->input[2].bit.addr += p_sai[NUE2_IN_IDX2].pa;
#endif
					//p_parm->roi_in.bit.addr += p_sai[NUE_ROI_IDX].va;
				}
				break;
			default:
				DBG_ERR("unknown engine type(%d) in ll\r\n", (int)p_ll_head->eng);
				break;
			}
//========== by CCC 191004 ==========
#else
			DBG_ERR("first layer can't be linked list\r\n");
			return HD_ERR_ABORT;
#endif
		} else if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_COMMON) {
			const UINT32 parm_addr = p_mctrl[process_index].addr;
			switch (p_mctrl[process_index].mode) {
			case NN_POSTPROC: {
					NN_POSTPROC_PARM *p_parm = (NN_POSTPROC_PARM *)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case NN_SOFTMAX: {
					NN_SOFTMAX_PARM *p_parm = (NN_SOFTMAX_PARM *)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
                /*
			case NN_PREPROC: {
					NN_PRE_PARM *p_parm = (NN_PRE_PARM*)parm_addr;
					p_parm->in_addr += p_io_mem->SAI[0].va;
				}
				break;
				*/
			case NN_FC_POST: {
					NN_FC_POST_PARM *p_parm = (NN_FC_POST_PARM*)parm_addr;
#if AI_V4
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
#else
					p_parm->addr_in += p_sai[BUF_IN_IDX].va;
#endif
				}
				break;
			case NN_POOL: {
					NN_POOL_PARM *p_parm = (NN_POOL_PARM*)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case NN_LAYER_NORMALIZATION: {
					NN_LAYER_NORMALIZATION_PARM *p_parm = (NN_LAYER_NORMALIZATION_PARM*)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case NN_CUSTOMER: {
					NN_CUSTOM_PARM *p_parm = (NN_CUSTOM_PARM*)parm_addr;
#if CUST_SUPPORT_MULTI_IO
					NN_DATA* io_head = NULL;
					UINT32 i = 0;

					// parsing input
					io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM));
					for (i = 0; i < p_parm->input_num; i++) {
						io_head[i].va += p_sai[i].va;
						io_head[i].pa += p_sai[i].pa;
					}
#else
					p_parm->input.va += p_sai[BUF_IN_IDX].va;
					p_parm->input.pa += p_sai[BUF_IN_IDX].pa;
#endif
				}
				break;
			case NN_BNSCALE: {
					NN_BNSCALE_PARM *p_parm = (NN_BNSCALE_PARM*)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			default:
				DBG_ERR("unknown input mode(%d) in common\r\n", (int)p_mctrl[process_index].mode);
				break;
			}
		} else {
			DBG_ERR("unknown first layer trigger source: %d\r\n", (int)p_mctrl[process_index].trig_src);
			return HD_ERR_ABORT;
		}
	}

	return E_OK;
}

HD_RESULT vendor_ais_proc_input_init(NN_MODE mode, NN_DATA *p_1st_imem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num, imem_cnt = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_imem_ofs, *p_model_data;
	VENDOR_AIS_FLOW_PROC_INPUT_INFO input_info = {0};
	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT ret = HD_OK;

	ret = vendor_ais_get_net_info(&net_info, mem.va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	if (p_mctrl[0].mode != mode) {
		DBG_ERR("input mode(%d) isn't matched with first layer model mode(%d)\r\n", (int)mode, (int)p_mctrl[0].mode);
		return HD_ERR_ABORT;
	}

	///< update first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[0].iomem.imem_addr;
	p_imem_ofs = p_1st_imem;
	imem_cnt = p_mctrl[0].iomem.imem_cnt;
	if (imem_cnt > NN_IMEM_NUM) {
		DBG_ERR("first layer imem count (%d) should be < %d\r\n", (int)imem_cnt, NN_IMEM_NUM);
		return HD_ERR_ABORT;
	}
	for (idx = 0; idx < imem_cnt; idx++) {
		if (p_imem_ofs[idx].size == 0) {
			continue;
		}
		memcpy(&p_model_data[idx], &p_imem_ofs[idx], sizeof(NN_DATA));
	}

	///< update first layer address of parameter
	vendor_ais_proc_assign_input_addr(proc_layer_num, p_mctrl, p_imem_ofs);

	///< update first layer address in kernel space
	input_info.net_addr = mem.va;
	input_info.net_id = net_id;
	memcpy(&input_info.imem, p_imem_ofs, imem_cnt * sizeof(NN_DATA));

	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_INPUT_INIT, &input_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_INPUT_INIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

HD_RESULT vendor_ais_proc_input_uninit(NN_DATA *p_1st_imem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num, imem_cnt = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_imem_ofs, *p_model_data;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AIS_FLOW_PROC_INPUT_INFO input_info = {0};
	HD_RESULT ret = HD_OK;
	NN_DATA first_mem_ofs[NN_IMEM_NUM];

	ret = vendor_ais_get_net_info(&net_info, mem.va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	memcpy(first_mem_ofs, p_1st_imem, sizeof(first_mem_ofs));

	///< restore first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[0].iomem.imem_addr;
	p_imem_ofs = first_mem_ofs;
	imem_cnt = p_mctrl[0].iomem.imem_cnt;
	if (imem_cnt > NN_IMEM_NUM) {
		DBG_ERR("first layer imem count (%d) should be < %d\r\n", (int)imem_cnt, NN_IMEM_NUM);
		return HD_ERR_ABORT;
	}

	for (idx = 0; idx < imem_cnt; idx++) {
		if (p_imem_ofs[idx].size == 0) {
			continue;
		}
		memset(&p_model_data[idx], 0, sizeof(NN_DATA));
		p_imem_ofs[idx].pa = -1 * ((INT32)p_imem_ofs[idx].pa);
		p_imem_ofs[idx].va = -1 * ((INT32)p_imem_ofs[idx].va);
	}

	///< restore first layer address of parameter
	vendor_ais_proc_assign_input_addr(proc_layer_num, p_mctrl, p_imem_ofs);

	///< restore first layer address in kernel space
	input_info.net_addr = mem.va;
	input_info.net_id = net_id;
	memcpy(&input_info.imem, p_imem_ofs, imem_cnt * sizeof(NN_DATA));

	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_INPUT_UNINIT, &input_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_INPUT_UNINIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

#if CNN_MULTI_INPUT
HD_RESULT vendor_ais_proc_input_init2(NN_MODE mode, NN_DATA *p_1st_imem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 proc_idx, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num, imem_cnt = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_imem_ofs, *p_model_data;
	VENDOR_AIS_FLOW_PROC_INPUT_INFO input_info = {0};
	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT ret = HD_OK;

	ret = vendor_ais_get_net_info(&net_info, mem.va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	//proc_layer_num = p_head->mode_ctrl_num;

	if (p_mctrl[proc_idx].mode != mode) {
		DBG_ERR("input mode(%d) isn't matched with first layer model mode(%d)\r\n", (int)mode, (int)p_mctrl[proc_idx].mode);
		return HD_ERR_ABORT;
	}

	///< update first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[proc_idx].iomem.imem_addr;
	p_imem_ofs = p_1st_imem;
	imem_cnt = p_mctrl[proc_idx].iomem.imem_cnt;
	if (imem_cnt > NN_IMEM_NUM) {
		DBG_ERR("first layer imem count (%d) should be < %d\r\n", (int)imem_cnt, NN_IMEM_NUM);
		return HD_ERR_ABORT;
	}
	for (idx = 0; idx < imem_cnt; idx++) {
		if (p_imem_ofs[idx].size == 0) {
			continue;
		}
		// update addr & size
		p_model_data[idx].pa = p_imem_ofs[idx].pa;
		p_model_data[idx].va = p_imem_ofs[idx].va;
		p_model_data[idx].size = p_imem_ofs[idx].size;
	}
	if (vendor_ais_get_net_is_batch(net_id) == 1) { // (is_batch == 1), assign input addr to user-specified mctrl layer
		proc_layer_num = 1;
	} else {                                        // (is_batch != 1), assign input addr that needs to be set through the for_loop
		if (p_head->mode_ctrl_num < proc_idx) {
			DBG_ERR("invalid input!! mode_ctrl_num(%lu) < proc_idx(%lu)\r\n", p_head->mode_ctrl_num, proc_idx);
			return HD_ERR_NG;
		}
		proc_layer_num = p_head->mode_ctrl_num - proc_idx;
	}

	///< update first layer address of parameter
	vendor_ais_proc_assign_input_addr(proc_layer_num, &p_mctrl[proc_idx], p_imem_ofs);

	///< update first layer address in kernel space
	input_info.net_addr = mem.va;
	input_info.net_id = net_id;
	input_info.proc_idx = proc_idx;
	memcpy(&input_info.imem, p_imem_ofs, imem_cnt * sizeof(NN_DATA));

	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_INPUT_INIT, &input_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_INPUT_INIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

HD_RESULT vendor_ais_proc_input_uninit2(NN_DATA *p_1st_imem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 proc_idx, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num, imem_cnt = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_imem_ofs, *p_model_data;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AIS_FLOW_PROC_INPUT_INFO input_info = {0};
	HD_RESULT ret = HD_OK;
	NN_DATA first_mem_ofs[NN_IMEM_NUM];

	ret = vendor_ais_get_net_info(&net_info, mem.va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	if (vendor_ais_get_net_is_batch(net_id) == 1) { // (is_batch == 1), assign input addr to user-specified mctrl layer
		proc_layer_num = 1;
	} else {                                        // (is_batch != 1), assign input addr that needs to be set through the for_loop
		if (p_head->mode_ctrl_num < proc_idx) {
			DBG_ERR("invalid input!! mode_ctrl_num(%lu) < proc_idx(%lu)\r\n", p_head->mode_ctrl_num, proc_idx);
			return HD_ERR_NG;
		}
		proc_layer_num = p_head->mode_ctrl_num - proc_idx;
	}

	memcpy(first_mem_ofs, p_1st_imem, sizeof(first_mem_ofs));

	///< restore first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[proc_idx].iomem.imem_addr;
	p_imem_ofs = first_mem_ofs;
	imem_cnt = p_mctrl[proc_idx].iomem.imem_cnt;
	if (imem_cnt > NN_IMEM_NUM) {
		DBG_ERR("first layer imem count (%d) should be < %d\r\n", (int)imem_cnt, NN_IMEM_NUM);
		return HD_ERR_ABORT;
	}

	for (idx = 0; idx < imem_cnt; idx++) {
		if (p_imem_ofs[idx].size == 0) {
			continue;
		}
		// clear addr & size
		p_model_data[idx].pa = 0;
		p_model_data[idx].va = 0;
		p_model_data[idx].size = 0;

		// turn to negative (for reduction assignment)
		p_imem_ofs[idx].pa = -1 * ((INT32)p_imem_ofs[idx].pa);
		p_imem_ofs[idx].va = -1 * ((INT32)p_imem_ofs[idx].va);
	}

	///< restore first layer address of parameter
	vendor_ais_proc_assign_input_addr(proc_layer_num, &p_mctrl[proc_idx], p_imem_ofs);

	///< restore first layer address in kernel space
	input_info.net_addr = mem.va;
	input_info.net_id = net_id;
	//proc_idx kernal space starts
	input_info.proc_idx = proc_idx;
	memcpy(&input_info.imem, p_imem_ofs, imem_cnt * sizeof(NN_DATA));

	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_INPUT_UNINIT, &input_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_INPUT_UNINIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}
#endif // end of "#if CNN_MULTI_INPUT"
#endif

NN_GEN_MODE_CTRL *vendor_ais_get_proclayer(UINT32 layer, VENDOR_AIS_FLOW_MEM_PARM mem)
{

	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return NULL;
	}
	p_mctrl = net_info.p_mctrl;

	return &p_mctrl[layer];
}

HD_RESULT vendor_ais_update_proclayer(UINT32 layer, UINT32 net_id)
{
	VENDOR_AIS_FLOW_UPDATE_INFO up_info;
	up_info.layer = layer;
	up_info.net_id = net_id;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_UPDATE_PARM, &up_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_UPDATE_PARM ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

HD_RESULT vendor_ais_set_ll_base(UINT32 net_id, UINT32 eng, UINT32 addr)
{
	VENDOR_AIS_FLOW_LL_BASE ll_base;

	/* convert type: vendor to kdrv */
	switch (eng) {
	case VENDOR_AI_ENGINE_CNN:
		ll_base.eng = AI_ENG_CNN;
		break;
	case VENDOR_AI_ENGINE_NUE:
		ll_base.eng = AI_ENG_NUE;
		break;
	case VENDOR_AI_ENGINE_NUE2:
		ll_base.eng = AI_ENG_NUE2;
		break;
	default:
		DBG_ERR("invalid eng(%ld)\r\n", eng);
		return E_SYS;
	}
	ll_base.addr = addr;
	ll_base.net_id = net_id;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_LL_BASE, &ll_base) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_LL_BASE ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

HD_RESULT vendor_ais_set_mem_ofs(UINT32 net_id, UINT32 ofs)
{
	VENDOR_AIS_FLOW_MEM_OFS mem_ofs;

	mem_ofs.ofs = ofs;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_MEM_OFS, &mem_ofs) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_MEM_OFS ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

HD_RESULT vendor_ais_chk_vers(UINT32 chip_id, UINT32 gentool_vers, UINT32 net_id)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AIS_FLOW_VERS vers_info = {0};
	INT dev_fd = 0;

	// open device
	CHAR device[256] = {0};
	snprintf(device, 256, DEFAULT_DEVICE"%i", MAX_PROC_CNT + 1);
	dev_fd = KFLOW_AI_OPEN(device, O_RDWR);
	if (dev_fd == -1) {
		fprintf(stderr, "open %s failed\r\n", device);
		vendor_ai_errno_location();
		return HD_ERR_NG;
	}

	// set variables
	vers_info.proc_id = net_id;
	vers_info.chip_id = chip_id;
	vers_info.gentool_vers = gentool_vers;

	// ioctl to check version
	if (KFLOW_AI_IOCTL(dev_fd, VENDOR_AIS_FLOW_IOC_VERS, &vers_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_VERS ioctl failed\r\n");
		vendor_ai_errno_location();
		ret = HD_ERR_ABORT;
		goto exit;
	}

	ret = (vers_info.rv == HD_OK)? HD_OK:HD_ERR_NOT_SUPPORT;

exit:
	// close device
	if (dev_fd != -1) {
		if (KFLOW_AI_CLOSE(dev_fd) == -1) {
			fprintf(stderr, "close %s failed\r\n", DEFAULT_DEVICE);
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}
	return ret;
}

INT32 vendor_ai_get_kflow_version(CHAR* kflow_version)
{
	INT32 fd = -1;
	INT32 ret = 0;

	CHAR device[256] = {0};
	snprintf(device, 256, DEFAULT_DEVICE"%i", MAX_PROC_CNT + 1);
	fd = KFLOW_AI_OPEN(device, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "open %s failed\r\n", device);
		vendor_ai_errno_location();
		return HD_ERR_NG;
	}

	ret = KFLOW_AI_IOCTL(fd, VENDOR_AIS_FLOW_IOC_GET_VER, kflow_version);
	if (ret < 0) {
		DBG_ERR("vendor_ai_get_kflow_version: get kflow version ioctl fail!\n");
	}

	if (fd != -1) {
		if (KFLOW_AI_CLOSE(fd) == -1) {
			fprintf(stderr, "close %s failed\r\n", DEFAULT_DEVICE);
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}

	return ret;
}

INT32 vendor_ai_get_kdrv_version(CHAR* kdrv_version)
{
	INT32 fd = -1;
	INT32 ret = 0;

	fd = KFLOW_AI_OPEN("/dev/nvt_ai_module0", O_RDWR);
	if (fd < 0) {
		DBG_ERR("[VENDOR_AI] Open kdrv_ai fail!\n");
		return 0;
	}

	ret = KFLOW_AI_IOCTL(fd, AI_IOC_GET_VER, kdrv_version);
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai open fail! \n");
	}

	if (fd != -1) {
		if (KFLOW_AI_CLOSE(fd) == -1) {
			fprintf(stderr, "close /dev/nvt_ai_module0 failed\r\n");
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}


	return ret;
}

HD_RESULT vendor_ais_dma_abort(VOID)
{
	HD_RESULT ret = HD_OK;
	// ioctl to dma abort
	if (vendor_ais_flow_fd[0] != -1) {
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], KFLOW_AI_IOC_CMD_DMA_ABORT, NULL) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_VERS ioctl failed\r\n");
			vendor_ai_errno_location();
			ret = HD_ERR_ABORT;
		}
	} else {
		DBG_ERR("dma abort failed, fd = (-1)\r\n");
	}
	return ret;
}

#if 0 //LL_SUPPORT_ROI
UINT32 g_ll_info_addr[NN_SUPPORT_NET_MAX] = {0};
HD_RESULT vendor_ais_set_ll_info(AI_DRV_LL_USR_INFO *ll_usr_info, UINT32 net_id)
{
	if (ll_usr_info == NULL) {
		DBG_ERR("vendor_ais_set_ll_info input is NULL...\r\n");
		return HD_ERR_BAD_DATA;
	}

	g_ll_info_addr[net_id] = (UINT32)ll_usr_info;

	return HD_OK;
}
#endif

#if (NEW_AI_FLOW == 1)
/*
HD_RESULT vendor_ais_net_builtin_JOB_done(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem)
{
	return HD_OK;
}
*/
#endif

#if (NEW_AI_FLOW == 1)

#if defined(_BSP_NA51090_)
#else
static void vendor_ais_flush_mem(void* virt_addr, unsigned int size, VENDOR_COMMON_MEM_DMA_DIR dir, int cpu_id)
{
	if (cpu_id == 0xff) {
		//DBG_DUMP("vendor_ais_flush_mem : no-bind!\r\n");
		vendor_common_mem_cache_sync(virt_addr, size, dir);
	} else if (cpu_id < 2) {
		//DBG_DUMP("vendor_ais_flush_mem : bind to %d!\r\n", cpu_id);
		vendor_common_mem_cache_sync_all(virt_addr, size, VENDOR_COMMON_MEM_DMA_BIDIRECTIONAL);
	} else {
		DBG_ERR("vendor_ais_flush_mem : invalid cpu_id %d?\r\n", cpu_id);
	}
}
#endif

#if NN_DLI
HD_RESULT vendor_ais_net_builtin_CPU_init(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem)
{
	HD_RESULT r;
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};

	//op context
	NN_GEN_MODE_CTRL *p_layer;

	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	p_layer = net_info.p_mctrl + job_id; //process id

	// eng = CPU, ch = 0
	if (g_ext_cb_fp[0]) {
		r = (g_ext_cb_fp[0])(proc_id, job_id, p_layer->mode | VENDOR_AI_CTRL_LYR_START, (UINT32)p_layer, p_layer->addr);
	} else {
		r = HD_ERR_NOT_SUPPORT;
	}
	return r; // this net has sw layer, but user doesn't register callback !!
}

HD_RESULT vendor_ais_net_builtin_CPU_exit(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem)
{
	HD_RESULT r;
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};

	//op context
	NN_GEN_MODE_CTRL *p_layer;

	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	p_layer = net_info.p_mctrl + job_id; //process id

	// eng = CPU, ch = 0
	if (g_ext_cb_fp[0]) {
		r = (g_ext_cb_fp[0])(proc_id, job_id, p_layer->mode | VENDOR_AI_CTRL_LYR_STOP, (UINT32)p_layer, p_layer->addr);
	} else {
		r = HD_ERR_NOT_SUPPORT;
	}
	return r; // this net has sw layer, but user doesn't register callback !!
}
#endif


HD_RESULT vendor_ais_net_builtin_CPU_exec(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem)
{
	HD_RESULT r;
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};

	//net
	//NN_GEN_MODEL_HEAD *p_head;
	//UINT32 proc_layer_num;

	//op context
	NN_GEN_MODE_CTRL *p_layer;

	//io context
#if !CNN_25_MATLAB
	NN_DATA *p_sai, *p_sao;
	UINT32 imem_cnt, omem_cnt;
#else
	NN_IOMEM *p_buffer;
#endif
	UINT32 ibuf_id = 0;
	UINT32 obuf_id = 0;
#if defined(_BSP_NA51090_)
#else
	int cpu_id;
#endif

	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

#if defined(_BSP_NA51090_)
#else
	cpu_id = vendor_ai_cpu_thread_getbind(proc_id);
#endif

	//proc_layer_num = p_head->mode_ctrl_num;
	//p_head = net_info.p_head;
	p_layer = net_info.p_mctrl + job_id; //process id
#if CNN_25_MATLAB
	p_buffer = net_info.p_io_mem + p_layer->layer_index; //layer id

	for (ibuf_id = 0; ibuf_id < NN_IMEM_NUM; ibuf_id++) {
		if ((p_buffer->SAI[ibuf_id].va == 0) || (p_buffer->SAI[ibuf_id].size == 0)) {
			continue;
		}
#if defined(_BSP_NA51090_)
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size);
		vendor_common_mem_cache_sync((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size, VENDOR_COMMON_MEM_DMA_BIDIRECTIONAL); ///< cache invalidate - input from engine's output
#else
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size);
		vendor_ais_flush_mem((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE, cpu_id); ///< cache invalidate - input from engine's output
#endif
	}
#else
	imem_cnt = p_layer->iomem.imem_cnt;
	p_sai = (NN_DATA*)p_layer->iomem.imem_addr;
	
	for (ibuf_id = 0; ibuf_id < imem_cnt; ibuf_id++) {
		if ((p_sai[ibuf_id].va == 0) || (p_sai[ibuf_id].size == 0)) {
			continue;
		}
/*  // TODO : could TMP buffer doesn't need flush??
#if CNN_CGEN_NEW_TMP_BUF
		// skip flush tmp buffer
		if ((p_layer->mode == NN_SOFTMAX) || (p_layer->mode == NN_PRIORBOX) || (p_layer->mode == NN_DETOUT) ) {
			if (ibuf_id == 2) {
				continue; // for those NN_MODE, sai[2] is tmp buffer, don't have to flush cache ( but if this address reuse previous some HW layer?? )
			}
		}
#endif
*/
#if defined(_BSP_NA51090_)
		vendor_pcie_mem_cache_sync((VOID *)p_sai[ibuf_id].va, p_sai[ibuf_id].size, VENDOR_PCIE_MEM_DMA_BIDIRECTIONAL); ///< cache invalidate - input from engine's output
//		vendor_pcie_mem_flush_cache((VOID *)p_sai[ibuf_id].va, p_sai[ibuf_id].size);
#else
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size);
		vendor_ais_flush_mem((VOID *)p_sai[ibuf_id].va, p_sai[ibuf_id].size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE, cpu_id); ///< cache invalidate - input from engine's output
#endif
	}
#endif

	// eng = CPU, ch = 0
	if (g_ext_cb_fp[0]) {
		r = (g_ext_cb_fp[0])(proc_id, job_id, p_layer->mode, (UINT32)p_layer, p_layer->addr);
	} else {
		r = HD_ERR_NOT_SUPPORT;
	}
#if CNN_25_MATLAB
	for (obuf_id = 0; obuf_id < NN_OMEM_NUM; obuf_id++) {
		if ((p_buffer->SAO[obuf_id].va == 0) || (p_buffer->SAO[obuf_id].size == 0)) {
			continue;
		}
#if defined(_BSP_NA51090_)
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAO[obuf_id].va, p_buffer->SAO[obuf_id].size);
		vendor_common_mem_cache_sync((VOID *)p_buffer->SAO[obuf_id].va, p_buffer->SAO[obuf_id].size, VENDOR_COMMON_MEM_DMA_TO_DEVICE); ///< cache clean - output to engine's input
#else
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAO[obuf_id].va, p_buffer->SAO[obuf_id].size);
		vendor_ais_flush_mem((VOID *)p_buffer->SAO[obuf_id].va, p_buffer->SAO[obuf_id].size, VENDOR_COMMON_MEM_DMA_TO_DEVICE, cpu_id); ///< cache clean - output to engine's input
#endif
	}
#else
	//if (p_layer->mode == NN_CUSTOMER) {		// NN_CUSTOMER layer will "write input buffer", so we have to cache flush input buffer after execute !!
	if (1) { // always flush (current proc is ok...but next-proc will use pre-proc tmp buffer range for HW)
		imem_cnt = p_layer->iomem.imem_cnt;
		p_sai = (NN_DATA*)p_layer->iomem.imem_addr;

		for (ibuf_id = 0; ibuf_id < imem_cnt; ibuf_id++) {
			if ((p_sai[ibuf_id].va == 0) || (p_sai[ibuf_id].size == 0)) {
			       continue;
			}
#if defined(_BSP_NA51090_)
			vendor_pcie_mem_cache_sync((VOID *)p_sai[ibuf_id].va, p_sai[ibuf_id].size, VENDOR_PCIE_MEM_DMA_BIDIRECTIONAL); ///< cache invalidate - input from engine's output
//			vendor_pcie_mem_flush_cache((VOID *)p_sai[ibuf_id].va, p_sai[ibuf_id].size);
#else
			//hd_common_mem_flush_cache((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size);
			vendor_ais_flush_mem((VOID *)p_sai[ibuf_id].va, p_sai[ibuf_id].size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE, cpu_id); ///< cache invalidate - input from engine's output
#endif
		}
	}

	omem_cnt = p_layer->iomem.omem_cnt;
	p_sao = (NN_DATA*)p_layer->iomem.omem_addr;

	for (obuf_id = 0; obuf_id < omem_cnt; obuf_id++) {
		if ((p_sao[obuf_id].va == 0) || (p_sao[obuf_id].size == 0)) {
			continue;
		}
#if defined(_BSP_NA51090_)
		vendor_pcie_mem_cache_sync((VOID *)p_sao[obuf_id].va, p_sao[obuf_id].size, VENDOR_PCIE_MEM_DMA_BIDIRECTIONAL); ///< cache clean - output to engine's input
//		vendor_pcie_mem_flush_cache((VOID *)p_sao[obuf_id].va, p_sao[obuf_id].size);
#else
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAO[obuf_id].va, p_buffer->SAO[obuf_id].size);
		vendor_ais_flush_mem((VOID *)p_sao[obuf_id].va, p_sao[obuf_id].size, VENDOR_COMMON_MEM_DMA_TO_DEVICE, cpu_id); ///< cache clean - output to engine's input
#endif
	}
#endif
	return r; // this net has sw layer, but user doesn't register callback !!
}

#endif


#if (NEW_AI_FLOW == 1)

HD_RESULT vendor_ais_net_builtin_DSP_exec(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem)
{
	HD_RESULT r;
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};

	//net
	//NN_GEN_MODEL_HEAD *p_head;
	//UINT32 proc_layer_num;

	//op context
	NN_GEN_MODE_CTRL *p_layer;

	//io context
#if !CNN_25_MATLAB
	NN_DATA *p_sai, *p_sao;
	UINT32 imem_cnt, omem_cnt;
#else
	NN_IOMEM *p_buffer;
#endif
	UINT32 ibuf_id = 0;
	UINT32 obuf_id = 0;

	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	//proc_layer_num = p_head->mode_ctrl_num;
	//p_head = net_info.p_head;
	p_layer = net_info.p_mctrl + job_id; //process id
#if CNN_25_MATLAB
	p_buffer = net_info.p_io_mem + p_layer->layer_index; //layer id

	for (ibuf_id = 0; ibuf_id < NN_IMEM_NUM; ibuf_id++) {
		if ((p_buffer->SAI[ibuf_id].va == 0) || (p_buffer->SAI[ibuf_id].size == 0)) {
			continue;
		}
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size);
		vendor_common_mem_cache_sync((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE); ///< cache invalidate - input from engine's output
	}
#else
	imem_cnt = p_layer->iomem.imem_cnt;
	p_sai = (NN_DATA*)p_layer->iomem.imem_addr;

	for (ibuf_id = 0; ibuf_id < imem_cnt; ibuf_id++) {
		if ((p_sai[ibuf_id].va == 0) || (p_sai[ibuf_id].size == 0)) {
			continue;
		}
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAI[ibuf_id].va, p_buffer->SAI[ibuf_id].size);
		vendor_common_mem_cache_sync((VOID *)p_sai[ibuf_id].va, p_sai[ibuf_id].size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE); ///< cache invalidate - input from engine's output
	}
#endif

	// eng = DSP, ch = 0
	if (g_ext_cb_fp[1]) {
		r = (g_ext_cb_fp[1])(proc_id, job_id, p_layer->mode, (UINT32)p_layer, p_layer->addr);
	} else {
		r = HD_ERR_NOT_SUPPORT;
	}
#if CNN_25_MATLAB
	for (obuf_id = 0; obuf_id < NN_OMEM_NUM; obuf_id++) {
		if ((p_buffer->SAO[obuf_id].va == 0) || (p_buffer->SAO[obuf_id].size == 0)) {
			continue;
		}
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAO[obuf_id].va, p_buffer->SAO[obuf_id].size);
		vendor_common_mem_cache_sync((VOID *)p_buffer->SAO[obuf_id].va, p_buffer->SAO[obuf_id].size, VENDOR_COMMON_MEM_DMA_TO_DEVICE); ///< cache clean - output to engine's input
	}
#else
	omem_cnt = p_layer->iomem.omem_cnt;
	p_sao = (NN_DATA*)p_layer->iomem.omem_addr;

	for (obuf_id = 0; obuf_id < omem_cnt; obuf_id++) {
		if ((p_sao[obuf_id].va == 0) || (p_sao[obuf_id].size == 0)) {
			continue;
		}
		//hd_common_mem_flush_cache((VOID *)p_buffer->SAO[obuf_id].va, p_buffer->SAO[obuf_id].size);
		vendor_common_mem_cache_sync((VOID *)p_sao[obuf_id].va, p_sao[obuf_id].size, VENDOR_COMMON_MEM_DMA_TO_DEVICE); ///< cache clean - output to engine's input
	}
#endif
	return r; // this net has sw layer, but user doesn't register callback !!
}


/*

#if NN_USE_DSP
		if (ctrl_type == NN_GEN_ENG_DSP) {
			for (idx = 0; idx < NN_IMEM_NUM; idx++) {
				if ((p_io_mem[layer_index].SAI[idx].va == 0) || (p_io_mem[layer_index].SAI[idx].size == 0)) {
					continue;
				}
				hd_common_mem_flush_cache((VOID *)p_io_mem[layer_index].SAI[idx].va, p_io_mem[layer_index].SAI[idx].size);
			}
			for (idx = 0; idx < NN_OMEM_NUM; idx++) {
				if ((p_io_mem[layer_index].SAO[idx].va == 0) || (p_io_mem[layer_index].SAO[idx].size == 0)) {
					continue;
				}
				hd_common_mem_flush_cache((VOID *)p_io_mem[layer_index].SAO[idx].va, p_io_mem[layer_index].SAO[idx].size);
			}
		}
#endif

*/


/*
static BOOL g_enable_dump = 0;
*/
extern HD_RESULT _vendor_ai_net_debug_dump(UINT32 proc_id, UINT32 space, UINT32 item, CHAR *filepath);

//////////////////////////////////////////////////////////////////////////////////

#if defined(__LINUX)
#include <pthread.h>
#endif
#if defined(__FREERTOS)
#include <FreeRTOS.h>
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#endif

//static pthread_mutex_t     lock[16];
//static pthread_cond_t		cond[16];
HD_RESULT vendor_ai_init_job_graph(UINT32 net_id)
{
	HD_RESULT r;
	UINT32 job_id = 0xf0000001;
	r = vendor_ai_dla_push_job(net_id, 0xf0000001, 0); // push begin
	if (r != HD_OK) return r;
	r = vendor_ai_dla_pull_job(net_id, &job_id); // pull begin
/*
	pthread_mutex_init(&lock[net_id], NULL);
	pthread_cond_init(&cond[net_id], NULL);
*/
	return r;
}

HD_RESULT vendor_ai_uninit_job_graph(UINT32 net_id)
{
	HD_RESULT r;
	UINT32 job_id = 0xf0000002;
	r = vendor_ai_dla_push_job(net_id, 0xf0000002, 0); // push end
	if (r != HD_OK) return r;
	r = vendor_ai_dla_pull_job(net_id, &job_id); // pull                                                                                                            end
/*
	pthread_mutex_destroy(&lock[net_id]);
	pthread_cond_destroy(&cond[net_id]);
*/
	return r;
}

HD_RESULT vendor_ai_sig_job_graph(UINT32 net_id)
{
	HD_RESULT r;
	UINT32 job_id = 0xf0000003;
	r = vendor_ai_dla_pull_job(net_id, &job_id); // pull sig
/*
	pthread_mutex_lock(&lock[net_id]);
	pthread_cond_broadcast(&cond[net_id]);
	pthread_mutex_unlock(&lock[net_id]);
*/
	return r;
}

HD_RESULT vendor_ai_wait_job_graph(UINT32 net_id, UINT32* job_id)
{
	HD_RESULT r;
	UINT32 pulljob_id = 0xf0000004;
	//vendor_ai_net_trace(net_id, AI_FLOW, "wait_job() - begin\r\n");
	r = vendor_ai_dla_pull_job(net_id, &pulljob_id); // pull wait
	//vendor_ai_net_trace(net_id, AI_FLOW, "wait_job() - end\r\n");
/*
	pthread_mutex_lock(&lock[net_id]);
	pthread_cond_wait(&cond[net_id], &lock[net_id]);
	pthread_mutex_unlock(&lock[net_id]);
*/
	return r;
}


//////////////////////////////////////////////////////////////////////////////////
/*
typedef struct _VENDOR_AI_CB_TASK {
	UINT32 proc_id;
	int en;
	pthread_t  thread_id;
} VENDOR_AI_CB_TASK;


//VENDOR_AI_CB_TASK _vendor_ai_job_task[16];

static void *_vendor_ai_job_callback_thread(void *arg);
static VENDOR_AIS_JOB_CB _vendor_ai_job_callback = 0;

HD_RESULT vendor_ais_net_reg_JOB(VENDOR_AIS_JOB_CB fp)
{
	_vendor_ai_job_callback = fp;
	return HD_OK;
}

static HD_RESULT _vendor_ai_job_callback_thread_init(UINT32 proc_id)
{
	int ret;
	VENDOR_AI_CB_TASK* p_cb_task = _vendor_ai_job_task + proc_id;
	p_cb_task->proc_id = proc_id;
	p_cb_task->en = 1;
	p_cb_task->thread_id = 0;
	ret = pthread_create(&p_cb_task->thread_id, NULL, _vendor_ai_job_callback_thread, (void *)p_cb_task);
	if (ret < 0) {
		//DBG_ERR("create dump thread fail? %d\r\n", r);
		return HD_ERR_FAIL;
	}
	return HD_OK;
}

static HD_RESULT _vendor_ai_job_callback_thread_exit(UINT32 proc_id)
{
	VENDOR_AI_CB_TASK* p_cb_task = _vendor_ai_job_task + proc_id;
	if (p_cb_task->thread_id == 0) {
		//DBG_ERR("create dump thread fail? %d\r\n", r);
		return HD_ERR_FAIL;
	}
	// destroy thread
	pthread_join(p_cb_task->thread_id, NULL);
	return HD_OK;
}

static void *_vendor_ai_job_callback_thread(void *arg)
{
	int r;
	VENDOR_AI_CB_TASK* p_cb_task = (VENDOR_AI_CB_TASK*)arg;
	VENDOR_AIS_FLOW_JOB_WAI wai_cmd = {0};
	VENDOR_AIS_FLOW_JOB_SIG sig_cmd = {0};
	UINT32 proc_id = p_cb_task->proc_id;

	while(p_cb_task->en) {
		// wait log cmd
		wai_cmd.proc_id = proc_id;
		r = KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_WAI_JOB, &wai_cmd);
		vendor_ai_net_trace(proc_id, AI_FLOW, "job_callback() - get event\r\n");
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
			//printf("user: job[%d] done\r\n", (int)wai_cmd.job_id);
			vendor_ai_net_trace(proc_id, AI_FLOW, "job_callback() - CB start\r\n");
			vendor_ai_sig_job_graph(proc_id);
			if (_vendor_ai_job_callback)
				_vendor_ai_job_callback(proc_id, wai_cmd.job_id);
			vendor_ai_net_trace(proc_id, AI_FLOW, "job_callback() - CB end\r\n");
		}
		if (p_cb_task->en) {
			vendor_ai_net_trace(proc_id, AI_FLOW, "job_callback() - wait event\r\n");
			sig_cmd.proc_id = proc_id;
			sig_cmd.job_id = wai_cmd.job_id;
			// signal log cmd
			r = KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_SIG_JOB, &sig_cmd);
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
*/

//////////////////////////////////////////////////////////////////////////////////

static void _vendor_ai_job_select_eng(UINT32 job_id, NN_GEN_MODE_CTRL *p_this_mctrl, UINT32* p_eng_id, UINT32* p_eng_op)
{
	UINT32 engine_id;
	UINT32 engine_op;

	if ((p_this_mctrl->eng == NN_GEN_ENG_CNN) || (p_this_mctrl->eng == NN_GEN_ENG_CNN2)) {
		engine_id = VENDOR_AI_ENGINE_CNN; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
	} else if (p_this_mctrl->eng == NN_GEN_ENG_NUE) {
#if (CNN_NUE_EXCLUSIVE == 1)
		//running this job by CNN engine, and mark it with 0x00008000 for NUE mode
		engine_id = VENDOR_AI_ENGINE_CNN; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode) | 0x00008000);
#else
		engine_id = VENDOR_AI_ENGINE_NUE; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
#endif
	} else if (p_this_mctrl->eng == NN_GEN_ENG_NUE2) {
		engine_id = VENDOR_AI_ENGINE_NUE2; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
	} else if (p_this_mctrl->eng == NN_GEN_ENG_CPU) {
		engine_id = VENDOR_AI_ENGINE_CPU; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
	} else if (p_this_mctrl->eng == NN_GEN_ENG_DSP) {
		engine_id = VENDOR_AI_ENGINE_DSP; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
	} else {
		engine_id = VENDOR_AI_ENGINE_UNKNOWN; engine_op = 0;
	}

	if (p_eng_id) p_eng_id[0] = engine_id;
	if (p_eng_op) p_eng_op[0] = engine_op;
}

INT _cvt_order_name(VENDOR_AI_NET_JOB_OPT order, CHAR *p_ret_string, INT max_str_len)
{
	switch (order) {
		case VENDOR_AI_NET_JOB_OPT_LINEAR:      snprintf(p_ret_string, max_str_len, "linear");    break;
		case VENDOR_AI_NET_JOB_OPT_LINEAR_O1:   snprintf(p_ret_string, max_str_len, "linear-o1"); break;
		case VENDOR_AI_NET_JOB_OPT_GRAPH:       snprintf(p_ret_string, max_str_len, "graph");     break;
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O1:    snprintf(p_ret_string, max_str_len, "graph-o1");  break;
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O2:    snprintf(p_ret_string, max_str_len, "graph-o2");  break;
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O3:    snprintf(p_ret_string, max_str_len, "graph-o3");  break;
		default:
			snprintf(p_ret_string, max_str_len, "(n/a)");   break;
			return (-1);
	}
	return 0;
}

HD_RESULT vendor_ais_proc_net_dump_group(UINT32 net_id)
{
	HD_RESULT ret = HD_OK;
	CHAR full_path[CMD_MAX_PATH_LEN] = {0};
	UINT32 dump_bmp = 0;

	dump_bmp = vendor_ai_cmd_get_group_dump(net_id);
	if (dump_bmp & VENDOR_AI_NET_CMD_DOT_GROUP) {
		snprintf(full_path, CMD_MAX_PATH_LEN, "/mnt/sd//dot_group.txt");
		ret = _vendor_ai_net_debug_dump(net_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_DOT_GROUP_NODE, full_path);
	}
	if (dump_bmp & VENDOR_AI_NET_CMD_MCTRL_ENTRY) {
		snprintf(full_path, CMD_MAX_PATH_LEN, "/mnt/sd//mctrl_entry.txt");
		ret = _vendor_ai_net_debug_dump(net_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MCTRL_ENTRY, full_path);
	}
	if (dump_bmp & VENDOR_AI_NET_CMD_GROUP) {
		snprintf(full_path, CMD_MAX_PATH_LEN, "/mnt/sd//group.txt");
		ret = _vendor_ai_net_debug_dump(net_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_GROUP, full_path);
	}
	if (dump_bmp & VENDOR_AI_NET_CMD_MEM_LIST) {
		snprintf(full_path, CMD_MAX_PATH_LEN, "/mnt/sd//mem_list.txt");
		ret = _vendor_ai_net_debug_dump(net_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MEM_ENTRY, full_path);
	}

	return ret;
}

HD_RESULT vendor_ais_update_diff_mem(VENDOR_AIS_FLOW_UPDATE_NET_INFO *p_info)
{
	UINT32 net_id = p_info->net_id;

	if (vendor_ais_flow_fd[net_id+1] < 0) {
		DBG_ERR("vendor ai ioctl not init...\r\n");
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_PROC_UPDATE_INFO, p_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_PROC_UPDATE_INFO ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
	return HD_OK;
}

HD_RESULT vendor_ais_restore_diff_mem(VENDOR_AIS_FLOW_UPDATE_NET_INFO *p_info)
{
	UINT32 net_id = p_info->net_id;

	if (vendor_ais_flow_fd[net_id+1] < 0) {
		DBG_ERR("vendor ai ioctl not init...\r\n");
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_RESTORE_UPDATE_INFO, p_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_RESTORE_UPDATE_INFO ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
	return HD_OK;
}

HD_RESULT vendor_ais_get_multiscale_max_dim(UINT32 proc_id, HD_DIM *p_max_dim)
{
	VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_HEAD* p_model_diff_head = NULL;
	NN_DIFF_MODEL_HEAD* p_model_head = NULL;
	UINT32 model_num = 0;
	UINT32 max_res_idx = 0; // 0 for max resolution index

	if (p_max_dim == NULL) {
		DBG_ERR("p_max_dim is NULL\r\n");
		return HD_ERR_ABORT;
	}
	p_cfg = _vendor_ai_cfg(proc_id);
	p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
	if (p_diff_resinfo == NULL || p_diff_resinfo->diff_model.va == 0) {
		DBG_ERR("invalid p_diff_resinfo!!\r\n");
		return HD_ERR_ABORT;
	}
	p_model_head = (NN_DIFF_MODEL_HEAD *)(p_diff_resinfo->diff_model.va);
	if (p_model_head == NULL) {
		DBG_ERR("invalid p_model_head!!\r\n");
		return HD_ERR_ABORT;
	}

	model_num = p_model_head->stripe_model_num;
	p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((uintptr_t)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + max_res_idx*sizeof(NN_DIFF_MODE_CTRL));
	p_model_diff_head = (NN_DIFF_HEAD*)((uintptr_t)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
	p_max_dim->w = p_model_diff_head->diff_id.scale_w;
	p_max_dim->h = p_model_diff_head->diff_id.scale_h;

	//printf("%s:%d max_res_idx(%lu) curr_dim(%ldx%ld)\n", __func__, __LINE__, max_res_idx, p_max_dim->w, p_max_dim->h);

	return HD_OK;
}

#define MULTISCALE_SEARCH_FIT_SIZE 1
HD_RESULT vendor_ais_set_model_id(VENDOR_AI_DIFF_MODEL_RESINFO *p_resinfo)
{
	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_HEAD* p_model_diff_head = NULL;
	NN_DIFF_MODEL_HEAD* p_model_head = NULL;
	UINT32 model_num = 0;
	UINT32 model_idx = 0;
	UINT32 model_width = 0;
	UINT32 model_height = 0;
	UINT32 new_idx = 0;
	UINT32 new_width = 0;
	UINT32 new_height = 0;
#if MULTISCALE_SEARCH_FIT_SIZE
	UINT32 model_diff_size = 0;
	UINT32 fit_diff_size = 1920*1080;
	UINT32 fit_model_idx = 0;
	UINT32 fit_model_width = 0;
	UINT32 fit_model_height = 0;
#endif

	if (p_resinfo == NULL || p_resinfo->diff_model.va == 0) {
		return HD_ERR_ABORT;
	}
	p_model_head = (NN_DIFF_MODEL_HEAD *)(p_resinfo->diff_model.va);

	//printf("%s:%d INPUT\n", __func__, __LINE__);
	//printf("    new_id(%lu) curr_id(%lu)\n", p_resinfo->new_id, p_resinfo->curr_id);
	//printf("    new_dim = %dx%d, curr_dim=%dx%d\n", (unsigned int)p_resinfo->new_dim.w, (unsigned int)p_resinfo->new_dim.h, (unsigned int)p_resinfo->curr_dim.w, (unsigned int)p_resinfo->curr_dim.h);

	/* Set which to be update */
	model_num = p_model_head->stripe_model_num;
	if (p_resinfo->new_id != p_resinfo->curr_id) {
		//printf("new id = %lu, curr id(%lu) dim(%ldx%ld)\n", p_resinfo->new_id, p_resinfo->curr_id, p_resinfo->curr_dim.w, p_resinfo->curr_dim.h);
		new_idx = p_resinfo->new_id;
		if (new_idx > model_num) {
			DBG_ERR("RES_ID = %ld > max num = %ld\r\n", new_idx, model_num);
			return HD_ERR_LIMIT;
		}
		p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + new_idx*sizeof(NN_DIFF_MODE_CTRL));
		p_model_diff_head = (NN_DIFF_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
		p_resinfo->curr_id = new_idx;
		p_resinfo->curr_dim.w = p_model_diff_head->diff_id.scale_w;
		p_resinfo->curr_dim.h = p_model_diff_head->diff_id.scale_h;

	} else if ((p_resinfo->new_dim.w != p_resinfo->curr_dim.w) || (p_resinfo->new_dim.h != p_resinfo->curr_dim.h)) {
		new_width = p_resinfo->new_dim.w;
		new_height = p_resinfo->new_dim.h;
		//printf("model num = %d\n", (unsigned int)model_num);
		for (model_idx = 0; model_idx < model_num; model_idx++) {
			p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_idx*sizeof(NN_DIFF_MODE_CTRL));
			p_model_diff_head = (NN_DIFF_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);

			model_width = p_model_diff_head->diff_id.scale_w;
			model_height = p_model_diff_head->diff_id.scale_h;
			//printf("model = %dx%d, input=%dx%d\n", (unsigned int)model_width, (unsigned int)model_height, (unsigned int)new_width, (unsigned int)new_height);
#if MULTISCALE_SEARCH_FIT_SIZE
			if (model_width >= new_width && model_height >= new_height) {
				model_diff_size = model_width*model_height - new_width*new_height;
				if (fit_diff_size >= model_diff_size) {
					fit_diff_size = model_diff_size;
					fit_model_idx = model_idx;
					fit_model_width = model_width;
					fit_model_height = model_height;
				}
			} else if (model_idx == 0) {
				fit_model_width = model_width;
				fit_model_height = model_height;
			}
#else
			if (model_width == new_width && model_height == new_height) {
				break;
			} else if (model_width < new_width || model_height < new_height) {
				break;
			}
#endif
		}

#if MULTISCALE_SEARCH_FIT_SIZE
		p_resinfo->curr_id = fit_model_idx;
		p_resinfo->curr_dim.w = fit_model_width;
		p_resinfo->curr_dim.h = fit_model_height;
#else
		p_resinfo->curr_id = model_idx;
		p_resinfo->curr_dim.w = model_width;
		p_resinfo->curr_dim.h = model_height;
#endif

	} else {
		new_idx = p_resinfo->new_id;;
		p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + new_idx*sizeof(NN_DIFF_MODE_CTRL));
		p_model_diff_head = (NN_DIFF_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
		p_resinfo->curr_id = new_idx;
		p_resinfo->curr_dim.w = p_model_diff_head->diff_id.scale_w;
		p_resinfo->curr_dim.h = p_model_diff_head->diff_id.scale_h;
	}

	//printf("%s:%d OUTPUT\n", __func__, __LINE__);
	//printf("    curr_id(%lu) curr_dim(%ldx%ld)\n", p_resinfo->curr_id, p_resinfo->curr_dim.w, p_resinfo->curr_dim.h);
	return HD_OK;
}

VOID print_nn_diff_parm(NN_DIFF_PARM p_diff_param){
	printf("p_diff_param value check:\n");
	printf("stripe_inaddr[0]:%lu, stripe_inaddr[1]:%lu, stripe_inaddr[2]:%lu\n", p_diff_param.stripe_inaddr[0], p_diff_param.stripe_inaddr[1], p_diff_param.stripe_inaddr[2]);
	printf("stripe_outaddr[0]:%lu, stripe_outaddr[1]:%lu, stripe_outaddr[2]:%lu\n", p_diff_param.stripe_outaddr[0], p_diff_param.stripe_outaddr[1], p_diff_param.stripe_outaddr[2]);
	printf("in_width:%lu, in_height:%lu, batch:%lu, channel:%lu\n",p_diff_param.in_width, p_diff_param.in_height, p_diff_param.batch, p_diff_param.channel);
	printf("in_ofs[0].line_ofs:%lu, in_ofs[1].line_ofs:%lu, in_ofs[2].line_ofs:%lu, out_ofs[0].line_ofs:%lu, out_ofs[1].line_ofs:%lu, out_ofs[2].line_ofs:%lu\n",p_diff_param.in_ofs[0].line_ofs, p_diff_param.in_ofs[1].line_ofs, p_diff_param.in_ofs[2].line_ofs, p_diff_param.out_ofs[0].line_ofs, p_diff_param.out_ofs[1].line_ofs, p_diff_param.out_ofs[2].line_ofs);
	//printf("conv_pad:%u, deconv_pad:%u, pool_pad:%u\n", p_diff_param.conv_pad, p_diff_param.deconv_pad, p_diff_param.pool_pad);
	printf("h_stripe_en:%u, v_stripe_en:%u, skip_en:%u\n", p_diff_param.h_stripe_en, p_diff_param.v_stripe_en, p_diff_param.skip_en);
	printf("is_top_pad:%u, is_bot_pad:%u, is_left_pad:%u, is_right_pad:%u\n", p_diff_param.is_top_pad, p_diff_param.is_bot_pad, p_diff_param.is_left_pad, p_diff_param.is_right_pad);
	printf("tc_width:%lu, tc_height:%lu, sca_width:%lu, sca_height:%lu\n", p_diff_param.tc_width, p_diff_param.tc_height, p_diff_param.sca_width, p_diff_param.sca_height);
	printf("mean_width:%lu, mean_height:%lu, pad_out_width:%lu, pad_out_height:%lu\n", p_diff_param.mean_width, p_diff_param.mean_height, p_diff_param.pad_out_width, p_diff_param.pad_out_height);
}

VOID print_nn_diff_parm_multiCust(NN_DIFF_PARM_MULTICUST p_diff_param){
	UINT32 i = 0;

	printf("p_diff_param value check:\n");
	printf("stripe_inaddr[0]:%lu, stripe_inaddr[1]:%lu, stripe_inaddr[2]:%lu\n", p_diff_param.stripe_inaddr[0], p_diff_param.stripe_inaddr[1], p_diff_param.stripe_inaddr[2]);
	printf("stripe_outaddr[0]:%lu, stripe_outaddr[1]:%lu, stripe_outaddr[2]:%lu\n", p_diff_param.stripe_outaddr[0], p_diff_param.stripe_outaddr[1], p_diff_param.stripe_outaddr[2]);
	printf("in_width:%lu, in_height:%lu, batch:%lu, channel:%lu\n",p_diff_param.in_width, p_diff_param.in_height, p_diff_param.batch, p_diff_param.channel);
	printf("in_ofs[0].line_ofs:%lu, in_ofs[1].line_ofs:%lu, in_ofs[2].line_ofs:%lu, out_ofs[0].line_ofs:%lu, out_ofs[1].line_ofs:%lu, out_ofs[2].line_ofs:%lu\n",p_diff_param.in_ofs[0].line_ofs, p_diff_param.in_ofs[1].line_ofs, p_diff_param.in_ofs[2].line_ofs, p_diff_param.out_ofs[0].line_ofs, p_diff_param.out_ofs[1].line_ofs, p_diff_param.out_ofs[2].line_ofs);
	//printf("conv_pad:%u, deconv_pad:%u, pool_pad:%u\n", p_diff_param.conv_pad, p_diff_param.deconv_pad, p_diff_param.pool_pad);
	printf("h_stripe_en:%u, v_stripe_en:%u, skip_en:%u\n", p_diff_param.h_stripe_en, p_diff_param.v_stripe_en, p_diff_param.skip_en);
	printf("is_top_pad:%u, is_bot_pad:%u, is_left_pad:%u, is_right_pad:%u\n", p_diff_param.is_top_pad, p_diff_param.is_bot_pad, p_diff_param.is_left_pad, p_diff_param.is_right_pad);
	printf("tc_width:%lu, tc_height:%lu, sca_width:%lu, sca_height:%lu\n", p_diff_param.tc_width, p_diff_param.tc_height, p_diff_param.sca_width, p_diff_param.sca_height);
	printf("mean_width:%lu, mean_height:%lu, pad_out_width:%lu, pad_out_height:%lu\n", p_diff_param.mean_width, p_diff_param.mean_height, p_diff_param.pad_out_width, p_diff_param.pad_out_height);

	for(i=0; i<7;i++){
		printf("idx = %lu, multi_in_width:%lu, multi_in_height:%lu, multi_in_batch:%lu, multi_in_channel:%lu\n",i, p_diff_param.multi_in_width[i] , p_diff_param.multi_in_height[i], p_diff_param.multi_in_batch[i], p_diff_param.multi_in_channel[i]);
	}
	for(i=0;i<5;i++){
		printf("idx = %lu, multi_in_ofs.line_ofs:%lu, multi_in_ofs.channel_ofs:%lu, multi_in_ofs.batch_ofs:%lu\n", i, p_diff_param.multi_in_ofs[i].line_ofs, p_diff_param.multi_in_ofs[i].channel_ofs, p_diff_param.multi_in_ofs[i].batch_ofs);
	}

}

HD_RESULT vendor_ais_update_diff_iomem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, NN_DIFF_MODEL_HEAD *p_model_head, VENDOR_AI_DIFF_MODEL_RESINFO *p_resinfo, UINT32 mode)
{

	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT er = HD_OK;
	NN_GEN_MODEL_HEAD *p_head;

#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
	NN_IOMEM *p_diff_io_mem;
#else
//	NN_DIFF_IOMEM *p_diff_io_mem;
//	NN_DATA* p_sai = NULL;
//	NN_DATA* p_sao = NULL;
#endif
//	UINT32 buf0_type = 0, buf1_type = 0, buf0_va = 0, buf1_va = 0, buf0_pa = 0, buf1_pa = 0;
//	UINT32 idx = 0;
//	UINT32 layer_index = 0;
//	UINT32 tmp = 0;

	//NN_DATA *p_sai, *p_sao, *p_diff_sai, *p_diff_sao;
	//UINT32 imem_cnt, omem_cnt, idiffmem_cnt, odiffmem_cnt;


	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_HEAD* p_model_diff_head = NULL;
//
	UINT32 model_num;
//	UINT32 model_pa_ofs, buff_pa_ofs;
//	UINT32 model_va_ofs, buff_va_ofs;
	UINT32 model_id = 0;

#if ENABLE_IN_PADDING
	NN_DIFF_PARM* p_diff_param = NULL;
	NN_DIFF_PARM_MULTICUST* p_diff_param_multiCust = NULL;	//for multi-scale with multiCust
#endif

	UINT32 proc_idx = 0, proc_num = 0;
	NN_GEN_MODE_CTRL *p_mctrl;

	if (p_mem == NULL || p_model_head == NULL || p_resinfo == NULL) {
		DBG_ERR("input pointer is null...\r\n");
        return -1;
	}

	if (p_mem->user_parm.va == 0) {
		DBG_ERR("input addr is null...\r\n");
        return -1;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
    if (er != HD_OK) {
        DBG_ERR("vendor_ais_get_net_info fail...\r\n");
        return er;
    }

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	UINT8 fmt_sub  = (UINT8)(((p_head->chip.id) & 0xFF000000) >> 24);
	UINT8 fmt_sub_multiCust = (UINT8)(fmt_sub & NN_CHIP_AI_SUBVER3_MASK);
	// printf("id(0x%08x), fmt_sub(0x%02x)\r\n",(unsigned int)p_head->chip.id, (unsigned int)fmt_sub);
	// printf("fmt_sub_multiCust = 0x%02x\n",(unsigned int) fmt_sub_multiCust);

#if CNN_25_MATLAB
	p_io_mem = net_info.p_io_mem;
#else
	//p_sai = (NN_DATA*)p_mctrl[process_index].iomem.imem_addr;
	//p_sao = (NN_DATA*)p_mctrl[process_index].iomem.omem_addr;
#endif
	proc_num = p_head->mode_ctrl_num;
	model_num = p_model_head->stripe_model_num;
	model_id = p_resinfo->curr_id;

	if (model_id >= model_num) {
		DBG_ERR("model id is exceed the max diff model num\r\n");
        return -1;
	}

	p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_id*sizeof(NN_DIFF_MODE_CTRL));
	p_model_diff_head = (NN_DIFF_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
#if CNN_25_MATLAB
	p_diff_io_mem = (NN_IOMEM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_HEAD));
#else
//	p_diff_io_mem = (NN_DIFF_IOMEM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_HEAD));
#endif
#if ENABLE_IN_PADDING
	if (((p_head->chip.gentool_vers & 0xFF000000)>>24) <= 0x15) { // for gentool 2.0
		p_diff_param  = (NN_DIFF_PARM*)((uintptr_t)p_model_diff_head + sizeof(NN_DIFF_HEAD) + p_head->layer_num*sizeof(NN_DIFF_IOMEM));
	} else { // for AI_Tool 3.0
		if(fmt_sub_multiCust == NN_CHIP_AI_SUBVER3){
			//printf("use NN_DIFF_PARM_MULTICUST\n");
			p_diff_param_multiCust  = (NN_DIFF_PARM_MULTICUST*)((uintptr_t)p_model_diff_head + sizeof(NN_DIFF_HEAD) + p_model_diff_head->io_buff_size);
		}
		else{
			//printf("use NN_DIFF_PARM\n");
			p_diff_param  = (NN_DIFF_PARM*)((uintptr_t)p_model_diff_head + sizeof(NN_DIFF_HEAD) + p_model_diff_head->io_buff_size);
		}
	}
#endif

/*
	model_va_ofs = ALIGN_CEIL_4(p_mem->user_model.va);
	buff_va_ofs = ALIGN_CEIL_4(p_mem->user_buff.va);
	model_pa_ofs = ALIGN_CEIL_4(p_mem->user_model.pa);
	buff_pa_ofs = ALIGN_CEIL_4(p_mem->user_buff.pa);


	if (buff_va_ofs > model_va_ofs) {
		buf1_va = buff_va_ofs;
		buf1_pa = buff_pa_ofs;
		buf1_type = NN_GEN_BUF_ADDR_TYPE;
		buf0_va = model_va_ofs;
		buf0_pa = model_pa_ofs;
		buf0_type = NN_GEN_MODEL_ADDR_TYPE;
	} else {
		buf1_va = model_va_ofs;
		buf1_pa = model_pa_ofs;
		buf1_type = NN_GEN_MODEL_ADDR_TYPE;
		buf0_va = buff_va_ofs;
		buf0_pa = buff_pa_ofs;
		buf0_type = NN_GEN_BUF_ADDR_TYPE;
	}
*/
#if CNN_25_MATLAB
	for (layer_index = 0; layer_index < p_head->layer_num; layer_index++) {
		for (idx = 0; idx < NN_IMEM_NUM; idx++) {
			if (layer_index == 0) {
				continue;
			}
			if (p_diff_io_mem[layer_index].SAI[idx].va == 0) {
				continue;
			}
			//printf("layer %d in %d addr = 0x%08X\n", (int)layer_index, (int)idx, (unsigned int)p_diff_io_mem[layer_index].SAI[idx].va);

			if (mode == 0) {
				if ((p_diff_io_mem[layer_index].SAI[idx].va & NN_GEN_BUF_ADDR_TYPE) > 0) {
					p_diff_io_mem[layer_index].SAI[idx].va = (p_diff_io_mem[layer_index].SAI[idx].va & 0xFFFFFFF) + buff_va_ofs;
					p_diff_io_mem[layer_index].SAI[idx].pa = (p_diff_io_mem[layer_index].SAI[idx].pa & 0xFFFFFFF) + buff_pa_ofs;
				} else {
					p_diff_io_mem[layer_index].SAI[idx].va = (p_diff_io_mem[layer_index].SAI[idx].va & 0xFFFFFFF) + model_va_ofs;
					p_diff_io_mem[layer_index].SAI[idx].pa = (p_diff_io_mem[layer_index].SAI[idx].pa & 0xFFFFFFF) + model_pa_ofs;
				}

			} else {
				if (p_io_mem[layer_index].SAI[idx].va > buf1_va) {
					p_io_mem[layer_index].SAI[idx].va = (p_io_mem[layer_index].SAI[idx].va - buf1_va) | buf1_type;
					p_io_mem[layer_index].SAI[idx].pa = (p_io_mem[layer_index].SAI[idx].pa - buf1_pa) | buf1_type;
				} else {
					p_io_mem[layer_index].SAI[idx].va = (p_io_mem[layer_index].SAI[idx].va - buf0_va) | buf0_type;
					p_io_mem[layer_index].SAI[idx].pa = (p_io_mem[layer_index].SAI[idx].pa - buf0_pa) | buf0_type;
				}
			}
			SWAP(p_diff_io_mem[layer_index].SAI[idx].va, p_io_mem[layer_index].SAI[idx].va, tmp);
			SWAP(p_diff_io_mem[layer_index].SAI[idx].pa, p_io_mem[layer_index].SAI[idx].pa, tmp);
			SWAP(p_diff_io_mem[layer_index].SAI[idx].size, p_io_mem[layer_index].SAI[idx].size, tmp);
		}
		for (idx = 0; idx < NN_OMEM_NUM; idx++) {
			if (p_diff_io_mem[layer_index].SAO[idx].va == 0)
				continue;
			//printf("layer %d out %d addr = 0x%08X\n", (int)layer_index, (int)idx, (unsigned int)p_diff_io_mem[layer_index].SAO[idx].va);
			if (mode == 0) {
				p_diff_io_mem[layer_index].SAO[idx].va = (p_diff_io_mem[layer_index].SAO[idx].va & 0xFFFFFFF) + buff_va_ofs;
				p_diff_io_mem[layer_index].SAO[idx].pa = (p_diff_io_mem[layer_index].SAO[idx].pa & 0xFFFFFFF) + buff_pa_ofs;
			} else {
				p_io_mem[layer_index].SAO[idx].va = (p_io_mem[layer_index].SAO[idx].va - buff_va_ofs) | NN_GEN_BUF_ADDR_TYPE;
				p_io_mem[layer_index].SAO[idx].pa = (p_io_mem[layer_index].SAO[idx].pa - buff_pa_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			SWAP(p_diff_io_mem[layer_index].SAO[idx].va, p_io_mem[layer_index].SAO[idx].va, tmp);
			SWAP(p_diff_io_mem[layer_index].SAO[idx].pa, p_io_mem[layer_index].SAO[idx].pa, tmp);
			SWAP(p_diff_io_mem[layer_index].SAO[idx].size, p_io_mem[layer_index].SAO[idx].size, tmp);
		}
	}
#else

/*
 *  No need to update user parm because it's only for ai1 flow,
 *  Keep the useless code for sync
 *
	layer_index = 0;
	for (proc_idx = 0; proc_idx < proc_num; proc_idx++) {
		if (p_mctrl[proc_idx].layer_index == layer_index) {
			p_sai = (NN_DATA*)p_mctrl[proc_idx].iomem.imem_addr;
			p_sao = (NN_DATA*)p_mctrl[proc_idx].iomem.omem_addr;
			for (idx = 0; idx < p_mctrl[proc_idx].iomem.imem_cnt; idx++) {
				if (layer_index == 0) {
					break;
				}
				if (p_diff_io_mem[layer_index].SAI[idx].va == 0) {
					continue;
				}
				if (mode == 0) {
					if ((p_diff_io_mem[layer_index].SAI[idx].va & NN_GEN_BUF_ADDR_TYPE) > 0) {
						p_diff_io_mem[layer_index].SAI[idx].va = (p_diff_io_mem[layer_index].SAI[idx].va & 0xFFFFFFF) + buff_va_ofs;
						p_diff_io_mem[layer_index].SAI[idx].pa = (p_diff_io_mem[layer_index].SAI[idx].pa & 0xFFFFFFF) + buff_pa_ofs;
					} else {
						p_diff_io_mem[layer_index].SAI[idx].va = (p_diff_io_mem[layer_index].SAI[idx].va & 0xFFFFFFF) + model_va_ofs;
						p_diff_io_mem[layer_index].SAI[idx].pa = (p_diff_io_mem[layer_index].SAI[idx].pa & 0xFFFFFFF) + model_pa_ofs;
					}

				} else {
					if (p_sai[idx].va > buf1_va) {
						p_sai[idx].va = (p_sai[idx].va - buf1_va) | buf1_type;
						p_sai[idx].pa = (p_sai[idx].pa - buf1_pa) | buf1_type;
					} else {
						p_sai[idx].va = (p_sai[idx].va - buf0_va) | buf0_type;
						p_sai[idx].pa = (p_sai[idx].pa - buf0_pa) | buf0_type;
					}
				}
				SWAP(p_diff_io_mem[layer_index].SAI[idx].va, p_sai[idx].va, tmp);
				SWAP(p_diff_io_mem[layer_index].SAI[idx].pa, p_sai[idx].pa, tmp);
				SWAP(p_diff_io_mem[layer_index].SAI[idx].size, p_sai[idx].size, tmp);
			}

			for (idx = 0; idx < p_mctrl[proc_idx].iomem.omem_cnt; idx++) {
				if (p_diff_io_mem[layer_index].SAO[idx].va == 0) {
					continue;
				}
				//printf("layer %d out %d addr = 0x%08X\n", (int)layer_index, (int)idx, (unsigned int)p_diff_io_mem[layer_index].SAO[idx].va);
				if (mode == 0) {
					p_diff_io_mem[layer_index].SAO[idx].va = (p_diff_io_mem[layer_index].SAO[idx].va & 0xFFFFFFF) + buff_va_ofs;
					p_diff_io_mem[layer_index].SAO[idx].pa = (p_diff_io_mem[layer_index].SAO[idx].pa & 0xFFFFFFF) + buff_pa_ofs;
				} else {
					p_sao[idx].va = (p_sao[idx].va - buff_va_ofs) | NN_GEN_BUF_ADDR_TYPE;
					p_sao[idx].pa = (p_sao[idx].pa - buff_pa_ofs) | NN_GEN_BUF_ADDR_TYPE;
				}
				SWAP(p_diff_io_mem[layer_index].SAO[idx].va, p_sao[idx].va, tmp);
				SWAP(p_diff_io_mem[layer_index].SAO[idx].pa, p_sao[idx].pa, tmp);
				SWAP(p_diff_io_mem[layer_index].SAO[idx].size, p_sao[idx].size, tmp);
			}

			layer_index++;
			if (layer_index == p_head->layer_num) {
				break;
			}
		}
	}
*/
#endif


	for (proc_idx = 0; proc_idx < proc_num; proc_idx++) {
		if (p_mctrl[proc_idx].mode == NN_CUSTOMER) {
			NN_CUSTOM_PARM *p_cust_head = (NN_CUSTOM_PARM *)(p_mctrl[proc_idx].addr);
#if 1
			UINT32 input_num = p_cust_head->input_num;
			UINT32 output_num = p_cust_head->output_num;
			UINT32 model_num = p_cust_head->model_num;

			if(fmt_sub_multiCust == NN_CHIP_AI_SUBVER3){
				//print_nn_diff_parm_multiCust(p_diff_param_multiCust[proc_idx]);
				NN_CUSTOM_DIM *p_dim_info = (NN_CUSTOM_DIM*)(p_mctrl[proc_idx].addr + sizeof(NN_CUSTOM_PARM) + (input_num+output_num+model_num)*sizeof(NN_DATA));
				
				for(UINT32 in_idx = 0; in_idx<input_num; in_idx++){
					if(in_idx<1){
						p_dim_info[in_idx].dim[0] = p_diff_param_multiCust[proc_idx].in_width; //size_width
						p_dim_info[in_idx].dim[1] = p_diff_param_multiCust[proc_idx].in_height;//size_height
						p_dim_info[in_idx].dim[2] = p_diff_param_multiCust[proc_idx].channel;  //size_channel
						p_dim_info[in_idx].dim[3] = p_diff_param_multiCust[proc_idx].batch;    //batch_num 
					}
					else{
						p_dim_info[in_idx].dim[0] = p_diff_param_multiCust[proc_idx].multi_in_width[in_idx-1]; //size_width
						p_dim_info[in_idx].dim[1] = p_diff_param_multiCust[proc_idx].multi_in_height[in_idx-1];//size_height
						p_dim_info[in_idx].dim[2] = p_diff_param_multiCust[proc_idx].multi_in_channel[in_idx-1];  //size_channel
						p_dim_info[in_idx].dim[3] = p_diff_param_multiCust[proc_idx].multi_in_batch[in_idx-1];    //batch_num
					}

					if(in_idx<3){
						p_dim_info[in_idx].ofs[0] = p_diff_param_multiCust[proc_idx].in_ofs[in_idx].line_ofs; //in_ofs_line_ofs
						p_dim_info[in_idx].ofs[1] = p_diff_param_multiCust[proc_idx].in_ofs[in_idx].channel_ofs; //in_ofs_channel_ofs
						p_dim_info[in_idx].ofs[2] = p_diff_param_multiCust[proc_idx].in_ofs[in_idx].batch_ofs; //in_ofs_batch_ofs
					}
					else{
						p_dim_info[in_idx].ofs[0] = p_diff_param_multiCust[proc_idx].multi_in_ofs[in_idx].line_ofs; //in_ofs_line_ofs
						p_dim_info[in_idx].ofs[1] = p_diff_param_multiCust[proc_idx].multi_in_ofs[in_idx].channel_ofs; //in_ofs_channel_ofs
						p_dim_info[in_idx].ofs[2] = p_diff_param_multiCust[proc_idx].multi_in_ofs[in_idx].batch_ofs; //in_ofs_batch_ofs
					}
					
					// p_dim_info += sizeof(NN_CUSTOM_DIM);
				}

				// for(UINT32 in_idx = 0; in_idx<input_num; in_idx++){
				// 	printf("p_dim_info->dim[0]:%lu, p_dim_info->dim[1]:%lu, p_dim_info->dim[2]:%lu, p_dim_info->dim[3]:%lu\n", p_dim_info[in_idx].dim[0], p_dim_info[in_idx].dim[1], p_dim_info[in_idx].dim[2], p_dim_info[in_idx].dim[3]);
				// }
			}
			else{
				//print_nn_diff_parm(p_diff_param[proc_idx]);
				NN_CUSTOM_DIM *p_dim_info = (NN_CUSTOM_DIM*)(p_mctrl[proc_idx].addr + sizeof(NN_CUSTOM_PARM) + (input_num+output_num+model_num)*sizeof(NN_DATA));
				p_dim_info->dim[0] = p_diff_param[proc_idx].in_width; //size_width
				p_dim_info->dim[1] = p_diff_param[proc_idx].in_height;//size_height
				p_dim_info->dim[2] = p_diff_param[proc_idx].channel;  //size_channel
				p_dim_info->dim[3] = p_diff_param[proc_idx].batch;    //batch_num
				p_dim_info->ofs[0] = p_diff_param[proc_idx].in_ofs[0].line_ofs; //in_ofs_line_ofs
				p_dim_info->ofs[1] = p_diff_param[proc_idx].in_ofs[0].channel_ofs; //in_ofs_channel_ofs
				p_dim_info->ofs[2] = p_diff_param[proc_idx].in_ofs[0].batch_ofs; //in_ofs_batch_ofs
			}

			
#else
#if CUST_SUPPORT_MULTI_IO
			UINT32 input_num = p_cust_head->input_num;
			UINT32 output_num = p_cust_head->output_num;
			UINT32 model_num = p_cust_head->model_num;
			UINT32 dim_addr = (UINT32)(p_mctrl[proc_idx].addr + sizeof(NN_CUSTOM_PARM) + (input_num+output_num+model_num)*sizeof(NN_DATA));
			UINT32 *p_layer_type_id = (UINT32 *)(dim_addr + sizeof(NN_CUSTOM_DIM)*(input_num+output_num));
#else
			UINT32 *p_layer_type_id = (UINT32 *)(p_cust_head + 1);
#endif
			UINT32 *p_weight_num = (UINT32 *)(p_layer_type_id + 1);
			UINT32 weight_num = *p_weight_num;
			UINT32 *p_layer_parm_len = (UINT32 *)(p_weight_num + 1 + weight_num);
			UINT32 *p_isf_len = (UINT32 *)(p_layer_parm_len + 1);
			UINT32 isf_num = (*p_isf_len) / 8;
			INT32 *p_isf_parm = (INT32 *)(p_isf_len + 1);
			UINT32 *p_osf_len = (UINT32 *)(p_isf_parm + isf_num*2);
			UINT32 osf_num = (*p_osf_len) / 8;
			INT32 *p_osf_parm = (INT32 *)(p_osf_len + 1);
			UINT32 *p_bin_parm_len = (UINT32 *)(p_osf_parm + osf_num*2);
			UINT32* cust_parm = (UINT32*)(p_bin_parm_len + 1);
			//printf("origin: %d %d %d %d %d %d\n", (int)cust_parm[3], (int)cust_parm[4], (int)cust_parm[5], (int)cust_parm[6], (int)cust_parm[7], (int)cust_parm[8]);
			cust_parm[3] = p_diff_param[proc_idx].in_width;//size_width
			cust_parm[4] = p_diff_param[proc_idx].in_height;//size_height
			cust_parm[5] = p_diff_param[proc_idx].channel;//size_channel
			cust_parm[6] = p_diff_param[proc_idx].batch;//batch_num
			cust_parm[7] = p_diff_param[proc_idx].in_ofs[0].line_ofs;//in_ofs_line_ofs
			cust_parm[8] = p_diff_param[proc_idx].in_ofs[0].channel_ofs;//in_ofs_channel_ofs
			//printf("after: %d %d %d %d %d %d\n", (int)cust_parm[3], (int)cust_parm[4], (int)cust_parm[5], (int)cust_parm[6], (int)cust_parm[7], (int)cust_parm[8]);
			//cust_parm[9] = p_diff_param[proc_idx].in_ofs[0].batch_ofs;//in_ofs_batch_ofs
#endif

		}
	}
	// update scale w & h for padding
#if ENABLE_IN_PADDING
	/* if (p_resinfo->curr_dim.w > 0 && p_resinfo->curr_dim.h > 0){
		p_diff_param[0].sca_width  = p_resinfo->curr_dim.w;
		p_diff_param[0].sca_height = p_resinfo->curr_dim.h;
	} */
#endif

	return HD_OK;
}

#define NEW_RES_COUNT	1

HD_RESULT vendor_ais_start_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id, UINT32 order, INT32 wait_ms, UINT32 ddr_id)
{
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};
	//VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);

	//net
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
#if (NEW_RES_COUNT == 1)
	UINT32 BIND_CNT = 0;
	UINT32 JOB_CNT = 0;
#else
	UINT32 id_list_size;
#endif

	//op context
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 process_id;
	UINT32 src_count = 0;
	UINT32 dest_count = 0;

	//io context
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
	char order_str[16];

	er = vendor_ais_get_net_info(&net_info, net_mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	p_head = net_info.p_head;
	proc_layer_num = p_head->mode_ctrl_num;

	p_mctrl = net_info.p_mctrl;
#if CNN_25_MATLAB
	p_io_mem = net_info.p_io_mem;
#endif

#if (NEW_RES_COUNT == 1)
	process_id = 0;
	{
		NN_GEN_MODE_CTRL *p_this_mctrl = p_mctrl + process_id;
		_cvt_order_name(order, order_str, 16);

		if ((wait_ms < -1) || (wait_ms > 10000)) {
			DBG_ERR("invalid wait_ms=%d\r\n", (int)wait_ms);
			return HD_ERR_ABORT;
		}
		if ((p_this_mctrl->eng == NN_GEN_ENG_CPU) || (p_this_mctrl->eng == NN_GEN_ENG_DSP)) {
			DBG_ERR("not support job[0] with engine=%d\r\n", (int)p_this_mctrl->eng);
			return HD_ERR_ABORT;
		}

		if (p_this_mctrl->trig_src == NN_GEN_TRIG_APP_AI_DRV) {

			order = 0;
			wait_ms = -1;
			DBG_DUMP("net(app-mode) =>	FORCE ((order=%s sync=%d)\r\n", order_str, (int)wait_ms);

		} else if (p_this_mctrl->trig_src == NN_GEN_TRIG_LL_AI_DRV) {

			//DBG_DUMP("net(ll-mode)\r\n");

		} else {
			DBG_ERR("invalid trig_src=%d of job[0]\r\n", (int)p_this_mctrl->trig_src);
			return HD_ERR_ABORT;
		}

		//bind with linear order
		//DBG_DUMP("- bind_job [order=%s]\r\n", order_str);
		//put with linear order
		//DBG_DUMP("- put_job [order=%s] [wait_ms=%d]\r\n", order_str, (int)wait_ms);
	}

	/////////////////////////////////////////////////////////////////////////////

	vendor_ai_net_trace(net_id, AI_JOB, "start() - bind_job (order=%s sync=%d) - begin\r\n", order_str, (int)wait_ms);

	if (_vendor_ai_net_is_linear_job(order) && _vendor_ai_net_is_optimize_job(order) == FALSE) {
		//--- linear job  w/o optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		//VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_proc->job_opt;

		for (process_id = 0; process_id < proc_layer_num; process_id++) {

			//NN_GEN_MODE_CTRL *p_this_mctrl = NULL;

			//this
			//p_this_mctrl = p_mctrl + process_id;

			//BIND JOB with linear order
			if (process_id < proc_layer_num-1) {
				BIND_CNT++;
			}

			//SET JOB
			JOB_CNT++;
		}
		//DBG_DUMP("proc_id(%lu) res: job=%d, bind=%d\r\n", net_id, (int)JOB_CNT, (int)BIND_CNT);
		p_proc->info.total_job_cnt = JOB_CNT;
		p_proc->info.total_bind_cnt = BIND_CNT;

	} else if (_vendor_ai_net_is_linear_job(order) && _vendor_ai_net_is_optimize_job(order) == TRUE) {
		//--- linear job with optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		//VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_proc->job_opt;

		if (p_proc->priv.b_simplify_bin == TRUE) {
			//<simplify model bin> mode (find those [HW layer whoes tot_trig_eng_times > 0] & [CPU layer], then bind/set job)
			INT32 bind_id[2] = {-1, -1};  // pre & next
			//NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			for (process_id = 0; process_id < proc_layer_num; process_id++) {
				if ((p_mctrl[process_id].tot_trig_eng_times > 0) || (p_mctrl[process_id].trig_src == NN_GEN_TRIG_COMMON)) {

					bind_id[0] = bind_id[1];
					bind_id[1] = (INT32)process_id;

					if (bind_id[0] >= 0) {
						//this
						//p_this_mctrl = p_mctrl + (UINT32)bind_id[0];
						//BIND JOB
						BIND_CNT++;
						//SET JOB
						JOB_CNT++;
					}
				}
			}
			// last job
			if (bind_id[1] >= 0) {
				//this
				//p_this_mctrl = p_mctrl + (UINT32)bind_id[1];
				//SET JOB
				JOB_CNT++;
			}

		} else {
			//<normal> mode
			VENDOR_AI_NET_LLGROUP_LIST *p_group_list = {0};
			UINT32 j;

			p_group_list = vendor_ai_net_group_getgroupresult(net_id);
			if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
				DBG_ERR("null p_group_list\r\n");
				return HD_ERR_INV;
			}

			for (j = 0; j < p_group_list->group_num; j++) {

				//NN_GEN_MODE_CTRL *p_tail_mctrl = NULL;
				//NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
				//UINT32 tail_process_id;
				//UINT32 next_process_id;

				//this
				//p_this_mctrl = p_group_list->p_group[j].p_head;
				//process_id = p_this_mctrl - p_mctrl;

				//BIND JOB with linear order
				//p_tail_mctrl = p_group_list->p_group[j].p_tail;
				//tail_process_id = p_tail_mctrl - p_mctrl;
				if (j < (p_group_list->group_num-1)) {
					//next_process_id = tail_process_id + 1;
					BIND_CNT++;
				}

				//SET JOB
				JOB_CNT++;
			}
		}
		//DBG_DUMP("proc_id(%lu) res: job=%d, bind=%d\r\n", net_id, (int)JOB_CNT, (int)BIND_CNT);
		p_proc->info.total_job_cnt = JOB_CNT;
		p_proc->info.total_bind_cnt = BIND_CNT;

	} else if (_vendor_ai_net_is_graph_job(order) && _vendor_ai_net_is_optimize_job(order) == FALSE) {
		//--- graph job w/o optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		//VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_proc->job_opt;

		for (process_id = 0; process_id < proc_layer_num; process_id++) {

			NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			//UINT32* next_process_list = NULL;
			UINT32 i;

			//this
			p_this_mctrl = p_mctrl + process_id;

			//BIND JOB with graph order
			//next_process_list = (UINT32 *)(((char*)net_info.p_id_list) + p_this_mctrl->next_layer_idx_addr);
			for(i = 0; i < p_this_mctrl->next_num; i++) {
				//UINT32 next_process_id = next_process_list[i];
				BIND_CNT++;
			}

			//SET JOB
			JOB_CNT++;
		}
		//DBG_DUMP("proc_id(%lu) res: job=%d, bind=%d\r\n", net_id, (int)JOB_CNT, (int)BIND_CNT);
		p_proc->info.total_job_cnt = JOB_CNT;
		p_proc->info.total_bind_cnt = BIND_CNT;

	} else if (_vendor_ai_net_is_graph_job(order) && _vendor_ai_net_is_optimize_job(order) == TRUE) {
		//--- graph job with optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		//VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_proc->job_opt;
		VENDOR_AI_NET_LLGROUP_LIST *p_group_list = {0};
		UINT32 j;

		p_group_list = vendor_ai_net_group_getgroupresult(net_id);
		if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
			DBG_ERR("null p_group_list\r\n");
			return HD_ERR_INV;
		}

		for (j = 0; j < p_group_list->group_num; j++) {

			NN_GEN_MODE_CTRL *p_tail_mctrl = NULL;
			NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			//UINT32* next_process_list = NULL;
			UINT32 i;

			//this
			p_this_mctrl = p_group_list->p_group[j].p_head;
			process_id = p_this_mctrl - p_mctrl;

			//BIND JOB with graph order
			p_tail_mctrl = p_group_list->p_group[j].p_tail;
			//next_process_list = (UINT32 *)(((char*)net_info.p_id_list) + p_tail_mctrl->next_layer_idx_addr);
			for(i = 0; i < p_tail_mctrl->next_num; i++) {
				//UINT32 next_process_id = next_process_list[i];
				BIND_CNT++;
			}

			//SET JOB
			JOB_CNT++;

		}
		//DBG_DUMP("proc_id(%lu) res: job=%d, bind=%d\r\n", net_id, (int)JOB_CNT, (int)BIND_CNT);
		p_proc->info.total_job_cnt = JOB_CNT;
		p_proc->info.total_bind_cnt = BIND_CNT;

	} else {

		DBG_ERR("proc_id(%lu) not support job-opt %lu\r\n", net_id, order);
		return HD_ERR_FAIL;
	}

#else

	if (p_proc->priv.b_simplify_bin == TRUE) {
		id_list_size = proc_layer_num * 2 * sizeof(UINT32);  // SIMPLIFY BIN (id_list_size = 0 in bin) could only run on linear mode, so graph size should be at most equal to mctrl_layer_num * 2 ( pre/next for each layer )
	} else {
		id_list_size = p_head->layer_id_list_size;
	}

#endif


	//OPEN
#if (0)
	//DBG_DUMP(""model: total %d layers\r\n", (int)proc_layer_num);
#endif
#if (NEW_RES_COUNT == 1)
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - create_joblist (%u max_jobs, %u jobs, %u binds)\r\n", proc_layer_num, JOB_CNT, BIND_CNT*2);
	vendor_ai_dla_create_joblist(net_id, proc_layer_num, JOB_CNT, BIND_CNT*2, ddr_id);
#else
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - create_joblist (%u max_jobs, %u jobs, %u binds)\r\n", proc_layer_num, proc_layer_num, id_list_size);
	vendor_ai_dla_create_joblist(net_id, proc_layer_num, proc_layer_num, id_list_size, ddr_id);
#endif
	//vendor_ai_net_trace(net_id, AI_FLOW, "start() - job_callback_thread_init\r\n");
	//_vendor_ai_job_callback_thread_init(net_id); //job done callback
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - cpu_callback_thread_init\r\n");
	{
		//VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(net_id);
		VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_cfg->job_opt;
		int cpu_bind = (p_job_opt->schd_parm & 0x0000ff00) >> 8;
#if defined(_BSP_NA51090_)   //cache sync is fast enough, not support cpu binding
		if (cpu_bind == 0) {
			vendor_ai_cpu_thread_setbind(net_id, 0xff);
			//DBG_ERR("cpu_bind=0xff\r\n");
		} else {
			vendor_ai_cpu_thread_setbind(net_id, 0xff);
			DBG_ERR("start() - not support cpu_bind=%d!\r\n", (int)cpu_bind);
		}
#else
		if (cpu_bind == 0) {
			vendor_ai_cpu_thread_setbind(net_id, 0xff);
			//DBG_ERR("cpu_bind=0xff\r\n");
		} else if (cpu_bind == 1) {
			vendor_ai_cpu_thread_setbind(net_id, 0x00);
			//DBG_ERR("cpu_bind=0x00\r\n");
		} else if (cpu_bind == 2) {
			vendor_ai_cpu_thread_setbind(net_id, 0x01);
			//DBG_ERR("cpu_bind=0x01\r\n");
		} else {
			vendor_ai_cpu_thread_setbind(net_id, 0xff);
			DBG_ERR("start() - invalid cpu_bind=%d?\r\n", (int)cpu_bind);
		}
#endif
	}
	vendor_ai_cpu_thread_init(net_id); //cpu exec callback
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - dsp_callback_thread_init\r\n");
	vendor_ai_dsp_thread_init(net_id); //dsp exec callback

	process_id = 0;
	{
		NN_GEN_MODE_CTRL *p_this_mctrl = p_mctrl + process_id;
		_cvt_order_name(order, order_str, 16);

		if ((wait_ms < -1) || (wait_ms > 10000)) {
			DBG_ERR("invalid wait_ms=%d\r\n", (int)wait_ms);
			return HD_ERR_ABORT;
		}
		if ((p_this_mctrl->eng == NN_GEN_ENG_CPU) || (p_this_mctrl->eng == NN_GEN_ENG_DSP)) {
			DBG_ERR("not support job[0] with engine=%d\r\n", (int)p_this_mctrl->eng);
			return HD_ERR_ABORT;
		}

		if (p_this_mctrl->trig_src == NN_GEN_TRIG_APP_AI_DRV) {

			order = 0;
			wait_ms = -1;
			DBG_DUMP("net(app-mode) =>  FORCE ((order=%s sync=%d)\r\n", order_str, (int)wait_ms);

		} else if (p_this_mctrl->trig_src == NN_GEN_TRIG_LL_AI_DRV) {

			//DBG_DUMP("net(ll-mode)\r\n");

		} else {
			DBG_ERR("invalid trig_src=%d of job[0]\r\n", (int)p_this_mctrl->trig_src);
			return HD_ERR_ABORT;
		}

		//bind with linear order
		//DBG_DUMP("- bind_job [order=%s]\r\n", order_str);
		//put with linear order
		//DBG_DUMP("- put_job [order=%s] [wait_ms=%d]\r\n", order_str, (int)wait_ms);
	}

	/////////////////////////////////////////////////////////////////////////////

	vendor_ai_net_trace(net_id, AI_JOB, "start() - bind_job (order=%s sync=%d) - begin\r\n", order_str, (int)wait_ms);

	if (_vendor_ai_net_is_linear_job(order) && _vendor_ai_net_is_optimize_job(order) == FALSE) {
		//--- linear job  w/o optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(net_id);
		VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_cfg->job_opt;

		//add job (node of DAG)
		for (process_id = 0; process_id < proc_layer_num; process_id++) {

			NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			NN_IOMEM *p_this_buffer = NULL;
			UINT32 engine_id;
			UINT32 engine_op;

			//this
			p_this_mctrl = p_mctrl + process_id;
#if CNN_25_MATLAB
			p_this_buffer = p_io_mem + p_this_mctrl->layer_index;
#else
			p_this_buffer = (NN_IOMEM *)&p_this_mctrl->iomem;
#endif
			//SET JOB
			_vendor_ai_job_select_eng(process_id, p_this_mctrl, &engine_id, &engine_op);
			vendor_ai_dla_set_job(net_id, process_id, engine_id, engine_op, p_job_opt->schd_parm, (void*)p_this_mctrl, (void*)p_this_buffer, wait_ms);
		}
		//bind job (edge of DAG)
		for (process_id = 0; process_id < proc_layer_num; process_id++) {

			//BIND JOB with linear order
			if (process_id < proc_layer_num-1) {
				vendor_ai_dla_bind_job(net_id, process_id, process_id+1);
			}
		}
		vendor_ai_dla_sum_job(net_id, &src_count, &dest_count);
		//DBG_DUMP("proc_id(%lu) summary: src=%d, dest=%d\r\n", net_id, int)src_count, (int)dest_count);
		p_proc->info.in_job_cnt = src_count;
		p_proc->info.out_job_cnt = dest_count;

	} else if (_vendor_ai_net_is_linear_job(order) && _vendor_ai_net_is_optimize_job(order) == TRUE) {
		//--- linear job with optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(net_id);
		VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_cfg->job_opt;

		if (p_proc->priv.b_simplify_bin == TRUE) {
			//<simplify model bin> mode (find those [HW layer whoes tot_trig_eng_times > 0] & [CPU layer], then bind/set job)
			INT32 bind_id[2] = {-1, -1};  // pre & next
			NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			NN_IOMEM *p_this_buffer = NULL;
			UINT32 engine_id;
			UINT32 engine_op;

			//add job (node of DAG)
			for (process_id = 0; process_id < proc_layer_num; process_id++) {
				if ((p_mctrl[process_id].tot_trig_eng_times > 0) || (p_mctrl[process_id].trig_src == NN_GEN_TRIG_COMMON)) {

					bind_id[0] = bind_id[1];
					bind_id[1] = (INT32)process_id;

					if (bind_id[0] >= 0) {
						//this
						p_this_mctrl = p_mctrl + (UINT32)bind_id[0];
#if CNN_25_MATLAB
						p_this_buffer = p_io_mem + p_this_mctrl->layer_index;
#else
						p_this_buffer = (NN_IOMEM *)&p_this_mctrl->iomem;
#endif

						//SET JOB
						_vendor_ai_job_select_eng(process_id, p_this_mctrl, &engine_id, &engine_op);
						vendor_ai_dla_set_job(net_id, (UINT32)bind_id[0], engine_id, engine_op, p_job_opt->schd_parm, (void*)p_this_mctrl, (void*)p_this_buffer, wait_ms);
					}
				}
			}
			if (bind_id[1] >= 0) { // last job
				//this
				p_this_mctrl = p_mctrl + (UINT32)bind_id[1];
#if CNN_25_MATLAB
				p_this_buffer = p_io_mem + p_this_mctrl->layer_index;
#else
				p_this_buffer = (NN_IOMEM *)&p_this_mctrl->iomem;
#endif
				//SET JOB
				_vendor_ai_job_select_eng(process_id, p_this_mctrl, &engine_id, &engine_op);
				vendor_ai_dla_set_job(net_id, (UINT32)bind_id[1], engine_id, engine_op, p_job_opt->schd_parm, (void*)p_this_mctrl, (void*)p_this_buffer, wait_ms);
			}

			//bind job (edge of DAG)
			bind_id[0] = -1;
			bind_id[1] = -1;
			for (process_id = 0; process_id < proc_layer_num; process_id++) {
				if ((p_mctrl[process_id].tot_trig_eng_times > 0) || (p_mctrl[process_id].trig_src == NN_GEN_TRIG_COMMON)) {

					bind_id[0] = bind_id[1];
					bind_id[1] = (INT32)process_id;

					if (bind_id[0] >= 0) {
						//this
						p_this_mctrl = p_mctrl + (UINT32)bind_id[0];
#if CNN_25_MATLAB
						p_this_buffer = p_io_mem + p_this_mctrl->layer_index;
#else
						p_this_buffer = (NN_IOMEM *)&p_this_mctrl->iomem;
#endif
						//BIND JOB
						vendor_ai_dla_bind_job(net_id, (UINT32)bind_id[0], (UINT32)bind_id[1]);
					}
				}
			}

		} else {
			//<normal> mode
			VENDOR_AI_NET_LLGROUP_LIST *p_group_list = {0};
			UINT32 j;

			p_group_list = vendor_ai_net_group_getgroupresult(net_id);
			if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
				DBG_ERR("null p_group_list\r\n");
				return HD_ERR_INV;
			}

			//add job (node of DAG)
			for (j = 0; j < p_group_list->group_num; j++) {

				NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
				NN_IOMEM *p_this_buffer = NULL;
				UINT32 engine_id;
				UINT32 engine_op;

				//this
				p_this_mctrl = p_group_list->p_group[j].p_head;
				process_id = p_this_mctrl - p_mctrl;
#if CNN_25_MATLAB
				p_this_buffer = p_io_mem + p_this_mctrl->layer_index;
#else
				p_this_buffer = (NN_IOMEM *)&p_this_mctrl->iomem;
#endif

				//SET JOB
				_vendor_ai_job_select_eng(process_id, p_this_mctrl, &engine_id, &engine_op);
				vendor_ai_dla_set_job(net_id, process_id, engine_id, engine_op, p_job_opt->schd_parm, (void*)p_this_mctrl, (void*)p_this_buffer, wait_ms);

			}
			//bind job (edge of DAG)
			for (j = 0; j < p_group_list->group_num; j++) {

				NN_GEN_MODE_CTRL *p_tail_mctrl = NULL;
				NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
				UINT32 tail_process_id;
				UINT32 next_process_id;

				//this
				p_this_mctrl = p_group_list->p_group[j].p_head;
				process_id = p_this_mctrl - p_mctrl;

				//BIND JOB with linear order
				p_tail_mctrl = p_group_list->p_group[j].p_tail;
				tail_process_id = p_tail_mctrl - p_mctrl;
				if (j < (p_group_list->group_num-1)) {
					next_process_id = tail_process_id + 1;
					vendor_ai_dla_bind_job(net_id, process_id, next_process_id);
				}
			}
		}
		vendor_ai_dla_sum_job(net_id, &src_count, &dest_count);
		//DBG_DUMP("proc_id(%lu) summary: src=%d, dest=%d\r\n", net_id, int)src_count, (int)dest_count);
		p_proc->info.in_job_cnt = src_count;
		p_proc->info.out_job_cnt = dest_count;

	} else if (_vendor_ai_net_is_graph_job(order) && _vendor_ai_net_is_optimize_job(order) == FALSE) {
		//--- graph job w/o optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(net_id);
		VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_cfg->job_opt;

		//add job (node of DAG)
		for (process_id = 0; process_id < proc_layer_num; process_id++) {

			NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			NN_IOMEM *p_this_buffer = NULL;
			UINT32 engine_id;
			UINT32 engine_op;

			//this
			p_this_mctrl = p_mctrl + process_id;
#if CNN_25_MATLAB
			p_this_buffer = p_io_mem + p_this_mctrl->layer_index;
#else
			p_this_buffer = (NN_IOMEM *)&p_this_mctrl->iomem;
#endif

			//SET JOB
			_vendor_ai_job_select_eng(process_id, p_this_mctrl, &engine_id, &engine_op);
			vendor_ai_dla_set_job(net_id, process_id, engine_id, engine_op, p_job_opt->schd_parm, (void*)p_this_mctrl, (void*)p_this_buffer, wait_ms);
		}
		//bind job (edge of DAG)
		for (process_id = 0; process_id < proc_layer_num; process_id++) {

			NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			UINT32* next_process_list = NULL;
			UINT32 i;

			//this
			p_this_mctrl = p_mctrl + process_id;
			//BIND JOB with graph order
			next_process_list = (UINT32 *)(((char*)net_info.p_id_list) + p_this_mctrl->next_layer_idx_addr);
			for(i = 0; i < p_this_mctrl->next_num; i++) {
				UINT32 next_process_id = next_process_list[i];
				vendor_ai_dla_bind_job(net_id, process_id, next_process_id);
			}
		}
		vendor_ai_dla_sum_job(net_id, &src_count, &dest_count);
		//DBG_DUMP("proc_id(%lu) summary: src=%d, dest=%d\r\n", net_id, int)src_count, (int)dest_count);
		p_proc->info.in_job_cnt = src_count;
		p_proc->info.out_job_cnt = dest_count;

	} else if (_vendor_ai_net_is_graph_job(order) && _vendor_ai_net_is_optimize_job(order) == TRUE) {
		//--- graph job with optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(net_id);
		VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_cfg->job_opt;
		VENDOR_AI_NET_LLGROUP_LIST *p_group_list = {0};
		UINT32 j;

		p_group_list = vendor_ai_net_group_getgroupresult(net_id);
		if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
			DBG_ERR("null p_group_list\r\n");
			return HD_ERR_INV;
		}

		//add job (node of DAG)
		for (j = 0; j < p_group_list->group_num; j++) {

			NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			NN_IOMEM *p_this_buffer = NULL;
			UINT32 engine_id;
			UINT32 engine_op;

			//this
			p_this_mctrl = p_group_list->p_group[j].p_head;
			process_id = p_this_mctrl - p_mctrl;
#if CNN_25_MATLAB
			p_this_buffer = p_io_mem + p_this_mctrl->layer_index;
#else
			p_this_buffer = (NN_IOMEM *)&p_this_mctrl->iomem;
#endif

			//SET JOB
			_vendor_ai_job_select_eng(process_id, p_this_mctrl, &engine_id, &engine_op);
			vendor_ai_dla_set_job(net_id, process_id, engine_id, engine_op, p_job_opt->schd_parm, (void*)p_this_mctrl, (void*)p_this_buffer, wait_ms);

		}
		//bind job (edge of DAG)
		for (j = 0; j < p_group_list->group_num; j++) {

			NN_GEN_MODE_CTRL *p_tail_mctrl = NULL;
			NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			UINT32* next_process_list = NULL;
			UINT32 i;

			//this
			p_this_mctrl = p_group_list->p_group[j].p_head;
			process_id = p_this_mctrl - p_mctrl;

			//BIND JOB with graph order
			p_tail_mctrl = p_group_list->p_group[j].p_tail;
			next_process_list = (UINT32 *)(((char*)net_info.p_id_list) + p_tail_mctrl->next_layer_idx_addr);
			for(i = 0; i < p_tail_mctrl->next_num; i++) {
				UINT32 next_process_id = next_process_list[i];
				vendor_ai_dla_bind_job(net_id, process_id, next_process_id);
			}
		}
		vendor_ai_dla_sum_job(net_id, &src_count, &dest_count);
		//DBG_DUMP("proc_id(%lu) summary: src=%d, dest=%d\r\n", net_id, int)src_count, (int)dest_count);
		p_proc->info.in_job_cnt = src_count;
		p_proc->info.out_job_cnt = dest_count;

	} else {

		DBG_ERR("proc_id(%lu) not support job-opt %lu\r\n", net_id, order);
		return HD_ERR_FAIL;
	}
	vendor_ai_net_trace(net_id, AI_JOB, "start() - bind_job - end\r\n");
	/////////////////////////////////////////////////////////////////////////////

	return HD_OK;
}

HD_RESULT vendor_ais_stop_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id)
{
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};

	er = vendor_ais_get_net_info(&net_info, net_mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

//CLOSE
	vendor_ai_net_trace(net_id, AI_FLOW, "stop() - destory_joblist\r\n");
	vendor_ai_dla_destory_joblist(net_id);
	//vendor_ai_net_trace(net_id, AI_FLOW, "stop() - job_callback_thread_exit\r\n");
	//_vendor_ai_job_callback_thread_exit(net_id); //job done callback
	vendor_ai_net_trace(net_id, AI_FLOW, "stop() - cpu_callback_thread_exit\r\n");
	vendor_ai_cpu_thread_exit(net_id); //cpu exec callback
	vendor_ai_net_trace(net_id, AI_FLOW, "stop() - dsp_callback_thread_exit\r\n");
	vendor_ai_dsp_thread_exit(net_id); //dsp exec callback

	return HD_OK;
}

HD_RESULT vendor_ais_push_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id, UINT32 order)
{
	HD_RESULT r;
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};

	//net
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;

	//op context
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 process_id;

	UINT64 ts = 0, ts_new = 0;
	UINT64 ts_enqueue = 0;
	char order_str[16];
#if CNN_MULTI_INPUT
	//NDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(net_id);
	UINT32 schd_parm = p_cfg->job_opt.schd_parm;
#endif
	//UINT32 end_process_id;
//OPEN
	er = vendor_ais_get_net_info(&net_info, net_mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	p_head = net_info.p_head;
	proc_layer_num = p_head->mode_ctrl_num;

	p_mctrl = net_info.p_mctrl;

	vendor_ais_proc_net_dump_group(net_id);
	//_vendor_ai_net_debug_dump(0, 1, 2, "/mnt/sd/aiparm.txt");


	_cvt_order_name(order, order_str, 16);
    /////////////////////////////////////////////////////////////////////////////
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts = hd_gettime_us();
	}

	vendor_ai_net_trace(net_id, AI_FLOW, "proc() - push (order=%s) - begin \r\n", order_str);

#if CNN_MULTI_INPUT
	// check multi-input resouce is ready 	
	r = _vendor_ais_net_check_multi_input(net_id);
	if(r != HD_OK) goto quit_1;
#endif

	if (_vendor_ai_net_is_linear_job(order) && _vendor_ai_net_is_optimize_job(order) == FALSE) {
		//--- linear job  w/o optimize ---
        //PUT JOB
		r = vendor_ai_dla_lock_job(net_id, 0);
		if (r != HD_OK) goto quit_1;
		for (process_id = 0; process_id < proc_layer_num; process_id ++) {
			r = vendor_ai_dla_push_job(net_id, process_id, schd_parm);
			if (r != HD_OK) goto quit_1;
		}
		r = vendor_ai_dla_unlock_job(net_id, 0); //start
		if (r != HD_OK) goto quit_1;
	} else if (_vendor_ai_net_is_linear_job(order) && _vendor_ai_net_is_optimize_job(order) == TRUE) {
		//--- linear job  with optimize ---
		VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
		if (p_proc->priv.b_simplify_bin == TRUE) {
			//<simplify model bin> mode (find those [HW layer whoes tot_trig_eng_times > 0] & [CPU layer], then push job)
			INT32 bind_id[2] = {-1, -1};  // pre & next
	        //PUT JOB
			r = vendor_ai_dla_lock_job(net_id, 0);
			if (r != HD_OK) goto quit_1;
			for (process_id = 0; process_id < proc_layer_num; process_id++) {
				if ((p_mctrl[process_id].tot_trig_eng_times > 0) || (p_mctrl[process_id].trig_src == NN_GEN_TRIG_COMMON)) {
					bind_id[0] = bind_id[1];
					bind_id[1] = (INT32)process_id;

					if (bind_id[0] >= 0) {
						r = vendor_ai_dla_push_job(net_id, (UINT32)bind_id[0], schd_parm);
						if (r != HD_OK) goto quit_1;
					}
				}
			}
			// last job
			if (bind_id[1] >= 0) {
				r = vendor_ai_dla_push_job(net_id, (UINT32)bind_id[1], schd_parm);
				if (r != HD_OK) goto quit_1;
			}
			r = vendor_ai_dla_unlock_job(net_id, 0); //start
			if (r != HD_OK) goto quit_1;
		}else {
			//<normal> mode
			VENDOR_AI_NET_LLGROUP_LIST *p_group_list = {0};
			UINT32 j;

			p_group_list = vendor_ai_net_group_getgroupresult(net_id);
			if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
				DBG_ERR("null p_group_list\r\n");
				return HD_ERR_INV;
			}

	        //PUT JOB
			r = vendor_ai_dla_lock_job(net_id, 0);
			if (r != HD_OK) goto quit_1;
			for (j = 0; j < p_group_list->group_num; j++) {

			    NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
				p_this_mctrl = p_group_list->p_group[j].p_head;
				process_id = p_this_mctrl - p_mctrl;
				r = vendor_ai_dla_push_job(net_id, process_id, schd_parm);
				if (r != HD_OK) goto quit_1;
			}
			r = vendor_ai_dla_unlock_job(net_id, 0); //start
			if (r != HD_OK) goto quit_1;
		}

	} else if (_vendor_ai_net_is_graph_job(order) && _vendor_ai_net_is_optimize_job(order) == FALSE) {
		//--- graph job w/o optimize ---
        //PUT JOB
		r = vendor_ai_dla_lock_job(net_id, 0);
		if (r != HD_OK) goto quit_1;
		for (process_id = 0; process_id < proc_layer_num; process_id ++) {
			r = vendor_ai_dla_push_job(net_id, process_id, schd_parm);
			if (r != HD_OK) goto quit_1;
		}
		r = vendor_ai_dla_unlock_job(net_id, 0); //start
		if (r != HD_OK) goto quit_1;

    } else if (_vendor_ai_net_is_graph_job(order) && _vendor_ai_net_is_optimize_job(order) == TRUE) {
		//--- graph job with optimize ---
		VENDOR_AI_NET_LLGROUP_LIST *p_group_list = {0};
		UINT32 j;

		p_group_list = vendor_ai_net_group_getgroupresult(net_id);
		if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
			DBG_ERR("null p_group_list\r\n");
			return HD_ERR_INV;
		}

        //PUT JOB
		r = vendor_ai_dla_lock_job(net_id, 0);
		if (r != HD_OK) goto quit_1;
		for (j = 0; j < p_group_list->group_num; j++) {

		    NN_GEN_MODE_CTRL *p_this_mctrl = NULL;
			p_this_mctrl = p_group_list->p_group[j].p_head;
			process_id = p_this_mctrl - p_mctrl;
			r = vendor_ai_dla_push_job(net_id, process_id, schd_parm);
			if (r != HD_OK) goto quit_1;
		}
		r = vendor_ai_dla_unlock_job(net_id, 0); //start
		if (r != HD_OK) goto quit_1;

    } else {

		DBG_ERR("proc_id(%lu) not support job-opt %lu\r\n", net_id, order);
		return HD_ERR_FAIL;
    }

	/////////////////////////////////////////////////////////////////////////////
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
    	ts_new = hd_gettime_us();
		ts_enqueue = ts_new-ts;
		vendor_ai_net_trace(net_id, AI_FLOW, "proc() - push_job - end \r\n");

		vendor_ai_net_trace(net_id, AI_JOB|AI_PERF, "proc() - >>> push job time = %llu\r\n", ts_enqueue );
	}

	return HD_OK;

quit_1:
	DBG_ERR("proc_id(%lu) net push fail=%d\r\n", net_id, (int)r);
	return r;
}


HD_RESULT vendor_ais_pull_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id, INT32 wait_ms)
{
	HD_RESULT r = HD_OK;

	UINT64 ts = 0, ts_new = 0;
	UINT64 ts_dequeue = 0;
	//char order_str[16];

	UINT32 end_process_id;

	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
    	ts = hd_gettime_us();
	}
	vendor_ai_net_trace(net_id, AI_FLOW, "proc() - pull (sync=%d) - begin \r\n", (int)wait_ms);
	vendor_ai_net_trace(net_id, AI_FLOW, "proc() - wait_job - begin \r\n");
	end_process_id = 0;
	while(r != HD_ERR_EOL) {
		r = vendor_ai_wait_job_graph(net_id, &end_process_id);
		if ((r != HD_OK) && (r != HD_ERR_EOL)) {
			//DBG_ERR("proc_id(%lu) wait job fail=%d\r\n", net_id, (int)r);
			goto quit_2;
		}
		vendor_ai_net_trace(net_id, AI_FLOW, "proc() - wait_job - job[%d] ok \r\n", (int)end_process_id);
	}
	vendor_ai_net_trace(net_id, AI_FLOW, "proc() - wait_job - end \r\n");

	/////////////////////////////////////////////////////////////////////////////
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_dequeue = ts_new-ts;

		vendor_ai_net_trace(net_id, AI_JOB|AI_PERF, "proc() - >>> pull job time = %llu\r\n", ts_dequeue);
	}
	return HD_OK;

quit_2:
	DBG_ERR("proc_id(%lu) net pull fail=%d\r\n", net_id, (int)r);
	return r;
}

HD_RESULT vendor_ais_proc_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id, UINT32 order, INT32 wait_ms)
{
	HD_RESULT r;
	r = vendor_ais_push_net (net_mem, net_id, order);
	if (r != HD_OK)
		return r;

	r = vendor_ais_pull_net (net_mem, net_id, wait_ms);
	return r;
}

#else

HD_RESULT vendor_ais_proc_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, VENDOR_AIS_FLOW_MEM_PARM rslt_mem, UINT32 net_id)
{
	CHAR img_file_path[STR_MAX_LENGTH], img_folder_path[STR_MAX_LENGTH];
	CHAR t_cmd[STR_MAX_LENGTH];
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_IOMEM *p_io_mem;
	VENDOR_AI_TRIGGER_PARAM trig_parm = {0};
	//NN_PRE_PARM *p_pre_pram;
	//VENDOR_AIS_PRE_BUFF pre_buff = {0};
	BOOL is_output = TRUE;
	UINT32 idx = 0;

	// Start network flow
	UINT32 process_index = 0, layer_index = 0;
	NN_GEN_ENG_TYPE ctrl_type;
	VENDOR_AI_ENG engine = 0;

	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;

	er = vendor_ais_get_net_info(&net_info, net_mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	p_io_mem = net_info.p_io_mem;
	proc_layer_num = p_head->mode_ctrl_num;

#if (0)
	DBG_DUMP("\nSTART PROCESSING ...\r\n");
#endif

	trig_parm.is_nonblock = FALSE;
	trig_parm.time_out_ms = ENG_TIME_OUT;


	if ((p_io_mem->SAI[0].pa == 0) || (p_io_mem->SAI[0].va == 0) || (p_io_mem->SAI[0].size == 0)) {
		DBG_ERR("first layer memory(pa=0x%08x, va=0x%08x, size=%d) should be init before nn process\r\n"
				, (int)p_io_mem->SAI[0].pa, (int)p_io_mem->SAI[0].va, (int)p_io_mem->SAI[0].size);
		return HD_ERR_ABORT;
	}

#if 0
	usleep(TEST_POWER_SLEEP_TM << 1);
#endif

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		PROF_START();
#if NN_RPOC_KERL_SWITCH
		if ((p_mctrl[process_index].trig_src == NN_GEN_TRIG_APP_AI_DRV) || (p_mctrl[process_index].trig_src == NN_GEN_TRIG_LL_AI_DRV)) {
			VENDOR_AIS_FLOW_PROC_INFO proc_info;
			VENDOR_AIS_FLOW_PROC_PARM *p_proc_parm = &proc_info.parm;
			p_proc_parm->is_nonblock	= trig_parm.is_nonblock;
			p_proc_parm->time_out_ms	= trig_parm.time_out_ms;
			p_proc_parm->net_addr		= net_mem.va;
			p_proc_parm->start_layer	= process_index;
			p_proc_parm->end_layer		= proc_layer_num;
			proc_info.net_id 			= net_id;

			if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id], VENDOR_AIS_FLOW_IOC_SWITCH_PROC, &proc_info) < 0) {
				fprintf(stderr, "VENDOR_AIS_FLOW_IOC_SWITCH_PROC ioctl failed\r\n");
				vendor_ai_errno_location();
				continue;
			}

			if (p_proc_parm->start_layer >= proc_layer_num) {
				break;
			} else {
				process_index = p_proc_parm->start_layer;
			}
		}
#endif
		layer_index = p_mctrl[process_index].layer_index;
		if (p_mctrl[process_index].eng == NN_GEN_ENG_CNN) {
			engine = AI_ENG_CNN;
		} else if (p_mctrl[process_index].eng == NN_GEN_ENG_CNN2) {
			engine = AI_ENG_CNN2;
		} else if (p_mctrl[process_index].eng == NN_GEN_ENG_NUE) {
			engine = AI_ENG_NUE;
        } else if (p_mctrl[process_index].eng == NN_GEN_ENG_NUE2) {
			engine = AI_ENG_NUE2;
		} else {
			engine = AI_ENG_UNKNOWN;
		}

#if (0)
		DBG_DUMP("--------Process: %d--------\r\n", (int)(process_index));
		DBG_DUMP("Layer: %d\r\n", (int)layer_index);
		DBG_DUMP("trig_src = %d\r\n", p_mctrl[process_index].trig_src);
		DBG_DUMP("mode = %d\r\n", (NN_MODE)p_mctrl[process_index].mode);
#endif

		ctrl_type = p_mctrl[process_index].eng;

		if (ctrl_type == NN_GEN_ENG_CPU) {
			for (idx = 0; idx < NN_IMEM_NUM; idx++) {
				if ((p_io_mem[layer_index].SAI[idx].va == 0) || (p_io_mem[layer_index].SAI[idx].size == 0)) {
					continue;
				}
				//hd_common_mem_flush_cache((VOID *)p_io_mem[layer_index].SAI[idx].va, p_io_mem[layer_index].SAI[idx].size);
				vendor_common_mem_cache_sync((VOID *)p_io_mem[layer_index].SAI[idx].va, p_io_mem[layer_index].SAI[idx].size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE); ///< cache invalidate - input from engine's output
			}
		}

#if 0
		usleep(TEST_POWER_SLEEP_TM);
#endif

		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			if (p_mctrl[process_index].tot_trig_eng_times) {
				AI_DRV_MODEINFO modeinfo = { AI_TRIG_MODE_APP, engine, net_id };
				VENDOR_AI_APP_INFO info = { (VENDOR_AI_APP_HEAD *)(p_mctrl[process_index].addr), p_mctrl[process_index].tot_trig_eng_times };
				AI_DRV_APPINFO drvinfo = { (VENDOR_AI_APP_INFO)info, engine, net_id };
				AI_DRV_TRIGINFO triginfo = { (VENDOR_AI_TRIGGER_PARAM)trig_parm, engine, net_id };

				vendor_ai_drv_open(engine, net_id);
				vendor_ai_drv_set_param(VENDOR_AI_PARAM_MODE_INFO, &modeinfo);
				vendor_ai_drv_set_param(VENDOR_AI_PARAM_APP_INFO, &drvinfo);
				vendor_ai_drv_trigger(&triginfo);
				vendor_ai_drv_close(engine, net_id);

				// goto last trigger engine step
				if (p_mctrl[process_index].tot_trig_eng_times > 1) {
					process_index += (p_mctrl[process_index].tot_trig_eng_times - 1);
					layer_index = p_mctrl[process_index].layer_index;
				}
			}
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			if (p_mctrl[process_index].tot_trig_eng_times) {
				AI_DRV_MODEINFO modeinfo = { AI_TRIG_MODE_LL, engine, net_id };
				VENDOR_AI_LL_INFO info = { (VENDOR_AI_LL_HEAD *)(p_mctrl[process_index].addr), p_mctrl[process_index].tot_trig_eng_times };
				AI_DRV_LLINFO drvinfo = { info, engine, net_id };
				AI_DRV_TRIGINFO triginfo = { trig_parm, engine, net_id };
#if 0 //LL_SUPPORT_ROI
				AI_DRV_LL_USR_INFO* ll_usr_info = (AI_DRV_LL_USR_INFO*)g_ll_info_addr[net_id];
				//PROF_START();
				if (ll_usr_info == NULL) {
					//DBG_ERR("ll_usr_info is NULL...\r\n");
				} else {
					ll_usr_info->layer_info.total_layer_num = p_mctrl[process_index].tot_trig_eng_times;
					ll_usr_info->net_id = net_id;
					vendor_ai_drv_set_link_info(ll_usr_info);
				}
				//PROF_END("set_link_info");
#endif

				vendor_ai_drv_open(engine, net_id);
				vendor_ai_drv_set_param(VENDOR_AI_PARAM_MODE_INFO, &modeinfo);
				vendor_ai_drv_set_param(VENDOR_AI_PARAM_LL_INFO, &drvinfo);
				vendor_ai_drv_trigger(&triginfo);
				if (trig_parm.is_nonblock) {
					vendor_ai_drv_waitdone(&triginfo);
				}
				vendor_ai_drv_close(engine, net_id);

				// goto linked list last trigger engine step
				if (p_mctrl[process_index].tot_trig_eng_times > 1) {
					process_index += (p_mctrl[process_index].tot_trig_eng_times - 1);
					layer_index = p_mctrl[process_index].layer_index;
				}
			}
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			if (g_ext_cb_fp[0]) {
				HD_RESULT rv = (g_ext_cb_fp[0])((UINT32)net_id, process_index, (UINT32)p_mctrl[process_index].mode, (UINT32)p_mctrl[process_index], (UINT32)p_mctrl[process_index].addr);
				if (rv != HD_OK ) return rv;
			} else {
				DBG_ERR("FATAL ERROR : user doesn't set VENDOR_AI_CFG_ENGINE_PLUGIN for this layer !! proc layer failed !!\r\n");
				return HD_ERR_USER; // this net has sw layer, but user doesn't register callback !!
			}

			break;
		}

		if (ctrl_type == NN_GEN_ENG_CPU) {
			for (idx = 0; idx < NN_OMEM_NUM; idx++) {
				if ((p_io_mem[layer_index].SAO[idx].va == 0) || (p_io_mem[layer_index].SAO[idx].size == 0)) {
					continue;
				}
				//hd_common_mem_flush_cache((VOID *)p_io_mem[layer_index].SAO[idx].va, p_io_mem[layer_index].SAO[idx].size);
				vendor_common_mem_cache_sync((VOID *)p_io_mem[layer_index].SAO[idx].va, p_io_mem[layer_index].SAO[idx].size, VENDOR_COMMON_MEM_DMA_TO_DEVICE); ///< cache clean - output to engine's input
			}
		}

		for (idx = (process_index + 1); idx < proc_layer_num; idx++) {
			if (p_mctrl[process_index].layer_index == p_mctrl[idx].layer_index) {
				is_output = FALSE;
				break;
			} else {
				is_output = TRUE;
			}
		}
#if (!NN_DUMP_LAYER)
		is_output = FALSE;
#endif

#if PROF
		is_output = FALSE;
		snprintf(t_cmd, STR_MAX_LENGTH, "%s, proc idx = %d,", __func__, (int)process_index);
		PROF_END(t_cmd);
		if (is_output) {
#else
		if (is_output) {
			snprintf(img_folder_path, STR_MAX_LENGTH, "%s%d", OUT_PATH, (int)net_id);
			snprintf(t_cmd, STR_MAX_LENGTH, "mkdir --parents %s", img_folder_path);
			system(t_cmd);
#endif

#if 0

			for (idx = 0; idx < NN_IMEM_NUM; idx++) {
				const UINT32 dump_size 	= p_io_mem[layer_index].SAI[idx].size;
				const UINT32 dump_addr 	= p_io_mem[layer_index].SAI[idx].va;
				if (dump_size == 0) {
					continue;
				}

				switch (ctrl_type) {
				case NN_GEN_ENG_CNN:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/CNN%d_IN%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				case NN_GEN_ENG_NUE:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/NUE%d_IN%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				case NN_GEN_ENG_CPU:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/CPU%d_IN%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				case NN_GEN_ENG_VPE:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/VPE%d_IN%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				default:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/NAN%d_IN%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				}
				DBG_DUMP("img_file_path : %s; addr=%08x; size=%d\n", img_file_path, (unsigned int)dump_addr, (unsigned int)dump_size);
				vendor_ais_write_file(img_file_path, dump_addr, dump_size);
			}

#endif

			for (idx = 0; idx < NN_OMEM_NUM; idx++) {
				const UINT32 dump_size 	= p_io_mem[layer_index].SAO[idx].size;
				const UINT32 dump_addr	= p_io_mem[layer_index].SAO[idx].va;
				if (dump_size == 0) {
					continue;
				}

				switch (ctrl_type) {
				case NN_GEN_ENG_CNN:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/CNN_%d_OUT%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
                case NN_GEN_ENG_CNN2:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/CNN2_%d_OUT%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				case NN_GEN_ENG_NUE:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/NUE_%d_OUT%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
                case NN_GEN_ENG_NUE2:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/NUE2_%d_OUT%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				case NN_GEN_ENG_CPU:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/CPU_%d_OUT%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				case NN_GEN_ENG_VPE:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/VPE_%d_OUT%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				default:
					snprintf(img_file_path, STR_MAX_LENGTH, "%s/NAN_%d_OUT%d.bin", img_folder_path, (int)process_index, (int)idx);
					break;
				}
				DBG_DUMP("img_file_path : %s; addr=0x%08x; size=%d\n", img_file_path, (unsigned int)dump_addr, (unsigned int)dump_size);
				vendor_ais_write_file(img_file_path, dump_addr, dump_size);
			}
		}
	}

#if (0)
	DBG_DUMP("\nEND PROCESSING ...\n");
#endif

	return HD_OK;
}
#endif

HD_RESULT vendor_ais_update_external(VENDOR_AI_DIFF_MODEL_RESINFO *p_resinfo, UINT32 proc_id, UINT32 mode)
{
    VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	HD_RESULT rv = HD_OK;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 dst_tag = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));
    UINT32 dst_tag2 = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('2')<<24));
	UINT32 num_output_layer = 0, tmp_size = 0, p_diff_output_id = 0, p_diff_output_layer_num = 0, output_info_size = 0;
	NN_DIFF_OUTPUT_PARM* p_diff_output_parm = NULL;
    UINT64* p_output_info_size = NULL;
    NN_IN_OUT_BUF_INFO2* ext_info2 = NULL;
    
    p_output_info_size = (UINT64* )((uintptr_t)p_resinfo->output_info.va);
    output_info_size = *p_output_info_size;
    p_diff_output_layer_num = output_info_size/sizeof(NN_DIFF_OUTPUT_PARM);
    if (mode == 0) {
        p_diff_output_parm = (NN_DIFF_OUTPUT_PARM*)((uintptr_t)p_resinfo->output_info.va + sizeof(UINT64) + (uintptr_t)(p_resinfo->curr_id*output_info_size));
    } else {
        p_diff_output_parm = (NN_DIFF_OUTPUT_PARM*)((uintptr_t)p_resinfo->output_info.va + sizeof(UINT64));
    }
	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_mctrl = net_info.p_mctrl;

	// get layer info
	p_head = net_info.p_head;

	

	// get layer info
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

    // try to find dst_tag2 first
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((uintptr_t)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag2) { // bit1 represent tag_type
			ext_info2 = (NN_IN_OUT_BUF_INFO2 *)(((uintptr_t)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info2 == NULL) {
				DBG_ERR("ext_info2 pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

    // original dst_tag
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((uintptr_t)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			NN_IN_OUT_BUF_INFO* ext_info = (NN_IN_OUT_BUF_INFO *)(((uintptr_t)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info == NULL) {
				DBG_ERR("ext_info pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			num_output_layer = (p_tmp_head[0] - 2*sizeof(UINT32)) / sizeof(NN_IN_OUT_BUF_INFO); // number of external info

			for(UINT32 i = 0; i < num_output_layer; i++) {
				UINT32 buf_attr, mctrl_id;
				//port_id;
				// uintptr_t imem_addr, omem_addr;

				// parse ext_id
				buf_attr = (ext_info[i].ext_id & 0xf0000000) >> 28;   // 0x9: input buffer, 0xa: output buffer
				//port_id = (ext_info[i].ext_id & 0xfff0000) >> 16;
				mctrl_id = (ext_info[i].ext_id & 0xffff);

				// debug
				/*printf("buf_attr(0x%lx) port_id(0x%lx) mctrl_id(0x%lx) ext_info_num(%u)\n", buf_attr, port_id, mctrl_id, ext_info_num);
				printf("========= i(%d) ==============\n", (int)i);
				printf("layer_name = %s\n", ext_info[i].layer_name);
				printf("layer_type = %s\n", ext_info[i].layer_type);
				printf("caffe_layer_index = %d\n", ext_info[i].caffe_layer_index);
				printf("fusion_layer_index = %d\n", ext_info[i].fusion_layer_index);
				printf("out_bitdepth = %d\n", ext_info[i].out_bitdepth);
				printf("out_sign_bit_num = %d\n", ext_info[i].out_sign_bit_num);
				printf("out_int_bit_num = %d\n", ext_info[i].out_int_bit_num);
				printf("out_frac_bit_num = %d\n", ext_info[i].out_frac_bit_num);
				printf("out_scale_ratio = %f\n", ext_info[i].out_scale_ratio);
				printf("width = %d\n", ext_info[i].width);
				printf("height = %d\n", ext_info[i].height);
				printf("channel = %d\n", ext_info[i].channel);
				printf("batch = %d\n", ext_info[i].batch);
				printf("time = %d\n", ext_info[i].time);
				printf("out_lofs = %d\n", ext_info[i].out_lofs);
				printf("out_ch_ofs = %d\n", ext_info[i].out_ch_ofs);
				printf("out_batch_ofs  = %u\n", ext_info[i].out_batch_ofs);
				printf("out_time_ofs = %u\n", ext_info[i].out_time_ofs);
				printf("ext_id(0x%lx)\n", ext_info[i].ext_id);
				printf("dataorder = %s\n", ext_info[i].data_order);
				printf("\n\n");*/

				// check va fixing
				
				// omem_addr = p_mctrl[mctrl_id].iomem.omem_addr;
				// if (omem_addr < p_mem->va) omem_addr += p_mem->va; // if not fix yet(call this funciton before gen_init(), fix it

				// config to NN_DATA
				if (buf_attr == 0xa && (UINT32)OUT_BUF_ATTR_GET(&p_mctrl[mctrl_id], NN_OUT_BUF_ATTR_PRESERVE)) {
                    ext_info[i].width = p_diff_output_parm[p_diff_output_id].width ; 
                    ext_info[i].height = p_diff_output_parm[p_diff_output_id].height;
                    ext_info[i].channel = p_diff_output_parm[p_diff_output_id].channel;
                    //ext_info[i].batch = p_diff_output_parm[p_diff_output_id].batch;
                    ext_info[i].out_lofs = p_diff_output_parm[p_diff_output_id].out_ofs.line_ofs;
                    ext_info[i].out_ch_ofs = p_diff_output_parm[p_diff_output_id].out_ofs.channel_ofs;
                    ext_info[i].out_batch_ofs = p_diff_output_parm[p_diff_output_id].out_ofs.batch_ofs;
                    if(ext_info2) {
                        ext_info2[i].width = p_diff_output_parm[p_diff_output_id].width ; 
                        ext_info2[i].height = p_diff_output_parm[p_diff_output_id].height;
                        ext_info2[i].channel = p_diff_output_parm[p_diff_output_id].channel;
                        //ext_info2[i].batch = p_diff_output_parm[p_diff_output_id].batch;
                        ext_info2[i].out_lofs = p_diff_output_parm[p_diff_output_id].out_ofs.line_ofs;
                        ext_info2[i].out_ch_ofs = p_diff_output_parm[p_diff_output_id].out_ofs.channel_ofs;
                    }
					if ( p_diff_output_id < p_diff_output_layer_num)
						p_diff_output_id += 1 ;

                    /*
					printf("width = %d\n", ext_info[i].width);
					printf("height = %d\n", ext_info[i].height);
					printf("channel = %d\n", ext_info[i].channel);
					printf("batch = %d\n", ext_info[i].batch);
					printf("time = %d\n", ext_info[i].time);
					printf("out_lofs = %d\n", ext_info[i].out_lofs);
					printf("out_ch_ofs = %d\n", ext_info[i].out_ch_ofs);
					printf("out_batch_ofs  = %u\n", ext_info[i].out_batch_ofs);
					printf("out_time_ofs = %u\n", ext_info[i].out_time_ofs);
                    */
					// config feature_in (only for oldtool)
					
				} 

			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

exit:
	return rv;

}

HD_RESULT vendor_ais_pars_diff_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo, UINT32 net_id)
{
	HD_RESULT hd_ret = HD_OK;
	VENDOR_AIS_FLOW_UPDATE_NET_INFO update_net_info = {0};

	PROF_START();
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - pars_diff_mem - begin \r\n");

	if (p_mem == NULL || p_diff_resinfo == NULL) {
		DBG_ERR("input pointer or addr is null...\r\n");
		return HD_ERR_ABORT;
	}
	if (net_id >= g_ai_support_net_max) {
		DBG_ERR("net_id is invalid...\r\n");
		return HD_ERR_ABORT;
	}
	if (p_diff_resinfo->diff_model.pa == 0) {
		DBG_ERR("diff mem addr is null...\r\n");
		return HD_ERR_ABORT;
	}

	hd_ret = vendor_ais_set_model_id(p_diff_resinfo);
	if (hd_ret == HD_ERR_ABORT) {
		return HD_ERR_ABORT;
	} else if (hd_ret != HD_OK) {
		DBG_ERR("find model id fail...\r\n");
		return HD_ERR_ABORT;
	}

    if(p_diff_resinfo->output_info.va != 0 && p_diff_resinfo->output_info.pa != 0) {
        hd_ret = vendor_ais_update_external(p_diff_resinfo, net_id, 0) ; 
        if (hd_ret != HD_OK) {
            return HD_ERR_ABORT;
        }
    }

	memcpy(&update_net_info.map_parm, p_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));

	hd_ret = vendor_ais_update_diff_iomem(p_mem, (NN_DIFF_MODEL_HEAD *)(p_diff_resinfo->diff_model.va), p_diff_resinfo, 0);
	if (hd_ret != HD_OK) {
		return HD_ERR_ABORT;
	}
	//vendor_common_mem_cache_sync((VOID *)p_diff_resinfo->diff_model.va, p_diff_resinfo->diff_model.size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE);  // IVOT_N12047_CO-363 : FROM_DEVICE is wrong!! should be TO_DEVICE. or just remove this cache flush (only CPU will touch this memory.. cache flush is NOT required. )

	update_net_info.net_info_pa = p_diff_resinfo->diff_model.pa;
	update_net_info.net_info_size = p_diff_resinfo->diff_model.size;
	update_net_info.model_id = p_diff_resinfo->curr_id;
	update_net_info.net_id = net_id;

	hd_ret = vendor_ais_update_diff_mem(&update_net_info);
	if (hd_ret != HD_OK) {
		return HD_ERR_ABORT;
	}
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - pars_diff_mem - net_id(%u), model_id(%u), pa(0x%lx), sz(0x%x)\r\n",
						update_net_info.net_id, update_net_info.model_id, update_net_info.net_info_pa, update_net_info.net_info_size);
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - pars_diff_mem - end \r\n");
	PROF_END("parsing_time");

	return HD_OK;
}

HD_RESULT vendor_ais_unpars_diff_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo, UINT32 net_id)
{
	HD_RESULT hd_ret = HD_OK;
	VENDOR_AIS_FLOW_UPDATE_NET_INFO update_net_info = {0};

	PROF_START();
	if (p_mem == NULL || p_diff_resinfo == NULL) {
		DBG_ERR("input pointer or addr is null...\r\n");
		return HD_ERR_ABORT;
	}
	if (net_id >= g_ai_support_net_max) {
		DBG_ERR("net_id is invalid...\r\n");
		return HD_ERR_ABORT;
	}
	if (p_diff_resinfo->diff_model.pa == 0) {
		DBG_ERR("diff mem addr is null...\r\n");
		return HD_ERR_ABORT;
	}
	
#if 0
	p_diff_resinfo->new_id = 0;
	hd_ret = vendor_ais_set_model_id(p_diff_resinfo);
	if (hd_ret == HD_ERR_ABORT) {
		return HD_ERR_ABORT;
	} else if (hd_ret != HD_OK) {
		DBG_ERR("find model id fail...\r\n");
		return HD_ERR_ABORT;
	}
#else
	/* We should use the same model_id for restoring,
	 * so here we directly use the previous curr_id,
	 * which is getting from "start stage". */
	//printf("%s:%d curr_id = %u\n", __func__, __LINE__, p_diff_resinfo->curr_id);
#endif

	if(p_diff_resinfo->output_info.va != 0 && p_diff_resinfo->output_info.pa != 0) {
        hd_ret = vendor_ais_update_external(p_diff_resinfo, net_id, 1) ; 
        if (hd_ret != HD_OK) {
            return HD_ERR_ABORT;
        }
    }
	
	memcpy(&update_net_info.map_parm, p_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));

	hd_ret = vendor_ais_update_diff_iomem(p_mem, (NN_DIFF_MODEL_HEAD *)(p_diff_resinfo->diff_model.va), p_diff_resinfo, 1);
	if (hd_ret != HD_OK) {
		return HD_ERR_ABORT;
	}

	update_net_info.net_info_pa = p_diff_resinfo->diff_model.pa;
	update_net_info.net_info_size = p_diff_resinfo->diff_model.size;
	update_net_info.model_id = p_diff_resinfo->curr_id;
	update_net_info.net_id = net_id;

	hd_ret = vendor_ais_restore_diff_mem(&update_net_info);
	if (hd_ret != HD_OK) {
		return HD_ERR_ABORT;
	}

	PROF_END("unparsing_time");

	return HD_OK;
}

BOOL vendor_ais_check_diff_batch_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem)
{
	VENDOR_AIS_FLOW_MEM_PARM mem = {0};
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	//NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 dst_tag = (UINT32)((UINT32)('B') | ((UINT32)('M')<<8) | ((UINT32)('I')<<16) | ((UINT32)('F')<<24));
	UINT32 tmp_size = 0;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	HD_RESULT er = HD_OK;


	if (p_mem->user_parm.va == 0) {
		DBG_ERR("input memory is 0...\r\n");
		return -1;
	}
	mem.va = p_mem->user_parm.va;
	mem.pa = p_mem->user_parm.pa;
	mem.size = p_mem->user_parm.size;
	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return 0;
	}
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(mem.va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((uintptr_t)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) {
			return 1;
		} else {
			if (p_tmp_head[0] == 0) {
				break;
			}
			tmp_size += p_tmp_head[0];
		}
	}
	return 0;
}

static HD_RESULT vendor_ais_get_diff_batch_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AIS_FLOW_MEM_PARM* p_diff_mem)
{
	VENDOR_AIS_FLOW_MEM_PARM mem = {0};
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	//NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 dst_tag = (UINT32)((UINT32)('B') | ((UINT32)('M')<<8) | ((UINT32)('I')<<16) | ((UINT32)('F')<<24));
	UINT32 tmp_size = 0;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	HD_RESULT er = HD_OK;


	if (p_mem->user_parm.va == 0) {
		DBG_ERR("input memory is 0...\r\n");
		return -1;
	}
	mem.va = p_mem->user_parm.va;
	mem.pa = p_mem->user_parm.pa;
	mem.size = p_mem->user_parm.size;
	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return -1;
	}
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(mem.va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) {
			p_diff_mem->va = ((UINT32)p_tmp_head) + 2*sizeof(UINT32);
			p_diff_mem->pa = p_diff_mem->va - p_mem->user_parm.va + p_mem->user_parm.pa;
			p_diff_mem->size = p_tmp_head[0];
			return 0;
		} else {
			if (p_tmp_head[0] == 0) {
				break;
			}
			tmp_size += p_tmp_head[0];
		}
	}
	DBG_ERR("get batch mem info fail...\r\n");

	return -1;
}

static HD_RESULT vendor_ais_update_diff_batch_mem(VENDOR_AIS_FLOW_UPDATE_NET_INFO *p_info)
{
	UINT32 net_id = p_info->net_id;

	if (vendor_ais_flow_fd[net_id+1] < 0) {
		DBG_ERR("vendor_ais_update_diff_batch_mem: ai kflow ioctl not init...\r\n");
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_PROC_UPDATE_BATCH_INFO, p_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_PROC_UPDATE_BATCH_INFO ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
}

static HD_RESULT vendor_ais_restore_diff_batch_mem(VENDOR_AIS_FLOW_UPDATE_NET_INFO *p_info)
{
	UINT32 net_id = p_info->net_id;

	if (vendor_ais_flow_fd[net_id+1] < 0) {
		DBG_ERR("vendor_ais_restore_diff_batch_mem: ai kflow ioctl not init...\r\n");
		return HD_ERR_ABORT;
	} else if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_RESTORE_UPDATE_BATCH_INFO, p_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_RESTORE_UPDATE_BATCH_INFO ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	} else {
		return HD_OK;
	}
}


static HD_RESULT vendor_ais_set_batch_model_id(VENDOR_AIS_DIFF_BATCH_MODEL_INFO *p_info, NN_DIFF_MODEL_HEAD *p_model_head)
{
	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_BATCH_HEAD* p_model_diff_head = NULL;
	UINT32 model_num = 0;
	UINT32 model_idx = 0;
	UINT32 fit_model_idx = 99999;

	if (p_info == NULL || p_model_head == NULL) {
		DBG_ERR("input model info is null...\r\n");
		return HD_ERR_ABORT;
	}

	model_num = p_model_head->stripe_model_num;
	//printf("model num = %d\n", (unsigned int)model_num);
	for (model_idx = 0; model_idx < model_num; model_idx++) {
		p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_idx*sizeof(NN_DIFF_MODE_CTRL));
		p_model_diff_head = (NN_DIFF_BATCH_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
		if (p_info->batch_num == p_model_diff_head->batch_id) {
			fit_model_idx = model_idx;
			break;
		}
	}

	if (fit_model_idx == 99999) {
		DBG_ERR("no suitable batch model id...\r\n");
		return HD_ERR_ABORT;
	} else {
		p_info->id = fit_model_idx;
	}


	return HD_OK;
}




static HD_RESULT vendor_ais_update_diff_batch_info(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, NN_DIFF_MODEL_HEAD *p_model_head, VENDOR_AIS_DIFF_BATCH_MODEL_INFO* p_usr_info, UINT32 mode)
{

	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT er = HD_OK;
	NN_GEN_MODEL_HEAD *p_head;

//	NN_DIFF_IOMEM *p_diff_io_mem;
//	NN_DATA* p_sai = NULL;
//	NN_DATA* p_sao = NULL;
//
//	UINT32 buf0_type = 0, buf1_type = 0, buf0_va = 0, buf1_va = 0, buf0_pa = 0, buf1_pa = 0;
//	UINT32 idx = 0;
//	UINT32 layer_index = 0;
//	UINT32 tmp;

	//NN_DATA *p_sai, *p_sao, *p_diff_sai, *p_diff_sao;
	//UINT32 imem_cnt, omem_cnt, idiffmem_cnt, odiffmem_cnt;


	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_BATCH_HEAD* p_model_diff_head = NULL;

	UINT32 model_num;
//	UINT32 model_pa_ofs, buff_pa_ofs;
//	UINT32 model_va_ofs, buff_va_ofs;
	UINT32 model_id = 0;
	UINT32 parm_addr = 0;

	NN_DIFF_BATCH_PARM* p_diff_param;

	UINT32 proc_idx = 0, proc_num = 0;
	NN_GEN_MODE_CTRL *p_mctrl;

	if (p_mem == NULL || p_model_head == NULL || p_usr_info == NULL) {
		DBG_ERR("input pointer is null...\r\n");
        return -1;
	}

	if (p_mem->user_parm.va == 0) {
		DBG_ERR("input addr is null...\r\n");
        return -1;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
    if (er != HD_OK) {
        DBG_ERR("vendor_ais_get_net_info fail...\r\n");
        return er;
    }

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	proc_num = p_head->mode_ctrl_num;
	model_num = p_model_head->stripe_model_num;
	model_id = p_usr_info->id;

	if (model_id >= model_num) {
		DBG_ERR("model id is exceed the max diff model num\r\n");
        return -1;
	}

	p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_id*sizeof(NN_DIFF_MODE_CTRL));
	p_model_diff_head = (NN_DIFF_BATCH_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);

//	p_diff_io_mem = (NN_DIFF_IOMEM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_BATCH_HEAD));

	p_diff_param  = (NN_DIFF_BATCH_PARM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_BATCH_HEAD) + p_model_diff_head->io_buff_size);

//	printf("diff mode ctrl    = 0x%08X\n", (unsigned int)p_model_diff_mode_ctrl);
//	printf("p_model_diff_head = 0x%08X\n", (unsigned int)p_model_diff_head);
//	printf("p_diff_io_mem     = 0x%08X\n", (unsigned int)p_diff_io_mem);
//	printf("p_diff_param      = 0x%08X\n", (unsigned int)p_diff_param);
//
//	printf("model num     = %d\n", (unsigned int)model_num);
//	printf("mode ctrl ofs = %d\n", (unsigned int)p_model_diff_mode_ctrl->offset);
//	printf("io buf size       = %d\n", (unsigned int)p_model_diff_head->io_buff_size);
//	printf("batch id          = %d\n", (unsigned int)p_model_diff_head->batch_id);
//	printf("batch_parm_size   = %d\n", (unsigned int)p_model_diff_head->batch_parm_size);

/*
 *  No need to update user parm because it's only for ai1 flow,
 *  Keep the useless code for sync
 *
	model_va_ofs = ALIGN_CEIL_4(p_mem->user_model.va);
	buff_va_ofs = ALIGN_CEIL_4(p_mem->user_buff.va);
	model_pa_ofs = ALIGN_CEIL_4(p_mem->user_model.pa);
	buff_pa_ofs = ALIGN_CEIL_4(p_mem->user_buff.pa);

	if (buff_va_ofs > model_va_ofs) {
		buf1_va = buff_va_ofs;
		buf1_pa = buff_pa_ofs;
		buf1_type = NN_GEN_BUF_ADDR_TYPE;
		buf0_va = model_va_ofs;
		buf0_pa = model_pa_ofs;
		buf0_type = NN_GEN_MODEL_ADDR_TYPE;
	} else {
		buf1_va = model_va_ofs;
		buf1_pa = model_pa_ofs;
		buf1_type = NN_GEN_MODEL_ADDR_TYPE;
		buf0_va = buff_va_ofs;
		buf0_pa = buff_pa_ofs;
		buf0_type = NN_GEN_BUF_ADDR_TYPE;
	}

	layer_index = 0;
	for (proc_idx = 0; proc_idx < proc_num; proc_idx++) {
		if (p_mctrl[proc_idx].layer_index == layer_index) {
			p_sai = (NN_DATA*)p_mctrl[proc_idx].iomem.imem_addr;
			p_sao = (NN_DATA*)p_mctrl[proc_idx].iomem.omem_addr;
			for (idx = 0; idx < p_mctrl[proc_idx].iomem.imem_cnt; idx++) {
				if (layer_index == 0) {
					break;
				}
				if (p_diff_io_mem[layer_index].SAI[idx].va == 0) {
					continue;
				}
				if (mode == 0) {
					if ((p_diff_io_mem[layer_index].SAI[idx].va & NN_GEN_BUF_ADDR_TYPE) > 0) {
						p_diff_io_mem[layer_index].SAI[idx].va = (p_diff_io_mem[layer_index].SAI[idx].va & 0xFFFFFFF) + buff_va_ofs;
						p_diff_io_mem[layer_index].SAI[idx].pa = (p_diff_io_mem[layer_index].SAI[idx].pa & 0xFFFFFFF) + buff_pa_ofs;
					} else {
						p_diff_io_mem[layer_index].SAI[idx].va = (p_diff_io_mem[layer_index].SAI[idx].va & 0xFFFFFFF) + model_va_ofs;
						p_diff_io_mem[layer_index].SAI[idx].pa = (p_diff_io_mem[layer_index].SAI[idx].pa & 0xFFFFFFF) + model_pa_ofs;
					}

				} else {
					if (p_sai[idx].va > buf1_va) {
						p_sai[idx].va = (p_sai[idx].va - buf1_va) | buf1_type;
						p_sai[idx].pa = (p_sai[idx].pa - buf1_pa) | buf1_type;
					} else {
						p_sai[idx].va = (p_sai[idx].va - buf0_va) | buf0_type;
						p_sai[idx].pa = (p_sai[idx].pa - buf0_pa) | buf0_type;
					}
				}
				SWAP(p_diff_io_mem[layer_index].SAI[idx].va, p_sai[idx].va, tmp);
				SWAP(p_diff_io_mem[layer_index].SAI[idx].pa, p_sai[idx].pa, tmp);
				SWAP(p_diff_io_mem[layer_index].SAI[idx].size, p_sai[idx].size, tmp);
			}

			for (idx = 0; idx < p_mctrl[proc_idx].iomem.omem_cnt; idx++) {
				if (p_diff_io_mem[layer_index].SAO[idx].va == 0) {
					continue;
				}
//				printf("layer %d out %d addr = 0x%08X\n", (int)layer_index, (int)idx, (unsigned int)p_diff_io_mem[layer_index].SAO[idx].va);
				if (mode == 0) {
					p_diff_io_mem[layer_index].SAO[idx].va = (p_diff_io_mem[layer_index].SAO[idx].va & 0xFFFFFFF) + buff_va_ofs;
					p_diff_io_mem[layer_index].SAO[idx].pa = (p_diff_io_mem[layer_index].SAO[idx].pa & 0xFFFFFFF) + buff_pa_ofs;
				} else {
					p_sao[idx].va = (p_sao[idx].va - buff_va_ofs) | NN_GEN_BUF_ADDR_TYPE;
					p_sao[idx].pa = (p_sao[idx].pa - buff_pa_ofs) | NN_GEN_BUF_ADDR_TYPE;
				}
				SWAP(p_diff_io_mem[layer_index].SAO[idx].va, p_sao[idx].va, tmp);
				SWAP(p_diff_io_mem[layer_index].SAO[idx].pa, p_sao[idx].pa, tmp);
				SWAP(p_diff_io_mem[layer_index].SAO[idx].size, p_sao[idx].size, tmp);
			}

			layer_index++;
			if (layer_index == p_head->layer_num) {
				break;
			}
		}
	}
*/
	// update parameter in cpu layers
	for (proc_idx = 0; proc_idx < proc_num; proc_idx++) {
		if (p_mctrl[proc_idx].trig_src == NN_GEN_TRIG_COMMON) {
			FLOAT in_batch_ratio = 0.0;
			FLOAT out_batch_ratio = 0.0;
			UINT32 tmp_ratio = 0;
			FLOAT* p_ratio = NULL;
			parm_addr = p_mctrl[proc_idx].addr;
			if ((p_diff_param[proc_idx].in_batch_ratio >> 31) > 0) {
				tmp_ratio = p_diff_param[proc_idx].in_batch_ratio & 0x7FFFFFFF;
				p_ratio = (FLOAT*)(&tmp_ratio);
				in_batch_ratio = *p_ratio;
			} else {
				in_batch_ratio = (FLOAT)p_diff_param[proc_idx].in_batch_ratio;
			}


			if ((p_diff_param[proc_idx].out_batch_ratio >> 31) > 0) {
				tmp_ratio = p_diff_param[proc_idx].out_batch_ratio & 0x7FFFFFFF;
				p_ratio = (FLOAT*)(&tmp_ratio);
				out_batch_ratio = *p_ratio;
			} else {
				out_batch_ratio = (FLOAT)p_diff_param[proc_idx].out_batch_ratio;
			}

			switch (p_mctrl[proc_idx].mode) {
				case NN_PROPOSAL: {
					NN_CPU_PARM *p_cpu_parm = (NN_CPU_PARM *)parm_addr;

					if (mode == 0) {
						p_cpu_parm->batch = (unsigned int)(AI_ROUND(((FLOAT)p_cpu_parm->batch)/in_batch_ratio));
					} else {
						p_cpu_parm->batch = (unsigned int)(AI_ROUND(((FLOAT)p_cpu_parm->batch)*in_batch_ratio));
					}
				}
					break;
				case NN_POSTPROC: {
					NN_POSTPROC_PARM *p_postproc_parm = (NN_POSTPROC_PARM *)parm_addr;

					if (mode == 0) {
						p_postproc_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_postproc_parm->shape.batch_num)/in_batch_ratio));
					} else {
						p_postproc_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_postproc_parm->shape.batch_num)*in_batch_ratio));
					}
				}
					break;
				case NN_SOFTMAX: {
					NN_SOFTMAX_PARM *p_softmax_parm = (NN_SOFTMAX_PARM *)parm_addr;

					if (mode == 0) {
						p_softmax_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_softmax_parm->shape.batch_num)/in_batch_ratio));
					} else {
						p_softmax_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_softmax_parm->shape.batch_num)*in_batch_ratio));
					}

				}
					break;
				case NN_FC_POST: {
					NN_FC_POST_PARM *p_fc_post_parm = (NN_FC_POST_PARM *)parm_addr;
					if (mode == 0) {
						p_fc_post_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_fc_post_parm->shape.batch_num)/in_batch_ratio));
					} else {
						p_fc_post_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_fc_post_parm->shape.batch_num)*in_batch_ratio));
					}
				}
					break;
				case NN_POOL: {
					NN_POOL_PARM *p_pool_parm = (NN_POOL_PARM *)parm_addr;
					if (mode == 0) {
						p_pool_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_pool_parm->shape.batch_num)/in_batch_ratio));
					} else {
						p_pool_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_pool_parm->shape.batch_num)*in_batch_ratio));
					}
				}
					break;
				case NN_CUSTOMER: {
					NN_CUSTOM_DIM* dim_head = NULL;
					NN_CUSTOM_PARM* p_cust_head = (NN_CUSTOM_PARM*)parm_addr;
					UINT32 i = 0;
					UINT32 input_dim_ofs = 0;
					UINT32 output_dim_ofs = 0;

					input_dim_ofs = sizeof(NN_CUSTOM_PARM) + (p_cust_head->input_num + p_cust_head->output_num + p_cust_head->model_num)*sizeof(NN_DATA);
					output_dim_ofs = input_dim_ofs + sizeof(NN_CUSTOM_DIM)*p_cust_head->input_num;

					// parsing input batch
					dim_head = (NN_CUSTOM_DIM*)(parm_addr + input_dim_ofs);
					for (i = 0; i < p_cust_head->input_num; i++) {
						if (mode == 0) {
							dim_head[i].dim[3] = (unsigned int)(AI_ROUND(((FLOAT)dim_head[i].dim[3])/in_batch_ratio));
						} else {
							dim_head[i].dim[3] = (unsigned int)(AI_ROUND(((FLOAT)dim_head[i].dim[3])*in_batch_ratio));
						}
					}

					// parsing output
					dim_head = (NN_CUSTOM_DIM*)(parm_addr + output_dim_ofs);
					for (i = 0; i < p_cust_head->output_num; i++) {
						if (out_batch_ratio > 0.0) {
							if (mode == 0) {
								dim_head[i].dim[3] = (unsigned int)(AI_ROUND(((FLOAT)dim_head[i].dim[3])/out_batch_ratio));
							} else {
								dim_head[i].dim[3] = (unsigned int)(AI_ROUND(((FLOAT)dim_head[i].dim[3])*out_batch_ratio));
							}
						}
					}
				}

					break;
				/*case NN_BNSCALE: {
					NN_BNSCALE_PARM *p_bn_scl_parm = (NN_BNSCALE_PARM *)parm_addr;
					p_bn_scl_parm->shape.batch_num = batch;
				}
					break;*/

				case NN_PRELU: {
					NN_PRELU_PARM *p_prelu_parm = (NN_PRELU_PARM *)parm_addr;

					if (mode == 0) {
						p_prelu_parm->batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_prelu_parm->batch_num)/in_batch_ratio));
					} else {
						p_prelu_parm->batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_prelu_parm->batch_num)*in_batch_ratio));
					}
				}
					break;
				/*case NN_PRIORBOX: {
					NN_PRIORBOX_PARM *p_priorbox_parm = (NN_PRIORBOX_PARM *)parm_addr;
					p_priorbox_parm->shape.batch_num = batch;
				}
					break;*/
				case NN_DETOUT: {
					NN_DETOUT_PARM *p_detout_parm = (NN_DETOUT_PARM *)parm_addr;

					if (mode == 0) {
						p_detout_parm->batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_detout_parm->batch_num)/in_batch_ratio));
					} else {
						p_detout_parm->batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_detout_parm->batch_num)*in_batch_ratio));
					}
				}
					break;

				case NN_RESHAPE: {
					NN_PERMUTE_PARM *p_permute_parm = (NN_PERMUTE_PARM *)parm_addr;

					if (mode == 0) {
						p_permute_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_permute_parm->shape.batch_num)/in_batch_ratio));
					} else {
						p_permute_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_permute_parm->shape.batch_num)*in_batch_ratio));
					}
				}
					break;
				case NN_REVERSE: {
					NN_REVERSE_PARM *p_reverse_parm = (NN_REVERSE_PARM *)parm_addr;

					if (mode == 0) {
						p_reverse_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_reverse_parm->shape.batch_num)/in_batch_ratio));
					} else {
						p_reverse_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_reverse_parm->shape.batch_num)*in_batch_ratio));
					}
				}
					break;
				case NN_LSTM: {
					NN_LSTM_PARM* p_lstm_parm = (NN_LSTM_PARM *)parm_addr;

					if (mode == 0) {
						p_lstm_parm->in_shape0.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_lstm_parm->in_shape0.batch_num)/in_batch_ratio));
						p_lstm_parm->in_shape1.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_lstm_parm->in_shape1.batch_num)/in_batch_ratio));
					} else {
						p_lstm_parm->in_shape0.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_lstm_parm->in_shape0.batch_num)*in_batch_ratio));
						p_lstm_parm->in_shape1.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_lstm_parm->in_shape1.batch_num)*in_batch_ratio));
					}

				}
					break;
				case NN_NORM: {
					NN_NORM_PARM *p_norm_parm = (NN_NORM_PARM *)parm_addr;
					if (mode == 0) {
						p_norm_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_norm_parm->shape.batch_num)/in_batch_ratio));
					} else {
						p_norm_parm->shape.batch_num = (unsigned int)(AI_ROUND(((FLOAT)p_norm_parm->shape.batch_num)*in_batch_ratio));
					}
				}
					break;
				default :
					break;
			}
		}
	}

	// update external output layer info
	{
		UINT32* p_ex_head = (UINT32*)(p_mem->user_parm.va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
		UINT32 tmp_size = 0;
		UINT32 dst_tag  = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));
		UINT32 dst_tag2 = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('2')<<24));
		NN_IN_OUT_BUF_INFO2* buf_info2 = NULL;

		// try to find dst_tag2 first
		tmp_size = 0;
		while (tmp_size < p_head->external_size) {
			UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
			if (p_tmp_head[1] == dst_tag2) { // bit1 represent tag_type
				buf_info2 = (NN_IN_OUT_BUF_INFO2 *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
				if (buf_info2 == NULL) {
					DBG_ERR("ext_info2 pointer NULL!\r\n");
					return HD_ERR_NULL_PTR;
				}
				break;
			} else {
				if (p_tmp_head[0] == 0) { // bit0 represent size
					break;
				}
				tmp_size += p_tmp_head[0]; // add tmp_size, move to next
			}
		}

		// original dst_tag
		tmp_size = 0;
		while (tmp_size < p_head->external_size) {
			UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
			if (p_tmp_head[1] == dst_tag) {
				UINT32 i = 0;
				NN_IN_OUT_BUF_INFO* buf_info = (NN_IN_OUT_BUF_INFO*)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
				UINT32 buf_num = (p_tmp_head[0] - 2*sizeof(UINT32))/sizeof(NN_IN_OUT_BUF_INFO);

				if (((p_tmp_head[0] - 2*sizeof(UINT32)) % sizeof(NN_IN_OUT_BUF_INFO)) > 0) {
					DBG_ERR("external buffer info may no align with model!\r\n");
					buf_num = 0;
				}

				if (buf_num > 2) {
					for (i = 0; i < buf_num - 2; i++) {
						for (proc_idx = 0; proc_idx < proc_num; proc_idx++) {
							if (p_mctrl[proc_idx].layer_index == buf_info[i+2].fusion_layer_index) {
								FLOAT out_batch_ratio = 0.0;
								UINT32 tmp_ratio = 0;
								FLOAT* p_ratio = NULL;

								if ((p_diff_param[proc_idx].out_batch_ratio >> 31) > 0) {
									tmp_ratio = p_diff_param[proc_idx].out_batch_ratio & 0x7FFFFFFF;
									p_ratio = (FLOAT*)(&tmp_ratio);
									out_batch_ratio = *p_ratio;
								} else {
									out_batch_ratio = (FLOAT)p_diff_param[proc_idx].out_batch_ratio;
								}

								if (out_batch_ratio > 0.0) {
									if (mode == 0) {
										buf_info[i+2].batch = (unsigned int)(AI_ROUND(((FLOAT)buf_info[i+2].batch)/out_batch_ratio));
										if (buf_info2 != NULL) {
											buf_info2[i+2].batch = (unsigned int)(AI_ROUND(((FLOAT)buf_info2[i+2].batch)/out_batch_ratio));
										}
									} else {
										buf_info[i+2].batch = (unsigned int)(AI_ROUND(((FLOAT)buf_info[i+2].batch)*out_batch_ratio));
										if (buf_info2 != NULL) {
											buf_info2[i+2].batch = (unsigned int)(AI_ROUND(((FLOAT)buf_info2[i+2].batch)*out_batch_ratio));
										}
									}
								}
								break;
							}
						}
					}
				}
				break;
			} else {
				if (p_tmp_head[0] == 0) {
					break;
				}
				tmp_size += p_tmp_head[0];
			}
		}
	}
	return HD_OK;
}

HD_RESULT vendor_ais_pars_diff_batch_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AIS_DIFF_BATCH_MODEL_INFO *p_diff_info, UINT32 net_id)
{
	VENDOR_AIS_FLOW_UPDATE_NET_INFO update_net_info = {0};
	VENDOR_AIS_FLOW_MEM_PARM diff_mem = {0};
	//NVTMPP_IOC_VB_PA_TO_VA_S nvtmpp_msg = {0};

	PROF_START();
	if (p_mem == NULL || p_diff_info == NULL) {
		DBG_ERR("input pointer or addr is null...\r\n");
		return HD_ERR_ABORT;
	}
	if (net_id >= NN_SUPPORT_NET_MAX) {
		DBG_ERR("net_id is invalid...\r\n");
		return HD_ERR_ABORT;
	}

	if(vendor_ais_get_diff_batch_mem(p_mem, &diff_mem)) {
		DBG_ERR("get diff mem info fail...\r\n");
		return HD_ERR_ABORT;
	}

	if (vendor_ais_set_batch_model_id(p_diff_info, (NN_DIFF_MODEL_HEAD *)(diff_mem.va))) {
		DBG_ERR("find model id fail...\r\n");
		return HD_ERR_ABORT;
	}

	memcpy(&update_net_info.map_parm, p_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));

	vendor_ais_update_diff_batch_info(p_mem, (NN_DIFF_MODEL_HEAD *)(diff_mem.va), p_diff_info, 0);
	//nvtmpp_msg.pa = p_diff_mem->pa;
	//if (NVTMPP_IOCTL(ai_nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg) < 0) {
	//	DBG_ERR("pa to va ioctl fail...\r\n");
	//	return HD_ERR_ABORT;
	//}

	//update_net_info.net_info_va = nvtmpp_msg.va;
	update_net_info.net_info_pa = diff_mem.pa;
	update_net_info.net_info_size = diff_mem.size;
	update_net_info.model_id = p_diff_info->id;
	update_net_info.net_id = net_id;

	vendor_ais_update_diff_batch_mem(&update_net_info);

	PROF_END("parsing_time");

	return HD_OK;
}

HD_RESULT vendor_ais_unpars_diff_batch_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AIS_DIFF_BATCH_MODEL_INFO *p_diff_info, UINT32 net_id)
{
	VENDOR_AIS_FLOW_UPDATE_NET_INFO update_net_info = {0};
	VENDOR_AIS_FLOW_MEM_PARM diff_mem = {0};

	PROF_START();
	if (p_mem == NULL || p_diff_info == NULL) {
		DBG_ERR("input pointer or addr is null...\r\n");
		return HD_ERR_ABORT;
	}
	if (net_id >= NN_SUPPORT_NET_MAX) {
		DBG_ERR("net_id is invalid...\r\n");
		return HD_ERR_ABORT;
	}

	if(vendor_ais_get_diff_batch_mem(p_mem, &diff_mem)) {
		DBG_ERR("get diff mem info fail...\r\n");
		return HD_ERR_ABORT;
	}

	if (vendor_ais_set_batch_model_id(p_diff_info, (NN_DIFF_MODEL_HEAD *)(diff_mem.va))) {
		DBG_ERR("find model id fail...\r\n");
		return HD_ERR_ABORT;
	}

	memcpy(&update_net_info.map_parm, p_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));

	vendor_ais_update_diff_batch_info(p_mem, (NN_DIFF_MODEL_HEAD *)(diff_mem.va), p_diff_info, 1);

	update_net_info.net_info_pa = diff_mem.pa;
	update_net_info.net_info_size = diff_mem.size;
	update_net_info.model_id = p_diff_info->id;
	update_net_info.net_id = net_id;

	vendor_ais_restore_diff_batch_mem(&update_net_info);

	PROF_END("unparsing_time");

	return HD_OK;
}

HD_RESULT vendor_ais_get_diff_batch_id_num(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 *batch_id_num)
{
	VENDOR_AIS_FLOW_MEM_PARM diff_mem = {0};
	NN_DIFF_MODEL_HEAD *p_model_head = NULL;

	if (p_mem == NULL || batch_id_num == NULL) {
		DBG_ERR("input pointer or addr is null...\r\n");
		return HD_ERR_ABORT;
	}

	if(vendor_ais_get_diff_batch_mem(p_mem, &diff_mem)) {
		DBG_ERR("get diff mem info fail...\r\n");
		return HD_ERR_ABORT;
	}

	p_model_head = (NN_DIFF_MODEL_HEAD *)(diff_mem.va);
	*batch_id_num = p_model_head->stripe_model_num;

	return HD_OK;
}

HD_RESULT vendor_ais_get_diff_batch_id_info(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 *max_batch_num, UINT32* p_batch_id)
{
	VENDOR_AIS_FLOW_MEM_PARM diff_mem = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_NET_INFO net_info = {0};
	NN_DIFF_BATCH_HEAD* p_model_diff_head = NULL;
	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_MODEL_HEAD *p_model_head = NULL;
	NN_DIFF_IOMEM *p_diff_io_mem;
	UINT32 model_num = 0;
	UINT32 model_idx = 0;
	UINT32 proc_idx = 0, proc_num = 0;
	UINT32 idx = 0;
	UINT32 layer_index = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	HD_RESULT er = HD_OK;


	if (p_mem == NULL || p_batch_id == NULL || max_batch_num == NULL) {
		DBG_ERR("input pointer or addr is null...\r\n");
		return HD_ERR_ABORT;
	}

	if (p_mem->user_parm.va == 0) {
		DBG_ERR("input addr is null...\r\n");
        return -1;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
    if (er != HD_OK) {
        DBG_ERR("vendor_ais_get_net_info fail...\r\n");
        return er;
    }

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_num = p_head->mode_ctrl_num;

	if(vendor_ais_get_diff_batch_mem(p_mem, &diff_mem)) {
		DBG_ERR("get diff mem info fail...\r\n");
		return HD_ERR_ABORT;
	}

	p_model_head = (NN_DIFF_MODEL_HEAD *)(diff_mem.va);
	model_num = p_model_head->stripe_model_num;

	// store the diff batch id info
	for (model_idx = 0; model_idx < model_num; model_idx++) {
		p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_idx*sizeof(NN_DIFF_MODE_CTRL));
		p_model_diff_head = (NN_DIFF_BATCH_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
		p_batch_id[model_idx] = p_model_diff_head->batch_id;
	}

	// store the max batch num
	model_idx = 0;
	p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_idx*sizeof(NN_DIFF_MODE_CTRL));
	p_model_diff_head = (NN_DIFF_BATCH_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
	p_diff_io_mem = (NN_DIFF_IOMEM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_BATCH_HEAD));
	layer_index = 0;
	*max_batch_num = 0;
	for (proc_idx = 0; proc_idx < proc_num; proc_idx++) {
		if (p_mctrl[proc_idx].eng == NN_GEN_ENG_CNN) {
			NN_DATA* p_sao = (NN_DATA*)p_mctrl[proc_idx].iomem.omem_addr;
			layer_index = p_mctrl[proc_idx].layer_index;
			for (idx = 0; idx < p_mctrl[proc_idx].iomem.omem_cnt; idx++) {
				if (p_diff_io_mem[layer_index].SAO[idx].va == 0) {
					continue;
				}
				*max_batch_num = p_batch_id[model_idx]*p_sao[idx].size/p_diff_io_mem[layer_index].SAO[idx].size;
				break;
			}

			break;
		}
	}
	if (*max_batch_num == 0) {
		DBG_ERR("calculate max_batch_num fail...the max_batch_num cannot be referenced\r\n");
		return HD_ERR_ABORT;
	}


	return HD_OK;
}

HD_RESULT vendor_ais_pars_diff_batch(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AI_BATCH_INFO *p_diff_batch, UINT32 net_id)
{
	HD_RESULT hd_ret = HD_OK;
	VENDOR_AIS_DIFF_BATCH_MODEL_INFO diff_batch_info = {0};

	PROF_START();
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - pars_diff_batch - begin \r\n");

	if (p_mem == NULL || p_diff_batch == NULL) {
		DBG_ERR("input pointer or addr is null...\r\n");
		return HD_ERR_ABORT;
	}
	if (net_id >= g_ai_support_net_max) {
		DBG_ERR("net_id is invalid...\r\n");
		return HD_ERR_ABORT;
	}

	if (0) {  // for debug
		UINT32* p_batch_id = NULL;
		UINT32 max_batch_id_num = 0;
		UINT32 max_batch_num = 0;
		UINT32 i;
		// dump useable batch id
		vendor_ais_get_diff_batch_id_num(p_mem, &max_batch_id_num);
		DBG_DUMP("useable switch batch id num = %d\n", (int)max_batch_id_num);
		p_batch_id = (UINT32*)malloc(max_batch_id_num*sizeof(UINT32));

		vendor_ais_get_diff_batch_id_info(p_mem, &max_batch_num, p_batch_id);
		DBG_DUMP("max batch num = %d\n", (int)max_batch_num);
		for (i = 0; i < max_batch_id_num; i++) {
			DBG_DUMP("switchable batch id = %d\n", (int)p_batch_id[i]);
		}
		free(p_batch_id);
	}

	diff_batch_info.batch_num = p_diff_batch->num;
	diff_batch_info.id = p_diff_batch->id;
	hd_ret = vendor_ais_pars_diff_batch_mem(p_mem, &diff_batch_info, net_id);
	if (hd_ret != HD_OK) {
		DBG_ERR("vendor_ais_pars_diff_batch_mem (%d)\n\r", hd_ret);
		return hd_ret;
	}

	vendor_ai_net_trace(net_id, AI_FLOW, "start() - pars_diff_batch - net_id(%u), batch_num(%u), id(%d)\r\n",
						net_id, diff_batch_info.batch_num, diff_batch_info.id);
	vendor_ai_net_trace(net_id, AI_FLOW, "start() - pars_diff_batch - end \r\n");
	PROF_END("parsing_time");

	return HD_OK;
}

HD_RESULT vendor_ais_unpars_diff_batch(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AI_BATCH_INFO *p_diff_batch, UINT32 net_id)
{
	HD_RESULT hd_ret = HD_OK;
	VENDOR_AIS_DIFF_BATCH_MODEL_INFO diff_batch_info = {0};

	PROF_START();
	if (p_mem == NULL || p_diff_batch == NULL) {
		DBG_ERR("input pointer or addr is null...\r\n");
		return HD_ERR_ABORT;
	}
	if (net_id >= g_ai_support_net_max) {
		DBG_ERR("net_id is invalid...\r\n");
		return HD_ERR_ABORT;
	}

	diff_batch_info.batch_num = p_diff_batch->num;
	diff_batch_info.id = p_diff_batch->id;
	hd_ret = vendor_ais_unpars_diff_batch_mem(p_mem, &diff_batch_info, net_id);
	if (hd_ret != HD_OK) {
		return HD_ERR_ABORT;
	}

	PROF_END("unparsing_time");

	return HD_OK;
}

HD_RESULT vendor_ais_get_net_info(NN_GEN_NET_INFO *p_info, UINT32 net_addr)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num, model_head_size, mctrl_size;
	NN_GEN_MODE_CTRL *p_mctrl;
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
#if CNN_528_PSW
	UINT32 *p_id_list;
#endif
	if ((p_info == NULL) || (net_addr == 0)) {
		DBG_ERR("null input...\r\n");
		return HD_ERR_BAD_DATA;
	}

	p_head = (NN_GEN_MODEL_HEAD *)net_addr;
	proc_layer_num = p_head->mode_ctrl_num;
	model_head_size = ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD));
	mctrl_size = ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * proc_layer_num);
	p_mctrl = (NN_GEN_MODE_CTRL *)((UINT32)p_head + model_head_size);
#if CNN_528_PSW
	p_id_list = (UINT32 *)((UINT32)p_mctrl + mctrl_size);
#if CNN_25_MATLAB
	p_io_mem = (NN_IOMEM *)((UINT32)p_id_list + p_head->layer_id_list_size);
#endif
#else
	p_io_mem = (NN_IOMEM *)((UINT32)p_mctrl + mctrl_size);
#endif

	p_info->p_head      = p_head;
	p_info->p_mctrl     = p_mctrl;
#if CNN_25_MATLAB
	p_info->p_io_mem    = p_io_mem;
#endif
#if CNN_528_PSW
	p_info->p_id_list   = p_id_list;
#endif

	return HD_OK;
}

#if CNN_MULTI_INPUT
#define AI_INPUT_ADDR_TYPE 0
UINT32 vendor_ais_net_get_input_layer_index(VENDOR_AIS_FLOW_MEM_PARM mem)
{
	UINT32 preproc_idx_num = 0;
#if !CNN_25_MATLAB
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	ER er = E_OK;
	UINT32 i = 0;
	UINT32 proc_layer_num;

	if (mem.va == 0) {
		DBG_ERR("input memory is 0...");
		return 0;
	}

	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return 0;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	for (i = 0; i < proc_layer_num; i++) {
		NN_DATA* p_input;
		UINT32 imem_addr = p_mctrl[i].iomem.imem_addr;
		if (imem_addr < mem.va) imem_addr += mem.va; // if not fix yet(call this funciton before gen_init(), fix it

		p_input = (NN_DATA*)imem_addr;
		if (p_input[0].va == AI_INPUT_ADDR_TYPE) {
#if NET_TABLE_HEAP
#else
			if (preproc_idx_num > AI2_MAX_BATCH_NUM) {
				DBG_ERR("preproc_idx_num(%lu) exceeds AI2_MAX_BATCH_NUM(%d) limit !!\r\n", preproc_idx_num, AI2_MAX_BATCH_NUM);
				return 0;
			}
#endif
			preproc_idx_num++;
		} else {
			// suppose the input layers are always set at the first n layers...
			continue;
		}
	}
#endif

	return preproc_idx_num;
}
#endif
UINT32 vendor_ais_net_get_fc_cmd_size(UINT32 max_weight_h, UINT32 net_id)
{
	UINT32 size = max_weight_h;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_GET_FC_CMD_SIZE, &size) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_GET_FC_CMD_SIZE ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return size;
}

HD_RESULT vendor_ais_set_fc_ll_cmd(VENDOR_AI_OP_FC_CMDBUF *cmd_buf, UINT32 net_id)
{
	KFLOW_AI_OP_FC_CMDBUF fc_cmd_buf = {0};
	fc_cmd_buf.proc_id = net_id;
	fc_cmd_buf.buf_pa = cmd_buf->pa;
	fc_cmd_buf.buf_va = cmd_buf->va;
	fc_cmd_buf.buf_size = cmd_buf->size;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id+1], VENDOR_AIS_FLOW_IOC_SET_FC_LL_CMD, &fc_cmd_buf) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_SET_FC_LL_CMD ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}
