#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

INT32 PB_PlayBrowserMode(PPLAY_BROWSER_OBJ pPlayBrowserObj)
{
	UINT8  JumpOffset;
	FLGPTN uiFlag;
	UINT8  BrowserNums, BrowserCommand;

	if (!(pPlayBrowserObj->BrowserCommand & (PB_BROWSER_CURR | PB_BROWSER_NEXT | PB_BROWSER_PREV))) {
		return PBERR_PAR;
	}

	// Check total images number
	if ((pPlayBrowserObj->HorNums * pPlayBrowserObj->VerNums) > PLAY_MAX_THUMB_NUM) {
		return PBERR_PAR;
	}

	// Wait for plaback finish its job to prevent modify global variable that in using
	wai_flg(&uiFlag, FLG_PB, FLGPB_NOTBUSY, TWF_ORW);

	// Don't modify pPlayBrowerObj
	JumpOffset = pPlayBrowserObj->JumpOffset;
	BrowserNums = pPlayBrowserObj->HorNums * pPlayBrowserObj->VerNums;
	BrowserCommand = pPlayBrowserObj->BrowserCommand;
	gMenuPlayInfo.RotateDir = PLAY_ROTATE_DIR_UNKNOWN;
	// If first enter browser mode
	if (pPlayBrowserObj->BrowserCommand & PB_BROWSER_CURR) {
		// initial value
		if (gMenuPlayInfo.ZoomIndex != 0) {
			gMenuPlayInfo.ZoomIndex = 0;        // Default = 1X
		}
	} else if (pPlayBrowserObj->BrowserCommand & PB_BROWSER_PREV) {
		// Still in the same page
		if (gMenuPlayInfo.CurFileIndex > JumpOffset) {
			gMenuPlayInfo.CurFileIndex -= JumpOffset;
			return PB_STA_DONE;
		}
		// Previous image is not in current page, load previous page
		else {
			JumpOffset -= gMenuPlayInfo.CurFileIndex;
			JumpOffset += gMenuPlayInfo.DispThumbNums;
			if (pPlayBrowserObj->bReDecodeImages == FALSE) {
				gMenuPlayInfo.CurFileIndex = gMenuPlayInfo.DispThumbNums - (JumpOffset % gMenuPlayInfo.DispThumbNums);
				return PB_STA_DONE;
			}
		}
	} else { // if(PlayCommand == PB_BROWSER_NEXT)
		// Still in the same page
		if ((gMenuPlayInfo.CurFileIndex + JumpOffset) <= gMenuPlayInfo.DispThumbNums) {
			gMenuPlayInfo.CurFileIndex += JumpOffset;
			return PB_STA_DONE;
		}
		// Next image is not in current page, load next page
		else {
			JumpOffset -= (gMenuPlayInfo.DispThumbNums - gMenuPlayInfo.CurFileIndex);
			if (pPlayBrowserObj->bReDecodeImages == FALSE) {
				gMenuPlayInfo.CurFileIndex = JumpOffset;
				return PB_STA_DONE;
			}
		}
	}
	gPBCmdObjPlayBrowser.BrowserCommand = BrowserCommand;
	gPBCmdObjPlayBrowser.BrowserNums    = BrowserNums;
	gPBCmdObjPlayBrowser.JumpOffset     = JumpOffset;
	PB_PlaySetCmmd(PB_CMMD_BROWSER);

	return PBERR_OK;
}

