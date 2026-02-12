/**
    @file       kdrv_sie.c
    @ingroup	Predefined_group_name

    @brief      sie device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kdrv_sie_int.h"
#include "kdrv_sie_debug_int.h"
#include "kdrv_sie_config.h"
#include "kwrap/file.h"
#include <kdrv_builtin/kdrv_builtin.h>

#define _INLINE static inline

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

//support function list and limitation table
static UINT32 kdrv_sie_spt_func_98515[KDRV_SIE_MAX_ENG_NT98515] = {
	KDRV_SIE_FUNC_SPT_DIRECT|KDRV_SIE_FUNC_SPT_PATGEN|KDRV_SIE_FUNC_SPT_DVI|KDRV_SIE_FUNC_SPT_OB_AVG|KDRV_SIE_FUNC_SPT_OB_BYPASS|KDRV_SIE_FUNC_SPT_CGAIN|KDRV_SIE_FUNC_SPT_DPC|KDRV_SIE_FUNC_SPT_ECS|KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V|KDRV_SIE_FUNC_SPT_RAWENC|KDRV_SIE_FUNC_SPT_LA|KDRV_SIE_FUNC_SPT_LA_HISTO|KDRV_SIE_FUNC_SPT_CA|KDRV_SIE_FUNC_SPT_RGBIR_FMT_SEL|KDRV_SIE_FUNC_SPT_RGGB_FMT_SEL|KDRV_SIE_FUNC_SPT_COMPANDING|KDRV_SIE_FUNC_SPT_SINGLE_OUT,
	KDRV_SIE_FUNC_SPT_PATGEN|KDRV_SIE_FUNC_SPT_DVI|KDRV_SIE_FUNC_SPT_OB_AVG|KDRV_SIE_FUNC_SPT_OB_BYPASS|KDRV_SIE_FUNC_SPT_CGAIN|KDRV_SIE_FUNC_SPT_DPC|KDRV_SIE_FUNC_SPT_ECS|KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V|KDRV_SIE_FUNC_SPT_RAWENC|KDRV_SIE_FUNC_SPT_LA|KDRV_SIE_FUNC_SPT_LA_HISTO|KDRV_SIE_FUNC_SPT_CA|KDRV_SIE_FUNC_SPT_RGBIR_FMT_SEL|KDRV_SIE_FUNC_SPT_RGGB_FMT_SEL|KDRV_SIE_FUNC_SPT_COMPANDING|KDRV_SIE_FUNC_SPT_SINGLE_OUT|KDRV_SIE_FUNC_SPT_RING_BUF,
	KDRV_SIE_FUNC_SPT_PATGEN|KDRV_SIE_FUNC_SPT_DVI|KDRV_SIE_FUNC_SPT_CGAIN|KDRV_SIE_FUNC_SPT_SINGLE_OUT|KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V,
};

KDRV_SIE_LIMIT kdrv_sie_limit_98515[KDRV_SIE_MAX_ENG_NT98515] = {
	//max_clk_rate		max_mclk_rate		max id		   max_out_ch		ca_win_num	la_win_num	y_win_num	la_hist	pat_gen	active	crop	scl_out	ca_crop	la_crop	out_ch_lofs		spt_func	ring_buf_len
	{SIE_MAX_CLK_FREQ,	SIE_MAX_MCLK_FREQ,	KDRV_SIE_ID_3, KDRV_SIE_CH2, 	{32,32}, 	{32,32}, 	{0,0}, 		128, 	{2,1}, 	{4,1}, 	{4,1}, 	{4,2}, 	{4,1},	{2,1}, 	{4, 4, 4}, 		0,			0},
	{SIE_MAX_CLK_FREQ,	SIE_MAX_MCLK_FREQ,	KDRV_SIE_ID_3, KDRV_SIE_CH2, 	{32,32}, 	{32,32}, 	{0,0}, 		128, 	{2,1}, 	{2,1}, 	{2,1}, 	{2,2}, 	{2,1},	{1,1}, 	{4, 4, 4}, 		0,			1023},
	{SIE_MAX_CLK_FREQ,	SIE_MAX_MCLK_FREQ,	KDRV_SIE_ID_3, KDRV_SIE_CH1, 	{32,32}, 	{32,32}, 	{0,0}, 		128, 	{2,1}, 	{2,1}, 	{2,1}, 	{2,2}, 	{2,1},	{1,1}, 	{4, 4, 4}, 		0,			0},
};

static UINT32 kdrv_sie_spt_func_98528[KDRV_SIE_MAX_ENG_NT98528] = {
	KDRV_SIE_FUNC_SPT_DIRECT|KDRV_SIE_FUNC_SPT_PATGEN|KDRV_SIE_FUNC_SPT_DVI|KDRV_SIE_FUNC_SPT_OB_AVG|KDRV_SIE_FUNC_SPT_OB_BYPASS|KDRV_SIE_FUNC_SPT_CGAIN|KDRV_SIE_FUNC_SPT_DGAIN|KDRV_SIE_FUNC_SPT_DPC|KDRV_SIE_FUNC_SPT_ECS|KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V|KDRV_SIE_FUNC_SPT_RAWENC|KDRV_SIE_FUNC_SPT_LA|KDRV_SIE_FUNC_SPT_LA_HISTO|KDRV_SIE_FUNC_SPT_CA|KDRV_SIE_FUNC_SPT_RGBIR_FMT_SEL|KDRV_SIE_FUNC_SPT_RGGB_FMT_SEL|KDRV_SIE_FUNC_SPT_COMPANDING|KDRV_SIE_FUNC_SPT_SINGLE_OUT|KDRV_SIE_FUNC_SPT_MD,
	KDRV_SIE_FUNC_SPT_PATGEN|KDRV_SIE_FUNC_SPT_DVI|KDRV_SIE_FUNC_SPT_OB_AVG|KDRV_SIE_FUNC_SPT_OB_BYPASS|KDRV_SIE_FUNC_SPT_CGAIN|KDRV_SIE_FUNC_SPT_DGAIN|KDRV_SIE_FUNC_SPT_DPC|KDRV_SIE_FUNC_SPT_ECS|KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V|KDRV_SIE_FUNC_SPT_RAWENC|KDRV_SIE_FUNC_SPT_LA|KDRV_SIE_FUNC_SPT_LA_HISTO|KDRV_SIE_FUNC_SPT_CA|KDRV_SIE_FUNC_SPT_RGBIR_FMT_SEL|KDRV_SIE_FUNC_SPT_RGGB_FMT_SEL|KDRV_SIE_FUNC_SPT_COMPANDING|KDRV_SIE_FUNC_SPT_SINGLE_OUT|KDRV_SIE_FUNC_SPT_RING_BUF|KDRV_SIE_FUNC_SPT_MD,
	KDRV_SIE_FUNC_SPT_PATGEN|KDRV_SIE_FUNC_SPT_DVI|KDRV_SIE_FUNC_SPT_CGAIN|KDRV_SIE_FUNC_SPT_SINGLE_OUT|KDRV_SIE_FUNC_SPT_MD|KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V,
	KDRV_SIE_FUNC_SPT_PATGEN|KDRV_SIE_FUNC_SPT_DVI|KDRV_SIE_FUNC_SPT_OB_AVG|KDRV_SIE_FUNC_SPT_OB_BYPASS|KDRV_SIE_FUNC_SPT_CGAIN|KDRV_SIE_FUNC_SPT_DGAIN|KDRV_SIE_FUNC_SPT_DPC|KDRV_SIE_FUNC_SPT_ECS|KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V|KDRV_SIE_FUNC_SPT_RAWENC|KDRV_SIE_FUNC_SPT_LA|KDRV_SIE_FUNC_SPT_LA_HISTO|KDRV_SIE_FUNC_SPT_CA|KDRV_SIE_FUNC_SPT_RGBIR_FMT_SEL|KDRV_SIE_FUNC_SPT_RGGB_FMT_SEL|KDRV_SIE_FUNC_SPT_COMPANDING|KDRV_SIE_FUNC_SPT_SINGLE_OUT|KDRV_SIE_FUNC_SPT_MD,
	KDRV_SIE_FUNC_SPT_PATGEN|KDRV_SIE_FUNC_SPT_DVI|KDRV_SIE_FUNC_SPT_OB_AVG|KDRV_SIE_FUNC_SPT_OB_BYPASS|KDRV_SIE_FUNC_SPT_CGAIN|KDRV_SIE_FUNC_SPT_DGAIN|KDRV_SIE_FUNC_SPT_DPC|KDRV_SIE_FUNC_SPT_ECS|KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V|KDRV_SIE_FUNC_SPT_RAWENC|KDRV_SIE_FUNC_SPT_LA|KDRV_SIE_FUNC_SPT_LA_HISTO|KDRV_SIE_FUNC_SPT_CA|KDRV_SIE_FUNC_SPT_RGBIR_FMT_SEL|KDRV_SIE_FUNC_SPT_RGGB_FMT_SEL|KDRV_SIE_FUNC_SPT_COMPANDING|KDRV_SIE_FUNC_SPT_SINGLE_OUT|KDRV_SIE_FUNC_SPT_MD,
};

KDRV_SIE_LIMIT kdrv_sie_limit_98528[KDRV_SIE_MAX_ENG_NT98528] = {
	//max_clk_rate 		max_mclk_rate		max id		   max_out_ch		ca_win_num	la_win_num	y_win_num	la_hist	pat_gen	active	crop	scl_out	ca_crop	la_crop	out_ch_lofs		spt_func	ring_buf_len
	{SIE_MAX_CLK_FREQ,	SIE_MAX_MCLK_FREQ,	KDRV_SIE_ID_5, KDRV_SIE_CH2, 	{32,32}, 	{32,32}, 	{0,0}, 		128, 	{2,1}, 	{4,1}, 	{4,1}, 	{4,2}, 	{4,1},	{2,1}, 	{4, 4, 4}, 		0,			0},
	{SIE_MAX_CLK_FREQ,	SIE_MAX_MCLK_FREQ,	KDRV_SIE_ID_5, KDRV_SIE_CH2, 	{32,32}, 	{32,32}, 	{0,0}, 		128, 	{2,1}, 	{2,1}, 	{2,1}, 	{2,2}, 	{2,1},	{1,1}, 	{4, 4, 4}, 		0,			2047},
	{SIE_MAX_CLK_FREQ,	SIE_MAX_MCLK_FREQ,	KDRV_SIE_ID_5, KDRV_SIE_CH1, 	{32,32}, 	{32,32}, 	{0,0}, 		128, 	{2,1}, 	{2,1}, 	{2,1}, 	{2,2}, 	{2,1},	{1,1}, 	{4, 4, 4}, 		0,			0},
	{SIE_MAX_CLK_FREQ,	SIE_MAX_MCLK_FREQ,	KDRV_SIE_ID_5, KDRV_SIE_CH2, 	{32,32}, 	{32,32}, 	{0,0}, 		128, 	{2,1}, 	{4,1}, 	{4,1}, 	{4,2}, 	{4,1},	{2,1}, 	{4, 4, 4}, 		0,			0},
	{SIE_MAX_CLK_FREQ,	SIE_MAX_MCLK_FREQ,	KDRV_SIE_ID_5, KDRV_SIE_CH2, 	{32,32}, 	{32,32}, 	{0,0}, 		128, 	{2,1}, 	{4,1}, 	{4,1}, 	{4,2}, 	{4,1},	{2,1}, 	{4, 4, 4}, 		0,			0},
};

KDRV_SIE_LIMIT kdrv_sie_com_limit[KDRV_SIE_MAX_ENG] = {0};
static BOOL kdrv_sie_init_flg = FALSE;
static KDRV_SIE_HDL_CONTEXT kdrv_sie_hdl_ctx = {0};
static UINT8 kdrv_sie_load_flg[KDRV_SIE_MAX_ENG][KDRV_SIE_PARAM_MAX] = {0};
static KDRV_SIE_STATUS kdrv_sie_status[KDRV_SIE_MAX_ENG] = {0};
static KDRV_SIE_CLK_HDL kdrv_sie_clk_hdl[KDRV_SIE_MAX_ENG] = {0};
static KDRV_SIE_FB_CLK_INFO kdrv_sie_fb_clk_info[KDRV_SIE_MAX_ENG] = {0};
static KDRV_SIE_INFO *kdrv_sie_info[KDRV_SIE_MAX_ENG]= {NULL};

int fastboot = 0;
static BOOL kdrv_sie_fb_normal_stream[KDRV_SIE_MAX_ENG] = {FALSE};

#if 0
#endif

/**
	kdrv internel used
*/

KDRV_SIE_HDL_CONTEXT *kdrv_sie_get_ctx(void)
{
	if (kdrv_sie_init_flg) {
		return &kdrv_sie_hdl_ctx;
	} else {
		kdrv_sie_dbg_err("kdrv sie init flag is FALSE\r\n");
		return NULL;
	}
}

KDRV_SIE_CLK_HDL *kdrv_sie_get_clk_hdl(KDRV_SIE_PROC_ID id)
{
	if (!kdrv_sie_chk_id_valid(id)) {
		return NULL;
	}

	return &kdrv_sie_clk_hdl[id];
}

BOOL kdrv_sie_chk_id_valid(KDRV_SIE_PROC_ID id)
{
	if(id > kdrv_sie_com_limit[id].max_spt_id) {
		kdrv_sie_dbg_err("illegal id %d > maximum id %d\r\n", (int)id, (int)(kdrv_sie_com_limit[id].max_spt_id));
		return FALSE;
	}
	return TRUE;
}

void kdrv_sie_init(void)
{
	UINT32 i;

	#if defined __FREERTOS
	sie_platform_create_resource();
	#endif
	kdrv_sie_install_id();

	for (i = 0; i < KDRV_SIE_MAX_ENG; i++) {
		kdrv_sie_get_sie_limit(i, (void *)&kdrv_sie_com_limit[i]);
	}
}

void kdrv_sie_uninit(void)
{
	sie_platform_release_resource();
	memset((void *)&kdrv_sie_com_limit[0], 0, sizeof(kdrv_sie_com_limit));
}

KDRV_SIE_STATUS kdrv_sie_get_state_machine(KDRV_SIE_PROC_ID id)
{
	unsigned long flags;
	KDRV_SIE_STATUS status;

	if (!kdrv_sie_chk_id_valid(id)) {
		return KDRV_SIE_STS_MAX;
	}

	loc_cpu(flags);
	status = kdrv_sie_status[id];
	unl_cpu(flags);

	return status;
}

KDRV_SIE_INFO *kdrv_sie_get_hdl(KDRV_SIE_PROC_ID id)
{
	unsigned long flags;
	KDRV_SIE_INFO *kdrv_sie_hdl;

	if (!kdrv_sie_chk_id_valid(id)) {
		return NULL;
	}

	loc_cpu(flags);
	kdrv_sie_hdl = kdrv_sie_info[id];
	unl_cpu(flags);
	return kdrv_sie_hdl;
}

BOOL kdrv_sie_typecast_ccir(KDRV_SIE_DATA_FMT src)
{
	if (src == KDRV_SIE_YUV_422_SPT ||
		src == KDRV_SIE_YUV_422_NOSPT ||
		src == KDRV_SIE_YUV_420_SPT) {
		return TRUE;
	}
	return FALSE;
}

BOOL kdrv_sie_typecast_raw(KDRV_SIE_DATA_FMT src)
{
	if (src == KDRV_SIE_BAYER_8 ||
		src == KDRV_SIE_BAYER_10 ||
		src == KDRV_SIE_BAYER_12 ||
		src == KDRV_SIE_BAYER_16) {
		return TRUE;
	}
	return FALSE;
}

void kdrv_sie_chk_act_crp_start_y(KDRV_SIE_PROC_ID id)
{
	if (kdrv_sie_info[id]->out_dest == KDRV_SIE_OUT_DEST_DIRECT || kdrv_sie_info[id]->out_dest == KDRV_SIE_OUT_DEST_BOTH) {
		if ((kdrv_sie_info[id]->act_window.win.y + kdrv_sie_info[id]->crp_window.win.y) == 0) {
			kdrv_sie_dbg_err("N.S. direct mode (active_start y + crop_start_y) = 0\r\n");
		}
	}
}

SIE_ENGINE_ID kdrv_sie_conv2_sie_id(KDRV_SIE_PROC_ID id)
{
	switch (id) {
	case KDRV_SIE_ID_1:
		return SIE_ENGINE_ID_1;

	case KDRV_SIE_ID_2:
		return SIE_ENGINE_ID_2;

	case KDRV_SIE_ID_3:
		return SIE_ENGINE_ID_3;

	case KDRV_SIE_ID_4:
		return SIE_ENGINE_ID_4;

	case KDRV_SIE_ID_5:
		return SIE_ENGINE_ID_5;

	case KDRV_SIE_ID_6:
		return SIE_ENGINE_ID_6;

	case KDRV_SIE_ID_7:
		return SIE_ENGINE_ID_7;

	case KDRV_SIE_ID_8:
		return SIE_ENGINE_ID_8;

	default:
		kdrv_sie_dbg_err("kdrv id overflow\r\n");
		return SIE_ENGINE_ID_MAX;
	}
}

#if 0
#endif
/**
	static functions

*/

static SIE_CLKSRC_SEL __kdrv_sie_typecast_clk_src(KDRV_SIE_CLKSRC_SEL kdrv_clk_src)
{
	switch (kdrv_clk_src) {
	case KDRV_SIE_CLKSRC_CURR:
		return SIE_CLKSRC_CURR;

	case KDRV_SIE_CLKSRC_480:
		return SIE_CLKSRC_480;

	case KDRV_SIE_CLKSRC_PLL5:
		return SIE_CLKSRC_PLL5;

	case KDRV_SIE_CLKSRC_PLL13:
		return SIE_CLKSRC_PLL13;

	case KDRV_SIE_CLKSRC_PLL12:
		return SIE_CLKSRC_PLL12;

	case KDRV_SIE_CLKSRC_320:
		return SIE_CLKSRC_320;

	case KDRV_SIE_CLKSRC_192:
		return SIE_CLKSRC_192;

	case KDRV_SIE_CLKSRC_PLL10:
		return SIE_CLKSRC_PLL10;

	default:
		kdrv_sie_dbg_err("Unknown clk_src_sel %d, force to KDRV_SIE_CLKSRC_480\r\n", (int)kdrv_clk_src);
		return SIE_CLKSRC_480;
	}
}

static SIE_PXCLKSRC __kdrv_sie_typecast_pxclk_src(KDRV_SIE_PXCLKSRC_SEL kdrv_pxclk_src)
{
	switch (kdrv_pxclk_src) {
		case KDRV_SIE_PXCLKSRC_OFF:
			return SIE_PXCLKSRC_OFF;

		case KDRV_SIE_PXCLKSRC_PAD:
			return SIE_PXCLKSRC_PAD;

		case KDRV_SIE_PXCLKSRC_PAD_AHD:
			return SIE_PXCLKSRC_PAD_AHD;

		case KDRV_SIE_PXCLKSRC_MCLK:
			return SIE_PXCLKSRC_MCLK;

		case KDRV_SIE_PXCLKSRC_VX1_1X:
			return SIE_PXCLKSRC_VX1_1X;

		case KDRV_SIE_PXCLKSRC_VX1_2X:
			return SIE_PXCLKSRC_VX1_2X;

		default:
			kdrv_sie_dbg_err("Unknown pclk_src_sel %d, force to KDRV_SIE_PXCLKSRC_PAD\r\n", (int)kdrv_pxclk_src);
			return SIE_PXCLKSRC_PAD;
	}
}

