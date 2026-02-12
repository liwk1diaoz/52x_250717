#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "sde_drv.h"
#include "sde_ioctl.h"
#include "sde_dbg.h"
#include "sde_lib.h"
#include "kdrv_videoprocess/kdrv_sde.h"
#include "kdrv_sde_int.h"
#include "sde_platform.h"


#define WRITE_REG(VALUE, ADDR)  SDE_SETREG(ADDR, VALUE)//iowrite32(VALUE, ADDR)
#define READ_REG(ADDR)          SDE_GETREG(ADDR)//ioread32(ADDR)




struct clk *sde_clk[SDE_CLK_NUM];

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_sde_drv_wait_cmd_complete(PSDE_INFO pmodule_info);
int nvt_sde_drv_ioctl(unsigned char if_id, SDE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_sde_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_sde_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int i_event_flag = 0;

extern void sde_isr(void);
/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_sde_drv_open(PSDE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_sde_drv_release(PSDE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */
	return 0;
}

int nvt_sde_drv_init(SDE_INFO *pmodule_info)
{
	int err = 0;
	struct clk *pclk;

	init_waitqueue_head(&pmodule_info->sde_wait_queue);
	vk_spin_lock_init(&pmodule_info->sde_spinlock);
	sema_init(&pmodule_info->sde_sem, 1);
	init_completion(&pmodule_info->sde_completion);
	tasklet_init(&pmodule_info->sde_tasklet, nvt_sde_drv_do_tasklet, (unsigned long)pmodule_info);

	/* initial clock here */
	pclk = clk_get(NULL, "fix480m");
	if (IS_ERR(pclk)) {
		printk("%s: get source fix480m fail\r\n", __func__);
	}
	clk_set_parent(pmodule_info->pclk[0], pclk);
	//clk_prepare(pmodule_info->pclk[0]);
	//clk_enable(pmodule_info->pclk[0]);
	clk_put(pclk);
	sde_clk[0] = pmodule_info->pclk[0];
	clk_prepare(sde_clk[0]);

	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_sde_drv_isr, IRQF_TRIGGER_HIGH, "SDE_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		err = -ENODEV;
		goto FAIL_FREE_IRQ;
	}


	/* Add HW Module initialization here when driver loaded */
	//nvt_dbg(IND, "sde set engine resource...\r\n");
	sde_setBaseAddr((UINT32)pmodule_info->io_addr[0]);
	//sde_create_resource();
	
	sde_platform_create_resource(pmodule_info);
	kdrv_sde_install_id();
	kdrv_sde_flow_init();


	//nvt_dbg(IND, "sde set engine resource...done\r\n");
	return err;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return err;
}

int nvt_sde_drv_remove(SDE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	//sde_release_resource();

	kdrv_sde_uninstall_id();

	/* Add HW Moduel release operation here*/
	if (IS_ERR(sde_clk[0])) {
		// to do
	} else {
		clk_unprepare(sde_clk[0]);
	}

	return 0;
}

int nvt_sde_drv_suspend(SDE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_sde_drv_resume(SDE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_sde_drv_ioctl(unsigned char if_id, SDE_INFO *pmodule_info, unsigned int cmd, unsigned long argc)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int loop_count;
	int err = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", if_id, cmd);



	switch (cmd) {
	case SDE_IOC_START:
		/*call someone to start operation*/
		break;

	case SDE_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case SDE_IOC_READ_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err) {
			reg_info.reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info.reg_addr);
			err = copy_to_user((void __user *)argc, &reg_info, sizeof(REG_INFO));
		}
		break;

	case SDE_IOC_WRITE_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err) {
			WRITE_REG(reg_info.reg_value, pmodule_info->io_addr[if_id] + reg_info.reg_addr);
		}
		break;

	case SDE_IOC_READ_REG_LIST:
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
	case SDE_IOC_WRITE_REG_LIST:
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

irqreturn_t nvt_sde_drv_isr(int irq, void *devid)
{
	PSDE_INFO pmodule_info = (PSDE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->sde_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->sde_tasklet);

	sde_isr();
	return IRQ_HANDLED;
}

int nvt_sde_drv_wait_cmd_complete(PSDE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->sde_completion);
	return 0;
}

int nvt_sde_drv_write_reg(PSDE_INFO pmodule_info, unsigned long addr, unsigned long value)
{

	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_sde_drv_read_reg(PSDE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_sde_drv_do_tasklet(unsigned long data)
{
	PSDE_INFO pmodule_info = (PSDE_INFO)data;

	//nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pmodule_info->sde_completion);
}
