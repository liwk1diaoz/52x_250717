#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

void PB_PlaySpecFile(PPLAY_SPECFILE_OBJ pPlayFileObj)
{
	gPBCmdObjPlaySpecFile.bDisplayWoReDec  = pPlayFileObj->bDisplayWoReDec;
	gPBCmdObjPlaySpecFile.PlayFileVideo    = pPlayFileObj->PlayFileVideo;
	gPBCmdObjPlaySpecFile.PlayFileClearBuf = pPlayFileObj->PlayFileClearBuf;
	gPBCmdObjPlaySpecFile.PlayRect.x = pPlayFileObj->PlayRect.x;
	gPBCmdObjPlaySpecFile.PlayRect.y = pPlayFileObj->PlayRect.y;

	if (pPlayFileObj->PlayRect.w == 0) {
		gPBCmdObjPlaySpecFile.PlayRect.w = g_PBDispInfo.uiDisplayW;
	} else {
		gPBCmdObjPlaySpecFile.PlayRect.w = pPlayFileObj->PlayRect.w;
	}

	if (pPlayFileObj->PlayRect.h == 0) {
		gPBCmdObjPlaySpecFile.PlayRect.h = g_PBDispInfo.uiDisplayH;
	} else {
		gPBCmdObjPlaySpecFile.PlayRect.h = pPlayFileObj->PlayRect.h;
	}

	PB_PlaySetCmmd(PB_CMMD_DE_SPECFILE);
}