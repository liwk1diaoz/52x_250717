#include "ImageApp_UsbMovie_int.h"

//#include <stdlib.h>
//#include <string.h>

/// ========== Cross file variables ==========
UINT32 IsUsbImgLinkOpened[MAX_USBIMG_PATH] = {0};
USBMOVIE_IMAGE_LINK UsbImgLink[MAX_USBIMG_PATH];
USBMOVIE_IMAGE_LINK_STATUS UsbImgLinkStatus[MAX_USBIMG_PATH] = {0};
USIZE usbimg_vcap_out_size[MAX_USBIMG_PATH] = {0};
#if defined(_BSP_NA51089_)
UINT32 usbimg_vcap_sie_remap[MAX_USBIMG_PATH] = {0};
#endif
UVAC_IPL_SIZE_INFO UsbImgIPLUserSize[MAX_USBIMG_PATH] = {0};
HD_DIM usbimg_max_imgsize[MAX_USBIMG_PATH] = {0};
HD_VIDEOCAP_CTRL usbimg_vcap_ctrl[MAX_USBIMG_PATH] = {0};
HD_VIDEOCAP_DRV_CONFIG usbimg_vcap_cfg[MAX_USBIMG_PATH] = {0};
HD_VIDEOCAP_IN usbimg_vcap_in_param[MAX_USBIMG_PATH] = {0};
HD_VIDEOCAP_CROP usbimg_vcap_crop_param[MAX_USBIMG_PATH] = {0};
HD_VIDEOCAP_OUT usbimg_vcap_out_param[MAX_USBIMG_PATH] = {0};
HD_VIDEOCAP_FUNC_CONFIG usbimg_vcap_func_cfg[MAX_USBIMG_PATH] = {0};
HD_VIDEO_PXLFMT usbimg_vcap_senout_pxlfmt[MAX_USBIMG_PATH] = {0};
HD_VIDEO_PXLFMT usbimg_vcap_capout_pxlfmt[MAX_USBIMG_PATH] = {0};
UINT32 usbimg_vcap_data_lane[MAX_USBIMG_PATH] = {0};
UINT32 usbimg_vcap_func[MAX_USBIMG_PATH] = {0};
UINT32 usbimg_vcap_patgen[MAX_USBIMG_PATH] = {0};
HD_VIDEOPROC_DEV_CONFIG usbimg_vproc_cfg[MAX_USBIMG_PATH] = {0};
HD_VIDEOPROC_CTRL usbimg_vproc_ctrl[MAX_USBIMG_PATH] = {0};
HD_VIDEOPROC_FUNC_CONFIG usbimg_vproc_func_cfg[MAX_USBIMG_PATH] = {0};
HD_VIDEOPROC_IN usbimg_vproc_in_param[MAX_USBIMG_PATH] = {0};
HD_VIDEOPROC_OUT usbimg_vproc_param[MAX_USBIMG_PATH][2] = {0};
HD_VIDEOPROC_OUT_EX usbimg_vproc_param_ex[MAX_USBIMG_PATH][1] = {0};
UINT32 usbimg_vproc_func[MAX_USBIMG_PATH] = {0};
HD_VIDEOENC_PATH_CONFIG usbimg_venc_path_cfg[MAX_USBIMG_PATH][1] = {0};
HD_VIDEOENC_IN  usbimg_venc_in_param[MAX_USBIMG_PATH][1] = {0};
HD_VIDEOENC_OUT2 usbimg_venc_out_param[MAX_USBIMG_PATH][1] = {0};
HD_H26XENC_RATE_CONTROL2 usbimg_venc_rc_param[MAX_USBIMG_PATH][1] = {0};
HD_VIDEOENC_BUFINFO usbimg_venc_bs_buf[MAX_USBIMG_PATH][1] = {0};
UINT32 usbimg_venc_bs_buf_va[MAX_USBIMG_PATH][1] = {0};
UINT32 g_UsbMovie_TBR_MJPG = NVT_USBMOVIE_TBR_MJPG_DEFAULT;
UINT32 g_UsbMovie_TBR_H264 = NVT_USBMOVIE_TBR_H264_DEFAULT;
UINT32 g_UsbMovie_TBR_Max[MAX_USBIMG_PATH][1] = {0};
UINT32 usbimg_venc_buf_ms[MAX_USBIMG_PATH][1] = {0};
UVAC_JPG_QUALITY usbimg_jpg_quality = {50, 0xffffffff};
// Important notice!!! If enlarge MAX_USBIMG_PATH, following variable shoule also update initial value too !!!
#if (MAX_USBIMG_PATH == 1)
UINT32 usbimg_sensor_mapping[MAX_USBIMG_PATH] = {0};
#elif (MAX_USBIMG_PATH == 2)
UINT32 usbimg_sensor_mapping[MAX_USBIMG_PATH] = {0, 1};
#endif

