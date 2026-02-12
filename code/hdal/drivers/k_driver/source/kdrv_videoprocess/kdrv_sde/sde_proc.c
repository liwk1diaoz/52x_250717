#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>


#include "sde_proc.h"
#include "sde_dbg.h"
#include "sde_main.h"
#include "sde_api.h"
#include "sde_lib.h"

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
	int (*execute)(PSDE_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
PSDE_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
	{ "reg",            nvt_sde_api_read_reg           }, // read reg
};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
	{ "reg",            nvt_sde_api_write_reg          }, // write value to hw
	{ "pattern",		nvt_kdrv_sde_api_test_kdrv		}, // read file from sd:
	{ "pattern2",		nvt_kdrv_sde_api_test_drv		} // read file from sd:
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))

static int nvt_sde_proc_cmd_show(struct seq_file *sfile, void *v)
{
	nvt_dbg(IND, "\n");
	return 0;
}

static int nvt_sde_proc_cmd_open(struct inode *inode, struct file *file)
{
	nvt_dbg(IND, "\n");
	return single_open(file, nvt_sde_proc_cmd_show, &pdrv_info_data->module_info);
}



static ssize_t nvt_sde_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	unsigned char loop;

	nvt_dbg(ERR, "sde cmd len = %d \n", len);

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
	if (copy_from_user(cmd_line, buf, len) != 0)
		goto ERR_OUT;

	
	cmd_line[len - 1] = '\0';

	nvt_dbg(ERR, "CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL)
		    break;
	}

	// dispatch command handler
	if (strncmp(argv[0], "r", 2) == 0) {

		for (loop = 0 ; loop < NUM_OF_READ_CMD; loop++) {
			if (strncmp(argv[1], cmd_read_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_read_list[loop].execute(&pdrv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}
		if (loop >= NUM_OF_READ_CMD)
			goto ERR_INVALID_CMD;
	} else if (strncmp(argv[0], "w", 2) == 0)  {
		
		nvt_dbg(ERR, "argv[1] = %s ;  argv[2] = %s\n", argv[1], argv[2]);
		
		for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&pdrv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}

		if (loop >= NUM_OF_WRITE_CMD)
		goto ERR_INVALID_CMD;

	} else
		goto ERR_INVALID_CMD;

	return size;

ERR_INVALID_CMD:
	nvt_dbg(ERR, "Invalid CMD \"%s\"\n", cmd_line);

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_sde_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_sde_proc_cmd_write
};




//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_sde_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " Add message here\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int nvt_sde_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_sde_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_sde_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};



//=============================================================================
// proc "fw_gating_en" file operation functions
//=============================================================================

	
static ssize_t nvt_sde_proc_gating_write(struct file *file, const char __user *buffer, size_t size, loff_t *off)
{
	char value_str[16] = {'\0'};
	if (copy_from_user(value_str, buffer, size))
		return -EFAULT;

	sscanf(value_str, "%d\n", &g_sde_fw_power_saving_en);

	if(g_sde_fw_power_saving_en > 1){
		g_sde_fw_power_saving_en = 1;
	}
	return size;
}

static ssize_t nvt_sde_proc_hw_gating_write(struct file *file, const char __user *buffer, size_t size, loff_t *off)
{
	char value_str[16] = {'\0'};
	if (copy_from_user(value_str, buffer, size))
		return -EFAULT;

	sscanf(value_str, "%d\n", &g_sde_hw_power_saving_en);

	if(g_sde_hw_power_saving_en > 1){
		g_sde_hw_power_saving_en = 1;
	}

	if (g_sde_hw_power_saving_en == 1) {
		clk_set_phase((pdrv_info_data->module_info.pclk[0]), 1);
		clk_set_phase((pdrv_info_data->module_info.p_pclk[0]), 1);
		DBG_WRN("SDE HW clock gating enable!\r\n");
	} else {
		clk_set_phase((pdrv_info_data->module_info.pclk[0]), 0);
		clk_set_phase((pdrv_info_data->module_info.p_pclk[0]), 0);
		DBG_WRN("SDE HW clock gating disable!\r\n");
	}

	return size;
}



static int nvt_sde_proc_power_saving_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "sde_fw_power_saving_en = %d\n", g_sde_fw_power_saving_en);
	//nvt_dbg(IND, "sde FW clock gating  = %d\n", g_sde_fw_power_saving_en);//show clock gating. 
	return 0;
}

static int nvt_sde_proc_hw_power_saving_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "sde_hw_power_saving_en = %d\n", g_sde_hw_power_saving_en);
	//nvt_dbg(IND, "sde FW clock gating  = %d\n", g_sde_fw_power_saving_en);//show clock gating. 
	return 0;
}



static int nvt_sde_proc_fw_power_saving_open(struct inode *inode, struct file *file)
{
	//nvt_dbg(IND, "\n");
	return single_open(file, nvt_sde_proc_power_saving_show, &pdrv_info_data->module_info);
}

static int nvt_sde_proc_hw_power_saving_open(struct inode *inode, struct file *file)
{
	//nvt_dbg(IND, "\n");
	return single_open(file, nvt_sde_proc_hw_power_saving_show, &pdrv_info_data->module_info);
}



static struct file_operations proc_fw_power_saving_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_sde_proc_fw_power_saving_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_sde_proc_gating_write
};

static struct file_operations proc_hw_power_saving_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_sde_proc_hw_power_saving_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_sde_proc_hw_gating_write
};



int nvt_sde_proc_init(PSDE_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("kdrv_sde", NULL);
	if (pmodule_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_module_root = pmodule_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pmodule_root, &proc_help_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_help;
	}
	
	pdrv_info->pproc_help_entry = pentry;

	
	pentry = proc_create("fw_power_saving", S_IRUGO | S_IXUGO, pmodule_root, &proc_fw_power_saving_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc fw_gating!\n");
		ret = -EINVAL;
		goto remove_gating;
	}
	
	pdrv_info->pproc_fw_power_saving_entry = pentry;

	pdrv_info_data = pdrv_info;

	pentry = proc_create("hw_power_saving", S_IRUGO | S_IXUGO, pmodule_root, &proc_hw_power_saving_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc hw_gating!\n");
		ret = -EINVAL;
		goto remove_hw_gating;
	}
	
	pdrv_info->pproc_hw_power_saving_entry = pentry;

	pdrv_info_data = pdrv_info;




	

	return ret;

remove_hw_gating:
	proc_remove(pdrv_info->pproc_hw_power_saving_entry);
remove_gating:
	proc_remove(pdrv_info->pproc_fw_power_saving_entry);
remove_help:
	proc_remove(pdrv_info->pproc_help_entry);
remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

int nvt_sde_proc_remove(PSDE_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_fw_power_saving_entry);
	proc_remove(pdrv_info->pproc_hw_power_saving_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}
