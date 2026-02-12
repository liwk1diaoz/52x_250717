/*
    Software timer module.

    Software timer module. Provide inaccurate timer for low priority
    or tolerance allowed purpose.

    @file       SwTimer.c
    @ingroup    mIUtilSwTimer
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "timer.h"
#include "SwTimer_int.h"
#include "comm/hwclock.h"

#define TIMER_LOSING_EVENT_SILENCE          ENABLE

#define wai_sem vos_sem_wait
#define sig_sem vos_sem_sig
//static  VK_DEFINE_SPINLOCK(my_lock);
//#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
//#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#define Delay_DelayMs(ms) vos_util_delay_ms(ms)

// Task, Flag and semaphore ID
extern THREAD_HANDLE uiSwTimerTaskID;
extern ID uiSwTimerSemID;
extern ID uiSwTimerFlagID[SWTIMER_GROUP_CNT];

// bit-wise open status
// 0: Close
// 1: Open
static UINT32   uiSwTimerOpen[SWTIMER_GROUP_CNT]  = { 0 };
#define SwTimer_ChgStatusOpen(id)       (uiSwTimerOpen[SWTIMER_ID_2_GROUP(id)] |= SWTIMER_ID_2_BIT(id))
#define SwTimer_ChgStatusClose(id)      (uiSwTimerOpen[SWTIMER_ID_2_GROUP(id)] &= ~(SWTIMER_ID_2_BIT(id)))
#define SwTimer_IsOpen(id)              ((uiSwTimerOpen[SWTIMER_ID_2_GROUP(id)] & SWTIMER_ID_2_BIT(id)) != 0)
#define SwTimer_IsClose(id)             ((uiSwTimerOpen[SWTIMER_ID_2_GROUP(id)] & SWTIMER_ID_2_BIT(id)) == 0)

// bit-wise play status
// 0: Stop
// 1: Play
static UINT32   uiSwTimerPlay[SWTIMER_GROUP_CNT]  = { 0 };
#define SwTimer_ChgStatusPlay(id)       (uiSwTimerPlay[SWTIMER_ID_2_GROUP(id)] |= SWTIMER_ID_2_BIT(id))
#define SwTimer_ChgStatusStop(id)       (uiSwTimerPlay[SWTIMER_ID_2_GROUP(id)] &= ~(SWTIMER_ID_2_BIT(id)))
#define SwTimer_IsPlay(id)              ((uiSwTimerPlay[SWTIMER_ID_2_GROUP(id)] & SWTIMER_ID_2_BIT(id)) != 0)
#define SwTimer_IsStop(id)              ((uiSwTimerPlay[SWTIMER_ID_2_GROUP(id)] & SWTIMER_ID_2_BIT(id)) == 0)

// bit-wise mode
// 0: One-shot
// 1: Free-run
static UINT32   uiSwTimerMode[SWTIMER_GROUP_CNT]  = { 0 };
#define SwTimer_ChgModeFreerun(id)      (uiSwTimerMode[SWTIMER_ID_2_GROUP(id)] |= SWTIMER_ID_2_BIT(id))
#define SwTimer_ChgModeOneshot(id)      (uiSwTimerMode[SWTIMER_ID_2_GROUP(id)] &= ~(SWTIMER_ID_2_BIT(id)))
#define SwTimer_IsFreerun(id)           ((uiSwTimerMode[SWTIMER_ID_2_GROUP(id)] & SWTIMER_ID_2_BIT(id)) != 0)
#define SwTimer_IsOneshot(id)           ((uiSwTimerMode[SWTIMER_ID_2_GROUP(id)] & SWTIMER_ID_2_BIT(id)) == 0)

// The real HW timer ID for software timer module
static TIMER_ID SwTimerHWID = TIMER_INVALID;

// Current resolution
static UINT32   uiSwTimerCurRes     = 10 * 1000;
// New resolution (set by upper layer and not update to software timer yet)
static UINT32   uiSwTimerNewRes     = 10 * 1000;
// How many software timer is locked
static UINT32   uiSwTimerLockNum    = 0;

static SWTIMER_INFO SwTimerInfo[SWTIMER_NUM] = { 0 };

THREAD_RETTYPE SwTimer_Task( void * pvParameters );

/**
    @addtogroup mIUtilSwTimer
*/
//@{

