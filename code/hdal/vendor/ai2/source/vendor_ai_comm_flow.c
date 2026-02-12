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
#include "vendor_ai_comm_flow.h"
#include "vendor_ai.h"
#include "vendor_ai_net/vendor_ai_net_group.h"
//#include "vendor_ai_net_flow.h"
//#include "vendor_ai_net_gen.h"

#include "kwrap/platform.h"

#if defined(__LINUX)
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

//=============================================================
#define __CLASS__ 				"[ai][lib][comm]"
#include "vendor_ai_debug.h"
//=============================================================

extern UINT32 g_ai_support_net_max;
extern BOOL is_multi_process;
#if (NET_FD_HEAP == 1)
static INT32 fix_flow_fd[MAX_PROC_CNT+1] = {0};
#endif
extern INT32 *vendor_ais_flow_fd;

static UINT32 vendor_ais_init_cnt = 0;
static UINT32 vendor_ais_init_chk = 0;
static VENDOR_AIS_MEM_INFO g_mem_info = {0};
#if defined(_BSP_NA51055_)
UINT32 chip_old = 0; 
#endif

VOID vendor_ai_errno_location(VOID)
{
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	return;
#else
	fprintf(stderr, "vendor_ai_errno_location: errno = %d\r\n", errno);
	fprintf(stderr, "vendor_ai_errno_location: errno string = %s\r\n", strerror(errno));
	return;
#endif
}


VOID* vendor_ai_malloc(size_t size)
{
	VOID* ptr = malloc(size);
	if (ptr != 0) {
		g_mem_info.value[0] += size;
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_MEM_INFO, &g_mem_info) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_MEM_INFO ioctl failed\r\n");
			vendor_ai_errno_location();
		}
	}
	return ptr;
}

VOID vendor_ai_free(VOID* ptr, size_t size)
{
	if (ptr != 0) {
		g_mem_info.value[0] -= size;
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_MEM_INFO, &g_mem_info) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_MEM_INFO ioctl failed\r\n");
			vendor_ai_errno_location();
		}
	}
	free(ptr);
}

