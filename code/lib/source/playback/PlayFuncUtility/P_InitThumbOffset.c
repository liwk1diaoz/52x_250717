#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Initial all frame-buffer-addr in thumbnail mode
    This is internal API.

    @return void
*/
void PB_InitThumbOffset(void)
{
	UINT32 uiTotalNum = gPBCmdObjPlayBrowser.BrowserNums;
	PURECT pDrawThumb = gMenuPlayInfo.pRectDrawThumb;
	PURECT pDrawThumb2 = gMenuPlayInfo.pRectDrawThumb2;

	memcpy(g_PBIdxView[PBDISP_IDX_PRI], pDrawThumb, sizeof(URECT)*uiTotalNum);
	if (pDrawThumb2) {
		memcpy(g_PBIdxView[PBDISP_IDX_SEC], pDrawThumb2, sizeof(URECT)*uiTotalNum);
	}
}

