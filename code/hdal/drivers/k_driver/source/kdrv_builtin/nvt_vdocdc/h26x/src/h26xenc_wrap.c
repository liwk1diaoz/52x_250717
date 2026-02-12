/*
    H.26x wrapper operation.

    @file       h26x_wrap.c
    @ingroup    mIDrvCodec_H26x
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "nvt_vdocdc_dbg.h"
#include "h26x_common.h"
#include "h26xenc_api.h"

#include "h26xenc_wrap.h"

#if H26X_SYNC_ROWRC_MAXMIN_QP
unsigned int g_RRCSyncRCQPCond = 1;
#endif

void h26xEnc_wrapUsrQpCfg(H26XRegSet *pRegSet, H26XUsrQp *pUsrQp, CODEC_TYPE eCodecType)
{
	save_to_reg(&pRegSet->FUNC_CFG[0], pUsrQp->stCfg.bEnable, 8, 1);
	save_to_reg(&pRegSet->QP_MAP_ADDR, h26x_getPhyAddr(pUsrQp->uiQpMapAddr), 0, 32);
	save_to_reg(&pRegSet->QP_MAP_LINE_OFFSET, pUsrQp->stCfg.uiQpMapLineOffset/4, 0, 16);
}

void h26xEnc_wrapPSNRCfg(H26XRegSet *pRegSet, BOOL bEnable)
{
	save_to_reg(&pRegSet->FUNC_CFG[0], bEnable, 12, 1);
}

void h26xEnc_wrapSliceSplitCfg(H26XRegSet *pRegSet, H26XEncSliceSplit *pSliceSplit)
{
	save_to_reg(&pRegSet->PIC_CFG, pSliceSplit->stExe.uiSliceRowNum, 16, 16);
}

void h26xEnc_wrapVarCfg(H26XRegSet *pRegSet, H26XEncVarCfg *pVarCfg)
{
	save_to_reg(&pRegSet->VAR_CFG[0], pVarCfg->usThreshold, 1, 11);
	save_to_reg(&pRegSet->VAR_CFG[0], pVarCfg->ucAVGMin, 12, 8);
	save_to_reg(&pRegSet->VAR_CFG[0], pVarCfg->ucAVGMax, 20, 8);
	save_to_reg(&pRegSet->VAR_CFG[0], pVarCfg->ucDelta, 28, 4);

	save_to_reg(&pRegSet->VAR_CFG[1], pVarCfg->ucIRangeDelta, 0, 8);
	save_to_reg(&pRegSet->VAR_CFG[1], pVarCfg->ucPRangeDelta, 8, 8);
	save_to_reg(&pRegSet->VAR_CFG[1], pVarCfg->ucMerage, 16, 2);
}

void h26xEnc_wrapGdrCfg(H26XRegSet *pRegSet, H26XEncGdr *pGdr, UINT16 usRowHeight, UINT32 uiPicCnt, BOOL bIsIfrm)
{
	if (pGdr->stCfg.bEnable)
	{
		if (bIsIfrm) {
			if (pGdr->stCfg.bGDRIFrm == 1) {
				pGdr->uiStartPicCnt = uiPicCnt;
				//265
				if ((pRegSet->FUNC_CFG[0] & 1) == 0) {
					pGdr->usStart = H265E_GDRI_MIN_SLICE_HEIGHT/16;
				} else {
					pGdr->usStart = H264E_GDRI_MIN_SLICE_HEIGHT/16;
				}
			} else {
				pGdr->uiStartPicCnt = uiPicCnt + pGdr->stCfg.usPeriod;
				pGdr->usStart = 0;
			}
		}

		if (bIsIfrm == 0 && uiPicCnt >= pGdr->uiStartPicCnt) {
			UINT16 usNxtStart = pGdr->usStart + pGdr->stCfg.usNumber;

			save_to_reg(&pRegSet->GDR_CFG[0], pGdr->stCfg.bEnable, 0, 1);
			if (pGdr->stCfg.bGDRIFrm == 1) {
				if (pGdr->usStart > 2) {
					save_to_reg(&pRegSet->GDR_CFG[1], pGdr->usStart-2, 0, 16);
				} else {
					save_to_reg(&pRegSet->GDR_CFG[1], pGdr->usStart, 0, 16);
				}
			} else {
				save_to_reg(&pRegSet->GDR_CFG[1], pGdr->usStart, 0, 16);
			}
			save_to_reg(&pRegSet->GDR_CFG[1], usNxtStart,   16, 16);

			pGdr->usStart = (usNxtStart < usRowHeight) ? usNxtStart : 0;

			if (pGdr->usStart == 0) {
				pGdr->uiStartPicCnt += pGdr->stCfg.usPeriod;
			}

			//gdr qp
			save_to_reg(&pRegSet->GDR_CFG[0], 1, 1, 1);
			save_to_reg(&pRegSet->GDR_CFG[0], pGdr->stCfg.ucGDRQp, 2, 6);
		}
		else {
			save_to_reg(&pRegSet->GDR_CFG[0], 0,  0, 1);
			save_to_reg(&pRegSet->GDR_CFG[0], 0,  1, 1);
			save_to_reg(&pRegSet->GDR_CFG[1], 0,  0, 16);
			save_to_reg(&pRegSet->GDR_CFG[1], 0, 16, 16);
		}
	}
}

void h26xEnc_wrapRoiCfg(H26XRegSet *pRegSet, H26XEncRoi *pRoi, UINT8 ucIdx)
{
	save_to_reg(&pRegSet->ROI_CFG[0], (pRoi->ucNumber != 0), 0, 1);

	if (pRoi->stCfg[ucIdx].bEnable == TRUE)
	{
		save_to_reg(&pRegSet->ROI_CFG[ucIdx*2+1], pRoi->stCfg[ucIdx].usCoord_X,  0, 9);
		save_to_reg(&pRegSet->ROI_CFG[ucIdx*2+1], pRoi->stCfg[ucIdx].usCoord_Y, 12, 9);
		save_to_reg(&pRegSet->ROI_CFG[ucIdx*2+1], pRoi->stCfg[ucIdx].cQP,       24, 6);

		save_to_reg(&pRegSet->ROI_CFG[ucIdx*2+2], (pRoi->stCfg[ucIdx].usCoord_X + pRoi->stCfg[ucIdx].usWidth),   0, 9);
		save_to_reg(&pRegSet->ROI_CFG[ucIdx*2+2], (pRoi->stCfg[ucIdx].usCoord_Y + pRoi->stCfg[ucIdx].usHeight), 12, 9);
		save_to_reg(&pRegSet->ROI_CFG[ucIdx*2+2], pRoi->stCfg[ucIdx].ucMode, 24, 7);
	}
	else
	{
		save_to_reg(&pRegSet->ROI_CFG[ucIdx*2+1], 0, 0, 32);
		save_to_reg(&pRegSet->ROI_CFG[ucIdx*2+2], 0, 0, 32);
	}
}

void h26xEnc_wrapRowRcCfg(H26XRegSet *pRegSet, H26XEncRowRc *pRowRc)
{
	// start to calulate row rc coefficient eanble at encode start, so update parameter settings while enable row rc //
	// avoid abnormal row rc settings while disable row rc cause calulate row rc coefficient risk                            //

	if (pRowRc->stCfg.bEnable == TRUE)
	{
		save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->stCfg.ucScale,       8, 3);
		save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->stCfg.ucQPRange[0], 12, 4);
		save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->stCfg.ucQPStep[0],  16, 4);
		save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->ucMinCostTh,     	  20, 4);	// non-release at api //

		save_to_reg(&pRegSet->RRC_CFG[1], pRowRc->stCfg.ucMinQP[0],    8, 6);
		save_to_reg(&pRegSet->RRC_CFG[1], pRowRc->stCfg.ucMaxQP[0],   16, 6);
	}
}

void h26xEnc_wrapRowRcPicUpdate(H26XRegSet *pRegSet, H26XEncRowRc *pRowRc, UINT32 uiPicCnt, BOOL bIsPFrm, UINT8  ucInitQp, UINT32 uiSVCId, H26XCOMN_CTX *pComnCtx)
{
	if (0 == pRowRc->uiPredBits) {
		save_to_reg(&pRegSet->RRC_CFG[0], 0, 1, 1);
		save_to_reg(&pRegSet->RRC_CFG[0], 0, 0, 1);
	}
	else {
		if (uiPicCnt > 1) {
			save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->stCfg.bEnable, 1, 1);	// disable rrc at anytime, but enable while encode picture frame > 1 //
	    }
	    else {
	        save_to_reg(&pRegSet->RRC_CFG[0], 0, 1, 1);
	    }
		save_to_reg(&pRegSet->RRC_CFG[0], 1, 0, 1);
	}

	save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->stCfg.ucPredWt[bIsPFrm], 4, 4);
	save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->stCfg.ucScale,    8, 3);
	save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->stCfg.ucQPRange[bIsPFrm],  12, 4);
	save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->stCfg.ucQPStep[bIsPFrm],   16, 4);
	save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->ucMinCostTh,      20, 4); // non-release at api //
#if H26X_SYNC_ROWRC_MAXMIN_QP
	if (g_RRCSyncRCQPCond) {
		if (bIsPFrm) {
			save_to_reg(&pRegSet->RRC_CFG[1], pComnCtx->stRc.m_minPQp, 8, 6);
			save_to_reg(&pRegSet->RRC_CFG[1], pComnCtx->stRc.m_maxPQp, 16, 6);
		}
		else {
			save_to_reg(&pRegSet->RRC_CFG[1], pComnCtx->stRc.m_minIQp, 8, 6);
			save_to_reg(&pRegSet->RRC_CFG[1], pComnCtx->stRc.m_maxIQp, 16, 6);
		}
	}
	else
#endif
	{
		save_to_reg(&pRegSet->RRC_CFG[1], pRowRc->stCfg.ucMinQP[bIsPFrm], 8, 6);
		save_to_reg(&pRegSet->RRC_CFG[1], pRowRc->stCfg.ucMaxQP[bIsPFrm], 16, 6);
	}
	save_to_reg(&pRegSet->RRC_CFG[1], 0x1FF, 22, 9);    //  row_level_rate_control_index_start

	save_to_reg(&pRegSet->RRC_CFG[1], ucInitQp,                 0, 6);
	save_to_reg(&pRegSet->RRC_CFG[2], pRowRc->uiPlannedStop,    0, 32);
	save_to_reg(&pRegSet->RRC_CFG[3], pRowRc->uiPlannedTop,     0, 32);
	save_to_reg(&pRegSet->RRC_CFG[4], pRowRc->uiPlannedBot,     0, 32);

    //#if SUPPORT_SVC_RC
    #if RRC_BY_FRAME_LEVEL
	save_to_reg(&pRegSet->RRC_CFG[5], pRowRc->usRefFrmCoeff[0][pRowRc->ucFrameLevel], 0, 16);
	save_to_reg(&pRegSet->RRC_CFG[5], pRowRc->usRefFrmCoeff[2][pRowRc->ucFrameLevel], 16, 16);
	save_to_reg(&pRegSet->RRC_CFG[10], pRowRc->usRefFrmCoeff[1][pRowRc->ucFrameLevel], 0, 16);
	save_to_reg(&pRegSet->RRC_CFG[10], pRowRc->usRefFrmCoeff[3][pRowRc->ucFrameLevel], 16, 16);
	save_to_reg(&pRegSet->RRC_REF_FRM_COEFF4, pRowRc->usRefFrmCoeff[4][pRowRc->ucFrameLevel], 0, 16);
	save_to_reg(&pRegSet->RRC_CFG[6], pRowRc->uiFrmCostLsb[pRowRc->ucFrameLevel],  0, 32);
	save_to_reg(&pRegSet->RRC_CFG[7], pRowRc->uiFrmCostMsb[pRowRc->ucFrameLevel],  0, 32);
	save_to_reg(&pRegSet->RRC_CFG[8], pRowRc->uiFrmCmpxLsb[pRowRc->ucFrameLevel],  0, 32);
	save_to_reg(&pRegSet->RRC_CFG[9], pRowRc->uiFrmCmpxMsb[pRowRc->ucFrameLevel],  0, 32);
    #else
	save_to_reg(&pRegSet->RRC_CFG[5], pRowRc->usRefFrmCoeff[0][bIsPFrm], 0, 16);
	save_to_reg(&pRegSet->RRC_CFG[5], pRowRc->usRefFrmCoeff[2][bIsPFrm], 16, 16);
	save_to_reg(&pRegSet->RRC_CFG[10], pRowRc->usRefFrmCoeff[1][bIsPFrm], 0, 16);
	save_to_reg(&pRegSet->RRC_CFG[10], pRowRc->usRefFrmCoeff[3][bIsPFrm], 16, 16);
	save_to_reg(&pRegSet->RRC_REF_FRM_COEFF4, pRowRc->usRefFrmCoeff[4][bIsPFrm], 0, 16);
	save_to_reg(&pRegSet->RRC_CFG[6], pRowRc->uiFrmCostLsb[bIsPFrm],  0, 32);
	save_to_reg(&pRegSet->RRC_CFG[7], pRowRc->uiFrmCostMsb[bIsPFrm],  0, 32);
	save_to_reg(&pRegSet->RRC_CFG[8], pRowRc->uiFrmCmpxLsb[bIsPFrm],  0, 32);
	save_to_reg(&pRegSet->RRC_CFG[9], pRowRc->uiFrmCmpxMsb[bIsPFrm],  0, 32);
    #endif
	if (0 == pRowRc->ucNDQPStep || 0 == pRowRc->ucNDQPRange) {
		save_to_reg(&pRegSet->DEC1_CFG, pRowRc->ucNDQPStep, 0, 4);
		save_to_reg(&pRegSet->DEC1_CFG, pRowRc->ucNDQPRange, 8, 4);
		save_to_reg(&pRegSet->DEC1_CFG, 0, 16, 1);
	}
	else {
		save_to_reg(&pRegSet->DEC1_CFG, pRowRc->ucNDQPStep, 0, 4);
		save_to_reg(&pRegSet->DEC1_CFG, pRowRc->ucNDQPRange, 8, 4);
		save_to_reg(&pRegSet->DEC1_CFG, 1, 16, 1);
	}

	if (h26x_getChipId() == 98528) {
		save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->ucZeroBitMod, 24, 3);
		save_to_reg(&pRegSet->RRC_CFG[0], pRowRc->bRRCMode, 27, 1);

		save_to_reg(&pRegSet->RRC_CFG_11[0], pRowRc->iTargetBitsScale, 0, 20);
		save_to_reg(&pRegSet->RRC_CFG_11[1], pRowRc->iTH_M, 0, 31);
		save_to_reg(&pRegSet->RRC_CFG_11[2], pRowRc->iTH_0, 0, 28);

		save_to_reg(&pRegSet->RRC_CFG_11[3], pRowRc->iTH_1, 0, 7);
		save_to_reg(&pRegSet->RRC_CFG_11[3], pRowRc->iTH_2, 7, 8);
		save_to_reg(&pRegSet->RRC_CFG_11[3], pRowRc->iTH_3, 15, 7);
		save_to_reg(&pRegSet->RRC_CFG_11[3], pRowRc->iTH_4, 22, 9);

		save_to_reg(&pRegSet->RRC_CFG_11[4], pRowRc->m_ModU_Frm[bIsPFrm], 0, 8);
		save_to_reg(&pRegSet->RRC_CFG_11[4], pRowRc->m_ModD_Frm[bIsPFrm], 8, 7);

		save_to_reg(&pRegSet->RRC_CFG_11[5], pRowRc->bTileQPRst, 0, 1);
		save_to_reg(&pRegSet->RRC_CFG_11[5], pRowRc->ucTileQPStep[bIsPFrm], 1, 6);
	}

}

void h26xEnc_wrapAqCfg(H26XRegSet *pRegSet, H26XEncAq *pAq)
{
	int i;

	save_to_reg(&pRegSet->SRAQ_CFG[0], pAq->stCfg.bEnable,   0,  1);
	save_to_reg(&pRegSet->SRAQ_CFG[0], pAq->stCfg.ucIC2,     4,  8);
	save_to_reg(&pRegSet->SRAQ_CFG[0], pAq->stCfg.cMaxDQp,  16,  5);
	save_to_reg(&pRegSet->SRAQ_CFG[0], pAq->stCfg.cMinDQp,  24,  5);

	save_to_reg(&pRegSet->SRAQ_CFG[1], pAq->stCfg.ucDepth,    8,  2);
	save_to_reg(&pRegSet->SRAQ_CFG[1], pAq->stCfg.ucPlaneX,  12,  2);
	save_to_reg(&pRegSet->SRAQ_CFG[1], pAq->stCfg.ucPlaneY,  16,  2);

	for (i = 0; i < H26X_AQ_TH_TBL_SIZE/2; i++)
	{
		save_to_reg(&pRegSet->SRAQ_CFG[i+2], pAq->stCfg.sTh[i*2],    0, 10);
		save_to_reg(&pRegSet->SRAQ_CFG[i+2], pAq->stCfg.sTh[i*2+1], 16, 10);
	}

	if (h26x_getChipId() == 98528) {
		save_to_reg(&pRegSet->SRAQ_CFG_17, pAq->stCfg.bAqMode,   0,  1);
	}
}

void h26xEnc_wrapLpmCfg(H26XRegSet *pRegSet, H26XEncLpm *pLpm)
{
	if (pLpm->stCfg.bEnable == TRUE)
	{
		save_to_reg(&pRegSet->LPM_CFG, pLpm->stCfg.ucRmdSadEn,   0,  2);
		save_to_reg(&pRegSet->LPM_CFG, pLpm->stCfg.ucIMEStopEn,  2,  2);
		save_to_reg(&pRegSet->LPM_CFG, pLpm->stCfg.ucIMEStopTh,  4,  8);
		save_to_reg(&pRegSet->LPM_CFG, pLpm->stCfg.ucRdoStopEn, 12,  2);
		save_to_reg(&pRegSet->LPM_CFG, pLpm->stCfg.ucRdoStopTh, 14,  8);
		save_to_reg(&pRegSet->LPM_CFG, pLpm->stCfg.ucChrmDmEn,  22,  2);
	}
	else
	{
		save_to_reg(&pRegSet->LPM_CFG, 0,  0,  2);
		save_to_reg(&pRegSet->LPM_CFG, 0,  2,  2);
		//save_to_reg(&pRegSet->LPM_CFG, pLpm->stCfg.ucIMEStopTh,  4,  8);
		save_to_reg(&pRegSet->LPM_CFG, 0, 12,  2);
		//save_to_reg(&pRegSet->LPM_CFG, pLpm->stCfg.ucRdoStopTh, 14,  8);
		save_to_reg(&pRegSet->LPM_CFG, 0, 22,  2);
	}
}

void h26xEnc_wrapRndCfg(H26XRegSet *pRegSet, h26XEncRnd *pRnd)
{
	save_to_reg(&pRegSet->RND_CFG[0], pRnd->stCfg.uiSeed,   0,  32);

	save_to_reg(&pRegSet->RND_CFG[1], pRnd->stCfg.bEnable,  0,  1);
	save_to_reg(&pRegSet->RND_CFG[1], pRnd->stCfg.ucRange,  1,  4);
}

void h26xEnc_wrapScdCfg(H26XRegSet *pRegSet, H26XEncScd *pScd)
{
	save_to_reg(&pRegSet->SCD_CFG, pScd->stCfg.usTh,   0, 14);
	save_to_reg(&pRegSet->SCD_CFG, pScd->stCfg.ucSc,  16,  2);
	save_to_reg(&pRegSet->SCD_CFG, pScd->stCfg.bStop, 20,  1);
	save_to_reg(&pRegSet->SCD_CFG, pScd->stCfg.ucOverrideRowRC, 24,  2);
}

void h26xEnc_wrapOsgRgbCfg(H26XRegSet *pRegSet, H26XEncOsgRgbCfg *pOsgRgb)
{
	unsigned int i;

	for (i = 0; i < 3; i++)
	{
		save_to_reg(&pRegSet->OSG_CFG[0x488/4+i], pOsgRgb->ucRgb2Yuv[i][0],  0, 8);
		save_to_reg(&pRegSet->OSG_CFG[0x488/4+i], pOsgRgb->ucRgb2Yuv[i][1],  8, 8);
		save_to_reg(&pRegSet->OSG_CFG[0x488/4+i], pOsgRgb->ucRgb2Yuv[i][2], 16, 8);
	}
}
void h26xEnc_wrapOsgPalCfg(H26XRegSet *pRegSet, UINT8 ucIdx, H26XEncOsgPalCfg *pOsgPal)
{
	UINT32 offset = 0x494/4  + ucIdx * 1;
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgPal->ucBlue,          	0, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgPal->ucGreen,          8, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgPal->ucRed,           16, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgPal->ucAlpha,         24, 8);
}

void h26xEnc_wrapOsgWinCfg(H26XRegSet *pRegSet, UINT8 ucIdx, H26XEncOsgWinCfg *pOsgWin)
{
	UINT32 offset = ucIdx * 9;

	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->bEnable,          0, 1);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stKey.bEnable,    1, 1);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.ucMode,    2, 2);
	save_to_reg(&pRegSet->OSG_CFG[offset], 	 pOsgWin->stGrap.ucType,    4, 3);
	save_to_reg(&pRegSet->OSG_CFG[offset], 	 pOsgWin->stKey.bAlphaEn,   7, 1);
	save_to_reg(&pRegSet->OSG_CFG[offset], 	 pOsgWin->stQpmap.cQpVal,  	8, 6);
	save_to_reg(&pRegSet->OSG_CFG[offset],	 pOsgWin->stQpmap.ucQpMode,	   14, 2);
	save_to_reg(&pRegSet->OSG_CFG[offset], 	 pOsgWin->stQpmap.ucFroMode,   16, 2);
	save_to_reg(&pRegSet->OSG_CFG[offset], 	 pOsgWin->stQpmap.ucTnrMode,   18, 2);
	save_to_reg(&pRegSet->OSG_CFG[offset++], pOsgWin->stQpmap.ucLpmMode,   20, 1);

	if ((pOsgWin->bEnable == TRUE) && (pOsgWin->stDisp.ucMode == 0))
		save_to_reg(&pRegSet->OSG_CFG[offset++], h26x_getPhyAddr(pOsgWin->stGrap.uiAddr), 0, 32);
	else
		save_to_reg(&pRegSet->OSG_CFG[offset++], 0, 0, 32);

	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGrap.usWidth,   0, 13);
	save_to_reg(&pRegSet->OSG_CFG[offset++], pOsgWin->stGrap.usHeight, 16, 13);

	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.usXStr,    0, 13);
	save_to_reg(&pRegSet->OSG_CFG[offset++], pOsgWin->stDisp.usYStr,   16, 13);

	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.ucBgAlpha,      0, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.ucFgAlpha,      8, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.ucMaskType,    16, 1);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.ucMaskBdSize,  17, 2);
	save_to_reg(&pRegSet->OSG_CFG[offset++], pOsgWin->stGrap.usLofs, 		19, 13);

	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.ucMaskCb,      0, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.ucMaskY[0],    8, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stDisp.ucMaskCr,     16, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset++], pOsgWin->stDisp.ucMaskY[1],   24, 8);

	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucBlkWidth,    0, 7);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucBlkHeight,   8, 7);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucBlkHNum,    16, 7);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucOrgColorLv, 24, 3);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucInvColorLv, 28, 3);
	save_to_reg(&pRegSet->OSG_CFG[offset++], pOsgWin->stGcac.bEnable,      31, 1);

	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucNorDiffTh,     0, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucInvDiffTh,     8, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucStaOnlyMode,  16, 1);
	save_to_reg(&pRegSet->OSG_CFG[offset],   pOsgWin->stGcac.ucFillEvalMode, 17, 1);
	save_to_reg(&pRegSet->OSG_CFG[offset++], pOsgWin->stGcac.ucEvalLumTarg, 24, 8);

	save_to_reg(&pRegSet->OSG_CFG[offset],	 pOsgWin->stKey.ucBlue,	  		0, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],	 pOsgWin->stKey.ucGreen,	  	8, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],	 pOsgWin->stKey.ucRed,	  	   16, 8);
	save_to_reg(&pRegSet->OSG_CFG[offset],	 pOsgWin->stKey.ucAlpha,	   24, 8);

}

void h26xEnc_wrapMotAqCfg(H26XRegSet *pRegSet, H26XEncMotAq *pMAq)
{
	save_to_reg(&pRegSet->MAQ_CFG[0], pMAq->stCfg.ucMode,		  		  0, 2);
	save_to_reg(&pRegSet->MAQ_CFG[0], pMAq->stCfg.uc8x8to16x16Th, 		  2, 2);
	save_to_reg(&pRegSet->MAQ_CFG[0], pMAq->stCfg.ucDqpRoiTh,	  		  4, 2);
	save_to_reg(&pRegSet->MAQ_CFG[0], ((UINT8)pMAq->stCfg.cDqp[0])&0xff, 16, 6);
	if (pMAq->stMAQDiffInfoCfg.enable == FALSE) {
	save_to_reg(&pRegSet->MAQ_CFG[0], ((UINT8)pMAq->stCfg.cDqp[1])&0xff, 24, 6);
	}

	save_to_reg(&pRegSet->MAQ_CFG[1], ((UINT8)pMAq->stCfg.cDqp[2])&0xff,  0, 6);
	save_to_reg(&pRegSet->MAQ_CFG[1], ((UINT8)pMAq->stCfg.cDqp[3])&0xff,  8, 6);
	save_to_reg(&pRegSet->MAQ_CFG[1], ((UINT8)pMAq->stCfg.cDqp[4])&0xff, 16, 6);
	save_to_reg(&pRegSet->MAQ_CFG[1], ((UINT8)pMAq->stCfg.cDqp[5])&0xff, 24, 6);
}

INT32 h26xEnc_wrapMotAddrCfg(H26XRegSet *pRegSet, H26XEncMotAddrCfg *pMotAddr)
{
	UINT32 i;

	//fix cim
	if (pMotAddr->ucMotBufNum > 3) {
		DBG_ERR("wrong motion buffer number:%d\r\n", (int)pMotAddr->ucMotBufNum);
		return H26XENC_FAIL;
	} else {
		save_to_reg(&pRegSet->QP_MAP_LINE_OFFSET, pMotAddr->uiMotLineOffset/32, 16, 16);
		save_to_reg(&pRegSet->MAQ_CFG[0], pMotAddr->ucMotBufNum, 6, 2);

		for (i = 0; i < pMotAddr->ucMotBufNum; i++)
			save_to_reg(&pRegSet->MOTION_BIT_ADDR[i], h26x_getPhyAddr(pMotAddr->uiMotAddr[i]), 0, 32);

		for (; i < 3; i++)
			save_to_reg(&pRegSet->MOTION_BIT_ADDR[i], 0, 0, 32);
		return H26XENC_SUCCESS;
	}
}

void h26xEnc_wrapJndCfg(H26XRegSet *pRegSet, H26XEncJnd *pJnd)
{
	save_to_reg(&pRegSet->JND_CFG, pJnd->stCfg.bEnable,	 0, 1);
	save_to_reg(&pRegSet->JND_CFG, pJnd->stCfg.ucStr,	 8, 4);
	save_to_reg(&pRegSet->JND_CFG, pJnd->stCfg.ucLevel,	12, 4);
	save_to_reg(&pRegSet->JND_CFG, pJnd->stCfg.ucTh,	16, 8);
	if (h26x_getChipId() == 98528) {
		save_to_reg(&pRegSet->JND_CFG_1, pJnd->stCfg.ucCStr,	 0, 4);
		save_to_reg(&pRegSet->JND_CFG_1, pJnd->stCfg.ucR5Flag,   4, 1);
		save_to_reg(&pRegSet->JND_CFG_1, 1, 5, 1);
		save_to_reg(&pRegSet->JND_CFG_1, pJnd->stCfg.ucLsigmaTh, 6, 8);
		save_to_reg(&pRegSet->JND_CFG_1, pJnd->stCfg.ucLsigma,  14, 8);
	}
}

void h26xEnc_wrapQpRelatedCfg(H26XRegSet *pRegSet, H26XEncQpRelatedCfg *pQpCfg)
{
    save_to_reg(&pRegSet->FUNC_CFG[1], pQpCfg->ucSaveDeltaQp,             8, 4);
}
// NT98528 //
#if 1
void h26xEnc_wrapBgrCfg(H26XRegSet *pRegSet, H26XEncBgr *pBgr)
{
	save_to_reg(&pRegSet->BGR_CFG[0], (pBgr->stCfg.bEnable != 0), 0, 1);
	save_to_reg(&pRegSet->BGR_CFG[0], pBgr->stCfg.bgr_typ,   1, 2);
	save_to_reg(&pRegSet->BGR_CFG[0], pBgr->stCfg.bgr_dth[0],  4, 4);
	save_to_reg(&pRegSet->BGR_CFG[0], pBgr->stCfg.bgr_dth[1],  8, 4);

	save_to_reg(&pRegSet->BGR_CFG[1], pBgr->stCfg.bgr_th[0],   0, 16);
	save_to_reg(&pRegSet->BGR_CFG[1], pBgr->stCfg.bgr_th[1],   16, 16);

	save_to_reg(&pRegSet->BGR_CFG[2], pBgr->stCfg.bgr_qp[0],   0, 6);
	save_to_reg(&pRegSet->BGR_CFG[2], pBgr->stCfg.bgr_qp[1],   6, 6);
	save_to_reg(&pRegSet->BGR_CFG[2], pBgr->stCfg.bgr_vt[0],   12, 6);
	save_to_reg(&pRegSet->BGR_CFG[2], pBgr->stCfg.bgr_vt[1],   18, 6);
	save_to_reg(&pRegSet->BGR_CFG[2], pBgr->stCfg.bgr_dq[0],   24, 4);
	save_to_reg(&pRegSet->BGR_CFG[2], pBgr->stCfg.bgr_dq[1],   28, 4);

	save_to_reg(&pRegSet->BGR_CFG[3], pBgr->stCfg.bgr_bth[0],   0, 16);
	save_to_reg(&pRegSet->BGR_CFG[3], pBgr->stCfg.bgr_bth[1],   16, 16);
}

void h26xEnc_wrapRmdCfg(H26XRegSet *pRegSet, H26XEncRmd *pIraMod)
{
	save_to_reg(&pRegSet->RMD_1, pIraMod->stCfg.ucIraModeBit0,	 0, 5);
	save_to_reg(&pRegSet->RMD_1, pIraMod->stCfg.ucIraModeBit1,	 5, 5);
	save_to_reg(&pRegSet->RMD_1, pIraMod->stCfg.ucIraModeBit2,	 10, 5);
	save_to_reg(&pRegSet->RMD_1, pIraMod->stCfg.ucIraModeBit3,	 15, 5);
	save_to_reg(&pRegSet->RMD_1, pIraMod->stCfg.ucIraModeBit4,	 20, 5);
	save_to_reg(&pRegSet->RMD_1, pIraMod->stCfg.ucIraModeBit5,	 25, 5);
}

void h26xEnc_wrapTnrCfg(H26XRegSet *pRegSet, H26XEncTnr *pTnr)
{
	save_to_reg(&pRegSet->TNR_CFG[0], pTnr->stCfg.nr_3d_mode,               0, 1);
	save_to_reg(&pRegSet->TNR_CFG[0], pTnr->stCfg.mctf_p2p_pixel_blending,  4, 1);
	save_to_reg(&pRegSet->TNR_CFG[0], pTnr->stCfg.tnr_p2p_input,            5, 1);
	save_to_reg(&pRegSet->TNR_CFG[0], pTnr->stCfg.tnr_p2p_sad_mode,         8, 2);
	save_to_reg(&pRegSet->TNR_CFG[0], pTnr->stCfg.tnr_p2p_input_weight,    10, 2);
	save_to_reg(&pRegSet->TNR_CFG[0], pTnr->stCfg.tnr_mctf_sad_mode,       12, 2);
	save_to_reg(&pRegSet->TNR_CFG[0], pTnr->stCfg.tnr_mctf_bias_mode,      16, 2);
	save_to_reg(&pRegSet->TNR_CFG[0], pTnr->stCfg.ref_p2p_mctf_motion_th,  20, 8);

	save_to_reg(&pRegSet->TNR_CFG[1], pTnr->stCfg.nr3d_temporal_spatial_y[0],  0, 3);
	save_to_reg(&pRegSet->TNR_CFG[1], pTnr->stCfg.nr3d_temporal_spatial_y[1],  4, 3);
	save_to_reg(&pRegSet->TNR_CFG[1], pTnr->stCfg.nr3d_temporal_spatial_y[2],  8, 3);
	save_to_reg(&pRegSet->TNR_CFG[1], pTnr->stCfg.nr3d_temporal_spatial_c[0],  16, 3);
	save_to_reg(&pRegSet->TNR_CFG[1], pTnr->stCfg.nr3d_temporal_spatial_c[1],  20, 3);
	save_to_reg(&pRegSet->TNR_CFG[1], pTnr->stCfg.nr3d_temporal_spatial_c[2],  24, 3);

	save_to_reg(&pRegSet->TNR_CFG[2], pTnr->stCfg.nr_3d_adp_th_p2p[0],          0, 8);
	save_to_reg(&pRegSet->TNR_CFG[2], pTnr->stCfg.nr_3d_adp_th_p2p[1],          8, 8);
	save_to_reg(&pRegSet->TNR_CFG[2], pTnr->stCfg.nr_3d_adp_th_p2p[2],         16, 8);

	save_to_reg(&pRegSet->TNR_CFG[3], pTnr->stCfg.nr_3d_adp_weight_p2p[0],      0, 5);
	save_to_reg(&pRegSet->TNR_CFG[3], pTnr->stCfg.nr_3d_adp_weight_p2p[1],      8, 5);
	save_to_reg(&pRegSet->TNR_CFG[3], pTnr->stCfg.nr_3d_adp_weight_p2p[2],     16, 5);

	save_to_reg(&pRegSet->TNR_CFG[4], pTnr->stCfg.cur_p2p_mctf_motion_th,       0, 8);
	save_to_reg(&pRegSet->TNR_CFG[4], pTnr->stCfg.tnr_p2p_mctf_motion_wt[0],    8, 2);
	save_to_reg(&pRegSet->TNR_CFG[4], pTnr->stCfg.tnr_p2p_mctf_motion_wt[1],   12, 2);
	save_to_reg(&pRegSet->TNR_CFG[4], pTnr->stCfg.tnr_p2p_mctf_motion_wt[2],   16, 2);
	save_to_reg(&pRegSet->TNR_CFG[4], pTnr->stCfg.tnr_p2p_mctf_motion_wt[3],   20, 2);

	save_to_reg(&pRegSet->TNR_CFG[5], pTnr->stCfg.nr3d_temporal_range_y[0],     0, 8);
	save_to_reg(&pRegSet->TNR_CFG[5], pTnr->stCfg.nr3d_temporal_range_y[1],     8, 8);
	save_to_reg(&pRegSet->TNR_CFG[5], pTnr->stCfg.nr3d_temporal_range_y[2],    16, 8);

	save_to_reg(&pRegSet->TNR_CFG[6], pTnr->stCfg.nr3d_temporal_range_c[0],     0, 8);
	save_to_reg(&pRegSet->TNR_CFG[6], pTnr->stCfg.nr3d_temporal_range_c[1],     8, 8);
	save_to_reg(&pRegSet->TNR_CFG[6], pTnr->stCfg.nr3d_temporal_range_c[2],    16, 8);

	save_to_reg(&pRegSet->TNR_CFG[7], pTnr->stCfg.nr3d_temporal_range_y_mctf[0],    0, 8);
	save_to_reg(&pRegSet->TNR_CFG[7], pTnr->stCfg.nr3d_temporal_range_y_mctf[1],    8, 8);
	save_to_reg(&pRegSet->TNR_CFG[7], pTnr->stCfg.nr3d_temporal_range_y_mctf[2],   16, 8);

	save_to_reg(&pRegSet->TNR_CFG[8], pTnr->stCfg.nr3d_temporal_range_c_mctf[0],    0, 8);
	save_to_reg(&pRegSet->TNR_CFG[8], pTnr->stCfg.nr3d_temporal_range_c_mctf[1],    8, 8);
	save_to_reg(&pRegSet->TNR_CFG[8], pTnr->stCfg.nr3d_temporal_range_c_mctf[2],   16, 8);

	save_to_reg(&pRegSet->TNR_CFG[9], pTnr->stCfg.nr3d_temporal_spatial_y_mctf[0],  0, 3);
	save_to_reg(&pRegSet->TNR_CFG[9], pTnr->stCfg.nr3d_temporal_spatial_y_mctf[1],  4, 3);
	save_to_reg(&pRegSet->TNR_CFG[9], pTnr->stCfg.nr3d_temporal_spatial_y_mctf[2],  8, 3);
	save_to_reg(&pRegSet->TNR_CFG[9], pTnr->stCfg.nr3d_temporal_spatial_c_mctf[0],  16, 3);
	save_to_reg(&pRegSet->TNR_CFG[9], pTnr->stCfg.nr3d_temporal_spatial_c_mctf[1],  20, 3);
	save_to_reg(&pRegSet->TNR_CFG[9], pTnr->stCfg.nr3d_temporal_spatial_c_mctf[2],  24, 3);

	save_to_reg(&pRegSet->TNR_CFG[10], pTnr->stCfg.nr3d_clampy_th,               0, 8);
	save_to_reg(&pRegSet->TNR_CFG[10], pTnr->stCfg.nr3d_clampy_div,              8, 3);
	save_to_reg(&pRegSet->TNR_CFG[10], pTnr->stCfg.nr3d_clampc_th,              16, 8);
	save_to_reg(&pRegSet->TNR_CFG[10], pTnr->stCfg.nr3d_clampc_div,             24, 3);

	save_to_reg(&pRegSet->TNR_CFG[11], pTnr->stCfg.tnr_p2p_border_check_sc,         0, 3);
	save_to_reg(&pRegSet->TNR_CFG[11], pTnr->stCfg.tnr_p2p_border_check_th,         8, 8);

	save_to_reg(&pRegSet->TNR_CFG[12], pTnr->stCfg.nr3d_clampy_th_mctf,          0, 8);
	save_to_reg(&pRegSet->TNR_CFG[12], pTnr->stCfg.nr3d_clampy_div_mctf,         8, 3);
	save_to_reg(&pRegSet->TNR_CFG[12], pTnr->stCfg.nr3d_clampc_th_mctf,          16, 8);
	save_to_reg(&pRegSet->TNR_CFG[12], pTnr->stCfg.nr3d_clampc_div_mctf,         24, 3);

	save_to_reg(&pRegSet->TNR_CFG[13], pTnr->stCfg.cur_motion_sad_th,             0, 8);
	save_to_reg(&pRegSet->TNR_CFG[13], pTnr->stCfg.cur_motion_rat_th,             8, 4);

	save_to_reg(&pRegSet->TNR_CFG[14], pTnr->stCfg.cur_motion_twr_p2p_th[0],     0, 8);
	save_to_reg(&pRegSet->TNR_CFG[14], pTnr->stCfg.cur_motion_twr_p2p_th[1],     8, 8);
	save_to_reg(&pRegSet->TNR_CFG[14], pTnr->stCfg.ref_motion_twr_p2p_th[0],     16, 8);
	save_to_reg(&pRegSet->TNR_CFG[14], pTnr->stCfg.ref_motion_twr_p2p_th[1],     24, 8);

	save_to_reg(&pRegSet->TNR_CFG[15], pTnr->stCfg.cur_motion_twr_mctf_th[0],     0, 8);
	save_to_reg(&pRegSet->TNR_CFG[15], pTnr->stCfg.cur_motion_twr_mctf_th[1],     8, 8);
	save_to_reg(&pRegSet->TNR_CFG[15], pTnr->stCfg.ref_motion_twr_mctf_th[0],     16, 8);
	save_to_reg(&pRegSet->TNR_CFG[15], pTnr->stCfg.ref_motion_twr_mctf_th[1],     24, 8);

	save_to_reg(&pRegSet->TNR_CFG[16], pTnr->stCfg.nr3d_temporal_spatial_y_1[0],  0, 3);
	save_to_reg(&pRegSet->TNR_CFG[16], pTnr->stCfg.nr3d_temporal_spatial_y_1[1],  3, 3);
	save_to_reg(&pRegSet->TNR_CFG[16], pTnr->stCfg.nr3d_temporal_spatial_y_1[2],  6, 3);
	save_to_reg(&pRegSet->TNR_CFG[16], pTnr->stCfg.nr3d_temporal_spatial_c_1[0],  9, 3);
	save_to_reg(&pRegSet->TNR_CFG[16], pTnr->stCfg.nr3d_temporal_spatial_c_1[1],  12, 3);
	save_to_reg(&pRegSet->TNR_CFG[16], pTnr->stCfg.nr3d_temporal_spatial_c_1[2],  15, 3);


	save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[0],  0, 3);
	save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[1],  3, 3);
	save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[2],  6, 3);
	save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[0],  9, 3);
	save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[1],  12, 3);
	save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[2],  15, 3);

	save_to_reg(&pRegSet->TNR_CFG[18], pTnr->stCfg.sad_twr_p2p_th[0],               0, 8);
	save_to_reg(&pRegSet->TNR_CFG[18], pTnr->stCfg.sad_twr_p2p_th[1],               8, 8);
	save_to_reg(&pRegSet->TNR_CFG[18], pTnr->stCfg.sad_twr_mctf_th[0],             16, 8);
	save_to_reg(&pRegSet->TNR_CFG[18], pTnr->stCfg.sad_twr_mctf_th[1],             24, 8);
}

void h26xEnc_wrapTnrUpdate(H26XRegSet *pRegSet, H26XEncTnr *pTnr, BOOL keyframe)
{
	if (0 == pTnr->stCfg.nr_3d_mode)
		return;

	if (keyframe) {
		save_to_reg(&pRegSet->TNR_CFG[17], 0,  0, 3);	// pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[0]
		save_to_reg(&pRegSet->TNR_CFG[17], 7,  3, 3);	// pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[1]
		save_to_reg(&pRegSet->TNR_CFG[17], 0,  6, 3);	// pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[2]

		save_to_reg(&pRegSet->TNR_CFG[17], 0,  9, 3);	// pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[0]
		save_to_reg(&pRegSet->TNR_CFG[17], 7,  12, 3);	// pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[1]
		save_to_reg(&pRegSet->TNR_CFG[17], 0,  15, 3);	// pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[2]

		save_to_reg(&pRegSet->TNR_CFG[18], 4,			   16, 8);	// pTnr->stCfg.sad_twr_mctf_th[0]
		save_to_reg(&pRegSet->TNR_CFG[18], 4,			   24, 8);	// pTnr->stCfg.sad_twr_mctf_th[1]
	}
	else {
		save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[0],  0, 3);
		save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[1],  3, 3);
		save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_y_mctf_1[2],  6, 3);

		save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[0],  9, 3);
		save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[1],  12, 3);
		save_to_reg(&pRegSet->TNR_CFG[17], pTnr->stCfg.nr3d_temporal_spatial_c_mctf_1[2],  15, 3);

		save_to_reg(&pRegSet->TNR_CFG[18], pTnr->stCfg.sad_twr_mctf_th[0],			   16, 8);
		save_to_reg(&pRegSet->TNR_CFG[18], pTnr->stCfg.sad_twr_mctf_th[1],			   24, 8);
	}
}

void h26xEnc_wrapLambdaCfg(H26XRegSet *pRegSet, H26XEncLambda *pLambda)
{
	int i;

	for(i=0; i<26; i++) {
		save_to_reg(&pRegSet->LMTBL_CFG[i], pLambda->stCfg.lambda_table[2*i],   0, 16);
		save_to_reg(&pRegSet->LMTBL_CFG[i], pLambda->stCfg.lambda_table[2*i+1],  16, 16);
	}

	for(i=0; i<17; i++) {
		save_to_reg(&pRegSet->SLMTBL_CFG[i], pLambda->stCfg.sqrt_lambda_table[3*i],   0, 10);
		save_to_reg(&pRegSet->SLMTBL_CFG[i], pLambda->stCfg.sqrt_lambda_table[3*i+1],  10, 10);
		save_to_reg(&pRegSet->SLMTBL_CFG[i], pLambda->stCfg.sqrt_lambda_table[3*i+2],  20, 10);
	}
	save_to_reg(&pRegSet->SLMTBL_CFG[17], pLambda->stCfg.sqrt_lambda_table[51],   0, 10);
	save_to_reg(&pRegSet->SLMTBL_CFG[0], pLambda->stCfg.adaptlambda_en,           31, 1);
}

void h26xEnc_wrapMAQDiffCfg(H26XRegSet *pRegSet, H26XEncMAQDiffInfoCfg *pMAQDiffCfg)
{
	if (pMAQDiffCfg->enable == TRUE) {
		save_to_reg(&pRegSet->MAQ_CFG[0], ((UINT8)pMAQDiffCfg->str)&0xff, 24, 6);
	} else {
		save_to_reg(&pRegSet->MAQ_CFG[0], 0, 24, 6);
	}
}

#else
void h26xEnc_wrapBgrCfg(H26XRegSet *pRegSet, H26XEncBgr *pBgr){}
void h26xEnc_wrapRmdCfg(H26XRegSet *pRegSet, H26XEncRmd *pIraMod){}
void h26xEnc_wrapTnrCfg(H26XRegSet *pRegSet, H26XEncTnr *pTnr){}
void h26xEnc_wrapLambdaCfg(H26XRegSet *pRegSet, H26XEncLambda *pLambda){}
#endif
