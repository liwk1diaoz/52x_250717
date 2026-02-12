#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "ife_drv.h"
#include "ife_ioctl.h"
#include "ife_dbg.h"
#include "ife_lib.h"

#include "ife_platform.h"
#include "kdrv_ife_int.h"


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ife_drv_wait_cmd_complete(PIFE_MODULE_INFO pmodule_info);
int nvt_ife_drv_ioctl(unsigned char ucIF, IFE_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_ife_drv_do_tasklet(unsigned long data);
//irqreturn_t nvt_ife_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
//int iEventFlag = 0;
//struct clk *ife_clk[MODULE_CLK_NUM];

extern void ife_isr(void);
//extern void ife_test2(UINT32 uiaddr);

//static UINT32 uiTmpPhyBufAddr = 0x08000000, uiTmpBufSz = 16000000, uiGbl_non_cache_base;
/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ife_drv_open(PIFE_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ife_drv_release(PIFE_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	INT32 status = 0;

	nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	ife_platform_release_resource();

	kdrv_ife_uninstall_id();

	return status;
}

int nvt_ife_drv_init(IFE_MODULE_INFO *pmodule_info)
{
	int iRet = 0;
	//struct clk *pclk;

	init_waitqueue_head(&pmodule_info->ife_wait_queue);

	// coverity[side_effect_free]
	vk_spin_lock_init(&pmodule_info->ife_spinlock);

	vk_sema_init(&pmodule_info->ife_sem, 1);
	init_completion(&pmodule_info->ife_completion);
	tasklet_init(&pmodule_info->ife_tasklet, nvt_ife_drv_do_tasklet, (unsigned long)pmodule_info);

	//ife_platform_create_resource(pmodule_info);
	//ife_platform_prepare_clk();
	//kdrv_ife_init();
	//ife_platform_set_clk_rate(360);

	/* initial clock here */
	/*
	pclk = clk_get(NULL, "fix480m");
	if (IS_ERR(pclk)) {
	    printk("%s: get source fix480m fail\r\n", __func__);
	}
	clk_set_parent(pmodule_info->pclk[0], pclk);
	clk_prepare(pmodule_info->pclk[0]);
	clk_enable(pmodule_info->pclk[0]);
	clk_put(pclk);
	ife_clk[0] = pmodule_info->pclk[0];
	//clk_set_rate(pmodule_info->pclk[0], 240000000);
	*/

#if 0
	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_ife_drv_isr, IRQF_TRIGGER_HIGH, "IFE_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
#endif

	ife_platform_create_resource(pmodule_info);
	ife_platform_prepare_clk();
	kdrv_ife_init();

	/* Add HW Module initialization here when driver loaded */
	//ife_setBaseAddr((UINT32)pmodule_info->io_addr[0]);


	//uiGbl_non_cache_base = ioremap_nocache(uiTmpPhyBufAddr, uiTmpBufSz);
	//nvt_dbg(INFO,"IFE: mapping to uiGbl_non_cache_base = 0x%08x\r\n",uiGbl_non_cache_base);
	//memset(uiGbl_non_cache_base+16*16+16, 0x33, 128);

	//ife_test2(uiTmpPhyBufAddr);

	/*  kdrv_ife_install_id();
	    kdrv_ife_flow_init();*/


	return iRet;

	/* Add error handler here */

#if 0
FAIL_FREE_IRQ:
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);
	return iRet;
#endif
}
#if 0
void ife_test2(UINT32 uiaddr)
{
	IFE_OPENOBJ ObjCB;
	IFE_PARAM FilterInfo = {0};

	ife_open(&ObjCB);

	FilterInfo.SrcFmt = IFE_FMT_RAW;
	FilterInfo.uiWidth = 16;
	FilterInfo.uiHeight = 16;
	FilterInfo.uiInAddr = uiaddr;
	FilterInfo.uiInOfs = 16;
	FilterInfo.uiOutAddr = uiaddr + 16 * 16 + 16;
	FilterInfo.uiOutOfs = 16;

	ife_set_mode(&FilterInfo);
	ife_start();
	ife_wait_flag_frame_end();
}
#endif
int nvt_ife_drv_remove(IFE_MODULE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	ife_platform_unprepare_clk();

	return 0;
}

int nvt_ife_drv_suspend(IFE_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ife_drv_resume(IFE_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_ife_drv_ioctl(unsigned char ucIF, IFE_MODULE_INFO *pmodule_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);



	switch (uiCmd) {
	case IFE_IOC_START:
		/*call someone to start operation*/
		break;

	case IFE_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case IFE_IOC_READ_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			reg_info.uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
			iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case IFE_IOC_WRITE_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			WRITE_REG(reg_info.uiValue, pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
		}
		break;

	case IFE_IOC_READ_REG_LIST:
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
	case IFE_IOC_WRITE_REG_LIST:
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

irqreturn_t nvt_ife_drv_isr(int irq, void *devid)
{
	PIFE_MODULE_INFO pmodule_info = (PIFE_MODULE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->ife_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->ife_tasklet);

	ife_isr();

	return IRQ_HANDLED;
}

int nvt_ife_drv_wait_cmd_complete(PIFE_MODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->ife_completion);
	return 0;
}

int nvt_ife_drv_write_reg(PIFE_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_ife_drv_read_reg(PIFE_MODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_ife_drv_do_tasklet(unsigned long data)
{
	PIFE_MODULE_INFO pmodule_info = (PIFE_MODULE_INFO)data;
	//nvt_dbg(INFO, "\n");

	/* do something you want*/
	complete(&pmodule_info->ife_completion);
}
