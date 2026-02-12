/**
    IPL Utility.

    @file       ipl_utility.c
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "ipp_debug_int.h"
#include "kflow_videoprocess/ctl_ipp_util.h"
#include "ctl_ipp_util_int.h"
#include "comm/hwclock.h"

UINT32 ctl_ipp_util_yuvsize(VDO_PXLFMT fmt, UINT32 y_width, UINT32 y_height)
{
	UINT32 ratio, size, tmp_lof, tmp_height;

	switch (fmt) {
	case VDO_PXLFMT_YUV444_PLANAR:
	case VDO_PXLFMT_YUV444:
	case VDO_PXLFMT_YUV444_ONE:
	case VDO_PXLFMT_RGB888_PLANAR:
		ratio = 30;
		size = ALIGN_CEIL_4((y_width * y_height * ratio) / 10);
		break;

	case VDO_PXLFMT_YUV422_PLANAR:
	case VDO_PXLFMT_YUV422:
		ratio = 20;
		size = ALIGN_CEIL_4((y_width * y_height * ratio) / 10);
		break;

	case VDO_PXLFMT_YUV420_PLANAR:
	case VDO_PXLFMT_YUV420:
		ratio = 15;
		size = ALIGN_CEIL_4((y_width * y_height * ratio) / 10);
		break;

	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_520_IR8:
	case VDO_PXLFMT_520_IR16:
		ratio = 10;
		size = ALIGN_CEIL_4((y_width * y_height * ratio) / 10);
		break;

	case VDO_PXLFMT_YUV420_NVX1_H265:
		/* 680 HEVC Compress, yuv + side_info */
		/* y */
		tmp_lof = ALIGN_CEIL(y_width, 128) << 2;
		tmp_height = ALIGN_CEIL(y_height, 64) >> 2;
		size = tmp_lof * tmp_height;

		/* uv */
		tmp_lof = ALIGN_CEIL(ctl_ipp_util_y2uvlof(fmt, y_width), 128) * 3 / 4;
		tmp_height = ALIGN_CEIL(ctl_ipp_util_y2uvheight(fmt, y_height), 32);
		size += tmp_lof * tmp_height;

		/* side_info */
		tmp_lof = ALIGN_CEIL(y_width, 128) >> 7;
		tmp_height = ALIGN_CEIL(y_height, 64) >> 6;
		size += ALIGN_CEIL((tmp_lof << 2), 32) * tmp_height;
		break;

	case VDO_PXLFMT_YUV420_NVX1_H264:
		/* 680 H264 Compress, yuv + side_info */
		/* y */
		tmp_lof = ALIGN_CEIL(y_width, 128) << 2;
		tmp_height = ALIGN_CEIL(y_height, 16) >> 2;
		size = tmp_lof * tmp_height;

		/* uv */
		tmp_lof = ALIGN_CEIL(ctl_ipp_util_y2uvlof(fmt, y_width), 128) * 3 / 4;
		tmp_height = ALIGN_CEIL(ctl_ipp_util_y2uvheight(fmt, y_height), 8);
		size += tmp_lof * tmp_height;

		/* side_info */
		tmp_lof = ALIGN_CEIL(y_width, 128) >> 7;
		tmp_height = ALIGN_CEIL(y_height, 16) >> 4;
		size += ALIGN_CEIL(tmp_lof, 32) * tmp_height;
		break;

	case VDO_PXLFMT_YUV420_NVX2:
		/* 520 yuv compress, compress rate fix 75%, 16x align */
		/* y */
		tmp_lof = ALIGN_CEIL(y_width, 16) * 3 / 4;
		size = tmp_lof * y_height;

		/* uv */
		tmp_lof = ALIGN_CEIL(ctl_ipp_util_y2uvlof(fmt, y_width), 16) * 3 / 4;
		size += tmp_lof * y_height / 2;
		break;

	default:
		ratio = 0;
		size = 0;
		CTL_IPP_DBG_WRN("cal yuv size fail 0x%.8x %d %d %d\r\n", (unsigned int)fmt, (int)y_width, (int)y_height, (int)ratio);
		break;
	}

	/* align size to 64 for cache */
	size = ALIGN_CEIL_64(size);

	return size;
}

