/*
    WDT module driver

    WDT module driver.

    @file       wdt.c
    @ingroup    miDrvTimer_WDT
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "wdt_int.h"
#include "wdt_reg.h"
#include "wdt.h"
#include "interrupt.h"
#include "pll_protected.h"
#include "io_address.h"

#include "kwrap/type.h"
#include <kwrap/nvt_type.h>
#include <kwrap/spinlock.h>
#include "kwrap/semaphore.h"
#include <kwrap/flag.h>
#include <kwrap/debug.h>
#include <comm/driver.h>

static VK_DEFINE_SPINLOCK(wdt_spinlock);
#define loc_cpu(wdt_flags) vk_spin_lock_irqsave(&wdt_spinlock, wdt_flags)
#define unl_cpu(wdt_flags) vk_spin_unlock_irqrestore(&wdt_spinlock, wdt_flags)

#define loc_multiCores(wdt_flags)    loc_cpu(wdt_flags)
#define unl_multiCores(wdt_flags)    unl_cpu(wdt_flags)

static ID SEMID_WDT;
static ID FLG_ID_WDT;
#define FLGPTN_WDT 0x1

// Set register base address
REGDEF_SETIOBASE(IOADDR_WDT_REG_BASE);

static BOOL     bWDTOpened = FALSE;

/*
    SIF interrupt service routine

    SIF interrupt service routine.

    @return void
*/
irqreturn_t wdt_isr(int irq, void *devid)
{
	RCW_DEF(WDT_STS_REG);

	RCW_LD(WDT_STS_REG);

	if (RCW_OF(WDT_STS_REG).Status == 1) {
		// Clear interrupt status
		RCW_ST(WDT_STS_REG);

		// Signal event flag
		iset_flg(FLG_ID_WDT, FLGPTN_WDT);
	}

	return IRQ_HANDLED;
}


/**
    @addtogroup miDrvTimer_WDT
*/
//@{

/**
    Open WDT driver

    Open WDT driver to access WDT controller. Before calling any other WDT driver APIs, you
    have to call this function first and make sure you get E_OK returned code from it.

    @return
        - @b E_OK       : Operation is successful.
        - @b E_ID       : Outside semaphore ID number range.
        - @b E_NOEXS    : Semaphore does not yet exist.
*/
ER wdt_open(void)
{
	RCW_DEF(WDT_CTRL_REG);
	ER erReturn;

	if (bWDTOpened == FALSE) {
		cre_flg(&FLG_ID_WDT, NULL, "FLG_ID_WDT");
		vos_sem_create(&SEMID_WDT, 1, "SEMID_WDT");
	}

	// Wait for semaphore
	if ((erReturn = vos_sem_wait(SEMID_WDT)) != E_OK) {
		return erReturn;
	}

	// Enable WDT clock
	pll_enableClock(WDT_CLK);

	// Enable interrupt
	RCW_LD(WDT_CTRL_REG);
	if (RCW_OF(WDT_CTRL_REG).Mode == WDT_MODE_INT) {
		request_irq(INT_ID_WDT, wdt_isr ,IRQF_TRIGGER_HIGH, "wdt", 0);
	}

	bWDTOpened = TRUE;

	return E_OK;
}

/**
    Close WDT driver

    Close WDT driver to release the resource of WDT controller.

    @return
        - @b E_OK       : Operation is successful.
        - @b E_OACV     : WDT driver is not opened yet.
        - @b E_ID       : Outside semaphore ID number range.
        - @b E_NOEXS    : Semaphore does not yet exist.
        - @b E_QOVR     : Semaphore's counter error, maximum counter < counter.
*/
ER wdt_close(void)
{
	RCW_DEF(WDT_CTRL_REG);
	RCW_DEF(WDT_STS_REG);
	unsigned long flags;

	// Check WDT is opened or not
	if (bWDTOpened != TRUE) {
		return E_OACV;
	}

	// Enter critical section
	loc_cpu(flags);

	// Disable WDT
	RCW_LD(WDT_CTRL_REG);
	RCW_OF(WDT_CTRL_REG).Enable     = 0;
	RCW_OF(WDT_CTRL_REG).Key_Ctrl   = WDT_KEY_VALUE;
	RCW_ST(WDT_CTRL_REG);

	// Wait for WDT is really disabled
	while (1) {
		RCW_LD(WDT_STS_REG);
		if (RCW_OF(WDT_STS_REG).En_Status == 0) {
			break;
		}
	}

	// Disable interrupt
	if (RCW_OF(WDT_CTRL_REG).Mode == WDT_MODE_INT) {
		free_irq(INT_ID_WDT, 0);
	}

	// Disable WDT clock
	pll_disableClock(WDT_CLK);

	bWDTOpened = FALSE;

	// Leave critical section
	unl_cpu(flags);

	// Signal semaphore
	vos_sem_sig(SEMID_WDT);

	rel_flg(FLG_ID_WDT);
	vos_sem_destroy(SEMID_WDT);

	return E_OK;
}

