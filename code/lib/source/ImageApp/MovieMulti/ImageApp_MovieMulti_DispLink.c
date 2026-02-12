#include "ImageApp_MovieMulti_int.h"

///// Cross file variables
UINT32 IsDispLinkOpened[MAX_DISP_PATH] = {0};
UINT32 IsDispLinkStarted[MAX_DISP_PATH] = {0};
MOVIE_DISP_LINK DispLink[MAX_DISP_PATH] = {0};
MOVIE_DISP_LINK_STATUS DispLinkStatus[MAX_DISP_PATH] = {0};
UINT32 g_DispFps[MAX_DISP_PATH] = {30};
UINT32 g_DispMirrorFlip[MAX_DISP_PATH] = {0};
MOVIE_RAWPROC_CB  g_DispCb = NULL;
HD_URECT user_disp_win[MAX_DISP_PATH] = {0};
HD_VIDEOOUT_FUNC_CONFIG disp_func_cfg[MAX_DISP_PATH] = {0};
UINT32 disp_queue_depth[MAX_DISP_PATH] = {2};
///// Noncross file variables
static UINT32 disp_tsk_run[MAX_DISP_PATH], is_disp_tsk_running[MAX_DISP_PATH];
static THREAD_HANDLE iamovie_disp_tsk_id;

#define PRI_IAMOVIE_DISP             7
#define STKSIZE_IAMOVIE_DISP         8192

ER _ImageApp_MovieMulti_DispLinkCfg(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}

	// vdoout
	// VdoOut flow refine: vout is opened in project
	#if 0
	DispLink[idx].vout_i[0] = HD_VIDEOOUT_IN (idx, 0);
	DispLink[idx].vout_o[0] = HD_VIDEOOUT_OUT(idx, 0);
	#endif

	return E_OK;
}

static ER _ImageApp_MovieMulti_DispLink(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;
	HD_RESULT hd_ret;
	HD_URECT win;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_DISP_ID_1;

	if (DispLink[idx].vout_p_ctrl == 0 || DispLink[idx].vout_p[0] == 0) {
		DBG_ERR("Disp path is not config\r\n");
		return E_SYS;
	}

	disp_func_cfg[idx].ddr_id = dram_cfg.video_out;

	// VdoOut flow refine: vout is opened in project
	#if 0
	// set videoout
	IACOMM_VOUT_CFG vout_cfg = {
		.p_video_out_ctrl = &(DispLink[idx].vout_p_ctrl),
		.out_type         = 1,
		.hdmi_id          = HD_VIDEOOUT_HDMI_1920X1080P30,
	};
	if ((ret = vout_set_cfg(&vout_cfg)) != HD_OK) {
		DBG_ERR("vout_set_cfg fail=%d\n", ret);
		return E_SYS;
	}
	if ((ret = vout_open_ch(DispLink[idx].vout_i[0], DispLink[idx].vout_o[0], &(DispLink[idx].vout_p[0]))) != HD_OK) {
		return E_SYS;
	}
	#endif

	if ((hd_ret = vout_get_caps(DispLink[idx].vout_p_ctrl, &(DispLink[idx].vout_syscaps))) != HD_OK) {
		DBG_ERR("vout_get_caps fail(%d)\n", hd_ret);
		return E_SYS;
	}

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
		return E_SYS;
	}
	return E_OK;
}

static ER _ImageApp_MovieMulti_DispUnlink(MOVIE_CFG_REC_ID id)
{
	// VdoOut flow refine: vout is opened in project
	#if 0
	UINT32 i = id, idx;
	HD_RESULT ret;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_DISP_ID_1;

	// set videoout
	if ((ret = vout_close_ch(DispLink[idx].vout_p[0])) != HD_OK) {
		DBG_ERR("vout_close_ch fail=%d\n", ret);
		return E_SYS;
	}
	#endif
	return E_OK;
}

THREAD_RETTYPE _ImageApp_MovieMulti_DispTsk(void)
{
	VOS_TICK t1, t2, t_diff;
	THREAD_ENTRY();

	is_disp_tsk_running[0] = TRUE;
	while (disp_tsk_run[0]) {
		if (g_DispCb == NULL) {
			DBG_ERR("disp rawproc cb is not registered.\r\n");
			disp_tsk_run[0] = FALSE;
			break;
		}
		vos_perf_mark(&t1);
		g_DispCb();
		vos_perf_mark(&t2);
		t_diff = (t2 - t1) / 1000;
		if (t_diff > IAMOVIE_COMM_CB_TIMEOUT_CHECK) {
			DBG_WRN("ImageApp_MovieMulti DispCB run %dms > %dms!\r\n", t_diff, IAMOVIE_COMM_CB_TIMEOUT_CHECK);
		}
	}
	is_disp_tsk_running[0] = FALSE;

	THREAD_RETURN(0);
}