UINT32 ctl_ipp_util_y2uvlof(VDO_PXLFMT fmt, UINT32 y_lof)
{
	fmt = fmt & (~VDO_PIX_YCC_MASK);

	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NVX) {
		fmt = VDO_PXLFMT_YUV420;
	}

	switch (fmt) {
	case VDO_PXLFMT_YUV444_PLANAR:
		return y_lof;

	case VDO_PXLFMT_YUV422_PLANAR:
		return y_lof >> 1;

	case VDO_PXLFMT_YUV420_PLANAR:
		return y_lof >> 1;

	case VDO_PXLFMT_YUV444:
		return y_lof << 1;

	case VDO_PXLFMT_YUV422:
		return y_lof;

	case VDO_PXLFMT_YUV420:
		return y_lof;

	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_YUV444_ONE:
	case VDO_PXLFMT_RGB888_PLANAR:
	case VDO_PXLFMT_520_IR16:
	case VDO_PXLFMT_520_IR8:
		return y_lof;

	default:
		CTL_IPP_DBG_WRN("cal uv lineoffset fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_lof);
		break;
	}

	return y_lof;
}

UINT32 ctl_ipp_util_y2uvwidth(VDO_PXLFMT fmt, UINT32 y_w)
{
	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NVX) {
		fmt = VDO_PXLFMT_YUV420;
	}

	switch (fmt) {
	case VDO_PXLFMT_YUV444_PLANAR:
		return y_w;

	case VDO_PXLFMT_YUV422_PLANAR:
		return y_w >> 1;

	case VDO_PXLFMT_YUV420_PLANAR:
		return y_w >> 1;

	case VDO_PXLFMT_YUV444:
		return y_w;

	case VDO_PXLFMT_YUV422:
		return y_w >> 1;

	case VDO_PXLFMT_YUV420:
		return y_w >> 1;

	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_YUV444_ONE:
	case VDO_PXLFMT_RGB888_PLANAR:
	case VDO_PXLFMT_520_IR8:
	case VDO_PXLFMT_520_IR16:
		return y_w;

	default:
		CTL_IPP_DBG_WRN("cal uv width fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_w);
		break;
	}

	return y_w;
}

UINT32 ctl_ipp_util_y2uvheight(VDO_PXLFMT fmt, UINT32 y_h)
{
	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NVX) {
		fmt = VDO_PXLFMT_YUV420;
	}

	switch (fmt) {
	case VDO_PXLFMT_YUV444_PLANAR:
		return y_h;

	case VDO_PXLFMT_YUV422_PLANAR:
		return y_h;

	case VDO_PXLFMT_YUV420_PLANAR:
		return y_h >> 1;

	case VDO_PXLFMT_YUV444:
		return y_h;

	case VDO_PXLFMT_YUV422:
		return y_h;

	case VDO_PXLFMT_YUV420:
		return y_h >> 1;

	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_YUV444_ONE:
	case VDO_PXLFMT_RGB888_PLANAR:
	case VDO_PXLFMT_520_IR8:
	case VDO_PXLFMT_520_IR16:
		return y_h;

	default:
		CTL_IPP_DBG_WRN("cal uv height fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_h);
		break;
	}

	return y_h;
}

UINT32 ctl_ipp_util_ratio2value(UINT32 base, UINT32 ratio, UINT32 unit, UINT32 align)
{
	UINT32 value;

	if (unit == 0) {
		return 0;
	}

	value = (base * ratio) / unit;

	if (align != 0) {
		value = ALIGN_CEIL(value, align);
	}

	return value;
}

