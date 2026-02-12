#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#if defined(__FREERTOS)
#include "crypto.h"
#else  // defined(__FREERTOS)
#include <crypto/cryptodev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#endif // defined(__FREERTOS)

PB_ERR PB_SetParam(PB_PRMID ParamID, UINT32 uiValue)
{
	PB_ERR ErrCode = PBERR_OK;

	switch (ParamID) {
	case PBPRMID_PLAYBACK_OBJ:
		xPB_SetPBObject((PPLAY_OBJ)uiValue);
		break;

	case PBPRMID_MAX_PANELSZ:
		g_uiMAXPanelSize = uiValue;
		break;

	case PBPRMID_MAX_FILE_SIZE:
		g_uiMaxFileSize = uiValue;
		break;
		
	case PBPRMID_MAX_RAW_SIZE:
		g_uiMaxRawSize = uiValue;
		break;

	case PBPRMID_MAX_DECODE_WIDTH:
		g_uiMaxDecodeW = uiValue;
		break;

	case PBPRMID_MAX_DECODE_HEIGHT:
		g_uiMaxDecodeH = uiValue;
		break;

	case PBPRMID_PIXEL_FORMAT:
		g_pPbHdInfo->hd_vdo_pix_fmt = (HD_VIDEO_PXLFMT)uiValue;
		break;

    case PBPRMID_HD_VIDEODEC_PATH:
		g_pPbHdInfo->p_hd_vdec_path = (HD_PATH_ID *)uiValue;
		break;

	case PBPRMID_EN_FLAGS:
		g_PBSetting.EnableFlags = uiValue;
		break;

	case PBPRMID_BG_COLOR:
		g_uiDefaultBGColor = uiValue;
		break;

	case PBPRMID_ZOOM_LVL_TBL:
		gMenuPlayInfo.uipZoomLevelTbl = (UINT32 *)uiValue;
		break;


	case PBPRMID_DISP_DIRECTION:
		g_PBSetting.DispDirection = uiValue;
		break;

	case PBPRMID_THUMB_SHOW_METHOD:
		g_PBSetting.ThumbShowMethod = uiValue;
		break;

	case PBPRMID_THUMB_LAYOUT_ARRAY:
		gMenuPlayInfo.pRectDrawThumb = (PURECT)uiValue;
		break;

//#NT#2012/11/27#Scottie -begin
//#NT#Support Dual Display for PB
	case PBPRMID_THUMB_LAYOUT_ARRAY2:
		gMenuPlayInfo.pRectDrawThumb2 = (PURECT)uiValue;
		break;
//#NT#2012/11/27#Scottie -end

	case PBPRMID_AUTO_ROTATE:
		g_PBSetting.bEnableAutoRotate = uiValue;
		break;

	case PBPRMID_SLIDE_SPEED_TBL:
		g_PBSetting.pSlideEffectSpeedTbl = (UINT8 *)uiValue;
		break;

	case PBPRMID_FILEDB_HANDLE:
		if (g_PBSetting.EnableFlags & PB_ENABLE_SEARCH_FILE_WITHOUT_DCF) {
			g_PBSetting.FileHandle = (INT32)uiValue;
		} else {
			g_PBSetting.FileHandle = 0;

			DBG_ERR("FileDB is disabled!\r\n");
			ErrCode = PBERR_FAIL;
		}
		break;
	//#NT#2013/02/20#Lincy Lin -begin
	//#NT#Support Filelist plugin
	case PBPRMID_FILELIST_OBJ:
		g_PBSetting.pFileListObj = (PPBX_FLIST_OBJ)uiValue;
		break;

	//#NT#2013/02/20#Lincy Lin -end
	//#NT#2013/04/18#Ben Wang -begin
	//#NT#Add callback for decoding image and video files.
	case PBPRMID_DEC_IMG_CALLBACK:
		g_fpDecImageCB = (PB_DECIMG_CB)uiValue;
		break;

	case PBPRMID_DEC_VIDEO_CALLBACK:
		g_fpDecVideoCB = (PB_DECVIDEO_CB)uiValue;
		break;
	//#NT#2013/04/18#Ben Wang -end

//#NT#2013/07/02#Scottie -begin
//#NT#Add callback for doing something before trigger next (DspSrv)
	case PBPRMID_CFG4NEXT_CALLBACK:
		g_fpCfg4NextCB = (PB_CFG4NEXT_CB)uiValue;
		break;
//#NT#2013/07/02#Scottie -end

	case PBPRMID_DISP_TRIG_CALLBACK:
		g_fpDispTrigCB = (PB_DISPTRIG_CB)uiValue;
		break;

	case PBPRMID_ONDRAW_CALLBACK:
		g_fpOnDrawCB = (PB_ONDRAW_CB)uiValue;
		break;

	case PBPRMID_DUAL_DISP:
		gDualDisp = uiValue;
		break;

	case PBPRMID_ENABLE_DEBUG_MSG:
		g_uiPbDebugMsg = uiValue;	

	case PBPRMID_VID_DECRYPT_KEY:
		{
			if (uiValue == 0) {
				g_PBSetting.bEnableVidDecrypt = FALSE;
			} else {

				if(guiOnPlaybackMode == TRUE) {
					DBG_ERR("Video decrypto key must be set before open!\r\n");
			        ErrCode = PBERR_FAIL;
			    } else {

			    	UINT8 *key = (UINT8 *)uiValue;

					for (int i = 0; i < VID_DECRYPT_KEY_LEN; i++) {
						g_PBSetting.VidDecryptKey[i] = *(key + i);
					}
					g_PBSetting.bEnableVidDecrypt = TRUE;
			    }
			}
		}
		break;
	case PBPRMID_VID_DECRYPT_MODE:
		{
			if (uiValue == PB_DECRYPT_MODE_AES128 || uiValue == PB_DECRYPT_MODE_AES256) {
				g_PBSetting.VidDecryptMode = uiValue;
			} else {
				DBG_ERR("Not supported mode = %d\r\n", uiValue);
			}
		}
		break;
	case PBPRMID_VID_DECRYPT_POS:
		{
			g_PBSetting.VidDecryptPos = uiValue;
		}
		break;
	case PBPRMID_JPG_DECRYPT_KEY:
		{
			UINT8 *key = (UINT8 *)uiValue;

			if (key == NULL) {
				g_PBSetting.bEnableJpgDecrypt = FALSE;
			} else {
				if(guiOnPlaybackMode == TRUE) {
					DBG_ERR("JPG decrypto key must be set before open!\r\n");
					ErrCode = PBERR_FAIL;
				} else {
					g_PBSetting.JpgDecryptKey = key;
					g_PBSetting.bEnableJpgDecrypt = TRUE;
				}
			}
		}
		break;
	case PBPRMID_JPG_DECRYPT_LEN:
		{
			g_PBSetting.JpgDecryptLen = uiValue;
		}
		break;

	default:
		DBG_ERR("Unsupported parameter!\r\n");
		ErrCode = PBERR_PAR;
		break;
	}

	return ErrCode;
}

