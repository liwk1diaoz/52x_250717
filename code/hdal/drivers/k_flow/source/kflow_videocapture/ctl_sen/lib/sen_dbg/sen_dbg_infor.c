/*
    Sensor Interface library - debug information

    Sensor Interface library.

    @file       sen_dbg_infor.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "sen_dbg_infor_int.h"

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

/* for debug msg */
CTL_SEN_DBG_LV ctl_sen_dbg_level = CTL_SEN_DBG_LV_WRN;
CTL_SEN_MSG_TYPE ctl_sen_msg_type[CTL_SEN_ID_MAX_SUPPORT] = {0};
BOOL ctl_sen_er_log = FALSE;
static BOOL set_er_lock[CTL_SEN_ID_MAX_SUPPORT] = {0};

static CTL_SEN_PROC_TIME_ADV sen_proc_time_adv[CTL_SEN_PROC_TIME_CNT_MAX] = {0};
static CTL_SEN_PROC_TIME_ADV dump_proc_time_adv[CTL_SEN_PROC_TIME_CNT_MAX];
static UINT32 sen_proc_time_cnt_adv = 0;
static UINT32 sen_op_io_cnt[CTL_SEN_ID_MAX][CTL_SEN_ER_OP_MAX] = {0};
static UINT32 sen_proc_time_io_cnt[CTL_SEN_ID_MAX] = {0};

CTL_SEN_PROC_TIME_ADV *ctl_sen_get_proc_time_adv(void);
UINT32 ctl_sen_get_proc_time_cnt_adv(void);

