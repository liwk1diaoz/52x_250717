/**
    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.

    @file       DetKey.c
    @ingroup    mIPRJAPKeyIO

    @brief      Scan key, modedial
                Scan key, modedial

    @note       Nothing.

    @date       2017/05/02
*/

/** \addtogroup mIPRJAPKeyIO */
//@{

#include "DxCfg.h"
#include "IOCfg.h"
#include "DxInput.h"
#include "KeyDef.h"
#include "comm/hwclock.h"
#include "comm/hwpower.h"
#if 0
#include "rtc.h"
#include "Delay.h"
#endif
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          DxKey
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////
//#include "Customer.h"



#define DEV_KEY_EVENT_BASE              0x11060000 ///< Key-input device event
typedef enum {
	NVTEVT_KEY_EVT_START        = DEV_KEY_EVENT_BASE, ///< Min value = 0x11060000

	NVTEVT_KEY_STATUS_START     = 0x11060000,    // Status key start
	NVTEVT_KEY_STATUS1      = 0x11060001,
	NVTEVT_KEY_STATUS2      = 0x11060002,
	NVTEVT_KEY_STATUS3      = 0x11060003,
	NVTEVT_KEY_STATUS4      = 0x11060004,
	NVTEVT_KEY_STATUS5      = 0x11060005,
	NVTEVT_KEY_STATUS6      = 0x11060006,
	NVTEVT_KEY_STATUS7      = 0x11060007,
	NVTEVT_KEY_STATUS8      = 0x11060008,
	NVTEVT_KEY_STATUS9      = 0x11060009,
	NVTEVT_KEY_STATUS10     = 0x1106000a,
	/* INSERT NEW EVENT HRER */
	NVTEVT_KEY_STATUS_END       = NVTEVT_KEY_STATUS_START + 0x100 - 1, // Status key end

	NVTEVT_KEY_BUTTON_START     = 0x11060100,    // Button key start

	NVTEVT_KEY_PRESS_START  = 0x11060100,    // Press key start
	NVTEVT_KEY_PRESS    = 0x11060101,
	NVTEVT_KEY_POWER    = 0x11060102,
	NVTEVT_KEY_UP       = 0x11060103,
	NVTEVT_KEY_DOWN     = 0x11060104,
	NVTEVT_KEY_LEFT     = 0x11060105,
	NVTEVT_KEY_RIGHT    = 0x11060106,
	NVTEVT_KEY_ENTER    = 0x11060107,
	NVTEVT_KEY_TELE     = 0x11060108,
	NVTEVT_KEY_WIDE     = 0x11060109,
	NVTEVT_KEY_DEL      = 0x1106010a,
	NVTEVT_KEY_ZOOMIN   = 0x1106010b,
	NVTEVT_KEY_ZOOMOUT  = 0x1106010c,
	NVTEVT_KEY_SHUTTER1 = 0x1106010d,
	NVTEVT_KEY_SHUTTER2 = 0x1106010e,
	NVTEVT_KEY_FACEDETECT = 0x1106010f,
	NVTEVT_KEY_MODE     = 0x11060110,
	NVTEVT_KEY_MOVIE    = 0x11060111,
	NVTEVT_KEY_PLAYBACK = 0x11060112,
	NVTEVT_KEY_MENU     = 0x11060113,
	NVTEVT_KEY_DISPLAY  = 0x11060114,
	NVTEVT_KEY_I        = 0x11060115,
	NVTEVT_KEY_CUSTOM1  = 0x11060116,
	NVTEVT_KEY_CUSTOM2  = 0x11060117,
	NVTEVT_KEY_NEXT     = 0x11060118,
	NVTEVT_KEY_PREV     = 0x11060119,
	NVTEVT_KEY_SELECT   = 0x1106011a,
	NVTEVT_KEY_RC_SHUTTER2       = 0x1106011b,
	NVTEVT_KEY_RC_SHUTTER2_LONG  = 0x1106011c,
	NVTEVT_KEY_RC_MOVIEREC       = 0x1106011d,
	NVTEVT_KEY_RC_MOVIEREC_LONG  = 0x1106011e,
	//#NT#2016/06/23#Niven Cho -begin
	//#NT#Enter calibration by cgi event or command event
	NVTEVT_KEY_CALIBRATION = 0x1106011f,
	//#NT#2016/06/23#Niven Cho -end
	/* INSERT NEW EVENT HRER */
	NVTEVT_KEY_PRESS_END    = NVTEVT_KEY_PRESS_START + 0x100 - 1, // Press key end

	NVTEVT_KEY_CONTINUE_START = 0x11060200, // Continue key start
	NVTEVT_KEY_CONTINUE = 0x11060201, // must = NVTEVT_KEY_PRESS + 0x100,
	NVTEVT_KEY_POWER_CONT = 0x11060202, // must = NVTEVT_KEY_POWER + 0x100,
	NVTEVT_KEY_UP_CONT  = 0x11060203, // must = NVTEVT_KEY_UP + 0x100,
	NVTEVT_KEY_DOWN_CONT = 0x11060204, // must = NVTEVT_KEY_DOWN + 0x100,
	NVTEVT_KEY_LEFT_CONT = 0x11060205, // must = NVTEVT_KEY_LEFT + 0x100,
	NVTEVT_KEY_RIGHT_CONT = 0x11060206, // must = NVTEVT_KEY_RIGHT + 0x100,
	NVTEVT_KEY_CONTINUE_END = NVTEVT_KEY_CONTINUE_START + 0x100 - 1, // Continue key end

	NVTEVT_KEY_RELEASE_START = 0x11060400, // Release key start
	NVTEVT_KEY_RELEASE = 0x11060401, // must = NVTEVT_KEY_PRESS + 0x300,
	NVTEVT_KEY_POWER_REL = 0x11060402, // must = NVTEVT_KEY_POWER + 0x300,
	NVTEVT_KEY_UP_REL  = 0x11060403, // must = NVTEVT_KEY_UP + 0x300,
	NVTEVT_KEY_DOWN_REL = 0x11060404, // must = NVTEVT_KEY_DOWN + 0x300,
	NVTEVT_KEY_LEFT_REL = 0x11060405, // must = NVTEVT_KEY_LEFT + 0x300,
	NVTEVT_KEY_RIGHT_REL = 0x11060406, // must = NVTEVT_KEY_RIGHT + 0x300,
	NVTEVT_KEY_ZOOMRELEASE = 0x1106040b, // must = NVTEVT_KEY_ZOOMIN + 0x300,
	NVTEVT_KEY_SHUTTER1_REL = 0x1106040d, // must = NVTEVT_KEY_SHUTTER1 + 0x300,
	NVTEVT_KEY_SHUTTER2_REL = 0x1106040e, // must = NVTEVT_KEY_SHUTTER2 + 0x300,

	NVTEVT_KEY_MIC = 0x1106040f, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_SOS = 0x11060410, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_RECORD = 0x11060411, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_CAPTRUE = 0x11060412, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_RF_K1 = 0x11060413, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_RF_K2 = 0x11060414, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_RF_K3 = 0x11060415, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_RF_K4 = 0x11060416, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_RF_K5 = 0x11060417, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_RF_PAIR = 0x11060418, // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	//NVTEVT_KEY_RF_K1 = 0x11060412, // must = NVTEVT_KEY_SHUTTER2 + 0x300,



	NVTEVT_KEY_TP_PRESS , // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_TP_RELEASE , // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_TP_MOVE , // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_TP_SLIDE_R , // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_TP_SLIDE_L , // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_TP_SLIDE_U , // must = NVTEVT_KEY_SHUTTER2 + 0x300,
	NVTEVT_KEY_TP_SLIDE_D , // must = NVTEVT_KEY_SHUTTER2 + 0x300,


	NVTEVT_KEY_START_REC ,
	NVTEVT_KEY_STOP_REC ,
	NVTEVT_KEY_WIFI ,
	CLOSE_FROM_WIFI,
	KEY_FTOM_POWER,
	INTO_NEWUI,
	UIWIFI_CHANGEMODE_PREVIEWABLE,

	NVTEVT_KEY_RELEASE_END  = NVTEVT_KEY_RELEASE_START + 0x100 - 1, // Release key end

	NVTEVT_KEY_BUTTON_END       = NVTEVT_KEY_BUTTON_START + 0x800 - 1, // Button key end

	NVTEVT_KEY_EVT_END          = DEV_KEY_EVENT_BASE + 0x1000 - 1, ///< Max value = 0x11060fff
}
NVT_KEY_EVENT;