#if (NN_USER_HEAP == 1)
HD_RESULT vendor_ais_get_net_max(UINT32 *p_support_net_max)
{
	INT32 tmp_fd = 0;
	VENDOR_AIS_FLOW_ID id_info = {0};
	HD_RESULT ret = HD_OK;

	// open tmp fd[0]
	CHAR device[256] = {0};
	snprintf(device, 256, DEFAULT_DEVICE"%i", MAX_PROC_CNT + 1);
	tmp_fd = KFLOW_AI_OPEN(device, O_RDWR);
	if (tmp_fd == -1) {
		fprintf(stderr, "open tmp %s for ioctl failed, please insert kflow_ai_net.ko first!\r\n", device);
		vendor_ai_errno_location();
		ret = HD_ERR_NG;
		goto exit2;
	}
	
	if (KFLOW_AI_IOCTL(tmp_fd, VENDOR_AIS_FLOW_IOC_GET_NUM, &id_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_GET_NUM ioctl failed\r\n");
		vendor_ai_errno_location();
		ret =  HD_ERR_ABORT;
		goto exit2;
	}

	*p_support_net_max = id_info.ai_support_net_max;
	if(*p_support_net_max < 1) {
		fprintf(stderr, "ai_support_net_max is ZERO.\r\n");
		ret =  HD_ERR_ABORT;
		goto exit2;
	}
exit2:
	// close tmp_fd[0]
	if (tmp_fd != -1) {
		//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
		if (KFLOW_AI_CLOSE(tmp_fd) < 0) {
			fprintf(stderr, "close tmp %s failed\r\n", DEFAULT_DEVICE);
			vendor_ai_errno_location();
			ret =  HD_ERR_NG;
		}
	}
	return ret;
}
#endif

HD_RESULT _vendor_ai_set_is_multi_process(VOID)
{
	INT32 tmp_fd = 0;
	HD_RESULT ret = HD_OK;

	// open tmp fd[0]
	CHAR device[256] = {0};
	snprintf(device, 256, DEFAULT_DEVICE"%i", MAX_PROC_CNT + 1);
	tmp_fd = KFLOW_AI_OPEN(device, O_RDWR);
	if (tmp_fd == -1) {
		fprintf(stderr, "open tmp %s for ioctl failed, please insert kflow_ai_net.ko first!\r\n", DEFAULT_DEVICE);
		vendor_ai_errno_location();
		ret = HD_ERR_NG;
		goto exit;
	}

	if (KFLOW_AI_IOCTL(tmp_fd, VENDOR_AIS_FLOW_IOC_MULTI_PROCESS, &is_multi_process) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_MULTI_PROCESS ioctl failed\r\n");
		vendor_ai_errno_location();
		ret =  HD_ERR_ABORT;
		goto exit;
	}

exit:
	// close tmp_fd[0]
	if (tmp_fd != -1) {
		//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
		if (KFLOW_AI_CLOSE(tmp_fd) < 0) {
			fprintf(stderr, "close tmp %s failed\r\n", device);
			vendor_ai_errno_location();
			ret =  HD_ERR_NG;
		}
	}
	return ret;
}

HD_RESULT _vendor_ai_query_support_net_max(UINT32 *p_support_net_max)
{
	//INT32 tmp_fd = 0;
	VENDOR_AIS_FLOW_ID id_info = {0};
	HD_RESULT ret = HD_OK;

	// open tmp fd[0]
	/*
	tmp_fd = KFLOW_AI_OPEN(DEFAULT_DEVICE, O_RDWR);
	if (tmp_fd == -1) {
		fprintf(stderr, "open tmp %s for ioctl failed, please insert kflow_ai_net.ko first!\r\n", DEFAULT_DEVICE);
		vendor_ai_errno_location();
		ret = HD_ERR_NG;
		goto exit;
	}
	*/

	// query support_net_max from kdrv_ai
	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_NET_RESET, &id_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_NET_RESET ioctl failed\r\n");
		vendor_ai_errno_location();
		ret =  HD_ERR_ABORT;
		goto exit;
	}

	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_NET_INIT, &id_info) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_NET_INIT ioctl failed\r\n");
		vendor_ai_errno_location();
		ret =  HD_ERR_ABORT;
		goto exit;
	}
	
	*p_support_net_max = id_info.ai_support_net_max;
	if(*p_support_net_max < 1) {
		fprintf(stderr, "ai_support_net_max is ZERO.\r\n");
		ret =  HD_ERR_ABORT;
		goto exit;
	}
	if (*p_support_net_max > AI_SUPPORT_NET_MAX) {
		fprintf(stderr, "ai_support_net_max(%ld) > AI_SUPPORT_NET_MAX(%d)\r\n", *p_support_net_max, AI_SUPPORT_NET_MAX);
	}

exit:
	// close tmp_fd[0]
	/*
	if (tmp_fd != -1) {
		//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
		if (KFLOW_AI_CLOSE(tmp_fd) < 0) {
			fprintf(stderr, "close tmp %s failed\r\n", DEFAULT_DEVICE);
			vendor_ai_errno_location();
			ret =  HD_ERR_NG;
		}
	}
	*/
	return ret;
}

HD_RESULT vendor_ais_cfgchk(UINT32 chk_interval)
{
	vendor_ais_init_chk = chk_interval;
	return HD_OK;
}

