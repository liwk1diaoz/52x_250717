/**
    @file       kdrv_ime.c
    @ingroup    Predefined_group_name

    @brief      ime device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "ime_platform.h"

#include "kdrv_type.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "kdrv_videoprocess/kdrv_ipp_utility.h"
#include "kdrv_videoprocess/kdrv_ime.h"
#include "kdrv_ime_int.h"



#if defined (__KERNEL__) || defined (__FREERTOS)
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"

static VK_DEFINE_SPINLOCK(kdrv_ime_spin_loc);

#else

#endif



typedef struct _KDRV_IME_FUNC_CTRL_ {
	union {
		struct {
			unsigned cmf       : 1;        // bits : 0
			unsigned lca       : 1;        // bits : 1
			unsigned dbcs      : 1;        // bits : 2
			unsigned ssr       : 1;        // bits : 3
			unsigned fgn       : 1;        // bits : 4
			unsigned tmnr      : 1;        // bits : 5
			unsigned yuvcvt    : 1;        // bits : 6
		} bit;
		UINT32 val;
	} ime_ctrl_bit;
} KDRV_IME_FUNC_CTRL;


typedef struct _KDRV_IME_FUNC_INT_CTRL_ {
	KDRV_IME_FUNC_CTRL func_sel;
	KDRV_IME_FUNC_CTRL func_op;
} KDRV_IME_FUNC_INT_CTRL;


static UINT32 g_kdrv_ime_init_flg = 0;
static UINT32 g_kdrv_ime_open_cnt[KDRV_IME_ID_MAX_NUM] = {0};
static UINT32 kdrv_ime_scaling_isd_stp_out_h_limit = 1344;
static UINT32 kdrv_ime_stp_max = 2688;

static KDRV_IME_HANDLE *g_kdrv_ime_trig_hdl = NULL;

#if 0
static KDRV_IME_HANDLE g_kdrv_ime_handle_tab[KDRV_IME_HANDLE_MAX_NUM];
// open info
KDRV_IME_OPENCFG g_kdrv_ime_clk_info[KDRV_IME_HANDLE_MAX_NUM] = {
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
	{280},
};
//parameter
KDRV_IME_PRAM  g_kdrv_ime_info[KDRV_IME_HANDLE_MAX_NUM];
//dbcs table
//static UINT32 g_kdrv_ime_dbcs_luma_wt_y_tab[KDRV_IME_HANDLE_MAX_NUM][KDRV_IME_DBCS_WT_LUT_TAB];
//static UINT32 g_kdrv_ime_dbcs_luma_wt_c_tab[KDRV_IME_HANDLE_MAX_NUM][KDRV_IME_DBCS_WT_LUT_TAB];


static KDRV_IME_FUNC_CTRL g_iq_func_en[KDRV_IME_HANDLE_MAX_NUM] = {0};
static KDRV_IME_FUNC_INT_CTRL g_ipl_func_en[KDRV_IME_HANDLE_MAX_NUM] = {0};
static KDRV_IME_FUNC_CTRL g_ime_iq_func_en[KDRV_IME_HANDLE_MAX_NUM] = {0};

static UINT32 g_ime_update_param[KDRV_IPP_MAX_CHANNEL_NUM][KDRV_IME_PARAM_MAX] = {FALSE};

#else
static KDRV_IME_HANDLE *g_kdrv_ime_handle_tab = NULL;
// open info
KDRV_IME_OPENCFG *g_kdrv_ime_clk_info = NULL;
//parameter
KDRV_IME_PRAM  *g_kdrv_ime_info = NULL;

static KDRV_IME_FUNC_CTRL *g_iq_func_en = NULL;
static KDRV_IME_FUNC_INT_CTRL *g_ipl_func_en = NULL;
static KDRV_IME_FUNC_CTRL *g_ime_iq_func_en = NULL;

#endif

//static BOOL g_ime_update_param[KDRV_IPP_MAX_CHANNEL_NUM][KDRV_IME_PARAM_MAX] = {FALSE};
static uint8_t g_ime_update_param[KDRV_IPP_MAX_CHANNEL_NUM][KDRV_IME_PARAM_MAX] = {FALSE};
static uint8_t g_ime_int_update_param[KDRV_IME_PARAM_MAX] = {FALSE};


BOOL g_kdrv_ime_set_clk_flg[KDRV_IME_ID_MAX_NUM] = {
	FALSE
};

//UINT32 kdrv_ime_channel_num = KDRV_IME_HANDLE_MAX_NUM;
UINT32 kdrv_ime_channel_num = 0;





//fixed parameters for 3dnr
//static IME_3DNR_LUMA_FUNC_CTRL g_tplnr_sq_luma_ctrl = {ENABLE, DISABLE, ENABLE, ENABLE, ENABLE, DISABLE, DISABLE, DISABLE};
//static UINT32 g_tplnr_candiate_pos_x[7] = {0x5, 0x1, 0x0, 0x6, 0x2, 0x5, 0x1};
//static UINT32 g_tplnr_candiate_pos_y[7] = {0x5, 0x5, 0x2, 0x2, 0x2, 0x5, 0x5};
//static UINT32 g_tplnr_periodic_sad_similar[4] =  {0x2, 0x0, 0x0, 0x0};
//static UINT32 g_tplnr_motion_map[5] = {0x2c, 0x23, 0x19, 0xc, 0x8};
//static UINT32 g_tplnr_chroma_level_map[16] = {0x0, 0x3, 0x7, 0xc, 0x11, 0x18, 0x20, 0x29, 0x34, 0x42, 0x51, 0x64, 0x7a, 0x95, 0xb4, 0xd9};

/*compile warning: the frame size of 2880 bytes is larger than 1024 bytes, need check how to slove*/
static IME_MODE_PRAM ime = {0};
IME_IQ_FLOW_INFO ime_iq_info;
static IME_CHROMA_ADAPTION_INFO        ime_lca_info;        ///< chroma adaption parameters, if useless, please set NULL
static IME_CHROMA_ADAPTION_SUBOUT_INFO ime_lca_sub_out_info;    ///< chroma adaption subout parameters, if useless, please set NULL
static IME_DBCS_INFO                   ime_dbcs_info;          ///< dark and bright region chroma supression parameters, if useless, please set NULL
//static IME_SSR_INFO                    ime_ssr_info;           ///< single image supre-resolution parameters, if useless, please set NULL
//static IME_CST_INFO                    ime_cst_info;            ///< color space transformation parameters, if useless, please set NULL
//static IME_FILM_GRAIN_INFO             ime_fg_info;            ///< film grain parameters, if useless, please set NULL
static IME_STAMP_INFO                  ime_ds_info;            ///< Data stamp parameters, if useless, please set NULL
static IME_STL_INFO                    ime_stl_info;           ///< edge statistic parameters, if useless, please set NULL
static IME_PM_INFO                     ime_pm_info;            ///< privacy mask parameters, if useless, please set NULL
static IME_TMNR_INFO                   ime_tmnr_info;         ///< 3DNR parameters
static IME_TMNR_REF_OUT_INFO           ime_tmnr_refout_info;  ///< 3dnr reference output
static IME_YUV_CVT_INFO                ime_yuv_cvt_info;       ///< yuv converter
static IME_YUV_COMPRESSION_INFO        ime_comps_info;         ///< yuv compression

// tmnr default value
static KDRV_IME_TMNR_PARAM ime_tmnr_default = {
	FALSE,
	{
		// KDRV_IME_TMNR_ME_PARAM          me_param
		KDRV_ME_UPDATE_RAND, TRUE, KDRV_MV_DOWN_SAMPLE_AVERAGE, KDRV_ME_SDA_TYPE_COMPENSATED,
		0, 3, 3, 16, 2048,
		{0x80, 0x80, 0x0, 0x100, 0x100, 0x100, 0x200, 0x200},
		{0xA, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF},
		0x8c,
		{0x4, 0x4, 0x3, 0x4, 0x4, 0x4, 0x4, 0x4},
		{0x1, 0x0, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0},
	},
	{
		// KDRV_IME_TMNR_MD_PARAM          md_param
		{0x18, 0x18, 0x16, 0x17, 0x16, 0x16, 0x16, 0x16},
		{0x26, 0x10, 0x38, 0x12, 0x1C, 0x25, 0x25, 0x25},
		{0x3D, 0x4F, 0x53, 0x5A, 0x4D, 0x52, 0x52, 0x52},
		{0x8, 0x10},
	},
	{
		// KDRV_IME_TMNR_MD_ROI_PARAM      md_roi_param
		{0x8, 0x10},
	},
	{
		// KDRV_IME_TMNR_MC_PARAM          mc_param
		{0x16E, 0x20C, 0x2EE, 0x314, 0x2E5, 0x2F2, 0x2F2, 0x2F2},
		{0x18, 0x18, 0x16, 0x17, 0x16, 0x16, 0x16, 0x16},
		{0x26, 0x10, 0x38, 0x12, 0x1C, 0x25, 0x25, 0x25},
		{0x3D, 0x4F, 0x53, 0x5A, 0x4D, 0x52, 0x52, 0x52},
		{0x8, 0x10},
	},
	{
		// KDRV_IME_TMNR_MC_ROI_PARAM      mc_roi_param
		{0x8, 0x10},
	},
	{
		// KDRV_IME_TMNR_PS_PARAM          ps_param
		FALSE, TRUE, TRUE, KDRV_PS_MODE_0, KDRV_MV_INFO_MODE_AVERAGE,
		0x8, 0x8,
		{0xb4, 0xe6},
		{0x200, 0x400},
		//{0x168, 0x96},
		0x6, 0x6, 0x0,
		{0xc0, 0x180},
		//0x154,
		0x800,
	},
	{
		// KDRV_IME_TMNR_NR_PARAM          nr_param
		TRUE, TRUE,
		0x1, 0x0, 0x80, 0x1, 0x1,
		{0x10, 0x10, 0x10, 0x10},
		{0x30, 0x20, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10},
		KDRV_PRE_FILTER_TYPE_B,
		{0x50, 0x5a, 0x64, 0x96},
		{0x40, 0x20},
		{0x8, 0x20, 0x20},
		{0xff, 0x20, 0x10},
		0x1f4, 0x640,
		{0x8, 0x10, 0x18, 0x20},
		{0x0, 0x4, 0xA, 0x14, 0x20, 0x2E, 0x40, 0x5A},
		{0x0, 0x4, 0xA, 0x14, 0x20, 0x2E, 0x40, 0x5A},
		{0x80, 0x40},
		FALSE,
	},
	//{
	//  // KDRV_IME_TMNR_DBG_PARAM   dbg_param
	//  FALSE, 0
	//},
	//{
	//  // KDRV_IME_TMNR_STATISTIC_PARAM   sta_param
	//  0x8, 0x4, 0x3c, 0x44, 0x0, 0x0
	//},
};

static KDRV_IME_TMNR_DBG_PARAM ime_tmnr_dbg_default = {
	FALSE, 0
};


unsigned long kdrv_ime_spin_lock(VOID)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
	return 0;
#else
	unsigned long loc_status;
	vk_spin_lock_irqsave(&kdrv_ime_spin_loc, loc_status);
	return loc_status;
#endif
}
//---------------------------------------------------------------

VOID kdrv_ime_spin_unlock(unsigned long loc_status)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#else
	vk_spin_unlock_irqrestore(&kdrv_ime_spin_loc, loc_status);
#endif
}


//--------------------------------------------------------------------------------------------
UINT32   ime_tmnr_sta_total_num[32][32] = {0}, ime_tmnr_sta_diff[32][32] = {0};

UINT16 tmnr_sta_div[33] = {
	8192, 8192, 4096, 2730, 2048, 1638, 1365, 1170, 1024, 910, 819, 744,
	682, 630, 585, 546, 512, 481, 455, 431, 409, 390, 372, 356,
	341, 327, 315, 303, 292, 282, 273, 264, 256,
};

static ER kdrv_ime_cal_sta_para(INT32 size_h, INT32 size_v, INT32 max_size, KDRV_IME_TMNR_STATISTIC_PARAM *get_sta_param)
{
	UINT32    ret_ne_sample_step_hori = 0;  //U8
	UINT32    ret_ne_sample_step_vert = 0;  //U8
	UINT32   ret_ne_sample_num_x = 0;      //U12
	UINT32   ret_ne_sample_num_y = 0;      //u12
	//UINT16   ret_ne_sample_st_x = 1;   //U12
	//UINT16   ret_ne_sample_st_y = 1;   //U12

	UINT32   dist;
	UINT32    step_x, step_y;
	INT32    cal_a, cal_b;
	UINT32    x_sample_num = 0, y_sample_num = 0;

	UINT32   num_x = ((UINT32)size_h >> 2) - 1;
	UINT32   num_y = ((UINT32)size_v >> 2) - 1;

	for (step_y = 1;  step_y <= 32 ; step_y++) {
		for (step_x = 1; step_x <= 32; step_x++) {
			x_sample_num = 0, y_sample_num = 0;

			//x_sample_num = (step_x == 1) ? (num_x) : (num_x / step_x) + 1;
			//y_sample_num = (step_y == 1) ? (num_y) : (num_y / step_y) + 1;
			x_sample_num = (step_x == 1) ? (num_x) : ((num_x * (UINT32)tmnr_sta_div[step_x]) >> 13) + 1;
			y_sample_num = (step_y == 1) ? (num_y) : ((num_y * (UINT32)tmnr_sta_div[step_y]) >> 13) + 1;
			ime_tmnr_sta_total_num[step_y - 1][step_x - 1] = y_sample_num * x_sample_num;

			cal_a = ((INT32)ime_tmnr_sta_total_num[step_y - 1][step_x - 1] - max_size);
			cal_b = cal_a * ((cal_a < 0) ? -1 : 1);
			ime_tmnr_sta_diff[step_y - 1][step_x - 1] = (UINT32)cal_b;
		}
	}

	ret_ne_sample_step_hori = 0;
	ret_ne_sample_step_vert = 0;
	dist = 0xFFFFFFFF;
	for (step_y = 1;  step_y <= 32 ; step_y++) {
		for (step_x = 1; step_x <= 32; step_x++) {

			if ((ime_tmnr_sta_diff[step_y - 1][step_x - 1] <= dist) && (ime_tmnr_sta_total_num[step_y - 1][step_x - 1] <= (UINT32)max_size)) {
				ret_ne_sample_step_hori = step_x;
				ret_ne_sample_step_vert = step_y;
				dist = ime_tmnr_sta_diff[step_y - 1][step_x - 1];
			}
		}
	}

	//ret_ne_sample_num_y = (ret_ne_sample_step_vert == 1) ? (num_y) : (num_y / ret_ne_sample_step_vert) + 1;
	//ret_ne_sample_num_x = (ret_ne_sample_step_hori == 1) ? (num_x) : (num_x / ret_ne_sample_step_hori) + 1;
	ret_ne_sample_num_y = (ret_ne_sample_step_vert == 1) ? (num_y) : ((num_y * (UINT32)tmnr_sta_div[ret_ne_sample_step_vert]) >> 13) + 1;
	ret_ne_sample_num_x = (ret_ne_sample_step_hori == 1) ? (num_x) : ((num_x * (UINT32)tmnr_sta_div[ret_ne_sample_step_hori]) >> 13) + 1;


	if ((INT32)(ret_ne_sample_num_x * ret_ne_sample_num_y) > max_size) {
		DBG_ERR("KDRV IME: wrong tmnr sta parameter...\r\n");
		return E_PAR;
	}

	//return reg value
	get_sta_param->sample_step_hori = ret_ne_sample_step_hori;
	get_sta_param->sample_step_vert = ret_ne_sample_step_vert;
	get_sta_param->sample_num_x = ret_ne_sample_num_x;
	get_sta_param->sample_num_y = ret_ne_sample_num_y;
	get_sta_param->sample_st_x = 1;
	get_sta_param->sample_st_y = 1;

	return E_OK;
}


static ER kdrv_ime_set_clk(UINT32 id, VOID *data)
{
	UINT32 i;
	KDRV_IME_OPENCFG *get_clk = NULL;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_WRN("KDRV IME: set clock parameter NULL...\r\n");
		return E_OK;
	}

	//loc_status = kdrv_ime_spin_lock();

	get_clk = (KDRV_IME_OPENCFG *)data;

	for (i = 0; i < kdrv_ime_channel_num; i++) {
		g_kdrv_ime_clk_info[id].ime_clock_sel = get_clk->ime_clock_sel;
	}

	g_kdrv_ime_set_clk_flg[id] = TRUE;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_OPENCFG] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ime_int_in_fmt_trans(KDRV_IME_IN_FMT dal_in_type, IME_INPUT_FORMAT_SEL *ime_drv_fmt)
{
	switch (dal_in_type) {
	case KDRV_IME_IN_FMT_YUV444:
		//*ime_drv_fmt = IME_INPUT_YCC_444;
		//DBG_ERR("KDRV IME: input don't support YCC444 format %d\r\n", dal_in_type);
		return E_NOSPT;
		break;

	case KDRV_IME_IN_FMT_YUV422:
		//*ime_drv_fmt = IME_INPUT_YCC_422;
		//DBG_ERR("KDRV IME: intput don't support YCC422 format %d\r\n", dal_in_type);
		return E_NOSPT;
		break;

	case KDRV_IME_IN_FMT_YUV420:
		*ime_drv_fmt = IME_INPUT_YCC_420;
		break;

	case KDRV_IME_IN_FMT_Y_PACK_UV422:
		//*ime_drv_fmt = IME_INPUT_YCCP_422;
		//DBG_ERR("KDRV IME: intput don't support YCC422-Packed format %d\r\n", dal_in_type);
		return E_NOSPT;
		break;

	case KDRV_IME_IN_FMT_Y_PACK_UV420:
		*ime_drv_fmt = IME_INPUT_YCCP_420;
		break;

	case KDRV_IME_IN_FMT_Y:
		*ime_drv_fmt = IME_INPUT_Y_ONLY;
		break;

	case KDRV_IME_IN_FMT_RGB:
		//*ime_drv_fmt = IME_INPUT_RGB;
		//DBG_ERR("KDRV IME: intput don't support RGB(YCC444) format %d\r\n", dal_in_type);
		return E_NOSPT;
		break;

	default:
		//DBG_ERR("KDRV IME: don't support this input format %d\r\n", dal_in_type);
		return E_NOSPT;
		break;
	}

	return E_OK;
}

static ER kdrv_ime_int_out_yuvfmt_trans(KDRV_IME_OUT_FMT dal_out_type, IME_OUTPUT_FORMAT_INFO *ime_drv_fmt)
{
	switch (dal_out_type) {
	case KDRV_IME_OUT_FMT_YUV444:
	case KDRV_IME_OUT_FMT_RGB:
		//ime_drv_fmt->out_format_sel = IME_OUTPUT_YCC_444;
		//ime_drv_fmt->out_format_type_sel = IME_OUTPUT_YCC_PLANAR;
		//DBG_ERR("KDRV IME: output don't support YCC444 format %d\r\n", dal_out_type);
		return E_NOSPT;
		break;

	case KDRV_IME_OUT_FMT_YUV422:
		//ime_drv_fmt->out_format_sel = IME_OUTPUT_YCC_422_COS;
		//ime_drv_fmt->out_format_type_sel = IME_OUTPUT_YCC_PLANAR;
		//DBG_ERR("KDRV IME: output don't support YCC422 output format %d\r\n", dal_out_type);
		return E_NOSPT;
		break;

	case KDRV_IME_OUT_FMT_YUV420:
		ime_drv_fmt->out_format_sel = IME_OUTPUT_YCC_420;
		ime_drv_fmt->out_format_type_sel = IME_OUTPUT_YCC_PLANAR;
		break;

	case KDRV_IME_OUT_FMT_Y_PACK_UV422:
		//ime_drv_fmt->out_format_sel = IME_OUTPUT_YCC_422_COS;
		//ime_drv_fmt->out_format_type_sel = IME_OUTPUT_YCC_UVPACKIN;
		//DBG_ERR("KDRV IME: output don't support YCC422-Packed format %d\r\n", dal_out_type);
		return E_NOSPT;
		break;

	case KDRV_IME_OUT_FMT_Y_PACK_UV420:
		ime_drv_fmt->out_format_sel = IME_OUTPUT_YCC_420;
		ime_drv_fmt->out_format_type_sel = IME_OUTPUT_YCC_UVPACKIN;
		break;

	case KDRV_IME_OUT_FMT_Y_PACK_UV444:
		//ime_drv_fmt->out_format_sel = IME_OUTPUT_YCC_444;
		//ime_drv_fmt->out_format_type_sel = IME_OUTPUT_YCC_UVPACKIN;
		//DBG_ERR("KDRV IME: output don't support YCC444-Packed format %d\r\n", dal_out_type);
		return E_NOSPT;
		break;

	case KDRV_IME_OUT_FMT_Y:
		ime_drv_fmt->out_format_sel = IME_OUTPUT_Y_ONLY;
		ime_drv_fmt->out_format_type_sel = IME_OUTPUT_YCC_PLANAR;
		break;

	default:
		//DBG_ERR("KDRV IME: don't support this output format %d\r\n", dal_out_type);
		return E_NOSPT;
		break;
	}

	return E_OK;
}

static ER kdrv_ime_int_scaler_trans(KDRV_IME_SCALER dal_scl, IME_SCALE_METHOD_SEL *drv_scl)
{
	switch (dal_scl) {
	case KDRV_IME_SCALER_BICUBIC:
		*drv_scl = IMEALG_BICUBIC;
		break;

	case KDRV_IME_SCALER_BILINEAR:
		*drv_scl = IMEALG_BILINEAR;
		break;

	case KDRV_IME_SCALER_NEAREST:
		*drv_scl = IMEALG_NEAREST;
		break;

	case KDRV_IME_SCALER_INTEGRATION:
		*drv_scl = IMEALG_INTEGRATION;
		break;

	default:
		*drv_scl = IMEALG_BILINEAR;
		break;
	}

	return E_OK;
}

static IME_SCALE_METHOD_SEL kdrv_ime_int_scale_method_sel(UINT32 in_h, UINT32 in_v, UINT32 out_h, UINT32 out_v, UINT32 strp_h_max, KDRV_IME_SCL_METHOD_SEL_INFO scl_sel)
{
	IME_STRIPE_CAL_INFO cal_stp_info = {{0, 0}};
	IME_STRIPE_INFO stp_info_h = {0};
	UINT32 out_strp = 0;
	KDRV_IME_SCALER dal_scl_method = KDRV_IME_SCALER_BILINEAR;
	IME_SCALE_METHOD_SEL scl_method = IMEALG_BILINEAR;
	UINT32 th_in, th_out, th_ratio;

	if (((in_h > out_h) && ((out_h * KDRV_IME_SCALING_MAX_RATIO) < (in_h * KDRV_IME_SCALE_BASE))) ||
		((in_v > out_v) && ((out_v * KDRV_IME_SCALING_MAX_RATIO) < (in_v * KDRV_IME_SCALE_BASE))) ||
		((in_h < out_h) && ((in_h * KDRV_IME_SCALING_MAX_RATIO) < (out_h * KDRV_IME_SCALE_BASE))) ||
		((in_v < out_v) && ((in_v * KDRV_IME_SCALING_MAX_RATIO) < (out_v * KDRV_IME_SCALE_BASE)))) {
		DBG_ERR("KDRV IME: Scale ratio over than 31.99, in_size(%d, %d), out_size(%d, %d)\r\n", (int)in_h, (int)in_v, (int)out_h, (int)out_v);
		return scl_method;
	}

	/* retrieve threshold information */
	th_in = scl_sel.scl_th & 0x0000FFFF;
	th_out = (scl_sel.scl_th & 0xFFFF0000) >> 16;

	if (th_in == 0) {
		th_in = 1;
	}

	if (th_out == 0) {
		th_out = 1;
	}

	if (th_in >= th_out) {
		th_ratio = (th_in * KDRV_IME_SCALE_BASE) / th_out;
	} else {
		th_ratio = (th_out * KDRV_IME_SCALE_BASE) / th_in;
	}

	/* scale down */
	if ((in_h >= out_h) && (in_v >= out_v)) {
		if (((out_h * KDRV_IME_SCALING_ISD_MAX_RATIO) > (in_h * KDRV_IME_SCALE_BASE)) && ((out_v * KDRV_IME_SCALING_ISD_MAX_RATIO) > (in_v * KDRV_IME_SCALE_BASE))) {
			if (strp_h_max == 0) {
				cal_stp_info.in_size.size_h = in_h;
				cal_stp_info.in_size.size_v = in_v;
				//cal_stp_info.SsrEn = IME_FUNC_DISABLE;
				ime_cal_d2d_hstripe_size(&cal_stp_info, &stp_info_h);

				if (stp_info_h.stp_m == 1) {
					strp_h_max = stp_info_h.stp_l;
				} else if (stp_info_h.stp_m > 1) {
					strp_h_max = stp_info_h.stp_n;
				} else {
					DBG_ERR("KDRV IME: cal H Stripe Size fail in_size(%d, %d) N/L/M(%d/%d/%d)\r\n", (int)cal_stp_info.in_size.size_h,
							(int)cal_stp_info.in_size.size_v, (int)stp_info_h.stp_n, (int)stp_info_h.stp_l, (int)stp_info_h.stp_m);
				}

				if ((in_v > out_v) && (in_h > out_h)) {
					if (strp_h_max == 0) {
						strp_h_max = in_h;
					}
				}
			}

			/* scale method select base on threshold */
			if (th_in >= th_out) {
				UINT32 ratio = (in_h * KDRV_IME_SCALE_BASE) / out_h;

				if (ratio >= th_ratio) {
					dal_scl_method = scl_sel.method_l;
				} else {
					dal_scl_method = scl_sel.method_h;
				}
			} else {
				dal_scl_method = scl_sel.method_l;
			}

			/* check limitation */
			if (dal_scl_method == KDRV_IME_SCALER_INTEGRATION || dal_scl_method == KDRV_IME_SCALER_AUTO) {
				out_strp = (((strp_h_max - 1) * (out_h - 1) / (in_h - 1)) + 1);
				if (out_strp <= kdrv_ime_scaling_isd_stp_out_h_limit) {
					//scl_method = IMEALG_INTEGRATION;
					dal_scl_method = KDRV_IME_SCALER_INTEGRATION;
				} else {
					dal_scl_method = KDRV_IME_SCALER_BILINEAR;
				}
			}
		}
	} else {
		/* scale method select base on threshold */
		if (th_in < out_h) {
			UINT32 ratio = (out_h * KDRV_IME_SCALE_BASE) / in_h;

			if (ratio >= th_ratio) {
				dal_scl_method = scl_sel.method_l;
			} else {
				dal_scl_method = scl_sel.method_h;
			}
		} else {
			dal_scl_method = scl_sel.method_h;
		}

		/* check limitation */
		if (dal_scl_method == KDRV_IME_SCALER_INTEGRATION || dal_scl_method == KDRV_IME_SCALER_AUTO) {
			dal_scl_method = KDRV_IME_SCALER_BILINEAR;
		}
	}

	kdrv_ime_int_scaler_trans(dal_scl_method, &scl_method);
	return scl_method;
}

