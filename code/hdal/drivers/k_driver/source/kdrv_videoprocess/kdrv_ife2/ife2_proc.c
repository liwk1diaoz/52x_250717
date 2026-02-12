#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>


#include "ife2_proc.h"
#include "ife2_dbg.h"
#include "ife2_main.h"
#include "ife2_api.h"

#include "ife2_kit.h"

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
	int (*execute)(PIFE2_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
static PIFE2_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
#if (__IFE2_DBG_CMD__ == 1)
	{ "reg",            nvt_ife2_api_read_reg           },
	{ "dump",           nvt_ife2_api_dump           },
#endif
};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
#if (__IFE2_DBG_CMD__ == 1)
	{ "reg",            nvt_ife2_api_write_reg          },
	{ "pattern",        nvt_ife2_api_write_pattern      },
#endif

#if !defined (CONFIG_NVT_SMALL_HDAL)
	{ "ife2_kdrv",      nvt_kdrv_ife2_test              },
	{ "ife2_hw_pwsv",    nvt_ife2_hw_power_saving        },
	{ "ife2_fw_pwsv",    nvt_ife2_fw_power_saving        },
#endif
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))

static int nvt_ife2_proc_cmd_show(struct seq_file *sfile, void *v)
{
	//nvt_dbg(IND, "\n");
	return 0;
}

static int nvt_ife2_proc_cmd_open(struct inode *inode, struct file *file)
{
	//nvt_dbg(IND, "\n");
	return single_open(file, nvt_ife2_proc_cmd_show, &pdrv_info_data->ife2_info);
}

static ssize_t nvt_ife2_proc_cmd_write(struct file *file, const char __user *buf,
									   size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	//unsigned char loop;
	int32_t loop;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len) != 0) {
		goto ERR_OUT;
	}

	if (len > 0) {
		cmd_line[len - 1] = '\0';
	}

	//nvt_dbg(IND, "CMD:%s\n", cmd_line);

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
				ret = cmd_read_list[loop].execute(&pdrv_info_data->ife2_info, ucargc - 2, &argv[2]);
				break;
			}
		}

		//if (loop >= NUM_OF_READ_CMD) {
		//	goto ERR_INVALID_CMD;
		//}
	} else if (strncmp(argv[0], "w", 2) == 0)  {
		for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&pdrv_info_data->ife2_info, ucargc - 2, &argv[2]);
				break;
			}
		}

		//if (loop >= NUM_OF_WRITE_CMD) {
		//	goto ERR_INVALID_CMD;
		//}
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
	.open    = nvt_ife2_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_ife2_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_ife2_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " Add message here\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int nvt_ife2_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ife2_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ife2_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_ife2_proc_init(PIFE2_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pife2_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pife2_root = proc_mkdir("kdrv_ife2", NULL);
	if (pife2_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_ife2_root = pife2_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pife2_root, &proc_cmd_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pife2_root, &proc_help_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_help_entry = pentry;


	pdrv_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_ife2_root);
	return ret;
}

int nvt_ife2_proc_remove(PIFE2_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_ife2_root);
	return 0;
}
