#include "ImageApp_MovieMulti_int.h"

///// Cross file variables
ID IAMOVIE_FLG_ID = 0, IAMOVIE_FLG_ID2 = 0, IAMOVIE_FLG_BSMUX_ID = 0;
ID IAMOVIE_FLG_BG_ID = 0;
ID IAMOVIE_SEMID_FM = 0;
ID IAMOVIE_SEMID_CRYPTO = 0;
ID IAMOVIE_SEMID_OPERATION = 0;
ID IAMOVIE_SEMID_ACAP = 0;
MovieAudBsCb  *aud_bs_cb[MAX_AUDCAP_PATH] = {0};
MovieVdoBsCb  *vdo_bs_rec_cb[MAX_IMG_PATH][2] = {0};
MovieVdoBsCb  *vdo_bs_strm_cb[MAX_WIFI_PATH] = {0};
MovieVdoBsCb  *vdo_bs_ethcam_cb[MAX_ETHCAM_PATH] = {0};

///// Noncross file variables
static THREAD_HANDLE iamovie_acpull_tsk_id = 0;
static THREAD_HANDLE iamovie_aepull_tsk_id = 0;
static THREAD_HANDLE iamovie_vepull_tsk_id = 0;
static THREAD_HANDLE iamovie_bg_tsk_id = 0;
static UINT32 acap_tsk_run = 0, is_acap_tsk_running = 0;
static UINT32 aenc_tsk_run = 0, is_aenc_tsk_running = 0;
static UINT32 venc_tsk_run = 0, is_venc_tsk_running = 0;
static UINT32 bg_tsk_run = 0, is_bg_tsk_running = 0;

#define PRI_IAMOVIE_ACPULL             10
#define STKSIZE_IAMOVIE_ACPULL         4096
#define PRI_IAMOVIE_AEPULL             10
#define STKSIZE_IAMOVIE_AEPULL         8192
#define PRI_IAMOVIE_VEPULL             10
#define STKSIZE_IAMOVIE_VEPULL         8192
#define PRI_IAMOVIE_BG                 10
#define STKSIZE_IAMOVIE_BG             4096

#define PHY2VIRT_REC_VSTRM(id, tbl, pa)    (img_venc_bs_buf_va[id][tbl] + (pa - img_venc_bs_buf[id][tbl].buf_info.phy_addr))
#define PHY2VIRT_WIFI_VSTRM(id, ch, pa)    (wifi_venc_bs_buf_va[id]     + (pa - wifi_venc_bs_buf[id].buf_info.phy_addr))
#define PHY2VIRT_ETHCAM_VSTRM(id, tbl, pa) (ethcam_venc_bs_buf_va[id]   + (pa - ethcam_venc_bs_buf[id].buf_info.phy_addr))
#define PHY2VIRT_ACAP_STRM(id, ch, pa)     (aud_acap_frame_va           + (pa - aud_acap_bs_buf_info.buf_info.phy_addr))
#define PHY2VIRT_AENC_ASTRM(id, ch, pa)    (aud_aenc_bs_buf_va[id]      + (pa - aud_aenc_bs_buf[id].buf_info.phy_addr))

static UINT32 _cal_crypto_size(UINT32 bs_size)
{
	UINT32 crypto_size = 0;

	if (bs_size < IAMOVIE_CRYPTO_OFFSET) {
		crypto_size = 0;
	} else if (bs_size > (IAMOVIE_CRYPTO_OFFSET + IAMOVIE_CRYPTO_SIZE)) {
		crypto_size = IAMOVIE_CRYPTO_SIZE;
	} else {
		crypto_size = ALIGN_FLOOR_16(bs_size - IAMOVIE_CRYPTO_OFFSET);
	}
	return crypto_size;
}

THREAD_RETTYPE _ImageApp_MovieMulti_ACapPullTsk(void)
{
	HD_RESULT hd_ret;
	FLGPTN uiFlag;
	HD_AUDIO_FRAME acap_data = {
		.sign        = MAKEFOURCC('A', 'F', 'R', 'M'),
		.p_next      = NULL,
		.ddr_id      = DDR_ID0,
		.size        = 0,
		.phy_addr[0] = 0,
		.bit_width   = 16,
		.sound_mode  = (g_AudioInfo.aud_ch == _CFG_AUDIO_CH_STEREO) ? HD_AUDIO_SOUND_MODE_STEREO : HD_AUDIO_SOUND_MODE_MONO,
		.sample_rate = g_AudioInfo.aud_rate,
		.count       = 0,
		.timestamp   = 0,
	};
#if (BSMUX_USE_NEW_PUSH_METHOD == ENABLE)
	HD_BSMUX_BS bsmuxer_data = {0};
#endif
	UINT32 idx = 0, aidx, j;
#if (USE_ALSA_LIB == ENABLE)
	snd_pcm_uframes_t frames;
	UINT32 buf_id = 0, offset;
#endif

	THREAD_ENTRY();

	is_acap_tsk_running = TRUE;
	vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_FLOW_LINK_CFG, TWF_ORW);

#if 0
	UINT32 va = 0, len = 0, wraddr;
	FILE *fp;
	HD_AUDIOCAP_BUFINFO aud_acap_bs_buf;


	if ((fp = fopen("/mnt/sd/acap.pcm", "wb")) == NULL) {
		DBG_ERR("open write file fail\r\n");
	}

	while (1) {
		vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_AC_MASK, TWF_ORW);

		if (va == 0) {
			hd_audiocap_get(AudCapLink[idx].acap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &(aud_acap_bs_buf));
			va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, aud_acap_bs_buf.buf_info.phy_addr, aud_acap_bs_buf.buf_info.buf_size);
		}

		if ((hd_ret = hd_audiocap_pull_out_buf(AudCapLink[idx].acap_p[0], &acap_data, 500)) == HD_OK) {
			len = acap_data.size;
			if (fp) {
				wraddr = va + acap_data.phy_addr[0] - aud_acap_bs_buf.buf_info.phy_addr;
				fwrite((UINT8*)wraddr, 1, len, fp);
				fflush(fp);
			}
			// release data
			hd_ret = hd_audiocap_release_out_buf(AudCapLink[idx].acap_p[0], &acap_data);
		} else {
			DBG_ERR("hd_ret=%d\r\n", hd_ret);
		}
		uiFlag = vos_flag_chk(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_MASK);
		if (uiFlag == 0) {
			if (fp) {
				fclose(fp);
				hd_common_mem_munmap((void *)va, aud_acap_bs_buf.buf_info.buf_size);
				system("sync");
				va = 0;
			}
		}
	}
