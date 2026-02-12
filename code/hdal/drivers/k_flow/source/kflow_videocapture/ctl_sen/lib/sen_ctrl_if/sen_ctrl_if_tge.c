/*
    Sensor Interface library - ctrl sensor interface (tge)

    Sensor Interface library.

    @file       sen_ctrl_if_tge.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "sen_ctrl_if_int.h"

static BOOL sen_tge_swap[CTL_SEN_ID_MAX_SUPPORT] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
static BOOL sen_tge_start[CTL_SEN_ID_MAX_SUPPORT] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
UINT32 ctl_sen_tge_open_cnt = 0;
BOOL sen_tge_isr_vd_flag[CTL_SEN_TGE_MAX_CH] = {0};

static KDRV_TGE_VDHD_CH get_tge_trig_id(CTL_SEN_ID id)
{
	UINT32 chsft;

	chsft = sen_get_drvdev_tge_chsft(id);

	if (chsft == CTL_SEN_IGNORE) {
		return KDRV_TGE_VDHD_CH1;
	}

	if (sen_tge_swap[id]) {
		if (chsft >= TGE_CH_SWAP_MAP) {
			chsft = chsft - TGE_CH_SWAP_MAP;
		} else {
			chsft = chsft + TGE_CH_SWAP_MAP;
		}
	}

	return (KDRV_TGE_VDHD_CH)(KDRV_TGE_VDHD_CH1 << chsft);
}

static UINT32 get_tge_ch(UINT32 channel)
{
	return KDRV_DEV_ID(KDRV_CHIP_TGE, KDRV_VDOCAP_TGE_ENGINE0, channel);
}

static TGE_WAIT_EVENT_SEL get_tge_vd_event(CTL_SEN_ID id)
{
	UINT32 chsft;

	chsft = sen_get_drvdev_tge_chsft(id);

	if (chsft == CTL_SEN_IGNORE) {
		return TGE_WAIT_VD;
	}

	if (sen_tge_swap[id]) {
		if (chsft >= TGE_CH_SWAP_MAP) {
			chsft = chsft - TGE_CH_SWAP_MAP;
		} else {
			chsft = chsft + TGE_CH_SWAP_MAP;
		}
	}

	return (TGE_WAIT_EVENT_SEL)(TGE_WAIT_VD << chsft);
}

static KDRV_TGE_SWAP_FUNC_ID get_tge_swap_func_id(CTL_SEN_ID id)
{
	KDRV_TGE_SWAP_FUNC_ID swap_func_id = KDRV_TGE_SWAP_CH15;
	UINT32 chsft;

	chsft = sen_get_drvdev_tge_chsft(id);

	if (chsft == CTL_SEN_IGNORE) {
		return swap_func_id;
	}

	switch (chsft) {
	case TGE_CH_SFT0:
	case TGE_CH_SFT4:
		swap_func_id =  KDRV_TGE_SWAP_CH15;
		break;
	case TGE_CH_SFT1:
	case TGE_CH_SFT5:
		swap_func_id = KDRV_TGE_SWAP_CH26;
		break;
	case TGE_CH_SFT2:
	case TGE_CH_SFT6:
		swap_func_id = KDRV_TGE_SWAP_CH37;
		break;
	case TGE_CH_SFT3:
	case TGE_CH_SFT7:
		swap_func_id = KDRV_TGE_SWAP_CH48;
		break;
	default:
		CTL_SEN_DBG_ERR("N.S. sensor id %d\r\n", id);
		break;
	}
	return swap_func_id;
}

static void tge_isr_cb(KDRV_TGE_ISR_EVENT event, void *param)
{
	UINT32 chsft;

	for (chsft = TGE_CH_SFT0; chsft < TGE_CH_SFT7; chsft++) {
		if ((event & (KDRV_TGE_INT_VD1 << chsft)) == (KDRV_TGE_INT_VD1 << chsft)) {
			sen_tge_isr_vd_flag[chsft] = TRUE;
		}
	}
}

static KDRV_TGE_CLK_ID sen_get_tge_clk_id(CTL_SEN_ID id)
{
	KDRV_TGE_CLK_ID tge_clk_id = KDRV_TGECLK1;
	UINT8 ch_sft = sen_get_drvdev_tge_chsft(id);

	// 52x TGE NO swap function
	switch (ch_sft) {
	case TGE_CH_SFT0:
		tge_clk_id = KDRV_TGECLK1;
		break;
	case TGE_CH_SFT1:
		tge_clk_id = KDRV_TGECLK2;
		break;
	default:
		CTL_SEN_DBG_ERR("id %d sht %d ovfl\r\n", id, ch_sft);
	}
	return tge_clk_id;
}

static INT32 sen_ctrl_if_tge_open(CTL_SEN_ID id)
{
	ER kdrv_rt = E_OK;
	KDRV_TGE_OPENCFG kdrv_tge_open_obj;
	KDRV_TGE_CLK_SRC_SEL kdrv_tge_clk_sel = {0};
	CTL_SEN_MODE mode = CTL_SEN_MODE_PWR;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	/*
	    set swap
	*/
	sen_tge_swap[id] = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.swap;

	/*
	    chk open cnt
	    if open_cnt!= 0 (already open), set clk src only
	*/
	ctl_sen_tge_open_cnt++;
	if (ctl_sen_tge_open_cnt > 1) {
		// set tge_clock_sel
		kdrv_tge_clk_sel.clk_src_info = sen_ctrl_clksel_cov2_tge(sen_uti_get_mclksel(id, mode));
		kdrv_rt |=  kdrv_tge_set(get_tge_ch(sen_get_tge_clk_id(id)), KDRV_TGE_PARAM_IPL_TGE_CLK_SRC, (void *)(&kdrv_tge_clk_sel));
		if (kdrv_rt == E_OK) {
			return CTL_SEN_E_OK;
		}
		CTL_SEN_DBG_ERR("cfg clk error\r\n");
		return CTL_SEN_E_KDRV_TGE;
	}

	/*
	    tge open cfg
	*/
	memset((void *)&kdrv_tge_open_obj, 0, sizeof(KDRV_TGE_OPENCFG));

	kdrv_tge_open_obj.tge_clock_sel = KDRV_TGE_CLK_MCLK1;
	kdrv_tge_open_obj.tge_clock_sel2 = KDRV_TGE_CLK_MCLK2;

	if (sen_get_tge_clk_id(id) == KDRV_TGECLK1) {
		kdrv_tge_open_obj.tge_clock_sel = sen_ctrl_clksel_cov2_tge(sen_uti_get_mclksel(id, mode));
	} else if (sen_get_tge_clk_id(id) == KDRV_TGECLK2) {
		kdrv_tge_open_obj.tge_clock_sel2 = sen_ctrl_clksel_cov2_tge(sen_uti_get_mclksel(id, mode));
	}

	kdrv_rt |=  kdrv_tge_set(get_tge_ch(0), KDRV_TGE_PARAM_IPL_OPENCFG, (void *)(&kdrv_tge_open_obj));

	/*
	    tge open
	*/
	kdrv_rt |=  kdrv_tge_open(KDRV_CHIP_TGE, KDRV_VDOCAP_TGE_ENGINE0);

	if (kdrv_rt != E_OK) {
		CTL_SEN_DBG_ERR("id %d open fail\r\n", id);
		return CTL_SEN_E_KDRV_TGE;
	}

	/*
	    set tge isr cb
	*/
	kdrv_rt |=  kdrv_tge_set(get_tge_ch(0), KDRV_TGE_PARAM_IPL_SET_ISR_CB, (void *)(&tge_isr_cb));

	if (kdrv_rt != E_OK) {
		CTL_SEN_DBG_ERR("id %d set isr fail\r\n", id);
		return CTL_SEN_E_KDRV_TGE;
	}

	return CTL_SEN_E_OK;
}

