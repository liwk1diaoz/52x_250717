/*
    Remote controller

    Remote controller Driver

    @file       remote.c
    @ingroup    mIDrvIO_Remote
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "remote_int.h"
#include "remote_reg.h"

// Set register base address
//REGDEF_SETIOBASE(IOADDR_REMOTE_REG_BASE);

/**
    @addtogroup mIDrvIO_Remote
*/
//@{

#if (DRV_SUPPORT_IST == ENABLE)

#else
static DRV_CB pRemoteCBFunc = NULL;
#endif

static UINT32 guiRemoteWakeupParam[REMOTE_WAKEUP_PARAM_MAX] = {REMOTE_INT_MATCH, 0};
static UINT32 remote_debug = REMOTE_DEBUG;

/*
    remote interrupt service routine

    remote interrupt service routine

    @return void
*/
void remote_isr(void)
{
	UINT32 spin_flags;
	RCW_DEF(REMOTE_STATUS_REG);
	RCW_DEF(REMOTE_INTEN_REG);
	RCW_DEF(REMOTE_CONTROL_REG);

	spin_flags = remote_platform_spin_lock();

	RCW_LD(REMOTE_STATUS_REG);
	RCW_LD(REMOTE_INTEN_REG);

	// Get the interrupt status bits which is also enabled.
	RCW_VAL(REMOTE_STATUS_REG) &= RCW_VAL(REMOTE_INTEN_REG);

	// Check if the ISR is triggered by the remote or not. (Due to the system share interrupt design.)
	if (RCW_VAL(REMOTE_STATUS_REG)) {
		// Clear the interrupt status bits, which trigger this ISR event.
		RCW_ST(REMOTE_STATUS_REG);

#if (DRV_SUPPORT_IST == ENABLE)
		// If the IST implementation is enabled,
		// Trigger the IST event to make the IST handle the Remote Callback function.
		if (pf_ist_cbs_remote_platform != NULL) {
			remote_platform_set_ist_event((UINT32)RCW_VAL(REMOTE_STATUS_REG));
		}
#else
		// If the IST implementation is disabled,
		// The Remote Callback function is executed in the remote ISR.
		if (pRemoteCBFunc) {
			pRemoteCBFunc(RCW_VAL(REMOTE_STATUS_REG));
		}
#endif

		if (remote_debug) {
			// Below messages are only for internal debug usages.
			if (RCW_OF(REMOTE_STATUS_REG).rm_rd == 1) {
				RCW_LD(REMOTE_CONTROL_REG);
				if (RCW_OF(REMOTE_CONTROL_REG).bi_phase_en == 1) {
					REMOTE_PATTERN    pattern, pattern_raw;
					remote_getDataCommand(&pattern);
					remote_getRawDataCommand(&pattern_raw);
					DBG_DUMP("%s: RM_RD  (H)0x%x  (L)0x%x  (RH)0x%x  (RL) 0x%x\r\n", __func__, (unsigned int)(pattern.uiHigh), (unsigned int)(pattern.uiLow), (unsigned int)(pattern_raw.uiHigh), (unsigned int)(pattern_raw.uiLow));
				} else {
					REMOTE_PATTERN    pattern;
					remote_getDataCommand(&pattern);
					DBG_DUMP("%s: RM_RD  (H)0x%x  (L)0x%x\r\n", __func__, (unsigned int)(pattern.uiHigh), (unsigned int)(pattern.uiLow));
				}
			}
			if (RCW_OF(REMOTE_STATUS_REG).rm_err == 1) {
				DBG_DUMP("%s: RM_ERR\r\n", __func__);
			}
			if (RCW_OF(REMOTE_STATUS_REG).match  == 1) {
				DBG_DUMP("%s: RM_MATCH\r\n", __func__);
			}
			if (RCW_OF(REMOTE_STATUS_REG).repeat == 1) {
				DBG_DUMP("%s: RM_REP\r\n", __func__);
			}
			if (RCW_OF(REMOTE_STATUS_REG).overrun == 1) {
				DBG_DUMP("%s: RM_OVERRUN\r\n", __func__);
			}
		}
	}

	remote_platform_spin_unlock(spin_flags);
}


