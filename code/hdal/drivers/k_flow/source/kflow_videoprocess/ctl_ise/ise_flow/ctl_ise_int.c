#include "kflow_videoprocess/ctl_ise.h"
#include "gximage/gximage.h"
#include "ctl_ise_int.h"
#include "kwrap/cpu.h"

#if CTL_ISE_MODULE_ENABLE
static void ctl_ise_msg_reset(CTL_ISE_MSG_EVENT *p_evt)
{
	if (p_evt) {
		p_evt->cmd = 0;
		p_evt->param[0] = 0;
		p_evt->param[1] = 0;
		p_evt->param[2] = 0;
		p_evt->rev[0] = CTL_ISE_MSG_STS_FREE;
	}
}

INT32 ctl_ise_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, CTL_ISE_MEM_POOL *p_que)
{
	unsigned long loc_flg;
	CTL_ISE_MSG_EVENT *p_evt;
	INT32 rt = CTL_ISE_E_OK;

	vk_spin_lock_irqsave(&p_que->lock, loc_flg);

	p_evt = vos_list_first_entry_or_null(&p_que->free_list_head, CTL_ISE_MSG_EVENT, list);
	if (p_evt != NULL) {
		vos_list_del_init(&p_evt->list);
		p_que->cur_free_num--;
		if ((p_que->blk_num - p_que->cur_free_num) > p_que->max_used_num) {
			p_que->max_used_num = p_que->blk_num - p_que->cur_free_num;
		}
		p_evt->rev[0] = CTL_ISE_MSG_STS_LOCK;

		p_evt->cmd = cmd;
		p_evt->param[0] = p1;
		p_evt->param[1] = p2;
		p_evt->param[2] = p3;
		p_evt->timestamp = hwclock_get_counter();
		vos_list_add_tail(&p_evt->list, &p_que->used_list_head);
		vos_flag_iset(p_que->flg_id, CTL_ISE_QUE_FLG_PROC);
	} else {
		rt = CTL_ISE_E_QOVR;
	}

	vk_spin_unlock_irqrestore(&p_que->lock, loc_flg);

	return rt;
}

INT32 ctl_ise_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, UINT32 *time, CTL_ISE_MEM_POOL *p_que)
{
	unsigned long loc_flg;
	FLGPTN flag;
	CTL_ISE_MSG_EVENT *p_evt = NULL;

	while (1) {
		/* try to receive event */
		vk_spin_lock_irqsave(&p_que->lock, loc_flg);
		p_evt = vos_list_first_entry_or_null(&p_que->used_list_head, CTL_ISE_MSG_EVENT, list);
		if (p_evt) {
			vos_list_del_init(&p_evt->list);

			*cmd = p_evt->cmd;
			*p1 = p_evt->param[0];
			*p2 = p_evt->param[1];
			*p3 = p_evt->param[2];
			*time = p_evt->timestamp;

			ctl_ise_msg_reset(p_evt);
			vos_list_add_tail(&p_evt->list, &p_que->free_list_head);
			p_que->cur_free_num++;
		}
		vk_spin_unlock_irqrestore(&p_que->lock, loc_flg);

		if (p_evt) {
			break;
		}

		/* recieve event null, wait flag for next attempt */
		vos_flag_wait(&flag, p_que->flg_id, CTL_ISE_QUE_FLG_PROC, TWF_CLR);
	}

	return CTL_ISE_E_OK;
}

INT32 ctl_ise_msg_flush(CTL_ISE_MEM_POOL *p_que, CTL_ISE_HANDLE *p_hdl, CTL_ISE_MSG_FLUSH_CB flush_cb)
{
	unsigned long loc_flg;
	CTL_ISE_LIST_HEAD *p_list;
	CTL_ISE_LIST_HEAD *n;
	CTL_ISE_MSG_EVENT *p_evt;

	vk_spin_lock_irqsave(&p_que->lock, loc_flg);

	vos_list_for_each_safe(p_list, n, &p_que->used_list_head) {
		p_evt = vos_list_entry(p_list, CTL_ISE_MSG_EVENT, list);
		if ((p_evt->param[0] == (UINT32)p_hdl) && (p_evt->cmd == CTL_ISE_MSG_PROCESS)) {
			p_evt->cmd = CTL_ISE_MSG_IGNORE;
			if (flush_cb) {
				p_hdl->reserved[0] = p_evt->timestamp;
				flush_cb(p_evt->param[0], p_evt->param[1], p_evt->param[2], CTL_ISE_E_FLUSH);
			}
		}
	}

	vk_spin_unlock_irqrestore(&p_que->lock, loc_flg);

	return CTL_ISE_E_OK;
}

