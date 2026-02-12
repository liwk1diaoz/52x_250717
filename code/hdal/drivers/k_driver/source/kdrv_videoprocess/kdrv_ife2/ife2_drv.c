#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>


//#include "ife2_ioctl.h"
#include "ife2_drv.h"
#include "ife2_dbg.h"

//#include "ife2_int.h"
//#include "ife2_lib.h"
#include "ife2_platform.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "kdrv_videoprocess/kdrv_ipp.h"


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ife2_drv_wait_cmd_complete(PIFE2_INFO pife2_info);
//int nvt_ife2_drv_ioctl(unsigned char ucIF, IFE2_INFO *pife2_info, unsigned int cmd, unsigned long arg);
void nvt_ife2_drv_do_tasklet(unsigned long data);
//irqreturn_t nvt_ife2_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
extern VOID ife2_isr(VOID);
//struct clk *ife2_clk[IFE2_CLK_NUM];
//int i_event_flag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ife2_drv_open(PIFE2_INFO pife2_info, unsigned char ucIF)
{
	//nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ife2_drv_release(PIFE2_INFO pife2_info, unsigned char ucIF)
{
	//nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	//Free IRQ
	free_irq(pife2_info->iinterrupt_id[0], pife2_info);

	//ife2_release_resource();
	//ife2_platform_release_resource();
	//kdrv_ife2_uninstall_id();

	kdrv_ife2_uninit();

	return 0;
}

int nvt_ife2_drv_init(IFE2_INFO *pife2_info)
{
	int iRet = 0;
	//struct clk *pclk;

	init_waitqueue_head(&pife2_info->ife2_wait_queue);

	// coverity[side_effect_free]
	vk_spin_lock_init(&pife2_info->ife2_spinlock);
	vk_sema_init(&pife2_info->ife2_sem, 1);
	init_completion(&pife2_info->ife2_completion);
	tasklet_init(&pife2_info->ife2_tasklet, nvt_ife2_drv_do_tasklet, (unsigned long)pife2_info);

#if 0
	/* register IRQ here*/
	if (request_irq(pife2_info->iinterrupt_id[0], nvt_ife2_drv_isr, IRQF_TRIGGER_HIGH, "IFE2_INT", pife2_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", (int)pife2_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
#endif

	/* Add HW Module initialization here when driver loaded */
	//ife2_setBaseAddr((UINT32)pife2_info->io_addr[0]);
	//ife2_create_resource();

	ife2_platform_create_resource(pife2_info);
	ife2_platform_prepare_clk();
	kdrv_ife2_init();
	//kdrv_ife2_install_id();
	//kdrv_ife2_flow_init();

	nvt_dbg(IND, "ife2 initialization...\r\n");

	return iRet;

	/* Add error handler here */

#if 0
FAIL_FREE_IRQ:
	free_irq(pife2_info->iinterrupt_id[0], pife2_info);
	return iRet;
#endif
}

int nvt_ife2_drv_remove(IFE2_INFO *pife2_info)
{

	//Free IRQ
	free_irq(pife2_info->iinterrupt_id[0], pife2_info);

	/* Add HW Moduel release operation here*/
	ife2_platform_unprepare_clk();

	return 0;
}

int nvt_ife2_drv_suspend(IFE2_INFO *pife2_info)
{
	//nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ife2_drv_resume(IFE2_INFO *pife2_info)
{
	//nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

#if 0
int nvt_ife2_drv_ioctl(unsigned char ucIF, IFE2_INFO *pife2_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;

	//nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);



	switch (uiCmd) {
	case IFE2_IOC_START:
		/*call someone to start operation*/
		break;

	case IFE2_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case IFE2_IOC_READ_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			reg_info.uiValue = READ_REG(pife2_info->io_addr[ucIF] + reg_info.uiAddr);
			iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case IFE2_IOC_WRITE_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			WRITE_REG(reg_info.uiValue, pife2_info->io_addr[ucIF] + reg_info.uiAddr);
		}
		break;

	case IFE2_IOC_READ_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= IFE2_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++) {
					reg_info_list.RegList[iLoop].uiValue = READ_REG(pife2_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}

			iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case IFE2_IOC_WRITE_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= IFE2_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++) {
					WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pife2_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
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
#endif

irqreturn_t nvt_ife2_drv_isr(int irq, void *devid)
{
	PIFE2_INFO pife2_info = (PIFE2_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pife2_info->ife2_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pife2_info->ife2_tasklet);

	ife2_isr();

	return IRQ_HANDLED;
}

int nvt_ife2_drv_wait_cmd_complete(PIFE2_INFO pife2_info)
{
	wait_for_completion(&pife2_info->ife2_completion);
	return 0;
}

int nvt_ife2_drv_write_reg(PIFE2_INFO pife2_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pife2_info->io_addr[0] + addr);
	return 0;
}

int nvt_ife2_drv_read_reg(PIFE2_INFO pife2_info, unsigned long addr)
{
	return READ_REG(pife2_info->io_addr[0] + addr);
}

void nvt_ife2_drv_do_tasklet(unsigned long data)
{
	PIFE2_INFO pife2_info = (PIFE2_INFO)data;
	//nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pife2_info->ife2_completion);
}
