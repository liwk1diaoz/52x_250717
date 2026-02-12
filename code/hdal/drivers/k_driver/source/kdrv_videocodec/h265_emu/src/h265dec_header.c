/*
    H265ENC module driver for NT96660

    decode h265 header

    @file       h265dec_header.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "kdrv_vdocdc_dbg.h"

#include "h26x_common.h"

#include "h265dec_header.h"
#include "h265dec_int.h"

//#if !DEBUG
#define ptl_get_bits(X,Y,args...) get_bits(X,Y)
#define ptl_skip_bits(X,Y,args...) get_bits(X,Y) //skip_bits(X,Y)
#define vps_get_bits(X,Y,args...) get_bits(X,Y)
#define vps_get_uvlc(X,args...) read_uvlc_codeword(X)
#define sps_get_bits(X,Y,args...) get_bits(X,Y)
#define sps_get_uvlc(X,args...) read_uvlc_codeword(X)
#define strps_get_bits(X,Y,args...) get_bits(X,Y)
#define strps_get_uvlc(X,args...) read_uvlc_codeword(X)
#define pps_get_bits(X,Y,args...) get_bits(X,Y)
#define pps_get_uvlc(X,args...) read_uvlc_codeword(X)
#define pps_get_svlc(X,args...) read_signed_uvlc_codeword(X)
#define slice_get_bits(X,Y,args...) get_bits(X,Y)
#define slice_get_uvlc(X,args...) read_uvlc_codeword(X)
#define slice_get_svlc(X,args...) read_signed_uvlc_codeword(X)
#define rplm_get_bits(X,Y,args...) get_bits(X,Y)
#define vui_get_bits(X,Y,args...) get_bits(X,Y)
#define vui_get_uvlc(X,args...) read_uvlc_codeword(X)
#define nal_get_bits(X,Y,args...) get_bits(X,Y)

#define MinReqBits(X) ({int _req; if ((X) < 2) _req = 0; else _req = 32 - __builtin_clz((X)-1); _req;})
#define erc_msg(msg,incVal,newVal) //do {DBG_DUMP("[ERC]%s : %d => %d\r\n",msg, (int)incVal, (int)newVal); incVal = newVal;}while(0)
#define assert(Y) do { if (!(Y)) {DBG_ERR("assert issue at line %d.\n", __LINE__); return -1;} }while(0)

#define validate assert

/*
static void dec_dump_buf(const unsigned char *buf, size_t len)
{
	size_t i;
	DBG_DUMP("buf(%08p, %08x):\r\n", buf, len);

	for (i = 0; i < len; i++) {
		DBG_DUMP("%02x ", *(buf + i));
	}
	DBG_DUMP("\r\n");
}
*/

static void parseVUI(H265DecSpsObj *sps, bstream *pBs, H26XDEC_VUI *pVui)
{
	VUIP *vui = &sps->vui_parameters;

	vui->aspect_ratio_info_present_flag = vui_get_bits(pBs, 1, "aspect_ratio_info_present_flag");
	if (vui->aspect_ratio_info_present_flag) {
		vui->aspect_ratio_idc = vui_get_bits(pBs, 8, "aspect_ratio_idc");

		if (vui->aspect_ratio_idc == 255) {
			vui->sar_width = vui_get_bits(pBs, 16, "sar_width");
			vui->sar_height = vui_get_bits(pBs, 16, "sar_height");
		}
	}

    vui_get_bits(pBs, 1, "overscan_info_present_flag");

	vui->video_signal_type_present_flag = vui_get_bits(pBs, 1, "video_signal_type_present_flag");
	if (vui->video_signal_type_present_flag) {
		vui->video_format = vui_get_bits(pBs, 3, "video_format");
		vui->video_full_range_flag = vui_get_bits(pBs, 1, "video_full_range_flag");
		vui->colour_description_present_flag = vui_get_bits(pBs, 1, "colour_description_present_flag");
		if (vui->colour_description_present_flag) {
			vui->colour_primaries = vui_get_bits(pBs, 8, "colour_primaries");
			vui->transfer_characteristics = vui_get_bits(pBs, 8, "transfer_characteristics");
			vui->matrix_coeffs = vui_get_bits(pBs, 8, "matrix_coeffs");
			//VPS_DBG(VPS_LOG_SEI, "colour_primaries(%d), transfer_characteristics(%d), matrix_coeffs(%d)\n", vui->colour_primaries, vui->transfer_characteristics, vui->matrix_coeffs);
		}
	}

	vui_get_bits(pBs, 1, "chroma_loc_info_present_flag");

	vui_get_bits(pBs, 1, "neutral_chroma_indication_flag");
	/* When general_frame_only_constraint_flag is equal to 1, the value of field_seq_flag shall be equal to 0. */
	vui_get_bits(pBs, 1, "field_seq_flag");

	/*
	When frame_field_info_present_flag is present and either or both of the following conditions are true, frame_field_info_present_flag shall be equal to 1:
	-   field_seq_flag is equal to 1.
	-   general_progressive_source_flag is equal to 1 and general_interlaced_source_flag is equal to 1
	*/
	vui->frame_field_info_present_flag = vui_get_bits(pBs, 1, "frame_field_info_present_flag");
	vui->default_display_window_flag = vui_get_bits(pBs, 1, "default_display_window_flag");
	if (vui->default_display_window_flag) {
		vui->def_disp_win_left_offset = vui_get_uvlc(pBs, "def_disp_win_left_offset");
		vui->def_disp_win_right_offset = vui_get_uvlc(pBs, "def_disp_win_right_offset");
		vui->def_disp_win_top_offset = vui_get_uvlc(pBs, "def_disp_win_top_offset");
		vui->def_disp_win_bottom_offset = vui_get_uvlc(pBs, "def_disp_win_bottom_offset");
	}

	vui->vui_timing_info_present_flag = vui_get_bits(pBs, 1, "vui_timing_info_present_flag");
	if (vui->vui_timing_info_present_flag) {
		vui->vui_num_units_in_tick = vui_get_bits(pBs, 32, "vui_num_units_in_tick");
		vui->vui_time_scale = vui_get_bits(pBs, 32, "vui_time_scale");
		vui->vui_poc_proportional_to_timing_flag = vui_get_bits(pBs, 1, "vui_poc_proportional_to_timing_flag");
		if (vui->vui_poc_proportional_to_timing_flag) {
			vui->vui_num_ticks_poc_diff_one_minus1 = vui_get_uvlc(pBs, "vui_num_ticks_poc_diff_one_minus1");
		}

        vui_get_bits(pBs, 1, "vui_hrd_parameters_present_flag");
	} else {
	}

	vui->bitstream_restriction_flag = vui_get_bits(pBs, 1, "bitstream_restriction_flag");
	if (vui->bitstream_restriction_flag) {
		vui->tiles_fixed_structure_flag = vui_get_bits(pBs, 1, "tiles_fixed_structure_flag");
		vui->motion_vectors_over_pic_boundaries_flag = vui_get_bits(pBs, 1, "motion_vectors_over_pic_boundaries_flag");
		vui->restricted_ref_pic_lists_flag = vui_get_bits(pBs, 1, "restricted_ref_pic_lists_flag");
		vui->min_spatial_segmentation_idc = vui_get_uvlc(pBs, "min_spatial_segmentation_idc");
		vui->max_bytes_per_pic_denom = vui_get_uvlc(pBs, "max_bytes_per_pic_denom");
		vui->max_bits_per_min_cu_denom = vui_get_uvlc(pBs, "max_bits_per_min_cu_denom");
		vui->log2_max_mv_length_horizontal = vui_get_uvlc(pBs, "log2_max_mv_length_horizontal");
		vui->log2_max_mv_length_vertical = vui_get_uvlc(pBs, "log2_max_mv_length_vertical");
	}

	pVui->uiSarWidth = vui->sar_width;
	pVui->uiSarHeight = vui->sar_height;
	pVui->ucMatrixCoeff = vui->matrix_coeffs;
	pVui->ucTransChar = vui->transfer_characteristics;
	pVui->ucColorPrimaries = vui->colour_primaries;
	pVui->ucVideoFmt = vui->video_format;
	pVui->ucColorRange = vui->video_full_range_flag;
	pVui->bTimingPresentFlag = vui->vui_timing_info_present_flag;
}

