/*
    Sensor Interface kflow library - ctrl sensor interface (global)

    Sensor Interface KFLOW library.

    @file       sen_ctrl_if_glb.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.
*/
#include "sen_ctrl_if_int.h"

static void sen_mclk_en_1(CTL_SEN_CLK_SEL mclksel, BOOL en);
static void sen_mclk_en_2(CTL_SEN_CLK_SEL mclksel, BOOL en);
static void sen_mclk_en_3(CTL_SEN_CLK_SEL mclksel, BOOL en);
static void sen_mclk_en_4(CTL_SEN_CLK_SEL mclksel, BOOL en);
static void sen_mclk_en_5(CTL_SEN_CLK_SEL mclksel, BOOL en);
static void sen_mclk_en_6(CTL_SEN_CLK_SEL mclksel, BOOL en);
static void sen_mclk_en_7(CTL_SEN_CLK_SEL mclksel, BOOL en);
static void sen_mclk_en_8(CTL_SEN_CLK_SEL mclksel, BOOL en);

typedef struct {
	BOOL isset;
	CTL_SEN_CLKSRC_SEL clksrc_sel;
} SEN_MCLKSRCSYNC_INFO;

static SEN_MCLKSRCSYNC_INFO mclksrc_sync_info[CTL_SEN_ID_MAX] = {0};
static BOOL is_glb_opened[CTL_SEN_ID_MAX] = {0};

static INT32 sen_ctrl_if_glb_open(CTL_SEN_ID id)
{
	if (id == CTL_SEN_ID_1) {
		sen_uti_set_clken_cb(id, sen_mclk_en_1);
	} else if (id == CTL_SEN_ID_2) {
		sen_uti_set_clken_cb(id, sen_mclk_en_2);
	} else if (id == CTL_SEN_ID_3) {
		sen_uti_set_clken_cb(id, sen_mclk_en_3);
	} else if (id == CTL_SEN_ID_4) {
		sen_uti_set_clken_cb(id, sen_mclk_en_4);
	} else if (id == CTL_SEN_ID_5) {
		sen_uti_set_clken_cb(id, sen_mclk_en_5);
	} else if (id == CTL_SEN_ID_6) {
		sen_uti_set_clken_cb(id, sen_mclk_en_6);
	} else if (id == CTL_SEN_ID_7) {
		sen_uti_set_clken_cb(id, sen_mclk_en_7);
	} else if (id == CTL_SEN_ID_8) {
		sen_uti_set_clken_cb(id, sen_mclk_en_8);
	} else {
		CTL_SEN_DBG_ERR("id %d overflow\r\n", id);
	}

	is_glb_opened[id] = 1;
	return CTL_SEN_E_OK;
}

static INT32 sen_ctrl_if_glb_close(CTL_SEN_ID id)
{
	is_glb_opened[id] = 0;
	return CTL_SEN_E_OK;
}

static INT32 sen_ctrl_if_glb_set_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value)
{
	INT32 kdrv_rt, rt = 0;

	kdrv_rt = kdrv_ssenif_set(KDRV_SSENIF_GLB_HDL, cfg_id, value);

	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d cfg_id 0x%.8x value %d error\r\n", id, cfg_id, value);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}

	return rt;
}

static INT32 sen_ctrl_if_glb_get_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value)
{
	INT32 kdrv_rt, rt = 0;

	kdrv_rt = kdrv_ssenif_get(KDRV_SSENIF_GLB_HDL, cfg_id, (VOID *)value);

	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d cfg_id 0x%.8x error\r\n", id, (unsigned int)cfg_id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}

	return rt;
}

static void sen_set_mclksrc_sync_info(CTL_SEN_ID id, CTL_SEN_CLKSRC_SEL clksrc_sel, BOOL set)
{
	mclksrc_sync_info[id].isset = set;
	if (set) {
		mclksrc_sync_info[id].clksrc_sel = clksrc_sel;
	}
}

static SEN_MCLKSRCSYNC_INFO sen_get_mclksrc_sync_info(CTL_SEN_ID id)
{
	return mclksrc_sync_info[id];
}


