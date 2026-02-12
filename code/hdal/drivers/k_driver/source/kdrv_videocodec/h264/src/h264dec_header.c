/*
    H264ENC module driver for NT96660

    decode h264 header

    @file       h264dec_header.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "kdrv_vdocdc_dbg.h"

#include "h26x_common.h"

#include "h264dec_header.h"
#include "h264dec_int.h"
#include "h264_def.h"

//#define DecNalBufSize 512
//UINT8 ucH264DecNalBuf[DecNalBufSize];
/*
static void dec_dump_buf(const unsigned char *buf, size_t len)
{
    size_t i;
	DBG_DUMP("buf(%08p, %08x):\r\n", buf, len);

    for (i = 0; i < len; i++)
        DBG_DUMP("%02x ", *(buf + i));
    DBG_DUMP("\r\n");
}
*/


static int ReadHRDParameters(bstream *pBstream)
{
	UINT32 SchedSelIdx;
	UINT32 cpb_cnt_minus1;

	cpb_cnt_minus1 = read_uvlc_codeword(pBstream);
	get_bits(pBstream, 4);
	get_bits(pBstream, 4);

	for( SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++ )
	{
		read_uvlc_codeword(pBstream);
		read_uvlc_codeword(pBstream);
		get_bits(pBstream, 1);
	}

	get_bits(pBstream, 5);
	get_bits(pBstream, 5);
	get_bits(pBstream, 5);
	get_bits(pBstream, 5);
DBG_DUMP("parse vui_hrd_param\r\n");

	return 0;
}

static void parseVUI(H264DecSpsObj *pH264DecSpsObj,bstream *pBstream, H26XDEC_VUI *pVui)
{
#if 1
    UINT32 aspect_ratio_info_present_flag;
    UINT32 aspect_ratio_idc;
    //UINT32 sar_width;
    //UINT32 sar_height;
    //UINT32 overscan_info_present_flag;
    UINT32 video_signal_type_present_flag;
    //UINT32 chroma_loc_info_present_flag;
    //UINT32 timing_info_present_flag;
    //UINT32 nal_hrd_parameters_present_flag;
    //UINT32 vcl_hrd_parameters_present_flag;
    //UINT32 pic_struct_present_flag;
    //UINT32 bitstream_restriction_flag;
    //UINT32 num_units_in_tick,time_scale,fixed_fr_flag;

    aspect_ratio_info_present_flag = get_bits(pBstream,1);
    if (aspect_ratio_info_present_flag)
    {
        aspect_ratio_idc = get_bits(pBstream,8);
        if (aspect_ratio_idc == 255) // Extended_SAR
        {
            pVui->uiSarWidth = get_bits(pBstream,16);
            pVui->uiSarHeight = get_bits(pBstream,16);
        }
    }

    get_bits(pBstream,1);	//overscan_info_present_flag, suppose it's 0 //
    video_signal_type_present_flag = get_bits(pBstream,1);	//video_signal_type_present_flag //
	if (video_signal_type_present_flag) {
		// parse video_signal_type_present //
		pVui->ucVideoFmt = get_bits(pBstream, 3);
		pVui->ucColorRange = get_bits(pBstream, 1);
		if (get_bits(pBstream, 1) == 1) {
			pVui->ucColorPrimaries = get_bits(pBstream, 8);
			pVui->ucTransChar = get_bits(pBstream, 8);
			pVui->ucMatrixCoeff = get_bits(pBstream, 8);
		}
	}
    get_bits(pBstream,1);	//chroma_loc_info_present_flag, suppose it's 0 //
    pVui->bTimingPresentFlag = get_bits(pBstream,1); // suppose it's 0
    if(pVui->bTimingPresentFlag)
    {

        //num_units_in_tick  = get_bits(pBstream,32);
        //time_scale         = get_bits(pBstream,32);
        //fixed_fr_flag      = get_bits(pBstream,1);
        get_bits(pBstream,32);
        get_bits(pBstream,32);
        get_bits(pBstream,1);
    }

	{
		BOOL nal_hrd_parameters_present_flag, vcl_hrd_parameters_present_flag;
		nal_hrd_parameters_present_flag = get_bits(pBstream,1); // suppose it's 0
		if (nal_hrd_parameters_present_flag) {
			ReadHRDParameters(pBstream);
		}
		vcl_hrd_parameters_present_flag = get_bits(pBstream,1); // suppose it's 0
		if (vcl_hrd_parameters_present_flag) {
			ReadHRDParameters(pBstream);
		}
		if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
			//low_delay_hrd_flag
			get_bits(pBstream,1);
		}
		//pic_struct_present_flag         = get_bits(pBstream,1); // suppose it's 0
		get_bits(pBstream,1);
		//bitstream_restriction_flag      = get_bits(pBstream,1); // suppose it's 0
		if (get_bits(pBstream,1)) {
			//motion_vectors_over_pic_boundaries_flag
			get_bits(pBstream,1);
			//max_bytes_per_pic_denom
			read_uvlc_codeword(pBstream);
			//max_bits_per_mb_denom
			read_uvlc_codeword(pBstream);
			//log2_max_mv_length_horizontal
			read_uvlc_codeword(pBstream);
			//log2_max_mv_length_vertical
			read_uvlc_codeword(pBstream);
			//num_reorder_frames
			read_uvlc_codeword(pBstream);
			//max_dec_frame_buffering
			read_uvlc_codeword(pBstream);
		}
	}
#endif
}

