#include "ImageApp_Common_int.h"

/// ========== videocap area ==========
HD_RESULT vcap_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_videocap_open(_in_id, _out_id, p_path_id);
}

HD_RESULT vcap_close_ch(HD_PATH_ID path_id)
{
	return hd_videocap_close(path_id);
}

HD_RESULT vcap_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	return hd_videocap_bind(_out_id, _dest_in_id);
}

HD_RESULT vcap_unbind(HD_OUT_ID _out_id)
{
	return hd_videocap_unbind(_out_id);
}

HD_RESULT vcap_set_cfg(IACOMM_VCAP_CFG *pvcap_cfg)
{
	HD_RESULT hd_ret, result = HD_OK;
	HD_PATH_ID video_cap_ctrl = 0;

	if ((hd_ret = hd_videocap_open(0, pvcap_cfg->ctrl_id, &video_cap_ctrl)) != HD_OK) {
		DBG_ERR("hd_videocap_open() fail(%d)\r\n", hd_ret);
		return hd_ret;
	}
	if ((hd_ret = hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, pvcap_cfg->pcfg)) != HD_OK) {
		DBG_ERR("set HD_VIDEOCAP_PARAM_DRV_CONFIG fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	if((hd_ret = hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, pvcap_cfg->pctrl)) != HD_OK) {
		DBG_ERR("set HD_VIDEOCAP_PARAM_CTRL fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	*(pvcap_cfg->p_video_cap_ctrl) = video_cap_ctrl;

	return result;
}

HD_RESULT vcap_set_param(IACOMM_VCAP_PARAM *pvcap_param)
{
	HD_RESULT hd_ret, result = HD_OK;

	if (pvcap_param->pin_param) {
		if ((hd_ret = hd_videocap_set(pvcap_param->video_cap_path, HD_VIDEOCAP_PARAM_IN, pvcap_param->pin_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOCAP_PARAM_IN fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	if (pvcap_param->pcrop_param) {
		if ((hd_ret = hd_videocap_set(pvcap_param->video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, pvcap_param->pcrop_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	if (pvcap_param->pout_param) {
		if ((hd_ret = hd_videocap_set(pvcap_param->video_cap_path, HD_VIDEOCAP_PARAM_OUT, pvcap_param->pout_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	if (pvcap_param->pfunc_cfg) {
		if ((hd_ret = hd_videocap_set(pvcap_param->video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, pvcap_param->pfunc_cfg)) != HD_OK) {
			DBG_ERR("set HD_VIDEOCAP_PARAM_FUNC_CONFIG fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	if (pvcap_param->data_lane != 0) {
		if ((hd_ret = vendor_videocap_set(pvcap_param->video_cap_path, VENDOR_VIDEOCAP_PARAM_DATA_LANE, &(pvcap_param->data_lane))) != HD_OK) {
			DBG_ERR("set VENDOR_VIDEOCAP_PARAM_DATA_LANE fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	return result;
}

HD_RESULT vcap_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	// workaround for vcap/vprc, since the depth info will be reset when path stop, the depth should be set again before start
	// however, only control variable in kflow is reset, the proc or get api still get original setting
	// so here we just get and set out param again
	if (state == STATE_RUN) {
		HD_VIDEOCAP_OUT vcap_out_param = {0};
		if ((hd_ret = hd_videocap_get(path_id, HD_VIDEOCAP_PARAM_OUT, &vcap_out_param)) != HD_OK) {
			DBG_ERR("Get HD_VIDEOCAP_PARAM_OUT fail(%d)\r\n", hd_ret);
		}
		if ((hd_ret = hd_videocap_set(path_id, HD_VIDEOCAP_PARAM_OUT, &vcap_out_param)) != HD_OK) {
			DBG_ERR("Get HD_VIDEOCAP_PARAM_OUT fail(%d)\r\n", hd_ret);
		}
	}

	if ((hd_ret = (state == STATE_RUN)? hd_videocap_start(path_id) : hd_videocap_stop(path_id)) != HD_OK) {
		DBG_ERR("vcap_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT vcap_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID vcap_ctrl = 0;

	if ((hd_ret = hd_videocap_open(0, HD_VIDEOCAP_CTRL(id), &vcap_ctrl)) != HD_OK) {
		DBG_ERR("hd_videocap_open(HD_VCAP_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = vcap_ctrl;

	return hd_ret;
}

HD_RESULT vcap_get_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	return hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
}

