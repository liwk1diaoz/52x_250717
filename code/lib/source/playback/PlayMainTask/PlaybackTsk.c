/*

    All functions for Playback mode

    @file       PlaybackTsk.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/**
    Playback main task.
    This is internal API.

    @return void
*/
void PlaybackTsk(void)
{
	FLGPTN PlayCMD;

	//kent_tsk();

	DBG_IND("Start...\r\n");
	gusCurrPlayStatus = PB_STA_UNKNOWN;
	PB_Hdl_Init();

	while (PLAYBAKTSK_ID) {
		set_flg(FLG_PB, FLGPB_NOTBUSY);
		PROFILE_TASK_IDLE();
		wai_flg(&PlayCMD, FLG_PB, FLGPB_COMMAND, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		clr_flg(FLG_PB, FLGPB_NOTBUSY);

		PlayCMD = guiCurrPlayCmmd;
		DBG_IND("Commd  = %d\r\n", PlayCMD);
		DBG_IND("Status = %2d \r\n", gusCurrPlayStatus);

		if (gusCurrPlayStatus == PB_STA_NOIMAGE) {
			DBG_WRN("Status = NOIMAGE\r\n");
			switch (PlayCMD) {
			default:
				PlayCMD = PB_CMMD_NONE;
				break;
			}
		} else if (gusCurrPlayStatus == PB_STA_INITFAIL) {
			DBG_ERR("Status = INITFAIL\r\n");
			// only support (format) command
			PlayCMD = PB_CMMD_NONE;
		}

		if (PlayCMD == 0 && gusCurrPlayStatus == PB_STA_NOIMAGE) {
			PB_SetFinishFlag(DECODE_JPG_NOIMAGE);
		}


		switch (PlayCMD) {
		//----------------------------------------------------------------------------------
		// Single & Browser & Zoom & Rotate
		//----------------------------------------------------------------------------------
		case PB_CMMD_SINGLE:
		case PB_CMMD_BROWSER:
		case PB_CMMD_ZOOM:
		case PB_CMMD_ROTATE_DISPLAY:
		case PB_CMMD_DE_SPECFILE:
		case PB_CMMD_CUSTOMIZE_DISPLAY:
			PB_Hdl_DisplayImage(PlayCMD);
			break;

		//----------------------------------------------------------------------------------
		// Resize & Requality & Rotate re-encode & ReArrange & Update EXIF-Orientation tag
		//----------------------------------------------------------------------------------
		case PB_CMMD_RE_SIZE:
		case PB_CMMD_RE_QTY:
		case PB_CMMD_UPDATE_EXIF:
		case PB_CMMD_CROP_SAVE:
		case PB_CMMD_CUSTOMIZE_EDIT:
			PB_Hdl_EditImage(PlayCMD);
			break;

		//----------------------------------------------------------------------------------
		// Open file
		//----------------------------------------------------------------------------------
		case PB_CMMD_OPEN_NEXT:
		case PB_CMMD_OPEN_PREV:
		case PB_CMMD_OPEN_SPECFILE:
			PB_Hdl_OpenFile(PlayCMD);
			break;

		//----------------------------------------------------------------------------------
		// Capture screen image
		//----------------------------------------------------------------------------------
		case PB_CMMD_CAPTURE_SCREEN:
			PB_Hdl_CaptureScreen();
			break;

		//----------------------------------------------------------------------------------
		// Others
		//----------------------------------------------------------------------------------
		default:
			break;
		}// End of switch(PlayCMD)

		DBG_IND("Commd = %2d done\r\n", PlayCMD);
	}// End of while (1)
}// End of void PlaybackTsk(void)


void PlaybackDispTsk(void)
{
	FLGPTN flag;

	THREAD_ENTRY(); //kent_tsk();
	
	DBG_IND("Start...\r\n");
	clr_flg(FLG_ID_PB_DRAW, FLGPB_ONDRAW);

	while (PLAYBAKTSK_ID) {
		PROFILE_TASK_IDLE();
		wai_flg(&flag, FLG_ID_PB_DRAW, FLGPB_ONDRAW, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		if (g_fpOnDrawCB) {
			g_fpOnDrawCB(gPbViewState, &g_pPbHdInfo->hd_out_video_frame);
		}

		DBG_IND("Draw done\r\n");
	}// End of while (1)
}

