/*
    ctl sie iosize info.

    ctl sie iosize table.

    @file       ctl_sie_iosize.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "ctl_sie_iosize_int.h"

/*
    These parameters may be based on IC variability, ctl sie calculation or externally set parameters.

    *g_dest_ext_crp_prop: (destination extra crop proportion)
    if IC support RSC/DIS/..., parameters will set by user (CTL_SIE_ITEM_IO_SIZE)
    else, g_dest_ext_crp_prop.w = g_dest_ext_crp_prop.h = 0
    calculation:
    w = w * (100 - dest_ext_crp_prop.w) / 100
    h = h * (100 - dest_ext_crp_prop.h) / 100

    *g_sie_scl_max_sz:
    if IC support SIE scale, parameters must be set by user (CTL_SIE_ITEM_IO_SIZE)
    else, g_sie_scl_max_sz = CTL_SIE_DFT (for default, CTL_SIE_DEST_MAX_SZ_W / CTL_SIE_DEST_MAX_SZ_H)
    calculation:
    if (sie_crp_sz > g_sie_scl_max_sz), sie_scl_sz = dest_max_crp

    *g_dest_align:
    if IC support SIE scale, parameters must be set by user (CTL_SIE_ITEM_IO_SIZE)
    else, g_dest_align = CTL_SIE_ITEM_IO_SIZE / align

    *g_sie_scl_sz:
    if IC support SIE scale and CTL_SIE_IOSIZE_MANUAL, parameters must be set by user (need to remove g_sie_scl_sz = g_sie_crp_win)
    else, g_sie_scl_sz = g_sie_crp_win
*/
static USIZE g_dest_ext_crp_prop[CTL_SIE_MAX_SUPPORT_ID];
static USIZE g_sie_scl_max_sz[CTL_SIE_MAX_SUPPORT_ID];
static USIZE g_dest_align[CTL_SIE_MAX_SUPPORT_ID];
static USIZE g_sie_scl_sz[CTL_SIE_MAX_SUPPORT_ID];

/*
    internal used
*/
static USIZE g_re_crp_sz[CTL_SIE_MAX_SUPPORT_ID];  ///< the remaining crop size
static URECT g_sie_crp_win[CTL_SIE_MAX_SUPPORT_ID];
static IPOINT g_re_sft_sz[CTL_SIE_MAX_SUPPORT_ID];  ///< the remaining shift size

static void __ctl_sie_conv_ratio(CTL_SIE_ID id, UINT32 *ratio, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param)
{
	if (*ratio == CTL_SIE_DFT || *ratio == 0) {
		if (*ratio == 0) {
			ctl_sie_dbg_wrn("N.S. align to 0\r\n");
		}

		if (sen_mode_param == NULL) {
			ctl_sie_dbg_err("sen_mode_param NULL\r\n");
			*ratio = CTL_SIE_RATIO(CTL_SIE_DFT_WIDTH, CTL_SIE_DFT_HEIGHT);
		} else {
			*ratio = sen_mode_param->ratio_h_v;
		}
	}
}

static void __ctl_sie_conv_align(CTL_SIE_ID id, UINT32 *align, UINT32 dft_val)
{
	if (*align == CTL_SIE_DFT) {
		*align = dft_val;
	}

	if (*align > CTL_SIE_ALIGN_MAX) {
		ctl_sie_dbg_wrn("align value %d over than max %d\r\n", (int)(*align), (int)(CTL_SIE_ALIGN_MAX));
		*align = dft_val;
	}

	if (*align == 0) {
		ctl_sie_dbg_wrn("N.S. align to 0\r\n");
		*align = dft_val;
	}
}

