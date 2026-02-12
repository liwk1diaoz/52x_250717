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
#else
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <stdarg.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <kwrap/dev.h>
#include <linux/vmalloc.h>
#endif

#include "kwrap/type.h"
//=============================================================
#define __CLASS__ 				"[ai][kflow][cmd]"
#include "kflow_ai_debug.h"
//=============================================================

#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/kflow_ai_core.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"
#include "kflow_ai_net_proc.h"
#include "kdrv_ai_version.h"
#include "kflow_ai_version.h"

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
#if CNN_AI_FASTBOOT
extern UINT32 kflow_ai_net_fboot_dump_en;
#endif
extern UINT32 g_ai_support_net_max;

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
static struct proc_dir_entry *kflow_ai_net_proc;
static struct proc_dir_entry *kflow_ai_net_proc_info_proc;
static struct proc_dir_entry *kflow_ai_net_proc_ut_proc;
static struct proc_dir_entry *kflow_ai_net_proc_cmd_proc;
static struct proc_dir_entry *kflow_ai_net_proc_kcmd_proc;
static struct proc_dir_entry *kflow_ai_net_proc_help_proc;
static struct proc_dir_entry *kflow_ai_net_proc_version_proc;

static KFLOW_AI_IOC_CMD_OUT kflow_ai_ioc_cmd_out = {0};
static wait_queue_head_t kflow_ai_cmd_out_wq;
static wait_queue_head_t kflow_ai_cmd_out_wq2;
static UINT32 kflow_ai_cmd_out_init = 0;
static UINT32 kflow_ai_cmd_out_begin = 0;
static UINT32 kflow_ai_cmd_out_end = 0;
static UINT32 kflow_ai_cmd_out_pid = 0;

static UINT32 kflow_ai_cmd_out_debug_state = KFLOW_AI_DEBUG_RUN;
static wait_queue_head_t kflow_ai_cmd_out_debug_wq;

KFLOW_AI_IOC_VERSION lib_version = {0};
static KFLOW_AI_MODEL_VERSION *gen_version;
static INT g_proc_init_cnt = 0;

static UINT32 kflow_ai_kcmd_proc_id = 0;
static UINT32 kflow_ai_kcmd_trace = 0;
static UINT32 kflow_ai_kcmd_perf = 0;
static UINT32 kflow_ai_kcmd_group_dump_bmp = 0;
static UINT32 kflow_ai_kcmd_iomem_dump_bmp = 0;
static INT32 kflow_ai_kcmd_set_preserve_bufid = -1;

// group
#define VENDOR_AI_NET_CMD_DOT_GROUP		(1 << 1)
#define VENDOR_AI_NET_CMD_MCTRL_ENTRY	(1 << 2)
#define VENDOR_AI_NET_CMD_GROUP			(1 << 3)
#define VENDOR_AI_NET_CMD_MEM_LIST		(1 << 4)

// iomem
#define VENDOR_AI_NET_CMD_IOMEM_DUMP_DEBUG      (1 << 1)
#define VENDOR_AI_NET_CMD_IOMEM_CHK_OVERLAP     (1 << 2)
#define VENDOR_AI_NET_CMD_IOMEM_REORDER_DEBUG   (1 << 3)
#define VENDOR_AI_NET_CMD_IOMEM_SIM_AI1_BUG     (1 << 4)
#define VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG    (1 << 5)
#define VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP    (1 << 6)
#define VENDOR_AI_NET_CMD_IOMEM_CLEAR_IOBUF     (1 << 7)

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static int kflow_ai_net_proc_info_show(struct seq_file *s, void *v)
{
	seq_printf(s, "debug_state(%#x)\r\n", kflow_ai_cmd_out_debug_state);
	seq_printf(s, "cmd_init(%#x) cmd_begin(%#x) cmd_end(%#x) cmd_pid(%#x)\r\n", kflow_ai_cmd_out_init,
			kflow_ai_cmd_out_begin, kflow_ai_cmd_out_end, kflow_ai_cmd_out_pid);
	seq_printf(s, "output path(%s)\r\n", output_path);
	return 0;
}

static int kflow_ai_net_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, kflow_ai_net_proc_info_show, NULL);
}

static struct file_operations kflow_ai_net_proc_info_ops = {
	.owner   = THIS_MODULE,
	.open    = kflow_ai_net_proc_info_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct seq_file *g_seq_file = 0;

#define PERF_LOG_SIZE	256

static int kflow_ai_seg_printf(const char *fmtstr, ...)
{
	char    buf[PERF_LOG_SIZE];
	int     len;
    
	va_list marker;

	//DBG_DUMP("=====================================================================\n");
	//seq_printf(g_seq_file, buf);
	//DBG_DUMP("fmtstr= %s\r\n", fmtstr);
	//DBG_DUMP("=====================================================================\n");
    
	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);

	//DBG_DUMP("=====================================================================\n");
	seq_printf(g_seq_file, buf);
	//DBG_DUMP("buf= %s\r\n", buf);
	//DBG_DUMP("=====================================================================\n");

	return 0;
}


static int kflow_ai_net_proc_ut_show(struct seq_file *s, void *v)
{
	//seq_printf(s, "debug_ut\r\n");

    //time
    //bw
    //engine time_ut
    //engine bw_ut
    
	g_seq_file = s;

	kflow_ai_cat_perf(1, kflow_ai_seg_printf);

	return 0;
}

