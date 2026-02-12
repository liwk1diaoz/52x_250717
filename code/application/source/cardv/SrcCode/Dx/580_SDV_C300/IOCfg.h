/**
    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.

    @file       IOCfg.h
    @ingroup    mIPRJAPCommonIO

    @brief      Header file of IO config
                This file is the header file of IO config

    @note       Nothing.
    @date       2012/09/04
*/

/** \addtogroup mIPRJAPCommonIO */
//@{

#ifndef _IOCFG_H
#define _IOCFG_H

#include "kwrap/type.h"
#include "IOInit.h"
#if 0
#include "pad.h"
#include "top.h"
#include "gpio.h"
#include "adc.h"
#include "pwm.h"
#include "gpio_info.h"
#endif
#include "io/adc.h"
#include "io/gpio.h"
//#include "Drivers/yqpwm.h"

#define USE_VIO DISABLE

#if (USE_VIO == ENABLE)
#define VIO_TV_DET                  1
#define VIO_HDMI_DET                2
#define VIO_LCD2_DET                3
#define VIO_SENSOR2_DET             4
#define VIO_SENSOR1_DET             5
#define VIO_AIN_DET                 6
#define VIO_MAX_ID                  16
extern UINT32 vio_getpin(UINT32 id);
extern void vio_setpin(UINT32 id, UINT32 v);
#endif

//--------------------------------------------------------------------
// ADC common
//--------------------------------------------------------------------

#define GPIO_KEY            DISABLE
#define ADC_KEY             DISABLE

#define VOLDET_ADC_CONT_MODE        ENABLE //!< ENABLE for continuous, DISABLE for one-shot

#if (VOLDET_ADC_CONT_MODE == ENABLE)
#define VOLDET_ADC_MODE             TRUE
#else
#define VOLDET_ADC_MODE             FALSE
#endif

//--------------------------------------------------------------------
// PWM common
//--------------------------------------------------------------------
#define PWR_KEY_2  P_GPIO_2


//--------------------------------------------------------------------
// Display device
//--------------------------------------------------------------------
// LCD communicate
#if 0
#define LCD_COMM_DUMMY              0
#define LCD_COMM_BY_GPIO            1
#define LCD_COMM_BY_SIF             2
#define LCD_COMM_CTRL               LCD_COMM_DUMMY

#if (LCD_COMM_CTRL == LCD_COMM_BY_GPIO)
#define GPIO_LCD_SIF_SEN            L_GPIO_21//S_GPIO_4 //FPGA
#define GPIO_LCD_SIF_SCK            L_GPIO_22//S_GPIO_5 //FPGA
#define GPIO_LCD_SIF_SDA            L_GPIO_23//S_GPIO_6 //FPGA
#endif
#if (LCD_COMM_CTRL == LCD_COMM_BY_SIF)
#define SIF_LCD_CH                  SIF_CH2
#define PAD_LCD_SIF_SEN             PAD_PIN_LGPIO21
#define PAD_LCD_SIF_SCK             PAD_PIN_LGPIO22
#define PAD_LCD_SIF_SDA             PAD_PIN_LGPIO23
#define DIR_LCD_SIF_SEN             PAD_PULLUP
#define DIR_LCD_SIF_SCK             PAD_PULLUP
#define DIR_LCD_SIF_SDA             PAD_PULLUP
#endif
//#define GPIO_LCD_SLEEP              87 //FPGA not support
#endif /* 0 */

#define GPIO_LCD_RESET              L_GPIO_20//L_GPIO_13 //FPGA not support

// LCD backlight
#define LCD_BACKLIGHT_DUMMY         0
#define LCD_BACKLIGHT_BY_GPIO       1
#define LCD_BACKLIGHT_BY_PWM        2
#define LCD_BACKLIGHT_BY_PWRIC      3
#define LCD_BACKLIGHT_CTRL          LCD_BACKLIGHT_BY_PWM //FPGA
#if (LCD_BACKLIGHT_CTRL == LCD_BACKLIGHT_BY_PWM)
//#define GPIO_PWM_LCD_BLG_PCTL       x
//#define PWMID_LCD_BLG_PCTL          PWMID_x
#define PWMID_LCD_BLG_PCTL          YQ_PWMID_1
#elif (LCD_BACKLIGHT_CTRL == LCD_BACKLIGHT_BY_GPIO)
#define GPIO_LCD_BLG_PCTL           P_GPIO_3
#define PAD_LCD_BLG_PCTL            PAD_PIN_PGPIO3
#endif