void parsePTL(int profilePresentFlag, int maxNumSubLayersMinus1, PTL *pptl, bstream *pBs)
{
	int i, j;

	pptl->general.profile_present_flag = profilePresentFlag;
	if (profilePresentFlag) {
		ptl_get_bits(pBs, 2, "general_profile_space");
		pptl->general.tier_flag = ptl_get_bits(pBs, 1, "general_tier_flag");
		pptl->general.profile_idc = ptl_get_bits(pBs, 5, "general_profile_idc");

		pptl->general.profile_compatibility_flag = 0;
		for (j = 0; j < 32; j++) {
			pptl->general.profile_compatibility_flag |= ptl_get_bits(pBs, 1, "general_profile_compatibility_flag[%d]", j) << j;
		}

		ptl_get_bits(pBs, 1, "general_progressive_source_flag");
		ptl_get_bits(pBs, 1, "general_interlaced_source_flag");
		ptl_get_bits(pBs, 1, "general_non_packed_constraint_flag");
		ptl_get_bits(pBs, 1, "general_frame_only_constraint_flag");
        
        ptl_skip_bits(pBs, 16, "general_reserved_zero_43bits[0..15]");
        ptl_skip_bits(pBs, 16, "general_reserved_zero_43bits[16..31]");
        ptl_skip_bits(pBs, 11, "general_reserved_zero_43bits[32..42]");

        ptl_get_bits(pBs, 1, "general_inbld_flag");

	}

	pptl->general.level_present_flag = 1;
	pptl->general.level_idc = ptl_get_bits(pBs, 8, "general_level_idc");
	//VPS_DBG(VPS_LOG_SPS, "general_level_idc = %d\n", pptl->general.level_idc);

	for (i = 0; i < maxNumSubLayersMinus1; i++) {
		ptl_get_bits(pBs, 1, "sub_layer_profile_present_flag[%d]", i);
		ptl_get_bits(pBs, 1, "sub_layer_level_present_flag[%d]", i);
	}

	if (maxNumSubLayersMinus1 > 0) {
		for (i = maxNumSubLayersMinus1; i < 8; i++) {
			ptl_skip_bits(pBs, 2, "reserved_zero_2bits");
		}
	}

}
int parseSTRPS(int idxRps, int num_short_term_ref_pic_sets, STRPS *rps, bstream *pBs, H265DecSpsObj *sps)
{
	//int inter_ref_pic_set_prediction_flag = 0;
	int i;

	if (idxRps != 0) {
		//inter_ref_pic_set_prediction_flag = strps_get_bits(pBs, 1, "inter_ref_pic_set_prediction_flag");
		strps_get_bits(pBs, 1, "inter_ref_pic_set_prediction_flag");
	}

	rps->curr_num_delta_pocs = 0;
	rps->UsedByCurrPicS0 = rps->UsedByCurrPicS1 = 0;

	{
		int poc, delta;
		u32 NumNegativePics, NumPositivePics;

		NumNegativePics = strps_get_uvlc(pBs, "num_negative_pics");
		assert(NumNegativePics <= MAX_HEVC_RLIST_ENTRY - 1);
		assert(NumNegativePics <= sps->sps_max_dec_pic_buffering_minus1[sps->sps_max_sub_layers_minus1] + 1); //relax the constraint
		rps->NumNegativePics = NumNegativePics;

		NumPositivePics = strps_get_uvlc(pBs, "num_positive_pics");
		assert(NumPositivePics <= MAX_HEVC_RLIST_ENTRY - 1 - NumNegativePics);
		assert(NumPositivePics <= (sps->sps_max_dec_pic_buffering_minus1[sps->sps_max_sub_layers_minus1] + 1 - NumNegativePics)); //relax the constraint

		poc = 0;
		for (i = 0; i < (int)NumNegativePics; i++) {
			delta = strps_get_uvlc(pBs, "delta_poc_s0_minus1[%d]", i);
			poc = poc - (delta + 1);
			rps->DeltaPocS0[i] = poc;
			rps->UsedByCurrPicS0 |= (strps_get_bits(pBs, 1, "used_by_curr_pic_s0_flag[%d]", i) << i);
		}

		poc = 0;
		for (i = 0; i < (int)NumPositivePics; i++) {
			delta = strps_get_uvlc(pBs, "delta_poc_s1_minus1[%d]", i);
			poc = poc + (delta + 1);
			rps->UsedByCurrPicS1 |= (strps_get_bits(pBs, 1, "used_by_curr_pic_s1_flag[%d]", i) << i);
		}
    	rps->NumDeltaPocs = rps->NumNegativePics + NumPositivePics; //(7-57)
	}

	return 0;
}


UINT32 H265DecNal(bstream *pBstream, H265DecSliceObj *slice)
{
	UINT32 uiVal;
	UINT32 nal;

	DBG_IND("=========== Nal Unit ===========\r\n");

	uiVal = get_bits(pBstream, 24); //start code

	if (uiVal == 0x000000) {
		uiVal = get_bits(pBstream, 8); //start code
	}

	if (uiVal != 1) {  //if ( uiVal != 0x00000001 || uiVal != 0x000001){
		DBG_ERR("[H265DecNal]StartCodec Suffix != 0x000001 (0x%08x)\r\n", uiVal);
		return H265DEC_FAIL;
	}

	get_bits(pBstream, 1);//skip_bits //forbidden_zero_bit
	nal = nal_get_bits(pBstream,    6,  "nal_unit_type");
	nal_get_bits(pBstream,  6,  "nuh_layer_id");
	uiVal = nal_get_bits(pBstream,  3,  "nuh_temporal_id_plus1");
	if (slice) {
		slice->uiTemporalId = uiVal - 1;
	}
	//assert(uiVal == 1); //assume temporal_id = 0

	return nal;

}
INT32 H265DecVps(H265DecVpsObj *vps, bstream *pBs)
{
	UINT32 uiVal;
	int video_parameter_set_id, i, j;
	int subLayerOrderingInfoPresentFlag;

	uiVal = H265DecNal(pBs, NULL);
	if (uiVal != NAL_UNIT_VPS) {
		DBG_ERR("[H265DecVps]Nal Unit Type(%d) != NALU_TYPE_VPS\r\n", (int)(uiVal));
		return H265DEC_FAIL;
	}
	DBG_IND("=========== Video Parameter Set ID ===========\r\n");
	video_parameter_set_id = vps_get_bits(pBs, 4, "vps_video_parameter_set_id");
	if (video_parameter_set_id >= MAX_VPS_NUM) {
		DBG_ERR("video_parameter_set_id(%d)", (int)(video_parameter_set_id));
	}
	vps->vps_base_layer_internal_flag = vps_get_bits(pBs, 1, "vps_base_layer_internal_flag");
	assert(vps->vps_base_layer_internal_flag == 1);
	vps->vps_base_layer_available_flag = vps_get_bits(pBs, 1, "vps_base_layer_available_flag");
	assert(vps->vps_base_layer_available_flag == 1);
	vps->vps_max_layers_minus1 = vps_get_bits(pBs, 6, "vps_max_layers_minus1");

	//The bitstream shall contain exactly two layers, a base layer and an enhancement layer,
	//and the value of vps_max_layers_minus1 of each VPS shall be set equal to 1
	assert(vps->vps_max_layers_minus1 <= 1);

	vps->MaxLayersMinus1 = (vps->vps_max_layers_minus1 < 62) ? vps->vps_max_layers_minus1 : 62;
	vps->vps_max_sub_layers_minus1 = vps_get_bits(pBs, 3, "vps_max_sub_layers_minus1");
	//assert(vps->vps_max_sub_layers_minus1 < MAX_VPS_SUB_LAYER_SIZE);
	vps->vps_temporal_id_nesting_flag = vps_get_bits(pBs, 1, "vps_temporal_id_nesting_flag");

	vps_get_bits(pBs, 16, "vps_reserved_0xffff_16bits");

	//vps->vps_profile_present_flag[0] = 1;
	//parsePTL(vps->vps_profile_present_flag[0], vps->vps_max_sub_layers_minus1, &vps->profile_tier_level[0], pBs);
	parsePTL(1, vps->vps_max_sub_layers_minus1, &vps->profile_tier_level[0], pBs);
	subLayerOrderingInfoPresentFlag = vps_get_bits(pBs, 1, "vps_sub_layer_ordering_info_present_flag");

	for (i = 0; i <= vps->vps_max_sub_layers_minus1; i++) {
		vps->vps_max_dec_pic_buffering_minus1[i] = vps_get_uvlc(pBs, "vps_max_dec_pic_buffering_minus1[%d]", i);
		vps->vps_max_num_reorder_pics[i] = vps_get_uvlc(pBs, "vps_max_num_reorder_pics[%d]", i);
		// coverity[overflow_const]
		vps->vps_max_latency_increase_plus1[i] = vps_get_uvlc(pBs, "vps_max_latency_increase_plus1[%d]", i);

		assert(vps->vps_max_dec_pic_buffering_minus1[i] < MAX_HEVC_RLIST_ENTRY);
		assert(vps->vps_max_num_reorder_pics[i] <= vps->vps_max_dec_pic_buffering_minus1[i]);

		if (!subLayerOrderingInfoPresentFlag) {
			for (i++; i <= vps->vps_max_sub_layers_minus1; i++) {
				vps->vps_max_dec_pic_buffering_minus1[i] = vps->vps_max_dec_pic_buffering_minus1[0];
				vps->vps_max_num_reorder_pics[i] = vps->vps_max_num_reorder_pics[0];
				vps->vps_max_latency_increase_plus1[i] = vps->vps_max_latency_increase_plus1[0];
			}
			break;
		}
	}
	vps->vps_max_layer_id = vps_get_bits(pBs, 6, "vps_max_layer_id");
	assert(vps->vps_max_layer_id  <  MAX_NUM_LAYER_IDS);

	vps->vps_num_layer_sets_minus1 = vps_get_uvlc(pBs, "vps_num_layer_sets_minus1");
	vps->NumLayerSets = vps->vps_num_layer_sets_minus1 + 1;
	assert(vps->NumLayerSets <= MAX_VPS_LAYER_SETS_PLUS1);

	// for Layer set 0
	vps->NumLayersInIdList[0] = 1;
	vps->LayerSetLayerIdList[0][0] = 0;

	for (i = 1; i <= (int) vps->vps_num_layer_sets_minus1; i++) {
		int n = 0;
		for (j = 0; j <= vps->vps_max_layer_id; j++) {
			int layer_id_included_flag = vps_get_bits(pBs, 1, "layer_id_included_flag[%d][%d]", i, j);
			if (layer_id_included_flag) {
				assert(n < MAX_NUM_ATSC_3_0_LAYERS);
				vps->LayerSetLayerIdList[i][n++] = j;
			}
		}
		vps->NumLayersInIdList[i] = n;
	}

	vps->vps_timing_info_present_flag = vps_get_bits(pBs, 1, "vps_timing_info_present_flag");

	if (vps->vps_timing_info_present_flag) {
		vps->vps_num_units_in_tick = vps_get_bits(pBs, 32, "vps_num_units_in_tick");
		vps->vps_time_scale = vps_get_bits(pBs, 32, "vps_time_scale");
		vps->vps_poc_proportional_to_timing_flag = vps_get_bits(pBs, 1, "vps_poc_proportional_to_timing_flag");

		if (vps->vps_poc_proportional_to_timing_flag) {
			vps->vps_num_ticks_poc_diff_one_minus1 = vps_get_uvlc(pBs, "vps_num_ticks_poc_diff_one_minus1");
		}

		vps->vps_num_hrd_parameters = vps_get_uvlc(pBs, "vps_num_hrd_parameters");
		assert(vps->vps_num_hrd_parameters <= vps->vps_num_layer_sets_minus1 + 1);

		for (i = 0; i < (int) vps->vps_num_hrd_parameters; i++) {
#if 0
			int cprms_present_flag = 1;

			vps_get_uvlc(pBs, "hrd_layer_set_idx[%d]", i);

			if (i > 0) {
				cprms_present_flag = vps_get_bits(pBs, 1, "cprms_present_flag[%d]", i);
			}

			parseHRD(cprms_present_flag, vps->vps_max_sub_layers_minus1, &vps->hrd_parameters, pBs);
#else
			assert(0);//TODO, parseHRD
#endif
		}
	}

	vps->vps_extension_flag = vps_get_bits(pBs, 1, "vps_extension_flag");

	// When MaxLayersMinus1 is greater than 0, vps_extension_flag shall be equal to 1.
	if (vps->MaxLayersMinus1 > 0) {
		assert(vps->vps_extension_flag == 1);
	}

	if (vps->vps_extension_flag) {
#if 0
		while ((bit_length(pBs) % 8) != 0) {
			int needs = 8 - bit_length(pBs) % 8;
			int code;

			code = vps_get_bits(pBs, needs, "vps_extension_alignment_bit_equal_to_one");
			assert(code, ==, ((1 << needs) - 1));
		}

		assert(parseVPSExtension(vps, pBs), ==, 0);
		vps->vps_extension2_flag = vps_get_bits(pBs, 1, "vps_extension2_flag");
		if (vps->vps_extension2_flag) {
			while (bit_remainder(pBs) > 0) {
				vps_get_bits(pBs, 1, "vps_extension_data_flag");
			}
		}
#else
		assert(0);//TODO, parseVPSExtension
#endif
	} else {
		//TODO //defaultVPSExtension(vps, pBs);
	}

	//assert(checkOutputLayerSet(vps, hevc->targetLayerId, &hevc->info), ==, 0);
	//assert(derivedSmallestLayerId(vps, hevc->info.TargetOlsIdx, &hevc->info), ==, 0);
	//assert(derivedLayerInfo(vps, hevc->info.TargetOlsIdx, &hevc->info), ==, 0);

	vps->valid = 1;

#if !ERC
	assert(bit_length(pBs) > 0);
#endif

	read_rbsp_trailing_bits(pBs);


	return H265DEC_SUCCESS;
}

