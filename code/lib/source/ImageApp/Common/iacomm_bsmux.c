#include "ImageApp_Common_int.h"

/// ========== bsmux area ==========
HD_RESULT bsmux_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_bsmux_open(_in_id, _out_id, p_path_id);
}

HD_RESULT bsmux_close_ch(HD_PATH_ID path_id)
{
	return hd_bsmux_close(path_id);
}

HD_RESULT bsmux_set_cfg(IACOMM_BSMUX_CFG *pbsmux_cfg)
{
	return hd_bsmux_set(pbsmux_cfg->path_id, pbsmux_cfg->id, pbsmux_cfg->p_param);
}

HD_RESULT bsmux_set_param(IACOMM_BSMUX_PARAM *pbsmux_param)
{
	return hd_bsmux_set(pbsmux_param->path_id, pbsmux_param->id, pbsmux_param->p_param);
}

HD_RESULT bsmux_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if ((hd_ret = (state == STATE_RUN)? hd_bsmux_start(path_id) : hd_bsmux_stop(path_id)) != HD_OK) {
		DBG_ERR("bsmux_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT bsmux_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID bsmux_ctrl = 0;

	if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(id), &bsmux_ctrl)) != HD_OK) {
		DBG_ERR("hd_bsmux_open(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = bsmux_ctrl;

	return hd_ret;
}

