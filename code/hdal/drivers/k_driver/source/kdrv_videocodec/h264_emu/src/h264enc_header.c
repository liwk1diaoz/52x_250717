/*
    H264ENC module driver for NT96660

    use SeqCfg and PicCfg to setup H264RegSet

    @file       h264enc_int.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#if 0
#include "h26x_common.h"
#include "h26x_bitstream.h"

#include "h26xenc_int.h"
#include "h264enc_int.h"

int gH264FrameNumGapAllow = 1;

static UINT32 write_nal_header(UINT8 *p_addr, UINT32 ref_idc, UINT32 nal_unit_type)
{
	UINT32 i = 0;

	/* Prepend a NAL units with 00 00 00 01 to generate */
	/* Annex B byte stream format bitstreams.           */
	p_addr[i++] = 0x00;
	p_addr[i++] = 0x00;
	p_addr[i++] = 0x00;
	p_addr[i++] = 0x01;

	/* 1                  | 2             | 5             */
	/* forbidden_zero_bit | bitnal_ref_idc| nal_unit_type */
	p_addr[i++] = ((ref_idc & 0x3) << 5) | (nal_unit_type & 0x1F);

	return i;
}

static void write_slice_nal_header(bstream *bs, UINT32 ref_idc, UINT32 nal_unit_type)
{
	put_bits(bs, 1, 32);    // suffix : 00 00 00 01 //
	put_bits(bs, 0, 1);     // forbidden_zero_bit //
	put_bits(bs, ref_idc, 2);
	put_bits(bs, nal_unit_type, 5);
}

static void encode_sps_frm_cropping(bstream *bs, H264EncSeqCfg *pSeqCfg)
{
	UINT32 crop_width = (pSeqCfg->usMbWidth*16 - pSeqCfg->uiDisplayWidth)/2;
	UINT32 crop_bot = (pSeqCfg->usMbHeight*16 - pSeqCfg->usHeight)/2;

	if ((crop_bot != 0) || (crop_width != 0))
	{
		put_bits(bs, 1, 1);		/* frame cropping flag */
		write_uvlc_codeword(bs, 0);			// frame_crop_left_offset
		write_uvlc_codeword(bs, crop_width);// frame_crop_right_offset
		write_uvlc_codeword(bs, 0);   // frame_crop_top_offset
		write_uvlc_codeword(bs, crop_bot);   // frame_crop_bottom_offset
	}
	else
	{
		put_bits(bs, 0, 1);		/* frame cropping flag */
	}
}

static void encode_sps_vui(bstream *bs, H264EncSeqCfg *pSeqCfg)
{
    UINT32 num_units_in_tick, time_scale, fixed_fr_flag = 0;
    UINT32 uiFrameRate = pSeqCfg->uiFrmRate;

    /* aspect_ratio_info_present_flag */
    if (pSeqCfg->usSarWidth == 1 && pSeqCfg->usSarHeight == 1) {
        put_bits(bs, 0, 1);

    } else {
        put_bits(bs,   1,                    1);
        put_bits(bs,   255,                  8);   /* aspect_ratio_idc (Extended_SAR) */
        put_bits(bs,   pSeqCfg->usSarWidth,  16);  /* sar_width                    */
        put_bits(bs,   pSeqCfg->usSarHeight,  16); /* sar_height                   */
    }

    /* overscan_info_presetn_flag */
    put_bits(bs, 0, 1);
#if 0
    /* video_signal_type_presetn_flag */
    put_bits(pH264Bs, 0, 1);
#else
    put_bits(bs, 1, 1); /* video_signal_type_presetn_flag */

    put_bits(bs, pSeqCfg->ucVideoFormat, 3); /* video_format : Component */
    put_bits(bs, pSeqCfg->ucColorRange, 1); /* video_full_range_flag */
    put_bits(bs, 1, 1); /* colour_description_present_flag */
    put_bits(bs, pSeqCfg->ucColourPrimaries, 8); /* colour_primaries */
    put_bits(bs, pSeqCfg->ucTransferCharacteristics, 8); /* transfer_characteristics */
    put_bits(bs, pSeqCfg->ucMatrixCoef, 8); /* matrix_coefficients */
#endif
    /* chroma_loc_info_presetn_flag */
    put_bits(bs, 0, 1);

    if (0 == uiFrameRate) {
        pSeqCfg->bTimeingPresentFlag = 0;
    }
    put_bits(bs, pSeqCfg->bTimeingPresentFlag, 1);
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
                    time_scale        = uiCeilFr * 2; // two ticks per frame
            } else {

                    num_units_in_tick = 1000;
                    time_scale        = (uiFrameRate / 1000) * (num_units_in_tick << 1); // two ticks per frame
            }

        } else {
                num_units_in_tick = 1000;
                time_scale        = uiFrameRate * 2; // two ticks per frame
        }

        put_bits(bs, num_units_in_tick, 32);
        put_bits(bs, time_scale, 32);
        put_bits(bs, fixed_fr_flag, 1); /* fixed_fr_flag */
    }

    /* nal_hrd_presetn_flag */
    put_bits(bs, 0, 1);
    /* vcl_hrd_presetn_flag */
    put_bits(bs, 0, 1);
    /* pic_struct_presetn_flag */
    put_bits(bs, 0, 1);
    /* bitstream_restriction_flag */
    put_bits(bs, 0, 1);
}

