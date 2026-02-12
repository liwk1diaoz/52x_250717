#include <stdio.h>
#include <string.h>
#include "kwrap/util.h"
#include "Utility/SwTimer.h"
//#include "GxSystem.h"
#include "DxPower.h"
#include "GxPower.h"
#include "PowerDef.h"
#include "DxApi.h"
#include "Dx.h"
#include "DxType.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxPwr
#define __DBGLVL__          THIS_DBGLVL
//#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#define __DBGFLT__          "[charge]" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"

#define CHARGE_TEMPERATURE_STATUS_NORMAL    0
#define CHARGE_TEMPERATURE_STATUS_COLD      1
#define CHARGE_TEMPERATURE_STATUS_LOW       2
#define CHARGE_TEMPERATURE_STATUS_MEDIUM    3
#define CHARGE_TEMPERATURE_STATUS_HIGH      4

#define CHARGE_TEMPERATURE_COLD             0
#define CHARGE_TEMPERATURE_LOW             10
#define CHARGE_TEMPERATURE_MEDIUM          45
#define CHARGE_TEMPERATURE_HIGH            60
#define CHARGE_TEMPERATURE_TH               2


#define KEYSCAN_AUTOPOWEROFF_DISABLED   0

DX_HANDLE power_obj = 0;

volatile UINT32     g_uiKeyScanSleepCnt            = 0;
volatile BOOL       g_bKeyScanSleepEn              = TRUE;
volatile BOOL       g_bKeyScanResetSleeping        = FALSE;
volatile UINT32     g_uiKeyScanSleepLevel          = 0;


static volatile UINT32  g_uiKeyScanAutoPoweroffCnt;
static volatile BOOL    g_bKeyScanAutoPoweroffEn = FALSE;
//extern BOOL     bEnterPoweroff;
static UINT32   uiSystemAutoPoweroffTime     = KEYSCAN_AUTOPOWEROFF_DISABLED;
static UINT32   uiLCDAutoPoweroffTime  = KEYSCAN_AUTOPOWEROFF_DISABLED;
static UINT32   uiLCDAutoPoweroffTime_L  = KEYSCAN_AUTOPOWEROFF_DISABLED;
static UINT32   uiLCDAutoPoweroffTime_D  = KEYSCAN_AUTOPOWEROFF_DISABLED;
static UINT32  ubBatteryLevel         = VOLDET_BATTERY_LVL_UNKNOWN;
static UINT32  uiPrevBatteryLvl       = VOLDET_BATTERY_LVL_UNKNOWN;
static UINT32  uiBatteryLvl           = VOLDET_BATTERY_LVL_UNKNOWN;
GX_CALLBACK_PTR   g_fpPowerCB = NULL;
static BOOL     g_bBatteryDetEn         = TRUE;
static UINT32   uiChargeCount           = 0;
static UINT32   uiReChargeCount         = 0;
static BOOL     bIsReCharge             = FALSE;
static UINT32   ChargeTemperatureSts    = CHARGE_TEMPERATURE_STATUS_NORMAL;
static UINT32   normalTempChargeCurrent;
static BOOL     g_bIgnoreDet            = FALSE;

#define CHARGE_OK_STABLE_COUNT          3

//extern void xGxPower_InstallCmd(void);

UINT32 GxPower_Open(DX_HANDLE hPwr, GX_CALLBACK_PTR DxPower_CB)
{
	//xGxPower_InstallCmd();
	UINT32 uiDxState = 0;
	power_obj = hPwr;
	if (!power_obj) {
		DBG_ERR("can not get Dx power object\r\n");
		return GX_ERROR_INITDEVICE;
	}
	if (Dx_GetState(power_obj, DRVPWR_CTRL_IS_DUMMUYLOAD_POWEROFF, &uiDxState)) {
		ubBatteryLevel = VOLDET_BATTERY_LVL_UNKNOWN;
	} else {
		Dx_Init(power_obj, NULL, (DX_CALLBACK_PTR)DxPower_CB, 0);
		// DrvPower_PowerOnInit();
	}
	if (Dx_Open((DX_HANDLE)power_obj) != DX_OK) { //here will update current size to INFOBUF
		DBG_ERR("Dx_Open fail\r\n");
		return GX_ERROR_INITDEVICE;
	}
	return GX_OK;
}