static int kflow_ai_net_proc_ut_open(struct inode *inode, struct file *file)
{
	return single_open(file, kflow_ai_net_proc_ut_show, NULL);
}

static struct file_operations kflow_ai_net_proc_ut_ops = {
	.owner   = THIS_MODULE,
	.open    = kflow_ai_net_proc_ut_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};


int kflow_ai_net_kcmd_core_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "core");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "core", "dump current core");
	return 0;
}

int kflow_ai_net_kcmd_mem_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "mem");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "mem", "dump current memory");
	return 0;
}

int kflow_ai_net_kcmd_flow_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "flow");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "flow [mask]", "dump flow log (after proc)");
	seq_printf(s, " mask = 1000 : graph stage\r\n");
	seq_printf(s, " mask = 0200 : bind job\r\n");
	seq_printf(s, " mask = 0100 : setup job\r\n");
	seq_printf(s, " mask = 0040 : last job notify user pulle\r\n");
	seq_printf(s, " mask = 0020 : job notify next job\r\n");
	seq_printf(s, " mask = 0010 : user push first job\r\n");
	seq_printf(s, " mask = 0004 : add to wait job\r\n");
	seq_printf(s, " mask = 0002 : add to ready job\r\n");
	seq_printf(s, " mask = 0001 : trigger to run job\r\n");
	return 0;
}

int kflow_ai_net_kcmd_bind_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "bind");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "bind [proc_id]", "dump bind (before proc)");
	return 0;
}

int kflow_ai_net_kcmd_ctx_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "ctx");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "ctx [proc_id]", "dump context (after proc)");
	return 0;
}

int kflow_ai_net_kcmd_obuf_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "obuf");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "obuf [proc_id]", "dump output buffer (while proc)");
	return 0;
}

int kflow_ai_net_kcmd_time_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "time");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "time [proc_id]", "dump execute time (after proc)");
	return 0;
}

int kflow_ai_net_kcmd_timeline_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "timeline");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "timeline [proc_id]", "dump html timeline (after proc)");
	return 0;
}

int kflow_ai_net_kcmd_debug_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "dump");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "prog", "program debug dump start");
	seq_printf(s, "%-25s : %s\r\n", "run", "program debug dump run");
	return 0;
}

int kflow_ai_net_cmd_group_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "group");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "dot_group [proc_id] [on/off]", "dot output of group node");
	seq_printf(s, "%-25s : %s\r\n", "mctrl_entry [proc_id] [on/off]", "mctrl entry result");
	seq_printf(s, "%-25s : %s\r\n", "group [proc_id] [on/off]", "group result");
	seq_printf(s, "%-25s : %s\r\n", "meme_list [proc_id] [on/off]", "mem alloc/free result");
	return 0;
}

int kflow_ai_net_kcmd_outpath_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "outpath");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "outpath [folder path]", "set dump path");
	return 0;
}

int kflow_ai_net_kcmd_dot_group_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "dot_group");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "dot_group [on/off]", "set dot_group");
	return 0;
}

int kflow_ai_net_kcmd_clear_debug_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "clear_debug");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "clear_debug [all]", "clear all debug");
	return 0;
}

int kflow_ai_net_kcmd_mctrl_entry_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "mctrl_entry");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "mctrl_entry [on/off]", "set mctrl_entry");
	return 0;
}

int kflow_ai_net_kcmd_group_dump_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "group_dump");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "group_dump [on/off]", "set group_dump");
	return 0;
}

int kflow_ai_net_kcmd_mem_list_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "mem_list");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "mem_list [on/off]", "set mem_list");
	return 0;
}

int kflow_ai_net_kcmd_trace_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "trace");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "trace [mask]", "set trace");
	return 0;
}

int kflow_ai_net_kcmd_perf_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "perf");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "perf [mask]", "set perf");
	return 0;
}

int kflow_ai_net_kcmd_iomem_dump_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "iomem_dump");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "iomem_dump [on/off]", "set iomem_dump");
	return 0;
}

int kflow_ai_net_kcmd_overlap_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "overlap");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "overlap [on/off]", "set overlap");
	return 0;
}

int kflow_ai_net_kcmd_reorder_dump_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "reorder_dump");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "reorder_dump [on/off]", "set reorder_dump");
	return 0;
}

int kflow_ai_net_kcmd_sim_ai1_bug_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "sim_ai1_bug");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "sim_ai1_bug [on/off]", "set sim_ai1_bug");
	return 0;
}

int kflow_ai_net_kcmd_shrink_dump_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "shrink_dump");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "shrink_dump [on/off]", "set shrink_dump");
	return 0;
}

int kflow_ai_net_kcmd_netinfo_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "netinfo");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "netinfo [on/off]", "set netinfo");
	return 0;
}

int kflow_ai_net_kcmd_clear_iobuf_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "clear_iobuf");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "clear_iobuf [on/off]", "set clear_iobuf");
	return 0;
}

int kflow_ai_net_kcmd_set_preserve_bufid_showhelp(struct seq_file *s, void *v)
{
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "  %s\n", "set_preserve_bufid");
	seq_printf(s, "=====================================================================\n");
	seq_printf(s, "%-25s : %s\r\n", "set_preserve_bufid [id]", "set set_preserve_bufid");
	return 0;
}