static CHAR *sen_get_vendor_str(CTL_SEN_VENDOR in)
{
	UINT32 str_num = CTL_SEN_VENDOR_OTHERS + 1;
	CHAR *str_out_fmt[] = {"SONY", "OMNI", "ONSM", "PANA", "OTHR", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_signaltype_str(CTL_SEN_SIGNAL_TYPE in)
{
	UINT32 str_num = CTL_SEN_SIGNAL_MAX_NUM;
	CHAR *str_out_fmt[] = {"MAST", "SLV", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_activesel_str(CTL_SEN_ACTIVE_SEL in)
{
	UINT32 str_num = 2;
	CHAR *str_out_fmt[] = {"H", "L", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_phasesel_str(CTL_SEN_PHASE_SEL in)
{
	UINT32 str_num = 2;
	CHAR *str_out_fmt[] = {"R", "F", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

CHAR *sen_get_cmdiftype_str(CTL_SEN_CMDIF_TYPE in)
{
	UINT32 str_num = CTL_SEN_CMDIF_TYPE_MAX_NUM;
	CHAR *str_out_fmt[] = {"UNKWN", "Vx1", "SIF", "I2C", "IO", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_i2cch_str(CTL_SEN_I2C_CH in)
{
	UINT32 str_num = 5;
	CHAR *str_out_fmt[] = {"I2C1", "I2C2", "I2C3", "I2C4", "I2C5", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_i2cwsel_str(UINT32 in)
{
	UINT32 str_num = CTL_SEN_I2C_W_ADDR_SEL_MAX_NUM;
	CHAR *str_out_fmt[] = {"DFT", "OP1", "OP2", "OP3", "OP4", "OP5", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_iftype_str(CTL_SEN_IF_TYPE in)
{
	UINT32 str_num = CTL_SEN_IF_TYPE_MAX_NUM;
	CHAR *str_out_fmt[] = {"PARALLEL", "LVDS", "MIPI", "SLVSEC", "DUMMY", "PATGEN", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_clanesel_str(CTL_SEN_CLANE_SEL in)
{
	UINT32 str_num = 22;
	CHAR *str_out_fmt[] = {
		"CSI0_USE_C0",
		"CSI0_USE_C2",
		"CSI1_USE_C4",
		"CSI1_USE_C6",
		"CSI2_USE_C2",
		"CSI3_USE_C6",
		"CSI4_USE_C1",
		"CSI5_USE_C3",
		"CSI6_USE_C5",
		"CSI7_USE_C7",
		"LVDS0_USE_C0C4",
		"LVDS0_USE_C2C6",
		"LVDS1_USE_C4",
		"LVDS1_USE_C6",
		"LVDS2_USE_C2",
		"LVDS3_USE_C6",
		"LVDS4_USE_C1",
		"LVDS5_USE_C3",
		"LVDS6_USE_C5",
		"LVDS7_USE_C7",
		"CSI1_USE_C1",
		"LVDS1_USE_C1",
		"ERR",
	};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_serialpin_str(UINT32 in)
{
	UINT32 str_num = CTL_SEN_SER_MAX_DATALANE + 1;
	CHAR *str_out_fmt[] = {"0", "1", "2", "3", "4", "5", "6", "7", "IGN", "ERR"};

	if (in == CTL_SEN_IGNORE) {
		return str_out_fmt[CTL_SEN_SER_MAX_DATALANE];
	}
	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_modesel_str(CTL_SEN_MODESEL in)
{
	UINT32 str_num = 2;
	CHAR *str_out_fmt[] = {"AUTO", "MANUAL", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_dvifmt_str(CTL_SEN_DVI_FMT in)
{
	UINT32 str_num = 7;
	CHAR *str_out_fmt[] = {"CCIR601", "CCIR656_EAV", "CCIR656_ACT", "CCIR709", "CCIR601_1120", "CCIR656_1120_EAV", "CCIR656_1120_ACT", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}


static CHAR *sen_get_ccirfmt_str(CTL_SEN_CCIR_FMT_SEL in)
{
	UINT32 str_num = 4;
	CHAR *str_out_fmt[] = {"CCIR601", "CCIR656", "CCIR709", "CCIR1120", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_dvidatamode_str(CTL_SEN_DVI_DATA_MODE in)
{
	UINT32 str_num = 2;
	CHAR *str_out_fmt[] = {"SD", "HD", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_siemclksrc_str(CTL_SEN_SIEMCLK_SRC in)
{
	UINT32 str_num = CTL_SEN_SIEMCLK_SRC_MAX;
	CHAR *str_out_fmt[] = {"DFT", "MCLK", "MCLK2", "MCLK3", "ERR"};
	CHAR *str_out_fmt_ext[] = {"IGN"};

	if (in == CTL_SEN_SIEMCLK_SRC_IGNORE) {
		return str_out_fmt_ext[0];
	}

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

CHAR *sen_get_clksel_str(CTL_SEN_CLK_SEL in)
{
	UINT32 str_num = CTL_SEN_CLK_SEL_MAX;
	CHAR *str_out_fmt[] = {"MCLK", "MCLK2", "MCLK3", "ERR"};
	CHAR *str_out_fmt_ext[] = {"IGN"};

	if (in == CTL_SEN_CLK_SEL_SIEMCLK_IGNORE) {
		return str_out_fmt_ext[0];
	}

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

CHAR *sen_get_opstate_str(CTL_SEN_STATE in)
{
	UINT32 i, str_num = 13;
	CHAR *str_out_fmt[] = {
		"NONE",
		"INITCFG",
		"REGSEN",
		"OPEN",
		"PWRON",
		"PWRSAVE",
		"SLP",
		"WREG",
		"RREG",
		"CHGMODE",
		"SET",
		"GET",
		"WAITINT",
		"ERR",
	};
	UINT32 num[] = {
		CTL_SEN_STATE_NONE,
		CTL_SEN_STATE_INTCFG,
		CTL_SEN_STATE_REGSEN,
		CTL_SEN_STATE_OPEN,
		CTL_SEN_STATE_PWRON,
		CTL_SEN_STATE_PWRSAVE,
		CTL_SEN_STATE_SLEEP,
		CTL_SEN_STATE_WRITEREG,
		CTL_SEN_STATE_READREG,
		CTL_SEN_STATE_CHGMODE,
		CTL_SEN_STATE_SETCFG,
		CTL_SEN_STATE_GETCFG,
		CTL_SEN_STATE_WAITINTE,
	};

	for (i = 0; i < str_num; i++) {
		if (in == num[i]) {
			return str_out_fmt[i];
		}
	}

	CTL_SEN_DBG_ERR("0x%.8x error\r\n", in);

	return str_out_fmt[str_num];
}

CHAR *sen_get_inte_str(CTL_SEN_INTE in)
{
	UINT32 i, str_num = 23;
	CHAR *str_out_fmt[] = {
		"NONE",
		"VD",
		"VD2",
		"VD3",
		"VD4",
		"FMD",
		"FMD2",
		"FMD3",
		"FMD4",
		"ABORT",
		"VD_TGE",
		"VD_VX1",
		"VD2_VX1",
		"VD3_VX1",
		"VD4_VX1",
		"VD_TO_SIE0",
		"VD_TO_SIE1",
		"VD_TO_SIE3",
		"VD_TO_SIE4",
		"FMD_TO_SIE0",
		"FMD_TO_SIE1",
		"FMD_TO_SIE3",
		"FMD_TO_SIE4",
		"ERR",
	};
	UINT32 num[] = {
		CTL_SEN_INTE_NONE,
		CTL_SEN_INTE_VD,
		CTL_SEN_INTE_VD2,
		CTL_SEN_INTE_VD3,
		CTL_SEN_INTE_VD4,
		CTL_SEN_INTE_FRAMEEND,
		CTL_SEN_INTE_FRAMEEND2,
		CTL_SEN_INTE_FRAMEEND3,
		CTL_SEN_INTE_FRAMEEND4,
		CTL_SEN_INTE_ABORT,
		CTL_SEN_INTE_TGE_VD,
		CTL_SEN_INTE_VX1_VD,
		CTL_SEN_INTE_VX1_VD2,
		CTL_SEN_INTE_VX1_VD3,
		CTL_SEN_INTE_VX1_VD4,
		CTL_SEN_INTE_VD_TO_SIE0,
		CTL_SEN_INTE_VD_TO_SIE1,
		CTL_SEN_INTE_VD_TO_SIE3,
		CTL_SEN_INTE_VD_TO_SIE4,
		CTL_SEN_INTE_FMD_TO_SIE0,
		CTL_SEN_INTE_FMD_TO_SIE1,
		CTL_SEN_INTE_FMD_TO_SIE3,
		CTL_SEN_INTE_FMD_TO_SIE4,
	};

	for (i = 0; i < str_num; i++) {
		if (in == num[i]) {
			return str_out_fmt[i];
		}
	}

	CTL_SEN_DBG_ERR("0x%.8x error\r\n", in);

	return str_out_fmt[str_num];
}

static CHAR *sen_get_proc_time_item_str(CTL_SEN_PROC_TIME_ITEM in)
{
	UINT32 str_num = CTL_SEN_PROC_TIME_ITEM_MAX;
	CHAR *str_out_fmt[] = {"ENTER", "EXIT", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}


CHAR *sen_get_clksrc_str(CTL_SEN_CLKSRC_SEL in)
{
	UINT32 str_num = CTL_SEN_CLKSRC_MAX;
	CHAR *str_out_fmt[] = {
		"fixed_480M",
		"PLL2",
		"PLL4",
		"PLL5",
		"PLL10",
		"PLL12",
		"PLL14",
		"PLL18",
		"ERR",
	};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_vx1_txtype_str(KDRV_SSENIFVX1_TXTYPE in)
{
	UINT32 str_num = 3;
	CHAR *str_out_fmt[] = {"tx235", "tx231", "tx241", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_tge_sie_vd_src_str(CTL_SEN_TGE_SIE_VD_SRC in)
{
	UINT32 str_num = 4;
	CHAR *str_out_fmt[] = {"SIE1_VD_SRC_CH1", "SIE1_VD_SRC_CH3", "SIE3_VD_SRC_CH5", "SIE3_VD_SRC_CH7", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_senmode_str(CTL_SEN_MODE in)
{
	UINT32 str_num = 14;
	CHAR *str_out_fmt[] = {
		"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"
	};

	CHAR *str_out_fmt_2[] = {
		"UNKWN", "CUR", "ERR"
	};
	if (in <= str_num) {
		return str_out_fmt[in];
	} else if (in == CTL_SEN_MODE_UNKNOWN) {
		return str_out_fmt_2[0];
	} else if (in == CTL_SEN_MODE_CUR) {
		return str_out_fmt_2[1];
	}
	return str_out_fmt_2[2];
}

static CHAR *sen_get_datafmt_str(CTL_SEN_DATA_FMT in)
{
	UINT32 str_num = 5;
	CHAR *str_out_fmt[] = {
		"RGB",
		"RGBIR",
		"RCCB",
		"YUV",
		"Y_ONLY",
		"ERR",
	};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR modetype_str_out0[30], modetype_str_out1[30], modetype_str_out2[30], modetype_str_out3[30];
static CHAR *sen_get_modetype_str(CTL_SEN_MODE_TYPE in, UINT32 frm_idx)
{
	CHAR *str_out_fmt_outtype[] = {
		"unkn",
		"raw",
		"ccir_p",
		"ccir_i",
		"pdaf",
	};
	CHAR *str_out_fmt_exptgain[] = {
		"unkn",
		"expt",
		"hcg(dcg)",
		"lcg",
	};
	CHAR *str_out_fmt_outcombine[] = {
		"unkn",
		"linear",
		"pwl",
	};

	if (frm_idx == 0) {
		sprintf(modetype_str_out0, "frm%d(%s,%s,%s)"
				, (int)frm_idx
				, str_out_fmt_outtype[CTL_SEN_ENTITY_MODETYPE_OUTTYPE(in, frm_idx)]
				, str_out_fmt_exptgain[CTL_SEN_ENTITY_MODETYPE_EXPTGAIN(in, frm_idx)]
				, str_out_fmt_outcombine[CTL_SEN_ENTITY_MODETYPE_OUTCOMB(in, frm_idx)]);
		return modetype_str_out0;
	}
	if (frm_idx == 1) {
		sprintf(modetype_str_out1, "frm%d(%s,%s,%s)"
				, (int)frm_idx
				, str_out_fmt_outtype[CTL_SEN_ENTITY_MODETYPE_OUTTYPE(in, frm_idx)]
				, str_out_fmt_exptgain[CTL_SEN_ENTITY_MODETYPE_EXPTGAIN(in, frm_idx)]
				, str_out_fmt_outcombine[CTL_SEN_ENTITY_MODETYPE_OUTCOMB(in, frm_idx)]);
		return modetype_str_out1;
	}
	if (frm_idx == 2) {
		sprintf(modetype_str_out2, "frm%d(%s,%s,%s)"
				, (int)frm_idx
				, str_out_fmt_outtype[CTL_SEN_ENTITY_MODETYPE_OUTTYPE(in, frm_idx)]
				, str_out_fmt_exptgain[CTL_SEN_ENTITY_MODETYPE_EXPTGAIN(in, frm_idx)]
				, str_out_fmt_outcombine[CTL_SEN_ENTITY_MODETYPE_OUTCOMB(in, frm_idx)]);
		return modetype_str_out2;
	}
	if (frm_idx == 3) {
		sprintf(modetype_str_out3, "frm%d(%s,%s,%s)"
				, (int)frm_idx
				, str_out_fmt_outtype[CTL_SEN_ENTITY_MODETYPE_OUTTYPE(in, frm_idx)]
				, str_out_fmt_exptgain[CTL_SEN_ENTITY_MODETYPE_EXPTGAIN(in, frm_idx)]
				, str_out_fmt_outcombine[CTL_SEN_ENTITY_MODETYPE_OUTCOMB(in, frm_idx)]);
		return modetype_str_out3;
	}

	return str_out_fmt_outtype[0];
}

CHAR *sen_get_stpix_str(CTL_SEN_STPIX in)
{
	UINT32 i, str_num = 22;
	CHAR *str_out_fmt[] = {
		"R",
		"GR",
		"GB",
		"B",

		"RGBIR_RGBG_GIGI",
		"RGBIR_GBGR_IGIG",
		"RGBIR_GIGI_BGRG",
		"RGBIR_IGIG_GRGB",
		"RGBIR_BGRG_GIGI",
		"RGBIR_GRGB_IGIG",
		"RGBIR_GIGI_RGBG",
		"RGBIR_IGIG_GBGR",

		"RCCB_RC",
		"RCCB_CR",
		"RCCB_CB",
		"RCCB_BC",

		"Y_ONLY",
		"YUV_YUYV",
		"YUV_YVYU",
		"YUV_UYVY",
		"YUV_VYUY",
		"NONE",

		"ERR",
	};
	UINT32 num[] = {
		CTL_SEN_STPIX_R,
		CTL_SEN_STPIX_GR,
		CTL_SEN_STPIX_GB,
		CTL_SEN_STPIX_B,

		CTL_SEN_STPIX_RGBIR_RGBG_GIGI,
		CTL_SEN_STPIX_RGBIR_GBGR_IGIG,
		CTL_SEN_STPIX_RGBIR_GIGI_BGRG,
		CTL_SEN_STPIX_RGBIR_IGIG_GRGB,
		CTL_SEN_STPIX_RGBIR_BGRG_GIGI,
		CTL_SEN_STPIX_RGBIR_GRGB_IGIG,
		CTL_SEN_STPIX_RGBIR_GIGI_RGBG,
		CTL_SEN_STPIX_RGBIR_IGIG_GBGR,

		CTL_SEN_STPIX_RCCB_RC,
		CTL_SEN_STPIX_RCCB_CR,
		CTL_SEN_STPIX_RCCB_CB,
		CTL_SEN_STPIX_RCCB_BC,

		CTL_SEN_STPIX_Y_ONLY,
		CTL_SEN_STPIX_YUV_YUYV,
		CTL_SEN_STPIX_YUV_YVYU,
		CTL_SEN_STPIX_YUV_UYVY,
		CTL_SEN_STPIX_YUV_VYUY,
		CTL_SEN_STPIX_NONE,
	};

	for (i = 0; i < str_num; i++) {
		if (in == num[i]) {
			return str_out_fmt[i];
		}
	}

	CTL_SEN_DBG_ERR("0x%.8x error\r\n", in);

	return str_out_fmt[str_num];
}

static CHAR *sen_get_fmt_str(CTL_SEN_FMT in)
{
	UINT32 str_num = 1;
	CHAR *str_out_fmt[] = {"POGRESSIVE", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_datainbitorder_str(CTL_SEN_DATAIN_BIT_ORDER in)
{
	UINT32 str_num = 2;
	CHAR *str_out_fmt[] = {"LSB", "MSB", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_er_op_str(CTL_SEN_ER_OP in)
{
	UINT32 str_num = CTL_SEN_ER_OP_MAX;
	CHAR *str_out_fmt[] = {
		"INIT", "REG", "UNREG", "OPEN", "OPEN_CLK",
		"OPEN_PIN", "CLOSE", "CLOSE_PIN", "PWR", "SLP",
		"WUP", "WR", "RR", "CHG", "CHGFPS",
		"SET", "GET", "BUF_Q", "G_INIT", "G_UNINIT",
		"PRE_CLK", "UNP_CLK", "ERR"
	};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_er_item_str(CTL_SEN_ER_ITEM in)
{
	UINT32 str_num = CTL_SEN_ER_ITEM_MAX;
	CHAR *str_out_fmt[] = {"OUTPUT", "SYS", "CMDIF", "IF", "SENDRV", "ERR"};

	if (in >= str_num) {
		return str_out_fmt[str_num];
	}
	return str_out_fmt[in];
}

static CHAR *sen_get_er_str(INT32 in)
{
	UINT32 i, str_num = 22;
	CHAR *str_out_fmt[] = {
		"OK",

		"STATE",
		"MAP_TBL",
		"ID_OVFL",
		"IN_PARAM",
		"PINMUX",
		"CLK",
		"SYS",
		"NS",
		"IF",
		"CMDIF",
		"SENDRV",

		"SENDRV_GET",
		"SENDRV_SET",
		"SENDRV_OP",
		"SENDRV_TBL_NULL",
		"SENDRV_PARAM",
		"SENDRV_NS",

		"KERNEL",
		"KDRV_SSENIF",
		"KDRV_SIE",
		"KDRV_TGE",

		"ERR",
	};
	INT32 num[] = {
		0,
		(-1), (-2), (-3), (-4), (-5), (-6), (-7), (-8), (-9), (-10), (-11),
		(-100), (-200), (-300), (-400), (-500), (-600),
		(-10000), (-20000), (-30000), (-40000),
	};

	for (i = 0; i < str_num; i++) {
		if (in == num[i]) {
			return str_out_fmt[i];
		}
	}

	CTL_SEN_DBG_ERR("0x%.8x error\r\n", in);

	return str_out_fmt[str_num];
}

static void sen_dbg_set_er_lock(CTL_SEN_ID id)
{
	set_er_lock[id] = TRUE;
}

static void sen_dbg_set_er_unlock(CTL_SEN_ID id)
{
	set_er_lock[id] = FALSE;
}

void ctl_sen_set_er(CTL_SEN_ID id, CTL_SEN_ER_OP op, CTL_SEN_ER_ITEM item, INT32 rt)
{
	if ((ctl_sen_er_log) && (rt != CTL_SEN_E_OK)) {
		DBG_DUMP("[SEN_KFLOW]id%d,%s,%s,%s\r\n", id, sen_get_er_op_str(op), sen_get_er_item_str(item), sen_get_er_str(rt));
	}
}

void ctl_sen_set_er_sendrv(CTL_SEN_ID id, CTL_SEN_ER_OP op, UINT32 item)
{
	if (ctl_sen_er_log) {
		DBG_DUMP("[SENDRV]id%d,%s,0x%.8x fail\r\n", id, sen_get_er_op_str(op), (unsigned int)item);
	}
}

static void sen_dbg_dump_info(CTL_SEN_ID id)
{
	CTL_SEN_GET_ATTR_PARAM attr_param;
	CTL_SEN_GET_CMDIF_PARAM cmdif_param;
	CTL_SEN_GET_IF_PARAM if_param;
	CTL_SEN_GET_DVI_PARAM dvi_param;
	UINT32 temp, i, value[10] = {0};
	CTL_SEN_GET_FPS_PARAM fps_param;
	CTL_SEN_GET_CLK_INFO_PARAM clk_info_param;
	CTL_SEN_GET_MODESEL_PARAM modesel_param;
	CTL_SEN_GET_MODE_BASIC_PARAM mode_basic_param;
	CTL_SEN_GET_MFR_OUTPUT_TIMING_PARAM mfr_output_timing_param;
	CTL_SEN_CTL_VX1_GPIO_PARAM vx1_gpio_param[CTL_SEN_VX1_GPIO_NUM];
	BOOL plug, is_dump;
	CTL_SEN_FLIP flip;
	INT32 rt;

	sen_dbg_set_er_lock(id);

	DBG_DUMP("\r\n==================sensor info==================\r\n");
	memset((void *)(&attr_param), 0, sizeof(CTL_SEN_GET_ATTR_PARAM));
	memset((void *)(&cmdif_param), 0, sizeof(CTL_SEN_GET_CMDIF_PARAM));
	memset((void *)(&if_param), 0, sizeof(CTL_SEN_GET_IF_PARAM));
	memset((void *)(&dvi_param), 0, sizeof(CTL_SEN_GET_DVI_PARAM));
	memset((void *)(&temp), 0, sizeof(UINT32));
	memset((void *)(&fps_param), 0, sizeof(CTL_SEN_GET_FPS_PARAM));
	memset((void *)(&clk_info_param), 0, sizeof(CTL_SEN_GET_CLK_INFO_PARAM));
	memset((void *)(&modesel_param), 0, sizeof(CTL_SEN_GET_MODESEL_PARAM));
	memset((void *)(&mode_basic_param), 0, sizeof(CTL_SEN_GET_MODE_BASIC_PARAM));
	memset((void *)(&mfr_output_timing_param), 0, sizeof(CTL_SEN_GET_MFR_OUTPUT_TIMING_PARAM));
	memset((void *)(&vx1_gpio_param[0]), 0, sizeof(CTL_SEN_CTL_VX1_GPIO_PARAM) * CTL_SEN_VX1_GPIO_NUM);
	memset((void *)(&plug), 0, sizeof(BOOL));
	memset((void *)(&flip), 0, sizeof(CTL_SEN_FLIP));

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_ATTR, (void *)&attr_param);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("-----ATTR-----\r\n");
		DBG_DUMP("%20s %10s %8s %10s %10s %11s %11s %8s %8s %6s %6s %6s %6s %6s\r\n"
				 , "name", "vendor", "maxmode", "cmdif_type", "if_type", "drvdev"
				 , "property", "sync", "sgl_type", "vd_act", "hd_act", "vd_pha"
				 , "hd_pha", "dt_pha");
		DBG_DUMP("%20s %10s %8d %10s %10s 0x%.8x 0x%.8x %8d %8s %6s %6s %6s %6s %6s\r\n"
				 , attr_param.name
				 , sen_get_vendor_str(attr_param.vendor)
				 , (int)attr_param.max_senmode
				 , sen_get_cmdiftype_str(attr_param.cmdif_type)
				 , sen_get_iftype_str(attr_param.if_type)
				 , (unsigned int)attr_param.drvdev
				 , (int)attr_param.property
				 , (int)attr_param.sync_timing
				 , sen_get_signaltype_str(attr_param.signal_type)
				 , sen_get_activesel_str(attr_param.signal_info.vd_inv)
				 , sen_get_activesel_str(attr_param.signal_info.hd_inv)
				 , sen_get_phasesel_str(attr_param.signal_info.vd_phase)
				 , sen_get_phasesel_str(attr_param.signal_info.hd_phase)
				 , sen_get_phasesel_str(attr_param.signal_info.data_phase));
	}

	DBG_DUMP("-----CMDIF-----\r\n");
	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_CMDIF, (void *)&cmdif_param);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("%8s\r\n", "type");
		DBG_DUMP("%8s\r\n", sen_get_cmdiftype_str(cmdif_param.type));
		if (cmdif_param.type == CTL_SEN_CMDIF_TYPE_VX1) {
			DBG_DUMP("-----CMDIF VX1-----\r\n");
			DBG_DUMP("%-12s%-12s\r\n", "if_sel", "tx");
			DBG_DUMP("%-12d%-12s\r\n", (int)cmdif_param.vx1.if_sel, sen_get_vx1_txtype_str(cmdif_param.vx1.tx_type));
			rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_VX1_SLAVE_ADDR, (void *)&value[0]);
			rt |= sen_ctrl_get_cfg(id, CTL_SEN_CFGID_VX1_ADDR_BYTE_CNT, (void *)&value[1]);
			rt |= sen_ctrl_get_cfg(id, CTL_SEN_CFGID_VX1_DATA_BYTE_CNT, (void *)&value[2]);
			rt |= sen_ctrl_get_cfg(id, CTL_SEN_CFGID_VX1_I2C_SPEED, (void *)&value[3]);
			rt |= sen_ctrl_get_cfg(id, CTL_SEN_CFGID_VX1_I2C_NACK_CHK, (void *)&value[4]);
			rt |= sen_ctrl_get_cfg(id, CTL_SEN_CFGID_VX1_PLUG, (void *)&value[5]);
			for (i = 0; i < CTL_SEN_VX1_GPIO_NUM; i++) {
				vx1_gpio_param[i].pin = i;
				rt |= sen_ctrl_get_cfg(id, CTL_SEN_CFGID_VX1_GPIO, (void *)&vx1_gpio_param[i]);
			}
			if (rt == CTL_SEN_E_OK) {
				DBG_DUMP("%-12s%-10s%-10s%-10s%-10s%-10s%-6s%-6s%-6s%-6s%-6s\r\n", "slv_addr", "adr_byte", "dt_byte", "i2c_sp", "i2c_nk_ck", "plug"
						 , "gpio0", "gpio1", "gpio2", "gpio3", "gpio4");
				DBG_DUMP("0x%.8x  %-10d%-10d%-10d%-10d%-10d%-6d%-6d%-6d%-6d%-6d\r\n", (int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], (int)value[5]
						 , (int)vx1_gpio_param[0].value, (int)vx1_gpio_param[1].value, (int)vx1_gpio_param[2].value, (int)vx1_gpio_param[3].value, (int)vx1_gpio_param[4].value);
			}
		}

	}

	DBG_DUMP("-----IF-----\r\n");
	for (i = CTL_SEN_MODE_1; i < attr_param.max_senmode; i++) {
		if_param.mode = i;
		rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_IF, (void *)&if_param);
		if (rt == CTL_SEN_E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%5s%8s\r\n", "mode", "type");
			DBG_DUMP("%5s%8s\r\n"
					 , sen_get_senmode_str(if_param.mode)
					 , sen_get_iftype_str(if_param.type));
			if (if_param.type == CTL_SEN_IF_TYPE_PARALLEL) {
				DBG_DUMP("%10s\r\n", "mx_dt_num");
				DBG_DUMP("%10d\r\n", (int)if_param.info.parallel.mux_info.mux_data_num);
			}
			if (if_param.type == CTL_SEN_IF_TYPE_MIPI) {
			}
			if (if_param.type == CTL_SEN_IF_TYPE_LVDS) {
			}
		}
	}

	is_dump = FALSE;
	for (i = CTL_SEN_MODE_1; i < attr_param.max_senmode; i++) {
		mode_basic_param.mode = i;
		rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_MODE_BASIC, (void *)&mode_basic_param);
		if ((rt == CTL_SEN_E_OK) && ((mode_basic_param.mode_type ==  CTL_SEN_MODE_CCIR) || (mode_basic_param.mode_type ==  CTL_SEN_MODE_CCIR_INTERLACE))) {
			if (is_dump == FALSE) {
				DBG_DUMP("-----MODE_DVI-----\r\n");
				is_dump = TRUE;
			}
			dvi_param.mode = i;
			rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_DVI, &dvi_param);
			if (rt == E_OK) {
				if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
					DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
				}
				DBG_DUMP("%8s%15s%15s\r\n", "mode", "fmt", "dt_mode");
				DBG_DUMP("%8s%15s%15s\r\n", sen_get_senmode_str(dvi_param.mode), sen_get_dvifmt_str(dvi_param.fmt), sen_get_dvidatamode_str(dvi_param.data_mode));
			}
		}
	}

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_TEMP, (void *)&temp);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("-----TEMP-----\r\n");
		DBG_DUMP("%10s\r\n", "temp");
		DBG_DUMP("0x%.8x\r\n", (int)temp);
	}

	DBG_DUMP("-----FPS-----\r\n");
	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_FPS, (void *)&fps_param);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("%8s %8s %8s\r\n", "dft", "chg", "cur");
		DBG_DUMP("%8d %8d %8d\r\n", (int)fps_param.dft_fps, (int)fps_param.chg_fps, (int)fps_param.cur_fps);
	}


	DBG_DUMP("-----CLK_INFO-----\r\n");
	for (i = CTL_SEN_MODE_1; i < attr_param.max_senmode; i++) {
		clk_info_param.mode = i;
		rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_CLK_INFO, (void *)&clk_info_param);
		if (rt == CTL_SEN_E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%-12s%-12s%-12s%-12s%-12s%-12s\r\n", "mode", "mclk_freq", "mclk_sel", "mclk_src", "data_rate", "pclk");
			DBG_DUMP("%-12s%-12d%-12s%-12s%-12d%-12d\r\n"
					 , sen_get_senmode_str(clk_info_param.mode)
					 , (int)clk_info_param.mclk_freq
					 , sen_get_clksel_str(clk_info_param.mclk_sel)
					 , sen_get_clksrc_str(clk_info_param.mclk_src_sel)
					 , (int)clk_info_param.data_rate
					 , (int)clk_info_param.pclk);
		}
	}



	DBG_DUMP("-----MFR_OUTPUT_TIMING-----\r\n");
	DBG_DUMP("%10s%10s%5s %16s %16s\r\n", "mode", "fps", "num", "diff_row", "vd_diff_row");
	for (i = CTL_SEN_MODE_1; i < attr_param.max_senmode; i++) {
		UINT32 diff_row[CTL_SEN_MFRAME_MAX_NUM - 1] = {0};
		UINT32 diff_row_vd[CTL_SEN_MFRAME_MAX_NUM - 1] = {0};

		mode_basic_param.mode = i;
		rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_MODE_BASIC, (void *)&mode_basic_param);
		if (rt == E_OK) {
			mfr_output_timing_param.mode = i;
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				mfr_output_timing_param.frame_rate = fps_param.cur_fps;
			} else {
				mfr_output_timing_param.frame_rate = mode_basic_param.dft_fps;
			}
			mfr_output_timing_param.num = mode_basic_param.frame_num;
			mfr_output_timing_param.diff_row = &diff_row[0];
			mfr_output_timing_param.diff_row_vd = &diff_row_vd[0];
			rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_MFR_OUTPUT_TIMING, &mfr_output_timing_param);
		}
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%10s%10d%5d [%-4d,%-4d,%-4d] [%-4d,%-4d,%-4d] \r\n"
					 , sen_get_senmode_str(mfr_output_timing_param.mode)
					 , mfr_output_timing_param.frame_rate
					 , mfr_output_timing_param.num
					 , mfr_output_timing_param.diff_row[0]
					 , mfr_output_timing_param.diff_row[1]
					 , mfr_output_timing_param.diff_row[2]
					 , mfr_output_timing_param.diff_row_vd[0]
					 , mfr_output_timing_param.diff_row_vd[1]
					 , mfr_output_timing_param.diff_row_vd[2]);
		}
	}


	DBG_DUMP("-----MODE_BASIC-----\r\n");
	for (i = CTL_SEN_MODE_1; i < attr_param.max_senmode; i++) {
		mode_basic_param.mode = i;
		rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_MODE_BASIC, (void *)&mode_basic_param);
		if (rt == CTL_SEN_E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%5s%10s %18s %10s%10s%10s%10s%12s\r\n", "mode", "data_fmt", "mode_type", "dft_fps", "frame_num", "stpix", "depth", "fmt");
			DBG_DUMP("%5s%10s 0x%.16llx %10d%10d%10s%10d%12s\r\n"
					 , sen_get_senmode_str(mode_basic_param.mode)
					 , sen_get_datafmt_str(mode_basic_param.data_fmt)
					 , mode_basic_param.mode_type
					 , (int)mode_basic_param.dft_fps
					 , (int)mode_basic_param.frame_num
					 , sen_get_stpix_str(mode_basic_param.stpix)
					 , (int)mode_basic_param.pixel_depth
					 , sen_get_fmt_str(mode_basic_param.fmt));


			DBG_DUMP("%-5s %-20s\r\n", "", "mode_type(str)");
			if (mode_basic_param.frame_num == 1) {
				DBG_DUMP("%-5s %-30s\r\n", "", sen_get_modetype_str(mode_basic_param.mode_type, 0));
			} else if (mode_basic_param.frame_num == 2) {
				DBG_DUMP("%-5s %-30s %-30s\r\n", "", sen_get_modetype_str(mode_basic_param.mode_type, 0), sen_get_modetype_str(mode_basic_param.mode_type, 1));
			} else if (mode_basic_param.frame_num == 3) {
				DBG_DUMP("%-5s %-30s %-30s %-30s\r\n", "", sen_get_modetype_str(mode_basic_param.mode_type, 0), sen_get_modetype_str(mode_basic_param.mode_type, 1), sen_get_modetype_str(mode_basic_param.mode_type, 0));
			} else if (mode_basic_param.frame_num == 4) {
				DBG_DUMP("%-5s %-30s %-30s %-30s %-30s\r\n", "", sen_get_modetype_str(mode_basic_param.mode_type, 0), sen_get_modetype_str(mode_basic_param.mode_type, 1), sen_get_modetype_str(mode_basic_param.mode_type, 2), sen_get_modetype_str(mode_basic_param.mode_type, 3));
			}

			DBG_DUMP("%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s\r\n", "valid.w", "valid.h", "act.x", "act.y", "act.w", "act.h", "crp.w", "crp.h", "hd_sync", "hd_period", "vd_sync", "vd_period");
			DBG_DUMP("%10d%10d%10d%10d%10d%10d%10d%10d%10d%10d%10d%10d\r\n"
					 , (int)mode_basic_param.valid_size.w
					 , (int)mode_basic_param.valid_size.h
					 , (int)mode_basic_param.act_size[0].x
					 , (int)mode_basic_param.act_size[0].y
					 , (int)mode_basic_param.act_size[0].w
					 , (int)mode_basic_param.act_size[0].h
					 , (int)mode_basic_param.crp_size.w
					 , (int)mode_basic_param.crp_size.h
					 , (int)mode_basic_param.signal_info.hd_sync
					 , (int)mode_basic_param.signal_info.hd_period
					 , (int)mode_basic_param.signal_info.vd_sync
					 , (int)mode_basic_param.signal_info.vd_period);
			if ((mode_basic_param.mode_type == CTL_SEN_MODE_STAGGER_HDR) || (mode_basic_param.mode_type == CTL_SEN_MODE_PDAF)) {
				DBG_DUMP("%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s\r\n"
						 , "act.x", "[0]", "[1]", "[2]", "[3]"
						 , "act.y", "[0]", "[1]", "[2]", "[3]"
						 , "act.w", "[0]", "[1]", "[2]", "[3]"
						 , "act.h", "[0]", "[1]", "[2]", "[3]");
				DBG_DUMP("%-6s%-6d%-6d%-6d%-6d%-6s%-6d%-6d%-6d%-6d%-6s%-6d%-6d%-6d%-6d%-6s%-6d%-6d%-6d%-6d\r\n"
						 , ""
						 , (int)mode_basic_param.act_size[0].x
						 , (int)mode_basic_param.act_size[1].x
						 , (int)mode_basic_param.act_size[2].x
						 , (int)mode_basic_param.act_size[3].x
						 , ""
						 , (int)mode_basic_param.act_size[0].y
						 , (int)mode_basic_param.act_size[1].y
						 , (int)mode_basic_param.act_size[2].y
						 , (int)mode_basic_param.act_size[3].y
						 , ""
						 , (int)mode_basic_param.act_size[0].w
						 , (int)mode_basic_param.act_size[1].w
						 , (int)mode_basic_param.act_size[2].w
						 , (int)mode_basic_param.act_size[3].w
						 , ""
						 , (int)mode_basic_param.act_size[0].h
						 , (int)mode_basic_param.act_size[1].h
						 , (int)mode_basic_param.act_size[2].h
						 , (int)mode_basic_param.act_size[3].h);
			}
			DBG_DUMP("%11s%10s%10s%10s%10s%10s\r\n", "ratio_h_v", "gain.min", "gain.max", "bining_rt", "row_t", "row_ts");
			DBG_DUMP("0x%.8x%10d%10d%10d%10d%10d\r\n"
					 , (int)mode_basic_param.ratio_h_v
					 , (int)mode_basic_param.gain.min
					 , (int)mode_basic_param.gain.max
					 , (int)mode_basic_param.bining_ratio
					 , (int)mode_basic_param.row_time
					 , (int)mode_basic_param.row_time_step);
		}
	}

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_PLUG, (void *)&plug);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("-----PLUG-----\r\n");
		DBG_DUMP("%8s\r\n", "b_plug");
		DBG_DUMP("%8d\r\n", (int)plug);
	}

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_SENDRV_VER, (void *)&value[0]);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("-----VER-----\r\n");
		DBG_DUMP("%11s\r\n", "version");
		DBG_DUMP("0x%.8x\r\n", (int)value[0]);
	}

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_FLIP_TYPE, (void *)&flip);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("-----FLIP-----\r\n");
		DBG_DUMP("%11s\r\n", "flip");
		DBG_DUMP("0x%.8x\r\n", (int)flip);
	}

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_INIT_IF_TIMEOUT_MS, (void *)&value[0]);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("-----TIMEOUT-----\r\n");
		DBG_DUMP("%-12s\r\n", "timeout(ms)");
		DBG_DUMP("%-12d\r\n", (int)value[0]);
	}

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_INIT_SIGNAL_SYNC, (void *)&value[0]);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("-----SIGNAL_SYNC-----\r\n");
		DBG_DUMP("%-12s\r\n", "sync");
		DBG_DUMP("0x%.8x\r\n", (int)value[0]);
	}

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_TGE_VD_PERIOD, (void *)&value[0]);
	rt += sen_ctrl_get_cfg(id, CTL_SEN_CFGID_TGE_HD_PERIOD, (void *)&value[1]);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("-----TGE_PERIOD-----\r\n");
		DBG_DUMP("%-12s  %-12s\r\n", "vd", "hd");
		DBG_DUMP("0x%.8x  0x%.8x\r\n", (int)value[0], (int)value[1]);
	}


	DBG_DUMP("\r\n\r\n==================sensor status==================\r\n\r\n");
	DBG_DUMP("%-8s%-12s\r\n", "SEL", "dest");
	DBG_DUMP("%-8s0x%.8x\r\n"
			 , sen_get_modesel_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.mode_sel)
			 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.output_dest);
	if (g_ctl_sen_ctrl_obj[id]->cur_chgmode.mode_sel == CTL_SEN_MODESEL_AUTO) {
		DBG_DUMP("%-8s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-15s%-10s%-10s %-18s %-10s\r\n"
				 , "AUTO:", "fps", "sz.w", "sz.h", "num", "fmt", "bit", "ccir_int", "ccir_fmt", "mux_en", "mux_num", "mode_type", "dl");
		DBG_DUMP("%-8s%-10d%-10d%-10d%-10d%-10s%-10d%-10d%-15s%-10d%-10d 0x%.16llx %-10d\r\n"
				 , ""
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.frame_rate
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.size.w
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.size.h
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.frame_num
				 , sen_get_datafmt_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.data_fmt)
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.pixdepth
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.ccir.interlace
				 , sen_get_ccirfmt_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.ccir.fmt)
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mux_singnal_en
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mux_signal_info.mux_data_num
				 , g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.data_lane);
		DBG_DUMP("%-20s\r\n", "mode_type(str)");
		if (mode_basic_param.frame_num == 1) {
			DBG_DUMP("%-30s\r\n", sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 0));
		} else if (mode_basic_param.frame_num == 2) {
			DBG_DUMP("%-30s %-30s\r\n", sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 0), sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 1));
		} else if (mode_basic_param.frame_num == 3) {
			DBG_DUMP("%-30s %-30s %-30s\r\n", sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 0), sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 1), sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 0));
		} else if (mode_basic_param.frame_num == 4) {
			DBG_DUMP("%-30s %-30s %-30s %-30s\r\n", sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 0), sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 1), sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 2), sen_get_modetype_str(g_ctl_sen_ctrl_obj[id]->cur_chgmode.auto_info.mode_type_sel, 3));
		}
	} else if (g_ctl_sen_ctrl_obj[id]->cur_chgmode.mode_sel == CTL_SEN_MODESEL_MANUAL) {
		DBG_DUMP("%-8s%-10s%-10s\r\n", "MANUAL:", "fps", "mode");
		DBG_DUMP("%-8s%-10d%-10d\r\n", ""
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.manual_info.frame_rate
				 , (int)g_ctl_sen_ctrl_obj[id]->cur_chgmode.manual_info.sen_mode);
	}
	DBG_DUMP("%12s", "cur_senmode");
	DBG_DUMP("%12s\r\n"
			 , sen_get_senmode_str(g_ctl_sen_ctrl_obj[id]->cur_sen_mode));

	sen_dbg_set_er_unlock(id);
}