#else
	while (acap_tsk_run) {
		if (aud_acap_by_hdal) {
			vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_AC_MASK, TWF_ORW);
			if (uiFlag & FLGIAMOVIE_AC_ESC) {
				acap_tsk_run = 0;
				vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_AC_ESC);
				break;
			}
			vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_AC_IDLE);
			if ((hd_ret = hd_audiocap_pull_out_buf(AudCapLink[idx].acap_p[0], &acap_data, -1)) == HD_OK) {
				vos_sem_wait(IAMOVIE_SEMID_ACAP);
				HD_AUDIO_BS aenc_data = {
					.sign          = MAKEFOURCC('A', 'S', 'T', 'M'),
					.p_next        = NULL,
					.acodec_format = HD_AUDIO_CODEC_PCM,
					.timestamp     = acap_data.timestamp,
					.size          = acap_data.size,
					.ddr_id        = acap_data.ddr_id,
					.phy_addr      = acap_data.phy_addr[0],
					.blk           = -2,
				};
				// move here for audio pre-processing
				_ImageApp_MovieMulti_AudCapFrameProc(&acap_data);

				if (aud_mute_enc_func && aud_mute_enc) {
					UINT8* p_buf_va = (UINT8 *)PHY2VIRT_ACAP_STRM(0, 0, acap_data.phy_addr[0]);
					memset((void *)p_buf_va, 0, acap_data.size);
					hd_common_mem_flush_cache((void *)p_buf_va, acap_data.size);
				}

				for (aidx = 0; aidx < MaxLinkInfo.MaxImgLink; aidx++) {
					for (j = 0; j < 4; j++) {
						if (_LinkCheckStatus(ImgLinkStatus[aidx].bsmux_p[j]) && (img_bsmux_finfo[aidx][j].recformat == _CFG_REC_TYPE_AV) && (img_bsmux_audio_pause[aidx][j] == FALSE)) {
							if (ImgLinkRecInfo[aidx][j/2].aud_codec == _CFG_AUD_CODEC_PCM) {
								if ((trig_once_limited[aidx][j/2] == 0) || (trig_once_cnt[aidx][j/2] < trig_once_limited[aidx][j/2])) {
									#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
									hd_bsmux_push_in_buf_audio(ImgLink[aidx].bsmux_p[j], &aenc_data, -1);
									#else
									bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
									bsmuxer_data.data_type = HD_BSMUX_TYPE_AUDIO;
									bsmuxer_data.timestamp = hd_gettime_us();
									bsmuxer_data.p_user_bs = (VOID *)&aenc_data;
									bsmuxer_data.p_user_data = NULL;
									hd_bsmux_push_in_buf_struct(ImgLink[aidx].bsmux_p[j], &bsmuxer_data, -1);
									#endif
								}
							}
						}
					}
				}
				for (aidx = 0; aidx < MaxLinkInfo.MaxEthCamLink; aidx++) {
					for (j = 0; j < 2; j++) {
						if (_LinkCheckStatus(EthCamLinkStatus[aidx].bsmux_p[j]) && (ethcam_bsmux_finfo[aidx][j].recformat == _CFG_REC_TYPE_AV) && (ethcam_bsmux_audio_pause[aidx][j] == FALSE)) {
							if (EthCamLinkRecInfo[aidx].aud_codec == _CFG_AUD_CODEC_PCM) {
								#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
								hd_bsmux_push_in_buf_audio(EthCamLink[aidx].bsmux_p[j], &aenc_data, -1);
								#else
								bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
								bsmuxer_data.data_type = HD_BSMUX_TYPE_AUDIO;
								bsmuxer_data.timestamp = hd_gettime_us();
								bsmuxer_data.p_user_bs = (VOID *)&aenc_data;
								bsmuxer_data.p_user_data = NULL;
								hd_bsmux_push_in_buf_struct(EthCamLink[aidx].bsmux_p[j], &bsmuxer_data, -1);
								#endif
							}
						}
					}
				}

				//_ImageApp_MovieMulti_AudCapFrameProc(&acap_data);
				// do _ImageApp_MovieMulti_AudCapFrameProc before uvac audio since AudCapFrameProc will do mmap
				if (wifi_movie_uvac_func_en && IsUvacLinkStarted[0]) {
					UVAC_STRM_FRM strmFrm = {0};
					strmFrm.path = UVAC_STRM_AUD;
					strmFrm.addr = acap_data.phy_addr[0];
					strmFrm.va = PHY2VIRT_ACAP_STRM(0, 0, acap_data.phy_addr[0]);
					strmFrm.size = acap_data.size;
					strmFrm.pStrmHdr = 0;
					strmFrm.strmHdrSize = 0;
					UVAC_SetEachStrmInfo(&strmFrm);
				}
				if (_LinkCheckStatus(AudCapLinkStatus[idx].aenc_p[0])) {
					if (aud_mute_enc_func && aud_mute_enc) {
						acap_data.phy_addr[0] = aud_mute_enc_buf_pa;
					}
					hd_ret = hd_audioenc_push_in_buf(AudCapLink[idx].aenc_p[0], &acap_data, NULL, 500);
				}
				// release data
				hd_ret = hd_audiocap_release_out_buf(AudCapLink[idx].acap_p[0], &acap_data);
				vos_sem_sig(IAMOVIE_SEMID_ACAP);
			} else {
				DBG_ERR("hd_audiocap_pull_out_buf() failed(%d)\r\n", hd_ret);
				vos_util_delay_ms(100);
			}
			vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_AC_IDLE);
		} else {
#if (USE_ALSA_LIB == ENABLE)
			offset = 1024 * g_AudioInfo.aud_ch_num * 2 * buf_id;
			buf_id = (buf_id + 1) % 10;
			frames = snd_pcm_readi(alsa_hdl, (void *)(alsabuf_va + offset), 1024);
			if (frames == 1024) {
				if (aud_mute_enc_func && aud_mute_enc) {
					acap_data.phy_addr[0] = aud_mute_enc_buf_pa;
				} else {
					acap_data.phy_addr[0] = alsabuf_pa + offset;
				}
				acap_data.size        = 1024 * g_AudioInfo.aud_ch_num * 2;
				acap_data.timestamp   = hd_gettime_us();

				if (_LinkCheckStatus(AudCapLinkStatus[idx].aenc_p[0])) {
					hd_ret = hd_audioenc_push_in_buf(AudCapLink[idx].aenc_p[0], &acap_data, NULL, 500);
				}


			} else if ((signed int)frames == -EPIPE) {
				snd_pcm_prepare(alsa_hdl);
				DBG_WRN("alsa overrun %d\r\n", frames);
			} else if ((signed int)frames < 0) {
				DBG_ERR("snd_pcm_readi() failed(%d)\r\n", frames);
				vos_util_delay_ms(100);
			} else {
				DBG_WRN("->%d\r\n", frames);
			}
#else
			vos_util_delay_ms(1000);        // avoid busy loop
#endif
		}
	}
	is_acap_tsk_running = FALSE;
#endif
	THREAD_RETURN(0);
}

THREAD_RETTYPE _ImageApp_MovieMulti_AEncPullTsk(void)
{
	HD_RESULT hd_ret;
	FLGPTN uiFlag;
	HD_AUDIO_BS aenc_data = {0};
#if (BSMUX_USE_NEW_PUSH_METHOD == ENABLE)
	HD_BSMUX_BS bsmuxer_data = {0};
#endif
	UINT32 idx = 0, aidx, j;
	STRM_ABUFFER_ELEMENT *pABuf = NULL;
	VOS_TICK t1, t2, t_diff;
	UINT32 use_external_stream = FALSE;

	THREAD_ENTRY();

	is_aenc_tsk_running = TRUE;
	vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_FLOW_LINK_CFG, TWF_ORW);

#if 0
	UINT32 va, len;
	FILE *fp;

	if ((fp = fopen("/mnt/sd/aes.dat", "wb")) == NULL) {
		DBG_ERR("open write file fail\r\n");
	}

	while (1) {
		vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_AE_MASK, TWF_ORW);
		if ((hd_ret = hd_audioenc_pull_out_buf(AudCapLink[idx].aenc_p[0], &aenc_data, -1)) == HD_OK) {
			va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, aenc_data.phy_addr, aenc_data.size);
			len = aenc_data.size;
			if (fp) {
				fwrite((UINT8*)va, 1, len, fp);
				fflush(fp);
			}
			hd_common_mem_munmap((void *)va, aenc_data.size);
			// release data
			hd_ret = hd_audioenc_release_out_buf(AudCapLink[idx].aenc_p[0], &aenc_data);
		} else {
			DBG_ERR("hd_ret=%d\r\n", hd_ret);
		}
		uiFlag = vos_flag_chk(IAMOVIE_FLG_ID, FLGIAMOVIE_AE_MASK);
		if (uiFlag == 0) {
			if (fp) {
				fclose(fp);
			}
		}
	}
