#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "remote_drv.h"
#include "remote_ioctl.h"
#include "remote_dbg.h"
#include "remote_int.h"
#include "remote_platform.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_remote_drv_wait_cmd_complete(PREMOTE_MODULE_INFO pmodule_info);
int nvt_remote_drv_ioctl(unsigned char ucIF, REMOTE_MODULE_INFO* pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_remote_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_remote_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
static void nvt_remote_drv_isr_cb(UINT32 uiCallID)
{
	REMOTE_PATTERN DataPattern;
	static UINT32 EmuRmCnt = 0;

	remote_getDataCommand(&DataPattern);

	EmuRmCnt++;

	if (uiCallID & REMOTE_INT_RD) {
		DBG_DUMP("(%d)RM Data-Rdy  (H)0x%08X  (L)0x%08X\r\n", (int)(EmuRmCnt), (int)(DataPattern.uiHigh), (int)(DataPattern.uiLow));
	}

	if (uiCallID & REMOTE_INT_ERR) {
		DBG_DUMP("(%d)RM Data-Err\r\n", (int)(EmuRmCnt));
	}

	if (uiCallID & REMOTE_INT_MATCH) {
		DBG_DUMP("(%d)RM Match  (H)0x%08X  (L)0x%08X\r\n", (int)(EmuRmCnt), (int)(DataPattern.uiHigh), (int)(DataPattern.uiLow));
	}

	if (uiCallID & REMOTE_INT_REPEAT) {
		DBG_DUMP("(%d)RM Repeat  (H)0x%08X  (L)0x%08X\r\n", (int)(EmuRmCnt), (int)(DataPattern.uiHigh), (int)(DataPattern.uiLow));
	}
}

int nvt_remote_drv_open(PREMOTE_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int nvt_remote_drv_release(PREMOTE_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_remote_drv_init(PREMOTE_MODULE_INFO pmodule_info)
{
	int iRet = 0;

	init_waitqueue_head(&pmodule_info->remote_wait_queue);
	//coverity[side_effect_free]
	spin_lock_init(&pmodule_info->remote_spinlock);
	sema_init(&pmodule_info->remote_sem, 1);

	remote_platform_create_resource(pmodule_info);
	remote_open(nvt_remote_drv_isr_cb);

	/* initial clock here */
	//clk_prepare(pmodule_info->pclk[0]);
	//clk_enable(pmodule_info->pclk[0]);

	/* register IRQ here*/
	if(request_irq(pmodule_info->iinterrupt_id[0], nvt_remote_drv_isr, IRQF_TRIGGER_HIGH, "REMOTE_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ IF:%d Int:%d\n", 0, pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	remote_setEnable(ENABLE);

	/* Add HW Module initialization here when driver loaded */

	return iRet;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return iRet;
}

int nvt_remote_drv_remove(PREMOTE_MODULE_INFO pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	remote_setEnable(DISABLE);
	remote_close();
	remote_platform_release_resource();

	return 0;
}

int nvt_remote_drv_suspend(REMOTE_MODULE_INFO* pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_remote_drv_resume(REMOTE_MODULE_INFO* pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_remote_drv_ioctl(unsigned char ucIF, REMOTE_MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	REMOTE_CONFIG_INFO config_info;
	UINT32 value;
	REMOTE_INTERRUPT interrupt_info;
	int iLoop;
	int iRet = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);

	switch(uiCmd) {
		case REMOTE_IOC_START:
			/*call someone to start operation*/
			break;

		case REMOTE_IOC_STOP:
			/*call someone to stop operation*/
			break;

		case REMOTE_IOC_READ_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if (!iRet) {
		        reg_info.uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
			}
			break;

		case REMOTE_IOC_WRITE_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if (!iRet)
				WRITE_REG(reg_info.uiValue, pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
			break;

		case REMOTE_IOC_READ_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if (!iRet) {
				if (reg_info_list.uiCount <= MODULE_REG_LIST_NUM) {
					for(iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++) {
				        reg_info_list.RegList[iLoop].uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
					}
				} else {
					DBG_ERR("Loop bound error!\r\n");
				}
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
			}
			break;

		case REMOTE_IOC_WRITE_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if (!iRet) {
				if (reg_info_list.uiCount <= MODULE_REG_LIST_NUM) {
					for(iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++) {
						WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
					}
				} else {
					DBG_ERR("Loop bound error!\r\n");
				}
	        }
			break;

		case REMOTE_IOC_SET_ENABLE:
			iRet = copy_from_user(&value, (void __user *)ulArg, sizeof(UINT32));
	        if (!iRet) {
				remote_setEnable(value);
			} else {
				DBG_ERR("Remote set enable error!\r\n");
			}
			break;

		case REMOTE_IOC_SET_CONFIG:
			iRet = copy_from_user(&config_info, (void __user *)ulArg, sizeof(REMOTE_CONFIG_INFO));
	        if (!iRet) {
				remote_setConfig(config_info.id, config_info.value);
			} else {
				DBG_ERR("Remote set config error!\r\n");
			}
			break;

		case REMOTE_IOC_SET_INTERRUPT_ENABLE:
			iRet = copy_from_user(&interrupt_info, (void __user *)ulArg, sizeof(REMOTE_INTERRUPT));
	        if (!iRet) {
				remote_setInterruptEnable(interrupt_info);
			} else {
				DBG_ERR("Remote set interrupt enable error!\r\n");
			}
			break;
	}

	return iRet;
}

irqreturn_t nvt_remote_drv_isr(int irq, void *devid)
{
	remote_isr();
	return IRQ_HANDLED;
}

int nvt_remote_drv_write_reg(PREMOTE_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_remote_drv_read_reg(PREMOTE_MODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

int nvt_remote_drv_set_en(BOOL en)
{
	return remote_setEnable(en);
}

int nvt_remote_drv_set_config(REMOTE_CONFIG_INFO *config_info)
{
	return remote_setConfig(config_info->id, config_info->value);
}

int nvt_remote_drv_set_interrupt_en(REMOTE_INTERRUPT int_en)
{
	return remote_setInterruptEnable(int_en);
}

