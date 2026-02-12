/*
    Sensor Interface library - utility

    Sensor Interface library.

    @file       sen_utility.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "sen_int.h"
#include "sen_id_map_int.h"

/*
    kdrv_ssenif globle ctrl - clk
*/
static CTL_SEN_CLK_CB sen_clken_cb[CTL_SEN_ID_MAX] = {NULL};
static CTL_SEN_CLKSRC_SEL sen_mclk_src[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 sen_mclk_freq[CTL_SEN_CLK_SEL_MAX] = {0}; // Hz
static BOOL sendrv_en_mipi[CTL_SEN_ID_MAX] = {0};

KDRV_SSENIF_INTERRUPT sen_ctrl_inte_cov2_ssenif(CTL_SEN_INTE inte)
{
	KDRV_SSENIF_INTERRUPT ssenif_inte = 0x0;

	if ((inte & CTL_SEN_INTE_VD) == CTL_SEN_INTE_VD) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_VD;
	}
	if ((inte & CTL_SEN_INTE_VD2) == CTL_SEN_INTE_VD2) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_VD2;
	}
	if ((inte & CTL_SEN_INTE_VD3) == CTL_SEN_INTE_VD3) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_VD3;
	}
	if ((inte & CTL_SEN_INTE_VD4) == CTL_SEN_INTE_VD4) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_VD4;
	}
	if ((inte & CTL_SEN_INTE_FRAMEEND) == CTL_SEN_INTE_FRAMEEND) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_FRAMEEND;
	}
	if ((inte & CTL_SEN_INTE_FRAMEEND2) == CTL_SEN_INTE_FRAMEEND2) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_FRAMEEND2;
	}
	if ((inte & CTL_SEN_INTE_FRAMEEND3) == CTL_SEN_INTE_FRAMEEND3) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_FRAMEEND3;
	}
	if ((inte & CTL_SEN_INTE_FRAMEEND4) == CTL_SEN_INTE_FRAMEEND4) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_FRAMEEND4;
	}
	if ((inte & CTL_SEN_INTE_ABORT) == CTL_SEN_INTE_ABORT) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_ABORT;
	}

	/* TO SIE */
	if ((inte & CTL_SEN_INTE_VD_TO_SIE0) == CTL_SEN_INTE_VD_TO_SIE0) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_SIE1_VD;
	}
	if ((inte & CTL_SEN_INTE_VD_TO_SIE1) == CTL_SEN_INTE_VD_TO_SIE1) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_SIE2_VD;
	}
	if ((inte & CTL_SEN_INTE_VD_TO_SIE3) == CTL_SEN_INTE_VD_TO_SIE3) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_SIE4_VD;
	}
	if ((inte & CTL_SEN_INTE_VD_TO_SIE4) == CTL_SEN_INTE_VD_TO_SIE4) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_SIE5_VD;
	}

	/* TO SIE */
	if ((inte & CTL_SEN_INTE_FMD_TO_SIE0) == CTL_SEN_INTE_FMD_TO_SIE0) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_SIE1_FRAMEEND;
	}
	if ((inte & CTL_SEN_INTE_FMD_TO_SIE1) == CTL_SEN_INTE_FMD_TO_SIE1) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_SIE2_FRAMEEND;
	}
	if ((inte & CTL_SEN_INTE_FMD_TO_SIE3) == CTL_SEN_INTE_FMD_TO_SIE3) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_SIE4_FRAMEEND;
	}
	if ((inte & CTL_SEN_INTE_FMD_TO_SIE4) == CTL_SEN_INTE_FMD_TO_SIE4) {
		ssenif_inte |= KDRV_SSENIF_INTERRUPT_SIE5_FRAMEEND;
	}

	return ssenif_inte;
}

