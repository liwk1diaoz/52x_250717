#include <stdio.h>
#include <string.h>
#include "kwrap/util.h"
#include "GxTime.h"
#include "DxPower.h"
#include "PowerDef.h"
#include "DxApi.h"
#include "Dx.h"
#include "DxType.h"
#include <time.h>

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxTm
#define __DBGLVL__          THIS_DBGLVL
//#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#define __DBGFLT__          "[charge]" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"

extern DX_HANDLE power_obj;

//-----------------------------------------------------------------------------
// Date-Time util
//-----------------------------------------------------------------------------

static struct tm g_ctv_zero = {0};

void GxTime_GetTime(struct tm* p_ctv)
{
	if (!p_ctv) return;
	if (!power_obj) {
		*(p_ctv) = g_ctv_zero;
		return;
	}
	
	Dx_Control(power_obj, DRVPWR_CTRL_CURRENT_TIME, 1, (UINT32)p_ctv);//1=read
}

void GxTime_SetTime(struct tm ctv)
{
	if (!power_obj) {
		return;
	}
	Dx_Control(power_obj, DRVPWR_CTRL_CURRENT_TIME, 0, (UINT32)&ctv); //0=write
}


/**
    RTC Date
*/
typedef union {
	// Format conformed to RTC register
	_PACKED_BEGIN struct {
		UBITFIELD   day: 5;     ///< Day
		UBITFIELD   month: 4;   ///< Month
		UBITFIELD   year: 12;   ///< Year
	} _PACKED_END s;
	UINT32      value;          ///< Date value all together Y:M:D
} RTC_DATE, *PRTC_DATE;

static const UINT8      uiRTCDayOfMonth[2][12] = {
	// Not leap year
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	// Leap year
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static const UINT32     uiRTCDayOfYear[2] = {365, 366};

// Base date
static RTC_DATE         RTCBaseDate = {1, 1, 1900};

/**
    Check if leap year

    This function is used to check if the specified year is a leap year.

    @param[in] uiYear   The year you want to check (0~0xFFFFFFFF)
    @return
        - @b TRUE   : Is a leap year
        - @b FALSE  : Isn't a leap year
*/
BOOL GxTime_IsLeapYear(UINT32 uiYear)
{
	// New algorithm
	return (BOOL)((((uiYear % 4) == 0) && (((uiYear % 100) != 0) || ((uiYear % 400) == 0))));
}

/**
    Convert date to number of days to base date.

    Convert date to number of days to base date.

    @param[in] rtcDate      Absolute date
    @return Days from base date
*/
///////// -- Chris -- Buggy if base date is not yyyy/01/01
UINT32 GxTime_Time2Days(struct tm ctv)
{
	UINT32 uiDays;
	UINT32 i;
	RTC_DATE rtcDate;

	rtcDate.s.day = ctv.tm_mday;     /* day of the month - [1,31]         */
	rtcDate.s.month = ctv.tm_mon;    /* months since January - [0,11]     */
	rtcDate.s.year = ctv.tm_year;    /* years since 2000                  */
	uiDays = 0;

	// Year
	for (i = RTCBaseDate.s.year; i < rtcDate.s.year; i++) {
		uiDays += uiRTCDayOfYear[GxTime_IsLeapYear(i)];
	}

	// Month
	for (i = RTCBaseDate.s.month; i < rtcDate.s.month; i++) {
		uiDays += uiRTCDayOfMonth[GxTime_IsLeapYear(rtcDate.s.year)][i - 1];
	}

	uiDays += rtcDate.s.day - RTCBaseDate.s.day;

	return uiDays;
}


/**
    Convert days to date from base date.

    This function is used to convert days to date from base date.

    @param[in] uiDays   Number of days from base date
    @return Converted absolute date
*/
///////// -- Chris -- Buggy if base date is not yyyy/01/01
struct tm GxTime_Days2Time(UINT32 uiDays)
{
	struct tm ctv = {0};

	RTC_DATE Date;
	UINT32   uiYear, uiMonth;

	for (uiYear = RTCBaseDate.s.year; uiDays >= uiRTCDayOfYear[GxTime_IsLeapYear(uiYear)]; uiYear++) {
		uiDays -= uiRTCDayOfYear[GxTime_IsLeapYear(uiYear)];
	}

	for (uiMonth = RTCBaseDate.s.month; uiDays >= uiRTCDayOfMonth[GxTime_IsLeapYear(uiYear)][uiMonth - 1]; uiMonth++) {
		uiDays -= uiRTCDayOfMonth[GxTime_IsLeapYear(uiYear)][uiMonth - 1];
	}

	Date.value      = 0;
	Date.s.day      = RTCBaseDate.s.day + uiDays;
	Date.s.month    = uiMonth;
	Date.s.year     = uiYear;

	ctv.tm_mday = Date.s.day;     /* day of the month - [1,31]         */
	ctv.tm_mon = Date.s.month;    /* months since January - [0,11]     */
	ctv.tm_year = Date.s.year;    /* years since 2000                  */
	ctv.tm_hour = 0;
	ctv.tm_min = 0;
	ctv.tm_sec = 0;
	// NOT support the following function..
	ctv.tm_isdst = 0;
	ctv.tm_yday = 0;
	ctv.tm_wday = 0;

	return ctv;
}

INT32 GxTime_CalcMonthDays(INT32 year, INT32 month)
{
	INT32 day = 31;
	INT32 year_ext = 0;
	if (GxTime_IsLeapYear(year) == TRUE) {
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

struct tm GxTime_AddTime(struct tm ctv, struct tm diff)
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
	maxday = GxTime_CalcMonthDays(ctv.tm_year, ctv.tm_mon);
	while (ctv.tm_mday > maxday) {
		ctv.tm_mday -= maxday;
		ctv.tm_mon ++;
		//month range = [1,12]
		if (ctv.tm_mon > 12) {
			ctv.tm_year += (ctv.tm_mon - 1) / 12;
			ctv.tm_mon = ((ctv.tm_mon - 1) % 12) + 1;
		}
		maxday = GxTime_CalcMonthDays(ctv.tm_year, ctv.tm_mon);
	}
	//month range = [1,12]
	if (ctv.tm_mon > 12) {
		ctv.tm_year += (ctv.tm_mon - 1) / 12;
		ctv.tm_mon = ((ctv.tm_mon - 1) % 12) + 1;
	}
	return ctv;
}

//@}
