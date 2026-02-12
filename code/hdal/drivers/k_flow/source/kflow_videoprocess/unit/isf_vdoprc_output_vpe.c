#include "isf_vdoprc_int.h"

#if (USE_VPE == ENABLE)
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_ov
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_ov_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_ov_debug_level, isf_vdoprc_ov_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_ov_debug_level, "vdoprc_ov debug level");

///////////////////////////////////////////////////////////////////////////////

#define WATCH_PORT			ISF_OUT(0)
#define FORCE_DUMP_DATA		DISABLE

#define DUMP_DATA_ERROR		DISABLE

#define DUMP_PERFORMANCE		DISABLE
#define AUTO_RELEASE			ENABLE
#define DUMP_TRACK			DISABLE

#define DUMP_FPS				DISABLE

#define SYNC_TAG 				MAKEFOURCC('S','Y','N','C')
#define RANDOM_RANGE(n)     	(rand() % (n))

#define DUMP_KICK_ERR			DISABLE

#if 0
static void _vdoprc_vpe_dump_outpath(ISF_UNIT *p_thisunit, CTL_VPE_OUT_PATH *p_data)
{
	DBG_DUMP("######## %s OUT[%d] INFO  ########\r\n", p_thisunit->unit_name, p_data->pid);
	DBG_DUMP(".enable = %d\r\n", p_data->enable);
	DBG_DUMP(".fmt = 0x%X\r\n", p_data->fmt);
	DBG_DUMP(".bg_size = %dx%d\r\n", p_data->bg_size.w, p_data->bg_size.h);
	DBG_DUMP(".scl_size = %dx%d\r\n", p_data->scl_size.w, p_data->scl_size.h);
	DBG_DUMP(".pre_scl_crop = %d,%d,%d,%d\r\n", p_data->pre_scl_crop.x, p_data->pre_scl_crop.y, p_data->pre_scl_crop.w, p_data->pre_scl_crop.h);
	DBG_DUMP(".post_scl_crop = %d,%d,%d,%d\r\n", p_data->post_scl_crop.x, p_data->post_scl_crop.y, p_data->post_scl_crop.w, p_data->post_scl_crop.h);
	DBG_DUMP(".out_window = %d,%d,%d,%d\r\n", p_data->out_window.x, p_data->out_window.y, p_data->out_window.w, p_data->out_window.h);
	DBG_DUMP(".dst_pos = %d,%d\r\n", p_data->dst_pos.x, p_data->dst_pos.y);
	DBG_DUMP(".hole_region = %d,%d,%d,%d\r\n", p_data->hole_region.x, p_data->hole_region.y, p_data->hole_region.w, p_data->hole_region.h);
	DBG_DUMP("#######################################\r\n");
}
#endif
static ISF_RV _vdoprc_check_vpe_out_fmt(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 imgfmt)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 vfmt = imgfmt;
	switch (vfmt) {
	case VDO_PXLFMT_YUV420:
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_YUV)) {
			vfmt = 0;
		}
		break;
	case VDO_PXLFMT_YUV420_NVX2:
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_NVX)) {
			vfmt = 0;
		}
		break;
	default:
		vfmt = 0;
		break;
	}
	if (!vfmt) {
		DBG_ERR("-out%d:fmt=%08x is not supported with pipe=%d!\r\n", pid, imgfmt, p_ctx->cur_mode);
		return ISF_ERR_FAILED;
	}
	return ISF_OK;
}