/**
    Configure WDT

    Configuration for WDT.

    @param[in] ConfigID     Configuration ID
    @param[in] uiConfig     Configuration value
    @return void
*/
void wdt_setConfig(WDT_CONFIG_ID ConfigID, UINT32 uiConfig)
{
	RCW_DEF(WDT_CTRL_REG);
	RCW_DEF(WDT_UDATA_REG);
	RCW_DEF(WDT_MANUAL_RESET_REG);
	UINT32  uiValue;
	unsigned long flags;

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return;
	}

	switch (ConfigID) {
	case WDT_CONFIG_ID_MODE:
		if (uiConfig <= WDT_MODE_RESET) {
			// Enter critical section
			loc_cpu(flags);

			RCW_LD(WDT_CTRL_REG);

			// Only modify register when setting is not match current configuration
			if (RCW_OF(WDT_CTRL_REG).Mode != uiConfig) {
				RCW_OF(WDT_CTRL_REG).Mode       = uiConfig;
				RCW_OF(WDT_CTRL_REG).Key_Ctrl   = WDT_KEY_VALUE;
				RCW_ST(WDT_CTRL_REG);

				// Enalbe or disable interrupt
				// if (uiConfig == WDT_MODE_RESET) {
					// drv_disableInt(DRV_INT_WDT);
				// } else {
					// drv_enableInt(DRV_INT_WDT);
				// }
			}

			// Leave critical section
			unl_cpu(flags);
		} else {
			DBG_ERR("Invalid value (%ld) for ID %ld!\r\n", uiConfig, ConfigID);
		}
		break;

	case WDT_CONFIG_ID_TIMEOUT:
		uiValue = (uiConfig * WDT_SOURCE_CLOCK / 1000) >> 12;
		if (uiValue > WDT_MSB_MAX) {
			DBG_WRN("Invalid value (%ld) for ID %ld!\r\n", uiConfig, ConfigID);
			uiValue = WDT_MSB_MAX;
		}

		// Enter critical section
		loc_cpu(flags);

		RCW_LD(WDT_CTRL_REG);
		RCW_OF(WDT_CTRL_REG).MSB        = uiValue;
		RCW_OF(WDT_CTRL_REG).Key_Ctrl   = WDT_KEY_VALUE;
		RCW_ST(WDT_CTRL_REG);

		// Leave critical section
		unl_cpu(flags);
		break;

	case WDT_CONFIG_ID_USERDATA:
		// Enter critical section
		loc_cpu(flags);

		RCW_VAL(WDT_UDATA_REG) = uiConfig;
		RCW_ST(WDT_UDATA_REG);

		// Leave critical section
		unl_cpu(flags);
		break;

	case WDT_CONFIG_ID_EXT_RESET:
		if (uiConfig > WDT_EXT_RESET) {
			DBG_ERR("Invalid value (%ld) for ID %ld!\r\n", uiConfig, ConfigID);
		}
		loc_cpu(flags);
		RCW_LD(WDT_CTRL_REG);
		RCW_OF(WDT_CTRL_REG).external_reset = uiConfig;
		RCW_OF(WDT_CTRL_REG).Key_Ctrl = WDT_KEY_VALUE;
		RCW_ST(WDT_CTRL_REG);
		unl_cpu(flags);
		break;

	case WDT_CONFIG_ID_MANUAL_RESET:
		if (uiConfig > WDT_MANUAL_RESET) {
			DBG_ERR("Invalid value (%ld) for ID %ld!\r\n", uiConfig, ConfigID);
		}
		loc_cpu(flags);
		RCW_LD(WDT_CTRL_REG);
		RCW_OF(WDT_CTRL_REG).Key_Ctrl = WDT_KEY_VALUE;
		RCW_ST(WDT_CTRL_REG);
		RCW_LD(WDT_MANUAL_RESET_REG);
		RCW_OF(WDT_MANUAL_RESET_REG).Manual_Reset = uiConfig;
		RCW_ST(WDT_MANUAL_RESET_REG);
		unl_cpu(flags);
		break;

	case WDT_CONFIG_ID_RST_NUM:
		if (uiConfig > WDT_RST_NUM_ENABLE) {
			DBG_ERR("Invalid value (%ld) for ID %ld!\r\n", uiConfig, ConfigID);
		}
		loc_cpu(flags);
		RCW_LD(WDT_CTRL_REG);
		RCW_OF(WDT_CTRL_REG).reset_num1_en = uiConfig;
		RCW_OF(WDT_CTRL_REG).Key_Ctrl      = WDT_KEY_VALUE;
		RCW_ST(WDT_CTRL_REG);
		unl_cpu(flags);
		break;

	default:
		break;
	}
}