static INT32 sen_pll_set_en(CTL_SEN_CLKSRC_SEL clksel, UINT32 en)
{
	KDRV_SSENIF_PARAM_ID kdrv_item;

	CTL_SEN_DBG_IND("CTL_SEN_CLKSRC_SEL %d en %d GO\r\n", clksel, en);

	if (clksel == CTL_SEN_CLKSRC_480) {
		// fixed 480MHz is always enabled
		return 0;
	}

	switch (clksel) {
	case CTL_SEN_CLKSRC_PLL5:
		kdrv_item = KDRV_SSENIF_PLL05_ENABLE;
		break;
	case CTL_SEN_CLKSRC_PLL12:
		kdrv_item = KDRV_SSENIF_PLL12_ENABLE;
		break;
	case CTL_SEN_CLKSRC_PLL18:
		kdrv_item = KDRV_SSENIF_PLL18_ENABLE;
		break;
	default:
		CTL_SEN_DBG_ERR("CTL_SEN_CLKSRC_SEL %d N.S.\r\n", clksel);
		return CTL_SEN_E_IN_PARAM;
	}
	return kdrv_ssenif_set(KDRV_SSENIF_GLB_HDL, kdrv_item, en);
}

static INT32 sen_pll_get_en(CTL_SEN_CLKSRC_SEL clksel, UINT32 *en)
{
	KDRV_SSENIF_PARAM_ID kdrv_item;

	if (clksel == CTL_SEN_CLKSRC_480) {
		// fixed 480MHz is always enabled
		return 1;
	}

	switch (clksel) {
	case CTL_SEN_CLKSRC_PLL5:
		kdrv_item = KDRV_SSENIF_PLL05_ENABLE;
		break;
	case CTL_SEN_CLKSRC_PLL12:
		kdrv_item = KDRV_SSENIF_PLL12_ENABLE;
		break;
	case CTL_SEN_CLKSRC_PLL18:
		kdrv_item = KDRV_SSENIF_PLL18_ENABLE;
		break;
	default:
		CTL_SEN_DBG_ERR("CTL_SEN_CLKSRC_SEL %d N.S.\r\n", clksel);
		return CTL_SEN_E_IN_PARAM;
	}

	return kdrv_ssenif_get(KDRV_SSENIF_GLB_HDL, kdrv_item, en);
}

static INT32 sen_pll_set_freq(CTL_SEN_CLKSRC_SEL clksel, UINT32 freq)
{
	KDRV_SSENIF_PARAM_ID kdrv_item;

	CTL_SEN_DBG_IND("CTL_SEN_CLKSRC_SEL %d GO\r\n", clksel);

	switch (clksel) {
	case CTL_SEN_CLKSRC_PLL5:
		kdrv_item = KDRV_SSENIF_PLL05_FREQUENCY;
		break;
	case CTL_SEN_CLKSRC_PLL12:
		kdrv_item = KDRV_SSENIF_PLL12_FREQUENCY;
		break;
	case CTL_SEN_CLKSRC_PLL18:
		kdrv_item = KDRV_SSENIF_PLL18_FREQUENCY;
		break;
	default:
		CTL_SEN_DBG_ERR("CTL_SEN_CLKSRC_SEL %d N.S.\r\n", clksel);
		return CTL_SEN_E_IN_PARAM;
	}
	return kdrv_ssenif_set(KDRV_SSENIF_GLB_HDL, kdrv_item, freq);
}

static INT32 sen_pll_get_freq(CTL_SEN_CLKSRC_SEL clksel, UINT32 *freq)
{
	KDRV_SSENIF_PARAM_ID kdrv_item;

	CTL_SEN_DBG_IND("CTL_SEN_CLKSRC_SEL %d GO\r\n", clksel);

	switch (clksel) {
	case CTL_SEN_CLKSRC_480:
		*freq = CTL_SEN_480M_HZ;
		return 0;
	case CTL_SEN_CLKSRC_PLL5:
		kdrv_item = KDRV_SSENIF_PLL05_FREQUENCY;
		break;
	case CTL_SEN_CLKSRC_PLL12:
		kdrv_item = KDRV_SSENIF_PLL12_FREQUENCY;
		break;
	case CTL_SEN_CLKSRC_PLL18:
		kdrv_item = KDRV_SSENIF_PLL18_FREQUENCY;
		break;
	default:
		CTL_SEN_DBG_ERR("CTL_SEN_CLKSRC_SEL %d N.S.\r\n", clksel);
		return CTL_SEN_E_IN_PARAM;
	}

	return kdrv_ssenif_get(KDRV_SSENIF_GLB_HDL, kdrv_item, freq);
}