USIZE ctl_ipp_util_lca_subout_size(UINT32 w, UINT32 h, CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_size_ratio, UINT32 strp)
{
	USIZE lca_size = {0, 0};
	UINT32 base;
	UINT32 ratio;

	base = CTL_IPP_RATIO_UNIT_DFT;
	ratio = base / 4;

	/* base should > 0 */
	if (lca_size_ratio.ratio_base > 0) {
		base = lca_size_ratio.ratio_base;
		ratio = lca_size_ratio.ratio;

		if (ratio > base || ratio == 0 || (base < ratio * 4)) {
			CTL_IPP_DBG_WRN("illegal lca ratio %d, base %d, force ratio to be %d\r\n", (int)ratio, (int)base, (int)base/4);
			ratio = base / 4;
		}
	}

	/* calculate lca size by ratio */
	lca_size.w = ctl_ipp_util_ratio2value(w, ratio, base, 4);
	lca_size.h = ctl_ipp_util_ratio2value(h, ratio, base, 4);

	/* check lca size for ime&ife2
		520:
		single stripe: lca_width <= 668
		multi stripe:  lca_width <= 660 per stripe

		528:
		lca_width <= 1024 per stripe, no need to check due to line buffer is 4096
	*/
	if (strp == 0) {
		/* bypass limitation check */
		return lca_size;
	}

	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (strp == 0xFFFFFFFF) {
			if (lca_size.w > 668) {
				lca_size.h = ALIGN_CEIL(lca_size.h * 668 / lca_size.w, 2);
				lca_size.w = 668;
			}
		} else if (strp > 2640) {
			if (ctl_ipp_util_ratio2value(strp, ratio, base, 4) > 660) {
				/* 660/2688 ~= 0.2455 */
				base = 1000;
				ratio = 240;
				lca_size.w = ctl_ipp_util_ratio2value(w, ratio, base, 4);
				lca_size.h = ctl_ipp_util_ratio2value(h, ratio, base, 4);
			}
		}
	}

	return lca_size;
}

UINT64 ctl_ipp_util_get_syst_timestamp(void)
{
	UINT64 rt;

	rt = hwclock_get_longcounter();
	return rt;
}

UINT32 ctl_ipp_util_get_syst_counter(void)
{
	return hwclock_get_counter();
}

UINT32 ctl_ipp_util_youtsize(UINT32 w, UINT32 h)
{
	return ALIGN_CEIL_4((w * 12) / 8) * h;
}

UINT32 ctl_ipp_util_3dnr_ms_roi_size(UINT32 w, UINT32 h)
{
	UINT32 lofs, size;

	lofs = ctl_ipp_util_3dnr_ms_roi_lofs(w);
	size = lofs * ((h + 15) >> 4);

	return size;
}

UINT32 ctl_ipp_util_3dnr_ms_roi_width(UINT32 w)
{
	return (ALIGN_CEIL_16(w) / 16);
}

UINT32 ctl_ipp_util_3dnr_ms_roi_height(UINT32 h)
{
	return (ALIGN_CEIL_16(h) / 16);
}

UINT32 ctl_ipp_util_3dnr_ms_roi_lofs(UINT32 w)
{
	return (((w + 511) >> 9) << 2);
}