static ISF_RV _vdoprc_check_vpe_out_dir(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 imgdir)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	if (imgdir != 0) {
		if ((imgdir & VDO_DIR_MIRRORX) && !(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_MIRRORX)) {
			DBG_ERR("%s.out[%d]! cannot enable mirror-x with pipe=%08x\r\n",
				p_thisunit->unit_name, pid, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		if ((imgdir & VDO_DIR_MIRRORY) && !(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_MIRRORY)) {
			DBG_ERR("%s.out[%d]! cannot enable mirror-y with pipe=%08x\r\n",
				p_thisunit->unit_name, pid, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		/*
		if ((imgdir & (VDO_DIR_ROTATE_90|VDO_DIR_ROTATE_270)) && !(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_ROTATE)) {
			DBG_ERR("%s.out[%d]! cannot enable rotate with pipe=%08x\r\n",
				p_thisunit->unit_name, pid, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		*/
	}
	return ISF_OK;
}

static ISF_RV _vdoprc_check_vpe_out_crop(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 imgcrop)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_dest_info = p_thisunit->port_outinfo[pid - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);

	if (imgcrop) {
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_CROP)) {
			DBG_ERR("%s.out[%d]! cannot enable crop with pipe=%08x\r\n",
				p_thisunit->unit_name, pid, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}

		if (p_vdoinfo->window.w == 0 || p_vdoinfo->window.h == 0) {
			DBG_ERR("-out%d:crop win(%d,%d) is zero?\r\n",
				pid, p_vdoinfo->window.w, p_vdoinfo->window.h);
			return ISF_ERR_FAILED;
		}
		if (p_ctx->out[pid].size.w && p_ctx->out[pid].size.h) {
			if ( ((p_vdoinfo->window.x + p_vdoinfo->window.w) > p_ctx->out[pid].size.w)
			|| ((p_vdoinfo->window.y + p_vdoinfo->window.h) > p_ctx->out[pid].size.h) ) {
				DBG_ERR("-out%d:crop(%d,%d,%d,%d) is out of dim(%d,%d)?\r\n",
					pid,
					p_vdoinfo->window.x, p_vdoinfo->window.y, p_vdoinfo->window.w, p_vdoinfo->window.h,
					p_ctx->out[pid].size.w, p_ctx->out[pid].size.h);
				return ISF_ERR_FAILED;
			}
		}
#if defined(_BSP_NA51023_)
		if (p_ctx->ctrl.cur_func2 & VDOPRC_FUNC2_VIEWTRACKING1) {
			DBG_ERR("-out%d:crop is conflict to track!\r\n",
				pid);
		}
#endif
	}
	return ISF_OK;
}

static ISF_RV _vdoprc_check_vpe_pre_scl_crop(ISF_UNIT *p_thisunit, UINT32 pid)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	INT32 x, y, w, h;

	x = (INT32)p_ctx->pre_scl_crop[pid].x;
	y = (INT32)p_ctx->pre_scl_crop[pid].y;
	w = (INT32)p_ctx->pre_scl_crop[pid].w;
	h = (INT32)p_ctx->pre_scl_crop[pid].h;

	if (!x && !y && !w && !h) {
		//no crop
		return ISF_OK;
	}

	if (((x + w) > p_ctx->in[0].max_size.w) || ((y + h) > p_ctx->in[0].max_size.h) ) {
		DBG_ERR("-out%d:pre scl crop(%d,%d,%d,%d) is out of in_max dim(%d,%d)?\r\n", pid, x, y, w, h,
			p_ctx->in[0].max_size.w, p_ctx->in[0].max_size.h);
		return ISF_ERR_FAILED;
	}
	if (w == 0 || h == 0) {
		DBG_ERR("-out%d:pre scl win(%d,%d) is zero?\r\n", pid, w, h);
		return ISF_ERR_FAILED;
	}

	if ((CTL_VPE_IN_CROP_MODE)p_ctx->in[0].crop.mode != CTL_VPE_IN_CROP_NONE) {
		INT32 in_x, in_y, in_w, in_h;

		in_x = (INT32)p_ctx->in[0].crop.crp_window.x;
		in_y = (INT32)p_ctx->in[0].crop.crp_window.y;
		in_w = (INT32)p_ctx->in[0].crop.crp_window.w;
		in_h = (INT32)p_ctx->in[0].crop.crp_window.h;
		if ((x < in_x) || y < (in_y) || ((x+w) > (in_x+in_w)) || ((y+h) > (in_y+in_h))) {
			DBG_ERR("-out%d:pre scl crop(%d,%d,%d,%d) is out of in_crop dim(%d,%d,%d,%d)?\r\n", pid, x, y, w, h, in_x, in_y, in_w, in_h);
			return ISF_ERR_FAILED;
		}
	}
	return ISF_OK;
}

