/*
    Sensor Interface library - crtl

    Sensor Interface library.

    @file       sen_ctrl.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "sen_ctrl_int.h"

INT32 sen_ctrl_init_sendrv(CTL_SEN_ID id)
{
	CTL_SEN_AD_ID_MAP_PARAM ad_id_map = {0};
	INT rt;

	ad_id_map.chip_id = g_ctl_sen_init_obj[id]->init_cfg_obj.sendrv_cfg.ad_id_map.chip_id;
	ad_id_map.vin_id = g_ctl_sen_init_obj[id]->init_cfg_obj.sendrv_cfg.ad_id_map.vin_id;

	rt = sen_ctrl_drv_get_obj()->set_cfg(id, CTL_SEN_CFGID_AD_ID_MAP, (void *)&ad_id_map);
	if ((rt != CTL_SEN_E_OK) && (rt != CTL_SEN_E_NS)) {
		CTL_SEN_DBG_ERR("set sendrv fail\r\n");
		ctl_sen_set_er(id, CTL_SEN_ER_OP_INIT, CTL_SEN_ER_ITEM_SENDRV, rt);
		return CTL_SEN_E_SENDRV;
	}

	return CTL_SEN_E_OK;

}

INT32 sen_ctrl_chk_support_type(CTL_SEN_ID id)
{
	return CTL_SEN_E_OK;
}

INT32 sen_ctrl_clk_prepare(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	INT32 rt;

	rt = sen_ctrl_if_glb_get_obj()->clk_prepare(id, mode);

	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d cfg clk fail\r\n", id);
		ctl_sen_set_er(id, CTL_SEN_ER_OP_PRE_CLK, CTL_SEN_ER_ITEM_SYS, rt);
		return CTL_SEN_E_CLK;
	}

	return CTL_SEN_E_OK;
}

INT32 sen_ctrl_clk_unprepare(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	INT32 rt;

	rt = sen_ctrl_if_glb_get_obj()->clk_unprepare(id, mode);

	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d clk_unprepare fail\r\n", id);
		ctl_sen_set_er(id, CTL_SEN_ER_OP_UNP_CLK, CTL_SEN_ER_ITEM_SYS, rt);
		return CTL_SEN_E_CLK;
	}
	return CTL_SEN_E_OK;
}

INT32 sen_ctrl_mclksync_init(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	INT32 rt;

	rt = sen_ctrl_if_glb_get_obj()->mclksync_init(id, mode);

	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d mclksync_init fail\r\n", id);
		ctl_sen_set_er(id, CTL_SEN_ER_OP_PWR, CTL_SEN_ER_ITEM_SYS, rt);
		return CTL_SEN_E_CLK;
	}
	return CTL_SEN_E_OK;
}

INT32 sen_ctrl_mclksync_uninit(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	INT32 rt;

	rt = sen_ctrl_if_glb_get_obj()->mclksync_uninit(id, mode);

	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d mclksync_uninit fail\r\n", id);
		ctl_sen_set_er(id, CTL_SEN_ER_OP_PWR, CTL_SEN_ER_ITEM_SYS, rt);
		return CTL_SEN_E_CLK;
	}
	return CTL_SEN_E_OK;
}

INT sen_ctrl_chgmclk(CTL_SEN_ID id, CTL_SEN_MODE mode_curr, CTL_SEN_MODE mode_next)
{
	INT32 rt = CTL_SEN_E_OK;

	UINT32 mclk_curr, mclk_next;
	CTL_SEN_CLK_SEL mclksel_curr, mclksel_next;

	if (mode_curr == CTL_SEN_MODE_UNKNOWN) {
		mode_curr = CTL_SEN_MODE_PWR; // for power on -> change mode
	}

	/* get mclk information */
	mclk_curr = sen_uti_get_mclk_freq(id, mode_curr);
	mclk_next = sen_uti_get_mclk_freq(id, mode_next);
	mclksel_curr = sen_uti_get_mclksel(id, mode_curr);
	mclksel_next = sen_uti_get_mclksel(id, mode_next);

	if ((mclk_curr == mclk_next) && (mclksel_curr == mclksel_next)) {
		// no need to change mclk frequency
		return CTL_SEN_E_OK;
	}
	if ((mclk_curr == 0) || (mclk_next == 0)) {
		CTL_SEN_DBG_ERR("id %d mclk zero %d=0x%.8x %d=0x%.8x\r\n", id, mode_curr, (unsigned int)mclk_curr, mode_next, (unsigned int)mclk_next);
		return CTL_SEN_E_OK;
	}

	/* mclk sync flow */
	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync) {
		rt |= sen_ctrl_mclksync_uninit(id, mode_curr);
		rt |= sen_ctrl_mclksync_init(id, mode_next);
	}

	/* disable mclk */
	sen_uti_get_clken_cb(id)(mclksel_curr, DISABLE);

	/* change mclk */
	rt |= sen_ctrl_clk_unprepare(id, mode_curr);
	rt |= sen_ctrl_clk_prepare(id, mode_next);

	/* enable mclk */
	sen_uti_get_clken_cb(id)(mclksel_next, ENABLE);

	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d mode %d->%d mclk %d->%d fail\r\n", id, mode_curr, mode_next, mclk_curr, mclk_next);
		return CTL_SEN_E_CLK;
	}
	return CTL_SEN_E_OK;
}

