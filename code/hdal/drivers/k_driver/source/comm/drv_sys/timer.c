/*
    Timer module driver

    This file is the driver of timer module

    @file       timer.c
    @ingroup    miDrvTimer_Timer
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/****************************************************************************/
/*                                                                          */
/*  Todo: Modify register accessing code to the macro in RCWMacro.h         */
/*                                                                          */
/****************************************************************************/

//#include "interrupt.h"
#include "pll_protected.h"
#include "cpu_protected.h"
#include "timer_int.h"
#include "timer_reg.h"
#if defined __FREERTOS
#include "top.h"
#else
#include <plat/top.h>
#endif
//#include "ist.h"

#if defined __FREERTOS
REGDEF_SETIOBASE(IOADDR_TIMER_REG_BASE);
#endif
REGDEF_SETGRPOFS(0x10);

int timer_isr(void);
void timer_dump_owner(void);

#if defined(_NVT_FPGA_)
static BOOL bTimerChangeInterval = TRUE;
#endif

// Timer lock status, its value is bit field.
static UINT32   TimerLockStatus = NO_TASK_LOCKED;

// Timer current locked numbers
static UINT32   uiTimerLockedNums = 0;

// Timer start/stop flow, find next free timer ID
static TIMER_ID uiTimerPrevID = TIMER_FIRST_ID;

// Timer interrupt callback function address table
// GNU extension
// Please refer to http://gcc.gnu.org/onlinedocs/gcc-4.4.7/gcc/Designated-Inits.html
static DRV_CB   pfTimerCB[TIMER_NUM] = { [0 ...(TIMER_NUM - 1)] = NULL };

// Timer auto close
static BOOL     bTimerAutoClose[TIMER_NUM] = { [0 ...(TIMER_NUM - 1)] = FALSE };

// Timer state [CLOSE, OPEN, PLAY]
static TIMER_DRV_STATE  vTimerState[TIMER_NUM] = { [0 ...(TIMER_NUM - 1)] = TIMER_DRV_STATE_CLOSE };

#if (TIMER_LOG_OWNER == ENABLE)
// For debug purpose, log owner task ID
static ID   vTimerOwner[TIMER_NUM] = { 0 };
#endif

static BOOL bTimerCBInIsr[TIMER_NUM] = { [0 ...(TIMER_NUM - 1)] = FALSE };

// Internal APIs
static ER       timer_lock(PTIMER_ID pTimerID);
static ER       timer_lockSpecific(TIMER_ID TimerID);
static ER       timer_unlock(TIMER_ID TimerID);
static void     timer_clrINTStatus(TIMER_ID TimerID);
static UINT32   timer_getINTStatus(TIMER_ID TimerID);

#if (TIMER_LOG_OWNER == ENABLE)
static unsigned int get_tid(ID *p_tskid)
{
	*p_tskid = vos_task_get_tid();
	return 0;
}
#endif

