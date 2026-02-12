/*
    H.26x wrapper operation.

    @file       h264enc_wrap.c
    @ingroup    mIDrvCodec_H26x
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "h26x.h"
#include "h26x_def.h"
#include "h26x_common.h"
#include "h26xenc_int.h"

#include "h264enc_wrap.h"

int gH264PReduce16Planar = 0;

void h264Enc_wrapSeqCfg(H26XRegSet *pRegSet, H264ENC_CTX *pVdoCtx)
{
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H26XEncAddr   *pAddr   = &pVdoCtx->stAddr;

	//pRegSet->INT_EN = (H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT);

	save_to_reg(&pRegSet->FUNC_CFG[0], 1, 0, 1);	// codec type //
	save_to_reg(&pRegSet->FUNC_CFG[0], 0, 1, 2);	// codec mode //
	save_to_reg(&pRegSet->FUNC_CFG[0], 1, 4, 1);	// ae_en //
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bFastSearchEn, 5, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->ucRotate, 9, 2);
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bGrayEn, 11, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bFBCEn, 13, 1);	//REC_FBC_EN
	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bFBCEn, 19, 1); //REF_FBC_EN

	// address and lineoffset set by each picture //
	save_to_reg(&pRegSet->REC_LINE_OFFSET, pAddr->uiRecYLineOffset,   0, 16);
	save_to_reg(&pRegSet->REC_LINE_OFFSET, pAddr->uiRecCLineOffset,  16, 16);
	save_to_reg(&pRegSet->SIDE_INFO_LINE_OFFSET, pAddr->uiSideInfoLineOffset,  0, 16);

	save_to_reg(&pRegSet->FUNC_CFG[0], pSeqCfg->bHwPaddingEn, 18, 1);

	save_to_reg(&pRegSet->FUNC_CFG[1], (pSeqCfg->eEntropyMode == CABAC), 28, 1);
	save_to_reg(&pRegSet->FUNC_CFG[1], pSeqCfg->bTrans8x8En, 29, 1);

	save_to_reg(&pRegSet->SEQ_CFG[0], pSeqCfg->usWidth,   0, 16);
	save_to_reg(&pRegSet->SEQ_CFG[0], pSeqCfg->usHeight, 16, 16);

	save_to_reg(&pRegSet->SEQ_CFG[1], SIZE_16X(pSeqCfg->usWidth),             0, 16);
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
	//save_to_reg(&pRegSet->RRC_CFG[0], 1, 0, 1);	// calculate rrc, //
}

void h264Enc_wrapPicCfg(H26XRegSet *pRegSet, H264ENC_INFO *pInfo, H26XENC_VAR *pVar)
{
	H264ENC_CTX   *pVdoCtx  = (H264ENC_CTX *)pVar->pVdoCtx;
	H26XFUNC_CTX  *pFuncCtx = (H26XFUNC_CTX *)pVar->pFuncCtx;
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg  = &pVdoCtx->stPicCfg;
	H264EncRdoCfg *pRdoCfg  = &pVdoCtx->stRdoCfg;
	H26XEncAddr   *pAddr    = &pVdoCtx->stAddr;
	H26XEncRcCfg  *pRcCfg   = &pVdoCtx->stRcCfg;

	// address and lineoffset set by each picture //
	save_to_reg(&pRegSet->SRC_Y_ADDR, h26x_getPhyAddr(pInfo->uiSrcYAddr), 0, 32);
	save_to_reg(&pRegSet->SRC_C_ADDR, h26x_getPhyAddr(pInfo->uiSrcCAddr), 0, 32);
	save_to_reg(&pRegSet->REC_Y_ADDR, h26x_getPhyAddr(pAddr->uiRecRefY[pAddr->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->REC_C_ADDR, h26x_getPhyAddr(pAddr->uiRecRefC[pAddr->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->REF_Y_ADDR, h26x_getPhyAddr(pAddr->uiRecRefY[pAddr->eRefIdx]), 0, 32);
	save_to_reg(&pRegSet->REF_C_ADDR, h26x_getPhyAddr(pAddr->uiRecRefC[pAddr->eRefIdx]), 0, 32);
    if(pSeqCfg->bGrayEn)
    {
        pRegSet->SRC_C_ADDR = 0;
    }
    if(pSeqCfg->bGrayEn == 1 && pSeqCfg->bGrayColorEn == 0)
    {
        pRegSet->REC_C_ADDR = 0;
        pRegSet->REF_C_ADDR = 0;
    }

	if (pInfo->bSrcOutEn)
	{
		save_to_reg(&pRegSet->TNR_OUT_Y_ADDR, h26x_getPhyAddr(pInfo->uiSrcOutYAddr), 0, 32);
		save_to_reg(&pRegSet->TNR_OUT_C_ADDR, h26x_getPhyAddr(pInfo->uiSrcOutCAddr), 0, 32);

		save_to_reg(&pRegSet->TNR_OUT_LINE_OFFSET, pInfo->uiSrcOutYLineOffset/4,  0, 16);
		save_to_reg(&pRegSet->TNR_OUT_LINE_OFFSET, pInfo->uiSrcOutCLineOffset/4, 16, 16);
	}
	save_to_reg(&pRegSet->COL_MVS_WR_ADDR, h26x_getPhyAddr(pAddr->uiColMvs[pAddr->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->COL_MVS_RD_ADDR, h26x_getPhyAddr(pAddr->uiColMvs[pAddr->eRefIdx]), 0, 32);
	save_to_reg(&pRegSet->SIDE_INFO_WR_ADDR_0, h26x_getPhyAddr(pAddr->uiSideInfo[0][pAddr->eRecIdx]), 0, 32);
	save_to_reg(&pRegSet->SIDE_INFO_RD_ADDR_0, h26x_getPhyAddr(pAddr->uiSideInfo[0][pAddr->eRefIdx]), 0, 32);

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
		H26X_SWAP(pAddr->uiRRC[1][0], pAddr->uiRRC[1][1], UINT32);
	}
	//save_to_reg(&pRegSet->NAL_LEN_OUT_ADDR, h26x_getPhyAddr(pInfo->uiNalLenOutAddr), 0, 32);
	save_to_reg(&pRegSet->NAL_LEN_OUT_ADDR, h26x_getPhyAddr(pInfo->uiNalLenOutAddr), 0, 32);
	save_to_reg(&pRegSet->BSDMA_CMD_BUF_ADDR, h26x_getPhyAddr(pAddr->uiBsdma), 0, 32);
	save_to_reg(&pRegSet->BSOUT_BUF_ADDR, h26x_getPhyAddr(pInfo->uiBsOutBufAddr), 0, 32);
	save_to_reg(&pRegSet->BSOUT_BUF_SIZE, pInfo->uiBsOutBufSize, 0, 32);
	save_to_reg(&pRegSet->NAL_HDR_LEN_TOTAL_LEN, pPicCfg->uiNalHdrLenSum, 0, 32);
	save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->uiSrcYLineOffset/4,  0, 16);
	save_to_reg(&pRegSet->SRC_LINE_OFFSET, pInfo->uiSrcCLineOffset/4, 16, 16);

	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->bSrcYDeCmpsEn, 2, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->bSrcCDeCmpsEn, 3, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->bSkipFrmEn, 	 6, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->ucSrcOutMode, 16, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], pInfo->bSrcCbCrIv,   17, 1);

	save_to_reg(&pRegSet->PIC_CFG, IS_PSLICE(pPicCfg->ePicType), 0, 1);
	save_to_reg(&pRegSet->PIC_CFG, (pAddr->eRecIdx != FRM_IDX_NON_REF), 4, 1);	// REC_OUT_EN
	save_to_reg(&pRegSet->PIC_CFG, (pAddr->eRecIdx != FRM_IDX_NON_REF), 5, 1);	// COL_MVS_OUT_EN
	save_to_reg(&pRegSet->PIC_CFG, (pAddr->eRecIdx != FRM_IDX_NON_REF), 6, 1);	// SIDEINFO_OUT_EN
	save_to_reg(&pRegSet->PIC_CFG, pInfo->bSrcOutEn, 7, 1);
	save_to_reg(&pRegSet->PIC_CFG, (pAddr->eRecIdx == pAddr->eRefIdx), 8, 1);

	save_to_reg(&pRegSet->PIC_CFG, pSeqCfg->bGrayColorEn, 10, 1);
	save_to_reg(&pRegSet->PIC_CFG, pSeqCfg->bGrayColorEn, 11, 1);
	save_to_reg(&pRegSet->QP_CFG[0], pPicCfg->ucSliceQP, 0, 6);

	if (IS_ISLICE(pPicCfg->ePicType))
	{
		save_to_reg(&pRegSet->QP_CFG[1], pRcCfg->ucMinIQp, 0, 6);
		save_to_reg(&pRegSet->QP_CFG[1], pRcCfg->ucMaxIQp, 8, 6);
	}
	else
	{
		save_to_reg(&pRegSet->QP_CFG[1], pRcCfg->ucMinPQp, 0, 6);
		save_to_reg(&pRegSet->QP_CFG[1], pRcCfg->ucMaxPQp, 8, 6);
	}

	if (IS_ISLICE(pPicCfg->ePicType))
	{
		save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[0][0],  4, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[0][1],  8, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[0][2], 12, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[0][3], 16, 4);
	}
	else
	{
		save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[1][0],  4, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[1][1],  8, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[1][2], 12, 4);
		save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[1][3], 16, 4);
	}

#if H264_P_REDUCE_16_PLANAR
    if (gH264PReduce16Planar && IS_PSLICE(pPicCfg->ePicType)) {
        save_to_reg(&pRegSet->RDO_CFG[1], 0/*pRdoCfg->ucI16_CT_DC*/,   24, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], 0/*pRdoCfg->ucI16_CT_H*/,    26, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], 3/*pRdoCfg->ucI16_CT_P*/,    28, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], 0/*pRdoCfg->ucI16_CT_V*/,    30, 2);
    }
    else {
        save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI16_CT_DC,   24, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI16_CT_H,    26, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI16_CT_P,    28, 2);
        save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI16_CT_V,    30, 2);
    }
