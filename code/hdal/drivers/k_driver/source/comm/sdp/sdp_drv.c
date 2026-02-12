#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>


//#include "sdp_ioctl.h"
#include "comm/sdp.h"
#include "sdp_drv.h"
#include "sdp_dbg.h"
#include <asm/io.h>

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_sdp_drv_ioctl(unsigned char ucIF, SDP_MODULE_INFO* pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_sdp_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_sdp_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
static SDP_ID emu_sdp_id = SDP_ID_1;
//static UINT32 idx_mrx;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int nvt_sdp_drv_open(PSDP_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(INFO, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/
	return 0;
}


int nvt_sdp_drv_release(PSDP_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(INFO, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_sdp_drv_init(SDP_MODULE_INFO* pmodule_info)
{
	int iRet = 0;
    nvt_dbg(INFO, "\n");
    //printk(KERN_INFO  "[Ben] \033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__);

	init_waitqueue_head(&pmodule_info->sdp_wait_queue);
	//spin_lock_init(&pmodule_info->sdp_spinlock);
	sema_init(&pmodule_info->sdp_sem, 1);

	/* initial clock here */
	clk_prepare(pmodule_info->pclk[0]);
	clk_enable(pmodule_info->pclk[0]);

	/* register IRQ here*/
	if(request_irq(pmodule_info->iinterrupt_id[0], nvt_sdp_drv_isr, IRQF_TRIGGER_HIGH, "SDP_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
    }

	/* Add HW Module initialization here when driver loaded */
	//_REGIOBASE = (UINT32)pmodule_info->io_addr[0];

    if(1)
    {
        sdp_init((UINT32)pmodule_info->io_addr[0]);

    }
    sdp_platform_init();

	return iRet;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return iRet;
}

int nvt_sdp_drv_remove(SDP_MODULE_INFO* pmodule_info)
{
    nvt_dbg(INFO, "\n");

    sdp_platform_uninit();
    sdp_close(emu_sdp_id);

	//Free IRQ
	pr_info("Free IRQ\n");
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	pr_info("HW Moduel release\n");
	clk_disable(pmodule_info->pclk[0]);
	clk_unprepare(pmodule_info->pclk[0]);
    sdp_reset();

	return 0;
}

int nvt_sdp_drv_suspend(SDP_MODULE_INFO* pmodule_info)
{
	nvt_dbg(INFO, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_sdp_drv_resume(SDP_MODULE_INFO* pmodule_info)
{
	nvt_dbg(INFO, "\n");
	/* Add resume operation here*/

	return 0;
}

irqreturn_t nvt_sdp_drv_isr(int irq, void *devid)
{
    sdp_isr();
    return IRQ_HANDLED;
}

/*-------------------------------------------------------------------------*/

