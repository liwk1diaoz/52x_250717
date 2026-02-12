/*
    H265ENC module driver for NT96660

    use SeqCfg and PicCfg to setup H265RegSet

    @file       h265enc_int.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "nvt_vdocdc_dbg.h"

#include "h26x_common.h"
#include "h26x_bitstream.h"

#include "h265enc_int.h"

#define FOLLOW_K_NALU_RULE  1

#define MB_SIZE       16
#define CEILING_DIV(NUM, DIV) (((NUM)+(DIV)-1) / (DIV))
//#define abs(a)   ((a > 0)? a : -a)
UINT32 gForceLongStartCode = 0;

static INT32 calculteFrameCropping(H265EncSeqCfg *pSeqCfg, H265EncPicCfg *pPicCfg, INT32 *pCropBottom, INT32 *pCropRight)
{
	INT32 iFrameCroppingFlag, iScale;
	INT32 iHeight, iTmp;

	//*pCropRight = (pSeqCfg->usWidth - pSeqCfg->uiDisplayWidth) / 2;
	*pCropRight = (pSeqCfg->usWidth - (pSeqCfg->frm_crop_info.frm_crop_enable? pSeqCfg->frm_crop_info.display_w: pSeqCfg->uiDisplayWidth)) / 2;

	iScale = (MB_SIZE);

	iHeight = CEILING_DIV(pSeqCfg->usHeight, iScale) * iScale;

	iTmp = 2;

	//*pCropBottom = (iHeight - pSeqCfg->usHeight) / iTmp;
	*pCropBottom = (iHeight - (pSeqCfg->frm_crop_info.frm_crop_enable? pSeqCfg->frm_crop_info.display_h: pSeqCfg->usHeight)) / iTmp;

	if (*pCropRight != 0 || *pCropBottom != 0) {
		iFrameCroppingFlag = 1;
	} else {
		iFrameCroppingFlag = 0;
	}

	DBG_IND("Crop R = %d, %d, flag = %d\r\n", (int)(*pCropRight), (int)(*pCropBottom), (int)(iFrameCroppingFlag));

	return iFrameCroppingFlag;

}

//! Write Nal Header
/*!
    Write Nal Header

    @return void
*/


static UINT32 writeNalHeader(UINT8 *pucAddr, UINT32 uiNalUnitType, UINT32 uiTmeporalid)
{
	UINT32 uiIdx = 0;
	/* Prepend a NAL units with 00 00 00 01 to generate */
	/* Annex B byte stream format bitstreams.           */
	pucAddr[uiIdx++] = 0x00;
	pucAddr[uiIdx++] = 0x00;
	pucAddr[uiIdx++] = 0x00;
	pucAddr[uiIdx++] = 0x01;

	/* 1                 | 6           | 1           */
	/* forbidden_zero_bit | nal_unit_type| nuh_layer_id */
	pucAddr[uiIdx++] = ((uiNalUnitType & 0x3F) << 1);
	//DBG_IND("writeNalHeader = 0x%08x\r\n",pucAddr[uiIdx-1]);

	/* 5                 | 3 */
	/* nuh_layer_id | nuh_temporal_id_plus1*/
	pucAddr[uiIdx++] = ((uiTmeporalid + 1) & 0x7);
	//DBG_IND("writeNalHeader = 0x%08x\r\n",pucAddr[uiIdx-1]);

	return uiIdx;
}


static void write_nal_header(bstream *bs, UINT32 tmeporalid, UINT32 nal_unit_type, UINT32 id)
{
	/* 1                 | 6           | 1           */
	/* forbidden_zero_bit | nal_unit_type| nuh_layer_id */
	/* 5                 | 3 */
	/* nuh_layer_id | nuh_temporal_id_plus1*/

	if (gForceLongStartCode || id == 0) {
		put_bits(bs, 0, 8);         // suffix : 00 //
	}
	put_bits(bs, 1, 24);            // suffix : 00 00 01 //
	put_bits(bs, 0, 1);             // forbidden_zero_bit //
	put_bits(bs, nal_unit_type, 6); //nal_unit_type
	put_bits(bs, 0, 6);             //nuh_layer_id
	put_bits(bs, tmeporalid + 1, 3); //nuh_temporal_id_plus1
}

static void writeVUI(bstream *pH265Bs, H265EncSeqCfg *pSeqCfg, H265EncPicCfg *pPicCfg)
{
	UINT32 num_units_in_tick, time_scale, poc_proportional_flag = 0;
	UINT32 uiFrameRate = pSeqCfg->uiFrmRate;
	UINT32 video_signal_type_presetn_flag = 1;

	/* aspect_ratio_info_present_flag */
#if CMODEL_VERIFY
    put_bits(pH265Bs,   1,                    1);
    put_bits(pH265Bs,   0,                  8);   /* aspect_ratio_idc (Extended_SAR) */
#else
    if (pSeqCfg->usSarWidth == 0 || pSeqCfg->usSarHeight == 0) {
        put_bits(pH265Bs, 0, 1);

    } else {
        put_bits(pH265Bs,   1,                    1);
        put_bits(pH265Bs,   255,                  8);   /* aspect_ratio_idc (Extended_SAR) */
        put_bits(pH265Bs,   pSeqCfg->usSarWidth,  16);  /* sar_width                    */
        put_bits(pH265Bs,   pSeqCfg->usSarHeight,  16); /* sar_height                   */
    }
#endif
	/* overscan_info_presetn_flag */
	put_bits(pH265Bs, 0, 1);
	/* video_signal_type_presetn_flag */
	put_bits(pH265Bs, video_signal_type_presetn_flag, 1);
	if (video_signal_type_presetn_flag) {
		UINT32 colour_description_present_flag = (pSeqCfg->ucColourPrimaries == 2 && pSeqCfg->ucTransferCharacteristics == 2 && pSeqCfg->ucMatrixCoef == 2)? 0: 1;
		/* video_format : Component */
		put_bits(pH265Bs, pSeqCfg->ucVideoFormat, 3);
		/* video_full_range_flag */
		put_bits(pH265Bs, pSeqCfg->ucColorRange, 1);
		/* colour_description_present_flag */
		put_bits(pH265Bs, colour_description_present_flag, 1);
		if (colour_description_present_flag) {
			/* colour_primaries */
			put_bits(pH265Bs, pSeqCfg->ucColourPrimaries, 8);
			/* transfer_characteristics */
			put_bits(pH265Bs, pSeqCfg->ucTransferCharacteristics, 8);
			/* matrix_coefficients */
			put_bits(pH265Bs, pSeqCfg->ucMatrixCoef, 8);
		}
	}

	/* chroma_loc_info_presetn_flag */
	put_bits(pH265Bs, 0, 1);


	/* neutral_chroma_indication_flag */
	put_bits(pH265Bs, 0, 1);
	/* field_seq_flag */
	put_bits(pH265Bs, 0, 1);
	/* frame_field_info_present_flag */
	put_bits(pH265Bs, 0, 1);
	/* default_display_window_flag */
#if CMODEL_VERIFY
	put_bits(pH265Bs, 1, 1);
    write_uvlc_codeword(pH265Bs, 0);
    write_uvlc_codeword(pH265Bs, 0);
    write_uvlc_codeword(pH265Bs, 0);
    write_uvlc_codeword(pH265Bs, 0);
#else
    put_bits(pH265Bs, 0, 1);
#endif
    if (0 == uiFrameRate) {
        pSeqCfg->bTimeingPresentFlag = 0;
    }
	put_bits(pH265Bs, pSeqCfg->bTimeingPresentFlag, 1);
	if (pSeqCfg->bTimeingPresentFlag) {
		if ((uiFrameRate / 1000 * 1000) != uiFrameRate) {
			UINT32 uiCeilFr;
			UINT32 uiDiff1, uiDiff2;
			uiCeilFr = ((uiFrameRate / 1000) + 1) * 1000;
			uiDiff1 = abs((1001 * uiFrameRate / 1000) - uiCeilFr);
			uiDiff2 = abs(uiFrameRate - uiCeilFr);

			// check whether multiplying it with 1001/1000=1.001 brings it closer to frame_rate_integer
			if (uiDiff1 < uiDiff2) {
				num_units_in_tick = 1001;
				time_scale        = uiCeilFr;
			} else {

				num_units_in_tick = 1000;
				time_scale        = (uiFrameRate / 1000) * (num_units_in_tick);
			}

		} else {
            time_scale       = 27000000;
            num_units_in_tick = time_scale/(uiFrameRate/1000);
		}
		put_bits(pH265Bs, num_units_in_tick, 32);
		put_bits(pH265Bs, time_scale, 32);
		put_bits(pH265Bs, poc_proportional_flag, 1);
		if (poc_proportional_flag) {
			//TODO
		}
		/* vui_hrd_parameters_present_flag */
		put_bits(pH265Bs, 0, 1);
	}
	/* bitstream_restriction_flag */
	put_bits(pH265Bs, 0, 1);
}

