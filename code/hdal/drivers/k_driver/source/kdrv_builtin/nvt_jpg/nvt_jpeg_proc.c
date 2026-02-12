#ifdef __KERNEL__
#include <linux/slab.h>
#include <asm/segment.h>
#include <linux/buffer_head.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "jpeg_proc.h"
#include "jpeg_dbg.h"
#include "jpeg_main.h"
#include "jpeg_api.h"



//extern void jpg_emu_enc(void);
//extern void jpg_emu_dec(void);
//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     6


#define get_jiffies()            (jiffies)
#define gm_jiffies_to_msecs(time)   jiffies_to_msecs(time)
#define gm_jiffies_to_usecs(time)   jiffies_to_usecs(time)
#define gm_msecs_to_jiffies(time)   msecs_to_jiffies(time)

#define JPG_JIFFIES   gm_jiffies_to_usecs(get_jiffies())


unsigned int mjd_utilization_period = 5; //5sec calculation
unsigned int mjd_utilization_start, mjd_utilization_record;
unsigned int mjd_engine_start, mjd_engine_end;
unsigned int mjd_engine_time;
BOOL mjd_utilization_show = 0;

/* utilization */
extern unsigned int mjd_utilization_record;
extern unsigned int mjd_utilization_period;

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PJPG_MODULE_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
PJPG_DRV_INFO pdrvjpg_info_data;

//============================================================================
// Function define
//============================================================================

#if (NVT_JPG_TEST_CMD == 1)
//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
	{ "reg",            nvt_jpg_api_read_reg           },

};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
	{ "reg",            nvt_jpg_api_write_reg          },
	{ "pattern",        nvt_jpg_api_write_pattern       }
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))
#endif



static int nvt_jpg_proc_cmd_show(struct seq_file *sfile, void *v)
{
	nvt_dbg(INFO, "\n");
	return 0;
}

static int nvt_jpg_proc_cmd_open(struct inode *inode, struct file *file)
{
	nvt_dbg(INFO, "\n");
	return single_open(file, nvt_jpg_proc_cmd_show, &pdrvjpg_info_data->module_info);
}

