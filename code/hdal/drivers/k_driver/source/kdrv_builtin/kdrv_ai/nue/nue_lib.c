/*
    NUE module driver

    NT98313 NUE driver.

    @file       nue_lib.c
    @ingroup    mIIPPNUE

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
//#include    <stdio.h>
//#include    <stdlib.h>
//#include    <string.h>

#include "nue_platform.h"
#include "nue_lib.h"
#include "nue_reg.h"
#include "nue_int.h"
#include "nue_ll_cmd.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "comm/ddr_arb.h"
#include "kwrap/task.h"
//#include    "../Common/LinkListCmd.h"
//#include    "dma.h"
//#include    "ist.h"
/**
    @addtogroup mIIPPNUE
*/
//@{

#define NUE_PRINT_PARM                  DISABLE
#define NUE_PRINT_REG                   DISABLE

#if 1
#define FLGPTN_NUE_ROUTEND              FLGPTN_BIT(0)
#define FLGPTN_NUE_DMAIN0END            FLGPTN_BIT(1)
#define FLGPTN_NUE_DMAIN1END            FLGPTN_BIT(2)
#define FLGPTN_NUE_DMAIN2END            FLGPTN_BIT(3)
#define FLGPTN_NUE_POOLING_ILLEGALSIZE  FLGPTN_BIT(4)

#define FLGPTN_NUE_LLEND                FLGPTN_BIT(8)
#define FLGPTN_NUE_LLERROR              FLGPTN_BIT(9)

#define FLGPTN_NUE_FCD_DECODE_DONE      FLGPTN_BIT(16)
#define FLGPTN_NUE_FCD_VLC_DEC_ERR     FLGPTN_BIT(17)
#define FLGPTN_NUE_FCD_BS_SIZE_ERR      FLGPTN_BIT(18)
#define FLGPTN_NUE_FCD_SPARSE_DATA_ERR     FLGPTN_BIT(19)
#define FLGPTN_NUE_FCD_SPARSE_INDEX_ERR    FLGPTN_BIT(20)

#define FLGPTN_NUE_AXI0ERROR            FLGPTN_BIT(24)
#define FLGPTN_NUE_AXI1ERROR            FLGPTN_BIT(25)
#define FLGPTN_NUE_AXI2ERROR            FLGPTN_BIT(26)

#endif

static SEM_HANDLE semid_nue;
static SEM_HANDLE semid_nue_lib;

UINT32 nue_reg_base_addr    = 0;
UINT32 nue_ll_base_addr     = 0;

static UINT32 g_nue_status    = NUE_ENGINE_IDLE;
static ID g_nue_lock_status   = NO_TASK_LOCKED;
UINT32 g_nue_enable_cnt = 0;
BOOL g_nue_cb_flg = 0;
static UINT32 g_nue_dma_abort_delay=1000;

static VOID (*p_nue_int_cb)(UINT32 int_status);


//ISR
extern void nue_isr(void);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
static ER nue_set_para2eng(NUE_OPMODE mode, NUE_PARM *p_parm);
#endif
static ER nue_chk_state_machine(UINT32 operation, BOOL update);
static ER nue_lock(VOID);
static ER nue_unlock(VOID);
static VOID nue_attach(VOID);
static VOID nue_detach(VOID);

static ER   nue_set_clock_rate(UINT32 clock);

static VOID nue_clr_all(VOID);
static VOID nue_wait_dma_abort(VOID);

#if NUE_CYCLE_TEST
UINT64 nue_total_cycle = 0;
#if NUE_CYCLE_LAYER
UINT32 nue_cnt = 0;
UINT32 nue_dbg_cnt[20] = {0};
#endif
UINT64 nue_dump_total_cycle(UINT32 debug_layer)
{
	UINT64 total_cost = nue_total_cycle;
    #if NUE_CYCLE_LAYER
    if(debug_layer==1) {
        UINT32 i = 0;
    	for (i = 0; i < nue_cnt; i++) {
    		printk("item %d nue cycle = %d\n", i, (unsigned int)nue_dbg_cnt[i]);
    	}
    }
    //#else
        //printk("NUE_CYCLE_LAYER is disable now.\n");
    #endif
	//printk("nue total cycle = 0x%08X%08X\n", (unsigned int)(nue_total_cycle >> 32), (unsigned int)(nue_total_cycle & 0xFFFFFFFF));
	nue_total_cycle = 0;

	return total_cost;
}
#endif

