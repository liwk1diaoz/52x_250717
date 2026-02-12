/**
    System time module.

    This module handles the time.

    @file       GxTime.h
    @ingroup    mILibTimeCtrl

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _GXTIME_H
#define _GXTIME_H

#include "GxCommon.h"
#include "DxType.h"
#include "Dx.h"
#include <time.h>

/**
    @addtogroup mILibPowerCtrl
*/
//@{

////////////////////////////////////////////////////////////////////////////////

extern void   GxTime_GetTime(struct tm* p_ctv);
extern void   GxTime_SetTime(struct tm ctv);


/**
     Calculate maximum days number of given (year, month).

     Calculate maximum days number of given (year, month).

     @param[in] year      Year.
     @param[in] month     Month

     @return Days number.
*/
extern INT32 GxTime_CalcMonthDays(INT32 year, INT32 month);

/**
     Convert date-time value to days number.

     Convert date-time value to days number.

     @param[in] ctv     Date time value.

     @return Days number. [second precison]
*/
extern UINT32 GxTime_Time2Days(struct tm ctv);

/**
     Convert days number to date-time value.

     Convert days number to date-time value.

     @param[in] days    Days number.

     @return Date time value. [day precison]
*/
extern struct tm GxTime_Days2Time(UINT32 days);

/**
     Calculate sum of date-time value and another differece

     Calculate sum of date-time value and another differece

     @param[in] ctv     Date time value.
     @param[in] diff    Difference (date time value).

     @return Sum (date time value). [second precison]
*/
extern struct tm GxTime_AddTime(struct tm ctv, struct tm diff);

/**
    Check if leap year

    This function is used to check if the specified year is a leap year.

    @param[in] uiYear   The year you want to check (0~0xFFFFFFFF)
    @return
        - @b TRUE   : Is a leap year
        - @b FALSE  : Isn't a leap year
*/
extern BOOL GxTime_IsLeapYear(UINT32 year);

//@}
#endif //_GXTIME_H