//
//  Static register programming routines
//

/*
    remote lock
*/
static ER remote_lock(void)
{
	// Acquire semaphore for remote controller use
	return remote_platform_sem_wait();
}

/*
    remote unlock
*/
static ER remote_unlock(void)
{
	// Release remote controller semaphore
	return remote_platform_sem_signal();
}

/*
    Attach remote controller driver.

    Open remote clock and select remote pinmux.

    @return
    - @b E_OK: Done and success.
*/
static ER remote_attach(void)
{
	remote_platform_select_clock_source(REMOTE_CLK_SRC_RTC);
	remote_platform_clock_enable();

	return E_OK;
}

/*
    Detach remote controller driver

    Close remote clock and set remote_rx as GPIO.

    @return
    - @b E_OK: Done and success.
*/
static ER remote_detach(void)
{
	remote_platform_clock_disable();

	return E_OK;
}

/*
    remote wakeup CPU Event CallBack Handler

    In Entering power down process, the handler would store original interrupt enable bits and then replace it
    with the project defined events. In power down Exit process, the handler would restore the original interrupt enable bits.
*/
/*static ER remote_wakeupCallback(CLK_PDN_MODE Mode, BOOL bEnter)
{
	UINT32 spin_flags;
	RCW_DEF(REMOTE_INTEN_REG);

	if (guiRemoteStatus != REMOTE_STATUS_OPEN) {
		return E_OACV;
	}

	spin_flags = remote_platform_spin_lock();

	// Set the CPU wakeup conditions if the CPU is entering the SLEEP Mode 1/2/3.
	if (Mode != CLK_PDN_MODE_CLKSCALING) {
		if (bEnter == DISABLE) {
			// Exit PowerDown:
			// Restore the original interrupt enable bits.
			RCW_VAL(REMOTE_INTEN_REG) = guiRemoteWakeupParam[REMOTE_WAKEUP_ORIGINAL_INTEN];
		} else {
			// Enter PowerDown:
			// Save the original interrupt enable bits and then replace the enable bits by the project assigned.
			RCW_LD(REMOTE_INTEN_REG);
			guiRemoteWakeupParam[REMOTE_WAKEUP_ORIGINAL_INTEN] = RCW_VAL(REMOTE_INTEN_REG);
			RCW_VAL(REMOTE_INTEN_REG) = guiRemoteWakeupParam[REMOTE_WAKEUP_SRC];
		}

		RCW_ST(REMOTE_INTEN_REG);
	}
	remote_platform_spin_unlock(spin_flags);
	return E_OK;
}*/

#if 1

/**
    @name Remote Controller Driver API
*/
//@{


/**
    Open remote controller driver for access.

    Open remote controller driver for access.
    The ISR callback handler can be assigned together.
    If none of the callback is needed, just assign it to NULL.

    @param[in] pEventHdl    Callback function registered for interrupt notification.

    @return
     - @b E_OK: if operation is successful
     - @b Others: error codes if any errors.
*/
ER remote_open(DRV_CB pEventHdl)
{
	UINT32 erReturn;
	RCW_DEF(REMOTE_STATUS_REG);

	if (guiRemoteStatus == REMOTE_STATUS_OPEN) {
		return E_OK;
	}

	erReturn = remote_lock();
	if (erReturn != E_OK) {
		return erReturn;
	}

	// Enable Remote Source Clock
	remote_attach();

	// Clear all interrupt status
	RCW_VAL(REMOTE_STATUS_REG) = REMOTE_INT_ALL;
	RCW_ST(REMOTE_STATUS_REG);

	// Enable interrupt
	remote_platform_int_enable();

	// Set CPU wakeup event callback to clock driver.
	//clk_set_callback(CLK_CALLBACK_RM_PWRDN, remote_wakeupCallback);

#if (DRV_SUPPORT_IST == ENABLE)
	pf_ist_cbs_remote_platform  = pEventHdl;
#else
	pRemoteCBFunc   = pEventHdl;
#endif

	guiRemoteStatus = REMOTE_STATUS_OPEN;

	return E_OK;
}