/**
    Init and start the software timer module

    Init and start the software timer module.

    @note   This is the software timer module's 2nd API that system should call.
            And must call once only.

    @return void
*/
void SwTimer_Init(void)
{
	SwTimer_InstallID();

	// Init already done?
	if (SwTimerHWID != TIMER_INVALID) {
		DBG_ERR("Calling multiple times!\r\n");
		return;
	}
	// Open HW timer
	if (timer_open(&SwTimerHWID, NULL) != E_OK) {
		DBG_ERR("No available HW timer!\r\n");
		return;
	}

#if (TIMER_LOSING_EVENT_SILENCE == ENABLE)
	int h_timer;
	char timer_silence_info[4];
	snprintf(timer_silence_info, 3, "%d", SwTimerHWID);
	if ((h_timer = open("/proc/nvt_timer_module/silence", O_RDWR)) >= 0) {
		write(h_timer, timer_silence_info, (strlen(timer_silence_info) + 1));
		close(h_timer);
	}
#endif

	uiSwTimerTaskID = vos_task_create(SwTimer_Task, 0, "SwTimer", SWTIMER_PRIORITY, SWTIMER_STKSIZE);
	if (0 == uiSwTimerTaskID) {
		DBG_ERR("SwTimer create failed\r\n");
        return;
	}
	vos_task_resume(uiSwTimerTaskID);
}

/**
    Open software timer

    One of available software timers will be allocated and the timer ID will be
    returned.

    @param[out] pSwTimerID      Software Timer ID that is allocated.
    @param[in] EventHandler     Event handler that will be called when timer expired.
                                Assign NULL if the handler is not required.

    @return Open status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Parameters are not valid
        - @b E_SYS  : Maximum number of software timers is exceeded or software timer is not init yet
*/
ER SwTimer_Open(PSWTIMER_ID pSwTimerID, SWTIMER_CB EventHandler)
{
	UINT32 i;
	//unsigned long flags;

	// Not init yet?
	if (SwTimerHWID == TIMER_INVALID) {
		DBG_ERR("SwTimer is not init yet!\r\n");
		return E_SYS;
	}

	// Accquire available software timer ID
	if (uiSwTimerLockNum == SWTIMER_NUM) {
		DBG_ERR("No SwTimer!\r\n");
		*pSwTimerID = SWTIMER_INVALID;
		return E_SYS;
	}

	// Wait semaphore
	wai_sem(uiSwTimerSemID);

	//loc_cpu(flags);

	// Update locked software timer number
	uiSwTimerLockNum++;

	// Find available ID
	i = 0;
	while (1) {
		if (SwTimer_IsClose(i)) {
			break;
		}

		i++;
	}

	*pSwTimerID = i;

	// Well, this software timer is now opened and stopped
	SwTimer_ChgStatusOpen(*pSwTimerID);
	SwTimer_ChgStatusStop(*pSwTimerID);

	// Update event handler
	SwTimerInfo[*pSwTimerID].EventHandler   = EventHandler;

	//unl_cpu(flags);
	sig_sem(uiSwTimerSemID);

	return E_OK;
}

/**
    Close software timer

    The specified software timer ID will be closed and resource will be released.

    @param[in] SwTimerID    The software timer ID to be closed

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid software timer ID
        - @b E_SYS  : Software timer is not opened
*/
ER SwTimer_Close(SWTIMER_ID SwTimerID)
{
	//unsigned long flags;

	// Check Timer ID
	if (SwTimerID >= SWTIMER_NUM) {
		DBG_ERR("Invalid SwTimer ID (%ld)\r\n", SwTimerID);
		return E_PAR;
	}

	// Check if software timer is opened
	if (SwTimer_IsClose(SwTimerID)) {
		DBG_ERR("SwTimer %ld is not opened!\r\n", SwTimerID);
		return E_SYS;
	}

	//loc_cpu(flags);
	wai_sem(uiSwTimerSemID);

	// Update locked software timer number
	uiSwTimerLockNum--;

	// Well, this software timer is now closed and stopped
	SwTimer_ChgStatusClose(SwTimerID);
	SwTimer_ChgStatusStop(SwTimerID);

	//unl_cpu(flags);

	// Release resource
	sig_sem(uiSwTimerSemID);
	return E_OK;
}

