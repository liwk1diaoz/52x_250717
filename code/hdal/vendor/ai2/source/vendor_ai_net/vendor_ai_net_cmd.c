/**
	@brief Source file of vendor ai net cmd.

	@file vendor_ai_net_cmd.c

	@ingroup vendor_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
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
#include <stdio.h>
#include <string.h>
#include "hd_type.h"
#include "hd_common.h"
#include "kwrap/nvt_type.h"
#include "kwrap/error_no.h"
#include <kwrap/util.h> //for sleep API

#include "vendor_ai_net_debug.h"
#include "vendor_ai_net_flow.h"
#include "vendor_ai_net_gen.h"
#include "vendor_ai_net_group.h"
#include "vendor_ai_net_cmd.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "vendor_common.h" // for vendor_common_mem_cache_sync()
#include "vendor_ai_comm_flow.h"
#include "vendor_ai.h"
#include "vendor_ai_version.h"

//=============================================================
#define __CLASS__ 				"[ai][lib][cmd]"
#include "vendor_ai_debug.h"
//=============================================================

unsigned int vendor_ai_debug_level = NVT_DBG_ERR;

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define CMD_MAX_ARG_NUM		20

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct _PROC_CMD {
	char cmd[KFLOW_AI_MAX_CMD_LENGTH];
	HD_RESULT (*execute)(unsigned int proc_id, char* sub_cmd_name, char *cmd_args);
} PROC_CMD, *PPROC_CMD;

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/
//#define DBG_ERR(fmtstr, args...) printf("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
//#define DBG_WRN(fmtstr, args...) printf("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
//#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

HD_RESULT _vendor_ai_net_debug_dump(UINT32 proc_id, UINT32 space, UINT32 item, CHAR *filepath);
VOID vendor_ai_errno_location(VOID);
extern int _vendor_ai_net_mem_debug_set_preserve_buf(UINT32 buf_id);
/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
extern UINT32 g_ai_support_net_max;
static int g_ai_cmd_log = 1;
static pthread_t  _vendor_ai_cmd_callback_thread_id = 0;
static void *_vendor_ai_cmd_callback_thread(void *arg);
UINT32 vendor_ai_cmd_group_dump_bmp = 0;
UINT32 vendor_ai_cmd_iomem_dump_bmp[MAX_PROC_CNT] = {0};
extern INT32 *vendor_ais_flow_fd;

HD_RESULT vendor_ai_cmd_get_group_dump(unsigned int proc_id)
{
	return vendor_ai_cmd_group_dump_bmp;
}

HD_RESULT vendor_ai_cmd_set_group_dump(unsigned int proc_id, char* sub_cmd_name, char *cmd_args)
{
	HD_RESULT ret = HD_OK;
	CHAR full_path[CMD_MAX_PATH_LEN] = {0};

	if (proc_id > g_ai_support_net_max) {
		ret = HD_ERR_NG;
		goto exit;
	}
	if (sub_cmd_name == NULL) {
		ret = HD_ERR_NG;
		goto exit;
	}
	if (cmd_args == NULL) {
		ret = HD_ERR_NG;
		goto exit;
	}

	snprintf(full_path, CMD_MAX_PATH_LEN, "/mnt/mtd/%s.txt", sub_cmd_name);

	if (strcmp(sub_cmd_name, "dot_group") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_group_dump_bmp |= (VENDOR_AI_NET_CMD_DOT_GROUP);
		} else {
			vendor_ai_cmd_group_dump_bmp &= ~(VENDOR_AI_NET_CMD_DOT_GROUP);
		}
	}
	if (strcmp(sub_cmd_name, "mctrl_entry") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_group_dump_bmp |= (VENDOR_AI_NET_CMD_MCTRL_ENTRY);
		} else {
			vendor_ai_cmd_group_dump_bmp &= ~(VENDOR_AI_NET_CMD_MCTRL_ENTRY);
		}
	}
	if (strcmp(sub_cmd_name, "group") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_group_dump_bmp |= (VENDOR_AI_NET_CMD_GROUP);
		} else {
			vendor_ai_cmd_group_dump_bmp &= ~(VENDOR_AI_NET_CMD_GROUP);
		}
	}
	if (strcmp(sub_cmd_name, "mem_list") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_group_dump_bmp |= (VENDOR_AI_NET_CMD_MEM_LIST);
		} else {
			vendor_ai_cmd_group_dump_bmp &= ~(VENDOR_AI_NET_CMD_MEM_LIST);
		}
	}

exit:
	return ret;
}

HD_RESULT vendor_ai_cmd_set_flow_dump(unsigned int proc_id, char* sub_cmd_name, char *cmd_args)
{
	HD_RESULT ret = HD_OK;
	// 1. show api flow and parameters
	if (strcmp(sub_cmd_name, "trace") == 0) {
		UINT32 class_mask;
		sscanf(cmd_args, "%x", (unsigned int*)&class_mask);
		vendor_ai_net_set_trace_class(proc_id, class_mask);
	}
	/*
	// 2. show job model (id, gentoolver, ll/app, total jobs)
	if (strcmp(sub_cmd_name, "job_bin") == 0) {
		if (strncmp(cmd_args, "1", 2) == 0) {
		} else {
		}
	}
	// 3. show job rutnime option (job-opt, job-sync)
	if (strcmp(sub_cmd_name, "job_run") == 0) {
		if (strncmp(cmd_args, "1", 2) == 0) {
		} else {
		}
	}
	*/
	// 4. show flow perf (open time, proc time, close time)
	if (strcmp(sub_cmd_name, "perf") == 0) {
		UINT32 class_mask;
		sscanf(cmd_args, "%x", (unsigned int*)&class_mask);
		vendor_ai_net_set_trace_class(proc_id, class_mask);
	}
	return ret;
};

