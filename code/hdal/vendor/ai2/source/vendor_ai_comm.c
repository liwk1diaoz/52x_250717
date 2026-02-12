/**
	@brief Source file of vendor user-space net API.

	@file vendor_ai_net.c

	@ingroup vendor_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "hd_type.h"
#include "hd_common.h"

#include "vendor_common.h"
#include "vendor_ai_comm.h"
#include "vendor_ai_comm_flow.h"
#include "vendor_ai.h"
#include "vendor_ai_net/vendor_ai_net_flow.h"
#include "vendor_ai_net/vendor_ai_net_debug.h"
#include "vendor_ai_net/vendor_ai_net_cmd.h"
#include "vendor_ai_net/vendor_ai_net_gen.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "vendor_ai_version.h"
#include "kflow_ai_version.h"
#include "kdrv_ai_version.h"

#if defined(__LINUX)
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

//=============================================================
#define __CLASS__ 				"[ai][lib][comm]"
#include "vendor_ai_debug.h"
//=============================================================

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern HD_RESULT vendor_ais_dma_abort(VOID);
#if CNN_AI_FASTBOOT
extern UINT32 vendor_ai_net_get_fastboot_rslt_start_addr(UINT32 proc_id);
extern HD_RESULT vendor_ais_set_fastboot_proc_period(UINT32 frame_cnt);
#endif
extern UINT32 g_ai_support_net_max;

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

static VENDOR_AI_PROC_STATE fix_proc_state[MAX_PROC_CNT] = {0};
static VENDOR_AI_NET_CFG_PROC fix_proc_cfg[MAX_PROC_CNT] = {0};
#if (NET_OPEN_HEAP == 1)
static VENDOR_AI_NET_INFO_PROC* fixed_proc_entry[MAX_PROC_CNT] = {0};
#endif
static VENDOR_AI_INFO g_ai_net_info[1] = {0};
static VENDOR_AI_COMMON_INFO g_ai_comm_info[1] = {0};

static VENDOR_AI_COMMON_INFO g_ai_comm_info_first[1] = {0};
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
BOOL is_multi_process = 0;
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

void _vendor_ai_cfg_max(UINT32 maxproc)
{
	if (!g_ai_net_info[0].proc) {
		g_ai_support_net_max = maxproc;
		if (g_ai_support_net_max > MAX_PROC_CNT) {
			DBG_WRN("cfg max_proc=%lu is larger than built-in max_proc=%lu!\r\n", g_ai_support_net_max, (UINT32)MAX_PROC_CNT);
			g_ai_support_net_max = MAX_PROC_CNT;
		}
	}
}

BOOL _vendor_ai_is_init(VOID)
{
	return (g_ai_net_info[0].proc != NULL);
}

HD_RESULT _vendor_ai_validate(UINT32 proc_id)
{
	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	if (proc_id >= g_ai_support_net_max) {
		DBG_ERR("proc_id(%lu) >= max limit(%lu)r\n", proc_id, g_ai_support_net_max);
		return HD_ERR_LIMIT;
	}

	if (proc_id >= g_ai_support_net_max) {
		DBG_ERR("proc_id(%lu) >= g_ai_support_net_max(%lu)r\n", proc_id, g_ai_support_net_max);
		return HD_ERR_LIMIT;
	}

	return HD_OK;
}

VENDOR_AI_PROC_STATE* _vendor_ai_state(UINT32 proc_id)
{
	if (proc_id >= g_ai_support_net_max) return 0;
	return &(fix_proc_state[proc_id]);
}

#if (NN_USER_HEAP == 1)
VENDOR_AI_NET_CFG_PROC* _vendor_ai_cfg(UINT32 proc_id)
{
	if (!g_ai_net_info[0].cfg) return 0;
	return &(g_ai_net_info[0].cfg[proc_id]);
}
#endif

VENDOR_AI_NET_INFO_PROC* _vendor_ai_info(UINT32 proc_id)
{
#if (NET_OPEN_HEAP == 1)
	if (!g_ai_net_info[0].proc) return 0;
	return (g_ai_net_info[0].proc[proc_id]);
#else
	if (!g_ai_net_info[0].proc) return 0;
	return &(g_ai_net_info[0].proc[proc_id]);
#endif
}

VENDOR_AI_COMMON_INFO* _vendor_ai_common_info(void)
{
	return &g_ai_comm_info_first[0];
}

extern HD_RESULT vendor_ai_net_reset_context (UINT32 proc_id);
extern HD_RESULT vendor_ai_net_reset_state (UINT32 proc_id);
extern HD_RESULT vendor_ais_set_ext_cb(UINT32 engine_id, VENDOR_AI_NET_PROC_CB fp);

//TODO: reset VENDOR_AI_COMMON_INFO to default
/*
static INT _vendor_ai_net_verify_param_CFG_EXT_CB(VENDOR_AI_ENGINE_PLUGIN *p_cfg_engine, CHAR *p_ret_string, INT max_str_len)
{
	if (p_cfg_engine->proc_cb == NULL) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_ENGINE: proc_cb is NULL.");
		goto exit;
	}

	return 0;
exit:
	return -1;
}
case VENDOR_AI_CFG_PLUGIN_ENGINE:
	ret = _vendor_ai_net_verify_param_CFG_EXT_CB((VENDOR_AI_ENGINE_PLUGIN*) p_param, p_ret_string, max_str_len);
	break;

*/