INT32 sen_ctrl_cfg_pinmux(CTL_SEN_ID id, UINT32 cur_open_id_bit)
{
	INT32 rt = CTL_SEN_E_OK;
	ER rt_er;
	BOOL fail = FALSE;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	PIN_GROUP_CONFIG pinmux_cfg[CTL_SEN_PINMUX_MAX_NUM] = {0};
	CTL_SEN_PINMUX *pinmux = NULL;
	int i;


	pinmux = &init_cfg_obj.pin_cfg.pinmux;
	i = 0;
	do {
		pinmux_cfg[i].pin_function = pinmux->func;
		CTL_SEN_DBG_IND("[CAP] %d\r\n", pinmux_cfg[i].pin_function);
		pinmux = pinmux->pnext;
		i++;
	} while ((pinmux != NULL) && (i < CTL_SEN_PINMUX_MAX_NUM));

	if (pinmux != NULL) {
		CTL_SEN_DBG_ERR("pincfg num ovfl (max %d)\r\n", CTL_SEN_PINMUX_MAX_NUM);
		return CTL_SEN_E_IN_PARAM;
	}


	/* get current pinmux cfg */
	rt_er = nvt_pinmux_capture(pinmux_cfg, i);
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get pinmux error\r\n");
		ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN_PINMUX, CTL_SEN_ER_ITEM_SYS, CTL_SEN_E_KERNEL);
		fail = TRUE;
	}

	/* update pinmux cfg from user setting */
	pinmux = &init_cfg_obj.pin_cfg.pinmux;
	i = 0;
	do {
		pinmux_cfg[i].pin_function = pinmux->func;
		if (pinmux_cfg[i].pin_function == PIN_FUNC_I2C) {
			// PIN_FUNC_I2C need set "pinmux to function" after "sensor power on"
		} else {
			pinmux_cfg[i].config |= pinmux->cfg;
		}
		CTL_SEN_DBG_IND("[UPD] %d = 0x%.8x\r\n", pinmux_cfg[i].pin_function, (unsigned int)pinmux_cfg[i].config);
		pinmux = pinmux->pnext;
		i++;
	} while ((pinmux != NULL) && (i < CTL_SEN_PINMUX_MAX_NUM));

	/* set pinmux cfg */
	rt_er = nvt_pinmux_update(pinmux_cfg, i);
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("set pinmux error\r\n");
		ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN_PINMUX, CTL_SEN_ER_ITEM_SYS, CTL_SEN_E_KERNEL);
		fail = TRUE;
	}

	if (fail) {
		rt = CTL_SEN_E_PINMUX;
	}

	return rt;
}

INT32 sen_ctrl_cfg_pinmux_cmdif(CTL_SEN_ID id, UINT32 cur_open_id_bit, BOOL en)
{
	INT32 rt = CTL_SEN_E_OK;
	ER rt_er;
	BOOL fail = FALSE;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	PIN_GROUP_CONFIG pinmux_cfg[CTL_SEN_PINMUX_MAX_NUM] = {0};
	CTL_SEN_PINMUX *pinmux = NULL;
	int i, j;
	UINT32 pwron_i2c_pinmux = 0;

	/*
	    update pinmux cfg from user setting - for curret open id
	    multi-device use same pinmux
	    ex : "device A use i2c2-1" & "device B use i2c2-1"
	*/
	if ((en) || (cur_open_id_bit == 0)) {
		goto skip_upd_cur;
	}
	for (j = 0; j < CTL_SEN_ID_MAX_SUPPORT; j++) {
		if (((cur_open_id_bit) & (1 << j)) && (j != (int)id)) {
			pinmux = &g_ctl_sen_init_obj[j]->init_cfg_obj.pin_cfg.pinmux;
			i = 0;
			do {
				if (pinmux->func == PIN_FUNC_I2C) {
					pwron_i2c_pinmux |= pinmux->cfg;
				}
				CTL_SEN_DBG_IND("[UPD][%d] %d = 0x%.8x\r\n", j, pinmux->func, (unsigned int)pinmux->cfg);
				pinmux = pinmux->pnext;
				i++;
			} while ((pinmux != NULL) && (i < CTL_SEN_PINMUX_MAX_NUM));
		}
	}
skip_upd_cur:

	/* get current pinmux cfg */
	pinmux = &init_cfg_obj.pin_cfg.pinmux;
	i = 0;
	do {
		pinmux_cfg[i].pin_function = pinmux->func;
		CTL_SEN_DBG_IND("[CAP] %d\r\n", pinmux_cfg[i].pin_function);
		pinmux = pinmux->pnext;
		i++;
	} while ((pinmux != NULL) && (i < CTL_SEN_PINMUX_MAX_NUM));

	if (pinmux != NULL) {
		CTL_SEN_DBG_ERR("pincfg num ovfl (max %d)\r\n", CTL_SEN_PINMUX_MAX_NUM);
		return CTL_SEN_E_IN_PARAM;
	}

	rt_er = nvt_pinmux_capture(pinmux_cfg, i);
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get pinmux error\r\n");
		ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN_PINMUX, CTL_SEN_ER_ITEM_SYS, CTL_SEN_E_KERNEL);
		fail = TRUE;
	}

	/* update pinmux cfg from user setting */
	pinmux = &init_cfg_obj.pin_cfg.pinmux;
	i = 0;
	do {
		pinmux_cfg[i].pin_function = pinmux->func;

		if ((!en) && (pinmux_cfg[i].pin_function == PIN_FUNC_I2C)) {
			// PIN_FUNC_I2C need set "pinmux to gpio" before "sensor power off"
			pinmux_cfg[i].config &= ~(pinmux->cfg);
			pinmux_cfg[i].config |= pwron_i2c_pinmux;
		} else {
			pinmux_cfg[i].config |= pinmux->cfg;
		}
		CTL_SEN_DBG_IND("[UPD] %d = 0x%.8x\r\n", pinmux_cfg[i].pin_function, (unsigned int)pinmux_cfg[i].config);
		pinmux = pinmux->pnext;
		i++;
	} while ((pinmux != NULL) && (i < CTL_SEN_PINMUX_MAX_NUM));


	/* set pinmux cfg */
	rt_er = nvt_pinmux_update(pinmux_cfg, i);
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("set pinmux error\r\n");
		ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN_PINMUX, CTL_SEN_ER_ITEM_SYS, CTL_SEN_E_KERNEL);
		fail = TRUE;
	}

	if (fail) {
		rt = CTL_SEN_E_PINMUX;
	}

	return rt;
}