/*
    Timer ISR.

    Timer interrupt service routine.

    @return void
*/
int timer_isr(void)
{
	RCW_DEF(TIMER_STS0_REG);
	RCW_DEF(TIMER_INT0_REG);
	UINT32              uiTimerID;
	FLGPTN              Flag;
#if (DRV_SUPPORT_IST == ENABLE)
	UINT32              uiEvent;
#endif

	// Get interrupt status
	RCW_LD(TIMER_STS0_REG);
	RCW_LD(TIMER_INT0_REG);
	RCW_VAL(TIMER_STS0_REG) &= RCW_VAL(TIMER_INT0_REG);

#ifdef CONFIG_NOVATEK_TIMER
	if (RCW_VAL(TIMER_STS0_REG) & 0xFFFF) { // FIXME
		RCW_VAL(TIMER_STS0_REG) &= 0xFFFF;
#else
	if (RCW_VAL(TIMER_STS0_REG) != 0) {
#endif
		// Clear interrupt status
		RCW_ST(TIMER_STS0_REG);

		Flag        = 0;
#if (DRV_SUPPORT_IST == ENABLE)
		uiEvent     = 0;
#endif
		while (RCW_VAL(TIMER_STS0_REG)) {
			// GNU built-in function
			// Please refer to http://gcc.gnu.org/onlinedocs/gcc-4.4.7/gcc/Other-Builtins.html#Other-Builtins
			// __builtin_ctz(x) will return the number of trailing 0-bits in x.
			uiTimerID = __builtin_ctz(RCW_VAL(TIMER_STS0_REG));

			// Auto close?
			if (bTimerAutoClose[uiTimerID] == TRUE) {
				timer_close(uiTimerID);
			}

			// Signal event flag
			Flag |= (FLGPTN_TIMER0 << uiTimerID);

#if (DRV_SUPPORT_IST == ENABLE)
			if (pf_ist_cbs_timer[uiTimerID] != NULL) {
				if (bTimerCBInIsr[uiTimerID]) {
					(pf_ist_cbs_timer[uiTimerID])(0);
				} else {
					uiEvent |= (1 << uiTimerID);
				}
			} else
#endif
				// Call the event handler
				if (pfTimerCB[uiTimerID] != NULL) {
					UINT32 uiStartT, uiEndT;

					uiStartT = timer_get_current_count(TIMER_SYSTIMER_ID);
					(pfTimerCB[uiTimerID])(0);
					uiEndT = timer_get_current_count(TIMER_SYSTIMER_ID);
					if ((uiEndT - uiStartT) > 200) {
						DBG_ERR("timer %d ISR callback elps %d us\r\n", uiTimerID, uiEndT-uiStartT);
					}
				}

			RCW_VAL(TIMER_STS0_REG) &= ~(1 << uiTimerID);
		}

		// Signal event flag
		if (Flag != 0) {
			timer_platform_flg_set(Flag);
		}

#if (DRV_SUPPORT_IST == ENABLE)
		if (uiEvent != 0) {
			timer_platform_set_ist_event(uiEvent);
		}
#endif
	}
	else
		return IRQ_NONE;
	return IRQ_HANDLED;
}

/*
    Acquire timer ID.

    One of N timers will be allocated and the timer ID will be returned.
    Total N timers are available.

    @param[out] pTimerID    Return acquired timer ID

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_SYS  : No available timer
*/
static ER timer_lock(PTIMER_ID pTimerID)
{
	UINT32 i;
	UINT32 spin_flags;

	// There is no more availble timer
	// "Last - first + 1" is the number of available ID, + 2 is for system timer
	if (uiTimerLockedNums == (TIMER_LAST_ID - TIMER_FIRST_ID + 2)) {
#if (TIMER_LOG_OWNER == ENABLE)
		timer_dump_owner();
#endif
		*pTimerID = TIMER_INVALID;
		return E_SYS;
	}

	timer_platform_sem_wait();

	spin_flags = timer_platform_spin_lock();

	// Find unlocked ID
	// Assign Timer ID sequence is TIMER_1 -> ...-> TIMER_X -> TIMER_1 -> ...
	// (TIMER_0 is reserved for system timer)
	// Note: The algorithm might get the ID that are just released
	i = uiTimerPrevID;
	while (1) {
		// Advance timer ID
		if (i == TIMER_LAST_ID) {
			i = TIMER_FIRST_ID;
		} else {
			i++;
		}

		// Timer ID is unlocked
		if ((TimerLockStatus & (TASK_LOCKED << i)) == NO_TASK_LOCKED) {
			*pTimerID       = i;
			uiTimerPrevID   = i;
			break;
		}
	}

	// Let the task own the TIMER
	TimerLockStatus |= (TASK_LOCKED << *pTimerID);
	vTimerState[*pTimerID] = TIMER_DRV_STATE_OPEN;

#if (TIMER_LOG_OWNER == ENABLE)
	get_tid(&vTimerOwner[*pTimerID]);
#endif

	uiTimerLockedNums++;

	timer_platform_spin_unlock(spin_flags);

	return E_OK;
}

/*
    Acquire specific timer ID

    Try to allocate specific timer ID.

    @param[in] TimerID      Timer ID (TIMER_x)

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_SYS  : timer is already locked
*/
static ER timer_lockSpecific(TIMER_ID TimerID)
{
	UINT32 spin_flags;

	if (TimerLockStatus & (TASK_LOCKED << TimerID)) {
		return E_SYS;
	}

	timer_platform_sem_wait();

	spin_flags = timer_platform_spin_lock();

	// Let the task own the TIMER
	TimerLockStatus |= (TASK_LOCKED << TimerID);
	vTimerState[TimerID] = TIMER_DRV_STATE_OPEN;

#if (TIMER_LOG_OWNER == ENABLE)
	get_tid(&vTimerOwner[TimerID]);
#endif
	uiTimerLockedNums++;

	timer_platform_spin_unlock(spin_flags);

	return E_OK;
}

/*
    Release timer ID

    Release timer ID.

    @param[in] TimerID      Timer ID (TIMER_x) to be released

    @return Operation status
        - @b E_OK   : Everything is OK
*/
static ER timer_unlock(TIMER_ID TimerID)
{
	UINT32 spin_flags;

	spin_flags = timer_platform_spin_lock();

	TimerLockStatus &= (~(TASK_LOCKED << TimerID));
	vTimerState[TimerID] = TIMER_DRV_STATE_CLOSE;
	uiTimerLockedNums--;

	timer_platform_spin_unlock(spin_flags);

	// Release TIMER semaphore
	return timer_platform_sem_signal();
}

/*
    Clear timer interrupt status

    Clear timer interrupt status of specific timer ID.

    @param[in] TIMER_ID     The timer ID whose interrupt status you would like to clear.

    @return void

    @note   This is somewhat implementation dependent and must be used with care.
*/
static void timer_clrINTStatus(TIMER_ID TimerID)
{
	RCW_DEF(TIMER_STS0_REG);

	RCW_VAL(TIMER_STS0_REG) = (TIMER_REG_STS << TimerID);
	RCW_ST(TIMER_STS0_REG);
}

/*
    Get timer interrupt status

    Get timer interrupt status of specific timer ID.

    @param[in] TIMER_ID     The timer ID whose interrupt status you would like to get.

    @return Interrupt status
        - @b 0      : No interrupt is occured
        - @b Others : Interrupt is pending

    @note   This is somewhat implementation dependent and must be used with care.
*/
static UINT32 timer_getINTStatus(TIMER_ID TimerID)
{
	RCW_DEF(TIMER_STS0_REG);

	RCW_LD(TIMER_STS0_REG);

	return (RCW_VAL(TIMER_STS0_REG) & (TIMER_REG_STS << TimerID));
}

/*
    Set timer's parameter

    Set timer's parameter.

    @param[in] TimerID      The timer ID to be set.
    @param[in] uiIntval     Timer expired interval (1 ~ 0xFFFF_FFFF), unit: Timer clock
    @param[in] TimerMode    Timer operating mode.\n
            Set either one-shot (TIMER_MODE_ONE_SHOT) or free run mode (TIMER_MODE_FREE_RUN).\n
            Set clock source from divider 0 (TIMER_MODE_CLKSRC_DIV0) or divider 1 (TIMER_MODE_CLKSRC_DIV1).\n
            Additional TIMER_MODE_ENABLE_INT can be specified to enable interrupt.
    @param[in] TimerState   Set timer to play (TIMER_STATE_PLAY) or pause (TIMER_STATE_PAUSE).
*/
_INLINE void timer_setParameter(TIMER_ID TimerID, UINT32 uiIntval, TIMER_MODE TimerMode, TIMER_STATE TimerState)
{
	UINT32 spin_flags;
	RCW_DEF(TIMER_CTRLx_REG);
	RCW_DEF(TIMER_TARGETx_REG);
	RCW_DEF(TIMER_INT0_REG);
	RCW_DEF(TIMER_CLKDIV_REG);

#if defined(_NVT_FPGA_)
	if (bTimerChangeInterval == TRUE) {
		// In real chip, clock source is 480MHz / 160
		// In FPGA, clock source is (OSC * 2) / 160
		uiIntval /= (240000000 / _FPGA_PLL_OSC_);
//		DBG_DUMP("uiIntval = %d 0x%08x\r\n", (int)uiIntval, (int)_FPGA_PLL_OSC_);
		if (uiIntval == 0) {
			uiIntval = 1;
		}
	}
#endif
	// Must disable timer before setting
	RCW_LD2(TIMER_CTRLx_REG, TimerID);
	if (RCW_OF(TIMER_CTRLx_REG).Enable == TIMER_REG_EN_ENABLE) {
		// Disable timer
		RCW_OF(TIMER_CTRLx_REG).Enable = TIMER_REG_EN_DISABLE;
		RCW_ST2(TIMER_CTRLx_REG, TimerID);

		// Wait for timer is really disabled
		do {
			RCW_LD2(TIMER_CTRLx_REG, TimerID);
		} while (RCW_OF(TIMER_CTRLx_REG).Enable == TIMER_REG_EN_ENABLE);
	}

	// Clear interrupt status and flag
	timer_platform_flg_clear((FLGPTN_TIMER0 << TimerID));
	timer_clrINTStatus(TimerID);

	// Lock CPU to prevent multiple entry
	spin_flags = timer_platform_spin_lock();

	// Configure Interrupt Enable Register
	RCW_LD(TIMER_INT0_REG);
	if (TimerMode & TIMER_MODE_ENABLE_INT) {
		RCW_VAL(TIMER_INT0_REG) |= (TIMER_REG_INTEN_ENABLE << TimerID);
	} else {
		RCW_VAL(TIMER_INT0_REG) &= ~(TIMER_REG_INTEN_ENABLE << TimerID);
	}
	RCW_ST(TIMER_INT0_REG);

	// Configure Clock Divider Register
	RCW_LD(TIMER_CLKDIV_REG);

	if (TimerMode & TIMER_MODE_CLKSRC_DIV1) {
		// Clock source in Control Register
		RCW_OF(TIMER_CTRLx_REG).SrcClk = TIMER_REG_CLKSRC_DIV1;

		// Enable clock source divider 1
		RCW_OF(TIMER_CLKDIV_REG).Divider1En = 1;
	} else {
		// Clock source in Control Register
		RCW_OF(TIMER_CTRLx_REG).SrcClk = TIMER_REG_CLKSRC_DIV0;

		// Enable clock source divider 0
		RCW_OF(TIMER_CLKDIV_REG).Divider0En = 1;
	}
	RCW_ST(TIMER_CLKDIV_REG);

	// Configure Target Register
	RCW_VAL(TIMER_TARGETx_REG) = uiIntval;
	RCW_ST2(TIMER_TARGETx_REG, TimerID);

	// Configure Control Register
	// Mode
	// Free run
	if (TimerMode & TIMER_MODE_FREE_RUN) {
		RCW_OF(TIMER_CTRLx_REG).Mode = TIMER_REG_MODE_FREE_RUN;
	}
	// One shot
	else {
		RCW_OF(TIMER_CTRLx_REG).Mode = TIMER_REG_MODE_ONE_SHOT;
	}

	if (TimerMode & TIMER_MODE_CB_IN_ISR) {
		bTimerCBInIsr[TimerID] = 1;
	} else {
		bTimerCBInIsr[TimerID] = 0;
	}

	// State
	// Pause
	if (TimerState == TIMER_STATE_PAUSE) {
		RCW_OF(TIMER_CTRLx_REG).Enable  = TIMER_REG_EN_DISABLE;
		vTimerState[TimerID]            = TIMER_DRV_STATE_OPEN;
	}
	// Play
	else {
		RCW_OF(TIMER_CTRLx_REG).Enable  = TIMER_REG_EN_ENABLE;
		vTimerState[TimerID]            = TIMER_DRV_STATE_PLAY;
	}
	RCW_ST2(TIMER_CTRLx_REG, TimerID);

	// Wait for timer is really enabled
	if (TimerState == TIMER_STATE_PLAY) {
		do {
			RCW_LD2(TIMER_CTRLx_REG, TimerID);
		} while ((RCW_OF(TIMER_CTRLx_REG).Enable == TIMER_REG_EN_DISABLE) && (timer_getINTStatus(TimerID) == 0));
	}

	// Must unlock CPU after polling enable bit done, since caller might be
	// switched to ready queue after unlock CPU, and enable bit will be
	// auto-cleared in One-Shot mode.
	timer_platform_spin_unlock(spin_flags);

}

/**
    @addtogroup miDrvTimer_Timer
*/
//@{

/**
    @name Timer open/close APIs
*/
//@{
/**
    Open timer.

    One of 16 timers will be allocated and the timer ID will be returned.
    Total 16 timers are available.

    @param[out] pTimerID        Timer ID that is allocated
    @param[in] EventHandler     Timer expired callback function. Assign NULL if callback is not required.

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Parameters are not valid
        - @b E_SYS  : Maximum number of timers is exceeded
*/
ER timer_open(PTIMER_ID pTimerID, DRV_CB EventHandler)
{
	ER erReturn;

	// Accquire available timer ID
	erReturn = timer_lock(pTimerID);

	if (erReturn != E_OK) {
		return erReturn;
	}

#ifdef CONFIG_NOVATEK_TIMER
	if (nvttmr_base != NULL) {
		if (MAX_TIMER_NUM < *pTimerID) {
			DBG_ERR("Timer num not support\r\n");
			return erReturn;
		}
		if ((nvttmr_in_use & (1 << *pTimerID))) {
			DBG_ERR("Timer num %d already in use\r\n", *pTimerID);
			return erReturn;
		} else {
			nvttmr_in_use |= (1 << *pTimerID);
		}
	}
	else {
		DBG_ERR("nvttmr_base is NULL\r\n");
		return erReturn;
	}
#endif

	// Assign callback function
#if (DRV_SUPPORT_IST == ENABLE)
//	printk("%s: install callback 0x%p\r\n", __func__, EventHandler);
//	printk("%s: id %d, ptr 0x%p\r\n", __func__, *pTimerID,
//		 &pf_ist_cbs_timer[*pTimerID]);
	if (*pTimerID >= TIMER_NUM) {
		DBG_ERR("pTimerID(%d) >= TIMER_NUM(%d) \r\n", *pTimerID, TIMER_NUM);
		return erReturn;
	} else {
		pf_ist_cbs_timer[*pTimerID] = EventHandler;
	}
#else
	pfTimerCB[*pTimerID] = EventHandler;
#endif

	// Clear flag pattern
	timer_platform_flg_clear(FLGPTN_TIMER0 << *pTimerID);


	return E_OK;
}

/**
    Open timer and will auto close the timer when timer is expired.

    One of N timers will be allocated and the timer ID will be returned.
    Total N timers are available. The timer will be automatically closed
    when first interrupt is issue (always = one-shot mode). Please don't
    call timer_close() if you call this API.

    @param[out] pTimerID        Timer ID that is allocated
    @param[in] EventHandler     Timer expired callback function. Assign NULL if callback is not required.

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Parameters are not valid
        - @b E_SYS  : Maximum number of timers is exceeded
*/
ER timer_open_auto_close(PTIMER_ID pTimerID, DRV_CB EventHandler)
{
	ER erReturn;

	erReturn = timer_open(pTimerID, EventHandler);

	if (erReturn == E_OK) {
		bTimerAutoClose[*pTimerID] = TRUE;
	}
	return erReturn;
}

/*
    Open timer ISR.

    Same as timer_open() except this API will invoke callback in ISR instead of IST.

    @note Please DON'T use this API. Only LEADER PERMIITED application can use this API.

    @param[out] pTimerID        Timer ID that is allocated
    @param[in] EventHandler     Timer expired callback function. Assign NULL if callback is not required.

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Parameters are not valid
        - @b E_SYS  : Maximum number of timers is exceeded
*/
ER timer_open_isr(PTIMER_ID pTimerID, DRV_CB EventHandler)
{
	ER erReturn;

	// Accquire available timer ID
	erReturn = timer_lock(pTimerID);

	if (erReturn != E_OK) {
		return erReturn;
	}

	// Assign callback function
	pfTimerCB[*pTimerID] = EventHandler;

	// Clear flag pattern
	timer_platform_flg_clear(FLGPTN_TIMER0 << *pTimerID);

	return E_OK;
}

/**
    Close timer.

    The timer of specified timer ID will be closed.

    @param[in] TimerID      The timer ID to be closed

    @return Operation status
        - @b E_OK   : everything is OK
        - @b E_PAR  : Invalid timer ID
        - @b E_SYS  : Target timer is not opened
*/
ER timer_close(TIMER_ID TimerID)
{
	RCW_DEF(TIMER_CTRLx_REG);

	//If emulation == ON, do not check these limit or Timer 0 will not be released
#if (_EMULATION_ == DISABLE)
	// Check Timer ID, system timer is not allowed to be referenced by this API
	if ((TimerID > TIMER_LAST_ID) || (TimerID < TIMER_FIRST_ID)) {
		return E_PAR;
	}
#endif
	// Check timer is opened or not
	if ((TimerLockStatus & (TASK_LOCKED << TimerID)) == NO_TASK_LOCKED) {
		return E_SYS;
	}

	// Disable auto close
	bTimerAutoClose[TimerID] = FALSE;

	RCW_LD2(TIMER_CTRLx_REG, TimerID);
	RCW_OF(TIMER_CTRLx_REG).Enable = TIMER_REG_EN_DISABLE;
	RCW_ST2(TIMER_CTRLx_REG, TimerID);

	pf_ist_cbs_timer[TimerID] = NULL;
	pfTimerCB[TimerID] = NULL;

#ifdef CONFIG_NOVATEK_TIMER
	nvttmr_in_use &= ~(1 << TimerID);
#endif
	// Unlock timer X
	return timer_unlock(TimerID);
}

//@}

/**
    @name Timer configuration API
*/
//@{
/**
    Set timer parameter.

    Configure timer expired interval, clock source, operating mode and state.

    @param[in] TimerID      The timer ID to be configured.
    @param[in] uiIntval     Timer expired interval (1 ~ 0xFFFF_FFFF), unit: Timer clock
    @param[in] TimerMode    Timer operating mode.\n
            Set either one-shot (TIMER_MODE_ONE_SHOT) or free-run mode (TIMER_MODE_FREE_RUN).\n
            Set clock source from divider 0 (TIMER_MODE_CLKSRC_DIV0) or divider 1 (TIMER_MODE_CLKSRC_DIV1).\n
            Additional TIMER_MODE_ENABLE_INT can be specified to enable interrupt.
    @param[in] TimerState   Set timer to play (TIMER_STATE_PLAY) or pause (TIMER_STATE_PAUSE).

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid timer ID or interval
        - @b E_SYS  : Timer for the ID is not opened yet

    @note   If the timer is running, we will stop the timer first.
*/
ER timer_cfg(TIMER_ID TimerID, UINT32 uiIntval, TIMER_MODE TimerMode, TIMER_STATE TimerState)
{
#if (_EMULATION_ == DISABLE)
	// Check Timer ID and interval, system timer is not allowed to be referenced by this API
	if ((TimerID > TIMER_LAST_ID) || (TimerID < TIMER_FIRST_ID) || (uiIntval == 0)) {
		return E_PAR;
	}
#endif
	// Check timer is opened or not
	if ((TimerLockStatus & (TASK_LOCKED << TimerID)) == NO_TASK_LOCKED) {
		return E_SYS;
	}

	timer_setParameter(TimerID, uiIntval, TimerMode, TimerState);

	return E_OK;
}

/*
    Set timer target to specific core status

    Configure timer timeout target to specific core(CPU1/CPU2/DSP/DSP2)

    @param[in] TimerID      The timer ID to be configured.
    @param[in] CoreID       Specific core

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid timer ID or interval
        - @b E_SYS  : Timer for the ID is not opened yet

    @note   If the timer is running, we will stop the timer first.
*/
#if defined __FREERTOS
ER timer_cfg_core_target(TIMER_ID TimerID, CC_CORE_ID CoreID)
{
	UINT32 spin_flags;
	UINT32 uiTargetValue;

	if (nvt_get_chip_id() != CHIP_NA51055) {
		// Lock CPU to prevent multiple entry
		spin_flags = timer_platform_spin_lock();

		uiTargetValue = TIMER_GETREG(TIMER_DST0_REG_OFS + (CoreID << 2));

		uiTargetValue |= (1 << TimerID);

		TIMER_SETREG(TIMER_DST0_REG_OFS + (CoreID << 2), uiTargetValue);

		// unlock CPU to prevent multiple entry
		timer_platform_spin_unlock(spin_flags);
	}
	return E_OK;
}
#endif
/*
    Set timer target to specific core interrupt enable

    Configure timer timeout target to specific core(CPU1/CPU2/DSP/DSP2)

    @param[in] TimerID      The timer ID to be configured.
    @param[in] CoreID       Specific core

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid timer ID or interval
        - @b E_SYS  : Timer for the ID is not opened yet

    @note   If the timer is running, we will stop the timer first.
*/
#if defined __FREERTOS
ER timer_cfg_core_target_int_enable(TIMER_ID TimerID, CC_CORE_ID CoreID)
{
	UINT32 spin_flags;
	UINT32 uiTargetValue;

	if (nvt_get_chip_id() != CHIP_NA51055) {
		// Lock CPU to prevent multiple entry
		spin_flags = timer_platform_spin_lock();

		uiTargetValue = TIMER_GETREG(TIMER_INT0_REG_OFS + (CoreID << 2));

		uiTargetValue |= (1 << TimerID);

		TIMER_SETREG(TIMER_INT0_REG_OFS + (CoreID << 2), uiTargetValue);

		// unlock CPU to prevent multiple entry
		timer_platform_spin_unlock(spin_flags);
	}
	return E_OK;
}
#endif
//@}

/**
    @name Timer operation API
*/
//@{
/**
    Run / Stop timer

    Run / Stop timer.

    @param[in] TimerID      The timer ID which you would like to run / stop.
    @param[in] TimerState   Run or stop, must be one of the following:
        - @b TIMER_STATE_PLAY   : Run
        - @b TIMER_STATE_PAUSE  : stop

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid timer ID
        - @b E_SYS  : Timer for the ID is not opened yet
*/
ER timer_pause_play(TIMER_ID TimerID, TIMER_STATE TimerState)
{
	UINT32 spin_flags;
	RCW_DEF(TIMER_CTRLx_REG);

#if (_EMULATION_ == DISABLE)
	// Check Timer ID, system timer is not allowed to be referenced by this API
	if ((TimerID > TIMER_LAST_ID) || (TimerID < TIMER_FIRST_ID)) {
		return E_PAR;
	}
#endif
	// Check timer is opened or not
	if ((TimerLockStatus & (TASK_LOCKED << TimerID)) == NO_TASK_LOCKED) {
		return E_SYS;
	}

	if (TimerState == TIMER_STATE_PLAY) {
		// Open and not run
		if (vTimerState[TimerID] == TIMER_DRV_STATE_OPEN) {
			// Wait for timer is really disabled
			do {
				RCW_LD2(TIMER_CTRLx_REG, TimerID);
			} while (RCW_OF(TIMER_CTRLx_REG).Enable == TIMER_REG_EN_ENABLE);
		}

		// Clear interrupt status and flag
		timer_platform_flg_clear((FLGPTN_TIMER0 << TimerID));
		timer_clrINTStatus(TimerID);

		// Must lock CPU before enable bit is set and unlock CPU after polling
		// enable bit done, since caller might be switched to ready queue after
		// unlock CPU, and enable bit will be auto-cleared in One-Shot mode.
		// And polling enable bit will be a infinite loop.
		spin_flags = timer_platform_spin_lock();

		RCW_LD2(TIMER_CTRLx_REG, TimerID);
		RCW_OF(TIMER_CTRLx_REG).Enable  = TIMER_REG_EN_ENABLE;
		vTimerState[TimerID]            = TIMER_DRV_STATE_PLAY;
		RCW_ST2(TIMER_CTRLx_REG, TimerID);

		// Wait for timer is really enabled
		do {
			RCW_LD2(TIMER_CTRLx_REG, TimerID);
		} while (RCW_OF(TIMER_CTRLx_REG).Enable == TIMER_REG_EN_DISABLE);

		timer_platform_spin_unlock(spin_flags);
	} else {
		spin_flags = timer_platform_spin_lock();

		RCW_LD2(TIMER_CTRLx_REG, TimerID);
		RCW_OF(TIMER_CTRLx_REG).Enable  = TIMER_REG_EN_DISABLE;
		vTimerState[TimerID]            = TIMER_DRV_STATE_OPEN;
		RCW_ST2(TIMER_CTRLx_REG, TimerID);

		timer_platform_spin_unlock(spin_flags);
	}

	return E_OK;
}

/**
    Wait for timer expired

    Wait for timer expired. If interrupt is not enabled, CPU will polling until
    timer is expired. Otherwise, CPU will wait for interrupt issued.

    @param[in] TimerID      The timer ID whose time-up event you would like to wait.

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid timer ID
        - @b E_SYS  : Timer for the ID is not opened yet
*/
ER timer_wait_timeup(TIMER_ID TimerID)
{
	RCW_DEF(TIMER_INT0_REG);
//	FLGPTN      Flag;
#if (_EMULATION_ == DISABLE)
	// Check Timer ID, system timer is not allowed to be referenced by this API
	if ((TimerID > TIMER_LAST_ID) || (TimerID < TIMER_FIRST_ID)) {
		return E_PAR;
	}
#endif
	// Check timer is opened or not
	if ((TimerLockStatus & (TASK_LOCKED << TimerID)) == NO_TASK_LOCKED) {
		return E_SYS;
	}

	RCW_LD(TIMER_INT0_REG);

	// Interrupt is enabled, use event flag mechanism
	if (RCW_VAL(TIMER_INT0_REG) & (TIMER_REG_INTEN_ENABLE << TimerID)) {
		// Wait timer expire by waiting timer event flag issued by timer ISR
		timer_platform_flg_wait(FLGPTN_TIMER0 << TimerID);
	}
	// Interrupt is disabled, use polling instead
	else {
		while (timer_getINTStatus(TimerID) == 0);
		// Clear status
		timer_clrINTStatus(TimerID);
	}

	return E_OK;
}

/**
    Check if the timer is time up

    Check if the timer is time up and return to caller immediatelly.

    @param[in] TimerID      The timer ID whose status (time is up or not) you would like to check.
    @param[out] pbEn        Timer expired status
        - @b TRUE   : Timer expires already.
        - @b FALSE  : Timer doesn't expire.

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid timer ID
        - @b E_SYS  : Timer for the ID is not opened yet
*/
ER timer_check_timeup(TIMER_ID TimerID, BOOL *pbEn)
{
	RCW_DEF(TIMER_INT0_REG);
#if (_EMULATION_ == DISABLE)
	// Check Timer ID, system timer is not allowed to be referenced by this API
	if ((TimerID > TIMER_LAST_ID) || (TimerID < TIMER_FIRST_ID)) {
		return E_PAR;
	}
#endif

	// Check timer is opened or not
	if ((TimerLockStatus & (TASK_LOCKED << TimerID)) == NO_TASK_LOCKED) {
		return E_SYS;
	}

	RCW_LD(TIMER_INT0_REG);

	// Interrupt is enabled, use event flag mechanism
	if (RCW_VAL(TIMER_INT0_REG) & (TIMER_REG_INTEN_ENABLE << TimerID)) {
		// Wait for timer expired by event flag issued by timer ISR
		*pbEn = (BOOL)(timer_platform_flg_check(FLGPTN_TIMER0 << TimerID) != 0);
	}
	// Interrupt is disabled
	else {
		// Check status
		*pbEn = (BOOL)(timer_getINTStatus(TimerID) != 0);
	}

	return E_OK;
}

/**
    Reload timer

    Reload timer.

    @param[in] TimerID      The timer ID which you would like to reload.
    @param[in] interval     Timer expired interval (1 ~ 0xFFFF_FFFF), unit: Timer clock
    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid timer ID
        - @b E_SYS  : Timer for the ID is not opened yet
*/
ER timer_reload(TIMER_ID TimerID, UINT32 interval)
{
	UINT32 spin_flags;
	RCW_DEF(TIMER_RELOADx_REG);
	RCW_DEF(TIMER_TARGETx_REG);

#if (_EMULATION_ == DISABLE)
	// Check Timer ID, system timer is not allowed to be referenced by this API
	if ((TimerID > TIMER_LAST_ID) || (TimerID < TIMER_FIRST_ID)) {
		return E_PAR;
	}
#endif
	// Check timer is opened or not
	if ((TimerLockStatus & (TASK_LOCKED << TimerID)) == NO_TASK_LOCKED) {
		return E_SYS;
	}
	RCW_LD2(TIMER_RELOADx_REG, TimerID);
	if(RCW_OF(TIMER_RELOADx_REG).RELOAD == TIMER_REG_EN_RELOAD) {
		DBG_WRN("TimerID[%d] not under reloading ... \r\n", TimerID);
		return E_SYS;
	}
	// Must lock CPU before RELOAD bit is set and unlock CPU after polling
	// RELOAD bit done, since caller might be switched to ready queue after
	// unlock CPU, and RELOAD bit will be auto-cleared.
	// And polling RELOAD bit will be a infinite loop.
	spin_flags = timer_platform_spin_lock();

	RCW_VAL(TIMER_TARGETx_REG) = interval;
	RCW_ST2(TIMER_TARGETx_REG, TimerID);

	RCW_LD2(TIMER_RELOADx_REG, TimerID);
	RCW_OF(TIMER_RELOADx_REG).RELOAD  = TIMER_REG_EN_RELOAD;
	RCW_ST2(TIMER_RELOADx_REG, TimerID);

	// Wait for timer is really enabled
#if (_FPGA_EMULATION_ == ENABLE)
	//Dummy read twice
	RCW_LD2(TIMER_RELOADx_REG, TimerID);
	RCW_LD2(TIMER_RELOADx_REG, TimerID);
#else
//	do {
//		RCW_LD2(TIMER_RELOADx_REG, TimerID);
//	} while (RCW_OF(TIMER_RELOADx_REG).RELOAD == TIMER_REG_EN_RELOAD);
#endif
	timer_platform_spin_unlock(spin_flags);

	return E_OK;
}

//@}

/**
    @name Timer utility APIs

    @note This is somewhat hardware dependent and is not suggested to use.
          It is provided here in case of hardware debugging.
*/
//@{

/**
    Configure timer

    Configure timer.

    @param[in] ConfigID     Configuration ID
    @param[in] uiConfig     Configuration value
    @return void
*/
void timer_set_config(TIMER_CONFIG_ID ConfigID, UINT32 uiConfig)
{
	// Only allow to modify in driver release environment
#if (_EMULATION_ == ENABLE)
	DBG_ERR("Not support!\r\n");
#else
	RCW_DEF(TIMER_CLKDIV_REG);
	UINT32 uiDivider;
	UINT32 spin_flags;

	if (ConfigID != TIMER_CONFIG_ID_DIV1_CLK) {
		DBG_ERR("Not supported ID (%d)!\r\n", ConfigID);
		return;
	}

	// Calculate divider
	uiDivider = TIMER_DIVIDER_MIN;

	while (1) {
		if (((TIMER_SOURCE_CLOCK / (uiDivider + 1)) <= uiConfig) || (uiDivider == TIMER_DIVIDER_MAX)) {
			break;
		}

		uiDivider++;
	}

	spin_flags = timer_platform_spin_lock();

	RCW_LD(TIMER_CLKDIV_REG);
	RCW_OF(TIMER_CLKDIV_REG).Divider1 = uiDivider;
	RCW_ST(TIMER_CLKDIV_REG);

	timer_platform_spin_unlock(spin_flags);
#endif
}

/**
    Get timer configuration

    Get timer configuration.

    @param[in] ConfigID     Configuration ID
    @return Configuration value.
*/
UINT32 timer_get_config(TIMER_CONFIG_ID ConfigID)
{
	RCW_DEF(TIMER_CLKDIV_REG);

	if (ConfigID != TIMER_CONFIG_ID_DIV1_CLK) {
		return 0;
	}

	RCW_LD(TIMER_CLKDIV_REG);
	return (TIMER_SOURCE_CLOCK / (RCW_OF(TIMER_CLKDIV_REG).Divider1 + 1));
}

/**
    Return system timer ID

    Return system timer ID.
    System timer will be opened when calling drv_init(). This timer is configured to
    free run where 1 tick is 1us and interrupt is disabled. This timer ID will be only
    valid in the following APIs:
        - @b timer_get_current_count
        - @b timer_getTargetCount

    @return Return system timer ID
*/
TIMER_ID timer_get_sys_timer_id(void)
{
	return TIMER_SYSTIMER_ID;
}

/**
    Read current timer count.

    When timer is paused, current timer count is zero. The timer count will feedback
    faster than the enabled status.

    @param[in] TimerID      The timer ID whose count value you would like to get.

    @return Return unsigned 32-bit timer tick count value
*/
UINT32 timer_get_current_count(TIMER_ID TimerID)
{
	RCW_DEF(TIMER_COUNTERx_REG);

	RCW_LD2(TIMER_COUNTERx_REG, TimerID);

#if (_FPGA_EMULATION_ == ENABLE)
	// Release mode, float type has some problem need to confirm
//    return (UINT32)(TIMER_GETREG(TIMER_0_COUNTER_REG_OFS + TIMER_GET_REGOFFSET(TimerID)) * ((FLOAT)240000000 / _FPGA_PLL_OSC_));
	return (UINT32)(RCW_VAL(TIMER_COUNTERx_REG) * (240000000 / _FPGA_PLL_OSC_));
#else

	return RCW_VAL(TIMER_COUNTERx_REG);
#endif
}

/**
    Read timer's target count value.

    Read timer's target count value. Timer is expired when the current count value
    reaches this target count value.

    @param[in] TimerID      The timer ID whose target count you would like to get.

    @return Return unsigned 32-bit timer tick count value
*/
UINT32 timer_get_target_count(TIMER_ID TimerID)
{
	RCW_DEF(TIMER_TARGETx_REG);

	RCW_LD2(TIMER_TARGETx_REG, TimerID);

	return RCW_VAL(TIMER_TARGETx_REG);
}

/**
    Current timer numbers.

    Get current HW Timer locked numbers.

    @return Return unsigned 32-bit current HW timer numbers
*/
#if 0
UINT32 timer_getCurrHWTimerLockNums(void)
{
	return uiTimerLockedNums;
}
#endif

//@}

//@}

//
// Non-public APIs
//

/*
    Open system timer

    Open system timer.

    @return Void
*/
void timer_open_sys_timer(void)
{
//#ifdef __ECOS
	RCW_DEF(TIMER_CTRLx_REG);
//#endif
	if (nvt_get_chip_id() != CHIP_NA51055) {
#if (_EMULATION_ON_CPU2_ == ENABLE)
		RCW_DEF(TIMER_DST1_REG);
#else
		RCW_DEF(TIMER_DST0_REG);
#endif

	// Configure timer interrupt to specific CPU
#if (_EMULATION_ == ENABLE)
#if (_EMULATION_MULTI_CPU_ == ENABLE)
#if (_EMULATION_ON_CPU2_ == ENABLE)
		// Multiple CPU emulation, timer 16 ~ 19 to CPU2
		RCW_VAL(TIMER_DST1_REG) = 0x000F0000;
		RCW_ST(TIMER_DST1_REG);
#else
		// Multiple CPU emulation, timer 0 ~ 15 to CPU1
		RCW_VAL(TIMER_DST0_REG) = 0x0000FFFF;
		RCW_ST(TIMER_DST0_REG);
#endif
#else
#if (_EMULATION_ON_CPU2_ == ENABLE)
		// Single CPU emulation, all timer's destination are CPU1
		RCW_VAL(TIMER_DST1_REG) = 0x000FFFFF;
		RCW_ST(TIMER_DST1_REG);
#else
		// Single CPU emulation, all timer's destination are CPU1
		RCW_VAL(TIMER_DST0_REG) = 0x000FFFFF;
		RCW_ST(TIMER_DST0_REG);
#endif
#endif
#else
		// Release code, timer 0 ~ 15 to CPU1 (***Need confimr***)
		RCW_VAL(TIMER_DST0_REG) = 0x000FFFFF;
		RCW_ST(TIMER_DST0_REG);
#endif
	}

	// Lock the ID
	if (timer_lockSpecific(TIMER_SYSTIMER_ID) != E_OK) {
		DBG_ERR("TIMER_%d is already opened\r\n", TIMER_SYSTIMER_ID);
		return;
	}

	// Enable TIMER Clock
//	pll_enableClock(TMR_CLK);

//	drv_enableInt(DRV_INT_TIMER);

//	int_gic_set_target(DRV_INT_TIMER, 1);

	// Configure to free run where 1 tick is 1us and interrupt is disable
	// Since timer_cfg() will check the timer ID and that will cause configuration fail.
	// We must call timer_setParameter() instead of timer_cfg().
//#ifdef __ECOS
	// Work-around solution, the system timer will be opened before calling this API.
	// If system timer is enabled, assume the parameter is correct and don't config again.
	RCW_LD2(TIMER_CTRLx_REG, TIMER_SYSTIMER_ID);
	//ctrlReg.Reg = TIMER_GETREG(TIMER_0_CONTROL_REG_OFS + TIMER_GET_REGOFFSET(TIMER_SYSTIMER_ID));
	if (RCW_OF(TIMER_CTRLx_REG).Enable == TIMER_REG_EN_DISABLE)
		//if ((RCW_OF(TIMER_CTRLx_REG).Enable == TIMER_REG_EN_DISABLE) || (RCW_OF(TIMER_CTRLx_REG).Mode == TIMER_REG_MODE_ONE_SHOT))
	{
		DBG_WRN("TIMER_%d is not configured\r\n", TIMER_SYSTIMER_ID);
//#endif
		timer_setParameter(TIMER_SYSTIMER_ID, 0xFFFFFFFF, TIMER_MODE_CLKSRC_DIV0 | TIMER_MODE_FREE_RUN, TIMER_STATE_PLAY);
//#ifdef __ECOS
	}
//#endif
}

#if (TIMER_LOG_OWNER == ENABLE)
/*
    (Verification / Debug code) Dump owner task ID

    (Verification / Debug code) Dump owner task ID.

    @return void
*/
void timer_dump_owner(void)
{
	UINT32 i;
	UINT32 spin_flags;

	spin_flags = timer_platform_spin_lock();

	for (i = 0; i < TIMER_NUM; i++) {
		if ((TimerLockStatus & (TASK_LOCKED << i)) == TASK_LOCKED) {
			DBG_DUMP("Timer %ld occupied by Task %ld\r\n", i, vTimerOwner[i]);
		}
	}

	timer_platform_spin_unlock(spin_flags);
}
#endif

// Emulation only code
#if (_EMULATION_ == ENABLE)

BOOL    timertest_compare_reg_default(void);
ER      timertest_open_specific(TIMER_ID TimerID, DRV_CB EventHandler);
ER      timertest_wait_stop(TIMER_ID TimerID);
UINT32  timertest_get_current_count(TIMER_ID TimerID);
void    timertest_set_config(TIMER_CONFIG_ID ConfigID, UINT32 uiConfig);
void    timertest_set_change_interval(BOOL bChange);

static TIMER_REG_DEFAULT    TimerRegDefault[] = {
	{ 0x00,     TIMER_DST_REG_DEFAULT,          "Dstination 0"  },
	{ 0x04,     TIMER_DST_REG_DEFAULT,          "Dstination 1"  },
	{ 0x08,     TIMER_DST_REG_DEFAULT,          "Dstination 2"  },
	{ 0x0C,     TIMER_DST_REG_DEFAULT,          "Dstination 3"  },
	{ 0x10,     TIMER_STATUS_REG_DEFAULT,       "Status 0"      },
	{ 0x14,     TIMER_STATUS_REG_DEFAULT,       "Status 1"      },
	{ 0x18,     TIMER_STATUS_REG_DEFAULT,       "Status 2"      },
	{ 0x1C,     TIMER_STATUS_REG_DEFAULT,       "Status 3"      },
	{ 0x20,     TIMER_INT_REG_DEFAULT,          "Interrupt 0"   },
	{ 0x24,     TIMER_INT_REG_DEFAULT,          "Interrupt 1"   },
	{ 0x28,     TIMER_INT_REG_DEFAULT,          "Interrupt 2"   },
	{ 0x2C,     TIMER_INT_REG_DEFAULT,          "Interrupt 3"   },
	{ 0x30,     TIMER_CLKDIV_REG_DEFAULT,       "ClockDivider"  }
};

static TIMER_REG_DEFAULT    TimerXRegDefault[] = {
	{ 0x100,    TIMER_CTRLX_REG_DEFAULT,        "Control"       },
	{ 0x104,    TIMER_TARGETX_REG_DEFAULT,      "Target"        },
	{ 0x108,    TIMER_COUNTERX_REG_DEFAULT,     "Counter"       },
	{ 0x10C,    TIMER_RELOADX_REG_DEFAULT,      "Reload"        }
};

/*
    (Verification code) Compare Timer register default value

    (Verification code) Compare Timer register default value.

    @return Compare status
        - @b FALSE  : Register default value is incorrect.
        - @b TRUE   : Register default value is correct.
*/
BOOL timertest_compare_reg_default(void)
{
	UINT32  uiIndex, uiTimerID, uiOffset;
	BOOL    bReturn = TRUE;

	// Shared configuration
	for (uiIndex = 0; uiIndex < (sizeof(TimerRegDefault) / sizeof(TIMER_REG_DEFAULT)); uiIndex++) {
		if (INW(IOADDR_TIMER_REG_BASE + TimerRegDefault[uiIndex].uiOffset) != TimerRegDefault[uiIndex].uiValue) {
			DBG_ERR("%s Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
					TimerRegDefault[uiIndex].pName,
					TimerRegDefault[uiIndex].uiOffset,
					INW(IOADDR_TIMER_REG_BASE + TimerRegDefault[uiIndex].uiOffset),
					TimerRegDefault[uiIndex].uiValue);
			bReturn = FALSE;
		}
	}

	// Timer X configuration
	uiOffset = 0;
	for (uiTimerID = 0; uiTimerID < TIMER_NUM; uiTimerID++) {
		for (uiIndex = 0; uiIndex < (sizeof(TimerXRegDefault) / sizeof(TIMER_REG_DEFAULT)); uiIndex++) {
			if (INW(IOADDR_TIMER_REG_BASE + TimerXRegDefault[uiIndex].uiOffset + uiOffset) != TimerXRegDefault[uiIndex].uiValue) {
				DBG_ERR("%s%ld Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
						TimerXRegDefault[uiIndex].pName,
						uiTimerID,
						TimerXRegDefault[uiIndex].uiOffset + uiOffset,
						INW(IOADDR_TIMER_REG_BASE + TimerXRegDefault[uiIndex].uiOffset + uiOffset),
						TimerXRegDefault[uiIndex].uiValue);
				bReturn = FALSE;
			}
		}

		uiOffset += 0x10;
	}

	return bReturn;
}

/*
    (Verification code) Open specific timer

    (Verification code) Open specific timer.

    @param[in] TimerID          Timer ID to open
    @param[in] EventHandler     Timer expired callback function. Assign NULL if callback is not required.

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Parameters are not valid
        - @b E_SYS  : Maximum number of timers is exceeded
*/
ER timertest_open_specific(TIMER_ID TimerID, DRV_CB EventHandler)
{
	ER erReturn;

	// Check Timer ID
	if (TimerID >= TIMER_NUM) {
		return E_PAR;
	}

	// Lock the ID
	erReturn = timer_lockSpecific(TimerID);
	if (erReturn != E_OK) {
		DBG_ERR("TIMER_%ld is already opened\r\n", TimerID);
		return erReturn;
	}

	// Assign callback function
#if (DRV_SUPPORT_IST == ENABLE)
	pf_ist_cbs_timer[TimerID] = EventHandler;
#else
	pfTimerCB[TimerID] = EventHandler;
#endif

	// Clear flag pattern
	timer_platform_flg_clear(FLGPTN_TIMER0 << TimerID);

	return E_OK;
}

/*
    (Verification code) Wait for timer stopped

    (Verification code) Wait for timer stopped by polling enable bit.

    @param[in] TimerID          Timer ID (TIMER_x) to wait for stopped

    @return
        - @b E_OK   : Done with no errors
        - @b E_PAR  : Invalid timer ID
        - @b E_SYS  : Timer is not opened or wiat for stopped timeout
*/
ER timertest_wait_stop(TIMER_ID TimerID)
{
	RCW_DEF(TIMER_CTRLx_REG);
	UINT32  uiTimeout;

	// Check Timer ID
	if (TimerID >= TIMER_NUM) {
		return E_PAR;
	}

	// Check timer is opened or not
	if ((TimerLockStatus & (TASK_LOCKED << TimerID)) == NO_TASK_LOCKED) {
		return E_SYS;
	}

	uiTimeout = 0;
	do {
		RCW_LD2(TIMER_CTRLx_REG, TimerID);
		uiTimeout++;
	} while ((RCW_OF(TIMER_CTRLx_REG).Enable == TIMER_REG_EN_ENABLE) && (uiTimeout < 0x4000));

	if (uiTimeout == 0x4000) {
		return E_SYS;
	}

	return E_OK;
}

/*
    (Verification code) Get timer current count

    (Verification code) Get timer current count form register without calculating.

    @param[in] TimerID          Timer ID (TIMER_x) to get current count

    @return 32 bits current count
*/
UINT32 timertest_get_current_count(TIMER_ID TimerID)
{
	RCW_DEF(TIMER_COUNTERx_REG);

	RCW_LD2(TIMER_COUNTERx_REG, TimerID);

	return RCW_VAL(TIMER_COUNTERx_REG);
}

/*
    (Verification code) Configure timer

    (Verification code) Configure timer.

    @param[in] ConfigID     Configuration ID
    @param[in] uiConfig     Configuration value (Register setting)
    @return void
*/
void timertest_set_config(TIMER_CONFIG_ID ConfigID, UINT32 uiConfig)
{
	RCW_DEF(TIMER_CLKDIV_REG);

	RCW_LD(TIMER_CLKDIV_REG);

	if (ConfigID == TIMER_CONFIG_ID_DIV1_CLK) {
		RCW_OF(TIMER_CLKDIV_REG).Divider1 = uiConfig;
	} else {
		RCW_OF(TIMER_CLKDIV_REG).Divider0 = uiConfig;
	}

	RCW_ST(TIMER_CLKDIV_REG);
}

// FPGA emulation only code
#if (_FPGA_EMULATION_ == ENABLE)
/*
    (Verification code) Calling timer_cfg() will change interval parameter according to timer clock or not

    (Verification code) Calling timer_cfg() will change interval parameter according to timer clock or not.

    @param[in] bChange      Change timer_cfg() interval parameter or not
        - @b FALSE  : Don't change interval parameter
        - @b TRUE   : Change interval parameter

    @return void
*/
void timertest_set_change_interval(BOOL bChange)
{
	bTimerChangeInterval = bChange;
}
#endif

#endif