static void sen_mclk_en(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	KDRV_SSENIF_PARAM_ID kdrv_item;
	INT32 kdrv_rt;

	CTL_SEN_DBG_IND("CTL_SEN_CLK_SEL %d enable %d GO\r\n", mclksel, en);
	if (mclksel == CTL_SEN_CLK_SEL_SIEMCLK_IGNORE) {
		return;
	}

	switch (mclksel) {
	case CTL_SEN_CLK_SEL_SIEMCLK:
		kdrv_item = KDRV_SSENIF_SIEMCLK_ENABLE;
		break;
	case CTL_SEN_CLK_SEL_SIEMCLK2:
		kdrv_item = KDRV_SSENIF_SIEMCLK2_ENABLE;
		break;
	case CTL_SEN_CLK_SEL_SIEMCLK3:
		kdrv_item = KDRV_SSENIF_SIEMCLK3_ENABLE;
		break;
	default:
		CTL_SEN_DBG_ERR("CTL_SEN_CLK_SEL %d N.S.\r\n", mclksel);
		return;
	}

	kdrv_rt = kdrv_ssenif_set(KDRV_SSENIF_GLB_HDL, kdrv_item, en);
	if (kdrv_rt) {
		CTL_SEN_DBG_ERR("CTL_SEN_CLK_SEL %d enable %d fail.\r\n", mclksel, en);
	}
}

static INT32 sen_mclk_syncfreq(UINT32 freq)
{
	INT32 kdrv_rt;

	CTL_SEN_DBG_IND("KDRV_SSENIF_SIEMCLK_12SYNC_FREQUENCY %d GO\r\n", freq);

	kdrv_rt = kdrv_ssenif_set(KDRV_SSENIF_GLB_HDL, KDRV_SSENIF_SIEMCLK_12SYNC_FREQUENCY, freq);
	if (kdrv_rt) {
		CTL_SEN_DBG_ERR("KDRV_SSENIF_SIEMCLK_12SYNC_FREQUENCY %d fail.\r\n", freq);
	}
	return kdrv_rt;
}

/*
    input : user pinmux (PIN_SENSOR_CFG)
    output : mclk pinmux (select from user pinmux)
*/
static UINT32 sen_chk_pinmux_sen1(UINT32 pinmux)
{
#define MCLK_PINNUM 4
	int i;
	UINT32 mckpinmux_tbl[MCLK_PINNUM] = {PIN_SENSOR_CFG_MCLK, PIN_SENSOR_CFG_MCLK2, PIN_SENSOR_CFG_MCLK_2ND, PIN_SENSOR_CFG_MCLK_3RD};
	UINT32 mckpinmux = 0;

	for (i = 0; i < MCLK_PINNUM; i++) {
		if ((pinmux & mckpinmux_tbl[i]) == mckpinmux_tbl[i]) {
			mckpinmux = mckpinmux_tbl[i];
			break;
		}
	}

#undef MCLK_PINNUM

	return mckpinmux;
}

/*
    input : user pinmux (PIN_SENSOR2_CFG)
    output : mclk pinmux (select from user pinmux)
*/
static UINT32 sen_chk_pinmux_sen2(UINT32 pinmux)
{
#define MCLK_PINNUM 3
	int i;
	UINT32 mckpinmux_tbl[MCLK_PINNUM] = {PIN_SENSOR2_CFG_MCLK, PIN_SENSOR2_CFG_MCLK_2ND, PIN_SENSOR2_CFG_SN3_MCLK};
	UINT32 mckpinmux = 0;

	for (i = 0; i < MCLK_PINNUM; i++) {
		if ((pinmux & mckpinmux_tbl[i]) == mckpinmux_tbl[i]) {
			mckpinmux = mckpinmux_tbl[i];
			break;
		}
	}
#undef MCLK_PINNUM

	return mckpinmux;
}

