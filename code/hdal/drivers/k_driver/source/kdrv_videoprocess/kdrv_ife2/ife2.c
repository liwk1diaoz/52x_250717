
/*
    IFE2 module driver

    NT98520 IFE2 driver.

    @file       IFE2.c
    @ingroup    mIIPPIFE2

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include    "ife2_platform.h"


#include    "ife2_base.h"
#include    "ife2_int.h"
#include    "ife2_lib.h"

#if defined (_NVT_EMULATION_)
#include "comm/timer.h"
static TIMER_ID ife2_timer_id;
BOOL ife2_end_time_out_status = FALSE;
#define IFE2_POWER_SAVING_EN 0//do not enable power saving in emulation
#else
#define IFE2_POWER_SAVING_EN 1
#endif

typedef struct _IFE2_OUTPUT_BUF_INFO_ {
	IFE2_OPMODE op_mode;
	UINT32 out_buf_size;
	UINT32 out_buf_addr;
} IFE2_OUTPUT_BUF_INFO;


static IFE2_ENGINE_STATUS ife2_engine_status = IFE2_ENGINE_IDLE;
//static IFE2_ADDR_PARAM ife2_set_addr = {0x0};
//static IFE2_LOFS_PARAM ife2_set_lofs = {0x0};
//static IFE2_REFCENT_PARAM ife2_set_ref_cent = {0x0};
//static IFE2_FILTER_PARAM ife2_set_filter = {0x0};
//static IFE2_GRAY_STATAL ife2_set_gray_sta = {0x0};

//static IFE2_REFCENT_PARAM ife2_set_ref_cent_info = {0x0};
//static IFE2_FILTER_LUT_PARAM ife2_set_filter_info = {0x0};


static IFE2_IMG_SIZE ife2_get_image_size = {0x0};
static IFE2_LINEOFFSET ife2_get_lofs = {0x0};

IFE2_OUTPUT_BUF_INFO ife2_get_output_buf_info;

#if 0//(DRV_SUPPORT_IST == ENABLE)
// do nothing
#else
static void (*pf_ife2_int_cb)(UINT32 interrupt_status);
#endif

extern VOID ife2_isr(VOID);
//static ER ife2_set_avg_fltr(IFE2_AVGFILTERSET *p_avg_filter_set_cb, IFE2_AVGFILTERSET *p_avg_filter_set_cr);


static IFE2_LOCK_STATUS g_ife2_lock_status = IFE2_UNLOCKED;
static UINT32 g_ife2_image_size_h_limt_for_all_dir = 672;

static VOID ife2_load(VOID);
static ER ife2_lock(VOID);
static ER ife2_unlock(VOID);
static VOID ife2_attach(VOID);
static VOID ife2_detach(VOID);



static VOID ife2_load(VOID)
{

	if (ife2_get_output_buf_info.op_mode == IFE2_OPMODE_SIE_IPP) {
		ife2_set_engine_control(IFE2_CTRL_LOAD, (UINT32)IFE2_DIRECT_START_LOAD);
	} else {
		if (ife2_engine_status == IFE2_ENGINE_RUN) {
			ife2_set_engine_control(IFE2_CTRL_LOAD, (UINT32)IFE2_FRMEND_LOAD);
		}
	}
}
//-----------------------------------------------------------------------


static ER ife2_lock(void)
{
	ER er_return = E_OK;

	if (g_ife2_lock_status == IFE2_LOCKED) {
		return E_SYS;
	}

	ife2_platform_sem_wait();

#if 0
#ifdef __KERNEL__

	SEM_WAIT(semid_ife2);

#else

	er_return = wai_sem(semid_ife2);
	if (er_return != E_OK) {
		return er_return;
	}

#endif
#endif

	g_ife2_lock_status = IFE2_LOCKED;

	return er_return;
}
//-----------------------------------------------------------------------

static ER ife2_unlock(void)
{
	ER er_return = E_OK;

	if (g_ife2_lock_status == IFE2_UNLOCKED) {
		return E_SYS;
	}

	ife2_platform_sem_signal();

#if 0
#ifdef __KERNEL__

	SEM_SIGNAL(semid_ife2);

#else

	er_return = sig_sem(semid_ife2);
	if (er_return != E_OK) {
		return er_return;
	}

#endif
#endif

	g_ife2_lock_status = IFE2_UNLOCKED;

	return er_return;

}
//-----------------------------------------------------------------------

static VOID ife2_attach(void)
{
	// Enable IFE2 Clock

	ife2_platform_enable_clk();

#if 0
#ifdef __KERNEL__

	if (IS_ERR(ife2_clk[0])) {
		DBG_WRN("IFE2: get clk fail 0x%p\r\n", ife2_clk[0]);
	} else {
		clk_enable(ife2_clk[0]);
	}

#else
	pll_enableClock(IFE2_CLK);
#endif
#endif
}
//-----------------------------------------------------------------------

static VOID ife2_detach(void)
{
	// Disable IFE2 Clock

	ife2_platform_disable_clk();
#if 0
#ifdef __KERNEL__

	if (IS_ERR(ife2_clk[0])) {
		DBG_WRN("IFE2: get clk fail 0x%p\r\n", ife2_clk[0]);
	} else {
		clk_disable(ife2_clk[0]);
	}

#else
	pll_disableClock(IFE2_CLK);
#endif
#endif
}
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------

/**
IFE2 Internal API

@name   IFE2_Internal
*/
//@{


/*
    IFE2 interrupt handler

    IFE2 interrupt service routine.
*/
VOID ife2_isr(VOID)
{
	FLGPTN flag = 0x0;

	UINT32 interrupt_enable = 0x0;
	UINT32 interrupt_status = 0x0;
	UINT32 interrupt_handle = 0x0;

	UINT32 ife2_interrupt_cb = 0x0;

	interrupt_status = ife2_get_engine_info(IFE2_GET_INTSTATUS);

	if (ife2_platform_get_init_status() == FALSE) {
		ife2_set_engine_control(IFE2_CTRL_INTP, interrupt_status);
		return;
	}


	interrupt_enable = ife2_get_engine_info(IFE2_GET_INTESTATUS);
	interrupt_handle = (interrupt_status & interrupt_enable);
	ife2_set_engine_control(IFE2_CTRL_INTP, interrupt_handle);

	if (interrupt_handle == 0) {
		return;
	}

	if (interrupt_handle & IFE2_INTS_FRMEND) {
		//DBG_DUMP("f2\r\n");
		flag |= FLGPTN_IFE2_FRAMEEND;

		ife2_interrupt_cb |= IFE2_INTS_FRMEND;
	}

	if (interrupt_handle & IFE2_INTE_FRMERR) {

		ife2_interrupt_cb |= IFE2_INTS_FRMERR;

		//DBG_WRN("IFE2: frame error\r\n");
		vk_print_isr("IFE2: frame error\r\n");
	}

	if (interrupt_handle & IFE2_INTS_OVFL) {
		ife2_interrupt_cb |= IFE2_INTS_OVFL;

		//DBG_WRN("IFE2: output overflow\r\n");
		vk_print_isr("IFE2: output overflow\r\n");
	}

	if (interrupt_handle & IFE2_INTS_LL_END) {
		//DBG_DUMP("f2\r\n");
		flag |= FLGPTN_IFE2_LLEND;

		ife2_interrupt_cb |= IFE2_INTS_LL_END;
	}

	if (interrupt_handle & IFE2_INTS_LL_JEND) {
		//DBG_DUMP("f2\r\n");
		flag |= FLGPTN_IFE2_JEND;

		ife2_interrupt_cb |= IFE2_INTS_LL_JEND;
	}

#if 0//(DRV_SUPPORT_IST == ENABLE)
	if (pfIstCB_IFE2 != NULL) {
		drv_setIstEvent(DRV_IST_MODULE_IFE2, ife2_interrupt_cb);
	}
#else
	if (pf_ife2_int_cb != NULL) {
		pf_ife2_int_cb(ife2_interrupt_cb);
	}
#endif

	//iset_flg(flg_id_ife2, flag);
	ife2_platform_flg_set(flag);

}

//@}
//-----------------------------------------------------------------------

/**
IFE2 External API

@name   IFE2_External
*/
//@{


ER ife2_open(IFE2_OPENOBJ *p_obj_cb)
{
	ER er_return;

#if 0
#ifdef __KERNEL__

	struct clk *parent_clk;

#else

	UINT32 uiIfe2ClkSel;

#endif
#endif

	if (p_obj_cb == NULL) {
		DBG_ERR("input argument NULL ...\r\n");

		return E_PAR;
	}

	// Lock semaphore
	er_return = ife2_lock();
	if (er_return != E_OK) {
		DBG_ERR("Re-opened...\r\n");
		return er_return;
	}

	// Check state-machine
	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_OPEN);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d\r\n", (int)IFE2_ENGINE_IDLE);

		ife2_unlock();

		return er_return;
	}