CTL_SEN_INTE sen_ctrl_ssenifinte_cov2_sen(KDRV_SSENIF_INTERRUPT ssenif_inte)
{
	CTL_SEN_INTE inte = CTL_SEN_INTE_NONE;

	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_VD) == KDRV_SSENIF_INTERRUPT_VD) {
		inte |= CTL_SEN_INTE_VD;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_VD2) == KDRV_SSENIF_INTERRUPT_VD2) {
		inte |= CTL_SEN_INTE_VD2;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_VD3) == KDRV_SSENIF_INTERRUPT_VD3) {
		inte |= CTL_SEN_INTE_VD3;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_VD4) == KDRV_SSENIF_INTERRUPT_VD4) {
		inte |= CTL_SEN_INTE_VD4;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_FRAMEEND) == KDRV_SSENIF_INTERRUPT_FRAMEEND) {
		inte |= CTL_SEN_INTE_FRAMEEND;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_FRAMEEND2) == KDRV_SSENIF_INTERRUPT_FRAMEEND2) {
		inte |= CTL_SEN_INTE_FRAMEEND2;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_FRAMEEND3) == KDRV_SSENIF_INTERRUPT_FRAMEEND3) {
		inte |= CTL_SEN_INTE_FRAMEEND3;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_FRAMEEND4) == KDRV_SSENIF_INTERRUPT_FRAMEEND4) {
		inte |= CTL_SEN_INTE_FRAMEEND4;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_ABORT) == KDRV_SSENIF_INTERRUPT_ABORT) {
		inte |= CTL_SEN_INTE_ABORT;
	}

	/* TO SIE */
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_SIE1_VD) == KDRV_SSENIF_INTERRUPT_SIE1_VD) {
		inte |= CTL_SEN_INTE_VD_TO_SIE0;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_SIE2_VD) == KDRV_SSENIF_INTERRUPT_SIE2_VD) {
		inte |= CTL_SEN_INTE_VD_TO_SIE1;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_SIE4_VD) == KDRV_SSENIF_INTERRUPT_SIE4_VD) {
		inte |= CTL_SEN_INTE_VD_TO_SIE3;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_SIE5_VD) == KDRV_SSENIF_INTERRUPT_SIE5_VD) {
		inte |= CTL_SEN_INTE_VD_TO_SIE4;
	}

	/* TO SIE */
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_SIE1_FRAMEEND) == KDRV_SSENIF_INTERRUPT_SIE1_FRAMEEND) {
		inte |= CTL_SEN_INTE_FMD_TO_SIE0;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_SIE2_FRAMEEND) == KDRV_SSENIF_INTERRUPT_SIE2_FRAMEEND) {
		inte |= CTL_SEN_INTE_FMD_TO_SIE1;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_SIE4_FRAMEEND) == KDRV_SSENIF_INTERRUPT_SIE4_FRAMEEND) {
		inte |= CTL_SEN_INTE_FMD_TO_SIE3;
	}
	if ((ssenif_inte & KDRV_SSENIF_INTERRUPT_SIE5_FRAMEEND) == KDRV_SSENIF_INTERRUPT_SIE5_FRAMEEND) {
		inte |= CTL_SEN_INTE_FMD_TO_SIE4;
	}

	return inte;
}

UINT32 sen_ctrl_clanesel_conv2_ssenif(CTL_SEN_ID id, CTL_SEN_CLANE_SEL clane_sel)
{
	switch (clane_sel) {
	case CTL_SEN_CLANE_SEL_CSI0_USE_C0:
		return KDRV_SSENIF_CLANE_CSI0_USE_C0;
	case CTL_SEN_CLANE_SEL_CSI0_USE_C2:
		return KDRV_SSENIF_CLANE_CSI0_USE_C2;
	case CTL_SEN_CLANE_SEL_CSI1_USE_C4:
		return KDRV_SSENIF_CLANE_CSI1_USE_C4;
	case CTL_SEN_CLANE_SEL_CSI1_USE_C6:
		return KDRV_SSENIF_CLANE_CSI1_USE_C6;
	case CTL_SEN_CLANE_SEL_LVDS0_USE_C0C4:
		return KDRV_SSENIF_CLANE_LVDS0_USE_C0C4;
	case CTL_SEN_CLANE_SEL_LVDS0_USE_C2C6:
		return KDRV_SSENIF_CLANE_LVDS0_USE_C2C6;
	case CTL_SEN_CLANE_SEL_LVDS1_USE_C4:
		return KDRV_SSENIF_CLANE_LVDS1_USE_C4;
	case CTL_SEN_CLANE_SEL_LVDS1_USE_C6:
		return KDRV_SSENIF_CLANE_LVDS1_USE_C6;
	case CTL_SEN_CLANE_SEL_CSI1_USE_C1:
		return KDRV_SSENIF_CLANE_CSI1_USE_C1;
	case CTL_SEN_CLANE_SEL_LVDS1_USE_C1:
		return KDRV_SSENIF_CLANE_LVDS1_USE_C1;
	/* KDRV NOT SUPPORT select */
	case CTL_SEN_CLANE_SEL_CSI2_USE_C2:
	case CTL_SEN_CLANE_SEL_CSI3_USE_C6:
	case CTL_SEN_CLANE_SEL_CSI4_USE_C1:
	case CTL_SEN_CLANE_SEL_CSI5_USE_C3:
	case CTL_SEN_CLANE_SEL_CSI6_USE_C5:
	case CTL_SEN_CLANE_SEL_CSI7_USE_C7:
	case CTL_SEN_CLANE_SEL_LVDS2_USE_C2:
	case CTL_SEN_CLANE_SEL_LVDS3_USE_C6:
	case CTL_SEN_CLANE_SEL_LVDS4_USE_C1:
	case CTL_SEN_CLANE_SEL_LVDS5_USE_C3:
	case CTL_SEN_CLANE_SEL_LVDS6_USE_C5:
	case CTL_SEN_CLANE_SEL_LVDS7_USE_C7:
		return CTL_SEN_SKIP_SET_KDRV;
	default:
		CTL_SEN_DBG_ERR("\r\n");
	}
	return CTL_SEN_SKIP_SET_KDRV;
}