UINT32 GxPower_Close(void)
{
	UINT32 ret;
	if (!power_obj) {
		DBG_ERR("can not get Dx power object\r\n");
		return GX_ERROR_INITDEVICE;
	}
	ret = Dx_Close(power_obj);
	if (DX_OK != ret && DX_NOT_OPEN != ret) {
		DBG_ERR("Dx_Close fail, err: %d\r\n", ret);
		return GX_ERROR_INITDEVICE;
	}
	power_obj = 0;
	return GX_OK;
}

static UINT32 GxPower_MapBattLvl(UINT32 DxpwrLvl)
{
	UINT32 rtnlvl = 0;

	switch (DxpwrLvl) {
	case DRVPWR_BATTERY_LVL_UNKNOWN:
		rtnlvl = VOLDET_BATTERY_LVL_UNKNOWN;
		break;
	case DRVPWR_BATTERY_LVL_EMPTY:
		rtnlvl = VOLDET_BATTERY_LVL_EMPTY;
		break;
	case DRVPWR_BATTERY_LVL_0:
		rtnlvl = VOLDET_BATTERY_LVL_0;
		break;
	case DRVPWR_BATTERY_LVL_1:
		rtnlvl = VOLDET_BATTERY_LVL_1;
		break;
	case DRVPWR_BATTERY_LVL_2:
		rtnlvl = VOLDET_BATTERY_LVL_2;
		break;
	case DRVPWR_BATTERY_LVL_3:
		rtnlvl = VOLDET_BATTERY_LVL_3;
		break;
	case DRVPWR_BATTERY_LVL_4:
		rtnlvl = VOLDET_BATTERY_LVL_4;
		break;
	default:
		DBG_ERR("Unknown BattLvl %d\r\n", DxpwrLvl);
		break;
	}
	return rtnlvl;
}

/**
  Detect battery voltage level

  Detect battery voltage level and store it in uiBatteryLvl (static variable)
  If the battery voltage is VOLDET_BATTERY_LVL_EMPTY, then power off the system.
  [KeyScan internal API]

  @param void
  @return void
*/
void GxPower_DetBattery(void)
{
#define EMPTY_BATT_COUNT  2
	//static UINT32 u32EmptyBattCount = 0;
	static BOOL bLockBattery = FALSE;
	UINT32 uiDxStatus = 0;
	if (!power_obj) {
		DBG_ERR("can not get Dx power object\r\n");
		return;
	}
	
	if ((GxPower_GetControl(GXPWR_CTRL_BATTERY_DETECT_EN))) {

		//uiBatteryLvl = GxPower_MapBattLvl(DrvPower_GetControl(DRVPWR_CTRL_BATTERY_LEVEL));
		if (Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_LEVEL, &uiDxStatus) == DX_OK) {

			uiBatteryLvl = GxPower_MapBattLvl(uiDxStatus);
		} else {
			DBG_ERR("Dx_GetState get DRVPWR_CTRL_BATTERY_LEVEL error\r\n");
		}
	}
	if (uiBatteryLvl == VOLDET_BATTERY_LVL_EMPTY) {
		if (g_fpPowerCB) {
			g_fpPowerCB(POWER_CB_BATT_EMPTY, 0, 0);
		}
	} else if ((uiPrevBatteryLvl == VOLDET_BATTERY_LVL_UNKNOWN)
			   && (uiBatteryLvl != VOLDET_BATTERY_LVL_UNKNOWN)) {
		//this is first time detect
		ubBatteryLevel   = uiBatteryLvl;
		uiPrevBatteryLvl = uiBatteryLvl;
		if (g_fpPowerCB) {
			g_fpPowerCB(POWER_CB_BATT_CHG, 0, 0);
		}
	} else if (!Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, &uiDxStatus)) { //use power
		if ((uiPrevBatteryLvl > uiBatteryLvl) && (!bLockBattery)) {
			ubBatteryLevel   = uiBatteryLvl;
			uiPrevBatteryLvl = uiBatteryLvl;
			if (g_fpPowerCB) {
				g_fpPowerCB(POWER_CB_BATT_CHG, 0, 0);
			}
		}
	} else { //charge power
		if ((uiPrevBatteryLvl < uiBatteryLvl) && (!bLockBattery)) {
			ubBatteryLevel   = uiBatteryLvl;
			uiPrevBatteryLvl = uiBatteryLvl;
			if (g_fpPowerCB) {
				g_fpPowerCB(POWER_CB_BATT_CHG, 0, 0);
			}
		}
	}