static int kflow_ai_net_proc_cmd_show(struct seq_file *s, void *v)
{
	return 0;
}
static int kflow_ai_net_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	char cmd_line[KFLOW_AI_MAX_CMD_LENGTH] = {0};
	int proc_id = 0;

	if (size > (KFLOW_AI_MAX_CMD_LENGTH - 1)) {
		DBG_ERR("Command length is too long!\n");
		goto ERR_OUT;
	}

	if (size == 0) {
		DBG_ERR("Command length = 0!\n");
		goto ERR_OUT;
	}

	if (copy_from_user(cmd_line, buf, size)) {
		goto ERR_OUT;
	}

	DBG_DUMP("CMD:%s\n", cmd_line);
	snprintf(kflow_ai_ioc_cmd_out.str, KFLOW_AI_MAX_CMD_LENGTH, cmd_line);
	kflow_cmd_out_cb(proc_id);
	memset(&kflow_ai_ioc_cmd_out, 0x0, sizeof(KFLOW_AI_IOC_CMD_OUT));

	return size;

ERR_OUT:
	return -EINVAL;
}

static int kflow_ai_net_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, kflow_ai_net_proc_cmd_show, NULL);
}

static struct file_operations kflow_ai_net_proc_cmd_ops = {
	.owner   = THIS_MODULE,
	.open    = kflow_ai_net_proc_cmd_open,
	.read    = seq_read,
	.write   = kflow_ai_net_proc_cmd_write,
	.llseek  = seq_lseek,
	.release = seq_release
};