INT32 ctl_ise_msg_modify(CTL_ISE_MEM_POOL *p_que, CTL_ISE_HANDLE *p_hdl, CTL_ISE_MSG_MODIFY_CB modify_cb, void *p_data)
{
	unsigned long loc_flg;
	CTL_ISE_LIST_HEAD *p_list;
	CTL_ISE_LIST_HEAD *n;
	CTL_ISE_MSG_EVENT *p_evt;
	CTL_ISE_MSG_MODIFY_CB_DATA cb_data;

	vk_spin_lock_irqsave(&p_que->lock, loc_flg);

	vos_list_for_each_safe(p_list, n, &p_que->used_list_head) {
		p_evt = vos_list_entry(p_list, CTL_ISE_MSG_EVENT, list);
		if ((p_evt->param[0] == (UINT32)p_hdl) && (p_evt->cmd == CTL_ISE_MSG_PROCESS)) {
			if (modify_cb) {
				cb_data.hdl = p_evt->param[0];
				cb_data.data = p_evt->param[1];
				cb_data.buf_id = p_evt->param[2];
				cb_data.err = CTL_ISE_E_FLUSH;
				cb_data.p_modify_info = p_data;
				modify_cb(&cb_data);
			}
		}
	}

	vk_spin_unlock_irqrestore(&p_que->lock, loc_flg);

	return CTL_ISE_E_OK;
}

INT32 ctl_ise_msg_init_queue(CTL_ISE_MEM_POOL *p_que)
{
	CTL_ISE_MSG_EVENT *p_pool;
	CTL_ISE_MSG_EVENT *p_evt;
	UINT32 i;

	VOS_INIT_LIST_HEAD(&p_que->free_list_head);
	VOS_INIT_LIST_HEAD(&p_que->used_list_head);

	p_pool = (CTL_ISE_MSG_EVENT *)p_que->start_addr;
	for (i = 0; i < p_que->blk_num; i++) {
		p_evt = &p_pool[i];

		ctl_ise_msg_reset(p_evt);
		p_evt->rev[1] = i;

		VOS_INIT_LIST_HEAD(&p_evt->list);
		vos_list_add_tail(&p_evt->list, &p_que->free_list_head);
	}

	return CTL_ISE_E_OK;
}

#if 0
#endif

UINT32 ctl_ise_util_y2uvlof(VDO_PXLFMT fmt, UINT32 y_lof)
{
	fmt = fmt & (~VDO_PIX_YCC_MASK);

	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NVX) {
		fmt = VDO_PXLFMT_YUV420;
	}

	switch (fmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
		return y_lof >> 1;

	case VDO_PXLFMT_YUV420_PLANAR:
		return y_lof >> 1;

	case VDO_PXLFMT_YUV444:
		return y_lof << 1;

	default:
//		DBG_WRN("cal uv lineoffset fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_lof);
		return y_lof;
	}
}

UINT32 ctl_ise_util_y2uvwidth(VDO_PXLFMT fmt, UINT32 y_w)
{
	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NVX) {
		fmt = VDO_PXLFMT_YUV420;
	}

	switch (fmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
		return y_w >> 1;

	case VDO_PXLFMT_YUV420_PLANAR:
		return y_w >> 1;

	case VDO_PXLFMT_YUV422:
		return y_w >> 1;

	case VDO_PXLFMT_YUV420:
		return y_w >> 1;

	default:
		return y_w;
//		DBG_WRN("cal uv width fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_w);
	}
}

