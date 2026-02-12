#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

/**
    Check if current image-file with memo file.

    @return WithMemoStatus (TRUE/FALSE)
*/
UINT32 xPB_GetFILEWithMemo(void)
{
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;

	if (xPB_GetCurFileFormat() & PBFMT_JPGMEMO) {
		INT32  iResult;
		UINT32 isWithMemo;

		iResult = pFlist->GetInfo(PBX_FLIST_GETINFO_ISWITHMEMO, &isWithMemo, NULL);
		if ((iResult == E_OK) && isWithMemo) {
			return TRUE;
		} else {
			return FALSE;
		}
	} else {
		return FALSE;
	}
}