#if 0
	if (u32EmptyBattCount) {
		u32EmptyBattCount++;
		if (u32EmptyBattCount > EMPTY_BATT_COUNT) {
			g_fpPowerCB(POWER_CB_BATT_EMPTY, 0, 0);
		}
	}
	if (uiBatteryLvl == VOLDET_BATTERY_LVL_EMPTY) {
		u32EmptyBattCount ++;
		bLockBattery = TRUE;
	}
#else
#endif

	if (Dx_GetState(power_obj, DRVPWR_CTRL_IS_BATT_OVERHEAT, &uiDxStatus)) {
		if (g_fpPowerCB) {
			g_fpPowerCB(POWER_CB_BATT_OVERHEAT, 0, 0);
		}
	}
}

/**
  Detect auto power off

  Detect auto power off.
  If the auto power off counter reach auto power off setting, then do power off
  [KeyScan internal API]

  @param void
  @return void
*/
void GxPower_DetAutoPoweroff(void)
{
	if (!power_obj) {
		DBG_ERR("can not get Dx power object\r\n");
		return;
	}
	
	if ((GxPower_GetControl(GXPWR_CTRL_AUTOPOWEROFF_TIME) != KEYSCAN_AUTOPOWEROFF_DISABLED)
		&& (g_bKeyScanAutoPoweroffEn == TRUE)) {
		DBG_IND("AutoPwrOff Cnt = %d\r\n", g_uiKeyScanAutoPoweroffCnt);

		g_uiKeyScanAutoPoweroffCnt++;

		if (g_uiKeyScanAutoPoweroffCnt >= GxPower_GetControl(GXPWR_CTRL_AUTOPOWEROFF_TIME)) {
			DBG_IND("Do AutoPwrOff!\r\n");
			g_fpPowerCB(POWER_CB_POWEROFF, 0, 0);
		}
	}
}


void GxPower_DetSleep(void)
{
	if (!power_obj) {
		DBG_ERR("can not get Dx power object\r\n");
		return;
	}
	
	if (g_bKeyScanResetSleeping) {
		g_bKeyScanResetSleeping = FALSE;
		GxPower_SetControl(GXPWR_CTRL_AUTOSLEEP_EN, 0xff); //reset
	}
	if ((!g_bKeyScanSleepEn) || (g_uiKeyScanSleepLevel == 3)) {
		return;
	}

	if (GxPower_GetControl(GXPWR_CTRL_AUTOSLEEP_TIME)  != 0) {
		UINT32 OffTime = GxPower_GetControl(GXPWR_CTRL_AUTOSLEEP_TIME) ;
		UINT32 OffTime_L = GxPower_GetControl(GXPWR_CTRL_AUTOSLEEP_TIME_L) ;
		UINT32 OffTime_D = GxPower_GetControl(GXPWR_CTRL_AUTOSLEEP_TIME_D) ;

		DBG_IND("AutoSleep Cnt = %d\r\n", g_uiKeyScanSleepCnt);

		g_uiKeyScanSleepCnt++;

		if ((OffTime_L > 0) && (g_uiKeyScanSleepCnt == OffTime_L)) {
			g_uiKeyScanSleepLevel = 1;
			DBG_IND("Do AutoSleep level 1\r\n");
			g_fpPowerCB(POWER_CB_SLEEP_ENTER_L, 0, 0);
		}
		if ((OffTime > 0) && (g_uiKeyScanSleepCnt == OffTime)) {
			g_uiKeyScanSleepLevel = 2;
			DBG_IND("Do AutoSleep level 2\r\n");
			g_fpPowerCB(POWER_CB_SLEEP_ENTER, 0, 0);
		}
		if ((OffTime_D > 0) && (g_uiKeyScanSleepCnt == OffTime_D)) {
			g_uiKeyScanSleepLevel = 3;
			DBG_IND("Do AutoSleep level 3\r\n");
			g_fpPowerCB(POWER_CB_SLEEP_ENTER_D, 0, 0);
		}
	}
}

