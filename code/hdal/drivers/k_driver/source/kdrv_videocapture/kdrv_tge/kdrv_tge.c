/**
    @file       kdrv_tge.c
    @ingroup	Predefined_group_name

    @brief      tge device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kwrap/type.h"
#include <kdrv_builtin/kdrv_builtin.h>
//#include "kwrap/flag.h"  // conflict with tge_platform.h
#include "tge_platform.h"
#include "tge_lib.h"
#include "kdrv_tge_int.h"
#include "kdrv_tge_int_dbg.h"

static BOOL g_kdrv_tge_init_flg = FALSE;
static BOOL g_kdrv_tge_is_fastboot = FALSE;		// indicate if flow is fastboot
static UINT32 g_kdrv_tge_fastboot_trig_flg = 0;	// indicate if current state is not trigger yet
static KDRV_TGE_PRAM g_kdrv_tge_info = {0};
static UINT32 g_kdrv_tge_update = 0;
static TGE_OPENOBJ g_tge_drv_obj;
static BOOL g_tge_start_statue = 0;

#if 0
#endif

/**
	internel used
*/
static ER kdrv_tge_chk_vaild_ch(UINT32 ch, UINT32 vaild_mask)
{
	if (((ch & vaild_mask) != 0) && ((ch & (~vaild_mask)) == 0)) {
		return E_OK;
	} else {
		DBG_ERR("TGE channel error(0x%x), that should be the bits in mask 0x%x\r\n", ch, vaild_mask);
		return E_PAR;
	}
}

static ER kdrv_tge_trig_vd_hd(UINT32 ch, BOOL ch_enable)
{
	UINT32 i;
	TGE_VD_RST_INFO param = {0};
	TGE_CHANGE_RST_SEL fun_sel = 0;

	DBG_FUNC("ch = 0x%x, en = %u\r\n", ch, ch_enable);

	if (kdrv_tge_chk_vaild_ch(ch, KDRV_TGE_VDHD_CH_ALL) != E_OK) {
		return E_PAR;
	}

	if ((g_kdrv_tge_update & KDRV_TGE_VDHD_CH_ALL) == 0) {
		goto rst_done;
	}

	for (i = 0; i < KDRV_TGE_VDHD_CH_MAX; i++) {

		if (ch & (1 << i)) {

			if (g_kdrv_tge_update & (1 << (KDRV_TGE_UPDATE_BIT_BASE_VDHD_PARAM+i))) {
				fun_sel |= (1 << i);

			} else {
				DBG_ERR("VDHD_CH%d is not set yet! Stop trigger it!\r\n", i+1);
			}
		}
	}
	DBG_FUNC("fun_sel = 0x%x\r\n", fun_sel);

	if (g_kdrv_tge_is_fastboot && (g_kdrv_tge_fastboot_trig_flg == 0)) {
		// fastboot do nothing before first trigger
	} else {
		param.VdRst = TGE_VD_DISABLE;
		param.Vd2Rst = TGE_VD_DISABLE;
		tge_chgRst(&param, fun_sel);

		if (ch_enable) {
			param.VdRst = TGE_VD_ENABLE;
			param.Vd2Rst = TGE_VD_ENABLE;
			tge_chgRst(&param, fun_sel);
		}
	}

rst_done:

	if (g_tge_start_statue == 0) {
		if (g_kdrv_tge_is_fastboot && (g_kdrv_tge_fastboot_trig_flg == 0)) {
			// fastboot do nothing before first trigger
		} else {
			tge_start();
		}
		g_tge_start_statue = 1;
	}

	return E_OK;
}

static ER kdrv_tge_wait_isr_event(KDRV_TGE_ISR_EVENT event, BOOL clear)
{
	DBG_FUNC("event = 0x%x, clear = %u\r\n", event, clear);
	tge_waitEvent((TGE_WAIT_EVENT_SEL)event, clear);

	return E_OK;
}