/**
    Get WDT configuration

    Get WDT configuration.

    @param[in] ConfigID     Configuration ID
    @return Configuration value.
*/
UINT32 wdt_getConfig(WDT_CONFIG_ID ConfigID)
{
	RCW_DEF(WDT_CTRL_REG);
	RCW_DEF(WDT_UDATA_REG);

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return 0;
	}

	switch (ConfigID) {
	case WDT_CONFIG_ID_MODE:
		RCW_LD(WDT_CTRL_REG);
		return RCW_OF(WDT_CTRL_REG).Mode;

	case WDT_CONFIG_ID_TIMEOUT:
		RCW_LD(WDT_CTRL_REG);
		return ((RCW_OF(WDT_CTRL_REG).MSB << 12) | 0xFFF) * 1000 / WDT_SOURCE_CLOCK;
		break;

	case WDT_CONFIG_ID_USERDATA:
		RCW_LD(WDT_UDATA_REG);
		return RCW_VAL(WDT_UDATA_REG);

	default:
		return 0;
	}
}

/**
    Enable WDT

    Enable WDT. The WDT will start to count down from the expired time set via
    wdt_setConfig(WDT_CONFIG_ID_TIMEOUT).

    @return void
*/
void wdt_enable(void)
{
	RCW_DEF(WDT_CTRL_REG);
	RCW_DEF(WDT_STS_REG);

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return;
	}

	// Enable WDT
	RCW_LD(WDT_CTRL_REG);
	RCW_OF(WDT_CTRL_REG).Enable = 1;
	RCW_OF(WDT_CTRL_REG).reset_num_en = 1;
	RCW_OF(WDT_CTRL_REG).Key_Ctrl = WDT_KEY_VALUE;
	RCW_ST(WDT_CTRL_REG);

	// Wait for WDT is really enabled
	while (1) {
		RCW_LD(WDT_STS_REG);
		if (RCW_OF(WDT_STS_REG).En_Status == 1) {
			break;
		}
	}
}

/**
    Disable WDT

    Disable WDT. The WDT will stop counting and counter value back to initial value.

    @return void
*/
void wdt_disable(void)
{
	RCW_DEF(WDT_CTRL_REG);
	RCW_DEF(WDT_STS_REG);

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return;
	}

	// Disable WDT
	RCW_LD(WDT_CTRL_REG);
	RCW_OF(WDT_CTRL_REG).Enable     = 0;
	RCW_OF(WDT_CTRL_REG).Key_Ctrl   = WDT_KEY_VALUE;
	RCW_ST(WDT_CTRL_REG);

	// Wait for WDT is really disabled
	while (1) {
		RCW_LD(WDT_STS_REG);
		if (RCW_OF(WDT_STS_REG).En_Status == 0) {
			break;
		}
	}
}

