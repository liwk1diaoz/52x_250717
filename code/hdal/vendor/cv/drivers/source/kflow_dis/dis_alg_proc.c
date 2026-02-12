#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "dis_alg_drv.h"
#include "dis_alg_flow_api.h"
#include "dis_alg_platform.h"

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     6

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PDISFLOW_DRV_INFO pdrv, unsigned char argc, char **p_argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
PDISFLOW_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================
#if !defined(CONFIG_NVT_SMALL_HDAL)
int nvtdisflow_proc_cmd_init(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
    #ifdef DEBUG_KDRV_DIS_ALG
	exam_kflow_dis_task_init(argc, argv);
    #endif
	return 0;
}
int nvtdisflow_proc_cmd_get(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
    #ifdef DEBUG_KDRV_DIS_ALG
	exam_kflow_dis_task_get(argc, argv);
    #endif
	return 0;
}
int nvtdisflow_proc_cmd_reset(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
    #ifdef DEBUG_KDRV_DIS_ALG
	exam_kflow_dis_task_reset(argc, argv);
    #endif
	return 0;
}
int nvtdisflow_proc_cmd_uninit(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
    #ifdef DEBUG_KDRV_DIS_ALG
	exam_kflow_dis_task_uninit(argc, argv);
    #endif
	return 0;
}
#endif
int nvtdisflow_proc_cmd_read_version(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
	exam_kflow_dis_read_version(argc, argv);
	return 0;
}

int nvtdisflow_proc_cmd_dump_global_mv_start(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
	exam_kflow_dis_dump_global_mv_start(argc, argv);
	return 0;
}

int nvtdisflow_proc_cmd_dump_global_mv_end(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
	exam_kflow_dis_dump_global_mv_end(argc, argv);
	return 0;
}

int nvtdisflow_proc_cmd_dump_param_in(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
	exam_kflow_dis_dump_param_in(argc, argv);
	return 0;
}

int nvtdisflow_proc_cmd_dump_buffsize(PDISFLOW_DRV_INFO p_drv, unsigned char argc, char **argv)
{
	exam_kflow_dis_dump_buffsize(argc, argv);
	return 0;
}

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_list[] = {
	// keyword          function name
	#ifdef DEBUG_KDRV_DIS_ALG
	{ "init",             nvtdisflow_proc_cmd_init                   },
 	{ "get",              nvtdisflow_proc_cmd_get                    },
	{ "reset",            nvtdisflow_proc_cmd_reset                  },
    { "uninit",           nvtdisflow_proc_cmd_uninit                 },
	#endif
    { "ver",                   nvtdisflow_proc_cmd_read_version           },
    { "dump_mv_start",         nvtdisflow_proc_cmd_dump_global_mv_start   },
    { "dump_mv_end",           nvtdisflow_proc_cmd_dump_global_mv_end     },
    { "dump_param_setting",    nvtdisflow_proc_cmd_dump_param_in          },
    { "dump_buffsize",         nvtdisflow_proc_cmd_dump_buffsize          },
};

#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))

static int nvt_dis_alg_proc_cmd_show(struct seq_file *p_sfile, void *v)
{
	nvt_dbg(IND, "\n");
	return 0;
}

static int nvt_dis_alg_proc_cmd_open(struct inode *p_inode, struct file *p_file)
{
	nvt_dbg(IND, "\n");
	return single_open(p_file, nvt_dis_alg_proc_cmd_show, PDE_DATA(p_inode));
}

static ssize_t nvt_dis_alg_proc_cmd_write(struct file *p_file, const char __user *buf,
									  size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *p_cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *p_argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	unsigned char loop;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}
	if (len == 0) {
		nvt_dbg(ERR, "Command length is zero!\n");
		goto ERR_OUT;
	}
	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	nvt_dbg(IND, "CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		p_argv[ucargc] = strsep(&p_cmdstr, delimiters);

		if (p_argv[ucargc] == NULL) {
			break;
		}
	}

	// dispatch command handler

	for (loop = 0 ; loop < NUM_OF_CMD ; loop++) {
		if (strncmp(p_argv[1], cmd_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
			ret = cmd_list[loop].execute(pdrv_info_data, ucargc - 2, &p_argv[2]);
			break;
		}
	}

	if (loop >= NUM_OF_CMD) {
		goto ERR_INVALID_CMD;
	}


	return size;

ERR_INVALID_CMD:
	nvt_dbg(ERR, "Invalid CMD \"%s\"\n", cmd_line);

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_dis_alg_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_dis_alg_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_dis_alg_proc_help_show(struct seq_file *p_sfile, void *v)
{
	seq_printf(p_sfile, "=====================================================================\n");
	seq_printf(p_sfile, " Add message here\n");
	seq_printf(p_sfile, "=====================================================================\n");
	return 0;
}

static int nvt_dis_alg_proc_help_open(struct inode *p_inode, struct file *p_file)
{
	return single_open(p_file, nvt_dis_alg_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_dis_alg_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_dis_alg_proc_init(PDISFLOW_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *p_module_root = NULL;
	struct proc_dir_entry *p_entry = NULL;

	p_module_root = proc_mkdir("hdal/vendor/kflow_dis", NULL);
	if (p_module_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->p_proc_module_root = p_module_root;


	p_entry = proc_create("cmd", S_IRUGO | S_IXUGO, p_module_root, &proc_cmd_fops);
	if (p_entry == NULL) {
		nvt_dbg(ERR, "failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->p_proc_cmd_entry = p_entry;

	p_entry = proc_create("help", S_IRUGO | S_IXUGO, p_module_root, &proc_help_fops);
	if (p_entry == NULL) {
		nvt_dbg(ERR, "failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->p_proc_help_entry = p_entry;


	pdrv_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->p_proc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->p_proc_module_root);
	return ret;
}

int nvt_dis_alg_proc_remove(PDISFLOW_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->p_proc_help_entry);
	proc_remove(pdrv_info->p_proc_cmd_entry);
	proc_remove(pdrv_info->p_proc_module_root);
	return 0;
}
