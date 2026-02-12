#include "ImageApp_Common_int.h"

/// ========== fileout area ==========
HD_RESULT fileout_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_fileout_open(_in_id, _out_id, p_path_id);
}

HD_RESULT fileout_close_ch(HD_PATH_ID path_id)
{
	return hd_fileout_close(path_id);
}

HD_RESULT fileout_set_cfg(IACOMM_FILEOUT_CFG *pfileout_cfg)
{
	return hd_fileout_set(pfileout_cfg->path_id, pfileout_cfg->id, pfileout_cfg->p_param);
}

HD_RESULT fileout_set_param(IACOMM_FILEOUT_PARAM *pfileout_param)
{
	return hd_fileout_set(pfileout_param->path_id, pfileout_param->id, pfileout_param->p_param);
}

HD_RESULT fileout_set_state(HD_PATH_ID path_id, UINT32 state)
{
	HD_RESULT hd_ret;

	if ((hd_ret = (state == STATE_RUN)? hd_fileout_start(path_id) : hd_fileout_stop(path_id)) != HD_OK) {
		DBG_ERR("fileout_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
}

HD_RESULT fileout_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID fileout_ctrl = 0;

	if ((hd_ret = hd_fileout_open(0, HD_FILEOUT_CTRL(id), &fileout_ctrl)) != HD_OK) {
		DBG_ERR("hd_fileout_open(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = fileout_ctrl;

	return hd_ret;
}