UINT32 ctl_ise_util_y2uvheight(VDO_PXLFMT fmt, UINT32 y_h)
{
	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NVX) {
		fmt = VDO_PXLFMT_YUV420;
	}

	switch (fmt) {
	case VDO_PXLFMT_YUV420_PLANAR:
		return y_h >> 1;

	case VDO_PXLFMT_YUV420:
		return y_h >> 1;

	default:
//		DBG_WRN("cal uv height fail 0x%.8x, %d\r\n", (unsigned int)fmt, (int)y_h);
		return y_h;
	}
}

UINT32 ctl_ise_util_yuv_size(VDO_PXLFMT fmt, UINT32 y_width, UINT32 y_height)
{
	UINT32 ratio, size, tmp_lof;

	switch (fmt) {
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

	case VDO_PXLFMT_YUV420_NVX2:
		/* 520 yuv compress, compress rate fix 75%, 16x align */
		/* y */
		tmp_lof = ALIGN_CEIL(y_width, 16) * 3 / 4;
		size = tmp_lof * y_height;

		/* uv */
		tmp_lof = ALIGN_CEIL(ctl_ise_util_y2uvlof(fmt, y_width), 16) * 3 / 4;
		size += tmp_lof * y_height / 2;
		break;

	default:
		ratio = 0;
		size = 0;
		DBG_ERR("cal yuv size fail 0x%.8x %d %d %d\r\n", (unsigned int)fmt, (int)y_width, (int)y_height, (int)ratio);
		break;
	}

	/* align size to 64 for cache */
	size = ALIGN_CEIL(size, VOS_ALIGN_BYTES);

	return size;
}

#if 0
#endif