UINT32 sen_get_ssenif_cap(CTL_SEN_ID id)
{
	UINT32 drvdev = CTL_SEN_IGNORE;

	switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
	case CTL_SEN_IF_TYPE_MIPI:
		drvdev = sen_get_drvdev_csi(id);
		break;
	case CTL_SEN_IF_TYPE_LVDS:
		drvdev = sen_get_drvdev_lvds(id);
		break;
	case CTL_SEN_IF_TYPE_SLVSEC: // wait kdrv ready
	default:
		break;
	}

	if (drvdev == CTL_SEN_IGNORE) {
		CTL_SEN_DBG_ERR("id %d get drvdev fail\r\n", id);
		return 0;
	}

	if (nvt_get_chip_id() == CHIP_NA51055) {
		switch (drvdev) {
		case KDRV_SSENIF_ENGINE_CSI0:
		case KDRV_SSENIF_ENGINE_LVDS0:
			return KDRV_SSENIF_CAP_CSILVDS0;
		case KDRV_SSENIF_ENGINE_CSI1:
		case KDRV_SSENIF_ENGINE_LVDS1:
			return KDRV_SSENIF_CAP_CSILVDS1;
		case KDRV_SSENIF_ENGINE_CSI2:
		case KDRV_SSENIF_ENGINE_LVDS2:
			return KDRV_SSENIF_CAP_CSILVDS2;
		case KDRV_SSENIF_ENGINE_CSI3:
		case KDRV_SSENIF_ENGINE_LVDS3:
			return KDRV_SSENIF_CAP_CSILVDS3;
		case KDRV_SSENIF_ENGINE_CSI4:
		case KDRV_SSENIF_ENGINE_LVDS4:
			return KDRV_SSENIF_CAP_CSILVDS4;
		case KDRV_SSENIF_ENGINE_CSI5:
		case KDRV_SSENIF_ENGINE_LVDS5:
			return KDRV_SSENIF_CAP_CSILVDS5;
		case KDRV_SSENIF_ENGINE_CSI6:
		case KDRV_SSENIF_ENGINE_LVDS6:
			return KDRV_SSENIF_CAP_CSILVDS6;
		case KDRV_SSENIF_ENGINE_CSI7:
		case KDRV_SSENIF_ENGINE_LVDS7:
			return KDRV_SSENIF_CAP_CSILVDS7;
		default:
			CTL_SEN_DBG_ERR("id %d \r\n", id);
		}
	} else { // wait linux top driver define CHIP_NA51084
		switch (drvdev) {
		case KDRV_SSENIF_ENGINE_CSI0:
		case KDRV_SSENIF_ENGINE_LVDS0:
			return KDRV_SSENIF_CAP_CSILVDS0_528;
		case KDRV_SSENIF_ENGINE_CSI1:
		case KDRV_SSENIF_ENGINE_LVDS1:
			return KDRV_SSENIF_CAP_CSILVDS1_528;
		case KDRV_SSENIF_ENGINE_CSI2:
		case KDRV_SSENIF_ENGINE_LVDS2:
			return KDRV_SSENIF_CAP_CSILVDS2;
		case KDRV_SSENIF_ENGINE_CSI3:
		case KDRV_SSENIF_ENGINE_LVDS3:
			return KDRV_SSENIF_CAP_CSILVDS3;
		case KDRV_SSENIF_ENGINE_CSI4:
		case KDRV_SSENIF_ENGINE_LVDS4:
			return KDRV_SSENIF_CAP_CSILVDS4;
		case KDRV_SSENIF_ENGINE_CSI5:
		case KDRV_SSENIF_ENGINE_LVDS5:
			return KDRV_SSENIF_CAP_CSILVDS5;
		case KDRV_SSENIF_ENGINE_CSI6:
		case KDRV_SSENIF_ENGINE_LVDS6:
			return KDRV_SSENIF_CAP_CSILVDS6;
		case KDRV_SSENIF_ENGINE_CSI7:
		case KDRV_SSENIF_ENGINE_LVDS7:
			return KDRV_SSENIF_CAP_CSILVDS7;
		default:
			CTL_SEN_DBG_ERR("id %d \r\n", id);
		}
	}

	return 0;
}