HD_RESULT vendor_ai_cmd_get_iomem_debug(unsigned int proc_id)
{
	return vendor_ai_cmd_iomem_dump_bmp[proc_id];
}

HD_RESULT vendor_ai_cmd_set_iomem_debug(unsigned int proc_id, char* sub_cmd_name, char *cmd_args)
{
	HD_RESULT ret = HD_OK;

	if (proc_id >= g_ai_support_net_max) {
		ret = HD_ERR_NG;
		goto exit;
	}
	if (sub_cmd_name == NULL) {
		ret = HD_ERR_NG;
		goto exit;
	}
	if (cmd_args == NULL) {
		ret = HD_ERR_NG;
		goto exit;
	}

	if (strcmp(sub_cmd_name, "dump") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] |= (VENDOR_AI_NET_CMD_IOMEM_DUMP_DEBUG);
		} else {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] &= ~(VENDOR_AI_NET_CMD_IOMEM_DUMP_DEBUG);
		}
	}
	if (strcmp(sub_cmd_name, "overlap") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] |= (VENDOR_AI_NET_CMD_IOMEM_CHK_OVERLAP);
		} else {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] &= ~(VENDOR_AI_NET_CMD_IOMEM_CHK_OVERLAP);
		}
	}
	if (strcmp(sub_cmd_name, "reorder_dump") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] |= (VENDOR_AI_NET_CMD_IOMEM_REORDER_DEBUG);
		} else {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] &= ~(VENDOR_AI_NET_CMD_IOMEM_REORDER_DEBUG);
		}
	}
	if (strcmp(sub_cmd_name, "sim_ai1_bug") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] |= (VENDOR_AI_NET_CMD_IOMEM_SIM_AI1_BUG);
		} else {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] &= ~(VENDOR_AI_NET_CMD_IOMEM_SIM_AI1_BUG);
		}
	}
	if (strcmp(sub_cmd_name, "shrink_dump") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] |= (VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG);
		} else {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] &= ~(VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG);
		}
	}
	if (strcmp(sub_cmd_name, "netinfo") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] |= (VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP);
		} else {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] &= ~(VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP);
		}
	}
	if (strcmp(sub_cmd_name, "clear_iobuf") == 0) {
		if (strncmp(cmd_args, "on", 2) == 0) {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] |= (VENDOR_AI_NET_CMD_IOMEM_CLEAR_IOBUF);
		} else {
			vendor_ai_cmd_iomem_dump_bmp[proc_id] &= ~(VENDOR_AI_NET_CMD_IOMEM_CLEAR_IOBUF);
		}
	}
	if (strcmp(sub_cmd_name, "set_preserve_bufid") == 0) {
		UINT32 buf_id=0;
		sscanf(cmd_args, "%lu", &buf_id);
		_vendor_ai_net_mem_debug_set_preserve_buf(buf_id);
	}
