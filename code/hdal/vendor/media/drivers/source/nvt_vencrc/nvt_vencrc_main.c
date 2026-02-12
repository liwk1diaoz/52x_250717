#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "kdrv_videoenc/kdrv_videoenc.h"
#include "nvt_vencrc_dbg.h"
#include "h26xenc_rc.h"
#include "h26xenc_rate_control.h"

#if H26X_RC_PROC_ENABLE

#define VEMCRC_PROC_DIR "vencrc"
#define PROC_MAX_BUFFER 256

unsigned int nvt_vencrc_debug_level = (NVT_DBG_WRN | NVT_DBG_ERR);

/********** local parameter **********/
static struct proc_dir_entry *vencrc_entry_proc = NULL;
static struct proc_dir_entry *vencrc_info_proc = NULL;
static struct proc_dir_entry *vencrc_loglevel_proc = NULL;
static struct proc_dir_entry *vencrc_cmd_proc = NULL;

/********** extern parameter **********/
#if (0 == SUPPORT_BUILTIN_RC)
#ifdef __LINUX
extern H26XEncRC *pH26XRCData[KDRV_VDOENC_ID_MAX];
extern unsigned int H26XRCMaxChn;
#endif
#endif

/********** extern function **********/
#if H26X_RC_LOG_VERSION
	extern int h26xEnc_getRcVersion(char *str);
#endif
#if H26X_LOG_RC_INIT_INFO
	extern int h26xEnc_RcGetInitInfo(int chn, H26XEncRCInitInfo *pInfo);
#endif
#if H26X_RC_BR_MEASURE
	extern int h26xEnc_RcSetDumpBR(int log_chn);
#endif
#if H26X_RC_DBG_LEVEL
	extern int h26xEnc_RcSetDbgLv(unsigned int uiDbgLv);
	extern int h26xEnc_RcGetDbgLv(unsigned int *uiDbgLv);
#endif
#if H26X_RC_DBG_CMD
	extern int h26xEnc_RcGetCmd(int cmd_idx, char *cmd);
	extern int h26xEnc_RcSetCmd(char *str);
#endif


