#include "kflow_videoprocess/ctl_vpe.h"
#include "ctl_vpe_int.h"
#include "ctl_vpe_isp_int.h"
#include "kwrap/cpu.h"
#include "kflow_common/nvtmpp.h"

#if CTL_VPE_MODULE_ENABLE
static void ctl_vpe_msg_reset(CTL_VPE_MSG_EVENT *p_evt)
{
	if (p_evt) {
		p_evt->cmd = 0;
		p_evt->param[0] = 0;
		p_evt->param[1] = 0;
		p_evt->param[2] = 0;
		p_evt->rev[0] = CTL_VPE_MSG_STS_FREE;
	}
}

INT32 ctl_vpe_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, CTL_VPE_MEM_POOL *p_que)
{
	unsigned long loc_flg;
	CTL_VPE_MSG_EVENT *p_evt;
	INT32 rt = CTL_VPE_E_OK;

	vk_spin_lock_irqsave(&p_que->lock, loc_flg);

	p_evt = vos_list_first_entry_or_null(&p_que->free_list_head, CTL_VPE_MSG_EVENT, list);
	if (p_evt != NULL) {
		vos_list_del_init(&p_evt->list);
		p_que->cur_free_num--;
		if ((p_que->blk_num - p_que->cur_free_num) > p_que->max_used_num) {
			p_que->max_used_num = p_que->blk_num - p_que->cur_free_num;
		}
		p_evt->rev[0] = CTL_VPE_MSG_STS_LOCK;

		p_evt->cmd = cmd;
		p_evt->param[0] = p1;
		p_evt->param[1] = p2;
		p_evt->param[2] = p3;
		p_evt->timestamp = hwclock_get_counter();
		vos_list_add_tail(&p_evt->list, &p_que->used_list_head);
		vos_flag_iset(p_que->flg_id, CTL_VPE_QUE_FLG_PROC);
	} else {
		rt = CTL_VPE_E_QOVR;
	}

	vk_spin_unlock_irqrestore(&p_que->lock, loc_flg);

	return rt;
}

INT32 ctl_vpe_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, UINT32 *time, CTL_VPE_MEM_POOL *p_que)
{
	unsigned long loc_flg;
	FLGPTN flag;
	CTL_VPE_MSG_EVENT *p_evt = NULL;

	while (1) {
		/* try to receive event */
		vk_spin_lock_irqsave(&p_que->lock, loc_flg);
		p_evt = vos_list_first_entry_or_null(&p_que->used_list_head, CTL_VPE_MSG_EVENT, list);
		if (p_evt) {
			vos_list_del_init(&p_evt->list);

			*cmd = p_evt->cmd;
			*p1 = p_evt->param[0];
			*p2 = p_evt->param[1];
			*p3 = p_evt->param[2];
			*time = p_evt->timestamp;

			ctl_vpe_msg_reset(p_evt);
			vos_list_add_tail(&p_evt->list, &p_que->free_list_head);
			p_que->cur_free_num++;
		}
		vk_spin_unlock_irqrestore(&p_que->lock, loc_flg);

		if (p_evt) {
			break;
		}

		/* recieve event null, wait flag for next attempt */
		vos_flag_wait(&flag, p_que->flg_id, CTL_VPE_QUE_FLG_PROC, TWF_CLR);
	}

	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_msg_flush(CTL_VPE_MEM_POOL *p_que, CTL_VPE_HANDLE *p_hdl, CTL_VPE_MSG_FLUSH_CB flush_cb)
{
	unsigned long loc_flg;
	CTL_VPE_LIST_HEAD *p_list;
	CTL_VPE_LIST_HEAD *n;
	CTL_VPE_MSG_EVENT *p_evt;

	vk_spin_lock_irqsave(&p_que->lock, loc_flg);

	vos_list_for_each_safe(p_list, n, &p_que->used_list_head) {
		p_evt = list_entry(p_list, CTL_VPE_MSG_EVENT, list);
		if ((p_evt->param[0] == (UINT32)p_hdl) && (p_evt->cmd == CTL_VPE_MSG_PROCESS)) {
			p_evt->cmd = CTL_VPE_MSG_IGNORE;
			if (flush_cb) {
				p_hdl->reserved[0] = p_evt->timestamp;
				flush_cb(p_evt->param[0], p_evt->param[1], p_evt->param[2], CTL_VPE_E_FLUSH);
			}
		}
	}

	vk_spin_unlock_irqrestore(&p_que->lock, loc_flg);

	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_msg_modify(CTL_VPE_MEM_POOL *p_que, CTL_VPE_HANDLE *p_hdl, CTL_VPE_MSG_MODIFY_CB modify_cb, void *p_data)
{
	unsigned long loc_flg;
	CTL_VPE_LIST_HEAD *p_list;
	CTL_VPE_LIST_HEAD *n;
	CTL_VPE_MSG_EVENT *p_evt;
	CTL_VPE_MSG_MODIFY_CB_DATA cb_data;

	vk_spin_lock_irqsave(&p_que->lock, loc_flg);

	vos_list_for_each_safe(p_list, n, &p_que->used_list_head) {
		p_evt = list_entry(p_list, CTL_VPE_MSG_EVENT, list);
		if ((p_evt->param[0] == (UINT32)p_hdl) && (p_evt->cmd == CTL_VPE_MSG_PROCESS)) {
			if (modify_cb) {
				cb_data.hdl = p_evt->param[0];
				cb_data.data = p_evt->param[1];
				cb_data.buf_id = p_evt->param[2];
				cb_data.err = CTL_VPE_E_FLUSH;
				cb_data.p_modify_info = p_data;
				modify_cb(&cb_data);
			}
		}
	}

	vk_spin_unlock_irqrestore(&p_que->lock, loc_flg);

	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_msg_init_queue(CTL_VPE_MEM_POOL *p_que)
{
	CTL_VPE_MSG_EVENT *p_pool;
	CTL_VPE_MSG_EVENT *p_evt;
	UINT32 i;

	VOS_INIT_LIST_HEAD(&p_que->free_list_head);
	VOS_INIT_LIST_HEAD(&p_que->used_list_head);

	p_pool = (CTL_VPE_MSG_EVENT *)p_que->start_addr;
	for (i = 0; i < p_que->blk_num; i++) {
		p_evt = &p_pool[i];

		ctl_vpe_msg_reset(p_evt);
		p_evt->rev[1] = i;

		VOS_INIT_LIST_HEAD(&p_evt->list);
		vos_list_add_tail(&p_evt->list, &p_que->free_list_head);
	}

	return CTL_VPE_E_OK;
}

#if 0
#endif

UINT32 ctl_vpe_util_y2uvlof(VDO_PXLFMT fmt, UINT32 y_lof)
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
		DBG_WRN("cal uv lineoffset fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_lof);
		break;
	}

	return y_lof;
}

UINT32 ctl_vpe_util_y2uvwidth(VDO_PXLFMT fmt, UINT32 y_w)
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
		DBG_WRN("cal uv width fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_w);
		break;
	}

	return y_w;
}

UINT32 ctl_vpe_util_y2uvheight(VDO_PXLFMT fmt, UINT32 y_h)
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
		DBG_WRN("cal uv height fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_h);
		break;
	}

	return y_h;
}

UINT32 ctl_vpe_util_yuv_size(VDO_PXLFMT fmt, UINT32 y_width, UINT32 y_height)
{
	UINT32 ratio, size, tmp_lof;

	switch (fmt) {
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

	case VDO_PXLFMT_YUV420_NVX2:
		/* 520 yuv compress, compress rate fix 75%, 16x align */
		/* y */
		tmp_lof = ALIGN_CEIL(y_width, 16) * 3 / 4;
		size = tmp_lof * y_height;

		/* uv */
		tmp_lof = ALIGN_CEIL(ctl_vpe_util_y2uvlof(fmt, y_width), 16) * 3 / 4;
		size += tmp_lof * y_height / 2;
		break;

	default:
		ratio = 0;
		size = 0;
		DBG_WRN("cal yuv size fail 0x%.8x %d %d %d\r\n", (unsigned int)fmt, (int)y_width, (int)y_height, (int)ratio);
		break;
	}

	/* align size to 64 for cache */
	size = ALIGN_CEIL(size, VOS_ALIGN_BYTES);

	return size;
}

#if 0
#endif

static KDRV_VPE_DES_TYPE ctl_vpe_typecast_out_fmt(VDO_PXLFMT fmt)
{
	if (fmt == VDO_PXLFMT_YUV420) {
		return KDRV_VPE_DES_TYPE_YUV420;
	} else if (fmt == VDO_PXLFMT_YUV420_NVX2) {
		return KDRV_VPE_DES_TYPE_YCC_YUV420;
	} else {
		DBG_WRN("unsupport out fmt 0x%.8x\r\n", (unsigned int)fmt);
	}

	return 0;
}

static UINT32 ctl_vpe_typecast_src_drt(VDO_PXLFMT fmt)
{
	UINT32 yuvfmt = fmt & VDO_PIX_YCC_MASK;

	switch (yuvfmt) {
	case VDO_PIX_YCC_BT601:
	case VDO_PIX_YCC_BT709:
		return 3;	/* tv to pc level */

	default:
		return 0;	/* bypass if input is pc level */
	}
}