exit:
	return ret;
}

static PROC_CMD cmd_list[] = {
	// keyword      function name
	{ "group",		vendor_ai_cmd_set_group_dump    },
	{ "flow",		vendor_ai_cmd_set_flow_dump     },
	{ "iomem",		vendor_ai_cmd_set_iomem_debug   },
};

#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

HD_RESULT vendor_ai_cmd_parsing(CHAR *cmdstr)
{
	HD_RESULT ret = HD_OK;
	CHAR *cmd_line_res;
	const CHAR delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	CHAR *argv[CMD_MAX_ARG_NUM] = {0};
	UINT loop = 0;
	UINT proc_id = 0;

	// parse command string
	argv[0] = strsep(&cmdstr, delimiters);  // main command
	if (argv[0] == NULL) {
		goto exit;
	}
	argv[1] = strsep(&cmdstr, delimiters);  // sub  command
	if (argv[1] == NULL) {
		goto exit;
	}
	argv[2] = strsep(&cmdstr, delimiters);  // proc id
	if (argv[2] == NULL) {
		goto exit;
	}
	cmd_line_res = cmdstr;                  // command params

	sscanf(argv[2], "%u", &proc_id);
	if (proc_id > g_ai_support_net_max) {
		ret = HD_ERR_NG;
		goto exit;
	}

	DBG_DUMP("CMD(%s) argv(%s, %s, %s) res(%s)\n", cmdstr, argv[0], argv[1], argv[2], cmd_line_res);

	// dispatch command handler
	for (loop = 0 ; loop < NUM_OF_CMD; loop++) {
		if (strncmp(argv[0], cmd_list[loop].cmd, KFLOW_AI_MAX_CMD_LENGTH) == 0) {
			ret = cmd_list[loop].execute(proc_id, argv[1], cmd_line_res);
			break;
		}
	}
	if (loop >= NUM_OF_CMD) {
		ret = HD_ERR_NG;
		goto exit;
	}

exit:
	return ret;
}

static void *_vendor_ai_cmd_callback_thread(void *arg)
{
	INT r;
	KFLOW_AI_IOC_CMD_OUT cmd_out = {0};
	UINT32 proc_id = 0;

	if (vendor_ais_flow_fd[proc_id] == 0) {
		return 0;
	}

	while(g_ai_cmd_log) {
		// wait log cmd
		cmd_out.proc_id = proc_id;
		r = KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id], KFLOW_AI_IOC_CMD_OUT_WAI, &cmd_out);
		if(r < 0) {
			break; //quit now!
		}
		//DBG_DUMP("CMD(%s) proc_id(%lu)\n", cmd_out.str, cmd_out.proc_id);

		if (vendor_ai_cmd_parsing(cmd_out.str) != HD_OK) {
			DBG_ERR("command parsing fail, please check by \"cat /proc/kflow_ai/help\"\r\n");
			continue;
		}
		if (cmd_out.proc_id == 0xffffffff) {
			g_ai_cmd_log = 0; //quit cat info!
			break; //quit now!
		}
		if (g_ai_cmd_log) {
			cmd_out.proc_id = proc_id;
			// signal log cmd
			r = KFLOW_AI_IOCTL(vendor_ais_flow_fd[proc_id], KFLOW_AI_IOC_CMD_OUT_SIG, &cmd_out);
			if(r < 0) {
				break; //quit now!
			}
		}
	}
	return 0;
}

HD_RESULT vendor_ai_cmd_init(UINT32 net_id)
{
	INT ret;
	KFLOW_AI_IOC_CMD_OUT cmd_out = {0};
	KFLOW_AI_IOC_VERSION version = {0};
	KFLOW_AI_IOC_DEBUG_INFO debug_info = {0};
#if defined(__FREERTOS)
#else
	KFLOW_AI_IOC_DEBUG_LVL debug_lvl = {0};
#endif

	version.vendor_ai_version = VENDOR_AI_VERSION;
	snprintf(version.vendor_ai_impl_version, VENDOR_AI_VERSION_LEN, VENDOR_AI_IMPL_VERSION);
	ret = KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id], KFLOW_AI_IOC_SET_VERSION, &version);
	if(ret < 0) {
		DBG_ERR("KFLOW_AI_IOC_SET_VERSION fail\r\n");
		return HD_ERR_NG;
	}