INT32 H265DecSps(H265DecSpsObj *sps, H265DecVpsObj *vps, bstream *pBs, int nuh_layer_id, H26XDEC_VUI *pVui )
{

	UINT32 uiVal;
	int i;
	int PicSizeInSamplesY, valid = 1;
	int sps_video_parameter_set_id = 0;
	int sps_max_sub_layers_minus1, sps_ext_or_max_sub_layers_minus1;
	int sps_temporal_id_nesting_flag;
	//int nuh_layer_id = 0;//sps->header.nal_layer_id;
	int MultiLayerExtSpsFlag;
	//H265DecVpsObj *vps;

	uiVal = H265DecNal(pBs, NULL);

	if (uiVal != NAL_UNIT_SPS) {
		DBG_ERR("[H265DecSps]Nal Unit Type(%d) != NALU_TYPE_SPS\r\n", (int)(uiVal));
		//vps = &vps0[sps_video_parameter_set_id]; //fix CIM
		return H265DEC_FAIL;
	}
	DBG_IND("=========== Sequence Parameter Set ID ===========\r\n");


	sps_video_parameter_set_id = sps_get_bits(pBs, 4, "sps_video_parameter_set_id");
	if (sps_video_parameter_set_id >= MAX_VPS_NUM) {
		DBG_IND("sps_video_parameter_set_id(%d)\r\n", (int)(sps_video_parameter_set_id));
	}
	//vps = &vps0[sps_video_parameter_set_id];

	if (nuh_layer_id == 0) {
		sps_max_sub_layers_minus1 = sps_get_bits(pBs, 3, "sps_max_sub_layers_minus1");
	} else {
		sps_ext_or_max_sub_layers_minus1 = sps_get_bits(pBs, 3, "sps_ext_or_max_sub_layers_minus1");
		/* When not present, the value of sps_max_sub_layers_minus1 is inferred to be equal to ( sps_ext_or_max_sub_layers_minus1 = = 7 ) ? vps_max_sub_layers_minus1 : sps_ext_or_max_sub_layers_minus1. */
		sps_max_sub_layers_minus1 = (sps_ext_or_max_sub_layers_minus1 == 7) ? vps->vps_max_sub_layers_minus1 : sps_ext_or_max_sub_layers_minus1;
	}

	MultiLayerExtSpsFlag = (nuh_layer_id != 0 && sps_ext_or_max_sub_layers_minus1 == 7);
	if (!MultiLayerExtSpsFlag) {
		sps_temporal_id_nesting_flag = sps_get_bits(pBs, 1, "sps_temporal_id_nesting_flag");
		parsePTL(1, sps_max_sub_layers_minus1, &sps->profile_tier_level, pBs);
	} else {
		assert(vps->valid == 1);
		/* If sps_max_sub_layers_minus1 is greater than 0, the value of sps_temporal_id_nesting_flag is inferred to be equal to vps_temporal_id_nesting_flag. */
		sps_temporal_id_nesting_flag = (sps_max_sub_layers_minus1 > 0) ? vps->vps_temporal_id_nesting_flag : 1;
	}

	sps_get_uvlc(pBs, "sps_seq_parameter_set_id");

	sps->valid = 0;
	sps->sps_video_parameter_set_id = sps_video_parameter_set_id;
	sps->sps_max_sub_layers_minus1 = sps_max_sub_layers_minus1;
	sps->sps_temporal_id_nesting_flag = sps_temporal_id_nesting_flag;

	if (MultiLayerExtSpsFlag) {
		assert(0);//TODO
#if 0
		int repFormatIdx;
		RF *rep_format;
		int update_rep_format_flag = sps_get_bits(pBs, 1, "update_rep_format_flag");

		if (update_rep_format_flag) {
			repFormatIdx = sps_get_bits(pBs, 8, "sps_rep_format_idx");
		} else {
			repFormatIdx = vps->vps_rep_format_idx[vps->LayerIdxInVps[nuh_layer_id]];
		}

		rep_format = &vps->rep_format[repFormatIdx];

		sps->chroma_format_idc = rep_format->chroma_format_vps_idc;
		sps->separate_colour_plane_flag = rep_format->separate_colour_plane_vps_flag;
		sps->pic_width_in_luma_samples = rep_format->pic_width_vps_in_luma_samples;
		sps->pic_height_in_luma_samples = rep_format->pic_height_vps_in_luma_samples;
		sps->bit_depth_luma_minus8 = rep_format->bit_depth_vps_luma_minus8;
		sps->bit_depth_chroma_minus8 = rep_format->bit_depth_vps_chroma_minus8;
		if (rep_format->conformance_window_vps_flag) {
			sps->conf_win_top_offset = rep_format->conf_win_vps_top_offset;
			sps->conf_win_bottom_offset = rep_format->conf_win_vps_bottom_offset;
			sps->conf_win_left_offset = rep_format->conf_win_vps_left_offset;
			sps->conf_win_right_offset = rep_format->conf_win_vps_right_offset;
		} else {
			sps->conf_win_top_offset = 0;
			sps->conf_win_bottom_offset = 0;
			sps->conf_win_left_offset = 0;
			sps->conf_win_right_offset = 0;
		}
#endif
	} else {
		sps->chroma_format_idc = sps_get_uvlc(pBs, "chroma_format_idc");

		if (sps->chroma_format_idc == 3) {
			sps->separate_colour_plane_flag = sps_get_bits(pBs, 1, "separate_colour_plane_flag");
		} else {
			sps->separate_colour_plane_flag = 0;
		}

		sps->pic_width_in_luma_samples = sps_get_uvlc(pBs, "pic_width_in_luma_samples");
		sps->pic_height_in_luma_samples = sps_get_uvlc(pBs, "pic_height_in_luma_samples");

		if (sps_get_bits(pBs, 1, "conformance_window_flag")) {
			sps->conf_win_left_offset = sps_get_uvlc(pBs, "conf_win_left_offset");
			sps->conf_win_right_offset = sps_get_uvlc(pBs, "conf_win_right_offset");

			sps->conf_win_top_offset = sps_get_uvlc(pBs, "conf_win_top_offset");
			sps->conf_win_bottom_offset = sps_get_uvlc(pBs, "conf_win_bottom_offset");
		} else {
			sps->conf_win_left_offset = 0;
			sps->conf_win_right_offset = 0;
			sps->conf_win_top_offset = 0;
			sps->conf_win_bottom_offset = 0;
		}

		sps_get_uvlc(pBs, "bit_depth_luma_minus8");
		sps_get_uvlc(pBs, "bit_depth_chroma_minus8");
	}

	assert(sps->chroma_format_idc <= 3);
	assert(sps->pic_width_in_luma_samples >= 16);
	assert(sps->pic_height_in_luma_samples >= 16);
	assert((u32)(sps->conf_win_left_offset + sps->conf_win_right_offset) * SubWidthC[sps->chroma_format_idc] < sps->pic_width_in_luma_samples);
	assert((u32)(sps->conf_win_top_offset + sps->conf_win_bottom_offset) * SubHeightC[sps->chroma_format_idc] < sps->pic_height_in_luma_samples);
	validate(sps->pic_width_in_luma_samples <= MAX_HEVC_WIDTH);
	validate(sps->pic_height_in_luma_samples <= MAX_HEVC_HEIGHT);
	//validate(sps->bit_depth_luma_minus8 <= 2);
	//validate(sps->bit_depth_chroma_minus8 <= 2);

	PicSizeInSamplesY = sps->pic_width_in_luma_samples * sps->pic_height_in_luma_samples;

	sps->log2_max_pic_order_cnt_lsb_minus4 = sps_get_uvlc(pBs, "log2_max_pic_order_cnt_lsb_minus4");
	assert(sps->log2_max_pic_order_cnt_lsb_minus4 <= 12);

	if (!MultiLayerExtSpsFlag) {
		int sps_sub_layer_ordering_info_present_flag = sps_get_bits(pBs, 1, "sps_sub_layer_ordering_info_present_flag");

		for (i = 0; i <= (int)sps->sps_max_sub_layers_minus1; i++) {
			sps->sps_max_dec_pic_buffering_minus1[i] = sps_get_uvlc(pBs, "sps_max_dec_pic_buffering_minus1[%d]", i);
			sps->sps_max_num_reorder_pics[i] = sps_get_uvlc(pBs, "sps_max_num_reorder_pics[%d]", i);
			sps->sps_max_latency_increase_plus1[i] = sps_get_uvlc(pBs, "sps_max_latency_increase_plus1[%d]", i);

			//VPS_DBG(VPS_LOG_SPS, "sps_max_dec_pic_buffering_minus1[%d]=%d\n", i , sps->sps_max_dec_pic_buffering_minus1[i]);

			if (!sps_sub_layer_ordering_info_present_flag) {
				for (i++; i <= sps->sps_max_sub_layers_minus1; i++) {
					sps->sps_max_dec_pic_buffering_minus1[i] = sps->sps_max_dec_pic_buffering_minus1[0];
					sps->sps_max_num_reorder_pics[i] = sps->sps_max_num_reorder_pics[0];
					sps->sps_max_latency_increase_plus1[i] = sps->sps_max_latency_increase_plus1[0];
				}
				break;
			}

		}
	} else {
		assert(0);//TODO
#if 0
		int TargetDecLayerSetIdx = hevc->info.TargetDecLayerSetIdx;
		int TargetOlsIdx = hevc->info.TargetOlsIdx;
		int layerIdx = -1;

		for (j = 0; j < (int)vps->NumLayersInIdList[TargetDecLayerSetIdx]; j++) {
			if (nuh_layer_id == vps->LayerSetLayerIdList[TargetDecLayerSetIdx][j]) {
				layerIdx = j;
				break;
			}
		}

		assert(layerIdx != -1);
		for (i = 0; i <= (int)sps->sps_max_sub_layers_minus1; i++) {
			sps->sps_max_dec_pic_buffering_minus1[i] = vps->max_vps_dec_pic_buffering_minus1[TargetOlsIdx][layerIdx][i];
			sps->sps_max_num_reorder_pics[i] = vps->max_vps_num_reorder_pics[TargetOlsIdx][i];
			sps->sps_max_latency_increase_plus1[i] = vps->max_vps_latency_increase_plus1[TargetOlsIdx][i];
		}
#endif
	}

	for (i = 0; i <= sps->sps_max_sub_layers_minus1; i++) {
		if (PicSizeInSamplesY <= (MaxLumaPs >> 2)) { /* refer to A-2 */
			validate(sps->sps_max_dec_pic_buffering_minus1[i] + 1 <= MAX_HEVC_RLIST_ENTRY);
		} else if (PicSizeInSamplesY <= (MaxLumaPs >> 1)) {
			validate(sps->sps_max_dec_pic_buffering_minus1[i] + 1 <= 12);
		} else if (PicSizeInSamplesY <= ((3 * MaxLumaPs) >> 2)) {
			validate(sps->sps_max_dec_pic_buffering_minus1[i] + 1 <= 8);
		} else {
			validate(sps->sps_max_dec_pic_buffering_minus1[i] + 1 <= 6);
		}

		assert(sps->sps_max_num_reorder_pics[i] <= sps->sps_max_dec_pic_buffering_minus1[i]);

		if (i > 0) {
			/* When i is greater than 0, sps_max_dec_pic_buffering_minus1[ i ] shall be greater than or equal to sps_max_dec_pic_buffering_minus1[ i - 1 ]. */
			assert(sps->sps_max_dec_pic_buffering_minus1[i] >= sps->sps_max_dec_pic_buffering_minus1[i - 1]);
			assert(sps->sps_max_num_reorder_pics[i] >= sps->sps_max_num_reorder_pics[i - 1]);
		}

		if (MultiLayerExtSpsFlag) {
			assert(sps->sps_max_dec_pic_buffering_minus1[i] <= vps->vps_max_dec_pic_buffering_minus1[i]);
			assert(sps->sps_max_num_reorder_pics[i] <= vps->vps_max_num_reorder_pics[i]);
		}
	}

	sps->log2_min_luma_coding_block_size_minus3 = sps_get_uvlc(pBs, "log2_min_luma_coding_block_size_minus3");
	sps->log2_diff_max_min_luma_coding_block_size = sps_get_uvlc(pBs, "log2_diff_max_min_luma_coding_block_size");
	sps->log2_min_transform_block_size_minus2 = sps_get_uvlc(pBs, "log2_min_luma_transform_block_size_minus2");
	sps->log2_diff_max_min_transform_block_size = sps_get_uvlc(pBs, "log2_diff_max_min_luma_transform_block_size");
	sps->max_transform_hierarchy_depth_inter = sps_get_uvlc(pBs, "max_transform_hierarchy_depth_inter");
	sps->max_transform_hierarchy_depth_intra = sps_get_uvlc(pBs, "max_transform_hierarchy_depth_intra");

	sps->scaling_list_enabled_flag = sps_get_bits(pBs, 1, "scaling_list_enabled_flag");
	if (sps->scaling_list_enabled_flag) {
		if (MultiLayerExtSpsFlag) {
			sps->sps_infer_scaling_list_flag = sps_get_bits(pBs, 1, "sps_infer_scaling_list_flag");
		} else {
			sps->sps_infer_scaling_list_flag = 0;
		}

		if (sps->sps_infer_scaling_list_flag) {
			sps->sps_scaling_list_ref_layer_id = sps_get_bits(pBs, 6, "sps_scaling_list_ref_layer_id");
			assert(sps->sps_scaling_list_ref_layer_id < MAX_NUM_ATSC_3_0_LAYERS);
		} else {
			sps->sps_scaling_list_data_present_flag = sps_get_bits(pBs, 1, "sps_scaling_list_data_present_flag");
			if (sps->sps_scaling_list_data_present_flag) {
				//TODO: parseSLD
				//if (0 > parseSLD(&sps->scaling_list_data, pBs))
				//    return -1;
			}
		}
	} else {
		sps->sps_infer_scaling_list_flag = 0;
		sps->sps_scaling_list_data_present_flag = 0;
	}

	sps->amp_enabled_flag = sps_get_bits(pBs, 1, "amp_enabled_flag");
	sps->sample_adaptive_offset_enabled_flag = sps_get_bits(pBs, 1, "sample_adaptive_offset_enabled_flag");
	sps_get_bits(pBs, 1, "pcm_enabled_flag");

	sps->num_short_term_ref_pic_sets = sps_get_uvlc(pBs, "num_short_term_ref_pic_sets");
	assert(sps->num_short_term_ref_pic_sets <= MAX_ST_RPS_SIZE);
	for (i = 0; i < (int)sps->num_short_term_ref_pic_sets; i++) {
		if (0 > parseSTRPS(i, sps->num_short_term_ref_pic_sets, &sps->short_term_ref_pic_set[i], pBs, sps)) {
			return -1;
		}
	}

	sps->long_term_ref_pics_present_flag = sps_get_bits(pBs, 1, "long_term_ref_pics_present_flag");
	if (sps->long_term_ref_pics_present_flag) {
		u32 bitsForPOC = 4 + sps->log2_max_pic_order_cnt_lsb_minus4;

		sps->num_long_term_ref_pics_sps = sps_get_uvlc(pBs, "num_long_term_ref_pics_sps");
		assert(sps->num_long_term_ref_pics_sps <= MAX_LT_RP_SIZE);

		sps->used_by_curr_pic_lt_sps_flag = 0;
		for (i = 0; i < (int)sps->num_long_term_ref_pics_sps; i++) {
			sps->lt_ref_pic_poc_lsb_sps[i] = sps_get_bits(pBs, bitsForPOC, "lt_ref_pic_poc_lsb_sps[%d]", i);
			sps->used_by_curr_pic_lt_sps_flag |= (sps_get_bits(pBs, 1, "used_by_curr_pic_lt_sps_flag[%d]", i) << i);
		}
	}

	sps->sps_temporal_mvp_enabled_flag = sps_get_bits(pBs, 1, "sps_temporal_mvp_enabled_flag");
	sps->strong_intra_smoothing_enabled_flag = sps_get_bits(pBs, 1, "strong_intra_smoothing_enabled_flag");
	sps->vui_parameters_present_flag = sps_get_bits(pBs, 1, "vui_parameters_present_flag");

	sps->vui_parameters.colour_primaries = 2;
	sps->vui_parameters.transfer_characteristics = 2;
	sps->vui_parameters.matrix_coeffs = 2;

	if (sps->vui_parameters_present_flag) {
		pVui->bPresentFlag = TRUE;
		parseVUI(sps, pBs, pVui);
	} else {
	}

	sps_get_bits(pBs, 1, "sps_extension_present_flag");

#if !ERC
	assert(bit_length(pBs) > 0);
#endif

	assert(sps->chroma_format_idc == 1);

	sps->valid = valid;

	read_rbsp_trailing_bits(pBs);


	return H265DEC_SUCCESS;

}