#if 1
	ife2_platform_set_clk_rate(p_obj_cb->uiIfe2ClkSel);
#else
#ifdef __KERNEL__

	if (IS_ERR(ife2_clk[0])) {
		DBG_ERR("IFE2: get clk fail...0x%p\r\n", ife2_clk[0]);
		return E_SYS;
	}

	if (p_obj_cb->uiIfe2ClkSel < 240) {
		DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", p_obj_cb->uiIfe2ClkSel);
		parent_clk = clk_get(NULL, "fix240m");
	} else if (p_obj_cb->uiIfe2ClkSel == 240) {
		parent_clk = clk_get(NULL, "fix240m");
	} else if (p_obj_cb->uiIfe2ClkSel < 320) {
		DBG_WRN("IFE2: input frequency %d round to 320MHz\r\n", p_obj_cb->uiIfe2ClkSel);
		parent_clk = clk_get(NULL, "pllf320");
	} else if (p_obj_cb->uiIfe2ClkSel == 320) {
		parent_clk = clk_get(NULL, "pllf320");
	} else if (p_obj_cb->uiIfe2ClkSel < 360) {
		DBG_WRN("IFE2: input frequency %d round to 240MHz\r\n", p_obj_cb->uiIfe2ClkSel);
		parent_clk = clk_get(NULL, "pll13");
	} else if (p_obj_cb->uiIfe2ClkSel == 360) {
		parent_clk = clk_get(NULL, "pll13");
	} else {
		DBG_WRN("IFE2: input frequency %d round to 360MHz\r\n", p_obj_cb->uiIfe2ClkSel);
		parent_clk = clk_get(NULL, "pll13");
	}

	clk_set_parent(ife2_clk[0], parent_clk);
	clk_put(parent_clk);
#else

	// Power on module(power domain on)
	pmc_turnonPower(PMC_MODULE_IFE2);

	// Select PLL clock source
	if (p_obj_cb->uiIfe2ClkSel > 360) {
		uiIfe2ClkSel = PLL_CLKSEL_IFE2_PLL13;

		DBG_WRN("IFE2: user-clock - %d, engine-clock: 360MHz\r\n", p_obj_cb->uiIfe2ClkSel);
	} else if (p_obj_cb->uiIfe2ClkSel == 360) {
		uiIfe2ClkSel = PLL_CLKSEL_IFE2_PLL13;
	} else if (p_obj_cb->uiIfe2ClkSel > 320) {
		uiIfe2ClkSel = PLL_CLKSEL_IFE2_PLL13;

		DBG_WRN("IFE2: user-clock - %d, engine-clock: 360MHz\r\n", p_obj_cb->uiIfe2ClkSel);
	} else if (p_obj_cb->uiIfe2ClkSel == 320) {
		uiIfe2ClkSel = PLL_CLKSEL_IFE2_320;
	} else if (p_obj_cb->uiIfe2ClkSel > 240) {
		uiIfe2ClkSel = PLL_CLKSEL_IFE2_320;

		DBG_WRN("IFE2: user-clock - %d, engine-clock: 320MHz\r\n", p_obj_cb->uiIfe2ClkSel);
	} else if (p_obj_cb->uiIfe2ClkSel == 240) {
		uiIfe2ClkSel = PLL_CLKSEL_IFE2_240;
	} else {
		uiIfe2ClkSel = PLL_CLKSEL_IFE2_240;

		DBG_WRN("IFE2: user-clock - %d, engine-clock: 240MHz\r\n", p_obj_cb->uiIfe2ClkSel);
	}

	if (uiIfe2ClkSel == PLL_CLKSEL_IFE2_PLL13) {
		if (pll_getPLLEn(PLL_ID_13) == FALSE) {
			pll_setPLLEn(PLL_ID_13, TRUE);//foolproof
		}
	}

	if (uiIfe2ClkSel == PLL_CLKSEL_IFE2_320) {
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		}
	}

	// Enable PLL clock source
	pll_setClockRate(PLL_CLKSEL_IFE2, uiIfe2ClkSel);
#endif // end #ifdef __KERNEL__s
#endif

	// Attach(enable engine clock)
	//ife2_platform_prepare_clk();
	ife2_attach();

#if 1
	ife2_platform_disable_sram_shutdown();
	ife2_platform_int_enable();
#else
#ifdef __KERNEL__
	nvt_disable_sram_shutdown(IFE2_SD);
#else
	pinmux_disableSramShutDown(IFE2_SD);

	//enable engine interrupt of system
	drv_enableInt(DRV_INT_IFE2);
#endif
#endif

#if 0 // remove for builtin
	// clear interrupt status
	ife2_set_engine_control(IFE2_CTRL_INTPEN, 0x0);
	ife2_set_engine_control(IFE2_CTRL_INTP, IFE2_INTS_ALL);

	// Clear SW flag
#if 1
	ife2_platform_flg_clear(FLGPTN_IFE2_FRAMEEND | FLGPTN_IFE2_LLEND | FLGPTN_IFE2_JEND);
#else
	clr_flg(flg_id_ife2, FLGPTN_IFE2_FRAMEEND);
	clr_flg(flg_id_ife2, FLGPTN_IFE2_LLEND);
	clr_flg(flg_id_ife2, FLGPTN_IFE2_JEND);
#endif

	// engine soft reset
	ife2_set_engine_control(IFE2_CTRL_RESET, 0x0);
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ife2_ctrl_flow_to_do == ENABLE) {
		// clear interrupt status
		ife2_set_engine_control(IFE2_CTRL_INTPEN, 0x0);
		ife2_set_engine_control(IFE2_CTRL_INTP, IFE2_INTS_ALL);

		// Clear SW flag
#if 1
		ife2_platform_flg_clear(FLGPTN_IFE2_FRAMEEND | FLGPTN_IFE2_LLEND | FLGPTN_IFE2_JEND);
#else
		clr_flg(flg_id_ife2, FLGPTN_IFE2_FRAMEEND);
		clr_flg(flg_id_ife2, FLGPTN_IFE2_LLEND);
		clr_flg(flg_id_ife2, FLGPTN_IFE2_JEND);
#endif

		// engine soft reset
		ife2_set_engine_control(IFE2_CTRL_RESET, 0x0);
	}
#else
	// clear interrupt status
	ife2_set_engine_control(IFE2_CTRL_INTPEN, 0x0);
	ife2_set_engine_control(IFE2_CTRL_INTP, IFE2_INTS_ALL);

	// Clear SW flag
#if 1
	ife2_platform_flg_clear(FLGPTN_IFE2_FRAMEEND | FLGPTN_IFE2_LLEND | FLGPTN_IFE2_JEND);
#else
	clr_flg(flg_id_ife2, FLGPTN_IFE2_FRAMEEND);
	clr_flg(flg_id_ife2, FLGPTN_IFE2_LLEND);
	clr_flg(flg_id_ife2, FLGPTN_IFE2_JEND);
#endif

	// engine soft reset
	ife2_set_engine_control(IFE2_CTRL_RESET, 0x0);
#endif


	// Hook call-back function
#if 0 //(DRV_SUPPORT_IST == ENABLE)
	pfIstCB_IFE2 = p_obj_cb->pfIfe2IsrCb;
#else
	pf_ife2_int_cb = p_obj_cb->pfIfe2IsrCb;
#endif

	if (nvt_get_chip_id() == CHIP_NA51055) {
		g_ife2_image_size_h_limt_for_all_dir = 668;
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		g_ife2_image_size_h_limt_for_all_dir = 1024;
	} else {
        DBG_ERR("IFE2: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }

	//  Update state-machine
	ife2_engine_status = IFE2_ENGINE_READY;

	return E_OK;
}
//-----------------------------------------------------------------------


BOOL ife2_is_opened(VOID)
{
	return ((BOOL)g_ife2_lock_status);
}
//-----------------------------------------------------------------------


ER ife2_close(VOID)
{
	ER er_return;

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_CLOSE);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d\r\n", (int)IFE2_ENGINE_PAUSE);

		return er_return;
	}

	// Check lock status
	if (g_ife2_lock_status != IFE2_LOCKED) {
		return E_SYS;
	}

#if 1
	ife2_platform_enable_sram_shutdown();
	ife2_platform_int_disable();
#else
#ifdef __KERNEL__

	nvt_enable_sram_shutdown(IFE2_SD);

