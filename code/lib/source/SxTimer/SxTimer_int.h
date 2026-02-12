#ifndef _SXTIMER_INT_H
#define _SXTIMER_INT_H

#include <kwrap/nvt_type.h>
#include <kwrap/task.h>

// Service Lib
#define SXTIMER_PRIORITY         6
#define SXTIMER_STKSIZE          4096

extern THREAD_HANDLE SXTIMERTSK_ID; // Event Service: high priority
extern THREAD_DECLARE(SxTimerTsk, arglist);

void SxTimer_InstallID(void);

#endif //_SXTIMER_INT_H