void GxPower_DetCharge(void)
{
	INT32 temperature;
	UINT32 uiDxState = 0;
	if (!power_obj) {
		DBG_ERR("can not get Dx power object\r\n");
		return;
	}

	if (Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, &uiDxState)) {

		if (Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_OK, &uiDxState) && uiReChargeCount > CHARGE_OK_STABLE_COUNT) {
			Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, FALSE);
			Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_ISET, TRUE);
			g_fpPowerCB(POWER_CB_CHARGE_OK, 0, 0);
			DBG_DUMP("Charge OK\r\n");
		}

		else if (Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_OK, &uiDxState) && uiChargeCount > CHARGE_OK_STABLE_COUNT) {

			if (Dx_GetState(power_obj, DRVPWR_CTRL_IS_NEED_RECHARGE, &uiDxState) && bIsReCharge == FALSE) {
				// need to set ISET false and recharge again.
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_ISET, FALSE);
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, FALSE);

				SwTimer_DelayMs(10);
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, TRUE);
				bIsReCharge = TRUE;
			} else {
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, FALSE);
				g_fpPowerCB(POWER_CB_CHARGE_OK, 0, 0);
				DBG_DUMP("Charge OK\r\n");
			}
		}

		temperature = Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_TEMPERATURE, &uiDxState);
		//#NT#2011/04/19#Lincy Lin -begin
		//#NT#add control for batt not have Temperature Sensor
		if (temperature == BATTERY_TEMPERATURE_UNKNOWN) {
			Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, FALSE);
			g_fpPowerCB(POWER_CB_CHARGE_OK, 0, 0);
			DBG_DUMP("Charge OK\r\n");
		}
		//#NT#2011/04/19#Lincy Lin -end
		else if (temperature > CHARGE_TEMPERATURE_HIGH) {
			if (ChargeTemperatureSts != CHARGE_TEMPERATURE_STATUS_HIGH) {
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, FALSE);
				ChargeTemperatureSts = CHARGE_TEMPERATURE_STATUS_HIGH;
				g_fpPowerCB(POWER_CB_CHARGE_SUSPEND, 0, 0);
				DBG_ERR("Charge Temp High\r\n");
			}

		} else if (temperature > CHARGE_TEMPERATURE_MEDIUM) {
			if (ChargeTemperatureSts != CHARGE_TEMPERATURE_STATUS_MEDIUM) {
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_ISET, TRUE);
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_VSET, FALSE);
				ChargeTemperatureSts = CHARGE_TEMPERATURE_STATUS_MEDIUM;
				DBG_ERR("Charge Temp Med\r\n");
			}
		} else if (temperature < CHARGE_TEMPERATURE_COLD) {
			if (ChargeTemperatureSts != CHARGE_TEMPERATURE_STATUS_COLD) {
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, FALSE);
				ChargeTemperatureSts = CHARGE_TEMPERATURE_STATUS_COLD;
				g_fpPowerCB(POWER_CB_CHARGE_SUSPEND, 0, 0);
				DBG_ERR("Charge Temp Cold\r\n");
			}
		} else if (temperature < CHARGE_TEMPERATURE_LOW) {
			if (ChargeTemperatureSts != CHARGE_TEMPERATURE_STATUS_LOW) {
				ChargeTemperatureSts = CHARGE_TEMPERATURE_STATUS_LOW;
				normalTempChargeCurrent = Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_CURRENT, &uiDxState);
				Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_CURRENT, BATTERY_CHARGE_CURRENT_MEDIUM);
				DBG_ERR("Charge Temp low\r\n");
			}
		} else {
			if (ChargeTemperatureSts != CHARGE_TEMPERATURE_STATUS_NORMAL) {
				if ((ChargeTemperatureSts == CHARGE_TEMPERATURE_STATUS_MEDIUM) && (temperature <= CHARGE_TEMPERATURE_MEDIUM - CHARGE_TEMPERATURE_TH)) {
					Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_ISET, TRUE);
					Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_VSET, TRUE);
					ChargeTemperatureSts = CHARGE_TEMPERATURE_STATUS_NORMAL;
					DBG_ERR("Charge Temp medium -> normal\r\n");
				} else if ((ChargeTemperatureSts == CHARGE_TEMPERATURE_STATUS_LOW) && (temperature >= CHARGE_TEMPERATURE_LOW + CHARGE_TEMPERATURE_TH)) {
					Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_ISET, TRUE);
					Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_VSET, TRUE);
					Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_CURRENT, normalTempChargeCurrent);
					ChargeTemperatureSts = CHARGE_TEMPERATURE_STATUS_NORMAL;
					DBG_ERR("Charge Temp low -> normal\r\n");
				}
			}
		}
		uiChargeCount++;
		if (bIsReCharge) {
			uiReChargeCount++;
		}
		DBG_IND("[charge]Cnt=%d,Rcnt=%d,Rchg=%d,Curr=%d,ISET=%d,VSET=%d,En=%d,OK=%d,AD=%d,Tmp=%d,TmpS=%d\r\n"\
				, uiChargeCount, uiReChargeCount, bIsReCharge\
				, Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_CURRENT, &uiDxState)\
				, Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_ISET, &uiDxState)\
				, Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_VSET, &uiDxState)\
				, Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, &uiDxState)\
				, Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_OK, &uiDxState)\
				, Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_ADC_VALUE, &uiDxState)\
				, Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_TEMPERATURE, &uiDxState)\
				, ChargeTemperatureSts);
		{
#if 0
			struct tm cDateTime = GxSysTime_GetValue();
			DBG_IND("[charge]H=%d, M=%d, S=%d\r\n", cDateTime.tm_hour, cDateTime.tm_min, cDateTime.tm_sec);
#endif
		}
	} else if (ChargeTemperatureSts == CHARGE_TEMPERATURE_STATUS_HIGH) {

		temperature = Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_TEMPERATURE, &uiDxState);
		if (temperature <= CHARGE_TEMPERATURE_HIGH - CHARGE_TEMPERATURE_TH) {
			Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, TRUE);
			g_fpPowerCB(POWER_CB_CHARGE_RESUME, 0, 0);
		}
	} else if (ChargeTemperatureSts == CHARGE_TEMPERATURE_STATUS_COLD) {
		temperature = Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_TEMPERATURE, &uiDxState);
		if (temperature >= (CHARGE_TEMPERATURE_COLD + CHARGE_TEMPERATURE_TH)) {
			Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, TRUE);
			ChargeTemperatureSts = CHARGE_TEMPERATURE_STATUS_LOW;
			Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_CURRENT, BATTERY_CHARGE_CURRENT_MEDIUM);
			DBG_ERR("Charge Temp cold -> low\r\n");
			g_fpPowerCB(POWER_CB_CHARGE_RESUME, 0, 0);
		}
	}
}

