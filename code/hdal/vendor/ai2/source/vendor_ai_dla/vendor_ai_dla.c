/**
    Source file for dal_ai_dla

    This file is the source file that implements the API for vendor_ai_dla.

    @file       vendor_ai_dla.c
    @ingroup    vendor_ai_dla

    Copyright   Novatek Microelectronics Corp. 2018.    All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "hd_type.h"
#include "hd_common.h"

#include "kflow_ai_net/kflow_ai_net.h"
#include "vendor_ai_dla/vendor_ai_dla.h"
#include "ai_ioctl.h"
#include "hdal.h"
#include "vendor_ai_comm_flow.h"

//=============================================================
#define __CLASS__ 				"[ai][lib][dla]"
#include "vendor_ai_debug.h"
//=============================================================

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
VOID vendor_ai_errno_location(VOID);
/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

#if (NEW_AI_FLOW == 1)

extern INT32 *vendor_ais_flow_fd;
VENDOR_AIS_FLOW_CORE_INFO core_info = {0};

HD_RESULT vendor_ai_dla_getinfo(VENDOR_AI_ENGINE_ID engine_id)
{
	switch (engine_id) {
	case VENDOR_AI_ENGINE_CNN:
		if (((core_info.info >> 8) & 0xff) > 0) {
			return HD_OK;
		}
		break;
	case VENDOR_AI_ENGINE_CNN2:
		if (((core_info.info >> 8) & 0xff) > 1) {
			return HD_OK;
		}
		break;
	case VENDOR_AI_ENGINE_NUE:
		if (((core_info.info) & 0xff) > 0) {
			return HD_OK;
		}
		break;
	case VENDOR_AI_ENGINE_NUE2:
		if (((core_info.info >> 16) & 0xff) > 0) {
			return HD_OK;
		}
		break;
	default:
		return HD_ERR_NG;
	}
	return HD_ERR_NG;
}

HD_RESULT vendor_ai_dla_reset (void)
{
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_CORE_RESET, &core_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CORE_RESET ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dla_init (void)
{
	UINT32 max_nue2 = 0xff; //no-limit
	UINT32 max_cnn = 0xff; //no-limit
	UINT32 max_nue = 0xff; //no-limit
	
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	INT32 ret;
	UINT32 cfg2;
	//check CNN2 access SDRAM of IPP
	ret = hd_common_get_sysconfig(0, &cfg2);
	if (ret != HD_OK) {
		DBG_ERR("[VENDOR_AI] Project not config error!\n");
		return HD_ERR_NG;
	}
	if (!((cfg2 >> 16) & 0x01)) {
		//DBG_ERR("[VENDOR_AI] force set cnn sw-limt=1 (cannot use CNN2)!\n");
		max_cnn = 1; //force set cnn sw-limt=1 (cannot use CNN2)
	}
#endif

	core_info.info = (max_nue2 << 16) | (max_cnn << 8) | (max_nue);

	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_CORE_INIT, &core_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CORE_INIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dla_uninit (void)
{
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_CORE_UNINIT, &core_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CORE_UNINIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

HD_RESULT vendor_ai_dla_open_path(UINT32 proc_id)
{
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_OPEN_PATH, &proc_id) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CORE_UNINIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dla_close_path(UINT32 proc_id)
{
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_CLOSE_PATH, &proc_id) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CORE_UNINIT ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dla_perf_ut(UINT32 enable, VENDOR_AI_ENGINE_PERF_UT* p_perf_ut)
{
	if (enable == 1) {
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], KFLOW_AI_IOC_CMD_PERF_UT_BEGIN, &enable) < 0) {
			fprintf(stderr, "KFLOW_AI_IOC_CMD_PERF_UT_BEGIN ioctl failed\r\n");
			vendor_ai_errno_location();
			return HD_ERR_ABORT;
		}
	} else {
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], KFLOW_AI_IOC_CMD_PERF_UT_END, p_perf_ut) < 0) {
			fprintf(stderr, "KFLOW_AI_IOC_CMD_PERF_UT_END ioctl failed\r\n");
			vendor_ai_errno_location();
			return HD_ERR_ABORT;
		}
	}

	return HD_OK;
}

HD_RESULT vendor_ai_dla_create_joblist (UINT32 proc_id, UINT32 max_job_cnt, UINT32 job_cnt, UINT32 bind_cnt, UINT32 ddr_id)
{
	VENDOR_AIS_FLOW_JOBLIST_INFO joblist_info;

	joblist_info.proc_id = proc_id;
	joblist_info.max_job_cnt = max_job_cnt;
	joblist_info.job_cnt = job_cnt;
	joblist_info.bind_cnt = bind_cnt;
	joblist_info.ddr_id = ddr_id;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_NEW_JOBLIST, &joblist_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_NEW_JOBLIST ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}
HD_RESULT vendor_ai_dla_destory_joblist (UINT32 proc_id)
{
	VENDOR_AIS_FLOW_JOBLIST_INFO joblist_info;
	joblist_info.proc_id = proc_id;
	joblist_info.job_cnt = 0;
	joblist_info.bind_cnt = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_DEL_JOBLIST, &joblist_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_DEL_JOBLIST ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

HD_RESULT vendor_ai_dla_dump_joblist(UINT32 proc_id, UINT32 info)
{
	VENDOR_AIS_FLOW_JOBLIST_INFO joblist_info;
	joblist_info.proc_id = proc_id;
	joblist_info.job_cnt = info;
	joblist_info.bind_cnt = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_DUMP_JOBLIST, &joblist_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_DUMP_JOBLIST ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	return HD_OK;
}


HD_RESULT vendor_ai_dla_clear_job (UINT32 proc_id, UINT32 job_id)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = 0;
	job_info.engine_id = 0;
	job_info.engine_op = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_CLR_JOB, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CLR_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dla_clear_job3 (UINT32 proc_id, UINT32 job_id)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = 0;
	job_info.engine_id = 0;
	job_info.engine_op = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_CLR_JOB3, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CLR_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

//HD_RESULT vendor_ai_dla_set_job (UINT32 proc_id, UINT32 job_id, UINT32 engine_id, UINT32 engine_op, void* p_op_info, void* p_io_info, INT32 wait_ms)
HD_RESULT vendor_ai_dla_set_job (UINT32 proc_id, UINT32 job_id, UINT32 engine_id, UINT32 engine_op, UINT32 schd_parm, void* p_op_info, void* p_io_info, INT32 wait_ms)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = (UINT32)p_op_info;
	job_info.info2 = (UINT32)p_io_info;
	job_info.engine_id = engine_id;
	job_info.engine_op = engine_op;
	job_info.schd_parm = schd_parm;
	job_info.wait_ms = wait_ms;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_SET_JOB, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_SET_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

//HD_RESULT vendor_ai_dla_set_job2 (UINT32 proc_id, UINT32 job_id, UINT32 engine_id, UINT32 engine_op, void* p_op_info, void* p_io_info, INT32 wait_ms)
HD_RESULT vendor_ai_dla_set_job2 (UINT32 proc_id, UINT32 job_id, UINT32 engine_id, UINT32 engine_op, UINT32 schd_parm, void* p_op_info, void* p_io_info, INT32 wait_ms)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = (UINT32)p_op_info;
	job_info.info2 = (UINT32)p_io_info;
	job_info.engine_id = engine_id;
	job_info.engine_op = engine_op;
	job_info.schd_parm = schd_parm;
	job_info.wait_ms = wait_ms;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_SET_JOB2, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_SET_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dla_set_job3 (UINT32 proc_id, UINT32 job_id, UINT32 engine_id, UINT32 engine_op, UINT32 schd_parm, void* p_op_info, void* p_io_info, INT32 wait_ms)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = (UINT32)p_op_info;
	job_info.info2 = (UINT32)p_io_info;
	job_info.engine_id = engine_id;
	job_info.engine_op = engine_op;
	job_info.schd_parm = schd_parm;
	job_info.wait_ms = wait_ms;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_SET_JOB3, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_SET_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_dla_bind_job (UINT32 proc_id, UINT32 job_id, UINT32 next_job_id)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = next_job_id;
	job_info.wait_ms = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_BIND_JOB, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_BIND_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	return HD_OK;
}
HD_RESULT vendor_ai_dla_sum_job (UINT32 proc_id, UINT32* src_count, UINT32* dest_count)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = 0;
	job_info.info = 0;
	job_info.wait_ms = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_UNBIND_JOB, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_UNBIND_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}

	if (src_count)
		src_count[0] = job_info.info;
	if (dest_count)
		dest_count[0] = job_info.info2;
	
	return HD_OK;
}
HD_RESULT vendor_ai_dla_push_job (UINT32 proc_id, UINT32 job_id, UINT32 schd_parm)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = 0;
	job_info.wait_ms = 0;
	job_info.schd_parm = schd_parm;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_PUSH_JOB, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_PUSH_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	if (job_info.wait_ms == -1) {
		return HD_ERR_NOT_START;
	}
	return HD_OK;
}
HD_RESULT vendor_ai_dla_pull_job (UINT32 proc_id, UINT32* job_id)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	UINT32 cjob;
	cjob = job_id[0];
	job_info.proc_id = proc_id;
	job_info.job_id = job_id[0];
	job_info.info = 0;
	job_info.wait_ms = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_PULL_JOB, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_PULL_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	if (job_info.wait_ms == -1) {
		return HD_ERR_NOT_START;
	}
	if (cjob == 0xf0000004) {
		job_id[0] = job_info.job_id;
		if (job_info.info == 0) {
			return HD_ERR_EOL;
		}
		if (job_info.info == (UINT32)-1) {
			return HD_ERR_FAIL;
		}
		if (job_info.info == (UINT32)-2) {
			return HD_ERR_TIMEDOUT;
		}
	}
	return HD_OK;
}
HD_RESULT vendor_ai_dla_lock_job (UINT32 proc_id, UINT32 job_id)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = 0;
	job_info.wait_ms = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_LOCK_JOB, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_LOCK_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	if (job_info.wait_ms == -1) {
		return HD_ERR_NOT_START;
	}
	if (job_info.rv == -1) {
		return HD_ERR_FAIL;
	}
	return HD_OK;
}
HD_RESULT vendor_ai_dla_unlock_job (UINT32 proc_id, UINT32 job_id)
{
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	job_info.proc_id = proc_id;
	job_info.job_id = job_id;
	job_info.info = 0;
	job_info.wait_ms = 0;
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id+1], VENDOR_AIS_FLOW_IOC_UNLOCK_JOB, &job_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_UNLOCK_JOB ioctl failed\r\n");
		vendor_ai_errno_location();
		return HD_ERR_ABORT;
	}
	if (job_info.wait_ms == -1) {
		return HD_ERR_NOT_START;
	}
	if (job_info.rv == -1) {
		return HD_ERR_FAIL;
	}
	return HD_OK;
}
#if 0
INT32 vendor_ai_drv_set_link_info(AI_DRV_LL_USR_INFO *p_param)
{
	return HD_OK;
}
INT32 vendor_ai_drv_uninit_link_info(UINT32 net_id)
{
	return HD_OK;
}
INT32 vendor_ai_drv_init_ll_buf(AI_DRV_LL_BUF_INFO *p_param)
{
	return HD_OK;
}
INT32 vendor_ai_drv_uninit_ll_buf(UINT32 net_id)
{
	return HD_OK;
}
#endif

#else

#define PROF                DISABLE
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


/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static BOOL ai_api_init[AI_SUPPORT_NET_MAX] = {FALSE};
static INT32 ai_fd[AI_SUPPORT_NET_MAX] = {-1};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT32 vendor_ai_drv_init(AI_DRV_MAP_MEM map_mem, UINT32 net_id)
{
	INT32 ret;

	AI_DRV_OPENCFG ai_opencfg;
	AI_DRV_MAP_MEMINFO mem_info;

	if (ai_api_init[net_id] == FALSE) {
		ai_api_init[net_id] = TRUE;
		
		// open ai device
		ai_fd[net_id] = open("/dev/nvt_ai_module0", O_RDWR);
		if (ai_fd[net_id] < 0) {
			DBG_ERR("[VENDOR_AI] Open kdrv_ai fail!\n");
			return HD_ERR_NG;
		}

		ai_opencfg.opencfg.clock_sel = 480;
		ai_opencfg.engine = AI_ENG_CNN;
		ai_opencfg.net_id = net_id;
		ret = ioctl(ai_fd[net_id], AI_IOC_OPENCFG, &ai_opencfg);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai CNN opencfg fail! \n");
			return ret;
		}

		ai_opencfg.opencfg.clock_sel = 480;
		ai_opencfg.engine = AI_ENG_NUE;
		ai_opencfg.net_id = net_id;
		ret = ioctl(ai_fd[net_id], AI_IOC_OPENCFG, &ai_opencfg);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai NUE opencfg fail! \n");
			return ret;
		}

		mem_info.mem = map_mem;
		mem_info.net_id = net_id;
		ret = ioctl(ai_fd[net_id], AI_IOC_SET_MAP_ADDR, &mem_info);     //registration of memory
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai memory mapping fail! \n");
			return ret;
		}
		ret = ioctl(ai_fd[net_id], AI_IOC_ENG_INIT, &ai_opencfg);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai set eng init fail! \n");
			return ret;
		}

		return 0;
	} else {
		DBG_ERR("[VENDOR_AI] ai module is already initialised. \n");
		return -1;
	}
}

INT32 vendor_ai_drv_uninit(UINT32 net_id)
{
	if (ai_api_init[net_id] == TRUE) {
		ai_api_init[net_id] = FALSE;
		if (ioctl(ai_fd[net_id], AI_IOC_ENG_UNINIT, NULL) < 0) {
			DBG_ERR("[VENDOR_AI] ai set eng uninit fail! \n");
			return -1;
		}
		if (ai_fd[net_id] != -1) {
			close(ai_fd[net_id]);
		}

		return 0;
	} else {
		DBG_ERR("[VENDOR_AI] ai module is already uninitialised. \n");
		return -1;
	}
}

INT32 vendor_ai_drv_open(VENDOR_AI_ENG engine, UINT32 net_id)
{
	INT32 ret = 0;
	ret = ioctl(ai_fd[net_id], AI_IOC_OPEN, &engine);
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai open fail! \n");
	}
	return ret;
}

INT32 vendor_ai_drv_close(VENDOR_AI_ENG engine, UINT32 net_id)
{
	INT32 ret = 0;
	ret = ioctl(ai_fd[net_id], AI_IOC_CLOSE, &engine);
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai close fail! \n");
	}
	return ret;
}

INT32 vendor_ai_drv_set_param(VENDOR_AI_PARAM_ID param_id, void *p_param)
{
	int ret = 0;
	UINT32 net_id = 0;
	if (p_param == NULL) {
		DBG_ERR("[VENDOR_AI] p_param is null\n");
		return -1;
	}
	if (param_id == VENDOR_AI_PARAM_MODE_INFO) {
		net_id = ((AI_DRV_MODEINFO*)p_param)->net_id;
	} else if (param_id == VENDOR_AI_PARAM_APP_INFO) {
		net_id = ((AI_DRV_APPINFO*)p_param)->net_id;
	} else if (param_id == VENDOR_AI_PARAM_LL_INFO) {
		net_id = ((AI_DRV_LLINFO*)p_param)->net_id;
	}

	if (ai_api_init[net_id] == FALSE) {
		DBG_ERR("[VENDOR_AI] ai should be init before set param\n");
		return -1;
	}

	switch (param_id) {
	case VENDOR_AI_PARAM_MODE_INFO:
		ret = ioctl(ai_fd[net_id], AI_IOC_SET_MODE, (AI_DRV_MODEINFO*)p_param);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai set AI_DRV_MODEINFO fail! \n");
			return ret;
		}
		break;
	case VENDOR_AI_PARAM_APP_INFO:
		ret = ioctl(ai_fd[net_id], AI_IOC_SET_APP, (AI_DRV_APPINFO*)p_param);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai set AI_DRV_APPINFO fail! \n");
			return ret;
		}
		break;
	case VENDOR_AI_PARAM_LL_INFO:
		ret = ioctl(ai_fd[net_id], AI_IOC_SET_LL, (AI_DRV_LLINFO*)p_param);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai set AI_DRV_LLINFO fail! \n");
			return ret;
		}
		break;
	default:
		DBG_ERR("[VENDOR_AI] not support id %d \n", param_id);
		break;
	}

	return 0;
}

INT32 vendor_ai_drv_get_param(VENDOR_AI_PARAM_ID param_id, void *p_param)
{
	int ret = 0;
	UINT32 net_id = 0;

	if (p_param == NULL) {
		DBG_ERR("[VENDOR_AI] p_param is null\n");
		return -1;
	}
	if (param_id == VENDOR_AI_PARAM_MODE_INFO) {
		net_id = ((AI_DRV_MODEINFO*)p_param)->net_id;
	} else if (param_id == VENDOR_AI_PARAM_APP_INFO) {
		net_id = ((AI_DRV_APPINFO*)p_param)->net_id;
	} else if (param_id == VENDOR_AI_PARAM_LL_INFO) {
		net_id = ((AI_DRV_LLINFO*)p_param)->net_id;
	}

	if (ai_api_init[net_id] == FALSE) {
		DBG_ERR("[VENDOR_AI] ai should be init before get param\n");
		return -1;
	}

	switch (param_id) {
	case VENDOR_AI_PARAM_MODE_INFO:
		ret = ioctl(ai_fd[net_id], AI_IOC_GET_MODE, (AI_DRV_MODEINFO*)p_param);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai get AI_DRV_MODEINFO fail! \n");
			return ret;
		}
		break;
	case VENDOR_AI_PARAM_APP_INFO:
		ret = ioctl(ai_fd[net_id], AI_IOC_GET_APP, (AI_DRV_APPINFO*)p_param);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai get AI_DRV_APPINFO fail! \n");
			return ret;
		}
		break;
	case VENDOR_AI_PARAM_LL_INFO:
		ret = ioctl(ai_fd[net_id], AI_IOC_GET_LL, (AI_DRV_LLINFO*)p_param);
		if (ret < 0) {
			DBG_ERR("[VENDOR_AI] ai get AI_DRV_LLINFO fail! \n");
			return ret;
		}
		break;
	default:
		DBG_ERR("[VENDOR_AI] not support id %d \n", param_id);
		break;
	}

	return 0;
}

INT32 vendor_ai_drv_trigger(AI_DRV_TRIGINFO *p_param)
{
	int ret = 0;
	UINT32 net_id = 0;

	if (p_param == NULL) {
		DBG_ERR("[VENDOR_AI] p_param is null\n");
		return -1;
	}

	net_id = p_param->net_id;
	if (ai_api_init[net_id] == FALSE) {
		DBG_ERR("[VENDOR_AI] ai should be init before trigger\n");
		return -1;
	}

	PROF_START();
	ret = ioctl(ai_fd[net_id], AI_IOC_TRIGGER, p_param);
	PROF_END("vendor_ai_drv");
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai trigger fail! \n");
		return ret;
	}

	return ret;
}

INT32 vendor_ai_drv_waitdone(AI_DRV_TRIGINFO *p_param)
{
	int ret = 0;
	UINT32 net_id = 0;

	if (p_param == NULL) {
		DBG_ERR("[VENDOR_AI] p_param is null\n");
		return -1;
	}

	net_id = p_param->net_id;
	if (ai_api_init[net_id] == FALSE) {
		DBG_ERR("[VENDOR_AI] ai should be init before waiting done\n");
		return -1;
	}

	PROF_START();
	ret = ioctl(ai_fd[net_id], AI_IOC_WAITDONE, p_param);
	PROF_END("vendor_ai_drv");
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai wait done fail! \n");
		return ret;
	}

	return ret;
}

INT32 vendor_ai_drv_reset(AI_DRV_TRIGINFO *p_param)
{
	int ret = 0;
	UINT32 net_id = 0;

	if (p_param == NULL) {
		DBG_ERR("[VENDOR_AI] p_param is null\n");
		return -1;
	}

	net_id = p_param->net_id;
	if (ai_api_init[net_id] == FALSE) {
		DBG_ERR("[VENDOR_AI] ai should be init before reset\n");
		return -1;
	}

	ret = ioctl(ai_fd[net_id], AI_IOC_RESET, p_param);
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai reset fail! \n");
		return ret;
	}

	return ret;
}

#if 0 //LL_SUPPORT_ROI
INT32 vendor_ai_drv_set_link_info(AI_DRV_LL_USR_INFO *p_param)
{
	int ret = 0;
	UINT32 net_id = 0;

	if (p_param == NULL) {
		DBG_ERR("[VENDOR_AI] p_param is null\n");
		return -1;
	}

	net_id = p_param->net_id;
	if (ai_api_init[net_id] == FALSE) {
		DBG_ERR("[VENDOR_AI] ai should be init before set link info\n");
		return -1;
	}

	ret = ioctl(ai_fd[net_id], AI_IOC_SET_LL_USR_INFO, p_param);
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai set link info fail!\n");
		return ret;
	}

	return ret;
}

INT32 vendor_ai_drv_uninit_link_info(UINT32 net_id)
{
	int ret = 0;

	ret = ioctl(ai_fd[net_id], AI_IOC_UNINIT_LINK_INFO, &net_id);
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai uninit link info fail!\n");
		return ret;
	}

	return ret;
}

INT32 vendor_ai_drv_init_ll_buf(AI_DRV_LL_BUF_INFO *p_param)
{
	int ret = 0;

	if (p_param == NULL) {
		DBG_ERR("[VENDOR_AI] p_param is null\n");
		return -1;
	}

	ret = ioctl(ai_fd[p_param->net_id], AI_IOC_INIT_LL_BUF, p_param);
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai init ll buffer fail!\n");
		return ret;
	}

	return ret;
}

INT32 vendor_ai_drv_uninit_ll_buf(UINT32 net_id)
{
	int ret = 0;

	ret = ioctl(ai_fd[net_id], AI_IOC_UNINIT_LL_BUF, &net_id);
	if (ret < 0) {
		DBG_ERR("[VENDOR_AI] ai uninit ll buffer fail!\n");
		return ret;
	}

	return ret;
}
#endif

#endif
