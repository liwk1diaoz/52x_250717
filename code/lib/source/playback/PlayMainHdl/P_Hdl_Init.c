#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"
#include "hdal.h"
#include "hd_type.h"
#include <kwrap/util.h>

#if _TODO
JPGHEAD_DEC_CFG g_PBJDCParams;
#endif

void PB_Hdl_Init(void)
{
	UINT32          uiAllFileNums;
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;

	if (!pFlist) {
		DBG_ERR("FileList is NULL\r\n");
		return;
	}
	pFlist->Init();

	DBG_IND("Initial...\r\n");
	guiCurrPlayCmmd = PB_CMMD_NONE;
	gusCurrPlayStatus = PB_STA_UNKNOWN;
	gMenuPlayInfo.ZoomIndex  = 0;        // Default = 1X
	gMenuPlayInfo.RotateDir  = PLAY_ROTATE_DIR_UNKNOWN;
	gMenuPlayInfo.JumpOffset   = 1;
	gMenuPlayInfo.CurFileIndex = 1;
	PB_GetDefaultFileBufAddr();

#if _TODO
	// Decode one JPG info
	gDecOneJPGInfo.p_src_addr        = (UINT8 *)guiPlayFileBuf;
	gDecOneJPGInfo.p_dst_addr        = (UINT8 *)guiPlayRawBuf;
	gDecOneJPGInfo.p_dec_cfg         = &g_PBJDCParams;
	gDecOneJPGInfo.enable_timeout    = TRUE;
	gDecOneJPGInfo.timer_start_cb    = PB_JpgDecodeTimerStartCB;
	gDecOneJPGInfo.timer_wait_cb     = PB_JpgDecodeTimerWaitCB;
	gDecOneJPGInfo.timer_pause_cb    = PB_JpgDecodeTimerPauseCB;
	if (g_PBSetting.EnableFlags & PB_ENABLE_SPEEDUP_SCREENNAIL) {
		gDecOneJPGInfo.speed_up_sn = TRUE;
	} else {
		gDecOneJPGInfo.speed_up_sn = FALSE;
	}
#endif

	if (PB_WaitFileSystemInit() != E_OK) {
		DBG_ERR("FileSys Init Fail\r\n");
		PB_SetFinishFlag(DECODE_JPG_FILESYSFAIL);
	} else {
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILENUMS, &uiAllFileNums, NULL);
	
		if (uiAllFileNums == 0) {
            vos_util_delay_ms(300); //wait for creating PlaybackDispTsk thread.
			PB_SetFinishFlag(DECODE_JPG_NOIMAGE);
		} else {
			PB_SetFinishFlag(DECODE_JPG_DONE);
		}
	}
}