KDRV_SSENIF_CAP sen_get_ssenif_cap_sie(UINT32 sie_id)
{
	switch (sie_id) {
	case 0:
		return KDRV_SSENIF_CAP_SIE0;
	case 1:
		return KDRV_SSENIF_CAP_SIE1;
	case 2:
		return KDRV_SSENIF_CAP_SIE2;
	case 3:
		return KDRV_SSENIF_CAP_SIE3;
	case 4:
		return KDRV_SSENIF_CAP_SIE4;
	case 5:
		return KDRV_SSENIF_CAP_SIE5;
	case 6:
		return KDRV_SSENIF_CAP_SIE6;
	case 7:
		return KDRV_SSENIF_CAP_SIE7;
	default:
		CTL_SEN_DBG_ERR("\r\n");
	}
	return KDRV_SSENIF_CAP_SIE0;
}

KDRV_SSENIF_CAP sen_get_ssenif_cap_dl(UINT32 in)
{
	switch (in) {
	case 0:
		return KDRV_SSENIF_CAP_DL0;
	case 1:
		return KDRV_SSENIF_CAP_DL1;
	case 2:
		return KDRV_SSENIF_CAP_DL2;
	case 3:
		return KDRV_SSENIF_CAP_DL3;
	case 4:
		return KDRV_SSENIF_CAP_DL4;
	case 5:
		return KDRV_SSENIF_CAP_DL5;
	case 6:
		return KDRV_SSENIF_CAP_DL6;
	case 7:
		return KDRV_SSENIF_CAP_DL7;
	default:
		CTL_SEN_DBG_ERR("\r\n");
	}
	return KDRV_SSENIF_CAP_DL0;
}

KDRV_SSENIF_CAP sen_get_ssenif_cap_ck(UINT32 in)
{
	switch (in) {
	case 0:
		return KDRV_SSENIF_CAP_CK0;
	case 1:
		return KDRV_SSENIF_CAP_CK1;
	case 2:
		return KDRV_SSENIF_CAP_CK2;
	case 3:
		return KDRV_SSENIF_CAP_CK3;
	case 4:
		return KDRV_SSENIF_CAP_CK4;
	case 5:
		return KDRV_SSENIF_CAP_CK5;
	case 6:
		return KDRV_SSENIF_CAP_CK6;
	case 7:
		return KDRV_SSENIF_CAP_CK7;
	default:
		CTL_SEN_DBG_ERR("\r\n");
	}
	return KDRV_SSENIF_CAP_CK0;
}
/*
    kdrv_ssenif globle ctrl - clk
*/
void sen_uti_set_clken_cb(CTL_SEN_ID id, CTL_SEN_CLK_CB cb)
{
	sen_clken_cb[id] = cb;
}

CTL_SEN_CLK_CB sen_uti_get_clken_cb(CTL_SEN_ID id)
{
	if (sen_clken_cb[id] == NULL) {
		CTL_SEN_DBG_ERR("id %d NULL\r\n", id);
	}
	return sen_clken_cb[id];
}

void sen_uti_set_mclk_src(CTL_SEN_CLK_SEL mclksel, CTL_SEN_CLKSRC_SEL clksel)
{
	sen_mclk_src[mclksel] = clksel;
}

CTL_SEN_CLKSRC_SEL sen_uti_get_mclk_src(CTL_SEN_CLK_SEL mclksel)
{
	return sen_mclk_src[mclksel];
}