ISF_RV _vdoprc_config_out_vpe(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en, BOOL runtime_update)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_dest_info = p_thisunit->port_outinfo[pid - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
	ISF_VDO_INFO *p_dest_vdoinfo = (ISF_VDO_INFO *)(p_thisunit->p_base->get_bind_info(p_thisunit, pid));
	ISF_RV rv;
	CTL_VPE_OUT_PATH out_path = {0};

	pid = pid - ISF_OUT_BASE;
	if ( pid < CTL_VPE_OUT_PATH_ID_MAX ) {

		if (!en) {
			memset((void*)&(p_ctx->out[pid]), 0, sizeof(CTL_IPP_OUT_PATH));
			p_ctx->out[pid].pid = out_path.pid = pid;
			p_ctx->out[pid].enable = out_path.enable = FALSE;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "off");
#if (USE_OUT_FRC == ENABLE)
			_isf_frc_stop(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]));
#endif
			ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
		} else {
			UINT32 w,h,imgfmt;

			p_ctx->out[pid].pid = pid;
			p_ctx->out[pid].enable = TRUE;
    			w = p_vdoinfo->imgsize.w;
    			h = p_vdoinfo->imgsize.h;
    			imgfmt = p_vdoinfo->imgfmt;
			if (p_dest_vdoinfo && (w == 0 && h == 0)) {
				//auto sync w,h from dest
	    			w = p_dest_vdoinfo->imgsize.w;
	    			h = p_dest_vdoinfo->imgsize.h;
			}
			if (p_dest_vdoinfo && (imgfmt == 0)) {
				//auto sync fmt from dest
	    			imgfmt = p_dest_vdoinfo->imgfmt;
			}
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "size(%d,%d) fmt=%08x",
				w, h, imgfmt);
			if ((w == 0 || h == 0) && (p_ctx->out_win[pid].window.w == 0 || p_ctx->out_win[pid].window.h == 0) ) {
				DBG_ERR("-out%d:size(%d,%d) is zero?\r\n",
					pid, w, h);
				return ISF_ERR_FAILED;
			}
			if (imgfmt == 0) {
				DBG_ERR("-out%d:fmt is zero?\r\n", pid);
				return ISF_ERR_FAILED;
			}
			p_ctx->out[pid].fmt = imgfmt;
			p_ctx->out[pid].size.w = w;
			p_ctx->out[pid].size.h = h;
			p_ctx->out[pid].lofs = ALIGN_CEIL_16(w);
			if (p_vdoinfo->window.x == 0 && p_vdoinfo->window.y == 0 && p_vdoinfo->window.w == 0 && p_vdoinfo->window.h == 0) {
				//use full image size
				p_ctx->out_crop_mode[pid] = 0;
				p_ctx->out[pid].crp_window.x = 0;
				p_ctx->out[pid].crp_window.y = 0;
				p_ctx->out[pid].crp_window.w = w;
				p_ctx->out[pid].crp_window.h = h;
			} else {
				p_ctx->out_crop_mode[pid] = 1;
				p_ctx->out[pid].crp_window = p_vdoinfo->window;
				p_ctx->out[pid].lofs = ALIGN_CEIL_16(p_vdoinfo->window.w);
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "-out%d:crop(%d,%d,%d,%d)",
					pid, p_vdoinfo->window.x, p_vdoinfo->window.y, p_vdoinfo->window.w, p_vdoinfo->window.h);
			}
			rv = _vdoprc_check_vpe_out_fmt(p_thisunit, pid, imgfmt);
			if (rv != ISF_OK) {
				return rv;
			}
			rv = _vdoprc_check_vpe_out_crop(p_thisunit, pid, p_ctx->out_crop_mode[pid]);
			if (rv != ISF_OK) {
				return rv;
			}
			rv = _vdoprc_check_vpe_out_dir(p_thisunit, pid, p_vdoinfo->direct);
			if (rv != ISF_OK) {
				return rv;
			}
			rv = _vdoprc_check_vpe_pre_scl_crop(p_thisunit, pid);
			if (rv != ISF_OK) {
				return rv;
			}