/********** proc info functrion **********/
#if H26X_RC_LOG_VERSION
static const char RC_NAME[H26X_RC_NOT_SUPPORT][10] = {"NULL", "CBR", "VBR", "VBR2", "FQP", "EVBR"};
//static const char RC_NAME[H26X_RC_NOT_SUPPORT][10] = {"NULL", "CBR", "VBR", "FixQP", "EVBR"};
static int proc_info_show(struct seq_file *sfile, void *v)
{
    char str[PROC_MAX_BUFFER];
    #if H26X_LOG_RC_INIT_INFO
	H26XEncRCInitInfo rc_info;
    int chn;
    #endif
	#if H26X_RC_LOG_VERSION
    h26xEnc_getRcVersion(str);
	#endif
    seq_printf(sfile, "%s\r\n", str);

    // dump rc init info
    #if H26X_LOG_RC_INIT_INFO
    seq_printf(sfile, "chn   mode      gop/kp     frame rate  bitrate(gop)    I qp      P qp    weight   c.pos    still qp    m.aq  bt  m.size  m.r  qp\n\r");
    seq_printf(sfile, "===  =======  ===========  ==========  ============  ========  ========  =======  =====  ============  ====  ==  ======  ===  ==\n\r");
    for (chn = 0; chn < MAX_LOG_CHN; chn++) {
		if (h26xEnc_RcGetInitInfo(chn, &rc_info) >= 0) {
			if (rc_info.rc_mode < H26X_RC_CBR || rc_info.rc_mode > H26X_RC_EVBR)
				continue;

			if (H26X_RC_FixQp != rc_info.rc_mode) {
				if (rc_info.rc_mode == H26X_RC_EVBR) {
					seq_printf(sfile, "%3d  %-4s(%c)  %3u/%-2u(%3d)   %4u/%-4u   %10lld   %2u/%2u/%2u  %2u/%2u/%2u  %3d/%-3d",
						chn, RC_NAME[rc_info.rc_mode], (int)rc_info.evbr_state_char,
						(unsigned int)rc_info.gop, (unsigned int)rc_info.kp_period, rc_info.smooth_win_size,
						(unsigned int)rc_info.framerate_base, (unsigned int)rc_info.framerate_incr, rc_info.gop_bitrate,
						(unsigned int)rc_info.min_i_qp, (unsigned int)rc_info.init_i_qp, (unsigned int)rc_info.max_i_qp,
						(unsigned int)rc_info.min_p_qp, (unsigned int)rc_info.init_p_qp, (unsigned int)rc_info.max_p_qp,
						rc_info.ip_weight, rc_info.kp_weight);
				}
				else {
					seq_printf(sfile, "%3d    %-4s   %3u/%-2u(%3d)   %4u/%-4u   %10lld   %2u/%2u/%2u  %2u/%2u/%2u  %3d/%-3d",
						chn, RC_NAME[rc_info.rc_mode], (unsigned int)rc_info.gop, (unsigned int)rc_info.kp_period, rc_info.smooth_win_size,
						(unsigned int)rc_info.framerate_base, (unsigned int)rc_info.framerate_incr, rc_info.gop_bitrate,
						(unsigned int)rc_info.min_i_qp, (unsigned int)rc_info.init_i_qp, (unsigned int)rc_info.max_i_qp,
						(unsigned int)rc_info.min_p_qp, (unsigned int)rc_info.init_p_qp, (unsigned int)rc_info.max_p_qp,
						rc_info.ip_weight, rc_info.kp_weight);
				}
			}
			else {
				seq_printf(sfile, "%3d  %-5s  %3u          %4u/%-4u    ----------      %2u        %2u     ---/---",
					chn, RC_NAME[rc_info.rc_mode], (unsigned int)rc_info.gop, (unsigned int)rc_info.framerate_base,
					(unsigned int)rc_info.framerate_incr, (unsigned int)rc_info.init_i_qp, (unsigned int)rc_info.init_p_qp);
			}
			if (H26X_RC_CBR == rc_info.rc_mode || H26X_RC_FixQp == rc_info.rc_mode)
				seq_printf(sfile, "   ---       -----     ");
			else if (H26X_RC_VBR == rc_info.rc_mode || H26X_RC_VBR2 == rc_info.rc_mode)
				seq_printf(sfile, "   %3d       -----     ", (unsigned int)rc_info.change_pos);
			else if (H26X_RC_EVBR == rc_info.rc_mode) {
				seq_printf(sfile, "   ---   %02d/%02d/%02d(%2d)  ", (unsigned int)rc_info.still_i_qp,
					(unsigned int)rc_info.still_kp_qp, (unsigned int)rc_info.still_p_qp, (int)rc_info.still_dqp);
			}
			seq_printf(sfile, "%3d   ", rc_info.motion_aq_str);
			seq_printf(sfile, "%2d  ", rc_info.br_tolerance);
			seq_printf(sfile, "%6u  ", (unsigned int)rc_info.max_frame_size);
			seq_printf(sfile, "%3u  ", (unsigned int)rc_info.motion_ratio);
			seq_printf(sfile, "%d", rc_info.active_qp);
			seq_printf(sfile, "\n");
		}
    }
    #endif

    return 0;
}
static int proc_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_info_show, NULL);
}
static int proc_info_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    char proc_buffer[PROC_MAX_BUFFER];
    int log_chn;
    if (count > sizeof(proc_buffer) - 1) {
        printk("input is too large\n");
        return -EINVAL;
    }
    if (copy_from_user(proc_buffer, buffer, count)) {
        printk("proc log level copy from user failed\n");
        return -EFAULT;
    }
	proc_buffer[count] = '\0';
    sscanf(proc_buffer, "%d", &log_chn);
    h26xEnc_RcSetDumpBR(log_chn);

    return count;
}    
static struct file_operations info_proc_ops = {
    .owner = THIS_MODULE,
    .open = proc_info_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_info_write
};
#endif

/********** proc loglevel functrion **********/
#if H26X_RC_DBG_LEVEL
static int proc_log_level_show(struct seq_file *sfile, void *v)
{
	uint32_t dbg_level;
	h26xEnc_RcGetDbgLv(&dbg_level);

	seq_printf(sfile, "Log level = %d (%d: err, %d: wrn, %d: dbg, %d: info)\r\n",
		(int)dbg_level, RC_LOG_ERR, RC_LOG_WRN, RC_LOG_DBG, RC_LOG_INF);
    return 0;
}
static int proc_log_level_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_log_level_show, NULL);
}
static int proc_log_level_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    char proc_buffer[PROC_MAX_BUFFER];
    int level;
    if (count > sizeof(proc_buffer) - 1) {
        printk("input is too large\n");
        return -EINVAL;
    }
    if (copy_from_user(proc_buffer, buffer, count)) {
        printk("proc log level copy from user failed\n");
        return -EFAULT;
    }
	proc_buffer[count] = '\0';
    sscanf(proc_buffer, "%d", &level);
    h26xEnc_RcSetDbgLv(level);
    return count;
}
static struct file_operations loglevel_proc_ops = {
    .owner = THIS_MODULE,
    .open = proc_log_level_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_log_level_write
};
#endif

