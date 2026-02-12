/*
    H.26x wrapper operation.

    @file       h264enc_wrap.c
    @ingroup    mIDrvCodec_H26x
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "nvt_vdocdc_dbg.h"

#include "h26x.h"
#include "h26x_def.h"
#include "h26x_common.h"
#include "h26xenc_int.h"

#include "h264enc_wrap.h"

int gH264PReduce16Planar = 0;

void h264Enc_wrapSeqCfg(H264ENC_CTX *pVdoCtx, H26XEncAddr *pAddr)
{
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H26XRegSet    *pRegSet = (H26XRegSet *)pAddr->uiAPBAddr;

	//pRegSet->INT_EN = (H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT);

	save_to_reg(&pRegSet->FUNC_CFG[0], 1, 0, 1);	// codec type //
	save_to_reg(&pRegSet->FUNC_CFG[0], 0, 1, 2);	// codec mode //
	save_to_reg(&pRegSet->FUNC_CFG[0], 1, 4, 1);	// ae_en //
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bFastSearchEn, 5, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->ucRotate, 9, 2);
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bGrayEn, 11, 2);
	save_to_reg(&pRegSet->FUNC_CFG[0], 1, 15, 1); // nalu len report

	// address and lineoffset set by each picture //
	save_to_reg(&pRegSet->REC_LINE_OFFSET, pAddr->uiRecYLineOffset,   0, 16);
	save_to_reg(&pRegSet->REC_LINE_OFFSET, pAddr->uiRecCLineOffset,  16, 16);
	save_to_reg(&pRegSet->SIDE_INFO_LINE_OFFSET, pAddr->uiSideInfoLineOffset,  0, 16);

	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bHwPaddingEn, 18, 1);

	if (h26x_getChipId() == 98520) {
		save_to_reg(&pRegSet->FUNC_CFG[1], (pSeqCfg->usWidth <= 2048), 3, 1);  // flexible search range enable //
	} else {
		if (pSeqCfg->usWidth <= H26X_MAX_W_WITHOUT_TILE_V36_528)
			save_to_reg(&pRegSet->FUNC_CFG[1], 1, 22, 1);  // flexible search range enable //
		else if (pSeqCfg->usWidth <= H26X_MAX_W_WITHOUT_TILE_V20_528)
			save_to_reg(&pRegSet->FUNC_CFG[1], 1, 3, 1);  // flexible search range enable //
		else
			save_to_reg(&pRegSet->FUNC_CFG[1], 0, 3, 1);  // flexible search range enable //
	}
	save_to_reg(&pRegSet->FUNC_CFG[1], (pSeqCfg->eEntropyMode == CABAC), 28, 1);
	save_to_reg(&pRegSet->FUNC_CFG[1], pSeqCfg->bTrans8x8En, 29, 1);

	save_to_reg(&pRegSet->SEQ_CFG[0], pSeqCfg->uiDisplayWidth,	 0, 16);
	save_to_reg(&pRegSet->SEQ_CFG[0], pSeqCfg->usHeight, 16, 16);

	save_to_reg(&pRegSet->SEQ_CFG[1], pSeqCfg->usWidth,             0, 16);
	save_to_reg(&pRegSet->SEQ_CFG[1], SIZE_16X(pSeqCfg->usHeight), 16, 16);

	save_to_reg(&pRegSet->QP_CFG[0], pSeqCfg->cChrmQPIdx,     8, 6);
	save_to_reg(&pRegSet->QP_CFG[0], pSeqCfg->cSecChrmQPIdx, 16, 6);

	save_to_reg(&pRegSet->ILF_CFG[0], (pSeqCfg->ucDisLFIdc != 1), 0, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], (pSeqCfg->ucDisLFIdc == 0), 2, 1);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->cDBBeta, 4, 4);
	save_to_reg(&pRegSet->ILF_CFG[0], pSeqCfg->cDBAlpha, 8, 4);

	save_to_reg(&pRegSet->AEAD_CFG, 0,  0, 1);	// SPS delta_pic_order_always_zero_flag
	save_to_reg(&pRegSet->AEAD_CFG, 0,  1, 1);	// PPS pic_order_present_flag
	save_to_reg(&pRegSet->AEAD_CFG, 0,  2, 1);	// PPS redundant_pic_cnt_present_flag
	save_to_reg(&pRegSet->AEAD_CFG, 0,  3, 7);	// PPS init_qp_minus26
	save_to_reg(&pRegSet->AEAD_CFG, pSeqCfg->usLog2MaxFrm, 10, 4);	// SPS log2_max_frame_num_minus4
	save_to_reg(&pRegSet->AEAD_CFG, pSeqCfg->usLog2MaxPoc, 28, 4);	// SPS log2_max_pic_order_cnt_lsb_minus4

	// roi : set ROI maximum window number //
	save_to_reg(&pRegSet->ROI_CFG[0], 10, 16, 4);
	// rrc : enable row rc calualte //
	//save_to_reg(&pRegSet->RRC_CFG[0], 1, 1, 1);
	save_to_reg(&pRegSet->RRC_CFG[0], 1, 0, 1);	// calculate rrc, //
	if (h26x_getChipId() == 98528) {
		save_to_reg(&pRegSet->TIMEOUT_CNT_MAX, 0x70800, 0, 24);
	} else {
		save_to_reg(&pRegSet->TIMEOUT_CNT_MAX, 0x25800, 0, 24);
	}
}

#if VDOCDC_DUMP_PA_ADDR
static void h264Enc_dumpPhyAddr(H264ENC_INFO *pInfo, H26XENC_VAR *pVar)
{
	H264ENC_CTX   *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XCOMN_CTX  *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;
	H26XEncAddr   *pAddr    = &pComnCtx->stVaAddr;
	#if RRC_BY_FRAME_LEVEL
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XEncRowRc  *pRowRc = &pFuncCtx->stRowRc;
	#endif

	DBG_DUMP("SRC_Y_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pInfo->uiSrcYAddr));
	DBG_DUMP("SRC_C_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pInfo->uiSrcCAddr));
	DBG_DUMP("REC_Y_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRecRefY[pVdoCtx->eRecIdx]));
	DBG_DUMP("REC_C_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRecRefC[pVdoCtx->eRecIdx]));
	DBG_DUMP("REF_Y_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRecRefY[pVdoCtx->eRefIdx]));
	DBG_DUMP("REF_C_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRecRefC[pVdoCtx->eRefIdx]));

	if (pInfo->bSrcOutEn) {
		DBG_DUMP("TNR_OUT_Y_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pInfo->uiSrcOutYAddr));
		DBG_DUMP("TNR_OUT_C_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pInfo->uiSrcOutCAddr));
	}

	DBG_DUMP("COL_MVS_WR_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiColMvs[0]));
	DBG_DUMP("COL_MVS_RD_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiColMvs[0]));
	DBG_DUMP("SIDE_INFO_WR_ADDR_0 : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiSideInfo[0][pVdoCtx->eRecIdx]));
	DBG_DUMP("SIDE_INFO_RD_ADDR_0 : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiSideInfo[0][pVdoCtx->eRefIdx]));

	#if RRC_BY_FRAME_LEVEL
	DBG_DUMP("RRC_WR_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRRC[pRowRc->ucFrameLevel][0]));
	DBG_DUMP("RRC_RD_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRRC[pRowRc->ucFrameLevel][1]));
	#else
	if (IS_ISLICE(pVdoCtx->stPicCfg.ePicType)) {
		DBG_DUMP("RRC_WR_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRRC[0][0]));
		DBG_DUMP("RRC_RD_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRRC[0][1]));
	}
	else {
		DBG_DUMP("RRC_WR_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRRC[1][0]));
		DBG_DUMP("RRC_RD_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiRRC[1][1]));
	}
	#endif
	DBG_DUMP("NAL_LEN_OUT_ADDR : %08x\r\n", (int)h26x_getPhyAddr((pInfo->uiNalLenOutAddr == 0 ) ? pAddr->uiNaluLen : pInfo->uiNalLenOutAddr));
	DBG_DUMP("BSDMA_CMD_BUF_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pAddr->uiBsdma));
	DBG_DUMP("BSOUT_BUF_ADDR : %08x\r\n", (int)h26x_getPhyAddr(pInfo->uiBsOutBufAddr));
}
#endif

void h264Enc_wrapPicCfg(H26XRegSet *pRegSet, H264ENC_INFO *pInfo, H26XENC_VAR *pVar)
{
	H264ENC_CTX   *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H26XCOMN_CTX  *pComnCtx = (H26XCOMN_CTX *)pVar->pComnCtx;

	H264EncSeqCfg *pSeqCfg  = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg  = &pVdoCtx->stPicCfg;
	H264EncRdo    *pRdo	= &pVdoCtx->stRdo;
	H26XEncAddr   *pAddr    = &pComnCtx->stVaAddr;
	#if RRC_BY_FRAME_LEVEL
	H26XEncRowRc  *pRowRc = &pFuncCtx->stRowRc;
	#endif

	#if VDOCDC_DUMP_PA_ADDR
	h264Enc_dumpPhyAddr(pInfo, pVar);
	#endif

	// address and lineoffset set by each picture //
	save_to_reg(&pRegSet->SRC_Y_ADDR, h26x_getPhyAddr(pInfo->uiSrcYAddr), 0, 32);
	save_to_reg(&pRegSet->SRC_C_ADDR, h26x_getPhyAddr(pInfo->uiSrcCAddr), 0, 32);
	save_to_reg(&pRegSet->REC_Y_ADDR, h26x_getPhyAddr(pAddr->uiRecRefY[pVdoCtx->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->REC_C_ADDR, h26x_getPhyAddr(pAddr->uiRecRefC[pVdoCtx->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->REF_Y_ADDR, h26x_getPhyAddr(pAddr->uiRecRefY[pVdoCtx->eRefIdx]), 0, 32);
	save_to_reg(&pRegSet->REF_C_ADDR, h26x_getPhyAddr(pAddr->uiRecRefC[pVdoCtx->eRefIdx]), 0, 32);

	if (pInfo->bSrcOutEn)
	{
		save_to_reg(&pRegSet->TNR_OUT_Y_ADDR, h26x_getPhyAddr(pInfo->uiSrcOutYAddr), 0, 32);
		save_to_reg(&pRegSet->TNR_OUT_C_ADDR, h26x_getPhyAddr(pInfo->uiSrcOutCAddr), 0, 32);

		save_to_reg(&pRegSet->TNR_OUT_LINE_OFFSET, pInfo->uiSrcOutYLineOffset/4,  0, 16);
		save_to_reg(&pRegSet->TNR_OUT_LINE_OFFSET, pInfo->uiSrcOutCLineOffset/4, 16, 16);
	}
	save_to_reg(&pRegSet->COL_MVS_WR_ADDR, h26x_getPhyAddr(pAddr->uiColMvs[0]), 0, 32);
	save_to_reg(&pRegSet->COL_MVS_RD_ADDR, h26x_getPhyAddr(pAddr->uiColMvs[0]), 0, 32);
	save_to_reg(&pRegSet->SIDE_INFO_WR_ADDR_0, h26x_getPhyAddr(pAddr->uiSideInfo[0][pVdoCtx->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->SIDE_INFO_RD_ADDR_0, h26x_getPhyAddr(pAddr->uiSideInfo[0][pVdoCtx->eRefIdx]), 0, 32);

	save_to_reg(&pRegSet->FUNC_CFG[0], pPicCfg->bFBCEn[pVdoCtx->eRecIdx], 13, 1); //REC_FBC_EN
	save_to_reg(&pRegSet->FUNC_CFG[0], (pComnCtx->uiLastHwInt & H26X_FINISH_INT) ? pPicCfg->bFBCEn[pVdoCtx->eRefIdx] : 0, 19, 1); //REF_FBC_EN
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->bSrcD2DEn,    20, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->ucSrcD2DMode, 21, 1);

	#if RRC_BY_FRAME_LEVEL
	save_to_reg(&pRegSet->RRC_WR_ADDR, h26x_getPhyAddr(pAddr->uiRRC[pRowRc->ucFrameLevel][0]), 0, 32);
	save_to_reg(&pRegSet->RRC_RD_ADDR, h26x_getPhyAddr(pAddr->uiRRC[pRowRc->ucFrameLevel][1]), 0, 32);
	if (pFuncCtx->stRowRc.uiPredBits) {
		H26X_SWAP(pAddr->uiRRC[pRowRc->ucFrameLevel][0], pAddr->uiRRC[pRowRc->ucFrameLevel][1], UINT32);
	}
	#else
	if (IS_ISLICE(pPicCfg->ePicType))
	{
		save_to_reg(&pRegSet->RRC_WR_ADDR, h26x_getPhyAddr(pAddr->uiRRC[0][0]), 0, 32);
		save_to_reg(&pRegSet->RRC_RD_ADDR, h26x_getPhyAddr(pAddr->uiRRC[0][1]), 0, 32);
		H26X_SWAP(pAddr->uiRRC[0][0], pAddr->uiRRC[0][1], UINT32);
	}
	else
	{
		save_to_reg(&pRegSet->RRC_WR_ADDR, h26x_getPhyAddr(pAddr->uiRRC[1][0]), 0, 32);
		save_to_reg(&pRegSet->RRC_RD_ADDR, h26x_getPhyAddr(pAddr->uiRRC[1][1]), 0, 32);
		if (pFuncCtx->stRowRc.uiPredBits) {
			H26X_SWAP(pAddr->uiRRC[1][0], pAddr->uiRRC[1][1], UINT32);
		}
	}
	#endif
	//save_to_reg(&pRegSet->NAL_LEN_OUT_ADDR, h26x_getPhyAddr(pInfo->uiNalLenOutAddr), 0, 32);
	save_to_reg(&pRegSet->NAL_LEN_OUT_ADDR, h26x_getPhyAddr((pInfo->uiNalLenOutAddr == 0 ) ? pAddr->uiNaluLen : pInfo->uiNalLenOutAddr), 0, 32);
	save_to_reg(&pRegSet->BSDMA_CMD_BUF_ADDR, h26x_getPhyAddr(pAddr->uiBsdma), 0, 32);
	save_to_reg(&pRegSet->BSOUT_BUF_ADDR, h26x_getPhyAddr(pInfo->uiBsOutBufAddr), 0, 32);
	save_to_reg(&pRegSet->BSOUT_BUF_SIZE, pInfo->uiBsOutBufSize, 0, 32);
	save_to_reg(&pRegSet->NAL_HDR_LEN_TOTAL_LEN, pPicCfg->uiPicHdrLen, 0, 32);
	if (pInfo->SdeCfg.bEnable) {
		save_to_reg(&pRegSet->FUNC_CFG[0], 1, 2, 1); 	// bSrcYDeCmpsEn
		save_to_reg(&pRegSet->FUNC_CFG[0], 1, 3, 1);	// bSrcCDeCmpsEn
		save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->SdeCfg.uiYLofst/4,  0, 16);
		save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->SdeCfg.uiCLofst/4, 16, 16);
	}
	else {
		save_to_reg(&pRegSet->FUNC_CFG[0], 0, 2, 1); 	// bSrcYDeCmpsEn
		save_to_reg(&pRegSet->FUNC_CFG[0], 0, 3, 1);	// bSrcCDeCmpsEn
		save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->uiSrcYLineOffset/4,  0, 16);
		save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->uiSrcCLineOffset/4, 16, 16);
	}

	save_to_reg(&pRegSet->FUNC_CFG[0], (pInfo->bSkipFrmEn && IS_PSLICE(pPicCfg->eNxtPicType)), 	 6, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->ucSrcOutMode, 16, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->bSrcCbCrIv,   17, 1);

	save_to_reg(&pRegSet->PIC_CFG, IS_PSLICE(pPicCfg->ePicType), 0, 1);
	save_to_reg(&pRegSet->PIC_CFG, (pVdoCtx->eRecIdx != FRM_IDX_NON_REF), 4, 1);	// REC_OUT_EN
	if (pSeqCfg->bColEn == 0) {
			save_to_reg(&pRegSet->PIC_CFG, 0, 5, 1);	// COL_MVS_OUT_EN
	} else {
		if (pComnCtx->uiPicCnt == 0) {
			save_to_reg(&pRegSet->PIC_CFG, 0, 5, 1);	// COL_MVS_OUT_EN
		} else {
			save_to_reg(&pRegSet->PIC_CFG, (pVdoCtx->eRecIdx != FRM_IDX_NON_REF), 5, 1);	// COL_MVS_OUT_EN
		}
	}
	save_to_reg(&pRegSet->PIC_CFG, (pVdoCtx->eRecIdx != FRM_IDX_NON_REF), 6, 1);	// SIDEINFO_OUT_EN
	save_to_reg(&pRegSet->PIC_CFG, pInfo->bSrcOutEn, 7, 1);
	save_to_reg(&pRegSet->PIC_CFG, (pVdoCtx->eRecIdx == pVdoCtx->eRefIdx), 8, 1);

	save_to_reg(&pRegSet->QP_CFG[0], pPicCfg->ucSliceQP, 0, 6);

	if (IS_ISLICE(pPicCfg->ePicType))
	{
		save_to_reg(&pRegSet->QP_CFG[1], pComnCtx->stRc.m_minIQp, 0, 6);
		save_to_reg(&pRegSet->QP_CFG[1], pComnCtx->stRc.m_maxIQp, 8, 6);
	}
	else
	{
		save_to_reg(&pRegSet->QP_CFG[1], pComnCtx->stRc.m_minPQp, 0, 6);
		save_to_reg(&pRegSet->QP_CFG[1], pComnCtx->stRc.m_maxPQp, 8, 6);
	}

	if (IS_ISLICE(pPicCfg->ePicType))
	{
		save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[0][0],  4, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[0][1],  8, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[0][2], 12, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[0][3], 16, 4);
	}
	else
	{
		save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[1][0],  4, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[1][1],  8, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[1][2], 12, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[1][3], 16, 4);
	}

#if H264_P_REDUCE_16_PLANAR
    if (gH264PReduce16Planar && IS_PSLICE(pPicCfg->ePicType)) {
        save_to_reg(&pRegSet->RDO_CFG[1], 0/*pRdoCfg->ucI16_CT_DC*/,   24, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], 0/*pRdoCfg->ucI16_CT_H*/,    26, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], 3/*pRdoCfg->ucI16_CT_P*/,    28, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], 0/*pRdoCfg->ucI16_CT_V*/,    30, 2);
    }
    else {
        save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_DC,   24, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_H,    26, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_P,    28, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_V,    30, 2);
    }
