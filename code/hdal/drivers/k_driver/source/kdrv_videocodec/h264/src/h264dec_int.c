/*
    H264ENC module driver for NT96660

    use SeqCfg and PicCfg to setup H264RegSet

    @file       h264dec_int.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "h26x_common.h"
#include "h264dec_int.h"

//! Initialize H264 Decode Register Set
/*!
    Initialize H264 Decode Register Set and call function to init HW

    @return void
*/
void H264Dec_initRegSet(H26XRegSet *pH264RegSet, H26XDecAddr *pH264VirDecAddr){
	memset(pH264RegSet,0,sizeof(H26XRegSet));

	pH264VirDecAddr->uiRef0Idx = 0;
	pH264VirDecAddr->uiRecIdx = 1;

	pH264RegSet->COL_MVS_WR_ADDR = h26x_getPhyAddr(pH264VirDecAddr->uiColMvsAddr[0]);
	pH264RegSet->COL_MVS_RD_ADDR = h26x_getPhyAddr(pH264VirDecAddr->uiColMvsAddr[0]);
	pH264RegSet->SIDE_INFO_WR_ADDR_0 = h26x_getPhyAddr(pH264VirDecAddr->uiIlfSideInfoAddr[0]);
	pH264RegSet->SIDE_INFO_RD_ADDR_0 = h26x_getPhyAddr(pH264VirDecAddr->uiIlfSideInfoAddr[1]);

	pH264RegSet->BSDMA_CMD_BUF_ADDR = h26x_getPhyAddr(pH264VirDecAddr->uiCMDBufAddr);
	pH264RegSet->NAL_LEN_OUT_ADDR = h26x_getPhyAddr(pH264VirDecAddr->uiDummyWTAddr);

	// Set HW init Register //
	save_to_reg(&pH264RegSet->FUNC_CFG[0], 1,   0, 1); // codec type //
	save_to_reg(&pH264RegSet->FUNC_CFG[0], 1,   1, 1); //h26x_setDecMode
	if (h26x_getChipId() == 98528) {
		save_to_reg(&pH264RegSet->TIMEOUT_CNT_MAX, 0x70800, 0, 24);
	} else {
		save_to_reg(&pH264RegSet->TIMEOUT_CNT_MAX, 0x25800, 0, 24);
	}
}