INT32 sen_ctrl_open(CTL_SEN_ID id)
{
	INT32 rt = 0;

	// open sensor cmd interface
	rt = sen_ctrl_cmdif_get_obj()->open(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN, CTL_SEN_ER_ITEM_CMDIF, rt);
		return CTL_SEN_E_CMDIF;
	}

	// open sensor interface
	rt = sen_ctrl_if_get_obj()->open(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN, CTL_SEN_ER_ITEM_IF, rt);
		return CTL_SEN_E_IF;
	}


	// open sensor interface global item
	rt = sen_ctrl_if_glb_get_obj()->open(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN_CLK, CTL_SEN_ER_ITEM_IF, rt);
		return CTL_SEN_E_IF;
	}


	// open sensor driver
	rt = sen_ctrl_drv_get_obj()->open(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN, CTL_SEN_ER_ITEM_SENDRV, rt);
		return CTL_SEN_E_SENDRV;
	}

	return rt;

}
INT32 sen_ctrl_close(CTL_SEN_ID id)
{
	INT32 rt = 0;

	// close sensor driver
	rt |= sen_ctrl_drv_get_obj()->close(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_CLOSE, CTL_SEN_ER_ITEM_SENDRV, rt);
		return CTL_SEN_E_SENDRV;
	}

	// close sensor cmd interface
	rt |= sen_ctrl_cmdif_get_obj()->close(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_CLOSE, CTL_SEN_ER_ITEM_CMDIF, rt);
		return CTL_SEN_E_CMDIF;
	}

	// close sensor interface global item
	rt = sen_ctrl_if_glb_get_obj()->close(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_CLOSE_CLK, CTL_SEN_ER_ITEM_IF, rt);
		return CTL_SEN_E_IF;
	}

	// close sensor interface
	rt |= sen_ctrl_if_get_obj()->close(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_CLOSE, CTL_SEN_ER_ITEM_IF, rt);
		return CTL_SEN_E_IF;
	}

	return rt;
}

INT32 sen_ctrl_stop_if(CTL_SEN_ID id)
{
	INT32 rt = 0;
	CTL_SEN_STATUS status = CTL_SEN_STATUS_STANDBY;

	// stop sensor interface
	// must be stop before power off & standby, otherwise power off will cause stop csi/lvds/... cannot wait fmd

	rt |= sen_ctrl_if_get_obj()->stop(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_CLOSE, CTL_SEN_ER_ITEM_IF, rt);
		return CTL_SEN_E_IF;
	}
	// csi sensor must be set stanby(LP) before power off

	sen_ctrl_drv_get_obj()->set_cfg(id, CTL_SEN_CFGID_SET_STATUS, (void *)&status);

	return rt;
}


INT32 sen_ctrl_pwr_ctrl(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag)
{
	INT32 rt = 0;

	rt |= sen_ctrl_drv_get_obj()->pwr_ctrl(id, flag);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_PWR, CTL_SEN_ER_ITEM_SENDRV, rt);
		return CTL_SEN_E_SENDRV;
	}

	return rt;
}

INT32 sen_ctrl_sleep(CTL_SEN_ID id)
{
	INT32 rt = 0;

	rt |= sen_ctrl_drv_get_obj()->sleep(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_SLP, CTL_SEN_ER_ITEM_SENDRV, rt);
		return CTL_SEN_E_SENDRV;
	}

	return rt;
}

INT32 sen_ctrl_wakeup(CTL_SEN_ID id)
{
	INT32 rt = 0;

	rt |= sen_ctrl_drv_get_obj()->wakeup(id);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_WUP, CTL_SEN_ER_ITEM_SENDRV, rt);
		return CTL_SEN_E_SENDRV;
	}

	return rt;
}

INT32 sen_ctrl_write_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	INT32 rt = 0;

	rt |= sen_ctrl_drv_get_obj()->write(id, cmd);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_WR, CTL_SEN_ER_ITEM_SENDRV, rt);
		return CTL_SEN_E_SENDRV;
	}

	return rt;
}

INT32 sen_ctrl_read_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	INT32 rt = 0;

	rt |= sen_ctrl_drv_get_obj()->read(id, cmd);
	if (rt != CTL_SEN_E_OK) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_RR, CTL_SEN_ER_ITEM_SENDRV, rt);
		return CTL_SEN_E_SENDRV;
	}

	return rt;
}

CTL_SEN_INTE sen_ctrl_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	return sen_ctrl_if_get_obj()->wait_interrupt(id, waited_flag);
}

static CTL_SEN_MODE sen_ctrl_get_chgsenmode(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	CTL_SEN_MODE mode = CTL_SEN_MODE_UNKNOWN;

	if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_AUTO) {
		CTL_SEN_GET_MODESEL_PARAM modesel_param = {0};

		modesel_param.frame_rate = chgmode_obj.auto_info.frame_rate;
		modesel_param.size = chgmode_obj.auto_info.size;
		modesel_param.frame_num = chgmode_obj.auto_info.frame_num;
		modesel_param.data_fmt = chgmode_obj.auto_info.data_fmt;
		modesel_param.pixdepth = chgmode_obj.auto_info.pixdepth;
		modesel_param.ccir = chgmode_obj.auto_info.ccir;
		modesel_param.mux_singnal_en = chgmode_obj.auto_info.mux_singnal_en;
		modesel_param.mux_signal_info = chgmode_obj.auto_info.mux_signal_info;
		modesel_param.mode_type_sel = chgmode_obj.auto_info.mode_type_sel;
		modesel_param.data_lane = chgmode_obj.auto_info.data_lane;
		if (sen_ctrl_drv_get_obj()->get_cfg(id, CTL_SEN_CFGID_GET_MODESEL, (void *)&modesel_param) == CTL_SEN_E_OK) {
			mode = modesel_param.mode;
		}
	} else if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_MANUAL) {
		mode = chgmode_obj.manual_info.sen_mode;
	}
	return mode;
}

