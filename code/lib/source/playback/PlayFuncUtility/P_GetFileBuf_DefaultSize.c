#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Get default-file-buffer addr.
    This is internal API.

    @return void:
*/
void PB_GetDefaultFileBufAddr(void)
{
    UINT32 uifileSize;
	
    PB_GetParam(PBPRMID_MAX_FILE_SIZE,  &uifileSize);
	guiPlayFileBuf = (g_uiPBBufEnd - uifileSize);
}