#else
	// Release interrupt
	drv_disableInt(DRV_INT_IFE2);

	pinmux_enableSramShutDown(IFE2_SD);
#endif
#endif

	ife2_detach();
	//ife2_platform_unprepare_clk();


	ife2_engine_status = IFE2_ENGINE_IDLE;

	er_return = ife2_unlock();
	if (er_return != E_OK) {
		return er_return;
	}

	return E_OK;
}
//-----------------------------------------------------------------------

ER ife2_reset(VOID)
{
	ife2_set_engine_control(IFE2_CTRL_START, (UINT32)IFE2_FUNC_DISABLE);

	ife2_platform_enable_sram_shutdown();
	ife2_platform_disable_sram_shutdown();

	ife2_detach();
	ife2_platform_unprepare_clk();

	ife2_platform_prepare_clk();
	ife2_attach();

	// clear interrupt status
	ife2_set_engine_control(IFE2_CTRL_INTPEN, 0x0);
	ife2_set_engine_control(IFE2_CTRL_INTP, IFE2_INTS_ALL);

	ife2_platform_flg_clear(FLGPTN_IFE2_FRAMEEND | FLGPTN_IFE2_LLEND | FLGPTN_IFE2_JEND);

	// engine soft reset
	ife2_set_engine_control(IFE2_CTRL_RESET, 0x0);

	ife2_engine_status = IFE2_ENGINE_READY;

	return E_OK;
}
//-----------------------------------------------------------------------



ER ife2_set_mode(IFE2_PARAM *p_set_param)
{
	ER er_return;
	UINT32 interrupt_enable = 0x0;
	IFE2_OUTDEST_SEL ife2_out_dest;

	IFE2_ADDR_PARAM set_addr = {0x0};
	IFE2_LOFS_PARAM set_lofs = {0x0};

	if (p_set_param == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_SETPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d\r\n", (int)IFE2_ENGINE_READY);

		return er_return;
	}

	if (ife2_ctrl_flow_to_do == DISABLE) {
	    ife2_get_output_buf_info.op_mode = p_set_param->op_mode;

		goto SKIP_PARAMS;
	}


	//--------------------------------------------------------------------------
	// input data mode
	ife2_get_output_buf_info.op_mode = p_set_param->op_mode;
	switch (p_set_param->op_mode) {
	case IFE2_OPMODE_D2D:
		ife2_out_dest = IFE2_OUTDEST_DRAM;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)ife2_out_dest);
		break;

	case IFE2_OPMODE_IFE_IPP:
		ife2_out_dest = IFE2_OUTDEST_IME;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)ife2_out_dest);
		break;

	case IFE2_OPMODE_SIE_IPP:
		ife2_out_dest = IFE2_OUTDEST_ADRT;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)ife2_out_dest);
		break;

	default:
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		DBG_ERR("IFE2: operation mode error ...\r\n");
		return er_return;
	}


	//--------------------------------------------------------------------------
	// engine control
	//ife2_chg_burst_length(&(p_set_param->bst_size));
	if (p_set_param != NULL) {
#if (defined(_NVT_EMULATION_) == ON)
		ife2_set_burst_length_reg((UINT32)(p_set_param->bst_size.in_bst_len), (UINT32)(p_set_param->bst_size.out_bst_len));

#else
		ife2_set_burst_length_reg((UINT32)(p_set_param->bst_size.in_bst_len), (UINT32)IFE2_BURST_32W);
#endif
	}

	ife2_set_engine_control(IFE2_CTRL_FTREN, ENABLE);
	ife2_set_engine_control(IFE2_CTRL_OUTDRAM, p_set_param->out_dram_en);

	//--------------------------------------------------------------------------
	// interrupt enable
#if 0  // remove for builtin
	interrupt_enable = p_set_param->interrupt_en;
	if (p_set_param->interrupt_en == 0) {
		interrupt_enable = (IFE2_INTE_FRMEND | IFE2_INTE_LL_END | IFE2_INTE_LL_JEND);
	}
	ife2_set_engine_control(IFE2_CTRL_INTPEN, interrupt_enable);
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ife2_ctrl_flow_to_do == ENABLE) {
		interrupt_enable = p_set_param->interrupt_en;
		if (p_set_param->interrupt_en == 0) {
			interrupt_enable = (IFE2_INTE_FRMEND | IFE2_INTE_LL_END | IFE2_INTE_LL_JEND);
		}
		ife2_set_engine_control(IFE2_CTRL_INTPEN, interrupt_enable);
	} else {
		if (p_set_param->op_mode != IFE2_OPMODE_SIE_IPP) {
			interrupt_enable = p_set_param->interrupt_en;
			if (p_set_param->interrupt_en == 0) {
				interrupt_enable = (IFE2_INTE_FRMEND | IFE2_INTE_LL_END | IFE2_INTE_LL_JEND);
			}
			ife2_set_engine_control(IFE2_CTRL_INTPEN, interrupt_enable);
		}
	}
#else
	interrupt_enable = p_set_param->interrupt_en;
	if (p_set_param->interrupt_en == 0) {
		interrupt_enable = (IFE2_INTE_FRMEND | IFE2_INTE_LL_END | IFE2_INTE_LL_JEND);
	}
	ife2_set_engine_control(IFE2_CTRL_INTPEN, interrupt_enable);
#endif


	//--------------------------------------------------------------------------
	// input and output format
	er_return = ife2_chg_src_fmt(p_set_param->src_fmt);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ife2_chg_dst_fmt(p_set_param->dst_fmt);
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// input image size
	er_return = ife2_chg_in_size(&(p_set_param->img_size));
	if (er_return != E_OK) {
		return er_return;
	}


	//--------------------------------------------------------------------------
	// input lineoffset
	set_lofs = p_set_param->in_lofs;
	er_return = ife2_chg_lineoffset(&set_lofs);
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// input dma address
	set_addr = p_set_param->in_addr;
	er_return = ife2_chg_dma_addr(&set_addr);
	if (er_return != E_OK) {
		return er_return;
	}


	// do not update to PatternGen.
	if (p_set_param->op_mode == IFE2_OPMODE_D2D) {
		//--------------------------------------------------------------------------
		// output lineoffset
		set_lofs = p_set_param->out_lofs;
		er_return = ife2_chg_lineoffset(&set_lofs);
		if (er_return != E_OK) {
			return er_return;
		}

		//--------------------------------------------------------------------------
		// output dma address0
		set_addr = p_set_param->out_addr0;
		set_addr.chl_sel = IFE2_DMAHDL_OUT0;
		er_return = ife2_chg_dma_addr(&set_addr);
		if (er_return != E_OK) {
			return er_return;
		}
	}

	//--------------------------------------------------------------------------
	// reference center parameters
	//ife2_set_ref_cent = p_set_param->ref_cent;
	er_return = ife2_chg_ref_center_param(&(p_set_param->ref_cent));
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// filter parameters
	//ife2_set_filter = p_set_param->filter;
	er_return = ife2_chg_filter_param(&(p_set_param->filter));
	if (er_return != E_OK) {
		return er_return;
	}

	//ife2_set_gray_sta = p_set_param->gray_sta;
	er_return = ife2_chg_gray_statal_param(&(p_set_param->gray_sta));
	if (er_return != E_OK) {
		return er_return;
	}

	ife2_engine_status = IFE2_ENGINE_PAUSE;

	return er_return;

SKIP_PARAMS:
	ife2_engine_status = IFE2_ENGINE_PAUSE;

	return er_return;
}
//-----------------------------------------------------------------------