void ctl_ise_process_dbg_dump_kflow_cfg(CTL_ISE_BASEINFO *p_base, int (*dump)(const char *fmt, ...))
{
	UINT32 i;

	if (p_base == NULL)
		return ;

	if (dump == NULL) {
		dump = ctl_ise_int_printf;
	}

	dump("\r\n----- ctl_ise dump kflow cfg -----\r\n");
	dump("%12s %12s %12s %12s %12s %12s %12s %12s\r\n",
				"src_w", "src_h", "src_h_align", "crp_mode", "crp_x", "crp_y", "crp_w", "crp_h");
	dump("%12d %12d %12d %12d %12d %12d %12d %12d\r\n\r\n",
				(int)p_base->src_img.size.w,
				(int)p_base->src_img.size.h,
				(int)p_base->src_img.src_img_h_align,
				(int)p_base->in_crop.mode,
				(int)p_base->in_crop.crp_window.x,
				(int)p_base->in_crop.crp_window.y,
				(int)p_base->in_crop.crp_window.w,
				(int)p_base->in_crop.crp_window.h);

	dump("%4s %4s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %10s\r\n", "pid", "en",
				"pre_crp_x", "pre_crp_y", "pre_crp_w", "pre_crp_h", "scl_w", "scl_h",
				"post_crp_x", "post_crp_y", "post_crp_w", "post_crp_h", "fmt");
	for (i = 0; i < CTL_ISE_OUT_PATH_ID_MAX; i++) {
		CTL_ISE_OUT_PATH *p_path = &p_base->out_path[i];

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
	for (i = 0; i < CTL_ISE_OUT_PATH_ID_MAX; i++) {
		CTL_ISE_OUT_PATH *p_path = &p_base->out_path[i];

		dump("%4d %4d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %10d\r\n",
					(int)i, (int)p_path->enable,
					(int)p_path->bg_size.w, (int)p_path->bg_size.h,
					(int)p_path->out_window.x, (int)p_path->out_window.y, (int)p_path->out_window.w, (int)p_path->out_window.h,
					(int)p_path->dst_pos.x, (int)p_path->dst_pos.y,
					(int)p_path->hole_region.x, (int)p_path->hole_region.y, (int)p_path->hole_region.w, (int)p_path->hole_region.h,
					(int)p_base->out_path_h_align[i]);
	}
}

void ctl_ise_process_dbg_dump_kdrv_cfg(CTL_ISE_JOB *p_job, int (*dump)(const char *fmt, ...))
{
#if 0
	CTL_ISE_HANDLE *p_hdl;
	KDRV_ISE_CONFIG *p_kdrv_cfg;
	UINT32 i;

	if (p_job == NULL)
		return ;

	if (dump == NULL) {
		dump = ctl_ise_int_printf;
	}

	p_hdl = (CTL_ISE_HANDLE *)p_job->owner;
	p_kdrv_cfg = &p_job->kdrv_cfg;
	dump("----- ctl_ise(%s) dump kdrv_ise cfg -----\r\n", p_hdl->name);
	dump("%12s %12s %12s %12s %12s %12s %10s %10s\r\n",
				"width", "height", "crp_x", "crp_y", "crp_w", "crp_h", "addr", "func_mode");
	DBG_DUMP("%12d %12d %12d %12d %12d %12d 0x%.8x 0x%.8x\r\n",
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
	for (i = 0; i < KDRV_ISE_MAX_IMG; i++) {
		KDRV_ISE_OUT_IMG_INFO *p_path = &p_kdrv_cfg->out_img_info[i];

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
	for (i = 0; i < KDRV_ISE_MAX_IMG; i++) {
		KDRV_ISE_OUT_IMG_INFO *p_path = &p_kdrv_cfg->out_img_info[i];

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

	if (p_kdrv_cfg->func_mode & KDRV_ISE_FUNC_SHP_EN) {
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
		for (i = 0; i < KDRV_ISE_NOISE_CURVE_NUMS; i++) {
			str_len += snprintf(str_mem + str_len, 128 - str_len, "%d, ", (int)p_kdrv_cfg->sharpen_param.noise_curve[i]);
		}
		dump("%s\r\n\r\n", str_mem);
	}

	if (p_kdrv_cfg->func_mode & KDRV_ISE_FUNC_DCE_EN) {
		dump("\r\ndce param\r\n");
		dump("%8s %12s %8s %8s %8s %8s %8s %8s %8s %8s%10s\r\n",
					"mode", "lsb_rand", "width", "height", "radius",
					"lut_sz", "xofs_i", "xofs_f", "yofs_i", "yofs_f", "lut_addr");
		dump("%8d %12d %8d %8d %8d %8d %8d %8d %8d %8d 0x%.8x\r\n",
					(int)p_kdrv_cfg->dce_param.dce_mode,
					(int)p_kdrv_cfg->dce_param.lsb_rand,
					(int)p_kdrv_cfg->dce_param.dewarp_width,
					(int)p_kdrv_cfg->dce_param.dewarp_height,
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
#endif
}

UINT32 ctl_ise_process_dbg_dump_cfg(UINT32 op, UINT32 val)
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

INT32 ctl_ise_process_alloc_outpath_buf(CTL_ISE_BASEINFO *p_info, CTL_ISE_JOB *p_job, CTL_ISE_EVENT_FP bufio_fp)
{
	VDO_FRAME *p_in_frm;
	UINT32 alloc_success_cnt;
	UINT32 i;
	UINT32 bg_height;

	if (bufio_fp == NULL) {
		DBG_WRN("null out bufio callback\r\n");
		return CTL_ISE_E_SYS;
	}

	alloc_success_cnt = 0;
	p_in_frm = (VDO_FRAME *)p_job->in_evt.data_addr;
	for (i = 0; i < CTL_ISE_OUT_PATH_ID_MAX; i++) {
		CTL_ISE_OUT_PATH *p_path = &p_info->out_path[i];
		CTL_ISE_OUT_BUF_INFO *p_buf = &p_job->buf_info[i];

		memset((void *)p_buf, 0, sizeof(CTL_ISE_OUT_BUF_INFO));
		p_buf->pid = i;

		if (!p_path->enable) {
			continue;
		}

		if (p_info->out_path_h_align[i] == 0) {
			bg_height = p_path->bg_size.h;
		} else {
			bg_height = ALIGN_CEIL(p_path->bg_size.h, p_info->out_path_h_align[i]);
		}

		p_buf->buf_size = ctl_ise_util_yuv_size(p_path->fmt, p_path->bg_size.w, bg_height);
		if (p_buf->buf_size == 0) {
			continue;
		}

		p_buf->vdo_frm.count = p_in_frm->count;
		p_buf->vdo_frm.timestamp = p_in_frm->timestamp;

		ctl_ise_outbuf_cb_wrapper(bufio_fp, p_buf, CTL_ISE_BUF_NEW);
		if (p_buf->buf_addr) {
			alloc_success_cnt++;

			p_buf->vdo_frm.sign = MAKEFOURCC('V', 'F', 'R', 'M');
			p_buf->vdo_frm.pxlfmt = p_path->fmt;
			p_buf->vdo_frm.size.w = p_path->bg_size.w;
			p_buf->vdo_frm.size.h = p_path->bg_size.h;
			p_buf->vdo_frm.addr[0] = p_buf->buf_addr;

			p_buf->vdo_frm.pw[VDO_PINDEX_Y] = p_buf->vdo_frm.size.w;
			p_buf->vdo_frm.pw[VDO_PINDEX_U] = ctl_ise_util_y2uvwidth(p_buf->vdo_frm.pxlfmt, p_buf->vdo_frm.size.w);
			p_buf->vdo_frm.pw[VDO_PINDEX_V] = p_buf->vdo_frm.pw[VDO_PINDEX_U];

			p_buf->vdo_frm.ph[VDO_PINDEX_Y] = p_buf->vdo_frm.size.h;
			p_buf->vdo_frm.ph[VDO_PINDEX_U] = ctl_ise_util_y2uvheight(p_buf->vdo_frm.pxlfmt, p_buf->vdo_frm.size.h);
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
				p_buf->vdo_frm.loff[VDO_PINDEX_U] = ctl_ise_util_y2uvlof(p_buf->vdo_frm.pxlfmt, p_buf->vdo_frm.loff[VDO_PINDEX_Y]);
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
		return CTL_ISE_E_NOMEM;
	}

	return CTL_ISE_E_OK;
}

INT32 ctl_ise_process_chk_input_lmt(VDO_FRAME *p_in_frm)
{
	if (p_in_frm == NULL)
		return CTL_ISE_E_PAR;

	if (p_in_frm->addr[0] == 0) {
		DBG_ERR("vdoin info error addr 0\r\n");
		return CTL_ISE_E_INDATA;
	}

	if (p_in_frm->size.w == 0 || p_in_frm->size.h == 0) {
		DBG_ERR("vdoin info error width/height = 0\r\n");
		return CTL_ISE_E_INDATA;
	}

	if (p_in_frm->loff[0] == 0) {
		DBG_ERR("vdoin info error loff = 0\r\n");
		return CTL_ISE_E_INDATA;
	}

	p_in_frm->pxlfmt = p_in_frm->pxlfmt & (~VDO_PIX_YCC_MASK);	//remove ycc mask
	if (p_in_frm->pxlfmt != VDO_PXLFMT_YUV420 &&
		p_in_frm->pxlfmt != VDO_PXLFMT_YUV422 &&
		p_in_frm->pxlfmt != VDO_PXLFMT_Y8) {
		DBG_ERR("vdoin info error, ise only support yuv420/yuv422/y8, cur fmt 0x%.8x\r\n", (unsigned int)p_in_frm->pxlfmt);
		return CTL_ISE_E_INDATA;
	}

	return CTL_ISE_E_OK;
}

INT32 ctl_ise_process_d2d(CTL_ISE_HANDLE *p_hdl, CTL_ISE_JOB *p_job, void *cb_fp)
{
	unsigned long loc_flg;
	CTL_ISE_BASEINFO *p_info;
	VDO_FRAME *p_in_frm;
	VDO_FRAME out_frm;
	IRECT in_roi;
	IRECT out_roi;
	UINT32 i;
	INT32 rt;

	/* parse input frame info and ctl info to kdrv struct */
	p_in_frm = (VDO_FRAME *)p_job->in_evt.data_addr;
	if(p_job->p_dbg_node) {
		p_job->p_dbg_node->handle = (UINT32)p_hdl;
		p_job->p_dbg_node->input_frm_count = p_in_frm->count;
	}
	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	p_job->base_info = p_hdl->ctl_info;
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
	p_info = &p_job->base_info;

	/* check header info */
	if (ctl_ise_process_chk_input_lmt(p_in_frm) != CTL_ISE_E_OK) {
		atomic_inc(&p_hdl->dbg_err_accu_info.err_indata);
		return CTL_ISE_E_INDATA;
	}

	/* pre process for un-config settings */
	for (i = 0; i < CTL_ISE_OUT_PATH_ID_MAX; i++) {
		CTL_ISE_OUT_PATH *p_ctl_out = &p_info->out_path[i];

		if (p_ctl_out->post_scl_crop.x == 0 &&
			p_ctl_out->post_scl_crop.y == 0 &&
			p_ctl_out->post_scl_crop.w == 0 &&
			p_ctl_out->post_scl_crop.h == 0) {
			p_ctl_out->post_scl_crop.x = 0;
			p_ctl_out->post_scl_crop.y = 0;
			p_ctl_out->post_scl_crop.w = p_ctl_out->scl_size.w;
			p_ctl_out->post_scl_crop.h = p_ctl_out->scl_size.h;
		}

		p_ctl_out->bg_size.w = p_ctl_out->post_scl_crop.w;
		p_ctl_out->bg_size.h = p_ctl_out->post_scl_crop.h;
		p_ctl_out->out_window.x = 0;
		p_ctl_out->out_window.y = 0;
		p_ctl_out->out_window.w = p_ctl_out->bg_size.w;
		p_ctl_out->out_window.h = p_ctl_out->bg_size.h;
	}

	/* alloc buffer */
	rt = ctl_ise_process_alloc_outpath_buf(p_info, p_job, p_hdl->cb_fp[CTL_ISE_CBEVT_OUT_BUF]);
	if (rt != CTL_ISE_E_OK) {
		atomic_inc(&p_hdl->dbg_err_accu_info.err_nomem);
		DBG_IND("alloc out path buffer failed\r\n");
		return rt;
	}


	/* input frame, set src image info back to ctl_info for debug */
	p_info->src_img.size.w = p_in_frm->size.w;
	p_info->src_img.size.h = p_in_frm->size.h;
	if (p_in_frm->reserved[0] == MAKEFOURCC('H', 'A', 'L', 'N')) {
		/* check input buffer height alignment */
		p_info->src_img.src_img_h_align = p_in_frm->reserved[1];
	} else {
		p_info->src_img.src_img_h_align = 0;
	}

	if (p_info->in_crop.mode == CTL_ISE_IN_CROP_AUTO) {
		/* crop window from sie */
		p_info->in_crop.crp_window.x = (p_in_frm->reserved[5] & 0xffff0000) >> 16;
		p_info->in_crop.crp_window.y = (p_in_frm->reserved[5] & 0x0000ffff);
		p_info->in_crop.crp_window.w = (p_in_frm->reserved[6] & 0xffff0000) >> 16;
		p_info->in_crop.crp_window.h = (p_in_frm->reserved[6] & 0x0000ffff);
	} else if (p_info->in_crop.mode == CTL_ISE_IN_CROP_NONE) {
		p_info->in_crop.crp_window.x = 0;
		p_info->in_crop.crp_window.y = 0;
		p_info->in_crop.crp_window.w = p_in_frm->size.w;
		p_info->in_crop.crp_window.h = p_in_frm->size.h;
	} else if (p_info->in_crop.mode == CTL_ISE_IN_CROP_USER) {
		/* do nothing, already set in info */
	} else {
		atomic_inc(&p_hdl->dbg_err_accu_info.err_indata);
		DBG_ERR("Unknown crop mode\r\n");
		return CTL_ISE_E_PAR;
	}
	in_roi.x = p_info->in_crop.crp_window.x;
	in_roi.y = p_info->in_crop.crp_window.y;
	in_roi.w = p_info->in_crop.crp_window.w;
	in_roi.h = p_info->in_crop.crp_window.h;
	p_hdl->ctl_info.src_img = p_info->src_img;
	p_hdl->ctl_info.in_crop.crp_window = p_info->in_crop.crp_window;

	for (i = 0; i < CTL_ISE_OUT_PATH_ID_MAX; i++) {
		CTL_ISE_OUT_BUF_INFO *p_buf = &p_job->buf_info[i];
		CTL_ISE_OUT_PATH *p_ctl_out = &p_info->out_path[i];

		if (p_ctl_out->enable && p_buf->vdo_frm.addr[0]) {
			out_frm = p_buf->vdo_frm;
			out_frm.size.w = p_ctl_out->post_scl_crop.w;
			out_frm.size.h = p_ctl_out->post_scl_crop.h;
			out_roi.x = -p_ctl_out->post_scl_crop.x;
			out_roi.y = -p_ctl_out->post_scl_crop.y;
			out_roi.w = p_ctl_out->scl_size.w;
			out_roi.h = p_ctl_out->scl_size.h;
			/* trigger ise by gximg */
			rt = gximg_scale_data_no_flush(p_in_frm, &in_roi, &out_frm, &out_roi, GXIMG_SCALE_AUTO, 0);
			if (rt != E_OK) {
				atomic_inc(&p_hdl->dbg_err_accu_info.err_kdrv_trig);
				DBG_ERR("trig gximg_scale failed %d\r\n", rt);
				return CTL_ISE_E_KDRV_TRIG;
			}

			/* unsupport function */
			#if 0
			/* sca crop window(0, 0, 0, 0) -> no crop */
			if (p_ctl_out->pre_scl_crop.x == 0 &&
				p_ctl_out->pre_scl_crop.y == 0 &&
				p_ctl_out->pre_scl_crop.w == 0 &&
				p_ctl_out->pre_scl_crop.h == 0) {
				p_kdrv_out->res_dim_info.sca_crop_x = 0;
				p_kdrv_out->res_dim_info.sca_crop_y = 0;
				p_kdrv_out->res_dim_info.sca_crop_w = p_kdrv_cfg->glb_img_info.src_in_w;
				p_kdrv_out->res_dim_info.sca_crop_h = p_kdrv_cfg->glb_img_info.src_in_h;
			} else {
				p_kdrv_out->res_dim_info.sca_crop_x = p_ctl_out->pre_scl_crop.x;
				p_kdrv_out->res_dim_info.sca_crop_y = p_ctl_out->pre_scl_crop.y;
				p_kdrv_out->res_dim_info.sca_crop_w = p_ctl_out->pre_scl_crop.w;
				p_kdrv_out->res_dim_info.sca_crop_h = p_ctl_out->pre_scl_crop.h;
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
			#endif
		}
	}

	/* debug dump info */
	if (ctl_ise_process_dbg_dump_cfg(0, 0)) {
		ctl_ise_process_dbg_dump_kflow_cfg(p_info, ctl_ise_int_printf);
		ctl_ise_process_dbg_dump_kdrv_cfg(p_job, ctl_ise_int_printf);
	}

	return CTL_ISE_E_OK;
}

#if 0
#endif

INT32 ctl_ise_inbuf_cb_wrapper(CTL_ISE_EVENT_FP bufio_fp, CTL_ISE_EVT *p_evt, CTL_ISE_BUF_IO bufio)
{
	if (bufio_fp == NULL) {
		DBG_ERR("null inbuf cb fp\r\n");
		return CTL_ISE_E_SYS;
	}

	bufio_fp(bufio, (void *)p_evt, NULL);
	return CTL_ISE_E_OK;
}

INT32 ctl_ise_outbuf_cb_wrapper(CTL_ISE_EVENT_FP bufio_fp, CTL_ISE_OUT_BUF_INFO *p_buf, CTL_ISE_BUF_IO bufio)
{
	if (bufio_fp == NULL) {
		DBG_ERR("null outbuf cb fp\r\n");
		return CTL_ISE_E_SYS;
	}

	bufio_fp(bufio, (void *)p_buf, NULL);
	return CTL_ISE_E_OK;
}

#if 0
#endif

int ctl_ise_int_printf(const char *fmtstr, ...)
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
