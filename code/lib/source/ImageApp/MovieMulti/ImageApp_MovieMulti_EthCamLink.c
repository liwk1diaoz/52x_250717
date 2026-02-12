#include "ImageApp_MovieMulti_int.h"

/// ========== Cross file variables ==========
UINT32 EthCamRxFuncEn = FALSE;
MOVIE_ETHCAM_LINK EthCamLink[MAX_ETHCAM_PATH];
MOVIE_ETHCAM_LINK_STATUS EthCamLinkStatus[MAX_ETHCAM_PATH] = {0};
UINT32 is_ethcamlink_opened[MAX_ETHCAM_PATH] = {0};
UINT32 is_ethcamlink_for_disp_enabled[MAX_ETHCAM_PATH][MAX_DISP_PATH] = {0};
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
UINT32 ethcam_vproc_vpe_en[MAX_ETHCAM_PATH] = {0};
MovieVpeCb *ethcam_vproc_vpe_cb[MAX_ETHCAM_PATH] = {0};
HD_VIDEOPROC_DEV_CONFIG ethcam_vproc_cfg[MAX_ETHCAM_PATH] = {0};
HD_VIDEOPROC_CTRL ethcam_vproc_ctrl[MAX_ETHCAM_PATH] = {0};
HD_VIDEOPROC_IN ethcam_vproc_in_param[MAX_ETHCAM_PATH] = {0};
HD_VIDEOPROC_FUNC_CONFIG ethcam_vproc_func_cfg[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_vproc_vpe_scene[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_vproc_queue_depth[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_vproc_no_ext[MAX_ETHCAM_PATH] = {0};
#endif
HD_VIDEOENC_PATH_CONFIG ethcam_venc_path_cfg[MAX_ETHCAM_PATH] = {0};
HD_VIDEOENC_IN  ethcam_venc_in_param[MAX_ETHCAM_PATH] = {0};
HD_VIDEOENC_OUT2 ethcam_venc_out_param[MAX_ETHCAM_PATH] = {0};
HD_H26XENC_RATE_CONTROL2 ethcam_venc_rc_param[MAX_ETHCAM_PATH] = {0};
HD_VIDEOENC_BUFINFO ethcam_venc_bs_buf[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_bs_buf_va[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_quality_base_mode_en[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_sout_type[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_sout_pa[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_sout_va[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_sout_size[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_vui_disable[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_vui_color_tv_range[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_max_frame_size[MAX_ETHCAM_PATH] = {0};
HD_COMMON_MEM_VB_BLK ethcam_venc_sout_blk[MAX_ETHCAM_PATH] = {0};
MOVIE_CFG_PROFILE ethcam_venc_h264_profile[MAX_ETHCAM_PATH] = {0};
MOVIE_CFG_LEVEL ethcam_venc_h264_level[MAX_ETHCAM_PATH] = {0};
MOVIE_CFG_PROFILE ethcam_venc_h265_profile[MAX_ETHCAM_PATH] = {0};
MOVIE_CFG_LEVEL ethcam_venc_h265_level[MAX_ETHCAM_PATH] = {0};
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
VENDOR_VDOENC_XVR_APP ethcam_venc_xvr_app[MAX_ETHCAM_PATH] = {0};
#endif
UINT32 ethcam_venc_skip_frame_cfg[MAX_ETHCAM_PATH] = {0};
HD_VIDEOENC_SVC_LAYER ethcam_venc_svc[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_low_power_mode[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_trigger_mode[MAX_ETHCAM_PATH] = {0};
UINT64 ethcam_venc_timestamp[MAX_ETHCAM_PATH] = {0};
#if (MAX_ETHCAM_PATH == 1)
UINT32 EthCamLinkVdoEncBufMs[MAX_ETHCAM_PATH] = {3000};
#elif (MAX_ETHCAM_PATH == 2)
UINT32 EthCamLinkVdoEncBufMs[MAX_ETHCAM_PATH] = {3000, 3000};
#endif
HD_VIDEOENC_MAXMEM ethcam_venc_maxmem[MAX_ETHCAM_PATH] = {0};
HD_VIDEODEC_PATH_CONFIG ethcam_vdec_cfg[MAX_ETHCAM_PATH][1] = {0};
HD_VIDEODEC_IN ethcam_vdec_in_param[MAX_ETHCAM_PATH][1] = {0};
HD_H26XDEC_DESC ethcam_vdec_desc[MAX_ETHCAM_PATH][1] = {0};
MOVIEMULTI_ETHCAM_REC_INFO EthCamLinkRecInfo[MAX_ETHCAM_PATH] = {0};
UINT32 EthCamLinkTimelapseTime[MAX_ETHCAM_PATH] = {0};
HD_BSMUX_VIDEOINFO ethcam_bsmux_vinfo[MAX_ETHCAM_PATH][2] = {0};
HD_BSMUX_AUDIOINFO ethcam_bsmux_ainfo[MAX_ETHCAM_PATH][2] = {0};
HD_BSMUX_FILEINFO ethcam_bsmux_finfo[MAX_ETHCAM_PATH][2] = {0};
UINT32 ethcam_bsmux_front_moov_en[MAX_ETHCAM_PATH][2] = {0};
UINT32 ethcam_bsmux_front_moov_flush_sec[MAX_ETHCAM_PATH][2] = {0};
UINT32 ethcam_bsmux_meta_data_en[MAX_ETHCAM_PATH][2] = {0};
UINT32 ethcam_bsmux_audio_pause[MAX_ETHCAM_PATH][2] = {0};
#if (MAX_ETHCAM_PATH == 1)
UINT32 ethcam_bsmux_bufsec[MAX_ETHCAM_PATH][2] = {{7, 7}};
#elif (MAX_ETHCAM_PATH == 2)
UINT32 ethcam_bsmux_bufsec[MAX_ETHCAM_PATH][2] = {{7, 7}, {7, 7}};
#endif
UINT32 ethcam_bsmux_cutnow_with_period_cnt[MAX_ETHCAM_PATH][2] = {0};
UINT32 ethcam_emr_loop_start[MAX_ETHCAM_PATH][1] = {0};
USIZE ethcam_venc_imgcap_thum_size[MAX_ETHCAM_PATH] = {0};
UINT32 ethcam_venc_imgcap_on_rx[MAX_ETHCAM_PATH] = {0};
/// ========== Noncross file variables ==========

ER _ImageApp_MovieMulti_EthCamLinkCfg(MOVIE_CFG_REC_ID id, UINT32 submit)
{
	UINT32 i = id, idx;

	if (i < _CFG_ETHCAM_ID_1 || i >= (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_ETHCAM_ID_1;

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	UINT32 vprc_id;
	if (submit) {
		vprc_id = _ImageApp_MovieMulti_GetFreeVprc();
	} else {
		vprc_id = MaxLinkInfo.MaxImgLink * 3 + idx;
	}
	// video process
	EthCamLink[idx].vproc_ctrl      = HD_VIDEOPROC_CTRL(vprc_id);
	EthCamLink[idx].vproc_i[0]      = HD_VIDEOPROC_IN (vprc_id, 0);
	EthCamLink[idx].vproc_o_phy[0]  = HD_VIDEOPROC_OUT(vprc_id, 0);
	EthCamLink[idx].vproc_o[0]      = HD_VIDEOPROC_OUT(vprc_id, 5);
#endif
	// video encode
	EthCamLink[idx].venc_i[0]       = HD_VIDEOENC_IN (0, MaxLinkInfo.MaxImgLink * 2 + MaxLinkInfo.MaxWifiLink * 1 + 1 /*ImgCapLink */ + idx);
	EthCamLink[idx].venc_o[0]       = HD_VIDEOENC_OUT(0, MaxLinkInfo.MaxImgLink * 2 + MaxLinkInfo.MaxWifiLink * 1 + 1 /*ImgCapLink */ + idx);
	// videodec
	EthCamLink[idx].vdec_i[0]       = HD_VIDEODEC_IN (0, 0 + idx);
	EthCamLink[idx].vdec_o[0]       = HD_VIDEODEC_OUT(0, 0 + idx);
	// bsmux
	EthCamLink[idx].bsmux_i[0]      = HD_BSMUX_IN (0, 0 + 4 * MaxLinkInfo.MaxImgLink + 2 * idx);
	EthCamLink[idx].bsmux_o[0]      = HD_BSMUX_OUT(0, 0 + 4 * MaxLinkInfo.MaxImgLink + 2 * idx);
	EthCamLink[idx].bsmux_i[1]      = HD_BSMUX_IN (0, 1 + 4 * MaxLinkInfo.MaxImgLink + 2 * idx);
	EthCamLink[idx].bsmux_o[1]      = HD_BSMUX_OUT(0, 1 + 4 * MaxLinkInfo.MaxImgLink + 2 * idx);
	// fileout
	EthCamLink[idx].fileout_i[0]    = HD_FILEOUT_IN (0, 0 + 4 * MaxLinkInfo.MaxImgLink + 2 * idx);
	EthCamLink[idx].fileout_o[0]    = HD_FILEOUT_OUT(0, 0 + 4 * MaxLinkInfo.MaxImgLink + 2 * idx);
	EthCamLink[idx].fileout_i[1]    = HD_FILEOUT_IN (0, 1 + 4 * MaxLinkInfo.MaxImgLink + 2 * idx);
	EthCamLink[idx].fileout_o[1]    = HD_FILEOUT_OUT(0, 1 + 4 * MaxLinkInfo.MaxImgLink + 2 * idx);

	return E_OK;
}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
ER _ImageApp_MovieMulti_EthCamReopenVPrc(UINT32 idx)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (ethcam_vproc_vpe_en[idx]) {
		if ((EthCamLinkStatus[idx].vproc_p_phy[0] == 0) && (EthCamLinkStatus[idx].vproc_p[0] == 0)) {
			if (EthCamLink[idx].vproc_p_phy[0]) {
				if ((hd_ret = vproc_close_ch(EthCamLink[idx].vproc_p_phy[0])) != HD_OK) {
					DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				EthCamLink[idx].vproc_p_phy[0] = 0;
			}
			if (EthCamLink[idx].vproc_p[0]) {
				if ((hd_ret = vproc_close_ch(EthCamLink[idx].vproc_p[0])) != HD_OK) {
					DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				EthCamLink[idx].vproc_p[0] = 0;
			}
			ethcam_vproc_cfg[idx].isp_id = idx; //ISP_EFFECT_ID;
			ethcam_vproc_cfg[idx].in_max.func = 0;
			ethcam_vproc_cfg[idx].in_max.dim.w = ALIGN_CEIL_16(EthCamLinkRecInfo[idx].width);
			ethcam_vproc_cfg[idx].in_max.dim.h = ALIGN_CEIL_16(EthCamLinkRecInfo[idx].height);
			ethcam_vproc_cfg[idx].in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
			ethcam_vproc_cfg[idx].in_max.frc = _ImageApp_MovieMulti_frc(1, 1);
			ethcam_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_VPE;
			ethcam_vproc_cfg[idx].ctrl_max.func = 0;

			ethcam_vproc_ctrl[idx].func = 0;
			ethcam_vproc_ctrl[idx].ref_path_3dnr = 0;

			// set dram id
			ethcam_vproc_func_cfg[idx].ddr_id = DDR_ID0; // dram_cfg.video_proc[idx];
			ethcam_vproc_func_cfg[idx].in_func = 0;

			IACOMM_VPROC_CFG vproc_cfg = {0};
			vproc_cfg.p_video_proc_ctrl = &(EthCamLink[idx].vproc_p_ctrl);
			vproc_cfg.ctrl_id           = EthCamLink[idx].vproc_ctrl;
			vproc_cfg.pcfg              = &(ethcam_vproc_cfg[idx]);
			vproc_cfg.pctrl             = &(ethcam_vproc_ctrl[idx]);
			vproc_cfg.pfunc             = &(ethcam_vproc_func_cfg[idx]);

			if ((hd_ret = vproc_set_cfg(&vproc_cfg)) != HD_OK) {
				DBG_ERR("vproc_set_cfg fail(%d)\n", hd_ret);
				ret = E_SYS;
			}

			if ((hd_ret = vendor_videoproc_set(EthCamLink[idx].vproc_p_ctrl, VENDOR_VIDEOPROC_PARAM_VPE_SCENE, &(ethcam_vproc_vpe_scene[idx]))) != HD_OK) {
				DBG_ERR("vendor_videoproc_set(VENDOR_VIDEOPROC_PARAM_VPE_SCENE)=%d fail(%d)\n", img_vproc_vpe_scene[idx], hd_ret);
			}

			ethcam_vproc_in_param[idx].func   = 0;
			ethcam_vproc_in_param[idx].dim.w  = ALIGN_CEIL_16(EthCamLinkRecInfo[idx].width);
			ethcam_vproc_in_param[idx].dim.h  = ALIGN_CEIL_16(EthCamLinkRecInfo[idx].height);
			ethcam_vproc_in_param[idx].pxlfmt = HD_VIDEO_PXLFMT_YUV420;
			ethcam_vproc_in_param[idx].dir    = HD_VIDEO_DIR_NONE;
			ethcam_vproc_in_param[idx].frc    = _ImageApp_MovieMulti_frc(1, 1);

			{
				UINT32 in_depth = ethcam_vproc_queue_depth[idx] ? ethcam_vproc_queue_depth[idx] : 2;
				if ((hd_ret = vendor_videoproc_set(EthCamLink[idx].vproc_p_ctrl, VENDOR_VIDEOPROC_PARAM_IN_DEPTH, &in_depth)) != HD_OK) {
					DBG_ERR("vendor_videoproc_set(VENDOR_VIDEOPROC_PARAM_IN_DEPTH, %d) fail(%d)\r\n", in_depth, hd_ret);
				}
			}

			IACOMM_VPROC_PARAM_IN vproc_param_in = {0};
			vproc_param_in.video_proc_path = EthCamLink[idx].vproc_i[0];
			vproc_param_in.in_param        = &(ethcam_vproc_in_param[idx]);

			if ((hd_ret = vproc_set_param_in(&vproc_param_in)) != HD_OK) {
				DBG_ERR("vproc_set_param_in fail(%d)\n", hd_ret);
				ret = E_SYS;
			}

			// physical path
			if ((hd_ret = vproc_open_ch(EthCamLink[idx].vproc_i[0], EthCamLink[idx].vproc_o_phy[0], &(EthCamLink[idx].vproc_p_phy[0]))) != HD_OK) {
				DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
				ret = E_SYS;
			}

			HD_DIM sz = {ALIGN_CEIL_16(EthCamLinkRecInfo[idx].width), ALIGN_CEIL_16(EthCamLinkRecInfo[idx].height)};
			HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
			IACOMM_VPROC_PARAM vproc_param = {0};
			IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};

			vproc_func_param.ddr_id = DDR_ID0; //dram_cfg.video_proc[idx];

			vproc_param.video_proc_path = EthCamLink[idx].vproc_p_phy[0];
			vproc_param.p_dim           = &sz;
			vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
			vproc_param.frc             = _ImageApp_MovieMulti_frc(1, 1);
			vproc_param.pfunc           = &vproc_func_param;
			if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
				DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			if (ethcam_vproc_no_ext[idx]) {
				_ImageApp_MovieMulti_UpdateVprcDepth(vproc_param.video_proc_path, 1);
			}

			if (ethcam_vproc_no_ext[idx] == FALSE) {
				// extension path
				if ((hd_ret = vproc_open_ch(EthCamLink[idx].vproc_i[0], EthCamLink[idx].vproc_o[0], &(EthCamLink[idx].vproc_p[0]))) != HD_OK) {
					DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				vproc_param_ex.video_proc_path_ex         = EthCamLink[idx].vproc_p[0];
				vproc_param_ex.video_proc_path_src        = EthCamLink[idx].vproc_p_phy[0];
				vproc_param_ex.p_dim                      = &sz;
				vproc_param_ex.pxlfmt                     = HD_VIDEO_PXLFMT_YUV420;
				vproc_param_ex.dir                        = HD_VIDEO_DIR_NONE;
				vproc_param_ex.frc                        = _ImageApp_MovieMulti_frc(1, 1);
				vproc_param_ex.depth                      = 1;
				vproc_param_ex.video_proc_crop_param.mode = HD_CROP_OFF;
				if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
					DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
			}
		}
	}
	return ret;
}
#endif

static ER _ImageApp_MovieMulti_SetEthCamSrcOutBuf(UINT32 idx, UINT32 tbl)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	VENDOR_VIDEOENC_COMM_SRCOUT comm_srcout = {0};
	UINT32 w, h;
	UINT32 pa, blk_size;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id = dram_cfg.video_encode;

	if (ethcam_venc_sout_type[idx] != MOVIE_IMGCAP_SOUT_NONE) {
		if (EthCamLinkRecInfo[idx].codec == _CFG_CODEC_H264) {
			w = ALIGN_CEIL_16(EthCamLinkRecInfo[idx].width);
			h = ALIGN_CEIL_16(EthCamLinkRecInfo[idx].height);
		} else {
			w = ALIGN_CEIL_64(EthCamLinkRecInfo[idx].width);
			h = ALIGN_CEIL_64(EthCamLinkRecInfo[idx].height);
		}
		blk_size = w * h * 3 / 2;
		if (ethcam_venc_sout_type[idx] == MOVIE_IMGCAP_SOUT_PRIVATE) {
			if ((hd_ret = hd_common_mem_alloc("soutbuf_e", &pa, (void **)&va, blk_size, ddr_id)) != HD_OK) {
				ret = E_SYS;
				DBG_ERR("Alloc source out buffer fail(%d)! codec=%d, w=%d(%d), h=%d(%d), wanted size=%x\r\n", hd_ret, EthCamLinkRecInfo[idx].codec, EthCamLinkRecInfo[idx].width, w, EthCamLinkRecInfo[idx].height, h, blk_size);
				ethcam_venc_sout_va[idx] = 0;
				ethcam_venc_sout_pa[idx] = 0;
				ethcam_venc_sout_size[idx] = 0;
				return ret;
			} else {
				ethcam_venc_sout_va[idx] = (UINT32)va;
				ethcam_venc_sout_pa[idx] = (UINT32)pa;
				ethcam_venc_sout_size[idx] = blk_size;
			}
			DBG_DUMP("Alloc source out buffer OK! codec=%d, w=%d(%d), h=%d(%d), wanted size=%x\r\n", EthCamLinkRecInfo[idx].codec, EthCamLinkRecInfo[idx].width, w, EthCamLinkRecInfo[idx].height, h, blk_size);
		} else if (ethcam_venc_sout_type[idx] == MOVIE_IMGCAP_SOUT_COMMON) {
			if ((ethcam_venc_sout_blk[idx] = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id)) == HD_COMMON_MEM_VB_INVALID_BLK) {
				ret = E_SYS;
				DBG_ERR("Get source out buffer fail(%d)! codec=%d, w=%d(%d), h=%d(%d), wanted size=%x\r\n", ethcam_venc_sout_blk[idx], EthCamLinkRecInfo[idx].codec, EthCamLinkRecInfo[idx].width, w, EthCamLinkRecInfo[idx].height, h, blk_size);
				ethcam_venc_sout_blk[idx] = 0;
				return ret;
			}
			if ((pa = hd_common_mem_blk2pa(ethcam_venc_sout_blk[idx])) == 0) {
				ret = E_SYS;
				DBG_ERR("hd_common_mem_blk2pa fail\r\n");
				if ((hd_ret = hd_common_mem_release_block(ethcam_venc_sout_blk[idx])) != HD_OK) {
					DBG_ERR("hd_common_mem_release_block fail(%d)\r\n", hd_ret);
				}
				ethcam_venc_sout_blk[idx] = 0;
				return ret;
			} else {
				ethcam_venc_sout_pa[idx] = (UINT32)pa;
				ethcam_venc_sout_size[idx] = blk_size;
			}
			DBG_DUMP("Get source out buffer OK(%x)! codec=%d, w=%d(%d), h=%d(%d), wanted size=%x\r\n", ethcam_venc_sout_blk[idx], EthCamLinkRecInfo[idx].codec, EthCamLinkRecInfo[idx].width, w, EthCamLinkRecInfo[idx].height, h, blk_size);
		} else if (ethcam_venc_sout_type[idx] == MOVIE_IMGCAP_SOUT_SHARED_PRIVATE) {
			UINT32 buf_pa, buf_va;
			if ((ret = _ImageApp_MovieMulti_AllocSharedSrcOutBuf(ddr_id, blk_size, &buf_pa, &buf_va)) != E_OK) {
				return ret;
			} else {
				pa = buf_pa;
			}
		}
		comm_srcout.b_enable = 1;
		comm_srcout.b_no_jpeg_enc = 1;
		comm_srcout.addr = pa;
		comm_srcout.size = blk_size;
		if ((hd_ret = vendor_videoenc_set(EthCamLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_COMM_SRCOUT, &comm_srcout)) != HD_OK) {
			ret = E_SYS;
			DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_COMM_SRCOUT) failed(%d)\r\n", hd_ret);
		}
		ethcam_venc_path_cfg[idx].max_mem.source_output = 1;
		ethcam_venc_out_param[idx].h26x.source_output = 1;
	} else {
		ethcam_venc_path_cfg[idx].max_mem.source_output = 0;
		ethcam_venc_out_param[idx].h26x.source_output = 0;
	}
	return ret;
}

static ER _ImageApp_MovieMulti_ReleaseEthCamSrcOutBuf(UINT32 idx, UINT32 tbl)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (ethcam_venc_sout_type[idx] != MOVIE_IMGCAP_SOUT_NONE) {
		if (ethcam_venc_sout_type[idx] == MOVIE_IMGCAP_SOUT_PRIVATE) {
			if (ethcam_venc_sout_va[idx]) {
				if ((hd_ret = hd_common_mem_free(ethcam_venc_sout_pa[idx], (void *)ethcam_venc_sout_va[idx])) != HD_OK) {
					DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
					ret = E_SYS;
				}
				ethcam_venc_sout_va[idx] = 0;
				ethcam_venc_sout_pa[idx] = 0;
				ethcam_venc_sout_size[idx] = 0;
			}
		} else if (ethcam_venc_sout_type[idx] == MOVIE_IMGCAP_SOUT_COMMON) {
			if (ethcam_venc_sout_blk[idx] && ethcam_venc_sout_blk[idx] != HD_COMMON_MEM_VB_INVALID_BLK) {
				if ((hd_ret = hd_common_mem_release_block(ethcam_venc_sout_blk[idx])) != HD_OK) {
					DBG_ERR("hd_common_mem_release_block(%d) failed(%d)\r\n", ethcam_venc_sout_blk[idx], hd_ret);
					ret = E_SYS;
				}
				ethcam_venc_sout_blk[idx] = 0;
			}
		} else if (ethcam_venc_sout_type[idx] == MOVIE_IMGCAP_SOUT_SHARED_PRIVATE) {
			_ImageApp_MovieMulti_FreeSharedSrcOutBuf();
		}
	}
	return ret;
}

ER _ImageApp_MovieMulti_EthCamSetVEncParam(UINT32 idx)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
	VENDOR_VIDEOENC_H26X_XVR_CFG h26x_xvr = {0};
	h26x_xvr.xvr_app = ethcam_venc_xvr_app[idx];
	if ((hd_ret = vendor_videoenc_set(EthCamLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_H26X_XVR, &h26x_xvr)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_H26X_XVR) fail(%d)\n", hd_ret);
	}
#endif

	if (ethcam_venc_imgcap_on_rx[idx] == TRUE) {
		_ImageApp_MovieMulti_SetEthCamSrcOutBuf(idx, 0);
	}

	HD_VIDEOENC_FUNC_CONFIG ethcam_venc_func_cfg = {0};
	ethcam_venc_func_cfg.ddr_id  = dram_cfg.video_encode;
	ethcam_venc_func_cfg.in_func = 0;
	IACOMM_VENC_CFG venc_cfg = {
		.video_enc_path = EthCamLink[idx].venc_p[0],
		.ppath_cfg      = &(ethcam_venc_path_cfg[idx]),
		.pfunc_cfg      = &(ethcam_venc_func_cfg),
	};
	if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
		DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	IACOMM_VENC_PARAM venc_param = {0};
	venc_param.video_enc_path = EthCamLink[idx].venc_p[0];
	venc_param.pin            = &(ethcam_venc_in_param[idx]);
	venc_param.pout           = &(ethcam_venc_out_param[idx]);
	venc_param.prc            = &(ethcam_venc_rc_param[idx]);
	if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
		DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	HD_H26XENC_VUI vui_param = {0};
	vui_param.vui_en                     = ethcam_venc_vui_disable[idx] ? FALSE : TRUE;              ///< enable vui. default: 0, range: 0~1 (0: disable, 1: enable)
	vui_param.sar_width                  = 0;    /// 0: Unspecified                                  ///< Horizontal size of the sample aspect ratio. default: 0, range: 0~65535
	vui_param.sar_height                 = 0;    /// 0: Unspecified                                  ///< Vertical size of the sample aspect rat. default: 0, range: 0~65535
	vui_param.matrix_coef                = 1;    /// 1: bt.709                                       ///< Matrix coefficients are used to derive the luma and Chroma signals from green, blue, and red primaries. default: 2, range: 0~255
	vui_param.transfer_characteristics   = 1;    /// 1: bt.709                                       ///< The opto-electronic transfers characteristic of the source pictures. default: 2, range: 0~255
	vui_param.colour_primaries           = 1;    /// 1: bt.709                                       ///< Chromaticity coordinates the source primaries. default: 2, range: 0~255
	vui_param.video_format               = 5;    /// 5: Unspecified video format                     ///< Indicate the representation of pictures. default: 5, range: 0~7
	vui_param.color_range                = ethcam_venc_vui_color_tv_range[idx] ? FALSE : TRUE;       ///< Indicate the black level and range of the luma and Chroma signals. default: 0, range: 0~1 (0: Not full range, 1: Full range)
	vui_param.timing_present_flag        = 1;    /// 1: enabled

	if (vui_param.vui_en && g_ia_moviemulti_usercb) {
		UINT32 vui_en = vui_param.vui_en;
		UINT32 color_range = vui_param.color_range;
		UINT32 id = _CFG_ETHCAM_ID_1 + idx;
		g_ia_moviemulti_usercb(id, MOVIE_USER_CB_EVENT_SET_ENC_VUI, (UINT32)&vui_param);
		if ((vui_en != vui_param.vui_en) || color_range != vui_param.color_range) {
			DBG_WRN("id%d: vui_en(%d/%d) or color_range(%d/%d) has been changed\r\n", id, vui_en, vui_param.vui_en, color_range, vui_param.color_range);
		}
	}

	if ((hd_ret = hd_videoenc_set(EthCamLink[idx].venc_p[0], HD_VIDEOENC_PARAM_OUT_VUI, &vui_param)) != HD_OK) {
		DBG_ERR("hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_VUI) fail(%d)\n", hd_ret);
	}

	VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_size = {0};
	bs_reserved_size.reserved_size = (EthCamLinkRecInfo[idx].rec_format == _CFG_FILE_FORMAT_TS) ? VENC_BS_RESERVED_SIZE_TS : VENC_BS_RESERVED_SIZE_MP4;
	if ((hd_ret = vendor_videoenc_set(EthCamLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_size)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	VENDOR_VIDEOENC_TIMELAPSE_TIME_CFG timelapse_cfg = {0};
	timelapse_cfg.timelapse_time = (EthCamLinkRecInfo[idx].rec_mode == _CFG_REC_TYPE_TIMELAPSE) ? EthCamLinkTimelapseTime[idx] : 0;
	if ((hd_ret = vendor_videoenc_set(EthCamLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_TIMELAPSE_TIME, &timelapse_cfg)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_TIMELAPSE_TIME) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	VENDOR_VIDEOENC_LONG_START_CODE lsc = {0};
	lsc.long_start_code_en = ENABLE;
	if ((hd_ret = vendor_videoenc_set(EthCamLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE, &lsc)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	VENDOR_VIDEOENC_QUALITY_BASE_MODE qbm = {0};
	qbm.quality_base_en = ethcam_venc_quality_base_mode_en[idx];
	if ((hd_ret = vendor_videoenc_set(EthCamLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_QUALITY_BASE, &qbm)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_QUALITY_BASE) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
	VENDOR_VIDEOENC_H26X_LOW_POWER_CFG h26x_low_power = {0};
	h26x_low_power.b_enable = ethcam_venc_low_power_mode[idx];
	if ((hd_ret = vendor_videoenc_set(EthCamLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_H26X_LOW_POWER, &h26x_low_power)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_H26X_LOW_POWER) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
#endif

	return ret;
}

ER _ImageApp_MovieMulti_EthCamReopenVEnc(UINT32 idx)
{
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if (EthCamLinkStatus[idx].venc_p[0] == 0) {
		if (ethcam_venc_imgcap_on_rx[idx] == TRUE) {
			_ImageApp_MovieMulti_ReleaseEthCamSrcOutBuf(idx, 0);
		}
		if ((hd_ret = venc_close_ch(EthCamLink[idx].venc_p[0])) != HD_OK) {
			DBG_ERR("venc_close_ch fail(%d:%x)(%d)\n", idx, EthCamLink[idx].venc_p[0], hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = venc_open_ch(EthCamLink[idx].venc_i[0], EthCamLink[idx].venc_o[0], &(EthCamLink[idx].venc_p[0]))) != HD_OK) {
			DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		_ImageApp_MovieMulti_EthCamSetVEncParam(idx);
		if (ethcam_venc_bs_buf_va[idx]) {
			hd_common_mem_munmap((void *)ethcam_venc_bs_buf_va[idx], ethcam_venc_bs_buf[idx].buf_info.buf_size);
			ethcam_venc_bs_buf_va[idx] = 0;
		}
		ethcam_venc_bs_buf[idx].buf_info.phy_addr = 0;
		ethcam_venc_bs_buf[idx].buf_info.buf_size = 0;
	} else {
		ret = E_SYS;
	}
	return ret;
}

static ER _ImageApp_MovieMulti_EthCamLink(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j;
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	// set videoprc
	_ImageApp_MovieMulti_EthCamReopenVPrc(idx);
#endif

	// set videoenc
	if ((hd_ret = venc_open_ch(EthCamLink[idx].venc_i[0], EthCamLink[idx].venc_o[0], &(EthCamLink[idx].venc_p[0]))) != HD_OK) {
		DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	_ImageApp_MovieMulti_EthCamSetVEncParam(idx);
	if (ethcam_venc_bs_buf_va[idx]) {
		hd_common_mem_munmap((void *)ethcam_venc_bs_buf_va[idx], ethcam_venc_bs_buf[idx].buf_info.buf_size);
		ethcam_venc_bs_buf_va[idx] = 0;
	}
	ethcam_venc_bs_buf[idx].buf_info.phy_addr = 0;
	ethcam_venc_bs_buf[idx].buf_info.buf_size = 0;

	// set videodec
	if ((hd_ret = vdec_open_ch(EthCamLink[idx].vdec_i[0], EthCamLink[idx].vdec_o[0], &(EthCamLink[idx].vdec_p[0]))) != HD_OK) {
		DBG_ERR("vdec_open_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	IACOMM_VDEC_CFG vdec_cfg = {
		.video_dec_path = EthCamLink[idx].vdec_p[0],
		.pcfg           = &(ethcam_vdec_cfg[idx][0]),
	};
	if ((hd_ret = vdec_set_cfg(&vdec_cfg)) != HD_OK) {
		DBG_ERR("vdec_set_cfg fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	IACOMM_VDEC_PARAM vdec_param = {
		.video_dec_path = EthCamLink[idx].vdec_p[0],
		.pin            = &(ethcam_vdec_in_param[idx][0]),
		.pdesc          = &(ethcam_vdec_desc[idx][0]),
	};
	if ((hd_ret = vdec_set_param(&vdec_param)) != HD_OK) {
		DBG_ERR("vdec_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	VENDOR_VIDEODEC_YUV_AUTO_DROP auto_drop = {
		.enable = 1,
	};
	if ((hd_ret = vendor_videodec_set(EthCamLink[idx].vdec_p[0], VENDOR_VIDEODEC_PARAM_IN_YUV_AUTO_DROP, &auto_drop)) != HD_OK) {
		DBG_ERR("vendor_videodec_set(VENDOR_VIDEODEC_PARAM_IN_YUV_AUTO_DROP) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	VENDOR_VIDEODEC_RAWQUE_MAX_NUM raw_que = {
		.rawque_max_num = 2,
	};
	if ((hd_ret = vendor_videodec_set(EthCamLink[idx].vdec_p[0], VENDOR_VIDEODEC_PARAM_IN_RAWQUE_MAX_NUM, &raw_que)) != HD_OK) {
		DBG_ERR("vendor_videodec_set(VENDOR_VIDEODEC_PARAM_IN_RAWQUE_MAX_NUM) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// set bsmux
	IACOMM_BSMUX_PARAM bsmux_param;
	HD_BSMUX_EN_UTIL util = {0};
	for (j = 0; j < 2; j++) {
		bsmux_open_ch(EthCamLink[idx].bsmux_i[j], EthCamLink[idx].bsmux_o[j], &(EthCamLink[idx].bsmux_p[j]));

		bsmux_param.path_id = EthCamLink[idx].bsmux_p[j];
		bsmux_param.id      = HD_BSMUX_PARAM_VIDEOINFO;
		bsmux_param.p_param = (void*)&(ethcam_bsmux_vinfo[idx][j]);
		bsmux_set_param(&bsmux_param);

		bsmux_param.path_id = EthCamLink[idx].bsmux_p[j];
		bsmux_param.id      = HD_BSMUX_PARAM_AUDIOINFO;
		bsmux_param.p_param = (void*)&(ethcam_bsmux_ainfo[idx][j]);
		bsmux_set_param(&bsmux_param);

		bsmux_param.path_id = EthCamLink[idx].bsmux_p[j];
		bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
		bsmux_param.p_param = (void*)&(ethcam_bsmux_finfo[idx][j]);
		bsmux_set_param(&bsmux_param);

		bsmux_param.path_id = EthCamLink[idx].bsmux_p[j];
		bsmux_param.id      = HD_BSMUX_PARAM_REG_CALLBACK;
		bsmux_param.p_param = (void*)_ImageApp_MovieMulti_BsMux_CB;
		bsmux_set_param(&bsmux_param);

		UINT32 tbl = j / 2;
		util.type = HD_BSMUX_EN_UTIL_GPS_DATA;
		util.enable = gpsdata_eth_disable[idx][tbl] ? FALSE : TRUE;
		util.resv[0] = gpsdata_eth_rate[idx][tbl] ? gpsdata_eth_rate[idx][tbl] : 1;
		hd_bsmux_set(EthCamLink[idx].bsmux_p[j], HD_BSMUX_PARAM_EN_UTIL, &util);

		util.type = HD_BSMUX_EN_UTIL_DUR_LIMIT;
		util.enable = TRUE;
		util.resv[0] = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? ((ImgLinkFileInfo.emr_sec * 1000000) + IAMOVIE_TIMESTAMP_WARNING_TIME) : 0;
		hd_bsmux_set(EthCamLink[idx].bsmux_p[j], HD_BSMUX_PARAM_EN_UTIL, &util);
	}

	HD_PATH_ID bsmux_ctrl_path = 0;
	bsmux_get_ctrl_path(0, &bsmux_ctrl_path);
    util.type = HD_BSMUX_EN_UTIL_PTS_RESET ;
    util.enable = TRUE;
    hd_bsmux_set(bsmux_ctrl_path, HD_BSMUX_PARAM_EN_UTIL, &util);

	// set fileout
	IACOMM_FILEOUT_PARAM fileout_param;
	for (j = 0; j < 2; j++) {
		fileout_open_ch(EthCamLink[idx].fileout_i[j], EthCamLink[idx].fileout_o[j], &(EthCamLink[idx].fileout_p[j]));

		fileout_param.path_id = EthCamLink[idx].fileout_p[j];
		fileout_param.id = HD_FILEOUT_PARAM_REG_CALLBACK;
		fileout_param.p_param = (void*)_ImageApp_MovieMulti_FileOut_CB;
		fileout_set_param(&fileout_param);
		DBG_IND(">>> fileout reg callback %d\r\n", (int)_ImageApp_MovieMulti_FileOut_CB);
	}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	// bind vprc-venc
	if ((hd_ret = vproc_bind(EthCamLink[idx].vproc_o[0], EthCamLink[idx].venc_i[0])) != HD_OK) {
		DBG_ERR("vproc_bind fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
#endif

	return ret;
}

static ER _ImageApp_MovieMulti_EthCamUnlink(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j;
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	// unbind vprc-venc
	if ((hd_ret = vproc_unbind(EthCamLink[idx].vproc_o[0])) != HD_OK) {
		DBG_ERR("vproc_unbind fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// videoprc
	if ((hd_ret = vproc_close_ch(EthCamLink[idx].vproc_p_phy[0])) != HD_OK) {
		DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	EthCamLink[idx].vproc_p_phy[0] = 0;
	if ((hd_ret = vproc_close_ch(EthCamLink[idx].vproc_p[0])) != HD_OK) {
		DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	EthCamLink[idx].vproc_p[0] = 0;
#endif

	// videoenc
	if ((hd_ret = venc_close_ch(EthCamLink[idx].venc_p[0])) != HD_OK) {
		DBG_ERR("venc_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// videodec
	if ((hd_ret = vdec_close_ch(EthCamLink[idx].vdec_p[0])) != HD_OK) {
		DBG_ERR("vdec_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// set bsmux
	for (j = 0; j < 2; j++) {
		if ((ret = bsmux_close_ch(EthCamLink[idx].bsmux_p[j])) != HD_OK) {
			DBG_ERR("bsmux_close_ch fail(%d)\n", ret);
			ret = E_SYS;
		}
	}

	// set fileout
	for (j = 0; j < 2; j++) {
		if ((ret = fileout_close_ch(EthCamLink[idx].fileout_p[j])) != HD_OK) {
			DBG_ERR("fileout_close_ch fail(%d)\n", ret);
			ret = E_SYS;
		}
	}
	return ret;
}

ER _ImageApp_MovieMulti_EthCamLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction)
{
	UINT32 i = id, idx;
	ER ret = E_OK;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}

	if (direction == UPDATE_FORWARD) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		// set videoprc
		ret |= _LinkUpdateStatus(vproc_set_state,     EthCamLink[idx].vproc_p_phy[0],  &(EthCamLinkStatus[idx].vproc_p_phy[0]),   NULL);
		ret |= _LinkUpdateStatus(vproc_set_state,     EthCamLink[idx].vproc_p[0],      &(EthCamLinkStatus[idx].vproc_p[0]),       NULL);
#endif
		// set videoenc
		ret |= _LinkUpdateStatus(venc_set_state,      EthCamLink[idx].venc_p[0],       &(EthCamLinkStatus[idx].venc_p[0]), NULL);
		// set videodec
		ret |= _LinkUpdateStatus(vdec_set_state,      EthCamLink[idx].vdec_p[0],       &(EthCamLinkStatus[idx].vdec_p[0]), NULL);
		// set bsmux
		ret |= _LinkUpdateStatus(bsmux_set_state,     EthCamLink[idx].bsmux_p[0],      &(EthCamLinkStatus[idx].bsmux_p[0]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     EthCamLink[idx].bsmux_p[1],      &(EthCamLinkStatus[idx].bsmux_p[1]),       NULL);
		// set fileout
		ret |= _LinkUpdateStatus(fileout_set_state,   EthCamLink[idx].fileout_p[0],    &(EthCamLinkStatus[idx].fileout_p[0]),     NULL);
		ret |= _LinkUpdateStatus(fileout_set_state,   EthCamLink[idx].fileout_p[1],    &(EthCamLinkStatus[idx].fileout_p[1]),     NULL);
	} else {
		// set fileout
		ret |= _LinkUpdateStatus(fileout_set_state,   EthCamLink[idx].fileout_p[0],    &(EthCamLinkStatus[idx].fileout_p[0]),     NULL);
		ret |= _LinkUpdateStatus(fileout_set_state,   EthCamLink[idx].fileout_p[1],    &(EthCamLinkStatus[idx].fileout_p[1]),     NULL);
		// set bsmux
		ret |= _LinkUpdateStatus(bsmux_set_state,     EthCamLink[idx].bsmux_p[0],      &(EthCamLinkStatus[idx].bsmux_p[0]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     EthCamLink[idx].bsmux_p[1],      &(EthCamLinkStatus[idx].bsmux_p[1]),       NULL);
		// set videodec
		ret |= _LinkUpdateStatus(vdec_set_state,      EthCamLink[idx].vdec_p[0],       &(EthCamLinkStatus[idx].vdec_p[0]), NULL);
		// set videoenc
		ret |= _LinkUpdateStatus(venc_set_state,      EthCamLink[idx].venc_p[0],       &(EthCamLinkStatus[idx].venc_p[0]), NULL);
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		// set videoprc
		ret |= _LinkUpdateStatus(vproc_set_state,     EthCamLink[idx].vproc_p_phy[0],  &(EthCamLinkStatus[idx].vproc_p_phy[0]),   NULL);
		ret |= _LinkUpdateStatus(vproc_set_state,     EthCamLink[idx].vproc_p[0],      &(EthCamLinkStatus[idx].vproc_p[0]),       NULL);
#endif
	}
	return ret;
}

ER _ImageApp_MovieMulti_EthCamLinkOpen(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}
	if (is_ethcamlink_opened[idx] == TRUE) {
		DBG_ERR("EthCam[%d] is already opened!\r\n", idx);
		return E_SYS;
	}

	_ImageApp_MovieMulti_EthCamLinkCfg(i, TRUE);

	// init variables
	for (j = 0; j < 2; j++) {
		ethcam_bsmux_cutnow_with_period_cnt[idx][j] = UNUSED;
	}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	if (ethcam_vproc_vpe_cb[idx]) {
		ethcam_vproc_vpe_cb[idx](i, MOVIE_VPE_STATE_INIT, NULL);
	}
#endif

	_ImageApp_MovieMulti_SetupRecParam(i);
	_ImageApp_MovieMulti_SetupFileOption(i);

	_ImageApp_MovieMulti_EthCamLink(i);
	is_ethcamlink_opened[idx] = TRUE;

	return E_OK;
}

ER _ImageApp_MovieMulti_EthCamLinkClose(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}

	if (is_ethcamlink_opened[idx] == FALSE) {
		DBG_ERR("EthCam[%d] is already closed!\r\n", idx);
		return E_SYS;
	}

	// check whether main record path is still running
	if (_LinkCheckStatus(EthCamLinkStatus[idx].fileout_p[0])) {
		// stop main record
	}
	// check whether clone record path is still running
	if (_LinkCheckStatus(EthCamLinkStatus[idx].fileout_p[1])) {
		// stop clone record
	}
	// check whether display path is still running
	if (_LinkCheckStatus(EthCamLinkStatus[idx].vdec_p[0])) {
		// stop decode
	}

	_ImageApp_MovieMulti_EthCamUnlink(i);

	// release buffer
	if (ethcam_venc_imgcap_on_rx[idx] == TRUE) {
		_ImageApp_MovieMulti_ReleaseEthCamSrcOutBuf(idx, 0);
	}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	if (ethcam_vproc_vpe_cb[idx]) {
		ethcam_vproc_vpe_cb[idx](i, MOVIE_VPE_STATE_UNINIT, NULL);
	}
#endif

	// reset variables
	ethcam_emr_loop_start[idx][0] = 0;
	ethcam_venc_imgcap_thum_size[idx].w = 0;
	ethcam_venc_imgcap_thum_size[idx].h = 0;
	ethcam_venc_imgcap_on_rx[idx] = 0;

	ethcam_venc_vui_disable[idx] = 0;
	ethcam_venc_vui_color_tv_range[idx] = 0;
	ethcam_venc_max_frame_size[idx] = 0;
	ethcam_venc_h264_profile[idx] = 0;
	ethcam_venc_h264_level[idx] = 0;
	ethcam_venc_h265_profile[idx] = 0;
	ethcam_venc_h265_level[idx] = 0;
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
	ethcam_venc_xvr_app[idx] = 0;
#endif
	ethcam_venc_skip_frame_cfg[idx] = 0;
	ethcam_venc_svc[idx] = 0;
	ethcam_venc_low_power_mode[idx] = 0;
	ethcam_venc_trigger_mode[idx] = MOVIE_CODEC_TRIGGER_TIMER;
	if (ethcam_venc_bs_buf_va[idx]) {
		hd_common_mem_munmap((void *)ethcam_venc_bs_buf_va[idx], ethcam_venc_bs_buf[idx].buf_info.buf_size);
		ethcam_venc_bs_buf_va[idx] = 0;
	}
	ethcam_venc_bs_buf[idx].buf_info.phy_addr = 0;
	ethcam_venc_bs_buf[idx].buf_info.buf_size = 0;

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	ethcam_vproc_vpe_en[idx] = 0;
	ethcam_vproc_vpe_cb[idx] = 0;
	ethcam_vproc_vpe_scene[idx] = 0;
	ethcam_vproc_queue_depth[idx] = 0;
	ethcam_vproc_no_ext[idx] = 0;
#endif

	for (j = 0; j < 2; j++) {
		ethcam_bsmux_finfo[idx][j].ddr_id = DDR_ID0;
		ethcam_bsmux_meta_data_en[idx][j] = 0;
		ethcam_bsmux_audio_pause[idx][j] = 0;
	}
	memset((void *)&(ethcam_venc_maxmem[idx]), 0, sizeof(HD_VIDEOENC_MAXMEM));

	is_ethcamlink_opened[idx] = FALSE;

	return E_OK;
}

ER ImageApp_MovieMulti_EthCamLinkForDisp(MOVIE_CFG_REC_ID id, UINT32 action, BOOL allow_pull)
{
	UINT32 i = id, idx;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}
	if (is_ethcamlink_opened[idx] == FALSE) {
		return E_SYS;
	}

	if ((is_ethcamlink_for_disp_enabled[idx][0] == TRUE) && (action == ENABLE)) {
		DBG_ERR("EthCamLink %d is already enabled!\r\n", i);
		return E_SYS;
	}
	if ((is_ethcamlink_for_disp_enabled[idx][0] == FALSE) && (action == DISABLE)) {
		DBG_ERR("EthCamLink %d is already disabled!\r\n", i);
		return E_SYS;
	}
	if (!ethcam_vdec_cfg[idx][0].max_mem.dim.w || !ethcam_vdec_cfg[idx][0].max_mem.dim.h ||
		!ethcam_vdec_in_param[idx][0].codec_type ||
		!ethcam_vdec_desc[idx][0].addr || !ethcam_vdec_desc[idx][0].len) {
		DBG_ERR("Dec parameter not configured!!!\r\n");
		return E_SYS;
	}

	if (action == ENABLE) {
		_LinkPortCntInc(&(EthCamLinkStatus[idx].vdec_p[0]));
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (ethcam_vproc_vpe_en[idx]) {
			_LinkPortCntInc(&(EthCamLinkStatus[idx].vproc_p_phy[0]));
			_LinkPortCntInc(&(EthCamLinkStatus[idx].vproc_p[0]));
		}
#endif
		is_ethcamlink_for_disp_enabled[idx][0] = TRUE;
	} else if (action == DISABLE) {
		_LinkPortCntDec(&(EthCamLinkStatus[idx].vdec_p[0]));
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (ethcam_vproc_vpe_en[idx]) {
			_LinkPortCntDec(&(EthCamLinkStatus[idx].vproc_p_phy[0]));
			_LinkPortCntDec(&(EthCamLinkStatus[idx].vproc_p[0]));
		}
#endif
		is_ethcamlink_for_disp_enabled[idx][0] = FALSE;
	}
	_ImageApp_MovieMulti_EthCamLinkStatusUpdate(i, (action == ENABLE) ? UPDATE_REVERSE : UPDATE_FORWARD);

	return E_OK;
}

UINT32 ImageApp_MovieMulti_EthCamLinkForDispStatus(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return FALSE;
	}
	if (is_ethcamlink_opened[idx] == FALSE) {
		return FALSE;
	}
	return is_ethcamlink_for_disp_enabled[idx][0];
}

ER ImageApp_MovieMulti_EthCamLinkPushVdoBsToDecoder(MOVIE_CFG_REC_ID id, HD_VIDEODEC_BS *p_video_bs)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 i = id, idx;
	HD_COMMON_MEM_VB_BLK blk;
	HD_COMMON_MEM_DDR_ID ddr_id = dram_cfg.video_decode;
	UINT32 blk_size = 0, pa = 0;
	HD_VIDEO_FRAME video_frame = {0};

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}

	if (is_ethcamlink_opened[idx] == FALSE) {
		DBG_ERR("EthCam[%d] is already closed!\r\n", idx);
		return E_SYS;
	}

	if (ethcam_vdec_in_param[idx][0].codec_type == HD_CODEC_TYPE_H265 || ethcam_vdec_in_param[idx][0].codec_type == HD_CODEC_TYPE_JPEG) {
		blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(ALIGN_CEIL_64(ethcam_vdec_cfg[idx][0].max_mem.dim.w), ALIGN_CEIL_64(ethcam_vdec_cfg[idx][0].max_mem.dim.h), HD_VIDEO_PXLFMT_YUV420);  // align to 64 for h265 raw buffer
	} else if (ethcam_vdec_in_param[idx][0].codec_type == HD_CODEC_TYPE_H264) {
		blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(ALIGN_CEIL_64(ethcam_vdec_cfg[idx][0].max_mem.dim.w), ALIGN_CEIL_64(ethcam_vdec_cfg[idx][0].max_mem.dim.h), HD_VIDEO_PXLFMT_YUV420);  // align to 64 for h265 raw buffer
	}
	if ((blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id)) == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("hd_common_mem_get_block fail, blk(0x%x)\n", blk);
		return E_NOMEM;
	}
	if ((pa = hd_common_mem_blk2pa(blk)) == 0) {
		DBG_ERR("hd_common_mem_blk2pa fail, blk(0x%x)\n", blk);
		return E_SYS;
	}
	video_frame.sign = MAKEFOURCC('V', 'F', 'R', 'M');
	video_frame.ddr_id = ddr_id;
	video_frame.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_frame.dim.w = ALIGN_CEIL_64(ethcam_vdec_cfg[idx][0].max_mem.dim.w);
	video_frame.dim.h = ALIGN_CEIL_64(ethcam_vdec_cfg[idx][0].max_mem.dim.h);
	video_frame.phy_addr[0] = pa;
	video_frame.blk = blk;
	if ((hd_ret = hd_videodec_push_in_buf(EthCamLink[idx].vdec_p[0], p_video_bs, &video_frame, -1)) != HD_OK) {
		DBG_ERR("hd_videodec_push_in_buf fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = hd_videodec_release_out_buf(EthCamLink[idx].vdec_p[0], &video_frame)) != HD_OK) {
		DBG_ERR("hd_videodec_release_out_buf fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	return ret;
}

ER ImageApp_MovieMulti_EthCamLinkPushVdoBsToBsMux(MOVIE_CFG_REC_ID id, HD_VIDEOENC_BS *p_video_bs)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 i = id, idx;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}

	if (is_ethcamlink_opened[idx] == FALSE) {
		DBG_ERR("EthCam[%d] is already closed!\r\n", idx);
		return E_SYS;
	}

	if (_LinkCheckStatus(EthCamLinkStatus[idx].bsmux_p[0])) {
		if ((hd_ret = hd_bsmux_push_in_buf_video(EthCamLink[idx].bsmux_p[0], p_video_bs, -1)) != HD_OK) {
			DBG_ERR("hd_bsmux_push_in_buf_video(0) fail(%d)\r\n", hd_ret);
			ret = E_SYS;
		}
	}
	if (_LinkCheckStatus(EthCamLinkStatus[idx].bsmux_p[1])) {
		if ((hd_ret = hd_bsmux_push_in_buf_video(EthCamLink[idx].bsmux_p[1], p_video_bs, -1)) != HD_OK) {
			DBG_ERR("hd_bsmux_push_in_buf_video(1) fail(%d)\r\n", hd_ret);
			ret = E_SYS;
		}
	}
	return ret;
}

ER ImageApp_MovieMulti_EthCamLinkPushJpgToFileOut(MOVIE_CFG_REC_ID id, HD_FILEOUT_BUF *p_jpg)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 i = id, idx;

	if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
		idx = i - _CFG_ETHCAM_ID_1;
	} else {
		return E_SYS;
	}

	if (is_ethcamlink_opened[idx] == FALSE) {
		DBG_ERR("EthCam[%d] is already closed!\r\n", idx);
		return E_SYS;
	}

	if (_LinkCheckStatus(EthCamLinkStatus[idx].fileout_p[0])) {
		if ((hd_ret = hd_fileout_push_in_buf(EthCamLink[idx].fileout_p[0], p_jpg, -1)) != HD_OK) {
			DBG_ERR("hd_fileout_push_in_buf fail(%d)\r\n", hd_ret);
			ret = E_SYS;
		}
	}
	return ret;
}

ER ImageApp_MovieMulti_EthCamLinkPushThumbToBsMux(MOVIE_CFG_REC_ID id, HD_BSMUX_PUT_DATA *p_thumb_info, UINT32 is_emr)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	if (p_thumb_info == NULL) {
		DBG_ERR("p_thumb_info = NULL\r\n");
		return ret;
	}

	if (is_emr > 1) {
		DBG_ERR("Invalid is_emr value(%d)\r\n");
		return ret;
	}

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (is_ethcamlink_opened[tb.idx] == FALSE) {
			DBG_ERR("EthCam[%d] is already closed!\r\n", tb.idx);
		} else {
			if (_LinkCheckStatus(EthCamLinkStatus[tb.idx].bsmux_p[0 + is_emr])) {
				if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[0 + is_emr], HD_BSMUX_PARAM_PUT_DATA, p_thumb_info)) != HD_OK) {
					DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_THUMB) fail(%d)\n", hd_ret);
				} else {
					ret = E_OK;
				}
			}
		}
	}
	return ret;
}

static UINT32 g_RecId1_Tx_RawEncodeAddr=0;
static UINT32 g_RecId1_Tx_RawEncodeSize=0;
static UINT32 g_bRecId1_Tx_TriggerRawEncode=0;

static UINT32 g_RecId1_Tx_ThumbAddr=0;
static UINT32 g_RecId1_Tx_ThumbSize=0;
static UINT32 g_bRecId1_Tx_TriggerThumb=0;

static UINT32 g_CloneId1_Tx_RawEncodeAddr=0;
static UINT32 g_CloneId1_Tx_RawEncodeSize=0;
#define HEAD_TYPE_THUMB     		0xff
#define HEAD_TYPE_RAW_ENCODE     	0xfe
ER ImageApp_MovieMulti_EthCamTxRecId1_SetRawEncodeBuff(UINT32 WorkAddr, UINT WorkSize)
{
	g_RecId1_Tx_RawEncodeAddr=WorkAddr;
	g_RecId1_Tx_RawEncodeSize=WorkSize;
	return E_OK;
}
ER ImageApp_MovieMulti_EthCamTxRecId1_TriggerRawEncode(UINT32 addr_va, UINT32 size)
{
	UINT8 *pbuf_rawencode;

	if(g_RecId1_Tx_RawEncodeAddr==0){
		DBG_ERR("RawEncode buffer is NULL!\r\n");
		return E_SYS;
	}
	if(g_RecId1_Tx_RawEncodeSize==0){
		DBG_ERR("RawEncode buffer size is 0!\r\n");
		return E_SYS;
	}
	pbuf_rawencode=(UINT8*)g_RecId1_Tx_RawEncodeAddr;
	if(g_RecId1_Tx_RawEncodeSize<size){
		DBG_ERR("RawEncode buffer size is too small!\r\n");
		return E_SYS;
	}
	pbuf_rawencode[0]=size & 0xff;
	pbuf_rawencode[1]=(size & 0xff00) >> 8;
	pbuf_rawencode[2]=(size & 0xff0000) >> 16;
	pbuf_rawencode[3]=0;//UserId;

	pbuf_rawencode[4]=0;
	pbuf_rawencode[5]=0;
	pbuf_rawencode[6]=0;
	pbuf_rawencode[7]=1;
	pbuf_rawencode[8]=HEAD_TYPE_RAW_ENCODE;//0xFE;
	memcpy((UINT8*)&pbuf_rawencode[9], (UINT8*)(addr_va), (UINT32)size);

	g_bRecId1_Tx_TriggerRawEncode=1;

	return E_OK;
}

ER ImageApp_MovieMulti_EthCamTxRecId1_SetThumbBuff(UINT32 WorkAddr, UINT WorkSize)
{
	g_RecId1_Tx_ThumbAddr=WorkAddr;
	g_RecId1_Tx_ThumbSize=WorkSize;
	return E_OK;
}
ER ImageApp_MovieMulti_EthCamTxRecId1_TriggerThumb(UINT32 addr_va, UINT32 size)
{
	UINT8 *pbuf_thumb;

	if(g_RecId1_Tx_ThumbAddr==0){
		DBG_ERR("Thumb buffer is NULL!\r\n");
		return E_SYS;
	}
	if(g_RecId1_Tx_ThumbSize==0){
		DBG_ERR("Thumb buffer size is 0!\r\n");
		return E_SYS;
	}
	pbuf_thumb=(UINT8*)g_RecId1_Tx_ThumbAddr;
	if(g_RecId1_Tx_ThumbSize<size){
		DBG_ERR("Thumb buffer size is overflow, want=%d, now=%d\r\n",g_RecId1_Tx_ThumbSize,size );
		return E_SYS;
	}
	pbuf_thumb[0]=size & 0xff;
	pbuf_thumb[1]=(size & 0xff00) >> 8;
	pbuf_thumb[2]=(size & 0xff0000) >> 16;
	pbuf_thumb[3]=0;//UserId;

	pbuf_thumb[4]=0;
	pbuf_thumb[5]=0;
	pbuf_thumb[6]=0;
	pbuf_thumb[7]=1;
	pbuf_thumb[8]=HEAD_TYPE_THUMB;//0xFF;
	memcpy((UINT8*)&pbuf_thumb[9], (UINT8*)(addr_va), (UINT32)size);

	g_bRecId1_Tx_TriggerThumb=1;

	return E_OK;
}
ER ImageApp_MovieMulti_EthCamTxCloneId1_SetRawEncodeBuff(UINT32 WorkAddr, UINT WorkSize)
{
	g_CloneId1_Tx_RawEncodeAddr=WorkAddr;
	g_CloneId1_Tx_RawEncodeSize=WorkSize;
	return E_OK;
}
static UINT32 g_EthCamTxRecId1_FrameIndex=0;
static UINT32 g_EthCamTxRecId1_Prev_FrameIndex=0;
static UINT32 g_EthCamTxCloneId1_FrameIndex=0;
static UINT32 g_EthCamTxCloneId1_Prev_FrameIndex=0;

static UINT32 g_RecId1_Tx_PrevHeaderBufIdx=0;
static UINT32 g_CloneId1_Tx_PrevHeaderBufIdx=0;

#define ETHCAM_FRM_HEADER_SIZE_PART1 (24)
#define ETHCAM_FRM_HEADER_SIZE (24+ 6 + 10)
#define ETHCAM_PREVHEADER_MAX_FRMNUM 60//30//15   //backup max frame number
static UINT8 g_RecId1_Tx_PrevHeader[ETHCAM_PREVHEADER_MAX_FRMNUM][ETHCAM_FRM_HEADER_SIZE]={0};
static UINT8 g_CloneId1_Tx_PrevHeader[ETHCAM_PREVHEADER_MAX_FRMNUM][ETHCAM_FRM_HEADER_SIZE]={0};

#define H264_TYPE_SLICE  1       ///< Coded slice (P-Frame)
#define H264_TYPE_IDR 5 ///< Instantaneous decoder refresh (I-Frame)

#define H265_TYPE_SLICE 1
#define H265_TYPE_IDR   19
#define H265_TYPE_VPS  32

#define H264_START_CODE_I		0x88
#define H264_START_CODE_P     	0x9A

#define ETHCAM_AUDCAP_QUEUE_BLOCK_SIZE (1*1024)

ER ImageApp_MovieMulti_EthCamTxRecId1_DumpBsInfo(void)
{
	UINT32 i, j;
	DBG_DUMP("RecId1, BufIdx=%d, FrmIdx=%d,%d\r\n", g_RecId1_Tx_PrevHeaderBufIdx, g_EthCamTxRecId1_FrameIndex,g_EthCamTxRecId1_Prev_FrameIndex);
	for(i=0;i<ETHCAM_PREVHEADER_MAX_FRMNUM;i++){
		DBG_DUMP("[%d] ", i);
		for(j=0;j<ETHCAM_FRM_HEADER_SIZE;j++){

			DBG_DUMP("%02x, ",  g_RecId1_Tx_PrevHeader[i][j]);
		}
		DBG_DUMP("\r\n");
	}

	return E_OK;
}
ER ImageApp_MovieMulti_EthCamTxCloneId1_DumpBsInfo(void)
{
	UINT32 i, j;
	DBG_DUMP("CloneId1, BufIdx=%d, FrmIdx=%d,%d\r\n", g_CloneId1_Tx_PrevHeaderBufIdx, g_EthCamTxCloneId1_FrameIndex,g_EthCamTxCloneId1_Prev_FrameIndex);
	for(i=0;i<ETHCAM_PREVHEADER_MAX_FRMNUM;i++){
		DBG_DUMP("[%d] ", i);
		for(j=0;j<ETHCAM_FRM_HEADER_SIZE;j++){

			DBG_DUMP("%02x, ",  g_CloneId1_Tx_PrevHeader[i][j]);
		}
		DBG_DUMP("\r\n");
	}

	return E_OK;
}
UINT32 g_EthCamTxRecId1_VdoEncVaAddr=0;
HD_VIDEOENC_BUFINFO g_EthCamTxRecId1_VdoEncBufinfo;
ER ImageApp_MovieMulti_EthCamTxRecId1_SetVdoEncVaAddr(UINT32 Pathid)
{
	hd_videoenc_get(Pathid, HD_VIDEOENC_PARAM_BUFINFO, &g_EthCamTxRecId1_VdoEncBufinfo);  //?? ?? bitstream  ? pa  addr
	//DBG_DUMP("SetVdoEncVaAddr, phy_addr=0x%x, buf_size=%d\r\n",g_EthCamTxCloneId1_VdoEncBufinfo.buf_info.phy_addr,g_EthCamTxCloneId1_VdoEncBufinfo.buf_info.buf_size);
	g_EthCamTxRecId1_VdoEncVaAddr = (UINT32) hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, g_EthCamTxRecId1_VdoEncBufinfo.buf_info.phy_addr, g_EthCamTxRecId1_VdoEncBufinfo.buf_info.buf_size) ;
	return E_OK;
}

ER ImageApp_MovieMulti_EthCamTxRecId1_CBLink(HD_VIDEOENC_BS *pvenc_data, UINT32 SndFunc)
{
	ER ret = E_OK;
	// add your code here
	ETHCAM_SND_FUNC pSndFunc =(ETHCAM_SND_FUNC)SndFunc;
	UINT32 *pImgBuf;
	//pImgBuf = &pvenc_data->video_pack[pvenc_data->pack_num-1].phy_addr;//&pData->Desc[0];
	pImgBuf = &pvenc_data->sign;//&pData->Desc[0];
	UINT32 StreamAddr=0;
	UINT32 StreamAddr_pa=0;
	UINT32 StreamSize;
	//UINT8 SVCLayerId;
	//UINT8 FrameType;
	BOOL bIframe=0;
	BOOL bPframe=0;
	UINT32 SPS_Addr = 0;
	UINT32 SPS_Addr_pa = 0;
	UINT32 SPS_Size = 0;
	UINT32 PPS_Addr = 0;
	UINT32 PPS_Addr_pa = 0;
	UINT32 PPS_Size = 0;// ,i;
	//UINT32 VPS_Addr = 0;
	UINT32 VPS_Size = 0;// ,i;
	//UINT8 *pBuf_SPS;
	//UINT8 *pBuf_PPS;
	//static UINT32 send_count_max = 100;
	//static UINT32 send_index = 1;
	//UINT32 bufSize=4;// 4 bytes
	//UINT8 pbuf_size[4] = {0};
	UINT8 *pbuf_stream = NULL;
	//static UINT32 frame_index=0;
	//static UINT32 frame_cnt=0;
	//UINT32 UserId; ///< user id for identifying user device
	UINT8 *pbuf_rawencode;
	UINT8 *pbuf_thumb;

	UINT32 checkSumHead=0;
	UINT32 checkSumTail=0;
	UINT32 checksum = 0;
	UINT8 checksumLen = 64;
	UINT32 i;
	#define LONGCNT_STAMP_EN 2
	#define LONGCNT_LENGTH 8
	#define REC1_PHY2VIRT_MAIN(pa) (g_EthCamTxRecId1_VdoEncVaAddr + (pa - g_EthCamTxRecId1_VdoEncBufinfo.buf_info.phy_addr))

	//if (*pImgBuf == MAKEFOURCC('T', 'H', 'U', 'M')) {
	if (g_bRecId1_Tx_TriggerThumb) {
		g_bRecId1_Tx_TriggerThumb=0;
		//ISF_VIDEO_STREAM_BUF *pVS = (ISF_VIDEO_STREAM_BUF *)pImgBuf;
		if(g_RecId1_Tx_ThumbAddr==0){
			DBG_ERR("Thumb buffer is NULL!\r\n");
			return E_SYS;
		}
		if(g_RecId1_Tx_ThumbSize==0){
			DBG_ERR("Thumb buffer size is 0!\r\n");
			return E_SYS;
		}
		pbuf_thumb=(UINT8*)g_RecId1_Tx_ThumbAddr;
		StreamSize=pbuf_thumb[0]+ (pbuf_thumb[1]<<8)+ (pbuf_thumb[2]<<16);

		DBG_DUMP("Thumb!!,StreamSize=%d\r\n",StreamSize);

		StreamSize+=9;
		//DBG_DUMP("0x%x, rawencode addr=0x%x, 0x%x\r\n",g_RecId1_Tx_RawEncodeAddr, pbuf_rawencode[0], pbuf_rawencode);

		pSndFunc((char *)pbuf_thumb, (int*)&StreamSize);
	}
	if (g_bRecId1_Tx_TriggerRawEncode) {
		g_bRecId1_Tx_TriggerRawEncode=0;
		//ISF_VIDEO_STREAM_BUF *pVS = (ISF_VIDEO_STREAM_BUF *)pImgBuf;
		if(g_RecId1_Tx_RawEncodeAddr==0){
			DBG_ERR("RawEncode buffer is NULL!\r\n");
			return E_SYS;
		}
		if(g_RecId1_Tx_RawEncodeSize==0){
			DBG_ERR("RawEncode buffer size is 0!\r\n");
			return E_SYS;
		}
		pbuf_rawencode=(UINT8*)g_RecId1_Tx_RawEncodeAddr;
		StreamSize=pbuf_rawencode[0]+ (pbuf_rawencode[1]<<8)+ (pbuf_rawencode[2]<<16);

		DBG_DUMP("RawEncode!!,StreamSize=%d\r\n",StreamSize);

		StreamSize+=9;
		//DBG_DUMP("0x%x, rawencode addr=0x%x, 0x%x\r\n",g_RecId1_Tx_RawEncodeAddr, pbuf_rawencode[0], pbuf_rawencode);

		pSndFunc((char *)pbuf_rawencode, (int*)&StreamSize);
	}
	if (*pImgBuf == MAKEFOURCC('V', 'S', 'T', 'M')) {
	//}else if (*pImgBuf == MAKEFOURCC('V', 'S', 'T', 'M')) {
		HD_VIDEOENC_BS *pVS = pvenc_data;
		//DBG_DUMP("vcodec_format=%d\r\n",  pVS->vcodec_format);

		//if (pVS->CodecType == MEDIAVIDENC_H264) {
		if (pVS->vcodec_format == HD_CODEC_TYPE_H264 || pVS->vcodec_format == HD_CODEC_TYPE_H265) {
			//if (send_index < 10)
			{
				//reference: _hd_videoenc_wrap_bitstream()
				if(pVS->vcodec_format == HD_CODEC_TYPE_H265){
					if( pVS->video_pack[0].pack_type.h265_type == H265_NALU_TYPE_VPS){
						VPS_Size = pVS->video_pack[0].size;//pps data size

						SPS_Addr_pa = pVS->video_pack[1].phy_addr; //sps addr
						SPS_Size = pVS->video_pack[1].size;//sps data size
						PPS_Addr_pa = pVS->video_pack[2].phy_addr; //pps addr
						PPS_Size = pVS->video_pack[2].size;//pps data size
					}
				}else{
					if( pVS->video_pack[0].pack_type.h264_type == H264_NALU_TYPE_SPS){
						SPS_Addr_pa = pVS->video_pack[0].phy_addr; //sps addr
						SPS_Size = pVS->video_pack[0].size;//sps data size
						PPS_Addr_pa = pVS->video_pack[1].phy_addr; //pps addr
						PPS_Size = pVS->video_pack[1].size;//pps data size
					}
				}

				//DBG_DUMP("main VPS_Size=%d, %d,%d\r\n",  VPS_Size,SPS_Size,PPS_Size);
				//DBG_DUMP("main pack_type=%d, %d,%d,%d\r\n",  pVS->video_pack[0].pack_type,pVS->video_pack[1].pack_type,pVS->video_pack[2].pack_type,pVS->video_pack[3].pack_type);

				#if 1
				StreamAddr_pa = pVS->video_pack[pVS->pack_num-1].phy_addr;

				StreamSize = pVS->video_pack[pVS->pack_num-1].size;// + (SPS_Size + PPS_Size + VPS_Size);
				StreamAddr = REC1_PHY2VIRT_MAIN(StreamAddr_pa);//(UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pVS->video_pack[pVS->pack_num-1].phy_addr, pVS->video_pack[pVS->pack_num-1].size);

				//StreamAddr = pVS->video_pack[pVS->pack_num-1].phy_addr -(SPS_Size + PPS_Size + VPS_Size);
				StreamAddr = StreamAddr -(SPS_Size + PPS_Size + VPS_Size);
				StreamSize+= (SPS_Size + PPS_Size + VPS_Size);


				#else
				StreamAddr = pVS->video_pack[pVS->pack_num-1].phy_addr -(SPS_Size + PPS_Size + VPS_Size);
				StreamSize = pVS->video_pack[pVS->pack_num-1].size + (SPS_Size + PPS_Size + VPS_Size);
				#endif
				//SVCLayerId = pVS->Resv[9];
				//FrameType = pVS->Resv[6];
				//pBuf_SPS=(UINT8 *)SPS_Addr;
				//pBuf_PPS=(UINT8 *)PPS_Addr;


				pbuf_stream =(UINT8 *)StreamAddr;
				//DBG_DUMP("pbuf_stream[0]=0x%x, %x,%x,%x,%x,%x\r\n",  pbuf_stream[0],pbuf_stream[1],pbuf_stream[2],pbuf_stream[3],pbuf_stream[4],pbuf_stream[5]);
				//UINT32 k;
				//DBG_DUMP("main pbuf_stream[0]=\r\n");
				//for(k=0;k<(SPS_Size + PPS_Size + VPS_Size+6);k++){
				//	DBG_DUMP("[%d]=%x, ",  k,pbuf_stream[k]);
				//}
				//DBG_DUMP("\r\n");
				#if 0
				UINT32 k;
				//DBG_DUMP("clone pbuf_stream[0]=\r\n");
				for(k=0;k<(SPS_Size + PPS_Size + VPS_Size+15);k++){
					DBG_DUMP("0x%x, ", pbuf_stream[k]);
				}
				DBG_DUMP("\r\n");
				#endif
				#if 0
				if(pVS->vcodec_format == HD_CODEC_TYPE_H264){
					if((pbuf_stream[SPS_Size + PPS_Size + 0]==0 && pbuf_stream[SPS_Size + PPS_Size + 1]==0 && pbuf_stream[SPS_Size + PPS_Size + 2]==0 && pbuf_stream[SPS_Size + PPS_Size + 3]==1 ) &&
						(*(pbuf_stream + SPS_Size + PPS_Size + 4) & 0x1F) ==H264_TYPE_IDR  && (*(pbuf_stream + SPS_Size + PPS_Size + 5)) == H264_START_CODE_I){ //I frame
						bIframe= 1;//StreamSender_Util_IsFrameI(StreamAddr);
					}else if(((*(pbuf_stream + 0) & 0x1F) ==0 && (*(pbuf_stream + 1) & 0x1F) ==0 && (*(pbuf_stream + 2) & 0x1F) ==0 && (*(pbuf_stream + 3) & 0x1F) ==1 ) &&
						(*(pbuf_stream + 4) & 0x1F) ==H264_TYPE_SLICE && (*(pbuf_stream + 5)) == H264_START_CODE_P){ //P frame
						bPframe= 1;//StreamSender_Util_IsFrameI(StreamAddr);
					}else{
						bIframe= 0;
					}
				}else{//MEDIAVIDENC_H265
					if( (pbuf_stream[0]==0 && pbuf_stream[1]==0 && pbuf_stream[2]==0 && pbuf_stream[3]==1 ) &&
						((*(pbuf_stream + 4) >>1 )&0x3F)==H265_TYPE_VPS  &&
						(pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 0]==0 && pbuf_stream[SPS_Size + PPS_Size +VPS_Size+  1]==0 && pbuf_stream[SPS_Size + PPS_Size +VPS_Size+  2]==0 && pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 3]==1 ) &&
						((*(pbuf_stream + SPS_Size + PPS_Size + VPS_Size+ 4) >>1 )&0x3F)==H265_TYPE_IDR){ //I frame
						bIframe= 1;//StreamSender_Util_IsFrameI(StreamAddr);
					}else if(((*(pbuf_stream + 0) & 0x1F) ==0 && (*(pbuf_stream + 1) & 0x1F) ==0 && (*(pbuf_stream + 2) & 0x1F) ==0 && (*(pbuf_stream + 3) & 0x1F) ==1 ) &&
						((*(pbuf_stream + 4) >>1)&0x3F) ==H265_TYPE_SLICE){ //P frmae
						bPframe= 1;//StreamSender_Util_IsFrameI(StreamAddr);
					}else{
						bIframe= 0;
					}
				}
				#else
				if(pVS->vcodec_format == HD_CODEC_TYPE_H264){
					if(pVS->video_pack[pVS->pack_num-1].pack_type.h264_type==H264_NALU_TYPE_IDR ){ //I frame
						bIframe= 1;
					}else if( pVS->video_pack[pVS->pack_num-1].pack_type.h264_type==H264_NALU_TYPE_SLICE){ //P frame
						bPframe= 1;
					}else{
						bIframe= 0;
					}
				}else{//MEDIAVIDENC_H265
					if(pVS->video_pack[pVS->pack_num-1].pack_type.h265_type==H265_NALU_TYPE_IDR ){ //I frame
						bIframe= 1;
					}else if( pVS->video_pack[pVS->pack_num-1].pack_type.h265_type==H265_NALU_TYPE_SLICE){ //P frame
						bPframe= 1;
					}else{
						bIframe= 0;
					}
				}
				#endif

				//DBG_DUMP("main bIframe=%d, %d\r\n", bIframe,bPframe);

				UINT8 pbuf_startcode[10];
				pbuf_startcode[0]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 0];
				pbuf_startcode[1]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 1];
				pbuf_startcode[2]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 2];
				pbuf_startcode[3]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 3];
				pbuf_startcode[4]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 4];

				pbuf_startcode[5]=pbuf_stream[0];
				pbuf_startcode[6]=pbuf_stream[1];
				pbuf_startcode[7]=pbuf_stream[2];
				pbuf_startcode[8]=pbuf_stream[3];
				pbuf_startcode[9]=pbuf_stream[4];

				g_EthCamTxRecId1_Prev_FrameIndex=g_EthCamTxRecId1_FrameIndex;


				if(bIframe){
					//DBG_DUMP("I\r\n");
					// (4byte sps length header + sps data) + (4byte pps length header + pps data) + (4byte I frame length header + I data) + (4byte P frame length header + P data)
					// (2byte sps length header + sps data) + (2byte pps length header + pps data) + (8byte I frame length header + I data) + (8byte P frame length header + P data)
					//frame_cnt++;
					//frame_index=1;
					g_EthCamTxRecId1_FrameIndex=1;
					//make checksum
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						StreamAddr=StreamAddr+(SPS_Size + PPS_Size);//I Start
						StreamSize=StreamSize-(SPS_Size + PPS_Size); //I Size
					}
					//UINT32          uiTime=Perf_GetCurrent();
					if(StreamSize>checksumLen){
						checksum=0;
						pbuf_stream =(UINT8 *)StreamAddr;
						for(i=0;i<checksumLen;i++){
							checksum+= *(pbuf_stream+i);
						}
						checkSumHead=checksum & 0xff;//EthCamSocket_Checksum((UINT8 *)StreamAddr, 64) & 0xff;
						checksum=0;
						pbuf_stream =(UINT8 *)(StreamAddr+ StreamSize-checksumLen);
						for(i=0;i<checksumLen;i++){
							checksum+= *(pbuf_stream+i);
						}
						checkSumTail=checksum & 0xff;//EthCamSocket_Checksum((UINT8 *)(StreamAddr+ StreamSize-1-64), 64) & 0xff;
					}
					//DBG_DUMP("chksum=%d, 0x%x, 0x%x, %d\r\n", StreamSize,checkSumHead,checkSumTail,Perf_GetCurrent() - uiTime);
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						StreamAddr=StreamAddr-(SPS_Size + PPS_Size);
						StreamSize=StreamSize+ (SPS_Size + PPS_Size);
					}

					//StreamAddr-=(12);//shift left 12 bytes for 2 bytes sps size,  2 bytes pps size,  8 bytes I frame custom header
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						StreamAddr-=(12+LONGCNT_LENGTH);//shift left 20 bytes for 2 bytes sps size,  2 bytes pps size,  8 bytes longconter, 8 bytes I frame custom header
					}else{
						StreamAddr-=(8+LONGCNT_LENGTH);//shift left 16 bytes for  8 bytes longconter, 8 bytes I frame custom header
					}
					pbuf_stream =(UINT8 *)StreamAddr;
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						////hwmem_open();
						////hwmem_memcpy((UINT32)&pbuf_stream[2], (UINT32)(SPS_Addr), (UINT32)SPS_Size);
						////hwmem_memcpy((UINT32)&pbuf_stream[2+SPS_Size+2], (UINT32)(PPS_Addr), (UINT32)PPS_Size);
						////hwmem_close();
						SPS_Addr = REC1_PHY2VIRT_MAIN(SPS_Addr_pa);//(UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, SPS_Addr_pa, SPS_Size);
						PPS_Addr =  REC1_PHY2VIRT_MAIN(PPS_Addr_pa);//(UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, PPS_Addr_pa, PPS_Size);
						#if 0
						static UINT16 bFirstIn=1;
						if(bFirstIn){
							bFirstIn=0;
							DBG_DUMP("SPS SPS_Size=%d, \r\n",SPS_Size);
							UINT16 zz;
							UINT8 *SPSTmp=(UINT8 *) SPS_Addr;
							for(zz=0;zz<SPS_Size;zz++){
								DBG_DUMP("0x%x, ", SPSTmp[zz]);
							}
							fflush(stdout);
							DBG_DUMP("PPS PPS_Size=%d, \r\n",PPS_Size);
							UINT8 *PPSTmp=(UINT8 *) PPS_Addr;
							for(zz=0;zz<PPS_Size;zz++){
								DBG_DUMP("0x%x, ", PPSTmp[zz]);
							}
							fflush(stdout);
						}
						#endif

						memcpy((UINT8*)&pbuf_stream[2], (UINT8*)(SPS_Addr), (UINT32)SPS_Size);
						memcpy((UINT8*)&pbuf_stream[2+SPS_Size+2], (UINT8*)(PPS_Addr), (UINT32)PPS_Size);
						pbuf_stream[0]=SPS_Size & 0xff;
						pbuf_stream[1]=(SPS_Size & 0xff00) >> 8;
						pbuf_stream[2+SPS_Size]=PPS_Size & 0xff;
						pbuf_stream[2+SPS_Size+1]=(PPS_Size & 0xff00) >> 8;

						#if 1//add longcounter
						UINT64 LongCounter=pvenc_data->timestamp;//HwClock_GetLongCounter();
						//Hi byte
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+0]=((LongCounter >> 32) & 0xff000000)>>24; 	//-16
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+1]=((LongCounter >> 32) & 0x00ff0000)>>16; 	//-15
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+2]=((LongCounter >> 32) & 0x0000ff00)>>8;  	//-14
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+3]=(LongCounter >> 32) & 0x000000ff;       	//-13
						//Low byte
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+4]=(LongCounter & 0xff000000)>>24; //-12
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+5]=(LongCounter & 0x00ff0000)>>16; //-11
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+6]=(LongCounter & 0x0000ff00)>>8; 	//-10
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+7]=LongCounter & 0x000000ff; 		//-9
						#endif

						StreamSize=StreamSize- (SPS_Size + PPS_Size);//I Frame Size
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size]=StreamSize & 0xff;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+1]=(StreamSize & 0xff00) >> 8;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+2]=(StreamSize & 0xff0000) >> 16;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+3]=g_EthCamTxRecId1_FrameIndex;//frame_index;

						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+4]=checkSumHead;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+5]=checkSumTail;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+6]=0;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+7]=LONGCNT_STAMP_EN;
						StreamSize=StreamSize+ (SPS_Size + PPS_Size)+ 12+LONGCNT_LENGTH;
					}else{
						#if 1//add longcounter
						UINT64 LongCounter=pvenc_data->timestamp;//HwClock_GetLongCounter();
						//Hi byte
						pbuf_stream[0]=((LongCounter >> 32) & 0xff000000)>>24; 	//-16
						pbuf_stream[1]=((LongCounter >> 32) & 0x00ff0000)>>16; 	//-15
						pbuf_stream[2]=((LongCounter >> 32) & 0x0000ff00)>>8;  	//-14
						pbuf_stream[3]=(LongCounter >> 32) & 0x000000ff;       	//-13
						//Low byte
						pbuf_stream[4]=(LongCounter & 0xff000000)>>24; //-12
						pbuf_stream[5]=(LongCounter & 0x00ff0000)>>16; //-11
						pbuf_stream[6]=(LongCounter & 0x0000ff00)>>8; 	//-10
						pbuf_stream[7]=LongCounter & 0x000000ff; 		//-9
						#endif

						//StreamSize=StreamSize- (SPS_Size + PPS_Size+ VPS_Size);//I Frame Size
						pbuf_stream[LONGCNT_LENGTH]=StreamSize & 0xff;
						pbuf_stream[LONGCNT_LENGTH+1]=(StreamSize & 0xff00) >> 8;
						pbuf_stream[LONGCNT_LENGTH+2]=(StreamSize & 0xff0000) >> 16;
						pbuf_stream[LONGCNT_LENGTH+3]=g_EthCamTxRecId1_FrameIndex;//frame_index;

						pbuf_stream[LONGCNT_LENGTH+4]=checkSumHead;
						pbuf_stream[LONGCNT_LENGTH+5]=checkSumTail;
						pbuf_stream[LONGCNT_LENGTH+6]=(SPS_Size + PPS_Size + VPS_Size);
						pbuf_stream[LONGCNT_LENGTH+7]=LONGCNT_STAMP_EN;
						StreamSize=StreamSize+  8 +LONGCNT_LENGTH;
					}

					#if 1 //debug info
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						pbuf_stream =(UINT8 *)(StreamAddr + SPS_Size + 2+ PPS_Size +2);
					}else{
						pbuf_stream =(UINT8 *)(StreamAddr);
					}
					//memset(&g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][0], 0, ETHCAM_FRM_HEADER_SIZE);
					memset(&g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][0], 0, ETHCAM_FRM_HEADER_SIZE);
					for(i=0;i<ETHCAM_FRM_HEADER_SIZE_PART1;i++){
						//g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][i]=pbuf_stream[i];
						g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][i]=pbuf_stream[i];
					}
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][0]=pVS->Resv[6];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][1]=pVS->Resv[7];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][2]=pVS->Resv[8];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][3]=pVS->Resv[9];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][4]=pVS->Resv[10];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][5]=pVS->vcodec_format;


					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1]=bIframe;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+1]=bPframe;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+2]=SPS_Size ;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+3]=PPS_Size;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+4]=VPS_Size;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+5]=g_EthCamTxRecId1_FrameIndex;

					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+6]=pbuf_startcode[0];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+7]=pbuf_startcode[1];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+8]=pbuf_startcode[2];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+9]=pbuf_startcode[3];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+10]=pbuf_startcode[4];

					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+11]=pbuf_startcode[5];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+12]=pbuf_startcode[6];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+13]=pbuf_startcode[7];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+14]=pbuf_startcode[8];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+15]=pbuf_startcode[9];

					g_RecId1_Tx_PrevHeaderBufIdx++;
					g_RecId1_Tx_PrevHeaderBufIdx= (g_RecId1_Tx_PrevHeaderBufIdx >=ETHCAM_PREVHEADER_MAX_FRMNUM) ? 0 : g_RecId1_Tx_PrevHeaderBufIdx;
					#endif

					pSndFunc((char *)StreamAddr, (int*)&StreamSize);

				}else if(bPframe){ // p frame
					//DBG_DUMP("P\r\n");
					//frame_index++;
					g_EthCamTxRecId1_FrameIndex++;
					//frame_cnt++;

					if(StreamSize>checksumLen){
						checksum=0;
						pbuf_stream =(UINT8 *)StreamAddr;
						for(i=0;i<checksumLen;i++){
							checksum+= *(pbuf_stream+i);
						}
						checkSumHead=checksum & 0xff;//EthCamSocket_Checksum((UINT8 *)StreamAddr, 64) & 0xff;
						checksum=0;
						pbuf_stream =(UINT8 *)(StreamAddr+ StreamSize-checksumLen);
						for(i=0;i<checksumLen;i++){
							checksum+= *(pbuf_stream+i);
						}
						checkSumTail=checksum & 0xff;//EthCamSocket_Checksum((UINT8 *)(StreamAddr+ StreamSize-1-64), 64) & 0xff;
					}

					StreamAddr-=8; //shift left 8 bytes for P frame custom header
					pbuf_stream =(UINT8 *)StreamAddr;

					pbuf_stream[0]=StreamSize & 0xff;                     //-8 from star code 00 00 00 01
					pbuf_stream[1]=(StreamSize & 0xff00) >> 8;        //-7
					pbuf_stream[2]=(StreamSize & 0xff0000) >> 16;   //-6
					pbuf_stream[3]=g_EthCamTxRecId1_FrameIndex;//frame_index;                              //-5

					pbuf_stream[4]=checkSumHead;                          //-4
					pbuf_stream[5]=checkSumTail;                            //-3
					pbuf_stream[6]=0;                                            //-2
					pbuf_stream[7]=LONGCNT_STAMP_EN;                 //-1
					StreamSize+=8;

					#if 1 //add longcounter
					//add timestamp
					StreamSize=StreamSize+ LONGCNT_LENGTH;
					StreamAddr-=LONGCNT_LENGTH; //shift left 8 bytes for longconter
					pbuf_stream =(UINT8 *)StreamAddr;
					UINT64 LongCounter=pvenc_data->timestamp;//HwClock_GetLongCounter();
					//Hi byte
					pbuf_stream[0]=((LongCounter >> 32) & 0xff000000)>>24; 	//-16
					pbuf_stream[1]=((LongCounter >> 32) & 0x00ff0000)>>16; 	//-15
					pbuf_stream[2]=((LongCounter >> 32) & 0x0000ff00)>>8;  	//-14
					pbuf_stream[3]=(LongCounter >> 32) & 0x000000ff;       	//-13
					//Low byte
					pbuf_stream[4]=(LongCounter & 0xff000000)>>24; //-12
					pbuf_stream[5]=(LongCounter & 0x00ff0000)>>16; //-11
					pbuf_stream[6]=(LongCounter & 0x0000ff00)>>8; 	//-10
					pbuf_stream[7]=LongCounter & 0x000000ff; 		//-9
					//DBG_DUMP("Ppbuf[0]=0x%x, %x, %x, %x, %x, %x, %x, %x\r\n",  pbuf_stream[0] ,  pbuf_stream[1] ,  pbuf_stream[2] ,  pbuf_stream[3] ,  pbuf_stream[4] ,  pbuf_stream[5] ,  pbuf_stream[6] ,  pbuf_stream[7]);
					//DBG_DUMP("Ppbuf[8]=0x%x, %x, %x, %x, %x, %x, %x, %x\r\n",  pbuf_stream[8] ,  pbuf_stream[9] ,  pbuf_stream[10] ,  pbuf_stream[11] ,  pbuf_stream[12] ,  pbuf_stream[13] ,  pbuf_stream[14] ,  pbuf_stream[15]);
					#endif

					#if 1//add debug info
					pbuf_stream =(UINT8 *)StreamAddr;
					//memset(&g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][0], 0, ETHCAM_FRM_HEADER_SIZE);
					memset(&g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][0], 0, ETHCAM_FRM_HEADER_SIZE);
					for(i=0;i<ETHCAM_FRM_HEADER_SIZE_PART1;i++){
						//g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][i]=pbuf_stream[i];
						g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][i]=pbuf_stream[i];
					}

					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][0]=pVS->Resv[6];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][1]=pVS->Resv[7];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][2]=pVS->Resv[8];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][3]=pVS->Resv[9];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][4]=pVS->Resv[10];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][5]=pVS->vcodec_format;


					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1]=bIframe;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+1]=bPframe;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+2]=SPS_Size ;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+3]=PPS_Size;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+4]=VPS_Size;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+5]=g_EthCamTxRecId1_FrameIndex;

					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+6]=pbuf_startcode[0];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+7]=pbuf_startcode[1];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+8]=pbuf_startcode[2];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+9]=pbuf_startcode[3];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+10]=pbuf_startcode[4];

					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+11]=pbuf_startcode[5];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+12]=pbuf_startcode[6];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+13]=pbuf_startcode[7];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+14]=pbuf_startcode[8];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+15]=pbuf_startcode[9];


					g_RecId1_Tx_PrevHeaderBufIdx++;
					g_RecId1_Tx_PrevHeaderBufIdx= (g_RecId1_Tx_PrevHeaderBufIdx >=ETHCAM_PREVHEADER_MAX_FRMNUM) ? 0 : g_RecId1_Tx_PrevHeaderBufIdx;
					#endif
					#if 0
					if(AudCapFuncEn){
						if((AudCapQ =_ImageApp_MovieMulti_EthCamTxRecId1AudCap_HowManyInQ())){
							UINT32 DataSize=0;
							UINT32 DataAddr=0;
							DataAddr=(StreamAddr-ETHCAM_AUDCAP_FRAME_SIZE);
							memset((char *)(DataAddr), 0, (ETHCAM_AUDCAP_FRAME_SIZE));
							pbuf_stream =(UINT8 *)(DataAddr);
							for(i=0;((i<AudCapQ) && (i<3));i++){
								if(i==0){
									pbuf_stream[0]=1;   //CopyCount
									pbuf_stream[1]=0;   //total size
									pbuf_stream[2]=0;   //total size

									pbuf_stream[3]=0; //start code
									pbuf_stream[4]=0;
									pbuf_stream[5]=0;
									pbuf_stream[6]=1;
									pbuf_stream[7]=HEAD_TYPE_AUDCAP;
									DataSize=_ImageApp_MovieMulti_EthCamTxRecId1AudCap_GetQ((DataAddr+8));
									DataSize+=8;
								}else{
									DataAddr+=DataSize;
									//DataSize+=_ImageApp_MovieMulti_EthCamTxRecId1AudCap_GetQ(DataAddr);
									DataSize=_ImageApp_MovieMulti_EthCamTxRecId1AudCap_GetQ(DataAddr);
									pbuf_stream[0]++;
								}
							}
							if(DataSize >=ETHCAM_AUDCAP_FRAME_SIZE){
								DBG_ERR("P DataSize over flow=%d\r\n",DataSize);
							}
							pbuf_stream[1]=DataSize & 0xff;
							pbuf_stream[2]=(DataSize & 0xff00) >> 8;
							StreamAddr-=ETHCAM_AUDCAP_FRAME_SIZE;
							StreamSize+=ETHCAM_AUDCAP_FRAME_SIZE;
							//DBG_DUMP("P[%d] sz=%d, ACapQ=%d, sz=%d\r\n",frame_index,P_Size,AudCapQ,DataSize);
						}else{

							//StreamAddr-=ETHCAM_AUDCAP_FRAME_SIZE;
							//StreamSize+=ETHCAM_AUDCAP_FRAME_SIZE;
							//memset((char *)(StreamAddr), 0, (ETHCAM_AUDCAP_FRAME_SIZE));
							//DBG_DUMP("P StreamAddr=0x%x, 0x%x, sz=%d\r\n",StreamAddr,StreamAddr-ETHCAM_AUDCAP_FRAME_SIZE, StreamSize);
							//DBG_DUMP("P checkSumHead=0x%x, 0x%x\r\n",checkSumHead,checkSumHead1);
						}
					}
					#endif
					pSndFunc((char *)StreamAddr, (int*)&StreamSize);

				}else{
					DBG_ERR("frame type error\r\n");
					#if 1//add debug info
					pbuf_stream =(UINT8 *)StreamAddr;
					//memset(&g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][0], 0, ETHCAM_FRM_HEADER_SIZE);
					memset(&g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][0], 0, ETHCAM_FRM_HEADER_SIZE);
					for(i=0;i<ETHCAM_FRM_HEADER_SIZE_PART1;i++){
						//g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][i]=pbuf_stream[i];
						g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][i]=pbuf_stream[i];
					}

					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][0]=pVS->Resv[6];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][1]=pVS->Resv[7];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][2]=pVS->Resv[8];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][3]=pVS->Resv[9];
					//g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][4]=pVS->Resv[10];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][5]=pVS->vcodec_format;

					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1]=bIframe;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+1]=bPframe;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+2]=SPS_Size ;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+3]=PPS_Size;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+4]=VPS_Size;
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+5]=g_EthCamTxRecId1_FrameIndex;

					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+6]=pbuf_startcode[0];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+7]=pbuf_startcode[1];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+8]=pbuf_startcode[2];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+9]=pbuf_startcode[3];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+10]=pbuf_startcode[4];

					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+11]=pbuf_startcode[5];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+12]=pbuf_startcode[6];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+13]=pbuf_startcode[7];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+14]=pbuf_startcode[8];
					g_RecId1_Tx_PrevHeader[g_RecId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+15]=pbuf_startcode[9];


					g_RecId1_Tx_PrevHeaderBufIdx++;
					g_RecId1_Tx_PrevHeaderBufIdx= (g_RecId1_Tx_PrevHeaderBufIdx >=ETHCAM_PREVHEADER_MAX_FRMNUM) ? 0 : g_RecId1_Tx_PrevHeaderBufIdx;
					#endif
					return E_SYS;
				}