HD_RESULT _vendor_ai_dump_engine (void)
{
	HD_RESULT rv = HD_OK;
	INT32 i;

	//get dla engine config
	if (vendor_ai_dla_getinfo(VENDOR_AI_ENGINE_CNN) == HD_OK) {
		printf("AI-init: engine: CNN\r\n");
	}
	if (vendor_ai_dla_getinfo(VENDOR_AI_ENGINE_CNN2) == HD_OK) {
		printf("AI-init: engine: CNN2\r\n");
	}
	if (vendor_ai_dla_getinfo(VENDOR_AI_ENGINE_NUE) == HD_OK) {
		printf("AI-init: engine: NUE\r\n");
	}
	if (vendor_ai_dla_getinfo(VENDOR_AI_ENGINE_NUE2) == HD_OK) {
		printf("AI-init: engine: NUE2\r\n");
	}
	
	//get plugin engine config
	for (i=0; i<4;i++) {
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&g_ai_comm_info_first[0].engine_plug[i];
		UINT32 engine_id = i;
		if (p_cfg_engine->proc_cb != 0) {
			switch (engine_id) {
			case 0:
				// CPU
				DBG_DUMP("AI-init: plugin engine: CPU%d\r\n", (int)p_cfg_engine->eng);
				break;
			case 1:
				// DSP
				DBG_DUMP("AI-init: plugin engine: DSP%d\r\n", (int)p_cfg_engine->eng);
				break;
			default:
				break;
			}
		}
	}
	return rv;
}

#if (NET_OPEN_HEAP == 1)
HD_RESULT _vendor_ai_alloc_proc (UINT32 proc_id)
{
	g_ai_net_info[0].proc[proc_id] = (VENDOR_AI_NET_INFO_PROC*)vendor_ai_malloc(sizeof(VENDOR_AI_NET_INFO_PROC));
	if (g_ai_net_info[0].proc[proc_id] == NULL) {
		DBG_ERR("cannot alloc info for proc_id =%d\r\n", (int)proc_id);
		return HD_ERR_NOBUF;
	}
	memset(g_ai_net_info[0].proc[proc_id], 0, sizeof(VENDOR_AI_NET_INFO_PROC));
	{
			VENDOR_AI_NET_INFO_PROC *p_proc = g_ai_net_info[0].proc[proc_id];
			VENDOR_AI_CPU_TASK* p_cb_task = &(p_proc->cpu_task);
			p_cb_task->cpu_id = 0xff;
	}
	//printf("+++ alloc proc[%d] = %08x\r\n", (int)proc_id, (int)(g_ai_net_info[0].proc[proc_id]));
	return HD_OK;
}
void _vendor_ai_free_proc (UINT32 proc_id)
{
	if (g_ai_net_info[0].proc) {
		if (g_ai_net_info[0].proc[proc_id]) {
			//printf("+++ free proc[%d] = %08x\r\n", (int)proc_id, (int)(g_ai_net_info[0].proc[proc_id]));
			vendor_ai_free(g_ai_net_info[0].proc[proc_id], sizeof(VENDOR_AI_NET_INFO_PROC));
			g_ai_net_info[0].proc[proc_id] = 0;
		}
	}
}
#endif