INT32 H264DecSps(H264DecSpsObj *pH264DecSpsObj,bstream *pBstream, H26XDEC_VUI *pVui){
    UINT8 ucVal;
    UINT32 uiVal;

    uiVal = get_bits(pBstream,32);
    if ( uiVal != 0x00000001){
        DBG_ERR("[H264DecSps]StartCodec Suffix != 0x000001 (0x%08x)\r\n",(unsigned int)uiVal);
        return H264DEC_FAIL;
    }

    ucVal = get_bits(pBstream, 8) & 0x1F;

    if (ucVal != NALU_TYPE_SPS){
        DBG_ERR("[H264DecSps]Nal Unit Type(%d) != NALU_TYPE_SPS\r\n", (int)(ucVal));
        return H264DEC_FAIL;
    }

    pH264DecSpsObj->uiProfileIdc = get_bits(pBstream,8);    /* profile_idc     */
    get_bits(pBstream,3);                           /* constraint_flag */
    get_bits(pBstream,5);                           /* reserved zero   */
    pH264DecSpsObj->uiLevelIdc = get_bits(pBstream,8);      /* level_idc       */
    pH264DecSpsObj->uiSpsId= read_uvlc_codeword(pBstream);  /* Sps_id          */

    if(pH264DecSpsObj->uiProfileIdc == PROFILE_HIGH || pH264DecSpsObj->uiProfileIdc == PROFILE_HIGH10)
    {
        pH264DecSpsObj->uiChormaFormatIdc = read_uvlc_codeword(pBstream);       /* chroma_format_idc               */
        pH264DecSpsObj->uiBitDepthLumaMinus8 = read_uvlc_codeword(pBstream);    /* bit_depth_luma_minus8           */
        pH264DecSpsObj->uiBitDepthChromaMinus8 = read_uvlc_codeword(pBstream);  /* bit_depth_luma_minus8           */
        get_bits(pBstream,1);                                           /* QPY' zero transform bypass flag */
        get_bits(pBstream,1);                                           /* seq_scaling_matrix_present_flag */
    }
    pH264DecSpsObj->uiLog2MaxFrmMinus4 = read_uvlc_codeword(pBstream);      /* log2_max_frame_num_minus4 */
    pH264DecSpsObj->uiPocType = read_uvlc_codeword(pBstream);               /* pic_order_cnt_type */
    pH264DecSpsObj->uiLog2MaxPocLsbMinus4 = read_uvlc_codeword(pBstream);   /* log2_max_pic_order_cnt_lsb_minus4 */
    pH264DecSpsObj->uiNumRefFrm = read_uvlc_codeword(pBstream);             /* Num_Ref_Frames */
    get_bits(pBstream,1);  			                                        /* gaps_in_frame_num_allowed */
    pH264DecSpsObj->uiMbWidth = read_uvlc_codeword(pBstream) + 1;           /* width_in_MB   */
    pH264DecSpsObj->uiMbHeight = read_uvlc_codeword(pBstream) + 1;          /* height_in_MB  */
    pH264DecSpsObj->uiFrmMbsOnly = get_bits(pBstream,1);                    /* frame_mbs_only_flag */
    pH264DecSpsObj->uiTrans8x8 = get_bits(pBstream,1);                      /* direct_8x8_inference_flag */
    pH264DecSpsObj->uiCropping = get_bits(pBstream,1);                      /* frame_cropping_flag */
    if (pH264DecSpsObj->uiCropping){
        pH264DecSpsObj->uiCropLeftOffset  = read_uvlc_codeword(pBstream);
        pH264DecSpsObj->uiCropRightOffset = read_uvlc_codeword(pBstream);
        pH264DecSpsObj->uiCropTopOffset   = read_uvlc_codeword(pBstream);
        pH264DecSpsObj->uiCropBtmOffset   = read_uvlc_codeword(pBstream);
    }
    else{
        pH264DecSpsObj->uiCropLeftOffset  = 0;
        pH264DecSpsObj->uiCropRightOffset = 0;
        pH264DecSpsObj->uiCropTopOffset   = 0;
        pH264DecSpsObj->uiCropBtmOffset   = 0;
    }
    pH264DecSpsObj->uiVuiParameterPresentFlag = get_bits(pBstream,1);       /* vui_parameters_present_flag */
    if (pH264DecSpsObj->uiVuiParameterPresentFlag)
    {
    	pVui->bPresentFlag = TRUE;
        parseVUI( pH264DecSpsObj, pBstream, pVui);
    }

    read_rbsp_trailing_bits(pBstream);

    return H264DEC_SUCCESS;
}