static URECT __sie_crp_win_auto(CTL_SIE_ID id, CTL_SIE_HDL *hdl, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param)
{
	URECT rst_win = {0, 0, CTL_SIE_DFT_WIDTH, CTL_SIE_DFT_HEIGHT};
	USIZE dest_sz = {0};
	IPOINT chk_sft;
	UINT32 sen_rt_h, sen_rt_v, in_rt_h, in_rt_v;

	if ((hdl->rtc_ctrl.sie_act_win.w >= sen_mode_param->crp_size.w) && (hdl->rtc_ctrl.sie_act_win.h >= sen_mode_param->crp_size.h)) {
		rst_win.w = sen_mode_param->crp_size.w;
		rst_win.h = sen_mode_param->crp_size.h;
	} else {
		ctl_sie_dbg_err("id %d, sensor act size (%d, %d) < crp size (%d, %d), set crp size to dft (%d, %d)\r\n", (int)(id), (int)(hdl->rtc_ctrl.sie_act_win.w), (int)(hdl->rtc_ctrl.sie_act_win.h), (int)(sen_mode_param->crp_size.w), (int)(sen_mode_param->crp_size.h), (int)rst_win.w, (int)rst_win.h);
		return rst_win;
	}

	/* check factor range */
	if (hdl->rtc_param.io_size_info.auto_info.factor > 1000) {
		ctl_sie_dbg_err("factor %d ovf, max should be 1000, set to max\r\n", (int)(hdl->rtc_param.io_size_info.auto_info.factor));
		hdl->rtc_param.io_size_info.auto_info.factor = 1000;
	}

	/* convert ratio */
	__ctl_sie_conv_ratio(id, &hdl->rtc_param.io_size_info.auto_info.ratio_h_v, sen_mode_param);
	/* ratio */
	in_rt_h = (hdl->rtc_param.io_size_info.auto_info.ratio_h_v) >> 16;
	in_rt_v = (hdl->rtc_param.io_size_info.auto_info.ratio_h_v) & 0xffff;
	sen_rt_h = (sen_mode_param->ratio_h_v) >> 16;
	sen_rt_v = (sen_mode_param->ratio_h_v) & 0xffff;
	if ((in_rt_h * sen_rt_v) < (in_rt_v * sen_rt_h)) {
		// crop horizotal fit input ratio
		rst_win.w = CTL_SIE_DIV_U64((UINT64)rst_win.w * in_rt_h, in_rt_v);
		rst_win.w = CTL_SIE_DIV_U64((UINT64)rst_win.w * sen_rt_v, sen_rt_h);
	} else if ((in_rt_h * sen_rt_v) > (in_rt_v * sen_rt_h)) {
		// crop vertical fit input ratio
		rst_win.h = CTL_SIE_DIV_U64((UINT64)rst_win.h * in_rt_v, in_rt_h);
		rst_win.h = CTL_SIE_DIV_U64((UINT64)rst_win.h * sen_rt_h, sen_rt_v);
	} else { // sensor and input same ratio
		// do nothing
	}

	/* calc w & h according to factor */
	if (hdl->rtc_param.io_size_info.auto_info.crp_sel == CTL_SIE_CRP_OFF) {
		dest_sz.w = CTL_SIE_DIV_U64((UINT64)rst_win.w * hdl->rtc_param.io_size_info.auto_info.factor, 1000);
		dest_sz.h = CTL_SIE_DIV_U64((UINT64)rst_win.h * hdl->rtc_param.io_size_info.auto_info.factor, 1000);
	} else {// if (hdl->rtc_param.io_size_info.auto_info.crp_sel == CTL_SIE_CRP_ON) {
		rst_win.w = CTL_SIE_DIV_U64((UINT64)rst_win.w * hdl->rtc_param.io_size_info.auto_info.factor, 1000);
		rst_win.h = CTL_SIE_DIV_U64((UINT64)rst_win.h * hdl->rtc_param.io_size_info.auto_info.factor, 1000);
	}

	/* align w & h */
	rst_win.w = ALIGN_FLOOR(rst_win.w, hdl->rtc_param.io_size_info.align.w);
	rst_win.h = ALIGN_FLOOR(rst_win.h, hdl->rtc_param.io_size_info.align.h);

	if (hdl->rtc_param.io_size_info.auto_info.crp_sel == CTL_SIE_CRP_OFF) {
		/* set remaining crop size for dest crop */
		if (rst_win.w < dest_sz.w) {
			g_re_crp_sz[id].w = 0;
		} else {
			g_re_crp_sz[id].w = rst_win.w - dest_sz.w;
		}
		if (rst_win.h < dest_sz.h) {
			g_re_crp_sz[id].h = 0;
		} else {
			g_re_crp_sz[id].h = rst_win.h - dest_sz.h;
		}
	} else {// if (hdl->param.io_size_info.auto_info.crp_sel == CTL_SIE_CRP_ON) {
		/* set remaining crop size for dest crop */
		if (rst_win.w < hdl->rtc_param.io_size_info.auto_info.sie_crp_min.w) {
			if (hdl->rtc_param.io_size_info.auto_info.sie_crp_min.w > sen_mode_param->crp_size.w) {
				ctl_sie_dbg_wrn("min w ovf\r\n");
				hdl->rtc_param.io_size_info.auto_info.sie_crp_min.w = sen_mode_param->crp_size.w;
			}
			g_re_crp_sz[id].w = hdl->rtc_param.io_size_info.auto_info.sie_crp_min.w - rst_win.w;
			rst_win.w = hdl->rtc_param.io_size_info.auto_info.sie_crp_min.w;
		} else {
			g_re_crp_sz[id].w = 0;
		}
		if (rst_win.h < hdl->rtc_param.io_size_info.auto_info.sie_crp_min.h) {
			if (hdl->rtc_param.io_size_info.auto_info.sie_crp_min.h > sen_mode_param->crp_size.h) {
				ctl_sie_dbg_wrn("min h ovf\r\n");
				hdl->rtc_param.io_size_info.auto_info.sie_crp_min.h = sen_mode_param->crp_size.h;
			}
			g_re_crp_sz[id].h = hdl->rtc_param.io_size_info.auto_info.sie_crp_min.h - rst_win.h;
			rst_win.h = hdl->rtc_param.io_size_info.auto_info.sie_crp_min.h;
		} else {
			g_re_crp_sz[id].h = 0;
		}
	}

	/* check sensor output and calc x & y, according to sft*/
	g_re_sft_sz[id].x = 0;
	g_re_sft_sz[id].y = 0;
	chk_sft.x = hdl->rtc_param.io_size_info.auto_info.sft.x + ((hdl->rtc_ctrl.sie_act_win.w - rst_win.w) >> 1);
	chk_sft.y = hdl->rtc_param.io_size_info.auto_info.sft.y + ((hdl->rtc_ctrl.sie_act_win.h - rst_win.h) >> 1);
	if (chk_sft.x < 0) {
		ctl_sie_dbg_err("sft x underflow\r\n");
		g_re_sft_sz[id].x = chk_sft.x;
		chk_sft.x = 0;
	}
	if (chk_sft.y < 0) {
		ctl_sie_dbg_err("sft y underflow\r\n");
		g_re_sft_sz[id].y = chk_sft.y;
		chk_sft.y = 0;
	}
	if (chk_sft.x + rst_win.w > hdl->rtc_ctrl.sie_act_win.w) {
		ctl_sie_dbg_err("sft x ovf, set to max w\r\n");
		g_re_sft_sz[id].x = (chk_sft.x + rst_win.w) - hdl->rtc_ctrl.sie_act_win.w;
		chk_sft.x = hdl->rtc_ctrl.sie_act_win.w - rst_win.w;
	}
	if (chk_sft.y + rst_win.h > hdl->rtc_ctrl.sie_act_win.h) {
		ctl_sie_dbg_err("sft y ovf, set to max h\r\n");
		g_re_sft_sz[id].y = (chk_sft.y + rst_win.h) - hdl->rtc_ctrl.sie_act_win.h;
		chk_sft.y = hdl->rtc_ctrl.sie_act_win.h - rst_win.h;
	}
	rst_win.x = ALIGN_FLOOR((UINT32)chk_sft.x, CTL_SIE_MANUAL_CROP_START_ALIGN);
	rst_win.y = ALIGN_FLOOR((UINT32)chk_sft.y, CTL_SIE_MANUAL_CROP_START_ALIGN);

	/* update for get parameters */
	hdl->rtc_param.io_size_info.auto_info.sft.x = rst_win.x - ((hdl->rtc_ctrl.sie_act_win.w - sen_mode_param->crp_size.w) >> 1);
	hdl->rtc_param.io_size_info.auto_info.sft.y = rst_win.y - ((hdl->rtc_ctrl.sie_act_win.h - sen_mode_param->crp_size.h) >> 1);
	return rst_win;
}