HD_RESULT vendor_ai_init (void)
{
	UINT32 proc_id = 0;
	HD_RESULT rv = HD_OK;
	INT32 i;

	pthread_mutex_lock(&g_mutex);
	vendor_ai_net_trace(0xffffffff, AI_FLOW, "init() - begin\r\n");

	if (g_ai_comm_info[0].init_num == 0) {
		memcpy(g_ai_comm_info_first, g_ai_comm_info, sizeof(VENDOR_AI_COMMON_INFO));
		g_ai_comm_info[0].init_num += 1;
		g_ai_comm_info_first[0].init_num += 1;
	}
	else {
		if (memcmp(g_ai_comm_info_first, g_ai_comm_info, sizeof(VENDOR_AI_COMMON_INFO)) != 0) {
			DBG_ERR("Current setting of VENDOR_AI_CFG is not same as first setting !!\r\n");
			pthread_mutex_unlock(&g_mutex);
			return HD_ERR_NG;
		}
		if (g_ai_comm_info_first[0].is_multi == 0) {
			DBG_ERR("already init !!\n");
			pthread_mutex_unlock(&g_mutex);
			return HD_ERR_INIT;
		}
		else {
			g_ai_comm_info[0].init_num += 1;
			g_ai_comm_info_first[0].init_num += 1;
			pthread_mutex_unlock(&g_mutex);
			return HD_OK;
		}
	}

	if (g_ai_support_net_max == 0) {
		DBG_ERR("g_ai_support_net_max =0?\r\n");
		pthread_mutex_unlock(&g_mutex);
		return HD_ERR_NO_CONFIG;
	}

	if (!vendor_ais_check_component_ver()) {
		DBG_ERR("vendor_ais_check_component_ver fail\r\n");
		pthread_mutex_unlock(&g_mutex);
		return HD_ERR_NG;
	}

	//apply chk_interval
	if(g_ai_comm_info_first[0].chk_interval == 0) {
		vendor_ais_cfgchk(1000);
	}
	else if(g_ai_comm_info_first[0].chk_interval < 100) {
		DBG_WRN("AI-init: cfg chk_interval %d < 100? set to 100\r\n", (int)g_ai_comm_info_first[0].chk_interval);
		vendor_ais_cfgchk(100);
	}
	else if(g_ai_comm_info_first[0].chk_interval > 3000) {
		DBG_WRN("AI-init: cfg chk_interval %d > 3000? set to 3000\r\n", (int)g_ai_comm_info_first[0].chk_interval);
		vendor_ais_cfgchk(3000);
	}
	else {
		vendor_ais_cfgchk(g_ai_comm_info_first[0].chk_interval);
	}
	//printf("AI-init: chk_interval: %d\r\n", (int)g_ai_comm_info_first[0].chk_interval);
	

#if (NN_USER_HEAP == 1)
	g_ai_net_info[0].cfg = fix_proc_cfg;
	memset(g_ai_net_info[0].cfg, 0, sizeof(VENDOR_AI_NET_CFG_PROC)*MAX_PROC_CNT);
#if (NET_OPEN_HEAP == 1)
	g_ai_net_info[0].proc = fixed_proc_entry;
	memset(g_ai_net_info[0].proc, 0, sizeof(VENDOR_AI_NET_INFO_PROC*)*MAX_PROC_CNT);
#else
#endif
#endif

	rv = vendor_ais_init_all(); // update g_ai_support_net_max here
	if (rv != HD_OK) {
		pthread_mutex_unlock(&g_mutex);
		return rv;
	}

#if (NN_USER_HEAP == 1)
#if (NET_OPEN_HEAP == 1)
#else
	g_ai_net_info[0].proc = (VENDOR_AI_NET_INFO_PROC*)vendor_ai_malloc(sizeof(VENDOR_AI_NET_INFO_PROC)*g_ai_support_net_max);
	if (g_ai_net_info[0].proc == NULL) {
		DBG_ERR("cannot alloc heap for g_ai_support_net_max =%d\r\n", (int)g_ai_support_net_max);
		vendor_ais_uninit_all();
		pthread_mutex_unlock(&g_mutex);
		return HD_ERR_HEAP;
	}
	memset(g_ai_net_info[0].proc, 0, sizeof(VENDOR_AI_NET_INFO_PROC)*g_ai_support_net_max);
#endif
	for(proc_id = 0; proc_id < g_ai_support_net_max; proc_id++) {
		rv = vendor_ai_net_reset_context(proc_id);
	}
#endif

#if NET_TABLE_HEAP
	rv = vendor_ais_net_alloc_map_table_addr();
	if (rv != HD_OK) {
		vendor_ais_uninit_all();
		pthread_mutex_unlock(&g_mutex);
		return rv;
	}
#endif

	//apply global config
	for (i=0; i<4;i++) {
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&g_ai_comm_info_first[0].engine_plug[i];
		UINT32 engine_id = i;
		if (p_cfg_engine->proc_cb != 0) {
			switch (engine_id) {
			case 0:
				// CPU
				break;
			case 1:
				// DSP
				break;
			default:
				DBG_ERR("AI-init: plugin engine: not support type = %d!\r\n", (int)engine_id+1);
				continue;
				break;
			}
			//vendor_ai_net_trace(proc_id, AI_JOB, "set() - (ENGINE %d / CH %d) = 0x%08x\r\n", (int)engine_id, (int)p_user->ch, (UINT32)p_user->proc_cb);
			
			rv = vendor_ais_set_ext_cb(engine_id, p_cfg_engine->proc_cb); // register proc_cb to vendor_ai_net_flow
			if (rv != HD_OK) {
				DBG_DUMP("AI-init: plugin engine %d fail = %d\r\n", (int)engine_id+1, rv);
			}
		}
	}

	//apply schd
	switch (g_ai_comm_info_first[0].schd) {
	case VENDOR_AI_PROC_SCHD_FIFO:
		//printf("AI-init: proc scheduler: FIFO\r\n");
		break;
	case VENDOR_AI_PROC_SCHD_FAIR:
		//printf("AI-init: proc scheduler: FAIR\r\n");
		break;
	default:
		DBG_ERR("AI-init: proc scheduler => unknown type = %d!\r\n", (int)g_ai_comm_info_first[0].schd);
		g_ai_comm_info_first[0].schd = 0;
		//printf("AI-init: proc scheduler: FIFO (force)\r\n");
		break;
	}
	vendor_ais_cfgschd(g_ai_comm_info_first[0].schd);

	//init lib
#if (NN_USER_HEAP == 1)
#else
	g_ai_net_info[0].proc = (VENDOR_AI_NET_INFO_PROC*)vendor_ai_malloc(sizeof(VENDOR_AI_NET_INFO_PROC)*g_ai_support_net_max);
	if (g_ai_net_info[0].proc == NULL) {
		DBG_ERR("cannot alloc heap for g_ai_support_net_max =%d\r\n", (int)g_ai_support_net_max);
		vendor_ais_uninit_all();
		pthread_mutex_unlock(&g_mutex);
		return HD_ERR_HEAP;
	}
	for(proc_id = 0; proc_id < g_ai_support_net_max; proc_id++) {
		rv = vendor_ai_net_reset_context(proc_id);
	}
#endif

	vendor_ai_cmd_init(0);

	vendor_ai_net_trace(0xffffffff, AI_FLOW, "init() - end\r\n");

	pthread_mutex_unlock(&g_mutex);
	return rv;
}