UINT32 ctl_ipp_info_pxlfmt_by_flip(UINT32 pixfmt, CTL_IPP_FLIP_TYPE flip)
{
	UINT32 frm_num;
	UINT32 bpp;
	UINT32 pix_sta;
	VDO_PXLFMT pixfmt_tmp;

	pixfmt_tmp = (VDO_PXLFMT) pixfmt;
	frm_num = VDO_PXLFMT_PLANE(pixfmt_tmp);
	bpp = VDO_PXLFMT_BPP(pixfmt_tmp);
	pix_sta = VDO_PXLFMT_PIX(pixfmt_tmp);

	if (flip & CTL_IPP_FLIP_H) {
		switch (pix_sta) {
		case VDO_PIX_RGGB_R:
			pix_sta = VDO_PIX_RGGB_GR;
			break;

		case VDO_PIX_RGGB_GR:
			pix_sta = VDO_PIX_RGGB_R;
			break;

		case VDO_PIX_RGGB_GB:
			pix_sta = VDO_PIX_RGGB_B;
			break;

		case VDO_PIX_RGGB_B:
			pix_sta = VDO_PIX_RGGB_GB;
			break;

		case VDO_PIX_RCCB_RC:
			pix_sta = VDO_PIX_RCCB_CR;
			break;

		case VDO_PIX_RCCB_CR:
			pix_sta = VDO_PIX_RCCB_RC;
			break;

		case VDO_PIX_RCCB_CB:
			pix_sta = VDO_PIX_RCCB_BC;
			break;

		case VDO_PIX_RCCB_BC:
			pix_sta = VDO_PIX_RCCB_CB;
			break;

		case VDO_PIX_RGBIR44_RGBG_GIGI:
			pix_sta = VDO_PIX_RGBIR44_GBGR_IGIG;
			break;

		case VDO_PIX_RGBIR44_GBGR_IGIG:
			pix_sta = VDO_PIX_RGBIR44_RGBG_GIGI;
			break;

		case VDO_PIX_RGBIR44_GIGI_BGRG:
			pix_sta = VDO_PIX_RGBIR44_IGIG_GRGB;
			break;

		case VDO_PIX_RGBIR44_IGIG_GRGB:
			pix_sta = VDO_PIX_RGBIR44_GIGI_BGRG;
			break;

		case VDO_PIX_RGBIR44_BGRG_GIGI:
			pix_sta = VDO_PIX_RGBIR44_GRGB_IGIG;
			break;

		case VDO_PIX_RGBIR44_GRGB_IGIG:
			pix_sta = VDO_PIX_RGBIR44_BGRG_GIGI;
			break;

		case VDO_PIX_RGBIR44_GIGI_RGBG:
			pix_sta = VDO_PIX_RGBIR44_IGIG_GBGR;
			break;

		case VDO_PIX_RGBIR44_IGIG_GBGR:
			pix_sta = VDO_PIX_RGBIR44_GIGI_RGBG;
			break;

		default:
			CTL_IPP_DBG_WRN("Unknown pix 0x%.8x\r\n", (unsigned int)pix_sta);
			pix_sta = VDO_PIX_RGGB_R;

		}
	}

	if (flip & CTL_IPP_FLIP_V) {
		switch (pix_sta) {
		case VDO_PIX_RGGB_R:
			pix_sta = VDO_PIX_RGGB_GB;
			break;

		case VDO_PIX_RGGB_GR:
			pix_sta = VDO_PIX_RGGB_B;
			break;

		case VDO_PIX_RGGB_GB:
			pix_sta = VDO_PIX_RGGB_R;
			break;

		case VDO_PIX_RGGB_B:
			pix_sta = VDO_PIX_RGGB_GR;
			break;

		case VDO_PIX_RCCB_RC:
			pix_sta = VDO_PIX_RCCB_CB;
			break;

		case VDO_PIX_RCCB_CR:
			pix_sta = VDO_PIX_RCCB_BC;
			break;

		case VDO_PIX_RCCB_CB:
			pix_sta = VDO_PIX_RCCB_RC;
			break;

		case VDO_PIX_RCCB_BC:
			pix_sta = VDO_PIX_RCCB_CR;
			break;

		case VDO_PIX_RGBIR44_RGBG_GIGI:
			pix_sta = VDO_PIX_RGBIR44_GIGI_RGBG;
			break;

		case VDO_PIX_RGBIR44_GBGR_IGIG:
			pix_sta = VDO_PIX_RGBIR44_IGIG_GBGR;
			break;

		case VDO_PIX_RGBIR44_GIGI_BGRG:
			pix_sta = VDO_PIX_RGBIR44_BGRG_GIGI;
			break;

		case VDO_PIX_RGBIR44_IGIG_GRGB:
			pix_sta = VDO_PIX_RGBIR44_GRGB_IGIG;
			break;

		case VDO_PIX_RGBIR44_BGRG_GIGI:
			pix_sta =  VDO_PIX_RGBIR44_GIGI_BGRG;
			break;

		case VDO_PIX_RGBIR44_GRGB_IGIG:
			pix_sta = VDO_PIX_RGBIR44_IGIG_GRGB;
			break;

		case VDO_PIX_RGBIR44_GIGI_RGBG:
			pix_sta = VDO_PIX_RGBIR44_RGBG_GIGI;
			break;

		case VDO_PIX_RGBIR44_IGIG_GBGR:
			pix_sta = VDO_PIX_RGBIR44_GBGR_IGIG;
			break;

		default:
			CTL_IPP_DBG_WRN("Unknown pix 0x%.8x\r\n", (unsigned int)pix_sta);
			pix_sta = VDO_PIX_RGGB_R;

		}
	}

	if (VDO_PXLFMT_CLASS(pixfmt_tmp) == VDO_PXLFMT_CLASS_NRX) {
		pixfmt_tmp = VDO_PXLFMT_MAKE_NRX(bpp, frm_num, pix_sta);
	} else {
		pixfmt_tmp = VDO_PXLFMT_MAKE_RAW(bpp, frm_num, pix_sta);
	}

	return (UINT32) pixfmt_tmp;

}