static ER kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID path_id, KDRV_IME_PRAM *p_dal_param, IME_OUTPATH_INFO *p_out_path_info)
{
	ER rt = E_OK;

	if (p_dal_param == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	if (p_out_path_info == NULL) {
		DBG_ERR("KDRV IME: set output path parameter NULL...\r\n");
		return E_PAR;
	}


	p_out_path_info->out_path_enable = (IME_FUNC_EN)p_dal_param->out_path_info[path_id].path_en;
	p_out_path_info->out_path_dram_enable = (IME_FUNC_EN)p_dal_param->out_path_info[path_id].path_en;
	if (p_out_path_info->out_path_enable == IME_FUNC_ENABLE) {
		//p_out_path_info->out_path_out_dest = IME_OUTDST_DRAM;

		//DBG_DUMP("path_id: %d\r\n", path_id);

		p_out_path_info->out_path_dma_flush = (IME_BUF_FLUSH_SEL)p_dal_param->out_path_info[path_id].out_dma_flush;

		rt = kdrv_ime_int_out_yuvfmt_trans(p_dal_param->out_path_info[path_id].type, &p_out_path_info->out_path_image_format);
		if (rt != E_OK) {
			return rt;
		}
		p_out_path_info->out_path_scale_size.size_h = p_dal_param->out_path_info[path_id].ch[KDRV_IPP_YUV_Y].width;
		p_out_path_info->out_path_scale_size.size_v = p_dal_param->out_path_info[path_id].ch[KDRV_IPP_YUV_Y].height;

		//DBG_DUMP("out_path_scale_size.size_h: %d\r\n", p_out_path_info->out_path_scale_size.size_h);
		//DBG_DUMP("out_path_scale_size.size_v: %d\r\n", p_out_path_info->out_path_scale_size.size_v);

		p_out_path_info->out_path_crop_pos.pos_x = p_dal_param->out_path_info[path_id].crp_window.x;
		p_out_path_info->out_path_crop_pos.pos_y = p_dal_param->out_path_info[path_id].crp_window.y;
		p_out_path_info->out_path_out_size.size_h = p_dal_param->out_path_info[path_id].crp_window.w;
		p_out_path_info->out_path_out_size.size_v = p_dal_param->out_path_info[path_id].crp_window.h;
		p_out_path_info->out_path_out_lineoffset.lineoffset_y = p_dal_param->out_path_info[path_id].ch[KDRV_IPP_YUV_Y].line_ofs;
		p_out_path_info->out_path_out_lineoffset.lineoffset_cb = p_dal_param->out_path_info[path_id].ch[KDRV_IPP_YUV_U].line_ofs;

		//DBG_DUMP("out_path_out_lineoffset.lineoffset_y: %d\r\n", p_out_path_info->out_path_out_lineoffset.lineoffset_y);
		//DBG_DUMP("out_path_out_lineoffset.lineoffset_cb: %d\r\n", p_out_path_info->out_path_out_lineoffset.lineoffset_cb);

		p_out_path_info->out_path_clamp.min_y = p_dal_param->out_path_info[path_id].out_range_clamp.min_y;
		p_out_path_info->out_path_clamp.min_uv = p_dal_param->out_path_info[path_id].out_range_clamp.min_uv;
		p_out_path_info->out_path_clamp.max_y = p_dal_param->out_path_info[path_id].out_range_clamp.max_y;
		p_out_path_info->out_path_clamp.max_uv = p_dal_param->out_path_info[path_id].out_range_clamp.max_uv;

		p_out_path_info->out_path_flip_enable = (IME_FUNC_EN)p_dal_param->out_path_info[path_id].flip_en;

		if (p_out_path_info->out_path_flip_enable == IME_FUNC_DISABLE) {
			p_out_path_info->out_path_addr.addr_y = p_dal_param->out_path_info[path_id].pixel_addr.addr_y;
			p_out_path_info->out_path_addr.addr_cb = p_dal_param->out_path_info[path_id].pixel_addr.addr_cb;
			p_out_path_info->out_path_addr.addr_cr = p_dal_param->out_path_info[path_id].pixel_addr.addr_cr;
		} else {
			UINT32 flip_size;

			flip_size = (p_out_path_info->out_path_out_lineoffset.lineoffset_y * (p_out_path_info->out_path_out_size.size_v - 1));
			p_out_path_info->out_path_addr.addr_y  = p_dal_param->out_path_info[path_id].pixel_addr.addr_y + flip_size;

			if ((p_out_path_info->out_path_image_format.out_format_sel == IME_OUTPUT_YCC_420) && (p_out_path_info->out_path_image_format.out_format_type_sel == IME_OUTPUT_YCC_PLANAR)) {

				flip_size = (p_out_path_info->out_path_out_lineoffset.lineoffset_cb * ((p_out_path_info->out_path_out_size.size_v >> 1) - 1));

				p_out_path_info->out_path_addr.addr_cb = p_dal_param->out_path_info[path_id].pixel_addr.addr_cb + flip_size;
				p_out_path_info->out_path_addr.addr_cr = p_dal_param->out_path_info[path_id].pixel_addr.addr_cr + flip_size;
			}

			if ((p_out_path_info->out_path_image_format.out_format_sel == IME_OUTPUT_YCC_420) && (p_out_path_info->out_path_image_format.out_format_type_sel == IME_OUTPUT_YCC_UVPACKIN)) {
				flip_size = (p_out_path_info->out_path_out_lineoffset.lineoffset_cb * ((p_out_path_info->out_path_out_size.size_v >> 1) - 1));
				p_out_path_info->out_path_addr.addr_cb = p_dal_param->out_path_info[path_id].pixel_addr.addr_cb + flip_size;
			}
		}

		//Output Separate Parameters
		p_out_path_info->out_path_sprtout_enable = (IME_FUNC_EN)p_dal_param->out_path_info[path_id].out_sprt_info.enable;
		if (p_out_path_info->out_path_sprtout_enable == IME_FUNC_ENABLE) {
			p_out_path_info->out_path_sprt_pos = p_dal_param->out_path_info[path_id].out_sprt_info.sprt_pos;

			p_out_path_info->out_path_out_lineoffset2.lineoffset_y = p_dal_param->out_path_info[path_id].out_sprt_info.sprt_out_line_ofs_y;
			p_out_path_info->out_path_out_lineoffset2.lineoffset_cb = p_dal_param->out_path_info[path_id].out_sprt_info.sprt_out_line_ofs_cb;

			if (p_out_path_info->out_path_flip_enable == IME_FUNC_DISABLE) {
				p_out_path_info->out_path_addr2.addr_y = p_dal_param->out_path_info[path_id].out_sprt_addr.addr_y;
				p_out_path_info->out_path_addr2.addr_cb = p_dal_param->out_path_info[path_id].out_sprt_addr.addr_cb;
				p_out_path_info->out_path_addr2.addr_cr = p_dal_param->out_path_info[path_id].out_sprt_addr.addr_cr;
			} else {
				UINT32 flip_size;

				flip_size = p_out_path_info->out_path_out_lineoffset2.lineoffset_y * ((p_out_path_info->out_path_out_size.size_v >> 1) - 1);
				p_out_path_info->out_path_addr2.addr_y = p_dal_param->out_path_info[path_id].out_sprt_addr.addr_y + flip_size;


				if ((p_out_path_info->out_path_image_format.out_format_sel == IME_OUTPUT_YCC_420) && (p_out_path_info->out_path_image_format.out_format_type_sel == IME_OUTPUT_YCC_PLANAR)) {
					flip_size = (p_out_path_info->out_path_out_lineoffset2.lineoffset_cb * ((p_out_path_info->out_path_out_size.size_v >> 1) - 1));

					p_out_path_info->out_path_addr2.addr_cb = p_dal_param->out_path_info[path_id].out_sprt_addr.addr_cb + flip_size;
					p_out_path_info->out_path_addr2.addr_cr = p_dal_param->out_path_info[path_id].out_sprt_addr.addr_cr + flip_size;
				}

				if ((p_out_path_info->out_path_image_format.out_format_sel == IME_OUTPUT_YCC_420) && (p_out_path_info->out_path_image_format.out_format_type_sel == IME_OUTPUT_YCC_UVPACKIN)) {
					flip_size = (p_out_path_info->out_path_out_lineoffset2.lineoffset_cb * ((p_out_path_info->out_path_out_size.size_v >> 1) - 1));

					p_out_path_info->out_path_addr2.addr_cb = p_dal_param->out_path_info[path_id].out_sprt_addr.addr_cb + flip_size;
				}
			}


		}

		//NT96680 Only Path1 support Output Encode
		if (path_id == KDRV_IME_PATH_ID_1) {
			p_out_path_info->out_path_encode_enable = (IME_FUNC_EN)p_dal_param->out_path_info[path_id].out_encode_info.enable;

			p_out_path_info->out_path_encode_smode_enable = IME_FUNC_DISABLE;

#if 0  // 520 compression is a lossy method, which do not need additional parameters
			if (p_out_path_info->OutPathEncode.EncodeEn == IME_FUNC_ENABLE) {
				if (p_out_path_info->out_path_sprtout_enable == IME_FUNC_ENABLE) {
					DBG_WRN("KDRV IME: KDRV_IME_PATH_ID_%d, not support Enable Both YCC Encode and Output Sprt\r\n", path_id + 1);
					p_out_path_info->OutPathEncode.EncodeEn = IME_FUNC_DISABLE;
				} else {
					p_out_path_info->OutPathEncode.EncodeSInfoFmt = (IME_SINFO_FMT)p_dal_param->out_path_info[path_id].out_encode_info.enc_side_info_fmt;
					p_out_path_info->OutPathEncode.EncodeClearKTabEn = IME_FUNC_DISABLE;
					p_out_path_info->OutPathEncode.EncodeUpdateKTabEn = IME_FUNC_ENABLE;
					p_out_path_info->OutPathEncode.uiEncodeSideInfoAddr = p_dal_param->out_path_info[path_id].out_encode_info.enc_side_info_addr;
					p_out_path_info->OutPathEncode.EncodeKTab.uiKt0 = p_dal_param->out_path_info[path_id].out_encode_info.k_tbl0;
					p_out_path_info->OutPathEncode.EncodeKTab.uiKt1 = p_dal_param->out_path_info[path_id].out_encode_info.k_tbl1;
					p_out_path_info->OutPathEncode.EncodeKTab.uiKt2 = p_dal_param->out_path_info[path_id].out_encode_info.k_tbl2;
				}
			}
#endif
		} else {
			p_out_path_info->out_path_encode_enable = IME_FUNC_DISABLE;
			p_out_path_info->out_path_encode_smode_enable = IME_FUNC_DISABLE;
		}

		//NT96680 IME path4 only support bicubic scale
		if (path_id == KDRV_IME_PATH_ID_4) {
			p_out_path_info->out_path_scale_method = IMEALG_BICUBIC;
		} else if (path_id == KDRV_IME_PATH_ID_1) {
			p_out_path_info->out_path_scale_method = IMEALG_BICUBIC;
		} else {
			UINT32 h_strp_max;

			if (p_dal_param->in_img_info.in_src == KDRV_IME_IN_SRC_DIRECT) {
				if (p_dal_param->extend_info.stripe_h_max == 0) {
					h_strp_max = kdrv_ime_scaling_isd_stp_out_h_limit;
					DBG_WRN("KDRV IME: h_stripe_max=%d, Need Input stripe info when IME in Direct Mode", (int)h_strp_max);
					//return E_SYS;
				} else {
					h_strp_max = p_dal_param->extend_info.stripe_h_max;
				}
			} else if (p_dal_param->in_img_info.in_src == KDRV_IME_IN_SRC_ALL_DIRECT) {
				if (p_dal_param->extend_info.stripe_h_max == 0) {
					h_strp_max = kdrv_ime_scaling_isd_stp_out_h_limit;
					DBG_WRN("KDRV IME: h_stripe_max=%d, Need Input stripe info when IME in All Direct Mode", (int)h_strp_max);
					//return E_SYS;
				} else if (p_dal_param->extend_info.stripe_h_max > kdrv_ime_stp_max) {
					h_strp_max = kdrv_ime_stp_max;
					DBG_WRN("KDRV IME: h_stripe_max=%d, Input stripe overflow when IME in All Direct Mode", (int)p_dal_param->extend_info.stripe_h_max);
					//return E_SYS;
				} else {
					h_strp_max = p_dal_param->extend_info.stripe_h_max;
				}

			} else {
				if (p_dal_param->extend_info.stripe_h_max == 0) {
					h_strp_max = kdrv_ime_scaling_isd_stp_out_h_limit;
					//DBG_WRN("KDRV IME: h_stripe_max=%d, Need Input stripe info when IME in Dram to Dram Mode", (int)h_strp_max);
					//return E_SYS;
				} else if (p_dal_param->extend_info.stripe_h_max > kdrv_ime_stp_max) {
					h_strp_max = kdrv_ime_stp_max;
					//DBG_WRN("KDRV IME: h_stripe_max=%d, Input stripe overflow when IME in Dram to Dram Mode", (int)p_dal_param->extend_info.stripe_h_max);
					//return E_SYS;
				} else {
					h_strp_max = p_dal_param->extend_info.stripe_h_max;
				}

			}

			p_out_path_info->out_path_scale_method = kdrv_ime_int_scale_method_sel(p_dal_param->in_img_info.ch[KDRV_IPP_YUV_Y].width,
					p_dal_param->in_img_info.ch[KDRV_IPP_YUV_Y].height,
					p_dal_param->out_path_info[path_id].ch[KDRV_IPP_YUV_Y].width,
					p_dal_param->out_path_info[path_id].ch[KDRV_IPP_YUV_Y].height, h_strp_max,
					p_dal_param->out_scl_method_sel);
		}
		p_out_path_info->out_path_scale_factors.CalScaleFactorMode = IME_SCALE_FACTOR_COEF_AUTO_MODE;
		p_out_path_info->out_path_scale_filter.CoefCalMode = IME_SCALE_FILTER_COEF_AUTO_MODE;
	}

	return rt;
}

static ER kdrv_ime_int_set_lca_info(KDRV_IME_PRAM *pkdrv_ime_par, IME_CHROMA_ADAPTION_INFO *pIme_lca_info)
{
	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set lca parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_lca_info == NULL) {
		DBG_ERR("KDRV IME: set lca parameter NULL...\r\n");
		return E_PAR;
	}

	//pIme_lca_info->lca_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.lca.enable;
	pIme_lca_info->lca_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.lca_en_ctrl.enable;
	if (pIme_lca_info->lca_enable == IME_FUNC_ENABLE) {
		//LCA IQ Chroma info
		pIme_lca_info->lca_iq_chroma_info.lca_ref_y_wet = pkdrv_ime_par->sub_modules.lca.chroma.ref_y_wt;
		pIme_lca_info->lca_iq_chroma_info.lca_ref_uv_wet = pkdrv_ime_par->sub_modules.lca.chroma.ref_uv_wt;
		pIme_lca_info->lca_iq_chroma_info.lca_out_uv_wet = pkdrv_ime_par->sub_modules.lca.chroma.out_uv_wt;
		pIme_lca_info->lca_iq_chroma_info.lca_y_range = pkdrv_ime_par->sub_modules.lca.chroma.y_rng;
		pIme_lca_info->lca_iq_chroma_info.lca_y_wet_prc = pkdrv_ime_par->sub_modules.lca.chroma.y_wt_prc;
		pIme_lca_info->lca_iq_chroma_info.lca_y_th = pkdrv_ime_par->sub_modules.lca.chroma.y_th;
		pIme_lca_info->lca_iq_chroma_info.lca_y_wet_start = pkdrv_ime_par->sub_modules.lca.chroma.y_wt_s;
		pIme_lca_info->lca_iq_chroma_info.lca_y_wet_end = pkdrv_ime_par->sub_modules.lca.chroma.y_wt_e;

		pIme_lca_info->lca_iq_chroma_info.lca_uv_range = pkdrv_ime_par->sub_modules.lca.chroma.uv_rng;
		pIme_lca_info->lca_iq_chroma_info.lca_uv_wet_prc = pkdrv_ime_par->sub_modules.lca.chroma.uv_wt_prc;
		pIme_lca_info->lca_iq_chroma_info.lca_uv_th = pkdrv_ime_par->sub_modules.lca.chroma.uv_th;
		pIme_lca_info->lca_iq_chroma_info.lca_uv_wet_start = pkdrv_ime_par->sub_modules.lca.chroma.uv_wt_s;
		pIme_lca_info->lca_iq_chroma_info.lca_uv_wet_end = pkdrv_ime_par->sub_modules.lca.chroma.uv_wt_e;

		//LCA IQ Luma info
		pIme_lca_info->lca_iq_luma_info.lca_la_enable = pkdrv_ime_par->sub_modules.lca.luma.enable;
		pIme_lca_info->lca_iq_luma_info.lca_la_ref_wet = pkdrv_ime_par->sub_modules.lca.luma.ref_wt;
		pIme_lca_info->lca_iq_luma_info.lca_la_out_wet = pkdrv_ime_par->sub_modules.lca.luma.out_wt;
		pIme_lca_info->lca_iq_luma_info.lca_la_range = pkdrv_ime_par->sub_modules.lca.luma.rng;
		pIme_lca_info->lca_iq_luma_info.lca_la_wet_prc = pkdrv_ime_par->sub_modules.lca.luma.wt_prc;
		pIme_lca_info->lca_iq_luma_info.lca_la_th = pkdrv_ime_par->sub_modules.lca.luma.th;
		pIme_lca_info->lca_iq_luma_info.lca_la_wet_start = pkdrv_ime_par->sub_modules.lca.luma.wt_s;
		pIme_lca_info->lca_iq_luma_info.lca_la_wet_end = pkdrv_ime_par->sub_modules.lca.luma.wt_e;

		//LCA Chroma Adjust info
		pIme_lca_info->lca_chroma_adj_info.lca_ca_enable = pkdrv_ime_par->sub_modules.lca.ca.enable;
		pIme_lca_info->lca_chroma_adj_info.lca_ca_cent_u = pkdrv_ime_par->sub_modules.lca_ca_cent.cent_u;
		pIme_lca_info->lca_chroma_adj_info.lca_ca_cent_v = pkdrv_ime_par->sub_modules.lca_ca_cent.cent_v;
		pIme_lca_info->lca_chroma_adj_info.lca_ca_uv_range = pkdrv_ime_par->sub_modules.lca.ca.uv_rng;
		pIme_lca_info->lca_chroma_adj_info.lca_ca_uv_wet_prc = pkdrv_ime_par->sub_modules.lca.ca.uv_wt_prc;
		pIme_lca_info->lca_chroma_adj_info.lca_ca_uv_th = pkdrv_ime_par->sub_modules.lca.ca.uv_th;
		pIme_lca_info->lca_chroma_adj_info.lca_ca_uv_wet_start = pkdrv_ime_par->sub_modules.lca.ca.uv_wt_s;
		pIme_lca_info->lca_chroma_adj_info.lca_ca_uv_wet_end = pkdrv_ime_par->sub_modules.lca.ca.uv_wt_e;

		//LCA ref image
		pIme_lca_info->lca_image_info.lca_dma_addr0.addr_y = pkdrv_ime_par->sub_modules.lca_img.dma_addr;
		pIme_lca_info->lca_image_info.lca_img_size.size_h = pkdrv_ime_par->sub_modules.lca_img.img_size.width;
		pIme_lca_info->lca_image_info.lca_img_size.size_v = pkdrv_ime_par->sub_modules.lca_img.img_size.height;
		pIme_lca_info->lca_image_info.lca_lofs.lineoffset_y = pkdrv_ime_par->sub_modules.lca_img.img_size.line_ofs;
		pIme_lca_info->lca_image_info.lca_fmt = IME_LCAF_YCCYCC;
		if (pkdrv_ime_par->sub_modules.lca_img.in_src == KDRV_IME_LCA_IN_SRC_DIRECT) {
			pIme_lca_info->lca_image_info.lca_src = IME_LCA_SRC_IFE2;
		} else {
			pIme_lca_info->lca_image_info.lca_src = IME_LCA_SRC_DRAM;
		}

		pIme_lca_info->lca_bypass_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.lca_en_ctrl.bypass;
		//if (pIme_lca_info->lca_bypass_enable == ENABLE) {
		//  pIme_lca_info->lca_iq_luma_info.LcaLaEnable = DISABLE;
		//  pIme_lca_info->lca_chroma_adj_info.LcaCaEnable  = DISABLE;
		//  pIme_lca_info->lca_iq_chroma_info.LcaOutUVWt = 0;
		//  pIme_lca_info->lca_iq_luma_info.LcaLaOutWt = 0;
		//}
	}

	return E_OK;
}

// KDRV_IME_IN_INFO *pdal_img_in_info
static ER kdrv_ime_int_set_lca_sub_out_info(KDRV_IME_PRAM *pkdrv_ime_par, KDRV_IME_LCA_SUBOUT_INFO *pdal_lca_sub_out_info, IME_CHROMA_ADAPTION_SUBOUT_INFO *pIme_lca_sub_out_info)
{
	UINT32 scale_down_factor = 0;
	ER rt = E_OK;

	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set lca subout image parameter NULL...\r\n");
		return E_PAR;
	}

	if (pdal_lca_sub_out_info == NULL) {
		DBG_ERR("KDRV IME: set lca subout image parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_lca_sub_out_info == NULL) {
		DBG_ERR("KDRV IME: set lca subout image parameter NULL...\r\n");
		return E_PAR;
	}

	pIme_lca_sub_out_info->lca_subout_enable = (IME_FUNC_EN)pdal_lca_sub_out_info->enable;
	if (pIme_lca_sub_out_info->lca_subout_enable == IME_FUNC_ENABLE) {

#if (KDRV_IME_PQ_NEW == 0)
		pIme_lca_sub_out_info->lca_subout_src = (IME_LCA_SUBOUT_SRC)pdal_lca_sub_out_info->sub_out_src;
#endif

#if (KDRV_IME_PQ_NEW == 1)
		pIme_lca_sub_out_info->lca_subout_src = (IME_LCA_SUBOUT_SRC)pkdrv_ime_par->sub_modules.lca.sub_out_src;
#endif

		pIme_lca_sub_out_info->lca_subout_addr.addr_y = pdal_lca_sub_out_info->sub_out_addr;
		pIme_lca_sub_out_info->lca_subout_lofs.lineoffset_y = pdal_lca_sub_out_info->sub_out_size.line_ofs;

		//pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_h = pdal_img_in_info->ch[KDRV_IPP_YUV_Y].width;
		//pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_v = pdal_img_in_info->ch[KDRV_IPP_YUV_Y].height;
		pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_h = pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].width;
		pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_v = pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].height;
		pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_h = pdal_lca_sub_out_info->sub_out_size.width;
		pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_v = pdal_lca_sub_out_info->sub_out_size.height;
		pIme_lca_sub_out_info->lca_subout_scale_info.subout_scale_factor.CalScaleFactorMode = IME_SCALE_FACTOR_COEF_AUTO_MODE;

		if ((pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_h < pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_h) || (pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_h % 4 != 0)) {
			DBG_ERR("KDRV IME: LCA Sub out size Width error src_width: %d, sub_out_width: %d\r\n", (int)pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_h, (int)pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_h);
			DBG_ERR("KDRV IME: src_width need >= sub_out_width, and sub_oub_width must be multiple of 4\r\n");
			rt = E_PAR;
		}

		if ((pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_v < pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_v) || (pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_v % 2 != 0)) {
			DBG_ERR("KDRV IME: LCA Sub out size Height error src_height: %d < sub_out_height: %d\r\n", (int)pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_v, (int)pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_v);
			DBG_ERR("KDRV IME: src_height need >= sub_out_height, and sub_oub_height must be multiple of 2\r\n");
			rt = E_PAR;
		}

		scale_down_factor = ((pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_h - 1) * KDRV_IME_SCALE_BASE) / (pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_h - 1);
		if (scale_down_factor <= KDRV_IME_LCA_SUB_OUT_H_SCALE_LIMIT) {
			//DBG_ERR("KDRV IME: LCA subout horizontal scale down ratio must > 2.07x, src_width: %d, sub_out_width: %d\r\n", pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_h, pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_h);
			DBG_ERR("KDRV IME: LCA subout horizontal scale down ratio must > 2.0x, src_width: %d, sub_out_width: %d\r\n", (int)pIme_lca_sub_out_info->lca_subout_scale_info.pri_img_size.size_h, (int)pIme_lca_sub_out_info->lca_subout_scale_info.ref_img_size.size_h);
			rt = E_PAR;
		}

		if (rt != E_OK) {
			pIme_lca_sub_out_info->lca_subout_enable = IME_FUNC_DISABLE;
		}
	}

	return rt;
}

static ER kdrv_ime_int_set_dbcs_info(KDRV_IME_DBCS_PARAM *pdal_dbcs_info, IME_DBCS_INFO *pIme_dbcs_info)
{
	if (pdal_dbcs_info == NULL) {
		DBG_ERR("KDRV IME: set dbcs parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_dbcs_info == NULL) {
		DBG_ERR("KDRV IME: set dbcs parameter NULL...\r\n");
		return E_PAR;
	}

	//pIme_dbcs_info->dbcs_enable = (IME_FUNC_EN)pdal_dbcs_info->enable;
	if (pIme_dbcs_info->dbcs_enable == IME_FUNC_ENABLE) {
		pIme_dbcs_info->dbcs_iq_info.OpMode = pdal_dbcs_info->op_mode;
		pIme_dbcs_info->dbcs_iq_info.uiCentU = pdal_dbcs_info->cent_u;
		pIme_dbcs_info->dbcs_iq_info.uiCentV = pdal_dbcs_info->cent_v;
		pIme_dbcs_info->dbcs_iq_info.uiStepY = pdal_dbcs_info->step_y;
		pIme_dbcs_info->dbcs_iq_info.uiStepC = pdal_dbcs_info->step_c;
		pIme_dbcs_info->dbcs_iq_info.puiWtY = pdal_dbcs_info->wt_y;
		pIme_dbcs_info->dbcs_iq_info.puiWtC = pdal_dbcs_info->wt_c;
	}

	return E_OK;
}

static ER kdrv_ime_int_set_ssr_info(KDRV_IME_SSR_PARAM *pdal_ssr_info, IME_SSR_INFO *pIme_ssr_info)
{
#if 0
	if (pdal_ssr_info == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_ssr_info == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	pIme_ssr_info->SsrEnable = (IME_FUNC_EN)pdal_ssr_info->enable;
	if (pdal_ssr_info->enable == ENABLE) {
		if (pdal_ssr_info->auto_mode_en == ENABLE) {
			pIme_ssr_info->SsrMode = IME_SSR_MODE_AUTO;
		} else {
			pIme_ssr_info->SsrMode = IME_SSR_MODE_USER;
		}
		pIme_ssr_info->SsrIqInfo.uiDTh = pdal_ssr_info->diag_th;
		pIme_ssr_info->SsrIqInfo.uiHVTh = pdal_ssr_info->h_v_th;
	}
#endif

	//DBG_ERR("KDRV IME: don't support SSR function...\r\n");

	return E_OK;
}

static ER kdrv_ime_int_set_film_grain_info(KDRV_IME_FGN_PARAM *pdal_fg_info, IME_FILM_GRAIN_INFO *pIme_fg_info)
{
#if 0
	if (pdal_fg_info == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_fg_info == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	pIme_fg_info->FgnEnable = (IME_FUNC_EN)pdal_fg_info->enable;
	if (pdal_fg_info->enable == ENABLE) {
		pIme_fg_info->FgnIqInfo.uiFgnLumTh = pdal_fg_info->lum_th;
		pIme_fg_info->FgnIqInfo.uiFgnNLP1 = pdal_fg_info->nl_p1;
		pIme_fg_info->FgnIqInfo.uiFgnInitP1 = pdal_fg_info->init_p1;
		pIme_fg_info->FgnIqInfo.uiFgnNLP2 = pdal_fg_info->nl_p2;
		pIme_fg_info->FgnIqInfo.uiFgnInitP2 = pdal_fg_info->init_p2;
		pIme_fg_info->FgnIqInfo.uiFgnNLP3 = pdal_fg_info->nl_p3;
		pIme_fg_info->FgnIqInfo.uiFgnInitP3 = pdal_fg_info->init_p3;
		pIme_fg_info->FgnIqInfo.uiFgnNLP5 = pdal_fg_info->nl_p5;
		pIme_fg_info->FgnIqInfo.uiFgnInitP5 = pdal_fg_info->init_p5;
	}
#endif

	//DBG_ERR("KDRV IME: don't support SSR function...\r\n");

	return E_OK;
}

static ER kdrv_ime_int_set_ds_info(KDRV_IME_PRAM *pkdrv_ime_par, IME_STAMP_INFO *pIme_ds_info)
{
	UINT32 i;

	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set osd parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_ds_info == NULL) {
		DBG_ERR("KDRV IME: set osd parameter NULL...\r\n");
		return E_PAR;
	}

	//Set 0
	pIme_ds_info->ds_set0.ds_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].enable;
	if (pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].enable == ENABLE) {
		pIme_ds_info->ds_set0.ds_image_info.ds_img_size.size_h = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].image.img_size.w;
		pIme_ds_info->ds_set0.ds_image_info.ds_img_size.size_v = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].image.img_size.h;
		pIme_ds_info->ds_set0.ds_image_info.ds_fmt = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].image.img_fmt;
		pIme_ds_info->ds_set0.ds_image_info.ds_pos.pos_x = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].image.pos.x;
		pIme_ds_info->ds_set0.ds_image_info.ds_pos.pos_y = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].image.pos.y;
		pIme_ds_info->ds_set0.ds_image_info.ds_lofs = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].image.ime_lofs;
		pIme_ds_info->ds_set0.ds_image_info.ds_addr = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].image.img_addr;

		pIme_ds_info->ds_set0.ds_iq_info.ds_ck_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.color_key_en;
		//pIme_ds_info->ds_set0.ds_iq_info.uiDsCk = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.color_key_val;
		pIme_ds_info->ds_set0.ds_iq_info.ds_bld_wet0 = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.bld_wt_0;
		pIme_ds_info->ds_set0.ds_iq_info.ds_bld_wet1 = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.bld_wt_1;

		pIme_ds_info->ds_set0.ds_iq_info.ds_plt_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.plt_en;
		pIme_ds_info->ds_set0.ds_iq_info.ds_ck_mode = (IME_DS_CK_MODE_SEL)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.color_key_mode;
		pIme_ds_info->ds_set0.ds_iq_info.ds_ck_a = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.color_key_val[0];  ///< Color key for alpha channel
		pIme_ds_info->ds_set0.ds_iq_info.ds_ck_r = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.color_key_val[1];  ///< Color key for R channel
		pIme_ds_info->ds_set0.ds_iq_info.ds_ck_g = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.color_key_val[2];  ///< Color key for G channel
		pIme_ds_info->ds_set0.ds_iq_info.ds_ck_b = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].ds_iq.color_key_val[3];  ///< Color key for B channel

	}

	//Set 1
	pIme_ds_info->ds_set1.ds_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].enable;
	if (pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].enable == ENABLE) {
		pIme_ds_info->ds_set1.ds_image_info.ds_img_size.size_h = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].image.img_size.w;
		pIme_ds_info->ds_set1.ds_image_info.ds_img_size.size_v = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].image.img_size.h;
		pIme_ds_info->ds_set1.ds_image_info.ds_fmt = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].image.img_fmt;
		pIme_ds_info->ds_set1.ds_image_info.ds_pos.pos_x = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].image.pos.x;
		pIme_ds_info->ds_set1.ds_image_info.ds_pos.pos_y = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].image.pos.y;
		pIme_ds_info->ds_set1.ds_image_info.ds_lofs = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].image.ime_lofs;
		pIme_ds_info->ds_set1.ds_image_info.ds_addr = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].image.img_addr;

		pIme_ds_info->ds_set1.ds_iq_info.ds_ck_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.color_key_en;
		//pIme_ds_info->ds_set1.ds_iq_info.uiDsCk = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.color_key_val;
		pIme_ds_info->ds_set1.ds_iq_info.ds_bld_wet0 = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.bld_wt_0;
		pIme_ds_info->ds_set1.ds_iq_info.ds_bld_wet1 = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.bld_wt_1;

		pIme_ds_info->ds_set1.ds_iq_info.ds_plt_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.plt_en;
		pIme_ds_info->ds_set1.ds_iq_info.ds_ck_mode = (IME_DS_CK_MODE_SEL)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.color_key_mode;
		pIme_ds_info->ds_set1.ds_iq_info.ds_ck_a = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.color_key_val[0];  ///< Color key for alpha channel
		pIme_ds_info->ds_set1.ds_iq_info.ds_ck_r = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.color_key_val[1];  ///< Color key for R channel
		pIme_ds_info->ds_set1.ds_iq_info.ds_ck_g = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.color_key_val[2];  ///< Color key for G channel
		pIme_ds_info->ds_set1.ds_iq_info.ds_ck_b = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].ds_iq.color_key_val[3];  ///< Color key for B channel
	}

	//Set 2
	pIme_ds_info->ds_set2.ds_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].enable;
	if (pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].enable == ENABLE) {
		pIme_ds_info->ds_set2.ds_image_info.ds_img_size.size_h = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].image.img_size.w;
		pIme_ds_info->ds_set2.ds_image_info.ds_img_size.size_v = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].image.img_size.h;
		pIme_ds_info->ds_set2.ds_image_info.ds_fmt = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].image.img_fmt;
		pIme_ds_info->ds_set2.ds_image_info.ds_pos.pos_x = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].image.pos.x;
		pIme_ds_info->ds_set2.ds_image_info.ds_pos.pos_y = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].image.pos.y;
		pIme_ds_info->ds_set2.ds_image_info.ds_lofs = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].image.ime_lofs;
		pIme_ds_info->ds_set2.ds_image_info.ds_addr = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].image.img_addr;

		pIme_ds_info->ds_set2.ds_iq_info.ds_ck_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.color_key_en;
		//pIme_ds_info->ds_set2.ds_iq_info.uiDsCk = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.color_key_val;
		pIme_ds_info->ds_set2.ds_iq_info.ds_bld_wet0 = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.bld_wt_0;
		pIme_ds_info->ds_set2.ds_iq_info.ds_bld_wet1 = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.bld_wt_1;

		pIme_ds_info->ds_set2.ds_iq_info.ds_plt_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.plt_en;
		pIme_ds_info->ds_set2.ds_iq_info.ds_ck_mode = (IME_DS_CK_MODE_SEL)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.color_key_mode;
		pIme_ds_info->ds_set2.ds_iq_info.ds_ck_a = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.color_key_val[0];  ///< Color key for alpha channel
		pIme_ds_info->ds_set2.ds_iq_info.ds_ck_r = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.color_key_val[1];  ///< Color key for R channel
		pIme_ds_info->ds_set2.ds_iq_info.ds_ck_g = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.color_key_val[2];  ///< Color key for G channel
		pIme_ds_info->ds_set2.ds_iq_info.ds_ck_b = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].ds_iq.color_key_val[3];  ///< Color key for B channel
	}

	//Set 3
	pIme_ds_info->ds_set3.ds_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].enable;
	if (pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].enable == ENABLE) {
		pIme_ds_info->ds_set3.ds_image_info.ds_img_size.size_h = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].image.img_size.w;
		pIme_ds_info->ds_set3.ds_image_info.ds_img_size.size_v = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].image.img_size.h;
		pIme_ds_info->ds_set3.ds_image_info.ds_fmt = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].image.img_fmt;
		pIme_ds_info->ds_set3.ds_image_info.ds_pos.pos_x = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].image.pos.x;
		pIme_ds_info->ds_set3.ds_image_info.ds_pos.pos_y = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].image.pos.y;
		pIme_ds_info->ds_set3.ds_image_info.ds_lofs = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].image.ime_lofs;
		pIme_ds_info->ds_set3.ds_image_info.ds_addr = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].image.img_addr;

		pIme_ds_info->ds_set3.ds_iq_info.ds_ck_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.color_key_en;
		//pIme_ds_info->ds_set3.ds_iq_info.uiDsCk = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.color_key_val;
		pIme_ds_info->ds_set3.ds_iq_info.ds_bld_wet0 = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.bld_wt_0;
		pIme_ds_info->ds_set3.ds_iq_info.ds_bld_wet1 = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.bld_wt_1;

		pIme_ds_info->ds_set3.ds_iq_info.ds_plt_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.plt_en;
		pIme_ds_info->ds_set3.ds_iq_info.ds_ck_mode = (IME_DS_CK_MODE_SEL)pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.color_key_mode;
		pIme_ds_info->ds_set3.ds_iq_info.ds_ck_a = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.color_key_val[0];  ///< Color key for alpha channel
		pIme_ds_info->ds_set3.ds_iq_info.ds_ck_r = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.color_key_val[1];  ///< Color key for R channel
		pIme_ds_info->ds_set3.ds_iq_info.ds_ck_g = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.color_key_val[2];  ///< Color key for G channel
		pIme_ds_info->ds_set3.ds_iq_info.ds_ck_b = pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].ds_iq.color_key_val[3];  ///< Color key for B channel
	}

	if ((pkdrv_ime_par->in_img_info.in_src == KDRV_IME_IN_SRC_DRAM) && (pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].enable == ENABLE || pkdrv_ime_par->sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].enable == ENABLE)) {
		DBG_ERR("KDRV IME: OSD do not support set1 and set2 under Dram2Dram mode\r\n");
		pIme_ds_info->ds_set1.ds_enable = IME_FUNC_DISABLE;
		pIme_ds_info->ds_set2.ds_enable = IME_FUNC_DISABLE;
	}

	// osd palette table
	pIme_ds_info->ds_plt.ds_plt_mode = (IME_DS_PLT_SEL)pkdrv_ime_par->sub_modules.ds_plt.plt_mode;
	for (i = 0; i < KDRV_IME_DS_PLT_TAB; i++) {
		pIme_ds_info->ds_plt.ds_plt_tab_a[i] = pkdrv_ime_par->sub_modules.ds_plt.plt_a[i];
		pIme_ds_info->ds_plt.ds_plt_tab_r[i] = pkdrv_ime_par->sub_modules.ds_plt.plt_r[i];
		pIme_ds_info->ds_plt.ds_plt_tab_g[i] = pkdrv_ime_par->sub_modules.ds_plt.plt_g[i];
		pIme_ds_info->ds_plt.ds_plt_tab_b[i] = pkdrv_ime_par->sub_modules.ds_plt.plt_b[i];
	}

	//data stamp cst
	pIme_ds_info->ds_cst_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.ds_cst.enable;
	if (pkdrv_ime_par->sub_modules.ds_cst.enable == ENABLE) {
		if (pkdrv_ime_par->sub_modules.ds_cst.cst_auto_param_en == ENABLE) {
			pIme_ds_info->ds_cst_coef.ds_cst_param_mode = IME_PARAM_AUTO_MODE;
		} else {
			pIme_ds_info->ds_cst_coef.ds_cst_param_mode = IME_PARAM_USER_MODE;
			pIme_ds_info->ds_cst_coef.ds_cst_coef0 = pkdrv_ime_par->sub_modules.ds_cst.cst_coef0;
			pIme_ds_info->ds_cst_coef.ds_cst_coef1 = pkdrv_ime_par->sub_modules.ds_cst.cst_coef1;
			pIme_ds_info->ds_cst_coef.ds_cst_coef2 = pkdrv_ime_par->sub_modules.ds_cst.cst_coef2;
			pIme_ds_info->ds_cst_coef.ds_cst_coef3 = pkdrv_ime_par->sub_modules.ds_cst.cst_coef3;
		}
	}

	return E_OK;
}