#else
	while (aenc_tsk_run) {
		vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_AE_MASK, TWF_ORW);
		if (uiFlag & FLGIAMOVIE_AE_ESC) {
			aenc_tsk_run = 0;
			vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_AE_ESC);
			break;
		}
		vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_AE_IDLE);
		if ((hd_ret = hd_audioenc_pull_out_buf(AudCapLink[idx].aenc_p[0], &aenc_data, -1)) == HD_OK) {
			if (iamovie_encrypt_type & MOVIEMULTI_ENCRYPT_TYPE_AUDIO) {
				_ImageApp_MovieMulti_Crypto(PHY2VIRT_AENC_ASTRM(idx, 0, aenc_data.phy_addr + IAMOVIE_CRYPTO_OFFSET), _cal_crypto_size(aenc_data.size));
			}
			#if (IAMOVIE_TIMESTAMP_WARNING == ENABLE)
			// check time diff between enc and push
			UINT64 t_stamp = hd_gettime_us() - aenc_data.timestamp;
			if (t_stamp > IAMOVIE_TIMESTAMP_WARNING_TIME) {
				DBG_DUMP("iamovie_aepull:path=%x,time=%llu\r\n", AudCapLink[idx].aenc_p[0], t_stamp);
			}
			#endif
			for (aidx = 0; aidx < MaxLinkInfo.MaxImgLink; aidx++) {
				for (j = 0; j < 4; j++) {
					if (_LinkCheckStatus(ImgLinkStatus[aidx].bsmux_p[j]) && (img_bsmux_finfo[aidx][j].recformat == _CFG_REC_TYPE_AV) && (img_bsmux_audio_pause[aidx][j] == FALSE)) {
						if ((ImgLinkRecInfo[aidx][j/2].aud_codec == _CFG_AUD_CODEC_AAC) || (ImgLinkRecInfo[aidx][j/2].aud_codec == _CFG_AUD_CODEC_ULAW) || (ImgLinkRecInfo[aidx][j/2].aud_codec == _CFG_AUD_CODEC_ALAW)) {
							if ((trig_once_limited[aidx][j/2] == 0) || (trig_once_cnt[aidx][j/2] < trig_once_limited[aidx][j/2])) {
								#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
								hd_bsmux_push_in_buf_audio(ImgLink[aidx].bsmux_p[j], &aenc_data, -1);
								#else
								bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
								bsmuxer_data.data_type = HD_BSMUX_TYPE_AUDIO;
								bsmuxer_data.timestamp = hd_gettime_us();
								bsmuxer_data.p_user_bs = (VOID *)&aenc_data;
								bsmuxer_data.p_user_data = NULL;
								hd_bsmux_push_in_buf_struct(ImgLink[aidx].bsmux_p[j], &bsmuxer_data, -1);
								#endif
							}
						}
					}
				}
			}
			for (aidx = 0; aidx < MaxLinkInfo.MaxEthCamLink; aidx++) {
				for (j = 0; j < 2; j++) {
					if (_LinkCheckStatus(EthCamLinkStatus[aidx].bsmux_p[j]) && (ethcam_bsmux_finfo[aidx][j].recformat == _CFG_REC_TYPE_AV) && (ethcam_bsmux_audio_pause[aidx][j] == FALSE)) {
						if ((EthCamLinkRecInfo[aidx].aud_codec == _CFG_AUD_CODEC_AAC) || (EthCamLinkRecInfo[aidx].aud_codec == _CFG_AUD_CODEC_ULAW) || (EthCamLinkRecInfo[aidx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
							#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
							hd_bsmux_push_in_buf_audio(EthCamLink[aidx].bsmux_p[j], &aenc_data, -1);
							#else
							bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
							bsmuxer_data.data_type = HD_BSMUX_TYPE_AUDIO;
							bsmuxer_data.timestamp = hd_gettime_us();
							bsmuxer_data.p_user_bs = (VOID *)&aenc_data;
							bsmuxer_data.p_user_data = NULL;
							hd_bsmux_push_in_buf_struct(EthCamLink[aidx].bsmux_p[j], &bsmuxer_data, -1);
							#endif
						}
					}
				}
			}
			ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING, &use_external_stream);
			if (use_external_stream == FALSE) {
				for (aidx = 0; aidx < MaxLinkInfo.MaxWifiLink; aidx++) {
					if ((vos_flag_chk(IAMOVIE_FLG_ID, FLGIAMOVIE_AE_S0)) && (iacomm_flag_chk(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_QUE_RDY << aidx)))) {
						iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_CQUE_FREE << aidx));
						if ((pABuf = (STRM_ABUFFER_ELEMENT *)_QUEUE_DeQueueFromHead(&ABufferQueueClear[aidx])) != NULL) {
							pABuf->timestamp = aenc_data.timestamp;
							pABuf->acodec_format = aenc_data.acodec_format;
							pABuf->phy_addr = aenc_data.phy_addr;
							pABuf->size = aenc_data.size;
							_QUEUE_EnQueueToTail(&ABufferQueueReady[aidx], (QUEUE_BUFFER_ELEMENT *)pABuf);
							iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_BS_RDY << aidx));
						}
						iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_CQUE_FREE << aidx));
					}
				}
			}
			//if (aud_bs_cb[0] && IsWifiLinkStarted[0] && WifiLinkStrmInfo[idx].aud_codec != _CFG_AUD_CODEC_NONE) {
			if (aud_bs_cb[0]) {
				vos_perf_mark(&t1);
				aud_bs_cb[0](0, &aenc_data);
				vos_perf_mark(&t2);
				t_diff = (t2 - t1) / 1000;
				if (t_diff > IAMOVIE_BS_CB_TIMEOUT_CHECK) {
					DBG_WRN("ImageApp_MovieMulti aud_bs_cb[0] run %dms > %dms!\r\n", t_diff, IAMOVIE_BS_CB_TIMEOUT_CHECK);
				}
			}
			// release data
			hd_ret = hd_audioenc_release_out_buf(AudCapLink[idx].aenc_p[0], &aenc_data);
		} else {
			DBG_ERR("hd_audioenc_pull_out_buf() failed(%d)\r\n", hd_ret);
			vos_util_delay_ms(100);
		}
		vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_AE_IDLE);
	}
	is_aenc_tsk_running = FALSE;
#endif
	THREAD_RETURN(0);
}

THREAD_RETTYPE _ImageApp_MovieMulti_VEncPullTsk(void)
{
	HD_RESULT hd_ret;
	FLGPTN uiFlag;
	UINT32 venc_pull_cnt = 0, i, idx, loop, vidx;
	HD_VIDEOENC_POLL_LIST venc_poll_list[MAX_IMG_PATH*2+MAX_WIFI_PATH*1] = {0};
	HD_VIDEOENC_BS  venc_data = {0};
#if (BSMUX_USE_NEW_PUSH_METHOD == ENABLE)
	HD_BSMUX_BS bsmuxer_data = {0};
#endif
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
	UINT32 enc_sec;
#endif
	STRM_VBUFFER_ELEMENT *pVBuf = NULL;
	UINT32 trig_once_stop_id = UNUSED;
	VOS_TICK t1, t2, t_diff;
	UINT32 use_external_stream = FALSE;

	THREAD_ENTRY();

	is_venc_tsk_running = TRUE;
	vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_FLOW_LINK_CFG, TWF_ORW);
	for (i = 0; i < MaxLinkInfo.MaxImgLink; i++) {
		venc_poll_list[venc_pull_cnt+0].path_id = ImgLink[i].venc_p[0];
		venc_poll_list[venc_pull_cnt+1].path_id = ImgLink[i].venc_p[1];
		venc_pull_cnt += 2;
	}
	for (i = 0; i < MaxLinkInfo.MaxWifiLink; i++) {
		venc_poll_list[venc_pull_cnt].path_id   = WifiLink[i].venc_p[0];
		venc_pull_cnt ++;
	}
	for (i = 0; i < MaxLinkInfo.MaxEthCamLink; i++) {
		venc_poll_list[venc_pull_cnt].path_id   = EthCamLink[i].venc_p[0];
		venc_pull_cnt ++;
	}

