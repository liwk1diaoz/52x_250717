#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "ime_drv.h"
//#include "ime_ioctl.h"
#include "ime_dbg.h"

//#include "ime_int.h"
//#include "ime_lib.h"

#include "ime_platform.h"
//#include "kdrv_videoprocess/kdrv_ipp_config.h"
//#include "kdrv_videoprocess/kdrv_ipp.h"



/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ime_drv_wait_cmd_complete(PIME_INFO pime_info);
//int nvt_ime_drv_ioctl(unsigned char ucIF, IME_INFO *pime_info, unsigned int cmd, unsigned long arg);
void nvt_ime_drv_do_tasklet(unsigned long data);
//irqreturn_t nvt_ime_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
extern void ime_isr(void);

//struct clk *ime_clk[IME_CLK_NUM];
//int i_event_flag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ime_drv_open(PIME_INFO pime_info, unsigned char uc_if)
{
	//nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ime_drv_release(PIME_INFO pime_info, unsigned char uc_if)
{
	//nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	//Free IRQ
	free_irq(pime_info->iinterrupt_id[0], pime_info);

	//ime_release_resource();
	//ime_platform_release_resource();
	//kdrv_ime_uninstall_id();

	//kdrv_ime_uninit();

	return 0;
}

int nvt_ime_drv_init(IME_INFO *pime_info)
{
	int i_ret = 0;
	//struct clk *pclk;

	init_waitqueue_head(&pime_info->ime_wait_queue);

	// coverity[side_effect_free]
	vk_spin_lock_init(&pime_info->ime_spinlock);
	vk_sema_init(&pime_info->ime_sem, 1);
	init_completion(&pime_info->ime_completion);
	tasklet_init(&pime_info->ime_tasklet, nvt_ime_drv_do_tasklet, (unsigned long)pime_info);

#if 0
	/* register IRQ here*/
	if (request_irq(pime_info->iinterrupt_id[0], nvt_ime_drv_isr, IRQF_TRIGGER_HIGH, "IME_INT", pime_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", (int)pime_info->iinterrupt_id[0]);
		i_ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
#endif

	/* Add HW Module initialization here when driver loaded */
	//ime_setBaseAddr((UINT32)pime_info->io_addr[0]);
	//ime_create_resource();

	ime_platform_create_resource(pime_info);
	ime_platform_prepare_clk();
    ime_platform_set_clk_rate(280);
	//kdrv_ime_init();
	//kdrv_ime_install_id();
	//kdrv_ime_flow_init();

	nvt_dbg(IND, "ime initialization...\r\n");
	//printk("ime initialization...\r\n");

	//nvt_dbg(IND, "ime set engine resource...done\r\n");

	return i_ret;

	/* Add error handler here */

#if 0
FAIL_FREE_IRQ:
	free_irq(pime_info->iinterrupt_id[0], pime_info);
	return i_ret;
#endif
}

int nvt_ime_drv_remove(IME_INFO *pime_info)
{

	//Free IRQ
	free_irq(pime_info->iinterrupt_id[0], pime_info);

	/* Add HW Moduel release operation here*/
	ime_platform_unprepare_clk();

	return 0;
}

int nvt_ime_drv_suspend(IME_INFO *pime_info)
{
	//nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ime_drv_resume(IME_INFO *pime_info)
{
	//nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

#if 0
int nvt_ime_drv_ioctl(unsigned char uc_if, IME_INFO *pime_info, unsigned int cmd, unsigned long arg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int i_loop;
	int i_ret = 0;

	//nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, cmd);



	switch (cmd) {
	case IME_IOC_START:
		/*call someone to start operation*/
		break;

	case IME_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case IME_IOC_READ_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)arg, sizeof(REG_INFO));
		if (!i_ret) {
			reg_info.uiValue = READ_REG(pime_info->io_addr[uc_if] + reg_info.uiAddr);
			i_ret = copy_to_user((void __user *)arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case IME_IOC_WRITE_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)arg, sizeof(REG_INFO));
		if (!i_ret) {
			WRITE_REG(reg_info.uiValue, pime_info->io_addr[uc_if] + reg_info.uiAddr);
		}
		break;

	case IME_IOC_READ_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			if (reg_info_list.uiCount <= IME_REG_LIST_NUM) {
				for (i_loop = 0 ; i_loop < reg_info_list.uiCount; i_loop++) {
					reg_info_list.RegList[i_loop].uiValue = READ_REG(pime_info->io_addr[uc_if] + reg_info_list.RegList[i_loop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}

			i_ret = copy_to_user((void __user *)arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case IME_IOC_WRITE_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			if (reg_info_list.uiCount <= IME_REG_LIST_NUM) {
				for (i_loop = 0 ; i_loop < reg_info_list.uiCount ; i_loop++) {
					WRITE_REG(reg_info_list.RegList[i_loop].uiValue, pime_info->io_addr[uc_if] + reg_info_list.RegList[i_loop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}
		}
		break;

		/* Add other operations here */
	}

	return i_ret;
}
#endif

irqreturn_t nvt_ime_drv_isr(int irq, void *devid)
{
	PIME_INFO pime_info = (PIME_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pime_info->ime_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pime_info->ime_tasklet);

	ime_isr();

	return IRQ_HANDLED;
}

int nvt_ime_drv_wait_cmd_complete(PIME_INFO pime_info)
{
	wait_for_completion(&pime_info->ime_completion);
	return 0;
}

int nvt_ime_drv_write_reg(PIME_INFO pime_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pime_info->io_addr[0] + addr);
	return 0;
}

int nvt_ime_drv_read_reg(PIME_INFO pime_info, unsigned long addr)
{
	return READ_REG(pime_info->io_addr[0] + addr);
}

void nvt_ime_drv_do_tasklet(unsigned long data)
{
	PIME_INFO pime_info = (PIME_INFO)data;
	//nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pime_info->ime_completion);
}
