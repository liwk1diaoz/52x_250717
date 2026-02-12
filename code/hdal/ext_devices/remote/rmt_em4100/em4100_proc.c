
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <kwrap/file.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include <kwrap/semaphore.h>
#include <kwrap/stdio.h>
#include <kwrap/task.h>
#include <kwrap/util.h>

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include "em4100.h"
#include "em4100_proc.h"
#include "rmt_dbg.h"

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     6

#define rfid_read_start     FLGPTN_BIT(0)
#define rfid_read_end       FLGPTN_BIT(1)

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
    char cmd[MAX_CMD_LENGTH];
    int (*execute)(unsigned char argc, char** argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
THREAD_HANDLE rfid_read_task;
ID      rfid_read_flg        = 0;

int g_read_len = 1024;
int g_sleep_time = 100;
int g_decode_mode = 1;
bool g_rfid_task_run_flg = 0;

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_list[] = {
    // keyword          function name
    { "loop",           em4100_proc_loop           },
    { "end",            em4100_proc_close          },
    { "sleep",          em4100_proc_sleep          },
    { "mode",           em4100_proc_mode           },
    { "len",            em4100_proc_length         },
    { "read",           em4100_proc_read_data      },
    { "read2",		    em4100_proc_read_data2     }
};
#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))

/*-------------------------------------------------------------------------*/

static int em4100_read_data(char *rfid_buf)
{
    int len = g_read_len;
    int id = 0, data = 0;

    em4100_api_read(rfid_buf, len, g_decode_mode, &id, &data);

    if(id < 1) {
        pr_info("tag not found.\n");
    }else {
        pr_info("Customer ID: 0x%x\n", id);
        pr_info("Data:        0x%x\n", data);
        //pr_info("Data(DEC):   %d\n", data);
    }

    return 0;
}

int rfid_read_loop(void)
{
    FLGPTN uiFlag = 0;
    int id = 0, data = 0, len = 1024;
    char *rfid_buf = kmalloc(len, GFP_KERNEL);

    pr_info("RFID reader task start.\n");
    while(1) {
        set_flg(rfid_read_flg, rfid_read_start);

        em4100_api_read(rfid_buf, len, g_decode_mode, &id, &data);
        if(id < 1) {
            //pr_info("tag not found.\n");
        }else {
            pr_info("Customer ID: 0x%x\n", id);
            pr_info("Data:        0x%x\n", data);
            //pr_info("Data(DEC):   %d\n", data);
        }
        msleep(g_sleep_time);
        wai_flg(&uiFlag, rfid_read_flg, rfid_read_start | rfid_read_end,
                                                        TWF_ORW | TWF_CLR);

        if (uiFlag & rfid_read_end) {
            g_rfid_task_run_flg = 0;
            kfree(rfid_buf);
            pr_info("RFID reader task close.\r\n");
            THREAD_RETURN(0);
            //return vos_task_return(0);
        }
    }
}

/*-------------------------------------------------------------------------*/

int em4100_proc_loop(unsigned char argc, char **argv)
{
    if(g_rfid_task_run_flg) {
        pr_info("RFID task is already executing.\n");
    } else {
        vos_flag_create(&rfid_read_flg, NULL, "rfid_read_flg");
        rfid_read_task = vos_task_create(rfid_read_loop, NULL,
                                        "rfid_read_task", 10, 4096);
        g_rfid_task_run_flg = 1;
        vos_task_resume(rfid_read_task);
    }
    return 0;
}

int em4100_proc_close(unsigned char argc, char **argv)
{
    if(g_rfid_task_run_flg) {
        g_rfid_task_run_flg = 0;
        set_flg(rfid_read_flg, rfid_read_end);
        vos_util_delay_ms(200);
    } else {
        pr_info("Task is not started.\n");
    }
    return 0;
}

int em4100_proc_sleep(unsigned char argc, char** argv)
{
    if (argc != 1) {
        nvt_dbg(ERR, "wrong argument:%d", argc);
        return -EINVAL;
    } else if (kstrtoint(argv[0], 0, &g_sleep_time) == 0){
        pr_info("Set task sleep time: %d\n", g_sleep_time);
    } else {
        pr_info("error: string conver to int.\n");
        return -1;
    }
    return 0;
}

int em4100_proc_mode(unsigned char argc, char** argv)
{
    if (argc != 1) {
        nvt_dbg(ERR, "wrong argument:%d", argc);
        return -EINVAL;
    } else if (kstrtoint(argv[0], 0, &g_decode_mode) == 0){
        if (g_decode_mode == 1 || g_decode_mode == 2) {
            pr_info("Set decode mode: %d\n", g_decode_mode);
        } else {
            g_decode_mode = 1;
            pr_info("error: out of range\nreset mode = 1\n");
        }
    } else {
        pr_info("error: string conver to int.\n");
        return -1;
    }
    return 0;
}

int em4100_proc_length(unsigned char argc, char **argv)
{
    if (argc != 1) {
        nvt_dbg(ERR, "wrong argument:%d", argc);
        return -EINVAL;
    } else if (kstrtoint(argv[0], 0, &g_read_len) == 0){
        if (g_read_len > 8192 || g_read_len < 1024) {
            pr_info("error: length %d , length range is 1024 to 8192\n",
                                                            g_read_len);
            return -1;
        } else {
            pr_info("Set read len: %d\n", g_read_len);
        }
    } else {
        pr_info("error: string conver to int.\n");
        g_read_len = 1024;
        pr_info("reset read length = 1024 . \n");
        return -1;
    }
    return 0;
}

int em4100_proc_read_data(unsigned char argc, char **argv)
{
    int ret = 0;
    char *rfid_buf = kmalloc(g_read_len, GFP_KERNEL);

    pr_info(KERN_INFO "read buf ksize:%u\n", ksize(rfid_buf));
    ret = em4100_read_data(rfid_buf);
    kfree(rfid_buf);
	return ret;
}

int em4100_proc_read_data2(unsigned char argc, char **argv)
{
    int len = g_read_len;
    char char_buf[128];
    int data_cnt = 0;
    int i, j, l;
    char *rfid_buf = kmalloc(len, GFP_KERNEL);

    em4100_read_data(rfid_buf);
    // print spi read bit
    pr_info("origin data:\n");
    for (i = 0; i < len; i++) {
        l = 1 << 7;
        for (j = 0; j < 8;j++) {
            rfid_buf[i] & l ? (char_buf[data_cnt] = '1') :
                                (char_buf[data_cnt] = '0');
            l = l >> 1;
            data_cnt++;
        }
        char_buf[data_cnt] = ' ';
        data_cnt++;
        if (i % 8 == 7) {
            char_buf[data_cnt] = '\0';
            pr_info("%s\n", char_buf);
            memset(char_buf, 0, 64);
            data_cnt = 0;
        }
    }
    kfree(rfid_buf);
    return 0;
}

/*-------------------------------------------------------------------------*/

ssize_t em4100_proc_write(struct file *filp, const char __user *buf,
                                size_t count, loff_t *f_pos)
{
    int len = count;
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

	if (len == 0)
		cmd_line[0] = '\0';
	else
    	cmd_line[len - 1] = '\0';

	nvt_dbg(IND, "CMD:%s\n", cmd_line);

    // parse command string
    for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
        argv[ucargc] = strsep(&cmdstr, delimiters);

        if (argv[ucargc] == NULL)
            break;
    }

    // dispatch command handler
    for (loop = 0 ; loop < NUM_OF_CMD; loop++) {
        if (strncmp(argv[0], cmd_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
            ret = cmd_list[loop].execute(ucargc - 1, &argv[1]);
            break;
        }
    }
    if (loop >= NUM_OF_CMD)
	    goto ERR_INVALID_CMD;

    return count;

ERR_INVALID_CMD:
    nvt_dbg(ERR, "Invalid CMD \"%s\"\n", cmd_line);

ERR_OUT:
    return -1;
}

ssize_t em4100_proc_read(struct file *filp, char __user *buf, size_t count,
                                loff_t *f_pos)
{
    int ret = 0;
    char *rfid_buf = kmalloc(g_read_len, GFP_KERNEL);

    pr_info(KERN_INFO "read buf ksize:%u\n", ksize(rfid_buf));
    ret = em4100_read_data(rfid_buf);
    kfree(rfid_buf);

	return ret;
}

static int em4100_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "=======================================================\n");
    seq_printf(m, " em4100_proc_show\n");
    seq_printf(m, "=======================================================\n");
    return 0;
}

static int em4100_proc_open(struct inode *inode, struct  file *file)
{
  return single_open(file, em4100_proc_show, NULL);
}

static const struct file_operations em4100_proc_fops = {
  .owner = THIS_MODULE,
  .open = em4100_proc_open,
  .read = em4100_proc_read,
  .write = em4100_proc_write,
  .llseek = seq_lseek,
  .release = single_release,
};

/*-------------------------------------------------------------------------*/

int em4100_proc_init(void)
{
    proc_create("nvt_rmt_em4100_proc", 0, NULL, &em4100_proc_fops);
    return 0;
}

int em4100_proc_exit(void)
{
    while(g_rfid_task_run_flg) {
        set_flg(rfid_read_flg, rfid_read_end);
        msleep(g_sleep_time);
    }
    msleep(100);

    remove_proc_entry("nvt_rmt_em4100_proc", NULL);
    return 0;
}