static void kdrv_tge_lock(BOOL flag)
{
/*
	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, kdrv_ipl_get_flag_id(FLG_ID_KDRV_DCE), (KDRV_IPL_DCE_LOCK << id), TWF_CLR);
	} else {
		set_flg(kdrv_ipl_get_flag_id(FLG_ID_KDRV_DCE), (KDRV_IPL_DCE_LOCK << id));
	}
*/
}

/**
	tge isr cb
*/
static void kdrv_tge_isr_cb(UINT32 intstatus)
{
	DBG_IND("intr = 0x%x, fp = %u\r\n", intstatus, (UINT32)g_kdrv_tge_info.isrcb_fp);

	if (g_kdrv_tge_info.isrcb_fp != NULL) {
		g_kdrv_tge_info.isrcb_fp(intstatus, NULL);
	}
}

static CHAR *kdrv_tge_dump_str_mode(KDRV_TGE_MODE_SEL mode)
{
	CHAR *str_mode[4 + 1] = {
		"Master",
		"Slave-Pad",
		"Slave-CSI",
		"Slave-SLVSEC",
		"ERROR",
	};

	if (mode >= 4) {
		return str_mode[4];
	}
	return str_mode[mode];
}

static CHAR *kdrv_tge_dump_str_phase(KDRV_TGE_PHASE_SEL phase)
{
	CHAR *str_phase[2 + 1] = {
		"Rising",
		"Falling",
		"ERROR",
	};

	if (phase >= 2) {
		return str_phase[2];
	}
	return str_phase[phase];
}

static CHAR *kdrv_tge_dump_str_booling(BOOL inverse)
{
	CHAR *str_booling[2 + 1] = {
		"FALSE",
		"TRUE",
		"ERROR",
	};

	if (inverse >= 2) {
		return str_booling[2];
	}
	return str_booling[inverse];
}

#if 0
static CHAR *kdrv_tge_dump_str_signal_type(KDRV_TGE_MSH_SIGNAL_TYPE type)
{
	CHAR *str_signal[2 + 1] = {
		"PULSE",
		"LEVEL",
		"ERROR",
	};

	if (type >= 2) {
		return str_signal[2];
	}
	return str_signal[type];
}

static CHAR *kdrv_tge_dump_str_shutter_ch(UINT32 ch)
{
	CHAR *str_ch[4 + 1] = {
		"CHA_Close",
		"CHA_open",
		"CHB_Close",
		"CHB_open",
		"ERROR",
	};

	if (ch >= 4) {
		return str_ch[4];
	}
	return str_ch[ch];
}


static CHAR *kdrv_tge_dump_str_trig_src(KDRV_TGE_TRG_SRC src)
{
	CHAR *str_src[2 + 1] = {
		"Sync-VD",
		"Ext-Signal",
		"ERROR",
	};

	if (src >= 2) {
		return str_src[2];
	}
	return str_src[src];
}

static CHAR *kdrv_tge_dump_str_sie_src(KDRV_TGE_SIE_VD_SRC src)
{
	CHAR *str_src[4 + 1] = {
		"CH1",
		"CH3",
		"CH5",
		"CH7",
		"ERROR",
	};

	if (src >= 4) {
		return str_src[4];
	}
	return str_src[src];
}
#endif


#if 0
#endif

static ER kdrv_tge_set_opencfg(UINT32 ch, void* data)
{
	KDRV_TGE_OPENCFG kdrv_tge_open_obj;

	DBG_FUNC("\r\n");

	kdrv_tge_open_obj = *(KDRV_TGE_OPENCFG *)data;
	g_tge_drv_obj.TgeClkSel = (TGE_CLKSRC_SEL)kdrv_tge_open_obj.tge_clock_sel;
	g_tge_drv_obj.TgeClkSel2 = (TGE_CLKSRC_SEL)kdrv_tge_open_obj.tge_clock_sel2;

	return E_OK;
}