static UINT32 ctl_vpe_typecast_dst_drt(UINT32 src_drt, CTL_VPE_ISP_YUV_CVT out_drt)
{
	/*
		in_pc -> bypass(bypass) pc(bypass) tv(tv)
		in_tv -> bypass(tv) pc(bypass) tv(tv)
	*/
	UINT32 rt = 0;

	if (src_drt == 3) {
		/* in tv2pc */
		if (out_drt == CTL_VPE_ISP_YUV_CVT_TV || out_drt == CTL_VPE_ISP_YUV_CVT_NONE) {
			rt = 2;	/* pc2tv */
		} else {
			rt = 0;	/* bypass */
		}
	} else if (src_drt == 0) {
		/* in pc */
		if (out_drt == CTL_VPE_ISP_YUV_CVT_TV) {
			rt = 2;
		} else {
			rt = 0;
		}
	} else {
		DBG_WRN("unknown src_drt %d\r\n", src_drt);
	}

	return rt;
}

#if 0
#endif

void ctl_vpe_process_dbg_dump_kflow_isp_param(CTL_VPE_ISP_PARAM *p_isp, int (*dump)(const char *fmt, ...))
{
	CHAR str_mem[128];
	UINT32 str_len;
	UINT32 i;

	CHAR *flip_rot_sel[9] = {
	"ROT_0",		//rotate 0 degree
	"ROT_90",		//rotate 90 degree
	"ROT_180",		//rotate 180 degree
	"ROT_270",		//rotate 270 degree
	"H_ROT_0",    	//horizontal flip + rotate 0 degree
	"H_ROT_90",   	//horizontal flip + rotate 90 degree
	"H_ROT_180",  	//horizontal flip + rotate 180 degree
	"H_ROT_270",	//horizontal flip + rotate 270 degree
	"MANUAL_ROT",
	};

	CHAR *hv_flip_sel[4] = {
	"FLIP_NONE",
	"FLIP_H",
	"FLIP_V",
	"FLIP_HV",
	};

	CHAR *ratio_mode_sel[4] = {
	"FIX_ASPECT_RATIO_UP",
	"FIX_ASPECT_RATIO_DOWN",
	"FIT_ROI",
	"MANUAL",
	};

	if (p_isp == NULL)
		return ;

	dump("\r\n----- ctl_vpe dump isp param -----\r\n");

	dump("sharpen param\r\n");
	dump("%4s %8s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %8s\r\n",
				"en", "src_sel", "weight_th", "weight_gain", "noise_lv", "inv_gamma",
				"edge_str1", "edge_str2", "flat_str", "coring_th",
				"b_halo_clip", "d_hal_clip", "out_sel");
	dump("%4d %8d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %8d\r\n",
				(int)p_isp->sharpen.enable,
				(int)p_isp->sharpen.edge_weight_src_sel,
				(int)p_isp->sharpen.edge_weight_th,
				(int)p_isp->sharpen.edge_weight_gain,
				(int)p_isp->sharpen.noise_level,
				(int)p_isp->sharpen.blend_inv_gamma,
				(int)p_isp->sharpen.edge_sharp_str1,
				(int)p_isp->sharpen.edge_sharp_str2,
				(int)p_isp->sharpen.flat_sharp_str,
				(int)p_isp->sharpen.coring_th,
				(int)p_isp->sharpen.bright_halo_clip,
				(int)p_isp->sharpen.dark_halo_clip,
				(int)p_isp->sharpen.sharpen_out_sel);
	dump("noise_curve: ");
	str_len = 0;
	for (i = 0; i < CTL_VPE_ISP_NOISE_CURVE_NUMS; i++) {
		str_len += snprintf(str_mem + str_len, 128 - str_len, "%d, ", p_isp->sharpen.noise_curve[i]);
	}
	dump("%s\r\n\r\n", str_mem);

	dump("dce param\r\n");
	dump("%4s %10s %8s %12s\r\n", "en", "lsb_rand", "mode", "lens_radius");
	dump("%4d %10d %8d %12d\r\n",
				(int)p_isp->dce_ctl.enable,
				(int)p_isp->dce_ctl.lsb_rand,
				(int)p_isp->dce_ctl.dce_mode,
				(int)p_isp->dce_ctl.lens_radius);

	dump("%12s %8s %8s %8s %8s %10s %8s %8s\r\n",
				"lut_sz", "xofs_i", "xofs_f", "yofs_i", "yofs_f", "lut_addr", "out_w", "out_h");
	dump("%12d %8d %8d %8d %8d 0x%.8x %8d %8d\r\n",
				(int)p_isp->dce_2dlut_param.lut_sz,
				(int)p_isp->dce_2dlut_param.xofs_i,
				(int)p_isp->dce_2dlut_param.xofs_f,
				(int)p_isp->dce_2dlut_param.yofs_i,
				(int)p_isp->dce_2dlut_param.yofs_f,
				(unsigned int)p_isp->dce_2dlut_param.p_lut,
				(int)p_isp->dce_2dlut_param.out_size.w,
				(int)p_isp->dce_2dlut_param.out_size.h);

	dump("%12s %8s %8s %8s %8s/%-8s %8s/%-8s %8s %8s %8s\r\n",
				"fovbound", "boundy", "boundu", "boundv",
				"cent_x", "base", "cent_y", "base",
				"xdist", "ydist", "fovgain");
	dump("%12d %8d %8d %8d %8d/%-8d %8d/%-8d %8d %8d %8d\r\n\r\n",
				p_isp->dce_gdc_param.fovbound,
				p_isp->dce_gdc_param.boundy,
				p_isp->dce_gdc_param.boundu,
				p_isp->dce_gdc_param.boundv,
				p_isp->dce_gdc_param.cent_x_ratio,
				p_isp->dce_gdc_param.cent_ratio_base,
				p_isp->dce_gdc_param.cent_y_ratio,
				p_isp->dce_gdc_param.cent_ratio_base,
				p_isp->dce_gdc_param.xdist_a1,
				p_isp->dce_gdc_param.ydist_a1,
				p_isp->dce_gdc_param.fovgain);

	dump("yuv cvt param\r\n");
	dump("%12s %12s\r\n", "pid", "yuv_cvt_sel");
	for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
		dump("%12d %12d\r\n", i, p_isp->yuv_cvt_param.cvt_sel[i]);
	}

	dump("\r\ndctg param\r\n");
	dump("%4s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %8s %8s\r\n",
				"en", "mount_type", "lut2d_sz", "lens_r", "lens_x_st",
				"lens_y_st", "theta_st", "theta_ed", "phi_st",
				"phi_ed", "rot_z", "rot_y", "out_w", "out_h");
	dump("%4d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %8d %8d\r\n",
				(int)p_isp->dctg_param.enable,
				(int)p_isp->dctg_param.mount_type,
				(int)p_isp->dctg_param.lut2d_sz,
				(int)p_isp->dctg_param.lens_r,
				(int)p_isp->dctg_param.lens_x_st,
				(int)p_isp->dctg_param.lens_y_st,
				(int)p_isp->dctg_param.theta_st,
				(int)p_isp->dctg_param.theta_ed,
				(int)p_isp->dctg_param.phi_st,
				(int)p_isp->dctg_param.phi_ed,
				(int)p_isp->dctg_param.rot_z,
				(int)p_isp->dctg_param.rot_y,
				(int)p_isp->dctg_param.out_size.w,
				(int)p_isp->dctg_param.out_size.h);

	dump("\r\nflip rot param\r\n");
	dump("%12s %12s %12s %22s %8s %10s %8s %8s %8s\r\n", "flip_mode", "rot_degree", "flip_sel", "ratio_mode", "ratio", "fovbound", "boundy", "boundu", "boundv");
	dump("%12s %12d %12s %22s %8d %10d %8d %8d %8d\r\n\r\n",
				flip_rot_sel[p_isp->flip_rot_ctl.flip_rot_mode],
				p_isp->flip_rot_ctl.rot_manual_param.rot_degree,
				hv_flip_sel[p_isp->flip_rot_ctl.rot_manual_param.flip],
				ratio_mode_sel[p_isp->flip_rot_ctl.rot_manual_param.ratio_mode],
				p_isp->flip_rot_ctl.rot_manual_param.ratio,
				p_isp->flip_rot_ctl.rot_manual_param.fovbound,
				p_isp->flip_rot_ctl.rot_manual_param.boundy,
				p_isp->flip_rot_ctl.rot_manual_param.boundu,
				p_isp->flip_rot_ctl.rot_manual_param.boundv);
}

void ctl_vpe_process_dbg_dump_kflow_cfg(CTL_VPE_BASEINFO *p_base, int (*dump)(const char *fmt, ...))
{
	UINT32 i;

	if (p_base == NULL)
		return ;

	dump("\r\n----- ctl_vpe dump kflow cfg -----\r\n");
	dump("%12s %12s %12s %12s %12s %12s %12s %12s %10s\r\n",
				"src_w", "src_h", "src_h_align", "crp_mode", "crp_x", "crp_y", "crp_w", "crp_h", "dcs_mode");
	dump("%12d %12d %12d %12d %12d %12d %12d %12d %10d\r\n\r\n",
				(int)p_base->src_img.size.w,
				(int)p_base->src_img.size.h,
				(int)p_base->src_img.src_img_h_align,
				(int)p_base->in_crop.mode,
				(int)p_base->in_crop.crp_window.x,
				(int)p_base->in_crop.crp_window.y,
				(int)p_base->in_crop.crp_window.w,
				(int)p_base->in_crop.crp_window.h,
				(int)p_base->dcs_mode);

	dump("%4s %4s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %10s\r\n", "pid", "en",
				"pre_crp_x", "pre_crp_y", "pre_crp_w", "pre_crp_h", "scl_w", "scl_h",
				"post_crp_x", "post_crp_y", "post_crp_w", "post_crp_h", "fmt");
	for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
		CTL_VPE_OUT_PATH *p_path = &p_base->out_path[i];

		dump("%4d %4d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d 0x%.8x\r\n",
					(int)i, (int)p_path->enable,
					(int)p_path->pre_scl_crop.x, (int)p_path->pre_scl_crop.y, (int)p_path->pre_scl_crop.w, (int)p_path->pre_scl_crop.h,
					(int)p_path->scl_size.w, (int)p_path->scl_size.h,
					(int)p_path->post_scl_crop.x, (int)p_path->post_scl_crop.y, (int)p_path->post_scl_crop.w, (int)p_path->post_scl_crop.h,
					(unsigned int)p_path->fmt);
	}

	dump("\r\n%4s %4s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %10s\r\n", "pid", "en",
				"bg_w", "bg_h",  "out_x", "out_y", "out_w", "out_h",
				"dst_x", "dst_y", "hole_x", "hole_y", "hole_w", "hole_h",
				"h_align");
	for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
		CTL_VPE_OUT_PATH *p_path = &p_base->out_path[i];

		dump("%4d %4d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %10d\r\n",
					(int)i, (int)p_path->enable,
					(int)p_path->bg_size.w, (int)p_path->bg_size.h,
					(int)p_path->out_window.x, (int)p_path->out_window.y, (int)p_path->out_window.w, (int)p_path->out_window.h,
					(int)p_path->dst_pos.x, (int)p_path->dst_pos.y,
					(int)p_path->hole_region.x, (int)p_path->hole_region.y, (int)p_path->hole_region.w, (int)p_path->hole_region.h,
					(int)p_base->out_path_h_align[i]);
	}
}