static void sen_mclk_upd_pinmux(CTL_SEN_ID id, BOOL add)
{
#define UPD_PINNUM 1
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	PIN_GROUP_CONFIG pinmux_cfg[UPD_PINNUM] = {0};
	CTL_SEN_PINMUX *pinmux = NULL;
	ER rt_er;
	UINT32 i, mckpinmux = 0;


	/* parsing mclk pinmux from user setting */
	pinmux = &init_cfg_obj.pin_cfg.pinmux;
	i = 0;
	do {
		CTL_SEN_DBG_IND("[USR] %d\r\n", pinmux->func);
		if (pinmux->func == PIN_FUNC_SENSOR) {
			mckpinmux = sen_chk_pinmux_sen1(pinmux->cfg);
			if (mckpinmux != 0) {
				break;
			}
		}
		if (pinmux->func == PIN_FUNC_SENSOR2) {
			mckpinmux = sen_chk_pinmux_sen2(pinmux->cfg);
			if (mckpinmux != 0) {
				break;
			}
		}

		pinmux = pinmux->pnext;
		i++;
	} while ((pinmux != NULL) && (i < CTL_SEN_PINMUX_MAX_NUM));

	/* update mclk pinmux */
	if (mckpinmux != 0) {
		pinmux_cfg[0].pin_function = pinmux->func;
		rt_er = nvt_pinmux_capture(pinmux_cfg, UPD_PINNUM);
		if (rt_er != E_OK) {
			CTL_SEN_DBG_ERR("get pinmux error\r\n");
			return;
		}
		if (add) {
			pinmux_cfg[0].config |= mckpinmux; // set MCLK pinmux to FUNCITON
		} else {
			pinmux_cfg[0].config &= ~(mckpinmux); // set MCLK pinmux to PINMUX
		}
		CTL_SEN_DBG_IND("[UPD] %d = 0x%.8x\r\n", pinmux_cfg[0].pin_function, (unsigned int)pinmux_cfg[0].config);
		rt_er = nvt_pinmux_update(pinmux_cfg, UPD_PINNUM);
		if (rt_er != E_OK) {
			CTL_SEN_DBG_ERR("set pinmux error\r\n");
			return;
		}
	}
}

static void sen_mclk_en_1(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	/* enable mclk */
	sen_mclk_en(mclksel, en);

	/* update mclk pinmux */
	sen_mclk_upd_pinmux(CTL_SEN_ID_1, en);
}

static void sen_mclk_en_2(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	/* enable mclk */
	sen_mclk_en(mclksel, en);

	/* update mclk pinmux */
	sen_mclk_upd_pinmux(CTL_SEN_ID_2, en);
}

static void sen_mclk_en_3(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	/* enable mclk */
	sen_mclk_en(mclksel, en);

	/* update mclk pinmux */
	sen_mclk_upd_pinmux(CTL_SEN_ID_3, en);
}

static void sen_mclk_en_4(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	/* enable mclk */
	sen_mclk_en(mclksel, en);

	/* update mclk pinmux */
	sen_mclk_upd_pinmux(CTL_SEN_ID_4, en);
}

static void sen_mclk_en_5(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	/* enable mclk */
	sen_mclk_en(mclksel, en);

	/* update mclk pinmux */
	sen_mclk_upd_pinmux(CTL_SEN_ID_5, en);
}

static void sen_mclk_en_6(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	/* enable mclk */
	sen_mclk_en(mclksel, en);

	/* update mclk pinmux */
	sen_mclk_upd_pinmux(CTL_SEN_ID_6, en);
}

static void sen_mclk_en_7(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	/* enable mclk */
	sen_mclk_en(mclksel, en);

	/* update mclk pinmux */
	sen_mclk_upd_pinmux(CTL_SEN_ID_7, en);
}

static void sen_mclk_en_8(CTL_SEN_CLK_SEL mclksel, BOOL en)
{
	/* enable mclk */
	sen_mclk_en(mclksel, en);

	/* update mclk pinmux */
	sen_mclk_upd_pinmux(CTL_SEN_ID_8, en);
}