static void encode_sps(bstream *bs, H264EncSeqCfg *pSeqCfg)
{
	put_bits(bs, pSeqCfg->eProfile, 8);
	put_bits(bs, 0, 3);    				/* constraint_flag */
	put_bits(bs, 0, 5);    				/* reserved zero   */
	put_bits(bs, pSeqCfg->ucLevelIdc, 8);

	write_uvlc_codeword(bs, 0);			/* seq_parameter_set_id */

	if (pSeqCfg->eProfile == PROFILE_HIGH || pSeqCfg->eProfile == PROFILE_HIGH10)
	{
		write_uvlc_codeword(bs, 1);		/* chroma_format_idc               */
		write_uvlc_codeword(bs, 0);		/* bit_depth_luma_minus8           */
		write_uvlc_codeword(bs, 0);		/* bit_depth_chroma_minus8         */
		put_bits(bs, 0, 1);				/* QPY' zero transform bypass flag */
		put_bits(bs, 0, 1);				/* seq_scaling_matrix_present_flag */
	}
	write_uvlc_codeword(bs, pSeqCfg->usLog2MaxFrm - 4);
	write_uvlc_codeword(bs, 0);			/* pic_order_cnt_type */
	write_uvlc_codeword(bs, pSeqCfg->usLog2MaxPoc - 4);
	write_uvlc_codeword(bs, pSeqCfg->ucNumRefIdxL0);
    put_bits(bs, gH264FrameNumGapAllow, 1);
	//put_bits(bs, 1, 1);					/* gaps_in_frame_num_allowed  */
	write_uvlc_codeword(bs, pSeqCfg->usMbWidth - 1);
	write_uvlc_codeword(bs, pSeqCfg->usMbHeight - 1);
	put_bits(bs, 1, 1);					/* frame_mbs_only_flag  */
	put_bits(bs, pSeqCfg->bDirect8x8En, 1);
	encode_sps_frm_cropping(bs, pSeqCfg);
	put_bits(bs, pSeqCfg->bVUIEn, 1);

	if (pSeqCfg->bVUIEn)
		encode_sps_vui(bs, pSeqCfg);

	write_rbsp_trailing_bits(bs);
}