#if defined(__FREERTOS)
    //lib vendor_ai_debug_level is the same variable on kflow, this "get" action is useless.
#else
	/* get debug lvl from kcmd of kflow_ai */
	ret = KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id], KFLOW_AI_IOC_GET_DEBUG_LVL, &debug_lvl);
	if(ret < 0) {
		DBG_ERR("KFLOW_AI_IOC_GET_DEBUG_LVL fail\r\n");
		return HD_ERR_NG;
	}
	vendor_ai_debug_level = debug_lvl.lvl;
#endif

	/* get debug setting from kcmd of kflow_ai */
	ret = KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id], KFLOW_AI_IOC_GET_DEBUG_INFO, &debug_info);
	if(ret < 0) {
		DBG_ERR("KFLOW_AI_IOC_GET_DEBUG_INFO fail\r\n");
		return HD_ERR_NG;
	}
	if (debug_info.kcmd_proc_id < g_ai_support_net_max) {
		if (debug_info.kcmd_proc_trace != 0) {
			vendor_ai_net_set_trace_class(debug_info.kcmd_proc_id, debug_info.kcmd_proc_trace);
		}
		vendor_ai_cmd_group_dump_bmp = debug_info.kcmd_group_dump_bmp;
		vendor_ai_cmd_iomem_dump_bmp[debug_info.kcmd_proc_id] = debug_info.kcmd_iomem_dump_bmp;
		if (debug_info.kcmd_set_preserve_bufid >= 0) {
			_vendor_ai_net_mem_debug_set_preserve_buf(debug_info.kcmd_set_preserve_bufid);
		}
	}

	ret = KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id], KFLOW_AI_IOC_CMD_OUT_INIT, &cmd_out);
	if(ret < 0) {
		DBG_ERR("KFLOW_AI_IOC_CMD_OUT_INIT fail\r\n");
		return HD_ERR_NG;
	}

	g_ai_cmd_log = 1;
	ret = pthread_create(&_vendor_ai_cmd_callback_thread_id, NULL, _vendor_ai_cmd_callback_thread, (void *)NULL);
	if (ret < 0) {
		//DBG_ERR("create cmd thread fail? %d\r\n", ret);
		return HD_ERR_FAIL;
	}
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#else
    {
        ret = pthread_setname_np(_vendor_ai_cmd_callback_thread_id, "ai_cmd_tsk");
    }
    if (ret < 0) {
        //DBG_ERR("name thread fail? %d\r\n", r);
        return HD_OK;
    }
#endif

	ret = KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id], KFLOW_AI_IOC_CMD_OUT_PROG_DEBUG, &cmd_out);
	if(ret < 0) {
		DBG_ERR("KFLOW_AI_IOC_CMD_OUT_PROG_DEBUG fail\r\n");
		return HD_ERR_NG;
	}

	return HD_OK;

}

HD_RESULT vendor_ai_cmd_uninit(UINT32 net_id)
{
	INT ret;
	KFLOW_AI_IOC_CMD_OUT cmd_out = {0};
	UINT32 delay_time = 0;

	if (vendor_ais_flow_fd[net_id] != -1) {
		ret = KFLOW_AI_IOCTL(vendor_ais_flow_fd[net_id], KFLOW_AI_IOC_CMD_OUT_UNINIT, &cmd_out);
		if(ret < 0) {
			DBG_ERR("KFLOW_AI_IOC_CMD_OUT_UNINIT fail\r\n");
		}
	}

	if (_vendor_ai_cmd_callback_thread_id == 0) {
		return HD_ERR_FAIL;
	}
	// destroy thread 
	while(g_ai_cmd_log == 1) {
		vos_util_delay_us(100);
		delay_time += 100;
		if(delay_time >= 3000000){
			DBG_ERR("_vendor_ai_cmd_callback_thread quit cat info timeout\r\n");
			return HD_ERR_TIMEDOUT;
		}
	} 
	pthread_join(_vendor_ai_cmd_callback_thread_id, NULL); 
	return HD_OK;
}