#if 0
ER ife2_chg_all_param(IFE2_PARAM *p_set_param)
{
	ER er_return;
	UINT32 interrupt_en = 0x0;
	IFE2_OUTDEST_SEL ife2_out_dest;

	IFE2_ADDR_PARAM set_addr = {0x0};
	IFE2_LOFS_PARAM set_lofs = {0x0};

	if (p_set_param == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	//--------------------------------------------------------------------------
	// input data mode
	switch (p_set_param->op_mode) {
	case IFE2_OPMODE_D2D:
		ife2_out_dest = IFE2_OUTDEST_DRAM;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)ife2_out_dest);
		break;

	case IFE2_OPMODE_IFE_IPP:
		ife2_out_dest = IFE2_OUTDEST_IME;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)ife2_out_dest);
		break;

	case IFE2_OPMODE_SIE_IPP:
		ife2_out_dest = IFE2_OUTDEST_IME;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)ife2_out_dest);
		break;

	default:
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		DBG_ERR("IFE2: operation mode error ...\r\n");
		return er_return;
	}


	//--------------------------------------------------------------------------
	// engine control
	//ife2_chg_burst_length(&(p_set_param->bst_size));
	if (p_set_param != NULL) {
#if (defined(_NVT_EMULATION_) == ON)
		ife2_set_burst_length_reg((UINT32)(p_set_param->bst_size.in_bst_len), (UINT32)(p_set_param->bst_size.out_bst_len));
#else
		ife2_set_burst_length_reg((UINT32)(p_set_param->bst_size.in_bst_len), (UINT32)IFE2_BURST_32W);
#endif
	}

	ife2_set_engine_control(IFE2_CTRL_FTREN, ENABLE);

	//--------------------------------------------------------------------------
	// interrupt enable
	interrupt_en = p_set_param->interrupt_en;
	if (p_set_param->interrupt_en == 0) {
		interrupt_en = IFE2_INTE_FRMEND;
	}
	ife2_set_engine_control(IFE2_CTRL_INTPEN, interrupt_en);


	//--------------------------------------------------------------------------
	// input and output format
	er_return = ife2_chg_src_fmt(p_set_param->src_fmt);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ife2_chg_dst_fmt(p_set_param->dst_fmt);
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// input image size
	er_return = ife2_chg_in_size(&(p_set_param->img_size));
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// input lineoffset
	set_lofs = p_set_param->in_lofs;
	er_return = ife2_chg_lineoffset(&set_lofs);
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// input dma address
	set_addr = p_set_param->in_addr;
	er_return = ife2_chg_dma_addr(&set_addr);
	if (er_return != E_OK) {
		return er_return;
	}

	if (p_set_param->op_mode == IFE2_OPMODE_D2D) {
		//--------------------------------------------------------------------------
		// output lineoffset
		set_lofs = p_set_param->out_lofs;
		er_return = ife2_chg_lineoffset(&set_lofs);
		if (er_return != E_OK) {
			return er_return;
		}

		//--------------------------------------------------------------------------
		// output dma address0
		set_addr = p_set_param->out_addr0;
		set_addr.chl_sel = IFE2_DMAHDL_OUT0;
		er_return = ife2_chg_dma_addr(&set_addr);
		if (er_return != E_OK) {
			return er_return;
		}
	}

	//--------------------------------------------------------------------------
	// reference center parameters
	//ife2_set_ref_cent = p_set_param->ref_cent;
	er_return = ife2_chg_ref_center_param(&(p_set_param->ref_cent));
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// filter parameters
	//ife2_set_filter = p_set_param->filter;
	er_return = ife2_chg_filter_param(&(p_set_param->filter));
	if (er_return != E_OK) {
		return er_return;
	}

	//ife2_set_gray_sta = p_set_param->gray_sta;
	er_return = ife2_chg_gray_statal_param(&(p_set_param->gray_sta));
	if (er_return != E_OK) {
		return er_return;
	}

	ife2_load();

	return er_return;
}
#endif
//-----------------------------------------------------------------------

#if defined (_NVT_EMULATION_)
static void ife2_time_out_cb(UINT32 event)
{
	DBG_ERR("IFE2 time out!\r\n");
	ife2_end_time_out_status = TRUE;
	ife2_platform_flg_set(FLGPTN_IFE2_FRAMEEND);
}
#endif

