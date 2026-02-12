#include "ImageApp_MovieMulti_int.h"

/// ========== Cross file variables ==========
MOVIE_STRM_INFO WifiLinkStrmInfo[MAX_WIFI_PATH] = {0};
UINT32 IsWifiLinkOpened[MAX_WIFI_PATH] = {0};
UINT32 IsWifiLinkStarted[MAX_WIFI_PATH] = {0};
UINT32 IsUvacLinkStarted[MAX_WIFI_PATH] = {0};
MOVIE_WIFI_LINK WifiLink[MAX_WIFI_PATH];
MOVIE_WIFI_LINK_STATUS WifiLinkStatus[MAX_WIFI_PATH] = {0};
HD_VIDEOENC_PATH_CONFIG wifi_venc_path_cfg[MAX_WIFI_PATH] = {0};
HD_VIDEOENC_IN  wifi_venc_in_param[MAX_WIFI_PATH] = {0};
HD_VIDEOENC_OUT2 wifi_venc_out_param[MAX_WIFI_PATH] = {0};
HD_H26XENC_RATE_CONTROL2 wifi_venc_rc_param[MAX_WIFI_PATH] = {0};
UINT32 wifi_venc_vui_disable[MAX_WIFI_PATH] = {0};
UINT32 wifi_venc_vui_color_tv_range[MAX_WIFI_PATH] = {0};
UINT32 wifi_venc_max_frame_size[MAX_WIFI_PATH] = {0};
MOVIE_CFG_PROFILE wifi_venc_h264_profile[MAX_WIFI_PATH] = {0};
MOVIE_CFG_LEVEL wifi_venc_h264_level[MAX_WIFI_PATH] = {0};
MOVIE_CFG_PROFILE wifi_venc_h265_profile[MAX_WIFI_PATH] = {0};
MOVIE_CFG_LEVEL wifi_venc_h265_level[MAX_WIFI_PATH] = {0};
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
VENDOR_VDOENC_XVR_APP wifi_venc_xvr_app[MAX_WIFI_PATH] = {0};
#endif
HD_VIDEOENC_SVC_LAYER wifi_venc_svc[MAX_WIFI_PATH] = {0};
UINT32 wifi_venc_low_power_mode[MAX_WIFI_PATH] = {0};
UINT64 wifi_venc_timestamp[MAX_WIFI_PATH] = {0};
UINT32 WifiLinkVdoEncBufMs[MAX_WIFI_PATH] = {3000};
HD_VIDEOENC_MAXMEM wifi_venc_maxmem[MAX_WIFI_PATH] = {0};
MOVIE_RAWPROC_CB  g_WifiCb = NULL;
UINT32 wifi_movie_uvac_func_en = FALSE;
UVAC_INFO gWifiMovieUvacInfo = {0};
UVAC_VEND_DEV_DESC *gpWifiMovieUvacVendDevDesc = NULL;
UINT32 wifi_movie_uvac_pa = 0, wifi_movie_uvac_va = 0, wifi_movie_uvac_size = 0;
HD_VIDEOENC_BUFINFO wifi_venc_bs_buf[MAX_WIFI_PATH] = {0};
UINT32 wifi_venc_bs_buf_va[MAX_WIFI_PATH] = {0};
UINT32 wifi_venc_trigger_mode[MAX_WIFI_PATH] = {MOVIE_CODEC_TRIGGER_DIRECT};
/// ========== Noncross file variables ==========
static THREAD_HANDLE iamovie_wifi_tsk_id;
static UINT32 wifi_tsk_run[MAX_WIFI_PATH] = {0}, is_wifi_tsk_running[MAX_WIFI_PATH] = {0};

#define PRI_IAMOVIE_WIFI             8
#define STKSIZE_IAMOVIE_WIFI         4096

