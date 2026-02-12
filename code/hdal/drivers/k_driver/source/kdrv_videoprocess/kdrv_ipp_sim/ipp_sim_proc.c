#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "ipp_sim_proc.h"
#include "ipp_sim_dbg.h"
#include "ipp_sim_main.h"
#include "ipp_sim_api.h"


//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     10
#define MAX_CMD_TEXT_LENGTH 512

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PIPP_SIM_INFO pdrv, unsigned char argc, char **argv);
	char text[MAX_CMD_TEXT_LENGTH];
} IPP_PROC_CMD, *PIPP_PROC_CMD;

//============================================================================
// Global variable
//============================================================================
PIPP_SIM_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static IPP_PROC_CMD cmd_read_list[] = {
	// keyword          function name                                 description
	{"ipp_show_stripe", nvt_kdrv_ipp_show_stripe_settings,            "Show IPP stripe settings."},
};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(IPP_PROC_CMD))

static IPP_PROC_CMD cmd_write_list[] = {
	// keyword          function name                                 description
	{"ipp_init",        nvt_kdrv_ipp_initialize_hw_and_param_buf,     "Open IPP HW and initialize parameter buffers."},
	{"ipp_rel",         nvt_kdrv_ipp_release_hw_and_param_buf,        "Close IPP HW and release parameter buffers."},
	{"ipp_sim_f02",     nvt_kdrv_ipp_flow02_test,                     "Run IPP flow0 or flow2."},
	{"ipp_sim_f1",      nvt_kdrv_ipp_flow1_test,                      "Run IPP flow1."},
	{"lca_sim",         nvt_kdrv_lca_test,                            "Run IFE2-IME."},
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(IPP_PROC_CMD))

static int nvt_ipp_sim_proc_cmd_show(struct seq_file *sfile, void *v)
{
	//nvt_dbg(IND, "\n");
	return 0;
}

static int nvt_ipp_sim_proc_cmd_open(struct inode *inode, struct file *file)
{
	//nvt_dbg(IND, "\n");
	return single_open(file, nvt_ipp_sim_proc_cmd_show, &pdrv_info_data->ipp_sim_info);
}

static ssize_t nvt_ipp_sim_proc_cmd_write(struct file *file, const char __user *buf,
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
				ret = cmd_read_list[loop].execute(&pdrv_info_data->ipp_sim_info, ucargc - 2, &argv[2]);
				break;
			}
		}
		if (loop >= NUM_OF_READ_CMD) {
			goto ERR_INVALID_CMD;
		}

	} else if (strncmp(argv[0], "w", 2) == 0)  {

		for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&pdrv_info_data->ipp_sim_info, ucargc - 2, &argv[2]);
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
	.open    = nvt_ipp_sim_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_ipp_sim_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_ipp_sim_proc_help_show(struct seq_file *sfile, void *v)
{
	int loop;

	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " IPP simulation command list\n");
	seq_printf(sfile, "=====================================================================\n");

	for (loop = 0 ; loop < NUM_OF_READ_CMD ; loop++) {
		seq_printf(sfile, "r %15s : %s\r\n", cmd_read_list[loop].cmd, cmd_read_list[loop].text);
	}

	for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
		seq_printf(sfile, "w %15s : %s\r\n", cmd_write_list[loop].cmd, cmd_write_list[loop].text);
	}
	return 0;
}

static int nvt_ipp_sim_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ipp_sim_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ipp_sim_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_ipp_sim_proc_init(PIPP_SIM_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pipp_sim_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pipp_sim_root = proc_mkdir(IPP_SIM_NAME, NULL);
	if (pipp_sim_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_ipp_sim_root = pipp_sim_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pipp_sim_root, &proc_cmd_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pipp_sim_root, &proc_help_fops);
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
	proc_remove(pdrv_info->pproc_ipp_sim_root);
	return ret;
}

int nvt_ipp_sim_proc_remove(PIPP_SIM_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_ipp_sim_root);
	return 0;
}