INT32 H265DecPps(H265DecPpsObj *pps, bstream *pBs)
{
	UINT32 uiVal;
	int pps_seq_parameter_set_id, i, temp;

	uiVal = H265DecNal(pBs, NULL);

	if (uiVal != NAL_UNIT_PPS) {
		DBG_ERR("[H265DecPps]Nal Unit Type(%d) != NALU_TYPE_PPS\r\n", (int)(uiVal));
		return H265DEC_FAIL;
	}

	DBG_IND("=========== Picture Parameter Set ID ===========\r\n");

	pps_get_uvlc(pBs, "pps_pic_parameter_set_id");

	pps_seq_parameter_set_id = pps_get_uvlc(pBs, "pps_seq_parameter_set_id");
	assert((unsigned int)pps_seq_parameter_set_id < MAX_SPS_NUM);

	pps->valid = 0;
	pps->pps_seq_parameter_set_id = pps_seq_parameter_set_id;

	pps->dependent_slice_segments_enabled_flag = pps_get_bits(pBs, 1, "dependent_slice_segments_enabled_flag");
	pps->output_flag_present_flag = pps_get_bits(pBs, 1, "output_flag_present_flag");
	pps->num_extra_slice_header_bits = pps_get_bits(pBs, 3, "num_extra_slice_header_bits");
	pps_get_bits(pBs, 1, "sign_data_hiding_enabled_flag");
	pps->cabac_init_present_flag = pps_get_bits(pBs, 1, "cabac_init_present_flag");
	pps->num_ref_idx_l0_default_active_minus1 = pps_get_uvlc(pBs, "num_ref_idx_l0_default_active_minus1");
	assert(pps->num_ref_idx_l0_default_active_minus1 < (MAX_HEVC_RLIST_ENTRY - 1));
	pps->num_ref_idx_l1_default_active_minus1 = pps_get_uvlc(pBs, "num_ref_idx_l1_default_active_minus1");
	assert(pps->num_ref_idx_l1_default_active_minus1 < (MAX_HEVC_RLIST_ENTRY - 1));
	pps->init_qp_minus26 = pps_get_svlc(pBs, "init_qp_minus26");
	pps_get_bits(pBs, 1, "constrained_intra_pred_flag");
	pps_get_bits(pBs, 1, "transform_skip_enabled_flag");
	pps->cu_qp_delta_enabled_flag = pps_get_bits(pBs, 1, "cu_qp_delta_enabled_flag");
	if (pps->cu_qp_delta_enabled_flag) {
		pps->diff_cu_qp_delta_depth = pps_get_uvlc(pBs, "diff_cu_qp_delta_depth");
	} else {
		pps->diff_cu_qp_delta_depth = 0;
	}

	temp = pps_get_svlc(pBs, "pps_cb_qp_offset");
	if (temp < -12) {
		erc_msg("pps_cb_qp_offset", temp, -12);
	} else if (temp > 12) {
		erc_msg("pps_cb_qp_offset", temp, 12);
	}

	pps->pps_cb_qp_offset = temp;

	temp = pps_get_svlc(pBs, "pps_cr_qp_offset");
	if (temp < -12) {
		erc_msg("pps_cr_qp_offset", temp, -12);
	} else if (temp > 12) {
		erc_msg("pps_cr_qp_offset", temp, 12);
	}
	pps->pps_cr_qp_offset = temp;

	pps->pps_slice_chroma_qp_offsets_present_flag = pps_get_bits(pBs, 1, "pps_slice_chroma_qp_offsets_present_flag");
	pps->weighted_pred_flag = pps_get_bits(pBs, 1, "weighted_pred_flag");
	pps_get_bits(pBs, 1, "weighted_bipred_flag");
	pps_get_bits(pBs, 1, "transquant_bypass_enabled_flag");
	pps->tiles_enabled_flag = pps_get_bits(pBs, 1, "tiles_enabled_flag");
	pps->entropy_coding_sync_enabled_flag = pps_get_bits(pBs, 1, "entropy_coding_sync_enabled_flag");

	pps->to_calculate_each_tile_boundary = 1; //for internal use

	if (pps->tiles_enabled_flag) {
		pps->num_tile_columns_minus1 = pps_get_uvlc(pBs, "num_tile_columns_minus1");
		if (pps->num_tile_columns_minus1 >= MaxTileCols) {
			erc_msg("num_tile_columns_minus1", pps->num_tile_columns_minus1, MaxTileCols - 1);
		}

		pps->num_tile_rows_minus1 = pps_get_uvlc(pBs, "num_tile_rows_minus1");
		if (pps->num_tile_rows_minus1 >= MaxTileRows) {
			erc_msg("num_tile_rows_minus1", pps->num_tile_rows_minus1, MaxTileRows - 1);
		}

		pps->uniform_spacing_flag = pps_get_bits(pBs, 1, "uniform_spacing_flag");
		if (!pps->uniform_spacing_flag) {
			for (i = 0; i < (int)pps->num_tile_columns_minus1; i++) {
				pps->column_width[i] = pps_get_uvlc(pBs, "column_width_minus1[%d]", i) + 1;
			}
			for (i = 0; i < (int)pps->num_tile_rows_minus1; i++) {
				pps->row_height[i] = pps_get_uvlc(pBs, "row_height_minus1[%d]", i) + 1;
			}
		}

		pps->loop_filter_across_tiles_enabled_flag = pps_get_bits(pBs, 1, "loop_filter_across_tiles_enabled_flag");
	} else {
		pps->uniform_spacing_flag = 1;
		pps->num_tile_columns_minus1 = 0;
		pps->num_tile_rows_minus1 = 0;
		pps->loop_filter_across_tiles_enabled_flag = 1;
	}

	pps->pps_loop_filter_across_slices_enabled_flag = pps_get_bits(pBs, 1, "pps_loop_filter_across_slices_enabled_flag");
	pps->deblocking_filter_control_present_flag = pps_get_bits(pBs, 1, "deblocking_filter_control_present_flag");

	if (pps->deblocking_filter_control_present_flag) {
		pps->deblocking_filter_override_enabled_flag = pps_get_bits(pBs, 1, "deblocking_filter_override_enabled_flag");
		pps->pps_deblocking_filter_disabled_flag = pps_get_bits(pBs, 1, "pps_deblocking_filter_disabled_flag");

		if (!pps->pps_deblocking_filter_disabled_flag) {
			int offset_div2;

			offset_div2 = pps_get_svlc(pBs, "pps_beta_offset_div2");
			if (offset_div2 < -6) {
				erc_msg("pps_beta_offset_div2", offset_div2, -6);
			} else if (offset_div2 > 6) {
				erc_msg("pps_beta_offset_div2", offset_div2, 6);
			}
			pps->pps_beta_offset_div2 = offset_div2;

			offset_div2 = pps_get_svlc(pBs, "pps_tc_offset_div2");
			if (offset_div2 < -6) {
				erc_msg("pps_tc_offset_div2", offset_div2, -6);
			} else if (offset_div2 > 6) {
				erc_msg("pps_tc_offset_div2", offset_div2, 6);
			}
			pps->pps_tc_offset_div2 = offset_div2;
		} else {
			pps->pps_beta_offset_div2 = 0;
			pps->pps_tc_offset_div2 = 0;
		}
	} else {
		pps->deblocking_filter_override_enabled_flag = 0;
		pps->pps_deblocking_filter_disabled_flag = 0;
		pps->pps_beta_offset_div2 = 0;
		pps->pps_tc_offset_div2 = 0;
	}

	pps_get_bits(pBs, 1, "pps_scaling_list_data_present_flag");

	pps->lists_modification_present_flag = pps_get_bits(pBs, 1, "lists_modification_present_flag");
	pps_get_uvlc(pBs, "log2_parallel_merge_level_minus2");
	pps->slice_segment_header_extension_present_flag = pps_get_bits(pBs, 1, "slice_segment_header_extension_present_flag");

	pps_get_bits(pBs, 1, "pps_extension_present_flag");

#if !ERC
	assert(bit_length(pBs) > 0);
#endif

	pps->valid = 1;

	read_rbsp_trailing_bits(pBs);

	return H265DEC_SUCCESS;

}