static ER kdrv_tge_set_vdhd_param(UINT32 ch, void* data)
{
	KDRV_TGE_VDHD_INFO *vd_hd_info = (KDRV_TGE_VDHD_INFO *)data;
	TGE_VD_PHASE phase = {0};
	TGE_VD_INV vd_inv = {0};
	TGE_MODE_SEL signal_src;
	TGE_VDHD_INFO signal_info = {0};
	UINT32 i;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	if (kdrv_tge_chk_vaild_ch(ch, KDRV_TGE_VDHD_CH_ALL) != E_OK) {
		return E_PAR;
	}

	for (i = 0; i < KDRV_TGE_VDHD_CH_MAX; i++) {

		if ((ch & (1 << i)) == 0) {
			continue;
		}

		g_kdrv_tge_info.vdhd_info[i] = *vd_hd_info;

		phase.VdPhase = vd_hd_info->vd_phase;
		phase.HdPhase = vd_hd_info->hd_phase;
		tge_chgParam(&phase, TGE_CHG_VD_PHASE+i);

		vd_inv.bVdInv = vd_hd_info->vd_inverse;
		vd_inv.bHdInv = vd_hd_info->hd_inverse;
		tge_chgParam(&vd_inv, TGE_CHG_VD_INV+i);

		signal_src = vd_hd_info->mode;
		tge_chgParam(&signal_src, TGE_CHG_VD_MODE+i);

		signal_info.uiVdPeriod = vd_hd_info->vd_period;
		signal_info.uiVdAssert = vd_hd_info->vd_assert;
		signal_info.uiVdFrontBlnk = vd_hd_info->vd_frontblnk;
		signal_info.uiHdPeriod = vd_hd_info->hd_period;
		signal_info.uiHdAssert = vd_hd_info->hd_assert;
		signal_info.uiHdCnt = vd_hd_info->hd_cnt;
		tge_chgParam(&signal_info, TGE_CHG_VDHD+i);

		g_kdrv_tge_update |= (1 << (KDRV_TGE_UPDATE_BIT_BASE_VDHD_PARAM+i));
	}

	return E_OK;
}

static ER kdrv_tge_set_bp_line(UINT32 ch, void* data)
{
	KDRV_TGE_BP_INFO *bp_info = (KDRV_TGE_BP_INFO *)data;
	UINT32 bp_line;
	UINT32 i;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	if (kdrv_tge_chk_vaild_ch(ch, KDRV_TGE_VDHD_CH_ALL) != E_OK) {
		return E_PAR;
	}

	for (i = 0; i < KDRV_TGE_VDHD_CH_MAX; i++) {

		if ((ch & (1 << i)) == 0) {
			continue;
		}

		g_kdrv_tge_info.tge_bp[i] = *bp_info;

		bp_line = bp_info->bp_line;
		tge_chgParam(&bp_line, TGE_CHG_VD_BP+i);
	}

	return E_OK;
}

static ER kdrv_tge_set_pause(UINT32 ch, void* data)
{
	KDRV_TGE_TIMING_PAUSE_INFO *pause_info = (KDRV_TGE_TIMING_PAUSE_INFO *)data;
	TGE_TIMING_PAUSE_INFO set_pause_info;
	UINT32 i;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	if (kdrv_tge_chk_vaild_ch(ch, KDRV_TGE_VDHD_CH_ALL) != E_OK) {
		return E_PAR;
	}

	for (i = 0; i < KDRV_TGE_VDHD_CH_MAX; i++) {

		if ((ch & (1 << i)) == 0) {
			continue;
		}

		g_kdrv_tge_info.tge_pause[i] = *pause_info;

		DBG_FUNC("id %d, pause info: %d/%d\r\n", ch, pause_info->vd_pause, pause_info->hd_pause);
		set_pause_info.bVdPause = pause_info->vd_pause;
		set_pause_info.bHdPause = pause_info->hd_pause;
		tge_chgParam(&set_pause_info, TGE_CHG_TIMING_VD_PAUSE+i);
	}

	return E_OK;
}

