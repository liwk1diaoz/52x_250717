/*
    Sensor Interface DAL library - ctrl sensor driver

    Sensor Interface DAL library.

    @file       sen_ctrl_drv.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "sen_ctrl_drv_int.h"
#include "sen_id_map_int.h"

static INT32 sen_ctrl_drv_get_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *data);


static INT32 sendrv_chgfps(CTL_SEN_ID id, UINT32 fps)
{
	INT32 rt;

	if (!sen_ctrl_chk_add_ctx(id)) {
		rt = CTL_SEN_E_ID_OVFL;
	} else if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		rt = CTL_SEN_E_ID_OVFL;
	} else if (g_ctl_sen_init_obj[id]->drv_tab->chgfps == NULL) {
		rt = CTL_SEN_E_ID_OVFL;
	} else {
		rt = g_ctl_sen_init_obj[id]->drv_tab->chgfps(id, fps);
	}

	if (rt) {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_CHGFPS, 0);
	}

	return rt;
}

ER sendrv_get(CTL_SEN_ID id, CTL_SENDRV_CFGID sendrv_cfg_id, void *data)
{
	INT32 rt;
	UINT32 dbg_tag = (UINT32)hwclock_get_longcounter();

	if (!sen_ctrl_chk_add_ctx(id)) {
		rt = E_NOEXS;
	} else if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		rt = E_NOEXS;
	} else if (g_ctl_sen_init_obj[id]->drv_tab->get_cfg == NULL) {
		rt = E_NOEXS;
	} else {
		ctl_sen_set_proc_time_adv("get(drv)", sendrv_cfg_id, id, CTL_SEN_PROC_TIME_ITEM_ENTER, dbg_tag);
		rt = g_ctl_sen_init_obj[id]->drv_tab->get_cfg(id, sendrv_cfg_id, data);
		ctl_sen_set_proc_time_adv("get(drv)", sendrv_cfg_id, id, CTL_SEN_PROC_TIME_ITEM_EXIT, dbg_tag);
	}

	if (rt) {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_GET, sendrv_cfg_id);
	}

	return rt;
}

#define CTL_SEN_SENDRV_DBG_SET 0
#if CTL_SEN_SENDRV_DBG_SET
static void sendrv_set_log(CTL_SEN_ID id, CTL_SENDRV_CFGID sendrv_cfg_id, void *data)
{
	switch (sendrv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		DBG_IND("id %d,CTL_SENDRV_CFGID_SET_EXPT[data][0x%.8x]\r\n", id, (int)data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		DBG_IND("id %d,CTL_SENDRV_CFGID_SET_GAIN[data][0x%.8x]\r\n", id, (int)data);
		break;
	case CTL_SENDRV_CFGID_SET_FPS:
		DBG_IND("id %d,CTL_SENDRV_CFGID_SET_FPS[data][0x%.8x]\r\n", id, *(UINT32 *)data);
		break;
	case CTL_SENDRV_CFGID_SET_STATUS:
		DBG_IND("id %d,CTL_SENDRV_CFGID_SET_STATUS[data][0x%.8x]\r\n", id, *(CTL_SEN_STATUS *)data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		DBG_IND("id %d,CTL_SENDRV_CFGID_FLIP_TYPE[data][0x%.8x]\r\n", id, *(CTL_SEN_FLIP *)data);
		break;
	default:
		CTL_SEN_DBG_ERR("N.S. item 0x%.8x\r\n", (int)sendrv_cfg_id);
		break;

	}
}
#endif

static ER sendrv_set(CTL_SEN_ID id, CTL_SENDRV_CFGID sendrv_cfg_id, void *data)
{
	INT32 rt;
	UINT32 dbg_tag = (UINT32)hwclock_get_longcounter();

	if (!sen_ctrl_chk_add_ctx(id)) {
		rt = E_NOEXS;
	} else if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		rt = E_NOEXS;
	} else if (g_ctl_sen_init_obj[id]->drv_tab->set_cfg == NULL) {
		rt = E_NOEXS;
	} else {
#if CTL_SEN_SENDRV_DBG_SET
		sendrv_set_log(id, sendrv_cfg_id, data);
#endif
		ctl_sen_set_proc_time_adv("set(drv)", sendrv_cfg_id, id, CTL_SEN_PROC_TIME_ITEM_ENTER, dbg_tag);
		rt = g_ctl_sen_init_obj[id]->drv_tab->set_cfg(id, sendrv_cfg_id, data);
		ctl_sen_set_proc_time_adv("set(drv)", sendrv_cfg_id, id, CTL_SEN_PROC_TIME_ITEM_EXIT, dbg_tag);
	}

	if (rt) {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_SET, sendrv_cfg_id);
	}

	return rt;
}

static INT32 sen_chk_senmode(CTL_SEN_ID id, CTL_SEN_MODE senmode_in, CTL_SEN_MODE *senmode_rst)
{
	INT32 rt = 0;
	ER rt_drv;

	if (senmode_in == CTL_SEN_MODE_CUR) {
		if (g_ctl_sen_ctrl_obj[id]->cur_sen_mode != CTL_SEN_MODE_UNKNOWN) {
			*senmode_rst = g_ctl_sen_ctrl_obj[id]->cur_sen_mode;
		} else {
			CTL_SEN_DBG_WRN("cur sensor mode is unknown\r\n");
			rt = CTL_SEN_E_IN_PARAM;
			//*senmode_rst = CTL_SEN_MODE_1;
		}
	} else if (senmode_in == CTL_SEN_MODE_PWR) {
		*senmode_rst = senmode_in;
	} else {
		CTL_SENDRV_GET_ATTR_BASIC_PARAM drv_attr_basic_param;

		memset((void *)&drv_attr_basic_param, 0, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));

		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_BASIC, &drv_attr_basic_param);
		if (rt_drv == E_OK) {
			if (senmode_in < CTL_SEN_MODE_1 + drv_attr_basic_param.max_senmode) {
				*senmode_rst = senmode_in;
			} else {
				CTL_SEN_DBG_ERR("input senmode %d overflow\r\n", senmode_in);
				rt = CTL_SEN_E_IN_PARAM;
			}
		} else {
			CTL_SEN_DBG_ERR("get max senmode fail\r\n");
			rt = CTL_SEN_E_SENDRV_GET_FAIL;
		}
	}

	return rt;
}

static UINT32 sen_ctrl_drv_chk_fps(CTL_SEN_ID id, CTL_SEN_MODE mode, UINT32 frame_rate)
{
	ER rt_drv;
	CTL_SEN_GET_MODE_BASIC_PARAM mode_basic_param = {0};
	CTL_SEN_GET_ATTR_PARAM attr_param;
	UINT32 frame_rate_rst;

	frame_rate_rst = frame_rate;

	mode_basic_param.mode = mode;
	rt_drv = sen_ctrl_drv_get_cfg(id, CTL_SEN_CFGID_GET_MODE_BASIC, &mode_basic_param);
	memset((void *)&attr_param, 0, sizeof(CTL_SEN_GET_ATTR_PARAM));
	rt_drv |= sen_ctrl_drv_get_cfg(id, CTL_SEN_CFGID_GET_ATTR, &attr_param);

	if (rt_drv == E_OK) {
		if ((attr_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) == CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
			/* sensor driver surppot chg fps*/
			if (mode_basic_param.dft_fps < frame_rate) {
				frame_rate_rst = mode_basic_param.dft_fps;
			}
		} else {
			/* sensor driver not surppot chg fps*/
			if (mode_basic_param.dft_fps != frame_rate) {
				frame_rate_rst = mode_basic_param.dft_fps;
			}
		}

	} else {
		CTL_SEN_DBG_ERR("\r\n");
	}

	if (frame_rate_rst != frame_rate) {
		CTL_SEN_DBG_ERR("force fps %d -> %d\r\n", frame_rate, frame_rate_rst);
	}

	return frame_rate_rst;
}

