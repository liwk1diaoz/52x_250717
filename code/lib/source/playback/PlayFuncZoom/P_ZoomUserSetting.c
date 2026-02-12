#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Digital zoom by user setting.
    This is internal API.

    @param[in] LeftUp_X the start-Left-Up-pixel-X of this picture
    @param[in] LeftUp_Y the start-Left-Up-pixel-Y of this picture
    @param[in] RightDown_X the end-Right-Down-pixel-X of this picture
    @param[in] RightDown_Y the end-Right-Down-pixel-Y of this picture
    @return void
*/
void PB_DigitalZoomUserSetting(UINT32 LeftUp_X, UINT32 LeftUp_Y, UINT32 RightDown_X, UINT32 RightDown_Y)
{
#if _TODO
	UINT32 SrcWidth, SrcHeight, SrcStart_X, SrcStart_Y;

	////////////////////////////////////////////////////////////////////////
	// Alignment for IME Scaling function
	////////////////////////////////////////////////////////////////////////
	SrcWidth = ALIGN_CEIL_16(RightDown_X - LeftUp_X);
	SrcHeight = ALIGN_CEIL_8(RightDown_Y - LeftUp_Y);
	SrcStart_X = ALIGN_CEIL_8(LeftUp_X);
	SrcStart_Y = LeftUp_Y;
	PB_Scale2Display(SrcWidth, SrcHeight, SrcStart_X, SrcStart_Y,
					 gMenuPlayInfo.pJPGInfo->fileformat, PLAYSYS_COPY2TMPBUF);
	PB_DispSrv_Trigger();
	#if 0
	PB_FrameBufSwitch(PLAYSYS_COPY2TMPBUF);
	PB_CopyImage2Display(PLAYSYS_COPY2TMPBUF, PLAYSYS_COPY2FBBUF);
	PB_FrameBufSwitch(PLAYSYS_COPY2FBBUF);
	#endif
	gucZoomUpdateNewFB = TRUE;  // for rotate
#endif	
}