static ER kdrv_ime_int_set_stl_info(KDRV_IME_STL *pdal_stl_par, IME_STL_INFO *pIme_stl_info)
{
	if (pdal_stl_par == NULL) {
		DBG_ERR("KDRV IME: set statistical parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_stl_info == NULL) {
		DBG_ERR("KDRV IME: set statistical parameter NULL...\r\n");
		return E_PAR;
	}

	pIme_stl_info->stl_enable = (IME_FUNC_EN)pdal_stl_par->stl_info.enable;
	pIme_stl_info->stl_filter_enable = (IME_FUNC_EN)pdal_stl_par->stl_info.median_ftr_enable;
	if (pdal_stl_par->stl_info.img_out_aft_filt == TRUE) {
		pIme_stl_info->stl_img_out_type = IME_STL_IMGOUT_AF;
	} else {
		pIme_stl_info->stl_img_out_type = IME_STL_IMGOUT_BF;
	}

	pIme_stl_info->stl_edge_map.stl_edge_ker0_enable = pdal_stl_par->stl_info.edge.ker0_enable;
	pIme_stl_info->stl_edge_map.stl_edge_ker0 = pdal_stl_par->stl_info.edge.ker0_sel;
	pIme_stl_info->stl_edge_map.stl_sft0 = pdal_stl_par->stl_info.edge.shift0;
	pIme_stl_info->stl_edge_map.stl_edge_ker1_enable = pdal_stl_par->stl_info.edge.ker1_enable;
	pIme_stl_info->stl_edge_map.stl_edge_ker1 = pdal_stl_par->stl_info.edge.ker1_sel;
	pIme_stl_info->stl_edge_map.stl_sft1 = pdal_stl_par->stl_info.edge.shift1;

	pIme_stl_info->stl_hist0.stl_set_sel = pdal_stl_par->stl_hist.set_sel0;
	pIme_stl_info->stl_hist0.stl_hist_pos.pos_x = pdal_stl_par->stl_hist.crop_win0.x;
	pIme_stl_info->stl_hist0.stl_hist_pos.pos_y = pdal_stl_par->stl_hist.crop_win0.y;
	pIme_stl_info->stl_hist0.stl_hist_size.size_h = pdal_stl_par->stl_hist.crop_win0.w;
	pIme_stl_info->stl_hist0.stl_hist_size.size_v = pdal_stl_par->stl_hist.crop_win0.h;
	pIme_stl_info->stl_hist0.stl_hist_acc_tag.acc_tag = pdal_stl_par->stl_hist.acc_targ0;

	pIme_stl_info->stl_hist1.stl_set_sel = pdal_stl_par->stl_hist.set_sel1;
	pIme_stl_info->stl_hist1.stl_hist_pos.pos_x = pdal_stl_par->stl_hist.crop_win1.x;
	pIme_stl_info->stl_hist1.stl_hist_pos.pos_y = pdal_stl_par->stl_hist.crop_win1.y;
	pIme_stl_info->stl_hist1.stl_hist_size.size_h = pdal_stl_par->stl_hist.crop_win1.w;
	pIme_stl_info->stl_hist1.stl_hist_size.size_v = pdal_stl_par->stl_hist.crop_win1.h;
	pIme_stl_info->stl_hist1.stl_hist_acc_tag.acc_tag = pdal_stl_par->stl_hist.acc_targ1;

	pIme_stl_info->stl_roi.stl_row_pos.stl_row0 = pdal_stl_par->stl_roi.row0;
	pIme_stl_info->stl_roi.stl_row_pos.stl_row1 = pdal_stl_par->stl_roi.row1;

	pIme_stl_info->stl_roi.stl_roi0.stl_roi_src = pdal_stl_par->stl_roi.roi[0].src;
	pIme_stl_info->stl_roi.stl_roi0.stl_roi_th0 = pdal_stl_par->stl_roi.roi[0].th0;
	pIme_stl_info->stl_roi.stl_roi0.stl_roi_th1 = pdal_stl_par->stl_roi.roi[0].th1;
	pIme_stl_info->stl_roi.stl_roi0.stl_roi_th2 = pdal_stl_par->stl_roi.roi[0].th2;

	pIme_stl_info->stl_roi.stl_roi1.stl_roi_src = pdal_stl_par->stl_roi.roi[1].src;
	pIme_stl_info->stl_roi.stl_roi1.stl_roi_th0 = pdal_stl_par->stl_roi.roi[1].th0;
	pIme_stl_info->stl_roi.stl_roi1.stl_roi_th1 = pdal_stl_par->stl_roi.roi[1].th1;
	pIme_stl_info->stl_roi.stl_roi1.stl_roi_th2 = pdal_stl_par->stl_roi.roi[1].th2;

	pIme_stl_info->stl_roi.stl_roi2.stl_roi_src = pdal_stl_par->stl_roi.roi[2].src;
	pIme_stl_info->stl_roi.stl_roi2.stl_roi_th0 = pdal_stl_par->stl_roi.roi[2].th0;
	pIme_stl_info->stl_roi.stl_roi2.stl_roi_th1 = pdal_stl_par->stl_roi.roi[2].th1;
	pIme_stl_info->stl_roi.stl_roi2.stl_roi_th2 = pdal_stl_par->stl_roi.roi[2].th2;

	pIme_stl_info->stl_roi.stl_roi3.stl_roi_src = pdal_stl_par->stl_roi.roi[3].src;
	pIme_stl_info->stl_roi.stl_roi3.stl_roi_th0 = pdal_stl_par->stl_roi.roi[3].th0;
	pIme_stl_info->stl_roi.stl_roi3.stl_roi_th1 = pdal_stl_par->stl_roi.roi[3].th1;
	pIme_stl_info->stl_roi.stl_roi3.stl_roi_th2 = pdal_stl_par->stl_roi.roi[3].th2;

	pIme_stl_info->stl_roi.stl_roi4.stl_roi_src = pdal_stl_par->stl_roi.roi[4].src;
	pIme_stl_info->stl_roi.stl_roi4.stl_roi_th0 = pdal_stl_par->stl_roi.roi[4].th0;
	pIme_stl_info->stl_roi.stl_roi4.stl_roi_th1 = pdal_stl_par->stl_roi.roi[4].th1;
	pIme_stl_info->stl_roi.stl_roi4.stl_roi_th2 = pdal_stl_par->stl_roi.roi[4].th2;

	pIme_stl_info->stl_roi.stl_roi5.stl_roi_src = pdal_stl_par->stl_roi.roi[5].src;
	pIme_stl_info->stl_roi.stl_roi5.stl_roi_th0 = pdal_stl_par->stl_roi.roi[5].th0;
	pIme_stl_info->stl_roi.stl_roi5.stl_roi_th1 = pdal_stl_par->stl_roi.roi[5].th1;
	pIme_stl_info->stl_roi.stl_roi5.stl_roi_th2 = pdal_stl_par->stl_roi.roi[5].th2;

	pIme_stl_info->stl_roi.stl_roi6.stl_roi_src = pdal_stl_par->stl_roi.roi[6].src;
	pIme_stl_info->stl_roi.stl_roi6.stl_roi_th0 = pdal_stl_par->stl_roi.roi[6].th0;
	pIme_stl_info->stl_roi.stl_roi6.stl_roi_th1 = pdal_stl_par->stl_roi.roi[6].th1;
	pIme_stl_info->stl_roi.stl_roi6.stl_roi_th2 = pdal_stl_par->stl_roi.roi[6].th2;

	pIme_stl_info->stl_roi.stl_roi7.stl_roi_src = pdal_stl_par->stl_roi.roi[7].src;
	pIme_stl_info->stl_roi.stl_roi7.stl_roi_th0 = pdal_stl_par->stl_roi.roi[7].th0;
	pIme_stl_info->stl_roi.stl_roi7.stl_roi_th1 = pdal_stl_par->stl_roi.roi[7].th1;
	pIme_stl_info->stl_roi.stl_roi7.stl_roi_th2 = pdal_stl_par->stl_roi.roi[7].th2;


	pIme_stl_info->stl_edge_map_flip_enable = (IME_FUNC_EN)pdal_stl_par->stl_info.edge.flip_en;

	pIme_stl_info->stl_edge_map_lofs = pdal_stl_par->stl_out_img.edge_map_lofs;

	if (pIme_stl_info->stl_edge_map_flip_enable == IME_FUNC_DISABLE) {
		pIme_stl_info->stl_edge_map_addr = pdal_stl_par->stl_out_img.edge_map_addr;
	} else {
		UINT32 flip_size;

		flip_size = pIme_stl_info->stl_edge_map_lofs * (pIme_stl_info->stl_img_size.size_v - 1);

		pIme_stl_info->stl_edge_map_addr = pdal_stl_par->stl_out_img.edge_map_addr + flip_size;
	}

	pIme_stl_info->stl_hist_addr = pdal_stl_par->stl_out_img.hist_addr;

	return E_OK;
}

static ER kdrv_ime_int_set_pm_par(KDRV_IME_PRAM *pkdrv_ime_par, IME_PM_INFO *pIme_pm_info)
{
	UINT32 i = 0;
	BOOL bdisable_type_pxl = FALSE;
	BOOL bdisable_pm_set = FALSE;
	UINT32 min_pxl_img_w = 0xffffffff, min_pxl_img_h = 0xffffffff;

	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set privay mask parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_pm_info == NULL) {
		DBG_ERR("KDRV IME: privay mask parameter NULL...\r\n");
		return E_PAR;
	}

	switch (pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.blk_size) {
	case KDRV_IME_PM_PIXELATION_08:
		min_pxl_img_w = (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].width + (8 - 1)) >> 3;
		min_pxl_img_h = (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].height + (8 - 1)) >> 3;
		break;

	case KDRV_IME_PM_PIXELATION_16:
		min_pxl_img_w = (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].width + (16 - 1)) >> 4;
		min_pxl_img_h = (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].height + (16 - 1)) >> 4;
		break;

	case KDRV_IME_PM_PIXELATION_32:
		min_pxl_img_w = (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].width + (32 - 1)) >> 5;
		min_pxl_img_h = (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].height + (32 - 1)) >> 5;
		break;

	case KDRV_IME_PM_PIXELATION_64:
		min_pxl_img_w = (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].width + (64 - 1)) >> 6;
		min_pxl_img_h = (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].height + (64 - 1)) >> 6;
		break;

	default:
		break;
	}

	if (pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.img_size.w < min_pxl_img_w || pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.img_size.h < min_pxl_img_h) {
		bdisable_type_pxl = TRUE;
	}

	if (bdisable_type_pxl) {
		for (i = 0; i < KDRV_IME_PM_SET_IDX_MAX; i++) {
			if (pkdrv_ime_par->sub_modules.pm.pm_info[i].enable == ENABLE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
				bdisable_pm_set = TRUE;
				DBG_WRN("KDRV IME: PM PXL Image Size too smal, Disable IME_PM_SET_IDX_%d\r\n", (int)i);
			}
		}
		if (bdisable_pm_set) {
			if (pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.blk_size > KDRV_IME_PM_PIXELATION_64) {
				DBG_WRN("KDRV IME: PM PXL BLK SIZE overflow pm_pxl_blk_size = %d > %d\r\n", (int)pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.blk_size, (int)KDRV_IME_PM_PIXELATION_64);
			}

			DBG_WRN("KDRV IME: IME Input Size (%d, %d), PM_PXL_IMG Size (%d, %d), PM_PXL_BLK_SIZE_IDX %d\r\n",
					(int)pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].width, (int)pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].height,
					(int)pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.img_size.w, (int)pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.img_size.h,
					(int)pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.blk_size);
		}
	}

	//set 0
	pIme_pm_info->pm_set0.pm_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].enable;
	if (pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].enable == ENABLE) {
		if (bdisable_pm_set == TRUE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
			pIme_pm_info->pm_set0.pm_enable = IME_FUNC_DISABLE;
		} else {
			for (i = 0; i < 4; i++) {
				pIme_pm_info->pm_set0.pm_coord[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].coord[i].x;
				pIme_pm_info->pm_set0.pm_coord[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].coord[i].y;
			}
			pIme_pm_info->pm_set0.pm_mask_type = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].msk_type;
			pIme_pm_info->pm_set0.pm_color.pm_color_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].color[KDRV_IPP_YUV_Y];
			pIme_pm_info->pm_set0.pm_color.pm_color_u = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].color[KDRV_IPP_YUV_U];
			pIme_pm_info->pm_set0.pm_color.pm_color_v = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].color[KDRV_IPP_YUV_V];
			pIme_pm_info->pm_set0.pm_wet = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].weight;

			pIme_pm_info->pm_set0.pm_hlw_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].hlw_enable;
			if (pIme_pm_info->pm_set0.pm_hlw_enable == IME_FUNC_ENABLE) {
				for (i = 0; i < 4; i++) {
					pIme_pm_info->pm_set0.pm_coord_hlw[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].coord2[i].x;
					pIme_pm_info->pm_set0.pm_coord_hlw[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].coord2[i].y;
				}
			}
		}
	}

	//set 1
	pIme_pm_info->pm_set1.pm_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].enable;
	if (pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].enable == ENABLE) {
		if (bdisable_pm_set == TRUE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
			pIme_pm_info->pm_set1.pm_enable = IME_FUNC_DISABLE;
		} else {
			for (i = 0; i < 4; i++) {
				pIme_pm_info->pm_set1.pm_coord[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].coord[i].x;
				pIme_pm_info->pm_set1.pm_coord[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].coord[i].y;
			}
			pIme_pm_info->pm_set1.pm_mask_type = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].msk_type;
			pIme_pm_info->pm_set1.pm_color.pm_color_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].color[KDRV_IPP_YUV_Y];
			pIme_pm_info->pm_set1.pm_color.pm_color_u = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].color[KDRV_IPP_YUV_U];
			pIme_pm_info->pm_set1.pm_color.pm_color_v = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].color[KDRV_IPP_YUV_V];
			pIme_pm_info->pm_set1.pm_wet = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].weight;

			pIme_pm_info->pm_set1.pm_hlw_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].hlw_enable;
			if (pIme_pm_info->pm_set1.pm_hlw_enable == IME_FUNC_ENABLE) {
				for (i = 0; i < 4; i++) {
					pIme_pm_info->pm_set1.pm_coord_hlw[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].coord2[i].x;
					pIme_pm_info->pm_set1.pm_coord_hlw[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].coord2[i].y;
				}
			}
		}
	}

	//set 2
	pIme_pm_info->pm_set2.pm_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].enable;
	if (pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].enable == ENABLE) {
		if (bdisable_pm_set == TRUE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
			pIme_pm_info->pm_set2.pm_enable = IME_FUNC_DISABLE;
		} else {
			for (i = 0; i < 4; i++) {
				pIme_pm_info->pm_set2.pm_coord[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].coord[i].x;
				pIme_pm_info->pm_set2.pm_coord[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].coord[i].y;
			}
			pIme_pm_info->pm_set2.pm_mask_type = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].msk_type;
			pIme_pm_info->pm_set2.pm_color.pm_color_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].color[KDRV_IPP_YUV_Y];
			pIme_pm_info->pm_set2.pm_color.pm_color_u = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].color[KDRV_IPP_YUV_U];
			pIme_pm_info->pm_set2.pm_color.pm_color_v = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].color[KDRV_IPP_YUV_V];
			pIme_pm_info->pm_set2.pm_wet = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].weight;

			pIme_pm_info->pm_set2.pm_hlw_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].hlw_enable;
			if (pIme_pm_info->pm_set2.pm_hlw_enable == IME_FUNC_ENABLE) {
				for (i = 0; i < 4; i++) {
					pIme_pm_info->pm_set2.pm_coord_hlw[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].coord2[i].x;
					pIme_pm_info->pm_set2.pm_coord_hlw[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].coord2[i].y;
				}
			}
		}
	}

	//set 3
	pIme_pm_info->pm_set3.pm_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].enable;
	if (pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].enable == ENABLE) {
		if (bdisable_pm_set == TRUE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
			pIme_pm_info->pm_set3.pm_enable = IME_FUNC_DISABLE;
		} else {
			for (i = 0; i < 4; i++) {
				pIme_pm_info->pm_set3.pm_coord[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].coord[i].x;
				pIme_pm_info->pm_set3.pm_coord[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].coord[i].y;
			}
			pIme_pm_info->pm_set3.pm_mask_type = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].msk_type;
			pIme_pm_info->pm_set3.pm_color.pm_color_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].color[KDRV_IPP_YUV_Y];
			pIme_pm_info->pm_set3.pm_color.pm_color_u = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].color[KDRV_IPP_YUV_U];
			pIme_pm_info->pm_set3.pm_color.pm_color_v = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].color[KDRV_IPP_YUV_V];
			pIme_pm_info->pm_set3.pm_wet = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].weight;

			pIme_pm_info->pm_set3.pm_hlw_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].hlw_enable;
			if (pIme_pm_info->pm_set3.pm_hlw_enable == IME_FUNC_ENABLE) {
				for (i = 0; i < 4; i++) {
					pIme_pm_info->pm_set3.pm_coord_hlw[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].coord2[i].x;
					pIme_pm_info->pm_set3.pm_coord_hlw[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].coord2[i].y;
				}
			}
		}
	}
	//set 4
	pIme_pm_info->pm_set4.pm_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].enable;
	if (pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].enable == ENABLE) {
		if (bdisable_pm_set == TRUE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
			pIme_pm_info->pm_set4.pm_enable = IME_FUNC_DISABLE;
		} else {
			for (i = 0; i < 4; i++) {
				pIme_pm_info->pm_set4.pm_coord[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].coord[i].x;
				pIme_pm_info->pm_set4.pm_coord[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].coord[i].y;
			}
			pIme_pm_info->pm_set4.pm_mask_type = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].msk_type;
			pIme_pm_info->pm_set4.pm_color.pm_color_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].color[KDRV_IPP_YUV_Y];
			pIme_pm_info->pm_set4.pm_color.pm_color_u = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].color[KDRV_IPP_YUV_U];
			pIme_pm_info->pm_set4.pm_color.pm_color_v = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].color[KDRV_IPP_YUV_V];
			pIme_pm_info->pm_set4.pm_wet = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].weight;

			pIme_pm_info->pm_set4.pm_hlw_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].hlw_enable;
			if (pIme_pm_info->pm_set4.pm_hlw_enable == IME_FUNC_ENABLE) {
				for (i = 0; i < 4; i++) {
					pIme_pm_info->pm_set4.pm_coord_hlw[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].coord2[i].x;
					pIme_pm_info->pm_set4.pm_coord_hlw[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].coord2[i].y;
				}
			}
		}
	}

	//set 5
	pIme_pm_info->pm_set5.pm_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].enable;
	if (pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].enable == ENABLE) {
		if (bdisable_pm_set == TRUE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
			pIme_pm_info->pm_set5.pm_enable = IME_FUNC_DISABLE;
		} else {
			for (i = 0; i < 4; i++) {
				pIme_pm_info->pm_set5.pm_coord[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].coord[i].x;
				pIme_pm_info->pm_set5.pm_coord[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].coord[i].y;
			}
			pIme_pm_info->pm_set5.pm_mask_type = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].msk_type;
			pIme_pm_info->pm_set5.pm_color.pm_color_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].color[KDRV_IPP_YUV_Y];
			pIme_pm_info->pm_set5.pm_color.pm_color_u = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].color[KDRV_IPP_YUV_U];
			pIme_pm_info->pm_set5.pm_color.pm_color_v = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].color[KDRV_IPP_YUV_V];
			pIme_pm_info->pm_set5.pm_wet = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].weight;

			pIme_pm_info->pm_set5.pm_hlw_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].hlw_enable;
			if (pIme_pm_info->pm_set5.pm_hlw_enable == IME_FUNC_ENABLE) {
				for (i = 0; i < 4; i++) {
					pIme_pm_info->pm_set5.pm_coord_hlw[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].coord2[i].x;
					pIme_pm_info->pm_set5.pm_coord_hlw[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].coord2[i].y;
				}
			}
		}
	}

	//set 6
	pIme_pm_info->pm_set6.pm_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].enable;
	if (pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].enable == ENABLE) {
		if (bdisable_pm_set == TRUE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
			pIme_pm_info->pm_set6.pm_enable = IME_FUNC_DISABLE;
		} else {
			for (i = 0; i < 4; i++) {
				pIme_pm_info->pm_set6.pm_coord[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].coord[i].x;
				pIme_pm_info->pm_set6.pm_coord[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].coord[i].y;
			}
			pIme_pm_info->pm_set6.pm_mask_type = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].msk_type;
			pIme_pm_info->pm_set6.pm_color.pm_color_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].color[KDRV_IPP_YUV_Y];
			pIme_pm_info->pm_set6.pm_color.pm_color_u = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].color[KDRV_IPP_YUV_U];
			pIme_pm_info->pm_set6.pm_color.pm_color_v = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].color[KDRV_IPP_YUV_V];
			pIme_pm_info->pm_set6.pm_wet = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].weight;

			pIme_pm_info->pm_set6.pm_hlw_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].hlw_enable;
			if (pIme_pm_info->pm_set6.pm_hlw_enable == IME_FUNC_ENABLE) {
				for (i = 0; i < 4; i++) {
					pIme_pm_info->pm_set6.pm_coord_hlw[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].coord2[i].x;
					pIme_pm_info->pm_set6.pm_coord_hlw[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].coord2[i].y;
				}
			}
		}
	}

	//set 7
	pIme_pm_info->pm_set7.pm_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].enable;
	if (pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].enable == ENABLE) {
		if (bdisable_pm_set == TRUE && pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
			pIme_pm_info->pm_set7.pm_enable = IME_FUNC_DISABLE;
		} else {
			for (i = 0; i < 4; i++) {
				pIme_pm_info->pm_set7.pm_coord[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].coord[i].x;
				pIme_pm_info->pm_set7.pm_coord[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].coord[i].y;
			}
			pIme_pm_info->pm_set7.pm_mask_type = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].msk_type;
			pIme_pm_info->pm_set7.pm_color.pm_color_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].color[KDRV_IPP_YUV_Y];
			pIme_pm_info->pm_set7.pm_color.pm_color_u = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].color[KDRV_IPP_YUV_U];
			pIme_pm_info->pm_set7.pm_color.pm_color_v = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].color[KDRV_IPP_YUV_V];
			pIme_pm_info->pm_set7.pm_wet = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].weight;

			pIme_pm_info->pm_set7.pm_hlw_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].hlw_enable;
			if (pIme_pm_info->pm_set7.pm_hlw_enable == IME_FUNC_ENABLE) {
				for (i = 0; i < 4; i++) {
					pIme_pm_info->pm_set7.pm_coord_hlw[i].coord_x = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].coord2[i].x;
					pIme_pm_info->pm_set7.pm_coord_hlw[i].coord_y = pkdrv_ime_par->sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].coord2[i].y;
				}
			}
		}
	}

	//pm pxl image info
	pIme_pm_info->pm_pxl_img_size.size_h = pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.img_size.w;
	pIme_pm_info->pm_pxl_img_size.size_v = pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.img_size.h;
	pIme_pm_info->pm_pxl_blk_size = pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.blk_size;
	pIme_pm_info->pm_pxl_lofs = pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.lofs;
	pIme_pm_info->pm_pxl_dma_addr = pkdrv_ime_par->sub_modules.pm.pm_pxl_img_info.dma_addr;

	return E_OK;
}

