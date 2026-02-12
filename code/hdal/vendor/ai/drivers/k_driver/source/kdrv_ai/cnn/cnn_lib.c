/*
CNN module driver

NT98520 CNN driver.

@file       cnn_lib.c
@ingroup    mIIPPCNN

Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "cnn_platform.h"
#include "cnn_lib.h"
#include "cnn_reg.h"
#include "cnn_int.h"
#include "cnn_ll_cmd.h"
#include <kwrap/verinfo.h> 
#include "efuse_protected_na51055.h"
#include "kwrap/task.h"

//=============================================================
#define __CLASS__ 				"[ai][kdrv][cnn]"
#include "kdrv_ai_debug.h"
//=============================================================

/**
@addtogroup mIIPPCNN
*/
//@{

#if 1
#define FLGPTN_CNN_FRM_DONE             FLGPTN_BIT(0)
#define FLGPTN_CNN_SRC_ILLEGALSIZE      FLGPTN_BIT(2)
#define FLGPTN_CNN_CONVOUT_ILLEGALSIZE  FLGPTN_BIT(3)
#define FLGPTN_CNN_LLEND                FLGPTN_BIT(8)

#define FLGPTN_CNN_LLERROR              FLGPTN_BIT(9)
#define FLGPTN_CNN_FCD_DECODE_DONE      FLGPTN_BIT(16)
#define FLGPTN_CNN_FCD_VLC_DEC_ERR      FLGPTN_BIT(17)
#define FLGPTN_CNN_FCD_BS_SIZE_ERR      FLGPTN_BIT(18)
#define FLGPTN_CNN_FCD_SPARSE_DATA_ERR  FLGPTN_BIT(19)
#define FLGPTN_CNN_FCD_SPARSE_INDEX_ERR FLGPTN_BIT(20)
#endif //#if 1

#define CNN_PRINT_PARM                  DISABLE

#define PROF                            DISABLE
#if PROF
static struct timeval tstart, tend;
static INT32 is_setmode = 0;
#define PROF_START()    do_gettimeofday(&tstart);
#define PROF_END(msg) \
	if (is_setmode == 0) { \
	do_gettimeofday(&tend); \
	DBG_DUMP("%s time (us): %lu\r\n", msg, \
	(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec)); \
	}
#else
#define PROF_START()
#define PROF_END(msg)
#endif


UINT32 cnn_reg_base_addr1   = 0;
UINT32 cnn_reg_base_addr2   = 0;
UINT32 cnn_ll_base_addr1    = 0;
UINT32 cnn_ll_base_addr2    = 0;

UINT32 g_cnn_enable_cnt[2] = {0};
BOOL g_cnn_cb1_flg = 0;
BOOL g_cnn_cb2_flg = 0;
static UINT32 g_cnn_dma_abort_delay=1000;

UINT32 cnn_perf_en[2]     = {0};
UINT32 cnn_perf_status[2] = {0};
UINT32 cnn_eng_time[2]    = {0};
#if defined(__LINUX)
static struct timeval cnn_tstart[2], cnn_tend[2];
static struct timeval cnn_total_tstart[2], cnn_total_tend[2];
#endif
UINT32 cnn_total_time[2] = {0};
static UINT32 cnn_status1       = CNN_ENGINE_IDLE;
static ID cnn_lock_status1      = NO_TASK_LOCKED;
static VOID (*p_cnn_int_cb1)(UINT32 int_status);
static UINT32 cnn_status2       = CNN_ENGINE_IDLE;
static ID cnn_lock_status2      = NO_TASK_LOCKED;
static VOID (*p_cnn_int_cb2)(UINT32 int_status);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER cnn_set_para2eng(BOOL cnn_id, CNN_OPMODE mode, CNN_PARM *p_parms);
#endif

static ER   cnn_chk_state_machine(BOOL cnn_id, UINT32 operation, BOOL update);
static ER   cnn_lock(BOOL cnn_id);
static ER   cnn_unlock(BOOL cnn_id);

static VOID cnn_clr_all(BOOL cnn_id);

static VOID cnn_wait_dma_abort(BOOL cnn_id);
//static VOID cnn_clr_frameend(BOOL cnn_id);
//static VOID cnn_clr_ll_frameend(BOOL cnn_id);

VOID cnn_set_perf_en(UINT32 enable, UINT32 cnn_id)
{
#if defined(__LINUX)
	if (cnn_id >= 2) {
		DBG_ERR( "cnn_set_perf_en: invalid cnn id %d\r\n", cnn_id);
		return;
	}
	
	if (enable) {
		cnn_perf_en[cnn_id] = enable;
		cnn_eng_time[cnn_id] = 0;
		do_gettimeofday(&cnn_total_tstart[cnn_id]);
	} else {
		if (cnn_perf_en[cnn_id] == 1) {
			do_gettimeofday(&cnn_total_tend[cnn_id]); 
			cnn_total_time[cnn_id] = (cnn_total_tend[cnn_id].tv_sec - cnn_total_tstart[cnn_id].tv_sec) * 1000000 + (cnn_total_tend[cnn_id].tv_usec - cnn_total_tstart[cnn_id].tv_usec);
			cnn_perf_en[cnn_id] = 0;
		}
	}
#endif
}

UINT32 cnn_get_eng_time(UINT32 cnn_id)
{
	if (cnn_id >= 2) {
		DBG_ERR( "cnn_get_eng_time: invalid cnn id %d\r\n", cnn_id);
		return 0;
	}
	
	return cnn_eng_time[cnn_id];
}