#if 0//write to a file
			static UINT32 file_count_max = 300;
			static UINT32 file_index = 0;
			static char file_name[50] = {0};
			static FST_FILE  FileHdl;
			char enc_typ = 0x65;
			ISF_VIDEO_STREAM_BUF *pVS_tmp = (ISF_VIDEO_STREAM_BUF *)pImgBuf;
			UINT32 tmp_size = pVS_tmp->Resv[1] + pVS_tmp->Resv[3];
			UINT32 header_addr = pVS_tmp->Resv[0];
			static UINT32 frame_index2=0;
			StreamAddr = pVS_tmp->DataAddr;
			StreamSize = pVS_tmp->DataSize;
			SPS_Addr = pVS_tmp->Resv[0]; //sps, pps addr
			SPS_Size = pVS_tmp->Resv[1];//sps, pps data size
			PPS_Addr = pVS_tmp->Resv[2]; //sps, pps addr
			PPS_Size = pVS_tmp->Resv[3];//sps, pps data size

			if (pVS_tmp->CodecType == MEDIAVIDENC_H264) {
				sprintf(file_name, "A:\\MovieH264_%dX%dp%d.264", pVS_tmp->Width, pVS_tmp->Height, pVS_tmp->FramePerSecond);
				enc_typ = 0x65;
				tmp_size = pVS_tmp->Resv[1] + pVS_tmp->Resv[3];
				header_addr = pVS_tmp->Resv[0];
			}
			if (file_index == 0) {
				DBG_DUMP("start write stream to file %s\r\n", file_name);
				FileHdl = FileSys_OpenFile(file_name, FST_OPEN_WRITE | FST_CREATE_ALWAYS);
			}
			if (file_index < file_count_max) {
				if (*((char *)StreamAddr + 3) == 0x01 && *((char *)StreamAddr + 4) == enc_typ) {
					////this is a I frame, need add SPS PPS data first
						//pbuf_size = (UINT8 *)&SPS_Size;
						pbuf_size[0]=SPS_Size & 0xff;
						pbuf_size[1]=(SPS_Size & 0xff00) >> 8;
						pbuf_size[2]=(SPS_Size & 0xff0000) >> 16;
						pbuf_size[3]=0;
					FileSys_WriteFile(FileHdl, (UINT8 *)pbuf_size, &bufSize, 0, NULL);
					FileSys_WriteFile(FileHdl, (UINT8 *)SPS_Addr, &SPS_Size, 0, NULL);

						//pbuf_size = (UINT8 *)&PPS_Size;
						pbuf_size[0]=PPS_Size & 0xff;
						pbuf_size[1]=(PPS_Size & 0xff00) >> 8;
						pbuf_size[2]=(PPS_Size & 0xff0000) >> 16;
						pbuf_size[3]=0;
					FileSys_WriteFile(FileHdl, (UINT8 *)pbuf_size, &bufSize, 0, NULL);
					FileSys_WriteFile(FileHdl, (UINT8 *)PPS_Addr, &PPS_Size, 0, NULL);

					//FileSys_WriteFile(FileHdl, (UINT8 *)header_addr, &tmp_size, 0, NULL);
						frame_index2=1;
					}else{
						frame_index2++;
				}
					//pbuf_size = (UINT8 *)&StreamSize;
					pbuf_size[0]=StreamSize & 0xff;
					pbuf_size[1]=(StreamSize & 0xff00) >> 8;
					pbuf_size[2]=(StreamSize & 0xff0000) >> 16;
					pbuf_size[3]=frame_index2;
				FileSys_WriteFile(FileHdl, (UINT8 *)pbuf_size, &bufSize, 0, NULL);
				FileSys_WriteFile(FileHdl, (UINT8 *)StreamAddr, &StreamSize, 0, NULL);
				file_index++;
			}
			if (file_index == file_count_max) {
				DBG_DUMP("finish write stream to file\r\n");
				FileSys_CloseFile(FileHdl);
				file_index++;
			}