void ctl_vpe_process_dbg_dump_kdrv_cfg(CTL_VPE_JOB *p_job, int (*dump)(const char *fmt, ...))
{
	CTL_VPE_HANDLE *p_hdl;
	KDRV_VPE_CONFIG *p_kdrv_cfg;
	UINT32 i;

	if (p_job == NULL)
		return ;

	p_hdl = (CTL_VPE_HANDLE *)p_job->owner;
	p_kdrv_cfg = &p_job->kdrv_cfg;
	dump("----- ctl_vpe(%s) dump kdrv_vpe cfg -----\r\n", p_hdl->name);
	dump("%12s %12s %12s %12s %12s %12s %10s %10s\r\n",
				"width", "height", "crp_x", "crp_y", "crp_w", "crp_h", "addr", "func_mode");
	dump("%12d %12d %12d %12d %12d %12d 0x%.8x 0x%.8x\r\n",
				(int)p_kdrv_cfg->glb_img_info.src_width,
				(int)p_kdrv_cfg->glb_img_info.src_height,
				(int)p_kdrv_cfg->glb_img_info.src_in_x,
				(int)p_kdrv_cfg->glb_img_info.src_in_y,
				(int)p_kdrv_cfg->glb_img_info.src_in_w,
				(int)p_kdrv_cfg->glb_img_info.src_in_h,
				p_kdrv_cfg->glb_img_info.src_addr,
				p_kdrv_cfg->func_mode);

	dump("%4s %4s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %10s %4s %4s\r\n", "pid", "en",
				"pre_crp_x", "pre_crp_y", "pre_crp_w", "pre_crp_h", "scl_w", "scl_h",
				"post_crp_x", "post_crp_y", "post_crp_w", "post_crp_h",
				"addr", "fmt", "drt");
	for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {
		KDRV_VPE_OUT_IMG_INFO *p_path = &p_kdrv_cfg->out_img_info[i];

		dump("%4d %4d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d 0x%.8x %4d %4d\r\n", i,
					(int)p_path->out_dup_num,
					(int)p_path->res_dim_info.sca_crop_x, (int)p_path->res_dim_info.sca_crop_y, (int)p_path->res_dim_info.sca_crop_w, (int)p_path->res_dim_info.sca_crop_h,
					(int)p_path->res_dim_info.sca_width, (int)p_path->res_dim_info.sca_height,
					(int)p_path->res_dim_info.des_crop_out_x, (int)p_path->res_dim_info.des_crop_out_y, (int)p_path->res_dim_info.des_crop_out_w, (int)p_path->res_dim_info.des_crop_out_h,
					(unsigned int)p_path->out_dup_info.out_addr, (int)p_path->out_dup_info.des_type, (int)p_path->res_des_drt);
	}

	dump("\r\n%4s %4s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s\r\n",
				"pid", "en", "bg_w", "bg_h",
				"out_x", "out_y", "out_w", "out_h",
				"rlt_x", "rlt_y", "rlt_w", "rlt_h",
				"hole_x", "hole_y", "hole_w", "hole_h");
	for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {
		KDRV_VPE_OUT_IMG_INFO *p_path = &p_kdrv_cfg->out_img_info[i];

		dump("%4d %4d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d\r\n", i,
					(int)p_path->out_dup_num,
					(int)p_path->res_dim_info.des_width, (int)p_path->res_dim_info.des_height,
					(int)p_path->res_dim_info.des_out_x, (int)p_path->res_dim_info.des_out_y, (int)p_path->res_dim_info.des_out_w, (int)p_path->res_dim_info.des_out_h,
					(int)p_path->res_dim_info.des_rlt_x, (int)p_path->res_dim_info.des_rlt_y, (int)p_path->res_dim_info.des_rlt_w, (int)p_path->res_dim_info.des_rlt_h,
					(int)p_path->res_dim_info.hole_x, (int)p_path->res_dim_info.hole_y, (int)p_path->res_dim_info.hole_w, (int)p_path->res_dim_info.hole_h);
	}

#if 0
	dump("%4s %12s %12s %12s\r\n", "col", "proc_w", "proc_st_x", "col_st_x");
	for (i = 0; i <= p_kdrv_cfg->col_info.col_num; i++) {
		dump("%4d %12d %12d %12d\r\n", i,
			(int)p_kdrv_cfg->col_info.col_size_info[i].proc_width,
			(int)p_kdrv_cfg->col_info.col_size_info[i].proc_x_start,
			(int)p_kdrv_cfg->col_info.col_size_info[i].col_x_start);
	}
	dump("\r\n");
#endif

	if (p_kdrv_cfg->func_mode & KDRV_VPE_FUNC_SHP_EN) {
		CHAR str_mem[128];
		UINT32 str_len;

		dump("\r\nsharpen param\r\n");
		dump("%8s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %8s\r\n",
					"src_sel", "weight_th", "weight_gain", "noise_lv", "inv_gamma",
					"edge_str1", "edge_str2", "flat_str", "coring_th",
					"b_halo_clip", "d_hal_clip", "out_sel");
		dump("%8d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %8d\r\n",
					(int)p_kdrv_cfg->sharpen_param.edge_weight_src_sel,
					(int)p_kdrv_cfg->sharpen_param.edge_weight_th,
					(int)p_kdrv_cfg->sharpen_param.edge_weight_gain,
					(int)p_kdrv_cfg->sharpen_param.noise_level,
					(int)p_kdrv_cfg->sharpen_param.blend_inv_gamma,
					(int)p_kdrv_cfg->sharpen_param.edge_sharp_str1,
					(int)p_kdrv_cfg->sharpen_param.edge_sharp_str2,
					(int)p_kdrv_cfg->sharpen_param.flat_sharp_str,
					(int)p_kdrv_cfg->sharpen_param.coring_th,
					(int)p_kdrv_cfg->sharpen_param.bright_halo_clip,
					(int)p_kdrv_cfg->sharpen_param.dark_halo_clip,
					(int)p_kdrv_cfg->sharpen_param.sharpen_out_sel);
		dump("\r\nnoise_curve: ");
		str_len = 0;
		for (i = 0; i < KDRV_VPE_NOISE_CURVE_NUMS; i++) {
			str_len += snprintf(str_mem + str_len, 128 - str_len, "%d, ", (int)p_kdrv_cfg->sharpen_param.noise_curve[i]);
		}
		dump("%s\r\n\r\n", str_mem);
	}

	if (p_kdrv_cfg->func_mode & KDRV_VPE_FUNC_DCE_EN) {
		dump("\r\ndce param\r\n");
		dump("%8s %12s %10s %10s %12s %12s %8s %8s %8s %8s %8s %8s%10s\r\n",
					"mode", "lsb_rand", "in_width", "in_height", "out_width", "out_height", "radius",
					"lut_sz", "xofs_i", "xofs_f", "yofs_i", "yofs_f", "lut_addr");
		dump("%8d %12d %10d %10d %12d %12d %8d %8d %8d %8d %8d %8d 0x%.8x\r\n",
					(int)p_kdrv_cfg->dce_param.dce_mode,
					(int)p_kdrv_cfg->dce_param.lsb_rand,
					(int)p_kdrv_cfg->dce_param.dewarp_in_width,
					(int)p_kdrv_cfg->dce_param.dewarp_in_height,
					(int)p_kdrv_cfg->dce_param.dewarp_out_width,
					(int)p_kdrv_cfg->dce_param.dewarp_out_height,
					(int)p_kdrv_cfg->dce_param.lens_radius,
					(int)p_kdrv_cfg->dce_param.lut2d_sz,
					(int)p_kdrv_cfg->dce_param.xofs_i,
					(int)p_kdrv_cfg->dce_param.xofs_f,
					(int)p_kdrv_cfg->dce_param.yofs_i,
					(int)p_kdrv_cfg->dce_param.yofs_f,
					(unsigned int)p_kdrv_cfg->dce_param.dce_l2d_addr);

		dump("\r\n%12s %8s %8s %8s %8s %8s %8s %8s %8s\r\n",
					"fovbound", "boundy", "boundu", "boundv", "cent_x", "cent_y",
					"xdist", "ydist", "fovgain");
		dump("%12d %8d %8d %8d %8d %8d %8d %8d %8d\r\n\r\n",
					(int)p_kdrv_cfg->dce_param.fovbound,
					(int)p_kdrv_cfg->dce_param.boundy,
					(int)p_kdrv_cfg->dce_param.boundu,
					(int)p_kdrv_cfg->dce_param.boundv,
					(int)p_kdrv_cfg->dce_param.cent_x_s,
					(int)p_kdrv_cfg->dce_param.cent_y_s,
					(int)p_kdrv_cfg->dce_param.xdist_a1,
					(int)p_kdrv_cfg->dce_param.ydist_a1,
					(int)p_kdrv_cfg->dce_param.fovgain);
	}

	if (p_kdrv_cfg->func_mode & KDRV_VPE_FUNC_DCTG_EN) {
		dump("\r\ndctg param\r\n");
		dump("%12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s\r\n",
					"mount_type", "ra_en", "lut2d_sz", "lens_r", "lens_x_st",
					"lens_y_st", "theta_st", "theta_ed", "phi_st",
					"phi_ed", "rot_z", "rot_y");
		dump("%12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d\r\n",
					(int)p_kdrv_cfg->dctg_param.mount_type,
					(int)p_kdrv_cfg->dctg_param.ra_en,
					(int)p_kdrv_cfg->dctg_param.lut2d_sz,
					(int)p_kdrv_cfg->dctg_param.lens_r,
					(int)p_kdrv_cfg->dctg_param.lens_x_st,
					(int)p_kdrv_cfg->dctg_param.lens_y_st,
					(int)p_kdrv_cfg->dctg_param.theta_st,
					(int)p_kdrv_cfg->dctg_param.theta_ed,
					(int)p_kdrv_cfg->dctg_param.phi_st,
					(int)p_kdrv_cfg->dctg_param.phi_ed,
					(int)p_kdrv_cfg->dctg_param.rot_z,
					(int)p_kdrv_cfg->dctg_param.rot_y);
	}
}

