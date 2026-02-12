#ifdef __FREERTOS
#include "plat/rtc.h"
#include "kwrap/error_no.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"
#include "comm/timer.h"
#include "comm/hwclock.h"
#include "comm/hwpower.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "wdt.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          HwClk
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////


#define CFG_SUPPORT_DRTC 0
#define CFG_SUPPORT_WDT 1

static HWCLOCK_PERMISSION hwclock_permission = HWCLOCK_PERMISSION_READWRITE;
static HWCLOCK_MODE hwclock_mode = HWCLOCK_MODE_RTC; //0 for RTC, 1 for DRTC
static BOOL hwclock_opened = FALSE;

ER hwclock_open(HWCLOCK_MODE mode)
{
	ER ret = E_OK;

	if (hwclock_opened) {
		DBG_ERR("HwClock is already opened\r\n");
		return ret;
	}

	hwclock_mode = mode;

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		ret = rtc_open();
	} else {
#if (CFG_SUPPORT_DRTC)
		struct tm current_time = {0};

		ret = drtc_open();
		current_time.tm_year = DEFAULT_YEAR;
		current_time.tm_mon = DEFAULT_MON;
		current_time.tm_mday = DEFAULT_DAY;
		current_time.tm_hour = DEFAULT_HOUR;
		current_time.tm_min = DEFAULT_MIN;
		current_time.tm_sec = DEFAULT_SEC;
		HwClock_SetTime(TIME_ID_CURRENT, current_time, 0);
#endif
	}
	if (ret == E_OK) {
		hwclock_opened = TRUE;
	}
	return ret;
}

void hwclock_close(void)
{
	if (!hwclock_opened) {
		DBG_ERR("HwClock is not opened\r\n");
		return;
	}

	hwclock_opened = FALSE;
	hwclock_mode = 0;

	return;
}

ER hwclock_set_param(HWCLOCK_PARAM_ID id, UINT32 p1, UINT32 p2)
{
	ER ret = E_OK;

	switch (id) {
		case HWCLOCK_PARAM_ID_HWCLOCK_PERMISSION: {
				hwclock_permission = p1;
				break;
			}
		case HWCLOCK_PARAM_ID_READ_TIME_OFFSET: {
				if (hwclock_mode == HWCLOCK_MODE_RTC) {
					rtc_setReadTimeOffset(p1, p2);
				} else {
#if (CFG_SUPPORT_DRTC)
					drtc_set_read_timeoffset(p1, p2);
#endif
				}
				break;
			}
		default:
			DBG_ERR("Not supported ID (%ld)!\r\n", id);
			ret = E_PAR;
			break;
	}

	return ret;
}

