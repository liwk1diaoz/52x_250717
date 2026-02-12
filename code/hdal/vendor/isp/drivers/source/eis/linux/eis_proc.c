#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "eis_proc.h"
#include "../eis_api.h"
#include "../eis_int.h"

unsigned int nvt_eis_debug_level = THIS_DBGLVL;
module_param_named(nvt_eis_debug_level, nvt_eis_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(nvt_eis_debug_level, "nvt eis debug level");
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
	int (*execute)(PEIS_DRV_INFO pdrv, unsigned char argc, char **p_argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
PEIS_DRV_INFO pdrv_info_data;


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static int nvt_eis_proc_cmd_show(struct seq_file *p_sfile, void *v)
{
	return 0;
}

static int nvt_eis_proc_cmd_open(struct inode *p_inode, struct file *p_file)
{
	return single_open(p_file, nvt_eis_proc_cmd_show, PDE_DATA(p_inode));
}

static ssize_t nvt_eis_proc_cmd_write(struct file *p_file, const char __user *buf,
									  size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	if (len) {
		cmd_line[len - 1] = '\0';
	} else {
		cmd_line[0] = 0;
	}

	DBG_IND("CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 1; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}
	if (ucargc==0) {
		goto ERR_OUT;
	}

	// dispatch command handler
	ret = eis_cmd_execute(ucargc, argv);

	return size;

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_eis_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_eis_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_eis_proc_help_show(struct seq_file *p_sfile, void *v)
{
	seq_printf(p_sfile, "=====================================================================\n");
	seq_printf(p_sfile, " Add message here\n");
	seq_printf(p_sfile, "=====================================================================\n");
	return 0;
}

static int nvt_eis_proc_help_open(struct inode *p_inode, struct file *p_file)
{
	return single_open(p_file, nvt_eis_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_eis_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_eis_proc_init(PEIS_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *p_module_root = NULL;
	struct proc_dir_entry *p_entry = NULL;

	p_module_root = proc_mkdir("hdal/vendor/eis", NULL);
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

int nvt_eis_proc_remove(PEIS_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->p_proc_help_entry);
	proc_remove(pdrv_info->p_proc_cmd_entry);
	proc_remove(pdrv_info->p_proc_module_root);
	return 0;
}