INT32 H264DecPps(H264DecSpsObj *pH264DecSpsObj,H264DecPpsObj *pH264DecPpsObj,bstream *pBstream){
    UINT8 ucVal;

    if (get_bits(pBstream,32) != 0x00000001){
        DBG_ERR("[H264DecPps]StartCodec Suffix != 0x00000001\r\n");
        return H264DEC_FAIL;
    }

    ucVal = get_bits(pBstream,8) & 0x1F;

    if (ucVal != NALU_TYPE_PPS){
        DBG_ERR("[H264DecPps]Nal Unit Type != NALU_TYPE_SPS\r\n");
        return H264DEC_FAIL;
    }

    pH264DecPpsObj->uiPpsId= read_uvlc_codeword(pBstream);  /* Pps_id          */
    pH264DecPpsObj->uiSpsId= read_uvlc_codeword(pBstream);  /* Sps_id          */

    pH264DecPpsObj->uiEntropyCoding = get_bits(pBstream,1); /* entropy_coding_mode_flag */
    ucVal = get_bits(pBstream,1);                           /* pic_order_present_flag */
    ucVal = read_uvlc_codeword(pBstream);                   /* num_slice_groups_minus1 */

    pH264DecPpsObj->uiNumRefIdxL0Minus1 = read_uvlc_codeword(pBstream); /* num_ref_idx_l0_active - 1 */
    pH264DecPpsObj->uiNumRefIdxL1Minus1 = read_uvlc_codeword(pBstream); /* num_ref_idx_l1_active - 1 */

    ucVal = get_bits(pBstream,1);                           /* weighted_pred_flag */
    ucVal = get_bits(pBstream,2);                           /* weighted_bipred_flag */

    pH264DecPpsObj->uiPicInitQPMinus26 = read_signed_uvlc_codeword(pBstream);            /* pic_init_qp - 26 */
    pH264DecPpsObj->uiPicInitQSMinus26 = read_signed_uvlc_codeword(pBstream);            /* pic_init_qs - 26 */
    pH264DecPpsObj->uiChromQPIdxoffset = read_signed_uvlc_codeword(pBstream);            /* chroma_qp_idx_offset */
    pH264DecPpsObj->uiDelockingFilterControlPresentFlag = get_bits(pBstream,1);          /* use special inloop filter */

    ucVal = get_bits(pBstream,1);                           /* use_constrained_intra_pred */
    ucVal = get_bits(pBstream,1);                           /* redundant_pic_cnt_present */

    if (pH264DecSpsObj->uiProfileIdc == PROFILE_HIGH){
        pH264DecPpsObj->uiPicTrans8x8Mode = get_bits(pBstream,1);   /* transform_8x8_mode_flag */
        ucVal = get_bits(pBstream,1);                               /* pic_scaling_matrix_present_flag */
        pH264DecPpsObj->uiPicSecondChormaQPIdxoffset = read_signed_uvlc_codeword(pBstream);                /* (sign)second_croma_qp_index_offset */
    }
    else{
        pH264DecPpsObj->uiPicSecondChormaQPIdxoffset = pH264DecPpsObj->uiChromQPIdxoffset;
    }

    read_rbsp_trailing_bits(pBstream);

    return H264DEC_SUCCESS;
}

static INT32  H264DecSVCHeader(H264DecHdrObj *pH264DecHdrObj, bstream *pBstream)
{
#if 1
    UINT8 ucVal;
    UINT8  temporal_id;
    H264DecSVCObj *pH264DecSvcObj = &pH264DecHdrObj->sH264DecSvcObj;

    /* decode SVC Extension */
    if (get_bits(pBstream,32) != 0x00000001){
        DBG_ERR("[H264DecSVC]StartCodec Suffix != 0x00000001\r\n");
        return H264DEC_FAIL;
    }

    /* nal_unit_header_svc_extension */
    ucVal = get_bits(pBstream,8) & 0x1F;
    if (ucVal != NALU_TYPE_PREFIX){
        DBG_ERR("[H264DecSVCHeader]Nal Unit Type != NALU_TYPE_PREFIX (%x)\r\n", (unsigned int)(ucVal));
        return H264DEC_FAIL;
    }

    ucVal = get_bits(pBstream,1);          /* svc_extension_flag        */
    if (ucVal != 1){
        DBG_ERR("[H264DecSps] Error at svc_extension_flag (%d)\r\n", (int)(ucVal));
        return H264DEC_FAIL;
    }

    //idr_flag = get_bits(pBstream,1);        /* idr_flag                        */
    //priority_id = get_bits(pBstream,6);     /* priority_id                     */

    ucVal = get_bits(pBstream,8);           /* no_inter_layer_pred_flag, dependency_id, quality_id   */
    if (ucVal != 0x80){
        DBG_ERR("[H264DecSps] Error at no_inter_layer_pred_flag (%d)\r\n", (int)(ucVal));
        return H264DEC_FAIL;
    }

    temporal_id = get_bits(pBstream,3);     /* temporal_id                     */
    pH264DecSvcObj->uiTid = temporal_id;

    ucVal = get_bits(pBstream,5);           /* use_ref_base_pic_flag,...   */
    if (ucVal != 0xF){
        DBG_ERR("[H264DecSps] Error at no_inter_layer_pred_flag (%d)\r\n", (int)(ucVal));
        return H264DEC_FAIL;
    }

    DBG_IND("temporal_id = %d\r\n", (int)(temporal_id));

    /* prefix_nal_unit_svc */
    ucVal = get_bits(pBstream,2);           /* store_ref_base_pic_flag,...   */
    if (ucVal != 0x0){
        DBG_ERR("[H264DecSps] Error at no_inter_layer_pred_flag (%d)\r\n", (int)(ucVal));
        return H264DEC_FAIL;
    }

    read_rbsp_trailing_bits(pBstream);
#endif
    return H264DEC_SUCCESS;
}



//static UINT8  type_name[3][64] = {"P","B","I"};