static URECT __sie_crp_win_manual(CTL_SIE_ID id, CTL_SIE_HDL *hdl)
{
	INT32 rt = CTL_SIE_E_OK;
	URECT rst_win = {0};
	CTL_SIE_IO_SIZE_INFO *io_size_info = &(hdl->rtc_param.io_size_info);
	URECT *sie_act_win = &(hdl->rtc_ctrl.sie_act_win);
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);

	//crop start -1 means to force crop center
	if (io_size_info->size_info.sie_crp.x == 0xffffffff) {
		io_size_info->size_info.sie_crp.x = sie_act_win->w;
	}

	if (io_size_info->size_info.sie_crp.y == 0xffffffff) {
		io_size_info->size_info.sie_crp.y = sie_act_win->h;
	}

	//bayer scale crop start need align to even
	if ((sie_limit->support_func & KDRV_SIE_FUNC_SPT_BS_H) || (sie_limit->support_func & KDRV_SIE_FUNC_SPT_BS_V)) {
		io_size_info->size_info.sie_crp.x = ALIGN_FLOOR(io_size_info->size_info.sie_crp.x, CTL_SIE_MANUAL_CROP_START_ALIGN);
		io_size_info->size_info.sie_crp.y = ALIGN_FLOOR(io_size_info->size_info.sie_crp.y, CTL_SIE_MANUAL_CROP_START_ALIGN);
	}

	/* check boundary */
	if (io_size_info->size_info.sie_crp.w > sie_act_win->w) {
		ctl_sie_dbg_wrn("act_win_w: %d,crp_w %d\r\n", (int)(sie_act_win->w), (int)(io_size_info->size_info.sie_crp.w));
		io_size_info->size_info.sie_crp.x = 0;
		io_size_info->size_info.sie_crp.w = sie_act_win->w;
		rt = CTL_SIE_E_PAR;
	} else if ((io_size_info->size_info.sie_crp.x + io_size_info->size_info.sie_crp.w) > sie_act_win->w) {
//		ctl_sie_dbg_wrn("act_win_w: %d,crp_x %d,crp_w %d\r\n", (int)(sie_act_win->w), (int)(io_size_info->size_info.sie_crp.x), (int)(io_size_info->size_info.sie_crp.w));
		io_size_info->size_info.sie_crp.x = ALIGN_FLOOR((sie_act_win->w - io_size_info->size_info.sie_crp.w)/2, CTL_SIE_MANUAL_CROP_START_ALIGN);
//		rt = CTL_SIE_E_PAR;
	}

	if (io_size_info->size_info.sie_crp.h > sie_act_win->h) {
		ctl_sie_dbg_wrn("act_win_h: %d,crp_h %d\r\n", (int)(sie_act_win->h), (int)(io_size_info->size_info.sie_crp.h));
		io_size_info->size_info.sie_crp.y = 0;
		io_size_info->size_info.sie_crp.h = sie_act_win->h;
		rt = CTL_SIE_E_PAR;
	} else if ((io_size_info->size_info.sie_crp.y + io_size_info->size_info.sie_crp.h) > sie_act_win->h) {
//		ctl_sie_dbg_wrn("act_win_h: %d,crp_y %d,crp_h %d\r\n", (int)(sie_act_win->h), (int)(io_size_info->size_info.sie_crp.y), (int)(io_size_info->size_info.sie_crp.h));
		io_size_info->size_info.sie_crp.y = ALIGN_FLOOR((sie_act_win->h - io_size_info->size_info.sie_crp.h)/2, CTL_SIE_MANUAL_CROP_START_ALIGN);
//		rt = CTL_SIE_E_PAR;
	}

	/* align */
	io_size_info->size_info.sie_crp.w = ALIGN_FLOOR(io_size_info->size_info.sie_crp.w, io_size_info->align.w);
	io_size_info->size_info.sie_crp.h = ALIGN_FLOOR(io_size_info->size_info.sie_crp.h, io_size_info->align.h);

	if (rt != CTL_SIE_E_OK) {
		ctl_sie_dbg_wrn("force set to  (sie_crp:x%dy%dw%d,h%d) (sen:x%dy%dw%d,h%d)\r\n"
				, io_size_info->size_info.sie_crp.x, io_size_info->size_info.sie_crp.y, io_size_info->size_info.sie_crp.w, io_size_info->size_info.sie_crp.h
				, sie_act_win->x, sie_act_win->y, sie_act_win->w, sie_act_win->h);
	}

	rst_win = io_size_info->size_info.sie_crp;

	return rst_win;
}