static void profile_tier_level(bstream *pH265Bs, int profilePresentFlag, int maxNumSubLayersMinus1, UINT8 ucLevelIdc)
{
	int i, j;
	int general_profile_compatibility_flag[32] = {0};
	HEVC_PROFILE_TYPE general_profile_idc = HEVC_PROFILE_MAIN;
	HEVC_LEVEL_TYPE general_level_idc = ucLevelIdc;
	HEVC_TIER_TYPE general_tier_flag = HEVC_TIER_MAIN;

	general_profile_compatibility_flag[general_profile_idc] = 1;
	if (general_profile_idc == HEVC_PROFILE_MAIN) {
		/* A Profile::MAIN10 decoder can always decode Profile::MAIN */
		general_profile_compatibility_flag[HEVC_PROFILE_MAIN10] = 1;
	}

	if (profilePresentFlag) {
		put_bits(pH265Bs,                 0, 2);    /* general_profile_space  */
		put_bits(pH265Bs,  general_tier_flag, 1);   /* general_tier_flag */
		put_bits(pH265Bs, general_profile_idc, 5);  /* general_profile_idc */
	}
	for (j = 0; j < 32; j++) {
		put_bits(pH265Bs,             general_profile_compatibility_flag[ j ], 1);    /* general_profile_compatibility_flag[ j ] */
	}
	put_bits(pH265Bs,             0, 1);    /* general_progressive_source_flag */
	put_bits(pH265Bs,             0, 1);    /* general_interlaced_source_flag */
	put_bits(pH265Bs,             0, 1);    /* general_non_packed_constraint_flag */
	put_bits(pH265Bs,             0, 1);    /* general_frame_only_constraint_flag */
	if (general_profile_idc == 4 || general_profile_compatibility_flag[ 4 ] || general_profile_idc == 5 || general_profile_compatibility_flag[ 5 ] || general_profile_idc == 6 || general_profile_compatibility_flag[ 6 ] || general_profile_idc == 7 || general_profile_compatibility_flag[ 7 ]) {
		//TODO
	} else {
		put_bits(pH265Bs,             0, 43);    /* general_reserved_zero_43bits */
	}
	if ((general_profile_idc >= 1 && general_profile_idc <= 5) || general_profile_compatibility_flag[ 1 ] || general_profile_compatibility_flag[ 2 ] || general_profile_compatibility_flag[ 3 ] || general_profile_compatibility_flag[ 4 ] || general_profile_compatibility_flag[ 5 ]) {
		int general_inbld_flag = 0;
		put_bits(pH265Bs,             general_inbld_flag, 1);    /* general_inbld_flag */

	} else {
		put_bits(pH265Bs,             0, 1);    /* general_reserved_zero_bit */
	}
	put_bits(pH265Bs,             general_level_idc, 8);    /* general_level_idc */
	for (i = 0; i < maxNumSubLayersMinus1; i++) {
		put_bits(pH265Bs,             0, 1);    /* sub_layer_profile_present_flag[ i ] */
		put_bits(pH265Bs,             0, 1);    /* sub_layer_level_present_flag[ i ] */
	}
	if (maxNumSubLayersMinus1 > 0) {
		for (i = maxNumSubLayersMinus1; i < 8; i++) {
			put_bits(pH265Bs,             0, 2);    /* reserved_zero_2bits[ i ] */
		}
	}
	for (i = 0; i < maxNumSubLayersMinus1; i++) {
		//TODO
	}
}

static void st_ref_pic_set(bstream *pH265Bs, int stRpsIdx, STRPS *rps)
{
	int i;

	//function paramter
	int inter_ref_pic_set_prediction_flag = 0;

	//tmp value
	//int num_negative_pics=1;
	int num_positive_pics = 0;
	//int delta_poc_s0_minus1[1]={0};
	//int used_by_curr_pic_s0_flag[1]={1};
	int delta_poc_s1_minus1[1] = {0};
	int used_by_curr_pic_s1_flag[1] = {0};
    int prev = 0;

	if (stRpsIdx != 0) {
		put_bits(pH265Bs,             inter_ref_pic_set_prediction_flag, 1);    /* inter_ref_pic_set_prediction_flag */
	}
	if (inter_ref_pic_set_prediction_flag) {
		//TODO
	} else {
		write_uvlc_codeword(pH265Bs, rps->NumNegativePics);      /* num_negative_pics */
		write_uvlc_codeword(pH265Bs, num_positive_pics);     /* num_positive_pics */

		for (i = 0; i < (int)rps->NumNegativePics; i++) {
			write_uvlc_codeword(pH265Bs, prev - rps->DeltaPocS0[i] - 1);     /* delta_poc_s0_minus1[ i ] */
			prev = rps->DeltaPocS0[i];
			put_bits(pH265Bs,             rps->UsedByCurrPicS0[i], 1);  /* used_by_curr_pic_s0_flag[ i ] */
		}
		for (i = 0; i < num_positive_pics; i++) {
			write_uvlc_codeword(pH265Bs, delta_poc_s1_minus1[i]);    /* delta_poc_s1_minus1[ i ] */
			put_bits(pH265Bs,             used_by_curr_pic_s1_flag[i], 1);  /* used_by_curr_pic_s1_flag[ i ] */
		}
	}
	rps->NumDeltaPocs = rps->NumNegativePics + num_positive_pics; //(7-63)

}
//! Write  VPS
/*!
    Write  VPS

    @return void
*/
static void writeH265VPS(bstream *pH265Bs, H265EncSeqCfg *pSeqCfg, H265EncPicCfg *pPicCfg)
{
	int i;

	//function parameter
	//int vps_max_sub_layers_minus1=0;
	int vps_sub_layer_ordering_info_present_flag = 1;
	int vps_num_layer_sets_minus1 = 0;
	int vps_timing_info_present_flag = 0;
	int vps_extension_flag = 0;
	//int iRefFrmNum;

	//tmp value
	int vps_max_dec_pic_buffering_minus1[MAX_SUB_LAYER_SIZE] = {0};
	int vps_max_num_reorder_pics[MAX_SUB_LAYER_SIZE] = {0};
	int vps_max_latency_increase_plus1[MAX_SUB_LAYER_SIZE] = {0};

	put_bits(pH265Bs,             0, 4);    /* vps_video_parameter_set_id     */
	put_bits(pH265Bs,             1, 1);    /* vps_base_layer_internal_flag */
	put_bits(pH265Bs,             1, 1);    /* vps_base_layer_available_flag */
	put_bits(pH265Bs,             0, 6);    /* vps_reserved_zero_6bits   */

	put_bits(pH265Bs,             pSeqCfg->uiMaxSubLayerMinus1, 3);    /* vps_max_sub_layers_minus1       */
	put_bits(pH265Bs,             pSeqCfg->uiTempIdNestFlg, 1);    /* vps_temporal_id_nesting_flag       */
	put_bits(pH265Bs,        0xffff, 16);    /* vps_reserved_ffff_16bits       */

	profile_tier_level(pH265Bs, 1, pSeqCfg->uiMaxSubLayerMinus1, pSeqCfg->ucLevelIdc);

	put_bits(pH265Bs,             vps_sub_layer_ordering_info_present_flag, 1);  /* vps_sub_layer_ordering_info_present_flag */
	for (i = (vps_sub_layer_ordering_info_present_flag ? 0 : pSeqCfg->uiMaxSubLayerMinus1); i <= (int)pSeqCfg->uiMaxSubLayerMinus1; i++) {

		/* reference frame information */
		/* num_ref_frames              */
		//iRefFrmNum = 2;
		vps_max_dec_pic_buffering_minus1[i] = pSeqCfg->iRefFrmNum - 1;
		write_uvlc_codeword(pH265Bs, vps_max_dec_pic_buffering_minus1[ i ]);     /* vps_max_dec_pic_buffering_minus1[ i ]  */
		write_uvlc_codeword(pH265Bs, vps_max_num_reorder_pics[ i ]);     /* vps_max_num_reorder_pics[ i ]  */
		write_uvlc_codeword(pH265Bs, vps_max_latency_increase_plus1[ i ]);     /* vps_max_latency_increase_plus1[ i ]  */
	}
	put_bits(pH265Bs,             0, 6);     /* vps_max_layer_id */
	write_uvlc_codeword(pH265Bs, vps_num_layer_sets_minus1);     /* vps_num_layer_sets_minus1 */
	for (i = 1; i <= vps_num_layer_sets_minus1; i++) {
		//TODO
	}
	put_bits(pH265Bs,             vps_timing_info_present_flag, 1);    /* vps_timing_info_present_flag       */
	if (vps_timing_info_present_flag) {
		//TODO
	}
	put_bits(pH265Bs,             vps_extension_flag, 1);    /* vps_extension_flag       */
	if (vps_extension_flag) {
		//TODO
	}
	/* copies the last couple of bits into the byte buffer */
	write_rbsp_trailing_bits(pH265Bs);
}