#else
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_DC,   24, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_H,    26, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_P,    28, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_V,    30, 2);
#endif

	if (pFuncCtx->stAq.stCfg.bEnable == TRUE)
	{
		save_to_reg(&pRegSet->SRAQ_CFG[0], (IS_ISLICE(pPicCfg->ePicType)) ? pFuncCtx->stAq.stCfg.ucIStr : pFuncCtx->stAq.stCfg.ucPStr, 12, 4);
		save_to_reg(&pRegSet->SRAQ_CFG[1], pFuncCtx->stAq.stCfg.ucAslog2, 0, 6);
		if (h26x_getChipId() == 98528) {
			save_to_reg(&pRegSet->SRAQ_CFG_17, (IS_ISLICE(pPicCfg->ePicType)) ? pFuncCtx->stAq.stCfg.ucIStr1 : pFuncCtx->stAq.stCfg.ucPStr1, 4, 4);
			save_to_reg(&pRegSet->SRAQ_CFG_17, (IS_ISLICE(pPicCfg->ePicType)) ? pFuncCtx->stAq.stCfg.ucIStr2 : pFuncCtx->stAq.stCfg.ucPStr2, 8, 4);
		}
	}

	if (pFuncCtx->stLpm.stCfg.bEnable == TRUE)
	{
		save_to_reg(&pRegSet->LPM_CFG, (IS_ISLICE(pPicCfg->ePicType)) ? 0 : pFuncCtx->stLpm.stCfg.ucRdoStopEn, 12, 2);
	}

	save_to_reg(&pRegSet->OSG_CFG[0x484/4], h26x_getPhyAddr(pFuncCtx->stOsg.uiGcacStatAddr0), 0, 32);
	save_to_reg(&pRegSet->OSG_CFG[0x480/4], h26x_getPhyAddr(pFuncCtx->stOsg.uiGcacStatAddr1), 0, 32);
	if (h26x_getChipId() == 98528)
		save_to_reg(&pRegSet->OSG_CFG[0x494/4  + 16 * 1], 1, 1, 1);
}