static INT32 sen_mclk_set_src(CTL_SEN_CLK_SEL mclksel, CTL_SEN_CLKSRC_SEL clksel)
{
	KDRV_SSENIF_PARAM_ID kdrv_item;
	KDRV_SSENIFGLO_SIEMCLK_SOURCE mclksrc = KDRV_SSENIFGLO_SIEMCLK_SOURCE_480;

	CTL_SEN_DBG_IND("CTL_SEN_CLK_SEL %d CTL_SEN_CLKSRC_SEL %d GO\r\n", mclksel, clksel);

	switch (mclksel) {
	case CTL_SEN_CLK_SEL_SIEMCLK:
		kdrv_item = KDRV_SSENIF_SIEMCLK_SOURCE;
		break;
	case CTL_SEN_CLK_SEL_SIEMCLK2:
		kdrv_item = KDRV_SSENIF_SIEMCLK2_SOURCE;
		break;
	case CTL_SEN_CLK_SEL_SIEMCLK3:
		kdrv_item = KDRV_SSENIF_SIEMCLK3_SOURCE;
		break;
	default:
		CTL_SEN_DBG_ERR("CTL_SEN_CLK_SEL %d N.S.\r\n", mclksel);
		return CTL_SEN_E_IN_PARAM;
	}

	switch (clksel) {
	case CTL_SEN_CLKSRC_480:
		mclksrc = KDRV_SSENIFGLO_SIEMCLK_SOURCE_480;
		break;
	case CTL_SEN_CLKSRC_PLL5:
		mclksrc = KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5;
		break;
	case CTL_SEN_CLKSRC_PLL10:
		mclksrc = KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10;
		break;
	case CTL_SEN_CLKSRC_PLL12:
		mclksrc = KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12;
		break;
	case CTL_SEN_CLKSRC_PLL18:
		mclksrc = KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL18;
		break;
	default:
		CTL_SEN_DBG_ERR("CTL_SEN_CLKSRC_SEL %d N.S. \r\n", clksel);
		return CTL_SEN_E_IN_PARAM;
	}

	sen_uti_set_mclk_src(mclksel, clksel);

	return kdrv_ssenif_set(KDRV_SSENIF_GLB_HDL, kdrv_item, mclksrc);
}

INT32 sen_mclk_get_src(CTL_SEN_CLK_SEL mclksel, CTL_SEN_CLKSRC_SEL *clksel)
{
	*clksel = sen_uti_get_mclk_src(mclksel);

	return CTL_SEN_E_OK;
}

static INT32 sen_mclk_set_freq(CTL_SEN_CLK_SEL mclksel, UINT32 freq)
{
	KDRV_SSENIF_PARAM_ID kdrv_item;

	CTL_SEN_DBG_IND("CTL_SEN_CLK_SEL %d freq %d GO\r\n", mclksel, freq);

	switch (mclksel) {
	case CTL_SEN_CLK_SEL_SIEMCLK:
		kdrv_item = KDRV_SSENIF_SIEMCLK_FREQUENCY;
		break;
	case CTL_SEN_CLK_SEL_SIEMCLK2:
		kdrv_item = KDRV_SSENIF_SIEMCLK2_FREQUENCY;
		break;
	case CTL_SEN_CLK_SEL_SIEMCLK3:
		kdrv_item = KDRV_SSENIF_SIEMCLK3_FREQUENCY;
		break;
	default:
		CTL_SEN_DBG_ERR("CTL_SEN_CLK_SEL %d N.S.\r\n", mclksel);
		return CTL_SEN_E_IN_PARAM;
	}

	sen_uti_set_mclksel_freq(mclksel, freq);

	return kdrv_ssenif_set(KDRV_SSENIF_GLB_HDL, kdrv_item, freq);
}