INT32 H264DecSlice(H264DEC_INFO *pH264DecInfo,H264DecHdrObj *pH264DecHdrObj,UINT32 uiBsAddr,UINT32 uiBsLen){
    H264DecSpsObj   *pH264DecSpsObj   = &pH264DecHdrObj->sH264DecSpsObj;
    H264DecPpsObj   *pH264DecPpsObj   = &pH264DecHdrObj->sH264DecPpsObj;
    H264DecSliceObj *pH264DecSliceObj = &pH264DecHdrObj->sH264DecSliceObj;
    H264DecSVCObj   *pH264DecSvcObj   = &pH264DecHdrObj->sH264DecSvcObj;
    bstream sBstream;
    bstream *pBstream = &sBstream;
    UINT32 uiVal,uiHeaderSize;

#if 0
    init_parse_bitstream(pBstream,(void *)uiBsAddr,(uiBsLen * 8));
#else
    //uiHeaderSize = (uiBsLen * 8);
    uiHeaderSize = uiBsLen;
    if(uiHeaderSize > DecNalBufSize) uiHeaderSize = DecNalBufSize;
    uiHeaderSize = ebspTorbsp((UINT8 *)uiBsAddr, pH264DecHdrObj->ucH264DecNalBuf, uiHeaderSize);
    init_parse_bitstream(pBstream,(void *)pH264DecHdrObj->ucH264DecNalBuf,uiHeaderSize);
	//dec_dump_buf((const unsigned char *)ucH264DecNalBuf, uiHeaderSize);
#endif

    if(pH264DecSvcObj->uiSVCFlag)
    {
        if(H264DecSVCHeader( pH264DecHdrObj, pBstream) != H264DEC_SUCCESS)
        {
            DBG_ERR("[H264DecSlice] H264DecSVCHeader Error\r\n");
            return H264DEC_FAIL;
        }
    }

    if (get_bits(pBstream,32) != 0x00000001){
        DBG_ERR("[H264DecSlice]StartCodec Suffix != 0x000001\r\n");
        return H264DEC_FAIL;
    }

    uiVal = get_bits(pBstream,8);
    pH264DecSliceObj->uiNalRefIdc   = (uiVal>>5) & 0x3;
    pH264DecSliceObj->uiNalUnitType = uiVal & 0x1F;

    DBG_IND("uiNalRefIdc = %d\r\n", (int)(pH264DecSliceObj->uiNalRefIdc));

    if ((pH264DecSliceObj->uiNalUnitType != NALU_TYPE_IDR) && (pH264DecSliceObj->uiNalUnitType != NALU_TYPE_SLICE)){
        DBG_ERR("[H264DecSlice]Nal Unit Type != NALU_TYPE_IDR or NALU_TYPE_SLICE %d \r\n", (int)(pH264DecSliceObj->uiNalUnitType));
        return H264DEC_FAIL;
    }

    pH264DecSliceObj->uiFirstMbInSlice = read_uvlc_codeword(pBstream);
    // Slice Type //

    pH264DecSliceObj->ePreSliceType = pH264DecSliceObj->eSliceType;

    //pH264DecSliceObj->eSliceType = (SLICE_TYPE)read_uvlc_codeword(pBstream);
    uiVal = read_uvlc_codeword(pBstream);
    switch(uiVal){
    case 0:
    case 3:
    case 5:
    case 8:
        pH264DecSliceObj->eSliceType = P_SLICE;
        //DBG_ERR("P slice\r\n");
        break;
    case 1:
    case 6:
        pH264DecSliceObj->eSliceType = B_SLICE;
        //DBG_ERR("B slice\r\n");
        break;
    case 2:
    case 4:
    case 7:
    case 9:
        pH264DecSliceObj->eSliceType = I_SLICE;
        //DBG_ERR("I slice\r\n");
        break;
    default:
        DBG_ERR("[H264DecSlice]Slice Type Error[%d]\r\n", (int)(uiVal));
        break;
    }

    pH264DecSliceObj->uiPpsId = read_uvlc_codeword(pBstream);
    pH264DecSliceObj->uiFrmNum = get_bits(pBstream,(pH264DecSpsObj->uiLog2MaxFrmMinus4 + 4));

    if(pH264DecSliceObj->ePreSliceType != B_SLICE)
    {
        pH264DecSliceObj->uiPreIdrFlag = pH264DecSliceObj->uiIdrFlag;
    }

    if (pH264DecSliceObj->uiNalUnitType == NALU_TYPE_IDR){
        pH264DecSliceObj->uiIdrFlag = 1;
        pH264DecSliceObj->uiIdrPicId = read_uvlc_codeword(pBstream);
    }
    else{
        pH264DecSliceObj->uiIdrFlag = 0;
    }

    if (pH264DecSpsObj->uiPocType == 0){
        pH264DecSliceObj->iPocLsb = get_bits(pBstream,(pH264DecSpsObj->uiLog2MaxPocLsbMinus4 + 4));
    }
    pH264DecSliceObj->iPoc = pH264DecSliceObj->iPocLsb;

    if (pH264DecSliceObj->eSliceType == B_SLICE){
        pH264DecSliceObj->uiDirectModeType = get_bits(pBstream,1);
    }

    pH264DecSliceObj->uiNumRefIdxL0 = pH264DecPpsObj->uiNumRefIdxL0Minus1+ 1;
    pH264DecSliceObj->uiNumRefIdxL1 = pH264DecPpsObj->uiNumRefIdxL1Minus1+ 1;

    if (pH264DecSliceObj->eSliceType == P_SLICE || pH264DecSliceObj->eSliceType == B_SLICE){
        if (get_bits(pBstream,1)){
            pH264DecSliceObj->uiNumRefIdxL0 = 1 + read_uvlc_codeword(pBstream);

            if (pH264DecSliceObj->eSliceType == B_SLICE){
                pH264DecSliceObj->uiNumRefIdxL1 = 1 + read_uvlc_codeword(pBstream);
            }
        }
    }

    if (pH264DecSliceObj->eSliceType != B_SLICE){
        pH264DecSliceObj->uiNumRefIdxL1 = 0;
    }

    /* ref_pic_list_reordering */
	pH264DecSliceObj->uiAbsDiffPicNumL0 = 0x1fffffff;
	pH264DecSliceObj->uiLongTermPicNum = 0x1fffffff;

	pH264DecSliceObj->uiRefPicListReorderL0 = 0;
    if (pH264DecSliceObj->eSliceType != I_SLICE){
        pH264DecSliceObj->uiRefPicListReorderL0 = get_bits(pBstream,1);

        DBG_IND(" uiRefPicListReorderL0 = %d\r\n", (int)(pH264DecSliceObj->uiRefPicListReorderL0));

        if (pH264DecSliceObj->uiRefPicListReorderL0 != 0){

            UINT32 uiModifyPicIdc;

            do{
                uiModifyPicIdc = read_uvlc_codeword(pBstream);

                if(uiModifyPicIdc == 0 || uiModifyPicIdc == 1)
                {
                    pH264DecSliceObj->uiAbsDiffPicNumL0 = read_uvlc_codeword(pBstream) + 1;
                }
				else if (uiModifyPicIdc == 2)
                {
                    pH264DecSliceObj->uiLongTermPicNum = read_uvlc_codeword(pBstream);
                }
            }while(uiModifyPicIdc != 3);
        }
    }

    /* dec_ref_pic_marking */
	memset(&pH264DecSliceObj->sMMCO, 0, sizeof(H264DecMMCO));

    if (pH264DecSliceObj->uiNalRefIdc){
        if (pH264DecSliceObj->uiNalUnitType == NALU_TYPE_IDR){
            get_bits(pBstream,1);   /* no_output_of_prior_pics_flag */
			#if 0
            get_bits(pBstream,1);   /* long_term_reference_flag */
			#else
			pH264DecSliceObj->bIDRSetLTFlag = get_bits(pBstream,1);
			if (pH264DecSliceObj->bLTEn == 0)
				pH264DecSliceObj->bLTEn = (pH264DecSliceObj->bIDRSetLTFlag == 1);
			#endif
        }
        else{
			H264DecMMCO *pMMCO = &pH264DecSliceObj->sMMCO;
			pMMCO->uiEnable = get_bits(pBstream,1);
			if (pMMCO->uiEnable)	/* adaptive_ref_pic_buf_flag */
			{
#if 1
				do {
					//memory_management_control_operation
					uiVal = read_uvlc_codeword(pBstream);
					if (uiVal) {
						pMMCO->uiMMCO = uiVal;
						if (pMMCO->uiMMCO != 3 && pMMCO->uiMMCO != 1) {
							DBG_ERR("[H264DecSlice] only support mmco(%d) mark to long-term \r\n", (int)(uiVal));
							return H264DEC_FAIL;
						}
					}
					if (uiVal  == 1 || uiVal == 3) {
						//difference_of_pic_nums_minus1
						pMMCO->uiSTPicNum = read_uvlc_codeword(pBstream);	// get short-term pic_num			//
					}
					if (uiVal == 2) {
						//long_term_pic_num
						read_uvlc_codeword(pBstream);
					}
					if (uiVal == 3 || uiVal == 6) {
						//long_term_frame_idx
						pMMCO->uiLTPicNum = read_uvlc_codeword(pBstream);	// get long-term pic_num , shall be 0 	//
						pH264DecSliceObj->bLTEn = 1;
					}
					if (uiVal == 4) {
						//max_long_term_frame_idx_plus1
						read_uvlc_codeword(pBstream);
					}
				} while (uiVal != 0);
#else
				// only support mmco : 3 , mark reference to long-term //
				uiVal = read_uvlc_codeword(pBstream);

				if (uiVal != 3)
				{
	                DBG_ERR("[H264DecSlice] NT96680 only support mmco(%d) mark to long-term \r\n", (int)(uiVal));
	                return H264DEC_FAIL;
				}

				pMMCO->uiSTPicNum = read_uvlc_codeword(pBstream);	// get short-term pic_num			//
				pMMCO->uiLTPicNum = read_uvlc_codeword(pBstream);	// get long-term pic_num , shall be 0 	//
				uiVal = read_uvlc_codeword(pBstream);	// shall be 0, end mmco 				//
				if (uiVal != 0)
				{
	                DBG_ERR("[H264DecSlice] mmco(%d) shall be equal 0 to finish mmco \r\n", (int)(uiVal));
	                return H264DEC_FAIL;
				}
#endif
            }
        }
    }

    if (pH264DecPpsObj->uiEntropyCoding && pH264DecSliceObj->eSliceType != I_SLICE){
        pH264DecSliceObj->uiCabacInitIdc = read_uvlc_codeword(pBstream);
    }

    pH264DecSliceObj->uiQPDelta = read_signed_uvlc_codeword(pBstream);
    pH264DecSliceObj->uiQP = pH264DecSliceObj->uiQPDelta +26 + pH264DecPpsObj->uiPicInitQPMinus26;

    if (pH264DecPpsObj->uiDelockingFilterControlPresentFlag){
        pH264DecSliceObj->uiDisableDBFilterIdc = read_uvlc_codeword(pBstream);
        if (pH264DecSliceObj->uiDisableDBFilterIdc != 1){
            pH264DecSliceObj->uiAlphaC0OffsetDiv2 = read_signed_uvlc_codeword(pBstream);
            pH264DecSliceObj->uiBetaOffsetDiv2 = read_signed_uvlc_codeword(pBstream);
        }
        else{
            pH264DecSliceObj->uiAlphaC0OffsetDiv2 = 0;
            pH264DecSliceObj->uiBetaOffsetDiv2 = 0;
        }
    }
    else{
        pH264DecSliceObj->uiDisableDBFilterIdc = 0;
        pH264DecSliceObj->uiAlphaC0OffsetDiv2 = 0;
        pH264DecSliceObj->uiBetaOffsetDiv2 = 0;
    }

    return H264DEC_SUCCESS;
}