void sen_uti_set_mclksel_freq(CTL_SEN_CLK_SEL mclksel, UINT32 freq)
{
	sen_mclk_freq[mclksel] = freq;
}

UINT32 sen_uti_get_mclksel_freq(CTL_SEN_CLK_SEL mclksel)
{
	return sen_mclk_freq[mclksel];
}

UINT32 sen_uti_get_mclk_freq(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	INT32 rt;
	CTL_SENDRV_GET_SPEED_PARAM speed_param = {0};
	UINT32 mclk_freq = 0;

	speed_param.mode = mode;
	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_SPEED, (void *)&speed_param);
	if (rt == CTL_SEN_E_OK) {
		mclk_freq = speed_param.mclk;
	} else {
		CTL_SEN_DBG_ERR("id %d get sendrv CTL_SEN_CFGID_GET_SPEED error %d\r\n", id, rt);
	}
	return mclk_freq;
}

/*
    kdrv_tge
*/
KDRV_TGE_CLK_SRC sen_ctrl_clksel_cov2_tge(CTL_SEN_CLK_SEL clc_sel)
{
	switch (clc_sel) {
	case CTL_SEN_CLK_SEL_SIEMCLK:
		return KDRV_TGE_CLK_MCLK1;
	case CTL_SEN_CLK_SEL_SIEMCLK2:
		return KDRV_TGE_CLK_MCLK2;
	default:
		CTL_SEN_DBG_ERR("\r\n");
	}
	return KDRV_TGE_CLK_MCLK1;
}

/*
    common
*/
UINT32 sen_fps_fmt_conv2_real(UINT32 sen_fps_fmt)
{
#if 1
	return sen_fps_fmt;
#else   //x1000
	if ((sen_fps_fmt & 0xffff) == 0) {
		sen_fps_fmt |= 1;
	}
	return (((sen_fps_fmt & 0xffff0000) >> 16) * 1000) / (sen_fps_fmt & 0xffff);
#endif
}

/*
    common
*/
UINT32 sen_uint64_dividend(UINT64 dividend, UINT32 divisor)
{
#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
	return (UINT32)(dividend / divisor);
#else
	do_div(dividend, divisor);
	return (UINT32)(dividend);
#endif
}

/*
    get sendrv information
*/
CTL_SEN_CLK_SEL sen_conv_mclksel(CTL_SEN_ID id, CTL_SEN_SIEMCLK_SRC mclk_src)
{
	CTL_SEN_CLK_SEL clk_sel;

	/*
	    get clock select MCLK/MCLK2
	*/
	if (mclk_src == CTL_SEN_SIEMCLK_SRC_DFT) {
		switch (id) {
		case CTL_SEN_ID_1:
			clk_sel = CTL_SEN_CLK_SEL_SIEMCLK;
			break;
		case CTL_SEN_ID_2:
		case CTL_SEN_ID_3:
		case CTL_SEN_ID_4:
		case CTL_SEN_ID_5:
		case CTL_SEN_ID_6:
		case CTL_SEN_ID_7:
		case CTL_SEN_ID_8:
			clk_sel = CTL_SEN_CLK_SEL_SIEMCLK2;
			break;
		default:
			CTL_SEN_DBG_ERR("id overflow %d\r\n", id);
			clk_sel = CTL_SEN_CLK_SEL_SIEMCLK;
			break;
		}
	} else if (mclk_src == CTL_SEN_SIEMCLK_SRC_MCLK) {
		clk_sel = CTL_SEN_CLK_SEL_SIEMCLK;
	} else if (mclk_src == CTL_SEN_SIEMCLK_SRC_MCLK2) {
		clk_sel = CTL_SEN_CLK_SEL_SIEMCLK2;
	} else if (mclk_src == CTL_SEN_SIEMCLK_SRC_MCLK3) {
		clk_sel = CTL_SEN_CLK_SEL_SIEMCLK3;
	} else if (mclk_src == CTL_SEN_SIEMCLK_SRC_IGNORE) {
		clk_sel = CTL_SEN_CLK_SEL_SIEMCLK_IGNORE;
	} else {
		CTL_SEN_DBG_ERR("id %d mclk src error %d\r\n", id, mclk_src);
		clk_sel = CTL_SEN_CLK_SEL_SIEMCLK;
	}

	return clk_sel;
}

