#ifndef _HFSNVT_ID_H
#define _HFSNVT_ID_H
#include "kwrap/type.h"

extern SEM_HANDLE     HFSNVT_SEM_ID;
extern VK_TASK_HANDLE HFSNVT_TSK_RCV_ID;
extern ID             HFSNVT_FLG_ID;

extern void HfsNvt_InstallID(void);
extern void HfsNvt_UnInstallID(void);
#endif //_HFSNVT_ID_H