static INT32 sen_ctrl_if_tge_close(CTL_SEN_ID id)
{
	ER kdrv_rt = E_OK;
	KDRV_TGE_TRIG_INFO trig_cfg;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	ctl_sen_tge_open_cnt--;
	if (ctl_sen_tge_open_cnt != 0) {
		return CTL_SEN_E_OK;
	}

	memset((void *)&trig_cfg, 0, sizeof(KDRV_TGE_TRIG_INFO));

	trig_cfg.trig_type = KDRV_TGE_TRIG_STOP;
	trig_cfg.ch_enable = FALSE;
	trig_cfg.wait_end_enable = FALSE;
	trig_cfg.wait_event = get_tge_vd_event(id);

	kdrv_rt |=  kdrv_tge_trigger(get_tge_ch(get_tge_trig_id(id)), NULL, NULL, (void *)&trig_cfg);

	if (kdrv_rt != E_OK) {
		CTL_SEN_DBG_ERR("id %d stop fail\r\n", id);
		return CTL_SEN_E_KDRV_TGE;
	}

	kdrv_rt |=  kdrv_tge_close(KDRV_CHIP_TGE, KDRV_VDOCAP_TGE_ENGINE0);

	if (kdrv_rt != E_OK) {
		CTL_SEN_DBG_ERR("id %d close fail\r\n", id);
		return CTL_SEN_E_KDRV_TGE;
	}

	return CTL_SEN_E_OK;
}