/*
    NUE Interrupt Service Routine

    NUE interrupt service routine

    @return None.
*/
VOID nue_isr(VOID)
{
	UINT32 nue_int_status = nue_get_intr_status();
	nue_int_status = nue_int_status & nue_get_int_enable();

	if (nue_int_status == 0) {
		return;
	}
	nue_clr_intr_status(nue_int_status);

	if (nue_int_status & NUE_INTE_ROUTEND) {
		nue_pt_iset_flg(FLGPTN_NUE_ROUTEND);
		#if NUE_CYCLE_TEST
        #if NUE_CYCLE_LAYER
        nue_dbg_cnt[nue_cnt++] = (unsigned int)nue_getcycle();
		#endif
		nue_total_cycle += nue_getcycle();
		#endif
	}
	if (nue_int_status & NUE_INTE_DMAIN0END) {
		nue_pt_iset_flg(FLGPTN_NUE_DMAIN0END);
	}
	if (nue_int_status & NUE_INTE_DMAIN1END) {
		nue_pt_iset_flg(FLGPTN_NUE_DMAIN1END);
	}
	if (nue_int_status & NUE_INTE_DMAIN2END) {
		nue_pt_iset_flg(FLGPTN_NUE_DMAIN2END);
	}
	if (nue_int_status & NUE_INTE_LLEND) {
		nue_pt_iset_flg(FLGPTN_NUE_LLEND);
	}
	if (nue_int_status & NUE_INTE_LLERR) {
		nvt_dbg(ERR, "Illegal linked list setting\r\n");
		nue_pt_iset_flg(FLGPTN_NUE_LLERROR);
	}
	if (nue_int_status & NUE_INTE_FCD_DECODE_DONE) {
		nue_pt_iset_flg(FLGPTN_NUE_FCD_DECODE_DONE);
	}
	if (nue_int_status & NUE_INTE_FCD_VLC_DEC_ERR) {
		nvt_dbg(ERR, "Bitstream lens overflow\r\n");
		nue_pt_iset_flg(FLGPTN_NUE_FCD_VLC_DEC_ERR);
	}
	if (nue_int_status & NUE_INTE_FCD_BS_SIZE_ERR) {
		nvt_dbg(ERR, "Bitstream size not align\r\n");
		nue_pt_iset_flg(FLGPTN_NUE_FCD_BS_SIZE_ERR);
	}
	if (nue_int_status & NUE_INTE_FCD_SPARSE_DATA_ERR) {
		nvt_dbg(ERR, "SPARSE odd number of encoded data\r\n");
		nue_pt_iset_flg(FLGPTN_NUE_FCD_SPARSE_DATA_ERR);
	}
	if (nue_int_status & NUE_INTE_FCD_SPARSE_INDEX_ERR) {
		nvt_dbg(ERR, "SPARSE index < 1\r\n");
		nue_pt_iset_flg(FLGPTN_NUE_FCD_SPARSE_INDEX_ERR);
	}


#if 0
	if ((p_nue_int_cb) && ((nue_int_status & NUE_INTE_ROUTEND) || (nue_int_status & NUE_INTE_LLEND))) {
		nue_pause();
		p_nue_int_cb(nue_int_status);
	}
#else
	if (p_nue_int_cb) {
		p_nue_int_cb(nue_int_status);
	}
#endif
}

/*
    NUE while loop routine

    NUE while loop routine

    @return None.
*/
VOID nue_loop_frameend(VOID)
{
    nue_pt_loop_frameend();
}

/**
    NUE get operation clock

    Get NUE clock selection.

    @param None.

    @return UINT32 NUE operation clock rate.
*/
UINT32 nue_get_clock_rate(VOID)
{
	UINT32 clk_freq_mhz = 0;

	clk_freq_mhz = nue_pt_get_clock_rate();

	return clk_freq_mhz;
}

void nue_enable_clk(void)
{
	nue_pt_clk_enable();
}

void nue_disable_clk(void)
{
	nue_pt_clk_disable();
}

