#include "ImageApp_Common_int.h"

/// ========== audioout area ==========
HD_RESULT aout_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_audioout_open(_in_id, _out_id, p_path_id);
}

HD_RESULT aout_close_ch(HD_PATH_ID path_id)
{
	return hd_audioout_close(path_id);
}

HD_RESULT aout_set_cfg(IACOMM_AOUT_CFG *paout_cfg)
{
	HD_RESULT hd_ret;
	HD_PATH_ID audio_out_ctrl = 0;

	if ((hd_ret = hd_audioout_open(0, paout_cfg->ctrl_id, &audio_out_ctrl)) != HD_OK) {
		return hd_ret;
	}
	if (paout_cfg->pdev_cfg) {
		if ((hd_ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DEV_CONFIG, (VOID *)(paout_cfg->pdev_cfg))) != HD_OK) {
			return hd_ret;
		}
	}
	if (paout_cfg->pdrv_cfg) {
		if ((hd_ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, (VOID *)(paout_cfg->pdrv_cfg))) != HD_OK) {
			return hd_ret;
		}
	}
	if (paout_cfg->p_audio_out_ctrl) {
		*(paout_cfg->p_audio_out_ctrl) = audio_out_ctrl;
	}
	return hd_ret;
}

HD_RESULT aout_set_param(IACOMM_AOUT_PARAM *paout_param)
{
	HD_RESULT hd_ret;

	if (paout_param->pin_param) {
		if ((hd_ret = hd_audioout_set(paout_param->audio_out_path, HD_AUDIOOUT_PARAM_IN, (VOID *)(paout_param->pin_param))) != HD_OK) {
			DBG_ERR("hd_audioout_set(HD_AUDIOOUT_PARAM_IN) fail(%d)\r\n", hd_ret);
			return hd_ret;
		}
	}
	if (paout_param->pout_param) {
		if ((hd_ret = hd_audioout_set(paout_param->audio_out_path, HD_AUDIOOUT_PARAM_OUT, (VOID *)(paout_param->pout_param))) != HD_OK) {
			DBG_ERR("hd_audioout_set(HD_AUDIOOUT_PARAM_OUT) fail(%d)\r\n", hd_ret);
			return hd_ret;
		}
	}
	if (paout_param->p_vol) {
		if ((hd_ret = hd_audioout_set(paout_param->audio_out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, (VOID *)(paout_param->p_vol))) != HD_OK) {
			DBG_ERR("hd_audioout_set(HD_AUDIOOUT_PARAM_VOLUME) fail(%d)\r\n", hd_ret);
			return hd_ret;
		}
	}
	return hd_ret;
}

HD_RESULT aout_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if ((hd_ret = (state == STATE_RUN)? hd_audioout_start(path_id) : hd_audioout_stop(path_id)) != HD_OK) {
		DBG_ERR("audioout_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT aout_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID aout_ctrl = 0;

	if ((hd_ret = hd_audioout_open(0, HD_AUDIOOUT_CTRL(id), &aout_ctrl)) != HD_OK) {
		DBG_ERR("hd_audioout_open(HD_AOUT_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = aout_ctrl;

	return hd_ret;
}