// LCD2 communicate
#if 0//528 not supprot
#define LCD2_COMM_CTRL              LCD_COMM_DUMMY //FPGA
#if (LCD2_COMM_CTRL == LCD_COMM_BY_GPIO)
#define GPIO_LCD2_SIF_SEN           P_GPIO_23 //FPGA --- NOTE! it will conflict with PIN_SIF_CFG_CH2
#define GPIO_LCD2_SIF_SCK           P_GPIO_25 //FPGA --- NOTE! it will conflict with PIN_SIF_CFG_CH2
#define GPIO_LCD2_SIF_SDA           P_GPIO_26 //FPGA --- NOTE! it will conflict with PIN_SIF_CFG_CH2
#endif
#if (LCD2_COMM_CTRL == LCD_COMM_BY_SIF)
#define SIF_LCD2_CH                 SIF_CHx
#define PAD_LCD2_SIF_SEN            PAD_PIN_PGPIO23
#define PAD_LCD2_SIF_SCK            PAD_PIN_PGPIO25
#define PAD_LCD2_SIF_SDA            PAD_PIN_PGPIO26
#endif
//#define GPIO_LCD2_SLEEP             87 //FPGA not support
//#define GPIO_LCD2_RESET             88 //FPGA not support

// LCD2 backlight
#define LCD2_BACKLIGHT_CTRL         LCD_BACKLIGHT_DUMMY //FPGA
#if (LCD2_BACKLIGHT_CTRL == LCD_BACKLIGHT_BY_PWM)
#define GPIO_PWM_LCD2_BLG_PCTL      x
#define PWMID_LCD2_BLG_PCTL         PWMID_x
#elif (LCD2_BACKLIGHT_CTRL == LCD_BACKLIGHT_BY_GPIO)
#define GPIO_LCD2_BLG_PCTL          x_GPIO_x
#define PAD_LCD2_BLG_PCTL           PAD_PIN_xGPIOx
#endif
#endif /* 0 */

// TV
#if 0//528 not TV
#define TV_DET_DUMMY                0
#define TV_DET_BY_VIO               1
#define TV_DET_BY_GPIO              2
#define TV_DET_BY_ADC               3
#define TV_DET_CTRL                 TV_DET_BY_VIO //FPGA
#if (TV_DET_CTRL == TV_DET_BY_GPIO)
#define GPIO_TV_PLUG                C_GPIO_x
#define PAD_TV_PLUG                 PAD_PIN_CGPIOx
#endif
#if (TV_DET_CTRL == TV_DET_BY_ADC)
#define ADC_CH_TV_PLUG              ADC_CHANNEL_x
#endif
#endif /* 0 */

// HDMI
#if 0//528 not supprot
#define HDMI_DET_DUMMY              0
#define HDMI_DET_BY_VIO             1
#define HDMI_DET_BY_GPIO            2
#define HDMI_DET_BY_ADC             3
#define HDMI_DET_BY_HOTPLUG         4
#define HDMI_DET_CTRL               HDMI_DET_BY_VIO //EVB

// LCD2
#define LCD2_DET_DUMMY              0
#define LCD2_DET_BY_VIO             1
#define LCD2_DET_BY_GPIO            2
#define LCD2_DET_BY_ADC             3
#define LCD2_DET_CTRL               LCD2_DET_BY_VIO //EVB

// AIN
#define AIN_DET_DUMMY               0
#define AIN_DET_BY_VIO              1
#define AIN_DET_BY_GPIO             2
#define AIN_DET_CTRL                AIN_DET_BY_VIO //EVB
#if (AIN_DET_CTRL == AIN_DET_BY_GPIO)
#define GPIO_AIN_PLUG               C_GPIO_x
#define PAD_AIN_PLUG                PAD_PIN_CGPIOx
#endif
#endif /* 0 */

#define GPIO_GPS_EN                 L_GPIO_12//S_GPIO_12