VOID ife2_wait_flag_frame_end(BOOL is_clear_flag)
{
	FLGPTN flag;

	if (is_clear_flag) {
		//clr_flg(flg_id_ife2, FLGPTN_IFE2_FRAMEEND);
		ife2_platform_flg_clear(FLGPTN_IFE2_FRAMEEND);
	}

#if defined (_NVT_EMULATION_)
	if (timer_open(&ife2_timer_id, ife2_time_out_cb) != E_OK) {
		DBG_WRN("IFE2 allocate timer fail\r\n");
	} else {
		timer_cfg(ife2_timer_id, 6000000, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	}
#endif

	//wai_flg(&uiFlag, flg_id_ife2, FLGPTN_IFE2_FRAMEEND, TWF_ORW | TWF_CLR);
	ife2_platform_flg_wait(&flag, FLGPTN_IFE2_FRAMEEND);

#if defined (_NVT_EMULATION_)
	timer_close(ife2_timer_id);
#endif

#if 0
	if (ife2_get_output_buf_info.op_mode == IFE2_OPMODE_D2D) {
#ifdef __KERNEL__
		fmem_dcache_sync((void *)ife2_get_output_buf_info.out_buf_addr, ife2_get_output_buf_info.out_buf_size, DMA_BIDIRECTIONAL);
#else
		dma_flushReadCacheDmaEnd(ife2_get_output_buf_info.out_buf_addr, ife2_get_output_buf_info.out_buf_size);
#endif
	}
#endif

}
//-----------------------------------------------------------------------

VOID ife2_wait_flag_linked_list_end(BOOL is_clear_flag)
{
	FLGPTN uiFlag;

	if (is_clear_flag) {
		//clr_flg(flg_id_ife2, FLGPTN_IFE2_LLEND);
		ife2_platform_flg_clear(FLGPTN_IFE2_LLEND);
	}

	//wai_flg(&uiFlag, flg_id_ife2, FLGPTN_IFE2_LLEND, TWF_ORW | TWF_CLR);

	ife2_platform_flg_wait(&uiFlag, FLGPTN_IFE2_LLEND);
}
//-----------------------------------------------------------------------

VOID ife2_wait_flag_linked_list_job_end(BOOL is_clear_flag)
{
	FLGPTN uiFlag;

	if (is_clear_flag) {
		//clr_flg(flg_id_ife2, FLGPTN_IFE2_JEND);
		ife2_platform_flg_clear(FLGPTN_IFE2_JEND);
	}

	//wai_flg(&uiFlag, flg_id_ife2, FLGPTN_IFE2_JEND, TWF_ORW | TWF_CLR);
	ife2_platform_flg_wait(&uiFlag, FLGPTN_IFE2_JEND);
}
//-----------------------------------------------------------------------

VOID ife2_clear_flag_frame_end(VOID)
{
	//clr_flg(flg_id_ife2, FLGPTN_IFE2_FRAMEEND);
	ife2_platform_flg_clear(FLGPTN_IFE2_FRAMEEND);
}
//-----------------------------------------------------------------------

VOID ife2_clear_flag_linked_list_end(VOID)
{
	//clr_flg(flg_id_ife2, FLGPTN_IFE2_LLEND);
	ife2_platform_flg_clear(FLGPTN_IFE2_LLEND);
}
//-----------------------------------------------------------------------

VOID ife2_clear_flag_linked_list_job_end(VOID)
{
	//clr_flg(flg_id_ife2, FLGPTN_IFE2_JEND);
	ife2_platform_flg_clear(FLGPTN_IFE2_JEND);
}
//-----------------------------------------------------------------------
#if 0
BOOL ife2_checkFlagFrameEnd(VOID)
{
	if (kchk_flg(flg_id_ife2, FLGPTN_IFE2_FRAMEEND)) {
		return TRUE;
	} else {
		return FALSE;
	}
}
#endif
//-----------------------------------------------------------------------



UINT32 ife2_get_int_status(VOID)
{
	return (ife2_get_engine_info(IFE2_GET_INTSTATUS));
}
//-----------------------------------------------------------------------


VOID ife2_clear_interrupt(UINT32 intp_val)
{
	ife2_set_engine_control(IFE2_CTRL_INTP, intp_val);
}
//-----------------------------------------------------------------------


VOID ife2_get_dma_addr(IFE2_DMAHDL_SEL chl_sel, IFE2_DMA_ADDR *p_get_addr)
{
	UINT32 tmp_addr = 0x0;

	if (p_get_addr != NULL) {
		switch (chl_sel) {
		case IFE2_DMAHDL_IN:
			tmp_addr = ife2_get_engine_info(IFE2_GET_IN_DMAY);

			p_get_addr->addr_y = tmp_addr;//dma_getNonCacheAddr(uiTmpAddrY);
			break;

		case IFE2_DMAHDL_OUT0:
			tmp_addr = ife2_get_engine_info(IFE2_GET_OUT_DMAY0);

			p_get_addr->addr_y = tmp_addr;//dma_getNonCacheAddr(uiTmpAddrY);
			break;

		default:
			// do nothing...
			break;
		}
	}
}
//-----------------------------------------------------------------------


ER ife2_chg_lineoffset(IFE2_LOFS_PARAM *p_set_lofs)
{
	ER er_return;
	UINT32 lofs_y = 0x0;

	if (p_set_lofs == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	// input lineoffset
	lofs_y = ALIGN_CEIL_4(p_set_lofs->lofs.lofs_y);
	if (abs(lofs_y - p_set_lofs->lofs.lofs_y) != 0) {
		DBG_WRN("IFE2: input lineoffset of Y channel is not word aligned, modified...\r\n");
	}

	if ((lofs_y > IFE2_LOFS_SIZE_MAX)) {
		DBG_ERR("IFE2: input lineoffset overflow...\r\n");
		return E_PAR;
	}

	switch (p_set_lofs->chl_sel) {
	case IFE2_DMAHDL_IN:
		//ife2_set_lineoffset(IFE2_IOTYPE_IN, lofs_y, lofs_y);
		ife2_set_input_lineoffset_reg(lofs_y, lofs_y);
		break;

	case IFE2_DMAHDL_OUT0:
		//ife2_set_lineoffset(IFE2_IOTYPE_OUT, lofs_y, lofs_y);
		ife2_set_output_lineoffset_reg(lofs_y, lofs_y);
		break;

	default:
		// do nothing ...
		break;
	}

	ife2_load();

	return E_OK;
}
//-----------------------------------------------------------------------


VOID ife2_get_lineoffset(IFE2_DMAHDL_SEL chl_sel, IFE2_LINEOFFSET *p_set_lofs)
{
	if (p_set_lofs != NULL) {
		switch (chl_sel) {
		case IFE2_DMAHDL_IN:
			p_set_lofs->lofs_y = ife2_get_engine_info(IFE2_GET_IN_LOFSY);
			break;

		case IFE2_DMAHDL_OUT0:
			p_set_lofs->lofs_y = ife2_get_engine_info(IFE2_GET_OUT_LOFSY);
			break;

		default:
			// do nothing...
			break;
		}
	}
}


//-----------------------------------------------------------------------


ER ife2_chg_dma_addr(IFE2_ADDR_PARAM *p_set_addr)
{
	ER er_return;

	UINT32 dma_addr_y = 0x0;
	UINT32 lofs_y = 0x0, flush_size = 0;
	IFE2_DMAFLUSH_TYPE dma_flush_type;


	if (p_set_addr == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	// check dma address
	dma_addr_y  = ALIGN_CEIL_4(p_set_addr->addr.addr_y);
	if (abs(dma_addr_y - p_set_addr->addr.addr_y) != 0) {
		DBG_WRN("IFE2: input address of Y channel is not word aligned, modified...\r\n");
	}


	ife2_get_in_size(&(ife2_get_image_size.img_size_h), &(ife2_get_image_size.img_size_v));

	ife2_get_lineoffset(p_set_addr->chl_sel, &ife2_get_lofs);
	lofs_y = ife2_get_lofs.lofs_y;

	// update channel address
	switch (p_set_addr->chl_sel) {
	case IFE2_DMAHDL_IN:

		if (dma_addr_y == 0) {
			//
			DBG_ERR("IFE2: input buffer address zero...\r\n");
			return E_PAR;
		}

		if (ife2_platform_dma_is_cacheable(dma_addr_y)) {
			flush_size = (lofs_y * ife2_get_image_size.img_size_v);
			flush_size = IFE2_ALIGN_CEIL_32(flush_size);

			dma_flush_type = IFE2_IFLUSH;
			ife2_flush_dma_buf(dma_flush_type, dma_addr_y, flush_size, ife2_get_output_buf_info.op_mode);
		}

		er_return = ife2_check_dma_addr(dma_addr_y);
		if (er_return != E_OK) {
			return er_return;
		}

		//ife2_set_address(IFE2_DMA_IN, dma_addr_y, dma_addr_y);
		ife2_set_input_dma_addr_reg(dma_addr_y, dma_addr_y);

		break;

	case IFE2_DMAHDL_OUT0:

		if (dma_addr_y == 0) {
			//
			DBG_ERR("IFE2: output buffer address zero...\r\n");
			return E_PAR;
		}

		if (ife2_platform_dma_is_cacheable(dma_addr_y)) {
			flush_size = (lofs_y * ife2_get_image_size.img_size_v);
			flush_size = IFE2_ALIGN_CEIL_32(flush_size);

			dma_flush_type = ((ife2_get_image_size.img_size_h == lofs_y) ? IFE2_OFLUSH_EQ : IFE2_OFLUSH_NEQ);
			ife2_flush_dma_buf(dma_flush_type, dma_addr_y, flush_size, ife2_get_output_buf_info.op_mode);
		}

		er_return = ife2_check_dma_addr(dma_addr_y);
		if (er_return != E_OK) {
			return er_return;
		}

		//ife2_set_address(IFE2_DMA_OUT0, dma_addr_y, dma_addr_y);
		ife2_set_output_dma_addr0_reg(dma_addr_y, dma_addr_y);

		break;

	default:
		break;
	}

	ife2_load();

	return E_OK;
}
//-----------------------------------------------------------------------

VOID ife2_chg_linked_list_addr(UINT32 addr)
{
	UINT32 uiTmpAddr = addr;

	//uiTmpAddr = dma_getPhyAddr(addr);

	ife2_set_linked_list_dma_addr_reg(uiTmpAddr);
}
//-----------------------------------------------------------------------

VOID ife2_set_linked_list_fire(VOID)
{
	ife2_set_linked_list_fire_reg(1);
}
//-----------------------------------------------------------------------

VOID ife2_set_linked_list_terminate(VOID)
{
	ife2_set_linked_list_terminate_reg(1);
}
//-----------------------------------------------------------------------



ER ife2_chg_ref_center_param(IFE2_REFCENT_PARAM *p_set_ref_cent)
{
	ER er_return;
	UINT32 i = 0x0;
	IFE2_FMT get_fmt;

	if (p_set_ref_cent == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	get_fmt = ife2_get_src_fmt();

	//--------------------------------------------------------------------------
	// reference center parameters
	if (get_fmt == IFE2_FMT_SIGL) {
		for (i = 0; i < 3; i++) {
			if (p_set_ref_cent->ref_cent_y.range_th[i] > IFE2_THRESHOLD_SIZE_MAX) {
				DBG_ERR("IFE2: Y channel reference cneter parameter - Rth overflow...\r\n");
				return E_PAR;
			}
		}

		for (i = 0; i < 4; i++) {
			if (p_set_ref_cent->ref_cent_y.range_wet[i] > IFE2_RCWT_SIZE_MAX) {
				DBG_ERR("IFE2: Y channel reference cneter parameter - RWt overflow...\r\n");
				return E_PAR;
			}
		}
		//ife2_set_ref_cent_info.ref_cent_y = p_set_ref_cent->ref_cent_y;
		//ife2_set_ref_cent_info.ref_cent_uv = p_set_ref_cent->ref_cent_y;

		//ife2_set_reference_center_info(IFE2_CHL_ALL, &(p_set_ref_cent->ref_cent_y), &(p_set_ref_cent->ref_cent_y));
		//ife2_set_reference_center_info(IFE2_CHL_ALL, &(p_set_ref_cent->ref_cent_y), &(p_set_ref_cent->ref_cent_uv));

		ife2_set_reference_center_cal_y_reg(p_set_ref_cent->ref_cent_y.range_th, p_set_ref_cent->ref_cent_y.cnt_wet, p_set_ref_cent->ref_cent_y.range_wet, p_set_ref_cent->ref_cent_y.outlier_th);
		ife2_set_reference_center_cal_uv_reg(p_set_ref_cent->ref_cent_y.range_th, p_set_ref_cent->ref_cent_y.cnt_wet, p_set_ref_cent->ref_cent_y.range_wet, p_set_ref_cent->ref_cent_y.outlier_th);

		ife2_set_outlier_difference_threshold_reg(p_set_ref_cent->ref_cent_y.outlier_dth, p_set_ref_cent->ref_cent_y.outlier_dth, p_set_ref_cent->ref_cent_y.outlier_dth);
	} else {
		for (i = 0; i < 3; i++) {
			if (p_set_ref_cent->ref_cent_y.range_th[i] > IFE2_THRESHOLD_SIZE_MAX) {
				DBG_ERR("IFE2: Y channel reference cneter parameter - Rth overflow...\r\n");
				return E_PAR;
			}

			if (p_set_ref_cent->ref_cent_uv.range_th[i] > IFE2_THRESHOLD_SIZE_MAX) {
				DBG_ERR("IFE2: UV channel reference cneter parameter - Rth overflow...\r\n");
				return E_PAR;
			}
		}

		for (i = 0; i < 4; i++) {
			if (p_set_ref_cent->ref_cent_y.range_wet[i] > IFE2_RCWT_SIZE_MAX) {
				DBG_ERR("IFE2: V channel reference cneter parameter - RWt overflow...\r\n");
				return E_PAR;
			}

			if (p_set_ref_cent->ref_cent_uv.range_wet[i] > IFE2_RCWT_SIZE_MAX) {
				DBG_ERR("IFE2: UV channel reference cneter parameter - RWt overflow...\r\n");
				return E_PAR;
			}
		}
		//ife2_set_ref_cent_info.ref_cent_y = p_set_ref_cent->ref_cent_y;
		//ife2_set_ref_cent_info.ref_cent_uv = p_set_ref_cent->ref_cent_uv;

		//ife2_set_reference_center_info(IFE2_CHL_ALL, &(p_set_ref_cent->ref_cent_y), &(p_set_ref_cent->ref_cent_uv));

		ife2_set_reference_center_cal_y_reg(p_set_ref_cent->ref_cent_y.range_th, p_set_ref_cent->ref_cent_y.cnt_wet, p_set_ref_cent->ref_cent_y.range_wet, p_set_ref_cent->ref_cent_y.outlier_th);
		ife2_set_reference_center_cal_uv_reg(p_set_ref_cent->ref_cent_uv.range_th, p_set_ref_cent->ref_cent_uv.cnt_wet, p_set_ref_cent->ref_cent_uv.range_wet, p_set_ref_cent->ref_cent_uv.outlier_th);

		ife2_set_outlier_difference_threshold_reg(p_set_ref_cent->ref_cent_y.outlier_dth, p_set_ref_cent->ref_cent_uv.outlier_dth, p_set_ref_cent->ref_cent_uv.outlier_dth);
	}

	//ife2_set_reference_center_info(IFE2_CHL_ALL, &ife2_set_ref_cent_info);

	ife2_load();

	return E_OK;
}
//-----------------------------------------------------------------------


ER ife2_chg_filter_param(IFE2_FILTER_PARAM *p_set_filter)
{
	ER er_return;
	UINT32 i = 0x0;

	IFE2_FMT get_fmt;

	if (p_set_filter == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	get_fmt = ife2_get_src_fmt();

	ife2_set_engine_control(IFE2_CTRL_YFTREN, (UINT32)p_set_filter->filter_y_en);

	//ife2_set_filter_win_size(p_set_filter->filter_size, p_set_filter->edge_ker_size);
	ife2_set_filter_size_reg((UINT32)p_set_filter->filter_size, (UINT32)p_set_filter->edge_ker_size);

	//ife2_set_edge_direction_threshold(&(p_set_filter->filter_edth));
	ife2_set_edge_direction_threshold_reg(p_set_filter->filter_edth.edth_pn, p_set_filter->filter_edth.edth_hv);
	if (get_fmt == IFE2_FMT_SIGL) {
		for (i = 0; i < 6; i++) {
			if (p_set_filter->filter_set_y.filter_wet[i] > IFE2_FWT_SIZE_MAX) {
				DBG_ERR("IFE2: Y channel filter weighting overflow...\r\n");
				return E_PAR;
			}
		}
		//ife2_set_filter_info.filter_set_y = p_set_filter->filter_set_y;
		//ife2_set_filter_info.filter_set_u = p_set_filter->filter_set_y;
		//ife2_set_filter_info.filter_set_v = p_set_filter->filter_set_y;

		//ife2_ife2_set_filter_info(IFE2_CHL_ALL, &(p_set_filter->filter_set_y), &(p_set_filter->filter_set_y), &(p_set_filter->filter_set_y));

		ife2_set_filter_computation_param_y_reg(p_set_filter->filter_set_y.filter_th, p_set_filter->filter_set_y.filter_wet);
		ife2_set_filter_computation_param_u_reg(p_set_filter->filter_set_y.filter_th, p_set_filter->filter_set_y.filter_wet);
		ife2_set_filter_computation_param_v_reg(p_set_filter->filter_set_y.filter_th, p_set_filter->filter_set_y.filter_wet);
	} else {
		for (i = 0; i < 6; i++) {
			if (p_set_filter->filter_set_y.filter_wet[i] > IFE2_FWT_SIZE_MAX) {
				DBG_ERR("IFE2: Y channel filter weighting overflow...\r\n");
				return E_PAR;
			}

			if (p_set_filter->filter_set_u.filter_wet[i] > IFE2_FWT_SIZE_MAX) {
				DBG_ERR("IFE2: U channel filter weighting overflow...\r\n");
				return E_PAR;
			}

			if (p_set_filter->filter_set_v.filter_wet[i] > IFE2_FWT_SIZE_MAX) {
				DBG_ERR("IFE2: V channel filter weighting overflow...\r\n");
				return E_PAR;
			}
		}

		//ife2_set_filter_info.filter_set_y = p_set_filter->filter_set_y;
		//ife2_set_filter_info.filter_set_u = p_set_filter->filter_set_u;
		//ife2_set_filter_info.filter_set_v = p_set_filter->filter_set_v;

		//ife2_ife2_set_filter_info(IFE2_CHL_ALL, &(p_set_filter->filter_set_y), &(p_set_filter->filter_set_u), &(p_set_filter->filter_set_v));

		ife2_set_filter_computation_param_y_reg(p_set_filter->filter_set_y.filter_th, p_set_filter->filter_set_y.filter_wet);
		ife2_set_filter_computation_param_u_reg(p_set_filter->filter_set_u.filter_th, p_set_filter->filter_set_u.filter_wet);
		ife2_set_filter_computation_param_v_reg(p_set_filter->filter_set_v.filter_th, p_set_filter->filter_set_v.filter_wet);
	}

	//ife2_ife2_set_filter_info(IFE2_CHL_ALL, &ife2_set_filter_info);

	ife2_load();

	return E_OK;
}
//-----------------------------------------------------------------------


ER ife2_chg_in_size(IFE2_IMG_SIZE *p_set_size)
{
	ER er_return;

	if (p_set_size == NULL) {
		DBG_ERR("IFE2: input parameter, NULL...\r\n");
		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	if ((p_set_size->img_size_h > IFE2_IMAGE_SIZE_MAX) || (p_set_size->img_size_v > IFE2_IMAGE_SIZE_MAX)) {
		DBG_ERR("IFE2: input image size overflow...\r\n");
		return E_PAR;
	}

	if ((ife2_get_output_buf_info.op_mode == IFE2_OPMODE_D2D) || (ife2_get_output_buf_info.op_mode == IFE2_OPMODE_SIE_IPP)) {
		if (nvt_get_chip_id() == CHIP_NA51055) {
			if (p_set_size->img_size_h > g_ife2_image_size_h_limt_for_all_dir) {
				DBG_ERR("IFE2: input image width can not larger than %d in D2D and all direct mode...\r\n", (int)g_ife2_image_size_h_limt_for_all_dir);
				return E_PAR;
			}
		} else if (nvt_get_chip_id() == CHIP_NA51084) {
			if (p_set_size->img_size_h > g_ife2_image_size_h_limt_for_all_dir) {
				DBG_ERR("IFE2: input image width can not larger than %d in D2D and all direct mode...\r\n", (int)g_ife2_image_size_h_limt_for_all_dir);
				return E_PAR;
			}
		} else {
            DBG_ERR("IFE2: get chip id %d out of range\r\n", nvt_get_chip_id());
            return E_PAR;
        }
	} else if (ife2_get_output_buf_info.op_mode == IFE2_OPMODE_IFE_IPP) {
		// do nothing...,  ife2 input image width = ime input image width / 4
	} else {
		DBG_ERR("IFE2: operation mode error...\r\n");
		return E_PAR;
	}

	//ife2_set_image_size(p_set_size->img_size_h, p_set_size->img_size_v);
	ife2_set_image_size_reg(p_set_size->img_size_h, p_set_size->img_size_v);

	ife2_load();

	return E_OK;
}
//-----------------------------------------------------------------------


VOID ife2_get_in_size(UINT32 *p_size_h, UINT32 *p_size_v)
{
	if (p_size_h != NULL) {
		*p_size_h = ife2_get_engine_info(IFE2_GET_IMG_HSIZE);
	}

	if (p_size_v != NULL) {
		*p_size_v = ife2_get_engine_info(IFE2_GET_IMG_VSIZE);
	}
}
//-----------------------------------------------------------------------


ER ife2_chg_src_fmt(IFE2_FMT src_fmt)
{
	ER er_return;

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	switch (src_fmt) {
	case IFE2_FMT_YUVP:
	case IFE2_FMT_SIGL:
	case IFE2_FMT_Y_UVP:

		er_return = E_OK;

		break;

	default:
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		DBG_ERR("IFE2: input source format error ...\r\n");
		return er_return;
	}

	ife2_set_engine_control(IFE2_CTRL_IFMT, (UINT32)src_fmt);

	ife2_load();

	return E_OK;
}
//-----------------------------------------------------------------------


ER ife2_chg_dst_fmt(IFE2_FMT dst_fmt)
{
	ER er_return;

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	switch (dst_fmt) {
	case IFE2_FMT_YUVP:
	case IFE2_FMT_SIGL:
	case IFE2_FMT_Y_UVP:

		er_return = E_OK;

		break;

	default:
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		DBG_ERR("IFE2: output destination format error ...\r\n");
		return er_return;
	}

	ife2_set_engine_control(IFE2_CTRL_OFMT, (UINT32)dst_fmt);

	ife2_load();

	return E_OK;
}
//-----------------------------------------------------------------------


UINT32 ife2_get_src_fmt(VOID)
{
	return ife2_get_engine_info(IFE2_GET_IFMT);
}
//-----------------------------------------------------------------------


UINT32 ife2_get_dst_fmt(VOID)
{
	return ife2_get_engine_info(IFE2_GET_OFMT);
}
//-----------------------------------------------------------------------

ER ife2_start(VOID)
{
	ER er_return;

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_STARTRUN);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d\r\n", IFE2_ENGINE_PAUSE);

		return er_return;
	}

#if (IFE2_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ife2_power_saving_en == TRUE) {
		if (ife2_get_output_buf_info.op_mode != IFE2_OPMODE_SIE_IPP) {
			ife2_platform_disable_sram_shutdown();
			ife2_attach(); //enable engine clock
		}
	}
#endif

#if 0 // remove for builtin
	// clear FW flag
	ife2_clear_flag_frame_end();

	ife2_set_engine_control(IFE2_CTRL_INTP, (UINT32)IFE2_INTS_ALL);
	ife2_set_engine_control(IFE2_CTRL_LOAD, (UINT32)IFE2_START_LOAD);

	ife2_engine_status = IFE2_ENGINE_RUN;
	ife2_set_engine_control(IFE2_CTRL_START, (UINT32)ENABLE);
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ife2_ctrl_flow_to_do == ENABLE) {
		// clear FW flag
		ife2_clear_flag_frame_end();

		ife2_set_engine_control(IFE2_CTRL_INTP, (UINT32)IFE2_INTS_ALL);
		ife2_set_engine_control(IFE2_CTRL_LOAD, (UINT32)IFE2_START_LOAD);

		ife2_engine_status = IFE2_ENGINE_RUN;
		ife2_set_engine_control(IFE2_CTRL_START, (UINT32)ENABLE);
	} else {
		if (ife2_get_output_buf_info.op_mode == IFE2_OPMODE_SIE_IPP) {
			ife2_engine_status = IFE2_ENGINE_RUN;
		} else {
			// clear FW flag
			ife2_clear_flag_frame_end();

			ife2_set_engine_control(IFE2_CTRL_INTP, (UINT32)IFE2_INTS_ALL);
			ife2_set_engine_control(IFE2_CTRL_LOAD, (UINT32)IFE2_START_LOAD);

			ife2_engine_status = IFE2_ENGINE_RUN;
			ife2_set_engine_control(IFE2_CTRL_START, (UINT32)ENABLE);
		}
	}
#else
	// clear FW flag
	ife2_clear_flag_frame_end();

	ife2_set_engine_control(IFE2_CTRL_INTP, (UINT32)IFE2_INTS_ALL);
	ife2_set_engine_control(IFE2_CTRL_LOAD, (UINT32)IFE2_START_LOAD);

	ife2_engine_status = IFE2_ENGINE_RUN;
	ife2_set_engine_control(IFE2_CTRL_START, (UINT32)ENABLE);
#endif

	ife2_ctrl_flow_to_do = ENABLE;

	//ife2_engine_status = IFE2_ENGINE_RUN;

#if defined (_NVT_EMULATION_)
	ife2_end_time_out_status = FALSE;
#endif

	return E_OK;
}
//-----------------------------------------------------------------------



ER ife2_pause(VOID)
{
	ER er_return;

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_PAUSE);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d, or %d\r\n", IFE2_ENGINE_RUN, IFE2_ENGINE_PAUSE);

		return er_return;
	}

#if 0
	if ((ife2_getEngineInfo(IFE2_GET_STARTSTATUS) == ON)) {
		DBG_ERR("IFE2: can not stop engine, still in running...\r\n");

		return E_SYS;
	}
#endif

	if (ife2_engine_status == IFE2_ENGINE_RUN) {
		ife2_set_engine_control(IFE2_CTRL_START, (UINT32)DISABLE);
	}


#if (IFE2_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ife2_power_saving_en == TRUE) {
		if (ife2_get_output_buf_info.op_mode != IFE2_OPMODE_SIE_IPP) {
			ife2_platform_enable_sram_shutdown();
			ife2_detach(); //disable engine clock
		}
	}
