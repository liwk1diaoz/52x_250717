/**
	@brief Source file of vendor net flow sample.

	@file kflow_ai_net_proc.c

	@ingroup kflow ai net proc file

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#if defined(__FREERTOS)
#include <string.h>         // for memset, strncmp
#include <stdio.h>          // sscanf
#include <malloc.h>
#include <kwrap/task.h>
#include <kwrap/file.h>
#include <kwrap/cmdsys.h>
#include <kwrap/sxcmd.h>
#endif

#include "kwrap/type.h"
///////////////////////////////////////////////////////////////
#define __CLASS__ 				"[ai][kflow][cmd]"
#include "kflow_ai_debug.h"
///////////////////////////////////////////////////////////////

#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/kflow_ai_core.h"

#include "kflow_cnn/kflow_cnn.h"
#include "kflow_nue/kflow_nue.h"
#include "kflow_nue2/kflow_nue2.h"
#include "kflow_cpu/kflow_cpu.h"
#include "kflow_dsp/kflow_dsp.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define KFLOW_AI_DEBUG_PROG		1
#define KFLOW_AI_DEBUG_RUN		2

#define KFLOW_AI_MAX_ARG_NUM    20
#define KFLOW_AI_WAIT_VALUE     0xffffffff

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct _PROC_CMD {
	char cmd[KFLOW_AI_MAX_CMD_LENGTH];
	int (*execute)(void* p_ctx, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
char output_path[STR_MAX_LENGTH] = DBG_OUT_PATH;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
void kflow_cmd_out_cb(UINT32 proc_id);
int kflow_cmd_out_run_debug(void);

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

static KFLOW_AI_IOC_CMD_OUT kflow_ai_ioc_cmd_out = {0};
static ID kflow_ai_cmd_out_wq;
static ID kflow_ai_cmd_out_wq2;
static UINT32 kflow_ai_cmd_out_init = 0;
static UINT32 kflow_ai_cmd_out_begin = 0;
static UINT32 kflow_ai_cmd_out_end = 0;
static UINT32 kflow_ai_cmd_out_pid = 0;

static UINT32 kflow_ai_cmd_out_debug_state = KFLOW_AI_DEBUG_RUN;
static ID kflow_ai_cmd_out_debug_wq;
static KFLOW_AI_MODEL_VERSION *gen_version;
static INT g_proc_init_cnt = 0;

#define FLG_ID_WQ_ALL		0xFFFFFFFF
#define FLG_ID_WQ_BEGIN		FLGPTN_BIT(0)
#define FLG_ID_WQ_END		FLGPTN_BIT(1)

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

BOOL kflow_ai_net_proc_info_show(unsigned char argc, char **argv)
{
	DBG_DUMP("debug_state(%#x)\r\n", kflow_ai_cmd_out_debug_state);
	DBG_DUMP("cmd_init(%#x) cmd_begin(%#x) cmd_end(%#x) cmd_pid(%#x)\r\n", kflow_ai_cmd_out_init,
			kflow_ai_cmd_out_begin, kflow_ai_cmd_out_end, kflow_ai_cmd_out_pid);
	DBG_DUMP("output path(%s)\r\n", output_path);
	return 0;
}

int kflow_ai_net_kcmd_flow_showhelp(unsigned char argc, char **argv)
{
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "flow");
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("%-25s : %s\r\n", "bind [proc_id]", "dump bind");
	DBG_DUMP("%-25s : %s\r\n", "schd [proc_id]", "dump schedule");
	DBG_DUMP("%-25s : %s\r\n", "ctx [proc_id]", "dump context");
	DBG_DUMP("%-25s : %s\r\n", "obuf [proc_id]", "dump output buffer");
	DBG_DUMP("%-25s : %s\r\n", "time [proc_id]", "dump execute time");
	DBG_DUMP("%-25s : %s\r\n", "timeline [proc_id]", "save proc timeline to html file");
	return 0;
}

int kflow_ai_net_kcmd_debug_showhelp(unsigned char argc, char **argv)
{
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "dump");
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("%-25s : %s\r\n", "prog", "program debug dump start");
	DBG_DUMP("%-25s : %s\r\n", "run", "program debug dump run");
	return 0;
}

int kflow_ai_net_cmd_group_showhelp(unsigned char argc, char **argv)
{
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "group");
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("%-25s : %s\r\n", "dot_group [proc_id] [on/off]", "dot output of group node");
	DBG_DUMP("%-25s : %s\r\n", "mctrl_entry [proc_id] [on/off]", "mctrl entry result");
	DBG_DUMP("%-25s : %s\r\n", "group [proc_id] [on/off]", "group result");
	DBG_DUMP("%-25s : %s\r\n", "meme_list [proc_id] [on/off]", "mem alloc/free result");
	return 0;
}

int kflow_ai_net_kcmd_outpath_showhelp(unsigned char argc, char **argv)
{
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "outpath");
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("%-25s : %s\r\n", "outpath [folder path]", "set dump path");
	return 0;
}

BOOL kflow_ai_net_proc_cmd_write(unsigned char argc, char **argv)
{
	int i = 0, len = 0, avail_len = KFLOW_AI_MAX_CMD_LENGTH - 1;
	int proc_id = 0;

	if (argc > KFLOW_AI_MAX_CMD_LENGTH) {
		argc = KFLOW_AI_MAX_CMD_LENGTH;
	}
	for (i = 0; i < argc; i++) {
		strncat(kflow_ai_ioc_cmd_out.str, argv[i], avail_len);
		len = strlen(kflow_ai_ioc_cmd_out.str);
		avail_len = avail_len - len;
		if (avail_len <= 1) {
			break;
		}
		strncat(kflow_ai_ioc_cmd_out.str, " ", avail_len);
		avail_len--;
	}
	kflow_cmd_out_cb(proc_id);
	memset(&kflow_ai_ioc_cmd_out, 0x0, sizeof(KFLOW_AI_IOC_CMD_OUT));

	return 0;
}

/* ============================================================================= */
/* proc "Kernel Command" file operation functions								 */
/* ============================================================================= */
int kflow_ai_net_kcmd_mem(void* p_ctx, unsigned char argc, char **argv)
{
	return 0;
}

