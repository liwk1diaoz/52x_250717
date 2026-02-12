#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

//#NT#2011/01/31#Ben Wang -begin
//#NT#Add screen control to sync display with OSD
static BOOL bLocked = FALSE;

void PB_ScreenControl(PB_SCREEN_CTRL_TYPE CtrlType)
{
	FLGPTN uiFlag;
	//#NT#2011/02/08#Ben Wang -begin
	//#NT#check if playback not start
	if (guiOnPlaybackMode != TRUE) {
		return;
	}
	//#NT#2011/02/08#Ben Wang -end
	// Wait for plaback finish its job to prevent modify global variable that in using
	wai_flg(&uiFlag, FLG_PB, FLGPB_NOTBUSY, TWF_ORW);
	gScreenControlType = CtrlType;
	DBG_IND("PB_ScreenControl CtrlType=%d, bLocked=%d\r\n", CtrlType, bLocked);
	if (PB_FLUSH_SCREEN == CtrlType && TRUE == bLocked) {
		#if 0
		PB_FrameBufSwitch(PLAYSYS_COPY2TMPBUF);
		PB_CopyImage2Display(PLAYSYS_COPY2TMPBUF, PLAYSYS_COPY2FBBUF);
		PB_FrameBufSwitch(PLAYSYS_COPY2FBBUF);
		#endif
		bLocked = FALSE;
	} else if (PB_LOCK_SCREEN == CtrlType) {
		bLocked = TRUE;
	}
}
//#NT#2011/01/31#Ben Wang -end