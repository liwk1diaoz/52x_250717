#include <string.h>
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Move data to display frame buffer.
    This is internal API.

    @param[in] FileFormat
    @return void
*/
void PB_MoveImageTo(UINT32 FileFormat)
{
	if (PB_FLUSH_SCREEN == gScreenControlType) {
#if 1
		PB_DispSrv_SetDrawAct(PB_DISPSRV_FLIP);
		PB_DispSrv_Trigger();
#else
		PB_FrameBufSwitch(PLAYSYS_COPY2TMPBUF);
		PB_CopyImage2Display(PLAYSYS_COPY2TMPBUF, PLAYSYS_COPY2FBBUF);
		PB_FrameBufSwitch(PLAYSYS_COPY2FBBUF);
#endif
	} else {
		gScreenControlType = PB_FLUSH_SCREEN;
	}
}

void xPB_DirectClearFrameBuffer(UINT32 uiBGColor)
{
	//gximg_fill_data(g_pPlayTmpImgBuf, GXIMG_REGION_MATCH_IMG, uiBGColor);

	gPbPushSrcIdx = PB_DISP_SRC_TMP;
	PB_DispSrv_SetDrawCb(PB_VIEW_STATE_BYPASS);
	PB_DispSrv_Trigger();
}