int kflow_ai_net_kcmd_flow(void* p_ctx, unsigned char argc, char **argv)
{
	/*
	UINT32 proc_id;
	UINT32 mask;
	KFLOW_AI_NET* p_net = NULL;

	if (argc < 2)
		return -1;
	sscanf(argv[0], "%u", &proc_id);
	if (proc_id > g_ai_support_net_max) {
		return -1;
	}
	sscanf(argv[1], "%x", &mask);
	if (mask == 0) {
		return -1;
	}
	p_net = kflow_ai_core_net(proc_id);
	if (p_net == NULL) {
		return -1;
	}

	DBG_DUMP("=> flow begin: proc[%d] %04x\r\n", (int)proc_id, mask);
	kflow_ai_net_flow(p_net, mask); //dump flow log after proc
	*/
	UINT32 mask = 0;

	if (argc < 1)
		return -1;
	sscanf(argv[0], "%lx", &mask);

	if (mask == 0) {
		DBG_DUMP("=> flow end\r\n");
	} else {
		DBG_DUMP("=> flow begin: mask=%04x\r\n", mask);
	}
	kflow_ai_net_flow(mask); //dump flow log after proc
	return 0;
}

int kflow_ai_net_kcmd_dump(char* sub_cmd_name)
{
	if (sub_cmd_name == NULL) {
		DBG_DUMP("NULL sub_cmd_name\n");
		return -1;
	}
	if (strcmp(sub_cmd_name, "prog") == 0) {
		kflow_ai_cmd_out_debug_state = KFLOW_AI_DEBUG_PROG;
		DBG_DUMP("=> init() set break\n");
	} else if (strcmp(sub_cmd_name, "run") == 0) {
		kflow_cmd_out_run_debug();
		DBG_DUMP("=> init() continue\n");
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	return 0;
}

int kflow_ai_net_kcmd_version(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 proc_id;
	UINT16 fmt;
	UINT16 id;

	if (kflow_ai_cmd_out_init == 0) {
		return -1;
	}
	if (argc < 1) {
		return -1;
	}
	sscanf(argv[0], "%lu", &proc_id);
	if (proc_id >= g_ai_support_net_max) {
		return -1;
	}

	fmt = (UINT16)((gen_version[proc_id].nn_chip) >> 16);
	id = (UINT16)((gen_version[proc_id].nn_chip) & 0xFFFF);

	DBG_DUMP("NN_GEN_MODEL: proc[%d] fmt(%#08x) id(%#08x) gentool(%#08x) chip(%#08x)\r\n",
			(int)proc_id, fmt, id, gen_version[proc_id].gentool_vers, gen_version[proc_id].real_chip);

	return 0;
}

int kflow_ai_net_kcmd_outpath(void* p_ctx, unsigned char argc, char **argv)
{

	if (argc < 1) {
		return -1;
	}
	snprintf(output_path, STR_MAX_LENGTH-1, argv[0]);
	DBG_DUMP("Set output path: %s\r\n", output_path);

	kflow_cnn_set_output_path(output_path);
	kflow_cpu_set_output_path(output_path);
	kflow_dsp_set_output_path(output_path);
	kflow_nue_set_output_path(output_path);
	kflow_nue2_set_output_path(output_path);

	return 0;
}

static PROC_CMD kcmd_list[] = {
	// keyword      function name
	{ "mem",		kflow_ai_net_kcmd_mem    },
	{ "flow",		kflow_ai_net_kcmd_flow   },
	{ "version",	kflow_ai_net_kcmd_version	 },
	{ "outpath",	kflow_ai_net_kcmd_outpath	 },
};

BOOL kflow_ai_net_proc_kcmd_show(unsigned char argc, char **argv)
{
	return 0;
}

#define NUM_OF_CMD (sizeof(kcmd_list) / sizeof(PROC_CMD))

BOOL kflow_ai_net_proc_kcmd_write(unsigned char argc, char **argv)
{
	int ret = 0;
	unsigned char loop;
	unsigned int proc_id = 0;

	if (argc < 3) {
		goto ERR_INVALID_CMD;
	}

	if (strncmp(argv[0], "dump", KFLOW_AI_MAX_CMD_LENGTH) == 0) {
		ret = kflow_ai_net_kcmd_dump(argv[1]);
	} else {
		sscanf(argv[2], "%u", &proc_id);
		if (proc_id > g_ai_support_net_max) {
			goto ERR_INVALID_CMD;
		}

		// dispatch command handler
		for (loop = 0 ; loop < NUM_OF_CMD; loop++) {
			if (strncmp(argv[0], kcmd_list[loop].cmd, KFLOW_AI_MAX_CMD_LENGTH) == 0) {
				ret = kcmd_list[loop].execute(0, argc, &argv[1]);
				break;
			}
		}
		if (loop >= NUM_OF_CMD) {
			goto ERR_INVALID_CMD;
		}
	}

	return ret;

ERR_INVALID_CMD:
	DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/kflow_ai/help\" for help.\r\n");
	return -1;
}