UINT32 GxPower_GetPwrKey(GXPWR_KEY kid)
{
	UINT32 getv = 0;
	UINT32 key;
	if (!power_obj) {
		return 0;
	}
	
	switch (kid) {
	case GXPWR_KEY_POWER1:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_PSW1, (UINT32*)&key);
		break;
	case GXPWR_KEY_HWRESET:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_HWRT, (UINT32*)&key);
		break;
	case GXPWR_KEY_SWRESET:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_SWRT, (UINT32*)&key);
		break;
	default:
		break;
	}
	return getv;
}

void GxPower_SetPwrKey(GXPWR_KEY kid, UINT32 key)
{
	if (!power_obj) {
		return;
	}
	
	switch (kid) {
	case GXPWR_KEY_POWER1:
		Dx_SetState(power_obj, DRVPWR_CTRL_PSW1, key);
		break;
	case GXPWR_KEY_HWRESET:
		Dx_SetState(power_obj, DRVPWR_CTRL_HWRT, key);
		break;
	case GXPWR_KEY_SWRESET:
		Dx_SetState(power_obj, DRVPWR_CTRL_SWRT, key);
		break;
	default:
		break;
	}
}


UINT32  GxPower_GetControl(GXPWR_CTRL PowerCtrl)
{
	UINT32 getv = 0;
	UINT32 uiDxState = 0;
	if (!power_obj) {
		return 0;
	}
	
	switch (PowerCtrl) {
	case GXPWR_CTRL_POWERON_DETECT_EN:
		getv = 0;
		break;
	case GXPWR_CTRL_POWERON_SRC:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_POWERON_SOURCE, &uiDxState);
		getv =uiDxState;
		break;
	case GXPWR_CTRL_POWERON_LOST:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_POWER_LOST, &uiDxState); //"firs time power-on" or "lost power of Gold capacitor"
		getv =uiDxState;
		break;
	case GXPWR_CTRL_AUTOPOWEROFF_EN:
		getv = (UINT32)g_bKeyScanAutoPoweroffEn;
		break;
	case GXPWR_CTRL_AUTOPOWEROFF_TIME:
		getv = uiSystemAutoPoweroffTime;
		break;
		
	case GXPWR_CTRL_SLEEP_LEVEL:
		getv = g_uiKeyScanSleepLevel;
		break;
	case GXPWR_CTRL_AUTOSLEEP_EN:
		getv = (UINT32)g_bKeyScanSleepEn;
		break;
	case GXPWR_CTRL_AUTOSLEEP_TIME:
		getv = uiLCDAutoPoweroffTime;
		break;
	case GXPWR_CTRL_AUTOSLEEP_TIME_L:
		getv = uiLCDAutoPoweroffTime_L;
		break;
	case GXPWR_CTRL_AUTOSLEEP_TIME_D:
		getv = uiLCDAutoPoweroffTime_D;
		break;
		
	case GXPWR_CTRL_BATTERY_DETECT_EN:
		getv = (UINT32)g_bBatteryDetEn;
		break;
	case GXPWR_CTRL_BATTERY_LEVEL:
		getv = ubBatteryLevel;
		break;
	case GXPWR_CTRL_BATTERY_IS_INSERT:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_IS_BATT_INSERT, &uiDxState);
		//DBG_IND("[charge] IsBattIn=%d\r\n",getv);
		break;
	case GXPWR_CTRL_BATTERY_IS_DEAD:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_IS_DEAD_BATT, &uiDxState);
		//DBG_IND("[charge] IsDeatBatt=%d\r\n",getv);
		break;
	case GXPWR_CTRL_BATTERY_CHARGE_EN:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, &uiDxState);
		break;
	case GXPWR_CTRL_BATTERY_CHARGE_OK:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_OK, &uiDxState);
		break;
	case GXPWR_CTRL_BATTERY_POWERON_CHECK_EN:
		getv = (UINT32)(g_bIgnoreDet)?(0):(1);
		break;
	case GXPWR_CTRL_BATTERY_FAIL_POWEROFF_EN:
		getv = Dx_GetState(power_obj, DRVPWR_CTRL_IS_DUMMUYLOAD_POWEROFF, &uiDxState);
		break;
	default:
		DBG_ERR("PowerCtrl(%d)\r\n", PowerCtrl);
		getv = 0;
		break;
	}
	return getv;
}