static void sen_dbg_dump_drv_info(CTL_SEN_ID id)
{
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	ER rt;
	CTL_SENDRV_GET_ATTR_BASIC_PARAM drv_attr_basic_param;
	CTL_SENDRV_GET_ATTR_SIGNAL_PARAM drv_attr_signal_param;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM drv_attr_cmdif_param;
	CTL_SENDRV_GET_ATTR_IF_PARAM drv_attr_if_param;
	CTL_SENDRV_GET_TEMP_PARAM drv_temp_param;
	CTL_SENDRV_GET_FPS_PARAM drv_fps_param;
	CTL_SENDRV_GET_SPEED_PARAM drv_speed_param;
	CTL_SENDRV_GET_MODESEL_PARAM drv_modesel_param;
	CTL_SENDRV_GET_MODE_BASIC_PARAM drv_mode_basic_param;
	CTL_SENDRV_GET_MODE_LVDS_PARAM drv_mode_lvds_param;
	CTL_SENDRV_GET_MODE_MIPI_PARAM drv_mode_mipi_param;
	CTL_SENDRV_GET_MODE_MIPI_CLANE_CMETHOD drv_mode_mipi_clane_param;
	CTL_SENDRV_GET_MODE_MIPI_HSDATAOUT_DLY drv_mode_mipi_hsdly_param;
	CTL_SENDRV_GET_MODE_MANUAL_IADJ drv_mode_mipi_manual_iadj_param;
	CTL_SENDRV_GET_MODE_ROWTIME_PARAM drv_mode_rowtime_param;
	CTL_SENDRV_GET_MFR_OUTPUT_TIMING_PARAM drv_mfr_output_timing_param;
	CTL_SENDRV_GET_MODE_DVI_PARAM drv_mode_dvi_param;
	CTL_SENDRV_GET_MODE_TGE_PARAM drv_mode_tge_param;
	CTL_SENDRV_GET_PROBE_SEN_PARAM drv_probe_sen_param;
	CTL_SENDRV_GET_PLUG_INFO_PARAM drv_plug_info_param;
	UINT32 drv_sendrv_ver = 0;
	UINT32 i;

	sen_dbg_set_er_lock(id);

	DBG_DUMP("\r\n==================sensor driver info==================\r\n");
	memset((void *)(&drv_attr_basic_param), 0, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
	memset((void *)(&drv_attr_signal_param), 0, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
	memset((void *)(&drv_attr_cmdif_param), 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	memset((void *)(&drv_attr_if_param), 0, sizeof(CTL_SENDRV_GET_ATTR_IF_PARAM));
	memset((void *)(&drv_temp_param), 0, sizeof(CTL_SENDRV_GET_TEMP_PARAM));
	memset((void *)(&drv_fps_param), 0, sizeof(CTL_SENDRV_GET_FPS_PARAM));
	memset((void *)(&drv_speed_param), 0, sizeof(CTL_SENDRV_GET_SPEED_PARAM));
	memset((void *)(&drv_modesel_param), 0, sizeof(CTL_SENDRV_GET_MODESEL_PARAM));
	memset((void *)(&drv_mode_basic_param), 0, sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
	memset((void *)(&drv_mode_lvds_param), 0, sizeof(CTL_SENDRV_GET_MODE_LVDS_PARAM));
	memset((void *)(&drv_mode_mipi_param), 0, sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
	memset((void *)(&drv_mode_mipi_clane_param), 0, sizeof(CTL_SENDRV_GET_MODE_MIPI_CLANE_CMETHOD));
	memset((void *)(&drv_mode_mipi_hsdly_param), 0, sizeof(CTL_SENDRV_GET_MODE_MIPI_HSDATAOUT_DLY));
	memset((void *)(&drv_mode_mipi_manual_iadj_param), 0, sizeof(CTL_SENDRV_GET_MODE_MANUAL_IADJ));
	memset((void *)(&drv_mode_rowtime_param), 0, sizeof(CTL_SENDRV_GET_MODE_ROWTIME_PARAM));
	memset((void *)(&drv_mfr_output_timing_param), 0, sizeof(CTL_SENDRV_GET_MFR_OUTPUT_TIMING_PARAM));
	memset((void *)(&drv_mode_dvi_param), 0, sizeof(CTL_SENDRV_GET_MODE_DVI_PARAM));
	memset((void *)(&drv_mode_tge_param), 0, sizeof(CTL_SENDRV_GET_MODE_TGE_PARAM));
	memset((void *)(&drv_probe_sen_param), 0, sizeof(CTL_SENDRV_GET_PROBE_SEN_PARAM));
	memset((void *)(&drv_plug_info_param), 0, sizeof(CTL_SENDRV_GET_PLUG_INFO_PARAM));

	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_BASIC, (void *)&drv_attr_basic_param);
	if (rt == E_OK) {
		DBG_DUMP("-----ATTR_BASIC-----\r\n");
		DBG_DUMP("%20s %10s %10s %10s %4s\r\n", "name", "vendor", "maxmode", "sp_prop", "sync");
		DBG_DUMP("%20s %10s %10d 0x%.8x %4d\r\n"
				 , drv_attr_basic_param.name
				 , sen_get_vendor_str(drv_attr_basic_param.vendor)
				 , (int)drv_attr_basic_param.max_senmode
				 , (int)drv_attr_basic_param.property
				 , (int)drv_attr_basic_param.sync_timing);

	}

	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_SIGNAL, (void *)&drv_attr_signal_param);
	if (rt == E_OK) {
		DBG_DUMP("-----ATTR_SIGNAL-----\r\n");
		DBG_DUMP("%6s %6s %6s %6s %6s %6s\r\n", "type", "vd_act", "hd_act", "vd_pha", "hd_pha", "dt_pha");
		DBG_DUMP("%6s %6s %6s %6s %6s %6s\r\n"
				 , sen_get_signaltype_str(drv_attr_signal_param.type)
				 , sen_get_activesel_str(drv_attr_signal_param.info.vd_inv)
				 , sen_get_activesel_str(drv_attr_signal_param.info.hd_inv)
				 , sen_get_phasesel_str(drv_attr_signal_param.info.vd_phase)
				 , sen_get_phasesel_str(drv_attr_signal_param.info.hd_phase)
				 , sen_get_phasesel_str(drv_attr_signal_param.info.data_phase));
	}

	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, (void *)&drv_attr_cmdif_param);
	if (rt == E_OK) {
		DBG_DUMP("-----ATTR_CMDIF-----\r\n");
		DBG_DUMP("%6s\r\n", "type");
		DBG_DUMP("%6s\r\n", sen_get_cmdiftype_str(drv_attr_cmdif_param.type));
		if ((drv_attr_cmdif_param.type == CTL_SEN_CMDIF_TYPE_I2C) || (drv_attr_cmdif_param.type == CTL_SEN_CMDIF_TYPE_VX1)) {
			DBG_DUMP("%-6s%-12s%-8s%-12s\r\n", "ch", "w_addr_sel", "w_addr", "clk");
			DBG_DUMP("%-6s%-12s0x%-.2x    %-12d\r\n"
					 , sen_get_i2cch_str(drv_attr_cmdif_param.info.i2c.ch)
					 , sen_get_i2cwsel_str(drv_attr_cmdif_param.info.i2c.cur_w_addr_info.w_addr_sel)
					 , (int)drv_attr_cmdif_param.info.i2c.cur_w_addr_info.w_addr
					 , (int)drv_attr_cmdif_param.info.i2c.s_clk);
			DBG_DUMP("%-10s:%-8s%-10s:%-8s%-10s:%-8s%-10s:%-8s%-10s:%-8s%-10s:%-8s\r\n"
					 , "w_addr_sel", "w_addr", "w_addr_sel", "w_addr", "w_addr_sel", "w_addr", "w_addr_sel", "w_addr", "w_addr_sel", "w_addr", "w_addr_sel", "w_addr");
			DBG_DUMP("%-10s:0x%-.2x    %-10s:0x%-.2x    %-10s:0x%-.2x    %-10s:0x%-.2x    %-10s:0x%-.2x    %-10s:0x%-.2x\r\n"
					 , sen_get_i2cwsel_str(drv_attr_cmdif_param.info.i2c.w_addr_info[0].w_addr_sel), (int)drv_attr_cmdif_param.info.i2c.w_addr_info[0].w_addr
					 , sen_get_i2cwsel_str(drv_attr_cmdif_param.info.i2c.w_addr_info[1].w_addr_sel), (int)drv_attr_cmdif_param.info.i2c.w_addr_info[1].w_addr
					 , sen_get_i2cwsel_str(drv_attr_cmdif_param.info.i2c.w_addr_info[2].w_addr_sel), (int)drv_attr_cmdif_param.info.i2c.w_addr_info[2].w_addr
					 , sen_get_i2cwsel_str(drv_attr_cmdif_param.info.i2c.w_addr_info[3].w_addr_sel), (int)drv_attr_cmdif_param.info.i2c.w_addr_info[3].w_addr
					 , sen_get_i2cwsel_str(drv_attr_cmdif_param.info.i2c.w_addr_info[4].w_addr_sel), (int)drv_attr_cmdif_param.info.i2c.w_addr_info[4].w_addr
					 , sen_get_i2cwsel_str(drv_attr_cmdif_param.info.i2c.w_addr_info[5].w_addr_sel), (int)drv_attr_cmdif_param.info.i2c.w_addr_info[5].w_addr);
		}
		if (drv_attr_cmdif_param.type == CTL_SEN_CMDIF_TYPE_VX1) {
			DBG_DUMP("%-12s%-12s%-12s\r\n", "241_cl_sp", "241_clk", "rl_mclk");
			DBG_DUMP("%-12d%-12d%-12d\r\n", (int)drv_attr_cmdif_param.vx1.tx241_clane_speed, (int)drv_attr_cmdif_param.vx1.tx241_input_clk_freq, (int)drv_attr_cmdif_param.vx1.tx241_real_mclk);
		}
	}

	drv_attr_if_param.type = init_cfg_obj.if_cfg.type;
	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_IF, (void *)&drv_attr_if_param);
	if (rt == E_OK) {
		DBG_DUMP("-----ATTR_IF-----\r\n");
		DBG_DUMP("%6s\r\n", "type");
		DBG_DUMP("%6s\r\n", sen_get_iftype_str(drv_attr_if_param.type));
	}

	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_TEMP, (void *)&drv_temp_param);
	if (rt == E_OK) {
		DBG_DUMP("-----TEMP-----\r\n");
		DBG_DUMP("%10s\r\n", "temp");
		DBG_DUMP("0x%.8x\r\n", (int)drv_temp_param.temp);
	}

	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_PROBE_SEN, (void *)&drv_probe_sen_param);
	if (rt == E_OK) {
		DBG_DUMP("-----PROBE SEN-----\r\n");
		DBG_DUMP("%7s\r\n", "probe_rst");
		DBG_DUMP("%7d\r\n", (int)drv_probe_sen_param.probe_rst);
	}

	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_PLUG_INFO, (void *)&drv_plug_info_param);
	if (rt == E_OK) {
		DBG_DUMP("-----PLUG INFO-----\r\n");
		DBG_DUMP("%7s%7s%7s%12s%8s%8s%8s%8s%8s\r\n", "sz.w", "sz.h", "fps", "itl", "pr[0]", "pr[1]", "pr[2]", "pr[3]", "pr[4]");
		DBG_DUMP("%7d%7d0x%.8x%7d%8d%8d%8d%8d%8d\r\n", (int)drv_plug_info_param.size.w, (int)drv_plug_info_param.size.h, (int)drv_plug_info_param.fps, (int)drv_plug_info_param.interlace
				 , (int)drv_plug_info_param.param[0], (int)drv_plug_info_param.param[1], (int)drv_plug_info_param.param[2], (int)drv_plug_info_param.param[3], (int)drv_plug_info_param.param[4]);
	}

	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_SENDRV_VER, (void *)&drv_sendrv_ver);
	if (rt == E_OK) {
		DBG_DUMP("-----SENDRV VER-----\r\n");
		DBG_DUMP("%10s\r\n", "ver");
		DBG_DUMP("0x%.8x\r\n", (int)drv_sendrv_ver);
	}

	DBG_DUMP("-----FPS-----\r\n");
	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_FPS, (void *)&drv_fps_param);
	if (rt == E_OK) {
		DBG_DUMP("%8s %8s\r\n", "chg", "cur");
		DBG_DUMP("%8d %8d\r\n", (int)drv_fps_param.chg_fps, (int)drv_fps_param.cur_fps);
	}

	DBG_DUMP("-----SPEED-----\r\n");
	for (i = CTL_SEN_MODE_1; i < drv_attr_basic_param.max_senmode; i++) {
		drv_speed_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_SPEED, (void *)&drv_speed_param);
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%5s %10s %10s %10s %10s\r\n", "mode", "mclk_src", "mclk", "pclk", "dt_rate");
			DBG_DUMP("%5s %10s %10d %10d %10d \r\n", sen_get_senmode_str(drv_speed_param.mode), sen_get_siemclksrc_str(drv_speed_param.mclk_src)
					 , (int)drv_speed_param.mclk, (int)drv_speed_param.pclk, (int)drv_speed_param.data_rate);
		}
	}

	/*rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODESEL, (void *)&drv_modesel_param);
	if (rt == CTL_SEN_E_OK) {
	    DBG_DUMP("-----MODESEL-----\r\n");
	}*/

	DBG_DUMP("-----MODE_BASIC-----\r\n");
	for (i = CTL_SEN_MODE_1; i < drv_attr_basic_param.max_senmode; i++) {
		drv_mode_basic_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_BASIC, (void *)&drv_mode_basic_param);
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%10s%10s%10s %18s %10s%10s%12s%10s%12s\r\n", "mode", "if_type", "data_fmt", "mode_type", "dft_fps", "frame_num", "stpix", "pixel_depth", "fmt");
			DBG_DUMP("%10s%10s%10s 0x%.16llx %10d%10d%12s%10d%12s\r\n"
					 , sen_get_senmode_str(drv_mode_basic_param.mode)
					 , sen_get_iftype_str(drv_mode_basic_param.if_type)
					 , sen_get_datafmt_str(drv_mode_basic_param.data_fmt)
					 , drv_mode_basic_param.mode_type
					 , (int)drv_mode_basic_param.dft_fps
					 , (int)drv_mode_basic_param.frame_num
					 , sen_get_stpix_str(drv_mode_basic_param.stpix)
					 , (int)drv_mode_basic_param.pixel_depth
					 , sen_get_fmt_str(drv_mode_basic_param.fmt));

			DBG_DUMP("%-10s %-20s\r\n", "", "mode_type(str)");
			if (drv_mode_basic_param.frame_num == 1) {
				DBG_DUMP("%-10s %-30s\r\n", "", sen_get_modetype_str(drv_mode_basic_param.mode_type, 0));
			} else if (drv_mode_basic_param.frame_num == 2) {
				DBG_DUMP("%-10s %-30s %-30s\r\n", "", sen_get_modetype_str(drv_mode_basic_param.mode_type, 0), sen_get_modetype_str(drv_mode_basic_param.mode_type, 1));
			} else if (drv_mode_basic_param.frame_num == 3) {
				DBG_DUMP("%-10s %-30s %-30s %-30s\r\n", "", sen_get_modetype_str(drv_mode_basic_param.mode_type, 0), sen_get_modetype_str(drv_mode_basic_param.mode_type, 1), sen_get_modetype_str(drv_mode_basic_param.mode_type, 0));
			} else if (drv_mode_basic_param.frame_num == 4) {
				DBG_DUMP("%-10s %-30s %-30s %-30s %-30s\r\n", "", sen_get_modetype_str(drv_mode_basic_param.mode_type, 0), sen_get_modetype_str(drv_mode_basic_param.mode_type, 1), sen_get_modetype_str(drv_mode_basic_param.mode_type, 2), sen_get_modetype_str(drv_mode_basic_param.mode_type, 3));
			}

			DBG_DUMP("%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s\r\n", "valid.w", "valid.h", "act.x", "act.y", "act.w", "act.h", "crp.w", "crp.h", "hd_sync", "hd_period", "vd_sync", "vd_period");
			DBG_DUMP("%12d%12d%12d%12d%12d%12d%12d%12d%12d%12d%12d%12d\r\n"
					 , (int)drv_mode_basic_param.valid_size.w
					 , (int)drv_mode_basic_param.valid_size.h
					 , (int)drv_mode_basic_param.act_size[0].x
					 , (int)drv_mode_basic_param.act_size[0].y
					 , (int)drv_mode_basic_param.act_size[0].w
					 , (int)drv_mode_basic_param.act_size[0].h
					 , (int)drv_mode_basic_param.crp_size.w
					 , (int)drv_mode_basic_param.crp_size.h
					 , (int)drv_mode_basic_param.signal_info.hd_sync
					 , (int)drv_mode_basic_param.signal_info.hd_period
					 , (int)drv_mode_basic_param.signal_info.vd_sync
					 , (int)drv_mode_basic_param.signal_info.vd_period);
			if ((drv_mode_basic_param.mode_type == CTL_SEN_MODE_STAGGER_HDR) || (drv_mode_basic_param.mode_type == CTL_SEN_MODE_PDAF)) {
				DBG_DUMP("%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s%-6s\r\n"
						 , "act.x", "[0]", "[1]", "[2]", "[3]"
						 , "act.y", "[0]", "[1]", "[2]", "[3]"
						 , "act.w", "[0]", "[1]", "[2]", "[3]"
						 , "act.h", "[0]", "[1]", "[2]", "[3]");
				DBG_DUMP("%-6s%-6d%-6d%-6d%-6d%-6s%-6d%-6d%-6d%-6d%-6s%-6d%-6d%-6d%-6d%-6s%-6d%-6d%-6d%-6d\r\n"
						 , ""
						 , (int)drv_mode_basic_param.act_size[0].x
						 , (int)drv_mode_basic_param.act_size[1].x
						 , (int)drv_mode_basic_param.act_size[2].x
						 , (int)drv_mode_basic_param.act_size[3].x
						 , ""
						 , (int)drv_mode_basic_param.act_size[0].y
						 , (int)drv_mode_basic_param.act_size[1].y
						 , (int)drv_mode_basic_param.act_size[2].y
						 , (int)drv_mode_basic_param.act_size[3].y
						 , ""
						 , (int)drv_mode_basic_param.act_size[0].w
						 , (int)drv_mode_basic_param.act_size[1].w
						 , (int)drv_mode_basic_param.act_size[2].w
						 , (int)drv_mode_basic_param.act_size[3].w
						 , ""
						 , (int)drv_mode_basic_param.act_size[0].h
						 , (int)drv_mode_basic_param.act_size[1].h
						 , (int)drv_mode_basic_param.act_size[2].h
						 , (int)drv_mode_basic_param.act_size[3].h);
			}
			DBG_DUMP("%14s%12s%12s%12s\r\n", "ratio_h_v", "gain.min", "gain.max", "bining_rt");
			DBG_DUMP("0x%.8x%12d%12d%12d\r\n"
					 , (int)drv_mode_basic_param.ratio_h_v
					 , (int)drv_mode_basic_param.gain.min
					 , (int)drv_mode_basic_param.gain.max
					 , (int)drv_mode_basic_param.bining_ratio);
		}
	}

	DBG_DUMP("-----MODE_LVDS-----\r\n");
	for (i = CTL_SEN_MODE_1; i < drv_attr_basic_param.max_senmode; i++) {
		drv_mode_lvds_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_LVDS, &drv_mode_lvds_param);
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%6s%6s%6s%6s%6s%6s%6s%6s%6s%6s%6s%8s%8s%8s%8s%12s%8s%8s\r\n", "mode", "cl", "dl", "odr0", "odr1", "odr2", "odr3", "odr4", "odr5", "odr6", "odr7", "sl_id0", "sl_id1", "sl_id2", "sl_id3", "sl_ofs", "fs_bit", "odr_in");
			DBG_DUMP("%6s%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%8d%8d%8d%8d 0x%.8x%8d%8s\r\n"
					 , sen_get_senmode_str(drv_mode_lvds_param.mode)
					 , (int)drv_mode_lvds_param.clk_lane
					 , (int)drv_mode_lvds_param.data_lane
					 , (int)drv_mode_lvds_param.output_pixel_order[0]
					 , (int)drv_mode_lvds_param.output_pixel_order[1]
					 , (int)drv_mode_lvds_param.output_pixel_order[2]
					 , (int)drv_mode_lvds_param.output_pixel_order[3]
					 , (int)drv_mode_lvds_param.output_pixel_order[4]
					 , (int)drv_mode_lvds_param.output_pixel_order[5]
					 , (int)drv_mode_lvds_param.output_pixel_order[6]
					 , (int)drv_mode_lvds_param.output_pixel_order[7]
					 , (int)drv_mode_lvds_param.sel_frm_id[0]
					 , (int)drv_mode_lvds_param.sel_frm_id[1]
					 , (int)drv_mode_lvds_param.sel_frm_id[2]
					 , (int)drv_mode_lvds_param.sel_frm_id[3]
					 , (int)drv_mode_lvds_param.sel_bit_ofs
					 , (int)drv_mode_lvds_param.fset_bit
					 , sen_get_datainbitorder_str(drv_mode_lvds_param.data_in_order));
		}
	}

	DBG_DUMP("-----MODE_MIPI-----\r\n");
	for (i = CTL_SEN_MODE_1; i < drv_attr_basic_param.max_senmode; i++) {
		drv_mode_mipi_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI, &drv_mode_mipi_param);
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%6s%6s%6s%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s\r\n"
					 , "mode", "cl", "dl", "mn_bit0", "mn_id0", "mn_bit1", "mn_id1", "mn_bit2", "mn_id2"
					 , "svpwr", "sl_id0", "sl_id1", "sl_id2", "sl_id3", "sl_ofs");
			DBG_DUMP("%6s%6d%6d%8d%8d%8d%8d%8d%8d%8d%8d%8d%8d%8d%8d\r\n"
					 , sen_get_senmode_str(drv_mode_mipi_param.mode)
					 , (int)drv_mode_mipi_param.clk_lane
					 , (int)drv_mode_mipi_param.data_lane
					 , (int)drv_mode_mipi_param.manual_info[0].bit, (int)drv_mode_mipi_param.manual_info[0].data_id
					 , (int)drv_mode_mipi_param.manual_info[1].bit, (int)drv_mode_mipi_param.manual_info[1].data_id
					 , (int)drv_mode_mipi_param.manual_info[2].bit, (int)drv_mode_mipi_param.manual_info[2].data_id
					 , (int)drv_mode_mipi_param.save_pwr
					 , (int)drv_mode_mipi_param.sel_frm_id[0], (int)drv_mode_mipi_param.sel_frm_id[1]
					 , (int)drv_mode_mipi_param.sel_frm_id[2], (int)drv_mode_mipi_param.sel_frm_id[3]
					 , (int)drv_mode_mipi_param.sel_bit_ofs);
		}

		drv_mode_mipi_clane_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI_CLANE_CMETHOD, &drv_mode_mipi_clane_param);
		if (rt == E_OK) {
			DBG_DUMP("%6s%16s\r\n", "mode", "clane_ctl_mode");
			DBG_DUMP("%6s%16d\r\n"
					 , sen_get_senmode_str(drv_mode_mipi_clane_param.mode)
					 , (int)drv_mode_mipi_clane_param.clane_ctl_mode);
		}

		drv_mode_mipi_hsdly_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI_HSDATAOUT_DLY, &drv_mode_mipi_hsdly_param);
		if (rt == E_OK) {
			DBG_DUMP("%6s%16s\r\n", "mode", "delay");
			DBG_DUMP("%6s%16d\r\n"
					 , sen_get_senmode_str(drv_mode_mipi_hsdly_param.mode)
					 , (int)drv_mode_mipi_hsdly_param.delay);
		}

		DBG_DUMP("%16s\r\n", "MIPI_EN_USER");
		DBG_DUMP("%16d\r\n", sen_uti_get_sendrv_en_mipi(i));

		drv_mode_mipi_manual_iadj_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI_MANUAL_IADJ, &drv_mode_mipi_manual_iadj_param);
		if (rt == E_OK) {
			if (drv_mode_mipi_manual_iadj_param.sel == CTL_SEN_MANUAL_IADJ_SEL_OFF) {
				DBG_DUMP("%6s%16s\r\n", "mode", "IADJ_SEL_OFF");
				DBG_DUMP("%6s%16s\r\n"
						 , sen_get_senmode_str(drv_mode_mipi_manual_iadj_param.mode)
						 , "--");
			} else if (drv_mode_mipi_manual_iadj_param.sel == CTL_SEN_MANUAL_IADJ_SEL_IADJ) {
				DBG_DUMP("%6s%16s\r\n", "mode", "val_iadj");
				DBG_DUMP("%6s%16d\r\n"
						 , sen_get_senmode_str(drv_mode_mipi_manual_iadj_param.mode)
						 , (int)drv_mode_mipi_manual_iadj_param.val.iadj);
			} else if (drv_mode_mipi_manual_iadj_param.sel == CTL_SEN_MANUAL_IADJ_SEL_DATARATE) {
				DBG_DUMP("%6s%16s\r\n", "mode", "val_dt_rate");
				DBG_DUMP("%6s%16d\r\n"
						 , sen_get_senmode_str(drv_mode_mipi_manual_iadj_param.mode)
						 , (int)drv_mode_mipi_manual_iadj_param.val.data_rate);
			} else {
				DBG_DUMP("%6s%16s\r\n", "mode", "IADJ_SEL_ERR");
				DBG_DUMP("%6s%16s\r\n"
						 , sen_get_senmode_str(drv_mode_mipi_manual_iadj_param.mode)
						 , "--");
			}
		}

	}

	DBG_DUMP("-----MODE_ROWTIME-----\r\n");
	for (i = CTL_SEN_MODE_1; i < drv_attr_basic_param.max_senmode; i++) {
		drv_mode_rowtime_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_ROWTIME, &drv_mode_rowtime_param);
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%10s%10s%10s\r\n", "mode", "row_t", "row_s");
			DBG_DUMP("%10s%10d%10d\r\n"
					 , sen_get_senmode_str(drv_mode_rowtime_param.mode)
					 , drv_mode_rowtime_param.row_time
					 , drv_mode_rowtime_param.row_time_step);
		}
	}

	DBG_DUMP("-----MFR_OUTPUT_TIMING-----\r\n");
	DBG_DUMP("%10s%10s%5s%16s %16s\r\n", "mode", "fps", "num", "diff_row", "diff_row_vd");
	for (i = CTL_SEN_MODE_1; i < drv_attr_basic_param.max_senmode; i++) {
		UINT32 diff_row[CTL_SEN_MFRAME_MAX_NUM - 1] = {0};
		UINT32 diff_row_vd[CTL_SEN_MFRAME_MAX_NUM - 1] = {0};

		drv_mode_basic_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_BASIC, (void *)&drv_mode_basic_param);
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			drv_mfr_output_timing_param.mode = i;
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				drv_mfr_output_timing_param.frame_rate = drv_fps_param.cur_fps;
			} else {
				drv_mfr_output_timing_param.frame_rate = drv_mode_basic_param.dft_fps;
			}
			drv_mfr_output_timing_param.num = drv_mode_basic_param.frame_num;
			drv_mfr_output_timing_param.diff_row = &diff_row[0];
			drv_mfr_output_timing_param.diff_row_vd = &diff_row_vd[0];
			rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MFR_OUTPUT_TIMING, &drv_mfr_output_timing_param);
		}
		if (rt == E_OK) {
			DBG_DUMP("%10s%10d%5d [%-4d,%-4d,%-4d] [%-4d,%-4d,%-4d]\r\n"
					 , sen_get_senmode_str(drv_mfr_output_timing_param.mode)
					 , drv_mfr_output_timing_param.frame_rate
					 , drv_mfr_output_timing_param.num
					 , drv_mfr_output_timing_param.diff_row[0]
					 , drv_mfr_output_timing_param.diff_row[1]
					 , drv_mfr_output_timing_param.diff_row[2]
					 , drv_mfr_output_timing_param.diff_row_vd[0]
					 , drv_mfr_output_timing_param.diff_row_vd[1]
					 , drv_mfr_output_timing_param.diff_row_vd[2] );
		}
	}

	DBG_DUMP("-----MODE_DVI-----\r\n");
	for (i = CTL_SEN_MODE_1; i < drv_attr_basic_param.max_senmode; i++) {
		drv_mode_dvi_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_DVI, &drv_mode_dvi_param);
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%8s%8s%8s\r\n"
					 , "mode", "fmt", "dt_mode");
			DBG_DUMP("%8s%8s%8s\r\n"
					 , sen_get_senmode_str(drv_mode_dvi_param.mode)
					 , sen_get_dvifmt_str(drv_mode_dvi_param.fmt)
					 , sen_get_dvidatamode_str(drv_mode_dvi_param.data_mode));
		}
	}

	DBG_DUMP("-----MODE_TGE-----\r\n");
	for (i = CTL_SEN_MODE_1; i < drv_attr_basic_param.max_senmode; i++) {
		drv_mode_tge_param.mode = i;
		rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_TGE, &drv_mode_tge_param);
		if (rt == E_OK) {
			if (i == g_ctl_sen_ctrl_obj[id]->cur_sen_mode) {
				DBG_DUMP("%s\r\n", SEN_STR_CUR_MODE);
			}
			DBG_DUMP("%-5s%-12s%-12s%-12s%-12s\r\n"
					 , "mode", "hd_sync", "hd_period", "vd_sync", "vd_period");
			DBG_DUMP("%-5s%-12d%-12d%-12d%-12d\r\n"
					 , sen_get_senmode_str(drv_mode_tge_param.mode)
					 , (int)drv_mode_tge_param.signal.hd_sync
					 , (int)drv_mode_tge_param.signal.hd_period
					 , (int)drv_mode_tge_param.signal.vd_sync
					 , (int)drv_mode_tge_param.signal.vd_period);
		}
	}

	sen_dbg_set_er_unlock(id);
}