void h264Enc_wrapRdoCfg(H26XRegSet *pRegSet, H264ENC_CTX *pVdoCtx)
{
	H264EncRdo *pRdo = &pVdoCtx->stRdo;

	save_to_reg(&pRegSet->RDO_CFG[0], 1, 0, 1);	// rdo enable //

	save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[2][0], 20, 4);
	save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[2][1], 24, 4);
	save_to_reg(&pRegSet->RDO_CFG[0], pRdo->ucSlope[2][3], 28, 4);

	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->stRdoCfg.ucI_Y4_CB_P,      0, 5);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->stRdoCfg.ucI_Y8_CB_P,      8, 5);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->stRdoCfg.ucI_Y16_CB_P,    16, 5);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_DC,   24, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_H,    26, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_P,    28, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI16_CT_V,    30, 2);

	save_to_reg(&pRegSet->RDO_CFG[2], pRdo->stRdoCfg.ucP_Y4_CB,      0, 5);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdo->stRdoCfg.ucP_Y8_CB,      8, 5);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdo->ucIP_C_CT_DC,  24, 2);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdo->ucIP_C_CT_H,   26, 2);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdo->ucIP_C_CT_P,   28, 2);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdo->ucIP_C_CT_V,   30, 2);

	save_to_reg(&pRegSet->RDO_CFG[3], pRdo->stRdoCfg.ucP_Y_COEFF_COST_TH,	0, 8);
	save_to_reg(&pRegSet->RDO_CFG[3], pRdo->stRdoCfg.ucP_C_COEFF_COST_TH,	8, 8);
	save_to_reg(&pRegSet->RDO_CFG[3], pRdo->stRdoCfg.ucIP_CB_SKIP,	16, 5);
}