/**
    Trigger WDT

    Trigger WDT and the counter will start to count down again.

    @return void
*/
void wdt_trigger(void)
{
	RCW_DEF(WDT_TRIG_REG);

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return;
	}

	// Wait for previous trigger done
	while (1) {
		RCW_LD(WDT_TRIG_REG);
		if (RCW_OF(WDT_TRIG_REG).Trigger == 0) {
			break;
		}
	}

	// Trigger WDT
	RCW_OF(WDT_TRIG_REG).Trigger = 1;
	RCW_ST(WDT_TRIG_REG);
}

/**
    Clear WDT expired status

    Clear WDT expired status.

    @return void
*/
void wdt_clearTimeout(void)
{
	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return;
	}

	// Clear flag pattern
	clr_flg(FLG_ID_WDT, FLGPTN_WDT);
}

/**
    Wait for WDT expired

    Wait for WDT expired. To make sure you wait the correct event,
    please clear WDT expired status before this API.

    @note   This API is only valid when WDT mode is WDT_MODE_INT.

    @return void
*/
void wdt_waitTimeout(void)
{
	FLGPTN  FlagPtn;

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return;
	}

	// Wait for WDT expired
	wai_flg(&FlagPtn, FLG_ID_WDT, FLGPTN_WDT, TWF_ORW | TWF_CLR);
}

/**
    Get WDT reset number.

    The reset number will be increased one when WDT reset the system. This number
    will only be reset when HW reset and will be kept on 255 if greater than 255.

    @return WDT reset number.
*/
UINT32 wdt_getResetNum(void)
{
	RCW_DEF(WDT_REC_REG);

	//#NT#2016/01/26#Steven Wang -begin
	//#NT#reset number not necessary clock enable
#if 0
	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return 0;
	}
#endif
	//#NT#2016/01/26#Steven Wang -end
	RCW_LD(WDT_REC_REG);
	return RCW_OF(WDT_REC_REG).Reset_Num;
}

//@}

/**
    Get WDT reset number.

    The reset number will be increased one when WDT reset the system. This number
    will only be reset when HW reset and will be kept on 255 if greater than 255.

    @return WDT reset number.
*/
UINT32 wdt_getResetNum1(void)
{
	RCW_DEF(WDT_REC_REG);

	//#NT#2016/01/26#Steven Wang -begin
	//#NT#reset number not necessary clock enable
#if 0
	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return 0;
	}
#endif
	//#NT#2016/01/26#Steven Wang -end
	RCW_LD(WDT_REC_REG);
	return RCW_OF(WDT_REC_REG).Reset_Num1;
}

//@}

// Emulation only code
#if defined(_NVT_EMULATION_)

BOOL    wdtTest_compareRegDefaultValue(void);
void    wdtTest_setConfig(WDT_CONFIG_ID ConfigID, UINT32 uiConfig);
UINT32  wdtTest_getCounter(void);
BOOL    wdtTest_checkTimeout(void);
BOOL    wdtTest_verifyCtrlKey(UINT32 uiKey);
void    wdtTest_waitTriggerDone(void);

static WDT_REG_DEFAULT  WDTRegDefault[] = {
	{ 0x00,     WDT_CTRL_REG_DEFAULT,           "Control"        },
	{ 0x04,     WDT_STS_REG_DEFAULT,            "Status"         },
	{ 0x08,     WDT_TRIG_REG_DEFAULT,           "Trigger"        },
	{ 0x0C,     WDT_MANUAL_RESET_REG_DEFAULT,   "Manual_Reset"   },
};

/*
    (Verification code) Compare WDT register default value

    (Verification code) Compare WDT register default value.

    @note   Not include WDT Timeout Record / User Data Register.

    @return Compare status
        - @b FALSE  : Register default value is incorrect.
        - @b TRUE   : Register default value is correct.
*/
BOOL wdtTest_compareRegDefaultValue(void)
{
	UINT32  uiIndex;
	BOOL    bReturn = TRUE;

	for (uiIndex = 0; uiIndex < (sizeof(WDTRegDefault) / sizeof(WDT_REG_DEFAULT)); uiIndex++) {
		if (INW(_REGIOBASE + WDTRegDefault[uiIndex].uiOffset) != WDTRegDefault[uiIndex].uiValue) {
			DBG_ERR("%s Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
					WDTRegDefault[uiIndex].pName,
					WDTRegDefault[uiIndex].uiOffset,
					INW(_REGIOBASE + WDTRegDefault[uiIndex].uiOffset),
					WDTRegDefault[uiIndex].uiValue);
			bReturn = FALSE;
		}
	}

	return bReturn;
}