void GxPower_SetControl(GXPWR_CTRL PowerCtrl, UINT32 value)
{
	if (!power_obj) {
		DBG_ERR("can not get Dx power object\r\n");
		return;
	}
	
	DBG_IND("cmd=(%d), value=%d\r\n", PowerCtrl, value);
	switch (PowerCtrl) {
	case GXPWR_CTRL_POWERON_DETECT_EN:
		break;
	case GXPWR_CTRL_AUTOPOWEROFF_EN:
		if (value == 0xff) {
			//reset
			g_uiKeyScanAutoPoweroffCnt = 0;
		} else {
			if (g_bKeyScanAutoPoweroffEn == FALSE && value == TRUE) {
				g_uiKeyScanAutoPoweroffCnt = 0;
			}
			g_bKeyScanAutoPoweroffEn = (INT32)value;
		}
		break;
	case GXPWR_CTRL_AUTOPOWEROFF_TIME:
		uiSystemAutoPoweroffTime = value;
		DBG_IND("AutoPwrOff Max cnt = %d\r\n", uiSystemAutoPoweroffTime);
		break;
		
	case GXPWR_CTRL_AUTOSLEEP_EN:
		if (value == 0xff) {
			//reset
			if (g_uiKeyScanSleepCnt && g_bKeyScanSleepEn) {
				g_uiKeyScanSleepCnt = 0;
				if (g_uiKeyScanSleepLevel > 0) {
					g_fpPowerCB(POWER_CB_SLEEP_RESET, 0, 0);
					g_uiKeyScanSleepLevel = 0;
				}
			}
		} else {
			g_bKeyScanSleepEn = (INT32)value;
		}
		break;
	case GXPWR_CTRL_AUTOSLEEP_TIME:
		uiLCDAutoPoweroffTime = value;
		DBG_IND("AutoSleep Max2 cnt = %d\r\n", uiLCDAutoPoweroffTime);
		break;
	case GXPWR_CTRL_AUTOSLEEP_TIME_L:
		uiLCDAutoPoweroffTime_L = value;
		DBG_IND("AutoSleep Max1 cnt = %d\r\n", uiLCDAutoPoweroffTime_L);
		break;
	case GXPWR_CTRL_AUTOSLEEP_TIME_D:
		uiLCDAutoPoweroffTime_D = value;
		DBG_IND("AutoSleep Max3 cnt = %d\r\n", uiLCDAutoPoweroffTime_D);
		break;
		
	case GXPWR_CTRL_BATTERY_DETECT_EN:
		g_bBatteryDetEn = (INT32)value;
		if (value) {
			uiPrevBatteryLvl = VOLDET_BATTERY_LVL_UNKNOWN;
			uiBatteryLvl = VOLDET_BATTERY_LVL_UNKNOWN;
		}
		break;
	case GXPWR_CTRL_BATTERY_CHARGE_EN:
		if (value == TRUE) {
			Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_ISET, TRUE);
			Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_VSET, TRUE);

		}
		Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_EN, value);
		DBG_IND("[charge] Encharge =%d\r\n", value);
		uiChargeCount = 0;
		uiReChargeCount = 0;
		bIsReCharge = FALSE;
		ChargeTemperatureSts = CHARGE_TEMPERATURE_STATUS_NORMAL;
		break;
	case GXPWR_CTRL_BATTERY_CHARGE_CURRENT:
		Dx_SetState(power_obj, DRVPWR_CTRL_BATTERY_CHARGE_CURRENT, value);
		DBG_IND("[charge] Charge Current=%d\r\n", value);
		break;
	case GXPWR_CTRL_BATTERY_POWERON_CHECK_EN:
		g_bIgnoreDet = (BOOL)(value)?(0):(1);
		break;
	default:
		DBG_ERR("PowerCtrl(%d)\r\n", PowerCtrl);
		break;
	}
}