static ER kdrv_ime_int_set_tmnr_par(KDRV_IME_PRAM *pkdrv_ime_par, IME_TMNR_INFO *pIme_tmnr_info)
{
	UINT32 i = 0;

	UINT32 ps_mix_slope[KDRV_IME_TMNR_PS_MIX_SLOPE_TAB] = {0};
	UINT32 ps_edge_slope = 0;


	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set tmnr parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_tmnr_info == NULL) {
		DBG_ERR("KDRV IME: set tmnr parameter NULL...\r\n");
		return E_PAR;
	}

	//pIme_tmnr_info->tmnr_en = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.tmnr.nr_iq.enable;
	if (pIme_tmnr_info->tmnr_en == IME_FUNC_ENABLE) {
		if (pkdrv_ime_par->in_img_info.ch[KDRV_IPP_YUV_Y].width % 4 == 0) {

			pIme_tmnr_info->dbg_ctrl.dbg_froce_mv0_en = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.tmnr.nr_dbg.dbg_mv0;
			pIme_tmnr_info->dbg_ctrl.dbg_mode = pkdrv_ime_par->sub_modules.tmnr.nr_dbg.dbg_mode;

			// motion estimation
			pIme_tmnr_info->me_param.update_mode = (IME_TMNR_ME_UPDATE_MODE)pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.update_mode;
			pIme_tmnr_info->me_param.boundary_set = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.boundary_set;
			pIme_tmnr_info->me_param.ds_mode = (IME_TMNR_MV_DOWN_SAMPLE_MODE)pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.ds_mode;
			pIme_tmnr_info->me_param.sad_type = (IME_TMNR_ME_SDA_TYPE)pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.sad_type;
			pIme_tmnr_info->me_param.sad_shift = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.sad_shift;

			pIme_tmnr_info->me_param.rand_bit_x  = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.rand_bit_x;
			pIme_tmnr_info->me_param.rand_bit_y  = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.rand_bit_y;
			pIme_tmnr_info->me_param.min_detail  = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.min_detail;
			pIme_tmnr_info->me_param.periodic_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.periodic_th;
			pIme_tmnr_info->me_param.switch_rto  = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.switch_rto;

			for (i = 0; i < KDRV_IME_TMNR_ME_SAD_PENALTY_TAB; i++) {
				pIme_tmnr_info->me_param.sad_penalty[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.sad_penalty[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_ME_SWITCH_THRESHOLD_TAB; i++) {
				pIme_tmnr_info->me_param.switch_th[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.switch_th[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_ME_DETAIL_PENALTY_TAB; i++) {
				pIme_tmnr_info->me_param.detail_penalty[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.detail_penalty[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_ME_PROBABILITY_TAB; i++) {
				pIme_tmnr_info->me_param.probability[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.me_param.probability[i];
			}

			// motion detection
			for (i = 0; i < KDRV_IME_TMNR_MD_SAD_COEFA_TAB; i++) {
				pIme_tmnr_info->md_param.sad_coefa[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.md_param.sad_coefa[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_MD_SAD_COEFB_TAB; i++) {
				pIme_tmnr_info->md_param.sad_coefb[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.md_param.sad_coefb[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_MD_SAD_STD_TAB; i++) {
				pIme_tmnr_info->md_param.sad_std[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.md_param.sad_std[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_MD_FINAL_THRESHOLD_TAB; i++) {
				pIme_tmnr_info->md_param.fth[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.md_param.fth[i];
			}

			// motion detection for ROI
			for (i = 0; i < KDRV_IME_TMNR_MD_ROI_FINAL_THRESHOLD_TAB; i++) {
				pIme_tmnr_info->md_roi_param.fth[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.md_roi_param.fth[i];
			}

			// motion compensation
			for (i = 0; i < KDRV_IME_TMNR_MC_SAD_BASE_TAB; i++) {
				pIme_tmnr_info->mc_param.sad_base[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.mc_param.sad_base[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_MC_SAD_BASE_TAB; i++) {
				pIme_tmnr_info->mc_param.sad_coefa[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.mc_param.sad_coefa[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_MC_SAD_COEFB_TAB; i++) {
				pIme_tmnr_info->mc_param.sad_coefb[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.mc_param.sad_coefb[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_MC_SAD_STD_TAB; i++) {
				pIme_tmnr_info->mc_param.sad_std[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.mc_param.sad_std[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_MC_FINAL_THRESHOLD_TAB; i++) {
				pIme_tmnr_info->mc_param.fth[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.mc_param.fth[i];
			}

			// motion compensation for ROI
			for (i = 0; i < KDRV_IME_TMNR_MC_ROI_FINAL_THRESHOLD_TAB; i++) {
				pIme_tmnr_info->mc_roi_param.fth[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.mc_roi_param.fth[i];
			}


			// patch selection
			pIme_tmnr_info->ps_param.smart_roi_ctrl_en = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.smart_roi_ctrl_en;
			pIme_tmnr_info->ps_param.mv_check_en = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.mv_check_en;
			pIme_tmnr_info->ps_param.roi_mv_check_en = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.roi_mv_check_en;
			pIme_tmnr_info->ps_param.ps_mode = (IME_TMNR_PS_MODE)pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.ps_mode;
			pIme_tmnr_info->ps_param.mv_info_mode = (IME_TMNR_MV_INFO_MODE)pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.mv_info_mode;
			pIme_tmnr_info->ps_param.mv_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.mv_th;
			pIme_tmnr_info->ps_param.roi_mv_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.roi_mv_th;
			pIme_tmnr_info->ps_param.ds_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.ds_th;
			pIme_tmnr_info->ps_param.ds_th_roi = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.ds_th_roi;
			pIme_tmnr_info->ps_param.edge_wet = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.edge_wet;
			pIme_tmnr_info->ps_param.fs_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.fs_th;



			for (i = 0; i < KDRV_IME_TMNR_PS_MIX_RATIO_TAB; i++) {
				pIme_tmnr_info->ps_param.mix_ratio[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.mix_ratio[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_PS_MIX_THRESHOLD_TAB; i++) {
				pIme_tmnr_info->ps_param.mix_th[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.mix_th[i];
			}

			ps_mix_slope[0] = (pIme_tmnr_info->ps_param.mix_ratio[0] * 1024) / pIme_tmnr_info->ps_param.mix_th[0];
			ps_mix_slope[1] = ((pIme_tmnr_info->ps_param.mix_ratio[1] - pIme_tmnr_info->ps_param.mix_ratio[0]) * 1024) / (pIme_tmnr_info->ps_param.mix_th[1] - pIme_tmnr_info->ps_param.mix_th[0]);
			for (i = 0; i < KDRV_IME_TMNR_PS_MIX_SLOPE_TAB; i++) {
				//pIme_tmnr_info->ps_param.mix_slope[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.mix_slope[i];
				pIme_tmnr_info->ps_param.mix_slope[i] = ps_mix_slope[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_PS_EDGE_THRESHOLD_TAB; i++) {
				pIme_tmnr_info->ps_param.edge_th[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.ps_param.edge_th[i];
			}

			ps_edge_slope = ((255 - pIme_tmnr_info->ps_param.edge_wet) * 256) / (pIme_tmnr_info->ps_param.edge_th[1] - pIme_tmnr_info->ps_param.edge_th[0]);
			pIme_tmnr_info->ps_param.edge_slope = ps_edge_slope;
			// noise reduction
			pIme_tmnr_info->nr_param.luma_ch_en = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.luma_ch_en;
			pIme_tmnr_info->nr_param.chroma_ch_en = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.chroma_ch_en;

			pIme_tmnr_info->nr_param.center_wzeros_y = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.center_wzeros_y;
			pIme_tmnr_info->nr_param.chroma_fsv_en = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.chroma_fsv_en;
			pIme_tmnr_info->nr_param.chroma_fsv = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.chroma_fsv;
			pIme_tmnr_info->nr_param.luma_residue_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.luma_residue_th;
			pIme_tmnr_info->nr_param.chroma_residue_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.chroma_residue_th;

			pIme_tmnr_info->nr_param.pre_filter_type = (IME_TMNR_PRE_FILTER_TYPE)pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.pre_filter_type;
			pIme_tmnr_info->nr_param.snr_base_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.snr_base_th;
			pIme_tmnr_info->nr_param.tnr_base_th = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.tnr_base_th;

			pIme_tmnr_info->nr_param.luma_nr_type = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.luma_nr_type;

			for (i = 0; i < KDRV_IME_TMNR_NR_FREQ_WEIGHT_TAB; i++) {
				pIme_tmnr_info->nr_param.freq_wet[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.freq_wet[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_LUMA_WEIGHT_TAB; i++) {
				pIme_tmnr_info->nr_param.luma_wet[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.luma_wet[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_PRE_FILTER_STRENGTH_TAB; i++) {
				pIme_tmnr_info->nr_param.pre_filter_str[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.pre_filter_str[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_PRE_FILTER_RATION_TAB; i++) {
				pIme_tmnr_info->nr_param.pre_filter_rto[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.pre_filter_rto[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_SFILTER_STRENGTH_TAB; i++) {
				pIme_tmnr_info->nr_param.snr_str[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.snr_str[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_TFILTER_STRENGTH_TAB; i++) {
				pIme_tmnr_info->nr_param.tnr_str[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.tnr_str[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_LUMA_THRESHOLD_TAB; i++) {
				pIme_tmnr_info->nr_param.luma_3d_th[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.luma_3d_th[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_LUMA_LUT_TAB; i++) {
				pIme_tmnr_info->nr_param.luma_3d_lut[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.luma_3d_lut[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_CHROMA_LUT_TAB; i++) {
				pIme_tmnr_info->nr_param.chroma_3d_lut[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.chroma_3d_lut[i];
			}

			for (i = 0; i < KDRV_IME_TMNR_NR_CHROMA_RATIO_TAB; i++) {
				pIme_tmnr_info->nr_param.chroma_3d_rto[i] = pkdrv_ime_par->sub_modules.tmnr.nr_iq.nr_param.chroma_3d_rto[i];
			}

			// statistic data
			pIme_tmnr_info->sta_param.sta_out_en = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_out_en;
			pIme_tmnr_info->sta_param.sample_step_hori = pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_param.sample_step_hori;
			pIme_tmnr_info->sta_param.sample_step_vert = pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_param.sample_step_vert;
			pIme_tmnr_info->sta_param.sample_num_x = pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_param.sample_num_x;
			pIme_tmnr_info->sta_param.sample_num_y = pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_param.sample_num_y;

			pIme_tmnr_info->sta_param.sample_st_x = pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_param.sample_st_x;
			pIme_tmnr_info->sta_param.sample_st_y = pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_param.sample_st_y;


			pIme_tmnr_info->ref_in_dec_en = pkdrv_ime_par->sub_modules.tmnr.nr_ref_in_img.decode.enable;
			pIme_tmnr_info->ref_in_flip_en = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.tmnr.nr_ref_in_img.flip_en;
			pIme_tmnr_info->ref_in_ofs.lineoffset_y = pkdrv_ime_par->sub_modules.tmnr.nr_ref_in_img.img_line_ofs_y;
			pIme_tmnr_info->ref_in_ofs.lineoffset_cb = pkdrv_ime_par->sub_modules.tmnr.nr_ref_in_img.img_line_ofs_cb;

			if (pIme_tmnr_info->ref_in_flip_en == IME_FUNC_DISABLE) {
				pIme_tmnr_info->ref_in_addr.addr_y = pkdrv_ime_par->sub_modules.tmnr.nr_ref_in_img.img_addr_y;
				pIme_tmnr_info->ref_in_addr.addr_cb = pkdrv_ime_par->sub_modules.tmnr.nr_ref_in_img.img_addr_cb;
			} else {
				UINT32 flip_size;

				flip_size = pIme_tmnr_info->ref_in_ofs.lineoffset_y * (pIme_tmnr_info->m_img_size.size_v - 4);
				pIme_tmnr_info->ref_in_addr.addr_y = pkdrv_ime_par->sub_modules.tmnr.nr_ref_in_img.img_addr_y + flip_size;

				flip_size = pIme_tmnr_info->ref_in_ofs.lineoffset_cb * ((pIme_tmnr_info->m_img_size.size_v >> 1) - 1);
				pIme_tmnr_info->ref_in_addr.addr_cb = pkdrv_ime_par->sub_modules.tmnr.nr_ref_in_img.img_addr_cb + flip_size;
			}


			pIme_tmnr_info->mot_sta_ofs = pkdrv_ime_par->sub_modules.tmnr.nr_ms.mot_sta_ofs;
			pIme_tmnr_info->mot_sta_in_addr = pkdrv_ime_par->sub_modules.tmnr.nr_ms.mot_sta_in_addr;
			pIme_tmnr_info->mot_sta_out_addr = pkdrv_ime_par->sub_modules.tmnr.nr_ms.mot_sta_out_addr;

			pIme_tmnr_info->mot_sta_roi_out_en = pkdrv_ime_par->sub_modules.tmnr.nr_ms_roi.mot_sta_roi_out_en;
			pIme_tmnr_info->mot_sta_roi_out_flip_en = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.tmnr.nr_ms_roi.mot_sta_roi_out_flip_en;
			pIme_tmnr_info->mot_sta_roi_out_ofs = pkdrv_ime_par->sub_modules.tmnr.nr_ms_roi.mot_sta_roi_out_ofs;

			if (pIme_tmnr_info->mot_sta_roi_out_flip_en == IME_FUNC_DISABLE) {
				pIme_tmnr_info->mot_sta_roi_out_addr = pkdrv_ime_par->sub_modules.tmnr.nr_ms_roi.mot_sta_roi_out_addr;
			} else {
				UINT32 flip_size = 0;

				flip_size = pIme_tmnr_info->mot_sta_roi_out_ofs * ((pIme_tmnr_info->m_img_size.size_v >> 1) - 1);
				pIme_tmnr_info->mot_sta_roi_out_addr = pkdrv_ime_par->sub_modules.tmnr.nr_ms_roi.mot_sta_roi_out_addr + flip_size;
			}

			pIme_tmnr_info->mot_vec_ofs = pkdrv_ime_par->sub_modules.tmnr.nr_mv.mot_vec_ofs;
			pIme_tmnr_info->mot_vec_in_addr = pkdrv_ime_par->sub_modules.tmnr.nr_mv.mot_vec_in_addr;
			pIme_tmnr_info->mot_vec_out_addr = pkdrv_ime_par->sub_modules.tmnr.nr_mv.mot_vec_out_addr;


			pIme_tmnr_info->sta_out_ofs = pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_out_ofs;
			pIme_tmnr_info->sta_out_addr = pkdrv_ime_par->sub_modules.tmnr.nr_sta.sta_out_addr;

		} else {
			pIme_tmnr_info->tmnr_en = IME_FUNC_DISABLE;
			DBG_WRN("KDRV IME: TMNR Input Image Width must be multiple of 4, Disable TPLNR function\r\n");
		}
	}
	return E_OK;
}

static ER kdrv_ime_int_set_tmnr_refout_par(KDRV_IME_PRAM *pkdrv_ime_par, IME_TMNR_REF_OUT_INFO *pIme_tmnr_refout_info)
{
	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set tmnr parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_tmnr_refout_info == NULL) {
		DBG_ERR("KDRV IME: set tmnr reference output parameter NULL...\r\n");
		return E_PAR;
	}


	pIme_tmnr_refout_info->ref_out_enable = pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.enable;
	if (pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.enable == ENABLE) {
		pIme_tmnr_refout_info->ref_out_lofs.lineoffset_y = pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.img_line_ofs_y;
		pIme_tmnr_refout_info->ref_out_lofs.lineoffset_cb = pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.img_line_ofs_cb;

		pIme_tmnr_refout_info->ref_out_dma_flush = (IME_BUF_FLUSH_SEL)pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.buf_flush;

		pIme_tmnr_refout_info->ref_out_flip_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.flip_en;
		pIme_tmnr_refout_info->ref_out_enc_enable = pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.encode.enable;
		pIme_tmnr_refout_info->ref_out_enc_smode_enable = IME_FUNC_DISABLE;

		if (pIme_tmnr_refout_info->ref_out_flip_enable == IME_FUNC_DISABLE) {
			pIme_tmnr_refout_info->ref_out_addr.addr_y = pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.img_addr_y;
			pIme_tmnr_refout_info->ref_out_addr.addr_cb = pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.img_addr_cb;
		} else {
			UINT32 flip_size;

			flip_size = pIme_tmnr_refout_info->ref_out_lofs.lineoffset_y * (pIme_tmnr_refout_info->m_img_size.size_v - 1);
			pIme_tmnr_refout_info->ref_out_addr.addr_y = pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.img_addr_y + flip_size;

			flip_size = pIme_tmnr_refout_info->ref_out_lofs.lineoffset_cb * ((pIme_tmnr_refout_info->m_img_size.size_v >> 1) - 1);
			pIme_tmnr_refout_info->ref_out_addr.addr_cb = pkdrv_ime_par->sub_modules.tmnr_refout.nr_ref_out_img.img_addr_cb + flip_size;
		}
	}

	return E_OK;
}

static ER kdrv_ime_int_set_break_point(KDRV_IME_PRAM *pkdrv_ime_par, IME_BREAK_POINT_INFO *pIme_bp_info)
{
	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set tmnr parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_bp_info == NULL) {
		DBG_ERR("KDRV IME: set break-point parameter NULL...\r\n");
		return E_PAR;
	}

	pIme_bp_info->bp1 = pkdrv_ime_par->break_point_ctrl.bp1;
	pIme_bp_info->bp2 = pkdrv_ime_par->break_point_ctrl.bp2;
	pIme_bp_info->bp3 = pkdrv_ime_par->break_point_ctrl.bp3;

	if (nvt_get_chip_id() != CHIP_NA51055) {
		pIme_bp_info->bp_mode = (IME_BREAK_POINT_MODE)pkdrv_ime_par->break_point_ctrl.bp_mode;
	}

	return E_OK;
}

static ER kdrv_ime_int_set_single_output(KDRV_IME_PRAM *pkdrv_ime_par, IME_SINGLE_OUT_INFO *pIme_sout_info)
{
	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set single output parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_sout_info == NULL) {
		DBG_ERR("KDRV IME: set single output parameter NULL...\r\n");
		return E_PAR;
	}

	pIme_sout_info->sout_enable = (IME_FUNC_EN)pkdrv_ime_par->single_out_ctrl.sout_enable;
	pIme_sout_info->sout_ch = pkdrv_ime_par->single_out_ctrl.sout_chl;

	return E_OK;
}


static ER kdrv_ime_int_set_low_delay(KDRV_IME_PRAM *pkdrv_ime_par, IME_LOW_DELAY_INFO *pIme_ldy_info)
{
	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set low delay parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_ldy_info == NULL) {
		DBG_ERR("KDRV IME: set low delay parameter NULL...\r\n");
		return E_PAR;
	}

	pIme_ldy_info->dly_enable = (IME_FUNC_EN)pkdrv_ime_par->low_delay_ctrl.dly_enable;
	pIme_ldy_info->dly_ch = (IME_LOW_DELAY_CHL_SEL)pkdrv_ime_par->low_delay_ctrl.dly_ch;

	return E_OK;
}

static ER kdrv_ime_int_set_interrupt_enable(KDRV_IME_PRAM *pkdrv_ime_par, UINT32 *pIme_interrupt_enable_info)
{
	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set interrupt enable parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_interrupt_enable_info == NULL) {
		DBG_ERR("KDRV IME: set interrupt enable parameter NULL...\r\n");
		return E_PAR;
	}

	*pIme_interrupt_enable_info = pkdrv_ime_par->inte_en;

	return E_OK;
}

static ER kdrv_ime_int_set_yuv_cvt(KDRV_IME_PRAM *pkdrv_ime_par, IME_YUV_CVT_INFO *pIme_yuvcvt_info)
{
	if (pkdrv_ime_par == NULL) {
		DBG_ERR("KDRV IME: set yuv-cvt parameter NULL...\r\n");
		return E_PAR;
	}

	if (pIme_yuvcvt_info == NULL) {
		DBG_ERR("KDRV IME: set yuv-cvt parameter NULL...\r\n");
		return E_PAR;
	}

	//pIme_yuvcvt_info->yuv_cvt_enable = (IME_FUNC_EN)pkdrv_ime_par->sub_modules.yuv_cvt.enable;
	if (pIme_yuvcvt_info->yuv_cvt_enable == IME_FUNC_ENABLE) {
		pIme_yuvcvt_info->yuv_cvt_sel = (IME_YUV_CVT)pkdrv_ime_par->sub_modules.yuv_cvt.cvt_sel;
	}

	return E_OK;
}

static VOID kdrv_ime_int_set_param_init(UINT32 id)
{
	KDRV_IME_FUNC_CTRL func_sel = {0}, func_op = {0};
	uint32_t iq_en = 0;
	//unsigned long loc_status;

	/*compile warning: the frame size of 2880 bytes is larger than 1024 bytes, need check how to slove*/
	memset((void *)&ime, 0x0, sizeof(IME_MODE_PRAM));
	memset((void *)&ime_iq_info, 0x0, sizeof(IME_IQ_FLOW_INFO));
	memset((void *)&ime_lca_info, 0x0, sizeof(IME_CHROMA_ADAPTION_INFO));
	memset((void *)&ime_lca_sub_out_info, 0x0, sizeof(IME_CHROMA_ADAPTION_SUBOUT_INFO));
	memset((void *)&ime_dbcs_info, 0x0, sizeof(IME_DBCS_INFO));
	//memset((void)&ime_ssr_info, 0x0, sizeof(IME_SSR_INFO));
	//memset((void)&ime_cst_info, 0x0, sizeof(IME_CST_INFO));
	//memset((void)&ime_fg_info, 0x0, sizeof(IME_FILM_GRAIN_INFO));
	memset((void *)&ime_ds_info, 0x0, sizeof(IME_STAMP_INFO));
	memset((void *)&ime_stl_info, 0x0, sizeof(IME_STL_INFO));
	memset((void *)&ime_pm_info, 0x0, sizeof(IME_PM_INFO));
	memset((void *)&ime_tmnr_info, 0x0, sizeof(IME_TMNR_INFO));
	memset((void *)&ime_tmnr_refout_info, 0x0, sizeof(IME_TMNR_REF_OUT_INFO));

	memset((void *)&ime_yuv_cvt_info, 0x0, sizeof(IME_YUV_CVT_INFO));

	//static IME_MODE_PRAM ime = {0};
	//IME_IQ_FLOW_INFO ime_iq_info;
	//static IME_CHROMA_ADAPTION_INFO        ime_lca_info;        ///< chroma adaption parameters, if useless, please set NULL
	//static IME_CHROMA_ADAPTION_SUBOUT_INFO ime_lca_sub_out_info;    ///< chroma adaption subout parameters, if useless, please set NULL
	//static IME_DBCS_INFO                   ime_dbcs_info;          ///< dark and bright region chroma supression parameters, if useless, please set NULL
	//static IME_SSR_INFO                    ime_ssr_info;           ///< single image supre-resolution parameters, if useless, please set NULL
	//static IME_CST_INFO                    ime_cst_info;            ///< color space transformation parameters, if useless, please set NULL
	//static IME_FILM_GRAIN_INFO             ime_fg_info;            ///< film grain parameters, if useless, please set NULL
	//static IME_STAMP_INFO                  ime_ds_info;            ///< Data stamp parameters, if useless, please set NULL
	//static IME_STL_INFO                    ime_stl_info;           ///< edge statistic parameters, if useless, please set NULL
	//static IME_PM_INFO                     ime_pm_info;            ///< privacy mask parameters, if useless, please set NULL
	//static IME_TMNR_INFO                   ime_tmnr_info;         ///< 3DNR parameters
	//static IME_TMNR_REF_OUT_INFO           ime_tmnr_refout_info;  ///< 3dnr reference output


	//loc_status = kdrv_ime_spin_lock();

	// set IQ function enable with IPL flow enable control
	func_sel = g_ipl_func_en[id].func_sel;
	func_op  = g_ipl_func_en[id].func_op;

	if (func_sel.ime_ctrl_bit.bit.cmf == ENABLE) {
		iq_en = g_iq_func_en[id].ime_ctrl_bit.bit.cmf;

		if ((func_op.ime_ctrl_bit.bit.cmf == ENABLE) && (iq_en == ENABLE)) {
			//g_kdrv_ime_info[id].sub_modules.chroma_filter.enable = ENABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.cmf = ENABLE;
		} else {
			//g_kdrv_ime_info[id].sub_modules.chroma_filter.enable = DISABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.cmf = DISABLE;
		}
	}

#if 0
	if (func_sel.ime_ctrl_bit.bit.lca == ENABLE) {
		if ((func_op.ime_ctrl_bit.bit.lca == ENABLE) && (g_iq_func_en[id].ime_ctrl_bit.bit.lca == ENABLE)) {
			g_kdrv_ime_info[id].sub_modules.lca.enable = ENABLE;
		} else {
			g_kdrv_ime_info[id].sub_modules.lca.enable = DISABLE;
		}
	}
#endif

	if (func_sel.ime_ctrl_bit.bit.dbcs == ENABLE) {
		iq_en = g_iq_func_en[id].ime_ctrl_bit.bit.dbcs;

		if ((func_op.ime_ctrl_bit.bit.dbcs == ENABLE) && (iq_en == ENABLE)) {
			//g_kdrv_ime_info[id].sub_modules.dbcs.enable = ENABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.dbcs = ENABLE;
		} else {
			//g_kdrv_ime_info[id].sub_modules.dbcs.enable = DISABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.dbcs = DISABLE;
		}
	}

	if (func_sel.ime_ctrl_bit.bit.ssr == ENABLE) {
		iq_en = g_iq_func_en[id].ime_ctrl_bit.bit.ssr;

		if ((func_op.ime_ctrl_bit.bit.ssr == ENABLE) && (iq_en == ENABLE)) {
			//g_kdrv_ime_info[id].sub_modules.ssr.enable = ENABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.ssr = ENABLE;
		} else {
			//g_kdrv_ime_info[id].sub_modules.ssr.enable = DISABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.ssr = DISABLE;
		}
	}

	if (func_sel.ime_ctrl_bit.bit.fgn == ENABLE) {
		iq_en = g_iq_func_en[id].ime_ctrl_bit.bit.fgn;

		if ((func_op.ime_ctrl_bit.bit.fgn == ENABLE) && (iq_en == ENABLE)) {
			//g_kdrv_ime_info[id].sub_modules.film_grain.enable = ENABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.fgn = ENABLE;
		} else {
			//g_kdrv_ime_info[id].sub_modules.film_grain.enable = DISABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.fgn = DISABLE;
		}
	}

	if (func_sel.ime_ctrl_bit.bit.tmnr == ENABLE) {
		iq_en = g_iq_func_en[id].ime_ctrl_bit.bit.tmnr;

		if ((func_op.ime_ctrl_bit.bit.tmnr == ENABLE) && (iq_en == ENABLE)) {
			//g_kdrv_ime_info[id].sub_modules.tmnr.nr_iq.enable = ENABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.tmnr = ENABLE;
		} else {
			//g_kdrv_ime_info[id].sub_modules.tmnr.nr_iq.enable = DISABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.tmnr = DISABLE;
		}
	}

	if (func_sel.ime_ctrl_bit.bit.yuvcvt == ENABLE) {
		iq_en = g_iq_func_en[id].ime_ctrl_bit.bit.yuvcvt;

		if ((func_op.ime_ctrl_bit.bit.yuvcvt == ENABLE) && (iq_en == ENABLE)) {
			//g_kdrv_ime_info[id].sub_modules.yuv_cvt.enable = ENABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.yuvcvt = ENABLE;
		} else {
			//g_kdrv_ime_info[id].sub_modules.yuv_cvt.enable = DISABLE;
			g_ime_iq_func_en[id].ime_ctrl_bit.bit.yuvcvt = DISABLE;
		}
	}

	//kdrv_ime_spin_unlock(loc_status);
}


static ER kdrv_ime_setmode(UINT32 id)
{
	ER rt;
	UINT32 i;
	//unsigned long loc_status;

	//IME Input Parameters
	ime.interrupt_enable = IME_INTE_ALL;

	if (g_kdrv_ime_info[id].in_img_info.in_src == KDRV_IME_IN_SRC_DRAM) {
		ime.operation_mode = IME_OPMODE_D2D;
		//IME Stripe Setting
		ime.set_stripe.stripe_cal_mode = IME_STRIPE_AUTO_MODE; //auto stripe cal
	} else if (g_kdrv_ime_info[id].in_img_info.in_src == KDRV_IME_IN_SRC_DIRECT) {
		ime.operation_mode = IME_OPMODE_IFE_IPP;

		//IME Stripe Setting
		ime.set_stripe.stripe_cal_mode = IME_STRIPE_USER_MODE;
		ime.set_stripe.stp_h.stp_n = 0;
		ime.set_stripe.stp_h.stp_l = 0;
		ime.set_stripe.stp_h.stp_m = 0;
		ime.set_stripe.stp_v.stp_n = 0;
		ime.set_stripe.stp_v.stp_l = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height; //must be
		ime.set_stripe.stp_v.stp_m = 0;

		if (g_kdrv_ime_info[id].extend_info.stripe_num > 1 && g_kdrv_ime_info[id].extend_info.p1_enc_en == ENABLE) {
			ime.set_stripe.overlap_h_sel = IME_H_ST_OVLP_USER;
			ime.set_stripe.overlap_h_size = 256;
			ime.set_stripe.prt_h_sel = IME_H_ST_PRT_USER;
			ime.set_stripe.prt_h_size = 128;
		} else if (g_kdrv_ime_info[id].extend_info.stripe_num > 1 && g_kdrv_ime_info[id].extend_info.tmnr_en == ENABLE) {
			ime.set_stripe.overlap_h_sel = IME_H_ST_OVLP_USER;
			ime.set_stripe.overlap_h_size = 128;
			ime.set_stripe.prt_h_sel = IME_H_ST_PRT_USER;
			ime.set_stripe.prt_h_size = 64;
		} else {
			ime.set_stripe.overlap_h_sel = IME_H_ST_OVLP_32P;
			ime.set_stripe.prt_h_sel = IME_H_ST_PRT_5P;
		}
	} else if (g_kdrv_ime_info[id].in_img_info.in_src == KDRV_IME_IN_SRC_ALL_DIRECT) {
		ime.operation_mode = IME_OPMODE_SIE_IPP;

		//IME Stripe Setting
		ime.set_stripe.stripe_cal_mode = IME_STRIPE_USER_MODE;
		ime.set_stripe.stp_h.stp_n = 0;
		ime.set_stripe.stp_h.stp_l = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width >> 2;
		ime.set_stripe.stp_h.stp_m = 1;
		ime.set_stripe.stp_v.stp_n = 0;
		ime.set_stripe.stp_v.stp_l = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height; //must be
		ime.set_stripe.stp_v.stp_m = 0;

		ime.set_stripe.overlap_h_sel = IME_H_ST_OVLP_32P;
		ime.set_stripe.prt_h_sel = IME_H_ST_PRT_5P;
	} else {
		DBG_ERR("KDRV IME: HANDLE %d, input source %d error", (int)id, (int)g_kdrv_ime_info[id].in_img_info.in_src);
		return E_NOSPT;
	}

	ime.in_path_info.in_size.size_h = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width;
	ime.in_path_info.in_size.size_v = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height;

	if (ime.in_path_info.in_size.size_h % 4 != 0) {
		DBG_ERR("KDRV IME: HANDLE %d, main image width is not the mutiple of 4, %d\r\n", (int)id, (int)ime.in_path_info.in_size.size_h);
		return E_PAR;
	}

	if (ime.in_path_info.in_size.size_v % 4 != 0) {
		DBG_ERR("KDRV IME: HANDLE %d, main image height is not the mutiple of 4, %d\r\n", (int)id, (int)ime.in_path_info.in_size.size_v);
		return E_PAR;
	}

	ime.in_path_info.in_addr.addr_y = g_kdrv_ime_info[id].in_pixel_addr.addr_y;
	ime.in_path_info.in_addr.addr_cb = g_kdrv_ime_info[id].in_pixel_addr.addr_cb;
	ime.in_path_info.in_addr.addr_cr = g_kdrv_ime_info[id].in_pixel_addr.addr_cr;

	ime.in_path_info.in_lineoffset.lineoffset_y = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].line_ofs;
	ime.in_path_info.in_lineoffset.lineoffset_cb = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_U].line_ofs;
	ime.in_path_info.in_path_dma_flush = (IME_BUF_FLUSH_SEL)g_kdrv_ime_info[id].in_img_info.dma_flush;

	kdrv_ime_int_in_fmt_trans(g_kdrv_ime_info[id].in_img_info.type, &ime.in_path_info.in_format);

	//IME Output Parameters
	rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_1, &(g_kdrv_ime_info[id]), &(ime.out_path1));
	if (rt != E_OK) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_1 set out path info fail\r\n", (int)id);
		return rt;
	}
	rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_2, &(g_kdrv_ime_info[id]), &(ime.out_path2));
	if (rt != E_OK) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_2 set out path info fail\r\n", (int)id);
		return rt;
	}
	rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_3, &(g_kdrv_ime_info[id]), &(ime.out_path3));
	if (rt != E_OK) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_3 set out path info fail\r\n", (int)id);
		return rt;
	}
	rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_4, &(g_kdrv_ime_info[id]), &(ime.out_path4));
	if (rt != E_OK) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_4 set out path info fail\r\n", (int)id);
		return rt;
	}

	//rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_5, &g_kdrv_ime_info[id], &ime.out_path5);
	//if (rt != E_OK) {
	//  DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_5 set out path info fail\r\n", (int)id);
	//  return rt;
	//}

	//IME IQ Parameters
	//chroma median filter, 520 removed
	//ime_iq_info.pCmfInfo = (IME_FUNC_EN *)&g_kdrv_ime_info[id].sub_modules.chroma_filter.enable;
	if (g_ime_iq_func_en[id].ime_ctrl_bit.bit.cmf == ENABLE) {
		DBG_WRN("KDRV IME: do not support chroma median filter function\r\n");
	}

	//chroma adaption sub out info
	ime_iq_info.p_lca_subout_info = &ime_lca_sub_out_info;
	rt |= kdrv_ime_int_set_lca_sub_out_info(&(g_kdrv_ime_info[id]), &(g_kdrv_ime_info[id].sub_modules.lca_subout), &ime_lca_sub_out_info);
	//rt |= kdrv_ime_int_set_lca_sub_out_info(&(g_kdrv_ime_info[id].in_img_info), &(g_kdrv_ime_info[id].sub_modules.lca_subout), &ime_lca_sub_out_info);

	//chroma adaption parameters
	ime_iq_info.p_lca_info = &ime_lca_info;
	rt |= kdrv_ime_int_set_lca_info(&(g_kdrv_ime_info[id]), &ime_lca_info);

	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (ime_lca_info.lca_enable == ENABLE) {
			UINT32 stp_out_size;

			stp_out_size = (((ime_lca_info.lca_image_info.lca_img_size.size_h - 1) * (g_kdrv_ime_info[id].extend_info.stripe_h_max - 1)) / (g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width - 1)) + 1;
			if (stp_out_size > 672) {
				DBG_ERR("KDRV IME: HANDLE %d, LCA stripe size overflow, %d\r\n", (int)id, (int)stp_out_size);
				return E_PAR;
			}
		}
	} else if (nvt_get_chip_id() == CHIP_NA51084) {

    } else {
        DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }


	//dark and bright region chroma supression parameters
	ime_dbcs_info.dbcs_enable = (IME_FUNC_EN)g_ime_iq_func_en[id].ime_ctrl_bit.bit.dbcs;
	ime_iq_info.p_dbcs_info = &ime_dbcs_info;
	rt |= kdrv_ime_int_set_dbcs_info(&(g_kdrv_ime_info[id].sub_modules.dbcs), &ime_dbcs_info);

	//single image super-resolution parameters, 520 removed
	//ime_iq_info.pSsrInfo = &ime_ssr_info;
	//g_kdrv_ime_info[id].sub_modules.ssr.enable = DISABLE;
	rt |= kdrv_ime_int_set_ssr_info(&(g_kdrv_ime_info[id].sub_modules.ssr), NULL);
	if (g_ime_iq_func_en[id].ime_ctrl_bit.bit.ssr == ENABLE) {

		DBG_WRN("KDRV IME: do not support SSR function\r\n");
	}

	//color space transformation parameters, 520 removed
	//ime_iq_info.pColorSpaceTrans = &ime_cst_info;
	//ime_cst_info.Rgb2YccEnable = (IME_FUNC_EN)g_kdrv_ime_info[id].sub_modules.rgb2ycc.enable;
	//ime_cst_info.Ycc2RgbEnable = (IME_FUNC_EN)g_kdrv_ime_info[id].sub_modules.ycc2rgb.enable;
	g_kdrv_ime_info[id].sub_modules.rgb2ycc.enable = DISABLE;
	g_kdrv_ime_info[id].sub_modules.ycc2rgb.enable = DISABLE;
	if (g_kdrv_ime_info[id].sub_modules.rgb2ycc.enable == ENABLE) {
		DBG_WRN("KDRV IME: do not support RGB-to-YUV transform function\r\n");
	}

	if (g_kdrv_ime_info[id].sub_modules.ycc2rgb.enable == ENABLE) {
		DBG_WRN("KDRV IME: do not support YUV-to-RGB transform function\r\n");
	}


	//film grain parameters, 520 removed
	//ime_iq_info.p_film_grain_info = &ime_fg_info;
	//g_kdrv_ime_info[id].sub_modules.film_grain.enable = DISABLE;
	rt |= kdrv_ime_int_set_film_grain_info(&(g_kdrv_ime_info[id].sub_modules.film_grain), NULL);
	if (g_ime_iq_func_en[id].ime_ctrl_bit.bit.fgn == ENABLE) {
		DBG_WRN("KDRV IME: do not support grain noise function\r\n");
	}

	//Data stamp parameters
	ime_iq_info.p_data_stamp_info = &ime_ds_info;
	rt |= kdrv_ime_int_set_ds_info(&(g_kdrv_ime_info[id]), &ime_ds_info);

	//progressive to interlaced parameter, 520 removed
	//ime_iq_info.pP2IInfo = (IME_FUNC_EN *)&g_kdrv_ime_info[id].sub_modules.p2i.enable;
	g_kdrv_ime_info[id].sub_modules.p2i.enable = DISABLE;
	if (g_kdrv_ime_info[id].sub_modules.p2i.enable == ENABLE) {
		DBG_WRN("KDRV IME: do not support P2I function\r\n");
	}

	//for ADAS, not ready, wait pin check
	//edge statistic parameters
	ime_stl_info.stl_img_size.size_h = g_kdrv_ime_info[id].out_path_info[KDRV_IME_PATH_ID_4].crp_window.w;
	ime_stl_info.stl_img_size.size_v = g_kdrv_ime_info[id].out_path_info[KDRV_IME_PATH_ID_4].crp_window.h;
	ime_iq_info.p_sta_info = &ime_stl_info;
	rt |= kdrv_ime_int_set_stl_info(&(g_kdrv_ime_info[id].sub_modules.stl), &ime_stl_info);

	//privacy mask parameters
	ime_iq_info.p_pm_info = &ime_pm_info;
	rt |= kdrv_ime_int_set_pm_par(&(g_kdrv_ime_info[id]), &ime_pm_info);

	//3DNR parameters
	ime_tmnr_info.m_img_size.size_h = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width;
	ime_tmnr_info.m_img_size.size_v = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height;
	ime_tmnr_info.tmnr_en = (IME_FUNC_EN)g_ime_iq_func_en[id].ime_ctrl_bit.bit.tmnr;
	ime_iq_info.p_tmnr_info = &ime_tmnr_info;
	rt |= kdrv_ime_int_set_tmnr_par(&(g_kdrv_ime_info[id]), &ime_tmnr_info);

	ime_tmnr_refout_info.m_img_size.size_h = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width;
	ime_tmnr_refout_info.m_img_size.size_v = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height;
	ime_iq_info.p_tmnr_refout_info = &ime_tmnr_refout_info;
	rt |= kdrv_ime_int_set_tmnr_refout_par(&(g_kdrv_ime_info[id]), &ime_tmnr_refout_info);

	ime_yuv_cvt_info.yuv_cvt_enable = (IME_FUNC_EN)g_ime_iq_func_en[id].ime_ctrl_bit.bit.yuvcvt;
	ime_iq_info.p_yuv_cvt_info = &ime_yuv_cvt_info;
	rt |= kdrv_ime_int_set_yuv_cvt(&(g_kdrv_ime_info[id]), &ime_yuv_cvt_info);

	ime_comps_info.comp_param_mode = IME_PARAM_AUTO_MODE;
	ime_iq_info.p_comp_info = &ime_comps_info;

	rt |= kdrv_ime_int_set_break_point(&(g_kdrv_ime_info[id]), &(ime.break_point));

	rt |= kdrv_ime_int_set_single_output(&(g_kdrv_ime_info[id]), &(ime.single_out));

	rt |= kdrv_ime_int_set_low_delay(&(g_kdrv_ime_info[id]), &(ime.low_delay));

	rt |= kdrv_ime_int_set_interrupt_enable(&(g_kdrv_ime_info[id]), &(ime.interrupt_enable));


	ime.p_ime_iq_info = &ime_iq_info;
	rt |= ime_set_mode(&ime);

	if (ime_get_fsatboot_flow_ctrl_status() == ENABLE) {

		//loc_status = kdrv_ime_spin_lock();
		for (i = 0; i < KDRV_IME_PARAM_MAX; i++) {
			g_ime_update_param[id][i] = FALSE;
		}
		//kdrv_ime_spin_unlock(loc_status);
	}

	return rt;
}

#if 0
#endif

static void kdrv_ime_lock(KDRV_IME_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}


static KDRV_IME_HANDLE *kdrv_ime_check_handle(KDRV_IME_HANDLE *p_handle)
{
	UINT32 i;

	for (i = 0; i < kdrv_ime_channel_num; i ++) {
		if (p_handle == &g_kdrv_ime_handle_tab[i]) {
			return p_handle;
		}
	}

	return NULL;
}

static void kdrv_ime_handle_lock(void)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, kdrv_ime_get_flag_id(FLG_ID_KDRV_IME), KDRV_IPP_IME_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_ime_handle_unlock(void)
{
	set_flg(kdrv_ime_get_flag_id(FLG_ID_KDRV_IME), KDRV_IPP_IME_HDL_UNLOCK);
}

static BOOL kdrv_ime_channel_alloc(UINT32 chl_id)
{
	UINT32 i, j;
	//KDRV_IME_HANDLE *p_handle;
	BOOL eng_init_flag = 0;


	//p_handle = NULL;
	eng_init_flag = FALSE;
	i = chl_id;
	if (!(g_kdrv_ime_init_flg & (1 << i))) {
		//
		kdrv_ime_handle_lock();

		if (g_kdrv_ime_init_flg == 0) {
			eng_init_flag = TRUE;
		}

		g_kdrv_ime_init_flg |= (1 << i);

		memset(&g_kdrv_ime_handle_tab[i], 0, sizeof(KDRV_IME_HANDLE));
		memset(&g_kdrv_ime_clk_info[i], 0, sizeof(KDRV_IME_OPENCFG));
		memset(&g_kdrv_ime_info[i], 0, sizeof(KDRV_IME_PRAM));
		memset(&g_iq_func_en[i], 0, sizeof(KDRV_IME_FUNC_CTRL));
		memset(&g_ipl_func_en[i], 0, sizeof(KDRV_IME_FUNC_INT_CTRL));
		memset(&g_ime_iq_func_en[i], 0, sizeof(KDRV_IME_FUNC_CTRL));

		g_kdrv_ime_handle_tab[i].entry_id = i;
		g_kdrv_ime_handle_tab[i].flag_id = kdrv_ime_get_flag_id(FLG_ID_KDRV_IME);
		g_kdrv_ime_handle_tab[i].lock_bit = (1 << i);
		g_kdrv_ime_handle_tab[i].sts |= KDRV_IME_HANDLE_LOCK;
		g_kdrv_ime_handle_tab[i].sem_id = kdrv_ime_get_sem_id(SEMID_KDRV_IME);
		//p_handle = &g_kdrv_ime_handle_tab[i];

		for (j = 0; j < KDRV_IME_PARAM_MAX; j++) {
			g_ime_update_param[i][j] = FALSE;
		}

		g_kdrv_ime_info[i].sub_modules.tmnr.nr_iq = ime_tmnr_default;
		g_kdrv_ime_info[i].sub_modules.tmnr.nr_dbg = ime_tmnr_dbg_default;

		kdrv_ime_handle_unlock();
	}


	//if (i >= KDRV_IME_HANDLE_MAX_NUM) {
	//  DBG_ERR("get free handle fail(0x%.8x)\r\n", (unsigned int)g_kdrv_ime_init_flg);
	//}

	return eng_init_flag;
}

#if 0
static KDRV_IME_HANDLE *kdrv_ime_handle_alloc(UINT32 *eng_init_flag)
{
	UINT32 i;
	KDRV_IME_HANDLE *p_handle;

	kdrv_ime_handle_lock();
	p_handle = NULL;
	*eng_init_flag = FALSE;
	for (i = 0; i < KDRV_IME_HANDLE_MAX_NUM; i ++) {
		if (!(g_kdrv_ime_init_flg & (1 << i))) {

			if (g_kdrv_ime_init_flg == 0) {
				*eng_init_flag = TRUE;
			}

			g_kdrv_ime_init_flg |= (1 << i);

			memset(&g_kdrv_ime_handle_tab[i], 0, sizeof(KDRV_IME_HANDLE));
			g_kdrv_ime_handle_tab[i].entry_id = i;
			g_kdrv_ime_handle_tab[i].flag_id = kdrv_ime_get_flag_id(FLG_ID_KDRV_IME);
			g_kdrv_ime_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_ime_handle_tab[i].sts |= KDRV_IME_HANDLE_LOCK;
			g_kdrv_ime_handle_tab[i].sem_id = kdrv_ime_get_sem_id(SEMID_KDRV_IME);
			p_handle = &g_kdrv_ime_handle_tab[i];
			break;
		}
	}
	kdrv_ime_handle_unlock();

	if (i >= KDRV_IME_HANDLE_MAX_NUM) {
		DBG_ERR("get free handle fail(0x%.8x)\r\n", (unsigned int)g_kdrv_ime_init_flg);
	}
	return p_handle;
}

static UINT32 kdrv_ime_handle_free(KDRV_IME_HANDLE *p_handle)
{
	UINT32 rt = FALSE;
	kdrv_ime_handle_lock();
	p_handle->sts = 0;
	g_kdrv_ime_init_flg &= ~(1 << p_handle->entry_id);
	if (g_kdrv_ime_init_flg == 0) {
		rt = TRUE;
	}
	kdrv_ime_handle_unlock();

	return rt;
}
#endif

static KDRV_IME_HANDLE *kdrv_ime_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_ime_handle_tab[entry_id];
}


static void kdrv_ime_sem(KDRV_IME_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		SEM_WAIT(*p_handle->sem_id);    // wait semaphore
	} else {
		//SEM_SIGNAL(*p_handle->sem_id);  // wait semaphore
		SEM_SIGNAL_ISR(*p_handle->sem_id);  // wait semaphore
	}
}

static void kdrv_ime_frm_end(KDRV_IME_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		//set_flg(p_handle->flag_id, KDRV_IPP_IME_FMD);
		iset_flg(p_handle->flag_id, KDRV_IPP_IME_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_IPP_IME_FMD);
	}
}

static void kdrv_ime_isr_cb(UINT32 intstatus)
{
	KDRV_IME_HANDLE *p_handle;
	p_handle = g_kdrv_ime_trig_hdl;

	if (p_handle == NULL) {
		return;
	}

	if (intstatus & KDRV_IME_INT_FRM_END) {
		//ime_pause();    //set ime pause when ime frame end
		kdrv_ime_sem(p_handle, FALSE);
		kdrv_ime_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
	}
}

#if 0
#endif
INT32 kdrv_ime_open(UINT32 chip, UINT32 engine)
{
	IME_OPENOBJ ime_drv_obj = {0};
	//KDRV_IME_OPENCFG kdrv_ime_open_obj = {0};
	//KDRV_IME_HANDLE* p_handle;
	//UINT32 eng_init_flag;
	UINT32 eng_id = 0;
	//UINT32 i, j;

	switch (engine) {
	case KDRV_VIDEOPROCS_IME_ENGINE0:
		eng_id = KDRV_IME_ID_0;
		break;

	default:
		DBG_ERR("KDRV IME: engine id error...\r\n");
		return -1;
		break;
	}


	//p_handle = kdrv_ime_handle_alloc(&eng_init_flag);
	//if (p_handle == NULL) {
	//  DBG_WRN("KDRV IME: no free handle, max handle num = %d\r\n", KDRV_IME_HANDLE_MAX_NUM);
	//  return 0;
	//}

	//if (eng_init_flag) {
	if ((g_kdrv_ime_open_cnt[eng_id] == 0x0)) {
		//
		if (g_kdrv_ime_set_clk_flg[eng_id] != TRUE) {
			DBG_WRN("KDRV IME: no open info. from user, using default...\r\n");
		}

		//kdrv_ime_open_obj = g_kdrv_ime_clk_info[(UINT32)eng_id];
		//ime_drv_obj.uiImeClockSel = kdrv_ime_open_obj.ime_clock_sel;
		ime_drv_obj.uiImeClockSel = g_kdrv_ime_clk_info[(UINT32)eng_id].ime_clock_sel;
		ime_drv_obj.FP_IMEISR_CB = kdrv_ime_isr_cb;

		if (ime_open(&ime_drv_obj) != E_OK) {
			//kdrv_ime_handle_free(p_handle);
			DBG_WRN("KDRV IME: ime_open failed\r\n");
			return -1;
		}

		//memset((void *)g_kdrv_ime_info, 0x0, sizeof(KDRV_IME_PRAM)*kdrv_ime_channel_num);

		//for (j = 0; j < kdrv_ime_channel_num; j++) {
		//  for (i = 0; i < KDRV_IME_PARAM_MAX; i++) {
		//      g_ime_update_param[j][i] = FALSE;
		//  }
		//
		//  g_kdrv_ime_info[j].sub_modules.tmnr.nr_iq = ime_tmnr_default;
		//  g_kdrv_ime_info[j].sub_modules.tmnr.nr_dbg = ime_tmnr_dbg_default;
		//
		//}

	}

	g_kdrv_ime_open_cnt[eng_id] += 1;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		kdrv_ime_scaling_isd_stp_out_h_limit = 1344;
		kdrv_ime_stp_max = 2688;
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		kdrv_ime_scaling_isd_stp_out_h_limit = 2048;
		kdrv_ime_stp_max = 4096;
	} else {
        DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
        return -1;
    }


	return 0;
}

//--------------------------------------------------------------

INT32 kdrv_ime_pause(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;
	//KDRV_IME_HANDLE* p_handle;

	rt = ime_pause();
	if (rt != E_OK) {
		return -1;
	}

	return 0;
}

//--------------------------------------------------------------


INT32 kdrv_ime_close(UINT32 chip, UINT32 engine)
{
	//ER rt = E_OK;
	//KDRV_IME_HANDLE* p_handle;

	KDRV_IME_ID id = 0;


	switch (engine) {
	case KDRV_VIDEOPROCS_IME_ENGINE0:
		id = KDRV_IME_ID_0;

		g_kdrv_ime_open_cnt[id] -= 1;

		if (g_kdrv_ime_open_cnt[id] == 0) {
			if (ime_close() != E_OK) {
				//
				DBG_ERR("KDRV IME: engine close error...\r\n");
				return -1;
			}
			g_kdrv_ime_init_flg = 0;
			g_kdrv_ime_set_clk_flg[id] = FALSE;

			g_kdrv_ime_trig_hdl = NULL;
		}

		break;

	default:
		DBG_ERR("KDRV IME: engine id error...\r\n");
		return -1;
		break;
	}

#if 0
	p_handle = (KDRV_IME_HANDLE *)hdl;
	if (kdrv_ime_check_handle(p_handle) == NULL) {
		DBG_ERR("KDRV IME: illegal handle 0x%.8x\r\n", hdl);
		return E_SYS;
	}

	if ((p_handle->sts & KDRV_IME_HANDLE_LOCK) != KDRV_IME_HANDLE_LOCK) {
		DBG_ERR("KDRV IME: illegal handle sts 0x%.8x\r\n", p_handle->sts);
		return E_SYS;
	}

	kdrv_ime_lock(p_handle, TRUE);

	if (kdrv_ime_handle_free(p_handle)) {
		rt = ime_close();
	}

	kdrv_ime_lock(p_handle, FALSE);
#endif



	return 0;
}

//--------------------------------------------------------------

#if 0
#endif



static ER kdrv_ime_set_reset(UINT32 id, void *data)
{
	KDRV_IME_HANDLE *p_handle;

	if (ime_reset() != E_OK) {
		DBG_ERR("KDRV IME: engine reset error...\r\n");
		return E_PAR;
	}

	p_handle = kdrv_ime_entry_id_conv2_handle(id);
	kdrv_ime_sem(p_handle, FALSE);

	return E_OK;
}


static ER kdrv_ime_set_builtin_flow_disable(UINT32 id, void *data)
{
    ime_set_builtin_flow_disable();

    return E_OK;
}


static ER kdrv_ime_set_trig(UINT32 id, void *data)
{
	ER rt;
	KDRV_IME_HANDLE *p_handle;


	p_handle = kdrv_ime_entry_id_conv2_handle(id);
	kdrv_ime_sem(p_handle, TRUE);


	kdrv_ime_int_set_param_init(id);


	//set KDRV IME parameters to ime driver
	rt = kdrv_ime_setmode(id);
	if (rt != E_OK) {
		kdrv_ime_sem(p_handle, FALSE);
		return rt;
	}

	//update trig id and trig_cfg
	g_kdrv_ime_trig_hdl = p_handle;
	//trigger ime start
	kdrv_ime_frm_end(p_handle, FALSE);
	ime_clear_flag_frame_end();

	//trigger ime start
	rt = ime_start();
	if (rt != E_OK) {
		kdrv_ime_sem(p_handle, FALSE);
		return rt;
	}

	return rt;
}

static ER kdrv_ime_set_in_addr(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].in_pixel_addr = *(KDRV_IME_DMA_ADDR_INFO *)data;

	if (g_kdrv_ime_info[id].in_pixel_addr.addr_y == 0) {
		DBG_ERR("KDRV IME: HANDLE %d, set Invaid Input Address_Y: %#x\n", (int)id, (unsigned int)g_kdrv_ime_info[id].in_pixel_addr.addr_y);
		return E_PAR;
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_IN_ADDR] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_in_img_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set input image parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].in_img_info = *(KDRV_IME_IN_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_IN_IMG] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_out_addr(UINT32 id, void *data)
{
	KDRV_IME_OUT_PATH_ADDR_INFO path_addr_info;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: output image buffer address parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	path_addr_info = *(KDRV_IME_OUT_PATH_ADDR_INFO *)data;
	g_kdrv_ime_info[id].out_path_info[path_addr_info.path_id].pixel_addr = path_addr_info.addr_info;

	if (g_kdrv_ime_info[id].out_path_info[path_addr_info.path_id].pixel_addr.addr_y == 0) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_%d, set Invalid Input Address_Y: %#x\n", (int)id, (int)path_addr_info.path_id, (unsigned int)g_kdrv_ime_info[id].in_pixel_addr.addr_y);
		return E_PAR;
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_OUT_ADDR] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_out_img_info(UINT32 id, void *data)
{
	KDRV_IME_OUT_PATH_IMG_INFO img_info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set output image parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	img_info = *(KDRV_IME_OUT_PATH_IMG_INFO *)data;

	g_kdrv_ime_info[id].out_path_info[img_info.path_id].path_en = img_info.path_en;
	if (g_kdrv_ime_info[id].out_path_info[img_info.path_id].path_en == ENABLE) {
		//check output format
		if (img_info.path_id == KDRV_IME_PATH_ID_1) {
		} else if (img_info.path_id == KDRV_IME_PATH_ID_4) {
			if (img_info.type != KDRV_IME_OUT_FMT_Y) {
				DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_%d, set output format %d fail, path4 only support KDRV_IME_OUT_FMT_Y\r\n", (int)id, (int)(img_info.path_id + 1), (int)img_info.type);
				return E_NOSPT;
			}
		} else if (img_info.type != KDRV_IME_OUT_FMT_Y_PACK_UV422 && img_info.type != KDRV_IME_OUT_FMT_Y_PACK_UV420 && img_info.type != KDRV_IME_OUT_FMT_Y) {
			DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_%d, set output format %d fail, only support 444/422/420 Y/UV-Packed\r\n", (int)id, (int)(img_info.path_id + 1), (int)img_info.type);
			return E_NOSPT;
		}
		//check output size
		if (((img_info.crp_window.x + img_info.crp_window.w) > img_info.ch[KDRV_IPP_YUV_Y].width) || ((img_info.crp_window.y + img_info.crp_window.h) > img_info.ch[KDRV_IPP_YUV_Y].height)) {
			DBG_ERR("KDRV IME: HANDLE%d, KDRV_IME_PATH_ID_%d, set out img info fail, crop_x: %d + cro_w: %d need small than output_width: %d, crop_y: %d + cro_h: %d need small than output_height: %d",
					(int)id, (int)(img_info.path_id + 1),
					(int)img_info.crp_window.x, (int)img_info.crp_window.w, (int)img_info.ch[KDRV_IPP_YUV_Y].width,
					(int)img_info.crp_window.y, (int)img_info.crp_window.h, (int)img_info.ch[KDRV_IPP_YUV_Y].height);
			return E_NOSPT;
		}

		g_kdrv_ime_info[id].out_path_info[img_info.path_id].flip_en = img_info.path_flip_en;

		g_kdrv_ime_info[id].out_path_info[img_info.path_id].type = img_info.type;
		g_kdrv_ime_info[id].out_path_info[img_info.path_id].ch[KDRV_IPP_YUV_Y] = img_info.ch[KDRV_IPP_YUV_Y];
		g_kdrv_ime_info[id].out_path_info[img_info.path_id].ch[KDRV_IPP_YUV_U] = img_info.ch[KDRV_IPP_YUV_U];
		g_kdrv_ime_info[id].out_path_info[img_info.path_id].ch[KDRV_IPP_YUV_V] = img_info.ch[KDRV_IPP_YUV_V];
		g_kdrv_ime_info[id].out_path_info[img_info.path_id].crp_window = img_info.crp_window;

		g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_dma_flush = img_info.dma_flush;

		if (g_kdrv_ime_info[id].sub_modules.yuv_cvt.enable == ENABLE) {
			g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_range_clamp.min_y = 16;
			g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_range_clamp.max_y = 235;

			g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_range_clamp.min_uv = 16;
			g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_range_clamp.max_uv = 240;
		} else {
			g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_range_clamp.min_y = 0;
			g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_range_clamp.max_y = 255;

			g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_range_clamp.min_uv = 0;
			g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_range_clamp.max_uv = 255;
		}
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_OUT_IMG] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_out_sprt_addr(UINT32 id, void *data)
{
	KDRV_IME_OUT_SPRT_ADDR_INFO sprt_addr_info;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set output image buffer address parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	sprt_addr_info = *(KDRV_IME_OUT_SPRT_ADDR_INFO *)data;
	g_kdrv_ime_info[id].out_path_info[sprt_addr_info.path_id].out_sprt_addr = sprt_addr_info.sprt_addr;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_OUT_SPRT_ADDR] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

#if 0
	if (dma_isCacheAddr(g_kdrv_ime_info[id].out_path_info[sprt_addr_info.path_id].out_sprt_addr.addr_y)) {
		DBG_ERR("KDRV IME: HANDLE %d, set Invaid Address: %#x, address should be non_cacheable\r\n", id, g_kdrv_ime_info[id].out_path_info[sprt_addr_info.path_id].out_sprt_addr.addr_y);
		return E_PAR;
	}
#endif

	return E_OK;
}

static ER kdrv_ime_set_out_sprt_info(UINT32 id, void *data)
{
	KDRV_IME_OUT_SPRT_INFO img_info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set output stitching parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	img_info = *(KDRV_IME_OUT_SPRT_INFO *)data;

	g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_sprt_info.enable = img_info.enable;
	if (g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_sprt_info.enable == ENABLE) {
		g_kdrv_ime_info[id].out_path_info[img_info.path_id].out_sprt_info = img_info;

		if (img_info.sprt_pos % 4 != 0) {
			DBG_ERR("KDRV IME: separate position must be 4x...\r\n");
			return E_PAR;
		}
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_OUT_SPRT] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_extend_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set extend parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].extend_info = *(KDRV_IME_EXTEND_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_EXTEND] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_encode_info(UINT32 id, void *data)
{
	KDRV_IME_ENCODE_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_ENCODE_INFO *)data;
	g_kdrv_ime_info[id].out_path_info[KDRV_IME_PATH_ID_1].out_encode_info.enable = info.enable;
	//if (info.enable == ENABLE) {
	//  g_kdrv_ime_info[id].out_path_info[KDRV_IME_PATH_ID_1].out_encode_info = *(KDRV_IME_ENCODE_INFO *)data;
	//}

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_isrcb(UINT32 id, void *data)
{
	KDRV_IME_HANDLE *p_handle = NULL;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set call-back parameter NULL...\r\n");
		return E_PAR;
	}

	p_handle = kdrv_ime_entry_id_conv2_handle(id);

	p_handle->isrcb_fp = (KDRV_IPP_ISRCB)data;

	return E_OK;
}

static ER kdrv_ime_set_cmf(UINT32 id, void *data)
{
#if 0
	KDRV_IME_CMF_PARAM info = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	info = *(KDRV_IME_CMF_PARAM *)data;

	g_kdrv_ime_info[id].sub_modules.chroma_filter.enable = info.enable;
	g_iq_func_en[id].ime_ctrl_bit.bit.cmf = info.enable;
#endif

	DBG_ERR("KDRV IME: don't support chroma median filter function...\r\n");

	return E_OK;
}

static ER kdrv_ime_set_lca(UINT32 id, void *data)
{
	//KDRV_IME_LCA_PARAM info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set lca parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	//info = *(KDRV_IME_LCA_PARAM *)data;
	//g_kdrv_ime_info[id].sub_modules.lca.enable = info.enable;
	//g_iq_func_en[id].ime_ctrl_bit.bit.lca = info.enable;
	//if (info.enable == ENABLE) {
	//  g_kdrv_ime_info[id].sub_modules.lca = *(KDRV_IME_LCA_PARAM *)data;
	//}
	g_kdrv_ime_info[id].sub_modules.lca = *(KDRV_IME_LCA_PARAM *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IQ_LCA] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}
//-------------------------------------------------------------------------

static ER kdrv_ime_set_lca_en_ctrl(UINT32 id, void *data)
{
	KDRV_IME_LCA_CTRL_PARAM info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set lca function enable parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_LCA_CTRL_PARAM *)data;

	g_kdrv_ime_info[id].sub_modules.lca_en_ctrl.enable = info.enable;
	g_kdrv_ime_info[id].sub_modules.lca_en_ctrl.bypass = info.bypass;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_LCA_FUNC_EN] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}
//------------------------------------------------------------------------

static ER kdrv_ime_set_lca_ca_cent(UINT32 id, void *data)
{
	KDRV_IME_LCA_CA_CENT_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set lca-ca parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_LCA_CA_CENT_INFO *)data;
	g_kdrv_ime_info[id].sub_modules.lca_ca_cent = info;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_LCA_CA_CENT] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_lca_img_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set lca intput image parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.lca_img = *(KDRV_IME_LCA_IMG_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_LCA_REF_IMG] = TRUE;

#if 0
	if (g_kdrv_ime_info[id].sub_modules.lca_img_info.in_src == KDRV_IME_IN_SRC_DRAM && dma_isCacheAddr(g_kdrv_ime_info[id].sub_modules.lca_img_info.dma_addr)) {
		DBG_ERR("KDRV IME: HANDLE %d, set Invalid Input Address_Y: %#x, address should be non_cacheable\r\n", id, g_kdrv_ime_info[id].sub_modules.lca_img_info.dma_addr);
		return E_PAR;
	}
#endif

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_lca_sub_out(UINT32 id, void *data)
{
	KDRV_IME_LCA_SUBOUT_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set lca output image parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_LCA_SUBOUT_INFO *)data;
	g_kdrv_ime_info[id].sub_modules.lca_subout.enable = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.lca_subout = *(KDRV_IME_LCA_SUBOUT_INFO *)data;

#if 0
		if (dma_isCacheAddr(g_kdrv_ime_info[id].sub_modules.lca_subout.sub_out_addr)) {
			DBG_ERR("KDRV IME: HANDLE %d, set Invaid Input Address_Y: %#x, address should be non_cacheable\r\n", id, g_kdrv_ime_info[id].sub_modules.lca_subout.sub_out_addr);
			return E_PAR;
		}
#endif
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_LCA_SUB] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_dbcs(UINT32 id, void *data)
{
	KDRV_IME_DBCS_PARAM info = {0};
	//unsigned long loc_status;
	//UINT32 i = 0;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set dbcs parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_DBCS_PARAM *)data;
	g_kdrv_ime_info[id].sub_modules.dbcs.enable = info.enable;
	g_iq_func_en[id].ime_ctrl_bit.bit.dbcs = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.dbcs = *(KDRV_IME_DBCS_PARAM *)data;

#if 0
		//copy weighting table
		for (i = 0; i < KDRV_IME_DBCS_PARAM_WT_LUT_TAB; i++) {
			g_kdrv_ime_dbcs_luma_wt_y_tab[id][i] = g_kdrv_ime_info[id].sub_modules.dbcs.wt_y[i];
			g_kdrv_ime_dbcs_luma_wt_c_tab[id][i] = g_kdrv_ime_info[id].sub_modules.dbcs.wt_c[i];
		}
		g_kdrv_ime_info[id].sub_modules.dbcs.wt_y = &g_kdrv_ime_dbcs_luma_wt_y_tab[id][0];
		g_kdrv_ime_info[id].sub_modules.dbcs.wt_c = &g_kdrv_ime_dbcs_luma_wt_c_tab[id][0];
#endif
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IQ_DBCS] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_ref_in_addr_use_va2pa_en(UINT32 id, void *data)
{
    BOOL get_set_en;

    if (data == NULL) {
		DBG_ERR("KDRV IME: set using va2pa parameter NULL...\r\n");
		return E_PAR;
	}

    get_set_en = *(BOOL *)data;
    ime_chg_tmnr_ref_in_addr_use_va2pa(get_set_en);

    return E_OK;
}

static ER kdrv_ime_set_ssr(UINT32 id, void *data)
{
#if 0
	KDRV_IME_SSR_PARAM info = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	info = *(KDRV_IME_SSR_PARAM *)data;
	g_kdrv_ime_info[id].sub_modules.ssr.enable = info.enable;
	g_iq_func_en[id].ime_ctrl_bit.bit.ssr = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.ssr = *(KDRV_IME_SSR_PARAM *)data;
	}
#endif

	DBG_ERR("KDRV IME: don't support SSR function...\r\n");

	return E_OK;
}

#if 0
static ER kdrv_ime_set_cst(UINT32 id, void *data)
{
	g_kdrv_ime_info[id].sub_modules.color_space_trans = *(KDRV_IME_CST_PARAM *)data;

	return E_OK;
}
#endif

static ER kdrv_ime_set_rgb2ycc(UINT32 id, void *data)
{
#if 0
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	g_kdrv_ime_info[id].sub_modules.rgb2ycc = *(KDRV_IME_RGB2YCC_INFO *)data;
#endif

	DBG_ERR("KDRV IME: don't support rgb2ycc function...\r\n");

	return E_OK;
}

static ER kdrv_ime_set_ycc2rgb(UINT32 id, void *data)
{
#if 0
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	g_kdrv_ime_info[id].sub_modules.ycc2rgb = *(KDRV_IME_YCC2RGB_INFO *)data;
#endif

	DBG_ERR("KDRV IME: don't support ycc2rgb function...\r\n");

	return E_OK;
}

static ER kdrv_ime_set_fgn(UINT32 id, void *data)
{
#if 0
	KDRV_IME_FGN_PARAM info = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	info = *(KDRV_IME_FGN_PARAM *)data;
	g_kdrv_ime_info[id].sub_modules.film_grain.enable = info.enable;
	g_iq_func_en[id].ime_ctrl_bit.bit.fgn = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.film_grain = *(KDRV_IME_FGN_PARAM *)data;
	}
#endif

	DBG_ERR("KDRV IME: don't support file-grain-noise function...\r\n");

	return E_OK;
}

static ER kdrv_ime_set_ds(UINT32 id, void *data)
{
	KDRV_IME_OSD_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set osd parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_OSD_INFO *)data;

	g_kdrv_ime_info[id].sub_modules.ds[info.ds_set_idx].enable = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.ds[info.ds_set_idx] = *(KDRV_IME_OSD_INFO *)data;
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_OSD] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_ds_cst(UINT32 id, void *data)
{
	KDRV_IME_OSD_CST_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set osd-cst parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_OSD_CST_INFO *)data;
	g_kdrv_ime_info[id].sub_modules.ds_cst.enable = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.ds_cst = *(KDRV_IME_OSD_CST_INFO *)data;
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_OSD_CST] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_ds_plt(UINT32 id, void *data)
{
	KDRV_IME_OSD_PLT_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set osd-palette parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_OSD_PLT_INFO *)data;

#if 0
	if (info.plt_a == NULL) {
		DBG_ERR("KDRV IME: set osd-palette-A parameter NULL...\r\n");
		return E_PAR;
	}

	if (info.plt_r == NULL) {
		DBG_ERR("KDRV IME: set osd-palette-R parameter NULL...\r\n");
		return E_PAR;
	}

	if (info.plt_g == NULL) {
		DBG_ERR("KDRV IME: set osd-palette-G parameter NULL...\r\n");
		return E_PAR;
	}

	if (info.plt_b == NULL) {
		DBG_ERR("KDRV IME: set osd-palette-B parameter NULL...\r\n");
		return E_PAR;
	}
#endif

	g_kdrv_ime_info[id].sub_modules.ds_plt = info;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_OSD_PLT] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);


	return E_OK;
}



static ER kdrv_ime_set_p2i(UINT32 id, void *data)
{
#if 0
	KDRV_IME_P2I_INFO info = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	info = *(KDRV_IME_P2I_INFO *)data;

	g_kdrv_ime_info[id].sub_modules.p2i.enable = info.enable;
#endif

	DBG_ERR("KDRV IME: don't support P2I function...\r\n");

	return E_OK;
}

static ER kdrv_ime_set_stl(UINT32 id, void *data)
{
	KDRV_IME_STL_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set statistical parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_STL_INFO *)data;
	g_kdrv_ime_info[id].sub_modules.stl.stl_info.enable = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.stl.stl_info = *(KDRV_IME_STL_INFO *)data;
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_STL] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_stl_roi(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set statistical ROI parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.stl.stl_roi = *(KDRV_IME_STL_ROI_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_STL_ROI] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_stl_hist(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set statistical histogram parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.stl.stl_hist = *(KDRV_IME_STL_HIST_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_STL_HIST] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_stl_out_img(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: statistical output image parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.stl.stl_out_img = *(KDRV_IME_STL_OUT_IMG_INFO *)data;

#if 0
	if (dma_isCacheAddr(g_kdrv_ime_info[id].sub_modules.stl.stl_out_img.edge_map_addr)) {
		DBG_ERR("KDRV IME: HANDLE %d, set Invaid Address: %#x, address should be non_cacheable\r\n", id, g_kdrv_ime_info[id].sub_modules.stl.stl_out_img.edge_map_addr);
		return E_PAR;
	}

	if (dma_isCacheAddr(g_kdrv_ime_info[id].sub_modules.stl.stl_out_img.hist_addr)) {
		DBG_ERR("KDRV IME: HANDLE %d, set Invaid Address: %#x, address should be non_cacheable\r\n", id, g_kdrv_ime_info[id].sub_modules.stl.stl_out_img.hist_addr);
		return E_PAR;
	}
#endif

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_STL_OUT_IMG] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_pm(UINT32 id, void *data)
{
	KDRV_IME_PM_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set privacy mask parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_PM_INFO *)data;

	g_kdrv_ime_info[id].sub_modules.pm.pm_info[info.set_idx].enable = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.pm.pm_info[info.set_idx] = info;
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_PM] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_pm_pxl_img_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set privay mask pixlation parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.pm.pm_pxl_img_info = *(KDRV_IME_PM_PXL_IMG_INFO *)data;

#if 0
	if (dma_isCacheAddr(g_kdrv_ime_info[id].sub_modules.pm.pm_pxl_img_info.dma_addr)) {
		DBG_ERR("KDRV IME: HANDLE %d, set Invaid Input Address: %#x, address should be non_cacheable\r\n", id, g_kdrv_ime_info[id].sub_modules.pm.pm_pxl_img_info.dma_addr);
		return E_PAR;
	}
#endif

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_PM_PXL_IMG] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_tmnr(UINT32 id, void *data)
{
	//UINT32 i= 0;
	KDRV_IME_TMNR_PARAM info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set tmnr parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_TMNR_PARAM *)data;
	g_kdrv_ime_info[id].sub_modules.tmnr.nr_iq.enable = info.enable;
	g_iq_func_en[id].ime_ctrl_bit.bit.tmnr = info.enable;
	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.tmnr.nr_iq = *(KDRV_IME_TMNR_PARAM *)data;
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IQ_TMNR] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_tmnr_ref_in_img(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set tmnr input reference image parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.tmnr.nr_ref_in_img = *(KDRV_IME_TMNR_REF_IN_IMG_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_TMNR_REF_IN_IMG] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_tmnr_ref_out_img(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set tmnr output reference image parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.tmnr_refout.nr_ref_out_img = *(KDRV_IME_TMNR_REF_OUT_IMG_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_TMNR_REF_OUT_IMG] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_tmnr_motion_status_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set tmnr input and output motion status parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.tmnr.nr_ms = *(KDRV_IME_TMNR_MOTION_STATUS_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_TMNR_MS] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_tmnr_motion_status_roi_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set tmnr output motion status parameter for ROI NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.tmnr.nr_ms_roi = *(KDRV_IME_TMNR_MOTION_STATUS_ROI_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_TMNR_MS_ROI] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_tmnr_motion_vector_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set tmnr input and output motion vector parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.tmnr.nr_mv = *(KDRV_IME_TMNR_MOTION_VECTOR_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_TMNR_MV] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_tmnr_motion_statistic_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set tmnr output statistic data parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.tmnr.nr_sta = *(KDRV_IME_TMNR_STATISTIC_DATA_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_TMNR_STA] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ime_set_tmnr_dbg(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set tmnr output statistic data parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].sub_modules.tmnr.nr_dbg = *(KDRV_IME_TMNR_DBG_PARAM *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IQ_TMNR_DBG] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_scl_method_sel(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set scaling method parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].out_scl_method_sel = *(KDRV_IME_SCL_METHOD_SEL_INFO *)data;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_SCL_METHOD_SEL] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_ipl_func_enable(UINT32 id, void *data)
{
	//unsigned long loc_status;
	KDRV_IME_PARAM_IPL_FUNC_EN info = {0};
	KDRV_IME_FUNC_CTRL get_func_sel = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IME: set function enable parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_PARAM_IPL_FUNC_EN *)data;

	get_func_sel.ime_ctrl_bit.val = info.ipl_ctrl_func;

	//----------------------------------------------------------------
	// set cmf
	//if (get_func_sel.ime_ctrl_bit.bit.cmf == ENABLE) {
	g_ipl_func_en[id].func_sel.ime_ctrl_bit.bit.cmf = DISABLE;
	g_ipl_func_en[id].func_op.ime_ctrl_bit.bit.cmf = DISABLE;
	//}

	//----------------------------------------------------------------
	// set lca
	//if (get_func_sel.ime_ctrl_bit.bit.lca == ENABLE) {
	//  g_ipl_func_en[id].func_sel.ime_ctrl_bit.bit.lca = ENABLE;
	//  g_ipl_func_en[id].func_op.ime_ctrl_bit.bit.lca = ((info.enable == TRUE) ? ENABLE : DISABLE);
	//}

	//----------------------------------------------------------------
	// set dbcs
	if (get_func_sel.ime_ctrl_bit.bit.dbcs == ENABLE) {
		g_ipl_func_en[id].func_sel.ime_ctrl_bit.bit.dbcs = ENABLE;
		g_ipl_func_en[id].func_op.ime_ctrl_bit.bit.dbcs = ((info.enable == TRUE) ? ENABLE : DISABLE);
	}

	//----------------------------------------------------------------
	// set ssr
	//if (get_func_sel.ime_ctrl_bit.bit.ssr == ENABLE) {
	g_ipl_func_en[id].func_sel.ime_ctrl_bit.bit.ssr = DISABLE;
	g_ipl_func_en[id].func_op.ime_ctrl_bit.bit.ssr = DISABLE;
	//}

	//----------------------------------------------------------------
	// set fgn
	//if (get_func_sel.ime_ctrl_bit.bit.fgn == ENABLE) {
	g_ipl_func_en[id].func_sel.ime_ctrl_bit.bit.fgn = DISABLE;
	g_ipl_func_en[id].func_op.ime_ctrl_bit.bit.fgn = DISABLE;
	//}

	//----------------------------------------------------------------
	// set tmnr
	if (get_func_sel.ime_ctrl_bit.bit.tmnr == ENABLE) {
		g_ipl_func_en[id].func_sel.ime_ctrl_bit.bit.tmnr = ENABLE;
		g_ipl_func_en[id].func_op.ime_ctrl_bit.bit.tmnr = ((info.enable == TRUE) ? ENABLE : DISABLE);
	}

	//----------------------------------------------------------------
	// set yuc-cvt
	if (get_func_sel.ime_ctrl_bit.bit.yuvcvt == ENABLE) {
		g_ipl_func_en[id].func_sel.ime_ctrl_bit.bit.yuvcvt = ENABLE;
		g_ipl_func_en[id].func_op.ime_ctrl_bit.bit.yuvcvt = ((info.enable == TRUE) ? ENABLE : DISABLE);
	}

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ime_set_break_point(UINT32 id, void *data)
{
	//UINT32 i= 0;
	KDRV_IME_BREAK_POINT_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set break-point parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_BREAK_POINT_INFO *)data;

	g_kdrv_ime_info[id].break_point_ctrl = info;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_BREAK_POINT] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_single_output(UINT32 id, void *data)
{
	//UINT32 i= 0;
	KDRV_IME_SINGLE_OUT_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set single output parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_SINGLE_OUT_INFO *)data;

	g_kdrv_ime_info[id].single_out_ctrl = info;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_SINGLE_OUTPUT] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ime_set_low_delay(UINT32 id, void *data)
{
	//UINT32 i= 0;
	KDRV_IME_LOW_DELAY_INFO info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set low dealy parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_LOW_DELAY_INFO *)data;

	g_kdrv_ime_info[id].low_delay_ctrl = info;

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_LOW_DELAY] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ime_set_inte_en(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	g_kdrv_ime_info[id].inte_en = *((UINT32 *)data);

	g_ime_update_param[id][KDRV_IME_PARAM_IPL_INTER] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ime_set_ycc_cvt(UINT32 id, void *data)
{
	UINT32 i;
	KDRV_IME_YCC_CVT_PARAM info = {0};
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IME: set ycc-cvt parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = kdrv_ime_spin_lock();

	info = *(KDRV_IME_YCC_CVT_PARAM *)data;
	g_kdrv_ime_info[id].sub_modules.yuv_cvt.enable = info.enable;
	g_iq_func_en[id].ime_ctrl_bit.bit.yuvcvt = info.enable;

	if (info.enable == ENABLE) {
		g_kdrv_ime_info[id].sub_modules.yuv_cvt = *(KDRV_IME_YCC_CVT_PARAM *)data;
	}


	for (i = 0; i < (UINT32)KDRV_IME_PATH_ID_MAX; i++) {
		if (g_kdrv_ime_info[id].sub_modules.yuv_cvt.enable == ENABLE) {
			g_kdrv_ime_info[id].out_path_info[(KDRV_IME_PATH_ID)i].out_range_clamp.min_y = 16;
			g_kdrv_ime_info[id].out_path_info[(KDRV_IME_PATH_ID)i].out_range_clamp.max_y = 235;

			g_kdrv_ime_info[id].out_path_info[(KDRV_IME_PATH_ID)i].out_range_clamp.min_uv = 16;
			g_kdrv_ime_info[id].out_path_info[(KDRV_IME_PATH_ID)i].out_range_clamp.max_uv = 240;
		} else {
			g_kdrv_ime_info[id].out_path_info[(KDRV_IME_PATH_ID)i].out_range_clamp.min_y = 0;
			g_kdrv_ime_info[id].out_path_info[(KDRV_IME_PATH_ID)i].out_range_clamp.max_y = 255;

			g_kdrv_ime_info[id].out_path_info[(KDRV_IME_PATH_ID)i].out_range_clamp.min_uv = 0;
			g_kdrv_ime_info[id].out_path_info[(KDRV_IME_PATH_ID)i].out_range_clamp.max_uv = 255;
		}
	}

	g_ime_update_param[id][KDRV_IME_PARAM_IQ_YCCCVT] = TRUE;

	//kdrv_ime_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ime_set_iq_all(UINT32 id, void *data)
{
	ER retval;

	KDRV_IME_PARAM_IQ_ALL_PARAM info = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IME: set all IQ parameter NULL...\r\n");
		return E_PAR;
	}

	info = *(KDRV_IME_PARAM_IQ_ALL_PARAM *)data;

#if 0
	if (info.p_cmf != NULL) {
		retval = kdrv_ime_set_cmf(id, (void *)info.p_cmf);
		if (retval != E_OK) {
			return retval;
		}
	}
#endif

	if (info.p_lca != NULL) {
		retval = kdrv_ime_set_lca(id, (void *)info.p_lca);
		if (retval != E_OK) {
			return retval;
		}
	}

	if (info.p_dbcs != NULL) {
		retval = kdrv_ime_set_dbcs(id, (void *)info.p_dbcs);
		if (retval != E_OK) {
			return retval;
		}
	}

#if 0
	if (info.p_ssr != NULL) {
		retval = kdrv_ime_set_ssr(id, (void *)info.p_ssr);
		if (retval != E_OK) {
			return retval;
		}
	}
#endif

#if 0
	if (info.p_fgn != NULL) {
		retval = kdrv_ime_set_fgn(id, (void *)info.p_fgn);
		if (retval != E_OK) {
			return retval;
		}
	}
#endif

	if (info.p_tmnr != NULL) {
		retval = kdrv_ime_set_tmnr(id, (void *)info.p_tmnr);
		if (retval != E_OK) {
			return retval;
		}
	}

	if (info.p_tmnr_dbg != NULL) {
		retval = kdrv_ime_set_tmnr_dbg(id, (void *)info.p_tmnr_dbg);
		if (retval != E_OK) {
			return retval;
		}
	}

	if (info.p_ycccvt != NULL) {
		retval = kdrv_ime_set_ycc_cvt(id, (void *)info.p_ycccvt);
		if (retval != E_OK) {
			return retval;
		}
	}

	return E_OK;
}



static ER kdrv_ime_set_load(UINT32 id, void *data)
{
	ER rt;
	uint32_t set_load_check;
	uint32_t i;
	//unsigned long loc_status;

	//loc_status = kdrv_ime_spin_lock();

	set_load_check = 0;
	for (i = 0; i < KDRV_IME_PARAM_MAX; i++) {
		g_ime_int_update_param[i] = g_ime_update_param[id][i];

		set_load_check += (uint32_t)g_ime_update_param[id][i];

		g_ime_update_param[id][i] = FALSE;
	}

	//kdrv_ime_spin_unlock(loc_status);

	if (set_load_check == 0) {
		return E_OK;
	}


	kdrv_ime_int_set_param_init(id);


	//IME Input Parameters
	ime.interrupt_enable = IME_INTE_ALL;
	if (g_kdrv_ime_info[id].in_img_info.in_src == KDRV_IME_IN_SRC_DRAM) {
		ime.operation_mode = IME_OPMODE_D2D;
		//IME Stripe Setting
		ime.set_stripe.stripe_cal_mode = IME_STRIPE_AUTO_MODE; //auto stripe cal
	} else if (g_kdrv_ime_info[id].in_img_info.in_src == KDRV_IME_IN_SRC_DIRECT) {
		ime.operation_mode = IME_OPMODE_IFE_IPP;

		//IME Stripe Setting
		ime.set_stripe.stripe_cal_mode = IME_STRIPE_USER_MODE;
		ime.set_stripe.stp_h.stp_n = 0;
		ime.set_stripe.stp_h.stp_l = 0;
		ime.set_stripe.stp_h.stp_m = 0;
		ime.set_stripe.stp_v.stp_n = 0;
		ime.set_stripe.stp_v.stp_l = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height; //must be
		ime.set_stripe.stp_v.stp_m = 0;

		if (g_kdrv_ime_info[id].extend_info.stripe_num > 1 && g_kdrv_ime_info[id].extend_info.p1_enc_en == ENABLE) {
			ime.set_stripe.overlap_h_sel = IME_H_ST_OVLP_USER;
			ime.set_stripe.overlap_h_size = 256;
			ime.set_stripe.prt_h_sel = IME_H_ST_PRT_USER;
			ime.set_stripe.prt_h_size = 128;
		} else if (g_kdrv_ime_info[id].extend_info.stripe_num > 1 && g_kdrv_ime_info[id].extend_info.tmnr_en == ENABLE) {
			ime.set_stripe.overlap_h_sel = IME_H_ST_OVLP_USER;
			ime.set_stripe.overlap_h_size = 128;
			ime.set_stripe.prt_h_sel = IME_H_ST_PRT_USER;
			ime.set_stripe.prt_h_size = 64;
		} else {
			ime.set_stripe.overlap_h_sel = IME_H_ST_OVLP_32P;
			ime.set_stripe.prt_h_sel = IME_H_ST_PRT_5P;
		}
	} else if (g_kdrv_ime_info[id].in_img_info.in_src == KDRV_IME_IN_SRC_ALL_DIRECT) {
		ime.operation_mode = IME_OPMODE_SIE_IPP;

		//IME Stripe Setting
		ime.set_stripe.stripe_cal_mode = IME_STRIPE_USER_MODE;
		ime.set_stripe.stp_h.stp_n = 0;
		ime.set_stripe.stp_h.stp_l = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width >> 2;
		ime.set_stripe.stp_h.stp_m = 1;
		ime.set_stripe.stp_v.stp_n = 0;
		ime.set_stripe.stp_v.stp_l = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height; //must be
		ime.set_stripe.stp_v.stp_m = 0;

		ime.set_stripe.overlap_h_sel = IME_H_ST_OVLP_32P;
		ime.set_stripe.prt_h_sel = IME_H_ST_PRT_5P;
	} else {
		DBG_ERR("KDRV IME: HANDLE %d, input source %d error", (int)id, (int)g_kdrv_ime_info[id].in_img_info.in_src);
		return E_NOSPT;
	}


	ime.in_path_info.in_size.size_h = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width;
	ime.in_path_info.in_size.size_v = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height;
	if (ime.in_path_info.in_size.size_h % 4 != 0) {
		DBG_ERR("KDRV IME: HANDLE %d, main image width is not the mutiple of 4, %d\r\n", (int)id, (int)ime.in_path_info.in_size.size_h);
		return E_PAR;
	}

	if (ime.in_path_info.in_size.size_v % 4 != 0) {
		DBG_ERR("KDRV IME: HANDLE %d, main image height is not the mutiple of 4, %d\r\n", (int)id, (int)ime.in_path_info.in_size.size_v);
		return E_PAR;
	}

	ime.in_path_info.in_addr.addr_y = g_kdrv_ime_info[id].in_pixel_addr.addr_y;
	ime.in_path_info.in_addr.addr_cb = g_kdrv_ime_info[id].in_pixel_addr.addr_cb;
	ime.in_path_info.in_addr.addr_cr = g_kdrv_ime_info[id].in_pixel_addr.addr_cr;

	ime.in_path_info.in_lineoffset.lineoffset_y = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].line_ofs;
	ime.in_path_info.in_lineoffset.lineoffset_cb = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_U].line_ofs;
	kdrv_ime_int_in_fmt_trans(g_kdrv_ime_info[id].in_img_info.type, &ime.in_path_info.in_format);

	//IME Output Parameters
	rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_1, &(g_kdrv_ime_info[id]), &(ime.out_path1));
	if (rt != E_OK) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_1 set out path info fail\r\n", (int)id);
		return rt;
	}
	rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_2, &(g_kdrv_ime_info[id]), &(ime.out_path2));
	if (rt != E_OK) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_2 set out path info fail\r\n", (int)id);
		return rt;
	}
	rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_3, &(g_kdrv_ime_info[id]), &(ime.out_path3));
	if (rt != E_OK) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_3 set out path info fail\r\n", (int)id);
		return rt;
	}
	rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_4, &(g_kdrv_ime_info[id]), &(ime.out_path4));
	if (rt != E_OK) {
		DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_4 set out path info fail\r\n", (int)id);
		return rt;
	}

	//rt = kdrv_ime_int_set_out_path_info(KDRV_IME_PATH_ID_5, &g_kdrv_ime_info[id], &ime.out_path5);
	//if (rt != E_OK) {
	//  DBG_ERR("KDRV IME: HANDLE %d, KDRV_IME_PATH_ID_5 set out path info fail\r\n", (int)id);
	//  return rt;
	//}

	//IME IQ Parameters
	//chroma median filter, 520 removed
	//ime_iq_info.pCmfInfo = (IME_FUNC_EN *)&g_kdrv_ime_info[id].sub_modules.chroma_filter.enable;
	if (g_ime_iq_func_en[id].ime_ctrl_bit.bit.cmf == ENABLE) {
		DBG_WRN("KDRV IME: do not support chroma median filter function\r\n");
	}

	//chroma adaption sub out info
	ime_iq_info.p_lca_subout_info = &ime_lca_sub_out_info;
	rt |= kdrv_ime_int_set_lca_sub_out_info(&(g_kdrv_ime_info[id]), &(g_kdrv_ime_info[id].sub_modules.lca_subout), &ime_lca_sub_out_info);
	//rt |= kdrv_ime_int_set_lca_sub_out_info(&(g_kdrv_ime_info[id].in_img_info), &(g_kdrv_ime_info[id].sub_modules.lca_subout), &ime_lca_sub_out_info);


	//chroma adaption parameters
	ime_iq_info.p_lca_info = &ime_lca_info;
	rt |= kdrv_ime_int_set_lca_info(&(g_kdrv_ime_info[id]), &ime_lca_info);

	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (ime_lca_info.lca_enable == ENABLE) {
			UINT32 stp_out_size;

			stp_out_size = (((ime_lca_info.lca_image_info.lca_img_size.size_h - 1) * (g_kdrv_ime_info[id].extend_info.stripe_h_max - 1)) / (g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width - 1)) + 1;
			if (stp_out_size > 672) {
				DBG_ERR("KDRV IME: HANDLE %d, LCA stripe size overflow, %d\r\n", (int)id, (int)stp_out_size);
				return E_PAR;
			}
		}
	} else if (nvt_get_chip_id() == CHIP_NA51084) {

    } else {
        DBG_ERR("IME: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }

	//dark and bright region chroma supression parameters
	ime_dbcs_info.dbcs_enable = (IME_FUNC_EN)g_ime_iq_func_en[id].ime_ctrl_bit.bit.dbcs;
	ime_iq_info.p_dbcs_info = &ime_dbcs_info;
	rt |= kdrv_ime_int_set_dbcs_info(&(g_kdrv_ime_info[id].sub_modules.dbcs), &ime_dbcs_info);

	//single image super-resolution parameters, 520 removed
	//ime_iq_info.pSsrInfo = &ime_ssr_info;
	//g_kdrv_ime_info[id].sub_modules.ssr.enable = DISABLE;
	rt |= kdrv_ime_int_set_ssr_info(&(g_kdrv_ime_info[id].sub_modules.ssr), NULL);
	if (g_ime_iq_func_en[id].ime_ctrl_bit.bit.ssr == ENABLE) {

		DBG_WRN("KDRV IME: do not support SSR function\r\n");
	}

	//color space transformation parameters, 520 removed
	//ime_iq_info.pColorSpaceTrans = &ime_cst_info;
	//ime_cst_info.Rgb2YccEnable = (IME_FUNC_EN)g_kdrv_ime_info[id].sub_modules.rgb2ycc.enable;
	//ime_cst_info.Ycc2RgbEnable = (IME_FUNC_EN)g_kdrv_ime_info[id].sub_modules.ycc2rgb.enable;
	g_kdrv_ime_info[id].sub_modules.rgb2ycc.enable = DISABLE;
	g_kdrv_ime_info[id].sub_modules.ycc2rgb.enable = DISABLE;
	if (g_kdrv_ime_info[id].sub_modules.rgb2ycc.enable == ENABLE) {
		DBG_WRN("KDRV IME: do not support RGB-to-YUV transform function\r\n");
	}

	if (g_kdrv_ime_info[id].sub_modules.ycc2rgb.enable == ENABLE) {
		DBG_WRN("KDRV IME: do not support YUV-to-RGB transform function\r\n");
	}


	//film grain parameters, 520 removed
	//ime_iq_info.p_film_grain_info = &ime_fg_info;
	//g_kdrv_ime_info[id].sub_modules.film_grain.enable = DISABLE;
	rt |= kdrv_ime_int_set_film_grain_info(&(g_kdrv_ime_info[id].sub_modules.film_grain), NULL);
	if (g_ime_iq_func_en[id].ime_ctrl_bit.bit.fgn == ENABLE) {
		DBG_WRN("KDRV IME: do not support grain noise function\r\n");
	}

	//Data stamp parameters
	ime_iq_info.p_data_stamp_info = &ime_ds_info;
	rt |= kdrv_ime_int_set_ds_info(&(g_kdrv_ime_info[id]), &ime_ds_info);

	//progressive to interlaced parameter, 520 removed
	//ime_iq_info.pP2IInfo = (IME_FUNC_EN *)&g_kdrv_ime_info[id].sub_modules.p2i.enable;
	g_kdrv_ime_info[id].sub_modules.p2i.enable = DISABLE;
	if (g_kdrv_ime_info[id].sub_modules.p2i.enable == ENABLE) {
		DBG_WRN("KDRV IME: do not support P2I function\r\n");
	}

	//for ADAS, not ready, wait pin check
	//edge statistic parameters
	ime_stl_info.stl_img_size.size_h = g_kdrv_ime_info[id].out_path_info[KDRV_IME_PATH_ID_4].crp_window.w;
	ime_stl_info.stl_img_size.size_v = g_kdrv_ime_info[id].out_path_info[KDRV_IME_PATH_ID_4].crp_window.h;
	ime_iq_info.p_sta_info = &ime_stl_info;
	rt |= kdrv_ime_int_set_stl_info(&(g_kdrv_ime_info[id].sub_modules.stl), &ime_stl_info);

	//privacy mask parameters
	ime_iq_info.p_pm_info = &ime_pm_info;
	rt |= kdrv_ime_int_set_pm_par(&(g_kdrv_ime_info[id]), &ime_pm_info);

	//3DNR parameters
	ime_tmnr_info.m_img_size.size_h = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width;
	ime_tmnr_info.m_img_size.size_v = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height;
	ime_tmnr_info.tmnr_en = (IME_FUNC_EN)g_ime_iq_func_en[id].ime_ctrl_bit.bit.tmnr;
	ime_iq_info.p_tmnr_info = &ime_tmnr_info;
	rt |= kdrv_ime_int_set_tmnr_par(&(g_kdrv_ime_info[id]), &ime_tmnr_info);


	ime_tmnr_refout_info.m_img_size.size_h = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].width;
	ime_tmnr_refout_info.m_img_size.size_v = g_kdrv_ime_info[id].in_img_info.ch[KDRV_IPP_YUV_Y].height;
	ime_iq_info.p_tmnr_refout_info = &ime_tmnr_refout_info;
	rt |= kdrv_ime_int_set_tmnr_refout_par(&(g_kdrv_ime_info[id]), &ime_tmnr_refout_info);

	ime_yuv_cvt_info.yuv_cvt_enable = (IME_FUNC_EN)g_ime_iq_func_en[id].ime_ctrl_bit.bit.yuvcvt;
	ime_iq_info.p_yuv_cvt_info = &ime_yuv_cvt_info;
	rt |= kdrv_ime_int_set_yuv_cvt(&(g_kdrv_ime_info[id]), &ime_yuv_cvt_info);

	rt |= kdrv_ime_int_set_break_point(&(g_kdrv_ime_info[id]), &(ime.break_point));

	rt |= kdrv_ime_int_set_single_output(&(g_kdrv_ime_info[id]), &(ime.single_out));

	rt |= kdrv_ime_int_set_low_delay(&(g_kdrv_ime_info[id]), &(ime.low_delay));

	rt |= kdrv_ime_int_set_interrupt_enable(&(g_kdrv_ime_info[id]), &(ime.interrupt_enable));

	ime.p_ime_iq_info = &ime_iq_info;

	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_IN_IMG] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_IN_ADDR] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_OUT_IMG] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_OUT_ADDR] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_OUT_SPRT] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_OUT_SPRT_ADDR] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_EXTEND] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_ENCODE] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_LCA_REF_IMG] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_LCA_SUB] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_STL_OUT_IMG] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_SCL_METHOD_SEL] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_REF_IN_IMG] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_REF_OUT_IMG] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_MS] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_MS_ROI] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_MV] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_STA]
	   ) {

		rt |= ime_chg_direct_in_out_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_IN_IMG]           = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_IN_ADDR]          = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_OUT_IMG]          = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_OUT_ADDR]         = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_OUT_SPRT]         = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_OUT_SPRT_ADDR]    = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_EXTEND]           = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_ENCODE]           = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_LCA_REF_IMG]      = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_LCA_SUB]          = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_STL_OUT_IMG]      = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_SCL_METHOD_SEL]   = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_REF_IN_IMG]  = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_REF_OUT_IMG] = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_MS]          = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_MS_ROI]      = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_MV]          = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_TMNR_STA]         = FALSE;

	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IQ_LCA] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_LCA_CA_CENT] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_LCA_FUNC_EN]
	   ) {
		rt |= ime_chg_direct_lca_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IQ_LCA] = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_LCA_CA_CENT] = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_LCA_FUNC_EN] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IQ_DBCS]) {
		rt |= ime_chg_direct_dbcs_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IQ_DBCS] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_OSD] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_OSD_CST]) {
		rt |= ime_chg_direct_osd_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_OSD] = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_OSD_CST] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_OSD_PLT]) {
		rt |= ime_chg_direct_osd_palette_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_OSD_PLT] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_STL] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_STL_ROI] |
		g_ime_int_update_param[KDRV_IME_PARAM_IPL_STL_HIST]
	   ) {
		rt |= ime_chg_direct_adas_stl_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_STL] = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_STL_ROI] = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_STL_HIST] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_PM] | g_ime_int_update_param[KDRV_IME_PARAM_IPL_PM_PXL_IMG]) {
		rt |= ime_chg_direct_pm_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_PM] = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_PM_PXL_IMG] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IQ_TMNR] | g_ime_int_update_param[KDRV_IME_PARAM_IQ_TMNR_DBG]) {
		rt |= ime_chg_direct_tmnr_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IQ_TMNR] = FALSE;
		//g_ime_int_update_param[KDRV_IME_PARAM_IQ_TMNR_DBG] = FALSE;
	}


	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_BREAK_POINT]) {
		rt |= ime_chg_direct_break_point_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_BREAK_POINT] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_SINGLE_OUTPUT]) {
		rt |= ime_chg_direct_single_output_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_SINGLE_OUTPUT] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_LOW_DELAY]) {
		rt |= ime_chg_direct_low_delay_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_LOW_DELAY] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IPL_INTER]) {
		rt |= ime_chg_direct_interrupt_enable_param(&ime);

		//g_ime_int_update_param[KDRV_IME_PARAM_IPL_INTER] = FALSE;
	}

	if (g_ime_int_update_param[KDRV_IME_PARAM_IQ_YCCCVT]) {
		IME_CLAMP_INFO set_clamp;

		rt |= ime_chg_direct_yuv_cvt_param(&ime);

		set_clamp = ime.out_path1.out_path_clamp;
		ime_chg_output_path_range_clamp_param(IME_PATH1_SEL, &set_clamp);

		set_clamp = ime.out_path2.out_path_clamp;
		ime_chg_output_path_range_clamp_param(IME_PATH2_SEL, &set_clamp);

		set_clamp = ime.out_path3.out_path_clamp;
		ime_chg_output_path_range_clamp_param(IME_PATH3_SEL, &set_clamp);

		set_clamp = ime.out_path4.out_path_clamp;
		ime_chg_output_path_range_clamp_param(IME_PATH4_SEL, &set_clamp);

		//g_ime_int_update_param[KDRV_IME_PARAM_IQ_YCCCVT] = FALSE;
	}

	return rt;
}