static INT32 sen_ctrl_drv_pwr_ctrl(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag)
{
	if (g_ctl_sen_init_obj[id]->pwr_ctrl == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	ctl_sen_set_proc_time("pwr(drv)", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	g_ctl_sen_init_obj[id]->pwr_ctrl(id, flag, sen_uti_get_clken_cb(id));

	ctl_sen_set_proc_time("pwr(drv)", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	return CTL_SEN_E_OK;
}

static INT32 sen_ctrl_drv_open(CTL_SEN_ID id)
{
	ER rt_drv = E_OK;
	INT32 rt;

	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	if (g_ctl_sen_init_obj[id]->drv_tab->open == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	ctl_sen_set_proc_time("open(drv)", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt_drv |= g_ctl_sen_init_obj[id]->drv_tab->open(id);

	ctl_sen_set_proc_time("open(drv)", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	if (rt_drv == E_OK) {
		rt = CTL_SEN_E_OK;
	} else {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_OPEN, 0);
		rt = CTL_SEN_E_SENDRV_OP;
	}
	return rt;
}

static INT32 sen_ctrl_drv_close(CTL_SEN_ID id)
{
	ER rt_drv = E_OK;
	INT32 rt;

	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->close == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	ctl_sen_set_proc_time("close(drv)", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt_drv |= g_ctl_sen_init_obj[id]->drv_tab->close(id);

	ctl_sen_set_proc_time("close(drv)", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	if (rt_drv == E_OK) {
		rt = CTL_SEN_E_OK;
	} else {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_CLOSE, 0);
		rt = CTL_SEN_E_SENDRV_OP;
	}
	return rt;
}

static INT32 sen_ctrl_drv_sleep(CTL_SEN_ID id)
{
	ER rt_drv = E_OK;
	INT32 rt;

	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->sleep == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	ctl_sen_set_proc_time("slp(drv)", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt_drv |= g_ctl_sen_init_obj[id]->drv_tab->sleep(id);

	ctl_sen_set_proc_time("slp(drv)", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	if (rt_drv == E_OK) {
		rt = CTL_SEN_E_OK;
	} else {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_SLP, 0);
		rt = CTL_SEN_E_SENDRV_OP;
	}
	return rt;
}

static INT32 sen_ctrl_drv_wakeup(CTL_SEN_ID id)
{
	ER rt_drv = E_OK;
	INT32 rt;

	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->wakeup == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	ctl_sen_set_proc_time("wakeup(drv)", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt_drv |= g_ctl_sen_init_obj[id]->drv_tab->wakeup(id);

	ctl_sen_set_proc_time("wakeup(drv)", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	if (rt_drv == E_OK) {
		rt = CTL_SEN_E_OK;
	} else {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_WUP, 0);
		rt = CTL_SEN_E_SENDRV_OP;
	}
	return rt;
}

static INT32 sen_ctrl_drv_write(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	ER rt_drv = E_OK;
	INT32 rt;

	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->write == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	rt_drv |= g_ctl_sen_init_obj[id]->drv_tab->write(id, cmd);

	if (rt_drv == E_OK) {
		rt = CTL_SEN_E_OK;
	} else {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_WR, 0);
		rt = CTL_SEN_E_SENDRV_OP;
	}
	return rt;
}

static INT32 sen_ctrl_drv_read(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	ER rt_drv = E_OK;
	INT32 rt;

	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->read == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	rt_drv |= g_ctl_sen_init_obj[id]->drv_tab->read(id, cmd);

	if (rt_drv == E_OK) {
		rt = CTL_SEN_E_OK;
	} else {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_RR, 0);
		rt = CTL_SEN_E_SENDRV_OP;
	}
	return rt;
}

static INT32 sen_ctrl_drv_chgmode(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj, CTL_SEN_MODE *chg_sen_mode_rst)
{
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	CTL_SENDRV_CHGMODE_OBJ drv_chgmode_obj = {0};

	ER rt_drv = E_OK;
	INT32 rt = 0;
	BOOL sendrv_get_error = FALSE;

	if (g_ctl_sen_init_obj[id] == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->chgmode == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_AUTO) {
		CTL_SENDRV_GET_MODESEL_PARAM drv_modesel_param = {0};

		drv_modesel_param.frame_rate = chgmode_obj.auto_info.frame_rate;
		drv_modesel_param.frame_num = chgmode_obj.auto_info.frame_num;
		drv_modesel_param.size = chgmode_obj.auto_info.size;
		drv_modesel_param.if_type = init_cfg_obj.if_cfg.type;
		drv_modesel_param.data_fmt = chgmode_obj.auto_info.data_fmt;
		drv_modesel_param.pixdepth = chgmode_obj.auto_info.pixdepth;
		drv_modesel_param.ccir = chgmode_obj.auto_info.ccir;
		drv_modesel_param.mux_singnal_en = chgmode_obj.auto_info.mux_singnal_en;
		drv_modesel_param.mux_signal_info = chgmode_obj.auto_info.mux_signal_info;
		drv_modesel_param.mode_type_sel = chgmode_obj.auto_info.mode_type_sel;
		drv_modesel_param.data_lane = chgmode_obj.auto_info.data_lane;
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODESEL, &drv_modesel_param);
		if (rt_drv == E_OK) {
			drv_chgmode_obj.mode = drv_modesel_param.mode;
		} else {
			drv_chgmode_obj.mode = CTL_SEN_MODE_1;
			CTL_SEN_DBG_ERR("sensor auto sel mode fail\r\n");
			sendrv_get_error = TRUE;
		}
		drv_chgmode_obj.frame_rate = chgmode_obj.auto_info.frame_rate;
	} else if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_MANUAL) {
		drv_chgmode_obj.mode = chgmode_obj.manual_info.sen_mode;
		drv_chgmode_obj.frame_rate = chgmode_obj.manual_info.frame_rate;
	}

	drv_chgmode_obj.frame_rate = sen_ctrl_drv_chk_fps(id, drv_chgmode_obj.mode, drv_chgmode_obj.frame_rate);

	ctl_sen_set_proc_time("chgmode(drv)", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt_drv = g_ctl_sen_init_obj[id]->drv_tab->chgmode(id, drv_chgmode_obj);

	ctl_sen_set_proc_time("chgmode(drv)", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	*chg_sen_mode_rst = drv_chgmode_obj.mode;

	if (sendrv_get_error) {
		rt = CTL_SEN_E_SENDRV_GET_FAIL;
	} else if (rt_drv != E_OK) {
		ctl_sen_set_er_sendrv(id, CTL_SEN_ER_OP_CHG, 0);
		rt = CTL_SEN_E_SENDRV_OP;
	}

	return rt;
}


static INT32 sen_ctrl_drv_chgfps(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	ER rt_drv = E_OK;
	INT32 rt = 0;
	UINT32 frame_rate;
	BOOL in_param_error = FALSE;

	if (g_ctl_sen_init_obj[id] == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->chgfps == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_AUTO) {
		frame_rate = chgmode_obj.auto_info.frame_rate;
	} else if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_MANUAL) {
		frame_rate = chgmode_obj.manual_info.frame_rate;
	} else {
		CTL_SEN_DBG_ERR("\r\n");
		frame_rate = 30 * 100;
		in_param_error = TRUE;
	}

	frame_rate = sen_ctrl_drv_chk_fps(id, g_ctl_sen_ctrl_obj[id]->cur_sen_mode, frame_rate);

	ctl_sen_set_proc_time("chgfps(drv)", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt_drv |= sendrv_chgfps(id, frame_rate);

	ctl_sen_set_proc_time("chgfps(drv)", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	if (rt_drv != E_OK) {
		rt = CTL_SEN_E_SENDRV_OP;
	}
	if (in_param_error) {
		rt = CTL_SEN_E_IN_PARAM;
	}

	return rt;
}

static INT32 sen_ctrl_drv_set_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *data)
{
	ER rt_drv = E_OK;
	INT32 rt = 0;

	if (g_ctl_sen_init_obj[id] == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->set_cfg == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	if (cfg_id == CTL_SEN_CFGID_SET_EXPT) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_SET_EXPT, data);
	}

	if (cfg_id == CTL_SEN_CFGID_SET_GAIN) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_SET_GAIN, data);
	}

	if (cfg_id == CTL_SEN_CFGID_SET_FPS) {
		UINT32 fps;

		fps = sen_ctrl_drv_chk_fps(id, g_ctl_sen_ctrl_obj[id]->cur_sen_mode, *(UINT32 *)data);
		rt_drv = sendrv_chgfps(id, fps);
	}

	if (cfg_id == CTL_SEN_CFGID_SET_STATUS) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_SET_STATUS, (CTL_SEN_STATUS *)data);
	}

	if (cfg_id == CTL_SEN_CFGID_FLIP_TYPE) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_FLIP_TYPE, (CTL_SEN_FLIP *)data);
	}

	if (cfg_id == CTL_SEN_CFGID_AD_ID_MAP) {
		CTL_SENDRV_AD_ID_MAP_PARAM drv_id_map_param;
		CTL_SEN_AD_ID_MAP_PARAM *id_map_param = (CTL_SEN_AD_ID_MAP_PARAM *)data;

		memset((void *)&drv_id_map_param, 0, sizeof(CTL_SENDRV_AD_ID_MAP_PARAM));

		drv_id_map_param.chip_id = id_map_param->chip_id;
		drv_id_map_param.vin_id = id_map_param->vin_id;
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_AD_ID_MAP, &drv_id_map_param);
		if (rt_drv == E_NOSPT) {
			// only ad sendrv need to set
			rt_drv = E_OK;
		}
	}

	if (cfg_id == CTL_SEN_CFGID_AD_TYPE) {
		CTL_SENDRV_AD_IMAGE_PARAM drv_param;
		CTL_SEN_AD_IMAGE_PARAM *user_param = (CTL_SEN_AD_IMAGE_PARAM *)data;

		memset((void *)&drv_param, 0, sizeof(CTL_SENDRV_AD_IMAGE_PARAM));

		drv_param.vin_id = user_param->vin_id;
		drv_param.val = user_param->val;
		drv_param.reserved = user_param->reserved;
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_AD_TYPE, &drv_param);
		if (rt_drv == E_NOSPT) {
			// only ad sendrv need to set
			rt_drv = E_OK;
		}
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE1) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_USER_DEFINE1, data);
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE2) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_USER_DEFINE2, data);
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE3) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_USER_DEFINE3, data);
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE4) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_USER_DEFINE4, data);
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE5) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_USER_DEFINE5, data);
	}

	if (cfg_id == CTL_SEN_CFGID_AD_INIT) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_AD_INIT, data);
		if (rt_drv == E_NOSPT) {
			// only ad sendrv need to set
			rt_drv = E_OK;
		}
	}

	if (cfg_id == CTL_SEN_CFGID_AD_UNINIT) {
		rt_drv = sendrv_set(id, CTL_SENDRV_CFGID_AD_UNINIT, data);
		if (rt_drv == E_NOSPT) {
			// only ad sendrv need to set
			rt_drv = E_OK;
		}
	}

	if (rt_drv == E_NOSPT) {
		CTL_SEN_DBG_WRN("sensor driver not support CTL_SEN_CFGID 0x%.8x\r\n", cfg_id);
		rt = CTL_SEN_E_SENDRV_NS;
	} else if (rt_drv != E_OK) {
		CTL_SEN_DBG_WRN("sensor driver set CTL_SEN_CFGID 0x%.8x fail\r\n", cfg_id);
		rt = CTL_SEN_E_SENDRV_SET_FAIL;
	}

	return rt;
}