////////////////////////////////////////////////////////////////////////////////
// ADC related
////////////////////////////////////////////////////////////////////////////////
#if (ADC_KEY == ENABLE)
#define VOLDET_KEY_ADC_LVL1          (251)
#define VOLDET_KEY_ADC_LVL2          (155)
#define VOLDET_KEY_ADC_TH            (460)

#define VOLDET_KEY_LVL_UNKNOWN           0xFFFFFFFF
#define VOLDET_KEY_LVL_0                 0
#define VOLDET_KEY_LVL_1                 1
#define VOLDET_KEY_LVL_2                 2
#define VOLDET_KEY_LVL_3                 3
#define VOLDET_KEY_LVL_4                 4
#define VOLDET_KEY_LVL_5                 5
#endif
#if (ADC_KEY == ENABLE)
/**
  Get ADC key voltage level

  Get  ADC key  2 voltage level.

  @param void
  @return UINT32 key level, refer to VoltageDet.h -> VOLDET_MS_LVL_XXXX
*/

static UINT32 VolDet_GetKey1Level(void)
{
	//UINT32    uiKey2ADC;
	
	//uiKey2ADC = VolDet_GetKey2ADC();
        //printf("gpio_getPin(PWR_KEY_2)=%d\n",gpio_getPin(PWR_KEY_2));
    /*if(GetLcdType_Frontbehind()==0){
        return VOLDET_KEY_LVL_UNKNOWN;
    }*/

	return VOLDET_KEY_LVL_UNKNOWN;
}

