#include "ImageApp_Common_int.h"

/// ========== videoout area ==========
HD_RESULT vout_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	return hd_videoout_open(_in_id, _out_id, p_path_id);
}

HD_RESULT vout_close_ch(HD_PATH_ID path_id)
{
	return hd_videoout_close(path_id);
}

HD_RESULT vout_set_cfg(IACOMM_VOUT_CFG *pvout_cfg)
{
	HD_RESULT hd_ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	if ((hd_ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl)) != HD_OK) {
		return hd_ret;
	}

	printf("out_type=%d\r\n", pvout_cfg->out_type);
	switch(pvout_cfg->out_type){
	case 0:
		printf("open display with CVBS\r\n");
		videoout_mode.output_type       = HD_COMMON_VIDEO_OUT_CVBS;
		videoout_mode.input_dim         = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.cvbs  = HD_VIDEOOUT_CVBS_NTSC;
	break;
	case 1:
		printf("open display with LCD\r\n");
		videoout_mode.output_type       = HD_COMMON_VIDEO_OUT_LCD;
		videoout_mode.input_dim         = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.lcd   = HD_VIDEOOUT_LCD_0;
	break;
	case 2:
		printf("open display with HDMI\r\n");
		videoout_mode.output_type       = HD_COMMON_VIDEO_OUT_HDMI;
		videoout_mode.input_dim         = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.hdmi  = pvout_cfg->hdmi_id;
	break;
	default:
		printf("not support out_type\r\n");
	break;
	}
	if ((hd_ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &videoout_mode)) != HD_OK) {
		DBG_ERR("set HD_VIDEOOUT_PARAM_MODE fail(%d)\r\n", hd_ret);
		return hd_ret;
	}
	*(pvout_cfg->p_video_out_ctrl) = video_out_ctrl;

	return hd_ret;
}

HD_RESULT vout_set_param(IACOMM_VOUT_PARAM *pvout_param)
{
	HD_RESULT hd_ret, result = HD_OK;
	HD_VIDEOOUT_IN video_out_param = {0};
	HD_VIDEOOUT_WIN_ATTR video_out_param2 = {0};

	video_out_param.dim.w       = pvout_param->p_rect->w;
	video_out_param.dim.h       = pvout_param->p_rect->h;
	video_out_param.pxlfmt      = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir         = pvout_param->dir;
	if ((hd_ret = hd_videoout_set(pvout_param->video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param)) != HD_OK) {
		DBG_ERR("hd_videoout_set fail(%d)\r\n", hd_ret);
	}

	if (((pvout_param->dir & HD_VIDEO_DIR_ROTATE_270) == HD_VIDEO_DIR_ROTATE_270) || ((pvout_param->dir & HD_VIDEO_DIR_ROTATE_90) == HD_VIDEO_DIR_ROTATE_90)) {
		video_out_param2.visible    = pvout_param->enable;
		video_out_param2.rect.x     = pvout_param->p_rect->y;
		video_out_param2.rect.y     = pvout_param->p_rect->x;
		video_out_param2.rect.w     = pvout_param->p_rect->h;
		video_out_param2.rect.h     = pvout_param->p_rect->w;
		video_out_param2.layer      = HD_LAYER1;
	} else {
		video_out_param2.visible    = pvout_param->enable;
		video_out_param2.rect.x     = pvout_param->p_rect->x;
		video_out_param2.rect.y     = pvout_param->p_rect->y;
		video_out_param2.rect.w     = pvout_param->p_rect->w;
		video_out_param2.rect.h     = pvout_param->p_rect->h;
		video_out_param2.layer      = HD_LAYER1;
	}
	if ((hd_ret = hd_videoout_set(pvout_param->video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param2)) != HD_OK) {
		DBG_ERR("hd_videoout_set(HD_VIDEOOUT_PARAM_IN_WIN_ATTR) fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	if ((hd_ret = hd_videoout_set(pvout_param->video_out_path, HD_VIDEOOUT_PARAM_FUNC_CONFIG, pvout_param->pfunc_cfg)) != HD_OK) {
		DBG_ERR("hd_videoout_set(HD_VIDEOOUT_PARAM_FUNC_CONFIG) fail(%d)\r\n", hd_ret);
		result = HD_ERR_NG;
	}
	return result;
}

HD_RESULT vout_set_state(HD_PATH_ID path_id, UINT32 state)
{
#if 1
	HD_RESULT hd_ret;
	VENDOR_VIDEOOUT_FUNC_CONFIG videoout_cfg = {0};

    //set vout stop would keep last frame for change mode
    videoout_cfg.in_func = VENDOR_VIDEOOUT_INFUNC_KEEP_LAST;
	if ((hd_ret = vendor_videoout_set(path_id, VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG, &videoout_cfg)) != HD_OK) {
		DBG_ERR("vendor_videoout_set(VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG) fail: id=%d, state=%d, ret=%d\r\n", path_id, state, hd_ret);
		return hd_ret;
	}
	if ((hd_ret = (state == STATE_RUN)? hd_videoout_start(path_id) : hd_videoout_stop(path_id)) != HD_OK) {
		DBG_ERR("vout_set_state fail: id=%d, state=%d, ret=%d\r\n", path_id, state, hd_ret);
	}
	return hd_ret;
#else
	HD_RESULT hd_ret = HD_OK;
	if (state == STATE_RUN) {
		if ((hd_ret = hd_videoout_start(path_id)) != HD_OK) {
			DBG_ERR("vout_set_state fail: path_id=%x, state=%d, ret=%d\r\n", path_id, state, hd_ret);
		}
	}
	return ret;
#endif
}

HD_RESULT vout_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path)
{
	HD_RESULT hd_ret = HD_OK;
	HD_PATH_ID vout_ctrl = 0;

	if ((hd_ret = hd_videoout_open(0, HD_VIDEOOUT_CTRL(id), &vout_ctrl)) != HD_OK) {
		DBG_ERR("hd_videoout_open(HD_VOUT_CTRL) fail(%d)\n", hd_ret);
	}
	*p_ctrl_path = vout_ctrl;

	return hd_ret;
}

HD_RESULT vout_get_caps(HD_PATH_ID video_out_ctrl,HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps)
{
	HD_RESULT hd_ret = HD_OK;
    HD_DEVCOUNT video_out_dev = {0};

	if ((hd_ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_DEVCOUNT, &video_out_dev)) != HD_OK) {
		return hd_ret;
	}

	if ((hd_ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps)) != HD_OK) {
		return hd_ret;
	}
	return hd_ret;
}