#if (USE_OUT_FRC == ENABLE)
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM_IMM, "frc(%d,%d)",
				pid, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
			if ((p_vdoinfo->framepersecond != VDO_FRC_ALL) && (p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (pid == p_ctx->ctrl._3dnr_refpath)) {
				DBG_ERR("-out%d:frc(%d,%d) is conflict to func 3DNR, ignore this frc!!!\r\n", pid, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
				p_vdoinfo->framepersecond = VDO_FRC_ALL;
			}
			if (runtime_update == FALSE || p_ctx->outfrc[pid].sample_mode == ISF_SAMPLE_OFF) {
				_isf_frc_start(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
			} else {
				_isf_frc_update_imm(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
			}
#endif
			out_path.pid = pid;
			out_path.enable = TRUE;
			out_path.fmt = p_ctx->out[pid].fmt;
			out_path.pre_scl_crop.x = p_ctx->pre_scl_crop[pid].x;
			out_path.pre_scl_crop.y = p_ctx->pre_scl_crop[pid].y;
			out_path.pre_scl_crop.w = p_ctx->pre_scl_crop[pid].w;
			out_path.pre_scl_crop.h = p_ctx->pre_scl_crop[pid].h;
			if (p_ctx->out[pid].size.w && p_ctx->out[pid].size.h &&
				p_ctx->out_win[pid].imgaspect.w == 0 && p_ctx->out_win[pid].imgaspect.h == 0) {
				out_path.scl_size = p_ctx->out[pid].size;
				if (p_ctx->out_crop_mode[pid]) {
					out_path.bg_size.w = p_ctx->out[pid].crp_window.w;
					out_path.bg_size.h = p_ctx->out[pid].crp_window.h;
					out_path.out_window.x = 0;
					out_path.out_window.y = 0;
					out_path.out_window.w = p_ctx->out[pid].crp_window.w;
					out_path.out_window.h = p_ctx->out[pid].crp_window.h;
				} else {
					out_path.bg_size.w = p_ctx->out[pid].size.w;
					out_path.bg_size.h = p_ctx->out[pid].size.h;
					out_path.out_window.x = 0;
					out_path.out_window.y = 0;
					out_path.out_window.w = out_path.scl_size.w;
					out_path.out_window.h = out_path.scl_size.h;
				}
			} else {
				out_path.bg_size.w = p_ctx->out_win[pid].imgaspect.w;
				out_path.bg_size.h = p_ctx->out_win[pid].imgaspect.h;
				if (p_ctx->out_crop_mode[pid]) {
					out_path.out_window.x = p_ctx->out_win[pid].window.x;
					out_path.out_window.y = p_ctx->out_win[pid].window.y;
					out_path.out_window.w = p_ctx->out[pid].crp_window.w;
					out_path.out_window.h = p_ctx->out[pid].crp_window.h;

				} else {
					out_path.out_window = p_ctx->out_win[pid].window;
				}
				out_path.scl_size.w = p_ctx->out_win[pid].window.w;
				out_path.scl_size.h = p_ctx->out_win[pid].window.h;
			}
			if (p_ctx->out_crop_mode[pid]) {
				out_path.post_scl_crop = p_ctx->out[pid].crp_window;
			}
			out_path.dst_pos.x = 0;
			out_path.dst_pos.y = 0;
			out_path.hole_region.x = p_ctx->hole_region[pid].x;
			out_path.hole_region.y = p_ctx->hole_region[pid].y;
			out_path.hole_region.w = p_ctx->hole_region[pid].w;
			out_path.hole_region.h = p_ctx->hole_region[pid].h;
			//_vdoprc_vpe_dump_outpath(p_thisunit, &out_path);
			ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_OUT_PATH, (void *)&out_path);
			if (runtime_update == FALSE) {
				p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
			}
			// set h_algin
			{
				CTL_VPE_OUT_PATH_HALIGN h_aln;

				h_aln.pid = pid;
				h_aln.align = p_ctx->out_h_align[pid];
				ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_OUT_PATH_HALIGN, (void *)&h_aln);
				//DBG_ERR("set ime path %d, h_aln = %d\r\n", pid, p_ctx->out_h_align[pid]);
			}
		}
	}
	#if (USE_OUT_EXT == ENABLE)
	else if ((pid >= ISF_VDOPRC_PHY_OUT_NUM) && (pid < ISF_VDOPRC_OUT_NUM)) {
		 //extend path
		_vdoprc_config_out_ext(p_thisunit, pid, en);
	}
	#endif
	return ISF_OK;
}



