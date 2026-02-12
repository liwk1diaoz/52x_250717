#include "ImageApp_MovieMulti_int.h"
#include "FileDB.h"

/// ========== Cross file variables ==========
UINT32 g_ia_moviemulti_filedb_filter = (FILEDB_FMT_MOV | FILEDB_FMT_MP4 | FILEDB_FMT_TS | FILEDB_FMT_JPG | FILEDB_FMT_RAW);
UINT32 g_ia_moviemulti_filedb_max_num = 10000;
UINT32 g_ia_moviemulti_delete_filter = (FILEDB_FMT_MOV | FILEDB_FMT_MP4 | FILEDB_FMT_TS | FILEDB_FMT_JPG | FILEDB_FMT_RAW);
UINT32 g_ia_moviemulti_crash_not_ro = FALSE;
UINT32 g_ia_moviemulti_emr_not_ro = FALSE;
UINT32 iamovie_gps_buffer_size = MAX_GPS_DATA_SIZE;
UINT32 iamovie_encrypt_type = MOVIEMULTI_ENCRYPT_TYPE_NONE;
UINT32 iamovie_encrypt_mode = MOVIEMULTI_ENCRYPT_MODE_AES128;
UINT8  iamovie_encrypt_key[32] = {0};

/// ========== Noncross file variables ==========


ER _ImageApp_MovieMulti_SetupAudioParam(void)
{
	UINT32 idx = 0;
	UINT32 i, j;
	UINT32 enlarge_acap_buf_num = FALSE;
	UINT32 acap_buf_num = 10;

	for (i = 0; i < MaxLinkInfo.MaxImgLink; i++) {
		for (j = 0; j < 2; j++) {
			if (IsSetImgLinkRecInfo[i][j] == TRUE) {
				if (ImgLinkRecInfo[i][j].aud_codec == _CFG_AUD_CODEC_PCM) {
					enlarge_acap_buf_num = TRUE;
				}
			}
		}
	}
	if (AudCapBufMs[0]) {
		acap_buf_num = AudCapBufMs[0] * g_AudioInfo.aud_rate / 1024000;
		if ((AudCapBufMs[0] * g_AudioInfo.aud_rate) % 1024000) {
			acap_buf_num ++;
		}
	} else if (enlarge_acap_buf_num) {
		acap_buf_num = 3000 * g_AudioInfo.aud_rate / 1024000;
		if ((3000 * g_AudioInfo.aud_rate) % 1024000) {
			acap_buf_num ++;
		}
	}
	if (acap_buf_num < 10) {
		acap_buf_num = 10;
	}

	aud_acap_drv_cfg[idx].mono                  = (g_AudioInfo.aud_ch == _CFG_AUDIO_CH_RIGHT) ? HD_AUDIO_MONO_RIGHT : HD_AUDIO_MONO_LEFT;

	aud_acap_dev_cfg[idx].in_max.sample_rate    = g_AudioInfo.aud_rate;
	aud_acap_dev_cfg[idx].in_max.sample_bit     = HD_AUDIO_BIT_WIDTH_16;
	aud_acap_dev_cfg[idx].in_max.mode           = (g_AudioInfo.aud_ch_num == 2) ? HD_AUDIO_SOUND_MODE_STEREO : HD_AUDIO_SOUND_MODE_MONO;
	aud_acap_dev_cfg[idx].in_max.frame_sample   = 1024;
	aud_acap_dev_cfg[idx].frame_num_max         = acap_buf_num;

	if (aud_acap_anr_param[0].enabled == TRUE) {
		aud_acap_dev_cfg[idx].anr_max.enabled          = aud_acap_anr_param[0].enabled;
		aud_acap_dev_cfg[idx].anr_max.suppress_level   = aud_acap_anr_param[0].suppress_level;
		aud_acap_dev_cfg[idx].anr_max.hpf_cut_off_freq = aud_acap_anr_param[0].hpf_cut_off_freq;
		aud_acap_dev_cfg[idx].anr_max.bias_sensitive   = aud_acap_anr_param[0].bias_sensitive;
	}

	if (aud_acap_aec_param[0].enabled == TRUE) {
		aud_acap_dev_cfg[idx].aec_max.enabled               = aud_acap_aec_param[0].enabled;
		aud_acap_dev_cfg[idx].aec_max.leak_estimate_enabled = aud_acap_aec_param[0].leak_estimate_enabled;
		aud_acap_dev_cfg[idx].aec_max.leak_estimate_value   = aud_acap_aec_param[0].leak_estimate_value;
		aud_acap_dev_cfg[idx].aec_max.echo_cancel_level     = aud_acap_aec_param[0].echo_cancel_level;
		aud_acap_dev_cfg[idx].aec_max.noise_cancel_level    = aud_acap_aec_param[0].noise_cancel_level ;
		aud_acap_dev_cfg[idx].aec_max.filter_length         = aud_acap_aec_param[0].filter_length ;
		aud_acap_dev_cfg[idx].aec_max.frame_size            = aud_acap_aec_param[0].frame_size ;
		aud_acap_dev_cfg[idx].aec_max.notch_radius          = aud_acap_aec_param[0].notch_radius ;
		aud_acap_dev_cfg[idx].aec_max.lb_channel            = aud_acap_aec_param[0].lb_channel ;
	}

	aud_acap_param[idx].sample_rate             = g_AudioInfo.aud_rate;
	aud_acap_param[idx].sample_bit              = HD_AUDIO_BIT_WIDTH_16;
	aud_acap_param[idx].mode                    = (g_AudioInfo.aud_ch_num == 2) ? HD_AUDIO_SOUND_MODE_STEREO : HD_AUDIO_SOUND_MODE_MONO;
	aud_acap_param[idx].frame_sample            = 1024;

	if (AudEncCodec[0] == _CFG_AUD_CODEC_ULAW) {
		aud_aenc_cfg[idx].max_mem.codec_type        = HD_AUDIO_CODEC_ULAW;
	} else if (AudEncCodec[0] == _CFG_AUD_CODEC_ALAW) {
		aud_aenc_cfg[idx].max_mem.codec_type        = HD_AUDIO_CODEC_ALAW;
	} else {
		aud_aenc_cfg[idx].max_mem.codec_type        = HD_AUDIO_CODEC_AAC;
	}
	aud_aenc_cfg[idx].max_mem.sample_rate       = g_AudioInfo.aud_rate;
	aud_aenc_cfg[idx].max_mem.sample_bit        = HD_AUDIO_BIT_WIDTH_16;
	aud_aenc_cfg[idx].max_mem.mode              = (g_AudioInfo.aud_ch_num == 2) ? HD_AUDIO_SOUND_MODE_STEREO : HD_AUDIO_SOUND_MODE_MONO;
	aud_aenc_cfg[idx].max_mem.buf_ms            = AudEncBufMs[idx];

	aud_aenc_in_param[idx].sample_rate          = g_AudioInfo.aud_rate;
	aud_aenc_in_param[idx].sample_bit           = HD_AUDIO_BIT_WIDTH_16;
	aud_aenc_in_param[idx].mode                 = (g_AudioInfo.aud_ch_num == 2) ? HD_AUDIO_SOUND_MODE_STEREO : HD_AUDIO_SOUND_MODE_MONO;

	if (AudEncCodec[0] == _CFG_AUD_CODEC_ULAW) {
		aud_aenc_out_param[idx].codec_type          = HD_AUDIO_CODEC_ULAW;
		aud_aenc_out_param[idx].aac_adts            = FALSE;
	} else if (AudEncCodec[0] == _CFG_AUD_CODEC_ALAW) {
		aud_aenc_out_param[idx].codec_type          = HD_AUDIO_CODEC_ALAW;
		aud_aenc_out_param[idx].aac_adts            = FALSE;
	} else {
		aud_aenc_out_param[idx].codec_type          = HD_AUDIO_CODEC_AAC;
		aud_aenc_out_param[idx].aac_adts            = TRUE;
	}

	return E_OK;
}

ER _ImageApp_MovieMulti_SetupRecParam(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	UINT32 i = id, j;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (img_venc_maxmem[tb.idx][tb.tbl].bitrate && img_venc_maxmem[tb.idx][tb.tbl].enc_buf_ms) {
			memcpy((void *)&(img_venc_path_cfg[tb.idx][tb.tbl].max_mem), (void *)&(img_venc_maxmem[tb.idx][tb.tbl]), sizeof(HD_VIDEOENC_MAXMEM));
		} else {
			if (ImgLinkRecInfo[tb.idx][tb.tbl].codec == _CFG_CODEC_MJPG) {
				img_venc_path_cfg[tb.idx][tb.tbl].max_mem.codec_type          = HD_CODEC_TYPE_JPEG;
			} else if (ImgLinkRecInfo[tb.idx][tb.tbl].codec == _CFG_CODEC_H264) {
				img_venc_path_cfg[tb.idx][tb.tbl].max_mem.codec_type          = HD_CODEC_TYPE_H264;
			} else {
				img_venc_path_cfg[tb.idx][tb.tbl].max_mem.codec_type          = HD_CODEC_TYPE_H265;
			}
			img_venc_path_cfg[tb.idx][tb.tbl].max_mem.max_dim.w           = ImgLinkRecInfo[tb.idx][tb.tbl].size.w;
			img_venc_path_cfg[tb.idx][tb.tbl].max_mem.max_dim.h           = ImgLinkRecInfo[tb.idx][tb.tbl].size.h;
			img_venc_path_cfg[tb.idx][tb.tbl].max_mem.bitrate             = ImgLinkRecInfo[tb.idx][tb.tbl].target_bitrate * 8;       // project setting is byte rate
			if (tb.tbl == MOVIETYPE_MAIN) {
				img_venc_path_cfg[tb.idx][tb.tbl].max_mem.enc_buf_ms          = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? (ImgLinkVdoEncBufMs[tb.idx][tb.tbl] + ImgLinkFileInfo.emr_sec * 1000) : ImgLinkVdoEncBufMs[tb.idx][tb.tbl];
			} else {
				img_venc_path_cfg[tb.idx][tb.tbl].max_mem.enc_buf_ms          = (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) ? (ImgLinkVdoEncBufMs[tb.idx][tb.tbl] + ImgLinkFileInfo.clone_emr_sec * 1000) : ImgLinkVdoEncBufMs[tb.idx][tb.tbl];
			}

			img_venc_path_cfg[tb.idx][tb.tbl].max_mem.svc_layer           = img_venc_svc[tb.idx][tb.tbl];
			img_venc_path_cfg[tb.idx][tb.tbl].max_mem.ltr                 = FALSE;
			img_venc_path_cfg[tb.idx][tb.tbl].max_mem.rotate              = FALSE;
			img_venc_path_cfg[tb.idx][tb.tbl].max_mem.source_output       = FALSE;
		}
		img_venc_path_cfg[tb.idx][tb.tbl].isp_id                      = 0;

		img_venc_in_param[tb.idx][tb.tbl].dim.w                       = ImgLinkRecInfo[tb.idx][tb.tbl].size.w;
		img_venc_in_param[tb.idx][tb.tbl].dim.h                       = ImgLinkRecInfo[tb.idx][tb.tbl].size.h;
		if (tb.tbl == MOVIETYPE_MAIN && img_vproc_yuv_compress[tb.idx]) {
			img_venc_in_param[tb.idx][tb.tbl].pxl_fmt                     = HD_VIDEO_PXLFMT_YUV420_NVX2;
		} else {
			img_venc_in_param[tb.idx][tb.tbl].pxl_fmt                     = HD_VIDEO_PXLFMT_YUV420;
		}
		img_venc_in_param[tb.idx][tb.tbl].dir                         = HD_VIDEO_DIR_NONE;
		if (img_venc_trigger_mode[tb.idx][tb.tbl] == MOVIE_CODEC_TRIGGER_TIMER) {
			img_venc_in_param[tb.idx][tb.tbl].frc                         = _ImageApp_MovieMulti_frc(ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate, 1);
		} else {
			img_venc_in_param[tb.idx][tb.tbl].frc                         = _ImageApp_MovieMulti_frc(1, 1);
		}

		if (ImgLinkRecInfo[tb.idx][tb.tbl].codec == _CFG_CODEC_MJPG) {
			img_venc_out_param[tb.idx][tb.tbl].codec_type                 = HD_CODEC_TYPE_JPEG;
			img_venc_out_param[tb.idx][tb.tbl].jpeg.retstart_interval     = 0;
			img_venc_out_param[tb.idx][tb.tbl].jpeg.image_quality         = 50;
			img_venc_out_param[tb.idx][tb.tbl].jpeg.bitrate               = 0;
			img_venc_out_param[tb.idx][tb.tbl].jpeg.frame_rate_base       = 1;
			img_venc_out_param[tb.idx][tb.tbl].jpeg.frame_rate_incr       = 1;
		} else if (ImgLinkRecInfo[tb.idx][tb.tbl].codec == _CFG_CODEC_H264) {
			img_venc_out_param[tb.idx][tb.tbl].codec_type                 = HD_CODEC_TYPE_H264;
			img_venc_out_param[tb.idx][tb.tbl].h26x.gop_num               = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiGOP ? ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiGOP : 15;
			img_venc_out_param[tb.idx][tb.tbl].h26x.source_output         = DISABLE;
			img_venc_out_param[tb.idx][tb.tbl].h26x.profile               = img_venc_h264_profile[tb.idx][tb.tbl] ? img_venc_h264_profile[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H264_PROFILE;
			img_venc_out_param[tb.idx][tb.tbl].h26x.level_idc             = img_venc_h264_level[tb.idx][tb.tbl] ? img_venc_h264_level[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H264_LEVEL;
			img_venc_out_param[tb.idx][tb.tbl].h26x.svc_layer             = img_venc_svc[tb.idx][tb.tbl];
			img_venc_out_param[tb.idx][tb.tbl].h26x.entropy_mode          = (img_venc_h264_profile[tb.idx][tb.tbl] != _CFG_H264_PROFILE_BASELINE) ? HD_H264E_CABAC_CODING : HD_H264E_CAVLC_CODING;
		} else {
			img_venc_out_param[tb.idx][tb.tbl].codec_type                 = HD_CODEC_TYPE_H265;
			img_venc_out_param[tb.idx][tb.tbl].h26x.gop_num               = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiGOP ? ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiGOP : 15;
			img_venc_out_param[tb.idx][tb.tbl].h26x.source_output         = DISABLE;
			img_venc_out_param[tb.idx][tb.tbl].h26x.profile               = img_venc_h265_profile[tb.idx][tb.tbl] ? img_venc_h265_profile[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H265_PROFILE;
			img_venc_out_param[tb.idx][tb.tbl].h26x.level_idc             = img_venc_h265_level[tb.idx][tb.tbl] ? img_venc_h265_level[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H265_LEVEL;
			img_venc_out_param[tb.idx][tb.tbl].h26x.svc_layer             = img_venc_svc[tb.idx][tb.tbl];
			img_venc_out_param[tb.idx][tb.tbl].h26x.entropy_mode          = HD_H265E_CABAC_CODING;
		}

		img_venc_rc_param[tb.idx][tb.tbl].rc_mode                     = HD_RC_MODE_CBR;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.bitrate                 = ImgLinkRecInfo[tb.idx][tb.tbl].target_bitrate * 8;       // project setting is byte rate
		img_venc_rc_param[tb.idx][tb.tbl].cbr.frame_rate_base         = ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.frame_rate_incr         = 1;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.init_i_qp               = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiInitIQp;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.max_i_qp                = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiMaxIQp;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.min_i_qp                = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiMinIQp;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.init_p_qp               = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiInitPQp;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.max_p_qp                = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiMaxPQp;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.min_p_qp                = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiMinPQp;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.static_time             = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiStaticTime;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.ip_weight               = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.iIPWeight;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.key_p_period            = 0;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.kp_weight               = 0;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.p2_weight               = 0;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.p3_weight               = 0;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.lt_weight               = 0;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.motion_aq_str           = 0;
		img_venc_rc_param[tb.idx][tb.tbl].cbr.max_frame_size          = img_venc_max_frame_size[tb.idx][tb.tbl];

		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(id, MOVIE_USER_CB_EVENT_SET_ENC_RC_INFO, (UINT32)&(img_venc_rc_param[tb.idx][tb.tbl]));
		}

		for (j = 0; j < 2; j ++) {
			img_bsmux_vinfo[tb.idx][2*tb.tbl+j].vidcodec                  = ImgLinkRecInfo[tb.idx][tb.tbl].codec;
			img_bsmux_vinfo[tb.idx][2*tb.tbl+j].vfr                       = ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate;
			img_bsmux_vinfo[tb.idx][2*tb.tbl+j].width                     = ImgLinkRecInfo[tb.idx][tb.tbl].size.w;
			img_bsmux_vinfo[tb.idx][2*tb.tbl+j].height                    = ImgLinkRecInfo[tb.idx][tb.tbl].size.h;
			img_bsmux_vinfo[tb.idx][2*tb.tbl+j].tbr                       = ImgLinkRecInfo[tb.idx][tb.tbl].target_bitrate;   // project setting is byte rate
			img_bsmux_vinfo[tb.idx][2*tb.tbl+j].DAR                       = ImgLinkRecInfo[tb.idx][tb.tbl].dar;
			img_bsmux_vinfo[tb.idx][2*tb.tbl+j].gop                       = ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiGOP ? ImgLinkRecInfo[tb.idx][tb.tbl].cbr_info.uiGOP : 15;
			if (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_AAC) {
				img_bsmux_ainfo[tb.idx][2*tb.tbl+j].codectype                 = MOVAUDENC_AAC;
			} else if (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ULAW) {
				img_bsmux_ainfo[tb.idx][2*tb.tbl+j].codectype                 = MOVAUDENC_ULAW;
			} else if (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ALAW) {
				img_bsmux_ainfo[tb.idx][2*tb.tbl+j].codectype                 = MOVAUDENC_ALAW;
			} else {
				img_bsmux_ainfo[tb.idx][2*tb.tbl+j].codectype                 = MOVAUDENC_PPCM;
			}
			img_bsmux_ainfo[tb.idx][2*tb.tbl+j].chs                       = g_AudioInfo.aud_ch_num;
			img_bsmux_ainfo[tb.idx][2*tb.tbl+j].asr                       = g_AudioInfo.aud_rate;
			img_bsmux_ainfo[tb.idx][2*tb.tbl+j].aud_en                    = (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_AV) ? TRUE : FALSE;
			img_bsmux_ainfo[tb.idx][2*tb.tbl+j].adts_bytes                = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_AAC) ? 7 : 0;
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (ethcam_venc_maxmem[tb.idx].bitrate && ethcam_venc_maxmem[tb.idx].enc_buf_ms) {
			memcpy((void *)&(ethcam_venc_path_cfg[tb.idx].max_mem), (void *)&(ethcam_venc_maxmem[tb.idx]), sizeof(HD_VIDEOENC_MAXMEM));
		} else {
			if (EthCamLinkRecInfo[tb.idx].codec == _CFG_CODEC_MJPG) {
				ethcam_venc_path_cfg[tb.idx].max_mem.codec_type          = HD_CODEC_TYPE_JPEG;
			} else if (EthCamLinkRecInfo[tb.idx].codec == _CFG_CODEC_H264) {
				ethcam_venc_path_cfg[tb.idx].max_mem.codec_type          = HD_CODEC_TYPE_H264;
			} else {
				ethcam_venc_path_cfg[tb.idx].max_mem.codec_type          = HD_CODEC_TYPE_H265;
			}
			ethcam_venc_path_cfg[tb.idx].max_mem.max_dim.w           = EthCamLinkRecInfo[tb.idx].width;
			ethcam_venc_path_cfg[tb.idx].max_mem.max_dim.h           = EthCamLinkRecInfo[tb.idx].height;
			ethcam_venc_path_cfg[tb.idx].max_mem.bitrate             = EthCamLinkRecInfo[tb.idx].tbr * 8;       // project setting is byte rate
			ethcam_venc_path_cfg[tb.idx].max_mem.enc_buf_ms          = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? (EthCamLinkVdoEncBufMs[tb.idx] + ImgLinkFileInfo.emr_sec * 1000) : EthCamLinkVdoEncBufMs[tb.idx];
			ethcam_venc_path_cfg[tb.idx].max_mem.svc_layer           = ethcam_venc_svc[tb.idx];
			ethcam_venc_path_cfg[tb.idx].max_mem.ltr                 = FALSE;
			ethcam_venc_path_cfg[tb.idx].max_mem.rotate              = FALSE;
			ethcam_venc_path_cfg[tb.idx].max_mem.source_output       = FALSE;
		}
		ethcam_venc_path_cfg[tb.idx].isp_id                      = 0;

		ethcam_venc_in_param[tb.idx].dim.w                       = EthCamLinkRecInfo[tb.idx].width;
		ethcam_venc_in_param[tb.idx].dim.h                       = EthCamLinkRecInfo[tb.idx].height;
		ethcam_venc_in_param[tb.idx].pxl_fmt                     = HD_VIDEO_PXLFMT_YUV420;
		ethcam_venc_in_param[tb.idx].dir                         = HD_VIDEO_DIR_NONE;
		if (ethcam_venc_trigger_mode[tb.idx] == MOVIE_CODEC_TRIGGER_TIMER) {
			ethcam_venc_in_param[tb.idx].frc                         = _ImageApp_MovieMulti_frc(EthCamLinkRecInfo[tb.idx].vfr, 1);
		} else {
			ethcam_venc_in_param[tb.idx].frc                         = _ImageApp_MovieMulti_frc(1, 1);
		}

		if (EthCamLinkRecInfo[tb.idx].codec == _CFG_CODEC_MJPG) {
			ethcam_venc_out_param[tb.idx].codec_type                 = HD_CODEC_TYPE_JPEG;
			ethcam_venc_out_param[tb.idx].jpeg.retstart_interval     = 0;
			ethcam_venc_out_param[tb.idx].jpeg.image_quality         = 50;
			ethcam_venc_out_param[tb.idx].jpeg.bitrate               = 0;
			ethcam_venc_out_param[tb.idx].jpeg.frame_rate_base       = 1;
			ethcam_venc_out_param[tb.idx].jpeg.frame_rate_incr       = 1;
		} else if (EthCamLinkRecInfo[tb.idx].codec == _CFG_CODEC_H264) {
			ethcam_venc_out_param[tb.idx].codec_type                 = HD_CODEC_TYPE_H264;
			ethcam_venc_out_param[tb.idx].h26x.gop_num               = EthCamLinkRecInfo[tb.idx].gop ? EthCamLinkRecInfo[tb.idx].gop : 15;
			ethcam_venc_out_param[tb.idx].h26x.source_output         = DISABLE;
			ethcam_venc_out_param[tb.idx].h26x.profile               = ethcam_venc_h264_profile[tb.idx] ? ethcam_venc_h264_profile[tb.idx] : IAMVOIE_DEFAULT_H264_PROFILE;
			ethcam_venc_out_param[tb.idx].h26x.level_idc             = ethcam_venc_h264_level[tb.idx] ? ethcam_venc_h264_level[tb.idx] : IAMVOIE_DEFAULT_H264_LEVEL;
			ethcam_venc_out_param[tb.idx].h26x.svc_layer             = ethcam_venc_svc[tb.idx];
			ethcam_venc_out_param[tb.idx].h26x.entropy_mode          = (ethcam_venc_h264_profile[tb.idx] != _CFG_H264_PROFILE_BASELINE) ? HD_H264E_CABAC_CODING : HD_H264E_CAVLC_CODING;
		} else {
			ethcam_venc_out_param[tb.idx].codec_type                 = HD_CODEC_TYPE_H265;
			ethcam_venc_out_param[tb.idx].h26x.gop_num               = EthCamLinkRecInfo[tb.idx].gop ? EthCamLinkRecInfo[tb.idx].gop : 15;
			ethcam_venc_out_param[tb.idx].h26x.source_output         = DISABLE;
			ethcam_venc_out_param[tb.idx].h26x.profile               = ethcam_venc_h265_profile[tb.idx] ? ethcam_venc_h265_profile[tb.idx] : IAMVOIE_DEFAULT_H265_PROFILE;
			ethcam_venc_out_param[tb.idx].h26x.level_idc             = ethcam_venc_h265_level[tb.idx] ? ethcam_venc_h265_level[tb.idx] : IAMVOIE_DEFAULT_H265_LEVEL;
			ethcam_venc_out_param[tb.idx].h26x.svc_layer             = ethcam_venc_svc[tb.idx];
			ethcam_venc_out_param[tb.idx].h26x.entropy_mode          = HD_H265E_CABAC_CODING;
		}

		ethcam_venc_rc_param[tb.idx].rc_mode                     = HD_RC_MODE_CBR;
		ethcam_venc_rc_param[tb.idx].cbr.bitrate                 = EthCamLinkRecInfo[tb.idx].tbr * 8;       // project setting is byte rate
		ethcam_venc_rc_param[tb.idx].cbr.frame_rate_base         = EthCamLinkRecInfo[tb.idx].vfr;
		ethcam_venc_rc_param[tb.idx].cbr.frame_rate_incr         = 1;
		ethcam_venc_rc_param[tb.idx].cbr.init_i_qp               = EthCamLinkRecInfo[tb.idx].cbr_info.uiInitIQp;
		ethcam_venc_rc_param[tb.idx].cbr.max_i_qp                = EthCamLinkRecInfo[tb.idx].cbr_info.uiMaxIQp;
		ethcam_venc_rc_param[tb.idx].cbr.min_i_qp                = EthCamLinkRecInfo[tb.idx].cbr_info.uiMinIQp;
		ethcam_venc_rc_param[tb.idx].cbr.init_p_qp               = EthCamLinkRecInfo[tb.idx].cbr_info.uiInitPQp;
		ethcam_venc_rc_param[tb.idx].cbr.max_p_qp                = EthCamLinkRecInfo[tb.idx].cbr_info.uiMaxPQp;
		ethcam_venc_rc_param[tb.idx].cbr.min_p_qp                = EthCamLinkRecInfo[tb.idx].cbr_info.uiMinPQp;
		ethcam_venc_rc_param[tb.idx].cbr.static_time             = EthCamLinkRecInfo[tb.idx].cbr_info.uiStaticTime;
		ethcam_venc_rc_param[tb.idx].cbr.ip_weight               = EthCamLinkRecInfo[tb.idx].cbr_info.iIPWeight;
		ethcam_venc_rc_param[tb.idx].cbr.key_p_period            = 0;
		ethcam_venc_rc_param[tb.idx].cbr.kp_weight               = 0;
		ethcam_venc_rc_param[tb.idx].cbr.p2_weight               = 0;
		ethcam_venc_rc_param[tb.idx].cbr.p3_weight               = 0;
		ethcam_venc_rc_param[tb.idx].cbr.lt_weight               = 0;
		ethcam_venc_rc_param[tb.idx].cbr.motion_aq_str           = 0;
		ethcam_venc_rc_param[tb.idx].cbr.max_frame_size          = ethcam_venc_max_frame_size[tb.idx];

		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(id, MOVIE_USER_CB_EVENT_SET_ENC_RC_INFO, (UINT32)&(ethcam_venc_rc_param[tb.idx]));
		}

		for (j = 0; j < 2; j ++) {
			ethcam_bsmux_vinfo[tb.idx][j].vidcodec                        = EthCamLinkRecInfo[tb.idx].codec;
			ethcam_bsmux_vinfo[tb.idx][j].vfr                             = EthCamLinkRecInfo[tb.idx].vfr;
			ethcam_bsmux_vinfo[tb.idx][j].width                           = EthCamLinkRecInfo[tb.idx].width;
			ethcam_bsmux_vinfo[tb.idx][j].height                          = EthCamLinkRecInfo[tb.idx].height;
			ethcam_bsmux_vinfo[tb.idx][j].tbr                             = EthCamLinkRecInfo[tb.idx].tbr;   // project setting is byte rate
			ethcam_bsmux_vinfo[tb.idx][j].DAR                             = EthCamLinkRecInfo[tb.idx].ar;
			ethcam_bsmux_vinfo[tb.idx][j].gop                             = EthCamLinkRecInfo[tb.idx].gop ? EthCamLinkRecInfo[tb.idx].gop : 15;
			if (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_AAC) {
				ethcam_bsmux_ainfo[tb.idx][j].codectype                       = MOVAUDENC_AAC;
			} else if (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ULAW) {
				ethcam_bsmux_ainfo[tb.idx][j].codectype                       = MOVAUDENC_ULAW;
			} else if (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ALAW) {
				ethcam_bsmux_ainfo[tb.idx][j].codectype                       = MOVAUDENC_ALAW;
			} else {
				ethcam_bsmux_ainfo[tb.idx][j].codectype                       = MOVAUDENC_PPCM;
			}
			ethcam_bsmux_ainfo[tb.idx][j].chs                             = g_AudioInfo.aud_ch_num;
			ethcam_bsmux_ainfo[tb.idx][j].asr                             = g_AudioInfo.aud_rate;
			ethcam_bsmux_ainfo[tb.idx][j].aud_en                          = (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_AV) ? TRUE : FALSE;
			ethcam_bsmux_ainfo[tb.idx][j].adts_bytes                      = (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_AAC) ? 7 : 0;
		}
	} else {
		DBG_ERR("id%d is out of range.\r\n", i);
		ret = E_SYS;
	}
	return ret;
}

ER _ImageApp_MovieMulti_SetupFileOption(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	UINT32 i = id, j;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		for (j = 0; j < 2; j ++) {
			if (j == 0) {       // normal path
				img_bsmux_finfo[tb.idx][2*tb.tbl+j].emron          = FALSE;
			} else {            // emr path
				if (tb.tbl == 0) {
					img_bsmux_finfo[tb.idx][2*tb.tbl+j].emron          = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? TRUE : FALSE;
					img_bsmux_finfo[tb.idx][2*tb.tbl+j].emrloop        = (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) ? TRUE : FALSE;
				} else {
					img_bsmux_finfo[tb.idx][2*tb.tbl+j].emron          = (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) ? TRUE : FALSE;
					img_bsmux_finfo[tb.idx][2*tb.tbl+j].emrloop        = (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) ? TRUE : FALSE;
				}
			}
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].rollbacksec    = (tb.tbl == MOVIETYPE_MAIN) ? ImgLinkFileInfo.emr_sec : ImgLinkFileInfo.clone_emr_sec;
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].keepsec        = (tb.tbl == MOVIETYPE_MAIN) ? ImgLinkFileInfo.keep_sec : ImgLinkFileInfo.clone_keep_sec;
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].strgid         = 0;
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].seamlessSec    = ImgLinkFileInfo.seamless_sec;
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].endtype        = ImgLinkFileInfo.end_type;
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].filetype       = ImgLinkRecInfo[tb.idx][tb.tbl].file_format;
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].recformat      = ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode;
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].playvfr        = ImgLinkTimelapsePlayRate;
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].revsec         = img_bsmux_bufsec[tb.idx][2*tb.tbl+j];
			img_bsmux_finfo[tb.idx][2*tb.tbl+j].overlop_on     = (ImgLinkFileInfo.overlap_sec) ? TRUE : FALSE;
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		for (j = 0; j < 2; j ++) {
			if (j == 0) {       // normal path
				ethcam_bsmux_finfo[tb.idx][j].emron          = FALSE;
				ethcam_bsmux_finfo[tb.idx][j].emrloop        = FALSE;
			} else {            // emr path
				ethcam_bsmux_finfo[tb.idx][j].emron          = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? TRUE : FALSE;
				ethcam_bsmux_finfo[tb.idx][j].emrloop        = (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) ? TRUE : FALSE;
			}
			ethcam_bsmux_finfo[tb.idx][j].rollbacksec        = ImgLinkFileInfo.emr_sec;
			ethcam_bsmux_finfo[tb.idx][j].keepsec            = ImgLinkFileInfo.keep_sec;
			ethcam_bsmux_finfo[tb.idx][j].strgid             = 0;
			ethcam_bsmux_finfo[tb.idx][j].seamlessSec        = ImgLinkFileInfo.seamless_sec;
			ethcam_bsmux_finfo[tb.idx][j].endtype            = ImgLinkFileInfo.end_type;
			ethcam_bsmux_finfo[tb.idx][j].filetype           = EthCamLinkRecInfo[tb.idx].rec_format;
			ethcam_bsmux_finfo[tb.idx][j].recformat          = EthCamLinkRecInfo[tb.idx].rec_mode;
			ethcam_bsmux_finfo[tb.idx][j].playvfr            = ImgLinkTimelapsePlayRate;
			ethcam_bsmux_finfo[tb.idx][j].revsec             = ethcam_bsmux_bufsec[tb.idx][j];
			ethcam_bsmux_finfo[tb.idx][j].overlop_on         = (ImgLinkFileInfo.overlap_sec) ? TRUE : FALSE;
		}
	} else {
		DBG_ERR("id%d is out of range.\r\n", i);
		ret = E_SYS;
	}
	return ret;
}