/********** proc cmd functrion **********/
#if H26X_RC_DBG_CMD
static int proc_cmd_show(struct seq_file *sfile, void *v)
{
    char cmd[PROC_MAX_BUFFER];
    int i;
    seq_printf(sfile, "usage: echo [param name] [value] > /proc/%s/cmd\n\r", VEMCRC_PROC_DIR);
    seq_printf(sfile, "     parameter        value                      comment\n\r");
    seq_printf(sfile, "====================  =====  ===============================================\n\r");
    for (i = 0; i < MAX_RC_PROC_PARAM_NUM; i++) {
        if (h26xEnc_RcGetCmd(i, cmd) > 0)
            seq_printf(sfile, "%s\n\r", cmd);
    }
    return 0;
}
static int proc_cmd_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_cmd_show, NULL);
}
static int proc_cmd_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    //int cmd_idx, value;    
    char proc_buffer[PROC_MAX_BUFFER];

    if (count > sizeof(proc_buffer) - 1) {
        printk("input is too large\n");
        return -EINVAL;
    }
    if (copy_from_user(proc_buffer, buffer, count)) {
        printk("proc dbg copy from user failed\n");
        return -EFAULT;
    }
	proc_buffer[count] = '\0';

    h26xEnc_RcSetCmd(proc_buffer);

    return count;
}
static struct file_operations cmd_proc_ops = {
    .owner = THIS_MODULE,
    .open = proc_cmd_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_cmd_write
};
#endif	// H26X_RC_DBG_CMD
#endif	// H26X_RC_PROC_ENABLE

/********** proc main functrion **********/
#if H26X_RC_PROC_ENABLE
static int vencrc_proc_close(void)
{
#if H26X_RC_DBG_CMD
    if (vencrc_cmd_proc)
        proc_remove(vencrc_cmd_proc);
#endif
#if H26X_RC_DBG_LEVEL
    if (vencrc_loglevel_proc)
        proc_remove(vencrc_loglevel_proc);
#endif
#if H26X_RC_LOG_VERSION
    if (vencrc_info_proc)
        proc_remove(vencrc_info_proc);
#endif
    if (vencrc_entry_proc)
        proc_remove(vencrc_entry_proc);
    return 0;
}

static int vencrc_proc_init(void)
{
    vencrc_entry_proc = proc_mkdir(VEMCRC_PROC_DIR, NULL);
    if (NULL == vencrc_entry_proc) {
        printk("Error to create %s proc\n", VEMCRC_PROC_DIR);
        goto fail_init_proc;
    }
#if H26X_RC_LOG_VERSION
    vencrc_info_proc = proc_create("info", S_IRUGO | S_IXUGO, vencrc_entry_proc, &info_proc_ops);
    if (NULL == vencrc_info_proc) {
        printk("Error to create %s/info proc\n", VEMCRC_PROC_DIR);
        goto fail_init_proc;
    }
#endif
#if H26X_RC_DBG_LEVEL
    vencrc_loglevel_proc = proc_create("loglevel", S_IRUGO | S_IXUGO, vencrc_entry_proc, &loglevel_proc_ops);
    if (NULL == vencrc_loglevel_proc) {
        printk("Error to create %s/loglevel proc\n", VEMCRC_PROC_DIR);
        goto fail_init_proc;
    }
#endif
#if H26X_RC_DBG_CMD
    vencrc_cmd_proc = proc_create("cmd", S_IRUGO | S_IXUGO, vencrc_entry_proc, &cmd_proc_ops);
    if (NULL == vencrc_cmd_proc) {
        printk("Error to create %s/cmd proc\n", VEMCRC_PROC_DIR);
        goto fail_init_proc;
    }
#endif
    return 0;
fail_init_proc:
    return -EFAULT;
}
#endif

int __init nvt_vencrc_module_init(void)
{
#if (0 == SUPPORT_BUILTIN_RC)
	rc_cb *cb = rc_cb_init();
	
	cb->h26xEnc_RcInit = h26xEnc_RcInit;
	cb->h26xEnc_RcPreparePicture = h26xEnc_RcPreparePicture;
	cb->h26xEnc_RcUpdatePicture = h26xEnc_RcUpdatePicture;
    cb->h26xEnc_RcGetLog = h26xEnc_RcGetLog;
	#ifdef __LINUX
	H26XRCMaxChn = KDRV_VDOENC_ID_MAX;
	{
		int chn;
		for (chn = 0; chn < H26XRCMaxChn; chn++) {
			pH26XRCData[chn] = NULL;
		}
	}
	#endif
#endif
#if H26X_RC_PROC_ENABLE
    vencrc_proc_init();
#endif

	return 0;
}

void __exit nvt_vencrc_module_exit(void)
{
#if (0 == SUPPORT_BUILTIN_RC)
	rc_cb *cb = rc_cb_init();

	cb->h26xEnc_RcInit = NULL;
	cb->h26xEnc_RcPreparePicture = NULL;
	cb->h26xEnc_RcUpdatePicture = NULL;
    cb->h26xEnc_RcGetLog = NULL;
	#ifdef __LINUX
	{
		int chn;
		for (chn = 0; chn < H26XRCMaxChn; chn++) {
			if (pH26XRCData[chn])
				kfree(pH26XRCData[chn]);
			pH26XRCData[chn] = NULL;
		}
	}
	#endif
#endif
#if H26X_RC_PROC_ENABLE
    vencrc_proc_close();
#endif
}

module_init(nvt_vencrc_module_init);
module_exit(nvt_vencrc_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("vencrc driver");
MODULE_LICENSE("Proprietary");
MODULE_VERSION(H26X_RC_VER_STR);