#endif

	ife2_engine_status = IFE2_ENGINE_PAUSE;

	return E_OK;
}
//-----------------------------------------------------------------------


UINT32 ife2_get_clock_rate(VOID)
{

#ifdef __KERNEL__
	return 240;
#else
	UINT32 ife2_clock_sel;
	ife2_clock_sel = pll_getClockRate(PLL_CLKSEL_IFE2);

	if (ife2_clock_sel == PLL_CLKSEL_IFE2_240) {
		return 240;
		//} else if (ife2_clock_sel == PLL_CLKSEL_IFE2_PLL6) {
		//  return 220;
	} else {
		DBG_ERR("Fail to get clock-rate, output zero\r\n");
		return 0;
	}
#endif
}
//-----------------------------------------------------------------------

UINT32 ife2_get_reg_base_addr(VOID)
{
	return ife2_get_engine_info(IFE2_GET_BASE_ADDR);
}
//-----------------------------------------------------------------------

VOID ife2_get_gray_average(UINT32 *p_get_avg_u, UINT32 *p_get_avg_v)
{
	if (p_get_avg_u != NULL) {
		*p_get_avg_u = ife2_get_engine_info(IFE2_GET_AVG_U);
	}

	if (p_get_avg_v) {
		*p_get_avg_v = ife2_get_engine_info(IFE2_GET_AVG_V);
	}
}
//-----------------------------------------------------------------------