static INT32 sen_ctrl_if_clk_prepare(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	// MCLK PLL selection order
#define SEN_RANK_TBL_LEN 5
	static CTL_SEN_CLKSRC_SEL sen_mclk_srcsel_rank_520[CTL_SEN_CLK_SEL_MAX][SEN_RANK_TBL_LEN] = {
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL5,   CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX,     CTL_SEN_CLKSRC_MAX}, // CTL_SEN_CLK_SEL_SIEMCLK
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL5,   CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX,     CTL_SEN_CLKSRC_MAX}, // CTL_SEN_CLK_SEL_SIEMCLK2
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL5,   CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX,     CTL_SEN_CLKSRC_MAX}, // CTL_SEN_CLK_SEL_SIEMCLK3
	};

	static CTL_SEN_CLKSRC_SEL sen_mclk_srcsel_rank_528[CTL_SEN_CLK_SEL_MAX][SEN_RANK_TBL_LEN] = {
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL5,   CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX,     CTL_SEN_CLKSRC_MAX},       // CTL_SEN_CLK_SEL_SIEMCLK
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL5,   CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX,     CTL_SEN_CLKSRC_MAX},       // CTL_SEN_CLK_SEL_SIEMCLK2
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL18,  CTL_SEN_CLKSRC_PLL5,    CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX},       // CTL_SEN_CLK_SEL_SIEMCLK3
	};

	static CTL_SEN_CLKSRC_SEL sen_mclk_srcsel_rank_528_mclk3sync[CTL_SEN_CLK_SEL_MAX][SEN_RANK_TBL_LEN] = {
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL5,   CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX,     CTL_SEN_CLKSRC_MAX},       // CTL_SEN_CLK_SEL_SIEMCLK
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL5,   CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX,     CTL_SEN_CLKSRC_MAX},       // CTL_SEN_CLK_SEL_SIEMCLK2
		{CTL_SEN_CLKSRC_480, CTL_SEN_CLKSRC_PLL5,   CTL_SEN_CLKSRC_PLL12,   CTL_SEN_CLKSRC_MAX,     CTL_SEN_CLKSRC_MAX},       // CTL_SEN_CLK_SEL_SIEMCLK3
	};

	BOOL mclk_src_sync_sel[CTL_SEN_CLK_SEL_MAX] = {0}, update_mclk = 0; // chk which mclksel must be same source (480Hz,pll5,pll12,..)
	UINT32 i, pll_en = 0, mclk_freq, pll_freq, mclksrc_sync = 0;
	INT32 kdrv_rt, rt = CTL_SEN_E_OK;
	CTL_SEN_CLK_SEL mclk_sel_sendrv;
	CTL_SEN_CLKSRC_SEL *mclk_src_sel_rank = NULL;
	CTL_SEN_CLKSRC_SEL mclk_src_sel_tmp;

	/*
	    get clk info from sendrv
	*/
	mclk_sel_sendrv = sen_uti_get_mclksel(id, mode);
	mclk_freq = sen_uti_get_mclk_freq(id, mode);

	CTL_SEN_DBG_IND("id %d mode %d CTL_SEN_CLK_SEL %d mclk_freq %d\r\n", id, mode, mclk_sel_sendrv, mclk_freq);

	/*
	    chk mclk src sync bits
	*/
	if (sen_get_mclksrc_sync_info(id).isset) {
		mclk_src_sel_tmp = sen_get_mclksrc_sync_info(id).clksrc_sel;
		update_mclk = 1;
		goto mclksrc_sel_end;
	}

	if (mclk_sel_sendrv == CTL_SEN_CLK_SEL_SIEMCLK_IGNORE) {
		// ext mclk
		return CTL_SEN_E_OK;
	} else if (mclk_sel_sendrv >= CTL_SEN_CLK_SEL_MAX) {
		CTL_SEN_DBG_ERR("id %d mclk_sel %d overflow\r\n", id, mclk_sel_sendrv);
		return CTL_SEN_E_SYS;
	}
	if (mclk_freq == 0) {
		CTL_SEN_DBG_ERR("id %d mclk_freq zero\r\n", id);
		return CTL_SEN_E_SENDRV;
	}


	/*
	    get mclk sel rank table
	*/
	if (nvt_get_chip_id() == CHIP_NA51055) {
		mclk_src_sel_rank = &sen_mclk_srcsel_rank_520[mclk_sel_sendrv][0];
	} else {
		// wait linux define CHIP_NA51084
		if (mclk_src_sync_sel[CTL_SEN_CLK_SEL_SIEMCLK3]) {
			mclk_src_sel_rank = &sen_mclk_srcsel_rank_528_mclk3sync[mclk_sel_sendrv][0];
		} else {
			mclk_src_sel_rank = &sen_mclk_srcsel_rank_528[mclk_sel_sendrv][0];
		}
	}

	if (mclk_src_sel_rank == NULL) {
		CTL_SEN_DBG_ERR("mclk_src_sel_rank tbl NULL\r\n");
		return CTL_SEN_E_SYS;
	}

	/*
	    cfg mclk src and corresponding pllx
	*/
	for (i = 0; i < SEN_RANK_TBL_LEN; i++) {
		mclk_src_sel_tmp = mclk_src_sel_rank[i];
		if (mclk_src_sel_tmp == CTL_SEN_CLKSRC_MAX) {
			// rank array end
			CTL_SEN_DBG_ERR("id %d cannot find pll for %s\r\n", id, sen_get_clksel_str(mclk_sel_sendrv));
			rt = CTL_SEN_E_CLK;
			break;
		} else if (mclk_src_sel_tmp == CTL_SEN_CLKSRC_480) {
			// chk if "mclk frequency" is divisible by "480MHz"
			if ((CTL_SEN_480M_HZ % mclk_freq) == 0) {
				// update mclk info
				update_mclk = 1;
				break;

			}
		} else { // pllx
			// chk pllx enable
			if (sen_pll_get_en(mclk_src_sel_tmp, &pll_en) != 0) {
				CTL_SEN_DBG_ERR("%s sen_pll_get_en\r\n", sen_get_clksrc_str(mclk_src_sel_tmp));
				rt = CTL_SEN_E_IF_GLB;
				break;
			}
			if (pll_en) {
				// get "pllx current frequency"
				if (sen_pll_get_freq(mclk_src_sel_tmp, &pll_freq) != 0) {
					CTL_SEN_DBG_ERR("%s sen_pll_get_freq\r\n", sen_get_clksrc_str(mclk_src_sel_tmp));
					rt = CTL_SEN_E_IF_GLB;
					break;
				}
				if (pll_freq == 0) {
					CTL_SEN_DBG_WRN("%s enable but freq is zero\r\n", sen_get_clksrc_str(mclk_src_sel_tmp));
				} else if ((pll_freq % mclk_freq) == 0) {
					// "mclk frequency" is divisible by "pllx current frequency"
					update_mclk = 1;
					break;
				}
			} else {
				// set pllx frequency
				kdrv_rt = sen_pll_set_freq(mclk_src_sel_tmp, mclk_freq);
				if (kdrv_rt) {
					CTL_SEN_DBG_ERR("%s set freq fail %d \r\n", sen_get_clksrc_str(mclk_src_sel_tmp), kdrv_rt);
					rt = CTL_SEN_E_IF_GLB;
					break;
				}
				update_mclk = 1;
				break;
			}
		}
	}