//! Write  SPS
/*!
    Write  SPS

    @return void
*/
static void writeH265SPS(bstream *pH265Bs, H265EncSeqCfg *pSeqCfg, H265EncPicCfg *pPicCfg)
{
	int i;
	INT32  iFrameCroppingFlag = 1, iCropBottom = 0, iCropRight = 0;
	//function paramter
	//int sps_max_sub_layers_minus1=0;
	int sps_sub_layer_ordering_info_present_flag = 1;
	int sps_extension_present_flag = 0;
	UINT32 uiWidthMB, uiHeightMB;
	//tmp value
	int sps_max_dec_pic_buffering_minus1[MAX_SUB_LAYER_SIZE] = {0};
	int sps_max_num_reorder_pics[MAX_SUB_LAYER_SIZE] = {0};
	int sps_max_latency_increase_plus1[MAX_SUB_LAYER_SIZE] = {0};

	uiWidthMB = (pSeqCfg->usWidth + 15) / 16;
	uiHeightMB = (pSeqCfg->usHeight + 15) / 16;

	put_bits(pH265Bs,             0, 4);    /* sps_video_parameter_set_id */
	put_bits(pH265Bs,             pSeqCfg->uiMaxSubLayerMinus1, 3); /* sps_max_sub_layers_minus1 */
	put_bits(pH265Bs,             pSeqCfg->uiTempIdNestFlg, 1); /* sps_temporal_id_nesting_flag */
	profile_tier_level(pH265Bs, 1, pSeqCfg->uiMaxSubLayerMinus1, pSeqCfg->ucLevelIdc);

	write_uvlc_codeword(pH265Bs, 0);    /* sps_seq_parameter_set_id */
	write_uvlc_codeword(pH265Bs, pSeqCfg->uiChromaIdc); /* chroma_format_idc  */
	write_uvlc_codeword(pH265Bs, uiWidthMB << 4); /* pic_width_in_luma_samples  */
	write_uvlc_codeword(pH265Bs, uiHeightMB << 4); /* pic_height_in_luma_samples  */
	put_bits(pH265Bs,             1, 1);    /* conformance_window_flag */

	/* frame_cropping_flag */
	calculteFrameCropping(pSeqCfg, pPicCfg, &iCropBottom, &iCropRight);
	//DBG_IND("Crop = %d %d\r\n", (int)(iFrameCroppingFlag), (int)(iCropBottom));
	if (iFrameCroppingFlag) {
		write_uvlc_codeword(pH265Bs,           0);   // frame_crop_left_offset
		write_uvlc_codeword(pH265Bs,  iCropRight);   // frame_crop_right_offset
		write_uvlc_codeword(pH265Bs,           0);   // frame_crop_top_offset
		write_uvlc_codeword(pH265Bs, iCropBottom);   // frame_crop_bottom_offset
	}
	write_uvlc_codeword(pH265Bs, 0);    /* bit_depth_luma_minus8           */
	write_uvlc_codeword(pH265Bs, 0);    /* bit_depth_chroma_minus8         */

	/* log2_max_pic_order_cnt_lsb_minus4 */
#if CMODEL_VERIFY
	write_uvlc_codeword(pH265Bs, 4);
#else
    write_uvlc_codeword(pH265Bs, pSeqCfg->usLog2MaxPoc - 4);
#endif
	put_bits(pH265Bs, sps_sub_layer_ordering_info_present_flag, 1); /* sps_sub_layer_ordering_info_present_flag */

	for (i = (sps_sub_layer_ordering_info_present_flag ? 0 : pSeqCfg->uiMaxSubLayerMinus1); i <= (int)pSeqCfg->uiMaxSubLayerMinus1; i++) {
		/* reference frame information */
		/* num_ref_frames             */
		//iRefFrmNum = 2;
		sps_max_dec_pic_buffering_minus1[i] = pSeqCfg->iRefFrmNum - 1;

		write_uvlc_codeword(pH265Bs, sps_max_dec_pic_buffering_minus1[ i ]);    /* sps_max_dec_pic_buffering_minus1[ i ] */
		write_uvlc_codeword(pH265Bs, sps_max_num_reorder_pics[ i ]);     /* sps_max_num_reorder_pics[ i ] */
		write_uvlc_codeword(pH265Bs, sps_max_latency_increase_plus1[ i ]);    /* sps_max_latency_increase_plus1[ i ] */
	}

	write_uvlc_codeword(pH265Bs, 1);     /* log2_min_luma_coding_block_size_minus3 */
	write_uvlc_codeword(pH265Bs, 2);     /* log2_diff_max_min_luma_coding_block_size */
	write_uvlc_codeword(pH265Bs, 0);     /* log2_min_luma_transform_block_size_minus2 */
	write_uvlc_codeword(pH265Bs, 2);     /* log2_diff_max_min_luma_transform_block_size */
	write_uvlc_codeword(pH265Bs, 0);   /* max_transform_hierarchy_depth_inter */
	write_uvlc_codeword(pH265Bs, 1);   /* max_transform_hierarchy_depth_intra */
	put_bits(pH265Bs, 0, 1);           /* scaling_list_enabled_flag */

	put_bits(pH265Bs,              0, 1); /* amp_enabled_flag */
	put_bits(pH265Bs, pSeqCfg->sao_attr.sao_en, 1); /* sample_adaptive_offset_enabled_flag */
	put_bits(pH265Bs,              0, 1); /* pcm_enabled_flag */
	write_uvlc_codeword(pH265Bs, pSeqCfg->uiNumStRps);  /* num_short_term_ref_pic_sets */
	for (i = 0; i < (int)pSeqCfg->uiNumStRps; i++) {
		st_ref_pic_set(pH265Bs, i, &pSeqCfg->short_term_ref_pic_set[i]);
	}
	put_bits(pH265Bs,             pSeqCfg->uiLTRpicFlg, 1); /* long_term_ref_pics_present_flag */
	if (pSeqCfg->uiLTRpicFlg) {
		write_uvlc_codeword(pH265Bs, pSeqCfg->uiNumLTRpsSps);   /* num_long_term_ref_pics_sps */
		for (i = 0; i < (int)pSeqCfg->uiNumLTRpsSps; i++) {
			//TODO
		}
	}
	put_bits(pH265Bs,             pSeqCfg->uiTempMvpEn, 1); /* sps_temporal_mvp_enabled_flag */
	put_bits(pH265Bs,             1, 1); /* strong_intra_smoothing_enabled_flag */

	put_bits(pH265Bs,    pSeqCfg->bVUIEn, 1);   /* vui_parameters_present_flag */
	if (pSeqCfg->bVUIEn) {
		writeVUI(pH265Bs, pSeqCfg, pPicCfg);
	}
	put_bits(pH265Bs,             sps_extension_present_flag, 1); /* sps_extension_present_flag*/
	if (sps_extension_present_flag) {
		//TODO
	}
	/* copies the last couple of bits into the byte buffer */
	write_rbsp_trailing_bits(pH265Bs);

}