static INT32 parseSEIMessage( H264DecHdrObj *pH264DecHdrObj, bstream *pBstream)
{
    UINT32 uiTemp,uiType = 0,uiSize = 0;

    uiTemp = 0xFF;
    while( 0xFF == uiTemp )
    {
        uiTemp = get_bits(pBstream,8) & 0x1F;
        uiType += uiTemp;
    }

    if(uiType != SCALABLE_SEI)
    {
        DBG_ERR("[parseSEIMessage] uiType = %d1\r\n", (int)(uiType));
        return H264DEC_FAIL;
    }

    uiTemp = 0xFF;
    while( 0xFF == uiTemp )
    {
        uiTemp = get_bits(pBstream,8) & 0x1F;
        uiSize += uiTemp;
    }

    //debug_msg("parseSEIMessage : Type %d, Size %d\r\n", (int)(uiType), (int)(uiSize));

    return H264DEC_SUCCESS;
}

static INT32 parseSEIPayload( H264DecHdrObj *pH264DecHdrObj, bstream *pBstream)
{
    UINT8 ucVal;
    UINT32 uiIdx;
    UINT32 uiLayer,temporal_id;
    UINT32 uiFrmRateInfoFlag,uiFrmSizeInfoFlag,uiLayerDpndInfoFlag;
    UINT32 uiSPSInfoPstFlag;
    H264DecSVCObj *pH264DecSvcObj = &pH264DecHdrObj->sH264DecSvcObj;


    ucVal = get_bits(pBstream,3);
    if(ucVal != 0)
    {
        DBG_ERR("[parseSEIPayload] Error : start = %d\r\n", (int)(ucVal));
        return H264DEC_FAIL;
    }

    uiLayer = read_uvlc_codeword(pBstream);            /* num_layers_minus1 */
    uiLayer++;
    pH264DecSvcObj->uiLayer = uiLayer;

    DBG_IND("uiLayer = %d\r\n", (int)(uiLayer));
    if(uiLayer > 3)
    {
        DBG_ERR("[parseSEIPayload] Error : uiLayer = %d\r\n", (int)(uiLayer));
        return H264DEC_FAIL;
    }

    for(uiIdx = 0; uiIdx < uiLayer; uiIdx++)
    {
        ucVal = read_uvlc_codeword(pBstream);           /* layer_id            */
        if(ucVal != 0)
        {
            DBG_ERR("[parseSEIPayload] Error : layer_id = %d\r\n", (int)(ucVal));
            return H264DEC_FAIL;
        }
        ucVal = get_bits(pBstream,6);                    /* priority_id         */
        if(ucVal != 0)
        {
            DBG_ERR("[parseSEIPayload] Error : priority_id = %d\r\n", (int)(ucVal));
            return H264DEC_FAIL;
        }
        ucVal = get_bits(pBstream,1);                    /* discardable_flag     */
        if(ucVal != 1)
        {
            DBG_ERR("[parseSEIPayload] Error : priority_id = %d\r\n", (int)(ucVal));
            return H264DEC_FAIL;
        }
        ucVal = get_bits(pBstream,7);                    /* dependency_id        */
        if(ucVal != 0)
        {
            DBG_ERR("[parseSEIPayload] Error : dependency_id & quality_id = %d\r\n", (int)(ucVal));
            return H264DEC_FAIL;
        }
        temporal_id = get_bits(pBstream,3);              /* temporal_id          */

        pH264DecSvcObj->uiTemporalId[uiIdx] = temporal_id;
        if(temporal_id != uiIdx)
        {
            DBG_ERR("[parseSEIPayload] Error : temporal_id & quality_id = %d\r\n", (int)(temporal_id));
            return H264DEC_FAIL;
        }
        DBG_IND("temporal_id = %d\r\n", (int)(uiIdx));

        ucVal = get_bits(pBstream,5);                   /* sub_pic_layer_flag     */
        if(ucVal != 0)
        {
            DBG_ERR("[parseSEIPayload] Error : sub_pic_layer_flag & ... = %d\r\n", (int)(ucVal));
            return H264DEC_FAIL;
        }
        uiFrmRateInfoFlag   = get_bits(pBstream,1);        /* frm_rate_info_present_flag     */
        uiFrmSizeInfoFlag   = get_bits(pBstream,1);        /* uiFrmSizeInfoFlag              */
        uiLayerDpndInfoFlag = get_bits(pBstream,1);        /* uiLayerDpndInfoFlag            */
        uiSPSInfoPstFlag    = get_bits(pBstream,1);        /* uiSPSInfoPstFlag               */
        pH264DecSvcObj->uiFrmRateInfoFlag = uiFrmRateInfoFlag;
        pH264DecSvcObj->uiFrmSizeInfoFlag = uiFrmSizeInfoFlag;

        //debug_msg("uiFrmRateInfoFlag   = %d\r\n", (int)(uiFrmRateInfoFlag));
        //debug_msg("uiFrmSizeInfoFlag   = %d\r\n", (int)(uiFrmSizeInfoFlag));
        //debug_msg("uiLayerDpndInfoFlag = %d\r\n", (int)(uiLayerDpndInfoFlag));
        //debug_msg("uiSPSInfoPstFlag    = %d\r\n", (int)(uiSPSInfoPstFlag));

        ucVal = get_bits(pBstream,4);                   /* bitstream_restriction_info_present_flag  */
        if(ucVal != 1)
        {
            DBG_ERR("[parseSEIPayload] Error : bitstream_restriction_info_present_flag & ... = %d\r\n", (int)(ucVal));
            return H264DEC_FAIL;
        }

        //if ( m_profile_level_info_present_flag[i] )
        //    put_bits(pH264Bs,             0, 24);      /* layer_profile_level_idc                 */

        //if ( m_bitrate_info_present_flag[i] )
        //{
        //    put_bits(pH264Bs,             0, 16);      /* avg_bitrate                             */
        //    put_bits(pH264Bs,             0, 16);      /* max_bitrate_layer                       */
        //    put_bits(pH264Bs,             0, 16);      /* max_bitrate_layer_representation        */
        //    put_bits(pH264Bs,             0, 16);      /* max_bitrate_calc_window                 */
        //}

        if(uiFrmRateInfoFlag)
        {
            UINT32 uiAvgFrmRate,uiAvgFrmRate2;
//            uiConstantFrmRateIdc = get_bits(pBstream,2);                  /* constant_frm_bitrate_idc  */
            uiAvgFrmRate         = get_bits(pBstream,16);                 /* avg_frm_rate  */
            uiAvgFrmRate2        = uiAvgFrmRate  * 1000 / 256;
            //debug_msg("FrameRate = %d, %d, %d\r\n", (int)(uiConstantFrmRateIdc), (int)(uiAvgFrmRate), (int)(uiAvgFrmRate2));
            pH264DecSvcObj->uiAvgFrmRate[uiIdx] = uiAvgFrmRate2;
        }

        if(uiFrmSizeInfoFlag)
        {
            UINT32 uiWidthMB,uiHeightMB;
            UINT32 uiWidth,uiHeight;
            uiWidthMB  = read_uvlc_codeword(pBstream);                        /* frm_width_in_mbs_minus1             */
            uiHeightMB = read_uvlc_codeword(pBstream);                        /* frm_height_in_mbs_minus1            */
            uiWidth    = (uiWidthMB + 1) << 4;
            uiHeight   = (uiHeightMB + 1) << 4;
            //debug_msg("uiWidth = %d * %d\r\n", (int)(uiWidth), (int)(uiHeight));
            pH264DecSvcObj->uiWidth = uiWidth;
            pH264DecSvcObj->uiHeight = uiHeight;
        }

        if(uiLayerDpndInfoFlag)
        {
            UINT uiNumDrtDpndLayers; // 0,1,1,...
            UINT uiJdx;

            uiNumDrtDpndLayers  = read_uvlc_codeword(pBstream);              /* num_directly_dependent_layers */
            for(uiJdx = 0; uiJdx < uiNumDrtDpndLayers; uiJdx++)
            {
                UINT32 uiDpntLayerDelta;
                uiDpntLayerDelta  = read_uvlc_codeword(pBstream);            /* directly_dependent_layers_id_delta_minus1 */

                if(uiDpntLayerDelta != 0)
                {
                    DBG_ERR("[parseSEIPayload] Error : uiDpntLayerDelta = %d\r\n", (int)(uiDpntLayerDelta));
                    return H264DEC_FAIL;
                }
            }
        }else{
            //UINT32 uiDpntLayerDelta = 0;
            //uiDpntLayerDelta  = read_uvlc_codeword(pBstream);
        }

        if(uiSPSInfoPstFlag)
        {
            // ...
            DBG_ERR("[parseSEIPayload] Error : uiSPSInfoPstFlag = %d\r\n", (int)(ucVal));
            return H264DEC_FAIL;

        }else{
            UINT32 PSLayerDelta;
            PSLayerDelta  = read_uvlc_codeword(pBstream); /* parameter_sets_info_src_layer_id_delta     */

            if(PSLayerDelta != 0)
            {
                DBG_ERR("[parseSEIPayload] Error : PSLayerDelta = %d\r\n", (int)(PSLayerDelta));
                return H264DEC_FAIL;
            }
        }
    }
    read_rbsp_trailing_bits(pBstream);

    return H264DEC_SUCCESS;
}

