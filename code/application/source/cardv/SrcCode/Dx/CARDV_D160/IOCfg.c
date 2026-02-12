/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       IOCfg.c
    @ingroup    mIPRJAPCommonIO

    @brief      IO config module
                This file is the IO config module

    @note       Nothing.

    @date       2005/12/24
*/

/** \addtogroup mIPRJAPCommonIO */
//@{

#include "kwrap/type.h"
#include "DrvExt.h"
#include "IOCfg.h"
#include "IOInit.h"
#include "PrjInc.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          DxDrv
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#if (USE_VIO == ENABLE)
UINT32 Virtual_IO[VIO_MAX_ID] = {0};
UINT32 vio_getpin(UINT32 id)
{
	if (id >= VIO_MAX_ID) {
		return 0;
	}
	return Virtual_IO[id];
}
void vio_setpin(UINT32 id, UINT32 v)
{
	if (id >= VIO_MAX_ID) {
		return;
	}
	Virtual_IO[id] = v;
}
#endif

#define GPIO_SET_NONE           0xffffff
#define GPIO_SET_OUTPUT_LOW     0x0
#define GPIO_SET_OUTPUT_HI      0x1

/**
  Do GPIO initialization

  Initialize input/output pins, and pin status

  @param void
  @return void
*/
void IO_InitGPIO(void)
{
#if 0
	UINT32 iSValue;

	MODELEXT_HEADER *header;
	GPIO_INIT_OBJ *uiGPIOMapInitTab;

	uiGPIOMapInitTab = (GPIO_INIT_OBJ *)Dx_GetModelExtCfg(MODELEXT_TYPE_GPIO_INFO, &header);
	if (!uiGPIOMapInitTab || !header) {
		DBG_FATAL("Modelext: iocfg is null\n");
		return;
	}

	UINT32 totalGpioInitCount = header->number;

	DBG_IND("GPIO START\r\n");
	//--------------------------------------------------------------------
	// Open GPIO driver
	//--------------------------------------------------------------------
	gpio_open();
	for (iSValue = 0 ; iSValue < totalGpioInitCount ; iSValue++) {
		if (uiGPIOMapInitTab[iSValue].GpioDir == GPIO_DIR_INPUT) {
			gpio_setDir(uiGPIOMapInitTab[iSValue].GpioPin, GPIO_DIR_INPUT);
			pad_set_pull_updown(uiGPIOMapInitTab[iSValue].PadPin, uiGPIOMapInitTab[iSValue].PadDir);
		} else {
			gpio_setDir(uiGPIOMapInitTab[iSValue].GpioPin, GPIO_DIR_OUTPUT);
			if (uiGPIOMapInitTab[iSValue].PadDir == GPIO_SET_OUTPUT_HI) {
				gpio_setPin(uiGPIOMapInitTab[iSValue].GpioPin);
			} else {
				gpio_clearPin(uiGPIOMapInitTab[iSValue].GpioPin);
			}
		}
	}

	//--------------------------------------------------------------------
	// Use Non-Used PWM to be Delay Timer
	//--------------------------------------------------------------------
#if defined(PWMID_TIMER)
	Delay_setPwmChannels(PWMID_TIMER);
#endif

	DBG_IND("GPIO END\r\n");
#endif
}

/**
  Initialize voltage detection

  Initialize voltage detection for battery and flash

  @param void
  @return void
*/