UINT32 ctl_vpe_process_dbg_dump_cfg(UINT32 op, UINT32 val)
{
	static UINT32 dump_cnt = 0;
	UINT32 rt = 0;

	if (op == 0) {
		rt = dump_cnt;
		if (dump_cnt > 0) {
			dump_cnt--;
		}
	} else {
		dump_cnt += val;
	}

	return rt;
}

static INT32 ctl_vpe_process_alloc_outpath_buf(CTL_VPE_BASEINFO *p_info, CTL_VPE_JOB *p_job, CTL_VPE_EVENT_FP bufio_fp)
{
	VDO_FRAME *p_in_frm;
	UINT32 alloc_success_cnt;
	UINT32 i;
	UINT32 bg_height;

	if (bufio_fp == NULL) {
		DBG_WRN("null out bufio callback\r\n");
		return CTL_VPE_E_SYS;
	}

	alloc_success_cnt = 0;
	p_in_frm = (VDO_FRAME *)p_job->in_evt.data_addr;
	for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
		CTL_VPE_OUT_PATH *p_path = &p_info->out_path[i];
		CTL_VPE_OUT_BUF_INFO *p_buf = &p_job->buf_info[i];

		memset((void *)p_buf, 0, sizeof(CTL_VPE_OUT_BUF_INFO));
		p_buf->pid = i;

		if (!p_path->enable) {
			continue;
		}

		if (p_info->out_path_h_align[i] == 0) {
			bg_height = p_path->bg_size.h;
		} else {
			bg_height = ALIGN_CEIL(p_path->bg_size.h, p_info->out_path_h_align[i]);
		}

		p_buf->buf_size = ctl_vpe_util_yuv_size(p_path->fmt, p_path->bg_size.w, bg_height);
		if (p_buf->buf_size == 0) {
			continue;
		}
		p_buf->vdo_frm.count = p_in_frm->count;
		p_buf->vdo_frm.timestamp = p_in_frm->timestamp;
		ctl_vpe_outbuf_cb_wrapper(bufio_fp, p_buf, CTL_VPE_BUF_NEW);
		if (p_buf->buf_addr) {
			alloc_success_cnt++;

			p_buf->vdo_frm.sign = MAKEFOURCC('V', 'F', 'R', 'M');
			p_buf->vdo_frm.pxlfmt = p_path->fmt;
			p_buf->vdo_frm.size.w = p_path->bg_size.w;
			p_buf->vdo_frm.size.h = p_path->bg_size.h;
			p_buf->vdo_frm.addr[0] = p_buf->buf_addr;

			p_buf->vdo_frm.pw[VDO_PINDEX_Y] = p_buf->vdo_frm.size.w;
			p_buf->vdo_frm.pw[VDO_PINDEX_U] = ctl_vpe_util_y2uvwidth(p_buf->vdo_frm.pxlfmt, p_buf->vdo_frm.size.w);
			p_buf->vdo_frm.pw[VDO_PINDEX_V] = p_buf->vdo_frm.pw[VDO_PINDEX_U];

			p_buf->vdo_frm.ph[VDO_PINDEX_Y] = p_buf->vdo_frm.size.h;
			p_buf->vdo_frm.ph[VDO_PINDEX_U] = ctl_vpe_util_y2uvheight(p_buf->vdo_frm.pxlfmt, p_buf->vdo_frm.size.h);
			p_buf->vdo_frm.ph[VDO_PINDEX_V] = p_buf->vdo_frm.ph[VDO_PINDEX_U];

			if (VDO_PXLFMT_CLASS(p_buf->vdo_frm.pxlfmt) == VDO_PXLFMT_CLASS_NVX) {
				if (p_buf->vdo_frm.pxlfmt != VDO_PXLFMT_YUV420_NVX2) {
					DBG_ERR("Unsupport yuv format 0x%.8x\r\n", (unsigned int)p_buf->vdo_frm.pxlfmt);
				}

				p_buf->vdo_frm.loff[VDO_PINDEX_Y] = ALIGN_CEIL(p_buf->vdo_frm.size.w, 16) * 3 / 4;
				p_buf->vdo_frm.loff[VDO_PINDEX_U] = p_buf->vdo_frm.loff[VDO_PINDEX_Y];
				p_buf->vdo_frm.loff[VDO_PINDEX_V] = p_buf->vdo_frm.loff[VDO_PINDEX_U];

				p_buf->vdo_frm.addr[VDO_PINDEX_Y] = p_buf->buf_addr;
				p_buf->vdo_frm.addr[VDO_PINDEX_U] = p_buf->vdo_frm.addr[VDO_PINDEX_Y] + (p_buf->vdo_frm.loff[VDO_PINDEX_Y] * bg_height);
				p_buf->vdo_frm.addr[VDO_PINDEX_V] = p_buf->vdo_frm.addr[VDO_PINDEX_U];
			} else {
				p_buf->vdo_frm.loff[VDO_PINDEX_Y] = p_buf->vdo_frm.size.w;
				p_buf->vdo_frm.loff[VDO_PINDEX_U] = ctl_vpe_util_y2uvlof(p_buf->vdo_frm.pxlfmt, p_buf->vdo_frm.loff[VDO_PINDEX_Y]);
				p_buf->vdo_frm.loff[VDO_PINDEX_V] = p_buf->vdo_frm.loff[VDO_PINDEX_U];

				p_buf->vdo_frm.addr[VDO_PINDEX_Y] = p_buf->buf_addr;
				p_buf->vdo_frm.addr[VDO_PINDEX_U] = p_buf->vdo_frm.addr[VDO_PINDEX_Y] + (p_buf->vdo_frm.loff[VDO_PINDEX_Y] * bg_height);
				p_buf->vdo_frm.addr[VDO_PINDEX_V] = p_buf->vdo_frm.addr[VDO_PINDEX_U];
			}



			if (p_info->out_path_h_align[i] > 0) {
				p_buf->vdo_frm.reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N');
				p_buf->vdo_frm.reserved[1] = bg_height;
			}
		}
	}

	if (alloc_success_cnt == 0) {
		/* all path have no buffer, return failed and not trigger engine */
		return CTL_VPE_E_NOMEM;
	}

	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_process_config_dce(KDRV_VPE_CONFIG *p_kdrv_cfg, CTL_VPE_ISP_PARAM *p_iq, USIZE dce_out_sz)
{
	/* force disable 2d_lut_en when flip/rot func enable */
	p_kdrv_cfg->dce_param.rot = p_iq->flip_rot_ctl.flip_rot_mode;
	if (p_iq->flip_rot_ctl.flip_rot_mode != CTL_VPE_ISP_ROTATE_0) {
		if (p_iq->dce_ctl.enable == TRUE) {
			DBG_ERR("force disable 2dlut when flip/rot on \r\n");
		}
		p_kdrv_cfg->dce_param.dce_mode = (UINT8)CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY;
		p_kdrv_cfg->dce_param.dce_2d_lut_en = FALSE;

		if (p_iq->flip_rot_ctl.flip_rot_mode == CTL_VPE_ISP_ROTATE_MANUAL) {
			p_kdrv_cfg->dce_param.rot_param.rad = p_iq->flip_rot_ctl.rot_manual_param.rot_degree * CTL_VPE_ISP_ANGLE_PARSE_VALUE / CTL_VPE_ISP_ROT_MAX_ANGLE;
			p_kdrv_cfg->dce_param.rot_param.flip = p_iq->flip_rot_ctl.rot_manual_param.flip;
			p_kdrv_cfg->dce_param.rot_param.ratio_mode = p_iq->flip_rot_ctl.rot_manual_param.ratio_mode;
			p_kdrv_cfg->dce_param.rot_param.ratio = p_iq->flip_rot_ctl.rot_manual_param.ratio;
			//bg color setting
			p_kdrv_cfg->dce_param.fovbound = p_iq->flip_rot_ctl.rot_manual_param.fovbound;
			p_kdrv_cfg->dce_param.boundy = p_iq->flip_rot_ctl.rot_manual_param.boundy;
			p_kdrv_cfg->dce_param.boundu = p_iq->flip_rot_ctl.rot_manual_param.boundu;
			p_kdrv_cfg->dce_param.boundv = p_iq->flip_rot_ctl.rot_manual_param.boundv;
		}
		return CTL_VPE_E_OK;
	}

	if (p_iq->dce_ctl.enable == FALSE) {
		return CTL_VPE_E_OK;
	}

	if (p_iq->dce_ctl.dce_mode >= CTL_VPE_ISP_DCE_MODE_MAX) {
		DBG_WRN("unknown dce mode\r\n");
		return CTL_VPE_E_PAR;
	}

	if (p_iq->dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_DCTG) {
		p_kdrv_cfg->dce_param.dce_mode = (UINT8)CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY;
	} else {
		p_kdrv_cfg->dce_param.dce_mode = (UINT8)p_iq->dce_ctl.dce_mode;
	}
	p_kdrv_cfg->dce_param.lsb_rand = p_iq->dce_ctl.lsb_rand;

	if (p_iq->dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_GDC_ONLY) {
		p_kdrv_cfg->dce_param.fovbound = p_iq->dce_gdc_param.fovbound;
		p_kdrv_cfg->dce_param.boundy = p_iq->dce_gdc_param.boundy;
		p_kdrv_cfg->dce_param.boundu = p_iq->dce_gdc_param.boundu;
		p_kdrv_cfg->dce_param.boundv = p_iq->dce_gdc_param.boundv;
		p_kdrv_cfg->dce_param.xdist_a1 = p_iq->dce_gdc_param.xdist_a1;
		p_kdrv_cfg->dce_param.ydist_a1 = p_iq->dce_gdc_param.ydist_a1;
		p_kdrv_cfg->dce_param.fovgain = p_iq->dce_gdc_param.fovgain;
		memcpy((void *)&p_kdrv_cfg->dce_param.geo_lut[0], (void *)&p_iq->dce_gdc_param.geo_lut[0], sizeof(p_kdrv_cfg->dce_param.geo_lut));

		if (p_iq->dce_gdc_param.cent_ratio_base) {
			UINT32 in_w = p_kdrv_cfg->glb_img_info.src_in_w;
			UINT32 in_h = p_kdrv_cfg->glb_img_info.src_in_h;

			p_kdrv_cfg->dce_param.cent_x_s = in_w * p_iq->dce_gdc_param.cent_x_ratio / p_iq->dce_gdc_param.cent_ratio_base;
			p_kdrv_cfg->dce_param.cent_y_s = in_h * p_iq->dce_gdc_param.cent_y_ratio / p_iq->dce_gdc_param.cent_ratio_base;
		} else {
			DBG_WRN("dce cent ratio 0\r\n");
			return CTL_VPE_E_PAR;
		}
	}

	if (p_iq->dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY) {
		if (p_iq->dce_2dlut_param.p_lut) {
			p_kdrv_cfg->dce_param.dce_2d_lut_en = 1;
			p_kdrv_cfg->dce_param.lut2d_sz = p_iq->dce_2dlut_param.lut_sz;
			p_kdrv_cfg->dce_param.xofs_i = p_iq->dce_2dlut_param.xofs_i;
			p_kdrv_cfg->dce_param.xofs_f = p_iq->dce_2dlut_param.xofs_f;
			p_kdrv_cfg->dce_param.yofs_i = p_iq->dce_2dlut_param.yofs_i;
			p_kdrv_cfg->dce_param.yofs_f = p_iq->dce_2dlut_param.yofs_f;
			p_kdrv_cfg->dce_param.dce_l2d_addr = vos_cpu_get_phy_addr((UINT32)p_iq->dce_2dlut_param.p_lut);
			/* 2dlut out size */
			p_kdrv_cfg->dce_param.dewarp_out_width = dce_out_sz.w;
			p_kdrv_cfg->dce_param.dewarp_out_height = dce_out_sz.h;
			if (p_kdrv_cfg->dce_param.dce_l2d_addr == VOS_ADDR_INVALID) {
				DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)p_iq->dce_2dlut_param.p_lut);
				return CTL_VPE_E_NOMEM;
			}
		} else {
			DBG_WRN("dce 2dlut null\r\n");
			return CTL_VPE_E_PAR;
		}
	} else if (p_iq->dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_DCTG) {
			p_kdrv_cfg->dce_param.dce_2d_lut_en = 0;
			p_kdrv_cfg->dce_param.lut2d_sz = p_iq->dctg_param.lut2d_sz;
			p_kdrv_cfg->dce_param.xofs_i = p_iq->dce_2dlut_param.xofs_i;
			p_kdrv_cfg->dce_param.xofs_f = p_iq->dce_2dlut_param.xofs_f;
			p_kdrv_cfg->dce_param.yofs_i = p_iq->dce_2dlut_param.yofs_i;
			p_kdrv_cfg->dce_param.yofs_f = p_iq->dce_2dlut_param.yofs_f;
			p_kdrv_cfg->dce_param.dewarp_out_width = dce_out_sz.w;
			p_kdrv_cfg->dce_param.dewarp_out_height = dce_out_sz.h;
	}

	/* set distortion lens radius from iq */
	if (p_iq->dce_ctl.lens_radius == 0) {
		p_kdrv_cfg->dce_param.lens_radius = p_kdrv_cfg->glb_img_info.src_in_w >> 1;
	} else {
		p_kdrv_cfg->dce_param.lens_radius = p_iq->dce_ctl.lens_radius;
	}

	/* todo: check if removed */
	p_kdrv_cfg->dce_param.dewarp_in_width = p_kdrv_cfg->glb_img_info.src_in_w;
	p_kdrv_cfg->dce_param.dewarp_in_height = p_kdrv_cfg->glb_img_info.src_in_h;

	p_kdrv_cfg->func_mode |= KDRV_VPE_FUNC_DCE_EN;

	return CTL_VPE_E_OK;
}

