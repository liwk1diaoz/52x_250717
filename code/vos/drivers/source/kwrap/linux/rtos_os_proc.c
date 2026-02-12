#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <stdarg.h>
#include "rtos_os_proc.h"

#define __MODULE__    rtos_proc
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/flag.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>

extern int test_vos(int argc, char** argv);
extern int vos_task_debug_dump(int argc, char** argv);
//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 64
#define MAX_ARG_NUM     6

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_proc_debug_level = NVT_DBG_WRN;
static struct seq_file *g_sfile = NULL;
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*pfunc)(int argc, char** argv);
} PROC_CMD, *PPROC_CMD;

#define NUM_OF_PROC_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))
static PROC_CMD cmd_list[] = {
	{ "test_vos", test_vos},
	{ "tskcell", vos_task_debug_dump},
};

DEFINE_SEMAPHORE(vos_proc_lock);
static struct proc_dir_entry *proc_entry_root = NULL;

static int vos_seq_printf(const char *fmt, ...)
{
	char buf[512]= {0};
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	seq_printf(g_sfile, buf);

	return 0;
}

static int cmd_ops_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int cmd_ops_open(struct inode *inode, struct file *file)
{
	return single_open(file, cmd_ops_show, NULL);
}

static ssize_t cmd_ops_write(struct file *file, const char __user *buf,
		size_t size, loff_t *off)
{
	char cmd_line[MAX_CMD_LENGTH];
	const char delimiters[] = {' ', '\r', '\n', '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char argc = 0;

	char *cmdstr = cmd_line;
	unsigned int cmd_idx;
	int ret = -EINVAL;

	// Note: In fact, size will never be smaller than 1 in proc cmd
	//       to prevent coverity warning, we still check the size < 1
	// e.g. cmd [abcde], size = 6
	// e.g. cmd [], size = 1
	if (size > MAX_CMD_LENGTH || size < 1) {
		DBG_ERR("Invalid cmd size %ld\n", (ULONG)size);
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, size))
		goto ERR_OUT;

	cmd_line[size - 1] = '\0';

	DBG_DUMP("VOS_PROC_CMD: [%s], size %ld\n", cmd_line, (ULONG)size);

	// parse command string
	for (argc = 0; argc < MAX_ARG_NUM; argc++) {
		argv[argc] = strsep(&cmdstr, delimiters);

		if (argv[argc] == NULL)
			break;
	}

	// dispatch command handler
	for (cmd_idx = 0 ; cmd_idx < NUM_OF_PROC_CMD; cmd_idx++) {
		if (strncmp(argv[0], cmd_list[cmd_idx].cmd, MAX_CMD_LENGTH) == 0) {
			ret = cmd_list[cmd_idx].pfunc(argc - 1, &argv[1]);
			break;
		}
	}
	if (cmd_idx >= NUM_OF_PROC_CMD)
		goto ERR_INVALID_CMD;

	return size;

ERR_INVALID_CMD:
	DBG_ERR("Invalid CMD [%s]\n", cmd_line);

ERR_OUT:
	return -EINVAL;
}

static struct file_operations cmd_fops = {
	.owner  = THIS_MODULE,
	.open   = cmd_ops_open,
	.read   = seq_read,
	.write  = cmd_ops_write,
	.llseek = seq_lseek,
};

static int help_ops_show(struct seq_file *sfile, void *v)
{
	int cmd_idx;

	seq_printf(sfile, "Usage1:\r\n cat /proc/nvt_vos/sem (or flag, task)\r\n\r\n");
	seq_printf(sfile, "Usage2:\r\n cat [CMD] > /proc/nvt_vos/cmd\r\n\r\n");
	seq_printf(sfile, "CMD could be:\r\n");
	for (cmd_idx = 0 ; cmd_idx < NUM_OF_PROC_CMD; cmd_idx++) {
		seq_printf(sfile, "%s\r\n", cmd_list[cmd_idx].cmd);
	}

	return 0;
}

