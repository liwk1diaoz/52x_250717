#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "ipe_drv.h"
// NOTE: XXXXX
//#include "ipe_ioctl.h"
#include "ipe_dbg.h"
// NOTE: XXXXX
//#include "ipe_lib.h"
#include "ipe_platform.h"
#include "kdrv_ipe_int.h"
#include "kdrv_videoprocess/kdrv_ipp.h"

// NOTE: XXXXX
//struct clk *ipe_clk[MODULE_CLK_NUM];

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ipe_drv_wait_cmd_complete(PIPE_INFO pmodule_info);
// NOTE: XXXXX
//int nvt_ipe_drv_ioctl(unsigned char if_id, IPE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_ipe_drv_do_tasklet(unsigned long data);
//irqreturn_t nvt_ipe_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
//int i_event_flag = 0;

extern void ipe_isr(void);
/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ipe_drv_open(PIPE_INFO pmodule_info, unsigned char if_id)
{
	//nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ipe_drv_release(PIPE_INFO pmodule_info, unsigned char if_id)
{
	INT32 status = 0;

	//nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	kdrv_ipe_uninit();
	//ipe_platform_release_resource();
	//kdrv_ipe_uninstall_id();

	return status;
}

int nvt_ipe_drv_init(IPE_INFO *pmodule_info)
{
	int err = 0;
	// NOTE: XXXXX
	//struct clk *pclk;

	init_waitqueue_head(&pmodule_info->ipe_wait_queue);
	vk_spin_lock_init(&pmodule_info->ipe_spinlock);
	vk_sema_init(&pmodule_info->ipe_sem, 1);
	init_completion(&pmodule_info->ipe_completion);
	tasklet_init(&pmodule_info->ipe_tasklet, nvt_ipe_drv_do_tasklet, (unsigned long)pmodule_info);

#if 0
	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_ipe_drv_isr, IRQF_TRIGGER_HIGH, "IPE_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		err = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
#endif

	/* Add HW Module initialization here when driver loaded */
	//nvt_dbg(IND, "ipe set engine resource...\r\n");
	// NOTE: XXXXX
	//ipe_set_base_addr((UINT32)pmodule_info->io_addr[0]);
	//ipe_create_resource(pmodule_info);

	ipe_platform_create_resource(pmodule_info);
	ipe_platform_prepare_clk();
	kdrv_ipe_init();
	//ipe_platform_set_clk_rate(360);


	//kdrv_ipe_install_id();
	//kdrv_ipe_flow_init();

	//nvt_dbg(IND, "ipe set engine resource...done\r\n");
	return err;

	/* Add error handler here */

#if 0
FAIL_FREE_IRQ:
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);
	return err;
#endif
}

int nvt_ipe_drv_remove(IPE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	ipe_platform_unprepare_clk();

	return 0;
}

int nvt_ipe_drv_suspend(IPE_INFO *pmodule_info)
{
	//nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ipe_drv_resume(IPE_INFO *pmodule_info)
{
	//nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

// NOTE: XXXXX
#if 0
int nvt_ipe_drv_ioctl(unsigned char if_id, IPE_INFO *pmodule_info, unsigned int cmd, unsigned long argc)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int loop_count;
	int err = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", if_id, cmd);



	switch (cmd) {
	case IPE_IOC_START:
		/*call someone to start operation*/
		break;

	case IPE_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case IPE_IOC_READ_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err) {
			reg_info.reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info.reg_addr);
			err = copy_to_user((void __user *)argc, &reg_info, sizeof(REG_INFO));
		}
		break;

	case IPE_IOC_WRITE_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err) {
			WRITE_REG(reg_info.reg_value, pmodule_info->io_addr[if_id] + reg_info.reg_addr);
		}
		break;

	case IPE_IOC_READ_REG_LIST:
		err = copy_from_user(&reg_info_list, (void __user *)argc, sizeof(REG_INFO_LIST));
		if (!err) {
			if (reg_info_list.reg_cnt <= MODULE_REG_LIST_NUM) {
				for (loop_count = 0 ; loop_count < reg_info_list.reg_cnt ; loop_count++) {
					reg_info_list.reg_list[loop_count].reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info_list.reg_list[loop_count].reg_addr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}
			err = copy_to_user((void __user *)argc, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case IPE_IOC_WRITE_REG_LIST:
		err = copy_from_user(&reg_info_list, (void __user *)argc, sizeof(REG_INFO_LIST));
		if (!err) {
			if (reg_info_list.reg_cnt <= MODULE_REG_LIST_NUM) {
				for (loop_count = 0 ; loop_count < reg_info_list.reg_cnt ; loop_count++) {
					WRITE_REG(reg_info_list.reg_list[loop_count].reg_value, pmodule_info->io_addr[if_id] + reg_info_list.reg_list[loop_count].reg_addr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}
		}
		break;

		/* Add other operations here */
	}

	return err;
}
#endif

irqreturn_t nvt_ipe_drv_isr(int irq, void *devid)
{
	PIPE_INFO pmodule_info = (PIPE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->ipe_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->ipe_tasklet);

	ipe_isr();
	return IRQ_HANDLED;
}

int nvt_ipe_drv_wait_cmd_complete(PIPE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->ipe_completion);
	return 0;
}

int nvt_ipe_drv_write_reg(PIPE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_ipe_drv_read_reg(PIPE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_ipe_drv_do_tasklet(unsigned long data)
{
	PIPE_INFO pmodule_info = (PIPE_INFO)data;

	//nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pmodule_info->ipe_completion);
}