static void sen_dbg_dump_ext_info(CTL_SEN_ID id)
{
	CTL_SEN_PINMUX *pinmux = NULL;

	sen_dbg_set_er_lock(id);

	DBG_DUMP("\r\n==================sensor ext info==================\r\n");

	/* IF info */
	DBG_DUMP("%10s\r\n", "iftype");
	DBG_DUMP("%10s\r\n", sen_get_iftype_str(g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type));
	DBG_DUMP("%15s %15s\r\n", "timeout_ms", "mclksrc_sync");
	DBG_DUMP("%15d 0x%.8x\r\n"
			 , (int)g_ctl_sen_init_obj[id]->init_ext_obj.if_info.timeout_ms
			 , (int)g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync);
	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
		DBG_DUMP("%-8s%-6s%-18s\r\n", "tge_en", "swap", "vd_src");
		DBG_DUMP("%-8d%-6d%-18s\r\n"
				 , (int)g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en
				 , (int)g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.swap
				 , sen_get_tge_sie_vd_src_str(g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.sie_vd_src));
	} else {
		DBG_DUMP("%8s\r\n", "tge_en");
		DBG_DUMP("%8d\r\n", (int)g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en);
	}

	/* PIN info */
	DBG_DUMP("%16s %16s\r\n", "PIN_FUNC", "cfg");
	pinmux = &g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.pinmux;
	while (pinmux != NULL) {
		DBG_DUMP("      0x%.8x       0x%.8x\r\n", (int)pinmux->func, (int)pinmux->cfg);
		pinmux = pinmux->pnext;
	}
	DBG_DUMP("\r\n");
	DBG_DUMP("%16s %8s: %3s %3s %3s %3s %3s %3s %3s %3s %8s %8s %8s\r\n", "CLANE_SEL", "data pin", "[0]", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "c_ml_sw", "c_vdhd", "vx1_cko");
	DBG_DUMP("%16s %8s  %3s %3s %3s %3s %3s %3s %3s %3s %8d %8d %8d"
			 , sen_get_clanesel_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.clk_lane_sel)
			 , ""
			 , sen_get_serialpin_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.sen_2_serial_pin_map[0])
			 , sen_get_serialpin_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.sen_2_serial_pin_map[1])
			 , sen_get_serialpin_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.sen_2_serial_pin_map[2])
			 , sen_get_serialpin_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.sen_2_serial_pin_map[3])
			 , sen_get_serialpin_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.sen_2_serial_pin_map[4])
			 , sen_get_serialpin_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.sen_2_serial_pin_map[5])
			 , sen_get_serialpin_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.sen_2_serial_pin_map[6])
			 , sen_get_serialpin_str(g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.sen_2_serial_pin_map[7])
			 , (int)g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.ccir_msblsb_switch
			 , (int)g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.ccir_vd_hd_pin
			 , (int)g_ctl_sen_init_obj[id]->init_cfg_obj.pin_cfg.vx1_tx241_cko_pin);
	DBG_DUMP("\r\n");

	/* CMD IF info */
	if (g_ctl_sen_init_obj[id]->init_cfg_obj.cmd_if_cfg.vx1.en) {
		DBG_DUMP("%-15s%-15s\r\n", "vx1_if_sel", "vx1_tx");
		DBG_DUMP("%-15d%-15s\r\n"
				 , (int)g_ctl_sen_init_obj[id]->init_cfg_obj.cmd_if_cfg.vx1.if_sel
				 , sen_get_vx1_txtype_str(g_ctl_sen_init_obj[id]->init_cfg_obj.cmd_if_cfg.vx1.tx_type));
	}
	DBG_DUMP("\r\n");

	sen_dbg_set_er_unlock(id);
}

