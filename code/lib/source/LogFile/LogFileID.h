#ifndef _LOGFILE_ID_H
#define _LOGFILE_ID_H
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"

extern SEM_HANDLE _SECTION(".kercfg_data") LOGFILE_SEM_ID;
extern SEM_HANDLE _SECTION(".kercfg_data") LOGFILE_WRITE_SEM_ID;
extern ID _SECTION(".kercfg_data") LOGFILE_FLG_ID;
extern THREAD_HANDLE _SECTION(".kercfg_data") LOGFILE_TSK_MAIN_ID;
extern THREAD_HANDLE _SECTION(".kercfg_data") LOGFILE_TSK_EMR_ID;
#endif //_LOGFILE_ID_H