/**
    Close remote controller driver.

    Close remote controller driver.

    @return
     - @b E_OK:     The remote driver is in the closed state.
     - @b Others:   Error codes if any errors.
*/
ER remote_close(void)
{
	RCW_DEF(REMOTE_INTEN_REG);

	if (guiRemoteStatus == REMOTE_STATUS_CLOSED) {
		return E_OK;
	}

	// Close Remote Source Clock
	remote_detach();

	// Disable interrupt
	remote_platform_int_disable();

	// Disable all interrupt enable bits
	RCW_VAL(REMOTE_INTEN_REG) = 0;
	RCW_ST(REMOTE_INTEN_REG);

#if (DRV_SUPPORT_IST == ENABLE)
	pf_ist_cbs_remote_platform  = NULL;
#else
	pRemoteCBFunc   = NULL;
#endif

	guiRemoteStatus = REMOTE_STATUS_CLOSED;

	return remote_unlock();
}

/**
    Enable/Disable remote controller

    Enable/Disable remote controller.
    The remote_open() must be initialized first before enabling remote controller.

    @param[in] bEn  TRUE is to enable, FALSE is to disable remote controller.

    @return
        - @b E_OK:   Done and success
        - @b E_OACV: Error! API is called before remote_open()
*/
ER remote_setEnable(BOOL bEn)
{
	UINT32 spin_flags;
	RCW_DEF(REMOTE_CONTROL_REG);

	if (guiRemoteStatus != REMOTE_STATUS_OPEN) {
		return E_OACV;
	}

	spin_flags = remote_platform_spin_lock();
	RCW_LD(REMOTE_CONTROL_REG);
	RCW_OF(REMOTE_CONTROL_REG).rm_en = bEn;
	RCW_ST(REMOTE_CONTROL_REG);
	remote_platform_spin_unlock(spin_flags);

	return E_OK;
}

