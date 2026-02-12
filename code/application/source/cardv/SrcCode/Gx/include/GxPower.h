/**
    System power control module.

    This module handles the power control, including battery level detection, auto power off, auto sleep.
    It also control the battery charge flow.

    @file       GxPower.h
    @ingroup    mILibPowerCtrl

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _GXPOWER_H
#define _GXPOWER_H

#include "GxCommon.h"
#include "DxType.h"
#include "Dx.h"
#include <time.h>

/**
    @addtogroup mILibPowerCtrl
*/
//@{

////////////////////////////////////////////////////////////////////////////////

/**
     Gx Power time ID
*/
typedef enum _GXTIME_ID {

	GXTIME_HWRESET = 0x0102,                   ///<  hw-reset time, use tm struct (hour, min, sec)
	GXTIME_SWRESET = 0x0103,                   ///<  sw-reset keep-alive period (sec)
}
GXTIME_ID;

extern UINT32   GxPower_GetTime(GXTIME_ID tid, struct tm* p_ctv);
extern void     GxPower_SetTime(GXTIME_ID tid, struct tm ctv, UINT32 param);


/**
     Calculate maximum days number of given (year, month).

     Calculate maximum days number of given (year, month).

     @param[in] year      Year.
     @param[in] month     Month

     @return Days number.
*/
extern INT32 GxTime_CalcMonthDays(INT32 year, INT32 month);

/**
     Convert date-time value to days number.

     Convert date-time value to days number.

     @param[in] ctv     Date time value.

     @return Days number. [second precison]
*/
extern UINT32 GxTime_Time2Days(struct tm ctv);

/**
     Convert days number to date-time value.

     Convert days number to date-time value.

     @param[in] days    Days number.

     @return Date time value. [day precison]
*/
extern struct tm GxTime_Days2Time(UINT32 days);

/**
     Calculate sum of date-time value and another differece

     Calculate sum of date-time value and another differece

     @param[in] ctv     Date time value.
     @param[in] diff    Difference (date time value).

     @return Sum (date time value). [second precison]
*/
extern struct tm GxTime_AddTime(struct tm ctv, struct tm diff);

/**
    Check if leap year

    This function is used to check if the specified year is a leap year.

    @param[in] uiYear   The year you want to check (0~0xFFFFFFFF)
    @return
        - @b TRUE   : Is a leap year
        - @b FALSE  : Isn't a leap year
*/
extern BOOL GxTime_IsLeapYear(UINT32 year);


/**
     Gx Power key ID
*/
typedef enum _GXPWR_KEY {

	//time and alaram
	GXPWR_KEY_POWER1 = 0x0000,                     ///<  power key 1: write 0xf0: clear auto-power-off timer, 0xff: power off immediately
	//GXPWR_KEY_POWER2 = 0x0001,                     ///<  power key 2:
	//GXPWR_KEY_POWER3 = 0x0002,                     ///<  power key 3:
	//GXPWR_KEY_POWER4 = 0x0003,                     ///<  power key 4:
	GXPWR_KEY_HWRESET = 0x0005,                    ///<  hw-reset key: write 0x00: disable, 0x01: enable, 0xff: reset immediately / read: is enable?
	GXPWR_KEY_SWRESET = 0x0006,                    ///<  sw-reset key: write 0x00: disable, 0x01: enable, 0xf0: kick: 0xff: reset immediately / read: is enable?
}
GXPWR_KEY;

/**
     Gx Power control ID
*/
typedef enum _GXPWR_CTRL {

	//power-on
	GXPWR_CTRL_POWERON_DETECT_EN = 0x1000,         ///<  power-on detect enable (WO), ex: PIR sensor
	GXPWR_CTRL_POWERON_SRC= 0x1001,                ///<  after power-on, get power source type (RO)
	GXPWR_CTRL_POWERON_LOST= 0x1002,               ///<  after power-on, get date-time power lost status (RO)
	//auto-power-off
	GXPWR_CTRL_AUTOPOWEROFF_EN = 0x1030,           ///<  write 0x00: disable, 0x01: enable, 0xff: reset
	GXPWR_CTRL_AUTOPOWEROFF_TIME = 0x1031,         ///<  auto power off system time value set, get

	//power-save
	GXPWR_CTRL_SLEEP_LEVEL = 0x2000,               ///<  current sleep level 0~3 (RO)
	//auto-power-save
	GXPWR_CTRL_AUTOSLEEP_EN = 0x2001,              ///<  write 0x00: disable, 0x01: enable, 0xff: reset
	GXPWR_CTRL_AUTOSLEEP_TIME_L = 0x2003,          ///<  low level sleep mode time value set, get
	GXPWR_CTRL_AUTOSLEEP_TIME = 0x2004,            ///<  normal level sleep mode time value set, get
	GXPWR_CTRL_AUTOSLEEP_TIME_D = 0x2005,          ///<  deep level sleep mode time value set, get

	//power-detect
	GXPWR_CTRL_BATTERY_DETECT_EN = 0x3000,         ///<  write 0x00: disable, 0x01: enable
	GXPWR_CTRL_BATTERY_VALUE = 0x3001,             ///<  current battery native value get (RO)
	GXPWR_CTRL_BATTERY_LEVEL = 0x3002,             ///<  current battery level get (RO)
	GXPWR_CTRL_BATTERY_IS_INSERT = 0x3003,         ///<  battery is insert or not (RO)
	GXPWR_CTRL_BATTERY_IS_DEAD = 0x3004,           ///<  battery is dead or not (RO)
	GXPWR_CTRL_BATTERY_POWERON_CHECK_EN = 0x3005,  ///<  power on battery "dummy load" check, default enable, write 0 to disable
	GXPWR_CTRL_BATTERY_FAIL_POWEROFF_EN = 0x3006,  ///<  if "dummy load" check battery fail need to power off (RO)
	//power-charge
	GXPWR_CTRL_BATTERY_CHARGE_EN = 0x3010,         ///<  write 0x00: disable, 0x01: enable
	GXPWR_CTRL_BATTERY_CHARGE_CURRENT = 0x3011,    ///<  battery charge current set ,get(RW)
	GXPWR_CTRL_BATTERY_CHARGE_OK = 0x3012,         ///<  battery charge ok status get (RO)
	
	ENUM_DUMMY4WORD(GXPWR_CTRL)
} GXPWR_CTRL;