UINT32 hwclock_get_param(HWCLOCK_PARAM_ID id, UINT32 p1)
{
	UINT32 ret = 0;

	switch (id) {
		case HWCLOCK_PARAM_ID_HWCLOCK_PERMISSION: {
				ret = hwclock_permission;
				break;
			}
		default:
			DBG_ERR("Not supported ID (%ld)!\r\n", id);
			break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Current Time
//-----------------------------------------------------------------------------

static struct tm _HwClock_GetCurrentTime(void)
{
	struct tm ctv = {0};

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		RTC_TIME stTime;
		RTC_DATE stDate;

		stTime = rtc_getTime();
		stDate = rtc_getDate();

		ctv.tm_sec = stTime.s.second;   /* seconds after the minute - [0,59] */
		ctv.tm_min = stTime.s.minute;   /* minutes after the hour - [0,59]   */
		ctv.tm_hour = stTime.s.hour;    /* hours since midnight - [0,23]     */
		ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		ctv.tm_year = stDate.s.year;    /* years since 2000                  */
	} else {
#if (CFG_SUPPORT_DRTC)
		DRTC_TIME stTime;
		DRTC_DATE stDate;

		stTime = drtc_get_time();
		stDate = drtc_get_date();

		ctv.tm_sec = stTime.s.second;   /* seconds after the minute - [0,59] */
		ctv.tm_min = stTime.s.minute;   /* minutes after the hour - [0,59]   */
		ctv.tm_hour = stTime.s.hour;    /* hours since midnight - [0,23]     */
		ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		ctv.tm_year = stDate.s.year;    /* years since 2000                  */
#endif
	}

	// NOT support the following function..
	ctv.tm_wday = 0;             /* days since Sunday - [0,6]         */
	ctv.tm_yday = 0;             /* days since January 1 - [0,365]    */
	ctv.tm_isdst = 0;            /* daylight savings time flag        */

	return ctv;
}

static void _HwClock_SetCurrentTime(struct tm ctv)
{
	if (hwclock_permission != HWCLOCK_PERMISSION_READWRITE) {
		DBG_ERR("HwClock is read only\r\n");
		return;
	}

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		RTC_TIME stTime;
		RTC_DATE stDate;

		stDate = rtc_getMaxDate();

		if (stDate.s.year < ctv.tm_year) {
			DBG_ERR("Date is overflow\r\n");
			return;
		}

		stTime.s.second = ctv.tm_sec;   /* seconds after the minute - [0,59] */
		stTime.s.minute = ctv.tm_min;  /* minutes after the hour - [0,59]   */
		stTime.s.hour = ctv.tm_hour;    /* hours since midnight - [0,23]     */
		stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		stDate.s.year = ctv.tm_year;    /* years since 2000                  */

		if (rtc_setDate(stDate.s.year, stDate.s.month, stDate.s.day) != E_OK) {
			DBG_IND("Set Date Fail:Y=%d,M=%d,D=%d\r\n", stDate.s.year, stDate.s.month, stDate.s.day);
			return;
		}
		if (rtc_setTime(stTime.s.hour, stTime.s.minute, stTime.s.second) != E_OK) {
			DBG_IND("Set Time Fail:H=%d,M=%d,S=%d\r\n", stTime.s.hour, stTime.s.minute, stTime.s.second);
			return;
		}
		rtc_triggerCSET();
	} else {
#if (CFG_SUPPORT_DRTC)
		DRTC_TIME stTime;
		DRTC_DATE stDate;

		stDate = drtc_get_maxdate();

		if (stDate.s.year < ctv.tm_year) {
			DBG_ERR("Date is overflow\r\n");
			return;
		}

		stTime.s.second = ctv.tm_sec;   /* seconds after the minute - [0,59] */
		stTime.s.minute = ctv.tm_min;  /* minutes after the hour - [0,59]   */
		stTime.s.hour = ctv.tm_hour;    /* hours since midnight - [0,23]     */
		stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		stDate.s.year = ctv.tm_year;    /* years since 2000                  */

		if (drtc_set_date(stDate.s.year, stDate.s.month, stDate.s.day) != E_OK) {
			DBG_IND("Set Date Fail:Y=%d,M=%d,D=%d\r\n", stDate.s.year, stDate.s.month, stDate.s.day);
			return;
		}
		if (drtc_set_time(stTime.s.hour, stTime.s.minute, stTime.s.second) != E_OK) {
			DBG_IND("Set Time Fail:H=%d,M=%d,S=%d\r\n", stTime.s.hour, stTime.s.minute, stTime.s.second);
			return;
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// RTC Alarm
//-----------------------------------------------------------------------------


static struct tm _HwClock_GetAlarmTime(void)
{
	struct tm ctv = {0};

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		RTC_TIME stTime;
		RTC_DATE stDate;

		rtc_getAlarm(&stDate, &stTime);

		ctv.tm_sec = stTime.s.second;   /* seconds after the minute - [0,59] */
		ctv.tm_min = stTime.s.minute;   /* minutes after the hour - [0,59]   */
		ctv.tm_hour = stTime.s.hour;    /* hours since midnight - [0,23]     */
		ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		ctv.tm_year = stDate.s.year;    /* years since 2000                  */
	} else {
#if (CFG_SUPPORT_DRTC)
		DRTC_TIME stTime;
		DRTC_DATE stDate;

		drtc_get_alarm(&stDate, &stTime);

		ctv.tm_sec = stTime.s.second;   /* seconds after the minute - [0,59] */
		ctv.tm_min = stTime.s.minute;   /* minutes after the hour - [0,59]   */
		ctv.tm_hour = stTime.s.hour;    /* hours since midnight - [0,23]     */
		ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		ctv.tm_year = stDate.s.year;    /* years since 2000                  */
#endif
	}

	// NOT support the following function..
	ctv.tm_wday = 0;             /* days since Sunday - [0,6]         */
	ctv.tm_yday = 0;             /* days since January 1 - [0,365]    */
	ctv.tm_isdst = 0;            /* daylight savings time flag        */

	return ctv;
}

static FP _HwClock_AlarmTime_EventCB = 0;
static void _HwClock_AlarmTimeCB(UINT32 uiEvent)
{
	if (_HwClock_AlarmTime_EventCB != 0) {
		_HwClock_AlarmTime_EventCB();
	}
}

static void _HwClock_SetAlarmTime(struct tm ctv, FP pEventCB)
{
	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		RTC_TIME stTime;
		RTC_DATE stDate;

		stDate = rtc_getMaxDate();

		if (stDate.s.year < ctv.tm_year) {
			DBG_ERR("Date is overflow\r\n");
			return;
		}

		stTime.s.second = ctv.tm_sec;   /* seconds after the minute - [0,59] */
		stTime.s.minute = ctv.tm_min;  /* minutes after the hour - [0,59]   */
		stTime.s.hour = ctv.tm_hour;    /* hours since midnight - [0,23]     */
		stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		stDate.s.year = ctv.tm_year;    /* years since 2000                  */

		_HwClock_AlarmTime_EventCB = pEventCB;
		if (rtc_setAlarm(stDate, stTime, _HwClock_AlarmTimeCB) != E_OK) {
			DBG_ERR("Set Date Fail:Y=%d,M=%d,D=%d\r\n", stDate.s.year, stDate.s.month, stDate.s.day);
			DBG_ERR("Set Time Fail:H=%d,M=%d,S=%d\r\n", stTime.s.hour, stTime.s.minute, stTime.s.second);
			return;
		}
		rtc_triggerCSET();
	} else {
#if (CFG_SUPPORT_DRTC)
		DRTC_TIME stTime;
		DRTC_DATE stDate;

		stDate = drtc_get_maxdate();

		if (stDate.s.year < ctv.tm_year) {
			DBG_ERR("Date is overflow\r\n");
			return;
		}

		stTime.s.second = ctv.tm_sec;   /* seconds after the minute - [0,59] */
		stTime.s.minute = ctv.tm_min;  /* minutes after the hour - [0,59]   */
		stTime.s.hour = ctv.tm_hour;    /* hours since midnight - [0,23]     */
		stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		stDate.s.year = ctv.tm_year;    /* years since 2000                  */

		_HwClock_AlarmTime_EventCB = pEventCB;
		if (drtc_set_alarm(stDate, stTime, _HwClock_AlarmTimeCB) != E_OK) {
			DBG_IND("Set Date Fail:Y=%d,M=%d,D=%d\r\n", stDate.s.year, stDate.s.month, stDate.s.day);
			DBG_IND("Set Time Fail:H=%d,M=%d,S=%d\r\n", stTime.s.hour, stTime.s.minute, stTime.s.second);
			return;
		}
		drtc_set_config(DRTC_CONFIG_ID_ALARM_PERIOD, DRTC_ALARM_PERIOD_ONCE);
		drtc_set_config(DRTC_CONFIG_ID_ALARM_EN, DRTC_ALARM_ENABLE);
#endif
	}
}

//-----------------------------------------------------------------------------
// RTC Power Alarm Date-Time
//-----------------------------------------------------------------------------
static struct tm _HwClock_GetPowerAlarmTime(void)
{
	struct tm ctv = {0};

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		RTC_TIME stTime;
		RTC_DATE stDate;

		rtc_getPWRAlarm(&stDate, &stTime);

		ctv.tm_sec = stTime.s.second;   /* seconds after the minute - [0,59] */
		ctv.tm_min = stTime.s.minute;   /* minutes after the hour - [0,59]   */
		ctv.tm_hour = stTime.s.hour;    /* hours since midnight - [0,23]     */
		ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		ctv.tm_year = stDate.s.year;    /* years since 2000                  */

		// NOT support the following function..
		ctv.tm_wday = 0;             /* days since Sunday - [0,6]         */
		ctv.tm_yday = 0;             /* days since January 1 - [0,365]    */
		ctv.tm_isdst = 0;            /* daylight savings time flag        */
	} else {
		DBG_WRN("Not support\r\n");
	}

	return ctv;
}