static INT32 sen_ctl_chk_chgmode_cond(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj, BOOL *b_chg_senmode, BOOL *b_chg_fps)
{
	CTL_SEN_CHGMODE_OBJ cur_chgmode_obj;
	CTL_SEN_GET_ATTR_PARAM sen_attr;
	CTL_SEN_MODE cur_senmode, chg_senmode_sel;
	UINT32 cur_frame_rate = 0;
	INT32 rt = 0;
	BOOL sup_chg_fps = FALSE;

	/* get current sensor info */
	cur_chgmode_obj = g_ctl_sen_ctrl_obj[id]->cur_chgmode;
	cur_senmode = g_ctl_sen_ctrl_obj[id]->cur_sen_mode;
	if (cur_chgmode_obj.mode_sel == CTL_SEN_MODESEL_AUTO) {
		cur_frame_rate = cur_chgmode_obj.auto_info.frame_rate;
	} else if (cur_chgmode_obj.mode_sel == CTL_SEN_MODESEL_MANUAL) {
		cur_frame_rate = cur_chgmode_obj.manual_info.frame_rate;
	} else {
		CTL_SEN_DBG_ERR("\r\n");
		return CTL_SEN_E_SYS;
	}

	/* chk input chgmode info */
	if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_AUTO) {
		CTL_SEN_GET_MODESEL_PARAM modesel_param = {0};

		modesel_param.frame_rate = chgmode_obj.auto_info.frame_rate;
		modesel_param.size = chgmode_obj.auto_info.size;
		modesel_param.frame_num = chgmode_obj.auto_info.frame_num;
		modesel_param.data_fmt = chgmode_obj.auto_info.data_fmt;
		modesel_param.pixdepth = chgmode_obj.auto_info.pixdepth;
		modesel_param.ccir = chgmode_obj.auto_info.ccir;
		modesel_param.mux_singnal_en = chgmode_obj.auto_info.mux_singnal_en;
		modesel_param.mux_signal_info = chgmode_obj.auto_info.mux_signal_info;
		modesel_param.mode_type_sel = chgmode_obj.auto_info.mode_type_sel;
		modesel_param.data_lane = chgmode_obj.auto_info.data_lane;
		rt = sen_ctrl_drv_get_obj()->get_cfg(id, CTL_SEN_CFGID_GET_MODESEL, &modesel_param);
		if (rt == CTL_SEN_E_OK) {
			chg_senmode_sel = modesel_param.mode;
		} else {
//			chg_senmode_sel = CTL_SEN_MODE_1;
			CTL_SEN_DBG_ERR("sensor not support auto sel mode\r\n");
			return CTL_SEN_E_SENDRV_GET_FAIL;
		}

		if (chg_senmode_sel != cur_senmode) {
			*b_chg_senmode = TRUE;
			*b_chg_fps = TRUE;
		} else if (chgmode_obj.auto_info.frame_rate != cur_frame_rate) {
			*b_chg_senmode = FALSE;
			*b_chg_fps = TRUE;
		} else {
			*b_chg_senmode = FALSE;
			*b_chg_fps = FALSE;
		}

	} else if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_MANUAL) {
		if (chgmode_obj.manual_info.sen_mode != cur_senmode) {
			*b_chg_senmode = TRUE;
			*b_chg_fps = TRUE;
		} else if (chgmode_obj.manual_info.frame_rate != cur_frame_rate) {
			*b_chg_senmode = FALSE;
			*b_chg_fps = TRUE;
		} else {
			*b_chg_senmode = FALSE;
			*b_chg_fps = FALSE;
		}

	} else {
		*b_chg_senmode = TRUE;
		*b_chg_fps = TRUE;
		CTL_SEN_DBG_ERR("error\r\n");
	}


	/* chk sensor driver support chgfps */
	memset((void *)&sen_attr, 0, sizeof(CTL_SEN_GET_ATTR_PARAM));
	rt = sen_ctrl_drv_get_obj()->get_cfg(id, CTL_SEN_CFGID_GET_ATTR, &sen_attr);
	if (rt == CTL_SEN_E_OK) {
		if ((sen_attr.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) == CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
			sup_chg_fps = TRUE;
		}
	}
	if (sup_chg_fps == FALSE) {
		if ((*b_chg_senmode == FALSE) && (*b_chg_fps == TRUE)) {
			// sensor driver not support chgfps, conv chgfps to chgmode
			*b_chg_senmode = TRUE;
			*b_chg_fps = FALSE;
		}
	}

	CTL_SEN_DBG_IND("b_chg_senmode %d b_chg_fps %d\r\n", *b_chg_senmode, *b_chg_fps);

	return rt;
}

static INT32 sen_ctl_csi_en_cb(CTL_SEN_ID id, BOOL en)
{
	if (en) {
		return sen_ctrl_if_csi_get_obj()->start(id);
	} else {
		return sen_ctrl_if_csi_get_obj()->stop(id);
	}
}