//--------------------------------------------------------------------
// Storage device
//--------------------------------------------------------------------
// CARD
// card power

// SD card detect
#define GPIO_CARD_DETECT            C_GPIO_7
#define PAD_CARD_DETECT             PAD_PIN_CGPIO7
// SD card write protect
//#define GPIO_CARD_WP                P_GPIO_18
//#define PAD_CARD_WP                 PAD_PIN_PGPIO18

extern BOOL     SDIOCardUser_CheckCardInserted(void);
extern BOOL     SDIOCardUser_CheckCardWP(void);
#if defined(_EMBMEM_EMMC_)
extern BOOL     SDIO2CardUser_CheckCardInserted(void);
extern BOOL     SDIO2CardUser_CheckCardWP(void);
#endif

//--------------------------------------------------------------------
// Audio device
//--------------------------------------------------------------------
//Audio
extern BOOL     GPIOMap_DetAudio(void);


//--------------------------------------------------------------------
// Input device
//--------------------------------------------------------------------
// Key defined (650 FPGA KEYPAD)
//
// <Layout>
//--------------------------//   //--------------------------//
//  SW3   SW1   SW4   SW2   //   //  kzi   kzo   ks1   ks2   //
//                          //   //                          //
//           SW7            //   //           ku             //
//                          //   //                          //
//      SW5  SW6  SW8       //   //      kl   ke   kr        //
//                          //   //                          //
//           SW10           //   //           kd             //
//                          //   //                          //
//    SW12   SW11   SW9     //   //    km     kmd    kmp     //
//                          //   //                          //
//--------------------------//   //--------------------------//
//
// <Mapping>  SW1~SW12 = MC22~MC31,DCPIO4,5 = C_GPIO_22~31,DCPIO4,5 (Roy)
// <Active>   Low (Roy)
//
#if (GPIO_KEY == ENABLE)
#define GPIO_KEY_ZOOMOUT            x_GPIO_x           // Key Zoom Out (kzo)
#define PAD_KEY_ZOOMOUT             PAD_PIN_xGPIOx
#define GPIO_KEY_SHUTTER2           x_GPIO_x           // Key Shutter2 (ks2)
#define PAD_KEY_SHUTTER2            PAD_PIN_xGPIOx
#define GPIO_KEY_ZOOMIN             x_GPIO_x           // Key Zoom In (kzi)
#define PAD_KEY_ZOOMIN              PAD_PIN_xGPIOx
#define GPIO_KEY_SHUTTER1           x_GPIO_x           // Key Shutter1 (ks1)
#define PAD_KEY_SHUTTER1            PAD_PIN_xGPIOx
#define GPIO_KEY_LEFT               x_GPIO_x           // Key Left (kl)
#define PAD_KEY_LEFT                PAD_PIN_xGPIOx
#define GPIO_KEY_ENTER              x_GPIO_x           // Key OK (ke)
#define PAD_KEY_ENTER               PAD_PIN_xGPIOx
#define GPIO_KEY_UP                 x_GPIO_x           // Key Up (ku)
#define PAD_KEY_UP                  PAD_PIN_xGPIOx
#define GPIO_KEY_RIGHT              x_GPIO_x           // Key Right (kr)
#define PAD_KEY_RIGHT               PAD_PIN_xGPIOx
#define GPIO_KEY_PLAYBACK           x_GPIO_x           // Key Playback (kmp)
#define PAD_KEY_PLAYBACK            PAD_PIN_xGPIOx
#define GPIO_KEY_DOWN               x_GPIO_x           // Key Down (kd)
#define PAD_KEY_DOWN                PAD_PIN_xGPIOx
#define GPIO_KEY_MODE               x_GPIO_x            // Key Mode (kmd)
#define PAD_KEY_MODE                PAD_PIN_xGPIOx
#define GPIO_KEY_MENU               x_GPIO_x            // Key Menu (km)
#define PAD_KEY_MENU                PAD_PIN_xGPIOx
#endif /* 0 */

#if (ADC_KEY == ENABLE)
//#define ADC_CH_VOLDET_MS1           ADC_CHANNEL_1
//#define ADC_CH_VOLDET_MS2           ADC_CHANNEL_0
#define ADC_CH_VOLDET_KEY1          ADC_CHANNEL_0
#endif