static void _HwClock_SetPowerAlarmTime(struct tm ctv)
{
	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		RTC_TIME stTime;
		RTC_DATE stDate;

		stTime.s.second = ctv.tm_sec;   /* seconds after the minute - [0,59] */
		stTime.s.minute = ctv.tm_min;  /* minutes after the hour - [0,59]   */
		stTime.s.hour = ctv.tm_hour;    /* hours since midnight - [0,23]     */
		stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		stDate.s.year = ctv.tm_year;    /* years since 2000                  */

		rtc_setPWRAlarm(stDate, stTime);
		rtc_triggerCSET();
	} else {
		DBG_WRN("Not support\r\n");
	}
}

//-----------------------------------------------------------------------------
// Date-Time util
//-----------------------------------------------------------------------------

BOOL timeutil_is_leap_year(UINT32 uiYear)
{
	BOOL ret = FALSE;

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		ret = rtc_isLeapYear(uiYear);
	} else {
#if (CFG_SUPPORT_DRTC)
		ret = drtc_isleapyear(uiYear);
#endif
	}

	return ret;
}

UINT32 timeutil_tm_to_days(struct tm ctv)
{
	UINT32 ret = 0;

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		RTC_DATE stDate;

		stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		stDate.s.year = ctv.tm_year;    /* years since 2000                  */
		ret = rtc_convertDate2Days(stDate);
	} else {
#if (CFG_SUPPORT_DRTC)
		DRTC_DATE stDate;

		stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		stDate.s.year = ctv.tm_year;    /* years since 2000                  */
		ret = drtc_convert_date2days(stDate);
#endif
	}

	return ret;
}