#endif

	//if (pFuncCtx->stAq.stCfg.bEnable == TRUE)
	{
		save_to_reg(&pRegSet->SRAQ_CFG[0], (IS_ISLICE(pPicCfg->ePicType)) ? pFuncCtx->stAq.stCfg.ucIStr : pFuncCtx->stAq.stCfg.ucPStr, 12, 4);
		save_to_reg(&pRegSet->SRAQ_CFG[1], pFuncCtx->stAq.stCfg.ucAslog2, 0, 6);
	}

	if (pFuncCtx->stLpm.stCfg.bEnable == TRUE)
	{
		save_to_reg(&pRegSet->LPM_CFG, (IS_ISLICE(pPicCfg->ePicType)) ? 0 : pFuncCtx->stLpm.stCfg.ucRdoStopEn, 12, 2);
	}

	save_to_reg(&pRegSet->OSG_CFG[0x484/4], h26x_getPhyAddr(pFuncCtx->stOsg.uiGcacStatAddr0), 0, 32);
	save_to_reg(&pRegSet->OSG_CFG[0x480/4], h26x_getPhyAddr(pFuncCtx->stOsg.uiGcacStatAddr1), 0, 32);

	if (pFuncCtx->stMAq.stCfg.ucDqpnum == 2)
	{
		H26X_SWAP(pRegSet->MOTION_BIT_ADDR[0], pRegSet->MOTION_BIT_ADDR[1], UINT32);
	}
	else if (pFuncCtx->stMAq.stCfg.ucDqpnum == 3)
	{
		H26X_SWAP(pRegSet->MOTION_BIT_ADDR[2], pRegSet->MOTION_BIT_ADDR[0], UINT32);
		H26X_SWAP(pRegSet->MOTION_BIT_ADDR[1], pRegSet->MOTION_BIT_ADDR[2], UINT32);
	}



}

