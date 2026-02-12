
/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
#include "FileDBID.h"
#include "SysKer.h"
#include "FileDB.h"
#else
// INCLUDE_PROTECTED
#include "FileDBID.h"
#endif

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
UINT32 SEMID_FILEDB[FILEDB_NUM] = {0};
UINT32 SEMID_FILEDB_COMM;
UINT32 FLG_ID_FILEDB = 0;
#else
SEM_HANDLE SEMID_FILEDB[FILEDB_NUM] = {0};
SEM_HANDLE SEMID_FILEDB_COMM;
ID FLG_ID_FILEDB = 0;
#endif

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void FileDB_InstallID(void)
{
	OS_CONFIG_SEMPHORE(SEMID_FILEDB_COMM, 0, 1, 1);
	OS_CONFIG_SEMPHORE(SEMID_FILEDB[0], 0, 1, 1);
	OS_CONFIG_SEMPHORE(SEMID_FILEDB[1], 0, 1, 1);
	OS_CONFIG_SEMPHORE(SEMID_FILEDB[2], 0, 1, 1);
	OS_CONFIG_FLAG(FLG_ID_FILEDB);
}

void FileDB_UnInstallID(void)
{
	SEM_DESTROY(SEMID_FILEDB_COMM);
	SEM_DESTROY(SEMID_FILEDB[0]);
	SEM_DESTROY(SEMID_FILEDB[1]);
	SEM_DESTROY(SEMID_FILEDB[2]);
	rel_flg(FLG_ID_FILEDB);
}
