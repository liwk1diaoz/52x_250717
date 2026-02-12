/*
    ADC module driver.

    This file is the driver of ADC module.

    @file       adc.c
    @ingroup    mIDrvIO_ADC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "io/adc.h"
#include "adc_reg.h"
#include "adc_int.h"
#include "kwrap/type.h"
#include <kwrap/nvt_type.h>
#include "kwrap/semaphore.h"
#include <kwrap/flag.h>
#include <kwrap/debug.h>

// Set register base address
//REGDEF_SETIOBASE(IOADDR_ADC_REG_BASE);
//REGDEF_SETGRPOFS(4);

/**
    @addtogroup mIDrvIO_ADC
*/
//@{
static UINT32  uiADCOpened = FALSE;
static BOOL    g_bADCEnable = FALSE;

UINT32 g_adcCaliOffset = 0;     //0 offset is ideal case
//#if defined(_NVT_FPGA_)
//UINT32 g_adcCaliVDDADC = 3300;  //3.3V is used to do calibration
//#else
UINT32 g_adcCaliVDDADC = 2500;  //2.5V is used to do calibration
//#endif

#if ADC_TODO
static UINT32               uiADCOpenChannel    = 0;
static UINT32               uiAdcOClkFreq       = ADC_SRCCLOCK / 32; // Proposal Default Value
static UINT32               uiADCStatus         = 0;

static UINT32               guiAdcWakeupParam[ADCWU_PARAM_MAX] = {0, 0, 0, 0, 0};
#endif

#if defined(_NVT_EMULATION_)
//#if ADC_DEBUG
volatile UINT32    adcTrigHit = 0;
#endif

#if ADC_TODO
static VK_DEFINE_SPINLOCK(adc_spinlock);
#endif
unsigned long adc_flags;
#define loc_cpu() vk_spin_lock_irqsave(&adc_spinlock, adc_flags)
#define unl_cpu() vk_spin_unlock_irqrestore(&adc_spinlock, adc_flags)

#define loc_multiCores()    loc_cpu()
#define unl_multiCores()    unl_cpu()

#if ADC_TODO
static SEM_HANDLE SEMID_ADC;
static SEM_HANDLE FLG_ID_ADC;
static SEM_HANDLE SEMID_ADC_0;
static SEM_HANDLE SEMID_ADC_1;
static SEM_HANDLE SEMID_ADC_2;
static SEM_HANDLE SEMID_ADC_3;
#endif

#define FLGPTN_ADC 0x1
/*
    ADC controller ISR.

    @return void
*/
#if ADC_TODO
irqreturn_t adc_isr(int irq, void *devid)
{
	RCW_DEF(ADC_STATUS_REG);
	RCW_DEF(ADC_INTCTRL_REG);
#if defined(_NVT_EMULATION_)
	UINT32  i;
#endif

	RCW_LD(ADC_STATUS_REG);
	RCW_LD(ADC_INTCTRL_REG);

	// Only handle interrupt enabled status
	RCW_VAL(ADC_STATUS_REG) &= RCW_VAL(ADC_INTCTRL_REG);

	if (RCW_VAL(ADC_STATUS_REG)) {
		// Save status
		uiADCStatus |= RCW_VAL(ADC_STATUS_REG);

		// Clear interrupt status
		RCW_ST(ADC_STATUS_REG);

		iset_flg(FLG_ID_ADC, FLGPTN_ADC);


#if defined(_NVT_EMULATION_)
//        #if ADC_DEBUG
#if 1
		for (i = 0; i < ADC_TOTAL_CH; i++) {
			if (RCW_VAL(ADC_STATUS_REG) & (0x1 << i)) {
				DBG_DUMP("(%d)CH(%d) Rdy = %d mV\r\n", adcTrigHit, i, adc_readVoltage(i));
			}
		}
		if (RCW_OF(ADC_STATUS_REG).AINTR0) {
			DBG_DUMP("(%d)CH(0) VAL = %d mV\r\n", adcTrigHit, adc_readVoltage(ADC_CHANNEL_0));
		}
		if (RCW_OF(ADC_STATUS_REG).AINTR1) {
			DBG_DUMP("(%d)CH(1) VAL = %d mV\r\n", adcTrigHit, adc_readVoltage(ADC_CHANNEL_1));
		}
		if (RCW_OF(ADC_STATUS_REG).AINTR2) {
			DBG_DUMP("(%d)CH(2) VAL = %d mV\r\n", adcTrigHit, adc_readVoltage(ADC_CHANNEL_2));
		}
#endif
		adcTrigHit++;
#endif
	}

	return IRQ_HANDLED;
}
#endif