INT32 sen_ctrl_chgmode(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj, CTL_SEN_MODE *chg_sen_mode_rst)
{
	INT32 rt = 0;
	BOOL b_chg_senmode, b_chg_fps;
	CTL_SEN_MODE chg_sen_mode = sen_ctrl_get_chgsenmode(id, chgmode_obj);
	CTL_SEN_IF_TYPE if_type = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type;
	CTL_SEN_STATUS status = CTL_SEN_STATUS_STANDBY;

	/* chk chgmode condition */
	rt = sen_ctl_chk_chgmode_cond(id, chgmode_obj, &b_chg_senmode, &b_chg_fps);
	if (rt != 0) {
		ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_SYS, rt);
		return CTL_SEN_E_SYS;
	}

	if ((b_chg_senmode == FALSE) && (b_chg_fps == FALSE)) {
		*chg_sen_mode_rst = g_ctl_sen_ctrl_obj[id]->cur_sen_mode;
	} else if ((b_chg_senmode == FALSE) && (b_chg_fps == TRUE)) {
		rt |= sen_ctrl_drv_get_obj()->chgfps(id, chgmode_obj);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_SENDRV, rt);
			return CTL_SEN_E_SENDRV;
		}
		*chg_sen_mode_rst = g_ctl_sen_ctrl_obj[id]->cur_sen_mode;
	} else {

		/* chk sendrv enable mipi or not */
		sen_uti_set_sendrv_en_mipi(id, sen_ctl_csi_en_cb);

		// stop sensor interface
		rt |= sen_ctrl_if_get_obj()->stop(id);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_IF, rt);
			return CTL_SEN_E_IF;
		}

		// set sendrv to CTL_SEN_STATUS_STANDBY (in order to change clk)
		sen_ctrl_drv_get_obj()->set_cfg(id, CTL_SEN_CFGID_SET_STATUS, (void *)&status);

		// cfg change mclk
		rt = sen_ctrl_chgmclk(id, g_ctl_sen_ctrl_obj[id]->cur_sen_mode, chg_sen_mode);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_SYS, rt);
			return CTL_SEN_E_CLK;
		}

		// cfg sensor interface
		rt |= sen_ctrl_if_get_obj()->set_mode_cfg(id, chg_sen_mode, chgmode_obj.output_dest);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_IF, rt);
			return CTL_SEN_E_IF;
		}

		// cfg and start sensor cmd interface (vx1 need to enable before i2c cmd)
		rt |= sen_ctrl_cmdif_get_obj()->set_mode_cfg(id, chg_sen_mode, chgmode_obj.output_dest);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_CMDIF, rt);
			return CTL_SEN_E_CMDIF;
		}
		rt |= sen_ctrl_cmdif_get_obj()->start(id);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_CMDIF, rt);
			return CTL_SEN_E_CMDIF;
		}

		// start sensor interface and set sensor i2c cmd (sensor driver chgmode)
		rt |= sen_ctrl_if_get_obj()->start(id, FALSE);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_IF, rt);
			return CTL_SEN_E_IF;
		}

		rt |= sen_ctrl_drv_get_obj()->chgmode(id, chgmode_obj, chg_sen_mode_rst);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_SENDRV, rt);
			return CTL_SEN_E_SENDRV;
		}

		rt |= sen_ctrl_if_get_obj()->start(id, TRUE);
		if (rt != CTL_SEN_E_OK) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_IF, rt);
			return CTL_SEN_E_IF;
		}

		if (if_type == CTL_SEN_IF_TYPE_SLVSEC) {
			rt = sen_ctrl_if_get_obj()->set_cfg(id, KDRV_SSENIF_SLVSEC_POST_INIT, 0); // need to set after sensor i2c cmd
			if (rt != CTL_SEN_E_OK) {
				ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_IF, rt);
				return CTL_SEN_E_IF;
			}
		}
	}

	return rt;
}