/**
    Set software timer parameter

    Set the software timer expired interval. one-shot or free-run mode.

    @param[in] SwTimerID    The software timer ID to be configured.
    @param[in] iInterval    Software timer expired interval (1 ~ 2,147,483), unit: ms
    @param[in] SwTimerMode  Software timer operating mode.

    @note   Software timer will stopped. You have to call SwTimer_Start() to start counting
            after calling this API.

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid software timer ID, interval or mode
        - @b E_SYS  : Software timer is not init, opened
*/
ER SwTimer_Cfg(SWTIMER_ID SwTimerID, INT32 iInterval, SWTIMER_MODE SwTimerMode)
{
	BOOL bRunning = FALSE;
	//unsigned long flags;

	// Not init yet?
	if (SwTimerHWID == TIMER_INVALID) {
		DBG_ERR("SwTimer is not init yet!\r\n");
		return E_SYS;
	}

	// Check Timer ID, interval and mode
	if (
		(SwTimerID >= SWTIMER_NUM) ||
		(iInterval < SWTIMER_MIN_INTERVAL) || (iInterval > SWTIMER_MAX_INTERVAL) ||
		((SwTimerMode & ~SWTIMER_MODE_FREE_RUN) != 0)
	) {
		DBG_ERR("Invalid SwTimer ID (%ld), interval (%ld) or mode (%ld)!\r\n", SwTimerID, iInterval, SwTimerMode);
		return E_PAR;
	}

	// Check if software timer is opened
	if (SwTimer_IsClose(SwTimerID)) {
		DBG_ERR("SwTimer %ld is not opened!\r\n", SwTimerID);
		return E_SYS;
	}

	// Check if sotware timer is running
	if (SwTimer_IsPlay(SwTimerID)) {
		bRunning = TRUE;
	}

	// ms to us (HW timer's tick)
	iInterval *= 1000;

	//loc_cpu(flags);
	wai_sem(uiSwTimerSemID);

	// Update interval
	SwTimerInfo[SwTimerID].iInterval = iInterval;

	// Stop software timer
	SwTimer_ChgStatusStop(SwTimerID);

	// Update mode
	if (SwTimerMode == SWTIMER_MODE_ONE_SHOT) {
		SwTimer_ChgModeOneshot(SwTimerID);
	} else {
		SwTimer_ChgModeFreerun(SwTimerID);
	}

	//unl_cpu(flags);
	sig_sem(uiSwTimerSemID);

	if (bRunning == TRUE) {
		DBG_ERR("SwTimer %ld is running! We stop it!\r\n", SwTimerID);
	}

	return E_OK;
}

/**
    Start software timer

    Start software timer.

    @param[in] SwTimerID    The software timer ID which you would like to start

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid software timer ID
        - @b E_SYS  : Software timer is not opened
*/
ER SwTimer_Start(SWTIMER_ID SwTimerID)
{
	//unsigned long flags;

	// Check Timer ID
	if (SwTimerID >= SWTIMER_NUM) {
		DBG_ERR("Invalid SwTimer ID (%ld)!\r\n", SwTimerID);
		return E_PAR;
	}

	// Check if software timer is opened
	if (SwTimer_IsClose(SwTimerID)) {
		DBG_ERR("SwTimer %ld is not opened!\r\n", SwTimerID);
		return E_SYS;
	}

	// Clear flag pattern
	clr_flg(uiSwTimerFlagID[SWTIMER_ID_2_GROUP(SwTimerID)], SWTIMER_ID_2_BIT(SwTimerID));

	//loc_cpu(flags);
	wai_sem(uiSwTimerSemID);

	// HW timer is keep running, we have to consider current count (unit: us) to make sure
	// expired period won't shorter than expected
	SwTimerInfo[SwTimerID].iCount = 0 - (INT32)timer_get_current_count(SwTimerHWID);
	SwTimer_ChgStatusPlay(SwTimerID);

	//unl_cpu(flags);
	sig_sem(uiSwTimerSemID);

	return E_OK;
}

/**
    Stop software timer

    Stop software timer.

    @param[in] SwTimerID    The software timer ID which you would like to stop

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid software timer ID
        - @b E_SYS  : Software timer is not opened
*/
ER SwTimer_Stop(SWTIMER_ID SwTimerID)
{
	//unsigned long flags;
	// Check Timer ID
	if (SwTimerID >= SWTIMER_NUM) {
		DBG_ERR("Invalid SwTimer ID (%ld)!\r\n", SwTimerID);
		return E_PAR;
	}

	// Check if software timer is opened
	if (SwTimer_IsClose(SwTimerID)) {
		DBG_ERR("SwTimer %ld is not opened!\r\n", SwTimerID);
		return E_SYS;
	}

	//loc_cpu(flags);
	wai_sem(uiSwTimerSemID);

	SwTimer_ChgStatusStop(SwTimerID);

	//unl_cpu(flags);
	sig_sem(uiSwTimerSemID);

	return E_OK;
}