static INT32 sen_ctrl_if_tge_start(CTL_SEN_ID id)
{
	INT32 kdrv_rt = 0, rt = 0;
	KDRV_TGE_TRIG_INFO trig_cfg;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	memset((void *)&trig_cfg, 0, sizeof(KDRV_TGE_TRIG_INFO));

	trig_cfg.trig_type = KDRV_TGE_TRIG_VDHD;
	trig_cfg.ch_enable = TRUE;
	trig_cfg.wait_end_enable = TRUE;
	trig_cfg.wait_event = get_tge_vd_event(id);

	kdrv_rt |=  kdrv_tge_trigger(get_tge_ch(get_tge_trig_id(id)), NULL, NULL, (void *)&trig_cfg);

	if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_TGE;
	} else {
		sen_tge_start[id] = TRUE;
	}
	return rt;
}

static INT32 sen_ctrl_if_tge_stop(CTL_SEN_ID id)
{
	INT32 kdrv_rt = 0, rt = 0;
	KDRV_TGE_TRIG_INFO trig_cfg;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	if (sen_tge_start[id] == FALSE) {
		return CTL_SEN_E_OK;
	}

	memset((void *)&trig_cfg, 0, sizeof(KDRV_TGE_TRIG_INFO));

	trig_cfg.trig_type = KDRV_TGE_TRIG_VDHD;
	trig_cfg.ch_enable = FALSE;
	trig_cfg.wait_end_enable = FALSE;//TRUE;
	trig_cfg.wait_event = get_tge_vd_event(id);

	kdrv_rt |=  kdrv_tge_trigger(get_tge_ch(get_tge_trig_id(id)), NULL, NULL, (void *)&trig_cfg);

	if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_TGE;
	} else {
		sen_tge_start[id] = FALSE;
	}
	return rt;
}