mclksrc_sel_end:

	if (update_mclk) {
		kdrv_rt = sen_mclk_set_src(mclk_sel_sendrv, mclk_src_sel_tmp);
		if (kdrv_rt) {
			CTL_SEN_DBG_ERR("id %d sen_mclk_set_src fail\r\n", id);
			rt = CTL_SEN_E_IF_GLB;
		}
		kdrv_rt = sen_mclk_set_freq(mclk_sel_sendrv, mclk_freq);
		if (kdrv_rt) {
			CTL_SEN_DBG_ERR("id %d sen_mclk_set_freq fail\r\n", id);
			rt = CTL_SEN_E_IF_GLB;
		}
		kdrv_rt = sen_pll_set_en(mclk_src_sel_tmp, ENABLE);
		if (kdrv_rt) {
			CTL_SEN_DBG_ERR("id %d sen_pll_set_en fail\r\n", id);
			rt = CTL_SEN_E_IF_GLB;
		}

		mclksrc_sync = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync;
		for (i = 0; i < CTL_SEN_ID_MAX; i++) {
			if (!mclksrc_sync) {
				break;
			}
			if ((mclksrc_sync) & (1 << i)) {
				sen_set_mclksrc_sync_info(i, mclk_src_sel_tmp, 1);
				/* KDRV_SSENIF_SIEMCLK_12SYNC_FREQUENCY need to set before mclk1/2_en and mclk1/2_src */
				sen_mclk_set_src(sen_uti_get_mclksel(i, mode), mclk_src_sel_tmp);
			}

			mclksrc_sync &= ~(1 << i);
		}
	}

	return rt;
}

static INT32 sen_ctrl_if_clk_unprepare(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	CTL_SEN_CLK_SEL mclk_sel_sendrv;
	UINT32 mclk_freq, i, mclksrc_sync;
	INT32 kdrv_rt, rt = CTL_SEN_E_OK;

	/*
	    get clk info from sendrv
	*/
	mclk_sel_sendrv = sen_uti_get_mclksel(id, mode);
	mclk_freq = sen_uti_get_mclk_freq(id, mode);

	if (mclk_sel_sendrv == CTL_SEN_CLK_SEL_SIEMCLK_IGNORE) {
		// ext mclk
		return CTL_SEN_E_OK;
	} else if (mclk_sel_sendrv >= CTL_SEN_CLK_SEL_MAX) {
		CTL_SEN_DBG_ERR("id %d %s overflow\r\n", id, sen_get_clksel_str(mclk_sel_sendrv));
		return CTL_SEN_E_SYS;
	}
	if (mclk_freq == 0) {
		CTL_SEN_DBG_ERR("id %d mclk_freq zero\r\n", id);
		return CTL_SEN_E_SENDRV;
	}

	/*
	    if all mclksrc_sync id closed, reset mclksrc_sync info
	*/
	mclksrc_sync = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync;
	mclksrc_sync &= ~(1 << id);
	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		if (!mclksrc_sync) {
			break;
		}
		if (!is_glb_opened[i]) {
			mclksrc_sync &= ~(1 << i);
		}
	}

	// all of mclksrc_sync id is closed
	if (mclksrc_sync == 0) {
		mclksrc_sync = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync;
		for (i = 0; i < CTL_SEN_ID_MAX; i++) {
			if ((mclksrc_sync) & (1 << i)) {
				sen_set_mclksrc_sync_info(i, CTL_SEN_CLKSRC_480, 0); // reset mclksrc_sync info
			}
		}


	}

	// disable pllx
	kdrv_rt = sen_pll_set_en(sen_uti_get_mclk_src(mclk_sel_sendrv), DISABLE);
	if (kdrv_rt) {
		CTL_SEN_DBG_ERR("%s set dis fail\r\n", sen_get_clksel_str(mclk_sel_sendrv));
		rt = CTL_SEN_E_IF_GLB;
	}

	return rt;
}

