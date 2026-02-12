/**
	@brief Source file of IO control of vendor net flow sample.

	@file net_flow_sample_ioctl.c

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kwrap/type.h"
#include "kwrap/util.h"
#include <linux/cdev.h>
#include <kwrap/dev.h>
#include <kwrap/spinlock.h>
#if defined(_BSP_NA51055_)
#include "rtos_na51055/top.h"
#elif defined(_BSP_NA51089_)
#include "rtos_na51089/top.h"
#endif
//#include "frammap/frammap_if.h"
#include "mach/fmem.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/nn_diff.h"
#include "kflow_ai_net_ioctl.h"
#include "kflow_ai_net_parm.h"
#include "../kflow_ai_net_int.h"
//=============================================================
#define __CLASS__ 				"[ai][kflow][ictl]"
#include "kflow_ai_debug.h"
//=============================================================

#include "kdrv_ai.h" //for NEW_AI_FLOW

#include "kflow_ai_net/kflow_ai_core.h"
#include "kflow_ai_net/kflow_ai_core_callback.h"
#include "kflow_ai_net/kflow_ai_net_platform.h"

#include "kflow_cnn/kflow_cnn.h"
#include "kflow_nue/kflow_nue.h"
#include "kflow_nue2/kflow_nue2.h"
#include "kflow_cpu/kflow_cpu.h"
#include "kflow_cpu/kflow_cpu_callback.h"
#include "kflow_dsp/kflow_dsp.h"
#include "kflow_dsp/kflow_dsp_callback.h"

#include "kflow_ai_net_proc.h"
#include "kflow_ai_version.h"


#if CNN_AI_FASTBOOT
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/kflow_ai_builtin_api.h"
#endif


unsigned int kflow_ai_debug_level = NVT_DBG_ERR;
#if defined (__LINUX)
module_param_named(kflow_ai_debug_level, kflow_ai_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(kflow_ai_debug_level, "kflow_ai_debug_level");
#endif
unsigned int vendor_ai_debug_level = NVT_DBG_ERR;
#if defined (__LINUX)
module_param_named(vendor_ai_debug_level, vendor_ai_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vendor_ai_debug_level, "vendor_ai_debug_level");
#endif



extern UINT32 kflow_init;
extern BOOL g_ai_net_init;
extern UINT32 kflow_init_count;
extern UINT32 g_ai_net_init_count;
extern BOOL g_ai_net_path_open[AI_SUPPORT_NET_MAX];
extern BOOL g_ai_net_path_is_used[AI_SUPPORT_NET_MAX];
extern BOOL g_ai_net_path_need_reset[AI_SUPPORT_NET_MAX];
extern BOOL g_ai_net_is_multi_process;
extern BOOL g_ai_net_has_set_is_multi_process;
extern SEM_HANDLE g_ai_path_sem_id;
extern BOOL nvt_ai_is_path_need_reset(VOID);
extern UINT32 kdrv_ai_drv_get_net_supported_num(VOID);

SEM_HANDLE g_kflow_net_sem_id;
/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
#define MODULE_NAME VENDOR_AIS_FLOW_DEV_NAME
#define MODULE_MINOR_COUNT AI_SUPPORT_NET_MAX + 2
typedef struct _KFLOW_AI_NET_DEV {
	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	//struct resource* presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;
	UINT32 minor_base;
} KFLOW_AI_NET_DEV, *PKFLOW_AI_NET_DEV;
static KFLOW_AI_NET_DEV kflow_ai_net_dev;

vk_spinlock_t ioctl_lock;     ///< device ioctl locker
extern VENDOR_AIS_FLOW_MAP_MEM_PARM g_ai_map_mem;

//static void *vendor_ais_buf_va_addr = NULL;     //???!! from structure changed to void
#if CNN_AI_FASTBOOT
int g_is_fboot_first_frame = 1; // fastboot first frame
#endif


/*-----------------------------------------------------------------------------*/
/* external Functions                                                          */
/*-----------------------------------------------------------------------------*/
#if CNN_AI_FASTBOOT
typedef void (* kdrv_ai_nue_isr_t)(VOID);
typedef void (* kdrv_ai_nue2_isr_t)(VOID);
typedef void (* kdrv_ai_cnn_isr_t)(BOOL);
extern void nue_isr(VOID);
extern void nue2_isr(VOID);
extern void cnn_isr(BOOL cnn_id);
extern VOID kdrv_ai_builtin_reg_nue_isr_cb(kdrv_ai_nue_isr_t nue_reg_isr);
extern VOID kdrv_ai_builtin_reg_nue2_isr_cb(kdrv_ai_nue2_isr_t nue2_reg_isr);
extern VOID kdrv_ai_builtin_reg_cnn_isr_cb(kdrv_ai_cnn_isr_t cnn_reg_isr);
extern void nvt_bootts_add_ts(char *name);
#endif


/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
static int vendor_ais_flow_miscdev_open(struct inode *inode, struct file *file)
{
	KFLOW_AI_NET_DEV *pdrv_info = &kflow_ai_net_dev;
	file->private_data = (void *)pdrv_info;
	return 0;
}


extern unsigned int nvt_ai_user2kerl_va(unsigned int addr, UINT32 net_id);
extern unsigned int nvt_ai_kerl2user_va(unsigned int addr, UINT32 net_id);

static void kflow_cpu_exec(KFLOW_AI_JOB* p_job)
{
	if (p_job->state == 13) {   //ABORT
		//force stop (cancel last CPU job for reset after ctrl-c)
		kflow_ai_cpu_sig(p_job->proc_id, p_job);
		return;
	}

#if NN_DLI
	if (p_job->state == 0xff) {   //start
		//call user space to do CPU start
		kflow_ai_cpu_cb2(p_job->proc_id, p_job);
		p_job->state = 0;
		return;
	}
	
	if (p_job->state == 0xfe) {   //stop
		//call user space to do CPU stop
		kflow_ai_cpu_cb3(p_job->proc_id, p_job);
		p_job->state = 0;
		return;
	}
#endif
	
	//call user space to do CPU proc
	kflow_ai_cpu_cb(p_job->proc_id, p_job);
}

static long vendor_ais_flow_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int __user *argp = (int __user *)arg;



	VENDOR_AIS_MEM_INFO mem_info;

	VENDOR_AIS_FLOW_MAP_MEM_INFO map_mem_info;
	VENDOR_AIS_FLOW_CORE_INFO core2_info;
	VENDOR_AIS_FLOW_CORE_CFG core_cfg;
	VENDOR_AIS_FLOW_PROC_INPUT_INFO input_info;
	VENDOR_AIS_FLOW_UPDATE_INFO up_info;
	VENDOR_AIS_FLOW_MEM_OFS mem_ofs;

	VENDOR_AIS_FLOW_ID id_info;
	VENDOR_AIS_FLOW_JOBLIST_INFO joblist_info;
	VENDOR_AIS_FLOW_JOB_INFO job_info;
	KFLOW_AI_NET* p_net;
	KFLOW_AI_JOB* p_job;
	KFLOW_AI_JOB* p_next_job;
	ER er;
#if defined(_BSP_NA51055_)
	extern  UINT32 kflow_chip_old;