BOOL kflow_ai_net_proc_help_show(unsigned char argc, char **argv)
{
	DBG_DUMP("\n\n1. 'cat /proc/kflow_ai/info' will show all the kflow_ai info\n");
	DBG_DUMP("2. 'echo [proc_id] xxx > /proc/kflow_ai/kcmd' can input command for some debug purpose\n");
	DBG_DUMP("   where \"xxx\" is as following ....\n\n");

	kflow_ai_net_kcmd_flow_showhelp(argc, argv);
	kflow_ai_net_kcmd_debug_showhelp(argc, argv);
	kflow_ai_net_kcmd_outpath_showhelp(argc, argv);

	DBG_DUMP("\n\n3. 'echo [proc_id] xxx > /proc/kflow_ai/cmd' can input command for some debug purpose\n");
	DBG_DUMP("   where \"xxx\" is as following ....\n\n");
	kflow_ai_net_cmd_group_showhelp(argc, argv);

	return 0;
}

int kflow_cmd_out_init(void)
{
	kflow_ai_cmd_out_begin = KFLOW_AI_WAIT_VALUE;
	kflow_ai_cmd_out_end = KFLOW_AI_WAIT_VALUE;
	kflow_ai_cmd_out_pid = KFLOW_AI_WAIT_VALUE;
	OS_CONFIG_FLAG(kflow_ai_cmd_out_wq);
	OS_CONFIG_FLAG(kflow_ai_cmd_out_wq2);
	clr_flg(kflow_ai_cmd_out_wq, FLG_ID_WQ_ALL);
	clr_flg(kflow_ai_cmd_out_wq2, FLG_ID_WQ_ALL);
	kflow_ai_cmd_out_init = 1;
	return 1;
}
int kflow_cmd_out_uninit(void)
{
	if(!kflow_ai_cmd_out_init) {
		return 0;
	}

	kflow_ai_cmd_out_begin = KFLOW_AI_WAIT_VALUE;
	kflow_ai_cmd_out_end = KFLOW_AI_WAIT_VALUE;
	kflow_ai_cmd_out_pid = KFLOW_AI_WAIT_VALUE;
	set_flg(kflow_ai_cmd_out_wq2, FLG_ID_WQ_BEGIN);
	set_flg(kflow_ai_cmd_out_wq, FLG_ID_WQ_BEGIN);
	rel_flg(kflow_ai_cmd_out_wq2);
	rel_flg(kflow_ai_cmd_out_wq);
	kflow_ai_cmd_out_init = 0;
	return 1;
}