void h264Enc_wrapRdoCfgUpdate(H26XRegSet *pRegSet, H264EncRdo *pRdo, BOOL bIsIfrm)
{
	if (pRdo->stRdoCfg.bEnable) {
		if (bIsIfrm) {
			save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI_Y4_CB,      0, 5);
			save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI_Y8_CB,      8, 5);
			save_to_reg(&pRegSet->RDO_CFG[1], pRdo->ucI_Y16_CB,    16, 5);
		} else {
			save_to_reg(&pRegSet->RDO_CFG[1], pRdo->stRdoCfg.ucI_Y4_CB_P,      0, 5);
			save_to_reg(&pRegSet->RDO_CFG[1], pRdo->stRdoCfg.ucI_Y8_CB_P,      8, 5);
			save_to_reg(&pRegSet->RDO_CFG[1], pRdo->stRdoCfg.ucI_Y16_CB_P,    16, 5);
		}

		save_to_reg(&pRegSet->RDO_CFG[2], pRdo->stRdoCfg.ucP_Y4_CB,      0, 5);
		save_to_reg(&pRegSet->RDO_CFG[2], pRdo->stRdoCfg.ucP_Y8_CB,      8, 5);

		save_to_reg(&pRegSet->RDO_CFG[3], pRdo->stRdoCfg.ucIP_CB_SKIP,	16, 5);
		save_to_reg(&pRegSet->RDO_CFG[3], pRdo->stRdoCfg.ucP_Y_COEFF_COST_TH,	0, 8);
		save_to_reg(&pRegSet->RDO_CFG[3], pRdo->stRdoCfg.ucP_C_COEFF_COST_TH,	8, 8);
	}
}