VDO_PXLFMT ctl_ipp_info_pxlfmt_by_crop(VDO_PXLFMT pixfmt, UINT32 crp_x, UINT32 crp_y)
{
	UINT32 frm_num;
	UINT32 bpp;
	UINT32 pix_sta;
	VDO_PXLFMT pixfmt_tmp;

	pixfmt_tmp = pixfmt;
	frm_num = VDO_PXLFMT_PLANE(pixfmt_tmp);
	bpp = VDO_PXLFMT_BPP(pixfmt_tmp);
	pix_sta = VDO_PXLFMT_PIX(pixfmt_tmp);

	if (crp_x & 1) {
		switch (pix_sta) {
		case VDO_PIX_RGGB_R:
			pix_sta = VDO_PIX_RGGB_GR;
			break;

		case VDO_PIX_RGGB_GR:
			pix_sta = VDO_PIX_RGGB_R;
			break;

		case VDO_PIX_RGGB_GB:
			pix_sta = VDO_PIX_RGGB_B;
			break;

		case VDO_PIX_RGGB_B:
			pix_sta = VDO_PIX_RGGB_GB;
			break;

		case VDO_PIX_RCCB_RC:
			pix_sta = VDO_PIX_RCCB_CR;
			break;

		case VDO_PIX_RCCB_CR:
			pix_sta = VDO_PIX_RCCB_RC;
			break;

		case VDO_PIX_RCCB_CB:
			pix_sta = VDO_PIX_RCCB_BC;
			break;

		case VDO_PIX_RCCB_BC:
			pix_sta = VDO_PIX_RCCB_CB;
			break;

		case VDO_PIX_RGBIR44_RGBG_GIGI:
			pix_sta = VDO_PIX_RGBIR44_GBGR_IGIG;
			break;

		case VDO_PIX_RGBIR44_GBGR_IGIG:
			pix_sta = VDO_PIX_RGBIR44_BGRG_GIGI;
			break;

		case VDO_PIX_RGBIR44_GIGI_BGRG:
			pix_sta = VDO_PIX_RGBIR44_IGIG_GRGB;
			break;

		case VDO_PIX_RGBIR44_IGIG_GRGB:
			pix_sta = VDO_PIX_RGBIR44_GIGI_RGBG;
			break;

		case VDO_PIX_RGBIR44_BGRG_GIGI:
			pix_sta = VDO_PIX_RGBIR44_GRGB_IGIG;
			break;

		case VDO_PIX_RGBIR44_GRGB_IGIG:
			pix_sta = VDO_PIX_RGBIR44_RGBG_GIGI;
			break;

		case VDO_PIX_RGBIR44_GIGI_RGBG:
			pix_sta = VDO_PIX_RGBIR44_IGIG_GBGR;
			break;

		case VDO_PIX_RGBIR44_IGIG_GBGR:
			pix_sta = VDO_PIX_RGBIR44_GIGI_BGRG;
			break;

		default:
			CTL_IPP_DBG_WRN("Unknown pix 0x%.8x\r\n", (unsigned int)pix_sta);
			pix_sta = VDO_PIX_RGGB_R;

		}
	}

	if (crp_y & 1) {
		switch (pix_sta) {
		case VDO_PIX_RGGB_R:
			pix_sta = VDO_PIX_RGGB_GB;
			break;

		case VDO_PIX_RGGB_GR:
			pix_sta = VDO_PIX_RGGB_B;
			break;

		case VDO_PIX_RGGB_GB:
			pix_sta = VDO_PIX_RGGB_R;
			break;

		case VDO_PIX_RGGB_B:
			pix_sta = VDO_PIX_RGGB_GR;
			break;

		case VDO_PIX_RCCB_RC:
			pix_sta = VDO_PIX_RCCB_CB;
			break;

		case VDO_PIX_RCCB_CR:
			pix_sta = VDO_PIX_RCCB_BC;
			break;

		case VDO_PIX_RCCB_CB:
			pix_sta = VDO_PIX_RCCB_RC;
			break;

		case VDO_PIX_RCCB_BC:
			pix_sta = VDO_PIX_RCCB_CR;
			break;

		case VDO_PIX_RGBIR44_RGBG_GIGI:
			pix_sta = VDO_PIX_RGBIR44_GIGI_BGRG;
			break;

		case VDO_PIX_RGBIR44_GBGR_IGIG:
			pix_sta = VDO_PIX_RGBIR44_IGIG_GRGB;
			break;

		case VDO_PIX_RGBIR44_GIGI_BGRG:
			pix_sta = VDO_PIX_RGBIR44_BGRG_GIGI;
			break;

		case VDO_PIX_RGBIR44_IGIG_GRGB:
			pix_sta = VDO_PIX_RGBIR44_GRGB_IGIG;
			break;

		case VDO_PIX_RGBIR44_BGRG_GIGI:
			pix_sta =  VDO_PIX_RGBIR44_GIGI_BGRG;
			break;

		case VDO_PIX_RGBIR44_GRGB_IGIG:
			pix_sta = VDO_PIX_RGBIR44_IGIG_GRGB;
			break;

		case VDO_PIX_RGBIR44_GIGI_RGBG:
			pix_sta = VDO_PIX_RGBIR44_RGBG_GIGI;
			break;

		case VDO_PIX_RGBIR44_IGIG_GBGR:
			pix_sta = VDO_PIX_RGBIR44_GBGR_IGIG;
			break;

		default:
			CTL_IPP_DBG_WRN("Unknown pix 0x%.8x\r\n", (unsigned int)pix_sta);
			pix_sta = VDO_PIX_RGGB_R;

		}
	}

	if (VDO_PXLFMT_CLASS(pixfmt_tmp) == VDO_PXLFMT_CLASS_NRX) {
		pixfmt_tmp = VDO_PXLFMT_MAKE_NRX(bpp, frm_num, pix_sta);
	} else {
		pixfmt_tmp = VDO_PXLFMT_MAKE_RAW(bpp, frm_num, pix_sta);
	}

	return (UINT32) pixfmt_tmp;
}

UINT32 ctl_ipp_util_ethsize(UINT32 w, UINT32 h, BOOL out_bit_sel, BOOL subsample_en)
{
	UINT32 lofs;
	UINT32 eth_size;

	if (w == 0 || h == 0) {
		return 0;
	}

	/* 2/8 bit output */
	if (out_bit_sel == 0) {
		lofs = (w + 2) >> 2;
	} else {
		lofs = w;
	}

	/* subsample, w/2, h/2 */
	if (subsample_en == 0) {
		lofs = ALIGN_CEIL_4(lofs);
		eth_size = lofs * h;
	} else {
		lofs = ALIGN_CEIL_4(((lofs + 1) >> 1));
		eth_size = lofs * (h >> 1);
	}

	return eth_size;
}

UINT32 ctl_ipp_util_rawsize(VDO_PXLFMT fmt, UINT32 w, UINT32 h)
{
	return ALIGN_CEIL_4(w * h * VDO_PXLFMT_BPP(fmt) / 8);
}