ER _ImageApp_MovieMulti_DispLinkOpen(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_DISP_ID_1;

	if (IsDispLinkOpened[idx] == TRUE) {
		DBG_ERR("Disp path %d is already opened!\r\n", i);
		return E_SYS;
	}

	_ImageApp_MovieMulti_LinkCfg();

	_ImageApp_MovieMulti_DispLink(i);

	IsDispLinkOpened[idx] = TRUE;
	disp_tsk_run[idx] = FALSE;
	is_disp_tsk_running[idx] = FALSE;

	return E_OK;
}

ER _ImageApp_MovieMulti_DispLinkClose(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;
	HD_RESULT hd_ret;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_DISP_ID_1;

	if (IsDispLinkOpened[idx] == FALSE) {
		DBG_ERR("Disp path %d is already closed.\r\n", i);
		return E_SYS;
	}
	if (is_disp_tsk_running[idx]) {
		DBG_WRN("Disp task still running, force stop.\r\n");
		ImageApp_MovieMulti_DispStop(id);
	}

	if ((hd_ret = vout_get_caps(DispLink[idx].vout_p_ctrl, &(DispLink[idx].vout_syscaps))) != HD_OK) {
		DBG_ERR("vout_get_caps fail(%d)\n", hd_ret);
	}


	_ImageApp_MovieMulti_DispUnlink(i);

	memset(g_DispMirrorFlip, 0, sizeof(g_DispMirrorFlip));
	memset(user_disp_win, 0, sizeof(user_disp_win));
	disp_queue_depth[idx] = 2;

	IsDispLinkOpened[idx] = FALSE;
	return E_OK;
}

static void vout_status_cb(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if (state == CB_STATE_BEFORE_RUN) {
		VENDOR_VIDEOOUT_IN vout_in = {0};
		vout_in.queue_depth = disp_queue_depth[0];
		if ((hd_ret = vendor_videoout_set(path_id, VENDOR_VIDEOOUT_ITEM_PARAM_IN, &vout_in)) != HD_OK) {
			DBG_ERR("vendor_videoout_set(VENDOR_VIDEOOUT_IN) fail: id=%d, state=%d, ret=%d\r\n", path_id, state, hd_ret);
		}
	}
}

ER _ImageApp_MovieMulti_DispLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction)
{
	UINT32 i = id, idx;
	ER ret = E_OK;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_DISP_ID_1;

	// set videoout
	ret |= _LinkUpdateStatus(vout_set_state, DispLink[idx].vout_p[0], &(DispLinkStatus[idx].vout_p[0]), vout_status_cb);

	return ret;
}

ER ImageApp_MovieMulti_DispStart(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;
	ER ret;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_DISP_ID_1;

	if (IsDispLinkOpened[idx] == FALSE) {
		DBG_ERR("DispLink not opend.\r\n");
		return E_SYS;
	}
	if (IsDispLinkStarted[idx] == TRUE) {
		DBG_ERR("DispLink is already started.\r\n");
		return E_SYS;
	}

	_LinkPortCntInc(&(DispLinkStatus[idx].vout_p[0]));
	ret = _ImageApp_MovieMulti_DispLinkStatusUpdate(i, UPDATE_REVERSE);

	disp_tsk_run[idx] = TRUE;
	if ((iamovie_disp_tsk_id = vos_task_create(_ImageApp_MovieMulti_DispTsk, 0, "IAMovieDispTsk", PRI_IAMOVIE_DISP, STKSIZE_IAMOVIE_DISP)) == 0) {
		DBG_ERR("IAMovieDispTsk create failed.\r\n");
	} else {
		vos_task_resume(iamovie_disp_tsk_id);
	}

	IsDispLinkStarted[idx] = TRUE;

	return ret;
}

ER ImageApp_MovieMulti_DispStop(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx, delay_cnt;
	ER ret = E_OK;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}
	idx = i - _CFG_DISP_ID_1;

	if (IsDispLinkOpened[idx] == FALSE) {
		DBG_ERR("DispLink not opend.\r\n");
		return E_SYS;
	}
	if (IsDispLinkStarted[idx] == FALSE) {
		DBG_ERR("DispLink is not started!\r\n");
		return E_SYS;
	}

	disp_tsk_run[idx] = FALSE;       // set this flag and task will return by itself, no need to destroy

	delay_cnt = 50;
	while (is_disp_tsk_running[0] && delay_cnt) {
		vos_util_delay_ms(10);
		delay_cnt --;
	}
	if (is_disp_tsk_running[0] == TRUE) {
		DBG_WRN("Destroy DispTsk\r\n");
		vos_task_destroy(iamovie_disp_tsk_id);
	}

	_LinkPortCntDec(&(DispLinkStatus[idx].vout_p[0]));
	ret = _ImageApp_MovieMulti_DispLinkStatusUpdate(i, UPDATE_FORWARD);

	IsDispLinkStarted[idx] = FALSE;

	return ret;
}