//! Prepare H264 Decode Register Set
/*!
    Prepare H264 Decode Register Set and call function to prepare HW

    @return void
*/
void H264Dec_prepareRegSet(H26XRegSet *pH264RegSet, H26XDecAddr *pH264VirDecAddr, H264DEC_CTX *pContext)
{
    H264DecSeqCfg *pH264DecSeqCfg = &pContext->sH264DecSeqCfg;
    //H264DecPicCfg *pH264DecPicCfg = &pContext->sH264DecPicCfg;
    H264DecHdrObj *pH264DecHdrObj = &pContext->sH264DecHdrObj;

	H264DecSpsObj   *sps   = &pH264DecHdrObj->sH264DecSpsObj;
    H264DecPpsObj   *pps   = &pH264DecHdrObj->sH264DecPpsObj;
    H264DecSliceObj *slice = &pH264DecHdrObj->sH264DecSliceObj;

	save_to_reg(&pH264RegSet->SEQ_CFG[0], pH264DecSeqCfg->uiPicWidth,   0, 16);
	save_to_reg(&pH264RegSet->SEQ_CFG[0], pH264DecSeqCfg->uiPicHeight, 16, 16);
	save_to_reg(&pH264RegSet->SEQ_CFG[1], pH264DecSeqCfg->uiPicWidth,   0, 16);
	save_to_reg(&pH264RegSet->SEQ_CFG[1], SIZE_16X(pH264DecSeqCfg->uiPicHeight), 16, 16);

	save_to_reg(&pH264RegSet->REC_LINE_OFFSET, pH264DecSeqCfg->uiRecLineOffset,  0, 32);

	save_to_reg(&pH264RegSet->SIDE_INFO_LINE_OFFSET, ((pH264DecSeqCfg->uiPicWidth + 63)/64), 0, 32);

	save_to_reg(&pH264RegSet->PIC_CFG, IS_PSLICE(slice->eSliceType), 0, 1);
	save_to_reg(&pH264RegSet->PIC_CFG, 1,  4, 1);	// ilf_rec_data_output_enable //
	save_to_reg(&pH264RegSet->PIC_CFG, 1,  5, 1);	// COL_MVS_OUT_EN // hk ??
	save_to_reg(&pH264RegSet->PIC_CFG, 1,  6, 1);	// SIDINFO_OUT_EN //  hk ??
	save_to_reg(&pH264RegSet->FUNC_CFG[0], 1, 4, 1);	// ae enable //
	if (h26x_getChipId() == 98520) {
		save_to_reg(&pH264RegSet->FUNC_CFG[1], (pH264DecSeqCfg->uiPicWidth <= 2048), 3, 1);  // flexible search range enable //
	} else {
		if (pH264DecSeqCfg->uiPicWidth <= H26X_MAX_W_WITHOUT_TILE_V36_528)
			save_to_reg(&pH264RegSet->FUNC_CFG[1], 1, 22, 1);  // flexible search range enable //
		else if (pH264DecSeqCfg->uiPicWidth <= H26X_MAX_W_WITHOUT_TILE_V20_528)
			save_to_reg(&pH264RegSet->FUNC_CFG[1], 1, 3, 1);  // flexible search range enable //
		else
			save_to_reg(&pH264RegSet->FUNC_CFG[1], 0, 3, 1);  // flexible search range enable //
	}
	save_to_reg(&pH264RegSet->FUNC_CFG[1], pps->uiEntropyCoding, 28, 1);
	save_to_reg(&pH264RegSet->FUNC_CFG[1], pps->uiPicTrans8x8Mode, 29, 1);

	//QP //
	save_to_reg(&pH264RegSet->QP_CFG[0], slice->uiQP, 0, 6);
	save_to_reg(&pH264RegSet->QP_CFG[0], pps->uiChromQPIdxoffset, 8, 6);
	save_to_reg(&pH264RegSet->QP_CFG[0], pps->uiPicSecondChormaQPIdxoffset, 16, 6);

	save_to_reg(&pH264RegSet->FUNC_CFG[0], 0, 13, 1);
	save_to_reg(&pH264RegSet->FUNC_CFG[0], 0, 19, 1);

	// ilf //
	save_to_reg(&pH264RegSet->ILF_CFG[0], pps->uiDelockingFilterControlPresentFlag, 1, 1);
	save_to_reg(&pH264RegSet->ILF_CFG[0], ((slice->uiDisableDBFilterIdc & 0x1) == 0), 0, 1);
	save_to_reg(&pH264RegSet->ILF_CFG[0], (slice->uiBetaOffsetDiv2 & 0xf), 4, 4);
	save_to_reg(&pH264RegSet->ILF_CFG[0], (slice->uiAlphaC0OffsetDiv2 & 0xf), 8, 4);
	save_to_reg(&pH264RegSet->ILF_CFG[0], (slice->uiDisableDBFilterIdc == 0), 2, 1);

	// AEAD //
	save_to_reg(&pH264RegSet->AEAD_CFG, 0,  0, 1);
	save_to_reg(&pH264RegSet->AEAD_CFG, 0,  1, 1);
	save_to_reg(&pH264RegSet->AEAD_CFG, 0,  2, 1);
	save_to_reg(&pH264RegSet->ILF_CFG[0], 0, 3, 1);
	save_to_reg(&pH264RegSet->ILF_CFG[0], slice->uiCabacInitIdc, 28, 2);

	save_to_reg(&pH264RegSet->AEAD_CFG, sps->uiLog2MaxPocLsbMinus4, 28, 4);

	//save_to_reg(&pH264RegSet->DEC1_CFG, 0, 16, 2);
	save_to_reg(&pH264RegSet->DEC1_CFG, sps->uiPocType, 16, 2);

	save_to_reg(&pH264RegSet->AEAD_CFG, 0, 3, 7);
	save_to_reg(&pH264RegSet->AEAD_CFG, sps->uiLog2MaxFrmMinus4, 10, 4);
}