void IO_InitADC(void)
{
#if 0
	if (adc_open(ADC_CH_VOLDET_BATTERY) != E_OK) {
		DBG_ERR("Can't open ADC channel for battery voltage detection\r\n");
		return;
	}

	//650 Range is 250K Hz ~ 2M Hz
	adc_setConfig(ADC_CONFIG_ID_OCLK_FREQ, 250000); //250K Hz

	//battery voltage detection
	adc_setChConfig(ADC_CH_VOLDET_BATTERY, ADC_CH_CONFIG_ID_SAMPLE_FREQ, 10000); //10K Hz, sample once about 100 us for CONTINUOUS mode
	adc_setChConfig(ADC_CH_VOLDET_BATTERY, ADC_CH_CONFIG_ID_SAMPLE_MODE, (VOLDET_ADC_MODE) ? ADC_CH_SAMPLEMODE_CONTINUOUS : ADC_CH_SAMPLEMODE_ONESHOT);
	adc_setChConfig(ADC_CH_VOLDET_BATTERY, ADC_CH_CONFIG_ID_INTEN, FALSE);
#endif
#if (ADC_KEY == ENABLE)
	if (adc_open(ADC_CH_VOLDET_KEY1) != E_OK) {
		DBG_ERR("Can't open ADC channel for battery voltage detection\r\n");
		return;
	}
	adc_setChConfig(ADC_CH_VOLDET_KEY1, ADC_CH_CONFIG_ID_SAMPLE_FREQ, 10000); //10K Hz, sample once about 100 us for CONTINUOUS mode
	adc_setChConfig(ADC_CH_VOLDET_KEY1, ADC_CH_CONFIG_ID_SAMPLE_MODE, (VOLDET_ADC_MODE) ? ADC_CH_SAMPLEMODE_CONTINUOUS : ADC_CH_SAMPLEMODE_ONESHOT);
	adc_setChConfig(ADC_CH_VOLDET_KEY1, ADC_CH_CONFIG_ID_INTEN, FALSE);
#endif

	// Enable adc control logic
	adc_setEnable(TRUE);
	//???
	//this delay is from 650, but it is not necessary for current IC
	//Delay_DelayMs(15); //wait ADC stable  //for pwr on speed up
}

void GPIO_InitPinmux(void)
{
	gpio_setDir(GPIO_TVI_DET1, GPIO_DIR_INPUT);
	gpio_setDir(GPIO_TVI_DET2, GPIO_DIR_INPUT);
	gpio_pullSet(GPIO_TVI_DET1);
	gpio_pullSet(GPIO_TVI_DET2);
}

void GPIOMap_TurnOnTVI(void)
{
	gpio_setDir(GPIO_TVI_12EN, GPIO_DIR_OUTPUT);
	gpio_setPin(GPIO_TVI_12EN); 
	gpio_setDir(GPIO_TVI_POWER_ON1, GPIO_DIR_OUTPUT);
	gpio_setPin(GPIO_TVI_POWER_ON1); 
	gpio_setDir(GPIO_TVI_POWER_ON2, GPIO_DIR_OUTPUT);
	gpio_setPin(GPIO_TVI_POWER_ON2); 
}

void GPIOMap_TurnOffTVI(void)
{
	gpio_setDir(GPIO_TVI_12EN, GPIO_DIR_OUTPUT);
	gpio_clearPin(GPIO_TVI_12EN); 
	gpio_setDir(GPIO_TVI_POWER_ON1, GPIO_DIR_OUTPUT);
	gpio_clearPin(GPIO_TVI_POWER_ON1); 
	gpio_setDir(GPIO_TVI_POWER_ON2, GPIO_DIR_OUTPUT);
	gpio_clearPin(GPIO_TVI_POWER_ON2); 
}

void GPIOMap_TurnOnSysLed(void)
{
	gpio_setDir(GPIO_RED_LED, GPIO_DIR_OUTPUT);
	gpio_clearPin(GPIO_RED_LED); 
}

void GPIOMap_TurnOffSysLed(void)
{
	gpio_setDir(GPIO_RED_LED, GPIO_DIR_OUTPUT);
	gpio_setPin(GPIO_RED_LED); 
}

void GPIOMap_TurnOnSpkMute(void)
{
	gpio_setDir(GPIO_SPK_MUTE, GPIO_DIR_OUTPUT);
	gpio_clearPin(GPIO_SPK_MUTE); 
}

void GPIOMap_TurnOffSpkMute(void)
{
	gpio_setDir(GPIO_SPK_MUTE, GPIO_DIR_OUTPUT);
	gpio_setPin(GPIO_SPK_MUTE); 
}