#endif

	if (_IOC_TYPE(cmd) != VENDOR_AIS_FLOW_IOC_MAGIC) {
		ret = -ENOIOCTLCMD;
		goto exit;
	}

	switch (cmd) {
	case VENDOR_AIS_FLOW_IOC_MEM_INFO:
		if (copy_from_user(&mem_info, (void __user *)arg, sizeof(VENDOR_AIS_MEM_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_mem_info_update_1(&mem_info);
		ret = 0;
		break;
	case VENDOR_AIS_FLOW_IOC_GET_NUM:
		if (copy_from_user(&id_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_ID))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_get_max_net(&id_info);
		ret = (copy_to_user((void __user *)arg, &id_info, sizeof(VENDOR_AIS_FLOW_ID))) ? (-EFAULT) : 0;
		break;

	case VENDOR_AIS_FLOW_IOC_NET_RESET:
		if (copy_from_user(&id_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_ID))) {
			ret = -EFAULT;
			goto exit;
		}

		if (g_ai_net_is_multi_process) SEM_WAIT(g_kflow_net_sem_id);
		nvt_ai_reset_net();
		if (g_ai_net_is_multi_process) SEM_SIGNAL(g_kflow_net_sem_id);

		ret = (copy_to_user((void __user *)arg, &id_info, sizeof(VENDOR_AIS_FLOW_ID))) ? (-EFAULT) : 0;
		break;

	case VENDOR_AIS_FLOW_IOC_NET_INIT:
		if (copy_from_user(&id_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_ID))) {
			ret = -EFAULT;
			goto exit;
		}
		if (g_ai_net_is_multi_process) SEM_WAIT(g_kflow_net_sem_id);
		nvt_ai_init_net(&id_info);
		if (g_ai_net_is_multi_process) SEM_SIGNAL(g_kflow_net_sem_id);

		ret = (copy_to_user((void __user *)arg, &id_info, sizeof(VENDOR_AIS_FLOW_ID))) ? (-EFAULT) : 0;
		break;

	case VENDOR_AIS_FLOW_IOC_NET_UNINIT:
		/*
		if (copy_from_user(&map_mem_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_ID))) {
			ret = -EFAULT;
			goto exit;
		}
		*/
		if (g_ai_net_is_multi_process) SEM_WAIT(g_kflow_net_sem_id);
		ret = nvt_ai_uninit_net();
		if (g_ai_net_is_multi_process) SEM_SIGNAL(g_kflow_net_sem_id);
		//ret = (copy_to_user((void __user *)arg, &map_mem_info, sizeof(VENDOR_AIS_FLOW_ID))) ? (-EFAULT) : 0;
		break;

	case VENDOR_AIS_FLOW_IOC_NET_LOCK:
		if (copy_from_user(&id_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_ID))) {
			ret = -EFAULT;
			goto exit;
		}
		er = nvt_ai_lock_net(id_info.net_id);
		if (er == E_RLWAI) {
			ret = -ERESTARTSYS; //if lock_net() is NOT interruptted by a ctrl-c signal, this IOCTL will retry again.
			DBG_WRN("proc[%d] NET_LOCK get signal.\r\n", id_info.net_id);
		}
		//ret = (copy_to_user((void __user *)arg, &id_info, sizeof(VENDOR_AIS_FLOW_ID))) ? (-EFAULT) : 0;
		break;

	case VENDOR_AIS_FLOW_IOC_NET_UNLOCK:
		if (copy_from_user(&id_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_ID))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_unlock_net(id_info.net_id);
		//ret = (copy_to_user((void __user *)arg, &id_info, sizeof(VENDOR_AIS_FLOW_ID))) ? (-EFAULT) : 0;
		break;
	
	case VENDOR_AIS_FLOW_IOC_COMMON_LOCK:
		nvt_ai_comm_lock();
		break;

	case VENDOR_AIS_FLOW_IOC_COMMON_UNLOCK:
		nvt_ai_comm_unlock();	
		break;	 

	case VENDOR_AIS_FLOW_IOC_REMAP_ADDR:
		if (copy_from_user(&map_mem_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_MAP_MEM_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_open_net(&map_mem_info.parm, map_mem_info.net_id);
		ret = (copy_to_user((void __user *)arg, &map_mem_info, sizeof(VENDOR_AIS_FLOW_MAP_MEM_INFO))) ? (-EFAULT) : 0;
		break;

	case VENDOR_AIS_FLOW_IOC_UNMAP_ADDR:
		if (copy_from_user(&map_mem_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_MAP_MEM_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_close_net(map_mem_info.net_id);
		nvt_ai_dynamic_batch_uninit(map_mem_info.net_id);
		break;

	case VENDOR_AIS_FLOW_IOC_PARS_MODEL:
		if (copy_from_user(&map_mem_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_MAP_MEM_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		if (nvt_ai_is_dynamic_batch(map_mem_info.net_id))
		{
			nvt_ai_dynamic_batch_init(map_mem_info.net_id);
		}
		nvt_ai_pars_net(&map_mem_info.parm, map_mem_info.net_id);
		break;

#if !CNN_25_MATLAB
	case VENDOR_AIS_FLOW_IOC_UNPARS_MODEL:
		if (copy_from_user(&map_mem_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_MAP_MEM_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_unpars_net(&map_mem_info.parm, map_mem_info.net_id);
		break;
#endif

	case VENDOR_AIS_FLOW_IOC_CORE_RESET:
		/*if (copy_from_user(&core2_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_CORE_INFO))) {
			ret = -EFAULT;
			goto exit;
		}*/
		{
			if (g_ai_net_is_multi_process) {
				SEM_WAIT(g_kflow_net_sem_id);
				if(kflow_init_count == 0 && kflow_init == 1) 
				{
					kflow_ai_core_reset();
					kflow_ai_cpu_uninit_cb();
					kflow_ai_dsp_uninit_cb();
				}
				SEM_SIGNAL(g_kflow_net_sem_id);
			} else {
				kflow_ai_core_reset();
				kflow_ai_cpu_uninit_cb();
				kflow_ai_dsp_uninit_cb();
			}
		}
		/*
		ret = (copy_to_user((void __user *)arg, &core2_info, sizeof(VENDOR_AIS_FLOW_CORE_INFO))) ? (-EFAULT) : 0;
		*/
		break;

	case VENDOR_AIS_FLOW_IOC_CORE_INIT:
		if (copy_from_user(&core2_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_CORE_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		if (g_ai_net_is_multi_process) {
			SEM_WAIT(g_kflow_net_sem_id);
			kflow_init_count += 1;
			if(kflow_init == 0)
			{
				UINT32 max_nue2 = (core2_info.info >> 16) & 0xff;
				UINT32 max_cnn = (core2_info.info >> 8) & 0xff;
				UINT32 max_nue = (core2_info.info) & 0xff;

#if CNN_AI_FASTBOOT
				if (kdrv_ai_is_fastboot()) {
					DBG_WRN("is_fastboot(%d), close kflow_ai_builtin first!\n", kdrv_ai_is_fastboot());
					// close builtin task and kdrv engine
					kflow_ai_builtin_exit();

					// register isr for normal flow
					kdrv_ai_builtin_reg_nue_isr_cb(nue_isr);
					kdrv_ai_builtin_reg_nue2_isr_cb(nue2_isr);
					kdrv_ai_builtin_reg_cnn_isr_cb(cnn_isr);
				}
#endif

				kflow_ai_core_reset_engine();
				kflow_ai_core_add_engine(0, kflow_cnn_get_engine()); // 0 = VENDOR_AI_ENGINE_CNN
				kflow_ai_core_add_engine(1, kflow_nue_get_engine()); // 1 = VENDOR_AI_ENGINE_NUE
				kflow_ai_core_add_engine(2, kflow_nue2_get_engine()); // 2 = VENDOR_AI_ENGINE_NUE2
				kflow_ai_core_add_engine(8, kflow_cpu_get_engine()); // 8 = VENDOR_AI_ENGINE_CPU
				kflow_ai_core_add_engine(9, kflow_dsp_get_engine()); // 9 = VENDOR_AI_ENGINE_DSP
				kflow_cpu_reg_exec_cb(kflow_cpu_exec);
				{
					//KFLOW_AI_ENGINE_CTX* p_eng = kflow_cpu_get_engine();
					//p_eng->p_ch[0]->trigger = kflow_cpu_ch1_trig; //hook
				}

				//set sw-limit
				kflow_ai_core_get_engine(0)->channel_max = max_cnn;
				kflow_ai_core_get_engine(1)->channel_max = max_nue;
				kflow_ai_core_get_engine(2)->channel_max = max_nue2;

				kflow_ai_cpu_init_cb();
				kflow_ai_dsp_init_cb();
				kflow_ai_core_init();

				//get final count
				max_cnn = kflow_ai_core_get_engine(0)->channel_count;
				max_nue = kflow_ai_core_get_engine(1)->channel_count;
				max_nue2 = kflow_ai_core_get_engine(2)->channel_count;

				core2_info.info = (max_nue2 << 16) | (max_cnn << 8) | (max_nue);
			}
			SEM_SIGNAL(g_kflow_net_sem_id);
		} else {
			UINT32 max_nue2 = (core2_info.info >> 16) & 0xff;
			UINT32 max_cnn = (core2_info.info >> 8) & 0xff;
			UINT32 max_nue = (core2_info.info) & 0xff;

#if CNN_AI_FASTBOOT
			if (kdrv_ai_is_fastboot()) {
				DBG_WRN("is_fastboot(%d), close kflow_ai_builtin first!\n", kdrv_ai_is_fastboot());
				// close builtin task and kdrv engine
				kflow_ai_builtin_exit();

				// register isr for normal flow
				kdrv_ai_builtin_reg_nue_isr_cb(nue_isr);
				kdrv_ai_builtin_reg_nue2_isr_cb(nue2_isr);
				kdrv_ai_builtin_reg_cnn_isr_cb(cnn_isr);
			}
#endif

			kflow_ai_core_reset_engine();
			kflow_ai_core_add_engine(0, kflow_cnn_get_engine()); // 0 = VENDOR_AI_ENGINE_CNN
			kflow_ai_core_add_engine(1, kflow_nue_get_engine()); // 1 = VENDOR_AI_ENGINE_NUE
			kflow_ai_core_add_engine(2, kflow_nue2_get_engine()); // 2 = VENDOR_AI_ENGINE_NUE2
			kflow_ai_core_add_engine(8, kflow_cpu_get_engine()); // 8 = VENDOR_AI_ENGINE_CPU
			kflow_ai_core_add_engine(9, kflow_dsp_get_engine()); // 9 = VENDOR_AI_ENGINE_DSP
			kflow_cpu_reg_exec_cb(kflow_cpu_exec);
			{
				//KFLOW_AI_ENGINE_CTX* p_eng = kflow_cpu_get_engine();
				//p_eng->p_ch[0]->trigger = kflow_cpu_ch1_trig; //hook
			}

			//set sw-limit
			kflow_ai_core_get_engine(0)->channel_max = max_cnn;
			kflow_ai_core_get_engine(1)->channel_max = max_nue;
			kflow_ai_core_get_engine(2)->channel_max = max_nue2;

			kflow_ai_cpu_init_cb();
			kflow_ai_dsp_init_cb();
			kflow_ai_core_init();

			//get final count
			max_cnn = kflow_ai_core_get_engine(0)->channel_count;
			max_nue = kflow_ai_core_get_engine(1)->channel_count;
			max_nue2 = kflow_ai_core_get_engine(2)->channel_count;

			core2_info.info = (max_nue2 << 16) | (max_cnn << 8) | (max_nue);
		}
		ret = (copy_to_user((void __user *)arg, &core2_info, sizeof(VENDOR_AIS_FLOW_CORE_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_CORE_UNINIT:
		/*
		if (copy_from_user(&core2_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_CORE_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		*/
		if (g_ai_net_is_multi_process) {
			SEM_WAIT(g_kflow_net_sem_id);
			//kflow_init_count -= 1;
			if(kflow_init_count == 1)
			{
				kflow_ai_core_uninit();
				kflow_ai_cpu_uninit_cb();
				kflow_ai_dsp_uninit_cb();
			}
			SEM_SIGNAL(g_kflow_net_sem_id);
		} else {
			kflow_ai_core_uninit();
			kflow_ai_cpu_uninit_cb();
			kflow_ai_dsp_uninit_cb();
		}
		/*
		ret = (copy_to_user((void __user *)arg, &core2_info, sizeof(VENDOR_AIS_FLOW_CORE_INFO))) ? (-EFAULT) : 0;
		*/
		break;
	case VENDOR_AIS_FLOW_IOC_CORE_CFGSCHD:
		if (copy_from_user(&core_cfg, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_CORE_CFG))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			kflow_ai_core_cfgschd(core_cfg.schd);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_CORE_CFGCHK:
		if (copy_from_user(&core_cfg, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_CORE_CFG))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			kflow_ai_core_cfgchk(core_cfg.schd);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_NEW_JOBLIST:
		if (copy_from_user(&joblist_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOBLIST_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		p_net = kflow_ai_core_net(joblist_info.proc_id);
		//DBG_DUMP("proc[%d] NEW_NET [max_job=%d, job=%d, bind=%d]\r\n", p_net->proc_id, (int)joblist_info.max_job_cnt, (int)joblist_info.job_cnt, (int)joblist_info.bind_cnt);
		kflow_ai_net_create(p_net, joblist_info.max_job_cnt, joblist_info.job_cnt, joblist_info.bind_cnt, joblist_info.ddr_id);
		ret = (copy_to_user((void __user *)arg, &joblist_info, sizeof(VENDOR_AIS_FLOW_JOBLIST_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_DUMP_JOBLIST:
		if (copy_from_user(&joblist_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOBLIST_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		p_net = kflow_ai_core_net(joblist_info.proc_id);
		kflow_ai_net_dump(p_net, joblist_info.job_cnt); //job_cnt = info
		ret = (copy_to_user((void __user *)arg, &joblist_info, sizeof(VENDOR_AIS_FLOW_JOBLIST_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_DEL_JOBLIST:
		if (copy_from_user(&joblist_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOBLIST_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		p_net = kflow_ai_core_net(joblist_info.proc_id);
		kflow_ai_net_destory(p_net);
		ret = (copy_to_user((void __user *)arg, &joblist_info, sizeof(VENDOR_AIS_FLOW_JOBLIST_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_CLR_JOB:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] CLR_JOB, invalid p_net?\r\n", job_info.proc_id);
				ret = 0;
				goto exit;
			}
			p_job = kflow_ai_net_job(p_net, job_info.job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] CLR_JOB, invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id);
				ret = 0;
				goto exit;
			}
			kflow_ai_core_clr_job(p_net, p_job);
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_CLR_JOB3:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_do_iounmap(job_info.proc_id);
		{
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] CLR_JOB, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			p_job = kflow_ai_net_job(p_net, job_info.job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] CLR_JOB, invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id);
				ret = 0;
				goto exit;
			}
			if (p_job == NULL) {
				DBG_ERR("proc[%d] CLR_JOB, invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id);
				ret = 0;
				goto exit;
			}
			kflow_ai_core_clr_job(p_net, p_job);
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_SET_JOB:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			//job_info.info is a modectrl ptr!, convert user va to kernel va
			UINT32 info = nvt_ai_user2kerl_va((unsigned int)job_info.info, job_info.proc_id);
			UINT32 info2 = nvt_ai_user2kerl_va((unsigned int)job_info.info2, job_info.proc_id);
			INT32 wait_ms = job_info.wait_ms;
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] SET_JOB, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			//DBG_DUMP("proc[%d] ADD_JOB [%d]\r\n", p_net->proc_id, (int)job_info.job_id);
			p_job = kflow_ai_net_add_job(p_net, job_info.job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] SET_JOB, invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id);
				ret = 0;
				goto exit;
			}
			p_job->engine_id = job_info.engine_id;
			p_job->engine_op = job_info.engine_op;
			p_job->schd_parm = job_info.schd_parm;
			p_job->wait_ms = job_info.wait_ms;
			p_job->p_eng = kflow_ai_core_get_engine(p_job->engine_id); //assign this job's engine!
			if (p_job->p_eng == 0) {
				DBG_ERR("proc[%d] SET_JOB, job[%d], invalid engine_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id, (int)p_job->engine_id);
				ret = 0;
				goto exit;
			}
			//kflow_ai_core_set_job(p_net, p_job, job_info.engine_id, job_info.engine_op, job_info.schd_parm, (void*)info, (void*)info2, wait_ms);
			kflow_ai_core_set_job(p_net, p_job, (void*)info, (void*)info2, wait_ms);
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_SET_JOB2:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_copy_net_from_user(job_info.proc_id);
		{
			//job_info.info is a modectrl ptr!, convert user va to kernel va
			UINT32 info = 0;
			//UINT32 info2 = 0;
			INT32 wait_ms = job_info.wait_ms;
			info = nvt_ai_user2kerl_va((unsigned int)job_info.info, job_info.proc_id);
			//info2 = nvt_ai_user2kerl_va((unsigned int)job_info.info2, job_info.proc_id);
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] SET_JOB2, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			p_job = kflow_ai_net_add_job(p_net, job_info.job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] SET_JOB2, invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id);
				ret = 0;
				goto exit;
			}
			p_job->engine_id = job_info.engine_id;
			p_job->engine_op = job_info.engine_op;
			p_job->schd_parm = job_info.schd_parm;
			p_job->wait_ms = job_info.wait_ms;
			p_job->p_eng = kflow_ai_core_get_engine(p_job->engine_id); //assign this job's engine!
			if (p_job->p_eng == 0) {
				DBG_ERR("proc[%d] SET_JOB, job[%d], invalid engine_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id, (int)p_job->engine_id);
				ret = 0;
				goto exit;
			}
			//kflow_ai_core_set_job(p_net, p_job, job_info.engine_id, job_info.engine_op, (void*)info, (void*)info2, wait_ms);
			kflow_ai_core_set_job(p_net, p_job, (void*)info, 0, wait_ms);
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_SET_JOB3:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_copy_net_from_user_ll(job_info.proc_id);
		{
			//job_info.info is a modectrl ptr!, convert user va to kernel va
			UINT32 info = 0;
			//UINT32 info2 = 0;
			INT32 wait_ms = job_info.wait_ms;
			info = nvt_ai_user2kerl_va((unsigned int)job_info.info, job_info.proc_id);
			//info2 = nvt_ai_user2kerl_va((unsigned int)job_info.info2, job_info.proc_id);
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] SET_JOB3, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			p_job = kflow_ai_net_add_job(p_net, job_info.job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] SET_JOB3, invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id);
				ret = 0;
				goto exit;
			}
			p_job->engine_id = job_info.engine_id;
			p_job->engine_op = job_info.engine_op;
			p_job->schd_parm = job_info.schd_parm;
			p_job->wait_ms = job_info.wait_ms;
			p_job->p_eng = kflow_ai_core_get_engine(p_job->engine_id); //assign this job's engine!
			if (p_job->p_eng == 0) {
				DBG_ERR("proc[%d] SET_JOB, job[%d], invalid engine_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id, (int)p_job->engine_id);
				ret = 0;
				goto exit;
			}
			//kflow_ai_core_set_job(p_net, p_job, job_info.engine_id, job_info.engine_op, (void*)info, (void*)info2, wait_ms);
			kflow_ai_core_set_job(p_net, p_job, (void*)info, 0, wait_ms);
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_BIND_JOB:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] BIND_JOB, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			//DBG_DUMP("proc[%d] BIND_JOB [%d~%d]\r\n", p_net->proc_id, (int)job_info.job_id, (int)job_info.info);
			p_job = kflow_ai_net_job(p_net, job_info.job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] BIND_JOB, invalid job_id=%d?\r\n", p_net->proc_id, (int)job_info.job_id);
				ret = 0;
				goto exit;
			}
			p_next_job = kflow_ai_net_job(p_net, job_info.info);
			if (p_next_job == NULL) {
				DBG_ERR("proc[%d] BIND_JOB, set end job_id=%d\r\n", p_net->proc_id, (int)job_info.job_id);
			}
			kflow_ai_core_bind_job(p_net, p_job, p_next_job);
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_UNBIND_JOB:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] UNBIND_JOB, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			kflow_ai_core_sum_job(p_net, &job_info.info, &job_info.info2);
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOBLIST_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_LOCK_JOB:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] UOCK_JOB, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			p_job = kflow_ai_net_job(p_net, job_info.job_id);
			if (p_job == 0) {
				job_info.rv = 0;
				job_info.wait_ms = -1;
			} else {

				job_info.rv = kflow_ai_core_lock_job(p_net, p_job);
				job_info.wait_ms = 0;
			}
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_UNLOCK_JOB:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] UNLOCK_JOB, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			p_job = kflow_ai_net_job(p_net, job_info.job_id);
			if (p_job == 0) {
				job_info.rv = 0;
				job_info.wait_ms = -1;
			} else {

				job_info.rv = kflow_ai_core_unlock_job(p_net, p_job);
				if (job_info.rv == -2) {
					DBG_ERR("proc[%d] UNLOCK_JOB abort.\r\n", p_net->proc_id);
				}
				job_info.wait_ms = 0;
			}
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_PUSH_JOB:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] PUSH_JOB, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			if (job_info.job_id >= 0xf0000000) {
				if (job_info.job_id == 0xf0000001) {
					//BEGIN
					kflow_ai_core_push_begin(p_net);
				} else if (job_info.job_id == 0xf0000002) {
					//END
					kflow_ai_core_push_end(p_net);
				}
			} else {
				p_job = kflow_ai_net_job(p_net, job_info.job_id);
				if (p_job == 0) {
					job_info.wait_ms = -1;
				} else {
					p_job->schd_parm = job_info.schd_parm;
					kflow_ai_core_push_job(p_net, p_job);
					job_info.wait_ms = 0;
				}
			}
		}
		ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		break;
	case VENDOR_AIS_FLOW_IOC_PULL_JOB:
		if (copy_from_user(&job_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		{
			p_net = kflow_ai_core_net(job_info.proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] PULL_JOB, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			job_info.rv = 0;
			if (job_info.job_id >= 0xf0000000) {
				if (job_info.job_id == 0xf0000001) {
					//BEGIN
					kflow_ai_core_pull_begin(p_net);
				} else if (job_info.job_id == 0xf0000002) {
					//END
					kflow_ai_core_pull_end(p_net);
				} else if (job_info.job_id == 0xf0000003) {
					//ready
					p_job = 0;
					kflow_ai_core_pull_ready(p_net, p_job);
				} else if (job_info.job_id == 0xf0000004) {
					KFLOW_AI_JOB* pull_job;
					//ready
					pull_job = 0;
					job_info.rv = kflow_ai_core_pull_job(p_net, &pull_job);
					if (job_info.rv == 0) {
						job_info.wait_ms = 0;
						job_info.job_id = pull_job->job_id;
						job_info.info = 0; // 0 => HD_ERR_EOL
						//DBG_ERR("proc[%d] PULL_JOB ok.\r\n", p_net->proc_id);
					} else if (job_info.rv > 0) {
						job_info.wait_ms = 0;
						job_info.job_id = pull_job->job_id;
						job_info.info = 1;  // 1 => HD_OK
						//DBG_ERR("proc[%d] PULL_JOB ok-2.\r\n", p_net->proc_id);
					} else if (job_info.rv == -2) {
						job_info.wait_ms = 0;
						ret = -ERESTARTSYS; //if pull_job() is NOT interruptted by a ctrl-c signal, this IOCTL will retry again.
						DBG_WRN("proc[%d] PULL_JOB get signal.\r\n", p_net->proc_id);
					} else if (job_info.rv == -3) {
						job_info.wait_ms = 0;
						if (p_net->rv == -1) {
							job_info.wait_ms = 0;
							job_info.job_id = pull_job->job_id;
							job_info.info = (UINT32)-1;  // -1 => HD_ERR_FAIL
							DBG_ERR("proc[%d] PULL_JOB fail!\r\n", p_net->proc_id);
						}
						if (p_net->rv == -2) {
							job_info.wait_ms = 0;
							job_info.job_id = pull_job->job_id;
							job_info.info = (UINT32)-2;  // -2 => HD_ERR_TIMEDOUT
							DBG_ERR("proc[%d] PULL_JOB timeout!\r\n", p_net->proc_id);
						}
					} else {    // if (job_info.rv < 0)
						//DBG_DUMP("->>> proc[%d] - IOCTL cancel\r\n", (int)job_info.proc_id);
						job_info.wait_ms = -1;
						DBG_ERR("proc[%d] PULL_JOB error!!!\r\n", p_net->proc_id);
					}
				}
			}
		}
		//ret = (copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO))) ? (-EFAULT) : 0;
		if (unlikely(copy_to_user((void __user *)arg, &job_info, sizeof(VENDOR_AIS_FLOW_JOB_INFO)))) {
			return -EFAULT;
		}
		break;
/*
	case VENDOR_AIS_FLOW_IOC_WAI_JOB: {
			VENDOR_AIS_FLOW_JOB_WAI cmd = {0};
			VENDOR_AIS_FLOW_JOB_WAI *p_cmd  = 0;
			p_cmd  = &cmd;
			if (unlikely(copy_from_user(p_cmd, argp, sizeof(VENDOR_AIS_FLOW_JOB_WAI)))) {
				//DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(VENDOR_AIS_FLOW_JOB_WAI));
				return -EFAULT;
			}
			//DBG_IND("ioctl OUT_WAI: open\r\n");
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			p_job = kflow_ai_core_cb_wait(p_cmd->proc_id);
			if (p_job == NULL) {
				p_cmd->job_id = 0xffffffff;
			} else {
				p_cmd->job_id = kflow_ai_net_job_id(p_net, p_job);
				if (p_cmd->job_id == 0xffffffff) {
					DBG_ERR("proc[%d] WAI_JOB, invalid job_id?\r\n", p_net->proc_id);
				}
			}
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(VENDOR_AIS_FLOW_JOB_WAI)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_JOB_WAI));
				return -EFAULT;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_SIG_JOB: {
			VENDOR_AIS_FLOW_JOB_SIG cmd = {0};
			VENDOR_AIS_FLOW_JOB_SIG *p_cmd  = 0;
			p_cmd  = &cmd;
			if (unlikely(copy_from_user(p_cmd, argp, sizeof(VENDOR_AIS_FLOW_JOB_SIG)))) {
				//DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(VENDOR_AIS_FLOW_JOB_SIG));
				return -EFAULT;
			}
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			//DBG_IND("ioctl OUT_LOG: close\r\n");
			p_job = kflow_ai_net_job(p_net, p_cmd->job_id);
			if (p_job == 0)
			    p_job = (KFLOW_AI_JOB*)0xffffffff;
			kflow_ai_core_cb_sig(p_cmd->proc_id, p_job);
		}
		break;
*/
	case VENDOR_AIS_FLOW_IOC_WAI_CPU: {
			VENDOR_AIS_FLOW_CPU_WAI cmd = {0};
			VENDOR_AIS_FLOW_CPU_WAI *p_cmd  = 0;
#if NN_DLI
			UINT32 event = 0;
#endif
			p_cmd  = &cmd;
			if (unlikely(copy_from_user(p_cmd, argp, sizeof(VENDOR_AIS_FLOW_CPU_WAI)))) {
				//DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(VENDOR_AIS_FLOW_CPU_WAI));
				return -EFAULT;
			}
			//DBG_IND("ioctl OUT_WAI: open\r\n");
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] WAI_CPU, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
#if NN_DLI
			p_job = kflow_ai_cpu_wait(p_cmd->proc_id, &event);
#else
			p_job = kflow_ai_cpu_wait(p_cmd->proc_id);
#endif

			if (p_job == (KFLOW_AI_JOB*)0x0C) { //interrupt by system (include ctrl-c or other signals)
				p_cmd->job_id = 0xffffffff;
				ret = -ERESTARTSYS; //if cpu_wait() is NOT interruptted by a ctrl-c signal, this IOCTL will retry again.
				//DBG_WRN("proc[%d] WAI_CPU get signal.\r\n", p_cmd->proc_id);
			} else if (p_job == NULL) {  //stopped by user (normal case)
				p_cmd->job_id = 0xffffffff;
			} else {
				p_cmd->job_id = kflow_ai_net_job_id(p_net, p_job);
				if (p_cmd->job_id == 0xffffffff) {
					DBG_ERR("proc[%d] WAI_CPU, invalid job_id?\r\n", p_net->proc_id);
				}
#if NN_DLI
				if (event == 0xff) {
					p_cmd->job_id |= 0xff000000; //start
				}
				if (event == 0xfe) {
					p_cmd->job_id |= 0xfe000000; //stop
				}
				//event == 0  //proc
#endif
			}
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(VENDOR_AIS_FLOW_CPU_WAI)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_CPU_WAI));
				return -EFAULT;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_SIG_CPU: {
			VENDOR_AIS_FLOW_CPU_SIG cmd = {0};
			VENDOR_AIS_FLOW_CPU_SIG *p_cmd  = 0;
			p_cmd  = &cmd;
			if (unlikely(copy_from_user(p_cmd, argp, sizeof(VENDOR_AIS_FLOW_CPU_SIG)))) {
				//DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(VENDOR_AIS_FLOW_CPU_SIG));
				return -EFAULT;
			}
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] SIG_CPU, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			//DBG_IND("ioctl OUT_LOG: close\r\n");
#if NN_DLI
			p_cmd->job_id &= ~0xff000000; //clean start/stop flag
#endif
			p_job = kflow_ai_net_job(p_net, p_cmd->job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] SIG_CPU, invalid job_id=%d?\r\n", p_net->proc_id, (int)p_cmd->job_id);
				ret = 0;
				goto exit;
			}
			kflow_ai_cpu_sig(p_cmd->proc_id, p_job);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_WAI_DSP: {
			VENDOR_AIS_FLOW_DSP_WAI cmd = {0};
			VENDOR_AIS_FLOW_DSP_WAI *p_cmd  = 0;
			p_cmd  = &cmd;
			if (unlikely(copy_from_user(p_cmd, argp, sizeof(VENDOR_AIS_FLOW_DSP_WAI)))) {
				//DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(VENDOR_AIS_FLOW_DSP_WAI));
				return -EFAULT;
			}
			//DBG_IND("ioctl OUT_WAI: open\r\n");
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] WAI_DSP, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			p_job = kflow_ai_dsp_wait(p_cmd->proc_id);
			if (p_job == NULL) {
				p_cmd->job_id = 0xffffffff;
			} else {
				p_cmd->job_id = kflow_ai_net_job_id(p_net, p_job);
				if (p_cmd->job_id == 0xffffffff) {
					DBG_ERR("proc[%d] WAI_DSP, invalid job_id?\r\n", p_net->proc_id);
				}
			}
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(VENDOR_AIS_FLOW_DSP_WAI)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_CPU_WAI));
				return -EFAULT;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_SIG_DSP: {
			VENDOR_AIS_FLOW_DSP_SIG cmd = {0};
			VENDOR_AIS_FLOW_DSP_SIG *p_cmd  = 0;
			p_cmd  = &cmd;
			if (unlikely(copy_from_user(p_cmd, argp, sizeof(VENDOR_AIS_FLOW_DSP_SIG)))) {
				//DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(VENDOR_AIS_FLOW_DSP_SIG));
				return -EFAULT;
			}
			p_net = kflow_ai_core_net(p_cmd->proc_id);
			if (p_net == NULL) {
				DBG_ERR("proc[%d] SIG_DSP, invalid p_net?\r\n", p_net->proc_id);
				ret = 0;
				goto exit;
			}
			//DBG_IND("ioctl OUT_LOG: close\r\n");
			p_job = kflow_ai_net_job(p_net, p_cmd->job_id);
			if (p_job == NULL) {
				DBG_ERR("proc[%d] SIG_DSP, invalid job_id=%d?\r\n", p_net->proc_id, (int)p_cmd->job_id);
				ret = 0;
				goto exit;
			}
			kflow_ai_dsp_sig(p_cmd->proc_id, p_job);
		}
		break;

	case VENDOR_AIS_FLOW_IOC_INPUT_INIT:
		if (copy_from_user(&input_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_PROC_INPUT_INFO))) {
			ret = -EFAULT;
			goto exit;
		}

#if CNN_AI_FASTBOOT
		if (g_is_fboot_first_frame) {
			nvt_bootts_add_ts("ai_trans"); // log transition time of ai fastboot (starting time of first frame)
			g_is_fboot_first_frame = 0;
		}
#endif

#if CNN_25_MATLAB
		er = nvt_ai_set_input(input_info.net_addr, &input_info.iomem, input_info.net_id);
#else
		#if CNN_MULTI_INPUT
		er = nvt_ai_set_input2(input_info.net_addr, input_info.imem, NN_IMEM_NUM, input_info.proc_idx, input_info.net_id);
		#else
		er = nvt_ai_set_input(input_info.net_addr, input_info.imem, NN_IMEM_NUM, input_info.net_id);
		#endif
#endif
		if (er != E_OK) {
			ret = er;
			goto exit;
		}
		break;

	case VENDOR_AIS_FLOW_IOC_INPUT_UNINIT:
		if (copy_from_user(&input_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_PROC_INPUT_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		#if CNN_MULTI_INPUT
		// each net_proc() will only call this function once
		// so we need to clear all input map table
		er = nvt_ai_clr_all_input2(input_info.net_addr, input_info.proc_idx, input_info.net_id);
		#else
		er = nvt_ai_clr_input(input_info.net_addr, input_info.net_id);
		#endif
		if (er != E_OK) {
			ret = er;
			goto exit;
		}
		break;

	case VENDOR_AIS_FLOW_IOC_UPDATE_PARM:
		if (copy_from_user(&up_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_UPDATE_INFO))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_update_layer(up_info.layer, up_info.net_id);
		break;

	case VENDOR_AIS_FLOW_IOC_LL_BASE:
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	{
		VENDOR_AIS_FLOW_LL_BASE ll_base = {0};
		if(nvt_get_chip_id() == CHIP_NA51084) { // only 528 IC support
			if (copy_from_user(&ll_base, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_LL_BASE))) {
				ret = -EFAULT;
				goto exit;
			}
			nvt_ai_set_ll_base(&ll_base);
		} else {
			DBG_ERR("VENDOR_AIS_FLOW_IOC_LL_BASE only support for 528\r\n");
		}
	}
#else
		DBG_ERR("VENDOR_AIS_FLOW_IOC_LL_BASE only support for 528\r\n");
#endif
		break;

	// only for FPGA test
	case VENDOR_AIS_FLOW_IOC_MEM_OFS:
		if (copy_from_user(&mem_ofs, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_MEM_OFS))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_ai_set_mem_ofs(&mem_ofs);
		break;

	case VENDOR_AIS_FLOW_IOC_VERS:
		{
			VENDOR_AIS_FLOW_VERS vers_info = {0};

			if (copy_from_user(&vers_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_VERS))) {
				ret = -EFAULT;
				goto exit;
			}

			vers_info.rv = nvt_ai_chk_vers(&vers_info);

			if (unlikely(copy_to_user((void __user *)arg, &vers_info, sizeof(VENDOR_AIS_FLOW_VERS)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_VERS));
				ret = -EFAULT;
				goto exit;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_GET_ID:
		{
			UINT32 proc_id = 0;
			if (copy_from_user(&proc_id, (void __user *)arg, sizeof(UINT32))) {
				ret = -EFAULT;
				goto exit;
			}
			nvt_ai_get_id(&proc_id);
	
			if (unlikely(copy_to_user((void __user *)arg, &proc_id, sizeof(UINT32)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_VERS));
				ret = -EFAULT;
				goto exit;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_RELEASE_ID:
		{
			UINT32 proc_id = 0;
			if (copy_from_user(&proc_id, (void __user *)arg, sizeof(UINT32))) {
				ret = -EFAULT;
				goto exit;
			}

			nvt_ai_release_id(proc_id);

			if (unlikely(copy_to_user((void __user *)arg, &proc_id, sizeof(UINT32)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_VERS));
				ret = -EFAULT;
				goto exit;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_OPEN_PATH:
		{
			UINT32 proc_id = 0;
			if (copy_from_user(&proc_id, (void __user *)arg, sizeof(UINT32))) {
				ret = -EFAULT;
				goto exit;
			}
			SEM_WAIT(g_kflow_net_sem_id);
			if(g_ai_net_path_need_reset[proc_id] == 1) {
				nvt_ai_reset_net_path(proc_id);
				kflow_ai_core_reset_path(proc_id);
				kflow_ai_cpu_uninit_cb_path(proc_id);
				kflow_ai_dsp_uninit_cb_path(proc_id);
				g_ai_net_path_need_reset[proc_id] = 0;
			}
 			g_ai_net_path_open[proc_id] = 1;
			SEM_SIGNAL(g_kflow_net_sem_id);
			if (unlikely(copy_to_user((void __user *)arg, &proc_id, sizeof(UINT32)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_VERS));
				ret = -EFAULT;
				goto exit;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_CLOSE_PATH:
		{
			UINT32 proc_id = 0;
			if (copy_from_user(&proc_id, (void __user *)arg, sizeof(UINT32))) {
				ret = -EFAULT;
				goto exit;
			}
			SEM_WAIT(g_kflow_net_sem_id);
			g_ai_net_path_open[proc_id] = 0;
			SEM_SIGNAL(g_kflow_net_sem_id);
			if (unlikely(copy_to_user((void __user *)arg, &proc_id, sizeof(UINT32)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_VERS));
				ret = -EFAULT;
				goto exit;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_MULTI_PROCESS:
		{
			BOOL is_multi_process = 0;
			if (copy_from_user(&is_multi_process, (void __user *)arg, sizeof(BOOL))) {
				ret = -EFAULT;
				goto exit;
			}
			SEM_WAIT(g_kflow_net_sem_id);
			if(g_ai_net_has_set_is_multi_process == 0) {
				g_ai_net_is_multi_process = is_multi_process;
				g_ai_net_has_set_is_multi_process = 1;
			} else {
				if (is_multi_process != g_ai_net_is_multi_process) {
					DBG_ERR("Current setting of VENDOR_AI_CFG_MULTI_PROCESS = %u, which is different from previous setting = %u\r\n",is_multi_process,g_ai_net_is_multi_process);
					ret = -EFAULT;
					SEM_SIGNAL(g_kflow_net_sem_id);
					goto exit;
				}
				if (is_multi_process == 0 && g_ai_net_is_multi_process == 0) {
					DBG_ERR("multi-process is disable, need to set VENDOR_AI_CFG_MULTI_PROCESS to 1 if you want to run in multi-process mode\r\n");
					ret = -EFAULT;
					SEM_SIGNAL(g_kflow_net_sem_id);
					goto exit;
				}
			}
			SEM_SIGNAL(g_kflow_net_sem_id);
			if (unlikely(copy_to_user((void __user *)arg, &is_multi_process, sizeof(BOOL)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_VERS));
				ret = -EFAULT;
				goto exit;
			}
		}
		break;

	case VENDOR_AIS_FLOW_IOC_PROC_UPDATE_INFO:
		{
			VENDOR_AIS_FLOW_UPDATE_NET_INFO up_net_info = {0};
			UINT32 kerl_va = 0;

			if (copy_from_user(&up_net_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_UPDATE_NET_INFO))) {
				ret = -EFAULT;
				goto exit;
			}
			kerl_va = (UINT32)nvt_ai_pa2va_remap(up_net_info.net_info_pa, up_net_info.net_info_size);
			nvt_ai_update_net_online(&(up_net_info.map_parm), (NN_DIFF_MODEL_HEAD *)kerl_va, up_net_info.model_id, up_net_info.net_id);
			nvt_ai_pa2va_unmap(kerl_va, up_net_info.net_info_pa);
			break;
		}

	case VENDOR_AIS_FLOW_IOC_RESTORE_UPDATE_INFO:
		{
			VENDOR_AIS_FLOW_UPDATE_NET_INFO up_net_info = {0};
			UINT32 kerl_va = 0;

			if (copy_from_user(&up_net_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_UPDATE_NET_INFO))) {
				ret = -EFAULT;
				goto exit;
			}
			kerl_va = (UINT32)nvt_ai_pa2va_remap(up_net_info.net_info_pa, up_net_info.net_info_size);
			nvt_ai_restore_net_online(&up_net_info.map_parm, (NN_DIFF_MODEL_HEAD *)kerl_va, up_net_info.model_id, up_net_info.net_id);
			nvt_ai_pa2va_unmap(kerl_va, up_net_info.net_info_pa);
			break;
		}

	case VENDOR_AIS_FLOW_IOC_PROC_UPDATE_BATCH_INFO:
		{
			VENDOR_AIS_FLOW_UPDATE_NET_INFO up_net_info = {0};
			UINT32 kerl_va = 0;
			if (copy_from_user(&up_net_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_UPDATE_NET_INFO))) {
				ret = -EFAULT;
				goto exit;
			}
			kerl_va = (UINT32)nvt_ai_pa2va_remap(up_net_info.net_info_pa, up_net_info.net_info_size);
			nvt_ai_update_net_online_batch(&up_net_info.map_parm, (NN_DIFF_MODEL_HEAD *)kerl_va, up_net_info.model_id, up_net_info.net_id);
			nvt_ai_pa2va_unmap(kerl_va, up_net_info.net_info_pa);
			break;
		}

	case VENDOR_AIS_FLOW_IOC_RESTORE_UPDATE_BATCH_INFO:
		{
			VENDOR_AIS_FLOW_UPDATE_NET_INFO up_net_info = {0};
			UINT32 kerl_va = 0;
			if (copy_from_user(&up_net_info, (void __user *)arg, sizeof(VENDOR_AIS_FLOW_UPDATE_NET_INFO))) {
				ret = -EFAULT;
				goto exit;
			}
			kerl_va = (UINT32)nvt_ai_pa2va_remap(up_net_info.net_info_pa, up_net_info.net_info_size);
			nvt_ai_restore_net_online_batch(&up_net_info.map_parm, (NN_DIFF_MODEL_HEAD *)kerl_va, up_net_info.model_id, up_net_info.net_id);
			nvt_ai_pa2va_unmap(kerl_va, up_net_info.net_info_pa);
			break;
		}

	case KFLOW_AI_IOC_CMD_DMA_ABORT:
		{
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			kdrv_ai_dma_abort(0, AI_ENG_TOTAL);
#else
			DBG_ERR("KFLOW_AI_IOC_CMD_DMA_ABORT only support for 52x/560\r\n");
#endif
			break;
		}

#if CNN_AI_FASTBOOT
	case VENDOR_AIS_FLOW_IOC_KERL_START_MEM:
		{
			VENDOR_AIS_FLOW_KERL_START_MEM cmd = {0};
			VENDOR_AIS_FLOW_KERL_START_MEM *p_cmd = 0;

			p_cmd  = &cmd;
			if (unlikely(copy_from_user(p_cmd, argp, sizeof(VENDOR_AIS_FLOW_KERL_START_MEM)))) {
				//DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(VENDOR_AIS_FLOW_KERL_START_MEM));
				return -EFAULT;
			}
			if (nvt_ai_get_kerl_start_mem(p_cmd->proc_id, p_cmd) != E_OK) {
				DBG_ERR("nvt_ai_get_kerl_start_mem fail\r\n");
			}
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(VENDOR_AIS_FLOW_KERL_START_MEM)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_KERL_START_MEM));
				return -EFAULT;
			}
			break;
		}

	case VENDOR_AIS_FLOW_IOC_GET_FBOOT_RSLT:
		{
			UINT32 proc_id = 0;
			KFLOW_AI_BUILTIN_RSLT_INFO rslt_info = {0};

			if (kdrv_ai_is_fastboot() == DISABLE) {
				DBG_WRN("Is not fastboot, skip getting results.\r\n");
				return -EFAULT;
			}

			if (kflow_ai_builtin_how_many_in_rsltq(proc_id) > 0) {
				kflow_ai_builtin_get_rslt(proc_id, &rslt_info);
			} else {
				//DBG_ERR("rslt queue empty, get rslt fail!\r\n");
				return -EFAULT;
			}

			if (unlikely(copy_to_user(argp, &rslt_info, sizeof(KFLOW_AI_BUILTIN_RSLT_INFO)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(VENDOR_AIS_FLOW_KERL_START_MEM));
				return -EFAULT;
			}
			break;
		}

	case VENDOR_AIS_FLOW_IOC_GET_FBOOT_DUMP_EN:
		{
			UINT32 proc_id = 0;
			UINT32 enable = 0;

			if (nvt_ai_get_fboot_dump_en(proc_id, &enable) != E_OK) {
				DBG_ERR("nvt_ai_get_fboot_dump_en fail\r\n");
				return -EFAULT;
			}

			if (unlikely(copy_to_user(argp, &enable, sizeof(UINT32)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)&enable, sizeof(UINT32));
				return -EFAULT;
			}
			break;
		}

	case VENDOR_AIS_FLOW_IOC_IS_FASTBOOT:
		{
			INT is_fastboot = 0;
			is_fastboot = kdrv_ai_is_fastboot();
			if (unlikely(copy_to_user(argp, &is_fastboot, sizeof(INT)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)&is_fastboot, sizeof(INT));
				return -EFAULT;
			}
			break;
		}

	case VENDOR_AIS_FLOW_IOC_GET_BUILTIN_MEM:
		{
			KFLOW_AI_BUILTIN_MAP_MEM_PARM builtin_mem = {0};
			VENDOR_AIS_FLOW_MAP_MEM_PARM hdal_mem = {0};
			if (kflow_ai_builtin_get_builtin_mem(&builtin_mem) != E_OK) {
				DBG_ERR("get_builtin_mem fail\r\n");
				return -EFAULT;
			}

			// kerl parm
			hdal_mem.kerl_parm.pa = builtin_mem.kerl_parm.pa;
			hdal_mem.kerl_parm.va = builtin_mem.kerl_parm.va;
			hdal_mem.kerl_parm.size = builtin_mem.kerl_parm.size;
			// user model
			hdal_mem.user_model.pa = builtin_mem.user_model.pa;
			hdal_mem.user_model.va = builtin_mem.user_model.va;
			hdal_mem.user_model.size = builtin_mem.user_model.size;
			// io buff
			hdal_mem.user_buff.pa = builtin_mem.io_buff.pa;
			hdal_mem.user_buff.va = builtin_mem.io_buff.va;
			hdal_mem.user_buff.size = builtin_mem.io_buff.size;

			if (unlikely(copy_to_user(argp, &hdal_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM)))) {
				//DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)&hdal_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));
				return -EFAULT;
			}
			break;
		}
#endif

	case KFLOW_AI_IOC_CMD_OUT_INIT:
		{
			kflow_cmd_out_init();
			break;
		}


	case KFLOW_AI_IOC_CMD_OUT_UNINIT:
		{
			kflow_cmd_out_uninit();
		}
		break;

	case KFLOW_AI_IOC_CMD_OUT_PROG_DEBUG:
		{
			kflow_cmd_out_prog_debug();
		}
		break;

	case KFLOW_AI_IOC_CMD_OUT_RUN_DEBUG:
		{
			kflow_cmd_out_run_debug();
		}
		break;

	case KFLOW_AI_IOC_CMD_PERF_UT_BEGIN:
		{
			//DBG_DUMP("=> api: perf ut begin:\r\n");
			kflow_ai_perf(3, 1);
		}
		break;

	case KFLOW_AI_IOC_CMD_PERF_UT_END:
		{
			KFLOW_AI_PERF_UT p_cmd = {0};

			kflow_ai_perf(0, 1);

			kflow_ai_core_get_perf_ut(&p_cmd);
			if (unlikely(copy_to_user((void __user *)arg, &p_cmd, sizeof(KFLOW_AI_PERF_UT)))) {
				return -EFAULT;
			}
			//DBG_DUMP("=> api: perf ut end:\r\n");
		}
		break;

	case KFLOW_AI_IOC_CMD_OUT_WAI:
		{
			KFLOW_AI_IOC_CMD_OUT p_cmd = {0};

			if (unlikely(copy_from_user((void *)&p_cmd, (void *)arg, sizeof(KFLOW_AI_IOC_CMD_OUT)))) {
				DBG_ERR("copy_from_user fail(%p, %#lx, %u)\r\n", &p_cmd, arg, sizeof(KFLOW_AI_IOC_CMD_OUT));
				return -EFAULT;
			}
			p_cmd.proc_id = kflow_ai_cmd_out_wait(&p_cmd);
			if (unlikely(copy_to_user((void *)arg, &p_cmd, sizeof(KFLOW_AI_IOC_CMD_OUT)))) {
				DBG_ERR("copy_to_user fail(%p, %#lx, %u)\r\n", &p_cmd, arg, sizeof(KFLOW_AI_IOC_CMD_OUT));
				return -EFAULT;
			}
		}
		break;

	case KFLOW_AI_IOC_CMD_OUT_SIG:
		{
			KFLOW_AI_IOC_CMD_OUT p_cmd = {0};

			if (unlikely(copy_from_user((void *)&p_cmd, (void *)arg, sizeof(KFLOW_AI_IOC_CMD_OUT)))) {
				DBG_ERR("copy_from_user fail(%p, %#lx, %u)\r\n", &p_cmd, arg, sizeof(KFLOW_AI_IOC_CMD_OUT));
				return -EFAULT;
			}
			kflow_ai_cmd_out_sig(p_cmd.proc_id, &p_cmd);
			if (unlikely(copy_to_user((void *)arg, &p_cmd, sizeof(KFLOW_AI_IOC_CMD_OUT)))) {
				DBG_ERR("copy_to_user fail(%p, %#lx, %u)\r\n", &p_cmd, arg, sizeof(KFLOW_AI_IOC_CMD_OUT));
				return -EFAULT;
			}
		}
		break;

	case KFLOW_AI_IOC_SET_VERSION:
		{
			KFLOW_AI_IOC_VERSION p_cmd = {0};

			if (unlikely(copy_from_user((void *)&p_cmd, (void *)arg, sizeof(KFLOW_AI_IOC_VERSION)))) {
				DBG_ERR("copy_from_user fail(%p, %#lx, %u)\r\n", &p_cmd, arg, sizeof(KFLOW_AI_IOC_VERSION));
				return -EFAULT;
			}
			ret = kflow_ai_set_lib_version(&p_cmd);
		}
		break;
	case VENDOR_AIS_FLOW_IOC_GET_VER:
		{
			CHAR version_info[32] = KFLOW_AI_IMPL_VERSION;
			ret = copy_to_user((void __user *)arg, version_info, sizeof(KFLOW_AI_IMPL_VERSION));
		}
		break;
	case VENDOR_AIS_FLOW_IOC_GET_FC_CMD_SIZE:
		{
			UINT32 max_weight_h = 0;
			UINT32 size = 0;

			if (copy_from_user(&max_weight_h, (void __user *)arg, sizeof(UINT32))) {
				ret = -EFAULT;
				goto exit;
			}

			size = kdrv_ai_get_fc_cmd_size(max_weight_h);

			if (unlikely(copy_to_user((void __user *)arg, &size, sizeof(UINT32)))) {
				ret = -EFAULT;
				goto exit;
			}
		}
		break;
	case VENDOR_AIS_FLOW_IOC_SET_FC_LL_CMD:
		{
			KFLOW_AI_OP_FC_CMDBUF fc_cmd_buf = {0};
			KDRV_AI_FC_INFO fc_info = {0};

			if (copy_from_user(&fc_cmd_buf, (void __user *)arg, sizeof(KFLOW_AI_OP_FC_CMDBUF))) {
				ret = -EFAULT;
				goto exit;
			}

			//fill fc_info
			fc_info.input_parm_pa = 1; //for prevent check
			fc_info.input_parm_va = nvt_ai_get_fc_input_parm_in_kerl(fc_cmd_buf.proc_id);
			fc_info.input_parm_size = sizeof(KDRV_AI_FC_PARM);
			fc_info.cmd_pa = fc_cmd_buf.buf_pa;
			fc_info.cmd_va = 0;
			fc_info.cmd_size = fc_cmd_buf.buf_size;

			kdrv_ai_set_fc_ll_cmd(&fc_info);
		}
		break;
	case KFLOW_AI_IOC_GET_DEBUG_INFO:
		{
			KFLOW_AI_IOC_DEBUG_INFO p_cmd = {0};
			ret = kflow_ai_get_debug_info(&p_cmd);
			if (unlikely(copy_to_user((void __user *)arg, &p_cmd, sizeof(KFLOW_AI_IOC_DEBUG_INFO)))) {
				ret = -EFAULT;
			}
		}
		break;
	case KFLOW_AI_IOC_GET_DEBUG_LVL:
		{
			KFLOW_AI_IOC_DEBUG_LVL p_cmd = {0};
			ret = kflow_ai_get_debug_lvl(&p_cmd);
			if (unlikely(copy_to_user((void __user *)arg, &p_cmd, sizeof(KFLOW_AI_IOC_DEBUG_LVL)))) {
				ret = -EFAULT;
			}
		}
		break;
#if defined(_BSP_NA51055_)		
	case VENDOR_AIS_FLOW_IOC_CHIP_OLD:
	{
		UINT32 chip_old  = 0 ; 
		if (copy_from_user(&chip_old, (void __user *)arg, sizeof(UINT32))) {
			ret = -EFAULT;
			goto exit;
		}
		chip_old = kflow_chip_old ; 
		
		ret = (copy_to_user((void __user *)arg, &chip_old, sizeof(UINT32))) ? (-EFAULT) : 0;

		break;
	}
#endif	
	default :
		break;
	}

exit:

	return ret;
}

static int vendor_ais_flow_miscdev_release(struct inode *inode, struct file *file)
{
//	if (vendor_ais_buf_va_addr) {
//		if (fmem_release_from_cma(vendor_ais_buf_va_addr, 0) < 0) {
//			return -1;
//		}
//		vendor_ais_buf_va_addr = NULL;
//	}
	KFLOW_AI_NET_DEV *pdrv_info = &kflow_ai_net_dev;
	UINT32 minor = (MINOR(inode->i_rdev) - pdrv_info->minor_base);
	UINT32 net_max_num = kdrv_ai_drv_get_net_supported_num(); // net_max_num should be got from kdriver
	if (g_ai_net_is_multi_process) {
		if (minor > 0 && minor <= net_max_num) {
			SEM_WAIT(g_ai_path_sem_id);
			SEM_WAIT(g_kflow_net_sem_id);
			if(g_ai_net_path_open[minor-1] == 1) {
				kflow_ai_reset_net_path(minor-1);
			}
			SEM_SIGNAL(g_kflow_net_sem_id);
			SEM_SIGNAL(g_ai_path_sem_id);
		}
		else if (minor == 0) {
			SEM_WAIT(g_kflow_net_sem_id);
			// avoid overflow  => ctrl + c happened between doing dla_reset(), dla_init() will cause overflow
			if(g_ai_net_init_count > 0 )
        		g_ai_net_init_count -= 1;
			if(kflow_init_count > 0 ) 
        		kflow_init_count -= 1;
			if(g_ai_net_init_count == 0) {
				g_ai_net_is_multi_process = 0;
				g_ai_net_has_set_is_multi_process = 0;
				if(g_ai_net_init == 1 && nvt_ai_is_path_need_reset() == 0) {
					nvt_ai_uninit_net();
				}
			}
			if(kflow_init_count == 0 && kflow_init == 1 && nvt_ai_is_path_need_reset() == 0) {
				kflow_ai_core_uninit();
				kflow_ai_cpu_uninit_cb();
				kflow_ai_dsp_uninit_cb();
			}
			SEM_SIGNAL(g_kflow_net_sem_id);
		}	
	} else {
		if (minor == 0) {
			SEM_WAIT(g_kflow_net_sem_id);
			g_ai_net_is_multi_process = 0;
			g_ai_net_has_set_is_multi_process = 0;
			SEM_SIGNAL(g_kflow_net_sem_id);
		}
	}
	return 0;
}

#if 0
static ssize_t vendor_ais_flow_miscdev_write(struct file *file, const char __user *buf, size_t count, loff_t *offp)
{
	int ret = 0;
	int bufsize = count;
	vendor_ais_ddr_t *mem;
	KFLOW_AI_NET_DEV *pdev = (KFLOW_AI_NET_DEV *)file->private_data;

	if (pdev->mem_type == 0) {
		mem = &vendor_ais_noncache_buf;
		if (bufsize >= (mem->bufsize - pdev->mem_offset - *offp)) {
			DBG_ERR("skip write file, write file size > noncache size\r\n");
			goto exit;
		}
	} else {
		mem = &vendor_ais_cache_buf;
		if (bufsize >= (mem->bufsize - pdev->mem_offset - *offp)) {
			DBG_ERR("skip write file, write file size > cache size\r\n");
			goto exit;
		}
	}
	if ((pdev->mem_limitsz != 0) && (bufsize > pdev->mem_limitsz)) {
		bufsize = pdev->mem_limitsz;
	}

	if (copy_from_user((void *)(mem->va + pdev->mem_offset + *offp), (void __user *)buf, bufsize)) {
		DBG_ERR("copy from user fail\r\n");
	} else {
		ret = bufsize;
	}

	if (mem->type == ALLOC_CACHEABLE) {
		fmem_dcache_sync((mem->va + pdev->mem_offset + *offp), bufsize, DMA_TO_DEVICE);
	}

exit:

	return ret;
}

static ssize_t vendor_ais_flow_miscdev_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	int ret = 0;
	int bufsize = count;
	vendor_ais_ddr_t *mem;
	KFLOW_AI_NET_DEV *pdev = (KFLOW_AI_NET_DEV *)file->private_data;

	if (pdev->mem_type == 0) {
		mem = &vendor_ais_noncache_buf;
		if (bufsize >= (mem->bufsize - pdev->mem_offset - *pos)) {
			DBG_ERR("skip read mem, read mem size > noncache size\r\n");
			goto exit;
		}
	} else {
		mem = &vendor_ais_cache_buf;
		if (bufsize >= (mem->bufsize - pdev->mem_offset - *pos)) {
			DBG_ERR("skip read mem, read mem size > cache size\r\n");
			goto exit;
		}
	}

	if ((pdev->mem_limitsz != 0) && (bufsize > pdev->mem_limitsz)) {
		bufsize = pdev->mem_limitsz;
	}

	if (mem->type == ALLOC_CACHEABLE) {
		fmem_dcache_sync((mem->va + pdev->mem_offset + *pos), bufsize, DMA_FROM_DEVICE);
	}

	if (copy_to_user((void __user *)buf, (mem->va + pdev->mem_offset + *pos), bufsize)) {
		DBG_ERR("copy to user fail\r\n");
	} else {
		ret = bufsize;
	}

exit:

	return ret;
}
#endif

static struct file_operations vendor_ais_flow_miscdev_fops = {
	.owner          = THIS_MODULE,
	.open           = vendor_ais_flow_miscdev_open,
	.release        = vendor_ais_flow_miscdev_release,
	.unlocked_ioctl = vendor_ais_flow_miscdev_ioctl,
//	.write          = vendor_ais_flow_miscdev_write,
//	.read           = vendor_ais_flow_miscdev_read,
};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int vendor_ais_flow_miscdev_init(void)
{
	int ret = 0;
	unsigned char ucloop;
	int module_count = kdrv_ai_drv_get_net_supported_num() + 2 ; // module_count should be got from kdriver
	KFLOW_AI_NET_DEV *pdrv_info = &kflow_ai_net_dev;
	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, module_count, MODULE_NAME)) {
		pr_err("Can't get device ID\n");
		return -ENODEV;
	}

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &vendor_ais_flow_miscdev_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, module_count)) {
		pr_err("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, MODULE_NAME);
	if(IS_ERR(pdrv_info->pmodule_class)) {
		pr_err("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->minor_base = MINOR(pdrv_info->dev_id);

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (module_count) ; ucloop++) {
		if (ucloop == 0) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
									   , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									   , MODULE_NAME);
		}else if (ucloop == module_count -1 ){
		// reserve the last one 
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
									   , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									   , MODULE_NAME"%d", 129);

		} else {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
									   , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									   , MODULE_NAME"%d", ucloop);
		}
		
		if (IS_ERR(pdrv_info->pdevice[ucloop])) {
			DBG_ERR("failed in creating device%d.\n", ucloop);

			#if (MODULE_MINOR_COUNT > 1)
				for (;ucloop > 0; ucloop--) {
					device_unregister(pdrv_info->pdevice[ucloop - 1]);
				}
			#endif
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
	vk_spin_lock_init(&ioctl_lock);
	kflow_ai_net_proc_create();
	SEM_CREATE(g_kflow_net_sem_id, 1);

	return ret;

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, module_count);

	return ret;
}

void vendor_ais_flow_miscdev_exit(void)
{
	unsigned char ucloop;
	int module_count = kdrv_ai_drv_get_net_supported_num() + 2 ; // module_count should be got from kdriver
	KFLOW_AI_NET_DEV *pdrv_info = &kflow_ai_net_dev;

	//--- clear context ---
    kflow_ai_core_reset();
    kflow_ai_cpu_uninit_cb();
    kflow_ai_dsp_uninit_cb();
    nvt_ai_reset_net();

	SEM_DESTROY(g_kflow_net_sem_id);
	kflow_ai_net_proc_remove();
	for (ucloop = 0 ; ucloop < module_count ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}
	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, module_count);
}

module_init(vendor_ais_flow_miscdev_init);
module_exit(vendor_ais_flow_miscdev_exit);