/* ============================================================================= */
/* proc "Kernel Command" file operation functions								 */
/* ============================================================================= */
int kflow_ai_net_kcmd_dump(void* p_ctx, unsigned char argc, char **argv)
{
	if (argv[0] == NULL) {
		DBG_ERR("Invalid cmd_args\r\n");
		return -1;
	}
	if (strcmp(argv[0], "prog") == 0) {
		kflow_ai_cmd_out_debug_state = KFLOW_AI_DEBUG_PROG;
		DBG_DUMP("=> init() set break\n");
	} else if (strcmp(argv[0], "run") == 0) {
		kflow_cmd_out_run_debug();
		DBG_DUMP("=> init() continue\n");
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	return 0;
}

int kflow_ai_net_kcmd_core(void* p_ctx, unsigned char argc, char **argv)
{
	DBG_DUMP("=> core dump begin:\r\n");
	kflow_ai_core_dump(); //dump current jobs in wait, ready and run
	DBG_DUMP("=> core dump end:\r\n");
	return 0;
}

int kflow_ai_net_kcmd_reg(void* p_ctx, unsigned char argc, char **argv)
{
	DBG_DUMP("=> reg dump begin:\r\n");
	kflow_ai_reg_dump(); //dump current registers of all engines
	DBG_DUMP("=> reg dump end:\r\n");
	return 0;
}

int kflow_ai_net_kcmd_mem(void* p_ctx, unsigned char argc, char **argv)
{
	//DBG_DUMP("=> mem dump begin:\r\n");
	//kflow_ai_mem_dump(); //dump current memory
	//DBG_DUMP("=> mem dump end:\r\n");
	UINT32 mask = 0;
	
	if (argc < 1)
		return -1;
	sscanf(argv[0], "%x", &mask);
	
	if (mask == 0) {
		DBG_DUMP("=> mem end\r\n");
	} else {
		DBG_DUMP("=> mem begin\r\n");
	}
	kflow_ai_mem_dump(mask);
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
	sscanf(argv[0], "%x", &mask);
	
	if (mask == 0) {
		DBG_DUMP("=> flow end\r\n");
	} else {
		DBG_DUMP("=> flow begin: mask=%04x\r\n", mask);
	}
	kflow_ai_net_flow(mask); //dump flow log after proc
	return 0;
}

int kflow_ai_net_kcmd_bind(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 proc_id;

	if (argc < 1)
		return -1;
	sscanf(argv[0], "%u", &proc_id);
	if (proc_id > g_ai_support_net_max) {
		return -1;
	}

	DBG_DUMP("=> bind begin: proc[%d]\r\n", (int)proc_id);
	kflow_ai_net_debug(proc_id, KFLOW_AI_DBG_BIND); //dump bind before proc
	return 0;
}

int kflow_ai_net_kcmd_ctx(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 proc_id;

	if (argc < 1)
		return -1;
	sscanf(argv[0], "%u", &proc_id);
	if (proc_id > g_ai_support_net_max) {
		return -1;
	}

	DBG_DUMP("=> ctx begin: proc[%d]\r\n", (int)proc_id);
	kflow_ai_net_debug(proc_id, KFLOW_AI_DBG_CTX); //dump context while proc (engine register)
	return 0;
}

int kflow_ai_net_kcmd_obuf(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 proc_id;

	if (argc < 1)
		return -1;
	sscanf(argv[0], "%u", &proc_id);
	if (proc_id > g_ai_support_net_max) {
		return -1;
	}

	DBG_DUMP("=> obuf begin: proc[%d]\r\n", (int)proc_id);
	kflow_ai_net_debug(proc_id, KFLOW_AI_DBG_OBUF); //dump buffer while proc
	return 0;
}

int kflow_ai_net_kcmd_perfall(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 util_en;
	UINT32 util_sample = 1;

	if (argc < 1)
		return -1;
	sscanf(argv[0], "%u", &util_en);
	if (argc > 1) {
    	sscanf(argv[1], "%u", &util_sample);
        if (util_sample < 1) util_sample = 1;
        if (util_sample > 60) util_sample = 60;
    }

	if (util_en) {
		DBG_DUMP("=> perf engine perfall begin:\r\n");
        DBG_DUMP("   : sample time = %lu\r\n", (ULONG)util_sample);
		kflow_ai_perf(2, util_sample);
	} else {
		DBG_DUMP("=> perf engine perfall end:\r\n");
        DBG_DUMP("   : sample time = %lu\r\n", (ULONG)util_sample);
		kflow_ai_perf(0, util_sample);
	}
	return 0;
}

int kflow_ai_net_kcmd_ut(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 util_en;
	UINT32 util_sample = 1;

	if (argc < 1)
		return -1;
	sscanf(argv[0], "%u", &util_en);
	if (argc > 1) {
    	sscanf(argv[1], "%u", &util_sample);
        if (util_sample < 1) util_sample = 1;
        if (util_sample > 60) util_sample = 60;
    }

	if (util_en) {
		DBG_DUMP("=> perf engine ut begin:\r\n");
        DBG_DUMP("   : sample time = %lu\r\n", (ULONG)util_sample);
		kflow_ai_perf(1, util_sample);
	} else {
		DBG_DUMP("=> perf engine ut end:\r\n");
        DBG_DUMP("   : sample time = %lu\r\n", (ULONG)util_sample);
		kflow_ai_perf(0, util_sample);
	}
	return 0;
}

int kflow_ai_net_kcmd_time(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 proc_id;

	if (argc < 1)
		return -1;
	sscanf(argv[0], "%u", &proc_id);
	if (proc_id > g_ai_support_net_max) {
		return -1;
	}

	DBG_DUMP("=> time begin: proc[%d]\r\n", (int)proc_id);
	kflow_ai_net_debug(proc_id, KFLOW_AI_DBG_TIME); //dump time after proc
	return 0;
}

int kflow_ai_net_kcmd_timeline(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 proc_id;

	if (argc < 1)
		return -1;
	sscanf(argv[0], "%u", &proc_id);
	if (proc_id > g_ai_support_net_max) {
		return -1;
	}

	DBG_DUMP("=> timeline begin: proc[%d]\r\n", (int)proc_id);
	kflow_ai_net_debug(proc_id, KFLOW_AI_DBG_TIMELINE); //dump timeline after proc
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
	sscanf(argv[0], "%u", &proc_id);
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

#if CNN_AI_FASTBOOT
int kflow_ai_net_kcmd_dump_fastboot(void* p_ctx, unsigned char argc, char **argv)
{
	UINT32 enable;

	if (argc < 1) {
		return -1;
	}
	sscanf(argv[0], "%u", &enable);

	kflow_ai_net_fboot_dump_en = (enable)? 1 : 0;

	DBG_DUMP("set fastboot dump enable = %u\r\n", kflow_ai_net_fboot_dump_en);

	return 0;
}
#endif

int kflow_ai_net_kcmd_clear_debug(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "all", 3) == 0) {
		kflow_ai_kcmd_trace = 0;
		kflow_ai_kcmd_perf = 0;
		kflow_ai_kcmd_group_dump_bmp = 0;
		kflow_ai_kcmd_iomem_dump_bmp = 0;
		kflow_ai_kcmd_set_preserve_bufid = -1;
	} else if (strcmp(argv[1], "trace") == 0) {
		kflow_ai_kcmd_trace = 0;
	} else if (strcmp(argv[1], "perf") == 0) {
		kflow_ai_kcmd_perf = 0;
	} else if (strcmp(argv[1], "group") == 0) {
		kflow_ai_kcmd_group_dump_bmp = 0;
	} else if (strcmp(argv[1], "iomem") == 0) {
		kflow_ai_kcmd_iomem_dump_bmp = 0;
	} else if (strcmp(argv[1], "preserve_bufid") == 0) {
		kflow_ai_kcmd_set_preserve_bufid = -1;
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> clear debug: proc[%d]\r\n", (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_dot_group(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_group_dump_bmp |= (VENDOR_AI_NET_CMD_DOT_GROUP);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_group_dump_bmp &= ~(VENDOR_AI_NET_CMD_DOT_GROUP);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> group_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_group_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_mctrl_entry(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_group_dump_bmp |= (VENDOR_AI_NET_CMD_MCTRL_ENTRY);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_group_dump_bmp &= ~(VENDOR_AI_NET_CMD_MCTRL_ENTRY);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> group_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_group_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_group_dump(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_group_dump_bmp |= (VENDOR_AI_NET_CMD_GROUP);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_group_dump_bmp &= ~(VENDOR_AI_NET_CMD_GROUP);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> group_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_group_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_mem_list(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_group_dump_bmp |= (VENDOR_AI_NET_CMD_MEM_LIST);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_group_dump_bmp &= ~(VENDOR_AI_NET_CMD_MEM_LIST);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> group_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_group_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_trace(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	sscanf(argv[1], "%x", &kflow_ai_kcmd_trace);
	DBG_DUMP("\n=> trace = %#x: proc[%d]\r\n", kflow_ai_kcmd_trace, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_perf(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	sscanf(argv[1], "%x", &kflow_ai_kcmd_perf);
	DBG_DUMP("\n=> perf = %#x: proc[%d]\r\n", kflow_ai_kcmd_perf, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_iomem_dump(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_iomem_dump_bmp |= (VENDOR_AI_NET_CMD_IOMEM_DUMP_DEBUG);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_iomem_dump_bmp &= ~(VENDOR_AI_NET_CMD_IOMEM_DUMP_DEBUG);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> iomem_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_iomem_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_overlap(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_iomem_dump_bmp |= (VENDOR_AI_NET_CMD_IOMEM_CHK_OVERLAP);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_iomem_dump_bmp &= ~(VENDOR_AI_NET_CMD_IOMEM_CHK_OVERLAP);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> iomem_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_iomem_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_reorder_dump(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_iomem_dump_bmp |= (VENDOR_AI_NET_CMD_IOMEM_REORDER_DEBUG);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_iomem_dump_bmp &= ~(VENDOR_AI_NET_CMD_IOMEM_REORDER_DEBUG);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> iomem_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_iomem_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_sim_ai1_bug(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_iomem_dump_bmp |= (VENDOR_AI_NET_CMD_IOMEM_SIM_AI1_BUG);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_iomem_dump_bmp &= ~(VENDOR_AI_NET_CMD_IOMEM_SIM_AI1_BUG);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> iomem_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_iomem_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_shrink_dump(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_iomem_dump_bmp |= (VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_iomem_dump_bmp &= ~(VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> iomem_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_iomem_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_netinfo(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_iomem_dump_bmp |= (VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_iomem_dump_bmp &= ~(VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> iomem_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_iomem_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_clear_iobuf(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	if (strncmp(argv[1], "on", 2) == 0) {
		kflow_ai_kcmd_iomem_dump_bmp |= (VENDOR_AI_NET_CMD_IOMEM_CLEAR_IOBUF);
	} else if (strcmp(argv[1], "off") == 0) {
		kflow_ai_kcmd_iomem_dump_bmp &= ~(VENDOR_AI_NET_CMD_IOMEM_CLEAR_IOBUF);
	} else {
		DBG_ERR("Invalid cmd_args\r\n");
	}
	DBG_DUMP("\n=> iomem_dump_bmp = %#x: proc[%d]\r\n", kflow_ai_kcmd_iomem_dump_bmp, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

int kflow_ai_net_kcmd_set_preserve_bufid(void* p_ctx, unsigned char argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}
	sscanf(argv[0], "%u", &kflow_ai_kcmd_proc_id);
	sscanf(argv[1], "%d", &kflow_ai_kcmd_set_preserve_bufid);
	DBG_DUMP("\n=> set_preserve_bufid = %d: proc[%d]\r\n", kflow_ai_kcmd_set_preserve_bufid, (int)kflow_ai_kcmd_proc_id);

	return 0;
}

static PROC_CMD kcmd_list[] = {
	// keyword      function name
	{ "dump",		kflow_ai_net_kcmd_dump    },
	{ "core",		kflow_ai_net_kcmd_core	 },
	{ "mem",		kflow_ai_net_kcmd_mem    },
	{ "flow",		kflow_ai_net_kcmd_flow   },
	{ "bind",		kflow_ai_net_kcmd_bind	 },
	{ "ctx",		kflow_ai_net_kcmd_ctx	 },
	{ "obuf",		kflow_ai_net_kcmd_obuf	 },
	{ "perfall",	kflow_ai_net_kcmd_perfall },
	{ "ut",			kflow_ai_net_kcmd_ut	 },
	{ "time",		kflow_ai_net_kcmd_time	 },
	{ "timeline",	kflow_ai_net_kcmd_timeline	 },
	{ "reg",		kflow_ai_net_kcmd_reg	 },
	{ "version",	kflow_ai_net_kcmd_version	 },
	{ "outpath",	kflow_ai_net_kcmd_outpath	 },
#if CNN_AI_FASTBOOT
	{ "dump_fastboot",	kflow_ai_net_kcmd_dump_fastboot	 },
#endif
	{ "clear_debug",  	kflow_ai_net_kcmd_clear_debug  	},
	{ "dot_group",  	kflow_ai_net_kcmd_dot_group  	},
	{ "mctrl_entry",  	kflow_ai_net_kcmd_mctrl_entry  	},
	{ "group_dump",  	kflow_ai_net_kcmd_group_dump  	},
	{ "mem_list",  		kflow_ai_net_kcmd_mem_list  	},
	{ "trace",			kflow_ai_net_kcmd_trace  		},
	{ "perf",			kflow_ai_net_kcmd_perf   		},
	{ "iomem_dump",		kflow_ai_net_kcmd_iomem_dump   	},
	{ "overlap",		kflow_ai_net_kcmd_overlap   	},
	{ "reorder_dump",	kflow_ai_net_kcmd_reorder_dump 	},
	{ "sim_ai1_bug",	kflow_ai_net_kcmd_sim_ai1_bug  	},
	{ "shrink_dump",	kflow_ai_net_kcmd_shrink_dump   },
	{ "netinfo",		kflow_ai_net_kcmd_netinfo   	},
	{ "clear_iobuf",	kflow_ai_net_kcmd_clear_iobuf  	},
	{ "set_preserve_bufid",	kflow_ai_net_kcmd_set_preserve_bufid  	},
};

static int kflow_ai_net_proc_kcmd_show(struct seq_file *s, void *v)
{
	DBG_DUMP("proc[%d]:\r\n", (int)kflow_ai_kcmd_proc_id);
	DBG_DUMP("  trace = %#x\r\n", kflow_ai_kcmd_trace);
	DBG_DUMP("  perf = %#x\r\n", kflow_ai_kcmd_perf);
	DBG_DUMP("  group_dump_bmp = %#x\r\n", kflow_ai_kcmd_group_dump_bmp);
	DBG_DUMP("  iomem_dump_bmp = %#x\r\n", kflow_ai_kcmd_iomem_dump_bmp);
	DBG_DUMP("  preserve_bufid = %d\r\n", kflow_ai_kcmd_set_preserve_bufid);
	return 0;
}

#define NUM_OF_CMD (sizeof(kcmd_list) / sizeof(PROC_CMD))

static int kflow_ai_net_proc_kcmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	int ret = 0;
	char cmd_line[KFLOW_AI_MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[KFLOW_AI_MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	unsigned char loop;

	// check command length
	if (len > (KFLOW_AI_MAX_CMD_LENGTH - 1)) {
		DBG_ERR("Command length is too long!\n");
		goto ERR_OUT;
	}

	if (len == 0) {
		DBG_ERR("Command length = 0!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < KFLOW_AI_MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	// dispatch command handler
	for (loop = 0 ; loop < NUM_OF_CMD; loop++) {
		if (strncmp(argv[0], kcmd_list[loop].cmd, KFLOW_AI_MAX_CMD_LENGTH) == 0) {
			ret = kcmd_list[loop].execute(0, ucargc - 1, &argv[1]);
			break;
		}
	}
	if (loop >= NUM_OF_CMD) {
		goto ERR_INVALID_CMD;
	}
	if (ret != 0) {
		goto ERR_INVALID_CMD;
	}

	return size;

ERR_INVALID_CMD:
	DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/kflow_ai/help\" for help.\r\n");
	return -EINVAL;

ERR_OUT:
	return -EPERM;
}

static int kflow_ai_net_proc_kcmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, kflow_ai_net_proc_kcmd_show, NULL);
}

static struct file_operations kflow_ai_net_proc_kcmd_ops = {
	.owner   = THIS_MODULE,
	.open    = kflow_ai_net_proc_kcmd_open,
	.read    = seq_read,
	.write   = kflow_ai_net_proc_kcmd_write,
	.llseek  = seq_lseek,
	.release = seq_release
};

static int kflow_ai_net_proc_help_show(struct seq_file *s, void *v)
{
	seq_printf(s, "\n\n1. 'cat /proc/kflow_ai/info' will show all the kflow_ai info\n");
	seq_printf(s, "2. 'echo [proc_id] xxx > /proc/kflow_ai/kcmd' can input command for some debug purpose\n");
	seq_printf(s, "   where \"xxx\" is as following ....\n\n");

	kflow_ai_net_kcmd_debug_showhelp(s, v);
	kflow_ai_net_kcmd_core_showhelp(s, v);
	kflow_ai_net_kcmd_mem_showhelp(s, v);
	kflow_ai_net_kcmd_flow_showhelp(s, v);
	kflow_ai_net_kcmd_bind_showhelp(s, v);
	kflow_ai_net_kcmd_ctx_showhelp(s, v);
	kflow_ai_net_kcmd_obuf_showhelp(s, v);
	kflow_ai_net_kcmd_time_showhelp(s, v);
	kflow_ai_net_kcmd_timeline_showhelp(s, v);
	kflow_ai_net_kcmd_outpath_showhelp(s, v);
	kflow_ai_net_kcmd_clear_debug_showhelp(s, v);
	kflow_ai_net_kcmd_dot_group_showhelp(s, v);
	kflow_ai_net_kcmd_mctrl_entry_showhelp(s, v);
	kflow_ai_net_kcmd_group_dump_showhelp(s, v);
	kflow_ai_net_kcmd_mem_list_showhelp(s, v);
	kflow_ai_net_kcmd_trace_showhelp(s, v);
	kflow_ai_net_kcmd_perf_showhelp(s, v);
	kflow_ai_net_kcmd_iomem_dump_showhelp(s, v);
	kflow_ai_net_kcmd_overlap_showhelp(s, v);
	kflow_ai_net_kcmd_reorder_dump_showhelp(s, v);
	kflow_ai_net_kcmd_sim_ai1_bug_showhelp(s, v);
	kflow_ai_net_kcmd_shrink_dump_showhelp(s, v);
	kflow_ai_net_kcmd_netinfo_showhelp(s, v);
	kflow_ai_net_kcmd_clear_iobuf_showhelp(s, v);
	kflow_ai_net_kcmd_set_preserve_bufid_showhelp(s, v);

	seq_printf(s, "\n\n3. 'echo [proc_id] xxx > /proc/kflow_ai/cmd' can input command for some debug purpose\n");
	seq_printf(s, "   where \"xxx\" is as following ....\n\n");
	kflow_ai_net_cmd_group_showhelp(s, v);

	return 0;
}

static int kflow_ai_net_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, kflow_ai_net_proc_help_show, NULL);
}

static struct file_operations kflow_ai_net_proc_help_ops = {
	.owner   = THIS_MODULE,
	.open    = kflow_ai_net_proc_help_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

static int kflow_ai_net_proc_version_show(struct seq_file *s, void *v)
{
	if (kflow_ai_cmd_out_init) {
		seq_printf(s, "VENDOR_AI_VERSION:      %2x.%2x.%2x\n",
					(lib_version.vendor_ai_version & 0xFF0000) >> 16,
					(lib_version.vendor_ai_version & 0x00FF00) >> 8,
					(lib_version.vendor_ai_version & 0x0000FF));
		seq_printf(s, "VENDOR_AI_IMPL_VERSION: %s\n", lib_version.vendor_ai_impl_version);
		seq_printf(s, "KFLOW_AI_IMPL_VERSION:  %s\n", KFLOW_AI_IMPL_VERSION);
		seq_printf(s, "KDRV_AI_IMPL_VERSION:   %s\n", KDRV_AI_IMPL_VERSION);
	}

	return 0;
}

static int kflow_ai_net_proc_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, kflow_ai_net_proc_version_show, NULL);
}

static struct file_operations kflow_ai_net_proc_version_ops = {
	.owner   = THIS_MODULE,
	.open    = kflow_ai_net_proc_version_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

int kflow_cmd_out_init(void)
{
	kflow_ai_cmd_out_begin = KFLOW_AI_WAIT_VALUE;
	kflow_ai_cmd_out_end = KFLOW_AI_WAIT_VALUE;
	kflow_ai_cmd_out_pid = KFLOW_AI_WAIT_VALUE;
	init_waitqueue_head(&kflow_ai_cmd_out_wq);
	init_waitqueue_head(&kflow_ai_cmd_out_wq2);
	kflow_ai_cmd_out_init = 1;
	return 1;
}
int kflow_cmd_out_uninit(void)
{
	if(!kflow_ai_cmd_out_init) {
		return 0;
	}
	kflow_ai_cmd_out_begin = 0;
	kflow_ai_cmd_out_end = 0;
	kflow_ai_cmd_out_pid = 0;
	wake_up_interruptible(&kflow_ai_cmd_out_wq2);
	wake_up_interruptible(&kflow_ai_cmd_out_wq);
	kflow_ai_cmd_out_init = 0;
	return 1;
}

void kflow_cmd_out_cb(UINT32 proc_id)
{
	if(!kflow_ai_cmd_out_init) {
		return;
	}
	kflow_ai_cmd_out_begin = proc_id;
	wake_up_interruptible(&kflow_ai_cmd_out_wq);
	///.....
	wait_event_interruptible(kflow_ai_cmd_out_wq2, (kflow_ai_cmd_out_end != KFLOW_AI_WAIT_VALUE));
	kflow_ai_cmd_out_end = KFLOW_AI_WAIT_VALUE;
}

int kflow_ai_cmd_out_wait(KFLOW_AI_IOC_CMD_OUT *p_out)
{
	if(!kflow_ai_cmd_out_init) {
		return KFLOW_AI_WAIT_VALUE;
	}
	if (p_out == NULL) {
		return KFLOW_AI_WAIT_VALUE;
	}
	snprintf(p_out->str, KFLOW_AI_MAX_CMD_LENGTH, kflow_ai_ioc_cmd_out.str);
	wait_event_interruptible(kflow_ai_cmd_out_wq, (kflow_ai_cmd_out_begin != KFLOW_AI_WAIT_VALUE));
	if (kflow_ai_cmd_out_end == 0) { // kflow_cmd_out_uninit was called
		return KFLOW_AI_WAIT_VALUE;
	}
	kflow_ai_cmd_out_pid = kflow_ai_cmd_out_begin;
	kflow_ai_cmd_out_begin = KFLOW_AI_WAIT_VALUE;
	return kflow_ai_cmd_out_pid;
}

void kflow_ai_cmd_out_sig(UINT32 proc_id, KFLOW_AI_IOC_CMD_OUT *p_out)
{
	if(!kflow_ai_cmd_out_init) {
		return;
	}
	if (p_out == NULL) {
		return;
	}
	snprintf(p_out->str, KFLOW_AI_MAX_CMD_LENGTH, kflow_ai_ioc_cmd_out.str);
	kflow_ai_cmd_out_end = proc_id;
	wake_up_interruptible(&kflow_ai_cmd_out_wq2);
	kflow_ai_cmd_out_pid = KFLOW_AI_WAIT_VALUE;
}

int kflow_cmd_out_prog_debug(void)
{
	if (kflow_ai_cmd_out_debug_state == KFLOW_AI_DEBUG_PROG) {
		DBG_DUMP("=> init() break\n");
		wait_event_interruptible(kflow_ai_cmd_out_debug_wq, (kflow_ai_cmd_out_debug_state != KFLOW_AI_DEBUG_PROG));
	}
	return 0;
}

int kflow_cmd_out_run_debug(void)
{
	int proc_id = 0;

	kflow_ai_cmd_out_debug_state = KFLOW_AI_DEBUG_RUN;
	wake_up_interruptible(&kflow_ai_cmd_out_debug_wq);
	kflow_ai_cmd_out_begin = proc_id;
	wake_up_interruptible(&kflow_ai_cmd_out_wq);
	kflow_ai_cmd_out_end = proc_id;
	wake_up_interruptible(&kflow_ai_cmd_out_wq2);
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

int kflow_ai_set_lib_version(KFLOW_AI_IOC_VERSION *p_vers_info)
{
	if (p_vers_info == NULL) {
		return -1;
	}
	memcpy(&lib_version, p_vers_info, sizeof(KFLOW_AI_IOC_VERSION));
	return 0;
}

CHAR* kflow_ai_get_gen_version(void)
{
	return lib_version.vendor_ai_impl_version;
}

int kflow_ai_get_debug_info(KFLOW_AI_IOC_DEBUG_INFO *p_debug_info)
{
	if (p_debug_info == NULL) {
		return -1;
	}
	p_debug_info->kcmd_proc_id = kflow_ai_kcmd_proc_id;
	p_debug_info->kcmd_proc_trace = (kflow_ai_kcmd_trace) | (kflow_ai_kcmd_perf);
	p_debug_info->kcmd_group_dump_bmp = kflow_ai_kcmd_group_dump_bmp;
	p_debug_info->kcmd_iomem_dump_bmp = kflow_ai_kcmd_iomem_dump_bmp;
	p_debug_info->kcmd_set_preserve_bufid = kflow_ai_kcmd_set_preserve_bufid;

	return 0;
}

extern unsigned int vendor_ai_debug_level;

int kflow_ai_get_debug_lvl(KFLOW_AI_IOC_DEBUG_LVL *p_debug_lvl)
{
	if (p_debug_lvl == NULL) {
		return -1;
	}
	p_debug_lvl->lvl = vendor_ai_debug_level;

	return 0;
}


int kflow_ai_net_proc_create(void)
{
	init_waitqueue_head(&kflow_ai_cmd_out_debug_wq);
	kflow_ai_cmd_out_debug_state = KFLOW_AI_DEBUG_RUN;

	kflow_ai_net_proc = proc_mkdir("kflow_ai", NULL);
	if (kflow_ai_net_proc == NULL) {
		return -EFAULT;
	}

	kflow_ai_net_proc_info_proc = proc_create("info", 0, kflow_ai_net_proc, &kflow_ai_net_proc_info_ops);
	if (kflow_ai_net_proc_info_proc == NULL) {
		return -EFAULT;
	}

	kflow_ai_net_proc_ut_proc = proc_create("ut", 0, kflow_ai_net_proc, &kflow_ai_net_proc_ut_ops);
	if (kflow_ai_net_proc_ut_proc == NULL) {
		return -EFAULT;
	}

	kflow_ai_net_proc_cmd_proc = proc_create("cmd", 0, kflow_ai_net_proc, &kflow_ai_net_proc_cmd_ops);
	if (kflow_ai_net_proc_cmd_proc == NULL) {
		return -EFAULT;
	}

	kflow_ai_net_proc_kcmd_proc = proc_create("kcmd", 0, kflow_ai_net_proc, &kflow_ai_net_proc_kcmd_ops);
	if (kflow_ai_net_proc_kcmd_proc == NULL) {
		return -EFAULT;
	}

	kflow_ai_net_proc_help_proc = proc_create("help", 0, kflow_ai_net_proc, &kflow_ai_net_proc_help_ops);
	if (kflow_ai_net_proc_help_proc == NULL) {
		return -EFAULT;
	}

	kflow_ai_net_proc_version_proc = proc_create("version", 0, kflow_ai_net_proc, &kflow_ai_net_proc_version_ops);
	if (kflow_ai_net_proc_version_proc == NULL) {
		return -EFAULT;
	}

	return 0;
}

void kflow_ai_net_proc_remove(void)
{
	if (kflow_ai_net_proc_version_proc != 0) {
		proc_remove(kflow_ai_net_proc_version_proc);
	}
	if (kflow_ai_net_proc_help_proc != 0) {
		proc_remove(kflow_ai_net_proc_help_proc);
	}
	if (kflow_ai_net_proc_kcmd_proc != 0) {
		proc_remove(kflow_ai_net_proc_kcmd_proc);
	}
	if (kflow_ai_net_proc_cmd_proc != 0) {
		proc_remove(kflow_ai_net_proc_cmd_proc);
	}
	if (kflow_ai_net_proc_ut_proc != 0) {
		proc_remove(kflow_ai_net_proc_ut_proc);
	}
	if (kflow_ai_net_proc_info_proc != 0) {
		proc_remove(kflow_ai_net_proc_info_proc);
	}
	if (kflow_ai_net_proc != 0) {
		proc_remove(kflow_ai_net_proc);
	}
}

int kflow_ai_net_proc_init(VOID)
{
	if (g_proc_init_cnt == 0) {
		gen_version = (KFLOW_AI_MODEL_VERSION *)vmalloc(sizeof(KFLOW_AI_MODEL_VERSION) * g_ai_support_net_max);
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
			vfree(gen_version);
		}
		g_proc_init_cnt = 0;
	}
	return E_OK;
}
