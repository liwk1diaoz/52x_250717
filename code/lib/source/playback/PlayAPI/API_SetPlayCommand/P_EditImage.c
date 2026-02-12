#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
//#include "PBXEdit.h"

PB_ERR PB_EditImage(PPB_EDIT_CONFIG pConfig)
{
	PB_ERR ErrCode = PBERR_OK;

	if (!(xPB_GetCurFileFormat() & PBFMT_JPG)) {
		DBG_ERR("Invalid file format!\r\n");
		return PBERR_NOT_SUPPORT;
	}

	switch (pConfig->Operation) {
	case PBEDIT_RESIZE:
		if ((pConfig->puiParam[0] != 0) && (pConfig->puiParam[1] != 0)) {
			gusPlayResizeNewWidth  = pConfig->puiParam[0];
			gusPlayResizeNewHeight = pConfig->puiParam[1];
			g_bPBSaveOverWrite     = pConfig->puiParam[2];
			PB_PlaySetCmmd(PB_CMMD_RE_SIZE);
		} else {
			DBG_ERR("Invalid Parameter(s) for PBEDIT_RESIZE!\r\n");
			ErrCode = PBERR_PAR;
		}
		break;

	case PBEDIT_REQTY:
		if (pConfig->puiParam[0] != 0) {
			gucPlayReqtyQuality = pConfig->puiParam[0];
			g_bPBSaveOverWrite  = pConfig->puiParam[1];
			PB_PlaySetCmmd(PB_CMMD_RE_QTY);
		} else {
			DBG_ERR("Invalid Parameter(s) for PBEDIT_REQTY!\r\n");
			ErrCode = PBERR_PAR;
		}
		break;

	case PBEDIT_ROTATE:
		if (pConfig->puiParam[0] != 0) {
			gPB_EXIFOri.ucEXIFOri  = pConfig->puiParam[0];
			gPB_EXIFOri.bDecodeIMG = pConfig->puiParam[1];
			g_bPBSaveOverWrite     = pConfig->puiParam[2];
			PB_PlaySetCmmd(PB_CMMD_UPDATE_EXIF);
		} else {
			DBG_ERR("Invalid Parameter(s) for PBEDIT_ROTATE!\r\n");
			ErrCode = PBERR_PAR;
		}
		break;

	case PBEDIT_ROTATE_DISP:
		if ((pConfig->puiParam[0] > PLAY_ROTATE_DIR_VER) && (pConfig->puiParam[0] != PLAY_ROTATE_DIR_0)) {
			// Not support
			DBG_ERR("Invalid Parameter(s) for PBEDIT_ROTATEDISP!\r\n");
			ErrCode = PBERR_PAR;
		} else {
			g_uiPBRotateDir = pConfig->puiParam[0];
			PB_PlaySetCmmd(PB_CMMD_ROTATE_DISPLAY);
		}
		break;

	case PBEDIT_CROP:
		if ((pConfig->puiParam[2] != 0) && (pConfig->puiParam[3] != 0)) {
			gPBCmdObjCropSave.DisplayStart_X = pConfig->puiParam[0];
			gPBCmdObjCropSave.DisplayStart_Y = pConfig->puiParam[1];
			gPBCmdObjCropSave.DisplayWidth   = pConfig->puiParam[2];
			gPBCmdObjCropSave.DisplayHeight  = pConfig->puiParam[3];
			g_bPBSaveOverWrite = FALSE;
			PB_PlaySetCmmd(PB_CMMD_CROP_SAVE);
		} else {
			DBG_ERR("Invalid Parameter(s) for PBEDIT_CROP!\r\n");
			ErrCode = PBERR_PAR;
		}
		break;

	case PBEDIT_CUSTOMIZE_DISP:
		g_PBUserEdit.uiPBX_EditFunc = pConfig->puiParam[0];
		g_PBUserEdit.uiNeedBufSize  = pConfig->puiParam[1];
		PB_PlaySetCmmd(PB_CMMD_CUSTOMIZE_DISPLAY);
		break;

	case PBEDIT_CUSTOMIZE:
		g_PBUserEdit.uiPBX_EditFunc = pConfig->puiParam[0];
		g_PBUserEdit.uiNeedBufSize  = pConfig->puiParam[1];
		g_bPBSaveOverWrite = pConfig->puiParam[2];
		PB_PlaySetCmmd(PB_CMMD_CUSTOMIZE_EDIT);
		break;

	case PBEDIT_CUSTOMIZE_PARA:
		g_PBUserEdit.uiParamNum = pConfig->puiParam[0];
		g_PBUserEdit.puiParam   = (UINT32 *) pConfig->puiParam[1];
		PB_SetFinishFlag(DECODE_JPG_DONE);
		break;

	default:
		ErrCode = PBERR_NOT_SUPPORT;
		break;
	}

	return ErrCode;
}