UINT32 cnn_get_eng_util(UINT32 cnn_id)
{
	if (cnn_id >= 2) {
		DBG_ERR( "cnn_get_eng_util: invalid cnn id %d\r\n", cnn_id);
		return 0;
	}
	
	if (cnn_total_time[cnn_id] == 0) {
		//DBG_ERR( "cnn_get_eng_util: invalid total time in cnn%d\r\n", cnn_id);
		return 0;
	}
	
	return 100*cnn_eng_time[cnn_id]/cnn_total_time[cnn_id];
}
#if CNN_CYCLE_TEST
UINT64 cnn_total_cycle[2] = {0};
#if CNN_CYCLE_LAYER
UINT32 cnn_cnt = 0;
UINT32 cnn_dbg_cnt[200] = {0};
#endif
UINT64 cnn_dump_total_cycle(UINT32 debug_layer)
{
	UINT64 total_cost = 0;
	if(nvt_get_chip_id() == CHIP_NA51055) {
		total_cost = 5*cnn_total_cycle[0] + 5*cnn_total_cycle[1];
	} else {
		total_cost = cnn_total_cycle[0] + cnn_total_cycle[1];
	}
#if CNN_CYCLE_LAYER
	if(debug_layer==1) {
		UINT32 i = 0;
		for (i = 0; i < cnn_cnt; i++) {
			DBG_DUMP("item %d cnn cycle = %d\n", i, (unsigned int)cnn_dbg_cnt[i]);
		}
	}
	//#else
	//DBG_DUMP("CNN_CYCLE_LAYER is disable now.\n");
#endif
	//cnn_total_cycle[0] *= 5;
	//cnn_total_cycle[1] *= 5;
	//DBG_DUMP("cnn1 total cycle = 0x%08X%08X\n", (unsigned int)(cnn_total_cycle[0] >> 32), (unsigned int)(cnn_total_cycle[0] & 0xFFFFFFFF));
	//DBG_DUMP("cnn2 total cycle = 0x%08X%08X\n", (unsigned int)(cnn_total_cycle[1] >> 32), (unsigned int)(cnn_total_cycle[1] & 0xFFFFFFFF));
	cnn_total_cycle[0] = 0;
	cnn_total_cycle[1] = 0;
	return total_cost;
}
#endif
/**
CNN Interrupt Service Routine

CNN interrupt service routine.

@return None.
*/
VOID cnn_isr(BOOL cnn_id)
{
	UINT32 cnn_int_status = cnn_get_intr_status(cnn_id);
	cnn_int_status = cnn_int_status & cnn_get_int_enable(cnn_id);

	if (cnn_int_status == 0) {
		//DBG_ERR("cnn_int_status isr err\r\n");
		return;
	}
	cnn_clr_intr_status(cnn_id, cnn_int_status);

	if (cnn_int_status & CNN_INTE_FRM_DONE) {
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_FRM_DONE);
#if CNN_CYCLE_TEST
#if CNN_CYCLE_LAYER
		cnn_dbg_cnt[cnn_cnt++] = (unsigned int)(cnn_getcycle(cnn_id)*5);
#endif
		cnn_total_cycle[cnn_id] += cnn_getcycle(cnn_id);
#endif
	}
	if (cnn_int_status & CNN_INTE_SRC_ILLEGALSIZE) {
		DBG_ERR( "Illegal input size\r\n");
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_SRC_ILLEGALSIZE);
	}
	if (cnn_int_status & CNN_INTE_CONVOUT_ILLEGALSIZE) {
		DBG_ERR("Illegal convolution output size\r\n");
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_CONVOUT_ILLEGALSIZE);
	}
	if (cnn_int_status & CNN_INTE_LLEND) {
#if defined(__LINUX)
		if (cnn_perf_status[cnn_id] == 1) {
			do_gettimeofday(&cnn_tend[cnn_id]); 
			cnn_eng_time[cnn_id] += ((cnn_tend[cnn_id].tv_sec - cnn_tstart[cnn_id].tv_sec) * 1000000 + (cnn_tend[cnn_id].tv_usec - cnn_tstart[cnn_id].tv_usec));
			cnn_perf_status[cnn_id] = 0;
		}
#endif
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_LLEND);
	}
	if (cnn_int_status & CNN_INTE_LLERROR) {
		DBG_ERR("Illegal linked list setting\r\n");
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_LLERROR);
	}
	if (cnn_int_status & CNN_INT_FCD_DECODE_DONE) {
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_FCD_DECODE_DONE);
	}
	if (cnn_int_status & CNN_INT_FCD_VLC_DEC_ERR) {
		DBG_ERR("Bitstream lens overflow\r\n");
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_FCD_VLC_DEC_ERR);
	}
	if (cnn_int_status & CNN_INT_FCD_BS_SIZE_ERR) {
		DBG_ERR( "Bitstream size not align\r\n");
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_FCD_BS_SIZE_ERR);
	}
	if (cnn_int_status & CNN_INT_FCD_SPARSE_DATA_ERR) {
		DBG_ERR( "SPARSE odd number of encoded data\r\n");
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_FCD_SPARSE_DATA_ERR);
	}
	if (cnn_int_status & CNN_INT_FCD_SPARSE_INDEX_ERR) {
		DBG_ERR( "SPARSE index < 1\r\n");
		cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_FCD_SPARSE_INDEX_ERR);
	}

#if 0
	if (cnn_id == 0) {
		if ((p_cnn_int_cb1) && ((cnn_int_status & CNN_INTE_FRM_DONE) || (cnn_int_status & CNN_INTE_LLEND))) {
			cnn_pause(0);
			p_cnn_int_cb1(cnn_int_status);
		}
	} else {
		if ((p_cnn_int_cb2) && ((cnn_int_status & CNN_INTE_FRM_DONE) || (cnn_int_status & CNN_INTE_LLEND))) {
			cnn_pause(1);
			p_cnn_int_cb2(cnn_int_status);
		}
	}
#else
	if (cnn_id == 0) {
		if (p_cnn_int_cb1) {
			p_cnn_int_cb1(cnn_int_status);
		}
	} else {
		if (p_cnn_int_cb2) {
			p_cnn_int_cb2(cnn_int_status);
		}
	}
#endif
}

/**
CNN get operation clock

Get CNN clock selection.

@param None.

@return UINT32 CNN operation clock rate.
*/
UINT32 cnn_get_clock_rate(BOOL cnn_id)
{

	UINT32 clk_freq_mhz = 0;

	clk_freq_mhz = cnn_pt_get_clock_rate(cnn_id);

	return clk_freq_mhz;
}

/*
CNN State Machine

CNN state machine.

@param[in] operation the operation to execute.
@param[in] update if we will update state machine status.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_chk_state_machine(BOOL cnn_id, UINT32 operation, BOOL update)
{
	ER er_return = E_OK;
	UINT32 cnn_status = CNN_ENGINE_IDLE;

	if (cnn_id == 0) {
		cnn_status = cnn_status1;
	} else  {
		cnn_status = cnn_status2;
	}

	switch (cnn_status) { //ori state
	case CNN_ENGINE_IDLE:
		switch (operation) { //wanna go state
		case CNN_OP_ATTACH:
			if (update) {
				cnn_status = CNN_ENGINE_IDLE;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		case CNN_OP_DETACH:
			if (update) {
				cnn_status = CNN_ENGINE_IDLE;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		case CNN_OP_OPEN:
			if (update) {
				cnn_status = CNN_ENGINE_READY;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		default:
			er_return = E_OBJ;
			break;
		}
		break;

	case CNN_ENGINE_READY:
		switch (operation) {
		case CNN_OP_CLOSE:
			if (update) {
				cnn_status = CNN_ENGINE_IDLE;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		case CNN_OP_SET2READY:
		case CNN_OP_READLUT:
		case CNN_OP_CHGINT:
			if (update) {
				cnn_status = CNN_ENGINE_READY;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		case CNN_OP_SET2PAUSE:
			if (update) {
				cnn_status = CNN_ENGINE_PAUSE;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		default:
			er_return = E_OBJ;
			break;
		}
		break;

	case CNN_ENGINE_PAUSE:
		switch (operation) {
		case CNN_OP_SET2PAUSE:
		case CNN_OP_READLUT:
		case CNN_OP_CHGINT:
			if (update) {
				cnn_status = CNN_ENGINE_PAUSE;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		case CNN_OP_SET2RUN:
			if (update) {
				cnn_status = CNN_ENGINE_RUN;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		case CNN_OP_SET2READY:
			if (update) {
				cnn_status = CNN_ENGINE_READY;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		case CNN_OP_CLOSE:
			if (update) {
				cnn_status = CNN_ENGINE_IDLE;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		default :
			er_return = E_OBJ;
			break;
		}
		break;

	case CNN_ENGINE_RUN:
		switch (operation) {
		case CNN_OP_SET2RUN:
		case CNN_OP_CHGINT:
			if (update) {
				cnn_status = CNN_ENGINE_RUN;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
			break;
		case CNN_OP_SET2PAUSE:
			if (update) {
				cnn_status = CNN_ENGINE_PAUSE;
			}
			er_return = E_OK;
			//DBG_ERR("cnn_chk_state_machine cnn_status:%d\n", cnn_status);
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
		DBG_ERR( "State machine error! st %d op %d\r\n", (int)cnn_status, (int)operation);
	}
	if (cnn_id == 0) {
		cnn_status1 = cnn_status;
	} else  {
		cnn_status2 = cnn_status;
	}

	return er_return;
}

/**
CNN change interrupt

CNN interrupt configuration.

for FPGA verification only.

@param[in] int_en Interrupt enable setting.

@return ER CNN change interrupt status.
*/
ER cnn_change_interrupt(BOOL cnn_id, UINT32 int_en)
{
	ER er_return = E_OK;

	//DBG_ERR("cnn_interrupt: cnn_chk_state_machine\n");
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_CHGINT, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	cnn_enable_int(cnn_id, ENABLE, int_en);
	//DBG_ERR("cnn_interrupt_ed: cnn_chk_state_machine\n");
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_CHGINT, TRUE);

	return er_return;
}

