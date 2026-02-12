/**
    @file       kdrv_ife2.c
    @ingroup    Predefined_group_name

    @brief      ife2 device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#include "ife2_platform.h"

#include "kdrv_type.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "kdrv_videoprocess/kdrv_ife2.h"
#include "kdrv_ife2_int.h"
#include "kdrv_videoprocess/kdrv_ife2_lmt.h"

typedef struct _KDRV_IFE2_FUNC_CTRL_ {
	union {
		struct {
			unsigned filter       : 1;        // bits : 0
		} bit;
		UINT32 val;
	} ife2_ctrl_bit;
} KDRV_IFE2_FUNC_CTRL;

typedef struct _KDRV_IFE2_FUNC_INT_CTRL_ {
	KDRV_IFE2_FUNC_CTRL func_sel;
	KDRV_IFE2_FUNC_CTRL func_op;
} KDRV_IFE2_FUNC_INT_CTRL;


static UINT32 g_kdrv_ife2_open_cnt[KDRV_IFE2_ID_MAX_NUM] = {0};

KDRV_IFE2_REFCENT_PARAM refcent_default = {
	{
		{4, 8, 12},
		{8, 4, 2, 1},
		16,
		9,
		6
	},
	{
		{8, 12, 16},
		{8, 4, 2, 1},
		16,
		9,
		6
	},
};

KDRV_IFE2_FILTER_PARAM filter_default = {
	TRUE,
	KDRV_IFE2_FLTR_SIZE_9x9,
	{32, 32},
	KDRV_IFE2_EKNL_SIZE_3x3,
	{
		{4, 8, 12, 16, 20},
		{16, 8, 4, 2, 1, 0}
	},
	{
		{8, 12, 16, 20, 24},
		{16, 8, 4, 2, 1, 0}
	},
	{
		{8, 12, 16, 20, 24},
		{16, 8, 4, 2, 1, 0}
	}
};


IFE2_PARAM g_ife2_set_param = {0};

static UINT32                   g_kdrv_ife2_init_flg                 = 0;
static KDRV_IFE2_HANDLE         *g_kdrv_ife2_trig_hdl                = NULL;

#if 0
static KDRV_IFE2_HANDLE             g_kdrv_ife2_handle_tab[KDRV_IFE2_HANDLE_MAX_NUM];
KDRV_IFE2_PRAM                   g_kdrv_ife2_info[KDRV_IFE2_HANDLE_MAX_NUM] = {0};

static KDRV_IFE2_FUNC_CTRL g_kdrv_ife2_iq_func_en[KDRV_IFE2_HANDLE_MAX_NUM] = {0};
static KDRV_IFE2_FUNC_INT_CTRL g_kdrv_ife2_ipl_func_en[KDRV_IFE2_HANDLE_MAX_NUM] = {0};

KDRV_IFE2_OPENCFG g_kdrv_ife2_clk_info[KDRV_IFE2_HANDLE_MAX_NUM] = {
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
#else

static KDRV_IFE2_HANDLE *g_kdrv_ife2_handle_tab = NULL;
KDRV_IFE2_PRAM *g_kdrv_ife2_info = NULL;

static KDRV_IFE2_FUNC_CTRL *g_kdrv_ife2_iq_func_en = NULL;
static KDRV_IFE2_FUNC_INT_CTRL *g_kdrv_ife2_ipl_func_en = NULL;
KDRV_IFE2_OPENCFG *g_kdrv_ife2_clk_info = NULL;

#endif

BOOL g_kdrv_ife2_set_clk_flg[KDRV_IFE2_ID_MAX_NUM] = {
	FALSE
};

static uint8_t g_ife2_update_param[KDRV_IPP_MAX_CHANNEL_NUM][KDRV_IFE2_PARAM_MAX] = {FALSE};
static uint8_t g_ife2_int_update_param[KDRV_IFE2_PARAM_MAX] = {FALSE};


UINT32 kdrv_ife2_channel_num = KDRV_IFE2_HANDLE_MAX_NUM;

static ER kdrv_ife2_set_clk(UINT32 id, VOID *data)
{
	UINT32 i = 0;
	KDRV_IFE2_OPENCFG *p_open_info = NULL;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_OK;
	}

	p_open_info = (KDRV_IFE2_OPENCFG *)data;

	for (i = 0; i < kdrv_ife2_channel_num; i++) {
		g_kdrv_ife2_clk_info[i].ife2_clock_sel = p_open_info->ife2_clock_sel;
	}

	g_kdrv_ife2_set_clk_flg[KDRV_IFE2_ID_0] = TRUE;

	return E_OK;
}


static ER kdrv_ife2_param_init(INT32 id)
{
	g_kdrv_ife2_info[id].refcent = refcent_default;

#if 0
	g_kdrv_ife2_info[id].p_refcent->refcent_y.rng_th_addr  = (UINT32)&g_rng_th_y[id][0];
	g_kdrv_ife2_info[id].p_refcent->refcent_y.rng_wt_addr  = (UINT32)&g_rng_wt_y[id][0];
	g_kdrv_ife2_info[id].p_refcent->refcent_uv.rng_th_addr = (UINT32)&g_rng_th_uv[id][0];
	g_kdrv_ife2_info[id].p_refcent->refcent_uv.rng_wt_addr = (UINT32)&g_rng_wt_uv[id][0];
#endif

	g_kdrv_ife2_info[id].filter = filter_default;

#if 0
	g_kdrv_ife2_info[id].p_filter->set_y.filt_th_addr  = (UINT32)&g_filt_th_y[id][0];
	g_kdrv_ife2_info[id].p_filter->set_u.filt_th_addr  = (UINT32)&g_filt_th_u[id][0];
	g_kdrv_ife2_info[id].p_filter->set_v.filt_th_addr  = (UINT32)&g_filt_th_v[id][0];
	g_kdrv_ife2_info[id].p_filter->set_y.filt_wt_addr  = (UINT32)&g_filt_wt_y[id][0];
	g_kdrv_ife2_info[id].p_filter->set_u.filt_wt_addr  = (UINT32)&g_filt_wt_u[id][0];
	g_kdrv_ife2_info[id].p_filter->set_v.filt_wt_addr  = (UINT32)&g_filt_wt_v[id][0];
#endif

	return E_OK;
}

static VOID kdrv_ife2_int_set_param_init(UINT32 id)
{
	KDRV_IFE2_FUNC_CTRL func_sel = {0}, func_op = {0};
	//unsigned long loc_status;
	uint8_t iq_en;

	memset((void *)&g_ife2_set_param, 0x0, sizeof(IFE2_PARAM));

	// set ife2 IQ function enable with IPL flow control

	//loc_status = ife2_platform_spin_lock();

	func_sel = g_kdrv_ife2_ipl_func_en[id].func_sel;
	func_op  = g_kdrv_ife2_ipl_func_en[id].func_op;

	if (func_sel.ife2_ctrl_bit.bit.filter == ENABLE) {
		iq_en = g_kdrv_ife2_iq_func_en[id].ife2_ctrl_bit.bit.filter;

		if ((func_op.ife2_ctrl_bit.bit.filter == ENABLE) && (iq_en == ENABLE)) {
			g_kdrv_ife2_info[id].filter.enable = ENABLE;
		} else {
			g_kdrv_ife2_info[id].filter.enable = DISABLE;
		}
	}

	//ife2_platform_spin_unlock(loc_status);
}


static ER kdrv_ife2_setmode(INT32 id)
{
	ER rt;
	UINT32 i = 0;

	g_kdrv_ife2_info[id].burst.input = KDRV_IFE2_BURST_LEN_32;

	// ife2 input
	if (g_kdrv_ife2_info[id].out_img_info.out_dst == KDRV_IFE2_OUT_DST_DIRECT) {
		g_ife2_set_param.op_mode             = IFE2_OPMODE_IFE_IPP;
		g_ife2_set_param.src_fmt             = KDRV_IFE2_IN_FMT_PACK_YUV444;
	} else if (g_kdrv_ife2_info[id].out_img_info.out_dst == KDRV_IFE2_OUT_DST_ALL_DIRECT) {
		g_ife2_set_param.op_mode             = IFE2_OPMODE_SIE_IPP;
		g_ife2_set_param.src_fmt             = KDRV_IFE2_IN_FMT_PACK_YUV444;
	} else {  //D2D
		g_ife2_set_param.op_mode             = IFE2_OPMODE_D2D;
		g_ife2_set_param.src_fmt             = g_kdrv_ife2_info[id].in_img_info.type;
	}

	g_ife2_set_param.in_addr.chl_sel          = IFE2_DMAHDL_IN;
	g_ife2_set_param.in_addr.addr.addr_y    = g_kdrv_ife2_info[id].in_pixel_addr;
	g_ife2_set_param.img_size.img_size_h     = g_kdrv_ife2_info[id].in_img_info.ch.width;
	g_ife2_set_param.img_size.img_size_v     = g_kdrv_ife2_info[id].in_img_info.ch.height;
	g_ife2_set_param.in_lofs.chl_sel           = IFE2_DMAHDL_IN;
	g_ife2_set_param.in_lofs.lofs.lofs_y = g_kdrv_ife2_info[id].in_img_info.ch.line_ofs;

	// ife2 output
	g_ife2_set_param.dst_fmt                 = g_ife2_set_param.src_fmt;

	g_ife2_set_param.out_addr0.chl_sel        = IFE2_DMAHDL_OUT0;
	g_ife2_set_param.out_addr0.addr.addr_y  = g_kdrv_ife2_info[id].out_pixel_addr;
	g_ife2_set_param.out_lofs.chl_sel          = IFE2_DMAHDL_OUT0;
	g_ife2_set_param.out_lofs.lofs.lofs_y = g_kdrv_ife2_info[id].out_img_info.line_ofs;

	// ife2 interrupt
	g_ife2_set_param.interrupt_en = g_kdrv_ife2_info[id].inte_en;

	// ife IQ setting
	g_ife2_set_param.gray_sta.u_th0 = g_kdrv_ife2_info[id].gray_statist.u_th0;
	g_ife2_set_param.gray_sta.u_th1 = g_kdrv_ife2_info[id].gray_statist.u_th1;
	g_ife2_set_param.gray_sta.v_th0 = g_kdrv_ife2_info[id].gray_statist.v_th0;
	g_ife2_set_param.gray_sta.v_th1 = g_kdrv_ife2_info[id].gray_statist.v_th1;

	//g_ife2_set_param.ref_cent.ref_cent_y.range_th [0] = g_rng_th_y [id][0];
	//g_ife2_set_param.ref_cent.ref_cent_y.range_th [1] = g_rng_th_y [id][1];
	//g_ife2_set_param.ref_cent.ref_cent_y.range_th [2] = g_rng_th_y [id][2];

	for (i = 0; i < KDRV_IFE2_RANGE_TH_TAB; i++) {
		g_ife2_set_param.ref_cent.ref_cent_y.range_th[i] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_th[i];
	}

	//g_ife2_set_param.ref_cent.ref_cent_uv.range_th[0] = g_rng_th_uv[id][0];
	//g_ife2_set_param.ref_cent.ref_cent_uv.range_th[1] = g_rng_th_uv[id][1];
	//g_ife2_set_param.ref_cent.ref_cent_uv.range_th[2] = g_rng_th_uv[id][2];
	for (i = 0; i < KDRV_IFE2_RANGE_TH_TAB; i++) {
		g_ife2_set_param.ref_cent.ref_cent_uv.range_th[i] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_th[i];
	}

	//g_ife2_set_param.ref_cent.ref_cent_y.range_wet [0] = g_rng_wt_y [id][0];
	//g_ife2_set_param.ref_cent.ref_cent_y.range_wet [1] = g_rng_wt_y [id][1];
	//g_ife2_set_param.ref_cent.ref_cent_y.range_wet [2] = g_rng_wt_y [id][2];
	//g_ife2_set_param.ref_cent.ref_cent_y.range_wet [3] = g_rng_wt_y [id][3];
	for (i = 0; i < KDRV_IFE2_RANGE_WT_TAB; i++) {
		g_ife2_set_param.ref_cent.ref_cent_y.range_wet[i] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_wt[i];
	}

	//g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[0] = g_rng_wt_uv[id][0];
	//g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[1] = g_rng_wt_uv[id][1];
	//g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[2] = g_rng_wt_uv[id][2];
	//g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[3] = g_rng_wt_uv[id][3];
	for (i = 0; i < KDRV_IFE2_RANGE_WT_TAB; i++) {
		g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[i] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_wt[i];
	}

	g_ife2_set_param.ref_cent.ref_cent_y.cnt_wet   = g_kdrv_ife2_info[id].refcent.refcent_y.cent_wt;
	g_ife2_set_param.ref_cent.ref_cent_y.outlier_dth = g_kdrv_ife2_info[id].refcent.refcent_y.outl_dth;
	g_ife2_set_param.ref_cent.ref_cent_y.outlier_th  = g_kdrv_ife2_info[id].refcent.refcent_y.outl_th;

	g_ife2_set_param.ref_cent.ref_cent_uv.cnt_wet  = g_kdrv_ife2_info[id].refcent.refcent_uv.cent_wt;
	g_ife2_set_param.ref_cent.ref_cent_uv.outlier_dth = g_kdrv_ife2_info[id].refcent.refcent_uv.outl_dth;
	g_ife2_set_param.ref_cent.ref_cent_uv.outlier_th = g_kdrv_ife2_info[id].refcent.refcent_uv.outl_th;

	g_ife2_set_param.filter.filter_y_en             = g_kdrv_ife2_info[id].filter.enable;
	g_ife2_set_param.filter.filter_size            = g_kdrv_ife2_info[id].filter.size;
	g_ife2_set_param.filter.filter_edth.edth_hv   = g_kdrv_ife2_info[id].filter.edg_dir.hv_th;
	g_ife2_set_param.filter.filter_edth.edth_pn   = g_kdrv_ife2_info[id].filter.edg_dir.pn_th;
	g_ife2_set_param.filter.edge_ker_size        = g_kdrv_ife2_info[id].filter.edg_ker_size;

	//g_ife2_set_param.filter.filter_set_y.filter_th[0] = g_filt_th_y[id][0];
	//g_ife2_set_param.filter.filter_set_y.filter_th[1] = g_filt_th_y[id][1];
	//g_ife2_set_param.filter.filter_set_y.filter_th[2] = g_filt_th_y[id][2];
	//g_ife2_set_param.filter.filter_set_y.filter_th[3] = g_filt_th_y[id][3];
	//g_ife2_set_param.filter.filter_set_y.filter_th[4] = g_filt_th_y[id][4];
	for (i = 0; i < KDRV_IFE2_FILTER_TH_TAB; i++) {
		g_ife2_set_param.filter.filter_set_y.filter_th[i] = g_kdrv_ife2_info[id].filter.set_y.filt_th[i];
	}

	//g_ife2_set_param.filter.filter_set_u.filter_th[0] = g_filt_th_u[id][0];
	//g_ife2_set_param.filter.filter_set_u.filter_th[1] = g_filt_th_u[id][1];
	//g_ife2_set_param.filter.filter_set_u.filter_th[2] = g_filt_th_u[id][2];
	//g_ife2_set_param.filter.filter_set_u.filter_th[3] = g_filt_th_u[id][3];
	//g_ife2_set_param.filter.filter_set_u.filter_th[4] = g_filt_th_u[id][4];
	for (i = 0; i < KDRV_IFE2_FILTER_TH_TAB; i++) {
		g_ife2_set_param.filter.filter_set_u.filter_th[i] = g_kdrv_ife2_info[id].filter.set_u.filt_th[i];
	}

	//g_ife2_set_param.filter.filter_set_v.filter_th[0] = g_filt_th_v[id][0];
	//g_ife2_set_param.filter.filter_set_v.filter_th[1] = g_filt_th_v[id][1];
	//g_ife2_set_param.filter.filter_set_v.filter_th[2] = g_filt_th_v[id][2];
	//g_ife2_set_param.filter.filter_set_v.filter_th[3] = g_filt_th_v[id][3];
	//g_ife2_set_param.filter.filter_set_v.filter_th[4] = g_filt_th_v[id][4];
	for (i = 0; i < KDRV_IFE2_FILTER_TH_TAB; i++) {
		g_ife2_set_param.filter.filter_set_v.filter_th[i] = g_kdrv_ife2_info[id].filter.set_v.filt_th[i];
	}

	//g_ife2_set_param.filter.filter_set_y.filter_wet[0] = g_filt_wt_y[id][0];
	//g_ife2_set_param.filter.filter_set_y.filter_wet[1] = g_filt_wt_y[id][1];
	//g_ife2_set_param.filter.filter_set_y.filter_wet[2] = g_filt_wt_y[id][2];
	//g_ife2_set_param.filter.filter_set_y.filter_wet[3] = g_filt_wt_y[id][3];
	//g_ife2_set_param.filter.filter_set_y.filter_wet[4] = g_filt_wt_y[id][4];
	//g_ife2_set_param.filter.filter_set_y.filter_wet[5] = g_filt_wt_y[id][5];
	for (i = 0; i < KDRV_IFE2_FILTER_WT_TAB; i++) {
		g_ife2_set_param.filter.filter_set_y.filter_wet[i] = g_kdrv_ife2_info[id].filter.set_y.filt_wt[i];
	}

	//g_ife2_set_param.filter.filter_set_u.filter_wet[0] = g_filt_wt_u[id][0];
	//g_ife2_set_param.filter.filter_set_u.filter_wet[1] = g_filt_wt_u[id][1];
	//g_ife2_set_param.filter.filter_set_u.filter_wet[2] = g_filt_wt_u[id][2];
	//g_ife2_set_param.filter.filter_set_u.filter_wet[3] = g_filt_wt_u[id][3];
	//g_ife2_set_param.filter.filter_set_u.filter_wet[4] = g_filt_wt_u[id][4];
	//g_ife2_set_param.filter.filter_set_u.filter_wet[5] = g_filt_wt_u[id][5];
	for (i = 0; i < KDRV_IFE2_FILTER_WT_TAB; i++) {
		g_ife2_set_param.filter.filter_set_u.filter_wet[i] = g_kdrv_ife2_info[id].filter.set_u.filt_wt[i];
	}

	//g_ife2_set_param.filter.filter_set_v.filter_wet[0] = g_filt_wt_v[id][0];
	//g_ife2_set_param.filter.filter_set_v.filter_wet[1] = g_filt_wt_v[id][1];
	//g_ife2_set_param.filter.filter_set_v.filter_wet[2] = g_filt_wt_v[id][2];
	//g_ife2_set_param.filter.filter_set_v.filter_wet[3] = g_filt_wt_v[id][3];
	//g_ife2_set_param.filter.filter_set_v.filter_wet[4] = g_filt_wt_v[id][4];
	//g_ife2_set_param.filter.filter_set_v.filter_wet[5] = g_filt_wt_v[id][5];
	for (i = 0; i < KDRV_IFE2_FILTER_WT_TAB; i++) {
		g_ife2_set_param.filter.filter_set_v.filter_wet[i] = g_kdrv_ife2_info[id].filter.set_v.filt_wt[i];
	}

	g_ife2_set_param.bst_size.in_bst_len = g_kdrv_ife2_info[id].burst.input;
	g_ife2_set_param.bst_size.out_bst_len = IFE2_BURST_32W;

	g_ife2_set_param.out_dram_en = IFE2_FUNC_DISABLE;

	rt = ife2_set_mode(&g_ife2_set_param);
	return rt;
}

#if 0
#endif
static void kdrv_ife2_lock(KDRV_IFE2_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}


static KDRV_IFE2_HANDLE *kdrv_ife2_chk_handle(KDRV_IFE2_HANDLE *p_handle)
{
	UINT32 i;

	for (i = 0; i < kdrv_ife2_channel_num; i ++) {
		if (p_handle == &g_kdrv_ife2_handle_tab[i]) {
			return p_handle;
		}
	}

	return NULL;
}

static void kdrv_ife2_handle_lock(void)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, kdrv_ife2_get_flag_id(FLG_ID_KDRV_IFE2), KDRV_IPP_IFE2_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}


static void kdrv_ife2_handle_unlock(void)
{
	set_flg(kdrv_ife2_get_flag_id(FLG_ID_KDRV_IFE2), KDRV_IPP_IFE2_HDL_UNLOCK);
}

static BOOL kdrv_ife2_channel_alloc(UINT32 chl_id)
{
	BOOL chl_init_flag;
	KDRV_IFE2_HANDLE *p_handle;
	UINT32 i = 0;


	p_handle = NULL;
	chl_init_flag = FALSE;
	i = chl_id;
	if ((g_kdrv_ife2_init_flg & (1 << i)) == 0) {

		kdrv_ife2_handle_lock();

		if (g_kdrv_ife2_init_flg == 0) {
			chl_init_flag = TRUE;
		}

		g_kdrv_ife2_init_flg |= (1 << i);

		memset(&g_kdrv_ife2_handle_tab[i], 0, sizeof(KDRV_IFE2_HANDLE));
		memset(&g_kdrv_ife2_iq_func_en[i], 0, sizeof(KDRV_IFE2_FUNC_CTRL));
		memset(&g_kdrv_ife2_ipl_func_en[i], 0, sizeof(KDRV_IFE2_FUNC_INT_CTRL));
		memset(&g_kdrv_ife2_info[i], 0, sizeof(KDRV_IFE2_PRAM));

		g_kdrv_ife2_handle_tab[i].entry_id = i;
		g_kdrv_ife2_handle_tab[i].flag_id = kdrv_ife2_get_flag_id(FLG_ID_KDRV_IFE2);
		g_kdrv_ife2_handle_tab[i].lock_bit = (1 << i);
		g_kdrv_ife2_handle_tab[i].sts |= KDRV_IFE2_HANDLE_LOCK;
		g_kdrv_ife2_handle_tab[i].sem_id = kdrv_ife2_get_sem_id(SEMID_KDRV_IFE2);
		p_handle = &g_kdrv_ife2_handle_tab[i];

		kdrv_ife2_handle_unlock();


		kdrv_ife2_lock(p_handle, TRUE);
		kdrv_ife2_param_init(p_handle->entry_id);
		kdrv_ife2_lock(p_handle, FALSE);
	}

	return chl_init_flag;
}

#if 0
static BOOL kdrv_ife2_channel_free(KDRV_IFE2_HANDLE *p_handle)
{
	BOOL rt = FALSE;

	kdrv_ife2_handle_lock();
	p_handle->sts = 0;
	g_kdrv_ife2_init_flg &= ~(1 << p_handle->entry_id);

	if (g_kdrv_ife2_init_flg == 0) {
		rt = TRUE;
	}
	kdrv_ife2_handle_unlock();

	return rt;
}
#endif

#if 0
static KDRV_IFE2_HANDLE *kdrv_ife2_handle_alloc(UINT32 *eng_init_flag)
{
	UINT32 i;
	KDRV_IFE2_HANDLE *p_handle;

	kdrv_ife2_handle_lock();
	p_handle = NULL;
	*eng_init_flag = FALSE;
	for (i = 0; i < KDRV_IFE2_HANDLE_MAX_NUM; i ++) {
		if (!(g_kdrv_ife2_init_flg & (1 << i))) {

			if (g_kdrv_ife2_init_flg == 0) {
				*eng_init_flag = TRUE;
			}

			g_kdrv_ife2_init_flg |= (1 << i);

			memset(&g_kdrv_ife2_handle_tab[i], 0, sizeof(KDRV_IFE2_HANDLE));
			g_kdrv_ife2_handle_tab[i].entry_id = i;
			g_kdrv_ife2_handle_tab[i].flag_id = kdrv_ife2_get_flag_id(FLG_ID_KDRV_IFE2);
			g_kdrv_ife2_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_ife2_handle_tab[i].sts |= KDRV_IFE2_HANDLE_LOCK;
			g_kdrv_ife2_handle_tab[i].sem_id = kdrv_ife2_get_sem_id(SEMID_KDRV_IFE2);
			p_handle = &g_kdrv_ife2_handle_tab[i];
			break;
		}
	}
	kdrv_ife2_handle_unlock();

	if (i >= KDRV_IFE2_HANDLE_MAX_NUM) {
		DBG_ERR("get free handle fail(0x%.8x)\r\n", (unsigned int)g_kdrv_ife2_init_flg);
	}
	return p_handle;
}

static UINT32 kdrv_ife2_handle_free(KDRV_IFE2_HANDLE *p_handle)
{
	UINT32 rt = FALSE;
	kdrv_ife2_handle_lock();
	p_handle->sts = 0;
	g_kdrv_ife2_init_flg &= ~(1 << p_handle->entry_id);
	if (g_kdrv_ife2_init_flg == 0) {
		rt = TRUE;
	}
	kdrv_ife2_handle_unlock();

	return rt;
}
#endif

static KDRV_IFE2_HANDLE *kdrv_ife2_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_ife2_handle_tab[entry_id];
}


static void kdrv_ife2_sem(KDRV_IFE2_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		SEM_WAIT(*p_handle->sem_id);    // wait semaphore
	} else {
		//SEM_SIGNAL(*p_handle->sem_id);  // wait semaphore
		SEM_SIGNAL_ISR(*p_handle->sem_id);
	}
}

static void kdrv_ife2_frm_end(KDRV_IFE2_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		//set_flg(p_handle->flag_id, KDRV_IPP_IFE2_FMD);
		iset_flg(p_handle->flag_id, KDRV_IPP_IFE2_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_IPP_IFE2_FMD);
	}
}

static void kdrv_ife2_isr_cb(UINT32 intstatus)
{
	KDRV_IFE2_HANDLE *p_handle;
	p_handle = g_kdrv_ife2_trig_hdl;

	if (p_handle == NULL) {
		return;
	}

	if (intstatus & IFE2_INTS_FRMEND) {
		//ife2_pause();
		kdrv_ife2_sem(p_handle, FALSE);
		kdrv_ife2_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
	}
}

#if 0
#endif
INT32 kdrv_ife2_open(UINT32 chip, UINT32 engine)
{
	UINT32 i, j;
	IFE2_OPENOBJ ife2_drv_obj;
	KDRV_IFE2_OPENCFG kdrv_ife2_open_obj;
	//KDRV_IFE2_HANDLE* p_handle;
	//UINT32 eng_init_flag;

	KDRV_IFE2_ID eng_id;

	switch (engine) {
	case KDRV_VIDEOPROCS_IFE2_ENGINE0:
		eng_id = KDRV_IFE2_ID_0;
		break;

	default:
		DBG_ERR("KDRV IFE2: engine id error...\r\n");
		return -1;
		break;
	}

	//p_handle = kdrv_ife2_handle_alloc(&eng_init_flag);
	//if (p_handle == NULL) {
	//  DBG_ERR("DAL IFE2: no free handle, max handle num = %d\r\n", KDRV_IFE2_HANDLE_MAX_NUM);
	//  return -1;
	//}

	//if (eng_init_flag) {
	if ((g_kdrv_ife2_open_cnt[eng_id] == 0x0)) {
		//
		if (g_kdrv_ife2_set_clk_flg[eng_id] != TRUE) {
			DBG_WRN("KDRV IFE2: no open info. from user, using default...\r\n");
		}

		kdrv_ife2_open_obj = g_kdrv_ife2_clk_info[(UINT32)eng_id];
		ife2_drv_obj.uiIfe2ClkSel = kdrv_ife2_open_obj.ife2_clock_sel;
		ife2_drv_obj.pfIfe2IsrCb = kdrv_ife2_isr_cb;

		if (ife2_open(&ife2_drv_obj) != E_OK) {
			//kdrv_ife2_handle_free(p_handle);
			DBG_ERR("KDRV IFE2: ife2_open failed\r\n");
			return -1;
		}

		for (j = 0; j < kdrv_ife2_channel_num; j++) {
			for (i = 0; i < KDRV_IFE2_PARAM_MAX; i++) {
				g_ife2_update_param[j][i] = FALSE;
			}
		}
	}

	//kdrv_ife2_lock(p_handle, TRUE);
	//kdrv_ife2_init(p_handle->entry_id);
	//kdrv_ife2_lock(p_handle, FALSE);

	g_kdrv_ife2_open_cnt[eng_id] += 1;

	return 0;
}


INT32 kdrv_ife2_close(UINT32 chip, UINT32 engine)
{
	//ER rt = E_OK;
	//KDRV_IFE2_HANDLE* p_handle;

	KDRV_IFE2_ID eng_id;

	switch (engine) {
	case KDRV_VIDEOPROCS_IFE2_ENGINE0:
		eng_id = KDRV_IFE2_ID_0;

		g_kdrv_ife2_open_cnt[eng_id] -= 1;

		if (g_kdrv_ife2_open_cnt[eng_id] == 0) {
			if (ife2_close() != E_OK) {
				//
				DBG_ERR("KDRV IFE2: engine close error...\r\n");
				return -1;
			}

			g_kdrv_ife2_init_flg = 0x0;
			g_kdrv_ife2_set_clk_flg[KDRV_IFE2_ID_0] = FALSE;

			g_kdrv_ife2_trig_hdl = NULL;
		}

		break;

	default:
		DBG_ERR("KDRV IFE2: engine id error...\r\n");
		return -1;
		break;
	}

	//p_handle = (KDRV_IFE2_HANDLE *)hdl;
	//if (kdrv_ife2_chk_handle(p_handle) == NULL) {
	//  DBG_ERR("DAL IFE2: illegal handle 0x%.8x\r\n", hdl);
	//  return E_SYS;
	//}

	//if ((p_handle->sts & KDRV_IFE2_HANDLE_LOCK) != KDRV_IFE2_HANDLE_LOCK) {
	//  DBG_ERR("DAL IFE2: illegal handle sts 0x%.8x\r\n", p_handle->sts);
	//  return E_SYS;
	//}

#if 0
	kdrv_ife2_lock(p_handle, TRUE);
	if (kdrv_ife2_handle_free(p_handle)) {
		rt = ife2_close();
	}
	kdrv_ife2_lock(p_handle, FALSE);
#endif

	return 0;
}

#if 0
#endif


INT32 kdrv_ife2_pause(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;
	//KDRV_IFE2_HANDLE* p_handle;

	rt = ife2_pause();
	if (rt != E_OK) {
		return -1;
	}

	return 0;
}

#if 0
#endif

static ER kdrv_ife2_set_reset(UINT32 id, void *data)
{
	KDRV_IFE2_HANDLE *p_handle;

	if (ife2_reset() != E_OK) {
		DBG_ERR("KDRV IFE2: engine reset error...\r\n");
		return E_PAR;
	}

	p_handle = kdrv_ife2_entry_id_conv2_handle(id);
	kdrv_ife2_sem(p_handle, FALSE);

	return E_OK;
}


static ER kdrv_ife2_set_trig(UINT32 id, void *data)
{
	ER rt;
	KDRV_IFE2_HANDLE *p_handle;

	p_handle = kdrv_ife2_entry_id_conv2_handle(id);

	kdrv_ife2_sem(p_handle, TRUE);

	kdrv_ife2_int_set_param_init(id);

	//set ife2 dal parameters to ife2 driver
	rt = kdrv_ife2_setmode(id);
	if (rt != E_OK) {
		kdrv_ife2_sem(p_handle, FALSE);
		return rt;
	}
	//update trig id and trig_cfg
	g_kdrv_ife2_trig_hdl = p_handle;
	//trigger ife2 start
	kdrv_ife2_frm_end(p_handle, FALSE);
	ife2_clear_flag_frame_end();

	//trigger ife2 start
	rt = ife2_start();
	if (rt != E_OK) {
		kdrv_ife2_sem(p_handle, FALSE);
		return rt;
	}
	return rt;
}

static ER kdrv_ife2_set_in_img_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	g_kdrv_ife2_info[id].in_img_info = *((KDRV_IFE2_IN_INFO *)data);

	if (g_kdrv_ife2_info[id].in_img_info.ch.width > 0x3FFF) {
		DBG_ERR("KDRV IFE2: Can not set width size(%d) > 0x3FFF\r\n", (int)g_kdrv_ife2_info[id].in_img_info.ch.width);
		g_kdrv_ife2_info[id].in_img_info.ch.width = 0x3FFF;
	} else if (g_kdrv_ife2_info[id].in_img_info.ch.width < IFE2_SRCBUF_WMIN) {
		DBG_ERR("KDRV IFE2: Can not set width size(%d) < %d\r\n", (int)g_kdrv_ife2_info[id].in_img_info.ch.width, (int)IFE2_SRCBUF_WMIN);
		g_kdrv_ife2_info[id].in_img_info.ch.width = IFE2_SRCBUF_WMIN;
	}

	if (g_kdrv_ife2_info[id].in_img_info.ch.height > 0x3FFF) {
		DBG_ERR("KDRV IFE2: Can not set height size(%d) > 0x3FFF\r\n", (int)g_kdrv_ife2_info[id].in_img_info.ch.height);
		g_kdrv_ife2_info[id].in_img_info.ch.height = 0x3FFF;
	} else if (g_kdrv_ife2_info[id].in_img_info.ch.height < IFE2_SRCBUF_HMIN) {
		DBG_ERR("KDRV IFE2: Can not set height size(%d) < %d\r\n", (int)g_kdrv_ife2_info[id].in_img_info.ch.height, (int)IFE2_SRCBUF_HMIN);
		g_kdrv_ife2_info[id].in_img_info.ch.height = IFE2_SRCBUF_HMIN;
	}

	g_ife2_update_param[id][KDRV_IFE2_PARAM_IPL_IN_IMG_INFO] = TRUE;

	return E_OK;
}

static ER kdrv_ife2_set_in_addr(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = ife2_platform_spin_lock();

	if ((*(UINT32 *)data) & 0x3) {
		DBG_ERR("KDRV IFE2: input address must be the multiple of 4\r\n");
		return E_SYS;
	}

	g_kdrv_ife2_info[id].in_pixel_addr = *(UINT32 *)data;

	g_ife2_update_param[id][KDRV_IFE2_PARAM_IPL_IN_ADDR] = TRUE;

	//ife2_platform_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ife2_set_out_img_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = ife2_platform_spin_lock();

	g_kdrv_ife2_info[id].out_img_info = *((KDRV_IFE2_OUT_IMG *)data);

	g_ife2_update_param[id][KDRV_IFE2_PARAM_IPL_OUT_IMG_INFO] = TRUE;

	//ife2_platform_spin_unlock(loc_status);


	return E_OK;
}



static ER kdrv_ife2_set_out_addr(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	if ((*(UINT32 *)data) & 0x3) {
		DBG_ERR("IFE2 DAL output address must be the multiple of 4\r\n");
		return E_SYS;
	}

	//loc_status = ife2_platform_spin_lock();

	g_kdrv_ife2_info[id].out_pixel_addr = *((UINT32 *)data);

	g_ife2_update_param[id][KDRV_IFE2_PARAM_IPL_OUT_ADDR] = TRUE;

	//ife2_platform_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ife2_set_isrcb(UINT32 id, void *data)
{
	KDRV_IFE2_HANDLE *p_handle = NULL;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	p_handle = kdrv_ife2_entry_id_conv2_handle(id);

	p_handle->isrcb_fp = (KDRV_IPP_ISRCB)data;
	return E_OK;
}

static ER kdrv_ife2_set_inte_en(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = ife2_platform_spin_lock();

	g_kdrv_ife2_info[id].inte_en = *((KDRV_IFE2_INTE *)data);

	g_ife2_update_param[id][KDRV_IFE2_PARAM_IPL_INTER] = TRUE;

	//ife2_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ife2_set_gray_stat(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = ife2_platform_spin_lock();

	g_kdrv_ife2_info[id].gray_statist = *((KDRV_IFE2_GRAY_STATIST *)data);

	g_ife2_update_param[id][KDRV_IFE2_PARAM_IQ_GRAY_STATIST] = TRUE;

#if 0
	KDRV_IFE2_GRAY_STATIST *ptr = (KDRV_IFE2_GRAY_STATIST *)data;

	g_kdrv_ife2_info[id].gray_statist.u_th0 = ptr->u_th0;
	g_kdrv_ife2_info[id].gray_statist.u_th1 = ptr->u_th1;
	g_kdrv_ife2_info[id].gray_statist.v_th0 = ptr->v_th0;
	g_kdrv_ife2_info[id].gray_statist.v_th1 = ptr->v_th1;
#endif

	//ife2_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ife2_set_refcent_param(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = ife2_platform_spin_lock();

	g_kdrv_ife2_info[id].refcent = *((KDRV_IFE2_REFCENT_PARAM *)data);

	g_ife2_update_param[id][KDRV_IFE2_PARAM_IQ_REFCENT] = TRUE;

#if 0
	KDRV_IFE2_REFCENT_PARAM *ptr = (KDRV_IFE2_REFCENT_PARAM *)data;

	g_kdrv_ife2_info[id].refcent.refcent_y.cent_wt   = ptr->refcent_y.cent_wt;
	g_kdrv_ife2_info[id].refcent.refcent_y.outl_dth  = ptr->refcent_y.outl_dth;
	g_kdrv_ife2_info[id].refcent.refcent_y.outl_th   = ptr->refcent_y.outl_th;

	g_kdrv_ife2_info[id].refcent.refcent_uv.cent_wt  = ptr->refcent_uv.cent_wt;
	g_kdrv_ife2_info[id].refcent.refcent_uv.outl_dth = ptr->refcent_uv.outl_dth;
	g_kdrv_ife2_info[id].refcent.refcent_uv.outl_th  = ptr->refcent_uv.outl_th;

	if (ptr->refcent_y.rng_th != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->refcent_y.rng_th;

		g_rng_th_y[id][0] = ptr2[0];
		g_rng_th_y[id][1] = ptr2[1];
		g_rng_th_y[id][2] = ptr2[2];
	}

	if (ptr->refcent_uv.rng_th != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->refcent_uv.rng_th;

		g_rng_th_uv[id][0] = ptr2[0];
		g_rng_th_uv[id][1] = ptr2[1];
		g_rng_th_uv[id][2] = ptr2[2];
	}

	if (ptr->refcent_y.rng_wt != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->refcent_y.rng_wt;

		g_rng_wt_y[id][0] = ptr2[0];
		g_rng_wt_y[id][1] = ptr2[1];
		g_rng_wt_y[id][2] = ptr2[2];
		g_rng_wt_y[id][3] = ptr2[3];
	}

	if (ptr->refcent_uv.rng_wt != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->refcent_uv.rng_wt;

		g_rng_wt_uv[id][0] = ptr2[0];
		g_rng_wt_uv[id][1] = ptr2[1];
		g_rng_wt_uv[id][2] = ptr2[2];
		g_rng_wt_uv[id][3] = ptr2[3];
	}
#endif

	//ife2_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ife2_set_filter_param(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = ife2_platform_spin_lock();

	g_kdrv_ife2_info[id].filter = *((KDRV_IFE2_FILTER_PARAM *)data);
	g_kdrv_ife2_iq_func_en[id].ife2_ctrl_bit.bit.filter = g_kdrv_ife2_info[id].filter.enable;

	g_ife2_update_param[id][KDRV_IFE2_PARAM_IQ_FILTER] = TRUE;

#if 0
	KDRV_IFE2_FILTER_PARAM *ptr = (KDRV_IFE2_FILTER_PARAM *)data;

	g_kdrv_ife2_info[id].filter.enable          = ptr->enable;
	g_kdrv_ife2_info[id].filter.size            = ptr->size;
	g_kdrv_ife2_info[id].filter.edg_dir.hv_th   = ptr->edg_dir.hv_th;
	g_kdrv_ife2_info[id].filter.edg_dir.pn_th   = ptr->edg_dir.pn_th;
	g_kdrv_ife2_info[id].filter.edg_ker_size    = ptr->edg_ker_size;

	if (ptr->set_y.filt_th != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->set_y.filt_th;

		g_filt_th_y[id][0] = ptr2[0];
		g_filt_th_y[id][1] = ptr2[1];
		g_filt_th_y[id][2] = ptr2[2];
		g_filt_th_y[id][3] = ptr2[3];
		g_filt_th_y[id][4] = ptr2[4];
	}

	if (ptr->set_u.filt_th != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->set_u.filt_th;

		g_filt_th_u[id][0] = ptr2[0];
		g_filt_th_u[id][1] = ptr2[1];
		g_filt_th_u[id][2] = ptr2[2];
		g_filt_th_u[id][3] = ptr2[3];
		g_filt_th_u[id][4] = ptr2[4];
	}

	if (ptr->set_v.filt_th != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->set_v.filt_th;

		g_filt_th_v[id][0] = ptr2[0];
		g_filt_th_v[id][1] = ptr2[1];
		g_filt_th_v[id][2] = ptr2[2];
		g_filt_th_v[id][3] = ptr2[3];
		g_filt_th_v[id][4] = ptr2[4];
	}

	if (ptr->set_y.filt_wt != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->set_y.filt_wt;

		g_filt_wt_y[id][0] = ptr2[0];
		g_filt_wt_y[id][1] = ptr2[1];
		g_filt_wt_y[id][2] = ptr2[2];
		g_filt_wt_y[id][3] = ptr2[3];
		g_filt_wt_y[id][4] = ptr2[4];
		g_filt_wt_y[id][5] = ptr2[5];
	}

	if (ptr->set_u.filt_wt != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->set_u.filt_wt;

		g_filt_wt_u[id][0] = ptr2[0];
		g_filt_wt_u[id][1] = ptr2[1];
		g_filt_wt_u[id][2] = ptr2[2];
		g_filt_wt_u[id][3] = ptr2[3];
		g_filt_wt_u[id][4] = ptr2[4];
		g_filt_wt_u[id][5] = ptr2[5];
	}

	if (ptr->set_v.filt_wt != 0) {
		UINT32 *ptr2 = (UINT32 *)ptr->set_v.filt_wt;

		g_filt_wt_v[id][0] = ptr2[0];
		g_filt_wt_v[id][1] = ptr2[1];
		g_filt_wt_v[id][2] = ptr2[2];
		g_filt_wt_v[id][3] = ptr2[3];
		g_filt_wt_v[id][4] = ptr2[4];
		g_filt_wt_v[id][5] = ptr2[5];
	}
#endif

	//ife2_platform_spin_unlock(loc_status);

	return E_OK;
}

#if 0
static ER kdrv_ife2_set_burst_param(UINT32 id, void *data)
{
	KDRV_IFE2_BURST *ptr = (KDRV_IFE2_BURST *)data;

	g_kdrv_ife2_info[id].burst.input = ptr->input;

	return E_OK;
}
#endif

static ER kdrv_ife2_set_iq_all(UINT32 id, void *data)
{
	ER retval;

	KDRV_IFE2_PARAM_IQ_ALL_PARAM info = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: set all IQ parameter NULL...\r\n");
		return E_PAR;
	}

	info = *(KDRV_IFE2_PARAM_IQ_ALL_PARAM *)data;

	if (info.p_gray_stl != NULL) {
		retval = kdrv_ife2_set_gray_stat(id, (void *)info.p_gray_stl);
		if (retval != E_OK) {
			return retval;
		}
	}

	if (info.p_ref_cent != NULL) {
		retval = kdrv_ife2_set_refcent_param(id, (void *)info.p_ref_cent);
		if (retval != E_OK) {
			return retval;
		}
	}

	if (info.p_filter != NULL) {
		retval = kdrv_ife2_set_filter_param(id, (void *)info.p_filter);
		if (retval != E_OK) {
			return retval;
		}
	}

	return E_OK;
}

static ER kdrv_ife2_set_ipl_func_enable(UINT32 id, void *data)
{
	KDRV_IFE2_PARAM_IPL_FUNC_EN info = {0};

	KDRV_IFE2_FUNC_CTRL get_func_sel = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: set all IQ function enable parameter NULL...\r\n");
		return E_PAR;
	}

	info = *(KDRV_IFE2_PARAM_IPL_FUNC_EN *)data;

	get_func_sel.ife2_ctrl_bit.val = info.ipl_ctrl_func;

	if (get_func_sel.ife2_ctrl_bit.bit.filter == ENABLE) {
		g_kdrv_ife2_ipl_func_en[id].func_sel.ife2_ctrl_bit.bit.filter = ENABLE;
		g_kdrv_ife2_ipl_func_en[id].func_op.ife2_ctrl_bit.bit.filter = ((info.enable == TRUE) ? ENABLE : DISABLE);
	}

	return E_OK;
}


static ER kdrv_ife2_set_load(UINT32 id, void *data)
{
	ER rt = E_OK;

	UINT32 i;
	uint32_t set_load_check;
	//unsigned long loc_status;

	//loc_status = ife2_platform_spin_lock();

	set_load_check = 0;
	for (i = 0; i < KDRV_IFE2_PARAM_MAX; i++) {
		g_ife2_int_update_param[i] = g_ife2_update_param[id][i];

		set_load_check += g_ife2_update_param[id][i];

		g_ife2_update_param[id][i] = FALSE;
	}

	//ife2_platform_spin_unlock(loc_status);

	if (set_load_check == 0) {
		return E_OK;
	}

	kdrv_ife2_int_set_param_init(id);

	g_kdrv_ife2_info[id].burst.input = KDRV_IFE2_BURST_LEN_32;

	// ife2 input
	if (g_kdrv_ife2_info[id].out_img_info.out_dst == KDRV_IFE2_OUT_DST_DIRECT) {
		g_ife2_set_param.op_mode             = IFE2_OPMODE_IFE_IPP;
		g_ife2_set_param.src_fmt             = KDRV_IFE2_IN_FMT_PACK_YUV444;
	} else if (g_kdrv_ife2_info[id].out_img_info.out_dst == KDRV_IFE2_OUT_DST_ALL_DIRECT) {
		g_ife2_set_param.op_mode             = IFE2_OPMODE_SIE_IPP;
		g_ife2_set_param.src_fmt             = KDRV_IFE2_IN_FMT_PACK_YUV444;
	} else {  //D2D
		g_ife2_set_param.op_mode             = IFE2_OPMODE_D2D;
		g_ife2_set_param.src_fmt             = g_kdrv_ife2_info[id].in_img_info.type;
	}

	g_ife2_set_param.in_addr.chl_sel          = IFE2_DMAHDL_IN;
	g_ife2_set_param.in_addr.addr.addr_y    = g_kdrv_ife2_info[id].in_pixel_addr;
	g_ife2_set_param.img_size.img_size_h     = g_kdrv_ife2_info[id].in_img_info.ch.width;
	g_ife2_set_param.img_size.img_size_v     = g_kdrv_ife2_info[id].in_img_info.ch.height;
	g_ife2_set_param.in_lofs.chl_sel           = IFE2_DMAHDL_IN;
	g_ife2_set_param.in_lofs.lofs.lofs_y = g_kdrv_ife2_info[id].in_img_info.ch.line_ofs;

	// ife2 output
	g_ife2_set_param.dst_fmt                 = g_ife2_set_param.src_fmt;

	g_ife2_set_param.out_addr0.chl_sel        = IFE2_DMAHDL_OUT0;
	g_ife2_set_param.out_addr0.addr.addr_y  = g_kdrv_ife2_info[id].out_pixel_addr;
	g_ife2_set_param.out_lofs.chl_sel          = IFE2_DMAHDL_OUT0;
	g_ife2_set_param.out_lofs.lofs.lofs_y = g_kdrv_ife2_info[id].out_img_info.line_ofs;

	// ife2 interrupt
	g_ife2_set_param.interrupt_en = g_kdrv_ife2_info[id].inte_en;

	g_ife2_set_param.bst_size.in_bst_len = g_kdrv_ife2_info[id].burst.input;

	g_ife2_set_param.out_dram_en = IFE2_FUNC_DISABLE;


	if (g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_IN_IMG_INFO] | g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_IN_ADDR]) {

		rt = ife2_chg_direct_in_param(&g_ife2_set_param);
		if (rt != E_OK) {
			return rt;
		}

		//g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_IN_IMG_INFO] = FALSE;
		//g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_IN_ADDR] = FALSE;
	}

	if (g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_OUT_IMG_INFO] | g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_OUT_ADDR]) {

		rt = ife2_chg_direct_out_param(&g_ife2_set_param);
		if (rt != E_OK) {
			return rt;
		}

		//g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_OUT_IMG_INFO] = FALSE;
		//g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_OUT_ADDR] = FALSE;
	}

	if (g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_INTER]) {

		rt = ife2_chg_direct_interrupt_enable_param(&g_ife2_set_param);
		if (rt != E_OK) {
			return rt;
		}

		//g_ife2_int_update_param[KDRV_IFE2_PARAM_IPL_INTER] = FALSE;
	}

	if (g_ife2_int_update_param[KDRV_IFE2_PARAM_IQ_GRAY_STATIST]) {

		g_ife2_set_param.gray_sta.u_th0 = g_kdrv_ife2_info[id].gray_statist.u_th0;
		g_ife2_set_param.gray_sta.u_th1 = g_kdrv_ife2_info[id].gray_statist.u_th1;
		g_ife2_set_param.gray_sta.v_th0 = g_kdrv_ife2_info[id].gray_statist.v_th0;
		g_ife2_set_param.gray_sta.v_th1 = g_kdrv_ife2_info[id].gray_statist.v_th1;

		rt = ife2_chg_direct_gray_stl_param(&g_ife2_set_param);
		if (rt != E_OK) {
			return rt;
		}

		//g_ife2_int_update_param[KDRV_IFE2_PARAM_IQ_GRAY_STATIST] = FALSE;
	}

	if (g_ife2_int_update_param[KDRV_IFE2_PARAM_IQ_REFCENT]) {

		//g_ife2_set_param.ref_cent.ref_cent_y.range_th [0] = g_rng_th_y [id][0];
		//g_ife2_set_param.ref_cent.ref_cent_y.range_th [1] = g_rng_th_y [id][1];
		//g_ife2_set_param.ref_cent.ref_cent_y.range_th [2] = g_rng_th_y [id][2];

		for (i = 0; i < KDRV_IFE2_RANGE_TH_TAB; i++) {
			g_ife2_set_param.ref_cent.ref_cent_y.range_th[i] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_th[i];
		}

		//g_ife2_set_param.ref_cent.ref_cent_uv.range_th[0] = g_rng_th_uv[id][0];
		//g_ife2_set_param.ref_cent.ref_cent_uv.range_th[1] = g_rng_th_uv[id][1];
		//g_ife2_set_param.ref_cent.ref_cent_uv.range_th[2] = g_rng_th_uv[id][2];
		for (i = 0; i < KDRV_IFE2_RANGE_TH_TAB; i++) {
			g_ife2_set_param.ref_cent.ref_cent_uv.range_th[i] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_th[i];
		}

		//g_ife2_set_param.ref_cent.ref_cent_y.range_wet [0] = g_rng_wt_y [id][0];
		//g_ife2_set_param.ref_cent.ref_cent_y.range_wet [1] = g_rng_wt_y [id][1];
		//g_ife2_set_param.ref_cent.ref_cent_y.range_wet [2] = g_rng_wt_y [id][2];
		//g_ife2_set_param.ref_cent.ref_cent_y.range_wet [3] = g_rng_wt_y [id][3];
		for (i = 0; i < KDRV_IFE2_RANGE_WT_TAB; i++) {
			g_ife2_set_param.ref_cent.ref_cent_y.range_wet[i] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_wt[i];
		}

		//g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[0] = g_rng_wt_uv[id][0];
		//g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[1] = g_rng_wt_uv[id][1];
		//g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[2] = g_rng_wt_uv[id][2];
		//g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[3] = g_rng_wt_uv[id][3];
		for (i = 0; i < KDRV_IFE2_RANGE_WT_TAB; i++) {
			g_ife2_set_param.ref_cent.ref_cent_uv.range_wet[i] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_wt[i];
		}

		g_ife2_set_param.ref_cent.ref_cent_y.cnt_wet   = g_kdrv_ife2_info[id].refcent.refcent_y.cent_wt;
		g_ife2_set_param.ref_cent.ref_cent_y.outlier_dth = g_kdrv_ife2_info[id].refcent.refcent_y.outl_dth;
		g_ife2_set_param.ref_cent.ref_cent_y.outlier_th  = g_kdrv_ife2_info[id].refcent.refcent_y.outl_th;

		g_ife2_set_param.ref_cent.ref_cent_uv.cnt_wet  = g_kdrv_ife2_info[id].refcent.refcent_uv.cent_wt;
		g_ife2_set_param.ref_cent.ref_cent_uv.outlier_dth = g_kdrv_ife2_info[id].refcent.refcent_uv.outl_dth;
		g_ife2_set_param.ref_cent.ref_cent_uv.outlier_th = g_kdrv_ife2_info[id].refcent.refcent_uv.outl_th;

		rt = ife2_chg_direct_reference_center_param(&g_ife2_set_param);
		if (rt != E_OK) {
			return rt;
		}

		//g_ife2_int_update_param[KDRV_IFE2_PARAM_IQ_REFCENT] = FALSE;
	}

	if (g_ife2_int_update_param[KDRV_IFE2_PARAM_IQ_FILTER]) {

		g_ife2_set_param.filter.filter_y_en             = g_kdrv_ife2_info[id].filter.enable;
		g_ife2_set_param.filter.filter_size            = g_kdrv_ife2_info[id].filter.size;
		g_ife2_set_param.filter.filter_edth.edth_hv   = g_kdrv_ife2_info[id].filter.edg_dir.hv_th;
		g_ife2_set_param.filter.filter_edth.edth_pn   = g_kdrv_ife2_info[id].filter.edg_dir.pn_th;
		g_ife2_set_param.filter.edge_ker_size        = g_kdrv_ife2_info[id].filter.edg_ker_size;

		//g_ife2_set_param.filter.filter_set_y.filter_th[0] = g_filt_th_y[id][0];
		//g_ife2_set_param.filter.filter_set_y.filter_th[1] = g_filt_th_y[id][1];
		//g_ife2_set_param.filter.filter_set_y.filter_th[2] = g_filt_th_y[id][2];
		//g_ife2_set_param.filter.filter_set_y.filter_th[3] = g_filt_th_y[id][3];
		//g_ife2_set_param.filter.filter_set_y.filter_th[4] = g_filt_th_y[id][4];
		for (i = 0; i < KDRV_IFE2_FILTER_TH_TAB; i++) {
			g_ife2_set_param.filter.filter_set_y.filter_th[i] = g_kdrv_ife2_info[id].filter.set_y.filt_th[i];
		}

		//g_ife2_set_param.filter.filter_set_u.filter_th[0] = g_filt_th_u[id][0];
		//g_ife2_set_param.filter.filter_set_u.filter_th[1] = g_filt_th_u[id][1];
		//g_ife2_set_param.filter.filter_set_u.filter_th[2] = g_filt_th_u[id][2];
		//g_ife2_set_param.filter.filter_set_u.filter_th[3] = g_filt_th_u[id][3];
		//g_ife2_set_param.filter.filter_set_u.filter_th[4] = g_filt_th_u[id][4];
		for (i = 0; i < KDRV_IFE2_FILTER_TH_TAB; i++) {
			g_ife2_set_param.filter.filter_set_u.filter_th[i] = g_kdrv_ife2_info[id].filter.set_u.filt_th[i];
		}

		//g_ife2_set_param.filter.filter_set_v.filter_th[0] = g_filt_th_v[id][0];
		//g_ife2_set_param.filter.filter_set_v.filter_th[1] = g_filt_th_v[id][1];
		//g_ife2_set_param.filter.filter_set_v.filter_th[2] = g_filt_th_v[id][2];
		//g_ife2_set_param.filter.filter_set_v.filter_th[3] = g_filt_th_v[id][3];
		//g_ife2_set_param.filter.filter_set_v.filter_th[4] = g_filt_th_v[id][4];
		for (i = 0; i < KDRV_IFE2_FILTER_TH_TAB; i++) {
			g_ife2_set_param.filter.filter_set_v.filter_th[i] = g_kdrv_ife2_info[id].filter.set_v.filt_th[i];
		}

		//g_ife2_set_param.filter.filter_set_y.filter_wet[0] = g_filt_wt_y[id][0];
		//g_ife2_set_param.filter.filter_set_y.filter_wet[1] = g_filt_wt_y[id][1];
		//g_ife2_set_param.filter.filter_set_y.filter_wet[2] = g_filt_wt_y[id][2];
		//g_ife2_set_param.filter.filter_set_y.filter_wet[3] = g_filt_wt_y[id][3];
		//g_ife2_set_param.filter.filter_set_y.filter_wet[4] = g_filt_wt_y[id][4];
		//g_ife2_set_param.filter.filter_set_y.filter_wet[5] = g_filt_wt_y[id][5];
		for (i = 0; i < KDRV_IFE2_FILTER_WT_TAB; i++) {
			g_ife2_set_param.filter.filter_set_y.filter_wet[i] = g_kdrv_ife2_info[id].filter.set_y.filt_wt[i];
		}

		//g_ife2_set_param.filter.filter_set_u.filter_wet[0] = g_filt_wt_u[id][0];
		//g_ife2_set_param.filter.filter_set_u.filter_wet[1] = g_filt_wt_u[id][1];
		//g_ife2_set_param.filter.filter_set_u.filter_wet[2] = g_filt_wt_u[id][2];
		//g_ife2_set_param.filter.filter_set_u.filter_wet[3] = g_filt_wt_u[id][3];
		//g_ife2_set_param.filter.filter_set_u.filter_wet[4] = g_filt_wt_u[id][4];
		//g_ife2_set_param.filter.filter_set_u.filter_wet[5] = g_filt_wt_u[id][5];
		for (i = 0; i < KDRV_IFE2_FILTER_WT_TAB; i++) {
			g_ife2_set_param.filter.filter_set_u.filter_wet[i] = g_kdrv_ife2_info[id].filter.set_u.filt_wt[i];
		}

		//g_ife2_set_param.filter.filter_set_v.filter_wet[0] = g_filt_wt_v[id][0];
		//g_ife2_set_param.filter.filter_set_v.filter_wet[1] = g_filt_wt_v[id][1];
		//g_ife2_set_param.filter.filter_set_v.filter_wet[2] = g_filt_wt_v[id][2];
		//g_ife2_set_param.filter.filter_set_v.filter_wet[3] = g_filt_wt_v[id][3];
		//g_ife2_set_param.filter.filter_set_v.filter_wet[4] = g_filt_wt_v[id][4];
		//g_ife2_set_param.filter.filter_set_v.filter_wet[5] = g_filt_wt_v[id][5];
		for (i = 0; i < KDRV_IFE2_FILTER_WT_TAB; i++) {
			g_ife2_set_param.filter.filter_set_v.filter_wet[i] = g_kdrv_ife2_info[id].filter.set_v.filt_wt[i];
		}

		rt = ife2_chg_direct_filter_param(&g_ife2_set_param);
		if (rt != E_OK) {
			return rt;
		}

		//g_ife2_int_update_param[KDRV_IFE2_PARAM_IQ_FILTER] = FALSE;
	}

	return rt;
}


static ER kdrv_ife2_set_builtin_flow_disable(UINT32 id, void *data)
{
	ife2_set_builtin_flow_disable();

	return E_OK;
}



KDRV_IFE2_SET_FP kdrv_ife2_set_fp[KDRV_IFE2_PARAM_MAX] = {
	kdrv_ife2_set_clk,
	kdrv_ife2_set_in_img_info,
	kdrv_ife2_set_in_addr,
	kdrv_ife2_set_out_img_info,
	kdrv_ife2_set_out_addr,
	kdrv_ife2_set_isrcb,
	kdrv_ife2_set_inte_en,
	kdrv_ife2_set_gray_stat,
	NULL,
	kdrv_ife2_set_refcent_param,
	kdrv_ife2_set_filter_param,
	kdrv_ife2_set_iq_all,
	kdrv_ife2_set_ipl_func_enable,
	kdrv_ife2_set_load,
	kdrv_ife2_set_reset,
	kdrv_ife2_set_builtin_flow_disable,
};


INT32 kdrv_ife2_set(UINT32 id, KDRV_IFE2_PARAM_ID param_id, VOID *p_param)
{
	//ER rt = E_OK;
	KDRV_IFE2_HANDLE *p_handle = NULL;
	UINT32 ign_chk;
	UINT32 eng_id, chl_id;

	ign_chk = (KDRV_IFE2_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_IFE2_IGN_CHK);

	switch (param_id) {
	case KDRV_IFE2_PARAM_IPL_LOAD:
	case KDRV_IFE2_PARAM_IPL_RESET:
	case KDRV_IFE2_PARAM_IPL_BUILTIN_DISABLE:
		break;

	default:
		if (p_param == NULL) {
			DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
			return -1;
		}
		break;
	}

	eng_id = KDRV_DEV_ID_ENGINE(id);
	chl_id = KDRV_DEV_ID_CHANNEL(id);

	switch (eng_id) {
	case KDRV_VIDEOPROCS_IFE2_ENGINE0:
		break;

	default:
		DBG_ERR("KDRV IFE2: engine id:%d error...\r\n", (int)eng_id);
		return -1;
		break;
	}

	if (param_id == KDRV_IFE2_PARAM_IPL_BUILTIN_DISABLE) {
	    if (kdrv_ife2_set_fp[param_id] != NULL) {
    		if (kdrv_ife2_set_fp[param_id](chl_id, p_param) != E_OK) {
    			return -1;
    		}
    	}
	} else {

    	if (chl_id >= kdrv_ife2_channel_num) {
    		DBG_ERR("KDRV IFE2: channel id:%d error...\r\n", (int)chl_id);
    		return -1;
    	}

    	//p_handle = (KDRV_IFE2_HANDLE *)hdl;
    	kdrv_ife2_channel_alloc(chl_id);
    	p_handle = &g_kdrv_ife2_handle_tab[chl_id];
    	if (kdrv_ife2_chk_handle(p_handle) == NULL) {
    		DBG_ERR("KDRV IFE2: illegal channel id:%d...\r\n", (int)chl_id);
    		return -1;
    	}

    	if ((p_handle->sts & KDRV_IFE2_HANDLE_LOCK) != KDRV_IFE2_HANDLE_LOCK) {
    		DBG_ERR("KDRV IFE2: illegal channel sts 0x%08x\r\n", (unsigned int)p_handle->sts);
    		return -1;
    	}



    	if (!ign_chk) {
    		kdrv_ife2_lock(p_handle, TRUE);
    	}

    	if (kdrv_ife2_set_fp[param_id] != NULL) {
    		if (kdrv_ife2_set_fp[param_id](p_handle->entry_id, p_param) != E_OK) {
    			return -1;
    		}
    	}

    	if (!ign_chk) {
    		kdrv_ife2_lock(p_handle, FALSE);
    	}
    }

	return 0;
}


INT32 kdrv_ife2_trigger(UINT32 id, KDRV_IFE2_TRIGGER_PARAM *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	UINT32 eng_id, chl_id;

	eng_id = KDRV_DEV_ID_ENGINE(id);
	chl_id = KDRV_DEV_ID_CHANNEL(id);

	switch (eng_id) {
	case KDRV_VIDEOPROCS_IFE2_ENGINE0:
		break;

	default:
		DBG_ERR("KDRV IFE2: engine id error\r\n");
		return -1;
		break;
	}

	if (chl_id >= kdrv_ife2_channel_num) {
		DBG_ERR("KDRV IFE2: channel id error\r\n");
		return -1;
	}

	if (eng_id == KDRV_VIDEOPROCS_IFE2_ENGINE0) {
		if (kdrv_ife2_set_trig(chl_id, NULL) != E_OK) {
			DBG_ERR("KDRV IFE2: trigger error\r\n");
			return -1;
		}
	} else {
		// do nothing...
	}

	return 0;
}


#if 0
#endif

static ER kdrv_ife2_get_in_addr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	*(UINT32 *)data = g_kdrv_ife2_info[id].in_pixel_addr;
	return E_OK;
}

static ER kdrv_ife2_get_in_img_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IFE2_IN_INFO *)data = g_kdrv_ife2_info[id].in_img_info;
	return E_OK;
}

static ER kdrv_ife2_get_out_addr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	*(UINT32 *)data = g_kdrv_ife2_info[id].out_pixel_addr;
	return E_OK;
}

static ER kdrv_ife2_get_out_img_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IFE2_OUT_IMG *)data = g_kdrv_ife2_info[id].out_img_info;
	return E_OK;
}

static ER kdrv_ife2_get_inte_en(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	*(KDRV_IFE2_INTE *)data = g_kdrv_ife2_info[id].inte_en;
	return E_OK;
}

static ER kdrv_ife2_get_gray_stat(UINT32 id, void *data)
{
	KDRV_IFE2_GRAY_STATIST *ptr = NULL;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	ptr = (KDRV_IFE2_GRAY_STATIST *)data;

	//ptr->u_th0 = g_kdrv_ife2_info[id].gray_statist.u_th0;
	//ptr->u_th1 = g_kdrv_ife2_info[id].gray_statist.u_th1;
	//ptr->v_th0 = g_kdrv_ife2_info[id].gray_statist.v_th0;
	//ptr->v_th1 = g_kdrv_ife2_info[id].gray_statist.v_th1;

	*ptr = g_kdrv_ife2_info[id].gray_statist;

	return E_OK;
}

static ER kdrv_ife2_get_gray_avg(UINT32 id, void *data)
{
	KDRV_IFE2_GRAY_AVG *ptr = NULL;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	ptr = (KDRV_IFE2_GRAY_AVG *)data;

	ife2_get_gray_average(&(ptr->u_avg), &(ptr->v_avg));

	return E_OK;
}

static ER kdrv_ife2_get_refcent_param(UINT32 id, void *data)
{
	KDRV_IFE2_REFCENT_PARAM *ptr;

	//UINT32 *ptr2;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	ptr = (KDRV_IFE2_REFCENT_PARAM *)data;

	*ptr = g_kdrv_ife2_info[id].refcent;

#if 0
	ptr->refcent_y.cent_wt      = g_kdrv_ife2_info[id].refcent.refcent_y.cent_wt;
	ptr->refcent_y.outl_dth     = g_kdrv_ife2_info[id].refcent.refcent_y.outl_dth;
	ptr->refcent_y.outl_th      = g_kdrv_ife2_info[id].refcent.refcent_y.outl_th;

	ptr->refcent_uv.cent_wt     = g_kdrv_ife2_info[id].refcent.refcent_uv.cent_wt;
	ptr->refcent_uv.outl_dth    = g_kdrv_ife2_info[id].refcent.refcent_uv.outl_dth;
	ptr->refcent_uv.outl_th     = g_kdrv_ife2_info[id].refcent.refcent_uv.outl_th;

	ptr2 = (UINT32 *)ptr->refcent_y.rng_th;
	ptr2[0] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_th[0];
	ptr2[1] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_th[1];
	ptr2[2] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_th[2];

	ptr2 = (UINT32 *)ptr->refcent_uv.rng_th;
	ptr2[0] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_th[0];
	ptr2[1] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_th[1];
	ptr2[2] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_th[2];

	ptr2 = (UINT32 *)ptr->refcent_y.rng_wt;
	ptr2[0] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_wt[0];
	ptr2[1] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_wt[1];
	ptr2[2] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_wt[2];
	ptr2[3] = g_kdrv_ife2_info[id].refcent.refcent_y.rng_wt[3];

	ptr2 = (UINT32 *)ptr->refcent_uv.rng_wt;
	ptr2[0] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_wt[0];
	ptr2[1] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_wt[1];
	ptr2[2] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_wt[2];
	ptr2[3] = g_kdrv_ife2_info[id].refcent.refcent_uv.rng_wt[3];
#endif

	return E_OK;
}

static ER kdrv_ife2_get_filter_param(UINT32 id, void *data)
{
	KDRV_IFE2_FILTER_PARAM *ptr;

	//UINT32 *ptr2;

	if (data == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return E_PAR;
	}

	ptr = (KDRV_IFE2_FILTER_PARAM *)data;

	*ptr = g_kdrv_ife2_info[id].filter;

#if 0
	ptr->enable         = g_kdrv_ife2_info[id].filter.enable;
	ptr->size           = g_kdrv_ife2_info[id].filter.size;
	ptr->edg_dir.hv_th  = g_kdrv_ife2_info[id].filter.edg_dir.hv_th;
	ptr->edg_dir.pn_th  = g_kdrv_ife2_info[id].filter.edg_dir.pn_th;
	ptr->edg_ker_size   = g_kdrv_ife2_info[id].filter.edg_ker_size;

	ptr2 = (UINT32 *)ptr->set_y.filt_th;
	ptr2[0] = g_kdrv_ife2_info[id].filter.set_y.filt_th[0];
	ptr2[1] = g_kdrv_ife2_info[id].filter.set_y.filt_th[1];
	ptr2[2] = g_kdrv_ife2_info[id].filter.set_y.filt_th[2];
	ptr2[3] = g_kdrv_ife2_info[id].filter.set_y.filt_th[3];
	ptr2[4] = g_kdrv_ife2_info[id].filter.set_y.filt_th[4];


	ptr2 = (UINT32 *)ptr->set_u.filt_th;
	ptr2[0] = g_kdrv_ife2_info[id].filter.set_u.filt_th[0];
	ptr2[1] = g_kdrv_ife2_info[id].filter.set_u.filt_th[1];
	ptr2[2] = g_kdrv_ife2_info[id].filter.set_u.filt_th[2];
	ptr2[3] = g_kdrv_ife2_info[id].filter.set_u.filt_th[3];
	ptr2[4] = g_kdrv_ife2_info[id].filter.set_u.filt_th[4];


	ptr2 = (UINT32 *)ptr->set_v.filt_th;
	ptr2[0] = g_kdrv_ife2_info[id].filter.set_v.filt_th[0];
	ptr2[1] = g_kdrv_ife2_info[id].filter.set_v.filt_th[1];
	ptr2[2] = g_kdrv_ife2_info[id].filter.set_v.filt_th[2];
	ptr2[3] = g_kdrv_ife2_info[id].filter.set_v.filt_th[3];
	ptr2[4] = g_kdrv_ife2_info[id].filter.set_v.filt_th[4];


	ptr2 = (UINT32 *)ptr->set_y.filt_wt;
	ptr2[0] = g_kdrv_ife2_info[id].filter.set_y.filt_wt[0];
	ptr2[1] = g_kdrv_ife2_info[id].filter.set_y.filt_wt[1];
	ptr2[2] = g_kdrv_ife2_info[id].filter.set_y.filt_wt[2];
	ptr2[3] = g_kdrv_ife2_info[id].filter.set_y.filt_wt[3];
	ptr2[4] = g_kdrv_ife2_info[id].filter.set_y.filt_wt[4];
	ptr2[5] = g_kdrv_ife2_info[id].filter.set_y.filt_wt[5];


	ptr2 = (UINT32 *)ptr->set_u.filt_wt;
	ptr2[0] = g_kdrv_ife2_info[id].filter.set_u.filt_wt[0];
	ptr2[1] = g_kdrv_ife2_info[id].filter.set_u.filt_wt[1];
	ptr2[2] = g_kdrv_ife2_info[id].filter.set_u.filt_wt[2];
	ptr2[3] = g_kdrv_ife2_info[id].filter.set_u.filt_wt[3];
	ptr2[4] = g_kdrv_ife2_info[id].filter.set_u.filt_wt[4];
	ptr2[5] = g_kdrv_ife2_info[id].filter.set_u.filt_wt[5];


	ptr2 = (UINT32 *)ptr->set_v.filt_wt;
	ptr2[0] = g_kdrv_ife2_info[id].filter.set_v.filt_wt[0];
	ptr2[1] = g_kdrv_ife2_info[id].filter.set_v.filt_wt[1];
	ptr2[2] = g_kdrv_ife2_info[id].filter.set_v.filt_wt[2];
	ptr2[3] = g_kdrv_ife2_info[id].filter.set_v.filt_wt[3];
	ptr2[4] = g_kdrv_ife2_info[id].filter.set_v.filt_wt[4];
	ptr2[5] = g_kdrv_ife2_info[id].filter.set_v.filt_wt[5];
#endif

	return E_OK;
}

#if 0
static ER kdrv_ife2_get_burst_param(UINT32 id, void *data)
{
	KDRV_IFE2_BURST *ptr = (KDRV_IFE2_BURST *)data;

	ptr->input = g_kdrv_ife2_info[id].burst.input;

	return E_OK;
}
#endif

KDRV_IFE2_GET_FP kdrv_ife2_get_fp[KDRV_IFE2_PARAM_MAX] = {
	NULL,
	kdrv_ife2_get_in_img_info,
	kdrv_ife2_get_in_addr,
	kdrv_ife2_get_out_img_info,
	kdrv_ife2_get_out_addr,
	NULL,
	kdrv_ife2_get_inte_en,
	kdrv_ife2_get_gray_stat,
	kdrv_ife2_get_gray_avg,
	kdrv_ife2_get_refcent_param,
	kdrv_ife2_get_filter_param,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
} ;


INT32 kdrv_ife2_get(UINT32 id, KDRV_IFE2_PARAM_ID param_id, void *p_param)
{
	//ER rt = E_OK;
	KDRV_IFE2_HANDLE *p_handle;
	UINT32 ign_chk;

	UINT32 eng_id, chl_id;

	if (p_param == NULL) {
		DBG_ERR("KDRV IFE2: parameter NULL...\r\n");
		return -1;
	}

	eng_id = KDRV_DEV_ID_ENGINE(id);
	chl_id = KDRV_DEV_ID_CHANNEL(id);

	switch (eng_id) {
	case KDRV_VIDEOPROCS_IFE2_ENGINE0:
		break;

	default:
		DBG_ERR("KDRV IFE2: engine id:%d error...\r\n", (int)eng_id);
		return -1;
		break;
	}

	if (chl_id >= kdrv_ife2_channel_num) {
		DBG_ERR("KDRV IFE2: channel id:%d error..\r\n", (int)chl_id);
		return -1;
	}

	if ((g_kdrv_ife2_init_flg & (1 << chl_id)) == 0) {
		DBG_WRN("KDRV IFE2: channel-%d info. is invalid\r\n", (int)chl_id);
	}

	//p_handle = (KDRV_IFE2_HANDLE *)hdl;
	p_handle = &g_kdrv_ife2_handle_tab[chl_id];
	if (kdrv_ife2_chk_handle(p_handle) == NULL) {
		DBG_ERR("KDRV IFE2: illegal channel 0x%d\r\n", (int)chl_id);
		return -1;
	}

	if ((p_handle->sts & KDRV_IFE2_HANDLE_LOCK) != KDRV_IFE2_HANDLE_LOCK) {
		DBG_ERR("KDRV IFE2: illegal handle sts 0x%08x\r\n", (unsigned int)p_handle->sts);
		return -1;
	}

	ign_chk = (KDRV_IFE2_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_IFE2_IGN_CHK);

	if (!ign_chk) {
		kdrv_ife2_lock(p_handle, TRUE);
	}
	if (kdrv_ife2_get_fp[param_id] != NULL) {
		if (kdrv_ife2_get_fp[param_id](p_handle->entry_id, p_param) != E_OK) {
			return -1;
		}
	}
	if (!ign_chk) {
		kdrv_ife2_lock(p_handle, FALSE);
	}

	return 0;
}


#if 0
#endif

static CHAR *kdrv_ife2_dump_str_in_fmt(KDRV_IFE2_IN_FMT in_fmt)
{
	CHAR *str_in_fmt[2 + 1] = {
		"PACK_YUV_444",
		"Y",
		"ERROR",
	};

	if (in_fmt >= 2) {
		return str_in_fmt[2];
	}
	return str_in_fmt[in_fmt];
}

static CHAR *kdrv_ife2_dump_str_out_dst(KDRV_IFE2_OUT_DST out_dst)
{
	CHAR *str_out_dst[2 + 1] = {
		"DIRECT",
		"DRAM",
		"ERROR",
	};

	if (out_dst >= 2) {
		return str_out_dst[2];
	}
	return str_out_dst[out_dst];
}

UINT32 kdrv_ife2_lock_chls = 0;
UINT32 kdrv_ife2_buf_query(UINT32 channel_num)
{
#if 1
	UINT32 buffer_size = 0, i = 0;

	if (channel_num == 0x0) {
		DBG_ERR("KDRV IFE2: user desired channel number is zero\r\n");
	}

	kdrv_ife2_channel_num = channel_num;

	if (kdrv_ife2_channel_num > KDRV_IPP_MAX_CHANNEL_NUM) {
		kdrv_ife2_channel_num = KDRV_IPP_MAX_CHANNEL_NUM;

		DBG_ERR("KDRV IFE2: user desired channel number is overflow, max = 28\r\n");
	}

	kdrv_ife2_lock_chls = 0;
	for (i = 0; i < kdrv_ife2_channel_num; i++) {
		kdrv_ife2_lock_chls += (1 << i);
	}

	buffer_size  += IFE2_ALIGN_CEIL_64(kdrv_ife2_channel_num * sizeof(KDRV_IFE2_PRAM));
	buffer_size  += IFE2_ALIGN_CEIL_64(kdrv_ife2_channel_num * sizeof(KDRV_IFE2_HANDLE));
	buffer_size  += IFE2_ALIGN_CEIL_64(kdrv_ife2_channel_num * sizeof(KDRV_IFE2_FUNC_CTRL));
	buffer_size  += IFE2_ALIGN_CEIL_64(kdrv_ife2_channel_num * sizeof(KDRV_IFE2_FUNC_INT_CTRL));
	buffer_size  += IFE2_ALIGN_CEIL_64(kdrv_ife2_channel_num * sizeof(KDRV_IFE2_OPENCFG));


	return buffer_size;
#else
	kdrv_ife2_channel_num = channel_num;

	return 0;
#endif
}

UINT32 kdrv_ife2_buf_init(UINT32 input_addr, UINT32 channel_num)
{
#if 1
	UINT32 buffer_size = 0;
	UINT32 get_chl_num = 0;

	get_chl_num = channel_num;
	if (get_chl_num != kdrv_ife2_channel_num) {
		DBG_ERR("KDRV IFE2: initial channel number %d != user queried channel number %d\r\n", (int)channel_num, (int)kdrv_ife2_channel_num);

		get_chl_num = kdrv_ife2_channel_num;
	}

	kdrv_ife2_flow_init();

	g_kdrv_ife2_info = (KDRV_IFE2_PRAM *)(input_addr + buffer_size);
	buffer_size += IFE2_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IFE2_PRAM));

	g_kdrv_ife2_handle_tab = (KDRV_IFE2_HANDLE *)(input_addr + buffer_size);
	buffer_size += IFE2_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IFE2_HANDLE));

	g_kdrv_ife2_iq_func_en = (KDRV_IFE2_FUNC_CTRL *)(input_addr + buffer_size);
	buffer_size += IFE2_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IFE2_FUNC_CTRL));

	g_kdrv_ife2_ipl_func_en = (KDRV_IFE2_FUNC_INT_CTRL *)(input_addr + buffer_size);
	buffer_size += IFE2_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IFE2_FUNC_INT_CTRL));

	g_kdrv_ife2_clk_info = (KDRV_IFE2_OPENCFG *)(input_addr + buffer_size);
	buffer_size += IFE2_ALIGN_CEIL_64(get_chl_num * sizeof(KDRV_IFE2_OPENCFG));

	return buffer_size;
#else
	kdrv_ife2_flow_init();

	return 0;
#endif
}

VOID kdrv_ife2_buf_uninit(VOID)
{
#if 1
	g_kdrv_ife2_info = NULL;
	g_kdrv_ife2_handle_tab = NULL;
	g_kdrv_ife2_iq_func_en = NULL;
	g_kdrv_ife2_ipl_func_en = NULL;
	g_kdrv_ife2_clk_info = NULL;

	kdrv_ife2_channel_num = 0;
	kdrv_ife2_lock_chls = 0;

	if (g_kdrv_ife2_open_cnt[KDRV_IFE2_ID_0] == 0) {
		g_kdrv_ife2_init_flg = 0x0;
		g_kdrv_ife2_set_clk_flg[KDRV_IFE2_ID_0] = FALSE;
	}
#endif
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
#include <comm/nvtmem.h>


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

struct nvt_fmem_mem_info_t kdrv_ife2_get_chl_buf = {0};
void *hdl_ife2_chl_buf = NULL;

#elif defined (__FREERTOS)

#include <malloc.h>
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"

extern void *pvPortMalloc(size_t xWantedSize);
extern void vPortFree(void *pv);

UINT32 *kdrv_ife2_get_chl_buf = NULL;

#endif

#endif
*/

