#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include <plat/efuse_protected.h>

#include "vpe_drv.h"
//#include "vpe_ioctl.h"
#include "vpe_dbg.h"

//#include "vpe_int.h"
//#include "vpe_lib.h"

#include "vpe_platform.h"
#include "kdrv_vpe_int.h"
#include "kdrv_videoprocess/kdrv_vpe.h"
#include "kdrv_videoprocess/kdrv_vpe_if.h"
//#include "kdrv_videoprocess/kdrv_ipp_config.h"
//#include "kdrv_videoprocess/kdrv_ipp.h"



/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_vpe_drv_wait_cmd_complete(PVPE_INFO pvpe_info);
//int nvt_vpe_drv_ioctl(unsigned char ucIF, VPE_INFO *pvpe_info, unsigned int cmd, unsigned long arg);
void nvt_vpe_drv_do_tasklet(unsigned long data);
//irqreturn_t nvt_vpe_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
extern void vpe_isr(void);

//struct clk *vpe_clk[VPE_CLK_NUM];
//int i_event_flag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_vpe_drv_open(PVPE_INFO pvpe_info, unsigned char uc_if)
{
	//nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_vpe_drv_release(PVPE_INFO pvpe_info, unsigned char uc_if)
{
	//nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	//Free IRQ
	free_irq(pvpe_info->iinterrupt_id[0], pvpe_info);

	//vpe_release_resource();
	//vpe_platform_release_resource();
	//kdrv_vpe_uninstall_id();

	//kdrv_vpe_uninit();

	return 0;
}

int nvt_vpe_drv_init(VPE_INFO *pvpe_info)
{
	KDRV_VPE_IF_OBJ vpe_if_obj;
	int i_ret = 0;
	//struct clk *pclk;

	init_waitqueue_head(&pvpe_info->vpe_wait_queue);

	// coverity[side_effect_free]
	vk_spin_lock_init(&pvpe_info->vpe_spinlock);
	vk_sema_init(&pvpe_info->vpe_sem, 1);
	init_completion(&pvpe_info->vpe_completion);
	tasklet_init(&pvpe_info->vpe_tasklet, nvt_vpe_drv_do_tasklet, (unsigned long)pvpe_info);

#if 0
	/* register IRQ here*/
	if (request_irq(pvpe_info->iinterrupt_id[0], nvt_vpe_drv_isr, IRQF_TRIGGER_HIGH, "VPE_INT", pvpe_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", (int)pvpe_info->iinterrupt_id[0]);
		i_ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
#endif

	/* Add HW Module initialization here when driver loaded */
	//vpe_setBaseAddr((UINT32)pvpe_info->io_addr[0]);
	//vpe_create_resource();

	if (nvt_get_chip_id() != CHIP_NA51055) {

        if (efuse_check_available("vpe_lib") != TRUE) {
            DBG_FATAL("cannot support %s.\r\n", "vpe_lib");

            vpe_if_obj.open = NULL;
    		vpe_if_obj.close = NULL;
    		vpe_if_obj.set = NULL;
    		vpe_if_obj.get = NULL;
    		vpe_if_obj.trigger = NULL;
    		kdrv_vpe_if_register(&vpe_if_obj);

            return -1;
        } else {
    		vpe_platform_create_resource(pvpe_info);
    		vpe_platform_prepare_clk();
            vpe_platform_set_clk_rate(400);
    		//kdrv_vpe_init();
    		//kdrv_vpe_install_id();
    		//kdrv_vpe_flow_init();

    		/* register to vpe_if */    		
    		vpe_if_obj.open = kdrv_vpe_open;
    		vpe_if_obj.close = kdrv_vpe_close;
    		vpe_if_obj.set = (KDRV_VPE_IF_SET_FP)kdrv_vpe_set;
    		vpe_if_obj.get = (KDRV_VPE_IF_GET_FP)kdrv_vpe_get;
    		vpe_if_obj.trigger = kdrv_vpe_trigger;
    		kdrv_vpe_if_register(&vpe_if_obj);
		}
	}

	//nvt_dbg(IND, "vpe initialization...\r\n");
	//printk("vpe initialization...\r\n");

	//nvt_dbg(IND, "vpe set engine resource...done\r\n");

	return i_ret;

	/* Add error handler here */

#if 0
FAIL_FREE_IRQ:
	free_irq(pvpe_info->iinterrupt_id[0], pvpe_info);
	return i_ret;
#endif
}

int nvt_vpe_drv_remove(VPE_INFO *pvpe_info)
{

	if (nvt_get_chip_id() != CHIP_NA51055) {
		//Free IRQ
		free_irq(pvpe_info->iinterrupt_id[0], pvpe_info);


		/* Add HW Moduel release operation here*/
		vpe_platform_unprepare_clk();
	}

	return 0;
}

int nvt_vpe_drv_suspend(VPE_INFO *pvpe_info)
{
	//nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_vpe_drv_resume(VPE_INFO *pvpe_info)
{
	//nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

#if 0
int nvt_vpe_drv_ioctl(unsigned char uc_if, VPE_INFO *pvpe_info, unsigned int cmd, unsigned long arg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int i_loop;
	int i_ret = 0;

	//nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, cmd);



	switch (cmd) {
	case VPE_IOC_START:
		/*call someone to start operation*/
		break;

	case VPE_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case VPE_IOC_READ_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)arg, sizeof(REG_INFO));
		if (!i_ret) {
			reg_info.uiValue = READ_REG(pvpe_info->io_addr[uc_if] + reg_info.uiAddr);
			i_ret = copy_to_user((void __user *)arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case VPE_IOC_WRITE_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)arg, sizeof(REG_INFO));
		if (!i_ret) {
			WRITE_REG(reg_info.uiValue, pvpe_info->io_addr[uc_if] + reg_info.uiAddr);
		}
		break;

	case VPE_IOC_READ_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			if (reg_info_list.uiCount <= VPE_REG_LIST_NUM) {
				for (i_loop = 0 ; i_loop < reg_info_list.uiCount; i_loop++) {
					reg_info_list.RegList[i_loop].uiValue = READ_REG(pvpe_info->io_addr[uc_if] + reg_info_list.RegList[i_loop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}

			i_ret = copy_to_user((void __user *)arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case VPE_IOC_WRITE_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			if (reg_info_list.uiCount <= VPE_REG_LIST_NUM) {
				for (i_loop = 0 ; i_loop < reg_info_list.uiCount ; i_loop++) {
					WRITE_REG(reg_info_list.RegList[i_loop].uiValue, pvpe_info->io_addr[uc_if] + reg_info_list.RegList[i_loop].uiAddr);
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

irqreturn_t nvt_vpe_drv_isr(int irq, void *devid)
{
	PVPE_INFO pvpe_info = (PVPE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pvpe_info->vpe_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pvpe_info->vpe_tasklet);

	vpe_isr();

	return IRQ_HANDLED;
}

int nvt_vpe_drv_wait_cmd_complete(PVPE_INFO pvpe_info)
{
	wait_for_completion(&pvpe_info->vpe_completion);
	return 0;
}

int nvt_vpe_drv_write_reg(PVPE_INFO pvpe_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pvpe_info->io_addr[0] + addr);
	return 0;
}

int nvt_vpe_drv_read_reg(PVPE_INFO pvpe_info, unsigned long addr)
{
	return READ_REG(pvpe_info->io_addr[0] + addr);
}

void nvt_vpe_drv_do_tasklet(unsigned long data)
{
	PVPE_INFO pvpe_info = (PVPE_INFO)data;
	//nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pvpe_info->vpe_completion);
}