INT32 sen_ctrl_set_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *value)
{
	INT32 rt = 0;
	CTL_SEN_CTL_VX1_GPIO_PARAM vx1_gpio_param = {0};
	KDRV_SSENIFVX1_I2CCMD vx1_i2c_cmd = {0};
	CTL_SEN_CMD *sen_cmd;
	KDRV_TGE_VDHD_INFO tge_vdhd_info = {0};
	UINT64 tmp_u64;

	switch (cfg_id) {
	case CTL_SEN_CFGID_SET_SIGNAL_RST:
		sen_ctrl_if_tge_get_obj()->reset(id, *(CTL_SEN_OUTPUT_DEST *)value, TRUE);
		break;
	case CTL_SEN_CFGID_INIT_IF_MAP:
		rt = CTL_SEN_E_NS;
		break;
	case CTL_SEN_CFGID_INIT_IF_TIMEOUT_MS:
		g_ctl_sen_init_obj[id]->init_ext_obj.if_info.timeout_ms = *(UINT32 *)value;
		break;
	case CTL_SEN_CFGID_INIT_SIGNAL_SYNC:
		g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync = *(UINT32 *)value;
		break;
	case CTL_SEN_CFGID_INIT_MIPI_CB:
		//rt = sen_ctrl_if_get_obj()->set_cfg(id, KDRV_SSENIF_CSI_CALLBACK, *(UINT32 *)value);
		DBG_ERR("wait kdrv_ssenif interface KDRV_SSENIF_CSI_CALLBACK ready\r\n");
		break;
	case CTL_SEN_CFGID_VX1_SLAVE_ADDR:
		rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_SENSOR_SLAVEADDR, *(UINT32 *)value);
		break;
	case CTL_SEN_CFGID_VX1_ADDR_BYTE_CNT:
		rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_SENREG_ADDR_BC, *(UINT32 *)value);
		break;
	case CTL_SEN_CFGID_VX1_DATA_BYTE_CNT:
		rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_SENREG_DATA_BC, *(UINT32 *)value);
		break;
	case CTL_SEN_CFGID_VX1_I2C_WRITE:
		sen_cmd = (CTL_SEN_CMD *)value;
		if (sen_cmd == NULL) {
			CTL_SEN_DBG_ERR("input obj NULL\r\n");
			rt = CTL_SEN_E_IN_PARAM;
		} else {
			vx1_i2c_cmd.reg_start_addr = sen_cmd->addr;
			vx1_i2c_cmd.data_size = sen_cmd->data_len;
			vx1_i2c_cmd.data_buffer = (void *)(&sen_cmd->data[0]);
			rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_I2C_WRITE, (UINT32)(&vx1_i2c_cmd));
		}
		break;
	case CTL_SEN_CFGID_VX1_I2C_READ:
		rt = CTL_SEN_E_NS;
		break;
	case CTL_SEN_CFGID_VX1_GPIO:
		vx1_gpio_param = *(CTL_SEN_CTL_VX1_GPIO_PARAM *)value;
		if (vx1_gpio_param.pin == 0) {
			rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_GPIO_0, (UINT32)vx1_gpio_param.value);
		} else if (vx1_gpio_param.pin == 1) {
			rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_GPIO_1, (UINT32)vx1_gpio_param.value);
		} else if (vx1_gpio_param.pin == 2) {
			rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_GPIO_2, (UINT32)vx1_gpio_param.value);
		} else if (vx1_gpio_param.pin == 3) {
			rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_GPIO_3, (UINT32)vx1_gpio_param.value);
		} else if (vx1_gpio_param.pin == 4) {
			rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_GPIO_4, (UINT32)vx1_gpio_param.value);
		} else {
			CTL_SEN_DBG_ERR("id %d pin %d error\r\n", id, vx1_gpio_param.pin);
			rt = CTL_SEN_E_IN_PARAM;
		}
		break;
	case CTL_SEN_CFGID_VX1_PLUG:
		rt = CTL_SEN_E_NS;
		break;
	case CTL_SEN_CFGID_VX1_I2C_SPEED:
		rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_I2C_SPEED, *(UINT32 *)value);
		break;
	case CTL_SEN_CFGID_VX1_I2C_NACK_CHK:
		rt |= sen_ctrl_cmdif_get_obj()->set_cfg(id, KDRV_SSENIF_VX1_I2CNACK_CHECK, *(UINT32 *)value);
		break;
	case CTL_SEN_CFGID_TGE_VD_PERIOD:
		rt |= sen_ctrl_if_get_obj()->get_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		if (rt == CTL_SEN_E_OK) {
			tge_vdhd_info.vd_period = *(UINT32 *)value;
			tge_vdhd_info.vd_period = (UINT32)(tge_vdhd_info.vd_period * tge_vdhd_info.hd_period);
			rt |= sen_ctrl_if_get_obj()->set_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		}
		break;
	case CTL_SEN_CFGID_TGE_HD_PERIOD:
		rt |= sen_ctrl_if_get_obj()->get_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		if (rt == CTL_SEN_E_OK) {
			if (tge_vdhd_info.vd_period == 0) {
				CTL_SEN_DBG_ERR("get KDRV_TGE_PARAM_IPL_VDHD vd error (%d)\r\n", tge_vdhd_info.vd_period);
				tge_vdhd_info.vd_period = 1;
			}
			if (tge_vdhd_info.hd_period == 0) {
				CTL_SEN_DBG_ERR("get KDRV_TGE_PARAM_IPL_VDHD hd error (%d)\r\n", tge_vdhd_info.hd_period);
				tge_vdhd_info.hd_period = 1;
			}
			tmp_u64 = tge_vdhd_info.vd_period;
			tmp_u64 = tmp_u64 * (*(UINT32 *)value);
			tmp_u64 = sen_uint64_dividend(tmp_u64, tge_vdhd_info.hd_period);
			tge_vdhd_info.vd_period = (UINT32)tmp_u64;
			tge_vdhd_info.hd_period = *(UINT32 *)value;
			rt |= sen_ctrl_if_get_obj()->set_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		}
		break;
	case CTL_SEN_CFGID_TGE_VD_SYNC:
		rt |= sen_ctrl_if_get_obj()->get_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		if (rt == CTL_SEN_E_OK) {
			tge_vdhd_info.vd_assert = *(UINT32 *)value;
			tge_vdhd_info.vd_assert = (UINT32)(tge_vdhd_info.vd_assert * tge_vdhd_info.hd_period);
			rt |= sen_ctrl_if_get_obj()->set_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		}
		break;
	case CTL_SEN_CFGID_TGE_HD_SYNC:
		rt |= sen_ctrl_if_get_obj()->get_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		if (rt == CTL_SEN_E_OK) {
			tge_vdhd_info.hd_assert = *(UINT32 *)value;
			rt |= sen_ctrl_if_get_obj()->set_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		}
		break;
	case CTL_SEN_CFGID_SET_STATUS:
		if (*(CTL_SEN_STATUS *)value == CTL_SEN_STATUS_STANDBY) {
			rt |= sen_ctrl_stop_if(id);
		} else {
			rt |= sen_ctrl_drv_get_obj()->set_cfg(id, cfg_id, value);
		}
		break;
	default:
		rt |= sen_ctrl_drv_get_obj()->set_cfg(id, cfg_id, value);
		break;
	}

	if (rt != CTL_SEN_E_OK) {
		if ((cfg_id >= CTL_SEN_CFGID_VX1_BASE) && (cfg_id <= CTL_SEN_CFGID_VX1_MAX)) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_SET, CTL_SEN_ER_ITEM_CMDIF, rt);
			rt = CTL_SEN_E_CMDIF;
		} else if ((cfg_id >= CTL_SEN_CFGID_TGE_BASE) && (cfg_id <= CTL_SEN_CFGID_TGE_MAX)) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_SET, CTL_SEN_ER_ITEM_IF, rt);
			rt = CTL_SEN_E_IF;
		} else {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_SET, CTL_SEN_ER_ITEM_SENDRV, rt);
			rt = CTL_SEN_E_SENDRV;
		}
	}

	return rt;
}

