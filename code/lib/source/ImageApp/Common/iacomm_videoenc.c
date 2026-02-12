#include "ImageApp_Common_int.h"

/// ========== videoenc area ==========
HD_RESULT venc_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_videoenc_open(_in_id, _out_id, p_path_id);
}

HD_RESULT venc_close_ch(HD_PATH_ID path_id)
{
	return hd_videoenc_close(path_id);
}

HD_RESULT venc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	return hd_videoenc_bind(_out_id, _dest_in_id);
}

HD_RESULT venc_unbind(HD_OUT_ID _out_id)
{
	return hd_videoenc_unbind(_out_id);
}

HD_RESULT venc_set_cfg(IACOMM_VENC_CFG *pvenc_cfg)
{
	HD_RESULT hd_ret, result;

	if (pvenc_cfg->ppath_cfg || pvenc_cfg->pfunc_cfg) {
		result = HD_OK;
	} else {
		result = HD_ERR_PARAM;
	}

	if (pvenc_cfg->ppath_cfg) {
		if ((hd_ret = hd_videoenc_set(pvenc_cfg->video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, pvenc_cfg->ppath_cfg)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_PATH_CONFIG fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	if (pvenc_cfg->pfunc_cfg) {
		if ((hd_ret = hd_videoenc_set(pvenc_cfg->video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, pvenc_cfg->pfunc_cfg)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_FUNC_CONFIG fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	return result;
}

HD_RESULT venc_set_param(IACOMM_VENC_PARAM *pvenc_param)
{
	HD_RESULT hd_ret, result = HD_OK;

	//--- HD_VIDEOENC_PARAM_IN ---
	if (pvenc_param->pin) {
		if ((hd_ret = hd_videoenc_set(pvenc_param->video_enc_path, HD_VIDEOENC_PARAM_IN, pvenc_param->pin)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_IN fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	if (pvenc_param->pout) {
		if ((hd_ret = hd_videoenc_set(pvenc_param->video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, pvenc_param->pout)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_OUT_ENC_PARAM2 fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	if (pvenc_param->prc) {
		if ((hd_ret = hd_videoenc_set(pvenc_param->video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, pvenc_param->prc)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2 fail(%d)\r\n", hd_ret);
			result = HD_ERR_NG;
		}
	}
	return result;
}

HD_RESULT venc_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if ((hd_ret = (state == STATE_RUN)? hd_videoenc_start(path_id) : hd_videoenc_stop(path_id)) != HD_OK) {
		DBG_ERR("venc_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT venc_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID venc_ctrl = 0;

	if ((hd_ret = hd_videoenc_open(0, HD_VIDEOENC_CTRL(id), &venc_ctrl)) != HD_OK) {
		DBG_ERR("hd_videoenc_open(HD_VENC_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = venc_ctrl;

	return hd_ret;
}