//! Stop Register Set
/*!
    Call function to stop HW

    @return void
*/
void H264Dec_stopRegSet(void){
    h26x_close();
}

void H264Dec_setFrmIdx(H264DecHdrObj *pH264DecHdrObj, H26XDecAddr *pH264VirDecAddr)
{
	H264DecSliceObj *pSlice = &pH264DecHdrObj->sH264DecSliceObj;

	// reference index //
	pH264VirDecAddr->uiRef0Idx = (pSlice->uiLongTermPicNum == 0) ? FRM_IDX_LT_0 : ((pSlice->uiAbsDiffPicNumL0 == 2) ? FRM_IDX_ST_1 : FRM_IDX_ST_0);
	// reconstruct index //
	pH264VirDecAddr->uiRecIdx = FRM_IDX_NON_REF;
}

void H264Dec_modifyFrmIdx(H264DecHdrObj *pH264DecHdrObj, H26XDecAddr *pH264VirDecAddr)
{
	H264DecSliceObj *pSlice = &pH264DecHdrObj->sH264DecSliceObj;
	pH264VirDecAddr->uiResYAddr = (pSlice->uiNalRefIdc) ? 0 : pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_NON_REF];
	pH264VirDecAddr->uiResYAddr2 = 0;

	if (pSlice->sMMCO.uiEnable)
	{
		if (pSlice->sMMCO.uiMMCO == 1) {
			pH264VirDecAddr->uiResYAddr = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0];
		} else {
			pH264VirDecAddr->uiResYAddr2 = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_LT_0];

			if (pSlice->sMMCO.uiSTPicNum == 0)
			{
				pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_LT_0]  = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0];
				pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_LT_0] = pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_0];
				pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0] = 0;
				pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_0] = 0;
			}
			else
			{
				pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_LT_0]  = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_1];
				pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_LT_0] = pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_1];
				pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_1] = 0;
				pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_1] = 0;
			}
		}
	}

	if (pSlice->uiNalRefIdc)
	{
		if (pSlice->bIDRSetLTFlag) {
			pH264VirDecAddr->uiResYAddr2 = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_LT_0];
			pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_LT_0]  = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_NON_REF];
			pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_LT_0]  = pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_NON_REF];
			pSlice->bIDRSetLTFlag = 0;
		}
		#if 0
		if (pSlice->sMMCO.uiEnable == 0)
			pH264VirDecAddr->uiResYAddr = (pSlice->uiAbsDiffPicNumL0 == 0) ? pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_1] : pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0];
		#else
		if (IS_ISLICE(pSlice->eSliceType)) {
			if ((pH264DecHdrObj->sH264DecSpsObj.uiNumRefFrm - pSlice->bLTEn) == 2) {
				pH264VirDecAddr->uiResYAddr = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_1];
				pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_1] = 0;
				pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_1] = 0;
			}
			else {
				pH264VirDecAddr->uiResYAddr = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0];
				pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0] = 0;
				pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_0] = 0;
			}
		}
		else
			pH264VirDecAddr->uiResYAddr = ((pH264DecHdrObj->sH264DecSpsObj.uiNumRefFrm - pSlice->bLTEn) == 1) ? pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0] : pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_1];
		#endif
		if (pH264VirDecAddr->uiResYAddr == pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_LT_0])
			pH264VirDecAddr->uiResYAddr = 0;

		pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_1]  = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0];
		pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_1] = pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_0];

		pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_ST_0]  = pH264VirDecAddr->uiRefAndRecYAddr[FRM_IDX_NON_REF];
		pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_ST_0] = pH264VirDecAddr->uiRefAndRecUVAddr[FRM_IDX_NON_REF];
	}
}