static INT32 H264DecSVC(H264DecHdrObj *pH264DecHdrObj, bstream *pBstream)
{
    UINT8 *p;
    UINT8 ucVal;
    H264DecSVCObj *pH264DecSvcObj = &pH264DecHdrObj->sH264DecSvcObj;

    p = pH264DecHdrObj->ucH264DecNalBuf;
    //debug_msg("ucH264DecNalBuf %d %d %d %d %d\r\n", (int)(p[0]), (int)(p[1]), (int)(p[2]), (int)(p[3]), (int)(p[4]));

    if(p[0] != 0 || p[1] != 0 || p[2] != 0 || p[3] != 1)
    {
        DBG_ERR("[H264DecSVC]StartCodec Suffix != 0x000001\r\n");
        return H264DEC_FAIL;
    }

    if(p[4] != NALU_TYPE_SEI)
    {
        //debug_msg("No SVC\r\n");
        pH264DecSvcObj->uiSVCFlag = 0;
        return H264DEC_SUCCESS;
    }

    pH264DecSvcObj->uiSVCFlag = 1;

    if (get_bits(pBstream,32) != 0x00000001){
        DBG_ERR("[H264DecSVC]StartCodec Suffix != 0x000001\r\n");
        return H264DEC_FAIL;
    }

    ucVal = get_bits(pBstream,8) & 0x1F;
    if (ucVal != NALU_TYPE_SEI){
        DBG_ERR("[H264DecSVC] Nal Unit Type != NALU_TYPE_SEI\r\n");
        return H264DEC_FAIL;
    }

    // parse SEI Message
    if (parseSEIMessage( pH264DecHdrObj, pBstream) != H264DEC_SUCCESS){
        DBG_ERR("[H264DecSVC] parse SEI message error\r\n");
        return H264DEC_FAIL;
    }

    if (parseSEIPayload( pH264DecHdrObj, pBstream) != H264DEC_SUCCESS){
        DBG_ERR("[H264DecSVC] parse SEI payload error\r\n");
        return H264DEC_FAIL;
    }

    // parse TailBit
    ucVal = get_bits(pBstream,8);
    //debug_msg("TailBit %d\r\n", (int)(ucVal));
    if(ucVal != 0x80)
    {
        DBG_ERR("[H264DecSVC] parse TailBit error 0x%08x\r\n",ucVal);
        return H264DEC_SUCCESS;
    }

    return H264DEC_SUCCESS;
}