INT32 sen_ctrl_get_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *value)
{
	INT32 rt = 0;
	UINT32 *timeout;
	UINT32 *tge_sync;
	UINT32 *tmp;
	CTL_SEN_CTL_VX1_GPIO_PARAM *vx1_gpio_param;
	UINT32 tmp_uint32 = 0;
	UINT32 vx1_i2c_cmd_data[2] = {0};
	KDRV_SSENIFVX1_I2CCMD vx1_i2c_cmd = {0};
	CTL_SEN_CMD *sen_cmd;
	KDRV_TGE_VDHD_INFO tge_vdhd_info = {0};

	switch (cfg_id) {
	case CTL_SEN_CFGID_INIT_IF_MAP:
		rt = CTL_SEN_E_NS;
		break;
	case CTL_SEN_CFGID_INIT_IF_TIMEOUT_MS:
		timeout = (UINT32 *)value;
		*timeout = g_ctl_sen_init_obj[id]->init_ext_obj.if_info.timeout_ms;
		break;
	case CTL_SEN_CFGID_INIT_SIGNAL_SYNC:
		tge_sync = (UINT32 *)value;
		*tge_sync = g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync;
		break;
	case CTL_SEN_CFGID_VX1_SLAVE_ADDR:
		rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_SENSOR_SLAVEADDR, value);
		break;
	case CTL_SEN_CFGID_VX1_ADDR_BYTE_CNT:
		rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_SENREG_ADDR_BC, value);
		break;
	case CTL_SEN_CFGID_VX1_DATA_BYTE_CNT:
		rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_SENREG_DATA_BC, value);
		break;
	case CTL_SEN_CFGID_VX1_I2C_WRITE:
		rt = E_NOSPT;
		break;
	case CTL_SEN_CFGID_VX1_I2C_READ:
		sen_cmd = (CTL_SEN_CMD *)value;
		if (sen_cmd == NULL) {
			CTL_SEN_DBG_ERR("input obj NULL\r\n");
			rt = CTL_SEN_E_IN_PARAM;
		} else {
			vx1_i2c_cmd.reg_start_addr = sen_cmd->addr;
			vx1_i2c_cmd.data_size = sen_cmd->data_len;
			if (sen_cmd->data_len > 2) {
				CTL_SEN_DBG_ERR("max support 2 data len\r\n");
				vx1_i2c_cmd.data_size = 2;
				rt |= CTL_SEN_E_IN_PARAM;
			}
			vx1_i2c_cmd.data_buffer = &vx1_i2c_cmd_data[0];
			rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_I2C_READ, (void *)(&vx1_i2c_cmd));
			if (vx1_i2c_cmd.data_size >= 1) {
				sen_cmd->data[0] = *(UINT32 *)vx1_i2c_cmd.data_buffer;
			}
			if (vx1_i2c_cmd.data_size >= 2) {
				sen_cmd->data[1] = *(UINT32 *)((int)vx1_i2c_cmd.data_buffer + 4);
			}
		}
		break;
	case CTL_SEN_CFGID_VX1_GPIO:
		vx1_gpio_param = (CTL_SEN_CTL_VX1_GPIO_PARAM *)value;
		if (vx1_gpio_param->pin == 0) {
			rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_GPIO_0, &tmp_uint32);
		} else if (vx1_gpio_param->pin == 1) {
			rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_GPIO_1, &tmp_uint32);
		} else if (vx1_gpio_param->pin == 2) {
			rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_GPIO_2, &tmp_uint32);
		} else if (vx1_gpio_param->pin == 3) {
			rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_GPIO_3, &tmp_uint32);
		} else if (vx1_gpio_param->pin == 4) {
			rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_GPIO_4, &tmp_uint32);
		} else {
			CTL_SEN_DBG_ERR("id %d pin %d error\r\n", id, vx1_gpio_param->pin);
			rt = CTL_SEN_E_IN_PARAM;
		}
		vx1_gpio_param->value = tmp_uint32;
		break;
	case CTL_SEN_CFGID_VX1_PLUG:
		rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_GET_PLUG, value);
		break;
	case CTL_SEN_CFGID_VX1_I2C_SPEED:
		rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_I2C_SPEED, value);
		break;
	case CTL_SEN_CFGID_VX1_I2C_NACK_CHK:
		rt |= sen_ctrl_cmdif_get_obj()->get_cfg(id, KDRV_SSENIF_VX1_I2CNACK_CHECK, value);
		break;
	case CTL_SEN_CFGID_TGE_VD_PERIOD:
		rt |= sen_ctrl_if_get_obj()->get_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		tmp = (UINT32 *)value;
		if (tge_vdhd_info.hd_period == 0) {
			tge_vdhd_info.hd_period = 1;
		}
		*tmp = (tge_vdhd_info.vd_period / tge_vdhd_info.hd_period);
		break;
	case CTL_SEN_CFGID_TGE_HD_PERIOD:
		rt |= sen_ctrl_if_get_obj()->get_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		tmp = (UINT32 *)value;
		*tmp = tge_vdhd_info.hd_period;
		break;
	case CTL_SEN_CFGID_TGE_VD_SYNC:
		rt |= sen_ctrl_if_get_obj()->get_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		tmp = (UINT32 *)value;
		if (tge_vdhd_info.hd_period == 0) {
			tge_vdhd_info.hd_period = 1;
		}
		*tmp = (tge_vdhd_info.vd_assert / tge_vdhd_info.hd_period);
		break;
	case CTL_SEN_CFGID_TGE_HD_SYNC:
		rt |= sen_ctrl_if_get_obj()->get_cfg_tge(id, KDRV_TGE_PARAM_IPL_VDHD, (void *)&tge_vdhd_info);
		tmp = (UINT32 *)value;
		*tmp = tge_vdhd_info.hd_assert;
		break;
	case CTL_SEN_CFGID_GET_VD_CNT:
		rt |= sen_ctrl_if_get_obj()->get_cfg_kflow(id, CTL_SEN_CFGID_GET_VD_CNT, value);
		break;
	case CTL_SEN_CFGID_GET_SSENIF_ERR_CNT:
		rt |= sen_ctrl_if_get_obj()->get_cfg_kflow(id, CTL_SEN_CFGID_GET_SSENIF_ERR_CNT, value);
		break;
	case CTL_SEN_CFGID_GET_FRAME_NUMBER:
		rt |= sen_ctrl_if_get_obj()->get_cfg_kflow(id, CTL_SEN_CFGID_GET_FRAME_NUMBER, value);
		break;
	default:
		rt |= sen_ctrl_drv_get_obj()->get_cfg(id, cfg_id, value);
		break;
	}

	if (rt != CTL_SEN_E_OK) {
		if ((cfg_id >= CTL_SEN_CFGID_VX1_BASE) && (cfg_id <= CTL_SEN_CFGID_VX1_MAX)) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_GET, CTL_SEN_ER_ITEM_CMDIF, rt);
			rt = CTL_SEN_E_CMDIF;
		} else if ((cfg_id >= CTL_SEN_CFGID_TGE_BASE) && (cfg_id <= CTL_SEN_CFGID_TGE_MAX)) {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_GET, CTL_SEN_ER_ITEM_IF, rt);
			rt = CTL_SEN_E_IF;
		} else {
			ctl_sen_set_er(id, CTL_SEN_ER_OP_GET, CTL_SEN_ER_ITEM_SENDRV, rt);
			rt = CTL_SEN_E_SENDRV;
		}
	}

	return rt;
}