static ER kdrv_tge_set_signal_swap(UINT32 ch, void* data)
{
	KDRV_TGE_SWAP_INFO *swap_info = (KDRV_TGE_SWAP_INFO *)data;
	UINT32 i;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	if (kdrv_tge_chk_vaild_ch(ch, KDRV_TGE_SWAP_CH_ALL) != E_OK) {
		return E_PAR;
	}

	for (i = 0; i < KDRV_TGE_SWAP_CH_MAX; i++) {

		if ((ch & (1 << i)) == 0) {
			continue;
		}

		if (swap_info->swap_enble) {
			DBG_ERR("Not support swap !!\r\n");
			return E_PAR;
		}

		g_kdrv_tge_info.tge_swap_vdhd_pin[i] = *swap_info;
	}

	return E_OK;
}

static ER kdrv_tge_set_sie_vdhd_src(UINT32 ch, void* data)
{
	KDRV_TGE_SIE_VD_INFO *sie_vd_info = (KDRV_TGE_SIE_VD_INFO *)data;
	UINT32 i;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	if (kdrv_tge_chk_vaild_ch(ch, KDRV_TGE_SIE_VD_SRC_CH_ALL) != E_OK) {
		return E_PAR;
	}

	for (i = 0; i < KDRV_TGE_SIE_SRC_CH_MAX; i++) {

		if ((ch & (1 << i)) == 0) {
			continue;
		}

		if (((1 << i) == KDRV_TGE_SIE1_IN && sie_vd_info->vd_src != KDRV_TGE_SIE1_VD_SRC_CH1) ||
			((1 << i) == KDRV_TGE_SIE3_IN && sie_vd_info->vd_src != KDRV_TGE_SIE3_VD_SRC_CH5)) {
			DBG_ERR("Not support sie source selection, fix to TGE1->SIE1, TGE2->SIE2 !!\r\n");
			return E_PAR;
		}

		g_kdrv_tge_info.tge_to_sie_vd_ch[i] = *sie_vd_info;
	}

	return E_OK;
}

static ER kdrv_tge_set_clksrc(UINT32 ch, void* data)
{
	KDRV_TGE_CLK_SRC_SEL *clk_src_info = (KDRV_TGE_CLK_SRC_SEL *)data;
	TGE_CLKSRC_SEL clk_data;
	UINT32 i;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	if (kdrv_tge_chk_vaild_ch(ch, KDRV_TGE_CLK_ID_ALL) != E_OK) {
		return E_PAR;
	}

	for (i = 0; i < KDRV_TGE_CLK_ID_MAX; i++) {

		if ((ch & (1 << i)) == 0) {
			continue;
		}

		g_kdrv_tge_info.tge_clk[i] = *clk_src_info;

		clk_data = (TGE_CLKSRC_SEL)clk_src_info->clk_src_info;
		tge_chgParam(&clk_data, TGE_CHG_CLK1+i);
	}

	return E_OK;
}

static ER kdrv_tge_set_isrcb(UINT32 ch, void* data)
{
	DBG_FUNC("\r\n");

	g_kdrv_tge_info.isrcb_fp = (KDRV_TGE_ISRCB)data;

	return E_OK;
}


static KDRV_TGE_SET_FP kdrv_tge_set_fp[KDRV_TGE_PARAM_MAX] = {
	kdrv_tge_set_opencfg,
	kdrv_tge_set_vdhd_param,
	kdrv_tge_set_bp_line,
	kdrv_tge_set_pause,
	kdrv_tge_set_signal_swap,
	kdrv_tge_set_sie_vdhd_src,
	kdrv_tge_set_clksrc,
	kdrv_tge_set_isrcb,
};

#if 0
#endif

