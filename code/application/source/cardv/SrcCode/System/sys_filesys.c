#include <string.h>
#include <sdio.h>
#include <hdal.h>
#include <FileSysTsk.h>
#include "sys_filesys.h"
#include "sys_mempool.h"
#include "FileDB.h"
#if (USE_DCF == ENABLE)
#include "DCF.h"
#endif

void filesys_init(void)
{
	FileSys_InstallID(FileSys_GetOPS_Linux());
#if (USE_FILEDB == ENABLE)
	FileDB_InstallID();
#endif
#if (USE_DCF == ENABLE)
	DCF_InstallID();
#endif
}