/*
    NUE2 module driver

    NT98520 NUE2 driver.

    @file       nue2_lib.c
    @ingroup    mIIPPNUE2

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "kwrap/semaphore.h"
#include "nue2_platform.h"
#include "nue2_lib.h"
#include "nue2_reg.h"
#include "nue2_int.h"
#include "kwrap/task.h"

//=============================================================
#define __CLASS__ 				"[ai][kdrv][nue2]"
#include "kdrv_ai_debug.h"
//=============================================================


#define NUE2_PRINT_PARM                DISABLE
#define NUE2_PRINT_REG                 DISABLE

#define FLGPTN_NUE2_FRMEND             FLGPTN_BIT(0)
#define FLGPTN_NUE2_DMAIN0END          FLGPTN_BIT(1)
#define FLGPTN_NUE2_DMAIN1END          FLGPTN_BIT(2)
#define FLGPTN_NUE2_DMAIN2END          FLGPTN_BIT(3)
#define FLGPTN_NUE2_LLEND              FLGPTN_BIT(8)
#define FLGPTN_NUE2_LLERR              FLGPTN_BIT(9)
#define FLGPTN_NUE2_LLJOBEND           FLGPTN_BIT(10)  //CHIP:528
#define FLGPTN_NUE2_SWRESET            FLGPTN_BIT(16)

static SEM_HANDLE SEMID_NUE2;
static SEM_HANDLE SEMID_NUE2_LIB;

#define NUE2_MODULE_CLK_NUM 1
UINT32 nue2_reg_base_addr    = 0;
UINT32 nue2_ll_base_addr     = 0;
static UINT32 nue2_status    = NUE2_ENGINE_IDLE;
static ID g_nue2_lock_status = NO_TASK_LOCKED;
UINT32 g_nue2_enable_cnt = 0;
BOOL g_nue2_cb_flg = 0;
static UINT32 g_nue2_dma_abort_delay=1000;

UINT32 nue2_perf_en     = 0;
UINT32 nue2_perf_status = 0;
UINT32 nue2_eng_time    = 0;
#if defined(__LINUX)
static struct timeval nue2_tstart, nue2_tend;
static struct timeval nue2_total_tstart, nue2_total_tend;
#endif
UINT32 nue2_total_time = 0;
static VOID (*p_nue2_int_cb)(UINT32 int_status);

// ISR
extern void nue2_isr(void);

// Control
static ER nue2_chk_state_machine(UINT32 operation, BOOL update);
static ER nue2_set_clock_rate(UINT32 clock);
static ER nue2_lock(void);
static ER nue2_unlock(void);
static void nue2_attach(void);
static void nue2_detach(void);
static VOID nue2_wait_dma_abort(VOID);

//set parameters
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
static ER nue2_set_para2eng(NUE2_OPMODE mode, NUE2_PARM *p_parm);
#endif
//get info
VOID nue2_clr_all(VOID);
//void nue2_clr_frameend(void);

VOID nue2_set_perf_en(UINT32 enable)
{
#if defined(__LINUX)
	if (enable) {
		nue2_perf_en = enable;
		nue2_eng_time = 0;
		do_gettimeofday(&nue2_total_tstart);
	} else {
		if (nue2_perf_en == 1) {
			do_gettimeofday(&nue2_total_tend); 
			nue2_total_time = (nue2_total_tend.tv_sec - nue2_total_tstart.tv_sec) * 1000000 + (nue2_total_tend.tv_usec - nue2_total_tstart.tv_usec);
			nue2_perf_en = 0;
		}
	}
#endif
}

UINT32 nue2_get_eng_time(VOID)
{
	return nue2_eng_time;
}

UINT32 nue2_get_eng_util(VOID)
{
	if (nue2_total_time == 0) {
		return 0;
	}
	
	return 100*nue2_eng_time/nue2_total_time;
}
/*
    NUE2 State Machine

    NUE2 state machine


    @param[in] operation the operation to execute
    @param[in] update if we will update state machine status

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_chk_state_machine(UINT32 operation, BOOL update)
{
	ER er_return;

	switch (nue2_status) {
	case NUE2_ENGINE_IDLE:
		switch (operation) {
		case NUE2_OP_ATTACH:
			if (update) {
				nue2_status = NUE2_ENGINE_IDLE;
			}
			er_return = E_OK;
			break;
		case NUE2_OP_DETACH:
			if (update) {
				nue2_status = NUE2_ENGINE_IDLE;
			}
			er_return = E_OK;
			break;
		case NUE2_OP_OPEN:
			if (update) {
				nue2_status = NUE2_ENGINE_READY;
			}
			er_return = E_OK;
			break;
		default:
			er_return = E_OBJ;
			break;
		}
		break;

	case NUE2_ENGINE_READY:
		switch (operation) {
		case NUE2_OP_CLOSE:
			if (update) {
				nue2_status = NUE2_ENGINE_IDLE;
			}
			er_return = E_OK;
			break;
		case NUE2_OP_SET2READY:
		case NUE2_OP_READLUT:
		case NUE2_OP_CHGINT:
			if (update) {
				nue2_status = NUE2_ENGINE_READY;
			}
			er_return = E_OK;
			break;
		case NUE2_OP_SET2PAUSE:
			if (update) {
				nue2_status = NUE2_ENGINE_PAUSE;
			}
			er_return = E_OK;
			break;
		default:
			er_return = E_OBJ;
			break;
		}
		break;

	case NUE2_ENGINE_PAUSE:
		switch (operation) {
		case NUE2_OP_SET2PAUSE:
		case NUE2_OP_READLUT:
		case NUE2_OP_CHGINT:
			if (update) {
				nue2_status = NUE2_ENGINE_PAUSE;
			}
			er_return = E_OK;
			break;
		case NUE2_OP_SET2RUN:
			if (update) {
				nue2_status = NUE2_ENGINE_RUN;
			}
			er_return = E_OK;
			break;
		case NUE2_OP_SET2READY:
			if (update) {
				nue2_status = NUE2_ENGINE_READY;
			}
			er_return = E_OK;
			break;
		case NUE2_OP_CLOSE:
			if (update) {
				nue2_status = NUE2_ENGINE_IDLE;
			}
			er_return = E_OK;
			break;
		default :
			er_return = E_OBJ;
			break;
		}
		break;

	case NUE2_ENGINE_RUN:
		switch (operation) {
		case NUE2_OP_SET2RUN:
		case NUE2_OP_CHGINT:
			if (update) {
				nue2_status = NUE2_ENGINE_RUN;
			}
			er_return = E_OK;
			break;
		case NUE2_OP_SET2PAUSE:
			if (update) {
				nue2_status = NUE2_ENGINE_PAUSE;
			}
			er_return = E_OK;
			break;
		default :
			er_return = E_OBJ;
			break;
		}
		break;
	default:
		er_return = E_OBJ;
	}
	if (er_return != E_OK) {
		DBG_ERR("State machine error! st %d op %d\r\n", (int)nue2_status, (int)operation);
	}

	return er_return;
}

/**
    NUE2 change interrupt

    NUE2 interrupt configuration.

    for FPGA verification only

    @param[in] int_en Interrupt enable setting.

    @return ER NUE2 change interrupt status.
*/
ER nue2_change_interrupt(UINT32 int_en)
{
	ER er_return = E_OK;
	er_return = nue2_chk_state_machine(NUE2_OP_CHGINT, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	nue2_enable_int(ENABLE, int_en);
	er_return = nue2_chk_state_machine(NUE2_OP_CHGINT, TRUE);
	return er_return;
}

#if NUE2_CYCLE_TEST
UINT64 nue2_total_cycle = 0;
#if NUE2_CYCLE_LAYER
UINT32 nue2_cnt = 0;
UINT32 nue2_dbg_cnt[20] = {0};
#endif
UINT64 nue2_dump_total_cycle(UINT32 debug_layer)
{
	UINT64 total_cost = nue2_total_cycle;
    #if NUE2_CYCLE_LAYER
    if(debug_layer==1) {
        UINT32 i = 0;
    	for (i = 0; i < nue2_cnt; i++) {
    		DBG_DUMP("item %d nue2 cycle = %d\n", i, (unsigned int)nue2_dbg_cnt[i]);
    	}
    }
    //#else
        //DBG_DUMP("NUE2_CYCLE_LAYER is disable now.\n");
    #endif
	//DBG_DUMP("nue total cycle = 0x%08X%08X\n", (unsigned int)(nue_total_cycle >> 32), (unsigned int)(nue_total_cycle & 0xFFFFFFFF));
	nue2_total_cycle = 0;

	return total_cost;
}
#endif

/*
    NUE2 Interrupt Service Routine

    NUE2 interrupt service routine

    @return None.
*/
void nue2_isr(void)
{
	UINT32 nue2_int_status = nue2_get_intr_status();
	nue2_int_status = nue2_int_status & nue2_get_int_enable();

	if (nue2_int_status == 0) {
		return;
	}
	nue2_clr_intr_status(nue2_int_status);

	if (nue2_int_status & NUE2_INTE_FRMEND) {
		nue2_pt_iset_flg(FLGPTN_NUE2_FRMEND);
        #if NUE2_CYCLE_TEST
        #if NUE2_CYCLE_LAYER
        nue2_dbg_cnt[nue2_cnt++] = (unsigned int)nue2_getcycle();
        #endif
		nue2_total_cycle += nue2_getcycle();
		#endif
	}
	if (nue2_int_status & NUE2_INTE_DMAIN0END) {
		nue2_pt_iset_flg(FLGPTN_NUE2_DMAIN0END);
	}
	if (nue2_int_status & NUE2_INTE_DMAIN1END) {
		nue2_pt_iset_flg(FLGPTN_NUE2_DMAIN1END);
	}
	if (nue2_int_status & NUE2_INTE_DMAIN2END) {
		nue2_pt_iset_flg(FLGPTN_NUE2_DMAIN2END);
	}
	if (nue2_int_status & NUE2_INTE_LLEND) {
#if defined(__LINUX)
		if (nue2_perf_status == 1) {
			do_gettimeofday(&nue2_tend); 
			nue2_eng_time += ((nue2_tend.tv_sec - nue2_tstart.tv_sec) * 1000000 + (nue2_tend.tv_usec - nue2_tstart.tv_usec));
			nue2_perf_status = 0;
		}
#endif
		nue2_pt_iset_flg(FLGPTN_NUE2_LLEND);
	}
	if (nue2_int_status & NUE2_INTE_LLERR) {
		DBG_ERR("Illegal linked list setting\r\n");
		nue2_pt_iset_flg(FLGPTN_NUE2_LLERR);
	}
    if (nue2_int_status & NUE2_INTE_SWRESET) {
		nue2_pt_iset_flg(FLGPTN_NUE2_SWRESET);
	}
#if 1
	if(nvt_get_chip_id() != CHIP_NA51055) {
		if (nue2_int_status & NUE2_INTE_LLJOBEND) {
			nue2_pt_iset_flg(FLGPTN_NUE2_LLJOBEND);
		}
	}
#endif
	if (p_nue2_int_cb) {
		p_nue2_int_cb(nue2_int_status);
	}

	return;
}

/**
    NUE2 get operation clock

    Get NUE2 clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static UINT32 nue2_get_clock_rate(VOID)
{
	UINT32 clk_freq_mhz;

	clk_freq_mhz = nue2_pt_get_clock_rate();

	return clk_freq_mhz;
}

/**
    NUE2 set operation clock

    Set NUE2 clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER nue2_set_clock_rate(UINT32 clock)
{
	ER er_return;

	er_return = nue2_pt_set_clock_rate(clock);

	return er_return;
}

/**
    NUE2 Lock Function

    Lock NUE2 engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER nue2_lock(void)
{
#if 0
	ER er_return;

	if (g_nue2_lock_status == NO_TASK_LOCKED) {
		er_return = wai_sem(SEMID_NUE2);
		if (er_return != E_OK) {
			return er_return;
		}
		g_nue2_lock_status = TASK_LOCKED;
		return E_OK;
	} else {
		return E_OBJ;
	}
#else
	ER er_return=E_OK;

#if (KDRV_AI_MINI_NO_LOCK == 1)
#else
	er_return = SEM_WAIT(SEMID_NUE2);
	if (er_return != E_OK) {
		return er_return;
	}
#endif //#if (KDRV_AI_MINI_NO_LOCK == 1)
	g_nue2_lock_status = TASK_LOCKED;
	return er_return;
#endif
}

/**
    NUE2 Unlock Function

    Unlock NUE2 engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER nue2_unlock(void)
{
#if 0
	if (g_nue2_lock_status == TASK_LOCKED) {
		g_nue2_lock_status = NO_TASK_LOCKED;
		return sig_sem(SEMID_NUE2);
	} else {
		return E_OBJ;
	}
#else
	g_nue2_lock_status = NO_TASK_LOCKED;
#if (KDRV_AI_MINI_NO_LOCK == 1)
#else
	SEM_SIGNAL(SEMID_NUE2);
#endif //#if (KDRV_AI_MINI_NO_LOCK == 1)
	return E_OK;
#endif
}

/**
    NUE2 Lock Function

    Lock NUE2 engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER nue2_lib_lock(void)
{
	ER er_return = SEM_WAIT(SEMID_NUE2_LIB);

	return er_return;

}

/**
    NUE2 Unlock Function

    Unlock NUE2 engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER nue2_lib_unlock(void)
{
	SEM_SIGNAL(SEMID_NUE2_LIB);
	return E_OK;
}

/**
    NUE2 Attach Function

    Pre-initialize driver required HW before driver open
    \n This function should be called before calling any other functions

    @return None
*/
static void nue2_attach(void)
{
	nue2_pt_clk_enable();
}

/**
    NUE2 Detach Function

    De-initialize driver required HW after driver close
    \n This function should be called before calling any other functions

    @return None
*/
static void nue2_detach(void)
{
	nue2_pt_clk_disable();
}

/**
    NUE2 Open Driver

    Open NUE2 engine

    @param[in] p_obj_cb NUE2 open object

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_open(NUE2_OPENOBJ *p_obj_cb)
{
	ER er_return;
	BOOL is_dma_disable;

	// Lock semaphore
	er_return = nue2_lock();
	if (er_return != E_OK) {
		return er_return;
	}

	// Check state-machine
	er_return = nue2_chk_state_machine(NUE2_OP_OPEN, FALSE);
	if (er_return != E_OK) {
		// release resource when stateMachine fail
		nue2_unlock();
		return er_return;
	}
#if 0
#if defined(__FREERTOS)
	nue2_set_clock_rate(240); //240,320,480,xx
#else
#if KERNEL_USAGE
	if (p_obj_cb != NULL) {
		er_return = nue2_set_clock_rate(480);
	}
	if (er_return != E_OK) {
		er_return = nue2_unlock();
		return er_return;
	}
#else
	// Select PLL clock source & enable PLL clock source
	if (p_obj_cb != NULL)
		nue2_set_clock_rate(p_obj_cb->clk_sel);
	pll_setClockRate(PLL_CLKSEL_NUE2, nue2_pt_get_true_clock());
#endif
#endif

	// Attach
	nue2_attach();

#if KERNEL_USAGE
	nvt_disable_sram_shutdown(NUE2_SD);
#else
	// Disable Sram shut down
	pinmux_disableSramShutDown(NUE2_SD);
	// Enable engine interrupt
	drv_enableInt(DRV_INT_NUE2);
#endif

	// Clear engine interrupt status
	nue2_clr_intr_status(NUE2_INT_ALL);
	// SW reset
	nue2_clr(0);
	nue2_clr(1);
	nue2_clr(0);
#else

#if (NUE2_AI_FLOW == ENABLE)
#else
	er_return = (ER) nue2_init();
#endif

#endif

	// Clear engine interrupt status
	if(nvt_get_chip_id() == CHIP_NA51055) {
    	nue2_clr_intr_status(NUE2_INT_ALL);
	} else {
		nue2_clr_intr_status(NUE2_INT_ALL_528);
	}

	nue2_clr_all();

	// Hook call-back function
	if ((p_obj_cb != NULL) && (p_obj_cb->fp_isr_cb != NULL)) {
		p_nue2_int_cb = p_obj_cb->fp_isr_cb;
	} else {
		p_nue2_int_cb = NULL;
	}

	is_dma_disable = nue2_get_dma_disable();
    if (is_dma_disable == 1) {
        DBG_IND("NUE2_OPEN: it has been in dma_disable state.\r\n");
    } else {
		// SW reset
		nue2_clr(0);
		nue2_clr(1);
		nue2_clr(0);
	}

	// Update state-machine
	er_return = nue2_chk_state_machine(NUE2_OP_OPEN, TRUE);
	if (er_return != E_OK) {
		return er_return;
	}

	nue2_pt_request_irq((VOID *) nue2_isr);

	return E_OK;
}

/*
    NUE2 Get Open Status

    Check if NUE2 engine is opened

    @return
        - @b FALSE: engine is not opened
        - @b TRUE: engine is opened
*/
BOOL nue2_is_opened(void)
{
	if (g_nue2_lock_status == TASK_LOCKED) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    NUE2 Close Driver

    Close NUE2 engine
	This function should be called before nue_detach()

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_close(void)
{
	ER er_return;

	// Check state-machine
	er_return = nue2_chk_state_machine(NUE2_OP_CLOSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Check semaphore
	if (g_nue2_lock_status != TASK_LOCKED) {
		return E_SYS;
	}
#if 0
#if !KERNEL_USAGE
	// Diable engine interrupt
	drv_disableInt(DRV_INT_NUE2);

	// Enable Sram shut down
	pinmux_enableSramShutDown(NUE2_SD);
#endif
	// Detach
	nue2_detach();
#else

#if (NUE2_AI_FLOW == ENABLE)
#else
	er_return = (ER) nue2_uninit();
#endif

#endif
	// Update state-machine
	er_return = nue2_chk_state_machine(NUE2_OP_CLOSE, TRUE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Unlock semaphore
	er_return = nue2_unlock();
	if (er_return != E_OK) {
		return er_return;
	}

	return er_return;
}

/**
    NUE2 Pause operation

    Pause NUE2 engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_pause(VOID)
{
	ER er_return = E_OK;

	er_return = nue2_chk_state_machine(NUE2_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	nue2_enable(DISABLE);

	er_return = nue2_chk_state_machine(NUE2_OP_SET2PAUSE, TRUE);

	return er_return;
}

/**
    NUE2 Start Operation

    Start NUE2 engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER nue2_start(void)
{
	ER er_return;

	er_return = nue2_chk_state_machine(NUE2_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	g_nue2_cb_flg = 0;
	// Clear SW flag
	nue2_clr_all();

	er_return = nue2_chk_state_machine(NUE2_OP_SET2RUN, TRUE);
	nue2_enable(ENABLE);

	return er_return;
}

/**
	NUE2 Start Operation

	Start NUE2 engine for isr

	@return
		- @b E_OK: Done with no error.
		- Others: Error occured.
*/
ER nue2_isr_start(void)
{
	ER er_return;

	er_return = nue2_chk_state_machine(NUE2_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	g_nue2_cb_flg = 1;
	// Clear SW flag
	nue2_clr_all();

	er_return = nue2_chk_state_machine(NUE2_OP_SET2RUN, TRUE);
	nue2_enable(ENABLE);

	return er_return;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
    NUE2 reset

    Reset NUE2 engine.

    @return None.
*/
VOID nue2_reset(VOID)
{
	nue2_set_dma_disable(1);
	nue2_pt_get_engine_idle();
    nue2_clr(1); // sw rst
    nue2_set_dma_disable(0); // release dma disable
    nue2_clr(0);
    nue2_clr_intr_status(NUE2_INT_ALL);
	nue2_pt_iset_flg(FLGPTN_NUE2_LLEND);
}

/**
    NUE2 Clear All

    Clear NUE2 all flag.

    @return None.
*/
VOID nue2_clr_all(VOID)
{
#if 1
	/*nue2_pt_clr_flg(FLGPTN_NUE2_FRMEND);
	nue2_pt_clr_flg(FLGPTN_NUE2_DMAIN0END);
	nue2_pt_clr_flg(FLGPTN_NUE2_DMAIN1END);
	nue2_pt_clr_flg(FLGPTN_NUE2_DMAIN2END);
	nue2_pt_clr_flg(FLGPTN_NUE2_LLEND);
	nue2_pt_clr_flg(FLGPTN_NUE2_LLERR);
	nue2_pt_clr_flg(FLGPTN_NUE2_LLJOBEND);
    nue2_pt_clr_flg(FLGPTN_NUE2_SWRESET);*/
	nue2_pt_clr_flg(FLGPTN_NUE2_FRMEND | FLGPTN_NUE2_DMAIN0END | FLGPTN_NUE2_DMAIN1END |
					FLGPTN_NUE2_DMAIN2END | FLGPTN_NUE2_LLEND | FLGPTN_NUE2_LLERR | 
					FLGPTN_NUE2_LLJOBEND |FLGPTN_NUE2_SWRESET, g_nue2_cb_flg);
#endif

}


/**
    NUE2 Wait Frame End

    Wait for NUE2 frame end flag.

    @param[in] is_clr_flag
        \n-@b TRUE: clear flag and wait for next flag.
        \n-@b FALSE: wait for flag.

    @return None.
*/
void nue2_wait_frameend(BOOL is_clr_flag)
{
	FLGPTN uiFlag;

	if (is_clr_flag == TRUE) {
		nue2_pt_clr_flg(FLGPTN_NUE2_FRMEND, g_nue2_cb_flg);
	}
	nue2_pt_wai_flg(&uiFlag, FLGPTN_NUE2_FRMEND);
}

/**
    NUE2 Set Parameters to Engine

    Set NUE2 parameters to engine.

    @param[in] Mode.
            - NUE2_OPMODE_1        : Operation of Mean Subtraction
            - NUE2_OPMODE_2        : Operation of HSV
            - NUE2_OPMODE_3        : Operation of Rotate
            - NUE2_OPMODE_USERDEFINE : User Define
    @param[in] p_parm NUE2 configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER nue2_set_para2eng(NUE2_OPMODE mode, NUE2_PARM *p_parm)
{
	ER er_return = E_OK;
	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}
#if NUE2_PRINT_PARM

    DBG_IND("NUE2 lib ...\r\n");
    DBGD(p_parm->func_en.yuv2rgb_en);
    DBGD(p_parm->func_en.sub_en);
    DBGD(p_parm->func_en.pad_en);
    DBGD(p_parm->func_en.hsv_en);
    DBGD(p_parm->func_en.rotate_en);

	DBGD(p_parm->infmt);
	DBGD(p_parm->outfmt.out_signedness);

	DBGD(p_parm->insize.in_width);
	DBGD(p_parm->insize.in_height);

	DBGD(p_parm->dmaio_lofs.in0_lofs);
	DBGD(p_parm->dmaio_lofs.in1_lofs);
    DBGD(p_parm->dmaio_lofs.in2_lofs);
    DBGD(p_parm->dmaio_lofs.out0_lofs);
    DBGD(p_parm->dmaio_lofs.out1_lofs);
    DBGD(p_parm->dmaio_lofs.out2_lofs);

	DBGD(p_parm->sub_parm.sub_mode);
	DBGD(p_parm->sub_parm.sub_in_width);
	DBGD(p_parm->sub_parm.sub_in_height);
	DBGD(p_parm->sub_parm.sub_coef0);
	DBGD(p_parm->sub_parm.sub_coef1);
	DBGD(p_parm->sub_parm.sub_coef2);
	DBGD(p_parm->sub_parm.sub_dup);

	DBGD(p_parm->pad_parm.pad_crop_x);
	DBGD(p_parm->pad_parm.pad_crop_y);
	DBGD(p_parm->pad_parm.pad_crop_width);
	DBGD(p_parm->pad_parm.pad_crop_height);
    DBGD(p_parm->pad_parm.pad_crop_out_x);
    DBGD(p_parm->pad_parm.pad_crop_out_y);
    DBGD(p_parm->pad_parm.pad_crop_out_width);
    DBGD(p_parm->pad_parm.pad_crop_out_height);
    DBGD(p_parm->pad_parm.pad_val0);
    DBGD(p_parm->pad_parm.pad_val1);
    DBGD(p_parm->pad_parm.pad_val2);

	DBGD(p_parm->hsv_parm.hsv_out_mode);
	DBGD(p_parm->hsv_parm.hsv_hue_shift);

	DBGD(p_parm->rotate_parm.rotate_mode);

	DBGD(p_parm->scale_parm.h_filtmode);
    DBGD(p_parm->scale_parm.v_filtmode);
    DBGD(p_parm->scale_parm.h_filtcoef);
    DBGD(p_parm->scale_parm.v_filtcoef);
    DBGD(p_parm->scale_parm.h_scl_size);
    DBGD(p_parm->scale_parm.v_scl_size);
    DBGD(p_parm->scale_parm.h_dnrate);
    DBGD(p_parm->scale_parm.v_dnrate);
    DBGD(p_parm->scale_parm.h_sfact);
    DBGD(p_parm->scale_parm.v_sfact);
    DBGD(p_parm->scale_parm.ini_h_dnrate);
    DBGD(p_parm->scale_parm.ini_h_sfact);
    DBGD(p_parm->scale_parm.final_h_dnrate);
    DBGD(p_parm->scale_parm.final_h_sfact);
	DBGD(p_parm->scale_parm.fact_update);

	if(nvt_get_chip_id() == CHIP_NA51055) {
		DBGD(p_parm->sub_parm.sub_shift);
	} else {
		DBGD(p_parm->mean_shift_parm.mean_shift_dir);
		DBGD(p_parm->mean_shift_parm.mean_shift);
		DBGD(p_parm->mean_shift_parm.mean_scale);

		DBGD(p_parm->scale_parm.scale_h_mode);
		DBGD(p_parm->scale_parm.scale_v_mode);
		DBGD(p_parm->flip_parm.flip_mode);
	}

#endif
	if (p_parm->insize.in_width >= p_parm->scale_parm.h_scl_size) {
		p_parm->scale_parm.scale_h_mode = 0;
	} else {
		p_parm->scale_parm.scale_h_mode = 1;
	}
	if (p_parm->insize.in_height >= p_parm->scale_parm.v_scl_size) {
		p_parm->scale_parm.scale_v_mode = 0;
	} else {
		p_parm->scale_parm.scale_v_mode = 1;
	}
	
	nue2_set_drvmode(mode);

	switch (mode) {
        case NUE2_OPMODE_1:
            nue2_set_infmt(p_parm->infmt);
    		nue2_set_outfmt(p_parm->outfmt);
    		er_return = nue2_set_func_en(p_parm->func_en);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_insize(p_parm->insize);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_scale_parm(p_parm->scale_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_sub_parm(p_parm->sub_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_pad_parm(p_parm->pad_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_dmaio_lofs(p_parm->dmaio_lofs);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_dmaio_addr(p_parm->dmaio_addr);
			if (er_return != E_OK) return E_PAR;
			if(nvt_get_chip_id() != CHIP_NA51055) {
				er_return = nue2_set_mean_shift_parm(p_parm->mean_shift_parm);
				if (er_return != E_OK) return E_PAR;
				er_return = nue2_set_flip_parm(p_parm->flip_parm);
				if (er_return != E_OK) return E_PAR;
			}
            break;
        case NUE2_OPMODE_2:
            nue2_set_infmt(p_parm->infmt);
    		nue2_set_outfmt(p_parm->outfmt);
    		er_return = nue2_set_func_en(p_parm->func_en);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_insize(p_parm->insize);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_scale_parm(p_parm->scale_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_hsv_parm(p_parm->hsv_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_dmaio_lofs(p_parm->dmaio_lofs);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_dmaio_addr(p_parm->dmaio_addr);
			if (er_return != E_OK) return E_PAR;
            break;
        case NUE2_OPMODE_3:
            nue2_set_infmt(p_parm->infmt);
    		nue2_set_outfmt(p_parm->outfmt);
    		er_return = nue2_set_func_en(p_parm->func_en);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_insize(p_parm->insize);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_rotate_parm(p_parm->rotate_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_dmaio_lofs(p_parm->dmaio_lofs);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_dmaio_addr(p_parm->dmaio_addr);
			if (er_return != E_OK) return E_PAR;
            break;
	    case NUE2_OPMODE_USERDEFINE:
		case NUE2_OPMODE_USERDEFINE_NO_INT:
    		nue2_set_infmt(p_parm->infmt);
    		nue2_set_outfmt(p_parm->outfmt);
    		er_return = nue2_set_func_en(p_parm->func_en);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_insize(p_parm->insize);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_scale_parm(p_parm->scale_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_sub_parm(p_parm->sub_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_pad_parm(p_parm->pad_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_hsv_parm(p_parm->hsv_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_rotate_parm(p_parm->rotate_parm);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_dmaio_lofs(p_parm->dmaio_lofs);
			if (er_return != E_OK) return E_PAR;
            er_return = nue2_set_dmaio_addr(p_parm->dmaio_addr);
			if (er_return != E_OK) return E_PAR;
			if(nvt_get_chip_id() != CHIP_NA51055) {
				er_return = nue2_set_mean_shift_parm(p_parm->mean_shift_parm);
				if (er_return != E_OK) return E_PAR;
				er_return = nue2_set_flip_parm(p_parm->flip_parm);
				if (er_return != E_OK) return E_PAR;
			}
            break;
        default:
    		DBG_ERR("Unknown operation mode %d!\r\n", mode);
    		return E_NOSPT;
	}

	return er_return;
}

/**
    NUE2 Set Mode

    Set NUE2 engine mode

    @param[in] Mode.
            - NUE2_OPMODE_1             : Mean Subtraction
            - NUE2_OPMODE_2             : HSV
            - NUE2_OPMODE_3             : Rotate
            - NUE2_OPMODE_USERDEFINE    : User Define
    @param[in] p_parm NUE2 configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_setmode(NUE2_OPMODE mode, NUE2_PARM *p_parm)
{
	ER er_return = nue2_chk_state_machine(NUE2_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	if (p_parm == NULL) {
		DBG_ERR("null input...\r\n");
		return E_NOEXS;
	}

	// Clear engine interrupt status
	if(nvt_get_chip_id() == CHIP_NA51055) {
		nue2_enable_int(DISABLE, NUE2_INTE_ALL);
	} else {
		nue2_enable_int(DISABLE, NUE2_INTE_ALL_528);
	}

	if(nvt_get_chip_id() == CHIP_NA51055) {
		nue2_clr_intr_status(NUE2_INT_ALL);
	} else {
		nue2_clr_intr_status(NUE2_INT_ALL_528);
	}

	er_return = nue2_set_para2eng(mode, p_parm);
	if (er_return != E_OK) {
		return er_return;
	}

	if(mode != NUE2_OPMODE_USERDEFINE_NO_INT) {
		if(nvt_get_chip_id() == CHIP_NA51055) {
			nue2_enable_int(ENABLE, NUE2_INTE_ALL);
		} else {
			nue2_enable_int(ENABLE, NUE2_INTE_ALL_528);
		}
	}

	er_return = nue2_pause();
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = nue2_chk_state_machine(NUE2_OP_SET2PAUSE, TRUE);

	return er_return;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
	NUE2 Set Base Address

	Set NUE2 engine registers address.

	@param[in] addr registers address.

	@return none.
*/
VOID nue2_set_base_addr(UINT32 addr)
{
	nue2_reg_base_addr = addr;
}

/**
	NUE2 Get Base Address

	Get NUE2 engine registers address.

	@return registers address.
*/
UINT32 nue2_get_base_addr(VOID)
{
	return nue2_reg_base_addr;
}

/**
	NUE2 Create Resource
*/
VOID nue2_create_resource(VOID *parm, UINT32 clk_freq)
{
	nue2_pt_create_resource(parm, clk_freq);
	SEM_CREATE(SEMID_NUE2, 1);
	SEM_CREATE(SEMID_NUE2_LIB, 1);
}

/**
	NUE2 Create Resource
*/
VOID nue2_release_resource(VOID)
{
	nue2_pt_rel_flg();
	SEM_DESTROY(SEMID_NUE2);
	SEM_DESTROY(SEMID_NUE2_LIB);
}

UINT32 nue2_init(VOID)
{
	UINT32 clk_freq_mhz;

	nue2_wait_dma_abort();

	nue2_lib_lock();
	if (g_nue2_enable_cnt == 0) {
		clk_freq_mhz = nue2_get_clock_rate();
		if (clk_freq_mhz == 0) {
			nue2_set_clock_rate(480);
		} else {
			nue2_set_clock_rate(clk_freq_mhz);
		}
		nue2_attach();
		nvt_disable_sram_shutdown(NUE2_SD);
	}
	g_nue2_enable_cnt++;
	nue2_set_cycle_en(NUE2_CYCLE_APP);
	nue2_lib_unlock();

	return 0;
}

UINT32 nue2_uninit(VOID)
{
	nue2_lib_lock();
	nue2_set_cycle_en(NUE2_CYCLE_OFF);
	g_nue2_enable_cnt--;
	if (g_nue2_enable_cnt == 0){
		nue2_detach();
		nvt_enable_sram_shutdown(NUE2_SD);
	}
	nue2_lib_unlock();

	return 0;
}
/**
    NUE2 Linked List Pause operation

    Pause linked list NUE2 engine.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_ll_pause(VOID)
{
	ER er_return = E_OK;

	//PROF_END("kdrv_nue2_ll");

	er_return = nue2_chk_state_machine(NUE2_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	nue2_ll_enable(DISABLE);
	nue2_ll_terminate(DISABLE);

	er_return = nue2_chk_state_machine(NUE2_OP_SET2PAUSE, TRUE);

	return er_return;
}

/**
    NUE2 Linked List Start operation

    Start linked list NUE2 engine.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_ll_start(VOID)
{
	ER er_return;
	er_return = nue2_chk_state_machine(NUE2_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	g_nue2_cb_flg = 0;
	// Clear SW flag
	nue2_clr_all();

	//PROF_START();

	
	er_return = nue2_chk_state_machine(NUE2_OP_SET2RUN, TRUE);
#if defined(__LINUX)
	if (nue2_perf_en == 1) {
		nue2_perf_status = 1;
		do_gettimeofday(&nue2_tstart); 
	}
#endif
	nue2_ll_enable(ENABLE);
	//nue_wait_ll_frameend(FALSE);
	//nue_pause();

	return er_return;
}

/**
	NUE2 Linked List Start operation

	Start linked list NUE2 engine for isr.

	@return
		- @b E_OK: Done with no error.
		- Others: Error occured.
*/
ER nue2_ll_isr_start(VOID)
{
	ER er_return;
	er_return = nue2_chk_state_machine(NUE2_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	g_nue2_cb_flg = 1;
	// Clear SW flag
	nue2_clr_all();

	//PROF_START();


	er_return = nue2_chk_state_machine(NUE2_OP_SET2RUN, TRUE);
	nue2_ll_enable(ENABLE);
	//nue_wait_ll_frameend(FALSE);
	//nue_pause();

	return er_return;
}
/**
    NUE2 Linked List Set Mode

    Set linked list NUE2 engine mode.

    @param[in] ll_addr NUE2 linked list address.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_ll_setmode(NUE2_LL_PRM *ll_parm)
{
	ER er_return = nue2_chk_state_machine(NUE2_OP_SET2PAUSE, FALSE);

	if (er_return != E_OK) {
		return er_return;
	}

	if(nvt_get_chip_id() == CHIP_NA51055) {
		// Clear engine interrupt status
		nue2_enable_int(DISABLE, NUE2_INTE_ALL);
		nue2_clr_intr_status(NUE2_INT_ALL);
	} else {
		// Clear engine interrupt status
		nue2_enable_int(DISABLE, NUE2_INTE_ALL_528);
		nue2_clr_intr_status(NUE2_INT_ALL_528);
	}

	nue2_set_dmain_lladdr(ll_parm->ll_addr);
	nue2_set_ll_base_addr(ll_parm->ll_base_addr);

	
	if(nvt_get_chip_id() == CHIP_NA51055) {
		nue2_enable_int(ENABLE, NUE2_INTE_ALL_RUNLL);
	} else {
		nue2_enable_int(ENABLE, NUE2_INTE_ALL_RUNLL_528);
	}

	er_return = nue2_ll_pause();
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = nue2_chk_state_machine(NUE2_OP_SET2PAUSE, TRUE);
	return er_return;
}

/**
    NUE2 Wait Linked list Frame End

    Wait for NUE2 linked list frame end flag.

    @param[in] is_clr_flag
        \n-@b TRUE: clear flag and wait for next flag.
        \n-@b FALSE: wait for flag.

    @return None.
*/
VOID nue2_wait_ll_frameend(BOOL is_clr_flag)
{
#if 1
	FLGPTN uiFlag;
	if (is_clr_flag == TRUE) {
		nue2_pt_clr_flg(FLGPTN_NUE2_LLEND, g_nue2_cb_flg);
	}
	nue2_pt_wai_flg(&uiFlag, FLGPTN_NUE2_LLEND);
#endif
}

VOID nue2_reset_status(VOID)
{
	ER er_return;

	if (g_nue2_lock_status != NO_TASK_LOCKED) {
		er_return = nue2_close();
		if (er_return != E_OK) {
			DBG_ERR("nue2_reset_status: Error at nue2_close().\r\n");
			goto err_exit;
		}
	}
	nue2_status = NUE2_ENGINE_IDLE;

err_exit:

	return;
}

/**
    NUE2 dma abort

    DMA abort NUE2 engine.

    @return None.
*/
VOID nue2_dma_abort(VOID)
{
	BOOL is_dma_disable;

	is_dma_disable = nue2_get_dma_disable();
	if (is_dma_disable == 1) {
		DBG_IND("NUE2_DMA_ABORT: it has been in dma_disable state.\r\n");
		return;
	}
	nue2_set_dma_disable(1);

	return;
}

static VOID nue2_wait_dma_abort(VOID)
{
    BOOL is_dma_disable;

    is_dma_disable = nue2_get_dma_disable();
    while (is_dma_disable == 1) {
		vos_task_delay_ms(g_nue2_dma_abort_delay);
		is_dma_disable = nue2_get_dma_disable();
    }

    return;
}

EXPORT_SYMBOL(nue2_init);
EXPORT_SYMBOL(nue2_uninit);
EXPORT_SYMBOL(nue2_reset_status);
EXPORT_SYMBOL(nue2_dma_abort);
EXPORT_SYMBOL(nue2_isr);


