#include "ImageApp_Common_int.h"

/// ========== videoprocess area ==========
HD_RESULT vproc_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_videoproc_open(_in_id, _out_id, p_path_id);
}

HD_RESULT vproc_close_ch(HD_PATH_ID path_id)
{
	return hd_videoproc_close(path_id);
}

HD_RESULT vproc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	return hd_videoproc_bind(_out_id, _dest_in_id);
}

HD_RESULT vproc_unbind(HD_OUT_ID _out_id)
{
	return hd_videoproc_unbind(_out_id);
}

HD_RESULT vproc_set_cfg(IACOMM_VPROC_CFG *pvproc_cfg)
{
	HD_RESULT hd_ret, result = HD_OK;
	HD_PATH_ID video_proc_ctrl = 0;

	if ((hd_ret = hd_videoproc_open(0, pvproc_cfg->ctrl_id, &video_proc_ctrl)) != HD_OK) {
		return hd_ret;
	}
	if ((hd_ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, pvproc_cfg->pcfg)) != HD_OK) {
		DBG_ERR("set HD_VIDEOPROC_PARAM_DEV_CONFIG fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	if ((hd_ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, pvproc_cfg->pctrl)) != HD_OK) {
		DBG_ERR("set HD_VIDEOPROC_PARAM_CTRL fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	if ((hd_ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, pvproc_cfg->pfunc)) != HD_OK) {
		DBG_ERR("set HD_VIDEOPROC_PARAM_FUNC_CONFIG fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	*(pvproc_cfg->p_video_proc_ctrl) = video_proc_ctrl;

	return result;
}

HD_RESULT vproc_set_param_in(IACOMM_VPROC_PARAM_IN *pvproc_param_in)
{
	return hd_videoproc_set(pvproc_param_in->video_proc_path, HD_VIDEOPROC_PARAM_IN, pvproc_param_in->in_param);
}

HD_RESULT vproc_set_param(IACOMM_VPROC_PARAM *pvproc_param)
{
	HD_RESULT hd_ret = HD_ERR_NG;

	if (pvproc_param->p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = pvproc_param->p_dim->w;
		video_out_param.dim.h = pvproc_param->p_dim->h;
		video_out_param.pxlfmt = pvproc_param->pxlfmt;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = pvproc_param->frc;
		video_out_param.depth = 0;
		if ((hd_ret = hd_videoproc_set(pvproc_param->video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOPROC_PARAM_OUT fail(%d)\r\n", hd_ret);
		}
		if (pvproc_param->pfunc) {
			if ((hd_ret = hd_videoproc_set(pvproc_param->video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, pvproc_param->pfunc)) != HD_OK) {
				DBG_ERR("set HD_VIDEOPROC_PARAM_FUNC_CONFIG fail(%d)\r\n", hd_ret);
			}
		}
		HD_VIDEOPROC_CROP video_out_crop_param = {0};
		video_out_crop_param.mode = pvproc_param->video_proc_crop_param.mode;
		video_out_crop_param.win.rect.x = pvproc_param->video_proc_crop_param.win.rect.x;
		video_out_crop_param.win.rect.y = pvproc_param->video_proc_crop_param.win.rect.y;
		video_out_crop_param.win.rect.w = pvproc_param->video_proc_crop_param.win.rect.w;
		video_out_crop_param.win.rect.h = pvproc_param->video_proc_crop_param.win.rect.h;
		if ((hd_ret = hd_videoproc_set(pvproc_param->video_proc_path, HD_VIDEOPROC_PARAM_OUT_CROP, &video_out_crop_param)) != HD_OK) {
			DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT_CROP) fail(%d)\r\n", hd_ret);
			return hd_ret;
		}
	}
	return hd_ret;
}

HD_RESULT vproc_set_param_ex(IACOMM_VPROC_PARAM_EX *pvproc_param_ex)
{
	HD_RESULT hd_ret = HD_ERR_NG;

	if (pvproc_param_ex->p_dim != NULL) {
		HD_VIDEOPROC_OUT_EX video_out_param_ex = {0};
		video_out_param_ex.src_path = pvproc_param_ex->video_proc_path_src;
		if (pvproc_param_ex->dir == HD_VIDEO_DIR_ROTATE_90 || pvproc_param_ex->dir == HD_VIDEO_DIR_ROTATE_270) {
			video_out_param_ex.dim.w = pvproc_param_ex->p_dim->h;
			video_out_param_ex.dim.h = pvproc_param_ex->p_dim->w;
		} else {
			video_out_param_ex.dim.w = pvproc_param_ex->p_dim->w;
			video_out_param_ex.dim.h = pvproc_param_ex->p_dim->h;
		}
		video_out_param_ex.pxlfmt = pvproc_param_ex->pxlfmt;
		video_out_param_ex.dir = pvproc_param_ex->dir;
		video_out_param_ex.frc = pvproc_param_ex->frc;
		video_out_param_ex.depth = pvproc_param_ex->depth;
		if ((hd_ret = hd_videoproc_set(pvproc_param_ex->video_proc_path_ex, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param_ex)) != HD_OK) {
			DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT_EX) fail(%d)\r\n", hd_ret);
			return hd_ret;
		}

		HD_VIDEOPROC_CROP video_out_crop_param_ex = {0};
		video_out_crop_param_ex.mode = pvproc_param_ex->video_proc_crop_param.mode;
		video_out_crop_param_ex.win.rect.x = pvproc_param_ex->video_proc_crop_param.win.rect.x;
		video_out_crop_param_ex.win.rect.y = pvproc_param_ex->video_proc_crop_param.win.rect.y;
		video_out_crop_param_ex.win.rect.w = pvproc_param_ex->video_proc_crop_param.win.rect.w;
		video_out_crop_param_ex.win.rect.h = pvproc_param_ex->video_proc_crop_param.win.rect.h;
		if ((hd_ret = hd_videoproc_set(pvproc_param_ex->video_proc_path_ex, HD_VIDEOPROC_PARAM_OUT_EX_CROP, &video_out_crop_param_ex)) != HD_OK) {
			DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT_EX_CROP) fail(%d)\r\n", hd_ret);
			return hd_ret;
		}
	}
	if (pvproc_param_ex->pfunc) {
		if ((hd_ret = hd_videoproc_set(pvproc_param_ex->video_proc_path_ex, HD_VIDEOPROC_PARAM_FUNC_CONFIG, pvproc_param_ex->pfunc)) != HD_OK) {
			DBG_ERR("set HD_VIDEOPROC_PARAM_FUNC_CONFIG fail(%d)\r\n", hd_ret);
		}
	}
	return hd_ret;
}

HD_RESULT vproc_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	// workaround for vcap/vprc, since the depth info will be reset when path stop, the depth should be set again before start
	// however, only control variable in kflow is reset, the proc or get api still get original setting
	// so here we just get and set out param again
	if (state == STATE_RUN) {
		if ((HD_GET_OUT(path_id) - 1) < 5) {        // path 0~4 for physical path
			HD_VIDEOPROC_OUT vprc_out_param = {0};
			if ((hd_ret = hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
				DBG_ERR("Get HD_VIDEOPROC_PARAM_OUT fail(%d)\r\n", hd_ret);
			}
			if ((hd_ret = hd_videoproc_set(path_id, HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
				DBG_ERR("Set HD_VIDEOPROC_PARAM_OUT fail(%d)\r\n", hd_ret);
			}
		} else {                                    // path 5~ for extend path
			HD_VIDEOPROC_OUT_EX vprc_out_param_ex = {0};
			if ((hd_ret = hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_OUT_EX, &vprc_out_param_ex)) != HD_OK) {
				DBG_ERR("Get HD_VIDEOPROC_PARAM_OUT_EX fail(%d)\r\n", hd_ret);
			}
			if ((hd_ret = hd_videoproc_set(path_id, HD_VIDEOPROC_PARAM_OUT_EX, &vprc_out_param_ex)) != HD_OK) {
				DBG_ERR("Set HD_VIDEOPROC_PARAM_OUT_EX fail(%d)\r\n", hd_ret);
			}
		}
	}

	if ((hd_ret = (state == STATE_RUN)? hd_videoproc_start(path_id) : hd_videoproc_stop(path_id)) != HD_OK) {
		DBG_ERR("vproc_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT vproc_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID vproc_ctrl = 0;

	if ((hd_ret = hd_videoproc_open(0, HD_VIDEOPROC_CTRL(id), &vproc_ctrl)) != HD_OK) {
		DBG_ERR("hd_videoproc_open(HD_VPROC_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = vproc_ctrl;

	return hd_ret;
}