VOID ife2_set_debug_function_enable(IFE2_FUNC_EN edge_en, IFE2_FUNC_EN ref_cent_en, IFE2_FUNC_EN eng_en)
{
	//ife2_set_debug_function((UINT32)edge_en, (UINT32)ref_cent_en, (UINT32)eng_en);
	ife2_set_debug_mode_reg((UINT32)edge_en, (UINT32)ref_cent_en, (UINT32)eng_en);
}
//-----------------------------------------------------------------------

ER ife2_chg_gray_statal_param(IFE2_GRAY_STATAL *p_set_gray_sta)
{
	ER er_return;

	if (p_set_gray_sta == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}


	//ife2_set_statistical_info(p_set_gray_sta);
	ife2_set_statistical_information_threshold_reg(p_set_gray_sta->u_th0, p_set_gray_sta->u_th1, p_set_gray_sta->v_th0, p_set_gray_sta->v_th1);

	ife2_load();

	return E_OK;
}
//-----------------------------------------------------------------------

INT32 ife2_get_burst_length(IFE2_GET_BURST_SEL get_bst_sel)
{
	return ife2_get_burst_length_info((UINT32)get_bst_sel);
}


ER ife2_chg_direct_in_param(IFE2_PARAM *p_set_param)
{
	ER er_return;

	IFE2_ADDR_PARAM set_addr = {0x0};
	IFE2_LOFS_PARAM set_lofs = {0x0};

	if (p_set_param == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	//--------------------------------------------------------------------------
	// input and output format
	er_return = ife2_chg_src_fmt(p_set_param->src_fmt);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ife2_chg_dst_fmt(p_set_param->dst_fmt);
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// input image size
	er_return = ife2_chg_in_size(&(p_set_param->img_size));
	if (er_return != E_OK) {
		return er_return;
	}


	//--------------------------------------------------------------------------
	// input lineoffset
	set_lofs = p_set_param->in_lofs;
	er_return = ife2_chg_lineoffset(&set_lofs);
	if (er_return != E_OK) {
		return er_return;
	}

	//--------------------------------------------------------------------------
	// input dma address
	set_addr = p_set_param->in_addr;
	er_return = ife2_chg_dma_addr(&set_addr);
	if (er_return != E_OK) {
		return er_return;
	}

	ife2_load();

	return er_return;
}

ER ife2_chg_direct_out_param(IFE2_PARAM *p_set_param)
{
	ER er_return;

	IFE2_OUTDEST_SEL out_dest;

	IFE2_ADDR_PARAM set_addr = {0x0};
	IFE2_LOFS_PARAM set_lofs = {0x0};

	if (p_set_param == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	//--------------------------------------------------------------------------
	// input data mode
	ife2_get_output_buf_info.op_mode = p_set_param->op_mode;
	switch (p_set_param->op_mode) {
	case IFE2_OPMODE_D2D:
		out_dest = IFE2_OUTDEST_DRAM;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)out_dest);
		break;

	case IFE2_OPMODE_IFE_IPP:
		out_dest = IFE2_OUTDEST_IME;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)out_dest);
		break;

	case IFE2_OPMODE_SIE_IPP:
		out_dest = IFE2_OUTDEST_ADRT;
		er_return = E_OK;

		ife2_set_engine_control(IFE2_CTRL_OUTDEST, (UINT32)out_dest);
		break;

	default:
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		DBG_ERR("IFE2: operation mode error ...\r\n");
		return er_return;
	}


	//--------------------------------------------------------------------------
	// engine control
	ife2_set_engine_control(IFE2_CTRL_OUTDRAM, p_set_param->out_dram_en);


	// do not update to PatternGen.
	if (p_set_param->op_mode == IFE2_OPMODE_D2D) {
		//--------------------------------------------------------------------------
		// output lineoffset
		set_lofs = p_set_param->out_lofs;
		er_return = ife2_chg_lineoffset(&set_lofs);
		if (er_return != E_OK) {
			return er_return;
		}

		//--------------------------------------------------------------------------
		// output dma address0
		set_addr = p_set_param->out_addr0;
		set_addr.chl_sel = IFE2_DMAHDL_OUT0;
		er_return = ife2_chg_dma_addr(&set_addr);
		if (er_return != E_OK) {
			return er_return;
		}
	}

	ife2_load();

	return er_return;
}