void h264Enc_wrapFroCfg(H26XRegSet *pRegSet, H264ENC_CTX *pVdoCtx)
{
	H264EncFroCfg *pFroCfg = &pVdoCtx->stFroCfg;

	save_to_reg(&pRegSet->FUNC_CFG[0], pFroCfg->bEnable, 7, 1); // fro enable //

	save_to_reg(&pRegSet->FRO_CFG[0], pFroCfg->uiDC[0][1],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[0], pFroCfg->ucST[0][1], 12, 5);
	save_to_reg(&pRegSet->FRO_CFG[0], pFroCfg->ucMX[0][1], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[1], pFroCfg->uiDC[0][0],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[1], pFroCfg->ucST[0][0], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[1], pFroCfg->ucMX[0][0], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[2], pFroCfg->uiDC[1][1],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[2], pFroCfg->ucST[1][1], 12, 5);
	save_to_reg(&pRegSet->FRO_CFG[2], pFroCfg->ucMX[1][1], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[3], pFroCfg->uiDC[1][0],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[3], pFroCfg->ucST[1][0], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[3], pFroCfg->ucMX[1][0], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[4], pFroCfg->uiDC[0][2],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[4], pFroCfg->ucST[0][2], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[4], pFroCfg->ucMX[0][2], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[5], pFroCfg->uiDC[0][3],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[5], pFroCfg->ucST[0][3], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[5], pFroCfg->ucMX[0][3], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[6], pFroCfg->uiDC[1][2],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[6], pFroCfg->ucST[1][2], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[6], pFroCfg->ucMX[1][2], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[7], pFroCfg->uiDC[1][3],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[7], pFroCfg->ucST[1][3], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[7], pFroCfg->ucMX[1][3], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[8], pFroCfg->uiDC[2][2],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[8], pFroCfg->ucST[2][2], 12, 4);
	save_to_reg(&pRegSet->FRO_CFG[8], pFroCfg->ucMX[2][2], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[9], pFroCfg->uiDC[2][0],  0, 9);
	save_to_reg(&pRegSet->FRO_CFG[9], pFroCfg->ucST[2][0], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[9], pFroCfg->ucMX[2][0], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[10], pFroCfg->uiDC[2][1],	 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[10], pFroCfg->ucST[2][1], 12, 5);
	save_to_reg(&pRegSet->FRO_CFG[10], pFroCfg->ucMX[2][1], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[11], pFroCfg->uiDC[2][3],	 0, 9);
	save_to_reg(&pRegSet->FRO_CFG[11], pFroCfg->ucST[2][3], 12, 6);
	save_to_reg(&pRegSet->FRO_CFG[11], pFroCfg->ucMX[2][3], 20, 4);

	save_to_reg(&pRegSet->FRO_CFG[12], pFroCfg->ucAC[0][1],	 0, 8);
	save_to_reg(&pRegSet->FRO_CFG[12], pFroCfg->ucAC[0][0],	 8, 8);
	save_to_reg(&pRegSet->FRO_CFG[12], pFroCfg->ucAC[1][1],	16, 8);
	save_to_reg(&pRegSet->FRO_CFG[12], pFroCfg->ucAC[1][0],	24, 8);

	save_to_reg(&pRegSet->FRO_CFG[13], pFroCfg->ucAC[0][2],	 0, 8);
	save_to_reg(&pRegSet->FRO_CFG[13], pFroCfg->ucAC[0][3],	 8, 8);
	save_to_reg(&pRegSet->FRO_CFG[13], pFroCfg->ucAC[1][2],	16, 8);
	save_to_reg(&pRegSet->FRO_CFG[13], pFroCfg->ucAC[1][3],	24, 8);

	save_to_reg(&pRegSet->FRO_CFG[14], pFroCfg->ucAC[2][2],	 0, 8);
	save_to_reg(&pRegSet->FRO_CFG[14], pFroCfg->ucAC[2][0],	 8, 8);
	save_to_reg(&pRegSet->FRO_CFG[14], pFroCfg->ucAC[2][1],	16, 8);
	save_to_reg(&pRegSet->FRO_CFG[14], pFroCfg->ucAC[2][3],	24, 8);
}

