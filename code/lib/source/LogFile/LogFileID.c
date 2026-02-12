#include "LogFileID.h"
#include "LogFileInt.h"
#include "LogFile.h"

#define PRI_LOGFILE_MAIN               25
#define PRI_LOGFILE_EMR                3

#define STKSIZE_LOGFILE_MAIN           4096 //2048
#define STKSIZE_LOGFILE_EMR            4096 //2048

SEM_HANDLE LOGFILE_SEM_ID = 0;
SEM_HANDLE LOGFILE_WRITE_SEM_ID = 0;
ID LOGFILE_FLG_ID = 0;
THREAD_HANDLE LOGFILE_TSK_MAIN_ID = 0;
THREAD_HANDLE LOGFILE_TSK_EMR_ID = 0;
void LogFile_InstallID(void)
{
	OS_CONFIG_SEMPHORE(LOGFILE_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(LOGFILE_WRITE_SEM_ID, 0, 1, 1);
	OS_CONFIG_FLAG(LOGFILE_FLG_ID);
	LOGFILE_TSK_MAIN_ID = vos_task_create(LogFile_MainTsk, 0, "LogFile_MainTsk", PRI_LOGFILE_MAIN, STKSIZE_LOGFILE_MAIN);
	LOGFILE_TSK_EMR_ID = vos_task_create(LogFile_EmrTsk, 0, "LogFile_EmrTsk", PRI_LOGFILE_EMR, STKSIZE_LOGFILE_EMR);
}

void LogFile_UnInstallID(void)
{
	vos_sem_destroy(LOGFILE_SEM_ID);
	vos_sem_destroy(LOGFILE_WRITE_SEM_ID);
	vos_flag_destroy(LOGFILE_FLG_ID);
}