ER ife2_chg_direct_interrupt_enable_param(IFE2_PARAM *p_set_param)
{
	ER er_return;
	UINT32 interrupt_enable = 0x0;

	if (p_set_param == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	//--------------------------------------------------------------------------
	// interrupt enable
	interrupt_enable = p_set_param->interrupt_en;
	if (p_set_param->interrupt_en == 0) {
		interrupt_enable = (IFE2_INTE_FRMEND | IFE2_INTE_LL_END | IFE2_INTE_LL_JEND);
	}
	ife2_set_engine_control(IFE2_CTRL_INTPEN, interrupt_enable);

	return er_return;
}


ER ife2_chg_direct_gray_stl_param(IFE2_PARAM *p_set_param)
{
	ER er_return;

	if (p_set_param == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	//ife2_set_gray_sta = p_set_param->gray_sta;
	er_return = ife2_chg_gray_statal_param(&(p_set_param->gray_sta));
	if (er_return != E_OK) {
		return er_return;
	}

	ife2_load();

	return er_return;
}

ER ife2_chg_direct_reference_center_param(IFE2_PARAM *p_set_param)
{
	ER er_return;

	if (p_set_param == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	//--------------------------------------------------------------------------
	// reference center parameters
	//ife2_set_ref_cent = p_set_param->ref_cent;
	er_return = ife2_chg_ref_center_param(&(p_set_param->ref_cent));
	if (er_return != E_OK) {
		return er_return;
	}

	ife2_load();

	return er_return;
}

ER ife2_chg_direct_filter_param(IFE2_PARAM *p_set_param)
{
	ER er_return;

	if (p_set_param == NULL) {
		DBG_ERR("IFE2: input argument NULL ...\r\n");

		return E_PAR;
	}

	er_return = ife2_check_state_machine(ife2_engine_status, IFE2_OP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("IFE2: state Machine Error ...\r\n");
		DBG_ERR("IFE2: current State : %d\r\n", (int)ife2_engine_status);
		DBG_ERR("IFE2: correct State : %d or %d\r\n", (int)IFE2_ENGINE_READY, (int)IFE2_ENGINE_RUN);

		return er_return;
	}

	//--------------------------------------------------------------------------
	// filter parameters
	//ife2_set_filter = p_set_param->filter;
	er_return = ife2_chg_filter_param(&(p_set_param->filter));
	if (er_return != E_OK) {
		return er_return;
	}

	ife2_load();

	return er_return;
}


VOID ife2_set_builtin_flow_disable(VOID)
{
	ife2_ctrl_flow_to_do = ENABLE;
}
//-------------------------------------------------------------------------------



#if (defined(_NVT_EMULATION_) == ON)
// add for emulation
VOID ife2_emu_set_load(UINT32 set_load_type)
{
	ife2_set_engine_control(IFE2_CTRL_LOAD, set_load_type);
}
//-------------------------------------------------------------------------------

// add for emulation
ER ife2_emu_set_start(VOID)
{
	//ER erReturn = E_OK;

	// after changing status of state machine, trigger engine to run
	ife2_set_engine_control(IFE2_CTRL_START, ENABLE);

	return E_OK;
}
#endif

#if 0
#ifdef __KERNEL__

#if 0
VOID ife2_set_base_addr(UINT32 uiAddr)
{
	_IFE2_REGIOBASE = uiAddr;
}

VOID ife2_create_resource(VOID)
{
	OS_CONFIG_FLAG(flg_id_ife2);
	SEM_CREATE(semid_ife2, 1);
}

VOID ife2_release_resource(VOID)
{
	rel_flg(flg_id_ife2);
	SEM_DESTROY(semid_ife2);
}
#endif

EXPORT_SYMBOL(ife2_is_opened);
EXPORT_SYMBOL(ife2_open);
EXPORT_SYMBOL(ife2_set_mode);
EXPORT_SYMBOL(ife2_pause);
EXPORT_SYMBOL(ife2_start);
EXPORT_SYMBOL(ife2_close);
EXPORT_SYMBOL(ife2_wait_flag_frame_end);
EXPORT_SYMBOL(ife2_wait_flag_linked_list_end);
EXPORT_SYMBOL(ife2_wait_flag_linked_list_job_end);
EXPORT_SYMBOL(ife2_clear_flag_frame_end);
EXPORT_SYMBOL(ife2_clear_flag_linked_list_end);
EXPORT_SYMBOL(ife2_clear_flag_linked_list_job_end);
//EXPORT_SYMBOL(ife2_checkFlagFrameEnd);
EXPORT_SYMBOL(ife2_get_int_status);
EXPORT_SYMBOL(ife2_clear_interrupt);
EXPORT_SYMBOL(ife2_chg_in_size);
EXPORT_SYMBOL(ife2_get_in_size);
EXPORT_SYMBOL(ife2_chg_src_fmt);
EXPORT_SYMBOL(ife2_get_src_fmt);
EXPORT_SYMBOL(ife2_chg_dst_fmt);
EXPORT_SYMBOL(ife2_get_dst_fmt);
EXPORT_SYMBOL(ife2_chg_lineoffset);
EXPORT_SYMBOL(ife2_get_lineoffset);
EXPORT_SYMBOL(ife2_chg_dma_addr);
EXPORT_SYMBOL(ife2_get_dma_addr);
EXPORT_SYMBOL(ife2_chg_ref_center_param);
EXPORT_SYMBOL(ife2_chg_filter_param);
EXPORT_SYMBOL(ife2_chg_gray_statal_param);
EXPORT_SYMBOL(ife2_get_clock_rate);
EXPORT_SYMBOL(ife2_get_reg_base_addr);
EXPORT_SYMBOL(ife2_get_gray_average);
EXPORT_SYMBOL(ife2_set_debug_function_enable);
//EXPORT_SYMBOL(ife2_chg_all_param);
EXPORT_SYMBOL(ife2_get_burst_length);
EXPORT_SYMBOL(ife2_chg_linked_list_addr);
EXPORT_SYMBOL(ife2_set_linked_list_fire);
EXPORT_SYMBOL(ife2_set_linked_list_terminate);
//EXPORT_SYMBOL(ife2_set_base_addr);
//EXPORT_SYMBOL(ife2_create_resource);
//EXPORT_SYMBOL(ife2_release_resource);

EXPORT_SYMBOL(ife2_chg_direct_in_param);
EXPORT_SYMBOL(ife2_chg_direct_out_param);
EXPORT_SYMBOL(ife2_chg_direct_interrupt_enable_param);
EXPORT_SYMBOL(ife2_chg_direct_gray_stl_param);
EXPORT_SYMBOL(ife2_chg_direct_reference_center_param);
EXPORT_SYMBOL(ife2_chg_direct_filter_param);


#endif

#endif


//@}