/**
    Wait for software timer expired

    Wait for software timer expired.

    @param[in] SwTimerID    The software timer ID you would like to wait

    @return Operation status
        - @b E_OK   : Everything is OK
        - @b E_PAR  : Invalid software timer ID
        - @b E_SYS  : Software timer is not opened and running
*/
ER SwTimer_WaitTimeup(SWTIMER_ID SwTimerID)
{
	FLGPTN  FlagPtn;
	BOOL bIsStop;
	FLGPTN tmrFlag;

	// Check Timer ID
	if (SwTimerID >= SWTIMER_NUM) {
		DBG_ERR("Invalid SwTimer ID (%ld)!\r\n", SwTimerID);
		return E_PAR;
	}

	// Check if software timer is opened
	if (SwTimer_IsClose(SwTimerID)) {
		DBG_ERR("SwTimer %ld is not opened!\r\n", SwTimerID);
		return E_SYS;
	}

	// Check if software timer is running
	// If flag pattern is set, that means a valid running timer (one-shot, run and stop)
	bIsStop = SwTimer_IsStop(SwTimerID);
	tmrFlag = kchk_flg(uiSwTimerFlagID[SWTIMER_ID_2_GROUP(SwTimerID)], SWTIMER_ID_2_BIT(SwTimerID));
	if (bIsStop && (tmrFlag == 0) && SwTimer_IsOneshot(SwTimerID))
//    if (SwTimer_IsStop(SwTimerID) &&
//        (kchk_flg(uiSwTimerFlagID[SWTIMER_ID_2_GROUP(SwTimerID)], SWTIMER_ID_2_BIT(SwTimerID)) == 0))
	{
		DBG_ERR("SwTimer %ld is not running!\r\n", SwTimerID);
		DBG_ERR("stop: 0x%lx, flag: 0x%lx\r\n", bIsStop, tmrFlag);
		return E_SYS;
	}

	// Wait for flag pattern
	vos_flag_wait_interruptible(&FlagPtn,
			uiSwTimerFlagID[SWTIMER_ID_2_GROUP(SwTimerID)],
			SWTIMER_ID_2_BIT(SwTimerID),
			TWF_ORW | TWF_CLR);

	return E_OK;
}

/**
    Configure software timer

    Configure software timer.

    @param[in] ConfigID     Configuration ID
    @param[in] uiConfig     Configuration value
    @return void
*/
void SwTimer_SetConfig(SWTIMER_CONFIG_ID ConfigID, UINT32 uiConfig)
{
	if (ConfigID != SWTIMER_CONFIG_ID_RESOLUTION) {
		DBG_ERR("Not supported ID (%ld)!\r\n", ConfigID);
		return;
	}

	if ((uiConfig < SWTIMER_MIN_RESOLUTION) || (uiConfig > SWTIMER_MAX_RESOLUTION)) {
		DBG_ERR("Invalid config value (%ld)!\r\n", uiConfig);
		return;
	}

	uiSwTimerNewRes = uiConfig * 1000;
}

/**
    Get software timer configuration

    Get software timer configuration.

    @param[in] ConfigID     Configuration ID
    @return Configuration value.
*/
UINT32 SwTimer_GetConfig(SWTIMER_CONFIG_ID ConfigID)
{
	if (ConfigID != SWTIMER_CONFIG_ID_RESOLUTION) {
		DBG_ERR("Not supported ID (%ld)!\r\n", ConfigID);
		return 0;
	}

	if (uiSwTimerNewRes != uiSwTimerCurRes) {
		return (uiSwTimerNewRes / 1000);
	} else {
		return (uiSwTimerCurRes / 1000);
	}
}

/**
    Delay in millisecond using software timer

    Delay in millisecond using software timer. The actual delay time might
    be greater than expected. The caller will sleep and release CPU until
    delay timer is expired.

    @param[in] uiMS     Number of millisecond to delay. 1 ~ 2,147,483.
    @return Void.
*/
void SwTimer_DelayMs(UINT32 uiMS)
{
	SWTIMER_ID  SwTimerID;

	// Check interval
	if ((uiMS < SWTIMER_MIN_INTERVAL) ||
		(uiMS > SWTIMER_MAX_INTERVAL)) {
		DBG_ERR("Invalid parameter!\r\n");
		return;
	}

	// Open software timer
	// There is no available software timer, try Delay_DelayMs()
	if (SwTimer_Open(&SwTimerID, NULL) != E_OK) {

		Delay_DelayMs(uiMS);
		DBG_ERR("No SwTimer!\r\n");
	}
	// Configure software timer and wait for software timer expired
	else {
		SwTimer_Cfg(SwTimerID, uiMS, SWTIMER_MODE_ONE_SHOT);
		SwTimer_Start(SwTimerID);
		SwTimer_WaitTimeup(SwTimerID);
		SwTimer_Close(SwTimerID);
	}
}

