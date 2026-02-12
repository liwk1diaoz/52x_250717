#include "kwrap/error_no.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"
#include "kwrap/semaphore.h"
#include "comm/hwclock.h"
#include "comm/hwpower.h"
#include "timer.h"
#include <time.h>
#include <sys/time.h>
#include "hd_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          HwClk
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////


#define CFG_SUPPORT_DRTC 0
#define CFG_SUPPORT_WDT 0

static HWCLOCK_PERMISSION hwclock_permission = HWCLOCK_PERMISSION_READWRITE;
static HWCLOCK_MODE hwclock_mode = HWCLOCK_MODE_RTC; //0 for RTC, 1 for DRTC
static BOOL hwclock_opened = FALSE;

///////////////////////////////////////////////////////////////////////////////

#define HWCLOCK_INIT_TAG       MAKEFOURCC('H', 'W', 'C', 'K') ///< a key value
#define LONG_COUNTER_PERIOD    1000000000                     ///< 1000 sec
#define BASE_YEAR              1900

typedef struct {
	UINT32         long_counter_init;
	TIMER_ID       long_counter_timerid;
	UINT64         long_counter_sample_cnt;
	UINT32         long_counter_offset;
	UINT32         last_sys_timer_count;
} HWCLOCK_CTRL;

typedef struct {
	int            psw1;
	int            psw2;
	int            psw3;
	int            psw4;
	char           power_src[5];
	char           power_lost[5];
} HWCLOCK_POWER_STATE;

//static HWCLOCK_CTRL  hwclock_ctrl;
static BOOL _long_counter_ready = FALSE;