static URECT __sie_crp_win_ccir_auto(CTL_SIE_ID id, CTL_SIE_HDL *hdl, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param)
{
	URECT rst_win = {0, 0, sen_mode_param->crp_size.w, sen_mode_param->crp_size.h};
	IPOINT chk_sft;
	UINT32 sen_rt_h, sen_rt_v, in_rt_h, in_rt_v;
	CTL_SEN_GET_DVI_PARAM sen_dvi_param = {0};

	sen_dvi_param.mode = sen_mode_param->mode;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_DVI, (void *)&sen_dvi_param) == CTL_SIE_E_OK) {
		if (sen_dvi_param.data_mode == CTL_SEN_DVI_DATA_MODE_SD) {
			// sie crp size == sensor driver crp size
		} else if (sen_dvi_param.data_mode == CTL_SEN_DVI_DATA_MODE_HD) {
			rst_win.w = rst_win.w * 2;
		} else {
			ctl_sie_dbg_err("get sen dvi data_mode err %d\r\n", (int)(sen_dvi_param.data_mode));
		}
	} else {
		ctl_sie_dbg_err("get sen dvi info err\r\n");
	}


	/* check factor range */
	if (hdl->rtc_param.io_size_info.auto_info.factor > 1000 || hdl->rtc_param.io_size_info.auto_info.factor == 0) {
		ctl_sie_dbg_err("factor %d ovf, max should be 1000\r\n", (int)(hdl->rtc_param.io_size_info.auto_info.factor));
		hdl->rtc_param.io_size_info.auto_info.factor = 1000;
	}

	/* convert ratio */
	__ctl_sie_conv_ratio(id, &hdl->rtc_param.io_size_info.auto_info.ratio_h_v, sen_mode_param);
	/* ratio */
	in_rt_h = (hdl->rtc_param.io_size_info.auto_info.ratio_h_v) >> 16;
	in_rt_v = (hdl->rtc_param.io_size_info.auto_info.ratio_h_v) & 0xffff;
	sen_rt_h = (sen_mode_param->ratio_h_v) >> 16;
	sen_rt_v = (sen_mode_param->ratio_h_v) & 0xffff;
	if ((in_rt_h * sen_rt_v) < (in_rt_v * sen_rt_h)) {
		// crop horizotal fit input ratio
		rst_win.w = CTL_SIE_DIV_U64((UINT64)rst_win.w * in_rt_h, in_rt_v);
		rst_win.w = CTL_SIE_DIV_U64((UINT64)rst_win.w * sen_rt_v, sen_rt_h);
	} else if ((in_rt_h * sen_rt_v) > (in_rt_v * sen_rt_h)) {
		// crop vertical fit input ratio
		rst_win.h = CTL_SIE_DIV_U64((UINT64)rst_win.h * in_rt_v, in_rt_h);
		rst_win.h = CTL_SIE_DIV_U64((UINT64)rst_win.h * sen_rt_h, sen_rt_v);
	} else { // sensor and input same ratio
		// do nothing
	}

	rst_win.w = CTL_SIE_DIV_U64((UINT64)rst_win.w * hdl->rtc_param.io_size_info.auto_info.factor, 1000);
	rst_win.h = CTL_SIE_DIV_U64((UINT64)rst_win.h * hdl->rtc_param.io_size_info.auto_info.factor, 1000);

	rst_win.w = ALIGN_FLOOR(rst_win.w, hdl->rtc_param.io_size_info.align.w);
	rst_win.h = ALIGN_FLOOR(rst_win.h, hdl->rtc_param.io_size_info.align.h);

	/* check sensor output and calc x & y, according to sft*/
	g_re_sft_sz[id].x = 0;
	g_re_sft_sz[id].y = 0;
	chk_sft.x = hdl->rtc_param.io_size_info.auto_info.sft.x + ((hdl->rtc_ctrl.sie_act_win.w - rst_win.w) >> 1);
	chk_sft.y = hdl->rtc_param.io_size_info.auto_info.sft.y + ((hdl->rtc_ctrl.sie_act_win.h - rst_win.h) >> 1);
	if (chk_sft.x < 0) {
		ctl_sie_dbg_err("sft x underflow\r\n");
		chk_sft.x = 0;
	}
	if (chk_sft.y < 0) {
		ctl_sie_dbg_err("sft y underflow\r\n");
		chk_sft.y = 0;
	}
	if (chk_sft.x + rst_win.w > hdl->rtc_ctrl.sie_act_win.w) {
		ctl_sie_dbg_err("sft x ovf\r\n");
		chk_sft.x = hdl->rtc_ctrl.sie_act_win.w - rst_win.w;
	}
	if (chk_sft.y + rst_win.h > hdl->rtc_ctrl.sie_act_win.h) {
		ctl_sie_dbg_err("sft y ovf\r\n");
		chk_sft.y = hdl->rtc_ctrl.sie_act_win.h - rst_win.h;
	}
	rst_win.x = (UINT32)chk_sft.x;
	rst_win.y = (UINT32)chk_sft.y;

	/* update for get parameters */
	hdl->rtc_param.io_size_info.auto_info.sft.x = rst_win.x - ((hdl->rtc_ctrl.sie_act_win.w - sen_mode_param->crp_size.w) >> 1);
	hdl->rtc_param.io_size_info.auto_info.sft.y = rst_win.y - ((hdl->rtc_ctrl.sie_act_win.h - sen_mode_param->crp_size.h) >> 1);
	return rst_win;
}

static USIZE __sie_scl_sz_auto(CTL_SIE_ID id, CTL_SIE_HDL *hdl)
{
	USIZE rst_sz = {0};
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);

	if (g_sie_scl_max_sz[id].w == CTL_SIE_DFT) {
		g_sie_scl_max_sz[id].w = g_sie_crp_win[id].w;
		hdl->rtc_param.io_size_info.auto_info.sie_scl_max_sz.w = g_sie_scl_max_sz[id].w;
	}

	if (g_sie_scl_max_sz[id].h == CTL_SIE_DFT) {
		g_sie_scl_max_sz[id].h = g_sie_crp_win[id].h;
		hdl->rtc_param.io_size_info.auto_info.sie_scl_max_sz.h = g_sie_scl_max_sz[id].h;
	}

	/* get sie scale size, accroding to g_sie_scl_max_sz */
	if (g_sie_crp_win[id].w >= g_sie_scl_max_sz[id].w) {
		g_sie_scl_sz[id].w = g_sie_scl_max_sz[id].w;
	} else {
		g_sie_scl_sz[id].w = g_sie_crp_win[id].w;
	}
	if (g_sie_crp_win[id].h >= g_sie_scl_max_sz[id].h) {
		g_sie_scl_sz[id].h = g_sie_scl_max_sz[id].h;
	} else {
		g_sie_scl_sz[id].h = g_sie_crp_win[id].h;
	}