/**
    Set remote control functionality configuration.

    Set remote control functionality configuration. This set configuration API can be used before driver opening.

    @param[in] CfgID        The set configuration group id. Please refer to REMOTE_CONFIG_ID for reference.
    @param[in] uiCfgValue   The set configuration value. Please refer to REMOTE_CONFIG_ID for the value details.

    @return
     - @b E_CTX:   Operation fail because unknown config ID.
     - @b E_OK:    Operation Done and Success.
*/
ER remote_setConfig(REMOTE_CONFIG_ID CfgID, UINT32 uiCfgValue)
{
	UINT32 spin_flags;
	RCW_DEF(REMOTE_TH0_REG);
	RCW_DEF(REMOTE_TH1_REG);
	RCW_DEF(REMOTE_TH2_REG);
	RCW_DEF(REMOTE_CONTROL_REG);
	RCW_DEF(REMOTE_MATCH_LOW_DATA_REG);
	RCW_DEF(REMOTE_MATCH_HIGH_DATA_REG);
	UINT64  count;

	if (CfgID == REMOTE_CONFIG_ID_CLK_SRC_SEL) {
		// Select input clock source. Don't put this section of code into the
		// switch block below, because of changing clock source cannot be within
		// the lock section in linux.
		remote_platform_clock_disable();
		remote_platform_select_clock_source(uiCfgValue);
		remote_platform_clock_enable();
		return E_OK;
	}

	// Re-Map the setting period from US to the controller clock cycles according to the module clock design.
	count = uiCfgValue * guiRemoteCycleTime;
#if defined __LINUX
	count = do_div(count, 1000000);
#else
	count = count / 1000000;
#endif

	spin_flags = remote_platform_spin_lock();
	switch (CfgID) {
	case REMOTE_CONFIG_ID_LOGIC_TH: {
			if (count > 2047) {
				count = 2047;
			}

			// Set Logic0/1 Threshold Value as clock cycles
			// The distance length larger than the threshold would be regarded as 1, otherwise 0.
			RCW_LD(REMOTE_TH1_REG);
			RCW_OF(REMOTE_TH1_REG).logic_th = count;
			RCW_ST(REMOTE_TH1_REG);
		}
		break;

	case REMOTE_CONFIG_ID_LOGIC_TH_HW: {
			if (uiCfgValue > 2047) {
				uiCfgValue = 2047;
			}

			// Set Logic0/1 Threshold Value as clock cycles
			// The distance length larger than the threshold would be regarded as 1, otherwise 0.
			RCW_LD(REMOTE_TH1_REG);
			RCW_OF(REMOTE_TH1_REG).logic_th = uiCfgValue;
			RCW_ST(REMOTE_TH1_REG);
		}
		break;

	case REMOTE_CONFIG_ID_GSR_TH: {
			if (count > 255) {
				count = 255;
			}

			// Set Glitch Suppress Threshold Value as clock cycles.
			// The input signal change below than the threshold period would be treated as glitch
			// And the glitch would be ignored.
			RCW_LD(REMOTE_TH1_REG);
			RCW_OF(REMOTE_TH1_REG).rm_gsr = count;
			RCW_ST(REMOTE_TH1_REG);
		}
		break;

	case REMOTE_CONFIG_ID_GSR_TH_HW: {
			if (uiCfgValue > 255) {
				uiCfgValue = 255;
			}

			// Set Glitch Suppress Threshold Value as clock cycles.
			// The input signal change below than the threshold period would be treated as glitch
			// And the glitch would be ignored.
			RCW_LD(REMOTE_TH1_REG);
			RCW_OF(REMOTE_TH1_REG).rm_gsr = uiCfgValue;
			RCW_ST(REMOTE_TH1_REG);
		}
		break;

	case REMOTE_CONFIG_ID_REPEAT_TH: {
			if (count > 65535) {
				count = 65535;
			}

			// This is designed to distinguish the repeat code or the header code.
			// In the NEC protocol, the normal command after header is about 4.2ms and the repeat pulse after
			// the header is about 2.2ms. So this paramter must be set to the range from 2.2 ~ 4.2 ms.
			// To help the controller differentiate the normal command or the repeat code.
			RCW_LD(REMOTE_TH0_REG);
			RCW_OF(REMOTE_TH0_REG).rep_th = count;
			RCW_ST(REMOTE_TH0_REG);
		}
		break;

	case REMOTE_CONFIG_ID_HEADER_TH: {
			if (count > 65535) {
				count = 65535;
			}

			// The pulse width larger than this header threshold value would be treated as HEADER.
			RCW_LD(REMOTE_TH0_REG);
			RCW_OF(REMOTE_TH0_REG).header_th = count;
			RCW_ST(REMOTE_TH0_REG);
		}
		break;

	case REMOTE_CONFIG_ID_ERROR_TH: {
			if (count > 65535) {
				count = 65535;
			}
			// If the un-complete command stopping over the error threshold value.
			// It would trigger the controller error interrupt event.
			RCW_LD(REMOTE_TH2_REG);
			RCW_OF(REMOTE_TH2_REG).error_th = count;
			RCW_ST(REMOTE_TH2_REG);
		}
		break;

	case REMOTE_CONFIG_ID_ERROR_TH_HW: {
			if (uiCfgValue > 65535) {
				uiCfgValue = 65535;
			}
			// If the un-complete command stopping over the error threshold value.
			// It would trigger the controller error interrupt event.
			RCW_LD(REMOTE_TH2_REG);
			RCW_OF(REMOTE_TH2_REG).error_th = uiCfgValue;
			RCW_ST(REMOTE_TH2_REG);
		}
		break;

	case REMOTE_CONFIG_ID_THRESHOLD_SEL: {
			// Set the signal logic 0/1 is judged by the distance of pulse or space.
			RCW_LD(REMOTE_CONTROL_REG);
			RCW_OF(REMOTE_CONTROL_REG).th_sel = uiCfgValue;
			RCW_ST(REMOTE_CONTROL_REG);
		}
		break;

	case REMOTE_CONFIG_ID_DATA_LENGTH: {
			// Set the command length is 1~64 bits.
			RCW_LD(REMOTE_CONTROL_REG);
			RCW_OF(REMOTE_CONTROL_REG).rm_length = uiCfgValue;
			RCW_ST(REMOTE_CONTROL_REG);
		}
		break;

	case REMOTE_CONFIG_ID_DATA_ORDER: {
			// Select the input command signal is LSb or MSb first.
			RCW_LD(REMOTE_CONTROL_REG);
			RCW_OF(REMOTE_CONTROL_REG).rm_msb = uiCfgValue;
			RCW_ST(REMOTE_CONTROL_REG);
		}
		break;

	case REMOTE_CONFIG_ID_HEADER_DETECT: {
			// Select the force header detection enable/disable.
			RCW_LD(REMOTE_CONTROL_REG);
			RCW_OF(REMOTE_CONTROL_REG).rm_hedf_chkmethod = uiCfgValue;
			RCW_ST(REMOTE_CONTROL_REG);
		}
		break;

	case REMOTE_CONFIG_ID_INPUT_INVERSE: {
			// Select input inverse enable/disable
			RCW_LD(REMOTE_CONTROL_REG);
			RCW_OF(REMOTE_CONTROL_REG).rm_inv = uiCfgValue;
			RCW_ST(REMOTE_CONTROL_REG);
		}
		break;

	case REMOTE_CONFIG_ID_REPEAT_DET_EN: {
			// Select repeat code detection enable/disable
			RCW_LD(REMOTE_CONTROL_REG);
			RCW_OF(REMOTE_CONTROL_REG).rm_rep_en = uiCfgValue;
			RCW_ST(REMOTE_CONTROL_REG);
		}
		break;

	case REMOTE_CONFIG_ID_MATCH_LOW: {
			// Configure the MATCH command LSb 32 bits.
			// If the received command is matched with the MATCH command configuration,
			// it would trigger the MATCH interrupt status.
			// This function is designed to wakeup the CPU from sleeo mode
			// through the external remote control's power key.
			RCW_LD(REMOTE_MATCH_LOW_DATA_REG);
			RCW_OF(REMOTE_MATCH_LOW_DATA_REG).match_data_l = uiCfgValue;
			RCW_ST(REMOTE_MATCH_LOW_DATA_REG);
		}
		break;

	case REMOTE_CONFIG_ID_MATCH_HIGH: {
			// Configure the MATCH command MSb 32 bits.
			// If the received command is matched with the MATCH command configuration,
			// it would trigger the MATCH interrupt status.
			// This function is designed to wakeup the CPU from sleeo mode
			// through the external remote control's power key.
			RCW_LD(REMOTE_MATCH_HIGH_DATA_REG);
			RCW_OF(REMOTE_MATCH_HIGH_DATA_REG).match_data_h = uiCfgValue;
			RCW_ST(REMOTE_MATCH_HIGH_DATA_REG);
		}
		break;

	case REMOTE_CONFIG_ID_WAKEUP_ENABLE: {
			// Select the wakeup CPU source interrupt enable
			guiRemoteWakeupParam[REMOTE_WAKEUP_SRC] |= (uiCfgValue & REMOTE_INT_ALL);
		}
		break;

	case REMOTE_CONFIG_ID_WAKEUP_DISABLE: {
			// Select the wakeup CPU source interrupt disable
			guiRemoteWakeupParam[REMOTE_WAKEUP_SRC] &= ~(uiCfgValue & REMOTE_INT_ALL);
		}
		break;

	case REMOTE_CONFIG_ID_BI_PHASE_EN: {
			if (nvt_get_chip_id() == CHIP_NA51055) {
				DBG_DUMP("%s: the platform does not support bi-phase mode\r\n", __func__);
			} else {
				// Select bi-phase mode enable/disable
				RCW_LD(REMOTE_CONTROL_REG);
				RCW_OF(REMOTE_CONTROL_REG).bi_phase_en = uiCfgValue;
				RCW_ST(REMOTE_CONTROL_REG);
			}
		}
		break;

	case REMOTE_CONFIG_ID_BI_PHASE_DETECT_HEADER_TH: {
			// Set bi-phase mode to detect the long header pulse, such as RC6 protocol.
			RCW_LD(REMOTE_CONTROL_REG);
			if (RCW_OF(REMOTE_CONTROL_REG).bi_phase_en == 1) {
				RCW_LD(REMOTE_CONTROL_REG);
				RCW_OF(REMOTE_CONTROL_REG).bi_phase_detect_header_th = uiCfgValue;
				RCW_ST(REMOTE_CONTROL_REG);
			} else {
				DBG_DUMP("%s: bi-phase mode is disabled, do nothing\r\n", __func__);
			}
		}
		break;

	case REMOTE_CONFIG_ID_BI_PHASE_HEADER_LENGTH: {
			// Set bi-phase header length, the unit is half cycle
			// EX: A 2T header with the following pattern should be described with match_length = 4
			//       |------|      |------|
			//       |      |      |      |
			// ------|      |------|      |
			//  1/2T   1/2T   1/2T   1/2T
			RCW_LD(REMOTE_CONTROL_REG);
			if (RCW_OF(REMOTE_CONTROL_REG).bi_phase_en == 1) {
				RCW_LD(REMOTE_CONTROL_REG);
				RCW_OF(REMOTE_CONTROL_REG).match_length = uiCfgValue;
				RCW_ST(REMOTE_CONTROL_REG);
			} else {
				DBG_DUMP("%s: bi-phase mode is disabled, do nothing\r\n", __func__);
			}
		}
		break;

	case REMOTE_CONFIG_ID_BI_PHASE_HEADER_MATCH_LOW: {
			// Set bi-phase header match data low, each bit represents a level of half cycle, and the order is LSb first
			// EX: A 2T header with the following pattern should be described with match_data_l = 0xA (1010)
			//       |------|      |------|
			//       |      |      |      |
			// ------|      |------|      |
			//  1/2T   1/2T   1/2T   1/2T
			RCW_LD(REMOTE_CONTROL_REG);
			if (RCW_OF(REMOTE_CONTROL_REG).bi_phase_en == 1) {
				RCW_LD(REMOTE_MATCH_LOW_DATA_REG);
				RCW_OF(REMOTE_MATCH_LOW_DATA_REG).match_data_l = uiCfgValue;
				RCW_ST(REMOTE_MATCH_LOW_DATA_REG);
			} else {
				DBG_DUMP("%s: bi-phase mode is disabled, do nothing\r\n", __func__);
			}
		}
		break;

	case REMOTE_CONFIG_ID_BI_PHASE_HEADER_MATCH_HIGH: {
			// Set bi-phase header match data high, each bit represents a level of half cycle, and the order is LSb first
			RCW_LD(REMOTE_CONTROL_REG);
			if (RCW_OF(REMOTE_CONTROL_REG).bi_phase_en == 1) {
				RCW_LD(REMOTE_MATCH_HIGH_DATA_REG);
				RCW_OF(REMOTE_MATCH_HIGH_DATA_REG).match_data_h = uiCfgValue;
				RCW_ST(REMOTE_MATCH_HIGH_DATA_REG);
			} else {
				DBG_DUMP("%s: bi-phase mode is disabled, do nothing\r\n", __func__);
			}
		}
		break;

	case REMOTE_CONFIG_ID_REMOTE_DEBUG: {
			if (remote_debug) {
				remote_debug = 0;
				DBG_DUMP("%s: remote_debug is disabled\r\n", __func__);
			} else {
				remote_debug = 1;
				DBG_DUMP("%s: remote_debug is enabled\r\n", __func__);
			}
		}
		break;

	default:
		remote_platform_spin_unlock(spin_flags);
		return E_CTX;
	}
	remote_platform_spin_unlock(spin_flags);


	return E_OK;
}


