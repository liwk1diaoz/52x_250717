#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "msdcnvt_proc.h"
#include "msdcnvt_main.h"
#include "msdcnvt_int.h"

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     6

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(MSDCNVT_INFO *p_drv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
MSDCNVT_DRV_INFO *p_drv_info_data;

//============================================================================
// Function define
//============================================================================

#if 0
//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
	{ "reg",            msdcnvt_api_read_reg           },
};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
	{ "reg",            msdcnvt_api_write_reg          },
	{ "pattern",        msdcnvt_api_write_pattern       }
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))

static int msdcnvt_proc_cmd_show(struct seq_file *sfile, void *v)
{
	DBG_IND("\n");
	return 0;
}

static int msdcnvt_proc_cmd_open(struct inode *inode, struct file *file)
{
	DBG_IND("\n");
	return single_open(file, msdcnvt_proc_cmd_show, &p_drv_info_data->module_info);
}

static ssize_t msdcnvt_proc_cmd_write(struct file *file, const char __user *buf,
		size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	unsigned char loop;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	// dispatch command handler
	if (strncmp(argv[0], "r", 2) == 0) {
		for (loop = 0 ; loop < NUM_OF_READ_CMD; loop++) {
			if (strncmp(argv[1], cmd_read_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_read_list[loop].execute(&p_drv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}
		if (loop >= NUM_OF_READ_CMD) {
			goto ERR_INVALID_CMD;
		}

	} else if (strncmp(argv[0], "w", 2) == 0)  {

		for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&p_drv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}

		if (loop >= NUM_OF_WRITE_CMD) {
			goto ERR_INVALID_CMD;
		}

	} else {
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
	.open    = msdcnvt_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = msdcnvt_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int msdcnvt_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " Add message here\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int msdcnvt_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdcnvt_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = msdcnvt_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};
#endif

int msdcnvt_proc_init(MSDCNVT_DRV_INFO *p_drv_info)
{
	int ret = 0;
	struct proc_dir_entry *p_module_root = NULL;

#if 0
	struct proc_dir_entry *pentry = NULL;
#endif

	p_module_root = proc_mkdir("msdcnvt", NULL);
	if (p_module_root == NULL) {
		DBG_ERR("failed to create module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	p_drv_info->p_proc_module_root = p_module_root;

#if 0
	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, p_module_root, &proc_cmd_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, p_module_root, &proc_help_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_help_entry = pentry;
#endif

	p_drv_info_data = p_drv_info;

	return ret;

#if 0
remove_cmd:
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_cmd_entry);
	}
	if (p_drv_info->p_proc_help_entry) {
		proc_remove(p_drv_info->p_proc_help_entry);
	}
#endif

remove_root:
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return ret;
}

int msdcnvt_proc_remove(MSDCNVT_DRV_INFO *p_drv_info)
{
	if (p_drv_info->p_proc_help_entry) {
		proc_remove(p_drv_info->p_proc_help_entry);
	}
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_cmd_entry);
	}
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return 0;
}