#if 0
	//check ipp bandwidth start
	{
		UINT64 ratio;
		UINT32 sen_mode_fps;
		if (hdl->rtc_param.chg_senmode_info.sel == CTL_SEN_MODESEL_AUTO) {
			sen_mode_fps = hdl->rtc_param.chg_senmode_info.auto_info.frame_rate;
		} else {
			sen_mode_fps = hdl->rtc_param.chg_senmode_info.manual_info.frame_rate;
		}

		if (sen_mode_fps == 0) {
			ctl_sie_dbg_wrn("sensor fps zero\r\n");
			sen_mode_fps = CTL_SIE_DEST_MAX_FPS;
		}

		if ((UINT64)g_sie_scl_sz[id].w * g_sie_scl_sz[id].h * sen_mode_fps > (UINT64)CTL_SIE_DEST_MAX_SZ_W * CTL_SIE_DEST_MAX_SZ_H * CTL_SIE_DEST_MAX_FPS) {
			ratio = CTL_SIE_DIV_U64((UINT64)CTL_SIE_DEST_MAX_SZ_W * CTL_SIE_DEST_MAX_SZ_H * CTL_SIE_IOSIZE_BASE, g_sie_scl_sz[id].w * g_sie_scl_sz[id].h);
			ratio = CTL_SIE_DIV_U64(ratio * CTL_SIE_DEST_MAX_FPS * CTL_SIE_IOSIZE_BASE, sen_mode_fps);
			ratio = int_sqrt(ratio);
			g_sie_scl_sz[id].w = CTL_SIE_ALIGN_ROUNDDOWN(CTL_SIE_DIV_U64(ratio * g_sie_scl_sz[id].w, CTL_SIE_IOSIZE_BASE), CTL_SIE_DEST_DFT_ALIGN);
			g_sie_scl_sz[id].h = CTL_SIE_ALIGN_ROUNDDOWN(CTL_SIE_DIV_U64(ratio * g_sie_scl_sz[id].h, CTL_SIE_IOSIZE_BASE), CTL_SIE_DEST_DFT_ALIGN);
		}
	}
	//check ipp bandwidth end
#endif

	//fit dest align
	rst_sz.w = ALIGN_FLOOR(g_sie_scl_sz[id].w, CTL_SIE_LCM(sie_limit->scale_out_align.w, hdl->rtc_param.io_size_info.align.w));
	rst_sz.h = ALIGN_FLOOR(g_sie_scl_sz[id].h, CTL_SIE_LCM(sie_limit->scale_out_align.h, hdl->rtc_param.io_size_info.align.h));
	return rst_sz;
}

static USIZE __sie_scl_sz_manual(CTL_SIE_ID id, CTL_SIE_HDL *hdl)
{
	USIZE rst_sz = {0};
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);

	if (g_sie_scl_sz[id].w > g_sie_crp_win[id].w) {
		ctl_sie_dbg_err("N.S. scl up w(%d->%d), set to crop w\r\n", (int)(g_sie_crp_win[id].w), (int)(g_sie_scl_sz[id].w));
		g_sie_scl_sz[id].w = g_sie_crp_win[id].w;
	}
	if (g_sie_scl_sz[id].h > g_sie_crp_win[id].h) {
		ctl_sie_dbg_err("N.S. scl up h(%d->%d), force set to crop h\r\n", (int)(g_sie_crp_win[id].h), (int)(g_sie_scl_sz[id].h));
		g_sie_scl_sz[id].h = g_sie_crp_win[id].h;
	}

	g_sie_scl_sz[id].w = ALIGN_FLOOR(g_sie_scl_sz[id].w, CTL_SIE_LCM(sie_limit->scale_out_align.w, hdl->rtc_param.io_size_info.align.w));
	g_sie_scl_sz[id].h = ALIGN_FLOOR(g_sie_scl_sz[id].h, CTL_SIE_LCM(sie_limit->scale_out_align.h, hdl->rtc_param.io_size_info.align.h));

	rst_sz = g_sie_scl_sz[id];

	return rst_sz;
}

static URECT __dest_crp_win_auto(CTL_SIE_ID id, CTL_SIE_HDL *hdl)
{
	URECT rst_win = {0};
	IPOINT chk_sft = {0};

	/* update g_re_crp_sz accroding to sie scale ratio */
	g_re_crp_sz[id].w = CTL_SIE_DIV_U64((UINT64)g_re_crp_sz[id].w * g_sie_scl_sz[id].w, g_sie_crp_win[id].w);
	g_re_crp_sz[id].h = CTL_SIE_DIV_U64((UINT64)g_re_crp_sz[id].h * g_sie_scl_sz[id].h, g_sie_crp_win[id].h);

	/* calc win */
	rst_win.w = g_sie_scl_sz[id].w - g_re_crp_sz[id].w;
	rst_win.h = g_sie_scl_sz[id].h - g_re_crp_sz[id].h;

	/* crop destination extra proportion */
	if (g_dest_ext_crp_prop[id].w > 100) {
		ctl_sie_dbg_err("dest_ext_crp_prop w err %d\r\n", (int)(g_dest_ext_crp_prop[id].w));
		g_dest_ext_crp_prop[id].w = 100;
	}
	if (g_dest_ext_crp_prop[id].h > 100) {
		ctl_sie_dbg_err("dest_ext_crp_prop h err %d\r\n", (int)(g_dest_ext_crp_prop[id].h));
		g_dest_ext_crp_prop[id].h = 100;
	}
	rst_win.w = CTL_SIE_DIV_U64((UINT64)rst_win.w * (100 - g_dest_ext_crp_prop[id].w), 100);
	rst_win.h = CTL_SIE_DIV_U64((UINT64)rst_win.h * (100 - g_dest_ext_crp_prop[id].h), 100);

	/* align */
	rst_win.w = ALIGN_FLOOR(rst_win.w, g_dest_align[id].w);
	rst_win.h = ALIGN_FLOOR(rst_win.h, g_dest_align[id].h);

	/* calc dest start x & y, according to sft*/
	chk_sft.x = g_re_sft_sz[id].x + ((g_sie_scl_sz[id].w - rst_win.w) >> 1);
	chk_sft.y = g_re_sft_sz[id].y + ((g_sie_scl_sz[id].h - rst_win.h) >> 1);
	if (chk_sft.x < 0) {
		ctl_sie_dbg_err("sft x underflow\r\n");
		chk_sft.x = 0;
	}
	if (chk_sft.y < 0) {
		ctl_sie_dbg_err("sft y underflow\r\n");
		chk_sft.y = 0;
	}
	if (chk_sft.x + rst_win.w > g_sie_scl_sz[id].w) {
		ctl_sie_dbg_err("sft x ovf\r\n");
		chk_sft.x = g_sie_scl_sz[id].w - rst_win.w;
	}
	if (chk_sft.y + rst_win.h > g_sie_scl_sz[id].h) {
		ctl_sie_dbg_err("sft y ovf\r\n");
		chk_sft.y = g_sie_scl_sz[id].h - rst_win.h;
	}
	rst_win.x = (UINT32)chk_sft.x;
	rst_win.y = (UINT32)chk_sft.y;

	return rst_win;
}