THREAD_RETTYPE _ImageApp_MovieMulti_WifiTsk(void)
{
	VOS_TICK t1, t2, t_diff;
	THREAD_ENTRY();

	is_wifi_tsk_running[0] = TRUE;
	while (wifi_tsk_run[0]) {
		if (g_WifiCb == NULL) {
			DBG_ERR("wifi rawproc cb is not registered.\r\n");
			wifi_tsk_run[0] = FALSE;
			break;
		}
		vos_perf_mark(&t1);
		g_WifiCb();
		vos_perf_mark(&t2);
		t_diff = (t2 - t1) / 1000;
		if (t_diff > IAMOVIE_COMM_CB_TIMEOUT_CHECK) {
			DBG_WRN("ImageApp_MovieMulti WifiCB run %dms > %dms!\r\n", t_diff, IAMOVIE_COMM_CB_TIMEOUT_CHECK);
		}
	}
	is_wifi_tsk_running[0] = FALSE;

	THREAD_RETURN(0);
}

ER _ImageApp_MovieMulti_WifiLinkCfg(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;

	if ((i < _CFG_STRM_ID_1) || (i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_STRM_ID_1;

	// video encode
	WifiLink[idx].venc_i[0]       = HD_VIDEOENC_IN (0, MaxLinkInfo.MaxImgLink * 2 + idx);
	WifiLink[idx].venc_o[0]       = HD_VIDEOENC_OUT(0, MaxLinkInfo.MaxImgLink * 2 + idx);

	return E_OK;
}

ER _ImageApp_MovieMulti_WifiSetVEncParam(UINT32 idx)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
	VENDOR_VIDEOENC_H26X_XVR_CFG h26x_xvr = {
		.xvr_app = wifi_venc_xvr_app[idx],
	};
	if ((hd_ret = vendor_videoenc_set(WifiLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_H26X_XVR, &h26x_xvr)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_H26X_XVR) fail(%d)\n", hd_ret);
	}
#endif

	HD_VIDEOENC_FUNC_CONFIG wifi_venc_func_cfg = {
		.ddr_id  = dram_cfg.video_encode,
		.in_func = 0,
	};
	IACOMM_VENC_CFG venc_cfg = {
		.video_enc_path = WifiLink[idx].venc_p[0],
		.ppath_cfg      = &(wifi_venc_path_cfg[idx]),
		.pfunc_cfg      = &(wifi_venc_func_cfg),
	};
	if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
		DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	IACOMM_VENC_PARAM venc_param = {
		.video_enc_path = WifiLink[idx].venc_p[0],
		.pin            = &(wifi_venc_in_param[idx]),
		.pout           = &(wifi_venc_out_param[idx]),
		.prc            = &(wifi_venc_rc_param[idx]),
	};
	if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
		DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	HD_H26XENC_VUI vui_param = {
		.vui_en                     = wifi_venc_vui_disable[idx] ? FALSE : TRUE,                ///< enable vui. default: 0, range: 0~1 (0: disable, 1: enable)
		.sar_width                  = 0,    /// 0: Unspecified                                  ///< Horizontal size of the sample aspect ratio. default: 0, range: 0~65535
		.sar_height                 = 0,    /// 0: Unspecified                                  ///< Vertical size of the sample aspect rat. default: 0, range: 0~65535
		.matrix_coef                = 1,    /// 1: bt.709                                       ///< Matrix coefficients are used to derive the luma and Chroma signals from green, blue, and red primaries. default: 2, range: 0~255
		.transfer_characteristics   = 1,    /// 1: bt.709                                       ///< The opto-electronic transfers characteristic of the source pictures. default: 2, range: 0~255
		.colour_primaries           = 1,    /// 1: bt.709                                       ///< Chromaticity coordinates the source primaries. default: 2, range: 0~255
		.video_format               = 5,    /// 5: Unspecified video format                     ///< Indicate the representation of pictures. default: 5, range: 0~7
		.color_range                = wifi_venc_vui_color_tv_range[idx] ? FALSE : TRUE,         ///< Indicate the black level and range of the luma and Chroma signals. default: 0, range: 0~1 (0: Not full range, 1: Full range)
		.timing_present_flag        = 1,    /// 1: enabled
	};

	if (vui_param.vui_en && g_ia_moviemulti_usercb) {
		UINT32 vui_en = vui_param.vui_en;
		UINT32 color_range = vui_param.color_range;
		UINT32 id = _CFG_STRM_ID_1 + idx;
		g_ia_moviemulti_usercb(id, MOVIE_USER_CB_EVENT_SET_ENC_VUI, (UINT32)&vui_param);
		if ((vui_en != vui_param.vui_en) || color_range != vui_param.color_range) {
			DBG_WRN("id%d: vui_en(%d/%d) or color_range(%d/%d) has been changed\r\n", id, vui_en, vui_param.vui_en, color_range, vui_param.color_range);
		}
	}

	if ((hd_ret = hd_videoenc_set(WifiLink[idx].venc_p[0], HD_VIDEOENC_PARAM_OUT_VUI, &vui_param)) != HD_OK) {
		DBG_ERR("hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_VUI) fail(%d)\n", hd_ret);
	}

	VENDOR_VIDEOENC_LONG_START_CODE lsc = {0};
	lsc.long_start_code_en = ENABLE;
	if ((hd_ret = vendor_videoenc_set(WifiLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE, &lsc)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
	VENDOR_VIDEOENC_H26X_LOW_POWER_CFG h26x_low_power = {0};
	h26x_low_power.b_enable = wifi_venc_low_power_mode[idx];
	if ((hd_ret = vendor_videoenc_set(WifiLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_H26X_LOW_POWER, &h26x_low_power)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_H26X_LOW_POWER) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
#endif

	return ret;
}

ER _ImageApp_MovieMulti_WifiReopenVEnc(UINT32 idx)
{
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if (WifiLinkStatus[idx].venc_p[0] == 0) {
		if ((hd_ret = venc_close_ch(WifiLink[idx].venc_p[0])) != HD_OK) {
			DBG_ERR("venc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = venc_open_ch(WifiLink[idx].venc_i[0], WifiLink[idx].venc_o[0], &(WifiLink[idx].venc_p[0]))) != HD_OK) {
			DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		_ImageApp_MovieMulti_WifiSetVEncParam(idx);
	} else {
		ret = E_SYS;
	}
	return ret;
}

static ER _ImageApp_MovieMulti_WifiLink(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if ((i < _CFG_STRM_ID_1) || (i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_STRM_ID_1;

	if ((hd_ret = venc_open_ch(WifiLink[idx].venc_i[0], WifiLink[idx].venc_o[0], &(WifiLink[idx].venc_p[0]))) != HD_OK) {
		DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	_ImageApp_MovieMulti_WifiSetVEncParam(idx);

	return ret;
}

static ER _ImageApp_MovieMulti_WifiUnlink(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if ((i < _CFG_STRM_ID_1) || (i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_STRM_ID_1;

	if ((hd_ret = venc_close_ch(WifiLink[idx].venc_p[0])) != HD_OK) {
		DBG_ERR("venc_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	return ret;
}

static UINT32 _ImageApp_MovieMulti_UVACStartVideoCB(UVAC_VID_DEV_CNT vidDevIdx, UVAC_STRM_INFO *pStrmInfo)
{
	if (g_ia_moviemulti_usercb) {
		g_ia_moviemulti_usercb(0, MOVIE_USER_CB_EVENT_UVAC, ENABLE);
	}
	return E_OK;
}

static void _ImageApp_MovieMulti_UVACStopVideoCB(UVAC_VID_DEV_CNT vidDevIdx)
{
	if (g_ia_moviemulti_usercb) {
		g_ia_moviemulti_usercb(0, MOVIE_USER_CB_EVENT_UVAC, DISABLE);
	}
}

static UINT32 _ImageApp_MovieMulti_UVACSetVolumeCB(UINT32 volume)
{
	return 0;
}

static void _ImageApp_MovieMulti_UVACmemcpy(UINT32 uiDst, UINT32 uiSrc, UINT32 uiSize)
{
	hd_gfx_memcpy(uiDst, uiSrc, uiSize);
}

ER _ImageApp_MovieMulti_WifiLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction)
{
	UINT32 i = id, idx;
	ER ret = E_OK;

	if ((i < _CFG_STRM_ID_1) || (i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_STRM_ID_1;

	// set videoenc
	ret |= _LinkUpdateStatus(venc_set_state, WifiLink[idx].venc_p[0], &(WifiLinkStatus[idx].venc_p[0]), NULL);

	return ret;
}

ER _ImageApp_MovieMulti_WifiLinkOpen(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;
	HD_RESULT hd_ret;
	UINT32 pa;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

	if ((i < _CFG_STRM_ID_1) || (i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_STRM_ID_1;

	if (IsWifiLinkOpened[idx] == TRUE) {
		DBG_ERR("id%d is already opened.\r\n", i);
		return E_SYS;
	}

	_ImageApp_MovieMulti_WifiLinkCfg(i);
	_ImageApp_MovieMulti_SetupStrmParam(i);
	_ImageApp_MovieMulti_WifiLink(i);

	if (wifi_movie_uvac_func_en) {
		wifi_movie_uvac_size = UVAC_GetNeedMemSize() * 1;

		if ((hd_ret = hd_common_mem_alloc("movieuvac", &pa, (void **)&va, wifi_movie_uvac_size, ddr_id)) != HD_OK) {
			DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
			wifi_movie_uvac_va = 0;
			wifi_movie_uvac_pa = 0;
			wifi_movie_uvac_size = 0;
		} else {
			wifi_movie_uvac_va = (UINT32)va;
			wifi_movie_uvac_pa = (UINT32)pa;
		}

		gWifiMovieUvacInfo.UvacMemAdr = wifi_movie_uvac_pa;
		gWifiMovieUvacInfo.UvacMemSize = wifi_movie_uvac_size;
		gWifiMovieUvacInfo.channel = UVAC_CHANNEL_1V1A;
		gWifiMovieUvacInfo.fpStartVideoCB = _ImageApp_MovieMulti_UVACStartVideoCB;
		gWifiMovieUvacInfo.fpStopVideoCB = _ImageApp_MovieMulti_UVACStopVideoCB;
		gWifiMovieUvacInfo.fpSetVolCB = _ImageApp_MovieMulti_UVACSetVolumeCB;

		if (gpWifiMovieUvacVendDevDesc) {
			UVAC_SetConfig(UVAC_CONFIG_VEND_DEV_DESC, (UINT32)gpWifiMovieUvacVendDevDesc);
		}

		UVAC_SetConfig(UVAC_CONFIG_HW_COPY_CB, (UINT32)_ImageApp_MovieMulti_UVACmemcpy);

		if (UVAC_Open(&gWifiMovieUvacInfo) != E_OK) {
			DBG_ERR("MovieUVAC_Open fail\r\n");
		}
	}
	IsWifiLinkOpened[idx] = TRUE;

	return E_OK;
}

ER _ImageApp_MovieMulti_WifiLinkClose(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;

	if ((i < _CFG_STRM_ID_1) || (i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_STRM_ID_1;

	if (IsWifiLinkOpened[idx] == FALSE) {
		DBG_ERR("id%d is already closed.\r\n", i);
		return E_SYS;
	}
	if (IsWifiLinkStarted[idx] == TRUE) {
		ImageApp_MovieMulti_StreamingStop(i);
	}
	if (IsUvacLinkStarted[idx] == TRUE) {
		ImageApp_MovieMulti_UvacStop(i);
	}
	_ImageApp_MovieMulti_WifiUnlink(i);

	wifi_venc_vui_disable[idx] = 0;
	wifi_venc_vui_color_tv_range[idx] = 0;
	wifi_venc_max_frame_size[idx] = 0;
	wifi_venc_h264_profile[idx] = 0;
	wifi_venc_h264_level[idx] = 0;
	wifi_venc_h265_profile[idx] = 0;
	wifi_venc_h265_level[idx] = 0;
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
	wifi_venc_xvr_app[idx] = 0;
#endif
	wifi_venc_svc[idx] = 0;
	wifi_venc_low_power_mode[idx] = 0;
	wifi_venc_trigger_mode[idx] = MOVIE_CODEC_TRIGGER_DIRECT;
	memset((void *)&(wifi_venc_maxmem[idx]), 0, sizeof(HD_VIDEOENC_MAXMEM));

	IsWifiLinkOpened[idx] = FALSE;

	return E_OK;
}

ER ImageApp_MovieMulti_StreamingStart(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;
	NVTLIVE555_AUDIO_INFO aud_info;
	UINT32 need_set_flag;
	UINT32 keep_sem_flag = 0;
	UINT32 use_external_stream = FALSE;
	HD_RESULT hd_ret;

	if ((i < _CFG_STRM_ID_1) || (i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_STRM_ID_1;

	if (IsWifiLinkStarted[idx] == TRUE) {
		DBG_ERR("id%d is already started!\r\n", i);
		return E_SYS;
	}

	ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING, &use_external_stream);
	if (use_external_stream == FALSE) {
		iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << idx) | (FLGIACOMMON_AE_S0_BS_RDY << idx));
		live555_reset_Aqueue(idx);
		live555_reset_Vqueue(idx);
	}

	ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_KEEP_SEM_FLAG, &keep_sem_flag);
	if (keep_sem_flag) {
		vos_util_delay_ms(1000);
	}

	// step: start WifiLink
	// video path
	need_set_flag = _LinkCheckStatus(WifiLinkStatus[idx].venc_p[0]) ? FALSE : TRUE;
	_LinkPortCntInc(&(WifiLinkStatus[idx].venc_p[0]));
	_ImageApp_MovieMulti_WifiLinkStatusUpdate(i, UPDATE_REVERSE);

	// mmap buffer earlier
	if (use_external_stream == FALSE) {
		live555_mmap_venc_bs(idx, WifiLink[idx].venc_p[0]);
	}

	if (need_set_flag) {
		vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_VE_S0);
	}
	// audio path
	if ((WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_AAC) || (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ULAW) || (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
		_ImageApp_MovieMulti_AudRecStart(0);
		vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_AE_S0);
	} else if (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_PCM) {
		DBG_WRN("PCM is not supported in wifi streaming\r\n");
	}

	// mmap venc buffer
	if (wifi_venc_bs_buf_va[idx] == 0) {
		if ((hd_ret = hd_videoenc_get(WifiLink[idx].venc_p[0], HD_VIDEOENC_PARAM_BUFINFO, &(wifi_venc_bs_buf[idx]))) != HD_OK) {
			wifi_venc_bs_buf_va[idx] = 0;
			DBG_ERR("Get HD_VIDEOENC_PARAM_BUFINFO fail(%d)\r\n", hd_ret);
		} else {
			wifi_venc_bs_buf_va[idx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, wifi_venc_bs_buf[idx].buf_info.phy_addr, wifi_venc_bs_buf[idx].buf_info.buf_size);
		}
	}

	// step: start raw buffer pull task
	if (is_wifi_tsk_running[0] == FALSE) {
		wifi_tsk_run[idx] = TRUE;
		if ((iamovie_wifi_tsk_id = vos_task_create(_ImageApp_MovieMulti_WifiTsk, 0, "IAMovieWifiTsk", PRI_IAMOVIE_WIFI, STKSIZE_IAMOVIE_WIFI)) == 0) {
			DBG_ERR("IAMovieWifiTsk create failed.\r\n");
		} else {
			vos_task_resume(iamovie_wifi_tsk_id);
		}
	}

	// step: start live555
	if (use_external_stream == FALSE) {
		//live555_mmap_venc_bs(idx, WifiLink[idx].venc_p[0]);
		if (WifiLinkStrmInfo[idx].codec != _CFG_CODEC_MJPG) {
			live555_refresh_video_info(idx);
		}
		if (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_AAC) {
			aud_info.codec_type = NVTLIVE555_CODEC_AAC;
		} else if (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ULAW) {
			aud_info.codec_type = NVTLIVE555_CODEC_G711_ULAW;
		} else if (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ALAW) {
			aud_info.codec_type = NVTLIVE555_CODEC_G711_ALAW;
		} else if (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_PCM) {
			aud_info.codec_type = NVTLIVE555_CODEC_PCM;
		} else {
			aud_info.codec_type = NVTLIVE555_CODEC_UNKNOWN;
		}
		aud_info.sample_per_second = g_AudioInfo.aud_rate;
		aud_info.bit_per_sample = HD_AUDIO_BIT_WIDTH_16;
		aud_info.channel_cnt = (g_AudioInfo.aud_ch == _CFG_AUDIO_CH_STEREO) ? 2 : 1;
		if ((WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_AAC) || (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ULAW) || (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
			live555_mmap_aenc_bs(idx, AudCapLink[0].aenc_p[0]);
		}
		live555_set_audio_info(idx, &aud_info);
	}

	IsWifiLinkStarted[idx] = TRUE;

	return E_OK;
}

ER ImageApp_MovieMulti_StreamingStop(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, delay_cnt;
	FLGPTN uiFlag;
	ER ret;
	UINT32 keep_sem_flag = 0;
	UINT32 use_external_stream = FALSE;

	if ((i < _CFG_STRM_ID_1) || (i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_STRM_ID_1;

	if (IsWifiLinkOpened[idx] == FALSE) {
		DBG_ERR("WifiLink not opend.\r\n");
		return E_SYS;
	}
	if (IsWifiLinkStarted[idx] == FALSE) {
		DBG_ERR("WifiLink is not started!\r\n");
		return E_SYS;
	}
	ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_KEEP_SEM_FLAG, &keep_sem_flag);
	if (keep_sem_flag == FALSE) {
		vos_sem_wait(IACOMMON_SEMID_LOCK_VIDEO);
	}

	// step: stop raw buffer pull task (if UvacLinkStated, do not destroy task since it's a share task)
	if (IsUvacLinkStarted == FALSE) {
		wifi_tsk_run[idx] = FALSE;       // set this flag and task will return by itself, no need to destroy
		delay_cnt = 50;
		while (is_wifi_tsk_running[0] && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		if (is_wifi_tsk_running[0] == TRUE) {
			DBG_WRN("Destroy WifiTsk\r\n");
			vos_task_destroy(iamovie_wifi_tsk_id);
		}
	}

	// step: stop WifiLink
	_LinkPortCntDec(&(WifiLinkStatus[idx].venc_p[0]));
	ret = _ImageApp_MovieMulti_WifiLinkStatusUpdate(i, UPDATE_FORWARD);
	if (_LinkCheckStatus(WifiLinkStatus[idx].venc_p[0]) == FALSE) {
		vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_VE_S0);
	}
	// audio path
	if ((WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_AAC) || (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ULAW) || (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
		vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_AE_S0);
		_ImageApp_MovieMulti_AudRecStop(0);
	} else if (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_PCM) {
		DBG_WRN("PCM is not supported in wifi streaming\r\n");
	}

	// step: stop live555
	ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING, &use_external_stream);
	if (use_external_stream == FALSE) {
		iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << idx) | (FLGIACOMMON_AE_S0_BS_RDY << idx));
		iacomm_flag_wait(&uiFlag, IACOMMON_FLG_ID, ((FLGIACOMMON_VE_S0_RQUE_FREE | FLGIACOMMON_AE_S0_RQUE_FREE) << idx), TWF_ANDW);

		// step: clear all queue
		while (_QUEUE_DeQueueFromHead(&VBufferQueueReady[idx]) != NULL) {
		}
		while (_QUEUE_DeQueueFromHead(&ABufferQueueReady[idx]) != NULL) {
		}
		while (_QUEUE_DeQueueFromHead(&VBufferQueueClear[idx]) != NULL) {
		}
		while (_QUEUE_DeQueueFromHead(&ABufferQueueClear[idx]) != NULL) {
		}

		live555_munmap_venc_bs(idx);
		if ((WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_AAC) || (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ULAW) || (WifiLinkStrmInfo[idx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
			live555_munmap_aenc_bs(idx);
		}
	}

	// ummmap venc buffer
	if ((IsUvacLinkStarted[idx] == 0) && wifi_venc_bs_buf_va[idx]) {   // wifi & uvc shared the same mmap
		hd_common_mem_munmap((void *)wifi_venc_bs_buf_va[idx], wifi_venc_bs_buf[idx].buf_info.buf_size);
		wifi_venc_bs_buf_va[idx] = 0;
		memset(&(wifi_venc_bs_buf[idx]), 0, sizeof(HD_VIDEOENC_BUFINFO));
	}

	_ImageApp_MovieMulti_AudReopenAEnc(0);
	_ImageApp_MovieMulti_WifiReopenVEnc(idx);

	wifi_venc_timestamp[idx] = 0;

	IsWifiLinkStarted[idx] = FALSE;
	if (keep_sem_flag == FALSE) {
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
	}

	return ret;
}

ER ImageApp_MovieMulti_UvacStart(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;
	UINT32 need_set_flag;
	HD_RESULT hd_ret;

	if ((i < _CFG_UVAC_ID_1) || (i >= (_CFG_UVAC_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_UVAC_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_UVAC_ID_1;
	i = _CFG_STRM_ID_1 + idx;    // wifi and uvac share same link

	if (IsUvacLinkStarted[idx] == TRUE) {
		DBG_ERR("id%d is already started!\r\n", i);
		return E_SYS;
	}

	// step: start WifiLink
	// video path
	need_set_flag = _LinkCheckStatus(WifiLinkStatus[idx].venc_p[0]) ? FALSE : TRUE;
	_LinkPortCntInc(&(WifiLinkStatus[idx].venc_p[0]));
	_ImageApp_MovieMulti_WifiLinkStatusUpdate(i, UPDATE_REVERSE);
	if (need_set_flag) {
		vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_VE_S0);
	}
	// audio path
	if (WifiLinkStrmInfo[idx].aud_codec != _CFG_AUD_CODEC_NONE) {
		ImageApp_MovieMulti_AudCapStart(0);
		vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_U0);
	}

	// mmap venc buffer
	if (wifi_venc_bs_buf_va[idx] == 0) {
		if ((hd_ret = hd_videoenc_get(WifiLink[idx].venc_p[0], HD_VIDEOENC_PARAM_BUFINFO, &(wifi_venc_bs_buf[idx]))) != HD_OK) {
			wifi_venc_bs_buf_va[idx] = 0;
			DBG_ERR("Get HD_VIDEOENC_PARAM_BUFINFO fail(%d)\r\n", hd_ret);
		} else {
			wifi_venc_bs_buf_va[idx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, wifi_venc_bs_buf[idx].buf_info.phy_addr, wifi_venc_bs_buf[idx].buf_info.buf_size);
		}
	}

	// step: start raw buffer pull task
	if (is_wifi_tsk_running[0] == FALSE) {
		wifi_tsk_run[idx] = TRUE;
		if ((iamovie_wifi_tsk_id = vos_task_create(_ImageApp_MovieMulti_WifiTsk, 0, "IAMovieWifiTsk", PRI_IAMOVIE_WIFI, STKSIZE_IAMOVIE_WIFI)) == 0) {
			DBG_ERR("IAMovieWifiTsk create failed.\r\n");
		} else {
			vos_task_resume(iamovie_wifi_tsk_id);
		}
	}
	IsUvacLinkStarted[idx] = TRUE;

	return E_OK;
}

ER ImageApp_MovieMulti_UvacStop(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, delay_cnt;
	ER ret;

	if ((i < _CFG_UVAC_ID_1) || (i >= (_CFG_UVAC_ID_1 + MaxLinkInfo.MaxWifiLink)) || (i >= (_CFG_UVAC_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_UVAC_ID_1;
	i = _CFG_STRM_ID_1 + idx;    // wifi and uvac share same link

	if (IsWifiLinkOpened[idx] == FALSE) {
		DBG_ERR("WifiLink not opend.\r\n");
		return E_SYS;
	}
	if (IsUvacLinkStarted[idx] == FALSE) {
		DBG_ERR("UvacLink is not started!\r\n");
		return E_SYS;
	}

	// step: stop raw buffer pull task (if WifiLinkStated, do not destroy task since it's a share task)
	if (IsWifiLinkStarted == FALSE) {
		wifi_tsk_run[idx] = FALSE;       // set this flag and task will return by itself, no need to destroy
		delay_cnt = 50;
		while (is_wifi_tsk_running[0] && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		if (is_wifi_tsk_running[0] == TRUE) {
			DBG_WRN("Destroy WifiTsk\r\n");
			vos_task_destroy(iamovie_wifi_tsk_id);
		}
	}

	// step: stop WifiLink
	_LinkPortCntDec(&(WifiLinkStatus[idx].venc_p[0]));
	ret = _ImageApp_MovieMulti_WifiLinkStatusUpdate(i, UPDATE_FORWARD);
	if (_LinkCheckStatus(WifiLinkStatus[idx].venc_p[0]) == FALSE) {
		vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_VE_S0);
	}
	// audio path
	if (WifiLinkStrmInfo[idx].aud_codec != _CFG_AUD_CODEC_NONE) {
		vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_U0);
		ImageApp_MovieMulti_AudCapStop(0);
	}

	// ummmap venc buffer
	if ((IsWifiLinkStarted[idx] == 0) && wifi_venc_bs_buf_va[idx]) {   // wifi & uvc shared the same mmap
		hd_common_mem_munmap((void *)wifi_venc_bs_buf_va[idx], wifi_venc_bs_buf[idx].buf_info.buf_size);
		wifi_venc_bs_buf_va[idx] = 0;
		memset(&(wifi_venc_bs_buf[idx]), 0, sizeof(HD_VIDEOENC_BUFINFO));
	}

	_ImageApp_MovieMulti_WifiReopenVEnc(idx);

	wifi_venc_timestamp[idx] = 0;

	IsUvacLinkStarted[idx] = FALSE;

	return ret;
}

HD_RESULT ImageApp_MovieMulti_WifiPullOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 i = id, idx;
	HD_RESULT hd_ret;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return HD_ERR_NG;
	}
	idx = i - _CFG_REC_ID_1;

	if (img_vproc_no_ext[idx] == FALSE) {
		hd_ret = hd_videoproc_pull_out_buf(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_WIFI], p_video_frame, wait_ms);
	} else {
		UINT32 iport;
		if (gSwitchLink[idx][IAMOVIE_VPRC_EX_WIFI] == UNUSED) {
			return HD_ERR_NOT_AVAIL;
		}
		iport = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_WIFI]];
		if (iport > 2) {
			iport = 2;
		}
		hd_ret = hd_videoproc_pull_out_buf(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][iport], p_video_frame, wait_ms);
	}
	return hd_ret;
}

HD_RESULT ImageApp_MovieMulti_WifiPushIn(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 i = id, idx;

	if (i < _CFG_STRM_ID_1 || i >= (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink) || (i >= (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return HD_ERR_NG;
	}
	idx = i - _CFG_STRM_ID_1;

	return hd_videoenc_push_in_buf(WifiLink[idx].venc_p[0], p_video_frame, NULL, wait_ms);
}

HD_RESULT ImageApp_MovieMulti_WifiReleaseOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame)
{
	HD_RESULT hd_ret;
	UINT32 i = id, idx;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return HD_ERR_NG;
	}
	idx = i - _CFG_REC_ID_1;

	if (p_img_to_imgcap_frame && (idx == img_to_imgcap_id) && (img_to_imgcap_path == IAMOVIE_VPRC_EX_WIFI)) {
		memcpy((void *)p_img_to_imgcap_frame, (void*)p_video_frame, sizeof(HD_VIDEO_FRAME));
		p_img_to_imgcap_frame = NULL;
		img_to_imgcap_id = UNUSED;
		img_to_imgcap_path = UNUSED;
		vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_FRM_WAIT);
		hd_ret = HD_OK;
	} else {
		if (img_vproc_no_ext[idx] == FALSE) {
			hd_ret = hd_videoproc_release_out_buf(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_WIFI], p_video_frame);
		} else {
			UINT32 iport;
			if (gSwitchLink[idx][IAMOVIE_VPRC_EX_WIFI] == UNUSED) {
				return HD_ERR_NOT_AVAIL;
			}
			iport = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_WIFI]];
			if (iport > 2) {
				iport = 2;
			}
			hd_ret = hd_videoproc_release_out_buf(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][iport], p_video_frame);
		}
	}
	return hd_ret;
}