_INLINE SIE_DVI_OUT_SWAP_SEL __kdrv_sie_typecast_yuv_order(KDRV_SIE_YUV_ORDER src, KDRV_SIE_FLIP flip, KDRV_SIE_DVI_IN_MODE_SEL dvi_mode)
{
	SIE_DVI_OUT_SWAP_SEL rst = DVI_OUT_SWAP_YUYV;
	SIE_DVI_OUT_SWAP_SEL rst_swap = DVI_OUT_SWAP_YUYV;

	switch (src) {
	case KDRV_SIE_YUYV:
		if (flip & KDRV_SIE_FLIP_H) {
			rst = DVI_OUT_SWAP_YVYU;
		} else {
			rst = DVI_OUT_SWAP_YUYV;
		}
		break;

	case KDRV_SIE_YVYU:
		if (flip & KDRV_SIE_FLIP_H) {
			rst = DVI_OUT_SWAP_YUYV;
		} else {
			rst = DVI_OUT_SWAP_YVYU;
		}
		break;

	case KDRV_SIE_UYVY:
		if (flip & KDRV_SIE_FLIP_H) {
			rst = DVI_OUT_SWAP_VYUY;
		} else {
			rst = DVI_OUT_SWAP_UYVY;
		}
		break;

	case KDRV_SIE_VYUY:
		if (flip & KDRV_SIE_FLIP_H) {
			rst = DVI_OUT_SWAP_UYVY;
		} else {
			rst = DVI_OUT_SWAP_VYUY;
		}
		break;

	default:
		rst = DVI_OUT_SWAP_YUYV;
		kdrv_sie_dbg_err("yuv order overflow %d, force set to DVI_OUT_SWAP_YUYV\r\n", (int)src);
		break;
	}

	switch (rst) {
	case DVI_OUT_SWAP_YUYV:
		if (dvi_mode == KDRV_DVI_MODE_HD_INV) {
			rst_swap = DVI_OUT_SWAP_UYVY;
		} else {
			rst_swap = DVI_OUT_SWAP_YUYV;
		}
		break;

	case DVI_OUT_SWAP_YVYU:
		if (dvi_mode == KDRV_DVI_MODE_HD_INV) {
			rst_swap = DVI_OUT_SWAP_VYUY;
		} else {
			rst_swap = DVI_OUT_SWAP_YVYU;
		}
		break;

	case DVI_OUT_SWAP_UYVY:
		if (dvi_mode == KDRV_DVI_MODE_HD_INV) {
			rst_swap = DVI_OUT_SWAP_YUYV;
		} else {
			rst_swap = DVI_OUT_SWAP_UYVY;
		}
		break;

	default:
		if (dvi_mode == KDRV_DVI_MODE_HD_INV) {
			rst_swap = DVI_OUT_SWAP_YVYU;
		} else {
			rst_swap = DVI_OUT_SWAP_VYUY;
		}
		break;
	}

	return rst_swap;
}

_INLINE SIE_CFAPAT_SEL __kdrv_sie_typecast_raw_cfa(KDRV_SIE_PIX src, UPOINT shift)
{
	if ((shift.x % 2) == 1) {
		if (src == KDRV_SIE_RGGB_PIX_R) {
			src = KDRV_SIE_RGGB_PIX_GR;
		} else if (src == KDRV_SIE_RGGB_PIX_GR) {
			src = KDRV_SIE_RGGB_PIX_R;
		} else if (src == KDRV_SIE_RGGB_PIX_GB) {
			src = KDRV_SIE_RGGB_PIX_B;
		} else {//if (src == KDRV_SIE_RGGB_PIX_B) {
			src = KDRV_SIE_RGGB_PIX_GB;
		}
	}

	if ((shift.y % 2) == 1) {
		if (src == KDRV_SIE_RGGB_PIX_R) {
			src = KDRV_SIE_RGGB_PIX_GB;
		} else if (src == KDRV_SIE_RGGB_PIX_GR) {
			src = KDRV_SIE_RGGB_PIX_B;
		} else if (src == KDRV_SIE_RGGB_PIX_GB) {
			src = KDRV_SIE_RGGB_PIX_R;
		} else {//if (src == KDRV_SIE_RGGB_PIX_B) {
			src = KDRV_SIE_RGGB_PIX_GR;
		}
	}

	switch (src) {
	case KDRV_SIE_RGGB_PIX_R:
		return CFA_R;

	case KDRV_SIE_RGGB_PIX_GR:
		return CFA_Gr;

	case KDRV_SIE_RGGB_PIX_GB:
		return CFA_Gb;

	case KDRV_SIE_RGGB_PIX_B:
		return CFA_B;

	default:
		return CFA_R;
	}
}

_INLINE SIE_RGBIR_CFAPAT_SEL __kdrv_sie_typecast_rgbir_cfa(KDRV_SIE_PIX src, UPOINT shift)
{
	UINT32 cfa8;
	UINT32 icfa = src - KDRV_SIE_RGBIR_PIX_RG_GI;

	#define SIEFC(row,col, CFASEL) ((((col) & 1) ^ (CFASEL & 1)) + ((((row) & 1) ^ ((CFASEL >> 1) & 1)) << 1))
	#define SIEFC8(row, col, CFASEL) ((((((col + (CFASEL & 1)) & 3) & 2) >> 1) ^ ((((row + ((CFASEL & 3) >> 1)) & 3) & 2) >> 1)) ^ ((CFASEL >> 2) & 1))

	cfa8 = SIEFC(shift.y, shift.x, icfa) + (SIEFC8(shift.y, shift.x, icfa) << 2);
	return cfa8;
}

_INLINE SIE_PACKBUS_SEL __kdrv_sie_typecast_packbus(KDRV_SIE_DATA_FMT src)
{
	switch (src) {
	case KDRV_SIE_BAYER_8:
		return SIE_PACKBUS_8;

	case KDRV_SIE_BAYER_10:
		return SIE_PACKBUS_10;

	case KDRV_SIE_BAYER_12:
		return SIE_PACKBUS_12;

	case KDRV_SIE_BAYER_16:
		return SIE_PACKBUS_16;

	case KDRV_SIE_YUV_422_SPT:
	case KDRV_SIE_YUV_422_NOSPT:
	case KDRV_SIE_YUV_420_SPT:
		return SIE_PACKBUS_8;

	default:
		kdrv_sie_dbg_err("pcakbus overflow %d, force set to SIE_PACKBUS_8\r\n", (int)src);
		return SIE_PACKBUS_8;
	}
}

_INLINE BOOL __kdrv_sie_chk_align(UINT32 src, UINT32 align)
{
	return (ALIGN_CEIL(src, align) == src);
}

static INT32 __kdrv_sie_calc_ir_level(KDRV_SIE_PROC_ID id, SIE_STCS_CA_RSLT_INFO *ca_rst, SIE_CA_WIN_INFO ca_win_info, SIE_CA_IRSUB_INFO ca_ir_sub_info)
{
	sie_calc_ir_level(id, ca_rst, ca_win_info, ca_ir_sub_info);
	return KDRV_SIE_E_OK;
}

_INLINE INT32 __kdrv_sie_load_main_in(KDRV_SIE_PROC_ID id)
{
	SIE_MAIN_INPUT_INFO main_in = {0};

	switch (kdrv_sie_info[id]->open_cfg.act_mode) {
	case KDRV_SIE_IN_PARA_MSTR_SNR:
		main_in.MainInSrc = MAIN_IN_PARA_MSTR_SNR;
		break;

	case KDRV_SIE_IN_PARA_SLAV_SNR:
		main_in.MainInSrc = MAIN_IN_PARA_SLAV_SNR;
		break;

	case KDRV_SIE_IN_PATGEN:
		main_in.MainInSrc = MAIN_IN_SELF_PATGEN;
		break;

	case KDRV_SIE_IN_VX1_IF0_SNR:
		main_in.MainInSrc = MAIN_IN_VX1_SNR;
		break;

	case KDRV_SIE_IN_CSI_1:
		main_in.MainInSrc = MAIN_IN_CSI_1;
		break;

	case KDRV_SIE_IN_CSI_2:
		main_in.MainInSrc = MAIN_IN_CSI_2;
		break;

	case KDRV_SIE_IN_CSI_3:
		main_in.MainInSrc = MAIN_IN_CSI_3;
		break;

	case KDRV_SIE_IN_CSI_4:
		main_in.MainInSrc = MAIN_IN_CSI_4;
		break;

	case KDRV_SIE_IN_CSI_5:
		main_in.MainInSrc = MAIN_IN_CSI_5;
		break;

	case KDRV_SIE_IN_CSI_6:
		main_in.MainInSrc = MAIN_IN_CSI_6;
		break;

	case KDRV_SIE_IN_CSI_7:
		main_in.MainInSrc = MAIN_IN_CSI_7;
		break;

	case KDRV_SIE_IN_CSI_8:
		main_in.MainInSrc = MAIN_IN_CSI_8;
		break;

	case KDRV_SIE_IN_AHD:
		main_in.MainInSrc = MAIN_IN_AHD;
		break;

	case KDRV_SIE_IN_SLVS_EC:
		main_in.MainInSrc = MAIN_IN_SLVS_EC;
		break;

	case KDRV_SIE_IN_VX1_IF1_SNR:
		main_in.MainInSrc = MAIN_IN_VX1_IF1_SNR;
		break;

	default:
		break;
	}
	main_in.VdPhase = kdrv_sie_info[id]->signal.vd_phase;
	main_in.HdPhase = kdrv_sie_info[id]->signal.hd_phase;
	main_in.DataPhase = kdrv_sie_info[id]->signal.data_phase;
	main_in.bVdInv = kdrv_sie_info[id]->signal.vd_inverse;
	main_in.bHdInv = kdrv_sie_info[id]->signal.hd_inverse;
	if ((kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DIRECT) && (kdrv_sie_info[id]->out_dest == KDRV_SIE_OUT_DEST_DIRECT || kdrv_sie_info[id]->out_dest == KDRV_SIE_OUT_DEST_BOTH)) {
		main_in.bDirect2RHE = TRUE;
	} else {
		main_in.bDirect2RHE = FALSE;
	}

	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&main_in, SIE_CHG_MAIN_IN);
	return KDRV_SIE_E_OK;
}

/**
	sie isr cb
*/
_INLINE void __sie_common_isr(KDRV_SIE_PROC_ID id, UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	if (kdrv_sie_info[id] != NULL) {
		if (status & KDRV_SIE_INT_CROPEND) {
			kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_RDY] = kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_CUR];
			kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_RDY] = kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_CUR];
		}

		if (status & KDRV_SIE_INT_VD) {
			kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_CUR] = kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_SET];
			kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_CUR] = kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_SET];
			if (info == NULL) {
				kdrv_sie_dbg_err("id %d info NULL\r\n", id);
			} else {
				kdrv_sie_upd_vd_intrpt_sts(id, info);
			}
		}

		if (status & KDRV_SIE_ERR_INT_EN) {
			kdrv_sie_upd_intrpt_err_sts(id, status);
		}

		// callback
		if ((kdrv_sie_info[id]->isrcb_fp != NULL) && (kdrv_sie_status[id] != KDRV_SIE_STS_CLOSE)) {
			kdrv_sie_info[id]->isrcb_fp(id, status, NULL, NULL);
		}
	}
}

static void __sie1_kdrv_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	__sie_common_isr(KDRV_SIE_ID_1, status, info);
}

static void __sie2_kdrv_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	__sie_common_isr(KDRV_SIE_ID_2, status, info);
}

static void __sie3_kdrv_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	__sie_common_isr(KDRV_SIE_ID_3, status, info);
}

#if (KDRV_SIE_MAX_ENG == KDRV_SIE_MAX_ENG_NT98528)
static void __sie4_kdrv_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	__sie_common_isr(KDRV_SIE_ID_4, status, info);
}

static void __sie5_kdrv_isr(UINT32 status, SIE_ENGINE_STATUS_INFO_CB *info)
{
	__sie_common_isr(KDRV_SIE_ID_5, status, info);
}
#endif

static void __kdrv_sie_sem(KDRV_SIE_PROC_ID id, BOOL flag)
{
	ID *sem_id = kdrv_sie_get_sem_id(id);

    if (sem_id == NULL) {
		kdrv_sie_dbg_err("id = %d, get sem id NULL\r\n", (int)id);
    } else {
		if (flag) {
			vos_sem_wait(*sem_id);
		} else {
			vos_sem_sig(*sem_id);
		}
    }
}

static INT32 __kdrv_sie_state_machine(UINT32 id, KDRV_SIE_OP op)
{
	INT32 rt = KDRV_SIE_E_OK;
	KDRV_SIE_STATUS cur_st = kdrv_sie_status[id];
	unsigned long flags;

	loc_cpu(flags);
	switch (op) {
	case KDRV_SIE_OP_OPEN:
		if (cur_st == KDRV_SIE_STS_CLOSE) {
			kdrv_sie_status[id] = KDRV_SIE_STS_READY;
		} else {
			rt = KDRV_SIE_E_STATE;
		}
		break;

	case KDRV_SIE_OP_CLOSE:
		if (cur_st == KDRV_SIE_STS_READY) {
			kdrv_sie_status[id] = KDRV_SIE_STS_CLOSE;
		} else {
			rt = KDRV_SIE_E_STATE;
		}
		break;

	case KDRV_SIE_OP_TRIG_START:
		if (cur_st == KDRV_SIE_STS_READY) {
			kdrv_sie_status[id] = KDRV_SIE_STS_RUN;
		} else {
			rt = KDRV_SIE_E_STATE;
		}
		break;

	case KDRV_SIE_OP_TRIG_STOP:
		if (cur_st == KDRV_SIE_STS_RUN) {
			kdrv_sie_status[id] = KDRV_SIE_STS_READY;
		} else {
			rt = KDRV_SIE_E_STATE;
		}
		break;

	case KDRV_SIE_OP_SET:
	case KDRV_SIE_OP_GET:
		if (cur_st == KDRV_SIE_STS_CLOSE) {
			rt = KDRV_SIE_E_STATE;
		}
		break;

	default:
		rt = KDRV_SIE_E_STATE;
		break;
	}
	unl_cpu(flags);
	if (rt != KDRV_SIE_E_OK) {
		kdrv_sie_dbg_err("id %d, err op %d at status %d\r\n", (int)(id), (int)(op), (int)(cur_st));
	}
	return rt;
}

#if 0
#endif