/// ========== Noncross file variables ==========

#define ISP_ID_REMAP_SIE(vcap_id, sie_map_id)  ((1 << 31) | (vcap_id << 24) | sie_map_id)

ER _ImageApp_UsbMovie_ImgLinkCfg(UINT32 id)
{
	UINT32 idx = id;

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	// video capture
	UsbImgLink[idx].vcap_ctrl        = HD_VIDEOCAP_CTRL(usbimg_sensor_mapping[idx]);
	UsbImgLink[idx].vcap_i[0]        = HD_VIDEOCAP_IN(usbimg_sensor_mapping[idx], 0);
	UsbImgLink[idx].vcap_o[0]        = HD_VIDEOCAP_OUT(usbimg_sensor_mapping[idx], 0);
	// video process
	UsbImgLink[idx].vproc_ctrl       = HD_VIDEOPROC_CTRL(usbimg_sensor_mapping[idx]);
	UsbImgLink[idx].vproc_i[0]       = HD_VIDEOPROC_IN(usbimg_sensor_mapping[idx], 0);
	UsbImgLink[idx].vproc_o_phy[0]   = HD_VIDEOPROC_OUT(usbimg_sensor_mapping[idx], 0);
	UsbImgLink[idx].vproc_o_phy[1]   = HD_VIDEOPROC_OUT(usbimg_sensor_mapping[idx], 1);
	UsbImgLink[idx].vproc_o[0]       = HD_VIDEOPROC_OUT(usbimg_sensor_mapping[idx], 5);
	// video encode
	UsbImgLink[idx].venc_i[0]        = HD_VIDEOENC_IN(0, 0 + idx * 1);
	UsbImgLink[idx].venc_o[0]        = HD_VIDEOENC_OUT(0, 0 + idx * 1);

	return E_OK;
}

