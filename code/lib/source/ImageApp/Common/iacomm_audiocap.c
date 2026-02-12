#include "ImageApp_Common_int.h"

/// ========== audiocap area ==========
HD_RESULT acap_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_audiocap_open(_in_id, _out_id, p_path_id);
}

HD_RESULT acap_close_ch(HD_PATH_ID path_id)
{
	return hd_audiocap_close(path_id);
}

HD_RESULT acap_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	return hd_audiocap_bind(_out_id, _dest_in_id);
}

HD_RESULT acap_unbind(HD_OUT_ID _out_id)
{
	return hd_audiocap_unbind(_out_id);
}

HD_RESULT acap_set_cfg(IACOMM_ACAP_CFG *pacap_cfg)
{
	HD_RESULT hd_ret;
	HD_PATH_ID audio_cap_ctrl = 0;

	if ((hd_ret = hd_audiocap_open(0, pacap_cfg->ctrl_id, &audio_cap_ctrl)) != HD_OK) {
		return hd_ret;
	}
	if ((hd_ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, (VOID *)(pacap_cfg->pdev_cfg))) != HD_OK) {
		return hd_ret;
	}
	if ((hd_ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, (VOID *)(pacap_cfg->pdrv_cfg))) != HD_OK) {
		return hd_ret;
	}
	*(pacap_cfg->p_audio_cap_ctrl) = audio_cap_ctrl;

	return hd_ret;
}

HD_RESULT acap_set_param(IACOMM_ACAP_PARAM *pacap_param)
{
	return hd_audiocap_set(pacap_param->audio_cap_path, HD_AUDIOCAP_PARAM_IN, (VOID *)(pacap_param->pcap_param));
}

HD_RESULT acap_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if ((hd_ret = (state == STATE_RUN)? hd_audiocap_start(path_id) : hd_audiocap_stop(path_id)) != HD_OK) {
		DBG_ERR("audiocap_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT acap_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID acap_ctrl = 0;

	if ((hd_ret = hd_audiocap_open(0, HD_AUDIOCAP_CTRL(id), &acap_ctrl)) != HD_OK) {
		DBG_ERR("hd_audiocap_open(HD_ACAP_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = acap_ctrl;

	return hd_ret;
}