#if 1
/*
    Lock ADC driver resource.

    @param[in] uiChannel  ADC channel
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return
        - @b E_OK: lock success
        - @b Else: lock fail
*/
#if ADC_TODO
static ER adc_lock(UINT32 uiChannel)
{
	if (uiChannel == 0)
		return vos_sem_wait(SEMID_ADC_0);
	else if (uiChannel == 1)
		return vos_sem_wait(SEMID_ADC_1);
	else if (uiChannel == 2)
		return vos_sem_wait(SEMID_ADC_2);
	else if (uiChannel == 3)
		return vos_sem_wait(SEMID_ADC_3);
	else {
		DBG_ERR("Invalidarte channel %ld\r\n", uiChannel);
		return E_OACV;
	}
	
}

/*
    Unlock ADC driver resource.

    @param[in] uiChannel  ADC channel
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return
        - @b E_OK: unlock success
        - @b Else: unlock fail
*/
static ER adc_unlock(UINT32 uiChannel)
{

	if (!uiADCOpenChannel) 
		return E_OK;

	if (uiChannel < ADC_TOTAL_CH) {
		if (uiChannel == 0) {
			vos_sem_sig(SEMID_ADC_0);
			return E_OK;
		} else if (uiChannel == 1) {
			vos_sem_sig(SEMID_ADC_1);
			return E_OK;
		} else if (uiChannel == 2) {
			vos_sem_sig(SEMID_ADC_2);
			return E_OK;
		} else if (uiChannel == 3) {
			vos_sem_sig(SEMID_ADC_3);
			return E_OK;
		} else {
			DBG_ERR("Invalidarte channel %ld\r\n", uiChannel);
			return E_OACV;
		}
	} else {
		return E_OK;
	}
}
#endif
/*
    Pre-initialize ADC driver.

    Pre-initialize driver required hardware before driver open.

    @return void
*/
#if ADC_TODO
static void adc_attach(void)
{
	pll_enableClock(ADC_CLK);
}
#endif

/*
    De-initialize ADC driver.

    De-initialize driver required hardware after driver close.

    @return void
*/
#if ADC_TODO
static void adc_detach(void)
{
	pll_disableClock(ADC_CLK);
}
#endif
/*
    ADC wakeup CPU Event CallBack Handler

    In Entering power down process, the handler would store original interrupt enable bits and then replace it
    with the project defined events. In power down Exit process, the handler would restore the original interrupt enable bits.
*/
#if 0
static ER adc_wakeupCallback(CLK_PDN_MODE Mode, BOOL bEnter)
{
	RCW_DEF(ADC_INTCTRL_REG);
	RCW_DEF(ADC_CTRL_REG);
	RCW_DEF(ADC_DIV0_REG);
	RCW_DEF(ADC_STATUS_REG);

	if (!uiADCOpenChannel) {
		return E_OACV;
	}

	adc_setEnable(DISABLE);

	loc_cpu();

	if (Mode != CLK_PDN_MODE_CLKSCALING) {
		if (bEnter == DISABLE) {
			/* Exit PowerDown */

			// Restore Original Interrupt Enable
			RCW_VAL(ADC_INTCTRL_REG)                = guiAdcWakeupParam[ADCWU_ORIGIN_INTEN];
			RCW_ST(ADC_INTCTRL_REG);

			// Restore the original CTRL_REG
			RCW_VAL(ADC_CTRL_REG)                   = guiAdcWakeupParam[ADCWU_ORIGIN_CTRL_REG];
			RCW_ST(ADC_CTRL_REG);

			// Restore the original CH1/2/6 Divider
			RCW_LD(ADC_DIV0_REG);
			RCW_OF(ADC_DIV0_REG).AIN0_DIV           = guiAdcWakeupParam[ADCWU_ORIGIN_DIVIDER] & 0xFF;
			RCW_OF(ADC_DIV0_REG).AIN1_DIV           = (guiAdcWakeupParam[ADCWU_ORIGIN_DIVIDER] >> 8) & 0xFF;
			RCW_OF(ADC_DIV0_REG).AIN2_DIV           = (guiAdcWakeupParam[ADCWU_ORIGIN_DIVIDER] >> 16) & 0xFF;
			RCW_ST(ADC_DIV0_REG);
			// Keep the interrupt status here and Handle it at ISR if it is originally enabled

		} else {
			/* Enter PowerDown */

			// Save Original Interrupt Enable and Assert Wakeup Source INTEN
			RCW_LD(ADC_INTCTRL_REG);
			guiAdcWakeupParam[ADCWU_ORIGIN_INTEN]   = RCW_VAL(ADC_INTCTRL_REG);
			RCW_VAL(ADC_INTCTRL_REG) = guiAdcWakeupParam[ADCWU_SOURCE];
			RCW_ST(ADC_INTCTRL_REG);

			// Save the original: CTRL_REG
			RCW_LD(ADC_CTRL_REG);
			guiAdcWakeupParam[ADCWU_ORIGIN_CTRL_REG] = RCW_VAL(ADC_CTRL_REG);
			RCW_OF(ADC_CTRL_REG).AIN0_MODE          = 1;    //Cts mode
			RCW_OF(ADC_CTRL_REG).AIN1_MODE          = 1;    //Cts mode
			RCW_OF(ADC_CTRL_REG).AIN2_MODE          = 1;    //Cts mode
			RCW_OF(ADC_CTRL_REG).CLKDIV             = 0x0;
			RCW_OF(ADC_CTRL_REG).SAMPAVG            = 0x0;
			RCW_OF(ADC_CTRL_REG).EXTSAMP_CNT        = 0x0;
			RCW_ST(ADC_CTRL_REG);

			//Save the original: CH0/CH1/CH2 divider
			RCW_LD(ADC_DIV0_REG);
			guiAdcWakeupParam[ADCWU_ORIGIN_DIVIDER] = (RCW_OF(ADC_DIV0_REG).AIN0_DIV) + (RCW_OF(ADC_DIV0_REG).AIN1_DIV << 8) + (RCW_OF(ADC_DIV0_REG).AIN2_DIV << 16);
			RCW_OF(ADC_DIV0_REG).AIN0_DIV = 0x0;
			RCW_OF(ADC_DIV0_REG).AIN1_DIV = 0x0;
			RCW_OF(ADC_DIV0_REG).AIN2_DIV = 0x0;
			RCW_ST(ADC_DIV0_REG);

			// Clear the Wakeup Source interrrupt Status
			RCW_VAL(ADC_STATUS_REG)                 = guiAdcWakeupParam[ADCWU_SOURCE];
			RCW_ST(ADC_STATUS_REG);
		}
	}

	unl_cpu();

	adc_setEnable(ENABLE);
	return E_OK;
}
#endif
#endif