static inline int getRapPicFlag(UINT32 type)
{
	return (type >= NAL_UNIT_CODED_SLICE_BLA_W_LP && type <= NAL_UNIT_RESERVED_IRAP_VCL23);
}
static inline int isIDR(UINT32 type)
{
	return type == NAL_UNIT_CODED_SLICE_IDR_W_RADL
		   || type == NAL_UNIT_CODED_SLICE_IDR_N_LP;
}
static inline int isBLA(UINT32 type)
{
	return type == NAL_UNIT_CODED_SLICE_BLA_W_LP
		   || type == NAL_UNIT_CODED_SLICE_BLA_W_RADL
		   || type == NAL_UNIT_CODED_SLICE_BLA_N_LP;
}

static inline int isCRA(UINT32 type)
{
	return type == NAL_UNIT_CODED_SLICE_CRA;
}
static inline int isIntra(UINT32 slice_type)
{
	return slice_type == I_SLICE;
}

#if 0
static inline int isInterB(UINT32 slice_type)
{
	return slice_type == B_SLICE;
}
#endif

static inline int isInterP(UINT32 slice_type)
{
	return slice_type == P_SLICE;
}
UINT32 getNumRpsCurrTempList(H265DecSliceObj *slice)  //201504-I, (F-56)
{
	int NumPocTotalCurr = 0;
	UINT32 i;

	for (i = 0; i < slice->short_term_ref_pic_set.NumNegativePics; i++) {
		if (slice->short_term_ref_pic_set.UsedByCurrPicS0 & (1 << i)) {
			NumPocTotalCurr++;
		}
	}

	for (i = 0; i < slice->num_long_term_ref_pics; i++) {
		if ((slice->UsedByCurrPicLt >> i) & 1) {
			NumPocTotalCurr++;
		}
	}

	return NumPocTotalCurr + slice->NumActiveRefLayerPics;
}
void parseRPLM(UINT32 type, H265DecSliceObj *slice, bstream *pBs)
{
	UINT32 i;
	int reqBits, numRpsCurrTempList;

	numRpsCurrTempList = getNumRpsCurrTempList(slice);
	reqBits = MinReqBits(numRpsCurrTempList);

	slice->ref_pic_list_modification_flag_l0 = rplm_get_bits(pBs, 1, "ref_pic_list_modification_flag_l0");
	if (slice->ref_pic_list_modification_flag_l0) {
		if (reqBits != 0) {
			for (i = 0; i < slice->num_ref_idx_l0_active; i++) {
				slice->list_entry_l0[i] = rplm_get_bits(pBs, reqBits, "list_entry_l0[%d]", i);
			}
		} else {
			for (i = 0; i < slice->num_ref_idx_l0_active; i++) {
				slice->list_entry_l0[i] = 0;
			}
		}
	}
}
int calculateLenOfSyntaxElement(int numVal)
{
	int numBits = 1;
	while ((1 << numBits) < numVal) {
		numBits++;
	}

	return numBits;
}

int readByteAlignment(bstream *const bs)
{
	UINT32 code;
	UINT32 remainder;

	code = get_bits(bs, 1);
	assert(code == 1);

	remainder = bs->offset % 8;
	if (remainder) {
		code = get_bits(bs, 8 - remainder);
		assert(code == 0);
	}

	return 0;
}