void sen_ctrl_dbg_info(CTL_SEN_ID id, CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	DBG_DUMP("======== ctl_sen dump begin id %d ========\r\n", id);
	if ((dbg_sel & CTL_SEN_DBG_SEL_DUMP) == CTL_SEN_DBG_SEL_DUMP) {
		sen_dbg_get_obj()->dump_info(id);
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_DUMP_EXT) == CTL_SEN_DBG_SEL_DUMP_EXT) {
		sen_dbg_get_obj()->dump_ext_info(id);
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_DUMP_DRV) == CTL_SEN_DBG_SEL_DUMP_DRV) {
		sen_dbg_get_obj()->dump_drv_info(id);
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_WAITVD) == CTL_SEN_DBG_SEL_WAITVD) {
		if ((dbg_sel & CTL_SEN_TGE_TAG) == CTL_SEN_TGE_TAG) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD | CTL_SEN_TGE_TAG);
		} else if ((dbg_sel & CTL_SEN_VX1_TAG) == CTL_SEN_VX1_TAG) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD | CTL_SEN_VX1_TAG);
		} else {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD);
		}
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_WAITVD2) == CTL_SEN_DBG_SEL_WAITVD2) {
		if ((dbg_sel & CTL_SEN_VX1_TAG) == CTL_SEN_VX1_TAG) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD2 | CTL_SEN_VX1_TAG);
		} else {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD2);
		}
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_WAITVD3) == CTL_SEN_DBG_SEL_WAITVD3) {
		if ((dbg_sel & CTL_SEN_VX1_TAG) == CTL_SEN_VX1_TAG) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD3 | CTL_SEN_VX1_TAG);
		} else {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD3);
		}
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_WAITVD4) == CTL_SEN_DBG_SEL_WAITVD4) {
		if ((dbg_sel & CTL_SEN_VX1_TAG) == CTL_SEN_VX1_TAG) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD4 | CTL_SEN_VX1_TAG);
		} else {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD4);
		}
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_WAITFMD) == CTL_SEN_DBG_SEL_WAITFMD) {
		sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_FRAMEEND);
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_WAITFMD2) == CTL_SEN_DBG_SEL_WAITFMD2) {
		sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_FRAMEEND2);
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_DUMP_MAP_TBL) == CTL_SEN_DBG_SEL_DUMP_MAP_TBL) {
		sen_dbg_get_obj()->dump_map_tbl_info();
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_DUMP_PROC_TIME) == CTL_SEN_DBG_SEL_DUMP_PROC_TIME) {
		sen_dbg_get_obj()->dump_proc_time();
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_DBG_MSG) == CTL_SEN_DBG_SEL_DBG_MSG) {
		sen_dbg_get_obj()->dbg_msg(id, CTL_SEN_DBG_MSG_EN(param), CTL_SEN_DBG_MSG_TYPE(param));
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_DUMP_CTL) == CTL_SEN_DBG_SEL_DUMP_CTL) {
		sen_dbg_get_obj()->dump_ctl_info(id);
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_DUMP_ER) == CTL_SEN_DBG_SEL_DUMP_ER) {
		sen_dbg_get_obj()->dump_er(id);
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_LV) == CTL_SEN_DBG_SEL_LV) {
		sen_dbg_get_obj()->set_lv(param);
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_WAIT_VD_TO_SIE) == CTL_SEN_DBG_SEL_WAIT_VD_TO_SIE) {
		if (param == 0) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD_TO_SIE0);
		} else if (param == 1) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD_TO_SIE1);
		} else if (param == 3) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD_TO_SIE3);
		} else if (param == 4) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_VD_TO_SIE4);
		}
	}

	if ((dbg_sel & CTL_SEN_DBG_SEL_WAIT_FMD_TO_SIE) == CTL_SEN_DBG_SEL_WAIT_FMD_TO_SIE) {
		if (param == 0) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_FMD_TO_SIE0);
		} else if (param == 1) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_FMD_TO_SIE1);
		} else if (param == 3) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_FMD_TO_SIE3);
		} else if (param == 4) {
			sen_dbg_get_obj()->wait_inte(id, CTL_SEN_INTE_FMD_TO_SIE4);
		}
	}

	DBG_DUMP("======== ctl_sen dump end id %d ========\r\n", id);
}