VOID kdrv_ife2_init(VOID)
{
#if defined (__FREERTOS)
	ife2_platform_create_resource();
	ife2_platform_set_clk_rate(280);
#else
    ife2_platform_set_clk_rate(280);
#endif

	kdrv_ife2_install_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
	{
		UINT32 kdrv_ife2_buf_size = 0, get_addr = 0;

		kdrv_ife2_buf_size = kdrv_ife2_buf_query(KDRV_IPP_CBUF_MAX_NUM);

		DBG_ERR("kdrv_ife2_buf_size: %08x\r\n", kdrv_ife2_buf_size);

#if defined (__LINUX) || defined (__KERNEL__)
		{
			INT32  ret = 0;
			ret = nvt_fmem_mem_info_init(&kdrv_ife2_get_chl_buf, NVT_FMEM_ALLOC_CACHE, kdrv_ife2_buf_size, NULL);
			if (ret >= 0) {
				hdl_ife2_chl_buf = nvtmem_alloc_buffer(&kdrv_ife2_get_chl_buf);
				get_addr = (UINT32)kdrv_ife2_get_chl_buf.vaddr;
			}
		}
#elif defined (__FREERTOS)
		{
			if (kdrv_ife2_get_chl_buf != NULL) {
				vPortFree((void *)kdrv_ife2_get_chl_buf);
				kdrv_ife2_get_chl_buf = NULL;
			}
			kdrv_ife2_get_chl_buf = (UINT32 *)pvPortMalloc((size_t)IFE2_ALIGN_CEIL_8(kdrv_ife2_buf_size));

			if (kdrv_ife2_get_chl_buf != NULL) {
				get_addr = (UINT32)kdrv_ife2_get_chl_buf;
			}
		}
#else
		get_addr = 0;
#endif

		DBG_ERR("get_addr: %08x\r\n", get_addr);

		kdrv_ife2_buf_init(get_addr, KDRV_IPP_CBUF_MAX_NUM);
	}
#endif


}