static void sen_dbg_wait_inte(CTL_SEN_ID id, CTL_SEN_INTE inte)
{
#define WAIT_TIMES 10
	UINT64 cur_time_uint64[WAIT_TIMES + 1];
	long            period[WAIT_TIMES];
	UINT32 i = 0;
	INT32 rt;

	sen_dbg_set_er_lock(id);

	rt = sen_ctrl_get_cfg(id, CTL_SEN_CFGID_GET_VD_CNT, (void *)&i);
	if (rt == CTL_SEN_E_OK) {
		DBG_DUMP("vd cnt %d", i);
	}

	DBG_DUMP("\r\n==================sensor wait inte==================\r\n");

	if ((inte & CTL_SEN_VX1_TAG) == CTL_SEN_VX1_TAG) {
		sen_ctrl_cmdif_get_obj()->wait_interrupt(id, inte);
		for (i = 0; i < WAIT_TIMES + 1; i++) {
			cur_time_uint64[i] = hwclock_get_longcounter();
			sen_ctrl_cmdif_get_obj()->wait_interrupt(id, inte);
		}
	} else {
		sen_ctrl_if_get_obj()->wait_interrupt(id, inte);
		for (i = 0; i < WAIT_TIMES + 1; i++) {
			cur_time_uint64[i] = hwclock_get_longcounter();
			sen_ctrl_if_get_obj()->wait_interrupt(id, inte);
		}
	}

	for (i = 0; i < WAIT_TIMES; i++) {
		period[i] = cur_time_uint64[i + 1] - cur_time_uint64[i];
	}

	for (i = 0; i < WAIT_TIMES; i++) {
		DBG_DUMP("inte(%-5s) Time %ld us\n", sen_get_inte_str(inte), period[i]);
	}

	sen_dbg_set_er_unlock(id);
}