/**
     @name Battery charge current value
*/
//@{
#define BATT_CHARGE_CURRENT_LOW         0          ///<  charge current low
#define BATT_CHARGE_CURRENT_MEDIUM      1          ///<  charge current medimum
#define BATT_CHARGE_CURRENT_HIGH        2          ///<  charge current high
//@}


/**
     @name Power Callback event
*/
//@{
#define POWER_CB_BATT_EMPTY             0          ///<  battery is emtpy
#define POWER_CB_BATT_CHG               1          ///<  battery level changed
#define POWER_CB_SLEEP_ENTER_L          2          ///<  enter low level sleep mode
#define POWER_CB_SLEEP_ENTER            3          ///<  enter normal level sleep mode
#define POWER_CB_SLEEP_ENTER_D          4          ///<  enter deep level sleep mode
#define POWER_CB_SLEEP_RESET            5          ///<  reset auto sleep count down timer
#define POWER_CB_POWEROFF               6          ///<  auto power off time is up
#define POWER_CB_BATT_OVERHEAT          7          ///<  battery temperature is overheat
#define POWER_CB_CHARGE_OK              8          ///<  battery charge is ok (charge full)
#define POWER_CB_CHARGE_SUSPEND         9          ///<  temperature is too high suspend charge
#define POWER_CB_CHARGE_RESUME          10         ///<  resume charge after temperature becomes normal
//@}

/**
     GxPower power on init.

     @note User must register callback funcion then can receive the Driver Power Callback event DRVPWR_CB_XXX.

     @param[in] fpDxPowerCB: the callback function pointer.

*/
extern UINT32   GxPower_Open(DX_HANDLE hPwr, GX_CALLBACK_PTR DxPower_CB);
extern UINT32   GxPower_Close(void);

extern UINT32   GxPower_GetPwrKey(GXPWR_KEY kid);
extern void     GxPower_SetPwrKey(GXPWR_KEY kid, UINT32 key);


/**
    Detect battery voltage level.

    Detect battery voltage level and store it in uiBatteryLvl (static variable)
    If the battery voltage is VOLDET_BATTERY_LVL_EMPTY, then callback event POWER_CB_BATT_EMPTY to power off the system.
*/
extern void     GxPower_DetBattery(void);

/**
    Detect auto power off

    Detect auto power off.
    If the auto power off counter reach auto power off setting, then callback event POWER_CB_POWEROFF to do power off.
*/
extern void     GxPower_DetAutoPoweroff(void);

/**
    Get GxPower setting value.

    @param[in] PowerCtrl:  setting ID
    @return the setting value.

*/
extern UINT32   GxPower_GetControl(GXPWR_CTRL PowerCtrl);

/**
    Set GxPower setting value.

    @param[in] PowerCtrl:  setting ID
    @param[in] value:  the setting value

*/
extern void     GxPower_SetControl(GXPWR_CTRL PowerCtrl, UINT32 value);

/**
    Do the dummy loading.

    This API is just mapping to DrvPower_DummyLoad(). User need to implement the function DrvPower_DummyLoad() to do the dummy load.\n

    @note User must call this API before calling GxPower_Open() when system power on.

    @return  TRUE:  battery status is ok can power on.
             FALSE: battery status is not ok can't power on.
*/
extern UINT32   GxPower_DummyLoad(void);

/**
     Register callback function.

     @note User must register callback funcion then can receive the Power Callback event POWER_CB_XXX.

     @param[in] fpPowerCB: the callback function pointer.
*/
extern void     GxPower_RegCB(GX_CALLBACK_PTR fpPowerCB);

/**
     Detect if enter sleep mode.

     If the sleep mode counter reachs setting, then callback event to enter sleep mode.\n
     The sleep mode have 3 levels, each level has separate time setting and separate callback event.

*/
extern void     GxPower_DetSleep(void);

/**
     Detect if the charge status is ok.

     Detect battery charge status and determine to continue charge or stop charging.\n
     It will also detect the battery temperature to check if the battery is overheat and need to suspend charge.

*/
extern void     GxPower_DetCharge(void);

/**
    Install SxCmd.

    Install uart command to SxCmd.
*/
extern void     GxPower_InstallCmd(void);

extern void     GxPower_InstallID(void);

//@}
#endif //_GXPOWER_H
