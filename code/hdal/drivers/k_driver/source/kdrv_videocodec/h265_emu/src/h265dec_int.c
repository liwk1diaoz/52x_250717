/*
    H265ENC module driver for NT96660

    use SeqCfg and PicCfg to setup H265RegSet

    @file       h265dec_int.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "kdrv_vdocdc_dbg.h"

#include "h265dec_int.h"

//! Initialize H265 Decode Register Set
/*!
    Initialize H265 Decode Register Set and call function to init HW

    @return void
*/
void H265Dec_initRegSet(H26XRegSet *pH265RegSet, H26XDecAddr *pH265VirDecAddr)
{
	pH265VirDecAddr->uiRef0Idx = 0;
	pH265VirDecAddr->uiRecIdx = -1;

	pH265RegSet->COL_MVS_WR_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiColMvsAddr[0]);
	pH265RegSet->COL_MVS_RD_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiColMvsAddr[0]);
	//pH265RegSet->SIDE_INFO_WR_ADDR_0 = h26x_getPhyAddr(pH265VirDecAddr->uiIlfSideInfoAddr[0]);
	//pH265RegSet->SIDE_INFO_RD_ADDR_0 = h26x_getPhyAddr(pH265VirDecAddr->uiIlfSideInfoAddr[1]);

	pH265RegSet->BSDMA_CMD_BUF_ADDR = h26x_getPhyAddr(pH265VirDecAddr->uiCMDBufAddr);

	// Set HW init Register //
	save_to_reg(&pH265RegSet->FUNC_CFG[0], 0,   0, 1); // codec type //
	save_to_reg(&pH265RegSet->FUNC_CFG[0], 1,   1, 1); //h26x_setDecMode 
	save_to_reg(&pH265RegSet->TIMEOUT_CNT_MAX, 0x25800, 0, 24);
}