INT32 H265DecSlice(H265DEC_INFO *pH265DecInfo, H265DecHdrObj *pH265DecHdrObj, UINT32 uiBsAddr, UINT32 uiBsLen, 
                    int nuh_layer_id)
{
	H265DecSpsObj   *sps   = &pH265DecHdrObj->sH265DecSpsObj;
	H265DecPpsObj   *pps   = &pH265DecHdrObj->sH265DecPpsObj;
	H265DecVpsObj   *vps   = &pH265DecHdrObj->sH265DecVpsObj;
	H265DecSliceObj *slice = &pH265DecHdrObj->sH265DecSliceObj;
	bstream sBstream;
	bstream *pBs = &sBstream;
	UINT32 uiHeaderSize;
	UINT32 i;
	//UINT32 PicHeightInCtbsY;
	//INT32 CtbSizeY;
	INT32 slice_pic_order_cnt_lsb, MaxPicOrderCntLsb, inter_layer_se_bits = 0;
	UINT32 type;
    UINT32 dependent_slice_segment_flag;

	uiHeaderSize = uiBsLen;
	if (uiHeaderSize > DecNalBufSize) {
		uiHeaderSize = DecNalBufSize;
	}
	uiHeaderSize = ebspTorbsp((UINT8 *)uiBsAddr, pH265DecHdrObj->ucH265DecNalBuf, uiHeaderSize);
	if (uiHeaderSize > DecNalBufSize) {
		uiHeaderSize = DecNalBufSize;
	}
	init_parse_bitstream(pBs, (void *)pH265DecHdrObj->ucH265DecNalBuf, uiHeaderSize);
	//dec_dump_buf((const unsigned char *)ucH265DecNalBuf, uiHeaderSize);

	type = H265DecNal(pBs, slice);
	slice->nalType = type;
	if ((slice->nalType != NAL_UNIT_CODED_SLICE_TRAIL_R)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_TRAIL_N)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_TSA_R)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_TSA_N)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_STSA_R)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_STSA_N)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_BLA_W_LP)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_BLA_W_RADL)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_BLA_N_LP)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_IDR_W_RADL)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_IDR_N_LP)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_CRA)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_RADL_R)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_RASL_R)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_RADL_N)
		&& (slice->nalType != NAL_UNIT_CODED_SLICE_RASL_N)
	   ) {
		DBG_ERR("[H265DecSlice]Nal Unit Type != NALU_TYPE_IDR or NALU_TYPE_SLICE %d \r\n", (int)(type));
		return H265DEC_FAIL;
	}
#if 0	
#if DEBUG
	DBG_DUMP("=========== Slice ===========\r\n");
