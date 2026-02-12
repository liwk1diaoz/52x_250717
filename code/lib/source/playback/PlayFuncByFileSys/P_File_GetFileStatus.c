//#include "FileSysTsk.h"
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Get file file system status.
    This is internal API.

    @return FileStatus
*/
#if 0
INT32 PB_GetFileSysStatus(void)
{

	PFILEDB_FILE_ATTR FileAttr;

	if (g_PBSetting.EnableFlags & PB_ENABLE_SEARCH_FILE_WITHOUT_DCF) {
		FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);

		if (FileAttr == NULL) {
			return E_SYS;
		}

		if (FileAttr->fileSize == 0) {
			return E_SYS;
		} else {
			return 0;
		}
	} else {
		//#NT#2012/03/27#Lincy Lin -begin
		//#NT#Porting PBXFile
		// get file-system parameter
#if 0
		PFILE_TSK_PARAM pParam;
		pParam = (PFILE_TSK_PARAM)FilesysGetFSTParam();
		if (pParam->pFile && pParam->pFile->fileSize == 0) {
			// 0 KB file Error
			return E_SYS;
		} else {
			return ((INT32)pParam->Status);
		}
#endif
		return PBX_File_GetStatus();
		//#NT#2012/03/27#Lincy Lin -end

	}
}
#endif