//! Prepare H265 Decode Register Set
/*!
    Prepare H265 Decode Register Set and call function to prepare HW

    @return void
*/
void H265Dec_prepareRegSet(H26XRegSet *pH265RegSet, H26XDecAddr *pH265VirDecAddr, H265DEC_CTX *pContext)
{
	H265DecSeqCfg *pH265DecSeqCfg = &pContext->sH265DecSeqCfg;
	H265DecHdrObj *pH265DecHdrObj = &pContext->sH265DecHdrObj;

	H265DecSpsObj   *sps   = &pH265DecHdrObj->sH265DecSpsObj;
	H265DecPpsObj   *pps   = &pH265DecHdrObj->sH265DecPpsObj;
	H265DecSliceObj *slice = &pH265DecHdrObj->sH265DecSliceObj;
	STRPS *rps = &sps->short_term_ref_pic_set[0]; //Notice:  assume num_short_term_ref_pic_sets = 1

	save_to_reg(&pH265RegSet->SEQ_CFG[0], pH265DecSeqCfg->uiPicWidth,   0, 16);
	save_to_reg(&pH265RegSet->SEQ_CFG[0], SIZE_16X(pH265DecSeqCfg->uiPicHeight), 16, 16);
	save_to_reg(&pH265RegSet->SEQ_CFG[1], pH265DecSeqCfg->uiPicWidth,   0, 16);
	save_to_reg(&pH265RegSet->SEQ_CFG[1], SIZE_16X(pH265DecSeqCfg->uiPicHeight), 16, 16);

	save_to_reg(&pH265RegSet->REC_LINE_OFFSET, pH265DecSeqCfg->uiRecLineOffset,  0, 32);
	//save_to_reg(&pH265RegSet->SIDE_INFO_LINE_OFFSET, (SIZE_32X((((pH265DecSeqCfg->uiPicWidth + 63) >> 6) << 3))) >> 2, 0, 16);

	save_to_reg(&pH265RegSet->PIC_CFG, IS_PSLICE(slice->slice_type), 0, 1);
	save_to_reg(&pH265RegSet->PIC_CFG, 1,  4, 1);  // ilf_rec_data_output_enable //
	save_to_reg(&pH265RegSet->PIC_CFG, 1,  5, 1);	// COL_MVS_OUT_EN // hk ??
	//save_to_reg(&pH265RegSet->PIC_CFG, 1,  6, 1);	// SIDINFO_OUT_EN //  hk ??
	save_to_reg(&pH265RegSet->FUNC_CFG[0], 1, 4, 1);  // ae enable //
	save_to_reg(&pH265RegSet->FUNC_CFG[1], sps->sps_temporal_mvp_enabled_flag,  0, 1); // TEMPORAL_MVP_ENABLE //

    // 1. search range: 0 (luma: +-28, chroma: +-16
    //      max frame width: 6656 for tile mode, 1536 for non-tile mode
    // 2. search range: 1 (luma: +-36, chroma: +-20 for tile mode)
    //                    (luma: +-20, chroma: +-12 for non-tile mode)
    //      max frame width: 4736 for tile mode, 2048 for non-tile mode
    if (pps->tiles_enabled_flag) {
        if (pH265DecSeqCfg->uiPicWidth <= 4736)
            save_to_reg(&pH265RegSet->FUNC_CFG[1], 0, 3, 1);  // flexible search range enable //
        else
            save_to_reg(&pH265RegSet->FUNC_CFG[1], 1, 3, 1);  // flexible search range enable //
    } else {
        if (pH265DecSeqCfg->uiPicWidth > 1536)
            save_to_reg(&pH265RegSet->FUNC_CFG[1], 1, 3, 1);  // flexible search range enable //
        else
            save_to_reg(&pH265RegSet->FUNC_CFG[1], 0, 3, 1);  // flexible search range enable //
    }

    // QP //
	save_to_reg(&pH265RegSet->QP_CFG[0], slice->slice_qp, 0, 6);
	save_to_reg(&pH265RegSet->QP_CFG[0], pps->pps_cb_qp_offset, 8, 6);
	save_to_reg(&pH265RegSet->QP_CFG[0], pps->pps_cr_qp_offset, 16, 6);
    #if 1
	save_to_reg(&pH265RegSet->QP_CFG[0], 0, 24, 1);
    #else
	save_to_reg(&pH265RegSet->QP_CFG[0], pps->chroma_qp_offset_list_enabled_flag, 24, 1);
    #endif

	// ilf //
	save_to_reg(&pH265RegSet->ILF_CFG[0], !slice->slice_deblocking_filter_disabled_flag, 0, 1);
	save_to_reg(&pH265RegSet->ILF_CFG[0], pps->deblocking_filter_control_present_flag, 1, 1);
	save_to_reg(&pH265RegSet->ILF_CFG[0], pps->pps_loop_filter_across_slices_enabled_flag, 2, 1);
	save_to_reg(&pH265RegSet->ILF_CFG[0], pps->deblocking_filter_override_enabled_flag, 3, 1);
	save_to_reg(&pH265RegSet->ILF_CFG[0], (slice->slice_beta_offset_div2 & 0xf), 4, 4);
	save_to_reg(&pH265RegSet->ILF_CFG[0], (slice->slice_tc_offset_div2 & 0xf), 8, 4);
    save_to_reg(&pH265RegSet->ILF_CFG[0], pps->loop_filter_across_tiles_enabled_flag, 12, 1);
    save_to_reg(&pH265RegSet->ILF_CFG[0], sps->sample_adaptive_offset_enabled_flag, 16, 1);
    save_to_reg(&pH265RegSet->ILF_CFG[0], slice->slice_sao_luma_flag, 17, 1);
    save_to_reg(&pH265RegSet->ILF_CFG[0], slice->slice_sao_chroma_flag, 18, 1);

	// AEAD //
	save_to_reg(&pH265RegSet->AEAD_CFG, pps->cabac_init_present_flag,  0, 1);
	save_to_reg(&pH265RegSet->AEAD_CFG, pps->lists_modification_present_flag,  1, 1);
	save_to_reg(&pH265RegSet->AEAD_CFG, sps->long_term_ref_pics_present_flag,  2, 1);
	save_to_reg(&pH265RegSet->AEAD_CFG, pps->init_qp_minus26,  3, 7);
	save_to_reg(&pH265RegSet->AEAD_CFG, slice->pic_len_ref_list,  10, 4);
	save_to_reg(&pH265RegSet->AEAD_CFG, sps->pic_len_slice_addr,  19, 4);
	save_to_reg(&pH265RegSet->AEAD_CFG, sps->Log2MinCuQpDeltaSize,  24, 2);
	save_to_reg(&pH265RegSet->AEAD_CFG, pps->cu_qp_delta_enabled_flag,  26, 1);
	save_to_reg(&pH265RegSet->AEAD_CFG, sps->log2_max_pic_order_cnt_lsb_minus4,  28, 3);

	save_to_reg(&pH265RegSet->DEC0_CFG, slice->num_long_term_ref_pics, 0, 6);
	save_to_reg(&pH265RegSet->DEC0_CFG, sps->num_short_term_ref_pic_sets, 8, 7);
	save_to_reg(&pH265RegSet->DEC0_CFG, sps->num_long_term_ref_pics_sps, 16, 7);

	save_to_reg(&pH265RegSet->DEC1_CFG, rps->NumNegativePics, 0, 5);
    #if 1
	save_to_reg(&pH265RegSet->DEC1_CFG, 0, 8, 5);
    #else
	save_to_reg(&pH265RegSet->DEC1_CFG, rps->NumPositivePics, 8, 5);
    #endif
	save_to_reg(&pH265RegSet->DEC1_CFG, rps->curr_num_delta_pocs, 16, 5);
	save_to_reg(&pH265RegSet->DEC1_CFG, sps->num_long_term_ref_pics_sps, 24, 6);
	if (rps->curr_num_delta_pocs != 0) {
		DBG_ERR("NumDeltaPocs Set Error");   //hw: 0
	}

	// distScaleFactor //
	save_to_reg(&pH265RegSet->DSF_CFG, slice->discalefactor, 0, 13);
	//if (pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRecIdx] != pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRef0Idx]) {
	//	save_to_reg(&pH265RegSet->DSF_CFG, 0, 16, 1);
	//	save_to_reg(&pH265RegSet->DSF_CFG, 0, 17, 1);
	//} else 
	{
		save_to_reg(&pH265RegSet->DSF_CFG, pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRecIdx], 16, 1);
		save_to_reg(&pH265RegSet->DSF_CFG, pH265VirDecAddr->uiRefAndRecIsLT[pH265VirDecAddr->uiRef0Idx], 17, 1);
	}

    // tile cfg //
    if (pps->tiles_enabled_flag) {
        save_to_reg(&pH265RegSet->TILE_CFG[0], pps->num_tile_columns_minus1, 0, 3);
        save_to_reg(&pH265RegSet->TILE_CFG[1], pH265DecSeqCfg->uiTileWidth[0] | (pH265DecSeqCfg->uiTileWidth[1] << 16), 0, 32);
        save_to_reg(&pH265RegSet->TILE_CFG[2], pH265DecSeqCfg->uiTileWidth[2] | (pH265DecSeqCfg->uiTileWidth[3] << 16), 0, 32);
    }
}

//! Stop Register Set
/*!
    Call function to stop HW

    @return void
*/
void H265Dec_stopRegSet(void)
{
	h26x_close();
}