static void sen_dbg_dump_map_tbl(void)
{
	CTL_SEN_PINMUX *pinmux = NULL;
	UINT32 i;

	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		if ((strcmp(sen_map_tbl[i].name, "NULL") != 0) || ((UINT32)sen_map_tbl[i].sendrv_reg_obj.pwr_ctrl != 0) || ((UINT32)sen_map_tbl[i].sendrv_reg_obj.det_plug_in != 0) ||
			((UINT32)sen_map_tbl[i].sendrv_reg_obj.drv_tab != 0) || (sen_map_tbl[i].id != CTL_SEN_ID_MAX)) {
			DBG_DUMP("=====idx %d=====\r\n", (int)i);
			DBG_DUMP("%32s %14s %14s %14s %4s\r\n", "name", "sendrv(pwr)", "sendrv(det)", "sendrv(tbl)", "id");
			DBG_DUMP("%32s    0x%.8x     0x%.8x     0x%.8x %4d"
					 , sen_map_tbl[i].name
					 , (int)sen_map_tbl[i].sendrv_reg_obj.pwr_ctrl, (int)sen_map_tbl[i].sendrv_reg_obj.det_plug_in, (int)sen_map_tbl[i].sendrv_reg_obj.drv_tab
					 , (int)sen_map_tbl[i].id);
			DBG_DUMP("\r\n");


			/* IF info */
			DBG_DUMP("%10s\r\n", "iftype");
			DBG_DUMP("%10s\r\n", sen_get_iftype_str(sen_map_tbl[i].init_cfg_obj.if_cfg.type));
			DBG_DUMP("\r\n");


			/* PIN info */
			DBG_DUMP("%16s %16s\r\n", "PIN_FUNC", "cfg");
			pinmux = &sen_map_tbl[i].init_cfg_obj.pin_cfg.pinmux;
			while (pinmux != NULL) {
				DBG_DUMP("      0x%.8x       0x%.8x\r\n", (int)pinmux->func, (int)pinmux->cfg);
				pinmux = pinmux->pnext;
			}
			DBG_DUMP("\r\n");
			DBG_DUMP("%16s %8s: %3s %3s %3s %3s %3s %3s %3s %3s %8s %8s %8s %8s\r\n", "CLANE_SEL", "data pin", "[0]", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "c_ml_sw", "c_vdhd", "vx1_cko", "vx1_2ln");
			DBG_DUMP("%16s %8s  %3s %3s %3s %3s %3s %3s %3s %3s %8d %8d %8d %8d"
					 , sen_get_clanesel_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.clk_lane_sel)
					 , ""
					 , sen_get_serialpin_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.sen_2_serial_pin_map[0])
					 , sen_get_serialpin_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.sen_2_serial_pin_map[1])
					 , sen_get_serialpin_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.sen_2_serial_pin_map[2])
					 , sen_get_serialpin_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.sen_2_serial_pin_map[3])
					 , sen_get_serialpin_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.sen_2_serial_pin_map[4])
					 , sen_get_serialpin_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.sen_2_serial_pin_map[5])
					 , sen_get_serialpin_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.sen_2_serial_pin_map[6])
					 , sen_get_serialpin_str(sen_map_tbl[i].init_cfg_obj.pin_cfg.sen_2_serial_pin_map[7])
					 , (int)sen_map_tbl[i].init_cfg_obj.pin_cfg.ccir_msblsb_switch
					 , (int)sen_map_tbl[i].init_cfg_obj.pin_cfg.ccir_vd_hd_pin
					 , (int)sen_map_tbl[i].init_cfg_obj.pin_cfg.vx1_tx241_cko_pin
					 , (int)sen_map_tbl[i].init_cfg_obj.pin_cfg.vx1_tx241_cfg_2lane_mode);
			DBG_DUMP("\r\n");

			/* CTL_SEN_DRVDEV info */
			DBG_DUMP("%-16s\r\n", "CTL_SEN_DRVDEV");
			DBG_DUMP("0x%.8x\r\n", (unsigned int)sen_map_tbl[i].init_cfg_obj.drvdev);
			DBG_DUMP("\r\n");


			/* SENDRV info */
			DBG_DUMP("%-16s%-16s\r\n", "ad_chip_id", "ad_vin");
			DBG_DUMP("%-16d%-16d"
					 , (int)sen_map_tbl[i].init_cfg_obj.sendrv_cfg.ad_id_map.chip_id
					 , (int)sen_map_tbl[i].init_cfg_obj.sendrv_cfg.ad_id_map.vin_id);
			DBG_DUMP("\r\n");
		} else {
			DBG_DUMP("=====idx %d===== skip \r\n", (int)i);
		}
	}
}