//! Write  PPS
/*!
    Write  PPS

    @return void
*/
static void writeH265PPS(bstream *pH265Bs, H265EncSeqCfg *pSeqCfg, H265EncPicCfg *pPicCfg, UINT32 uiPpsId)
{
	//UINT32 uiProfile_idc = pSeqCfg->uiEncProfile;

	//function paramter
	int pps_scaling_list_data_present_flag = 0;
	int pps_extension_present_flag = 0;

	DBG_IND("==============\r\n ppsid = %d\r\n", (int)(uiPpsId));
	/* PPS ID */
	write_uvlc_codeword(pH265Bs, uiPpsId); /* pps_pic_parameter_set_id */
	/* SPS ID */
	write_uvlc_codeword(pH265Bs, 0);     /* pps_seq_parameter_set_id */
	put_bits(pH265Bs, pPicCfg->uiDepSlcEn, 1);  /* dependent_slice_segments_enabled_flag */
	put_bits(pH265Bs, pPicCfg->uiOutPresFlg, 1);    /* output_flag_present_flag */
	put_bits(pH265Bs, pPicCfg->uiNumExSlcHdrBit, 3);    /* num_extra_slice_header_bits */
	put_bits(pH265Bs, 0, 1);    /* sign_data_hiding_enabled_flag */
	put_bits(pH265Bs, pPicCfg->uiCabacInitPreFlg, 1);   /* cabac_init_present_flag */

	/* some strange parameters for POC */
	write_uvlc_codeword(pH265Bs, pSeqCfg->ucNumRefIdxL0 - 1);         /* num_ref_idx_l0_active - 1 */
	write_uvlc_codeword(pH265Bs, 0);                                  /* num_ref_idx_l1_active - 1 */
	/* pic_init_qp - 26 */
	write_signed_uvlc_codeword(pH265Bs, 0);     // pic_init_qp - 26 //
	/* use_constrained_intra_pred */
	put_bits(pH265Bs, 0, 1);    /* constrained_intra_pred_flag */
	put_bits(pH265Bs, 0, 1);    /* transform_skip_enabled_flag */

	put_bits(pH265Bs, pPicCfg->uiCuQpDeltaEnabledFlag, 1);  /* cu_qp_delta_enabled_flag */
	if (pPicCfg->uiCuQpDeltaEnabledFlag) {
		UINT32 log2_mincu_size;
        UINT32 cusize[3] = {2,1,0};
		log2_mincu_size = cusize[pSeqCfg->uiUsrQpSize];
		write_uvlc_codeword(pH265Bs, log2_mincu_size);
	}
	write_signed_uvlc_codeword(pH265Bs, pPicCfg->iQpCbOffset);  // pps_cb_qp_offset //
	write_signed_uvlc_codeword(pH265Bs, pPicCfg->iQpCrOffset);  // pps_cr_qp_offset //
	put_bits(pH265Bs, pPicCfg->uiPpsSlcChrQpOfsPreFlg, 1); /* pps_slice_chroma_qp_offsets_present_flag */

	/* VCL parameters */
	/* weighted_pred_flag */
	put_bits(pH265Bs, pPicCfg->uiWeiPredFlg, 1);

	put_bits(pH265Bs, 0, 1);    /* weighted_bipred_flag */
	put_bits(pH265Bs, 0, 1);    /* transquant_bypass_enabled_flag */
	put_bits(pH265Bs, pSeqCfg->bTileEn, 1);    /* tiles_enabled_flag */
	put_bits(pH265Bs, pPicCfg->uiEntroCodSyncEn, 1);    /* entropy_coding_sync_enabled_flag */
	if (pSeqCfg->bTileEn) {
        UINT32 uiNumColumns = pSeqCfg->ucTileNum, uiNumRows = 1, i;
        write_uvlc_codeword(pH265Bs, uiNumColumns-1); /* num_tile_columns_minus1 */
        write_uvlc_codeword(pH265Bs, uiNumRows-1);  /* num_tile_rows_minus1  */
        put_bits(pH265Bs, 0, 1);    /* uniform_spacing_flag  */
        for( i=0; i < (uiNumColumns-1); i++)
        {
            write_uvlc_codeword(pH265Bs, (pSeqCfg->uiTileWidth[i]>>6)-1); /* column_width_minus1[0]  */
        }
        put_bits(pH265Bs, pSeqCfg->ilf_attr.ilf_across_tile_en, 1); /* pps_loop_filter_across_tiles_enabled_flag */
	}

	put_bits(pH265Bs, pSeqCfg->ilf_attr.ilf_across_slice_en, 1); /* pps_loop_filter_across_slices_enabled_flag */
	put_bits(pH265Bs, pSeqCfg->ilf_attr.db_control_present, 1);  /* deblocking_filter_control_present_flag */
	if (pSeqCfg->ilf_attr.db_control_present) {
		put_bits(pH265Bs, pSeqCfg->ilf_attr.ilf_db_override_en, 1); /* deblocking_filter_override_enabled_flag */
		put_bits(pH265Bs, pSeqCfg->ilf_attr.ilf_db_disable, 1);  /* pps_deblocking_filter_disabled_flag */
		if (!pSeqCfg->ilf_attr.ilf_db_disable) {
			write_signed_uvlc_codeword(pH265Bs, pSeqCfg->ilf_attr.llf_bata_offset_div2);
			write_signed_uvlc_codeword(pH265Bs, pSeqCfg->ilf_attr.llf_alpha_c0_offset_div2);
		}
	}

	/* pic_scaling_matrix_present_flag */
	put_bits(pH265Bs, pps_scaling_list_data_present_flag, 1);
	if (pps_scaling_list_data_present_flag) {
		//TODO
	}
	put_bits(pH265Bs, pPicCfg->uiListModPreFlg, 1); /* lists_modification_present_flag */
	write_uvlc_codeword(pH265Bs, 0);/* log2_parallel_merge_level_minus2 */
	put_bits(pH265Bs, pPicCfg->uiSlcSegHdrExtPreFlg, 1);    /* slice_segment_header_extension_present_flag */
	put_bits(pH265Bs, pps_extension_present_flag, 1);   /* pps_extension_present_flag */
	if (pps_extension_present_flag) {
		//TODO
	}
	write_rbsp_trailing_bits(pH265Bs);

}