/**
    Set remote control interrupt enable

    Set remote control interrupt enable.

    @param[in] IntEn    The interrupt IDs which specified to be enabled.

    @return
     - @b E_OACV:  Operation fail because remote has not open.
     - @b E_OK:    Operation Done and Success.
*/
ER remote_setInterruptEnable(REMOTE_INTERRUPT IntEn)
{
	UINT32 spin_flags;

	RCW_DEF(REMOTE_INTEN_REG);

	if (guiRemoteStatus != REMOTE_STATUS_OPEN) {
		return E_OACV;
	}

	spin_flags = remote_platform_spin_lock();
	RCW_LD(REMOTE_INTEN_REG);
	RCW_VAL(REMOTE_INTEN_REG) |= (IntEn & REMOTE_INT_ALL);
	RCW_ST(REMOTE_INTEN_REG);
	remote_platform_spin_unlock(spin_flags);

	return E_OK;
}

/**
    Set remote control interrupt disable

    Set remote control interrupt disable.

    @param[in] IntDis    The interrupt IDs which specified to be disabled.

    @return
     - @b E_OACV:  Operation fail because remote has not open.
     - @b E_OK:    Operation Done and Success.
*/
ER remote_setInterruptDisable(REMOTE_INTERRUPT IntDis)
{
	UINT32 spin_flags;
	RCW_DEF(REMOTE_INTEN_REG);

	if (guiRemoteStatus != REMOTE_STATUS_OPEN) {
		return E_OACV;
	}

	spin_flags = remote_platform_spin_lock();
	RCW_LD(REMOTE_INTEN_REG);
	RCW_VAL(REMOTE_INTEN_REG) &= ~(IntDis & REMOTE_INT_ALL);
	RCW_ST(REMOTE_INTEN_REG);
	remote_platform_spin_unlock(spin_flags);

	return E_OK;
}