static const UINT8 hwclock_day_of_month[2][12] = {
	// Not leap year
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	// Leap year
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static const UINT32 hwclock_day_of_year[2] = {365, 366};

// Base date
static const struct tm hwclock_base_date =
{
	.tm_mday = 1,
	.tm_mon = 1,
	.tm_year = 1900,
};

static struct tm hwclock_power_alarm_tm = {0};

static ID HWCLOCK_COMMON_SEM_ID;

/*static void hwclock_timer_cb_p(UINT32 event)
{
	//unsigned long      flags;
	//HWCLOCK_CTRL      *pctrl = &hwclock_ctrl;

	//loc_cpu(flags);
	//pctrl->last_sys_timer_count = timer_get_current_count(TIMER_SYSTIMER_ID);
	//pctrl->long_counter_sample_cnt++;
	//unl_cpu(flags);
}*/


void hwclock_init(void)
{
	//ER             ret;
	//HWCLOCK_CTRL   *pctrl = &hwclock_ctrl;

	//if (HWCLOCK_INIT_TAG == pctrl->long_counter_init)
	//	return;

	// Due to timer's HW restriction, the max interval value is 0xFFFFFFFF us
	// Init long counter
	/*if ((ret = timer_open(&pctrl->long_counter_timerid, hwclock_timer_cb_p)) == E_OK) {
		timer_cfg(pctrl->long_counter_timerid, LONG_COUNTER_PERIOD, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		pctrl->long_counter_offset = pctrl->last_sys_timer_count = timer_get_current_count(TIMER_SYSTIMER_ID);
		pctrl->long_counter_sample_cnt = pctrl->long_counter_offset/LONG_COUNTER_PERIOD;
		pctrl->long_counter_offset = pctrl->long_counter_offset % LONG_COUNTER_PERIOD;
		pctrl->long_counter_init = HWCLOCK_INIT_TAG;
		_long_counter_ready = TRUE;
	} else {
		DBG_ERR("init fail!(ret=%d)\r\n", ret);
	}*/
	_long_counter_ready = TRUE;
}

UINT32 hwclock_get_counter(void)
{
	return hd_gettime_ms();
}

UINT64 hwclock_get_longcounter(void)
{
	return hd_gettime_us();
}

UINT64 hwclock_diff_longcounter(UINT64 time_start, UINT64 time_end)
{
	UINT32 time_start_sec = 0;
	UINT32 time_start_usec = 0;
	UINT32 time_end_sec=0;
	UINT32 time_end_usec =0;
	INT32  diff_time_sec =0 ;
	INT32  diff_time_usec = 0;
	UINT64 diff_time;

	time_start_sec = (time_start >> 32) & 0xFFFFFFFF;
	time_start_usec = time_start & 0xFFFFFFFF;
	time_end_sec = (time_end >> 32) & 0xFFFFFFFF;
	time_end_usec = time_end & 0xFFFFFFFF;
	diff_time_sec = (INT32)time_end_sec - (INT32)time_start_sec;
	diff_time_usec = (INT32)time_end_usec - (INT32)time_start_usec;
	diff_time = (INT64)diff_time_sec * 1000000 + diff_time_usec;
	return diff_time;
}

BOOL hwclock_is_longcounter_ready(void)
{
	return _long_counter_ready;
}


/**
     Un-initialize system timer.

*/
void hwclock_exit(void)
{
	/*HWCLOCK_CTRL       *pctrl = &hwclock_ctrl;

	if (HWCLOCK_INIT_TAG != pctrl->long_counter_init)
		return;
	timer_close(pctrl->long_counter_timerid);
	pctrl->long_counter_timerid = TIMER_INVALID;
	pctrl->long_counter_init = 0;*/
}

///////////////////////////////////////////////////////////////////////////////


ER hwclock_open(HWCLOCK_MODE mode)
{
	ER ret = E_OK;
	int r;

	if (hwclock_opened) {
		DBG_ERR("HwClock is already opened\r\n");
		return ret;
	}

	hwclock_mode = mode;

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		//ret = rtc_open();
	} else {
#if (CFG_SUPPORT_DRTC)
		struct tm current_time = {0};

		//ret = drtc_open();
		current_time.tm_year = DEFAULT_YEAR;
		current_time.tm_mon = DEFAULT_MON;
		current_time.tm_mday = DEFAULT_DAY;
		current_time.tm_hour = DEFAULT_HOUR;
		current_time.tm_min = DEFAULT_MIN;
		current_time.tm_sec = DEFAULT_SEC;
		hwclock_set_time(TIME_ID_CURRENT, current_time, 0);
#endif
	}

	r = vos_sem_create(&HWCLOCK_COMMON_SEM_ID, 1, "SEMID_HWCLOCK_COMM");
	if(r == -1) {
		DBG_ERR("hwclock vos_sem_create error!\r\n");
		return E_SYS;
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

	vos_sem_destroy(HWCLOCK_COMMON_SEM_ID);

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
		/*case HWCLOCK_PARAM_ID_READ_TIME_OFFSET: {
				if (hwclock_mode == HWCLOCK_MODE_RTC) {
					//rtc_setReadTimeOffset(p1, p2);
				} else {
#if (CFG_SUPPORT_DRTC)
					//drtc_set_read_timeoffset(p1, p2);
#endif
				}
				break;
			}*/
		case HWCLOCK_PARAM_ID_READ_TIME_OFFSET:
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

		time_t timep;

		time(&timep);
		localtime_r(&timep, &ctv);

		//wrap for Linux
		ctv.tm_year = ctv.tm_year + BASE_YEAR;
		ctv.tm_mon  = ctv.tm_mon + 1;
	} else {
#if (CFG_SUPPORT_DRTC)
		DRTC_TIME stTime;
		DRTC_DATE stDate;

		//stTime = drtc_get_time();
		//stDate = drtc_get_date();

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
		struct timeval tv = {0};
		time_t timep;
		FILE * fp;

		//wrap for Linux
		if (ctv.tm_year >= BASE_YEAR) {
			ctv.tm_year = ctv.tm_year - BASE_YEAR;
		}

		//wrap for Linux
		if (ctv.tm_year > 0) {
			ctv.tm_mon  = ctv.tm_mon - 1;
		}

		timep = mktime(&ctv);

		tv.tv_sec = timep;

		settimeofday(&tv, 0);

		fp = popen("hwclock -w -u","r");
		if (fp == NULL) {
			DBG_ERR("set hwclock failed\r\n");
			return;
		}
		pclose(fp);

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
		//RTC_TIME stTime;
		//RTC_DATE stDate;

		//rtc_getAlarm(&stDate, &stTime);

		//ctv.tm_sec = stTime.s.second;   /* seconds after the minute - [0,59] */
		//ctv.tm_min = stTime.s.minute;   /* minutes after the hour - [0,59]   */
		//ctv.tm_hour = stTime.s.hour;    /* hours since midnight - [0,23]     */
		//ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		//ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		//ctv.tm_year = stDate.s.year;    /* years since 2000                  */

		DBG_ERR("Not Supported\r\n");
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
/*static void _HwClock_AlarmTimeCB(UINT32 uiEvent)
{
	if (_HwClock_AlarmTime_EventCB != 0) {
		_HwClock_AlarmTime_EventCB();
	}
}*/

static void _HwClock_SetAlarmTime(struct tm ctv, FP pEventCB)
{
	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		//RTC_TIME stTime;
		//RTC_DATE stDate;

		//stDate = rtc_getMaxDate();

		//if (stDate.s.year < ctv.tm_year) {
		//	DBG_ERR("Date is overflow\r\n");
		//	return;
		//}

		//stTime.s.second = ctv.tm_sec;   /* seconds after the minute - [0,59] */
		//stTime.s.minute = ctv.tm_min;  /* minutes after the hour - [0,59]   */
		//stTime.s.hour = ctv.tm_hour;    /* hours since midnight - [0,23]     */
		//stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		//stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		//stDate.s.year = ctv.tm_year;    /* years since 2000                  */

		_HwClock_AlarmTime_EventCB = pEventCB;
		/*if (rtc_setAlarm(stDate, stTime, _HwClock_AlarmTimeCB) != E_OK) {
			DBG_ERR("Set Date Fail:Y=%d,M=%d,D=%d\r\n", stDate.s.year, stDate.s.month, stDate.s.day);
			DBG_ERR("Set Time Fail:H=%d,M=%d,S=%d\r\n", stTime.s.hour, stTime.s.minute, stTime.s.second);
			return;
		}*/
		//rtc_triggerCSET();
		DBG_ERR("Not Supported\r\n");
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
		vos_sem_wait(HWCLOCK_COMMON_SEM_ID);
		ctv = hwclock_power_alarm_tm;
		vos_sem_sig(HWCLOCK_COMMON_SEM_ID);
	} else {
		DBG_WRN("Not support\r\n");
	}

	return ctv;
}

static void _HwClock_SetPowerAlarmTime(struct tm ctv)
{
	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		vos_sem_wait(HWCLOCK_COMMON_SEM_ID);
		hwclock_power_alarm_tm = ctv;
		vos_sem_sig(HWCLOCK_COMMON_SEM_ID);
	} else {
		DBG_WRN("Not support\r\n");
	}
}