#if 0
	UINT32 va, len, j;
	FILE *fp;

	if ((fp = fopen("/mnt/sd/ves.dat", "wb")) == NULL) {
		DBG_ERR("open write file fail\r\n");
	}

	while (1) {
		vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_VE_MASK, TWF_ORW);
		if ((hd_ret = hd_videoenc_poll_list(venc_poll_list, venc_pull_cnt, -1)) == HD_OK) {
			for (i = 0; i < venc_pull_cnt; i++) {
				if (venc_poll_list[i].revent.event == TRUE) {
					if ((hd_ret = hd_videoenc_pull_out_buf(venc_poll_list[i].path_id, &venc_data, 0)) == HD_OK) {
						for (j = 0; j < venc_data.pack_num; j++) {
							va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, venc_data.video_pack[j].phy_addr, venc_data.video_pack[j].size);
							len = venc_data.video_pack[j].size;
							if (fp) {
								fwrite((UINT8*)va, 1, len, fp);
								fflush(fp);
							}
							hd_common_mem_munmap((void *)va, venc_data.video_pack[j].size);
						}
						// release data
						hd_ret = hd_videoenc_release_out_buf(venc_poll_list[i].path_id, &venc_data);
					}
				}
			}
		} else {
			DBG_ERR("hd_ret=%d\r\n", hd_ret);
		}
		uiFlag = vos_flag_chk(IAMOVIE_FLG_ID, FLGIAMOVIE_VE_MASK);
		if (uiFlag == 0) {
			if (fp) {
				fclose(fp);
			}
		}
	}