static INT32 sen_ctrl_drv_get_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *data)
{
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	CTL_SEN_PLUG_IN plug_in = g_ctl_sen_init_obj[id]->det_plug_in;

	ER rt_drv = E_OK;
	INT32 rt = 0, rt_2 = 0;
	UINT32 i;
	BOOL in_param_error = FALSE, sendrv_error_nospt = FALSE, sendrv_error = FALSE;

	if (g_ctl_sen_init_obj[id] == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk init ctl_sensor first\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}
	if (g_ctl_sen_init_obj[id]->drv_tab->get_cfg == NULL) {
		CTL_SEN_DBG_ERR("OBJ NULL, pls chk sensor driver\r\n");
		return CTL_SEN_E_SENDRV_TBL_NULL;
	}

	if (cfg_id == CTL_SEN_CFGID_GET_EXPT) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_EXPT, data);
	}

	if (cfg_id == CTL_SEN_CFGID_GET_GAIN) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_GAIN, data);
	}

	if (cfg_id == CTL_SEN_CFGID_GET_ATTR) {
		CTL_SENDRV_GET_ATTR_CMDIF_PARAM drv_attr_cmdif_param;
		CTL_SENDRV_GET_ATTR_BASIC_PARAM drv_attr_basic_param;
		CTL_SENDRV_GET_ATTR_SIGNAL_PARAM drv_attr_signal_param;
		CTL_SEN_GET_ATTR_PARAM *attr_param = (CTL_SEN_GET_ATTR_PARAM *)data;

		memset((void *)&drv_attr_cmdif_param, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
		memset((void *)&drv_attr_basic_param, 0, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
		memset((void *)&drv_attr_signal_param, 0, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));

		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &drv_attr_cmdif_param);
		if (rt_drv == E_OK) {
			attr_param->cmdif_type = drv_attr_cmdif_param.type;
		} else if (rt_drv == E_NOSPT) {
			sendrv_error_nospt = TRUE;
		} else {
			sendrv_error = TRUE;
		}

		attr_param->if_type = init_cfg_obj.if_cfg.type;
		attr_param->drvdev = init_cfg_obj.drvdev;
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_BASIC, &drv_attr_basic_param);
		if (rt_drv == E_OK) {
			for (i = 0; i < CTL_SEN_NAME_LEN; i++) {
				attr_param->name[i] = drv_attr_basic_param.name[i];
			}
			attr_param->vendor = drv_attr_basic_param.vendor;
			attr_param->max_senmode = drv_attr_basic_param.max_senmode;
			attr_param->property = drv_attr_basic_param.property;
			attr_param->sync_timing = drv_attr_basic_param.sync_timing;
		} else if (rt_drv == E_NOSPT) {
			sendrv_error_nospt = TRUE;
		} else {
			sendrv_error = TRUE;
		}

		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_SIGNAL, &drv_attr_signal_param);
		if (rt_drv == E_OK) {
			attr_param->signal_type = drv_attr_signal_param.type;
			attr_param->signal_info = drv_attr_signal_param.info;
		} else if (rt_drv == E_NOSPT) {
			sendrv_error_nospt = TRUE;
		} else {
			sendrv_error = TRUE;
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_CMDIF) {
		CTL_SENDRV_GET_ATTR_CMDIF_PARAM drv_attr_cmdif_param;
		CTL_SEN_GET_CMDIF_PARAM *cmdif_param = (CTL_SEN_GET_CMDIF_PARAM *)data;

		memset((void *)&drv_attr_cmdif_param, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));

		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &drv_attr_cmdif_param);
		if (rt_drv == E_OK) {
			cmdif_param->type = drv_attr_cmdif_param.type;
			if (cmdif_param->type == CTL_SEN_CMDIF_TYPE_VX1) {
				if (init_cfg_obj.cmd_if_cfg.vx1.en) {
					cmdif_param->vx1.if_sel = init_cfg_obj.cmd_if_cfg.vx1.if_sel;
					cmdif_param->vx1.tx_type = init_cfg_obj.cmd_if_cfg.vx1.tx_type;
				} else {
					CTL_SEN_DBG_ERR("CTL_SEN_INIT_CFG_OBJ not enable vx1\r\n");
					in_param_error = TRUE;
				}
			}
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_IF) {
		CTL_SEN_GET_IF_PARAM *if_param = (CTL_SEN_GET_IF_PARAM *)data;
		CTL_SENDRV_GET_MODE_PARA_PARAM drv_mode_para_param = {0};

		if_param->type = init_cfg_obj.if_cfg.type;
		rt = sen_chk_senmode(id, if_param->mode, &drv_mode_para_param.mode);
		if (rt == CTL_SEN_E_OK) {
			if (if_param->type == CTL_SEN_IF_TYPE_PARALLEL) {
				if_param->info.parallel.mux_info.mux_data_num = 1; // initial value
				rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_PARA, &drv_mode_para_param);
				if (rt_drv ==  E_OK) {
					if_param->info.parallel.mux_info = drv_mode_para_param.mux_info;
				}
			}
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_DVI) {
		CTL_SEN_GET_DVI_PARAM *dvi_param = (CTL_SEN_GET_DVI_PARAM *)data;
		CTL_SENDRV_GET_MODE_DVI_PARAM drv_mode_dvi_param = {0};

		rt = sen_chk_senmode(id, dvi_param->mode, &drv_mode_dvi_param.mode);
		if (rt == CTL_SEN_E_OK) {
			rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_DVI, &drv_mode_dvi_param);
			if (rt_drv == E_OK) {
				dvi_param->fmt = drv_mode_dvi_param.fmt;
				dvi_param->data_mode = drv_mode_dvi_param.data_mode;
				dvi_param->msblsb_switch = init_cfg_obj.pin_cfg.ccir_msblsb_switch;
				if ((init_cfg_obj.pin_cfg.ccir_vd_hd_pin == FALSE) && ((dvi_param->fmt == CTL_SEN_DVI_CCIR601) || (dvi_param->fmt == CTL_SEN_DVI_CCIR709) || (dvi_param->fmt == CTL_SEN_DVI_CCIR601_1120))) {
					CTL_SEN_DBG_ERR("CCIR601 init obj pin must set ccir_vd_hd_pin\r\n");
					in_param_error = TRUE;
				}
			}
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_TEMP) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_TEMP, (UINT32 *)data);
	}

	if (cfg_id == CTL_SEN_CFGID_GET_FPS) {
		CTL_SEN_GET_FPS_PARAM *fps_param = (CTL_SEN_GET_FPS_PARAM *)data;
		CTL_SENDRV_GET_FPS_PARAM drv_fps_param = {0};
		CTL_SENDRV_GET_MODE_BASIC_PARAM drv_mode_basic_param = {0};

		/* initial */
		fps_param->dft_fps = CTL_SEN_IGNORE;
		fps_param->chg_fps = CTL_SEN_IGNORE;
		fps_param->cur_fps = CTL_SEN_IGNORE;

		if (g_ctl_sen_ctrl_obj[id]->cur_sen_mode == CTL_SEN_MODE_UNKNOWN) {
			sendrv_error_nospt = TRUE;
		} else {
			drv_mode_basic_param.mode = g_ctl_sen_ctrl_obj[id]->cur_sen_mode;
			rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_BASIC, &drv_mode_basic_param);
			if (rt_drv == E_OK) {
				fps_param->dft_fps = drv_mode_basic_param.dft_fps;
			} else if (rt_drv == E_NOSPT) {
				sendrv_error_nospt = TRUE;
			} else {
				sendrv_error = TRUE;
			}
		}

		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_FPS, &drv_fps_param);
		if (rt_drv == E_OK) {
			fps_param->chg_fps = drv_fps_param.chg_fps;
			fps_param->cur_fps = drv_fps_param.cur_fps;
		} else if (rt_drv == E_NOSPT) {
			sendrv_error_nospt = TRUE;
		} else {
			sendrv_error = TRUE;
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_CLK_INFO) {
		CTL_SEN_GET_CLK_INFO_PARAM *clk_info_param = (CTL_SEN_GET_CLK_INFO_PARAM *)data;
		CTL_SENDRV_GET_SPEED_PARAM drv_speed_param = {0};
		CTL_SEN_MODE mode;

		/* check input sensor mode */
		rt = sen_chk_senmode(id, clk_info_param->mode, &mode);
		if (rt != CTL_SEN_E_OK) {
			in_param_error = TRUE;
			goto err;
		}

		/* get sendrv speed info */
		drv_speed_param.mode = mode;
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_SPEED, &drv_speed_param);
		if (rt_drv == E_OK) {
			clk_info_param->mclk_freq = drv_speed_param.mclk;
			clk_info_param->data_rate = drv_speed_param.data_rate;
			clk_info_param->pclk = drv_speed_param.pclk;
		}
		clk_info_param->mclk_sel = sen_conv_mclksel(id, drv_speed_param.mclk_src);
		clk_info_param->mclk_src_sel = sen_uti_get_mclk_src(sen_conv_mclksel(id, drv_speed_param.mclk_src));
	}

	if (cfg_id == CTL_SEN_CFGID_GET_PLUG) {
		BOOL *plug_param = (BOOL *)data;

		if (plug_in == NULL) {
			CTL_SEN_DBG_ERR("dx not implement plug in detect\r\n");
			*plug_param = FALSE;
			rt_drv = E_NOSPT;
		} else {
			*plug_param = plug_in(id);
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_MODESEL) {
		CTL_SEN_GET_MODESEL_PARAM *modesel_param = (CTL_SEN_GET_MODESEL_PARAM *)data;
		CTL_SENDRV_GET_MODESEL_PARAM drv_modesel_param = {0};

		drv_modesel_param.frame_rate = modesel_param->frame_rate;
		drv_modesel_param.size = modesel_param->size;
		drv_modesel_param.if_type = init_cfg_obj.if_cfg.type;
		drv_modesel_param.data_fmt = modesel_param->data_fmt;
		drv_modesel_param.frame_num = modesel_param->frame_num;
		drv_modesel_param.pixdepth = modesel_param->pixdepth;
		drv_modesel_param.ccir = modesel_param->ccir;
		drv_modesel_param.mux_singnal_en = modesel_param->mux_singnal_en;
		drv_modesel_param.mux_signal_info = modesel_param->mux_signal_info;
		drv_modesel_param.mode_type_sel = modesel_param->mode_type_sel;
		drv_modesel_param.data_lane = modesel_param->data_lane;
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODESEL, &drv_modesel_param);
		if (rt_drv == E_OK) {
			modesel_param->mode = drv_modesel_param.mode;
		} else {
			CTL_SEN_DBG_ERR("sensor not support auto sel mode\r\n");
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_MODE_BASIC) {
		CTL_SEN_GET_MODE_BASIC_PARAM *mode_basic_param = (CTL_SEN_GET_MODE_BASIC_PARAM *)data;
		CTL_SENDRV_GET_MODE_BASIC_PARAM drv_mode_basic_param = {0};
		CTL_SENDRV_GET_MODE_ROWTIME_PARAM drv_mode_rowtime_param = {0};

		rt = sen_chk_senmode(id, mode_basic_param->mode, &drv_mode_basic_param.mode);
		if (rt == CTL_SEN_E_OK) {
			rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_BASIC, &drv_mode_basic_param);
			if (rt_drv == E_OK) {
				mode_basic_param->data_fmt = drv_mode_basic_param.data_fmt;
				mode_basic_param->mode_type = drv_mode_basic_param.mode_type;
				mode_basic_param->dft_fps = drv_mode_basic_param.dft_fps;
				mode_basic_param->frame_num = drv_mode_basic_param.frame_num;
				mode_basic_param->stpix = drv_mode_basic_param.stpix;
				mode_basic_param->pixel_depth = drv_mode_basic_param.pixel_depth;
				mode_basic_param->frame_num = drv_mode_basic_param.frame_num;
				mode_basic_param->valid_size = drv_mode_basic_param.valid_size;
				for (i = 0; i < CTL_SEN_MFRAME_MAX_NUM; i++) {
					mode_basic_param->act_size[i] = drv_mode_basic_param.act_size[i];
				}
				mode_basic_param->crp_size = drv_mode_basic_param.crp_size;
				mode_basic_param->signal_info = drv_mode_basic_param.signal_info;
				mode_basic_param->ratio_h_v = drv_mode_basic_param.ratio_h_v;
				mode_basic_param->gain = drv_mode_basic_param.gain;
				mode_basic_param->bining_ratio = drv_mode_basic_param.bining_ratio;
			} else {
				CTL_SEN_DBG_ERR("get sendrv MODE_BASIC fail\r\n");
			}

			// will not return error code, avoid sendrv not implement
			// set for debug only
			drv_mode_rowtime_param.mode = mode_basic_param->mode;
			rt_2 = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_ROWTIME, &drv_mode_rowtime_param);
			if (rt_2 == E_OK) {
				mode_basic_param->row_time = drv_mode_rowtime_param.row_time;
				mode_basic_param->row_time_step = drv_mode_rowtime_param.row_time_step;
			} else {
				CTL_SEN_DBG_ERR("get sendrv MODE_ROWTIME fail\r\n");
				ctl_sen_set_er(id, CTL_SEN_ER_OP_GET, CTL_SEN_ER_ITEM_SENDRV, rt_2);

			}
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_PLUG_INFO) {
		CTL_SEN_GET_PLUG_INFO_PARAM *probe_sen_param = (CTL_SEN_GET_PLUG_INFO_PARAM *)data;
		CTL_SENDRV_GET_PLUG_INFO_PARAM drv_probe_sen_param = {0};

		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_PLUG_INFO, &drv_probe_sen_param);
		if (rt_drv == E_OK) {
			probe_sen_param->size = drv_probe_sen_param.size;
			probe_sen_param->fps = drv_probe_sen_param.fps;
			probe_sen_param->interlace = drv_probe_sen_param.interlace;
			for (i = 0; i < CTL_SEN_PLUG_PARAM_NUM; i++) {
				probe_sen_param->param[i] = drv_probe_sen_param.param[i];
			}
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_PROBE_SEN) {
		CTL_SEN_GET_PROBE_SEN_PARAM *probe_sen_param = (CTL_SEN_GET_PROBE_SEN_PARAM *)data;
		CTL_SENDRV_GET_PROBE_SEN_PARAM drv_probe_sen_param = {0};

		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_PROBE_SEN, &drv_probe_sen_param);
		if (rt_drv == E_OK) {
			probe_sen_param->probe_rst = drv_probe_sen_param.probe_rst;
		}
	}

	if (cfg_id == CTL_SEN_CFGID_GET_SENDRV_VER) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_SENDRV_VER, (UINT32 *)data);
	}

	if (cfg_id == CTL_SEN_CFGID_FLIP_TYPE) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_FLIP_TYPE, (CTL_SEN_FLIP *)data);
	}


	if (cfg_id == CTL_SEN_CFGID_GET_MFR_OUTPUT_TIMING) {
		CTL_SEN_GET_MFR_OUTPUT_TIMING_PARAM *sen_param = (CTL_SEN_GET_MFR_OUTPUT_TIMING_PARAM *)data;
		CTL_SENDRV_GET_MFR_OUTPUT_TIMING_PARAM drv_sen_param = {0};

		if (sen_param->diff_row == NULL || sen_param->diff_row_vd == NULL) {
			rt_drv = E_SYS;
			CTL_SEN_DBG_ERR("input par NULL\r\n");
		} else {
			rt = sen_chk_senmode(id, sen_param->mode, &drv_sen_param.mode);
			if (rt == E_OK) {
				drv_sen_param.frame_rate = sen_param->frame_rate;
				drv_sen_param.num = sen_param->num;
				drv_sen_param.diff_row = sen_param->diff_row;
				drv_sen_param.diff_row_vd = sen_param->diff_row_vd;
				if (drv_sen_param.num > (CTL_SEN_MFRAME_MAX_NUM - 1)) {
					DBG_ERR("MFR_OUTPUT_TIMING num %d ovfl > %d\r\n", drv_sen_param.num, (CTL_SEN_MFRAME_MAX_NUM - 1));
					drv_sen_param.num = (CTL_SEN_MFRAME_MAX_NUM - 1);
				}
				rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_MFR_OUTPUT_TIMING, &drv_sen_param);
			}
		}
	}

	if (cfg_id == CTL_SEN_CFGID_AD_ID_MAP) {
		CTL_SENDRV_AD_ID_MAP_PARAM drv_id_map_param;
		CTL_SEN_AD_ID_MAP_PARAM *id_map_param = (CTL_SEN_AD_ID_MAP_PARAM *)data;

		memset((void *)&drv_id_map_param, 0, sizeof(CTL_SENDRV_AD_ID_MAP_PARAM));

		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_AD_ID_MAP, &drv_id_map_param);
		if (rt_drv == E_OK) {
			id_map_param->chip_id = drv_id_map_param.chip_id;
			id_map_param->vin_id = drv_id_map_param.vin_id;
		}
	}

	if (cfg_id == CTL_SEN_CFGID_AD_TYPE) {
		CTL_SENDRV_AD_IMAGE_PARAM drv_param;
		CTL_SEN_AD_IMAGE_PARAM *user_param = (CTL_SEN_AD_IMAGE_PARAM *)data;

		memset((void *)&drv_param, 0, sizeof(CTL_SENDRV_AD_IMAGE_PARAM));

		drv_param.vin_id = user_param->vin_id;
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_AD_TYPE, &drv_param);
		if (rt_drv == E_OK) {
			user_param->val = drv_param.val;
			user_param->reserved = drv_param.reserved;
		}
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE1) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_USER_DEFINE1, data);
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE2) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_USER_DEFINE2, data);
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE3) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_USER_DEFINE3, data);
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE4) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_USER_DEFINE4, data);
	}

	if (cfg_id == CTL_SEN_CFGID_USER_DEFINE5) {
		rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_USER_DEFINE5, data);
	}