INT32 H264DecSeqHeader(H264DEC_INIT *pH264DecInit, H264DecHdrObj *pH264DecHdrObj, H26XDEC_VUI *pVui){
    H264DecSpsObj *pH264DecSpsObj = &pH264DecHdrObj->sH264DecSpsObj;
    H264DecPpsObj *pH264DecPpsObj = &pH264DecHdrObj->sH264DecPpsObj;
    H264DEC_CODECINFO*pCodecInfo  = &pH264DecInit->CodecInfo;
    UINT32  uiHeaderSize;

    bstream sBstream;
    bstream *pBstream = &sBstream;

    memset(pH264DecHdrObj,  0x0, sizeof(H264DecHdrObj));

	//dec_dump_buf((const unsigned char *)pH264DecInit->uiH264DescAddr, pH264DecInit->uiH264DescSize);
    //uiHeaderSize = ebspTorbsp((UINT8 *)pH264DecInit->uiH264DescAddr, ucH264DecNalBuf, (pH264DecInit->uiH264DescSize * 8));
    uiHeaderSize = ebspTorbsp((UINT8 *)pH264DecInit->uiH264DescAddr, pH264DecHdrObj->ucH264DecNalBuf, pH264DecInit->uiH264DescSize);
	//dec_dump_buf((const unsigned char *)pH264DecHdrObj->ucH264DecNalBuf, uiHeaderSize);

    init_parse_bitstream(pBstream,(void *)pH264DecHdrObj->ucH264DecNalBuf,uiHeaderSize);

    if (H264DecSVC(pH264DecHdrObj,pBstream) != H264DEC_SUCCESS){
        return H264DEC_FAIL;
    }

    if (H264DecSps(pH264DecSpsObj,pBstream, pVui) != H264DEC_SUCCESS){
        return H264DEC_FAIL;
    }

    if (H264DecPps(pH264DecSpsObj,pH264DecPpsObj,pBstream) != H264DEC_SUCCESS){
        return H264DEC_FAIL;
    }

    if ((((pH264DecInit->uiWidth + 15)/16) != pH264DecSpsObj->uiMbWidth) || (((pH264DecInit->uiHeight + 15)/16) != pH264DecSpsObj->uiMbHeight)){
		//DBG_ERR("[H264DecSeqHeader]Decode Init Frame Size(%d, %d) and Decode Sequence Header Frame Size(%d, %d) are not match!\r\n ",
		//		pH264DecInit->uiWidth, pH264DecInit->uiHeight, pH264DecSpsObj->uiMbWidth<<4, pH264DecSpsObj->uiMbHeight<<4);
		DBG_DUMP("[H264DecSeqHeader]Decode Init Frame Size(%d, %d) and Decode Sequence Header Frame Size(%d, %d)\r\n ",
				(int)pH264DecInit->uiWidth, (int)pH264DecInit->uiHeight, (int)(pH264DecSpsObj->uiMbWidth<<4), (int)(pH264DecSpsObj->uiMbHeight<<4));

			//return H264DEC_FAIL;
    }


    // Set Codec Information //
    pCodecInfo->uiProfile     = (H264_PROFILE)pH264DecSpsObj->uiProfileIdc;
    pCodecInfo->uiLevel       = pH264DecSpsObj->uiLevelIdc;
    pCodecInfo->uiRefFrameNum = pH264DecSpsObj->uiNumRefFrm;
	// return init information //
	pH264DecInit->uiWidth        = pH264DecSpsObj->uiMbWidth<<4;
	pH264DecInit->uiHeight       = (pH264DecSpsObj->uiMbHeight<<4) - (pH264DecSpsObj->uiCropBtmOffset<<1);
	pH264DecInit->uiYLineOffset  = SIZE_64X(pH264DecInit->uiWidth)/4;
	pH264DecInit->uiUVLineOffset = SIZE_64X(pH264DecInit->uiWidth)/4;
    pH264DecInit->uiDisplayWidth = (pH264DecSpsObj->uiMbWidth<<4) - (pH264DecSpsObj->uiCropRightOffset<<1);

    //DBG_ERR("[H264DecSeqHeader]Decode uiDisplayWidth = %d!\r\n ", (int)(pH264DecInit->uiDisplayWidth)));

    return H264DEC_SUCCESS;
}


