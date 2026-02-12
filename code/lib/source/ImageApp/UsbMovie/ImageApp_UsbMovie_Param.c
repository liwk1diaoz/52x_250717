#include "ImageApp_UsbMovie_int.h"

/// ========== Cross file variables ==========
/// ========== Noncross file variables ==========
static BOOL uvac_suspend_close = FALSE;

ER _ImageApp_UsbMovie_SetupRecParam(UINT32 id)
{
	UINT32 idx = id, j;

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	// config vprc real path
	for (j = 0; j < 2; j ++) {
		usbimg_vproc_param[idx][j].func = 0;
		usbimg_vproc_param[idx][j].dim.w = usbimg_max_imgsize[idx].w;
		usbimg_vproc_param[idx][j].dim.h = usbimg_max_imgsize[idx].h;
		if (j == 1) {
			if (usbimg_vcap_out_size[idx].w && usbimg_vcap_out_size[idx].h) {
				usbimg_vproc_param[idx][j].dim.w = usbimg_vcap_out_size[idx].w;
				usbimg_vproc_param[idx][j].dim.h = usbimg_vcap_out_size[idx].h;
			} else if (UsbImgIPLUserSize[idx].size.w && UsbImgIPLUserSize[idx].size.h) {
				usbimg_vproc_param[idx][j].dim.w = UsbImgIPLUserSize[idx].size.w;
				usbimg_vproc_param[idx][j].dim.h = UsbImgIPLUserSize[idx].size.h;
			}
		}
		usbimg_vproc_param[idx][j].pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		usbimg_vproc_param[idx][j].dir = HD_VIDEO_DIR_NONE;
		usbimg_vproc_param[idx][j].frc = _ImageApp_UsbMovie_frc(1, 1);
		usbimg_vproc_param[idx][j].depth = 0;
	}

	// config vprc extend path
	for (j = 0; j < 1; j ++) {
		usbimg_vproc_param_ex[idx][j].src_path = 0;
		usbimg_vproc_param_ex[idx][j].dim.w = usbimg_max_imgsize[idx].w;
		usbimg_vproc_param_ex[idx][j].dim.h = usbimg_max_imgsize[idx].h;
		usbimg_vproc_param_ex[idx][j].pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		usbimg_vproc_param_ex[idx][j].dir = HD_VIDEO_DIR_NONE;
		usbimg_vproc_param_ex[idx][j].frc = _ImageApp_UsbMovie_frc(1, 1);
		usbimg_vproc_param_ex[idx][j].depth = 1;
	}

	// config venc
	for (j = 0; j < 1; j ++) {
		usbimg_venc_path_cfg[idx][j].max_mem.codec_type          = HD_CODEC_TYPE_H264;
		usbimg_venc_path_cfg[idx][j].max_mem.max_dim.w           = usbimg_max_imgsize[idx].w;
		usbimg_venc_path_cfg[idx][j].max_mem.max_dim.h           = usbimg_max_imgsize[idx].h;
		#if defined(_BSP_NA51102_)
		usbimg_venc_path_cfg[idx][j].max_mem.bitrate             = g_UsbMovie_TBR_Max[idx][j] ? (g_UsbMovie_TBR_Max[idx][j] * 8): NVT_USBMOVIE_TBR_MJPG_HIGH * 2;
		usbimg_venc_path_cfg[idx][j].max_mem.enc_buf_ms          = usbimg_venc_buf_ms[idx][j] ? usbimg_venc_buf_ms[idx][j] : 5000;
		#else
		usbimg_venc_path_cfg[idx][j].max_mem.bitrate             = g_UsbMovie_TBR_Max[idx][j] ? (g_UsbMovie_TBR_Max[idx][j] * 8) : NVT_USBMOVIE_TBR_MJPG_HIGH;
		usbimg_venc_path_cfg[idx][j].max_mem.enc_buf_ms          = usbimg_venc_buf_ms[idx][j] ? usbimg_venc_buf_ms[idx][j] : 2000;
		#endif
		usbimg_venc_path_cfg[idx][j].max_mem.svc_layer           = HD_SVC_DISABLE;
		usbimg_venc_path_cfg[idx][j].max_mem.ltr                 = FALSE;
		usbimg_venc_path_cfg[idx][j].max_mem.rotate              = FALSE;
		usbimg_venc_path_cfg[idx][j].max_mem.source_output       = FALSE;
		usbimg_venc_path_cfg[idx][j].isp_id                      = 0;

		usbimg_venc_in_param[idx][j].dim.w                       = usbimg_max_imgsize[idx].w;
		usbimg_venc_in_param[idx][j].dim.h                       = usbimg_max_imgsize[idx].h;
		usbimg_venc_in_param[idx][j].pxl_fmt                     = HD_VIDEO_PXLFMT_YUV420;
		usbimg_venc_in_param[idx][j].dir                         = HD_VIDEO_DIR_NONE;
		usbimg_venc_in_param[idx][j].frc                         = _ImageApp_UsbMovie_frc(1, 1);

		#if defined(_BSP_NA51102_)
		usbimg_venc_out_param[idx][j].codec_type                 = HD_CODEC_TYPE_H264;
		usbimg_venc_out_param[idx][j].h26x.gop_num               = 30;
		usbimg_venc_out_param[idx][j].h26x.ltr_interval          = 0;
		usbimg_venc_out_param[idx][j].h26x.ltr_pre_ref           = 0;
		usbimg_venc_out_param[idx][j].h26x.gray_en               = DISABLE;
		usbimg_venc_out_param[idx][j].h26x.source_output         = DISABLE;
		usbimg_venc_out_param[idx][j].h26x.profile               = HD_H264E_HIGH_PROFILE;
		usbimg_venc_out_param[idx][j].h26x.level_idc             = HD_H264E_LEVEL_5_1;
		usbimg_venc_out_param[idx][j].h26x.svc_layer             = HD_SVC_DISABLE;
		usbimg_venc_out_param[idx][j].h26x.entropy_mode          = HD_H264E_CABAC_CODING;

		usbimg_venc_rc_param[idx][j].rc_mode                     = HD_RC_MODE_CBR;
		usbimg_venc_rc_param[idx][j].cbr.bitrate                 = g_UsbMovie_TBR_H264;
		usbimg_venc_rc_param[idx][j].cbr.frame_rate_base         = 30;
		usbimg_venc_rc_param[idx][j].cbr.frame_rate_incr         = 1;
		usbimg_venc_rc_param[idx][j].cbr.init_i_qp               = 26;
		usbimg_venc_rc_param[idx][j].cbr.max_i_qp                = 50;
		usbimg_venc_rc_param[idx][j].cbr.min_i_qp                = 10;
		usbimg_venc_rc_param[idx][j].cbr.init_p_qp               = 26;
		usbimg_venc_rc_param[idx][j].cbr.max_p_qp                = 50;
		usbimg_venc_rc_param[idx][j].cbr.min_p_qp                = 10;
		usbimg_venc_rc_param[idx][j].cbr.static_time             = 4;
		usbimg_venc_rc_param[idx][j].cbr.ip_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.key_p_period            = 0;
		usbimg_venc_rc_param[idx][j].cbr.kp_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.p2_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.p3_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.lt_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.motion_aq_str           = 0;
		usbimg_venc_rc_param[idx][j].cbr.max_frame_size          = gUvacMaxFrameSize ? gUvacMaxFrameSize : 0;
		#else
		usbimg_venc_out_param[idx][j].codec_type                 = HD_CODEC_TYPE_JPEG;
		usbimg_venc_out_param[idx][j].jpeg.retstart_interval     = 0;
		usbimg_venc_out_param[idx][j].jpeg.image_quality         = usbimg_jpg_quality.jpg_quality;
		usbimg_venc_out_param[idx][j].jpeg.bitrate               = 0;
		usbimg_venc_out_param[idx][j].jpeg.frame_rate_base       = 1;
		usbimg_venc_out_param[idx][j].jpeg.frame_rate_incr       = 1;

		usbimg_venc_rc_param[idx][j].rc_mode                     = HD_RC_MODE_CBR;
		usbimg_venc_rc_param[idx][j].cbr.bitrate                 = g_UsbMovie_TBR_MJPG;
		usbimg_venc_rc_param[idx][j].cbr.frame_rate_base         = 30;
		usbimg_venc_rc_param[idx][j].cbr.frame_rate_incr         = 1;
		usbimg_venc_rc_param[idx][j].cbr.init_i_qp               = 26;
		usbimg_venc_rc_param[idx][j].cbr.max_i_qp                = 40;
		usbimg_venc_rc_param[idx][j].cbr.min_i_qp                = 10;
		usbimg_venc_rc_param[idx][j].cbr.init_p_qp               = 26;
		usbimg_venc_rc_param[idx][j].cbr.max_p_qp                = 40;
		usbimg_venc_rc_param[idx][j].cbr.min_p_qp                = 10;
		usbimg_venc_rc_param[idx][j].cbr.static_time             = 4;
		usbimg_venc_rc_param[idx][j].cbr.ip_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.key_p_period            = 0;
		usbimg_venc_rc_param[idx][j].cbr.kp_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.p2_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.p3_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.lt_weight               = 0;
		usbimg_venc_rc_param[idx][j].cbr.motion_aq_str           = 0;
		usbimg_venc_rc_param[idx][j].cbr.max_frame_size          = gUvacMaxFrameSize ? gUvacMaxFrameSize : 0;
		#endif
	}

	return E_OK;
}