//! Rbsp convert to Ebsp
/*!
    Rbsp convert to Ebsp

    @return void
*/

#if 0
static UINT32 rbspToebsp(UINT8 *pucAddr, UINT8 *pucBuf, UINT32 uiSize)
{
	UINT32 uiCount = 0, i, j;

	for (i = 0, j = 0; j < uiSize; i++, j++) {
		// starting from begin_bytepos to avoid header information
		if (uiCount == 2 && ((pucBuf[j] & 0xFC) == 0)) {
			//DBG_ERR("i = %d\r\n", (int)(i));
			pucAddr[i++] = 0x03;
			uiCount = 0;
		}
		pucAddr[i] = pucBuf[j];

		uiCount = (pucAddr[i]) ? 0 : uiCount + 1;
	}

	return i;
}
#endif


//! Write  H265 encoder header
/*!
    Write  H265 encoder header

    @return void
*/
UINT8 ucH265EncNalBuf[H265ENC_HEADER_MAXSIZE];

void h265Enc_encSeqHeader(H265ENC_CTX *pVdoCtx, H26XCOMN_CTX *pComnCtx)
{
	H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	UINT32 pHeaderAddr = pComnCtx->stVaAddr.uiSeqHdr;
	bstream sH265Bs;
	bstream *pH265Bs = &sH265Bs;
	UINT32 uiTotalBytes = 0, uiSize;
	UINT8  *pucAddr;
	UINT32 uiTmeporalid = 0;
	UINT32 uiVPSHdrLen;
	UINT32 uiSPSHdrLen;
	UINT32 uiPPSHdrLen;

	pucAddr = (UINT8 *)pHeaderAddr;

	/* encode VPS */
	uiTotalBytes += writeNalHeader(pucAddr + uiTotalBytes, NAL_UNIT_VPS, uiTmeporalid);

	//DBG_ERR("Nal 0x%x\r\n", (unsigned int)(uiTotalBytes));
	//H265PrtMem2((UINT32)pucAddr,uiTotalBytes);

	init_pack_bitstream(pH265Bs, (UINT8 *)ucH265EncNalBuf, 64);
	writeH265VPS(pH265Bs, pSeqCfg, pPicCfg);
	uiSize = bs_byte_length(pH265Bs);

	//DBG_ERR("VPS 0x%x\r\n", (unsigned int)(uiSize));
	//h26x_prtMem((UINT32)ucH265EncNalBuf,uiSize);

	uiTotalBytes += rbspToebsp(pucAddr + uiTotalBytes, ucH265EncNalBuf, uiSize);
	//DBG_DUMP("after VPS, uiTotalBytes = 0x%x\r\n", uiTotalBytes, uiSize);
	//h26x_prtMem((UINT32)pucAddr,uiTotalBytes);
	uiVPSHdrLen = uiTotalBytes;

	/* encode SPS */
	uiTotalBytes += writeNalHeader(pucAddr + uiTotalBytes, NAL_UNIT_SPS, uiTmeporalid);
	init_pack_bitstream(pH265Bs, (UINT8 *)ucH265EncNalBuf, 64);
	writeH265SPS(pH265Bs, pSeqCfg, pPicCfg);
	uiSize = bs_byte_length(pH265Bs);
	//DBG_ERR("SPS 0x%x\r\n", (unsigned int)(uiSize));
	//H265PrtMem2((UINT32)ucH265EncNalBuf,uiSize);
	uiTotalBytes += rbspToebsp(pucAddr + uiTotalBytes, ucH265EncNalBuf, uiSize);
	uiSPSHdrLen = uiTotalBytes - uiVPSHdrLen;

	//DBG_DUMP("after SPS, uiTotalBytes = 0x%x\r\n", uiTotalBytes, uiSize);
	//h26x_prtMem((UINT32)pucAddr,uiTotalBytes);

	/* encode PPS */
	uiTotalBytes += writeNalHeader(pucAddr + uiTotalBytes, NAL_UNIT_PPS, uiTmeporalid);
	init_pack_bitstream(pH265Bs, (UINT8 *)ucH265EncNalBuf, 128);
	writeH265PPS(pH265Bs, pSeqCfg, pPicCfg, 0); // 1 PPS
	uiSize = bs_byte_length(pH265Bs);
	uiTotalBytes += rbspToebsp(pucAddr + uiTotalBytes, ucH265EncNalBuf, uiSize);
	pVdoCtx->uiSeqHdrLen = uiTotalBytes;
	uiPPSHdrLen = uiTotalBytes - uiVPSHdrLen - uiSPSHdrLen;

	pVdoCtx->uiVPSHdrLen = uiVPSHdrLen;
	pVdoCtx->uiSPSHdrLen = uiSPSHdrLen;
	pVdoCtx->uiPPSHdrLen = uiPPSHdrLen;

	//DBG_DUMP("after PPS, uiTotalBytes = 0x%x\r\n", uiTotalBytes, uiSize);
	//h26x_prtMem((UINT32)pucAddr,uiTotalBytes);

	//dma_flushWriteCache((UINT32)pucAddr, uiTotalBytes);

	if (uiTotalBytes > pComnCtx->uiPicHdrBufSize) {
		DBG_ERR("Header Len Error :%d, Addr:0x%08x, max :%d\r\n", (int)uiTotalBytes, (unsigned int)pHeaderAddr, (int)pComnCtx->uiPicHdrBufSize);
	}

	//DBG_ERR("uiTotalBytes = %d\r\n", (int)(uiTotalBytes));
}


static void ref_pic_lists_modification(bstream *bs, UINT32 num_ref_idx_l0_active_minus1, UINT32 NumPicTotalCurr, STRPS *rps, H265EncSeqCfg *pSeqCfg)
{
	int ref_pic_list_modification_flag_l0 = 1;
	int i;
	int list_entry_l0[32] = {0, 1};

	put_bits(bs, ref_pic_list_modification_flag_l0, 1);
	if (ref_pic_list_modification_flag_l0) {
		for (i = 0; i <= (int)num_ref_idx_l0_active_minus1; i++) {
			int reqBits = 0;
			while ((1 << reqBits) < (int)(NumPicTotalCurr)) {
				reqBits++;
			}

			put_bits(bs, list_entry_l0[i], reqBits);
		}
	}
}

