
#if defined (__KERNEL__)

//#include <linux/version.h>
//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <linux/mm.h>
//#include <linux/sched.h>
//#include <linux/interrupt.h>
//#include <linux/dma-mapping.h>
//#include <linux/wait.h>
//#include <linux/irq.h>
//#include <asm/io.h>

#elif defined (__FREERTOS)

#else

#endif


#include "kdrv_videoprocess/kdrv_vpe.h"

//#include "vpe_ll_base.h"
//#include "vpe_config_base.h"
//#include "vpe_lib.h"
#include "kdrv_vpe_int.h"

//#include "vpe_int.h"

//#include <mach/nvt_pcie.h>

#if 0//(DRV_SUPPORT_IST == ENABLE)
#else
VPE_INT_CB vpe_p_int_hdl_callback = NULL;
#endif


extern VOID vpe_isr(VOID);

#define VPE_STATUS_REG          0x40

extern void vpe_drv_job_done(UINT32 status);
VOID vpe_isr(VOID)
{
	UINT32 interrupt_status = 0, interrupt_enable = 0, interrupt_handle = 0;
	UINT32 interrupt_callback = 0;

	FLGPTN flg = 0x0;

	interrupt_status = vpe_hw_read(VPE_REG_INTS_1);
	interrupt_enable = vpe_hw_read(VPE_REG_INTE_1);

	interrupt_handle = (interrupt_status & interrupt_enable);
	vpe_hw_write(VPE_REG_INTS_1, interrupt_handle);

	if (interrupt_handle & KDRV_VPE_INTS_FRM_END) {
		interrupt_callback |= KDRV_VPE_INTS_FRM_END;

		flg |= FLGPTN_VPE_FRAMEEND;
	}

	if (interrupt_handle & KDRV_VPE_INTS_LL_END) {
		interrupt_callback |= KDRV_VPE_INTS_LL_END;

		flg |= FLGPTN_VPE_LLEND;

		// disable engine clock
    	if (fw_vpe_power_saving_en == TRUE) {
    		vpe_platform_enable_sram_shutdown();
    		vpe_detach();
    	}
	}

	if (interrupt_handle & KDRV_VPE_INTS_LL_ERR) {
		interrupt_callback |= KDRV_VPE_INTS_LL_ERR;

		vk_print_isr("VPE: linked-list command error\r\n");
	}

	if (interrupt_handle & KDRV_VPE_INTS_RES0_NER_FRM_END) {
		interrupt_callback |= KDRV_VPE_INTS_RES0_NER_FRM_END;
	}

	if (interrupt_handle & KDRV_VPE_INTS_RES1_NER_FRM_END) {
		interrupt_callback |= KDRV_VPE_INTS_RES1_NER_FRM_END;
	}

	if (interrupt_handle & KDRV_VPE_INTS_RES2_NER_FRM_END) {
		interrupt_callback |= KDRV_VPE_INTS_RES2_NER_FRM_END;
	}

	if (interrupt_handle & KDRV_VPE_INTS_RES3_NER_FRM_END) {
		interrupt_callback |= KDRV_VPE_INTS_RES3_NER_FRM_END;
	}

	vpe_platform_flg_set(flg);


	if (vpe_p_int_hdl_callback != NULL) {
		vpe_p_int_hdl_callback(interrupt_callback);
	}

	vpe_drv_job_done(interrupt_callback);

}

#if 0
irqreturn_t vpe_hw_isr(INT32 irq, void *ptr)
{
	UINT32 status;

	status = vpe_hw_read(VPE_REG_INTS_1);
	if (status & (BIT3 | BIT4) || (status & (BIT1))) {
		printk("VPE DMA error %x\n", status);

		vpe_hw_write(VPE_REG_INTS_1, VPE_BIT_FRAME_DONE);
		vpe_hw_write(VPE_REG_GLOCTL, VPE_BIT_DMA0_DIS);
		vpe_hw_write(VPE_REG_GLOCTL, VPE_BIT_SW_RST);
		vpe_hw_write(VPE_REG_INTS_1, 0x00);
	} else {
		if ((status & (BIT0 | BIT2)) == (BIT0 | BIT2)) {
			vpe_hw_write(VPE_REG_INTS_1, (BIT0 | BIT2));
		}
	}
	vpe_drv_job_done(status);

	return IRQ_HANDLED;
}
#endif



ER vpe_wait_flag_frame_end(VPE_FLAG_CLEAR_SEL is_clear_flag)
{
	FLGPTN flag;

	if (is_clear_flag == VPE_FLAG_CLEAR) {

		vpe_platform_flg_clear(FLGPTN_VPE_FRAMEEND);
	}

	vpe_platform_flg_wait(&flag, FLGPTN_VPE_FRAMEEND);

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID vpe_clear_flag_frame_end(VOID)
{
	vpe_platform_flg_clear(FLGPTN_VPE_FRAMEEND);
}
//-------------------------------------------------------------------------------


ER vpe_wait_flag_linked_list_job_end(VPE_FLAG_CLEAR_SEL is_clear_flag)
{
	FLGPTN flag;

	if (is_clear_flag == VPE_FLAG_CLEAR) {
		vpe_platform_flg_clear(FLGPTN_VPE_LLEND);
	}

	vpe_platform_flg_wait(&flag, FLGPTN_VPE_LLEND);

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID vpe_clear_flag_linked_list_job_end(VOID)
{
	vpe_platform_flg_clear(FLGPTN_VPE_LLEND);
}
//-------------------------------------------------------------------------------


#if defined (__KERNEL__)
EXPORT_SYMBOL(vpe_isr);
#endif