HD_RESULT vendor_ais_init_all(void)
{
	if (vendor_ais_init_cnt != 0)
		return HD_ERR_STATE;

	if (g_ai_support_net_max == 0) {
		fprintf(stderr, "g_ai_support_net_max is ZERO\r\n");
		return HD_ERR_ABORT;
	}

#if (NET_FD_HEAP == 1)
	vendor_ais_flow_fd = fix_flow_fd;
	memset(vendor_ais_flow_fd, -1, sizeof(INT32) * (MAX_PROC_CNT+1)); // should "+1" for fd[0] as ctrl_path
#else
	vendor_ais_flow_fd = (INT32 *)vendor_ai_malloc(sizeof(INT32) * (g_ai_support_net_max+1)); // should "+1" for fd[0] as ctrl_path
	if (vendor_ais_flow_fd == NULL) {
		fprintf(stderr, "vendor_ais_flow_fd malloc fail\r\n");
		return HD_ERR_NOMEM;
	}
	memset(vendor_ais_flow_fd, -1, sizeof(INT32) * (g_ai_support_net_max+1)); // should "+1" for fd[0] as ctrl_path
#endif

	if (_vendor_ai_set_is_multi_process() != HD_OK) {
		fprintf(stderr, "_vendor_ai_set_is_multi_process fail\r\n");
		return HD_ERR_NG;
	}

	// ai net init
	vendor_ais_flow_fd[0] = KFLOW_AI_OPEN(DEFAULT_DEVICE, O_RDWR);
	if (vendor_ais_flow_fd[0] == -1) {
		fprintf(stderr, "open %s for ioctl failed\r\n", DEFAULT_DEVICE);
		vendor_ai_errno_location();
		return HD_ERR_NG;
	}

	// query support_net_max
	if (_vendor_ai_query_support_net_max(&g_ai_support_net_max) != HD_OK) {
		fprintf(stderr, "query support_net_max fail\r\n");
		return HD_ERR_NG;
	}

	if (vendor_ais_flow_fd[0] != -1) {
		//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
		VENDOR_AIS_FLOW_CORE_CFG cfg;
		cfg.schd = vendor_ais_init_chk;
		
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_CORE_CFGCHK, &cfg) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CORE_CFGCHK fail\r\n");
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}

	vendor_ais_init_net();

	vendor_ai_net_group_init();
	
	vendor_ai_dla_reset();
	
	// ai core init
	vendor_ai_dla_init();

	vendor_ais_init_cnt = 1;
	return HD_OK;
}

HD_RESULT vendor_ais_uninit_all(void)
{
	if (vendor_ais_init_cnt != 1)
		return HD_ERR_STATE;

	// ai core uninit
	vendor_ai_dla_uninit();

	vendor_ai_net_group_uninit();

	vendor_ais_uninit_net();
	
#if (NET_FD_HEAP == 1)
		//vendor_ais_flow_fd = 0;
#else
	// ai net uninit
	if (vendor_ais_flow_fd[0] != -1) {
		//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
		if (KFLOW_AI_CLOSE(vendor_ais_flow_fd[0]) < 0) {
			fprintf(stderr, "close %s failed\r\n", DEFAULT_DEVICE);
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}

	if (vendor_ais_flow_fd) {
		vendor_ai_free(vendor_ais_flow_fd, sizeof(INT32) * (g_ai_support_net_max+1)); // should "+1" for fd[0] as ctrl_path
	}
#endif

	vendor_ais_init_cnt = 0;
	return HD_OK;
}

#if (NET_FD_HEAP == 1)
HD_RESULT vendor_ais_uninit_fd(void)
{
	// ai net uninit
	if (vendor_ais_flow_fd[0] != -1) {
		//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
		if (KFLOW_AI_CLOSE(vendor_ais_flow_fd[0]) < 0) {
			fprintf(stderr, "close %s failed\r\n", DEFAULT_DEVICE);
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}
	
	vendor_ais_flow_fd = 0;
	return HD_OK;
}
#endif

HD_RESULT vendor_ais_cfgschd(UINT32 schd)
{
	if (vendor_ais_init_cnt != 1)
		return HD_ERR_STATE;

	//TODO: check all proc are not start!
	//if (vendor_ais_init_cnt != 1)
	//	return HD_ERR_STATE;

	if (vendor_ais_flow_fd[0] != -1) {
		//DBG_DUMP("close %s\r\n", DEFAULT_DEVICE);
		VENDOR_AIS_FLOW_CORE_CFG cfg;
		cfg.schd = schd;
		if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_CORE_CFGSCHD, &cfg) < 0) {
			fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CORE_CFGSCHD fail\r\n");
			vendor_ai_errno_location();
			return HD_ERR_NG;
		}
	}

	return HD_OK;
}
#if defined(_BSP_NA51055_)	
HD_RESULT _vendor_ai_query_chip_old(UINT32 *chip_old)
{
	//INT32 tmp_fd = 0;

	HD_RESULT ret = HD_OK;

	

	if (KFLOW_AI_IOCTL(vendor_ais_flow_fd[0], VENDOR_AIS_FLOW_IOC_CHIP_OLD, chip_old) < 0) {
		fprintf(stderr, "VENDOR_AIS_FLOW_IOC_CHIP_OLD ioctl failed\r\n");
		vendor_ai_errno_location();
		ret =  HD_ERR_ABORT;
		goto exit;
	}
exit:
	
	return ret;
}
#endif	




