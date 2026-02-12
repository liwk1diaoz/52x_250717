#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "dce_drv.h"
//#include "dce_ioctl.h"
//#include "dce_lib.h"
#include "dce_platform.h"//modified:SC
#include "dce_dbg.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
//#include "kdrv_videoprocess/kdrv_ipp_config.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_dce_drv_wait_cmd_complete(PDCE_INFO pmodule_info);
//int nvt_dce_drv_ioctl(unsigned char ucIF, DCE_INFO* pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_dce_drv_do_tasklet(unsigned long data);
//irqreturn_t nvt_dce_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
//int iEventFlag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_dce_drv_open(PDCE_INFO pmodule_info, unsigned char ucIF)
{
	DBG_MSG("%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_dce_drv_release(PDCE_INFO pmodule_info, unsigned char ucIF)
{
	INT32 status = 0;

	/* Add HW Moduel release operation here when device file closed */
	kdrv_dce_uninit();
	//dce_platform_release_resource();
	//kdrv_dce_uninstall_id();

	return status;
}

int nvt_dce_drv_init(DCE_INFO *pmodule_info)
{
	int iRet = 0;
	//struct clk *pclk;

	DBG_MSG("1\r\n");

	init_waitqueue_head(&pmodule_info->dce_wait_queue);
	vk_spin_lock_init(&pmodule_info->dce_spinlock);
	vk_sema_init(&pmodule_info->dce_sem, 1);
	init_completion(&pmodule_info->dce_completion);
	tasklet_init(&pmodule_info->dce_tasklet, nvt_dce_drv_do_tasklet, (unsigned long)pmodule_info);


	DBG_MSG("2\r\n");
#if 0
	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_dce_drv_isr, IRQF_TRIGGER_HIGH, "DCE_INT", pmodule_info)) {
		DBG_ERR("failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
#endif


	//dce_set_base_addr((UINT32)pmodule_info->io_addr[0]);
	//dce_create_resource();

	DBG_MSG("3\r\n");

	/* initial clock here */
	dce_platform_create_resource(pmodule_info);
	dce_platform_prepare_clk();
	kdrv_dce_init();
	//dce_platform_set_clk_rate(360);


	//kdrv_dce_install_id();
	//kdrv_dce_flow_init();


	/* Add HW Module initialization here when driver loaded */

	return iRet;

	/* Add error handler here */

#if 0
FAIL_FREE_IRQ:
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);
	return iRet;
#endif
}

int nvt_dce_drv_remove(DCE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	dce_platform_unprepare_clk();

	return 0;
}

int nvt_dce_drv_suspend(DCE_INFO *pmodule_info)
{
	DBG_MSG("\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_dce_drv_resume(DCE_INFO *pmodule_info)
{
	DBG_MSG("\n");
	/* Add resume operation here*/

	return 0;
}

#if 0
int nvt_dce_drv_ioctl(unsigned char ucIF, DCE_INFO *pmodule_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;

	DBG_MSG("IF-%d cmd:%x\n", ucIF, uiCmd);



	switch (uiCmd) {
	case DCE_IOC_START:
		/*call someone to start operation*/
		break;

	case DCE_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case DCE_IOC_READ_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			reg_info.uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
			iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case DCE_IOC_WRITE_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			WRITE_REG(reg_info.uiValue, pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
		}
		break;

	case DCE_IOC_READ_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= MODULE_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++) {
					reg_info_list.RegList[iLoop].uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}

			iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case DCE_IOC_WRITE_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= MODULE_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++) {
					WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
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

irqreturn_t nvt_dce_drv_isr(int irq, void *devid)
{
	PDCE_INFO pmodule_info = (PDCE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->dce_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->dce_tasklet);

	dce_isr();

	return IRQ_HANDLED;
}

int nvt_dce_drv_wait_cmd_complete(PDCE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->dce_completion);
	return 0;
}

int nvt_dce_drv_write_reg(PDCE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_dce_drv_read_reg(PDCE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_dce_drv_do_tasklet(unsigned long data)
{
	PDCE_INFO pmodule_info = (PDCE_INFO)data;
	//DBG_MSG("\n");

	/* do something you want*/
	complete(&pmodule_info->dce_completion);
}