static ER kdrv_tge_get_vdhd_param(UINT32 ch, void* data)
{
	UINT32 idx = 0;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	switch (ch) {
	case KDRV_TGE_VDHD_CH1:
		idx = 0;
		break;
	case KDRV_TGE_VDHD_CH2:
		idx = 1;
		break;
	default:
        DBG_ERR("ch(0x%x) error!! it should be KDRV_TGE_VDHD_CH_1 ~ 8!!\r\n", ch);
        return E_PAR;
	}

	*(KDRV_TGE_VDHD_INFO *)data = g_kdrv_tge_info.vdhd_info[idx];

	return E_OK;
}

static ER kdrv_tge_get_bp_line(UINT32 ch, void* data)
{
	UINT32 idx = 0;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	switch (ch) {
	case KDRV_TGE_VDHD_CH1:
		idx = 0;
		break;
	case KDRV_TGE_VDHD_CH2:
		idx = 1;
		break;
	default:
        DBG_ERR("ch(0x%x) error!! it should be KDRV_TGE_VDHD_CH_1 ~ 8!!\r\n", ch);
        return E_PAR;
	}

	*(KDRV_TGE_BP_INFO *)data = g_kdrv_tge_info.tge_bp[idx];

	return E_OK;
}

static ER kdrv_tge_get_pause(UINT32 ch, void* data)
{
	UINT32 idx = 0;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	switch (ch) {
	case KDRV_TGE_VDHD_CH1:
		idx = 0;
		break;
	case KDRV_TGE_VDHD_CH2:
		idx = 1;
		break;
	default:
        DBG_ERR("ch(0x%x) error!! it should be KDRV_TGE_VDHD_CH_1 ~ 8!!\r\n", ch);
        return E_PAR;
	}

	*(KDRV_TGE_TIMING_PAUSE_INFO *)data = g_kdrv_tge_info.tge_pause[idx];

	return E_OK;
}

static ER kdrv_tge_get_signal_swap(UINT32 ch, void* data)
{
	UINT32 idx = 0;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	switch (ch) {
	case KDRV_TGE_SWAP_CH15:
		idx = 0;
		break;
	case KDRV_TGE_SWAP_CH26:
		idx = 1;
		break;
	default:
        DBG_ERR("ch(0x%x) error!! it should be KDRV_TGE_SWAP_CH15/KDRV_TGE_SWAP_CH26!!\r\n", ch);
        return E_PAR;
	}

	*(KDRV_TGE_SWAP_INFO *)data = g_kdrv_tge_info.tge_swap_vdhd_pin[idx];

	return E_OK;
}

static ER kdrv_tge_get_sie_vdhd_src(UINT32 ch, void* data)
{
	UINT32 idx = 0;

	DBG_FUNC("ch = 0x%x\r\n", ch);

	switch (ch) {
	case KDRV_TGE_SIE1_IN:
		idx = 0;
		break;
	case KDRV_TGE_SIE3_IN:
		idx = 1;
		break;
	default:
        DBG_ERR("ch(0x%x) error!! it should be KDRV_TGE_SIE1_IN/KDRV_TGE_SIE3_IN!!\r\n", ch);
        return E_PAR;
	}

	*(KDRV_TGE_SIE_VD_INFO *)data = g_kdrv_tge_info.tge_to_sie_vd_ch[idx];

	return E_PAR;
}

static KDRV_TGE_GET_FP kdrv_tge_get_fp[KDRV_TGE_PARAM_MAX] = {
	NULL,
	kdrv_tge_get_vdhd_param,
	kdrv_tge_get_bp_line,
	kdrv_tge_get_pause,
	kdrv_tge_get_signal_swap,
	kdrv_tge_get_sie_vdhd_src,
	NULL,
	NULL,
};

#if 0
#endif

INT32 kdrv_tge_init(void)
{
	DBG_FUNC("\r\n");

#if defined __FREERTOS
	tge_platform_create_resource();
#endif

	return E_OK;
}

INT32 kdrv_tge_uninit(void)
{
	DBG_FUNC("\r\n");
	return E_OK;
}