//Touch panel controller
#if 0
//#define GPIO_TP_BUSY                (2 | GPIO_IS_DGPIO)  //HW TS_INT =DGPIO-02
#define GPIO_TP_PENIRQ              GPIO_INT_30 //HW TS_PENIRQ =DGPIO-12
#define GPIO_TP_DOUT                x_GPIO_x //P_GPIO_58 //HW TP_DI  =SBDAT0/AGPIO-26   (FW output = HW input)
#define GPIO_TP_DIN                 x_GPIO_x //HW TS_DO  =DGPIO-01  (FW input = HW output)
#define GPIO_TP_DCLK                x_GPIO_x//P_GPIO_57 //HW TS_CLK =SBCK0/AGPIO-25
//#define GPIO_TP_CS                  28 //HW TS_CS  =SBCS1/AGPIO-28  //SIF channel 1
#define GPIO_TP_CS                  x_GPIO_x//P_GPIO_56 //HW TS_CS  =SBCS0/AGPIO-27  //SIF channel 0
//#define PAD_TP_BUSY                 PAD_PIN_xGPIOx
#define PAD_TP_PENIRQ               PAD_PIN_xGPIOx
#define PAD_TP_DOUT                 PAD_PIN_xGPIOx
#define PAD_TP_DIN                  PAD_PIN_xGPIOx
#define PAD_TP_DCLK                 PAD_PIN_xGPIOx
//#define PAD_TP_CS                   PAD_PIN_xGPIOx  //SIF channel 1
#define PAD_TP_CS                   PAD_PIN_xGPIOx  //SIF channel 0
#endif /* 0 */

#define GPIO_TP_RESET              L_GPIO_12 //HW TS_PENIRQ =DGPIO-12

//G-Sensor
/*
#define GPIO_GS_INT1                (3 | GPIO_IS_DGPIO)
#define GPIO_GS_INT2                (4 | GPIO_IS_DGPIO)
#define PAD_GS_INT1                 PAD_PIN_DGPIO3
#define PAD_GS_INT2                 PAD_PIN_DGPIO4
*/
extern UINT32   GPIOMap_DetKey(void);
extern UINT32   GPIOMap_DetModeDial(void);
extern BOOL     GPIOMap_DetShutter1(void);
extern BOOL     GPIOMap_DetMacro(void);
extern BOOL     GPIOMap_DetPoweroff(void);

//--------------------------------------------------------------------
// Output device
//--------------------------------------------------------------------
// LED
#define LED_RED_DUMMY               0
#define LED_RED_BY_GPIO             1
#define LED_RED_BY_MCU              3
#define LED_RED_CTRL                LED_RED_DUMMY
#define GPIO_RED_LED                C_GPIO_4//L_GPIO_17//(1 | GPIO_IS_DGPIO)  //FPGA
#define PAD_RED_LED                 PAD_PIN_CGPIO4

#define LED_BLUE_DUMMY               0
#define LED_BLUE_BY_GPIO             1
#define LED_BLUE_BY_MCU              3
#define LED_BLUE_CTRL                LED_BLUE_DUMMY
#define GPIO_BLUE_LED               C_GPIO_6 //L_GPIO_14//(1 | GPIO_IS_DGPIO)  //FPGA
#define PAD_BLUE_LED                PAD_PIN_CGPIO6 //PAD_PIN_LGPIO14

#define LED_GREEN_DUMMY             0
#define LED_GREEN_BY_GPIO           1
#define LED_GREEN_BY_MCU            3
#define LED_GREEN_CTRL              LED_GREEN_DUMMY
#define GPIO_GREEN_LED              C_GPIO_5  //FPGA
#define PAD_GREEN_LED               PAD_PIN_CGPIO5

#define LED_FOCUS_DUMMY             0
#define LED_FOCUS_BY_GPIO           1
#define LED_FOCUS_BY_PWM            2 //use PWM to control LED brightness
#define LED_FOCUS_BY_MCU            3

#define LED_FOCUS_CTRL              LED_RED_DUMMY
#define GPIO_FOCUS_LED              x_GPIO_x  //FPGA
#define PAD_FOCUS_LED               PAD_PIN_xGPIOx
#define PWMID_FOCUS_LED             PWMID_x //FPGA


