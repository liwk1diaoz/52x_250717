#include "ImageApp_Common_int.h"

/// ========== videodec area ==========
HD_RESULT vdec_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_videodec_open(_in_id, _out_id, p_path_id);
}

HD_RESULT vdec_close_ch(HD_PATH_ID path_id)
{
	return hd_videodec_close(path_id);
}

HD_RESULT vdec_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	return hd_videodec_bind(_out_id, _dest_in_id);
}

HD_RESULT vdec_unbind(HD_OUT_ID _out_id)
{
	return hd_videodec_unbind(_out_id);
}

HD_RESULT vdec_set_cfg(IACOMM_VDEC_CFG *pvdec_cfg)
{
	return hd_videodec_set(pvdec_cfg->video_dec_path, HD_VIDEODEC_PARAM_PATH_CONFIG, pvdec_cfg->pcfg);
}

HD_RESULT vdec_set_param(IACOMM_VDEC_PARAM *pvdec_param)
{
	HD_RESULT hd_ret, result = HD_OK;

	if ((hd_ret = hd_videodec_set(pvdec_param->video_dec_path, HD_VIDEODEC_PARAM_IN, pvdec_param->pin)) != HD_OK) {
		DBG_ERR("set HD_VIDEODEC_PARAM_IN fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	if ((hd_ret = hd_videodec_set(pvdec_param->video_dec_path, HD_VIDEODEC_PARAM_IN_DESC, pvdec_param->pdesc)) != HD_OK) {
		DBG_ERR("set HD_VIDEODEC_PARAM_IN_DESC fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	return result;
}

HD_RESULT vdec_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if ((hd_ret = (state == STATE_RUN)? hd_videodec_start(path_id) : hd_videodec_stop(path_id)) != HD_OK) {
		DBG_ERR("vdec_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT vdec_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID vdec_ctrl = 0;

	if ((hd_ret = hd_videodec_open(0, HD_VIDEODEC_CTRL(id), &vdec_ctrl)) != HD_OK) {
		DBG_ERR("hd_videodec_open(HD_VDEC_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = vdec_ctrl;

	return hd_ret;
}

