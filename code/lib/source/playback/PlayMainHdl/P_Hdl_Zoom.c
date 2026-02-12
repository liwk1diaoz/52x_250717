#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

//----- Local functions define --------------------------------------------
static void PB_ZoomLookupTblHandle(UINT32 PlayCommand);
static void PB_ZoomPanHandle(UINT32 PlayCommand);
static void PB_ZoomUserDefineHandle(void);
//-------------------------------------------------------------------------

/*
    Zoom mode handler
    This is internal API.

    @return void
*/
void PB_ZoomHandle(void)
{
	UINT32 PlayCommand;
	UINT64 fileSize;
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;

	PlayCommand = gucCurrPlayZoomCmmd;

	if (FALSE == gMenuPlayInfo.DecodePrimaryOK) {
		// must decode primary
		INT32 iErrCode;

		// avoid file being closed, can not be read-continue
		PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)guiPlayFileBuf, 0x200, 0, 1);
		// do not display primary again
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &fileSize, NULL);
		iErrCode = PB_DecodePrimaryHandler((UINT32)fileSize, gMenuPlayInfo.CurFileFormat);
		if (DECODE_JPG_PRIMARY == iErrCode) {
			gMenuPlayInfo.DecodePrimaryOK = TRUE;
		} else {
			DBG_ERR("Dec primary error!\r\n");

			PB_SetFinishFlag(DECODE_JPG_DECODEERROR);
			return;
		}
	}

	if (gMenuPlayInfo.RotateDir != PLAY_ROTATE_DIR_UNKNOWN) {
		if (gucRotatePrimary == FALSE) {
			PB_RotatePrimaryHandle(gMenuPlayInfo.RotateDir);
			gucRotatePrimary = TRUE;
		}
	}

	if (PlayCommand & (PLAYZOOM_IN | PLAYZOOM_OUT)) {
		PB_ZoomLookupTblHandle(PlayCommand);
	} else if (PlayCommand & (PLAYZOOM_UP | PLAYZOOM_DOWN | PLAYZOOM_LEFT | PLAYZOOM_RIGHT)) {
		PB_ZoomPanHandle(PlayCommand);
	} else if (PlayCommand & (PLAYZOOM_USER)) {
		PB_ZoomUserDefineHandle();
	}

	DBG_IND("Zoom OK !\r\n");
	PB_SetFinishFlag(DECODE_JPG_DONE);
}

#if 0
#pragma mark -
#endif

/*
    Zoom in/out with look-up table handler
    This is internal API.

    @param[in] PlayCommand:
    @return void
*/
static void PB_ZoomLookupTblHandle(UINT32 PlayCommand)
{
	if (!(PlayCommand & (PLAYZOOM_IN | PLAYZOOM_OUT))) {
		return;
	}

	if (PlayCommand & PLAYZOOM_IN) {
		gMenuPlayInfo.ZoomIndex++;
	} else if ((PlayCommand & PLAYZOOM_OUT) && (gMenuPlayInfo.ZoomIndex != 0)) {
		gMenuPlayInfo.ZoomIndex--;
	}
	PB_DigitalZoom(PlayCommand);
}

/*
    Zoom pan mode handler
    This is internal API.

    @param[in] PlayCommand:
    @return void
*/
static void PB_ZoomPanHandle(UINT32 PlayCommand)
{
	PB_DigitalZoomMoving();
}

/*
    Zoom in/out with user define handler
    This is internal API.

    @return void
*/
static void PB_ZoomUserDefineHandle(void)
{
	PB_DigitalZoomUserSetting(gusPlayZoomUserLeftUp_X,
							  gusPlayZoomUserLeftUp_Y,
							  gusPlayZoomUserRightDown_X,
							  gusPlayZoomUserRightDown_Y);
}