static void _hwclock_enable_rtc_power_alarm(BOOL enable)
{
	FILE * fp;

	if (enable) {
		time_t timep;
		CHAR cmd[60];
		struct tm ctv;

		vos_sem_wait(HWCLOCK_COMMON_SEM_ID);
		ctv = hwclock_power_alarm_tm;
		vos_sem_sig(HWCLOCK_COMMON_SEM_ID);

		//wrap for Linux
		if (ctv.tm_year >= BASE_YEAR) {
			ctv.tm_year = ctv.tm_year - BASE_YEAR;
		}

		//wrap for Linux
		if (ctv.tm_year > 0) {
			ctv.tm_mon  = ctv.tm_mon - 1;
		}

		//reset wake alarm
		fp = popen("echo 0 > /sys/class/rtc/rtc0/wakealarm","r");
		if (fp == NULL) {
			DBG_ERR("reset wakealarm failed\r\n");
			return;
		}
		pclose(fp);

		timep = mktime(&ctv);

		sprintf(cmd, "echo %ld > /sys/class/rtc/rtc0/wakealarm", timep);

		fp = popen(cmd,"r");
		if (fp == NULL) {
			DBG_ERR("set wakealarm failed\r\n");
			return;
		}
		pclose(fp);
	} else {
		fp = popen("echo 0 > /sys/class/rtc/rtc0/wakealarm","r");
		if (fp == NULL) {
			DBG_ERR("reset wakealarm failed\r\n");
			return;
		}
		pclose(fp);
	}
}
//-----------------------------------------------------------------------------
// Date-Time util
//-----------------------------------------------------------------------------

static UINT32 timeutil_convertDate2Days(struct tm ctv)
{
	UINT32 uiDays;
	int i;

	uiDays = 0;

	// Year
	for (i = hwclock_base_date.tm_year; i < ctv.tm_year; i++) {
		uiDays += hwclock_day_of_year[timeutil_is_leap_year(i)];
	}

	// Month
	for (i = hwclock_base_date.tm_mon; i < ctv.tm_mon; i++) {
		uiDays += hwclock_day_of_month[timeutil_is_leap_year(ctv.tm_year)][i - 1];
	}

	uiDays += ctv.tm_mday - hwclock_base_date.tm_mday;

	return uiDays;
}

static struct tm timeutil_convertDays2Date(UINT32 uiDays)
{
	struct tm Date;
	UINT32   uiYear, uiMonth;

	for (uiYear = hwclock_base_date.tm_year; uiDays >= hwclock_day_of_year[timeutil_is_leap_year(uiYear)]; uiYear++) {
		uiDays -= hwclock_day_of_year[timeutil_is_leap_year(uiYear)];
	}

	for (uiMonth = hwclock_base_date.tm_mon; uiDays >= hwclock_day_of_month[timeutil_is_leap_year(uiYear)][uiMonth - 1]; uiMonth++) {
		uiDays -= hwclock_day_of_month[timeutil_is_leap_year(uiYear)][uiMonth - 1];
	}

	Date.tm_mday   = hwclock_base_date.tm_mday + uiDays;
	Date.tm_mon    = uiMonth;
	Date.tm_year   = uiYear;