err:
	/* check error */
	if ((rt_drv == E_NOSPT) || (sendrv_error_nospt)) {
		CTL_SEN_DBG_WRN("sensor driver not support CTL_SEN_CFGID 0x%.8x\r\n", cfg_id);
		rt = CTL_SEN_E_SENDRV_NS;
	} else if ((rt_drv != E_OK) || (sendrv_error)) {
		CTL_SEN_DBG_WRN("sensor driver get CTL_SEN_CFGID 0x%.8x fail\r\n", cfg_id);
		rt = CTL_SEN_E_SENDRV_GET_FAIL;
	} else if (in_param_error) {
		rt = CTL_SEN_E_IN_PARAM;
	}

	return rt;
}

static CTL_SEN_CTRL_DRV sen_ctrl_drv_tab = {
	sen_ctrl_drv_pwr_ctrl,

	sen_ctrl_drv_open,
	sen_ctrl_drv_close,
	sen_ctrl_drv_sleep,
	sen_ctrl_drv_wakeup,
	sen_ctrl_drv_write,
	sen_ctrl_drv_read,
	sen_ctrl_drv_chgmode,
	sen_ctrl_drv_chgfps,
	sen_ctrl_drv_set_cfg,
	sen_ctrl_drv_get_cfg,

	sendrv_set,
};


CTL_SEN_CTRL_DRV *sen_ctrl_drv_get_obj(void)
{
	return &sen_ctrl_drv_tab;
}

