#include "ImageApp_MovieMulti_int.h"
#include <stdlib.h>
#include <string.h>

/// ========== Cross file variables ==========
UINT32 IsImgLinkOpened[MAX_IMG_PATH] = {0};
MOVIE_IMAGE_LINK ImgLink[MAX_IMG_PATH];
MOVIE_IMAGE_LINK_STATUS ImgLinkStatus[MAX_IMG_PATH] = {0};
UINT32 gImgLinkAlgFuncEn[MAX_IMG_PATH] = {0};
UINT32 gImeMaxOutPath[MAX_IMG_PATH] = {0};
UINT32 gImgLinkForcedSizePath[MAX_IMG_PATH] = {0};
UINT32 gSwitchLink[MAX_IMG_PATH][6];    // record switch_ro1~6 to switch ri1~4 mapping table, 0:main, 1:clone, 2:disp, 3:wifi, 4:alg, 5:disp2
UINT32 gUserProcMap[MAX_IMG_PATH][4];   // record switch_ri1~4 to userproc o1~4 mapping table
UINT32 fps_vcap_out[MAX_IMG_PATH] = {0}, fps_vprc_p_out[MAX_IMG_PATH][3] = {0}, fps_vprc_e_out[MAX_IMG_PATH][6] = {0};
MOVIEMULTI_IPL_SIZE_INFO IPLUserSize[MAX_IMG_PATH] = {0};
UINT32 IsSetImgLinkRecInfo[MAX_IMG_PATH][2] = {0};
MOVIE_RECODE_INFO ImgLinkRecInfo[MAX_IMG_PATH][2] = {0};
UINT32 ImgLinkTimelapseTime[MAX_IMG_PATH][2] = {0};
UINT32 ImgLinkTimelapsePlayRate = 30;
UINT32 IsSetImgLinkAlgInfo[MAX_IMG_PATH] = {0};
MOVIE_ALG_INFO ImgLinkAlgInfo[MAX_IMG_PATH] = {0};
MOVIEMULTI_IME_CROP_INFO DispIMECropInfo[MAX_IMG_PATH] = {0};
MOVIEMULTI_IME_CROP_INFO MainIMECropInfo[MAX_IMG_PATH] = {0};
MOVIEMULTI_IME_CROP_INFO CloneIMECropInfo[MAX_IMG_PATH] = {0};
UINT32 DispIMECropNoAutoScaling[MAX_IMG_PATH] = {0};
UINT32 DispIMECropFixedOutSize[MAX_IMG_PATH] = {0};
UINT32 IsImgLinkForEthCamTxEnabled[MAX_IMG_PATH][2] = {0};
HD_VIDEOCAP_DRV_CONFIG img_vcap_cfg[MAX_IMG_PATH] = {0};
HD_VIDEO_PXLFMT img_vcap_senout_pxlfmt[MAX_IMG_PATH] = {0};
HD_VIDEO_PXLFMT img_vcap_capout_pxlfmt[MAX_IMG_PATH] = {0};
UINT32 img_vcap_data_lane[MAX_IMG_PATH] = {0};
HD_VIDEOCAP_CTRL img_vcap_ctrl[MAX_IMG_PATH] = {0};
HD_VIDEOCAP_IN img_vcap_in_param[MAX_IMG_PATH] = {0};
HD_VIDEOCAP_CROP img_vcap_crop_param[MAX_IMG_PATH] = {0};
HD_VIDEOCAP_OUT img_vcap_out_param[MAX_IMG_PATH] = {0};
HD_VIDEOCAP_FUNC_CONFIG img_vcap_func_cfg[MAX_IMG_PATH] = {0};
VENDOR_VIDEOCAP_CCIR_FMT_SEL img_vcap_ccir_fmt[MAX_IMG_PATH] = {0};
UINT32 img_vcap_ccir_mode[MAX_IMG_PATH] = {0};
UINT32 img_vcap_detect_loop[MAX_IMG_PATH] = {0};
UINT32 img_vcap_patgen[MAX_IMG_PATH] = {0};
MOVIEMULTI_VCAP_CROP_INFO img_vcap_crop_win[MAX_IMG_PATH] = {0};
USIZE img_vcap_out_size[MAX_IMG_PATH] = {0};
#if defined(_BSP_NA51089_)
UINT32 img_vcap_sie_remap[MAX_IMG_PATH] = {0};
#endif
UINT32 img_vcap_gyro_func[MAX_IMG_PATH] = {0};
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
UINT32 img_vproc_vpe_en[MAX_IMG_PATH] = {0};
MovieVpeCb *img_vproc_vpe_cb[MAX_IMG_PATH] = {0};
UINT32 img_vproc_vpe_scene[MAX_IMG_PATH] = {0};
#endif
UINT32 img_vproc_splitter[MAX_IMG_PATH] = {0};
HD_VIDEOPROC_DEV_CONFIG img_vproc_cfg[MAX_IMG_PATH] = {0};
HD_VIDEOPROC_CTRL img_vproc_ctrl[MAX_IMG_PATH] = {0};
HD_VIDEOPROC_IN img_vproc_in_param[MAX_IMG_PATH] = {0};
HD_VIDEOPROC_FUNC_CONFIG img_vproc_func_cfg[MAX_IMG_PATH] = {0};
UINT32 img_vproc_yuv_compress[MAX_IMG_PATH] = {0};
UINT32 img_vproc_3dnr_path[MAX_IMG_PATH] = {0};
UINT32 img_vproc_forced_use_pipe[MAX_IMG_PATH] = {0};
UINT32 img_vproc_dis_func[MAX_IMG_PATH] = {0};
UINT32 img_vproc_eis_func[MAX_IMG_PATH] = {0};
UINT32 img_vproc_queue_depth[MAX_IMG_PATH] = {0};
UINT32 img_vproc_no_ext[MAX_IMG_PATH] = {0};
HD_VIDEOENC_PATH_CONFIG img_venc_path_cfg[MAX_IMG_PATH][2] = {0};
HD_VIDEOENC_IN  img_venc_in_param[MAX_IMG_PATH][2] = {0};
HD_VIDEOENC_OUT2 img_venc_out_param[MAX_IMG_PATH][2] = {0};
HD_H26XENC_RATE_CONTROL2 img_venc_rc_param[MAX_IMG_PATH][2] = {0};
HD_VIDEOENC_BUFINFO img_venc_bs_buf[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_bs_buf_va[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_quality_base_mode_en[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_sout_type[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_sout_pa[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_sout_va[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_sout_size[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_vui_disable[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_vui_color_tv_range[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_max_frame_size[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_imgcap_yuvsrc[MAX_IMG_PATH][2] = {0};
HD_COMMON_MEM_VB_BLK img_venc_sout_blk[MAX_IMG_PATH][2] = {0};
MOVIE_CFG_PROFILE img_venc_h264_profile[MAX_IMG_PATH][2] = {0};
MOVIE_CFG_LEVEL img_venc_h264_level[MAX_IMG_PATH][2] = {0};
MOVIE_CFG_PROFILE img_venc_h265_profile[MAX_IMG_PATH][2] = {0};
MOVIE_CFG_LEVEL img_venc_h265_level[MAX_IMG_PATH][2] = {0};
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
VENDOR_VDOENC_XVR_APP img_venc_xvr_app[MAX_IMG_PATH][2] = {0};
#endif
UINT32 img_venc_skip_frame_cfg[MAX_IMG_PATH][2] = {0};
HD_VIDEOENC_SVC_LAYER img_venc_svc[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_low_power_mode[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_trigger_mode[MAX_IMG_PATH][2] = {0};
UINT64 img_venc_timestamp[MAX_IMG_PATH][2] = {0};
HD_VIDEOENC_MAXMEM img_venc_maxmem[MAX_IMG_PATH][2] = {0};
HD_BSMUX_VIDEOINFO img_bsmux_vinfo[MAX_IMG_PATH][4] = {0};
HD_BSMUX_AUDIOINFO img_bsmux_ainfo[MAX_IMG_PATH][4] = {0};
HD_BSMUX_FILEINFO img_bsmux_finfo[MAX_IMG_PATH][4] = {0};
UINT32 img_bsmux_front_moov_en[MAX_IMG_PATH][4] = {0};
UINT32 img_bsmux_front_moov_flush_sec[MAX_IMG_PATH][4] = {0};
UINT32 img_bsmux_2v1a_mode[MAX_IMG_PATH] = {0};
UINT32 img_bsmux_meta_data_en[MAX_IMG_PATH][4] = {0};
UINT32 img_bsmux_audio_pause[MAX_IMG_PATH][4] = {0};
UINT32 img_bsmux_cutnow_with_period_cnt[MAX_IMG_PATH][4] = {0};
MOVIE_RECODE_FILE_OPTION ImgLinkFileInfo = {0};
UINT32 forced_use_unique_3dnr_path[MAX_IMG_PATH] = {0};
UINT32 use_unique_3dnr_path[MAX_IMG_PATH] = {0};
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
UINT32 img_venc_frame_cnt[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_frame_cnt_clr[MAX_IMG_PATH][2] = {0};
UINT32 img_venc_1st_path_idx = UNUSED;
UINT32 img_venc_1st_path_ch;
#endif
UINT32 trig_once_limited[MAX_IMG_PATH][2] = {0};
UINT32 trig_once_cnt[MAX_IMG_PATH][2] = {0};
UINT32 trig_once_first_i[MAX_IMG_PATH][2] = {0};
UINT32 trig_once_2v1a_stop_path[MAX_IMG_PATH][2] = {0};
UINT32 gpsdata_pa = 0, gpsdata_va = 0, gpsdata_size = 0, gpsdata_offset = 0;
UINT32 gpsdata_rec_pa[MAX_IMG_PATH][2] = {0}, gpsdata_rec_va[MAX_IMG_PATH][2] = {0}, gpsdata_rec_size[MAX_IMG_PATH][2] = {0}, gpsdata_rec_offset[MAX_IMG_PATH][2] = {0};
UINT32 gpsdata_eth_pa[MAX_ETHCAM_PATH][1] = {0}, gpsdata_eth_va[MAX_ETHCAM_PATH][1] = {0}, gpsdata_eth_size[MAX_ETHCAM_PATH][1] = {0}, gpsdata_eth_offset[MAX_ETHCAM_PATH][1] = {0};
UINT32 gpsdata_rec_disable[MAX_IMG_PATH][2] = {0}, gpsdata_rec_rate[MAX_IMG_PATH][2] = {0};
UINT32 gpsdata_eth_disable[MAX_ETHCAM_PATH][1] = {0}, gpsdata_eth_rate[MAX_ETHCAM_PATH][1] = {0};
UINT32 img_pseudo_rec_mode[MAX_IMG_PATH][2] = {0};
UINT32 img_manual_push_vdo_frame[MAX_IMG_PATH][2] = {0};
UINT32 img_manual_push_raw_frame[MAX_IMG_PATH] = {0};
UINT32 img_manual_push_vpe_frame[MAX_IMG_PATH] = {0};
UINT32 img_to_imgcap_id = UNUSED;
UINT32 img_to_imgcap_path = UNUSED;
HD_VIDEO_FRAME *p_img_to_imgcap_frame = NULL;
UINT32 img_emr_loop_start[MAX_IMG_PATH][2] = {0};
USIZE img_venc_imgcap_thum_size[MAX_IMG_PATH][2] = {0};

/// ========== Noncross file variables ==========
static MOVIE_IMG_IME_SETTING g_ImeSetting[MAX_IMG_PATH][4];  // IME p1/p2/p3-1(trans)/p3-2(trans)/p4

#define ISP_ID_REMAP_SIE(vcap_id, sie_map_id)  ((1 << 31) | (vcap_id << 24) | sie_map_id)

static ER _ImageApp_MovieMulti_PreCfgSwitch(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j;
	MOVIE_IMG_IME_SETTING img[5] = {0};
	SW_PORT_TABLE imgtb = {0};
	SIZECONVERT_INFO scale_info = {0};
	USIZE sz;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_REC_ID_1;

	// main
	if (MainIMECropInfo[idx].IMEWin.w && MainIMECropInfo[idx].IMEWin.h) {
		img[0].size.w = MainIMECropInfo[idx].IMEWin.w;
		img[0].size.h = MainIMECropInfo[idx].IMEWin.h;
	} else {
		img[0].size.w = ImgLinkRecInfo[idx][0].size.w;
		img[0].size.h = ImgLinkRecInfo[idx][0].size.h;
	}
	if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
		img[0].fps = IPLUserSize[idx].fps;
	} else {
		img[0].fps = ImgLinkRecInfo[idx][0].frame_rate;
	}
	img[0].fmt = HD_VIDEO_PXLFMT_YUV420;
	img[0].ratio.w = ImgLinkRecInfo[idx][0].disp_ratio.w;
	img[0].ratio.h = ImgLinkRecInfo[idx][0].disp_ratio.h;
	// clone
	if (CloneIMECropInfo[idx].IMEWin.w && CloneIMECropInfo[idx].IMEWin.h) {
		img[1].size.w = CloneIMECropInfo[idx].IMEWin.w;
		img[1].size.h = CloneIMECropInfo[idx].IMEWin.h;
	} else {
		img[1].size.w = ImgLinkRecInfo[idx][1].size.w;
		img[1].size.h = ImgLinkRecInfo[idx][1].size.h;
	}
	if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
		img[1].fps = IPLUserSize[idx].fps;
	} else {
		img[1].fps = ImgLinkRecInfo[idx][1].frame_rate;
	}
	img[1].fmt = HD_VIDEO_PXLFMT_YUV420;
	img[1].ratio.w = ImgLinkRecInfo[idx][1].disp_ratio.w;
	img[1].ratio.h = ImgLinkRecInfo[idx][1].disp_ratio.h;
	// disp (if !g_DispFps[0], use max fps as display fps)
	if (MaxLinkInfo.MaxDispLink) {
		_ImageApp_MovieMulti_DispGetSize(_CFG_DISP_ID_1, &sz);

		if (DispIMECropNoAutoScaling[idx] && (DispIMECropInfo[idx].IMEWin.w == 0 || DispIMECropInfo[idx].IMEWin.h == 0)) {
			DBG_WRN("MOVIEMULTI_PARAM_DISP_IME_CROP_AUTO_SCALING_EN is DISABLE but MOVIEMULTI_PARAM_DISP_IME_CROP is not set! No effect!\r\n");
		}

		if (DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h && DispIMECropNoAutoScaling[idx]) {
			img[2].size.w = DispIMECropInfo[idx].IMEWin.w;
			img[2].size.h = DispIMECropInfo[idx].IMEWin.h;
			if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
				img[2].fps = IPLUserSize[idx].fps;
			} else {
				img[2].fps = g_DispFps[0];
			}
			img[2].fmt = HD_VIDEO_PXLFMT_YUV420;
			img[2].ratio.w = ImgLinkRecInfo[idx][0].disp_ratio.w;
			img[2].ratio.h = ImgLinkRecInfo[idx][0].disp_ratio.h;
			DispLink[0].vout_win.x = 0;
			DispLink[0].vout_win.y = 0;
			if (((DispLink[0].vout_dir & HD_VIDEO_DIR_ROTATE_270) == HD_VIDEO_DIR_ROTATE_270) || ((DispLink[0].vout_dir & HD_VIDEO_DIR_ROTATE_90) == HD_VIDEO_DIR_ROTATE_90)) {
				DispLink[0].vout_win.w = sz.h;
				DispLink[0].vout_win.h = sz.w;
			} else {
				DispLink[0].vout_win.w = sz.w;
				DispLink[0].vout_win.h = sz.h;
			}
		} else {
			if (((DispLink[0].vout_dir & HD_VIDEO_DIR_ROTATE_270) == HD_VIDEO_DIR_ROTATE_270) || ((DispLink[0].vout_dir & HD_VIDEO_DIR_ROTATE_90) == HD_VIDEO_DIR_ROTATE_90)) {
				if (DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h) {
					scale_info.uiSrcWidth  = DispIMECropInfo[idx].IMEWin.w;
					scale_info.uiSrcHeight = DispIMECropInfo[idx].IMEWin.h;
				} else {
					scale_info.uiSrcWidth  = ImgLinkRecInfo[idx][0].size.w;
					scale_info.uiSrcHeight = ImgLinkRecInfo[idx][0].size.h;
				}
				scale_info.uiDstWidth  = sz.h;
				scale_info.uiDstHeight = sz.w;
				scale_info.uiDstWRatio = DispLink[0].vout_ratio.w;
				scale_info.uiDstHRatio = DispLink[0].vout_ratio.h;
				scale_info.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
				DisplaySizeConvert(&scale_info);
				scale_info.uiOutWidth  = ALIGN_CEIL_16(scale_info.uiOutWidth);
				scale_info.uiOutHeight = ALIGN_CEIL_8 (scale_info.uiOutHeight);
				DispLink[0].vout_win.x = scale_info.uiOutX;
				DispLink[0].vout_win.y = scale_info.uiOutY;
				DispLink[0].vout_win.w = scale_info.uiOutWidth;
				DispLink[0].vout_win.h = scale_info.uiOutHeight;
				//DBG_DUMP("Disp:x=%d,y=%d,w=%d,h=%d\r\n", scale_info.uiOutX, scale_info.uiOutY, scale_info.uiOutWidth, scale_info.uiOutHeight);
				img[2].size.w = scale_info.uiOutWidth;
				img[2].size.h = scale_info.uiOutHeight;
				if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
					img[2].fps = IPLUserSize[idx].fps;
				} else {
					img[2].fps = g_DispFps[0];
				}
				img[2].fmt = HD_VIDEO_PXLFMT_YUV420;
				img[2].ratio.w = ImgLinkRecInfo[idx][0].disp_ratio.w;
				img[2].ratio.h = ImgLinkRecInfo[idx][0].disp_ratio.h;
			} else {
				if (DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h) {
					scale_info.uiSrcWidth  = DispIMECropInfo[idx].IMEWin.w;
					scale_info.uiSrcHeight = DispIMECropInfo[idx].IMEWin.h;
				} else {
					scale_info.uiSrcWidth  = ImgLinkRecInfo[idx][0].size.w;
					scale_info.uiSrcHeight = ImgLinkRecInfo[idx][0].size.h;
				}
				scale_info.uiDstWidth  = sz.w;
				scale_info.uiDstHeight = sz.h;
				scale_info.uiDstWRatio = DispLink[0].vout_ratio.w;
				scale_info.uiDstHRatio = DispLink[0].vout_ratio.h;
				scale_info.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
				DisplaySizeConvert(&scale_info);
				DispLink[0].vout_win.x = scale_info.uiOutX;
				DispLink[0].vout_win.y = scale_info.uiOutY;
				DispLink[0].vout_win.w = scale_info.uiOutWidth;
				DispLink[0].vout_win.h = scale_info.uiOutHeight;
				//DBG_DUMP("Disp:x=%d,y=%d,w=%d,h=%d\r\n", scale_info.uiOutX, scale_info.uiOutY, scale_info.uiOutWidth, scale_info.uiOutHeight);
				img[2].size.w = scale_info.uiOutWidth;
				img[2].size.h = scale_info.uiOutHeight;
				if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
					img[2].fps = IPLUserSize[idx].fps;
				} else {
					img[2].fps = g_DispFps[0];
				}
				img[2].fmt = HD_VIDEO_PXLFMT_YUV420;
				img[2].ratio.w = ImgLinkRecInfo[idx][0].disp_ratio.w;
				img[2].ratio.h = ImgLinkRecInfo[idx][0].disp_ratio.h;
			}
		}
		// force set disp fps if disp fps > max(rec, clone)
		if (img[2].fps > max(img[0].fps, img[1].fps)) {
			img[2].fps = max(img[0].fps, img[1].fps);
		}
	}
	// wifi
	if (MaxLinkInfo.MaxWifiLink) {
		img[3].size.w = WifiLinkStrmInfo[0].size.w;
		img[3].size.h = WifiLinkStrmInfo[0].size.h;
		if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
			img[3].fps = IPLUserSize[idx].fps;
		} else {
			img[3].fps = WifiLinkStrmInfo[0].frame_rate;
		}
		img[3].fmt = HD_VIDEO_PXLFMT_YUV420;
		img[3].ratio.w = 16;
		img[3].ratio.h = 9;
		// force set wifi fps if wifi fps > max(rec, clone)
		if (img[3].fps > max(img[0].fps, img[1].fps)) {
			img[3].fps = max(img[0].fps, img[1].fps);
		}
	}
	// alg
	if (ImgLinkAlgInfo[idx].path13.ImgSize.w && ImgLinkAlgInfo[idx].path13.ImgSize.h) {
		img[4].size.w = ImgLinkAlgInfo[idx].path13.ImgSize.w;
		img[4].size.h = ImgLinkAlgInfo[idx].path13.ImgSize.h;
		if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
			img[4].fps = IPLUserSize[idx].fps;
		} else {
			img[4].fps = (ImgLinkAlgInfo[idx].path13.ImgFps) ? ImgLinkAlgInfo[idx].path13.ImgFps : 30;
		}
		img[4].fmt = ImgLinkAlgInfo[idx].path13.ImgFmt;
		img[4].ratio.w = 16;
		img[4].ratio.h = 9;
		// force set alg fps if alg fps > max(rec, clone)
		if (img[4].fps > max(img[0].fps, img[1].fps)) {
			img[4].fps = max(img[0].fps, img[1].fps);
		}
	} else {
		img[4].size.w = 0;
		img[4].size.h = 0;
		img[4].fps = 0;
		img[4].fmt = 0;
		img[4].ratio.w = 0;
		img[4].ratio.h = 0;
	}
	ImageApp_MovieMulti_DUMP("id%d:\r\n", i);
	ImageApp_MovieMulti_DUMP("rec1 = %d/%d@%d(%d)\r\n", img[0].size.w, img[0].size.h, ImgLinkRecInfo[idx][0].frame_rate, img[0].fps);
	ImageApp_MovieMulti_DUMP("rec2 = %d/%d@%d(%d)\r\n", img[1].size.w, img[1].size.h, ImgLinkRecInfo[idx][1].frame_rate, img[1].fps);
	ImageApp_MovieMulti_DUMP("disp = %d/%d@%d(%d)\r\n", img[2].size.w, img[2].size.h, g_DispFps[0], img[2].fps);
	ImageApp_MovieMulti_DUMP("wifi = %d/%d@%d(%d)\r\n", img[3].size.w, img[3].size.h, WifiLinkStrmInfo[0].frame_rate, img[3].fps);
	ImageApp_MovieMulti_DUMP("alg  = %d/%d@%d(%d) (AlgEn = %d)\r\n", img[4].size.w, img[4].size.h, ImgLinkAlgInfo[idx].path13.ImgFps, img[4].fps, gImgLinkAlgFuncEn[idx]);

	_Switch_Auto_Begin(&imgtb);
	imgtb.OUT[0].w = img[0].size.w;
	imgtb.OUT[0].h = img[0].size.h;
	imgtb.OUT[0].frc = img[0].fps;
	imgtb.OUT[0].fmt = img[0].fmt;
	imgtb.OUT[1].w = img[1].size.w;
	imgtb.OUT[1].h = img[1].size.h;
	imgtb.OUT[1].frc = img[1].fps;
	imgtb.OUT[1].fmt = img[1].fmt;
	imgtb.OUT[2].w = img[2].size.w;
	imgtb.OUT[2].h = img[2].size.h;
	imgtb.OUT[2].frc = img[2].fps;
	imgtb.OUT[2].fmt = img[2].fmt;
	imgtb.OUT[3].w = img[3].size.w;
	imgtb.OUT[3].h = img[3].size.h;
	imgtb.OUT[3].frc = img[3].fps;
	imgtb.OUT[3].fmt = img[3].fmt;
	imgtb.OUT[4].w = img[4].size.w;
	imgtb.OUT[4].h = img[4].size.h;
	imgtb.OUT[4].frc = img[4].fps;
	imgtb.OUT[4].fmt = img[4].fmt;
	_Switch_Auto_End(&imgtb);

	gImeMaxOutPath[idx] = imgtb.AutoInNum;

	if (gImeMaxOutPath[idx] <= 4) {
		ImageApp_MovieMulti_DUMP("SWITCH: total %d result\r\n", gImeMaxOutPath[idx]);
		if (img_vproc_no_ext[idx] && gImeMaxOutPath[idx] == 4) {
			DBG_FATAL("Result should <=3 if enable MOVIEMULTI_PARAM_VPRC_NO_EXT\r\n");
			vos_util_delay_ms(100);         // add delay to show dbg_fatal message in linux
			vos_debug_halt();
		}
	} else {
		DBG_FATAL("SWITCH: total %d result (over max = 4)\r\n", gImeMaxOutPath[idx]);
		vos_util_delay_ms(100);         // add delay to show dbg_fatal message in linux
		vos_debug_halt();
	}

	//get result of iport spec, and set spec to its src port
	memset(g_ImeSetting, 0, sizeof(g_ImeSetting));
	for (j = 0; j < gImeMaxOutPath[idx] && j < 4; j++) {
		g_ImeSetting[idx][j].size.w = imgtb.IN[j].w;
		g_ImeSetting[idx][j].size.h = imgtb.IN[j].h;
		g_ImeSetting[idx][j].fps = imgtb.IN[j].frc;
		g_ImeSetting[idx][j].fmt = imgtb.IN[j].fmt;
		g_ImeSetting[idx][j].ratio.w = 16;
		g_ImeSetting[idx][j].ratio.h = 9;
		ImageApp_MovieMulti_DUMP("SWITCH: in[%d] %d x %d . %x. %d\r\n", j, imgtb.IN[j].w, imgtb.IN[j].h, imgtb.IN[j].fmt, imgtb.IN[j].frc);
	}

	// save gSwitchLink[] result, if port is not used, the value will be UNUSED
	for (j = 0; j < 6; j++) {
		gSwitchLink[idx][j] = _Switch_Auto_GetConnect(&imgtb, j);
	}
	ImageApp_MovieMulti_DUMP("gSwitchLink[%d][]=%d/%d/%d/%d/%d/%d\r\n", i, gSwitchLink[idx][0], gSwitchLink[idx][1], gSwitchLink[idx][2], gSwitchLink[idx][3], gSwitchLink[idx][4], gSwitchLink[idx][5]);

	return E_OK;
}

ER _ImageApp_MovieMulti_ImgLinkCfg(MOVIE_CFG_REC_ID id, UINT32 submit)
{
	UINT32 i = id, idx, j, vprc_id;

	if (i >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_REC_ID_1;

	// video capture
	ImgLink[idx].vcap_ctrl        = HD_VIDEOCAP_CTRL(img_sensor_mapping[idx]);
	ImgLink[idx].vcap_i[0]        = HD_VIDEOCAP_IN (img_sensor_mapping[idx], 0);
	ImgLink[idx].vcap_o[0]        = HD_VIDEOCAP_OUT(img_sensor_mapping[idx], 0);
	// video process
	for (j = 0; j < VPROC_PER_LINK; j++) {
		if (submit) {
			vprc_id = (j == 0) ? img_sensor_mapping[idx] : _ImageApp_MovieMulti_GetFreeVprc();
		} else {
			vprc_id = img_sensor_mapping[idx] + MaxLinkInfo.MaxImgLink * j;
		}
		ImgLink[idx].vproc_ctrl[j]       = HD_VIDEOPROC_CTRL(vprc_id);
		ImgLink[idx].vproc_i[j][0]       = HD_VIDEOPROC_IN (vprc_id, 0);
		ImgLink[idx].vproc_o_phy[j][0]   = HD_VIDEOPROC_OUT(vprc_id, 0);
		ImgLink[idx].vproc_o_phy[j][1]   = HD_VIDEOPROC_OUT(vprc_id, 1);
		ImgLink[idx].vproc_o_phy[j][2]   = HD_VIDEOPROC_OUT(vprc_id, 2);
		ImgLink[idx].vproc_o_phy[j][3]   = HD_VIDEOPROC_OUT(vprc_id, 3);
		ImgLink[idx].vproc_o_phy[j][4]   = HD_VIDEOPROC_OUT(vprc_id, 4);    // for 3dnr reference only
		ImgLink[idx].vproc_o[j][0]       = HD_VIDEOPROC_OUT(vprc_id, 5);    // main
		ImgLink[idx].vproc_o[j][1]       = HD_VIDEOPROC_OUT(vprc_id, 6);    // clone
		ImgLink[idx].vproc_o[j][2]       = HD_VIDEOPROC_OUT(vprc_id, 7);    // disp
		ImgLink[idx].vproc_o[j][3]       = HD_VIDEOPROC_OUT(vprc_id, 8);    // wifi
		ImgLink[idx].vproc_o[j][4]       = HD_VIDEOPROC_OUT(vprc_id, 9);    // alg
		ImgLink[idx].vproc_o[j][5]       = HD_VIDEOPROC_OUT(vprc_id, 10);   // disp2 (N/A)
		ImgLink[idx].vproc_o[j][6]       = HD_VIDEOPROC_OUT(vprc_id, 11);   // alg (y only)
	}
	// video encode
	ImgLink[idx].venc_i[0]        = HD_VIDEOENC_IN (0, 0 + idx * 2);
	ImgLink[idx].venc_o[0]        = HD_VIDEOENC_OUT(0, 0 + idx * 2);
	ImgLink[idx].venc_i[1]        = HD_VIDEOENC_IN (0, 1 + idx * 2);
	ImgLink[idx].venc_o[1]        = HD_VIDEOENC_OUT(0, 1 + idx * 2);
	// bsmux
	if (img_bsmux_2v1a_mode[idx] == 0) {                         // normal mode
		ImgLink[idx].bsmux_i[0]       = HD_BSMUX_IN (0, 0 + idx * 4);
		ImgLink[idx].bsmux_o[0]       = HD_BSMUX_OUT(0, 0 + idx * 4);
		ImgLink[idx].bsmux_i[1]       = HD_BSMUX_IN (0, 1 + idx * 4);
		ImgLink[idx].bsmux_o[1]       = HD_BSMUX_OUT(0, 1 + idx * 4);
		ImgLink[idx].bsmux_i[2]       = HD_BSMUX_IN (0, 2 + idx * 4);
		ImgLink[idx].bsmux_o[2]       = HD_BSMUX_OUT(0, 2 + idx * 4);
		ImgLink[idx].bsmux_i[3]       = HD_BSMUX_IN (0, 3 + idx * 4);
		ImgLink[idx].bsmux_o[3]       = HD_BSMUX_OUT(0, 3 + idx * 4);
	} else {                                                     // 2v1a mode
		ImgLink[idx].bsmux_i[0]       = HD_BSMUX_IN (0, 0 + idx * 4);
		ImgLink[idx].bsmux_o[0]       = HD_BSMUX_OUT(0, 0 + idx * 4);
		ImgLink[idx].bsmux_i[1]       = HD_BSMUX_IN (0, 1 + idx * 4);
		ImgLink[idx].bsmux_o[1]       = HD_BSMUX_OUT(0, 1 + idx * 4);
		ImgLink[idx].bsmux_i[2]       = HD_BSMUX_IN (0, 2 + idx * 4);
		ImgLink[idx].bsmux_o[2]       = HD_BSMUX_OUT(0, 0 + idx * 4);
		ImgLink[idx].bsmux_i[3]       = HD_BSMUX_IN (0, 3 + idx * 4);
		ImgLink[idx].bsmux_o[3]       = HD_BSMUX_OUT(0, 1 + idx * 4);
	}
	// fileout
	ImgLink[idx].fileout_i[0]     = HD_FILEOUT_IN (0, 0 + idx * 4);
	ImgLink[idx].fileout_o[0]     = HD_FILEOUT_OUT(0, 0 + idx * 4);
	ImgLink[idx].fileout_i[1]     = HD_FILEOUT_IN (0, 1 + idx * 4);
	ImgLink[idx].fileout_o[1]     = HD_FILEOUT_OUT(0, 1 + idx * 4);
	ImgLink[idx].fileout_i[2]     = HD_FILEOUT_IN (0, 2 + idx * 4);
	ImgLink[idx].fileout_o[2]     = HD_FILEOUT_OUT(0, 2 + idx * 4);
	ImgLink[idx].fileout_i[3]     = HD_FILEOUT_IN (0, 3 + idx * 4);
	ImgLink[idx].fileout_o[3]     = HD_FILEOUT_OUT(0, 3 + idx * 4);

	return E_OK;
}

static ER _ImageApp_MovieMulti_SetSrcOutBuf(UINT32 idx, UINT32 tbl)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	VENDOR_VIDEOENC_COMM_SRCOUT comm_srcout = {0};
	UINT32 w, h;
	UINT32 pa, blk_size;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id = dram_cfg.video_encode;

	if ((img_venc_imgcap_yuvsrc[idx][tbl] == MOVIE_IMGCAP_YUVSRC_SOUT) || (img_vproc_yuv_compress[idx] && (tbl == 0) && (img_venc_sout_type[idx][tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
		if (ImgLinkRecInfo[idx][tbl].codec == _CFG_CODEC_H264) {
			w = ALIGN_CEIL_16(ImgLinkRecInfo[idx][tbl].size.w);
			h = ALIGN_CEIL_16(ImgLinkRecInfo[idx][tbl].size.h);
		} else {
			w = ALIGN_CEIL_64(ImgLinkRecInfo[idx][tbl].size.w);
			h = ALIGN_CEIL_64(ImgLinkRecInfo[idx][tbl].size.h);
		}
		blk_size = w * h * 3 / 2;
		if (img_venc_sout_type[idx][tbl] == MOVIE_IMGCAP_SOUT_PRIVATE) {
			if ((hd_ret = hd_common_mem_alloc("soutbuf_i", &pa, (void **)&va, blk_size, ddr_id)) != HD_OK) {
				ret = E_SYS;
				DBG_ERR("Alloc source out buffer fail(%d)! codec=%d, w=%d(%d), h=%d(%d), wanted size=%x\r\n", hd_ret, ImgLinkRecInfo[idx][tbl].codec, ImgLinkRecInfo[idx][tbl].size.w, w, ImgLinkRecInfo[idx][tbl].size.h, h, blk_size);
				img_venc_sout_va[idx][tbl] = 0;
				img_venc_sout_pa[idx][tbl] = 0;
				img_venc_sout_size[idx][tbl] = 0;
				return ret;
			} else {
				img_venc_sout_va[idx][tbl] = (UINT32)va;
				img_venc_sout_pa[idx][tbl] = (UINT32)pa;
				img_venc_sout_size[idx][tbl] = blk_size;
			}
			DBG_DUMP("Alloc source out buffer OK! codec=%d, w=%d(%d), h=%d(%d), wanted size=%x\r\n", ImgLinkRecInfo[idx][tbl].codec, ImgLinkRecInfo[idx][tbl].size.w, w, ImgLinkRecInfo[idx][tbl].size.h, h, blk_size);
		} else if (img_venc_sout_type[idx][tbl] == MOVIE_IMGCAP_SOUT_COMMON) {
			if ((img_venc_sout_blk[idx][tbl] = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id)) == HD_COMMON_MEM_VB_INVALID_BLK) {
				ret = E_SYS;
				DBG_ERR("Get source out buffer fail(%d)! codec=%d, w=%d(%d), h=%d(%d), wanted size=%x\r\n", img_venc_sout_blk[idx][tbl], ImgLinkRecInfo[idx][tbl].codec, ImgLinkRecInfo[idx][tbl].size.w, w, ImgLinkRecInfo[idx][tbl].size.h, h, blk_size);
				img_venc_sout_blk[idx][tbl] = 0;
				return ret;
			}
			if ((pa = hd_common_mem_blk2pa(img_venc_sout_blk[idx][tbl])) == 0) {
				ret = E_SYS;
				DBG_ERR("hd_common_mem_blk2pa fail\r\n");
				if ((hd_ret = hd_common_mem_release_block(img_venc_sout_blk[idx][tbl])) != HD_OK) {
					DBG_ERR("hd_common_mem_release_block fail(%d)\r\n", hd_ret);
				}
				img_venc_sout_blk[idx][tbl] = 0;
				return ret;
			} else {
				img_venc_sout_pa[idx][tbl] = (UINT32)pa;
				img_venc_sout_size[idx][tbl] = blk_size;
			}
			DBG_DUMP("Get source out buffer OK(%x)! codec=%d, w=%d(%d), h=%d(%d), wanted size=%x\r\n", img_venc_sout_blk[idx][tbl], ImgLinkRecInfo[idx][tbl].codec, ImgLinkRecInfo[idx][tbl].size.w, w, ImgLinkRecInfo[idx][tbl].size.h, h, blk_size);
		} else if (img_venc_sout_type[idx][tbl] == MOVIE_IMGCAP_SOUT_SHARED_PRIVATE) {
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
		if ((hd_ret = vendor_videoenc_set(ImgLink[idx].venc_p[tbl], VENDOR_VIDEOENC_PARAM_OUT_COMM_SRCOUT, &comm_srcout)) != HD_OK) {
			ret = E_SYS;
			DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_COMM_SRCOUT) failed(%d)\r\n", hd_ret);
		}
		img_venc_path_cfg[idx][tbl].max_mem.source_output = 1;
		img_venc_out_param[idx][tbl].h26x.source_output = 1;
	} else {
		img_venc_path_cfg[idx][tbl].max_mem.source_output = 0;
		img_venc_out_param[idx][tbl].h26x.source_output = 0;
	}
	return ret;
}

static ER _ImageApp_MovieMulti_ReleaseSrcOutBuf(UINT32 idx, UINT32 tbl)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if ((img_venc_imgcap_yuvsrc[idx][tbl] == MOVIE_IMGCAP_YUVSRC_SOUT) || (img_vproc_yuv_compress[idx] && (tbl == 0) && (img_venc_sout_type[idx][tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
		if (img_venc_sout_type[idx][tbl] == MOVIE_IMGCAP_SOUT_PRIVATE) {
			if (img_venc_sout_va[idx][tbl]) {
				if ((hd_ret = hd_common_mem_free(img_venc_sout_pa[idx][tbl], (void *)img_venc_sout_va[idx][tbl])) != HD_OK) {
					DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
					ret = E_SYS;
				}
				img_venc_sout_va[idx][tbl] = 0;
				img_venc_sout_pa[idx][tbl] = 0;
				img_venc_sout_size[idx][tbl] = 0;
			}
		} else if (img_venc_sout_type[idx][tbl] == MOVIE_IMGCAP_SOUT_COMMON) {
			if (img_venc_sout_blk[idx][tbl] && img_venc_sout_blk[idx][tbl] != HD_COMMON_MEM_VB_INVALID_BLK) {
				if ((hd_ret = hd_common_mem_release_block(img_venc_sout_blk[idx][tbl])) != HD_OK) {
					DBG_ERR("hd_common_mem_release_block(%d) failed(%d)\r\n", img_venc_sout_blk[idx][tbl], hd_ret);
					ret = E_SYS;
				}
				img_venc_sout_blk[idx][tbl] = 0;
			}
		} else if (img_venc_sout_type[idx][tbl] == MOVIE_IMGCAP_SOUT_SHARED_PRIVATE) {
			_ImageApp_MovieMulti_FreeSharedSrcOutBuf();
		}
	}
	return ret;
}

ER _ImageApp_MovieMulti_ImgSetVEncParam(UINT32 idx, UINT32 tbl)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
	VENDOR_VIDEOENC_H26X_XVR_CFG h26x_xvr = {
		.xvr_app = img_venc_xvr_app[idx][tbl],
	};
	if ((hd_ret = vendor_videoenc_set(ImgLink[idx].venc_p[tbl], VENDOR_VIDEOENC_PARAM_OUT_H26X_XVR, &h26x_xvr)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_H26X_XVR) fail(%d)\n", hd_ret);
	}
#endif

	HD_VIDEOENC_FUNC_CONFIG img_venc_func_cfg = {
		.ddr_id  = dram_cfg.video_encode,
		.in_func = 0,
	};
	IACOMM_VENC_CFG venc_cfg = {
		.video_enc_path = ImgLink[idx].venc_p[tbl],
		.ppath_cfg      = &(img_venc_path_cfg[idx][tbl]),
		.pfunc_cfg      = &(img_venc_func_cfg),
	};

	_ImageApp_MovieMulti_SetSrcOutBuf(idx, tbl);

	if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
		DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	IACOMM_VENC_PARAM venc_param = {
		.video_enc_path = ImgLink[idx].venc_p[tbl],
		.pin            = &(img_venc_in_param[idx][tbl]),
		.pout           = &(img_venc_out_param[idx][tbl]),
		.prc            = &(img_venc_rc_param[idx][tbl]),
	};
	if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
		DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	HD_H26XENC_VUI vui_param = {
		.vui_en                     = img_venc_vui_disable[idx][tbl] ? FALSE : TRUE,            ///< enable vui. default: 0, range: 0~1 (0: disable, 1: enable)
		.sar_width                  = 0,    /// 0: Unspecified                                  ///< Horizontal size of the sample aspect ratio. default: 0, range: 0~65535
		.sar_height                 = 0,    /// 0: Unspecified                                  ///< Vertical size of the sample aspect rat. default: 0, range: 0~65535
		.matrix_coef                = 1,    /// 1: bt.709                                       ///< Matrix coefficients are used to derive the luma and Chroma signals from green, blue, and red primaries. default: 2, range: 0~255
		.transfer_characteristics   = 1,    /// 1: bt.709                                       ///< The opto-electronic transfers characteristic of the source pictures. default: 2, range: 0~255
		.colour_primaries           = 1,    /// 1: bt.709                                       ///< Chromaticity coordinates the source primaries. default: 2, range: 0~255
		.video_format               = 5,    /// 5: Unspecified video format                     ///< Indicate the representation of pictures. default: 5, range: 0~7
		.color_range                = img_venc_vui_color_tv_range[idx][tbl] ? FALSE : TRUE,     ///< Indicate the black level and range of the luma and Chroma signals. default: 0, range: 0~1 (0: Not full range, 1: Full range)
		.timing_present_flag        = 1,    /// 1: enabled                                      ///< timing info present flag. default: 0, range: 0~1 (0: disable, 1: enable)
	};

	if (vui_param.vui_en && g_ia_moviemulti_usercb) {
		UINT32 vui_en = vui_param.vui_en;
		UINT32 color_range = vui_param.color_range;
		UINT32 id = ((tbl == MOVIETYPE_MAIN) ? _CFG_REC_ID_1 : _CFG_CLONE_ID_1) + idx;
		g_ia_moviemulti_usercb(id, MOVIE_USER_CB_EVENT_SET_ENC_VUI, (UINT32)&vui_param);
		if ((vui_en != vui_param.vui_en) || color_range != vui_param.color_range) {
			DBG_WRN("id%d: vui_en(%d/%d) or color_range(%d/%d) has been changed\r\n", id, vui_en, vui_param.vui_en, color_range, vui_param.color_range);
		}
	}

	if ((hd_ret = hd_videoenc_set(ImgLink[idx].venc_p[tbl], HD_VIDEOENC_PARAM_OUT_VUI, &vui_param)) != HD_OK) {
		DBG_ERR("hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_VUI) fail(%d)\n", hd_ret);
	}

	VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_size = {0};
	bs_reserved_size.reserved_size = (ImgLinkRecInfo[idx][tbl].file_format == _CFG_FILE_FORMAT_TS) ? VENC_BS_RESERVED_SIZE_TS : VENC_BS_RESERVED_SIZE_MP4;
	if ((hd_ret = vendor_videoenc_set(ImgLink[idx].venc_p[tbl], VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_size)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	VENDOR_VIDEOENC_TIMELAPSE_TIME_CFG timelapse_cfg = {0};
	timelapse_cfg.timelapse_time = (ImgLinkRecInfo[idx][tbl].rec_mode == _CFG_REC_TYPE_TIMELAPSE) ? ImgLinkTimelapseTime[idx][tbl] : 0;
	if ((hd_ret = vendor_videoenc_set(ImgLink[idx].venc_p[tbl], VENDOR_VIDEOENC_PARAM_OUT_TIMELAPSE_TIME, &timelapse_cfg)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_TIMELAPSE_TIME) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	VENDOR_VIDEOENC_LONG_START_CODE lsc = {0};
	lsc.long_start_code_en = ENABLE;
	if ((hd_ret = vendor_videoenc_set(ImgLink[idx].venc_p[tbl], VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE, &lsc)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	VENDOR_VIDEOENC_QUALITY_BASE_MODE qbm = {0};
	qbm.quality_base_en = img_venc_quality_base_mode_en[idx][tbl];
	if ((hd_ret = vendor_videoenc_set(ImgLink[idx].venc_p[tbl], VENDOR_VIDEOENC_PARAM_OUT_QUALITY_BASE, &qbm)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_QUALITY_BASE) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
	VENDOR_VIDEOENC_H26X_LOW_POWER_CFG h26x_low_power = {0};
	h26x_low_power.b_enable = img_venc_low_power_mode[idx][tbl];
	if ((hd_ret = vendor_videoenc_set(ImgLink[idx].venc_p[tbl], VENDOR_VIDEOENC_PARAM_OUT_H26X_LOW_POWER, &h26x_low_power)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_H26X_LOW_POWER) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
#endif

	return ret;
}

ER _ImageApp_MovieMulti_ImgReopenVEnc(UINT32 idx, UINT32 tbl)
{
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if (ImgLinkStatus[idx].venc_p[tbl] == 0) {
		_ImageApp_MovieMulti_ReleaseSrcOutBuf(idx, tbl);
		if ((hd_ret = venc_close_ch(ImgLink[idx].venc_p[tbl])) != HD_OK) {
			DBG_ERR("venc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = venc_open_ch(ImgLink[idx].venc_i[tbl], ImgLink[idx].venc_o[tbl], &(ImgLink[idx].venc_p[tbl]))) != HD_OK) {
			DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		_ImageApp_MovieMulti_ImgSetVEncParam(idx, tbl);
		if (img_venc_bs_buf_va[idx][tbl]) {
			hd_common_mem_munmap((void *)img_venc_bs_buf_va[idx][tbl], img_venc_bs_buf[idx][tbl].buf_info.buf_size);
			img_venc_bs_buf_va[idx][tbl] = 0;
		}
		img_venc_bs_buf[idx][tbl].buf_info.phy_addr = 0;
		img_venc_bs_buf[idx][tbl].buf_info.buf_size = 0;
	} else {
		ret = E_SYS;
	}
	return ret;
}

static ER _ImageApp_MovieMulti_ImgLink(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j, k;
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 max_f_idx;
	HD_DIM sz = {0}, sz2 = {0};
	UINT32 iport[4], temp[4];

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_REC_ID_1;

	_ImageApp_MovieMulti_PreCfgSwitch(i);

	for (j = 0; j < 4; j++) {
		gUserProcMap[idx][j] = UNUSED;
	}
	if (gImeMaxOutPath[idx] < 4) {
		for (j = 0;  j < gImeMaxOutPath[idx]; j++) {
			gUserProcMap[idx][j] = j;
		}
	} else {
		for (j = 0; j < 4; j++) {
			temp[j] = j;
		}
		if ((gSwitchLink[idx][0] == gSwitchLink[idx][2]) || (MaxLinkInfo.MaxDispLink == 0)) {
			iport[0] = gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN];
			temp[iport[0]] = UNUSED;
			k = 1;
		} else {
			iport[0] = gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN];
			iport[1] = gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP];
			temp[iport[0]] = UNUSED;
			temp[iport[1]] = UNUSED;
			k = 2;
		}
		for (j = 0; j < 4; j++) {
			if(temp[j] != UNUSED) {
				iport[k] = temp[j];
				k++;
			}
		}
		for (j = 0; j < 4; j++) {
			gUserProcMap[idx][iport[j]] = j;
		}
	}
	ImageApp_MovieMulti_DUMP("gUserProcMap[%d][]=%d/%d/%d/%d\r\n", i, gUserProcMap[idx][0], gUserProcMap[idx][1], gUserProcMap[idx][2], gUserProcMap[idx][3]);

	// set videocap
	// disable vcap function if pxlfmt is yuv
	if (HD_VIDEO_PXLFMT_CLASS(img_vcap_capout_pxlfmt[idx]) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
		img_vcap_ctrl[idx].func = 0;
		if (img_vcap_patgen[idx]) {
			img_vcap_patgen[idx] = 0;
			DBG_WRN("vcap pat_gen is not supported in yuv mode\r\n");
		}
	} else {                                                                                // RAW
		img_vcap_ctrl[idx].func = ImgLinkRecInfo[idx][0].vcap_func;
	}

	if (img_vcap_cfg[idx].sen_cfg.shdr_map && !(img_vcap_ctrl[idx].func & HD_VIDEOCAP_FUNC_SHDR)) {
		DBG_WRN("id[%d] HD_VIDEOCAP_FUNC_SHDR is not enabled but shdr_map is set(0x%x)\r\n", idx, img_vcap_cfg[idx].sen_cfg.shdr_map);
		img_vcap_cfg[idx].sen_cfg.shdr_map = 0;
	}

	// update driver name if use pat_gen
	if (img_vcap_patgen[idx]) {
		snprintf(img_vcap_cfg[idx].sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
	}

	// AD map
	if (ad_sensor_map[idx].rec_id != UNUSED) {
		if (HD_VIDEO_PXLFMT_CLASS(img_vcap_capout_pxlfmt[idx]) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
			HD_PATH_ID video_cap_ctrl = 0;
			UINT32 chip_id     = (ad_sensor_map[idx].vin_id & 0x0000ff00) >> 8;
			UINT32 vin_id      = (ad_sensor_map[idx].vin_id & 0x000000ff);
			UINT32 vcap_id_bit =  ad_sensor_map[idx].vcap_id_bit;
			UINT32 ad_map      = VENDOR_VIDEOCAP_AD_MAP(chip_id, vin_id, vcap_id_bit);

			hd_videocap_open(0, ImgLink[idx].vcap_ctrl, &video_cap_ctrl);
			vendor_videocap_set(video_cap_ctrl, VENDOR_VIDEOCAP_PARAM_AD_MAP, &ad_map);
			#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
			UINT32 ad_type     = (ad_sensor_map[idx].vin_id & 0x00ff0000) >> 16;
			vendor_videocap_set(video_cap_ctrl, VENDOR_VIDEOCAP_PARAM_AD_TYPE, &ad_type);
			#endif
		}
	}

	IACOMM_VCAP_CFG vcap_cfg = {
		.p_video_cap_ctrl = &(ImgLink[idx].vcap_p_ctrl),
		.ctrl_id          = ImgLink[idx].vcap_ctrl,
		.pcfg             = &(img_vcap_cfg[idx]),
		.pctrl            = &(img_vcap_ctrl[idx]),
	};
	if ((hd_ret = vcap_set_cfg(&vcap_cfg)) != HD_OK) {
		DBG_ERR("vcap_set_cfg fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
#if 0
	// AD map
	if (ad_sensor_map[idx].rec_id != UNUSED) {
		if (HD_VIDEO_PXLFMT_CLASS(img_vcap_capout_pxlfmt[idx]) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
			UINT32 ad_map;
			UINT32 chip_id = 0;
			UINT32 vin_id = 0;
			UINT32 vcap_id_bit;
			vcap_id_bit = ad_sensor_map[idx].vcap_id_bit;
			vin_id = ad_sensor_map[idx].vin_id;

			ad_map = VENDOR_VIDEOCAP_AD_MAP(chip_id, vin_id, vcap_id_bit);
			vendor_videocap_set(*vcap_cfg.p_video_cap_ctrl, VENDOR_VIDEOCAP_PARAM_AD_MAP, &ad_map);
		}
	}
#endif

#if defined(_BSP_NA51089_)
	if (img_vcap_sie_remap[idx] == TRUE) {
		UINT32 sie_map = 1;
		//56x only support compression in SIE2
		if ((hd_ret = vendor_videocap_set(ImgLink[idx].vcap_p_ctrl, VENDOR_VIDEOCAP_PARAM_SIE_MAP, &sie_map)) != HD_OK) {
			DBG_ERR("vendor_videocap_set(VENDOR_VIDEOCAP_PARAM_SIE_MAP) failed(%d)\r\n", hd_ret);
		}
	}
#endif

	if ((hd_ret = vcap_open_ch(ImgLink[idx].vcap_i[0], ImgLink[idx].vcap_o[0], &(ImgLink[idx].vcap_p[0]))) != HD_OK) {
		DBG_ERR("vcap_open_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = vcap_get_caps(ImgLink[idx].vcap_p_ctrl, &(ImgLink[idx].vcap_syscaps))) != HD_OK) {
		DBG_ERR("vcap_get_caps fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// ccir mode
	if (img_vcap_senout_pxlfmt[idx] == HD_VIDEO_PXLFMT_YUV422 || img_vcap_senout_pxlfmt[idx] == HD_VIDEO_PXLFMT_YUV422_ONE) {
		UINT32 det_loop = img_vcap_detect_loop[idx];
		BOOL plugged = FALSE;
		VENDOR_VIDEOCAP_GET_PLUG_INFO plug_info = {0};
		VENDOR_VIDEOCAP_CCIR_INFO ccir_info = {
			.field_sel = VENDOR_VIDEOCAP_FIELD_DISABLE,
			.fmt = img_vcap_ccir_fmt[idx],
			.interlace = 0,
			.mux_data_index = 0,
		};
		do {
			if (det_loop) {
				vos_util_delay_ms(50);
				det_loop--;
			}
			if ((hd_ret = vendor_videocap_get(ImgLink[idx].vcap_p_ctrl, VENDOR_VIDEOCAP_PARAM_GET_PLUG, &plugged)) == HD_OK && plugged == TRUE) {
				if ((hd_ret = vendor_videocap_get(ImgLink[idx].vcap_p_ctrl, VENDOR_VIDEOCAP_PARAM_GET_PLUG_INFO, &plug_info)) == HD_OK) {
					ccir_info.interlace = plug_info.interlace;
					ccir_info.field_sel = plug_info.interlace ? VENDOR_VIDEOCAP_FIELD_EN_0 : VENDOR_VIDEOCAP_FIELD_DISABLE;
					DBG_DUMP("vendor_videocap_get(VENDOR_VIDEOCAP_PARAM_GET_PLUG_INFO): interlace=%d\r\n", ccir_info.interlace);
				} else {
					DBG_DUMP("vendor_videocap_get(VENDOR_VIDEOCAP_PARAM_GET_PLUG_INFO) failed(%d)\r\n", hd_ret);
				}
			} else {
				DBG_DUMP("vendor_videocap_get(VENDOR_VIDEOCAP_PARAM_GET_PLUG)=%d, pluged=%d\r\n", hd_ret, plugged);
			}
		} while (det_loop);

		if (img_vcap_ccir_mode[idx] == CCIR_INTERLACE) {
			ccir_info.interlace = 1;
			ccir_info.field_sel = VENDOR_VIDEOCAP_FIELD_EN_0;
			DBG_DUMP("Forced interlace mode\r\n");
		} else if (img_vcap_ccir_mode[idx] == CCIR_PROGRESSIVE){
			ccir_info.interlace = 0;
			ccir_info.field_sel = VENDOR_VIDEOCAP_FIELD_DISABLE;
			DBG_DUMP("Forced progressive mode\r\n");
		}
		vendor_videocap_set(ImgLink[idx].vcap_p[0], VENDOR_VIDEOCAP_PARAM_CCIR_INFO, &ccir_info);
	}

	// use max frame rate size as sensor input
	max_f_idx = 0;
	for (j = 1; j < gImeMaxOutPath[idx] && j < 4; j++) {
		if (g_ImeSetting[idx][j].fps > g_ImeSetting[idx][max_f_idx].fps) {
			max_f_idx = j;
		}
	}

	// check whether user forced set ipl input size
	if (gImgLinkForcedSizePath[idx] != MOVIE_IPL_SIZE_AUTO) {
		if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_MAIN) {
			j = gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN];
		} else if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_CLONE) {
			j = gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE];
		} else if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_DISP) {
			j = gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP];
		} else if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_WIFI) {
			j = gSwitchLink[idx][IAMOVIE_VPRC_EX_WIFI];
		} else if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_ALG) {
			j = gSwitchLink[idx][IAMOVIE_VPRC_EX_ALG];
		} else if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
			if (!IPLUserSize[idx].size.w || !IPLUserSize[idx].size.h || !IPLUserSize[idx].fps) {
				DBG_WRN("User IPL size not set, using MOVIE_IPL_SIZE_AUTO instead.\r\n");
				gImgLinkForcedSizePath[idx] = MOVIE_IPL_SIZE_AUTO;
			}
			j = max_f_idx;
		}
		if (j < 4) {
			max_f_idx = j;
		}
	}

	// sz for vcap in size, sz2 for vcap out size (vprc in size)
	if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
		sz.w = IPLUserSize[idx].size.w;
		sz.h = IPLUserSize[idx].size.h;
		fps_vcap_out[idx] = IPLUserSize[idx].fps;
	} else {
		sz.w = g_ImeSetting[idx][max_f_idx].size.w;
		sz.h = g_ImeSetting[idx][max_f_idx].size.h;
		fps_vcap_out[idx] = g_ImeSetting[idx][max_f_idx].fps;
	}

	if (img_vcap_out_size[idx].w && img_vcap_out_size[idx].h) {
		sz2.w = img_vcap_out_size[idx].w;
		sz2.h = img_vcap_out_size[idx].h;
	} else if (img_vcap_crop_win[idx].Win.w && img_vcap_crop_win[idx].Win.h) {
		sz2.w = img_vcap_crop_win[idx].Win.w;
		sz2.h = img_vcap_crop_win[idx].Win.h;
	} else if (gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) {
		sz2.w = IPLUserSize[idx].size.w;
		sz2.h = IPLUserSize[idx].size.h;
	} else {
		sz2.w = g_ImeSetting[idx][max_f_idx].size.w;
		sz2.h = g_ImeSetting[idx][max_f_idx].size.h;
	}

#if 0
	if (forced_use_unique_3dnr_path[idx]) {
		DBG_DUMP("forced_use_unique_3dnr_path[%d] is set\r\n", idx);
		use_unique_3dnr_path[idx] = TRUE;
	} else {
		if (img_vcap_out_size[idx].w && img_vcap_out_size[idx].h && (ImgLinkRecInfo[idx][0].vproc_func & HD_VIDEOPROC_FUNC_3DNR)) {
			if ((img_vcap_out_size[idx].w != g_ImeSetting[idx][max_f_idx].size.w) || (img_vcap_out_size[idx].h != g_ImeSetting[idx][max_f_idx].size.h)) {
				use_unique_3dnr_path[idx] = TRUE;
			} else {
				use_unique_3dnr_path[idx] = FALSE;
			}
		} else if ((gImgLinkForcedSizePath[idx] == MOVIE_IPL_SIZE_USER) && (ImgLinkRecInfo[idx][0].vproc_func & HD_VIDEOPROC_FUNC_3DNR)) {
			if ((sz.w != g_ImeSetting[idx][max_f_idx].size.w) || (sz.h != g_ImeSetting[idx][max_f_idx].size.h)) {
				use_unique_3dnr_path[idx] = TRUE;
			} else {
				use_unique_3dnr_path[idx] = FALSE;
			}
		} else {
			use_unique_3dnr_path[idx] = FALSE;
		}
	}
#else
	if (forced_use_unique_3dnr_path[idx]) {
		DBG_DUMP("forced_use_unique_3dnr_path[%d] is set\r\n", idx);
		use_unique_3dnr_path[idx] = TRUE;
	} else if (ImgLinkRecInfo[idx][0].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[idx] == MOVIE_VPE_NONE) {
#else
		if (1) {
#endif
			if ((sz2.w != g_ImeSetting[idx][max_f_idx].size.w) || (sz2.h != g_ImeSetting[idx][max_f_idx].size.h)) {
				use_unique_3dnr_path[idx] = TRUE;
			} else {
				use_unique_3dnr_path[idx] = FALSE;
			}
		} else {
			use_unique_3dnr_path[idx] = FALSE;
		}
	} else {
		use_unique_3dnr_path[idx] = FALSE;
	}
#endif

	// set dram id
	img_vcap_func_cfg[idx].ddr_id = dram_cfg.video_cap[idx];

	if (img_vcap_patgen[idx]) {
		img_vcap_in_param[idx].sen_mode = HD_VIDEOCAP_PATGEN_MODE(img_vcap_patgen[idx], ALIGN_CEIL((sz.w *10/65), 2));  // width value is a tried value
	} else {
		img_vcap_in_param[idx].sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO;
	}

	img_vcap_in_param[idx].frc = _ImageApp_MovieMulti_frc(fps_vcap_out[idx], 1);
	img_vcap_in_param[idx].dim.w = sz.w;
	img_vcap_in_param[idx].dim.h = sz.h;
	img_vcap_in_param[idx].pxlfmt = img_vcap_senout_pxlfmt[idx];

	if (img_vcap_ctrl[idx].func & HD_VIDEOCAP_FUNC_SHDR) {
		temp[0] = img_vcap_cfg[idx].sen_cfg.shdr_map & 0x0000ffff;
		temp[1] = 0;
		while (temp[0]) {
			if (temp[0] & 0x01) {
				temp[1]++;
			}
			temp[0] >>= 1;
		}
		img_vcap_in_param[idx].out_frame_num = temp[1];
		DBG_DUMP("Set id[%d] SHDR to %d frame mode\r\n", idx, img_vcap_in_param[idx].out_frame_num);
	} else {
		img_vcap_in_param[idx].out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
	}

	img_vcap_crop_param[idx].mode = HD_CROP_ON;
	if (img_vcap_crop_win[idx].Win.w && img_vcap_crop_win[idx].Win.h) {
		img_vcap_crop_param[idx].win.coord.w = 0;
		img_vcap_crop_param[idx].win.coord.h = 0;
		img_vcap_crop_param[idx].win.rect.x = img_vcap_crop_win[idx].Win.x;
		img_vcap_crop_param[idx].win.rect.y = img_vcap_crop_win[idx].Win.y;
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
		img_vcap_crop_param[idx].win.rect.w = img_vcap_crop_win[idx].Win.w;
#else
		if (img_vcap_senout_pxlfmt[idx] ==  HD_VIDEO_PXLFMT_YUV422 || img_vcap_senout_pxlfmt[idx] ==  HD_VIDEO_PXLFMT_YUV422_ONE) {
			img_vcap_crop_param[idx].win.rect.w = img_vcap_crop_win[idx].Win.w * 2;
		} else {
			img_vcap_crop_param[idx].win.rect.w = img_vcap_crop_win[idx].Win.w;
		}
#endif
		img_vcap_crop_param[idx].win.rect.h = img_vcap_crop_win[idx].Win.h;
	} else {
		img_vcap_crop_param[idx].win.coord.w = 0;
		img_vcap_crop_param[idx].win.coord.h = 0;
		img_vcap_crop_param[idx].win.rect.x = -1;
		img_vcap_crop_param[idx].win.rect.y = -1;
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
		img_vcap_crop_param[idx].win.rect.w = sz.w;
#else
		if (img_vcap_senout_pxlfmt[idx] ==  HD_VIDEO_PXLFMT_YUV422 || img_vcap_senout_pxlfmt[idx] ==  HD_VIDEO_PXLFMT_YUV422_ONE) {
			img_vcap_crop_param[idx].win.rect.w = sz.w * 2;
		} else {
			img_vcap_crop_param[idx].win.rect.w = sz.w;
		}
#endif
		img_vcap_crop_param[idx].win.rect.h = sz.h;
	}
	img_vcap_out_param[idx].pxlfmt = img_vcap_capout_pxlfmt[idx];
	img_vcap_out_param[idx].dir = img_vcap_capout_dir[idx];

	if (img_vcap_out_size[idx].w && img_vcap_out_size[idx].h) {
		img_vcap_out_param[idx].dim.w = img_vcap_out_size[idx].w;
		img_vcap_out_param[idx].dim.h = img_vcap_out_size[idx].h;
	} else {
		img_vcap_out_param[idx].dim.w = 0;
		img_vcap_out_param[idx].dim.h = 0;
	}
	img_vcap_out_param[idx].depth = (img_manual_push_raw_frame[idx] == FALSE) ? 0 : img_vcap_in_param[idx].out_frame_num;

	IACOMM_VCAP_PARAM vcap_param = {
		.video_cap_path = ImgLink[idx].vcap_p[0],
		.data_lane      = img_vcap_data_lane[idx],
		.pin_param      = &(img_vcap_in_param[idx]),
		.pcrop_param    = &(img_vcap_crop_param[idx]),
		.pout_param     = &(img_vcap_out_param[idx]),
		.pfunc_cfg      = &(img_vcap_func_cfg[idx]),
	};
	if ((hd_ret = vcap_set_param(&vcap_param)) != HD_OK) {
		DBG_ERR("vcap_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
	if (img_vcap_gyro_func[idx]) {
		VENDOR_VIDEOCAP_GYRO_INFO vcap_gyro_info = {0};
		vcap_gyro_info.en = TRUE;
		vcap_gyro_info.data_num = GYRO_DATA_NUM;
		if ((hd_ret = vendor_videocap_set(ImgLink[idx].vcap_p[0], VENDOR_VIDEOCAP_PARAM_GYRO_INFO, &vcap_gyro_info)) != HD_OK) {
			DBG_ERR("vendor_videocap_set(VENDOR_VIDEOCAP_PARAM_GYRO_INFO) fail(%d)\r\n", hd_ret);
		} else {
			DBG_DUMP("vendor_videocap_set(VENDOR_VIDEOCAP_PARAM_GYRO_INFO) OK\r\n");
		}
	}
#endif

	// set videoproc
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	if (img_vproc_vpe_en[idx] == MOVIE_VPE_NONE) {
#endif
		img_vproc_cfg[idx].isp_id = img_sensor_mapping[idx];
#if defined(_BSP_NA51089_)
		if (img_vcap_sie_remap[idx] == TRUE) {
			img_vproc_cfg[idx].isp_id = ISP_ID_REMAP_SIE(img_sensor_mapping[idx], 1);
		}
#endif
		img_vproc_cfg[idx].in_max.func = 0;
		img_vproc_cfg[idx].in_max.dim.w = ImgLink[idx].vcap_syscaps.max_dim.w;
		img_vproc_cfg[idx].in_max.dim.h = ImgLink[idx].vcap_syscaps.max_dim.h;
		img_vproc_cfg[idx].in_max.pxlfmt = img_vcap_capout_pxlfmt[idx];
		img_vproc_cfg[idx].in_max.frc = _ImageApp_MovieMulti_frc(1, 1);

		// config vproc_in_param here to find 3dnr path
		img_vproc_in_param[idx].func = 0;
#if 0
		if (img_vcap_out_size[idx].w && img_vcap_out_size[idx].h) {
				img_vproc_in_param[idx].dim.w = img_vcap_out_size[idx].w;
				img_vproc_in_param[idx].dim.h = img_vcap_out_size[idx].h;
		} else {
			if (img_vcap_crop_win[idx].Win.w && img_vcap_crop_win[idx].Win.h) {
				img_vproc_in_param[idx].dim.w = img_vcap_crop_win[idx].Win.w;
				img_vproc_in_param[idx].dim.h = img_vcap_crop_win[idx].Win.h;
			} else {
				img_vproc_in_param[idx].dim.w = img_vcap_in_param[idx].dim.w;
				img_vproc_in_param[idx].dim.h = img_vcap_in_param[idx].dim.h;
			}
		}
#else
		img_vproc_in_param[idx].dim.w = sz2.w;
		img_vproc_in_param[idx].dim.h = sz2.h;
#endif
		img_vproc_in_param[idx].pxlfmt = img_vcap_capout_pxlfmt[idx];
		//img_vproc_in_param[idx].dir = HD_VIDEO_DIR_NONE;
		img_vproc_in_param[idx].frc = _ImageApp_MovieMulti_frc(1, 1);
		if ((ImgLinkRecInfo[idx][0].vproc_func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE)) {
			if (gImeMaxOutPath[idx] < 4) {
				for (j = 0; j < gImeMaxOutPath[idx]; j++) {
					if ((img_vcap_in_param[idx].dim.w == g_ImeSetting[idx][j].size.w) && (img_vcap_in_param[idx].dim.h == g_ImeSetting[idx][j].size.h)) {
						img_vproc_3dnr_path[idx] = j;
					}
				}
			} else {
				for (j = 0; j < 3; j++) {
					if ((img_vcap_in_param[idx].dim.w == g_ImeSetting[idx][iport[j]].size.w) && (img_vcap_in_param[idx].dim.h == g_ImeSetting[idx][iport[j]].size.h)) {
						img_vproc_3dnr_path[idx] = j;
					}
				}
			}
		}

		if (HD_VIDEO_PXLFMT_CLASS(img_vcap_capout_pxlfmt[idx]) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
			// sensor 0 direct mode  , sensor 1, 2, 3 should use PIPE ISE mode.
			if ((img_vcap_func_cfg[0].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) && (idx != 0)) {
#if !defined(_BSP_NA51089_)
				img_vproc_cfg[idx].pipe = (img_vproc_forced_use_pipe[idx] == HD_VIDEOPROC_PIPE_VPE) ? HD_VIDEOPROC_PIPE_VPE : HD_VIDEOPROC_PIPE_YUVAUX;
#else
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_YUVAUX;
#endif
				img_vproc_cfg[idx].ctrl_max.func = 0;
				img_vproc_ctrl[idx].func = 0;
			} else if (img_vproc_forced_use_pipe[idx] == HD_VIDEOPROC_PIPE_YUVAUX) {
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_YUVAUX;
				img_vproc_cfg[idx].ctrl_max.func = 0;
				img_vproc_ctrl[idx].func = 0;
#if !defined(_BSP_NA51089_)
			} else if (img_vproc_forced_use_pipe[idx] == HD_VIDEOPROC_PIPE_VPE) {
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_VPE;
				img_vproc_cfg[idx].ctrl_max.func = 0;
				img_vproc_ctrl[idx].func = 0;
#endif
			} else if (img_vproc_forced_use_pipe[idx] == HD_VIDEOPROC_PIPE_SCALE) {
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_SCALE;
				img_vproc_cfg[idx].ctrl_max.func &= (HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA);
				img_vproc_ctrl[idx].func = 0;
			} else {
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_YUVALL | img_vproc_dis_func[idx];
				img_vproc_cfg[idx].ctrl_max.func = ImgLinkRecInfo[idx][0].vproc_func;
				img_vproc_ctrl[idx].func = ImgLinkRecInfo[idx][0].vproc_func;
				if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
					img_vproc_ctrl[idx].ref_path_3dnr = (use_unique_3dnr_path[idx] == TRUE) ? ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][4] : ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][img_vproc_3dnr_path[idx]];
				}
			}
		} else {                                                                                // RAW
			img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_RAWALL | img_vproc_dis_func[idx];
			img_vproc_cfg[idx].ctrl_max.func = ImgLinkRecInfo[idx][0].vproc_func;
			img_vproc_ctrl[idx].func = ImgLinkRecInfo[idx][0].vproc_func;
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				img_vproc_ctrl[idx].ref_path_3dnr = (use_unique_3dnr_path[idx] == TRUE) ? ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][4] : ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][img_vproc_3dnr_path[idx]];
			}
		}

		// set dram id
		img_vproc_func_cfg[idx].ddr_id = dram_cfg.video_proc[idx];
		img_vproc_func_cfg[idx].in_func = 0;
		if (img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
				img_vproc_func_cfg[idx].in_func |= HD_VIDEOPROC_INFUNC_DIRECT;
		} else if (img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_ONEBUF) {
				img_vproc_func_cfg[idx].in_func |= HD_VIDEOPROC_INFUNC_ONEBUF;
		}
		IACOMM_VPROC_CFG vproc_cfg = {
			.p_video_proc_ctrl = &(ImgLink[idx].vproc_p_ctrl[img_vproc_splitter[idx]]),
			.ctrl_id           = ImgLink[idx].vproc_ctrl[img_vproc_splitter[idx]],
			.pcfg              = &(img_vproc_cfg[idx]),
			.pctrl             = &(img_vproc_ctrl[idx]),
			.pfunc             = &(img_vproc_func_cfg[idx]),
		};
		if ((hd_ret = vproc_set_cfg(&vproc_cfg)) != HD_OK) {
			DBG_ERR("vproc_set_cfg fail(%d)\n", hd_ret);
			ret = E_SYS;
		}

		/*
		// move to vprc session beginning
		img_vproc_in_param[idx].func = 0;
		if (img_vcap_crop_win[idx].Win.w && img_vcap_crop_win[idx].Win.h) {
			img_vproc_in_param[idx].dim.w = img_vcap_crop_win[idx].Win.w;
			img_vproc_in_param[idx].dim.h = img_vcap_crop_win[idx].Win.h;
		} else {
			img_vproc_in_param[idx].dim.w = img_vcap_in_param[idx].dim.w;
			img_vproc_in_param[idx].dim.h = img_vcap_in_param[idx].dim.h;
		}
		img_vproc_in_param[idx].pxlfmt = img_vcap_capout_pxlfmt[idx];
		//img_vproc_in_param[idx].dir = HD_VIDEO_DIR_NONE;
		img_vproc_in_param[idx].frc = _ImageApp_MovieMulti_frc(1, 1);
		*/

		{
			UINT32 in_depth = img_vproc_queue_depth[idx] ? img_vproc_queue_depth[idx] : 2;
			if ((hd_ret = vendor_videoproc_set(ImgLink[idx].vproc_p_ctrl[img_vproc_splitter[idx]], VENDOR_VIDEOPROC_PARAM_IN_DEPTH, &in_depth)) != HD_OK) {
				DBG_ERR("vendor_videoproc_set(VENDOR_VIDEOPROC_PARAM_IN_DEPTH, %d) fail(%d)\r\n", in_depth, hd_ret);
			}
		}

		IACOMM_VPROC_PARAM_IN vproc_param_in = {
			.video_proc_path = ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0],
			.in_param        = &(img_vproc_in_param[idx]),
		};
		if ((hd_ret = vproc_set_param_in(&vproc_param_in)) != HD_OK) {
			DBG_ERR("vproc_set_param_in fail(%d)\n", hd_ret);
			ret = E_SYS;
		}

		if ((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == TRUE)) {
			// config 3dnr path
			HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
			vproc_func_param.ddr_id = dram_cfg.video_proc[idx];
			vproc_func_param.out_func |= HD_VIDEOPROC_OUTFUNC_ONEBUF;
#if 0
			if (img_vcap_out_size[idx].w && img_vcap_out_size[idx].h) {
				sz.w = img_vcap_out_size[idx].w;
				sz.h = img_vcap_out_size[idx].h;
			} else {
				if (IPLUserSize[idx].size.w && IPLUserSize[idx].size.h) {
					sz.w = IPLUserSize[idx].size.w;
					sz.h = IPLUserSize[idx].size.h;
				} else {
					sz.w = img_vproc_in_param[idx].dim.w;
					sz.h = img_vproc_in_param[idx].dim.h;
				}
			}
#endif
			if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][4], &(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][4]))) != HD_OK) {
				DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
				ret = E_SYS;
			}

			IACOMM_VPROC_PARAM vproc_param = {0};
			vproc_param.video_proc_path = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][4];
			vproc_param.p_dim           = &sz2;
			vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420_NVX2;
			vproc_param.frc             = _ImageApp_MovieMulti_frc(1, 1);
			vproc_param.pfunc           = &vproc_func_param;
			if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
				DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}

#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
		if (img_vproc_eis_func[idx]) {
			BOOL eis_func = TRUE;
			if ((hd_ret = vendor_videoproc_set(ImgLink[idx].vproc_p_ctrl[0], VENDOR_VIDEOPROC_PARAM_EIS_FUNC, &eis_func)) != HD_OK) {
				DBG_ERR("vendor_videoproc_set(VENDOR_VIDEOPROC_PARAM_EIS_FUNC) fail(%d)\r\n", hd_ret);
			} else {
				DBG_DUMP("vendor_videoproc_set(VENDOR_VIDEOPROC_PARAM_EIS_FUNC) OK\r\n");
			}
		}
#endif

		if (gImeMaxOutPath[idx] < 4) {
			// config real path
			IACOMM_VPROC_PARAM vproc_param = {0};
			HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
			vproc_func_param.ddr_id = dram_cfg.video_proc[idx];
			if (img_vproc_dis_func[idx]) {
				vproc_func_param.out_func |= HD_VIDEOPROC_OUTFUNC_DIS;
			}
			for (j = 0; j < gImeMaxOutPath[idx]; j++) {
				sz.w = g_ImeSetting[idx][j].size.w;
				sz.h = g_ImeSetting[idx][j].size.h;
				fps_vprc_p_out[idx][j] = g_ImeSetting[idx][j].fps;

				if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][j], &(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j]))) != HD_OK) {
					DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				vproc_param.video_proc_path = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j];
				vproc_param.p_dim           = &sz;
				if (j == 0 && img_vproc_yuv_compress[idx]) {
					vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420_NVX2;
				} else {
					vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
				}
				if (fps_vprc_p_out[idx][j] > fps_vcap_out[idx]) {
					fps_vprc_p_out[idx][j] = fps_vcap_out[idx];
				}
				vproc_param.frc             = _ImageApp_MovieMulti_frc(fps_vprc_p_out[idx][j], fps_vcap_out[idx]);
				vproc_param.pfunc           = &vproc_func_param;
				memset((void *)&(vproc_param.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
				if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN]])) {
					if (MainIMECropInfo[idx].IMEWin.w && MainIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = MainIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = MainIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = MainIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = MainIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = MainIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = MainIMECropInfo[idx].IMEWin.h;
					}
				} else if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE]])) {
					if (CloneIMECropInfo[idx].IMEWin.w && CloneIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = CloneIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = CloneIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = CloneIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = CloneIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = CloneIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = CloneIMECropInfo[idx].IMEWin.h;
					}
				} else if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP]])) {
					if (DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = DispIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = DispIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = DispIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = DispIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = DispIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = DispIMECropInfo[idx].IMEWin.h;
					}
				}
				if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
					DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				if (img_vproc_no_ext[idx]) {
					_ImageApp_MovieMulti_UpdateVprcDepth(vproc_param.video_proc_path, 1);
				}
			}
			if (img_vproc_no_ext[idx] == FALSE) {
				// config extend path
				// 0~5 mapping to main / clone / disp / wifi / alg / disp2
				IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};
				HD_VIDEOPROC_FUNC_CONFIG vproc_func_param_ex = {0};
				vproc_func_param_ex.ddr_id = dram_cfg.video_proc[idx];
				vproc_param_ex.pfunc = &vproc_func_param_ex;
				for (j = 0; j < 6; j++) {
					if (gSwitchLink[idx][j] != UNUSED) {
						sz.w = g_ImeSetting[idx][gSwitchLink[idx][j]].size.w;
						sz.h = g_ImeSetting[idx][gSwitchLink[idx][j]].size.h;
						if (j == IAMOVIE_VPRC_EX_MAIN) {
							fps_vprc_e_out[idx][j] = ImgLinkRecInfo[idx][0].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_CLONE) {
							fps_vprc_e_out[idx][j] = ImgLinkRecInfo[idx][1].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_DISP) {
							fps_vprc_e_out[idx][j] = MaxLinkInfo.MaxDispLink ? g_DispFps[0] : 0;
							if ((DispIMECropFixedOutSize[idx] == FALSE) && DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h) {
								sz.w = DispIMECropInfo[idx].IMEWin.w;
								sz.h = DispIMECropInfo[idx].IMEWin.h;
							}
						} else if (j == IAMOVIE_VPRC_EX_WIFI) {
							fps_vprc_e_out[idx][j] = WifiLinkStrmInfo[0].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_ALG) {
							fps_vprc_e_out[idx][j] = (ImgLinkAlgInfo[idx].path13.ImgSize.w && ImgLinkAlgInfo[idx].path13.ImgSize.h) ? ((ImgLinkAlgInfo[idx].path13.ImgFps) ? ImgLinkAlgInfo[idx].path13.ImgFps : 30) : 0;
						} else if (j == IAMOVIE_VPRC_EX_DISP2) {
							fps_vprc_e_out[idx][j] = 0;
						}

						if (j == IAMOVIE_VPRC_EX_ALG) {
							if (ImgLinkAlgInfo[idx].path13.Window.w && ImgLinkAlgInfo[idx].path13.Window.h) {
								sz.w = ImgLinkAlgInfo[idx].path13.Window.w;
								sz.h = ImgLinkAlgInfo[idx].path13.Window.h;
							}
						}

						if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o[img_vproc_splitter[idx]][j], &(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j]))) != HD_OK) {
							DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
						vproc_param_ex.video_proc_path_ex  = ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j];
						vproc_param_ex.video_proc_path_src = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][gSwitchLink[idx][j]];
						vproc_param_ex.p_dim               = &sz;
						if ((vproc_param_ex.video_proc_path_src == ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][0]) && img_vproc_yuv_compress[idx]) {
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420_NVX2;
						} else {
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420;
						}
						vproc_param_ex.dir                 = HD_VIDEO_DIR_NONE;
						if (fps_vprc_e_out[idx][j] > fps_vprc_p_out[idx][gSwitchLink[idx][j]]) {
							fps_vprc_e_out[idx][j] = fps_vprc_p_out[idx][gSwitchLink[idx][j]];
						}
						vproc_param_ex.frc                 = _ImageApp_MovieMulti_frc(fps_vprc_e_out[idx][j], fps_vprc_p_out[idx][gSwitchLink[idx][j]]);
						//vproc_param_ex.depth               = (j < 2) ? 0 : 1;       // since encode path using bind mode, set depth = 0
						vproc_param_ex.depth               = 1;                       // for rawenc case, the depth should keep 1

						memset((void *)&(vproc_param_ex.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
						if (j == IAMOVIE_VPRC_EX_ALG) {
							if (ImgLinkAlgInfo[idx].path13.Window.w && ImgLinkAlgInfo[idx].path13.Window.h) {
								HD_DIM src_sz, target_sz;
								HD_IRECT target_win;

								k = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_ALG]];
								src_sz.w = g_ImeSetting[idx][k].size.w;
								src_sz.h = g_ImeSetting[idx][k].size.h;

								target_sz.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.w * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_sz.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.h * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.x = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.x * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.y = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.y * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.w * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.h * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);

								if (target_sz.w > src_sz.w || target_sz.h > src_sz.h) {
									target_sz.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.w * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_sz.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.h * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.x = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.x * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.y = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.y * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.w * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.h * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
								}

								vproc_param_ex.p_dim->w = ImgLinkAlgInfo[idx].path13.Window.w;
								vproc_param_ex.p_dim->h = ImgLinkAlgInfo[idx].path13.Window.h;
								vproc_param_ex.video_proc_crop_param.mode = HD_CROP_ON;
								vproc_param_ex.video_proc_crop_param.win.rect.x = target_win.x;
								vproc_param_ex.video_proc_crop_param.win.rect.y = target_win.y;
								vproc_param_ex.video_proc_crop_param.win.rect.w = target_win.w;
								vproc_param_ex.video_proc_crop_param.win.rect.h = target_win.h;
								//DBG_DUMP("(1)=====>src_ime=%d, src_size=%d/%d, target_size=%d/%d, target_win=%d/%d/%d/%d\r\n", k, src_sz.w, src_sz.h, target_sz.w, target_sz.h, target_win.x, target_win.y, target_win.w, target_win.h);
							}
						}

						if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
							DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			}
		} else {
			// config real path
			IACOMM_VPROC_PARAM vproc_param = {0};
			HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
			vproc_func_param.ddr_id = dram_cfg.video_proc[idx];
			if (img_vproc_dis_func[idx]) {
				vproc_func_param.out_func |= HD_VIDEOPROC_OUTFUNC_DIS;
			}
			for (j = 0; j < 3; j++) {
				sz.w = g_ImeSetting[idx][iport[j]].size.w;
				sz.h = g_ImeSetting[idx][iport[j]].size.h;
				if (j < 2) {
					fps_vprc_p_out[idx][j] = g_ImeSetting[idx][iport[j]].fps;
				} else {
					fps_vprc_p_out[idx][j] = max(g_ImeSetting[idx][iport[2]].fps, g_ImeSetting[idx][iport[3]].fps);
				}
				if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][j], &(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j]))) != HD_OK) {   // p3 as dispay for test
					DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				vproc_param.video_proc_path = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j];
				vproc_param.p_dim           = &sz;
				if (j == 0 && img_vproc_yuv_compress[idx]) {
					vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420_NVX2;
				} else {
					vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
				}
				if (fps_vprc_p_out[idx][j] > fps_vcap_out[idx]) {
					fps_vprc_p_out[idx][j] = fps_vcap_out[idx];
				}
				vproc_param.frc             = _ImageApp_MovieMulti_frc(fps_vprc_p_out[idx][j], fps_vcap_out[idx]);
				vproc_param.pfunc           = &vproc_func_param;
				memset((void *)&(vproc_param.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
				if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN]])) {
					if (MainIMECropInfo[idx].IMEWin.w && MainIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = MainIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = MainIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = MainIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = MainIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = MainIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = MainIMECropInfo[idx].IMEWin.h;
					}
				} else if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE]])) {
					if (CloneIMECropInfo[idx].IMEWin.w && CloneIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = CloneIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = CloneIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = CloneIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = CloneIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = CloneIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = CloneIMECropInfo[idx].IMEWin.h;
					}
				} else if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP]])) {
					if (DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = DispIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = DispIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = DispIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = DispIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = DispIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = DispIMECropInfo[idx].IMEWin.h;
					}
				}
				if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
					DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				if (img_vproc_no_ext[idx]) {
					_ImageApp_MovieMulti_UpdateVprcDepth(vproc_param.video_proc_path, 1);
				}
			}
			if (img_vproc_no_ext[idx] == FALSE) {
				// config extend path
				// 0~5 mapping to main / clone / disp / wifi / alg / disp2
				IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};
				HD_VIDEOPROC_FUNC_CONFIG vproc_func_param_ex = {0};
				vproc_func_param_ex.ddr_id = dram_cfg.video_proc[idx];
				vproc_param_ex.pfunc = &vproc_func_param_ex;
				for (j = 0; j < 6; j++) {
					if (gSwitchLink[idx][j] != UNUSED) {
						sz.w = g_ImeSetting[idx][gSwitchLink[idx][j]].size.w;
						sz.h = g_ImeSetting[idx][gSwitchLink[idx][j]].size.h;
						if (j == IAMOVIE_VPRC_EX_MAIN) {
							fps_vprc_e_out[idx][j] = ImgLinkRecInfo[idx][0].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_CLONE) {
							fps_vprc_e_out[idx][j] = ImgLinkRecInfo[idx][1].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_DISP) {
							fps_vprc_e_out[idx][j] = MaxLinkInfo.MaxDispLink ? g_DispFps[0] : 0;
						} else if (j == IAMOVIE_VPRC_EX_WIFI) {
							fps_vprc_e_out[idx][j] = WifiLinkStrmInfo[0].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_ALG) {
							fps_vprc_e_out[idx][j] = (ImgLinkAlgInfo[idx].path13.ImgSize.w && ImgLinkAlgInfo[idx].path13.ImgSize.h) ? ((ImgLinkAlgInfo[idx].path13.ImgFps) ? ImgLinkAlgInfo[idx].path13.ImgFps : 30) : 0;
						} else if (j == IAMOVIE_VPRC_EX_DISP2) {
							fps_vprc_e_out[idx][j] = 0;
						}

						if (j == IAMOVIE_VPRC_EX_ALG) {
							if (ImgLinkAlgInfo[idx].path13.Window.w && ImgLinkAlgInfo[idx].path13.Window.h) {
								sz.w = ImgLinkAlgInfo[idx].path13.Window.w;
								sz.h = ImgLinkAlgInfo[idx].path13.Window.h;
							}
						}

						if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o[img_vproc_splitter[idx]][j], &(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j]))) != HD_OK) {
							DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
						k = gUserProcMap[idx][gSwitchLink[idx][j]];
						if (k == 3) {
							k = 2;
						}
						vproc_param_ex.video_proc_path_ex  = ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j];
						vproc_param_ex.video_proc_path_src = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][k];
						vproc_param_ex.p_dim               = &sz;
						if ((vproc_param_ex.video_proc_path_src == ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][0]) && img_vproc_yuv_compress[idx]) {
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420_NVX2;
						} else {
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420;
						}
						vproc_param_ex.dir                 = HD_VIDEO_DIR_NONE;
						if (fps_vprc_e_out[idx][j] > fps_vprc_p_out[idx][k]) {
							fps_vprc_e_out[idx][j] = fps_vprc_p_out[idx][k];
						}
						vproc_param_ex.frc                 = _ImageApp_MovieMulti_frc(fps_vprc_e_out[idx][j], fps_vprc_p_out[idx][k]);
						vproc_param_ex.depth               = 1;                       // for rawenc case, the depth should keep 1

						memset((void *)&(vproc_param_ex.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
						if (j == IAMOVIE_VPRC_EX_ALG) {
							if (ImgLinkAlgInfo[idx].path13.Window.w && ImgLinkAlgInfo[idx].path13.Window.h) {
								HD_DIM src_sz, target_sz;
								HD_IRECT target_win;

								//k = iport[gUserProcMap[idx][gSwitchLink[idx][4]]];
								src_sz.w = g_ImeSetting[idx][iport[k]].size.w;
								src_sz.h = g_ImeSetting[idx][iport[k]].size.h;

								target_sz.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.w * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_sz.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.h * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.x = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.x * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.y = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.y * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.w * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.h * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);

								if (target_sz.w > src_sz.w || target_sz.h > src_sz.h) {
									target_sz.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.w * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_sz.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.h * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.x = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.x * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.y = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.y * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.w * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.h * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
								}

								vproc_param_ex.p_dim->w = ImgLinkAlgInfo[idx].path13.Window.w;
								vproc_param_ex.p_dim->h = ImgLinkAlgInfo[idx].path13.Window.h;
								vproc_param_ex.video_proc_crop_param.mode = HD_CROP_ON;
								vproc_param_ex.video_proc_crop_param.win.rect.x = target_win.x;
								vproc_param_ex.video_proc_crop_param.win.rect.y = target_win.y;
								vproc_param_ex.video_proc_crop_param.win.rect.w = target_win.w;
								vproc_param_ex.video_proc_crop_param.win.rect.h = target_win.h;
								//DBG_DUMP("=====>(2)src_ime=%d, src_size=%d/%d, target_size=%d/%d, target_win=%d/%d/%d/%d\r\n", k, src_sz.w, src_sz.h, target_sz.w, target_sz.h, target_win.x, target_win.y, target_win.w, target_win.h);
							}
						}

						if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
							DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			}
		}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	} else {
		img_vproc_cfg[idx].isp_id = img_sensor_mapping[idx];
#if defined(_BSP_NA51089_)
		if (img_vcap_sie_remap[idx] == TRUE) {
			img_vproc_cfg[idx].isp_id = ISP_ID_REMAP_SIE(img_sensor_mapping[idx], 1);
		}
#endif
		img_vproc_cfg[idx].in_max.func = 0;
		img_vproc_cfg[idx].in_max.dim.w = ImgLink[idx].vcap_syscaps.max_dim.w;
		img_vproc_cfg[idx].in_max.dim.h = ImgLink[idx].vcap_syscaps.max_dim.h;
		img_vproc_cfg[idx].in_max.pxlfmt = img_vcap_capout_pxlfmt[idx];
		img_vproc_cfg[idx].in_max.frc = _ImageApp_MovieMulti_frc(1, 1);

		// config vproc_in_param here to find 3dnr path
		img_vproc_in_param[idx].func = 0;
#if 0
		if (img_vcap_out_size[idx].w && img_vcap_out_size[idx].h) {
				img_vproc_in_param[idx].dim.w = img_vcap_out_size[idx].w;
				img_vproc_in_param[idx].dim.h = img_vcap_out_size[idx].h;
		} else {
			if (img_vcap_crop_win[idx].Win.w && img_vcap_crop_win[idx].Win.h) {
				img_vproc_in_param[idx].dim.w = img_vcap_crop_win[idx].Win.w;
				img_vproc_in_param[idx].dim.h = img_vcap_crop_win[idx].Win.h;
			} else {
				img_vproc_in_param[idx].dim.w = img_vcap_in_param[idx].dim.w;
				img_vproc_in_param[idx].dim.h = img_vcap_in_param[idx].dim.h;
			}
		}
#else
		img_vproc_in_param[idx].dim.w = sz2.w;
		img_vproc_in_param[idx].dim.h = sz2.h;
#endif
		img_vproc_in_param[idx].pxlfmt = img_vcap_capout_pxlfmt[idx];
		//img_vproc_in_param[idx].dir = HD_VIDEO_DIR_NONE;
		img_vproc_in_param[idx].frc = _ImageApp_MovieMulti_frc(1, 1);
#if 0
		if ((ImgLinkRecInfo[idx][0].vproc_func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE)) {
			if (gImeMaxOutPath[idx] < 4) {
				for (j = 0; j < gImeMaxOutPath[idx]; j++) {
					if ((img_vcap_in_param[idx].dim.w == g_ImeSetting[idx][j].size.w) && (img_vcap_in_param[idx].dim.h == g_ImeSetting[idx][j].size.h)) {
						img_vproc_3dnr_path[idx] = j;
					}
				}
			} else {
				for (j = 0; j < 3; j++) {
					if ((img_vcap_in_param[idx].dim.w == g_ImeSetting[idx][iport[j]].size.w) && (img_vcap_in_param[idx].dim.h == g_ImeSetting[idx][iport[j]].size.h)) {
						img_vproc_3dnr_path[idx] = j;
					}
				}
			}
		}
#else
		img_vproc_3dnr_path[idx] = 0;
#endif

		if (HD_VIDEO_PXLFMT_CLASS(img_vcap_capout_pxlfmt[idx]) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
			// sensor 0 direct mode  , sensor 1, 2, 3 should use PIPE ISE mode.
			if ((img_vcap_func_cfg[0].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) && (idx != 0)) {
#if !defined(_BSP_NA51089_)
				img_vproc_cfg[idx].pipe = (img_vproc_forced_use_pipe[idx] == HD_VIDEOPROC_PIPE_VPE) ? HD_VIDEOPROC_PIPE_VPE : HD_VIDEOPROC_PIPE_YUVAUX;
#else
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_YUVAUX;
#endif
				img_vproc_cfg[idx].ctrl_max.func = 0;
				img_vproc_ctrl[idx].func = 0;
			} else if (img_vproc_forced_use_pipe[idx] == HD_VIDEOPROC_PIPE_YUVAUX) {
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_YUVAUX;
				img_vproc_cfg[idx].ctrl_max.func = 0;
				img_vproc_ctrl[idx].func = 0;
#if !defined(_BSP_NA51089_)
			} else if (img_vproc_forced_use_pipe[idx] == HD_VIDEOPROC_PIPE_VPE) {
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_VPE;
				img_vproc_cfg[idx].ctrl_max.func = 0;
				img_vproc_ctrl[idx].func = 0;
#endif
			} else if (img_vproc_forced_use_pipe[idx] == HD_VIDEOPROC_PIPE_SCALE) {
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_SCALE;
				img_vproc_cfg[idx].ctrl_max.func &= (HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA);
				img_vproc_ctrl[idx].func = 0;
			} else {
				img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_YUVALL | img_vproc_dis_func[idx];
				img_vproc_cfg[idx].ctrl_max.func = ImgLinkRecInfo[idx][0].vproc_func;
				img_vproc_ctrl[idx].func = ImgLinkRecInfo[idx][0].vproc_func;
				if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
					img_vproc_ctrl[idx].ref_path_3dnr = (use_unique_3dnr_path[idx] == TRUE) ? ImgLink[idx].vproc_o_phy[0][4] : ImgLink[idx].vproc_o_phy[0][img_vproc_3dnr_path[idx]];
				}
			}
		} else {                                                                                // RAW
			img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_RAWALL | img_vproc_dis_func[idx];
			img_vproc_cfg[idx].ctrl_max.func = ImgLinkRecInfo[idx][0].vproc_func;
			img_vproc_ctrl[idx].func = ImgLinkRecInfo[idx][0].vproc_func;
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				img_vproc_ctrl[idx].ref_path_3dnr = (use_unique_3dnr_path[idx] == TRUE) ? ImgLink[idx].vproc_o_phy[0][4] : ImgLink[idx].vproc_o_phy[0][img_vproc_3dnr_path[idx]];
			}
		}

		// set dram id
		img_vproc_func_cfg[idx].ddr_id = dram_cfg.video_proc[idx];
		img_vproc_func_cfg[idx].in_func = 0;
		if (img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
				img_vproc_func_cfg[idx].in_func |= HD_VIDEOPROC_INFUNC_DIRECT;
		} else if (img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_ONEBUF) {
				img_vproc_func_cfg[idx].in_func |= HD_VIDEOPROC_INFUNC_ONEBUF;
		}
		IACOMM_VPROC_CFG vproc_cfg = {0};
		vproc_cfg.p_video_proc_ctrl = &(ImgLink[idx].vproc_p_ctrl[0]),
		vproc_cfg.ctrl_id           = ImgLink[idx].vproc_ctrl[0];
		vproc_cfg.pcfg              = &(img_vproc_cfg[idx]);
		vproc_cfg.pctrl             = &(img_vproc_ctrl[idx]);
		vproc_cfg.pfunc             = &(img_vproc_func_cfg[idx]);

		if ((hd_ret = vproc_set_cfg(&vproc_cfg)) != HD_OK) {
			DBG_ERR("vproc_set_cfg fail(%d)\n", hd_ret);
			ret = E_SYS;
		}

		IACOMM_VPROC_PARAM_IN vproc_param_in = {0};
		vproc_param_in.video_proc_path = ImgLink[idx].vproc_i[0][0];
		vproc_param_in.in_param        = &(img_vproc_in_param[idx]);

		if ((hd_ret = vproc_set_param_in(&vproc_param_in)) != HD_OK) {
			DBG_ERR("vproc_set_param_in fail(%d)\n", hd_ret);
			ret = E_SYS;
		}


		if ((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == TRUE)) {
			// config 3dnr path
			HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
			vproc_func_param.ddr_id = dram_cfg.video_proc[idx];
			vproc_func_param.out_func |= HD_VIDEOPROC_OUTFUNC_ONEBUF;

			if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[0][0], ImgLink[idx].vproc_o_phy[0][4], &(ImgLink[idx].vproc_p_phy[0][4]))) != HD_OK) {
				DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
				ret = E_SYS;
			}

			IACOMM_VPROC_PARAM vproc_param = {0};
			vproc_param.video_proc_path = ImgLink[idx].vproc_p_phy[0][4];
			vproc_param.p_dim           = &sz2;
			vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420_NVX2;
			vproc_param.frc             = _ImageApp_MovieMulti_frc(1, 1);
			vproc_param.pfunc           = &vproc_func_param;

			if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
				DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}

		if (img_vproc_eis_func[idx]) {
			BOOL eis_func = TRUE;
			if ((hd_ret = vendor_videoproc_set(ImgLink[idx].vproc_p_ctrl[0], VENDOR_VIDEOPROC_PARAM_EIS_FUNC, &eis_func)) != HD_OK) {
				DBG_ERR("vendor_videoproc_set(VENDOR_VIDEOPROC_PARAM_EIS_FUNC) fail(%d)\r\n", hd_ret);
			}
		}

		// config physical path
		IACOMM_VPROC_PARAM vproc_param = {0};
		HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
		vproc_func_param.ddr_id = dram_cfg.video_proc[idx];
		if (img_vproc_dis_func[idx]) {
			vproc_func_param.out_func |= HD_VIDEOPROC_OUTFUNC_DIS;
		}
		for (j = 0; j < 1; j++) {
			sz.w = img_vproc_in_param[idx].dim.w;
			sz.h = img_vproc_in_param[idx].dim.h;

			if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[0][0], ImgLink[idx].vproc_o_phy[0][j], &(ImgLink[idx].vproc_p_phy[0][j]))) != HD_OK) {
				DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			vproc_param.video_proc_path = ImgLink[idx].vproc_p_phy[0][j];
			vproc_param.p_dim           = &sz;
			vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
			vproc_param.frc             = _ImageApp_MovieMulti_frc(1, 1);
			vproc_param.pfunc           = &vproc_func_param;
			memset((void *)&(vproc_param.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));

			if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
				DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
				ret = E_SYS;
			}

			if (img_manual_push_vpe_frame[idx] & MOVIE_VPE_1) {
				_ImageApp_MovieMulti_UpdateVprcDepth(ImgLink[idx].vproc_p_phy[0][j], 1);
			}
		}

		img_vproc_cfg[idx].isp_id = 0; //ISP_EFFECT_ID;
		img_vproc_cfg[idx].in_max.func = 0;
		img_vproc_cfg[idx].in_max.dim.w = sz.w;
		img_vproc_cfg[idx].in_max.dim.h = sz.h;
		img_vproc_cfg[idx].in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		img_vproc_cfg[idx].in_max.frc = _ImageApp_MovieMulti_frc(1, 1);

		// config vproc_in_param here to find 3dnr path
		img_vproc_in_param[idx].func = 0;
		img_vproc_in_param[idx].dim.w = sz.w;
		img_vproc_in_param[idx].dim.h = sz.h;
		img_vproc_in_param[idx].pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		//img_vproc_in_param[idx].dir = HD_VIDEO_DIR_NONE;
		img_vproc_in_param[idx].frc = _ImageApp_MovieMulti_frc(1, 1);

		img_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_VPE;
		img_vproc_cfg[idx].ctrl_max.func = 0;
		img_vproc_ctrl[idx].func = 0;
		img_vproc_ctrl[idx].ref_path_3dnr = 0;

		// set dram id
		img_vproc_func_cfg[idx].ddr_id = dram_cfg.video_proc[idx];
		img_vproc_func_cfg[idx].in_func = 0;

		vproc_cfg.p_video_proc_ctrl = &(ImgLink[idx].vproc_p_ctrl[1]);
		vproc_cfg.ctrl_id           = ImgLink[idx].vproc_ctrl[1];
		vproc_cfg.pcfg              = &(img_vproc_cfg[idx]);
		vproc_cfg.pctrl             = &(img_vproc_ctrl[idx]);
		vproc_cfg.pfunc             = &(img_vproc_func_cfg[idx]);

		if ((hd_ret = vproc_set_cfg(&vproc_cfg)) != HD_OK) {
			DBG_ERR("vproc_set_cfg fail(%d)\n", hd_ret);
			ret = E_SYS;
		}

		if ((hd_ret = vendor_videoproc_set(ImgLink[idx].vproc_p_ctrl[1], VENDOR_VIDEOPROC_PARAM_VPE_SCENE, &(img_vproc_vpe_scene[idx]))) != HD_OK) {
			DBG_ERR("vendor_videoproc_set(VENDOR_VIDEOPROC_PARAM_VPE_SCENE)=%d fail(%d)\n", img_vproc_vpe_scene[idx], hd_ret);
		}

		{
			UINT32 in_depth = img_vproc_queue_depth[idx] ? img_vproc_queue_depth[idx] : 2;
			if ((hd_ret = vendor_videoproc_set(ImgLink[idx].vproc_p_ctrl[img_vproc_splitter[idx]], VENDOR_VIDEOPROC_PARAM_IN_DEPTH, &in_depth)) != HD_OK) {
				DBG_ERR("vendor_videoproc_set(VENDOR_VIDEOPROC_PARAM_IN_DEPTH, %d) fail(%d)\r\n", in_depth, hd_ret);
			}
		}

		vproc_param_in.video_proc_path = ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0];
		vproc_param_in.in_param        = &(img_vproc_in_param[idx]);

		if ((hd_ret = vproc_set_param_in(&vproc_param_in)) != HD_OK) {
			DBG_ERR("vproc_set_param_in fail(%d)\n", hd_ret);
			ret = E_SYS;
		}

		if (gImeMaxOutPath[idx] < 4) {
			// config real path
			IACOMM_VPROC_PARAM vproc_param = {0};
			HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
			vproc_func_param.ddr_id = dram_cfg.video_proc[idx];
			if (img_vproc_dis_func[idx]) {
				vproc_func_param.out_func |= HD_VIDEOPROC_OUTFUNC_DIS;
			}
			for (j = 0; j < gImeMaxOutPath[idx]; j++) {
				sz.w = g_ImeSetting[idx][j].size.w;
				sz.h = g_ImeSetting[idx][j].size.h;
				fps_vprc_p_out[idx][j] = g_ImeSetting[idx][j].fps;

				if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][j], &(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j]))) != HD_OK) {
					DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				vproc_param.video_proc_path = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j];
				vproc_param.p_dim           = &sz;
				if (j == 0 && img_vproc_yuv_compress[idx]) {
					vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420_NVX2;
				} else {
					vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
				}
				if (fps_vprc_p_out[idx][j] > fps_vcap_out[idx]) {
					fps_vprc_p_out[idx][j] = fps_vcap_out[idx];
				}
				vproc_param.frc             = _ImageApp_MovieMulti_frc(fps_vprc_p_out[idx][j], fps_vcap_out[idx]);
				vproc_param.pfunc           = &vproc_func_param;
				memset((void *)&(vproc_param.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
				if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN]])) {
					if (MainIMECropInfo[idx].IMEWin.w && MainIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = MainIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = MainIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = MainIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = MainIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = MainIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = MainIMECropInfo[idx].IMEWin.h;
					}
				} else if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE]])) {
					if (CloneIMECropInfo[idx].IMEWin.w && CloneIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = CloneIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = CloneIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = CloneIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = CloneIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = CloneIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = CloneIMECropInfo[idx].IMEWin.h;
					}
				} else if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP]])) {
					if (DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = DispIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = DispIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = DispIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = DispIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = DispIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = DispIMECropInfo[idx].IMEWin.h;
					}
				}
				if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
					DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				if (img_vproc_no_ext[idx]) {
					_ImageApp_MovieMulti_UpdateVprcDepth(vproc_param.video_proc_path, 1);
				}
			}
			if (img_vproc_no_ext[idx] == FALSE) {
				// config extend path
				// 0~5 mapping to main / clone / disp / wifi / alg / disp2
				IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};
				HD_VIDEOPROC_FUNC_CONFIG vproc_func_param_ex = {0};
				vproc_func_param_ex.ddr_id = dram_cfg.video_proc[idx];
				vproc_param_ex.pfunc = &vproc_func_param_ex;
				for (j = 0; j < 6; j++) {
					if (gSwitchLink[idx][j] != UNUSED) {
						sz.w = g_ImeSetting[idx][gSwitchLink[idx][j]].size.w;
						sz.h = g_ImeSetting[idx][gSwitchLink[idx][j]].size.h;
						if (j == IAMOVIE_VPRC_EX_MAIN) {
							fps_vprc_e_out[idx][j] = ImgLinkRecInfo[idx][0].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_CLONE) {
							fps_vprc_e_out[idx][j] = ImgLinkRecInfo[idx][1].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_DISP) {
							fps_vprc_e_out[idx][j] = MaxLinkInfo.MaxDispLink ? g_DispFps[0] : 0;
							if ((DispIMECropFixedOutSize[idx] == FALSE) && DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h) {
								sz.w = DispIMECropInfo[idx].IMEWin.w;
								sz.h = DispIMECropInfo[idx].IMEWin.h;
							}
						} else if (j == IAMOVIE_VPRC_EX_WIFI) {
							fps_vprc_e_out[idx][j] = WifiLinkStrmInfo[0].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_ALG) {
							fps_vprc_e_out[idx][j] = (ImgLinkAlgInfo[idx].path13.ImgSize.w && ImgLinkAlgInfo[idx].path13.ImgSize.h) ? ((ImgLinkAlgInfo[idx].path13.ImgFps) ? ImgLinkAlgInfo[idx].path13.ImgFps : 30) : 0;
						} else if (j == IAMOVIE_VPRC_EX_DISP2) {
							fps_vprc_e_out[idx][j] = 0;
						}

						if (j == IAMOVIE_VPRC_EX_ALG) {
							if (ImgLinkAlgInfo[idx].path13.Window.w && ImgLinkAlgInfo[idx].path13.Window.h) {
								sz.w = ImgLinkAlgInfo[idx].path13.Window.w;
								sz.h = ImgLinkAlgInfo[idx].path13.Window.h;
							}
						}

						if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o[img_vproc_splitter[idx]][j], &(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j]))) != HD_OK) {
							DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
						vproc_param_ex.video_proc_path_ex  = ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j];
						vproc_param_ex.video_proc_path_src = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][gSwitchLink[idx][j]];
						vproc_param_ex.p_dim               = &sz;
						if ((vproc_param_ex.video_proc_path_src == ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][0]) && img_vproc_yuv_compress[idx]) {
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420_NVX2;
						} else {
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420;
						}
						vproc_param_ex.dir                 = HD_VIDEO_DIR_NONE;
						if (fps_vprc_e_out[idx][j] > fps_vprc_p_out[idx][gSwitchLink[idx][j]]) {
							fps_vprc_e_out[idx][j] = fps_vprc_p_out[idx][gSwitchLink[idx][j]];
						}
						vproc_param_ex.frc                 = _ImageApp_MovieMulti_frc(fps_vprc_e_out[idx][j], fps_vprc_p_out[idx][gSwitchLink[idx][j]]);
						//vproc_param_ex.depth               = (j < 2) ? 0 : 1;       // since encode path using bind mode, set depth = 0
						vproc_param_ex.depth               = 1;                       // for rawenc case, the depth should keep 1

						memset((void *)&(vproc_param_ex.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
						if (j == IAMOVIE_VPRC_EX_ALG) {
							if (ImgLinkAlgInfo[idx].path13.Window.w && ImgLinkAlgInfo[idx].path13.Window.h) {
								HD_DIM src_sz, target_sz;
								HD_IRECT target_win;

								k = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_ALG]];
								src_sz.w = g_ImeSetting[idx][k].size.w;
								src_sz.h = g_ImeSetting[idx][k].size.h;

								target_sz.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.w * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_sz.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.h * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.x = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.x * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.y = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.y * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.w * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.h * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);

								if (target_sz.w > src_sz.w || target_sz.h > src_sz.h) {
									target_sz.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.w * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_sz.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.h * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.x = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.x * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.y = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.y * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.w * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.h * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
								}

								vproc_param_ex.p_dim->w = ImgLinkAlgInfo[idx].path13.Window.w;
								vproc_param_ex.p_dim->h = ImgLinkAlgInfo[idx].path13.Window.h;
								vproc_param_ex.video_proc_crop_param.mode = HD_CROP_ON;
								vproc_param_ex.video_proc_crop_param.win.rect.x = target_win.x;
								vproc_param_ex.video_proc_crop_param.win.rect.y = target_win.y;
								vproc_param_ex.video_proc_crop_param.win.rect.w = target_win.w;
								vproc_param_ex.video_proc_crop_param.win.rect.h = target_win.h;
								//DBG_DUMP("(1)=====>src_ime=%d, src_size=%d/%d, target_size=%d/%d, target_win=%d/%d/%d/%d\r\n", k, src_sz.w, src_sz.h, target_sz.w, target_sz.h, target_win.x, target_win.y, target_win.w, target_win.h);
							}
						}

						if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
							DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			}
		} else {
			// config real path
			IACOMM_VPROC_PARAM vproc_param = {0};
			HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
			vproc_func_param.ddr_id = dram_cfg.video_proc[idx];
			if (img_vproc_dis_func[idx]) {
				vproc_func_param.out_func |= HD_VIDEOPROC_OUTFUNC_DIS;
			}
			for (j = 0; j < 3; j++) {
				sz.w = g_ImeSetting[idx][iport[j]].size.w;
				sz.h = g_ImeSetting[idx][iport[j]].size.h;
				if (j < 2) {
					fps_vprc_p_out[idx][j] = g_ImeSetting[idx][iport[j]].fps;
				} else {
					fps_vprc_p_out[idx][j] = max(g_ImeSetting[idx][iport[2]].fps, g_ImeSetting[idx][iport[3]].fps);
				}
				if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][j], &(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j]))) != HD_OK) {   // p3 as dispay for test
					DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				vproc_param.video_proc_path = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j];
				vproc_param.p_dim           = &sz;
				if (j == 0 && img_vproc_yuv_compress[idx]) {
					vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420_NVX2;
				} else {
					vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
				}
				if (fps_vprc_p_out[idx][j] > fps_vcap_out[idx]) {
					fps_vprc_p_out[idx][j] = fps_vcap_out[idx];
				}
				vproc_param.frc             = _ImageApp_MovieMulti_frc(fps_vprc_p_out[idx][j], fps_vcap_out[idx]);
				vproc_param.pfunc           = &vproc_func_param;
				memset((void *)&(vproc_param.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
				if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_MAIN]])) {
					if (MainIMECropInfo[idx].IMEWin.w && MainIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = MainIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = MainIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = MainIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = MainIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = MainIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = MainIMECropInfo[idx].IMEWin.h;
					}
				} else if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_CLONE]])) {
					if (CloneIMECropInfo[idx].IMEWin.w && CloneIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = CloneIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = CloneIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = CloneIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = CloneIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = CloneIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = CloneIMECropInfo[idx].IMEWin.h;
					}
				} else if ((gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP] != UNUSED) && (j == gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP]])) {
					if (DispIMECropInfo[idx].IMEWin.w && DispIMECropInfo[idx].IMEWin.h) {
						vproc_param.p_dim->w = DispIMECropInfo[idx].IMESize.w;
						vproc_param.p_dim->h = DispIMECropInfo[idx].IMESize.h;
						vproc_param.video_proc_crop_param.mode = HD_CROP_ON;
						vproc_param.video_proc_crop_param.win.rect.x = DispIMECropInfo[idx].IMEWin.x;
						vproc_param.video_proc_crop_param.win.rect.y = DispIMECropInfo[idx].IMEWin.y;
						vproc_param.video_proc_crop_param.win.rect.w = DispIMECropInfo[idx].IMEWin.w;
						vproc_param.video_proc_crop_param.win.rect.h = DispIMECropInfo[idx].IMEWin.h;
					}
				}
				if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
					DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
				if (img_vproc_no_ext[idx]) {
					_ImageApp_MovieMulti_UpdateVprcDepth(vproc_param.video_proc_path, 1);
				}
			}
			if (img_vproc_no_ext[idx] == FALSE) {
				// config extend path
				// 0~5 mapping to main / clone / disp / wifi / alg / disp2
				IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};
				HD_VIDEOPROC_FUNC_CONFIG vproc_func_param_ex = {0};
				vproc_func_param_ex.ddr_id = dram_cfg.video_proc[idx];
				vproc_param_ex.pfunc = &vproc_func_param_ex;
				for (j = 0; j < 6; j++) {
					if (gSwitchLink[idx][j] != UNUSED) {
						sz.w = g_ImeSetting[idx][gSwitchLink[idx][j]].size.w;
						sz.h = g_ImeSetting[idx][gSwitchLink[idx][j]].size.h;
						if (j == IAMOVIE_VPRC_EX_MAIN) {
							fps_vprc_e_out[idx][j] = ImgLinkRecInfo[idx][0].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_CLONE) {
							fps_vprc_e_out[idx][j] = ImgLinkRecInfo[idx][1].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_DISP) {
							fps_vprc_e_out[idx][j] = MaxLinkInfo.MaxDispLink ? g_DispFps[0] : 0;
						} else if (j == IAMOVIE_VPRC_EX_WIFI) {
							fps_vprc_e_out[idx][j] = WifiLinkStrmInfo[0].frame_rate;
						} else if (j == IAMOVIE_VPRC_EX_ALG) {
							fps_vprc_e_out[idx][j] = (ImgLinkAlgInfo[idx].path13.ImgSize.w && ImgLinkAlgInfo[idx].path13.ImgSize.h) ? ((ImgLinkAlgInfo[idx].path13.ImgFps) ? ImgLinkAlgInfo[idx].path13.ImgFps : 30) : 0;
						} else if (j == IAMOVIE_VPRC_EX_DISP2) {
							fps_vprc_e_out[idx][j] = 0;
						}

						if (j == IAMOVIE_VPRC_EX_ALG) {
							if (ImgLinkAlgInfo[idx].path13.Window.w && ImgLinkAlgInfo[idx].path13.Window.h) {
								sz.w = ImgLinkAlgInfo[idx].path13.Window.w;
								sz.h = ImgLinkAlgInfo[idx].path13.Window.h;
							}
						}

						if ((hd_ret = vproc_open_ch(ImgLink[idx].vproc_i[img_vproc_splitter[idx]][0], ImgLink[idx].vproc_o[img_vproc_splitter[idx]][j], &(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j]))) != HD_OK) {
							DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
						k = gUserProcMap[idx][gSwitchLink[idx][j]];
						if (k == 3) {
							k = 2;
						}
						vproc_param_ex.video_proc_path_ex  = ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j];
						vproc_param_ex.video_proc_path_src = ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][k];
						vproc_param_ex.p_dim               = &sz;
						if ((vproc_param_ex.video_proc_path_src == ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][0]) && img_vproc_yuv_compress[idx]) {
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420_NVX2;
						} else {
							vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420;
						}
						vproc_param_ex.dir                 = HD_VIDEO_DIR_NONE;
						if (fps_vprc_e_out[idx][j] > fps_vprc_p_out[idx][k]) {
							fps_vprc_e_out[idx][j] = fps_vprc_p_out[idx][k];
						}
						vproc_param_ex.frc                 = _ImageApp_MovieMulti_frc(fps_vprc_e_out[idx][j], fps_vprc_p_out[idx][k]);
						vproc_param_ex.depth               = 1;                       // for rawenc case, the depth should keep 1

						memset((void *)&(vproc_param_ex.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
						if (j == IAMOVIE_VPRC_EX_ALG) {
							if (ImgLinkAlgInfo[idx].path13.Window.w && ImgLinkAlgInfo[idx].path13.Window.h) {
								HD_DIM src_sz, target_sz;
								HD_IRECT target_win;

								//k = iport[gUserProcMap[idx][gSwitchLink[idx][4]]];
								src_sz.w = g_ImeSetting[idx][iport[k]].size.w;
								src_sz.h = g_ImeSetting[idx][iport[k]].size.h;

								target_sz.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.w * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_sz.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.h * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.x = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.x * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.y = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.y * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.w * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);
								target_win.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.h * src_sz.w / ImgLinkAlgInfo[idx].path13.ImgSize.w);

								if (target_sz.w > src_sz.w || target_sz.h > src_sz.h) {
									target_sz.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.w * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_sz.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.ImgSize.h * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.x = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.x * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.y = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.y * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.w = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.w * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
									target_win.h = ALIGN_CEIL_4(ImgLinkAlgInfo[idx].path13.Window.h * src_sz.h / ImgLinkAlgInfo[idx].path13.ImgSize.h);
								}

								vproc_param_ex.p_dim->w = ImgLinkAlgInfo[idx].path13.Window.w;
								vproc_param_ex.p_dim->h = ImgLinkAlgInfo[idx].path13.Window.h;
								vproc_param_ex.video_proc_crop_param.mode = HD_CROP_ON;
								vproc_param_ex.video_proc_crop_param.win.rect.x = target_win.x;
								vproc_param_ex.video_proc_crop_param.win.rect.y = target_win.y;
								vproc_param_ex.video_proc_crop_param.win.rect.w = target_win.w;
								vproc_param_ex.video_proc_crop_param.win.rect.h = target_win.h;
								//DBG_DUMP("=====>(2)src_ime=%d, src_size=%d/%d, target_size=%d/%d, target_win=%d/%d/%d/%d\r\n", k, src_sz.w, src_sz.h, target_sz.w, target_sz.h, target_win.x, target_win.y, target_win.w, target_win.h);
							}
						}

						if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
							DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
							ret = E_SYS;
						}
					}
				}
			}
		}
	}