void h264Enc_wrapSPFrmCfg(H264ENC_CTX *pVdoCtx, H26XEncAddr *pAddr)
{
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H26XRegSet    *pRegSet = (H26XRegSet *)pAddr->uiAPBAddr;

	save_to_reg(&pRegSet->PIC_CFG, 1, 0, 1);// set pic type as p frame
	save_to_reg(&pRegSet->BSOUT_BUF_ADDR, h26x_getPhyAddr(pAddr->uiSPFrmBsOutAddr), 0, 32);
	save_to_reg(&pRegSet->BSOUT_BUF_SIZE, ((pSeqCfg->usWidth*pSeqCfg->usHeight)>>7)<<7, 0, 32);
}

void h264Enc_wrapSIFrmCfg(H264ENC_CTX *pVdoCtx, H26XEncAddr *pAddr, UINT32 bs_buf_size)
{
	H26XRegSet    *pRegSet = (H26XRegSet *)pAddr->uiAPBAddr;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	save_to_reg(&pRegSet->SEQ_CFG[0], H264E_GDRI_MIN_SLICE_HEIGHT, 16, 16);
	save_to_reg(&pRegSet->SEQ_CFG[1], H264E_GDRI_MIN_SLICE_HEIGHT, 16, 16);

	save_to_reg(&pRegSet->BSOUT_BUF_ADDR, h26x_getPhyAddr(pAddr->uiBsOutAddr), 0, 32);
	save_to_reg(&pRegSet->BSOUT_BUF_SIZE, bs_buf_size, 0, 32);
	save_to_reg(&pRegSet->NAL_HDR_LEN_TOTAL_LEN, pPicCfg->uiPicHdrLen, 0, 32);
	save_to_reg(&pRegSet->NAL_LEN_OUT_ADDR, h26x_getPhyAddr(pAddr->uiSIFrmNaluAddr), 0, 32);
	save_to_reg(&pRegSet->PIC_CFG, 0, 0, 1); // set pic type as i frm
	save_to_reg(&pRegSet->PIC_CFG, 0, 16, 16);// set slice row num as 0
	//save_to_reg(&pRegSet->PIC_CFG, 0, 5, 1); // set colmv off
}

void h264Enc_wrapFirstPFrmCfg(H264ENC_CTX *pVdoCtx, H26XEncAddr *pAddr, UINT32 bs_buf_size)
{
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H26XRegSet    *pRegSet = (H26XRegSet *)pAddr->uiAPBAddr;

	save_to_reg(&pRegSet->SEQ_CFG[0], pSeqCfg->usHeight, 16, 16);
	save_to_reg(&pRegSet->SEQ_CFG[1], SIZE_16X(pSeqCfg->usHeight), 16, 16);
	save_to_reg(&pRegSet->BSOUT_BUF_SIZE, bs_buf_size, 0, 32);
}
