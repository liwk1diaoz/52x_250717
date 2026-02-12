#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

/*
    Get full-file-buffer addr.
    This is internal API.

    @return void:
*/
void PB_GetNewFileBufAddr(void)
{
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;
	UINT64         tmpFileSize;

	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &tmpFileSize, NULL);
	guiPlayFileBuf = ALIGN_FLOOR_16(g_uiPBBufEnd - (UINT32)tmpFileSize);
}