#else
	while (venc_tsk_run) {
		vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID, FLGIAMOVIE_VE_MASK, TWF_ORW);
		if (uiFlag & FLGIAMOVIE_VE_ESC) {
			venc_tsk_run = 0;
			vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_VE_ESC);
			break;
		}
		if ((hd_ret = hd_videoenc_poll_list(venc_poll_list, venc_pull_cnt, -1)) == HD_OK) {
			for (i = 0; i < venc_pull_cnt; i++) {
				if (venc_poll_list[i].revent.event == TRUE) {
					vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_VE_IDLE);
					if ((hd_ret = hd_videoenc_pull_out_buf(venc_poll_list[i].path_id, &venc_data, 0)) == HD_OK) {
						#if (IAMOVIE_TIMESTAMP_WARNING == ENABLE)
						// check time diff between enc and push
						UINT64 t_stamp_now = hd_gettime_us();
						UINT64 t_stamp_diff = t_stamp_now - venc_data.timestamp;
						if (venc_data.timestamp == 0) {
							DBG_WRN("iamovie_vepull:path=%x,timestamp=0,now=%llu\r\n", venc_poll_list[i].path_id, t_stamp_now);
						} else if (t_stamp_diff > IAMOVIE_TIMESTAMP_WARNING_TIME) {
							DBG_DUMP("iamovie_vepull:path=%x,time=%llu(%llu-%llu)\r\n", venc_poll_list[i].path_id, t_stamp_diff, t_stamp_now, venc_data.timestamp);
						}
						#endif
						trig_once_stop_id = UNUSED;
						if ((idx = ImageApp_MovieMulti_VePort2Imglink(i)) != UNUSED) {
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
							if (g_ia_moviemulti_usercb && venc_poll_list[i].path_id == ImgLink[img_venc_1st_path_idx].venc_p[img_venc_1st_path_ch]) {
								if (venc_data.frame_type == HD_FRAME_TYPE_IDR) {
									if (img_venc_frame_cnt[img_venc_1st_path_idx][img_venc_1st_path_ch] % ImgLinkRecInfo[img_venc_1st_path_idx][img_venc_1st_path_ch].frame_rate == 0) {
										enc_sec = img_venc_frame_cnt[img_venc_1st_path_idx][img_venc_1st_path_ch] / ImgLinkRecInfo[img_venc_1st_path_idx][img_venc_1st_path_ch].frame_rate;
										if (ImgLinkFileInfo.end_type == MOVREC_ENDTYPE_CUTOVERLAP || ImgLinkFileInfo.end_type == MOVREC_ENDTYPE_CUT_TILLCARDFULL) {
										    enc_sec %= ImgLinkFileInfo.seamless_sec;
											g_ia_moviemulti_usercb(0, MOVIE_USER_CB_EVENT_REC_ONE_SECOND, enc_sec);
											g_vdo_rec_sec = enc_sec;
										}
									}
								}
							}
#endif
							if (i % 2 == 0) {       // main
								// workaround for timestamp = 0
								if (venc_data.timestamp == 0) {
									venc_data.timestamp = img_venc_timestamp[idx][0];
								} else {
									img_venc_timestamp[idx][0] = venc_data.timestamp;
								}
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
								if (img_venc_frame_cnt_clr[idx][0] && (img_venc_frame_cnt[idx][0] % ImgLinkRecInfo[idx][0].cbr_info.uiGOP == 0)) {
									img_venc_frame_cnt[idx][0] = 0;
									img_venc_frame_cnt_clr[idx][0] = 0;
								}
								img_venc_frame_cnt[idx][0]++;
#endif
								if (iamovie_encrypt_type & MOVIEMULTI_ENCRYPT_TYPE_I_FRAME) {
									if (venc_data.pack_num > 1) {
										_ImageApp_MovieMulti_Crypto(PHY2VIRT_REC_VSTRM(idx, 0, venc_data.video_pack[venc_data.pack_num-1].phy_addr + IAMOVIE_CRYPTO_OFFSET), _cal_crypto_size(venc_data.video_pack[venc_data.pack_num-1].size));
									}
								}
								if (iamovie_encrypt_type & MOVIEMULTI_ENCRYPT_TYPE_P_FRAME) {
									if (venc_data.pack_num == 1) {
										_ImageApp_MovieMulti_Crypto(PHY2VIRT_REC_VSTRM(idx, 0, venc_data.video_pack[venc_data.pack_num-1].phy_addr + IAMOVIE_CRYPTO_OFFSET), _cal_crypto_size(venc_data.video_pack[venc_data.pack_num-1].size));
									}
								}
								if (vdo_bs_rec_cb[idx][0]) {
									vos_perf_mark(&t1);
									vdo_bs_rec_cb[idx][0]((_CFG_REC_ID_1 + idx), &venc_data);
									vos_perf_mark(&t2);
									t_diff = (t2 - t1) / 1000;
									if (t_diff > IAMOVIE_BS_CB_TIMEOUT_CHECK) {
										DBG_WRN("ImageApp_MovieMulti vdo_bs_cb(rec[%d][0]) run %dms > %dms!\r\n", idx, t_diff, IAMOVIE_BS_CB_TIMEOUT_CHECK);
									}
								}
								if (_LinkCheckStatus(ImgLinkStatus[idx].bsmux_p[0])) {
									if ((trig_once_limited[idx][0] == 0) || (trig_once_cnt[idx][0] < trig_once_limited[idx][0])) {
										vos_flag_clr(IAMOVIE_FLG_BSMUX_ID, (1 << HD_GET_OUT(ImgLink[idx].bsmux_p[0])));
										#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
										hd_bsmux_push_in_buf_video(ImgLink[idx].bsmux_p[0], &venc_data, -1);
										#else
										bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
										bsmuxer_data.data_type = HD_BSMUX_TYPE_VIDEO;
										bsmuxer_data.timestamp = hd_gettime_us();
										bsmuxer_data.p_user_bs = (VOID *)&venc_data;
										bsmuxer_data.p_user_data = NULL;
										if (img_bsmux_meta_data_en[idx][0]) {
											bsmuxer_data.data_type |= HD_BSMUX_TYPE_META;
										}
										hd_bsmux_push_in_buf_struct(ImgLink[idx].bsmux_p[0], &bsmuxer_data, -1);
										#endif
									}
									if (trig_once_limited[idx][0]) {
										if (trig_once_first_i[idx][0] == FALSE && venc_data.frame_type == HD_FRAME_TYPE_IDR) {
											trig_once_first_i[idx][0] = TRUE;
										}
										if (trig_once_first_i[idx][0]) {
											trig_once_cnt[idx][0] ++;
											if (trig_once_limited[idx][0] == trig_once_cnt[idx][0]) {
												trig_once_stop_id = _CFG_REC_ID_1 + idx;
											}
										}
									}
								}
								if (_LinkCheckStatus(ImgLinkStatus[idx].bsmux_p[1])) {
									if ((trig_once_limited[idx][0] == 0) || (trig_once_cnt[idx][0] < trig_once_limited[idx][0])) {
										vos_flag_clr(IAMOVIE_FLG_BSMUX_ID, (1 << HD_GET_OUT(ImgLink[idx].bsmux_p[1])));
										#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
										hd_bsmux_push_in_buf_video(ImgLink[idx].bsmux_p[1], &venc_data, -1);
										#else
										bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
										bsmuxer_data.data_type = HD_BSMUX_TYPE_VIDEO;
										bsmuxer_data.timestamp = hd_gettime_us();
										bsmuxer_data.p_user_bs = (VOID *)&venc_data;
										bsmuxer_data.p_user_data = NULL;
										if (img_bsmux_meta_data_en[idx][1]) {
											bsmuxer_data.data_type |= HD_BSMUX_TYPE_META;
										}
										hd_bsmux_push_in_buf_struct(ImgLink[idx].bsmux_p[1], &bsmuxer_data, -1);
										#endif
									}
								}
							} else {                // clone
								// workaround for timestamp = 0
								if (venc_data.timestamp == 0) {
									venc_data.timestamp = img_venc_timestamp[idx][1];
								} else {
									img_venc_timestamp[idx][1] = venc_data.timestamp;
								}
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
								if (img_venc_frame_cnt_clr[idx][1] && (img_venc_frame_cnt[idx][1] % ImgLinkRecInfo[idx][1].cbr_info.uiGOP == 0)) {
									img_venc_frame_cnt[idx][1] = 0;
									img_venc_frame_cnt_clr[idx][1] = 0;
								}
								img_venc_frame_cnt[idx][1]++;
#endif
								if (iamovie_encrypt_type & MOVIEMULTI_ENCRYPT_TYPE_I_FRAME) {
									if (venc_data.pack_num > 1) {
										_ImageApp_MovieMulti_Crypto(PHY2VIRT_REC_VSTRM(idx, 1, venc_data.video_pack[venc_data.pack_num-1].phy_addr + IAMOVIE_CRYPTO_OFFSET), _cal_crypto_size(venc_data.video_pack[venc_data.pack_num-1].size));
									}
								}
								if (iamovie_encrypt_type & MOVIEMULTI_ENCRYPT_TYPE_P_FRAME) {
									if (venc_data.pack_num == 1) {
										_ImageApp_MovieMulti_Crypto(PHY2VIRT_REC_VSTRM(idx, 1, venc_data.video_pack[venc_data.pack_num-1].phy_addr + IAMOVIE_CRYPTO_OFFSET), _cal_crypto_size(venc_data.video_pack[venc_data.pack_num-1].size));
									}
								}
								if (vdo_bs_rec_cb[idx][1]) {
									vos_perf_mark(&t1);
									vdo_bs_rec_cb[idx][1]((_CFG_CLONE_ID_1 + idx), &venc_data);
									vos_perf_mark(&t2);
									t_diff = (t2 - t1) / 1000;
									if (t_diff > IAMOVIE_BS_CB_TIMEOUT_CHECK) {
										DBG_WRN("ImageApp_MovieMulti vdo_bs_cb(rec[%d][1]) run %dms > %dms!\r\n", idx, t_diff, IAMOVIE_BS_CB_TIMEOUT_CHECK);
									}
								}
								if (_LinkCheckStatus(ImgLinkStatus[idx].bsmux_p[2])) {
									if ((trig_once_limited[idx][1] == 0) || (trig_once_cnt[idx][1] < trig_once_limited[idx][1])) {
										vos_flag_clr(IAMOVIE_FLG_BSMUX_ID, (1 << HD_GET_OUT(ImgLink[idx].bsmux_p[2])));
										#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
										hd_bsmux_push_in_buf_video(ImgLink[idx].bsmux_p[2], &venc_data, -1);
										#else
										bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
										bsmuxer_data.data_type = HD_BSMUX_TYPE_VIDEO;
										bsmuxer_data.timestamp = hd_gettime_us();
										bsmuxer_data.p_user_bs = (VOID *)&venc_data;
										bsmuxer_data.p_user_data = NULL;
										if (img_bsmux_meta_data_en[idx][2]) {
											bsmuxer_data.data_type |= HD_BSMUX_TYPE_META;
										}
										hd_bsmux_push_in_buf_struct(ImgLink[idx].bsmux_p[2], &bsmuxer_data, -1);
										#endif
									}
									if (trig_once_limited[idx][1]) {
										if (trig_once_first_i[idx][1] == FALSE && venc_data.frame_type == HD_FRAME_TYPE_IDR) {
											trig_once_first_i[idx][1] = TRUE;
										}
										if (trig_once_first_i[idx][1]) {
											trig_once_cnt[idx][1] ++;
											if (trig_once_limited[idx][1] == trig_once_cnt[idx][1]) {
												trig_once_stop_id = _CFG_CLONE_ID_1 + idx;
											}
										}
									}
								}
								if (_LinkCheckStatus(ImgLinkStatus[idx].bsmux_p[3])) {
									if ((trig_once_limited[idx][1] == 0) || (trig_once_cnt[idx][1] < trig_once_limited[idx][1])) {
										vos_flag_clr(IAMOVIE_FLG_BSMUX_ID, (1 << HD_GET_OUT(ImgLink[idx].bsmux_p[3])));
										#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
										hd_bsmux_push_in_buf_video(ImgLink[idx].bsmux_p[3], &venc_data, -1);
										#else
										bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
										bsmuxer_data.data_type = HD_BSMUX_TYPE_VIDEO;
										bsmuxer_data.timestamp = hd_gettime_us();
										bsmuxer_data.p_user_bs = (VOID *)&venc_data;
										bsmuxer_data.p_user_data = NULL;
										if (img_bsmux_meta_data_en[idx][3]) {
											bsmuxer_data.data_type |= HD_BSMUX_TYPE_META;
										}
										hd_bsmux_push_in_buf_struct(ImgLink[idx].bsmux_p[3], &bsmuxer_data, -1);
										#endif
									}
								}
							}
						} else if (i == MaxLinkInfo.MaxImgLink * 2) {
							vidx = i - MaxLinkInfo.MaxImgLink * 2;
							// workaround for timestamp = 0
							if (venc_data.timestamp == 0) {
								venc_data.timestamp = wifi_venc_timestamp[vidx];
							} else {
								wifi_venc_timestamp[vidx] = venc_data.timestamp;
							}
							ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING, &use_external_stream);
							if (use_external_stream == FALSE) {
								if (WifiLinkStrmInfo[vidx].codec != _CFG_CODEC_MJPG) {
									if (iacomm_flag_chk(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_QUE_RDY << vidx))) {
										iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_CQUE_FREE << vidx));
										if ((pVBuf = (STRM_VBUFFER_ELEMENT *)_QUEUE_DeQueueFromHead(&VBufferQueueClear[vidx])) != NULL) {
											pVBuf->timestamp = venc_data.timestamp;
											pVBuf->vcodec_format = venc_data.vcodec_format;
											pVBuf->pack_num = venc_data.pack_num;
											for (loop = 0; loop < pVBuf->pack_num; loop++) {
												pVBuf->phy_addr[loop] = venc_data.video_pack[loop].phy_addr;
												pVBuf->size[loop] = venc_data.video_pack[loop].size;
											}
											_QUEUE_EnQueueToTail(&VBufferQueueReady[vidx], (QUEUE_BUFFER_ELEMENT *)pVBuf);
											iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << vidx));
										}
										iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_CQUE_FREE << vidx));
									}
								} else {
									if (ImageApp_Common_IsRtspStart(0)) {
										LVIEWNVT_FRAME_INFO strmFrm = {0};
										strmFrm.addr = PHY2VIRT_WIFI_VSTRM(0, 0, venc_data.video_pack[venc_data.pack_num-1].phy_addr);
										strmFrm.size = venc_data.video_pack[venc_data.pack_num-1].size;
										lviewnvt_push_frame(&strmFrm);
									}
								}
							}
							if (wifi_movie_uvac_func_en && IsUvacLinkStarted[0]) {
								UVAC_STRM_FRM strmFrm = {0};
								strmFrm.path = UVAC_STRM_VID;
								strmFrm.addr = venc_data.video_pack[venc_data.pack_num-1].phy_addr;
								strmFrm.va   = PHY2VIRT_WIFI_VSTRM(0, 0, venc_data.video_pack[venc_data.pack_num-1].phy_addr);
								strmFrm.size = venc_data.video_pack[venc_data.pack_num-1].size;
								strmFrm.pStrmHdr = 0;
								strmFrm.strmHdrSize = 0;
								if (venc_data.pack_num > 1) {
									for (loop = 0; loop < venc_data.pack_num - 1; loop ++) {
										strmFrm.strmHdrSize += venc_data.video_pack[loop].size;
									}
									strmFrm.addr -= strmFrm.strmHdrSize;
									strmFrm.va   -= strmFrm.strmHdrSize;
									strmFrm.size += strmFrm.strmHdrSize;
								}
								UVAC_SetEachStrmInfo(&strmFrm);
							}
							if (vdo_bs_strm_cb[vidx]) {
								vos_perf_mark(&t1);
								vdo_bs_strm_cb[vidx]((_CFG_STRM_ID_1 + vidx), &venc_data);
								vos_perf_mark(&t2);
								t_diff = (t2 - t1) / 1000;
								if (t_diff > IAMOVIE_BS_CB_TIMEOUT_CHECK) {
									DBG_WRN("ImageApp_MovieMulti vdo_bs_cb(strm[%d]) run %dms > %dms!\r\n", vidx, t_diff, IAMOVIE_BS_CB_TIMEOUT_CHECK);
								}
							}
						} else if ((i >= (MaxLinkInfo.MaxImgLink * 2 + MaxLinkInfo.MaxWifiLink)) && (i < (MaxLinkInfo.MaxImgLink * 2 + MaxLinkInfo.MaxWifiLink + MaxLinkInfo.MaxEthCamLink))) {
							vidx = i - MaxLinkInfo.MaxImgLink * 2 - MaxLinkInfo.MaxWifiLink;
							// workaround for timestamp = 0
							if (venc_data.timestamp == 0) {
								venc_data.timestamp = ethcam_venc_timestamp[vidx];
							} else {
								ethcam_venc_timestamp[vidx] = venc_data.timestamp;
							}
							if (iamovie_encrypt_type & MOVIEMULTI_ENCRYPT_TYPE_I_FRAME) {
								if (venc_data.pack_num > 1) {
									_ImageApp_MovieMulti_Crypto(PHY2VIRT_ETHCAM_VSTRM(vidx, 0, venc_data.video_pack[venc_data.pack_num-1].phy_addr + IAMOVIE_CRYPTO_OFFSET), _cal_crypto_size(venc_data.video_pack[venc_data.pack_num-1].size));
								}
							}
							if (iamovie_encrypt_type & MOVIEMULTI_ENCRYPT_TYPE_P_FRAME) {
								if (venc_data.pack_num == 1) {
									_ImageApp_MovieMulti_Crypto(PHY2VIRT_ETHCAM_VSTRM(vidx, 0, venc_data.video_pack[venc_data.pack_num-1].phy_addr + IAMOVIE_CRYPTO_OFFSET), _cal_crypto_size(venc_data.video_pack[venc_data.pack_num-1].size));
								}
							}
							if (vdo_bs_ethcam_cb[vidx]) {
								vos_perf_mark(&t1);
								vdo_bs_ethcam_cb[vidx]((_CFG_ETHCAM_ID_1 + vidx), &venc_data);
								vos_perf_mark(&t2);
								t_diff = (t2 - t1) / 1000;
								if (t_diff > IAMOVIE_BS_CB_TIMEOUT_CHECK) {
									DBG_WRN("ImageApp_MovieMulti vdo_bs_cb(ethcam[%d][0]) run %dms > %dms!\r\n", idx, t_diff, IAMOVIE_BS_CB_TIMEOUT_CHECK);
								}
							}
							if (_LinkCheckStatus(EthCamLinkStatus[vidx].bsmux_p[0])) {
								vos_flag_clr(IAMOVIE_FLG_BSMUX_ID, (1 << HD_GET_OUT(EthCamLink[vidx].bsmux_p[0])));
								#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
								hd_bsmux_push_in_buf_video(EthCamLink[vidx].bsmux_p[0], &venc_data, -1);
								#else
								bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
								bsmuxer_data.data_type = HD_BSMUX_TYPE_VIDEO;
								bsmuxer_data.timestamp = hd_gettime_us();
								bsmuxer_data.p_user_bs = (VOID *)&venc_data;
								bsmuxer_data.p_user_data = NULL;
								if (ethcam_bsmux_meta_data_en[vidx][0]) {
									bsmuxer_data.data_type |= HD_BSMUX_TYPE_META;
								}
								hd_bsmux_push_in_buf_struct(EthCamLink[vidx].bsmux_p[0], &bsmuxer_data, -1);
								#endif
							}
							if (_LinkCheckStatus(EthCamLinkStatus[vidx].bsmux_p[1])) {
								vos_flag_clr(IAMOVIE_FLG_BSMUX_ID, (1 << HD_GET_OUT(EthCamLink[vidx].bsmux_p[1])));
								#if (BSMUX_USE_NEW_PUSH_METHOD == DISABLE)
								hd_bsmux_push_in_buf_video(EthCamLink[vidx].bsmux_p[1], &venc_data, -1);
								#else
								bsmuxer_data.sign = MAKEFOURCC('B', 'S', 'M', 'X');
								bsmuxer_data.data_type = HD_BSMUX_TYPE_VIDEO;
								bsmuxer_data.timestamp = hd_gettime_us();
								bsmuxer_data.p_user_bs = (VOID *)&venc_data;
								bsmuxer_data.p_user_data = NULL;
								if (ethcam_bsmux_meta_data_en[vidx][1]) {
									bsmuxer_data.data_type |= HD_BSMUX_TYPE_META;
								}
								hd_bsmux_push_in_buf_struct(EthCamLink[vidx].bsmux_p[1], &bsmuxer_data, -1);
								#endif
							}
						}
						if ((hd_ret = hd_videoenc_release_out_buf(venc_poll_list[i].path_id, &venc_data)) != HD_OK) {
							DBG_ERR("hd_videoenc_release_out_buf(%d) fail(%d)\r\n", venc_poll_list[i].path_id, hd_ret);
						}
						if (trig_once_stop_id != UNUSED) {
							MOVIE_TBL_IDX tb;
							_ImageApp_MovieMulti_RecidGetTableIndex(trig_once_stop_id, &tb);
							if (img_bsmux_2v1a_mode[tb.idx] == 0) {         // normal mode
								vos_flag_set(IAMOVIE_FLG_BG_ID, (FLGIAMOVIE_BG_VE_M0 << (2 * tb.idx + tb.tbl)));
							} else {
								trig_once_2v1a_stop_path[tb.idx][tb.tbl] = TRUE;
								if (trig_once_2v1a_stop_path[tb.idx][0] && trig_once_2v1a_stop_path[tb.idx][1]) {
									vos_flag_set(IAMOVIE_FLG_BG_ID, (FLGIAMOVIE_BG_VE_M0 << (2 * tb.idx)));
									trig_once_2v1a_stop_path[tb.idx][0] = FALSE;
									trig_once_2v1a_stop_path[tb.idx][1] = FALSE;
								}
							}
						}
					}
					vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_VE_IDLE);
				}
			}
		} else {
			DBG_ERR("hd_videoenc_poll_list() failed(%d)\r\n", hd_ret);
			vos_util_delay_ms(100);
		}
	}
	is_venc_tsk_running = FALSE;