static int help_ops_open(struct inode *inode, struct file *file)
{
	return single_open(file, help_ops_show, NULL);
}

static struct file_operations help_fops = {
	.owner  = THIS_MODULE,
	.open   = help_ops_open,
	.read   = seq_read,
	.llseek = seq_lseek,
};

static int flag_ops_show(struct seq_file *sfile, void *v)
{
	down(&vos_proc_lock);

	g_sfile = sfile;
	vos_flag_dump(vos_seq_printf);

	up(&vos_proc_lock);

	return 0;
}

static int flag_ops_open(struct inode *inode, struct file *file)
{
	return single_open(file, flag_ops_show, NULL);
}

static struct file_operations flag_fops = {
	.owner  = THIS_MODULE,
	.open   = flag_ops_open,
	.read   = seq_read,
	.llseek = seq_lseek,
};

static int sem_ops_show(struct seq_file *sfile, void *v)
{
	down(&vos_proc_lock);

	g_sfile = sfile;
	vos_sem_dump(vos_seq_printf, 1);

	up(&vos_proc_lock);

	return 0;
}

static int sem_ops_open(struct inode *inode, struct file *file)
{
    return single_open(file, sem_ops_show, NULL);
}

static struct file_operations sem_fops = {
	.owner  = THIS_MODULE,
	.open   = sem_ops_open,
	.read   = seq_read,
	.llseek = seq_lseek,
};

static int task_ops_show(struct seq_file *sfile, void *v)
{
	down(&vos_proc_lock);

	g_sfile = sfile;
	vos_task_dump(vos_seq_printf);

	up(&vos_proc_lock);

	return 0;
}

static int task_ops_open(struct inode *inode, struct file *file)
{
	return single_open(file, task_ops_show, NULL);
}

static struct file_operations task_fops = {
	.owner  = THIS_MODULE,
	.open   = task_ops_open,
	.read   = seq_read,
	.llseek = seq_lseek,
};

void rtos_proc_init(void *param)
{
	int ret = 0;

	proc_entry_root = proc_mkdir("nvt_vos", NULL);
	if (proc_entry_root == NULL) {
		DBG_ERR("failed to create root\n");
		ret = -EINVAL;
		goto rtos_proc_init_err;
	}

	if (NULL == proc_create("cmd", S_IRUGO | S_IXUGO, proc_entry_root, &cmd_fops)) {
		DBG_ERR("failed to create cmd\n");
		ret = -EINVAL;
		goto rtos_proc_init_err;
	}

	if (NULL == proc_create("help", S_IRUGO | S_IXUGO, proc_entry_root, &help_fops)) {
		DBG_ERR("failed to create help\n");
		ret = -EINVAL;
		goto rtos_proc_init_err;
	}

	if (NULL == proc_create("flag", S_IRUGO | S_IXUGO, proc_entry_root, &flag_fops)) {
		DBG_ERR("failed to create flag\n");
		ret = -EINVAL;
		goto rtos_proc_init_err;
	}

	if (NULL == proc_create("sem", S_IRUGO | S_IXUGO, proc_entry_root, &sem_fops)) {
		DBG_ERR("failed to create sem\n");
		ret = -EINVAL;
		goto rtos_proc_init_err;
	}

	if (NULL == proc_create("task", S_IRUGO | S_IXUGO, proc_entry_root, &task_fops)) {
		DBG_ERR("failed to create task\n");
		ret = -EINVAL;
		goto rtos_proc_init_err;
	}

	return;

rtos_proc_init_err:
	if (proc_entry_root) {
		proc_remove(proc_entry_root);
		proc_entry_root = NULL;
	}
}

void rtos_proc_exit(void)
{
	if (proc_entry_root) {
		proc_remove(proc_entry_root);
		proc_entry_root = NULL;
	}
}


