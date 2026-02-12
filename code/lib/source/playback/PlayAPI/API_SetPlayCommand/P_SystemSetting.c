#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

void PB_Sleep(void)
{
	//TBD..
}

void PB_Wakeup(void)
{
	//TBD..
}

void PB_PauseDMA(void)
{
	//TBD..
}

void PB_ResumeDMA(void)
{
	//TBD..
}

void PB_PauseDisp(BOOL bWaitFinish)
{
	guiIsSuspendPB = TRUE;
	PB_DispSrv_Suspend();
}

void PB_ResumeDisp(BOOL bWaitFinish)
{
	guiIsSuspendPB = FALSE;

	PB_DispSrv_Resume();

	// change display size
	//PB_ChangeDisplaySize();



	// change layout //

	// set playback command again to display new size
	if (guiCurrPlayCmmd == PB_CMMD_SINGLE) {
		gPBCmdObjPlaySingle.PlayCommand = (PB_SINGLE_CURR | PB_SINGLE_PRIMARY);
		gPBCmdObjPlaySingle.JumpOffset  = 1;
		PB_PlaySetCmmd(PB_CMMD_SINGLE);
	} else if (guiCurrPlayCmmd == PB_CMMD_BROWSER) {
		gPBCmdObjPlayBrowser.BrowserCommand = PB_BROWSER_CURR;
		PB_PlaySetCmmd(PB_CMMD_BROWSER);
	}
}