ER _ImageApp_MovieMulti_DispGetSize(MOVIE_CFG_REC_ID id, USIZE *sz)
{
	UINT32 i = id, idx;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		DBG_ERR("ID %d is out of range!\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_DISP_ID_1;

	if (IsDispLinkOpened[idx] == FALSE) {
		sz->w = 0;
		sz->h = 0;
	} else {
		sz->w = DispLink[idx].vout_syscaps.output_dim.w;
		sz->h = DispLink[idx].vout_syscaps.output_dim.h;
	}
	return E_OK;
}

HD_RESULT ImageApp_MovieMulti_DispPullOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	UINT32 i = id, idx;

	if (i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) {
		idx = i - _CFG_REC_ID_1;
		if (img_vproc_no_ext[idx] == FALSE) {
			hd_ret = hd_videoproc_pull_out_buf(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_DISP], p_video_frame, wait_ms);
		} else {
			UINT32 iport;
			if (gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP] == UNUSED) {
				return HD_ERR_NOT_AVAIL;
			}
			iport = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP]];
			if (iport > 2) {
				iport = 2;
			}
			hd_ret = hd_videoproc_pull_out_buf(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][iport], p_video_frame, wait_ms);
		}
	} else if (i >= _CFG_ETHCAM_ID_1 && i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink)) {
		idx = i - _CFG_ETHCAM_ID_1;
		hd_ret = hd_videodec_pull_out_buf(EthCamLink[idx].vdec_p[0], p_video_frame, wait_ms);
	} else {
		DBG_ERR("id%d is out of range.\r\n", i);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}

HD_RESULT ImageApp_MovieMulti_DispPushIn(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 i = id, idx;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink) || (i >= (_CFG_DISP_ID_1 + MAX_DISP_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return HD_ERR_NG;
	}
	idx = i - _CFG_DISP_ID_1;

	return hd_videoout_push_in_buf(DispLink[idx].vout_p[0], p_video_frame, NULL, wait_ms);
}

HD_RESULT ImageApp_MovieMulti_DispReleaseOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	UINT32 i = id, idx;

	if (i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) {
		idx = i - _CFG_REC_ID_1;
		if (p_img_to_imgcap_frame && (idx == img_to_imgcap_id) && (img_to_imgcap_path == IAMOVIE_VPRC_EX_DISP)) {
			memcpy((void *)p_img_to_imgcap_frame, (void*)p_video_frame, sizeof(HD_VIDEO_FRAME));
			p_img_to_imgcap_frame = NULL;
			img_to_imgcap_id = UNUSED;
			img_to_imgcap_path = UNUSED;
			vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_FRM_WAIT);
			return HD_OK;
		} else {
			if (img_vproc_no_ext[idx] == FALSE) {
				hd_ret = hd_videoproc_release_out_buf(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_DISP], p_video_frame);
			} else {
				UINT32 iport;
				if (gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP] == UNUSED) {
					return HD_ERR_NOT_AVAIL;
				}
				iport = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP]];
				if (iport > 2) {
					iport = 2;
				}
				hd_ret = hd_videoproc_release_out_buf(ImgLink[idx].vproc_p_phy[img_vproc_splitter[idx]][iport], p_video_frame);
			}
		}
	} else if (i >= _CFG_ETHCAM_ID_1 && i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink)) {
		idx = i - _CFG_ETHCAM_ID_1;
		hd_ret = hd_videodec_release_out_buf(EthCamLink[idx].vdec_p[0], p_video_frame);
	} else {
		DBG_ERR("id%d is out of range.\r\n", i);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}

HD_RESULT ImageApp_MovieMulti_DispWindowEnable(MOVIE_CFG_REC_ID id, UINT32 En)
{
	HD_RESULT hd_ret = HD_OK;
	UINT32 i = id, idx;

	if (i < (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) {
		idx = i - _CFG_DISP_ID_1;
		HD_URECT rect = {
			.x = 0,
			.y = 0,
			.w = DispLink[idx].vout_syscaps.output_dim.w,
			.h = DispLink[idx].vout_syscaps.output_dim.h,
		};
		IACOMM_VOUT_PARAM vout_param = {
			.video_out_path = DispLink[idx].vout_p[0],
			.p_rect         = En ? &(DispLink[idx].vout_win) : &rect,
			.enable         = TRUE,
			.dir            = (DispLink[idx].vout_dir | g_DispMirrorFlip[idx]),
			.pfunc_cfg      = &(disp_func_cfg[idx]),
		};

		if ((hd_ret = vout_set_param(&vout_param)) != HD_OK) {
			DBG_ERR("vout_set_param fail=%d\n", hd_ret);
		}
		if (ImageApp_MovieMulti_IsStreamRunning(i)) {
			if ((hd_ret = hd_videoout_start(DispLink[idx].vout_p[0])) != HD_OK) {
				DBG_ERR("hd_videoout_start fail: id=%d, ret=%d\r\n", DispLink[0].vout_p[0], hd_ret);
			}
		}
	}
	return hd_ret;
}