KDRV_IME_SET_FP kdrv_ime_set_fp[KDRV_IME_PARAM_MAX] = {
	kdrv_ime_set_clk,
	kdrv_ime_set_in_img_info,
	kdrv_ime_set_in_addr,
	kdrv_ime_set_out_img_info,
	kdrv_ime_set_out_addr,
	kdrv_ime_set_out_sprt_info,
	kdrv_ime_set_out_sprt_addr,
	kdrv_ime_set_extend_info,
	kdrv_ime_set_encode_info,
	kdrv_ime_set_isrcb,
	kdrv_ime_set_cmf,
	kdrv_ime_set_lca,
	kdrv_ime_set_lca_ca_cent,
	kdrv_ime_set_lca_img_info,
	kdrv_ime_set_lca_sub_out,
	kdrv_ime_set_dbcs,
	kdrv_ime_set_ssr,
	kdrv_ime_set_rgb2ycc,
	kdrv_ime_set_ycc2rgb,
	kdrv_ime_set_fgn,
	kdrv_ime_set_ds,
	kdrv_ime_set_ds_cst,
	kdrv_ime_set_ds_plt,
	kdrv_ime_set_p2i,
	kdrv_ime_set_stl,
	kdrv_ime_set_stl_roi,
	kdrv_ime_set_stl_hist,
	kdrv_ime_set_stl_out_img,
	kdrv_ime_set_pm,
	kdrv_ime_set_pm_pxl_img_info,
	kdrv_ime_set_tmnr,
	kdrv_ime_set_tmnr_ref_in_img,
	kdrv_ime_set_tmnr_ref_out_img,
	kdrv_ime_set_tmnr_motion_status_info,
	kdrv_ime_set_tmnr_motion_status_roi_info,
	kdrv_ime_set_tmnr_motion_vector_info,
	kdrv_ime_set_tmnr_motion_statistic_info,
	kdrv_ime_set_scl_method_sel,
	NULL,
	kdrv_ime_set_iq_all,
	kdrv_ime_set_ipl_func_enable,
	kdrv_ime_set_lca_en_ctrl,
	kdrv_ime_set_break_point,
	kdrv_ime_set_single_output,
	kdrv_ime_set_low_delay,
	kdrv_ime_set_inte_en,
	kdrv_ime_set_load,
	kdrv_ime_set_ycc_cvt,
	NULL,
	kdrv_ime_set_tmnr_dbg,
	NULL,
	kdrv_ime_set_reset,
	kdrv_ime_set_builtin_flow_disable,
	kdrv_ime_set_ref_in_addr_use_va2pa_en,
} ;



