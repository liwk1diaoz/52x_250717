/**
    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.

    @file       GxTimer.c
    @ingroup    mIPRJAPKeyIO

    @brief      Detect Sx timer count  to send timer event for UI

    @note       Nothing.

    @date       2009/04/24
*/

/** \addtogroup mIPRJAPKeyIO */
//@{


#include "GxTimer.h"
#include "GxTimer_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxTimer
#define __DBGLVL__          1 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#define wai_sem vos_sem_wait
#define sig_sem vos_sem_sig


// Timer
#define TIMER_NUM                       5

typedef struct _TIMER_INFO {
	//BOOL        isBusy;
	UINT32      interval;
	UINT32      eventID;
	UINT32      counter;
	TIMER_TYPE  type;
	BOOL        isEnabled;
} TIMER_INFO;

static TIMER_INFO  _sys_timer[TIMER_NUM] = {0};
static BOOL b_IsDetTimerInit = FALSE;
static UINT32 g_TimerBaseMs = 20;//ms

/**
  GxTimer_InitTimer

  Start a timer control array

  @param void
  @return void
*/
static void GxTimer_InitTimer(void)
{
	UINT32   i;
	if (b_IsDetTimerInit) {
		return;
	}
	for (i = 0; i < TIMER_NUM; i++) {
		_sys_timer[i].counter = 0;
		_sys_timer[i].interval = 0;
		_sys_timer[i].eventID = 0;
		_sys_timer[i].isEnabled = FALSE;
		_sys_timer[i].type = ONE_SHOT;
	}
	//xGxTimer_InstallCmd();
	b_IsDetTimerInit = TRUE;
}


void GxTimer_Init(void)
{
	GxTimer_InstallID();
}


/**
  GxTimer_GetFreeTimer

  Get a free timer of timer control array

  @param void
  @return void
*/
static UINT32 GxTimer_GetFreeTimer(void)
{
	UINT32   i;
	for (i = 0; i < TIMER_NUM; i++) {
		if (_sys_timer[i].isEnabled == FALSE) {
			return i;
		}
	}
	return NULL_TIMER;
}

/**
  GxTimer_LockTimer

  Get a sem to lock the resource

  @param void
  @return void
*/
static ER GxTimer_LockTimer(void)
{
	ER erReturn;
	erReturn = wai_sem(SEMID_GXTIMER);
	if (erReturn != E_OK) {
		return erReturn;
	}
	return E_OK;
}

/**
  GxTimer_UnlockTimer

  ulock the resource

  @param void
  @return void
*/
static void GxTimer_UnlockTimer(void)
{
	return sig_sem(SEMID_GXTIMER);
}


void GxTimer_CountTimer(void)
{
	UINT32      i;
	if (0 == SEMID_GXTIMER) {
		DBG_ERR("ID not installed!\r\n");
		return;
	}
	GxTimer_LockTimer();
	for (i = 0; i < TIMER_NUM; i++) {
		if (_sys_timer[i].isEnabled == TRUE) {
			_sys_timer[i].counter ++;
			if ((_sys_timer[i].counter * g_TimerBaseMs) >=  _sys_timer[i].interval) {
				if (g_fpTimerCB) {
					g_fpTimerCB(_sys_timer[i].eventID, 0, 0);
				}
				_sys_timer[i].counter = 0;

				if (_sys_timer[i].type == ONE_SHOT) {
					_sys_timer[i].eventID = 0;
					_sys_timer[i].isEnabled = FALSE;
				}
			}
		}
	}
	GxTimer_UnlockTimer();
}

UINT32  GxTimer_StartTimer(UINT32 interval, UINT32 TimerNameID, TIMER_TYPE type)
{
	//fix for CID 60992 - begin
	//UINT32 freeTimer = NULL_TIMER;
	UINT32 freeTimer;
	//fix for CID 60992 - end
	UINT32   i;
	GxTimer_LockTimer();
	GxTimer_InitTimer();

	//check duplicated timer
	for (i = 0; i < TIMER_NUM; i++) {
		if (_sys_timer[i].isEnabled == TRUE && _sys_timer[i].eventID == TimerNameID) {
			DBG_ERR("Duplicated eventID=%d (TimerID=%d)!!!\r\n ", TimerNameID, i);
		}
	}

	freeTimer = GxTimer_GetFreeTimer();
	if (freeTimer != NULL_TIMER) {
		_sys_timer[freeTimer].eventID = TimerNameID;
		_sys_timer[freeTimer].interval = interval;
		_sys_timer[freeTimer].isEnabled = TRUE;
		_sys_timer[freeTimer].type = type;
		_sys_timer[freeTimer].counter = 0;
	} else {
		UINT32 i = 0;
		DBG_ERR("GxTimer FULL!!!\r\n ");
		for (i = 0; i < TIMER_NUM; i++) {
			DBG_ERR("Timer eventID %d\r\n ", _sys_timer[i].eventID);
		}
	}
	GxTimer_UnlockTimer();

	return freeTimer;
}


void GxTimer_StopTimer(UINT32 *timerID)
{
	if (*timerID >= TIMER_NUM) {
		return;
	}
	GxTimer_LockTimer();
	_sys_timer[*timerID].counter = 0;
	_sys_timer[*timerID].interval = 0;
	_sys_timer[*timerID].eventID = 0;
	_sys_timer[*timerID].isEnabled = FALSE;
	_sys_timer[*timerID].type = ONE_SHOT;
	*timerID = NULL_TIMER;
	GxTimer_UnlockTimer();
}

void GxTimer_SetTimerBase(UINT32 TimerBaseMs)
{
	g_TimerBaseMs = TimerBaseMs;
}

void GxTimer_Dump(void)
{
	UINT32 i;
	DBG_DUMP("\r\n######## GxTimer (Base = %d ms)#######\r\n", g_TimerBaseMs);
	for (i = 0; i < TIMER_NUM; i++) {
		DBG_DUMP("Timer[%02d]= TimerNameID(%d), Interval=%d, Active=%d, Type=%d, Couting=%d \r\n",
				 i, _sys_timer[i].eventID,
				 _sys_timer[i].interval,
				 _sys_timer[i].isEnabled,
				 _sys_timer[i].type,
				 _sys_timer[i].counter);
	}
}

//@}