static INT32 sen_ctrl_if_tge_reset(CTL_SEN_ID id, CTL_SEN_OUTPUT_DEST output_dest, BOOL en)
{
	INT32 kdrv_rt = 0, rt = 0;
	KDRV_TGE_TRIG_INFO trig_cfg;
	UINT32 i, trig_id;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	memset((void *)&trig_cfg, 0, sizeof(KDRV_TGE_TRIG_INFO));

	trig_cfg.trig_type = KDRV_TGE_TRIG_VDHD;
	trig_cfg.ch_enable = en;
	trig_cfg.wait_end_enable = en;
	trig_id = 0;
	for (i = CTL_SEN_ID_1; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (!sen_ctrl_chk_add_ctx(i)) {
			continue;
		}
		if ((int)(output_dest & (1 << i)) == (int)(1 << i)) {
			trig_id |= get_tge_trig_id(i);
			trig_cfg.wait_event |= get_tge_vd_event(i);
		}
	}

	kdrv_rt |=  kdrv_tge_trigger(get_tge_ch(trig_id), NULL, NULL, (void *)&trig_cfg);

	if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("id %d output_dest 0x%.8x error\r\n", id, output_dest);
		rt = CTL_SEN_E_KDRV_TGE;
	}
	return rt;
}


static CTL_SEN_INTE sen_ctrl_if_tge_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	UINT32 count = 0;
	CTL_SEN_INTE inte_rt = CTL_SEN_INTE_NONE;
	UINT32 chsft;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return inte_rt;
	}

	chsft = sen_get_drvdev_tge_chsft(id);

	if (chsft == CTL_SEN_IGNORE) {
		return inte_rt;
	}

	if ((waited_flag & CTL_SEN_INTE_TGE_VD) == CTL_SEN_INTE_TGE_VD) {
		sen_tge_isr_vd_flag[chsft] = FALSE;
		while (1) {
			if (sen_tge_isr_vd_flag[chsft] == TRUE) {
				inte_rt = CTL_SEN_INTE_VD;
				break;
			}
			if (count > g_ctl_sen_init_obj[id]->init_ext_obj.if_info.timeout_ms) {
				CTL_SEN_DBG_ERR("timeout\r\n");
				inte_rt = CTL_SEN_INTE_ABORT;
				break;
			}
			ctl_sen_delayms(1);
			count++;
		}
	}

	return inte_rt;
}


static KDRV_TGE_MODE_SEL sensor_conv2_tge_mode_info(CTL_SEN_ID id)
{
	KDRV_TGE_MODE_SEL tge_mode = KDRV_MODE_MASTER;
	UINT32 ch = get_tge_ch(id);
	CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_info;
	ER rt_drv;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return KDRV_MODE_MASTER;
	}

	memset((void *)&signal_info, 0, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));

	rt_drv = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_SIGNAL, &signal_info);
	if (rt_drv != E_OK) {
		CTL_SEN_DBG_ERR("get sensor signal fail\r\n");
		return KDRV_MODE_MASTER;
	}

	// according to sensor mode determines TGE mode
	switch (signal_info.type) {
	case CTL_SEN_SIGNAL_SLAVE:
		tge_mode = KDRV_MODE_MASTER;
		break;
	case CTL_SEN_SIGNAL_MASTER:
		if ((ch != KDRV_TGE_VDHD_CH1) && (ch != KDRV_TGE_VDHD_CH2) && (ch != KDRV_TGE_VDHD_CH3) && (ch != KDRV_TGE_VDHD_CH4)) {
			CTL_SEN_DBG_ERR("Tge not support sensor id %d ch 0x%.8x in TGE_MODE_SLAVE\r\n", id, ch);
			tge_mode = KDRV_MODE_MASTER;
		} else {
			switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
			case CTL_SEN_IF_TYPE_PARALLEL:
				tge_mode = KDRV_MODE_SLAVE_TO_PAD;
				break;
			case CTL_SEN_IF_TYPE_MIPI:
				tge_mode = KDRV_MODE_SLAVE_TO_CSI;
				break;
			case CTL_SEN_IF_TYPE_SLVSEC:
				tge_mode = KDRV_MODE_SLAVE_TO_SLVSEC;
				break;
			default:
				CTL_SEN_DBG_ERR("sensor id %d sensor if type %d error\r\n", id, g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
				break;
			}
		}
		break;
	default:
		CTL_SEN_DBG_ERR("sensor id %d sensor signal type %d error\r\n", id, signal_info.type);
		break;
	}

	return tge_mode;
}