INT32 kdrv_ime_trigger(UINT32 id, KDRV_IME_TRIGGER_PARAM *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	UINT32 eng_id, chl_id;

	eng_id = KDRV_DEV_ID_ENGINE(id);
	chl_id = KDRV_DEV_ID_CHANNEL(id);

	switch (eng_id) {
	case KDRV_VIDEOPROCS_IME_ENGINE0:
		break;

	default:
		DBG_ERR("KDRV IME: engine id error\r\n");
		return -1;
		break;
	}

	if (chl_id >= kdrv_ime_channel_num) {
		DBG_ERR("KDRV IME: channel id error\r\n");
		return -1;
	}

	if (eng_id == KDRV_VIDEOPROCS_IME_ENGINE0) {
		if (kdrv_ime_set_trig(chl_id, NULL) != E_OK) {
			DBG_ERR("KDRV IME: trigger error\r\n");
			return -1;
		}
	} else {
		// do nothing...
	}

	return 0;
}


INT32 kdrv_ime_set(UINT32 id, KDRV_IME_PARAM_ID param_id, void *p_param)
{
	//ER rt = E_OK;
	KDRV_IME_HANDLE *p_handle;
	UINT32 ign_chk;

	UINT32 eng_id, chl_id;

	ign_chk = (KDRV_IME_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_IME_IGN_CHK);

	switch (param_id) {
	case KDRV_IME_PARAM_IPL_LOAD:
	case KDRV_IME_PARAM_IPL_RESET:
	case KDRV_IME_PARAM_IPL_BUILTIN_DISABLE:
		break;

	default:
		if (p_param == NULL) {
			DBG_ERR("KDRV IME: set parameter NULL...\r\n");
			return -1;
		}
		break;
	}

	eng_id = KDRV_DEV_ID_ENGINE(id);
	chl_id = KDRV_DEV_ID_CHANNEL(id);

	switch (eng_id) {
	case KDRV_VIDEOPROCS_IME_ENGINE0:
		break;

	default:
		DBG_ERR("KDRV IME: engine id error\r\n");
		return -1;
		break;
	}

	if (param_id == KDRV_IME_PARAM_IPL_BUILTIN_DISABLE) {
        if (kdrv_ime_set_fp[param_id] != NULL) {
    		if (kdrv_ime_set_fp[param_id](chl_id, p_param) != E_OK) {
    			return -1;
    		}
    	}
	} else {
    	if (chl_id >= kdrv_ime_channel_num) {
    		DBG_ERR("KDRV IME: channel id error\r\n");
    		return -1;
    	}

    	//p_handle = (KDRV_IME_HANDLE *)chl_id;
    	kdrv_ime_channel_alloc(chl_id);
    	p_handle = &g_kdrv_ime_handle_tab[chl_id];
    	if (kdrv_ime_check_handle(p_handle) == NULL) {
    		DBG_WRN("KDRV IME: illegal handle 0x%.8x\r\n", (unsigned int)chl_id);
    		return -1;
    	}

    	if ((p_handle->sts & KDRV_IME_HANDLE_LOCK) != KDRV_IME_HANDLE_LOCK) {
    		DBG_WRN("KDRV IME: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
    		return -1;
    	}


    	if (!ign_chk) {
    		kdrv_ime_lock(p_handle, TRUE);
    	}
    	if (kdrv_ime_set_fp[param_id] != NULL) {
    		if (kdrv_ime_set_fp[param_id](p_handle->entry_id, p_param) != E_OK) {
    			return -1;
    		}
    	}
    	if (!ign_chk) {
    		kdrv_ime_lock(p_handle, FALSE);
    	}
	}

	return 0;
}

#if 0
#endif

static ER kdrv_ime_get_in_addr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get input image buffer address parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_DMA_ADDR_INFO *)data = g_kdrv_ime_info[id].in_pixel_addr;

	return E_OK;
}