//@}

/*
    Software timer task

    Software timer task.

    @return void
*/
THREAD_RETTYPE SwTimer_Task( void * pvParameters )
{
	FLGPTN  FlagPtn[SWTIMER_GROUP_CNT];
	FLGPTN  TimeOver[SWTIMER_GROUP_CNT];
	UINT32  i, uiResolution;
	//unsigned long flags;

	// Must be the first statement of each task
	THREAD_ENTRY();

	if (uiSwTimerNewRes != uiSwTimerCurRes) {
		uiSwTimerCurRes = uiSwTimerNewRes;
	}

	// Configure timer to N ms, free run and interrupt enabled mode. And start timer.
	timer_cfg(SwTimerHWID,
			  uiSwTimerCurRes,
			  TIMER_MODE_CLKSRC_DIV0 | TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT,
			  TIMER_STATE_PLAY);

	//coverity[no_escape]
	while (1) {
		// Assume each callback execution will < 1ms
		memset(TimeOver, 0, sizeof(TimeOver));

		PROFILE_TASK_IDLE();
		timer_wait_timeup(SwTimerHWID);
		PROFILE_TASK_BUSY();

		// Save current resolution
		uiResolution = uiSwTimerCurRes;

		// Update resolution for next expired timer
		if (uiSwTimerNewRes != uiSwTimerCurRes) {
			uiSwTimerCurRes = uiSwTimerNewRes;

			// Restart HW timer in new resolution
			timer_cfg(SwTimerHWID,
					  uiSwTimerCurRes,
					  TIMER_MODE_CLKSRC_DIV0 | TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT,
					  TIMER_STATE_PLAY);
		}

		// Init Flag pattern
		for (i = 0; i < SWTIMER_GROUP_CNT; i++) {
			FlagPtn[i] = 0;
		}

		//loc_cpu(flags);
		wai_sem(uiSwTimerSemID);

		for (i = 0; i < SWTIMER_NUM; i++) {
			// Software timer is running
			if (SwTimer_IsPlay(i)) {
				// Update count
				SwTimerInfo[i].iCount += (INT32)uiResolution;

				// Software timer is expired
				if (SwTimerInfo[i].iCount >= SwTimerInfo[i].iInterval) {
					// Update base
					SwTimerInfo[i].iCount = 0;

					// Update flag pattern
					FlagPtn[SWTIMER_ID_2_GROUP(i)] |= SWTIMER_ID_2_BIT(i);

					// One-shot timer
					if (SwTimer_IsOneshot(i)) {
						// Stop the timer
						SwTimer_ChgStatusStop(i);
					}
				}
			}
		}

		//unl_cpu(flags);
		sig_sem(uiSwTimerSemID);

		// Set flag pattern
		for (i = 0; i < SWTIMER_GROUP_CNT; i++) {
			if (FlagPtn[i] != 0) {
				set_flg(uiSwTimerFlagID[i], FlagPtn[i]);
			}
		}

		// Process callback
		for (i = 0; i < SWTIMER_NUM; i++) {
			if ((FlagPtn[SWTIMER_ID_2_GROUP(i)] & SWTIMER_ID_2_BIT(i)) &&
				(SwTimerInfo[i].EventHandler != NULL)) {
				UINT32 uiBegin = timer_get_current_count(TIMER_SYSTIMER_ID);
				SwTimerInfo[i].EventHandler(i);
				UINT32 uiDiff = timer_get_current_count(TIMER_SYSTIMER_ID) - uiBegin;
				if (uiDiff > 1000) {
					TimeOver[SWTIMER_ID_2_GROUP(i)] |= SWTIMER_ID_2_BIT(i);
//                    DBG_ERR("0x%08X ran over than 1 ms (%d us)\r\n",SwTimerInfo[i].EventHandler,uiDiff);
				}
			}
		}

		// Dump callback which execution time is too long
		for (i = 0; i < SWTIMER_NUM; i++) {
			if (TimeOver[SWTIMER_ID_2_GROUP(i)] & SWTIMER_ID_2_BIT(i)) {
				DBG_ERR("0x%08X ran over than 1 ms\r\n", SwTimerInfo[i].EventHandler);
			}
		}
	}
	THREAD_RETURN(0);
}

