#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/**
    Get current file-name-id.

    @return UINT32 CurFileNameId
*/
UINT32 xPB_GetCurFileNameId(void)
{
	return gMenuPlayInfo.DispFileName[gMenuPlayInfo.CurFileIndex - 1];
}
