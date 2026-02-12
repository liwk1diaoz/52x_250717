#ifndef _LVIEWNVT_ID_H
#define _LVIEWNVT_ID_H
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"

extern ID              FLG_ID_LVIEWNVT;
extern ID              LVIEWD_FLG_ID;
extern VK_TASK_HANDLE  LVIEWD_RCV_TSK_ID;
extern SEM_HANDLE      LVIEWD_SEM_ID;

extern void LviewNvt_InstallID(void);
extern void LviewNvt_UnInstallID(void);
#endif //_LVIEW_ID_H