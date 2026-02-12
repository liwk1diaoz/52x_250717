/**
    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.

    @file       DetKey.c
    @ingroup    mIPRJAPKeyIO

    @brief      Scan key, modedial
                Scan key, modedial

    @note       Nothing.

    @date       2009/04/22
*/

/** \addtogroup mIPRJAPKeyIO */
//@{

#include "kwrap/type.h"
#include "KeyInLog.h"
#include "GxInput.h"
#include "DxInput.h"
#include "GxKey_int.h"

//extern volatile BOOL    g_bKeyScanResetSleeping;

#define KEYSCAN_PWROFF_INIT_STATE       0
#define KEYSCAN_PWROFF_RELEASE_STATE    1
#define KEYSCAN_PWROFF_PRESS_STATE      2

#define KEYSCAN_PWRKEY_UNKNOWN          0xFFFFFFFF
#define KEYSCAN_PWRKEY_RELEASED         0
#define KEYSCAN_PWRKEY_PRESSED          1

// -------------------------------------------------------------------
// Static variables
// -------------------------------------------------------------------
static UINT32       g_uiLastKeyStatus         = 0;
static UINT32       g_uiStableKeyStatus       = 0;
//#NT#2010/08/02#Ben Wang -begin
static UINT32       g_uiFirstKeyInvalidPress  = 0;
static UINT32       g_uiFirstKeyInvalidRelease  = 0;
static UINT32       g_uiFirstKeyInvalidContinue  = 0;
//#NT#2010/08/02#Ben Wang -end
static UINT32       g_uiSimKeyStatus       = 0;
//#NT#2010/02/23#Ben Wang -begin
//#NT#Refine code for continue key
//static UINT32       uiLastStableKeyStatus   = 0;
static UINT32       g_uiLastMDStatus[STATUS_KEY_GROUP_MAX]        = {0};
static UINT32       g_uiStableMDStatus[STATUS_KEY_GROUP_MAX]      = {0};


BOOL       g_bDumpKey = FALSE;


#if(KEYINLOG_FUNCTION == ENABLE)
static UINT32   g_uiKeyScanTimerCnt = 0;
#endif

#define   KEY_TIMER_CNT 10//10x20=200ms
#define   KEY_CONTINUE_DEBOUNCE  30//30x20=600ms

static GX_CALLBACK_PTR   g_fpKeyCB = NULL;
static UINT32       g_uiRepeatCnt = KEY_TIMER_CNT;
static UINT32       g_uiDebounceCnt = KEY_CONTINUE_DEBOUNCE;


void GxKey_RegCB(GX_CALLBACK_PTR pFuncKeyCB)
{
	g_fpKeyCB = pFuncKeyCB;
}

void GxKey_SetRepeatInterval(UINT32 RepeatCnt)
{
	g_uiRepeatCnt = RepeatCnt;
}

void GxKey_SetContDebounce(UINT32 DebounceCnt)
{
	g_uiDebounceCnt = DebounceCnt;
}