static UINT32 mclksync_init_cnt = 0;
static INT32 sen_ctrl_if_mclksync_init(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	INT rt = 0;
	UINT32 mclksrc_sync = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync;
	UINT32 i, mclksel_cnt[CTL_SEN_CLK_SEL_MAX] = {0};
	CTL_SEN_CLK_SEL mclk_sel;

	if (mclksync_init_cnt++) {
		return CTL_SEN_E_OK;
	}

	/*
	    chk sendrv mclksrc, mclk sync id cannot be same source (KDRV_SSENIF_SIEMCLK_12SYNC_FREQUENCY will error)
	    EX : id0&id1 sync, NG(id0 mclk1, id1 mclk1), OK(id0 mclk1, id1 mclk2)
	*/
	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		if ((mclksrc_sync) & (1 << i)) {
			mclk_sel = sen_uti_get_mclksel(i, mode);
			if (mclk_sel < CTL_SEN_CLK_SEL_MAX) {
				mclksel_cnt[mclk_sel]++;
				/* disable mclk pinmux */
				sen_mclk_upd_pinmux(i, 0);
			}
		}
	}
	// (drv not support MCLK3 sync flow)
	if ((mclksel_cnt[CTL_SEN_CLK_SEL_SIEMCLK] == 0) || (mclksel_cnt[CTL_SEN_CLK_SEL_SIEMCLK2] == 0)) {
		CTL_SEN_DBG_WRN("id %d mclksrc_sync 0x%.8x sendrv cnt(SIEMCLK %d SIEMCLK2 %d)\r\n", id, (unsigned int)mclksrc_sync, mclksel_cnt[CTL_SEN_CLK_SEL_SIEMCLK], mclksel_cnt[CTL_SEN_CLK_SEL_SIEMCLK2]);
	}

	/********************************/
	/******* sync flow for DD *******/
	/********************************/

	/* enable mclk (drv not support MCLK3 sync flow) */
	sen_mclk_en(CTL_SEN_CLK_SEL_SIEMCLK, ENABLE);
	sen_mclk_en(CTL_SEN_CLK_SEL_SIEMCLK2, ENABLE);

	/* set mclk div to 0 (drv cover flow) */
	/* delay 1 us (drv cover flow) */
	/* set mclk div to sendrv_mclk_freq (KDRV_SSENIF_SIEMCLK_12SYNC_FREQUENCY need to set before mclk1/2_en and mclk1/2_src) */
	rt |= sen_mclk_syncfreq(sen_uti_get_mclk_freq(id, mode));

	return rt;
}

static INT32 sen_ctrl_if_mclksync_uninit(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	INT rt = 0;

	if (--mclksync_init_cnt) {
		return CTL_SEN_E_OK;
	}

	/* disable mclk (drv not support MCLK3 sync flow) */
	sen_mclk_en(CTL_SEN_CLK_SEL_SIEMCLK, DISABLE);
	sen_mclk_en(CTL_SEN_CLK_SEL_SIEMCLK2, DISABLE);

	return rt;
}

static CTL_SEN_CTRL_IF_GLB sen_ctrl_if_glb_tab = {

	sen_ctrl_if_glb_open,
	sen_ctrl_if_glb_close,

	sen_ctrl_if_glb_set_cfg,
	sen_ctrl_if_glb_get_cfg,

	sen_ctrl_if_clk_prepare,
	sen_ctrl_if_clk_unprepare,

	sen_ctrl_if_mclksync_init,
	sen_ctrl_if_mclksync_uninit,
};

CTL_SEN_CTRL_IF_GLB *sen_ctrl_if_glb_get_obj(void)
{
	return &sen_ctrl_if_glb_tab;
}