CTL_SEN_CLK_SEL sen_uti_get_mclksel(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	CTL_SEN_SIEMCLK_SRC mclk_src;
	CTL_SENDRV_GET_SPEED_PARAM drv_speed_param;
	ER rt;

	memset((void *)&drv_speed_param, 0, sizeof(CTL_SENDRV_GET_SPEED_PARAM));

	drv_speed_param.mode = mode;
	rt = sendrv_get(id, CTL_SENDRV_CFGID_GET_SPEED, &drv_speed_param);
	if (rt == E_OK) {
		mclk_src = drv_speed_param.mclk_src;
	} else {
		mclk_src = CTL_SEN_SIEMCLK_SRC_DFT;
		CTL_SEN_DBG_ERR("get sendrv CTL_SENDRV_CFGID_GET_SPEED fail\r\n");
	}

	return sen_conv_mclksel(id, mclk_src);
}

void sen_uti_set_sendrv_en_mipi(CTL_SEN_ID id, CTL_SEN_CSI_EN_CB csi_en_cb)
{
	ER rt;
	CTL_SENDRV_GET_MODE_MIPI_EN_USER mipi_en_user = {0};

	mipi_en_user.csi_en_cb = csi_en_cb;
	rt = sen_ctrl_drv_get_obj()->sendrv_set(id, CTL_SENDRV_CFGID_GET_MODE_MIPI_EN_USER, (void *)&mipi_en_user);
	if (rt == E_OK) {
		sendrv_en_mipi[id] = 1;
	}
}

BOOL sen_uti_get_sendrv_en_mipi(CTL_SEN_ID id)
{
	return sendrv_en_mipi[id];
}


BOOL sen_ctrl_chk_data_fmt_is_raw(CTL_SEN_DATA_FMT data_fmt)
{
	BOOL is_raw = TRUE;

	if ((data_fmt == CTL_SEN_DATA_FMT_YUV) || (data_fmt == CTL_SEN_DATA_FMT_Y_ONLY)) {
		is_raw = FALSE;
	}
	return is_raw;
}

BOOL sen_ctrl_chk_mode_type_is_multiframe(CTL_SEN_MODE_TYPE mode_type)
{
	BOOL is_multiframe = FALSE;

	if ((mode_type == CTL_SEN_MODE_STAGGER_HDR) || (mode_type == CTL_SEN_MODE_PDAF)) {
		is_multiframe = TRUE;
	}
	return is_multiframe;
}

BOOL sen_ctrl_chk_add_ctx(CTL_SEN_ID id)
{
	if (id < CTL_SEN_ID_MAX_SUPPORT) {
		if ((b_add_ctx[id]) && ((g_ctl_sen_init_obj[id] == NULL) || (g_ctl_sen_ctrl_obj[id] == NULL))) {
			CTL_SEN_DBG_ERR("id %d be init, but obj is NULL (0x%.8x 0x%.8x)\r\n", (int)id, (int)g_ctl_sen_init_obj[id], (int)g_ctl_sen_ctrl_obj[id]);
			return FALSE;
		}
		return b_add_ctx[id];
	} else {
		CTL_SEN_DBG_ERR("id %d overflow\r\n", id);
		return FALSE;
	}
}

INT32 ctl_sen_util_os_malloc(CTL_SEN_VOS_MEM_INFO *vod_mem_info, UINT32 req_size)
{
	if (vos_mem_init_cma_info(&vod_mem_info->cma_info, VOS_MEM_CMA_TYPE_CACHE, req_size) != 0) {
		CTL_SEN_DBG_ERR("init cma failed\r\n");
		return CTL_SEN_E_NOMEM;
	}

	vod_mem_info->cma_hdl = vos_mem_alloc_from_cma(&vod_mem_info->cma_info);
	if (vod_mem_info->cma_hdl == NULL) {
		CTL_SEN_DBG_ERR("alloc cma failed\r\n");
		return CTL_SEN_E_NOMEM;
	}

	return CTL_SEN_E_OK;
}

INT32 ctl_sen_util_os_mfree(CTL_SEN_VOS_MEM_INFO *vod_mem_info)
{
	if (vos_mem_release_from_cma(vod_mem_info->cma_hdl) != 0) {
		CTL_SEN_DBG_ERR("release cma failed\r\n");
		return CTL_SEN_E_NOMEM;
	}
	return CTL_SEN_E_OK;
}