void ctl_vpe_process_config_dcs(KDRV_VPE_CONFIG *p_kdrv_cfg, CTL_VPE_BASEINFO *ctl_info)
{
	p_kdrv_cfg->dce_param.dc_scenes_sel = ctl_info->dcs_mode;
}

INT32 ctl_vpe_process_config_dctg(KDRV_VPE_CONFIG *p_kdrv_cfg, CTL_VPE_ISP_PARAM *p_iq)
{
	if (p_iq->dctg_param.enable == FALSE) {
		return CTL_VPE_E_OK;
	}

	p_kdrv_cfg->dctg_param.mount_type = p_iq->dctg_param.mount_type;
	p_kdrv_cfg->dctg_param.ra_en = 0;
	p_kdrv_cfg->dctg_param.lut2d_sz = p_iq->dctg_param.lut2d_sz;
	p_kdrv_cfg->dctg_param.lens_r = p_iq->dctg_param.lens_r;
	p_kdrv_cfg->dctg_param.lens_x_st = p_iq->dctg_param.lens_x_st;
	p_kdrv_cfg->dctg_param.lens_y_st = p_iq->dctg_param.lens_y_st;
	p_kdrv_cfg->dctg_param.theta_st = p_iq->dctg_param.theta_st;
	p_kdrv_cfg->dctg_param.theta_ed = p_iq->dctg_param.theta_ed;
	p_kdrv_cfg->dctg_param.phi_st = p_iq->dctg_param.phi_st;
	p_kdrv_cfg->dctg_param.phi_ed = p_iq->dctg_param.phi_ed;
	p_kdrv_cfg->dctg_param.rot_z = p_iq->dctg_param.rot_z;
	p_kdrv_cfg->dctg_param.rot_y = p_iq->dctg_param.rot_y;

	p_kdrv_cfg->func_mode |= KDRV_VPE_FUNC_DCTG_EN;

	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_process_config_sharpen(KDRV_VPE_CONFIG *p_kdrv_cfg, CTL_VPE_ISP_PARAM *p_iq)
{
	UINT32 i;

	if (p_iq->sharpen.enable == FALSE) {
		return CTL_VPE_E_OK;
	}

	p_kdrv_cfg->sharpen_param.edge_weight_src_sel = p_iq->sharpen.edge_weight_src_sel;
	p_kdrv_cfg->sharpen_param.edge_weight_th = p_iq->sharpen.edge_weight_th;
	p_kdrv_cfg->sharpen_param.edge_weight_gain = p_iq->sharpen.edge_weight_gain;
	p_kdrv_cfg->sharpen_param.noise_level = p_iq->sharpen.noise_level;
	p_kdrv_cfg->sharpen_param.blend_inv_gamma = p_iq->sharpen.blend_inv_gamma;
	p_kdrv_cfg->sharpen_param.edge_sharp_str1 = p_iq->sharpen.edge_sharp_str1;
	p_kdrv_cfg->sharpen_param.edge_sharp_str2 = p_iq->sharpen.edge_sharp_str2;
	p_kdrv_cfg->sharpen_param.flat_sharp_str = p_iq->sharpen.flat_sharp_str;
	p_kdrv_cfg->sharpen_param.coring_th = p_iq->sharpen.coring_th;
	p_kdrv_cfg->sharpen_param.bright_halo_clip = p_iq->sharpen.bright_halo_clip;
	p_kdrv_cfg->sharpen_param.dark_halo_clip = p_iq->sharpen.dark_halo_clip;
	p_kdrv_cfg->sharpen_param.sharpen_out_sel = p_iq->sharpen.sharpen_out_sel;
	for (i = 0; i < KDRV_VPE_NOISE_CURVE_NUMS; i++) {
		p_kdrv_cfg->sharpen_param.noise_curve[i] = p_iq->sharpen.noise_curve[i];
	}

	p_kdrv_cfg->func_mode |= KDRV_VPE_FUNC_SHP_EN;

	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_process_config_user_in_2dlut(KDRV_VPE_CONFIG *p_kdrv_cfg, CTL_VPE_ISP_PARAM *p_iq, CTL_VPE_INT_DCE_USER_2DLUT_PARAM *user_in_2dlut)
{
	UINT32 lut_size;
	UINT32 user_in_va;

	if (user_in_2dlut->lut_paddr) {	//check user in lut address isn't 0
		if (p_iq->dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY) {	//dce mode shoult select 2dlut only mode
			switch (user_in_2dlut->lut_sz) {
			case CTL_VPE_ISP_2DLUT_SZ_9X9:
				lut_size = 12*9*4;
				p_kdrv_cfg->dce_param.lut2d_sz = user_in_2dlut->lut_sz;
				break;
			case CTL_VPE_ISP_2DLUT_SZ_65X65:
				lut_size = 68*65*4;
				p_kdrv_cfg->dce_param.lut2d_sz = user_in_2dlut->lut_sz;
				break;

			case CTL_VPE_ISP_2DLUT_SZ_129X129:
				lut_size = 132*129*4;
				p_kdrv_cfg->dce_param.lut2d_sz = user_in_2dlut->lut_sz;
				break;

			case CTL_VPE_ISP_2DLUT_SZ_257X257:
				lut_size = 260*257*4;
				p_kdrv_cfg->dce_param.lut2d_sz = user_in_2dlut->lut_sz;
				break;

			default:
				DBG_ERR("user_in 2dlut size error %d\r\n", user_in_2dlut->lut_sz);
				return CTL_VPE_E_PAR;
			}

			user_in_va = nvtmpp_sys_pa2va(user_in_2dlut->lut_paddr);
			if (user_in_va == 0) {
				DBG_ERR("paddr(0x%x) conv to va fail\r\n", user_in_2dlut->lut_paddr);
				return CTL_VPE_E_NOMEM;
			}
			vos_cpu_dcache_sync(user_in_va, ALIGN_CEIL(lut_size, VOS_ALIGN_BYTES), VOS_DMA_TO_DEVICE);
			p_kdrv_cfg->dce_param.dce_l2d_addr = user_in_2dlut->lut_paddr;
		} else {
			DBG_ERR("user_in 2dlut enable, isp dce_mode need select 2dlut_only (cur_mode: %d)\r\n", (int)p_iq->dce_ctl.dce_mode);
			return CTL_VPE_E_ISP_PAR;
		}
	}
	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_process_chk_input_lmt(VDO_FRAME *p_in_frm)
{
	if (p_in_frm == NULL)
		return CTL_VPE_E_PAR;

	if (p_in_frm->addr[0] == 0) {
		DBG_ERR("vdoin info error addr 0\r\n");
		return CTL_VPE_E_INDATA;
	}

	if (p_in_frm->size.w == 0 || p_in_frm->size.h == 0) {
		DBG_ERR("vdoin info error width/height = 0\r\n");
		return CTL_VPE_E_INDATA;
	}

	if (p_in_frm->loff[0] == 0) {
		DBG_ERR("vdoin info error loff = 0\r\n");
		return CTL_VPE_E_INDATA;
	}

	if (p_in_frm->pxlfmt != VDO_PXLFMT_YUV420 &&
		p_in_frm->pxlfmt != (VDO_PXLFMT_YUV420|VDO_PIX_YCC_BT601) &&
		p_in_frm->pxlfmt != (VDO_PXLFMT_YUV420|VDO_PIX_YCC_BT709) &&
		p_in_frm->pxlfmt != VDO_PXLFMT_YUV420_NVX2) {
		DBG_ERR("vdoin info error, vpe only support yuv420, cur fmt 0x%.8x\r\n", (unsigned int)p_in_frm->pxlfmt);
		return CTL_VPE_E_INDATA;
	}

	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_process_chk_kdrv_cfg_lmt(KDRV_VPE_CONFIG *p_cfg)
{
	UINT8 i;

	if (p_cfg->glb_img_info.src_in_w > 4096 || p_cfg->glb_img_info.src_in_h > 4096) {
		DBG_ERR("input size overflow (%d, %d) > (4096, 4096)\r\n", (int)p_cfg->glb_img_info.src_in_w, (int)p_cfg->glb_img_info.src_in_h);
		return CTL_VPE_E_PAR;
	}

	for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {
		KDRV_VPE_OUT_IMG_INFO *p_out = &p_cfg->out_img_info[i];

		if (p_out->out_dup_num == 0) {
			continue;
		}

		if (p_out->res_dim_info.sca_crop_w > 4096 || p_out->res_dim_info.sca_crop_h > 4096) {
			DBG_ERR("pre-scale crop size overflow (%d, %d) > (4096, 4096)\r\n", (int)p_out->res_dim_info.sca_crop_w, (int)p_out->res_dim_info.sca_crop_h);
			return CTL_VPE_E_PAR;
		}

		if (p_out->res_dim_info.sca_width > 4096 || p_out->res_dim_info.sca_height > 4096) {
			DBG_ERR("scale size overflow (%d, %d) > (4096, 4096)\r\n", (int)p_out->res_dim_info.sca_width, (int)p_out->res_dim_info.sca_height);
			return CTL_VPE_E_PAR;
		}

		if (p_out->res_dim_info.des_crop_out_w > 4096 || p_out->res_dim_info.des_crop_out_h > 4096) {
			DBG_ERR("post-scale crop size overflow (%d, %d) > (4096, 4096)\r\n", (int)p_out->res_dim_info.des_crop_out_w, (int)p_out->res_dim_info.des_crop_out_h);
			return CTL_VPE_E_PAR;
		}

		if (p_out->res_dim_info.des_rlt_w > 4096 || p_out->res_dim_info.des_rlt_h > 4096) {
			DBG_ERR("rltwin size overflow (%d, %d) > (4096, 4096)\r\n", (int)p_out->res_dim_info.des_rlt_w, (int)p_out->res_dim_info.des_rlt_h);
			return CTL_VPE_E_PAR;
		}

		if (p_out->res_dim_info.des_out_w > 4096 || p_out->res_dim_info.des_out_h > 4096) {
			DBG_ERR("outwin size overflow (%d, %d) > (4096, 4096)\r\n", (int)p_out->res_dim_info.des_out_w, (int)p_out->res_dim_info.des_out_h);
			return CTL_VPE_E_PAR;
		}

		if (p_out->res_dim_info.hole_w > 4096 || p_out->res_dim_info.hole_h > 4096) {
			DBG_ERR("hole size overflow (%d, %d) > (4096, 4096)\r\n", (int)p_out->res_dim_info.hole_w, (int)p_out->res_dim_info.hole_h);
			return CTL_VPE_E_PAR;
		}
	}

	return CTL_VPE_E_OK;
}

void ctl_vpe_process_config_dce_out_size(CTL_VPE_HANDLE *p_hdl, CTL_VPE_BASEINFO *p_info)
{
	UINT32 i;
	USIZE max_out_sz = {0};	//all path max out scale size
	USIZE min_out_sz = {0xFFFFFFFF, 0xFFFFFFFF};	//all path min out scale size
	USIZE isp_dce_out_sz = {0};

	if (p_hdl->isp_param.dce_ctl.enable) {
		if (p_hdl->isp_param.dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY) {
			isp_dce_out_sz.w = p_hdl->isp_param.dce_2dlut_param.out_size.w;
			isp_dce_out_sz.h = p_hdl->isp_param.dce_2dlut_param.out_size.h;
		}
		if (p_hdl->isp_param.dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_DCTG) {
			isp_dce_out_sz.w = p_hdl->isp_param.dctg_param.out_size.w;
			isp_dce_out_sz.h = p_hdl->isp_param.dctg_param.out_size.h;
		}
	}

	if (isp_dce_out_sz.w != 0 && isp_dce_out_sz.h != 0) {
		for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
			if (p_info->out_path[i].enable) {
				//user set pre crop, force set dce out = dce in(dc_in = dc_out = pre_crop)
				if (p_info->out_path[i].pre_scl_crop.x != 0 || p_info->out_path[i].pre_scl_crop.y != 0 ||
					p_info->out_path[i].pre_scl_crop.w != 0 || p_info->out_path[i].pre_scl_crop.h != 0) {
						p_info->dce_out_sz.w = 0;
						p_info->dce_out_sz.h = 0;
						return;
				}
				//when scale size == 0, force set dce out = dce in(dc_in = dc_out = scale)
				if (p_info->out_path[i].scl_size.w == 0 || p_info->out_path[i].scl_size.h == 0) {
					p_info->dce_out_sz.w = 0;
					p_info->dce_out_sz.h = 0;
					return;
				}

				max_out_sz.w = max(p_info->out_path[i].scl_size.w, max_out_sz.w);
				max_out_sz.h = max(p_info->out_path[i].scl_size.h, max_out_sz.h);

				min_out_sz.w = min(p_info->out_path[i].scl_size.w, min_out_sz.w);
				min_out_sz.h = min(p_info->out_path[i].scl_size.h, min_out_sz.h);
			}
		}

		//replace dce out for keep max out size quality
		//vpe scale limit (in-1)/(out-1) < 15.99 or > 1/15.99
		if (max_out_sz.w == 0) {
			p_info->dce_out_sz.w = isp_dce_out_sz.w;
		} else if ((max_out_sz.w-1) > ((min_out_sz.w-1)*CTL_VPE_SCALE_RATIO_MAX * 2 / CTL_VPE_SCALE_RATIO_BASE)) {
			ctl_vpe_dbg_err("dce out %d, scale max/min (%d, %d), overflow\r\n", (int)isp_dce_out_sz.w, (int)max_out_sz.w, (int)min_out_sz.w);
			p_info->dce_out_sz.w = isp_dce_out_sz.w;
		} else {
			p_info->dce_out_sz.w = min(max_out_sz.w, ALIGN_FLOOR((min_out_sz.w-1) * CTL_VPE_SCALE_RATIO_MAX / CTL_VPE_SCALE_RATIO_BASE, 8));
		}

		if (max_out_sz.h == 0) {
			p_info->dce_out_sz.h = isp_dce_out_sz.h;
		} else if ((max_out_sz.h-1) > ((min_out_sz.h-1)*CTL_VPE_SCALE_RATIO_MAX * 2 / CTL_VPE_SCALE_RATIO_BASE)) {
			ctl_vpe_dbg_err("dce out %d, scale max/min (%d, %d), overflow\r\n", (int)isp_dce_out_sz.h, (int)max_out_sz.h, (int)min_out_sz.h);
			p_info->dce_out_sz.h = isp_dce_out_sz.h;
		} else {
			p_info->dce_out_sz.h = min(max_out_sz.h, ALIGN_FLOOR(((min_out_sz.h-1) * CTL_VPE_SCALE_RATIO_MAX / CTL_VPE_SCALE_RATIO_BASE), 2));
		}
	} else {
		p_info->dce_out_sz.w = 0;
		p_info->dce_out_sz.h = 0;
	}
}

INT32 ctl_vpe_process_d2d(CTL_VPE_HANDLE *p_hdl, CTL_VPE_JOB *p_job, void *cb_fp)
{
	static CTL_VPE_BASEINFO m_baseinfo;
	unsigned long loc_flg;
	VDO_FRAME *p_in_frm;
	CTL_VPE_BASEINFO *p_info;
	KDRV_VPE_CONFIG *p_kdrv_cfg;
	KDRV_VPE_JOB_LIST *p_kdrv_job;
	CTL_VPE_INT_DCE_USER_2DLUT_PARAM user_2dlut_param = {0};
	CTL_VPE_ISP_CB_PARAM isp_cb_param = {0};
	UINT32 i;
	INT32 rt;

	/* parse input frame info and ctl info to kdrv struct */
	p_in_frm = (VDO_FRAME *)p_job->in_evt.data_addr;
	/* parsing user in 2dlut parameters */
	user_2dlut_param.lut_sz = p_in_frm->reserved[2] & 0xf;
	user_2dlut_param.lut_paddr = p_in_frm->reserved[3];

	p_kdrv_cfg = &p_job->kdrv_cfg;
	p_kdrv_job = &p_job->kdrv_job;
	memset((void *)p_kdrv_cfg, 0, sizeof(KDRV_VPE_CONFIG));
	memset((void *)p_kdrv_job, 0, sizeof(KDRV_VPE_JOB_LIST));

	if(p_job->p_dbg_node) {
		p_job->p_dbg_node->handle = (UINT32)p_hdl;
		p_job->p_dbg_node->input_frm_count = p_in_frm->count;
	}

	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	m_baseinfo = p_hdl->ctl_info;
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
	p_info = &m_baseinfo;

	/* check header info */
	if (ctl_vpe_process_chk_input_lmt(p_in_frm) != CTL_VPE_E_OK) {
		atomic_inc(&p_hdl->dbg_err_accu_info.err_indata);
		return CTL_VPE_E_INDATA;
	}

	/* alloc buffer */
	rt = ctl_vpe_process_alloc_outpath_buf(p_info, p_job, p_hdl->cb_fp[CTL_VPE_CBEVT_OUT_BUF]);
	if (rt != CTL_VPE_E_OK) {
		DBG_IND("alloc out path buffer failed\r\n");
		atomic_inc(&p_hdl->dbg_err_accu_info.err_nomem);
		return rt;
	}

	if (p_info->in_crop.mode == CTL_VPE_IN_CROP_AUTO) {
		/* crop window from sie, crop info from vdo_frm.reserved[5][6] */
		isp_cb_param.dce_in_size.w = (UINT32)((p_in_frm->reserved[6] & 0xffff0000) >> 16);
		isp_cb_param.dce_in_size.h = (UINT32)((p_in_frm->reserved[6] & 0x0000ffff));
	} else if (p_info->in_crop.mode == CTL_VPE_IN_CROP_USER) {
		isp_cb_param.dce_in_size.w = p_info->in_crop.crp_window.w;
		isp_cb_param.dce_in_size.h = p_info->in_crop.crp_window.h;
	} else { // if (p_info->in_crop.mode == CTL_VPE_IN_CROP_NONE) {
		isp_cb_param.dce_in_size.w = p_in_frm->size.w;
		isp_cb_param.dce_in_size.h = p_in_frm->size.h;
	}
	/* trig and get iq param */
	rt = ctl_vpe_isp_event_cb(p_hdl->isp_id, ISP_EVENT_VPE_CFG_IMM, p_job, &isp_cb_param);
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("isp callback failed\r\n");
		atomic_inc(&p_hdl->dbg_err_accu_info.err_sys);
		return rt;
	}

	/* default config to none */
	p_kdrv_cfg->func_mode = KDRV_VPE_FUNC_NONE_EN;

	/* input frame, set src image info back to ctl_info for debug
		vpe input width should use loff, width is used at src_roi
	*/
	p_kdrv_cfg->glb_img_info.src_addr = vos_cpu_get_phy_addr(p_in_frm->addr[0]);
	if (p_kdrv_cfg->glb_img_info.src_addr == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)p_in_frm->addr[0]);
		atomic_inc(&p_hdl->dbg_err_accu_info.err_indata);
		return CTL_VPE_E_NOMEM;
	}
	p_kdrv_cfg->glb_img_info.src_width = p_in_frm->loff[0];
	p_kdrv_cfg->glb_img_info.src_height = p_in_frm->size.h;
	p_hdl->ctl_info.src_img.size.w = p_kdrv_cfg->glb_img_info.src_width;
	p_hdl->ctl_info.src_img.size.h = p_kdrv_cfg->glb_img_info.src_height;
	if (p_in_frm->reserved[0] == MAKEFOURCC('H', 'A', 'L', 'N')) {
		/* check input buffer height alignment */
		p_kdrv_cfg->glb_img_info.src_height = p_in_frm->reserved[1];
		p_hdl->ctl_info.src_img.src_img_h_align = p_kdrv_cfg->glb_img_info.src_height;
	} else {
		p_hdl->ctl_info.src_img.src_img_h_align = 0;
	}
	p_kdrv_cfg->glb_img_info.src_drt = ctl_vpe_typecast_src_drt(p_in_frm->pxlfmt);
	if (p_info->in_crop.mode == CTL_VPE_IN_CROP_AUTO) {
		/* crop window from sie */
		p_kdrv_cfg->glb_img_info.src_in_x = (p_in_frm->reserved[5] & 0xffff0000) >> 16;
		p_kdrv_cfg->glb_img_info.src_in_y = (p_in_frm->reserved[5] & 0x0000ffff);
		p_kdrv_cfg->glb_img_info.src_in_w = (p_in_frm->reserved[6] & 0xffff0000) >> 16;
		p_kdrv_cfg->glb_img_info.src_in_h = (p_in_frm->reserved[6] & 0x0000ffff);
	} else if (p_info->in_crop.mode == CTL_VPE_IN_CROP_NONE) {
		p_kdrv_cfg->glb_img_info.src_in_x = 0;
		p_kdrv_cfg->glb_img_info.src_in_y = 0;
		p_kdrv_cfg->glb_img_info.src_in_w = p_in_frm->size.w;
		p_kdrv_cfg->glb_img_info.src_in_h = p_in_frm->size.h;
	} else if (p_info->in_crop.mode == CTL_VPE_IN_CROP_USER) {
		p_kdrv_cfg->glb_img_info.src_in_x = p_info->in_crop.crp_window.x;
		p_kdrv_cfg->glb_img_info.src_in_y = p_info->in_crop.crp_window.y;
		p_kdrv_cfg->glb_img_info.src_in_w = p_info->in_crop.crp_window.w;
		p_kdrv_cfg->glb_img_info.src_in_h = p_info->in_crop.crp_window.h;
	} else {
		DBG_ERR("Unknown crop mode\r\n");
		atomic_inc(&p_hdl->dbg_err_accu_info.err_indata);
		return CTL_VPE_E_PAR;
	}
	p_hdl->ctl_info.in_crop.crp_window.x = p_kdrv_cfg->glb_img_info.src_in_x;
	p_hdl->ctl_info.in_crop.crp_window.y = p_kdrv_cfg->glb_img_info.src_in_y;
	p_hdl->ctl_info.in_crop.crp_window.w = p_kdrv_cfg->glb_img_info.src_in_w;
	p_hdl->ctl_info.in_crop.crp_window.h = p_kdrv_cfg->glb_img_info.src_in_h;
	ctl_vpe_process_config_dce_out_size(p_hdl, p_info);

	for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {
		CTL_VPE_OUT_BUF_INFO *p_buf = &p_job->buf_info[i];
		CTL_VPE_OUT_PATH *p_ctl_out = &p_info->out_path[i];
		KDRV_VPE_OUT_IMG_INFO *p_kdrv_out = &p_kdrv_cfg->out_img_info[i];

		if (p_ctl_out->enable && p_buf->vdo_frm.addr[0]) {
			p_kdrv_out->out_dup_num = 1;
			p_kdrv_out->out_dup_info.out_addr = vos_cpu_get_phy_addr(p_buf->vdo_frm.addr[0]);
			if (p_kdrv_out->out_dup_info.out_addr == VOS_ADDR_INVALID) {
				DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)p_buf->vdo_frm.addr[0]);
				atomic_inc(&p_hdl->dbg_err_accu_info.err_nomem);
				return CTL_VPE_E_NOMEM;
			}
			p_kdrv_out->out_dup_info.des_type = ctl_vpe_typecast_out_fmt(p_ctl_out->fmt);

			/*pre sca crop window(0, 0, 0, 0) -> no crop */
			if (p_ctl_out->pre_scl_crop.x == 0 &&
				p_ctl_out->pre_scl_crop.y == 0 &&
				p_ctl_out->pre_scl_crop.w == 0 &&
				p_ctl_out->pre_scl_crop.h == 0) {
				p_kdrv_out->res_dim_info.sca_crop_x = 0;
				p_kdrv_out->res_dim_info.sca_crop_y = 0;
				if ((p_hdl->isp_param.dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY || p_hdl->isp_param.dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_DCTG) &&
					p_info->dce_out_sz.w != 0 && p_info->dce_out_sz.h != 0) {
					/* set size to 2dlut out when 2dlut out mode */
					p_kdrv_out->res_dim_info.sca_crop_w = p_info->dce_out_sz.w;
					p_kdrv_out->res_dim_info.sca_crop_h = p_info->dce_out_sz.h;
	  			} else {
					p_kdrv_out->res_dim_info.sca_crop_w = p_kdrv_cfg->glb_img_info.src_in_w;
					p_kdrv_out->res_dim_info.sca_crop_h = p_kdrv_cfg->glb_img_info.src_in_h;
				}
			} else {
				p_kdrv_out->res_dim_info.sca_crop_x = p_ctl_out->pre_scl_crop.x;
				p_kdrv_out->res_dim_info.sca_crop_y = p_ctl_out->pre_scl_crop.y;
				p_kdrv_out->res_dim_info.sca_crop_w = p_ctl_out->pre_scl_crop.w;
				p_kdrv_out->res_dim_info.sca_crop_h = p_ctl_out->pre_scl_crop.h;
			}

			/* scl size */
			p_kdrv_out->res_dim_info.sca_width = p_ctl_out->scl_size.w;
			p_kdrv_out->res_dim_info.sca_height = p_ctl_out->scl_size.h;

			/* des crop window(0, 0, 0, 0) -> no crop, set size as scale size */
			if (p_ctl_out->post_scl_crop.x == 0 &&
				p_ctl_out->post_scl_crop.y == 0 &&
				p_ctl_out->post_scl_crop.w == 0 &&
				p_ctl_out->post_scl_crop.h == 0) {
				p_kdrv_out->res_dim_info.des_crop_out_x = 0;
				p_kdrv_out->res_dim_info.des_crop_out_y = 0;
				p_kdrv_out->res_dim_info.des_crop_out_w = p_kdrv_out->res_dim_info.sca_width;
				p_kdrv_out->res_dim_info.des_crop_out_h = p_kdrv_out->res_dim_info.sca_height;
			} else {
				p_kdrv_out->res_dim_info.des_crop_out_x = p_ctl_out->post_scl_crop.x;
				p_kdrv_out->res_dim_info.des_crop_out_y = p_ctl_out->post_scl_crop.y;
				p_kdrv_out->res_dim_info.des_crop_out_w = p_ctl_out->post_scl_crop.w;
				p_kdrv_out->res_dim_info.des_crop_out_h = p_ctl_out->post_scl_crop.h;
			}

			/* output window(background, output, rlt, hole */
			p_kdrv_out->res_dim_info.des_width = p_ctl_out->bg_size.w;
			p_kdrv_out->res_dim_info.des_height = p_ctl_out->bg_size.h;
			if (p_info->out_path_h_align[i] != 0) {
				p_kdrv_out->res_dim_info.des_height = ALIGN_CEIL(p_kdrv_out->res_dim_info.des_height, p_info->out_path_h_align[i]);
			}
			p_kdrv_out->res_dim_info.des_out_x = p_ctl_out->out_window.x;
			p_kdrv_out->res_dim_info.des_out_y = p_ctl_out->out_window.y;
			p_kdrv_out->res_dim_info.des_out_w = p_ctl_out->out_window.w;
			p_kdrv_out->res_dim_info.des_out_h = p_ctl_out->out_window.h;
			p_kdrv_out->res_dim_info.des_rlt_x = p_ctl_out->dst_pos.x;
			p_kdrv_out->res_dim_info.des_rlt_y = p_ctl_out->dst_pos.y;
			p_kdrv_out->res_dim_info.des_rlt_w = p_kdrv_out->res_dim_info.des_crop_out_w;
			p_kdrv_out->res_dim_info.des_rlt_h = p_kdrv_out->res_dim_info.des_crop_out_h;
			p_kdrv_out->res_dim_info.hole_x = p_ctl_out->hole_region.x;
			p_kdrv_out->res_dim_info.hole_y = p_ctl_out->hole_region.y;
			p_kdrv_out->res_dim_info.hole_w = p_ctl_out->hole_region.w;
			p_kdrv_out->res_dim_info.hole_h = p_ctl_out->hole_region.h;

			/* yuv output cvt */
			p_kdrv_out->res_des_drt = ctl_vpe_typecast_dst_drt(p_kdrv_cfg->glb_img_info.src_drt, p_hdl->isp_param.yuv_cvt_param.cvt_sel[i]);

			/* always config disable, use driver default setting */
			p_kdrv_out->res_sca_info.sca_iq_en = 0;

			/* todo: check config source */
			p_kdrv_out->out_bg_sel = 0;
		} else {
			/* disable output */
			p_kdrv_out->out_dup_num = 0;
		}
	}

	/* iq configuration */
	rt = ctl_vpe_process_config_dce(p_kdrv_cfg, &p_hdl->isp_param, p_info->dce_out_sz);
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("config dce iq param failed\r\n");
		atomic_inc(&p_hdl->dbg_err_accu_info.err_isp_par);
		return CTL_VPE_E_ISP_PAR;
	}

	ctl_vpe_process_config_dcs(p_kdrv_cfg, &p_hdl->ctl_info);

	/* check user in 2dlut and isp parameters */
	rt = ctl_vpe_process_config_user_in_2dlut(p_kdrv_cfg, &p_hdl->isp_param, &user_2dlut_param);
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("config user input 2dlut param failed\r\n");
		atomic_inc(&p_hdl->dbg_err_accu_info.err_isp_par);
		return CTL_VPE_E_ISP_PAR;
	}

	rt = ctl_vpe_process_config_sharpen(p_kdrv_cfg, &p_hdl->isp_param);
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("config sharpen param failed\r\n");
		atomic_inc(&p_hdl->dbg_err_accu_info.err_isp_par);
		return CTL_VPE_E_ISP_PAR;
	}

	rt = ctl_vpe_process_config_dctg(p_kdrv_cfg, &p_hdl->isp_param);
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("config dctg param failed\r\n");
		atomic_inc(&p_hdl->dbg_err_accu_info.err_isp_par);
		return CTL_VPE_E_ISP_PAR;
	}

	/* debug dump info */
	if (ctl_vpe_process_dbg_dump_cfg(0, 0)) {
		ctl_vpe_process_dbg_dump_kflow_cfg(p_info, ctl_vpe_int_printf);
		ctl_vpe_process_dbg_dump_kflow_isp_param(&p_hdl->isp_param, ctl_vpe_int_printf);
		ctl_vpe_process_dbg_dump_kdrv_cfg(p_job, ctl_vpe_int_printf);
	}

	/* check kdrv config limitation before trigger */
	rt = ctl_vpe_process_chk_kdrv_cfg_lmt(p_kdrv_cfg);
	if (rt != CTL_VPE_E_OK) {
		atomic_inc(&p_hdl->dbg_err_accu_info.err_par);
		return rt;
	}

	/* trigger vpe */
	INIT_LIST_HEAD(&p_kdrv_cfg->config_list);
	INIT_LIST_HEAD(&p_kdrv_job->job_list);
	list_add_tail(&p_kdrv_cfg->config_list, &p_kdrv_job->job_list);
	p_kdrv_job->callback = cb_fp;
	p_kdrv_job->job_num = 1;
	if (kdrv_vpe_if_trigger(p_hdl->kdrv_id, p_kdrv_job) != 0) {
		DBG_ERR("trig vpe failed\r\n");
		atomic_inc(&p_hdl->dbg_err_accu_info.err_kdrv_trig);
		return CTL_VPE_E_KDRV_TRIG;
	}

	return CTL_VPE_E_OK;
}

#if 0
#endif

INT32 ctl_vpe_inbuf_cb_wrapper(CTL_VPE_EVENT_FP bufio_fp, CTL_VPE_EVT *p_evt, CTL_VPE_BUF_IO bufio)
{
	if (bufio_fp == NULL) {
		DBG_WRN("null inbuf cb fp\r\n");
		return CTL_VPE_E_SYS;
	}

	bufio_fp(bufio, (void *)p_evt, NULL);
	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_outbuf_cb_wrapper(CTL_VPE_EVENT_FP bufio_fp, CTL_VPE_OUT_BUF_INFO *p_buf, CTL_VPE_BUF_IO bufio)
{
	if (bufio_fp == NULL) {
		DBG_WRN("null outbuf cb fp\r\n");
		return CTL_VPE_E_SYS;
	}

	bufio_fp(bufio, (void *)p_buf, NULL);
	return CTL_VPE_E_OK;
}

#if 0
#endif

int ctl_vpe_int_printf(const char *fmtstr, ...)
{
	char    buf[512];
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);
	if (len > 0) {
		DBG_DUMP("%s", buf);
	}

	return 0;
}

#endif
