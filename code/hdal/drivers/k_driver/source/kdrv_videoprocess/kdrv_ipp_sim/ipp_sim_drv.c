#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "ipp_sim_drv.h"
#include "ipp_sim_ioctl.h"
#include "ipp_sim_dbg.h"

#include "kdrv_videoprocess/kdrv_ipp_config.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ipp_sim_drv_wait_cmd_complete(PIPP_SIM_INFO pipp_sim_info);
int nvt_ipp_sim_drv_ioctl(unsigned char ucIF, IPP_SIM_INFO *pipp_sim_info, unsigned int cmd, unsigned long arg);
void nvt_ipp_sim_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_ipp_sim_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
//extern VOID ipp_sim_isr(VOID);
struct clk *ipp_sim_clk[IPP_SIM_CLK_NUM];
int iEventFlag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ipp_sim_drv_open(PIPP_SIM_INFO pipp_sim_info, unsigned char ucIF)
{
	//nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ipp_sim_drv_release(PIPP_SIM_INFO pipp_sim_info, unsigned char ucIF)
{
	//nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	//Free IRQ
	//free_irq(pipp_sim_info->iinterrupt_id[0], pipp_sim_info);

	//ipp_sim_release_resource();
	//kdrv_ipp_sim_uninstall_id();

	return 0;
}

int nvt_ipp_sim_drv_init(IPP_SIM_INFO *pipp_sim_info)
{
	int iRet = 0;
	//struct clk *pclk;

	//nvt_dbg(IND, "ipp_sim 1...\r\n");

	init_waitqueue_head(&pipp_sim_info->ipp_sim_wait_queue);

	// coverity[side_effect_free]
	spin_lock_init(&pipp_sim_info->ipp_sim_spinlock);
	sema_init(&pipp_sim_info->ipp_sim_sem, 1);

	//nvt_dbg(IND, "ipp_sim 2...\r\n");

	init_completion(&pipp_sim_info->ipp_sim_completion);
	tasklet_init(&pipp_sim_info->ipp_sim_tasklet, nvt_ipp_sim_drv_do_tasklet, (unsigned long)pipp_sim_info);

	/* initial clock here */
	//pclk = clk_get(NULL, "pll13");
	//if (IS_ERR(pclk)) {
	//  printk("%s: get source pll13 fail\r\n", __func__);
	//}

	//clk_set_parent(pipp_sim_info->pclk[0], pclk);
	//clk_prepare(pipp_sim_info->pclk[0]);
	//clk_enable(pipp_sim_info->pclk[0]);
	//clk_put(pclk);
	//clk_set_rate(pime_info->pclk[0], 480000000);
	//ipp_sim_clk[0] = pipp_sim_info->pclk[0];


	/* register IRQ here*/
	//if (request_irq(pipp_sim_info->iinterrupt_id[0], nvt_ipp_sim_drv_isr, IRQF_TRIGGER_HIGH, "IPP_SIM_INT", pipp_sim_info)) {
	//  nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pipp_sim_info->iinterrupt_id[0]);
	//  iRet = -ENODEV;
	//  goto FAIL_FREE_IRQ;
	//}

	/* Add HW Module initialization here when driver loaded */
	//ipp_sim_setBaseAddr((UINT32)pipp_sim_info->io_addr[0]);
	//ipp_sim_create_resource();

	//kdrv_ipp_sim_install_id();
	//kdrv_ipp_sim_flow_init();

	nvt_dbg(IND, "ipp_sim initialization...\r\n");

	return iRet;

//FAIL_FREE_IRQ:

	//free_irq(pipp_sim_info->iinterrupt_id[0], pipp_sim_info);

	/* Add error handler here */

//	return iRet;
}

int nvt_ipp_sim_drv_remove(IPP_SIM_INFO *pipp_sim_info)
{

	//Free IRQ
	//free_irq(pipp_sim_info->iinterrupt_id[0], pipp_sim_info);

	/* Add HW Moduel release operation here*/

	return 0;
}

int nvt_ipp_sim_drv_suspend(IPP_SIM_INFO *pipp_sim_info)
{
	//nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ipp_sim_drv_resume(IPP_SIM_INFO *pipp_sim_info)
{
	//nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_ipp_sim_drv_ioctl(unsigned char ucIF, IPP_SIM_INFO *pipp_sim_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;

	//nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);



	switch (uiCmd) {
	case IPP_SIM_IOC_START:
		/*call someone to start operation*/
		break;

	case IPP_SIM_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case IPP_SIM_IOC_READ_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			reg_info.uiValue = READ_REG(pipp_sim_info->io_addr[ucIF] + reg_info.uiAddr);
			iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case IPP_SIM_IOC_WRITE_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			WRITE_REG(reg_info.uiValue, pipp_sim_info->io_addr[ucIF] + reg_info.uiAddr);
		}
		break;

	case IPP_SIM_IOC_READ_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= IPP_SIM_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++) {
					reg_info_list.RegList[iLoop].uiValue = READ_REG(pipp_sim_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}

			iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case IPP_SIM_IOC_WRITE_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= IPP_SIM_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++) {
					WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pipp_sim_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}
		}
		break;

		/* Add other operations here */
	}

	return iRet;
}

irqreturn_t nvt_ipp_sim_drv_isr(int irq, void *devid)
{
	PIPP_SIM_INFO pipp_sim_info = (PIPP_SIM_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pipp_sim_info->ipp_sim_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pipp_sim_info->ipp_sim_tasklet);

	//ipp_sim_isr();

	return IRQ_HANDLED;
}

int nvt_ipp_sim_drv_wait_cmd_complete(PIPP_SIM_INFO pipp_sim_info)
{
	wait_for_completion(&pipp_sim_info->ipp_sim_completion);
	return 0;
}

int nvt_ipp_sim_drv_write_reg(PIPP_SIM_INFO pipp_sim_info, unsigned long addr, unsigned long value)
{
	//WRITE_REG(value, pipp_sim_info->io_addr[0] + addr);
	return 0;
}

int nvt_ipp_sim_drv_read_reg(PIPP_SIM_INFO pipp_sim_info, unsigned long addr)
{
	//return READ_REG(pipp_sim_info->io_addr[0] + addr);

	return 0;
}

void nvt_ipp_sim_drv_do_tasklet(unsigned long data)
{
	PIPP_SIM_INFO pipp_sim_info = (PIPP_SIM_INFO)data;
	//nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pipp_sim_info->ipp_sim_completion);
}