INT32 h265Enc_encSliceHeader(H265ENC_CTX *pVdoCtx, H26XFUNC_CTX *pFuncCtx, H26XCOMN_CTX *pComnCtx,BOOL bGetSeqHdrEn)
{
	H265EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H265EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	bstream sH265Bs;
	bstream *bs = &sH265Bs;
	UINT32 *bsdma_addr = (UINT32 *)pComnCtx->stVaAddr.uiBsdma;
	UINT32 i;
	//UINT32 uiTmeporalid = 0;
	UINT32 uiNalUnitType;
	//function parameter
	//UINT32 slice_temporal_mvp_enabled_flag = pSeqCfg->uiTempMvpEn;
	UINT32 collocated_from_l0_flag = 1;     //When collocated_from_l0_flag is not present, it is inferred to be equal to 1.
	UINT32 deblocking_filter_override_flag = 1;
	UINT32 slice_deblocking_filter_disabled_flag = pSeqCfg->ilf_attr.ilf_db_disable; //When slice_deblocking_filter_disabled_flag is not present, it is inferred to be equal to pps_deblocking_filter_disabled_flag
	UINT32 slice_loop_filter_across_slices_enabled_flag = pSeqCfg->ilf_attr.ilf_across_slice_en;
	//tmp value
	UINT32 short_term_ref_pic_set_sps_flag = 1;
	UINT32 MaxNumMergeCand = 2; //the maximum number of merging motion vector prediction (MVP) candidates supported
	//UINT32 si, /*slice_num, */slice_num2/*, tile_num = pSeqCfg->ucTileNum*/;
	UINT32 si;
	INT32 maxSliceSegmentAddress = (int)pSeqCfg->usLcuWidth * (int)pSeqCfg->usLcuHeight;
	INT32 bitsSliceSegmentAddress = 0;
	//UINT32 uiHwHeaderSize[H26X_MAX_HEIGHT/64*H26X_MAX_TILE_NUM];
	UINT32 uiHwBsLen = 0;
	UINT32 uiSliceRowNum;
	UINT32 uiNaluNum = pFuncCtx->stSliceSplit.uiNaluNum;
	UINT32 slice_segment_address = 0;
	UINT32 uiPicInfoLen = 0;

	//fix CIM coverity
	/*
	if (uiNaluNum > H26X_MAX_HEIGHT/64*H26X_MAX_TILE_NUM) {
		uiNaluNum = H26X_MAX_HEIGHT/64*H26X_MAX_TILE_NUM;
		DBG_DUMP("over nalu number \r\n");
	}
	*/
	if (h26x_getChipId() == 98520) {
		if ((uiNaluNum > H265_MAX_HEIGHT/64*H26X_MAX_TILE_NUM) || (maxSliceSegmentAddress > H265_MAX_WIDTH/64*H265_MAX_HEIGHT/64)) {
			DBG_ERR("fail! %d over max nalu number: 210, or %d over max slice segment address: 2688\r\n", (int)uiNaluNum, (int)maxSliceSegmentAddress);
			return H26XENC_FAIL;
		}
	} else {
		if ((uiNaluNum > H265_MAX_HEIGHT_528/64*H26X_TILE_NUM_528) || (maxSliceSegmentAddress > H265_MAX_WIDTH_528/64*H265_MAX_HEIGHT_528/64)) {
			DBG_ERR("fail! %d over max nalu number: 320, or %d over max slice segment address: 6400\r\n", (int)uiNaluNum, (int)maxSliceSegmentAddress);
			return H26XENC_FAIL;
		}
	}

	while (maxSliceSegmentAddress > (1 << bitsSliceSegmentAddress)) {
		bitsSliceSegmentAddress++;
	}

	uiSliceRowNum = (pFuncCtx->stSliceSplit.stExe.bEnable)? pFuncCtx->stSliceSplit.stExe.uiSliceRowNum: pSeqCfg->usLcuHeight;
	//slice_num = (pSeqCfg->usLcuHeight + uiSliceRowNum - 1) / uiSliceRowNum;
	//DBG_DUMP("slice_num = %d\r\n", (int)(slice_num));
	//slice_num2 = uiNaluNum;

	//if(pSeqCfg->bTileEn) uiNaluNum *= tile_num;
	//DBG_DUMP("slice_num = %d\r\n", (int)(slice_num));
	bsdma_addr[0] = uiNaluNum + 1;

	if (IS_IDRSLICE(pPicCfg->ePicType)) {
		uiNalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
	}
	//else if (pVdoCtx->eRecIdx == FRM_IDX_NON_REF) {
	//    uiNalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_N;
	//}
	else {
		uiNalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
	}

	//DBG_IND("uiNalUnitType = %d, Idr = %d\r\n", (int)(uiNalUnitType), (int)(pPicCfg->ucIdr));
	//DBG_IND("slice_num = %d\r\n", (int)(slice_num));
	pPicCfg->uiPicHdrLen = 0;

	if (bGetSeqHdrEn) {
		//fix CIM coverity
		if (pVdoCtx->uiSeqHdrLen > H265ENC_HEADER_MAXSIZE) {
		    pVdoCtx->uiSeqHdrLen = H265ENC_HEADER_MAXSIZE;
		    DBG_DUMP("over max header size\r\n");
		}

		memcpy((void *)pComnCtx->stVaAddr.uiPicHdr, (void *)pComnCtx->stVaAddr.uiSeqHdr, pVdoCtx->uiSeqHdrLen);
		uiPicInfoLen += pVdoCtx->uiSeqHdrLen;
	}
	if ((g_rc_dump_log == 1) && (pComnCtx->uiRcLogLen != 0)) {

		UINT8 *addr = (UINT8 *)(pComnCtx->stVaAddr.uiPicHdr + uiPicInfoLen);
		UINT32 idx  = writeNalHeader(addr, NAL_UNIT_PREFIX_SEI, 0);
		UINT32 bsplen;

		addr[idx++] = 0x55;
		addr[idx++] = pComnCtx->uiRcLogLen;
		bsplen = rbspToebsp((addr + idx), (UINT8 *)pComnCtx->uipRcLogAddr, pComnCtx->uiRcLogLen);
		addr[idx + bsplen] = 0x80;

		uiPicInfoLen += (idx + bsplen + 1);
	}
	if (pVdoCtx->bSEIPayloadBsChksum) {

		UINT8 *addr = (UINT8 *)(pComnCtx->stVaAddr.uiPicHdr + uiPicInfoLen);
		UINT32 idx  = writeNalHeader(addr, NAL_UNIT_PREFIX_SEI, 0);
		UINT32 bsplen;

		//payload type: reserved
		addr[idx++] = 0x37;
		//payload size
		addr[idx++] = 4;

		bsplen = rbspToebsp((addr + idx), (UINT8 *)&pVdoCtx->uiBsCheckSum, 4);
		addr[idx + bsplen] = 0x80;

		uiPicInfoLen += (idx + bsplen + 1);
	}
	init_pack_bitstream(bs, (UINT8 *)(pComnCtx->stVaAddr.uiPicHdr + uiPicInfoLen), (pComnCtx->uiPicHdrBufSize - uiPicInfoLen));

	for (si = 0; si < uiNaluNum; si++) {
		UINT32 first_slice_segment_in_pic_flag = (si == 0);
		UINT32 dependent_slice_segment_flag = 0;
		//UINT32 row_idx,tile_idx;
		//row_idx = si % slice_num2;
		//tile_idx = si / slice_num2;
		if (si) {
			if (pSeqCfg->bTileEn) {
				if (pFuncCtx->stSliceSplit.stExe.bEnable) {
					UINT32 slice_num = uiNaluNum/pSeqCfg->ucTileNum;
					UINT32 tile_idx = si/slice_num;
					slice_segment_address = (si%slice_num) * uiSliceRowNum * pSeqCfg->usLcuWidth;
					//if (tile_idx && si % slice_num == 0)
					{
						UINT32 j;
						for (j = 0; j < tile_idx; j++)
							slice_segment_address += ((pSeqCfg->uiTileWidth[j]+63)>>6);
					}
				} else {
					slice_segment_address += ((pSeqCfg->uiTileWidth[si-1]+63)>>6);
				}
			} else {
				slice_segment_address = si * uiSliceRowNum * pSeqCfg->usLcuWidth;
			}
		}
		//if (tile_idx == 0)
		//    slice_segment_address = row_idx * uiSliceRowNum * pSeqCfg->usLcuWidth;
		//else
		//    slice_segment_address += ((pSeqCfg->uiTileWidth[tile_idx-1]+63)/64);
		//DBG_DUMP("====== si = %d =============, addr = %d\r\n", (int)(si), (int)(slice_segment_address));
		//DBG_DUMP("t-w:%d\r\n", (int)(pSeqCfg->uiTileWidth[tile_idx-1]));

    #if FOLLOW_K_NALU_RULE
        {
            UINT32 nal_tid = 0;
            if(pSeqCfg->uiMaxSubLayerMinus1>0){
                if(  pSeqCfg->uiEn4xP && !pSeqCfg->bEnableLt && pPicCfg->uiTemporalId[0] > 0 ){
                        nal_tid = pPicCfg->uiTemporalId[0] - 1;         //hack for 313 special case
                }
                else if(  pSeqCfg->uiEn2xP && !pSeqCfg->bEnableLt && pPicCfg->uiTemporalId[0] == 1 ){
                        nal_tid = 0;                                                      //hack for 313 special case
                }
                else{
                        nal_tid = pPicCfg->uiTemporalId[0];
                }
            }
            write_nal_header(bs, nal_tid , uiNalUnitType, si);
        }
    #else
		write_nal_header(bs, pSeqCfg->uiMaxSubLayerMinus1>0 ? pPicCfg->uiTemporalId[0]: 0, uiNalUnitType, si);
    #endif

		put_bits(bs, first_slice_segment_in_pic_flag, 1);   // first_slice_segment_in_pic_flag : multi-slice not ready, use single slice now /
		if (uiNalUnitType >= NAL_UNIT_CODED_SLICE_BLA_W_LP && uiNalUnitType <= NAL_UNIT_RESERVED_IRAP_VCL23) {
			put_bits(bs, 0, 1); //no_output_of_prior_pics_flag
		}
		write_uvlc_codeword(bs, 0); // slice_pic_parameter_set_id : only 0 //
		if (!first_slice_segment_in_pic_flag) {

			if (pPicCfg->uiDepSlcEn) {
				put_bits(bs, dependent_slice_segment_flag, 1);   // first_slice_segment_in_pic_flag : multi-slice not ready, use single slice now /
			}
			put_bits(bs, slice_segment_address, bitsSliceSegmentAddress);   // slice_segment_address

		}

		if (!pPicCfg->uiDepSlcEn) {
			UINT32 uiPicType;
			for (i = 0; i < pPicCfg->uiNumExSlcHdrBit; i++) {
				put_bits(bs, 0, 1); //slice_reserved_flag[ i ]  //ignored
			}

			uiPicType = (pPicCfg->ePicType == P_SLICE) ? 1 : 2;
			write_uvlc_codeword(bs, uiPicType);   //slice_type  // support frame mode only //
			if (pPicCfg->uiOutPresFlg) {
				//TODO
			}
			if (uiNalUnitType != NAL_UNIT_CODED_SLICE_IDR_W_RADL && uiNalUnitType != NAL_UNIT_CODED_SLICE_IDR_N_LP) {
				UINT32 uiPoc = ((pSeqCfg->cPocLsb[0]) & ~((((UINT32)(-1)) << pSeqCfg->usLog2MaxPoc)));
				put_bits(bs, uiPoc, pSeqCfg->usLog2MaxPoc);    // slice_pic_order_cnt_lsb //
				put_bits(bs, short_term_ref_pic_set_sps_flag, 1);   //short_term_ref_pic_set_sps_flag
				// fix cim
				if (pSeqCfg->uiNumStRps < MAX_ST_RPS_SIZE) {
					if (!short_term_ref_pic_set_sps_flag) {
						//TODO
					} else if (pSeqCfg->uiNumStRps > 1) {
						int reqBits = 0;
						while ((1 << reqBits) < (int)pSeqCfg->uiNumStRps) {
							reqBits++;
						}
						put_bits(bs, pSeqCfg->uiSTRpsIdx, reqBits);   //short_term_ref_pic_set_idx,  stRpsIdx
					}
				} else {
					DBG_ERR("h265e: uiNumStRps is over boundary\r\n");
				}
				if (pSeqCfg->uiLTRpicFlg) {
					UINT32 num_long_term_sps = 0;

					if (pSeqCfg->uiNumLTRpsSps > 0) {
						write_uvlc_codeword(bs, num_long_term_sps);
					}
					//DBG_ERR("pSeqCfg->uiNumLTPic%d\r\n", (int)(pSeqCfg->uiNumLTPic));
					if (1 == pPicCfg->iPoc && 1 != pSeqCfg->u32LtInterval) {
						write_uvlc_codeword(bs, 0); //num_long_term_pics
					}
					else {
						write_uvlc_codeword(bs, pSeqCfg->uiNumLTPic); //num_long_term_pics
						for (i = 0; i < num_long_term_sps + pSeqCfg->uiNumLTPic; i++) {
							int delta_poc_msb_present_flag = 0;
							if (i < num_long_term_sps) {
								//TODO
							} else {
								put_bits(bs, pSeqCfg->iPocLT[i], pSeqCfg->usLog2MaxPoc);    // poc_lsb_lt[ i ] //
								put_bits(bs, pSeqCfg->uiUsedbyCurrLTFlag[i], 1);    // used_by_curr_pic_lt_flag[ i ]//
							}
							put_bits(bs, delta_poc_msb_present_flag, 1);    // delta_poc_msb_present_flag[ i ] //
							if (delta_poc_msb_present_flag > 0) {
								//TODO
							}
						}
					}
				}
				if (pSeqCfg->uiTempMvpEn) {
					#if 0
					UINT32 bIsCurrRefLongTerm = pComnCtx->stVaAddr.uiRefAndRecIsLT[pVdoCtx->eRecIdx];
					UINT32 bIsColRefLongTerm = pPicCfg->uiColRefIsLT;
					if (pSeqCfg->bEnableLt) {
						// 96510 not support ( bIsCurrRefLongTerm != bIsColRefLongTerm ) case
						pSeqCfg->uiSliceTempMvpEn = (bIsCurrRefLongTerm != bIsColRefLongTerm) ? 0 : 1;
					} else {
						pSeqCfg->uiSliceTempMvpEn = 1;
					}
					#else
					pSeqCfg->uiSliceTempMvpEn = 1;
					#endif

					if (pPicCfg->uiFrameSkip) {
						pSeqCfg->uiSliceTempMvpEn = 0;
					}

					put_bits(bs, pSeqCfg->uiSliceTempMvpEn, 1);   //slice_temporal_mvp_enabled_flag
				}
			} else {
				//pSeqCfg->uiSliceTempMvpEn = 0;// pSeqCfg->uiTempMvpEn;
				pSeqCfg->uiSliceTempMvpEn = pSeqCfg->uiTempMvpEn;
			}
			if (pSeqCfg->sao_attr.sao_en) {
				put_bits(bs, (pPicCfg->uiFrameSkip) ? 0 : pSeqCfg->sao_attr.sao_luma_flag, 1); //slice_sao_luma_flag
				if (pSeqCfg->uiChromaArrayType) {
					put_bits(bs, (pPicCfg->uiFrameSkip) ? 0 : pSeqCfg->sao_attr.sao_chroma_flag, 1); //slice_sao_chroma_flag
				}
			}
			if (!IS_ISLICE(pPicCfg->ePicType)) {
				STRPS *rps = &pSeqCfg->short_term_ref_pic_set[pSeqCfg->uiSTRpsIdx];
				UINT32 NumPicTotalCurr = (pSeqCfg->uiNumLTPic + rps->NumNegativePics);
				UINT32 num_ref_idx_active_override_flag = 0;//(NumPicTotalCurr>1)? 1: 0;
				UINT32 num_ref_idx_l0_active_minus1 = (num_ref_idx_active_override_flag == 0) ? (UINT32)(pSeqCfg->ucNumRefIdxL0 - 1) : (NumPicTotalCurr - 1); //pSeqCfg->uiNumRefIdxL0 = num_ref_idx_l0_default_active_minus1

				put_bits(bs, num_ref_idx_active_override_flag, 1);  // ref_pic_reordering_flag_L0 //
				if (num_ref_idx_active_override_flag) {
					//DBG_ERR("num_ref_idx_l0_active_minus1=%d\r\n", (int)(num_ref_idx_l0_active_minus1));
					write_uvlc_codeword(bs, num_ref_idx_l0_active_minus1);  //num_ref_idx_l0_active_minus1 // ref idx L0 active minus1 //
					if (pPicCfg->ePicType == B_SLICE) {
						//TODO
					}
				}
				if (pPicCfg->uiListModPreFlg && NumPicTotalCurr > 1) { //if( lists_modification_present_flag && NumPicTotalCurr > 1 )
					ref_pic_lists_modification(bs, num_ref_idx_l0_active_minus1, NumPicTotalCurr, rps, pSeqCfg);
				}
				if (pPicCfg->ePicType == B_SLICE) {
					//TODO
				}
				if (pPicCfg->uiCabacInitPreFlg) {
					//TODO
				}
				if (pSeqCfg->uiSliceTempMvpEn) {
					if (pPicCfg->ePicType == B_SLICE) {
						//TODO
					}
					if ((collocated_from_l0_flag && num_ref_idx_l0_active_minus1 > 0)) { //if( ( collocated_from_l0_flag && num_ref_idx_l0_active_minus1 > 0 ) | | ( !collocated_from_l0_flag && num_ref_idx_l1_active_minus1 > 0 ) )
						write_uvlc_codeword(bs, 0); //collocated_ref_idx
					}
				}
				if ((pPicCfg->uiWeiPredFlg && pPicCfg->ePicType == P_SLICE)) { //if( ( weighted_pred_flag && slice_type = = P ) | | ( weighted_bipred_flag && slice_type = = B ) )
					//TODO
				}
				write_uvlc_codeword(bs, 5 - MaxNumMergeCand); //five_minus_max_num_merge_cand
			}
			write_signed_uvlc_codeword(bs, ((INT32)pPicCfg->ucSliceQP - 26));   // slice_qp_delta //
			if (pPicCfg->uiPpsSlcChrQpOfsPreFlg) {
				//TODO
			}
			//if( chroma_qp_offset_list_enabled_flag )
			//TODO
			if (pSeqCfg->ilf_attr.db_control_present) { // reference cmodel's writing
				if (pSeqCfg->ilf_attr.ilf_db_override_en) {
					put_bits(bs, deblocking_filter_override_flag, 1);  // deblocking_filter_override_flag
				}

				if (deblocking_filter_override_flag) {
					if (pPicCfg->uiFrameSkip) {
						put_bits(bs, 1, 1); // slice_deblocking_filter_disabled_flag
					} else {
						put_bits(bs, slice_deblocking_filter_disabled_flag, 1); // slice_deblocking_filter_disabled_flag
						if (!slice_deblocking_filter_disabled_flag) {
							write_signed_uvlc_codeword(bs, (INT32)pSeqCfg->ilf_attr.llf_bata_offset_div2);
							write_signed_uvlc_codeword(bs, (INT32)pSeqCfg->ilf_attr.llf_alpha_c0_offset_div2);
						}
					}
				}
			}
			if (pPicCfg->uiFrameSkip) {
				;
			} else {
				if (pSeqCfg->ilf_attr.ilf_across_slice_en && (pSeqCfg->sao_attr.sao_luma_flag || pSeqCfg->sao_attr.sao_chroma_flag || !slice_deblocking_filter_disabled_flag)) {
					put_bits(bs, slice_loop_filter_across_slices_enabled_flag, 1);//slice_loop_filter_across_slices_enabled_flag
				}
			}
		}

		if (pSeqCfg->bTileEn || pPicCfg->uiEntroCodSyncEn) {
			write_uvlc_codeword(bs, 0);  /* num_entry_point_offsets  */

		}
		if (pPicCfg->uiSlcSegHdrExtPreFlg) {
			//TODO
		}
		write_rbsp_trailing_bits(bs);
		//uiSize = bs_byte_length(bs) - uiTotalBytes;
		//DBG_IND("[%d] uiSize = 0x%x 0x%x\r\n", (int)(pContext->sH265EncRegSet.ePicUnit.uiHwHeaderNum), (unsigned int)(uiSize), (unsigned int)(pContext->sH265EncRegSet.ePicUnit.uiHwBsLen));
		//uiTotalBytes += uiSize;
		#if 0
		bsdma_addr[si*2 + 1] = h26x_getPhyAddr(pComnCtx->stVaAddr.uiPicHdr) + pPicCfg->uiPicHdrLen;
		bsdma_addr[si*2 + 2] = (bs_byte_length(bs) - pPicCfg->uiPicHdrLen);
		#else
		if (si == 0) {
			bsdma_addr[1] = h26x_getPhyAddr(pComnCtx->stVaAddr.uiPicHdr);
			bsdma_addr[2] = bs_byte_length(bs) + uiPicInfoLen;
		}
		else {
			bsdma_addr[si*2 + 1] = h26x_getPhyAddr(pComnCtx->stVaAddr.uiPicHdr) + uiPicInfoLen + pPicCfg->uiPicHdrLen;
			bsdma_addr[si*2 + 2] = (bs_byte_length(bs) - pPicCfg->uiPicHdrLen);
		}
		#endif
        //uiHwHeaderSize[si] = bsdma_addr[si*2 + 2];

        //DBG_DUMP("bsdma_addr[%d] = 0x%08x 0x%x\r\n", si*2 + 1,bsdma_addr[si*2 + 1], bsdma_addr[si*2 + 2]);
        //h26x_prtMem((UINT32)pComnCtx->stVaAddr.uiPicHdr + pPicCfg->uiPicHdrLen,bsdma_addr[si*2 + 2]);

		pPicCfg->uiPicHdrLen = bs_byte_length(bs);
	}
#if 0
	pPicCfg->uiPicHdrLen = bs_byte_length(bs);
#else
	for(i = 0; i < uiNaluNum; i++)
	{
		uiHwBsLen += ((bsdma_addr[i*2 + 2]+3)>>2)<<2; // word align
	}
	//bsdma_addr[i*2 + 1] = h26x_getPhyAddr(pComnCtx->stVaAddr.uiPicHdr) + pPicCfg->uiPicHdrLen;
	bsdma_addr[i*2 + 1] = h26x_getPhyAddr(pComnCtx->stVaAddr.uiPicHdr) + uiPicInfoLen + pPicCfg->uiPicHdrLen ;
	bsdma_addr[i*2 + 2] = 1;
	uiHwBsLen += 4;
	pPicCfg->uiPicHdrLen = uiHwBsLen;
#endif

    {
        //UINT32 uiSizeBSDMA = (pSeqCfg->usLcuHeight*2 + 1)*4 * 2;
        //DBG_DUMP("uiBsdma = 0x%08x 0x%08x\r\n", (UINT32)pComnCtx->stVaAddr.uiBsdma, uiSizeBSDMA);
        //h26x_prtMem((UINT32)pComnCtx->stVaAddr.uiBsdma,uiSizeBSDMA);
    }

	h26x_cache_clean(pComnCtx->stVaAddr.uiPicHdr, pPicCfg->uiPicHdrLen);
	h26x_cache_clean(pComnCtx->stVaAddr.uiBsdma, ((uiNaluNum+1)*2 + 1)*4);

	return H26XENC_SUCCESS;
}


