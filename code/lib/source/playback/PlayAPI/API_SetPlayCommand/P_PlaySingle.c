#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

void PB_PlaySingleMode(PPLAY_SINGLE_OBJ pPlaySingleObj)
{
	FLGPTN uiFlag;

	// Not a valid command
	if (!(pPlaySingleObj->PlayCommand & (PB_SINGLE_CURR | PB_SINGLE_NEXT | PB_SINGLE_PREV | PB_SINGLE_PRIMARY | PB_SINGLE_THUMB))) {
		return;
	}

	// Wait for plaback finish its job to prevent modify global variable that in using
	wai_flg(&uiFlag, FLG_PB, FLGPB_NOTBUSY, TWF_ORW);

	// initial value
	if (gMenuPlayInfo.ZoomIndex != 0) {
		gMenuPlayInfo.ZoomIndex = 0;        // Default = 1X
	}
	if (gMenuPlayInfo.RotateDir != PLAY_ROTATE_DIR_UNKNOWN) {
		gMenuPlayInfo.RotateDir = PLAY_ROTATE_DIR_UNKNOWN;
	}
	gMenuPlayInfo.JumpOffset = 1;

	gPBCmdObjPlaySingle.PlayCommand = pPlaySingleObj->PlayCommand;
	gPBCmdObjPlaySingle.JumpOffset  = pPlaySingleObj->JumpOffset;

	PB_PlaySetCmmd(PB_CMMD_SINGLE);
}

