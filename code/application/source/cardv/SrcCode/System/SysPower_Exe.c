/*
    System Power Callback

    System Callback for Power Module.

    @file       SysPower_Exe.c
    @ingroup    mIPRJSYS

    @note

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

////////////////////////////////////////////////////////////////////////////////

#include "PrjInc.h"
#include "GxPower.h"
#include "PowerDef.h"
#include "GxTime.h"
#include "GxUSB.h"

#if 0
#include "SysCommon.h"
#include "AppCommon.h"
////////////////////////////////////////////////////////////////////////////////
#include "UIFrameworkExt.h"
#include "UICommon.h"
#include "AppLib.h"
#include "GxInput.h"
#include "DxPower.h"
#include "DxLens.h"
#include "GxFlash.h"
#include "GxPower.h"
#include "DbgUtApi.h"

#include "UsbDevDef.h"

//global debug level: PRJ_DBG_LVL
#include "PrjCfg.h"
#endif

#include "GxInput.h"


//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysPwrExe
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>


int SX_TIMER_DET_SLEEP_ID = -1;
int SX_TIMER_DET_AUTOPOWEROFF_ID = -1;
int SX_TIMER_DET_BATT_ID = -1;
int SX_TIMER_DET_CHARGE_ID = -1;
int SX_TIMER_DET_SHOWADC = -1;
int SX_TIMER_DET_BATTINS_ID = -1;

void UI_DetAutoSleep(void);
void UI_DetAutoPoweroff(void);
void UI_DetBattery(void);
void UI_DetBatteryInsert(void);
void System_DetShowADC(void);


#if (AUTOSLEEP_FUNCTION == ENABLE)
SX_TIMER_ITEM(GxPower_DetSleep, UI_DetAutoSleep, 50, TRUE)
#endif
#if (AUTOPWROFF_FUNCTION == ENABLE)
SX_TIMER_ITEM(GxPower_DetAutoPoweroff, UI_DetAutoPoweroff, 50, TRUE)
#endif
#if (PWRLEVEL_FUNCTION == ENABLE)
SX_TIMER_ITEM(GxPower_DetBattery, UI_DetBattery, 25, TRUE)
#endif
#if (USB_CHARGE_FUNCTION == ENABLE)
SX_TIMER_ITEM(GxPower_DetCharge, GxPower_DetCharge, 50, TRUE)
#endif
#if (SHOW_ADC_ON_SCREEN == ENABLE)
SX_TIMER_ITEM(System_DetShowADC, System_DetShowADC, 15, FALSE)
#endif
#if (USB_CHARGE_FUNCTION == ENABLE)
SX_TIMER_ITEM(GxPower_DetBatteryIns, UI_DetBatteryInsert, 25, TRUE)
#endif

UINT32 DxPower_CB(UINT32 event, UINT32 param1, UINT32 param2)
{
#if LENS_FUNCTION
	switch (event) {
	case DRVPWR_CB_IS_LENS_MOVING:
		return GxFlash_GetData(FLASH_SUSPEND_CALLER_CNT);
	case DRVPWR_CB_DUMMY_LOAD_START: {
			PMOTOR_TAB pMotorObj = motor_getObject();
			PLENS_DEVICE_OBJ pLensDevice = DrvLens_GetDevice();


			//first time,need to init motor dirver gpio
			pMotorObj->init(pLensDevice);
			pMotorObj->setSignal(MD_SIGNAL_STANDBY, ON);
			//Open shutter
			pMotorObj->shutter_setState(MOTOR_SHUTTER_DUMMYLOAD, SHUTTER_DUMMYLOAD_START);
		}
		return 0;
	case DRVPWR_CB_DUMMY_LOAD_END: {
			PMOTOR_TAB pMotorObj = motor_getObject();
			pMotorObj->shutter_setState(MOTOR_SHUTTER_DUMMYLOAD, SHUTTER_DUMMYLOAD_END);
			pMotorObj->setSignal(MD_SIGNAL_STANDBY, OFF);
		}
		return 0;
	default:
		DBG_ERR("event(%d)\r\n", event);
		return 0;

	}
#else
	return 0;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//SW Reset : implement by WDT reset
#define SIGN_WDTC    0x52544457
static BOOL bAutoSWReset = FALSE;
UINT32 gParamSWReset = 0;

void System_EnableSWReset(UINT32 param)
{
	gParamSWReset = param;
	bAutoSWReset = TRUE;
}
void System_CancelSWReset(void)
{
	gParamSWReset = 0;
	bAutoSWReset = FALSE;
}
UINT32 System_GetSWResetParam(void)
{
	return gParamSWReset;
}

///////////////////////////////////////////////////////////////////////////////
//HW Reset : implement by RTC power alarm reset
static BOOL bAutoHWReset = FALSE;
UINT32 gParamHWReset = 0;
struct tm AutoPowerOn_dDateTime = {0};

void System_EnableHWResetByAlarmTime(UINT32 param, INT32 hour, INT32 min, INT32 sec)
{
	struct tm dDateTime = {0};
	gParamHWReset = param;
	bAutoHWReset = TRUE;
	dDateTime.tm_hour = hour;
	dDateTime.tm_min = min;
	dDateTime.tm_sec = sec;
	AutoPowerOn_dDateTime = dDateTime;
}
void System_EnableHWReset(UINT32 param)
{
	struct tm dDateTime = {0};
	gParamHWReset = param;
	bAutoHWReset = TRUE;
	dDateTime.tm_hour = 0;
	dDateTime.tm_min = 0;
	dDateTime.tm_sec = 5;
	AutoPowerOn_dDateTime = dDateTime;
}
void System_CancelHWReset(void)
{
	struct tm dDateTime = {0};
	bAutoHWReset = FALSE;
	AutoPowerOn_dDateTime = dDateTime;
}

static UINT32 gPwrSrc = 0;

UINT32 System_GetPowerOnSource(void)
{
	return gPwrSrc;
}

///////////////////////////////////////////////////////////////////////////////

void System_OnPowerPreInit(void)
{
	// register DxPower Callback function
	//DrvPower_RegCB(DxPower_CB);
	DX_HANDLE power_obj;
	UINT32 PwLst = 0;
	UINT32 PwSrc = 0;

	power_obj = Dx_GetObject(DX_CLASS_POWER_EXT);
	if (!power_obj) {
		DBG_ERR("get power object error!\r\n");
		return;

	}
	GxPower_Open(power_obj, (GX_CALLBACK_PTR)DxPower_CB);
	
	PwLst = GxPower_GetControl(GXPWR_CTRL_POWERON_LOST);   //"firs time power-on" or "lost power of Gold capacitor"
	PwSrc = GxPower_GetControl(GXPWR_CTRL_POWERON_SRC);
	
	//dump power status
	DBG_DUMP("PwrLost = %d, PwrSrc = %d\r\n", PwLst, PwSrc);
	//if(PwLst)
	{
		//DBG_DUMP("^YPwrLost = 1 << should be first time power on, or power lost of Gold capacitor >>\r\n");
		//to notify user to configure current date-time!
	}
	gPwrSrc = PwSrc;

	if (GxKey_GetData(GXKEY_NORMAL_KEY) & FLGKEY_UP) {
		GxPower_SetControl(GXPWR_CTRL_BATTERY_POWERON_CHECK_EN, FALSE); //ignore check
	} else {
		GxPower_SetControl(GXPWR_CTRL_BATTERY_POWERON_CHECK_EN, TRUE);
	}
	GxPower_DummyLoad(); //Do Dummy Load

#if 0
	HwClock_EnablePowerAlarm(FALSE);
#endif

	//If press up key, enter "Battery Calibration" mode.
#if 0
	if (GxKey_GetData(GXKEY_NORMAL_KEY) & FLGKEY_UP)
#else
	if (!GxPower_GetControl(GXPWR_CTRL_BATTERY_POWERON_CHECK_EN))
#endif
	{
		//"battery calibration" path
		DBG_IND("USER force to ignore low-battery check.\r\n");

		GxPower_SetControl(GXPWR_CTRL_BATTERY_DETECT_EN, FALSE);
	} else {
		//normal path
		DBG_IND("Start low-battery check (dummyload).\r\n");

#if (USBINSERT_FUNCTION == ENABLE)
		//if under low-battery condition
		if (!GxPower_DummyLoad() && (!GxUSB_GetIsUSBPlug())) {
			DBG_FATAL("low power\r\n");
			System_SetState(SYS_STATE_POWERON, SYS_POWERON_LOWPOWER);
			System_SetState(SYS_STATE_POWEROFF, SYS_POWEROFF_LOWPOWER);
		}
#endif
		GxPower_SetControl(GXPWR_CTRL_BATTERY_DETECT_EN, TRUE);
	}

}

void System_OnPowerPostExit(void)
{
	if (bAutoSWReset) {
		//power reset by sw
		DBG_MSG("Enable SW Reset\r\n");
		DBG_MSG("Reset Param = %08x\r\n", gParamSWReset);
		DBG_MSG("\r\n");
		GxPower_SetPwrKey(GXPWR_KEY_SWRESET, 0xff);
		GxPower_Close();
	} else if (bAutoHWReset) {
		//power reset by hw
		//UINT32 t1=0;
		// Set Power on alarm
		struct tm cDateTime;
		struct tm dDateTime;
		DBG_MSG("Enable Auto-PowerOn-Alarm\r\n");
		DBG_MSG("Reset Param = %08x\r\n", gParamHWReset);
		DBG_MSG("\r\n");
		
		//wait for last time write setting to RTC is completed. (EX: date, time, key)
		GxTime_GetTime(&cDateTime);
		dDateTime = AutoPowerOn_dDateTime;
		//DBG_MSG("Current = \r\n");
		//DBG_MSG(" date = %02ld:%02ld:%02ld\r\n",cDateTime.tm_year, cDateTime.tm_mon, cDateTime.tm_mday);
		//DBG_MSG(" time = %02ld:%02ld:%02ld\r\n",cDateTime.tm_hour, cDateTime.tm_min, cDateTime.tm_sec);
		cDateTime = GxTime_AddTime(cDateTime, dDateTime);
		GxPower_SetTime(GXTIME_HWRESET, cDateTime, 0);
		//DBG_MSG("Set Auto-PowerOn-Alarm\r\n");
		//DBG_MSG(" date = %02ld:%02ld:%02ld\r\n",cDateTime.tm_year, cDateTime.tm_mon, cDateTime.tm_mday);
		//DBG_MSG(" time = %02ld:%02ld:%02ld\r\n",cDateTime.tm_hour, cDateTime.tm_min, cDateTime.tm_sec);
		GxPower_SetPwrKey(GXPWR_KEY_HWRESET, 1);
		//t1=Perf_GetCurrent();
		//DBG_MSG("dT = %d\r\n", (Perf_GetCurrent()-t1)/1000);
		//power off
		GxPower_SetPwrKey(GXPWR_KEY_POWER1, 0xff);
		GxPower_Close();
	} else {
		//power off
		GxPower_SetPwrKey(GXPWR_KEY_POWER1, 0xff);
		GxPower_Close();
	}
}

extern void Power_CB(UINT32 event, UINT32 param1, UINT32 param2);

void System_OnPowerInit(void)
{
	//PHASE-1 : Init & Open Drv or DrvExt
	{
		GxPower_RegCB(Power_CB);     //Register CB function of GxPower
	}
	//PHASE-2 : Init & Open Lib or LibExt
	{
		//1.設定init值
		//2.設定CB值,

		//3.註冊SxJob服務 ---------> System Job
		//4.註冊SxTimer服務 ---------> Detect Job
#if (AUTOSLEEP_FUNCTION == ENABLE)
		SX_TIMER_DET_SLEEP_ID = SxTimer_AddItem(&Timer_GxPower_DetSleep);
#endif
#if (AUTOPWROFF_FUNCTION == ENABLE)
		SX_TIMER_DET_AUTOPOWEROFF_ID = SxTimer_AddItem(&Timer_GxPower_DetAutoPoweroff);
#endif
#if (PWRLEVEL_FUNCTION == ENABLE)
		SX_TIMER_DET_BATT_ID = SxTimer_AddItem(&Timer_GxPower_DetBattery);
#endif
#if (USB_CHARGE_FUNCTION == ENABLE)
		SX_TIMER_DET_CHARGE_ID = SxTimer_AddItem(&Timer_GxPower_DetCharge);
#endif
#if (SHOW_ADC_ON_SCREEN == ENABLE)
		SX_TIMER_DET_SHOWADC = SxTimer_AddItem(&Timer_System_DetShowADC);
#endif
#if (USB_CHARGE_FUNCTION == ENABLE)
		SX_TIMER_DET_BATTINS_ID = SxTimer_AddItem(&Timer_GxPower_DetBatteryIns);
#endif
	}
}

void System_OnPowerExit(void)
{
	//PHASE-2 : Close Lib or LibExt
	{
	}
	//PHASE-1 : Close Drv or DrvExt
	{
	}
}

///////////////////////////////////////////////////////////////////////////////

#if (AUTOSLEEP_FUNCTION == ENABLE)
void UI_DetAutoSleep(void)
{
	if ((!UI_IsForceLock()) && (!UI_IsForceLockAutoSleep())) {
		GxPower_DetSleep();
	}
}
#endif
#if (AUTOPWROFF_FUNCTION == ENABLE)
void UI_DetAutoPoweroff(void)
{
	if ((!UI_IsForceLock()) && (!UI_IsForceLockAutoPWROff())) {
		GxPower_DetAutoPoweroff();
	}
}
#endif
#if (PWRLEVEL_FUNCTION == ENABLE)
void UI_DetBattery(void)
{
	if (!UI_IsForceLock()) {
		//#NT#2012/10/23#Philex Lin - begin
		// send NVTEVT_BATTERY event to flash power charging icon in power charging mode
		// power charging case
#if (USB_CHARGE_FUNCTION == ENABLE)
		if (GxUSB_GetConnectType() == USB_CONNECT_CHARGER) {
			Ux_PostEvent(NVTEVT_BATTERY, 0);
		} else 
#endif
		{
			// normal battery detect case
			GxPower_DetBattery();
		}
		//#NT#2012/10/23#Philex Lin - end
	}
}
#endif
#if (USB_CHARGE_FUNCTION == ENABLE)
BOOL gIsBattInsert = TRUE;
BOOL gIsBattDead = FALSE;
void UI_DetBatteryInsert(void)
{
	if (!UI_IsForceLock()) {
		BOOL bInsert;
#if (USB_CHARGE_FUNCTION == ENABLE)
		bInsert = GxPower_GetControl(GXPWR_CTRL_BATTERY_IS_INSERT);
#else
		bInsert = TRUE;
#endif
		if (bInsert != gIsBattInsert) {
			gIsBattInsert = bInsert;
			debug_msg("  Battery Insert = %s\r\n", gIsBattInsert ? "YES" : "NO");
		}
	}
}
#endif
#if (SHOW_ADC_ON_SCREEN == ENABLE)
void System_DetShowADC(void)
{
	IRECT  StringRect = { 200, 162, 340, 40 };
	IRECT  *pRect = &StringRect;
	static char ADCStr[20];

	//UIDebug_Sys("aa  = %d\r\n",UI_GetData(UI_ShowADC));
	if (UI_GetData(UI_ShowADC) == TRUE) {
		DC     **pDCList;
		UI_SetDisplayDirty(TRUE); //set TRUE to force dirty current begin/end

		pDCList = (DC **)UI_BeginScreen();
		sprintf(ADCStr, "AD0-%ld,AD1-%ld,AD2-%ld", adc_readData(0), adc_readData(1), adc_readData(2));
		GxGfx_SetTextStroke((const FONT *)gDemo_Font, FONTSTYLE_NORMAL, SCALE_1X);
		GxGfx_SetTextColor(_OSD_INDEX_TRANSPART, _OSD_INDEX_TRANSPART, 0);
		GxGfx_FillRect(((DC **)pDCList)[GxGfx_OSD], pRect->uiLeft, pRect->uiTop, pRect->uiLeft + pRect->uiWidth, pRect->uiTop + pRect->uiHeight);
		GxGfx_SetShapeColor(_OSD_INDEX_TRANSPART, _OSD_INDEX_TRANSPART, 0);
		GxGfx_SetTextColor(_OSD_INDEX_RED, _OSD_INDEX_GRAY, 0);
		GxGfx_Text(((DC **)pDCList)[GxGfx_OSD], pRect->uiLeft, pRect->uiTop, ADCStr);
		UI_EndScreen((UINT32)pDCList);
	}

}
#endif


///////////////////////////////////////////////////////////////////////////////

INT32 System_OnBattChange(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	Ux_PostEvent(NVTEVT_BATTERY, 0);
	DBG_DUMP("battery changed %d**********\r\n", GxPower_GetControl(GXPWR_CTRL_BATTERY_LEVEL));

	return NVTEVT_CONSUME;
}

INT32 System_OnBattEmpty(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if (UI_FUNC == ENABLE)
	if (UI_GetData(FL_IsCIPARunning)) {
		if (UI_GetData(FL_IsEmptyBatt) == FALSE) {
			UI_SetData(FL_IsEmptyBatt, TRUE);
		}
	}
#else
	if (1) {
	}
#endif
#if (EMPTYPWROFF_FUNCTION == ENABLE)
	else {
		// set battery low event to ui window
		Ux_PostEvent(NVTEVT_BATTERY_LOW, 0);
	}
#endif
	return NVTEVT_CONSUME;
}

INT32 System_OnPwrChargeOK(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, SET_TOGGLE_LED, FALSE);
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, TURNON_LED, TRUE);
	return NVTEVT_CONSUME;
}

INT32 System_OnPwrChargeSuspend(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, SET_TOGGLE_LED, FALSE);
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, TURNON_LED, FALSE);
	return NVTEVT_CONSUME;
}

INT32 System_OnPwrChargeResume(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, SETLED_SPEED, GXLED_1SEC_LED_TOGGLE_CNT);
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, SET_TOGGLE_LED, TRUE);
	return NVTEVT_CONSUME;
}

void System_ResetPowerSaveCount(void)
{
	//reset auto power off and sleep counter
#if (AUTOPWROFF_FUNCTION == ENABLE)
	GxPower_SetControl(GXPWR_CTRL_AUTOPOWEROFF_EN, 0xff);//reset
#endif
#if (AUTOSLEEP_FUNCTION == ENABLE)
	GxPower_SetControl(GXPWR_CTRL_AUTOSLEEP_EN, 0xff); //reset
#endif
}

#if (USB_CHARGE_FUNCTION == ENABLE)
void Power_StartUSBCharge(void)
{
	if (!gIsBattInsert) {
		DBG_DUMP("  NO Battery, Ignore START\r\n");
		return;
	}

	if (GxPower_GetControl(GXPWR_CTRL_BATTERY_CHARGE_EN)) {
		DBG_DUMP("  Already UNDER Charge, Ignore START\r\n");
		return;
	}

	if ((USB_GetSource() == USB_SRC_USB_ADAPTER)) {
		GxPower_SetControl(GXPWR_CTRL_BATTERY_CHARGE_CURRENT, BATT_CHARGE_CURRENT_HIGH);
		DBG_MSG("  Charge Current = %d\r\n", BATT_CHARGE_CURRENT_HIGH);
	}

	if ((USB_GetSource() == USB_SRC_USB_PC)) {
		GxPower_SetControl(GXPWR_CTRL_BATTERY_CHARGE_CURRENT, BATT_CHARGE_CURRENT_MEDIUM);
		DBG_MSG("  Charge Current = %d\r\n", BATT_CHARGE_CURRENT_MEDIUM);
	}

	DBG_DUMP("GxPower: Stop low-battery check.\r\n");
	GxPower_SetControl(GXPWR_CTRL_BATTERY_DETECT_EN, FALSE);
	DBG_DUMP("GxPower: Start to Charge Battery\r\n");
#if (USB_CHARGE_VERIFY == ENABLE)
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, TURNON_LED, TRUE);
#endif
	GxPower_SetControl(GXPWR_CTRL_BATTERY_CHARGE_EN, TRUE);

#if (USB_CHARGE_VERIFY == ENABLE)
#else
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN,TURNON_LED,FALSE);
#endif
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, SETLED_SPEED, GXLED_1SEC_LED_TOGGLE_CNT);
	//GxLED_SetCtrl(KEYSCAN_LED_GREEN, SET_TOGGLE_LED, TRUE);

#if (FLASHLIGHT_FUNCTION == ENABLE)
	// disable flash re-charge
	SxTimer_SetFuncActive(SX_TIMER_DET_RECHARGE_ID, FALSE);
#endif
}
void Power_StopUSBCharge(void)
{
	//DBG_MSG("  gIsUSBInsert=%d,gIsBattInsert=%d,gIsBattDead=%d,gIsUSBAdapter=%d\r\n",gIsUSBInsert,gIsBattInsert,gIsBattDead,gIsUSBAdapter);
	if (!gIsBattInsert) {
		DBG_DUMP("  NO Battery, Ignore STOP\r\n");
		return;
	}
	if ((USB_GetSource() == USB_SRC_USB_PC)) {
		//DBG_MSG("  USB Src = PC\r\n");
		GxPower_SetControl(GXPWR_CTRL_BATTERY_CHARGE_CURRENT, BATT_CHARGE_CURRENT_LOW);
		DBG_MSG("  Charge Current = %d\r\n", BATT_CHARGE_CURRENT_LOW);
	}

	if (!GxPower_GetControl(GXPWR_CTRL_BATTERY_CHARGE_EN)) {
		DBG_DUMP("  Already NOT Charge, Ignore STOP\r\n");
		return;
	}

	DBG_DUMP("GxPower: Stop to Charge Battery\r\n");
	GxPower_SetControl(GXPWR_CTRL_BATTERY_CHARGE_EN, FALSE);
	DBG_DUMP("GxPower: Start low-battery check.\r\n");
	GxPower_SetControl(GXPWR_CTRL_BATTERY_DETECT_EN, TRUE);
#if (FLASHLIGHT_FUNCTION == ENABLE)
	// enable flash re-charge
	SxTimer_SetFuncActive(SX_TIMER_DET_RECHARGE_ID, TRUE);
#endif
}
#endif