static URECT __dest_crp_win_manual(CTL_SIE_ID id, CTL_SIE_HDL *hdl)
{
	URECT rst_win = {0};
	CTL_SIE_IO_SIZE_INFO *io_size_info = &(hdl->rtc_param.io_size_info);
	BOOL chk;

	if (io_size_info->size_info.dest_crp.w > g_sie_scl_sz[id].w) {
		ctl_sie_dbg_err("ovf w(%d->%d)\r\n", (int)(g_sie_scl_sz[id].w), (int)(io_size_info->size_info.dest_crp.w));
		io_size_info->size_info.dest_crp.w = g_sie_scl_sz[id].w;
	}
	if (io_size_info->size_info.dest_crp.h > g_sie_scl_sz[id].h) {
		ctl_sie_dbg_err("ovf h(%d->%d)\r\n", (int)(g_sie_scl_sz[id].h), (int)(io_size_info->size_info.dest_crp.h));
		io_size_info->size_info.dest_crp.h = g_sie_scl_sz[id].h;
	}
	if (io_size_info->size_info.dest_crp.w + io_size_info->size_info.dest_crp.x > g_sie_scl_sz[id].w) {
		ctl_sie_dbg_err("ovf x %d+%d>%d\r\n", (int)(io_size_info->size_info.dest_crp.w), (int)(io_size_info->size_info.dest_crp.x), (int)(g_sie_scl_sz[id].w));
		io_size_info->size_info.dest_crp.w = g_sie_scl_sz[id].w;
	}
	if (io_size_info->size_info.dest_crp.h + io_size_info->size_info.dest_crp.y > g_sie_scl_sz[id].h) {
		ctl_sie_dbg_err("ovf y %d+%d>%d\r\n", (int)(io_size_info->size_info.dest_crp.h), (int)(io_size_info->size_info.dest_crp.y), (int)(g_sie_scl_sz[id].h));
		io_size_info->size_info.dest_crp.h = g_sie_scl_sz[id].h;
	}

	/* check align */
	chk = TRUE;
	chk &= CTL_SIE_CHK_ALIGN(g_dest_align[id].w, CTL_SIE_DEST_DFT_ALIGN_W);
	chk &= CTL_SIE_CHK_ALIGN(g_dest_align[id].h, CTL_SIE_DEST_DFT_ALIGN_H);
	if (!chk) {
		ctl_sie_dbg_wrn("dest need align to (%d %d) (%d %d)\r\n", (int)(CTL_SIE_DEST_DFT_ALIGN_W),(int)(CTL_SIE_DEST_DFT_ALIGN_H), (int)(g_dest_align[id].w), (int)(g_dest_align[id].h));
		g_dest_align[id].w = CTL_SIE_LCM(CTL_SIE_DEST_DFT_ALIGN_W, g_dest_align[id].w);
		g_dest_align[id].h = CTL_SIE_LCM(CTL_SIE_DEST_DFT_ALIGN_H, g_dest_align[id].h);
	}

	io_size_info->size_info.dest_crp.w = ALIGN_FLOOR(io_size_info->size_info.dest_crp.w, g_dest_align[id].w);
	io_size_info->size_info.dest_crp.h = ALIGN_FLOOR(io_size_info->size_info.dest_crp.h, g_dest_align[id].h);

	rst_win = io_size_info->size_info.dest_crp;

	return rst_win;
}