static void encode_pps(bstream *bs, H264EncSeqCfg *pSeqCfg, H264EncPicCfg *pPicCfg)
{
	write_uvlc_codeword(bs, 0);	/* PPS ID */
	write_uvlc_codeword(bs, 0);	/* SPS ID */
	put_bits(bs, pSeqCfg->eEntropyMode, 1);
	put_bits(bs, 0, 1);			/* pic_order_present_flag */
	write_uvlc_codeword(bs, 0);	/* num_slice_groups_minus1 */
	write_uvlc_codeword(bs, 0);	/* num_ref_idx_l0_active - 1 */
	write_uvlc_codeword(bs, 0);	/* num_ref_idx_l1_active - 1 */
	put_bits(bs, 0, 1);			/* weighted_pred_flag */
	put_bits(bs, 0, 2);			/* weighted_bipred_flag */
	write_signed_uvlc_codeword(bs, 0);	/* pic_init_qp - 26 */
	write_signed_uvlc_codeword(bs, 0);	/* pic_init_qs - 26 */
	write_signed_uvlc_codeword(bs, 0);	/* chroma_qp_idx_offset */
	put_bits(bs, pPicCfg->config_loop_filter, 1);
	put_bits(bs, 0, 1);			/* use_constrained_intra_pred */
	put_bits(bs, 0, 1);			/* redundant_pic_cnt_present */

	if (pSeqCfg->eProfile == PROFILE_HIGH)
	{
		put_bits(bs, pSeqCfg->bTrans8x8En, 1);	/* transform_8x8_mode_flag */
		put_bits(bs, 0, 1);						/* pic_scaling_matrix_present_flag */
		write_signed_uvlc_codeword(bs, 0);		/* (sign)second_croma_qp_index_offset */
	}
	write_rbsp_trailing_bits(bs);
}

//! encode sequence header
/*!
    encode sequence header including sps, pps and vui if needed

    @return
        - @b H264ENC_SUCCESS: init success
        - @b H264ENC_FAIL   : init fail
*/
void h264Enc_encSeqHeader(H264ENC_CTX *pVdoCtx, H26XCOMN_CTX *pComnCtx)
{
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;
	UINT8 *p_addr = (UINT8 *)pComnCtx->stVaAddr.uiSeqHdr;
	UINT8  bs_buf[64];
	bstream bs;

	pVdoCtx->uiSeqHdrLen = write_nal_header(p_addr, 3, NALU_TYPE_SPS);
	init_pack_bitstream(&bs,(void *)bs_buf, 64);
	encode_sps(&bs, pSeqCfg);
	pVdoCtx->uiSeqHdrLen += rbspToebsp(p_addr + pVdoCtx->uiSeqHdrLen, bs_buf, bs_byte_length(&bs));
	pVdoCtx->uiSPSHdrLen = pVdoCtx->uiSeqHdrLen;

	pVdoCtx->uiSeqHdrLen += write_nal_header(p_addr + pVdoCtx->uiSeqHdrLen, 3, NALU_TYPE_PPS);
	init_pack_bitstream(&bs,(void *)bs_buf, 64);
	encode_pps(&bs, pSeqCfg, pPicCfg);
	pVdoCtx->uiSeqHdrLen += rbspToebsp(p_addr + pVdoCtx->uiSeqHdrLen, bs_buf, bs_byte_length(&bs));
	pVdoCtx->uiPPSHdrLen = pVdoCtx->uiSeqHdrLen - pVdoCtx->uiSPSHdrLen;
}

static void ref_pic_list_modification(H264EncSeqCfg *pSeqCfg, H264EncPicCfg *pPicCfg, bstream *bs)
{
	if (0 == pSeqCfg->uiLTRInterval && 2 != pSeqCfg->ucSVCLayer) {
		put_bits(bs, 0, 1);
		return;
	}

	put_bits(bs, 1, 1); // ref_pic_reordering_flag_L0 //

	if (pSeqCfg->uiLTRInterval) {
		if ((pPicCfg->sPoc % pSeqCfg->uiLTRInterval) == 0) {
			write_uvlc_codeword(bs, 2); // modification_of_pic_nums_idc : 2
			write_uvlc_codeword(bs, 0); // long_term_pic_num : 0

			goto end_ref_pic_list;
		}
	}

	if ((pSeqCfg->ucSVCLayer == 2) && ((pPicCfg->sRefPoc % 4) == 0)) {
		if ((pPicCfg->sPoc == 4) && (pSeqCfg->bLTRPreRef == FALSE) && (pSeqCfg->uiLTRInterval != 0)) {
			write_uvlc_codeword(bs, 2); // modification_of_pic_nums_idc : 2
			write_uvlc_codeword(bs, 0); // long_term_pic_num : 0
		}
		else {
			write_uvlc_codeword(bs, 0); // modification_of_pic_nums_idc : 0
			write_uvlc_codeword(bs, 1); // abs_diff_pic_num_minus1 : 0
		}
	}

end_ref_pic_list:
	write_uvlc_codeword(bs, 3); // modification_of_pic_nums_idc : 3
}

