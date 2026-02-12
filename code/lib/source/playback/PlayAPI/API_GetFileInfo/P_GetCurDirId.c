#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/**
    Get current dir-id.

    @return UINT32 CurFileDirId
*/
UINT32 xPB_GetCurFileDirId(void)
{
	return gMenuPlayInfo.DispDirName[gMenuPlayInfo.CurFileIndex - 1];
}