ER _ImageApp_MovieMulti_SetupStrmParam(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
		if (wifi_venc_maxmem[tb.idx].bitrate && wifi_venc_maxmem[tb.idx].enc_buf_ms) {
			memcpy((void *)&(wifi_venc_path_cfg[tb.idx].max_mem), (void *)&(wifi_venc_maxmem[tb.idx]), sizeof(HD_VIDEOENC_MAXMEM));
		} else {
			if (WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_MJPG) {
				wifi_venc_path_cfg[tb.idx].max_mem.codec_type      = HD_CODEC_TYPE_JPEG;
			} else if (WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_H264) {
				wifi_venc_path_cfg[tb.idx].max_mem.codec_type      = HD_CODEC_TYPE_H264;
			} else {
				wifi_venc_path_cfg[tb.idx].max_mem.codec_type      = HD_CODEC_TYPE_H265;
			}
			wifi_venc_path_cfg[tb.idx].max_mem.max_dim.w       = WifiLinkStrmInfo[tb.idx].size.w;
			wifi_venc_path_cfg[tb.idx].max_mem.max_dim.h       = WifiLinkStrmInfo[tb.idx].size.h;
			wifi_venc_path_cfg[tb.idx].max_mem.bitrate         = WifiLinkStrmInfo[tb.idx].max_bit_rate * 8;               // project setting is byte rate
			wifi_venc_path_cfg[tb.idx].max_mem.enc_buf_ms      = WifiLinkVdoEncBufMs[tb.idx];
			wifi_venc_path_cfg[tb.idx].max_mem.svc_layer       = wifi_venc_svc[tb.idx];
			wifi_venc_path_cfg[tb.idx].max_mem.ltr             = FALSE;
			wifi_venc_path_cfg[tb.idx].max_mem.rotate          = FALSE;
			wifi_venc_path_cfg[tb.idx].max_mem.source_output   = FALSE;
		}

		wifi_venc_path_cfg[tb.idx].isp_id                  = 0;

		wifi_venc_in_param[tb.idx].dim.w                   = WifiLinkStrmInfo[tb.idx].size.w;
		wifi_venc_in_param[tb.idx].dim.h                   = WifiLinkStrmInfo[tb.idx].size.h;
		wifi_venc_in_param[tb.idx].pxl_fmt                 = HD_VIDEO_PXLFMT_YUV420;
		wifi_venc_in_param[tb.idx].dir                     = HD_VIDEO_DIR_NONE;
		if (wifi_venc_trigger_mode[tb.idx] == MOVIE_CODEC_TRIGGER_TIMER) {
			wifi_venc_in_param[tb.idx].frc                     = _ImageApp_MovieMulti_frc(WifiLinkStrmInfo[tb.idx].frame_rate, 1);
		} else {
			wifi_venc_in_param[tb.idx].frc                     = _ImageApp_MovieMulti_frc(1, 1);
		}

		if (WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_MJPG) {
			wifi_venc_out_param[tb.idx].codec_type             = HD_CODEC_TYPE_JPEG;
			wifi_venc_out_param[tb.idx].jpeg.retstart_interval = 0;
			wifi_venc_out_param[tb.idx].jpeg.image_quality     = 50;
			wifi_venc_out_param[tb.idx].jpeg.bitrate           = 0;
			wifi_venc_out_param[tb.idx].jpeg.frame_rate_base   = 1;
			wifi_venc_out_param[tb.idx].jpeg.frame_rate_incr   = 1;
		} else if (WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_H264) {
			wifi_venc_out_param[tb.idx].codec_type             = HD_CODEC_TYPE_H264;
			wifi_venc_out_param[tb.idx].h26x.gop_num           = WifiLinkStrmInfo[tb.idx].cbr_info.uiGOP ? WifiLinkStrmInfo[tb.idx].cbr_info.uiGOP : 30;
			wifi_venc_out_param[tb.idx].h26x.source_output     = DISABLE;
			wifi_venc_out_param[tb.idx].h26x.profile           = wifi_venc_h264_profile[tb.idx] ? wifi_venc_h264_profile[tb.idx] : IAMVOIE_DEFAULT_H264_PROFILE;
			wifi_venc_out_param[tb.idx].h26x.level_idc         = wifi_venc_h264_level[tb.idx] ? wifi_venc_h264_level[tb.idx] : IAMVOIE_DEFAULT_H264_LEVEL;
			wifi_venc_out_param[tb.idx].h26x.svc_layer         = wifi_venc_svc[tb.idx];
			wifi_venc_out_param[tb.idx].h26x.entropy_mode      = (wifi_venc_h264_profile[tb.idx] != _CFG_H264_PROFILE_BASELINE) ? HD_H264E_CABAC_CODING : HD_H264E_CAVLC_CODING;
		} else {
			wifi_venc_out_param[tb.idx].codec_type             = HD_CODEC_TYPE_H265;
			wifi_venc_out_param[tb.idx].h26x.gop_num           = WifiLinkStrmInfo[tb.idx].cbr_info.uiGOP ? WifiLinkStrmInfo[tb.idx].cbr_info.uiGOP : 30;
			wifi_venc_out_param[tb.idx].h26x.source_output     = DISABLE;
			wifi_venc_out_param[tb.idx].h26x.profile           = wifi_venc_h265_profile[tb.idx] ? wifi_venc_h265_profile[tb.idx] : IAMVOIE_DEFAULT_H265_PROFILE;
			wifi_venc_out_param[tb.idx].h26x.level_idc         = wifi_venc_h265_level[tb.idx] ? wifi_venc_h265_level[tb.idx] : IAMVOIE_DEFAULT_H265_LEVEL;
			wifi_venc_out_param[tb.idx].h26x.svc_layer         = wifi_venc_svc[tb.idx];
			wifi_venc_out_param[tb.idx].h26x.entropy_mode      = HD_H265E_CABAC_CODING;
		}

		wifi_venc_rc_param[tb.idx].rc_mode                     = HD_RC_MODE_CBR;
		wifi_venc_rc_param[tb.idx].cbr.bitrate                 = WifiLinkStrmInfo[tb.idx].max_bit_rate * 8;       // project setting is byte rate
		wifi_venc_rc_param[tb.idx].cbr.frame_rate_base         = WifiLinkStrmInfo[tb.idx].frame_rate;
		wifi_venc_rc_param[tb.idx].cbr.frame_rate_incr         = 1;
		wifi_venc_rc_param[tb.idx].cbr.init_i_qp               = WifiLinkStrmInfo[tb.idx].cbr_info.uiInitIQp;
		wifi_venc_rc_param[tb.idx].cbr.max_i_qp                = WifiLinkStrmInfo[tb.idx].cbr_info.uiMaxIQp;
		wifi_venc_rc_param[tb.idx].cbr.min_i_qp                = WifiLinkStrmInfo[tb.idx].cbr_info.uiMinIQp;
		wifi_venc_rc_param[tb.idx].cbr.init_p_qp               = WifiLinkStrmInfo[tb.idx].cbr_info.uiInitPQp;
		wifi_venc_rc_param[tb.idx].cbr.max_p_qp                = WifiLinkStrmInfo[tb.idx].cbr_info.uiMaxPQp;
		wifi_venc_rc_param[tb.idx].cbr.min_p_qp                = WifiLinkStrmInfo[tb.idx].cbr_info.uiMinPQp;
		wifi_venc_rc_param[tb.idx].cbr.static_time             = WifiLinkStrmInfo[tb.idx].cbr_info.uiStaticTime;
		wifi_venc_rc_param[tb.idx].cbr.ip_weight               = WifiLinkStrmInfo[tb.idx].cbr_info.iIPWeight;
		wifi_venc_rc_param[tb.idx].cbr.key_p_period            = 0;
		wifi_venc_rc_param[tb.idx].cbr.kp_weight               = 0;
		wifi_venc_rc_param[tb.idx].cbr.p2_weight               = 0;
		wifi_venc_rc_param[tb.idx].cbr.p3_weight               = 0;
		wifi_venc_rc_param[tb.idx].cbr.lt_weight               = 0;
		wifi_venc_rc_param[tb.idx].cbr.motion_aq_str           = 0;
		wifi_venc_rc_param[tb.idx].cbr.max_frame_size          = wifi_venc_max_frame_size[tb.idx];

		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(id, MOVIE_USER_CB_EVENT_SET_ENC_RC_INFO, (UINT32)&(wifi_venc_rc_param[tb.idx]));
		}

#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
		_iacommp_set_streaming_mode((WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_MJPG) ? IACOMMON_STREAMING_MODE_LVIEWNVT : IACOMMON_STREAMING_MODE_LIVE555);
#endif
	} else {
		DBG_ERR("id%d is out of range.\r\n", i);
		ret = E_SYS;
	}
	return ret;
}

ER _ImageApp_MovieMulti_SetupImgCapParam(MOVIE_CFG_REC_ID id)
{
	UINT32 idx = 0, tbl = 0;

	imgcap_venc_path_cfg[idx].max_mem.codec_type        = HD_CODEC_TYPE_JPEG;
	imgcap_venc_path_cfg[idx].max_mem.max_dim.w         = ImgLinkRecInfo[idx][tbl].raw_enc_size.w;
	imgcap_venc_path_cfg[idx].max_mem.max_dim.h         = ImgLinkRecInfo[idx][tbl].raw_enc_size.h;
	if (imgcap_jpg_quality.buf_size) {
		imgcap_venc_path_cfg[idx].max_mem.bitrate           = imgcap_jpg_quality.buf_size * 8;
	} else {
		imgcap_venc_path_cfg[idx].max_mem.bitrate           = VDO_YUV_BUFSIZE(ImgLinkRecInfo[idx][tbl].raw_enc_size.w, ImgLinkRecInfo[idx][tbl].raw_enc_size.h, HD_VIDEO_PXLFMT_YUV420) * 8 / 5;
	}
	imgcap_venc_path_cfg[idx].max_mem.enc_buf_ms        = 1500;
	if (ImgCapExifFuncEn) {
		imgcap_venc_path_cfg[idx].max_mem.enc_buf_ms += ((65536 * 2 * 8 * 1000) / imgcap_venc_path_cfg[idx].max_mem.bitrate);
	}
	imgcap_venc_path_cfg[idx].max_mem.svc_layer         = HD_SVC_DISABLE;
	imgcap_venc_path_cfg[idx].max_mem.ltr               = FALSE;
	imgcap_venc_path_cfg[idx].max_mem.rotate            = FALSE;
	imgcap_venc_path_cfg[idx].max_mem.source_output     = FALSE;
	imgcap_venc_path_cfg[idx].isp_id                    = 0;

	imgcap_venc_in_param[idx].dim.w                     = ImgLinkRecInfo[idx][tbl].raw_enc_size.w;
	imgcap_venc_in_param[idx].dim.h                     = ImgLinkRecInfo[idx][tbl].raw_enc_size.h;
	imgcap_venc_in_param[idx].pxl_fmt                   = HD_VIDEO_PXLFMT_YUV420;
	imgcap_venc_in_param[idx].dir                       = HD_VIDEO_DIR_NONE;
	imgcap_venc_in_param[idx].frc                       = _ImageApp_MovieMulti_frc(1, 1);

	imgcap_venc_out_param[idx].codec_type               = HD_CODEC_TYPE_JPEG;
	imgcap_venc_out_param[idx].jpeg.retstart_interval   = 0;
	imgcap_venc_out_param[idx].jpeg.image_quality       = imgcap_jpg_quality.jpg_quality;

	return E_OK;
}

/// ========== Set parameter area ==========
static ER _ImageApp_MovieMulti_SetParam_IPLGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id, j;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_IPL_MIRROR: {
			if (value & ~HD_VIDEO_DIR_MIRRORXY) {
				DBG_ERR("Invalid setting(%x) of MOVIEMULTI_PARAM_IPL_MIRROR\r\n", value);
				break;
			}
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				img_vproc_in_param[tb.idx].dir = value;
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// vprc
					IACOMM_VPROC_PARAM_IN vproc_param_in = {
						.video_proc_path = ImgLink[tb.idx].vproc_i[0][0],
						.in_param        = &(img_vproc_in_param[tb.idx]),
					};
					if ((hd_ret = vproc_set_param_in(&vproc_param_in)) != HD_OK) {
						DBG_ERR("vproc_set_param_in fail(%d)\n", hd_ret);
					}
					// resetrt one running path to active setting
					for(j = 0; j < 4; j++) {
						if (ImgLinkStatus[tb.idx].vproc_p_phy[0][j]) {
							if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p_phy[0][j])) != HD_OK) {
								DBG_ERR("hd_videoproc_start fail(%d)\n", hd_ret);
							}
							break;
						}
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VCAP_OUTFUNC: {
			if (value > HD_VIDEOCAP_OUTFUNC_ONEBUF) {
				DBG_ERR("Invalid setting(%d) of MOVIEMULTI_PARAM_D2D_MODE\r\n", value);
				break;
			}
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				img_vcap_func_cfg[tb.idx].out_func = value;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_IPL_FORCED_IMG_SIZE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				gImgLinkForcedSizePath[tb.idx] = value;
				ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_IPL_FORCED_IMG_SIZE=%d to idx[%d]:\r\n", gImgLinkForcedSizePath[tb.idx], tb.idx);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_IPL_USER_IMG_SIZE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (value) {
					MOVIEMULTI_IPL_SIZE_INFO *pipl_size = (MOVIEMULTI_IPL_SIZE_INFO *) value;
					// var
					memcpy(&(IPLUserSize[tb.idx]), pipl_size, sizeof(MOVIEMULTI_IPL_SIZE_INFO));
					ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_IPL_FORCED_IMG_SIZE=(%d,%d)p%d to idx[%d]:\r\n", IPLUserSize[tb.idx].size.w, IPLUserSize[tb.idx].size.h, IPLUserSize[tb.idx].fps, tb.idx);
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VCAP_CCIR_FMT: {
			if (value > VENDOR_VIDEOCAP_CCIR_FMT_SEL_MAX_NUM) {
				DBG_ERR("Invalid setting(%d) of MOVIEMULTI_PARAM_VCAP_CCIR_FMT\r\n", value);
				break;
			}
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				img_vcap_ccir_fmt[tb.idx] = value;
				ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_VCAP_CCIR_FMT=%d to idx[%d]:\r\n", img_vcap_ccir_fmt[tb.idx], tb.idx);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VCAP_CCIR_MIRROR_FLIP: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				img_vcap_capout_dir[tb.idx] = value;
				ret = E_OK;
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					img_vcap_out_param[tb.idx].dir = img_vcap_capout_dir[tb.idx];
					IACOMM_VCAP_PARAM vcap_param = {
						.video_cap_path = ImgLink[tb.idx].vcap_p[0],
						.data_lane      = 0,
						.pin_param      = NULL,
						.pcrop_param    = NULL,
						.pout_param     = &(img_vcap_out_param[tb.idx]),
						.pfunc_cfg      = NULL,
					};
					if ((hd_ret = vcap_set_param(&vcap_param)) != HD_OK) {
						DBG_ERR("vcap_set_param fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
					if (ImgLinkStatus[tb.idx].vcap_p[0]) {
						if ((hd_ret = hd_videocap_start(ImgLink[tb.idx].vcap_p[0])) != HD_OK) {
							DBG_ERR("hd_videocap_start fail(%d)\r\n", hd_ret);
						}
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VCAP_CCIR_MODE: {
			if (value > CCIR_PROGRESSIVE) {
				DBG_ERR("Invalid setting(%d) of MOVIEMULTI_PARAM_VCAP_CCIR_MODE\r\n", value);
				break;
			}
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				img_vcap_ccir_mode[tb.idx] = value;
				ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_VCAP_CCIR_MODE=%d to idx[%d]:\r\n", img_vcap_ccir_mode[tb.idx], tb.idx);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VCAP_CROP: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VCAP_CROP should set before open\r\n");
			} else {
				if (value) {
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						MOVIEMULTI_VCAP_CROP_INFO *pInfo = (MOVIEMULTI_VCAP_CROP_INFO *) value;
						memcpy(&(img_vcap_crop_win[tb.idx]), pInfo, sizeof(MOVIEMULTI_VCAP_CROP_INFO));
						ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_VCAP_CROP=(%d,%d,%d,%d) to idx[%d]:\r\n", img_vcap_crop_win[tb.idx].Win.x, img_vcap_crop_win[tb.idx].Win.y, img_vcap_crop_win[tb.idx].Win.w, img_vcap_crop_win[tb.idx].Win.h, tb.idx);
						ret = E_OK;
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_FORCED_UNIQUE_3DNR_PATH: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_FORCED_UNIQUE_3DNR_PATH should set before open\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					forced_use_unique_3dnr_path[tb.idx] = value ? TRUE : FALSE;
					ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_FORCED_UNIQUE_3DNR_PATH=%d to idx[%d]:\r\n", forced_use_unique_3dnr_path[tb.idx], tb.idx);
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_FORCED_USE_YUVAUX: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_FORCED_USE_YUVAUX should set before open\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					UINT32 prev = img_vproc_forced_use_pipe[tb.idx];
					img_vproc_forced_use_pipe[tb.idx] = value ? HD_VIDEOPROC_PIPE_YUVAUX : 0;
					ImageApp_MovieMulti_DUMP("id = %d: Set MOVIEMULTI_PARAM_FORCED_USE_YUVAUX from 0x%08x to 0x%08x\r\n", i, prev, img_vproc_forced_use_pipe[tb.idx]);
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VCAP_OUT_SIZE: {
#if defined(_BSP_NA51055_)
			DBG_ERR("MOVIEMULTI_PARAM_VCAP_OUT_SIZE is not supported in this chip.\r\n");
			ret = E_NOSPT;
#else
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VCAP_OUT_SIZE should set before open\r\n");
			} else {
				if (value) {
					USIZE *psz = (USIZE *)value;
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						img_vcap_out_size[tb.idx].w = psz->w;
						img_vcap_out_size[tb.idx].h = psz->h;
						ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_VCAP_OUT_SIZE=(%d, %d) to idx[%d]:\r\n", img_vcap_out_size[tb.idx].w, img_vcap_out_size[tb.idx].h, tb.idx);
						ret = E_OK;
					}
				}
			}
#endif
			break;
		}

	case MOVIEMULTI_PARAM_VCAP_CCIR_DET_LOOP: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VCAP_CCIR_DET_LOOP should set before open\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					// var
					img_vcap_detect_loop[tb.idx] = value;
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VCAP_PAT_GEN: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VCAP_PAT_GEN should set before open\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					if (value > HD_VIDEOCAP_SEN_PAT_HVINCREASE) {
						DBG_ERR("Set MOVIEMULTI_PARAM_VCAP_PAT_GEN out of range(%d)\r\n", value);
						ret = E_PAR;
					} else {
						// var
						img_vcap_patgen[tb.idx] = value;
						ret = E_OK;
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_FPS: {
			if (value == 0) {
				DBG_ERR("MOVIEMULTI_PARAM_VPRC_FPS is set with NULL value\r\n");
				ret = E_PAR;
			} else {
				if (IsImgLinkOpened[tb.idx] == FALSE) {
					DBG_ERR("MOVIEMULTI_PARAM_VPRC_FPS can only be set after open\r\n");
					ret = E_SYS;
					break;
				}
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					HD_VIDEOPROC_OUT_EX video_out_param_ex = {0};
					MOVIEMULTI_VPRC_FPS *pvprc_fps = (MOVIEMULTI_VPRC_FPS *)value;
					if (gSwitchLink[tb.idx][pvprc_fps->id] == UNUSED) {
						DBG_ERR("MOVIEMULTI_PARAM_VPRC_FPS cannot be set since gSwitchLink[%d][%d]=%d\r\n", tb.idx, pvprc_fps->id, gSwitchLink[tb.idx][pvprc_fps->id]);
						ret = E_SYS;
						break;
					}
					UINT32 k = gUserProcMap[tb.idx][gSwitchLink[tb.idx][pvprc_fps->id]];
					if (k == 3) {
						k = 2;
					}
					if (pvprc_fps->id != IAMOVIE_VPRC_EX_WIFI) {
						DBG_ERR("MOVIEMULTI_PARAM_VPRC_FPS is not supported in id[%d]\r\n", pvprc_fps->id);
						ret = E_NOSPT;
						break;
					}
					if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_fps->id])) {
						DBG_ERR("MOVIEMULTI_PARAM_VPRC_FPS cannot be set since sub_id[%d] is running\r\n", pvprc_fps->id);
						ret = E_SYS;
						break;
					}
					if ((hd_ret = hd_videoproc_get(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_fps->id], HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param_ex)) != HD_OK) {
						DBG_ERR("hd_videoproc_get(HD_VIDEOPROC_PARAM_OUT_EX) fail[%d]\r\n", hd_ret);
						ret = E_SYS;
						break;
					}
					if (pvprc_fps->fps > fps_vprc_p_out[tb.idx][k]) {
						DBG_WRN("MOVIEMULTI_PARAM_VPRC_FPS target fps %d is larger than original %d, force set to %d\r\n", pvprc_fps->fps, fps_vprc_p_out[tb.idx][k], fps_vprc_p_out[tb.idx][k]);
						pvprc_fps->fps = fps_vprc_p_out[tb.idx][k];
					}
					video_out_param_ex.frc = _ImageApp_MovieMulti_frc(pvprc_fps->fps, fps_vprc_p_out[tb.idx][k]);
					if ((hd_ret = hd_videoproc_set(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_fps->id], HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param_ex)) != HD_OK) {
						DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT_EX) fail[%d]\r\n", hd_ret);
						ret = E_SYS;
						break;
					}
					DBG_DUMP("MOVIEMULTI_PARAM_VPRC_FPS set id[%d] to fps %d\r\n", pvprc_fps->id, pvprc_fps->fps);
					ret = E_OK;
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_VPRC_FPS is not support in linktype[%d]\r\n", tb.link_type);
					ret = E_NOSPT;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_DIS_FUNC: {
			if (value == 0) {
				DBG_ERR("MOVIEMULTI_PARAM_DIS_FUNC is set with NULL value\r\n");
				ret = E_PAR;
			} else {
				if (IsImgLinkOpened[tb.idx]) {
					DBG_ERR("MOVIEMULTI_PARAM_DIS_FUNC should set before open\r\n");
					ret = E_SYS;
				} else {
					MOVIEMULTI_DIS_PARAM *pvprc_dis = (MOVIEMULTI_DIS_PARAM *)value;
					if ((pvprc_dis->scale_ratio != 1100) && (pvprc_dis->scale_ratio != 1200) && (pvprc_dis->scale_ratio != 1400)) {
						DBG_ERR("MOVIEMULTI_PARAM_DIS_FUNC: invalid sacle ratio(%d)\r\n", pvprc_dis->scale_ratio);
						ret = E_PAR;
						break;
					}
					if (pvprc_dis->subsample > 2) {
						DBG_ERR("MOVIEMULTI_PARAM_DIS_FUNC: invalid subsample(%d)\r\n", pvprc_dis->subsample);
						ret = E_PAR;
						break;
					}
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						img_vproc_dis_func[tb.idx] = (pvprc_dis->enable) ? HD_VIDEOPROC_PIPE_OUT_DIS : 0;
						if (img_vproc_dis_func[tb.idx]) {
							forced_use_unique_3dnr_path[tb.idx] = TRUE;
							vendor_videoproc_set(ImgLink[tb.idx].vproc_p_ctrl[img_vproc_splitter[tb.idx]], VENDOR_VIDEOPROC_CFG_DIS_SCALERATIO, &(pvprc_dis->scale_ratio));
							vendor_videoproc_set(ImgLink[tb.idx].vproc_p_ctrl[img_vproc_splitter[tb.idx]], VENDOR_VIDEOPROC_CFG_DIS_SUBSAMPLE, &(pvprc_dis->subsample));
						}
						ret = E_OK;
					} else {
						ret = E_NOSPT;
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_OUT_SIZE: {
			if (value == 0) {
				DBG_ERR("MOVIEMULTI_PARAM_VPRC_OUT_SIZE is set with NULL value\r\n");
				ret = E_PAR;
			} else {
				if (IsImgLinkOpened[tb.idx] == FALSE) {
					DBG_ERR("MOVIEMULTI_PARAM_VPRC_FPS can only be set after open\r\n");
					ret = E_SYS;
					break;
				}
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					HD_VIDEOPROC_CROP video_out_crop_param_ex = {0};
					HD_VIDEOPROC_OUT_EX video_out_param_ex = {0};
					MOVIEMULTI_VPRC_SIZE *pvprc_size = (MOVIEMULTI_VPRC_SIZE *)value;
					if (pvprc_size->id != IAMOVIE_VPRC_EX_ALG) {
						DBG_ERR("MOVIEMULTI_PARAM_VPRC_OUT_SIZE is not supported in id[%d]\r\n", pvprc_size->id);
						ret = E_NOSPT;
						break;
					}
					if (pvprc_size->crop_update) {
						if ((hd_ret = hd_videoproc_get(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_size->id], HD_VIDEOPROC_PARAM_OUT_EX_CROP, &video_out_crop_param_ex)) != HD_OK) {
							DBG_ERR("hd_videoproc_get(HD_VIDEOPROC_PARAM_OUT_EX_CROP) fail(%d)\r\n", hd_ret);
							ret = E_SYS;
							break;
						}
						DBG_DUMP("vprc(%d):Update crop window:(%d,%d,%d,%d)->(%d,%d,%d,%d)\r\n",
								(pvprc_size->id + 5),
								video_out_crop_param_ex.win.rect.x, video_out_crop_param_ex.win.rect.y, video_out_crop_param_ex.win.rect.w, video_out_crop_param_ex.win.rect.h,
								pvprc_size->crop_win.x, pvprc_size->crop_win.y, pvprc_size->crop_win.w, pvprc_size->crop_win.h);

						video_out_crop_param_ex.win.rect.x = pvprc_size->crop_win.x;
						video_out_crop_param_ex.win.rect.y = pvprc_size->crop_win.y;
						video_out_crop_param_ex.win.rect.w = pvprc_size->crop_win.w;
						video_out_crop_param_ex.win.rect.h = pvprc_size->crop_win.h;
						if ((hd_ret = hd_videoproc_set(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_size->id], HD_VIDEOPROC_PARAM_OUT_EX_CROP, &video_out_crop_param_ex)) != HD_OK) {
							DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT_EX_CROP) fail(%d)\r\n", hd_ret);
							ret = E_SYS;
							break;
						}
					}
					if (pvprc_size->out_update) {
						if ((hd_ret = hd_videoproc_get(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_size->id], HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param_ex)) != HD_OK) {
							DBG_ERR("hd_videoproc_get(HD_VIDEOPROC_PARAM_OUT_EX) fail[%d]\r\n", hd_ret);
							ret = E_SYS;
							break;
						}
						DBG_DUMP("vprc(%d):Update out size:(%d,%d)->(%d,%d)\r\n",
								(pvprc_size->id + 5),
								video_out_param_ex.dim.w, video_out_param_ex.dim.h,
								pvprc_size->out_size.w, pvprc_size->out_size.h);

						video_out_param_ex.dim.w = pvprc_size->out_size.w;
						video_out_param_ex.dim.h = pvprc_size->out_size.h;
						if ((hd_ret = hd_videoproc_set(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_size->id], HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param_ex)) != HD_OK) {
							DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT_EX) fail[%d]\r\n", hd_ret);
							ret = E_SYS;
							break;
						}
					}
					if (pvprc_size->crop_update || pvprc_size->out_update) {
						if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_size->id])) {
							if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_size->id])) != HD_OK) {
								DBG_ERR("hd_videoproc_start fail: id=%d, ret=%d\r\n", ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][pvprc_size->id], hd_ret);
								ret = E_SYS;
								break;
							}
						}
					}
					ret = E_OK;
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_VPRC_OUT_SIZE is not support in linktype[%d]\r\n", tb.link_type);
					ret = E_NOSPT;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_DIS_CROP_ALIGN: {
			//#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
			#if !defined(_BSP_NA51055_)
			if (value == 0) {
				DBG_ERR("MOVIEMULTI_PARAM_DIS_CROP_ALIGN is set with NULL value\r\n");
				ret = E_PAR;
			} else {
				if (img_vproc_dis_func[tb.idx] == 0) {
					DBG_ERR("Set MOVIEMULTI_PARAM_DIS_CROP_ALIGN but MOVIEMULTI_PARAM_DIS_FUNC is not set yet\r\n");
					ret = E_SYS;
					break;
				}
				if (IsImgLinkOpened[tb.idx] == FALSE) {
					DBG_ERR("MOVIEMULTI_PARAM_VPRC_FPS can only be set after open\r\n");
					ret = E_SYS;
					break;
				}
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					MOVIEMULTI_DIS_CROP_ALIGN_PARAM *pcrop_align = (MOVIEMULTI_DIS_CROP_ALIGN_PARAM *)value;
					UINT32 align = pcrop_align->align;
					if (pcrop_align->id != IAMOVIE_VPRC_EX_DISP) {
						DBG_ERR("MOVIEMULTI_PARAM_DIS_CROP_ALIGN is not supported in id[%d]\r\n", pcrop_align->id);
						ret = E_NOSPT;
						break;
					}
					if (gSwitchLink[tb.idx][pcrop_align->id] == UNUSED) {
						DBG_ERR("MOVIEMULTI_PARAM_DIS_CROP_ALIGN cannot be set since gSwitchLink[%d][%d]=%d\r\n", tb.idx, pcrop_align->id, gSwitchLink[tb.idx][pcrop_align->id]);
						ret = E_SYS;
						break;
					}
					j = gUserProcMap[tb.idx][gSwitchLink[tb.idx][pcrop_align->id]];
					j = (j == 3) ? 2 : j;
					if ((hd_ret = vendor_videoproc_set(ImgLink[tb.idx].vproc_p_phy[0][j], VENDOR_VIDEOPROC_PARAM_DIS_CROP_ALIGN, &align)) != HD_OK) {
						DBG_ERR("Set MOVIEMULTI_PARAM_DIS_CROP_ALIGN fail(%d)\r\n", hd_ret);
					} else {
						DBG_DUMP("idx[%d] ex_path_id[%d] set MOVIEMULTI_PARAM_DIS_CROP_ALIGN to %d\r\n", tb.idx, pcrop_align->id, pcrop_align->align);
						ret = E_OK;
					}
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_DIS_CROP_ALIGN is not supported in linktype[%d]\r\n", tb.link_type);
					ret = E_NOSPT;
				}
			}
			#else
			DBG_ERR("MOVIEMULTI_PARAM_DIS_CROP_ALIGN is not supported.\r\n");
			ret = E_NOSPT;
			#endif
			break;
		}

	case MOVIEMULTI_PARAM_SIE_REMAP: {
			#if defined(_BSP_NA51089_)
			if (IsImgLinkOpened[tb.idx] == TRUE) {
				DBG_ERR("MOVIEMULTI_PARAM_SIE_REMAP can only be set before open\r\n");
				ret = E_SYS;
				break;
			}
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				img_vcap_sie_remap[tb.idx] = value ? 1 : 0;
				DBG_DUMP("Set MOVIEMULTI_PARAM_SIE_REMAP id%d=%d\r\n", i, img_vcap_sie_remap[tb.idx]);
				ret = E_OK;
			} else {
				DBG_ERR("MOVIEMULTI_PARAM_SIE_REMAP is not supported in linktype[%d]\r\n", tb.link_type);
				ret = E_NOSPT;
			}
			#else
			DBG_ERR("MOVIEMULTI_PARAM_SIE_REMAP is not supported.\r\n");
			ret = E_NOSPT;
			#endif
			break;
		}

	case MOVIEMULTI_PARAM_VPE_EN: {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
#if defined(_BSP_NA51055_)
			char  *chip_name = getenv("NVT_CHIP_ID");
			if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
				// 580 or 529
			} else {
				DBG_ERR("MOVIEMULTI_PARAM_VPE_EN is not supported.\r\n");
				ret = E_NOSPT;
				break;
			}