static void sen_dbg_dump_proc_time(void)
{
	CTL_SEN_PROC_TIME *proc_time;
	UINT32 i, j, proc_cnt, idx;
	UINT64 pre_idx_ts = 0;

	DBG_DUMP("\r\n----------%14s----------\r\n", "sen proc begin");
	DBG_DUMP("%-3s%-16s%-8s%-18s%-18s\r\n", "id", "func(sub)", "item", "clock(us)", "diff(us)");

	proc_time = ctl_sen_get_proc_time();
	proc_cnt = ctl_sen_get_proc_time_cnt();
	for (i = 0; i < CTL_SEN_PROC_TIME_CNT_MAX; i++) {
		idx = (proc_cnt + i) % CTL_SEN_PROC_TIME_CNT_MAX;
		if (proc_time[idx].time_us_u64 != 0) {
			if (i == 0) {
				DBG_DUMP("%-3d%-16s%-8s%-18lld%-18s\r\n"
						 , (int)proc_time[idx].id, proc_time[idx].func_name, sen_get_proc_time_item_str(proc_time[idx].item)
						 , proc_time[idx].time_us_u64, "-");
			} else {
				DBG_DUMP("%-3d%-16s%-8s%-18lld%-18lld\r\n"
						 , (int)proc_time[idx].id, proc_time[idx].func_name, sen_get_proc_time_item_str(proc_time[idx].item)
						 , proc_time[idx].time_us_u64, proc_time[idx].time_us_u64 - pre_idx_ts);
			}


			pre_idx_ts = proc_time[idx].time_us_u64;
		}
	}

	DBG_DUMP("\r\n");
	DBG_DUMP("%-3s%-16s%-8s%-18s%-18s%-18s%-12s%-12s\r\n", "id", "func(sub)", "item", "clock(us)", "diff(us)", "tag", "cfg", "io_cnt");

	memcpy((void *)&dump_proc_time_adv[0], (void *)&sen_proc_time_adv[0], sizeof(sen_proc_time_adv)); // avoid racing

	proc_cnt = ctl_sen_get_proc_time_cnt_adv();
	for (i = 0; i < CTL_SEN_PROC_TIME_CNT_MAX; i++) {
		idx = (proc_cnt + i) % CTL_SEN_PROC_TIME_CNT_MAX;
		if (dump_proc_time_adv[idx].time_us_u64 != 0) {
			if (i == 0) {
				DBG_DUMP("%-3d%-16s%-8s%-18lld%-18s%-18d0x%.8x  %-12d\r\n"
						 , (int)dump_proc_time_adv[idx].id, dump_proc_time_adv[idx].func_name, sen_get_proc_time_item_str(dump_proc_time_adv[idx].item)
						 , dump_proc_time_adv[idx].time_us_u64, "-"
						 , dump_proc_time_adv[idx].tag, dump_proc_time_adv[idx].cfg
						 , (int)sen_proc_time_io_cnt[i]);
			} else {
				DBG_DUMP("%-3d%-16s%-8s%-18lld%-18lld%-18d0x%.8x\r\n"
						 , (int)dump_proc_time_adv[idx].id, dump_proc_time_adv[idx].func_name, sen_get_proc_time_item_str(dump_proc_time_adv[idx].item)
						 , dump_proc_time_adv[idx].time_us_u64, dump_proc_time_adv[idx].time_us_u64 - pre_idx_ts
						 , dump_proc_time_adv[idx].tag, dump_proc_time_adv[idx].cfg);
			}


			pre_idx_ts = dump_proc_time_adv[idx].time_us_u64;
		}
	}

	DBG_DUMP("\r\n");
	DBG_DUMP("%-3s%-12s%-8s\r\n", "id", "op", "io_cnt");
	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		for (j = 0; j < CTL_SEN_ER_OP_MAX; j++) {
			if (sen_op_io_cnt[i][j]) {
				DBG_DUMP("%-3d%-12s%-8d\r\n", i, sen_get_er_op_str(j), sen_op_io_cnt[i][j]);
			}
		}
	}
}