/**
    Get remote control Data Command

    When the remote data ready is issued, the user can get the remote data command by this API.
    The returned data command is shown by the structure REMOTE_PATTERN.

    @param[out] pDataCmd    The returned data command. Please reference to the structure REMOTE_PATTERN for details.

    @return void
*/
void remote_getDataCommand(PREMOTE_PATTERN pDataCmd)
{
	RCW_DEF(REMOTE_LOW_DATA_REG);
	RCW_DEF(REMOTE_HIGH_DATA_REG);

	RCW_LD(REMOTE_LOW_DATA_REG);
	RCW_LD(REMOTE_HIGH_DATA_REG);

	pDataCmd->uiLow  = RCW_VAL(REMOTE_LOW_DATA_REG);
	pDataCmd->uiHigh = RCW_VAL(REMOTE_HIGH_DATA_REG);
}

/**
    Get remote control Raw Bi-phase Data Command

	Only available with bi_phase_en is 1, each bit indicates '1' or '0' with Manchester coding.
    When the remote data ready is issued, the user can get the remote raw bi-phase data command by this API.
    The returned data command is shown by the structure REMOTE_PATTERN.

    @param[out] pDataCmd    The returned data command. Please reference to the structure REMOTE_PATTERN for details.

    @return void
*/
void remote_getRawDataCommand(PREMOTE_PATTERN pDataCmd)
{
	RCW_DEF(REMOTE_RAW_LOW_DATA_REG);
	RCW_DEF(REMOTE_RAW_HIGH_DATA_REG);

	RCW_LD(REMOTE_RAW_LOW_DATA_REG);
	RCW_LD(REMOTE_RAW_HIGH_DATA_REG);

	pDataCmd->uiLow  = RCW_VAL(REMOTE_RAW_LOW_DATA_REG);
	pDataCmd->uiHigh = RCW_VAL(REMOTE_RAW_HIGH_DATA_REG);
}

//@}
#endif

//@}