void kflow_cmd_out_cb(UINT32 proc_id)
{
	FLGPTN flag = 0;

	if(!kflow_ai_cmd_out_init) {
		return;
	}
	kflow_ai_cmd_out_begin = proc_id;
	set_flg(kflow_ai_cmd_out_wq, FLG_ID_WQ_BEGIN);
	///.....
	wai_flg(&flag, kflow_ai_cmd_out_wq2, FLG_ID_WQ_END, TWF_ORW|TWF_CLR);
	kflow_ai_cmd_out_end = KFLOW_AI_WAIT_VALUE;
}

int kflow_ai_cmd_out_wait(void)
{
	FLGPTN flag = 0;

	if(!kflow_ai_cmd_out_init) {
		return KFLOW_AI_WAIT_VALUE;
	}
	wai_flg(&flag, kflow_ai_cmd_out_wq, FLG_ID_WQ_END, TWF_ORW|TWF_CLR);
	if (kflow_ai_cmd_out_end == 0) { // kflow_cmd_out_uninit was called
		return KFLOW_AI_WAIT_VALUE;
	}
	kflow_ai_cmd_out_pid = kflow_ai_cmd_out_begin;
	kflow_ai_cmd_out_begin = KFLOW_AI_WAIT_VALUE;
	return kflow_ai_cmd_out_pid;
}

void kflow_ai_cmd_out_sig(UINT32 proc_id)
{
	if(!kflow_ai_cmd_out_init) {
		return;
	}
	kflow_ai_cmd_out_end = proc_id;
	set_flg(kflow_ai_cmd_out_wq2, FLG_ID_WQ_BEGIN);
	kflow_ai_cmd_out_pid = KFLOW_AI_WAIT_VALUE;
}

int kflow_cmd_out_prog_debug(void)
{
	FLGPTN flag = 0;

	if (kflow_ai_cmd_out_debug_state == KFLOW_AI_DEBUG_PROG) {
		DBG_DUMP("=> init() break\n");
		wai_flg(&flag, kflow_ai_cmd_out_debug_wq, FLG_ID_WQ_END, TWF_ORW|TWF_CLR);
	}
	return 0;
}

int kflow_cmd_out_run_debug(void)
{
	int proc_id = 0;

	kflow_ai_cmd_out_debug_state = KFLOW_AI_DEBUG_RUN;
	set_flg(kflow_ai_cmd_out_debug_wq, FLG_ID_WQ_BEGIN);
	kflow_ai_cmd_out_begin = proc_id;
	set_flg(kflow_ai_cmd_out_wq, FLG_ID_WQ_BEGIN);
	kflow_ai_cmd_out_end = proc_id;
	set_flg(kflow_ai_cmd_out_wq2, FLG_ID_WQ_BEGIN);
	return 0;
}