static ER kdrv_ime_get_in_img_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: set input image parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_IN_INFO *)data = g_kdrv_ime_info[id].in_img_info;

	return E_OK;
}

static ER kdrv_ime_get_out_addr(UINT32 id, void *data)
{
	KDRV_IME_OUT_PATH_ADDR_INFO *path_addr_info;

	if (data == NULL) {
		DBG_ERR("KDRV IME: get output image buffer address parameter NULL...\r\n");
		return E_PAR;
	}

	path_addr_info = (KDRV_IME_OUT_PATH_ADDR_INFO *)data;
	path_addr_info->addr_info = g_kdrv_ime_info[id].out_path_info[path_addr_info->path_id].pixel_addr;

	return E_OK;
}

static ER kdrv_ime_get_out_img_info(UINT32 id, void *data)
{
	KDRV_IME_OUT_PATH_IMG_INFO *img_info;

	if (data == NULL) {
		DBG_ERR("KDRV IME: get output image size parameter NULL...\r\n");
		return E_PAR;
	}

	img_info = (KDRV_IME_OUT_PATH_IMG_INFO *)data;

	img_info->path_en = g_kdrv_ime_info[id].out_path_info[img_info->path_id].path_en;
	img_info->type = g_kdrv_ime_info[id].out_path_info[img_info->path_id].type;
	img_info->ch[KDRV_IPP_YUV_Y] = g_kdrv_ime_info[id].out_path_info[img_info->path_id].ch[KDRV_IPP_YUV_Y];
	img_info->ch[KDRV_IPP_YUV_U] = g_kdrv_ime_info[id].out_path_info[img_info->path_id].ch[KDRV_IPP_YUV_U];
	img_info->ch[KDRV_IPP_YUV_V] = g_kdrv_ime_info[id].out_path_info[img_info->path_id].ch[KDRV_IPP_YUV_V];
	img_info->crp_window = g_kdrv_ime_info[id].out_path_info[img_info->path_id].crp_window;

	return E_OK;
}

static ER kdrv_ime_get_out_sprt_addr(UINT32 id, void *data)
{
	KDRV_IME_OUT_SPRT_ADDR_INFO *path_sprt_addr;

	if (data == NULL) {
		DBG_ERR("KDRV IME: get output stitching image buffer address parameter NULL...\r\n");
		return E_PAR;
	}

	path_sprt_addr = (KDRV_IME_OUT_SPRT_ADDR_INFO *)data;
	path_sprt_addr->sprt_addr = g_kdrv_ime_info[id].out_path_info[path_sprt_addr->path_id].out_sprt_addr;

	return E_OK;
}

static ER kdrv_ime_get_out_sprt_info(UINT32 id, void *data)
{
	KDRV_IME_OUT_SPRT_INFO *img_info;

	img_info = (KDRV_IME_OUT_SPRT_INFO *)data;
	img_info->enable = g_kdrv_ime_info[id].out_path_info[img_info->path_id].out_sprt_info.enable;
	*(KDRV_IME_OUT_SPRT_INFO *)data = g_kdrv_ime_info[id].out_path_info[img_info->path_id].out_sprt_info;

	return E_OK;
}

static ER kdrv_ime_get_extend_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_EXTEND_INFO *)data = g_kdrv_ime_info[id].extend_info;

	return E_OK;
}

static ER kdrv_ime_get_encode_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_ENCODE_INFO *)data = g_kdrv_ime_info[id].out_path_info[KDRV_IME_PATH_ID_1].out_encode_info;

	return E_OK;
}

static ER kdrv_ime_get_cmf(UINT32 id, void *data)
{
#if 0
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_CMF_PARAM *)data = g_kdrv_ime_info[id].sub_modules.chroma_filter;
#endif

	DBG_ERR("KDRV IME: don't support get chroma median filter fucntion information...\r\n");

	return E_OK;
}

static ER kdrv_ime_get_lca(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get lca parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_LCA_PARAM *)data = g_kdrv_ime_info[id].sub_modules.lca;

	return E_OK;
}

static ER kdrv_ime_get_lca_ca_cent(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get lca-ca parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_LCA_CA_CENT_INFO *)data = g_kdrv_ime_info[id].sub_modules.lca_ca_cent;

	return E_OK;
}

static ER kdrv_ime_get_lca_img_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get lca image parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_LCA_IMG_INFO *)data = g_kdrv_ime_info[id].sub_modules.lca_img;

	return E_OK;
}

static ER kdrv_ime_get_lca_sub(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get lca subout image parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_LCA_SUBOUT_INFO *)data = g_kdrv_ime_info[id].sub_modules.lca_subout;

	return E_OK;
}

static ER kdrv_ime_get_dbcs(UINT32 id, void *data)
{
	//UINT32 i;
	KDRV_IME_DBCS_PARAM *pdbcs_info = NULL;
	//UINT32* ptmp_wt_y = pdbcs_info->wt_y;
	//UINT32* ptmp_wt_c = pdbcs_info->wt_c;

	if (data == NULL) {
		DBG_ERR("KDRV IME: get dbcs parameter NULL...\r\n");
		return E_PAR;
	}

	pdbcs_info = (KDRV_IME_DBCS_PARAM *)data;

	*pdbcs_info = g_kdrv_ime_info[id].sub_modules.dbcs;

#if 0
	//copy weighting table
	if (ptmp_wt_y != NULL) {
		for (i = 0; i < KDRV_IME_DBCS_PARAM_WT_LUT_TAB ; i++) {
			ptmp_wt_y[i] = g_kdrv_ime_info[id].sub_modules.dbcs.wt_y[i];
		}
		pdbcs_info->wt_y = ptmp_wt_y;
	}
	if (ptmp_wt_c != NULL) {
		for (i = 0; i < KDRV_IME_DBCS_PARAM_WT_LUT_TAB ; i++) {
			ptmp_wt_c[i] = g_kdrv_ime_info[id].sub_modules.dbcs.wt_c[i];
		}
		pdbcs_info->wt_c = ptmp_wt_c;
	}
#endif

	return E_OK;
}

static ER kdrv_ime_get_ssr(UINT32 id, void *data)
{
#if 0
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_SSR_PARAM *)data = g_kdrv_ime_info[id].sub_modules.ssr;
#endif

	DBG_ERR("KDRV IME: don't support SSR function...\r\n");

	return E_OK;
}

#if 0
static ER kdrv_ime_get_cst(UINT32 id, void *data)
{
	*(KDRV_IME_CST_PARAM *)data = g_kdrv_ime_info[id].sub_modules.color_space_trans;

	return E_OK;
}
#endif

static ER kdrv_ime_get_rgb2ycc(UINT32 id, void *data)
{
#if 0
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_RGB2YCC_INFO *)data = g_kdrv_ime_info[id].sub_modules.rgb2ycc;
#endif

	DBG_ERR("KDRV IME: don't support RGB-to-YCC function...\r\n");

	return E_OK;
}

static ER kdrv_ime_get_ycc2rgb(UINT32 id, void *data)
{
#if 0
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_YCC2RGB_INFO *)data = g_kdrv_ime_info[id].sub_modules.ycc2rgb;
#endif

	DBG_ERR("KDRV IME: don't support YCC-to-RGB function...\r\n");

	return E_OK;
}

static ER kdrv_ime_get_fgn(UINT32 id, void *data)
{
#if 0
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_FGN_PARAM *)data = g_kdrv_ime_info[id].sub_modules.film_grain;
#endif

	DBG_ERR("KDRV IME: don't support file-grain-noise function...\r\n");

	return E_OK;
}

static ER kdrv_ime_get_ds(UINT32 id, void *data)
{
	KDRV_IME_OSD_INFO *ds_info;

	if (data == NULL) {
		DBG_ERR("KDRV IME: get osd parameter NULL...\r\n");
		return E_PAR;
	}

	ds_info = (KDRV_IME_OSD_INFO *)data;
	*ds_info = g_kdrv_ime_info[id].sub_modules.ds[ds_info->ds_set_idx];

	return E_OK;
}

static ER kdrv_ime_get_ds_cst_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get osd-cst parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_OSD_CST_INFO *)data = g_kdrv_ime_info[id].sub_modules.ds_cst;

	return E_OK;
}

static ER kdrv_ime_get_ds_plt_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get osd-palette parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_OSD_PLT_INFO *)data = g_kdrv_ime_info[id].sub_modules.ds_plt;

	return E_OK;
}


static ER kdrv_ime_get_p2i(UINT32 id, void *data)
{
#if 0
	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_P2I_INFO *)data = g_kdrv_ime_info[id].sub_modules.p2i;
#endif

	DBG_ERR("KDRV IME: don't support P2I function...\r\n");

	return E_OK;
}

static ER kdrv_ime_get_stl(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get statistical parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_STL_INFO *)data = g_kdrv_ime_info[id].sub_modules.stl.stl_info;

	return E_OK;
}

static ER kdrv_ime_get_stl_roi(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get statistical ROI parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_STL_ROI_INFO *)data = g_kdrv_ime_info[id].sub_modules.stl.stl_roi;

	return E_OK;
}

static ER kdrv_ime_get_stl_hist(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get statistical histogram parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_STL_HIST_INFO *)data = g_kdrv_ime_info[id].sub_modules.stl.stl_hist;

	return E_OK;
}

static ER kdrv_ime_get_stl_out_img(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get statistical image parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_STL_OUT_IMG_INFO *)data = g_kdrv_ime_info[id].sub_modules.stl.stl_out_img;

	return E_OK;
}

static ER kdrv_ime_get_pm(UINT32 id, void *data)
{
	KDRV_IME_PM_INFO *pm_info;

	if (data == NULL) {
		DBG_ERR("KDRV IME: get privay mask parameter NULL...\r\n");
		return E_PAR;
	}

	pm_info = (KDRV_IME_PM_INFO *)data;
	*pm_info = g_kdrv_ime_info[id].sub_modules.pm.pm_info[pm_info->set_idx];

	return E_OK;
}

static ER kdrv_ime_get_pm_pxl_img_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get privay mask pixlation parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_PM_PXL_IMG_INFO *)data = g_kdrv_ime_info[id].sub_modules.pm.pm_pxl_img_info;

	return E_OK;
}

static ER kdrv_ime_get_tmnr(UINT32 id, void *data)
{
	//UINT32 i;
	KDRV_IME_TMNR_PARAM *p_get_info = NULL;

	if (data == NULL) {
		DBG_ERR("KDRV IME: get tmnr parameter NULL...\r\n");
		return E_PAR;
	}

	p_get_info = (KDRV_IME_TMNR_PARAM *)data;

	*p_get_info = g_kdrv_ime_info[id].sub_modules.tmnr.nr_iq;


	return E_OK;
}

static ER kdrv_ime_get_tmnr_ref_in_img(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get tmnr reference input image parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_TMNR_REF_IN_IMG_INFO *)data = g_kdrv_ime_info[id].sub_modules.tmnr.nr_ref_in_img;

	return E_OK;
}

static ER kdrv_ime_get_tmnr_ref_out_img(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get tmnr reference output image parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_TMNR_REF_OUT_IMG_INFO *)data = g_kdrv_ime_info[id].sub_modules.tmnr_refout.nr_ref_out_img;

	return E_OK;
}

static ER kdrv_ime_get_tmnr_motion_status_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get tmnr motion status input and output info. parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_TMNR_MOTION_STATUS_INFO *)data = g_kdrv_ime_info[id].sub_modules.tmnr.nr_ms;

	return E_OK;
}

static ER kdrv_ime_get_tmnr_motion_status_roi_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get tmnr motion status output for ROI parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_TMNR_MOTION_STATUS_ROI_INFO *)data = g_kdrv_ime_info[id].sub_modules.tmnr.nr_ms_roi;

	return E_OK;
}

static ER kdrv_ime_get_tmnr_motion_vector_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get tmnr motion vector input and output parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_TMNR_MOTION_VECTOR_INFO *)data = g_kdrv_ime_info[id].sub_modules.tmnr.nr_mv;

	return E_OK;
}


static ER kdrv_ime_get_tmnr_motion_statistic_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get tmnr motion statistic output parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_TMNR_STATISTIC_DATA_INFO *)data = g_kdrv_ime_info[id].sub_modules.tmnr.nr_sta;

	return E_OK;
}