/**
  Detect Mode Switch state.

  Detect Mode Switch state.

  @param void
  @return UINT32 Mode Switch state (DSC Mode)
*/
#endif

////////////////////////////////////////////////////////////////////////////////
// GPIO related

//static BOOL g_bIsShutter2Pressed = FALSE;

/**
  Delay between toggle GPIO pin of input/output

  Delay between toggle GPIO pin of input/output

  @param void
  @return void
*/
static void DrvKey_DetKeyDelay(void)
{
	gpio_readData(0);
	gpio_readData(0);
	gpio_readData(0);
	gpio_readData(0);
}

void DrvKey_Init(void)
{
}
/**
  Detect normal key is pressed or not.

  Detect normal key is pressed or not.
  Return key pressed status (refer to KeyDef.h)

  @param void
  @return UINT32
*/

UINT32 DrvKey_DetNormalKey(void)
{
	UINT32 uiKeyCode = 0;

#if (ADC_KEY == ENABLE)
	UINT32 uiKey1Lvl = VolDet_GetKey1Level();
	switch (uiKey1Lvl) {
	case VOLDET_KEY_LVL_UNKNOWN:
	default:
		break;
	case VOLDET_KEY_LVL_1:
		uiKeyCode |= FLGKEY_LEFT;
		break;
	case VOLDET_KEY_LVL_2:
		uiKeyCode |= FLGKEY_RIGHT;
		break;
	}
#endif

	DBG_IND("KEY=%08x\r\n", uiKeyCode);

	DrvKey_DetKeyDelay();
	return uiKeyCode;
}
/**
  Detect power key is pressed or not.

  Detect power key is pressed or not.
  Return key pressed status (refer to KeyDef.h)

  @param void
  @return UINT32
*/

unsigned char First_Power_ON =1;
#define KEY_LONG

unsigned char  LongKeyPressFlag1 =0;

UINT32 LongButtonCount1=0;

unsigned char  LongKeyPressFlag2 =0;

UINT32 LongButtonCount2=0;


#define LONGKEY_COUNT 9
#define longcountpwr 20///23-2s   40  18

BOOL GetPowerKeyPress(void)
{
 	if(hwpower_get_power_key(POWER_ID_PSW1))
		return TRUE;

	return FALSE;
}

UINT32 DrvKey_DetPowerKey(void)
{
    return 0;
}
UINT32 DrvKey_DetStatusKey(DX_STATUS_KEY_GROUP KeyGroup)
{
	return 0;
}
void moto_Beeper_mode(unsigned char mode)
{

}