#endif
	THREAD_RETURN(0);
}

THREAD_RETTYPE _ImageApp_MovieMulti_BGTsk(void)
{
	UINT32 i;
	UINT32 stop_path = UNUSED;
	FLGPTN uiFlag;
	MOVIE_TBL_IDX tb;

	THREAD_ENTRY();

	is_bg_tsk_running = TRUE;

	while (venc_tsk_run) {
		vos_flag_wait(&uiFlag, IAMOVIE_FLG_BG_ID, FLGIAMOVIE_BG_MASK, TWF_ORW);
		// escape case
		if (uiFlag & FLGIAMOVIE_BG_ESC) {
			venc_tsk_run = 0;
			vos_flag_clr(IAMOVIE_FLG_BG_ID, FLGIAMOVIE_BG_ESC);
			break;
		} else if (vos_flag_chk(IAMOVIE_FLG_BG_ID, FLGIAMOVIE_BG_VE_MASK)) {
			stop_path = UNUSED;
			for (i = 0; i < MAX_IMG_PATH; i++) {
				if (uiFlag & (FLGIAMOVIE_BG_VE_M0 << (2 * i))) {
					stop_path = _CFG_REC_ID_1 + i;
					vos_flag_clr(IAMOVIE_FLG_BG_ID, (FLGIAMOVIE_BG_VE_M0 << (2 * i)));
					break;
				} else if (uiFlag & (FLGIAMOVIE_BG_VE_C0 << (2 * i))) {
					stop_path = _CFG_CLONE_ID_1 + i;
					vos_flag_clr(IAMOVIE_FLG_BG_ID, (FLGIAMOVIE_BG_VE_C0 << (2 * i)));
					break;
				}
			}
			if (stop_path != UNUSED) {
				_ImageApp_MovieMulti_RecidGetTableIndex(stop_path, &tb);
				if (img_bsmux_2v1a_mode[tb.idx] == 0) {         // normal mode
					vos_flag_wait(&uiFlag, IAMOVIE_FLG_BSMUX_ID, (1 << ImageApp_MovieMulti_Recid2BsPort(stop_path)), TWF_ANDW);
					DBG_DUMP("IAMovieBGTsk do RecStop(%d) for non-2V1A mode\r\n", stop_path);
					ImageApp_MovieMulti_RecStop(stop_path);
					trig_once_limited[tb.idx][tb.tbl] = 0;
					trig_once_cnt[tb.idx][tb.tbl] = 0;
					trig_once_first_i[tb.idx][tb.tbl] = 0;
				} else {
					vos_flag_wait(&uiFlag, IAMOVIE_FLG_BSMUX_ID, ((1 << ImageApp_MovieMulti_Recid2BsPort(_CFG_REC_ID_1 + tb.idx)) | (1 << ImageApp_MovieMulti_Recid2BsPort(_CFG_CLONE_ID_1 + tb.idx))), TWF_ANDW);
					DBG_DUMP("IAMovieBGTsk do RecStop(%d) for 2V1A mode\r\n", tb.idx);
					ImageApp_MovieMulti_RecStop(_CFG_CLONE_ID_1 + tb.idx);
					ImageApp_MovieMulti_RecStop(_CFG_REC_ID_1 + tb.idx);
					trig_once_limited[tb.idx][0] = 0;
					trig_once_cnt[tb.idx][0] = 0;
					trig_once_first_i[tb.idx][0] = 0;
					trig_once_limited[tb.idx][1] = 0;
					trig_once_cnt[tb.idx][1] = 0;
					trig_once_first_i[tb.idx][1] = 0;
				}
			}
		}
	}
	DBG_DUMP("_ImageApp_MovieMulti_BGTsk Exit\r\n");
	is_bg_tsk_running = FALSE;

	THREAD_RETURN(0);
}

