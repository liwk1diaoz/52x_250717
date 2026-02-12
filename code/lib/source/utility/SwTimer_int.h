/*
    Software timer internal header file

    Software timer internal header file.

    @file       SwTimer_int.h
    @ingroup    mIUtilSwTimer
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _SWTIMER_INT_H
#define _SWTIMER_INT_H

#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
//#include "kwrap/spinlock.h"
#include "kwrap/util.h"
#include "Utility/SwTimer.h"

// Default debug level
#ifndef __DBGLVL__
#define __DBGLVL__  1       // Output all message by default. __DBGLVL__ will be set to 1 via make parameter when release code.
#endif

// Default debug filter
#ifndef __DBGFLT__
#define __DBGFLT__  "*"     // Display everything when debug level is 2
#endif

#include <kwrap/debug.h>

/**
    @addtogroup mIUtilSwTimer
*/
//@{

// Task priority and stack size
#define SWTIMER_PRIORITY            2
#define SWTIMER_STKSIZE             2048
// Group counter
#define SWTIMER_GROUP_CNT           (ALIGN_CEIL(SWTIMER_NUM, 32) >> 5)

// Software timer ID to group number macro
#define SWTIMER_ID_2_GROUP(id)      ((id) >> 5)
// Software timer ID to bit macro
#define SWTIMER_ID_2_BIT(id)        (1 << ((id) & 0x1F))

typedef struct _SWTIMER_INFO_T {
	SWTIMER_CB      EventHandler;       // Callback function
	INT32           iInterval;          // Interval
	INT32           iCount;             // Current count
} SWTIMER_INFO;

//@}

#endif