	return Date;
}

BOOL timeutil_is_leap_year(UINT32 uiYear)
{
	BOOL ret = FALSE;

	if (hwclock_mode == HWCLOCK_MODE_RTC) {
		ret = (BOOL)((((uiYear % 4) == 0) && (((uiYear % 100) != 0) || ((uiYear % 400) == 0))));
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
		//RTC_DATE stDate;

		//stDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
		//stDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
		//stDate.s.year = ctv.tm_year;    /* years since 2000                  */
		ret = timeutil_convertDate2Days(ctv);
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
		//RTC_DATE stDate;

		ctv = timeutil_convertDays2Date(days);

		//ctv.tm_mday = stDate.s.day;     /* day of the month - [1,31]         */
		//ctv.tm_mon = stDate.s.month;    /* months since January - [0,11]     */
		//ctv.tm_year = stDate.s.year;    /* years since 2000                  */
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
		//rtc_resetShutdownTimer();
		system("echo 1 > /proc/pwbc"); //Please refer the JIRA IVOT_N12061_CO-6
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
		FILE * fp;

		vos_util_delay_ms(800);
		fp = popen("poweroff","r");//this func will turn-off system power
		if (fp == NULL) {
			DBG_ERR("poweroff failed\r\n");
			return;
		}
		pclose(fp);

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
			_hwclock_enable_rtc_power_alarm(TRUE);
		} else {
			DBG_WRN("Not support\r\n");
		}
		DBG_DUMP("+(pwr-alarm)\r\n");
	} else {
		DBG_DUMP("-(pwr-alarm)\r\n");
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			_hwclock_enable_rtc_power_alarm(FALSE);
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
	HWCLOCK_POWER_STATE power_state = {0};

	if (!hwclock_opened) {
		DBG_ERR("HwClock is not opened\r\n");
		return 0;
	}

	{
		//parse cat string
		FILE * fp;
		char buffer[40];
		fp = popen("cat /proc/pwbc","r");

		if (fp == NULL) {
			DBG_ERR("proc failed\r\n");
			return 0;
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power switch 1 value %d\r\n", &power_state.psw1);
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power switch 2 value %d\r\n", &power_state.psw2);
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power switch 3 value %d\r\n", &power_state.psw3);
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power switch 4 value %d\r\n", &power_state.psw4);
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power on source %s\r\n", (char *)&power_state.power_src);
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "Power lost: %s\r\n", (char *)&power_state.power_lost);
		}

		pclose(fp);
	}

	switch (stateID) {
	case POWER_STATE_SRC:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			if (strcmp(power_state.power_src, "SW1") == 0) {
				return POWER_ID_PSW1;
			} else if (strcmp(power_state.power_src, "SW2") == 0) {
				return POWER_ID_PSW2;
			} else if (strcmp(power_state.power_src, "SW3") == 0) {
				return POWER_ID_PSW3;
			} else if (strcmp(power_state.power_src, "SW4") == 0) {
				return POWER_ID_PSW4;
			} else if (strcmp(power_state.power_src, "None") == 0) {
				DBG_ERR("Unknown power on source\r\n");
				return POWER_ID_HWRT;
			}
		} else {
			DBG_WRN("Not support state id (%d)\r\n", stateID);
		}
		break;
	case POWER_STATE_FIRST:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			if (strcmp(power_state.power_lost, "Yes") == 0) {
				return 1;
			} else {
				return 0;
			}
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
	HWCLOCK_POWER_STATE power_state = {0};

	if (!hwclock_opened) {
		DBG_ERR("HwClock is not opened\r\n");
		return 0;
	}

	{
		//parse cat string
		FILE * fp;
		char buffer[40];
		fp = popen("cat /proc/pwbc","r");

		if (fp == NULL) {
			DBG_ERR("proc failed\r\n");
			return 0;
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power switch 1 value %d\r\n", &power_state.psw1);
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power switch 2 value %d\r\n", &power_state.psw2);
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power switch 3 value %d\r\n", &power_state.psw3);
		}

		if ((fgets(buffer, sizeof(buffer), fp)) != NULL) {
			sscanf(buffer, "power switch 4 value %d\r\n", &power_state.psw4);
		}

		pclose(fp);
	}

	switch (pwrID) {
	case POWER_ID_PSW1:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return power_state.psw1;
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_PSW2:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return power_state.psw2;
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_PSW3:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return power_state.psw3;
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_PSW4:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			return power_state.psw4;
		} else {
			DBG_WRN("Not support pwr id (%d)\r\n", pwrID);
		}
		break;
	case POWER_ID_HWRT:
		if (hwclock_mode == HWCLOCK_MODE_RTC) {
			//return rtc_isPWRAlarmEnabled();
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