static void mark_to_long_term(bstream *bs, UINT32 pic_num)
{
	put_bits(bs, 1, 1);                 // adaptive_ref_pic_marking_mode_flag  : set to 1 , enable mmco  //
	write_uvlc_codeword(bs, 3);         // mmco : 3
	write_uvlc_codeword(bs, pic_num);   // st pic_num;
	write_uvlc_codeword(bs, 0);         // set to LT pic_num = 0;
	write_uvlc_codeword(bs, 0);         // mmco : 0 // end mmco
}

static void dec_ref_pic_marking(H264EncSeqCfg *pSeqCfg, H264EncPicCfg *pPicCfg, bstream *bs)
{
	if (pPicCfg->ePicType == IDR_SLICE) {
		put_bits(bs, 0, 1); // no_output_of_prior_pics_flag //
		put_bits(bs, ((pSeqCfg->uiLTRInterval) && (pSeqCfg->bLTRPreRef == FALSE)), 1); // long_term_reference_flag //
	} else {
		if (pSeqCfg->uiLTRInterval) {
			INT32 last_short_term = 1 << pSeqCfg->ucSVCLayer;

			if (pSeqCfg->bLTRPreRef) {
				if ((pPicCfg->sPoc % (INT32)pSeqCfg->uiLTRInterval) == last_short_term) {
					mark_to_long_term(bs, (pSeqCfg->ucSVCLayer == 2));
				} else {
					put_bits(bs, 0, 1);    // adaptive_ref_pic_marking_mode_flag  : set to 1 , disable mmco  //
				}
			} else {
				put_bits(bs, 0, 1);    // adaptive_ref_pic_marking_mode_flag  : set to 1 , disable mmco  //
			}
		} else {
			put_bits(bs, 0, 1);    // adaptive_ref_pic_marking_mode_flag  : set to 1 , disable mmco  //
		}
	}
}