HD_RESULT vendor_ai_uninit (void)
{
	UINT32 proc_id = 0;
	//UINT32 engine_id = 0;
	HD_RESULT rv = HD_OK;

	pthread_mutex_lock(&g_mutex);
	vendor_ai_net_trace(0xffffffff, AI_FLOW, "uninit() - begin\r\n");

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("NOT init yet !!\n");
		pthread_mutex_unlock(&g_mutex);
		return HD_ERR_UNINIT;
	}

	g_ai_comm_info[0].init_num -= 1;
	g_ai_comm_info_first[0].init_num -= 1;
	if(g_ai_comm_info[0].init_num != 0) {
		pthread_mutex_unlock(&g_mutex);
		return HD_OK;
	}

	vendor_ai_cmd_uninit(0);

	for(proc_id = 0; proc_id < g_ai_support_net_max; proc_id++) {
		rv = vendor_ai_net_reset_state(proc_id);
		if (rv != HD_OK) {
			pthread_mutex_unlock(&g_mutex);
			return rv;
		}
	}

	rv = vendor_ais_uninit_all();
	if (rv != HD_OK) {
		pthread_mutex_unlock(&g_mutex);
		return rv;
	}

	//for(engine_id = 0; engine_id < 2; engine_id++) {
	//	// reset extend engine
	//	rv = vendor_ais_set_ext_cb(engine_id, NULL);  // un-register ext_cb to vendor_ai_net_flow
	//}

#if NET_TABLE_HEAP
	vendor_ais_net_free_map_table_addr();
#endif

	//uninit lib
	{
#if (NN_USER_HEAP == 1)
		g_ai_net_info[0].cfg = NULL;
#if (NET_OPEN_HEAP == 1)
		for(proc_id = 0; proc_id < g_ai_support_net_max; proc_id++) {
			if (g_ai_net_info[0].proc[proc_id] != 0) {
				DBG_WRN("NOT close proc_id %d before uninit?\n", (int)proc_id);
				vendor_ai_free(g_ai_net_info[0].proc[proc_id], sizeof(VENDOR_AI_NET_INFO_PROC));
				g_ai_net_info[0].proc[proc_id] = 0;
			}
		}
		g_ai_net_info[0].proc = NULL;
#else
		vendor_ai_free(g_ai_net_info[0].proc, sizeof(VENDOR_AI_NET_INFO_PROC)*g_ai_support_net_max);
#endif
#else
		vendor_ai_free(g_ai_net_info[0].proc, sizeof(VENDOR_AI_NET_INFO_PROC)*g_ai_support_net_max);
#endif
		g_ai_net_info[0].proc = NULL;
	}

#if (NET_FD_HEAP == 1)
	vendor_ais_uninit_fd();
#endif

	vendor_ai_net_trace(0xffffffff, AI_FLOW, "uninit() - end\r\n");

	pthread_mutex_unlock(&g_mutex);

	return rv;
}

HD_RESULT vendor_ai_comm_lock(void)
{
	HD_RESULT rv = HD_OK;
	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("NOT init yet !!\n");
		return HD_ERR_UNINIT;
	}
	rv = vendor_ais_comm_lock();
	if (rv != HD_OK){
		if (rv == HD_ERR_STATE){
			DBG_ERR("Invalid state !!\n");
			return rv ;
		}else{
			DBG_ERR("Ictol failed !!\n");
			return rv ;
		}
	}
	return rv;
}

HD_RESULT vendor_ai_comm_unlock(void)
{
	HD_RESULT rv = HD_OK;
	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("NOT init yet !!\n");
		return HD_ERR_UNINIT;
	}
	rv = vendor_ais_comm_unlock();
	if (rv != HD_OK){
		if (rv == HD_ERR_STATE){
			DBG_ERR("Invalid state !!\n");
			return rv ;
		}else{
			DBG_ERR("Ictol failed !!\n");
			return rv ;
		}
	}
	return rv;
}