struct tm timeutil_days_to_tm(UINT32 days)
{
	struct tm ctv = {0};

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		RTC_DATE stDate;

		stDate = rtc_convertDays2Date(days);

		ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		ctv.tm_year = stDate.s.year;    /* years since 2000                  */
		ctv.tm_hour = 0;
		ctv.tm_min = 0;
		ctv.tm_sec = 0;
	} else {
#if (CFG_SUPPORT_DRTC)
		DRTC_DATE stDate;

		stDate = drtc_convert_days2date(days);

		ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		ctv.tm_year = stDate.s.year;    /* years since 2000                  */
		ctv.tm_hour = 0;
		ctv.tm_min = 0;
		ctv.tm_sec = 0;
#endif
	}

	// NOT support the following function..
	ctv.tm_isdst = 0;
	ctv.tm_yday = 0;
	ctv.tm_wday = 0;

	return ctv;
}

INT32 timeutil_calc_month_days(INT32 year, INT32 month)
{
	INT32 day = 31;
	INT32 year_ext = 0;
	if (timeutil_is_leap_year(year) == TRUE) {
		year_ext = 1;
	}
	if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) {
		day = 30;
	}
	if (month == 2) {
		day = 28 + year_ext;
	}
	return day;
}

struct tm timeutil_tm_add(struct tm ctv, struct tm diff)
{
	INT32 maxday;

	ctv.tm_sec += diff.tm_sec;   /* seconds after the minute - [0,59] */
	ctv.tm_min += diff.tm_min;   /* minutes after the hour - [0,59]   */
	ctv.tm_hour += diff.tm_hour;    /* hours since midnight - [0,23]     */
	ctv.tm_mday += diff.tm_mday;     /* day of the month - [1,31]         */
	ctv.tm_mon += diff.tm_mon;    /* months since January - [0,11]     */
	ctv.tm_year += diff.tm_year;    /* years since 2000                  */

