#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/**
    Get current file-format.

    @return UINT32 CurFileFormat
*/
UINT32 xPB_GetCurFileFormat(void)
{
	return gMenuPlayInfo.AllThumbFormat[gMenuPlayInfo.CurFileIndex - 1];
}