#endif
			}
		}else{
			DBG_ERR("vcodec_format error\r\n");
			//return ISF_ERR_NOT_SUPPORT;
			return E_SYS;
		}
	}


	return ret;
}
UINT32 g_EthCamTxCloneId1_VdoEncVaAddr=0;
HD_VIDEOENC_BUFINFO g_EthCamTxCloneId1_VdoEncBufinfo;
ER ImageApp_MovieMulti_EthCamTxCloneId1_SetVdoEncVaAddr(UINT32 Pathid)
{
	hd_videoenc_get(Pathid, HD_VIDEOENC_PARAM_BUFINFO, &g_EthCamTxCloneId1_VdoEncBufinfo);  //?? ?? bitstream  ? pa  addr
	//DBG_DUMP("SetVdoEncVaAddr, phy_addr=0x%x, buf_size=%d\r\n",g_EthCamTxCloneId1_VdoEncBufinfo.buf_info.phy_addr,g_EthCamTxCloneId1_VdoEncBufinfo.buf_info.buf_size);
	g_EthCamTxCloneId1_VdoEncVaAddr = (UINT32) hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, g_EthCamTxCloneId1_VdoEncBufinfo.buf_info.phy_addr, g_EthCamTxCloneId1_VdoEncBufinfo.buf_info.buf_size) ;
	return E_OK;
}
ER ImageApp_MovieMulti_EthCamTxCloneId1_CBLink(HD_VIDEOENC_BS *pvenc_data, UINT32 SndFunc)
{
	ER ret = E_OK;
	// add your code here
	ETHCAM_SND_FUNC pSndFunc =(ETHCAM_SND_FUNC)SndFunc;
	UINT32 *pImgBuf = NULL;
	//pImgBuf = &pvenc_data->video_pack[pvenc_data->pack_num-1].phy_addr;//&pData->Desc[0];
	//pImgBuf = &pvenc_data->sign;//&pData->Desc[0];
	UINT32 StreamAddr=0;
	UINT32 StreamAddr_pa=0;
	UINT32 StreamSize;
	//UINT8 SVCLayerId;
	//UINT8 FrameType;
	UINT32 bIframe=0;
	UINT32 bPframe=0;
	UINT32 SPS_Addr = 0;
	UINT32 SPS_Addr_pa = 0;
	UINT32 SPS_Size = 0;
	UINT32 PPS_Addr = 0;
	UINT32 PPS_Addr_pa = 0;
	UINT32 PPS_Size = 0;// ,i;
	//UINT32 VPS_Addr = 0;
	UINT32 VPS_Size = 0;// ,i;
	//UINT8 *pBuf_SPS;
	//UINT8 *pBuf_PPS;
	//static UINT32 send_count_max = 100;
	//static UINT32 send_index = 1;
	//UINT32 bufSize=4;// 4 bytes
	//UINT8 pbuf_size[4] = {0};
	UINT8 *pbuf_stream = NULL;
	//static UINT32 frame_index=0;
	//static UINT32 frame_cnt=0;
	//UINT32 UserId; ///< user id for identifying user device
	//UINT8 *pbuf_rawencode;

	UINT32 checkSumHead=0;
	UINT32 checkSumTail=0;
	UINT32 checksum = 0;
	UINT8 checksumLen = 64;
	UINT32 i;
	#define LONGCNT_STAMP_EN 2
	#define LONGCNT_LENGTH 8
	#define PHY2VIRT_MAIN(pa) (g_EthCamTxCloneId1_VdoEncVaAddr + (pa - g_EthCamTxCloneId1_VdoEncBufinfo.buf_info.phy_addr))

	//const UINT32 TS_RESV_LENGTH= (ALIGN_CEIL_4(NMEDIAREC_TS_PES_HEADER_LENGTH + NMEDIAREC_TS_NAL_LENGTH))=24
	if(pvenc_data){
		pImgBuf = &pvenc_data->sign;//&pData->Desc[0];
	}else{
		DBG_ERR("pvenc_data NULL\r\n");
	}
	if (pImgBuf && *pImgBuf == MAKEFOURCC('V', 'S', 'T', 'M')) {
		HD_VIDEOENC_BS *pVS = pvenc_data;
		//if (pVS->CodecType == MEDIAVIDENC_H264) {
		if (pVS && (pVS->vcodec_format == HD_CODEC_TYPE_H264 || pVS->vcodec_format == HD_CODEC_TYPE_H265)) {
			//if (send_index < 10)
			{

				//reference: _hd_videoenc_wrap_bitstream()
				if(pVS->vcodec_format == HD_CODEC_TYPE_H265){
					if( pVS->video_pack[0].pack_type.h265_type == H265_NALU_TYPE_VPS){
						VPS_Size = pVS->video_pack[0].size;//pps data size

						SPS_Addr_pa = pVS->video_pack[1].phy_addr; //sps addr
						SPS_Size = pVS->video_pack[1].size;//sps data size
						PPS_Addr_pa = pVS->video_pack[2].phy_addr; //pps addr
						PPS_Size = pVS->video_pack[2].size;//pps data size
					}
				}else{
					if( pVS->video_pack[0].pack_type.h264_type == H264_NALU_TYPE_SPS){
						SPS_Addr_pa = pVS->video_pack[0].phy_addr; //sps addr
						SPS_Size = pVS->video_pack[0].size;//sps data size
						PPS_Addr_pa = pVS->video_pack[1].phy_addr; //pps addr
						PPS_Size = pVS->video_pack[1].size;//pps data size
					}
				}

				//DBG_DUMP("clone VPS_Size=%d, %d,%d ,pack_num=%d,  [0].addr=0x%x, [0].size=%d, [3].addr=0x%x, [3].size=%d\r\n",  VPS_Size,SPS_Size,PPS_Size, pVS->pack_num, pVS->video_pack[0].phy_addr,pVS->video_pack[0].size, pVS->video_pack[3].phy_addr,pVS->video_pack[3].size);
				//DBG_DUMP("clone pack_type=%d, %d,%d,%d\r\n",  pVS->video_pack[0].pack_type,pVS->video_pack[1].pack_type,pVS->video_pack[2].pack_type,pVS->video_pack[3].pack_type);
				StreamAddr_pa = pVS->video_pack[pVS->pack_num-1].phy_addr;

				StreamSize = pVS->video_pack[pVS->pack_num-1].size;// + (SPS_Size + PPS_Size + VPS_Size);
				StreamAddr = PHY2VIRT_MAIN(StreamAddr_pa);//(UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pVS->video_pack[pVS->pack_num-1].phy_addr, pVS->video_pack[pVS->pack_num-1].size);

				//StreamAddr = pVS->video_pack[pVS->pack_num-1].phy_addr -(SPS_Size + PPS_Size + VPS_Size);
				StreamAddr = StreamAddr -(SPS_Size + PPS_Size + VPS_Size);
				StreamSize+= (SPS_Size + PPS_Size + VPS_Size);
				//SVCLayerId = pVS->Resv[9];
				//FrameType = pVS->Resv[6];
				//pBuf_SPS=(UINT8 *)SPS_Addr;
				//pBuf_PPS=(UINT8 *)PPS_Addr;
				//pbuf_stream =(UINT8 *) pVS->video_pack[3].phy_addr;

				pbuf_stream =(UINT8 *)StreamAddr;//pVS->video_pack[pVS->pack_num-1].phy_addr;
				//DBG_DUMP("clone pbuf[0]=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",  pbuf_stream[0],pbuf_stream[1],pbuf_stream[2],pbuf_stream[3],pbuf_stream[4],pbuf_stream[5],pbuf_stream[6],pbuf_stream[7],pbuf_stream[8],pbuf_stream[9]);

				//DBG_DUMP("clone pbuf_stream[0]=0x%x,%x,%x,%x,%x,%x\r\n",  pbuf_stream[0],pbuf_stream[1],pbuf_stream[2],pbuf_stream[3],pbuf_stream[4],pbuf_stream[5]);
				//DBG_DUMP("clone pbuf_stream[desc+0]=0x%x,%x,%x,%x,%x\r\n",  pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 0],pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 1],pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 2],pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 3],pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 4]);
				#if 0
				UINT32 k;
				DBG_DUMP("sz=%d, ",StreamSize);
				for(k=0;k<(SPS_Size + PPS_Size + VPS_Size+15);k++){
				//for(k=0;k<(15);k++){
					DBG_DUMP("0x%x, ", pbuf_stream[k]);
				}
				DBG_DUMP("\r\n");
				DBG_DUMP("tail, ");
				for(k=0;k<(15);k++){
					DBG_DUMP("0x%x, ", pbuf_stream[StreamSize-(SPS_Size + PPS_Size + VPS_Size)-15+k]);
				}
				DBG_DUMP("\r\n");

				#endif

				//pbuf_stream =(UINT8 *)StreamAddr;
				#if 0
				if(pVS->vcodec_format == HD_CODEC_TYPE_H264){
					if((pbuf_stream[SPS_Size + PPS_Size + 0]==0 && pbuf_stream[SPS_Size + PPS_Size + 1]==0 && pbuf_stream[SPS_Size + PPS_Size + 2]==0 && pbuf_stream[SPS_Size + PPS_Size + 3]==1 ) &&
						(*(pbuf_stream + SPS_Size + PPS_Size + 4) & 0x1F) ==H264_TYPE_IDR  && (*(pbuf_stream + SPS_Size + PPS_Size + 5)) == H264_START_CODE_I){ //I frame
						bIframe= 1;//StreamSender_Util_IsFrameI(StreamAddr);
					}else if(((*(pbuf_stream + 0) & 0x1F) ==0 && (*(pbuf_stream + 1) & 0x1F) ==0 && (*(pbuf_stream + 2) & 0x1F) ==0 && (*(pbuf_stream + 3) & 0x1F) ==1 ) &&
						(*(pbuf_stream + 4) & 0x1F) ==H264_TYPE_SLICE && (*(pbuf_stream + 5)) == H264_START_CODE_P){ //P frame
						bPframe= 1;//StreamSender_Util_IsFrameI(StreamAddr);
					}else{
						bIframe= 0;
					}
				}else{//MEDIAVIDENC_H265
					if( (pbuf_stream[0]==0 && pbuf_stream[1]==0 && pbuf_stream[2]==0 && pbuf_stream[3]==1 ) &&
						((*(pbuf_stream + 4) >>1 )&0x3F)==H265_TYPE_VPS  &&
						(pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 0]==0 && pbuf_stream[SPS_Size + PPS_Size +VPS_Size+  1]==0 && pbuf_stream[SPS_Size + PPS_Size +VPS_Size+  2]==0 && pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 3]==1 ) &&
						((*(pbuf_stream + SPS_Size + PPS_Size + VPS_Size+ 4) >>1 )&0x3F)==H265_TYPE_IDR){ //I frame
						bIframe= 1;//StreamSender_Util_IsFrameI(StreamAddr);
					}else if(((*(pbuf_stream + 0) & 0x1F) ==0 && (*(pbuf_stream + 1) & 0x1F) ==0 && (*(pbuf_stream + 2) & 0x1F) ==0 && (*(pbuf_stream + 3) & 0x1F) ==1 ) &&
						((*(pbuf_stream + 4) >>1)&0x3F) ==H265_TYPE_SLICE){ //P frmae
						bPframe= 1;//StreamSender_Util_IsFrameI(StreamAddr);
					}else{
						bIframe= 0;
					}
				}
				#else
				if(pVS->vcodec_format == HD_CODEC_TYPE_H264){
					if(pVS->video_pack[pVS->pack_num-1].pack_type.h264_type==H264_NALU_TYPE_IDR ){ //I frame
						bIframe= 1;
					}else if( pVS->video_pack[pVS->pack_num-1].pack_type.h264_type==H264_NALU_TYPE_SLICE){ //P frame
						bPframe= 1;
					}else{
						bIframe= 0;
					}
				}else{//MEDIAVIDENC_H265
					if(pVS->video_pack[pVS->pack_num-1].pack_type.h265_type==H265_NALU_TYPE_IDR ){ //I frame
						bIframe= 1;
					}else if( pVS->video_pack[pVS->pack_num-1].pack_type.h265_type==H265_NALU_TYPE_SLICE){ //P frame
						bPframe= 1;
					}else{
						bIframe= 0;
					}
				}
				#endif
				//DBG_DUMP("clone bIframe=%d, %d\r\n", bIframe,bPframe);

				UINT8 pbuf_startcode[10];
				pbuf_startcode[0]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 0];
				pbuf_startcode[1]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 1];
				pbuf_startcode[2]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 2];
				pbuf_startcode[3]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 3];
				pbuf_startcode[4]=pbuf_stream[SPS_Size + PPS_Size + VPS_Size+ 4];

				pbuf_startcode[5]=pbuf_stream[0];
				pbuf_startcode[6]=pbuf_stream[1];
				pbuf_startcode[7]=pbuf_stream[2];
				pbuf_startcode[8]=pbuf_stream[3];
				pbuf_startcode[9]=pbuf_stream[4];

				g_EthCamTxCloneId1_Prev_FrameIndex=g_EthCamTxCloneId1_FrameIndex;

				if(bIframe ==1){
					//DBG_DUMP("I\r\n");


					//frame_cnt++;
					//frame_index=1;
					g_EthCamTxCloneId1_FrameIndex=1;
					//make checksum
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						StreamAddr=StreamAddr+(SPS_Size + PPS_Size);//I Start
						StreamSize=StreamSize-(SPS_Size + PPS_Size); //I Size
					}
					//UINT32          uiTime=Perf_GetCurrent();
					if(StreamSize>checksumLen){
						checksum=0;
						pbuf_stream =(UINT8 *)StreamAddr;
						for(i=0;i<checksumLen;i++){
							checksum+= *(pbuf_stream+i);
						}
						checkSumHead=checksum & 0xff;//EthCamSocket_Checksum((UINT8 *)StreamAddr, 64) & 0xff;
						checksum=0;
						pbuf_stream =(UINT8 *)(StreamAddr+ StreamSize-checksumLen);
						for(i=0;i<checksumLen;i++){
							checksum+= *(pbuf_stream+i);
						}
						checkSumTail=checksum & 0xff;//EthCamSocket_Checksum((UINT8 *)(StreamAddr+ StreamSize-1-64), 64) & 0xff;
					}
					//DBG_DUMP("sz=%d, chksum=0x%x, 0x%x, %d, %d, %d\r\n", StreamSize,checkSumHead,checkSumTail,SPS_Size,PPS_Size,VPS_Size);
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						StreamAddr=StreamAddr-(SPS_Size + PPS_Size);
						StreamSize=StreamSize+ (SPS_Size + PPS_Size);
					}

					//StreamAddr-=(12);//shift left 12 bytes for 2 bytes sps size,  2 bytes pps size,  8 bytes I frame custom header
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						StreamAddr-=(12+LONGCNT_LENGTH);//shift left 20 bytes for 2 bytes sps size,  2 bytes pps size,  8 bytes longconter, 8 bytes I frame custom header
					}else{
						StreamAddr-=(8+LONGCNT_LENGTH);//shift left 16 bytes for  8 bytes longconter, 8 bytes I frame custom header
					}
					pbuf_stream =(UINT8 *)StreamAddr;

					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						////hwmem_open();
						////hwmem_memcpy((UINT32)&pbuf_stream[2], (UINT32)(SPS_Addr), (UINT32)SPS_Size);
						////hwmem_memcpy((UINT32)&pbuf_stream[2+SPS_Size+2], (UINT32)(PPS_Addr), (UINT32)PPS_Size);
						////hwmem_close();
						SPS_Addr = PHY2VIRT_MAIN(SPS_Addr_pa);//(UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, SPS_Addr_pa, SPS_Size);
						PPS_Addr =  PHY2VIRT_MAIN(PPS_Addr_pa);//(UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, PPS_Addr_pa, PPS_Size);
						#if 0
						static UINT16 bFirstIn=1;
						if(bFirstIn){
							bFirstIn=0;
							DBG_DUMP("SPS SPS_Size=%d, \r\n",SPS_Size);
							UINT16 zz;
							UINT8 *SPSTmp=(UINT8 *) SPS_Addr;
							for(zz=0;zz<SPS_Size;zz++){
								DBG_DUMP("0x%x, ", SPSTmp[zz]);
							}
							DBG_DUMP("PPS PPS_Size=%d, \r\n",PPS_Size);
							UINT8 *PPSTmp=(UINT8 *) PPS_Addr;
							for(zz=0;zz<PPS_Size;zz++){
								DBG_DUMP("0x%x, ", PPSTmp[zz]);
							}
						}
						#endif
						memcpy((UINT8*)&pbuf_stream[2], (UINT8*)(SPS_Addr), (UINT32)SPS_Size);
						memcpy((UINT8*)&pbuf_stream[2+SPS_Size+2], (UINT8*)(PPS_Addr), (UINT32)PPS_Size);

						pbuf_stream[0]=SPS_Size & 0xff;
						pbuf_stream[1]=(SPS_Size & 0xff00) >> 8;
						pbuf_stream[2+SPS_Size]=PPS_Size & 0xff;
						pbuf_stream[2+SPS_Size+1]=(PPS_Size & 0xff00) >> 8;

						#if 1//add longcounter
						UINT64 LongCounter=pvenc_data->timestamp;//HwClock_GetLongCounter();
						//Hi byte
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+0]=((LongCounter >> 32) & 0xff000000)>>24; 	//-16
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+1]=((LongCounter >> 32) & 0x00ff0000)>>16; 	//-15
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+2]=((LongCounter >> 32) & 0x0000ff00)>>8;  	//-14
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+3]=(LongCounter >> 32) & 0x000000ff;       	//-13
						//Low byte
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+4]=(LongCounter & 0xff000000)>>24; //-12
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+5]=(LongCounter & 0x00ff0000)>>16; //-11
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+6]=(LongCounter & 0x0000ff00)>>8; 	//-10
						pbuf_stream[2+SPS_Size + 2+ PPS_Size+7]=LongCounter & 0x000000ff; 		//-9
						#endif

						StreamSize=StreamSize- (SPS_Size + PPS_Size);//I Frame Size
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size]=StreamSize & 0xff;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+1]=(StreamSize & 0xff00) >> 8;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+2]=(StreamSize & 0xff0000) >> 16;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+3]=g_EthCamTxCloneId1_FrameIndex;//frame_index;

						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+4]=checkSumHead;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+5]=checkSumTail;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+6]=0;
						pbuf_stream[LONGCNT_LENGTH+2+SPS_Size + 2+ PPS_Size+7]=LONGCNT_STAMP_EN;
						StreamSize=StreamSize+ (SPS_Size + PPS_Size)+ 12+LONGCNT_LENGTH;
					}else{
						#if 1//add longcounter
						UINT64 LongCounter=pvenc_data->timestamp;//HwClock_GetLongCounter();
						//Hi byte, sec
						pbuf_stream[0]=((LongCounter >> 32) & 0xff000000)>>24; 	//-16
						pbuf_stream[1]=((LongCounter >> 32) & 0x00ff0000)>>16; 	//-15
						pbuf_stream[2]=((LongCounter >> 32) & 0x0000ff00)>>8;  	//-14
						pbuf_stream[3]=(LongCounter >> 32) & 0x000000ff;       	//-13
						//Low byte, usec
						pbuf_stream[4]=(LongCounter & 0xff000000)>>24; //-12
						pbuf_stream[5]=(LongCounter & 0x00ff0000)>>16; //-11
						pbuf_stream[6]=(LongCounter & 0x0000ff00)>>8; 	//-10
						pbuf_stream[7]=LongCounter & 0x000000ff; 		//-9
						#endif

						//StreamSize=StreamSize- (SPS_Size + PPS_Size+ VPS_Size);//I Frame Size
						pbuf_stream[LONGCNT_LENGTH]=StreamSize & 0xff;
						pbuf_stream[LONGCNT_LENGTH+1]=(StreamSize & 0xff00) >> 8;
						pbuf_stream[LONGCNT_LENGTH+2]=(StreamSize & 0xff0000) >> 16;
						pbuf_stream[LONGCNT_LENGTH+3]=g_EthCamTxCloneId1_FrameIndex;//frame_index;

						pbuf_stream[LONGCNT_LENGTH+4]=checkSumHead;
						pbuf_stream[LONGCNT_LENGTH+5]=checkSumTail;
						pbuf_stream[LONGCNT_LENGTH+6]=(SPS_Size + PPS_Size + VPS_Size);
						pbuf_stream[LONGCNT_LENGTH+7]=LONGCNT_STAMP_EN;
						StreamSize=StreamSize+  8 +LONGCNT_LENGTH;
					}



					#if 1//add debug info
					if (pVS->vcodec_format == HD_CODEC_TYPE_H264) {
						pbuf_stream =(UINT8 *)(StreamAddr + SPS_Size + 2+ PPS_Size +2);
					}else{
						pbuf_stream =(UINT8 *)(StreamAddr);
					}
					//memset(&g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][0], 0, ETHCAM_FRM_HEADER_SIZE);
					memset(&g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][0], 0, ETHCAM_FRM_HEADER_SIZE);
					for(i=0;i<ETHCAM_FRM_HEADER_SIZE_PART1;i++){
						//g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][i]=pbuf_stream[i];
						g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][i]=pbuf_stream[i];
					}

					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][0]=pVS->Resv[6];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][1]=pVS->Resv[7];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][2]=pVS->Resv[8];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][3]=pVS->Resv[9];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][4]=pVS->Resv[10];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][5]=pVS->vcodec_format;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1]=bIframe;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+1]=bPframe;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+2]=SPS_Size ;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+3]=PPS_Size;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+4]=VPS_Size;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+5]=g_EthCamTxCloneId1_FrameIndex;

					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+6]=pbuf_startcode[0];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+7]=pbuf_startcode[1];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+8]=pbuf_startcode[2];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+9]=pbuf_startcode[3];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+10]=pbuf_startcode[4];

					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+11]=pbuf_startcode[5];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+12]=pbuf_startcode[6];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+13]=pbuf_startcode[7];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+14]=pbuf_startcode[8];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+15]=pbuf_startcode[9];


					g_CloneId1_Tx_PrevHeaderBufIdx++;
					g_CloneId1_Tx_PrevHeaderBufIdx= (g_CloneId1_Tx_PrevHeaderBufIdx >=ETHCAM_PREVHEADER_MAX_FRMNUM) ? 0 : g_CloneId1_Tx_PrevHeaderBufIdx;
					#endif

					if(pSndFunc){
						pSndFunc((char *)StreamAddr, (int*)&StreamSize);
					}

				}else  if(bPframe==1){ // p frame
					//DBG_DUMP("PPPPPPP\r\n");
					//fflush(stdout);
					//frame_index++;
					g_EthCamTxCloneId1_FrameIndex++;
					//frame_cnt++;

					if(StreamSize>checksumLen){
						checksum=0;
						pbuf_stream =(UINT8 *)StreamAddr;
						for(i=0;i<checksumLen;i++){
							checksum+= *(pbuf_stream+i);
						}
						checkSumHead=checksum & 0xff;//EthCamSocket_Checksum((UINT8 *)StreamAddr, 64) & 0xff;
						checksum=0;
						pbuf_stream =(UINT8 *)(StreamAddr+ StreamSize-checksumLen);
						for(i=0;i<checksumLen;i++){
							checksum+= *(pbuf_stream+i);
						}
						checkSumTail=checksum & 0xff;//EthCamSocket_Checksum((UINT8 *)(StreamAddr+ StreamSize-1-64), 64) & 0xff;
					}

					StreamAddr-=8;
					pbuf_stream =(UINT8 *)StreamAddr;

					pbuf_stream[0]=StreamSize & 0xff;
					pbuf_stream[1]=(StreamSize & 0xff00) >> 8;
					//pbuf_stream[2]=(StreamSize & 0xff0000) >> 16;
					pbuf_stream[2]=((StreamSize & 0x0f0000) >> 16)   | (((g_EthCamTxCloneId1_FrameIndex & 0xf00)>>8)<<4);
					pbuf_stream[3]=g_EthCamTxCloneId1_FrameIndex & 0xff;//frame_index;

					pbuf_stream[4]=checkSumHead;
					pbuf_stream[5]=checkSumTail;
					pbuf_stream[6]=0;
					pbuf_stream[7]=LONGCNT_STAMP_EN; // if longconter =2, else =0

					StreamSize+=8;

					#if 1//add timestamp
					StreamSize=StreamSize+ LONGCNT_LENGTH;
					StreamAddr-=LONGCNT_LENGTH;
					pbuf_stream =(UINT8 *)StreamAddr;
					UINT64 LongCounter=pvenc_data->timestamp; //hd_gettime_us(); hwclock_get_longcounter();
					//Hi byte ,sec
					pbuf_stream[0]=((LongCounter >> 32) & 0xff000000)>>24; 	//-16
					pbuf_stream[1]=((LongCounter >> 32) & 0x00ff0000)>>16; 	//-15
					pbuf_stream[2]=((LongCounter >> 32) & 0x0000ff00)>>8;  	//-14
					pbuf_stream[3]=(LongCounter >> 32) & 0x000000ff;       	//-13
					//Low byte ,usec
					pbuf_stream[4]=(LongCounter & 0xff000000)>>24; //-12
					pbuf_stream[5]=(LongCounter & 0x00ff0000)>>16; //-11
					pbuf_stream[6]=(LongCounter & 0x0000ff00)>>8; 	//-10
					pbuf_stream[7]=LongCounter & 0x000000ff; 		//-9
					#endif

					#if 1 //add debug info
					pbuf_stream =(UINT8 *)StreamAddr;
					//memset(&g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][0], 0, ETHCAM_FRM_HEADER_SIZE);
					memset(&g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][0], 0, ETHCAM_FRM_HEADER_SIZE);
					for(i=0;i<ETHCAM_FRM_HEADER_SIZE_PART1;i++){
						//g_RecId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][i]=pbuf_stream[i];
						g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][i]=pbuf_stream[i];
					}

					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][0]=pVS->Resv[6];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][1]=pVS->Resv[7];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][2]=pVS->Resv[8];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][3]=pVS->Resv[9];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][4]=pVS->Resv[10];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][5]=pVS->vcodec_format;


					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1]=bIframe;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+1]=bPframe;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+2]=SPS_Size ;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+3]=PPS_Size;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+4]=VPS_Size;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+5]=g_EthCamTxCloneId1_FrameIndex & 0xff;

					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+6]=pbuf_startcode[0];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+7]=pbuf_startcode[1];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+8]=pbuf_startcode[2];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+9]=pbuf_startcode[3];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+10]=pbuf_startcode[4];

					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+11]=pbuf_startcode[5];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+12]=pbuf_startcode[6];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+13]=pbuf_startcode[7];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+14]=pbuf_startcode[8];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+15]=pbuf_startcode[9];


					g_CloneId1_Tx_PrevHeaderBufIdx++;
					g_CloneId1_Tx_PrevHeaderBufIdx= (g_CloneId1_Tx_PrevHeaderBufIdx >=ETHCAM_PREVHEADER_MAX_FRMNUM) ? 0 : g_CloneId1_Tx_PrevHeaderBufIdx;
					#endif
					if(pSndFunc){
						pSndFunc((char *)StreamAddr, (int*)&StreamSize);
					}
				}else{
					DBG_ERR("frame type error\r\n");
					#if 1//add debug info
					pbuf_stream =(UINT8 *)StreamAddr;
					//memset(&g_CloneId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][0], 0, ETHCAM_FRM_HEADER_SIZE);
					memset(&g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][0], 0, ETHCAM_FRM_HEADER_SIZE);
					for(i=0;i<ETHCAM_FRM_HEADER_SIZE_PART1;i++){
						//g_CloneId1_Tx_PrevHeader[(frame_index%ETHCAM_PREVHEADER_MAX_FRMNUM)-1][i]=pbuf_stream[i];
						g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][i]=pbuf_stream[i];
					}

					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][0]=pVS->Resv[6];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][1]=pVS->Resv[7];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][2]=pVS->Resv[8];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][3]=pVS->Resv[9];
					//g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][4]=pVS->Resv[10];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][5]=pVS->vcodec_format;

					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1]=bIframe;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+1]=bPframe;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+2]=SPS_Size ;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+3]=PPS_Size;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+4]=VPS_Size;
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+5]=g_EthCamTxCloneId1_FrameIndex & 0xff;

					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+6]=pbuf_startcode[0];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+7]=pbuf_startcode[1];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+8]=pbuf_startcode[2];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+9]=pbuf_startcode[3];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+10]=pbuf_startcode[4];

					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+11]=pbuf_startcode[5];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+12]=pbuf_startcode[6];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+13]=pbuf_startcode[7];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+14]=pbuf_startcode[8];
					g_CloneId1_Tx_PrevHeader[g_CloneId1_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1+15]=pbuf_startcode[9];


					g_CloneId1_Tx_PrevHeaderBufIdx++;
					g_CloneId1_Tx_PrevHeaderBufIdx= (g_CloneId1_Tx_PrevHeaderBufIdx >=ETHCAM_PREVHEADER_MAX_FRMNUM) ? 0 : g_CloneId1_Tx_PrevHeaderBufIdx;
					#endif
					return E_SYS;
				}