	if (ctv.tm_sec >= 60) {
		ctv.tm_min += ctv.tm_sec / 60;
		ctv.tm_sec = ctv.tm_sec % 60;
	}
	if (ctv.tm_min >= 60) {
		ctv.tm_hour += ctv.tm_min / 60;
		ctv.tm_min = ctv.tm_min % 60;
	}
	if (ctv.tm_hour >= 24) {
		ctv.tm_mday += ctv.tm_hour / 24;
		ctv.tm_hour = ctv.tm_hour % 24;
	}
	maxday = timeutil_calc_month_days(ctv.tm_year, ctv.tm_mon);
	while (ctv.tm_mday > maxday) {
		ctv.tm_mday -= maxday;
		ctv.tm_mon ++;
		//month range = [1,12]
		if (ctv.tm_mon > 12) {
			ctv.tm_year += (ctv.tm_mon - 1) / 12;
			ctv.tm_mon = ((ctv.tm_mon - 1) % 12) + 1;
		}
		maxday = timeutil_calc_month_days(ctv.tm_year, ctv.tm_mon);
	}
	//month range = [1,12]
	if (ctv.tm_mon > 12) {
		ctv.tm_year += (ctv.tm_mon - 1) / 12;
		ctv.tm_mon = ((ctv.tm_mon - 1) % 12) + 1;
	}
	return ctv;
}

//-----------------------------------------------------------------------------
// Power Off
//-----------------------------------------------------------------------------

static void _HwClock_PowerOff_ClearCounter(void)
{
	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		rtc_resetShutdownTimer();
	} else {
		DBG_WRN("Not support\r\n");
	}
}

static BOOL gLoopQuit = FALSE;
extern void _HwClock_Quit(void);
void _HwClock_Quit(void)
{
	gLoopQuit = TRUE;
}

static void _HwClock_PowerOff(void)
{
	DBG_DUMP("(pwr-off)\r\n");

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
#if !defined(_NVT_EMULATION_)
		vos_util_delay_ms(800);
		rtc_poweroffPWR(); //this func will turn-off system power

		// Loop forever
		while (!gLoopQuit) { //fix for CID 43219
			;
		}
#else
		// Loop forever
		while (1) {
			;
		}
#endif
	} else {
		DBG_WRN("Not support\r\n");
		while (!gLoopQuit) {
			;
		}
	}
}

//-----------------------------------------------------------------------------
// Soft Reset (Watch Dog)
//-----------------------------------------------------------------------------

//#Fine tune the watch dog timer value
#define WDT_TIMER_INTERVAL         160
#define WDT_TIMEOUT                350     //350 ms

TIMER_ID     g_WatchDog_TimerID = TIMER_INVALID;
UINT32       g_WatchDog_Timeout = 0;
BOOL         g_WatchDog_Enable = 0;

static void _WatchDog_Isr(UINT32 uiEvent)
{
	// kick
#if (CFG_SUPPORT_WDT)
	wdt_trigger();
#endif
}
static void _HwClock_SoftReset_SetCounter(UINT32 uiTimeOut)
{
	if (g_WatchDog_TimerID != TIMER_INVALID) {
		return;    //alread enable
	}
	g_WatchDog_Timeout = uiTimeOut;
}
static void _HwClock_SoftReset_ClearCounter(void)
{
	// kick
#if (CFG_SUPPORT_WDT)
	wdt_trigger();
#endif
}