//--------------------------------------------------------------------
// Power device
//--------------------------------------------------------------------
//Battery
#define ADC_CH_VOLDET_BATTERY       ADC_CHANNEL_0
extern char    *VolDet_GetStatusString(void);



//--------------------------------------------------------------------
// Sensor device
//--------------------------------------------------------------------
// CCD
/*
typedef enum
{
    CCD_PWR_STATUS_OFF = 0,
    CCD_PWR_STATUS_ON,
    CCD_PWR_STATUS_SLEEP,
    CCD_PWR_STATUS_MAXIDX
}CCD_PWR_STATUS;
*/
/*
#define GPIO_CCDTG_VH               L_GPIO_20
#define GPIO_CCDTG_VL               L_GPIO_19
//#define GPIO_CCDTG_VH_2             50
#define GPIO_CCDTG_RST              C_GPIO_22
//#define GPIO_CCDTG_HDRV_EN          88
#define SIF_SENSOR_TG               SIF_CH2
#define SIF_SENSOR_AFE              SIF_CH3
*/
extern void     GPIOMap_TurnOnCCDPower(void);
extern void     GPIOMap_TurnOffCCDPower(void);
extern UINT8    GPIOMap_IsCCDPowerOn(void);
extern void     GPIOMap_SleepCCDPower(void);
extern void     GPIOMap_WakeUpCCDPower(void);

//--------------------------------------------------------------------
// Storbe device
//--------------------------------------------------------------------
// STROBE
/*
#define GPIO_FLASH_CHARGE           C_GPIO_21
#define PAD_FL_CHARGE               PAD_PIN_CGPIO21
#define GPIO_FLASH_RDY              P_GPIO_12
#define PAD_FL_RDY                  PAD_PIN_PGPIO12
#define GPIO_FLASH_TRIGGER          D_GPIO_7
#define PAD_FL_TRIGGER              PAD_PIN_DGPIO7
*/

//--------------------------------------------------------------------
// Lens device
//--------------------------------------------------------------------
// BCS 2.7 ~ 12 mm lens + SANYO LV8044 motor IC
#define GPIO_LENS_RESET             0              // output

#define GPIO_LENS_ZOOM_INTR         0              // input
#define PAD_LENS_ZOOM_INTR          0

#define GPIO_LENS_FOCUS_INTR        0              // input
#define PAD_LENS_FOCUS_INTR         0

#define GPIO_LENS_IR_CTRL0          x_GPIO_x       // output
#define GPIO_LENS_IR_CTRL1          x_GPIO_x       // output
#define PAD_LENS_IR_CTRL0           PAD_PIN_xGPIOx
#define PAD_LENS_IR_CTRL1           PAD_PIN_xGPIOx

#define GPIO_SN1_12EN		L_GPIO_16
#define GPIO_SN1_18EN		L_GPIO_15
#define GPIO_SN1_29EN		L_GPIO_14
#define GPIO_SN1_RESET		S_GPIO_4





#define GPIO_MIC_DETECT            P_GPIO_9
#define PAD_MIC_DETECT             PAD_PIN_PGPIO9

#define GPIO_CHARGEFULL_DET      D_GPIO_0
#define PAD_GPIO_CHARGEFULL_DET      PAD_PIN_DGPIO0


#define GPIO_TP_INT_SANQIAO                 C_GPIO_9// for ILI9341 + touch panel TEST!!!



#define GPIO_REC_LED                P_GPIO_7             //LED2
#define PAD_REC_LED                 PAD_PIN_PGPIO7


#define GPIO_WIFI_LED                P_GPIO_8//D_GPIO_1                //LED2
#define PAD_WIFI_LED                 PAD_PIN_PGPIO8//PAD_PIN_DGPIO1

#define GPIO_RF_EN                P_GPIO_6
#define PAD_RF_EN                 PAD_PIN_PGPIO6


//#define GPIO_CHARGE_A               P_GPIO_12 //NULL--GPIO,NO USE
//#define PAD_CHARGE_A                 PAD_PIN_PGPIO12

#define GPIO_WIFI_EN              D_GPIO_10


#endif

//@}
