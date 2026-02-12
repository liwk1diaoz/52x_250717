#include "ImageApp_MovieMulti_int.h"

/// ========== Cross file variables ==========
MOVIEMULTI_AUDIO_INFO g_AudioInfo = {0};
UINT32 IsAudCapLinkOpened[MAX_AUDCAP_PATH] = {0};
MOVIE_AUDCAP_LINK AudCapLink[MAX_AUDCAP_PATH];
MOVIE_AUDCAP_LINK_STATUS AudCapLinkStatus[MAX_AUDCAP_PATH] = {0};
HD_AUDIOCAP_DEV_CONFIG aud_acap_dev_cfg[MAX_AUDCAP_PATH] = {0};
HD_AUDIOCAP_DRV_CONFIG aud_acap_drv_cfg[MAX_AUDCAP_PATH] = {0};
HD_AUDIOCAP_IN aud_acap_param[MAX_AUDCAP_PATH] = {0};
HD_AUDIOCAP_ANR aud_acap_anr_param[MAX_AUDCAP_PATH] = {0};
HD_AUDIOCAP_AEC aud_acap_aec_param[MAX_AUDCAP_PATH] = {0};
HD_AUDIOENC_PATH_CONFIG2 aud_aenc_cfg[MAX_AUDCAP_PATH] = {0};
HD_AUDIOENC_IN aud_aenc_in_param[MAX_AUDCAP_PATH] = {0};
HD_AUDIOENC_OUT aud_aenc_out_param[MAX_AUDCAP_PATH] = {0};
HD_AUDIOCAP_BUFINFO aud_acap_bs_buf[MAX_AUDCAP_PATH] = {0};
HD_AUDIOENC_BUFINFO aud_aenc_bs_buf[MAX_AUDCAP_PATH] = {0};
UINT32 aud_aenc_bs_buf_va[MAX_AUDCAP_PATH] = {0};
UINT32 AudCapBufMs[MAX_AUDCAP_PATH] = {0};
UINT32 AudEncBufMs[MAX_AUDCAP_PATH] = {10000};
MOVIE_CFG_AUD_CODEC AudEncCodec[MAX_AUDCAP_PATH] = {_CFG_AUD_CODEC_AAC};
UINT32 aud_cap_volume = 100;
UINT32 aud_cap_alc_en = TRUE;
UINT32 aud_cap_prepwr_en = TRUE;
// Note for mute encode function
// if aud_mute_enc_func == TRUE , aud_mute_enc = TRUE will mute from aenc (need extra buffer)
// if aud_mute_enc_func == FALSE, aud_mute_enc = TRUE will mute from acap
BOOL aud_mute_enc_func = FALSE;
BOOL aud_mute_enc = FALSE;
UINT32 aud_mute_enc_buf_pa = 0, aud_mute_enc_buf_va = 0, aud_mute_enc_buf_size = 0;
MovieAudCapCb  *aud_cap_cb = NULL;
BOOL aud_acap_by_hdal = TRUE;
#if (USE_ALSA_LIB == ENABLE)
snd_pcm_t *alsa_hdl = NULL;
UINT32 alsabuf_pa = 0, alsabuf_va = 0, alsabuf_size = 0;
#endif
HD_AUDIOCAP_BUFINFO aud_acap_bs_buf_info = {0};
UINT32 aud_acap_frame_va = 0;
/// ========== Noncross file variables ==========
static UINT8 *p_aud_cap_frame_buf;
static UINT32 *p_aud_cap_frame_len;
#if (USE_ALSA_LIB == ENABLE)
#if (ALSA_DEBUG == ENABLE)
static snd_output_t *log;
#endif
#endif