static void _HwClock_SoftReset_Enable(BOOL bEnable)
{
	if (bEnable) {
		if (g_WatchDog_Enable) {
			return;
		}

		if (timer_open(&g_WatchDog_TimerID, _WatchDog_Isr) != E_OK)
		{
			DBG_ERR("WDT timer open failed!\r\n");
			g_WatchDog_TimerID = 0xffffffffU;
			return;
		}

		//to avoid wdt already in use
#if (CFG_SUPPORT_WDT)
		wdt_close();
		DBG_DUMP("+(watch-dog)\r\n");
		vos_util_delay_ms(1000); //wait message output complet
		// Open
		wdt_open();
		// Config WDT to [uiTimeOut] ms, reset system
		wdt_setConfig(WDT_CONFIG_ID_MODE, WDT_MODE_RESET);
		wdt_setConfig(WDT_CONFIG_ID_TIMEOUT, g_WatchDog_Timeout);
		// Enable WDT
		wdt_enable();
#endif
		// Assign ISR
		timer_cfg(g_WatchDog_TimerID, 1000 * WDT_TIMER_INTERVAL, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		g_WatchDog_Enable = 1;
	} else {
		if (!g_WatchDog_Enable) {
			return;
		}

		if (g_WatchDog_TimerID != 0xffffffffU) {
			timer_close(g_WatchDog_TimerID);
			g_WatchDog_TimerID = 0;
		}
		DBG_DUMP("-(watch-dog)\r\n");

#if (CFG_SUPPORT_WDT)
		// Disable WDT
		wdt_disable();
		wdt_close();
#endif

		g_WatchDog_Enable = 0;
	}
}

static void _HwClock_SoftReset(void)
{
	_HwClock_SoftReset_Enable(FALSE);
	DBG_DUMP("(sw-reset)\r\n");
#if !defined(_NVT_EMULATION_)
	vos_util_delay_ms(1000); //wait message output complete
#endif

#if (CFG_SUPPORT_WDT)
	//setting wdt start
	// Open
	wdt_open();
	// Config WDT to 350 ms, reset system
	wdt_setConfig(WDT_CONFIG_ID_MODE, WDT_MODE_RESET);
	//#NT#2017/11/22#HM Tseng -begin
	//#NT#Only for normal reboot
	wdt_setConfig(WDT_CONFIG_ID_EXT_RESET, WDT_EXT_RESET);
	//#NT#2017/11/22#HM Tseng -end
	//wdt_setConfig(WDT_CONFIG_ID_TIMEOUT, WDT_TIMEOUT);
	// Enable WDT
	wdt_enable();
	// Trigger WDT immediately
	wdt_setConfig(WDT_CONFIG_ID_MANUAL_RESET, WDT_MANUAL_RESET);
#endif

	//wait to system reset
#if 0 //!defined(_NVT_EMULATION_)
	abort();
#else
	// coverity[no_escape]
	while (1) { 	// Loop forever
		_ASM_NOP
	}
#endif
}

//-----------------------------------------------------------------------------
// Power Reset (Power Alarm)
//-----------------------------------------------------------------------------
static void _HwClock_PowerReset_Enable(BOOL bEnable)
{
	if (bEnable != 0) {
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			rtc_enablePWRAlarm();
		} else {
			DBG_WRN("Not support\r\n");
		}
		DBG_DUMP("+(pwr-alarm)\r\n");
	} else {
		DBG_DUMP("-(pwr-alarm)\r\n");
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			rtc_disablePWRAlarm();
		} else {
			DBG_WRN("Not support\r\n");
		}
	}
}

static void _HwClock_PowerReset_ClearCounter(void)
{
	DBG_ERR("Not Support!");
}

static void __add_1s(struct tm *ct)
{
	if (ct->tm_sec == 59) {
		if (ct->tm_min == 59) {
			ct->tm_hour += 1;
			ct->tm_min = 0;
			ct->tm_sec = 0;
		} else {
			ct->tm_min += 1;
			ct->tm_sec = 0;
		}
	} else {
		ct->tm_sec += 1;
	}
}

static void _HwClock_PowerReset(void)
{
	struct tm CurrDT;
	_HwClock_PowerReset_Enable(FALSE);

	CurrDT = hwclock_get_time(TIME_ID_CURRENT);
	__add_1s(&CurrDT);
	__add_1s(&CurrDT);
	__add_1s(&CurrDT);
	hwclock_set_time(TIME_ID_HWRT, CurrDT, 0);

	_HwClock_PowerReset_Enable(TRUE);

	_HwClock_PowerOff();
}

//-----------------------------------------------------------------------------
// Power
//-----------------------------------------------------------------------------
static const struct tm _dummy_ctv = {0};

struct tm hwclock_get_time(UINT32 timeID)
{
	if (!hwclock_opened) {
		DBG_ERR("HwClock is not opened\r\n");
		return _dummy_ctv;
	}