/**
CNN Lock Function

Lock CNN engine.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_lock(BOOL cnn_id)
{
#if 0
	if (cnn_id == 0) {
		if (cnn_lock_status1 == NO_TASK_LOCKED) {
			ER er_return = SEM_WAIT(semid_cnn1);
			if (er_return != E_OK) {
				return er_return;
			}
			cnn_lock_status1 = TASK_LOCKED;
			return er_return;
		} else {
			return E_OBJ;
		}
	} else {
		if (cnn_lock_status2 == NO_TASK_LOCKED) {
			ER er_return = SEM_WAIT(semid_cnn2);
			if (er_return != E_OK) {
				return er_return;
			}
			cnn_lock_status2 = TASK_LOCKED;
			return er_return;
		} else {
			return E_OBJ;
		}
	}
#else
	ER er_return=E_OK;

#if (KDRV_AI_MINI_NO_LOCK == 1)
#else
	er_return = cnn_pt_sem_wait(cnn_id);
	if (er_return != E_OK) {
		return er_return;
	}
#endif //#if (KDRV_AI_MINI_NO_LOCK == 1)

	if (cnn_id == 0) {
		cnn_lock_status1 = TASK_LOCKED;
	} else {
		cnn_lock_status2 = TASK_LOCKED;
	}

	return er_return;
#endif
}

/**
CNN Unlock Function

Unlock CNN engine.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_unlock(BOOL cnn_id)
{
#if 0
	if (cnn_id == 0) {
		if (cnn_lock_status1 == TASK_LOCKED) {
			cnn_lock_status1 = NO_TASK_LOCKED;
			SEM_SIGNAL(semid_cnn1);
			return E_OK;
		} else {
			return E_OBJ;
		}
	} else {
		if (cnn_lock_status2 == TASK_LOCKED) {
			cnn_lock_status2 = NO_TASK_LOCKED;
			SEM_SIGNAL(semid_cnn2);
			return E_OK;
		} else {
			return E_OBJ;
		}
	}

#else
	if (cnn_id == 0) {
		cnn_lock_status1 = NO_TASK_LOCKED;
#if (KDRV_AI_MINI_NO_LOCK == 1)
#else
		cnn_pt_sem_signal(cnn_id);
#endif //#if (KDRV_AI_MINI_NO_LOCK == 1)
	} else {
		cnn_lock_status2 = NO_TASK_LOCKED;
#if (KDRV_AI_MINI_NO_LOCK == 1)
#else
		cnn_pt_sem_signal(cnn_id);
#endif //#if (KDRV_AI_MINI_NO_LOCK == 1)
	}

	return E_OK;
#endif
}

/**
CNN lib Lock Function

Lock CNN lib.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/

ER cnn_lib_lock(BOOL cnn_id)
{
	ER er_return;

	er_return = cnn_pt_lib_sem_wait(cnn_id);

	return er_return;

}

/**
CNN lib Unlock Function

Unlock CNN lib.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_lib_unlock(BOOL cnn_id)
{
	cnn_pt_lib_sem_signal(cnn_id);

	return E_OK;
}

/**
CNN Set Clock Rate

Set current clock rate

@param[in] clock cnn clock rate

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_set_clock_rate(BOOL cnn_id, UINT32 clock)
{
	return cnn_pt_set_clock_rate(cnn_id, clock);
}

/**
CNN Open Driver

Open CNN engine
\n This function should be called after cnn_attach().

@param[in] p_obj_cb CNN open object.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_open(BOOL cnn_id, CNN_OPENOBJ *p_obj_cb)
{
	ER er_return;

	// Lock semaphore
	er_return = cnn_lock(cnn_id);
	if (er_return != E_OK) {
		return er_return;
	}
	//DBG_ERR("cnn_open: cnn_chk_state_machine\n");
	// Check state-machine
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_OPEN, FALSE);
	if (er_return != E_OK) {
		// release resource when stateMachine fail
		er_return = cnn_unlock(cnn_id);
		return er_return;
	}
#if 0
#if 0
	// Power on module
	pmc_turnonPower(PMC_MODULE_CNN);

	// Select PLL clock source & enable PLL clock source
	pll_setClockRate(PLL_CLKSEL_CNN, g_cnn_true_clock);
#endif
	if (p_obj_cb != NULL) {
		//er_return = cnn_set_clock_rate(p_obj_cb->clk_sel);
	}
	/*if (er_return != E_OK) {
	er_return = cnn_unlock(cnn_id);
	return er_return;
	}*/

	cnn_set_clock_rate(cnn_id, 600);
	// Attach
	cnn_pt_clk_enable(cnn_id);

#if 0
	// Disable Sram shut down
	//pll_enableSramShutDown(CNN_RSTN); // mismatch
	//pll_disableSramShutDown(CNN_RSTN);

	// Enable engine interrupt
	//drv_enableInt(DRV_INT_CNN);
	// Enable engine interrupt
	if (cnn_id == 0) {
		drv_enableInt(DRV_INT_CNN);
	} else {
		drv_enableInt(DRV_INT_CNN2);
	}
#else
	if (cnn_id == 0) {
		nvt_disable_sram_shutdown(CNN_SD);
	} else {
		nvt_disable_sram_shutdown(CNN2_SD);
	}
#endif



	// Clear engine interrupt status
	cnn_clr_intr_status(cnn_id, CNN_INT_ALL);

	// SW reset
	cnn_clr(cnn_id, 0);
	cnn_clr(cnn_id, 1);
	cnn_clr(cnn_id, 0);
#endif
	// Clear SW flag
	cnn_clr_all(cnn_id);

	// Hook call-back function
	if (cnn_id == 0) {
		if ((p_obj_cb != NULL) && (p_obj_cb->fp_isr_cb != NULL)) {
			p_cnn_int_cb1 = p_obj_cb->fp_isr_cb;
		} else {
			p_cnn_int_cb1 = NULL;
		}
	} else {
		if ((p_obj_cb != NULL) && (p_obj_cb->fp_isr_cb != NULL)) {
			p_cnn_int_cb2 = p_obj_cb->fp_isr_cb;
		} else {
			p_cnn_int_cb2 = NULL;
		}
	}




	// update state-machine
	//DBG_ERR("cnn_open_end: cnn_chk_state_machine\n");
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_OPEN, TRUE);

	return er_return;
}