ER _ImageApp_UsbMovie_SetupAudioParam(void)
{
	UINT32 idx = 0;

	usbaud_acap_drv_cfg[idx].mono                  = (g_UsbAudioInfo.aud_ch == HD_AUDIO_MONO_RIGHT) ? HD_AUDIO_MONO_RIGHT : HD_AUDIO_MONO_LEFT;

	usbaud_acap_dev_cfg[idx].in_max.sample_rate    = g_UsbAudioInfo.aud_rate;
	usbaud_acap_dev_cfg[idx].in_max.sample_bit     = HD_AUDIO_BIT_WIDTH_16;
	usbaud_acap_dev_cfg[idx].in_max.mode           = (g_UsbAudioInfo.aud_ch_num == 2) ? HD_AUDIO_SOUND_MODE_STEREO : HD_AUDIO_SOUND_MODE_MONO;
	usbaud_acap_dev_cfg[idx].in_max.frame_sample   = 1024;
	usbaud_acap_dev_cfg[idx].frame_num_max         = 10;

	usbaud_acap_param[idx].sample_rate             = g_UsbAudioInfo.aud_rate;
	usbaud_acap_param[idx].sample_bit              = HD_AUDIO_BIT_WIDTH_16;
	usbaud_acap_param[idx].mode                    = (g_UsbAudioInfo.aud_ch_num == 2) ? HD_AUDIO_SOUND_MODE_STEREO : HD_AUDIO_SOUND_MODE_MONO;
	usbaud_acap_param[idx].frame_sample            = 1024;

	return E_OK;
}