	switch (timeID) {
	case TIME_ID_CURRENT:
		return _HwClock_GetCurrentTime();
	case TIME_ID_ALARM:
		return _HwClock_GetAlarmTime();
	case TIME_ID_HWRT:
		return _HwClock_GetPowerAlarmTime();
	case TIME_ID_SWRT:
		//return _HwClock_GetPowerAlarmTime();
		return _dummy_ctv;
	default:
		DBG_ERR("unknown time id\r\n");
		break;
	}
	return _dummy_ctv;
}

void hwclock_set_time(UINT32 timeID, struct tm ctv, UINT32 param)
{
	if (!hwclock_opened) {
		DBG_ERR("HwClock is not opened\r\n");
		return;
	}

	switch (timeID) {
	case TIME_ID_CURRENT:
		_HwClock_SetCurrentTime(ctv);
		break;
	case TIME_ID_ALARM:
		_HwClock_SetAlarmTime(ctv, (FP)param);
		break;
	case TIME_ID_HWRT:
		_HwClock_SetPowerAlarmTime(ctv);
		break;
	case TIME_ID_SWRT:
		_HwClock_SoftReset_SetCounter(param);
		break;
	default:
		DBG_ERR("unknown time id\r\n");
		break;
	}
}

UINT32 hwpower_get_poweron_state(UINT32 stateID)
{
	UINT32 ret = 0;

	if (!hwclock_opened) {
		DBG_ERR("HwClock is not opened\r\n");
		return 0;
	}

	switch (stateID) {
	case POWER_STATE_SRC:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			ret = rtc_getPWROnSource();
		} else {
			DBG_WRN("Not support state id (%d)\r\n", stateID);
		}
		break;
	case POWER_STATE_FIRST:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return rtc_isPowerLost();
		} else {
			DBG_WRN("Not support state id (%d)\r\n", stateID);
		}
		break;
	default:
		DBG_ERR("unknown state id (%d)\r\n", stateID);
		break;
	}
	return ret;
}

UINT32 hwpower_get_power_key(UINT32 pwrID)
{
	UINT32 ret = 0;

	if (!hwclock_opened) {
		DBG_ERR("HwClock is not opened\r\n");
		return 0;
	}

	switch (pwrID) {
	case POWER_ID_PSW1:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return rtc_getPWRStatus();
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_PSW2:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return rtc_getPWR2Status();
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_PSW3:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return rtc_getPWR3Status();
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_PSW4:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return rtc_getPWR4Status();
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_HWRT:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return rtc_isPWRAlarmEnabled();
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_SWRT:
		return 0; //keep 1 if soft reset
		break;
	default:
		DBG_ERR("unknown pwr id (%d)\r\n", pwrID);
		break;
	}
	return ret;
}

void hwpower_set_power_key(UINT32 pwrID, UINT32 value)
{
	if (!hwclock_opened) {
		DBG_ERR("HwClock is not opened\r\n");
		return;
	}

	switch (pwrID) {
	case POWER_ID_PSW1:
		if (value == 0xf0) { //keep power on - clear power off timer
			_HwClock_PowerOff_ClearCounter();
		}
		if (value == 0xff) { //power off NOW
			_HwClock_PowerOff();
		}
		break;
	case POWER_ID_HWRT:
		if (value == 0) { //disable h/w reset timer
			_HwClock_PowerReset_Enable(FALSE);
		}
		if (value == 1) { //enable h/w reset timer
			_HwClock_PowerReset_Enable(TRUE);
		}
		if (value == 0xf0) { //keep power on - clear h/w reset timer
			_HwClock_PowerReset_ClearCounter();
		}
		if (value == 0xff) { //h/w reset NOW
			_HwClock_PowerReset();
		}
		break;
	case POWER_ID_SWRT:
		if (value == 0) { //disable s/w reset timer
			_HwClock_SoftReset_Enable(FALSE);
		}
		if (value == 1) { //enable s/w reset timer
			_HwClock_SoftReset_Enable(TRUE);
		}
		if (value == 0xf0) { //keep power on - clear s/w reset timer
			_HwClock_SoftReset_ClearCounter();
		}
		if (value == 0xff) { //s/w reset NOW
			_HwClock_SoftReset();
		}
		break;
	}
}

#endif