HD_RESULT vendor_ai_get_id (UINT32* proc_id)
{
	HD_RESULT rv = HD_OK;	
	
	if (proc_id == NULL) {
		DBG_ERR("give proc_id is NULL !!\n");
		return HD_ERR_USER;
	}

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("NOT init yet !!\n");
		return HD_ERR_UNINIT;
	}

	if (g_ai_support_net_max == 0) {
		DBG_ERR("g_ai_support_net_max =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	vendor_ai_net_trace(0xffffffff, AI_FLOW, "get_id() - begin\r\n");

	// find available proc_id
	
	vendor_ai_comm_lock();
	
	if (is_multi_process) {
	// open tmp fd[0]
		INT32 tmp_fd = 0;
		CHAR device[256] = {0};
		snprintf(device, 256, DEFAULT_DEVICE"%i", MAX_PROC_CNT + 1);
		tmp_fd = KFLOW_AI_OPEN(device, O_RDWR);
		if (tmp_fd == -1) {
			fprintf(stderr, "open tmp %s for ioctl failed, please insert kflow_ai_net.ko first!\r\n", device);
			vendor_ai_errno_location();
			rv = HD_ERR_NG;
			goto exit;
		}

		if (KFLOW_AI_IOCTL(tmp_fd, VENDOR_AIS_FLOW_IOC_GET_ID, proc_id) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_NET_RESET ioctl failed\r\n");
			vendor_ai_errno_location();
			rv =  HD_ERR_ABORT;
			goto exit;
		}

exit:
		// close tmp_fd[0]
		if (tmp_fd != -1) {
			//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
			if (KFLOW_AI_CLOSE(tmp_fd) < 0) {
				fprintf(stderr, "close tmp %s failed\r\n", DEFAULT_DEVICE);
				vendor_ai_errno_location();
				rv =  HD_ERR_NG;
			}
		}
	} else {
		UINT32 i = 0;
		VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
		for(i = 0; i < g_ai_support_net_max; i++) {
			p_cfg = _vendor_ai_cfg(i);
			if (p_cfg == NULL) {
				DBG_ERR("p_cfg is null for proc_id(%d) ...?\r\n", (int)i);
				vendor_ai_comm_unlock();
				return HD_ERR_NULL_PTR;
			}
			if (p_cfg->b_is_used == FALSE) {
				p_cfg->b_is_used = TRUE;
				break;
			}
		}
		*proc_id = i;
	}

	if (*proc_id < g_ai_support_net_max) {
		vendor_ai_net_trace(0xffffffff, AI_FLOW, "get_id() - proc_id(%d) is available.\r\n", (int)(*proc_id));
		//DBG_DUMP("get_id() - proc_id(%d) is available.\r\n", (int)i);
	} else {
		DBG_ERR("fail to get proc_id, all supported proc_id(0~%d) is in used !!\r\n", (int)(g_ai_support_net_max-1));
		vendor_ai_net_trace(0xffffffff, AI_FLOW, "get_id() - fail & end\r\n");
		vendor_ai_comm_unlock();
		return HD_ERR_RESOURCE;
	}
	
	vendor_ai_net_trace(0xffffffff, AI_FLOW, "get_id() - end\r\n");

	vendor_ai_comm_unlock();
	return rv;
}

HD_RESULT vendor_ai_release_id (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("NOT init yet !!\n");
		return HD_ERR_UNINIT;
	}

	if (g_ai_support_net_max == 0) {
		DBG_ERR("g_ai_support_net_max =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	vendor_ai_net_trace(0xffffffff, AI_FLOW, "release_id() - begin\r\n");

	// release given proc_id
	vendor_ai_comm_lock();
	if (is_multi_process) {
		// open tmp fd[0]
		INT32 tmp_fd = 0;
		CHAR device[256] = {0};
		snprintf(device, 256, DEFAULT_DEVICE"%i", MAX_PROC_CNT + 1);
		tmp_fd = KFLOW_AI_OPEN(device, O_RDWR);
		if (tmp_fd == -1) {
			fprintf(stderr, "open tmp %s for ioctl failed, please insert kflow_ai_net.ko first!\r\n", device);
			vendor_ai_errno_location();
			rv = HD_ERR_NG;
			goto exit;
		}

		if (KFLOW_AI_IOCTL(tmp_fd, VENDOR_AIS_FLOW_IOC_RELEASE_ID, &proc_id) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_NET_RESET ioctl failed\r\n");
			vendor_ai_errno_location();
			rv = HD_ERR_ABORT;
			goto exit;
		}
exit:
		// close tmp_fd[0]
		if (tmp_fd != -1) {
			//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
			if (KFLOW_AI_CLOSE(tmp_fd) < 0) {
				fprintf(stderr, "close tmp %s failed\r\n", DEFAULT_DEVICE);
				vendor_ai_errno_location();
				rv =  HD_ERR_NG;
			}
		}
	} else {
		VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
		p_cfg = _vendor_ai_cfg(proc_id);
		if (p_cfg == NULL) {
			DBG_ERR("p_cfg is null ...?\r\n");
			vendor_ai_comm_unlock();
			return HD_ERR_NULL_PTR;
		}
		if (p_cfg->b_is_used == FALSE) {
			DBG_ERR("fail to release proc_id(%d), it's NOT in used !!\r\n", (int)proc_id);
			vendor_ai_comm_unlock();
			return HD_ERR_USER;
		}
		p_cfg->b_is_used = FALSE;
	}

	vendor_ai_net_trace(0xffffffff, AI_FLOW, "release_id() - proc_id(%d) is released.\r\n", (int)proc_id);
	vendor_ai_net_trace(0xffffffff, AI_FLOW, "release_id() - end\r\n");

	vendor_ai_comm_unlock();
	return rv;
}

HD_RESULT vendor_ai_cfg_set (VENDOR_AI_CFG_ID cfg_id, void* p_param)
{
	HD_RESULT rv = HD_OK;

	switch ((UINT32)cfg_id) {
		case VENDOR_AI_CFG_PLUGIN_ENGINE:
			{
				VENDOR_AI_ENGINE_PLUGIN *p_user = (VENDOR_AI_ENGINE_PLUGIN *) p_param;
				VENDOR_AI_ENGINE_PLUGIN *p_cfg_engine;
				UINT32 engine_id;
				
				if (_vendor_ai_is_init() == TRUE && g_ai_comm_info_first[0].is_multi == 0) {
					DBG_ERR("already init!!\n");
					return HD_ERR_INIT;
				}
				
				if (p_user == 0) {
					DBG_ERR("cfg engine => null?\r\n");
					rv = HD_ERR_NULL_PTR; goto exit;
				}
				if (p_user->sign != MAKEFOURCC('A','E','N','G')) {
					DBG_ERR("cfg engine => invalid sign!\r\n");
					rv = HD_ERR_SIGN; goto exit;
				}
				switch (p_user->eng) {
				case 1:
					// CPU
					//DBG_DUMP("cfg engine => CPU%d\r\n", (int)p_user->ch+1);
					break;
				case 2:
					// DSP
					//DBG_DUMP("cfg engine => DSP%d\r\n", (int)p_user->ch+1);
					break;
				default:
					DBG_ERR("cfg engine => unknown type?\r\n");
					rv = HD_ERR_DEV; goto exit;
					break;
				}
				engine_id = p_user->eng-1;
				p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&g_ai_comm_info[0].engine_plug[engine_id];
				
				memcpy(p_cfg_engine, p_user, sizeof(VENDOR_AI_ENGINE_PLUGIN));
			}
			break;
			
		case VENDOR_AI_CFG_PROC_SCHD:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				UINT32 schd = 0;
				
				if (_vendor_ai_is_init() == TRUE && g_ai_comm_info_first[0].is_multi == 0) {
					DBG_ERR("already init!!\n");
					return HD_ERR_INIT;
				}
				
				if (p_user == 0) {
					return HD_ERR_PARAM;
				}
				schd = *p_user;
				switch (schd) {
				case VENDOR_AI_PROC_SCHD_FIFO:
					//printf("cfg scheduler: FIFO\r\n");
					g_ai_comm_info[0].schd = schd;
					break;
				case VENDOR_AI_PROC_SCHD_FAIR:
					//printf("cfg scheduler: FAIR\r\n");
					g_ai_comm_info[0].schd = schd;
					break;
				case VENDOR_AI_PROC_SCHD_CAPACITY:
					//printf("cfg scheduler: CAPACITY\r\n");
					g_ai_comm_info[0].schd = schd;
					break;
				default:
					DBG_ERR("cfg scheduler => unknown type?\r\n");
					rv = HD_ERR_DEV; goto exit;
					break;
				}
			}
			break;

		case VENDOR_AI_CFG_PROC_CHK_INTERVAL:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				UINT32 chk_interval = *p_user;
				
				if (_vendor_ai_is_init() == TRUE && g_ai_comm_info_first[0].is_multi == 0) {
					DBG_ERR("already init!!\n");
					return HD_ERR_INIT;
				}

				g_ai_comm_info[0].chk_interval = chk_interval;
			}
			break;

		case VENDOR_AI_CFG_PERF_UT:
			{
				if (_vendor_ai_is_init() == FALSE) {
					DBG_ERR("NOT init yet !!\n");
					return HD_ERR_UNINIT;
				}

				//printf("perf-begin\r\n");
				vendor_ai_dla_perf_ut(1, NULL);
			}
			break;
			

		case VENDOR_AI_CFG_SHAREMODEL_MODE:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				UINT32 share_mode = 0;
				if (_vendor_ai_is_init() == TRUE && g_ai_comm_info_first[0].is_multi == 0) {
					DBG_ERR("already init!! VENDOR_AI_CFG_SHAREMODEL_MODE could only be set before vendor_ai_init() !!\n");
					return HD_ERR_INIT;
				}
				if (p_user == 0) {
					DBG_ERR("p_param is NULL!!\n");
					return HD_ERR_PARAM;
				}
				share_mode = *p_user;
				if (FALSE == ((share_mode==0) || (share_mode==1) || (share_mode==2)) ) {
					DBG_ERR("invalid share model mode, should be [0/1/2], but set = %u\n", (unsigned int)share_mode);
					return HD_ERR_PARAM;
				}
				g_ai_comm_info[0].share_model_mode = share_mode;
			}
            break;

		case VENDOR_AI_CFG_MULTI_THREAD:
			{
				UINT32 *p_user = (UINT32 *) p_param;

				if (_vendor_ai_is_init() == TRUE && g_ai_comm_info_first[0].is_multi == 0) {
					DBG_ERR("already init!!\n");
					return HD_ERR_INIT;
				}

				if (p_user == 0) {
					DBG_ERR("cfg multiply init => null?\r\n");
					rv = HD_ERR_NULL_PTR; goto exit;
				}
				if (*p_user != 0 && *p_user != 1) {
					DBG_ERR("cfg multiply init => invalid value!\r\n");
					rv = HD_ERR_LIMIT; goto exit;
				}

				g_ai_comm_info[0].is_multi = *p_user;
			}
			break;

		case VENDOR_AI_CFG_MULTI_PROCESS:
			{
				UINT32 *p_user = (UINT32 *) p_param;

				if (_vendor_ai_is_init() == TRUE && g_ai_comm_info_first[0].is_multi == 0) {
					DBG_ERR("already init!!\n");
					return HD_ERR_INIT;
				}

				if (p_user == 0) {
					DBG_ERR("cfg multi-process => null?\r\n");
					rv = HD_ERR_NULL_PTR; goto exit;
				}
				if (*p_user != 0 && *p_user != 1) {
					DBG_ERR("cfg multi-process => invalid value!\r\n");
					rv = HD_ERR_LIMIT; goto exit;
				}

				is_multi_process = *p_user;
			}
			break;

		case VENDOR_AI_CFG_NOTCHECK_INPUT_ALIGN:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				UINT32 b_not_check = 0;
				if (_vendor_ai_is_init() == TRUE && g_ai_comm_info_first[0].is_multi == 0) {
					DBG_ERR("already init!! VENDOR_AI_CFG_NOTCHECK_INPUT_ALIGN could only be set before vendor_ai_init() !!\n");
					return HD_ERR_INIT;
				}
				if (p_user == 0) {
					DBG_ERR("p_param is NULL!!\n");
					return HD_ERR_PARAM;
				}
				b_not_check = *p_user;
				if (FALSE == ((b_not_check==0) || (b_not_check==1)) ) {
					DBG_ERR("invalid VENDOR_AI_CFG_NOTCHECK_INPUT_ALIGN parameter, should be [0/1], but set = %u\n", (unsigned int)b_not_check);
					return HD_ERR_PARAM;
				}
				g_ai_comm_info[0].not_check_input_align = b_not_check;
			}
			break;

		case 0xFFFF0000:
			{
				HD_RESULT ret = 0;
				ret = vendor_ais_dma_abort(); // dma_abort for cnn/nue/nue2
				if (ret != HD_OK) {
					DBG_ERR("dma abort failed...!!\r\n");
				}
			}
			break;