ER ImageApp_UsbMovie_SetParam(UINT32 id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 idx = id, j;

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	_ImageApp_UsbMovie_LinkCfg();

	switch (param) {
	case UVAC_PARAM_VCAP_FUNC: {
			usbimg_vcap_func[idx] = (UINT32)value;
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_VPROC_FUNC: {
			usbimg_vproc_func[idx] = (UINT32)value;
			ret = E_OK;
			break;
		}


	case UVAC_PARAM_UVAC_INFO: {
			if (value) {
				memcpy((void *)&gUvacInfo, (void *)value, sizeof(UVAC_INFO));
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_UVAC_VEND_DEV_DESC: {
			if (value) {
				gpUvacVendDevDesc = (UVAC_VEND_DEV_DESC *)value;
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_VID_RESO_ARY: {
			if (value) {
				PUVAC_VID_RESO_ARY pAry = (PUVAC_VID_RESO_ARY)value;
				idx = 0;
				usbimg_max_imgsize[idx].w = 0;
				usbimg_max_imgsize[idx].h = 0;
				for (j = 0; j < pAry->aryCnt; j++) {
					if (pAry->pVidResAry[j].width > usbimg_max_imgsize[idx].w) {
						usbimg_max_imgsize[idx].w = pAry->pVidResAry[j].width;
						usbimg_max_imgsize[idx].h = pAry->pVidResAry[j].height;
					}
				}
				UVAC_ConfigVidReso(pAry->pVidResAry, pAry->aryCnt);
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_AUD_SAMPLERATE_ARY: {
			if (value) {
				PUVAC_AUD_SAMPLERATE_ARY pAry = (PUVAC_AUD_SAMPLERATE_ARY)value;
				g_UsbAudioInfo.aud_rate = pAry->pAudSampleRateAry[0];
				g_UsbAudioInfo.aud_ch = 2;
				g_UsbAudioInfo.aud_ch_num = 2;
				UVAC_SetConfig(UVAC_CONFIG_AUD_SAMPLERATE, value);
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_UVAC_TBR_MJPG: {
			if (value) {
				if (value > NVT_USBMOVIE_TBR_MJPG_HIGH) {
					DBG_WRN("NVT_USBMOVIE_CFG_TBR_MJPG, value=0x%x > High=0x%x\r\n", value, NVT_USBMOVIE_TBR_MJPG_HIGH);
					value = NVT_USBMOVIE_TBR_MJPG_HIGH;
				}
				if (value < NVT_USBMOVIE_TBR_MJPG_LOW) {
					DBG_WRN("NVT_USBMOVIE_CFG_TBR_MJPG, value=0x%x < Low=0x%x\r\n", value, NVT_USBMOVIE_TBR_MJPG_LOW);
					value = NVT_USBMOVIE_TBR_MJPG_LOW;
				}
				g_UsbMovie_TBR_MJPG = (UINT32)value;
				UVAC_SetConfig(UVAC_CONFIG_MJPG_TARGET_SIZE, value);
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_UVAC_TBR_H264: {
			if (value) {
				if (value > NVT_USBMOVIE_TBR_H264_HIGH) {
					DBG_ERR("NVT_USBMOVIE_CFG_TBR_H264, value=0x%x > High=0x%x\r\n", value, NVT_USBMOVIE_TBR_H264_HIGH);
					value = NVT_USBMOVIE_TBR_H264_HIGH;
				}
				if (value < NVT_USBMOVIE_TBR_H264_LOW) {
					DBG_ERR("NVT_USBMOVIE_CFG_TBR_H264, value=0x%x < Low=0x%x\r\n", value, NVT_USBMOVIE_TBR_H264_LOW);
					value = NVT_USBMOVIE_TBR_H264_LOW;
				}
				g_UsbMovie_TBR_H264 = (UINT32)value;
				UVAC_SetConfig(UVAC_CONFIG_H264_TARGET_SIZE, value);
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_BS_START: {
			UVAC_SetConfig(UVAC_CONFIG_MFK_LIVEVIEW2REC, (UINT32)0);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_BS_STOP: {
			UVAC_SetConfig(UVAC_CONFIG_MFK_REC2LIVEVIEW, (UINT32)0);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_UVAC_CAP_M3: {
			UVAC_SetConfig(UVAC_CONFIG_UVAC_CAP_M3, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_EU_VENDCMDCB_ID01:
	case UVAC_PARAM_EU_VENDCMDCB_ID02:
	case UVAC_PARAM_EU_VENDCMDCB_ID03:
	case UVAC_PARAM_EU_VENDCMDCB_ID04:
	case UVAC_PARAM_EU_VENDCMDCB_ID05:
	case UVAC_PARAM_EU_VENDCMDCB_ID06:
	case UVAC_PARAM_EU_VENDCMDCB_ID07:
	case UVAC_PARAM_EU_VENDCMDCB_ID08: {
			#if 0
			idx = param - UVAC_PARAM_EU_VENDCMDCB_ID01;
			idx += UVAC_CONFIG_EU_VENDCMDCB_ID01;
			UVAC_SetConfig(idx, value);
			ret = E_OK;
			#else
			DBG_WRN("UVAC_PARAM_EU_VENDCMDCB_IDxx is a legacy setting. Not support anymore.\r\n");
			ret = E_NOSPT;
			#endif
			break;
		}

	case UVAC_PARAM_WINUSB_ENABLE: {
			//UVAC_SetConfig(UVAC_CONFIG_WINUSB_ENABLE, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_WINUSB_CB: {
			UVAC_SetConfig(UVAC_CONFIG_WINUSB_CB, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_BULK_DATA: {
			ret = E_NOSPT;
			break;
		}

	case UVAC_PARAM_VID_FMT_TYPE: {
			UVAC_SetConfig(UVAC_CONFIG_VIDEO_FORMAT_TYPE, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_CDC_ENABLE: {
			UVAC_SetConfig(UVAC_CONFIG_CDC_ENABLE, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_CDC_PSTN_REQUEST_CB: {
			UVAC_SetConfig(UVAC_CONFIG_CDC_PSTN_REQUEST_CB, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_AUD_FMT_TYPE: {
			ret = E_NOSPT;
			break;
		}

	case UVAC_PARAM_MAX_FRAME_SIZE: {
			gUvacMaxFrameSize = value;
			UVAC_SetConfig(UVAC_CONFIG_MAX_FRAME_SIZE, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_SUSPEND_CLOSE_IMM: {
			uvac_suspend_close = (BOOL)value;
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_EU_CTRL: {
			UVAC_SetConfig(UVAC_CONFIG_XU_CTRL, (UINT32)value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_VCAP_OUTFUNC: {
			if (value > HD_VIDEOCAP_OUTFUNC_ONEBUF) {
				DBG_ERR("Invalid setting(%d) of MOVIEMULTI_PARAM_D2D_MODE\r\n", value);
				break;
			}
			// var
			usbimg_vcap_func_cfg[idx].out_func = (HD_VIDEOCAP_OUTFUNC)value;
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_UVC2_MJPG_FRM_INFO: {
			if (value) {
				PUVAC_VID_RESO_ARY pAry = (PUVAC_VID_RESO_ARY)value;
				idx = 1;
				for (j = 0; j < pAry->aryCnt; j++) {
					if (pAry->pVidResAry[j].width > usbimg_max_imgsize[idx].w) {
						usbimg_max_imgsize[idx].w = pAry->pVidResAry[j].width;
						usbimg_max_imgsize[idx].h = pAry->pVidResAry[j].height;
					}
				}
				UVAC_SetConfig(UVAC_CONFIG_UVC2_MJPG_FRM_INFO, value);
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_UVC2_H264_FRM_INFO: {
			if (value) {
				PUVAC_VID_RESO_ARY pAry = (PUVAC_VID_RESO_ARY)value;
				idx = 1;
				for (j = 0; j < pAry->aryCnt; j++) {
					if (pAry->pVidResAry[j].width > usbimg_max_imgsize[idx].w) {
						usbimg_max_imgsize[idx].w = pAry->pVidResAry[j].width;
						usbimg_max_imgsize[idx].h = pAry->pVidResAry[j].height;
					}
				}
				UVAC_SetConfig(UVAC_CONFIG_UVC2_H264_FRM_INFO, value);
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_VCAP_PAT_GEN: {
			if (IsUsbImgLinkOpened[idx]) {
				DBG_ERR("UVAC_PARAM_VCAP_PAT_GEN should set before open\r\n");
			} else {
				if (value > HD_VIDEOCAP_SEN_PAT_HVINCREASE) {
					DBG_ERR("Set UVAC_PARAM_VCAP_PAT_GEN out of range(%d)\r\n", value);
					ret = E_PAR;
				} else {
					// var
					usbimg_vcap_patgen[idx] = (UINT32)value;
					ret = E_OK;
				}
			}
			break;
		}

	case UVAC_PARAM_IPL_MIRROR: {
			UINT32 dir = (UINT32)value;
			if (dir & ~HD_VIDEO_DIR_MIRRORXY) {
				DBG_ERR("Invalid setting(%x) of UVAC_PARAM_IPL_MIRROR\r\n", value);
				break;
			}
			// var
			usbimg_vproc_in_param[idx].dir = (HD_VIDEO_DIR)dir;
			if (IsUsbImgLinkOpened[idx] == TRUE) {
				// vprc
				IACOMM_VPROC_PARAM_IN vproc_param_in = {
					.video_proc_path = UsbImgLink[idx].vproc_i[0],
					.in_param        = &(usbimg_vproc_in_param[idx]),
				};
				if ((hd_ret = vproc_set_param_in(&vproc_param_in)) != HD_OK) {
					DBG_ERR("vproc_set_param_in fail(%d)\n", hd_ret);
				}
				// resetrt one running path to active setting
				for(j = 0; j < 2; j++) {
					if (UsbImgLinkStatus[idx].vproc_p_phy[j]) {
						if ((hd_ret = hd_videoproc_start(UsbImgLink[idx].vproc_p_phy[j])) != HD_OK) {
							DBG_ERR("hd_videoproc_start fail(%d)\n", hd_ret);
						}
						break;
					}
				}
			}
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_IPL_USER_IMG_SIZE: {
			if (value) {
				UVAC_IPL_SIZE_INFO *pipl_size = (UVAC_IPL_SIZE_INFO *) value;
				// var
				memcpy((void *)&(UsbImgIPLUserSize[idx]), (void *)pipl_size, sizeof(UVAC_IPL_SIZE_INFO));
				ImageApp_UsbMovie_DUMP("Set UVAC_PARAM_IPL_USER_IMG_SIZE=(%d,%d)p%d to idx[%d]:\r\n", UsbImgIPLUserSize[idx].size.w, UsbImgIPLUserSize[idx].size.h, UsbImgIPLUserSize[idx].fps, idx);
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_VCAP_OUT_SIZE: {
#if defined(_BSP_NA51055_)
			DBG_ERR("UVAC_PARAM_VCAP_OUT_SIZE is not supported in this chip.\r\n");
			ret = E_NOSPT;
#else
			if (IsUsbImgLinkOpened[idx]) {
				DBG_ERR("UVAC_PARAM_VCAP_OUT_SIZE should set before open\r\n");
			} else {
				if (value) {
					USIZE *psz = (USIZE *)value;
					usbimg_vcap_out_size[idx].w = psz->w;
					usbimg_vcap_out_size[idx].h = psz->h;
					ImageApp_UsbMovie_DUMP("Set UVAC_PARAM_VCAP_OUT_SIZE=(%d, %d) to idx[%d]:\r\n", usbimg_vcap_out_size[idx].w, usbimg_vcap_out_size[idx].h, idx);
					ret = E_OK;
				}
			}
#endif
			break;
		}

	case UVAC_PARAM_IMG_JPG_QUALITY: {
			if (IsUsbImgLinkOpened[idx]) {
				DBG_ERR("UVAC_PARAM_IMG_JPG_QUALITY should set before open\r\n");
				ret = E_SYS;
			} else {
				if (value) {
					UVAC_JPG_QUALITY *ptr = (UVAC_JPG_QUALITY *)value;
					if (ptr->jpg_quality <= 100 /*&& ptr->buf_size*/) {
						usbimg_jpg_quality.jpg_quality = ptr->jpg_quality;
						usbimg_jpg_quality.buf_size= ptr->buf_size;
						DBG_DUMP("UVAC_PARAM_IMG_JPG_QUALITY: set jpg_quality=%d and buf_size=%d\r\n", ptr->jpg_quality, ptr->buf_size);
						ret = E_OK;
					} else {
						DBG_ERR("UVAC_PARAM_IMG_JPG_QUALITY: jpg_quality %d should between 1~100 or buf_size = 0\r\n", ptr->jpg_quality);
						ret = E_PAR;
					}
				} else {
					DBG_ERR("UVAC_PARAM_IMG_JPG_QUALITY: null parameter\r\n");
					ret = E_PAR;
				}
			}
			break;
		}

	case UVAC_PARAM_SIE_REMAP: {
			#if defined(_BSP_NA51089_)
			if (IsUsbImgLinkOpened[idx]) {
				DBG_ERR("UVAC_PARAM_SIE_REMAP can only be set before open\r\n");
				ret = E_SYS;
				break;
			}
			usbimg_vcap_sie_remap[idx] = value ? 1 : 0;
			DBG_DUMP("Set UVAC_PARAM_SIE_REMAP id%d=%d\r\n", id, usbimg_vcap_sie_remap[idx]);
			ret = E_OK;
			#else
			DBG_ERR("UVAC_PARAM_SIE_REMAP is not supported.\r\n");
			ret = E_NOSPT;
			#endif
			break;
		}

	case UVAC_PARAM_CT_CONTROLS: {
			#if (defined(_BSP_NA51055_) && defined(__FREERTOS))
			ret = E_NOSPT;
			#else
			UVAC_SetConfig(UVAC_CONFIG_CT_CONTROLS, value);
			ret = E_OK;
			#endif
			break;
		}

	case UVAC_PARAM_PU_CONTROLS: {
			#if (defined(_BSP_NA51055_) && defined(__FREERTOS))
			ret = E_NOSPT;
			#else
			UVAC_SetConfig(UVAC_CONFIG_PU_CONTROLS, value);
			ret = E_OK;
			#endif
			break;
		}

	case UVAC_PARAM_CT_CB: {
			UVAC_SetConfig(UVAC_CONFIG_CT_CB, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_PU_CB: {
			UVAC_SetConfig(UVAC_CONFIG_PU_CB, value);
			ret = E_OK;
			break;
		}

	case UVAC_PARAM_UVAC_TBR_MAX: {
			if (IsUsbImgLinkOpened[idx]) {
				DBG_ERR("UVAC_PARAM_UVAC_TBR_MAX should set before open\r\n");
			} else {
				g_UsbMovie_TBR_Max[idx][0] = value;
				ImageApp_UsbMovie_DUMP("Set UVAC_PARAM_UVAC_TBR_MAX=%d to idx[%d]:\r\n", g_UsbMovie_TBR_Max[idx][0], idx);
				ret = E_OK;
			}
			break;
		}

	case UVAC_PARAM_ENCBUF_MS: {
			if (IsUsbImgLinkOpened[idx]) {
				DBG_ERR("UVAC_PARAM_ENCBUF_MS should set before open\r\n");
			} else {
				usbimg_venc_buf_ms[idx][0] = value;
				ImageApp_UsbMovie_DUMP("Set UVAC_PARAM_ENCBUF_MS=%d to idx[%d]:\r\n", usbimg_venc_buf_ms[idx][0], idx);
				ret = E_OK;
			}
			break;
		}
	}
	return ret;
}

ER ImageApp_UsbMovie_GetParam(UINT32 id, UINT32 param, UINT32* value)
{
	return E_NOSPT;
}