static INT32 sen_ctrl_if_tge_set_mode_cfg(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	INT32 kdrv_rt = 0, rt = 0;
	ER rt_er;
	BOOL in_param_error = FALSE;
	KDRV_TGE_VDHD_INFO vdhd_info;
	KDRV_TGE_SWAP_INFO swap_info;
	KDRV_TGE_BP_INFO bp;
	CTL_SENDRV_GET_MODE_TGE_PARAM tge_info;
	CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_info;
	KDRV_TGE_SIE_VD_INFO tge_vd_info;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	memset((void *)&vdhd_info, 0, sizeof(KDRV_TGE_VDHD_INFO));
	memset((void *)&swap_info, 0, sizeof(KDRV_TGE_SWAP_INFO));
	memset((void *)&tge_info, 0, sizeof(CTL_SENDRV_GET_MODE_TGE_PARAM));
	memset((void *)&signal_info, 0, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
	memset((void *)&tge_vd_info, 0, sizeof(KDRV_TGE_SIE_VD_INFO));

	tge_info.mode = mode;
	rt_er = E_OK;
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_TGE, &tge_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_SIGNAL, &signal_info);
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("id %d get sendrv info fail\r\n", id);
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	vdhd_info.mode = sensor_conv2_tge_mode_info(id);
	vdhd_info.vd_period = tge_info.signal.hd_period * tge_info.signal.vd_period;
	vdhd_info.vd_assert = tge_info.signal.vd_sync * tge_info.signal.hd_period;
	vdhd_info.vd_frontblnk = 0;
	vdhd_info.hd_period = tge_info.signal.hd_period;
	vdhd_info.hd_assert = tge_info.signal.hd_sync;
	vdhd_info.hd_cnt = 0;
	vdhd_info.vd_phase = (KDRV_TGE_PHASE_SEL)signal_info.info.vd_phase;
	vdhd_info.hd_phase = (KDRV_TGE_PHASE_SEL)signal_info.info.hd_phase;
	vdhd_info.vd_inverse = (BOOL)signal_info.info.vd_inv;
	vdhd_info.hd_inverse = (BOOL)signal_info.info.hd_inv;
	kdrv_rt |=  kdrv_tge_set(get_tge_ch(get_tge_trig_id(id)), KDRV_TGE_PARAM_IPL_VDHD, (void *)&vdhd_info);
	bp.bp_line = 0;
	kdrv_rt |=  kdrv_tge_set(get_tge_ch(get_tge_trig_id(id)), KDRV_TGE_PARAM_IPL_VD_BP, (void *)&bp);
	swap_info.swap_enble = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.swap;
	kdrv_rt |=  kdrv_tge_set(get_tge_ch(get_tge_swap_func_id(id)), KDRV_TGE_PARAM_IPL_SWAP, (void *)&swap_info);
	tge_vd_info.vd_src = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.sie_vd_src;
	if ((tge_vd_info.vd_src == KDRV_TGE_SIE1_VD_SRC_CH1) || (tge_vd_info.vd_src == KDRV_TGE_SIE1_VD_SRC_CH3)) {
		kdrv_rt |=  kdrv_tge_set(get_tge_ch(KDRV_TGE_SIE1_IN), KDRV_TGE_PARAM_IPL_SIE_VDHD_SRC, (void *)&tge_vd_info);
	} else if ((tge_vd_info.vd_src == KDRV_TGE_SIE3_VD_SRC_CH5) || (tge_vd_info.vd_src == KDRV_TGE_SIE3_VD_SRC_CH7)) {
		kdrv_rt |=  kdrv_tge_set(get_tge_ch(KDRV_TGE_SIE3_IN), KDRV_TGE_PARAM_IPL_SIE_VDHD_SRC, (void *)&tge_vd_info);
	} else {
		CTL_SEN_DBG_ERR("tge_vd_info.vd_src %d error\r\n", tge_vd_info.vd_src);
		in_param_error = TRUE;
	}

#if 0
	CTL_SEN_DBG_IND("KDRV_TGE_PARAM_IPL_VDHD id %d ch 0x%.8x mode %d vd %d %d %d hd %d %d %d ph %d %d inv %d %d\r\n", id, get_tge_ch(id)
					, vdhd_info.mode, vdhd_info.vd_period, vdhd_info.vd_assert, vdhd_info.vd_frontblnk, vdhd_info.hd_period, vdhd_info.hd_assert, vdhd_info.hd_cnt
					, vdhd_info.vd_phase, vdhd_info.hd_phase, vdhd_info.vd_inverse, vdhd_info.hd_inverse);
	CTL_SEN_DBG_IND("KDRV_TGE_PARAM_IPL_SWAP %d\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.swap);
#endif

	if (in_param_error) {
		rt = CTL_SEN_E_IN_PARAM;
	} else if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("id %d kdrv_rt %d error\r\n", id, kdrv_rt);
		rt = CTL_SEN_E_KDRV_TGE;
	}
	return rt;
}