static ER kdrv_ime_get_tmnr_extra_buffer_info(UINT32 id, void *data)
{
	UINT32 in_size_h, in_size_v;
	UINT32 cal_lofs, cal_size;
	UINT32 ne_sample_num_x, ne_max_num;


	if (data == NULL) {
		DBG_ERR("KDRV IME: get tmnr extra buffer info. NULL...\r\n");
		return E_PAR;
	}

	in_size_h = ((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->in_size_h;
	in_size_v = ((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->in_size_v;

	ne_max_num = ((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->in_sta_max_num;
	if (ne_max_num < 8192) {
		ne_max_num = 8192;
	}

	//ne_sample_num_x = ((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->in_ne_sample_num_x;
	//ne_sample_num_y = ((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->in_ne_sample_num_y;


	// cal. mv
	//cal_lofs = (((in_size_h >> 2) + 3) >> 2) << 2;
	cal_lofs = ((in_size_h + 15) >> 4) << 2;
	cal_size = cal_lofs * ((in_size_v >> 2) - 1);
	((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_mv_lofs = cal_lofs;
	((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_mv_size = cal_size;

	// cal ms
	//cal_lofs = (((in_size_h >> 7) + 3) >> 2) << 2;
	cal_lofs = ((in_size_h + 511) >> 9) << 2;
	cal_size = cal_lofs * ((in_size_v + 15) >> 4);
	((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_ms_lofs = cal_lofs;
	((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_ms_size = cal_size;

	// cal ms-roi
	//cal_lofs = (((in_size_h >> 7) + 3) >> 2) << 2;
	cal_lofs = ((in_size_h + 511) >> 9) << 2;
	cal_size = cal_lofs * ((in_size_v + 15) >> 4);
	((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_ms_roi_lofs = cal_lofs;
	((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_ms_roi_size = cal_size;

	// cal sta
	if (kdrv_ime_cal_sta_para(in_size_h, in_size_v, ((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->in_sta_max_num, &(((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_sta_param)) != E_OK) {
		return E_PAR;
	}
	ne_sample_num_x = ((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_sta_param.sample_num_x;
	cal_lofs = (((ne_sample_num_x * 5) + 3) >> 2) << 2;
	cal_size = ne_max_num * 5;//cal_lofs * ne_sample_num_y;
	((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_sta_lofs = cal_lofs;
	((KDRV_IME_TMNR_BUF_SIZE_INFO *)data)->get_sta_size = cal_size;

	return E_OK;
}

static ER kdrv_ime_get_scl_method_sel(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get scaling method parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_SCL_METHOD_SEL_INFO *)data = g_kdrv_ime_info[id].out_scl_method_sel;

	return E_OK;
}

static ER kdrv_ime_get_encode_ktab_rst(UINT32 id, void *data)
{
#if 0
	KDRV_IME_ENCODE_INFO *p_enc;
	IME_CODEC_KT_INFO ktab;

	if (data == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return E_PAR;
	}

	p_enc = (KDRV_IME_ENCODE_INFO *)data;
	*p_enc = g_kdrv_ime_info[id].out_path_info[KDRV_IME_PATH_ID_1].out_encode_info;

	ime_get_encode_ktable(&ktab);
	p_enc->k_tbl0 = ktab.uiKt0;
	p_enc->k_tbl1 = ktab.uiKt1;
	p_enc->k_tbl2 = ktab.uiKt2;
#endif

	DBG_ERR("KDRV IME: encoder don't support k-table function...\r\n");

	return E_OK;
}

static ER kdrv_ime_get_break_point(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get break-point parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_BREAK_POINT_INFO *)data = g_kdrv_ime_info[id].break_point_ctrl;

	return E_OK;
}

static ER kdrv_ime_get_single_output(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get single output parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_SINGLE_OUT_INFO *)data = g_kdrv_ime_info[id].single_out_ctrl;

	return E_OK;
}


static ER kdrv_ime_get_low_delay(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get low dealy parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_LOW_DELAY_INFO *)data = g_kdrv_ime_info[id].low_delay_ctrl;

	return E_OK;
}

static ER kdrv_ime_get_ycc_cvt(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IME: get ycc-cvt parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IME_YCC_CVT_PARAM *)data = g_kdrv_ime_info[id].sub_modules.yuv_cvt;

	return E_OK;
}

static ER kdrv_ime_get_single_output_hw(UINT32 id, void *data)
{
	//UINT32 i= 0;
	KDRV_IME_SINGLE_OUT_INFO info = {0};
	IME_SINGLE_OUT_INFO get_sout_hw_status = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IME: get single output parameter NULL...\r\n");
		return E_PAR;
	}

	ime_get_single_output(&get_sout_hw_status);

	info.sout_enable = (BOOL)get_sout_hw_status.sout_enable;
	info.sout_chl = get_sout_hw_status.sout_ch;

	*(KDRV_IME_SINGLE_OUT_INFO *)data = info;

	return E_OK;
}


KDRV_IME_GET_FP kdrv_ime_get_fp[KDRV_IME_PARAM_MAX] = {
	NULL,
	kdrv_ime_get_in_img_info,
	kdrv_ime_get_in_addr,
	kdrv_ime_get_out_img_info,
	kdrv_ime_get_out_addr,
	kdrv_ime_get_out_sprt_info,
	kdrv_ime_get_out_sprt_addr,
	kdrv_ime_get_extend_info,
	kdrv_ime_get_encode_info,
	NULL,
	kdrv_ime_get_cmf,
	kdrv_ime_get_lca,
	kdrv_ime_get_lca_ca_cent,
	kdrv_ime_get_lca_img_info,
	kdrv_ime_get_lca_sub,
	kdrv_ime_get_dbcs,
	kdrv_ime_get_ssr,
	kdrv_ime_get_rgb2ycc,
	kdrv_ime_get_ycc2rgb,
	kdrv_ime_get_fgn,
	kdrv_ime_get_ds,
	kdrv_ime_get_ds_cst_info,
	kdrv_ime_get_ds_plt_info,
	kdrv_ime_get_p2i,
	kdrv_ime_get_stl,
	kdrv_ime_get_stl_roi,
	kdrv_ime_get_stl_hist,
	kdrv_ime_get_stl_out_img,
	kdrv_ime_get_pm,
	kdrv_ime_get_pm_pxl_img_info,
	kdrv_ime_get_tmnr,
	kdrv_ime_get_tmnr_ref_in_img,
	kdrv_ime_get_tmnr_ref_out_img,
	kdrv_ime_get_tmnr_motion_status_info,
	kdrv_ime_get_tmnr_motion_status_roi_info,
	kdrv_ime_get_tmnr_motion_vector_info,
	kdrv_ime_get_tmnr_motion_statistic_info,
	kdrv_ime_get_scl_method_sel,
	kdrv_ime_get_encode_ktab_rst,
	NULL,
	NULL,
	NULL,
	kdrv_ime_get_break_point,
	kdrv_ime_get_single_output,
	kdrv_ime_get_low_delay,
	NULL,
	NULL,
	kdrv_ime_get_ycc_cvt,
	kdrv_ime_get_tmnr_extra_buffer_info,
	NULL,
	kdrv_ime_get_single_output_hw,
	NULL,
	NULL,
};


INT32 kdrv_ime_get(UINT32 id, KDRV_IME_PARAM_ID param_id, VOID *p_param)
{
	//ER rt = E_OK;
	KDRV_IME_HANDLE *p_handle;
	UINT32 ign_chk;
	UINT32 eng_id;
	UINT32 chl_id;

	if (p_param == NULL) {
		DBG_ERR("KDRV IME: parameter NULL...\r\n");
		return -1;
	}

	eng_id = KDRV_DEV_ID_ENGINE(id);
	chl_id = KDRV_DEV_ID_CHANNEL(id);

	switch (eng_id) {
	case KDRV_VIDEOPROCS_IME_ENGINE0:
		break;

	default:
		DBG_ERR("KDRV IME: engine id error\r\n");
		return -1;
		break;
	}

	if (chl_id >= kdrv_ime_channel_num) {
		DBG_ERR("KDRV IME: channel id error\r\n");
		return -1;
	}

	//p_handle = (KDRV_IME_HANDLE *)id;
	p_handle = &g_kdrv_ime_handle_tab[chl_id];
	if (kdrv_ime_check_handle(p_handle) == NULL) {
		DBG_WRN("KDRV IME: illegal handle 0x%.8x\r\n", (unsigned int)id);
		return -1;
	}

	if ((p_handle->sts & KDRV_IME_HANDLE_LOCK) != KDRV_IME_HANDLE_LOCK) {
		DBG_WRN("KDRV IME: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
		return -1;
	}

	ign_chk = (KDRV_IME_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_IME_IGN_CHK);

	if (!ign_chk) {
		kdrv_ime_lock(p_handle, TRUE);
	}

	if (kdrv_ime_get_fp[param_id] != NULL) {
		if (kdrv_ime_get_fp[param_id](p_handle->entry_id, p_param) != E_OK) {
			return -1;
		}
	}

	if (!ign_chk) {
		kdrv_ime_lock(p_handle, FALSE);
	}

	return 0;
}

UINT32 kdrv_ime_lock_chls = 0;
UINT32 kdrv_ime_buf_query(UINT32 channel_num)
{
	UINT32 buffer_size = 0, i = 0;

	if (channel_num == 0x0) {
		DBG_ERR("KDRV IME: user desired channel number is zero\r\n");
	}

	kdrv_ime_channel_num = channel_num;

	if (kdrv_ime_channel_num > KDRV_IPP_MAX_CHANNEL_NUM) {
		kdrv_ime_channel_num = KDRV_IPP_MAX_CHANNEL_NUM;

		DBG_ERR("KDRV IME: user desired channel number is overflow, max = 28\r\n");
	}

	kdrv_ime_lock_chls = 0;
	for (i = 0; i < kdrv_ime_channel_num; i++) {
		kdrv_ime_lock_chls += (1 << i);
	}

	buffer_size  += IME_ALIGN_CEIL_64(kdrv_ime_channel_num * sizeof(KDRV_IME_HANDLE));
	buffer_size  += IME_ALIGN_CEIL_64(kdrv_ime_channel_num * sizeof(KDRV_IME_OPENCFG));
	buffer_size  += IME_ALIGN_CEIL_64(kdrv_ime_channel_num * sizeof(KDRV_IME_PRAM));
	buffer_size  += IME_ALIGN_CEIL_64(kdrv_ime_channel_num * sizeof(KDRV_IME_FUNC_CTRL));
	buffer_size  += IME_ALIGN_CEIL_64(kdrv_ime_channel_num * sizeof(KDRV_IME_FUNC_INT_CTRL));
	buffer_size  += IME_ALIGN_CEIL_64(kdrv_ime_channel_num * sizeof(KDRV_IME_FUNC_CTRL));

	return buffer_size;
}

UINT32 kdrv_ime_buf_init(UINT32 input_addr, UINT32 channel_num)
{
	UINT32 buffer_size = 0;

	UINT32 get_chl_num = 0;

	kdrv_ime_install_id();

	get_chl_num = channel_num;
	if (get_chl_num != kdrv_ime_channel_num) {
		DBG_ERR("KDRV IME: initial channel number %d != user queried channel number %d\r\n", (int)channel_num, (int)kdrv_ime_channel_num);

		get_chl_num = kdrv_ime_channel_num;
	}

	kdrv_ime_flow_init();

	g_kdrv_ime_handle_tab = (KDRV_IME_HANDLE *)(input_addr + buffer_size);
	buffer_size += IME_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IME_HANDLE));

	g_kdrv_ime_clk_info = (KDRV_IME_OPENCFG *)(input_addr + buffer_size);
	buffer_size += IME_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IME_OPENCFG));

	g_kdrv_ime_info = (KDRV_IME_PRAM *)(input_addr + buffer_size);
	buffer_size += IME_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IME_PRAM));

	g_iq_func_en = (KDRV_IME_FUNC_CTRL *)(input_addr + buffer_size);
	buffer_size += IME_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IME_FUNC_CTRL));

	g_ipl_func_en = (KDRV_IME_FUNC_INT_CTRL *)(input_addr + buffer_size);
	buffer_size += IME_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IME_FUNC_INT_CTRL));

	g_ime_iq_func_en = (KDRV_IME_FUNC_CTRL *)(input_addr + buffer_size);
	buffer_size += IME_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IME_FUNC_CTRL));

	return buffer_size;
}

VOID kdrv_ime_buf_uninit(VOID)
{
	kdrv_ime_uninstall_id();

	g_kdrv_ime_handle_tab = NULL;
	g_kdrv_ime_clk_info = NULL;
	g_kdrv_ime_info = NULL;

	g_iq_func_en = NULL;
	g_ipl_func_en = NULL;
	g_ime_iq_func_en = NULL;

	kdrv_ime_channel_num = 0;
	kdrv_ime_lock_chls = 0;

	if (g_kdrv_ime_open_cnt[KDRV_IME_ID_0] == 0) {
		g_kdrv_ime_init_flg = 0;
		g_kdrv_ime_set_clk_flg[KDRV_IME_ID_0] = FALSE;
	}
}

/*
#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)

#if defined (__LINUX) || defined (__KERNEL__)
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
//#include <comm/nvtmem.h>



#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"
#include <kwrap/file.h>

#include "mach/rcw_macro.h"
#include "mach/nvt-io.h"
#include "kwrap/type.h"
#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>

struct nvt_fmem_mem_info_t kdrv_ime_get_chl_buf = {0};
void *hdl_ime_chl_buf = NULL;

#elif defined (__FREERTOS)

#include <malloc.h>
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"

extern void *pvPortMalloc(size_t xWantedSize);
extern void vPortFree(void *pv);

UINT32 *kdrv_ime_get_chl_buf = NULL;

#endif

#endif
*/

VOID kdrv_ime_init(VOID)
{
#if defined (__FREERTOS)
	ime_platform_create_resource();
	ime_platform_set_clk_rate(280);
#else
    ime_platform_set_clk_rate(280);
#endif

	//kdrv_ime_install_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
	{
		UINT32 kdrv_ime_buf_size = 0, get_addr = 0;

		kdrv_ime_buf_size = kdrv_ime_buf_query(KDRV_IPP_CBUF_MAX_NUM);

		//DBG_ERR("kdrv_ime_buf_size: %08x\r\n", kdrv_ime_buf_size);

#if defined (__LINUX) || defined (__KERNEL__)
		{
			INT32  ret = 0;

			ret = nvt_fmem_mem_info_init(&kdrv_ime_get_chl_buf, NVT_FMEM_ALLOC_CACHE, kdrv_ime_buf_size, NULL);
			if (ret >= 0) {
				//hdl_ime_chl_buf = nvtmem_alloc_buffer(&kdrv_ime_get_chl_buf);

				hdl_ime_chl_buf = fmem_alloc_from_cma(&kdrv_ime_get_chl_buf, 0);
				get_addr = (UINT32)kdrv_ime_get_chl_buf.vaddr;
			}
		}
#elif defined (__FREERTOS)
		{
			if (kdrv_ime_get_chl_buf != NULL) {
				vPortFree((void *)kdrv_ime_get_chl_buf);
				kdrv_ime_get_chl_buf = NULL;
			}
			kdrv_ime_get_chl_buf = (UINT32 *)pvPortMalloc((size_t)IME_ALIGN_CEIL_8(kdrv_ime_buf_size));

			if (kdrv_ime_get_chl_buf != NULL) {
				get_addr = (UINT32)kdrv_ime_get_chl_buf;
			}
		}
#else
		get_addr = 0;
#endif

		//DBG_ERR("get_addr: %08x\r\n", get_addr);

		kdrv_ime_buf_init(get_addr, KDRV_IPP_CBUF_MAX_NUM);
	}
#endif

}

VOID kdrv_ime_uninit(VOID)
{

#if defined (__FREERTOS)
	ime_platform_release_resource();
#endif

	//kdrv_ime_uninstall_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)

#if defined (__LINUX) || defined (__KERNEL__)
	if (hdl_ime_chl_buf != NULL) {
		INT32  ret = 0;

		//ret = nvtmem_release_buffer(hdl_ime_chl_buf);
		ret = fmem_release_from_cma(hdl_ime_chl_buf, 0);
		hdl_ime_chl_buf = NULL;
	}
#elif  defined (__FREERTOS)
	if (kdrv_ime_get_chl_buf != NULL) {
		vPortFree((void *)kdrv_ime_get_chl_buf);
		kdrv_ime_get_chl_buf = NULL;
	}
#endif

	kdrv_ime_buf_uninit();

#endif

}


#if 0
#endif

BOOL is_ime_open[KDRV_IPP_MAX_CHANNEL_NUM] = {FALSE};
#if defined __UITRON || defined __ECOS
void kdrv_ime_dump_info(void (*dump)(char *fmt, ...))
#else
void kdrv_ime_dump_info(int (*dump)(const char *fmt, ...))
#endif
{

	UINT32 hdl;
	KDRV_IME_PATH_ID path_idx;
	UINT32 ds_hdlx;

	CHAR *kdrv_ime_in_src_tbl[] = {
		"DIRECT",   //input direct from engine
		"DRAM",     //input from dram
		""
	};

	CHAR *kdrv_ime_in_fmt_tbl[] = {
		"YUV444",       ///< 444 planar
		"YUV422",       ///< 422 planar
		"YUV420",       ///< 420 planar
		"Y_PACK_UV422", ///< 422 format y planar UV pack UVUVUVUVUV.....
		"Y_PACK_UV420", ///< 420 format y planar UV pack UVUVUVUVUV.....
		"Y_ONLY",       ///< Only Y channel
		"RGB",          ///< RGB444
		""
	};

	CHAR *kdrv_ime_out_fmt_tbl[] = {
		"YUV444",           ///< 444 planar
		"YUV422",           ///< 422 planar
		"YUV420",           ///< 420 planar
		"Y_PACK_UV444",     ///< 444 format y planar UV pack UVUVUVUVUV.....
		"Y_PACK_UV422",     ///< 422 format y planar UV pack UVUVUVUVUV.....
		"Y_PACK_UV420",     ///< 420 format y planar UV pack UVUVUVUVUV.....
		"Y_ONLY",           ///< Only Y channel
		"RGB",              ///< RGB444
		""
	};

	if (g_kdrv_ime_init_flg == 0) {
		dump("\r\n[IME] not open\r\n");
		return;
	}

	for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
		if ((g_kdrv_ime_init_flg & (1 << hdl)) == (UINT32)(1 << hdl)) {
			is_ime_open[hdl] = TRUE;
			kdrv_ime_lock(kdrv_ime_entry_id_conv2_handle(hdl), TRUE);
		}
	}

	dump("\r\n[KDRV IME]\r\n");
	dump("\r\n-----ctrl info-----\r\n");
	dump("%12s%12s\r\n", "init_flg", "trig_hdl");
	dump("%#12x 0x%.8x\r\n", g_kdrv_ime_init_flg, g_kdrv_ime_trig_hdl);

	/**
	    input info
	*/
	dump("\r\n-----input info-----\r\n");
	dump("%3s%12s%12s%12s%8s%14s%8s%8s%8s%8s%12s%12s\r\n", "hdl", "AddrY", "AddrU", "AddrV", "in_src", "type", "width", "height", "y_lofs", "uv_lofs", "strp_num", "strp_h_max");
	for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
		if (!is_ime_open[hdl]) {
			continue;
		}
		dump("%3d%#12x%#12x%#12x%8s%14s%8d%8d%8d%8d%12d%12d\r\n", hdl
			 , g_kdrv_ime_info[hdl].in_pixel_addr.addr_y, g_kdrv_ime_info[hdl].in_pixel_addr.addr_cb, g_kdrv_ime_info[hdl].in_pixel_addr.addr_cr
			 , kdrv_ime_in_src_tbl[g_kdrv_ime_info[hdl].in_img_info.in_src]
			 , kdrv_ime_in_fmt_tbl[g_kdrv_ime_info[hdl].in_img_info.type]
			 , g_kdrv_ime_info[hdl].in_img_info.ch[KDRV_IPP_YUV_Y].width, g_kdrv_ime_info[hdl].in_img_info.ch[KDRV_IPP_YUV_Y].height
			 , g_kdrv_ime_info[hdl].in_img_info.ch[KDRV_IPP_YUV_Y].line_ofs, g_kdrv_ime_info[hdl].in_img_info.ch[KDRV_IPP_YUV_U].line_ofs
			 , g_kdrv_ime_info[hdl].extend_info.stripe_num, g_kdrv_ime_info[hdl].extend_info.stripe_h_max);
	}

	/**
	    ouput info
	*/
	for (path_idx = 0; path_idx < KDRV_IME_PATH_ID_MAX; path_idx++) {

		if (path_idx > KDRV_IME_PATH_ID_4) {
			continue;
		}

		dump("\r\n-----KDRV_IME_PATH_ID_%d output info-----\r\n", path_idx + 1);
		dump("%3s%8s%12s%12s%12s%14s%8s%8s%8s%8s%8s%8s%8s%8s%12s\r\n", "hdl", "path_en", "AddrY", "AddrU", "AddrV", "type", "scl_w", "scl_h", "y_lofs", "uv_lofs", "crp_x", "crp_y", "crp_w", "crp_h", "out_enc_en");
		for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
			if (!is_ime_open[hdl]) {
				continue;
			}
			dump("%3d%8d%#12x%#12x%#12x%14s%8d%8d%8d%8d%8d%8d%8d%8d%12d\r\n", hdl
				 , g_kdrv_ime_info[hdl].out_path_info[path_idx].path_en
				 , g_kdrv_ime_info[hdl].out_path_info[path_idx].pixel_addr.addr_y, g_kdrv_ime_info[hdl].out_path_info[path_idx].pixel_addr.addr_cb, g_kdrv_ime_info[hdl].out_path_info[path_idx].pixel_addr.addr_cr
				 , kdrv_ime_out_fmt_tbl[g_kdrv_ime_info[hdl].out_path_info[path_idx].type]
				 , g_kdrv_ime_info[hdl].out_path_info[path_idx].ch[KDRV_IPP_YUV_Y].width, g_kdrv_ime_info[hdl].out_path_info[path_idx].ch[KDRV_IPP_YUV_Y].height
				 , g_kdrv_ime_info[hdl].out_path_info[path_idx].ch[KDRV_IPP_YUV_Y].line_ofs, g_kdrv_ime_info[hdl].out_path_info[path_idx].ch[KDRV_IPP_YUV_U].line_ofs
				 , g_kdrv_ime_info[hdl].out_path_info[path_idx].crp_window.x, g_kdrv_ime_info[hdl].out_path_info[path_idx].crp_window.y
				 , g_kdrv_ime_info[hdl].out_path_info[path_idx].crp_window.w, g_kdrv_ime_info[hdl].out_path_info[path_idx].crp_window.h
				 , g_kdrv_ime_info[hdl].out_path_info[path_idx].out_encode_info.enable);
		}
#if 0
		dump("\r\n-----KDRV_IME_PATH_ID_%d output separate info-----\r\n", path_hdlx + 1);
		dump("%3s%8s%8s%12s%12s%12s%12s%12s%16s\r\n", "hdl", "path_en", "sprt_en", "sprt_pos", "sprt_AddrY", "sprt_AddrU", "sprt_AddrV", "sprt_y_lofs", "sprt_uv_lofs");
		for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
			if (!bis_open[hdl]) {
				continue;
			}
			dump("%3d%8d%8d%12d%#12x%#12x%#12x%12d%16d\r\n", hdl
				 , g_kdrv_ime_info[hdl].out_path_info[path_hdlx].path_en
				 , g_kdrv_ime_info[hdl].out_path_info[path_hdlx].out_sprt_info.enable, g_kdrv_ime_info[hdl].out_path_info[path_hdlx].out_sprt_info.sprt_pos
				 , g_kdrv_ime_info[hdl].out_path_info[path_hdlx].out_sprt_addr.AddrY, g_kdrv_ime_info[hdl].out_path_info[path_hdlx].out_sprt_addr.AddrCb, g_kdrv_ime_info[hdl].out_path_info[path_hdlx].out_sprt_addr.AddrCr
				 , g_kdrv_ime_info[hdl].out_path_info[path_hdlx].out_sprt_info.sprt_out_line_ofs_y, g_kdrv_ime_info[hdl].out_path_info[path_hdlx].out_sprt_info.sprt_out_line_ofs_cb);
		}
#endif
	}

	/**
	    IQ function enable info
	*/
	dump("\r\n-----IQ function ON/OFF	-----\r\n");
	dump("%3s%4s%4s%12s%8s%4s%8s%8s%4s%4s%4s%4s%4s%8s\r\n", "hdl"
		 , "CMF", "LCA", "LCA_Sub_Out", "DBCS", "SSR", "CST_R2Y", "CST_Y2R", "FG", "DS", "P2I", "STL", "PM", "TMNR");
	for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
		if (!is_ime_open[hdl]) {
			continue;
		}
		dump("%3d%4d%4d%12d%8d%4d%8d%8d%4d%4d%4d%4d%4d%8d\r\n", hdl
			 , g_kdrv_ime_info[hdl].sub_modules.chroma_filter.enable
			 , g_kdrv_ime_info[hdl].sub_modules.lca_en_ctrl.enable
			 , g_kdrv_ime_info[hdl].sub_modules.lca_subout.enable
			 , g_kdrv_ime_info[hdl].sub_modules.dbcs.enable
			 , g_kdrv_ime_info[hdl].sub_modules.ssr.enable
			 , g_kdrv_ime_info[hdl].sub_modules.rgb2ycc.enable
			 , g_kdrv_ime_info[hdl].sub_modules.ycc2rgb.enable
			 , g_kdrv_ime_info[hdl].sub_modules.film_grain.enable
			 , (g_kdrv_ime_info[hdl].sub_modules.ds[KDRV_IME_OSD_SET_IDX_0].enable | g_kdrv_ime_info[hdl].sub_modules.ds[KDRV_IME_OSD_SET_IDX_1].enable | g_kdrv_ime_info[hdl].sub_modules.ds[KDRV_IME_OSD_SET_IDX_2].enable | g_kdrv_ime_info[hdl].sub_modules.ds[KDRV_IME_OSD_SET_IDX_3].enable)
			 , g_kdrv_ime_info[hdl].sub_modules.p2i.enable
			 , g_kdrv_ime_info[hdl].sub_modules.stl.stl_info.enable
			 , (g_kdrv_ime_info[hdl].sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_0].enable | g_kdrv_ime_info[hdl].sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_1].enable | g_kdrv_ime_info[hdl].sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_2].enable | g_kdrv_ime_info[hdl].sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_3].enable | g_kdrv_ime_info[hdl].sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_4].enable | g_kdrv_ime_info[hdl].sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_5].enable | g_kdrv_ime_info[hdl].sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_6].enable | g_kdrv_ime_info[hdl].sub_modules.pm.pm_info[KDRV_IME_PM_SET_IDX_7].enable)
			 , g_kdrv_ime_info[hdl].sub_modules.tmnr.nr_iq.enable);
	}

	dump("\r\n-----LCA Parameters	-----\r\n");
	dump("%3s%12s%12s%12s%12s%12s%16s%12s%12s%12s\r\n", "hdl"
		 , "ref_in_addr", "ref_in_w", "ref_in_h", "ref_in_lofs", "ref_in_src", "sub_out_addr", "sub_out_w", "sub_out_h", "sub_out_lofs");
	for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
		if (!is_ime_open[hdl]) {
			continue;
		}
		dump("%3d%#12x%12d%12d%12d%12d%#16x%12d%12d%12d\r\n", hdl
			 , g_kdrv_ime_info[hdl].sub_modules.lca_img.dma_addr
			 , g_kdrv_ime_info[hdl].sub_modules.lca_img.img_size.width
			 , g_kdrv_ime_info[hdl].sub_modules.lca_img.img_size.height
			 , g_kdrv_ime_info[hdl].sub_modules.lca_img.img_size.line_ofs
			 , g_kdrv_ime_info[hdl].sub_modules.lca_img.in_src
			 , g_kdrv_ime_info[hdl].sub_modules.lca_subout.sub_out_addr
			 , g_kdrv_ime_info[hdl].sub_modules.lca_subout.sub_out_size.width
			 , g_kdrv_ime_info[hdl].sub_modules.lca_subout.sub_out_size.height
			 , g_kdrv_ime_info[hdl].sub_modules.lca_subout.sub_out_size.line_ofs);
	}

	dump("\r\n-----Data Stamp Parameters	-----\r\n");
	dump("%3s%8s%12s%12s%12s%16s%12s%8s%8s%16s%16s%12s%12s\r\n", "hdl"
		 , "index", "img_addr", "img_size_w", "img_size_h", "ime_size_lofs", "img_fmt", "pos_x", "pos_y", "color_key_en", "color_key_val", "bld_wt_0", "bld_wt_0");
	for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
		if (!is_ime_open[hdl]) {
			continue;
		}
		for (ds_hdlx = 0; ds_hdlx < KDRV_IME_OSD_SET_IDX_MAX; ds_hdlx++) {
			if (g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].enable == ENABLE) {
				dump("%3d%8d%#12x%12d%12d%16d%12d%8d%8d%16d%16d%12d%12d\r\n", hdl
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].ds_set_idx
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].image.img_addr
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].image.img_size.w
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].image.img_size.h
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].image.ime_lofs
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].image.img_fmt
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].image.pos.x
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].image.pos.y
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].ds_iq.color_key_en
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].ds_iq.color_key_val
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].ds_iq.bld_wt_0
					 , g_kdrv_ime_info[hdl].sub_modules.ds[ds_hdlx].ds_iq.bld_wt_1);
			}
		}
	}

	dump("\r\n-----Primacy Mask Parameters	-----\r\n");
	dump("%3s%12s%12s%12s%12s%16s\r\n", "hdl"
		 , "ref_in_addr", "ref_in_w", "ref_in_h", "ref_in_lofs", "pxl_blk_size");
	for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
		if (!is_ime_open[hdl]) {
			continue;
		}
		dump("%3d%#12x%12d%12d%12d%16d\r\n", hdl
			 , g_kdrv_ime_info[hdl].sub_modules.pm.pm_pxl_img_info.dma_addr
			 , g_kdrv_ime_info[hdl].sub_modules.pm.pm_pxl_img_info.img_size.w
			 , g_kdrv_ime_info[hdl].sub_modules.pm.pm_pxl_img_info.img_size.h
			 , g_kdrv_ime_info[hdl].sub_modules.pm.pm_pxl_img_info.lofs
			 , g_kdrv_ime_info[hdl].sub_modules.pm.pm_pxl_img_info.blk_size);
	}

	dump("\r\n");

	for (hdl = 0; hdl < kdrv_ime_channel_num; hdl++) {
		if (is_ime_open[hdl]) {
			kdrv_ime_lock(kdrv_ime_entry_id_conv2_handle(hdl), FALSE);
		}
	}

	dump("\r\n-----HW output path ON/OFF	-----\r\n");
	dump("%12s%12s%12s%12s\r\n"
		 , "Output_P1", "Output_P2", "Output_P3", "Output_P4");
	dump("%12d%12d%12d%12d\r\n"
		 , (UINT32)ime_get_func_enable_info(IME_OUTPUT_P1)
		 , (UINT32)ime_get_func_enable_info(IME_OUTPUT_P2)
		 , (UINT32)ime_get_func_enable_info(IME_OUTPUT_P3)
		 , (UINT32)ime_get_func_enable_info(IME_OUTPUT_P4)
		);


	dump("\r\n-----HW function ON/OFF	-----\r\n");
	dump("%4s%4s%12s%8s%4s%8s%8s%4s%4s%4s%4s%4s%8s\r\n"
		 , "LCA", "LCA_Sub_Out", "DBCS", "DS", "STL", "PM", "TMNR");
	dump("%4d%4d%12d%8d%4d%8d%8d%4d%4d%4d%4d%4d%8d\r\n"
		 , (UINT32)ime_get_func_enable_info(IME_LCA)
		 , (UINT32)ime_get_func_enable_info(IME_LCA_SUBOUT)
		 , (UINT32)ime_get_func_enable_info(IME_DBCS)
		 , (UINT32)((UINT32)ime_get_func_enable_info(IME_OSD0) | (UINT32)ime_get_func_enable_info(IME_OSD1) | (UINT32)ime_get_func_enable_info(IME_OSD2) | (UINT32)ime_get_func_enable_info(IME_OSD3))
		 , (UINT32)ime_get_func_enable_info(IME_STL)
		 , (UINT32)((UINT32)ime_get_func_enable_info(IME_PM0) | (UINT32)ime_get_func_enable_info(IME_PM1) | (UINT32)ime_get_func_enable_info(IME_PM2) | (UINT32)ime_get_func_enable_info(IME_PM3) | (UINT32)ime_get_func_enable_info(IME_PM4) | (UINT32)ime_get_func_enable_info(IME_PM5) | (UINT32)ime_get_func_enable_info(IME_PM6) | (UINT32)ime_get_func_enable_info(IME_PM7))
		 , (UINT32)ime_get_func_enable_info(IME_3DNR)
		);
}


#if defined (__KERNEL__)
EXPORT_SYMBOL(kdrv_ime_open);
EXPORT_SYMBOL(kdrv_ime_pause);
EXPORT_SYMBOL(kdrv_ime_close);
EXPORT_SYMBOL(kdrv_ime_trigger);
EXPORT_SYMBOL(kdrv_ime_set);
EXPORT_SYMBOL(kdrv_ime_get);
EXPORT_SYMBOL(kdrv_ime_dump_info);

EXPORT_SYMBOL(kdrv_ime_init);
EXPORT_SYMBOL(kdrv_ime_uninit);
EXPORT_SYMBOL(kdrv_ime_buf_query);
EXPORT_SYMBOL(kdrv_ime_buf_init);
EXPORT_SYMBOL(kdrv_ime_buf_uninit);
#endif