URECT __ctl_sie_iosize_get_sie_crp_win(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);
	URECT rst_win = {0};
	BOOL chk;

	if (hdl == NULL) {
		ctl_sie_dbg_err("id %d get sie hdl err\r\n", (int)(id));
		return rst_win;
	}

	if (sen_mode_param->mode_type == CTL_SEN_MODE_CCIR || sen_mode_param->mode_type == CTL_SEN_MODE_CCIR_INTERLACE) {
		/* conv align */
		if (hdl->rtc_param.data_fmt == CTL_SIE_YUV_422_SPT || hdl->rtc_param.data_fmt == CTL_SIE_YUV_420_SPT) {
			__ctl_sie_conv_align(id, &hdl->rtc_param.io_size_info.align.w, CTL_SIE_CCIR_CRP_ALIGN * 2);
			__ctl_sie_conv_align(id, &hdl->rtc_param.io_size_info.dest_align.w, CTL_SIE_CCIR_CRP_ALIGN);
		} else {
			__ctl_sie_conv_align(id, &hdl->rtc_param.io_size_info.align.w, CTL_SIE_CCIR_CRP_ALIGN);
			__ctl_sie_conv_align(id, &hdl->rtc_param.io_size_info.dest_align.w, CTL_SIE_CCIR_CRP_ALIGN);
		}

		__ctl_sie_conv_align(id, &hdl->rtc_param.io_size_info.align.h, CTL_SIE_CCIR_CRP_ALIGN);
		__ctl_sie_conv_align(id, &hdl->rtc_param.io_size_info.dest_align.h, CTL_SIE_CCIR_CRP_ALIGN);

		/* check input align match CTL_SIE_CRP_ALIGN */
		chk = TRUE;
		chk &= CTL_SIE_CHK_ALIGN(hdl->rtc_param.io_size_info.align.w, hdl->rtc_param.io_size_info.dest_align.w);
		chk &= CTL_SIE_CHK_ALIGN(hdl->rtc_param.io_size_info.align.h, hdl->rtc_param.io_size_info.dest_align.h);
		if (!chk) {
			ctl_sie_dbg_wrn("sie need align to (%d, %d), (%d, %d)\r\n", (int)(hdl->rtc_param.io_size_info.align.w), (int)(hdl->rtc_param.io_size_info.align.h), (int)(hdl->rtc_param.io_size_info.dest_align.w), (int)(hdl->rtc_param.io_size_info.dest_align.h));
			hdl->rtc_param.io_size_info.dest_align.w = hdl->rtc_param.io_size_info.align.w = CTL_SIE_LCM(hdl->rtc_param.io_size_info.dest_align.w, hdl->rtc_param.io_size_info.align.w);
			hdl->rtc_param.io_size_info.dest_align.h = hdl->rtc_param.io_size_info.align.h = CTL_SIE_LCM(hdl->rtc_param.io_size_info.dest_align.h, hdl->rtc_param.io_size_info.align.h);

		}

		if (hdl->rtc_param.io_size_info.iosize_sel == CTL_SIE_IOSIZE_AUTO) {
			rst_win = __sie_crp_win_ccir_auto(id, hdl, sen_mode_param);
		} else if (hdl->rtc_param.io_size_info.iosize_sel == CTL_SIE_IOSIZE_MANUAL) {
			rst_win = __sie_crp_win_manual(id, hdl);
		} else {
			ctl_sie_dbg_err("iosize_sel err\r\n");
		}
	} else {
		/* conv align */
		__ctl_sie_conv_align(id, &hdl->rtc_param.io_size_info.align.w, sie_limit->crp_win_align.w);
		__ctl_sie_conv_align(id, &hdl->rtc_param.io_size_info.align.h, sie_limit->crp_win_align.h);

		/* check input align match sie limitation */
		chk = TRUE;
		chk &= CTL_SIE_CHK_ALIGN(hdl->rtc_param.io_size_info.align.w, sie_limit->crp_win_align.w);
		chk &= CTL_SIE_CHK_ALIGN(hdl->rtc_param.io_size_info.align.h, sie_limit->crp_win_align.h);
		if (!chk) {
			ctl_sie_dbg_wrn("sie crop need align to(%d, %d), cur(%d, %d)\r\n", (int)(sie_limit->crp_win_align.w), (int)(sie_limit->crp_win_align.h), (int)(hdl->rtc_param.io_size_info.align.w), (int)(hdl->rtc_param.io_size_info.align.h));
			hdl->rtc_param.io_size_info.align.w = CTL_SIE_LCM(sie_limit->crp_win_align.w, hdl->rtc_param.io_size_info.align.w);
			hdl->rtc_param.io_size_info.align.h = CTL_SIE_LCM(sie_limit->crp_win_align.h, hdl->rtc_param.io_size_info.align.h);
		}

		if (hdl->rtc_param.io_size_info.iosize_sel == CTL_SIE_IOSIZE_AUTO) {
			hdl->rtc_param.io_size_info.align.w = CTL_SIE_LCM(CTL_SIE_AUTO_CROP_SIZE_ALIGN, hdl->rtc_param.io_size_info.align.w);
			hdl->rtc_param.io_size_info.align.h = CTL_SIE_LCM(CTL_SIE_AUTO_CROP_SIZE_ALIGN, hdl->rtc_param.io_size_info.align.h);
			rst_win = __sie_crp_win_auto(id, hdl, sen_mode_param);
		} else if (hdl->rtc_param.io_size_info.iosize_sel == CTL_SIE_IOSIZE_MANUAL) {
			rst_win = __sie_crp_win_manual(id, hdl);
		} else {
			ctl_sie_dbg_err("iosize_sel err\r\n");
		}
	}

	/* avoid system crash */
	if (rst_win.w == 0) {
		ctl_sie_dbg_err("get w fail, set crop w to %d\r\n", (int)(CTL_SIE_DFT_WIDTH));
		rst_win.w = CTL_SIE_DFT_WIDTH;
	}

	if (rst_win.h == 0) {
		ctl_sie_dbg_err("get h fail, set crop h to %d\r\n", (int)(CTL_SIE_DFT_HEIGHT));
		rst_win.h = CTL_SIE_DFT_HEIGHT;
	}
	g_sie_crp_win[id] = rst_win;

	ctl_sie_dbg_ind("[SIE][CRP]win(x%d,y%d,w%d,h%d)\r\n", (int)(rst_win.x), (int)(rst_win.y), (int)(rst_win.w), (int)(rst_win.h));
	return rst_win;
}

USIZE __ctl_sie_iosize_get_sie_scl_sz(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);
	USIZE rst_sz = {0};

	if (hdl == NULL) {
		ctl_sie_dbg_err("id %d get sie hdl err\r\n", (int)(id));
		return rst_sz;
	}

	if (sen_mode_param->mode_type == CTL_SEN_MODE_CCIR || sen_mode_param->mode_type == CTL_SEN_MODE_CCIR_INTERLACE) {
		g_sie_scl_max_sz[id].w = CTL_SIE_DFT;
		g_sie_scl_max_sz[id].h = CTL_SIE_DFT;

		if (hdl->rtc_param.data_fmt == CTL_SIE_YUV_422_NOSPT) {
			rst_sz.w = g_sie_crp_win[id].w;
		} else {	//CTL_SIE_YUV_422_SPT and CTL_SIE_YUV_420_SPT
			rst_sz.w = g_sie_crp_win[id].w >> 1;
		}
		rst_sz.h = g_sie_crp_win[id].h;
	} else {
		if (sie_limit->support_func & (KDRV_SIE_FUNC_SPT_BS_H|KDRV_SIE_FUNC_SPT_BS_V)) {
			g_sie_scl_max_sz[id] = hdl->rtc_param.io_size_info.auto_info.sie_scl_max_sz;
			g_sie_scl_sz[id] = hdl->rtc_param.io_size_info.size_info.sie_scl_out;
			if (hdl->rtc_param.io_size_info.iosize_sel == CTL_SIE_IOSIZE_AUTO) {
				rst_sz = __sie_scl_sz_auto(id, hdl);
			} else if (hdl->rtc_param.io_size_info.iosize_sel == CTL_SIE_IOSIZE_MANUAL) {
				rst_sz = __sie_scl_sz_manual(id, hdl);
			} else {
				ctl_sie_dbg_err("iosize_sel err\r\n");
			}

			/* avoid system crash */
			if (rst_sz.w == 0) {
				ctl_sie_dbg_err("get w fail, set scale out w to %d\r\n", (int)(CTL_SIE_DFT_WIDTH));
				rst_sz.w = CTL_SIE_DFT_WIDTH;
			}

			if (rst_sz.h == 0) {
				ctl_sie_dbg_err("get h fail, set scale out h to %d\r\n", (int)(CTL_SIE_DFT_HEIGHT));
				rst_sz.h = CTL_SIE_DFT_HEIGHT;
			}
		} else {
			g_sie_scl_max_sz[id].w = CTL_SIE_DFT;
			g_sie_scl_max_sz[id].h = CTL_SIE_DFT;
			rst_sz.w = g_sie_crp_win[id].w;
			rst_sz.h = g_sie_crp_win[id].h;
		}
	}

	/* update for get */
	g_sie_scl_sz[id] = rst_sz;
	ctl_sie_dbg_ind("[SIE][SCL]sz(w%d,h%d)\r\n", (int)(rst_sz.w), (int)(rst_sz.h));
	return rst_sz;
}