/*
    NUE State Machine

    NUE state machine

    @param[in] operation the operation to execute
    @param[in] update if we will update state machine status

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_chk_state_machine(UINT32 operation, BOOL update)
{
	//static UINT32 state = NUE_ENGINE_IDLE;
	ER er_return;

	switch (g_nue_status) {
	case NUE_ENGINE_IDLE:
		switch (operation) {
		case NUE_OP_ATTACH:
			if (update) {
				g_nue_status = NUE_ENGINE_IDLE;
			}
			er_return = E_OK;
			break;
		case NUE_OP_DETACH:
			if (update) {
				g_nue_status = NUE_ENGINE_IDLE;
			}
			er_return = E_OK;
			break;
		case NUE_OP_OPEN:
			if (update) {
				g_nue_status = NUE_ENGINE_READY;
			}
			er_return = E_OK;
			break;
		default:
			er_return = E_OBJ;
			break;
		}
		break;

	case NUE_ENGINE_READY:
		switch (operation) {
		case NUE_OP_CLOSE:
			if (update) {
				g_nue_status = NUE_ENGINE_IDLE;
			}
			er_return = E_OK;
			break;
		case NUE_OP_SET2READY:
		case NUE_OP_READLUT:
		case NUE_OP_CHGINT:
			if (update) {
				g_nue_status = NUE_ENGINE_READY;
			}
			er_return = E_OK;
			break;
		case NUE_OP_SET2PAUSE:
			if (update) {
				g_nue_status = NUE_ENGINE_PAUSE;
			}
			er_return = E_OK;
			break;
		default:
			er_return = E_OBJ;
			break;
		}
		break;

	case NUE_ENGINE_PAUSE:
		switch (operation) {
		case NUE_OP_SET2PAUSE:
		case NUE_OP_READLUT:
		case NUE_OP_CHGINT:
			if (update) {
				g_nue_status = NUE_ENGINE_PAUSE;
			}
			er_return = E_OK;
			break;
		case NUE_OP_SET2RUN:
			if (update) {
				g_nue_status = NUE_ENGINE_RUN;
			}
			er_return = E_OK;
			break;
		case NUE_OP_SET2READY:
			if (update) {
				g_nue_status = NUE_ENGINE_READY;
			}
			er_return = E_OK;
			break;
		case NUE_OP_CLOSE:
			if (update) {
				g_nue_status = NUE_ENGINE_IDLE;
			}
			er_return = E_OK;
			break;
		default :
			er_return = E_OBJ;
			break;
		}
		break;

	case NUE_ENGINE_RUN:
		switch (operation) {
		case NUE_OP_SET2RUN:
		case NUE_OP_CHGINT:
			if (update) {
				g_nue_status = NUE_ENGINE_RUN;
			}
			er_return = E_OK;
			break;
		case NUE_OP_SET2PAUSE:
			if (update) {
				g_nue_status = NUE_ENGINE_PAUSE;
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
		nvt_dbg(ERR, "State machine error! st %d op %d\r\n", (int)g_nue_status, (int)operation);
	}

	return er_return;
}

/**
    NUE change interrupt

    NUE interrupt configuration.

    for FPGA verification only

    @param[in] int_en Interrupt enable setting.

    @return ER NUE change interrupt status.
*/
ER nue_change_interrupt(UINT32 int_en)
{
	ER er_return = E_OK;
	er_return = nue_chk_state_machine(NUE_OP_CHGINT, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	nue_enable_int(ENABLE, int_en);
	er_return = nue_chk_state_machine(NUE_OP_CHGINT, TRUE);
	return er_return;
}

/**
    NUE Lock Function

    Lock NUE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_lock(VOID)
{
#if 0
	ER er_return;

	if (g_nue_lock_status == NO_TASK_LOCKED) {
		er_return = wai_sem(SEMID_NUE);
		if (er_return != E_OK) {
			return er_return;
		}
		g_nue_lock_status = TASK_LOCKED;
		return E_OK;
	} else {
		return E_OBJ;
	}
#else
	ER er_return=E_OK;

#if (KDRV_AI_MINI_NO_LOCK == 1)
#else
	er_return = SEM_WAIT(semid_nue);
	if (er_return != E_OK) {
		return er_return;
	}
#endif //#if (KDRV_AI_MINI_NO_LOCK == 1)
	g_nue_lock_status = TASK_LOCKED;
	return er_return;
#endif
}

/**
    NUE Unlock Function

    Unlock NUE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER nue_unlock(VOID)
{
#if 0
	if (g_nue_lock_status == TASK_LOCKED) {
		g_nue_lock_status = NO_TASK_LOCKED;
		return sig_sem(SEMID_NUE);
	} else {
		return E_OBJ;
	}
#else
	g_nue_lock_status = NO_TASK_LOCKED;
#if (KDRV_AI_MINI_NO_LOCK == 1)
#else
	SEM_SIGNAL(semid_nue);
#endif //#if (KDRV_AI_MINI_NO_LOCK == 1)
	return E_OK;
#endif
}

/**
    NUE Lock Function

    Lock NUE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_lib_lock(VOID)
{
	ER er_return = SEM_WAIT(semid_nue_lib);

	return er_return;
}

/**
    NUE Unlock Function

    Unlock NUE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER nue_lib_unlock(VOID)
{
	SEM_SIGNAL(semid_nue_lib);
	return E_OK;
}

/**
    NUE Attach Function

    Pre-initialize driver required HW before driver open
    \n This function should be called before calling any other functions

    @return None
*/
VOID nue_attach(VOID)
{
	//pll_enableClock(NUE_CLK);
	nue_pt_clk_enable();
}

/**
    NUE Detach Function

    De-initialize driver required HW after driver close
    \n This function should be called before calling any other functions

    @return None
*/
VOID nue_detach(VOID)
{
	//pll_disableClock(NUE_CLK);
	nue_pt_clk_disable();
}

/**
    NUE Set Clock Rate

    Set current clock rate

    @param[in] clock cnn clock rate

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER nue_set_clock_rate(UINT32 clock)
{
	ER er_return;

	er_return = nue_pt_set_clock_rate(clock);

	return er_return;
}

/**
    NUE Open Driver

    Open NUE engine
    \n This function should be called after nue_attach()

    @param[in] p_obj_cb NUE open object

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_open(NUE_OPENOBJ *p_obj_cb)
{
	ER er_return;

	// Lock semaphore
	er_return = nue_lock();
	if (er_return != E_OK) {
		return er_return;
	}

	// Check state-machine
	er_return = nue_chk_state_machine(NUE_OP_OPEN, FALSE);
	if (er_return != E_OK) {
		// release resource when stateMachine fail
		nue_unlock();
		return er_return;
	}
#if 0
#if 0
	// Power on module
	pmc_turnonPower(PMC_MODULE_NUE);
	if (p_obj_cb != NULL)
		nue_set_clock_rate(p_obj_cb->clk_sel);
	// Select PLL clock source & enable PLL clock source
	pll_setClockRate(PLL_CLKSEL_NUE, g_uinueTrueClock);
#endif
	if (p_obj_cb != NULL) {
		er_return = nue_set_clock_rate(p_obj_cb->clk_sel);
	} else {
		er_return = nue_set_clock_rate(600);
	}
	if (er_return != E_OK) {
		er_return = nue_unlock();
		return er_return;
	}

	// Attach
	nue_attach();

#if 0
	// Disable Sram shut down
	pinmux_disableSramShutDown(NUE_SD);
	// Enable engine interrupt
	drv_enableInt(DRV_INT_NUE);
#else
	nvt_disable_sram_shutdown(NUE_SD);
#endif

	// Clear engine interrupt status
	nue_clr_intr_status(NUE_INT_ALL);

	// SW reset
	nue_clr(0);
	nue_clr(1);
	nue_clr(0);
#else
	
#if (NUE_AI_FLOW == ENABLE)
#else
	er_return = (ER) nue_init();
#endif

#endif
	// Clear SW flag
	nue_clr_all();

	// Hook call-back function
	if ((p_obj_cb != NULL) && (p_obj_cb->fp_isr_cb != NULL)) {
		p_nue_int_cb = p_obj_cb->fp_isr_cb;
	} else {
		p_nue_int_cb = NULL;
	}



	//enable AXI bus
	//nue_set_axidis(FALSE);

	// update state-machine
	er_return = nue_chk_state_machine(NUE_OP_OPEN, TRUE);

	nue_pt_request_irq((VOID *) nue_isr);	

	return er_return;
}

/*
    NUE Get Open Status

    Check if NUE engine is opened

    @return
        - @b FALSE: engine is not opened
        - @b TRUE: engine is opened
*/
BOOL nue_is_opened(VOID)
{
	if (g_nue_lock_status == TASK_LOCKED) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    NUE Close Driver

    Close NUE engine
    \n This function should be called before nue_detach()

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_close(VOID)
{
	ER er_return;

	// Check state-machine
	er_return = nue_chk_state_machine(NUE_OP_CLOSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Check semaphore
	if (!nue_is_opened()) {
		return E_SYS;
	}

#if 0
#if 0
	// Diable engine interrupt
	drv_disableInt(DRV_INT_NUE);

	// Enable Sram shut down
	//pll_disableSramShutDown(NUE_RSTN);//mismatch
	pinmux_enableSramShutDown(NUE_SD);
#endif

	// Detach
	nue_detach();
#else

#if (NUE_AI_FLOW == ENABLE)
#else
	er_return = (ER) nue_uninit();
#endif

#endif

	// Unhook call-back function
	p_nue_int_cb = NULL;

	// Disable AXI bus
	//nue_set_axidis(TRUE);

	// update state-machine
	er_return = nue_chk_state_machine(NUE_OP_CLOSE, TRUE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Unlock semaphore
	er_return = nue_unlock();

	return er_return;
}

/**
    NUE Pause operation

    Pause NUE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_pause(VOID)
{
	ER er_return = E_OK;

	PROF_END("kdrv_nue");
	//printk("nue cycle = %d\n", nue_getcycle());
	er_return = nue_chk_state_machine(NUE_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	nue_enable(DISABLE);

	er_return = nue_chk_state_machine(NUE_OP_SET2PAUSE, TRUE);

	return er_return;
}

/**
    NUE Linked List Pause operation

    Pause linked list NUE engine.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_ll_pause(VOID)
{
	ER er_return = E_OK;

	PROF_END("kdrv_nue_ll");

	er_return = nue_chk_state_machine(NUE_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	nue_ll_enable(DISABLE);
	nue_ll_terminate(DISABLE);

	er_return = nue_chk_state_machine(NUE_OP_SET2PAUSE, TRUE);

	return er_return;
}

/**
    NUE Start operation

    Start NUE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER nue_start(VOID)
{
	ER er_return;
	er_return = nue_chk_state_machine(NUE_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

#if NUE_PRINT_REG
	{
		UINT32 ofs, val;
		for (ofs = 0; ofs < sizeof(NT98313_NUE_REG_STRUCT); ofs += 4) {
			val = NUE_GETDATA(ofs, nue_reg_base_addr);
			DBGH(val);
		}
	}
#endif
	g_nue_cb_flg = 0;
	// Clear SW flag
	nue_clr_all();

	PROF_START();

	er_return = nue_chk_state_machine(NUE_OP_SET2RUN, TRUE);
	nue_enable(ENABLE);


	//nue_wait_frameend(FALSE);
	//nue_pause();

	return er_return;
}

/**
	NUE Start operation

	Start NUE engine for isr

	@return
		- @b E_OK: Done with no error.
		- Others: Error occured.
*/
ER nue_isr_start(VOID)
{
	ER er_return;
	er_return = nue_chk_state_machine(NUE_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

#if NUE_PRINT_REG
	{
		UINT32 ofs, val;
		for (ofs = 0; ofs < sizeof(NT98313_NUE_REG_STRUCT); ofs += 4) {
			val = NUE_GETDATA(ofs, nue_reg_base_addr);
			DBGH(val);
		}
	}
#endif

	g_nue_cb_flg = 1;
	// Clear SW flag
	nue_clr_all();

	PROF_START();

	er_return = nue_chk_state_machine(NUE_OP_SET2RUN, TRUE);
	nue_enable(ENABLE);


	//nue_wait_frameend(FALSE);
	//nue_pause();

	return er_return;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
    NUE Linked List Start operation

    Start linked list NUE engine.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_ll_start(VOID)
{
	ER er_return;
	er_return = nue_chk_state_machine(NUE_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	g_nue_cb_flg = 0;
	// Clear SW flag
	nue_clr_all();

	PROF_START();


	er_return = nue_chk_state_machine(NUE_OP_SET2RUN, TRUE);
	nue_ll_enable(ENABLE);
	//nue_wait_ll_frameend(FALSE);
	//nue_pause();

	return er_return;
}

/**
	NUE Linked List Start operation

	Start linked list NUE engine for isr.

	@return
		- @b E_OK: Done with no error.
		- Others: Error occured.
*/
ER nue_ll_isr_start(VOID)
{
	ER er_return;
	er_return = nue_chk_state_machine(NUE_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	g_nue_cb_flg = 1;
	// Clear SW flag
	nue_clr_all();

	PROF_START();


	er_return = nue_chk_state_machine(NUE_OP_SET2RUN, TRUE);
	nue_ll_enable(ENABLE);
	//nue_wait_ll_frameend(FALSE);
	//nue_pause();

	return er_return;
}

/**
    NUE reset

    Reset NUE engine.

    @return None.
*/
VOID nue_reset(VOID)
{
	nue_pt_get_engine_idle();
	nue_clr(1); // sw rst
	nue_set_dma_disable(0); // release dma disable
	nue_clr(0);
	nue_clr_intr_status(NUE_INT_ALL);
	//HW reset
	/*VOID __iomem *ioaddr = NULL;
	UINT32 ioread;
	UINT32 reg_bit = 0x2000;
	const INT32 time_limit = 3000000;
	struct timeval s_time, e_time;
	INT32 dur_time = 0;	// (us)

	do_gettimeofday(&s_time);

	ioaddr = (VOID*)ioremap_nocache(0xfe000050, 0x200);
	ioread = ioread32(ioaddr + 0x0);

	nue_set_axidis(TRUE);

	while ((!nue_get_axiidle()) && (dur_time < time_limit)) {
		do_gettimeofday(&e_time);
		dur_time = (e_time.tv_sec - s_time.tv_sec) * 1000000 + (e_time.tv_usec - s_time.tv_usec);
	}

	if (dur_time > time_limit) {
		nvt_dbg(WRN, "reset time out\r\n");
	}

	if (nue_get_axiidle()) {
		ioread &= (~reg_bit);
		iowrite32(ioread, ioaddr + 0x0);

		ioread |= (reg_bit);
		iowrite32(ioread, ioaddr + 0x0);
	}

	nue_set_axidis(FALSE);

	iounmap(ioaddr);*/
}

/**
    NUE Clear All

    Clear NUE all flag.

    @return None.
*/
VOID nue_clr_all(VOID)
{
#if 1
	nue_pt_clr_flg(FLGPTN_NUE_ROUTEND | FLGPTN_NUE_DMAIN0END | FLGPTN_NUE_DMAIN1END | 
				 FLGPTN_NUE_DMAIN2END |FLGPTN_NUE_LLEND | FLGPTN_NUE_LLERROR |
				 FLGPTN_NUE_FCD_DECODE_DONE | FLGPTN_NUE_FCD_VLC_DEC_ERR |
				 FLGPTN_NUE_FCD_BS_SIZE_ERR | FLGPTN_NUE_FCD_SPARSE_DATA_ERR |
				 FLGPTN_NUE_FCD_SPARSE_INDEX_ERR, g_nue_cb_flg);
#endif

}


/**
    NUE Wait Result Out End

    Wait for NUE result out end flag.

    @param[in] is_clr_flag
        \n-@b TRUE: clear flag and wait for next flag.
        \n-@b FALSE: wait for flag.

    @return None.
*/
VOID nue_wait_frameend(BOOL is_clr_flag)
{
#if 1
	FLGPTN uiFlag;
	if (is_clr_flag == TRUE) {
		nue_pt_clr_flg(FLGPTN_NUE_ROUTEND, g_nue_cb_flg);
	}
	nue_pt_wai_flg(&uiFlag, FLGPTN_NUE_ROUTEND);
#endif
}

/**
    NUE Wait Linked list Frame End

    Wait for NUE linked list frame end flag.

    @param[in] is_clr_flag
        \n-@b TRUE: clear flag and wait for next flag.
        \n-@b FALSE: wait for flag.

    @return None.
*/
VOID nue_wait_ll_frameend(BOOL is_clr_flag)
{
#if 1
	FLGPTN uiFlag;
	if (is_clr_flag == TRUE) {
		nue_pt_clr_flg(FLGPTN_NUE_LLEND, g_nue_cb_flg);
	}
	nue_pt_wai_flg(&uiFlag, FLGPTN_NUE_LLEND);
#endif
}

/**
    NUE Set Parameters to Engine

    Set NUE parameters to engine.

    @param[in] Mode.
            - NUE_OPMODE_SVM        : NUE
            - NUE_OPMODE_RELU       : RELU
            - NUE_OPMODE_DOT        : Dot
            - NUE_OPMODE_USERDEFINE : User Define
    @param[in] p_parm NUE configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER nue_set_para2eng(NUE_OPMODE mode, NUE_PARM *p_parm)
{
	ER er_return = E_OK;
	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}
#if NUE_PRINT_PARM
	DBGD(p_parm->intype);
	DBGD(p_parm->outtype);
	DBGD(p_parm->insize.width);
	DBGD(p_parm->insize.height);
	DBGD(p_parm->insize.channel);

	DBGD(p_parm->insvm_size.insize);
	DBGD(p_parm->insvm_size.channel);
	DBGD(p_parm->insvm_size.objnum);
	DBGD(p_parm->insvm_size.sv_w);
	DBGD(p_parm->insvm_size.sv_h);

	DBGD(p_parm->userdef.inelt_type);
	DBGD(p_parm->userdef.outelt_type);
	DBGD(p_parm->elt_parm.is_insigned);
	DBGD(p_parm->elt_parm.shf0);
	DBGD(p_parm->elt_parm.coef0);
	DBGD(p_parm->elt_parm.shf1);
	DBGD(p_parm->elt_parm.coef1);
	DBGD(p_parm->elt_parm.out_right_shf);

	DBGD(p_parm->relu_parm.relu_type);
	DBGD(p_parm->relu_parm.leaky_val);
	DBGD(p_parm->relu_parm.leaky_shf);
	DBGD(p_parm->relu_parm.out_shf);

	DBGD(p_parm->globalpool_parm.ker_type);
	DBGD(p_parm->globalpool_parm.out_shf);
	DBGD(p_parm->globalpool_parm.avg_mul);
	DBGD(p_parm->globalpool_parm.avg_shf);
#endif

	nue_set_drvmode(mode);

	switch (mode) {
	case NUE_OPMODE_SVM:
		er_return = nue_set_engmode(NUE_SVM);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		//SVM mode
		er_return = nue_set_ker(p_parm->svm_parm.svmker_type);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_rstmode(NUE_SVMRST_FULL);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_dmao_en(DISABLE);
		//SVM
		nue_set_in_rfh(p_parm->svm_parm.in_rfh);
		er_return = nue_set_parm(&p_parm->svm_parm.svmker_parms);
		if (er_return != E_OK) {
			return er_return;
		}
		//FCD
		er_return = nue_set_fcd_parm(&p_parm->svm_parm.fcd_parm);
		if (er_return != E_OK) {
			return er_return;
		}
		break;
	case NUE_OPMODE_M_LINEAR_SVM:
		er_return = nue_set_engmode(NUE_SVM);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		nue_set_kerl_en(DISABLE, (NUE_SVM_KER2_EN | NUE_RELU_EN));
		nue_set_kerl_en(ENABLE, NUE_SVM_CAL_EN);
		er_return = nue_set_kerl1mode(NUE_SVMKER1_DOT);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_rstmode(NUE_SVMRST_SUB_RHO);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_dmao_en(ENABLE);
		er_return = nue_set_dmao_path(NUE_OUT_RST);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_in_rfh(p_parm->svm_parm.in_rfh);
		er_return = nue_set_parm(&p_parm->svm_parm.svmker_parms);
		if (er_return != E_OK) {
			return er_return;
		}
		//FCD
		er_return = nue_set_fcd_parm(&p_parm->svm_parm.fcd_parm);
		if (er_return != E_OK) {
			return er_return;
		}
		break;
	case NUE_OPMODE_DOT:
		er_return = nue_set_engmode(NUE_SVM);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		nue_set_kerl_en(DISABLE, (NUE_SVM_KER2_EN | NUE_RELU_EN | NUE_SVM_CAL_EN));
		er_return = nue_set_kerl1mode(NUE_SVMKER1_DOT);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_rstmode(NUE_SVMRST_FULL);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_dmao_en(ENABLE);
		er_return = nue_set_dmao_path(NUE_OUT_RST);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_in_rfh(p_parm->svm_parm.in_rfh);
		er_return = nue_set_parm(&p_parm->svm_parm.svmker_parms);
		if (er_return != E_OK) {
			return er_return;
		}
		//FCD
		er_return = nue_set_fcd_parm(&p_parm->svm_parm.fcd_parm);
		if (er_return != E_OK) {
			return er_return;
		}
		break;
	case NUE_OPMODE_FULLYCONNECT:
		er_return = nue_set_engmode(NUE_SVM);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		nue_set_kerl_en(DISABLE, (NUE_SVM_KER2_EN | NUE_RELU_EN | NUE_SVM_CAL_EN));
		er_return = nue_set_kerl1mode(NUE_SVMKER1_DOT);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_rstmode(NUE_SVMRST_SUB_RHO);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_dmao_en(ENABLE);
		er_return = nue_set_dmao_path(NUE_OUT_RST);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_in_rfh(p_parm->svm_parm.in_rfh);
		er_return = nue_set_parm(&p_parm->svm_parm.svmker_parms);
		if (er_return != E_OK) {
			return er_return;
		}
		//FCD
		er_return = nue_set_fcd_parm(&p_parm->svm_parm.fcd_parm);
		if (er_return != E_OK) {
			return er_return;
		}
		break;
	case NUE_OPMODE_ROIPOOLING:
		er_return = nue_set_engmode(NUE_ROIPOOLING);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		er_return = nue_set_roinum(p_parm->roipool_parm.roi_num);
		if (er_return != E_OK) {
			return er_return;
		}
		if ((p_parm->roipool_parm.olofs & 0x3) != 0){
			nvt_dbg(ERR, "NUE output lineoffset should be word-align.\r\n");
			return E_PAR;
		}
		nue_set_dmaout_lofs(p_parm->roipool_parm.olofs);
		nue_set_roipool_kersize(p_parm->roipool_parm.size);
		er_return = nue_set_roipool_ratio(p_parm->roipool_parm.ratio_mul, p_parm->roipool_parm.ratio_shf);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_roipool_shf(p_parm->roipool_parm.out_shf);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_roipool_mode(p_parm->roipool_parm.mode);
		if (er_return != E_OK) {
			return er_return;
		}
		break;
	case NUE_OPMODE_PERMUTE:
		er_return = nue_set_engmode(NUE_PERMUTE);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_permute_mode(p_parm->permute_parm.mode);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_permute_shf(p_parm->permute_parm.out_shf);
		nue_set_dmaout_lofs(p_parm->permute_parm.olofs);
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		break;
	case NUE_OPMODE_REORG:
		er_return = nue_set_engmode(NUE_REORGANIZATION);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		er_return =  nue_set_reorgshf(p_parm->reorg_parm.out_shf);
		if (er_return != E_OK) {
			return er_return;
		}
		break;
	case NUE_OPMODE_ANCHOR:
		er_return = nue_set_engmode(NUE_ANCHOR);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return =  nue_set_anchor_shf(p_parm->anchor_parm.at_w_shf);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_anchor_table_update(p_parm->anchor_parm.at_table_update);
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		break;
	case NUE_OPMODE_SOFTMAX:
		er_return = nue_set_engmode(NUE_SOFTMAX);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_softmax_in_shf(p_parm->softmax_parm.in_shf);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_softmax_out_shf(p_parm->softmax_parm.out_shf);
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = nue_set_softmax_group_num(p_parm->softmax_parm.group_num);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_softmax_setnum(p_parm->softmax_parm.set_num);
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		break;
	case NUE_OPMODE_USERDEFINE:
	case NUE_OPMODE_USERDEFINE_NO_INT:
		er_return = nue_set_engmode(p_parm->userdef.eng_mode);
		if (er_return != E_OK) {
			return er_return;
		}
		nue_set_intype(p_parm->intype);
		nue_set_outtype(p_parm->outtype);
		nue_set_kerl_en(DISABLE, NUE_ALL_EN);
		nue_set_kerl_en(ENABLE, p_parm->userdef.func_en);
		//SVM mode
		if (p_parm->userdef.eng_mode == NUE_SVM) {
			er_return = nue_set_kerl1mode(p_parm->userdef.svm_userdef.kerl1type);
			if (er_return != E_OK) {
				return er_return;
			}
			er_return = nue_set_kerl2mode(p_parm->userdef.svm_userdef.kerl2type);
			if (er_return != E_OK) {
				return er_return;
			}
			er_return = nue_set_rstmode(p_parm->userdef.svm_userdef.rslttype);
			if (er_return != E_OK) {
				return er_return;
			}
			nue_set_dmao_en(p_parm->userdef.svm_userdef.dmao_en);
			er_return = nue_set_dmao_path(p_parm->userdef.svm_userdef.dmao_path);
			if (er_return != E_OK) {
				return er_return;
			}

			//SVM
			nue_set_in_rfh(p_parm->svm_parm.in_rfh);
			er_return = nue_set_parm(&p_parm->svm_parm.svmker_parms);
			if (er_return != E_OK) {
				return er_return;
			}
			//ReLU
			er_return = nue_set_relu_leaky(p_parm->relu_parm.leaky_val, p_parm->relu_parm.leaky_shf);
			if (er_return != E_OK) {
				return er_return;
			}
			er_return = nue_set_relu_shf(p_parm->relu_parm.out_shf);
			if (er_return != E_OK) {
				return er_return;
			}
			
			//FCD
			er_return = nue_set_fcd_parm(&p_parm->svm_parm.fcd_parm);
			if (er_return != E_OK) {
				return er_return;
			}
		}
		
		//ROI Pooling
		if (p_parm->userdef.eng_mode == NUE_ROIPOOLING) {
			er_return = nue_set_roinum(p_parm->roipool_parm.roi_num);
			if (er_return != E_OK) {
				return er_return;
			}
			if ((p_parm->roipool_parm.olofs & 0x3) != 0){
				nvt_dbg(ERR, "NUE output lineoffset should be word-align.\r\n");
				return E_PAR;
			}
			nue_set_dmaout_lofs(p_parm->roipool_parm.olofs);
			nue_set_roipool_kersize(p_parm->roipool_parm.size);
			er_return = nue_set_roipool_ratio(p_parm->roipool_parm.ratio_mul, p_parm->roipool_parm.ratio_shf);
			if (er_return != E_OK) {
				return er_return;
			}
			er_return = nue_set_roipool_shf(p_parm->roipool_parm.out_shf);
			if (er_return != E_OK) {
				return er_return;
			}
			er_return = nue_set_roipool_mode(p_parm->roipool_parm.mode);
			if (er_return != E_OK) {
				return er_return;
			}
		}
		
		//Reorg
		if (p_parm->userdef.eng_mode == NUE_REORGANIZATION) {
			er_return = nue_set_reorgshf(p_parm->reorg_parm.out_shf);
			if (er_return != E_OK) {
				return er_return;
			}
		}
		
		//Permute
		if (p_parm->userdef.eng_mode == NUE_PERMUTE) {
			er_return = nue_set_permute_mode(p_parm->permute_parm.mode);
			if (er_return != E_OK) {
				return er_return;
			}
			nue_set_permute_shf(p_parm->permute_parm.out_shf);
			nue_set_dmaout_lofs(p_parm->permute_parm.olofs);
		}
		
		//Anchor transform
		if (p_parm->userdef.eng_mode == NUE_ANCHOR) {
			er_return = nue_set_anchor_shf(p_parm->anchor_parm.at_w_shf);
			if (er_return != E_OK) {
				return er_return;
			}
			nue_set_anchor_table_update(p_parm->anchor_parm.at_table_update);
		}
		
		//Softmax
		if (p_parm->userdef.eng_mode == NUE_SOFTMAX) {
			er_return = nue_set_softmax_in_shf(p_parm->softmax_parm.in_shf);
			if (er_return != E_OK) {
				return er_return;
			}
			er_return = nue_set_softmax_out_shf(p_parm->softmax_parm.out_shf);
			if (er_return != E_OK) {
				return er_return;
			}
			er_return = nue_set_softmax_group_num(p_parm->softmax_parm.group_num);
			if (er_return != E_OK) {
				return er_return;
			}
			nue_set_softmax_setnum(p_parm->softmax_parm.set_num);
		}
		break;
	default:
		nvt_dbg(ERR, "Unknown operation mode %d!\r\n", mode);
		return E_NOSPT;
	}

	if (nue_get_engmode() == NUE_SVM) {
		er_return = nue_set_svmsize(&p_parm->insvm_size);
	} else {
		er_return = nue_set_insize(&p_parm->insize);
	}
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = nue_set_dmaio_addr(p_parm->dmaio_addr);
	if (er_return != E_OK) {
		return er_return;
	}

	nue_set_in_shf(p_parm->in_parm.in_shf);
	nue_set_in_scale(p_parm->in_parm.in_scale);
	if ((p_parm->userdef.eng_mode == NUE_PERMUTE) && (p_parm->permute_parm.mode == 0)) {
		nue_set_permute_stripe(p_parm->userdef.permute_stripe_en);
	}
	else {
		if (p_parm->userdef.permute_stripe_en != 0) {
			nvt_dbg(ERR, "NUE permute stripe_en should be set 0.\r\n");
			return E_PAR;
		}
		nue_set_permute_stripe(p_parm->userdef.permute_stripe_en);
	}
	nue_set_clamp_th0(p_parm->clamp_parm.clamp_th_0);
	nue_set_clamp_th1(p_parm->clamp_parm.clamp_th_1);

	if (nue_chk_ker1_overflow()) {
		er_return =  E_CTX;
	}

	return er_return;
}

/**
    NUE Set Mode

    Set NUE engine mode

    @param[in] Mode.
            - NUE_OPMODE_SVM        : NUE
            - NUE_OPMODE_RELU       : RELU
            - NUE_OPMODE_DOT        : Dot
            - NUE_OPMODE_USERDEFINE : User Define
    @param[in] p_parm NUE configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_setmode(NUE_OPMODE mode, NUE_PARM *p_parm)
{
	ER er_return = nue_chk_state_machine(NUE_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	// Clear engine interrupt status
	nue_enable_int(DISABLE, NUE_INTE_ALL);
	nue_clr_intr_status(NUE_INT_ALL);

	er_return = nue_set_para2eng(mode, p_parm);
	if (er_return != E_OK) {
		return er_return;
	}

	if(mode != NUE_OPMODE_USERDEFINE_NO_INT) {
		nue_enable_int(ENABLE, NUE_INTE_ALL);
	}

#if PROF
	nue_pt_set_is_setmode(1);
#endif
	er_return = nue_pause();
	if (er_return != E_OK) {
		return er_return;
	}
#if PROF
	nue_pt_set_is_setmode(0);
#endif

	er_return = nue_chk_state_machine(NUE_OP_SET2PAUSE, TRUE);

	return er_return;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
    NUE Linked List Set Mode

    Set linked list NUE engine mode.

    @param[in] ll_addr NUE linked list address.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_ll_setmode(NUE_LL_PRM *ll_prm)
{
	ER er_return = nue_chk_state_machine(NUE_OP_SET2PAUSE, FALSE);

	if (er_return != E_OK) {
		return er_return;
	}

	// Clear engine interrupt status
	nue_enable_int(DISABLE, NUE_INTE_ALL);
	nue_clr_intr_status(NUE_INT_ALL);

	nue_set_dmain_lladdr(ll_prm->addrin_ll);
	nue_set_dmain_lladdr_base(ll_prm->addrin_ll_base);
	nue_enable_int(ENABLE, NUE_INTE_RUNLL);

#if PROF
	nue_pt_set_is_setmode(1);	
#endif
	er_return = nue_ll_pause();
	if (er_return != E_OK) {
		return er_return;
	}
#if PROF
	nue_pt_set_is_setmode(0);
#endif

	er_return = nue_chk_state_machine(NUE_OP_SET2PAUSE, TRUE);
	return er_return;
}

/**
	NUE Create Resource
*/
VOID nue_create_resource(VOID *parm, UINT32 clk_freq)
{
	nue_pt_create_resource(parm, clk_freq);
    SEM_CREATE(semid_nue, 1);
	SEM_CREATE(semid_nue_lib, 1);
}

/**
	NUE Release Resource
*/
VOID nue_release_resource(VOID)
{
	nue_pt_rel_flg();
	SEM_DESTROY(semid_nue);
	SEM_DESTROY(semid_nue_lib);
}

/**
	NUE Set Base Address

	Set NUE engine registers address.

	@param[in] addr registers address.

	@return none.
*/
VOID nue_set_base_addr(UINT32 addr)
{
	nue_reg_base_addr = addr;
}

/**
	NUE Get Base Address

	Get NUE engine registers address.

	@return registers address.
*/
UINT32 nue_get_base_addr(VOID)
{
	return nue_reg_base_addr;
}
/**
	NUE init

	init clk related info

	@return status.
*/
UINT32 nue_init(VOID)
{
	UINT32 clk_freq_mhz = 0;

	nue_wait_dma_abort();
	
	nue_lib_lock();
	if (g_nue_enable_cnt == 0) {
		clk_freq_mhz = nue_get_clock_rate();
		if (clk_freq_mhz == 0) {
			nue_set_clock_rate(600);
		} else {
			nue_set_clock_rate(clk_freq_mhz);
		}
		nue_attach();
		nvt_disable_sram_shutdown(NUE_SD);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
		dma_set_channel_outstanding(DMA_CH_OUTSTANDING_NUE_ALL, 1);
#endif
		nue_cycle_en(ENABLE);
	}
	g_nue_enable_cnt++;
	nue_lib_unlock();
	return 0;
}

/**
	NUE uninit

	uninit clk related info

	@return status.
*/
UINT32 nue_uninit(VOID)
{
	nue_lib_lock();
	g_nue_enable_cnt--;
	if (g_nue_enable_cnt == 0) {
		// Detach
		nue_detach();
		nvt_enable_sram_shutdown(NUE_SD);
	}	
	nue_lib_unlock();
	return 0;
}

VOID nue_reset_status(VOID)
{
	ER er_return;

	if (g_nue_lock_status != NO_TASK_LOCKED) {
		er_return = nue_close();
		if (er_return != E_OK) {
			nvt_dbg(ERR, "nue_reset_status: Error at nue_close().\r\n");
			goto err_exit;
		}
	}
	g_nue_status = NUE_ENGINE_IDLE;

err_exit:

	return;
}

VOID nue_dma_abort(VOID)
{
	BOOL is_dma_disable;

    is_dma_disable = nue_get_dma_disable();
    if (is_dma_disable == 1) {
        nvt_dbg(IND, "NUE_DMA_ABORT: it has been in dma_disable state.\r\n");
        return;
    }
	nue_set_dma_disable(1);

	return;
}

static VOID nue_wait_dma_abort(VOID)
{
    BOOL is_dma_disable;

    is_dma_disable = nue_get_dma_disable();
    while (is_dma_disable == 1) {
		vos_task_delay_ms(g_nue_dma_abort_delay);
		is_dma_disable = nue_get_dma_disable();
    }

    return;
}

EXPORT_SYMBOL(nue_init);
EXPORT_SYMBOL(nue_uninit);
EXPORT_SYMBOL(nue_isr);
EXPORT_SYMBOL(nue_get_clock_rate);
//EXPORT_SYMBOL(nue_chk_state_machine);
EXPORT_SYMBOL(nue_change_interrupt);
//EXPORT_SYMBOL(nue_lock);
//EXPORT_SYMBOL(nue_unlock);
//EXPORT_SYMBOL(nue_attach);
//EXPORT_SYMBOL(nue_detach);
//EXPORT_SYMBOL(nue_set_clock_rate);

EXPORT_SYMBOL(nue_open);
EXPORT_SYMBOL(nue_is_opened);
EXPORT_SYMBOL(nue_close);
EXPORT_SYMBOL(nue_ll_pause);
EXPORT_SYMBOL(nue_ll_start);
EXPORT_SYMBOL(nue_ll_isr_start);
EXPORT_SYMBOL(nue_reset);
//EXPORT_SYMBOL(nue_clr_all);
//EXPORT_SYMBOL(nue_clr_frameend);
EXPORT_SYMBOL(nue_wait_frameend);
//EXPORT_SYMBOL(nue_clr_ll_frameend);
EXPORT_SYMBOL(nue_wait_ll_frameend);
//EXPORT_SYMBOL(nue_set_para2eng);
//EXPORT_SYMBOL(nue_set_para2linkedlist);
EXPORT_SYMBOL(nue_ll_setmode);

EXPORT_SYMBOL(nue_create_resource);
EXPORT_SYMBOL(nue_release_resource);
EXPORT_SYMBOL(nue_set_base_addr);
EXPORT_SYMBOL(nue_get_base_addr);
EXPORT_SYMBOL(nue_reset_status);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
EXPORT_SYMBOL(nue_isr_start);
EXPORT_SYMBOL(nue_pause);
EXPORT_SYMBOL(nue_start);
EXPORT_SYMBOL(nue_setmode);
#endif
EXPORT_SYMBOL(nue_dma_abort);

//@}
