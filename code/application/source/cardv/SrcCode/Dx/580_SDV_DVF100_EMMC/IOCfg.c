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

//#include "Customer.h"


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

extern void     SwTimer_DelayMs(UINT32 uiMS);


void IO_InitGPIO(void)
{
    //gpio_setDir(P_GPIO_5, GPIO_DIR_OUTPUT);
    //gpio_clearPin(P_GPIO_5);
    gpio_setDir(GPIO_PWM_MOTOR, GPIO_DIR_OUTPUT);
    gpio_clearPin(GPIO_PWM_MOTOR);
    gpio_setDir(GPIO_BINK_LED, GPIO_DIR_OUTPUT);
    gpio_clearPin(GPIO_BINK_LED);



    gpio_setDir(GPIO_REC_LED, GPIO_DIR_OUTPUT);
    gpio_clearPin(GPIO_REC_LED);

    gpio_setDir(GPIO_WIFI_LED, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_WIFI_LED);

    gpio_setDir(GPIO_PHO_LED, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_PHO_LED);


    gpio_setDir(GPIO_RF_EN, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_RF_EN);


    gpio_setDir(GPIO_CHARGE_LOW, GPIO_DIR_OUTPUT);
    gpio_clearPin(GPIO_CHARGE_LOW);

//    gpio_setDir(GPIO_WIFI_EN, GPIO_DIR_OUTPUT);
  //  gpio_clearPin(GPIO_WIFI_EN);


  //  gpio_setDir(D_GPIO_8, GPIO_DIR_OUTPUT);
  //  gpio_clearPin(D_GPIO_8);///CARD POWER ON
    gpio_setDir(GPIO_SN1_12EN, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_SN1_12EN);

    gpio_setDir(GPIO_SN1_18EN, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_SN1_18EN);
    gpio_setDir(GPIO_SN1_29EN, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_SN1_29EN);

    gpio_setDir(GPIO_SN1_RESET, GPIO_DIR_OUTPUT);
    gpio_clearPin(GPIO_SN1_RESET);
    SwTimer_DelayMs(50);
    gpio_setPin(GPIO_SN1_RESET);

    gpio_setDir(GPIO_AMP_POWER, GPIO_DIR_OUTPUT);
    gpio_setPin(GPIO_AMP_POWER);




}

/**
  Initialize voltage detection

  Initialize voltage detection for battery and flash

  @param void
  @return void
*/

void IO_InitADC(void)
{
#if 1
	if (adc_open(ADC_CH_VOLDET_BATTERY) != E_OK) {
		DBG_ERR("Can't open ADC channel for battery voltage detection\r\n");
		return;
	}

	//650 Range is 250K Hz ~ 2M Hz
	//adc_setConfig(ADC_CONFIG_ID_OCLK_FREQ, 250000); //250K Hz

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

}

void Set_AudioAmplifierShdn_Gpio_State(BOOL state)
{

}

BOOL Get_BlueLed_State(void)
{
    return 0;
}

void Set_BlueLed_State(BOOL state)
{

}
void Set_RedLed_State(BOOL state)
{

}

UINT32 Get_Voltage_Values(void)
{
    //DBGD(adc_readData(ADC_CH_VOLDET_BATTERY));
    return adc_readData(ADC_CH_VOLDET_BATTERY);
}


BOOL Get_BackCar_State(void)
{
    return 0;
}

void GPIO_Open_gps_Power(void)
{
    DBG_DUMP("GPIO_Open_gps_Power\r\n");

}


BOOL DrvMIC_Det(void)
{
	return  0;//(gpio_getPin(GPIO_MIC_DETECT) ==1 ? TRUE : FALSE);
}


BOOL DrvDET_BATChargeFull(void)
{
    return (gpio_getPin(GPIO_CHARGEFULL_DET) == 0 ? TRUE : FALSE);
}

void LED_Turnoffchargebig(void)
{
	gpio_setPin(GPIO_CHARGE_LOW);
}


/******************************/
UINT32 Get_RecLED(void)
{
	return !gpio_getPin(GPIO_REC_LED);
}
void LED_TurnOnRecLED(void)
{
	gpio_setPin(GPIO_REC_LED);
}
void LED_TurnOffRecLED(void)
{
	gpio_clearPin(GPIO_REC_LED);
}
/******************************/
UINT32 Get_PHOLED(void)
{
	return gpio_getPin(GPIO_PHO_LED);
}
void LED_TurnOnPHOLED(void)
{
}
void LED_TurnOffPHOLED(void)
{
	gpio_setPin(GPIO_PHO_LED);
}
/******************************/
UINT32 GET_fWifiLED(void)
{
  return gpio_getPin(GPIO_WIFI_LED);
}
void LED_TurnOnWifiLED(void)
{
}
void LED_TurnOffWifiLED(void)
{
   gpio_setPin(GPIO_WIFI_LED);
}
/******************************/
void LED_TurnOffAllLED(void)
{
	gpio_setPin(GPIO_REC_LED);
	gpio_setPin(GPIO_PHO_LED);
	gpio_setPin(GPIO_WIFI_LED);
}

void LED_TurnOnBT(void)
{
         CHKPNT;
	 gpio_setPin(GPIO_RF_EN);
}

void LED_TurnOffBT(void)
{
         CHKPNT;
	 gpio_clearPin(GPIO_RF_EN);
}


void DrvWiFi_PowerOffDown(void)
{
	gpio_clearPin(GPIO_WIFI_EN);
}
BOOL DriverDetLcd2_Insert(void)
{
       UINT32 LCD2_Det = gpio_getPin(P_GPIO_12);
       //DBG_DUMP("LCD2_Det=%d\n",LCD2_Det);
       if(LCD2_Det)
            return FALSE;

    return TRUE;
}


void LED_TurnOnOffAMP_POWER(char on)
{
	   if(on)
	   	gpio_setPin(GPIO_AMP_POWER);
	   else
	   	gpio_clearPin(GPIO_AMP_POWER);
    	//printf("gpio_getPin(GPIO_AMP_POWER)=%d\r\n",gpio_getPin(GPIO_AMP_POWER));
}




void LED_TurnOnOff_ledwrn(char on)
{
	   if(on)
	   	gpio_setPin(GPIO_BINK_LED);
	   else
	   	gpio_clearPin(GPIO_BINK_LED);
}

unsigned char Gsensor_mode =0;  //0  DA380     1 DM07    2 NULL
void set_gsensor_mode(unsigned char mode )
{
	Gsensor_mode = mode;
}
unsigned char Get_gsensor_mode(void)
{
	return  Gsensor_mode;

}