/**
    @name   ADC General API
*/
//@{

/**
    Open ADC driver.

    This function should be called before calling any other functions and will
    enable interrup and clock.

    @param[in] Channel    ADC channel ID
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return Open ADC driver status
     - @b E_OK: Operation successful
     - @b Otherwise2: Operation failed
*/
ER adc_open(ADC_CHANNEL Channel)
{
	if (uiADCOpened) {
		uiADCOpened++;
		return E_OK;
	}

	uiADCOpened = 1;

	return E_OK;
}

/**
    Close ADC driver.

    Release ADC driver and let other application use ADC. This API will disable clock
    and interrupt if all the channels are closed.

    @param[in] Channel    ADC channel ID
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return Close ADC driver status
     - @b E_OK: Operation successful
     - @b Otherwise2: Operation failed
*/
ER adc_close(ADC_CHANNEL Channel)
{
	if (uiADCOpened == FALSE) {
		return E_OK;
	} else if (uiADCOpened > 1) {
		uiADCOpened--;
		return E_OK;
	}

	uiADCOpened = FALSE;

	return E_OK;
}

/**
    Check the ADC Channel is opened

    Check the specified ADC Channel is opened.

    @param[in] Channel  ADC channel ID
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return ADC driver is opened or not
        - @b TRUE:  Specified ADC Channel is opened.
        - @b FALSE: Specified ADC Channel is NOT opened.
*/
BOOL adc_isOpened(ADC_CHANNEL Channel)
{
	return uiADCOpened > 0;
}

/**
    Enable/Disable ADC controller

    Enable/Disable adce controller.
    The adc_open() must be initialized first before enabling adc controller.

    @param[in] bEn  TRUE is to enable, FALSE is to disable adc controller.

    @return
        - @b E_OK:   Done and success
        - @b E_OACV: Error! API is called before adc_open()
*/
ER adc_setEnable(BOOL bEn)
{
	g_bADCEnable = bEn;
	return E_OK;
}

/**
    Get Enable/Disable status of ADC controller

    Get Enable/Disable status of ADC controller

    @return
        - @b TRUE:   Enabled
        - @b FALSE:  Disabled
*/
BOOL adc_getEnable(void)
{
	return g_bADCEnable;
}

/**
    Set ADC general control

    Set general configurations for the ADC controller.

    @param[in] CfgID        The configuration group identification code. Please refer to ADC_CONFIG_ID for reference.
    @param[in] uiCfgValue   The configuration value. Please refer to ADC_CONFIG_ID for value range reference.

    @return void
*/
void adc_setConfig(ADC_CONFIG_ID CfgID, UINT32 uiCfgValue)
{
#if ADC_TODO
	RCW_DEF(ADC_CTRL_REG);
	RCW_DEF(ADC_THERMAL_CFG_REG);

	loc_cpu();
	RCW_LD(ADC_CTRL_REG);

	switch (CfgID) {
	case ADC_CONFIG_ID_OCLK_FREQ: {
			UINT32 uiDiv;

			if (uiCfgValue > ADC_OCLK_MAX) {
				uiCfgValue = ADC_OCLK_MAX;
			}

			uiDiv = ((ADC_SRCCLOCK / 2) + uiCfgValue - 1) / uiCfgValue;
			if (uiDiv > 32) {
				uiDiv = 32;
			}

			uiAdcOClkFreq = ADC_SRCCLOCK / 2 / uiDiv;

			RCW_OF(ADC_CTRL_REG).CLKDIV = uiDiv - 1;
		}
		break;

	case ADC_CONFIG_ID_SAMPLE_AVERAGE: {
			RCW_OF(ADC_CTRL_REG).SAMPAVG = uiCfgValue;
		}
		break;

	case ADC_CONFIG_ID_EXT_SAMPLE_CNT: {
			if (uiCfgValue > ADC_EXTCNT_MAX) {
				uiCfgValue = ADC_EXTCNT_MAX;
			}

			RCW_OF(ADC_CTRL_REG).EXTSAMP_CNT = uiCfgValue;
		}
		break;

	case ADC_CONFIG_ID_THERMAL_AVG: {
			if (uiCfgValue > ADC_THERMAL_AVG_MAX) {
				uiCfgValue = ADC_THERMAL_AVG_MAX;
			}

			RCW_LD(ADC_THERMAL_CFG_REG);
			RCW_OF(ADC_THERMAL_CFG_REG).AIN_AVG_CNT = uiCfgValue;
			RCW_ST(ADC_THERMAL_CFG_REG);
		}
		break;


	default:
		break;

	}

	RCW_ST(ADC_CTRL_REG);

	unl_cpu();
#endif
}

