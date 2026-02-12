#include "ImageApp_Common_int.h"

/// ========== audiodec area ==========
HD_RESULT adec_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_audiodec_open(_in_id, _out_id, p_path_id);
}

HD_RESULT adec_close_ch(HD_PATH_ID path_id)
{
	return hd_audiodec_close(path_id);
}

HD_RESULT adec_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	return hd_audiodec_bind(_out_id, _dest_in_id);
}

HD_RESULT adec_unbind(HD_OUT_ID _out_id)
{
	return hd_audiodec_unbind(_out_id);
}

HD_RESULT adec_set_cfg(IACOMM_ADEC_CFG *padec_cfg)
{
	return hd_audiodec_set(padec_cfg->audio_dec_path, HD_AUDIODEC_PARAM_PATH_CONFIG, (VOID *)(padec_cfg->pcfg));
}

HD_RESULT adec_set_param(IACOMM_ADEC_PARAM *padec_param)
{
	HD_RESULT hd_ret = HD_OK;

	if ((hd_ret = hd_audiodec_set(padec_param->audio_dec_path, HD_AUDIODEC_PARAM_IN, (VOID *)(padec_param->pin_param))) != HD_OK) {
		return hd_ret;
	}
	return hd_ret;
}

HD_RESULT adec_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if ((hd_ret = (state == STATE_RUN)? hd_audiodec_start(path_id) : hd_audiodec_stop(path_id)) != HD_OK) {
		DBG_ERR("adec_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT adec_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID adec_ctrl = 0;

	if ((hd_ret = hd_audiodec_open(0, HD_AUDIODEC_CTRL(id), &adec_ctrl)) != HD_OK) {
		DBG_ERR("hd_audiodec_open(HD_ADEC_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = adec_ctrl;

	return hd_ret;
}

