#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

PB_ERR PB_GetParam(PB_PRMID ParamID, UINT32 *puiVal)
{
	PB_ERR ErrCode = PBERR_OK;

	switch (ParamID) {
	case PBPRMID_FILE_ATTR_LOCK:
		if (xPB_GetCurFileFormat() & PBFMT_READONLY) {
			*puiVal = TRUE;
		} else {
			*puiVal = FALSE;
		}
		break;

	case PBPRMID_JPEG_WITH_VOICE:
		*puiVal = xPB_GetFILEWithMemo();
		break;

	case PBPRMID_PLAYBACK_MODE:
		*puiVal = gMenuPlayInfo.CurrentMode;
		break;

	case PBPRMID_PLAYBACK_STATUS:
		*puiVal = gusCurrPlayStatus;
		break;

	case PBPRMID_PLAYBACK_OBJ:
		memcpy(puiVal, (void *)&g_PBObj, sizeof(PLAY_OBJ));
		break;

	case PBPRMID_INFO_IMG:
		memcpy(puiVal, (void *)&g_pPbHdInfo->hd_out_video_frame, sizeof(HD_VIDEO_FRAME));
		break;

	case PBPRMID_INFO_VDO:		
		memcpy(puiVal, (void *)&g_PBVideoInfo, sizeof(GXVIDEO_INFO));		
		break;

	case PBPRMID_ROTATE_DIR:
		*puiVal = gMenuPlayInfo.RotateDir;
		break;

	case PBPRMID_MAX_PANELSZ:
		*puiVal = g_uiMAXPanelSize;
		break;

	case PBPRMID_MAX_FILE_SIZE:
		*puiVal = g_uiMaxFileSize;
		break;

	case PBPRMID_MAX_RAW_SIZE:
		*puiVal = g_uiMaxRawSize;
		break;

	case PBPRMID_MAX_DECODE_WIDTH:
		*puiVal = g_uiMaxDecodeW;
		break;

	case PBPRMID_MAX_DECODE_HEIGHT:
		*puiVal = g_uiMaxDecodeH;
		break;
		
	case PBPRMID_PIXEL_FORMAT:
		*puiVal = (UINT32)g_pPbHdInfo->hd_vdo_pix_fmt;
		break;

    case PBPRMID_HD_VIDEODEC_PATH:
		*puiVal = (UINT32)g_pPbHdInfo->p_hd_vdec_path;
		break;

	case PBPRMID_EN_FLAGS:
		*puiVal = g_PBSetting.EnableFlags;
		break;

	case PBPRMID_THUMB_CURR_IDX:
		*puiVal = gMenuPlayInfo.CurFileIndex;
		break;

	case PBPRMID_THUMB_CURR_NUM:
		*puiVal = gMenuPlayInfo.DispThumbNums;
		break;

	case PBPRMID_THUMB_FMT_ARRAY:
		*puiVal = (UINT32)&gMenuPlayInfo.AllThumbFormat[0];
		break;

	case PBPRMID_THUMB_DEC_ARRAY:
		*puiVal = (UINT32)&gMenuPlayInfo.bThumbDecErr[0];
		break;

	case PBPRMID_THUMB_SEQ_ARRAY:
		*puiVal = (UINT32)&gMenuPlayInfo.DispFileSeq[0];
		break;

	case PBPRMID_THUMB_VDO_LTH_ARRAY:
		*puiVal = (UINT32)&gMenuPlayInfo.MovieLength[0];
		break;

	case PBPRMID_ZOOM_INDEX:
		*puiVal = gMenuPlayInfo.ZoomIndex;
		break;

    case PBPRMID_FILE_BUF_HD_INFO:
		memcpy(puiVal, (void *)&g_hd_file_buf, sizeof(PB_HD_COM_BUF));
		break;

    case PBPRMID_INFO_IMG_HDBUF:
		memcpy(puiVal, (void *)&g_hd_raw_buf, sizeof(PB_HD_COM_BUF));
		break;
		
	case PBPRMID_EXIF_BUF_HD_INFO:
		memcpy(puiVal, (void *)&g_hd_exif_buf, sizeof(PB_HD_COM_BUF));
		break;


	//
	// PBZoom Pan info
	//
	case PBPRMID_PAN_CURX:
		*puiVal = gMenuPlayInfo.PanSrcStartX;
		break;
	case PBPRMID_PAN_CURY:
		*puiVal = gMenuPlayInfo.PanSrcStartY;
		break;
	case PBPRMID_PAN_MAXX:
		*puiVal = gMenuPlayInfo.PanMaxX;
		break;
	case PBPRMID_PAN_MAXY:
		*puiVal = gMenuPlayInfo.PanMaxY;
		break;


	case PBPRMID_NAMEID_FILE:
		*puiVal = xPB_GetCurFileNameId();
		break;

	case PBPRMID_DATABUF_ADDR:
		*puiVal = guiPlayFileBuf;
		break;

	case PBPRMID_DATABUF_SIZE:
		*puiVal = (g_uiPBBufEnd - guiPlayFileBuf);
		break;

	case PBPRMID_NAMEID_DIR:
		*puiVal = xPB_GetCurFileDirId();
		break;

	case PBPRMID_CURR_FILEPATH: {
			PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILEPATH, puiVal, NULL);
		}
		break;
	case PBPRMID_CURR_FILESEQ: {
			PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, puiVal, NULL);
		}
		break;
	case PBPRMID_CURR_FILESIZE: {
			PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;
			UINT64 tmpSize;
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &tmpSize, NULL);
			*puiVal = (UINT32)tmpSize;
		}
		break;
	case PBPRMID_CURR_FILEFMT:
		*puiVal = xPB_GetCurFileFormat();
		break;

	case PBPRMID_TOTAL_FILE_COUNT: {
			PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILENUMS, puiVal, NULL);
		}
		break;
	case PBPRMID_SRC_REGION: {
			*puiVal = (UINT32)&gSrcRegion;
			break;
		}
	case PBPRMID_VDO_RECT_ARRAY: {
			*puiVal = (UINT32)&g_PB1stVdoRect[0];
			break;
		}
	case PBPRMID_SRC_SIZE: {
			PUSIZE size = (PUSIZE)puiVal;
			size->w = gSrcWidth;
			size->h = gSrcHeight;
			break;
		}
	case PBPRMID_DST_REGION: {
			*puiVal = (UINT32)&gDstRegion;
			break;
		}
	case PBPRMID_VDO_WIN_ARRAY: {
			*puiVal = (UINT32)&g_PBVdoWIN[0];
			break;
		}
	case PBPRMID_THUMB_RECT: {
			*puiVal = (UINT32)&g_PBIdxView[0][0];
			break;
		}
	case PBPRMID_THUMB_WIDTH_ARRAY: {
			*puiVal = (UINT32)&ThumbSrcW[0];
			break;
		}
	case PBPRMID_THUMB_HEIGHT_ARRAY: {
			*puiVal = (UINT32)&ThumbSrcH[0];
			break;
		}
	case PBPRMID_EXIF_EXIST: {
			*puiVal = g_bExifExist;
			break;
		}
	case PBPRMID_EXIF_ORIENT: {
			*puiVal = g_uiExifOrientation;
			break;
		}

	case PBPRMID_ENABLE_DEBUG_MSG: {
		*puiVal = g_uiPbDebugMsg;
		break;			
		}

	default:
		DBG_ERR("Unsupported parameter!\r\n");
		ErrCode = PBERR_PAR;
		break;
	}

	return ErrCode;
}