#if CNN_AI_FASTBOOT
		case VENDOR_AI_FBOOT_PROC_PERIOD:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				UINT32 frame_cnt = *p_user;
				rv = vendor_ais_set_fastboot_proc_period(frame_cnt);
				if (rv != HD_OK) {
					DBG_ERR("set fastboot proc period fail\n");
					return rv;
				}
			}
			break;
#endif

		default:
			break;
	}

exit:
	
	return rv;
}

VENDOR_AI_ENGINE_PLUGIN *vendor_ai_get_engine (UINT32 eng_id)
{
	if (eng_id >= 4) {
		return 0;
	}
	return (VENDOR_AI_ENGINE_PLUGIN *)&g_ai_comm_info_first[0].engine_plug[eng_id];
}

HD_RESULT vendor_ai_cfg_get (VENDOR_AI_CFG_ID cfg_id, void* p_param)
{
	HD_RESULT rv = HD_OK;

	switch ((UINT32)cfg_id) {
		case VENDOR_AI_CFG_PLUGIN_ENGINE:
			if (_vendor_ai_is_init() == FALSE) {
				DBG_ERR("NOT init yet !!\n");
				return HD_ERR_UNINIT;
			}
			
			{
				VENDOR_AI_ENGINE_PLUGIN *p_user = (VENDOR_AI_ENGINE_PLUGIN *) p_param;
				VENDOR_AI_ENGINE_PLUGIN *p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&g_ai_comm_info_first[0].engine_plug[0];
				memcpy(p_user, p_cfg_engine, sizeof(VENDOR_AI_ENGINE_PLUGIN));
			}
			break;
		case VENDOR_AI_CFG_PROC_SCHD:
			break;

#if CNN_AI_FASTBOOT
		case VENDOR_AI_FBOOT_RSLT:
			if (_vendor_ai_is_init() == FALSE) {
				DBG_ERR("NOT init yet !!\n");
				return HD_ERR_UNINIT;
			}
			
			{
				PD_RESULT_INFO *p_user = (PD_RESULT_INFO *) p_param;
				KFLOW_AI_BUILTIN_RSLT_INFO rslt_info = {0};
				UINT32 proc_id = 0, rslt_start_addr = 0;

				rv = vendor_ais_get_fastboot_rslt(&rslt_info);
				if (rv != HD_OK) {
					//DBG_ERR("get fastboot rslt empty!!!\n");
					return rv;
				}

				// get fboot rslt mem
				rslt_start_addr = vendor_ai_net_get_fastboot_rslt_start_addr(proc_id);

				// assign info
				p_user->sign = MAKEFOURCC('R','T','P','D');
				p_user->chunk_size = 0;
				p_user->result_num = rslt_info.out_num;
				p_user->p_result = (PD_RESULT *)(rslt_start_addr + rslt_info.out_result.va);
			}
			break;
#endif
		case VENDOR_AI_CFG_PROC_COUNT:
			if (_vendor_ai_is_init() == FALSE) {
				DBG_ERR("NOT init yet !!\n");
				return HD_ERR_UNINIT;
			}
			
			{
				VENDOR_AI_NET_CFG_PROC_COUNT *p_user = (VENDOR_AI_NET_CFG_PROC_COUNT *) p_param;
				if (p_user == NULL) {
					rv = HD_ERR_INV_PTR;
					return rv;
				}
				p_user->max_proc_count = g_ai_support_net_max;
			}
			break;

		case VENDOR_AI_CFG_IMPL_VERSION:
			if (_vendor_ai_is_init() == FALSE) {
				DBG_ERR("NOT init yet !!\n");
				return HD_ERR_UNINIT;
			}
			
			{
				VENDOR_AI_NET_CFG_IMPL_VERSION *p_user = (VENDOR_AI_NET_CFG_IMPL_VERSION *) p_param;
				if (p_user == NULL) {
					rv = HD_ERR_INV_PTR;
					return rv;
				}
				snprintf(p_user->vendor_ai_impl_version, VENDOR_AI_IMPL_VERSION_LENGTH, VENDOR_AI_IMPL_VERSION);
				vendor_ai_get_kflow_version(p_user->kflow_ai_impl_version);
				vendor_ai_get_kdrv_version(p_user->kdrv_ai_impl_version);
			}
			break;

		case VENDOR_AI_CFG_LIB_IMPL_VERSION:
			if (_vendor_ai_is_init() == FALSE) {
				DBG_ERR("NOT init yet !!\n");
				return HD_ERR_UNINIT;
			}
			
			{
				CHAR *p_user = (CHAR *) p_param;
				if (p_user == NULL) {
					rv = HD_ERR_INV_PTR;
					return rv;
				}
				snprintf(p_user, VENDOR_AI_IMPL_VERSION_LENGTH, VENDOR_AI_IMPL_VERSION);
			}
			break;

		case VENDOR_AI_CFG_KFLOW_IMPL_VERSION:
			if (_vendor_ai_is_init() == FALSE) {
				DBG_ERR("NOT init yet !!\n");
				return HD_ERR_UNINIT;
			}
			
			{
				CHAR *p_user = (CHAR *) p_param;
				if (p_user == NULL) {
					rv = HD_ERR_INV_PTR;
					return rv;
				}
				vendor_ai_get_kflow_version(p_user);
			}
			break;

		case VENDOR_AI_CFG_KDRV_IMPL_VERSION:
			if (_vendor_ai_is_init() == FALSE) {
				DBG_ERR("NOT init yet !!\n");
				return HD_ERR_UNINIT;
			}
			
			{
				CHAR *p_user = (CHAR *) p_param;
				if (p_user == NULL) {
					rv = HD_ERR_INV_PTR;
					return rv;
				}
				vendor_ai_get_kdrv_version(p_user);
			}
			break;

		case VENDOR_AI_CFG_PERF_UT:
			if (_vendor_ai_is_init() == FALSE) {
				DBG_ERR("NOT init yet !!\n");
				return HD_ERR_UNINIT;
			}

			{
				VENDOR_AI_PERF_UT *p_user = (VENDOR_AI_PERF_UT *) p_param;
				if (p_user == NULL) {
					rv = HD_ERR_INV_PTR;
					return rv;
				}
				//printf("perf-end\r\n");
				vendor_ai_dla_perf_ut(0, (VENDOR_AI_ENGINE_PERF_UT*)p_user);
			}
			break;

		case VENDOR_AI_CFG_SHAREMODEL_MODE:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				if (p_user == 0) {
					DBG_ERR("p_param is NULL!!\n");
					return HD_ERR_PARAM;
				}
				*p_user = g_ai_comm_info_first[0].share_model_mode;
			}
			break;

		case VENDOR_AI_CFG_NOTCHECK_INPUT_ALIGN:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				if (p_user == 0) {
					DBG_ERR("p_param is NULL!!\n");
					return HD_ERR_PARAM;
				}
				*p_user = g_ai_comm_info_first[0].not_check_input_align;
			}
			break;

		default:
			break;
	}

//exit:
	
	return rv;
}