/*
CNN Get Open Status

Check if CNN engine is opened.

@return
- @b FALSE: engine is not opened.
- @b TRUE: engine is opened.
*/
BOOL cnn_is_opened(BOOL cnn_id)
{
	if (cnn_id == 0) {
		if (cnn_lock_status1 == TASK_LOCKED) {
			return TRUE;
		} else {
			return FALSE;
		}
	} else {
		if (cnn_lock_status2 == TASK_LOCKED) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
}

/**
CNN Close Driver

Close CNN engine
\n This function should be called before cnn_pt_clk_disable().

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_close(BOOL cnn_id)
{
	ER er_return;

	//DBG_ERR("cnn_close: cnn_chk_state_machine\n");
	// Check state-machine
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_CLOSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Check semaphore
	if (!cnn_is_opened(cnn_id)) {
		return E_SYS;
	}

#if 0
#if 0
	if (cnn_id == 0) {
		// Diable engine interrupt
		drv_disableInt(DRV_INT_CNN);

		// Enable Sram shut down
		//pll_disableSramShutDown(CNN_RSTN);//mismatch
		pll_enableSramShutDown(CNN_RSTN);
	} else {
		// Diable engine interrupt
		drv_disableInt(DRV_INT_CNN2);

		// Enable Sram shut down
		//pll_disableSramShutDown(CNN_RSTN);//mismatch
		pll_enableSramShutDown(CNN2_RSTN);
	}

#endif

	// Detach
	cnn_pt_clk_disable(cnn_id);

#endif
	// Unhook call-back function
	if (cnn_id == 0) {
		p_cnn_int_cb1   = NULL;
	} else {
		p_cnn_int_cb2   = NULL;
	}


	// update state-machine
	//DBG_ERR("cnn_close_ed: cnn_chk_state_machine\n");
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_CLOSE, TRUE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Unlock semaphore
	er_return = cnn_unlock(cnn_id);

	return er_return;
}

/**
CNN Pause operation

Pause CNN engine.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER cnn_pause(BOOL cnn_id)
{
	ER er_return;

	PROF_END("kdrv_cnn");
	//DBG_DUMP("cnn cycle = %d\n", cnn_getcycle(cnn_id));
	//DBG_DUMP("cnn_pause: cnn_chk_state_machine\n");
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	cnn_enable(cnn_id, DISABLE);

	//DBG_DUMP("cnn_pause_ed: cnn_chk_state_machine\n");
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2PAUSE, TRUE);

	return er_return;
}
#endif

/**
CNN Linked List Pause operation

Pause linked list CNN engine.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_ll_pause(BOOL cnn_id)
{
	ER er_return = E_OK;

	PROF_END("kdrv_cnn_ll");

	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	cnn_ll_enable(cnn_id, DISABLE);
	cnn_ll_terminate(cnn_id, DISABLE);

	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2PAUSE, TRUE);

	return er_return;
}

/**
CNN Start operation

Start CNN engine.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER cnn_start(BOOL cnn_id)
{
	ER er_return;
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	
	if (cnn_eng_valid(cnn_id) == FALSE) {
		return E_OBJ;
	}

	if (cnn_id == 0) {
		g_cnn_cb1_flg = 0;
	}
	else {
		g_cnn_cb2_flg = 0;
	}

	// Clear SW flag
	cnn_clr_all(cnn_id);
	PROF_START();

	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2RUN, TRUE);
	cnn_enable(cnn_id, ENABLE);

	return er_return;
}

/**
CNN Start operation

Start CNN engine for isr flow

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_isr_start(BOOL cnn_id)
{
	ER er_return;
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	if (cnn_eng_valid(cnn_id) == FALSE) {
		return E_OBJ;
	}


	if (cnn_id == 0) {
		g_cnn_cb1_flg = 1;
	}
	else {
		g_cnn_cb2_flg = 1;
	}

	// Clear SW flag
	cnn_clr_all(cnn_id);
	PROF_START();

	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2RUN, TRUE);
	cnn_enable(cnn_id, ENABLE);

	return er_return;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
/**
CNN Linked List Start operation

Start linked list CNN engine.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_ll_start(BOOL cnn_id)
{
	ER er_return;
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	if (cnn_eng_valid(cnn_id) == FALSE) {
		return E_OBJ;
	}
	if (cnn_id == 0) {
		g_cnn_cb1_flg = 0;
	} else {
		g_cnn_cb2_flg = 0;
	}

	// Clear SW flag
	cnn_clr_all(cnn_id);

	PROF_START();
#if defined(__LINUX)
	if (cnn_perf_en[cnn_id] == 1) {
		cnn_perf_status[cnn_id] = 1;
		do_gettimeofday(&cnn_tstart[cnn_id]); 
	}
#endif
	cnn_ll_enable(cnn_id, ENABLE);
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2RUN, TRUE);

	return er_return;
}

/**
CNN Linked List Start operation

Start linked list CNN engine for isr flow.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_ll_isr_start(BOOL cnn_id)
{
	ER er_return;
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2RUN, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}
	if (cnn_eng_valid(cnn_id) == FALSE) {
		return E_OBJ;
	}
	if (cnn_id == 0) {
		g_cnn_cb1_flg = 1;
	}
	else {
		g_cnn_cb2_flg = 1;
	}
	// Clear SW flag
	cnn_clr_all(cnn_id);

	PROF_START();

	cnn_ll_enable(cnn_id, ENABLE);
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2RUN, TRUE);

	return er_return;
}

/**
CNN reset

Reset CNN engine.

@return None.
*/
VOID cnn_reset(BOOL cnn_id)
{	
	cnn_pt_get_engine_idle(cnn_id);
	cnn_clr(cnn_id, 1); // sw rst
	cnn_set_dma_disable(cnn_id, 0); // release dma disable
	cnn_clr(cnn_id, 0);
	cnn_clr_intr_status(cnn_id, CNN_INT_ALL);
	cnn_pt_iset_flg(cnn_id, FLGPTN_CNN_LLEND);
	//HW reset
	//VOID __iomem *ioaddr = NULL;
	//UINT32 ioread;
	//UINT32 reg_bit = 0x1;
	//const INT32 time_limit = 3000000;
	//struct timeval s_time, e_time;
	//INT32 dur_time = 0;	// (us)

	//do_gettimeofday(&s_time);

	//ioaddr = (VOID*)ioremap_nocache(0xfe000050, 0x200);
	//ioread = ioread32(ioaddr + 0x0);

	//cnn_set_axidis(TRUE);

	//while ((!cnn_get_axiidle()) && (dur_time < time_limit)) {
	//do_gettimeofday(&e_time);
	//dur_time = (e_time.tv_sec - s_time.tv_sec) * 1000000 + (e_time.tv_usec - s_time.tv_usec);
	//}

	//if (dur_time > time_limit) {
	//DBG_ERR("reset time out\r\n");
	//}

	//if (cnn_get_axiidle()) {
	//ioread &= (~reg_bit);
	//iowrite32(ioread, ioaddr + 0x0);

	//ioread |= (reg_bit);
	//iowrite32(ioread, ioaddr + 0x0);
	//}

	//cnn_set_axidis(FALSE);

	//iounmap(ioaddr);
}

/**
CNN Clear All

Clear CNN all flag.

@return None.
*/
VOID cnn_clr_all(BOOL cnn_id)
{
#if 1
	if (cnn_id == 0) {
		cnn_pt_clr_flg(cnn_id, (FLGPTN_CNN_FRM_DONE | FLGPTN_CNN_SRC_ILLEGALSIZE |
			FLGPTN_CNN_CONVOUT_ILLEGALSIZE | FLGPTN_CNN_LLEND | FLGPTN_CNN_LLERROR |
			FLGPTN_CNN_FCD_DECODE_DONE | FLGPTN_CNN_FCD_VLC_DEC_ERR | FLGPTN_CNN_FCD_BS_SIZE_ERR |
			FLGPTN_CNN_FCD_SPARSE_DATA_ERR | FLGPTN_CNN_FCD_SPARSE_INDEX_ERR), g_cnn_cb1_flg);
	}
	else {
		cnn_pt_clr_flg(cnn_id, (FLGPTN_CNN_FRM_DONE | FLGPTN_CNN_SRC_ILLEGALSIZE |
			FLGPTN_CNN_CONVOUT_ILLEGALSIZE | FLGPTN_CNN_LLEND | FLGPTN_CNN_LLERROR |
			FLGPTN_CNN_FCD_DECODE_DONE | FLGPTN_CNN_FCD_VLC_DEC_ERR | FLGPTN_CNN_FCD_BS_SIZE_ERR |
			FLGPTN_CNN_FCD_SPARSE_DATA_ERR | FLGPTN_CNN_FCD_SPARSE_INDEX_ERR), g_cnn_cb2_flg);
	}

#endif
	//cnn_clr_frameend(cnn_id);
	//cnn_clr_ll_frameend(cnn_id);

}

/**
CNN Clear Frame End

Clear CNN frame end flag.

@return None.
*/
/*
VOID cnn_clr_frameend(BOOL cnn_id)
{
#if 1
	if (cnn_id == 0) {
		if (p_cnn_int_cb1 == NULL) {
			g_cnn_cb_flg = 0;
		}
		else {
			g_cnn_cb_flg = 1;
		}
	}
	else {
		if (p_cnn_int_cb2 == NULL) {
			g_cnn_cb_flg = 0;
		}
		else {
			g_cnn_cb_flg = 1;
		}
	}
	cnn_pt_clr_flg(cnn_id, FLGPTN_CNN_FRM_DONE, g_cnn_cb_flg);
#endif
}
*/
/**
CNN Wait Frame End

Wait for CNN frame end flag.

@param[in] is_clr_flag
\n-@b TRUE: clear flag and wait for next flag.
\n-@b FALSE: wait for flag.

@return None.
*/
VOID cnn_wait_frameend(BOOL cnn_id, BOOL is_clr_flag)
{
#if 1
	FLGPTN flag;

	if (is_clr_flag == TRUE) {
		if (cnn_id == 0) {
			cnn_pt_clr_flg(cnn_id, FLGPTN_CNN_FRM_DONE, g_cnn_cb1_flg);
		}
		else {
			cnn_pt_clr_flg(cnn_id, FLGPTN_CNN_FRM_DONE, g_cnn_cb2_flg);
		}
	}
	cnn_pt_wai_flg(cnn_id, &flag, FLGPTN_CNN_FRM_DONE);

#endif
}

/**
CNN Clear Linked List Frame End

Clear CNN linked list frame end flag.

@return None.
*/
//VOID cnn_clr_ll_frameend(BOOL cnn_id)
//{
//#if 1
//	if (cnn_id == 0) {
//		if (p_cnn_int_cb1 == NULL) {
//			g_cnn_cb_flg = 0;
//		}
//		else {
//			g_cnn_cb_flg = 1;
//		}
//	}
//	else {
//		if (p_cnn_int_cb2 == NULL) {
//			g_cnn_cb_flg = 0;
//		}
//		else {
//			g_cnn_cb_flg = 1;
//		}
//	}
//	cnn_pt_clr_flg(cnn_id, FLGPTN_CNN_LLEND, g_cnn_cb_flg);
//
//#endif
//}

/**
CNN Wait Linked list Frame End

Wait for CNN linked list frame end flag.

@param[in] is_clr_flag
\n-@b TRUE: clear flag and wait for next flag.
\n-@b FALSE: wait for flag.

@return None.
*/
VOID cnn_wait_ll_frameend(BOOL cnn_id, BOOL is_clr_flag)
{
#if 1
	FLGPTN flag;

	if (is_clr_flag == TRUE) {
		if (cnn_id == 0) {
			cnn_pt_clr_flg(cnn_id, FLGPTN_CNN_LLEND, g_cnn_cb1_flg);
		}
		else {
			cnn_pt_clr_flg(cnn_id, FLGPTN_CNN_LLEND, g_cnn_cb2_flg);
		}
	}
	cnn_pt_wai_flg(cnn_id, &flag, FLGPTN_CNN_LLEND);


#endif
}

/**
CNN Set Parameters to Engine

Set CNN parameters to engine.

@param[in] Mode.
- CNN_OPMODE_CONV                       : Feature In + Conv + BnScale + ReLU + Pool.
- CNN_OPMODE_DECONV                     : Feature In + Deconv.
- CNN_OPMODE_SACLEUP                    : Feature In + Scaleup.
- CNN_OPMODE_USERDEFINE                 : User Define.
@param[in] p_parms CNN configuration.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER cnn_set_para2eng(BOOL cnn_id, CNN_OPMODE mode, CNN_PARM *p_parms)
{
	ER er_return = E_OK;
#if CNN_PRINT_PARM
	DBGD(p_parms->intype);
	DBGD(p_parms->elttype);
	DBGD(p_parms->out0type);
	DBGD(p_parms->out1type);
	DBGD(p_parms->insize.width);
	DBGD(p_parms->insize.height);
	DBGD(p_parms->insize.channel);
	DBGD(p_parms->insize.batch);
	//DBGD(p_parms->out0size.out_ofs);
	DBGH(p_parms->dmaio_addr.inaddr0);
	DBGH(p_parms->dmaio_addr.inaddr4);
	DBGH(p_parms->dmaio_addr.inaddr5);
	DBGH(p_parms->dmaio_addr.inaddr6);
	DBGH(p_parms->dmaio_addr.inaddr7);
	DBGH(p_parms->dmaio_addr.outaddr0);
	DBGH(p_parms->dmaio_addr.outaddr1);
	DBGD(p_parms->dmaio_lofs.in0_lofs);
	DBGD(p_parms->dmaio_lofs.in1_lofs);
	DBGD(p_parms->dmaio_lofs.in2_lofs);
	DBGD(p_parms->dmaio_lofs.in3_lofs);
	DBGD(p_parms->dmaio_lofs.out0_lofs);
	DBGD(p_parms->dmaio_lofs.out1_lofs);
	DBGD(p_parms->dmaio_lofs.out2_lofs);
	DBGD(p_parms->dmaio_lofs.out3_lofs);

	DBGD(p_parms->conv_parm.convkerl_parms.conv_kersize);
	DBGD(p_parms->conv_parm.convkerl_parms.conv_stride);
	DBGD(p_parms->conv_parm.convkerl_parms.conv_setnum);
	DBGD(p_parms->conv_parm.convkerl_parms.conv_shf_acc);
	DBGD(p_parms->conv_parm.convkerl_parms.is_top_pad);
	DBGD(p_parms->conv_parm.convkerl_parms.is_bot_pad);
	DBGD(p_parms->conv_parm.convkerl_parms.is_left_pad);
	DBGD(p_parms->conv_parm.convkerl_parms.is_right_pad);
	DBGD(p_parms->conv_parm.convkerl_parms.conv_shf_bias);

	DBGD(p_parms->bnscale_parm.bn_shf_mean);
	DBGD(p_parms->bnscale_parm.scale_shf_bias);
	DBGD(p_parms->bnscale_parm.scale_shf_alpha);

	DBGD(p_parms->elt_parm.elt_in_shift_dir);
	DBGD(p_parms->elt_parm.elt_in_shift);
	DBGD(p_parms->elt_parm.elt_in_scale);
	DBGD(p_parms->elt_parm.elt_shf0);
	DBGD(p_parms->elt_parm.elt_shf1);
	DBGD(p_parms->elt_parm.elt_outshf);
	DBGD(p_parms->elt_parm.elt_coef0);
	DBGD(p_parms->elt_parm.elt_coef1);

	DBGD(p_parms->relu_parm.pre_relu.leaky_val);
	DBGD(p_parms->relu_parm.pre_relu.leaky_shf);
	DBGD(p_parms->relu_parm.pre_relu.negation);
	DBGD(p_parms->relu_parm.relu0.leaky_val);
	DBGD(p_parms->relu_parm.relu0.leaky_shf);
	DBGD(p_parms->relu_parm.relu0.negation);
	DBGD(p_parms->relu_parm.relu1.leaky_val);
	DBGD(p_parms->relu_parm.relu1.leaky_shf);
	DBGD(p_parms->relu_parm.relu1.negation);

	DBGD(p_parms->pool_parm.globalpool_parm.ker_type);
	DBGD(p_parms->pool_parm.globalpool_parm.avg_mul);
	DBGD(p_parms->pool_parm.globalpool_parm.avg_shf);

	DBGD(p_parms->pool_parm.local_pool.ker_type);
	DBGD(p_parms->pool_parm.local_pool.out_cal_type);
	DBGD(p_parms->pool_parm.local_pool.stride);
	DBGD(p_parms->pool_parm.local_pool.ker_size);
	DBGD(p_parms->pool_parm.local_pool.is_top_pad);
	DBGD(p_parms->pool_parm.local_pool.is_bot_pad);
	DBGD(p_parms->pool_parm.local_pool.is_left_pad);
	DBGD(p_parms->pool_parm.local_pool.is_right_pad);

	DBGD(p_parms->pool_parm.out_shf_dir);
	DBGD(p_parms->pool_parm.out_shf);
	DBGD(p_parms->pool_parm.out_scale);
	DBGD(p_parms->pool_parm.ave_div_type);

	DBGD(p_parms->deconv_parm.is_top_pad);
	DBGD(p_parms->deconv_parm.is_bot_pad);
	DBGD(p_parms->deconv_parm.is_left_pad);
	DBGD(p_parms->deconv_parm.is_right_pad);
	DBGD(p_parms->deconv_parm.deconv_padval);
	DBGD(p_parms->deconv_parm.deconv_stride);

	DBGD(p_parms->scaleup_parm.is_top_pad);
	DBGD(p_parms->scaleup_parm.is_bot_pad);
	DBGD(p_parms->scaleup_parm.is_left_pad);
	DBGD(p_parms->scaleup_parm.is_right_pad);
	DBGD(p_parms->scaleup_parm.scaleup_padval);
	DBGD(p_parms->scaleup_parm.scaleup_rate);

	DBGD(p_parms->out_scale_parm.conv_shf_dir);
	DBGD(p_parms->out_scale_parm.conv_shf);
	DBGD(p_parms->out_scale_parm.conv_scale);
	DBGD(p_parms->out_scale_parm.elt_shf_dir);
	DBGD(p_parms->out_scale_parm.elt_shf);
	DBGD(p_parms->out_scale_parm.elt_scale);
	DBGD(p_parms->out_scale_parm.out0_shf_dir);
	DBGD(p_parms->out_scale_parm.out0_shf);
	DBGD(p_parms->out_scale_parm.out0_scale);
	DBGD(p_parms->out_scale_parm.out1_shf_dir);
	DBGD(p_parms->out_scale_parm.out1_shf);
	DBGD(p_parms->out_scale_parm.out1_scale);
	DBGD(p_parms->out_scale_parm.pool_shf_dir);
	DBGD(p_parms->out_scale_parm.pool_shf);
	DBGD(p_parms->out_scale_parm.pool_scale);

	DBGD(p_parms->clamp_parm.clamp_th[0]);
	DBGD(p_parms->clamp_parm.clamp_th[1]);
	DBGD(p_parms->clamp_parm.clamp_th[2]);
	DBGD(p_parms->clamp_parm.clamp_th[3]);
	DBGD(p_parms->clamp_parm.clamp_th[4]);
	DBGD(p_parms->clamp_parm.clamp_th[5]);

#endif

	if (p_parms == NULL) {
		DBG_ERR( "null input...\r\n");
		return E_NOEXS;
	}

	switch (mode) {
	case CNN_OPMODE_CONV:
		er_return = cnn_set_engmode(cnn_id, CNN_CONV);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_engmode fail...\r\n");
			return er_return;
		}
		cnn_set_io_enable(cnn_id, DISABLE, CNN_ALL_IO_EN);
		cnn_set_io_enable(cnn_id, ENABLE, p_parms->userdef.io_enable);
		cnn_set_kerl_en(cnn_id, DISABLE, CNN_ALL_KERL_EN);
		cnn_set_kerl_en(cnn_id, ENABLE, p_parms->userdef.func_en);
		er_return = cnn_set_intype(cnn_id, p_parms->intype);

		if (er_return != E_OK) {
			DBG_ERR("cnn_set_intype fail...\r\n");
			return er_return;
		}
		cnn_set_eltwise_intype(cnn_id, p_parms->elttype);
		er_return = cnn_set_out0type(cnn_id, p_parms->out0type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0type fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out1type(cnn_id, p_parms->out1type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out1type fail...\r\n");
			return er_return;
		}

		er_return = cnn_set_eltmode(cnn_id, p_parms->userdef.elt_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_eltmode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_actmode(cnn_id, p_parms->userdef.act_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_actmode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_lutmode(cnn_id, p_parms->userdef.lut_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_lutmode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_poolmode(cnn_id, p_parms->userdef.pool_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_poolmode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out0mode(cnn_id, p_parms->userdef.out0_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0mode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_dma_lofs(cnn_id, &p_parms->dmaio_lofs);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dma_lofs fail...\r\n");
			return er_return;
		}

		//Conv mode
		er_return = cnn_set_convparm(cnn_id, &p_parms->conv_parm.convkerl_parms);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_convparm fail...\r\n");
			return er_return;
		}

		//set fcd parms
		er_return = cnn_set_fcd_parm(cnn_id, &p_parms->conv_parm.fcd_parm);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_fcd_parm fail...\r\n");
			return er_return;
		}

		//BnScale mode
		er_return = cnn_set_bnscale_parm(cnn_id, &p_parms->bnscale_parm);
		if (er_return != E_OK) {
			DBG_ERR( "cnn_set_bnscale_parm fail...\r\n");
			return er_return;
		}

		//Eltwise mode
		er_return = cnn_set_elt_parm(cnn_id, &p_parms->elt_parm);
		if (er_return != E_OK) {
			DBG_ERR( "cnn_set_elt_parm fail...\r\n");
			return er_return;
		}

		//PreReLU mode
		er_return = cnn_set_prerelu_parm(cnn_id, &p_parms->relu_parm.pre_relu);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_prerelu_parm fail...\r\n");
			return er_return;
		}
		//ReLU0 mode
		er_return = cnn_set_relu0_parm(cnn_id, &p_parms->relu_parm.relu0);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_relu0_parm fail...\r\n");
			return er_return;
		}
		//ReLU1 mode
		er_return = cnn_set_relu1_parm(cnn_id, &p_parms->relu_parm.relu1);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_relu1_parm fail...\r\n");
			return er_return;
		}

		//Pooling mode
		if (cnn_get_poolmode(cnn_id) == CNN_POOLING_LOCAL) {
			er_return = cnn_set_localpool_parm(cnn_id, &p_parms->local_pool);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_localpool_parm fail...\r\n");
				return er_return;
			}
		} else {
			er_return = cnn_set_globalpool_parm(cnn_id, &p_parms->global_pool);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_globalpool_parm fail...\r\n");
				return er_return;
			}
		}
		break;

	case CNN_OPMODE_DECONV:
		er_return = cnn_set_engmode(cnn_id, CNN_DECONV);                            ///< enable deconvolution
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_engmode fail...\r\n");
			return er_return;
		}
		cnn_set_io_enable(cnn_id, ENABLE, CNN_OUT0_EN);                 ///< output 0 enable
		cnn_set_io_enable(cnn_id, DISABLE, CNN_OUT1_EN);                ///< output 1 disable
		er_return = cnn_set_intype(cnn_id, p_parms->intype);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_intype fail...\r\n");
			return er_return;
		}
		cnn_set_eltwise_intype(cnn_id, p_parms->elttype);
		er_return = cnn_set_out0type(cnn_id, p_parms->out0type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0type fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out1type(cnn_id, p_parms->out1type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out1type fail...\r\n");
			return er_return;
		}

		er_return = cnn_set_out0mode(cnn_id, CNN_OUT0_RELU0);                      ///< output deconv
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0mode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_deconvparm(cnn_id, &p_parms->deconv_parm);  ///< set deconv params
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_deconvparm fail...\r\n");
			return er_return;
		}
		break;

	case CNN_OPMODE_SACLEUP:
		er_return = cnn_set_engmode(cnn_id, CNN_SCALEUP);                            ///< enable scale up
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_engmode fail...\r\n");
			return er_return;
		}
		cnn_set_io_enable(cnn_id, ENABLE, CNN_OUT0_EN);                 ///< output 0 enable
		cnn_set_io_enable(cnn_id, DISABLE, CNN_OUT1_EN);                ///< output 1 disable
		er_return = cnn_set_intype(cnn_id, p_parms->intype);

		if (er_return != E_OK) {
			DBG_ERR("cnn_set_intype fail...\r\n");
			return er_return;
		}
		cnn_set_eltwise_intype(cnn_id, p_parms->elttype);
		er_return = cnn_set_out0type(cnn_id, p_parms->out0type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0type fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out1type(cnn_id, p_parms->out1type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out1type fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out0mode(cnn_id, CNN_OUT0_RELU0);                      ///< output scale
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0mode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_scaleupparm(cnn_id, &p_parms->scaleup_parm);  ///< set scale params
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_scaleupparm fail...\r\n");
			return er_return;
		}
		break;

	case CNN_OPMODE_LRN:
		er_return = cnn_set_engmode(cnn_id, CNN_LRN);                            ///< enable scale up
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_engmode fail...\r\n");
			return er_return;
		}
		cnn_set_io_enable(cnn_id, ENABLE, CNN_OUT0_EN);                 ///< output 0 enable
		cnn_set_io_enable(cnn_id, DISABLE, CNN_OUT1_EN);                ///< output 1 disable
		er_return = cnn_set_intype(cnn_id, p_parms->intype);

		if (er_return != E_OK) {
			DBG_ERR("cnn_set_intype fail...\r\n");
			return er_return;
		}
		cnn_set_eltwise_intype(cnn_id, p_parms->elttype);
		er_return = cnn_set_out0type(cnn_id, p_parms->out0type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0type fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out1type(cnn_id, p_parms->out1type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out1type fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out0mode(cnn_id, CNN_OUT0_RELU0);                      ///< output scale
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0mode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_lrnparm(cnn_id, &p_parms->lrn_parm);		///< set lrn params
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_lrnparm fail...\r\n");
			return er_return;
		}
		break;

	case CNN_OPMODE_USERDEFINE:

		er_return = cnn_set_engmode(cnn_id, p_parms->userdef.eng_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_engmode fail...\r\n");
			return er_return;
		}
		cnn_set_io_enable(cnn_id, DISABLE, CNN_ALL_IO_EN);
		cnn_set_io_enable(cnn_id, ENABLE, p_parms->userdef.io_enable);
		cnn_set_kerl_en(cnn_id, DISABLE, CNN_ALL_KERL_EN);
		cnn_set_kerl_en(cnn_id, ENABLE, p_parms->userdef.func_en);

		er_return = cnn_set_intype(cnn_id, p_parms->intype);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_intype fail...\r\n");
			return er_return;
		}
		cnn_set_eltwise_intype(cnn_id, p_parms->elttype);
		er_return = cnn_set_out0type(cnn_id, p_parms->out0type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0type fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out1type(cnn_id, p_parms->out1type);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out1type fail...\r\n");
			return er_return;
		}

		er_return = cnn_set_eltmode(cnn_id, p_parms->userdef.elt_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_eltmode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_actmode(cnn_id, p_parms->userdef.act_mode);

		if (er_return != E_OK) {
			DBG_ERR("cnn_set_actmode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_lutmode(cnn_id, p_parms->userdef.lut_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_lutmode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_poolmode(cnn_id, p_parms->userdef.pool_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_poolmode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_out0mode(cnn_id, p_parms->userdef.out0_mode);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_out0mode fail...\r\n");
			return er_return;
		}
		er_return = cnn_set_dma_lofs(cnn_id, &p_parms->dmaio_lofs);
		if (er_return != E_OK) {
			DBG_ERR("cnn_set_dma_lofs fail...\r\n");
			return er_return;
		}

		if (cnn_get_engmode(cnn_id) == CNN_CONV) {
			//Conv mode
			er_return = cnn_set_convparm(cnn_id, &p_parms->conv_parm.convkerl_parms);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_convparm fail...\r\n");
				return er_return;
			}

			//set fcd parms
			er_return = cnn_set_fcd_parm(cnn_id, &p_parms->conv_parm.fcd_parm);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_fcd_parm fail...\r\n");
				return er_return;	
			}

			//BnScale mode
			er_return = cnn_set_bnscale_parm(cnn_id, &p_parms->bnscale_parm);
			if (er_return != E_OK) {
				DBG_ERR( "cnn_set_bnscale_parm fail...\r\n");
				return er_return;
			}

			//Eltwise mode
			er_return = cnn_set_elt_parm(cnn_id, &p_parms->elt_parm);
			if (er_return != E_OK) {
				DBG_ERR( "cnn_set_elt_parm fail...\r\n");
				return er_return;
			}

			//PreReLU mode
			er_return = cnn_set_prerelu_parm(cnn_id, &p_parms->relu_parm.pre_relu);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_prerelu_parm fail...\r\n");
				return er_return;
			}
			//ReLU0 mode
			er_return = cnn_set_relu0_parm(cnn_id, &p_parms->relu_parm.relu0);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_relu0_parm fail...\r\n");
				return er_return;
			}
			//ReLU1 mode
			er_return = cnn_set_relu1_parm(cnn_id, &p_parms->relu_parm.relu1);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_relu1_parm fail...\r\n");
				return er_return;
			}

			//Pooling mode
			er_return = cnn_set_poolmode(cnn_id, p_parms->userdef.pool_mode);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_poolmode fail...\r\n");
				return er_return;
			}
			if (cnn_get_poolmode(cnn_id) == CNN_POOLING_LOCAL) {
				er_return = cnn_set_localpool_parm(cnn_id, &p_parms->local_pool);
			} else {
				er_return = cnn_set_globalpool_parm(cnn_id, &p_parms->global_pool);
			}
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_pool_parm fail...\r\n");
				return er_return;
			}
		}
		else if (cnn_get_engmode(cnn_id) == CNN_DECONV) {
			//Deconv mode
			er_return = cnn_set_deconvparm(cnn_id, &p_parms->deconv_parm);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_deconvparm fail...\r\n");
				return er_return;
			}
		}
		else if (cnn_get_engmode(cnn_id) == CNN_SCALEUP) {
			//ScaleUp mode
			er_return = cnn_set_scaleupparm(cnn_id, &p_parms->scaleup_parm);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_scaleupparm fail...\r\n");
				return er_return;
			}
		}
		else if (cnn_get_engmode(cnn_id) == CNN_LRN) {
			//LRN mode
			er_return = cnn_set_lrnparm(cnn_id, &p_parms->lrn_parm);
			if (er_return != E_OK) {
				DBG_ERR("cnn_set_lrnparm fail...\r\n");
				return er_return;
			}
		}
		else {
			DBG_ERR( "Unknown operation mode %d!\r\n", mode);
			return er_return;
		}

		break;

	default:
		DBG_ERR( "Unknown operation mode %d!\r\n", mode);
		return er_return;
		break;
	}

	er_return = cnn_set_out0ofs(cnn_id, &p_parms->out_ofs);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_out0ofs fail...\r\n");
		return er_return;
	}
	er_return = cnn_set_insize(cnn_id, &p_parms->insize);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_insize fail...\r\n");
		return er_return;
	}
	er_return = cnn_set_dmaio_addr(cnn_id, p_parms->dmaio_addr);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_dmaio_addr fail...\r\n");
		return er_return;
	}
	er_return = cnn_set_out_scale(cnn_id, &p_parms->out_scale_parm);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_out_scale fail...\r\n");
		return er_return;
	}
	er_return = cnn_set_clamp(cnn_id, &p_parms->clamp_parm);
	if (er_return != E_OK) {
		DBG_ERR("cnn_set_clamp fail...\r\n");
		return er_return;
	}
	return er_return;
}

/**
CNN Set Mode

Set CNN engine mode.

@param[in] Mode.
- CNN_OPMODE_CONV                       : Feature In + Conv + BnScale + ReLU + Pool.
- CNN_OPMODE_DECONV                     : Feature In + Deconv.
- CNN_OPMODE_USERDEFINE                 : User Define.
@param[in] p_parms CNN configuration.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_setmode(BOOL cnn_id, CNN_OPMODE mode, CNN_PARM *p_parms)
{
	ER er_return;
	//DBG_ERR("cnn_setmode: cnn_chk_state_machine\n");
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2PAUSE, FALSE);
	if (er_return != E_OK) {
		return er_return;
	}

	if (p_parms == NULL) {
		DBG_ERR( "null input...\r\n");
		return E_NOEXS;
	}

	// Clear engine interrupt status
	cnn_enable_int(cnn_id, DISABLE, CNN_INTE_ALL);
	cnn_clr_intr_status(cnn_id, CNN_INT_ALL);

	er_return = cnn_set_para2eng(cnn_id, mode, p_parms);
	if (er_return != E_OK) {
		return er_return;
	}
	cnn_enable_int(cnn_id, ENABLE, CNN_INTE_APP_ALL);

#if PROF
	is_setmode = 1;
#endif
	er_return = cnn_pause(cnn_id);
	if (er_return != E_OK) {
		return er_return;
	}
#if PROF
	is_setmode = 0;
#endif

	//DBG_ERR("cnn_set_mode_ed: cnn_chk_state_machine\n");
	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2PAUSE, TRUE);

	return er_return;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
CNN Linked List Set Mode

Set linked list CNN engine mode.

@param[in] ll_addr CNN linked list address.

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_ll_setmode(BOOL cnn_id, CNN_LL_PRM *ll_prm)
{
	ER er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2PAUSE, FALSE);

	if (er_return != E_OK) {
		return er_return;
	}

	// Clear engine interrupt status
	cnn_enable_int(cnn_id, DISABLE, CNN_INTE_ALL);
	cnn_clr_intr_status(cnn_id, CNN_INT_ALL);

	cnn_set_dmain_lladdr(cnn_id, ll_prm->addrin_ll);
	cnn_set_dmain_lladdr_base(cnn_id, ll_prm->addrin_ll_base);
	cnn_enable_int(cnn_id, ENABLE, CNN_INTE_LL_ALL);

#if PROF
	is_setmode = 1;
#endif
	er_return = cnn_ll_pause(cnn_id);
	if (er_return != E_OK) {
		return er_return;
	}
#if PROF
	is_setmode = 0;
#endif

	er_return = cnn_chk_state_machine(cnn_id, CNN_OP_SET2PAUSE, TRUE);
	return er_return;
}

/**
CNN Create Resource
*/
#if defined __FREERTOS
void cnn_create_resource(BOOL cnn_id, UINT32 clk_freq)
{
	cnn_pt_create_resource(cnn_id, clk_freq);
}
#else
VOID cnn_create_resource(BOOL cnn_id, VOID *parm, UINT32 clk_freq)
{
	cnn_pt_create_resource(cnn_id, parm, clk_freq);
}
#endif

/**
CNN Release Resource
*/
VOID cnn_release_resource(BOOL cnn_id)
{
	cnn_pt_release_resource(cnn_id);
}

/**
CNN Set Base Address

Set CNN engine registers address.

@param[in] addr registers address.

@return none.
*/
VOID cnn_set_base_addr(BOOL cnn_id, UINT32 addr)
{
	if (cnn_id == 0) {
		cnn_reg_base_addr1 = addr;
	} else {
		cnn_reg_base_addr2 = addr;
	}
}

/**
CNN Get Base Address

Get CNN engine registers address.

@return registers address.
*/
UINT32 cnn_get_base_addr(BOOL cnn_id)
{
	if (cnn_id == 0) {
		return cnn_reg_base_addr1;
	} else {
		return cnn_reg_base_addr2;
	}
}

UINT32 cnn_init(BOOL cnn_id)
{
	UINT32 clk_freq_mhz = 0;

	cnn_wait_dma_abort(cnn_id);

	cnn_lib_lock(cnn_id);
	if (g_cnn_enable_cnt[cnn_id] == 0) {
		if (cnn_id == 0) {
			clk_freq_mhz = cnn_get_clock_rate(0);
			if (clk_freq_mhz == 0) {
				cnn_set_clock_rate(0, 600);
			}
			else {
				cnn_set_clock_rate(0, clk_freq_mhz);
			}
			cnn_pt_clk_enable(0);
			nvt_disable_sram_shutdown(CNN_SD);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
			dma_set_channel_outstanding(DMA_CH_OUTSTANDING_CNN_ALL, 1);
#endif
		} else {
			clk_freq_mhz = cnn_get_clock_rate(1);
			if (clk_freq_mhz == 0) {
				cnn_set_clock_rate(1, 600);
			}
			else {
				cnn_set_clock_rate(1, clk_freq_mhz);
			}
			cnn_pt_clk_enable(1);
			nvt_disable_sram_shutdown(CNN2_SD);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
			dma_set_channel_outstanding(DMA_CH_OUTSTANDING_CNN2_ALL, 1);
#endif
		}
	}
	g_cnn_enable_cnt[cnn_id]++;
	cnn_lib_unlock(cnn_id);

	return 0;
}

UINT32 cnn_uninit(BOOL cnn_id)
{
	cnn_lib_lock(cnn_id);
	g_cnn_enable_cnt[cnn_id]--;
	if (g_cnn_enable_cnt[cnn_id] == 0) {
		if (cnn_id == 0) {
			cnn_pt_clk_disable(0);
			nvt_enable_sram_shutdown(CNN_SD);
		} else {
			cnn_pt_clk_disable(1);
			nvt_enable_sram_shutdown(CNN2_SD);
		}
	}
	cnn_lib_unlock(cnn_id);

	return 0;
}

BOOL cnn_eng_valid(UINT32 cnn_id) 
{
	BOOL is_valid = FALSE;
	
	if (cnn_id == 0) {
		is_valid = TRUE;
	} else if (cnn_id == 1) {
		if (efuse_check_available(__xstring(__section_name__)) != TRUE) {
			DBG_ERR("not support CNN2!\r\n");
			is_valid = FALSE;
		} else {
			is_valid = TRUE;
		}
	} else {
		DBG_ERR("engine id is invalid!\r\n");
	}

	return is_valid;
}
VOID cnn_reset_status(VOID)
{
	ER er_return;

	if (cnn_lock_status1 != NO_TASK_LOCKED) {
		er_return = cnn_close(0);
		if (er_return != E_OK) {
			DBG_ERR("cnn_reset_status: Error at cnn_close().\r\n");
            goto err_exit;
		}
	}
	if (cnn_lock_status2 != NO_TASK_LOCKED) {
		er_return = cnn_close(1);
		if (er_return != E_OK) {
            DBG_ERR("cnn_reset_status: Error at cnn_close().\r\n");
            goto err_exit;
        }
	}
	
	cnn_status1 = CNN_ENGINE_IDLE;
	cnn_status2 = CNN_ENGINE_IDLE;

err_exit:

	return;
}

VOID cnn_dma_abort(UINT32 cnn_id)
{
	BOOL is_dma_disable;

    is_dma_disable = cnn_get_dma_disable(cnn_id);
    if (is_dma_disable == 1) {
        DBG_IND("CNN_DMA_ABORT: it has been in dma_disable state.\r\n");
        return;
    }
	cnn_set_dma_disable(cnn_id, 1);

	return;
}

static VOID cnn_wait_dma_abort(BOOL cnn_id)
{
    BOOL is_dma_disable;

    is_dma_disable = cnn_get_dma_disable(cnn_id);
    while (is_dma_disable == 1) {
        vos_task_delay_ms(g_cnn_dma_abort_delay);
        is_dma_disable = cnn_get_dma_disable(cnn_id);
    }

    return;
}

EXPORT_SYMBOL(cnn_init);
EXPORT_SYMBOL(cnn_uninit);
EXPORT_SYMBOL(cnn_isr);
EXPORT_SYMBOL(cnn_set_clock_rate);
EXPORT_SYMBOL(cnn_get_clock_rate);
EXPORT_SYMBOL(cnn_chk_state_machine);
EXPORT_SYMBOL(cnn_change_interrupt);
EXPORT_SYMBOL(cnn_lock);
EXPORT_SYMBOL(cnn_unlock);

EXPORT_SYMBOL(cnn_open);
EXPORT_SYMBOL(cnn_is_opened);
EXPORT_SYMBOL(cnn_close);
EXPORT_SYMBOL(cnn_ll_start);
EXPORT_SYMBOL(cnn_ll_pause);
EXPORT_SYMBOL(cnn_ll_isr_start);
EXPORT_SYMBOL(cnn_reset);
EXPORT_SYMBOL(cnn_clr_all);
//EXPORT_SYMBOL(cnn_clr_frameend);
EXPORT_SYMBOL(cnn_wait_frameend);
//EXPORT_SYMBOL(cnn_clr_ll_frameend);
EXPORT_SYMBOL(cnn_wait_ll_frameend);
EXPORT_SYMBOL(cnn_ll_setmode);
EXPORT_SYMBOL(cnn_create_resource);
EXPORT_SYMBOL(cnn_release_resource);
EXPORT_SYMBOL(cnn_set_base_addr);
EXPORT_SYMBOL(cnn_get_base_addr);
EXPORT_SYMBOL(cnn_reset_status);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
EXPORT_SYMBOL(cnn_pause);
EXPORT_SYMBOL(cnn_start);
EXPORT_SYMBOL(cnn_isr_start);
EXPORT_SYMBOL(cnn_set_para2eng);
EXPORT_SYMBOL(cnn_setmode);
#endif
EXPORT_SYMBOL(cnn_dma_abort);

//@}