/** sie kdrv set functions **/
static INT32 kdrv_sie_set_opencfg(KDRV_SIE_PROC_ID id, void *data)
{
	UINT32 i;
	unsigned long flags;

	if (!kdrv_sie_init_flg) {
		kdrv_sie_dbg_err("kdrv sie init flag is FALSE\r\n");
		return KDRV_SIE_E_STATE;
	}

	if (data == NULL) {
		kdrv_sie_dbg_err("NULL data\r\n");
		return KDRV_SIE_E_PAR;
	}

	loc_cpu(flags);
	if (kdrv_sie_hdl_ctx.ctx_used >= kdrv_sie_hdl_ctx.ctx_num) {
		kdrv_sie_dbg_err("overflow sie number, cur %d, max %d\r\n", kdrv_sie_hdl_ctx.ctx_used, kdrv_sie_hdl_ctx.ctx_num);
		unl_cpu(flags);
		return KDRV_SIE_E_HDL;
	} else {
		for (i = 0; i <= kdrv_sie_com_limit[id].max_spt_id; i++) {
			if (kdrv_sie_hdl_ctx.ctx_idx[i] == KDRV_SIE_ID_MAX_NUM) {
				kdrv_sie_hdl_ctx.ctx_idx[i] = id;
				break;
			}
		}

		kdrv_sie_info[id] = (KDRV_SIE_INFO *)(kdrv_sie_hdl_ctx.start_addr + (i * kdrv_sie_hdl_ctx.ctx_size));
		memset((void *)kdrv_sie_info[id], 0, sizeof(KDRV_SIE_INFO));
		kdrv_sie_hdl_ctx.ctx_used++;
		unl_cpu(flags);
	}

   	kdrv_sie_info[id]->open_cfg = *(KDRV_SIE_OPENCFG *)data;
	kdrv_sie_dbg_ind("id = %d, act_mode = %d, pxclkSel/clksel/clkrate = %d/%d/%d\r\n", (int)id, (int)kdrv_sie_info[id]->open_cfg.act_mode, (int)kdrv_sie_info[id]->open_cfg.pclk_src_sel, (int)kdrv_sie_info[id]->open_cfg.clk_src_sel, (int)kdrv_sie_info[id]->open_cfg.data_rate);
	return KDRV_SIE_E_OK;
}
static INT32 kdrv_sie_set_isrcb(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_info[id]->isrcb_fp = (KDRV_SIE_ISRCB)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_mclk(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_clk_hdl[id].mclk_info = *(KDRV_SIE_MCLK_INFO *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_pxclk(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_clk_hdl[id].pxclk_info = *(KDRV_SIE_PXCLKSRC_SEL *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_sieclk(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_clk_hdl[id].clk_info = *(KDRV_SIE_CLK_INFO *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_act_win(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_ACT_CRP_WIN *act_win = (KDRV_SIE_ACT_CRP_WIN *)data;

	// check align
	if (__kdrv_sie_chk_align(act_win->win.w, kdrv_sie_com_limit[id].act_win_align.w) == FALSE) {
		kdrv_sie_dbg_err("id = %d Active window width(%d) not %dx\r\n", (int)id, (int)act_win->win.w, (int)kdrv_sie_com_limit[id].act_win_align.w);
		return KDRV_SIE_E_PAR;
	}

	if (__kdrv_sie_chk_align(act_win->win.h, kdrv_sie_com_limit[id].act_win_align.h) == FALSE) {
		kdrv_sie_dbg_err("id = %d Active window height(%d) not %dx\r\n", (int)id, (int)act_win->win.h, (int)kdrv_sie_com_limit[id].act_win_align.h);
		return KDRV_SIE_E_PAR;
	}
	kdrv_sie_info[id]->act_window = *act_win;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_crp_win(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_ACT_CRP_WIN *crp_win = (KDRV_SIE_ACT_CRP_WIN *)data;

	// check align
	if (__kdrv_sie_chk_align(crp_win->win.w, kdrv_sie_com_limit[id].crp_win_align.w) == FALSE) {
		kdrv_sie_dbg_err("id = %d Crop window width(%d) not 2x\r\n", (int)id, (int)crp_win->win.w);
		return KDRV_SIE_E_PAR;
	}

	if (__kdrv_sie_chk_align(crp_win->win.h, kdrv_sie_com_limit[id].crp_win_align.h) == FALSE) {
		kdrv_sie_dbg_err("id = %d Crop window height(%d) not 2x\r\n", (int)id, (int)crp_win->win.h);
		return KDRV_SIE_E_PAR;
	}
	kdrv_sie_info[id]->crp_window = *crp_win;
	// update scale size = crop size when not support sie scale function
	if (!(kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_BS_H)) {
		kdrv_sie_info[id]->scl_size.w = crp_win->win.w;
	}

	if (!(kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_BS_V)) {
		kdrv_sie_info[id]->scl_size.h = crp_win->win.h;
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_out_dest(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_DEST out_dest = *(KDRV_SIE_OUT_DEST *)data;

	if (out_dest == KDRV_SIE_OUT_DEST_DIRECT || out_dest == KDRV_SIE_OUT_DEST_BOTH) {
		if (!(kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DIRECT)) {
			kdrv_sie_dbg_err("id = %d N.S. out_dest %d\r\n", (int)id, (int)out_dest);
			return KDRV_SIE_E_NOSPT;
		}
	}
	kdrv_sie_info[id]->out_dest = out_dest;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_data_format(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_info[id]->data_fmt = *(KDRV_SIE_DATA_FMT *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_yuv_order(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DVI) {
		kdrv_sie_info[id]->ccir_info.yuv_order = *(KDRV_SIE_YUV_ORDER *)data;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. DVI\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_set_signal(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_info[id]->signal = *(KDRV_SIE_SIGNAL *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_flip(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_FLIP flip = *(KDRV_SIE_FLIP *)data;

	if ((flip == KDRV_SIE_FLIP_NONE) ||	((flip == KDRV_SIE_FLIP_H) && (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_FLIP_H)) ||
	   ((flip == KDRV_SIE_FLIP_V) && (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_FLIP_V)) ||
	   ((flip == KDRV_SIE_FLIP_H_V) && (kdrv_sie_com_limit[id].support_func & (KDRV_SIE_FUNC_SPT_FLIP_H|KDRV_SIE_FUNC_SPT_FLIP_V)))) {
		kdrv_sie_info[id]->flip = flip;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. flip function %d\r\n", (int)id, (int)flip);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_set_inte(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_info[id]->inte = (*(KDRV_SIE_INT *)data | KDRV_SIE_DFT_INT);
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_out_lof(KDRV_SIE_PROC_ID id, KDRV_SIE_OUT_LOFS *lofs_info)
{
	if (lofs_info->ch_id > kdrv_sie_com_limit[id].out_max_ch) {
		kdrv_sie_dbg_wrn("id = %d output channel %d overflow (max ch %d)\r\n", (int)id, (int)lofs_info->ch_id, (int)kdrv_sie_com_limit[id].out_max_ch);
		return KDRV_SIE_E_PAR;
	}

	if (__kdrv_sie_chk_align(lofs_info->lofs, kdrv_sie_com_limit[id].out_lofs_align[lofs_info->ch_id])) {
		kdrv_sie_info[id]->out_ch_lof[lofs_info->ch_id] = lofs_info->lofs;
	} else {
		kdrv_sie_dbg_err("id = %d output lineoffset %d is not %dx align\r\n", (int)id, (int)lofs_info->lofs, (int)kdrv_sie_com_limit[id].out_lofs_align[lofs_info->ch_id]);
		return KDRV_SIE_E_PAR;
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_ch0_out_lof(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_LOFS lofs_info;

	lofs_info.ch_id = KDRV_SIE_CH0;
	lofs_info.lofs = *(UINT32 *)data;
	return kdrv_sie_set_out_lof(id, &lofs_info);
}

static INT32 kdrv_sie_set_ch1_out_lof(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_LOFS lofs_info;

	lofs_info.ch_id = KDRV_SIE_CH1;
	lofs_info.lofs = *(UINT32 *)data;
	return kdrv_sie_set_out_lof(id, &lofs_info);
}

static INT32 kdrv_sie_set_ch2_out_lof(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_LOFS lofs_info;

	lofs_info.ch_id = KDRV_SIE_CH2;
	lofs_info.lofs = *(UINT32 *)data;
	return kdrv_sie_set_out_lof(id, &lofs_info);
}

static INT32 kdrv_sie_set_out_addr(KDRV_SIE_PROC_ID id, KDRV_SIE_OUT_ADDR *addr_info)
{
	if (addr_info->addr == 0) {
		kdrv_sie_dbg_err("id = %d channel %d output address 0\r\n", (int)id, (int)addr_info->ch_id);
		return KDRV_SIE_E_PAR;
	}

	if (addr_info->ch_id > kdrv_sie_com_limit[id].out_max_ch) {
		kdrv_sie_dbg_wrn("id = %d output channel %d overflow (max ch %d)\r\n", (int)id, (int)addr_info->ch_id, (int)kdrv_sie_com_limit[id].out_max_ch);
		return KDRV_SIE_E_NOSPT;
	}

	if (__kdrv_sie_chk_align(addr_info->addr, 4) == FALSE) {
		kdrv_sie_dbg_err("id = %d output address(0x%.8x) is not 4byte align\r\n", (int)id, (unsigned int)addr_info->addr);
		return KDRV_SIE_E_PAR;
	}

	kdrv_sie_info[id]->out_ch_addr[addr_info->ch_id] = addr_info->addr;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_ch0_out_addr(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_ADDR addr_info;

	addr_info.ch_id = KDRV_SIE_CH0;
	addr_info.addr = *(UINT32 *)data;
	return kdrv_sie_set_out_addr(id, &addr_info);
}

static INT32 kdrv_sie_set_ch1_out_addr(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_ADDR addr_info;

	addr_info.ch_id = KDRV_SIE_CH1;
	addr_info.addr = *(UINT32 *)data;
	return kdrv_sie_set_out_addr(id, &addr_info);
}

static INT32 kdrv_sie_set_ch2_out_addr(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_ADDR addr_info;

	addr_info.ch_id = KDRV_SIE_CH2;
	addr_info.addr = *(UINT32 *)data;
	return kdrv_sie_set_out_addr(id, &addr_info);
}

static INT32 kdrv_sie_set_pat_gen_info(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_PATGEN) {
		kdrv_sie_info[id]->pat_gen_info = *(KDRV_SIE_PATGEN_INFO *)data;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_err("id = %d, N.S. pattern gen!!\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_set_ob(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OB_PARAM *ob_par = (KDRV_SIE_OB_PARAM *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_OB_BYPASS) {
		kdrv_sie_info[id]->ob_param.ob_ofs = ob_par->ob_ofs;
		kdrv_sie_info[id]->ob_param.bypass_enable = ob_par->bypass_enable;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d, N.S. OB!!\r\n", id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_set_ca_roi(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_CA_ROI *ca_roi = (KDRV_SIE_CA_ROI *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_CA) {
		if (ca_roi->roi.w != 0 && ca_roi->roi.h != 0 && ca_roi->roi_base != 0 && (ca_roi->roi.x + ca_roi->roi.w <= ca_roi->roi_base) && (ca_roi->roi.y + ca_roi->roi.h <= ca_roi->roi_base)) {
			kdrv_sie_info[id]->ca_roi.roi_base = ca_roi->roi_base;
			kdrv_sie_info[id]->ca_roi.roi.x = ALIGN_FLOOR_4((ca_roi->roi.x * kdrv_sie_info[id]->crp_window.win.w) / kdrv_sie_info[id]->ca_roi.roi_base);
			kdrv_sie_info[id]->ca_roi.roi.y = ALIGN_FLOOR_4((ca_roi->roi.y * kdrv_sie_info[id]->crp_window.win.h) / kdrv_sie_info[id]->ca_roi.roi_base);
			kdrv_sie_info[id]->ca_roi.roi.w = ALIGN_FLOOR((ca_roi->roi.w * kdrv_sie_info[id]->crp_window.win.w) / kdrv_sie_info[id]->ca_roi.roi_base, kdrv_sie_com_limit[id].ca_crp_win_align.w);
			kdrv_sie_info[id]->ca_roi.roi.h = ALIGN_FLOOR((ca_roi->roi.h * kdrv_sie_info[id]->crp_window.win.h) / kdrv_sie_info[id]->ca_roi.roi_base, kdrv_sie_com_limit[id].ca_crp_win_align.h);

			if (kdrv_sie_info[id]->ca_roi.roi.w > KDRV_SIE_CA_MAX_CROP_W) {
				kdrv_sie_dbg_wrn("id = %d, ca crop width %d overflow, max is %d \r\n", (int)id, (int)kdrv_sie_info[id]->ca_roi.roi.w, (int)KDRV_SIE_CA_MAX_CROP_W);
				kdrv_sie_info[id]->ca_roi.roi.x = ALIGN_FLOOR_4(kdrv_sie_info[id]->ca_roi.roi.x + (kdrv_sie_info[id]->ca_roi.roi.w - KDRV_SIE_CA_MAX_CROP_W) / 2);
				kdrv_sie_info[id]->ca_roi.roi.w = ALIGN_FLOOR(KDRV_SIE_CA_MAX_CROP_W, kdrv_sie_com_limit[id].ca_crp_win_align.h);
			}
			if ((kdrv_sie_info[id]->ca_roi.roi.x + kdrv_sie_info[id]->ca_roi.roi.w) > kdrv_sie_info[id]->crp_window.win.w) {
				kdrv_sie_dbg_wrn("id = %d, ca crop_x %d + crop_w %d over than scl width %d \r\n", (int)id, (int)kdrv_sie_info[id]->ca_roi.roi.x, (int)kdrv_sie_info[id]->ca_roi.roi.w, (int)kdrv_sie_info[id]->crp_window.win.w);
				kdrv_sie_info[id]->ca_roi.roi.w = kdrv_sie_info[id]->crp_window.win.w - kdrv_sie_info[id]->ca_roi.roi.x;
			}

			if (kdrv_sie_info[id]->ca_roi.roi.h > KDRV_SIE_CA_MAX_CROP_H) {
				kdrv_sie_dbg_wrn("id = %d, ca crop height %d overflow, max is %d \r\n", (int)id, (int)kdrv_sie_info[id]->ca_roi.roi.h, (int)KDRV_SIE_CA_MAX_CROP_H);
				kdrv_sie_info[id]->ca_roi.roi.y = ALIGN_FLOOR_4(kdrv_sie_info[id]->ca_roi.roi.y + (kdrv_sie_info[id]->ca_roi.roi.h - KDRV_SIE_CA_MAX_CROP_H) / 2);
				kdrv_sie_info[id]->ca_roi.roi.h = ALIGN_FLOOR(KDRV_SIE_CA_MAX_CROP_H, kdrv_sie_com_limit[id].ca_crp_win_align.h);
			}
			if ((kdrv_sie_info[id]->ca_roi.roi.y + kdrv_sie_info[id]->ca_roi.roi.h) > kdrv_sie_info[id]->crp_window.win.h) {
				kdrv_sie_dbg_wrn("id = %d, ca crop_y %d + crop_h %d over than scl height %d \r\n", (int)id, (int)kdrv_sie_info[id]->ca_roi.roi.y, (int)kdrv_sie_info[id]->ca_roi.roi.h, (int)kdrv_sie_info[id]->crp_window.win.h);
				kdrv_sie_info[id]->ca_roi.roi.h = kdrv_sie_info[id]->crp_window.win.h - kdrv_sie_info[id]->ca_roi.roi.y;
			}
			return KDRV_SIE_E_OK;
		} else {
			kdrv_sie_dbg_err("id = %d, CA_ROI error start (%d, %d), size (%d, %d), ROI_BASE: %d!!\r\n",(int)id, (int)ca_roi->roi.x, (int)ca_roi->roi.y, (int)ca_roi->roi.w, (int)ca_roi->roi.h, (int)ca_roi->roi_base);
			return KDRV_SIE_E_PAR;
		}
	} else {
		kdrv_sie_dbg_wrn("id = %d, N.S. CA!!\r\n", id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_set_ca(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_CA_PARAM *ca_set_info = (KDRV_SIE_CA_PARAM *)data;
	KDRV_SIE_CA_ROI ca_roi = {1000, {0,0,1000,1000}};

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_CA) {
		if (ca_set_info->enable) {
			if (kdrv_sie_info[id]->ca_roi.roi_base == 0) {
				kdrv_sie_dbg_wrn("ca roi zero, force set to full\r\n");
				kdrv_sie_set_ca_roi(id, (void *)&ca_roi);
			}

			if (ca_set_info->win_num.w > kdrv_sie_com_limit[id].ca_win_max_num.w) {// check boundary
				kdrv_sie_dbg_wrn("CA Window number %d overflow, clamp to %d\r\n", (int)ca_set_info->win_num.w, (int)kdrv_sie_com_limit[id].ca_win_max_num.w);
				ca_set_info->win_num.w = kdrv_sie_com_limit[id].ca_win_max_num.w;
			}
			if (ca_set_info->win_num.h > kdrv_sie_com_limit[id].ca_win_max_num.h) {// check boundary
				kdrv_sie_dbg_wrn("CA Window number %d overflow, clamp to %d\r\n", (int)ca_set_info->win_num.h, (int)kdrv_sie_com_limit[id].ca_win_max_num.h);
				ca_set_info->win_num.h = kdrv_sie_com_limit[id].ca_win_max_num.h;
			}

			kdrv_sie_info[id]->ca_param.win_num = ca_set_info->win_num;
			kdrv_sie_info[id]->ca_param.th_enable = ca_set_info->th_enable;
			if (kdrv_sie_info[id]->ca_param.th_enable) {
				kdrv_sie_info[id]->ca_param.g_th_l = ca_set_info->g_th_l;
				kdrv_sie_info[id]->ca_param.g_th_u = ca_set_info->g_th_u;
				kdrv_sie_info[id]->ca_param.r_th_l = ca_set_info->r_th_l;
				kdrv_sie_info[id]->ca_param.r_th_u = ca_set_info->r_th_u;
				kdrv_sie_info[id]->ca_param.b_th_l = ca_set_info->b_th_l;
				kdrv_sie_info[id]->ca_param.b_th_u = ca_set_info->b_th_u;
				kdrv_sie_info[id]->ca_param.p_th_l = ca_set_info->p_th_l;
				kdrv_sie_info[id]->ca_param.p_th_u = ca_set_info->p_th_u;
			}
			kdrv_sie_info[id]->ca_param.irsub_r_weight = ca_set_info->irsub_r_weight;
			kdrv_sie_info[id]->ca_param.irsub_g_weight = ca_set_info->irsub_g_weight;
			kdrv_sie_info[id]->ca_param.irsub_b_weight = ca_set_info->irsub_b_weight;
			kdrv_sie_info[id]->ca_param.ca_ob_ofs = ca_set_info->ca_ob_ofs;
			kdrv_sie_info[id]->ca_param.ca_src = ca_set_info->ca_src;
		}
		kdrv_sie_info[id]->ca_param.enable = ca_set_info->enable;
	} else {
		if (ca_set_info->enable) {
			kdrv_sie_dbg_wrn("id = %d, N.S. CA!!\r\n", (int)id);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_la_roi(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_LA_ROI *la_roi = (KDRV_SIE_LA_ROI *)data;
	USIZE la_max_crp_sz = {KDRV_SIE_LA_MAX_CROP_W, KDRV_SIE_LA_MAX_CROP_H};

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA) {
		if (la_roi->roi.w != 0 && la_roi->roi.h != 0 && la_roi->roi_base != 0 && (la_roi->roi.x + la_roi->roi.w <= la_roi->roi_base) && (la_roi->roi.y + la_roi->roi.h <= la_roi->roi_base)) {
			if (kdrv_sie_info[id]->ca_roi.roi.w != 0 && kdrv_sie_info[id]->ca_roi.roi.w < (KDRV_SIE_LA_MAX_CROP_W * 2)) {
				la_max_crp_sz.w = kdrv_sie_info[id]->ca_roi.roi.w / 2;
			}

			if (kdrv_sie_info[id]->ca_roi.roi.h != 0 && kdrv_sie_info[id]->ca_roi.roi.h < (KDRV_SIE_LA_MAX_CROP_H * 2)) {
				la_max_crp_sz.h = kdrv_sie_info[id]->ca_roi.roi.h / 2;
			}

			kdrv_sie_info[id]->la_roi.roi_base = la_roi->roi_base;
			kdrv_sie_info[id]->la_roi.roi.x = ALIGN_FLOOR_4((la_roi->roi.x * la_max_crp_sz.w) / kdrv_sie_info[id]->la_roi.roi_base);
			kdrv_sie_info[id]->la_roi.roi.y = ALIGN_FLOOR_4((la_roi->roi.y * la_max_crp_sz.h) / kdrv_sie_info[id]->la_roi.roi_base);
			kdrv_sie_info[id]->la_roi.roi.w = ALIGN_FLOOR((la_roi->roi.w * la_max_crp_sz.w) / kdrv_sie_info[id]->la_roi.roi_base, kdrv_sie_com_limit[id].la_crp_win_align.w);
			kdrv_sie_info[id]->la_roi.roi.h = ALIGN_FLOOR((la_roi->roi.h * la_max_crp_sz.h) / kdrv_sie_info[id]->la_roi.roi_base, kdrv_sie_com_limit[id].la_crp_win_align.h);

			if ((kdrv_sie_info[id]->la_roi.roi.x + kdrv_sie_info[id]->la_roi.roi.w) > la_max_crp_sz.w) {
				kdrv_sie_dbg_wrn("id = %d, la crop_x %d + crop_w %d over than la crop max width %d \r\n", (int)id, (int)kdrv_sie_info[id]->la_roi.roi.x, (int)kdrv_sie_info[id]->la_roi.roi.w, (int)la_max_crp_sz.w);
				kdrv_sie_info[id]->la_roi.roi.w = la_max_crp_sz.w - kdrv_sie_info[id]->la_roi.roi.x;
			}

			if ((kdrv_sie_info[id]->la_roi.roi.y + kdrv_sie_info[id]->la_roi.roi.h) > la_max_crp_sz.h) {
				kdrv_sie_dbg_wrn("id = %d, la crop_y %d + crop_h %d over than la crop max height %d \r\n", (int)id, (int)kdrv_sie_info[id]->la_roi.roi.y, (int)kdrv_sie_info[id]->la_roi.roi.h, (int)la_max_crp_sz.h);
				kdrv_sie_info[id]->la_roi.roi.h = la_max_crp_sz.h - kdrv_sie_info[id]->la_roi.roi.y;
			}
		} else {
			kdrv_sie_dbg_err("id = %d, LA_ROI error start (%d, %d), size (%d, %d), ROI_BASE: %d!!\r\n",(int)id, (int)la_roi->roi.x, (int)la_roi->roi.y, (int)la_roi->roi.w, (int)la_roi->roi.h, (int)la_roi->roi_base);
			return KDRV_SIE_E_PAR;
		}
	} else {
		kdrv_sie_dbg_wrn("id = %d, N.S. LA_ROI!!\r\n",(int)id);
		return KDRV_SIE_E_NOSPT;
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_la(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_LA_PARAM *la_set_info = (KDRV_SIE_LA_PARAM *)data;
	KDRV_SIE_CA_ROI la_roi = {1000, {0,0,1000,1000}};

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA) {
		if (la_set_info->enable) {
			if (kdrv_sie_info[id]->la_roi.roi_base == 0) {
				kdrv_sie_dbg_wrn("la roi zero, force set to full\r\n");
				kdrv_sie_set_la_roi(id, (void *)&la_roi);
			}
			if (la_set_info->win_num.w > kdrv_sie_com_limit[id].la_win_max_num.w) {// check boundary
				kdrv_sie_dbg_wrn("LA Window number %d overflow, clamp to %d\r\n", (int)la_set_info->win_num.w, (int)kdrv_sie_com_limit[id].la_win_max_num.w);
				la_set_info->win_num.w = kdrv_sie_com_limit[id].la_win_max_num.w;
			}
			if (la_set_info->win_num.h > kdrv_sie_com_limit[id].la_win_max_num.h) {// check boundary
				kdrv_sie_dbg_wrn("LA Window number %d overflow, clamp to %d\r\n", (int)la_set_info->win_num.h, (int)kdrv_sie_com_limit[id].la_win_max_num.h);
				la_set_info->win_num.h = kdrv_sie_com_limit[id].la_win_max_num.h;
			}

			kdrv_sie_info[id]->la_param.win_num = la_set_info->win_num;
			kdrv_sie_info[id]->la_param.la_src = la_set_info->la_src;
			kdrv_sie_info[id]->la_param.la_rgb2y1mod = la_set_info->la_rgb2y1mod;
			kdrv_sie_info[id]->la_param.la_rgb2y2mod = la_set_info->la_rgb2y2mod;

			kdrv_sie_info[id]->la_param.cg_enable = la_set_info->cg_enable;
			if (kdrv_sie_info[id]->la_param.cg_enable) {
				kdrv_sie_info[id]->la_param.r_gain = la_set_info->r_gain;
				kdrv_sie_info[id]->la_param.g_gain = la_set_info->g_gain;
				kdrv_sie_info[id]->la_param.b_gain = la_set_info->b_gain;
			}

			kdrv_sie_info[id]->la_param.histogram_enable = la_set_info->histogram_enable;
			if (la_set_info->histogram_enable) {
				if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA_HISTO) {
					kdrv_sie_info[id]->la_param.histogram_enable = la_set_info->histogram_enable;
					kdrv_sie_info[id]->la_param.histogram_src = la_set_info->histogram_src;
				} else {
					kdrv_sie_dbg_wrn("id = %d, N.S. LA HISTO!!\r\n",(int)id);
					kdrv_sie_info[id]->la_param.histogram_enable = DISABLE;
				}
			}

			kdrv_sie_info[id]->la_param.gamma_enable = la_set_info->gamma_enable;
			if (kdrv_sie_info[id]->la_param.gamma_enable) {
				kdrv_sie_info[id]->la_param.gamma_tbl_addr = la_set_info->gamma_tbl_addr;
			}

			kdrv_sie_info[id]->la_param.irsub_r_weight = la_set_info->irsub_r_weight;
			kdrv_sie_info[id]->la_param.irsub_g_weight = la_set_info->irsub_g_weight;
			kdrv_sie_info[id]->la_param.irsub_b_weight = la_set_info->irsub_b_weight;

			kdrv_sie_info[id]->la_param.la_ob_ofs = la_set_info->la_ob_ofs;
		}
		kdrv_sie_info[id]->la_param.enable = la_set_info->enable;
	} else {
		if (la_set_info->enable) {
			kdrv_sie_dbg_wrn("id = %d, N.S. LA!!\r\n", (int)id);
			kdrv_sie_info[id]->la_param.enable = DISABLE;
			kdrv_sie_info[id]->la_param.histogram_enable = DISABLE;
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_scl_size(KDRV_SIE_PROC_ID id, void *data)
{
	USIZE *scl_size = (USIZE *)data;
	INT32 rt = KDRV_SIE_E_OK;

	if (kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
		if (kdrv_sie_info[id]->crp_window.win.w != scl_size->w) {
			if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_BS_H) {
				kdrv_sie_info[id]->scl_size.w = scl_size->w;
			} else {
		    	kdrv_sie_dbg_err("id = %d N.S. H Raw-Scale!!, check crp_sz (%d,%d) != scl_size (%d,%d)\r\n", (int)id, (int)kdrv_sie_info[id]->crp_window.win.w, (int)kdrv_sie_info[id]->crp_window.win.h, (int)scl_size->w, (int)scl_size->h);
				kdrv_sie_info[id]->scl_size.w = kdrv_sie_info[id]->crp_window.win.w;
				rt = KDRV_SIE_E_NOSPT;
			}
		} else {
			kdrv_sie_info[id]->scl_size.w = scl_size->w;
		}

		if (kdrv_sie_info[id]->crp_window.win.h != scl_size->h) {
			if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_BS_V) {
				kdrv_sie_info[id]->scl_size.h = scl_size->h;
			} else {
		    	kdrv_sie_dbg_err("id = %d N.S. V Raw-Scale!!, check crp_sz (%d,%d) != scl_size (%d,%d)\r\n", (int)id, (int)kdrv_sie_info[id]->crp_window.win.w, (int)kdrv_sie_info[id]->crp_window.win.h, (int)scl_size->w, (int)scl_size->h);
				kdrv_sie_info[id]->scl_size.h = kdrv_sie_info[id]->crp_window.win.h;
				rt = KDRV_SIE_E_NOSPT;
			}
		} else {
			kdrv_sie_info[id]->scl_size.h = scl_size->h;
		}
	} else {
		kdrv_sie_info[id]->scl_size.w = scl_size->w;
		kdrv_sie_info[id]->scl_size.h = scl_size->h;
	}

	return rt;
}

static INT32 kdrv_sie_set_encode(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_RAW_ENCODE *enc_data = (KDRV_SIE_RAW_ENCODE*)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_RAWENC) {
		if (enc_data->enable) {
			kdrv_sie_info[id]->encode_info.enc_rate = enc_data->enc_rate;
		}
		kdrv_sie_info[id]->encode_info.enable = enc_data->enable;
	} else {
		if (enc_data->enable) {
			kdrv_sie_dbg_wrn("id = %d N.S. raw compress!!\r\n", (int)id);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_dgain(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_DGAIN *d_gain = (KDRV_SIE_DGAIN *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DGAIN) {
		if (d_gain->enable) {
			kdrv_sie_info[id]->dgain.gain = d_gain->gain;
		}
		kdrv_sie_info[id]->dgain.enable = d_gain->enable;
	} else {
		if (d_gain->enable) {
			kdrv_sie_dbg_wrn("id = %d N.S. Dgain!!\r\n", (int)id);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_cgain(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_CGAIN *c_gain = (KDRV_SIE_CGAIN *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_CGAIN) {
		if (c_gain->enable) {
			kdrv_sie_info[id]->cgain.sel_37_fmt = c_gain->sel_37_fmt;
			kdrv_sie_info[id]->cgain.r_gain = c_gain->r_gain;
			kdrv_sie_info[id]->cgain.gr_gain = c_gain->gr_gain;
			kdrv_sie_info[id]->cgain.gb_gain = c_gain->gb_gain;
			kdrv_sie_info[id]->cgain.b_gain = c_gain->b_gain;
			kdrv_sie_info[id]->cgain.ir_gain = c_gain->ir_gain;
		}
		kdrv_sie_info[id]->cgain.enable = c_gain->enable;
	} else {
		if (c_gain->enable) {
			kdrv_sie_dbg_wrn("id = %d N.S. cgain!!\r\n", (int)id);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_dpc(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_DPC *dpc = (KDRV_SIE_DPC *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DPC) {
		if (dpc->enable) {
			kdrv_sie_info[id]->dpc_info.tbl_addr = dpc->tbl_addr;
			kdrv_sie_info[id]->dpc_info.weight = dpc->weight;
			kdrv_sie_info[id]->dpc_info.dp_total_size = dpc->dp_total_size;
		}
		kdrv_sie_info[id]->dpc_info.enable = dpc->enable;
	} else {
		if (dpc->enable) {
			kdrv_sie_dbg_wrn("id = %d, N.S. dpc!!\r\n", (int)id);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_ecs(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_ECS *ecs = (KDRV_SIE_ECS *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_ECS) {
		if (ecs->enable) {
			kdrv_sie_info[id]->ecs_info.map_tbl_addr = ecs->map_tbl_addr;
			kdrv_sie_info[id]->ecs_info.map_sel = ecs->map_sel;
			kdrv_sie_info[id]->ecs_info.sel_37_fmt = ecs->sel_37_fmt;

			kdrv_sie_info[id]->ecs_info.dthr_enable = ecs->dthr_enable;
			kdrv_sie_info[id]->ecs_info.dthr_reset = ecs->dthr_reset;
			kdrv_sie_info[id]->ecs_info.bayer_mode = ecs->bayer_mode;
			if (ecs->dthr_level > KDRV_SIE_ECS_DITHER_MAX_LVL) {
				kdrv_sie_dbg_wrn("ecs dither level %d > %d, overflow\r\n", (int)ecs->dthr_level, (int)KDRV_SIE_ECS_DITHER_MAX_LVL);
				kdrv_sie_info[id]->ecs_info.dthr_level = KDRV_SIE_ECS_DITHER_MAX_LVL;
			} else {
				kdrv_sie_info[id]->ecs_info.dthr_level = ecs->dthr_level;
			}
		}
		kdrv_sie_info[id]->ecs_info.enable = ecs->enable;
	} else {
		if (ecs->enable) {
			kdrv_sie_dbg_wrn("id = %d, N.S. ecs!!\r\n", (int)id);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_ccir(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DVI) {
		kdrv_sie_info[id]->ccir_info = *(KDRV_SIE_CCIR_INFO *)data;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d, N.S. ccir!!\r\n",(int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_set_dma_out(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_info[id]->b_dma_out = *(BOOL *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_bp1(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_info[id]->bp_info.bp1 = *(UINT32 *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_bp2(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_info[id]->bp_info.bp2 = *(UINT32 *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_bp3(KDRV_SIE_PROC_ID id, void *data)
{
	kdrv_sie_info[id]->bp_info.bp3 = *(UINT32 *)data;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_companding(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_COMPANDING *comp_info = (KDRV_SIE_COMPANDING *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_COMPANDING) {
		if (comp_info->enable) {
			memcpy((void *)&kdrv_sie_info[id]->comp_info, (void *)comp_info, sizeof(KDRV_SIE_COMPANDING));
		}
		kdrv_sie_info[id]->comp_info.enable = comp_info->enable;
	} else {
		if (comp_info->enable) {
			kdrv_sie_dbg_wrn("id = %d, N.S. companding!!\r\n", (int)id);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_single_out(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_SINGLE_OUT_CTRL *sin_out_info = (KDRV_SIE_SINGLE_OUT_CTRL *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_SINGLE_OUT) {
		kdrv_sie_info[id]->singleout_info.ch0singleout_enable = sin_out_info->ch0singleout_enable;
		kdrv_sie_info[id]->singleout_info.ch1singleout_enable = sin_out_info->ch1singleout_enable;
		kdrv_sie_info[id]->singleout_info.ch2singleout_enable = sin_out_info->ch2singleout_enable;
	} else {
		if ((sin_out_info->ch0singleout_enable) || (sin_out_info->ch1singleout_enable) || (sin_out_info->ch2singleout_enable)) {
			kdrv_sie_dbg_wrn("id = %d, N.S. single out, ch(%d, %d, %d)!!\r\n", (int)id, (int)sin_out_info->ch0singleout_enable, (int)sin_out_info->ch1singleout_enable, (int)sin_out_info->ch2singleout_enable);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_out_mode(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_DRAM_OUT_CTRL *ch_out_mode = (KDRV_SIE_DRAM_OUT_CTRL *)data;

	if ((ch_out_mode->out0_mode == KDRV_SIE_SINGLE_OUT || ch_out_mode->out1_mode == KDRV_SIE_SINGLE_OUT || ch_out_mode->out2_mode == KDRV_SIE_SINGLE_OUT) && !(kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_SINGLE_OUT)) {
		kdrv_sie_dbg_wrn("id = %d, N.S. single out!!\r\n",(int)id);
		return KDRV_SIE_E_NOSPT;
	} else {
		kdrv_sie_info[id]->ch_output_mode = *(KDRV_SIE_DRAM_OUT_CTRL *)data;
		return KDRV_SIE_E_OK;
	}
}

static INT32 kdrv_sie_set_ring_buf(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_RINGBUF_INFO *ring_info = (KDRV_SIE_RINGBUF_INFO *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_SINGLE_OUT) {
		kdrv_sie_info[id]->ring_buf_info.enable = ring_info->enable;
		if (ring_info->enable) {
			kdrv_sie_info[id]->ring_buf_info.ring_buf_len = ring_info->ring_buf_len;
		}
	} else {
		if (ring_info->enable) {
			kdrv_sie_dbg_wrn("id = %d, N.S. ring buffer!!\r\n", (int)id);
			return KDRV_SIE_E_NOSPT;
		}
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_md(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_MD *md_info = (KDRV_SIE_MD *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_MD) {
		kdrv_sie_info[id]->md_info.enable = md_info->enable;
		if (md_info->enable) {
			kdrv_sie_info[id]->md_info = *(KDRV_SIE_MD *)data;
		}
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d, N.S. ring buffer!!\r\n",(int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

#if 0
#endif

INT32 kdrv_sie_set_load_act(KDRV_SIE_PROC_ID id)
{
	SIE_ACT_WIN_INFO act_win = {0};
	UPOINT shift = {0};

	act_win.uiStX = kdrv_sie_info[id]->act_window.win.x;
	act_win.uiStY = kdrv_sie_info[id]->act_window.win.y;
	act_win.uiSzX = kdrv_sie_info[id]->act_window.win.w;
	act_win.uiSzY = kdrv_sie_info[id]->act_window.win.h;
	if (act_win.uiStX == 0 && act_win.uiStY == 0 && act_win.uiSzX == 0 && act_win.uiSzY == 0) {
	} else {
		if (kdrv_sie_info[id]->act_window.cfa_pat >= KDRV_SIE_RGBIR_PIX_RG_GI && kdrv_sie_info[id]->act_window.cfa_pat <= KDRV_SIE_RGBIR_PIX_IG_GB)	{
			act_win.CfaPat = __kdrv_sie_typecast_rgbir_cfa(kdrv_sie_info[id]->act_window.cfa_pat, shift);
		} else {
			act_win.CfaPat = __kdrv_sie_typecast_raw_cfa(kdrv_sie_info[id]->act_window.cfa_pat, shift);
		}
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&act_win, SIE_CHG_ACT_WIN);
	}
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_crp(KDRV_SIE_PROC_ID id)
{
	SIE_CRP_WIN_INFO crp_win = {0};
	UPOINT shift = {kdrv_sie_info[id]->crp_window.win.x, kdrv_sie_info[id]->crp_window.win.y};
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	crp_win.uiStX = kdrv_sie_info[id]->crp_window.win.x;
	crp_win.uiStY = kdrv_sie_info[id]->crp_window.win.y;
	crp_win.uiSzX = kdrv_sie_info[id]->crp_window.win.w;
	crp_win.uiSzY = kdrv_sie_info[id]->crp_window.win.h;

	kdrv_sie_chk_act_crp_start_y(id);
	if (crp_win.uiStX == 0 && crp_win.uiStY == 0 && crp_win.uiSzX == 0 && crp_win.uiSzY == 0) {
	} else {
		if (kdrv_sie_info[id]->crp_window.cfa_pat >= KDRV_SIE_RGBIR_PIX_RG_GI && kdrv_sie_info[id]->crp_window.cfa_pat <= KDRV_SIE_RGBIR_PIX_IG_GB)	{
			crp_win.CfaPat = __kdrv_sie_typecast_rgbir_cfa(kdrv_sie_info[id]->crp_window.cfa_pat, shift);
			*func_en |= SIE_RGBIR_FORMAT_SEL;
		} else {
			crp_win.CfaPat = __kdrv_sie_typecast_raw_cfa(kdrv_sie_info[id]->crp_window.cfa_pat, shift);
			*func_en &= (~SIE_RGBIR_FORMAT_SEL);
		}
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&crp_win, SIE_CHG_CROP_WIN);
	}
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_module_set_load_bs_h(KDRV_SIE_PROC_ID id)
{
	SIE_BS_H_INFO bs_h = {0};
	SIE_BS_H_ADJ_INFO bs_h_adj;
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	//calc scaling factor
	bs_h.uiOutSize = kdrv_sie_info[id]->scl_size.w;
	if (kdrv_sie_info[id]->crp_window.win.w == bs_h.uiOutSize) {
		*func_en &= (~SIE_BS_H_EN);
	} else {
		*func_en |= SIE_BS_H_EN;
		bs_h_adj.uiInSz = kdrv_sie_info[id]->crp_window.win.w;
		bs_h_adj.uiOutSz = bs_h.uiOutSize;
		bs_h_adj.uiLpf = RAW_SCL_LPF;
		bs_h_adj.uiBinPwr = RAW_SCL_BIN_PWR;
		bs_h_adj.bAdaptiveLpf = RAW_SCL_ADAPT_LPF;
		sie_calcBSHScl(&bs_h, &bs_h_adj);
	}

	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&bs_h, SIE_CHG_BS_H);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_module_set_load_bs_v(KDRV_SIE_PROC_ID id)
{
	SIE_BS_V_INFO bs_v = {0};
	SIE_BS_V_ADJ_INFO bs_v_adj;
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	//calc scaling factor
	bs_v.uiOutSize = kdrv_sie_info[id]->scl_size.h;
	if (kdrv_sie_info[id]->crp_window.win.h == bs_v.uiOutSize) {
		*func_en &= (~SIE_BS_V_EN);
	} else {
		*func_en |= SIE_BS_V_EN;
		bs_v_adj.uiInSz = kdrv_sie_info[id]->crp_window.win.h;
		bs_v_adj.uiOutSz = bs_v.uiOutSize;
		bs_v_adj.uiLpf = RAW_SCL_LPF;
		bs_v_adj.uiBinPwr = RAW_SCL_BIN_PWR;
		bs_v_adj.bAdaptiveLpf = RAW_SCL_ADAPT_LPF;
		sie_calcBSVScl(&bs_v, &bs_v_adj);
	}

	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&bs_v, SIE_CHG_BS_V);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_scl_size(KDRV_SIE_PROC_ID id)
{
	// check h/v bayer scale limitation
	if (kdrv_sie_info[id]->scl_size.w > kdrv_sie_info[id]->crp_window.win.w) {
		kdrv_sie_dbg_err("Bayer scale only for scale down, crop_width %d, scl_width %d, reset scale as crop\r\n", (int)kdrv_sie_info[id]->crp_window.win.w, (int)kdrv_sie_info[id]->scl_size.w);
		kdrv_sie_info[id]->scl_size.w = kdrv_sie_info[id]->crp_window.win.w;// reset scale info as crop window
	}

	if (kdrv_sie_info[id]->scl_size.h > kdrv_sie_info[id]->crp_window.win.h) {
		kdrv_sie_dbg_err("Bayer scale only for scale down, crop_height %d, scl_height %d, reset scale as crop\r\n", (int)kdrv_sie_info[id]->crp_window.win.h, (int)kdrv_sie_info[id]->scl_size.h);
		kdrv_sie_info[id]->scl_size.h = kdrv_sie_info[id]->crp_window.win.h;// reset scale info as crop window
	}

	if ((kdrv_sie_info[id]->crp_window.win.w != kdrv_sie_info[id]->scl_size.w) || (kdrv_sie_info[id]->crp_window.win.h != kdrv_sie_info[id]->scl_size.h)) {
		if (kdrv_sie_info[id]->crp_window.cfa_pat == KDRV_SIE_RGGB_PIX_R) {
			if ((kdrv_sie_info[id]->crp_window.win.w == kdrv_sie_info[id]->scl_size.w) && (kdrv_sie_info[id]->crp_window.win.h != kdrv_sie_info[id]->scl_size.h)) {
				kdrv_sie_dbg_err("id = %d Can't Scale Vertical only, support H/V and H-only, reset scale as crop\r\n", (int)id);
				kdrv_sie_info[id]->scl_size.w = kdrv_sie_info[id]->crp_window.win.w;
				kdrv_sie_info[id]->scl_size.h = kdrv_sie_info[id]->crp_window.win.h;
			}
		} else {
			kdrv_sie_dbg_err("CFA need to be R for raw scale, current %d, reset scale as crop\r\n", (int)kdrv_sie_info[id]->crp_window.cfa_pat);
			kdrv_sie_info[id]->scl_size.w = kdrv_sie_info[id]->crp_window.win.w;
			kdrv_sie_info[id]->scl_size.h = kdrv_sie_info[id]->crp_window.win.h;
		}
	}

	kdrv_sie_module_set_load_bs_h(id);
	kdrv_sie_module_set_load_bs_v(id);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_dvi(KDRV_SIE_PROC_ID id)
{
	SIE_DVI_INFO dvi = {0};
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	switch (kdrv_sie_info[id]->data_fmt) {
	case KDRV_SIE_YUV_422_SPT:
		dvi.bOutSplit = TRUE;
		break;

	case KDRV_SIE_YUV_422_NOSPT:
		dvi.bOutSplit = FALSE;
		break;

	case KDRV_SIE_YUV_420_SPT:
		dvi.bOutSplit = TRUE;
		dvi.bOutYUV420= TRUE;
		break;

	default:
		break;
	}

	if (kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
		*func_en &= (~SIE_DVI_EN);
	} else {
		*func_en |= SIE_DVI_EN;
	}
	dvi.DviOutSwap = __kdrv_sie_typecast_yuv_order(kdrv_sie_info[id]->ccir_info.yuv_order, kdrv_sie_info[id]->flip, kdrv_sie_info[id]->ccir_info.dvi_mode);
	dvi.DviFormat = kdrv_sie_info[id]->ccir_info.fmt;
	dvi.DviInMode = kdrv_sie_info[id]->ccir_info.dvi_mode;
	dvi.bFieldEn = kdrv_sie_info[id]->ccir_info.filed_enable;
	dvi.bFieldSel = kdrv_sie_info[id]->ccir_info.filed_sel;
	dvi.bCCIR656VdSel = kdrv_sie_info[id]->ccir_info.ccir656_vd_sel;
	dvi.uiDataPeriod = kdrv_sie_info[id]->ccir_info.data_period;
	dvi.uiDataIdx = kdrv_sie_info[id]->ccir_info.data_idx;
	dvi.bAutoAlign = kdrv_sie_info[id]->ccir_info.auto_align;
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&dvi, SIE_CHG_DVI);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_addr(KDRV_SIE_PROC_ID id, UINT32 ch_bit)
{
	SIE_CHG_OUT_ADDR_INFO addr = {0};

	if (ch_bit & (1 << KDRV_SIE_CH0)) {
		addr.uiOut0Addr = kdrv_sie_info[id]->out_ch_addr[KDRV_SIE_CH0];
	}

	if (ch_bit & (1 << KDRV_SIE_CH1)) {
		addr.uiOut1Addr = kdrv_sie_info[id]->out_ch_addr[KDRV_SIE_CH1];
	}

	if (ch_bit & (1 << KDRV_SIE_CH2)) {
		addr.uiOut2Addr = kdrv_sie_info[id]->out_ch_addr[KDRV_SIE_CH2];
	}
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&addr, SIE_CHG_OUT_ADDR_FLIP);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_lof(KDRV_SIE_PROC_ID id, UINT32 ch_bit)
{
	SIE_CHG_IO_LOFS_INFO lofs = {0};

	if (ch_bit & (1 << KDRV_SIE_CH0)) {
		lofs.uiOut0Lofs = kdrv_sie_info[id]->out_ch_lof[KDRV_SIE_CH0];
	}

	if (ch_bit & (1 << KDRV_SIE_CH1)) {
		lofs.uiOut1Lofs = kdrv_sie_info[id]->out_ch_lof[KDRV_SIE_CH1];
	}

	if (ch_bit & (1 << KDRV_SIE_CH2)) {
		lofs.uiOut2Lofs = kdrv_sie_info[id]->out_ch_lof[KDRV_SIE_CH2];
	}
#if 0
	if (ch_bit & (1 << KDRV_SIE_CH3)) {
		lofs.uiOut3Lofs = kdrv_sie_info[id]->out_ch_lof[KDRV_SIE_CH3];
	}

	if (ch_bit & (1 << KDRV_SIE_CH4)) {
		lofs.uiOut4Lofs = kdrv_sie_info[id]->out_ch_lof[KDRV_SIE_CH4];
	}

	if (ch_bit & (1 << KDRV_SIE_CH5)) {
		lofs.uiOut5Lofs = kdrv_sie_info[id]->out_ch_lof[KDRV_SIE_CH5];
	}
#endif
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&lofs, SIE_CHG_IO_LOFS);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_flip(KDRV_SIE_PROC_ID id)
{
	SIE_FLIP_INFO flip = {0};
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	if (kdrv_sie_info[id]->flip & KDRV_SIE_FLIP_H) {
		if (*func_en & SIE_DVI_EN) {
			flip.bOut0HFlip = TRUE;
			flip.bOut1HFlip = TRUE;
		} else {
			kdrv_sie_dbg_wrn("SIE Unsupport RAW flip\r\n");
		}
	}

	if (kdrv_sie_info[id]->flip & KDRV_SIE_FLIP_V) {
		if (*func_en & SIE_DVI_EN) {
			flip.bOut0VFlip = TRUE;
			flip.bOut1VFlip = TRUE;
		} else {
			kdrv_sie_dbg_wrn("SIE Unsupport RAW flip\r\n");
		}
	}

	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&flip, SIE_CHG_FLIP);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_ob(KDRV_SIE_PROC_ID id)
{
	SIE_OB_OFS_INFO ob_ofs = {0};
	SIE_OB_DT_INFO ob_dt = {0};
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	if (!kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
		// only raw support ob
		return KDRV_SIE_E_NOSPT;
	}

	ob_ofs.uiObOfs = kdrv_sie_info[id]->ob_param.ob_ofs;
	#if 0 // for kdrv, need check @PQ suggest to force disable this func.
	if (kdrv_sie_info[id]->ob_param.b_ob_avg_en) {
		*func_en |= (SIE_OB_SUB_SEL|SIE_OB_AVG_EN);
		ob_dt.uiStX = kdrv_sie_info[id]->ob_param.ob_avg_win.x;
		ob_dt.uiStY = kdrv_sie_info[id]->ob_param.ob_avg_win.y;
		ob_dt.uiSzX = kdrv_sie_info[id]->ob_param.ob_avg_win.w;
		ob_dt.uiSzY = kdrv_sie_info[id]->ob_param.ob_avg_win.h;
		ob_dt.uiDivX = kdrv_sie_info[id]->ob_param.ob_avg_step;
		ob_dt.uiThres = kdrv_sie_info[id]->ob_param.ob_avg_thres;
		ob_dt.uiSubRatio = kdrv_sie_info[id]->ob_param.ob_avg_ratio;
	} else {
	#endif
		*func_en &= ~(SIE_OB_SUB_SEL|SIE_OB_AVG_EN);
	//}

	if (kdrv_sie_info[id]->ob_param.bypass_enable) {
		*func_en |= SIE_OB_BYPASS_EN;
	} else {
		*func_en &= (~SIE_OB_BYPASS_EN);
	}

	// load to driver
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&ob_ofs, SIE_CHG_OBOFS);
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&ob_dt, SIE_CHG_OBDT);

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_stcs_path(KDRV_SIE_PROC_ID id)
{
	SIE_STCS_PATH_INFO stcs_path = {0};

	if (!kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
		// only raw support stcs
		return KDRV_SIE_E_NOSPT;
	}

	if (kdrv_sie_info[id]->comp_info.enable) {
		stcs_path.Companding_shift = kdrv_sie_info[id]->comp_info.comp_shift;
	}

	if (kdrv_sie_info[id]->ca_param.enable) {
		stcs_path.bCaThEn = kdrv_sie_info[id]->ca_param.th_enable;
		stcs_path.bCaAccmSrc = kdrv_sie_info[id]->ca_param.ca_src;
	}

	if (kdrv_sie_info[id]->la_param.enable) {
		stcs_path.La1SrcSel = kdrv_sie_info[id]->la_param.la_src;
		stcs_path.LaRgb2Y1Mod = kdrv_sie_info[id]->la_param.la_rgb2y1mod;
		stcs_path.LaRgb2Y2Mod = kdrv_sie_info[id]->la_param.la_rgb2y2mod;
		stcs_path.bLaCgEn = kdrv_sie_info[id]->la_param.cg_enable;
		stcs_path.bLaGma1En = kdrv_sie_info[id]->la_param.gamma_enable;
		if (kdrv_sie_info[id]->la_param.histogram_enable) {
			stcs_path.HistoSrcSel = kdrv_sie_info[id]->la_param.histogram_src;
		}
	}

	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&stcs_path, SIE_CHG_STCS_PATH);

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_stcs_size(KDRV_SIE_PROC_ID id)
{
	SIE_STCS_CALASIZE_GRP_INFO stcs_size = {0};

	if (!kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
		return KDRV_SIE_E_NOSPT;
	}

	if (kdrv_sie_info[id]->ca_param.enable || kdrv_sie_info[id]->la_param.enable) {
		UPOINT shift = {0};
		SIE_STCS_CALASIZE_ADJ_INFO setting;

		stcs_size.StcsCaIrSubInfo.uiCaIrSubRWet = kdrv_sie_info[id]->ca_param.irsub_r_weight;
		stcs_size.StcsCaIrSubInfo.uiCaIrSubGWet = kdrv_sie_info[id]->ca_param.irsub_g_weight;
		stcs_size.StcsCaIrSubInfo.uiCaIrSubBWet = kdrv_sie_info[id]->ca_param.irsub_b_weight;

		stcs_size.StcsLaIrSubInfo.uiLaIrSubRWet = kdrv_sie_info[id]->la_param.irsub_r_weight;
		stcs_size.StcsLaIrSubInfo.uiLaIrSubGWet = kdrv_sie_info[id]->la_param.irsub_g_weight;
		stcs_size.StcsLaIrSubInfo.uiLaIrSubBWet = kdrv_sie_info[id]->la_param.irsub_b_weight;

		setting.uiCaRoiStX = kdrv_sie_info[id]->ca_roi.roi.x;
		setting.uiCaRoiStY = kdrv_sie_info[id]->ca_roi.roi.y;
		setting.uiCaRoiSzX = kdrv_sie_info[id]->ca_roi.roi.w;
		setting.uiCaRoiSzY = kdrv_sie_info[id]->ca_roi.roi.h;
		setting.uiCaWinNmX = kdrv_sie_info[id]->ca_param.win_num.w;
		setting.uiCaWinNmY = kdrv_sie_info[id]->ca_param.win_num.h;
		//error handle for only enable la(la still need ca crop and win)
		//ca crop set to crop window, ca win set to max value
		if (setting.uiCaRoiSzX == 0) {
			kdrv_sie_dbg_wrn("SIE CA roi width zero, force set to crop width\r\n");
			setting.uiCaRoiSzX = kdrv_sie_info[id]->crp_window.win.w;
		}
		if (setting.uiCaRoiSzY == 0) {
			kdrv_sie_dbg_wrn("SIE CA roi height zero, force set to crop height\r\n");
			setting.uiCaRoiSzY = kdrv_sie_info[id]->crp_window.win.h;
		}
		if (setting.uiCaWinNmX == 0) {
			kdrv_sie_dbg_wrn("SIE CA x win zero, force set to maximum %d\r\n", (int)kdrv_sie_com_limit[id].ca_win_max_num.w);
			setting.uiCaWinNmX = kdrv_sie_com_limit[id].ca_win_max_num.w;
		}
		if (setting.uiCaWinNmY == 0) {
			kdrv_sie_dbg_wrn("SIE CA y win zero, force set to maximum %d\r\n", (int)kdrv_sie_com_limit[id].ca_win_max_num.h);
			setting.uiCaWinNmY = kdrv_sie_com_limit[id].ca_win_max_num.h;
		}

		setting.uiLaRoiStX = kdrv_sie_info[id]->la_roi.roi.x;
		setting.uiLaRoiStY = kdrv_sie_info[id]->la_roi.roi.y;
		setting.uiLaRoiSzX = kdrv_sie_info[id]->la_roi.roi.w;
		setting.uiLaRoiSzY = kdrv_sie_info[id]->la_roi.roi.h;
		setting.uiLaWinNmX = kdrv_sie_info[id]->la_param.win_num.w;
		setting.uiLaWinNmY = kdrv_sie_info[id]->la_param.win_num.h;
		sie_calcCaLaSize(kdrv_sie_conv2_sie_id(id), &stcs_size, &setting);

		kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_SET] = stcs_size.StcsCaWinInfo;
		kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_SET] = stcs_size.StcsLaWinInfo;

		shift.x = setting.uiCaRoiStX;
		shift.y = setting.uiCaRoiStY;
		if (kdrv_sie_info[id]->crp_window.cfa_pat >= KDRV_SIE_RGBIR_PIX_RG_GI && kdrv_sie_info[id]->crp_window.cfa_pat <= KDRV_SIE_RGBIR_PIX_IG_GB) {
			stcs_size.StcsCaCrpInfo.CfaPat = __kdrv_sie_typecast_rgbir_cfa(kdrv_sie_info[id]->crp_window.cfa_pat, shift);
		} else {
			stcs_size.StcsCaCrpInfo.CfaPat = __kdrv_sie_typecast_raw_cfa(kdrv_sie_info[id]->crp_window.cfa_pat, shift);
		}
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&stcs_size, SIE_CHG_CALASIZE_GRP);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_out_dest(KDRV_SIE_PROC_ID id)
{
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	if (kdrv_sie_info[id]->out_dest == KDRV_SIE_OUT_DEST_DRAM || kdrv_sie_info[id]->out_dest == KDRV_SIE_OUT_DEST_BOTH) {
		*func_en |= SIE_DRAM_OUT0_EN;
	} else {
		*func_en &= (~SIE_DRAM_OUT0_EN);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_ca(KDRV_SIE_PROC_ID id)
{
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;
	SIE_CA_TH_INFO ca_th = {0};
	SIE_STCS_OB_INFO stcs_ob = {0};
	SIE_CA_IRSUB_INFO ca_irsub = {0};

	if (!kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
		// only raw support ca
		return KDRV_SIE_E_NOSPT;
	}
	//kdrv_sie_dbg_err( "ofs %d/%d\r\n",kdrv_sie_info[id]->ca_param.ca_ob_ofs, kdrv_sie_info[id]->la_param.la_ob_ofs);

	// CA info
	if (kdrv_sie_info[id]->ca_param.enable) {
		*func_en |= SIE_STCS_CA_EN;
		if (kdrv_sie_info[id]->ca_param.th_enable) {
			ca_th.uiGThLower = kdrv_sie_info[id]->ca_param.g_th_l;
			ca_th.uiGThUpper = kdrv_sie_info[id]->ca_param.g_th_u;
			ca_th.uiRGThLower = kdrv_sie_info[id]->ca_param.r_th_l;
			ca_th.uiRGThUpper = kdrv_sie_info[id]->ca_param.r_th_u;
			ca_th.uiBGThLower = kdrv_sie_info[id]->ca_param.b_th_l;
			ca_th.uiBGThUpper = kdrv_sie_info[id]->ca_param.b_th_u;
			ca_th.uiPGThLower = kdrv_sie_info[id]->ca_param.p_th_l;
			ca_th.uiPGThUpper = kdrv_sie_info[id]->ca_param.p_th_u;
			sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&ca_th, SIE_CHG_CA_TH);
		}
		ca_irsub.uiCaIrSubRWet = kdrv_sie_info[id]->ca_param.irsub_r_weight;
		ca_irsub.uiCaIrSubGWet = kdrv_sie_info[id]->ca_param.irsub_g_weight;
		ca_irsub.uiCaIrSubBWet = kdrv_sie_info[id]->ca_param.irsub_b_weight;
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&ca_irsub, SIE_CHG_CA_IRSUB);
		// update ca ofs
		stcs_ob.uiCaObOfs = kdrv_sie_info[id]->ca_param.ca_ob_ofs;
		stcs_ob.uiLaObOfs = kdrv_sie_info[id]->la_param.la_ob_ofs;
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&stcs_ob, SIE_CHG_STCS_OB);
	} else {
		*func_en &= (~SIE_STCS_CA_EN);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_la(KDRV_SIE_PROC_ID id)
{
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;
	SIE_LA_CG_INFO la_cg = {0};
	SIE_LA_GMA_INFO la_gma = {0};
	SIE_STCS_OB_INFO stcs_ob = {0};
	SIE_LA_IRSUB_INFO la_ir_sub = {0};

	if (!kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
		// only raw support la
		return KDRV_SIE_E_NOSPT;
	}

	if (kdrv_sie_info[id]->la_param.enable) {
		*func_en |= SIE_STCS_LA_EN;
		if (kdrv_sie_info[id]->la_param.histogram_enable) {
			*func_en |= SIE_STCS_HISTO_Y_EN;
		}

		if (kdrv_sie_info[id]->la_param.cg_enable) {
			la_cg.uiRGain = kdrv_sie_info[id]->la_param.r_gain;
			la_cg.uiGGain = kdrv_sie_info[id]->la_param.g_gain;
			la_cg.uiBGain = kdrv_sie_info[id]->la_param.b_gain;
			sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&la_cg, SIE_CHG_LA_CG);
		}

		if (kdrv_sie_info[id]->la_param.gamma_enable) {
			la_gma.puiGmaTbl = (UINT16 *)kdrv_sie_info[id]->la_param.gamma_tbl_addr;
			sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&la_gma, SIE_CHG_LA_GMA);
		}

		la_ir_sub.uiLaIrSubRWet = kdrv_sie_info[id]->la_param.irsub_r_weight;
		la_ir_sub.uiLaIrSubGWet = kdrv_sie_info[id]->la_param.irsub_g_weight;
		la_ir_sub.uiLaIrSubBWet = kdrv_sie_info[id]->la_param.irsub_b_weight;
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&la_ir_sub, SIE_CHG_LA_IRSUB);

		stcs_ob.uiCaObOfs = kdrv_sie_info[id]->ca_param.ca_ob_ofs;
		stcs_ob.uiLaObOfs = kdrv_sie_info[id]->la_param.la_ob_ofs;
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&stcs_ob, SIE_CHG_STCS_OB);
	} else {
		*func_en &= ~(SIE_STCS_LA_EN|SIE_STCS_HISTO_Y_EN);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_encode(KDRV_SIE_PROC_ID id)
{
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;
	UINT32 bcc_rate = kdrv_sie_info[id]->encode_info.enc_rate;

	if (kdrv_sie_info[id]->encode_info.enable) {
		if (!kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
			kdrv_sie_dbg_err("Only raw support encode\r\n");
			return KDRV_SIE_E_NOSPT;
		}

		*func_en |= SIE_RAWENC_EN;
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&bcc_rate, SIE_CHG_BCC_ADJ);
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&bcc_rate, SIE_CHG_RAWENC_RATE);
	} else {
		*func_en &= ~(SIE_RAWENC_EN);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_dgain(KDRV_SIE_PROC_ID id)
{
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;
	SIE_DGAIN_INFO dgain = {0};

	if (kdrv_sie_info[id]->dgain.enable) {
		*func_en |= SIE_DGAIN_EN;
		dgain.uiGainIn8P8Bit = kdrv_sie_info[id]->dgain.gain;
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&dgain, SIE_CHG_DGAIN);
	} else {
		*func_en &= ~(SIE_DGAIN_EN);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_cgain(KDRV_SIE_PROC_ID id)
{
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;
	SIE_CG_INFO cgain = {0};

	if (kdrv_sie_info[id]->cgain.enable) {
		*func_en |= SIE_CGAIN_EN;
		cgain.bGainSel = kdrv_sie_info[id]->cgain.sel_37_fmt;
		cgain.uiBGain = kdrv_sie_info[id]->cgain.b_gain;
		cgain.uiGbGain = kdrv_sie_info[id]->cgain.gb_gain;
		cgain.uiGrGain = kdrv_sie_info[id]->cgain.gr_gain;
		cgain.uiRGain = kdrv_sie_info[id]->cgain.r_gain;
		cgain.uiIrGain = kdrv_sie_info[id]->cgain.ir_gain;

		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&cgain, SIE_CHG_CG);
	} else {
		*func_en &= ~(SIE_CGAIN_EN);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_dpc(KDRV_SIE_PROC_ID id)
{
	SIE_CHG_IN_ADDR_INFO addr = {0};
	SIE_DPC_INFO dpc_info = {0};
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;
	UPOINT shift = {0};

	// DPC
	if (kdrv_sie_info[id]->dpc_info.enable) {
		if (!kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
			return KDRV_SIE_E_NOSPT;  // only raw sensor support dpc
		}
		*func_en |= SIE_DPC_EN;
		addr.uiIn1Addr = kdrv_sie_info[id]->dpc_info.tbl_addr;
		addr.uiIn1Size = kdrv_sie_info[id]->dpc_info.dp_total_size;
		dpc_info.DefFact = kdrv_sie_info[id]->dpc_info.weight;

		shift.x = kdrv_sie_info[id]->act_window.win.x;
		shift.y = kdrv_sie_info[id]->act_window.win.y;
		if (kdrv_sie_info[id]->act_window.cfa_pat >= KDRV_SIE_RGBIR_PIX_RG_GI && kdrv_sie_info[id]->act_window.cfa_pat <= KDRV_SIE_RGBIR_PIX_IG_GB)	{
			dpc_info.DpcCfapat = __kdrv_sie_typecast_rgbir_cfa(kdrv_sie_info[id]->act_window.cfa_pat, shift);
		} else {
			dpc_info.DpcCfapat = __kdrv_sie_typecast_raw_cfa(kdrv_sie_info[id]->act_window.cfa_pat, shift);
		}

		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&dpc_info, SIE_CHG_DP);
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&addr, SIE_CHG_IN_ADDR);
	} else {
		*func_en &= ~(SIE_DPC_EN);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_ecs(KDRV_SIE_PROC_ID id)
{
	SIE_ECS_ADJ_INFO ecs_adj;
	SIE_ECS_INFO ecs_info = {0};
	SIE_CHG_IN_ADDR_INFO addr = {0};
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	if (kdrv_sie_info[id]->ecs_info.enable) {
		if (!kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
			return KDRV_SIE_E_NOSPT;  // only raw sensor support ecs
		}
		*func_en |= SIE_ECS_EN;
		if (kdrv_sie_info[id]->ecs_info.bayer_mode == KDRV_SIE_ECS_4CH_8B) {
			*func_en |= SIE_ECS_BAYER_MODE_SEL;
		} else {
			*func_en &= ~SIE_ECS_BAYER_MODE_SEL;
		}
		addr.uiIn2Addr = kdrv_sie_info[id]->ecs_info.map_tbl_addr;
		ecs_info.MapSizeSel = kdrv_sie_info[id]->ecs_info.map_sel;
		if (ecs_info.MapSizeSel == ECS_MAPSIZE_65X65) {
			addr.uiIn2Size = 65*65;
		} else if (ecs_info.MapSizeSel == ECS_MAPSIZE_49X49) {
			addr.uiIn2Size = 49*49;
		} else if (ecs_info.MapSizeSel == ECS_MAPSIZE_33X33) {
			addr.uiIn2Size = 33*33;
		} else {
			kdrv_sie_dbg_err("Unknown map sel %d\r\n", ecs_info.MapSizeSel);
		}

		if (kdrv_sie_info[id]->ecs_info.sel_37_fmt) {
			ecs_info.uiMapShift = 7;
		} else {
			ecs_info.uiMapShift = 8;
		}

		ecs_info.bDthrEn = kdrv_sie_info[id]->ecs_info.dthr_enable;
		ecs_info.bDthrRst = kdrv_sie_info[id]->ecs_info.dthr_reset;
		ecs_info.uiDthrLvl = kdrv_sie_info[id]->ecs_info.dthr_level;

		ecs_adj.uiActSzX = kdrv_sie_info[id]->act_window.win.w;
		ecs_adj.uiActSzY = kdrv_sie_info[id]->act_window.win.h;
		sie_calcECSScl(&ecs_info, &ecs_adj);

		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&addr, SIE_CHG_IN_ADDR);
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&ecs_info, SIE_CHG_ECS);

	} else {
		*func_en &= ~(SIE_ECS_EN);
	}
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_mclk(KDRV_SIE_PROC_ID id)
{
	SIE_MCLKSRC_SEL mclk_src;

	switch (kdrv_sie_clk_hdl[id].mclk_info.mclk_src_sel) {
	case KDRV_SIE_MCLKSRC_CURR:
		mclk_src = SIE_MCLKSRC_CURR;
		break;

	case KDRV_SIE_MCLKSRC_480:
		mclk_src = SIE_MCLKSRC_480;
		break;

	case KDRV_SIE_MCLKSRC_PLL4:
		mclk_src = SIE_MCLKSRC_PLL4;
		break;

	case KDRV_SIE_MCLKSRC_PLL5:
		mclk_src = SIE_MCLKSRC_PLL5;
		break;

	case KDRV_SIE_MCLKSRC_PLL10:
		mclk_src = SIE_MCLKSRC_PLL10;
		break;

	case KDRV_SIE_MCLKSRC_PLL12:
		mclk_src = SIE_MCLKSRC_PLL12;
		break;

	case KDRV_SIE_MCLKSRC_PLL18:
		mclk_src = SIE_MCLKSRC_PLL18;
		break;

	default:
		kdrv_sie_dbg_err("mclk src select overflow, sel = %d\r\n", (int)kdrv_sie_clk_hdl[id].mclk_info.mclk_src_sel);
		mclk_src = SIE_MCLKSRC_CURR;
		break;
	}

	kdrv_sie_fb_clk_info[id].mclk_src = mclk_src;
    if (kdrv_sie_clk_hdl[id].mclk_info.mclk_id_sel == KDRV_SIE_MCLK) {
		sie_setmclock(mclk_src,kdrv_sie_clk_hdl[id].mclk_info.clk_rate, kdrv_sie_clk_hdl[id].mclk_info.clk_en);
	} else if (kdrv_sie_clk_hdl[id].mclk_info.mclk_id_sel == KDRV_SIE_MCLK2) {
		sie_setmclock2(mclk_src,kdrv_sie_clk_hdl[id].mclk_info.clk_rate, kdrv_sie_clk_hdl[id].mclk_info.clk_en);
	} else if (kdrv_sie_clk_hdl[id].mclk_info.mclk_id_sel == KDRV_SIE_MCLK3) {
		sie_setmclock3(mclk_src,kdrv_sie_clk_hdl[id].mclk_info.clk_rate, kdrv_sie_clk_hdl[id].mclk_info.clk_en);
	}
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_pxclk(KDRV_SIE_PROC_ID id)
{
	SIE_PXCLKSRC pxclk_src = __kdrv_sie_typecast_pxclk_src(kdrv_sie_clk_hdl[id].pxclk_info);

	kdrv_sie_fb_clk_info[id].pxclk_src = pxclk_src;
	sie_setPxClock(kdrv_sie_conv2_sie_id(id), pxclk_src);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_sieclk(KDRV_SIE_PROC_ID id)
{
	SIE_CLKSRC_SEL clk_src = __kdrv_sie_typecast_clk_src(kdrv_sie_clk_hdl[id].clk_info.clk_src_sel);

	kdrv_sie_fb_clk_info[id].clk_src = clk_src;
	sie_setClock(kdrv_sie_conv2_sie_id(id), clk_src, kdrv_sie_clk_hdl[id].clk_info.rate);
	sie_getClock(kdrv_sie_conv2_sie_id(id), &clk_src, &kdrv_sie_clk_hdl[id].clk_info.rate);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_dma_out(KDRV_SIE_PROC_ID id)
{
	BOOL dma_out = kdrv_sie_info[id]->b_dma_out;

	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&dma_out, SIE_CHG_ACT_ENG);
	if (kdrv_sie_status[id] == KDRV_SIE_STS_RUN) {
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&dma_out, SIE_CHG_ACT_EN);
	}
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_data_format(KDRV_SIE_PROC_ID id)
{
	SIE_PACKBUS_SEL data = __kdrv_sie_typecast_packbus(kdrv_sie_info[id]->data_fmt);
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	if (kdrv_sie_typecast_raw(kdrv_sie_info[id]->data_fmt)) {
		*func_en &= (~SIE_DVI_EN);
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&data, SIE_CHG_OUT_BITDEPTH);
	} else {
		*func_en |= SIE_DVI_EN;
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_bp1(KDRV_SIE_PROC_ID id)
{
	SIE_BREAKPOINT_INFO bp = {0};

	bp.uiBp1 = kdrv_sie_info[id]->bp_info.bp1;
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&bp, SIE_CHG_BP1);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_bp2(KDRV_SIE_PROC_ID id)
{
	SIE_BREAKPOINT_INFO bp = {0};

	bp.uiBp2 = kdrv_sie_info[id]->bp_info.bp2;
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&bp, SIE_CHG_BP2);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_bp3(KDRV_SIE_PROC_ID id)
{
	SIE_BREAKPOINT_INFO bp = {0};

	bp.uiBp3 = kdrv_sie_info[id]->bp_info.bp3;
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&bp, SIE_CHG_BP3);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_pat_gen(KDRV_SIE_PROC_ID id)
{
	SIE_SRC_WIN_INFO src_win;
	SIE_PATGEN_INFO pat_gen;
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	if (kdrv_sie_info[id]->pat_gen_info.src_win.w == 0 && kdrv_sie_info[id]->pat_gen_info.src_win.h == 0) {
		*func_en &= ~SIE_PATGEN_EN;
	} else {
		if (kdrv_sie_info[id]->open_cfg.act_mode == KDRV_SIE_IN_PATGEN || kdrv_sie_info[id]->open_cfg.act_mode == KDRV_SIE_IN_PARA_MSTR_SNR || kdrv_sie_info[id]->open_cfg.act_mode == KDRV_SIE_IN_PARA_SLAV_SNR) {
			*func_en |= SIE_PATGEN_EN;
		} else {
			*func_en &= ~SIE_PATGEN_EN;
		}
		src_win.uiSzX = kdrv_sie_info[id]->pat_gen_info.src_win.w;
		src_win.uiSzY = kdrv_sie_info[id]->pat_gen_info.src_win.h;
		pat_gen.PatGenMode = kdrv_sie_info[id]->pat_gen_info.mode;
		pat_gen.uiPatGenVal = kdrv_sie_info[id]->pat_gen_info.val;
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&src_win, SIE_CHG_SRC_WIN);
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&pat_gen, SIE_CHG_PAT_GEN);
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_ring_buf(KDRV_SIE_PROC_ID id)
{
	SIE_CHG_RINGBUF_INFO ring_buf;

	ring_buf.ringbufEn = kdrv_sie_info[id]->ring_buf_info.enable;
	ring_buf.ringbufLen = kdrv_sie_info[id]->ring_buf_info.ring_buf_len;
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&ring_buf, SIE_CHG_RINGBUF);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_single_out(KDRV_SIE_PROC_ID id)
{
	SIE_DRAM_SINGLE_OUT sin_out;

	sin_out.SingleOut0En = kdrv_sie_info[id]->singleout_info.ch0singleout_enable;
	sin_out.SingleOut1En = kdrv_sie_info[id]->singleout_info.ch1singleout_enable;
	sin_out.SingleOut2En = kdrv_sie_info[id]->singleout_info.ch2singleout_enable;
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&sin_out, SIE_CHG_SINGLEOUT);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_out_mode(KDRV_SIE_PROC_ID id)
{
	SIE_DRAM_OUT_CTRL ch_out_mode;

	ch_out_mode.out0mode = kdrv_sie_info[id]->ch_output_mode.out0_mode;
	ch_out_mode.out1mode = kdrv_sie_info[id]->ch_output_mode.out1_mode;
	ch_out_mode.out2mode = kdrv_sie_info[id]->ch_output_mode.out2_mode;
	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&ch_out_mode, SIE_CHG_OUTMODE);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_companding(KDRV_SIE_PROC_ID id)
{
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;

	if (kdrv_sie_info[id]->comp_info.enable) {
		*func_en |= SIE_COMPANDING_EN;
		// set companding and decompanding
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&kdrv_sie_info[id]->comp_info.decomp_info, SIE_CHG_DECOMPANDING);
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&kdrv_sie_info[id]->comp_info.comp_info, SIE_CHG_COMPANDING);
	} else {
		*func_en &= ~(SIE_COMPANDING_EN);
	}
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_inte(KDRV_SIE_PROC_ID id)
{
	UINT32 inte = kdrv_sie_info[id]->inte;

	sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&inte, SIE_CHG_INTE);
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_set_load_md(KDRV_SIE_PROC_ID id)
{
	UINT32 *func_en = &kdrv_sie_info[id]->func_en;
    SIE_MD_INFO sie_md_info;

	if (kdrv_sie_info[id]->md_info.enable) {
		*func_en |= SIE_MD_EN;
		sie_md_info.md_src = kdrv_sie_info[id]->md_info.md_src;
		sie_md_info.sum_frms = kdrv_sie_info[id]->md_info.sum_frms;
		sie_md_info.mask_mode = kdrv_sie_info[id]->md_info.mask_mode;
		sie_md_info.mask0 = kdrv_sie_info[id]->md_info.mask0;
		sie_md_info.mask1 = kdrv_sie_info[id]->md_info.mask1;
		sie_md_info.blkdiff_thr = kdrv_sie_info[id]->md_info.blkdiff_thr;
		sie_md_info.total_blkdiff_thr = kdrv_sie_info[id]->md_info.total_blkdiff_thr;
		sie_md_info.blkdiff_cnt_thr = kdrv_sie_info[id]->md_info.blkdiff_cnt_thr;
		sie_chgParam(kdrv_sie_conv2_sie_id(id), (void *)&sie_md_info, SIE_CHG_MD);
	} else {
		*func_en &= ~(SIE_MD_EN);
	}
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_set_load(KDRV_SIE_PROC_ID id, void *data)
{
	UINT32 ch_bit = 0;
	unsigned long myflags;
	UINT8 cur_load_flg[KDRV_SIE_PARAM_MAX] = {0};

	if (kdrv_sie_status[id] == KDRV_SIE_STS_RUN) {
		loc_cpu(myflags);
		memcpy((void *)&cur_load_flg[0], (void *)&kdrv_sie_load_flg[id], sizeof(cur_load_flg));
		memset((void *)&kdrv_sie_load_flg[id], 0, sizeof(kdrv_sie_load_flg[id]));
		unl_cpu(myflags);

		// Active Window
		if (cur_load_flg[KDRV_SIE_ITEM_ACT_WIN] != 0) {
			kdrv_sie_set_load_act(id);
			kdrv_sie_set_load_crp(id);
			kdrv_sie_set_load_stcs_size(id);
		}

		// Crop Window
		if (cur_load_flg[KDRV_SIE_ITEM_CROP_WIN] != 0) {
			kdrv_sie_set_load_crp(id);
			kdrv_sie_set_load_stcs_size(id);
		}

		// Scale out
		if (cur_load_flg[KDRV_SIE_ITEM_SCALEOUT] != 0) {
			kdrv_sie_set_load_scl_size(id);
			kdrv_sie_set_load_stcs_size(id);
		}

		//color gain
		if (cur_load_flg[KDRV_SIE_ITEM_CGAIN] != 0) {
			kdrv_sie_set_load_cgain(id);
		}

		// mclk
		if (cur_load_flg[KDRV_SIE_ITEM_MCLK] != 0) {
			kdrv_sie_set_load_mclk(id);
		}

		//pxclk
		if (cur_load_flg[KDRV_SIE_ITEM_PXCLK] != 0) {
			kdrv_sie_set_load_pxclk(id);
		}

		//clock
		if (cur_load_flg[KDRV_SIE_ITEM_SIECLK] != 0) {
			kdrv_sie_set_load_sieclk(id);
		}

		// Data fomat, YUV Order
		if ((cur_load_flg[KDRV_SIE_ITEM_YUV_ORDER] != 0) ||
			(cur_load_flg[KDRV_SIE_ITEM_FLIP] != 0) ||
			(cur_load_flg[KDRV_SIE_ITEM_CCIR] != 0)) {
			kdrv_sie_set_load_dvi(id);
		}

		// signal
		if (cur_load_flg[KDRV_SIE_ITEM_SIGNAL] != 0) {
			//update in kdrv_sie_set_load_main_in
		}

		// Flip
		if (cur_load_flg[KDRV_SIE_ITEM_FLIP] != 0) {
			kdrv_sie_set_load_flip(id);
		}

		// STCS PATH
		// STCS SIZE
		if (cur_load_flg[KDRV_SIE_ITEM_CA] != 0 ||
			cur_load_flg[KDRV_SIE_ITEM_LA] != 0) {
			kdrv_sie_set_load_stcs_path(id);
			kdrv_sie_set_load_stcs_size(id);
		}

		// out dest
		if (cur_load_flg[KDRV_SIE_ITEM_OUT_DEST] != 0) {
			kdrv_sie_set_load_out_dest(id);
		}

		// CA
		if (cur_load_flg[KDRV_SIE_ITEM_CA] != 0) {
			kdrv_sie_set_load_ca(id);
		}

		// LA
		if (cur_load_flg[KDRV_SIE_ITEM_LA] != 0) {
			kdrv_sie_set_load_la(id);
		}

		// DPC
		if (cur_load_flg[KDRV_SIE_ITEM_DPC] != 0) {
			kdrv_sie_set_load_dpc(id);
		}

		// ECS
		if (cur_load_flg[KDRV_SIE_ITEM_ECS] != 0) {
			kdrv_sie_set_load_ecs(id);
		}

		// Encode
		if (cur_load_flg[KDRV_SIE_ITEM_ENCODE] != 0) {
			kdrv_sie_set_load_encode(id);
		}

		// DGain
		if (cur_load_flg[KDRV_SIE_ITEM_DGAIN] != 0) {
			kdrv_sie_set_load_dgain(id);
		}

		// out Lineoffset
		ch_bit = 0;
		if (cur_load_flg[KDRV_SIE_ITEM_CH0_LOF] != 0) {
			ch_bit |= 1 << KDRV_SIE_CH0;
		}

		if (cur_load_flg[KDRV_SIE_ITEM_CH1_LOF] != 0) {
			ch_bit |= 1 << KDRV_SIE_CH1;
		}

		if (cur_load_flg[KDRV_SIE_ITEM_CH2_LOF] != 0) {
			ch_bit |= 1 << KDRV_SIE_CH2;
		}

		if (ch_bit != 0) {
			kdrv_sie_set_load_lof(id, ch_bit);
		}

		// out Addr
		if (cur_load_flg[KDRV_SIE_ITEM_CH0_ADDR] != 0) {
			ch_bit |= 1 << KDRV_SIE_CH0;
		}

		if (cur_load_flg[KDRV_SIE_ITEM_CH1_ADDR] != 0) {
			ch_bit |= 1 << KDRV_SIE_CH1;
		}

		if (cur_load_flg[KDRV_SIE_ITEM_CH2_ADDR] != 0) {
			ch_bit |= 1 << KDRV_SIE_CH2;
		}

		if (cur_load_flg[KDRV_SIE_ITEM_FLIP] != 0) {
			ch_bit = KDRV_SIE_CH_ALL;
		}
		if (ch_bit != 0) {
			kdrv_sie_set_load_addr(id, ch_bit);
		}

		// pattern gen
		if (cur_load_flg[KDRV_SIE_ITEM_PATGEN_INFO] != 0) {
			kdrv_sie_set_load_pat_gen(id);
		}

	    // ring-buffer
		if (cur_load_flg[KDRV_SIE_ITEM_RING_BUF] != 0) {
			kdrv_sie_set_load_ring_buf(id);
		}

		// OB
		if (cur_load_flg[KDRV_SIE_ITEM_OB] != 0) {
			kdrv_sie_set_load_ob(id);
		}

		// BP
		if (cur_load_flg[KDRV_SIE_ITEM_BP1] != 0) {
			kdrv_sie_set_load_bp1(id);
		}
		if (cur_load_flg[KDRV_SIE_ITEM_BP2] != 0) {
			kdrv_sie_set_load_bp2(id);
		}
		if (cur_load_flg[KDRV_SIE_ITEM_BP3] != 0) {
			kdrv_sie_set_load_bp3(id);
		}

		// out-mode
		if (cur_load_flg[KDRV_SIE_ITEM_OUTPUT_MODE] != 0) {
			kdrv_sie_set_load_out_mode(id);
		}

	    // single-out
		if (cur_load_flg[KDRV_SIE_ITEM_SINGLEOUT] != 0) {
			kdrv_sie_set_load_single_out(id);
		}

		// companding
		if (cur_load_flg[KDRV_SIE_ITEM_COMPANDING] != 0) {
			kdrv_sie_set_load_companding(id);
			kdrv_sie_set_load_stcs_path(id);	//update companding shift
		}

		//INTE
		if (cur_load_flg[KDRV_SIE_ITEM_INTE] != 0) {
			kdrv_sie_set_load_inte(id);
		}

		//data format
		if (cur_load_flg[KDRV_SIE_ITEM_DATA_FMT] != 0) {
			kdrv_sie_set_load_data_format(id);
		}

		//dma output(active enable)
		if (cur_load_flg[KDRV_SIE_ITEM_DMA_OUT_EN] != 0) {
			kdrv_sie_set_load_dma_out(id);
		}

	    // md
		if (cur_load_flg[KDRV_SIE_ITEM_MD] != 0) {
			kdrv_sie_set_load_md(id);
		}

		// Update funcen
		if (kdrv_sie_info[id] != NULL) {
			if ((kdrv_sie_fb_normal_stream[id] == FALSE) && fastboot) {
				sie_chgFuncEn(kdrv_sie_conv2_sie_id(id), SIE_FUNC_ENABLE, kdrv_sie_info[id]->func_en);
			} else {
				sie_chgFuncEn(kdrv_sie_conv2_sie_id(id), SIE_FUNC_SET, kdrv_sie_info[id]->func_en);
			}
		}
	}

	return KDRV_SIE_E_OK;
}

KDRV_SIE_SET_FP kdrv_sie_set_fp[KDRV_SIE_PARAM_MAX] = {
	kdrv_sie_set_opencfg,
	kdrv_sie_set_isrcb,
	kdrv_sie_set_mclk,
	kdrv_sie_set_pxclk,
	kdrv_sie_set_sieclk,
	kdrv_sie_set_act_win,
	kdrv_sie_set_crp_win,
	kdrv_sie_set_out_dest,
	kdrv_sie_set_data_format,
	kdrv_sie_set_yuv_order,
	kdrv_sie_set_signal,
	kdrv_sie_set_flip,
	kdrv_sie_set_inte,
	kdrv_sie_set_ch0_out_lof,
	kdrv_sie_set_ch1_out_lof,
	kdrv_sie_set_ch2_out_lof,
	kdrv_sie_set_ch0_out_addr,
	kdrv_sie_set_ch1_out_addr,
	kdrv_sie_set_ch2_out_addr,
	kdrv_sie_set_pat_gen_info,
	kdrv_sie_set_scl_size,
	kdrv_sie_set_ob,
	kdrv_sie_set_ca,
	kdrv_sie_set_ca_roi,
	NULL,
	kdrv_sie_set_la,
	kdrv_sie_set_la_roi,
	NULL,
	NULL,
	kdrv_sie_set_encode,
	kdrv_sie_set_dgain,
	kdrv_sie_set_cgain,
	kdrv_sie_set_dpc,
	kdrv_sie_set_ecs,
	kdrv_sie_set_ccir,
	kdrv_sie_set_load,
	kdrv_sie_set_dma_out,
	kdrv_sie_set_bp1,
	kdrv_sie_set_bp2,
	kdrv_sie_set_bp3,
	NULL,
	kdrv_sie_set_companding,
	kdrv_sie_set_single_out,
	kdrv_sie_set_out_mode,
	kdrv_sie_set_ring_buf,
	NULL,
	kdrv_sie_set_md,
	NULL,
};

#if 0
#endif

static INT32 kdrv_sie_get_mclk(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_MCLK_INFO *)data = kdrv_sie_clk_hdl[id].mclk_info;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_pxclk(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_PXCLKSRC_SEL *)data = kdrv_sie_clk_hdl[id].pxclk_info;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_sieclk(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_CLK_INFO *)data = kdrv_sie_clk_hdl[id].clk_info;
	return KDRV_SIE_E_OK;
}


static INT32 kdrv_sie_get_act_win(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_ACT_CRP_WIN *)data = kdrv_sie_info[id]->act_window;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_crp_win(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_ACT_CRP_WIN *)data = kdrv_sie_info[id]->crp_window;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_scl_size(KDRV_SIE_PROC_ID id, void *data)
{
	*(USIZE *)data = kdrv_sie_info[id]->scl_size;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_out_dest(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_OUT_DEST *)data = kdrv_sie_info[id]->out_dest;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_datafmt(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_DATA_FMT *)data = kdrv_sie_info[id]->data_fmt;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_yuv_order(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DVI) {
		*(KDRV_SIE_YUV_ORDER *)data = kdrv_sie_info[id]->ccir_info.yuv_order;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. DVI\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_signal(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_SIGNAL *)data = kdrv_sie_info[id]->signal;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_flip(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_FLIP *)data = kdrv_sie_info[id]->flip;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_inte(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_INT *)data = kdrv_sie_info[id]->inte;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_out_lof(KDRV_SIE_PROC_ID id, KDRV_SIE_OUT_LOFS *lofs_info)
{
	if (lofs_info->ch_id > kdrv_sie_com_limit[id].out_max_ch || lofs_info->ch_id >= KDRV_SIE_CH_MAX) {
		kdrv_sie_dbg_wrn("id = %d output channel %d overflow (max ch %d)\r\n", (int)id, (int)lofs_info->ch_id, (int)kdrv_sie_com_limit[id].out_max_ch);
		return KDRV_SIE_E_PAR;
	}
	lofs_info->lofs = kdrv_sie_info[id]->out_ch_lof[lofs_info->ch_id];
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_ch0_lof(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_LOFS lofs_info = {0};

	lofs_info.ch_id = KDRV_SIE_CH0;
	kdrv_sie_get_out_lof(id, &lofs_info);
	*(UINT32 *)data = lofs_info.lofs;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_ch1_lof(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_LOFS lofs_info = {0};

	lofs_info.ch_id = KDRV_SIE_CH1;
	kdrv_sie_get_out_lof(id, &lofs_info);
	*(UINT32 *)data = lofs_info.lofs;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_ch2_lof(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_LOFS lofs_info = {0};

	lofs_info.ch_id = KDRV_SIE_CH2;
	kdrv_sie_get_out_lof(id, &lofs_info);
	*(UINT32 *)data = lofs_info.lofs;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_out_addr(KDRV_SIE_PROC_ID id, KDRV_SIE_OUT_ADDR *addr_info)
{
	if (addr_info->ch_id > kdrv_sie_com_limit[id].out_max_ch || addr_info->ch_id >= KDRV_SIE_CH_MAX) {
		kdrv_sie_dbg_wrn("id = %d output channel %d overflow (max ch %d)\r\n", (int)id, (int)addr_info->ch_id, (int)kdrv_sie_com_limit[id].out_max_ch);
		return KDRV_SIE_E_NOSPT;
	}
	addr_info->addr = kdrv_sie_info[id]->out_ch_addr[addr_info->ch_id];
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_ch0_addr(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_ADDR addr_info = {0};

	addr_info.ch_id = KDRV_SIE_CH0;
	kdrv_sie_get_out_addr(id, &addr_info);
	*(UINT32 *)data = addr_info.addr;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_ch1_addr(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_ADDR addr_info = {0};

	addr_info.ch_id = KDRV_SIE_CH1;
	kdrv_sie_get_out_addr(id, &addr_info);
	*(UINT32 *)data = addr_info.addr;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_ch2_addr(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_OUT_ADDR addr_info = {0};

	addr_info.ch_id = KDRV_SIE_CH2;
	kdrv_sie_get_out_addr(id, &addr_info);
	*(UINT32 *)data = addr_info.addr;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_pat_gen_info(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_PATGEN) {
		*(KDRV_SIE_PATGEN_INFO *)data = kdrv_sie_info[id]->pat_gen_info;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. pattern gen\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_ob(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_OB_BYPASS) {
		*(KDRV_SIE_OB_PARAM *)data = kdrv_sie_info[id]->ob_param;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. OB\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_ca(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_CA) {
		*(KDRV_SIE_CA_PARAM *)data = kdrv_sie_info[id]->ca_param;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. ca\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_ca_roi(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_CA) {
		*(KDRV_SIE_CA_ROI *)data = kdrv_sie_info[id]->ca_roi;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. ca\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_ca_rst(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_CA_RST_INFO *kdrv_ca_rst = (KDRV_SIE_CA_RST_INFO *)data;
	SIE_STCS_CA_RSLT_INFO ca_rst_info;
	SIE_CA_IRSUB_INFO ca_irsub_info = {0};

	ca_rst_info.puiBufR = kdrv_ca_rst->ca_rst->buf_r;
	ca_rst_info.puiBufG = kdrv_ca_rst->ca_rst->buf_g;
	ca_rst_info.puiBufB = kdrv_ca_rst->ca_rst->buf_b;
	ca_rst_info.puiBufIR = kdrv_ca_rst->ca_rst->buf_ir;
	ca_rst_info.puiAccCnt = kdrv_ca_rst->ca_rst->acc_cnt;

	if (kdrv_ca_rst->src_addr != 0 && kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_RDY].uiWinNmX != 0 && kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_RDY].uiWinNmY != 0) {
		sie_get_ca_rslt(kdrv_sie_conv2_sie_id(id), &ca_rst_info, &kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_RDY], kdrv_ca_rst->src_addr);
		ca_irsub_info.uiCaIrSubRWet = kdrv_sie_info[id]->ca_param.irsub_r_weight;
		ca_irsub_info.uiCaIrSubGWet = kdrv_sie_info[id]->ca_param.irsub_g_weight;
		ca_irsub_info.uiCaIrSubBWet = kdrv_sie_info[id]->ca_param.irsub_b_weight;
		if (kdrv_sie_info[id]->act_window.cfa_pat >= KDRV_SIE_RGBIR_PIX_RG_GI && kdrv_sie_info[id]->act_window.cfa_pat <= KDRV_SIE_RGBIR_PIX_IG_GB) {
		    __kdrv_sie_calc_ir_level(id, &ca_rst_info,  kdrv_sie_info[id]->ca_win[KDRV_SIE_PAR_CTL_RDY], ca_irsub_info);
		}
	} else {
		return KDRV_SIE_E_NOSPT;
	}

	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_la(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_LA_PARAM *la_param = (KDRV_SIE_LA_PARAM *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA) {
		if (la_param != NULL) {
			*la_param = kdrv_sie_info[id]->la_param;
		}
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. LA\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_la_roi(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA) {
		*(KDRV_SIE_LA_ROI *)data = kdrv_sie_info[id]->la_roi;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. LA\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_la_rst(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_LA_RST_INFO *kdrv_la_rst = (KDRV_SIE_LA_RST_INFO *)data;

	if (kdrv_la_rst->src_addr != 0 && kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_RDY].uiWinNmX != 0 && kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_RDY].uiWinNmY != 0) {
		sie_get_la_rslt(kdrv_sie_conv2_sie_id(id), kdrv_la_rst->la_rst->buf_lum_1, kdrv_la_rst->la_rst->buf_lum_2, &kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_RDY], kdrv_la_rst->src_addr);
		if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA_HISTO) {
			sie_get_histo(kdrv_sie_conv2_sie_id(id), kdrv_la_rst->la_rst->buf_histogram);
		}
	} else {
		return KDRV_SIE_E_NOSPT;
	}

	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_la_accm(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_LA_ACCM *la_rst = (KDRV_SIE_LA_ACCM *)data;

	if (la_rst->src_addr != 0 && kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_RDY].uiWinNmX != 0 && kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_RDY].uiWinNmY != 0) {
		if (la_rst->lum_sel == 0) {
			sie_get_la_rslt(kdrv_sie_conv2_sie_id(id), la_rst->lum, NULL, &kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_RDY], la_rst->src_addr);
		} else if (la_rst->lum_sel == 1) {
			sie_get_la_rslt(kdrv_sie_conv2_sie_id(id), NULL, la_rst->lum, &kdrv_sie_info[id]->la_win[KDRV_SIE_PAR_CTL_RDY], la_rst->src_addr);
		}
	}
	return E_OK;
}

static INT32 kdrv_sie_get_encode(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_RAWENC) {
		*(KDRV_SIE_RAW_ENCODE *)data = kdrv_sie_info[id]->encode_info;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. RAW COMPRESS\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_dgain(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DGAIN) {
		*(KDRV_SIE_DGAIN *)data = kdrv_sie_info[id]->dgain;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. D gain\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_cgain(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_CGAIN) {
		*(KDRV_SIE_CGAIN *)data = kdrv_sie_info[id]->cgain;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. C gain\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_dpc(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DPC) {
		*(KDRV_SIE_DPC *)data = kdrv_sie_info[id]->dpc_info;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. DPC\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_ecs(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_ECS) {
		*(KDRV_SIE_ECS *)data = kdrv_sie_info[id]->ecs_info;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. ECS\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_ccir(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_DVI) {
		*(KDRV_SIE_CCIR_INFO *)data = kdrv_sie_info[id]->ccir_info;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. DVI\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_dma_out(KDRV_SIE_PROC_ID id, void *data)
{
	*(BOOL *)data = kdrv_sie_info[id]->b_dma_out;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_bp1(KDRV_SIE_PROC_ID id, void *data)
{
	*(UINT32 *)data = kdrv_sie_info[id]->bp_info.bp1;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_bp2(KDRV_SIE_PROC_ID id, void *data)
{
	*(UINT32 *)data = kdrv_sie_info[id]->bp_info.bp2;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_bp3(KDRV_SIE_PROC_ID id, void *data)
{
	*(UINT32 *)data = kdrv_sie_info[id]->bp_info.bp3;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_companding(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_COMPANDING) {
		*(KDRV_SIE_COMPANDING *)data = kdrv_sie_info[id]->comp_info;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. companding\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_single_out(KDRV_SIE_PROC_ID id, void *data)
{
	SIE_DRAM_SINGLE_OUT single_out_info = {0};
	KDRV_SIE_SINGLE_OUT_CTRL *single_out_ctrl = (KDRV_SIE_SINGLE_OUT_CTRL *)data;

	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_SINGLE_OUT) {
		sie_get_dram_single_out(kdrv_sie_conv2_sie_id(id),  &single_out_info);
		single_out_ctrl->ch0singleout_enable = single_out_info.SingleOut0En;
		single_out_ctrl->ch1singleout_enable = single_out_info.SingleOut1En;
		single_out_ctrl->ch2singleout_enable = single_out_info.SingleOut2En;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. single out\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_out_mode(KDRV_SIE_PROC_ID id, void *data)
{
	*(KDRV_SIE_DRAM_OUT_CTRL *)data = kdrv_sie_info[id]->ch_output_mode;
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_ring_buf(KDRV_SIE_PROC_ID id, void *data)
{
	if (kdrv_sie_com_limit[id].support_func & KDRV_SIE_FUNC_SPT_RING_BUF) {
		*(KDRV_SIE_RINGBUF_INFO *)data = kdrv_sie_info[id]->ring_buf_info;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("id = %d N.S. ring buffer\r\n", (int)id);
		return KDRV_SIE_E_NOSPT;
	}
}

static INT32 kdrv_sie_get_ir_info(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_RGBIR_INFO *ir_info = (KDRV_SIE_RGBIR_INFO *)data;

	ir_info->kdrv_sie_ir_level = sie_get_ir_level(id);
    sie_get_sat_gain_info(kdrv_sie_conv2_sie_id(id), &ir_info->kdrv_sie_ir_sat);
	return KDRV_SIE_E_OK;
}

static INT32 kdrv_sie_get_md_rst(KDRV_SIE_PROC_ID id, void *data)
{
	sie_get_mdrslt(kdrv_sie_conv2_sie_id(id), (SIE_MD_RESULT_INFO *)data);
	return KDRV_SIE_E_OK;
}

KDRV_SIE_GET_FP kdrv_sie_get_fp[KDRV_SIE_PARAM_MAX] = {
	NULL,
	NULL,
	kdrv_sie_get_mclk,
	kdrv_sie_get_pxclk,
	kdrv_sie_get_sieclk,
	kdrv_sie_get_act_win,
	kdrv_sie_get_crp_win,
	kdrv_sie_get_out_dest,
	kdrv_sie_get_datafmt,
	kdrv_sie_get_yuv_order,
	kdrv_sie_get_signal,
	kdrv_sie_get_flip,
	kdrv_sie_get_inte,
	kdrv_sie_get_ch0_lof,
	kdrv_sie_get_ch1_lof,
	kdrv_sie_get_ch2_lof,
	kdrv_sie_get_ch0_addr,
	kdrv_sie_get_ch1_addr,
	kdrv_sie_get_ch2_addr,
	kdrv_sie_get_pat_gen_info,
	kdrv_sie_get_scl_size,
	kdrv_sie_get_ob,
	kdrv_sie_get_ca,
	kdrv_sie_get_ca_roi,
	kdrv_sie_get_ca_rst,
	kdrv_sie_get_la,
	kdrv_sie_get_la_roi,
	kdrv_sie_get_la_rst,
	kdrv_sie_get_la_accm,
	kdrv_sie_get_encode,
	kdrv_sie_get_dgain,
	kdrv_sie_get_cgain,
	kdrv_sie_get_dpc,
	kdrv_sie_get_ecs,
	kdrv_sie_get_ccir,
	NULL,
	kdrv_sie_get_dma_out,
	kdrv_sie_get_bp1,
	kdrv_sie_get_bp2,
	kdrv_sie_get_bp3,
	kdrv_sie_get_sie_limit,
	kdrv_sie_get_companding,
	kdrv_sie_get_single_out,
	kdrv_sie_get_out_mode,
	kdrv_sie_get_ring_buf,
	kdrv_sie_get_ir_info,
	NULL,
	kdrv_sie_get_md_rst,
};

#if 0
#endif

/**
	kdrv sie public api
*/
UINT32 kdrv_sie_buf_query(UINT32 engine_num)
{
	UINT32 i;

	if(engine_num > (kdrv_sie_com_limit[0].max_spt_id + 1)) {
		kdrv_sie_dbg_err("KDRV SIE: engine number %d overflow, max = %d\r\n", (int)engine_num, (int)(kdrv_sie_com_limit[0].max_spt_id + 1));
		engine_num = kdrv_sie_com_limit[0].max_spt_id + 1;
	}

	memset((void *)&kdrv_sie_hdl_ctx, 0, sizeof(KDRV_SIE_HDL_CONTEXT));

	for (i = 0; i <= kdrv_sie_com_limit[0].max_spt_id; i++) {
		kdrv_sie_hdl_ctx.ctx_idx[i] = KDRV_SIE_ID_MAX_NUM;
	}
	kdrv_sie_hdl_ctx.ctx_num = engine_num;
	kdrv_sie_hdl_ctx.ctx_size = ALIGN_CEIL(sizeof(KDRV_SIE_INFO), VOS_ALIGN_BYTES);
	kdrv_sie_hdl_ctx.req_size = engine_num * kdrv_sie_hdl_ctx.ctx_size;

	kdrv_sie_dbg_ind("query num = %d, ctx_size = %x, req_size = %x\r\n", (int)engine_num, (unsigned int)kdrv_sie_hdl_ctx.ctx_size, (unsigned int)kdrv_sie_hdl_ctx.req_size);
	return kdrv_sie_hdl_ctx.req_size;
}

INT32 kdrv_sie_buf_init(UINT32 input_addr, UINT32 buf_size)
{
	if (kdrv_sie_init_flg) {
		kdrv_sie_dbg_wrn("kdrv sie already init\r\n");
		return KDRV_SIE_E_STATE;
	}

	kdrv_sie_dbg_ind("buf_addr = %x, buf_size = %x\r\n", (unsigned int)input_addr, (unsigned int)buf_size);
	if (input_addr == 0) {
		kdrv_sie_dbg_err("input address zero, kdrv init fail\r\n");
		return KDRV_SIE_E_NOMEM;
	}

	if (buf_size < kdrv_sie_hdl_ctx.req_size) {
		kdrv_sie_dbg_err("input size %x < req_size %x, kdrv init fail\r\n", (unsigned int)buf_size, (unsigned int)kdrv_sie_hdl_ctx.req_size);
		return KDRV_SIE_E_NOMEM;
	}

	kdrv_sie_hdl_ctx.start_addr = input_addr;
	kdrv_sie_init_flg = TRUE;
	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_buf_uninit(VOID)
{
	UINT32 i;

	if (kdrv_sie_init_flg) {
		for (i = 0 ; i < KDRV_SIE_MAX_ENG ; i++) {
			kdrv_sie_info[i] = NULL;
		}
		kdrv_sie_init_flg = FALSE;
		return KDRV_SIE_E_OK;
	} else {
		kdrv_sie_dbg_wrn("kdrv sie already uninit\r\n");
		return KDRV_SIE_E_STATE;
	}
}

INT32 kdrv_sie_open(UINT32 chip, UINT32 engine)
{
	SIE_OPENOBJ sie_open_obj = {0};
	KDRV_SIE_PROC_ID sie_id = engine - KDRV_VDOCAP_ENGINE0;
	INT32 rt = KDRV_SIE_E_OK;

	if (!kdrv_sie_init_flg) {
		kdrv_sie_dbg_err("kdrv sie init flag is FALSE\r\n");
		return KDRV_SIE_E_STATE;
	}

	if (!kdrv_sie_chk_id_valid(sie_id)) {
		return KDRV_SIE_E_ID;
	}

	__kdrv_sie_sem(sie_id, TRUE);// wait semophore
	rt = __kdrv_sie_state_machine(sie_id, KDRV_SIE_OP_OPEN);// check state machine
	if (rt != KDRV_SIE_E_OK) {
		__kdrv_sie_sem(sie_id, FALSE);// release semophore
		return rt;
	}

#if defined(__KERNEL__)
	fastboot = kdrv_builtin_is_fastboot();
#endif

	kdrv_sie_dbg_init(sie_id);

	kdrv_sie_clk_hdl[sie_id].clk_info.rate = kdrv_sie_info[sie_id]->open_cfg.data_rate;
	kdrv_sie_clk_hdl[sie_id].clk_info.clk_src_sel = kdrv_sie_info[sie_id]->open_cfg.clk_src_sel;
	kdrv_sie_clk_hdl[sie_id].pxclk_info = kdrv_sie_info[sie_id]->open_cfg.pclk_src_sel;

	sie_open_obj.pfSieIsrCb = NULL;
	sie_open_obj.uiSieClockRate = kdrv_sie_info[sie_id]->open_cfg.data_rate;
	sie_open_obj.SieClkSel = __kdrv_sie_typecast_clk_src(kdrv_sie_info[sie_id]->open_cfg.clk_src_sel);
	sie_open_obj.PxClkSel = __kdrv_sie_typecast_pxclk_src(kdrv_sie_info[sie_id]->open_cfg.pclk_src_sel);

	kdrv_sie_fb_clk_info[sie_id].clk_src = sie_open_obj.SieClkSel;	//update fastboot param.
	kdrv_sie_fb_clk_info[sie_id].pxclk_src = sie_open_obj.PxClkSel;	//update fastboot param.
	rt = sie_open(kdrv_sie_conv2_sie_id(sie_id), &sie_open_obj);	//open sie
	sie_getClock(kdrv_sie_conv2_sie_id(sie_id), &sie_open_obj.SieClkSel, &kdrv_sie_clk_hdl[sie_id].clk_info.rate);
	kdrv_sie_dbg_ind("id = %d, SieClockRate = %d, SieClkSel = %d, PxClkSel = %d\r\n", (int)sie_id, (int)sie_open_obj.uiSieClockRate, (int)sie_open_obj.SieClkSel, (int)sie_open_obj.PxClkSel);
	__kdrv_sie_sem(sie_id, FALSE);// release semophore
	return rt;
}

/**
	sie kdrv api
*/
INT32 kdrv_sie_close(UINT32 chip, UINT32 engine)
{
	KDRV_SIE_PROC_ID sie_id = engine - KDRV_VDOCAP_ENGINE0;
	INT32 rt = KDRV_SIE_E_OK;
	UINT32 i;
	SIE_DRAM_SINGLE_OUT single_out = {0};
	unsigned long flags;

	if (!kdrv_sie_init_flg) {
		kdrv_sie_dbg_err("kdrv sie init flag is FALSE\r\n");
		return KDRV_SIE_E_STATE;
	}

	if (!kdrv_sie_chk_id_valid(sie_id)) {
		return KDRV_SIE_E_ID;
	}
	__kdrv_sie_sem(sie_id, TRUE);// wait semophore
	rt = __kdrv_sie_state_machine(sie_id, KDRV_SIE_OP_CLOSE);// check state machine
	if (rt != KDRV_SIE_E_OK) {
		__kdrv_sie_sem(sie_id, FALSE);// release semophore
		return rt;
	}

	rt = sie_close(kdrv_sie_conv2_sie_id(sie_id));	//close sie
	kdrv_sie_dbg_uninit(sie_id);
	__kdrv_sie_sem(sie_id, FALSE);// release semophore
	sie_chgParam(kdrv_sie_conv2_sie_id(sie_id), (void *)&single_out, SIE_CHG_SINGLEOUT);

	//update context info
	loc_cpu(flags);
	kdrv_sie_hdl_ctx.ctx_used--;
	for (i = 0; i < KDRV_SIE_MAX_ENG; i++) {
		if (kdrv_sie_hdl_ctx.ctx_idx[i] == sie_id) {
			kdrv_sie_hdl_ctx.ctx_idx[i] = KDRV_SIE_ID_MAX_NUM;
			break;
		}
	}
	kdrv_sie_info[sie_id] = NULL;
	unl_cpu(flags);
	return rt;
}

INT32 kdrv_sie_resume(UINT32 id, void *param)
{
	ER rt = KDRV_SIE_E_OK;
	UINT32 sie_id = KDRV_DEV_ID_ENGINE(id) - KDRV_VDOCAP_ENGINE0;
	KDRV_SIE_RESUME_PARAM *res_param = (KDRV_SIE_RESUME_PARAM *)param;

	kdrv_sie_dbg_ind("id = %d, pxclk_en = %d\r\n", (int)sie_id, (int)res_param->pxclk_en);
	if (!kdrv_sie_chk_id_valid(sie_id)) {
		return KDRV_SIE_E_ID;
	}

	rt = __kdrv_sie_state_machine(sie_id, KDRV_SIE_OP_TRIG_START);
	if (rt != KDRV_SIE_E_OK) {
		return rt;
	}
	kdrv_sie_set_load(sie_id, NULL);
	sie_attach(kdrv_sie_conv2_sie_id(sie_id), TRUE);
	rt = sie_start(kdrv_sie_conv2_sie_id(sie_id));
	if (res_param->pxclk_en) {// enable pxclk
		// not-ready
	}
	return rt;
}

INT32 kdrv_sie_suspend(UINT32 id, void *param)
{
	ER rt = KDRV_SIE_E_OK;
	UINT32 sie_id = KDRV_DEV_ID_ENGINE(id) - KDRV_VDOCAP_ENGINE0;
	KDRV_SIE_SUSPEND_PARAM *sus_param = (KDRV_SIE_SUSPEND_PARAM *)param;

	kdrv_sie_dbg_ind("id = %d, pxclk_en = %d, wait_end = %d\r\n", (int)sie_id, (int)sus_param->pxclk_en, (int)sus_param->wait_end);
	if (!kdrv_sie_chk_id_valid(sie_id)) {
		return KDRV_SIE_E_ID;
	}

	rt = __kdrv_sie_state_machine(sie_id, KDRV_SIE_OP_TRIG_STOP);
	if (rt != KDRV_SIE_E_OK) {
		return rt;
	}
	rt = sie_pause(kdrv_sie_conv2_sie_id(sie_id));
#if 0	//remove wait end for sie_pause will enable sw_rst
	if (sus_param->wait_end) {
		sie_waitEvent(kdrv_sie_conv2_sie_id(sie_id), SIE_WAIT_VD, TRUE);  // wait-vd
	}
#endif
	sie_detach(kdrv_sie_conv2_sie_id(sie_id), TRUE);// disable sie_clk
    // disable pxclk
	if (sus_param->pxclk_en) {
		// not-ready
	}
	return rt;
}

INT32 kdrv_sie_set(UINT32 id, KDRV_SIE_PARAM_ID item, void *param)
{
	INT32 rt = KDRV_SIE_E_OK;
	UINT32 engine = KDRV_DEV_ID_ENGINE(id);
	UINT32 ign_chk = (KDRV_SIE_IGN_CHK & item);
	UINT32 sie_id = engine - KDRV_VDOCAP_ENGINE0;
	unsigned long myflags;

	item = item & (~KDRV_SIE_IGN_CHK);
	if (!kdrv_sie_init_flg) {
		kdrv_sie_dbg_err("kdrv sie init flag is FALSE\r\n");
		return KDRV_SIE_E_STATE;
	}

	if (!kdrv_sie_chk_id_valid(sie_id)) {
		return KDRV_SIE_E_ID;
	}

	if (item >= KDRV_SIE_PARAM_MAX) {
		kdrv_sie_dbg_err("id = %d illegal item %d > %d\r\n", (int)sie_id, (int)item, (int)KDRV_SIE_PARAM_MAX);
		return KDRV_SIE_E_PAR;
	}

	if (param == NULL && item != KDRV_SIE_ITEM_LOAD) {
		kdrv_sie_dbg_err( "data is NULL, id = %d, item = %d\r\n", (int)sie_id, (int)item);
		return KDRV_SIE_E_PAR;
	}

	if (!ign_chk) {
		__kdrv_sie_sem(sie_id, TRUE);	// wait semophore
	}

    if ((item != KDRV_SIE_ITEM_OPENCFG) && (item != KDRV_SIE_ITEM_LOAD) && (item != KDRV_SIE_ITEM_MCLK) && (item != KDRV_SIE_ITEM_SIECLK)) {
		rt = __kdrv_sie_state_machine(sie_id, KDRV_SIE_OP_SET);
		if (rt != KDRV_SIE_E_OK) {
			if (!ign_chk) {
				__kdrv_sie_sem(sie_id, FALSE); // release semophore
			}
			return rt;
		}
	}

	// check set function
	if ((kdrv_sie_set_fp[item] != NULL)) {
		if (item == KDRV_SIE_ITEM_OPENCFG || (kdrv_sie_info[sie_id] != NULL)) {
			rt = kdrv_sie_set_fp[item](sie_id, param);
			kdrv_sie_dbg_trc("[S]id:%d item:%d\r\n", (int)sie_id, (int)item);
			if (rt == KDRV_SIE_E_OK) {
				loc_cpu(myflags);
			    kdrv_sie_load_flg[sie_id][item] = 1; // update load flag for set_load
				unl_cpu(myflags);
			}
		}
	} else {
		kdrv_sie_dbg_wrn("fp is null, item %d\r\n", item);
	}
	if (!ign_chk) {
		__kdrv_sie_sem(sie_id, FALSE); // release semophore
	}
	return rt;
}

INT32 kdrv_sie_get(UINT32 id, KDRV_SIE_PARAM_ID item, void *param)
{
	INT32 rt = KDRV_SIE_E_OK;
	UINT32 engine = KDRV_DEV_ID_ENGINE(id);
	UINT32 ign_chk = (KDRV_SIE_IGN_CHK & item);
	UINT32 sie_id = engine - KDRV_VDOCAP_ENGINE0;

	if (!kdrv_sie_init_flg) {
		kdrv_sie_dbg_err("kdrv sie init flag is FALSE\r\n");
		return KDRV_SIE_E_STATE;
	}

	item = item & (~KDRV_SIE_IGN_CHK);
	if (!kdrv_sie_chk_id_valid(sie_id)) {
		return KDRV_SIE_E_ID;
	}
	if (item >= KDRV_SIE_PARAM_MAX) {
		kdrv_sie_dbg_err("illegal item %d > %d\r\n", (int)item, KDRV_SIE_PARAM_MAX);
		return KDRV_SIE_E_PAR;
	}

	if (param == NULL) {
		kdrv_sie_dbg_err("data is NULL, id = %d, item = %d\r\n", (int)sie_id, (int)item);
		return KDRV_SIE_E_PAR;
	}

	if (!ign_chk) {
		__kdrv_sie_sem(sie_id, TRUE);// wait semophore
	}
	if (item != KDRV_SIE_ITEM_LIMIT) {
		rt = __kdrv_sie_state_machine(sie_id, KDRV_SIE_OP_GET);
		if (rt != KDRV_SIE_E_OK) {
			if (!ign_chk) {
				__kdrv_sie_sem(sie_id, FALSE);// release semophore
			}
			return rt;
		}
	}

	// check set function
	if ((kdrv_sie_get_fp[item] != NULL) && (kdrv_sie_info[sie_id] != NULL)) {
		rt = kdrv_sie_get_fp[item](sie_id, param);
		kdrv_sie_dbg_trc("[G]id:%d item:%d\r\n", (int)sie_id, (int)item);
	} else {
		kdrv_sie_dbg_wrn("id = %d fp 0x%x or info 0x%x is null, item %d\r\n", (int)sie_id, (unsigned int)kdrv_sie_get_fp[item], (unsigned int)kdrv_sie_info[sie_id], (int)item);
	}

	if (!ign_chk) {
		__kdrv_sie_sem(sie_id, FALSE);// release semophore
	}
	return rt;
}

INT32 kdrv_sie_trigger(UINT32 id, KDRV_SIE_TRIGGER_PARAM *sie_param, KDRV_SIE_CALLBACK_FUNC *cb_func, void *user_data)
{
	INT32 rt = KDRV_SIE_E_OK;
	UINT32 engine = KDRV_DEV_ID_ENGINE(id);
	UINT32 sie_id = engine - KDRV_VDOCAP_ENGINE0;
	KDRV_SIE_TRIG_INFO trig_info = *(KDRV_SIE_TRIG_INFO *)user_data;
	SIE_DRAM_SINGLE_OUT single_out = {0};
	UINT32 i;
	const KDRV_SIE_DRV_ISR_FP sie_isr_fp[] = {
		__sie1_kdrv_isr,
		__sie2_kdrv_isr,
		__sie3_kdrv_isr,
#if (KDRV_SIE_MAX_ENG == KDRV_SIE_MAX_ENG_NT98528)
		__sie4_kdrv_isr,
		__sie5_kdrv_isr,
#endif
	};

	if (!kdrv_sie_chk_id_valid(sie_id)) {
		return KDRV_SIE_E_ID;
	}
	kdrv_sie_dbg_ind("id = %d, trig_type = %d, wait_end = %d\r\n", (int)sie_id, (int)trig_info.trig_type, (int)trig_info.wait_end);
	if (trig_info.trig_type == KDRV_SIE_TRIG_START) {
		rt = __kdrv_sie_state_machine(sie_id, KDRV_SIE_OP_TRIG_START);// check state machine
		if (rt != KDRV_SIE_E_OK) {
			return rt;
		}
		kdrv_sie_set_load(sie_id, NULL);
		__kdrv_sie_load_main_in(sie_id);

		sie_reg_vdlatch_isr_cb(kdrv_sie_conv2_sie_id(sie_id), sie_isr_fp[sie_id]);
		kdrv_sie_fb_normal_stream[sie_id] = TRUE;
		rt = sie_start(kdrv_sie_conv2_sie_id(sie_id));
		sie_platform_set_reg_log_disable(sie_id, FALSE);
	} else {
		rt = __kdrv_sie_state_machine(sie_id, KDRV_SIE_OP_TRIG_STOP);// check state machine
		if (rt != KDRV_SIE_E_OK) {
			return rt;
		}
		rt = sie_pause(kdrv_sie_conv2_sie_id(sie_id));
		if (rt == E_OK) {
			sie_chgParam(kdrv_sie_conv2_sie_id(sie_id), (void *)&single_out, SIE_CHG_SINGLEOUT);
		}
		for (i = 0; i < KDRV_SIE_MAX_ENG; i++) {
			kdrv_sie_fb_normal_stream[i] = TRUE;
		}
	}

	if (trig_info.wait_end) {
		if (sie_waitEvent(kdrv_sie_conv2_sie_id(sie_id), SIE_WAIT_VD, TRUE) != E_OK) {
			kdrv_sie_dbg_err("id = %d, trig_type = %d, wait VD fail\r\n", (int)sie_id, (int)trig_info.trig_type);
		}
	}
	return rt;
}

INT32 kdrv_sie_get_sie_limit(KDRV_SIE_PROC_ID id, void *data)
{
	KDRV_SIE_LIMIT *sie_limit = (KDRV_SIE_LIMIT *)data;

	memset((void *)sie_limit, 0, sizeof(KDRV_SIE_LIMIT));
	if(nvt_get_chip_id() == CHIP_NA51055) {	//@520
		if (id < KDRV_SIE_MAX_ENG_NT98515) {
			kdrv_sie_limit_98515[id].support_func = kdrv_sie_spt_func_98515[id];
			*sie_limit = kdrv_sie_limit_98515[id];
		}
	} else {	//CHIP_NA51084 @528
		if (id < KDRV_SIE_MAX_ENG_NT98528) {
			kdrv_sie_limit_98528[id].support_func = kdrv_sie_spt_func_98528[id];
			*sie_limit = kdrv_sie_limit_98528[id];
		}
	}

	if (sie_limit->max_spt_id >= KDRV_SIE_MAX_ENG) {
		kdrv_sie_dbg_err("max support id %d overflow, max id = %d\r\n", (int)sie_limit->max_spt_id, (int)(KDRV_SIE_MAX_ENG - 1));
		sie_limit->max_spt_id = KDRV_SIE_MAX_ENG - 1;
	}

	return KDRV_SIE_E_OK;
}

INT32 kdrv_sie_dump_fb_info(INT fd, UINT32 en)
{
	SIE_REG_LOG_RESULT reg_rst;
	UINT32 i, id = 0, strlen;
	CHAR strbuf[256];

	if (en) {
		sie_platform_set_reg_log_enable(id, TRUE);
	} else {
		for (id = 0; id < KDRV_SIE_MAX_ENG; id++) {
			if (kdrv_sie_get_state_machine(id) == KDRV_SIE_STS_CLOSE) {
				continue;
			}
			if (kdrv_sie_info[id] != NULL) {
				strlen = snprintf(strbuf, sizeof(strbuf), "\t\tsie%d_clk {\r\n\t\t\tclk_src = <%d>;\r\n\t\t\tclk_rate = <%d>;\r\n\t\t\tmclk_en = <%d>;\r\n\t\t\tmclk_id = <%d>;\r\n\t\t\tmclk_src = <%d>;\r\n\t\t\tmclk_rate = <%d>;\r\n\t\t\tpxclk_src = <%d>;\r\n\t\t};\r\n\r\n",
					(int)id+1, (int)kdrv_sie_fb_clk_info[id].clk_src, (int)kdrv_sie_clk_hdl[id].clk_info.rate, (int)kdrv_sie_clk_hdl[id].mclk_info.clk_en,
					(int)kdrv_sie_clk_hdl[id].mclk_info.mclk_id_sel+1, (int)kdrv_sie_fb_clk_info[id].mclk_src, (int)kdrv_sie_clk_hdl[id].mclk_info.clk_rate,
					(int)kdrv_sie_fb_clk_info[id].pxclk_src);
				vos_file_write(fd, (void *)strbuf, strlen);
			}
		}

		for (id = 0; id < KDRV_SIE_MAX_ENG; id++) {
			if (kdrv_sie_get_state_machine(id) == KDRV_SIE_STS_CLOSE) {
				continue;
			}
			if (kdrv_sie_info[id] != NULL) {
				reg_rst = sie_platform_get_reg_log(id);

				strlen = snprintf(strbuf, sizeof(strbuf), "\t\tsie%d_reg {\r\n\t\t\treg_num = <%d>;\r\n", (int)id+1, (int)reg_rst.cnt);
				vos_file_write(fd, (void *)strbuf, strlen);
				for (i = 0; i < reg_rst.cnt; i++) {
					if (i == 0) {
						strlen = snprintf(strbuf, sizeof(strbuf), "\t\t\treg_start = <0x%08x 0x%08x>,\r\n", (unsigned int)reg_rst.p_reg[i].ofs, (unsigned int)reg_rst.p_reg[i].val);
					} else if (i == reg_rst.cnt -1){
						strlen = snprintf(strbuf, sizeof(strbuf), "\t\t\t<0x%04x 0x%08x>;\r\n\t\t};\r\n\r\n", (unsigned int)reg_rst.p_reg[i].ofs, (unsigned int)reg_rst.p_reg[i].val);
					} else {
						strlen = snprintf(strbuf, sizeof(strbuf), "\t\t\t<0x%04x 0x%08x>,\r\n", (unsigned int)reg_rst.p_reg[i].ofs, (unsigned int)reg_rst.p_reg[i].val);
					}
					vos_file_write(fd, (void *)strbuf, strlen);
				}
				sie_platform_set_reg_log_disable(id, TRUE);
			}
		}
	}

	return KDRV_SIE_E_OK;
}

