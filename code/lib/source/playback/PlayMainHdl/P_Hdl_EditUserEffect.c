#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

#if _TODO
#include "PBXEdit.h"
#include "PBXFile.h"

static PBX_EDIT_PARAM m_EditParam = {0};
static IMG_BUF        m_TmpImgBuf = {0};
#endif

#if _TODO
static UINT32 xPB_GetTmpBuff(UINT32 uiNeedBufSZ)
{
	UINT32 DecStartAddr, DecEndAddr = 0;

	gximg_get_buf_addr(g_pDecodedImgBuf, &DecStartAddr, &DecEndAddr);
	if ((guiPlayFileBuf - DecEndAddr) < uiNeedBufSZ) {
		DBG_ERR("Insufficient memory!\r\n");

		return 0;
	}

	return DecEndAddr;
}
#endif

static INT32 xPB_DoUserEffect(void)
{
	#if _TODO
	ER     RtnValue;
	UINT32 uiTmpAddr, uiNeedBufSZ = g_PBUserEdit.uiNeedBufSize;

	uiTmpAddr = xPB_GetTmpBuff(uiNeedBufSZ);
	if (0 == uiTmpAddr) {
		return E_NOMEM;
	}

	PBXEdit_SetFunc((PBX_EDIT_FUNC)g_PBUserEdit.uiPBX_EditFunc);

	m_EditParam.pSrcImg     = g_pDecodedImgBuf;
	m_EditParam.pOutImg     = &m_TmpImgBuf;
	m_EditParam.workBuff    = uiTmpAddr;
	m_EditParam.workBufSize = uiNeedBufSZ;
	m_EditParam.paramNum    = g_PBUserEdit.uiParamNum;
	m_EditParam.param       = g_PBUserEdit.puiParam;
	RtnValue = PBXEdit_Apply(&m_EditParam);

#if 0// dump raw data for debug
	{
		UW uiNewFileSZ;
		FST_FILE fp = NULL;

		DBG_DUMP("^Y[USER] Dump %d x %d raw data..\r\n", m_TmpImgBuf.Width, m_TmpImgBuf.Height);
		uiNewFileSZ = m_TmpImgBuf.Width * m_TmpImgBuf.Height;

		fp = FileSys_OpenFile("A:\\USER_Y.raw", FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		FileSys_WriteFile(fp, (UINT8 *)m_TmpImgBuf.PxlAddr[0], &uiNewFileSZ, 0, NULL);
		FileSys_CloseFile(fp);

		fp = FileSys_OpenFile("A:\\USER_UV.raw", FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		FileSys_WriteFile(fp, (UINT8 *)m_TmpImgBuf.PxlAddr[1], &uiNewFileSZ, 0, NULL);
		FileSys_CloseFile(fp);
	}
#endif

	if (RtnValue != E_OK) {
		DBG_ERR("Customize Effect NG!\r\n");
	} else {
		memcpy(g_pDecodedImgBuf, &m_TmpImgBuf, sizeof(m_TmpImgBuf));
	}

	return RtnValue;
	#else
	return 0;
	#endif
}

void PB_UserDispHandle(void)
{
	ER RtnValue;

	RtnValue = xPB_DoUserEffect();
	if (RtnValue != E_OK) {
		PB_SetFinishFlag(DECODE_JPG_DECODEERROR);

		return;
	}

	// update the displayed image
	if (g_uiExifOrientation != JPEG_EXIF_ORI_DEFAULT) {
		PB_EXIFOrientationHandle();
	} else {
		gPbPushSrcIdx = PB_DISP_SRC_TMP;

		//gximg_scale_data_fine(g_pDecodedImgBuf, GXIMG_REGION_MATCH_IMG, g_pPlayTmpImgBuf, GXIMG_REGION_MATCH_IMG);
		PB_DispSrv_SetDrawCb(PB_VIEW_STATE_SINGLE);
		PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW | PB_DISPSRV_FLIP);
		PB_DispSrv_Trigger();
	}

	PB_SetFinishFlag(DECODE_JPG_DONE);
}

INT32 PB_UserEditHandle(UINT32 IfOverWrite)
{
	#if _TODO
	INT32 iErrChk;
	ER    RtnValue;

	RtnValue = xPB_DoUserEffect();
	if (RtnValue != E_OK) {
		return RtnValue;
	}

	// re-encode the image
	iErrChk = PB_SaveCurrRawBuf(0, 0, gMenuPlayInfo.pJPGInfo->imagewidth, gMenuPlayInfo.pJPGInfo->imageheight, IfOverWrite);
	if ((iErrChk == E_OK) && (FALSE == IfOverWrite)) {
		gMenuPlayInfo.DispFileName[0] = PBXFile_GetInfo(PBX_FILEINFO_MAXFILEID_INDIR, 0);
		gMenuPlayInfo.DispDirName[0]  = PBXFile_GetInfo(PBX_FILEINFO_DIRID, 0);
	}

	return iErrChk;
	#else
	return 0;
	#endif
}

