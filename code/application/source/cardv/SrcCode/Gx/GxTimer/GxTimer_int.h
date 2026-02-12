#ifndef _GXTIMER_INT_H
#define _GXTIMER_INT_H

#include <stdio.h>
#include <string.h>
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"

extern GX_CALLBACK_PTR g_fpTimerCB;
extern ID SEMID_GXTIMER;
extern void xGxTimer_InstallCmd(void);
extern void GxTimer_InstallID(void);

#endif //_GXTIMER_INT_H