URECT __ctl_sie_iosize_get_dest_crp_win(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	BOOL chk;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);
	URECT rst_win = {0};

	if (hdl == NULL) {
		ctl_sie_dbg_err("id %d get sie hdl err\r\n", (int)(id));
		return rst_win;
	}

	if (sen_mode_param->mode_type == CTL_SEN_MODE_CCIR || sen_mode_param->mode_type == CTL_SEN_MODE_CCIR_INTERLACE) {
		g_dest_ext_crp_prop[id].w = 0;
		g_dest_ext_crp_prop[id].h = 0;
		g_dest_align[id] = hdl->rtc_param.io_size_info.dest_align;
		rst_win.w = g_sie_scl_sz[id].w;
		rst_win.h = g_sie_scl_sz[id].h;
	} else {
		if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_BS_H) {
			g_dest_ext_crp_prop[id].w = hdl->rtc_param.io_size_info.auto_info.dest_ext_crp_prop.w;
			g_dest_align[id].w = hdl->rtc_param.io_size_info.dest_align.w;
		} else {
			g_dest_ext_crp_prop[id].w = 0;
			g_dest_align[id].w = hdl->rtc_param.io_size_info.align.w;
		}

		if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_BS_V) {
			g_dest_ext_crp_prop[id].h = hdl->rtc_param.io_size_info.auto_info.dest_ext_crp_prop.h;
			g_dest_align[id].h = hdl->rtc_param.io_size_info.dest_align.h;
		} else {
			g_dest_ext_crp_prop[id].h = 0;
			g_dest_align[id].h = hdl->rtc_param.io_size_info.align.h;
		}

		/* conv align */
		__ctl_sie_conv_align(id, &g_dest_align[id].w, CTL_SIE_DEST_DFT_ALIGN_W);
		__ctl_sie_conv_align(id, &g_dest_align[id].h, CTL_SIE_DEST_DFT_ALIGN_H);

		/* check input align match CTL_SIE_DEST_DFT_ALIGN */
		chk = TRUE;
		chk &= CTL_SIE_CHK_ALIGN(g_dest_align[id].w, CTL_SIE_DEST_DFT_ALIGN_W);
		chk &= CTL_SIE_CHK_ALIGN(g_dest_align[id].h, CTL_SIE_DEST_DFT_ALIGN_H);
		if (!chk) {
			ctl_sie_dbg_wrn("dest need align to (%d,%d) (%d %d)\r\n", (int)(CTL_SIE_DEST_DFT_ALIGN_W),(int)(CTL_SIE_DEST_DFT_ALIGN_H), (int)(g_dest_align[id].w), (int)(g_dest_align[id].h));
			g_dest_align[id].w = CTL_SIE_LCM(CTL_SIE_DEST_DFT_ALIGN_W, g_dest_align[id].w);
			g_dest_align[id].h = CTL_SIE_LCM(CTL_SIE_DEST_DFT_ALIGN_H, g_dest_align[id].h);
		}        			

		hdl->rtc_param.io_size_info.dest_align = g_dest_align[id];

		if (hdl->rtc_param.io_size_info.iosize_sel == CTL_SIE_IOSIZE_AUTO) {
			rst_win = __dest_crp_win_auto(id, hdl);
		} else if (hdl->rtc_param.io_size_info.iosize_sel == CTL_SIE_IOSIZE_MANUAL) {
			rst_win = __dest_crp_win_manual(id, hdl);
		} else {
			ctl_sie_dbg_err("iosize_sel err\r\n");
		}

		/* avoid system crash */
		if (rst_win.w == 0) {
			ctl_sie_dbg_err("get w fail, set to %d\r\n", (int)(CTL_SIE_DFT_WIDTH));
			rst_win.w = CTL_SIE_DFT_WIDTH;
		}

		if (rst_win.h == 0) {
			ctl_sie_dbg_err("get h fail, set to %d\r\n", (int)(CTL_SIE_DFT_HEIGHT));
			rst_win.h = CTL_SIE_DFT_HEIGHT;
		}
	}
	ctl_sie_dbg_ind("[SIE][DEST]win(x%d,y%d,w%d,h%d)\r\n", (int)(rst_win.x), (int)(rst_win.y), (int)(rst_win.w), (int)(rst_win.h));
	return rst_win;
}

static CTL_SIE_IOSIZE ctl_sie_iosize_tab = {
	__ctl_sie_iosize_get_sie_crp_win,
	__ctl_sie_iosize_get_sie_scl_sz,
	__ctl_sie_iosize_get_dest_crp_win,
};

CTL_SIE_IOSIZE *ctl_sie_iosize_get_obj(void)
{
	return &ctl_sie_iosize_tab;
}

void ctl_sie_iosize_init(CTL_SIE_ID id)
{
	memset((void *)&g_dest_ext_crp_prop[id], 0, sizeof(USIZE));
	g_sie_scl_max_sz[id].w = CTL_SIE_DFT;
	g_sie_scl_max_sz[id].h = CTL_SIE_DFT;
	g_dest_align[id].w = CTL_SIE_DFT;
	g_dest_align[id].h = CTL_SIE_DFT;
	memset((void *)&g_sie_scl_sz[id], 0, sizeof(USIZE));
	memset((void *)&g_re_crp_sz[id], 0, sizeof(USIZE));
	memset((void *)&g_sie_crp_win[id], 0, sizeof(URECT));
	memset((void *)&g_re_sft_sz[id], 0, sizeof(IPOINT));
}