BOOL _ImageApp_MovieMulti_IsBGTskRunning(void)
{
	if (is_bg_tsk_running || bg_tsk_run) {
		return TRUE;
	} else {
		return FALSE;
	}
}

ER _ImageApp_MovieMulti_CreateBGTsk(void)
{
	ER ret = E_OK;
	T_CFLG cflg;

	memset(&cflg, 0, sizeof(T_CFLG));
	if (vos_flag_create(&IAMOVIE_FLG_BG_ID, &cflg, "IAMOVIE_FLG_BG_ID") != E_OK) {
		DBG_ERR("IAMOVIE_FLG_BG_ID fail\r\n");
		ret = E_SYS;
	}

	if ((iamovie_bg_tsk_id = vos_task_create(_ImageApp_MovieMulti_BGTsk, 0, "IAMovie_BGTsk", PRI_IAMOVIE_BG, STKSIZE_IAMOVIE_BG)) == 0) {
		DBG_ERR("IAMovie_BGTsk create failed.\r\n");
		ret = E_SYS;
	} else {
		bg_tsk_run = TRUE;
		if (vos_task_resume(iamovie_bg_tsk_id)) {
			DBG_ERR("Resume IAMovie_BGTsk fail\r\n");
			bg_tsk_run = FALSE;
			ret = E_SYS;
		}
	}
	return ret;
}