static ssize_t nvt_jpg_proc_cmd_write(struct file *file, const char __user *buf,
									  size_t size, loff_t *off)
{

#if 1
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
#if (NVT_JPG_TEST_CMD == 1)
	int ret = -EINVAL;
	unsigned char loop;
#endif

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	if (len > 1) {
		cmd_line[len - 1] = '\0';
	}

	nvt_dbg(INFO, "CMD:%s\n", cmd_line);

	printk("CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

#if (NVT_JPG_TEST_CMD == 1)
	// dispatch command handler
	if (strncmp(argv[0], "r", 2) == 0) {
		for (loop = 0 ; loop < NUM_OF_READ_CMD; loop++) {
			if (strncmp(argv[1], cmd_read_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_read_list[loop].execute(&pdrvjpg_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}
		if (loop >= NUM_OF_READ_CMD) {
			goto ERR_INVALID_CMD;
		}

	} else if (strncmp(argv[0], "w", 2) == 0)  {

		for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&pdrvjpg_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}

		if (loop >= NUM_OF_WRITE_CMD) {
			goto ERR_INVALID_CMD;
		}

	} else if (strncmp(argv[0], "e", 1) == 0)  {

		printk("JPG encode test\r\n");
		//jpg_emu_enc();
	} else if (strncmp(argv[0], "decpri", 6) == 0)  {
		//extern void exam_jpg_dec_primary(void);

		printk("JPG decode primary\r\n");
		//exam_jpg_dec_primary();
	} else if (strncmp(argv[0], "d", 1) == 0)  {

		printk("JPG decode test\r\n");
		//jpg_emu_dec();
	} else {
		goto ERR_INVALID_CMD;
	}
#endif

	return size;

#if (NVT_JPG_TEST_CMD == 1)
ERR_INVALID_CMD:
	nvt_dbg(ERR, "Invalid CMD \"%s\"\n", cmd_line);
#endif

ERR_OUT:
	return -1;
#endif
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_jpg_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_jpg_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_jpg_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " Add message here\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int nvt_jpg_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_jpg_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_jpg_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};


void mjd_mark_engine_start(void)
{
    //if (mjd_engine_start != 0)
        //printk("Warning to nested use mjd_mark_engine_start function!\n");

    mjd_engine_start = JPG_JIFFIES;
	//printk("mjd_engine_start = 0x%x!\r\n",(unsigned int)mjd_engine_start);
    mjd_engine_end = 0;
    if (mjd_utilization_start == 0) {
		//printk("mjd_utilization_start this!\n");
        mjd_utilization_start = mjd_engine_start;
        mjd_engine_time = 0;
    }
}

void mjd_mark_engine_end(void)
{
    //if (mjd_engine_end != 0)
        //printk("Warning to nested use mjd_mark_engine_start function!\n");
    mjd_engine_end = JPG_JIFFIES;
    //printk("mjd_engine_end = 0x%x!\r\n",(unsigned int)mjd_engine_end);

    if (mjd_engine_end > mjd_engine_start)
        mjd_engine_time += mjd_engine_end - mjd_engine_start;
    if (mjd_utilization_start > mjd_engine_end) {
		//printk("mjd_utilization_start > mjd_engine_end\n");
        mjd_utilization_start = 0;
        mjd_engine_time = 0;
    }
    else if((mjd_utilization_start <= mjd_engine_end)&&
        (mjd_engine_end - mjd_utilization_start >= mjd_utilization_period*1000000)) {
        unsigned int utilization;

		//printk("mjd_engine_time = 0x%x!\r\n",(unsigned int)mjd_engine_time);
		//printk("mjd_engine_end = 0x%x!\r\n",(unsigned int)mjd_engine_end);
		//printk("mjd_utilization_start = 0x%x!\r\n",(unsigned int)mjd_utilization_start);
        utilization = (unsigned int)((100*mjd_engine_time)/(mjd_engine_end-mjd_utilization_start));
        if (utilization)
            mjd_utilization_record = utilization;
        mjd_utilization_start = 0;
        mjd_engine_time = 0;
    }
    mjd_engine_start = 0;
}

void mjd_mark_engine_show(void)
{
	if(mjd_utilization_show)
		printk("HW Utilization Period=%d(sec) Utilization=%d\n",
                mjd_utilization_period, mjd_utilization_record);
}

//=============================================================================
// proc "util" file operation functions
//=============================================================================
static int jpg_proc_util_show(struct seq_file *sfile, void *v)
{
	//int i, j;

#if 0
	if (utilization_period == 0){
		seq_printf(sfile, "HW utilization measurement is disabled. set measurment period to non-zero to enable it.\n");
		return 0;
	}
	for (j = 0; j < h26xd_max_chip; j++){
		for (i = 0; i < ENTITY_ENGINES; i++) {
			if (eng_util[j][i].utilization_record == 0){
				seq_printf(sfile, "Chip%d Engine%d HW Utilization Period=%d(sec) Utilization=N/A\n", j, i, utilization_period);
			}
			else{
                #if H26XD_UTILIZATION_BASE_TEN_THOUSAND
                unsigned int uti_num = eng_util[j][i].utilization_record / 100;
                unsigned int uti_den = eng_util[j][i].utilization_record % 100;
                seq_printf(sfile, "Chip%d Engine%d HW Utilization Period=%d(sec) Utilization=%d.%d\n", j, i, utilization_period, uti_num, uti_den);
                #else
				seq_printf(sfile, "Chip%d Engine%d HW Utilization Period=%d(sec) Utilization=%d\n", j, i, utilization_period, eng_util[j][i].utilization_record);
                #endif
			}
		}
	}
#endif

	if(mjd_utilization_show)
		mjd_utilization_show = 0;
	else
		mjd_utilization_show = 1;
	seq_printf(sfile, "HW Utilization Period=%d(sec) Utilization=%d\n",
                mjd_utilization_period, mjd_utilization_record);


	return 0;
}

static int proc_mjd_util_open(struct inode *inode, struct file *file)
{
	return single_open(file, jpg_proc_util_show, NULL);
}
#define PROC_MAX_BUFFER 256
static int proc_mjd_util_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    char proc_buffer[PROC_MAX_BUFFER];
    if (count > sizeof(proc_buffer) - 1) {
        printk("input is too large\n");
        return -EINVAL;
    }
    if (copy_from_user(proc_buffer, buffer, count)) {
        printk("copy from user failed\n");
        return -EFAULT;
    }

    proc_buffer[count] = '\0';

    sscanf(proc_buffer, "%d", &mjd_utilization_period);
    return count;
}

static struct file_operations proc_util_fops = {
	.owner  = THIS_MODULE,
	.open   = proc_mjd_util_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
    .write = proc_mjd_util_write
};

int nvt_jpg_proc_init(PJPG_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("jpg_module", NULL);
	if (pmodule_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_module_root = pmodule_root;

	pentry = proc_create("utilization", S_IRUGO | S_IXUGO, pmodule_root, &proc_util_fops);
	if (pentry == NULL) {
		printk("error to create /utilization proc\n");
		goto remove_cmd;
	}

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
		goto remove_cmd;
	}
	pdrv_info->pproc_help_entry = pentry;


	pdrvjpg_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

int nvt_jpg_proc_remove(PJPG_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}
#endif