#endif // #if (IAMOVIE_SUPPORT_VPE == ENABLE)

	// set videoenc
	for (j = 0; j < 2; j++) {
		if ((hd_ret = venc_open_ch(ImgLink[idx].venc_i[j], ImgLink[idx].venc_o[j], &(ImgLink[idx].venc_p[j]))) != HD_OK) {
			DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		_ImageApp_MovieMulti_ImgSetVEncParam(idx, j);
		if (img_venc_bs_buf_va[idx][j]) {
			hd_common_mem_munmap((void *)img_venc_bs_buf_va[idx][j], img_venc_bs_buf[idx][j].buf_info.buf_size);
			img_venc_bs_buf_va[idx][j] = 0;
		}
		img_venc_bs_buf[idx][j].buf_info.phy_addr = 0;
		img_venc_bs_buf[idx][j].buf_info.buf_size = 0;
	}

	// set bsmux
	IACOMM_BSMUX_PARAM bsmux_param;
	HD_BSMUX_EN_UTIL util = {0};
	for (j = 0; j < 4; j++) {
		bsmux_open_ch(ImgLink[idx].bsmux_i[j], ImgLink[idx].bsmux_o[j], &(ImgLink[idx].bsmux_p[j]));

		bsmux_param.path_id = ImgLink[idx].bsmux_p[j];
		bsmux_param.id      = HD_BSMUX_PARAM_VIDEOINFO;
		bsmux_param.p_param = (void*)&(img_bsmux_vinfo[idx][j]);
		bsmux_set_param(&bsmux_param);

		bsmux_param.path_id = ImgLink[idx].bsmux_p[j];
		bsmux_param.id      = HD_BSMUX_PARAM_AUDIOINFO;
		bsmux_param.p_param = (void*)&(img_bsmux_ainfo[idx][j]);
		bsmux_set_param(&bsmux_param);

		bsmux_param.path_id = ImgLink[idx].bsmux_p[j];
		bsmux_param.id      = HD_BSMUX_PARAM_FILEINFO;
		bsmux_param.p_param = (void*)&(img_bsmux_finfo[idx][j]);
		bsmux_set_param(&bsmux_param);

		bsmux_param.path_id = ImgLink[idx].bsmux_p[j];
		bsmux_param.id      = HD_BSMUX_PARAM_REG_CALLBACK;
		bsmux_param.p_param = (void*)_ImageApp_MovieMulti_BsMux_CB;
		bsmux_set_param(&bsmux_param);

		UINT32 tbl = j / 2;
		util.type = HD_BSMUX_EN_UTIL_GPS_DATA;
		util.enable = gpsdata_rec_disable[idx][tbl] ? FALSE : TRUE;
		util.resv[0] = gpsdata_rec_rate[idx][tbl] ? gpsdata_rec_rate[idx][tbl] : 1;
		hd_bsmux_set(ImgLink[idx].bsmux_p[j], HD_BSMUX_PARAM_EN_UTIL, &util);

		util.type = HD_BSMUX_EN_UTIL_DUR_LIMIT;
		util.enable = TRUE;
		if (tbl == 0) {         // main
			util.resv[0] = (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) ? ((ImgLinkFileInfo.emr_sec * 1000000) + IAMOVIE_TIMESTAMP_WARNING_TIME) : 0;
		} else {                // clone
			util.resv[0] = (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) ? ((ImgLinkFileInfo.clone_emr_sec * 1000000) + IAMOVIE_TIMESTAMP_WARNING_TIME) : 0 ;
		}
		hd_bsmux_set(ImgLink[idx].bsmux_p[j], HD_BSMUX_PARAM_EN_UTIL, &util);
	}

	HD_PATH_ID bsmux_ctrl_path = 0;
	bsmux_get_ctrl_path(0, &bsmux_ctrl_path);
    util.type = HD_BSMUX_EN_UTIL_PTS_RESET ;
    util.enable = TRUE;
    hd_bsmux_set(bsmux_ctrl_path, HD_BSMUX_PARAM_EN_UTIL, &util);

	// set fileout
	IACOMM_FILEOUT_PARAM fileout_param;
	for (j = 0; j < 4; j++) {
		fileout_open_ch(ImgLink[idx].fileout_i[j], ImgLink[idx].fileout_o[j], &(ImgLink[idx].fileout_p[j]));

		fileout_param.path_id = ImgLink[idx].fileout_p[j];
		fileout_param.id = HD_FILEOUT_PARAM_REG_CALLBACK;
		fileout_param.p_param = (void*)_ImageApp_MovieMulti_FileOut_CB;
		fileout_set_param(&fileout_param);
		DBG_IND(">>> fileout reg callback %d\r\n", (int)_ImageApp_MovieMulti_FileOut_CB);
	}

	// bind modules
	// notice: for direct mode, both vcap and vproc should be configured before bind,
	//         therefore move all the bind actions to the tail of function.
	if (img_manual_push_raw_frame[idx] == FALSE) {
		if ((hd_ret = vcap_bind(ImgLink[idx].vcap_o[0], ImgLink[idx].vproc_i[0][0])) != HD_OK) {
			DBG_ERR("vcap_bind fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	if ((img_vproc_vpe_en[idx] & MOVIE_VPE_1) && !(img_manual_push_vpe_frame[idx] & MOVIE_VPE_1)) {
		if ((hd_ret = vproc_bind(ImgLink[idx].vproc_o_phy[0][0], ImgLink[idx].vproc_i[1][0])) != HD_OK) {
			DBG_ERR("vproc_bind fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
	if ((img_vproc_vpe_en[idx] & MOVIE_VPE_2) && !(img_manual_push_vpe_frame[idx] & MOVIE_VPE_2)) {
		if ((hd_ret = vproc_bind(ImgLink[idx].vproc_o_phy[0][1], ImgLink[idx].vproc_i[2][0])) != HD_OK) {
			DBG_ERR("vproc_bind fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
#endif // #if (IAMOVIE_SUPPORT_VPE == ENABLE)
	for (j = 0; j < 2; j++) {
		if (img_manual_push_vdo_frame[idx][j] == FALSE) {
			if (img_vproc_no_ext[idx] == FALSE) {
				if ((hd_ret = vproc_bind(ImgLink[idx].vproc_o[img_vproc_splitter[idx]][j], ImgLink[idx].venc_i[j])) != HD_OK) {
					DBG_ERR("vproc_bind fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
			} else {
				if (gSwitchLink[idx][j] == UNUSED) {
					DBG_ERR("vproc_bind fail since gSwitchLink[%d][%d]=%d\n", idx, j, gSwitchLink[idx][j]);
					ret = E_SYS;
					continue;
				}
				iport[0] = gUserProcMap[idx][gSwitchLink[idx][j]];     // 0:main, 1:clone
				if (iport[0] > 2) {
					iport[0] = 2;
				}
				if ((hd_ret = vproc_bind(ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][iport[0]], ImgLink[idx].venc_i[j])) != HD_OK) {
					DBG_ERR("vproc_bind fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
			}
		}
	}
	return ret;
}

static ER _ImageApp_MovieMulti_ImgUnlink(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j;
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (i >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_REC_ID_1;

	// set videocap
	if (img_manual_push_raw_frame[idx] == FALSE) {
		if ((hd_ret = vcap_unbind(ImgLink[idx].vcap_o[0])) != HD_OK) {
			DBG_ERR("vcap_unbind fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
	if ((hd_ret = vcap_close_ch(ImgLink[idx].vcap_p[0])) != HD_OK) {
		DBG_ERR("vcap_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// set videoproc
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	if (img_vproc_vpe_en[idx] & MOVIE_VPE_1) {
		if (!(img_manual_push_vpe_frame[idx] & MOVIE_VPE_1)) {
			if ((hd_ret = vproc_unbind(ImgLink[idx].vproc_o_phy[0][0])) != HD_OK) {
				DBG_ERR("vproc_unbind fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
		if ((hd_ret = vproc_close_ch(ImgLink[idx].vproc_p_phy[0][0])) != HD_OK) {
			DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
	if (img_vproc_vpe_en[idx] & MOVIE_VPE_2) {
		if (!(img_manual_push_vpe_frame[idx] & MOVIE_VPE_2)) {
			if ((hd_ret = vproc_unbind(ImgLink[idx].vproc_o_phy[0][1])) != HD_OK) {
				DBG_ERR("vproc_unbind fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
		if ((hd_ret = vproc_close_ch(ImgLink[idx].vproc_p_phy[0][1])) != HD_OK) {
			DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
#endif // #if (IAMOVIE_SUPPORT_VPE == ENABLE)
	for (j = 0; j < 2; j++) {
		if (img_manual_push_vdo_frame[idx][j] == FALSE) {
			if (img_vproc_no_ext[idx] == FALSE) {
				if ((hd_ret = vproc_unbind(ImgLink[idx].vproc_o[img_vproc_splitter[idx]][j])) != HD_OK) {
					DBG_ERR("vproc_unbind fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
			} else {
				UINT32 iport;
				if (gSwitchLink[idx][j] == UNUSED) {
					DBG_ERR("vproc_unbind fail since gSwitchLink[%d][%d]=%d\n", idx, j, gSwitchLink[idx][j]);
					ret = E_SYS;
					continue;
				}
				iport = gUserProcMap[idx][gSwitchLink[idx][j]];     // 0:main, 1:clone
				if (iport > 2) {
					iport = 2;
				}
				if ((hd_ret = vproc_unbind(ImgLink[idx].vproc_o_phy[img_vproc_splitter[idx]][iport])) != HD_OK) {
					DBG_ERR("vproc_unbind fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
			}
		}
	}
	if (gImeMaxOutPath[idx] < 4) {
		for (j = 0;  j < gImeMaxOutPath[idx]; j++) {
			if ((hd_ret = vproc_close_ch(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j])) != HD_OK) {
				DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
		if (img_vproc_no_ext[idx] == FALSE) {
			// config extend path
			for (j = 0; j < 6; j++) {
				if (gSwitchLink[idx][j] != UNUSED) {
					if ((hd_ret = vproc_close_ch(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j])) != HD_OK) {
						DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
				}
			}
		}
	} else {
		for (j = 0;  j < 3; j++) {
			if ((hd_ret = vproc_close_ch(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][j])) != HD_OK) {
				DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
		if (img_vproc_no_ext[idx] == FALSE) {
			// config extend path
			for (j = 0; j < 6; j++) {
				if (gSwitchLink[idx][j] != UNUSED) {
					if ((hd_ret = vproc_close_ch(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][j])) != HD_OK) {
						DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
						ret = E_SYS;
					}
				}
			}
		}
	}

	// close unique 3ndr path
	if ((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == TRUE)) {
		if ((hd_ret = vproc_close_ch(ImgLink[idx].vproc_p_phy[0][4])) != HD_OK) {
			DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}

	// set videoenc
	for (j = 0; j < 2; j++) {
		if ((hd_ret = venc_close_ch(ImgLink[idx].venc_p[j])) != HD_OK) {
			DBG_ERR("venc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}

	// set bsmux
	for (j = 0; j < 4; j++) {
		if ((hd_ret = bsmux_close_ch(ImgLink[idx].bsmux_p[j])) != HD_OK) {
			DBG_ERR("bsmux_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}

	// set fileout
	for (j = 0; j < 4; j++) {
		if ((hd_ret = fileout_close_ch(ImgLink[idx].fileout_p[j])) != HD_OK) {
			DBG_ERR("fileout_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
	return ret;
}

#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
static void venc_update_1st_path(void)
{
	UINT32 idx, j;

	img_venc_1st_path_idx = UNUSED;

	for (j = 0; j < 2; j++) {                                   // search main path of each ImgLink first
		for (idx = 0; idx < MaxLinkInfo.MaxImgLink; idx++) {
			if (ImgLinkStatus[idx].venc_p[j]) {
				img_venc_1st_path_idx = idx;
				img_venc_1st_path_ch = j;
				return;
			}
		}
	}
}

static void venc_status_cb(HD_PATH_ID path_id, UINT32 state)
{
	UINT32 idx = 0, j = 0;

	for (idx = 0; idx < MaxLinkInfo.MaxImgLink; idx++) {        // search each path of ImgLink
		for (j = 0; j < 2; j++) {
			if (path_id == ImgLink[idx].venc_p[j]) {
				goto cb_proc;
			}
		}
	}

cb_proc:
	if (state == CB_STATE_BEFORE_RUN) {
		img_venc_frame_cnt[idx][j] = 0;
		img_venc_frame_cnt_clr[idx][j] = 0;
		if (img_venc_1st_path_idx == UNUSED) {
			venc_update_1st_path();
		}
	} else if (state == CB_STATE_AFTER_STOP) {
		if ((img_venc_1st_path_idx == UNUSED) || (path_id == ImgLink[img_venc_1st_path_idx].venc_p[img_venc_1st_path_ch])) {
			venc_update_1st_path();
		}
	}
}
#endif

ER _ImageApp_MovieMulti_ImgLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction)
{
	UINT32 i = id, idx, j;
	ER ret = E_OK;

	if (i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) {
		idx = i - _CFG_REC_ID_1;
	} else if ((i >= _CFG_CLONE_ID_1) && (i < (_CFG_CLONE_ID_1 + MaxLinkInfo.MaxImgLink))) {
		idx = i - _CFG_CLONE_ID_1;
	} else {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}

	if (direction == UPDATE_FORWARD) {
		// notice: if use direct mode, both start/stop should follow the sequence vproc -> vap
		// set videocap (if != direct mode)
		if (img_vcap_func_cfg[idx].out_func != HD_VIDEOCAP_OUTFUNC_DIRECT) {
			ret |= _LinkUpdateStatus(vcap_set_state,      ImgLink[idx].vcap_p[0],       &(ImgLinkStatus[idx].vcap_p[0]),        NULL);
		}
		// set videoproc
			for (j = 0; j < VPROC_PER_LINK; j++) {
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][0],  &(ImgLinkStatus[idx].vproc_p_phy[j][0]),   NULL);
				if ((j == 0) && (img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT)) {
					if (_LinkCheckStatus2(ImgLinkStatus[idx].vcap_p[0]) == STATE_PREPARE_TO_STOP) {
						_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[j][0]));
						ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][0],  &(ImgLinkStatus[idx].vproc_p_phy[j][0]),   NULL);
					}
				}
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][1],  &(ImgLinkStatus[idx].vproc_p_phy[j][1]),   NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][2],  &(ImgLinkStatus[idx].vproc_p_phy[j][2]),   NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][3],  &(ImgLinkStatus[idx].vproc_p_phy[j][3]),   NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][4],  &(ImgLinkStatus[idx].vproc_p_phy[j][4]),   NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][0],      &(ImgLinkStatus[idx].vproc_p[j][0]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][1],      &(ImgLinkStatus[idx].vproc_p[j][1]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][2],      &(ImgLinkStatus[idx].vproc_p[j][2]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][3],      &(ImgLinkStatus[idx].vproc_p[j][3]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][4],      &(ImgLinkStatus[idx].vproc_p[j][4]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][5],      &(ImgLinkStatus[idx].vproc_p[j][5]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][6],      &(ImgLinkStatus[idx].vproc_p[j][6]),       NULL);
			}
		// set videocap (if == direct mode)
		if (img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
			ret |= _LinkUpdateStatus(vcap_set_state,      ImgLink[idx].vcap_p[0],       &(ImgLinkStatus[idx].vcap_p[0]),        NULL);
		}
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
		// set videoenc
		ret |= _LinkUpdateStatus(venc_set_state,      ImgLink[idx].venc_p[0],       &(ImgLinkStatus[idx].venc_p[0]),        venc_status_cb);
		ret |= _LinkUpdateStatus(venc_set_state,      ImgLink[idx].venc_p[1],       &(ImgLinkStatus[idx].venc_p[1]),        venc_status_cb);
		// set bsmux
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[0],      &(ImgLinkStatus[idx].bsmux_p[0]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[1],      &(ImgLinkStatus[idx].bsmux_p[1]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[2],      &(ImgLinkStatus[idx].bsmux_p[2]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[3],      &(ImgLinkStatus[idx].bsmux_p[3]),       NULL);
#else
		// set videoenc
		ret |= _LinkUpdateStatus(venc_set_state,      ImgLink[idx].venc_p[0],       &(ImgLinkStatus[idx].venc_p[0]),        NULL);
		ret |= _LinkUpdateStatus(venc_set_state,      ImgLink[idx].venc_p[1],       &(ImgLinkStatus[idx].venc_p[1]),        NULL);
		// set bsmux
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[0],      &(ImgLinkStatus[idx].bsmux_p[0]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[1],      &(ImgLinkStatus[idx].bsmux_p[1]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[2],      &(ImgLinkStatus[idx].bsmux_p[2]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[3],      &(ImgLinkStatus[idx].bsmux_p[3]),       NULL);
#endif
		// set fileout
		ret |= _LinkUpdateStatus(fileout_set_state, ImgLink[idx].fileout_p[0],      &(ImgLinkStatus[idx].fileout_p[0]),     NULL);
		ret |= _LinkUpdateStatus(fileout_set_state, ImgLink[idx].fileout_p[1],      &(ImgLinkStatus[idx].fileout_p[1]),     NULL);
		ret |= _LinkUpdateStatus(fileout_set_state, ImgLink[idx].fileout_p[2],      &(ImgLinkStatus[idx].fileout_p[2]),     NULL);
		ret |= _LinkUpdateStatus(fileout_set_state, ImgLink[idx].fileout_p[3],      &(ImgLinkStatus[idx].fileout_p[3]),     NULL);
	} else {
		// set fileout
		ret |= _LinkUpdateStatus(fileout_set_state, ImgLink[idx].fileout_p[0],      &(ImgLinkStatus[idx].fileout_p[0]),     NULL);
		ret |= _LinkUpdateStatus(fileout_set_state, ImgLink[idx].fileout_p[1],      &(ImgLinkStatus[idx].fileout_p[1]),     NULL);
		ret |= _LinkUpdateStatus(fileout_set_state, ImgLink[idx].fileout_p[2],      &(ImgLinkStatus[idx].fileout_p[2]),     NULL);
		ret |= _LinkUpdateStatus(fileout_set_state, ImgLink[idx].fileout_p[3],      &(ImgLinkStatus[idx].fileout_p[3]),     NULL);
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
		// set bsmux
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[0],      &(ImgLinkStatus[idx].bsmux_p[0]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[1],      &(ImgLinkStatus[idx].bsmux_p[1]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[2],      &(ImgLinkStatus[idx].bsmux_p[2]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[3],      &(ImgLinkStatus[idx].bsmux_p[3]),       NULL);
		// set videoenc
		ret |= _LinkUpdateStatus(venc_set_state,      ImgLink[idx].venc_p[0],       &(ImgLinkStatus[idx].venc_p[0]),        venc_status_cb);
		ret |= _LinkUpdateStatus(venc_set_state,      ImgLink[idx].venc_p[1],       &(ImgLinkStatus[idx].venc_p[1]),        venc_status_cb);
#else
		// set bsmux
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[0],      &(ImgLinkStatus[idx].bsmux_p[0]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[1],      &(ImgLinkStatus[idx].bsmux_p[1]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[2],      &(ImgLinkStatus[idx].bsmux_p[2]),       NULL);
		ret |= _LinkUpdateStatus(bsmux_set_state,     ImgLink[idx].bsmux_p[3],      &(ImgLinkStatus[idx].bsmux_p[3]),       NULL);
		// set videoenc
		ret |= _LinkUpdateStatus(venc_set_state,      ImgLink[idx].venc_p[0],       &(ImgLinkStatus[idx].venc_p[0]),        NULL);
		ret |= _LinkUpdateStatus(venc_set_state,      ImgLink[idx].venc_p[1],       &(ImgLinkStatus[idx].venc_p[1]),        NULL);
#endif
		// set videoproc
			for (j = 0; j < VPROC_PER_LINK; j++) {
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][0],  &(ImgLinkStatus[idx].vproc_p_phy[j][0]),   NULL);
				if ((j == 0) && (img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT)) {
					if (_LinkCheckStatus2(ImgLinkStatus[idx].vcap_p[0]) == STATE_PREPARE_TO_RUN) {
						_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[j][0]));
						ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][0],  &(ImgLinkStatus[idx].vproc_p_phy[j][0]),   NULL);
					}
				}
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][1],  &(ImgLinkStatus[idx].vproc_p_phy[j][1]),   NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][2],  &(ImgLinkStatus[idx].vproc_p_phy[j][2]),   NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][3],  &(ImgLinkStatus[idx].vproc_p_phy[j][3]),   NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p_phy[j][4],  &(ImgLinkStatus[idx].vproc_p_phy[j][4]),   NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][0],      &(ImgLinkStatus[idx].vproc_p[j][0]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][1],      &(ImgLinkStatus[idx].vproc_p[j][1]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][2],      &(ImgLinkStatus[idx].vproc_p[j][2]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][3],      &(ImgLinkStatus[idx].vproc_p[j][3]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][4],      &(ImgLinkStatus[idx].vproc_p[j][4]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][5],      &(ImgLinkStatus[idx].vproc_p[j][5]),       NULL);
				ret |= _LinkUpdateStatus(vproc_set_state,     ImgLink[idx].vproc_p[j][6],      &(ImgLinkStatus[idx].vproc_p[j][6]),       NULL);
			}
		// set videocap
		ret |= _LinkUpdateStatus(vcap_set_state,      ImgLink[idx].vcap_p[0],       &(ImgLinkStatus[idx].vcap_p[0]),        NULL);
	}

	return ret;
}

ER _ImageApp_MovieMulti_ImgLinkOpen(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j;
	HD_RESULT hd_ret;

	if (i >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_REC_ID_1;

	if (IsImgLinkOpened[idx] == TRUE) {
		DBG_ERR("id%d is already opened.\r\n", i);
		return E_SYS;
	}

	// Force init again due to sensor mapping is not ready before app open
	_ImageApp_MovieMulti_ImgLinkCfg(i, TRUE);

	// init variables
	for (j = 0; j < 4; j++) {
		img_bsmux_cutnow_with_period_cnt[idx][j] = UNUSED;
	}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	if (img_vproc_vpe_cb[idx]) {
		img_vproc_vpe_cb[idx](i, MOVIE_VPE_STATE_INIT, NULL);
	}
#endif

	_ImageApp_MovieMulti_SetupRecParam(i);
	_ImageApp_MovieMulti_SetupFileOption(i);

	if (IsSetImgLinkRecInfo[idx][1] == TRUE) {
		_ImageApp_MovieMulti_SetupRecParam(_CFG_CLONE_ID_1 + i);
		_ImageApp_MovieMulti_SetupFileOption(_CFG_CLONE_ID_1 + i);
	}
	_ImageApp_MovieMulti_ImgLink(i);

	// update window of vdoout
	if (MaxLinkInfo.MaxDispLink) {
		HD_URECT win;
		if (user_disp_win[0].w && user_disp_win[0].h) {
			win.x = user_disp_win[0].x;
			win.y = user_disp_win[0].y;
			win.w = user_disp_win[0].w;
			win.h = user_disp_win[0].h;
		} else if (DispLink[0].vout_win.w && DispLink[0].vout_win.h) {
			win.x = DispLink[0].vout_win.x;
			win.y = DispLink[0].vout_win.y;
			win.w = DispLink[0].vout_win.w;
			win.h = DispLink[0].vout_win.h;
		} else {
			win.x = 0;
			win.y = 0;
			win.w = DispLink[0].vout_syscaps.output_dim.w;
			win.h = DispLink[0].vout_syscaps.output_dim.h;
		}

		IACOMM_VOUT_PARAM vout_param = {
			.video_out_path = DispLink[0].vout_p[0],
			.p_rect         = &win,
			.enable         = TRUE,
			.dir            = DispLink[0].vout_dir,
			.pfunc_cfg      = &(disp_func_cfg[0]),
		};
		if ((hd_ret = vout_set_param(&vout_param)) != HD_OK) {
			DBG_ERR("vout_set_param fail=%d\n", hd_ret);
		}
	}

	IsImgLinkOpened[idx] = TRUE;

	return E_OK;
}

ER _ImageApp_MovieMulti_ImgLinkClose(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, j;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_REC_ID_1;

	if (IsImgLinkOpened[idx] == FALSE) {
		DBG_ERR("id%d is already closed.\r\n", i);
		return E_SYS;
	}

	// check whether main record path is still running
	if (_LinkCheckStatus(ImgLinkStatus[idx].fileout_p[0]) || _LinkCheckStatus(ImgLinkStatus[idx].fileout_p[1])) {
		ImageApp_MovieMulti_RecStop(_CFG_REC_ID_1 + idx);
	}
	// check whether clone record path is still running
	if (_LinkCheckStatus(ImgLinkStatus[idx].fileout_p[2]) || _LinkCheckStatus(ImgLinkStatus[idx].fileout_p[3])) {
		ImageApp_MovieMulti_RecStop(_CFG_CLONE_ID_1 + idx);
	}
	// check whether display path is still running
	if (_LinkCheckStatus(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][2])) {
		ImageApp_MovieMulti_ImgLinkForDisp(i, DISABLE, TRUE);
	}
	// check whether wifi path is still running
	if (_LinkCheckStatus(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][3])) {
		ImageApp_MovieMulti_ImgLinkForStreaming(i, DISABLE, TRUE);
	}
	// check whether alg path is still running
	if (_LinkCheckStatus(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][4])) {
		ImageApp_MovieMulti_ImgLinkForAlg(i, _CFG_ALG_PATH3, DISABLE, TRUE);
	}
	if (_LinkCheckStatus(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][6])) {
		ImageApp_MovieMulti_ImgLinkForAlg(i, _CFG_ALG_PATH4, DISABLE, TRUE);
	}

	_ImageApp_MovieMulti_ImgUnlink(i);
	IsImgLinkOpened[idx] = FALSE;

	// release buffer
	for (j = 0; j < 2; j++) {
		_ImageApp_MovieMulti_ReleaseSrcOutBuf(idx, j);
	}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	if (img_vproc_vpe_cb[idx]) {
		img_vproc_vpe_cb[idx](i, MOVIE_VPE_STATE_UNINIT, NULL);
	}
#endif

	// reset variables
	forced_use_unique_3dnr_path[idx] = FALSE;
	//memset(&ImgLink[idx], 0, sizeof(MOVIE_IMAGE_LINK));
	memset(&ImgLinkStatus[idx], 0, sizeof(MOVIE_IMAGE_LINK_STATUS));
	memset(g_ImeSetting[idx], 0, sizeof(MOVIE_IMG_IME_SETTING) * 4);
	memset(&(ImgLinkRecInfo[idx][0]), 0, sizeof(MOVIE_RECODE_INFO) * 2);
	for (j = 0; j < MAX_DISP_PATH; j++) {
		IsImgLinkForDispEnabled[idx][j] = FALSE;
	}
	//g_IsImgLinkForStreamingEnabled[idx] = 0;
	//g_IsImgLinkForAlgEnabled[idx][_CFG_ALG_PATH3] = 0;
	//g_IsImgLinkForAlgEnabled[idx][_CFG_ALG_PATH4] = 0;
	gImgLinkForcedSizePath[idx] = MOVIE_IPL_SIZE_AUTO;
	gImeMaxOutPath[idx] = 0;
	IPLUserSize[idx].size.w = 0;
	IPLUserSize[idx].size.h = 0;
	IPLUserSize[idx].fps = 0;
	fps_vcap_out[idx] = 0;
	img_vcap_detect_loop[idx] = 0;
	img_vcap_patgen[idx] = 0;
	img_vcap_gyro_func[idx] = 0;
	for (j = 0; j < 3; j++) {
		fps_vprc_p_out[idx][j] = 0;
	}
	for (j = 0; j < 6; j++) {
		fps_vprc_e_out[idx][j] = 0;
	}
	for (j = 0; j < 2; j++) {
		img_pseudo_rec_mode[idx][j] = MOVIE_PSEUDO_REC_NONE;
		img_venc_vui_disable[idx][j] = 0;
		img_venc_vui_color_tv_range[idx][j] = 0;
		img_venc_max_frame_size[idx][j] = 0;
		img_venc_imgcap_yuvsrc[idx][j] = 0;
		img_venc_h264_profile[idx][j] = 0;
		img_venc_h264_level[idx][j] = 0;
		img_venc_h265_profile[idx][j] = 0;
		img_venc_h265_level[idx][j] = 0;
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
		img_venc_xvr_app[idx][j] = 0;
#endif
		img_venc_skip_frame_cfg[idx][j] = 0;
		img_venc_svc[idx][j] = 0;
		img_venc_low_power_mode[idx][j] = 0;
		img_venc_trigger_mode[idx][j] = MOVIE_CODEC_TRIGGER_TIMER;
		trig_once_limited[idx][j] = 0;
		trig_once_cnt[idx][j] = 0;
		trig_once_first_i[idx][j] = 0;
		trig_once_2v1a_stop_path[idx][j] = 0;
		if (img_venc_bs_buf_va[idx][j]) {
			hd_common_mem_munmap((void *)img_venc_bs_buf_va[idx][j], img_venc_bs_buf[idx][j].buf_info.buf_size);
			img_venc_bs_buf_va[idx][j] = 0;
		}
		img_manual_push_vdo_frame[idx][j] = 0;
		IsImgLinkTranscodeMode[idx][j] = 0;
		img_venc_imgcap_thum_size[idx][j].w = 0;
		img_venc_imgcap_thum_size[idx][j].h = 0;
		imgcap_thum_no_auto_scaling[idx][j] = 0;
		img_emr_loop_start[idx][j] = 0;
		memset((void *)&(img_venc_maxmem[idx][j]), 0, sizeof(HD_VIDEOENC_MAXMEM));
	}
	img_manual_push_raw_frame[idx] = 0;
	img_manual_push_vpe_frame[idx] = 0;
	img_vproc_3dnr_path[idx] = 0;
	img_vproc_forced_use_pipe[idx] = 0;
	img_vproc_dis_func[idx] = 0;
	img_vproc_eis_func[idx] = 0;
	img_vproc_queue_depth[idx] = 0;
	img_vproc_no_ext[idx] = 0;
	DispIMECropNoAutoScaling[idx] = 0;
	DispIMECropFixedOutSize[idx] = 0;
#if defined(_BSP_NA51089_)
	img_vcap_sie_remap[idx] = 0;
#endif

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	img_vproc_vpe_en[idx] = 0;
	img_vproc_vpe_cb[idx] = 0;
	img_vproc_vpe_scene[idx] = 0;
#endif
	img_vproc_splitter[idx] = 0;

	for (j = 0; j < 4; j++) {
		img_bsmux_finfo[idx][j].ddr_id = DDR_ID0;
		img_bsmux_meta_data_en[idx][j] = 0;
		img_bsmux_audio_pause[idx][j] = 0;
	}

	memset(&(img_vcap_crop_win[idx]), 0, sizeof(MOVIEMULTI_VCAP_CROP_INFO));
	memset(&(img_vcap_out_size[idx]), 0, sizeof(USIZE));
	//memset(&(DispIMECropInfo[idx]), 0, sizeof(MOVIEMULTI_DISP_IME_CROP_INFO));
	memset(&(img_venc_bs_buf[idx][0]), 0, sizeof(HD_VIDEOENC_BUFINFO) * 2);
	memset(&(ad_sensor_map[idx]), 0xff, sizeof(MOVIE_AD_MAP));

	return E_OK;
}

void _ImageApp_MovieMulti_GetVideoFrameForImgCap(UINT32 id, UINT32 path, HD_VIDEO_FRAME *p_frame)
{
	vos_flag_clr(IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_FRM_WAIT);
	img_to_imgcap_id = id;
	img_to_imgcap_path = path;
	p_img_to_imgcap_frame = p_frame;
}

HD_RESULT ImageApp_MovieMulti_RecPullOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		return hd_videoproc_pull_out_buf(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl], p_video_frame, wait_ms);
	}
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT ImageApp_MovieMulti_RecPushIn(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		return hd_videoenc_push_in_buf(ImgLink[tb.idx].venc_p[tb.tbl], p_video_frame, NULL, wait_ms);
	}
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT ImageApp_MovieMulti_RecReleaseOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (p_img_to_imgcap_frame && (tb.idx == img_to_imgcap_id) && (tb.tbl == img_to_imgcap_path)) {
			memcpy((void *)p_img_to_imgcap_frame, (void*)p_video_frame, sizeof(HD_VIDEO_FRAME));
			p_img_to_imgcap_frame = NULL;
			img_to_imgcap_id = UNUSED;
			img_to_imgcap_path = UNUSED;
			vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_FRM_WAIT);
			return HD_OK;
		} else {
			return hd_videoproc_release_out_buf(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl], p_video_frame);
		}
	}
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT ImageApp_MovieMulti_RawFramePullOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		return hd_videocap_pull_out_buf(ImgLink[tb.idx].vcap_p[0], p_video_frame, wait_ms);
	}
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT ImageApp_MovieMulti_RawFramePushIn(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_PATH_ID vprc_path;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		vprc_path = ImageApp_MovieMulti_GetVprcInPort(i);
		if (vprc_path) {
			return hd_videoproc_push_in_buf(vprc_path, p_video_frame, NULL, 0);  // Non blocking mode only
		} else {
			return HD_ERR_NOT_START;
		}
	}
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT ImageApp_MovieMulti_RawFrameReleaseOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		return hd_videocap_release_out_buf(ImgLink[tb.idx].vcap_p[0], p_video_frame);
	}
	return HD_ERR_NOT_SUPPORT;
}