#endif
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (IsImgLinkOpened[tb.idx]) {
					DBG_ERR("MOVIEMULTI_PARAM_VPE_EN should set before open\r\n");
					ret = E_SYS;
				} else {
					img_vproc_vpe_en[tb.idx] = value;
					img_vproc_splitter[tb.idx] = (img_vproc_vpe_en[tb.idx] & MOVIE_VPE_1_2) ? 1 : 0;
					ret = E_OK;
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_IMG_PATH)) {
				if (is_ethcamlink_opened[tb.idx]) {
					DBG_ERR("MOVIEMULTI_PARAM_VPE_EN should set before open\r\n");
					ret = E_SYS;
				} else {
					ethcam_vproc_vpe_en[tb.idx] = value;
					ret = E_OK;
				}
			}
#else
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_VPE_REG_CB: {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
#if defined(_BSP_NA51055_)
			char  *chip_name = getenv("NVT_CHIP_ID");
			if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
				// 580 or 529
			} else {
				DBG_ERR("MOVIEMULTI_PARAM_VPE_REG_CB is not supported.\r\n");
				ret = E_NOSPT;
				break;
			}
#endif
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (IsImgLinkOpened[tb.idx]) {
					DBG_ERR("MOVIEMULTI_PARAM_VPE_REG_CB should set before open\r\n");
					ret = E_SYS;
				} else {
					img_vproc_vpe_cb[tb.idx] = (MovieVpeCb *)value;
					ret = E_OK;
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_IMG_PATH)) {
				if (is_ethcamlink_opened[tb.idx]) {
					DBG_ERR("MOVIEMULTI_PARAM_VPE_REG_CB should set before open\r\n");
					ret = E_SYS;
				} else {
					ethcam_vproc_vpe_cb[tb.idx] = (MovieVpeCb *)value;
					ret = E_OK;
				}
			}
#else
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_VPE_SCENE: {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
#if defined(_BSP_NA51055_)
			char  *chip_name = getenv("NVT_CHIP_ID");
			if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
				// 580 or 529
			} else {
				DBG_ERR("MOVIEMULTI_PARAM_VPE_SCENE is not supported.\r\n");
				ret = E_NOSPT;
				break;
			}
#endif
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (IsImgLinkOpened[tb.idx]) {
					DBG_ERR("MOVIEMULTI_PARAM_VPE_SCENE should set before open\r\n");
					ret = E_SYS;
				} else {
					img_vproc_vpe_scene[tb.idx] = value;
					ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_VPE_SCENE]: sets %d to id%d\r\n", img_vproc_vpe_scene[tb.idx], tb.idx);
					ret = E_OK;
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_IMG_PATH)) {
				if (is_ethcamlink_opened[tb.idx]) {
					DBG_ERR("MOVIEMULTI_PARAM_VPE_SCENE should set before open\r\n");
					ret = E_SYS;
				} else {
					ethcam_vproc_vpe_scene[tb.idx] = value;
					ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_VPE_SCENE]: sets %d to id%d\r\n", ethcam_vproc_vpe_scene[tb.idx], tb.idx);
					ret = E_OK;
				}
			}
#else
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_FORCED_USE_VPE: {
#if defined(_BSP_NA51089_)
			DBG_ERR("MOVIEMULTI_PARAM_FORCED_USE_VPE is not supported.\r\n");
			ret = E_NOSPT;
			break;
#else
#if defined(_BSP_NA51055_)
			char  *chip_name = getenv("NVT_CHIP_ID");
			if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
				// 580 or 529
			} else {
				DBG_ERR("MOVIEMULTI_PARAM_FORCED_USE_VPE is not supported.\r\n");
				ret = E_NOSPT;
				break;
			}
#endif
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_FORCED_USE_VPE should set before open\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					UINT32 prev = img_vproc_forced_use_pipe[tb.idx];
					img_vproc_forced_use_pipe[tb.idx] = value ? HD_VIDEOPROC_PIPE_VPE : 0;
					ImageApp_MovieMulti_DUMP("id = %d: Set MOVIEMULTI_PARAM_FORCED_USE_VPE from 0x%08x to 0x%08x\r\n", i, prev, img_vproc_forced_use_pipe[tb.idx]);
					ret = E_OK;
				}
			}
			break;
#endif
		}

	case MOVIEMULTI_PARAM_VCAP_GYRO_FUNC_EN: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VCAP_GYRO_FUNC_EN should set before open\r\n");
				ret = E_SYS;
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_vcap_gyro_func[tb.idx] = value ? TRUE : FALSE;
					ImageApp_MovieMulti_DUMP("id=%d: Set MOVIEMULTI_PARAM_VCAP_GYRO_FUNC_EN to %d\r\n", i, img_vcap_gyro_func[tb.idx]);
					ret = E_OK;
				} else {
					ret = E_NOSPT;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_EIS_FUNC_EN: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VPRC_EIS_FUNC_EN should set before open\r\n");
				ret = E_SYS;
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_vproc_eis_func[tb.idx] = value ? TRUE : FALSE;
					ImageApp_MovieMulti_DUMP("id=%d: Set MOVIEMULTI_PARAM_VPRC_EIS_FUNC_EN to %d\r\n", i, img_vproc_eis_func[tb.idx]);
					ret = E_OK;
				} else {
					ret = E_NOSPT;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_QUEUE_DEPTH: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH)) {
				img_vproc_queue_depth[tb.idx] = value;
				ImageApp_MovieMulti_DUMP("id%d: set MOVIEMULTI_PARAM_VPRC_QUEUE_DEPTH = %d\r\n", i, img_vproc_queue_depth[tb.idx]);
				ret = E_OK;
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ethcam_vproc_queue_depth[tb.idx] = value;
				ImageApp_MovieMulti_DUMP("id%d: set MOVIEMULTI_PARAM_VPRC_QUEUE_DEPTH = %d\r\n", i, ethcam_vproc_queue_depth[tb.idx]);
				ret = E_OK;
#endif
			} else {
				DBG_ERR("id%d: set MOVIEMULTI_PARAM_VPRC_QUEUE_DEPTH not supported\r\n", i);
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_EIS_FUNC_USE_VPE_LITE: {
#if defined(_BSP_NA51102_)
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VPRC_EIS_FUNC_USE_VPE_LITE should set before open\r\n");
				ret = E_SYS;
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_vproc_eis_use_vpe_lite[tb.idx] = value ? TRUE : FALSE;
					ImageApp_MovieMulti_DUMP("id=%d: Set MOVIEMULTI_PARAM_VPRC_EIS_FUNC_USE_VPE_LITE to %d\r\n", i, img_vproc_eis_use_vpe_lite[tb.idx]);
					ret = E_OK;
				} else {
					ret = E_NOSPT;
				}
			}
#else
			DBG_ERR("MOVIEMULTI_PARAM_VPRC_EIS_FUNC_USE_VPE_LITE is not supported\r\n");
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_EIS_YUV_COMPRESS: {
#if defined(_BSP_NA51102_)
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VPRC_EIS_YUV_COMPRESS should set before open\r\n");
				ret = E_SYS;
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_vproc_yuv_compress_for_eis[tb.idx] = value ? TRUE : FALSE;
					ImageApp_MovieMulti_DUMP("id=%d: Set MOVIEMULTI_PARAM_VPRC_EIS_YUV_COMPRESS to %d\r\n", i, img_vproc_yuv_compress_for_eis[tb.idx]);
					ret = E_OK;
				} else {
					ret = E_NOSPT;
				}
			}
#else
			DBG_ERR("MOVIEMULTI_PARAM_VPRC_EIS_YUV_COMPRESS is not supported\r\n");
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_FORCED_USE_PIPE_SCALE: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_FORCED_USE_PIPE_SCALE should set before open\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					UINT32 prev = img_vproc_forced_use_pipe[tb.idx];
					img_vproc_forced_use_pipe[tb.idx] = value ? HD_VIDEOPROC_PIPE_SCALE : 0;
					ImageApp_MovieMulti_DUMP("id = %d: Set MOVIEMULTI_PARAM_FORCED_USE_PIPE_SCALE from 0x%08x to 0x%08x\r\n", i, prev, img_vproc_forced_use_pipe[tb.idx]);
					ret = E_OK;
				}
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_ImgGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	HD_RESULT hd_ret;
	ER ret = E_SYS;
	UINT32 i = id, j;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_MAIN_IME_CROP: {
			if (value) {
				MOVIEMULTI_IME_CROP_INFO *pInfo = (MOVIEMULTI_IME_CROP_INFO *) value;
				if (_LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[0])) {
					if ((pInfo->IMEWin.w != MainIMECropInfo[tb.idx].IMEWin.w) || (pInfo->IMEWin.h != MainIMECropInfo[tb.idx].IMEWin.h)) {
						DBG_ERR("[MOVIEMULTI_PARAM_MAIN_IME_CROP] Cannot change size during recording! Org(%d,%d), New(%d,%d)\r\n", pInfo->IMEWin.w, pInfo->IMEWin.h, MainIMECropInfo[tb.idx].IMEWin.w, MainIMECropInfo[tb.idx].IMEWin.h);
						break;
					}
				}
				if ((pInfo->IMEWin.x + pInfo->IMEWin.w > pInfo->IMESize.w) || (pInfo->IMEWin.y + pInfo->IMEWin.h > pInfo->IMESize.h)) {
					DBG_ERR("[MOVIEMULTI_PARAM_MAIN_IME_CROP] Window out of range! %d+%d>%d or %d+%d>%d\r\n", pInfo->IMEWin.x, pInfo->IMEWin.w, pInfo->IMESize.w, pInfo->IMEWin.y, pInfo->IMEWin.h, pInfo->IMESize.h);
				} else {
					// copy data structure
					ret = E_OK;
					memcpy(&(MainIMECropInfo[tb.idx]), pInfo, sizeof(MOVIEMULTI_IME_CROP_INFO));
					if (IsImgLinkOpened[tb.idx] == TRUE) {     // already opened, update window
						IACOMM_VPROC_PARAM vproc_param = {0};
						HD_DIM sz;
						if (gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_MAIN] == UNUSED) {
							DBG_ERR("MOVIEMULTI_PARAM_MAIN_IME_CROP cannot be set since gSwitchLink[%d][%d]=%d\r\n", tb.idx, IAMOVIE_VPRC_EX_MAIN, gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_MAIN]);
							ret = E_SYS;
							break;
						}
						j = gUserProcMap[tb.idx][gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_MAIN]];
						j = (j == 3) ? 2 : j;
						vproc_param.video_proc_path = ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j];
						vproc_param.p_dim           = &sz;
						vproc_param.p_dim->w        = MainIMECropInfo[tb.idx].IMESize.w;
						vproc_param.p_dim->h        = MainIMECropInfo[tb.idx].IMESize.h;
						vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
						vproc_param.frc             = _ImageApp_MovieMulti_frc(fps_vprc_p_out[tb.idx][j], fps_vcap_out[tb.idx]);
						vproc_param.pfunc           = NULL;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = MainIMECropInfo[tb.idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = MainIMECropInfo[tb.idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = MainIMECropInfo[tb.idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = MainIMECropInfo[tb.idx].IMEWin.h;
						if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
							DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
						if (img_vproc_no_ext[tb.idx]) {
							_ImageApp_MovieMulti_UpdateVprcDepth(vproc_param.video_proc_path, 1);
						}
						if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j])) {
							if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j])) != HD_OK) {
								DBG_ERR("hd_videoproc_start fail: id=%d, ret=%d\r\n", ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j], hd_ret);
								ret = E_SYS;
							}
						}

						if (img_vproc_no_ext[tb.idx] == FALSE) {
							IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};
							vproc_param_ex.video_proc_path_ex  = ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_MAIN];
							vproc_param_ex.video_proc_path_src = ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j];
							vproc_param_ex.p_dim               = &sz;
							vproc_param_ex.p_dim->w            = MainIMECropInfo[tb.idx].IMEWin.w;
							vproc_param_ex.p_dim->h            = MainIMECropInfo[tb.idx].IMEWin.h;
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420;
							vproc_param_ex.dir                 = HD_VIDEO_DIR_NONE;
							vproc_param_ex.frc                 = _ImageApp_MovieMulti_frc(fps_vprc_e_out[tb.idx][IAMOVIE_VPRC_EX_MAIN], fps_vprc_p_out[tb.idx][gSwitchLink[tb.idx][j]]);
							vproc_param_ex.depth               = 1;                       // for rawenc case, the depth should keep 1

							memset((void *)&(vproc_param_ex.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
							if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
								DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
								ret = E_SYS;
							}
							if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_MAIN])) {
								if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_MAIN])) != HD_OK) {
									DBG_ERR("hd_videoproc_start fail: id=%d, ret=%d\r\n", ImgLink[tb.idx].vproc_p[IAMOVIE_VPRC_EX_MAIN], hd_ret);
									ret = E_SYS;
								}
							}
						}
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_CLONE_IME_CROP: {
			if (value) {
				MOVIEMULTI_IME_CROP_INFO *pInfo = (MOVIEMULTI_IME_CROP_INFO *) value;
				if (_LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[1])) {
					if ((pInfo->IMEWin.w != CloneIMECropInfo[tb.idx].IMEWin.w) || (pInfo->IMEWin.h != CloneIMECropInfo[tb.idx].IMEWin.h)) {
						DBG_ERR("[MOVIEMULTI_PARAM_CLONE_IME_CROP] Cannot change size during recording! Org(%d,%d), New(%d,%d)\r\n", pInfo->IMEWin.w, pInfo->IMEWin.h, CloneIMECropInfo[tb.idx].IMEWin.w, CloneIMECropInfo[tb.idx].IMEWin.h);
						break;
					}
				}
				if ((pInfo->IMEWin.x + pInfo->IMEWin.w > pInfo->IMESize.w) || (pInfo->IMEWin.y + pInfo->IMEWin.h > pInfo->IMESize.h)) {
					DBG_ERR("[MOVIEMULTI_PARAM_CLONE_IME_CROP] Window out of range! %d+%d>%d or %d+%d>%d\r\n", pInfo->IMEWin.x, pInfo->IMEWin.w, pInfo->IMESize.w, pInfo->IMEWin.y, pInfo->IMEWin.h, pInfo->IMESize.h);
				} else {
					// copy data structure
					ret = E_OK;
					memcpy(&(CloneIMECropInfo[tb.idx]), pInfo, sizeof(MOVIEMULTI_IME_CROP_INFO));
					if (IsImgLinkOpened[tb.idx] == TRUE) {     // already opened, update window
						IACOMM_VPROC_PARAM vproc_param = {0};
						HD_DIM sz;
						if (gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_CLONE] == UNUSED) {
							DBG_ERR("MOVIEMULTI_PARAM_CLONE_IME_CROP cannot be set since gSwitchLink[%d][%d]=%d\r\n", tb.idx, IAMOVIE_VPRC_EX_CLONE, gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_CLONE]);
							ret = E_SYS;
							break;
						}
						j = gUserProcMap[tb.idx][gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_CLONE]];
						j = (j == 3) ? 2 : j;
						vproc_param.video_proc_path = ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j];
						vproc_param.p_dim           = &sz;
						vproc_param.p_dim->w        = CloneIMECropInfo[tb.idx].IMESize.w;
						vproc_param.p_dim->h        = CloneIMECropInfo[tb.idx].IMESize.h;
						vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
						vproc_param.frc             = _ImageApp_MovieMulti_frc(fps_vprc_p_out[tb.idx][j], fps_vcap_out[tb.idx]);
						vproc_param.pfunc           = NULL;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = CloneIMECropInfo[tb.idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = CloneIMECropInfo[tb.idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = CloneIMECropInfo[tb.idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = CloneIMECropInfo[tb.idx].IMEWin.h;
						if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
							DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
						if (img_vproc_no_ext[tb.idx]) {
							_ImageApp_MovieMulti_UpdateVprcDepth(vproc_param.video_proc_path, 1);
						}
						if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j])) {
							if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j])) != HD_OK) {
								DBG_ERR("hd_videoproc_start fail: id=%d, ret=%d\r\n", ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j], hd_ret);
								ret = E_SYS;
							}
						}

						if (img_vproc_no_ext[tb.idx] == FALSE) {
							IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};
							vproc_param_ex.video_proc_path_ex  = ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_CLONE];
							vproc_param_ex.video_proc_path_src = ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j];
							vproc_param_ex.p_dim               = &sz;
							vproc_param_ex.p_dim->w            = CloneIMECropInfo[tb.idx].IMEWin.w;
							vproc_param_ex.p_dim->h            = CloneIMECropInfo[tb.idx].IMEWin.h;
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420;
							vproc_param_ex.dir                 = HD_VIDEO_DIR_NONE;
							vproc_param_ex.frc                 = _ImageApp_MovieMulti_frc(fps_vprc_e_out[tb.idx][IAMOVIE_VPRC_EX_CLONE], fps_vprc_p_out[tb.idx][gSwitchLink[tb.idx][j]]);
							vproc_param_ex.depth               = 1;                       // for rawenc case, the depth should keep 1

							memset((void *)&(vproc_param_ex.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
							if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
								DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
								ret = E_SYS;
							}
							if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_CLONE])) {
								if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_CLONE])) != HD_OK) {
									DBG_ERR("hd_videoproc_start fail: id=%d, ret=%d\r\n", ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_CLONE], hd_ret);
									ret = E_SYS;
								}
							}
						}
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_VPE_LITE_CROP: {
#if defined(_BSP_NA51102_)
			if (value) {
				if ((img_vproc_vpe_en[tb.idx] == MOVIE_VPE_1) && (img_vproc_eis_use_vpe_lite[tb.idx] == TRUE)) {
					MOVIEMULTI_IME_CROP_INFO *pInfo = (MOVIEMULTI_IME_CROP_INFO *) value;
					if ((pInfo->IMEWin.x + pInfo->IMEWin.w > pInfo->IMESize.w) || (pInfo->IMEWin.y + pInfo->IMEWin.h > pInfo->IMESize.h)) {
						DBG_ERR("[MOVIEMULTI_PARAM_VPRC_VPE_LITE_CROP] Window out of range! %d+%d>%d or %d+%d>%d\r\n", pInfo->IMEWin.x, pInfo->IMEWin.w, pInfo->IMESize.w, pInfo->IMEWin.y, pInfo->IMEWin.h, pInfo->IMESize.h);
						ret = E_PAR;
					} else {
						// check align
						if ((pInfo->IMESize.w % 32) || (pInfo->IMESize.h % 2) || (pInfo->IMEWin.x % 32) || (pInfo->IMEWin.y % 2) || (pInfo->IMEWin.w % 32) || (pInfo->IMEWin.h % 2)) {
							DBG_ERR("id=%d: Set MOVIEMULTI_PARAM_VPRC_VPE_LITE_CROP to size(%d,%d), win(%d,%d,%d,%d) but w/x should align 32 and h/y should align 2\r\n", i, pInfo->IMESize.w, pInfo->IMESize.h, pInfo->IMEWin.x, pInfo->IMEWin.y, pInfo->IMEWin.w, pInfo->IMEWin.h);
							ret = E_PAR;
							break;
						}
						// copy data structure
						ret = E_OK;
						memcpy(&(VPELiteCropInfo[tb.idx]), pInfo, sizeof(MOVIEMULTI_IME_CROP_INFO));
						ImageApp_MovieMulti_DUMP("id=%d: Set MOVIEMULTI_PARAM_VPRC_VPE_LITE_CROP to size(%d,%d), win(%d,%d,%d,%d)\r\n", i, pInfo->IMESize.w, pInfo->IMESize.h, pInfo->IMEWin.x, pInfo->IMEWin.y, pInfo->IMEWin.w, pInfo->IMEWin.h);

						if (IsImgLinkOpened[tb.idx] == TRUE) {     // already opened, update window
							IACOMM_VPROC_PARAM vproc_param = {0};
							HD_DIM sz;
							j = 0;
							vproc_param.video_proc_path = ImgLink[tb.idx].vproc_p_phy[2][j];
							vproc_param.p_dim           = &sz;
							vproc_param.p_dim->w        = VPELiteCropInfo[tb.idx].IMESize.w;
							vproc_param.p_dim->h        = VPELiteCropInfo[tb.idx].IMESize.h;
							vproc_param.pxlfmt          = (img_vproc_yuv_compress_for_eis[tb.idx] == TRUE) ? HD_VIDEO_PXLFMT_YUV420_NVX2 : HD_VIDEO_PXLFMT_YUV420;
							vproc_param.frc             = _ImageApp_MovieMulti_frc(1, 1);
							vproc_param.pfunc           = NULL;
							vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
							vproc_param.video_proc_crop_param.win.rect.x = VPELiteCropInfo[tb.idx].IMEWin.x;
							vproc_param.video_proc_crop_param.win.rect.y = VPELiteCropInfo[tb.idx].IMEWin.y;
							vproc_param.video_proc_crop_param.win.rect.w = VPELiteCropInfo[tb.idx].IMEWin.w;
							vproc_param.video_proc_crop_param.win.rect.h = VPELiteCropInfo[tb.idx].IMEWin.h;
							if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
								DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
								ret = E_SYS;
							}
							if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p_phy[2][j])) {
								if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p_phy[2][j])) != HD_OK) {
									DBG_ERR("hd_videoproc_start fail: id=%d, ret=%d\r\n", ImgLink[tb.idx].vproc_p_phy[2][j], hd_ret);
									ret = E_SYS;
								}
							}
						}
					}
				} else {
					DBG_ERR("id=%d: Set MOVIEMULTI_PARAM_VPRC_VPE_LITE_CROP fail, MOVIEMULTI_PARAM_VPE_EN and MOVIEMULTI_PARAM_VPRC_EIS_FUNC_USE_VPE_LITE should be set fist\r\n", i);
					ret = E_SYS;
				}
			} else {
				DBG_ERR("id=%d: Set MOVIEMULTI_PARAM_VPRC_VPE_LITE_CROP with NULL pointer\r\n", i);
				ret = E_PAR;
			}
