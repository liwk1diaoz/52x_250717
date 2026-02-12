/*
    System Audio Callback

    System Callback for Audio Module.

    @file       SysLens_Audio.c
    @ingroup    mIPRJSYS

    @note

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

////////////////////////////////////////////////////////////////////////////////
#include "Dx.h"
#include "DxApi.h"
#include "GxSound.h"
#include "SysCommon.h"
#include "sys_mempool.h"
#include "UI/UIGraphics.h"
#include "vendor_common.h"

#if(UI_FUNC==ENABLE)
#include "UIWnd/UIFlow.h"
#endif

//global debug level: PRJ_DBG_LVL
#include "PrjInc.h"
//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysAudioExe
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>


#if (AUDIO_FUNC == ENABLE)

void System_SetAudioOutput(void);
void Audio_DetAudInsert(void);

void System_OnAudioInit(void)
{
	TM_BOOT_BEGIN("audio", "init");
	//PHASE-1 : Init & Open Drv or DrvExt
	{
		SOUND_MEM mem = {0};
		mem.pa = mempool_gxsound_pa;
		mem.va = mempool_gxsound_va;
		mem.size = POOL_SIZE_GXSOUND;
		GxSound_Set_Config(SOUND_CONFIG_MEM, (UINT32)&mem);
		GxSound_Open(NULL);
#if(UI_FUNC==ENABLE)
		UISound_RegTable();
#endif
	}
	//PHASE-2 : Init & Open Lib or LibExt
	{
		//config audio Output
		System_SetAudioOutput();
	}
	TM_BOOT_END("audio", "init");
}

void System_OnAudioExit(void)
{
	//PHASE-2 : Close Lib or LibExt
	{

	}
	//PHASE-1 : Close Drv or DrvExt
	{
		GxSound_Close();
	}
}

///////////////////////////////////////////////////////////////////////
void Audio_DetAudInsert(void)
{
	//DBG_MSG("\r\n");
}

///////////////////////////////////////////////////////////////////////

void System_SetAudioOutput(void)
{
	//=========================================================================
	// Change Audio Output to default
	//=========================================================================
	GxSound_SetOutDevConfigIdx(0);
}

///////////////////////////////////////////////////////////////////////
//Device flow event

INT32 System_OnAudioInsert(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	DBG_DUMP("^Y===================================== audio plug [%s]\r\n", "LINE");

	return NVTEVT_CONSUME;
}

INT32 System_OnAudioRemove(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	DBG_DUMP("^Y===================================== audio unplug [%s]\r\n", "LINE");

	return NVTEVT_CONSUME;
}


INT32 System_OnAudioAttach(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	//  AUDIO: if(boot)->play boot sound
	//  AUDIO: if(boot)->start audio srv
	return NVTEVT_CONSUME;
}

INT32 System_OnAudioDetach(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}

INT32 System_OnAudioMode(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}

INT32 System_OnAudioInput(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}

INT32 System_OnAudioVol(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}

BOOL System_GetAudioOut(void)
{
	return 0;
}

BOOL System_GetAudioMode(void)
{
	return 0;
}

BOOL System_GetAudioInput(void)
{
	return 0;
}

UINT32 System_GetAudioVol(void)
{
	return 0;
}

INT32 System_PutAudioData(UINT32 did, UINT32 addr, UINT32 *ptrsize)
{
	return 0;
}

//#NT#2016/09/08#HM Tseng -begin
//#NT#Support audio channel
UINT32 System_GetAudioChannel(void)
{
	return 0;
}
//#NT#2016/09/08#HM Tseng -end

UINT32 System_SwtichAudioSetting(UINT32 setting)
{
	return 0;
}

UINT32 System_GetAudioSetting(void)
{
	return 0;
}


#endif