void h264Enc_wrapRdoCfg(H26XRegSet *pRegSet, H264ENC_CTX *pVdoCtx)
{
	H264EncRdoCfg *pRdoCfg = &pVdoCtx->stRdoCfg;

	save_to_reg(&pRegSet->RDO_CFG[0], 1, 0, 1);	// rdo enable //

	save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[2][0], 20, 4);
	save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[2][1], 24, 4);
	save_to_reg(&pRegSet->RDO_CFG[0], pRdoCfg->ucSlope[2][3], 28, 4);

	save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI_Y4_CB,      0, 5);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI_Y8_CB,      8, 5);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI_Y16_CB,    16, 5);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI16_CT_DC,   24, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI16_CT_H,    26, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI16_CT_P,    28, 2);
	save_to_reg(&pRegSet->RDO_CFG[1], pRdoCfg->ucI16_CT_V,    30, 2);

	save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->ucP_Y4_CB,      0, 5);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->ucP_Y8_CB,      8, 5);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->ucIP_C_CT_DC,  24, 2);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->ucIP_C_CT_H,   26, 2);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->ucIP_C_CT_P,   28, 2);
	save_to_reg(&pRegSet->RDO_CFG[2], pRdoCfg->ucIP_C_CT_V,   30, 2);

	save_to_reg(&pRegSet->RDO_CFG[3], pRdoCfg->ucP_Y_COEFF_COST_TH,  0, 8);
	save_to_reg(&pRegSet->RDO_CFG[3], pRdoCfg->ucP_C_COEFF_COST_TH,  8, 8);
	save_to_reg(&pRegSet->RDO_CFG[3], pRdoCfg->ucIP_CB_SKIP,        16, 8);
}

void h264Enc_wrapRdoCfg2(H26XRegSet *pRegSet, H264ENC_CTX *pVdoCtx)
{
	H264EncRdoCfg *pRdoCfg = &pVdoCtx->stRdoCfg;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	save_to_reg(&pRegSet->RDO_CFG[0], (pRdoCfg->ucAVC_RATE_EST) & IS_ISLICE(pPicCfg->ePicType), 2, 2);	// rdo enable //

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