#else
			DBG_ERR("MOVIEMULTI_PARAM_VPRC_VPE_LITE_CROP is not supported\r\n");
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_NO_EXT: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_VPRC_NO_EXT should set before open\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_vproc_no_ext[tb.idx] = value ? TRUE : FALSE;
					ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_VPRC_NO_EXT=%d to id%d:\r\n", img_vproc_no_ext[tb.idx], i);
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
					ethcam_vproc_no_ext[tb.idx] = value ? TRUE : FALSE;
					ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_VPRC_NO_EXT=%d to id%d:\r\n", ethcam_vproc_no_ext[tb.idx], i);
					ret = E_OK;
#else
					ret = E_NOSPT;
#endif
				}
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_CodecGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id, j;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_CODEC:
	case MOVIEMULTI_PARAM_CODEC_PROFILE_LEVEL: {
			MOVIE_CFG_CODEC_INFO *pinfo =  NULL, codec_info = {0};
			if (param == MOVIEMULTI_PARAM_CODEC) {
				codec_info.codec = value;
			} else {
				pinfo = (MOVIE_CFG_CODEC_INFO *)value;
				codec_info.codec   = pinfo->codec;
				codec_info.profile = pinfo->profile;
				codec_info.level   = pinfo->level;
				if (_ImageApp_MovieMulti_CheckProfileLevelValid(&codec_info) == FALSE) {
					ret = E_PAR;
					break;
				}
			}
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				UINT32 reopen = FALSE;
				if (ImgLinkRecInfo[tb.idx][tb.tbl].codec != codec_info.codec && img_vproc_yuv_compress[tb.idx] && tb.tbl == 0) {
					reopen = TRUE;
				}
				ImgLinkRecInfo[tb.idx][tb.tbl].codec = codec_info.codec;
				if (param == MOVIEMULTI_PARAM_CODEC_PROFILE_LEVEL) {
					if (codec_info.codec == _CFG_CODEC_H264) {
						img_venc_h264_profile[tb.idx][tb.tbl] = codec_info.profile;
						img_venc_h264_level[tb.idx][tb.tbl]   = codec_info.level;
					} else {
						img_venc_h265_profile[tb.idx][tb.tbl] = codec_info.profile;
						img_venc_h265_level[tb.idx][tb.tbl]   = codec_info.level;
					}
				}
				if (reopen) {
					_ImageApp_MovieMulti_ImgReopenVEnc(tb.idx, tb.tbl);
				}

				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// venc
					if (ImgLinkRecInfo[tb.idx][tb.tbl].codec == _CFG_CODEC_H264) {
						img_venc_out_param[tb.idx][tb.tbl].codec_type        = HD_CODEC_TYPE_H264;
						img_venc_out_param[tb.idx][tb.tbl].h26x.profile      = img_venc_h264_profile[tb.idx][tb.tbl] ? img_venc_h264_profile[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H264_PROFILE;
						img_venc_out_param[tb.idx][tb.tbl].h26x.level_idc    = img_venc_h264_level[tb.idx][tb.tbl] ? img_venc_h264_level[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H264_LEVEL;
						img_venc_out_param[tb.idx][tb.tbl].h26x.entropy_mode = (img_venc_h264_profile[tb.idx][tb.tbl] != _CFG_H264_PROFILE_BASELINE) ? HD_H264E_CABAC_CODING : HD_H264E_CAVLC_CODING;
					} else {
						img_venc_out_param[tb.idx][tb.tbl].codec_type        = HD_CODEC_TYPE_H265;
						img_venc_out_param[tb.idx][tb.tbl].h26x.profile      = img_venc_h265_profile[tb.idx][tb.tbl] ? img_venc_h265_profile[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H265_PROFILE;
						img_venc_out_param[tb.idx][tb.tbl].h26x.level_idc    = img_venc_h265_level[tb.idx][tb.tbl] ? img_venc_h265_level[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H265_LEVEL;
						img_venc_out_param[tb.idx][tb.tbl].h26x.entropy_mode = HD_H265E_CABAC_CODING;
					}
					IACOMM_VENC_PARAM venc_param = {0};
					venc_param.video_enc_path = ImgLink[tb.idx].venc_p[tb.tbl];
					venc_param.pin            = &(img_venc_in_param[tb.idx][tb.tbl]);
					venc_param.pout           = &(img_venc_out_param[tb.idx][tb.tbl]);
					venc_param.prc            = &(img_venc_rc_param[tb.idx][tb.tbl]);
					if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
						DBG_ERR("venc_set_param fail(%d)\n", ret);
					}
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						img_bsmux_vinfo[tb.idx][2*tb.tbl+j].vidcodec                  = ImgLinkRecInfo[tb.idx][tb.tbl].codec;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_VIDEOINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_vinfo[tb.idx][2*tb.tbl+j]);
						if ((hd_ret = bsmux_set_param(&bsmux_param)) != HD_OK) {
							DBG_ERR("bsmux_set_param(HD_BSMUX_PARAM_VIDEOINFO) fail(%d)\n", hd_ret);
						}
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				// var
				UINT32 reopen = FALSE;
				if (WifiLinkStrmInfo[tb.idx].codec != codec_info.codec) {
					reopen = TRUE;
				}
				WifiLinkStrmInfo[tb.idx].codec = codec_info.codec;
				if (param == MOVIEMULTI_PARAM_CODEC_PROFILE_LEVEL) {
					if (codec_info.codec == _CFG_CODEC_H264) {
						wifi_venc_h264_profile[tb.idx] = codec_info.profile;
						wifi_venc_h264_level[tb.idx]   = codec_info.level;
					} else {
						wifi_venc_h265_profile[tb.idx] = codec_info.profile;
						wifi_venc_h265_level[tb.idx]   = codec_info.level;
					}
				}
				if (reopen) {
					_ImageApp_MovieMulti_WifiReopenVEnc(tb.idx);
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
					_iacommp_set_streaming_mode((WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_MJPG) ? IACOMMON_STREAMING_MODE_LVIEWNVT : IACOMMON_STREAMING_MODE_LIVE555);
#endif
				}

				if (IsWifiLinkOpened[tb.idx] == TRUE) {
					// venc
					if (WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_MJPG) {
						wifi_venc_out_param[tb.idx].codec_type                 = HD_CODEC_TYPE_JPEG;
						wifi_venc_out_param[tb.idx].jpeg.retstart_interval     = 0;
						wifi_venc_out_param[tb.idx].jpeg.image_quality         = 50;
						wifi_venc_out_param[tb.idx].jpeg.bitrate               = 0;
						wifi_venc_out_param[tb.idx].jpeg.frame_rate_base       = 1;
						wifi_venc_out_param[tb.idx].jpeg.frame_rate_incr       = 1;
					} else if (WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_H264) {
						wifi_venc_out_param[tb.idx].codec_type        = HD_CODEC_TYPE_H264;
						wifi_venc_out_param[tb.idx].h26x.profile      = wifi_venc_h264_profile[tb.idx] ? wifi_venc_h264_profile[tb.idx] : IAMVOIE_DEFAULT_H264_PROFILE;
						wifi_venc_out_param[tb.idx].h26x.level_idc    = wifi_venc_h264_level[tb.idx] ? wifi_venc_h264_level[tb.idx] : IAMVOIE_DEFAULT_H264_LEVEL;
						wifi_venc_out_param[tb.idx].h26x.entropy_mode = (wifi_venc_h264_profile[tb.idx] != _CFG_H264_PROFILE_BASELINE) ? HD_H264E_CABAC_CODING : HD_H264E_CAVLC_CODING;
					} else {
						wifi_venc_out_param[tb.idx].codec_type        = HD_CODEC_TYPE_H265;
						wifi_venc_out_param[tb.idx].h26x.profile      = wifi_venc_h265_profile[tb.idx] ? wifi_venc_h265_profile[tb.idx] : IAMVOIE_DEFAULT_H265_PROFILE;
						wifi_venc_out_param[tb.idx].h26x.level_idc    = wifi_venc_h265_level[tb.idx] ? wifi_venc_h265_level[tb.idx] : IAMVOIE_DEFAULT_H265_LEVEL;
						wifi_venc_out_param[tb.idx].h26x.entropy_mode = HD_H265E_CABAC_CODING;
					}
					IACOMM_VENC_PARAM venc_param = {0};
					venc_param.video_enc_path = WifiLink[tb.idx].venc_p[0];
					venc_param.pin            = &(wifi_venc_in_param[tb.idx]);
					venc_param.pout           = &(wifi_venc_out_param[tb.idx]);
					venc_param.prc            = &(wifi_venc_rc_param[tb.idx]);
					if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
						DBG_ERR("venc_set_param fail(%d)\n", ret);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				UINT32 reopen = FALSE;
				if (is_ethcamlink_opened[tb.idx] && (EthCamLinkRecInfo[tb.idx].codec != codec_info.codec)) {
					reopen = TRUE;
				}
				EthCamLinkRecInfo[tb.idx].codec = codec_info.codec;
				if (param == MOVIEMULTI_PARAM_CODEC_PROFILE_LEVEL) {
					if (codec_info.codec == _CFG_CODEC_H264) {
						ethcam_venc_h264_profile[tb.idx] = codec_info.profile;
						ethcam_venc_h264_level[tb.idx]   = codec_info.level;
					} else {
						ethcam_venc_h265_profile[tb.idx] = codec_info.profile;
						ethcam_venc_h265_level[tb.idx]   = codec_info.level;
					}
				}
				if (reopen) {
					_ImageApp_MovieMulti_EthCamReopenVEnc(tb.idx);
				}
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// venc
					if (EthCamLinkRecInfo[tb.idx].codec == _CFG_CODEC_MJPG) {
						ethcam_venc_out_param[tb.idx].codec_type                 = HD_CODEC_TYPE_JPEG;
						ethcam_venc_out_param[tb.idx].jpeg.retstart_interval     = 0;
						ethcam_venc_out_param[tb.idx].jpeg.image_quality         = 50;
						ethcam_venc_out_param[tb.idx].jpeg.bitrate               = 0;
						ethcam_venc_out_param[tb.idx].jpeg.frame_rate_base       = 1;
						ethcam_venc_out_param[tb.idx].jpeg.frame_rate_incr       = 1;
					} else if (EthCamLinkRecInfo[tb.idx].codec == _CFG_CODEC_H264) {
						ethcam_venc_out_param[tb.idx].codec_type        = HD_CODEC_TYPE_H264;
						ethcam_venc_out_param[tb.idx].h26x.profile      = ethcam_venc_h264_profile[tb.idx] ? ethcam_venc_h264_profile[tb.idx] : IAMVOIE_DEFAULT_H264_PROFILE;
						ethcam_venc_out_param[tb.idx].h26x.level_idc    = ethcam_venc_h264_level[tb.idx] ? ethcam_venc_h264_level[tb.idx] : IAMVOIE_DEFAULT_H264_LEVEL;
						ethcam_venc_out_param[tb.idx].h26x.entropy_mode = (ethcam_venc_h264_profile[tb.idx] != _CFG_H264_PROFILE_BASELINE) ? HD_H264E_CABAC_CODING : HD_H264E_CAVLC_CODING;
					} else {
						ethcam_venc_out_param[tb.idx].codec_type        = HD_CODEC_TYPE_H265;
						ethcam_venc_out_param[tb.idx].h26x.profile      = ethcam_venc_h265_profile[tb.idx] ? ethcam_venc_h265_profile[tb.idx] : IAMVOIE_DEFAULT_H265_PROFILE;
						ethcam_venc_out_param[tb.idx].h26x.level_idc    = ethcam_venc_h265_level[tb.idx] ? ethcam_venc_h265_level[tb.idx] : IAMVOIE_DEFAULT_H265_LEVEL;
						ethcam_venc_out_param[tb.idx].h26x.entropy_mode = HD_H265E_CABAC_CODING;
					}
					IACOMM_VENC_PARAM venc_param = {0};
					venc_param.video_enc_path = EthCamLink[tb.idx].venc_p[0];
					venc_param.pin            = &(ethcam_venc_in_param[tb.idx]);
					venc_param.pout           = &(ethcam_venc_out_param[tb.idx]);
					venc_param.prc            = &(ethcam_venc_rc_param[tb.idx]);
					if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
						DBG_ERR("venc_set_param fail(%d)\n", ret);
					}
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						ethcam_bsmux_vinfo[tb.idx][j].vidcodec                         = EthCamLinkRecInfo[tb.idx].codec;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_VIDEOINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_vinfo[tb.idx][j]);
						if ((hd_ret = bsmux_set_param(&bsmux_param)) != HD_OK) {
							DBG_ERR("bsmux_set_param(HD_BSMUX_PARAM_VIDEOINFO) fail(%d)\n", hd_ret);
						}
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_TIMELAPSE_TIME: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				ImgLinkTimelapseTime[tb.idx][tb.tbl] = value;
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// venc
					VENDOR_VIDEOENC_TIMELAPSE_TIME_CFG timelapse_cfg = {0};
					timelapse_cfg.timelapse_time = (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_TIMELAPSE) ? ImgLinkTimelapseTime[tb.idx][tb.tbl] : 0;
					if ((hd_ret = vendor_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], VENDOR_VIDEOENC_PARAM_OUT_TIMELAPSE_TIME, &timelapse_cfg)) != HD_OK) {
						DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_TIMELAPSE_TIME) fail(%d)\n", hd_ret);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				EthCamLinkTimelapseTime[tb.idx] = value;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// venc
					VENDOR_VIDEOENC_TIMELAPSE_TIME_CFG timelapse_cfg = {0};
					timelapse_cfg.timelapse_time = (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_TIMELAPSE) ? EthCamLinkTimelapseTime[tb.idx] : 0;
					if ((hd_ret = vendor_videoenc_set(EthCamLink[tb.idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_TIMELAPSE_TIME, &timelapse_cfg)) != HD_OK) {
						DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_TIMELAPSE_TIME) fail(%d)\n", hd_ret);
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_ENCBUF_MS: {
			ret = E_OK;
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				ImgLinkVdoEncBufMs[tb.idx][tb.tbl] = value;
				if (tb.tbl == MOVIETYPE_MAIN) {
					img_venc_path_cfg[tb.idx][tb.tbl].max_mem.enc_buf_ms = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? (ImgLinkVdoEncBufMs[tb.idx][tb.tbl] + ImgLinkFileInfo.emr_sec * 1000) : ImgLinkVdoEncBufMs[tb.idx][tb.tbl];
				} else {
					img_venc_path_cfg[tb.idx][tb.tbl].max_mem.enc_buf_ms = (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) ? (ImgLinkVdoEncBufMs[tb.idx][tb.tbl] + ImgLinkFileInfo.clone_emr_sec * 1000) : ImgLinkVdoEncBufMs[tb.idx][tb.tbl];
				}
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					/*
					HD_VIDEOENC_FUNC_CONFIG img_venc_func_cfg = {0};
					img_venc_func_cfg.ddr_id  = dram_cfg.video_encode;
					img_venc_func_cfg.in_func = 0;
					IACOMM_VENC_CFG venc_cfg = {0};
					venc_cfg.video_enc_path = ImgLink[tb.idx].venc_p[tb.tbl];
					venc_cfg.ppath_cfg      = &(img_venc_path_cfg[tb.idx][tb.tbl]);
					venc_cfg.pfunc_cfg      = &(img_venc_func_cfg);
					if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
						DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
					*/
					_ImageApp_MovieMulti_ImgReopenVEnc(tb.idx, tb.tbl);
				}
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				WifiLinkVdoEncBufMs[tb.idx] = value;
				wifi_venc_path_cfg[tb.idx].max_mem.enc_buf_ms = WifiLinkVdoEncBufMs[tb.idx];
				if (IsWifiLinkOpened[tb.idx] == TRUE) {
					/*
					HD_VIDEOENC_FUNC_CONFIG wifi_venc_func_cfg = {0};
					wifi_venc_func_cfg.ddr_id  = dram_cfg.video_encode;
					wifi_venc_func_cfg.in_func = 0;
					IACOMM_VENC_CFG venc_cfg = {0};
					venc_cfg.video_enc_path = WifiLink[tb.idx].venc_p[0];
					venc_cfg.ppath_cfg      = &(wifi_venc_path_cfg[tb.idx]);
					venc_cfg.pfunc_cfg      = &(wifi_venc_func_cfg);
					if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
						DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
					*/
					_ImageApp_MovieMulti_WifiReopenVEnc(tb.idx);
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				EthCamLinkVdoEncBufMs[tb.idx] = value;
				ethcam_venc_path_cfg[tb.idx].max_mem.enc_buf_ms = EthCamLinkVdoEncBufMs[tb.idx];
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					/*
					HD_VIDEOENC_FUNC_CONFIG ethcam_venc_func_cfg = {0};
					ethcam_venc_func_cfg.ddr_id  = dram_cfg.video_encode;
					ethcam_venc_func_cfg.in_func = 0;
					IACOMM_VENC_CFG venc_cfg = {0};
					venc_cfg.video_enc_path = EthCamLink[tb.idx].venc_p[0];
					venc_cfg.ppath_cfg      = &(ethcam_venc_path_cfg[tb.idx]);
					venc_cfg.pfunc_cfg      = &(ethcam_venc_func_cfg);
					if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
						DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
					*/
					#if (IAMOVIE_SUPPORT_VPE == ENABLE)
					_ImageApp_MovieMulti_EthCamReopenVPrc(tb.idx);
					#endif
				}
			} else {
				DBG_ERR("Set MOVIEMULTI_PARAM_ENCBUF_MS to %d is out of range\r\n", value);
				ret = E_PAR;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VDO_ENC_REQUEST_I: {
			HD_H26XENC_REQUEST_IFRAME req_i = {0};
			req_i.enable = 1;
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (_LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[tb.tbl])) {
					if ((hd_ret = hd_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &req_i)) != HD_OK) {
						DBG_ERR("HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME fail(%d)\r\n", hd_ret);
					}
					if ((hd_ret = hd_videoenc_start(ImgLink[tb.idx].venc_p[tb.tbl])) != HD_OK) {
						DBG_ERR("hd_videoenc_start fail(%d)\r\n", hd_ret);
					}
					ret = E_OK;
				} else {
					DBG_ERR("id%d is not started.\r\n", i);
				}
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				if (_LinkCheckStatus(WifiLinkStatus[tb.idx].venc_p[0])) {
					if ((hd_ret = hd_videoenc_set(WifiLink[tb.idx].venc_p[0], HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &req_i)) != HD_OK) {
						DBG_ERR("HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME fail(%d)\r\n", hd_ret);
					}
					if ((hd_ret = hd_videoenc_start(WifiLink[tb.idx].venc_p[0])) != HD_OK) {
						DBG_ERR("hd_videoenc_start fail(%d)\r\n", hd_ret);
					}
					ret = E_OK;
				} else {
					DBG_ERR("id%d is not started.\r\n", i);
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (_LinkCheckStatus(EthCamLinkStatus[tb.idx].venc_p[0])) {
					if ((hd_ret = hd_videoenc_set(EthCamLink[tb.idx].venc_p[0], HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &req_i)) != HD_OK) {
						DBG_ERR("HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME fail(%d)\r\n", hd_ret);
					}
					if ((hd_ret = hd_videoenc_start(EthCamLink[tb.idx].venc_p[0])) != HD_OK) {
						DBG_ERR("hd_videoenc_start fail(%d)\r\n", hd_ret);
					}
					ret = E_OK;
				} else {
					DBG_ERR("id%d is not started.\r\n", i);
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_VDO_ENC_REG_CB: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				vdo_bs_rec_cb[tb.idx][tb.tbl] = (MovieVdoBsCb *)value;
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				vdo_bs_strm_cb[tb.idx] = (MovieVdoBsCb *)value;
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				vdo_bs_ethcam_cb[tb.idx] = (MovieVdoBsCb *)value;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VDO_QUALITY_BASE_MODE_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				img_venc_quality_base_mode_en[tb.idx][tb.tbl] = value ? ENABLE : DISABLE;
				ret = E_OK;
			} else if((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				ethcam_venc_quality_base_mode_en[tb.idx] = value ? ENABLE : DISABLE;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_TARGETRATE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				ret = E_OK;
				img_venc_rc_param[tb.idx][tb.tbl].cbr.bitrate = value * 8;
				if (value > ImgLinkRecInfo[tb.idx][tb.tbl].target_bitrate) {
					DBG_WRN("Set MOVIEMULTI_PARAM_TARGETRATE(id=%x, tbr=%d bytes), but new tbr > init setting(%d bytes)\r\n", ImgLink[tb.idx].venc_p[tb.tbl], value, ImgLinkRecInfo[tb.idx][tb.tbl].target_bitrate);
				} else {
					DBG_DUMP("Set MOVIEMULTI_PARAM_TARGETRATE(id=%x, tbr=%d bytes), init setting(%d bytes)\r\n", ImgLink[tb.idx].venc_p[tb.tbl], value, ImgLinkRecInfo[tb.idx][tb.tbl].target_bitrate);
				}
				IACOMM_VENC_PARAM venc_param = {0};
				venc_param.video_enc_path = ImgLink[tb.idx].venc_p[tb.tbl];
				venc_param.pin            = NULL;
				venc_param.pout           = NULL;
				venc_param.prc            = &(img_venc_rc_param[tb.idx][tb.tbl]);
				if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
					DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
					ret = E_SYS;
				} else {
					if (_LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[tb.tbl])) {
						if ((hd_ret = hd_videoenc_start(ImgLink[tb.idx].venc_p[tb.tbl])) != HD_OK) {
							DBG_ERR("hd_videoenc_start fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				ret = E_OK;
				//WifiLinkStrmInfo[tb.idx].max_bit_rate = value;
				//wifi_venc_rc_param[tb.idx].cbr.bitrate = WifiLinkStrmInfo[tb.idx].max_bit_rate * 8;
				wifi_venc_rc_param[tb.idx].cbr.bitrate = value * 8;
				if (value > WifiLinkStrmInfo[tb.idx].max_bit_rate) {
					DBG_WRN("Set MOVIEMULTI_PARAM_TARGETRATE(id=%x, tbr=%d bytes), but new tbr > init setting(%d bytes)\r\n", WifiLink[tb.idx].venc_p[0], value, WifiLinkStrmInfo[tb.idx].max_bit_rate);
				} else {
					DBG_DUMP("Set MOVIEMULTI_PARAM_TARGETRATE(id=%x, tbr=%d bytes), init setting(%d bytes)\r\n", WifiLink[tb.idx].venc_p[0], value, WifiLinkStrmInfo[tb.idx].max_bit_rate);
				}
				IACOMM_VENC_PARAM venc_param = {0};
				venc_param.video_enc_path = WifiLink[tb.idx].venc_p[0];
				venc_param.pin            = NULL;
				venc_param.pout           = NULL;
				venc_param.prc            = &(wifi_venc_rc_param[tb.idx]);
				if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
					DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
					ret = E_SYS;
				} else {
					if (_LinkCheckStatus(WifiLinkStatus[tb.idx].venc_p[0])) {
						if ((hd_ret = hd_videoenc_start(WifiLink[tb.idx].venc_p[0])) != HD_OK) {
							DBG_ERR("hd_videoenc_start fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ret = E_OK;
				ethcam_venc_rc_param[tb.idx].cbr.bitrate = value * 8;
				if (value > EthCamLinkRecInfo[tb.idx].tbr) {
					DBG_WRN("Set MOVIEMULTI_PARAM_TARGETRATE(id=%x, tbr=%d bytes), but new tbr > init setting(%d bytes)\r\n", EthCamLink[tb.idx].venc_p[0], value, EthCamLinkRecInfo[tb.idx].tbr);
				} else {
					DBG_DUMP("Set MOVIEMULTI_PARAM_TARGETRATE(id=%x, tbr=%d bytes), init setting(%d bytes)\r\n", EthCamLink[tb.idx].venc_p[0], value, EthCamLinkRecInfo[tb.idx].tbr);
				}
				IACOMM_VENC_PARAM venc_param = {0};
				venc_param.video_enc_path = EthCamLink[tb.idx].venc_p[0];
				venc_param.pin            = NULL;
				venc_param.pout           = NULL;
				venc_param.prc            = &(ethcam_venc_rc_param[tb.idx]);
				if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
					DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
					ret = E_SYS;
				} else {
					if (_LinkCheckStatus(EthCamLinkStatus[tb.idx].venc_p[0])) {
						if ((hd_ret = hd_videoenc_start(EthCamLink[tb.idx].venc_p[0])) != HD_OK) {
							DBG_ERR("hd_videoenc_start fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_TIMER_TRIGGER_COMPENSATION: {
			#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
			VENDOR_VIDEOENC_TIMER_TRIG_COMP comp_cfg = {0};
			comp_cfg.b_enable = value ? TRUE : FALSE;
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if ((hd_ret = vendor_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], VENDOR_VIDEOENC_PARAM_OUT_TIMER_TRIG_COMP, &comp_cfg)) != HD_OK) {
					DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_TIMMER_TRIG_COMP) fail(%d)\n", hd_ret);
				} else {
					DBG_DUMP("Set MOVIEMULTI_PARAM_TIMER_TRIGGER_COMPENSATION = %d to imglink.venc[%d][%d]\r\n", comp_cfg.b_enable, tb.idx, tb.tbl);
					ret = E_OK;
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if ((hd_ret = vendor_videoenc_set(EthCamLink[tb.idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_TIMER_TRIG_COMP, &comp_cfg)) != HD_OK) {
					DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_TIMMER_TRIG_COMP) fail(%d)\n", hd_ret);
				} else {
					DBG_DUMP("Set MOVIEMULTI_PARAM_TIMER_TRIGGER_COMPENSATION = %d to ethcamlink.venc[%d]\r\n", comp_cfg.b_enable, tb.idx);
					ret = E_OK;
				}
			} else {
				ret = E_NOSPT;
			}
			#else
			ret = E_NOSPT;
			#endif
			break;
		}

	case MOVIEMULTI_PARAM_VUI_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				img_venc_vui_disable[tb.idx][tb.tbl] = value ? FALSE : TRUE;
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				wifi_venc_vui_disable[tb.idx] = value ? FALSE : TRUE;
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ethcam_venc_vui_disable[tb.idx] = value ? FALSE : TRUE;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VDO_ENC_MAX_FRAME_SIZE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				img_venc_max_frame_size[tb.idx][tb.tbl] = value;
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				wifi_venc_max_frame_size[tb.idx] = value;
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ethcam_venc_max_frame_size[tb.idx] = value;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VDO_ENC_PARAM: {
			if (value == 0) {
				DBG_ERR("MOVIEMULTI_PARAM_VDO_ENC_PARAM is set with NULL value\r\n");
				ret = E_PAR;
			} else {
				if (IsImgLinkOpened[tb.idx] == FALSE) {
					DBG_ERR("MOVIEMULTI_PARAM_VDO_ENC_PARAM can only be set after open\r\n");
					ret = E_SYS;
					break;
				}
				if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
					MOVIEMULTI_VDO_ENC_PARAM *penc_param = (MOVIEMULTI_VDO_ENC_PARAM *)value;
					if (_LinkCheckStatus(WifiLinkStatus[tb.idx].venc_p[0])) {
						DBG_ERR("MOVIEMULTI_PARAM_VDO_ENC_PARAM cannot be set since id[%d] is running\r\n", i);
						ret = E_SYS;
						break;
					}
					if (_ImageApp_MovieMulti_CheckProfileLevelValid((MOVIE_CFG_CODEC_INFO *)penc_param) == FALSE) {
						DBG_ERR("MOVIEMULTI_PARAM_VDO_ENC_PARAM invalid codec(%d)/prfile(%d)/level(%d) setting\r\n", penc_param->codec, penc_param->profile, penc_param->level);
						ret = E_PAR;
						break;
					}
					if (penc_param->tbr > (wifi_venc_path_cfg[tb.idx].max_mem.bitrate / 8)) {
						DBG_WRN("MOVIEMULTI_PARAM_VDO_ENC_PARAM tbr %d is larger than original %d, force set to %d\r\n", penc_param->tbr, wifi_venc_path_cfg[tb.idx].max_mem.bitrate, wifi_venc_path_cfg[tb.idx].max_mem.bitrate);
						penc_param->tbr = wifi_venc_path_cfg[tb.idx].max_mem.bitrate / 8;
					}
					WifiLinkStrmInfo[tb.idx].codec = penc_param->codec;
					if (WifiLinkStrmInfo[tb.idx].codec == _CFG_CODEC_H264) {
						wifi_venc_out_param[tb.idx].codec_type        = HD_CODEC_TYPE_H264;
						wifi_venc_h264_profile[tb.idx]                = penc_param->profile;
						wifi_venc_out_param[tb.idx].h26x.profile      = penc_param->profile;
						wifi_venc_h264_level[tb.idx]                  = penc_param->level;
						wifi_venc_out_param[tb.idx].h26x.level_idc    = penc_param->level;
						wifi_venc_out_param[tb.idx].h26x.entropy_mode = (penc_param->profile != _CFG_H264_PROFILE_BASELINE) ? HD_H264E_CABAC_CODING : HD_H264E_CAVLC_CODING;
					} else {
						wifi_venc_out_param[tb.idx].codec_type        = HD_CODEC_TYPE_H265;
						wifi_venc_h265_profile[tb.idx]                = penc_param->profile;
						wifi_venc_out_param[tb.idx].h26x.profile      = penc_param->profile;
						wifi_venc_h265_level[tb.idx]                  = penc_param->level;
						wifi_venc_out_param[tb.idx].h26x.level_idc    = penc_param->level;
						wifi_venc_out_param[tb.idx].h26x.entropy_mode = HD_H265E_CABAC_CODING;
					}
					wifi_venc_out_param[tb.idx].h26x.gop_num       = penc_param->gop;
					wifi_venc_rc_param[tb.idx].cbr.bitrate         = penc_param->tbr * 8;
					wifi_venc_rc_param[tb.idx].cbr.frame_rate_base = penc_param->fps;
					IACOMM_VENC_PARAM venc_param = {
						.video_enc_path = WifiLink[tb.idx].venc_p[0],
						.pin            = NULL,
						.pout           = &(wifi_venc_out_param[tb.idx]),
						.prc            = &(wifi_venc_rc_param[tb.idx]),
					};
					if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
						DBG_ERR("venc_set_param fail(%d)\n", ret);
						ret = E_SYS;
						break;
					}
					DBG_DUMP("MOVIEMULTI_PARAM_VDO_ENC_PARAM set id[%d] to fps=%d, gop=%d, tbr=%d(bytes)\r\n", i, penc_param->fps, penc_param->gop, penc_param->tbr);
					ret = E_OK;
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_VDO_ENC_PARAM is not support in linktype[%d]\r\n", tb.link_type);
					ret = E_NOSPT;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_XVR_APP: {
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_CODEC_XVR_APP should set before open\r\n");
				ret = E_SYS;
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					// var
					img_venc_xvr_app[tb.idx][tb.tbl] = value;
					DBG_DUMP("MOVIEMULTI_PARAM_CODEC_XVR_APP set rec_id%d to %d\r\n", i, img_venc_xvr_app[tb.idx][tb.tbl]);
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
					// var
					wifi_venc_xvr_app[tb.idx] = value;
					DBG_DUMP("MOVIEMULTI_PARAM_CODEC_XVR_APP set rec_id%d to %d\r\n", i, wifi_venc_xvr_app[tb.idx]);
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					// var
					ethcam_venc_xvr_app[tb.idx] = value;
					DBG_DUMP("MOVIEMULTI_PARAM_CODEC_XVR_APP set rec_id%d to %d\r\n", i, ethcam_venc_xvr_app[tb.idx]);
					ret = E_OK;
				}
			}
#else
			DBG_ERR("MOVIEMULTI_PARAM_CODEC_XVR_APP is not supported in this chip.\r\n");
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_SKIP_FRAME_MODE: {
#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (_LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[tb.tbl])) {
					DBG_ERR("MOVIEMULTI_PARAM_CODEC_SKIP_FRAME_MODE cannot set when venc is running\r\n");
					ret = E_SYS;
				} else {
					// var
					img_venc_skip_frame_cfg[tb.idx][tb.tbl] = value;
					DBG_DUMP("MOVIEMULTI_PARAM_CODEC_SKIP_FRAME_MODE set rec_id%d to 0x%x\r\n", i, img_venc_skip_frame_cfg[tb.idx][tb.tbl]);
					ret = E_OK;
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (_LinkCheckStatus(EthCamLinkStatus[tb.idx].venc_p[0])) {
					DBG_ERR("MOVIEMULTI_PARAM_CODEC_SKIP_FRAME_MODE cannot set when venc is running\r\n");
					ret = E_SYS;
				} else {
					// var
					ethcam_venc_skip_frame_cfg[tb.idx] = value;
					DBG_DUMP("MOVIEMULTI_PARAM_CODEC_SKIP_FRAME_MODE set rec_id%d to 0x%x\r\n", i, ethcam_venc_skip_frame_cfg[tb.idx]);
					ret = E_OK;
				}
			} else {
				DBG_ERR("MOVIEMULTI_PARAM_CODEC_SKIP_FRAME_MODE is not supported for link_type %d\r\n", tb.link_type);
				ret = E_NOSPT;
			}
#else
			DBG_ERR("MOVIEMULTI_PARAM_CODEC_SKIP_FRAME_MODE is not supported in this chip.\r\n");
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_SVC: {
			if (IsImgLinkOpened[tb.idx] == TRUE) {
				DBG_ERR("MOVIEMULTI_PARAM_VDO_ENC_PARAM can only be set before open\r\n");
				ret = E_SYS;
			} else {
				if (value < HD_SVC_MAX) {
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						img_venc_svc[tb.idx][tb.tbl] = value;
						ret = E_OK;
					} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
						wifi_venc_svc[tb.idx] = value;
						ret = E_OK;
					} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
						ethcam_venc_svc[tb.idx] = value;
						ret = E_OK;
					} else {
						DBG_ERR("MOVIEMULTI_PARAM_CODEC_SVC is not support in linktype[%d]\r\n", tb.link_type);
						ret = E_NOSPT;
					}
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_CODEC_SVC sets invalid parameter(%d)\r\n", value);
					ret = E_PAR;
				}
			}
			if (ret == E_OK) {
				ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_CODEC_SVC]: sets %d to id%d\r\n", value, i);
			}
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_LOW_POWER_MODE: {
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
			if (IsImgLinkOpened[tb.idx] == TRUE) {
				DBG_ERR("MOVIEMULTI_PARAM_CODEC_LOW_POWER_MODE can only be set before open\r\n");
				ret = E_SYS;
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_venc_low_power_mode[tb.idx][tb.tbl] = value ? TRUE : FALSE;
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
					wifi_venc_low_power_mode[tb.idx] = value ? TRUE : FALSE;
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					ethcam_venc_low_power_mode[tb.idx] = value ? TRUE : FALSE;
					ret = E_OK;
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_CODEC_LOW_POWER_MODE is not support in linktype[%d]\r\n", tb.link_type);
					ret = E_NOSPT;
				}
			}
			if (ret == E_OK) {
				ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_CODEC_LOW_POWER_MODE]: sets %d to id%d\r\n", value, i);
			}
#else
			DBG_ERR("MOVIEMULTI_PARAM_CODEC_LOW_POWER_MODE is not support in this branch\r\n");
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_META_DATA_CFG: {
			#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
			if (value) {
				if (IsMovieMultiOpened == FALSE) {
					DBG_ERR("Cannot set MOVIEMULTI_PARAM_CODEC_META_DATA_CFG before ImageApp_MovieMulti Open\r\n");
					ret = E_SYS;
				} else {
					VENDOR_META_ALLOC *ptr = (VENDOR_META_ALLOC *)value;
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						if (_LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[tb.tbl])) {
							DBG_ERR("Cannot do vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_META_ALLC) after start(%d)\r\n");
						} else {
							if ((hd_ret = vendor_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], VENDOR_VIDEOENC_PARAM_META_ALLC, (VOID *)ptr)) != HD_OK) {
								DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_META_ALLC) fail(%d)\r\n", hd_ret);
								ret = E_SYS;
							} else {
								ret = E_OK;
							}
						}
					} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
						if (_LinkCheckStatus(EthCamLinkStatus[tb.idx].venc_p[0])) {
							DBG_ERR("Cannot do vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_META_ALLC) after start(%d)\r\n");
						} else {
							if ((hd_ret = vendor_videoenc_set(EthCamLink[tb.idx].venc_p[0], VENDOR_VIDEOENC_PARAM_META_ALLC, (VOID *)ptr)) != HD_OK) {
								DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_META_ALLC) fail(%d)\r\n", hd_ret);
								ret = E_SYS;
							} else {
								ret = E_OK;
							}
						}
					}
				}
			} else {
				DBG_ERR("Set MOVIEMULTI_PARAM_CODEC_META_DATA_CFG fail. Param is NULL.\r\n");
				ret = E_PAR;
			}
			#else
			ret = E_NOSPT;
			#endif
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_TRIGGER_MODE: {
			if (value < MOVIE_CODEC_TRIGGER_MAX) {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_venc_trigger_mode[tb.idx][tb.tbl] = value;
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
					wifi_venc_trigger_mode[tb.idx] = value;
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					ethcam_venc_trigger_mode[tb.idx] = value;
					ret = E_OK;
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_CODEC_TRIGGER_MODE is not support in linktype[%d]\r\n", tb.link_type);
					ret = E_NOSPT;
				}
				if (ret == E_OK) {
					ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_CODEC_TRIGGER_MODE]: set id%d=%d\r\n", i, value);
				}
			} else {
				DBG_ERR("[MOVIEMULTI_PARAM_CODEC_TRIGGER_MODE] invalid param (%d)\r\n", value);
				ret = E_PAR;
			}
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_RRC:
	case MOVIEMULTI_PARAM_CODEC_AQ: {
			char param_string[30] = {0};
			UINT32 cmd = 0;
			if (param == MOVIEMULTI_PARAM_CODEC_RRC) {
				strncpy(param_string, "MOVIEMULTI_PARAM_CODEC_RRC", sizeof(param_string)-1);
				cmd = HD_VIDEOENC_PARAM_OUT_ROW_RC;
			} else if (param == MOVIEMULTI_PARAM_CODEC_AQ) {
				strncpy(param_string, "MOVIEMULTI_PARAM_CODEC_AQ", sizeof(param_string)-1);
				cmd = HD_VIDEOENC_PARAM_OUT_AQ;
			}
			if (!value) {
				DBG_ERR("[%s] param is NULL\r\n", param_string);
				ret = E_PAR;
				break;
			}
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					if ((hd_ret = hd_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], cmd, (void *)value)) != HD_OK) {
						DBG_ERR("[%s] hd_videoenc_set failed(%d)\r\n", param_string, hd_ret);
						ret = E_SYS;
						break;
					}
					ret = E_OK;
					if (ImgLinkStatus[tb.idx].venc_p[tb.tbl]) {
						if ((hd_ret = hd_videoenc_start(ImgLink[tb.idx].venc_p[tb.tbl])) != HD_OK) {
							DBG_ERR("hd_videocap_start fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						}
					}
				} else {
					DBG_ERR("Please set %s after ImageApp_MovieMulti_Open()\r\n", param_string);
					ret = E_SYS;
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					if ((hd_ret = hd_videoenc_set(EthCamLink[tb.idx].venc_p[0], cmd, (void *)value)) != HD_OK) {
						DBG_ERR("[%s] hd_videoenc_set failed(%d)\r\n", param_string, hd_ret);
						ret = E_SYS;
						break;
					}
					ret = E_OK;
					if (EthCamLinkStatus[tb.idx].venc_p[0]) {
						if ((hd_ret = hd_videoenc_start(EthCamLink[tb.idx].venc_p[0])) != HD_OK) {
							DBG_ERR("hd_videocap_start fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						}
					}
				} else {
					DBG_ERR("Please set %s after ImageApp_MovieMulti_Open()\r\n", param_string);
					ret = E_SYS;
				}
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_MAXMEM: {
			if (value) {
				HD_VIDEOENC_MAXMEM *ptr = (HD_VIDEOENC_MAXMEM *)value;
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					memcpy((void *)&(img_venc_maxmem[tb.idx][tb.tbl]), (void *)ptr, sizeof(HD_VIDEOENC_MAXMEM));
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
					memcpy((void *)&(wifi_venc_maxmem[tb.idx]), (void *)ptr, sizeof(HD_VIDEOENC_MAXMEM));
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					memcpy((void *)&(ethcam_venc_maxmem[tb.idx]), (void *)ptr, sizeof(HD_VIDEOENC_MAXMEM));
					ret = E_OK;
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_CODEC_MAXMEM is not support in linktype[%d]\r\n", tb.link_type);
					ret = E_NOSPT;
				}
				if (ret == E_OK) {
					ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_CODEC_MAXMEM]: set id%d=(%d,(%d,%d),%d,%d,%d,%d,%d,%d)\r\n",
											i, ptr->codec_type, ptr->max_dim.w, ptr->max_dim.h, ptr->bitrate, ptr->enc_buf_ms,
											ptr->svc_layer, ptr->ltr, ptr->rotate, ptr->source_output);
				}
			} else {
				DBG_ERR("[MOVIEMULTI_PARAM_CODEC_MAXMEM] NULL param\r\n");
				ret = E_PAR;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_ImgCapGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id, idx = 0;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_IMGCAP_EXIF_EN: {
			// var
			ret = E_OK;
			ImgCapExifFuncEn = value ? TRUE : FALSE;
			imgcap_venc_path_cfg[idx].max_mem.enc_buf_ms = 1600;
			if (ImgCapExifFuncEn) {
				imgcap_venc_path_cfg[idx].max_mem.enc_buf_ms += ((65536 * 2 * 8 * 1000) / imgcap_venc_path_cfg[idx].max_mem.bitrate);
			}
			// venc
			if ((hd_ret = venc_close_ch(ImgCapLink[idx].venc_p[0])) != HD_OK) {
				DBG_ERR("venc_close_ch fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			if ((hd_ret = venc_open_ch(ImgCapLink[idx].venc_i[0], ImgCapLink[idx].venc_o[0], &(ImgCapLink[idx].venc_p[0]))) != HD_OK) {
				DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			HD_VIDEOENC_FUNC_CONFIG imgcap_venc_func_cfg = {
				.ddr_id  = dram_cfg.video_encode,
				.in_func = 0,
			};
			IACOMM_VENC_CFG venc_cfg = {
				.video_enc_path = ImgCapLink[idx].venc_p[0],
				.ppath_cfg      = &(imgcap_venc_path_cfg[idx]),
				.pfunc_cfg      = &(imgcap_venc_func_cfg),
			};
			if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
				DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			IACOMM_VENC_PARAM venc_param = {
				.video_enc_path = ImgCapLink[idx].venc_p[0],
				.pin            = &(imgcap_venc_in_param[idx]),
				.pout           = &(imgcap_venc_out_param[idx]),
				.prc            = NULL,
			};
			if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
				DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_size = {0};
			bs_reserved_size.reserved_size = ImgCapExifFuncEn ? VENC_BS_RESERVED_SIZE_JPG : VENC_BS_RESERVED_SIZE_MP4;
			if ((hd_ret = vendor_videoenc_set(ImgCapLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_size)) != HD_OK) {
				DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE) fail(%d)\n", hd_ret);
				ret = E_SYS;
			}

			if (imgcap_venc_bs_buf_va[idx]) {
				hd_common_mem_munmap((void *)imgcap_venc_bs_buf_va[idx], imgcap_venc_bs_buf[idx].buf_info.buf_size);
				imgcap_venc_bs_buf_va[idx] = 0;
			}
			if ((hd_ret = hd_videoenc_start(ImgCapLink[idx].venc_p[0])) != HD_OK) {
				DBG_ERR("hd_videoenc_start fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			if ((hd_ret = hd_videoenc_get(ImgCapLink[idx].venc_p[0], HD_VIDEOENC_PARAM_BUFINFO, &(imgcap_venc_bs_buf[idx]))) != HD_OK) {
				DBG_ERR("hd_videoenc_get fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			if ((hd_ret = hd_videoenc_stop(ImgCapLink[idx].venc_p[0])) != HD_OK) {
				DBG_ERR("hd_videoenc_stop fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			imgcap_venc_bs_buf_va[idx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, imgcap_venc_bs_buf[idx].buf_info.phy_addr, imgcap_venc_bs_buf[idx].buf_info.buf_size);

			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_THUM_SIZE: {
			if (value) {
				USIZE *sz = (USIZE *)value;
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					if (sz->w >= 160 && sz->h >= 120) {
						img_venc_imgcap_thum_size[tb.idx][tb.tbl].w = sz->w;
						img_venc_imgcap_thum_size[tb.idx][tb.tbl].h = sz->h;
						ret = E_OK;
					}
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					if (sz->w >= 160 && sz->h >= 120) {
						ethcam_venc_imgcap_thum_size[tb.idx].w = sz->w;
						ethcam_venc_imgcap_thum_size[tb.idx].h = sz->h;
						ret = E_OK;
					}
				} else {
					DBG_ERR("[MOVIEMULTI_PARAM_IMGCAP_THUM_SIZE] not support in linktype[%d]\r\n", tb.link_type);
					ret = E_NOSPT;
				}
				if (ret == E_OK) {
					DBG_DUMP("[MOVIEMULTI_PARAM_IMGCAP_THUM_SIZE] set id%d=(%d,%d)\r\n", i, sz->w, sz->h);
				}
			} else {
				DBG_ERR("[MOVIEMULTI_PARAM_IMGCAP_THUM_SIZE]: param is NULL\r\n");
				ret = E_PAR;
			}
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_USE_VENC_BUF: {
			if (_ImageApp_MovieMulti_IsRecording()) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_IMGCAP_USE_VENC_BUF when recording.\r\n");
			} else {
				imgcap_use_venc_buf = value ? TRUE : FALSE;
			}
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE: {
#if defined(_BSP_NA51055_)
			if (value == MOVIE_IMGCAP_SOUT_NONE) {      // not support MOVIE_IMGCAP_SOUT_NONE for na51055
				DBG_ERR("MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE]: MOVIE_IMGCAP_SOUT_NONE is not supported for na51055\r\n");
				ret = E_NOSPT;
				break;
			}
#endif
			if (_ImageApp_MovieMulti_IsRecording()) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE when recording.\r\n");
			} else {
				if (value < MOVIE_IMGCAP_SOUT_MAX) {
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						img_venc_sout_type[tb.idx][tb.tbl] = value;
						ret = E_OK;
					} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
						ethcam_venc_sout_type[tb.idx] = value;
						ret = E_OK;
					}
				} else {
					DBG_ERR("The value of MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE is out of range (%d)\r\n", value);
				}
				if (ret == E_OK) {
					ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE] set id%d=%d\r\n", i, value);
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_YUVSRC: {
			if (_ImageApp_MovieMulti_IsRecording()) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_IMGCAP_YUVSRC when recording.\r\n");
			} else {
				if (value < MOVIE_IMGCAP_YUCSRC_MAX) {
					img_venc_imgcap_yuvsrc[tb.idx][tb.tbl] = value;
					ret = E_OK;
				} else {
					DBG_ERR("The value of MOVIEMULTI_PARAM_IMGCAP_YUVSRC is out of range (%d)\r\n", value);
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_THUM_WITH_EXIF: {
			imgcap_thum_with_exif = value ? TRUE : FALSE;
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_THUM_AUTO_SCALING_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (value) {
					imgcap_thum_no_auto_scaling[tb.idx][tb.tbl] &= ~0x01;
				} else {
					imgcap_thum_no_auto_scaling[tb.idx][tb.tbl] |= 0x01;
				}
				DBG_DUMP("Set MOVIEMULTI_PARAM_IMGCAP_THUM_AUTO_SCALING_EN to id%d=%d\r\n", i, value);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_EXIF_THUM_AUTO_SCALING_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (value) {
					imgcap_thum_no_auto_scaling[tb.idx][tb.tbl] &= ~0x02;
				} else {
					imgcap_thum_no_auto_scaling[tb.idx][tb.tbl] |= 0x02;
				}
				DBG_DUMP("Set MOVIEMULTI_PARAM_IMGCAP_EXIF_THUM_AUTO_SCALING_EN to id%d=%d\r\n", i, value);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_JPG_QUALITY: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_GPS_BUF_SIZE after ImageApp_MovieMulti Open\r\n");
				ret = E_SYS;
			} else {
				if (value) {
					MOVIEMULTI_JPG_QUALITY *ptr = (MOVIEMULTI_JPG_QUALITY *)value;
					if (ptr->jpg_quality <= 100 && ptr->buf_size) {
						imgcap_jpg_quality.jpg_quality = ptr->jpg_quality;
						imgcap_jpg_quality.buf_size= ptr->buf_size;
						DBG_DUMP("MOVIEMULTI_PARAM_IMGCAP_JPG_QUALITY: set jpg_quality=%d and buf_size=%d\r\n", ptr->jpg_quality, ptr->buf_size);
						ret = E_OK;
					} else {
						DBG_ERR("MOVIEMULTI_PARAM_IMGCAP_JPG_QUALITY: jpg_quality %d should between 1~100 or buf_size = 0\r\n", ptr->jpg_quality);
						ret = E_PAR;
					}
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_IMGCAP_JPG_QUALITY: null parameter\r\n");
					ret = E_PAR;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_SOUT_SHARED_BUF_SIZE: {
			if (IsImgLinkOpened[tb.idx] == TRUE) {
				DBG_ERR("MOVIEMULTI_PARAM_IMGCAP_SOUT_SHARED_BUF_SIZE can only be set before open\r\n");
				ret = E_SYS;
			} else {
				imgcap_venc_shared_sout_size = value;
				DBG_DUMP("[MOVIEMULTI_PARAM_IMGCAP_SOUT_SHARED_BUF_SIZE] set soutshared buf size=0x%08x\r\n", imgcap_venc_shared_sout_size);
				ret = E_OK;
			}
			break;

		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_AudGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;
	UINT32 i = id, idx = 0, j;
	HD_RESULT hd_ret;
	HD_AUDIOCAP_VOLUME audio_cap_volume = {0};
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_AUD_MUTE_ENC: {
			ret = E_OK;
			// var
			aud_mute_enc = (value == TRUE) ? TRUE : FALSE;
			// acap
			if (IsAudCapLinkOpened[idx] == TRUE) {
				if (aud_mute_enc_func == FALSE) {
					audio_cap_volume.volume = (aud_mute_enc == FALSE) ? aud_cap_volume : 0;
					if (aud_acap_by_hdal) {
						if ((hd_ret = hd_audiocap_set(AudCapLink[idx].acap_p_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &audio_cap_volume)) != HD_OK) {
							DBG_ERR("Set HD_AUDIOCAP_PARAM_VOLUME fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						}
						if (AudCapLinkStatus[idx].acap_p[0]) {
							if ((hd_ret = hd_audiocap_start(AudCapLink[idx].acap_p[0])) != HD_OK) {
								DBG_ERR("hd_audiocap_start fail(%d)\r\n", hd_ret);
							}
						}
					} else {
						// TODO
						ret = E_NOSPT;
					}
				}
			}
			break;
		}

	case NOVIEMULTI_PARAM_AUD_ACAP_BY_HDAL: {
#if defined(__FREERTOS)
			DBG_ERR("Param NOVIEMULTI_PARAM_AUD_ACAP_BY_HDAL is not supportted.");
			ret = E_NOSPT;
#else
			if (IsAudCapLinkOpened[idx] == TRUE) {
				DBG_ERR("Param NOVIEMULTI_PARAM_AUD_ACAP_BY_HDAL cannot be changed after open.");
			} else {
				// var
				aud_acap_by_hdal = (value == TRUE) ? TRUE : FALSE;
				ret = E_OK;
			}
#endif
			break;
		}

	case MOVIEMULTI_PARAM_AUD_MUTE_ENC_FUNC_EN: {
			if (IsAudCapLinkOpened[idx] == TRUE) {
				DBG_ERR("Param MOVIEMULTI_PARAM_AUD_MUTE_ENC_FUNC_EN cannot be changed after open.");
			} else {
				// var
				aud_mute_enc_func = (value == TRUE) ? TRUE : FALSE;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_CAP_REG_CB: {
			// var
			aud_cap_cb = (MovieAudCapCb *)value;
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ACAP_VOL: {
			// var
			if (value <= 160) {
				ret = E_OK;
				aud_cap_volume = value;
				// acap
				if (IsAudCapLinkOpened[idx] == TRUE) {
					audio_cap_volume.volume = aud_cap_volume;
					aud_mute_enc = FALSE;
					if (aud_acap_by_hdal) {
						if ((hd_ret = hd_audiocap_set(AudCapLink[idx].acap_p_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &audio_cap_volume)) != HD_OK) {
							DBG_ERR("Set HD_AUDIOCAP_PARAM_VOLUME fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						}
						if (AudCapLinkStatus[idx].acap_p[0]) {
							if ((hd_ret = hd_audiocap_start(AudCapLink[idx].acap_p[0])) != HD_OK) {
								DBG_ERR("hd_audiocap_start fail(%d)\r\n", hd_ret);
								ret = E_SYS;
							}
						}
					} else {
						// TODO
						ret = E_NOSPT;
					}
				}
			} else {
				DBG_ERR("Set MOVIEMULTI_PARAM_AUD_ACAP_VOL to %d is out of range\r\n", value);
				ret = E_PAR;
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ACAP_ALC_EN: {
			aud_cap_alc_en = value ? TRUE : FALSE;
			if (IsAudCapLinkOpened[idx] == TRUE) {
				vendor_audiocap_set(AudCapLink[idx].acap_p_ctrl, VENDOR_AUDIOCAP_ITEM_ALC_ENABLE, (VOID *)&aud_cap_alc_en);
			}
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ENC_REG_CB: {
			// var
			aud_bs_cb[0] = (MovieAudBsCb *)value;
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ACAP_ANR: {
#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
			if (IsAudCapLinkOpened[idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_AUD_ACAP_ANR should set before open\r\n");
				ret = E_SYS;
			} else {
				if (value) {
					HD_AUDIOCAP_ANR *p_anr_param = (HD_AUDIOCAP_ANR *)value;
					memcpy(&(aud_acap_anr_param[idx]), p_anr_param, sizeof(HD_AUDIOCAP_ANR));
					ret = E_OK;
				} else {
					DBG_ERR("[MOVIEMULTI_PARAM_AUD_ACAP_ANR]: param is NULL\r\n");
				}
			}
#else
			DBG_ERR("MOVIEMULTI_PARAM_AUD_ACAP_ANR is not supported in this chip.\r\n");
			ret = E_NOSPT;
#endif
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ACAP_DEFAULT_SETTING: {
			if (IsAudCapLinkOpened[idx] == TRUE) {
				if (AudCapLinkStatus[idx].acap_p[0] == 0) {
					UINT32 default_setting = value;
					if ((hd_ret = vendor_audiocap_set(AudCapLink[idx].acap_p[0], VENDOR_AUDIOCAP_ITEM_DEFAULT_SETTING, (VOID *)&default_setting)) != HD_OK) {
						DBG_ERR("Set VENDOR_AUDIOCAP_ITEM_DEFAULT_SETTING(%d) failed(%d)\r\n", default_setting, hd_ret);
					} else {
						ret = E_OK;
					}
				} else {
					DBG_ERR("Cannot do MOVIEMULTI_PARAM_AUD_ACAP_DEFAULT_SETTING since audcap is running.\r\n");
				}
			} else {
				DBG_ERR("Cannot do MOVIEMULTI_PARAM_AUD_ACAP_DEFAULT_SETTING since AudCapLink is not opened.\r\n");
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ACAP_NOISEGATE_THRESHOLD: {
			if (IsAudCapLinkOpened[idx] == TRUE) {
				if (AudCapLinkStatus[idx].acap_p[0] == 0) {
					UINT32 threshold = value;
					if ((hd_ret = vendor_audiocap_set(AudCapLink[idx].acap_p[0], VENDOR_AUDIOCAP_ITEM_NOISEGATE_THRESHOLD, (VOID *)&threshold)) != HD_OK) {
						DBG_ERR("Set VENDOR_AUDIOCAP_ITEM_NOISEGATE_THRESHOLD(%d) failed(%d)\r\n", threshold, hd_ret);
					} else {
						ret = E_OK;
					}
				} else {
					DBG_ERR("Cannot do MOVIEMULTI_PARAM_AUD_ACAP_NOISEGATE_THRESHOLD since audcap is running.\r\n");
				}
			} else {
				DBG_ERR("Cannot do MOVIEMULTI_PARAM_AUD_ACAP_NOISEGATE_THRESHOLD since AudCapLink is not opened.\r\n");
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ENCBUF_MS: {
			if (IsAudCapLinkOpened[idx] == TRUE) {
				if (AudCapLinkStatus[idx].aenc_p[0] == 0) {
					AudEncBufMs[idx] = value;
					aud_aenc_cfg[idx].max_mem.buf_ms = AudEncBufMs[idx];
					ret = _ImageApp_MovieMulti_AudReopenAEnc(idx);
				} else {
					DBG_ERR("Cannot update MOVIEMULTI_PARAM_AUD_ENCBUF_MS since audcap is running.\r\n");
				}
			} else {
				AudEncBufMs[idx] = value;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ACAP_PREPWR_EN: {
			if (IsAudCapLinkOpened[idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_AUD_ACAP_PREPWR_EN should set before open\r\n");
				ret = E_SYS;
			} else {
				aud_cap_prepwr_en = value ? ENABLE : DISABLE;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_CAPBUF_MS: {
			if (IsAudCapLinkOpened[idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_AUD_CAPBUF_MS should set before open\r\n");
				ret = E_SYS;
			} else {
				AudCapBufMs[idx] = value;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_PAUSE_PUSH: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				for (j = 0; j < 2; j ++) {
					img_bsmux_audio_pause[tb.idx][2 * tb.tbl + j] = value ? TRUE : FALSE;
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				for (j = 0; j < 2; j ++) {
					ethcam_bsmux_audio_pause[tb.idx][j] = value ? TRUE : FALSE;
				}
				ret = E_OK;
			} else {
				DBG_ERR("MOVIEMULTI_PARAM_AUD_PAUSE_PUSH is not support in linktype[%d]\r\n", tb.link_type);
				ret = E_NOSPT;
			}
			if (ret == E_OK) {
				ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_AUD_PAUSE_PUSH] set id%d=%s\r\n", i, (value ? "TRUE" : "FALSE"));
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_CODEC: {
			if (IsAudCapLinkOpened[idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_AUD_CODEC should set before open\r\n");
				ret = E_SYS;
			} else if (value >= _CFG_AUD_CODEC_MAX) {
				DBG_ERR("Set MOVIEMULTI_PARAM_AUD_CODEC %d is out of range\r\n");
				ret = E_SYS;
			} else {
				AudEncCodec[idx] = value;
				ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PARAM_AUD_CODEC] set id%d=%d\r\n", idx, value);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ACAP_AEC: {
#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
			if (IsAudCapLinkOpened[idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_AUD_ACAP_AEC should set before open\r\n");
				ret = E_SYS;
			} else {
				if (value) {
					HD_AUDIOCAP_AEC *p_aec_param = (HD_AUDIOCAP_AEC *)value;
					memcpy(&(aud_acap_aec_param[idx]), p_aec_param, sizeof(HD_AUDIOCAP_AEC));
					ret = E_OK;
				} else {
					DBG_ERR("[MOVIEMULTI_PARAM_AUD_ACAP_AEC]: param is NULL\r\n");
				}
			}
#else
			DBG_ERR("MOVIEMULTI_PARAM_AUD_ACAP_AEC is not supported in this chip.\r\n");
			ret = E_NOSPT;
#endif
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_FileGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id, j, k;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_FILE_FORMAT: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				ImgLinkRecInfo[tb.idx][tb.tbl].file_format = value;
				if (IsAudCapLinkOpened[0] == TRUE) {
					// aenc
					VENDOR_AUDIOENC_BS_RESERVED_SIZE_CFG aud_reserved_size = {0};
					aud_reserved_size.reserved_size = (ImgLinkRecInfo[tb.idx][tb.tbl].file_format == _CFG_FILE_FORMAT_TS) ? AENC_BS_RESERVED_SIZE_TS : AENC_BS_RESERVED_SIZE_MP4;
					if ((hd_ret = vendor_audioenc_set(AudCapLink[0].aenc_p[0], VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE, &aud_reserved_size)) != HD_OK) {
						DBG_ERR("vendor_audioenc_set(VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE) fail(%d)\r\n", hd_ret);
					}
				}
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_size = {0};
					bs_reserved_size.reserved_size = (ImgLinkRecInfo[tb.idx][tb.tbl].file_format == _CFG_FILE_FORMAT_TS) ? VENC_BS_RESERVED_SIZE_TS : VENC_BS_RESERVED_SIZE_MP4;
					if ((hd_ret = vendor_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_size)) != HD_OK) {
						DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE) fail(%d)\n", hd_ret);
					}
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+j].filetype = ImgLinkRecInfo[tb.idx][tb.tbl].file_format;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);

						img_bsmux_ainfo[tb.idx][2*tb.tbl+j].adts_bytes = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_AAC) ? 7 : 0;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_AUDIOINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_ainfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				EthCamLinkRecInfo[tb.idx].rec_format = value;
				if (IsAudCapLinkOpened[0] == TRUE) {
					// aenc
					VENDOR_AUDIOENC_BS_RESERVED_SIZE_CFG aud_reserved_size = {0};
					aud_reserved_size.reserved_size = (EthCamLinkRecInfo[tb.idx].rec_format == _CFG_FILE_FORMAT_TS) ? AENC_BS_RESERVED_SIZE_TS : AENC_BS_RESERVED_SIZE_MP4;
					if ((hd_ret = vendor_audioenc_set(AudCapLink[0].aenc_p[0], VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE, &aud_reserved_size)) != HD_OK) {
						DBG_ERR("vendor_audioenc_set(VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE) fail(%d)\r\n", hd_ret);
					}
				}
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_size = {0};
					bs_reserved_size.reserved_size = (EthCamLinkRecInfo[tb.idx].rec_format == _CFG_FILE_FORMAT_TS) ? VENC_BS_RESERVED_SIZE_TS : VENC_BS_RESERVED_SIZE_MP4;
					if ((hd_ret = vendor_videoenc_set(EthCamLink[tb.idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_size)) != HD_OK) {
						DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE) fail(%d)\n", hd_ret);
					}
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						ethcam_bsmux_finfo[tb.idx][j].filetype = EthCamLinkRecInfo[tb.idx].rec_format;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);

						ethcam_bsmux_ainfo[tb.idx][j].adts_bytes = (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_AAC) ? 7 : 0;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_AUDIOINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_ainfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_REC_FORMAT: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode = value;
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+j].recformat = ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);

						img_bsmux_ainfo[tb.idx][2*tb.tbl+j].aud_en = (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_AV) ? TRUE : FALSE;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_AUDIOINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_ainfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				EthCamLinkRecInfo[tb.idx].rec_mode = value;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						ethcam_bsmux_finfo[tb.idx][j].recformat = EthCamLinkRecInfo[tb.idx].rec_mode;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);

						ethcam_bsmux_ainfo[tb.idx][j].aud_en = (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_AV) ? TRUE : FALSE;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_AUDIOINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_ainfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_SEAMLESSSEC: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				ImgLinkFileInfo.seamless_sec = value;
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+j].seamlessSec = ImgLinkFileInfo.seamless_sec;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				ImgLinkFileInfo.seamless_sec = value;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						ethcam_bsmux_finfo[tb.idx][j].seamlessSec = ImgLinkFileInfo.seamless_sec;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_ENDTYPE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				ImgLinkFileInfo.end_type = value;
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+j].endtype = ImgLinkFileInfo.end_type;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);
					}
					if (iamovie_use_filedb_and_naming == TRUE) {
						INT32 is_loop_del = (value == MOVREC_ENDTYPE_CUTOVERLAP) ? TRUE : FALSE;
						if (iamoviemulti_fm_set_loop_del('A', is_loop_del) != 0) {
							DBG_ERR("set loop del failed, drive %c\r\n", 'A');
						}
						if (iamoviemulti_fm_set_loop_del('B', is_loop_del) != 0) {
							DBG_ERR("set loop del failed, drive %c\r\n", 'B');
						}
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				ImgLinkFileInfo.end_type = value;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						ethcam_bsmux_finfo[tb.idx][j].endtype = ImgLinkFileInfo.end_type;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);
					}
					if (iamovie_use_filedb_and_naming == TRUE) {
						INT32 is_loop_del = (value == MOVREC_ENDTYPE_CUTOVERLAP) ? TRUE : FALSE;
						if (iamoviemulti_fm_set_loop_del('A', is_loop_del) != 0) {
							DBG_ERR("set loop del failed, drive %c\r\n", 'A');
						}
						if (iamoviemulti_fm_set_loop_del('B', is_loop_del) != 0) {
							DBG_ERR("set loop del failed, drive %c\r\n", 'B');
						}
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_PLAYFRAMERATE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				ImgLinkTimelapsePlayRate = value;
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+j].playvfr = ImgLinkTimelapsePlayRate;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				ImgLinkTimelapsePlayRate = value;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						ethcam_bsmux_finfo[tb.idx][j].playvfr = ImgLinkTimelapsePlayRate;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_BUFRESSEC:
	case MOVIEMULTI_PARAM_FILE_BUFRESSEC_EMR: {
			UINT32 loop_start = (param == MOVIEMULTI_PARAM_FILE_BUFRESSEC) ? 0 : 1;
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				for (j = loop_start; j < 2; j ++) {
					img_bsmux_bufsec[tb.idx][2*tb.tbl+j] = value;
				}
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = loop_start; j < 2; j ++) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+j].revsec = img_bsmux_bufsec[tb.idx][2*tb.tbl+j];
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				for (j = loop_start; j < 2; j ++) {
					ethcam_bsmux_bufsec[tb.idx][j] = value;
				}
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = loop_start; j < 2; j ++) {
						ethcam_bsmux_finfo[tb.idx][j].revsec = ethcam_bsmux_bufsec[tb.idx][j];
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_ROLLBACKSEC: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				if (tb.tbl == MOVIETYPE_MAIN) {
					ImgLinkFileInfo.emr_sec = value;
				} else {
					ImgLinkFileInfo.clone_emr_sec = value;
				}
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+j].rollbacksec = (tb.tbl == MOVIETYPE_MAIN) ? ImgLinkFileInfo.emr_sec : ImgLinkFileInfo.clone_emr_sec;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				ImgLinkFileInfo.emr_sec = value;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						ethcam_bsmux_finfo[tb.idx][j].rollbacksec = ImgLinkFileInfo.emr_sec;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_KEEPSEC: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				// var
				if (tb.tbl == MOVIETYPE_MAIN) {
					ImgLinkFileInfo.keep_sec = value;
				} else {
					ImgLinkFileInfo.clone_keep_sec = value;
				}
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+j].keepsec = (tb.tbl == MOVIETYPE_MAIN) ? ImgLinkFileInfo.keep_sec : ImgLinkFileInfo.clone_keep_sec;
						bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				// var
				ImgLinkFileInfo.keep_sec = value;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// bsmux
					IACOMM_BSMUX_PARAM bsmux_param;
					for (j = 0; j < 2; j ++) {
						ethcam_bsmux_finfo[tb.idx][j].keepsec = ImgLinkFileInfo.keep_sec;
						bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
						bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
						bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
						bsmux_set_param(&bsmux_param);
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_EMRON: {
			UINT32 emr_time = 0;
			HD_BSMUX_EN_UTIL util = {0};
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (ImageApp_MovieMulti_IsStreamRunning(i)) {
					DBG_ERR("Cannot change EMR mode when recording!\r\n");
					break;
				}
				// var
				ImgLinkFileInfo.emr_on = value;
				if (_ImageApp_MovieMulti_CheckEMRMode() == E_SYS) {
					DBG_WRN("EMR mode conflicted(%x). Forced set to EMR_OFF\r\n", ImgLinkFileInfo.emr_on);
					ImgLinkFileInfo.emr_on = _CFG_EMR_OFF;
				}
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					if (tb.tbl == MOVIETYPE_MAIN) {
						img_venc_path_cfg[tb.idx][tb.tbl].max_mem.enc_buf_ms = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? (ImgLinkVdoEncBufMs[tb.idx][tb.tbl] + ImgLinkFileInfo.emr_sec * 1000) : ImgLinkVdoEncBufMs[tb.idx][tb.tbl];
					} else {
						img_venc_path_cfg[tb.idx][tb.tbl].max_mem.enc_buf_ms = (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) ? (ImgLinkVdoEncBufMs[tb.idx][tb.tbl] + ImgLinkFileInfo.clone_emr_sec * 1000) : ImgLinkVdoEncBufMs[tb.idx][tb.tbl];
					}
					// venc
					_ImageApp_MovieMulti_ImgReopenVEnc(tb.idx, tb.tbl);
					// check aenc buffer
					if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {
						emr_time = ImgLinkFileInfo.emr_sec;
					}
					if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {
						if (emr_time < ImgLinkFileInfo.clone_emr_sec) {
							emr_time = ImgLinkFileInfo.clone_emr_sec;
						}
					}
					emr_time = (emr_time + 3) * 1000;       // change sec to msec
					if (emr_time > AudEncBufMs[0]) {
						DBG_ERR("reserved aenc buf_ms is not enough! reserved %dms but recommended %dms\r\n", AudEncBufMs[0], emr_time);
					}
					// bsmux
					if (tb.tbl == MOVIETYPE_MAIN) {
						img_bsmux_finfo[tb.idx][2*tb.tbl+1].emron = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? TRUE : FALSE;
						img_bsmux_finfo[tb.idx][2*tb.tbl+1].emrloop = (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) ? TRUE : FALSE;
					} else {
						img_bsmux_finfo[tb.idx][2*tb.tbl+1].emron = (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) ? TRUE : FALSE;
						img_bsmux_finfo[tb.idx][2*tb.tbl+1].emrloop = (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) ? TRUE : FALSE;
					}
					IACOMM_BSMUX_PARAM bsmux_param = {
						.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+1],
						.id      = HD_BSMUX_PARAM_FILEINFO,
						.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+1]),
					};
					bsmux_set_param(&bsmux_param);

					util.type = HD_BSMUX_EN_UTIL_DUR_LIMIT;
					util.enable = TRUE;
					if (tb.tbl == MOVIETYPE_MAIN) {         // main
						util.resv[0] = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? ((ImgLinkFileInfo.emr_sec * 1000000) + IAMOVIE_TIMESTAMP_WARNING_TIME) : 0;
					} else {                                // clone
						util.resv[0] = (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) ? ((ImgLinkFileInfo.clone_emr_sec * 1000000) + IAMOVIE_TIMESTAMP_WARNING_TIME) : 0;
					}
					for (j = 0; j < 2; j++) {
						hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2*tb.tbl+j], HD_BSMUX_PARAM_EN_UTIL, &util);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (ImageApp_MovieMulti_IsStreamRunning(i)) {
					DBG_ERR("Cannot change EMR mode when recording!\r\n");
					break;
				}
				// var
				ImgLinkFileInfo.emr_on = value;
				if (_ImageApp_MovieMulti_CheckEMRMode() == E_SYS) {
					DBG_WRN("EMR mode conflicted(%x). Forced set to EMR_OFF\r\n", ImgLinkFileInfo.emr_on);
					ImgLinkFileInfo.emr_on = _CFG_EMR_OFF;
				}
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// check aenc buffer
					if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {
						emr_time = ImgLinkFileInfo.emr_sec;
					}
					if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {
						if (emr_time < ImgLinkFileInfo.clone_emr_sec) {
							emr_time = ImgLinkFileInfo.clone_emr_sec;
						}
					}
					emr_time = (emr_time + 3) * 1000;       // change sec to msec
					if (emr_time > AudEncBufMs[0]) {
						DBG_ERR("reserved aenc buf_ms is not enough! reserved %dms but recommended %dms\r\n", AudEncBufMs[0], emr_time);
					}
					// bsmux
					ethcam_bsmux_finfo[tb.idx][1].emron = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? TRUE : FALSE;
					ethcam_bsmux_finfo[tb.idx][1].emrloop = (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) ? TRUE : FALSE;
					IACOMM_BSMUX_PARAM bsmux_param = {
						.path_id = EthCamLink[tb.idx].bsmux_p[1],
						.id      = HD_BSMUX_PARAM_FILEINFO,
						.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][1]),
					};
					bsmux_set_param(&bsmux_param);

					util.type = HD_BSMUX_EN_UTIL_DUR_LIMIT;
					util.enable = TRUE;
					util.resv[0] = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? ((ImgLinkFileInfo.emr_sec * 1000000) + IAMOVIE_TIMESTAMP_WARNING_TIME) : 0;
					for (j = 0; j < 2; j++) {
						hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[j], HD_BSMUX_PARAM_EN_UTIL, &util);
					}
				}
				ret = E_OK;
			}
			break;
		}

		case MOVIEMULTI_PARAM_FILE_FLUSH_SEC:
		case MOVIEMULTI_PARAM_FILE_WRITE_BLKSIZE: {
			HD_BSMUX_WRINFO wrinfo = {0};
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					if ((hd_ret = hd_bsmux_get(ImgLink[tb.idx].bsmux_p[2*tb.tbl], HD_BSMUX_PARAM_WRINFO, (void*)&wrinfo)) != HD_OK) {
						DBG_ERR("hd_bsmux_get(HD_BSMUX_PARAM_WRINFO) fail(%d)\n", hd_ret);
						break;
					}
					if (param == MOVIEMULTI_PARAM_FILE_FLUSH_SEC) {
						wrinfo.flush_freq = value;
					} else {
						wrinfo.wrblk_size = value;
					}
					for (j = 0; j < 2; j++) {
						if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2*tb.tbl+j], HD_BSMUX_PARAM_WRINFO, (void*)&wrinfo)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_WRINFO) fail(%d)\n", hd_ret);
						}
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					if ((hd_ret = hd_bsmux_get(EthCamLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_WRINFO, (void*)&wrinfo)) != HD_OK) {
						DBG_ERR("hd_bsmux_get(HD_BSMUX_PARAM_WRINFO) fail(%d)\n", hd_ret);
						break;
					}
					if (param == MOVIEMULTI_PARAM_FILE_FLUSH_SEC) {
						wrinfo.flush_freq = value;
					} else {
						wrinfo.wrblk_size = value;
					}
					for (j = 0; j < 2; j++) {
						if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[j], HD_BSMUX_PARAM_WRINFO, (void*)&wrinfo)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_WRINFO) fail(%d)\n", hd_ret);
						}
					}
				}
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_GPS_DATA: {
			if (value) {
				UINT32 *p_gps_pa = NULL, *p_gps_va = NULL, *p_gps_size = NULL, *p_gps_offset = NULL;
				if (i == _CFG_CTRL_ID) {
					p_gps_pa = &gpsdata_pa;
					p_gps_va = &gpsdata_va;
					p_gps_size = &gpsdata_size;
					p_gps_offset = &gpsdata_offset;
				} else if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					p_gps_pa = &(gpsdata_rec_pa[tb.idx][tb.tbl]);
					p_gps_va = &(gpsdata_rec_va[tb.idx][tb.tbl]);
					p_gps_size = &(gpsdata_rec_size[tb.idx][tb.tbl]);
					p_gps_offset = &(gpsdata_rec_offset[tb.idx][tb.tbl]);
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					p_gps_pa = &gpsdata_eth_pa[tb.idx][0];
					p_gps_va = &gpsdata_eth_va[tb.idx][0];
					p_gps_size = &gpsdata_eth_size[tb.idx][0];
					p_gps_offset = &gpsdata_eth_offset[tb.idx][0];
				} else {
					ret = E_NOSPT;
					break;
				}

				if (*p_gps_va == 0) {
					UINT32 pa;
					void *va;
					HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

					*p_gps_size = iamovie_gps_buffer_size;
					if ((hd_ret = hd_common_mem_alloc("gpsdata", &pa, (void **)&va, *p_gps_size, ddr_id)) != HD_OK) {
						DBG_ERR("MOVIEMULTI_PARAM_FILE_GPS_DATA: hd_common_mem_alloc failed(%d)\r\n", hd_ret);
						*p_gps_va = 0;
						*p_gps_pa = 0;
						*p_gps_size = 0;
						*p_gps_offset = 0;
						ret = E_SYS;
						break;
					} else {
						*p_gps_va = (UINT32)va;
						*p_gps_pa = (UINT32)pa;
						*p_gps_offset = GPS_DATA_FRONT_RESERVED;
						DBG_DUMP("MOVIEMULTI_PARAM_FILE_GPS_DATA: allocate gps buffer pa=0x%08x va=0x%08x size=0x%08x\r\n", *p_gps_pa, *p_gps_va, *p_gps_size);
					}
				}

				MEM_RANGE *p_gpsdata = (MEM_RANGE *)value;
				UINT32 gps_size = (p_gpsdata->size > (iamovie_gps_buffer_size - GPS_DATA_FRONT_RESERVED))? (iamovie_gps_buffer_size - GPS_DATA_FRONT_RESERVED) : p_gpsdata->size;
				if ((*p_gps_offset + gps_size) > iamovie_gps_buffer_size) {
					*p_gps_offset = GPS_DATA_FRONT_RESERVED;
				}
				memcpy((void *)((*p_gps_va) + (*p_gps_offset)), (void *)p_gpsdata->addr, gps_size);
				hd_common_mem_flush_cache((VOID *)((*p_gps_va)+ (*p_gps_offset)), ALIGN_CEIL_4(gps_size));

				HD_BSMUX_PUT_DATA gps;
				gps.phy_addr = (*p_gps_pa) + (*p_gps_offset);
				gps.vir_addr = (*p_gps_va) + (*p_gps_offset);
				gps.size     = gps_size;
				gps.type     = HD_BSMUX_PUT_DATA_TYPE_GPS;

				*p_gps_offset += ALIGN_CEIL_4(gps_size + GPS_DATA_FRONT_RESERVED);
				if (*p_gps_offset >= iamovie_gps_buffer_size) {
					*p_gps_offset = GPS_DATA_FRONT_RESERVED;
				}

				ret = E_OK;
				if (i == _CFG_CTRL_ID) {
					for (j = 0; j < MaxLinkInfo.MaxImgLink; j ++) {
						//for (k = 0; k < 4; k++) {
						for (k = 0; k < (img_bsmux_2v1a_mode[j] ? 2 : 4); k++) {
							if (_LinkCheckStatus(ImgLinkStatus[j].bsmux_p[k])) {
								if ((hd_ret = hd_bsmux_set(ImgLink[j].bsmux_p[k], HD_BSMUX_PARAM_PUT_DATA, (void*)&gps)) != HD_OK) {
									DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_PUT_DATA) GPS data fail(%d)\n", hd_ret);
									ret = E_SYS;
								}
							}
						}
					}
					for (j = 0; j < MaxLinkInfo.MaxEthCamLink; j ++) {
						for (k = 0; k < 2; k++) {
							if (_LinkCheckStatus(EthCamLinkStatus[j].bsmux_p[k])) {
								if ((hd_ret = hd_bsmux_set(EthCamLink[j].bsmux_p[k], HD_BSMUX_PARAM_PUT_DATA, (void*)&gps)) != HD_OK) {
									DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_PUT_DATA) GPS data fail(%d)\n", hd_ret);
									ret = E_SYS;
								}
							}
						}
					}
				} else if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					for (k = 0; k < 2; k++) {
						UINT32 path = 2 * tb.tbl + k;
						if (_LinkCheckStatus(ImgLinkStatus[tb.idx].bsmux_p[path])) {
							if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_PUT_DATA, (void*)&gps)) != HD_OK) {
								DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_PUT_DATA) GPS data fail(%d)\n", hd_ret);
								ret = E_SYS;
							}
						}
					}
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					for (k = 0; k < 2; k++) {
						UINT32 path = k;
						if (_LinkCheckStatus(EthCamLinkStatus[tb.idx].bsmux_p[path])) {
							if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_PUT_DATA, (void*)&gps)) != HD_OK) {
								DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_PUT_DATA) GPS data fail(%d)\n", hd_ret);
								ret = E_SYS;
							}
						}
					}
				}
			} else {
				DBG_ERR("ImageApp_MovieMulti_SetParam(MOVIEMULTI_PARAM_FILE_GPS_DATA) fail. GPS_DATA is NULL.\r\n");
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_FRONT_MOOV:
	case MOVIEMULTI_PARAM_FILE_FRONT_MOOV_FLUSH_SEC: {
			HD_BSMUX_EN_UTIL util = {0};
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				for (j = 0; j < 2; j ++) {
					if (param == MOVIEMULTI_PARAM_FILE_FRONT_MOOV) {
						img_bsmux_front_moov_en[tb.idx][2*tb.tbl+j] = value ? ENABLE : DISABLE;
					} else {
						img_bsmux_front_moov_flush_sec[tb.idx][2*tb.tbl+j] = value;
					}
					util.type = HD_BSMUX_EN_UTIL_FRONTMOOV;
					util.enable = img_bsmux_front_moov_en[tb.idx][2*tb.tbl+j];
					util.resv[0] = img_bsmux_front_moov_flush_sec[tb.idx][2*tb.tbl+j];
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2*tb.tbl+j], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
					} else {
						ret = E_OK;
					}
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				for (j = 0; j < 2; j ++) {
					if (param == MOVIEMULTI_PARAM_FILE_FRONT_MOOV) {
						ethcam_bsmux_front_moov_en[tb.idx][j] = value ? ENABLE : DISABLE;
					} else {
						ethcam_bsmux_front_moov_flush_sec[tb.idx][j] = value;
					}
					util.type = HD_BSMUX_EN_UTIL_FRONTMOOV;
					util.enable = ethcam_bsmux_front_moov_en[tb.idx][j];
					util.resv[0] = ethcam_bsmux_front_moov_flush_sec[tb.idx][j];
					if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[j], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
					} else {
						ret = E_OK;
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_2V1A_MODE: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_2V1A_MODE after ImageApp_MovieMulti Open\r\n");
			} else if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				img_bsmux_2v1a_mode[tb.idx] = (value) ? 1 : 0;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_BSMUX_FAST_PUT: {
			HD_BSMUX_EN_UTIL util = {0};
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				for (j = 0; j < 2; j ++) {
					util.type = HD_BSMUX_EN_UTIL_FAST_PUT;
					util.enable = value ? TRUE : FALSE;
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2*tb.tbl+j], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
					} else {
						ret = E_OK;
					}
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				for (j = 0; j < 2; j ++) {
					util.type = HD_BSMUX_EN_UTIL_FAST_PUT;
					util.enable = value ? TRUE : FALSE;
					if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[j], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
					} else {
						ret = E_OK;
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_DROP_FRAME_WITHOUT_SLOW_CB:
	case MOVIEMULTI_PARAM_FILE_RECOVERY_FUNC_EN:
	case MOVIEMULTI_PARAM_FILE_BITSTREAM_ALIGN_EN:
	case MOVIEMULTI_PARAM_FILE_FREA_EN:
	case MOVIEMULTI_PARAM_FILE_UTC_AUTO_EN:
	case MOVIEMULTI_PARAM_FILE_CO64_EN: {
			HD_PATH_ID bsmux_ctrl_path = 0;
			HD_BSMUX_EN_UTIL util = {0};

			if (param == MOVIEMULTI_PARAM_FILE_DROP_FRAME_WITHOUT_SLOW_CB) {
				util.type = HD_BSMUX_EN_UTIL_EN_DROP;
				util.enable = value ? TRUE : FALSE;
			} else if (param == MOVIEMULTI_PARAM_FILE_RECOVERY_FUNC_EN) {
				util.type = HD_BSMUX_EN_UTIL_RECOVERY;
				util.enable = value ? TRUE : FALSE;
			} else if (param == MOVIEMULTI_PARAM_FILE_BITSTREAM_ALIGN_EN) {
				util.type = HD_BSMUX_EN_UTIL_MUXALIGN;
				util.enable = value ? TRUE : FALSE;
			} else if (param == MOVIEMULTI_PARAM_FILE_FREA_EN) {
				util.type = HD_BSMUX_EN_UTIL_FREA_BOX;
				util.enable = value ? TRUE : FALSE;
			} else if (param == MOVIEMULTI_PARAM_FILE_UTC_AUTO_EN) {
				util.type = HD_BSMUX_EN_UTIL_UTC_AUTO;
				util.enable = value ? TRUE : FALSE;
			} else if (param == MOVIEMULTI_PARAM_FILE_CO64_EN) {
				util.type = HD_BSMUX_EN_UTIL_BTAG_SIZE;
				util.enable = TRUE;
				util.resv[0] = value ? 64 : 32;
			}

			if ((hd_ret = bsmux_get_ctrl_path(0, &bsmux_ctrl_path)) != HD_OK) {
				DBG_ERR("bsmux_get_ctrl_path() fail(%d)\n", hd_ret);
			} else {
				if ((hd_ret = hd_bsmux_set(bsmux_ctrl_path, HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
					DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL,type=%x) fail(%d)\n", util.type, hd_ret);
				} else {
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_RO_DEL_TYPE: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				if (value == RO_DEL_TYPE_PERCENT) {
					iamoviemulti_fm_set_rofile_info('A', IAMOVIEMULTI_FM_CHK_PCT, IAMOVIEMULTI_FM_ROINFO_TYPE);
					iamoviemulti_fm_set_rofile_info('B', IAMOVIEMULTI_FM_CHK_PCT, IAMOVIEMULTI_FM_ROINFO_TYPE);
				} else if (value == RO_DEL_TYPE_NUM) {
					iamoviemulti_fm_set_rofile_info('A', IAMOVIEMULTI_FM_CHK_NUM, IAMOVIEMULTI_FM_ROINFO_TYPE);
					iamoviemulti_fm_set_rofile_info('B', IAMOVIEMULTI_FM_CHK_NUM, IAMOVIEMULTI_FM_ROINFO_TYPE);
					ret = E_OK;
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_RO_DEL_TYPE out of range %d\r\n", value);
				}
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_RO_DEL_PERCENT: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				if (value <= 100) {
					iamoviemulti_fm_set_rofile_info('A', value, IAMOVIEMULTI_FM_ROINFO_PCT);
					iamoviemulti_fm_set_rofile_info('B', value, IAMOVIEMULTI_FM_ROINFO_PCT);
					ret = E_OK;
				} else {
					DBG_ERR("MOVIEMULTI_PARAM_RO_DEL_PERCENT out of range %d\r\n", value);
				}
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_RO_DEL_NUM: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				iamoviemulti_fm_set_rofile_info('A', value, IAMOVIEMULTI_FM_ROINFO_NUM);
				iamoviemulti_fm_set_rofile_info('B', value, IAMOVIEMULTI_FM_ROINFO_NUM);
				ret = E_OK;
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_GPS_EN:
	case MOVIEMULTI_PARAM_FILE_GPS_RATE: {
			HD_BSMUX_EN_UTIL util = {0};
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				ret = E_OK;
				if (param == MOVIEMULTI_PARAM_FILE_GPS_EN) {
					gpsdata_rec_disable[tb.idx][tb.tbl] = value ? FALSE : TRUE;
				} else {
					gpsdata_rec_rate[tb.idx][tb.tbl] = value;
				}
				if (IsMovieMultiOpened == TRUE) {
					for (j = 0; j < 2; j ++) {
						UINT32 path = 2 * tb.tbl + j;
						util.type = HD_BSMUX_EN_UTIL_GPS_DATA;
						util.enable = gpsdata_rec_disable[tb.idx][tb.tbl] ? FALSE : TRUE;
						util.resv[0] = gpsdata_rec_rate[tb.idx][tb.tbl] ? gpsdata_rec_rate[tb.idx][tb.tbl] : 1;
						util.resv[1] = TRUE;        // gps queue enable
						if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ret = E_OK;
				gpsdata_eth_disable[tb.idx][0] = value ? FALSE : TRUE;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					for (j = 0; j < 2; j ++) {
						UINT32 path = j;
						util.type = HD_BSMUX_EN_UTIL_GPS_DATA;
						util.enable = gpsdata_eth_disable[tb.idx][0] ? FALSE : TRUE;
						util.resv[0] = gpsdata_eth_rate[tb.idx][0] ? gpsdata_eth_rate[tb.idx][0] : 1;
						util.resv[1] = TRUE;        // gps queue enable
						if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_MAX_POP_SIZE:
	case MOVIEMULTI_PARAM_FILE_MAX_FILE_SIZE: {
			HD_FILEOUT_CONFIG fout_cfg = {0};
			HD_PATH_ID fileout_ctrl = 0;
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				ret = E_OK;
				if (IsMovieMultiOpened == TRUE) {
					if ((hd_ret = hd_fileout_open(0, HD_FILEOUT_CTRL(0), &fileout_ctrl)) != HD_OK) {
						DBG_ERR("hd_fileout_open(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_fileout_get(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
						DBG_ERR("hd_fileout_get(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
					} else {
						DBG_DUMP("drive(%x) max_pop_size(0x%x)\r\n", (unsigned int)fout_cfg.drive, (unsigned int)fout_cfg.max_pop_size);
					}
					if (param == MOVIEMULTI_PARAM_FILE_MAX_POP_SIZE) {
						fout_cfg.max_pop_size = (UINT64)value;
					} else {
						fout_cfg.max_file_size = (UINT64)value;
					}
					if ((hd_ret = hd_fileout_set(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
						DBG_ERR("hd_fileout_set(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_fileout_close(fileout_ctrl)) != HD_OK) {
						DBG_ERR("hd_fileout_close(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
					}
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ret = E_OK;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					if ((hd_ret = hd_fileout_open(0, HD_FILEOUT_CTRL(0), &fileout_ctrl)) != HD_OK) {
						DBG_ERR("hd_fileout_open(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_fileout_get(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
						DBG_ERR("hd_fileout_get(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
					} else {
						DBG_DUMP("drive(%x) max_pop_size(0x%x)\r\n", (unsigned int)fout_cfg.drive, (unsigned int)fout_cfg.max_pop_size);
					}
					if (param == MOVIEMULTI_PARAM_FILE_MAX_POP_SIZE) {
						fout_cfg.max_pop_size = (UINT64)value;
					} else {
						fout_cfg.max_file_size = (UINT64)value;
					}
					if ((hd_ret = hd_fileout_set(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
						DBG_ERR("hd_fileout_set(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_fileout_close(fileout_ctrl)) != HD_OK) {
						DBG_ERR("hd_fileout_close(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
					}
				}
			} else {
				ret = E_NOSPT;
			}
			if (ret == E_OK) {
				if (param == MOVIEMULTI_PARAM_FILE_MAX_POP_SIZE) {
					ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_FILE_MAX_POP_SIZE to %llx\r\n", fout_cfg.max_pop_size);
				} else {
					ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_FILE_MAX_FILE_SIZE to %llx\r\n", fout_cfg.max_file_size);
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_EMR_BSQ_INCARD: {
			HD_BSMUX_EN_UTIL bsmux_util = {0};
			HD_PATH_ID bsmux_ctrl = 0;
			HD_FILEOUT_CONFIG fout_cfg = {0};
			HD_PATH_ID fileout_ctrl = 0;
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				ret = E_OK;
				if (IsMovieMultiOpened == TRUE) {
					// BsMux
					if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(0), &bsmux_ctrl)) != HD_OK) {
						DBG_ERR("hd_bsmux_open(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
					}
					bsmux_util.type = HD_BSMUX_EN_UTIL_STRG_BUF;
					bsmux_util.enable = (value ? 1 : 0);
					if ((hd_ret = hd_bsmux_set(bsmux_ctrl, HD_BSMUX_PARAM_EN_UTIL, &bsmux_util)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_bsmux_close(bsmux_ctrl)) != HD_OK) {
						DBG_ERR("hd_bsmux_close(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
					}
					// FileOut
					if ((hd_ret = hd_fileout_open(0, HD_FILEOUT_CTRL(0), &fileout_ctrl)) != HD_OK) {
						DBG_ERR("hd_fileout_open(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_fileout_get(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
						DBG_ERR("hd_fileout_get(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
					} else {
						DBG_DUMP("drive(%x) use_mem_blk(0x%x)\r\n", (unsigned int)fout_cfg.drive, (unsigned int)fout_cfg.use_mem_blk);
					}
					fout_cfg.use_mem_blk = (value ? 1 : 0);
					if ((hd_ret = hd_fileout_set(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
						DBG_ERR("hd_fileout_set(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_fileout_close(fileout_ctrl)) != HD_OK) {
						DBG_ERR("hd_fileout_close(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
					}
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ret = E_OK;
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					// BsMux
					if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(0), &bsmux_ctrl)) != HD_OK) {
						DBG_ERR("hd_bsmux_open(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
					}
					bsmux_util.type = HD_BSMUX_EN_UTIL_STRG_BUF;
					bsmux_util.enable = (value ? 1 : 0);
					if ((hd_ret = hd_bsmux_set(bsmux_ctrl, HD_BSMUX_PARAM_EN_UTIL, &bsmux_util)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_bsmux_close(bsmux_ctrl)) != HD_OK) {
						DBG_ERR("hd_bsmux_close(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
					}
					// FileOut
					if ((hd_ret = hd_fileout_open(0, HD_FILEOUT_CTRL(0), &fileout_ctrl)) != HD_OK) {
						DBG_ERR("hd_fileout_open(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_fileout_get(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
						DBG_ERR("hd_fileout_get(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
					} else {
						DBG_DUMP("drive(%x) use_mem_blk(0x%x)\r\n", (unsigned int)fout_cfg.drive, (unsigned int)fout_cfg.use_mem_blk);
					}
					fout_cfg.use_mem_blk = (value ? 1 : 0);
					if ((hd_ret = hd_fileout_set(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
						DBG_ERR("hd_fileout_set(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
					}
					if ((hd_ret = hd_fileout_close(fileout_ctrl)) != HD_OK) {
						DBG_ERR("hd_fileout_close(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
					}
				}
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_GPS_BUF_SIZE: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_GPS_BUF_SIZE after ImageApp_MovieMulti Open\r\n");
				ret = E_SYS;
			} else {
				if (value < GPS_DATA_FRONT_RESERVED) {
					iamovie_gps_buffer_size = GPS_DATA_FRONT_RESERVED;
				} else {
					iamovie_gps_buffer_size = value;
				}
				DBG_DUMP("MOVIEMULTI_PARAM_FILE_GPS_BUF_SIZE: set buffer size to %d bytes\r\n", iamovie_gps_buffer_size);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_CUST_TAG: {
			ret = E_OK;
			if (value == 0) {
				DBG_ERR("MOVIEMULTI_PARAM_FILE_CUST_TAG is set with NULL value\r\n");
				ret = E_PAR;
			} else {
				if (IsMovieMultiOpened == FALSE) {
					DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_GPS_BUF_SIZE before ImageApp_MovieMulti Open\r\n");
					ret = E_SYS;
				} else {
					MOVIEMULTI_MOV_CUSTOM_TAG *ptag = (MOVIEMULTI_MOV_CUSTOM_TAG *) value;
					IACOMM_BSMUX_PARAM bsmux_param = {0};
					HD_BSMUX_USER_DATA user_data   = {0};
					HD_BSMUX_CUST_DATA cust_data   = {0};
					if (ptag->tag == MAKEFOURCC('u', 'd', 't', 'a')) {
						user_data.useron       = TRUE;
						user_data.userdataadr  = ptag->addr;
						user_data.userdatasize = ptag->size;
						bsmux_param.id         = HD_BSMUX_PARAM_USER_DATA;
						bsmux_param.p_param    = (void*)&user_data;
					} else {
						cust_data.custon       = TRUE;
						cust_data.custtag      = ((ptag->tag & 0x000000ff) << 24) | ((ptag->tag & 0x0000ff00) << 8) | ((ptag->tag & 0x00ff0000) >> 8) | ((ptag->tag & 0xff000000) >> 24);
						cust_data.custaddr     = ptag->addr;
						cust_data.custsize     = ptag->size;
						bsmux_param.id         = HD_BSMUX_PARAM_CUST_DATA;
						bsmux_param.p_param    = (void*)&cust_data;
					}
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						for (j = 0; j < 2; j ++) {
							bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+j];
							if ((hd_ret = bsmux_set_param(&bsmux_param)) != HD_OK) {
								DBG_ERR("bsmux_set_param(HD_BSMUX_PARAM_USER_DATA) fail(%d)\n", hd_ret);
								ret = E_SYS;
							}
						}
					} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
						for (j = 0; j < 2; j ++) {
							bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
							if ((hd_ret = bsmux_set_param(&bsmux_param)) != HD_OK) {
								DBG_ERR("bsmux_set_param(HD_BSMUX_PARAM_USER_DATA) fail(%d)\n", hd_ret);
								ret = E_SYS;
							}
						}
					} else {
						ret = E_NOSPT;
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_ENCRYPT: {
			ret = E_SYS;
			if (value == 0) {
				DBG_ERR("MOVIEMULTI_PARAM_FILE_CUST_TAG is set with NULL value\r\n");
				ret = E_PAR;
			} else {
				if (IsMovieMultiOpened == TRUE) {
					DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_GPS_BUF_SIZE after ImageApp_MovieMulti Open\r\n");
				} else {
					MOVIEMULTI_FILE_ENCRYPT *pcrypt = (MOVIEMULTI_FILE_ENCRYPT *) value;
					if (pcrypt->key == NULL) {
						DBG_ERR("MOVIEMULTI_PARAM_FILE_ENCRYPT: key is null\r\n");
					} else {
						if ((pcrypt->mode == MOVIEMULTI_ENCRYPT_MODE_AES128 && pcrypt->key_len == 16) || (pcrypt->mode == MOVIEMULTI_ENCRYPT_MODE_AES256 && pcrypt->key_len == 32)) {
							iamovie_encrypt_type = pcrypt->type;
							iamovie_encrypt_mode = pcrypt->mode;
							memcpy(iamovie_encrypt_key, pcrypt->key, pcrypt->key_len);
							DBG_DUMP("Set MOVIEMULTI_PARAM_FILE_ENCRYPT type=0x%08x, mode=0x%08x\r\n", iamovie_encrypt_type, iamovie_encrypt_mode);
							#if 0
							DBG_DUMP("Key=");
							for (j = 0; j < pcrypt->key_len; j++) {
								DBG_DUMP(" %02x", iamovie_encrypt_key[j]);
							}
							DBG_DUMP("\r\n");
							#endif
							ret = E_OK;
						} else {
							DBG_ERR("MOVIEMULTI_PARAM_FILE_ENCRYPT: mode(%d) and key_len(%d) mismatch\r\n", pcrypt->mode, pcrypt->key_len);
						}
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_VAR_RATE: {
			#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
			ret = E_OK;
			if (value == 0 || value > 120) {
				DBG_ERR("MOVIEMULTI_PARAM_FILE_VAR_RATE invalid rate(%d)\r\n", value);
				ret = E_PAR;
			} else {
				DBG_DUMP("Set MOVIEMULTI_PARAM_FILE_VAR_RATE to %d\r\n", value);
				HD_PATH_ID bsmux_ctrl = 0;
				HD_BSMUX_EN_UTIL util = {
					.type    = HD_BSMUX_EN_UTIL_VAR_RATE,
					.enable  = ENABLE,
					.resv[0] = value,
					.resv[1] = 0,
				};
				if (i == _CFG_CTRL_ID) {
					if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(0), &bsmux_ctrl)) != HD_OK) {
						DBG_ERR("hd_bsmux_open(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
					if ((hd_ret = hd_bsmux_set(bsmux_ctrl, HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
					if ((hd_ret = hd_bsmux_close(bsmux_ctrl)) != HD_OK) {
						DBG_ERR("hd_bsmux_close(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
				} else if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					for (j = 0; j < 2; j ++) {
						UINT32 path = 2 * tb.tbl + j;
						if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}

				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					for (j = 0; j < 2; j ++) {
						UINT32 path = 2 * 0 + j;
						if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				} else {
					ret = E_NOSPT;
					break;
				}
			}
			#else
			ret = E_NOSPT;
			#endif
			break;
		}

	case MOVIEMULTI_PARAM_FILE_USE_FILEDB: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_USE_FILEDB after ImageApp_MovieMulti Open\r\n");
				ret = E_SYS;
			} else {
				iamovie_use_filedb_and_naming = value ? TRUE : FALSE;
				DBG_DUMP("MOVIEMULTI_PARAM_FILE_USE_FILEDB set to %d\r\n", iamovie_use_filedb_and_naming);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_CB_CLOSED_FILE_INFO: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_CB_CLOSED_FILE_INFO after ImageApp_MovieMulti Open\r\n");
				ret = E_SYS;
			} else {
				iamovie_movie_path_info = value ? TRUE : FALSE;
				DBG_DUMP("MOVIEMULTI_PARAM_FILE_CB_CLOSED_FILE_INFO set to %d\r\n", iamovie_movie_path_info);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_BUFLOCK_INFO: {
			ret = E_OK;
			if (value) {
				MOVIEMULTI_FILE_BUFLOCK *pinfo = (MOVIEMULTI_FILE_BUFLOCK *)value;
				HD_PATH_ID bsmux_ctrl_path = 0;
				HD_BSMUX_EN_UTIL util = {0};
				util.type    = HD_BSMUX_EN_UTIL_BUFLOCK;
				util.enable  = pinfo->enable? TRUE : FALSE;
				util.resv[0] = pinfo->lock_sec_ms;
				util.resv[1] = pinfo->timeout_ms;

				if (i == _CFG_CTRL_ID) {
					if ((hd_ret = bsmux_get_ctrl_path(0, &bsmux_ctrl_path)) != HD_OK) {
						DBG_ERR("bsmux_get_ctrl_path() fail(%d)\n", hd_ret);
						ret = E_SYS;
					} else {
						if ((hd_ret = hd_bsmux_set(bsmux_ctrl_path, HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL,type=0x%x) fail(%d)\n", util.type, hd_ret);
							ret = E_SYS;
						}
					}
				} else if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					for (j = 0; j < 2; j ++) {
						UINT32 path = 2 * tb.tbl + j;
						if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					for (j = 0; j < 2; j ++) {
						UINT32 path = 2 * 0 + j;
						if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
							DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				} else {
					ret = E_NOSPT;
					break;
				}
			} else {
				DBG_ERR("Set MOVIEMULTI_PARAM_FILE_BUFLOCK_INFO fail. Param is NULL.\r\n");
				ret = E_PAR;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_ROLLBACKSEC_CRASH: {
			if (value < ImgLinkFileInfo.seamless_sec) {
				ImgLinkFileInfo.rollback_sec = value;
				ret = E_OK;
			} else {
				ret = E_PAR;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_KEEPSEC_CRASH: {
			if (value < ImgLinkFileInfo.seamless_sec) {
				ImgLinkFileInfo.forward_sec = value;
				ret = E_OK;
			} else {
				ret = E_PAR;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_SETCRASH_NOT_RO: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				g_ia_moviemulti_crash_not_ro = ((value) ? TRUE : FALSE);
			}
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_FILE_EMR_NOT_RO: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				g_ia_moviemulti_emr_not_ro = ((value) ? TRUE : FALSE);
			}
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_FILE_BSMUX_DDR_ID: {
			if (ImageApp_MovieMulti_IsStreamRunning(i)) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_BSMUX_DDR_ID while recording\r\n");
				ret = E_SYS;
			} else {
				IACOMM_BSMUX_PARAM bsmux_param;
				UINT32 is_emr = (value & 0x80000000) ? 1 : 0;
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_bsmux_finfo[tb.idx][2*tb.tbl+is_emr].ddr_id = (value & 0x7fffffff);
					bsmux_param.path_id = ImgLink[tb.idx].bsmux_p[2*tb.tbl+is_emr];
					bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
					bsmux_param.p_param = (void*)&(img_bsmux_finfo[tb.idx][2*tb.tbl+is_emr]);
					bsmux_set_param(&bsmux_param);
					ret = E_OK;
				} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
					ethcam_bsmux_finfo[tb.idx][is_emr].ddr_id = (value & 0x7fffffff);
					bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[is_emr];
					bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
					bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][is_emr]);
					bsmux_set_param(&bsmux_param);
					ret = E_OK;
				} else {
					ret = E_NOSPT;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_META_DATA_CFG: {
			#if (BSMUX_USE_NEW_PUSH_METHOD == ENABLE)
			if (value) {
				if (IsMovieMultiOpened == FALSE) {
					DBG_ERR("Cannot set MOVIEMULTI_PARAM_FILE_META_DATA_CFG before ImageApp_MovieMulti Open\r\n");
					ret = E_SYS;
				} else {
					HD_BSMUX_META_ALLOC *ptr = (HD_BSMUX_META_ALLOC *)value;
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						for (j = 0; j < 2; j ++) {
							UINT32 path = 2 * tb.tbl + j;
							if (_LinkCheckStatus(ImgLinkStatus[tb.idx].bsmux_p[path])) {
								DBG_ERR("Cannot do hd_bsmux_set(HD_BSMUX_PARAM_META_ALLOC) after start(%d)\r\n");
							} else {
								if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_META_ALLOC, (VOID *)ptr)) != HD_OK) {
									DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_META_ALLOC) fail(%d)\r\n", hd_ret);
									ret = E_SYS;
								} else {
									img_bsmux_meta_data_en[tb.idx][path] = TRUE;
									ret = E_OK;
								}
							}
						}
					} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
						for (j = 0; j < 2; j ++) {
							UINT32 path = j;
							if (_LinkCheckStatus(EthCamLinkStatus[tb.idx].bsmux_p[path])) {
								DBG_ERR("Cannot do hd_bsmux_set(HD_BSMUX_PARAM_META_ALLOC) after start(%d)\r\n");
							} else {
								if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[path], HD_BSMUX_PARAM_META_ALLOC, (VOID *)ptr)) != HD_OK) {
									DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_META_ALLOC) fail(%d)\r\n", hd_ret);
									ret = E_SYS;
								} else {
									ethcam_bsmux_meta_data_en[tb.idx][path] = TRUE;
									ret = E_OK;
								}
							}
						}
					}
				}
			} else {
				DBG_ERR("Set MOVIEMULTI_PARAM_FILE_META_DATA_CFG fail. Param is NULL.\r\n");
				ret = E_PAR;
			}
			#else
			ret = E_NOSPT;
			#endif
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_DispGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	HD_RESULT hd_ret;
	ER ret = E_SYS;
	UINT32 i = id, idx, j;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_DISP_REG_CB: {
			if ((tb.link_type == LINKTYPE_DISP) && (tb.idx < MAX_DISP_PATH)) {
				g_DispCb = (MOVIE_RAWPROC_CB) value;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_DISP_IME_CROP: {
			if (value) {
				MOVIEMULTI_IME_CROP_INFO *pInfo = (MOVIEMULTI_IME_CROP_INFO *) value;
				if ((pInfo->IMEWin.x + pInfo->IMEWin.w > pInfo->IMESize.w) || (pInfo->IMEWin.y + pInfo->IMEWin.h > pInfo->IMESize.h)) {
					DBG_ERR("[MOVIEMULTI_PARAM_DISP_IME_CROP] Window out of range! %d+%d>%d or %d+%d>%d\r\n", pInfo->IMEWin.x, pInfo->IMEWin.w, pInfo->IMESize.w, pInfo->IMEWin.y, pInfo->IMEWin.h, pInfo->IMESize.h);
				} else {
					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						// copy data structure
						ret = E_OK;
						memcpy(&(DispIMECropInfo[tb.idx]), pInfo, sizeof(MOVIEMULTI_IME_CROP_INFO));
						if (IsImgLinkOpened[tb.idx] == TRUE) {     // already opened, update window
							IACOMM_VPROC_PARAM vproc_param = {0};
							HD_DIM sz;
							if (gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_DISP] == UNUSED) {
								DBG_ERR("MOVIEMULTI_PARAM_DISP_IME_CROP cannot be set since gSwitchLink[%d][%d]=%d\r\n", tb.idx, IAMOVIE_VPRC_EX_DISP, gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_DISP]);
								ret = E_SYS;
								break;
							}
							j = gUserProcMap[tb.idx][gSwitchLink[tb.idx][IAMOVIE_VPRC_EX_DISP]];
							j = (j == 3) ? 2 : j;
							vproc_param.video_proc_path = ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j];
							vproc_param.p_dim           = &sz;
							vproc_param.p_dim->w        = DispIMECropInfo[tb.idx].IMESize.w;
							vproc_param.p_dim->h        = DispIMECropInfo[tb.idx].IMESize.h;
							vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
							vproc_param.frc             = _ImageApp_MovieMulti_frc(fps_vprc_p_out[tb.idx][j], fps_vcap_out[tb.idx]);
							vproc_param.pfunc           = NULL;
							vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
							vproc_param.video_proc_crop_param.win.rect.x = DispIMECropInfo[tb.idx].IMEWin.x;
							vproc_param.video_proc_crop_param.win.rect.y = DispIMECropInfo[tb.idx].IMEWin.y;
							vproc_param.video_proc_crop_param.win.rect.w = DispIMECropInfo[tb.idx].IMEWin.w;
							vproc_param.video_proc_crop_param.win.rect.h = DispIMECropInfo[tb.idx].IMEWin.h;
							if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
								DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
								ret = E_SYS;
							}
							if (img_vproc_no_ext[tb.idx]) {
								_ImageApp_MovieMulti_UpdateVprcDepth(vproc_param.video_proc_path, 1);
							}
							if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j])) {
								if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j])) != HD_OK) {
									DBG_ERR("hd_videoproc_start fail: id=%d, ret=%d\r\n", ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j], hd_ret);
									ret = E_SYS;
								}
							}

							if (DispIMECropFixedOutSize[tb.idx] == FALSE) {
								if (img_vproc_no_ext[tb.idx] == FALSE) {
									IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};
									vproc_param_ex.video_proc_path_ex  = ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_DISP];
									vproc_param_ex.video_proc_path_src = ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j];
									vproc_param_ex.p_dim               = &sz;
									vproc_param_ex.p_dim->w            = DispIMECropInfo[tb.idx].IMEWin.w;
									vproc_param_ex.p_dim->h            = DispIMECropInfo[tb.idx].IMEWin.h;
									vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420;
									vproc_param_ex.dir                 = HD_VIDEO_DIR_NONE;
									vproc_param_ex.frc                 = _ImageApp_MovieMulti_frc(fps_vprc_e_out[tb.idx][IAMOVIE_VPRC_EX_DISP], fps_vprc_p_out[tb.idx][gSwitchLink[tb.idx][j]]);
									vproc_param_ex.depth               = 1;                       // for rawenc case, the depth should keep 1

									memset((void *)&(vproc_param_ex.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
									if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
										DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
										ret = E_SYS;
									}
									if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_DISP])) {
										if ((hd_ret = hd_videoproc_start(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_DISP])) != HD_OK) {
											DBG_ERR("hd_videoproc_start fail: id=%d, ret=%d\r\n", ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][IAMOVIE_VPRC_EX_DISP], hd_ret);
											ret = E_SYS;
										}
									}
								}
							}
						}
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_DISP_MIRROR_FLIP: {
			idx = 0;
			ret = E_OK;
			g_DispMirrorFlip[idx] = value;
			if (IsDispLinkOpened[idx] == TRUE) {
				HD_URECT win;
				if (DispLink[idx].vout_win.w && DispLink[idx].vout_win.h) {
					win.x = DispLink[idx].vout_win.x;
					win.y = DispLink[idx].vout_win.y;
					win.w = DispLink[idx].vout_win.w;
					win.h = DispLink[idx].vout_win.h;
				} else {
					win.x = 0;
					win.y = 0;
					win.w = DispLink[idx].vout_syscaps.output_dim.w;
					win.h = DispLink[idx].vout_syscaps.output_dim.h;
				}
				IACOMM_VOUT_PARAM vout_param = {
					.video_out_path = DispLink[idx].vout_p[0],
					.p_rect         = &win,
					.enable         = TRUE,
					.dir            = (DispLink[idx].vout_dir | g_DispMirrorFlip[idx]),
					.pfunc_cfg      = &(disp_func_cfg[idx]),
				};
				if ((hd_ret = vout_set_param(&vout_param)) != HD_OK) {
					DBG_ERR("vout_set_param fail=%d\n", hd_ret);
					ret = E_SYS;
				}
				if (_LinkCheckStatus(DispLinkStatus[idx].vout_p[0])) {
					if ((hd_ret = hd_videoout_start(DispLink[idx].vout_p[0])) != HD_OK) {
						DBG_ERR("hd_videoout_start fail: id=%d, ret=%d\r\n", DispLink[0].vout_p[0], hd_ret);
						ret = E_SYS;
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_DISP_IME_CROP_AUTO_SCALING_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				DispIMECropNoAutoScaling[tb.idx] = value ? FALSE : TRUE;
				ret = E_OK;
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_DISP_WINDOW: {
			if (value) {
				HD_URECT *p_rect = (HD_URECT *)value;
				ret = E_OK;
				idx = 0;
				#if 0   // remove check for ide scale
				if (((DispLink[0].vout_dir & HD_VIDEO_DIR_ROTATE_270) == HD_VIDEO_DIR_ROTATE_270) || ((DispLink[0].vout_dir & HD_VIDEO_DIR_ROTATE_90) == HD_VIDEO_DIR_ROTATE_90)) {
					if (((p_rect->x + p_rect->w) > DispLink[idx].vout_syscaps.output_dim.h) || ((p_rect->y + p_rect->h) > DispLink[idx].vout_syscaps.output_dim.w)) {
						DBG_ERR("Set MOVIEMULTI_PARAM_DISP_WINDOW failed. x(%d)+w(%d) or y(%d)+h(%d) out of range(%d,%d)\r\n", p_rect->x, p_rect->w, p_rect->y, p_rect->h, DispLink[idx].vout_syscaps.output_dim.h, DispLink[idx].vout_syscaps.output_dim.w);
						ret = E_SYS;
					}
				} else {
					if (((p_rect->x + p_rect->w) > DispLink[idx].vout_syscaps.output_dim.w) || ((p_rect->y + p_rect->h) > DispLink[idx].vout_syscaps.output_dim.h)) {
						DBG_ERR("Set MOVIEMULTI_PARAM_DISP_WINDOW failed. x(%d)+w(%d) or y(%d)+h(%d) out of range(%d,%d)\r\n", p_rect->x, p_rect->w, p_rect->y, p_rect->h, DispLink[idx].vout_syscaps.output_dim.w, DispLink[idx].vout_syscaps.output_dim.h);
						ret = E_SYS;
					}
				}
				#endif
				if (ret == E_OK) {
					user_disp_win[idx].x = p_rect->x;
					user_disp_win[idx].y = p_rect->y;
					user_disp_win[idx].w = p_rect->w;
					user_disp_win[idx].h = p_rect->h;
					if (IsDispLinkOpened[idx] == TRUE) {
						HD_URECT win;
						if (user_disp_win[idx].w && user_disp_win[idx].h) {
							win.x = user_disp_win[idx].x;
							win.y = user_disp_win[idx].y;
							win.w = user_disp_win[idx].w;
							win.h = user_disp_win[idx].h;
						} else if (DispLink[idx].vout_win.w && DispLink[idx].vout_win.h) {
							win.x = DispLink[idx].vout_win.x;
							win.y = DispLink[idx].vout_win.y;
							win.w = DispLink[idx].vout_win.w;
							win.h = DispLink[idx].vout_win.h;
						} else {
							win.x = 0;
							win.y = 0;
							win.w = DispLink[idx].vout_syscaps.output_dim.w;
							win.h = DispLink[idx].vout_syscaps.output_dim.h;
						}
						IACOMM_VOUT_PARAM vout_param = {
							.video_out_path = DispLink[idx].vout_p[0],
							.p_rect         = &win,
							.enable         = TRUE,
							.dir            = (DispLink[idx].vout_dir | g_DispMirrorFlip[idx]),
							.pfunc_cfg      = &(disp_func_cfg[idx]),
						};
						if ((hd_ret = vout_set_param(&vout_param)) != HD_OK) {
							DBG_ERR("vout_set_param fail=%d\n", hd_ret);
							ret = E_SYS;
						}
						if ((ret == E_OK) && _LinkCheckStatus(DispLinkStatus[idx].vout_p[0])) {
							if ((hd_ret = hd_videoout_start(DispLink[idx].vout_p[0])) != HD_OK) {
								DBG_ERR("hd_videoout_start fail: id=%d, ret=%d\r\n", DispLink[0].vout_p[0], hd_ret);
							}
						}
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_DISP_FPS: {
			if ((tb.link_type == LINKTYPE_DISP) && (tb.idx < MAX_DISP_PATH)) {
				if (value <= 60) {
					g_DispFps[tb.idx] = value;
					ret = E_OK;
				} else {
					DBG_ERR("Param MOVIEMULTI_PARAM_DISP_FPS out of range!(%d>60)\r\n", value);
					ret = E_PAR;
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_DISP_IME_CROP_FIXED_OUT_SIZE_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				DispIMECropFixedOutSize[tb.idx] = value ? TRUE : FALSE;
				ret = E_OK;
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_DISP_QUEUE_DEPTH: {
			if ((tb.link_type == LINKTYPE_DISP) && (tb.idx < MAX_DISP_PATH)) {
				disp_queue_depth[tb.idx] = value;
				ImageApp_MovieMulti_DUMP("id%d: set MOVIEMULTI_PARAM_DISP_QUEUE_DEPTH = %d\r\n", i, disp_queue_depth[tb.idx]);
				ret = E_OK;
			} else {
				DBG_ERR("id%d: set MOVIEMULTI_PARAM_DISP_QUEUE_DEPTH not supported\r\n", i);
				ret = E_NOSPT;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_WifiGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_WIFI_REG_CB: {
			g_WifiCb = (MOVIE_RAWPROC_CB) value;
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PRARM_UVAC_FUNC: {
			wifi_movie_uvac_func_en = value ? TRUE : FALSE;
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_UVAC_VEND_DEV_DESC: {
			if (value) {
				gpWifiMovieUvacVendDevDesc = (UVAC_VEND_DEV_DESC *)value;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_UVAC_VID_RESO_ARY: {
			if (value) {
				PUVAC_VID_RESO_ARY pAry = (PUVAC_VID_RESO_ARY)value;
				UVAC_ConfigVidReso(pAry->pVidResAry, pAry->aryCnt);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_UVAC_AUD_SAMPLERATE_ARY: {
			if (value) {
				UVAC_SetConfig(UVAC_CONFIG_AUD_SAMPLERATE, value);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_UVAC_VID_FMT_TYPE: {
			UVAC_SetConfig(UVAC_CONFIG_VIDEO_FORMAT_TYPE, value);
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_UVAC_CDC_EN: {
			UVAC_SetConfig(UVAC_CONFIG_CDC_ENABLE, value ? ENABLE : DISABLE);
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_UVAC_CDC2_EN: {
			#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
			UVAC_SetConfig(UVAC_CONFIG_CDC2_ENABLE, value ? ENABLE : DISABLE);
			ret = E_OK;
			#else
			ret = E_NOSPT;
			#endif
			break;
		}

	case MOVIEMULTI_PARAM_UVAC_CDC_PSTN_REQ_CB: {
			UVAC_SetConfig(UVAC_CONFIG_CDC_PSTN_REQUEST_CB, value);
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_WIFI_VENC_TIMER_MODE: {
			if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				wifi_venc_trigger_mode[tb.idx] = value ? MOVIE_CODEC_TRIGGER_TIMER : MOVIE_CODEC_TRIGGER_DIRECT;
				ImageApp_MovieMulti_DUMP("id%d set MOVIEMULTI_PARAM_WIFI_VENC_TIMER_MODE to %d\r\n", i, wifi_venc_trigger_mode[tb.idx]);
				ret = E_OK;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_MiscGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	UINT32 i = id;
	ER ret = E_SYS;
	MOVIE_TBL_IDX tb;
	HD_RESULT hd_ret;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_FILEDB_FILTER: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				g_ia_moviemulti_filedb_filter = value;
			}
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PRARM_FILEDB_MAX_MUM: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				g_ia_moviemulti_filedb_max_num = value;
			}
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_YUV_COMPRESS: {
			if (IsImgLinkOpened[tb.idx]) {
				DBG_ERR("MOVIEMULTI_PARAM_YUV_COMPRESS should set before open\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					img_vproc_yuv_compress[tb.idx] = value ? TRUE : FALSE;
					ImageApp_MovieMulti_DUMP("Set MOVIEMULTI_PARAM_YUV_COMPRESS=%d to idx[%d]:\r\n", img_vproc_yuv_compress[tb.idx], tb.idx);
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIEMULTI_PRARM_DELETE_FILTER: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				g_ia_moviemulti_delete_filter = value;
			}
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_PSEUDO_REC_MODE: {
			if (_ImageApp_MovieMulti_IsRecording()) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_PSEUDO_REC_MODE when recording.\r\n");
			} else {
				if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
					if (value < MOVIE_PSEUDO_REC_MODE_MAX) {
						DBG_DUMP("Set MOVIEMULTI_PARAM_PSEUDO_REC_MODE to id%d=%d\r\n", i, value);
						img_pseudo_rec_mode[tb.idx][tb.tbl] = value;
						ret = E_OK;
					} else {
						DBG_ERR("Set MOVIEMULTI_PARAM_PSEUDO_REC_MODE to id%d=%d out of range\r\n", i, value);
						ret = E_PAR;
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PARAM_TV_RANGE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				img_venc_vui_color_tv_range[tb.idx][tb.tbl] = value ? TRUE : FALSE;
				UINT32 isp_ver = 0;
				if (vendor_isp_get_common(ISPT_ITEM_VERSION, &isp_ver) == HD_ERR_UNINIT) {
					if ((hd_ret = vendor_isp_init()) != HD_OK) {
						DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
					}
				}
				IQT_YCC_FORMAT ycc_format = {
					.id = img_sensor_mapping[tb.idx],
					.format = img_venc_vui_color_tv_range[tb.idx][tb.tbl] ? IQ_UI_YCC_OUT_BT709 : IQ_UI_YCC_OUT_FULL,
				};
				vendor_isp_set_iq(IQT_ITEM_YCC_FORMAT, &ycc_format);
				if (isp_ver == 0) {
					if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
						DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				wifi_venc_vui_color_tv_range[tb.idx] = value ? TRUE : FALSE;
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ethcam_venc_vui_color_tv_range[tb.idx] = value ? TRUE : FALSE;
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_MANUAL_PUSH_VDO_FRAME: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_MANUAL_PUSH_VDO_FRAME after ImageApp_MovieMulti Open\r\n");
				ret = E_SYS;
			} else {
				img_manual_push_vdo_frame[tb.idx][tb.tbl] = value ? TRUE : FALSE;
				ImageApp_MovieMulti_DUMP("id%d: set MOVIEMULTI_PARAM_MANUAL_PUSH_VDO_FRAME to 0x%0x\r\n", i, img_manual_push_vdo_frame[tb.idx][tb.tbl]);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_MANUAL_PUSH_RAW_FRAME: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_MANUAL_PUSH_RAW_FRAME after ImageApp_MovieMulti Open\r\n");
				ret = E_SYS;
			} else {
				img_manual_push_raw_frame[tb.idx] = value ? TRUE : FALSE;
				ImageApp_MovieMulti_DUMP("id%d: set MOVIEMULTI_PARAM_MANUAL_PUSH_RAW_FRAME to 0x%0x\r\n", i, img_manual_push_raw_frame[tb.idx]);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_MANUAL_PUSH_VPE_FRAME: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_MANUAL_PUSH_VPE_FRAME after ImageApp_MovieMulti Open\r\n");
				ret = E_SYS;
			} else {
				img_manual_push_vpe_frame[tb.idx] = value ? TRUE : FALSE;
				ImageApp_MovieMulti_DUMP("id%d: set MOVIEMULTI_PARAM_MANUAL_PUSH_VPE_FRAME to 0x%0x\r\n", i, img_manual_push_vpe_frame[tb.idx]);
				ret = E_OK;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FLOW_FOR_DUMPED_MODE_FAST_BOOT: {
			if (IsMovieMultiOpened == TRUE) {
				DBG_ERR("Cannot set MOVIEMULTI_PARAM_FLOW_FOR_DUMPED_MODE_FAST_BOOT after ImageApp_MovieMulti Open\r\n");
				ret = E_SYS;
			} else {
				img_flow_for_dumped_fast_boot = value ? TRUE : FALSE;
				ImageApp_MovieMulti_DUMP("id%d: set MOVIEMULTI_PARAM_FLOW_FOR_DUMPED_MODE_FAST_BOOT to 0x%0x\r\n", i, img_flow_for_dumped_fast_boot);
				ret = E_OK;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_SetParam_EthCamGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id, j;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_RARAM_ETHCAM_RX_FUNC_EN: {
			EthCamRxFuncEn = value;
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PRARM_ETHCAM_DEC_INFO: {
			if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (EthCamRxFuncEn && value) {
					MOVIEMULTI_ETHCAM_DEC_INFO *pInfo = (MOVIEMULTI_ETHCAM_DEC_INFO *) value;
					ethcam_vdec_cfg[tb.idx][0].max_mem.codec_type = pInfo->codec;
					ethcam_vdec_cfg[tb.idx][0].max_mem.dim.w      = pInfo->width;
					ethcam_vdec_cfg[tb.idx][0].max_mem.dim.h      = pInfo->height;
					ethcam_vdec_cfg[tb.idx][0].max_mem.ddr_id     = dram_cfg.video_decode;
					ethcam_vdec_in_param[tb.idx][0].codec_type    = pInfo->codec;
					ethcam_vdec_desc[tb.idx][0].addr              = (UINT32)pInfo->header;
					ethcam_vdec_desc[tb.idx][0].len               = pInfo->header_size;
					if (is_ethcamlink_opened[tb.idx]) {
						IACOMM_VDEC_CFG vdec_cfg = {
							.video_dec_path = EthCamLink[tb.idx].vdec_p[0],
							.pcfg           = &(ethcam_vdec_cfg[tb.idx][0]),
						};
						if ((hd_ret = vdec_set_cfg(&vdec_cfg)) != HD_OK) {
							DBG_ERR("vdec_set_cfg fail(%d)\n", hd_ret);
							ret = E_SYS;
						}

						IACOMM_VDEC_PARAM vdec_param = {
							.video_dec_path = EthCamLink[tb.idx].vdec_p[0],
							.pin            = &(ethcam_vdec_in_param[tb.idx][0]),
							.pdesc          = &(ethcam_vdec_desc[tb.idx][0]),
						};
						if ((hd_ret = vdec_set_param(&vdec_param)) != HD_OK) {
							DBG_ERR("vdec_set_param fail(%d)\n", hd_ret);
						} else {
							ret = E_OK;
						}
					}
				}
			}
			break;
		}

	case MOVIEMULTI_PRARM_ETHCAM_REC_INFO: {
			if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (value) {
					MOVIEMULTI_ETHCAM_REC_INFO *recinfo = (MOVIEMULTI_ETHCAM_REC_INFO *)value;
					memcpy(&(EthCamLinkRecInfo[tb.idx]), recinfo, sizeof(MOVIEMULTI_ETHCAM_REC_INFO));
					_ImageApp_MovieMulti_SetupRecParam(i);
					if (is_ethcamlink_opened[tb.idx] == TRUE) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
						// reopen vprc
						_ImageApp_MovieMulti_EthCamReopenVPrc(tb.idx);
#endif
						// reopen venc
						_ImageApp_MovieMulti_EthCamReopenVEnc(tb.idx);
						// set bsmux
						IACOMM_BSMUX_PARAM bsmux_param;
						for (j = 0; j < 2; j++) {
							bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
							bsmux_param.id      = HD_BSMUX_PARAM_VIDEOINFO;
							bsmux_param.p_param = (void*)&(ethcam_bsmux_vinfo[tb.idx][j]);
							bsmux_set_param(&bsmux_param);

							bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
							bsmux_param.id      = HD_BSMUX_PARAM_AUDIOINFO;
							bsmux_param.p_param = (void*)&(ethcam_bsmux_ainfo[tb.idx][j]);
							bsmux_set_param(&bsmux_param);

							bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
							bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
							bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[tb.idx][j]);
							bsmux_set_param(&bsmux_param);

							bsmux_param.path_id = EthCamLink[tb.idx].bsmux_p[j];
							bsmux_param.id      = HD_BSMUX_PARAM_REG_CALLBACK;
							bsmux_param.p_param = (void*)_ImageApp_MovieMulti_BsMux_CB;
							bsmux_set_param(&bsmux_param);
						}
					}
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIEMULTI_RARAM_ETHCAM_TX_CLONE_DIRECTTRIGGER_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				img_venc_trigger_mode[tb.idx][tb.tbl] = value ? MOVIE_CODEC_TRIGGER_DIRECT : MOVIE_CODEC_TRIGGER_TIMER;
			}
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PRARM_ETHCAM_JPG_ENC_ON_RX: {
			if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				ethcam_venc_imgcap_on_rx[tb.idx] = value ? TRUE : FALSE;
				ImageApp_MovieMulti_DUMP("[MOVIEMULTI_PRARM_ETHCAM_JPG_ENC_ON_RX] set id%d=%d\r\n", i, ethcam_venc_imgcap_on_rx[tb.idx]);
				ret = E_OK;
			}
			break;
		}
	}
	return ret;
}

ER ImageApp_MovieMulti_SetParam(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value)
{
	ER ret;

	_ImageApp_MovieMulti_LinkCfg();

	if (param >= MOVIEMULTI_PARAM_IPL_GROUP_BEGIN && param < MOVIEMULTI_PARAM_IPL_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_IPLGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_IMG_GROUP_BEGIN && param < MOVIEMULTI_PARAM_IMG_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_ImgGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_CODEC_GROUP_BEGIN && param < MOVIEMULTI_PARAM_CODEC_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_CodecGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_IMGCAP_GROUP_BEGIN && param < MOVIEMULTI_PARAM_IMGCAP_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_ImgCapGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_AUD_GROUP_BEGIN && param < MOVIEMULTI_PARAM_AUD_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_AudGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_FILE_GROUP_BEGIN && param < MOVIEMULTI_PARAM_FILE_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_FileGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_DISP_GROUP_BEGIN && param < MOVIEMULTI_PARAM_DISP_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_DispGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_WIFI_GROUP_BEGIN && param < MOVIEMULTI_PARAM_WIFI_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_WifiGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_MISC_GROUP_BEGIN && param < MOVIEMULTI_PARAM_MISC_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_MiscGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_ETHCAM_GROUP_BEGIN && param < MOVIEMULTI_PARAM_ETHCAM_GROUP_END) {
		ret = _ImageApp_MovieMulti_SetParam_EthCamGroup(id, param, value);
	} else {
		DBG_WRN("Paramter id %x not matched.\r\n", param);
		ret = E_SYS;
	}
	return ret;
}

/// ========== Get parameter area ==========
static ER _ImageApp_MovieMulti_GetParam_IPLGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	ER ret = E_NOSPT;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_VCAP_GYRO_FUNC_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				*value = img_vcap_gyro_func[tb.idx];
				ret = E_OK;
			} else {
				*value = 0;
				ret = E_SYS;
			}
			break;
		}

	case MOVIEMULTI_PARAM_VPRC_EIS_FUNC_EN: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				*value = img_vproc_eis_func[tb.idx];
				ret = E_OK;
			} else {
				*value = 0;
				ret = E_SYS;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_GetParam_ImgGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	return E_NOSPT;
}

static ER _ImageApp_MovieMulti_GetParam_CodecGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	ER ret = E_PAR;
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_CODEC:
	case MOVIEMULTI_PARAM_CODEC_PROFILE_LEVEL: {
			MOVIE_CFG_CODEC_INFO *pinfo;
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (param == MOVIEMULTI_PARAM_CODEC) {
					*value = ImgLinkRecInfo[tb.idx][tb.tbl].codec;
				} else {
					pinfo = (MOVIE_CFG_CODEC_INFO *)value;
					pinfo->codec   = ImgLinkRecInfo[tb.idx][tb.tbl].codec;
					if (pinfo->codec == _CFG_CODEC_H264) {
						pinfo->profile = img_venc_h264_profile[tb.idx][tb.tbl] ? img_venc_h264_profile[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H264_PROFILE;
						pinfo->level   = img_venc_h264_level[tb.idx][tb.tbl]   ? img_venc_h264_level[tb.idx][tb.tbl]   : IAMVOIE_DEFAULT_H264_LEVEL;
					} else if (pinfo->codec == _CFG_CODEC_H265) {
						pinfo->profile = img_venc_h265_profile[tb.idx][tb.tbl] ? img_venc_h265_profile[tb.idx][tb.tbl] : IAMVOIE_DEFAULT_H265_PROFILE;
						pinfo->level   = img_venc_h265_level[tb.idx][tb.tbl]   ? img_venc_h265_level[tb.idx][tb.tbl]   : IAMVOIE_DEFAULT_H265_LEVEL;
					} else {
						pinfo->profile = 0;
						pinfo->level   = 0;
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				if (param == MOVIEMULTI_PARAM_CODEC) {
					*value = WifiLinkStrmInfo[tb.idx].codec;
				} else {
					pinfo = (MOVIE_CFG_CODEC_INFO *)value;
					pinfo->codec   = WifiLinkStrmInfo[tb.idx].codec;
					if (pinfo->codec == _CFG_CODEC_H264) {
						pinfo->profile = wifi_venc_h264_profile[tb.idx] ? wifi_venc_h264_profile[tb.idx] : IAMVOIE_DEFAULT_H264_PROFILE;
						pinfo->level   = wifi_venc_h264_level[tb.idx]   ? wifi_venc_h264_level[tb.idx]   : IAMVOIE_DEFAULT_H264_LEVEL;
					} else if (pinfo->codec == _CFG_CODEC_H265) {
						pinfo->profile = wifi_venc_h265_profile[tb.idx] ? wifi_venc_h265_profile[tb.idx] : IAMVOIE_DEFAULT_H265_PROFILE;
						pinfo->level   = wifi_venc_h265_level[tb.idx]   ? wifi_venc_h265_level[tb.idx]   : IAMVOIE_DEFAULT_H265_LEVEL;
					} else {
						pinfo->profile = 0;
						pinfo->level   = 0;
					}
				}
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (param == MOVIEMULTI_PARAM_CODEC) {
					*value = EthCamLinkRecInfo[tb.idx].codec;
				} else {
					pinfo = (MOVIE_CFG_CODEC_INFO *)value;
					pinfo->codec   = EthCamLinkRecInfo[tb.idx].codec;
					if (pinfo->codec == _CFG_CODEC_H264) {
						pinfo->profile = ethcam_venc_h264_profile[tb.idx] ? ethcam_venc_h264_profile[tb.idx] : IAMVOIE_DEFAULT_H264_PROFILE;
						pinfo->level   = ethcam_venc_h264_level[tb.idx]   ? ethcam_venc_h264_level[tb.idx]   : IAMVOIE_DEFAULT_H264_LEVEL;
					} else if (pinfo->codec == _CFG_CODEC_H265) {
						pinfo->profile = ethcam_venc_h265_profile[tb.idx] ? ethcam_venc_h265_profile[tb.idx] : IAMVOIE_DEFAULT_H265_PROFILE;
						pinfo->level   = ethcam_venc_h265_level[tb.idx]   ? ethcam_venc_h265_level[tb.idx]   : IAMVOIE_DEFAULT_H265_LEVEL;
					} else {
						pinfo->profile = 0;
						pinfo->level   = 0;
					}
				}
				ret = E_OK;
			} else {
				if (param == MOVIEMULTI_PARAM_CODEC) {
					*value = 0;
				} else {
					pinfo = (MOVIE_CFG_CODEC_INFO *)value;
					pinfo->codec   = 0;
					pinfo->profile = 0;
					pinfo->level   = 0;
				}
				ret = E_SYS;
			}
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_SVC: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				*value = img_venc_svc[tb.idx][tb.tbl];
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
				*value = wifi_venc_svc[tb.idx];
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				*value = ethcam_venc_svc[tb.idx];
				ret = E_OK;
			} else {
				ret = E_SYS;
			}
			break;
		}

	case MOVIEMULTI_PARAM_CODEC_RRC:
	case MOVIEMULTI_PARAM_CODEC_AQ: {
			char param_string[30] = {0};
			UINT32 cmd = 0;
			if (param == MOVIEMULTI_PARAM_CODEC_RRC) {
				strncpy(param_string, "MOVIEMULTI_PARAM_CODEC_RRC", sizeof(param_string)-1);
				cmd = HD_VIDEOENC_PARAM_OUT_ROW_RC;
			} else if (param == MOVIEMULTI_PARAM_CODEC_AQ) {
				strncpy(param_string, "MOVIEMULTI_PARAM_CODEC_AQ", sizeof(param_string)-1);
				cmd = HD_VIDEOENC_PARAM_OUT_AQ;
			}
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				if (IsImgLinkOpened[tb.idx] == TRUE) {
					if ((hd_ret = hd_videoenc_get(ImgLink[tb.idx].venc_p[tb.tbl], cmd, (void *)value)) != HD_OK) {
						DBG_ERR("[%s] hd_videoenc_get failed(%d)\r\n", param_string, hd_ret);
						ret = E_SYS;
					} else {
						ret = E_OK;
					}
				} else {
					DBG_ERR("Please get %s after ImageApp_MovieMulti_Open()\r\n", param_string);
					ret = E_SYS;
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				if (is_ethcamlink_opened[tb.idx] == TRUE) {
					if ((hd_ret = hd_videoenc_get(EthCamLink[tb.idx].venc_p[0], cmd, (void *)value)) != HD_OK) {
						DBG_ERR("[%s] hd_videoenc_get failed(%d)\r\n", param_string, hd_ret);
						ret = E_SYS;
					} else {
						ret = E_OK;
					}
				} else {
					DBG_ERR("Please get %s after ImageApp_MovieMulti_Open()\r\n", param_string);
					ret = E_SYS;
				}
			} else {
				ret = E_NOSPT;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_GetParam_ImgCapGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	ER ret = E_PAR;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				*value = (img_venc_out_param[tb.idx][tb.tbl].h26x.source_output) ? img_venc_sout_type[tb.idx][tb.tbl] : MOVIE_IMGCAP_SOUT_NONE;
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				*value = (ethcam_venc_out_param[tb.idx].h26x.source_output) ? ethcam_venc_sout_type[tb.idx] : MOVIE_IMGCAP_SOUT_NONE;
				ret = E_OK;
			} else {
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_IMGCAP_SOUT_SHARED_BUF_SIZE: {
			*value = imgcap_venc_shared_sout_size;
			ret = E_OK;
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_GetParam_AudGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	ER ret = E_NOSPT;
	UINT32 i = id, idx = 0;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_AUD_ACAP_VOL: {
			*value = aud_cap_volume;
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_AUD_ENCBUF_MS: {
			*value = AudEncBufMs[idx];
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_AUD_CAPBUF_MS: {
			*value = AudCapBufMs[idx];
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_AUD_PAUSE_PUSH: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				*value = img_bsmux_audio_pause[tb.idx][2 * tb.tbl];
				ret = E_OK;
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				*value = ethcam_bsmux_audio_pause[tb.idx][0];
				ret = E_OK;
			} else {
				*value = 0;
				DBG_ERR("MOVIEMULTI_PARAM_AUD_PAUSE_PUSH is not support in linktype[%d]\r\n", tb.link_type);
				ret = E_NOSPT;
			}
			break;
		}

	case MOVIEMULTI_PARAM_AUD_CODEC: {
			*value = AudEncCodec[idx];
			ret = E_OK;
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_GetParam_FileGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	ER ret = E_NOSPT;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_FILE_2V1A_MODE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				*value = img_bsmux_2v1a_mode[tb.idx];
				ret = E_OK;
			} else {
				*value = 0;
				ret = E_SYS;
			}
			break;
		}

	case MOVIEMULTI_PARAM_FILE_ROLLBACKSEC_CRASH: {
			*value = ImgLinkFileInfo.rollback_sec;
			ret = E_OK;
			break;
		}

	case MOVIEMULTI_PARAM_FILE_KEEPSEC_CRASH: {
			*value = ImgLinkFileInfo.forward_sec;
			ret = E_OK;
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_GetParam_DispGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	ER ret = E_NOSPT;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_DISP_QUEUE_DEPTH: {
			if ((tb.link_type == LINKTYPE_DISP) && (tb.idx < MAX_DISP_PATH)) {
				*value = disp_queue_depth[tb.idx];
				ret = E_OK;
			} else {
				*value = 0;
				ret = E_SYS;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_GetParam_WifiGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	return E_NOSPT;
}

static ER _ImageApp_MovieMulti_GetParam_MiscGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	ER ret = E_NOSPT;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	switch (param) {
	case MOVIEMULTI_PARAM_PSEUDO_REC_MODE: {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				*value = img_pseudo_rec_mode[tb.idx][tb.tbl];
				ret = E_OK;
			} else {
				*value = 0;
				ret = E_SYS;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_GetParam_EthCamGroup(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	return E_NOSPT;
}

ER ImageApp_MovieMulti_GetParam(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value)
{
	ER ret;

	if (value == NULL) {
		return E_SYS;
	}

	_ImageApp_MovieMulti_LinkCfg();

	if (param >= MOVIEMULTI_PARAM_IPL_GROUP_BEGIN && param < MOVIEMULTI_PARAM_IPL_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_IPLGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_IMG_GROUP_BEGIN && param < MOVIEMULTI_PARAM_IMG_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_ImgGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_CODEC_GROUP_BEGIN && param < MOVIEMULTI_PARAM_CODEC_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_CodecGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_IMGCAP_GROUP_BEGIN && param < MOVIEMULTI_PARAM_IMGCAP_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_ImgCapGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_AUD_GROUP_BEGIN && param < MOVIEMULTI_PARAM_AUD_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_AudGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_FILE_GROUP_BEGIN && param < MOVIEMULTI_PARAM_FILE_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_FileGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_DISP_GROUP_BEGIN && param < MOVIEMULTI_PARAM_DISP_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_DispGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_WIFI_GROUP_BEGIN && param < MOVIEMULTI_PARAM_WIFI_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_WifiGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_MISC_GROUP_BEGIN && param < MOVIEMULTI_PARAM_MISC_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_MiscGroup(id, param, value);
	} else if (param >= MOVIEMULTI_PARAM_ETHCAM_GROUP_BEGIN && param < MOVIEMULTI_PARAM_ETHCAM_GROUP_END) {
		ret = _ImageApp_MovieMulti_GetParam_EthCamGroup(id, param, value);
	} else {
		DBG_WRN("Paramter id %x not matched.\r\n", param);
		ret = E_SYS;
	}
	return ret;
}