#if 0//write to a file
			static UINT32 file_count_max = 300;
			static UINT32 file_index = 0;
			static char file_name[50] = {0};
			static FST_FILE  FileHdl;
			char enc_typ = 0x65;
			ISF_VIDEO_STREAM_BUF *pVS_tmp = (ISF_VIDEO_STREAM_BUF *)pImgBuf;
			UINT32 tmp_size = pVS_tmp->Resv[1] + pVS_tmp->Resv[3];
			UINT32 header_addr = pVS_tmp->Resv[0];
			static UINT32 frame_index2=0;
			StreamAddr = pVS_tmp->DataAddr;
			StreamSize = pVS_tmp->DataSize;
			SPS_Addr = pVS_tmp->Resv[0]; //sps, pps addr
			SPS_Size = pVS_tmp->Resv[1];//sps, pps data size
			PPS_Addr = pVS_tmp->Resv[2]; //sps, pps addr
			PPS_Size = pVS_tmp->Resv[3];//sps, pps data size

			if (pVS_tmp->CodecType == MEDIAVIDENC_H264) {
				sprintf(file_name, "A:\\MovieH264_%dX%dp%d.264", pVS_tmp->Width, pVS_tmp->Height, pVS_tmp->FramePerSecond);
				enc_typ = 0x65;
				tmp_size = pVS_tmp->Resv[1] + pVS_tmp->Resv[3];
				header_addr = pVS_tmp->Resv[0];
			}
			if (file_index == 0) {
				DBG_DUMP("start write stream to file %s\r\n", file_name);
				FileHdl = FileSys_OpenFile(file_name, FST_OPEN_WRITE | FST_CREATE_ALWAYS);
			}
			if (file_index < file_count_max) {
				if (*((char *)StreamAddr + 3) == 0x01 && *((char *)StreamAddr + 4) == enc_typ) {
					////this is a I frame, need add SPS PPS data first
						//pbuf_size = (UINT8 *)&SPS_Size;
						pbuf_size[0]=SPS_Size & 0xff;
						pbuf_size[1]=(SPS_Size & 0xff00) >> 8;
						pbuf_size[2]=(SPS_Size & 0xff0000) >> 16;
						pbuf_size[3]=0;
					FileSys_WriteFile(FileHdl, (UINT8 *)pbuf_size, &bufSize, 0, NULL);
					FileSys_WriteFile(FileHdl, (UINT8 *)SPS_Addr, &SPS_Size, 0, NULL);

						//pbuf_size = (UINT8 *)&PPS_Size;
						pbuf_size[0]=PPS_Size & 0xff;
						pbuf_size[1]=(PPS_Size & 0xff00) >> 8;
						pbuf_size[2]=(PPS_Size & 0xff0000) >> 16;
						pbuf_size[3]=0;
					FileSys_WriteFile(FileHdl, (UINT8 *)pbuf_size, &bufSize, 0, NULL);
					FileSys_WriteFile(FileHdl, (UINT8 *)PPS_Addr, &PPS_Size, 0, NULL);

					//FileSys_WriteFile(FileHdl, (UINT8 *)header_addr, &tmp_size, 0, NULL);
						frame_index2=1;
					}else{
						frame_index2++;
				}
					//pbuf_size = (UINT8 *)&StreamSize;
					pbuf_size[0]=StreamSize & 0xff;
					pbuf_size[1]=(StreamSize & 0xff00) >> 8;
					pbuf_size[2]=(StreamSize & 0xff0000) >> 16;
					pbuf_size[3]=frame_index2;
				FileSys_WriteFile(FileHdl, (UINT8 *)pbuf_size, &bufSize, 0, NULL);
				FileSys_WriteFile(FileHdl, (UINT8 *)StreamAddr, &StreamSize, 0, NULL);
				file_index++;
			}
			if (file_index == file_count_max) {
				DBG_DUMP("finish write stream to file\r\n");
				FileSys_CloseFile(FileHdl);
				file_index++;
			}
#endif
			}
		}else{
			DBG_ERR("vcodec_format error\r\n");
			//return ISF_ERR_NOT_SUPPORT;
			return E_SYS;
		}
	}else{
		DBG_ERR("sign error\r\n");
	}


	return ret;
}