/**
    Get ADC general control

    Get general configurations of the ADC controller.

    @param[in] CfgID        The configuration group identification code. Please refer to ADC_CONFIG_ID for reference.

    @return The configuration value.
*/
UINT32 adc_getConfig(ADC_CONFIG_ID CfgID)
{
	UINT32  Ret = 0;
#if ADC_TODO
	RCW_DEF(ADC_CTRL_REG);

	RCW_LD(ADC_CTRL_REG);
	switch (CfgID) {
	case ADC_CONFIG_ID_OCLK_FREQ: {
			Ret = uiAdcOClkFreq;
		}
		break;

	case ADC_CONFIG_ID_SAMPLE_AVERAGE: {
			Ret = RCW_OF(ADC_CTRL_REG).SAMPAVG;
		}
		break;

	case ADC_CONFIG_ID_EXT_SAMPLE_CNT: {
			Ret = RCW_OF(ADC_CTRL_REG).EXTSAMP_CNT;
		}
		break;

	default:
		break;

	}
#endif
	return Ret;
}

/**
    Set ADC Channel control

    Set ADC specified Channel configurations.

    @param[in] Channel      ADC channel ID
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @param[in] CfgID        The configuration group identification code. Please refer to ADC_CH_CONFIG_ID for reference.
    @param[in] uiCfgValue   The configuration value. Please refer to ADC_CH_CONFIG_ID for value range reference.

    @return void
*/
void adc_setChConfig(ADC_CHANNEL Channel, ADC_CH_CONFIG_ID CfgID, UINT32 uiCfgValue)
{
#if ADC_TODO
	RCW_DEF(ADC_CTRL_REG);
	RCW_DEF(ADC_INTCTRL_REG);

	if (Channel >= ADC_TOTAL_CH) {
		return;
	}

	loc_cpu();
	switch (CfgID) {
	case ADC_CH_CONFIG_ID_SAMPLE_MODE: {
			RCW_LD(ADC_CTRL_REG);
			if (uiCfgValue == ADC_CH_SAMPLEMODE_CONTINUOUS) {
				RCW_VAL(ADC_CTRL_REG) |= (0x01 << Channel);
			} else {
				RCW_VAL(ADC_CTRL_REG) &= ~(0x01 << Channel);
			}

			RCW_ST(ADC_CTRL_REG);
		}
		break;

	case ADC_CH_CONFIG_ID_SAMPLE_FREQ: {
			UINT32  uiDivider, Fs;
			RCW_DEF(ADC_DIV0_REG);

			RCW_LD(ADC_CTRL_REG);

			Fs = uiAdcOClkFreq / 4 / (26 + RCW_OF(ADC_CTRL_REG).EXTSAMP_CNT) / (0x1 << RCW_OF(ADC_CTRL_REG).SAMPAVG); // 4CH, each CH (26+N) T
			uiDivider = (Fs + uiCfgValue - 1) / uiCfgValue;
			if (uiDivider > 256) {
				uiDivider = 256;
			}

			RCW_LD(ADC_DIV0_REG);
			RCW_VAL(ADC_DIV0_REG) = (RCW_VAL(ADC_DIV0_REG) & ~(0xFF << (Channel * 8))) | ((uiDivider - 1) << (Channel * 8));
			RCW_ST(ADC_DIV0_REG);

#if defined(_NVT_EMULATION_)
			unl_cpu();// unloc before uart msg.
			DBG_DUMP("ADC%d src %dHertz\r\n", Channel, Fs);
			Fs = Fs / uiDivider;
			DBG_DUMP("ADC%d at %dHertz, div %d\r\n", Channel, Fs, uiDivider);
			return; // CPU already unlock
#endif
		}
		break;

	case ADC_CH_CONFIG_ID_INTEN: {
			// Set Interrupt Enable
			RCW_LD(ADC_INTCTRL_REG);
			if (uiCfgValue == ENABLE) {
				RCW_VAL(ADC_INTCTRL_REG) |= (AIN0_INT_EN << Channel);
			} else {
				RCW_VAL(ADC_INTCTRL_REG) &= ~(AIN0_INT_EN << Channel);
			}

			RCW_ST(ADC_INTCTRL_REG);
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_VOL_LOW: {
			RCW_DEF(ADC_TRIGVAL0_REG);
			RCW_DEF(ADC_TRIGVAL1_REG);
			RCW_DEF(ADC_TRIGVAL2_REG);


			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				if (uiCfgValue > g_adcCaliVDDADC) {
					uiCfgValue = g_adcCaliVDDADC;
				}

				uiCfgValue = ((uiCfgValue * 511 / g_adcCaliVDDADC) - g_adcCaliOffset) >> 1;

				if (Channel == ADC_CHANNEL_0) {
					RCW_LD(ADC_TRIGVAL0_REG);
					RCW_OF(ADC_TRIGVAL0_REG).TRIG0_START = uiCfgValue;
					RCW_ST(ADC_TRIGVAL0_REG);
				}

				if (Channel == ADC_CHANNEL_1) {
					RCW_LD(ADC_TRIGVAL1_REG);
					RCW_OF(ADC_TRIGVAL1_REG).TRIG1_START = uiCfgValue;
					RCW_ST(ADC_TRIGVAL1_REG);
				}

				if (Channel == ADC_CHANNEL_2) {
					RCW_LD(ADC_TRIGVAL2_REG);
					RCW_OF(ADC_TRIGVAL2_REG).TRIG2_START = uiCfgValue;
					RCW_ST(ADC_TRIGVAL2_REG);
				}
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_VOL_HIGH: {
			RCW_DEF(ADC_TRIGVAL0_REG);
			RCW_DEF(ADC_TRIGVAL1_REG);
			RCW_DEF(ADC_TRIGVAL2_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				if (uiCfgValue > g_adcCaliVDDADC) {
					uiCfgValue = g_adcCaliVDDADC;
				}

				uiCfgValue = ((uiCfgValue * 511 / g_adcCaliVDDADC) - g_adcCaliOffset) >> 1;

				if (Channel == ADC_CHANNEL_0) {
					RCW_LD(ADC_TRIGVAL0_REG);
					RCW_OF(ADC_TRIGVAL0_REG).TRIG0_END = uiCfgValue;
					RCW_ST(ADC_TRIGVAL0_REG);
				}

				if (Channel == ADC_CHANNEL_1) {
					RCW_LD(ADC_TRIGVAL1_REG);
					RCW_OF(ADC_TRIGVAL1_REG).TRIG1_END = uiCfgValue;
					RCW_ST(ADC_TRIGVAL1_REG);
				}

				if (Channel == ADC_CHANNEL_2) {
					RCW_LD(ADC_TRIGVAL2_REG);
					RCW_OF(ADC_TRIGVAL2_REG).TRIG2_END = uiCfgValue;
					RCW_ST(ADC_TRIGVAL2_REG);
				}
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_THD_LOW: {
			RCW_DEF(ADC_TRIGVAL0_REG);
			RCW_DEF(ADC_TRIGVAL1_REG);
			RCW_DEF(ADC_TRIGVAL2_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				if (uiCfgValue > ADC_VALTRIG_MAX) {
					uiCfgValue = ADC_VALTRIG_MAX;
				}

				if (Channel == ADC_CHANNEL_0) {
					RCW_LD(ADC_TRIGVAL0_REG);
					RCW_OF(ADC_TRIGVAL0_REG).TRIG0_START = uiCfgValue;
					RCW_ST(ADC_TRIGVAL0_REG);
				}

				if (Channel == ADC_CHANNEL_1) {
					RCW_LD(ADC_TRIGVAL1_REG);
					RCW_OF(ADC_TRIGVAL1_REG).TRIG1_START = uiCfgValue;
					RCW_ST(ADC_TRIGVAL1_REG);
				}

				if (Channel == ADC_CHANNEL_2) {
					RCW_LD(ADC_TRIGVAL2_REG);
					RCW_OF(ADC_TRIGVAL2_REG).TRIG2_START = uiCfgValue;
					RCW_ST(ADC_TRIGVAL2_REG);
				}
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_THD_HIGH: {
			RCW_DEF(ADC_TRIGVAL0_REG);
			RCW_DEF(ADC_TRIGVAL1_REG);
			RCW_DEF(ADC_TRIGVAL2_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				if (uiCfgValue > ADC_VALTRIG_MAX) {
					uiCfgValue = ADC_VALTRIG_MAX;
				}

				if (Channel == ADC_CHANNEL_0) {
					RCW_LD(ADC_TRIGVAL0_REG);
					RCW_OF(ADC_TRIGVAL0_REG).TRIG0_END = uiCfgValue;
					RCW_ST(ADC_TRIGVAL0_REG);
				}

				if (Channel == ADC_CHANNEL_1) {
					RCW_LD(ADC_TRIGVAL1_REG);
					RCW_OF(ADC_TRIGVAL1_REG).TRIG1_END = uiCfgValue;
					RCW_ST(ADC_TRIGVAL1_REG);
				}

				if (Channel == ADC_CHANNEL_2) {
					RCW_LD(ADC_TRIGVAL2_REG);
					RCW_OF(ADC_TRIGVAL2_REG).TRIG2_END = uiCfgValue;
					RCW_ST(ADC_TRIGVAL2_REG);
				}
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_RANGE: {
			RCW_DEF(ADC_TRIGCTRL_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				// Set Value Trigger function Enable
				RCW_LD(ADC_TRIGCTRL_REG);
				RCW_VAL(ADC_TRIGCTRL_REG) &= ~(0x01 << (ADC_VALTRIG_RANGE_OFS + Channel));
				RCW_VAL(ADC_TRIGCTRL_REG) |= (uiCfgValue << (ADC_VALTRIG_RANGE_OFS + Channel));
				RCW_ST(ADC_TRIGCTRL_REG);
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_MODE: {
			RCW_DEF(ADC_TRIGCTRL_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				// Set Value Trigger function Enable
				RCW_LD(ADC_TRIGCTRL_REG);
				RCW_VAL(ADC_TRIGCTRL_REG) &= ~(0x01 << (ADC_VALTRIG_MODE_OFS + Channel));
				RCW_VAL(ADC_TRIGCTRL_REG) |= (uiCfgValue << (ADC_VALTRIG_MODE_OFS + Channel));
				RCW_ST(ADC_TRIGCTRL_REG);
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_EN: {
			RCW_DEF(ADC_TRIGCTRL_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				// Set Value Trigger function Enable
				RCW_LD(ADC_TRIGCTRL_REG);

				if (uiCfgValue) {
					RCW_VAL(ADC_TRIGCTRL_REG) |= (0x1 << Channel);
				} else {
					RCW_VAL(ADC_TRIGCTRL_REG) &= ~(0x1 << Channel);
				}

				RCW_ST(ADC_TRIGCTRL_REG);
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_INTEN: {
			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				// Set Interrupt Enable
				RCW_LD(ADC_INTCTRL_REG);
				if (uiCfgValue) {
					RCW_VAL(ADC_INTCTRL_REG) |= (0x01 << (Channel + ADC_STS_VALTRIG_OFS));
				} else {
					RCW_VAL(ADC_INTCTRL_REG) &= ~(0x01 << (Channel + ADC_STS_VALTRIG_OFS));
				}

				RCW_ST(ADC_INTCTRL_REG);
			}
		}
		break;


	default:
		break;
	}
	unl_cpu();

#endif
}

/**
    Get ADC Channel control

    Get ADC specified Channel configurations.

    @param[in] Channel      ADC channel ID
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @param[in] CfgID        The configuration group identification code. Please refer to ADC_CH_CONFIG_ID for reference.

    @return The configuration value.
*/
UINT32 adc_getChConfig(ADC_CHANNEL Channel, ADC_CH_CONFIG_ID CfgID)
{
	UINT32  Ret = 0;
#if ADC_TODO
	RCW_DEF(ADC_CTRL_REG);
	RCW_DEF(ADC_INTCTRL_REG);

	if (Channel >= ADC_TOTAL_CH) {
		return Ret;
	}


	switch (CfgID) {
	case ADC_CH_CONFIG_ID_SAMPLE_MODE: {
			RCW_LD(ADC_CTRL_REG);
			Ret = ((RCW_VAL(ADC_CTRL_REG) >> Channel) & 0x1);
		}
		break;

	case ADC_CH_CONFIG_ID_SAMPLE_FREQ: {
			RCW_DEF(ADC_DIV0_REG);
			UINT32  uiDivider, Fs;


			RCW_LD(ADC_CTRL_REG);
			Fs = uiAdcOClkFreq / 4 / (26 + RCW_OF(ADC_CTRL_REG).EXTSAMP_CNT) / (0x1 << RCW_OF(ADC_CTRL_REG).SAMPAVG); // 4CH, each CH (26+N) T

			RCW_LD(ADC_DIV0_REG);
			uiDivider = (RCW_VAL(ADC_DIV0_REG) >> (Channel * 8)) & 0xFF;

			Ret = (UINT32) Fs / (uiDivider + 1);
		}
		break;

	case ADC_CH_CONFIG_ID_INTEN: {
			// Set Interrupt Enable
			RCW_LD(ADC_INTCTRL_REG);
			Ret = (RCW_VAL(ADC_INTCTRL_REG) >> Channel) & 0x1;
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_THD_LOW: {
			RCW_DEF(ADC_TRIGVAL0_REG);
			RCW_DEF(ADC_TRIGVAL1_REG);
			RCW_DEF(ADC_TRIGVAL2_REG);

			if (Channel == ADC_CHANNEL_0) {
				RCW_LD(ADC_TRIGVAL0_REG);
				Ret = RCW_OF(ADC_TRIGVAL0_REG).TRIG0_START;
			}

			if (Channel == ADC_CHANNEL_1) {
				RCW_LD(ADC_TRIGVAL1_REG);
				Ret = RCW_OF(ADC_TRIGVAL1_REG).TRIG1_START;
			}

			if (Channel == ADC_CHANNEL_2) {
				RCW_LD(ADC_TRIGVAL2_REG);
				Ret = RCW_OF(ADC_TRIGVAL2_REG).TRIG2_START;
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_THD_HIGH: {
			RCW_DEF(ADC_TRIGVAL0_REG);
			RCW_DEF(ADC_TRIGVAL1_REG);
			RCW_DEF(ADC_TRIGVAL2_REG);

			if (Channel == ADC_CHANNEL_0) {
				RCW_LD(ADC_TRIGVAL0_REG);
				Ret = RCW_OF(ADC_TRIGVAL0_REG).TRIG0_END;
			}

			if (Channel == ADC_CHANNEL_1) {
				RCW_LD(ADC_TRIGVAL1_REG);
				Ret = RCW_OF(ADC_TRIGVAL1_REG).TRIG1_END;
			}

			if (Channel == ADC_CHANNEL_2) {
				RCW_LD(ADC_TRIGVAL2_REG);
				Ret = RCW_OF(ADC_TRIGVAL2_REG).TRIG2_END;
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_RANGE: {
			RCW_DEF(ADC_TRIGCTRL_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				// Set Value Trigger function Enable
				RCW_LD(ADC_TRIGCTRL_REG);
				Ret = (RCW_VAL(ADC_TRIGCTRL_REG) >> (ADC_VALTRIG_RANGE_OFS + Channel)) & 0x1;
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_MODE: {
			RCW_DEF(ADC_TRIGCTRL_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				// Set Value Trigger function Enable
				RCW_LD(ADC_TRIGCTRL_REG);
				Ret = (RCW_VAL(ADC_TRIGCTRL_REG) >> (ADC_VALTRIG_MODE_OFS + Channel)) & 0x1;
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_EN: {
			RCW_DEF(ADC_TRIGCTRL_REG);

			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				// Set Value Trigger function Enable
				RCW_LD(ADC_TRIGCTRL_REG);
				Ret = (RCW_VAL(ADC_TRIGCTRL_REG) >> Channel) & 0x1;
			}
		}
		break;

	case ADC_CH_CONFIG_ID_VALUETRIG_INTEN: {
			if ((Channel == ADC_CHANNEL_0) || (Channel == ADC_CHANNEL_1) || (Channel == ADC_CHANNEL_2)) {
				// Set Interrupt Enable
				RCW_LD(ADC_INTCTRL_REG);
				Ret = (RCW_VAL(ADC_INTCTRL_REG) >> (Channel + ADC_STS_VALTRIG_OFS - 1)) & 0x1;
			}
		}
		break;
	default:
		break;
	}
#endif
	return Ret;

}

/**
    Trigger one-shot of ADC channel.

    This function trigger one-shot of ADC channel 0 ~ 7.
    After triggering, one can call adc_isDataReady(uiChannel) to check if
    the converted data is ready, and then use adc_readData(uiChannel) to
    read the data.

    @param[in] Channel    The channel you want to trigger one-shot
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return void
*/
void adc_triggerOneShot(ADC_CHANNEL Channel)
{
	char file_name[128];
	FILE *voltage_fd;
	UINT32 uiVoltage = 0;

	snprintf(file_name, sizeof(file_name), "/sys/devices/platform/f0260000.adc/iio:device0/in_voltage%d_raw", (int)Channel);

	if ((voltage_fd = fopen(file_name, "rb")) == NULL) {
		printf("open voltage file (%s) fail !!\r\n", file_name);
		return;
	}

	if (fscanf(voltage_fd, "%d\n", (int *)&uiVoltage) == EOF) {
		printf("[ERROR] fscanf error\n");
		fclose(voltage_fd);
		return;
	}

	fclose(voltage_fd);

	return;
}

/**
    Read ADC conversion value.

    This function read the AD conversion value of specific channel.

    @param[in] Channel    The channel you want to read conversion value
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return ADC conversion value of the channel, the value ranges in 0~511
*/
UINT32 adc_readData(ADC_CHANNEL Channel)
{
	char file_name[128];
	FILE *voltage_fd;
	UINT32 uiVoltage = 0;

	snprintf(file_name, sizeof(file_name), "/sys/devices/platform/f0260000.adc/iio:device0/in_voltage%d_raw", (int)Channel);

	if ((voltage_fd = fopen(file_name, "rb")) == NULL) {
		printf("open voltage file (%s) fail !!\r\n", file_name);
		return 0;
	}

	if (fscanf(voltage_fd, "%d\n", (int *)&uiVoltage) == EOF) {
		printf("[ERROR] fscanf error\n");
		fclose(voltage_fd);
		return 0;
	}

	fclose(voltage_fd);

	return ((uiVoltage * 511) / g_adcCaliVDDADC) - g_adcCaliOffset;
}

/**
    Read ADC conversion value in voltage.

    This function read the AD conversion value in voltage of specific channel.

    The voltage range assumes nV mapping to 512 steps, where n is set by
    adc_setCaliOffset(Offset,CalVDDADC)'s CalVDDADC parameter.  nV will be
    default to 3V (CalVDDADC=3000).

    @param[in] Channel    The channel you want to read conversion value
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return Voltage*1000 corresponds to ADC conversion value of the channel
*/
UINT32 adc_readVoltage(ADC_CHANNEL Channel)
{
	char file_name[128];
	FILE *voltage_fd;
	UINT32 uiVoltage = 0;

	snprintf(file_name, sizeof(file_name), "/sys/devices/platform/f0260000.adc/iio:device0/in_voltage%d_raw", (int)Channel);

	if ((voltage_fd = fopen(file_name, "rb")) == NULL) {
		printf("open voltage file (%s) fail !!\r\n", file_name);
		return 0;
	}

	if (fscanf(voltage_fd, "%d\n", (int *)&uiVoltage) == EOF) {
		printf("[ERROR] fscanf error\n");
		fclose(voltage_fd);
		return 0;
	}

	fclose(voltage_fd);

	return uiVoltage;
}

/**
    Read ADC thermal average value.

    This function read the average conversion value of specific channel.

    @return ADC thermal average conversion value, the value ranges in 0~511
*/
UINT32 adc_thermal_read_data(void)
{
	DBG_ERR("Not support\r\n");
	return 0;
}

/**
    Read ADC thermal average value in voltage.

    This function read the average conversion value of specific channel.

    The voltage range assumes nV mapping to 512 steps

    @return Voltage*1000 corresponds to ADC thermal average value
*/
UINT32 adc_thermal_read_voltage(void)
{
	DBG_ERR("Not support\r\n");
	return 0;
}

/**
    Check ADC data ready status for respective channel.

    This function check the data ready status of channel.

    @param[in] Channel    The channel you want to check
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return
     - @b TRUE: The channel has data
     - @b FALSE: The channel doesn't have data
*/
BOOL adc_isDataReady(ADC_CHANNEL Channel)
{
	DBG_ERR("Not support\r\n");
	return 0;
}

/**
    Clear ADC channel X's data ready status.

    This function clear the data ready status of channel.

    @param[in] Channel    The channel you want to clear data ready status
     - @b ADC_CHANNEL_0
     - @b ADC_CHANNEL_1
     - @b ADC_CHANNEL_2
     - @b ADC_CHANNEL_3

    @return void
*/
void adc_clearDataReady(ADC_CHANNEL Channel)
{
	DBG_ERR("Not support\r\n");
}

/**
    Set ADC Calibration offset.

    The ADC calibration is performed in two phases: first one is IC readout offset,
    second is power IC offset.  In the calibration process, ADC0 is used as IC
    offset calibration.\n

    Two calibrated data is calculated as below and they will be stored in NAND
    parameters area:\n

          Offset=511-AD0\n
          VDD_ADC=(3000*511)/(AD1+Offset)    ;input 3V in AD1 and calculate actual voltage\n

    The voltage read out is as below:\n
          Voltage=(AD1+Offset)*VDD_ADC/511\n
      =>\n
          AD1 = ((V*511)/VDD_ADC) - Offset\n

    @param[in] Offset       ADC IC offset data
    @param[in] CalVDDADC    Power IC calibrated data
    @return void
*/
void adc_setCaliOffset(UINT32 Offset, UINT32 CalVDDADC)
{
	g_adcCaliOffset = Offset;
	g_adcCaliVDDADC = CalVDDADC;
}

/**
    Set the ADC wakeup CPU source condition

    After CPU entering power down mode 1/2/3, the ADC module provides the ability to wakeup the CPU
    by the ADC channel or touch panel.

    @param[in] CfgID    Set wakeup control configurations select.
    @param[in] uiParam  Control Parameters.

    @return
     - @b E_OK:     Config Done and success.
     - @b E_NOEXS:  Error CfgID.
*/
ER adc_setWakeupConfig(ADC_WAKEUP_CONFIG CfgID, UINT32 uiParam)
{
	DBG_ERR("Not support\r\n");
	return E_OK;
}

/**
    Wait ADC interrupt event

    Wait the ADC specified interrupt event. When the interrupt event is triggered and match the wanted events,
    the waited event flag would be returned.

    @param[in] WaitedFlag  The bit-wise2 OR of the waited interrupt events.

    @return The waited interrupt events.
*/
ADC_INTERRUPT adc_waitInterrupt(ADC_INTERRUPT WaitedFlag)
{
	ADC_INTERRUPT   Ret = 0;
	DBG_ERR("Not support\r\n");
	return Ret;
}