UINT32  GxPower_DummyLoad(void)
{
	return DrvPower_DummyLoad();
}

void GxPower_RegCB(GX_CALLBACK_PTR fpPowerCB)
{
	g_fpPowerCB = fpPowerCB;
}

#if 0
void GxPower_OnSystem(int cmd)
{
	switch (cmd) {
	case SYSTEM_CMD_POWERON:
		if (g_fpPowerCB) {
			(*g_fpPowerCB)(SYSTEM_CB_CONFIG, 0, 0);
		}
		GxPower_PowerON();
		break;

	case SYSTEM_CMD_POWEROFF:
		break;
	default:
		break;
	}
}
#endif


//-----------------------------------------------------------------------------
// Date-Time util
//-----------------------------------------------------------------------------

static struct tm g_ctv_zero = {0};

UINT32 GxPower_GetTime(GXTIME_ID tid, struct tm* p_ctv)
{
	if (!power_obj) {
		*(p_ctv) = g_ctv_zero;
		return 0;
	}
	
	switch (tid) {
	case GXTIME_HWRESET:
		Dx_Control(power_obj, DRVPWR_CTRL_HWRT_TIME, 1, (UINT32)p_ctv);//1=read
		break;
	case GXTIME_SWRESET:
		Dx_Control(power_obj, DRVPWR_CTRL_SWRT_TIME, 1, (UINT32)p_ctv);//1=read
		break;
	default:
		break;
	}
	return 0;
}

void GxPower_SetTime(GXTIME_ID tid, struct tm ctv, UINT32 param)
{
	if (!power_obj) {
		return;
	}
	
	switch (tid) {
	case GXTIME_HWRESET:
		Dx_Control(power_obj, DRVPWR_CTRL_HWRT_TIME, 0, (UINT32)&ctv); //0=write
		break;
	case GXTIME_SWRESET:
		Dx_Control(power_obj, DRVPWR_CTRL_SWRT_TIME, 0, (UINT32)&ctv); //0=write
		break;
	default:
		break;
	}
}


//@}