#endif
#endif

	slice->valid = 0;
	slice->first_slice_segment_in_pic_flag = slice_get_bits(pBs, 1, "first_slice_segment_in_pic_flag");

	if (getRapPicFlag(type)) {
        slice_get_bits(pBs, 1, "no_output_of_prior_pics_flag");
	}

	slice_get_uvlc(pBs, "slice_pic_parameter_set_id");
	//pps = &hevc->layer[nuh_layer_id].stPPS[slice->slice_pic_parameter_set_id];
	assert(pps->valid != 0);
	assert(pps->pps_seq_parameter_set_id < MAX_SPS_NUM); //if this condition is truth, it seems memory is corrupted!!
	//sps = &hevc->layer[nuh_layer_id].stSPS[pps->pps_seq_parameter_set_id];
	assert(sps->valid != 0);
	assert(sps->sps_video_parameter_set_id < MAX_VPS_NUM);
	//vps = &hevc->stVPS[sps->sps_video_parameter_set_id];
	//assert(vps->valid, !=, 0);

	//CtbSizeY = 1 << (sps->log2_min_luma_coding_block_size_minus3 + 3 + sps->log2_diff_max_min_luma_coding_block_size);
	//PicHeightInCtbsY = (sps->pic_height_in_luma_samples + CtbSizeY - 1) / CtbSizeY;

	if (pps->dependent_slice_segments_enabled_flag && !slice->first_slice_segment_in_pic_flag) {
		dependent_slice_segment_flag = slice_get_bits(pBs, 1, "dependent_slice_segment_flag");
	} else {
		dependent_slice_segment_flag = 0;
	}

	if (!slice->first_slice_segment_in_pic_flag) {
		int CUWidth, numCUs, reqBits;

		CUWidth = 1 << (sps->log2_min_luma_coding_block_size_minus3 + 3 + sps->log2_diff_max_min_luma_coding_block_size);
		numCUs = ((sps->pic_width_in_luma_samples + (CUWidth - 1)) / CUWidth) * ((sps->pic_height_in_luma_samples + (CUWidth - 1)) / CUWidth);
		reqBits = MinReqBits(numCUs);

		slice->slice_segment_address = slice_get_bits(pBs, reqBits, "slice_segment_address");
	} else {
		slice->slice_segment_address = 0;
	}

	if (!dependent_slice_segment_flag) {
		i = 0;
		if (pps->num_extra_slice_header_bits > i) {
			i++;
			slice_get_bits(pBs, 1, "discardable_flag");
		}

		if (pps->num_extra_slice_header_bits > i) {
			i++;
            slice_get_bits(pBs, 1, "cross_layer_bla_flag");
		}

		for (; i < pps->num_extra_slice_header_bits; i++) {
			slice_get_bits(pBs, 1, "slice_reserved_flag[%d]", i);
		}
		slice->slice_type = slice_get_uvlc(pBs, "slice_type");

		//hk: hard code for h265 enc
		slice->slice_type = (slice->slice_type == 1) ? P_SLICE : ((slice->slice_type == 0) ? B_SLICE : I_SLICE);

		assert(slice->slice_type != B_SLICE);  // encoder current not support b slice

		assert(slice->slice_type <= 2);

		MaxPicOrderCntLsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
		slice->MaxPicOrderCntLsb = MaxPicOrderCntLsb;
		slice->decode_pic_order_cnt_lsb = (nuh_layer_id > 0 && !vps->poc_lsb_not_present_flag[vps->LayerIdxInVps[nuh_layer_id]]) ? 1 : 0;
		if ((nuh_layer_id > 0 && !vps->poc_lsb_not_present_flag[vps->LayerIdxInVps[nuh_layer_id]]) || !isIDR(type)) {
			int PicOrderCntMsb;
			int iPrevPOC = slice->PicOrderCntVal;
			int prevPicOrderCntLsb = iPrevPOC & (MaxPicOrderCntLsb  - 1);
			int prevPicOrderCntMsb = iPrevPOC - prevPicOrderCntLsb;

			slice_pic_order_cnt_lsb = slice_get_bits(pBs, sps->log2_max_pic_order_cnt_lsb_minus4 + 4, "slice_pic_order_cnt_lsb");

			if ((slice_pic_order_cnt_lsb < prevPicOrderCntLsb) && ((prevPicOrderCntLsb - slice_pic_order_cnt_lsb) >= (MaxPicOrderCntLsb / 2))) {
				PicOrderCntMsb = prevPicOrderCntMsb + MaxPicOrderCntLsb;
			} else if ((slice_pic_order_cnt_lsb > prevPicOrderCntLsb) && ((slice_pic_order_cnt_lsb - prevPicOrderCntLsb) > (MaxPicOrderCntLsb / 2))) {
				PicOrderCntMsb = prevPicOrderCntMsb - MaxPicOrderCntLsb;
			} else {
				PicOrderCntMsb = prevPicOrderCntMsb;
			}

			if (isBLA(type)) {
				// For BLA picture types, POCmsb is set to 0.
				PicOrderCntMsb = 0;
			}

			slice->PicOrderCntVal = PicOrderCntMsb + slice_pic_order_cnt_lsb;
			slice->slice_pic_order_cnt_lsb = slice_pic_order_cnt_lsb;
		} else {
			//When slice_pic_order_cnt_lsb is not present, slice_pic_order_cnt_lsb is inferred to be equal to 0.
			slice_pic_order_cnt_lsb = 0;
			slice->PicOrderCntVal = 0;
			slice->slice_pic_order_cnt_lsb = slice_pic_order_cnt_lsb;
		}

		if (!isIDR(type)) { //not IDR
			int short_term_ref_pic_set_sps_flag = slice_get_bits(pBs, 1, "short_term_ref_pic_set_sps_flag");

			if (!short_term_ref_pic_set_sps_flag) {
				if (0 > parseSTRPS(sps->num_short_term_ref_pic_sets, sps->num_short_term_ref_pic_sets, &slice->short_term_ref_pic_set, pBs, sps)) {
					return -1;
				}
			} else {
				int reqBits = MinReqBits(sps->num_short_term_ref_pic_sets);
				UINT32 StRpsIdx;

				if (reqBits > 0) {
					StRpsIdx = slice_get_bits(pBs, reqBits, "short_term_ref_pic_set_idx");
					if (StRpsIdx >= sps->num_short_term_ref_pic_sets) {
				        assert(StRpsIdx <= MAX_ST_RPS_SIZE);
					}
				} else {
					StRpsIdx = 0;
				}

				slice->short_term_ref_pic_set = sps->short_term_ref_pic_set[StRpsIdx]; //copy rps to slice's rps
			}

			slice->num_long_term_sps = 0;
			slice->num_long_term_ref_pics = 0;
			if (sps->long_term_ref_pics_present_flag) {
				int reqBits;
				UINT32 num_long_term_sps = 0;
				//int DeltaPocMSBCycleLt = 0;
				UINT32 num_long_term_pics;

				if (sps->num_long_term_ref_pics_sps > 0) {
					num_long_term_sps = slice_get_uvlc(pBs, "num_long_term_sps");
					if (num_long_term_sps > sps->num_long_term_ref_pics_sps) {
						/* replace the illegal value with previous one */
						erc_msg("num_long_term_sps", num_long_term_sps, slice->num_long_term_sps);
					}
					slice->num_long_term_sps = num_long_term_sps;
				}

				num_long_term_pics = slice_get_uvlc(pBs, "num_long_term_pics");
				assert(num_long_term_pics < MAX_HEVC_RLIST_ENTRY);
				assert(num_long_term_sps < MAX_HEVC_RLIST_ENTRY);
				assert(slice->short_term_ref_pic_set.NumDeltaPocs + num_long_term_sps + num_long_term_pics <= (MAX_NUM_REF_PICS - 1));

				reqBits = MinReqBits(sps->num_long_term_ref_pics_sps);

				slice->num_long_term_ref_pics = num_long_term_sps + num_long_term_pics;
				slice->UsedByCurrPicLt = 0;

				for (i = 0; i < slice->num_long_term_ref_pics; i++) {
					int lt_idx_sps;
					int poc_lsb_lt;
					//int delta_poc_msb_present_flag;

					if (i < num_long_term_sps) {
						if (reqBits > 0) {
							lt_idx_sps = slice_get_bits(pBs, reqBits, "lt_idx_sps[%d]", i);
						} else {
							lt_idx_sps = 0;
						}

						assert(lt_idx_sps >= 0 && lt_idx_sps < MAX_LT_RP_SIZE);

						poc_lsb_lt = sps->lt_ref_pic_poc_lsb_sps[lt_idx_sps];
						slice->UsedByCurrPicLt |= ((sps->used_by_curr_pic_lt_sps_flag >> lt_idx_sps) & 1) << i;
					} else {
						poc_lsb_lt = slice_get_bits(pBs, sps->log2_max_pic_order_cnt_lsb_minus4 + 4, "poc_lsb_lt[%d]", i);
						slice->UsedByCurrPicLt |= slice_get_bits(pBs, 1, "used_by_curr_pic_lt_flag[%d]", i) << i;
					}

					//delta_poc_msb_present_flag = slice_get_bits(pBs, 1, "delta_poc_msb_present_flag[%d]", i);
					slice_get_bits(pBs, 1, "delta_poc_msb_present_flag[%d]", i);

					{
						// spec L1003_v34 (7-38), When delta_poc_msb_cycle_lt[i] is not present, it is inferred to be equal to 0.
						if (i == 0 || i == num_long_term_sps) {
							//DeltaPocMSBCycleLt = 0;
						}

						slice->pocLt[i] = poc_lsb_lt;
					}
				}
			}

			if (sps->sps_temporal_mvp_enabled_flag) {
				slice->slice_temporal_mvp_enabled_flag = slice_get_bits(pBs, 1, "slice_temporal_mvp_enabled_flag");
			} else {
				slice->slice_temporal_mvp_enabled_flag = 0;
			}
		} else {
			slice->short_term_ref_pic_set.NumNegativePics = 0;
			slice->short_term_ref_pic_set.NumDeltaPocs = 0;
			slice->short_term_ref_pic_set.curr_num_delta_pocs = 0;
			slice->num_long_term_ref_pics = 0;
			slice->slice_temporal_mvp_enabled_flag = 0;
		}

		slice->NumActiveRefLayerPics = 0;
		slice->inter_layer_pred_enabled_flag = 0;

#if 0 //TODO
		if (nuh_layer_id > 0 && !vps->default_ref_layers_active_flag && vps->NumDirectRefLayers[nuh_layer_id] > 0) {
			inter_layer_se_bits += 1;
			slice->inter_layer_pred_enabled_flag = slice_get_bits(pBs, 1, "inter_layer_pred_enabled_falg");
			if (slice->inter_layer_pred_enabled_flag) {
				if (vps->NumDirectRefLayers[nuh_layer_id] > 1) {
					int len = calculateLenOfSyntaxElement(vps->NumDirectRefLayers[nuh_layer_id]);

					if (!vps->max_one_active_ref_layer_flag) {
						inter_layer_se_bits += len;
						slice->NumActiveRefLayerPics = slice_get_bits(pBs, len, "num_inter_layer_ref_pics_minus1") + 1;
					} else {
						for (i = 0; i < vps->NumDirectRefLayers[nuh_layer_id]; i++) {
							assert(0);//TODO
#if 0
							int refLayerIdx = vps->LayerIdxInVps[vps->IdDirectRefLayer[nuh_layer_id][i]];
							if (vps->sub_layers_vps_max_minus1[refLayerIdx] >= temporal_id && (temporal_id == 0 ||
									vps->max_tid_il_ref_pics_plus1[refLayerIdx][vps->LayerIdxInVps[nuh_layer_id]] > temporal_id)) {
								slice->NumActiveRefLayerPics = 1;
								break;
							}
#endif
						}
					}

					if (slice->NumActiveRefLayerPics == vps->NumDirectRefLayers[nuh_layer_id]) {
						for (i = 0; i < slice->NumActiveRefLayerPics; i++) {
							slice->RefPicLayerId[i] = vps->IdDirectRefLayer[nuh_layer_id][i];
						}
					} else {
						for (i = 0; i < slice->NumActiveRefLayerPics; i++) {
							int inter_layer_pred_layer_idc;

							inter_layer_se_bits += len;
							inter_layer_pred_layer_idc = slice_get_bits(pBs, len, "inter_layer_pred_layer_idc[%d]", i);
							slice->RefPicLayerId[i] = vps->IdDirectRefLayer[nuh_layer_id][inter_layer_pred_layer_idc];
						}
					}
				} else {
					int refLayerIdx = vps->LayerIdxInVps[vps->IdDirectRefLayer[nuh_layer_id][0]];
					if (vps->sub_layers_vps_max_minus1[refLayerIdx] >= temporal_id && (temporal_id == 0 ||
							vps->max_tid_il_ref_pics_plus1[refLayerIdx][vps->LayerIdxInVps[nuh_layer_id]] > temporal_id)) {
						slice->NumActiveRefLayerPics = 1;
						slice->RefPicLayerId[0] = vps->IdDirectRefLayer[nuh_layer_id][0];
					}
				}
			} else {
				slice->NumActiveRefLayerPics = 1;

			}
		} else if (vps->default_ref_layers_active_flag && nuh_layer_id > 0) {
			int numRefLayerPics;
			slice->inter_layer_pred_enabled_flag = 1;
			for (i = 0, numRefLayerPics = 0; i < vps->NumDirectRefLayers[nuh_layer_id]; i++) {
				int refLayerIdx = vps->LayerIdxInVps[vps->IdDirectRefLayer[nuh_layer_id][i]];
				if (vps->sub_layers_vps_max_minus1[refLayerIdx] >= temporal_id && (temporal_id == 0 ||
						vps->max_tid_il_ref_pics_plus1[refLayerIdx][vps->LayerIdxInVps[nuh_layer_id]] > temporal_id)) {

					slice->RefPicLayerId[numRefLayerPics] = vps->IdDirectRefLayer[nuh_layer_id][i];
					numRefLayerPics++;
				}

			}
			slice->NumActiveRefLayerPics = numRefLayerPics;
		}
#endif

		if (sps->sample_adaptive_offset_enabled_flag) {
			slice->slice_sao_luma_flag = slice_get_bits(pBs, 1, "slice_sao_luma_flag");
			slice->slice_sao_chroma_flag = slice_get_bits(pBs, 1, "slice_sao_chroma_flag");
		} else {
			slice->slice_sao_luma_flag = 0;
			slice->slice_sao_chroma_flag = 0;
		}

		if (!isIntra(slice->slice_type)) {
			int num_ref_idx_active_override_flag;

			num_ref_idx_active_override_flag = slice_get_bits(pBs, 1, "num_ref_idx_active_override_flag");

			if (num_ref_idx_active_override_flag) {
				slice->num_ref_idx_l0_active = slice_get_uvlc(pBs, "num_ref_idx_l0_active_minus1") + 1;
				assert((slice->num_ref_idx_l0_active - 1) <= (MAX_HEVC_RLIST_ENTRY - 2));

				{
					slice->num_ref_idx_l1_active = 0;
				}
			} else {
				slice->num_ref_idx_l0_active = pps->num_ref_idx_l0_default_active_minus1 + 1;

				{
					slice->num_ref_idx_l1_active = 0;
				}
			}
		} else {
			slice->num_ref_idx_l0_active = 0;
			slice->num_ref_idx_l1_active = 0;
		}

		slice->ref_pic_list_modification_flag_l0 = 0;
		slice->ref_pic_list_modification_flag_l1 = 0;
		if (!isIntra(slice->slice_type)) {
			if (pps->lists_modification_present_flag && getNumRpsCurrTempList(slice) > 1) {
				parseRPLM(type, slice, pBs);
			}
		}

		{
			slice->mvd_l1_zero_flag = 0;
		}

		if (!isIntra(slice->slice_type) && pps->cabac_init_present_flag) {
			slice->cabac_init_flag = slice_get_bits(pBs, 1, "cabac_init_flag");
		} else {
			slice->cabac_init_flag = 0;
		}

		if (slice->slice_temporal_mvp_enabled_flag) {
            {
				slice->collocated_from_l0_flag = 1;
			}

			if (
				!isIntra(slice->slice_type) &&
				((slice->collocated_from_l0_flag && slice->num_ref_idx_l0_active > 1)
				 || (!slice->collocated_from_l0_flag && slice->num_ref_idx_l1_active > 1))) {
				/*collocated_ref_idx = */slice_get_uvlc(pBs, "collocated_ref_idx");
			} else {
				//collocated_ref_idx = 0;
			}
		} else {
			slice->collocated_from_l0_flag = 1;
			//collocated_ref_idx = 0;
		}

		if ((pps->weighted_pred_flag && isInterP(slice->slice_type))
			/*|| (pps->weighted_bipred_flag && isInterB(slice->slice_type))*/) {
			assert(0);//TODO
			//parsePWT(sps->chroma_format_idc, slice, pBs);
		}

		if (!isIntra(slice->slice_type)) {
			/*five_minus_max_num_merge_cand =*/ slice_get_uvlc(pBs, "five_minus_max_num_merge_cand");
		}

		slice->slice_qp = pps->init_qp_minus26 + 26 + slice_get_svlc(pBs, "slice_qp_delta");
		if (slice->slice_qp > 51) {
			erc_msg("slice_qp_delta", slice->slice_qp, 51);
		}

		if (pps->pps_slice_chroma_qp_offsets_present_flag) {
			int qp_offset;

			qp_offset = slice_get_svlc(pBs, "slice_cb_qp_offset");
			if (qp_offset < -12 || (pps->pps_cb_qp_offset + qp_offset) < -12) {
				if (pps->pps_cb_qp_offset >= 0) {
					erc_msg("slice_cb_qp_offset", qp_offset, -12);
				} else {
					erc_msg("slice_cb_qp_offset", qp_offset, -12 - pps->pps_cb_qp_offset);
				}
			} else if (qp_offset > 12 || (pps->pps_cb_qp_offset + qp_offset) > 12) {
				if (pps->pps_cb_qp_offset >= 0) {
					erc_msg("slice_cb_qp_offset", qp_offset, 12 - pps->pps_cb_qp_offset);
				} else {
					erc_msg("slice_cb_qp_offset", qp_offset, 12);
				}
			}
			//slice_cb_qp_offset = qp_offset;

			qp_offset = slice_get_svlc(pBs, "slice_cr_qp_offset");
			if (qp_offset < -12 || (pps->pps_cr_qp_offset + qp_offset) < -12) {
				if (pps->pps_cr_qp_offset >= 0) {
					erc_msg("slice_cr_qp_offset", qp_offset, -12);
				} else {
					erc_msg("slice_cr_qp_offset", qp_offset, -12 - pps->pps_cr_qp_offset);
				}
			} else if (qp_offset > 12 || (pps->pps_cr_qp_offset + qp_offset) > 12) {
				if (pps->pps_cr_qp_offset >= 0) {
					erc_msg("slice_cr_qp_offset", qp_offset, 12 - pps->pps_cr_qp_offset);
				} else {
					erc_msg("slice_cr_qp_offset", qp_offset, 12);
				}
			}
			//slice_cr_qp_offset = qp_offset;
		} else {
			//slice_cb_qp_offset = 0;
			//slice_cr_qp_offset = 0;
		}

		if (pps->deblocking_filter_override_enabled_flag) {
			slice->deblocking_filter_override_flag = slice_get_bits(pBs, 1, "deblocking_filter_override_flag");
		} else {
			slice->deblocking_filter_override_flag = 0;
		}

		if (slice->deblocking_filter_override_flag) {
			slice->slice_deblocking_filter_disabled_flag = slice_get_bits(pBs, 1, "slice_deblocking_filter_disabled_flag");
			if (!slice->slice_deblocking_filter_disabled_flag) {
				slice->slice_beta_offset_div2 = slice_get_svlc(pBs, "slice_beta_offset_div2");
				slice->slice_tc_offset_div2 = slice_get_svlc(pBs, "slice_tc_offset_div2");
			}
		} else {
			slice->slice_deblocking_filter_disabled_flag = pps->pps_deblocking_filter_disabled_flag;
			slice->slice_beta_offset_div2 = pps->pps_beta_offset_div2;
			slice->slice_tc_offset_div2 =  pps->pps_tc_offset_div2;
		}

		if (pps->pps_loop_filter_across_slices_enabled_flag &&
			(slice->slice_sao_luma_flag || slice->slice_sao_chroma_flag || !slice->slice_deblocking_filter_disabled_flag)) {
			slice->slice_loop_filter_across_slices_enabled_flag = slice_get_bits(pBs, 1, "slice_loop_filter_across_slices_enabled_flag");
		} else {
			slice->slice_loop_filter_across_slices_enabled_flag = pps->pps_loop_filter_across_slices_enabled_flag;
		}
	} else {
		assert(0);//TODO
#if 0
		if (slice != dependent) {
			memcpy(&slice->dependent_slice_start, &dependent->dependent_slice_start, slice_dependent_size);
		}
#endif
	}

	if (pps->tiles_enabled_flag || pps->entropy_coding_sync_enabled_flag) {
        slice_get_uvlc(pBs, "num_entry_point_offsets");
	} else {
		//num_entry_point_offsets = 0;
	}

	if (pps->slice_segment_header_extension_present_flag) {
		assert(0);//TODO
#if 0
		int slice_segment_header_extension_length = slice_get_uvlc(pBs, "slice_segment_header_extension_length") * 8;
		int bitL = bit_length(pBs);

		if (slice_segment_header_extension_length) {
			int PocMsbValRequiredFlag;

			if (pps->poc_reset_info_present_flag) {
				slice->poc_reset_idc = slice_get_bits(pBs, 2, "poc_reset_idc");
			} else {
				slice->poc_reset_idc = 0;
			}

			if (slice->poc_reset_idc) {
				slice->poc_reset_period_id = slice_get_bits(pBs, 6, "poc_reset_period_id");
			} else {
				slice->poc_reset_period_id = hevc->poc_reset_period_id[nuh_layer_id];
			}

			if (slice->poc_reset_idc == 3) {
				slice->full_poc_reset_flag = slice_get_bits(pBs, 1, "full_poc_reset_flag");
				slice->poc_lsb_val = slice_get_bits(pBs, sps->log2_max_pic_order_cnt_lsb_minus4 + 4, "poc_lsb_val");
			}

			PocMsbValRequiredFlag = (isCRA(slice->nalType) || isBLA(slice->nalType)) && (!vps->vps_poc_lsb_aligned_flag ||
									(vps->vps_poc_lsb_aligned_flag && vps->NumDirectRefLayers[nuh_layer_id] == 0));

			if (!PocMsbValRequiredFlag && vps->vps_poc_lsb_aligned_flag) {
				slice->poc_msb_cycle_val_present_flag = slice_get_bits(pBs, 1, "poc_msb_cycle_val_present_flag");
			} else {
				if (PocMsbValRequiredFlag) {
					slice->poc_msb_cycle_val_present_flag = 1;
				} else {
					slice->poc_msb_cycle_val_present_flag = 0;
				}
			}

			if (slice->poc_msb_cycle_val_present_flag) {
				slice->poc_msb_cycle_val = slice_get_uvlc(pBs, "poc_msb_cycle_val");
			}

			if (slice_segment_header_extension_length > 256 * 8) {
				erc_msg("slice_segment_header_extension_length", slice_segment_header_extension_length, 0);
			}

			assert(bit_length(pBs) > bitL);

#if defined(__i386__) || defined(__x86_64__)
			slice_segment_header_extension_length -= (bit_length(pBs) - bitL);

			if (slice_segment_header_extension_length > 0) {
				for (i = 0; i < (slice_segment_header_extension_length / 8); i++) {
					slice_skip_bits(pBs, 8, "slice_segment_header_extension_data_bit");
				}

				slice_segment_header_extension_length %= 8;
				for (i = 0; i < slice_segment_header_extension_length; i++) {
					slice_skip_bits(pBs, 1, "slice_segment_header_extension_data_bit");
				}
			}
#endif
		} else {
			slice->poc_reset_idc = 0;
			slice->poc_reset_period_id = hevc->poc_reset_period_id[nuh_layer_id];
			slice->poc_msb_cycle_val_present_flag = 0;
		}
#endif
	}