INT32 kdrv_tge_open(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;
	TGE_MODE_PARAM temp = {0};

	DBG_FUNC("chip = 0x%x, eng = 0x%x\r\n", chip, engine);

	if (engine != KDRV_VDOCAP_TGE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", engine);
		return E_ID;
	}

	kdrv_tge_lock(TRUE);

#if defined(__KERNEL__)
	g_kdrv_tge_is_fastboot = kdrv_builtin_is_fastboot();
#endif

	if (g_kdrv_tge_init_flg == FALSE) {
		if (g_kdrv_tge_is_fastboot && (g_kdrv_tge_fastboot_trig_flg == 0)) {
			// fastboot do nothing before first trigger
		} else {
			rt = tge_open(&g_tge_drv_obj);
		}
	}

	if (rt == E_OK) {
		// update init flag
		g_kdrv_tge_init_flg = TRUE;
	}

	if (g_kdrv_tge_is_fastboot && (g_kdrv_tge_fastboot_trig_flg == 0)) {
		// fastboot do nothing before first trigger
	} else {
		temp.uiIntrpEn = TGE_INT_ALL;
		tge_setMode(&temp);
	}

	g_kdrv_tge_info.tge_to_sie_vd_ch[1].vd_src = KDRV_TGE_SIE3_VD_SRC_CH5;

	g_tge_start_statue = 0;

	kdrv_tge_lock(FALSE);

	return rt;
}

INT32 kdrv_tge_close(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;

	DBG_FUNC("chip = 0x%x, eng = 0x%x\r\n", chip, engine);

	if (engine != KDRV_VDOCAP_TGE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", engine);
		return E_ID;
	}

	kdrv_tge_lock(TRUE);

	if (g_kdrv_tge_init_flg == TRUE) {

		rt = tge_close();
		if (rt == E_OK) {
			g_kdrv_tge_init_flg = FALSE;
		}
	}

	g_kdrv_tge_update = 0;

	kdrv_tge_lock(FALSE);

	return rt;
}

INT32 kdrv_tge_set(UINT32 id, KDRV_TGE_PARAM_ID item, void* data)
{
	UINT32 engine = KDRV_DEV_ID_ENGINE(id);
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);
	ER rt;

	DBG_IND("id = 0x%x, item = %u\r\n", id, item);

	if (engine != KDRV_VDOCAP_TGE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", engine);
		return E_ID;
	}

	rt = kdrv_tge_set_fp[item](channel, data);

	return rt;
}

INT32 kdrv_tge_get(UINT32 id, KDRV_TGE_PARAM_ID item, void* data)
{
	UINT32 engine = KDRV_DEV_ID_ENGINE(id);
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);
	ER rt;

	DBG_IND("id = 0x%x, item = %u\r\n", id, item);

	if (engine != KDRV_VDOCAP_TGE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", engine);
		return E_ID;
	}

	rt = kdrv_tge_get_fp[item](channel, data);

	return rt;
}

INT32 kdrv_tge_trigger(UINT32 id, KDRV_TGE_TRIGGER_PARAM *p_tge_param, KDRV_TGE_CALLBACK_FUNC *p_cb_func, void *p_user_data)
{
	KDRV_TGE_TRIG_INFO *trig_info = (KDRV_TGE_TRIG_INFO *)p_user_data;
	UINT32 ch = KDRV_DEV_ID_CHANNEL(id);
	UINT32 i;

	DBG_FUNC("id = 0x%x, ch = %u, trig_type = %u\r\n", id, ch, trig_info->trig_type);

	// set isr_cb before trigger instead of open, to prevent early isr occured in fastboot flow
	tge_reg_isr_cb(kdrv_tge_isr_cb);

	if (trig_info->trig_type == KDRV_TGE_TRIG_VDHD) {
		kdrv_tge_trig_vd_hd(ch, trig_info->ch_enable);

	} else if (trig_info->trig_type == KDRV_TGE_TRIG_STOP) {
		tge_pause();
		g_tge_start_statue = 0;
		g_kdrv_tge_fastboot_trig_flg = 1;	// set trig_flg after stop to prevent tge reset at trig. dont do this after trig vdhd because ctl_sen might trig tge several times at boot, causing tge reset eventually

	} else {
		DBG_ERR("trigger type error, %d\r\n", trig_info->trig_type);
	}

	if (trig_info->wait_end_enable == TRUE && trig_info->trig_type != KDRV_TGE_TRIG_STOP && trig_info->ch_enable == TRUE) {

		for (i = 0; i < KDRV_TGE_VDHD_CH_MAX; i++) {
			if (trig_info->wait_event & (TGE_WAIT_VD << i)) {
				kdrv_tge_wait_isr_event((TGE_WAIT_VD << i), TRUE);
			}
		}

		for (i = 0; i < KDRV_TGE_VDHD_CH_MAX; i++) {
			if (trig_info->wait_event & (TGE_WAIT_VD_BP1 << i)) {
				kdrv_tge_wait_isr_event((TGE_WAIT_VD_BP1 << i), TRUE);
			}
		}
	}

	return E_OK;
}