static INT32 sen_ctrl_if_tge_set_cfg(CTL_SEN_ID id, UINT32 cfg_id, void *value)
{
	INT32 kdrv_rt, rt = 0;
	UINT32 ch = 0;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	if (cfg_id == KDRV_TGE_PARAM_IPL_VDHD) {
		ch = get_tge_trig_id(id);
	} else if (cfg_id == KDRV_TGE_PARAM_IPL_VD_BP) {
		ch = get_tge_trig_id(id);
	} else if (cfg_id == KDRV_TGE_PARAM_IPL_PAUSE) {
		ch = get_tge_trig_id(id);
	} else if (cfg_id == KDRV_TGE_PARAM_IPL_SWAP) {
		ch = get_tge_swap_func_id(id);
	}

	kdrv_rt = kdrv_tge_set(get_tge_ch(ch), cfg_id, value);

	if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("id %d ch 0x%.8x cfg_id %d error\r\n", id, ch, cfg_id);
		rt += CTL_SEN_E_KDRV_TGE;
	}
	return rt;
}

static INT32 sen_ctrl_if_tge_get_cfg(CTL_SEN_ID id, UINT32 cfg_id, void *value)
{
	INT32 kdrv_rt, rt = 0;
	UINT32 ch = 0;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}


	if (cfg_id == KDRV_TGE_PARAM_IPL_VDHD) {
		ch = get_tge_trig_id(id);
	} else if (cfg_id == KDRV_TGE_PARAM_IPL_VD_BP) {
		ch = get_tge_trig_id(id);
	} else if (cfg_id == KDRV_TGE_PARAM_IPL_PAUSE) {
		ch = get_tge_trig_id(id);
	} else if (cfg_id == KDRV_TGE_PARAM_IPL_SWAP) {
		ch = get_tge_swap_func_id(id);
	}

	kdrv_rt = kdrv_tge_get(get_tge_ch(ch), cfg_id, value);
	if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("id %d ch 0x%.8x cfg_id %d error\r\n", id, ch, cfg_id);
		rt += CTL_SEN_E_KDRV_TGE;
	}
	return rt;
}
static CTL_SEN_CTRL_IF_TGE sen_ctrl_if_tge_tab = {

	sen_ctrl_if_tge_open,
	sen_ctrl_if_tge_close,

	sen_ctrl_if_tge_start,
	sen_ctrl_if_tge_stop,
	sen_ctrl_if_tge_reset,

	sen_ctrl_if_tge_wait_interrupt,

	sen_ctrl_if_tge_set_mode_cfg,

	sen_ctrl_if_tge_set_cfg,
	sen_ctrl_if_tge_get_cfg,
};

CTL_SEN_CTRL_IF_TGE *sen_ctrl_if_tge_get_obj(void)
{
	return &sen_ctrl_if_tge_tab;
}