/**
    (Verification code) Configure WDT

    (Verification code) Configuration for WDT.

    @param[in] ConfigID     Configuration ID
    @param[in] uiConfig     Configuration value (Register
    @return void
*/
void wdtTest_setConfig(WDT_CONFIG_ID ConfigID, UINT32 uiConfig)
{
	RCW_DEF(WDT_CTRL_REG);
	unsigned long flags;

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return;
	}

	switch (ConfigID) {
	case WDT_CONFIG_ID_TIMEOUT:
		// Enter critical section
		loc_cpu(flags);

		if (uiConfig > WDT_MSB_MAX) {
			uiConfig = WDT_MSB_MAX;
		}
		RCW_LD(WDT_CTRL_REG);
		RCW_OF(WDT_CTRL_REG).MSB        = uiConfig;
		RCW_OF(WDT_CTRL_REG).Key_Ctrl   = WDT_KEY_VALUE;
		RCW_ST(WDT_CTRL_REG);

		// Leave critical section
		unl_cpu(flags);
		break;

	default:
		wdt_setConfig(ConfigID, uiConfig);
		break;
	}
}

/*
    (Verification code) Get current counter value

    (Verification code) Get current counter value.

    @return Current counter value.
*/
UINT32 wdtTest_getCounter(void)
{
	RCW_DEF(WDT_STS_REG);

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return 0;
	}

	RCW_LD(WDT_STS_REG);

	return RCW_OF(WDT_STS_REG).Cnt;
}

/*
    (Verification code) Check if any timeout event occur

    (Verification code) Check if any timeout event occur.

    @return Timeout event occur or not.
        - @b FALSE  : There is no timeout event occur
        - @b TRUE   : There is timeout event occur
*/
BOOL wdtTest_checkTimeout(void)
{
	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return FALSE;
	}

	if (kchk_flg(FLG_ID_WDT, FLGPTN_WDT) == FLGPTN_WDT) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/*
    (Verification code) Verify control key function

    (Verification code) Verify control key function.

    @param[in] uiKey    Key to verify
    @return Verification result.
        - @b FALSE  : Control key function fail
        - @b TRUE   : Control key function OK
*/
BOOL wdtTest_verifyCtrlKey(UINT32 uiKey)
{
	RCW_DEF(WDT_CTRL_REG);
	UINT32          uiNewValue, uiOldValue;
	static UINT32   uiMSB = 0;

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return FALSE;
	}

	RCW_LD(WDT_CTRL_REG);

	uiOldValue = RCW_VAL(WDT_CTRL_REG);

	if (uiMSB > WDT_MSB_MAX) {
		uiMSB = 0;
	}

	RCW_OF(WDT_CTRL_REG).Mode ^= WDT_MODE_RESET;
	RCW_OF(WDT_CTRL_REG).MSB = uiMSB;

	uiNewValue = RCW_VAL(WDT_CTRL_REG);

	uiKey &= 0xFFFF;

	RCW_OF(WDT_CTRL_REG).Key_Ctrl = uiKey;
	RCW_ST(WDT_CTRL_REG);

	RCW_LD(WDT_CTRL_REG);

	if (uiKey == WDT_KEY_VALUE) {
		if (RCW_VAL(WDT_CTRL_REG) != uiNewValue) {
			return FALSE;
		}
	} else {
		if (RCW_VAL(WDT_CTRL_REG) != uiOldValue) {
			return FALSE;
		}
	}

	return TRUE;
}

/*
    (Verification code) Wait for trigger done

    (Verification code) Wait for trigger done.

    @return void
*/
void wdtTest_waitTriggerDone(void)
{
	RCW_DEF(WDT_TRIG_REG);

	if (bWDTOpened == FALSE) {
		DBG_ERR("WDT is not opened!\r\n");
		return;
	}

	// Wait for previous trigger done
	while (1) {
		RCW_LD(WDT_TRIG_REG);
		if (RCW_OF(WDT_TRIG_REG).Trigger == 0) {
			return;
		}
	}
}

#endif