int kflow_ai_set_gen_version(VENDOR_AIS_FLOW_VERS *p_vers_info, UINT32 chip_id)
{
	UINT32 idx = 0;

	if (p_vers_info == NULL) {
		return -1;
	}

	idx = p_vers_info->proc_id;
	if (idx >= g_ai_support_net_max) {
		return -1;
	}

	gen_version[idx].proc_id = idx;
	gen_version[idx].nn_chip = p_vers_info->chip_id;
	gen_version[idx].gentool_vers = p_vers_info->gentool_vers;
	gen_version[idx].real_chip = chip_id;

	return 0;
}

int kflow_ai_net_ioctl(int fd, unsigned int cmd, void *arg)
{
	switch (cmd) {
	case KFLOW_AI_IOC_CMD_OUT_INIT: {
		kflow_cmd_out_init();
	}
	break;

	case KFLOW_AI_IOC_CMD_OUT_UNINIT: {
		kflow_cmd_out_uninit();
	}
	break;

	case KFLOW_AI_IOC_CMD_OUT_PROG_DEBUG: {
		kflow_cmd_out_prog_debug();
	}
	break;

	case KFLOW_AI_IOC_CMD_OUT_RUN_DEBUG: {
		kflow_cmd_out_run_debug();
	}
	break;

	case KFLOW_AI_IOC_CMD_OUT_WAI: {
			KFLOW_AI_IOC_CMD_OUT *p_cmd = &kflow_ai_ioc_cmd_out;
			memcpy((void *)p_cmd, (void *)arg, sizeof(KFLOW_AI_IOC_CMD_OUT));
			p_cmd->proc_id = kflow_ai_cmd_out_wait();
			memcpy((void *)arg, p_cmd, sizeof(KFLOW_AI_IOC_CMD_OUT));
		}
		break;

	case KFLOW_AI_IOC_CMD_OUT_SIG: {
			KFLOW_AI_IOC_CMD_OUT *p_cmd = &kflow_ai_ioc_cmd_out;
			memcpy((void *)p_cmd, (void *)arg, sizeof(KFLOW_AI_IOC_CMD_OUT));
			kflow_ai_cmd_out_sig(p_cmd->proc_id);
		}
		break;

	default :
		break;
	}
	return 0;
}

static SXCMD_BEGIN(kflow_ai_cmd_tbl, "kflow_ai")
SXCMD_ITEM("info",        kflow_ai_net_proc_info_show,  "dump info")
SXCMD_ITEM("cmd %",       kflow_ai_net_proc_cmd_write,  "write cmd to lib")
SXCMD_ITEM("kcmd %",      kflow_ai_net_proc_kcmd_write, "write cmd to driver")
SXCMD_ITEM("help",        kflow_ai_net_proc_help_show,  "dump help")
SXCMD_END()

void kflow_ai_install_cmd(void)
{
	OS_CONFIG_FLAG(kflow_ai_cmd_out_debug_wq);
	clr_flg(kflow_ai_cmd_out_debug_wq, FLG_ID_WQ_ALL);
	sxcmd_addtable(kflow_ai_cmd_tbl);
}

void kflow_ai_uninstall_cmd(void)
{

}

MAINFUNC_ENTRY(kflow_ai, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(kflow_ai_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		kflow_ai_net_proc_help_show(argc, argv);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], kflow_ai_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = kflow_ai_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		kflow_ai_net_proc_help_show(argc, argv);
		return -1;
	}
	return 0;
}

int kflow_ai_net_proc_init(VOID)
{
	if (g_proc_init_cnt == 0) {
		gen_version = (KFLOW_AI_MODEL_VERSION *)malloc(sizeof(KFLOW_AI_MODEL_VERSION) * g_ai_support_net_max);
		if (gen_version == NULL) {
			return E_NOMEM;
		}
		memset(gen_version, 0x0, sizeof(KFLOW_AI_MODEL_VERSION) * g_ai_support_net_max);
		g_proc_init_cnt = 1;
	}
	return E_OK;
}

int kflow_ai_net_proc_uninit(VOID)
{
	if (g_proc_init_cnt) {
		if (gen_version) {
			free(gen_version);
		}
		g_proc_init_cnt = 0;
	}
	return E_OK;
}