#if !ERC
	assert(bit_length(pBs) > 0);
#endif

	//readByteAlignment(pBs);

	slice->shvc_inter_layer_se_bits = inter_layer_se_bits;
	slice->valid = 1;

	return H265DEC_SUCCESS;
}


INT32 H265DecSeqHeader(H265DEC_INIT *pH265DecInit, H265DecHdrObj *pH265DecHdrObj, H26XDEC_VUI *pVui)
{
	H265DecVpsObj *pH265DecVpsObj = &pH265DecHdrObj->sH265DecVpsObj;
	H265DecSpsObj *pH265DecSpsObj = &pH265DecHdrObj->sH265DecSpsObj;
	H265DecPpsObj *pH265DecPpsObj = &pH265DecHdrObj->sH265DecPpsObj;
	//H265DEC_CODECINFO*pCodecInfo  = &pH265DecInit->CodecInfo;
	UINT32  uiHeaderSize;

	bstream sBstream;
	bstream *pBstream = &sBstream;

	memset(pH265DecHdrObj,  0x0, sizeof(H265DecHdrObj));

	//dec_dump_buf((const unsigned char *)pH265DecInit->uiH265DescAddr, pH265DecInit->uiH265DescSize);
	//uiHeaderSize = ebspTorbsp((UINT8 *)pH265DecInit->uiH265DescAddr, ucH265DecNalBuf, (pH265DecInit->uiH265DescSize * 8));
	uiHeaderSize = ebspTorbsp((UINT8 *)pH265DecInit->uiH265DescAddr, pH265DecHdrObj->ucH265DecNalBuf, pH265DecInit->uiH265DescSize);
	//dec_dump_buf((const unsigned char *)ucH265DecNalBuf, uiHeaderSize);

	init_parse_bitstream(pBstream, (void *)pH265DecHdrObj->ucH265DecNalBuf, uiHeaderSize);

	if (H265DecVps(pH265DecVpsObj, pBstream) != H265DEC_SUCCESS) {
		return H265DEC_FAIL;
	}

	if (H265DecSps(pH265DecSpsObj, pH265DecVpsObj, pBstream, 0, pVui) != H265DEC_SUCCESS) {
		return H265DEC_FAIL;
	}

	if (H265DecPps(pH265DecPpsObj, pBstream) != H265DEC_SUCCESS) {
		return H265DEC_FAIL;
	}

	//check
	//if (pH265DecInit->uiWidth != pH265DecSpsObj->pic_width_in_luma_samples ||
	//	(pH265DecInit->uiHeight + 15) / 16 * 16 != pH265DecSpsObj->pic_height_in_luma_samples) {
		//DBG_ERR("[H265DecSeqHeader] picture size error\r\n");
	//	DBG_DUMP("[H265DecSeqHeader]Decode Init Frame Size(%d, %d) and Decode Sequence Header Frame Size(%d, %d)\r\n ",
	//		(int)pH265DecInit->uiWidth, (int)pH265DecInit->uiHeight, (int)pH265DecSpsObj->pic_width_in_luma_samples, (int)pH265DecSpsObj->pic_height_in_luma_samples);
	//}

	// Set Codec Information //
	//hk; not used ??
	// return init information //
	pH265DecInit->uiWidth        = pH265DecSpsObj->pic_width_in_luma_samples;
	pH265DecInit->uiHeight       = pH265DecSpsObj->pic_height_in_luma_samples - (pH265DecSpsObj->conf_win_bottom_offset<<1);
	pH265DecInit->uiYLineOffset  = pH265DecInit->uiWidth/4;
	pH265DecInit->uiUVLineOffset = pH265DecInit->uiWidth/4;
	pH265DecInit->uiDisplayWidth = pH265DecInit->uiWidth - (pH265DecSpsObj->conf_win_left_offset<<1) - (pH265DecSpsObj->conf_win_right_offset<<1);

	//DBG_ERR("[H265DecSeqHeader]Decode uiDisplayWidth = %d!\r\n ", (int)(pH265DecInit->uiDisplayWidth)));

	return H265DEC_SUCCESS;
}