static void DetKey_ShowKeyName(UINT32 uiKeyCode, CHAR *pStr)
{
	if (g_bDumpKey == FALSE) {
		return;
	}
	DBG_DUMP("KeyCode 0x%x ", uiKeyCode);
#if 0
	if (uiKeyCode & FLGKEY_UP) {
		DBG_DUMP("DETKEY %s KEY UP\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_DOWN) {
		DBG_DUMP("DETKEY %s KEY DOWN\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_LEFT) {
		DBG_DUMP("DETKEY %s KEY LEFT\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_RIGHT) {
		DBG_DUMP("DETKEY %s KEY RIGHT\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_ENTER) {
		DBG_DUMP("DETKEY %s KEY ENTER\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_MENU) {
		DBG_DUMP("DETKEY %s KEY MENU\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_DEL) {
		DBG_DUMP("DETKEY %s KEY DEL\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_MODE) {
		DBG_DUMP("DETKEY %s KEY MODE\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_ZOOMOUT) {
		DBG_DUMP("DETKEY %s KEY ZOOMOUT\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_ZOOMIN) {
		DBG_DUMP("DETKEY %s KEY ZOOMIN\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_SHUTTER1) {
		DBG_DUMP("DETKEY %s KEY SHUTTER1\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_SHUTTER2) {
		DBG_DUMP("DETKEY %s KEY SHUTTER2\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_FACEDETECT) {
		DBG_DUMP("DETKEY %s KEY FACEDETECT\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_MOVIE) {
		DBG_DUMP("DETKEY %s KEY FLGKEY_MOVIE\r\n", pStr);
	}
	if (uiKeyCode & FLGKEY_I) {
		DBG_DUMP("DETKEY %s KEY FLGKEY_I\r\n", pStr);
	}
#endif
}

void GxKey_RaiseSimKey(UINT32 key)
{
	g_uiSimKeyStatus |= key;
}

void GxKey_Init(void)
{
	DrvKey_Init();
}

void GxKey_DetNormalKey(void)
{
	UINT32          uiCurKeyStatus, uiTempKeyStatus;
	UINT32          uiKeyChanged;
	UINT32          uiKeyPressed, uiKeyReleased, uiKeyContinue;
	static UINT32   uiContineKeyDebounce = 0;
	static UINT32   uiContinueKeyCount = 0 ;
	//#NT#2010/08/02#Ben Wang -begin
	static BOOL     bIsFirstPressKey = TRUE;
	static BOOL     bIsFirstReleaseKey = TRUE;
	//#NT#2010/08/02#Ben Wang -end

	/*
	使用方式 :

	錄Key:
	1. 修改 #define KEYINLOG_FUNCTION   ENABLE
	2. Update F/W 後, 將 Log 存下來 (Ex:KeyLog.log)
	3. 執行 KlTxtToHex KeyLog.log 產生 TransLog.hex
	4. 寄給分析人員

	播Key:
	1. 修改 #define KEYINLOG_FUNCTION   ENABLE
	2. 解除在 KeyScanTsk.c 中的 //Keylog_EnableRuningKeyLog(); 的註解
	3. Copy TransLog.hex 到 SD Card 的根目錄
	4. 開機
	*/

#if(KEYINLOG_FUNCTION == ENABLE)
	/*------------------------------------------------------------*
	 *                                                            *
	 *              KeyIn Log Play Section                        *
	 *                                                            *
	 *------------------------------------------------------------*/
	g_uiKeyScanTimerCnt ++;
	if (Keylog_IsRuningKeyLog()) {
		uiCurKeyStatus      =
			uiTempKeyStatus     =    Keylog_Play_DetKey();
		//#NT#2009/04/07#Niven Cho -begin
		//#NT#Added., Stop Play Record after playing finished.
		Keylog_ScanRecordEnd();
		//#NT#2009/04/07#Niven Cho -end
	}
	/*------------------------------------------------------------*
	 * End Section
	 *------------------------------------------------------------*/
	else
#endif
	{
		uiCurKeyStatus          =
			uiTempKeyStatus         = DrvKey_DetNormalKey();

		if(g_uiSimKeyStatus){
			g_uiLastKeyStatus |= g_uiSimKeyStatus;
			uiCurKeyStatus |= g_uiSimKeyStatus;
			uiTempKeyStatus |= g_uiSimKeyStatus;
			g_uiSimKeyStatus = 0;
		}
	}

	uiCurKeyStatus         &= g_uiLastKeyStatus;

	uiKeyChanged            = uiCurKeyStatus ^ g_uiStableKeyStatus;

	uiKeyPressed            = uiKeyChanged & uiCurKeyStatus;
	uiKeyReleased           = uiKeyChanged & (~uiCurKeyStatus);
	//#NT#2010/02/23#Ben Wang -begin
	//#NT#Refine code for continue key
	//uiKeyContinue           = uiCurKeyStatus & g_uiStableKeyStatus & uiLastStableKeyStatus;
	//uiLastStableKeyStatus   = g_uiStableKeyStatus & ~(uiKeyContinue);
	//ContinueKey is equal to StableKey ?
	uiKeyContinue           = uiCurKeyStatus;
	//#NT#2010/02/23#Ben Wang -end
	g_uiStableKeyStatus       = uiCurKeyStatus;
	g_uiLastKeyStatus         = uiTempKeyStatus;
#if(KEYINLOG_FUNCTION == ENABLE)
	/*------------------------------------------------------------*
	*                                                            *
	*              KeyIn Log Section                             *
	*                                                            *
	*------------------------------------------------------------*/
	if (Keylog_IsRecordingKeyLog()) {
		Keylog_Record_DetKey(uiCurKeyStatus);
	}
	/*------------------------------------------------------------*
	* End Section
	*------------------------------------------------------------*/
#endif


	if (uiKeyPressed) {
		DetKey_ShowKeyName(uiKeyPressed, "PRESS");
		//#NT#2010/08/02#Ben Wang -begin
		if (bIsFirstPressKey) {
			bIsFirstPressKey = FALSE;
			uiKeyPressed &= ~ g_uiFirstKeyInvalidPress;
		}
		//#NT#2010/08/02#Ben Wang -end
		if (g_fpKeyCB) {
			g_fpKeyCB(KEY_CB_NORMAL, KEY_PRESS, uiKeyPressed);
		}
	}

	if (uiKeyReleased) {
		uiContineKeyDebounce = 0;
		DetKey_ShowKeyName(uiKeyReleased, "RELEASE");
		//#NT#2010/08/02#Ben Wang -begin
		if (bIsFirstReleaseKey) {
			bIsFirstReleaseKey = FALSE;
			uiKeyReleased &= ~ g_uiFirstKeyInvalidRelease;
		}
		//#NT#2010/08/02#Ben Wang -end
		if (g_fpKeyCB) {
			g_fpKeyCB(KEY_CB_NORMAL, KEY_RELEASE, uiKeyReleased);
		}
	}
	if (uiKeyContinue) {
		//#NT#2010/02/23#Ben Wang -begin
		//#NT#Refine code for continue key
		if (uiContineKeyDebounce < g_uiDebounceCnt) {
			uiContineKeyDebounce++;
		} else {
			uiContinueKeyCount++;
		}
		if ((uiContineKeyDebounce == g_uiDebounceCnt) && (uiContinueKeyCount == g_uiRepeatCnt)) {
			uiContinueKeyCount = 0;

			DetKey_ShowKeyName(uiKeyContinue, "CONTINUE");
			//#NT#2010/08/02#Ben Wang -begin
			//it will always be "first continue" before first released key
			if (bIsFirstReleaseKey) {
				uiKeyContinue &= ~ g_uiFirstKeyInvalidContinue;
			}
			//#NT#2010/08/02#Ben Wang -end
			if (g_fpKeyCB) {
				g_fpKeyCB(KEY_CB_NORMAL, KEY_CONTINUE, uiKeyContinue);
			}

		}
		//#NT#2010/02/23#Ben Wang -end
	}

//-----------------------------------------------------------------------------------------------
	// Start to set flag
	//-----------------------------------------------------------------------------------------------

}

void GxKey_DetPwrKey(void)
{
	static UINT32 uiPowerKey    = KEYSCAN_PWRKEY_UNKNOWN;
	static UINT32 uiPWRState    = KEYSCAN_PWROFF_INIT_STATE;
	static UINT32 uiDxPwrKeyFlag = 0;
	UINT32 uiPwrKey = DrvKey_DetPowerKey();
	//DBG_DUMP("^YPwrKey = %08x\r\n", uiPwrKey);

	// Detect power off key
	if (uiPwrKey) {
		uiDxPwrKeyFlag = uiPwrKey;
		// Debounce
		if (uiPowerKey == KEYSCAN_PWRKEY_PRESSED) {
			if (uiPWRState == KEYSCAN_PWROFF_RELEASE_STATE) {
				uiPWRState = KEYSCAN_PWROFF_PRESS_STATE;
				if (g_fpKeyCB) {
					g_fpKeyCB(KEY_CB_POWER, KEY_PRESS, uiDxPwrKeyFlag);
				}
			} else if (uiPWRState == KEYSCAN_PWROFF_PRESS_STATE) {
				if (g_fpKeyCB) {
					g_fpKeyCB(KEY_CB_POWER, KEY_CONTINUE, uiDxPwrKeyFlag);
				}
			}
		} else {
			uiPowerKey = KEYSCAN_PWRKEY_PRESSED;
		}
	} else {
		// Debounce
		if (uiPowerKey == KEYSCAN_PWRKEY_RELEASED) {
			if (uiPWRState == KEYSCAN_PWROFF_INIT_STATE) {
				uiPWRState = KEYSCAN_PWROFF_RELEASE_STATE;
			} else if (uiPWRState == KEYSCAN_PWROFF_PRESS_STATE) {
				uiPWRState = KEYSCAN_PWROFF_RELEASE_STATE;
				if (g_fpKeyCB) {
					g_fpKeyCB(KEY_CB_POWER, KEY_RELEASE, uiDxPwrKeyFlag);
				}
			}
		} else {
			uiPowerKey = KEYSCAN_PWRKEY_RELEASED;
		}
	}
}


void GxKey_DetStatusKey(void)
{
	UINT32          uiCurMDStatus, uiTempMDStatus, uiMDChanged, uiModedial;
	UINT32          Group;


	for (Group = STATUS_KEY_GROUP1; Group < STATUS_KEY_GROUP_MAX; Group++) {
		//#NT#2009/03/19#Elvis Chuang - begin
#if 0//(KEYINLOG_FUNCTION == ENABLE)
		/*------------------------------------------------------------*
		 *                                                            *
		 *              KeyIn Log Play Section                        *
		 *                                                            *
		 *------------------------------------------------------------*/
		if (Keylog_IsRuningKeyLog()) {
			uiCurMDStatus       = Keylog_Play_DetModedial();
		}
		/*------------------------------------------------------------*
		 * End Section
		 *------------------------------------------------------------*/
		else
#endif
		{
			uiCurMDStatus       = DrvKey_DetStatusKey((STATUS_KEY_GROUP)Group);
		}
		//#NT#2009/03/19#Elvis Chuang - end

		uiTempMDStatus      = uiCurMDStatus;
		uiCurMDStatus      &= g_uiLastMDStatus[Group];
		uiMDChanged         = uiCurMDStatus ^ g_uiStableMDStatus[Group];
		uiModedial          = uiMDChanged & uiCurMDStatus;

		g_uiStableMDStatus[Group]    = uiCurMDStatus;
		g_uiLastMDStatus[Group]      = uiTempMDStatus;
		if (uiModedial != 0) {
#if 0//(KEYINLOG_FUNCTION == ENABLE)
			/*------------------------------------------------------------*
			 *                                                            *
			 *              KeyIn Log Section                             *
			 *                                                            *
			 *------------------------------------------------------------*/
			if (Keylog_IsRecordingKeyLog()) {
				Keylog_Record_DetModedial(uiModedial);
			}
			/*------------------------------------------------------------*
			 * End Section
			 *------------------------------------------------------------*/
#endif
			if (g_fpKeyCB) {
				g_fpKeyCB(KEY_CB_STATUS, Group, uiModedial);
			}
		}
	}
}

void GxKey_ForceStatusKeyDet(STATUS_KEY_GROUP Group)
{
	UINT32          uiCurMDStatus;

	uiCurMDStatus       = DrvKey_DetStatusKey(Group);
	if (uiCurMDStatus != 0) {
		g_uiStableMDStatus[Group] = g_uiLastMDStatus[Group] = uiCurMDStatus;
		if (g_fpKeyCB) {
			g_fpKeyCB(KEY_CB_STATUS, (UINT32)Group, uiCurMDStatus);
		}
	}

}
//#NT#2010/08/02#Ben Wang -begin
void GxKey_SetFirstKeyInvalid(KEY_STATUS status, UINT32 uiKey)
{
	switch (status) {
	case KEY_PRESS:
		g_uiFirstKeyInvalidPress      = uiKey;
		break;

	case KEY_RELEASE:
		g_uiFirstKeyInvalidRelease    = uiKey;
		break;

	case KEY_CONTINUE:
		g_uiFirstKeyInvalidContinue   = uiKey;
		break;

	default:
		break;
	}

}
//#NT#2010/08/02#Ben Wang -end
void Key_OnSystem(int cmd)
{
	switch (cmd) {
	case SYSTEM_CMD_POWERON:
		if (g_fpKeyCB) {
			(*g_fpKeyCB)(SYSTEM_CB_CONFIG, 0, 0);
		}
		//#NT#2010/08/02#Ben Wang -begin
		GxKey_DetStatusKey();
		//#NT#2010/08/02#Ben Wang -end
		break;

	case SYSTEM_CMD_POWEROFF:
		break;
	default:
		break;
	}
}


UINT32 GxKey_GetData(GX_KEY_DATA attribute)
{
	switch (attribute) {
	case GXKEY_NORMAL_KEY:
		return (DrvKey_DetPowerKey() | DrvKey_DetNormalKey());
	case GXKEY_STS_KEY1:
		return DrvKey_DetStatusKey(STATUS_KEY_GROUP1);
	case GXKEY_STS_KEY2:
		return DrvKey_DetStatusKey(STATUS_KEY_GROUP2);
	case GXKEY_STS_KEY3:
		return DrvKey_DetStatusKey(STATUS_KEY_GROUP3);
	case GXKEY_STS_KEY4:
		return DrvKey_DetStatusKey(STATUS_KEY_GROUP4);
	case GXKEY_STS_KEY5:
		return DrvKey_DetStatusKey(STATUS_KEY_GROUP5);
	default:
		DBG_ERR("[GxKey_GetData]no this attribute");
		return 0;
	}

}