VOID kdrv_ife2_uninit(VOID)
{
#if defined (__FREERTOS)
	ife2_platform_release_resource();
#endif

	kdrv_ife2_uninstall_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)

#if defined (__LINUX) || defined (__KERNEL__)
	if (hdl_ife2_chl_buf != NULL) {
		INT32  ret = 0;

		ret = nvtmem_release_buffer(hdl_ife2_chl_buf);
		hdl_ife2_chl_buf = NULL;
	}
#elif  defined (__FREERTOS)
	if (kdrv_ife2_get_chl_buf != NULL) {
		vPortFree((void *)kdrv_ife2_get_chl_buf);
		kdrv_ife2_get_chl_buf = NULL;
	}
#endif

	kdrv_ife2_buf_uninit();

#endif
}

BOOL is_ife2_open[KDRV_IPP_MAX_CHANNEL_NUM] = {FALSE};
#if defined __UITRON || defined __ECOS
void kdrv_ife2_dump_info(void (*dump)(char *fmt, ...))
#else
void kdrv_ife2_dump_info(int (*dump)(const char *fmt, ...))
#endif
{
	BOOL is_print_header;
	//BOOL is_open[KDRV_IFE2_HANDLE_MAX_NUM] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
	UINT32 id;

	if (g_kdrv_ife2_init_flg == 0) {
		dump("\r\n[IFE2] not open\r\n");
		return;
	}

	for (id = 0; id < kdrv_ife2_channel_num; id++) {
		if (g_kdrv_ife2_init_flg & (1 << id)) {
			is_ife2_open[id] = TRUE;
			kdrv_ife2_lock(kdrv_ife2_entry_id_conv2_handle(id), TRUE);
		}
	}

	dump("\r\n[DAL IFE2]\r\n");
	dump("\r\n-----ctrl info-----\r\n");
	dump("%12s%12s\r\n", "init_flg", "trig_id");
	dump("%#12x 0x%.8x\r\n", g_kdrv_ife2_init_flg, g_kdrv_ife2_trig_hdl);

	/**
	    input info
	*/
	dump("\r\n-----input info-----\r\n");
	dump("%3s%12s%14s%12s%12s%12s\r\n", "id", "Addr", "type", "width", "height", "line_ofs");
	for (id = 0; id < kdrv_ife2_channel_num; id++) {
		if (!is_ife2_open[id]) {
			continue;
		}
		dump("%3d%#12x%14s%12d%12d%12d\r\n",
			 id,
			 g_kdrv_ife2_info[id].in_pixel_addr,
			 kdrv_ife2_dump_str_in_fmt(g_kdrv_ife2_info[id].in_img_info.type),
			 g_kdrv_ife2_info[id].in_img_info.ch.width,
			 g_kdrv_ife2_info[id].in_img_info.ch.height,
			 g_kdrv_ife2_info[id].in_img_info.ch.line_ofs);
	}

	/**
	    ouput info
	*/
	dump("\r\n-----output info-----\r\n");
	dump("%3s%12s%14s%12s\r\n", "id", "Addr", "out_dst", "line_ofs");
	for (id = 0; id < kdrv_ife2_channel_num; id++) {
		if (!is_ife2_open[id]) {
			continue;
		}
		dump("%3d%#12x%14s%12d\r\n",
			 id,
			 g_kdrv_ife2_info[id].out_pixel_addr,
			 kdrv_ife2_dump_str_out_dst(g_kdrv_ife2_info[id].out_img_info.out_dst),
			 g_kdrv_ife2_info[id].out_img_info.line_ofs);
	}

	/**
	    interrupt info
	*/
	dump("\r\n-----interrupt info-----\r\n");
	dump("%3s%12s\r\n", "id", "inte_en:");
	for (id = 0; id < kdrv_ife2_channel_num; id++) {
		if (!is_ife2_open[id]) {
			continue;
		}
		dump("%3d", id);

		if (g_kdrv_ife2_info[id].inte_en & KDRV_IFE2_INT_OVFL) {
			dump("%12s\r\n", "OVFL");
		} else if (g_kdrv_ife2_info[id].inte_en & KDRV_IFE2_INT_FMD) {
			dump("%12s\r\n", "FMD");
		} else {
			dump("%12s\r\n", "CLR");
		}
	}

	/**
	    IQ function enable info
	*/
	/*dump("\r\n-----IQ function enable info-----\r\n");
	dump("%3s\r\n", "id");
	for (id = 0; id < kdrv_ife2_channel_num; id++) {
	    if (!is_open[id]) {
	        continue;
	    }
	    dump("%3d\r\n",
	        id);
	}*/

	/**
	    IQ parameter info
	*/
	dump("\r\n-----IQ parameter info-----\r\n");

	is_print_header = FALSE;
	for (id = 0; id < kdrv_ife2_channel_num; id++) {
		if ((!is_ife2_open[id]) || (!g_kdrv_ife2_info[id].filter.enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%15s%3s%8s\r\n", "FILT_Y_CH", "id", "enable");
			is_print_header = TRUE;
		}
		dump("%15s%3d%8d\r\n"
			 , "--", id, g_kdrv_ife2_info[id].filter.enable);
	}


	for (id = 0; id < kdrv_ife2_channel_num; id++) {
		if (is_ife2_open[id]) {
			kdrv_ife2_lock(kdrv_ife2_entry_id_conv2_handle(id), FALSE);
		}
	}

}


#if defined (__KERNEL__)

EXPORT_SYMBOL(kdrv_ife2_open);
EXPORT_SYMBOL(kdrv_ife2_close);
EXPORT_SYMBOL(kdrv_ife2_pause);
EXPORT_SYMBOL(kdrv_ife2_set);
EXPORT_SYMBOL(kdrv_ife2_trigger);
EXPORT_SYMBOL(kdrv_ife2_get);
EXPORT_SYMBOL(kdrv_ife2_dump_info);

EXPORT_SYMBOL(kdrv_ife2_init);
EXPORT_SYMBOL(kdrv_ife2_uninit);
EXPORT_SYMBOL(kdrv_ife2_buf_query);
EXPORT_SYMBOL(kdrv_ife2_buf_init);
EXPORT_SYMBOL(kdrv_ife2_buf_uninit);

#endif