ER _ImageApp_MovieMulti_InstallID(void)
{
	ER ret = E_OK;
	T_CFLG cflg;

	acap_tsk_run = FALSE;
	aenc_tsk_run = FALSE;
	venc_tsk_run = FALSE;
	is_acap_tsk_running = FALSE;
	is_aenc_tsk_running = FALSE;
	is_venc_tsk_running = FALSE;

	memset(&cflg, 0, sizeof(T_CFLG));
	if ((ret |= vos_flag_create(&IAMOVIE_FLG_ID, &cflg, "IAMOVIE_FLG")) != E_OK) {
		DBG_ERR("IAMOVIE_FLG_ID fail\r\n");
	}

	memset(&cflg, 0, sizeof(T_CFLG));
	if ((ret |= vos_flag_create(&IAMOVIE_FLG_ID2, &cflg, "IAMOVIE_FLG2")) != E_OK) {
		DBG_ERR("IAMOVIE_FLG_ID2 fail\r\n");
	}

	memset(&cflg, 0, sizeof(T_CFLG));
	if ((ret |= vos_flag_create(&IAMOVIE_FLG_BSMUX_ID, &cflg, "IAMOVIE_BSMUX")) != E_OK) {
		DBG_ERR("IAMOVIE_FLG_BSMUX_ID fail\r\n");
	}

	if ((ret |= vos_sem_create(&IAMOVIE_SEMID_FM, 1, "IAMOVIE_SEM_FM")) != E_OK) {
		DBG_ERR("Create IAMOVIE_SEMID_FM fail(%d)\r\n", ret);
	}

	if ((ret |= vos_sem_create(&IAMOVIE_SEMID_CRYPTO, 1, "IAMOVIE_SEM_CRYPTO")) != E_OK) {
		DBG_ERR("Create IAMOVIE_SEMID_CRYPTO fail(%d)\r\n", ret);
	}

	if ((ret |= vos_sem_create(&IAMOVIE_SEMID_OPERATION, 1, "IAMOVIE_SEM_OPERATION")) != E_OK) {
		DBG_ERR("Create IAMOVIE_SEMID_OPERATION fail(%d)\r\n", ret);
	}

	if ((ret |= vos_sem_create(&IAMOVIE_SEMID_ACAP, 1, "IAMOVIE_SEMID_ACAP")) != E_OK) {
		DBG_ERR("Create IAMOVIE_SEMID_ACAP fail(%d)\r\n", ret);
	}

	if ((iamovie_acpull_tsk_id = vos_task_create(_ImageApp_MovieMulti_ACapPullTsk, 0, "IAMovie_ACPullTsk", PRI_IAMOVIE_ACPULL, STKSIZE_IAMOVIE_ACPULL)) == 0) {
		DBG_ERR("IAMovie_ACPullTsk create failed.\r\n");
		ret |= E_SYS;
	} else {
		acap_tsk_run = TRUE;
		vos_task_resume(iamovie_acpull_tsk_id);
	}

	if ((iamovie_aepull_tsk_id = vos_task_create(_ImageApp_MovieMulti_AEncPullTsk, 0, "IAMovie_AEPullTsk", PRI_IAMOVIE_AEPULL, STKSIZE_IAMOVIE_AEPULL)) == 0) {
		DBG_ERR("IAMovie_AEPullTsk create failed.\r\n");
		ret |= E_SYS;
	} else {
		aenc_tsk_run = TRUE;
		vos_task_resume(iamovie_aepull_tsk_id);
	}

	if ((iamovie_vepull_tsk_id = vos_task_create(_ImageApp_MovieMulti_VEncPullTsk, 0, "IAMovie_VEPullTsk", PRI_IAMOVIE_VEPULL, STKSIZE_IAMOVIE_VEPULL)) == 0) {
		DBG_ERR("IAMovie_VEPullTsk create failed.\r\n");
		ret |= E_SYS;
	} else {
		venc_tsk_run = TRUE;
		vos_task_resume(iamovie_vepull_tsk_id);
	}
	return ret;
}

ER _ImageApp_MovieMulti_UninstallID(void)
{
	ER ret = E_OK;
	UINT32 i, delay_cnt; //, atsk_status = FALSE, vtsk_status = FALSE;

	delay_cnt = 50;
	acap_tsk_run = FALSE;
	aenc_tsk_run = FALSE;
	venc_tsk_run = FALSE;
	bg_tsk_run   = FALSE;

	vos_flag_set(IAMOVIE_FLG_ID, (FLGIAMOVIE_AC_ESC | FLGIAMOVIE_AE_ESC | FLGIAMOVIE_VE_ESC));

	if (IAMOVIE_FLG_BG_ID) {
		vos_flag_set(IAMOVIE_FLG_BG_ID, FLGIAMOVIE_BG_ESC);
	}

	for (i = 0; i < MaxLinkInfo.MaxAudCapLink; i++) {
		aud_bs_cb[i] = NULL;
	}
	for (i = 0; i < MaxLinkInfo.MaxImgLink; i++) {
		vdo_bs_rec_cb[i][0] = NULL;
		vdo_bs_rec_cb[i][1] = NULL;
	}
	for (i = 0; i < MaxLinkInfo.MaxWifiLink; i++) {
		vdo_bs_strm_cb[i] = NULL;
	}
	for (i = 0; i < MaxLinkInfo.MaxEthCamLink; i++) {
		vdo_bs_ethcam_cb[i] = NULL;
	}

	#if 0
	if (_LinkCheckStatus(AudCapLinkStatus[0].acap_p[0]) || _LinkCheckStatus(AudCapLinkStatus[0].aenc_p[0])) {
		atsk_status = TRUE;
	}
	for (i = 0; i < MaxLinkInfo.MaxImgLink; i++) {
		if (_LinkCheckStatus(ImgLinkStatus[i].venc_p[0]) || _LinkCheckStatus(ImgLinkStatus[i].venc_p[1])) {
			vtsk_status = TRUE;
			break;
		}
	}
	if (atsk_status || vtsk_status) {
		while ((is_acap_tsk_running || is_aenc_tsk_running || is_venc_tsk_running || is_bg_tsk_running) && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
	}
	#else
	while ((is_acap_tsk_running || is_aenc_tsk_running || is_venc_tsk_running || is_bg_tsk_running) && delay_cnt) {
		vos_util_delay_ms(10);
		delay_cnt --;
	}
	#endif

	if (is_acap_tsk_running) {
		ImageApp_MovieMulti_DUMP("Destroy ACapPullTsk\r\n");
		vos_task_destroy(iamovie_acpull_tsk_id);
		is_acap_tsk_running = 0;
	}
	if (is_aenc_tsk_running) {
		ImageApp_MovieMulti_DUMP("Destroy AEncPullTsk\r\n");
		vos_task_destroy(iamovie_aepull_tsk_id);
		is_aenc_tsk_running = 0;
	}
	if (is_venc_tsk_running) {
		ImageApp_MovieMulti_DUMP("Destroy VEncPullTsk\r\n");
		vos_task_destroy(iamovie_vepull_tsk_id);
		is_venc_tsk_running = 0;
	}
	if (is_bg_tsk_running) {
		ImageApp_MovieMulti_DUMP("Destroy BGTsk\r\n");
		vos_task_destroy(iamovie_bg_tsk_id);
		is_bg_tsk_running = 0;
	}
	if (vos_flag_destroy(IAMOVIE_FLG_ID) != E_OK) {
		DBG_ERR("IAMOVIE_FLG_ID destroy failed.\r\n");
		ret |= E_SYS;
	}
	if (vos_flag_destroy(IAMOVIE_FLG_ID2) != E_OK) {
		DBG_ERR("IAMOVIE_FLG_ID2 destroy failed.\r\n");
		ret |= E_SYS;
	}
	if (vos_flag_destroy(IAMOVIE_FLG_BSMUX_ID) != E_OK) {
		DBG_ERR("IAMOVIE_FLG_BSMUX_ID destroy failed.\r\n");
		ret |= E_SYS;
	}
	if (IAMOVIE_FLG_BG_ID) {
		if (vos_flag_destroy(IAMOVIE_FLG_BG_ID) != E_OK) {
			DBG_ERR("IAMOVIE_FLG_BG_ID destroy failed.\r\n");
			ret |= E_SYS;
		}
	}
	vos_sem_destroy(IAMOVIE_SEMID_FM);
	vos_sem_destroy(IAMOVIE_SEMID_CRYPTO);
	vos_sem_destroy(IAMOVIE_SEMID_OPERATION);
	vos_sem_destroy(IAMOVIE_SEMID_ACAP);

	IAMOVIE_FLG_ID = 0;
	IAMOVIE_FLG_ID2 = 0;
	IAMOVIE_FLG_BSMUX_ID = 0;
	IAMOVIE_FLG_BG_ID = 0;
	IAMOVIE_SEMID_FM = 0;
	IAMOVIE_SEMID_CRYPTO = 0;
	IAMOVIE_SEMID_OPERATION = 0;
	IAMOVIE_SEMID_ACAP = 0;

	return ret;
}