void kdrv_tge_dump_info(void)
{
	UINT32 i;

	if (g_kdrv_tge_init_flg == 0) {
		DBG_DUMP("\r\n[KDRV TGE] not open\r\n");
		return;
	}

	DBG_DUMP("\r\n[TGE]\r\n");

	/**
		input info
	*/
	DBG_DUMP("\r\n-----VD/HD info-----\r\n");
	DBG_DUMP("%3s%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s\r\n", "ch", "mode", "VD Period", "VD assert", "VD Blank", "HD period", "HD assert", "HD count", "VD phase", "HD phase", "VD Inverse", "HD Inverse");

	for (i = 0; i < KDRV_TGE_VDHD_CH_MAX; i++) {
		if (g_kdrv_tge_update & (1 << (KDRV_TGE_UPDATE_BIT_BASE_VDHD_PARAM+i))) {

			DBG_DUMP("%3d%12s%12d%12d%12d%12d%12d%12d%12s%12s%12s%12s\r\n", i+1
				 , kdrv_tge_dump_str_mode(g_kdrv_tge_info.vdhd_info[i].mode)
				 , g_kdrv_tge_info.vdhd_info[i].vd_period, g_kdrv_tge_info.vdhd_info[i].vd_assert, g_kdrv_tge_info.vdhd_info[i].vd_frontblnk
				 , g_kdrv_tge_info.vdhd_info[i].hd_period, g_kdrv_tge_info.vdhd_info[i].hd_assert, g_kdrv_tge_info.vdhd_info[i].hd_cnt
				 , kdrv_tge_dump_str_phase(g_kdrv_tge_info.vdhd_info[i].vd_phase), kdrv_tge_dump_str_phase(g_kdrv_tge_info.vdhd_info[i].hd_phase)
				 , kdrv_tge_dump_str_booling(g_kdrv_tge_info.vdhd_info[i].vd_inverse), kdrv_tge_dump_str_booling(g_kdrv_tge_info.vdhd_info[i].hd_inverse));
		}
	}

	DBG_DUMP("\r\n-----Break point line-----\r\n");
	DBG_DUMP("%8s%8s" /*"%8s%8s%8s%8s%8s%8s"*/ "\r\n", "ch1", "ch2"/*, "ch3", "ch4", "ch5", "ch6", "ch7", "ch8"*/);
	DBG_DUMP("%8d%8d" /*"%8d%8d%8d%8d%8d%8d"*/ "\r\n"
		 , g_kdrv_tge_info.tge_bp[0].bp_line, g_kdrv_tge_info.tge_bp[1].bp_line/*, g_kdrv_tge_info.tge_bp[2].bp_line, g_kdrv_tge_info.tge_bp[3].bp_line
		 , g_kdrv_tge_info.tge_bp[4].bp_line, g_kdrv_tge_info.tge_bp[5].bp_line, g_kdrv_tge_info.tge_bp[6].bp_line, g_kdrv_tge_info.tge_bp[7].bp_line*/);

}