void h264Enc_encSliceHeader(H264ENC_CTX *pVdoCtx, H26XFUNC_CTX *pFuncCtx, H26XCOMN_CTX *pComnCtx, BOOL bGetSeqHdrEn)
{
	H264EncSeqCfg *pSeqCfg = &pVdoCtx->stSeqCfg;
	H264EncPicCfg *pPicCfg = &pVdoCtx->stPicCfg;

	bstream sH264Bs;
	bstream *bs = &sH264Bs;
	UINT32 nal_unit_type, nal_ref_idc;
	UINT32 i;
	UINT32 *bsdma_addr = (UINT32 *)pComnCtx->stVaAddr.uiBsdma;
	UINT32 uiPicInfoLen = 0;

	bsdma_addr[0] = pFuncCtx->stSliceSplit.uiNaluNum;

	pPicCfg->uiPicHdrLen = 0;
	pPicCfg->sRefPoc = (pSeqCfg->uiLTRInterval) ? (INT16)(pPicCfg->sPoc % pSeqCfg->uiLTRInterval) : pPicCfg->sPoc;

	if (pPicCfg->ePicType == IDR_SLICE) {
		nal_unit_type = NALU_TYPE_IDR;
		nal_ref_idc   = NALU_PRIORITY_HIGHEST;

		pPicCfg->usFrmNum = 0;
        pPicCfg->sPoc = 0;
        pPicCfg->sRefPoc = 0;
	} else {
		nal_unit_type = NALU_TYPE_SLICE;

		if (IS_ISLICE(pPicCfg->ePicType)) {
			nal_ref_idc = NALU_PRIORITY_HIGH;
		} else {
			if (pSeqCfg->ucSVCLayer) {
				if (pPicCfg->sRefPoc % 2) {
					nal_ref_idc = NALU_PRIORITY_DISPOSABLE;
				} else if (pPicCfg->sRefPoc % 4) {
					nal_ref_idc = (pSeqCfg->uiLTRInterval != 0 && pSeqCfg->ucSVCLayer == 1) ? NALU_PRIORITY_LOW : NALU_PRIORITY_HIGH + (pSeqCfg->ucSVCLayer == 1) - (pSeqCfg->uiLTRInterval != 0);
				} else {
					nal_ref_idc = (pSeqCfg->uiLTRInterval != 0 && pSeqCfg->ucSVCLayer == 1) ? NALU_PRIORITY_LOW : NALU_PRIORITY_HIGHEST;
				}
			} else {
				nal_ref_idc = NALU_PRIORITY_LOW;
			}

			if (pSeqCfg->uiLTRInterval) {
				if (pPicCfg->sRefPoc == 0) {
					nal_ref_idc = NALU_PRIORITY_HIGH;
				}
			}
		}
	}
	//DBG_DUMP("frm = %d, poc = %d, %d\r\n", (int)(pPicCfg->uiFrmNum), (int)(pPicCfg->iPoc), (int)(nal_ref_idc));
	#if 0
	init_pack_bitstream(bs, (UINT8 *)(pComnCtx->stVaAddr.uiPicHdr), H264ENC_HEADER_MAXSIZE);
	if (bGetSeqHdrEn)
	{
		UINT8 *addr = (UINT8 *)pComnCtx->stVaAddr.uiSeqHdr;
		UINT32 i;

		for (i = 0; i < pVdoCtx->uiSeqHdrLen; i++)
			put_bits(bs, addr[i], 8);
	}
	#else
	if (bGetSeqHdrEn) {
		memcpy((void *)pComnCtx->stVaAddr.uiPicHdr, (void *)pComnCtx->stVaAddr.uiSeqHdr, pVdoCtx->uiSeqHdrLen);
		uiPicInfoLen += pVdoCtx->uiSeqHdrLen;
	}
	if ((g_rc_dump_log == 1) && (pComnCtx->uiRcLogLen != 0)) {
		#if 0
		char sei_test[32] = "hello world";

		UINT8 *addr = (UINT8 *)(pComnCtx->stVaAddr.uiPicHdr + uiPicInfoLen);
		UINT32 idx  = write_nal_header(addr, 0, 6);

		addr[idx++] = 0x55;

		addr[idx] = rbspToebsp(addr + idx + 1, sei_test, strlen(sei_test));
		addr[idx + addr[idx] + 1] = 0x80;

		uiPicInfoLen += (idx + addr[idx] + 2);
		#else
		UINT8 *addr = (UINT8 *)(pComnCtx->stVaAddr.uiPicHdr + uiPicInfoLen);
		UINT32 idx  = write_nal_header(addr, 0, 6);
		UINT32 bsplen;

		addr[idx++] = 0x55;
		addr[idx++] = pComnCtx->uiRcLogLen;
		bsplen = rbspToebsp((addr + idx), (UINT8 *)pComnCtx->uipRcLogAddr, pComnCtx->uiRcLogLen);
		addr[idx + bsplen] = 0x80;

		uiPicInfoLen += (idx + bsplen + 1);
		#endif
	}
	init_pack_bitstream(bs, (UINT8 *)(pComnCtx->stVaAddr.uiPicHdr + uiPicInfoLen), (H264ENC_HEADER_MAXSIZE - uiPicInfoLen));
	#endif

	for (i = 0; i < bsdma_addr[0]; i++) {
        UINT32 bit_res;
		write_slice_nal_header(bs, nal_ref_idc, nal_unit_type);

		write_uvlc_codeword(bs, (i * pFuncCtx->stSliceSplit.stCfg.uiSliceRowNum * pSeqCfg->usMbWidth)); // first_mb_in_slice //

		if (pPicCfg->ePicType == IDR_SLICE)
			write_uvlc_codeword(bs, (I_SLICE + 5));  // support frame mode only //
		else
			write_uvlc_codeword(bs, (pPicCfg->ePicType + 5));  // support frame mode only //
		write_uvlc_codeword(bs, 0); // pps_id : only 0 //
		put_bits(bs, pPicCfg->usFrmNum, pSeqCfg->usLog2MaxFrm);

		if (pPicCfg->ePicType == IDR_SLICE) {
			write_uvlc_codeword(bs, (pPicCfg->usFrmForIdrPicId % 2));
		}
		put_bits(bs, ((pPicCfg->sPoc<<1) & ~((((UINT32)(-1)) << pSeqCfg->usLog2MaxPoc))), pSeqCfg->usLog2MaxPoc);    // pic_odr_cnt_lsb //

		if (!IS_ISLICE(pPicCfg->ePicType)) {
			put_bits(bs, 0, 1); // num_ref_idx_active_override_flag //

			ref_pic_list_modification(pSeqCfg, pPicCfg, bs);
		}

		if (nal_ref_idc) {
			dec_ref_pic_marking(pSeqCfg, pPicCfg, bs);
		}

		if (pSeqCfg->eEntropyMode == CABAC && !IS_ISLICE(pPicCfg->ePicType)) {
			write_uvlc_codeword(bs, pPicCfg->cabac_init_idc);
		}

		write_signed_uvlc_codeword(bs, (pPicCfg->ucSliceQP - 26));   // slice_qp_delta //

		if (pPicCfg->config_loop_filter) {
			write_uvlc_codeword(bs, pSeqCfg->ucDisLFIdc);

			if (pSeqCfg->ucDisLFIdc != 1) {
				write_signed_uvlc_codeword(bs, pSeqCfg->cDBAlpha);
				write_signed_uvlc_codeword(bs, pSeqCfg->cDBBeta);
			}
		}

		//  cabac alignment one bit //
		bit_res = bit_length(bs) % 8;
		if (bit_res == 0) bit_res = 8;
		if (pSeqCfg->eEntropyMode == CABAC) {
			while (bit_res != 8) {
				put_bits(bs, 1, 1);
				bit_res++;
			}
		}
		else
		{
			if (bit_res != 8) put_bits(bs, 0, (8 - bit_res));
		}
		#if 0
		bsdma_addr[i*2 + 1] = h26x_getPhyAddr(pComnCtx->stVaAddr.uiPicHdr) + pPicCfg->uiPicHdrLen;
		bsdma_addr[i*2 + 2] = (bs_byte_length(bs) - pPicCfg->uiPicHdrLen) | (bit_res<<28);
		#else
		if (i == 0) {
			bsdma_addr[1] = h26x_getPhyAddr(pComnCtx->stVaAddr.uiPicHdr);
			bsdma_addr[2] = (bs_byte_length(bs) + uiPicInfoLen) | (bit_res<<28);;
		}
		else {
			bsdma_addr[i*2 + 1] = h26x_getPhyAddr(pComnCtx->stVaAddr.uiPicHdr) + uiPicInfoLen + pPicCfg->uiPicHdrLen;
			bsdma_addr[i*2 + 2] = (bs_byte_length(bs) - pPicCfg->uiPicHdrLen) | (bit_res<<28);
		}
		#endif
		pPicCfg->uiPicHdrLen = bs_byte_length(bs);
	}

	pPicCfg->uiPicHdrLen += uiPicInfoLen;
	pPicCfg->uiNalRefIdc = nal_ref_idc;

	h26x_flushCache(pComnCtx->stVaAddr.uiBsdma, (pFuncCtx->stSliceSplit.uiNaluNum*2 + 1)*4);
	h26x_flushCache(pComnCtx->stVaAddr.uiPicHdr, pPicCfg->uiPicHdrLen);
}
#endif
