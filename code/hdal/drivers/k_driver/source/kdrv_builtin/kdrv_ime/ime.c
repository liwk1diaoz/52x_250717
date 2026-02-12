
/*
    IME module driver

    NT98520 IME module driver.

    @file       ime_lib.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "ime_platform.h"

#include "ime_int_public.h"

#include "ime_control_base.h"

#include "ime_int.h"
#include "ime_lib.h"

//#if defined (__KERNEL__)
//#include "kdrv_builtin.h"
//#endif


#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Type.h"
#include "ErrorNo.h"
#include "DrvCommon.h"
#include "interrupt.h"
//#include "pll.h"
#include "pll_protected.h"
#include "top.h"
#include "Utility.h"
#include "Memory.h"
#include "ist.h"

#include "ime_control_base.h"

#include "ime_int.h"
#include "ime_lib.h"
#endif

static IME_STATE_MACHINE ime_engine_state_machine_status = IME_ENGINE_IDLE;


static IME_INT_CB ime_isr_fastboot_callback_fp = NULL;
static IME_INT_CB ime_isr_non_fastboot_callback_fp = NULL;
static IME_INT_CB ime_isr_get_non_fastboot_callback_fp = NULL;


#if 0//(DRV_SUPPORT_IST == ENABLE)
#else
static IME_INT_CB ime_p_int_hdl_callback = NULL;
#endif

extern VOID ime_isr(VOID);


#if (defined(_NVT_EMULATION_) == ON)
static UINT32 ime_eng_frame_cnt = 0;
#endif

//static UINT32 ime_eng_clock_rate = 0;
//static UINT32 g_uiImeIntEn = 0; // used for ISR

// used to internal setting(copy)
//static IME_INPATH_INFO ime_set_input_path_info = {0x0};
//static IME_OUTPATH_INFO g_SetOutPathInfo = {0x0};
//static IME_STL_HIST_INFO ime_set_stl_hist_info = {0x0};
//static IME_STL_ROI_INFO ime_set_stl_roi_info = {0x0};

//static IME_OUTPATH_INFO ime_set_output_path1_info = {0x0};
//static IME_OUTPATH_INFO ime_set_output_path2_info = {0x0};
//static IME_OUTPATH_INFO ime_set_output_path3_info = {0x0};
//static IME_OUTPATH_INFO ime_set_output_path4_info = {0x0};
//static IME_OUTPATH_INFO ime_set_output_path5_info = {0x0};


IME_HV_STRIPE_INFO      ime_stripe_info = {0x0};


static INT32 ime_set_pm_coef_line0[4] = {0}, ime_set_pm_coef_line1[4] = {0}, ime_set_pm_coef_line2[4] = {0}, ime_set_pm_coef_line3[4] = {0};
static INT32 ime_set_pm_coef_line_hlw0[4] = {0}, ime_set_pm_coef_line_hlw1[4] = {0}, ime_set_pm_coef_line_hlw2[4] = {0}, ime_set_pm_coef_line_hlw3[4] = {0};


#if (defined(_NVT_EMULATION_) == ON)
#include "comm/timer.h"
static TIMER_ID ime_timer_id;
BOOL ime_end_time_out_status = FALSE;
#define IME_POWER_SAVING_EN 0//do not enable power saving in emulation
#else
#define IME_POWER_SAVING_EN 1
#endif

IME_BURST_LENGTH ime_set_burst_info_dram1 = {
	IME_BURST_64W,  // 0, input Y
	IME_BURST_64W,  // 1, input U
	IME_BURST_64W,  // 2, input V
	IME_BURST_64W,  // 3, output path1 Y
	IME_BURST_64W,  // 4, output path1 U
	IME_BURST_64W,  // 5, output path1 V
	IME_BURST_64W,  // 6, output path2 Y
	IME_BURST_64W,  // 7, output path2 UV
	IME_BURST_32W,  // 8, output path3 Y
	IME_BURST_32W,  // 9, output path3 UV
	IME_BURST_32W,  // 10, output path4 Y
	IME_BURST_32W,  // 11, LCA sub-in
	IME_BURST_32W,  // 12, LCA sub-out
	IME_BURST_64W,  // 13, OSD
	IME_BURST_32W,  // 14, privacy mask input
	IME_BURST_64W,  // 15, 3DNR input Y
	IME_BURST_64W,  // 16, 3DNR input UV
	IME_BURST_64W,  // 17, 3DNR output Y
	IME_BURST_64W,  // 18, 3DNR output UV
	IME_BURST_32W,  // 19, 3DNR input motion vector
	IME_BURST_32W,  // 20, 3DNR output motion vector
	IME_BURST_32W,  // 21, 3DNR input motion sta
	IME_BURST_32W,  // 22, 3DNR output motion sta
	IME_BURST_32W,  // 23, 3DNR output motion sta for ROI
	IME_BURST_32W,  // 24, MD output STA
};

static IME_USED_STATUS ime_eng_lock_status  = IME_ENGINE_UNLOCKED;


static VOID ime_load(VOID);
static ER ime_lock(VOID);
static ER ime_unlock(VOID);
static ER ime_attach(VOID);
static ER ime_detach(VOID);

//static IME_FUNC_EN ime_get_tmnr_enable(VOID);
//static IME_FUNC_EN ime_get_tmnr_decoder_enable(VOID);

//-----------------------------------------------------------------------------

static VOID ime_load(VOID)
{
#if (defined(_NVT_EMULATION_) == OFF)
	{
		if (ime_engine_state_machine_status == IME_ENGINE_RUN)
		{
			if (ime_engine_operation_mode == IME_OPMODE_SIE_IPP) {
				//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_DIRECT_START_LOAD);
				ime_load_reg((UINT32)IME_DIRECT_START_LOAD, ENABLE);
			} else {
				//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_FRMEND_LOAD);
				ime_load_reg((UINT32)IME_FRMEND_LOAD, ENABLE);
			}
		}
	}
#endif
}


static ER ime_lock(VOID)
{
	ER er_return = E_OK;

	er_return = ime_platform_sem_wait();
	if (er_return != E_OK) {
		return er_return;
	}

	ime_eng_lock_status = IME_ENGINE_LOCKED;  // engine is opened

	return er_return;
}
//------------------------------------------------------------------------------


static ER ime_unlock(VOID)
{
	ER er_return;

	ime_eng_lock_status = IME_ENGINE_UNLOCKED;  // engine is closed

	er_return = ime_platform_sem_signal();
	if (er_return != E_OK) {
		return er_return;
	}

	return er_return;
}
//------------------------------------------------------------------------------


static ER ime_attach(VOID)
{
	ime_platform_enable_clk();

	return E_OK;
}
//------------------------------------------------------------------------------


static ER ime_detach(VOID)
{
	ime_platform_disable_clk();

	return E_OK;
}
//-----------------------------------------------------------------------------


//-------------------------------------------------------------------------------
// public function definition

#if (defined(_NVT_EMULATION_) == ON)
#define ime_get_val_uint32(addr)           (*(volatile UINT32*)(addr))
#define ime_set_val_uint32(addr, value)    (*(volatile UINT32*)(addr) = (UINT32)(value))
#endif

VOID ime_isr(VOID)
{
	UINT32 interrupt_enable = 0x0;
	UINT32 interrupt_status = 0x0;
	UINT32 interrupt_handle = 0x0;

	UINT32 imeInterruptCB = 0x0;
	FLGPTN flg = 0x0;

	interrupt_status = ime_get_interrupt_status_reg();

	if (ime_platform_get_init_status() == FALSE) {
		ime_set_interrupt_status_reg(interrupt_status);
		return;
	}

	interrupt_enable = ime_get_interrupt_enable_reg();
	interrupt_handle = (interrupt_enable & interrupt_status);
	ime_set_interrupt_status_reg(interrupt_handle);

	if (interrupt_handle == 0) {
		return;
	}
	//ime_set_engine_control(IME_ENGINE_SET_INTPS, interrupt_handle);

	//vk_print_isr("ime-status: %08x\r\n", interrupt_handle);

	if (interrupt_handle != 0) {
		if ((ime_isr_fastboot_callback_fp == NULL) && (ime_isr_non_fastboot_callback_fp == NULL)) {
			ime_p_int_hdl_callback = NULL;
		} else if ((ime_isr_fastboot_callback_fp == NULL) && (ime_isr_non_fastboot_callback_fp != NULL)) {
			ime_p_int_hdl_callback = ime_isr_non_fastboot_callback_fp;
		} else if ((ime_isr_fastboot_callback_fp != NULL) && (ime_isr_non_fastboot_callback_fp == NULL)) {
			ime_p_int_hdl_callback = ime_isr_fastboot_callback_fp;
		} else if ((ime_isr_fastboot_callback_fp != NULL) && (ime_isr_non_fastboot_callback_fp != NULL)) {
			ime_p_int_hdl_callback = ime_isr_fastboot_callback_fp;
		}

		if (interrupt_handle & IME_INTS_FRM_START) {
			flg |= FLGPTN_IME_FRAMESTART;
			imeInterruptCB |= IME_INTS_FRM_START;

			//vk_print_isr("ime-frame-start %d...%08x\r\n", ime_eng_frame_cnt, imeInterruptCB);
		}

		if (interrupt_handle & IME_INTS_FRM_END) {
			//DBG_DUMP("m\r\n");

			flg |= FLGPTN_IME_FRAMEEND;
			imeInterruptCB |= IME_INTS_FRM_END;

			if ((ime_isr_fastboot_callback_fp != NULL) && (ime_isr_non_fastboot_callback_fp != NULL)) {
				imeInterruptCB |= IME_INTS_FB_FRM_END;
			}

			//vk_print_isr("ime-frame-end %d...%08x\r\n", (int)ime_eng_frame_cnt, imeInterruptCB);

#if (defined(_NVT_EMULATION_) == ON)
			ime_eng_frame_cnt++;
			DBG_DUMP("^Yframe-end... %d\r\n", (int)ime_eng_frame_cnt);
#endif
		}

		if (interrupt_handle & IME_INTS_FRM_ERR) {
			//flg |= FLGPTN_IME_FRAMEEND;
			imeInterruptCB |= IME_INTS_FRM_ERR;

			//DBG_ERR("direct mode frame error...\r\n");
			vk_print_isr("direct mode frame error...\r\n");
		}


		if (interrupt_handle & IME_INTS_BP1) {
			flg |= FLGPTN_IME_BP1;
			imeInterruptCB |= IME_INTS_BP1;

#if (defined(_NVT_EMULATION_) == ON)
			DBG_DUMP("^Ybreak-point-1...\r\n");
#endif
		}

		if (interrupt_handle & IME_INTS_BP2) {
			flg |= FLGPTN_IME_BP2;
			imeInterruptCB |= IME_INTS_BP2;

#if (defined(_NVT_EMULATION_) == ON)
			DBG_DUMP("^Ybreak-point-2...\r\n");
#endif
		}

		if (interrupt_handle & IME_INTS_BP3) {
			flg |= FLGPTN_IME_BP3;
			imeInterruptCB |= IME_INTS_BP3;

#if (defined(_NVT_EMULATION_) == ON)
			DBG_DUMP("^Ybreak-point-3...\r\n");
#endif
		}

		if (interrupt_handle & IME_INTS_LL_END) {
			//DBG_DUMP("m\r\n");
			flg |= FLGPTN_IME_LLEND;
			imeInterruptCB |= IME_INTS_LL_END;
		}

		if (interrupt_handle & IME_INTS_LL_JEND) {
			//DBG_DUMP("m\r\n");
			flg |= FLGPTN_IME_JEND;
			imeInterruptCB |= IME_INTS_LL_JEND;
		}


		if (interrupt_handle & IME_INTS_P1_ENC_OVR) {
			imeInterruptCB |= IME_INTS_P1_ENC_OVR;

			//DBG_ERR("output path1 encoder overflow...\r\n");
			vk_print_isr("output path1 encoder overflow...\r\n");
		}

		if (interrupt_handle & IME_INTS_TMNR_ENC_OVR) {
			imeInterruptCB |= IME_INTS_TMNR_ENC_OVR;

			//DBG_ERR("3DNR ref-out encoder overflow...\r\n");
			vk_print_isr("3DNR ref-out encoder overflow...\r\n");
		}

		if (interrupt_handle & IME_INTS_TMNR_DEC_ERR) {
			imeInterruptCB |= IME_INTS_TMNR_DEC_ERR;

			//DBG_ERR("3DNR ref-in decoder error...\r\n");
			vk_print_isr("3DNR ref-in decoder error...\r\n");
		}


		if (interrupt_handle & IME_INTS_TMNR_MOT_END) {
			imeInterruptCB |= IME_INTS_TMNR_MOT_END;

#if (defined(_NVT_EMULATION_) == ON)
			DBG_DUMP("^YIME_INTS_TMNR_MOT_END...\r\n");
#endif
		}

		if (interrupt_handle & IME_INTS_TMNR_MV_END) {
			imeInterruptCB |= IME_INTS_TMNR_MV_END;

#if (defined(_NVT_EMULATION_) == ON)
			DBG_DUMP("^YIME_INTS_TMNR_MV_END...\r\n");
#endif
		}

		if (interrupt_handle & IME_INTS_TMNR_STA_END) {
			imeInterruptCB |= IME_INTS_TMNR_STA_END;

#if (defined(_NVT_EMULATION_) == ON)
			DBG_DUMP("^YIME_INTS_TMNR_STA_END...\r\n");
#endif
		}

#if (DRV_SUPPORT_IST == ENABLE)
		if (pfIstCB_IME != NULL) {
			drv_setIstEvent(DRV_IST_MODULE_IME, imeInterruptCB);
		}
#else
		if (ime_p_int_hdl_callback != NULL) {
			ime_p_int_hdl_callback(imeInterruptCB);

			if (interrupt_handle & IME_INTS_FRM_END) {
				if ((ime_isr_fastboot_callback_fp != NULL) && (ime_isr_non_fastboot_callback_fp != NULL)) {
					ime_isr_fastboot_callback_fp = NULL;
				}
			}
		}
#endif

		//iset_flg(flg_id_ime, flg); // isr task...
		ime_platform_flg_set(flg);

	}

}
//-------------------------------------------------------------------------------

ER ime_start(VOID)
{
	ER er_return = E_OK;

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_START);
	if (er_return != E_OK) {
		DBG_ERR("State Machine error ...\r\n");
		DBG_ERR("Current status : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct status : %d\r\n", (int)IME_ENGINE_PAUSE);

		return er_return;
	}

#if (IME_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ime_power_saving_en == TRUE) {
		if (ime_engine_operation_mode != IME_OPMODE_SIE_IPP) {
			ime_platform_disable_sram_shutdown();
			ime_attach(); //enable engine clock
		}
	}
#endif

#if (defined(_NVT_EMULATION_) == OFF)

	if (ime_engine_state_machine_status != IME_ENGINE_RUN) {
		er_return = ime_check_input_path_stripe();
		if (er_return != E_OK) {
			return er_return;
		}
	}
#endif

	if (nvt_get_chip_id() != CHIP_NA51055) {
		if (ime_get_low_delay_mode_reg() == IME_FUNC_ENABLE) {
#if defined (__KERNEL__) || defined (__LINUX)
			if (clk_get_phase(ime_clk[0]) == 1) {
				clk_set_phase(ime_clk[0], 0);
			}

			if (clk_get_phase(ime_pclk[0]) == 1) {
				clk_set_phase(ime_pclk[0], 0);
			}
#else
			pll_clearClkAutoGating(IME_M_GCLK);
			pll_clearPclkAutoGating(IME_GCLK);
#endif
		} else {

#if defined (__KERNEL__) || defined (__LINUX)
			if (clk_get_phase(ime_clk[0]) == 0) {
				clk_set_phase(ime_clk[0], 1);
			}

			if (clk_get_phase(ime_pclk[0]) == 0) {
				clk_set_phase(ime_pclk[0], 1);
			}
#else
			pll_setClkAutoGating(IME_M_GCLK);
			pll_setPclkAutoGating(IME_GCLK);
#endif

		}
	}

#if 0  // remove for builtin
	// clear FW frame-end flag
	ime_clear_flag_frame_end();

	if (ime_engine_operation_mode == IME_OPMODE_SIE_IPP) {
		//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_DIRECT_START_LOAD);
		ime_load_reg((UINT32)IME_DIRECT_START_LOAD, ENABLE);
	} else {
		//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_START_LOAD);
		ime_load_reg((UINT32)IME_START_LOAD, ENABLE);
	}

	// change status of state machine first
	ime_engine_state_machine_status = IME_ENGINE_RUN;

	// after changing status of state machine, trigger engine to run
	//ime_set_engine_control(IME_ENGINE_SET_START, ENABLE);
	ime_start_reg(ENABLE);
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ime_ctrl_flow_to_do == ENABLE) {
		// clear FW frame-end flag
		ime_clear_flag_frame_end();

		if (ime_engine_operation_mode == IME_OPMODE_SIE_IPP) {
			//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_DIRECT_START_LOAD);
			ime_load_reg((UINT32)IME_DIRECT_START_LOAD, ENABLE);
		} else {
			//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_START_LOAD);
			ime_load_reg((UINT32)IME_START_LOAD, ENABLE);
		}

        ime_isr_fastboot_callback_fp = NULL;
		ime_isr_non_fastboot_callback_fp = ime_isr_get_non_fastboot_callback_fp;

		// change status of state machine first
		ime_engine_state_machine_status = IME_ENGINE_RUN;

		// after changing status of state machine, trigger engine to run
		//ime_set_engine_control(IME_ENGINE_SET_START, ENABLE);
		ime_start_reg(ENABLE);
	} else {
		if (ime_engine_operation_mode == IME_OPMODE_SIE_IPP) {

			ime_isr_non_fastboot_callback_fp = ime_isr_get_non_fastboot_callback_fp;

			// change status of state machine first
			ime_engine_state_machine_status = IME_ENGINE_RUN;
		} else {
			// clear FW frame-end flag
			ime_clear_flag_frame_end();

			if (ime_engine_operation_mode == IME_OPMODE_SIE_IPP) {
				//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_DIRECT_START_LOAD);
				ime_load_reg((UINT32)IME_DIRECT_START_LOAD, ENABLE);
			} else {
				//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_START_LOAD);
				ime_load_reg((UINT32)IME_START_LOAD, ENABLE);
			}

			ime_isr_non_fastboot_callback_fp = ime_isr_get_non_fastboot_callback_fp;

			// change status of state machine first
			ime_engine_state_machine_status = IME_ENGINE_RUN;

			// after changing status of state machine, trigger engine to run
			//ime_set_engine_control(IME_ENGINE_SET_START, ENABLE);
			ime_start_reg(ENABLE);
		}
	}
#else
	// clear FW frame-end flag
	ime_clear_flag_frame_end();

	if (ime_engine_operation_mode == IME_OPMODE_SIE_IPP) {
		//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_DIRECT_START_LOAD);
		ime_load_reg((UINT32)IME_DIRECT_START_LOAD, ENABLE);
	} else {
		//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)IME_START_LOAD);
		ime_load_reg((UINT32)IME_START_LOAD, ENABLE);
	}

    ime_isr_fastboot_callback_fp = NULL;
	ime_isr_non_fastboot_callback_fp = ime_isr_get_non_fastboot_callback_fp;


	// change status of state machine first
	ime_engine_state_machine_status = IME_ENGINE_RUN;

	// after changing status of state machine, trigger engine to run
	//ime_set_engine_control(IME_ENGINE_SET_START, ENABLE);
	ime_start_reg(ENABLE);
#endif

	ime_ctrl_flow_to_do = ENABLE;

	// change status of state machine first
	//ime_engine_state_machine_status = IME_ENGINE_RUN;

#if (defined(_NVT_EMULATION_) == ON)
	ime_end_time_out_status = FALSE;
	DBG_DUMP("ime start...\r\n");
#endif

	return E_OK;
}
//-------------------------------------------------------------------------------

#if (defined(_NVT_EMULATION_) == ON)
// add for emulation
VOID ime_set_emu_load(IME_LOAD_TYPE SetLoadType)
{
	//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)SetLoadType);
	ime_load_reg((UINT32)SetLoadType, ENABLE);

	DBG_DUMP("^Yime load: %d...\r\n", (int)SetLoadType);
}

// add for emulation
ER ime_set_emu_start(VOID)
{
	ER er_return = E_OK;

	if (ime_engine_state_machine_status != IME_ENGINE_RUN) {
		er_return = ime_check_input_path_stripe();
		if (er_return != E_OK) {
			return er_return;
		}
	}

	// after changing status of state machine, trigger engine to run
	//ime_set_engine_control(IME_ENGINE_SET_START, ENABLE);
	ime_start_reg(ENABLE);

	DBG_DUMP("^Yime start...\r\n");

	return E_OK;
}
//-------------------------------------------------------------------------------
#endif


ER ime_open(IME_OPENOBJ *pImeOpenInfo)
{
	ER er_return = E_OK;

#if 0
#ifdef __KERNEL__

	struct clk *parent_clk;

#else

	UINT32 uiSelectedClock;

#endif
#endif

	if (pImeOpenInfo == NULL) {
		DBG_ERR("IME: open info. NULL...\r\n");

		return E_SYS;
	}

	// get ime resource
	er_return = ime_lock();
	if (er_return != E_OK) {
		DBG_ERR("IME: Re-opened...\r\n");

		return er_return;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_OPEN);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d\r\n", (int)IME_ENGINE_IDLE);

		ime_unlock();

		return er_return;
	}

	ime_base_reg_init();

#if (IME_DBG_MSG == ENABLE)
	ime_eng_frame_cnt = 0;
#endif

	ime_platform_set_clk_rate(pImeOpenInfo->uiImeClockSel);

#if 0
#ifdef __KERNEL__

	if (IS_ERR(ime_clk[0])) {
		DBG_ERR("IME: get clk fail...0x%p\r\n", ime_clk[0]);
		return E_SYS;
	}

	if (pImeOpenInfo->uiImeClockSel < 240) {
		DBG_WRN("IME: input frequency %d round to 240MHz\r\n", pImeOpenInfo->uiImeClockSel);
		parent_clk = clk_get(NULL, "fix240m");
		ime_eng_clock_rate = 240;
	} else if (pImeOpenInfo->uiImeClockSel == 240) {
		parent_clk = clk_get(NULL, "fix240m");
		ime_eng_clock_rate = 240;
	} else if (pImeOpenInfo->uiImeClockSel < 320) {
		DBG_WRN("IME: input frequency %d round to 240MHz\r\n", pImeOpenInfo->uiImeClockSel);
		parent_clk = clk_get(NULL, "fix240m");
		ime_eng_clock_rate = 240;
	} else if (pImeOpenInfo->uiImeClockSel == 320) {
		parent_clk = clk_get(NULL, "pllf320");
		ime_eng_clock_rate = 320;
	} else if (pImeOpenInfo->uiImeClockSel < 360) {
		DBG_WRN("IME: input frequency %d round to 320MHz\r\n", pImeOpenInfo->uiImeClockSel);
		parent_clk = clk_get(NULL, "pllf320");
		ime_eng_clock_rate = 320;
	} else if (pImeOpenInfo->uiImeClockSel == 360) {
		parent_clk = clk_get(NULL, "pll13");
		ime_eng_clock_rate = 360;
	} else {
		DBG_WRN("IME: input frequency %d round to 360MHz\r\n", pImeOpenInfo->uiImeClockSel);
		parent_clk = clk_get(NULL, "pll13");
		ime_eng_clock_rate = 360;
	}

	clk_set_parent(ime_clk[0], parent_clk);
	clk_put(parent_clk);

#else
	// Turn on power
	pmc_turnonPower(PMC_MODULE_IME);

	// select clock
	if (pImeOpenInfo->uiImeClockSel > 360) {
		uiSelectedClock = PLL_CLKSEL_IME_PLL13;
		DBG_WRN("IME: user-clock - %d, engine-clock: 360MHz\r\n", pImeOpenInfo->uiImeClockSel);
	} else if (pImeOpenInfo->uiImeClockSel == 360) {
		uiSelectedClock = PLL_CLKSEL_IME_PLL13;
	} else if (pImeOpenInfo->uiImeClockSel > 320) {
		uiSelectedClock = PLL_CLKSEL_IME_PLL13;

		ime_eng_clock_rate = 360;

		DBG_WRN("IME: user-clock - %d, engine-clock: 360MHz\r\n", pImeOpenInfo->uiImeClockSel);

	} else if (pImeOpenInfo->uiImeClockSel == 320) {
		uiSelectedClock = PLL_CLKSEL_IME_320;

		ime_eng_clock_rate = 320;
	} else if (pImeOpenInfo->uiImeClockSel > 240) {
		uiSelectedClock = PLL_CLKSEL_IME_320;

		ime_eng_clock_rate = 320;

		DBG_WRN("IME: user-clock - %d, engine-clock: 320MHz\r\n", pImeOpenInfo->uiImeClockSel);
	} else if (pImeOpenInfo->uiImeClockSel == 240) {
		uiSelectedClock = PLL_CLKSEL_IME_240;

		ime_eng_clock_rate = 240;
	} else {
		uiSelectedClock = PLL_CLKSEL_IME_240;

		ime_eng_clock_rate = 240;

		DBG_WRN("IME: user-clock - %d, engine-clock: 240MHz\r\n", pImeOpenInfo->uiImeClockSel);
	}

	if (uiSelectedClock == PLL_CLKSEL_IME_PLL13) {
		if (pll_getPLLEn(PLL_ID_13) == FALSE) {
			pll_setPLLEn(PLL_ID_13, TRUE);
		}
	}

	if (uiSelectedClock == PLL_CLKSEL_IME_320) {
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		}
	}

	pll_setClockRate(PLL_CLKSEL_IME, uiSelectedClock);

#endif
#endif

	//ime_platform_prepare_clk();
	ime_attach();

	ime_platform_disable_sram_shutdown();

#if 0 // remove for builtin
	//ime_set_engine_control(IME_ENGINE_SET_INTPE, 0x00000000);
	//ime_set_engine_control(IME_ENGINE_SET_INTPS, IME_INTS_ALL);

	ime_set_interrupt_enable_reg(0x00000000);
	ime_set_interrupt_status_reg(IME_INTS_ALL);

	ime_platform_flg_clear(FLGPTN_IME_FRAMEEND);
	ime_platform_flg_clear(FLGPTN_IME_LLEND);
	ime_platform_flg_clear(FLGPTN_IME_JEND);
	ime_platform_flg_clear(FLGPTN_IME_BP1);
	ime_platform_flg_clear(FLGPTN_IME_BP2);
	ime_platform_flg_clear(FLGPTN_IME_BP3);
#endif

	// soft-reset
	//ime_set_engine_control(IME_ENGINE_SET_RESET, (UINT32)ENABLE);
	//ime_set_engine_control(IME_ENGINE_SET_RESET, (UINT32)DISABLE);
#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ime_ctrl_flow_to_do == ENABLE) {
		//ime_set_engine_control(IME_ENGINE_SET_INTPE, 0x00000000);
		//ime_set_engine_control(IME_ENGINE_SET_INTPS, IME_INTS_ALL);
		ime_set_interrupt_enable_reg(0x00000000);
		ime_set_interrupt_status_reg(IME_INTS_ALL);

		ime_platform_flg_clear(FLGPTN_IME_FRAMEEND);
		ime_platform_flg_clear(FLGPTN_IME_LLEND);
		ime_platform_flg_clear(FLGPTN_IME_JEND);
		ime_platform_flg_clear(FLGPTN_IME_BP1);
		ime_platform_flg_clear(FLGPTN_IME_BP2);
		ime_platform_flg_clear(FLGPTN_IME_BP3);

		ime_reset_reg((UINT32)ENABLE);
		ime_reset_reg((UINT32)DISABLE);
		//printk("ime-open_reset, done\r\n");
	}
#else
	//ime_set_engine_control(IME_ENGINE_SET_INTPE, 0x00000000);
	//ime_set_engine_control(IME_ENGINE_SET_INTPS, IME_INTS_ALL);
	ime_set_interrupt_enable_reg(0x00000000);
	ime_set_interrupt_status_reg(IME_INTS_ALL);

	ime_platform_flg_clear(FLGPTN_IME_FRAMEEND);
	ime_platform_flg_clear(FLGPTN_IME_LLEND);
	ime_platform_flg_clear(FLGPTN_IME_JEND);
	ime_platform_flg_clear(FLGPTN_IME_BP1);
	ime_platform_flg_clear(FLGPTN_IME_BP2);
	ime_platform_flg_clear(FLGPTN_IME_BP3);

	ime_reset_reg((UINT32)ENABLE);
	ime_reset_reg((UINT32)DISABLE);
#endif

	ime_platform_int_enable();
	//drv_enableInt(DRV_INT_IME);

	if (nvt_get_chip_id() == CHIP_NA51055) {
		ime_max_stp_isd_out_buf_size = 1344;

		ime_stp_size_max = 672;  // ime_stp_size_max = 2688 / 4
		ime_stp_num_max = 255;
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		ime_max_stp_isd_out_buf_size = 2048;

		ime_stp_size_max = 1024;  // ime_stp_size_max = 2688 / 4
		ime_stp_num_max = 255;
	} else {
        DBG_ERR("get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }

#if (DRV_SUPPORT_IST == ENABLE)
	pfIstCB_IME     = pImeOpenInfo->FP_IMEISR_CB;
#else
	//ime_p_int_hdl_callback  = pImeOpenInfo->FP_IMEISR_CB;
	ime_isr_get_non_fastboot_callback_fp = pImeOpenInfo->FP_IMEISR_CB;
#endif

	ime_stripe_info.stripe_cal_mode = IME_STRIPE_AUTO_MODE;



	ime_engine_state_machine_status = IME_ENGINE_READY;

	return E_OK;
}
//-------------------------------------------------------------------------------


BOOL ime_is_open(VOID)
{
	BOOL state;

	if (ime_eng_lock_status == IME_ENGINE_UNLOCKED) {
		state = FALSE;
	} else {
		state = TRUE;
	}

	return state;
}
//------------------------------------------------------------------------------

void ime_set_builtin_flow_disable(void)
{
    ime_ctrl_flow_to_do = ENABLE;
}
//------------------------------------------------------------------------------


ER ime_set_mode(IME_MODE_PRAM *p_eng_info)
{
	ER er_return = E_OK;

	//UINT32 ime_interrupt_enable = 0x0;

	if (p_eng_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_PARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d\r\n", (int)IME_ENGINE_READY);

		return er_return;
	}

	if (ime_ctrl_flow_to_do == DISABLE) {
	    ime_engine_operation_mode = p_eng_info->operation_mode;

		goto SKIP_PARAMS;
	}

	//DBG_ERR("ime_set_mode\r\n");

#if (defined(_NVT_EMULATION_) == ON)
	ime_engine_state_machine_status = IME_ENGINE_PAUSE;
	return er_return;

SKIP_PARAMS:
	ime_engine_state_machine_status = IME_ENGINE_PAUSE;
	return er_return;
#else

#if 0 // remove for builtin
	//ime_set_init();
	ime_set_interrupt_enable_reg(0x00000000);
	ime_set_interrupt_status_reg(IME_INTS_ALL);
	ime_set_init_reg();
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ime_ctrl_flow_to_do == ENABLE) {
		//ime_set_init();
		ime_set_interrupt_enable_reg(0x00000000);
		ime_set_interrupt_status_reg(IME_INTS_ALL);
		ime_set_init_reg();
	}
#else
	//ime_set_init();
	ime_set_interrupt_enable_reg(0x00000000);
	ime_set_interrupt_status_reg(IME_INTS_ALL);
	ime_set_init_reg();
#endif



	//memset((VOID *)&ime_stripe_info, 0, sizeof(IME_HV_STRIPE_INFO));
	ime_stripe_info = p_eng_info->set_stripe;
	memset((void *)ime_out_buf_flush, 0x0, sizeof(IME_OUTPUT_BUF_INFO)*IME_OUT_BUF_MAX);

	//-------------------------------------------------------------
	// set operating mode
	er_return = E_OK;
	ime_engine_operation_mode = p_eng_info->operation_mode;
	switch (p_eng_info->operation_mode) {
	case IME_OPMODE_D2D:
		//ime_set_in_path_source(IME_INSRC_DRAM);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_IME);

		ime_set_input_source_reg(IME_INSRC_DRAM);
		ime_set_direct_path_control_reg(SRC_CTRL_IME);
		break;

	case IME_OPMODE_IFE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_DCE);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_DCE);
		break;

	case IME_OPMODE_DCE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_DCE);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_DCE);
		break;

	case IME_OPMODE_SIE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_IME);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_IME);
		break;

	default:
		DBG_ERR("IME: engine mode error...\r\n");
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// output path enable
	//ime_set_out_path_p1_enable(p_eng_info->out_path1.out_path_enable);
	//ime_set_out_path_p2_enable(p_eng_info->out_path2.out_path_enable);
	//ime_set_out_path_p3_enable(p_eng_info->out_path3.out_path_enable);
	//ime_set_out_path_p4_enable(p_eng_info->out_path4.out_path_enable);
	//ime_set_out_path_p5_enable(p_eng_info->out_path5.out_path_enable);

	ime_open_output_p1_reg((UINT32)p_eng_info->out_path1.out_path_enable);
	ime_open_output_p2_reg((UINT32)p_eng_info->out_path2.out_path_enable);
	ime_open_output_p3_reg((UINT32)p_eng_info->out_path3.out_path_enable);

	ime_set_output_p4_enable_reg((UINT32)p_eng_info->out_path4.out_path_enable);
	// #2015/06/12# adas control flow bug
	if (p_eng_info->out_path4.out_path_enable == IME_FUNC_DISABLE) {
		//ime_set_adas_enable(IME_FUNC_DISABLE);
		ime_set_adas_enable_reg(IME_FUNC_DISABLE);
	}
	// #2015/06/12# end

	er_return = ime_chg_single_output(&(p_eng_info->single_out));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ime_chg_break_point(&(p_eng_info->break_point));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ime_chg_low_delay(&(p_eng_info->low_delay));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set function enable
	if (p_eng_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// set chroma median filter enable
#if 0
		if (p_eng_info->p_ime_iq_info->pCmfInfo != NULL) {
			er_return = ime_chg_chroma_median_filter_enable(*(p_eng_info->p_ime_iq_info->pCmfInfo));
			if (er_return != E_OK) {
				return er_return;
			}
		}
#endif

		//-------------------------------------------------------------
		// set LCA enable
		if (p_eng_info->p_ime_iq_info->p_lca_info != NULL) {
			er_return = ime_chg_lca_enable(p_eng_info->p_ime_iq_info->p_lca_info->lca_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			if (p_eng_info->p_ime_iq_info->p_lca_info->lca_enable == IME_FUNC_ENABLE) {
                ime_set_lca_image_size_reg(p_eng_info->p_ime_iq_info->p_lca_info->lca_image_info.lca_img_size.size_h, p_eng_info->p_ime_iq_info->p_lca_info->lca_image_info.lca_img_size.size_v);
			}
		}

		//-------------------------------------------------------------
		// set LCA subout enable
		if (p_eng_info->p_ime_iq_info->p_lca_subout_info != NULL) {
			er_return = ime_chg_lca_subout_enable(p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			if (p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_enable == IME_FUNC_ENABLE) {
                ime_set_lca_image_size_reg(p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_scale_info.ref_img_size.size_h, p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_scale_info.ref_img_size.size_v);
			}
		}

		//-------------------------------------------------------------
		// set DBCS enable
		if (p_eng_info->p_ime_iq_info->p_dbcs_info != NULL) {
			er_return = ime_chg_dark_bright_chroma_suppress_enable(p_eng_info->p_ime_iq_info->p_dbcs_info->dbcs_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set SSR enable
#if 0
		if (p_eng_info->p_ime_iq_info->pSsrInfo != NULL) {
#if (defined(_NVT_EMULATION_) == OFF)
			{
				g_SsrMode = p_eng_info->p_ime_iq_info->pSsrInfo->SsrMode;
				if (p_eng_info->p_ime_iq_info->pSsrInfo->SsrMode == IME_SSR_MODE_USER)
				{
					er_return = ime_chg_single_super_resolution_enable(p_eng_info->p_ime_iq_info->pSsrInfo->SsrEnable);
					if (er_return != E_OK) {
						return er_return;
					}
				}
			}
#else
			{
				er_return = ime_chg_single_super_resolution_enable(p_eng_info->p_ime_iq_info->pSsrInfo->SsrEnable);
				if (er_return != E_OK)
				{
					return er_return;
				}
			}
#endif
		}
#endif

		//-------------------------------------------------------------
		// set YUV Converter
		if (p_eng_info->p_ime_iq_info->p_yuv_cvt_info != NULL) {
			er_return = ime_chg_yuv_converter_enable(p_eng_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set CST
#if 0 // 520 do not support
		if (p_eng_info->p_ime_iq_info->pColorSpaceTrans != NULL) {
			er_return = ime_chgColorSpaceTransformEnable((p_eng_info->p_ime_iq_info->pColorSpaceTrans));
			if (er_return != E_OK) {
				return er_return;
			}
		}
#endif

		//-------------------------------------------------------------
		// set grain noise
#if 0 // 520 do not support
		if (p_eng_info->p_ime_iq_info->p_film_grain_info != NULL) {
			er_return = ime_chg_film_grain_noise_enable(p_eng_info->p_ime_iq_info->p_film_grain_info->FgnEnable);
			if (er_return != E_OK) {
				return er_return;
			}
		}
#endif

		//-------------------------------------------------------------
		// set P2I
#if 0
		if (p_eng_info->p_ime_iq_info->pP2IInfo != NULL) {
			er_return = ime_chg_progressive_to_interlace_enable(*(p_eng_info->p_ime_iq_info->pP2IInfo));
			if (er_return != E_OK) {
				return er_return;
			}
		}
#endif

		//-------------------------------------------------------------
		// set data stamp enable
		if (p_eng_info->p_ime_iq_info->p_data_stamp_info != NULL) {
			// set0
			er_return = ime_chg_data_stamp_enable(IME_DS_SET0, p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set1
			er_return = ime_chg_data_stamp_enable(IME_DS_SET1, p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set2
			er_return = ime_chg_data_stamp_enable(IME_DS_SET2, p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set3
			er_return = ime_chg_data_stamp_enable(IME_DS_SET3, p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set CST enable
			er_return = ime_chg_data_stamp_cst_enable(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_cst_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// privacy mask
		if (p_eng_info->p_ime_iq_info->p_pm_info != NULL) {
			er_return = ime_chg_privacy_mask_enable(IME_PM_SET0, p_eng_info->p_ime_iq_info->p_pm_info->pm_set0.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET1, p_eng_info->p_ime_iq_info->p_pm_info->pm_set1.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET2, p_eng_info->p_ime_iq_info->p_pm_info->pm_set2.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET3, p_eng_info->p_ime_iq_info->p_pm_info->pm_set3.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET4, p_eng_info->p_ime_iq_info->p_pm_info->pm_set4.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET5, p_eng_info->p_ime_iq_info->p_pm_info->pm_set5.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET6, p_eng_info->p_ime_iq_info->p_pm_info->pm_set6.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET7, p_eng_info->p_ime_iq_info->p_pm_info->pm_set7.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}


		//-------------------------------------------------------------
		// adas enable
		if (p_eng_info->p_ime_iq_info->p_sta_info != NULL) {
			er_return = ime_chg_statistic_enable(p_eng_info->p_ime_iq_info->p_sta_info->stl_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// TMNR
		if (p_eng_info->p_ime_iq_info->p_tmnr_info != NULL) {
			er_return = ime_chg_tmnr_enable(p_eng_info->p_ime_iq_info->p_tmnr_info->tmnr_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_refin_decoder_enable(p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_dec_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_refin_flip_enable(p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_flip_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_motion_sta_roi_enable(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_motion_sta_roi_flip_enable(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_flip_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_statistic_out_enable(p_eng_info->p_ime_iq_info->p_tmnr_info->sta_param.sta_out_en);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// TMNR Reference output
		if (p_eng_info->p_ime_iq_info->p_tmnr_refout_info != NULL) {
			er_return = ime_chg_tmnr_ref_out_enable(p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_ref_out_encoder_enable(p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_enc_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_ref_out_flip_enable(p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_flip_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			ime_comps_tmnr_out_ref_encoder_shift_mode_enable_reg((UINT32)p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_enc_smode_enable);
		}

	}


	//-------------------------------------------------------------
	// set input path
	//memset((VOID *)&ime_set_input_path_info, 0, sizeof(ime_set_input_path_info));
	//ime_set_input_path_info = p_eng_info->in_path_info;
	//if (ime_set_input_path_info.in_format == IME_INPUT_RGB) {
	//  ime_set_input_path_info.in_format = IME_INPUT_YCC_444;
	//}
	//er_return = ime_chg_input_path_param(&ime_set_input_path_info);
	er_return = ime_chg_input_path_param(&(p_eng_info->in_path_info));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path1
	er_return = ime_chg_output_path_param(IME_PATH1_SEL, &(p_eng_info->out_path1));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path2
	er_return = ime_chg_output_path_param(IME_PATH2_SEL, &(p_eng_info->out_path2));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path3
	er_return = ime_chg_output_path_param(IME_PATH3_SEL, &(p_eng_info->out_path3));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path4
	er_return = ime_chg_output_path_param(IME_PATH4_SEL, &(p_eng_info->out_path4));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path5
#if 0
	er_return = ime_chg_output_path_param(IME_PATH5_SEL, &(p_eng_info->out_path5));
	if (er_return != E_OK) {
		return er_return;
	}
#endif

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_eng_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// set LCA
		if (p_eng_info->p_ime_iq_info->p_lca_info != NULL) {
			if (p_eng_info->p_ime_iq_info->p_lca_info->lca_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_chroma_adapt_image_param(&(p_eng_info->p_ime_iq_info->p_lca_info->lca_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_chroma_adjust_param(&(p_eng_info->p_ime_iq_info->p_lca_info->lca_chroma_adj_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_param(&(p_eng_info->p_ime_iq_info->p_lca_info->lca_iq_chroma_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_luma_suppress_param(&(p_eng_info->p_ime_iq_info->p_lca_info->lca_iq_luma_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_bypass_enable(p_eng_info->p_ime_iq_info->p_lca_info->lca_bypass_enable);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set LCA subout
		if (p_eng_info->p_ime_iq_info->p_lca_subout_info != NULL) {
			if (p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_chroma_adapt_subout_source(p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_src);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_subout_param(&(p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_scale_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_subout_lineoffset_info(&(p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_lofs));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_subout_dma_addr_info(&(p_eng_info->p_ime_iq_info->p_lca_subout_info->lca_subout_addr));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set DBCS
		if (p_eng_info->p_ime_iq_info->p_dbcs_info != NULL) {
			if (p_eng_info->p_ime_iq_info->p_dbcs_info->dbcs_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_dark_bright_chroma_suppress_param(&(p_eng_info->p_ime_iq_info->p_dbcs_info->dbcs_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set YUV Converter
		if (p_eng_info->p_ime_iq_info->p_yuv_cvt_info != NULL) {
			if (p_eng_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_yuv_converter_param(p_eng_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_sel);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set grain noise
#if 0 // 520 do not support
		if (p_eng_info->p_ime_iq_info->p_film_grain_info != NULL) {
			if (p_eng_info->p_ime_iq_info->p_film_grain_info->FgnEnable == IME_FUNC_ENABLE) {
				er_return = ime_chg_film_grain_noise_param(&(p_eng_info->p_ime_iq_info->p_film_grain_info->FgnIqInfo));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}
#endif

		//-------------------------------------------------------------
		// set data stamp
		if (p_eng_info->p_ime_iq_info->p_data_stamp_info != NULL) {
			UINT32 plt_en = 0;

			if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_cst_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_color_coefs_param(&(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_cst_coef));
				if (er_return != E_OK) {
					return er_return;
				}
			}

			// set0
			if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET0, &(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_data_stamp_param(IME_DS_SET0, &(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_iq_info.ds_plt_enable == IME_FUNC_ENABLE) {
					plt_en += 1;
				}
			}

			// set1
			if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET1, &(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_data_stamp_param(IME_DS_SET1, &(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_iq_info.ds_plt_enable == IME_FUNC_ENABLE) {
					plt_en += 1;
				}
			}

			// set2
			if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET2, &(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_data_stamp_param(IME_DS_SET2, &(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_iq_info.ds_plt_enable == IME_FUNC_ENABLE) {
					plt_en += 1;
				}
			}

			// set3
			if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET3, &(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_data_stamp_param(IME_DS_SET3, &(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_iq_info.ds_plt_enable == IME_FUNC_ENABLE) {
					plt_en += 1;
				}
			}

			if (plt_en != 0) {
				er_return = ime_chg_data_stamp_color_palette_info(&(p_eng_info->p_ime_iq_info->p_data_stamp_info->ds_plt));
				if (er_return != E_OK) {
					return er_return;
				}
			}

		}

		//-------------------------------------------------------------
		// privacy mask
		if (p_eng_info->p_ime_iq_info->p_pm_info != NULL) {
			UINT32 get_pm_enable = 0x0, get_pm_pxl_use = 0x0;

			if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set0.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET0, &(p_eng_info->p_ime_iq_info->p_pm_info->pm_set0));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set0.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set1.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET1, &(p_eng_info->p_ime_iq_info->p_pm_info->pm_set1));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set1.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set2.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET2, &(p_eng_info->p_ime_iq_info->p_pm_info->pm_set2));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set2.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set3.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET3, &(p_eng_info->p_ime_iq_info->p_pm_info->pm_set3));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set3.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set4.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET4, &(p_eng_info->p_ime_iq_info->p_pm_info->pm_set4));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set4.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set5.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET5, &(p_eng_info->p_ime_iq_info->p_pm_info->pm_set5));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set5.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set6.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET6, &(p_eng_info->p_ime_iq_info->p_pm_info->pm_set6));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set6.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set7.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET7, &(p_eng_info->p_ime_iq_info->p_pm_info->pm_set7));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_eng_info->p_ime_iq_info->p_pm_info->pm_set7.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if ((get_pm_enable != 0) && (get_pm_pxl_use != 0)) {
				er_return = ime_chg_privacy_mask_pixelation_image_size(&(p_eng_info->p_ime_iq_info->p_pm_info->pm_pxl_img_size));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_privacy_mask_pixelation_block_size(&(p_eng_info->p_ime_iq_info->p_pm_info->pm_pxl_blk_size));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_privacy_maask_pixelation_image_lineoffset(&(p_eng_info->p_ime_iq_info->p_pm_info->pm_pxl_lofs));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_privacy_maask_pixelation_image_dma_addr(&(p_eng_info->p_ime_iq_info->p_pm_info->pm_pxl_dma_addr));
				if (er_return != E_OK) {
					return er_return;
				}
			}

		}

		//-------------------------------------------------------------
		// adas
		if (p_eng_info->p_ime_iq_info->p_sta_info != NULL) {
			if (p_eng_info->p_ime_iq_info->p_sta_info->stl_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_statistic_filter_enable(p_eng_info->p_ime_iq_info->p_sta_info->stl_filter_enable);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistical_image_output_type(p_eng_info->p_ime_iq_info->p_sta_info->stl_img_out_type);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_kernel_param(&(p_eng_info->p_ime_iq_info->p_sta_info->stl_edge_map));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_param(&(p_eng_info->p_ime_iq_info->p_sta_info->stl_hist0));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_param(&(p_eng_info->p_ime_iq_info->p_sta_info->stl_hist1));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_roi_param(&(p_eng_info->p_ime_iq_info->p_sta_info->stl_roi));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_map_lineoffset(p_eng_info->p_ime_iq_info->p_sta_info->stl_edge_map_lofs);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_map_addr(p_eng_info->p_ime_iq_info->p_sta_info->stl_edge_map_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_addr(p_eng_info->p_ime_iq_info->p_sta_info->stl_hist_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_flip_enable(p_eng_info->p_ime_iq_info->p_sta_info->stl_edge_map_flip_enable);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// TMNR
#if 1
		if (p_eng_info->p_ime_iq_info->p_tmnr_info != NULL) {
			if (p_eng_info->p_ime_iq_info->p_tmnr_info->tmnr_en == IME_FUNC_ENABLE) {

				UINT32 cal_a = 0, cal_b = 0;
				UINT32 flush_size = 0, flush_addr = 0;

				er_return = ime_chg_tmnr_motion_estimation_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->me_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_detection_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->md_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_detection_roi_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->md_roi_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_compensation_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->mc_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_compensation_roi_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->mc_roi_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_patch_selection_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->ps_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_noise_filter_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->nr_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_debug_ctrl_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->dbg_ctrl));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_ref_in_lineoffset(&(p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_ofs));
				if (er_return != E_OK) {
					return er_return;
				}

                if (ime_tmnr_in_ref_va2pa_flag == TRUE) {
    				flush_size = p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_ofs.lineoffset_y * p_eng_info->in_path_info.in_size.size_v;
    				flush_size = IME_ALIGN_CEIL_32(flush_size);
    				flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_y;
    				flush_addr = ime_input_dma_buf_flush(flush_addr, flush_size, ime_engine_operation_mode);
    				er_return = ime_chg_tmnr_ref_in_y_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_y);
    				if (er_return != E_OK) {
    					return er_return;
    				}

    				flush_size = p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_ofs.lineoffset_y * (p_eng_info->in_path_info.in_size.size_v >> 1);
    				flush_size = IME_ALIGN_CEIL_32(flush_size);
    				flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_cb;
    				flush_addr = ime_input_dma_buf_flush(flush_addr, flush_size, ime_engine_operation_mode);
    				er_return = ime_chg_tmnr_ref_in_uv_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_cb);
    				if (er_return != E_OK) {
    					return er_return;
    				}
				} else {
                    er_return = ime_chg_tmnr_ref_in_y_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_y);
    				if (er_return != E_OK) {
    					return er_return;
    				}

    				er_return = ime_chg_tmnr_ref_in_uv_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_cb);
    				if (er_return != E_OK) {
    					return er_return;
    				}
				}

				er_return = ime_chg_tmnr_motion_status_lineoffset(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_ofs);
				if (er_return != E_OK) {
					return er_return;
				}

				cal_a = (p_eng_info->in_path_info.in_size.size_v >> 1);
				cal_b = (cal_a >> 2);
				if (cal_a - (cal_b << 2) != 0) {
					cal_b++;
				}
				flush_size = cal_b * p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_ofs;
				flush_size = IME_ALIGN_CEIL_32(flush_size);
				flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_in_addr;
				flush_addr = ime_input_dma_buf_flush(flush_addr, flush_size, ime_engine_operation_mode);
				er_return = ime_chg_tmnr_motion_status_in_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_in_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				cal_a = (p_eng_info->in_path_info.in_size.size_v >> 1);
				cal_b = (cal_a >> 2);
				if (cal_a - (cal_b << 2) != 0) {
					cal_b++;
				}
				flush_size = cal_b * p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_ofs;
				flush_size = IME_ALIGN_CEIL_32(flush_size);
				flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_out_addr;
				flush_addr = ime_output_dma_buf_flush(flush_addr, flush_size, FALSE, IME_DO_BUF_FLUSH, ime_engine_operation_mode);
				er_return = ime_chg_tmnr_motion_status_out_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_out_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				if (ime_engine_operation_mode == IME_OPMODE_D2D) {
					ime_out_buf_flush[IME_OUT_TMNR_MS].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_TMNR_MS].buf_addr = flush_addr;
					ime_out_buf_flush[IME_OUT_TMNR_MS].buf_size = flush_size;
				}

				if (p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_en == IME_FUNC_ENABLE) {
					er_return = ime_chg_tmnr_motion_status_roi_out_lineoffset(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_ofs);
					if (er_return != E_OK) {
						return er_return;
					}

					cal_a = (p_eng_info->in_path_info.in_size.size_v >> 1);
					cal_b = (cal_a >> 2);
					if (cal_a - (cal_b << 2) != 0) {
						cal_b++;
					}
					flush_size = cal_b * p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_ofs;
					flush_size = IME_ALIGN_CEIL_32(flush_size);
					flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_addr;
					flush_addr = ime_output_dma_buf_flush(flush_addr, flush_size, FALSE, IME_DO_BUF_FLUSH, ime_engine_operation_mode);
					er_return = ime_chg_tmnr_motion_status_roi_out_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_addr);
					if (er_return != E_OK) {
						return er_return;
					}

					if (ime_engine_operation_mode == IME_OPMODE_D2D) {
						ime_out_buf_flush[IME_OUT_TMNR_MSROI].flush_en = ENABLE;
						ime_out_buf_flush[IME_OUT_TMNR_MSROI].buf_addr = flush_addr;
						ime_out_buf_flush[IME_OUT_TMNR_MSROI].buf_size = flush_size;
					}
				}

				er_return = ime_chg_tmnr_motion_vector_lineoffset(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_vec_ofs);
				if (er_return != E_OK) {
					return er_return;
				}

				flush_size = ((p_eng_info->in_path_info.in_size.size_v >> 2) - 1) * p_eng_info->p_ime_iq_info->p_tmnr_info->mot_vec_ofs;
				flush_size = IME_ALIGN_CEIL_32(flush_size);
				flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_info->mot_vec_in_addr;
				flush_addr = ime_input_dma_buf_flush(flush_addr, flush_size, ime_engine_operation_mode);
				er_return = ime_chg_tmnr_motion_vector_in_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_vec_in_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				flush_size = ((p_eng_info->in_path_info.in_size.size_v >> 2) - 1) * p_eng_info->p_ime_iq_info->p_tmnr_info->mot_vec_ofs;
				flush_size = IME_ALIGN_CEIL_32(flush_size);
				flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_info->mot_vec_out_addr;
				flush_addr = ime_output_dma_buf_flush(flush_addr, flush_size, FALSE, IME_DO_BUF_FLUSH, ime_engine_operation_mode);
				er_return = ime_chg_tmnr_motion_vector_out_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->mot_vec_out_addr);
				if (er_return != E_OK) {
					return er_return;
				}
				if (ime_engine_operation_mode == IME_OPMODE_D2D) {
					ime_out_buf_flush[IME_OUT_TMNR_MV].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_TMNR_MV].buf_addr = flush_addr;
					ime_out_buf_flush[IME_OUT_TMNR_MV].buf_size = flush_size;
				}

				if (p_eng_info->p_ime_iq_info->p_tmnr_info->sta_param.sta_out_en == IME_FUNC_ENABLE) {

					er_return = ime_chg_tmnr_statistic_data_output_param(&(p_eng_info->p_ime_iq_info->p_tmnr_info->sta_param));
					if (er_return != E_OK) {
						return er_return;
					}

					er_return = ime_chg_tmnr_statistic_out_lineoffset(p_eng_info->p_ime_iq_info->p_tmnr_info->sta_out_ofs);
					if (er_return != E_OK) {
						return er_return;
					}

					flush_size = (p_eng_info->p_ime_iq_info->p_tmnr_info->sta_param.sample_num_y * p_eng_info->p_ime_iq_info->p_tmnr_info->sta_out_ofs);
					flush_size = IME_ALIGN_CEIL_32(flush_size);
					flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_info->sta_out_addr;
					flush_addr = ime_output_dma_buf_flush(flush_addr, flush_size, FALSE, IME_DO_BUF_FLUSH, ime_engine_operation_mode);
					er_return = ime_chg_tmnr_statistic_out_addr(p_eng_info->p_ime_iq_info->p_tmnr_info->sta_out_addr);
					if (er_return != E_OK) {
						return er_return;
					}

					if (ime_engine_operation_mode == IME_OPMODE_D2D) {
						ime_out_buf_flush[IME_OUT_TMNR_STA].flush_en = ENABLE;
						ime_out_buf_flush[IME_OUT_TMNR_STA].buf_addr = flush_addr;
						ime_out_buf_flush[IME_OUT_TMNR_STA].buf_size = flush_size;
					}
				}

			}
		}
#endif

		//-------------------------------------------------------------
		// TMNR reference output
		if (p_eng_info->p_ime_iq_info->p_tmnr_refout_info != NULL) {
			if (p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_enable == IME_FUNC_ENABLE) {

				UINT32 flush_size = 0, flush_addr = 0;

				IME_BUF_FLUSH_SEL get_flush_status = IME_NOTDO_BUF_FLUSH;

				er_return = ime_chg_tmnr_ref_out_lineoffset(&(p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_lofs));
				if (er_return != E_OK) {
					return er_return;
				}

				get_flush_status = p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_dma_flush;

				flush_size = p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_lofs.lineoffset_y * (p_eng_info->in_path_info.in_size.size_v >> 1);
				flush_size = IME_ALIGN_CEIL_32(flush_size);
				flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_addr.addr_y;								
				flush_addr = ime_output_dma_buf_flush(flush_addr, flush_size, FALSE, get_flush_status, ime_engine_operation_mode);
				er_return = ime_chg_tmnr_ref_out_y_addr(p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_addr.addr_y);
				if (er_return != E_OK) {
					return er_return;
				}
				if (ime_engine_operation_mode == IME_OPMODE_D2D) {
					ime_out_buf_flush[IME_OUT_TMNR_REF_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_TMNR_REF_Y].buf_addr = flush_addr;
					ime_out_buf_flush[IME_OUT_TMNR_REF_Y].buf_size = flush_size;
				}

				flush_size = p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_lofs.lineoffset_cb * (p_eng_info->in_path_info.in_size.size_v >> 1);
				flush_size = IME_ALIGN_CEIL_32(flush_size);
				flush_addr = p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_addr.addr_cb;								
				flush_addr = ime_output_dma_buf_flush(flush_addr, flush_size, FALSE, get_flush_status, ime_engine_operation_mode);
				er_return = ime_chg_tmnr_ref_out_uv_addr(p_eng_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_addr.addr_cb);
				if (er_return != E_OK) {
					return er_return;
				}
				if (ime_engine_operation_mode == IME_OPMODE_D2D) {
					ime_out_buf_flush[IME_OUT_TMNR_REF_U].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_TMNR_REF_U].buf_addr = flush_addr;
					ime_out_buf_flush[IME_OUT_TMNR_REF_U].buf_size = flush_size;
				}

			}
		}


		//-------------------------------------------------------------
		// compress parameter
		if (p_eng_info->p_ime_iq_info->p_comp_info != NULL) {
			er_return = ime_chg_compression_param(p_eng_info->p_ime_iq_info->p_comp_info);
			if (er_return != E_OK) {
				return er_return;
			}

		}
	}


	//---------------------------------------------------------------------------------------------
	// set input and output image size
	//er_return = ime_set_in_path_image_size(&(p_eng_info->in_path_info.in_size));
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	if ((p_eng_info->in_path_info.in_size.size_h & 0x3) != 0) {
		DBG_ERR("IME: input, image size error, horizontal size is not 4x...\r\n");

		return E_PAR;
	}

	if (p_eng_info->in_path_info.in_size.size_h < 8) {
		//
		DBG_WRN("IME: input, image width less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_eng_info->in_path_info.in_size.size_v < 8) {
		//
		DBG_WRN("IME: input, image height less than (<) 8 is risky, please check!!!\r\n");
	}

	ime_set_input_image_size_reg(p_eng_info->in_path_info.in_size.size_h, p_eng_info->in_path_info.in_size.size_v);
	//---------------------------------------------------------------------------------------------

	if (p_eng_info->out_path1.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p1(p_eng_info->out_path1.out_path_scale_method);
		ime_set_scale_interpolation_method_p1_reg((UINT32)p_eng_info->out_path1.out_path_scale_method);

		//----------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p1(&(p_eng_info->out_path1.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_eng_info->out_path1.out_path_scale_size.size_h > 65535) || (p_eng_info->out_path1.out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path1, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_eng_info->out_path1.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path1, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_eng_info->out_path1.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path1, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p1_reg(p_eng_info->out_path1.out_path_scale_size.size_h, p_eng_info->out_path1.out_path_scale_size.size_v);

		//----------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p1(&(p_eng_info->out_path1.out_path_scale_size), &(p_eng_info->out_path1.out_path_out_size), &(p_eng_info->out_path1.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_eng_info->out_path1.out_path_out_size.size_h > 65535) || (p_eng_info->out_path1.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path1, horizontal or vertical crop size overflow...\r\n");

			return E_PAR;
		}

		if (p_eng_info->out_path1.out_path_out_size.size_h < 32) {
			//
			DBG_WRN("IME: path1, horizontal crop size less than (<) 32 is risky, please check!!!\r\n");
		}

		if (p_eng_info->out_path1.out_path_out_size.size_v < 32) {
			//
			DBG_WRN("IME: path1, vertical crop size less than (<) 32 is risky, please check!!!\r\n");
		}

		if (((p_eng_info->out_path1.out_path_crop_pos.pos_x + p_eng_info->out_path1.out_path_out_size.size_h) > p_eng_info->out_path1.out_path_scale_size.size_h) || ((p_eng_info->out_path1.out_path_crop_pos.pos_y + p_eng_info->out_path1.out_path_out_size.size_v) > p_eng_info->out_path1.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path1, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p1_reg(p_eng_info->out_path1.out_path_out_size.size_h, p_eng_info->out_path1.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p1_reg(p_eng_info->out_path1.out_path_crop_pos.pos_x, p_eng_info->out_path1.out_path_crop_pos.pos_y);
	}

	if (p_eng_info->out_path2.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p2(p_eng_info->out_path2.out_path_scale_method);
		ime_set_scale_interpolation_method_p2_reg((UINT32)p_eng_info->out_path2.out_path_scale_method);

		//-------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p2(&(p_eng_info->out_path2.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_eng_info->out_path2.out_path_scale_size.size_h > 65535) || (p_eng_info->out_path2.out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path2, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_eng_info->out_path2.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path2, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_eng_info->out_path2.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path2, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p2_reg(p_eng_info->out_path2.out_path_scale_size.size_h, p_eng_info->out_path2.out_path_scale_size.size_v);

		//-------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p2(&(p_eng_info->out_path2.out_path_scale_size), &(p_eng_info->out_path2.out_path_out_size), &(p_eng_info->out_path2.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_eng_info->out_path2.out_path_out_size.size_h > 65535) || (p_eng_info->out_path2.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path2, horizontal or vertical crop size is overflow...\r\n");

			return E_PAR;
		}

		if (p_eng_info->out_path2.out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path2, horizontal crop size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_eng_info->out_path2.out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path2, vertical crop size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_eng_info->out_path2.out_path_crop_pos.pos_x + p_eng_info->out_path2.out_path_out_size.size_h) > p_eng_info->out_path2.out_path_scale_size.size_h) || ((p_eng_info->out_path2.out_path_crop_pos.pos_y + p_eng_info->out_path2.out_path_out_size.size_v) > p_eng_info->out_path2.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path2, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p2_reg(p_eng_info->out_path2.out_path_out_size.size_h, p_eng_info->out_path2.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p2_reg(p_eng_info->out_path2.out_path_crop_pos.pos_x, p_eng_info->out_path2.out_path_crop_pos.pos_y);
	}

	if (p_eng_info->out_path3.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p3(p_eng_info->out_path3.out_path_scale_method);
		ime_set_scale_interpolation_method_p3_reg((UINT32)p_eng_info->out_path3.out_path_scale_method);

		//-------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p3(&(p_eng_info->out_path3.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_eng_info->out_path3.out_path_scale_size.size_h > 65535) || (p_eng_info->out_path3.out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path3, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_eng_info->out_path3.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path3, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_eng_info->out_path3.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path3, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p3_reg(p_eng_info->out_path3.out_path_scale_size.size_h, p_eng_info->out_path3.out_path_scale_size.size_v);

		//-------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p3(&(p_eng_info->out_path3.out_path_scale_size), &(p_eng_info->out_path3.out_path_out_size), &(p_eng_info->out_path3.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_eng_info->out_path3.out_path_out_size.size_h > 65535) || (p_eng_info->out_path3.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path3, output size overflow...\r\n");

			return E_PAR;
		}

		if (p_eng_info->out_path3.out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path3, horizontal output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_eng_info->out_path3.out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path3, vertical output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_eng_info->out_path3.out_path_crop_pos.pos_x + p_eng_info->out_path3.out_path_out_size.size_h) > p_eng_info->out_path3.out_path_scale_size.size_h) || ((p_eng_info->out_path3.out_path_crop_pos.pos_y + p_eng_info->out_path3.out_path_out_size.size_v) > p_eng_info->out_path3.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path3, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p3_reg(p_eng_info->out_path3.out_path_out_size.size_h, p_eng_info->out_path3.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p3_reg(p_eng_info->out_path3.out_path_crop_pos.pos_x, p_eng_info->out_path3.out_path_crop_pos.pos_y);
	}


	if (p_eng_info->out_path4.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p4(p_eng_info->out_path4.out_path_scale_method);
		ime_set_scale_interpolation_method_p4_reg((UINT32)p_eng_info->out_path4.out_path_scale_method);

		//-----------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p4(&(p_eng_info->out_path4.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_eng_info->out_path4.out_path_scale_size.size_h > 65535) || (p_eng_info->out_path4.out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path4, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_eng_info->out_path4.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path4, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_eng_info->out_path4.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path4, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p4_reg(p_eng_info->out_path4.out_path_scale_size.size_h, p_eng_info->out_path4.out_path_scale_size.size_v);

		//-----------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p4(&(p_eng_info->out_path4.out_path_scale_size), &(p_eng_info->out_path4.out_path_out_size), &(p_eng_info->out_path4.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_eng_info->out_path4.out_path_out_size.size_h > 65535) || (p_eng_info->out_path4.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path4, horizontal or vertical crop size is overflow...\r\n");

			return E_PAR;
		}

		if (p_eng_info->out_path4.out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path4, horizontal output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_eng_info->out_path4.out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path4, vertical output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_eng_info->out_path4.out_path_crop_pos.pos_x + p_eng_info->out_path4.out_path_out_size.size_h) > p_eng_info->out_path4.out_path_scale_size.size_h) || ((p_eng_info->out_path4.out_path_crop_pos.pos_y + p_eng_info->out_path4.out_path_out_size.size_v) > p_eng_info->out_path4.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path4, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p4_reg(p_eng_info->out_path4.out_path_out_size.size_h, p_eng_info->out_path4.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p4_reg(p_eng_info->out_path4.out_path_crop_pos.pos_x, p_eng_info->out_path4.out_path_crop_pos.pos_y);
	}

#if 0
	if (p_eng_info->out_path5.out_path_enable == ENABLE) {
		ime_set_out_path_scale_method_p5(p_eng_info->out_path5.out_path_scale_method);

		er_return = ime_set_out_path_scale_size_p5(&(p_eng_info->out_path5.out_path_scale_size));
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_output_crop_size_p5(&(p_eng_info->out_path5.out_path_scale_size), &(p_eng_info->out_path5.out_path_out_size), &(p_eng_info->out_path5.out_path_crop_pos));
		if (er_return != E_OK) {
			return er_return;
		}
	}
#endif

	//-------------------------------------------------------------
	// set interrupt enable
	{
		UINT32 ime_interrupt_enable = 0;

		ime_interrupt_enable = p_eng_info->interrupt_enable;
		if (ime_interrupt_enable == 0x0) {
			ime_interrupt_enable = IME_INTE_ALL;
		}
		//ime_set_engine_control(IME_ENGINE_SET_INTPE, ime_interrupt_enable);
		ime_set_interrupt_enable_reg(ime_interrupt_enable);
	}

#if (defined(_NVT_EMULATION_) == OFF) // for real chip
	er_return = ime_chg_burst_length(&ime_set_burst_info_dram1);
	if (er_return != E_OK) {
		return er_return;
	}
#endif

	ime_engine_state_machine_status = IME_ENGINE_PAUSE;

	return er_return;

SKIP_PARAMS:  // for fastboot control flow
	ime_engine_state_machine_status = IME_ENGINE_PAUSE;

	return er_return;

#endif
}
//-------------------------------------------------------------------------------

#if 0
ER ime_chg_all_param(IME_MODE_PRAM *p_engine_info)
{

	ER er_return = E_OK;

	UINT32 ime_interrupt_enable = 0x0;


	if (p_engine_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_init();
	ime_set_interrupt_enable_reg(0x00000000);
	ime_set_interrupt_status_reg(IME_INTS_ALL);

	ime_set_init_reg();

	memset((VOID *)&ime_stripe_info, 0, sizeof(IME_HV_STRIPE_INFO));
	ime_stripe_info = p_engine_info->set_stripe;

	//-------------------------------------------------------------
	// set operating mode
	er_return = E_OK;
	ime_engine_operation_mode = p_engine_info->operation_mode;
	switch (p_engine_info->operation_mode) {
	case IME_OPMODE_D2D:
		//ime_set_in_path_source(IME_INSRC_DRAM);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_IME);

		ime_set_input_source_reg(IME_INSRC_DRAM);
		ime_set_direct_path_control_reg(SRC_CTRL_IME);
		break;

	case IME_OPMODE_IFE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_DCE);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_DCE);
		break;

	case IME_OPMODE_DCE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_DCE);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_DCE);
		break;

	case IME_OPMODE_SIE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_IME);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_IME);
		break;

	default:
		DBG_ERR("IME: engine mode error...\r\n");
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// output path enable
	//ime_set_out_path_p1_enable(p_engine_info->out_path1.out_path_enable);
	//ime_set_out_path_p2_enable(p_engine_info->out_path2.out_path_enable);
	//ime_set_out_path_p3_enable(p_engine_info->out_path3.out_path_enable);
	//ime_set_out_path_p4_enable(p_engine_info->out_path4.out_path_enable);

	ime_open_output_p1_reg((UINT32)p_engine_info->out_path1.out_path_enable);
	ime_open_output_p2_reg((UINT32)p_engine_info->out_path2.out_path_enable);
	ime_open_output_p3_reg((UINT32)p_engine_info->out_path3.out_path_enable);

	ime_set_output_p4_enable_reg((UINT32)p_engine_info->out_path4.out_path_enable);
	// #2015/06/12# adas control flow bug
	if (p_engine_info->out_path4.out_path_enable == IME_FUNC_DISABLE) {
		ime_set_adas_enable(IME_FUNC_DISABLE);
	}
	// #2015/06/12# end


	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// set chroma median filter enable
#if 0
		if (p_engine_info->p_ime_iq_info->pCmfInfo != NULL) {
			er_return = ime_chg_chroma_median_filter_enable(*(p_engine_info->p_ime_iq_info->pCmfInfo));
			if (er_return != E_OK) {
				return er_return;
			}
		}
#endif

		//-------------------------------------------------------------
		// set LCA enable
		if (p_engine_info->p_ime_iq_info->p_lca_info != NULL) {
			er_return = ime_chg_lca_enable(p_engine_info->p_ime_iq_info->p_lca_info->lca_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set LCA subout enable
		if (p_engine_info->p_ime_iq_info->p_lca_subout_info != NULL) {
			er_return = ime_chg_lca_subout_enable(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set DBCS enable
		if (p_engine_info->p_ime_iq_info->p_dbcs_info != NULL) {
			er_return = ime_chg_dark_bright_chroma_suppress_enable(p_engine_info->p_ime_iq_info->p_dbcs_info->dbcs_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set SSR enable
#if 0
		if (p_engine_info->p_ime_iq_info->pSsrInfo != NULL) {
			g_SsrMode = p_engine_info->p_ime_iq_info->pSsrInfo->SsrMode;
			if (p_engine_info->p_ime_iq_info->pSsrInfo->SsrMode == IME_SSR_MODE_USER) {
				er_return = ime_chg_single_super_resolution_enable(p_engine_info->p_ime_iq_info->pSsrInfo->SsrEnable);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}
#endif

		//-------------------------------------------------------------
		// set YUV Converter
		if (p_engine_info->p_ime_iq_info->p_yuv_cvt_info != NULL) {
			er_return = ime_chg_yuv_converter_enable(p_engine_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set CST
		if (p_engine_info->p_ime_iq_info->pColorSpaceTrans != NULL) {
			er_return = ime_chgColorSpaceTransformEnable((p_engine_info->p_ime_iq_info->pColorSpaceTrans));
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set grain noise
		if (p_engine_info->p_ime_iq_info->p_film_grain_info != NULL) {
			er_return = ime_chg_film_grain_noise_enable(p_engine_info->p_ime_iq_info->p_film_grain_info->FgnEnable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set P2I
#if 0
		if (p_engine_info->p_ime_iq_info->pP2IInfo != NULL) {
			er_return = ime_chg_progressive_to_interlace_enable(*(p_engine_info->p_ime_iq_info->pP2IInfo));
			if (er_return != E_OK) {
				return er_return;
			}
		}
#endif

		//-------------------------------------------------------------
		// set data stamp enable
		if (p_engine_info->p_ime_iq_info->p_data_stamp_info != NULL) {
			// set0
			er_return = ime_chg_data_stamp_enable(IME_DS_SET0, p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set1
			er_return = ime_chg_data_stamp_enable(IME_DS_SET1, p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set2
			er_return = ime_chg_data_stamp_enable(IME_DS_SET2, p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set3
			er_return = ime_chg_data_stamp_enable(IME_DS_SET3, p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set CST enable
			er_return = ime_chg_data_stamp_cst_enable(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_cst_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// adas enable
		if (p_engine_info->p_ime_iq_info->p_sta_info != NULL) {
			er_return = ime_chg_statistic_enable(p_engine_info->p_ime_iq_info->p_sta_info->stl_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

	}


	//-------------------------------------------------------------
	// set input path
	memset((VOID *)&ime_set_input_path_info, 0, sizeof(ime_set_input_path_info));
	ime_set_input_path_info = p_engine_info->in_path_info;
	//if (ime_set_input_path_info.in_format == IME_INPUT_RGB) {
	//  ime_set_input_path_info.in_format = IME_INPUT_YCC_444;
	//}
	er_return = ime_chg_input_path_param(&ime_set_input_path_info);
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path1
	memset((VOID *)&ime_set_output_path1_info, 0, sizeof(ime_set_output_path1_info));
	ime_set_output_path1_info = p_engine_info->out_path1;
	er_return = ime_chg_output_path_param(IME_PATH1_SEL, &ime_set_output_path1_info);
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path2
	memset((VOID *)&ime_set_output_path2_info, 0, sizeof(ime_set_output_path2_info));
	ime_set_output_path2_info = p_engine_info->out_path2;
	er_return = ime_chg_output_path_param(IME_PATH2_SEL, &ime_set_output_path2_info);
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path3
	memset((VOID *)&ime_set_output_path3_info, 0, sizeof(ime_set_output_path3_info));
	ime_set_output_path3_info = p_engine_info->out_path3;
	er_return = ime_chg_output_path_param(IME_PATH3_SEL, &ime_set_output_path3_info);
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path4
	memset((VOID *)&ime_set_output_path4_info, 0, sizeof(ime_set_output_path4_info));
	ime_set_output_path4_info = p_engine_info->out_path4;
	er_return = ime_chg_output_path_param(IME_PATH4_SEL, &ime_set_output_path4_info);
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path5
#if 0
	//memset((VOID*)&ime_set_output_path5_info, 0, sizeof(ime_set_output_path5_info));
	ime_set_output_path5_info = p_engine_info->out_path5;
	er_return = ime_chg_output_path_param(IME_PATH5_SEL, &ime_set_output_path5_info);
	if (er_return != E_OK) {
		return er_return;
	}
#endif

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// set LCA
		if (p_engine_info->p_ime_iq_info->p_lca_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_lca_info->lca_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_chroma_adapt_image_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_chroma_adjust_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_chroma_adj_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_iq_chroma_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_luma_suppress_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_iq_luma_info));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set LCA subout
		if (p_engine_info->p_ime_iq_info->p_lca_subout_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_chroma_adapt_subout_param(&(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_scale_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_subout_lineoffset_info(&(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_lofs));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_subout_dma_addr_info(&(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_addr));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set DBCS
		if (p_engine_info->p_ime_iq_info->p_dbcs_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_dbcs_info->dbcs_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_dark_bright_chroma_suppress_param(&(p_engine_info->p_ime_iq_info->p_dbcs_info->dbcs_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set SSR
#if 0
		if (p_engine_info->p_ime_iq_info->pSsrInfo != NULL) {
			if (p_engine_info->p_ime_iq_info->pSsrInfo->SsrEnable == IME_FUNC_ENABLE) {
				er_return = ime_chg_single_super_resolution_param(&(p_engine_info->p_ime_iq_info->pSsrInfo->SsrIqInfo));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}
#endif

		//-------------------------------------------------------------
		// set YUV Converter
		if (p_engine_info->p_ime_iq_info->p_yuv_cvt_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_yuv_converter_param(p_engine_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_sel);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set grain noise
		if (p_engine_info->p_ime_iq_info->p_film_grain_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_film_grain_info->FgnEnable == IME_FUNC_ENABLE) {
				er_return = ime_chg_film_grain_noise_param(&(p_engine_info->p_ime_iq_info->p_film_grain_info->FgnIqInfo));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// set data stamp
		if (p_engine_info->p_ime_iq_info->p_data_stamp_info != NULL) {
			// set0
			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET0, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

#if 0
				er_return = ime_chg_data_stamp_param(IME_DS_SET0, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}
#endif
			}

			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_cst_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_color_coefs_param(&(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_cst_coef));
				if (er_return != E_OK) {
					return er_return;
				}
			}

#if 0
			// color index LUT, // remove from nt96680
			if ((p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_enable == IME_FUNC_ENABLE) || (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_enable == IME_FUNC_ENABLE) || (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_enable == IME_FUNC_ENABLE)) {
				er_return = ime_set_osd_color_lut_info(p_engine_info->p_ime_iq_info->p_data_stamp_info->puiColorLutY, p_engine_info->p_ime_iq_info->p_data_stamp_info->puiColorLutU, p_engine_info->p_ime_iq_info->p_data_stamp_info->puiColorLutV);
				if (er_return != E_OK) {
					return er_return;
				}
			}
#endif

			// set1
			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET1, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

#if 0
				er_return = ime_chg_data_stamp_param(IME_DS_SET1, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}
#endif
			}

			// set2
			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET2, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

#if 0
				er_return = ime_chg_data_stamp_param(IME_DS_SET2, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}
#endif
			}

			// set3
			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET3, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

#if 0
				er_return = ime_chg_data_stamp_param(IME_DS_SET3, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}
#endif
			}
		}

		//-------------------------------------------------------------
		// adas
		if (p_engine_info->p_ime_iq_info->p_sta_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_sta_info->stl_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_statistic_filter_enable(p_engine_info->p_ime_iq_info->p_sta_info->stl_filter_enable);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistical_image_output_type(p_engine_info->p_ime_iq_info->p_sta_info->stl_img_out_type);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_kernel_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist0));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist1));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_roi_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_roi));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_addr(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist_addr);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

	}



	//-------------------------------------------------------------
	// set input and output image size
	er_return = ime_set_in_path_image_size(&(p_engine_info->in_path_info.in_size));
	if (er_return != E_OK) {
		return er_return;
	}

	if (p_engine_info->out_path1.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p1(p_engine_info->out_path1.out_path_scale_method);
		ime_set_scale_interpolation_method_p1_reg((UINT32)p_engine_info->out_path1.out_path_scale_method);

		er_return = ime_set_out_path_scale_size_p1(&(p_engine_info->out_path1.out_path_scale_size));
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_output_crop_size_p1(&(p_engine_info->out_path1.out_path_scale_size), &(p_engine_info->out_path1.out_path_out_size), &(p_engine_info->out_path1.out_path_crop_pos));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	if (p_engine_info->out_path2.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p2(p_engine_info->out_path2.out_path_scale_method);
		ime_set_scale_interpolation_method_p2_reg((UINT32)p_engine_info->out_path2.out_path_scale_method);

		//--------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p2(&(p_engine_info->out_path2.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path2.out_path_scale_size.size_h > 65535) || (p_engine_info->out_path2.out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path2, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path2.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path2, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path2.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path2, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p2_reg(p_engine_info->out_path2.out_path_scale_size.size_h, p_engine_info->out_path2.out_path_scale_size.size_v);

		//--------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p2(&(p_engine_info->out_path2.out_path_scale_size), &(p_engine_info->out_path2.out_path_out_size), &(p_engine_info->out_path2.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path2.out_path_out_size.size_h > 65535) || (p_engine_info->out_path2.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path2, horizontal or vertical crop size is overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path2.out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path2, horizontal crop size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path2.out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path2, vertical crop size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_engine_info->out_path2.out_path_crop_pos.pos_x + p_engine_info->out_path2.out_path_out_size.size_h) > p_engine_info->out_path2.out_path_scale_size.size_h) || ((p_engine_info->out_path2.out_path_crop_pos.pos_y + p_engine_info->out_path2.out_path_out_size.size_v) > p_engine_info->out_path2.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path2, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p2_reg(p_engine_info->out_path2.out_path_out_size.size_h, p_engine_info->out_path2.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p2_reg(p_engine_info->out_path2.out_path_crop_pos.pos_x, p_engine_info->out_path2.out_path_crop_pos.pos_y);
	}

	if (p_engine_info->out_path3.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p3(p_engine_info->out_path3.out_path_scale_method);
		ime_set_scale_interpolation_method_p3_reg((UINT32)p_engine_info->out_path3.out_path_scale_method);

		er_return = ime_set_out_path_scale_size_p3(&(p_engine_info->out_path3.out_path_scale_size));
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_output_crop_size_p3(&(p_engine_info->out_path3.out_path_scale_size), &(p_engine_info->out_path3.out_path_out_size), &(p_engine_info->out_path3.out_path_crop_pos));
		if (er_return != E_OK) {
			return er_return;
		}
	}


	if (p_engine_info->out_path4.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p4(p_engine_info->out_path4.out_path_scale_method);
		ime_set_scale_interpolation_method_p4_reg((UINT32)p_engine_info->out_path4.out_path_scale_method);

		er_return = ime_set_out_path_scale_size_p4(&(p_engine_info->out_path4.out_path_scale_size));
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_output_crop_size_p4(&(p_engine_info->out_path4.out_path_scale_size), &(p_engine_info->out_path4.out_path_out_size), &(p_engine_info->out_path4.out_path_crop_pos));
		if (er_return != E_OK) {
			return er_return;
		}
	}

#if 0
	if (p_engine_info->out_path5.out_path_enable == ENABLE) {
		ime_set_out_path_scale_method_p5(p_engine_info->out_path5.out_path_scale_method);

		er_return = ime_set_out_path_scale_size_p5(&(p_engine_info->out_path5.out_path_scale_size));
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_output_crop_size_p5(&(p_engine_info->out_path5.out_path_scale_size), &(p_engine_info->out_path5.out_path_out_size), &(p_engine_info->out_path5.out_path_crop_pos));
		if (er_return != E_OK) {
			return er_return;
		}
	}
#endif

	//-------------------------------------------------------------
	// set interrupt enable
	ime_interrupt_enable = p_engine_info->interrupt_enable;
	if (ime_interrupt_enable == 0x0) {
		ime_interrupt_enable |= IME_INTE_FRM_END;
	}
	//ime_set_engine_control(IME_ENGINE_SET_INTPE, ime_interrupt_enable);
	ime_set_interrupt_enable_reg(ime_interrupt_enable);

	ime_load();

	return E_OK;
}
#endif

//-------------------------------------------------------------------------------



ER ime_chg_output_path_enable(IME_PATH_SEL path_sel, IME_FUNC_EN set_en)
{
	ER er_return = E_OK;


	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (path_sel) {
	case IME_PATH1_SEL:
		//ime_set_out_path_p1_enable(set_en);
		ime_open_output_p1_reg((UINT32)set_en);
		break;

	case IME_PATH2_SEL:
		//ime_set_out_path_p2_enable(set_en);
		ime_open_output_p2_reg((UINT32)set_en);
		break;

	case IME_PATH3_SEL:
		//ime_set_out_path_p3_enable(set_en);
		ime_open_output_p3_reg((UINT32)set_en);
		break;

	case IME_PATH4_SEL:
		//ime_set_out_path_p4_enable(set_en);
		ime_set_output_p4_enable_reg((UINT32)set_en);

		// #2015/06/12# adas control flow bug
		if (set_en == IME_FUNC_DISABLE) {
			//ime_set_adas_enable(IME_FUNC_DISABLE);
			ime_set_adas_enable_reg(IME_FUNC_DISABLE);
		}
		// #2015/06/12# end
		break;

	//case IME_PATH5_SEL:
	//  ime_set_out_path_p5_enable(set_en);
	//  ime_open_output_p5_reg((UINT32)set_en);
	//  break;

	default:
		break;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_output_path_dram_out_enable(IME_PATH_SEL path_sel, IME_FUNC_EN set_en)
{
	ER er_return = E_OK;


	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (path_sel) {
	case IME_PATH1_SEL:
		ime_set_output_dram_enable_p1_reg((UINT32)set_en);
		break;

	case IME_PATH2_SEL:
		ime_set_output_dram_enable_p2_reg((UINT32)set_en);
		break;

	case IME_PATH3_SEL:
		ime_set_output_dram_enable_p3_reg((UINT32)set_en);
		break;

	case IME_PATH4_SEL:
		ime_set_output_dram_enable_p4_reg((UINT32)set_en);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_out_path_dma_output_enable_p5(set_en);
	//break;

	default:
		break;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_output_scaling_enhance_param(IME_PATH_SEL path_sel, IME_SCALE_ENH_INFO *p_set_scl_enh)
{
	ER er_return = E_OK;

	if (p_set_scl_enh == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}


	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (path_sel) {
	case IME_PATH1_SEL:
		ime_set_scaling_enhance_factor_p1_reg(p_set_scl_enh->enh_factor, p_set_scl_enh->enh_bit);
		break;

	case IME_PATH2_SEL:
		ime_set_scaling_enhance_factor_p2_reg(p_set_scl_enh->enh_factor, p_set_scl_enh->enh_bit);
		break;

	case IME_PATH3_SEL:
		ime_set_scaling_enhance_factor_p3_reg(p_set_scl_enh->enh_factor, p_set_scl_enh->enh_bit);
		break;

	case IME_PATH4_SEL:
		ime_set_scaling_enhance_factor_p4_reg(p_set_scl_enh->enh_factor, p_set_scl_enh->enh_bit);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_out_path_enhance_p5(p_set_scl_enh);
	//break;

	default:
		break;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

#if 0
ER ime_chg_output_scaling_factor_param(IME_PATH_SEL path_sel, IME_SCALE_FACTOR_INFO *p_set_scl_factor)
{
	ER er_return = E_OK;

	if (p_set_scl_factor == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (path_sel) {
	case IME_PATH1_SEL:
		memset((void *)&ime_scale_factor_path1_params, 0x00000000, sizeof(IME_SCALE_FACTOR_INFO));
		if (p_set_scl_factor->CalScaleFactorMode == IME_SCALE_FACTOR_COEF_USER_MODE) {
			ime_scale_factor_path1_params = *p_set_scl_factor;
		}
		break;

	case IME_PATH2_SEL:
		memset((void *)&ime_scale_factor_path2_params, 0x00000000, sizeof(IME_SCALE_FACTOR_INFO));
		if (p_set_scl_factor->CalScaleFactorMode == IME_SCALE_FACTOR_COEF_USER_MODE) {
			ime_scale_factor_path2_params = *p_set_scl_factor;
		}
		break;

	case IME_PATH3_SEL:
		memset((void *)&ime_scale_factor_path3_params, 0x00000000, sizeof(IME_SCALE_FACTOR_INFO));
		if (p_set_scl_factor->CalScaleFactorMode == IME_SCALE_FACTOR_COEF_USER_MODE) {
			ime_scale_factor_path3_params = *p_set_scl_factor;
		}
		break;

	case IME_PATH4_SEL:
		memset((void *)&ime_scale_factor_path4_params, 0x00000000, sizeof(IME_SCALE_FACTOR_INFO));
		if (p_set_scl_factor->CalScaleFactorMode == IME_SCALE_FACTOR_COEF_USER_MODE) {
			ime_scale_factor_path4_params = *p_set_scl_factor;
		}
		break;

#if 0
	case IME_PATH5_SEL:
		memset((void *)&ime_scale_factor_path5_params, 0x00000000, sizeof(IME_SCALE_FACTOR_INFO));
		if (p_set_scl_factor->CalScaleFactorMode == IME_SCALE_FACTOR_COEF_USER_MODE) {
			ime_scale_factor_path5_params = *p_set_scl_factor;
		}
		break;
#endif

	default:
		break;
	}

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

ER ime_chg_output_scaling_filter_param(IME_PATH_SEL path_sel, IME_SCALE_FILTER_INFO *p_set_scl_filter)
{
	ER er_return = E_OK;

	if (p_set_scl_filter == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (path_sel) {
	case IME_PATH1_SEL:
		//ime_set_out_path_scale_filter_p1(p_set_scl_filter);

		if ((p_set_scl_filter->scale_h_filter_coef > 63) || (p_set_scl_filter->scale_v_filter_coef > 63)) {
			DBG_ERR("IME: path1, scale filter coef. overflow...\r\n");

			return E_PAR;
		}

		ime_set_horizontal_scale_filtering_p1_reg((UINT32)p_set_scl_filter->scale_h_filter_enable, p_set_scl_filter->scale_h_filter_coef);
		ime_set_vertical_scale_filtering_p1_reg((UINT32)p_set_scl_filter->scale_v_filter_enable, p_set_scl_filter->scale_v_filter_coef);
		break;

	case IME_PATH2_SEL:
		//ime_set_out_path_scale_filter_p2(p_set_scl_filter);

		if ((p_set_scl_filter->scale_h_filter_coef > 63) || (p_set_scl_filter->scale_v_filter_coef > 63)) {
			DBG_ERR("IME: path2, scale filter coef. overflow...\r\n");

			return E_PAR;
		}

		ime_set_horizontal_scale_filtering_p2_reg((UINT32)p_set_scl_filter->scale_h_filter_enable, p_set_scl_filter->scale_h_filter_coef);
		ime_set_vertical_scale_filtering_p2_reg((UINT32)p_set_scl_filter->scale_v_filter_enable, p_set_scl_filter->scale_v_filter_coef);
		break;

	case IME_PATH3_SEL:
		//ime_set_out_path_scale_filter_p3(p_set_scl_filter);

		if ((p_set_scl_filter->scale_h_filter_coef > 63) || (p_set_scl_filter->scale_v_filter_coef > 63)) {
			DBG_ERR("IME: path3, scale filter coef. overflow...\r\n");

			return E_PAR;
		}

		ime_set_horizontal_scale_filtering_p3_reg((UINT32)p_set_scl_filter->scale_h_filter_enable, p_set_scl_filter->scale_h_filter_coef);
		ime_set_vertical_scale_filtering_p3_reg((UINT32)p_set_scl_filter->scale_v_filter_enable, p_set_scl_filter->scale_v_filter_coef);
		break;

	case IME_PATH4_SEL:
		//ime_set_out_path_scale_filter_p4(p_set_scl_filter);

		if ((p_set_scl_filter->scale_h_filter_coef > 63) || (p_set_scl_filter->scale_v_filter_coef > 63)) {
			DBG_ERR("IME: path4, scale filter coef. overflow...\r\n");

			return E_PAR;
		}

		ime_set_horizontal_scale_filtering_p4_reg((UINT32)p_set_scl_filter->scale_h_filter_enable, p_set_scl_filter->scale_h_filter_coef);
		ime_set_vertical_scale_filtering_p4_reg((UINT32)p_set_scl_filter->scale_v_filter_enable, p_set_scl_filter->scale_v_filter_coef);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_out_path_scale_filter_p5(p_set_scl_filter);
	//break;

	default:
		break;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_output_path_range_clamp_param(IME_PATH_SEL path_sel, IME_CLAMP_INFO *p_clamp_info)
{
	ER er_return = E_OK;

	if (p_clamp_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (path_sel) {
	case IME_PATH1_SEL:
		ime_clamp_p1_reg(p_clamp_info->min_y, p_clamp_info->max_y, p_clamp_info->min_uv, p_clamp_info->max_uv);
		break;

	case IME_PATH2_SEL:
		ime_clamp_p2_reg(p_clamp_info->min_y, p_clamp_info->max_y, p_clamp_info->min_uv, p_clamp_info->max_uv);
		break;

	case IME_PATH3_SEL:
		ime_clamp_p3_reg(p_clamp_info->min_y, p_clamp_info->max_y, p_clamp_info->min_uv, p_clamp_info->max_uv);
		break;

	case IME_PATH4_SEL:
		ime_clamp_p4_reg(p_clamp_info->min_y, p_clamp_info->max_y, p_clamp_info->min_uv, p_clamp_info->max_uv);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_out_path_scale_filter_p5(pSetSclFilter);
	//break;

	default:
		break;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------


#if 0
ER ime_chg_chroma_median_filter_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_cmf_enable(set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------
#endif

ER ime_chg_lca_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//er_return = ime_set_lca_enable(set_en);
	//if (er_return != E_OK) {
	//  return er_return;
	//}
	ime_set_lca_enable_reg((UINT32)set_en);

	if (set_en == IME_FUNC_ENABLE) {
		er_return = ime_set_lca_scale_factor();
		if (er_return != E_OK) {
			DBG_ERR("IME: LCA, scale factor error...\r\n");

			return er_return;
		}
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_chroma_adapt_bypass_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_lca_bypass_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_chg_chroma_adapt_chroma_adjust_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_lca_chroma_adj_enable(set_en);
	ime_set_lca_chroma_adj_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_lca_luma_suppress_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_lca_luma_enable(set_en);
	ime_set_lca_la_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_chroma_adapt_image_param(IME_CHROMA_ADAPTION_IMAGE_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	er_return = ime_set_lca_input_image_info(p_set_info);
	if (er_return != E_OK) {
		return er_return;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_chroma_adapt_chroma_adjust_param(IME_CHROMA_ADAPTION_CA_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//er_return = ime_set_lca_chroma_adjust_info(p_set_info);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	if (p_set_info->lca_ca_uv_wet_prc <= IME_RANGE_8) {
		if ((p_set_info->lca_ca_uv_wet_start > 8) || (p_set_info->lca_ca_uv_wet_end > 8)) {
			DBG_ERR("IME: LCA, chroma adjust weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_ca_uv_wet_prc <= IME_RANGE_16) {
		if ((p_set_info->lca_ca_uv_wet_start > 16) || (p_set_info->lca_ca_uv_wet_end > 16)) {
			DBG_ERR("IME: LCA, chroma adjust weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_ca_uv_wet_prc <= IME_RANGE_32) {
		if ((p_set_info->lca_ca_uv_wet_start > 32) || (p_set_info->lca_ca_uv_wet_end > 32)) {
			DBG_ERR("IME: LCA, chroma adjust weighting error...\r\n");

			return E_PAR;
		}
	} else {
		DBG_ERR("IME: LCA, chroma adjust max weighting is 32...\r\n");

		return E_PAR;
	}

	//ime_set_lca_chroma_adj_enable_reg((UINT32)p_set_info->lca_ca_enable);

	ime_set_lca_chroma_adj_enable_reg((UINT32)p_set_info->lca_ca_enable);
	ime_set_lca_ca_adjust_center_reg(p_set_info->lca_ca_cent_u, p_set_info->lca_ca_cent_v);
	ime_set_lca_ca_range_reg((UINT32)p_set_info->lca_ca_uv_range, (UINT32)p_set_info->lca_ca_uv_wet_prc);
	ime_set_lca_ca_weight_reg(p_set_info->lca_ca_uv_th, p_set_info->lca_ca_uv_wet_start, p_set_info->lca_ca_uv_wet_end);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_chroma_adapt_param(IME_CHROMA_ADAPTION_IQC_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//er_return = ime_set_lca_chroma_adapt_info(p_set_info);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	if (p_set_info->lca_y_wet_prc <= IME_RANGE_8) {
		if ((p_set_info->lca_y_wet_start > 8) || (p_set_info->lca_y_wet_end > 8)) {
			DBG_ERR("IME: LCA, chroma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_y_wet_prc <= IME_RANGE_16) {
		if ((p_set_info->lca_y_wet_start > 16) || (p_set_info->lca_y_wet_end > 16)) {
			DBG_ERR("IME: LCA, chroma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_y_wet_prc <= IME_RANGE_32) {
		if ((p_set_info->lca_y_wet_start > 32) || (p_set_info->lca_y_wet_end > 32)) {
			DBG_ERR("IME: LCA, chroma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	}

	if (p_set_info->lca_uv_wet_prc <= IME_RANGE_8) {
		if ((p_set_info->lca_uv_wet_start > 8) || (p_set_info->lca_uv_wet_end > 8)) {
			DBG_ERR("IME: LCA, chroma adaptation UV weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_uv_wet_prc <= IME_RANGE_16) {
		if ((p_set_info->lca_uv_wet_start > 16) || (p_set_info->lca_uv_wet_end > 16)) {
			DBG_ERR("IME: LCA, chroma adaptation UV weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_uv_wet_prc <= IME_RANGE_32) {
		if ((p_set_info->lca_uv_wet_start > 32) || (p_set_info->lca_uv_wet_end > 32)) {
			DBG_ERR("IME: LCA, chroma adaptation UV weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_uv_wet_prc <= IME_RANGE_64) {
		if ((p_set_info->lca_uv_wet_start > 64) || (p_set_info->lca_uv_wet_end > 64)) {
			DBG_ERR("IME: LCA, chroma adaptation UV weighting error...\r\n");

			return E_PAR;
		}
	} else {
		DBG_ERR("IME: LCA, chroma adaptation UV max weighting is 64...\r\n");

		return E_PAR;
	}

	ime_set_lca_chroma_ref_image_weight_reg(p_set_info->lca_ref_y_wet, p_set_info->lca_ref_uv_wet, p_set_info->lca_out_uv_wet);
	ime_set_lca_chroma_range_y_reg(p_set_info->lca_y_range, p_set_info->lca_y_wet_prc);
	ime_set_lca_chroma_weight_y_reg(p_set_info->lca_y_th, p_set_info->lca_y_wet_start, p_set_info->lca_y_wet_end);

	ime_set_lca_chroma_range_uv_reg(p_set_info->lca_uv_range, p_set_info->lca_uv_wet_prc);
	ime_set_lca_chroma_weight_uv_reg(p_set_info->lca_uv_th, p_set_info->lca_uv_wet_start, p_set_info->lca_uv_wet_end);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_chroma_adapt_luma_suppress_param(IME_CHROMA_ADAPTION_IQL_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//er_return = ime_set_lca_luma_suppress_info(p_set_info);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	if ((p_set_info->lca_la_ref_wet > 31) || (p_set_info->lca_la_out_wet > 31)) {
		DBG_ERR("IME: LCA, luma adaptation Y weighting error...\r\n");

		return E_PAR;
	}

	if (p_set_info->lca_la_wet_prc <= IME_RANGE_8) {
		if ((p_set_info->lca_la_wet_start > 8) || (p_set_info->lca_la_wet_end > 8)) {
			DBG_ERR("IME: LCA, luma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_la_wet_prc <= IME_RANGE_16) {
		if ((p_set_info->lca_la_wet_start > 16) || (p_set_info->lca_la_wet_end > 16)) {
			DBG_ERR("IME: LCA, luma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_la_wet_prc <= IME_RANGE_32) {
		if ((p_set_info->lca_la_wet_start > 32) || (p_set_info->lca_la_wet_end > 32)) {
			DBG_ERR("IME: LCA, luma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else {
		DBG_ERR("IME: LCA, luma adaptation Y max weighting is 32...\r\n");

		return E_PAR;
	}

	ime_set_lca_la_enable_reg((UINT32)p_set_info->lca_la_enable);
	ime_set_lca_luma_ref_image_weight_reg(p_set_info->lca_la_ref_wet, p_set_info->lca_la_out_wet);
	ime_set_lca_luma_range_y_reg(p_set_info->lca_la_range, p_set_info->lca_la_wet_prc);
	ime_set_lca_luma_weight_y_reg(p_set_info->lca_la_th, p_set_info->lca_la_wet_start, p_set_info->lca_la_wet_end);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_get_chroma_adapt_image_size_info(IME_SIZE_INFO *p_get_size)
{
	if (p_get_size != NULL) {
		//ime_get_lca_image_size_info(p_get_size);
		ime_get_lca_image_size_reg(&(p_get_size->size_h), &(p_get_size->size_v));
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_chroma_adapt_lineoffset_info(IME_LINEOFFSET_INFO *p_get_lofs)
{
	if (p_get_lofs != NULL) {
		//ime_get_lca_lineoffset_info(p_get_lofs);
		ime_get_lca_lineoffset_reg(&(p_get_lofs->lineoffset_y), &(p_get_lofs->lineoffset_cb));
	}
}
//------------------------------------------------------------------------------

VOID ime_get_chroma_adapt_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr0, IME_DMA_ADDR_INFO *p_get_addr1)
{
	//ime_get_lca_dma_addr_info(p_get_addr0, p_get_addr1);

	if (p_get_addr0 != NULL) {
		ime_get_lca_dma_addr0_reg(&(p_get_addr0->addr_y), &(p_get_addr0->addr_cb));
	}

	if (p_get_addr1 != NULL) {
		ime_get_lca_dma_addr1_reg(&(p_get_addr1->addr_y), &(p_get_addr1->addr_cb));
	}

#if (IME_DMA_CACHE_HANDLE == 1)

	if (p_get_addr0 != NULL) {
		//p_get_addr0->addr_y = dma_getNonCacheAddr(p_get_addr0->addr_y);
		//p_get_addr0->addr_cb = dma_getNonCacheAddr(p_get_addr0->addr_cb);
		//p_get_addr0->addr_cr = dma_getNonCacheAddr(p_get_addr0->addr_cr);
	}

	//p_get_addr1->addr_y = dma_getNonCacheAddr(p_get_addr1->addr_y);
	//p_get_addr1->addr_cb = dma_getNonCacheAddr(p_get_addr1->addr_cb);
	//p_get_addr1->addr_cr = dma_getNonCacheAddr(p_get_addr1->addr_cr);
#endif
}
//------------------------------------------------------------------------------

ER ime_chg_lca_subout_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_lca_subout_enable(set_en);
	ime_set_lca_subout_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_chroma_adapt_subout_source(IME_LCA_SUBOUT_SRC set_src)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_lca_subout_src(set_src);
	ime_set_lca_subout_source_reg((UINT32)set_src);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_chg_chroma_adapt_subout_param(IME_CHROMA_APAPTION_SUBOUT_PARAM *p_set_info)
{
	ER er_return = E_OK;


	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	er_return = ime_set_lca_subout(p_set_info);
	if (er_return != E_OK) {
		return er_return;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_chroma_adapt_subout_lineoffset_info(IME_LINEOFFSET_INFO *p_set_lofs)
{
	ER er_return = E_OK;

	if (p_set_lofs == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//er_return = ime_set_lca_subout_lineoffset(p_set_lofs->lineoffset_y);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	er_return = ime_check_lineoffset(p_set_lofs->lineoffset_y, p_set_lofs->lineoffset_y, TRUE);
	if (er_return != E_OK) {

		DBG_ERR("IME - uiLineOffsetY: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_y);
		DBG_ERR("IME - uiLineOffsetCb: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_y);

		return E_PAR;
	}

	ime_set_lca_subout_lineoffset_reg(p_set_lofs->lineoffset_y);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_get_chroma_adapt_subout_lineoffset_info(IME_LINEOFFSET_INFO *p_get_lofs)
{
	if (p_get_lofs != NULL) {
		//p_get_lofs->lineoffset_y = ime_get_lca_subout_lineoffset();
		//p_get_lofs->lineoffset_cb = p_get_lofs->lineoffset_y;

		p_get_lofs->lineoffset_y = ime_get_lca_subout_lineoffset_reg();
		p_get_lofs->lineoffset_cb = p_get_lofs->lineoffset_y;
	}
}
//-------------------------------------------------------------------------------


ER ime_chg_chroma_adapt_subout_dma_addr_info(IME_DMA_ADDR_INFO *p_set_addr)
{
	ER er_return = E_OK;

	UINT32 flush_size_y = 0x0;
	BOOL flush_type_y = FALSE;

	IME_LINEOFFSET_INFO set_lofs = {0x0};
	IME_DMA_ADDR_INFO set_addr = {0x0};
	IME_SIZE_INFO get_size = {0x0};

	if (p_set_addr == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_engine_operation_mode = ime_get_operate_mode();

	ime_get_chroma_adapt_image_size_info(&get_size);
	//set_lofs.lineoffset_y = ime_get_lca_subout_lineoffset();
	set_lofs.lineoffset_y = ime_get_lca_subout_lineoffset_reg();
	flush_size_y = (set_lofs.lineoffset_y * get_size.size_v);
	flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);

	// video buffer do not flush
	set_addr.addr_y  = ime_output_dma_buf_flush(p_set_addr->addr_y, flush_size_y, flush_type_y, IME_DO_BUF_FLUSH, ime_engine_operation_mode);
	set_addr.addr_cb = set_addr.addr_y;//ime_output_dma_buf_flush(set_addr.addr_cb, FlushSizeC, FlushTypeC, ime_engine_operation_mode);
	set_addr.addr_cr = set_addr.addr_y;//ime_output_dma_buf_flush(set_addr.addr_cr, FlushSizeC, FlushTypeC, ime_engine_operation_mode);

	if (ime_engine_operation_mode == IME_OPMODE_D2D) {
		ime_out_buf_flush[IME_OUT_LCASUBOUT].flush_en = ENABLE;
		ime_out_buf_flush[IME_OUT_LCASUBOUT].buf_addr = set_addr.addr_y;
		ime_out_buf_flush[IME_OUT_LCASUBOUT].buf_size = flush_size_y;
	}

	//er_return = ime_set_lca_subout_dma_addr(set_addr.addr_y);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_lca_subout_dma_addr_reg(set_addr.addr_y);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_get_chroma_adapt_subout_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr)
{
	//p_get_addr->addr_y = ime_get_lca_subout_dma_addr();

	p_get_addr->addr_y = ime_get_lca_subout_dma_addr_reg();
	p_get_addr->addr_cb = p_get_addr->addr_y;
	p_get_addr->addr_cr = p_get_addr->addr_y;
}
//-------------------------------------------------------------------------------

#if 0 // 520 do not support
ER ime_chgColorSpaceTransformEnable(IME_CST_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_cst_enable(p_set_info);

	ime_load();

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

ER ime_chg_dark_bright_chroma_suppress_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_dbcs_enable(set_en);
	ime_set_dbcs_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_dark_bright_chroma_suppress_param(IME_DBCS_IQ_INFO *p_set_info)
{
	ER er_return = E_OK;
	UINT32 i = 0;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//er_return = ime_set_dbcs_iq_info(p_set_info);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	if ((p_set_info->uiCentU > 255) || (p_set_info->uiCentV > 255)) {
		DBG_ERR("IME: DBCS, center parameter overflow...\r\n");

		return E_PAR;
	}

	for (i = 0; i < 16; i++) {
		if (p_set_info->puiWtY[i] > 16) {
			DBG_ERR("IME: DBCS, weighting y parameter overflow...\r\n");

			return E_PAR;
		}
	}

	for (i = 0; i < 16; i++) {
		if (p_set_info->puiWtC[i] > 16) {
			DBG_ERR("IME: DBCS, weighting uv parameter overflow...\r\n");

			return E_PAR;
		}
	}

	ime_set_dbcs_mode_reg((UINT32)p_set_info->OpMode);
	ime_set_dbcs_step_reg(p_set_info->uiStepY, p_set_info->uiStepC);
	ime_set_dbcs_center_reg(p_set_info->uiCentU, p_set_info->uiCentV);
	ime_set_dbcs_weight_y_reg(p_set_info->puiWtY);
	ime_set_dbcs_weight_uv_reg(p_set_info->puiWtC);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

#if 0
ER ime_chg_single_super_resolution_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

#if (defined(_NVT_EMULATION_) == OFF)
	{
		if (g_SsrMode == IME_SSR_MODE_USER) {
			ime_set_ssr_enable(set_en);
		}
	}
#else
	{
		ime_set_ssr_enable(set_en);
	}
#endif

	if (set_en == IME_FUNC_ENABLE) {
#if (defined(_NVT_EMULATION_) == ON)
		{
			er_return = ime_chg_stripe_param(&ime_stripe_info);
			if (er_return != E_OK)
			{
				return er_return;
			}
		}
#else
		{
			ime_stripe_info.stripe_cal_mode = IME_STRIPE_AUTO_MODE;
			er_return = ime_chg_stripe_param(&ime_stripe_info);
			if (er_return != E_OK)
			{
				return er_return;
			}
		}
#endif

		er_return = ime_set_scale_factor_with_ssr();
		if (er_return != E_OK) {
			return er_return;
		}
	}

	if (ime_engine_state_machine_status == IME_ENGINE_RUN) {
		er_return = ime_check_input_path_stripe();
		if (er_return != E_OK) {
			return er_return;
		}
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_single_super_resolution_param(IME_SSR_IQ_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	er_return = ime_set_ssr_iq_info(p_set_info);
	if (er_return != E_OK) {
		return er_return;
	}

	ime_load();

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0 // 520 do not support
ER ime_chg_film_grain_noise_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_gn_enable(set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_chg_film_grain_noise_param(IME_FILM_GRAIN_IQ_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	er_return = ime_set_gn_iq_info(p_set_info);
	if (er_return != E_OK) {
		return er_return;
	}

	ime_load();

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_chg_progressive_to_interlace_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_p2i_enable(set_en);

	ime_load();

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

ER ime_chg_data_stamp_enable(IME_DS_SETNUM set_num, IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (set_num) {
	case IME_DS_SET0:
		ime_set_osd_set0_enable_reg((UINT32)set_en);
		break;

	case IME_DS_SET1:
		ime_set_osd_set1_enable_reg((UINT32)set_en);
		break;

	case IME_DS_SET2:
		ime_set_osd_set2_enable_reg((UINT32)set_en);
		break;

	case IME_DS_SET3:
		ime_set_osd_set3_enable_reg((UINT32)set_en);
		break;

	default:
		break;
	}


	ime_load();

	return E_OK;;
}
//-------------------------------------------------------------------------------

ER ime_chg_data_stamp_cst_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_ds_cst_enable(set_en);
	ime_set_osd_cst_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;;
}
//-------------------------------------------------------------------------------

ER ime_chg_data_stamp_color_key_enable(IME_DS_SETNUM set_num, IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (set_num) {
	case IME_DS_SET0:
		ime_set_osd_set0_color_key_enable_reg((UINT32)set_en);
		break;

	case IME_DS_SET1:
		ime_set_osd_set1_color_key_enable_reg((UINT32)set_en);
		break;

	case IME_DS_SET2:
		ime_set_osd_set2_color_key_enable_reg((UINT32)set_en);
		break;

	case IME_DS_SET3:
		ime_set_osd_set3_color_key_enable_reg((UINT32)set_en);
		break;

	default:
		er_return = E_PAR;
		break;
	}

	ime_load();

	return er_return;
}
//-------------------------------------------------------------------------------



ER ime_chg_data_stamp_image_param(IME_DS_SETNUM set_num, IME_STAMP_IMAGE_INFO *p_set_info)
{
	ER er_return = E_OK;
	UINT32 set_addr = 0x0, data_size = 0x0;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (set_num) {
	case IME_DS_SET0:
		//er_return = ime_set_osd_set0_image_info(p_set_info);

		if ((p_set_info->ds_img_size.size_h > 8192) || (p_set_info->ds_img_size.size_v > 8192)) {
			DBG_ERR("IME: OSD0 input size parameter overflow...\r\n");

			return E_PAR;
		}

		//if(p_set_info->ds_img_size.size_v > 128)
		//{
		//    DBG_WRN("IME: V size is larger than 128, bandwidth may be not enought...\r\n");
		//}

		er_return  = ime_check_lineoffset(p_set_info->ds_lofs, p_set_info->ds_lofs, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_info->ds_lofs);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_info->ds_lofs);

			return er_return;
		}

		//GetInSrc = ime_get_engine_info(IME_GET_INPUT_SRC);
		//OpMode = ime_get_operate_mode();
		data_size = p_set_info->ds_lofs * p_set_info->ds_img_size.size_v;
		data_size = IME_ALIGN_CEIL_32(data_size);
		set_addr = ime_input_dma_buf_flush(p_set_info->ds_addr, data_size, ime_engine_operation_mode);
		//er_return = ime_check_dma_addr(set_addr, set_addr, set_addr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_osd_set0_image_size_reg(p_set_info->ds_img_size.size_h, p_set_info->ds_img_size.size_v);
		ime_set_osd_set0_format_reg((UINT32)p_set_info->ds_fmt);
		ime_set_osd_set0_position_reg(p_set_info->ds_pos.pos_x, p_set_info->ds_pos.pos_y);
		ime_set_osd_set0_lineoffset_reg(p_set_info->ds_lofs);
		ime_set_osd_set0_dma_addr_reg(set_addr);
		break;

	case IME_DS_SET1:
		//er_return = ime_set_osd_set1_image_info(p_set_info);

		if ((p_set_info->ds_img_size.size_h > 8192) || (p_set_info->ds_img_size.size_v > 8192)) {
			DBG_ERR("IME: OSD1, input size parameter overflow...\r\n");

			return E_PAR;
		}

		//if(p_set_info->ds_img_size.size_v > 128)
		//{
		//    DBG_WRN("IME: V size is larger than 128, bandwidth may be not enought...\r\n");
		//}

		er_return  = ime_check_lineoffset(p_set_info->ds_lofs, p_set_info->ds_lofs, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_info->ds_lofs);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_info->ds_lofs);

			return er_return;
		}

		//GetInSrc = ime_get_engine_info(IME_GET_INPUT_SRC);
		//OpMode = ime_get_operate_mode();
		data_size = p_set_info->ds_lofs * p_set_info->ds_img_size.size_v;
		data_size = IME_ALIGN_CEIL_32(data_size);
		set_addr = ime_input_dma_buf_flush(p_set_info->ds_addr, data_size, ime_engine_operation_mode);
		//er_return = ime_check_dma_addr(set_addr, set_addr, set_addr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_osd_set1_image_size_reg(p_set_info->ds_img_size.size_h, p_set_info->ds_img_size.size_v);
		ime_set_osd_set1_format_reg((UINT32)p_set_info->ds_fmt);
		ime_set_osd_set1_position_reg(p_set_info->ds_pos.pos_x, p_set_info->ds_pos.pos_y);
		ime_set_osd_set1_lineoffset_reg(p_set_info->ds_lofs);
		ime_set_osd_set1_dma_addr_reg(set_addr);
		break;

	case IME_DS_SET2:
		//er_return = ime_set_osd_set2_image_info(p_set_info);

		if ((p_set_info->ds_img_size.size_h > 8192) || (p_set_info->ds_img_size.size_v > 8192)) {
			DBG_ERR("IME: OSD2, input size parameter overflow...\r\n");

			return E_PAR;
		}

		//if(p_set_info->ds_img_size.size_v > 128)
		//{
		//    DBG_WRN("IME: V size is larger than 128, bandwidth may be not enought...\r\n");
		//}

		er_return  = ime_check_lineoffset(p_set_info->ds_lofs, p_set_info->ds_lofs, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_info->ds_lofs);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_info->ds_lofs);

			return er_return;
		}

		//GetInSrc = ime_get_engine_info(IME_GET_INPUT_SRC);
		//OpMode = ime_get_operate_mode();
		data_size = p_set_info->ds_lofs * p_set_info->ds_img_size.size_v;
		data_size = IME_ALIGN_CEIL_32(data_size);
		set_addr = ime_input_dma_buf_flush(p_set_info->ds_addr, data_size, ime_engine_operation_mode);
		//er_return = ime_check_dma_addr(SetAddr, SetAddr, SetAddr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_osd_set2_image_size_reg(p_set_info->ds_img_size.size_h, p_set_info->ds_img_size.size_v);
		ime_set_osd_set2_format_reg((UINT32)p_set_info->ds_fmt);
		ime_set_osd_set2_position_reg(p_set_info->ds_pos.pos_x, p_set_info->ds_pos.pos_y);
		ime_set_osd_set2_lineoffset_reg(p_set_info->ds_lofs);
		ime_set_osd_set2_dma_addr_reg(set_addr);
		break;

	case IME_DS_SET3:
		//er_return = ime_set_osd_set3_image_info(p_set_info);

		if ((p_set_info->ds_img_size.size_h > 8192) || (p_set_info->ds_img_size.size_v > 8192)) {
			DBG_ERR("IME: OSD3, input size parameter overflow...\r\n");

			return E_PAR;
		}

		//if(p_set_info->ds_img_size.size_v > 128)
		//{
		//    DBG_WRN("IME: V size is larger than 128, bandwidth may be not enought...\r\n");
		//}

		er_return  = ime_check_lineoffset(p_set_info->ds_lofs, p_set_info->ds_lofs, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_info->ds_lofs);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_info->ds_lofs);

			return er_return;
		}

		//GetInSrc = ime_get_engine_info(IME_GET_INPUT_SRC);
		//OpMode = ime_get_operate_mode();
		data_size = p_set_info->ds_lofs * p_set_info->ds_img_size.size_v;
		data_size = IME_ALIGN_CEIL_32(data_size);
		set_addr = ime_input_dma_buf_flush(p_set_info->ds_addr, data_size, ime_engine_operation_mode);
		//er_return = ime_check_dma_addr(set_addr, set_addr, set_addr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_osd_set3_image_size_reg(p_set_info->ds_img_size.size_h, p_set_info->ds_img_size.size_v);
		ime_set_osd_set3_format_reg((UINT32)p_set_info->ds_fmt);
		ime_set_osd_set3_position_reg(p_set_info->ds_pos.pos_x, p_set_info->ds_pos.pos_y);
		ime_set_osd_set3_lineoffset_reg(p_set_info->ds_lofs);
		ime_set_osd_set3_dma_addr_reg(set_addr);
		break;

	default:
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		return er_return;
	}

	ime_load();

	return E_OK;;
}
//-------------------------------------------------------------------------------

ER ime_chg_data_stamp_param(IME_DS_SETNUM set_num, IME_STAMP_IQ_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (set_num) {
	case IME_DS_SET0:
		//er_return = ime_set_osd_set0_iq_info(p_set_info);

		ime_set_osd_set0_blend_weight_reg(p_set_info->ds_bld_wet0, p_set_info->ds_bld_wet1);
		ime_set_osd_set0_color_key_enable_reg((UINT32)p_set_info->ds_ck_enable);

		ime_set_osd_set0_color_palette_enable_reg((UINT32)p_set_info->ds_plt_enable);
		ime_set_osd_set0_color_key_mode_reg((UINT32)p_set_info->ds_ck_mode);

		ime_set_osd_set0_color_key_reg(p_set_info->ds_ck_a, p_set_info->ds_ck_r, p_set_info->ds_ck_g, p_set_info->ds_ck_b);
		break;

	case IME_DS_SET1:
		//er_return = ime_set_osd_set1_iq_info(p_set_info);

		ime_set_osd_set1_blend_weight_reg(p_set_info->ds_bld_wet0, p_set_info->ds_bld_wet1);
		ime_set_osd_set1_color_key_enable_reg((UINT32)p_set_info->ds_ck_enable);

		ime_set_osd_set1_color_palette_enable_reg((UINT32)p_set_info->ds_plt_enable);
		ime_set_osd_set1_color_key_mode_reg((UINT32)p_set_info->ds_ck_mode);

		ime_set_osd_set1_color_key_reg(p_set_info->ds_ck_a, p_set_info->ds_ck_r, p_set_info->ds_ck_g, p_set_info->ds_ck_b);
		break;

	case IME_DS_SET2:
		//er_return = ime_set_osd_set2_iq_info(p_set_info);

		ime_set_osd_set2_blend_weight_reg(p_set_info->ds_bld_wet0, p_set_info->ds_bld_wet1);
		ime_set_osd_set2_color_key_enable_reg((UINT32)p_set_info->ds_ck_enable);

		ime_set_osd_set2_color_palette_enable_reg((UINT32)p_set_info->ds_plt_enable);
		ime_set_osd_set2_color_key_mode_reg((UINT32)p_set_info->ds_ck_mode);

		ime_set_osd_set2_color_key_reg(p_set_info->ds_ck_a, p_set_info->ds_ck_r, p_set_info->ds_ck_g, p_set_info->ds_ck_b);
		break;

	case IME_DS_SET3:
		//er_return = ime_set_osd_set3_iq_info(p_set_info);

		ime_set_osd_set3_blend_weight_reg(p_set_info->ds_bld_wet0, p_set_info->ds_bld_wet1);
		ime_set_osd_set3_color_key_enable_reg((UINT32)p_set_info->ds_ck_enable);

		ime_set_osd_set3_color_palette_enable_reg((UINT32)p_set_info->ds_plt_enable);
		ime_set_osd_set3_color_key_mode_reg((UINT32)p_set_info->ds_ck_mode);

		ime_set_osd_set3_color_key_reg(p_set_info->ds_ck_a, p_set_info->ds_ck_r, p_set_info->ds_ck_g, p_set_info->ds_ck_b);
		break;

	default:
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		return er_return;
	}

#if 0 // remove from nt96680
	er_return = ime_check_stripe_mode_data_stamp(set_num);
	if (er_return != E_OK) {
		DBG_ERR("IME: data stamp only for single stripe mode...\r\n");
		return er_return;
	}
#endif

	ime_load();

	return E_OK;;
}
//-------------------------------------------------------------------------------

ER ime_chg_data_stamp_color_coefs_param(IME_STAMP_CST_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (p_set_info->ds_cst_param_mode == IME_PARAM_USER_MODE) {
		//er_return = ime_set_osd_cst_coef(p_set_info->ds_cst_coef0, p_set_info->ds_cst_coef1, p_set_info->ds_cst_coef2, p_set_info->ds_cst_coef3);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_osd_cst_coef_reg(p_set_info->ds_cst_coef0, p_set_info->ds_cst_coef1, p_set_info->ds_cst_coef2, p_set_info->ds_cst_coef3);
	} else if (p_set_info->ds_cst_param_mode == IME_PARAM_AUTO_MODE) {
		//er_return = ime_set_osd_cst_coef(21, 19, 49, 43);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_osd_cst_coef_reg(21, 19, 49, 43);
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_data_stamp_color_palette_info(IME_STAMP_PLT_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_osd_color_palette_mode_reg(p_set_info->ds_plt_mode);
	ime_set_osd_color_palette_lut_reg(p_set_info->ds_plt_tab_a, p_set_info->ds_plt_tab_r, p_set_info->ds_plt_tab_g, p_set_info->ds_plt_tab_b);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_get_data_stamp_image_size_info(IME_DS_SETNUM set_num, IME_SIZE_INFO *p_get_info)
{
	if (p_get_info != NULL) {
		switch (set_num) {
		case IME_DS_SET0:
			//ime_get_osd_set0_image_size(p_get_info);
			ime_get_osd_set0_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
			break;

		case IME_DS_SET1:
			//ime_get_osd_set1_image_size(p_get_info);
			ime_get_osd_set1_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
			break;

		case IME_DS_SET2:
			//ime_get_osd_set2_image_size(p_get_info);
			ime_get_osd_set2_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
			break;

		case IME_DS_SET3:
			//ime_get_osd_set3_image_size(p_get_info);
			ime_get_osd_set3_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
			break;

		default:
			break;
		}
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_data_stamp_lineoffset_info(IME_DS_SETNUM set_num, IME_LINEOFFSET_INFO *p_get_info)
{
	if (p_get_info != NULL) {
		switch (set_num) {
		case IME_DS_SET0:
			//ime_get_osd_set0_lineoffset(p_get_info);

			ime_get_osd_set0_lineoffset_reg(&(p_get_info->lineoffset_y));
			p_get_info->lineoffset_cb = p_get_info->lineoffset_y;
			break;

		case IME_DS_SET1:
			//ime_get_osd_set1_lineoffset(p_get_info);

			ime_get_osd_set1_lineoffset_reg(&(p_get_info->lineoffset_y));
			p_get_info->lineoffset_cb = p_get_info->lineoffset_y;
			break;

		case IME_DS_SET2:
			//ime_get_osd_set2_lineoffset(p_get_info);

			ime_get_osd_set2_lineoffset_reg(&(p_get_info->lineoffset_y));
			p_get_info->lineoffset_cb = p_get_info->lineoffset_y;
			break;

		case IME_DS_SET3:
			//ime_get_osd_set3_lineoffset(p_get_info);

			ime_get_osd_set3_lineoffset_reg(&(p_get_info->lineoffset_y));
			p_get_info->lineoffset_cb = p_get_info->lineoffset_y;
			break;

		default:
			break;
		}
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_data_stamp_dma_addr_info(IME_DS_SETNUM set_num, IME_DMA_ADDR_INFO *p_get_info)
{
	if (p_get_info != NULL) {
		switch (set_num) {
		case IME_DS_SET0:
			//ime_get_osd_set0_dma_addr(p_get_info);

			ime_get_osd_set0_dma_addr_reg(&(p_get_info->addr_y));
			break;

		case IME_DS_SET1:
			//ime_get_osd_set1_dma_addr(p_get_info);

			ime_get_osd_set1_dma_addr_reg(&(p_get_info->addr_y));
			break;

		case IME_DS_SET2:
			//ime_get_osd_set2_dma_addr(p_get_info);

			ime_get_osd_set2_dma_addr_reg(&(p_get_info->addr_y));
			break;

		case IME_DS_SET3:
			//ime_get_osd_set3_dma_addr(p_get_info);

			ime_get_osd_set3_dma_addr_reg(&(p_get_info->addr_y));
			break;

		default:
			DBG_ERR("Data stamp selection error...\r\n");
			break;
		}

		p_get_info->addr_cb = p_get_info->addr_cr = p_get_info->addr_y;

#if (IME_DMA_CACHE_HANDLE == 1)
		//p_get_info->addr_y = dma_getNonCacheAddr(p_get_info->addr_y);
		//p_get_info->addr_cb = dma_getNonCacheAddr(p_get_info->addr_cb);
		//p_get_info->addr_cr = dma_getNonCacheAddr(p_get_info->addr_cr);
#endif
	}
}
//-------------------------------------------------------------------------------

ER ime_chg_privacy_mask_enable(IME_PM_SET_SEL set_num, IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_privacy_mask_enable(set_num, set_en);

	switch (set_num) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_enable_reg(set_en);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_enable_reg(set_en);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_enable_reg(set_en);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_enable_reg(set_en);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_enable_reg(set_en);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_enable_reg(set_en);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_enable_reg(set_en);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_enable_reg(set_en);
		break;

	default:
		break;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_privacy_mask_param(IME_PM_SET_SEL set_num, IME_PM_PARAM *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_cal_convex_hull_coefs(p_set_info->pm_coord, 4, ime_set_pm_coef_line0, ime_set_pm_coef_line1, ime_set_pm_coef_line2, ime_set_pm_coef_line3);

	ime_set_privacy_mask_enable(set_num, p_set_info->pm_enable);

	ime_set_privacy_mask_type(set_num, p_set_info->pm_mask_type);

	ime_set_privacy_mask_color(set_num, p_set_info->pm_color);

	ime_set_privacy_mask_weight(set_num, p_set_info->pm_wet);


	ime_set_privacy_mask_line0(set_num, ime_set_pm_coef_line0);
	ime_set_privacy_mask_line1(set_num, ime_set_pm_coef_line1);
	ime_set_privacy_mask_line2(set_num, ime_set_pm_coef_line2);
	ime_set_privacy_mask_line3(set_num, ime_set_pm_coef_line3);


	switch (set_num) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_hollow_enable_reg(p_set_info->pm_hlw_enable);

		if (p_set_info->pm_hlw_enable == IME_FUNC_ENABLE) {
			ime_cal_convex_hull_coefs(p_set_info->pm_coord_hlw, 4, ime_set_pm_coef_line_hlw0, ime_set_pm_coef_line_hlw1, ime_set_pm_coef_line_hlw2, ime_set_pm_coef_line_hlw3);

			ime_set_privacy_mask_line0(IME_PM_SET1, ime_set_pm_coef_line_hlw0);
			ime_set_privacy_mask_line1(IME_PM_SET1, ime_set_pm_coef_line_hlw1);
			ime_set_privacy_mask_line2(IME_PM_SET1, ime_set_pm_coef_line_hlw2);
			ime_set_privacy_mask_line3(IME_PM_SET1, ime_set_pm_coef_line_hlw3);
		}
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_hollow_enable_reg(p_set_info->pm_hlw_enable);

		if (p_set_info->pm_hlw_enable == IME_FUNC_ENABLE) {
			ime_cal_convex_hull_coefs(p_set_info->pm_coord_hlw, 4, ime_set_pm_coef_line_hlw0, ime_set_pm_coef_line_hlw1, ime_set_pm_coef_line_hlw2, ime_set_pm_coef_line_hlw3);

			ime_set_privacy_mask_line0(IME_PM_SET3, ime_set_pm_coef_line_hlw0);
			ime_set_privacy_mask_line1(IME_PM_SET3, ime_set_pm_coef_line_hlw1);
			ime_set_privacy_mask_line2(IME_PM_SET3, ime_set_pm_coef_line_hlw2);
			ime_set_privacy_mask_line3(IME_PM_SET3, ime_set_pm_coef_line_hlw3);
		}
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_hollow_enable_reg(p_set_info->pm_hlw_enable);

		if (p_set_info->pm_hlw_enable == IME_FUNC_ENABLE) {
			ime_cal_convex_hull_coefs(p_set_info->pm_coord_hlw, 4, ime_set_pm_coef_line_hlw0, ime_set_pm_coef_line_hlw1, ime_set_pm_coef_line_hlw2, ime_set_pm_coef_line_hlw3);

			ime_set_privacy_mask_line0(IME_PM_SET5, ime_set_pm_coef_line_hlw0);
			ime_set_privacy_mask_line1(IME_PM_SET5, ime_set_pm_coef_line_hlw1);
			ime_set_privacy_mask_line2(IME_PM_SET5, ime_set_pm_coef_line_hlw2);
			ime_set_privacy_mask_line3(IME_PM_SET5, ime_set_pm_coef_line_hlw3);
		}
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_hollow_enable_reg(p_set_info->pm_hlw_enable);

		if (p_set_info->pm_hlw_enable == IME_FUNC_ENABLE) {
			ime_cal_convex_hull_coefs(p_set_info->pm_coord_hlw, 4, ime_set_pm_coef_line_hlw0, ime_set_pm_coef_line_hlw1, ime_set_pm_coef_line_hlw2, ime_set_pm_coef_line_hlw3);

			ime_set_privacy_mask_line0(IME_PM_SET7, ime_set_pm_coef_line_hlw0);
			ime_set_privacy_mask_line1(IME_PM_SET7, ime_set_pm_coef_line_hlw1);
			ime_set_privacy_mask_line2(IME_PM_SET7, ime_set_pm_coef_line_hlw2);
			ime_set_privacy_mask_line3(IME_PM_SET7, ime_set_pm_coef_line_hlw3);
		}
		break;

	case IME_PM_SET1:
	case IME_PM_SET3:
	case IME_PM_SET5:
	case IME_PM_SET7:
		break;

	default:
		break;
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_privacy_mask_pixelation_image_size(IME_SIZE_INFO *p_set_size)
{
	ER er_return = E_OK;

	if (p_set_size == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (p_set_size->size_h > 2048) {
		DBG_ERR("IME: image width of pixelation > 2048...\r\n");

		return E_PAR;
	}

	if (p_set_size->size_v > 2048) {
		DBG_ERR("IME: image height of pixelation > 2048..\r\n");

		return E_PAR;
	}

	//ime_set_privacy_mask_pxl_image_size(p_set_size->size_h, p_set_size->size_v);
	ime_set_privacy_mask_pxl_image_size_reg(p_set_size->size_h, p_set_size->size_v);
	ime_set_privacy_mask_pxl_image_format_reg(2);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_privacy_mask_pixelation_block_size(IME_PM_PXL_BLK_SIZE *p_set_size)
{
	ER er_return = E_OK;

	if (p_set_size == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_privacy_mask_pxl_block_size((UINT32)*p_set_size);
	ime_set_privacy_mask_pxl_blk_size_reg(*p_set_size);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_privacy_maask_pixelation_image_lineoffset(UINT32 *p_set_lofs)
{
	ER er_return = E_OK;

	if (p_set_lofs == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	if (*p_set_lofs == 0x0) {
		DBG_ERR("IME: pixelation image Y channel lineoffset is zero...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_privacy_mask_pxl_image_lineoffset(*p_set_lofs);
	ime_set_privacy_mask_pxl_image_lineoffset_reg(*p_set_lofs);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_privacy_maask_pixelation_image_dma_addr(UINT32 *p_set_addr)
{
	ER er_return = E_OK;
	UINT32 get_size_h = 0, get_size_v = 0, get_lofs = 0;
	UINT32 flush_size = 0;

	UINT32 get_addr = 0x0;

	if (p_set_addr == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	get_addr = *p_set_addr;

	//ime_get_privacy_mask_pxl_image_size(&get_size_h, &get_size_v);
	ime_get_privacy_mask_pxl_image_size_reg(&get_size_h, &get_size_v);

	//ime_get_privacy_mask_pxl_image_lineoffset(&get_lofs);
	ime_get_privacy_mask_pxl_image_lineoffset_reg(&get_lofs);
	flush_size = (get_lofs * get_size_v);
	flush_size = IME_ALIGN_CEIL_32(flush_size);
	get_addr  = ime_input_dma_buf_flush(get_addr, flush_size, ime_engine_operation_mode);

	//er_return = ime_check_dma_addr(get_addr, get_addr, get_addr, IME_DMA_INPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	//ime_set_privacy_mask_pxl_image_dma_addr(get_addr);
	ime_set_privacy_mask_pxl_image_dma_addr_reg(get_addr);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pixelation_image_size_info(IME_SIZE_INFO *p_get_size)
{
	if (p_get_size != NULL) {
		//ime_get_privacy_mask_pxl_image_size(&(p_get_size->size_h), &(p_get_size->size_v));
		ime_get_privacy_mask_pxl_image_size_reg(&(p_get_size->size_h), &(p_get_size->size_v));
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pixelation_image_lineoffset_info(UINT32 *p_get_lofs)
{
	if (p_get_lofs != NULL) {
		//ime_get_privacy_mask_pxl_image_lineoffset(p_get_lofs);
		ime_get_privacy_mask_pxl_image_lineoffset_reg(p_get_lofs);
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pixelation_image_dma_addr_info(UINT32 *p_get_addr)
{
	if (p_get_addr != NULL) {
		//ime_get_privacy_mask_pxl_image_dma_addr(p_get_addr);
		ime_get_privacy_mask_pxl_image_dma_addr_reg(p_get_addr);

#if (IME_DMA_CACHE_HANDLE == 1)
		//*p_get_addr = dma_getNonCacheAddr(*p_get_addr);
#endif
	}
}
//-------------------------------------------------------------------------------

#if (defined(_NVT_EMULATION_) == ON)
static void ime_time_out_cb(UINT32 event)
{
	DBG_ERR("IME time out!\r\n");
	ime_end_time_out_status = TRUE;
	//iset_flg(flg_id_ime, FLGPTN_IME_FRAMEEND);
	ime_platform_flg_set(FLGPTN_IME_FRAMEEND);
}
#endif

ER ime_waitFlagFrameEnd(IME_FLAG_CLEAR_SEL IsClearFlag)
{
	FLGPTN uiFlag;

	if (IsClearFlag == IME_FLAG_CLEAR) {
		//clr_flg(flg_id_ime, FLGPTN_IME_FRAMEEND);
		ime_platform_flg_clear(FLGPTN_IME_FRAMEEND);
	}

#if (defined(_NVT_EMULATION_) == ON)
	if (timer_open(&ime_timer_id, ime_time_out_cb) != E_OK) {
		DBG_WRN("IME allocate timer fail\r\n");
	} else {
		timer_cfg(ime_timer_id, 15000000, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	}
#endif

	//wai_flg(&uiFlag, flg_id_ime, FLGPTN_IME_FRAMEEND, TWF_ORW | TWF_CLR);
	ime_platform_flg_wait(&uiFlag, FLGPTN_IME_FRAMEEND);

#if (defined(_NVT_EMULATION_) == OFF)
	if (ime_engine_operation_mode == IME_OPMODE_D2D) {
		ime_output_buffer_flush_dma_end();
	}
#endif

#if (defined(_NVT_EMULATION_) == ON)
	timer_close(ime_timer_id);
#endif

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_clear_flag_frame_end(VOID)
{
	//clr_flg(flg_id_ime, FLGPTN_IME_FRAMEEND);
	ime_platform_flg_clear(FLGPTN_IME_FRAMEEND);
}
//-------------------------------------------------------------------------------

ER ime_waitFlagFrameStart(IME_FLAG_CLEAR_SEL IsClearFlag)
{
	FLGPTN uiFlag;

	if (IsClearFlag == IME_FLAG_CLEAR) {
		ime_platform_flg_clear(FLGPTN_IME_FRAMESTART);
	}

	ime_platform_flg_wait(&uiFlag, FLGPTN_IME_FRAMESTART);

	return E_OK;
}


VOID ime_clear_flag_frame_start(VOID)
{
	ime_platform_flg_clear(FLGPTN_IME_FRAMESTART);
}
//-------------------------------------------------------------------------------

ER ime_waitFlagLinkedListEnd(IME_FLAG_CLEAR_SEL IsClearFlag)
{
	FLGPTN uiFlag;

	if (IsClearFlag == IME_FLAG_CLEAR) {
		//clr_flg(flg_id_ime, FLGPTN_IME_LLEND);
		ime_platform_flg_clear(FLGPTN_IME_FRAMEEND);
	}

	//wai_flg(&uiFlag, flg_id_ime, FLGPTN_IME_LLEND, TWF_ORW | TWF_CLR);
	ime_platform_flg_wait(&uiFlag, FLGPTN_IME_LLEND);

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_clearFlagLinkedListEnd(VOID)
{
	//clr_flg(flg_id_ime, FLGPTN_IME_LLEND);
	ime_platform_flg_clear(FLGPTN_IME_LLEND);
}
//-------------------------------------------------------------------------------

ER ime_waitFlagLinkedListJobEnd(IME_FLAG_CLEAR_SEL IsClearFlag)
{
	FLGPTN uiFlag;

	if (IsClearFlag == IME_FLAG_CLEAR) {
		//clr_flg(flg_id_ime, FLGPTN_IME_JEND);
		ime_platform_flg_clear(FLGPTN_IME_JEND);
	}

	//wai_flg(&uiFlag, flg_id_ime, FLGPTN_IME_JEND, TWF_ORW | TWF_CLR);
	ime_platform_flg_wait(&uiFlag, FLGPTN_IME_JEND);

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_clearFlagLinkedListJobEnd(VOID)
{
	//clr_flg(flg_id_ime, FLGPTN_IME_JEND);
	ime_platform_flg_clear(FLGPTN_IME_JEND);
}
//-------------------------------------------------------------------------------


ER ime_waitFlagBreakPoint_BP1(IME_FLAG_CLEAR_SEL IsClearFlag)
{
	FLGPTN uiFlag;

	if (IsClearFlag == IME_FLAG_CLEAR) {
		//clr_flg(flg_id_ime, FLGPTN_IME_BP1);
		ime_platform_flg_clear(FLGPTN_IME_BP1);
	}

	//wai_flg(&uiFlag, flg_id_ime, FLGPTN_IME_BP1, TWF_ORW | TWF_CLR);
	ime_platform_flg_wait(&uiFlag, FLGPTN_IME_BP1);

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_clearFlagBreakPoint_BP1(VOID)
{
	//clr_flg(flg_id_ime, FLGPTN_IME_BP1);
	ime_platform_flg_clear(FLGPTN_IME_BP1);
}
//-------------------------------------------------------------------------------

ER ime_waitFlagBreakPoint_BP2(IME_FLAG_CLEAR_SEL IsClearFlag)
{
	FLGPTN uiFlag;

	if (IsClearFlag == IME_FLAG_CLEAR) {
		//clr_flg(flg_id_ime, FLGPTN_IME_BP2);
		ime_platform_flg_clear(FLGPTN_IME_BP2);
	}

	//wai_flg(&uiFlag, flg_id_ime, FLGPTN_IME_BP2, TWF_ORW | TWF_CLR);
	ime_platform_flg_wait(&uiFlag, FLGPTN_IME_BP2);

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_clearFlagBreakPoint_BP2(VOID)
{
	//clr_flg(flg_id_ime, FLGPTN_IME_BP2);
	ime_platform_flg_clear(FLGPTN_IME_BP2);
}
//-------------------------------------------------------------------------------

ER ime_waitFlagBreakPoint_BP3(IME_FLAG_CLEAR_SEL IsClearFlag)
{
	FLGPTN uiFlag;

	if (IsClearFlag == IME_FLAG_CLEAR) {
		//clr_flg(flg_id_ime, FLGPTN_IME_BP3);
		ime_platform_flg_clear(FLGPTN_IME_BP3);
	}

	//wai_flg(&uiFlag, flg_id_ime, FLGPTN_IME_BP3, TWF_ORW | TWF_CLR);
	ime_platform_flg_wait(&uiFlag, FLGPTN_IME_BP3);

	return E_OK;
}
//-------------------------------------------------------------------------------

VOID ime_clearFlagBreakPoint_BP3(VOID)
{
	//clr_flg(flg_id_ime, FLGPTN_IME_BP3);
	ime_platform_flg_clear(FLGPTN_IME_BP3);
}
//-------------------------------------------------------------------------------



ER ime_pause(VOID)
{
	ER er_return = E_OK;

	if (ime_is_open() != TRUE) {
		return E_SYS;
	}

	//if (ime_engine_state_machine_status == IME_ENGINE_PAUSE) {
	//  return E_OK;
	//}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_PAUSE);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d\r\n", (int)IME_ENGINE_RUN);

		return E_SYS;
	}

	//if ((ime_engine_state_machine_status == IME_ENGINE_RUN)) {
	//  ime_set_engine_control(IME_ENGINE_SET_START, DISABLE);
	//}
	//ime_set_engine_control(IME_ENGINE_SET_START, DISABLE);
	ime_start_reg(DISABLE);

#if (IME_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ime_power_saving_en == TRUE) {
		if (ime_engine_operation_mode != IME_OPMODE_SIE_IPP) {
			DBG_IND("ime op mode %d", (int)ime_engine_operation_mode);
			ime_platform_enable_sram_shutdown();
			ime_detach(); //disable engine clock
		}
	}
#endif

	ime_engine_state_machine_status = IME_ENGINE_PAUSE;

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_close(VOID)
{
	ER er_return = E_OK;

	if (ime_is_open() == FALSE) {
		DBG_ERR("IME engine was already closed, can not close again ...\r\n");
		return E_OK;
	} else {
		er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_CLOSE);
		if (er_return != E_OK) {
			DBG_ERR("State Machine Error ...\r\n");
			DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
			DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

			return E_SYS;
		}

		// disable interrupt

		ime_platform_int_disable();
		//drv_disableInt(DRV_INT_IME);

		ime_platform_enable_sram_shutdown();

		// disable engine clock
		ime_detach();
		//ime_platform_unprepare_clk();

		ime_engine_state_machine_status = IME_ENGINE_IDLE;

		ime_isr_fastboot_callback_fp = NULL;
		ime_isr_non_fastboot_callback_fp = NULL;
		ime_isr_get_non_fastboot_callback_fp = NULL;

#if (DRV_SUPPORT_IST == ENABLE)
#else
		ime_p_int_hdl_callback = NULL;
#endif

		// release resource
		er_return = ime_unlock();
		if (er_return != E_OK) {
			return E_SYS;
		}

		return E_OK;
	}
}
//-------------------------------------------------------------------------------

ER ime_reset(VOID)
{
	//ime_set_engine_control(IME_ENGINE_SET_START, IME_FUNC_DISABLE);
	ime_start_reg(IME_FUNC_DISABLE);


	ime_platform_enable_sram_shutdown();
	ime_platform_disable_sram_shutdown();

	ime_detach();
	ime_platform_unprepare_clk();

	/*clk_prepare_enable(ime_clk[0]);*/
	ime_platform_prepare_clk();
	ime_attach();

	//ime_set_engine_control(IME_ENGINE_SET_INTPE, 0x00000000);
	//ime_set_engine_control(IME_ENGINE_SET_INTPS, IME_INTS_ALL);
	ime_set_interrupt_enable_reg(0x00000000);
	ime_set_interrupt_status_reg(IME_INTS_ALL);

	ime_platform_flg_clear(FLGPTN_IME_FRAMEEND);
	ime_platform_flg_clear(FLGPTN_IME_LLEND);
	ime_platform_flg_clear(FLGPTN_IME_JEND);
	ime_platform_flg_clear(FLGPTN_IME_BP1);
	ime_platform_flg_clear(FLGPTN_IME_BP2);
	ime_platform_flg_clear(FLGPTN_IME_BP3);

	// soft-reset
	//ime_set_engine_control(IME_ENGINE_SET_RESET, (UINT32)IME_FUNC_ENABLE);
	//ime_set_engine_control(IME_ENGINE_SET_RESET, (UINT32)IME_FUNC_DISABLE);
	ime_reset_reg(IME_FUNC_ENABLE);
	ime_reset_reg(IME_FUNC_DISABLE);

	//ime_set3DNR_Default();

	ime_engine_state_machine_status = IME_ENGINE_READY;

	return E_OK;
}
//-------------------------------------------------------------------------------



ER ime_chg_stripe_param(IME_HV_STRIPE_INFO *p_stripe_info)
{
	//==============================================================
	//    Note:
	//    when IME is operated in IPP mode,
	//    only horizontal stripe settings are invalid,
	//    overlap and vertical stripe register also must be set.
	//==============================================================

	ER er_return = E_OK;

	IME_H_STRIPE_OVLP_SEL stp_overlap_max = IME_H_ST_OVLP_16P;
	IME_H_STRIPE_OVLP_SEL path_overlap;


	IME_FUNC_EN path_en2 = IME_FUNC_DISABLE, path_en3 = IME_FUNC_DISABLE;//path_en1 = IME_FUNC_DISABLE, , path_en5 = IME_FUNC_DISABLE;
	//IME_FUNC_EN SsrEn = IME_FUNC_DISABLE;
	IME_FUNC_EN lca_subout_en = IME_FUNC_DISABLE;
	IME_FUNC_EN path1_encode_en = IME_FUNC_DISABLE;
	BOOL isd_path2 = FALSE, isd_path3 = FALSE, isd_lca_subout = FALSE; // isd_path1 = FALSE, isd_path5 = FALSE

	//IME_SIZE_INFO InImgSize = {0};
	IME_SCALE_METHOD_SEL path_scl_method = IMEALG_BICUBIC;
	UINT32 isd_cnt = 0, stripe_size = 0, stripe_scale_out_size = 0;

	UINT32 scale_factor_h2 = 0, scale_factor_h3 = 0, scale_factor_h_lca_subout = 0;  //scale_factor_h1 = 0, scale_factor_h5 = 0.0
	//FLOAT base_isd_factor_rate = 0.0;
	//IME_INSRC_SEL get_src = IME_INSRC_DRAM;

	IME_SIZE_INFO get_in_size = {0};
	IME_SIZE_INFO get_out_size = {0};

	if (p_stripe_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return E_SYS;
	}

	//ime_get_in_path_source(&get_src);
	//get_src = ime_get_input_source_reg();
	//isd_path1 = FALSE;
	isd_path2 = FALSE;
	isd_path3 = FALSE;
	//isd_path5 = FALSE;
	isd_lca_subout = FALSE;

	ime_stripe_info.stripe_cal_mode = p_stripe_info->stripe_cal_mode;

	isd_cnt = 0;
	//SsrEn = ime_get_ssr_enable();
	//ime_get_in_path_image_size(&get_in_size);
	ime_get_input_image_size_reg(&(get_in_size.size_h), &(get_in_size.size_v));
	//InImgSize = get_in_size;

	//base_isd_factor_rate = 2.07; //(2112 / 1024)
	//if (SsrEn == ENABLE) {
	//  base_isd_factor_rate = 4.13;  // ((2112 * 2) - 1) / 1024
	//}

#if 0
	ime_get_out_path_enable_p1(&path_en1);
	path_en1 = ime_get_output_path_enable_status_p1_reg();
	if (path_en1 == IME_FUNC_ENABLE) {
		//ime_get_out_path_scale_method_p1(&path_scl_method);
		path_scl_method = ime_get_scaling_method_p1_reg();
		if (path_scl_method == IMEALG_INTEGRATION) {
			isd_cnt += 1;
			isd_path1 = TRUE;
		}
	}
#endif

	//ime_get_out_path_enable_p2(&path_en2);
	path_en2 = (IME_FUNC_EN)ime_get_output_path_enable_status_p2_reg();
	if (path_en2 == IME_FUNC_ENABLE) {
		//ime_get_out_path_scale_method_p2(&path_scl_method);

		path_scl_method = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p2_reg();
		if (path_scl_method == IMEALG_INTEGRATION) {
			isd_cnt += 1;
			isd_path2 = TRUE;
		}
	}

	//ime_get_out_path_enable_p3(&path_en3);
	path_en3 = (IME_FUNC_EN)ime_get_output_path_enable_status_p3_reg();
	if (path_en3 == IME_FUNC_ENABLE) {
		//ime_get_out_path_scale_method_p3(&path_scl_method);

		path_scl_method = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p3_reg();
		if (path_scl_method == IMEALG_INTEGRATION) {
			isd_cnt += 1;
			isd_path3 = TRUE;
		}
	}

#if 0
	ime_get_out_path_enable_p5(&path_en5);
	if (path_en5 == IME_FUNC_ENABLE) {
		ime_get_out_path_scale_method_p5(&path_scl_method);
		if (path_scl_method == IMEALG_INTEGRATION) {
			isd_cnt += 1;
			isd_path5 = TRUE;
		}
	}
#endif

	//lca_subout_en = ime_get_lca_subout_enable();
	lca_subout_en = (IME_FUNC_EN)ime_get_lca_subout_enable_reg();
	if (lca_subout_en == IME_FUNC_ENABLE) {
		isd_cnt += 1;
		isd_lca_subout = TRUE;
	}

	//ime_get_out_path_encoder_enable_p1(&path1_encode_en);
	path1_encode_en = (IME_FUNC_EN)ime_get_encode_enable_p1_reg();

	if (ime_stripe_info.stripe_cal_mode == IME_STRIPE_USER_MODE) {
		ime_stripe_info.overlap_h_sel = p_stripe_info->overlap_h_sel;
		switch (ime_stripe_info.overlap_h_sel) {
		case IME_H_ST_OVLP_16P:
			ime_stripe_info.overlap_h_size = 16;
			break;

		case IME_H_ST_OVLP_24P:
			ime_stripe_info.overlap_h_size = 24;
			break;

		case IME_H_ST_OVLP_32P:
			ime_stripe_info.overlap_h_size = 32;
			break;

		default:
			ime_stripe_info.overlap_h_size = p_stripe_info->overlap_h_size;
			break;
		}

		ime_stripe_info.prt_h_sel = p_stripe_info->prt_h_sel;
		switch (ime_stripe_info.prt_h_sel) {
		case IME_H_ST_PRT_5P:
			ime_stripe_info.prt_h_size = 5;
			break;

		case IME_H_ST_PRT_3P:
			ime_stripe_info.prt_h_size = 3;
			break;

		case IME_H_ST_PRT_2P:
			ime_stripe_info.prt_h_size = 2;
			break;

		case IME_H_ST_PRT_USER:
			ime_stripe_info.prt_h_size = p_stripe_info->prt_h_size;
			break;

		default:
			ime_stripe_info.prt_h_size = (p_stripe_info->overlap_h_size >> 1);
			break;
		}

		ime_stripe_info.stp_size_mode = p_stripe_info->stp_size_mode;

		ime_stripe_info.stp_h.stp_n = p_stripe_info->stp_h.stp_n;
		ime_stripe_info.stp_h.stp_l = p_stripe_info->stp_h.stp_l;
		ime_stripe_info.stp_h.stp_m = p_stripe_info->stp_h.stp_m;

		ime_stripe_info.stp_v.stp_n = p_stripe_info->stp_v.stp_n;
		ime_stripe_info.stp_v.stp_l = p_stripe_info->stp_v.stp_l;
		ime_stripe_info.stp_v.stp_m = p_stripe_info->stp_v.stp_m;

		ime_stripe_info.stp_h.varied_size[0] = p_stripe_info->stp_h.varied_size[0];
		ime_stripe_info.stp_h.varied_size[1] = p_stripe_info->stp_h.varied_size[1];
		ime_stripe_info.stp_h.varied_size[2] = p_stripe_info->stp_h.varied_size[2];
		ime_stripe_info.stp_h.varied_size[3] = p_stripe_info->stp_h.varied_size[3];
		ime_stripe_info.stp_h.varied_size[4] = p_stripe_info->stp_h.varied_size[4];
		ime_stripe_info.stp_h.varied_size[5] = p_stripe_info->stp_h.varied_size[5];
		ime_stripe_info.stp_h.varied_size[6] = p_stripe_info->stp_h.varied_size[6];
		ime_stripe_info.stp_h.varied_size[7] = p_stripe_info->stp_h.varied_size[7];

		//------------------------------------------------------------------
		// for project use
#if (defined(_NVT_EMULATION_) == OFF)
		{
			IME_SIZE_INFO InImgSize = get_in_size;

			if (nvt_get_chip_id() == CHIP_NA51055) {
				switch (ime_engine_operation_mode) {
				case IME_OPMODE_D2D:
					stripe_size = 1344;
					break;

				case IME_OPMODE_SIE_IPP:
					stripe_size = 2688;
					break;

				case IME_OPMODE_IFE_IPP:
					if (lca_subout_en == IME_FUNC_DISABLE) {
						stripe_size = 2688;
					} else {
						if (get_in_size.size_h <= 2688) {
							stripe_size = 2688;
						} else {
							stripe_size = 2640;
						}
					}
					break;

				default:
					stripe_size = 2688;
					break;
				}
			} else if (nvt_get_chip_id() == CHIP_NA51084) {
				switch (ime_engine_operation_mode) {
				case IME_OPMODE_D2D:
					stripe_size = 2048;
					break;

				case IME_OPMODE_SIE_IPP:
				case IME_OPMODE_IFE_IPP:
					stripe_size = 4096;
					break;

				default:
					stripe_size = 4096;
					break;
				}
			} else {
                DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
                return E_PAR;
            }

			ime_cal_hv_strip_param(InImgSize.size_h, InImgSize.size_v, stripe_size, &ime_stripe_info);
		}
#endif
		//------------------------------------------------------------------------------------------------
		// set input path stripe
		//er_return = ime_set_in_path_stripe_info(&ime_stripe_info);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if (ime_stripe_info.stp_h.stp_n > ime_stp_size_max) {
			DBG_ERR("IME: stripe size HN overflow, > %d...\r\n", (int)ime_stp_size_max);

			return E_PAR;
		}

		if (ime_stripe_info.stp_h.stp_l > ime_stp_size_max) {
			DBG_ERR("IME: stripe size HL overflow, > %d...\r\n", (int)ime_stp_size_max);

			return E_PAR;
		}

		if (ime_stripe_info.stp_h.stp_m > ime_stp_num_max) {
			DBG_ERR("IME: stripe number HM overflow, > %d...\r\n", (int)ime_stp_num_max);

			return E_PAR;
		}

		ime_set_stripe_size_mode_reg((UINT32)ime_stripe_info.stp_size_mode);
		ime_set_horizontal_stripe_number_reg(ime_stripe_info.stp_h.stp_m);
		ime_set_horizontal_fixed_stripe_size_reg(ime_stripe_info.stp_h.stp_n, ime_stripe_info.stp_h.stp_l);

		if (ime_stripe_info.stp_size_mode == IME_STRIPE_SIZE_MODE_VARIED) {
			ime_set_horizontal_varied_stripe_size_reg(ime_stripe_info.stp_h.varied_size);
		} else {
			// do nothing...
		}

		ime_set_vertical_stripe_info_reg(ime_stripe_info.stp_v.stp_n, ime_stripe_info.stp_v.stp_l, ime_stripe_info.stp_v.stp_m);
		ime_set_horizontal_overlap_reg((UINT32)ime_stripe_info.overlap_h_sel, ime_stripe_info.overlap_h_size, (UINT32)ime_stripe_info.prt_h_sel, ime_stripe_info.prt_h_size);
		//------------------------------------------------------------------------------------------------


		er_return = ime_check_input_path_stripe();
		if (er_return != E_OK) {
			return er_return;
		}

		ime_load();
		//------------------------------------------------------------------
	} else {
		//
		IME_INSRC_SEL get_in_src;

		//ime_get_in_path_source(&get_in_src);
		get_in_src = ime_get_input_source_reg();

		stp_overlap_max = IME_H_ST_OVLP_16P;

		if (lca_subout_en == IME_FUNC_ENABLE) {
			//ime_get_in_path_image_size(&get_in_size);
			ime_get_input_image_size_reg(&(get_in_size.size_h), &(get_in_size.size_v));

			//ime_get_lca_image_size_info(&get_out_size);
			ime_get_lca_image_size_reg(&(get_out_size.size_h), &(get_out_size.size_v));

			scale_factor_h_lca_subout = (get_in_size.size_h - 1) * 512 / (get_out_size.size_h - 1);

			if (scale_factor_h_lca_subout > 8191) {
				DBG_ERR("IME: scaling down rate of ISD of LCA subout is overflow\r\n");
				DBG_ERR("IME: input width: %d,  lca subout width: %d\r\n", get_in_size.size_h, get_out_size.size_h);
				return E_PAR;
			}

			if (scale_factor_h_lca_subout < 512) {
				DBG_ERR("IME: scaling down rate of ISD of LCA subout is overflow\r\n");
				DBG_ERR("IME: input width: %d,  lca subout width: %d\r\n", get_in_size.size_h, get_out_size.size_h);
				return E_PAR;
			}

			path_overlap = ime_cal_overlap_size(&get_in_size, &get_out_size);
			if (path_overlap >= stp_overlap_max) {
				stp_overlap_max = path_overlap;
			}
		}

		// SSR is removed from NT96510
		//if (SsrEn == IME_FUNC_ENABLE) {
		//  get_in_size.size_h = (get_in_size.size_h << 1) - 1;
		//  get_in_size.size_v = (get_in_size.size_v << 1) - 1;
		//}

#if 0 // output path1 without ISD
		//ime_get_out_path_enable_p1(&path_en1);
		path_en1 = ime_get_output_path_enable_status_p1_reg();
		if (path_en1 == IME_FUNC_ENABLE) {
			//ime_get_out_path_scale_size_p1(&get_out_size);
			ime_get_scaling_size_p1_reg(&(get_out_size.size_h), &(get_out_size.size_v));

			scale_factor_h1 = (get_in_size.size_h - 1) * 512 / (get_out_size.size_h - 1);

			path_overlap = ime_cal_overlap_size(&get_in_size, &get_out_size);
			if (path_overlap >= stp_overlap_max) {
				stp_overlap_max = path_overlap;
			}
		}
#endif


		//ime_get_out_path_enable_p2(&path_en2);
		if (path_en2 == IME_FUNC_ENABLE) {
			//ime_get_out_path_scale_size_p2(&get_out_size);

			ime_get_scaling_size_p2_reg(&(get_out_size.size_h), &(get_out_size.size_v));

			scale_factor_h2 = (get_in_size.size_h - 1) * 512 / (get_out_size.size_h - 1);

			path_overlap = ime_cal_overlap_size(&get_in_size, &get_out_size);
			if (path_overlap >= stp_overlap_max) {
				stp_overlap_max = path_overlap;
			}
		}


		//ime_get_out_path_enable_p3(&path_en3);
		if (path_en3 == IME_FUNC_ENABLE) {
			//ime_get_out_path_scale_size_p3(&get_out_size);

			ime_get_scaling_size_p3_reg(&(get_out_size.size_h), &(get_out_size.size_v));

			scale_factor_h3 = (get_in_size.size_h - 1) * 512 / (get_out_size.size_h - 1);

			path_overlap = ime_cal_overlap_size(&get_in_size, &get_out_size);
			if (path_overlap >= stp_overlap_max) {
				stp_overlap_max = path_overlap;
			}
		}


#if 0
		// path4 without ISD
		ime_get_out_path_enable_p4(&path_en);
		if (path_en == IME_FUNC_ENABLE) {
			//ime_get_out_path_scale_size_p4(&get_out_size);
			ime_get_scaling_size_p4_reg(&(get_out_size.size_h), &(get_out_size.size_v));

			path_overlap = ime_cal_overlap_size(&get_in_size, &get_out_size);
			if (path_overlap >= stp_overlap_max) {
				stp_overlap_max = path_overlap;
			}
		}
#endif

#if 0
		//ime_get_out_path_enable_p5(&path_en5);
		if (path_en5 == IME_FUNC_ENABLE) {
			//ime_get_out_path_scale_method_p5(&path_scl_method);
			//if(path_scl_method == IMEALG_INTEGRATION)
			//{
			//    isd_cnt += 1;
			//    isd_path5 = TRUE;
			//}

			ime_get_out_path_scale_size_p5(&get_out_size);

			scale_factor_h5 = (FLOAT)(get_in_size.size_h - 1) / (FLOAT)(get_out_size.size_h - 1);

			path_overlap = ime_cal_overlap_size(&get_in_size, &get_out_size);
			if (path_overlap >= stp_overlap_max) {
				stp_overlap_max = path_overlap;
			}
		}
#endif



		if (path1_encode_en == IME_FUNC_ENABLE) {
			stp_overlap_max = IME_H_ST_OVLP_USER;
		}

		if (get_in_src == IME_INSRC_DRAM) {
			stp_overlap_max = IME_H_ST_OVLP_32P;
		}

		if (nvt_get_chip_id() == CHIP_NA51055) {
			switch (ime_engine_operation_mode) {
			case IME_OPMODE_D2D:
				stripe_size = 1344;
				break;

			case IME_OPMODE_SIE_IPP:
				stripe_size = 2688;
				break;

			case IME_OPMODE_IFE_IPP:
			case IME_OPMODE_DCE_IPP:
				if (lca_subout_en == IME_FUNC_DISABLE) {
					stripe_size = 2688;
				} else {
					if (get_in_size.size_h <= 2688) {
						stripe_size = 2688;
					} else {
						stripe_size = 2640;
					}
				}
				break;

			default:
				stripe_size = 512;
				break;
			}
		} else if (nvt_get_chip_id() == CHIP_NA51084) {
			switch (ime_engine_operation_mode) {
			case IME_OPMODE_D2D:
				stripe_size = 2048;
				break;

			case IME_OPMODE_SIE_IPP:
			case IME_OPMODE_IFE_IPP:
			case IME_OPMODE_DCE_IPP:
				stripe_size = 4096;
				break;

			default:
				stripe_size = 1024;
				break;
			}
		} else {
            DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
            return E_PAR;
        }

		ime_stripe_info.overlap_h_sel = stp_overlap_max;
		switch (ime_stripe_info.overlap_h_sel) {
		case IME_H_ST_OVLP_16P:
			ime_stripe_info.overlap_h_size = 16;
			ime_stripe_info.prt_h_size = 5;
			ime_stripe_info.prt_h_sel = IME_H_ST_PRT_5P;
			break;

		case IME_H_ST_OVLP_24P:
			ime_stripe_info.overlap_h_size = 24;
			ime_stripe_info.prt_h_size = 5;
			ime_stripe_info.prt_h_sel = IME_H_ST_PRT_5P;
			break;

		case IME_H_ST_OVLP_32P:
			ime_stripe_info.overlap_h_size = 32;
			ime_stripe_info.prt_h_size = 5;
			ime_stripe_info.prt_h_sel = IME_H_ST_PRT_5P;
			break;

		case IME_H_ST_OVLP_USER:
			ime_stripe_info.overlap_h_size = p_stripe_info->overlap_h_size;
			ime_stripe_info.prt_h_size = (p_stripe_info->overlap_h_size >> 1);
			ime_stripe_info.prt_h_sel = IME_H_ST_PRT_USER;
			break;

		default:
			ime_stripe_info.overlap_h_size = 32;
			ime_stripe_info.prt_h_size = 5;
			ime_stripe_info.prt_h_sel = IME_H_ST_PRT_5P;
			break;
		}

		//ime_get_in_path_image_size(&get_in_size);
		ime_get_input_image_size_reg(&(get_in_size.size_h), &(get_in_size.size_v));

		ime_cal_hv_strip_param(get_in_size.size_h, get_in_size.size_v, stripe_size, &ime_stripe_info);

		//if (get_in_src == IME_INSRC_DRAM) {
		if ((ime_engine_operation_mode == IME_OPMODE_D2D) || (ime_engine_operation_mode == IME_OPMODE_SIE_IPP)) {
			if (lca_subout_en == IME_FUNC_ENABLE) {
				if (isd_lca_subout == TRUE) {

					stripe_scale_out_size = ((stripe_size - 1) * 512 / scale_factor_h_lca_subout) + 1;

					if (stripe_scale_out_size > ime_max_stp_isd_out_buf_size) {
						DBG_ERR("IME: scaling down rate of ISD of LCA subout is overflow, please change stripe size...\r\n");
						DBG_ERR("IME: stripe size: %d, lca_h_factor: %d, stripe_scale_out_size: %d\r\n", stripe_size, scale_factor_h_lca_subout, stripe_scale_out_size);
						return E_PAR;
					}
				}
			}

			stripe_size = (((ime_stripe_info.stp_h.stp_m + 1) == 1) ? (ime_stripe_info.stp_h.stp_l << 2) : (ime_stripe_info.stp_h.stp_n << 2));

			// SSR is removed from NT96510
			//stripe_size = ((SsrEn == IME_FUNC_ENABLE) ? ((stripe_size << 1) - 1) : stripe_size);

#if 0 // output path1 without ISD
			if ((path_en1 == IME_FUNC_ENABLE)) {
				if ((isd_path1 == TRUE)) {
					if (scale_factor_h1 > 8191) {
						DBG_ERR("IME: scaling down rate of ISD of path1 is overflow, please use bicubic...\r\n");
						return E_PAR;
					}

					stripe_scale_out_size = ((stripe_size - 1) * 512 / scale_factor_h1) + 1;

					if (stripe_scale_out_size > ime_max_stp_isd_out_buf_size) {
						DBG_ERR("IME: scaling down rate of ISD of path1 is overflow, please use bicubic...\r\n");
						return E_PAR;
					}
				}
			}
#endif

			if ((path_en2 == IME_FUNC_ENABLE)) {
				if ((isd_path2 == TRUE)) {
					if (scale_factor_h2 > 8191) {
						DBG_ERR("IME: scaling down rate of ISD of path2 is overflow, please use bicubic...\r\n");
						return E_PAR;
					}

					stripe_scale_out_size = ((stripe_size - 1) * 512 / scale_factor_h2) + 1;
					if (stripe_scale_out_size > ime_max_stp_isd_out_buf_size) {
						//DBG_ERR("IME: scaling down rate of ISD of path2 is overflow, please use bicubic...\r\n");
						//return E_PAR;
						ime_set_scale_interpolation_method_p2_reg((UINT32)IMEALG_BILINEAR);
					}
				}
			}

			if ((path_en3 == IME_FUNC_ENABLE)) {
				if ((isd_path3 == TRUE)) {
					if (scale_factor_h3 > 8191) {
						DBG_ERR("IME: scaling down rate of ISD of path3 is overflow, please use bicubic...\r\n");
						return E_PAR;
					}

					stripe_scale_out_size = ((stripe_size - 1) * 512 / scale_factor_h3) + 1;
					if (stripe_scale_out_size > ime_max_stp_isd_out_buf_size) {
						//DBG_ERR("IME: scaling down rate of ISD of path3 is overflow, please use bicubic...\r\n");
						//return E_PAR;
						ime_set_scale_interpolation_method_p3_reg((UINT32)IMEALG_BILINEAR);
					}
				}
			}


#if 0
			if ((path_en5 == IME_FUNC_ENABLE)) {
				if ((isd_path5 == TRUE)) {
					if (scale_factor_h5 > 15.99) {
						DBG_ERR("IME: scaling down rate of ISD of path5 is overflow, please use bicubic...\r\n");
						return E_PAR;
					}

					if (scale_factor_h5 < base_isd_factor_rate) {
						stripe_scale_out_size = ((FLOAT)(stripe_size - 1) / scale_factor_h5) + 1.0;

						if (stripe_scale_out_size > ime_max_stp_isd_out_buf_size) {
							DBG_ERR("IME: scaling down rate of ISD of path5 is overflow, please use bicubic...\r\n");
							return E_PAR;
						}
					}
				}
			}
#endif
		}


		//------------------------------------------------------------------------------------------------
		// set input path stripe
		//er_return = ime_set_in_path_stripe_info(&ime_stripe_info);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if (ime_stripe_info.stp_h.stp_n > ime_stp_size_max) {
			DBG_ERR("IME: stripe size HN overflow, > %d...\r\n", (int)ime_stp_size_max);

			return E_PAR;
		}

		if (ime_stripe_info.stp_h.stp_l > ime_stp_size_max) {
			DBG_ERR("IME: stripe size HL overflow, > %d...\r\n", (int)ime_stp_size_max);

			return E_PAR;
		}

		if (ime_stripe_info.stp_h.stp_m > ime_stp_num_max) {
			DBG_ERR("IME: stripe number HM overflow, > %d...\r\n", (int)ime_stp_num_max);

			return E_PAR;
		}

		ime_set_stripe_size_mode_reg((UINT32)ime_stripe_info.stp_size_mode);
		ime_set_horizontal_stripe_number_reg(ime_stripe_info.stp_h.stp_m);
		ime_set_horizontal_fixed_stripe_size_reg(ime_stripe_info.stp_h.stp_n, ime_stripe_info.stp_h.stp_l);

		if (ime_stripe_info.stp_size_mode == IME_STRIPE_SIZE_MODE_VARIED) {
			ime_set_horizontal_varied_stripe_size_reg(ime_stripe_info.stp_h.varied_size);
		} else {
			// do nothing...
		}

		ime_set_vertical_stripe_info_reg(ime_stripe_info.stp_v.stp_n, ime_stripe_info.stp_v.stp_l, ime_stripe_info.stp_v.stp_m);
		ime_set_horizontal_overlap_reg((UINT32)ime_stripe_info.overlap_h_sel, ime_stripe_info.overlap_h_size, (UINT32)ime_stripe_info.prt_h_sel, ime_stripe_info.prt_h_size);
		//------------------------------------------------------------------------------------------------

		er_return = ime_check_input_path_stripe();
		if (er_return != E_OK) {
			return er_return;
		}

		ime_load();
	}

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_cal_d2d_hstripe_size(IME_STRIPE_CAL_INFO *pCalStpInfo, IME_STRIPE_INFO *pStripeH)
{
	IME_H_STRIPE_OVLP_SEL StpOverlapMax = {0};
	UINT32 StripeSize = 0;
	IME_HV_STRIPE_INFO StripeInfo = {0};


	if ((pCalStpInfo == NULL) || (pStripeH == NULL)) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}


	StpOverlapMax = IME_H_ST_OVLP_32P;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		StripeSize = 1344;
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		StripeSize = 2048;
	} else {
        DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }

	StripeInfo.overlap_h_sel = StpOverlapMax;
	switch (StripeInfo.overlap_h_sel) {
	case IME_H_ST_OVLP_16P:
		StripeInfo.overlap_h_size = 16;
		break;

	case IME_H_ST_OVLP_24P:
		StripeInfo.overlap_h_size = 24;
		break;

	case IME_H_ST_OVLP_32P:
		StripeInfo.overlap_h_size = 32;
		break;

	default:
		StripeInfo.overlap_h_size = 32;
		break;
	}

	ime_cal_hv_strip_param(pCalStpInfo->in_size.size_h, pCalStpInfo->in_size.size_v, StripeSize, &StripeInfo);

	pStripeH->stp_n = StripeInfo.stp_h.stp_n << 2;
	pStripeH->stp_l = StripeInfo.stp_h.stp_l << 2;
	pStripeH->stp_m = StripeInfo.stp_h.stp_m + 1;

	return E_OK;
}
//-------------------------------------------------------------------------------

void ime_get_input_path_stripe_info(IME_HV_STRIPE_INFO *pGetStripeInfo)
{
	if (pGetStripeInfo != NULL) {
		//ime_get_in_path_stripe_info(pGetStripeInfo);
		UINT32 uiOverlapSel = 0;

		ime_get_horizontal_stripe_info_reg(&(pGetStripeInfo->stp_h.stp_n), &(pGetStripeInfo->stp_h.stp_l), &(pGetStripeInfo->stp_h.stp_m));
		ime_get_vertical_stripe_info_reg(&(pGetStripeInfo->stp_v.stp_n), &(pGetStripeInfo->stp_v.stp_l), &(pGetStripeInfo->stp_v.stp_m));
		ime_get_horizontal_overlap_reg(&(uiOverlapSel), &(pGetStripeInfo->overlap_h_size));

		pGetStripeInfo->overlap_h_sel = (IME_H_STRIPE_OVLP_SEL)uiOverlapSel;
		pGetStripeInfo->stp_size_mode = (IME_STRIPE_MODE_SEL)ime_get_stripe_mode_reg();

		pGetStripeInfo->stripe_cal_mode = ime_stripe_info.stripe_cal_mode;
	}
}
//-------------------------------------------------------------------------------


ER ime_chg_input_path_param(IME_INPATH_INFO *p_set_in_info)
{
	ER er_return = E_OK;
	IME_FUNC_EN get_en = IME_FUNC_DISABLE;

	if (p_set_in_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return E_SYS;
	}

	er_return = ime_set_in_path(p_set_in_info);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ime_chg_stripe_param(&ime_stripe_info);
	if (er_return != E_OK) {
		return er_return;
	}

	get_en = IME_FUNC_DISABLE;
	//get_en = ime_get_ila_enable();
	get_en = (IME_FUNC_EN)ime_get_lca_enable_reg();
	if (get_en == IME_FUNC_ENABLE) {
		er_return = ime_set_lca_scale_factor();
		if (er_return != E_OK) {
			return er_return;
		}
	}

#if 0
	get_en = IME_FUNC_DISABLE;
#if (defined(_NVT_EMULATION_) == OFF)
	{
		if (g_SsrMode == IME_SSR_MODE_USER) {
			get_en = ime_get_ssr_enable();
		}
	}
#else
	{
		get_en = ime_get_ssr_enable();
	}
#endif

	if (get_en == IME_FUNC_ENABLE) {
#if (defined(_NVT_EMULATION_) == ON)
		{
			er_return = ime_chg_stripe_param(&ime_stripe_info);
			if (er_return != E_OK)
			{
				return er_return;
			}
		}
#else
		{
			ime_stripe_info.stripe_cal_mode = IME_STRIPE_AUTO_MODE;
			er_return = ime_chg_stripe_param(&ime_stripe_info);
			if (er_return != E_OK)
			{
				return er_return;
			}
		}
#endif

		er_return = ime_set_scale_factor_with_ssr();
		if (er_return != E_OK) {
			return er_return;
		}
	}
#endif

	if (ime_engine_state_machine_status == IME_ENGINE_RUN) {
		er_return = ime_check_input_path_stripe();
		if (er_return != E_OK) {
			return er_return;
		}
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_output_path_param(IME_PATH_SEL out_path_sel, IME_OUTPATH_INFO *p_set_out_info)
{
	ER er_return = E_OK;
	//IME_FUNC_EN GetEn = IME_FUNC_DISABLE;

	IME_OUTPUT_IMG_FORMAT_SEL get_user_fmt;

	if (p_set_out_info == NULL) {
		//
		if (out_path_sel == IME_PATH1_SEL) {
			//
			DBG_ERR("IME: path1, setting parameter NULL...\r\n");
		}

		if (out_path_sel == IME_PATH2_SEL) {
			//
			DBG_ERR("IME: path2, setting parameter NULL...\r\n");
		}

		if (out_path_sel == IME_PATH3_SEL) {
			//
			DBG_ERR("IME: path3, setting parameter NULL...\r\n");
		}

		if (out_path_sel == IME_PATH4_SEL) {
			//
			DBG_ERR("IME: path4, setting parameter NULL...\r\n");
		}

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return E_SYS;
	}


	if (out_path_sel == IME_PATH1_SEL) {
		//
		//memset((VOID *)&ime_set_output_path1_info, 0, sizeof(ime_set_output_path1_info));
		//ime_set_output_path1_info = *p_set_out_info;

		//if (ime_set_output_path1_info.out_path_enable == IME_FUNC_ENABLE) {
		//  get_user_fmt = ime_set_output_path1_info.out_path_image_format.out_format_sel;
		//  //if (get_user_fmt == IME_OUTPUT_RGB_444) {
		//  //  ime_set_output_path1_info.out_path_image_format.out_format_sel = IME_OUTPUT_YCC_444;
		//  //}
		//}

		//er_return = ime_set_out_path_p1(&ime_set_output_path1_info);
		er_return = ime_set_out_path_p1(p_set_out_info);
		if (er_return != E_OK) {
			return er_return;
		}
	} else if (out_path_sel == IME_PATH2_SEL) {
		//
		//memset((VOID *)&ime_set_output_path2_info, 0, sizeof(ime_set_output_path2_info));
		//ime_set_output_path2_info = *p_set_out_info;

		//if (ime_set_output_path2_info.out_path_enable == IME_FUNC_ENABLE) {
		//  get_user_fmt = ime_set_output_path2_info.out_path_image_format.out_format_sel;
		//  //if ((get_user_fmt == IME_OUTPUT_YCC_444) || (get_user_fmt == IME_OUTPUT_RGB_444)) {
		//  //  DBG_ERR("IME: path2, output format error...\r\n");
		//  //  return E_PAR;
		//  //}
		//}

		//er_return = ime_set_out_path_p2(&ime_set_output_path2_info);
		er_return = ime_set_out_path_p2(p_set_out_info);
		if (er_return != E_OK) {
			return er_return;
		}
	} else if (out_path_sel == IME_PATH3_SEL) {
		//
		//memset((VOID *)&ime_set_output_path3_info, 0, sizeof(ime_set_output_path3_info));
		//ime_set_output_path3_info = *p_set_out_info;

		//if (ime_set_output_path3_info.out_path_enable == IME_FUNC_ENABLE) {
		//  get_user_fmt = ime_set_output_path3_info.out_path_image_format.out_format_sel;
		//  //if ((get_user_fmt == IME_OUTPUT_YCC_444) || (get_user_fmt == IME_OUTPUT_RGB_444)) {
		//  //  DBG_ERR("IME: path3, output format error...\r\n");
		//  //  return E_PAR;
		//  //}
		//}

		//er_return = ime_set_out_path_p3(&ime_set_output_path3_info);
		er_return = ime_set_out_path_p3(p_set_out_info);
		if (er_return != E_OK) {
			return er_return;
		}
	} else if (out_path_sel == IME_PATH4_SEL) {
		//
		//memset((VOID *)&ime_set_output_path4_info, 0, sizeof(ime_set_output_path4_info));
		//ime_set_output_path4_info = *p_set_out_info;

		//if (ime_set_output_path4_info.out_path_enable == IME_FUNC_ENABLE) {
		//  get_user_fmt = ime_set_output_path4_info.out_path_image_format.out_format_sel;
		//  if (get_user_fmt != IME_OUTPUT_Y_ONLY) {
		//      DBG_ERR("IME: path4, output format error...\r\n");
		//      return E_PAR;
		//  }
		//}

		if (p_set_out_info->out_path_enable == IME_FUNC_ENABLE) {
			get_user_fmt = p_set_out_info->out_path_image_format.out_format_sel;
			if (get_user_fmt != IME_OUTPUT_Y_ONLY) {
				DBG_ERR("IME: path4, output format error...\r\n");
				return E_PAR;
			}
		}

		//er_return = ime_set_out_path_p4(&ime_set_output_path4_info);
		er_return = ime_set_out_path_p4(p_set_out_info);
		if (er_return != E_OK) {
			return er_return;
		}
	}
#if 0
	else if (out_path_sel == IME_PATH5_SEL) {
		//
		memset((VOID *)&ime_set_output_path5_info, 0, sizeof(ime_set_output_path5_info));
		ime_set_output_path5_info = *p_set_out_info;

		if (ime_set_output_path5_info.out_path_enable == IME_FUNC_ENABLE) {
			get_user_fmt = ime_set_output_path5_info.out_path_image_format.out_format_sel;
			if ((get_user_fmt == IME_OUTPUT_YCC_444) || (get_user_fmt == IME_OUTPUT_RGB_444)) {
				DBG_ERR("IME: path5, output format error...\r\n");
				return E_PAR;
			}
		}

		er_return = ime_set_out_path_p5(&ime_set_output_path5_info);
		if (er_return != E_OK) {
			return er_return;
		}
	}
#endif
	else {
		DBG_ERR("IME: path selection error...\r\n");

		return E_PAR;
	}

	er_return = ime_chg_stripe_param(&ime_stripe_info);
	if (er_return != E_OK) {
		return er_return;
	}

	//GetEn = IME_FUNC_DISABLE;
#if 0
#if (defined(_NVT_EMULATION_) == OFF)
	{
		if (g_SsrMode == IME_SSR_MODE_USER) {
			GetEn = ime_get_ssr_enable();
		}
	}
#else
	{
		GetEn = ime_get_ssr_enable();
	}
#endif

	if (GetEn == IME_FUNC_ENABLE) {
#if (defined(_NVT_EMULATION_) == ON)
		{
			er_return = ime_chg_stripe_param(&ime_stripe_info);
			if (er_return != E_OK)
			{
				return er_return;
			}
		}
#else
		{
			ime_stripe_info.stripe_cal_mode = IME_STRIPE_AUTO_MODE;
			er_return = ime_chg_stripe_param(&ime_stripe_info);
			if (er_return != E_OK)
			{
				return er_return;
			}
		}
#endif

		er_return = ime_set_scale_factor_with_ssr();
		if (er_return != E_OK) {
			return er_return;
		}
	}
#endif

	if (ime_engine_state_machine_status == IME_ENGINE_RUN) {
		er_return = ime_check_input_path_stripe();
		if (er_return != E_OK) {
			return er_return;
		}
	}

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------



/**
    Get Engine Information of IME

    This API provides users to get the in operating information of IME engine

    @param sel Select to get which information. \n
               Please refer to IME_GET_INFO_SEL.

    @return UINT32 Corresponding desired information.
*/
UINT32 ime_get_engine_info(IME_GET_INFO_SEL get_ime_info_sel)
{
	UINT32 get_val = 0x0;

	switch (get_ime_info_sel) {
	case IME_GET_INPUT_SRC:
		//ime_get_in_path_source((IME_INSRC_SEL *)&get_val);
		get_val = ime_get_input_source_reg();
		break;
	case IME_GET_INT_ENABLE:
		//get_val = ime_get_engine_control(IME_ENGINE_GET_INTPE);
		get_val = ime_get_interrupt_enable_reg();
		break;

	case IME_GET_INT_STATUS:
		//get_val = ime_get_engine_control(IME_ENGINE_GET_INTP);
		get_val = ime_get_interrupt_status_reg();
		break;

	case IME_GET_LOCKED_STATUS:
		get_val = ime_eng_lock_status;
		break;

	case IME_GET_LCA_ENABLE:
		//get_val = (UINT32)ime_get_ila_enable();
		get_val = (UINT32)ime_get_lca_enable_reg();
		break;

	case IME_GET_LCA_SUBOUT_ENABLE:
		//get_val = ime_get_lca_subout_enable();
		get_val = ime_get_lca_subout_enable_reg();
		break;

	case IME_GET_P1_ENABLE_STATUS:
		//ime_get_out_path_enable_p1((IME_FUNC_EN *)&get_val);
		get_val = ime_get_output_path_enable_status_p1_reg();
		break;

	case IME_GET_P1_OUTPUT_FORMAT:
		//ime_get_out_path_image_format_p1((IME_OUTPUT_IMG_FORMAT_SEL *)&get_val);
		get_val = ime_get_output_format_p1_reg();
		break;

	case IME_GET_P1_OUTPUT_TYPE:
		//ime_get_out_path_format_type_p1((IME_OUTPUT_FORMAT_TYPE *)&get_val);
		get_val = ime_get_output_format_type_p1_reg();
		break;

	case IME_GET_P1_SPRTOUT_ENABLE:
		//ime_get_out_path_sprt_out_enable_p1((IME_FUNC_EN *)&get_val);
		get_val = (IME_FUNC_EN)ime_get_stitching_enable_p1_reg();
		break;


	case IME_GET_P2_ENABLE_STATUS:
		//ime_get_out_path_enable_p2((IME_FUNC_EN *)&get_val);
		get_val = (IME_FUNC_EN)ime_get_output_path_enable_status_p2_reg();
		break;

	case IME_GET_P2_OUTPUT_FORMAT:
		//ime_get_out_path_image_format_p2((IME_OUTPUT_IMG_FORMAT_SEL *)&get_val);
		get_val = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_p2_reg();
		break;

	case IME_GET_P2_OUTPUT_TYPE:
		//ime_get_out_path_format_type_p2((IME_OUTPUT_FORMAT_TYPE *)&get_val);
		get_val = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_type_p2_reg();
		break;

	case IME_GET_P2_SPRTOUT_ENABLE:
		//ime_get_out_path_sprt_out_enable_p2((IME_FUNC_EN *)&get_val);
		get_val = (IME_FUNC_EN)ime_get_stitching_enable_p2_reg();
		break;

	case IME_GET_P3_ENABLE_STATUS:
		//ime_get_out_path_enable_p3((IME_FUNC_EN *)&get_val);
		get_val = (IME_FUNC_EN)ime_get_output_path_enable_status_p3_reg();
		break;

	case IME_GET_P3_OUTPUT_FORMAT:
		//ime_get_out_path_image_format_p3((IME_OUTPUT_IMG_FORMAT_SEL *)&get_val);
		get_val = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_p3_reg();
		break;

	case IME_GET_P3_OUTPUT_TYPE:
		//ime_get_out_path_format_type_p3((IME_OUTPUT_FORMAT_TYPE *)&get_val);
		get_val = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_type_p3_reg();
		break;

	case IME_GET_P3_SPRTOUT_ENABLE:
		//ime_get_out_path_sprt_out_enable_p3((IME_FUNC_EN *)&get_val);
		get_val = (IME_FUNC_EN)ime_get_stitching_enable_p3_reg();
		break;

	case IME_GET_P4_ENABLE_STATUS:
		//ime_get_out_path_enable_p4((IME_FUNC_EN *)&get_val);
		get_val = (IME_FUNC_EN)ime_get_output_path_enable_status_p4_reg();
		break;

	case IME_GET_P4_OUTPUT_FORMAT:
		//ime_get_out_path_image_format_p4((IME_OUTPUT_IMG_FORMAT_SEL *)&get_val);
		get_val = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_p4_reg();
		break;

	case IME_GET_P4_OUTPUT_TYPE:
		//ime_get_out_path_format_type_p4((IME_OUTPUT_FORMAT_TYPE *)&get_val);
		get_val = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_type_p4_reg();
		break;

#if 0
	case IME_GET_P5_ENABLE_STATUS:
		ime_get_out_path_enable_p5((IME_FUNC_EN *)&get_val);
		break;

	case IME_GET_P5_OUTPUT_FORMAT:
		ime_get_out_path_image_format_p5((IME_OUTPUT_IMG_FORMAT_SEL *)&get_val);
		break;

	case IME_GET_P5_OUTPUT_TYPE:
		ime_get_out_path_format_type_p5((IME_OUTPUT_FORMAT_TYPE *)&get_val);
		break;

	case IME_GET_P5_SPRTOUT_ENABLE:
		ime_get_out_path_sprt_out_enable_p5((IME_FUNC_EN *)&get_val);
		break;
#endif

	case IME_GET_DEBUG_MESSAGE:
		//ime_get_debug_message();
		get_val = 0;
		break;

	case IME_GET_FUNC_STATUS0:
		//get_val = ime_get_function0_enable();
		get_val = ime_get_function0_enable_reg();
		break;

	case IME_GET_FUNC_STATUS1:
		//get_val = ime_get_function1_enable();
		get_val = ime_get_function1_enable_reg();
		break;

	default:
		break;
	}

	return get_val;
}
//-------------------------------------------------------------------------------

VOID ime_get_input_path_image_size_info(IME_SIZE_INFO *p_get_size)
{
	if (p_get_size != NULL) {
		//ime_get_in_path_image_size(p_get_size);
		ime_get_input_image_size_reg(&(p_get_size->size_h), &(p_get_size->size_v));
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_input_path_lineoffset_info(IME_LINEOFFSET_INFO *p_get_lofs)
{
	if (p_get_lofs != NULL) {
		//ime_get_in_path_lineoffset(p_get_lofs);
		p_get_lofs->lineoffset_y = ime_get_input_lineoffset_y_reg();
		p_get_lofs->lineoffset_cb = ime_get_input_lineoffset_c_reg();
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_input_path_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr)
{
	if (p_get_addr != NULL) {
		//ime_get_in_path_dma_addr(p_get_addr);
		ime_get_input_channel_addr1_reg(&(p_get_addr->addr_y), &(p_get_addr->addr_cb), &(p_get_addr->addr_cr));

#if (IME_DMA_CACHE_HANDLE == 1)
		//p_get_addr->addr_y = dma_getNonCacheAddr(p_get_addr->addr_y);
		//p_get_addr->addr_cb = dma_getNonCacheAddr(p_get_addr->addr_cb);
		//p_get_addr->addr_cr = dma_getNonCacheAddr(p_get_addr->addr_cr);
#endif
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_output_path_image_size_info(IME_PATH_SEL path_sel, IME_SIZE_INFO *p_get_size)
{
	if (p_get_size != NULL) {
		switch (path_sel) {
		case IME_PATH1_SEL:
			//ime_get_out_path_output_size_p1(p_get_size);
			ime_get_output_size_p1_reg(&(p_get_size->size_h), &(p_get_size->size_v));
			break;

		case IME_PATH2_SEL:
			//ime_get_out_path_output_size_p2(p_get_size);
			ime_get_output_size_p2_reg(&(p_get_size->size_h), &(p_get_size->size_v));
			break;

		case IME_PATH3_SEL:
			//ime_get_out_path_output_size_p3(p_get_size);
			ime_get_output_size_p3_reg(&(p_get_size->size_h), &(p_get_size->size_v));
			break;

		case IME_PATH4_SEL:
			//ime_get_out_path_output_size_p4(p_get_size);
			ime_get_output_size_p4_reg(&(p_get_size->size_h), &(p_get_size->size_v));
			break;

		//case IME_PATH5_SEL:
		//  ime_get_out_path_output_size_p5(p_get_size);
		//  ime_get_output_size_p5_reg(&(p_get_size->size_h), &(p_get_size->size_v));
		//  break;

		default:
			break;
		}
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_output_path_lineoffset_info(IME_PATH_SEL path_sel, IME_PATH_OUTBUF_SEL buf_sel, IME_LINEOFFSET_INFO *p_get_lofs)
{
	if (p_get_lofs != NULL) {
		if (buf_sel == IME_PATH_OUTBUF_SET0) {
			switch (path_sel) {
			case IME_PATH1_SEL:
				//ime_get_out_path_lineoffset_p1(p_get_lofs);
				p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p1_reg();
				p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p1_reg();
				break;

			case IME_PATH2_SEL:
				//ime_get_out_path_lineoffset_p2(p_get_lofs);
				p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p2_reg();
				p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p2_reg();
				break;

			case IME_PATH3_SEL:
				//ime_get_out_path_lineoffset_p3(p_get_lofs);
				p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p3_reg();
				p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p3_reg();
				break;

			case IME_PATH4_SEL:
				//ime_get_out_path_lineoffset_p4(p_get_lofs);
				p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p4_reg();
				p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p4_reg();
				break;

			//case IME_PATH5_SEL:
			//  ime_get_out_path_lineoffset_p5(p_get_lofs);
			//  p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p5_reg();
			//  p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p5_reg();
			//  break;

			default:
				break;
			}
		} else if (buf_sel == IME_PATH_OUTBUF_SET1) {
			switch (path_sel) {
			case IME_PATH1_SEL:
				//ime_get_out_path_lineoffset2_p1(p_get_lofs);
				p_get_lofs->lineoffset_y = ime_get_stitching_output_lineoffset_y2_p1_reg();
				p_get_lofs->lineoffset_cb = ime_get_stitching_output_lineoffset_c2_p1_reg();
				break;

			case IME_PATH2_SEL:
				//ime_get_out_path_lineoffset2_p2(p_get_lofs);
				p_get_lofs->lineoffset_y = ime_get_stitching_output_lineoffset_y2_p2_reg();
				p_get_lofs->lineoffset_cb = ime_get_stitching_output_lineoffset_c2_p2_reg();
				break;

			case IME_PATH3_SEL:
				//ime_get_out_path_lineoffset2_p3(p_get_lofs);
				p_get_lofs->lineoffset_y = ime_get_stitching_output_lineoffset_y2_p3_reg();
				p_get_lofs->lineoffset_cb = ime_get_stitching_output_lineoffset_c2_p3_reg();
				break;

			case IME_PATH4_SEL:
				DBG_DUMP("IME: path4, only SET0...\r\n");
				break;

			//case IME_PATH5_SEL:
			//    ime_get_out_path_lineoffset2_p5(p_get_lofs);
			//break;

			default:
				break;
			}
		}
	}
}
//-------------------------------------------------------------------------------

VOID ime_get_output_path_dma_addr_info(IME_PATH_SEL path_sel, IME_PATH_OUTBUF_SEL buf_sel, IME_DMA_ADDR_INFO *p_get_addr)
{
	if (p_get_addr != NULL) {
		if (buf_sel == IME_PATH_OUTBUF_SET0) {
			switch (path_sel) {
			case IME_PATH1_SEL:
				//ime_get_out_path_dma_addr_p1(p_get_addr);
				p_get_addr->addr_y = ime_get_output_addr_y_p1_reg();
				p_get_addr->addr_cb = ime_get_output_addr_cb_p1_reg();
				p_get_addr->addr_cr = ime_get_output_addr_cr_p1_reg();
				break;

			case IME_PATH2_SEL:
				//ime_get_out_path_dma_addr_p2(p_get_addr);
				p_get_addr->addr_y = ime_get_output_addr_y_p2_reg();
				p_get_addr->addr_cb = ime_get_output_addr_cb_p2_reg();
				p_get_addr->addr_cr = ime_get_output_addr_cb_p2_reg();
				break;

			case IME_PATH3_SEL:
				//ime_get_out_path_dma_addr_p3(p_get_addr);
				p_get_addr->addr_y = ime_get_output_addr_y_p3_reg();
				p_get_addr->addr_cb = ime_get_output_addr_cb_p3_reg();
				p_get_addr->addr_cr = ime_get_output_addr_cb_p3_reg();
				break;

			case IME_PATH4_SEL:
				//ime_get_out_path_dma_addr_p4(p_get_addr);
				p_get_addr->addr_y = ime_get_output_addr_y_p4_reg();
				p_get_addr->addr_cb = ime_get_output_addr_cb_p4_reg();
				p_get_addr->addr_cr = ime_get_output_addr_cb_p4_reg();
				break;

			//case IME_PATH5_SEL:
			//  ime_get_out_path_dma_addr_p5(p_get_addr);
			//  p_get_addr->addr_y = ime_get_output_addr_y_p5_reg();
			//  p_get_addr->addr_cb = ime_get_output_addr_cb_p5_reg();
			//  p_get_addr->addr_cr = ime_get_output_addr_cb_p5_reg();
			//  break;

			default:
				break;
			}
		} else if (buf_sel == IME_PATH_OUTBUF_SET1) {
			switch (path_sel) {
			case IME_PATH1_SEL:
				//ime_get_out_path_dma_addr2_p1(p_get_addr);
				p_get_addr->addr_y = ime_get_stitching_output_addr_y2_p1_reg();
				p_get_addr->addr_cb = ime_get_stitching_output_addr_cb2_p1_reg();
				p_get_addr->addr_cr = ime_get_stitching_output_addr_cb2_p1_reg();
				break;

			case IME_PATH2_SEL:
				//ime_get_out_path_dma_addr2_p2(p_get_addr);
				p_get_addr->addr_y = ime_get_stitching_output_addr_y2_p2_reg();
				p_get_addr->addr_cb = ime_get_stitching_output_addr_cb2_p2_reg();
				p_get_addr->addr_cr = ime_get_stitching_output_addr_cb2_p2_reg();
				break;

			case IME_PATH3_SEL:
				//ime_get_out_path_dma_addr2_p3(p_get_addr);
				p_get_addr->addr_y = ime_get_stitching_output_addr_y2_p3_reg();
				p_get_addr->addr_cb = ime_get_stitching_output_addr_cb2_p3_reg();
				p_get_addr->addr_cr = ime_get_stitching_output_addr_cb2_p3_reg();
				break;

			case IME_PATH4_SEL:
				DBG_DUMP("IME: path4, only SET0...\r\n");
				break;

			//case IME_PATH5_SEL:
			//  ime_get_out_path_dma_addr2_p5(p_get_addr);
			//  p_get_addr->addr_y = ime_get_stitching_output_addr_y2_p5_reg();
			//  p_get_addr->addr_cb = ime_get_stitching_output_addr_cb2_p5_reg();
			//  p_get_addr->addr_cr = ime_get_stitching_output_addr_cb2_p5_reg();
			//  break;

			default:
				break;
			}
		}


#if (IME_DMA_CACHE_HANDLE == 1)
		//p_get_addr->addr_y = dma_getNonCacheAddr(p_get_addr->addr_y);
		//p_get_addr->addr_cb = dma_getNonCacheAddr(p_get_addr->addr_cb);
		//p_get_addr->addr_cr = dma_getNonCacheAddr(p_get_addr->addr_cr);
#endif
	}
}
//-------------------------------------------------------------------------------

ER ime_chg_statistic_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;
	IME_FUNC_EN get_path4_en;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	// #2015/06/12# adas control flow bug
	//ime_get_out_path_enable_p4(&get_path4_en);
	get_path4_en = (IME_FUNC_EN)ime_get_output_path_enable_status_p4_reg();
	if ((get_path4_en == IME_FUNC_DISABLE) && (set_en == IME_FUNC_ENABLE)) {
		DBG_ERR("IME: output path4 must be enabled first...\r\n");
		return E_PAR;
	}
	// #2015/06/12# end
	//ime_set_adas_enable(set_en);
	ime_set_adas_enable_reg(set_en);


#if 0 // removed from nt96680
	if (set_en == IME_FUNC_ENABLE) {
		er_return = ime_check_stripe_mode_statistical();
		if (er_return != E_OK) {
			DBG_ERR("IME: statistic info. only for single stripe mode...\r\n");
			return E_PAR;
		}
	}
#endif

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_statistic_filter_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_adas_median_filter_enable(set_en);
	ime_set_adas_median_filter_enable_reg(set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_statistical_image_output_type(IME_STL_IMGOUT_SEL img_out_sel)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_adas_image_out_sel(img_out_sel);
	ime_set_adas_after_filter_out_sel_reg((UINT32)img_out_sel);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_statistic_edge_kernel_param(IME_STL_EDGE_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: stl., edge kernal setting parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//er_return = ime_set_adas_edge_kernel(p_set_info);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	if ((p_set_info->stl_sft0 > 15) || (p_set_info->stl_sft1 > 15)) {
		DBG_ERR("IME: stl., edge kernal parameter overflow...\r\n");

		return E_PAR;
	}

	ime_set_adas_edge_kernel_enable_reg((UINT32)p_set_info->stl_edge_ker0_enable, (UINT32)p_set_info->stl_edge_ker1_enable);
	ime_set_adas_edge_kernel_sel_reg((UINT32)p_set_info->stl_edge_ker0, (UINT32)p_set_info->stl_edge_ker1);
	ime_set_adas_edge_kernel_shift_reg(p_set_info->stl_sft0, p_set_info->stl_sft1);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_chg_statistic_histogram_param(IME_STL_HIST_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: stl., histogram setting parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_stl_hist_info = *p_set_info;

	//er_return = ime_set_adas_histogram(&ime_set_stl_hist_info);
	//er_return = ime_set_adas_histogram(p_set_info);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	if ((p_set_info->stl_hist_pos.pos_x > 65535) || (p_set_info->stl_hist_pos.pos_y > 65535)) {
		DBG_ERR("IME: stl., histogram position parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_hist_size.size_h > 65535) || (p_set_info->stl_hist_size.size_v > 65535)) {
		DBG_ERR("IME: stl., histogram size parameter overflow...\r\n");

		return E_PAR;
	}

	if (p_set_info->stl_set_sel == IME_STL_HIST_SET0) {
		ime_set_adas_histogram_set0_reg(p_set_info->stl_hist_pos.pos_x, p_set_info->stl_hist_pos.pos_y, p_set_info->stl_hist_size.size_h, p_set_info->stl_hist_size.size_v);
		ime_set_adas_histogram_acc_target_set0_reg(p_set_info->stl_hist_acc_tag.acc_tag);
	} else if (p_set_info->stl_set_sel == IME_STL_HIST_SET1) {
		ime_set_adas_histogram_set1_reg(p_set_info->stl_hist_pos.pos_x, p_set_info->stl_hist_pos.pos_y, p_set_info->stl_hist_size.size_h, p_set_info->stl_hist_size.size_v);
		ime_set_adas_histogram_acc_target_set1_reg(p_set_info->stl_hist_acc_tag.acc_tag);
	}

#if 0
	er_return = ime_check_stripe_mode_statistical();
	if (er_return != E_OK) {
		DBG_ERR("IME: statistic info. only for single stripe mode...\r\n");
		return E_PAR;
	}
#endif

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_statistic_edge_map_lineoffset(UINT32 stp_lofs)
{
	ER er_return = E_OK;


	if (stp_lofs == 0x0) {
		DBG_ERR("IME: stl., edge map lineoffset = 0x0...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_adas_edge_map_lofs(stp_lofs);

	ime_set_adas_edge_map_dam_lineoffset_reg(stp_lofs);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_statistic_edge_map_addr(UINT32 addr)
{
	ER er_return = E_OK;

	UINT32 flush_addr, flush_size = 0, stp_lofs = 0;
	IME_SIZE_INFO get_out_size = {0};

	if (addr == 0x0) {
		DBG_ERR("IME: stl., edge map DMA address = 0x0...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//stp_lofs = ime_get_adas_edge_map_lofs();
	stp_lofs = ime_get_adas_edge_map_dam_lineoffset_reg();

	//ime_get_out_path_output_size_p4(&get_out_size);
	ime_get_output_size_p4_reg(&(get_out_size.size_h), &(get_out_size.size_v));

	ime_engine_operation_mode = ime_get_operate_mode();

	flush_size = (stp_lofs * get_out_size.size_v);
	flush_size = IME_ALIGN_CEIL_32(flush_size);
	flush_addr = addr;
	flush_addr = ime_output_dma_buf_flush(flush_addr, flush_size, FALSE, IME_DO_BUF_FLUSH, ime_engine_operation_mode);
	//er_return = ime_set_adas_edge_map_addr(tmp_addr);
	//if (er_return != E_OK) {
	//  return E_PAR;
	//}

	if (ime_engine_operation_mode == IME_OPMODE_D2D) {
		ime_out_buf_flush[IME_OUT_P4_E].flush_en = ENABLE;
		ime_out_buf_flush[IME_OUT_P4_E].buf_addr = flush_addr;
		ime_out_buf_flush[IME_OUT_P4_E].buf_size = (stp_lofs * get_out_size.size_v);
	}

	ime_set_adas_edge_map_dam_addr_reg(flush_addr);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_chg_statistic_histogram_addr(UINT32 hist_addr)
{
	ER er_return = E_OK;
	UINT32 tmp_addr = 0x0, hist_out_size = 0;

	if (hist_addr == 0x0) {
		DBG_ERR("IME: stl., histogram DMA address = 0x0...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_engine_operation_mode = ime_get_operate_mode();

	hist_out_size = 128 * 4 * 2;
	hist_out_size = IME_ALIGN_CEIL_32(hist_out_size);
	tmp_addr = ime_output_dma_buf_flush(hist_addr, hist_out_size, TRUE, IME_DO_BUF_FLUSH, ime_engine_operation_mode);

	//er_return = ime_set_adas_histogram_addr(tmp_addr);
	//if (er_return != E_OK) {
	//  return E_PAR;
	//}

	if (ime_engine_operation_mode == IME_OPMODE_D2D) {
		ime_out_buf_flush[IME_OUT_P4_H].flush_en = ENABLE;
		ime_out_buf_flush[IME_OUT_P4_H].buf_addr = tmp_addr;
		ime_out_buf_flush[IME_OUT_P4_H].buf_size = hist_out_size;
	}

	ime_set_adas_histogram_dam_addr_reg(tmp_addr);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_chg_statistic_edge_roi_param(IME_STL_ROI_INFO *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		DBG_ERR("IME: stl., ROI setting parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_stl_roi_info = *p_set_info;

	//er_return = ime_set_adas_roi(&ime_set_stl_roi_info);
	//er_return = ime_set_adas_roi(p_set_info);
	//if (er_return != E_OK) {
	//  return er_return;
	//}


	if ((p_set_info->stl_roi0.stl_roi_th0 > 2047) || (p_set_info->stl_roi0.stl_roi_th1 > 2047) || (p_set_info->stl_roi0.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI0 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_roi1.stl_roi_th0 > 2047) || (p_set_info->stl_roi1.stl_roi_th1 > 2047) || (p_set_info->stl_roi1.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI1 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_roi2.stl_roi_th0 > 2047) || (p_set_info->stl_roi2.stl_roi_th1 > 2047) || (p_set_info->stl_roi2.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI2 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_roi3.stl_roi_th0 > 2047) || (p_set_info->stl_roi3.stl_roi_th1 > 2047) || (p_set_info->stl_roi3.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI3 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_roi4.stl_roi_th0 > 2047) || (p_set_info->stl_roi4.stl_roi_th1 > 2047) || (p_set_info->stl_roi4.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI4 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_roi5.stl_roi_th0 > 2047) || (p_set_info->stl_roi5.stl_roi_th1 > 2047) || (p_set_info->stl_roi5.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI5 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_roi6.stl_roi_th0 > 2047) || (p_set_info->stl_roi6.stl_roi_th1 > 2047) || (p_set_info->stl_roi6.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI6 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_roi7.stl_roi_th0 > 2047) || (p_set_info->stl_roi7.stl_roi_th1 > 2047) || (p_set_info->stl_roi7.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI7 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_info->stl_row_pos.stl_row0 > 1080) || (p_set_info->stl_row_pos.stl_row1 > 1080)) {
		DBG_ERR("IME: stl., ROW position parameter overflow...\r\n");

		return E_PAR;
	}

	ime_set_adas_row_position_reg(p_set_info->stl_row_pos.stl_row0, p_set_info->stl_row_pos.stl_row1);

	ime_set_adas_roi_threshold0_reg(p_set_info->stl_roi0.stl_roi_th0, p_set_info->stl_roi0.stl_roi_th1, p_set_info->stl_roi0.stl_roi_th2, p_set_info->stl_roi0.stl_roi_src);
	ime_set_adas_roi_threshold1_reg(p_set_info->stl_roi1.stl_roi_th0, p_set_info->stl_roi1.stl_roi_th1, p_set_info->stl_roi1.stl_roi_th2, p_set_info->stl_roi1.stl_roi_src);
	ime_set_adas_roi_threshold2_reg(p_set_info->stl_roi2.stl_roi_th0, p_set_info->stl_roi2.stl_roi_th1, p_set_info->stl_roi2.stl_roi_th2, p_set_info->stl_roi2.stl_roi_src);
	ime_set_adas_roi_threshold3_reg(p_set_info->stl_roi3.stl_roi_th0, p_set_info->stl_roi3.stl_roi_th1, p_set_info->stl_roi3.stl_roi_th2, p_set_info->stl_roi3.stl_roi_src);
	ime_set_adas_roi_threshold4_reg(p_set_info->stl_roi4.stl_roi_th0, p_set_info->stl_roi4.stl_roi_th1, p_set_info->stl_roi4.stl_roi_th2, p_set_info->stl_roi4.stl_roi_src);
	ime_set_adas_roi_threshold5_reg(p_set_info->stl_roi5.stl_roi_th0, p_set_info->stl_roi5.stl_roi_th1, p_set_info->stl_roi5.stl_roi_th2, p_set_info->stl_roi5.stl_roi_src);
	ime_set_adas_roi_threshold6_reg(p_set_info->stl_roi6.stl_roi_th0, p_set_info->stl_roi6.stl_roi_th1, p_set_info->stl_roi6.stl_roi_th2, p_set_info->stl_roi6.stl_roi_src);
	ime_set_adas_roi_threshold7_reg(p_set_info->stl_roi7.stl_roi_th0, p_set_info->stl_roi7.stl_roi_th1, p_set_info->stl_roi7.stl_roi_th2, p_set_info->stl_roi7.stl_roi_src);

#if 0
	er_return = ime_check_stripe_mode_statistical();
	if (er_return != E_OK) {
		DBG_ERR("IME: statistic info. only for single stripe mode...\r\n");
		return E_PAR;
	}
#endif

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_statistic_edge_flip_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// check state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_adas_flip_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------


VOID ime_get_statistic_info(IME_GET_STL_INFO *p_get_hist_info)
{
	//ime_get_adas_hitogram_info(p_get_hist_info);
	UINT32 get_a = 0, get_b = 0;

	ime_get_adas_max_edge_reg(&get_a, &get_b);
	p_get_hist_info->get_hist_max0 = get_a;
	p_get_hist_info->get_hist_max1 = get_b;

	ime_get_adas_histogram_bin_reg(&get_a, &get_b);
	p_get_hist_info->get_acc_tag_bin0 = get_a;
	p_get_hist_info->get_acc_tag_bin1 = get_b;
}
//-------------------------------------------------------------------------------

VOID ime_get_statistic_edge_map_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr)
{
	if (p_get_addr != NULL) {
		//ime_get_adas_edge_map_addr(p_get_addr);

		ime_get_adas_edge_map_addr_reg(&(p_get_addr->addr_y));

		p_get_addr->addr_cb = p_get_addr->addr_cr = p_get_addr->addr_y;

#if (IME_DMA_CACHE_HANDLE == 1)
		//p_get_addr->addr_y = dma_getNonCacheAddr(p_get_addr->addr_y);
		//p_get_addr->addr_cb = dma_getNonCacheAddr(p_get_addr->addr_cb);
		//p_get_addr->addr_cr = dma_getNonCacheAddr(p_get_addr->addr_cr);
#endif
	}
}
//------------------------------------------------------------------------------

VOID ime_get_statistic_edge_map_lineoffset_info(UINT32 *p_get_lofs)
{
	if (p_get_lofs != NULL) {
		//*p_get_lofs = ime_get_adas_edge_map_lofs();

		*p_get_lofs = ime_get_adas_edge_map_dam_lineoffset_reg();
	}
}
//------------------------------------------------------------------------------


VOID ime_get_statistic_histogram_dma_addr_info(IME_DMA_ADDR_INFO *p_get_addr)
{
	//ime_get_adas_histogram_addr(p_get_addr);

	ime_get_adas_histogram_addr_reg(&(p_get_addr->addr_y));

	p_get_addr->addr_cb = p_get_addr->addr_cr = p_get_addr->addr_y;

#if (IME_DMA_CACHE_HANDLE == 1)
	//p_get_addr->addr_y = dma_getNonCacheAddr(p_get_addr->addr_y);
	//p_get_addr->addr_cb = dma_getNonCacheAddr(p_get_addr->addr_cb);
	//p_get_addr->addr_cr = dma_getNonCacheAddr(p_get_addr->addr_cr);
#endif
}
//------------------------------------------------------------------------------


ER ime_chg_yuv_converter_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_yuv_converter_enable(set_en);
	ime_set_yuv_converter_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_yuv_converter_param(IME_YUV_CVT set_cvt)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_yuv_converter_sel(set_cvt);

	ime_set_yuv_converter_sel_reg((UINT32)set_cvt);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

IME_YUV_TYPE ime_get_output_yuv_data_type(VOID)
{
	IME_YUV_TYPE get_type;

	if (ime_get_yuv_converter_enable_reg() == IME_FUNC_DISABLE) {
		//
		get_type = IME_YUV_TYPE_FULL;
	} else {
		//
		if (ime_get_yuv_converter_sel_reg() == IME_YUV_CVT_BT601) {
			//
			get_type = IME_YUV_TYPE_BT601;
		} else {
			//
			get_type = IME_YUV_TYPE_BT709;
		}
	}

	return get_type;
}
//------------------------------------------------------------------------------



ER ime_chg_stitching_enable(IME_PATH_SEL path_sel, IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_en == IME_FUNC_ENABLE) {

		if (path_sel == IME_PATH4_SEL) {
			DBG_WRN("IME: path4, do not support stitching...\r\n");

			return E_OK;
		}

		//er_return = ime_check_stitching(path_sel);
		//if (er_return != E_OK) {
		//  return er_return;
		//}
	}

	//ime_set_stitching_enable(path_sel, set_en);
	switch (path_sel) {
	case IME_PATH1_SEL:
		ime_set_stitching_enable_p1_reg((UINT32)set_en);
		break;

	case IME_PATH2_SEL:
		ime_set_stitching_enable_p2_reg((UINT32)set_en);
		break;

	case IME_PATH3_SEL:
		ime_set_stitching_enable_p3_reg((UINT32)set_en);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_stitching_enable_p5_reg((UINT32)set_en);
	//break;

	default:
		break;
	}

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_stitching_image_param(IME_PATH_SEL path_sel, IME_STITCH_INFO *p_stitch_info)
{
	ER er_return = E_OK;

	IME_SIZE_INFO get_size = {0x0};
	IME_OUTPUT_IMG_FORMAT_SEL  out_format = IME_OUTPUT_YCC_420;
	IME_OUTPUT_FORMAT_TYPE     out_format_type = IME_OUTPUT_YCC_UVPACKIN;
	IME_FUNC_EN path_en = IME_FUNC_DISABLE;

	UINT32 fluch_size_y = 0x0, fluch_size_c = 0x0;
	BOOL fluch_type_y = FALSE, fluch_type_c = FALSE;

	IME_LINEOFFSET_INFO set_lofs = {0x0};
	IME_DMA_ADDR_INFO set_addr = {0x0};


	if (p_stitch_info == NULL) {
		DBG_ERR("IME: stitching, setting parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (path_sel == IME_PATH4_SEL) {
		DBG_ERR("IME: path4, do not support stitching...\r\n");

		return E_PAR;
	}

	ime_get_output_path_image_size_info(path_sel, &get_size);
	if (p_stitch_info->stitch_pos % 4 != 0) {
		DBG_ERR("IME: stitching, left image width must be 4x...\r\n");

		return E_PAR;
	}

	if (path_sel == IME_PATH1_SEL) {
		out_format = (IME_OUTPUT_IMG_FORMAT_SEL)ime_get_engine_info(IME_GET_P1_OUTPUT_FORMAT);
		out_format_type = (IME_OUTPUT_FORMAT_TYPE)ime_get_engine_info(IME_GET_P1_OUTPUT_TYPE);
		path_en = (IME_FUNC_EN)ime_get_engine_info(IME_GET_P1_ENABLE_STATUS);
	} else if (path_sel == IME_PATH2_SEL) {
		out_format = (IME_OUTPUT_IMG_FORMAT_SEL)ime_get_engine_info(IME_GET_P2_OUTPUT_FORMAT);
		out_format_type = (IME_OUTPUT_FORMAT_TYPE)ime_get_engine_info(IME_GET_P2_OUTPUT_TYPE);
		path_en = (IME_FUNC_EN)ime_get_engine_info(IME_GET_P2_ENABLE_STATUS);
	} else if (path_sel == IME_PATH3_SEL) {
		out_format = (IME_OUTPUT_IMG_FORMAT_SEL)ime_get_engine_info(IME_GET_P3_OUTPUT_FORMAT);
		out_format_type = (IME_OUTPUT_FORMAT_TYPE)ime_get_engine_info(IME_GET_P3_OUTPUT_TYPE);
		path_en = (IME_FUNC_EN)ime_get_engine_info(IME_GET_P3_ENABLE_STATUS);
	} else {
		// do nothing...
	}

	if (path_en == IME_FUNC_DISABLE) {
		return E_OK;
	}

	if (out_format_type == IME_OUTPUT_YCC_PLANAR) {
		DBG_ERR("IME: stitching, only support Y/UV-Pack format...\r\n");

		return E_PAR;
	}

	//ime_set_stitching_base_position(path_sel, p_stitch_info->stitch_pos);
	if (path_sel == IME_PATH1_SEL) {
		ime_set_stitching_base_position_p1_reg(p_stitch_info->stitch_pos);
	} else if (path_sel == IME_PATH2_SEL) {
		ime_set_stitching_base_position_p2_reg(p_stitch_info->stitch_pos);
	} else if (path_sel == IME_PATH3_SEL) {
		ime_set_stitching_base_position_p3_reg(p_stitch_info->stitch_pos);
	} else {
		// do nothing...
	}


	ime_engine_operation_mode = ime_get_operate_mode();

	set_lofs = p_stitch_info->stitch_lofs;
	if (p_stitch_info->lofs_update == TRUE) {
		//ime_set_stitching_lineoffset(path_sel, &set_lofs);
		if (path_sel == IME_PATH1_SEL) {
			ime_set_stitching_output_lineoffset_y2_p1_reg(set_lofs.lineoffset_y);
			ime_set_stitching_output_lineoffset_c2_p1_reg(set_lofs.lineoffset_cb);
		} else if (path_sel == IME_PATH2_SEL) {
			ime_set_stitching_output_lineoffset_y2_p2_reg(set_lofs.lineoffset_y);
			ime_set_stitching_output_lineoffset_c2_p2_reg(set_lofs.lineoffset_cb);
		} else if (path_sel == IME_PATH3_SEL) {
			ime_set_stitching_output_lineoffset_y2_p3_reg(set_lofs.lineoffset_y);
			ime_set_stitching_output_lineoffset_c2_p3_reg(set_lofs.lineoffset_cb);
		} else {
			// do nothing...
		}
	}

	set_addr = p_stitch_info->stitch_dma_addr;
	if (p_stitch_info->dma_addr_update == TRUE) {
		switch (out_format) {
#if 0
		case IME_OUTPUT_YCC_444:
		case IME_OUTPUT_RGB_444:
			fluch_size_y = set_lofs.lineoffset_y * get_size.size_v;
			fluch_size_c = set_lofs.lineoffset_cb * get_size.size_v;

			fluch_type_y = ((get_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			fluch_type_c = ((get_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);
			break;
#endif

		//case IME_OUTPUT_YCC_422_COS:
		//case IME_OUTPUT_YCC_422_CEN:
		//case IME_OUTPUT_YCC_420_COS:
		case IME_OUTPUT_YCC_420:
			fluch_size_y = set_lofs.lineoffset_y * get_size.size_v;
			fluch_size_c = set_lofs.lineoffset_cb * get_size.size_v;

			fluch_size_y = IME_ALIGN_CEIL_32(fluch_size_y);
			fluch_size_c = IME_ALIGN_CEIL_32(fluch_size_c);

			fluch_type_y = ((((get_size.size_h - 1) - p_stitch_info->stitch_pos + 1) == set_lofs.lineoffset_y) ? TRUE : FALSE);
			fluch_type_c = ((((get_size.size_h - 1) - p_stitch_info->stitch_pos + 1) == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			// coverity[assigned_value]
			set_addr.addr_cr = set_addr.addr_cb;

			if (out_format_type == IME_OUTPUT_YCC_UVPACKIN) {
				set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, fluch_size_y, fluch_type_y, p_stitch_info->dma_flush, ime_engine_operation_mode);
				set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, fluch_size_c, fluch_type_c, p_stitch_info->dma_flush, ime_engine_operation_mode);
				//set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, fluch_size_c, fluch_type_c, p_stitch_info->dma_flush, ime_engine_operation_mode);

				if (path_sel == IME_PATH1_SEL) {
					if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (p_stitch_info->dma_flush == IME_DO_BUF_FLUSH)) {
						ime_out_buf_flush[IME_OUT_P1S_Y].flush_en = ENABLE;
						ime_out_buf_flush[IME_OUT_P1S_Y].buf_addr = set_addr.addr_y;
						ime_out_buf_flush[IME_OUT_P1S_Y].buf_size = fluch_size_y;

						ime_out_buf_flush[IME_OUT_P1S_U].flush_en = ENABLE;
						ime_out_buf_flush[IME_OUT_P1S_U].buf_addr = set_addr.addr_cb;
						ime_out_buf_flush[IME_OUT_P1S_U].buf_size = fluch_size_c;
					}
				} else if (path_sel == IME_PATH2_SEL) {
					if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (p_stitch_info->dma_flush == IME_DO_BUF_FLUSH)) {
						ime_out_buf_flush[IME_OUT_P2S_Y].flush_en = ENABLE;
						ime_out_buf_flush[IME_OUT_P2S_Y].buf_addr = set_addr.addr_y;
						ime_out_buf_flush[IME_OUT_P2S_Y].buf_size = fluch_size_y;

						ime_out_buf_flush[IME_OUT_P2S_U].flush_en = ENABLE;
						ime_out_buf_flush[IME_OUT_P2S_U].buf_addr = set_addr.addr_cb;
						ime_out_buf_flush[IME_OUT_P2S_U].buf_size = fluch_size_c;
					}
				} else if (path_sel == IME_PATH3_SEL) {
					if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (p_stitch_info->dma_flush == IME_DO_BUF_FLUSH)) {
						ime_out_buf_flush[IME_OUT_P3S_Y].flush_en = ENABLE;
						ime_out_buf_flush[IME_OUT_P3S_Y].buf_addr = set_addr.addr_y;
						ime_out_buf_flush[IME_OUT_P3S_Y].buf_size = fluch_size_y;

						ime_out_buf_flush[IME_OUT_P3S_U].flush_en = ENABLE;
						ime_out_buf_flush[IME_OUT_P3S_U].buf_addr = set_addr.addr_cb;
						ime_out_buf_flush[IME_OUT_P3S_U].buf_size = fluch_size_c;
					}
				} else {
					// do nothing...
				}
			}

			break;

		case IME_OUTPUT_Y_ONLY:
			// coverity[assigned_value]
			set_lofs.lineoffset_cb = set_lofs.lineoffset_y;

			fluch_size_y = set_lofs.lineoffset_y * get_size.size_v;
			//fluch_size_c = set_lofs.lineoffset_y * get_size.size_v;

			fluch_size_y = IME_ALIGN_CEIL_32(fluch_size_y);

			set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;

			fluch_type_y = ((((get_size.size_h - 1) - p_stitch_info->stitch_pos + 1) == set_lofs.lineoffset_y) ? TRUE : FALSE);
			//fluch_type_c = ((((get_size.size_h - 1) - p_stitch_info->stitch_pos + 1) == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, fluch_size_y, fluch_type_y, p_stitch_info->dma_flush, ime_engine_operation_mode);

			if (path_sel == IME_PATH1_SEL) {
				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (p_stitch_info->dma_flush == IME_DO_BUF_FLUSH)) {
					ime_out_buf_flush[IME_OUT_P1S_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P1S_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P1S_Y].buf_size = fluch_size_y;
				}
			} else if (path_sel == IME_PATH2_SEL) {
				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (p_stitch_info->dma_flush == IME_DO_BUF_FLUSH)) {
					ime_out_buf_flush[IME_OUT_P2S_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P2S_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P2S_Y].buf_size = fluch_size_y;
				}
			} else if (path_sel == IME_PATH3_SEL) {
				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (p_stitch_info->dma_flush == IME_DO_BUF_FLUSH)) {
					ime_out_buf_flush[IME_OUT_P3S_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P3S_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P3S_Y].buf_size = fluch_size_y;
				}
			} else {
				// do nothing...
			}

			break;

		default:
			break;
		}

		//ime_set_stitching_dma_addr(path_sel, &set_addr);
		if (path_sel == IME_PATH1_SEL) {
			ime_set_stitching_output_channel_addr2_p1_reg(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr);
		} else if (path_sel == IME_PATH2_SEL) {
			ime_set_stitching_output_channel_addr2_p2_reg(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr);
		} else if (path_sel == IME_PATH3_SEL) {
			ime_set_stitching_output_channel_addr2_p3_reg(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr);
		} else {
			// do nothing...
		}

	}

	//er_return = ime_check_stitching(path_sel);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	return E_OK;
}
//------------------------------------------------------------------------------
// TMNR
ER ime_chg_tmnr_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

//static IME_FUNC_EN ime_get_tmnr_enable(VOID)
//{
//	return (IME_FUNC_EN)ime_tmnr_get_enable_reg();
//}
//------------------------------------------------------------------------------

//static IME_FUNC_EN ime_get_tmnr_decoder_enable(VOID)
//{
//	return (IME_FUNC_EN)ime_tmnr_get_out_ref_encoder_enable_reg();
//}
//------------------------------------------------------------------------------


ER ime_chg_tmnr_luma_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_nr_luma_channel_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_chroma_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_nr_chroma_channel_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_refin_decoder_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_in_ref_decoder_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_refin_flip_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_in_ref_flip_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_sta_roi_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_ms_roi_output_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_statistic_out_enable(UINT32 set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_statistic_data_output_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------



ER ime_chg_tmnr_motion_sta_roi_flip_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_ms_roi_flip_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_estimation_param(IME_TMNR_ME_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_me_control_reg((UINT32)p_set_param->update_mode, (UINT32)p_set_param->boundary_set, (UINT32)p_set_param->ds_mode);

	ime_tmnr_set_me_sad_reg((UINT32)p_set_param->sad_type, (UINT32)p_set_param->sad_shift);


	ime_tmnr_set_me_sad_penalty_reg(p_set_param->sad_penalty);

	ime_tmnr_set_me_switch_threshold_reg(p_set_param->switch_th, p_set_param->switch_rto);

	ime_tmnr_set_me_detail_penalty_reg(p_set_param->detail_penalty);

	ime_tmnr_set_me_probability_reg(p_set_param->probability);

	ime_tmnr_set_me_rand_bit_reg(p_set_param->rand_bit_x, p_set_param->rand_bit_y);

	ime_tmnr_set_me_threshold_reg(p_set_param->min_detail, p_set_param->periodic_th);

	return E_OK;
}
//------------------------------------------------------------------------------


ER ime_chg_tmnr_motion_detection_param(IME_TMNR_MD_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_md_final_threshold_reg(p_set_param->fth[0], p_set_param->fth[1]);

	ime_tmnr_set_md_sad_coefs_reg(p_set_param->sad_coefa, p_set_param->sad_coefb);

	ime_tmnr_set_md_sad_standard_deviation_reg(p_set_param->sad_std);

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_detection_roi_param(IME_TMNR_MD_ROI_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_md_roi_final_threshold_reg(p_set_param->fth[0], p_set_param->fth[1]);

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_compensation_param(IME_TMNR_MC_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_mc_noise_base_level_reg(p_set_param->sad_base);

	ime_tmnr_set_mc_edge_coefs_offset_reg(p_set_param->sad_coefa, p_set_param->sad_coefb);

	ime_tmnr_set_mc_sad_standard_deviation_reg(p_set_param->sad_std);

	ime_tmnr_set_mc_final_threshold_reg(p_set_param->fth[0], p_set_param->fth[1]);

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_compensation_roi_param(IME_TMNR_MC_ROI_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_mc_roi_final_threshold_reg(p_set_param->fth[0], p_set_param->fth[1]);

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_patch_selection_param(IME_TMNR_PS_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_ps_mv_check_enable_reg((UINT32)p_set_param->mv_check_en);

	ime_tmnr_set_ps_mv_roi_check_enable_reg((UINT32)p_set_param->roi_mv_check_en);

	ime_tmnr_set_ps_control_reg((UINT32)p_set_param->mv_info_mode, (UINT32)p_set_param->ps_mode);

	ime_tmnr_set_ps_mv_threshold_reg(p_set_param->mv_th, p_set_param->roi_mv_th);

	ime_tmnr_set_ps_down_sample_reg(p_set_param->ds_th);

	ime_tmnr_set_ps_roi_down_sample_reg(p_set_param->ds_th_roi);

	ime_tmnr_set_ps_edge_control_reg(p_set_param->edge_wet, p_set_param->edge_th[0], p_set_param->edge_th[1], p_set_param->edge_slope);

	ime_tmnr_set_ps_path_error_threshold_reg(p_set_param->fs_th);


	ime_tmnr_set_ps_mix_ratio_reg(p_set_param->mix_ratio[0], p_set_param->mix_ratio[1]);

	ime_tmnr_set_ps_mix_threshold_reg(p_set_param->mix_th[0], p_set_param->mix_th[1]);

	ime_tmnr_set_ps_mix_slope_reg(p_set_param->mix_slope[0], p_set_param->mix_slope[1]);

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_noise_filter_param(IME_TMNR_NR_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	// luma channel
	ime_tmnr_set_nr_center_zero_enable_reg((UINT32)p_set_param->center_wzeros_y);

	ime_tmnr_set_y_pre_blur_strength_reg((UINT32)p_set_param->pre_filter_type);

	if (nvt_get_chip_id() != CHIP_NA51055) {
		ime_tmnr_set_nr_type_reg(p_set_param->luma_nr_type);
	}

	ime_tmnr_set_nr_luma_residue_threshold_reg(p_set_param->luma_residue_th);

	ime_tmnr_set_nr_varied_frequency_filter_weight_reg(p_set_param->freq_wet);

	ime_tmnr_set_nr_luma_filter_weight_reg(p_set_param->luma_wet);

	ime_tmnr_set_nr_prefiltering_reg(p_set_param->pre_filter_str, p_set_param->pre_filter_rto);

	ime_tmnr_set_nr_luma_noise_reduction_reg(p_set_param->luma_3d_th, p_set_param->luma_3d_lut);

	ime_tmnr_set_nr_snr_control_reg(p_set_param->snr_str, p_set_param->snr_base_th);

	ime_tmnr_set_nr_tnr_control_reg(p_set_param->tnr_str, p_set_param->tnr_base_th);

	ime_tmnr_set_nr_luma_channel_enable_reg((UINT32)p_set_param->luma_ch_en);


	// chroma channel
	ime_tmnr_set_nr_false_color_control_enable_reg((UINT32)p_set_param->chroma_fsv_en);

	ime_tmnr_set_nr_false_color_control_strength_reg(p_set_param->chroma_fsv);

	ime_tmnr_set_nr_chroma_residue_threshold_reg(p_set_param->chroma_residue_th);

	ime_tmnr_set_nr_chroma_noise_reduction_reg(p_set_param->chroma_3d_lut, p_set_param->chroma_3d_rto);

	ime_tmnr_set_nr_chroma_channel_enable_reg((UINT32)p_set_param->chroma_ch_en);

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_statistic_data_output_param(IME_TMNR_STATISTIC_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_statistic_data_output_enable_reg((UINT32)p_set_param->sta_out_en);
	if (p_set_param->sta_out_en == IME_FUNC_ENABLE) {
		ime_tmnr_set_statistic_data_output_start_position_reg(p_set_param->sample_st_x, p_set_param->sample_st_y);
		ime_tmnr_set_statistic_data_output_sample_numbers_reg(p_set_param->sample_num_x, p_set_param->sample_num_y);
		ime_tmnr_set_statistic_data_output_sample_steps_reg(p_set_param->sample_step_hori, p_set_param->sample_step_vert);
	}

	return E_OK;
}
//------------------------------------------------------------------------------



ER ime_chg_tmnr_debug_ctrl_param(IME_TMNR_DBG_CTRL_PARAM *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_debug_reg((UINT32)p_set_param->dbg_mode, (UINT32)p_set_param->dbg_froce_mv0_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_ref_in_lineoffset(IME_LINEOFFSET_INFO *p_set_lofs)
{
	ER er_return = E_OK;

	if (p_set_lofs == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}


	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_in_ref_y_lineoffset_reg(p_set_lofs->lineoffset_y);
	ime_tmnr_set_in_ref_uv_lineoffset_reg(p_set_lofs->lineoffset_cb);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

VOID ime_chg_tmnr_ref_in_addr_use_va2pa(BOOL set_en)
{
    ime_tmrn_set_ref_in_va2pa_enable_reg(set_en);
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_ref_in_y_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR reference input address Y is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_in_ref_y_addr_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_ref_in_uv_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR reference input address UV is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_in_ref_uv_addr_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_ref_in_y_addr(VOID)
{
	return ime_tmnr_get_in_ref_y_addr_reg();
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_ref_in_uv_addr(VOID)
{
	return ime_tmnr_get_in_ref_uv_addr_reg();
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_status_lineoffset(UINT32 set_lofs)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_in_motion_status_lineoffset_reg(set_lofs);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_status_in_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR MS input address is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_in_motion_status_address_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_status_out_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR MS output address is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_out_motion_status_address_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_motion_status_in_addr(VOID)
{
	return ime_tmnr_get_in_motion_status_address_reg();
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_motion_status_out_addr(VOID)
{
	return ime_tmnr_get_out_motion_status_address_reg();
}
//------------------------------------------------------------------------------


ER ime_chg_tmnr_motion_status_roi_out_lineoffset(UINT32 set_lofs)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_out_roi_motion_status_lineoffset_reg(set_lofs);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_status_roi_out_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR MS-ROI output address is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_out_roi_motion_status_address_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_motion_status_roi_out_addr(VOID)
{
	return ime_tmnr_get_out_roi_motion_status_address_reg();
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_vector_lineoffset(UINT32 set_lofs)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_in_motion_vector_lineoffset_reg(set_lofs);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_vector_in_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR MV input address is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_in_motion_vector_address_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_motion_vector_out_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR MV output address is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_out_motion_vector_address_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_motion_vector_in_addr(VOID)
{
	return ime_tmnr_get_in_motion_vector_address_reg();
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_motion_vector_out_addr(VOID)
{
	return ime_tmnr_get_out_motion_vector_address_reg();
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_statistic_out_lineoffset(UINT32 set_lofs)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_out_statistic_lineoffset_reg(set_lofs);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_statistic_out_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR statistic output address is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_out_statistic_address_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_statistic_out_addr(VOID)
{
	return ime_tmnr_get_out_statistic_address_reg();
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// TMNR reference output
ER ime_chg_tmnr_ref_out_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_out_ref_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------


ER ime_chg_tmnr_ref_out_encoder_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_out_ref_encoder_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_ref_out_flip_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_out_ref_flip_enable_reg((UINT32)set_en);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------



ER ime_chg_tmnr_ref_out_lineoffset(IME_LINEOFFSET_INFO *p_set_lofs)
{
	ER er_return = E_OK;

	if (p_set_lofs == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}


	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_tmnr_set_out_ref_y_lineoffset_reg(p_set_lofs->lineoffset_y);
	ime_tmnr_set_out_ref_uv_lineoffset_reg(p_set_lofs->lineoffset_cb);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_ref_out_y_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR reference output address Y is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_out_ref_y_addr_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

ER ime_chg_tmnr_ref_out_uv_addr(UINT32 set_addr)
{
	ER er_return = E_OK;

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (set_addr == 0x0) {
		DBG_ERR("IME: TMNR reference output address UV is zero ...\r\n");

		return E_PAR;
	}

	ime_tmnr_set_out_ref_uv_addr_reg(set_addr);

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_ref_out_y_addr(VOID)
{
	return ime_tmnr_get_out_ref_y_addr_reg();
}
//------------------------------------------------------------------------------

UINT32 ime_get_tmnr_ref_out_uv_addr(VOID)
{
	return ime_tmnr_get_out_ref_uv_addr_reg();
}
//------------------------------------------------------------------------------


ER ime_chg_compression_param(IME_YUV_COMPRESSION_INFO *p_set_param)
{
	ER er_return = E_OK;

	if (p_set_param == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (p_set_param->comp_param_mode == IME_PARAM_AUTO_MODE) {

		ime_yuvcomp_q_tbl_idx[0] = 1;
		ime_yuvcomp_q_tbl_idx[1] = 3;
		ime_yuvcomp_q_tbl_idx[2] = 4;
		ime_yuvcomp_q_tbl_idx[3] = 5;
		ime_yuvcomp_q_tbl_idx[4] = 6;
		ime_yuvcomp_q_tbl_idx[5] = 7;
		ime_yuvcomp_q_tbl_idx[6] = 8;
		ime_yuvcomp_q_tbl_idx[7] = 9;
		ime_yuvcomp_q_tbl_idx[8] = 10;
		ime_yuvcomp_q_tbl_idx[9] = 11;
		ime_yuvcomp_q_tbl_idx[10] = 12;
		ime_yuvcomp_q_tbl_idx[11] = 13;
		ime_yuvcomp_q_tbl_idx[12] = 14;
		ime_yuvcomp_q_tbl_idx[13] = 15;
		ime_yuvcomp_q_tbl_idx[14] = 16;
		ime_yuvcomp_q_tbl_idx[15] = 19;

		yuv_dct_qtab_encp[0][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[0]][0];
		yuv_dct_qtab_encp[0][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[0]][1];
		yuv_dct_qtab_encp[0][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[0]][2];
		yuv_dct_qtab_encp[1][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[1]][0];
		yuv_dct_qtab_encp[1][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[1]][1];
		yuv_dct_qtab_encp[1][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[1]][2];
		yuv_dct_qtab_encp[2][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[2]][0];
		yuv_dct_qtab_encp[2][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[2]][1];
		yuv_dct_qtab_encp[2][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[2]][2];
		yuv_dct_qtab_encp[3][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[3]][0];
		yuv_dct_qtab_encp[3][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[3]][1];
		yuv_dct_qtab_encp[3][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[3]][2];
		yuv_dct_qtab_encp[4][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[4]][0];
		yuv_dct_qtab_encp[4][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[4]][1];
		yuv_dct_qtab_encp[4][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[4]][2];
		yuv_dct_qtab_encp[5][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[5]][0];
		yuv_dct_qtab_encp[5][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[5]][1];
		yuv_dct_qtab_encp[5][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[5]][2];
		yuv_dct_qtab_encp[6][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[6]][0];
		yuv_dct_qtab_encp[6][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[6]][1];
		yuv_dct_qtab_encp[6][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[6]][2];
		yuv_dct_qtab_encp[7][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[7]][0];
		yuv_dct_qtab_encp[7][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[7]][1];
		yuv_dct_qtab_encp[7][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[7]][2];
		yuv_dct_qtab_encp[8][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[8]][0];
		yuv_dct_qtab_encp[8][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[8]][1];
		yuv_dct_qtab_encp[8][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[8]][2];
		yuv_dct_qtab_encp[9][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[9]][0];
		yuv_dct_qtab_encp[9][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[9]][1];
		yuv_dct_qtab_encp[9][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[9]][2];
		yuv_dct_qtab_encp[10][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[10]][0];
		yuv_dct_qtab_encp[10][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[10]][1];
		yuv_dct_qtab_encp[10][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[10]][2];
		yuv_dct_qtab_encp[11][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[11]][0];
		yuv_dct_qtab_encp[11][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[11]][1];
		yuv_dct_qtab_encp[11][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[11]][2];
		yuv_dct_qtab_encp[12][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[12]][0];
		yuv_dct_qtab_encp[12][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[12]][1];
		yuv_dct_qtab_encp[12][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[12]][2];
		yuv_dct_qtab_encp[13][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[13]][0];
		yuv_dct_qtab_encp[13][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[13]][1];
		yuv_dct_qtab_encp[13][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[13]][2];
		yuv_dct_qtab_encp[14][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[14]][0];
		yuv_dct_qtab_encp[14][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[14]][1];
		yuv_dct_qtab_encp[14][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[14]][2];
		yuv_dct_qtab_encp[15][0] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[15]][0];
		yuv_dct_qtab_encp[15][1] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[15]][1];
		yuv_dct_qtab_encp[15][2] = ime_yuvcomp_dct_enc_qtbl[ime_yuvcomp_q_tbl_idx[15]][2];


		yuv_dct_qtab_decp[0][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[0]][0];
		yuv_dct_qtab_decp[0][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[0]][1];
		yuv_dct_qtab_decp[0][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[0]][2];
		yuv_dct_qtab_decp[1][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[1]][0];
		yuv_dct_qtab_decp[1][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[1]][1];
		yuv_dct_qtab_decp[1][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[1]][2];
		yuv_dct_qtab_decp[2][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[2]][0];
		yuv_dct_qtab_decp[2][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[2]][1];
		yuv_dct_qtab_decp[2][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[2]][2];
		yuv_dct_qtab_decp[3][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[3]][0];
		yuv_dct_qtab_decp[3][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[3]][1];
		yuv_dct_qtab_decp[3][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[3]][2];
		yuv_dct_qtab_decp[4][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[4]][0];
		yuv_dct_qtab_decp[4][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[4]][1];
		yuv_dct_qtab_decp[4][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[4]][2];
		yuv_dct_qtab_decp[5][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[5]][0];
		yuv_dct_qtab_decp[5][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[5]][1];
		yuv_dct_qtab_decp[5][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[5]][2];
		yuv_dct_qtab_decp[6][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[6]][0];
		yuv_dct_qtab_decp[6][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[6]][1];
		yuv_dct_qtab_decp[6][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[6]][2];
		yuv_dct_qtab_decp[7][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[7]][0];
		yuv_dct_qtab_decp[7][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[7]][1];
		yuv_dct_qtab_decp[7][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[7]][2];
		yuv_dct_qtab_decp[8][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[8]][0];
		yuv_dct_qtab_decp[8][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[8]][1];
		yuv_dct_qtab_decp[8][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[8]][2];
		yuv_dct_qtab_decp[9][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[9]][0];
		yuv_dct_qtab_decp[9][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[9]][1];
		yuv_dct_qtab_decp[9][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[9]][2];
		yuv_dct_qtab_decp[10][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[10]][0];
		yuv_dct_qtab_decp[10][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[10]][1];
		yuv_dct_qtab_decp[10][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[10]][2];
		yuv_dct_qtab_decp[11][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[11]][0];
		yuv_dct_qtab_decp[11][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[11]][1];
		yuv_dct_qtab_decp[11][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[11]][2];
		yuv_dct_qtab_decp[12][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[12]][0];
		yuv_dct_qtab_decp[12][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[12]][1];
		yuv_dct_qtab_decp[12][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[12]][2];
		yuv_dct_qtab_decp[13][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[13]][0];
		yuv_dct_qtab_decp[13][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[13]][1];
		yuv_dct_qtab_decp[13][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[13]][2];
		yuv_dct_qtab_decp[14][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[14]][0];
		yuv_dct_qtab_decp[14][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[14]][1];
		yuv_dct_qtab_decp[14][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[14]][2];
		yuv_dct_qtab_decp[15][0] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[15]][0];
		yuv_dct_qtab_decp[15][1] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[15]][1];
		yuv_dct_qtab_decp[15][2] = ime_yuvcomp_dct_dec_qtbl[ime_yuvcomp_q_tbl_idx[15]][2];

		yuv_dct_qtab_dc[0] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[0]];
		yuv_dct_qtab_dc[1] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[1]];
		yuv_dct_qtab_dc[2] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[2]];
		yuv_dct_qtab_dc[3] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[3]];
		yuv_dct_qtab_dc[4] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[4]];
		yuv_dct_qtab_dc[5] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[5]];
		yuv_dct_qtab_dc[6] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[6]];
		yuv_dct_qtab_dc[7] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[7]];
		yuv_dct_qtab_dc[8] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[8]];
		yuv_dct_qtab_dc[9] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[9]];
		yuv_dct_qtab_dc[10] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[10]];
		yuv_dct_qtab_dc[11] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[11]];
		yuv_dct_qtab_dc[12] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[12]];
		yuv_dct_qtab_dc[13] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[13]];
		yuv_dct_qtab_dc[14] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[14]];
		yuv_dct_qtab_dc[15] = ime_yuvcomp_dct_max[ime_yuvcomp_q_tbl_idx[15]];

		ime_comps_encoder_parameters_reg(yuv_dct_qtab_encp);
		ime_comps_decoder_parameters_reg(yuv_dct_qtab_decp);
		ime_comps_dc_max_reg(yuv_dct_qtab_dc);


	} else {
		ime_comps_encoder_parameters_reg(p_set_param->comp_enc);
		ime_comps_decoder_parameters_reg(p_set_param->comp_dec);
		ime_comps_dc_max_reg(p_set_param->comp_q);
	}

	ime_load();

	return E_OK;
}
//------------------------------------------------------------------------------


#if 0
UINT32 ime_get_clock_rate(VOID)
{
	return ime_eng_clock_rate;
}
#endif
//-------------------------------------------------------------------------------

ER ime_chg_burst_length(IME_BURST_LENGTH *p_set_info)
{
	ER er_return = E_OK;

	if (p_set_info == NULL) {
		//
		DBG_ERR("IME: burst setting parameter NULL...\r\n");

		return E_PAR;
	}

	// check state machine
	if (ime_engine_state_machine_status == IME_ENGINE_RUN) {
		return E_SYS;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	switch (p_set_info->bst_input_y) {
	case IME_BURST_32W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: input, Y burst lenght setting error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_input_u) {
	case IME_BURST_32W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: input, U burst lenght setting error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_input_v) {
	case IME_BURST_32W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: input, V burst lenght setting error...\r\n");
		return E_PAR;
	}
	ime_set_in_path_burst_size(p_set_info->bst_input_y, p_set_info->bst_input_u, p_set_info->bst_input_v);



	switch (p_set_info->bst_output_path1_v) {
	case IME_BURST_32W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: path1, V burst size error...\r\n");
		return E_PAR;
	}
	ime_set_out_path_p1_burst_size(p_set_info->bst_output_path1_y, p_set_info->bst_output_path1_u, p_set_info->bst_output_path1_v);


	ime_set_out_path_p2_burst_size(p_set_info->bst_output_path2_y, p_set_info->bst_output_path2_u);


	switch (p_set_info->bst_output_path3_y) {
	case IME_BURST_32W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: path3, Y burst size error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_output_path3_u) {
	case IME_BURST_32W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: path3, UV burst size error...\r\n");
		return E_PAR;
	}
	ime_set_out_path_p3_burst_size(p_set_info->bst_output_path3_y, p_set_info->bst_output_path3_u);

	switch (p_set_info->bst_output_path4_y) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: path4, Y burst size error...\r\n");
		return E_PAR;
	}
	ime_set_out_path_p4_burst_size(p_set_info->bst_output_path4_y);


	// LCA subimage input
	switch (p_set_info->bst_input_lca) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: LCA, input burst size error...\r\n");
		return E_PAR;
	}
	ime_set_in_path_lca_burst_size(p_set_info->bst_input_lca);

	// LCA subimage output
	switch (p_set_info->bst_subout_lca) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: LCA, output burst size error...\r\n");
		return E_PAR;
	}
	ime_set_out_path_lca_burst_size(p_set_info->bst_subout_lca);

	switch (p_set_info->bst_stamp) {
	case IME_BURST_32W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: OSD, input burst size error...\r\n");
		return E_PAR;
	}
	ime_set_osd_set0_burst_size(p_set_info->bst_stamp);


	switch (p_set_info->bst_privacy_mask) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: privacy mask, input burst size error...\r\n");
		return E_PAR;
	}
	ime_set_privacy_mask_burst_size(p_set_info->bst_privacy_mask);


	// TMNR
	switch (p_set_info->bst_tmnr_input_y) {
	case IME_BURST_64W:
	case IME_BURST_32W:
		break;

	default:
		DBG_ERR("IME: TMNR, input Y burst size error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_tmnr_input_c) {
	case IME_BURST_64W:
	case IME_BURST_32W:
		break;

	default:
		DBG_ERR("IME: TMNR, input UV burst size error...\r\n");
		return E_PAR;
	}


	switch (p_set_info->bst_tmnr_input_mv) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: TMNR, input motion vector burst size error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_tmnr_input_mo_sta) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: TMNR, input motion status burst size error...\r\n");
		return E_PAR;
	}


	ime_set_in_path_3dnr_burst_size(p_set_info->bst_tmnr_input_y, p_set_info->bst_tmnr_input_c, p_set_info->bst_tmnr_input_mv, p_set_info->bst_tmnr_input_mo_sta);


	switch (p_set_info->bst_tmnr_output_y) {
	case IME_BURST_32W:
	case IME_BURST_16W:
	case IME_BURST_48W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: TMNR, output Y burst size error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_tmnr_output_c) {
	case IME_BURST_32W:
	case IME_BURST_16W:
	case IME_BURST_48W:
	case IME_BURST_64W:
		break;

	default:
		DBG_ERR("IME: TMNR, output UV burst size error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_tmnr_output_mv) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: TMNR, output motion vector burst size error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_tmnr_output_mo_sta) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: TMNR, output motion status burst size error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_tmnr_output_mo_sta_roi) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: TMNR, output motion status ROI burst size error...\r\n");
		return E_PAR;
	}

	switch (p_set_info->bst_tmnr_output_sta) {
	case IME_BURST_32W:
	case IME_BURST_16W:
		break;

	default:
		DBG_ERR("IME: TMNR, output sta burst size error...\r\n");
		return E_PAR;
	}
	ime_set_out_path_3dnr_burst_size(p_set_info->bst_tmnr_output_y, p_set_info->bst_tmnr_output_c, p_set_info->bst_tmnr_output_mv, p_set_info->bst_tmnr_output_mo_sta, p_set_info->bst_tmnr_output_mo_sta_roi, p_set_info->bst_tmnr_output_sta);



	return E_OK;
}

//-------------------------------------------------------------------------------

UINT32 ime_get_input_max_stripe_size(VOID)
{
	IME_SCALE_METHOD_SEL scl_method1 = IMEALG_BICUBIC, scl_method2 = IMEALG_BICUBIC, scl_method3 = IMEALG_BICUBIC;//, scl_method4 = IMEALG_BICUBIC;
	IME_FUNC_EN path1_en = IME_FUNC_DISABLE, path2_en = IME_FUNC_DISABLE, path3_en = IME_FUNC_DISABLE, path4_en = IME_FUNC_DISABLE;
	//IME_FUNC_EN SSREn = IME_FUNC_DISABLE;
	UINT32 ttl_en = 0, isd_cnt = 0;
	UINT32 max_stripe_size = 512;

	//ime_get_out_path_enable_p1(&path1_en);
	//ime_get_out_path_enable_p2(&path2_en);
	//ime_get_out_path_enable_p3(&path3_en);
	//ime_get_out_path_enable_p4(&path4_en);

	path1_en = ime_get_output_path_enable_status_p1_reg();
	path2_en = ime_get_output_path_enable_status_p2_reg();
	path3_en = ime_get_output_path_enable_status_p3_reg();
	path4_en = ime_get_output_path_enable_status_p4_reg();

	if ((path1_en == IME_FUNC_DISABLE) && (path2_en == IME_FUNC_DISABLE) && (path3_en == IME_FUNC_DISABLE) && (path4_en == IME_FUNC_DISABLE)) {
		return max_stripe_size;
	}

	//ime_get_out_path_scale_method_p1(&scl_method1);
	//ime_get_out_path_scale_method_p2(&scl_method2);
	//ime_get_out_path_scale_method_p3(&scl_method3);
	//ime_get_out_path_scale_method_p4(&scl_method4);

	scl_method1 = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p1_reg();
	scl_method2 = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p2_reg();
	scl_method3 = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p3_reg();
	//scl_method4 = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p4_reg();

	ttl_en = (UINT32)path1_en + (UINT32)path2_en + (UINT32)path3_en + (UINT32)path4_en;
	if (ttl_en != 0) {
		//SSREn = ime_get_ssr_enable();
		isd_cnt = 0;

		if (scl_method1 == IMEALG_INTEGRATION) {
			isd_cnt += 1;
		}
		if (scl_method2 == IMEALG_INTEGRATION) {
			isd_cnt += 1;
		}
		if (scl_method3 == IMEALG_INTEGRATION) {
			isd_cnt += 1;
		}

		//if((isd_cnt == 0) && (SSREn == IME_FUNC_DISABLE))
		if ((isd_cnt == 0)) {
			if (nvt_get_chip_id() == CHIP_NA51055) {
				max_stripe_size = 2688;
			} else if (nvt_get_chip_id() == CHIP_NA51084) {
				max_stripe_size = 4096;
			} else {
                max_stripe_size = 2688;
                DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
            }
		}

		//if((isd_cnt == 0) && (SSREn == IME_FUNC_ENABLE))
		//{
		//    max_stripe_size = 1344;
		//}

		//if((isd_cnt != 0) && (SSREn == IME_FUNC_DISABLE))
		if ((isd_cnt != 0)) {
			if (nvt_get_chip_id() == CHIP_NA51055) {
				max_stripe_size = 1344;
			} else if (nvt_get_chip_id() == CHIP_NA51084) {
				max_stripe_size = 2048;
			} else {
                max_stripe_size = 1344;
                DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
            }
		}

		//if((isd_cnt != 0) && (SSREn == IME_FUNC_ENABLE))
		//{
		//    max_stripe_size = 512;
		//}
	}

	return max_stripe_size;
}
//-------------------------------------------------------------------------------

INT32 ime_get_burst_length(IME_GET_BSTSIZE_SEL get_bst_size_sel)
{
	return ime_get_burst_size_info((UINT32)get_bst_size_sel);
}
//------------------------------------------------------------------------------


VOID ime_get_output_path_info(IME_PATH_SEL path_sel, IME_GET_OUTPATH_INFO *p_get_info)
{
	IME_FUNC_EN get_path_en = IME_FUNC_DISABLE;
	IME_OUTPUT_FORMAT_TYPE get_type = IME_OUTPUT_YCC_UVPACKIN;
	IME_OUTPUT_IMG_FORMAT_SEL get_fmt = IME_OUTPUT_YCC_420;
	IME_SCALE_METHOD_SEL get_method = IMEALG_BILINEAR;
	IME_SIZE_INFO get_scale_size = {0};
	IME_POS_INFO get_pos = {0};
	IME_SIZE_INFO get_out_size = {0};
	IME_LINEOFFSET_INFO get_lofs = {0};
	IME_DMA_ADDR_INFO get_addr = {0};

	if (p_get_info == NULL) {
		DBG_ERR("IME: output, get path info. NULL ...\r\n");
		return;
	}

	switch (path_sel) {
	case IME_PATH1_SEL:
		//ime_get_out_path_enable_p1(&get_path_en);
		//ime_get_out_path_format_type_p1(&get_type);
		//ime_get_out_path_image_format_p1(&get_fmt);
		//ime_get_out_path_scale_method_p1(&get_method);
		//ime_get_out_path_scale_size_p1(&get_scale_size);
		//ime_get_out_path_crop_start_pos_p1(&get_pos);
		//ime_get_out_path_output_size_p1(&get_out_size);
		//ime_get_out_path_lineoffset_p1(&get_lofs);
		//ime_get_out_path_dma_addr_p1(&get_addr);

		get_path_en = ime_get_output_path_enable_status_p1_reg();
		get_type = ime_get_output_format_type_p1_reg();
		get_fmt = ime_get_output_format_p1_reg();
		get_method = ime_get_scaling_method_p1_reg();
		ime_get_scaling_size_p1_reg(&(get_scale_size.size_h), &(get_scale_size.size_v));
		ime_get_output_size_p1_reg(&(get_out_size.size_h), &(get_out_size.size_v));

		get_lofs.lineoffset_y = ime_get_output_lineoffset_y_p1_reg();
		get_lofs.lineoffset_cb = ime_get_output_lineoffset_c_p1_reg();

		get_addr.addr_y = ime_get_output_addr_y_p1_reg();
		get_addr.addr_cb = ime_get_output_addr_cb_p1_reg();
		get_addr.addr_cr = ime_get_output_addr_cr_p1_reg();

		ime_get_output_crop_coordinate_p1_reg(&(get_pos.pos_x), &(get_pos.pos_y));
		break;

	case IME_PATH2_SEL:
		//ime_get_out_path_enable_p2(&get_path_en);
		//ime_get_out_path_format_type_p2(&get_type);
		//ime_get_out_path_image_format_p2(&get_fmt);
		//ime_get_out_path_scale_method_p2(&get_method);
		//ime_get_out_path_scale_size_p2(&get_scale_size);
		//ime_get_out_path_crop_start_pos_p2(&get_pos);
		//ime_get_out_path_output_size_p2(&get_out_size);
		//ime_get_out_path_lineoffset_p2(&get_lofs);
		//ime_get_out_path_dma_addr_p2(&get_addr);

		get_path_en = ime_get_output_path_enable_status_p2_reg();
		get_type = ime_get_output_format_type_p2_reg();
		get_fmt = ime_get_output_format_p2_reg();
		get_method = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p2_reg();

		ime_get_scaling_size_p2_reg(&(get_scale_size.size_h), &(get_scale_size.size_v));
		ime_get_output_size_p2_reg(&(get_out_size.size_h), &(get_out_size.size_v));

		get_lofs.lineoffset_y = ime_get_output_lineoffset_y_p2_reg();
		get_lofs.lineoffset_cb = ime_get_output_lineoffset_c_p2_reg();

		get_addr.addr_y = ime_get_output_addr_y_p2_reg();
		get_addr.addr_cb = ime_get_output_addr_cb_p2_reg();
		get_addr.addr_cr = ime_get_output_addr_cb_p2_reg();

		ime_get_output_crop_coordinate_p2_reg(&(get_pos.pos_x), &(get_pos.pos_y));
		break;

	case IME_PATH3_SEL:
		//ime_get_out_path_enable_p3(&get_path_en);
		//ime_get_out_path_format_type_p3(&get_type);
		//ime_get_out_path_image_format_p3(&get_fmt);
		//ime_get_out_path_scale_method_p3(&get_method);
		//ime_get_out_path_scale_size_p3(&get_scale_size);
		//ime_get_out_path_crop_start_pos_p3(&get_pos);
		//ime_get_out_path_output_size_p3(&get_out_size);
		//ime_get_out_path_lineoffset_p3(&get_lofs);
		//ime_get_out_path_dma_addr_p3(&get_addr);

		get_path_en = ime_get_output_path_enable_status_p3_reg();
		get_type = ime_get_output_format_type_p3_reg();
		get_fmt = ime_get_output_format_p3_reg();
		get_method = ime_get_scaling_method_p3_reg();

		ime_get_scaling_size_p3_reg(&(get_scale_size.size_h), &(get_scale_size.size_v));
		ime_get_output_size_p3_reg(&(get_out_size.size_h), &(get_out_size.size_v));

		get_lofs.lineoffset_y = ime_get_output_lineoffset_y_p3_reg();
		get_lofs.lineoffset_cb = ime_get_output_lineoffset_c_p3_reg();

		get_addr.addr_y = ime_get_output_addr_y_p3_reg();
		get_addr.addr_cb = ime_get_output_addr_cb_p3_reg();
		get_addr.addr_cr = ime_get_output_addr_cb_p3_reg();

		ime_get_output_crop_coordinate_p3_reg(&(get_pos.pos_x), &(get_pos.pos_y));
		break;

	case IME_PATH4_SEL:
		//ime_get_out_path_enable_p4(&get_path_en);
		//ime_get_out_path_format_type_p4(&get_type);
		//ime_get_out_path_image_format_p4(&get_fmt);
		//ime_get_out_path_scale_method_p4(&get_method);
		//ime_get_out_path_scale_size_p4(&get_scale_size);
		//ime_get_out_path_crop_start_pos_p4(&get_pos);
		//ime_get_out_path_output_size_p4(&get_out_size);
		//ime_get_out_path_lineoffset_p4(&get_lofs);
		//ime_get_out_path_dma_addr_p4(&get_addr);

		get_path_en = ime_get_output_path_enable_status_p4_reg();
		get_type = ime_get_output_format_type_p4_reg();
		get_fmt = ime_get_output_format_p4_reg();
		get_method = ime_get_scaling_method_p4_reg();

		ime_get_scaling_size_p4_reg(&(get_scale_size.size_h), &(get_scale_size.size_v));
		ime_get_output_size_p4_reg(&(get_out_size.size_h), &(get_out_size.size_v));

		get_lofs.lineoffset_y = ime_get_output_lineoffset_y_p4_reg();
		get_lofs.lineoffset_cb = ime_get_output_lineoffset_c_p4_reg();

		get_addr.addr_y = ime_get_output_addr_y_p4_reg();
		get_addr.addr_cb = ime_get_output_addr_cb_p4_reg();
		get_addr.addr_cr = ime_get_output_addr_cb_p4_reg();

		ime_get_output_crop_coordinate_p4_reg(&(get_pos.pos_x), &(get_pos.pos_y));
		break;

#if 0
	case IME_PATH5_SEL:
		ime_get_out_path_enable_p5(&get_path_en);
		ime_get_out_path_format_type_p5(&get_type);
		ime_get_out_path_image_format_p5(&get_fmt);
		ime_get_out_path_scale_method_p5(&get_method);
		ime_get_out_path_scale_size_p5(&get_scale_size);
		ime_get_out_path_crop_start_pos_p5(&get_pos);
		ime_get_out_path_output_size_p5(&get_out_size);
		ime_get_out_path_lineoffset_p5(&get_lofs);
		ime_get_out_path_dma_addr_p5(&get_addr);
		break;
#endif

	default:
		DBG_ERR("IME: output, path selection error ...\r\n");
		break;
	}

	if (p_get_info != NULL) {
		//
		p_get_info->out_path_enable = get_path_en;
		p_get_info->out_path_image_format.out_format_type_sel = get_type;
		p_get_info->out_path_image_format.out_format_sel = get_fmt;
		p_get_info->out_path_scale_method = get_method;
		p_get_info->out_path_scale_size = get_scale_size;
		p_get_info->out_path_crop_pos = get_pos;
		p_get_info->out_path_out_size = get_out_size;
		p_get_info->out_path_out_lineoffset = get_lofs;
		p_get_info->out_path_addr = get_addr;
		p_get_info->out_path_out_dest = IME_OUTDST_DRAM;
	}

}
//-------------------------------------------------------------------------------

VOID ime_get_input_path_info(IME_GET_INPATH_INFO *p_get_info)
{
	IME_INSRC_SEL get_src;
	IME_INPUT_FORMAT_SEL get_fmt;
	IME_SIZE_INFO get_img_size = {0};
	IME_LINEOFFSET_INFO get_lofs = {0};
	IME_DMA_ADDR_INFO get_addr = {0};

	if (p_get_info == NULL) {
		DBG_ERR("IME: input, get path info. NULL ...\r\n");
		return;
	}

	//ime_get_in_path_source(&get_src);
	//ime_get_in_path_image_format(&get_fmt);
	//ime_get_in_path_image_size(&get_img_size);
	//ime_get_in_path_lineoffset(&get_lofs);
	//ime_get_in_path_dma_addr(&get_addr);

	get_src = ime_get_input_source_reg();
	get_fmt = ime_get_input_image_format_reg();
	ime_get_input_image_size_reg(&(get_img_size.size_h), &(get_img_size.size_v));
	get_lofs.lineoffset_y = ime_get_input_lineoffset_y_reg();
	get_lofs.lineoffset_cb = ime_get_input_lineoffset_c_reg();
	ime_get_input_channel_addr1_reg(&(get_addr.addr_y), &(get_addr.addr_cb), &(get_addr.addr_cr));

	p_get_info->in_path_src = get_src;
	p_get_info->in_path_format = get_fmt;
	p_get_info->in_path_size = get_img_size;
	p_get_info->in_path_lineoffset = get_lofs;
	p_get_info->in_path_addr = get_addr;

}
//-------------------------------------------------------------------------------

IME_FUNC_EN ime_get_func_enable_info(IME_FUNC_SEL func_sel)
{
	IME_FUNC_EN get_en;

	switch (func_sel) {
	case IME_OUTPUT_P1:
		//ime_get_out_path_enable_p1(&get_en);
		get_en = ime_get_output_path_enable_status_p1_reg();
		break;

	case IME_OUTPUT_P2:
		//ime_get_out_path_enable_p2(&get_en);
		get_en = (IME_FUNC_EN)ime_get_output_path_enable_status_p2_reg();
		break;

	case IME_OUTPUT_P3:
		//ime_get_out_path_enable_p3(&get_en);
		get_en = (IME_FUNC_EN)ime_get_output_path_enable_status_p3_reg();
		break;

	case IME_OUTPUT_P4:
		//ime_get_out_path_enable_p4(&get_en);
		get_en = (IME_FUNC_EN)ime_get_output_path_enable_status_p4_reg();
		break;

	//case IME_OUTPUT_P5:
	//  ime_get_out_path_enable_p5(&get_en);
	//  break;

	//case IME_ICST:
	//  get_en = ime_get_icst_enable();
	//  break;

	//case IME_CST:
	//  get_en = ime_get_cst_enable();
	//  break;

	//case IME_CMF:
	//  get_en = ime_get_cmf_enable();
	//  break;

	case IME_LCA:
		//get_en = (IME_FUNC_EN)ime_get_ila_enable();
		get_en = (IME_FUNC_EN)ime_get_lca_enable_reg();
		break;

	case IME_LCA_SUBOUT:
		//get_en = (IME_FUNC_EN)ime_get_lca_subout_enable();
		get_en = (IME_FUNC_EN)ime_get_lca_subout_enable_reg();
		break;

	case IME_DBCS:
		//get_en = (IME_FUNC_EN)ime_get_dbcs_enable();
		get_en = (IME_FUNC_EN)ime_get_dbcs_enable_reg();
		break;

	//case IME_SSR:
	//  get_en = ime_get_ssr_enable();
	//  break;

	case IME_3DNR:
		//get_en = (IME_FUNC_EN)ime_get_tmnr_enable();
		get_en = (IME_FUNC_EN)ime_tmnr_get_enable_reg();
		break;

	//case IME_GRNS:
	//  get_en = ime_get_gn_enable();
	//  break;

	//case IME_P2I:
	//  get_en = ime_get_p2i_enable();
	//  break;

	case IME_STL:
		//get_en = (IME_FUNC_EN)ime_get_adas_enable();
		get_en = (IME_FUNC_EN)ime_get_adas_enable_reg();
		break;

	case IME_OSD0:
		//get_en = (IME_FUNC_EN)ime_get_osd_set0_enable();
		get_en = (IME_FUNC_EN)ime_get_osd_set0_enable_reg();
		break;

	case IME_OSD1:
		//get_en = (IME_FUNC_EN)ime_get_osd_set1_enable();
		get_en = (IME_FUNC_EN)ime_get_osd_set1_enable_reg();
		break;

	case IME_OSD2:
		//get_en = (IME_FUNC_EN)ime_get_osd_set2_enable();
		get_en = (IME_FUNC_EN)ime_get_osd_set2_enable_reg();
		break;

	case IME_OSD3:
		//get_en = (IME_FUNC_EN)ime_get_osd_set3_enable();
		get_en = (IME_FUNC_EN)ime_get_osd_set3_enable_reg();
		break;

	case IME_OUTPUT_P1_ENC:
		//ime_get_out_path_encoder_enable_p1(&get_en);
		get_en = (IME_FUNC_EN)ime_get_encode_enable_p1_reg();
		break;

	case IME_3DNR_DEC:
		//get_en = (IME_FUNC_EN)ime_get_tmnr_decoder_enable();
		get_en = (IME_FUNC_EN)ime_tmnr_get_out_ref_encoder_enable_reg();
		break;

	case IME_PM0:
		//get_en = (IME_FUNC_EN)ime_get_privacy_mask_enable(IME_PM_SET0);
		get_en = (IME_FUNC_EN)ime_get_privacy_mask_set0_enable_reg();
		break;

	case IME_PM1:
		//get_en = (IME_FUNC_EN)ime_get_privacy_mask_enable(IME_PM_SET1);
		get_en = (IME_FUNC_EN)ime_get_privacy_mask_set1_enable_reg();
		break;

	case IME_PM2:
		//get_en = (IME_FUNC_EN)ime_get_privacy_mask_enable(IME_PM_SET2);
		get_en = (IME_FUNC_EN)ime_get_privacy_mask_set2_enable_reg();
		break;

	case IME_PM3:
		//get_en = (IME_FUNC_EN)ime_get_privacy_mask_enable(IME_PM_SET3);
		get_en = (IME_FUNC_EN)ime_get_privacy_mask_set3_enable_reg();
		break;

	case IME_PM4:
		//get_en = (IME_FUNC_EN)ime_get_privacy_mask_enable(IME_PM_SET4);
		get_en = (IME_FUNC_EN)ime_get_privacy_mask_set4_enable_reg();
		break;

	case IME_PM5:
		//get_en = (IME_FUNC_EN)ime_get_privacy_mask_enable(IME_PM_SET5);
		get_en = (IME_FUNC_EN)ime_get_privacy_mask_set5_enable_reg();
		break;

	case IME_PM6:
		//get_en = (IME_FUNC_EN)ime_get_privacy_mask_enable(IME_PM_SET6);
		get_en = (IME_FUNC_EN)ime_get_privacy_mask_set6_enable_reg();
		break;

	case IME_PM7:
		//get_en = (IME_FUNC_EN)ime_get_privacy_mask_enable(IME_PM_SET7);
		get_en = (IME_FUNC_EN)ime_get_privacy_mask_set7_enable_reg();
		break;

	case IME_YUVCVT:
		//get_en = (IME_FUNC_EN)ime_get_yuv_converter_enable();
		get_en = (IME_FUNC_EN)ime_get_yuv_converter_enable_reg();
		break;

	default:
		get_en = IME_FUNC_DISABLE;
		break;
	}

	return get_en;
}
//-------------------------------------------------------------------------------


VOID ime_set_linked_list_addr(UINT32 set_addr)
{
	//UINT32 tmp_addr;

	//tmp_addr = dma_getPhyAddr(set_addr);

	ime_set_linked_list_addr_reg(set_addr);
}
//-------------------------------------------------------------------------------

VOID ime_set_linked_list_fire(VOID)
{
	ime_set_linked_list_fire_reg(1);
}
//-------------------------------------------------------------------------------

VOID ime_set_linked_list_terminate(VOID)
{
	ime_set_linked_list_terminate_reg(1);
}
//-------------------------------------------------------------------------------

ER ime_chg_single_output(IME_SINGLE_OUT_INFO *p_set_sout)
{
	ER er_return = E_OK;

	if (p_set_sout == NULL) {
		DBG_ERR("IME: single output parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_single_output_reg(p_set_sout->sout_enable, p_set_sout->sout_ch);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_get_single_output(IME_SINGLE_OUT_INFO *p_get_sout)
{
	UINT32 get_en = 0, get_chl_val = 0;

	if (p_get_sout == NULL) {
		DBG_ERR("IME: single output parameter NULL...\r\n");

		return E_PAR;
	}

	ime_get_single_output_reg(&get_en, &get_chl_val);

	p_get_sout->sout_enable = (IME_FUNC_EN)get_en;
	p_get_sout->sout_ch = get_chl_val;

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_chg_break_point(IME_BREAK_POINT_INFO *p_set_bp)
{
	ER er_return = E_OK;

	if (p_set_bp == NULL) {
		DBG_ERR("IME: break-point parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	if (nvt_get_chip_id() == CHIP_NA51055) {
		ime_set_break_point_mode_reg((UINT32)IME_BP_LINE_MODE);

		ime_set_line_based_break_point_reg(p_set_bp->bp1, p_set_bp->bp2, p_set_bp->bp3);
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		ime_set_break_point_mode_reg((UINT32)p_set_bp->bp_mode);

		if (p_set_bp->bp_mode == IME_BP_LINE_MODE) {
			ime_set_line_based_break_point_reg(p_set_bp->bp1, p_set_bp->bp2, p_set_bp->bp3);
		} else {
			ime_set_pixel_based_break_point_reg(p_set_bp->bp1, p_set_bp->bp2, p_set_bp->bp3);
		}
	} else {
        DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_chg_low_delay(IME_LOW_DELAY_INFO *p_set_ldy)
{
	ER er_return = E_OK;

	if (p_set_ldy == NULL) {
		DBG_ERR("IME: low delay mode parameter NULL...\r\n");

		return E_PAR;
	}

	// state machine
	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	ime_set_low_delay_mode_reg(p_set_ldy->dly_enable, p_set_ldy->dly_ch);

	ime_load();

	return E_OK;
}
//-------------------------------------------------------------------------------

// for direct mode change
ER ime_chg_direct_in_out_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//memset((VOID *)&ime_stripe_info, 0, sizeof(IME_HV_STRIPE_INFO));
	ime_stripe_info = p_engine_info->set_stripe;

	//-------------------------------------------------------------
	// set operating mode
	er_return = E_OK;
	ime_engine_operation_mode = p_engine_info->operation_mode;
	switch (p_engine_info->operation_mode) {
	case IME_OPMODE_D2D:
		//ime_set_in_path_source(IME_INSRC_DRAM);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_IME);

		ime_set_input_source_reg(IME_INSRC_DRAM);
		ime_set_direct_path_control_reg(SRC_CTRL_IME);
		break;

	case IME_OPMODE_IFE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_DCE);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_DCE);
		break;

	case IME_OPMODE_DCE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_DCE);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_DCE);
		break;

	case IME_OPMODE_SIE_IPP:
		//ime_set_in_path_source(IME_INSRC_IPE);
		//ime_set_in_path_direct_ctrl(SRC_CTRL_IME);

		ime_set_input_source_reg(IME_INSRC_IPE);
		ime_set_direct_path_control_reg(SRC_CTRL_IME);
		break;

	default:
		DBG_ERR("IME: engine mode error...\r\n");
		er_return = E_PAR;
		break;
	}
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// output path enable
	//ime_set_out_path_p1_enable(p_engine_info->out_path1.out_path_enable);
	//ime_set_out_path_p2_enable(p_engine_info->out_path2.out_path_enable);
	//ime_set_out_path_p3_enable(p_engine_info->out_path3.out_path_enable);
	//ime_set_out_path_p4_enable(p_engine_info->out_path4.out_path_enable);
	//ime_set_out_path_p5_enable(p_engine_info->out_path5.out_path_enable);

	ime_open_output_p1_reg((UINT32)p_engine_info->out_path1.out_path_enable);
	ime_open_output_p2_reg((UINT32)p_engine_info->out_path2.out_path_enable);
	ime_open_output_p3_reg((UINT32)p_engine_info->out_path3.out_path_enable);

	ime_set_output_p4_enable_reg((UINT32)p_engine_info->out_path4.out_path_enable);
	// #2015/06/12# adas control flow bug
	if (p_engine_info->out_path4.out_path_enable == IME_FUNC_DISABLE) {
		//ime_set_adas_enable(IME_FUNC_DISABLE);
		ime_set_adas_enable_reg(IME_FUNC_DISABLE);
	}
	// #2015/06/12# end


	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// set LCA enable
		if (p_engine_info->p_ime_iq_info->p_lca_info != NULL) {
			er_return = ime_chg_lca_enable(p_engine_info->p_ime_iq_info->p_lca_info->lca_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// set LCA subout enable
		if (p_engine_info->p_ime_iq_info->p_lca_subout_info != NULL) {
			er_return = ime_chg_lca_subout_enable(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// TMNR
		if (p_engine_info->p_ime_iq_info->p_tmnr_info != NULL) {
			er_return = ime_chg_tmnr_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->tmnr_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_refin_decoder_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_dec_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_refin_flip_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_flip_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_motion_sta_roi_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_motion_sta_roi_flip_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_flip_en);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//-------------------------------------------------------------
		// TMNR Reference output
		if (p_engine_info->p_ime_iq_info->p_tmnr_refout_info != NULL) {
			er_return = ime_chg_tmnr_ref_out_enable(p_engine_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_ref_out_encoder_enable(p_engine_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_enc_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_ref_out_flip_enable(p_engine_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_flip_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			ime_comps_tmnr_out_ref_encoder_shift_mode_enable_reg((UINT32)p_engine_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_enc_smode_enable);
		}

	}


	//-------------------------------------------------------------
	// set input path
	//memset((VOID *)&ime_set_input_path_info, 0, sizeof(ime_set_input_path_info));
	//ime_set_input_path_info = p_engine_info->in_path_info;
	//if (ime_set_input_path_info.in_format == IME_INPUT_RGB) {
	//  ime_set_input_path_info.in_format = IME_INPUT_YCC_444;
	//}
	//er_return = ime_chg_input_path_param(&ime_set_input_path_info);
	er_return = ime_chg_input_path_param(&(p_engine_info->in_path_info));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path1
	er_return = ime_chg_output_path_param(IME_PATH1_SEL, &(p_engine_info->out_path1));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path2
	er_return = ime_chg_output_path_param(IME_PATH2_SEL, &(p_engine_info->out_path2));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path3
	er_return = ime_chg_output_path_param(IME_PATH3_SEL, &(p_engine_info->out_path3));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path4
	er_return = ime_chg_output_path_param(IME_PATH4_SEL, &(p_engine_info->out_path4));
	if (er_return != E_OK) {
		return er_return;
	}

	//-------------------------------------------------------------
	// set output path5
#if 0
	er_return = ime_chg_output_path_param(IME_PATH5_SEL, &(p_engine_info->out_path5));
	if (er_return != E_OK) {
		return er_return;
	}
#endif

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// set LCA
		if (p_engine_info->p_ime_iq_info->p_lca_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_lca_info->lca_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_chroma_adapt_image_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				//er_return = ime_chg_chroma_adapt_chroma_adjust_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_chroma_adj_info));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_chroma_adapt_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_iq_chroma_info));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_chroma_adapt_luma_suppress_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_iq_luma_info));
				//if (er_return != E_OK) {
				//  return er_return;
				//}
			}
		}

		//-------------------------------------------------------------
		// set LCA subout
		if (p_engine_info->p_ime_iq_info->p_lca_subout_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_chroma_adapt_subout_source(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_src);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_subout_param(&(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_scale_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_subout_lineoffset_info(&(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_lofs));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_subout_dma_addr_info(&(p_engine_info->p_ime_iq_info->p_lca_subout_info->lca_subout_addr));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}



		//-------------------------------------------------------------
		// adas
		if (p_engine_info->p_ime_iq_info->p_sta_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_sta_info->stl_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_statistic_filter_enable(p_engine_info->p_ime_iq_info->p_sta_info->stl_filter_enable);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistical_image_output_type(p_engine_info->p_ime_iq_info->p_sta_info->stl_img_out_type);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_kernel_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map));
				if (er_return != E_OK) {
					return er_return;
				}

				//er_return = ime_chg_statistic_histogram_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist0));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_statistic_histogram_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist1));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_statistic_edge_roi_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_roi));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				er_return = ime_chg_statistic_edge_map_lineoffset(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map_lofs);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_map_addr(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_addr(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_flip_enable(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map_flip_enable);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		//-------------------------------------------------------------
		// TMNR
		if (p_engine_info->p_ime_iq_info->p_tmnr_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_tmnr_info->tmnr_en == IME_FUNC_ENABLE) {
				//er_return = ime_chg_tmnr_motion_estimation_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->me_param));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_tmnr_motion_detection_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->md_param));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_tmnr_motion_detection_roi_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->md_roi_param));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_tmnr_motion_compensation_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->mc_param));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_tmnr_motion_compensation_roi_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->mc_roi_param));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_tmnr_patch_selection_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->ps_param));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_tmnr_noise_filter_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->nr_param));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_tmnr_debug_ctrl_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->dbg_ctrl));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				er_return = ime_chg_tmnr_ref_in_lineoffset(&(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_ofs));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_ref_in_y_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_y);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_ref_in_uv_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_cb);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_status_lineoffset(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_ofs);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_status_in_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_in_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_status_out_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_out_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_en == IME_FUNC_ENABLE) {
					er_return = ime_chg_tmnr_motion_status_roi_out_lineoffset(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_ofs);
					if (er_return != E_OK) {
						return er_return;
					}

					er_return = ime_chg_tmnr_motion_status_roi_out_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_addr);
					if (er_return != E_OK) {
						return er_return;
					}
				}

				er_return = ime_chg_tmnr_motion_vector_lineoffset(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_vec_ofs);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_vector_in_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_vec_in_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_vector_out_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_vec_out_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_engine_info->p_ime_iq_info->p_tmnr_info->sta_param.sta_out_en == IME_FUNC_ENABLE) {
					er_return = ime_chg_tmnr_statistic_out_lineoffset(p_engine_info->p_ime_iq_info->p_tmnr_info->sta_out_ofs);
					if (er_return != E_OK) {
						return er_return;
					}

					er_return = ime_chg_tmnr_statistic_out_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->sta_out_addr);
					if (er_return != E_OK) {
						return er_return;
					}
				}

			}
		}

		//-------------------------------------------------------------
		// TMNR reference output
		if (p_engine_info->p_ime_iq_info->p_tmnr_refout_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_tmnr_ref_out_lineoffset(&(p_engine_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_lofs));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_ref_out_y_addr(p_engine_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_addr.addr_y);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_ref_out_uv_addr(p_engine_info->p_ime_iq_info->p_tmnr_refout_info->ref_out_addr.addr_cb);
				if (er_return != E_OK) {
					return er_return;
				}


			}
		}
	}

	//---------------------------------------------------------------------------------------------
	// set input and output image size
	//er_return = ime_set_in_path_image_size(&(p_eng_info->in_path_info.in_size));
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	if ((p_engine_info->in_path_info.in_size.size_h & 0x3) != 0) {
		DBG_ERR("IME: input, image size error, horizontal size is not 4x...\r\n");

		return E_PAR;
	}

	if (p_engine_info->in_path_info.in_size.size_h < 8) {
		//
		DBG_WRN("IME: input, image width less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_engine_info->in_path_info.in_size.size_v < 8) {
		//
		DBG_WRN("IME: input, image height less than (<) 8 is risky, please check!!!\r\n");
	}

	ime_set_input_image_size_reg(p_engine_info->in_path_info.in_size.size_h, p_engine_info->in_path_info.in_size.size_v);
	//---------------------------------------------------------------------------------------------

	if (p_engine_info->out_path1.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p1(p_engine_info->out_path1.out_path_scale_method);
		ime_set_scale_interpolation_method_p1_reg((UINT32)p_engine_info->out_path1.out_path_scale_method);

		//---------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p1(&(p_engine_info->out_path1.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path1.out_path_scale_size.size_h > 65535) || (p_engine_info->out_path1.out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path1, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path1.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path1, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path1.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path1, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p1_reg(p_engine_info->out_path1.out_path_scale_size.size_h, p_engine_info->out_path1.out_path_scale_size.size_v);

		//---------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p1(&(p_engine_info->out_path1.out_path_scale_size), &(p_engine_info->out_path1.out_path_out_size), &(p_engine_info->out_path1.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path1.out_path_out_size.size_h > 65535) || (p_engine_info->out_path1.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path1, horizontal or vertical crop size overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path1.out_path_out_size.size_h < 32) {
			//
			DBG_WRN("IME: path1, horizontal crop size less than (<) 32 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path1.out_path_out_size.size_v < 32) {
			//
			DBG_WRN("IME: path1, vertical crop size less than (<) 32 is risky, please check!!!\r\n");
		}

		if (((p_engine_info->out_path1.out_path_crop_pos.pos_x + p_engine_info->out_path1.out_path_out_size.size_h) > p_engine_info->out_path1.out_path_scale_size.size_h) || ((p_engine_info->out_path1.out_path_crop_pos.pos_y + p_engine_info->out_path1.out_path_out_size.size_v) > p_engine_info->out_path1.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path1, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p1_reg(p_engine_info->out_path1.out_path_out_size.size_h, p_engine_info->out_path1.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p1_reg(p_engine_info->out_path1.out_path_crop_pos.pos_x, p_engine_info->out_path1.out_path_crop_pos.pos_y);
	}

	if (p_engine_info->out_path2.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p2(p_engine_info->out_path2.out_path_scale_method);
		ime_set_scale_interpolation_method_p2_reg((UINT32)p_engine_info->out_path2.out_path_scale_method);

		//----------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p2(&(p_engine_info->out_path2.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path2.out_path_scale_size.size_h > 65535) || (p_engine_info->out_path2.out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path2, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path2.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path2, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path2.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path2, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p2_reg(p_engine_info->out_path2.out_path_scale_size.size_h, p_engine_info->out_path2.out_path_scale_size.size_v);

		//---------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p2(&(p_engine_info->out_path2.out_path_scale_size), &(p_engine_info->out_path2.out_path_out_size), &(p_engine_info->out_path2.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path2.out_path_out_size.size_h > 65535) || (p_engine_info->out_path2.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path2, horizontal or vertical crop size is overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path2.out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path2, horizontal crop size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path2.out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path2, vertical crop size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_engine_info->out_path2.out_path_crop_pos.pos_x + p_engine_info->out_path2.out_path_out_size.size_h) > p_engine_info->out_path2.out_path_scale_size.size_h) || ((p_engine_info->out_path2.out_path_crop_pos.pos_y + p_engine_info->out_path2.out_path_out_size.size_v) > p_engine_info->out_path2.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path2, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p2_reg(p_engine_info->out_path2.out_path_out_size.size_h, p_engine_info->out_path2.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p2_reg(p_engine_info->out_path2.out_path_crop_pos.pos_x, p_engine_info->out_path2.out_path_crop_pos.pos_y);
	}

	if (p_engine_info->out_path3.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p3(p_engine_info->out_path3.out_path_scale_method);
		ime_set_scale_interpolation_method_p3_reg((UINT32)p_engine_info->out_path3.out_path_scale_method);

		//--------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p3(&(p_engine_info->out_path3.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if (p_engine_info->out_path3.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path3, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path3.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path3, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p3_reg(p_engine_info->out_path3.out_path_scale_size.size_h, p_engine_info->out_path3.out_path_scale_size.size_v);

		//--------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p3(&(p_engine_info->out_path3.out_path_scale_size), &(p_engine_info->out_path3.out_path_out_size), &(p_engine_info->out_path3.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path3.out_path_out_size.size_h > 65535) || (p_engine_info->out_path3.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path3, output size overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path3.out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path3, horizontal output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path3.out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path3, vertical output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_engine_info->out_path3.out_path_crop_pos.pos_x + p_engine_info->out_path3.out_path_out_size.size_h) > p_engine_info->out_path3.out_path_scale_size.size_h) || ((p_engine_info->out_path3.out_path_crop_pos.pos_y + p_engine_info->out_path3.out_path_out_size.size_v) > p_engine_info->out_path3.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path3, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p3_reg(p_engine_info->out_path3.out_path_out_size.size_h, p_engine_info->out_path3.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p3_reg(p_engine_info->out_path3.out_path_crop_pos.pos_x, p_engine_info->out_path3.out_path_crop_pos.pos_y);
	}


	if (p_engine_info->out_path4.out_path_enable == ENABLE) {
		//ime_set_out_path_scale_method_p4(p_engine_info->out_path4.out_path_scale_method);
		ime_set_scale_interpolation_method_p4_reg((UINT32)p_engine_info->out_path4.out_path_scale_method);

		//------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p4(&(p_engine_info->out_path4.out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path4.out_path_scale_size.size_h > 65535) || (p_engine_info->out_path4.out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path4, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path4.out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path4, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path4.out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path4, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p4_reg(p_engine_info->out_path4.out_path_scale_size.size_h, p_engine_info->out_path4.out_path_scale_size.size_v);

		//------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p4(&(p_engine_info->out_path4.out_path_scale_size), &(p_engine_info->out_path4.out_path_out_size), &(p_engine_info->out_path4.out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_engine_info->out_path4.out_path_out_size.size_h > 65535) || (p_engine_info->out_path4.out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path4, horizontal or vertical crop size is overflow...\r\n");

			return E_PAR;
		}

		if (p_engine_info->out_path4.out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path4, horizontal output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_engine_info->out_path4.out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path4, vertical output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_engine_info->out_path4.out_path_crop_pos.pos_x + p_engine_info->out_path4.out_path_out_size.size_h) > p_engine_info->out_path4.out_path_scale_size.size_h) || ((p_engine_info->out_path4.out_path_crop_pos.pos_y + p_engine_info->out_path4.out_path_out_size.size_v) > p_engine_info->out_path4.out_path_scale_size.size_v)) {
			DBG_ERR("IME: path4, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p4_reg(p_engine_info->out_path4.out_path_out_size.size_h, p_engine_info->out_path4.out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p4_reg(p_engine_info->out_path4.out_path_crop_pos.pos_x, p_engine_info->out_path4.out_path_crop_pos.pos_y);
	}

#if 0
	if (p_engine_info->out_path5.out_path_enable == ENABLE) {
		ime_set_out_path_scale_method_p5(p_engine_info->out_path5.out_path_scale_method);

		er_return = ime_set_out_path_scale_size_p5(&(p_engine_info->out_path5.out_path_scale_size));
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_output_crop_size_p5(&(p_engine_info->out_path5.out_path_scale_size), &(p_engine_info->out_path5.out_path_out_size), &(p_engine_info->out_path5.out_path_crop_pos));
		if (er_return != E_OK) {
			return er_return;
		}
	}
#endif

	ime_load();

	return er_return;
}


ER ime_chg_direct_lca_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// set LCA enable
		if (p_engine_info->p_ime_iq_info->p_lca_info != NULL) {
			er_return = ime_chg_lca_enable(p_engine_info->p_ime_iq_info->p_lca_info->lca_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// set LCA
		if (p_engine_info->p_ime_iq_info->p_lca_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_lca_info->lca_enable == IME_FUNC_ENABLE) {
				//er_return = ime_chg_chroma_adapt_image_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_image_info));
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				er_return = ime_chg_chroma_adapt_chroma_adjust_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_chroma_adj_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_iq_chroma_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_luma_suppress_param(&(p_engine_info->p_ime_iq_info->p_lca_info->lca_iq_luma_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_chroma_adapt_bypass_enable(p_engine_info->p_ime_iq_info->p_lca_info->lca_bypass_enable);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}
	}

	ime_load();

	return er_return;
}

ER ime_chg_direct_dbcs_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// set DBCS enable
		if (p_engine_info->p_ime_iq_info->p_dbcs_info != NULL) {
			er_return = ime_chg_dark_bright_chroma_suppress_enable(p_engine_info->p_ime_iq_info->p_dbcs_info->dbcs_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// set DBCS
		if (p_engine_info->p_ime_iq_info->p_dbcs_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_dbcs_info->dbcs_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_dark_bright_chroma_suppress_param(&(p_engine_info->p_ime_iq_info->p_dbcs_info->dbcs_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}
	}

	ime_load();

	return er_return;
}


ER ime_chg_direct_osd_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// set data stamp enable
		if (p_engine_info->p_ime_iq_info->p_data_stamp_info != NULL) {
			// set0
			er_return = ime_chg_data_stamp_enable(IME_DS_SET0, p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set1
			er_return = ime_chg_data_stamp_enable(IME_DS_SET1, p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set2
			er_return = ime_chg_data_stamp_enable(IME_DS_SET2, p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set3
			er_return = ime_chg_data_stamp_enable(IME_DS_SET3, p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			// set CST enable
			er_return = ime_chg_data_stamp_cst_enable(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_cst_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// set data stamp
		if (p_engine_info->p_ime_iq_info->p_data_stamp_info != NULL) {
			UINT32 plt_en = 0;

			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_cst_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_color_coefs_param(&(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_cst_coef));
				if (er_return != E_OK) {
					return er_return;
				}
			}

			// set0
			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET0, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_data_stamp_param(IME_DS_SET0, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set0.ds_iq_info.ds_plt_enable == IME_FUNC_ENABLE) {
					plt_en += 1;
				}
			}

			// set1
			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET1, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_data_stamp_param(IME_DS_SET1, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set1.ds_iq_info.ds_plt_enable == IME_FUNC_ENABLE) {
					plt_en += 1;
				}
			}

			// set2
			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET2, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_data_stamp_param(IME_DS_SET2, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set2.ds_iq_info.ds_plt_enable == IME_FUNC_ENABLE) {
					plt_en += 1;
				}
			}

			// set3
			if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_data_stamp_image_param(IME_DS_SET3, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_image_info));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_data_stamp_param(IME_DS_SET3, &(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_iq_info));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_set3.ds_iq_info.ds_plt_enable == IME_FUNC_ENABLE) {
					plt_en += 1;
				}
			}

			//if (plt_en != 0) {
			//  er_return = ime_chg_data_stamp_color_palette_info(&(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_plt));
			//}

		}
	}

	ime_load();

	return er_return;
}

ER ime_chg_direct_osd_palette_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}


	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// set data stamp
		if (p_engine_info->p_ime_iq_info->p_data_stamp_info != NULL) {

			er_return = ime_chg_data_stamp_color_palette_info(&(p_engine_info->p_ime_iq_info->p_data_stamp_info->ds_plt));
		}
	}

	ime_load();

	return er_return;
}

ER ime_chg_direct_adas_stl_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// adas enable
		if (p_engine_info->p_ime_iq_info->p_sta_info != NULL) {
			er_return = ime_chg_statistic_enable(p_engine_info->p_ime_iq_info->p_sta_info->stl_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// adas
		if (p_engine_info->p_ime_iq_info->p_sta_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_sta_info->stl_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_statistic_filter_enable(p_engine_info->p_ime_iq_info->p_sta_info->stl_filter_enable);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistical_image_output_type(p_engine_info->p_ime_iq_info->p_sta_info->stl_img_out_type);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_kernel_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist0));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_histogram_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist1));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_statistic_edge_roi_param(&(p_engine_info->p_ime_iq_info->p_sta_info->stl_roi));
				if (er_return != E_OK) {
					return er_return;
				}

				//er_return = ime_chg_statistic_edge_map_lineoffset(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map_lofs);
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_statistic_edge_map_addr(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map_addr);
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_statistic_histogram_addr(p_engine_info->p_ime_iq_info->p_sta_info->stl_hist_addr);
				//if (er_return != E_OK) {
				//  return er_return;
				//}

				//er_return = ime_chg_statistic_edge_flip_enable(p_engine_info->p_ime_iq_info->p_sta_info->stl_edge_map_flip_enable);
				//if (er_return != E_OK) {
				//  return er_return;
				//}
			}
		}
	}

	ime_load();

	return er_return;
}

ER ime_chg_direct_pm_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// privacy mask
		if (p_engine_info->p_ime_iq_info->p_pm_info != NULL) {
			er_return = ime_chg_privacy_mask_enable(IME_PM_SET0, p_engine_info->p_ime_iq_info->p_pm_info->pm_set0.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET1, p_engine_info->p_ime_iq_info->p_pm_info->pm_set1.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET2, p_engine_info->p_ime_iq_info->p_pm_info->pm_set2.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET3, p_engine_info->p_ime_iq_info->p_pm_info->pm_set3.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET4, p_engine_info->p_ime_iq_info->p_pm_info->pm_set4.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET5, p_engine_info->p_ime_iq_info->p_pm_info->pm_set5.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET6, p_engine_info->p_ime_iq_info->p_pm_info->pm_set6.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_privacy_mask_enable(IME_PM_SET7, p_engine_info->p_ime_iq_info->p_pm_info->pm_set7.pm_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// privacy mask
		if (p_engine_info->p_ime_iq_info->p_pm_info != NULL) {
			UINT32 get_pm_enable = 0x0, get_pm_pxl_use = 0x0;

			if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set0.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET0, &(p_engine_info->p_ime_iq_info->p_pm_info->pm_set0));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set0.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set1.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET1, &(p_engine_info->p_ime_iq_info->p_pm_info->pm_set1));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set1.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set2.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET2, &(p_engine_info->p_ime_iq_info->p_pm_info->pm_set2));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set2.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set3.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET3, &(p_engine_info->p_ime_iq_info->p_pm_info->pm_set3));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set3.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set4.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET4, &(p_engine_info->p_ime_iq_info->p_pm_info->pm_set4));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set4.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set5.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET5, &(p_engine_info->p_ime_iq_info->p_pm_info->pm_set5));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set5.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set6.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET6, &(p_engine_info->p_ime_iq_info->p_pm_info->pm_set6));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set6.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set7.pm_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_privacy_mask_param(IME_PM_SET7, &(p_engine_info->p_ime_iq_info->p_pm_info->pm_set7));
				if (er_return != E_OK) {
					return er_return;
				}

				get_pm_enable += 1;

				if (p_engine_info->p_ime_iq_info->p_pm_info->pm_set7.pm_mask_type == IME_PM_MASK_TYPE_PXL) {
					get_pm_pxl_use += 1;
				}
			}

			if ((get_pm_enable != 0) && (get_pm_pxl_use != 0)) {
				er_return = ime_chg_privacy_mask_pixelation_image_size(&(p_engine_info->p_ime_iq_info->p_pm_info->pm_pxl_img_size));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_privacy_mask_pixelation_block_size(&(p_engine_info->p_ime_iq_info->p_pm_info->pm_pxl_blk_size));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_privacy_maask_pixelation_image_lineoffset(&(p_engine_info->p_ime_iq_info->p_pm_info->pm_pxl_lofs));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_privacy_maask_pixelation_image_dma_addr(&(p_engine_info->p_ime_iq_info->p_pm_info->pm_pxl_dma_addr));
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}
	}

	ime_load();

	return er_return;
}

ER ime_chg_direct_tmnr_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {


		//-------------------------------------------------------------
		// TMNR
		if (p_engine_info->p_ime_iq_info->p_tmnr_info != NULL) {
			er_return = ime_chg_tmnr_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->tmnr_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_refin_decoder_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_dec_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_refin_flip_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_flip_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_motion_sta_roi_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_en);
			if (er_return != E_OK) {
				return er_return;
			}

			er_return = ime_chg_tmnr_motion_sta_roi_flip_enable(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_flip_en);
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {
		//-------------------------------------------------------------
		// TMNR
		if (p_engine_info->p_ime_iq_info->p_tmnr_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_tmnr_info->tmnr_en == IME_FUNC_ENABLE) {
				er_return = ime_chg_tmnr_motion_estimation_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->me_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_detection_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->md_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_detection_roi_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->md_roi_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_compensation_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->mc_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_compensation_roi_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->mc_roi_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_patch_selection_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->ps_param));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_noise_filter_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->nr_param));
				if (er_return != E_OK) {
					return er_return;
				}

				if (p_engine_info->p_ime_iq_info->p_tmnr_info->sta_param.sta_out_en == IME_FUNC_ENABLE) {
					er_return = ime_chg_tmnr_statistic_data_output_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->sta_param));
					if (er_return != E_OK) {
						return er_return;
					}
				}

				er_return = ime_chg_tmnr_debug_ctrl_param(&(p_engine_info->p_ime_iq_info->p_tmnr_info->dbg_ctrl));
				if (er_return != E_OK) {
					return er_return;
				}

#if 0
				er_return = ime_chg_tmnr_ref_in_lineoffset(&(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_ofs));
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_ref_in_y_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_y);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_ref_in_uv_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->ref_in_addr.addr_cb);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_status_lineoffset(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_ofs);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_status_in_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_in_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_status_out_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_out_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_status_roi_out_lineoffset(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_ofs);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_status_roi_out_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_sta_roi_out_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_vector_lineoffset(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_vec_ofs);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_vector_in_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_vec_in_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_motion_vector_out_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->mot_vec_out_addr);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_statistic_out_lineoffset(p_engine_info->p_ime_iq_info->p_tmnr_info->sta_out_ofs);
				if (er_return != E_OK) {
					return er_return;
				}

				er_return = ime_chg_tmnr_statistic_out_addr(p_engine_info->p_ime_iq_info->p_tmnr_info->sta_out_addr);
				if (er_return != E_OK) {
					return er_return;
				}
#endif
			}
		}
	}

	ime_load();

	return er_return;
}


ER ime_chg_direct_break_point_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	er_return = ime_chg_break_point(&(p_engine_info->break_point));
	if (er_return != E_OK) {
		return er_return;
	}

	ime_load();

	return er_return;
}

ER ime_chg_direct_single_output_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	er_return = ime_chg_single_output(&(p_engine_info->single_out));
	if (er_return != E_OK) {
		return er_return;
	}

	ime_load();

	return er_return;
}


ER ime_chg_direct_low_delay_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	er_return = ime_chg_low_delay(&(p_engine_info->low_delay));
	if (er_return != E_OK) {
		return er_return;
	}

	ime_load();

	return er_return;
}


ER ime_chg_direct_interrupt_enable_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//ime_set_engine_control(IME_ENGINE_SET_INTPE, p_engine_info->interrupt_enable);
	ime_set_interrupt_enable_reg(p_engine_info->interrupt_enable);


	return er_return;
}

ER ime_chg_direct_yuv_cvt_param(IME_MODE_PRAM *p_engine_info)
{
	ER er_return = E_OK;

	if (p_engine_info == NULL) {
		DBG_ERR("IME: setting parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_state_machine(ime_engine_state_machine_status, IME_ACTOP_DYNAMICPARAM);
	if (er_return != E_OK) {
		DBG_ERR("State Machine Error ...\r\n");
		DBG_ERR("Current State : %d\r\n", (int)ime_engine_state_machine_status);
		DBG_ERR("Correct State : %d or %d\r\n", (int)IME_ENGINE_READY, (int)IME_ENGINE_PAUSE);

		return er_return;
	}

	//-------------------------------------------------------------
	// set function enable
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// set YUV Converter
		if (p_engine_info->p_ime_iq_info->p_yuv_cvt_info != NULL) {
			er_return = ime_chg_yuv_converter_enable(p_engine_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_enable);
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//-------------------------------------------------------------
	// set sub-function parameters
	if (p_engine_info->p_ime_iq_info != NULL) {

		//-------------------------------------------------------------
		// set YUV Converter
		if (p_engine_info->p_ime_iq_info->p_yuv_cvt_info != NULL) {
			if (p_engine_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_enable == IME_FUNC_ENABLE) {
				er_return = ime_chg_yuv_converter_param(p_engine_info->p_ime_iq_info->p_yuv_cvt_info->yuv_cvt_sel);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}
	}

	ime_load();

	return er_return;
}


ER ime_chg_builtin_isr_cb(IME_INT_CB fp)
{
#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() == ENABLE) {
		if (fp == NULL) {
			DBG_ERR("IME: user call-back function NULL...\r\n");

			return E_SYS;
		}

		ime_isr_fastboot_callback_fp = fp;
		ime_isr_non_fastboot_callback_fp = NULL;
		ime_isr_get_non_fastboot_callback_fp = NULL;
	} else {
		ime_isr_fastboot_callback_fp = NULL;
		ime_isr_non_fastboot_callback_fp = NULL;
		ime_isr_get_non_fastboot_callback_fp = NULL;
	}
#else
	ime_isr_fastboot_callback_fp = NULL;
	ime_isr_non_fastboot_callback_fp = NULL;
	ime_isr_get_non_fastboot_callback_fp = NULL;
#endif

	return E_OK;
}

BOOL ime_get_fsatboot_flow_ctrl_status(VOID)
{
	return ime_ctrl_flow_to_do;
}



#if (defined(_NVT_EMULATION_) == ON)
// add for emulation
VOID ime_emu_set_load(IME_LOAD_TYPE set_load_type)
{
	//ime_set_engine_control(IME_ENGINE_SET_LOAD, (UINT32)set_load_type);
	ime_load_reg((UINT32)set_load_type, ENABLE);

	DBG_DUMP("^Yime load: %d...\r\n", set_load_type);
}

// add for emulation
ER ime_emu_set_start(VOID)
{
	ER erReturn = E_OK;

	if (ime_engine_state_machine_status != IME_ENGINE_RUN) {
		erReturn = ime_check_input_path_stripe();
		if (erReturn != E_OK) {
			return erReturn;
		}
	}

	// after changing status of state machine, trigger engine to run
	//ime_set_engine_control(IME_ENGINE_SET_START, ENABLE);
	ime_start_reg(ENABLE);

	DBG_DUMP("^Yime start...\r\n");

	return E_OK;
}
//-------------------------------------------------------------------------------
#endif



#if 1 // for fast-buildin
#ifdef __KERNEL__

#if 0
VOID ime_set_base_addr(UINT32 uiAddr)
{
	_ime_reg_io_base = uiAddr;
}


VOID ime_create_resource(VOID)
{
	OS_CONFIG_FLAG(flg_id_ime);
	SEM_CREATE(semid_ime, 1);
}

VOID ime_release_resource(VOID)
{
	rel_flg(flg_id_ime);
	SEM_DESTROY(semid_ime);
}
#endif

//-----------------------------------------
// function export

//EXPORT_SYMBOL(ime_set_base_addr);
//EXPORT_SYMBOL(ime_create_resource);
//EXPORT_SYMBOL(ime_release_resource);



EXPORT_SYMBOL(ime_is_open);

EXPORT_SYMBOL(ime_open);

EXPORT_SYMBOL(ime_set_mode);

EXPORT_SYMBOL(ime_start);

EXPORT_SYMBOL(ime_pause);

EXPORT_SYMBOL(ime_close);

EXPORT_SYMBOL(ime_reset);

EXPORT_SYMBOL(ime_isr);

EXPORT_SYMBOL(ime_waitFlagFrameEnd);

EXPORT_SYMBOL(ime_clear_flag_frame_end);

EXPORT_SYMBOL(ime_waitFlagLinkedListEnd);

EXPORT_SYMBOL(ime_clearFlagLinkedListEnd);

EXPORT_SYMBOL(ime_waitFlagLinkedListJobEnd);

EXPORT_SYMBOL(ime_clearFlagLinkedListJobEnd);

EXPORT_SYMBOL(ime_waitFlagBreakPoint_BP1);

EXPORT_SYMBOL(ime_clearFlagBreakPoint_BP1);

EXPORT_SYMBOL(ime_waitFlagBreakPoint_BP2);

EXPORT_SYMBOL(ime_clearFlagBreakPoint_BP2);

EXPORT_SYMBOL(ime_waitFlagBreakPoint_BP3);

EXPORT_SYMBOL(ime_clearFlagBreakPoint_BP3);

EXPORT_SYMBOL(ime_chg_stripe_param);

EXPORT_SYMBOL(ime_cal_d2d_hstripe_size);

EXPORT_SYMBOL(ime_get_input_path_stripe_info);

EXPORT_SYMBOL(ime_chg_input_path_param);

EXPORT_SYMBOL(ime_chg_output_path_param);

EXPORT_SYMBOL(ime_chg_output_path_enable);

EXPORT_SYMBOL(ime_chg_output_path_dram_out_enable);

EXPORT_SYMBOL(ime_chg_output_scaling_filter_param);

//EXPORT_SYMBOL(ime_chg_output_scaling_factor_param);

EXPORT_SYMBOL(ime_chg_output_scaling_enhance_param);

EXPORT_SYMBOL(ime_chg_output_path_range_clamp_param);

EXPORT_SYMBOL(ime_chg_lca_enable);
EXPORT_SYMBOL(ime_chg_chroma_adapt_chroma_adjust_enable);
EXPORT_SYMBOL(ime_chg_lca_luma_suppress_enable);
EXPORT_SYMBOL(ime_chg_chroma_adapt_image_param);
EXPORT_SYMBOL(ime_chg_chroma_adapt_bypass_enable);

EXPORT_SYMBOL(ime_chg_chroma_adapt_chroma_adjust_param);

EXPORT_SYMBOL(ime_chg_chroma_adapt_param);

EXPORT_SYMBOL(ime_chg_chroma_adapt_luma_suppress_param);

EXPORT_SYMBOL(ime_get_chroma_adapt_image_size_info);

EXPORT_SYMBOL(ime_get_chroma_adapt_lineoffset_info);

EXPORT_SYMBOL(ime_get_chroma_adapt_dma_addr_info);

EXPORT_SYMBOL(ime_chg_lca_subout_enable);

EXPORT_SYMBOL(ime_chg_chroma_adapt_subout_source);

EXPORT_SYMBOL(ime_chg_chroma_adapt_subout_param);

EXPORT_SYMBOL(ime_chg_chroma_adapt_subout_lineoffset_info);

EXPORT_SYMBOL(ime_get_chroma_adapt_subout_lineoffset_info);

EXPORT_SYMBOL(ime_chg_chroma_adapt_subout_dma_addr_info);

EXPORT_SYMBOL(ime_get_chroma_adapt_subout_dma_addr_info);

EXPORT_SYMBOL(ime_chg_dark_bright_chroma_suppress_enable);

EXPORT_SYMBOL(ime_chg_dark_bright_chroma_suppress_param);

//EXPORT_SYMBOL(ime_chg_film_grain_noise_enable);

//EXPORT_SYMBOL(ime_chg_film_grain_noise_param);

EXPORT_SYMBOL(ime_chg_data_stamp_cst_enable);

EXPORT_SYMBOL(ime_chg_data_stamp_color_key_enable);

EXPORT_SYMBOL(ime_chg_data_stamp_enable);

EXPORT_SYMBOL(ime_chg_data_stamp_image_param);

EXPORT_SYMBOL(ime_chg_data_stamp_param);

EXPORT_SYMBOL(ime_chg_data_stamp_color_coefs_param);

EXPORT_SYMBOL(ime_chg_data_stamp_color_palette_info);

EXPORT_SYMBOL(ime_get_data_stamp_image_size_info);

EXPORT_SYMBOL(ime_get_data_stamp_lineoffset_info);

EXPORT_SYMBOL(ime_get_data_stamp_dma_addr_info);

EXPORT_SYMBOL(ime_chg_privacy_mask_enable);

EXPORT_SYMBOL(ime_chg_privacy_mask_param);

EXPORT_SYMBOL(ime_chg_privacy_mask_pixelation_image_size);

EXPORT_SYMBOL(ime_chg_privacy_mask_pixelation_block_size);

EXPORT_SYMBOL(ime_chg_privacy_maask_pixelation_image_lineoffset);

EXPORT_SYMBOL(ime_chg_privacy_maask_pixelation_image_dma_addr);

EXPORT_SYMBOL(ime_get_privacy_mask_pixelation_image_size_info);

EXPORT_SYMBOL(ime_get_privacy_mask_pixelation_image_lineoffset_info);

EXPORT_SYMBOL(ime_get_privacy_mask_pixelation_image_dma_addr_info);

EXPORT_SYMBOL(ime_get_engine_info);

EXPORT_SYMBOL(ime_get_input_path_image_size_info);

EXPORT_SYMBOL(ime_get_input_path_lineoffset_info);

EXPORT_SYMBOL(ime_get_input_path_dma_addr_info);

EXPORT_SYMBOL(ime_get_output_path_image_size_info);

EXPORT_SYMBOL(ime_get_output_path_lineoffset_info);

EXPORT_SYMBOL(ime_get_output_path_dma_addr_info);

EXPORT_SYMBOL(ime_chg_statistic_enable);

EXPORT_SYMBOL(ime_chg_statistic_filter_enable);

EXPORT_SYMBOL(ime_chg_statistical_image_output_type);

EXPORT_SYMBOL(ime_chg_statistic_edge_kernel_param);

EXPORT_SYMBOL(ime_chg_statistic_histogram_param);

EXPORT_SYMBOL(ime_chg_statistic_edge_map_lineoffset);

EXPORT_SYMBOL(ime_chg_statistic_edge_map_addr);

EXPORT_SYMBOL(ime_chg_statistic_histogram_addr);

EXPORT_SYMBOL(ime_chg_statistic_edge_roi_param);

EXPORT_SYMBOL(ime_get_statistic_info);

EXPORT_SYMBOL(ime_get_statistic_edge_map_dma_addr_info);

EXPORT_SYMBOL(ime_get_statistic_edge_map_lineoffset_info);

EXPORT_SYMBOL(ime_get_statistic_histogram_dma_addr_info);

EXPORT_SYMBOL(ime_chg_yuv_converter_enable);

EXPORT_SYMBOL(ime_chg_yuv_converter_param);

EXPORT_SYMBOL(ime_get_output_yuv_data_type);

EXPORT_SYMBOL(ime_chg_stitching_enable);

EXPORT_SYMBOL(ime_chg_stitching_image_param);



EXPORT_SYMBOL(ime_chg_tmnr_enable);
EXPORT_SYMBOL(ime_chg_tmnr_luma_enable);
EXPORT_SYMBOL(ime_chg_tmnr_chroma_enable);
EXPORT_SYMBOL(ime_chg_tmnr_refin_decoder_enable);
EXPORT_SYMBOL(ime_chg_tmnr_refin_flip_enable);
EXPORT_SYMBOL(ime_chg_tmnr_motion_sta_roi_flip_enable);
EXPORT_SYMBOL(ime_chg_tmnr_motion_sta_roi_enable);
EXPORT_SYMBOL(ime_chg_tmnr_motion_estimation_param);
EXPORT_SYMBOL(ime_chg_tmnr_motion_detection_param);
EXPORT_SYMBOL(ime_chg_tmnr_motion_detection_roi_param);
EXPORT_SYMBOL(ime_chg_tmnr_motion_compensation_param);
EXPORT_SYMBOL(ime_chg_tmnr_motion_compensation_roi_param);
EXPORT_SYMBOL(ime_chg_tmnr_patch_selection_param);
EXPORT_SYMBOL(ime_chg_tmnr_noise_filter_param);
EXPORT_SYMBOL(ime_chg_tmnr_statistic_data_output_param);
EXPORT_SYMBOL(ime_chg_tmnr_debug_ctrl_param);
EXPORT_SYMBOL(ime_chg_tmnr_ref_in_lineoffset);
EXPORT_SYMBOL(ime_chg_tmnr_ref_in_y_addr);
EXPORT_SYMBOL(ime_chg_tmnr_ref_in_uv_addr);
EXPORT_SYMBOL(ime_get_tmnr_ref_in_y_addr);
EXPORT_SYMBOL(ime_get_tmnr_ref_in_uv_addr);
EXPORT_SYMBOL(ime_chg_tmnr_motion_status_lineoffset);
EXPORT_SYMBOL(ime_chg_tmnr_motion_status_in_addr);
EXPORT_SYMBOL(ime_chg_tmnr_motion_status_out_addr);
EXPORT_SYMBOL(ime_get_tmnr_motion_status_in_addr);
EXPORT_SYMBOL(ime_get_tmnr_motion_status_out_addr);
EXPORT_SYMBOL(ime_chg_tmnr_motion_status_roi_out_lineoffset);
EXPORT_SYMBOL(ime_chg_tmnr_motion_status_roi_out_addr);
EXPORT_SYMBOL(ime_get_tmnr_motion_status_roi_out_addr);
EXPORT_SYMBOL(ime_chg_tmnr_motion_vector_lineoffset);
EXPORT_SYMBOL(ime_chg_tmnr_motion_vector_in_addr);
EXPORT_SYMBOL(ime_chg_tmnr_motion_vector_out_addr);
EXPORT_SYMBOL(ime_get_tmnr_motion_vector_in_addr);
EXPORT_SYMBOL(ime_get_tmnr_motion_vector_out_addr);
EXPORT_SYMBOL(ime_chg_tmnr_statistic_out_lineoffset);
EXPORT_SYMBOL(ime_chg_tmnr_statistic_out_addr);
EXPORT_SYMBOL(ime_get_tmnr_statistic_out_addr);

EXPORT_SYMBOL(ime_chg_tmnr_ref_out_enable);
EXPORT_SYMBOL(ime_chg_tmnr_ref_out_encoder_enable);
EXPORT_SYMBOL(ime_chg_tmnr_ref_out_flip_enable);
EXPORT_SYMBOL(ime_chg_tmnr_ref_out_lineoffset);
EXPORT_SYMBOL(ime_chg_tmnr_ref_out_y_addr);
EXPORT_SYMBOL(ime_chg_tmnr_ref_out_uv_addr);
EXPORT_SYMBOL(ime_get_tmnr_ref_out_y_addr);
EXPORT_SYMBOL(ime_get_tmnr_ref_out_uv_addr);

EXPORT_SYMBOL(ime_chg_compression_param);
//EXPORT_SYMBOL(ime_get_clock_rate);
EXPORT_SYMBOL(ime_chg_burst_length);
EXPORT_SYMBOL(ime_get_input_max_stripe_size);
//EXPORT_SYMBOL(ime_chg_all_param);
EXPORT_SYMBOL(ime_get_burst_length);
EXPORT_SYMBOL(ime_get_output_path_info);
EXPORT_SYMBOL(ime_get_input_path_info);

EXPORT_SYMBOL(ime_get_func_enable_info);

EXPORT_SYMBOL(ime_set_linked_list_addr);
EXPORT_SYMBOL(ime_set_linked_list_fire);
EXPORT_SYMBOL(ime_set_linked_list_terminate);

EXPORT_SYMBOL(ime_get_single_output);
EXPORT_SYMBOL(ime_chg_single_output);
EXPORT_SYMBOL(ime_chg_break_point);
EXPORT_SYMBOL(ime_chg_low_delay);

EXPORT_SYMBOL(ime_chg_direct_in_out_param);
EXPORT_SYMBOL(ime_chg_direct_lca_param);
EXPORT_SYMBOL(ime_chg_direct_dbcs_param);
EXPORT_SYMBOL(ime_chg_direct_osd_param);
EXPORT_SYMBOL(ime_chg_direct_osd_palette_param);
EXPORT_SYMBOL(ime_chg_direct_adas_stl_param);
EXPORT_SYMBOL(ime_chg_direct_pm_param);
EXPORT_SYMBOL(ime_chg_direct_tmnr_param);

EXPORT_SYMBOL(ime_chg_direct_break_point_param);
EXPORT_SYMBOL(ime_chg_direct_single_output_param);
EXPORT_SYMBOL(ime_chg_direct_low_delay_param);
EXPORT_SYMBOL(ime_chg_direct_interrupt_enable_param);
EXPORT_SYMBOL(ime_chg_direct_yuv_cvt_param);

EXPORT_SYMBOL(ime_chg_builtin_isr_cb);
EXPORT_SYMBOL(ime_get_fsatboot_flow_ctrl_status);
EXPORT_SYMBOL(ime_set_builtin_flow_disable);

EXPORT_SYMBOL(ime_chg_tmnr_ref_in_addr_use_va2pa);

#endif
#endif


//@}