void GPIOMap_TurnOnBt(void)
{
	gpio_setDir(GPIO_BT_WAKE, GPIO_DIR_OUTPUT);
	gpio_setDir(GPIO_BT_RST, GPIO_DIR_OUTPUT);

	gpio_setPin(GPIO_BT_WAKE); 
	//Delay_DelayMs(50);
	gpio_clearPin(GPIO_BT_RST); 
	Delay_DelayMs(20);
	gpio_setPin(GPIO_BT_RST); 
	Delay_DelayMs(20);
	DBG_ERR(">>>>>>>>>>>>>>>>>>>>>>>>> GPIOMap_TurnOnWifiBt\r\n");
}

void GPIOMap_TurnOffBt(void)
{
	//gpio_setDir(GPIO_WIFI_POWER, GPIO_DIR_OUTPUT);
	//gpio_setDir(GPIO_WIFI_REG_ON, GPIO_DIR_OUTPUT);
	gpio_setDir(GPIO_BT_WAKE, GPIO_DIR_OUTPUT);
	gpio_setDir(GPIO_BT_RST, GPIO_DIR_OUTPUT);

	gpio_clearPin(GPIO_BT_WAKE); 
	gpio_clearPin(GPIO_BT_RST); 
	DBG_ERR(">>>>>>>>>>>>>>>>>>>>>>>>> GPIOMap_TurnOffBt\r\n");
}

void GPIOMap_TurnOffWifi(void)
{
	gpio_setDir(GPIO_WIFI_POWER, GPIO_DIR_OUTPUT);
	gpio_setDir(GPIO_WIFI_REG_ON, GPIO_DIR_OUTPUT);

	gpio_clearPin(GPIO_WIFI_POWER); 
	gpio_clearPin(GPIO_WIFI_REG_ON); 
	DBG_ERR(">>>>>>>>>>>>>>>>>>>>>>>>> GPIOMap_TurnOffWifi\r\n");
}

BOOL GPIOMap_DetAHD1(void)
{
	//DBG_DUMP(">>>>>>>>>>>>>>>>>>>>>>>>> gpio_getPin(GPIO_TVI_DET1)=%d\r\n",gpio_getPin(GPIO_TVI_DET1));
	return (gpio_getPin(GPIO_TVI_DET1) == 0 ? TRUE : FALSE);
}

BOOL GPIOMap_DetAHD2(void)
{
	//DBG_DUMP(">>>>>>>>>>>>>>>>>>>>>>>>> gpio_getPin(GPIO_TVI_DET2)=%d\r\n",gpio_getPin(GPIO_TVI_DET2));
	return (gpio_getPin(GPIO_TVI_DET2) == 0 ? TRUE : FALSE);
}

void GPIOMap_TpInist(void)
{
	//gpio_setDir(GPIO_TP_INT_DET, GPIO_DIR_OUTPUT);
	gpio_setDir(GPIO_TP_RESET, GPIO_DIR_OUTPUT);

	//gpio_clearPin(GPIO_TP_INT_DET); 
	gpio_clearPin(GPIO_TP_RESET); 
}

BOOL GPIOMap_GetLCDBacklighState(void)
{
	return (gpio_getPin(GPIO_LCD_BLG_PCTL) == 1 ? TRUE : FALSE);
}

void Dx_InitIO(void)  // Config IO for external device
{

	//IO_InitIntDir();    //initial interrupt destination

	GPIO_InitPinmux();    //initial PINMUX config
	//IO_InitGPIO();      //initial GPIO pin status
	IO_InitADC();       //initial ADC pin status
#if 0//defined(_MCU_ENABLE_)
	//Open for DxPower operating
	Dx_Open(Dx_GetObject(DX_CLASS_POWER_EXT));
	MCUCtrl_Open();
#endif
	GPIOMap_TpInist();
	GPIOMap_TurnOnTVI();
	GPIOMap_TurnOffSpkMute();
}

void Dx_UninitIO(void)  // Config IO for external device
{
}



