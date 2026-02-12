/**
    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.

    @file       SysMain_Flow_Init.c
    @ingroup    mSystemFlow

    @brief      PowerOn Flow

    @note
                1.����power on������
                  System_PowerOn()
                  (a)System_PowerOn()����
                    �Umode���P��control condition
                    �]�t�q�����Pdevice init
                  (b)System_PowerOn()����
                    �Umode���P��control condition
                    �]�t����Pdevice�����ۤ�dependent��order

    @date       2012/1/1
*/

////////////////////////////////////////////////////////////////////////////////
#include "PrjInc.h"

//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysInit
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

/////////////////////////////////////////////////////////////////////////////
//#include "DeviceSysMan.h"
//#include "DeviceNandMan.h"
//#include "NvtSystem.h"

/////////////////////////////////////////////////////////////////////////////

#if (POWERON_FAST_BOOT == ENABLE)
INT32 g_iSysPowerOnMode = SYS_POWERON_NORMAL;
#else
INT32 g_iSysPowerOnMode = SYS_POWERON_SAFE;
#endif

/**
  System initialization

  Do system initialization

  @param void
  @return void
*/
void System_PowerOn(UINT32 pwrOnType)
{
	DBG_FUNC_BEGIN("\r\n");
	g_iSysPowerOnMode = pwrOnType;

	//Perf_Open();

	DBG_FUNC_END("\r\n");

	//avoid event in queue,and execute at the same time
	UI_UnlockEvent();
}


void System_BootStart(void)
{
	//DBG_DUMP("^MSystem Boot start\r\n");
}



///////////////////////////////////////////////////////////////////////////////
//
//  END
//
///////////////////////////////////////////////////////////////////////////////

void System_BootEnd(void)
{
	//Enable_WP
	//DBG_DUMP("^MSystem Boot end\r\n");
}


void SystemInit(void)
{
	DBG_MSG("^GInit Start\r\n");
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_SAFE) {
		//"safe power-on sequence"
		//DBG_DUMP("Power On Sequence = Safe\r\n");
#if (FWS_FUNC == ENABLE)
		System_OnStrgInit_EMBMEM();
		System_OnStrgInit_FWS();
		System_OnStrg_DownloadFW();
#endif
#if (DISPLAY_FUNC == ENABLE)
		System_OnVideoInit();
		System_OnVideoInit2();
#endif
#if (FS_FUNC == ENABLE)
		System_OnStrgInit_EXMEM();
#endif
#if (PST_FUNC == ENABLE)
		System_OnStrgInit_PS();
#endif
#if(ONVIF_PROFILE_S!=ENABLE) //No File System
#if (FS_FUNC == ENABLE)
		System_OnStrgInit_FS();
		//System_OnStrgInit_FS2();
		//System_OnStrgInit_FS3();
#endif
#endif
#if (PWR_FUNC == ENABLE)
		System_OnPowerInit();
#endif
#if (INPUT_FUNC == ENABLE)
		System_OnInputInit();
#endif
#if (AUDIO_FUNC == ENABLE)
		System_OnAudioInit();
#endif
	}
#if (POWERON_FAST_BOOT == ENABLE)
	else if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {
		///////////////////////////////////////////
		//Start Multi-UserMainProc

		INIT_SETFLAG(FLGINIT_BEGIN2);
		INIT_SETFLAG(FLGINIT_BEGIN3);
		INIT_SETFLAG(FLGINIT_BEGIN4);
		INIT_SETFLAG(FLGINIT_BEGIN5);
		INIT_SETFLAG(FLGINIT_BEGIN6);
		INIT_SETFLAG(FLGINIT_BEGIN7);
		INIT_SETFLAG(FLGINIT_BEGIN8);

		//"normal power-on sequence"
		DBG_DUMP("Power On Sequence = Normal\r\n");

		//PART-1

		//PART-2
		INIT_WAITFLAG(FLGINIT_END2);
		INIT_WAITFLAG(FLGINIT_END3); //display
		INIT_WAITFLAG(FLGINIT_END4); //audio
		//INIT_WAITFLAG(FLGINIT_END5); //do not wait FS finish
		INIT_WAITFLAG(FLGINIT_END6); //usb
		INIT_WAITFLAG(FLGINIT_END7); //sensor
		//INIT_WAITFLAG(FLGINIT_END8); //do not wait wifi finish
	}
#endif
	DBG_MSG("^GInit End\r\n");
}

#if (POWERON_FAST_BOOT == ENABLE)

void SystemInit2(void)
{
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {
		//INIT_WAITFLAG(FLGINIT_PWROPEN); //wait until PWR dummyload finish
		DBG_MSG("^GInit 2 Start\r\n");
		DBG_MSG("^GInit 2 End\r\n");
	}
}

void SystemInit3(void)
{
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {
		//INIT_WAITFLAG(FLGINIT_PWROPEN); //wait until PWR dummyload finish
		DBG_MSG("^GInit 3 Start\r\n");
		DBG_MSG("^GInit 3 End\r\n");
	}
}

void SystemInit4(void)
{
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {
		//INIT_WAITFLAG(FLGINIT_PWROPEN); //wait until PWR dummyload finish
		DBG_MSG("^GInit 4 Start\r\n");
		DBG_MSG("^GInit 4 End\r\n");
	}
}

void SystemInit5(void)
{
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {
		//INIT_WAITFLAG(FLGINIT_PWROPEN); //wait until PWR dummyload finish
		DBG_MSG("^GInit 5 Start\r\n");
		DBG_MSG("^GInit 5 End\r\n");
	}
}

void SystemInit6(void)
{
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {
		//INIT_WAITFLAG(FLGINIT_PWROPEN); //wait until PWR dummyload finish
		DBG_MSG("^GInit 6 Start\r\n");
		DBG_MSG("^GInit 6 End\r\n");
	}
}

void SystemInit7(void)
{
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {
		//INIT_WAITFLAG(FLGINIT_PWROPEN); //wait until PWR dummyload finish
		DBG_MSG("^GInit 7 Start\r\n");
		DBG_MSG("^GInit 7 End\r\n");
	}
}

void SystemInit8(void)
{
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {
		//INIT_WAITFLAG(FLGINIT_PWROPEN); //wait until PWR dummyload finish
		DBG_MSG("^GInit 8 Start\r\n");
		DBG_MSG("^GInit 8 End\r\n");
	}
}

#endif