/*
static void _vdoprc_oport_dumpdata(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j;
	unsigned long flags;
	loc_cpu(flags);
	DBG_DUMP("OutPOOL[%d]={", i);
	for (j = 0; j < p_ctx->outq.output_max[i]; j++) {
		if (p_ctx->outq.output_used[i][j] == 1) {
			DBG_DUMP("[%d]=%08x", j, p_ctx->outq.output_data[i][j].h_data);
		}
	}
	DBG_DUMP("}\r\n");
	unl_cpu(flags);
}
*/
void _isf_vdoprc_vpe_oport_do_out_cb(ISF_UNIT *p_thisunit, UINT32 event, void *p_in, void *p_out)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	CTL_VPE_OUT_BUF_INFO *p_info = (CTL_VPE_OUT_BUF_INFO *)p_in;
	VDO_FRAME* p_vdoframe = 0;
	UINT32 path, j;

	path = p_info->pid;
	//DBG_DUMP("%s[out%d]event=%d\r\n", p_thisunit->unit_name, path, event);

	if (event == CTL_VPE_BUF_NEW) { //new buffer

		UINT32 buf_size = p_info->buf_size;
		UINT32 buf_addr = (UINT32)&(p_info->vdo_frm);

		j = _isf_vdoprc_oport_do_new(p_thisunit, ISF_OUT(path), buf_size, p_ctx->ddr, &buf_addr);
		if (j == 0xff) {
			p_info->buf_id = 0xff;
			p_info->buf_addr = 0;
			if (p_ctx->vpe_mode && p_ctx->user_out_blk[path]) {
				DBG_IND("%s.out[%d]! no j!? Droped!\r\n", p_thisunit->unit_name, path);
			}
		} else {
			p_info->buf_id = j;
			p_info->buf_addr = buf_addr;
		}
		return;
	} else if (event == CTL_VPE_BUF_PUSH) { //push buffer
		j = p_info->buf_id;
		p_vdoframe = &(p_info->vdo_frm);
		_isf_vdoprc_oport_do_push(p_thisunit, ISF_OUT(path), j, p_vdoframe);
		return;
	} else if (event == CTL_VPE_BUF_LOCK) { //lock buffer
		j = p_info->buf_id;
		_isf_vdoprc_oport_do_lock(p_thisunit, ISF_OUT(path), j);
		return;
	} else if (event == CTL_VPE_BUF_UNLOCK) { //unlock buffer
		j = p_info->buf_id;
		_isf_vdoprc_oport_do_unlock(p_thisunit, ISF_OUT(path), j, p_info->err_msg);
		return;
	}
}
#endif