static ER _ImageApp_UsbMovie_ImgLink(UINT32 id)
{
	ER ret;
	HD_RESULT hd_ret;
	UINT32 idx = id, j;
	HD_DIM sz = {0};
	UINT32 temp[2];

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	// set videocap
	// disable vcap function if pxlfmt is yuv
	if (HD_VIDEO_PXLFMT_CLASS(usbimg_vcap_capout_pxlfmt[idx]) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
		usbimg_vcap_ctrl[idx].func = 0;
		if (usbimg_vcap_patgen[idx]) {
			usbimg_vcap_patgen[idx] = 0;
			DBG_WRN("vcap pat_gen is not supported in yuv mode\r\n");
		}
	} else {                                                                                // RAW
		usbimg_vcap_ctrl[idx].func = usbimg_vcap_func[idx];
	}
	// update driver name if use pat_gen
	if (usbimg_vcap_patgen[idx]) {
		snprintf(usbimg_vcap_cfg[idx].sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
	}

	if (usbimg_vcap_cfg[idx].sen_cfg.shdr_map && !(usbimg_vcap_ctrl[idx].func & HD_VIDEOCAP_FUNC_SHDR)) {
		DBG_WRN("id[%d] HD_VIDEOCAP_FUNC_SHDR is not enabled but shdr_map is set(0x%x)\r\n", idx, usbimg_vcap_cfg[idx].sen_cfg.shdr_map);
		usbimg_vcap_cfg[idx].sen_cfg.shdr_map = 0;
	}

	IACOMM_VCAP_CFG vcap_cfg = {0};
	vcap_cfg.p_video_cap_ctrl = &(UsbImgLink[idx].vcap_p_ctrl);
	vcap_cfg.ctrl_id          = UsbImgLink[idx].vcap_ctrl;
	vcap_cfg.pcfg             = &(usbimg_vcap_cfg[idx]);
	vcap_cfg.pctrl            = &(usbimg_vcap_ctrl[idx]);
	if ((hd_ret = vcap_set_cfg(&vcap_cfg)) != HD_OK) {
		DBG_ERR("vcap_set_cfg fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

#if defined(_BSP_NA51089_)
	if (usbimg_vcap_sie_remap[idx] == TRUE) {
		UINT32 sie_map = 1;
		//56x only support compression in SIE2
		if ((hd_ret = vendor_videocap_set(UsbImgLink[idx].vcap_p_ctrl, VENDOR_VIDEOCAP_PARAM_SIE_MAP, &sie_map)) != HD_OK) {
			DBG_ERR("vendor_videocap_set(VENDOR_VIDEOCAP_PARAM_SIE_MAP) failed(%d)\r\n", hd_ret);
		}
	}
#endif

	if ((hd_ret = vcap_open_ch(UsbImgLink[idx].vcap_i[0], UsbImgLink[idx].vcap_o[0], &(UsbImgLink[idx].vcap_p[0]))) != HD_OK) {
		DBG_ERR("vcap_open_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = vcap_get_caps(UsbImgLink[idx].vcap_p_ctrl, &(UsbImgLink[idx].vcap_syscaps))) != HD_OK) {
		DBG_ERR("vcap_get_caps fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	if (usbimg_vcap_patgen[idx]) {
		usbimg_vcap_in_param[idx].sen_mode = HD_VIDEOCAP_PATGEN_MODE(usbimg_vcap_patgen[idx], ALIGN_CEIL((usbimg_max_imgsize[idx].w *10/65), 2));  // width value is a tried value
	} else {
		usbimg_vcap_in_param[idx].sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO;
	}
	if (UsbImgIPLUserSize[idx].size.w && UsbImgIPLUserSize[idx].size.h && UsbImgIPLUserSize[idx].fps) {
		usbimg_vcap_in_param[idx].frc = HD_VIDEO_FRC_RATIO(UsbImgIPLUserSize[idx].fps, 1);
		usbimg_vcap_in_param[idx].dim.w = UsbImgIPLUserSize[idx].size.w;
		usbimg_vcap_in_param[idx].dim.h = UsbImgIPLUserSize[idx].size.h;
	} else {
		usbimg_vcap_in_param[idx].frc = HD_VIDEO_FRC_RATIO(30, 1);
		usbimg_vcap_in_param[idx].dim.w = usbimg_max_imgsize[idx].w;
		usbimg_vcap_in_param[idx].dim.h = usbimg_max_imgsize[idx].h;
	}
	usbimg_vcap_in_param[idx].pxlfmt = usbimg_vcap_senout_pxlfmt[idx];

	if (usbimg_vcap_ctrl[idx].func & HD_VIDEOCAP_FUNC_SHDR) {
		temp[0] = usbimg_vcap_cfg[idx].sen_cfg.shdr_map & 0x0000ffff;
		temp[1] = 0;
		while (temp[0]) {
			if (temp[0] & 0x01) {
				temp[1]++;
			}
			temp[0] >>= 1;
		}
		usbimg_vcap_in_param[idx].out_frame_num = temp[1];
		DBG_DUMP("Set id[%d] SHDR to %d frame mode\r\n", idx, usbimg_vcap_in_param[idx].out_frame_num);
	} else {
		usbimg_vcap_in_param[idx].out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
	}

	usbimg_vcap_crop_param[idx].mode = HD_CROP_ON;
	usbimg_vcap_crop_param[idx].win.coord.w = 0;
	usbimg_vcap_crop_param[idx].win.coord.h = 0;
	usbimg_vcap_crop_param[idx].win.rect.x = -1;
	usbimg_vcap_crop_param[idx].win.rect.y = -1;
	usbimg_vcap_crop_param[idx].win.rect.w = usbimg_vcap_in_param[idx].dim.w;
	if (usbimg_vcap_senout_pxlfmt[idx] ==  HD_VIDEO_PXLFMT_YUV422 || usbimg_vcap_senout_pxlfmt[idx] ==  HD_VIDEO_PXLFMT_YUV422_ONE) {
		usbimg_vcap_crop_param[idx].win.rect.w = usbimg_vcap_in_param[idx].dim.w;
	} else {
		usbimg_vcap_crop_param[idx].win.rect.w = usbimg_vcap_in_param[idx].dim.w;
	}
	usbimg_vcap_crop_param[idx].win.rect.h = usbimg_vcap_in_param[idx].dim.h;

	usbimg_vcap_out_param[idx].pxlfmt = usbimg_vcap_capout_pxlfmt[idx];
	usbimg_vcap_out_param[idx].dir = HD_VIDEO_DIR_NONE;

	if (usbimg_vcap_out_size[idx].w && usbimg_vcap_out_size[idx].h) {
		usbimg_vcap_out_param[idx].dim.w = usbimg_vcap_out_size[idx].w;
		usbimg_vcap_out_param[idx].dim.h = usbimg_vcap_out_size[idx].h;
	} else {
		usbimg_vcap_out_param[idx].dim.w = 0;
		usbimg_vcap_out_param[idx].dim.h = 0;
	}

	usbimg_vcap_func_cfg[idx].ddr_id = usbimg_dram_cfg.video_cap[idx];

	IACOMM_VCAP_PARAM vcap_param = {0};
	vcap_param.video_cap_path = UsbImgLink[idx].vcap_p[0];
	vcap_param.data_lane      = usbimg_vcap_data_lane[idx];
	vcap_param.pin_param      = &(usbimg_vcap_in_param[idx]);
	vcap_param.pcrop_param    = &(usbimg_vcap_crop_param[idx]);
	vcap_param.pout_param     = &(usbimg_vcap_out_param[idx]);
	vcap_param.pfunc_cfg      = &(usbimg_vcap_func_cfg[idx]);
	if ((hd_ret = vcap_set_param(&vcap_param)) != HD_OK) {
		DBG_ERR("vcap_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// set videoproc
	if (HD_VIDEO_PXLFMT_CLASS(usbimg_vcap_capout_pxlfmt[idx]) == HD_VIDEO_PXLFMT_CLASS_YUV) {  // YUV
		if ((usbimg_vcap_func_cfg[0].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) && (idx != 0)) {
			usbimg_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_YUVAUX;
			usbimg_vproc_cfg[idx].ctrl_max.func = 0;
			usbimg_vproc_ctrl[idx].func = 0;
		} else {
			usbimg_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_YUVALL;
			usbimg_vproc_cfg[idx].ctrl_max.func = usbimg_vproc_func[idx];
			usbimg_vproc_ctrl[idx].func = usbimg_vproc_func[idx];
		}
	} else {
		usbimg_vproc_cfg[idx].pipe = HD_VIDEOPROC_PIPE_RAWALL;
		usbimg_vproc_cfg[idx].ctrl_max.func = usbimg_vproc_func[idx];
		usbimg_vproc_ctrl[idx].func = usbimg_vproc_func[idx];
	}
	usbimg_vproc_cfg[idx].isp_id = usbimg_sensor_mapping[idx];
#if defined(_BSP_NA51089_)
	if (usbimg_vcap_sie_remap[idx] == TRUE) {
		usbimg_vproc_cfg[idx].isp_id = ISP_ID_REMAP_SIE(usbimg_sensor_mapping[idx], 1);
	}
#endif
	usbimg_vproc_cfg[idx].in_max.func = 0;
	usbimg_vproc_cfg[idx].in_max.dim.w = UsbImgLink[idx].vcap_syscaps.max_dim.w;
	usbimg_vproc_cfg[idx].in_max.dim.h = UsbImgLink[idx].vcap_syscaps.max_dim.h;
	usbimg_vproc_cfg[idx].in_max.pxlfmt = usbimg_vcap_capout_pxlfmt[idx];
	usbimg_vproc_cfg[idx].in_max.frc = HD_VIDEO_FRC_RATIO(1, 1);

	if (usbimg_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
		usbimg_vproc_ctrl[idx].ref_path_3dnr = UsbImgLink[idx].vproc_o_phy[1];
	}

	usbimg_vproc_func_cfg[idx].in_func = 0;
	if (usbimg_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
			usbimg_vproc_func_cfg[idx].in_func |= HD_VIDEOPROC_INFUNC_DIRECT;
	} else if (usbimg_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_ONEBUF) {
			usbimg_vproc_func_cfg[idx].in_func |= HD_VIDEOPROC_INFUNC_ONEBUF;
	}

	usbimg_vproc_func_cfg[idx].ddr_id = usbimg_dram_cfg.video_proc[idx];

	IACOMM_VPROC_CFG vproc_cfg = {0};
	vproc_cfg.p_video_proc_ctrl = &(UsbImgLink[idx].vproc_p_ctrl);
	vproc_cfg.ctrl_id           = UsbImgLink[idx].vproc_ctrl;
	vproc_cfg.pcfg              = &(usbimg_vproc_cfg[idx]);
	vproc_cfg.pctrl             = &(usbimg_vproc_ctrl[idx]);
	vproc_cfg.pfunc             = &(usbimg_vproc_func_cfg[idx]);
	if ((hd_ret = vproc_set_cfg(&vproc_cfg)) != HD_OK) {
		DBG_ERR("vproc_set_cfg fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	// config vprc in
	usbimg_vproc_in_param[idx].func = 0;
	if (usbimg_vcap_out_size[idx].w && usbimg_vcap_out_size[idx].h) {
		usbimg_vproc_in_param[idx].dim.w = usbimg_vcap_out_size[idx].w;
		usbimg_vproc_in_param[idx].dim.h = usbimg_vcap_out_size[idx].h;
	} else {
		usbimg_vproc_in_param[idx].dim.w = usbimg_max_imgsize[idx].w;
		usbimg_vproc_in_param[idx].dim.h = usbimg_max_imgsize[idx].h;
	}
	usbimg_vproc_in_param[idx].pxlfmt = usbimg_vcap_capout_pxlfmt[idx];
	//usbimg_vproc_in_param[idx].dir = HD_VIDEO_DIR_NONE;
	usbimg_vproc_in_param[idx].frc = HD_VIDEO_FRC_RATIO(1, 1);
	IACOMM_VPROC_PARAM_IN vproc_param_in = {0};
	vproc_param_in.video_proc_path = UsbImgLink[idx].vproc_i[0];
	vproc_param_in.in_param        = &(usbimg_vproc_in_param[idx]);
	if ((hd_ret = vproc_set_param_in(&vproc_param_in)) != HD_OK) {
		DBG_ERR("vproc_set_param_in fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// config real path
	IACOMM_VPROC_PARAM vproc_param = {0};
	HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
	vproc_func_param.ddr_id = usbimg_dram_cfg.video_proc[idx];
	for (j = 0; j < 2; j++) {
		if ((hd_ret = vproc_open_ch(UsbImgLink[idx].vproc_i[0], UsbImgLink[idx].vproc_o_phy[j], &(UsbImgLink[idx].vproc_p_phy[j]))) != HD_OK) {
			DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		vproc_param.video_proc_path = UsbImgLink[idx].vproc_p_phy[j];
		vproc_param.p_dim           = &(usbimg_vproc_param[idx][j].dim);
		vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
		vproc_param.frc             = HD_VIDEO_FRC_RATIO(1, 1);
		vproc_param.pfunc           = &vproc_func_param;
		memset((void *)&(vproc_param.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
		if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
			DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
	// config extend path
	IACOMM_VPROC_PARAM_EX vproc_param_ex = {0};
	HD_VIDEOPROC_FUNC_CONFIG vproc_func_param_ex = {0};
	vproc_func_param_ex.ddr_id = usbimg_dram_cfg.video_proc[idx];
	for (j = 0; j < 1; j++) {
		if ((hd_ret = vproc_open_ch(UsbImgLink[idx].vproc_i[0], UsbImgLink[idx].vproc_o[j], &(UsbImgLink[idx].vproc_p[j]))) != HD_OK) {
			DBG_ERR("vproc_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		vproc_param_ex.video_proc_path_ex  = UsbImgLink[idx].vproc_p[j];
		vproc_param_ex.video_proc_path_src = UsbImgLink[idx].vproc_p_phy[j];
		vproc_param_ex.p_dim               = &sz;
		vproc_param_ex.pxlfmt              = HD_VIDEO_PXLFMT_YUV420;
		vproc_param_ex.dir                 = HD_VIDEO_DIR_NONE;
		vproc_param_ex.frc                 = HD_VIDEO_FRC_RATIO(1, 1);
		vproc_param_ex.pfunc               = &vproc_func_param_ex;
		memset((void *)&(vproc_param_ex.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
		if ((hd_ret = vproc_set_param_ex(&vproc_param_ex)) != HD_OK) {
			DBG_ERR("vproc_set_param_ex fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}

	// set videoenc
	IACOMM_VENC_CFG venc_cfg = {0};
	IACOMM_VENC_PARAM venc_param = {0};
	HD_VIDEOENC_FUNC_CONFIG usbimg_venc_func_cfg = {0};
	usbimg_venc_func_cfg.ddr_id = usbimg_dram_cfg.video_encode;
	usbimg_venc_func_cfg.in_func = 0;
	for (j = 0; j < 1; j++) {
		if ((hd_ret = venc_open_ch(UsbImgLink[idx].venc_i[j], UsbImgLink[idx].venc_o[j], &(UsbImgLink[idx].venc_p[j]))) != HD_OK) {
			DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		venc_cfg.video_enc_path = UsbImgLink[idx].venc_p[j];
		venc_cfg.ppath_cfg      = &(usbimg_venc_path_cfg[idx][j]);
		venc_cfg.pfunc_cfg      = &(usbimg_venc_func_cfg);
		if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
			DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		venc_param.video_enc_path = UsbImgLink[idx].venc_p[j];
		venc_param.pin            = &(usbimg_venc_in_param[idx][j]);
		venc_param.pout           = &(usbimg_venc_out_param[idx][j]);
		venc_param.prc            = &(usbimg_venc_rc_param[idx][j]);
		if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
			DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = hd_videoenc_start(UsbImgLink[idx].venc_p[j])) != HD_OK) {
			DBG_ERR("hd_videoenc_start fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = hd_videoenc_get(UsbImgLink[idx].venc_p[j], HD_VIDEOENC_PARAM_BUFINFO, &(usbimg_venc_bs_buf[idx][j]))) != HD_OK) {
			DBG_ERR("hd_videoenc_get fail(%d)\n", hd_ret);
			ret = E_SYS;
		} else {
			usbimg_venc_bs_buf_va[idx][j] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, usbimg_venc_bs_buf[idx][j].buf_info.phy_addr, usbimg_venc_bs_buf[idx][j].buf_info.buf_size);
		}
		if ((hd_ret = hd_videoenc_stop(UsbImgLink[idx].venc_p[j])) != HD_OK) {
			DBG_ERR("hd_videoenc_stop fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}

	// bind modules
	// notice: for direct mode, both vcap and vproc should be configured before bind,
	//         therefore move all the bind actions to the tail of function.
	if ((hd_ret = vcap_bind(UsbImgLink[idx].vcap_o[0], UsbImgLink[idx].vproc_i[0])) != HD_OK) {
		DBG_ERR("vcap_bind fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	for (j = 0; j < 1; j++) {
		if ((hd_ret = vproc_bind(UsbImgLink[idx].vproc_o[j], UsbImgLink[idx].venc_i[j])) != HD_OK) {
			DBG_ERR("vproc_bind fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
	return ret;
}

static ER _ImageApp_UsbMovie_ImgUnlink(UINT32 id)
{
	UINT32 idx = id, j;
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	// set videocap
	if ((hd_ret = vcap_unbind(UsbImgLink[idx].vcap_o[0])) != HD_OK) {
		DBG_ERR("vcap_unbind fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = vcap_close_ch(UsbImgLink[idx].vcap_p[0])) != HD_OK) {
		DBG_ERR("vcap_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}

	// set videoproc
	for (j = 0; j < 1; j++) {
		if ((hd_ret = vproc_unbind(UsbImgLink[idx].vproc_o[j])) != HD_OK) {
			DBG_ERR("vproc_unbind fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = vproc_close_ch(UsbImgLink[idx].vproc_p[j])) != HD_OK) {
			DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
	for (j = 0; j < 2; j++) {
		if ((hd_ret = vproc_close_ch(UsbImgLink[idx].vproc_p_phy[j])) != HD_OK) {
			DBG_ERR("vproc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}

	// set videoenc
	for (j = 0; j < 1; j++) {
		if ((hd_ret = venc_close_ch(UsbImgLink[idx].venc_p[j])) != HD_OK) {
			DBG_ERR("venc_close_ch fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}

	// reset variable
	for (j = 0; j < 1; j++) {
		g_UsbMovie_TBR_Max[idx][j] = 0;
		usbimg_venc_buf_ms[idx][j] = 0;
	}

	return ret;
}

ER _ImageApp_UsbMovie_ImgLinkStatusUpdate(UINT32 id, UINT32 direction)
{
	UINT32 idx = id;
	ER ret = E_OK;

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	if (direction == UPDATE_FORWARD) {
		// notice: if use direct mode, both start/stop should follow the sequence vproc -> vap
		// set videocap (if != direct mode)
		if (usbimg_vcap_func_cfg[idx].out_func != HD_VIDEOCAP_OUTFUNC_DIRECT) {
			ret |= _LinkUpdateStatus(vcap_set_state,      UsbImgLink[idx].vcap_p[0],       &(UsbImgLinkStatus[idx].vcap_p[0]),        NULL);
		}
		// set videoproc
		ret |= _LinkUpdateStatus(vproc_set_state,     UsbImgLink[idx].vproc_p_phy[0],  &(UsbImgLinkStatus[idx].vproc_p_phy[0]),   NULL);
		ret |= _LinkUpdateStatus(vproc_set_state,     UsbImgLink[idx].vproc_p_phy[1],  &(UsbImgLinkStatus[idx].vproc_p_phy[1]),   NULL);
		ret |= _LinkUpdateStatus(vproc_set_state,     UsbImgLink[idx].vproc_p[0],      &(UsbImgLinkStatus[idx].vproc_p[0]),       NULL);
		// set videocap (if == direct mode)
		if (usbimg_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) {
			ret |= _LinkUpdateStatus(vcap_set_state,      UsbImgLink[idx].vcap_p[0],       &(UsbImgLinkStatus[idx].vcap_p[0]),        NULL);
		}
		// set videoenc
		ret |= _LinkUpdateStatus(venc_set_state,      UsbImgLink[idx].venc_p[0],       &(UsbImgLinkStatus[idx].venc_p[0]),        NULL);
	} else {
		// set videoenc
		ret |= _LinkUpdateStatus(venc_set_state,      UsbImgLink[idx].venc_p[0],       &(UsbImgLinkStatus[idx].venc_p[0]),        NULL);
		// set videoproc
		ret |= _LinkUpdateStatus(vproc_set_state,     UsbImgLink[idx].vproc_p_phy[0],  &(UsbImgLinkStatus[idx].vproc_p_phy[0]),   NULL);
		ret |= _LinkUpdateStatus(vproc_set_state,     UsbImgLink[idx].vproc_p_phy[1],  &(UsbImgLinkStatus[idx].vproc_p_phy[1]),   NULL);
		ret |= _LinkUpdateStatus(vproc_set_state,     UsbImgLink[idx].vproc_p[0],      &(UsbImgLinkStatus[idx].vproc_p[0]),       NULL);
		// set videocap
		ret |= _LinkUpdateStatus(vcap_set_state,      UsbImgLink[idx].vcap_p[0],       &(UsbImgLinkStatus[idx].vcap_p[0]),        NULL);
	}
	return ret;
}

ER _ImageApp_UsbMovie_ImgLinkOpen(UINT32 id)
{
	UINT32 i = id, idx = id;

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	if (IsUsbImgLinkOpened[idx] == TRUE) {
		DBG_ERR("UsbMovie%d is already opened.\r\n", idx);
		return E_SYS;
	}

	_ImageApp_UsbMovie_ImgLinkCfg(i);

	_ImageApp_UsbMovie_SetupRecParam(i);

	_ImageApp_UsbMovie_ImgLink(i);

	IsUsbImgLinkOpened[idx] = TRUE;

	return E_OK;
}

ER _ImageApp_UsbMovie_ImgLinkClose(UINT32 id)
{
	UINT32 i = id, idx = id, j;

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	if (IsUsbImgLinkOpened[idx] == FALSE) {
		DBG_ERR("UsbMovie%d is already closed.\r\n", i);
		return E_SYS;
	}

	_ImageApp_UsbMovie_ImgUnlink(i);

	// reset variables
	memset(&UsbImgLinkStatus[idx], 0, sizeof(USBMOVIE_IMAGE_LINK_STATUS));

	usbimg_max_imgsize[idx].w = 0;
	usbimg_max_imgsize[idx].h = 0;
	usbimg_vcap_patgen[idx] = 0;
	usbimg_vcap_out_size[idx].w = 0;
	usbimg_vcap_out_size[idx].h = 0;
	UsbImgIPLUserSize[idx].size.w = 0;
	UsbImgIPLUserSize[idx].size.h = 0;
	UsbImgIPLUserSize[idx].fps = 0;
	usbimg_jpg_quality.jpg_quality = 50;
	usbimg_jpg_quality.buf_size = 0xffffffff;
#if defined(_BSP_NA51089_)
	usbimg_vcap_sie_remap[idx] = 0;
#endif
	for (j = 0; j < 1; j++) {
		if (usbimg_venc_bs_buf_va[idx][j]) {
			hd_common_mem_munmap((void *)usbimg_venc_bs_buf_va[idx][j], usbimg_venc_bs_buf[idx][j].buf_info.buf_size);
			usbimg_venc_bs_buf_va[idx][j] = 0;
			memset(&(usbimg_venc_bs_buf[idx][j]), 0, sizeof(HD_VIDEOENC_BUFINFO));
		}
	}
	IsUsbImgLinkOpened[idx] = FALSE;

	return E_OK;
}

