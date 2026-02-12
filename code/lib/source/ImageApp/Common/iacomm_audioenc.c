#include "ImageApp_Common_int.h"

/// ========== audioenc area ==========
HD_RESULT aenc_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_audioenc_open(_in_id, _out_id, p_path_id);
}

HD_RESULT aenc_close_ch(HD_PATH_ID path_id)
{
	return hd_audioenc_close(path_id);
}

HD_RESULT aenc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	return hd_audioenc_bind(_out_id, _dest_in_id);
}

HD_RESULT aenc_unbind(HD_OUT_ID _out_id)
{
	return hd_audioenc_unbind(_out_id);
}

HD_RESULT aenc_set_cfg(IACOMM_AENC_CFG *paenc_cfg)
{
	return hd_audioenc_set(paenc_cfg->audio_enc_path, HD_AUDIOENC_PARAM_PATH_CONFIG2, (VOID *)(paenc_cfg->pcfg));
}

HD_RESULT aenc_set_param(IACOMM_AENC_PARAM *paenc_param)
{
	HD_RESULT hd_ret = HD_OK;

	if ((hd_ret = hd_audioenc_set(paenc_param->audio_enc_path, HD_AUDIOENC_PARAM_IN, (VOID *)(paenc_param->pin_param))) != HD_OK) {
		return hd_ret;
	}
	if ((hd_ret = hd_audioenc_set(paenc_param->audio_enc_path, HD_AUDIOENC_PARAM_OUT, (VOID *)(paenc_param->pout_param))) != HD_OK) {
		return hd_ret;
	}
	return hd_ret;
}

HD_RESULT aenc_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if ((hd_ret = (state == STATE_RUN)? hd_audioenc_start(path_id) : hd_audioenc_stop(path_id)) != HD_OK) {
		DBG_ERR("aenc_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT aenc_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID aenc_ctrl = 0;

	if ((hd_ret = hd_audioenc_open(0, HD_AUDIOENC_CTRL(id), &aenc_ctrl)) != HD_OK) {
		DBG_ERR("hd_audioenc_open(HD_AENC_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = aenc_ctrl;

	return hd_ret;
}