static void sen_dbg_dump_er(CTL_SEN_ID id)
{
	ctl_sen_er_log = ~(ctl_sen_er_log);
	DBG_DUMP("ERR LOG %s\r\n", (ctl_sen_er_log == 0) ? "OFF" : "ON");
}

static void sen_dbg_dbg_msg(CTL_SEN_ID id, BOOL en, CTL_SEN_MSG_TYPE type)
{
	if (id >= CTL_SEN_ID_MAX) {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		return;
	}

	if (en) {
		ctl_sen_msg_type[id] |= type;
	} else {
		ctl_sen_msg_type[id] &= (~type);
	}
}

static void sen_dbg_dump_ctl_info(CTL_SEN_ID id)
{
	UINT32 i;

	if (id >= CTL_SEN_ID_MAX_SUPPORT) {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		return;
	}



	DBG_DUMP("\r\n-----state-----\r\n");
	DBG_DUMP("%-5s%-12s\r\n", "id", "state");
	DBG_DUMP("%-5d0x%-10x\r\n", id, ctl_sen_get_cur_state(id));


	DBG_DUMP("\r\n-----context ctl-----\r\n");
	DBG_DUMP("%-7s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\r\n", "b_init", "ctx_num", "ctx_used", "ctx_size", "buf_addr", "buf_size", "kflow_sz", "kdrv_sz", "loc_alc");
	DBG_DUMP("%-7d0x%-10x0x%-10x0x%-10x0x%-10x0x%-10x0x%-10x0x%-10x%12d\r\n", ctl_sen_hdl_ctx.b_init, ctl_sen_hdl_ctx.context_num, ctl_sen_hdl_ctx.context_used, ctl_sen_hdl_ctx.context_size
			 , (int)ctl_sen_hdl_ctx.buf_addr, (int)ctl_sen_hdl_ctx.buf_size, (int)ctl_sen_hdl_ctx.kflow_size, (int)ctl_sen_hdl_ctx.kdrv_size, (int)ctl_sen_hdl_ctx.local_alloc);

	DBG_DUMP("%-5s%-12s%-12s%-12s\r\n", "id", "b_add_ctx", "init_obj", "ctrl_obj");
	DBG_DUMP("%-5d0x%-10x0x%-10x0x%-10x\r\n", id, b_add_ctx[id], (int)g_ctl_sen_init_obj[id], (int)g_ctl_sen_ctrl_obj[id]);

	DBG_DUMP("\r\n-----flg-----\r\n");
	for (i = 0; i < CTL_SEN_FLAG_COUNT; i++) {
		DBG_DUMP("%-5s%-5s\r\n", "cnt", "id");
		DBG_DUMP("%-5d%-5d\r\n", i, ctl_sen_flag_id[i]);
	}

	DBG_DUMP("\r\n-----sem-----\r\n");
	for (i = 0; i < CTL_SEN_SEMAPHORE_COUNT; i++) {
		DBG_DUMP("%-5s%-5s\r\n", "cnt", "id");
		DBG_DUMP("%-5d%-5d\r\n", i, ctl_sen_semtbl[i].semphore_id);
	}

}

static void sen_dbg_set_lv(CTL_SEN_DBG_LV dbg_lv)
{
	if (dbg_lv < CTL_SEN_DBG_LV_MAX) {
		ctl_sen_dbg_level = dbg_lv;
	} else {
		ctl_sen_dbg_level = CTL_SEN_DBG_LV_ERR;
		DBG_ERR("set level %d fail, force to ERR\r\n", dbg_lv);
	}
}

void ctl_sen_set_proc_time_adv(CHAR *func_name, UINT32 cfg, CTL_SEN_ID id, CTL_SEN_PROC_TIME_ITEM item, UINT32 tag)
{
	unsigned long flags;
	UINT32 cur_cnt;

	loc_cpu(flags);
	cur_cnt = sen_proc_time_cnt_adv;
	sen_proc_time_cnt_adv = (sen_proc_time_cnt_adv + 1) % CTL_SEN_PROC_TIME_CNT_MAX;
	if (item == CTL_SEN_PROC_TIME_ITEM_ENTER) {
		sen_proc_time_io_cnt[id]++;
	} else if (item == CTL_SEN_PROC_TIME_ITEM_EXIT) {
		sen_proc_time_io_cnt[id]--;
	}
	unl_cpu(flags);

	memcpy((void *)&sen_proc_time_adv[cur_cnt].func_name[0], (void *)func_name, sizeof(sen_proc_time_adv[cur_cnt].func_name));

	sen_proc_time_adv[cur_cnt].item = item;
	sen_proc_time_adv[cur_cnt].id = id;
	sen_proc_time_adv[cur_cnt].time_us_u64 = hwclock_get_longcounter();
	sen_proc_time_adv[cur_cnt].cfg = cfg;
	sen_proc_time_adv[cur_cnt].tag = tag;
}

CTL_SEN_PROC_TIME_ADV *ctl_sen_get_proc_time_adv(void)
{
	return &sen_proc_time_adv[0];
}

UINT32 ctl_sen_get_proc_time_cnt_adv(void)
{
	return sen_proc_time_cnt_adv;
}

void ctl_sen_set_op_io(CTL_SEN_ID id, CTL_SEN_ER_OP op, CTL_SEN_PROC_TIME_ITEM item)
{
	unsigned long flags;

	loc_cpu(flags);
	if (item == CTL_SEN_PROC_TIME_ITEM_ENTER) {
		sen_op_io_cnt[id][op]++;
	} else if (item == CTL_SEN_PROC_TIME_ITEM_EXIT) {
		sen_op_io_cnt[id][op]--;
	}
	unl_cpu(flags);
}

static CTL_SEN_DBG sen_dbg_tab = {
	sen_dbg_dump_info,
	sen_dbg_dump_ext_info,
	sen_dbg_dump_drv_info,
	sen_dbg_wait_inte,
	sen_dbg_dump_map_tbl,
	sen_dbg_dump_proc_time,
	sen_dbg_dbg_msg,
	sen_dbg_dump_ctl_info,
	sen_dbg_dump_er,
	sen_dbg_set_lv,
};

CTL_SEN_DBG *sen_dbg_get_obj(void)
{
	return &sen_dbg_tab;
}

