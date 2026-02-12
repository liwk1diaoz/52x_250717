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


void GPIOMap_AmpPowerOn(void);


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
    gpio_setDir(GPIO_SENSOR1_POWER, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_SENSOR1_POWER);
    //gpio_setDir(GPIO_SENSOR2_POWER, GPIO_DIR_OUTPUT);
    //gpio_setPin(GPIO_SENSOR2_POWER);
   // gpio_setDir(GPIO_SENSOR1_PWND, GPIO_DIR_OUTPUT);
   // gpio_setPin(GPIO_SENSOR1_PWND);
	//gpio_setDir(GPIO_SENSOR1_RESET, GPIO_DIR_OUTPUT);
    //gpio_setPin(GPIO_SENSOR1_RESET);
    //gpio_setDir(GPIO_SENSOR_AHD_POWER2, GPIO_DIR_OUTPUT);
   // gpio_setPin(GPIO_SENSOR_AHD_POWER2);
    //gpio_setDir(GPIO_SENSOR_AHD_POWER3, GPIO_DIR_OUTPUT);
    //gpio_setPin(GPIO_SENSOR_AHD_POWER3);
    gpio_setDir(GPIO_SENSOR_AHD_RESET, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_SENSOR_AHD_RESET);
	GPIOMap_AmpPowerOn();

    //gpio_setDir(GPIO_AMP_SHDN, GPIO_DIR_OUTPUT);
    //gpio_clearPin(GPIO_AMP_SHDN);

    gpio_setDir(GPIO_LCD_EN, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_LCD_EN);

    //gpio_setDir(GPIO_LCD_RESET, GPIO_DIR_OUTPUT);
    //gpio_setPin(GPIO_LCD_RESET);
    //gpio_setDir(GPIO_BAT_POWER, GPIO_DIR_OUTPUT);//Cy
    //gpio_setPin(GPIO_BAT_POWER);//Cy
    if(gpio_getDir(GPIO_MCU_STATUS)!=GPIO_DIR_OUTPUT) {
        DBG_DUMP("\033[34m [ZMD:%s:%d]:\033[0m\r\n",__func__,__LINE__);
        gpio_setDir(GPIO_MCU_STATUS, GPIO_DIR_OUTPUT);
    }
    if(gpio_getPin(GPIO_MCU_STATUS)!=1) {
        DBG_DUMP("\033[34m [ZMD:%s:%d]:\033[0m\r\n",__func__,__LINE__);
        gpio_setPin(GPIO_MCU_STATUS);
    }
	
    #if (LCD_BACKLIGHT_CTRL == LCD_BACKLIGHT_BY_GPIO)
    gpio_setDir(GPIO_LCD_BLG_PCTL, GPIO_DIR_OUTPUT);
    #endif

	#if defined(GPIO_GPS_EN)
	gpio_setDir(GPIO_GPS_EN, GPIO_DIR_OUTPUT);
	gpio_setPin(GPIO_GPS_EN);
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

    #if defined(VOLDET_ADC)
    if (adc_open(VOLDET_ADC) != E_OK) {
        DBG_ERR("Can't open ADC channel for battery voltage detection 2\r\n");
        return;
    }

    adc_setChConfig(VOLDET_ADC, ADC_CH_CONFIG_ID_SAMPLE_FREQ, 10000); //10K Hz, sample once about 100 us for CONTINUOUS mode
    adc_setChConfig(VOLDET_ADC, ADC_CH_CONFIG_ID_SAMPLE_MODE, (VOLDET_ADC_MODE) ? ADC_CH_SAMPLEMODE_CONTINUOUS : ADC_CH_SAMPLEMODE_ONESHOT);
    adc_setChConfig(VOLDET_ADC, ADC_CH_CONFIG_ID_INTEN, FALSE);
    #endif

    // Enable adc control logic
    adc_setEnable(TRUE);
    //???
    //this delay is from 650, but it is not necessary for current IC
    //Delay_DelayMs(15); //wait ADC stable  //for pwr on speed up
}

void Dx_InitIO(void)  // Config IO for external device
{

    //IO_InitIntDir();    //initial interrupt destination

    //IO_InitPinmux();    //initial PINMUX config
    IO_InitGPIO();      //initial GPIO pin status
    IO_InitADC();       //initial ADC pin status
    #if 0//defined(_MCU_ENABLE_)
    //Open for DxPower operating
    Dx_Open(Dx_GetObject(DX_CLASS_POWER_EXT));
    MCUCtrl_Open();
    #endif
}

void Dx_UninitIO(void)  // Config IO for external device
{
    gpio_setDir(GPIO_SENSOR1_POWER, GPIO_DIR_OUTPUT);
    gpio_clearPin(GPIO_SENSOR1_POWER);

    gpio_setDir(GPIO_SENSOR1_PWND, GPIO_DIR_OUTPUT);
    gpio_clearPin(GPIO_SENSOR1_PWND);

    gpio_setDir(GPIO_AMP_SHDN, GPIO_DIR_OUTPUT);
    gpio_clearPin(GPIO_AMP_SHDN);

   // gpio_clearPin(GPIO_WIFI_REG_ON);
   // SwTimer_DelayMs(10);
   //gpio_clearPin(GPIO_WIFI_PWR_ON);
   //SwTimer_DelayMs(10);
}

BOOL GPIOMap_IsAccPlugIn(void)
{
	return (gpio_getPin(GPIO_ACC_PLUG) == 1?FALSE:TRUE);
}


BOOL GPIOMap_ReserveDet(void)
{
	return gpio_getPin(GPIO_RESERVE_DET);
}


void GPIOMap_AmpPowerOn(void)
{
	gpio_setDir(GPIO_AMP_SHDN,GPIO_DIR_OUTPUT);
	gpio_setPin(GPIO_AMP_SHDN);
	Delay_DelayMs(1);
	gpio_clearPin(GPIO_AMP_SHDN);
	Delay_DelayMs(1);
	gpio_setPin(GPIO_AMP_SHDN);
	Delay_DelayMs(1);
	gpio_clearPin(GPIO_AMP_SHDN);
	Delay_DelayMs(1);
	gpio_setPin(GPIO_AMP_SHDN);
}

void GPIOMap_AmpPowerOff(void)
{
	gpio_setDir(GPIO_AMP_SHDN,GPIO_DIR_OUTPUT);
	gpio_clearPin(GPIO_AMP_SHDN);
}