ER _ImageApp_MovieMulti_AudCapLinkCfg(MOVIE_CFG_REC_ID id)
{
	UINT32 idx;

	idx = 0;

	// video capture
	if (aud_acap_by_hdal) {
		AudCapLink[idx].acap_ctrl   = HD_AUDIOCAP_CTRL(idx);
		AudCapLink[idx].acap_i[0]   = HD_AUDIOCAP_IN (idx, 0);
		AudCapLink[idx].acap_o[0]   = HD_AUDIOCAP_OUT(idx, 0);
	}
	// video encode
	AudCapLink[idx].aenc_i[0]   = HD_AUDIOENC_IN (0, 0 + idx);
	AudCapLink[idx].aenc_o[0]   = HD_AUDIOENC_OUT(0, 0 + idx);

	return E_OK;
}

ER _ImageApp_MovieMulti_AudSetAEncParam(UINT32 idx)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	IACOMM_AENC_CFG aenc_cfg = {
		.audio_enc_path = AudCapLink[idx].aenc_p[0],
		.pcfg           = &(aud_aenc_cfg[idx]),
	};
	if ((hd_ret = aenc_set_cfg(&aenc_cfg)) != HD_OK) {
		DBG_ERR("aenc_set_cfg fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	IACOMM_AENC_PARAM aenc_param = {
		.audio_enc_path = AudCapLink[idx].aenc_p[0],
		.pin_param      = &(aud_aenc_in_param[idx]),
		.pout_param     = &(aud_aenc_out_param[idx]),
	};
	if ((hd_ret = aenc_set_param(&aenc_param)) != HD_OK) {
		DBG_ERR("aenc_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	VENDOR_AUDIOENC_BS_RESERVED_SIZE_CFG aud_reserved_size = {0};
	if ((ImgLinkRecInfo[0][0].file_format == _CFG_FILE_FORMAT_TS) || (EthCamLinkRecInfo[idx].rec_format == _CFG_FILE_FORMAT_TS)) {
		aud_reserved_size.reserved_size = AENC_BS_RESERVED_SIZE_TS;
	} else {
		aud_reserved_size.reserved_size = AENC_BS_RESERVED_SIZE_MP4;
	}
	if ((hd_ret = vendor_audioenc_set(AudCapLink[0].aenc_p[0], VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE, &aud_reserved_size)) != HD_OK) {
		DBG_ERR("vendor_audioenc_set(VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE) fail(%d)\r\n", hd_ret);
	}
	return ret;
}

ER _ImageApp_MovieMulti_AudReopenAEnc(UINT32 idx)
{
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if (AudCapLinkStatus[idx].aenc_p[0] == 0) {
		if ((hd_ret = aenc_close_ch(AudCapLink[idx].aenc_p[0])) != HD_OK) {
			DBG_ERR("aenc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = aenc_open_ch(AudCapLink[idx].aenc_i[0], AudCapLink[idx].aenc_o[0], &(AudCapLink[idx].aenc_p[0]))) != HD_OK) {
			DBG_ERR("aenc_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		_ImageApp_MovieMulti_AudSetAEncParam(idx);
		if (aud_aenc_bs_buf_va[idx]) {
			hd_common_mem_munmap((void *)aud_aenc_bs_buf_va[idx], aud_aenc_bs_buf[idx].buf_info.buf_size);
			aud_aenc_bs_buf_va[idx] = 0;
		}
		aud_aenc_bs_buf[idx].buf_info.phy_addr = 0;
		aud_aenc_bs_buf[idx].buf_info.buf_size = 0;
	}
	return ret;
}

static ER _ImageApp_MovieMulti_AudCapLink(MOVIE_CFG_REC_ID id)
{
	UINT32 idx;
	ER ret = E_OK;
	HD_RESULT hd_ret;
	HD_AUDIOCAP_VOLUME audio_cap_volume = {0};
	UINT32 pa;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id;
#if (USE_ALSA_LIB == ENABLE)
	snd_pcm_hw_params_t *params = NULL;
	snd_pcm_uframes_t frames;
	UINT32 val;
#endif

	idx = 0;

	// set audiocap
	if (aud_acap_by_hdal) {
		IACOMM_ACAP_CFG acap_cfg = {
			.p_audio_cap_ctrl = &(AudCapLink[idx].acap_p_ctrl),
			.ctrl_id          = AudCapLink[idx].acap_ctrl,
			.pdev_cfg         = &(aud_acap_dev_cfg[idx]),
			.pdrv_cfg         = &(aud_acap_drv_cfg[idx]),
		};
		if ((hd_ret = acap_set_cfg(&acap_cfg)) != HD_OK) {
			DBG_ERR("acap_set_cfg fail(%d)\r\n", hd_ret);
			ret = E_SYS;
		}
		// set VENDOR_AUDIOCAP_ITEM_PREPWR_ENABLE before open
		if (img_flow_for_dumped_fast_boot == FALSE) {
			if ((hd_ret = vendor_audiocap_set(AudCapLink[0].acap_p_ctrl, VENDOR_AUDIOCAP_ITEM_PREPWR_ENABLE, &aud_cap_prepwr_en)) != HD_OK) {
				DBG_ERR("vendor_audiocap_set(VENDOR_AUDIOCAP_ITEM_PREPWR_ENABLE, %d) fail(%d)\n", aud_cap_prepwr_en, hd_ret);
			}
		}
		if ((hd_ret = acap_open_ch(AudCapLink[idx].acap_i[0], AudCapLink[idx].acap_o[0], &(AudCapLink[idx].acap_p[0]))) != HD_OK) {
			DBG_ERR("acap_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		IACOMM_ACAP_PARAM acap_param = {
			.audio_cap_path = AudCapLink[idx].acap_p[0],
			.pcap_param     = &(aud_acap_param[idx]),
		};
		if ((hd_ret = acap_set_param(&acap_param)) != HD_OK) {
			DBG_ERR("acap_set_param fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if (aud_mute_enc_func == TRUE) {        // use mute pcm buffer to encode, do not adjust acap
			audio_cap_volume.volume = aud_cap_volume;
		} else {
			audio_cap_volume.volume = (aud_mute_enc == FALSE) ? aud_cap_volume : 0;
		}
		if ((hd_ret = hd_audiocap_set(AudCapLink[0].acap_p_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &audio_cap_volume)) != HD_OK) {
			DBG_ERR("Set HD_AUDIOCAP_PARAM_VOLUME fail(%d)\r\n", hd_ret);
		}
		if ((hd_ret = vendor_audiocap_set(AudCapLink[idx].acap_p_ctrl, VENDOR_AUDIOCAP_ITEM_ALC_ENABLE, (VOID *)&aud_cap_alc_en)) != HD_OK) {
			DBG_ERR("Set VENDOR_AUDIOCAP_ITEM_ALC_ENABLE(%d) fail(%d)\r\n", aud_cap_alc_en, hd_ret);
		}
		if (aud_acap_anr_param[0].enabled == TRUE) {
			if ((hd_ret = hd_audiocap_set(AudCapLink[idx].acap_p[0], HD_AUDIOCAP_PARAM_OUT_ANR, &(aud_acap_anr_param[0]))) != HD_OK) {
				DBG_ERR("Set HD_AUDIOCAP_PARAM_OUT_ANR fail(%d)\r\n", hd_ret);
			}
		}
		if (aud_acap_aec_param[0].enabled == TRUE) {
			if ((hd_ret = hd_audiocap_set(AudCapLink[idx].acap_p[0], HD_AUDIOCAP_PARAM_OUT_AEC, &(aud_acap_aec_param[0]))) != HD_OK) {
				DBG_ERR("Set HD_AUDIOCAP_PARAM_OUT_AEC fail(%d)\r\n", hd_ret);
			}
		}

		//if ((hd_ret = acap_bind(AudCapLink[idx].acap_o[0], AudCapLink[idx].aenc_i[0])) != HD_OK) {
		//	DBG_ERR("acap_bind fail(%d)\n", hd_ret);
		//	ret = E_SYS;
		//}

#if !defined(_BSP_NA51055_)
		UINT32 mono_expand;
		if (((g_AudioInfo.aud_ch == _CFG_AUDIO_CH_LEFT) || (g_AudioInfo.aud_ch == _CFG_AUDIO_CH_RIGHT)) && g_AudioInfo.aud_ch_num == 2) {
			mono_expand = TRUE;
		} else {
			mono_expand = FALSE;
		}
		if ((hd_ret = vendor_audiocap_set(AudCapLink[0].acap_p_ctrl, VENDOR_AUDIOCAP_ITEM_MONO_EXPAND, &mono_expand)) != HD_OK) {
			DBG_ERR("vendor_audiocap_set(VENDOR_AUDIOCAP_ITEM_MONO_EXPAND, %d) fail(%d)\n", mono_expand, hd_ret);
		}
#endif

		if ((hd_ret = hd_audiocap_start(AudCapLink[idx].acap_p[0])) != HD_OK) {
			DBG_ERR("hd_audioenc_start fail(%d)\n", hd_ret);
		}
		if ((hd_ret = hd_audiocap_get(AudCapLink[idx].acap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &(aud_acap_bs_buf[idx]))) != HD_OK) {
			DBG_ERR("hd_audiocap_get(HD_AUDIOCAP_PARAM_BUFINFO) fail(%d)\n", hd_ret);
		}
		if ((hd_ret = hd_audiocap_stop(AudCapLink[idx].acap_p[0])) != HD_OK) {
			DBG_ERR("hd_audiocap_stop fail(%d)\n", hd_ret);
		}
	} else {
#if (USE_ALSA_LIB == ENABLE)
#if (ALSA_DEBUG == ENABLE)
		if ((ret = snd_output_stdio_attach(&log, stdout, 0)) != 0) {
			DBG_ERR("snd_output_stdio_attach() failed(%d)\r\n", ret);
		}
#endif
		if ((ret = snd_pcm_open(&alsa_hdl, "hw:0,0", SND_PCM_STREAM_CAPTURE, 0)) != 0) {
			DBG_ERR("snd_pcm_open() failed(%d)\r\n", ret);
		} else {
			// allocate  hardware parameters object
			snd_pcm_hw_params_alloca(&params);
			// fill it in with default values
			if ((ret = snd_pcm_hw_params_any(alsa_hdl, params)) != 0) {
				DBG_ERR("snd_pcm_hw_params_any() failed(%d)", ret);
			}

#if (ALSA_DEBUG == ENABLE)
			if ((ret = snd_pcm_hw_params_dump(params, log)) != 0) {
				DBG_ERR("snd_pcm_hw_params_dump() failed(%d)", ret);
			}
#endif

			// set the desired hardware parameters
			// interleaved mode
			if ((ret = snd_pcm_hw_params_set_access(alsa_hdl, params, SND_PCM_ACCESS_RW_INTERLEAVED)) != 0) {
				DBG_ERR("snd_pcm_hw_params_set_access() failed(%d)\r\n", ret);
			}
			// 16-bit LE format
			if ((ret = snd_pcm_hw_params_set_format(alsa_hdl, params, SND_PCM_FORMAT_S16_LE)) != 0) {
				DBG_ERR("snd_pcm_hw_params_set_format() failed(%d)\r\n", ret);
			}
			// channel no
			val = g_AudioInfo.aud_ch_num;
			if ((ret = snd_pcm_hw_params_set_channels(alsa_hdl, params, val)) != 0) {
				DBG_ERR("snd_pcm_hw_params_set_channels() failed(%d)\r\n", ret);
			}
			// SR
			val = g_AudioInfo.aud_rate;
			if ((ret = snd_pcm_hw_params_set_rate_near(alsa_hdl, params, (unsigned int *)&val, (int *)0)) != 0) {
				DBG_ERR("snd_pcm_hw_params_set_rate_near() failed(%d)\r\n", ret);
			}
			// period size
			frames = 1024;
			if ((ret = snd_pcm_hw_params_set_period_size_near(alsa_hdl, params, &frames, (int *)0)) != 0) {
				DBG_ERR("snd_pcm_hw_params_set_period_size_near() failed(%d)\r\n", ret);
			}
			// write the parameters to driver
			if ((ret = snd_pcm_hw_params(alsa_hdl, params)) != 0) {
				DBG_ERR("snd_pcm_hw_params() failed(%d)\r\n", ret);
			}
#if (ALSA_DEBUG == ENABLE)
			if ((ret = snd_pcm_dump(alsa_hdl, log)) != 0) {
				DBG_ERR("snd_pcm_dump() failed(%d)\r\n", ret);
			}
#endif

			ddr_id = DDR_ID0;
			alsabuf_size = frames * g_AudioInfo.aud_ch_num * 2 * 10;

			if ((hd_ret = hd_common_mem_alloc("alsabuf", &pa, (void **)&va, alsabuf_size, ddr_id)) != HD_OK) {
				DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
				alsabuf_va = 0;
				alsabuf_pa = 0;
				alsabuf_size = 0;
			} else {
				alsabuf_va = (UINT32)va;
				alsabuf_pa = (UINT32)pa;
			}
		}
#endif
	}

	// set audioenc
	if ((hd_ret = aenc_open_ch(AudCapLink[idx].aenc_i[0], AudCapLink[idx].aenc_o[0], &(AudCapLink[idx].aenc_p[0]))) != HD_OK) {
		DBG_ERR("aenc_open_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	_ImageApp_MovieMulti_AudSetAEncParam(idx);

	if (aud_mute_enc_func) {
		ddr_id = DDR_ID0;
		aud_mute_enc_buf_size = g_AudioInfo.aud_ch_num * 2 * 1024;

		if ((hd_ret = hd_common_mem_alloc("aencmutebuf", &pa, (void **)&va, aud_mute_enc_buf_size, ddr_id)) != HD_OK) {
			DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
			aud_mute_enc_buf_va = 0;
			aud_mute_enc_buf_pa = 0;
			aud_mute_enc_buf_size = 0;
		} else {
			aud_mute_enc_buf_va = (UINT32)va;
			aud_mute_enc_buf_pa = (UINT32)pa;
			memset((void *)aud_mute_enc_buf_va, 0, aud_mute_enc_buf_size);
		}
	}

	return ret;
}

static ER _ImageApp_MovieMulti_AudCapUnlink(MOVIE_CFG_REC_ID id)
{
	UINT32 idx;
	ER ret = E_OK;
	HD_RESULT hd_ret;

	idx = 0;

	// set audiocap
	if (aud_acap_by_hdal) {
		//if ((ret = acap_unbind(AudCapLink[idx].acap_o[0])) != HD_OK) {
		//	DBG_ERR("acap_unbind fail(%d)\n", ret);
		//	result = E_SYS;
		//}
		if ((hd_ret = acap_close_ch(AudCapLink[idx].acap_p[0])) != HD_OK) {
			DBG_ERR("acap_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		memset(&(aud_acap_bs_buf[0]), 0, sizeof(aud_acap_bs_buf));
		AudCapBufMs[idx] = 0;

	} else {
#if (USE_ALSA_LIB == ENABLE)
		if ((ret = snd_pcm_drain(alsa_hdl)) != 0) {
			DBG_ERR("snd_pcm_drain() failed(%d)\r\n", ret);
		}
		if ((ret = snd_pcm_close(alsa_hdl)) != 0) {
			DBG_ERR("snd_pcm_close() failed(%d)\r\n", ret);
		}
#if (ALSA_DEBUG == ENABLE)
		if ((ret = snd_output_close(log)) != 0) {
			DBG_ERR("snd_output_close() failed(%d)\r\n", ret);
		}
#endif
		if ((ret = snd_config_update_free_global()) != 0) {
			DBG_ERR("snd_config_update_free_global() failed(%d)\r\n", ret);
		}

		if (alsabuf_size) {
			if ((hd_ret = hd_common_mem_free(alsabuf_pa, (void *)alsabuf_va)) != HD_OK) {
				DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
			}
			alsabuf_pa = 0;
			alsabuf_va = 0;
			alsabuf_size = 0;
		}
#endif
	}

	// set audioenc
	if ((hd_ret = aenc_close_ch(AudCapLink[idx].aenc_p[0])) != HD_OK) {
		DBG_ERR("aenc_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	if (aud_mute_enc_func) {
		if (aud_mute_enc_buf_size) {
			if ((hd_ret = hd_common_mem_free(aud_mute_enc_buf_pa, (void *)aud_mute_enc_buf_va)) != HD_OK) {
				DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
			}
			aud_mute_enc_buf_pa = 0;
			aud_mute_enc_buf_va = 0;
			aud_mute_enc_buf_size = 0;
		}
	}
	if (aud_acap_frame_va) {
		hd_common_mem_munmap((void *)aud_acap_frame_va, aud_acap_bs_buf_info.buf_info.buf_size);
		aud_acap_frame_va = 0;
	}

	return ret;
}

static void acap_status_cb(HD_PATH_ID path_id, UINT32 state)
{
	FLGPTN vflag;

	if (aud_acap_by_hdal) {
		if (state == CB_STATE_AFTER_RUN) {
			vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_PCM);
		} else if (state == CB_STATE_BEFORE_STOP) {
			vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_PCM);
			vos_flag_wait_timeout(&vflag, IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_AC_IDLE, TWF_ORW, vos_util_msec_to_tick(100));
		} else if (state == CB_STATE_AFTER_STOP) {
			if (aud_acap_frame_va) {
				hd_common_mem_munmap((void *)aud_acap_frame_va, aud_acap_bs_buf_info.buf_info.buf_size);
				aud_acap_frame_va = 0;
			}
		}
	}
}

static void aenc_status_cb(HD_PATH_ID path_id, UINT32 state)
{
	FLGPTN vflag;

	if (state == CB_STATE_AFTER_RUN) {
		vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_AE_AAC);
	} else if (state == CB_STATE_BEFORE_STOP) {
		vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_AE_AAC);
		vos_flag_wait_timeout(&vflag, IAMOVIE_FLG_ID, (FLGIAMOVIE_TSK_AC_IDLE | FLGIAMOVIE_TSK_AE_IDLE), TWF_ORW, vos_util_msec_to_tick(100));
	}
}

ER _ImageApp_MovieMulti_AudCapLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction)
{
	UINT32 idx;
	ER ret = E_OK;

	idx = 0;

	if (direction == UPDATE_FORWARD) {
		// set audiocap
		if (aud_acap_by_hdal) {
			ret |= _LinkUpdateStatus(acap_set_state, AudCapLink[idx].acap_p[0], &(AudCapLinkStatus[idx].acap_p[0]), acap_status_cb);
		}
		// set audioenc
		ret |= _LinkUpdateStatus(aenc_set_state, AudCapLink[idx].aenc_p[0], &(AudCapLinkStatus[idx].aenc_p[0]), aenc_status_cb);
	} else {
		// set audioenc
		ret |= _LinkUpdateStatus(aenc_set_state, AudCapLink[idx].aenc_p[0], &(AudCapLinkStatus[idx].aenc_p[0]), aenc_status_cb);
		// set audiocap
		if (aud_acap_by_hdal) {
			ret |= _LinkUpdateStatus(acap_set_state, AudCapLink[idx].acap_p[0], &(AudCapLinkStatus[idx].acap_p[0]), acap_status_cb);
		}
	}
	return ret;
}

ER _ImageApp_MovieMulti_AudCapLinkOpen(MOVIE_CFG_REC_ID id)
{
	UINT32 i = 0, idx = 0;

	if (IsAudCapLinkOpened[idx] == TRUE) {
		DBG_ERR("AudCap[%d] is already opened.\r\n", idx);
		return E_SYS;
	}
	vos_sem_wait(IAMOVIE_SEMID_ACAP);
	_ImageApp_MovieMulti_AudCapLinkCfg(i);
	_ImageApp_MovieMulti_AudCapLink(i);
	vos_sem_sig(IAMOVIE_SEMID_ACAP);
	IsAudCapLinkOpened[idx] = TRUE;

	return E_OK;
}

ER _ImageApp_MovieMulti_AudCapLinkClose(MOVIE_CFG_REC_ID id)
{
	UINT32 i = 0, idx = 0;

	if (IsAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("AudCap[%d] is already closed.\r\n", idx);
		return E_SYS;
	}

	vos_sem_wait(IAMOVIE_SEMID_ACAP);
	while (AudCapLinkStatus[idx].aenc_p[0]) {
		_ImageApp_MovieMulti_AudRecStop(i);
	}

	_ImageApp_MovieMulti_AudCapUnlink(i);
	IsAudCapLinkOpened[idx] = FALSE;

	// reset variables
	memset(&(aud_acap_anr_param[idx]), 0, sizeof(HD_AUDIOCAP_ANR));
	memset(&(aud_acap_aec_param[idx]), 0, sizeof(HD_AUDIOCAP_AEC));
	if (aud_aenc_bs_buf_va[idx]) {
		hd_common_mem_munmap((void *)aud_aenc_bs_buf_va[idx], aud_aenc_bs_buf[idx].buf_info.buf_size);
		aud_aenc_bs_buf_va[idx] = 0;
	}
	memset(&(aud_aenc_bs_buf[idx]), 0, sizeof(HD_AUDIOENC_BUFINFO));
	AudEncCodec[idx] = _CFG_AUD_CODEC_AAC;
	vos_sem_sig(IAMOVIE_SEMID_ACAP);

	return E_OK;
}

ER _ImageApp_MovieMulti_AudRecStart(MOVIE_CFG_REC_ID id)
{
	ER ret;
	UINT32 i = 0, idx = 0;

	if (IsAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("AudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}
	if (aud_acap_by_hdal) {
		_LinkPortCntInc(&(AudCapLinkStatus[idx].acap_p[0]));
	}
	_LinkPortCntInc(&(AudCapLinkStatus[idx].aenc_p[0]));
	ret = _ImageApp_MovieMulti_AudCapLinkStatusUpdate(i, UPDATE_REVERSE);

	return ret;
}

ER _ImageApp_MovieMulti_AudRecStop(MOVIE_CFG_REC_ID id)
{
	ER ret;
	UINT32 i = 0, idx = 0;

	if (IsAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("AudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}
	if (aud_acap_by_hdal) {
		_LinkPortCntDec(&(AudCapLinkStatus[idx].acap_p[0]));
	}
	_LinkPortCntDec(&(AudCapLinkStatus[idx].aenc_p[0]));
	ret = _ImageApp_MovieMulti_AudCapLinkStatusUpdate(i, UPDATE_FORWARD);

	return ret;
}

void _ImageApp_MovieMulti_AudCapFrameProc(HD_AUDIO_FRAME *pdata)
{
	UINT32 idx = 0;
	UINT32 bs_va;
	VOS_TICK t1, t2, t_diff;

	if (aud_acap_frame_va == 0) {
		hd_audiocap_get(AudCapLink[idx].acap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &(aud_acap_bs_buf_info));
		aud_acap_frame_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, aud_acap_bs_buf_info.buf_info.phy_addr, aud_acap_bs_buf_info.buf_info.buf_size);
	}
	bs_va = aud_acap_frame_va + pdata->phy_addr[0] - aud_acap_bs_buf_info.buf_info.phy_addr;

	if (aud_cap_cb) {
		vos_perf_mark(&t1);
		aud_cap_cb(pdata->phy_addr[0], bs_va, pdata->size);
		vos_perf_mark(&t2);
		t_diff = (t2 - t1) / 1000;
		if (t_diff > IAMOVIE_COMM_CB_TIMEOUT_CHECK) {
			DBG_WRN("ImageApp_MovieMulti AudCapCB run %dms > %dms!\r\n", t_diff, IAMOVIE_COMM_CB_TIMEOUT_CHECK);
		}
	}

	if (vos_flag_chk(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_GET)) {
		if (pdata->size > *p_aud_cap_frame_len) {
			DBG_WRN("AcapGet buf not enough, need %d bug give %d\r\n", pdata->size, *p_aud_cap_frame_len);
		} else {
			*p_aud_cap_frame_len = pdata->size;
		}
		memcpy((void *)p_aud_cap_frame_buf, (void *)bs_va, *p_aud_cap_frame_len);
		vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_GET);
		vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_GET_DONE);
	}
}

ER ImageApp_MovieMulti_AudCapStart(MOVIE_CFG_REC_ID id)
{
	ER ret;
	UINT32 i = 0, idx = 0;

	if (IsAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("AudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}
	if (aud_acap_by_hdal) {
		_LinkPortCntInc(&(AudCapLinkStatus[idx].acap_p[0]));
	}
	ret = _ImageApp_MovieMulti_AudCapLinkStatusUpdate(i, UPDATE_REVERSE);

	return ret;
}

ER ImageApp_MovieMulti_AudCapStop(MOVIE_CFG_REC_ID id)
{
	ER ret;
	UINT32 i = 0, idx = 0;

	if (IsAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("AudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}
	if (aud_acap_by_hdal) {
		_LinkPortCntDec(&(AudCapLinkStatus[idx].acap_p[0]));
	}
	ret = _ImageApp_MovieMulti_AudCapLinkStatusUpdate(i, UPDATE_FORWARD);

	return ret;
}

ER ImageApp_MovieMulti_AudCapGetFrame(MOVIE_CFG_REC_ID id, UINT8 *p_buf, UINT32 *p_len, INT32 wait_ms)
{
	ER ret;
	FLGPTN uiFlag;
	UINT32 idx = 0;

	if (IsAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("AudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}

	if (p_buf == NULL || p_len == NULL) {
		DBG_ERR("buf(0x%x) or len(0x%x)pointer is NULL\r\n", (UINT32)p_buf, (UINT32)p_len);
		return E_SYS;
	}

	if (aud_acap_by_hdal) {
		if (AudCapLinkStatus[idx].acap_p[0]) {
			p_aud_cap_frame_buf = p_buf;
			p_aud_cap_frame_len = p_len;
			vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_GET);
			if (wait_ms == -1) {
				ret = vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_AC_GET_DONE, TWF_ORW);
			} else {
				ret = vos_flag_wait_timeout(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_AC_GET_DONE, TWF_ORW, vos_util_msec_to_tick(wait_ms));
			}
			vos_flag_clr(IAMOVIE_FLG_ID, (FLGIAMOVIE_AC_GET | FLGIAMOVIE_AC_GET_DONE));
		} else {
			DBG_ERR("AudCap[%d] is not start.\r\n", idx);
			ret = E_SYS;
		}
	} else {
		// TODO
		ret = E_NOSPT;
	}
	return ret;
}

ER ImageApp_MovieMulti_AudTranscodeStart(MOVIE_CFG_REC_ID id)
{
	ER ret;
	UINT32 i = 0, idx = 0;

	if (IsAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("AudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}
	_LinkPortCntInc(&(AudCapLinkStatus[idx].aenc_p[0]));
	ret = _ImageApp_MovieMulti_AudCapLinkStatusUpdate(i, UPDATE_REVERSE);

	return ret;
}

ER ImageApp_MovieMulti_AudTranscodeStop(MOVIE_CFG_REC_ID id)
{
	ER ret;
	UINT32 i = 0, idx = 0;

	if (IsAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("AudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}
	_LinkPortCntDec(&(AudCapLinkStatus[idx].aenc_p[0]));
	ret = _ImageApp_MovieMulti_AudCapLinkStatusUpdate(i, UPDATE_FORWARD);

	return ret;
}

