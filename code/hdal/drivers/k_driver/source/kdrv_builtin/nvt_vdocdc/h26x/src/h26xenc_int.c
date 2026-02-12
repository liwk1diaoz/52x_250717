/*
    H.26x wrapper operation.

    @file       h26x_wrap.c
    @ingroup    mIDrvCodec_H26x
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "h26x_common.h"

#include "h26xenc_int.h"
#include "h264enc_api.h"
#include "h265enc_api.h"
#include "nvt_vdocdc_dbg.h"

extern rc_cb g_rc_cb;

void h26XEnc_getRowRcState(SLICE_TYPE ePicType, H26XRegSet *pRegSet, H26XEncRowRc *pRowRc, UINT32 uiSVCId)
{
	#if (0 == RRC_BY_FRAME_LEVEL)
	UINT32 is_p_frm = IS_PSLICE(ePicType);
	#endif
	UINT32 uiRcEstBitlen;

    //#if SUPPORT_SVC_RC
    #if RRC_BY_FRAME_LEVEL
	pRowRc->uiFrmCostMsb[pRowRc->ucFrameLevel]  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COST_MSB];
	pRowRc->uiFrmCostLsb[pRowRc->ucFrameLevel]  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COST_LSB];
	pRowRc->uiFrmCmpxMsb[pRowRc->ucFrameLevel]  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COMPLEXITY_MSB];
	pRowRc->uiFrmCmpxLsb[pRowRc->ucFrameLevel]  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COMPLEXITY_LSB];
	pRowRc->usRefFrmCoeff[0][pRowRc->ucFrameLevel] = pRegSet->CHK_REPORT[H26X_RRC_COEFF];
	pRowRc->usRefFrmCoeff[1][pRowRc->ucFrameLevel] = (pRegSet->CHK_REPORT[H26X_RRC_COEFF] >> 16);
	pRowRc->usRefFrmCoeff[2][pRowRc->ucFrameLevel] = pRegSet->CHK_REPORT[H26X_RRC_COEFF2];
	pRowRc->usRefFrmCoeff[3][pRowRc->ucFrameLevel] = (pRegSet->CHK_REPORT[H26X_RRC_COEFF2] >> 16);
	pRowRc->usRefFrmCoeff[4][pRowRc->ucFrameLevel] = pRegSet->CHK_REPORT[H26X_RRC_COEFF4];
    #else
	pRowRc->uiFrmCostMsb[is_p_frm]  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COST_MSB];
	pRowRc->uiFrmCostLsb[is_p_frm]  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COST_LSB];
	pRowRc->uiFrmCmpxMsb[is_p_frm]  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COMPLEXITY_MSB];
	pRowRc->uiFrmCmpxLsb[is_p_frm]  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COMPLEXITY_LSB];
	pRowRc->usRefFrmCoeff[0][is_p_frm] = pRegSet->CHK_REPORT[H26X_RRC_COEFF];
	pRowRc->usRefFrmCoeff[1][is_p_frm] = (pRegSet->CHK_REPORT[H26X_RRC_COEFF] >> 16);
	pRowRc->usRefFrmCoeff[2][is_p_frm] = pRegSet->CHK_REPORT[H26X_RRC_COEFF2];
	pRowRc->usRefFrmCoeff[3][is_p_frm] = (pRegSet->CHK_REPORT[H26X_RRC_COEFF2] >> 16);
	pRowRc->usRefFrmCoeff[4][is_p_frm] = pRegSet->CHK_REPORT[H26X_RRC_COEFF4];
    #endif

	uiRcEstBitlen = pRegSet->CHK_REPORT[H26X_RRC_BIT_LEN];
	#if RRC_BY_FRAME_LEVEL
	pRowRc->iPrevPicBitsBias[pRowRc->ucFrameLevel] += (INT32)((pRegSet->CHK_REPORT[H26X_BS_LEN] * 8) - uiRcEstBitlen);
	pRowRc->iPrevPicBitsBias[pRowRc->ucFrameLevel] >>= 1;

	if (((pRowRc->uiPredBits / 2) > (pRegSet->CHK_REPORT[H26X_BS_LEN] * 8)) || ((pRegSet->CHK_REPORT[H26X_BS_LEN] * 8) > (pRowRc->uiPredBits * 2))) {
		int i;
		for (i = 0; i < MAX_RRC_FRAME_LEVEL; i++)
			pRowRc->iPrevPicBitsBias[i] = 0;
	}
	#else
	pRowRc->iPrevPicBitsBias[is_p_frm] += (INT32)((pRegSet->CHK_REPORT[H26X_BS_LEN] * 8) - uiRcEstBitlen);
	pRowRc->iPrevPicBitsBias[is_p_frm] >>= 1;

	if (((pRowRc->uiPredBits / 2) > (pRegSet->CHK_REPORT[H26X_BS_LEN] * 8)) || ((pRegSet->CHK_REPORT[H26X_BS_LEN] * 8) > (pRowRc->uiPredBits * 2))) {
		pRowRc->iPrevPicBitsBias[0] = 0;
		pRowRc->iPrevPicBitsBias[1] = 0;
	}
	#endif
}

void h26XEnc_setInitSde(H26XRegSet *pRegSet)
{
	// DCT_QTBL0_DEC //
	save_to_reg(&pRegSet->SDE_CFG[4], 28,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[4], 35, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[5], 45,  0, 10);

	// DCT_QTBL1_DEC //
	save_to_reg(&pRegSet->SDE_CFG[6], 36,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[6], 46, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[7], 58,  0, 10);

	// DCT_QTBL2_DEC //
	save_to_reg(&pRegSet->SDE_CFG[8], 40,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[8], 51, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[9], 64,  0, 10);

	// DCT_QTBL3_DEC //
	save_to_reg(&pRegSet->SDE_CFG[10], 44,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[10], 56, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[11], 70,  0, 10);

	// DCT_QTBL4_DEC //
	save_to_reg(&pRegSet->SDE_CFG[12], 52,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[12], 66, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[13], 83,  0, 10);

	// DCT_QTBL5_DEC //
	save_to_reg(&pRegSet->SDE_CFG[14], 56,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[14], 71, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[15], 90,  0, 10);

	// DCT_QTBL6_DEC //
	save_to_reg(&pRegSet->SDE_CFG[16], 64,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[16], 81, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[17], 102, 0, 10);

	// DCT_QTBL7_DEC //
	save_to_reg(&pRegSet->SDE_CFG[18], 72,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[18], 91, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[19], 115, 0, 10);

	// DCT_QTBL8_DEC //
	save_to_reg(&pRegSet->SDE_CFG[20], 80,   0, 10);
	save_to_reg(&pRegSet->SDE_CFG[20], 101, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[21], 128,  0, 10);

	// DCT_QTBL9_DEC //
	save_to_reg(&pRegSet->SDE_CFG[22], 88,   0, 10);
	save_to_reg(&pRegSet->SDE_CFG[22], 111, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[23], 141,  0, 10);

	// DCT_QTBL10_DEC //
	save_to_reg(&pRegSet->SDE_CFG[24], 104,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[24], 132, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[25], 166,  0, 10);

	// DCT_QTBL11_DEC //
	save_to_reg(&pRegSet->SDE_CFG[26], 112,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[26], 142, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[27], 179,  0, 10);

	// DCT_QTBL12_DEC //
	save_to_reg(&pRegSet->SDE_CFG[28], 128,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[28], 162, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[29], 205,  0, 10);

	// DCT_QTBL13_DEC //
	save_to_reg(&pRegSet->SDE_CFG[30], 144,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[30], 182, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[31], 230,  0, 10);

	// DCT_QTBL14_DEC //
	save_to_reg(&pRegSet->SDE_CFG[32], 160,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[32], 202, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[33], 256,  0, 10);

	// DCT_QTBL15_DEC //
	save_to_reg(&pRegSet->SDE_CFG[34], 224,  0, 10);
	save_to_reg(&pRegSet->SDE_CFG[34], 283, 16, 10);
	save_to_reg(&pRegSet->SDE_CFG[35], 358,  0, 10);

	// DCT_QTBL_DCMAX //
	save_to_reg(&pRegSet->SDE_CFG[36], 292,  0,  9);
	save_to_reg(&pRegSet->SDE_CFG[36], 227, 16,  9);
	save_to_reg(&pRegSet->SDE_CFG[37], 204,  0,  9);
	save_to_reg(&pRegSet->SDE_CFG[37], 186, 16,  9);
	save_to_reg(&pRegSet->SDE_CFG[38], 157,  0,  9);
	save_to_reg(&pRegSet->SDE_CFG[38], 146, 16,  9);
	save_to_reg(&pRegSet->SDE_CFG[39], 128,  0,  9);
	save_to_reg(&pRegSet->SDE_CFG[39], 114, 16,  9);
	save_to_reg(&pRegSet->SDE_CFG[40], 102,  0,  9);
	save_to_reg(&pRegSet->SDE_CFG[40], 93, 16,  9);
	save_to_reg(&pRegSet->SDE_CFG[41], 79,  0,  9);
	save_to_reg(&pRegSet->SDE_CFG[41], 73, 16,  9);
	save_to_reg(&pRegSet->SDE_CFG[42], 64,  0,  9);
	save_to_reg(&pRegSet->SDE_CFG[42], 57, 16,  9);
	save_to_reg(&pRegSet->SDE_CFG[43], 51,  0,  9);
	save_to_reg(&pRegSet->SDE_CFG[43], 37, 16,  9);

	// Dithering init //
	save_to_reg(&pRegSet->SDE_CFG[44], 0x2aaa,  0,  15);
	save_to_reg(&pRegSet->SDE_CFG[44], 0xa, 16,  4);
}

UINT8 h26XEnc_getRcQuant(H26XENC_VAR *pVar, SLICE_TYPE ePicType, BOOL bIsLTRFrm, UINT32 uiBsOutBufSize)
{
	H26XFUNC_CTX *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	H26XEncRCPreparePic sRcPic = {0};
	int qp;
	UINT8 ucQP;

	sRcPic.ucPicType = IS_ISLICE(ePicType) ? RC_I_SLICE : RC_P_SLICE;
	sRcPic.uiLTRFrame = bIsLTRFrm;
	sRcPic.uiEncodeRatio = pComnCtx->uiEncRatio;
	sRcPic.uiSVCLayer = IS_ISLICE(ePicType) ? 0 : (pVar->eCodecType == VCODEC_H264) ? h264Enc_getSVCLabel(pVar) : h265Enc_getSVCLabel(pVar);
	sRcPic.uiMaxFrameByte = uiBsOutBufSize;
	sRcPic.uiAslog2 = pFuncCtx->stAq.stCfg.ucAslog2;

	pComnCtx->uiRcLogLen = (g_rc_dump_log == 1) ? (UINT32)g_rc_cb.h26xEnc_RcGetLog(&pComnCtx->stRc, (unsigned int *)&pComnCtx->uipRcLogAddr) : 0;

	qp = g_rc_cb.h26xEnc_RcPreparePicture(&pComnCtx->stRc, &sRcPic);
	if (qp < 0) {
		ucQP = 0xFF;
		goto exit_rc;
	}
	else
		ucQP = (UINT8)qp;

   	if (sRcPic.uiMotionAQStr) {
		pFuncCtx->stMAq.stCfg.cDqp[0] = sRcPic.uiMotionAQStr;
        //pFuncCtx->stMAq.stCfg.cDqp[1] = pComnCtx->stRc.m_motionAQStrength;
    }
    else {
        pFuncCtx->stMAq.stCfg.cDqp[0] = -16;
        //pFuncCtx->stMAq.stCfg.cDqp[1] = -16;
    }
    h26XEnc_setMotAqCfg(pVar, &pFuncCtx->stMAq.stCfg);

	// update row level rate control info //
	pFuncCtx->stRowRc.uiPredBits = sRcPic.uiPredSize;
	#if RRC_BY_FRAME_LEVEL
	if (sRcPic.uiFrameLevel < MAX_RRC_FRAME_LEVEL)
		pFuncCtx->stRowRc.ucFrameLevel = (UINT8)sRcPic.uiFrameLevel;
	else {
		if (IS_ISLICE(ePicType))
			pFuncCtx->stRowRc.ucFrameLevel = 0;
		else
			pFuncCtx->stRowRc.ucFrameLevel = 1;
	}
	#endif
	DBG_IND("slice %d, qp %d\n", sRcPic.ucPicType, ucQP);

exit_rc:
	return ucQP;
}

void h26XEnc_UpdateRc(H26XENC_VAR *pVar, SLICE_TYPE ePicType, H26XEncResultCfg *pResult)
{
	H26XCOMN_CTX *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XEncRCUpdatePic sRcPic = {0};

	sRcPic.ucPicType = IS_ISLICE(ePicType) ? RC_I_SLICE : RC_P_SLICE;
	sRcPic.uiYMSE[0] = pResult->uiYPSNR[0];
	sRcPic.uiYMSE[1] = pResult->uiYPSNR[1];
	sRcPic.uiFrameSize = pResult->uiBSLen;
	sRcPic.uiAvgQP = pResult->uiAvgQP;
	DBG_IND("slice %d, frame size %d, avg qp %d\n", sRcPic.ucPicType, sRcPic.uiFrameSize, sRcPic.uiAvgQP);

	sRcPic.uiMotionRatio = pResult->uiMotionRatio;

	g_rc_cb.h26xEnc_RcUpdatePicture(&pComnCtx->stRc, &sRcPic);

	pResult->bEVBRStillFlag = sRcPic.bEVBRStillFlag;
}
