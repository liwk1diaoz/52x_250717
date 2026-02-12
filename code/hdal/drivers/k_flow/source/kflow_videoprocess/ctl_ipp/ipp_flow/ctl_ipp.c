#include "kflow_videoprocess/ctl_ipp.h"
#include "kflow_common/isp_if.h"
#include "ctl_ipp_int.h"
#include "ctl_ipp_id_int.h"
#include "ctl_ipp_buf_int.h"
#include "ctl_ipp_flow_task_int.h"
#include "ctl_ipp_isp_int.h"
#include "kdf_ipp_int.h"
#include "ipp_event_int.h"
#include "ipp_event_id_int.h"
#include "comm/ddr_arb.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

/* low delay buffer push mode
	ENABLE:	push low delay buffer by thread
	DISABLE:push low delay buffer in ime_bp isr
*/
#define CTL_IPP_LOW_DLY_PUSH_THROUGH_THREAD	(ENABLE)

/*
	tmp, stripe rule select mode
	DISABLE -> Use gdc enable(old version)
	ENABLE -> Use strp rule
*/
#define CTL_IPP_STRP_RULE_SELECT_ENABLE (ENABLE)

/*
	DISABLE -> Direct mode pirvate buffer double buffer
	ENABLE  -> Direct mode pirvate buffer use one buffer (LCA & 3DNR)
*/
#define CTL_IPP_SINGLE_BUFFER_ENABLE (ENABLE)

/*
	direct task mode enable
	ENABLE  -> process sie bp3 event by thread
	DISABLE -> process sie bp3 event in isr
*/
#define CTL_IPP_DIRECT_PROC_THROUGH_THREAD (DISABLE)

#if 0
#endif

/*
    global variables
*/
static BOOL ctl_ipp_task_opened = FALSE;
static BOOL ctl_ipp_is_init = FALSE;
static UINT32 ctl_ipp_task_pause_count;
static CTL_IPP_HANDLE *p_ctl_ipp_direct_handle;
static CTL_IPP_HANDLE_POOL ctl_ipp_handle_pool;
static UINT32 dir_prv_sts = CTL_IPP_HANDLE_STS_MASK;
static BOOL ctl_ipp_dir_tsk_en = CTL_IPP_DIRECT_PROC_THROUGH_THREAD; /* reserve this setting to switch back to legacy direct process method */

#if 0
#endif

static UINT32 ctl_ipp_get_vr360_posotion(CTL_IPP_INFO_LIST_ITEM *ctl_info)
{
	VDO_FRAME *p_vdoin_info;
	UINT32 vdoin_crop_y;

	p_vdoin_info = (VDO_FRAME *)ctl_info->input_header_addr;
	/*vdoin_crop_x = (p_vdoin_info->reserved[5] & 0xffff0000) >> 16;*/
	vdoin_crop_y = (p_vdoin_info->reserved[5] & 0x0000ffff);

	if (vdoin_crop_y == ctl_info->info.ife_ctrl.in_crp_window.y) {
		return CTL_IPP_VR360_TOP;
	} else {
		return CTL_IPP_VR360_BOTTOM;
	}
}

static UINT32 ctl_ipp_vr360_adjust(CTL_IPP_HANDLE *ctrl_hdl, CTL_IPP_INFO_LIST_ITEM *ctrl_info)
{
	UINT32 i;
	UINT32 p;

	if (ctrl_hdl->p_last_rdy_ctrl_info == NULL) {
		CTL_IPP_DBG_IND("Null last rdy ctrl info\r\n");
		return CTL_IPP_E_PAR;
	}

	p = ctl_ipp_get_vr360_posotion(ctrl_info);
	if (p == CTL_IPP_VR360_BOTTOM) {
		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			if (ctrl_info->info.ime_ctrl.out_img[i].enable == ENABLE) {
				ctrl_info->buf_info[i] = ctrl_hdl->p_last_rdy_ctrl_info->buf_info[i];
				ctrl_info->info.ime_ctrl.out_img[i].size.w = ctrl_info->info.ime_ctrl.out_img[i].size.w >> 1;
				ctrl_info->info.ime_ctrl.out_img[i].crp_window.w = ctrl_info->info.ime_ctrl.out_img[i].crp_window.w >> 1;
				ctrl_info->info.ime_ctrl.out_img[i].addr[0] = ctrl_hdl->p_last_rdy_ctrl_info->buf_info[i].vdo_frm.addr[0] + ctrl_hdl->p_last_rdy_ctrl_info->buf_info[i].vdo_frm.loff[VDO_PINDEX_Y] / 2;
				ctrl_info->info.ime_ctrl.out_img[i].addr[1] = ctrl_hdl->p_last_rdy_ctrl_info->buf_info[i].vdo_frm.addr[1] + ctrl_hdl->p_last_rdy_ctrl_info->buf_info[i].vdo_frm.loff[VDO_PINDEX_U] / 2;
				ctrl_info->info.ime_ctrl.out_img[i].addr[2] = ctrl_hdl->p_last_rdy_ctrl_info->buf_info[i].vdo_frm.addr[2] + ctrl_hdl->p_last_rdy_ctrl_info->buf_info[i].vdo_frm.loff[VDO_PINDEX_V] / 2;
			}
		}
	} else if (p == CTL_IPP_VR360_TOP) {
		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			if (ctrl_info->info.ime_ctrl.out_img[i].enable == ENABLE) {
				ctrl_info->info.ime_ctrl.out_img[i].size.w = ctrl_info->info.ime_ctrl.out_img[i].size.w >> 1;
				ctrl_info->info.ime_ctrl.out_img[i].crp_window.w = ctrl_info->info.ime_ctrl.out_img[i].crp_window.w >> 1;
			}
		}
	} else {
		CTL_IPP_DBG_IND("Unknown vr360 position %d\r\n", (int)(p));
		return CTL_IPP_E_SYS;
	}

	return CTL_IPP_E_OK;
}

static UINT32 ctl_ipp_region_chk(CTL_IPP_INFO_LIST_ITEM *ctrl_info)
{
	CTL_IPP_IME_OUT_IMG *p_ime_path;
	UINT32 i;

	for (i = 0; i < CTL_IPP_IME_PATH_ID_MAX; i++) {
		p_ime_path = &ctrl_info->info.ime_ctrl.out_img[i];

		if (p_ime_path->region.enable) {
			if (p_ime_path->md_enable) {
				CTL_IPP_DBG_WRN_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "Region path (%u) can't enable md\r\n", i);
			}
			if ((p_ime_path->fmt & 0xfff) == 0x420 || (p_ime_path->fmt & 0xfff) == 0x422) {
				/* check crop x, y is 2 align */
				if ((p_ime_path->region.region_ofs.x & 1) || (p_ime_path->region.region_ofs.y & 1)) {
					CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "Region path (%u) ofs (%u, %u) should be 2x align when fmt 0x%.8x\r\n", i,
						p_ime_path->region.region_ofs.x, p_ime_path->region.region_ofs.y, p_ime_path->fmt);
					goto path_err;
				}
			}
			if ((p_ime_path->region.region_ofs.x + p_ime_path->crp_window.w) > p_ime_path->region.bgn_lofs ||
				(p_ime_path->region.region_ofs.y + p_ime_path->crp_window.h) > p_ime_path->region.bgn_size.h) {
				CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "Region path (%u) ofs (%u, %u) exceed bgn (%u, %u)\r\n", i,
					p_ime_path->region.region_ofs.x + p_ime_path->crp_window.w, p_ime_path->region.region_ofs.y + p_ime_path->crp_window.h,
					p_ime_path->region.bgn_lofs, p_ime_path->region.bgn_size.h);
				goto path_err;
			}

			continue;

path_err:
			p_ime_path->enable = 0;
			p_ime_path->dma_enable = 0;
		}
	}

	return CTL_IPP_E_OK;
}

static void ctl_ipp_inbuf_fp_wrapper(UINT32 hdl, UINT32 inbuf_fp_addr, CTL_IPP_CBEVT_IN_BUF_MSG cfg, CTL_IPP_EVT *p_evt)
{
	unsigned long loc_cpu_flg;
	VDO_FRAME *p_frm;
	VDO_FRAME *p_tmp_frm;
	CTL_IPP_HANDLE *ctrl_hdl;
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_EVT_UNION m_evt;
	UINT32 ts_start, ts_end;
	UINT8 frm_num;
	UINT8 i;

	ctrl_hdl = (CTL_IPP_HANDLE *)hdl;
	/* bufio callback */
	bufio_fp = (IPP_EVENT_FP)inbuf_fp_addr;
	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	/* frame number check, get frm_num when input raw(hdr mode) */
	p_frm = (VDO_FRAME *)p_evt->data_addr;
	if (ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
		frm_num = 1;
	} else if (VDO_PXLFMT_CLASS(p_frm->pxlfmt) == VDO_PXLFMT_CLASS_RAW ||
		VDO_PXLFMT_CLASS(p_frm->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
		frm_num = VDO_PXLFMT_PLANE(p_frm->pxlfmt);
	} else {
		frm_num = 1;
	}

	for (i = 0; i < frm_num; i++) {
		if (ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
			m_evt.dir_evt.buf_addr = p_frm->reserved[3];
			m_evt.dir_evt.buf_id = p_frm->reserved[2];
			m_evt.dir_evt.reserved = (UINT32)p_frm;
			p_tmp_frm = p_frm;
		} else {
			m_evt.dft_evt= *p_evt;
			m_evt.dft_evt.buf_id = (frm_num == 1) ? p_evt->buf_id : (p_evt->buf_id >> (8 * i)) & (0xFF);
			m_evt.dft_evt.data_addr = (i == 0) ? p_evt->data_addr : p_frm->addr[i];
			p_tmp_frm = (VDO_FRAME *)m_evt.dft_evt.data_addr;
		}

		/* parsing debug write protect information */
		// sie debug dma write protect, conflict with ipp debug dma write protect. dont use at the same time
		if ((p_tmp_frm->reserved[0] & 0xC0000000) == 0xC0000000) {
			arb_disable_wp((p_tmp_frm->reserved[0] & 0xF), ((p_tmp_frm->reserved[0] >> 4) & 0xF));
		}

		ts_start = ctl_ipp_util_get_syst_counter();
		bufio_fp(cfg, (void *)&m_evt, NULL);
		ts_end = ctl_ipp_util_get_syst_counter();
		loc_cpu(loc_cpu_flg);
		ctrl_hdl->in_buf_re_cb_cnt++;
		unl_cpu(loc_cpu_flg);

		/* debug information */
		ctl_ipp_dbg_inbuf_log_set(hdl, p_evt->data_addr, cfg, ts_start, ts_end);
	}
}

static void ctl_ipp_procend_sndbufevt_err_handle(CTL_IPP_HANDLE *p_hdl, UINT32 evt_type)
{
	UINT32 i;

	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		if (p_hdl->p_last_rdy_ctrl_info->buf_info[i].buf_addr != 0) {
			CTL_IPP_DBG_WRN("failed snd evt %d, p%d buf = 0x%.8x, force release in isr\r\n",
				(int)evt_type, (int)(i+1), (unsigned int)p_hdl->p_last_rdy_ctrl_info->buf_info[i].buf_addr);
			ctl_ipp_buf_release_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl->p_last_rdy_ctrl_info, FALSE, i, CTL_IPP_E_QOVR);
		}
	}

	ctl_ipp_info_release(p_hdl->p_last_rdy_ctrl_info);
}

static void ctl_ipp_procend_update_debug_info(CTL_IPP_HANDLE *p_hdl, CTL_IPP_INFO_LIST_ITEM *p_info)
{
	CTL_IPP_BASEINFO *p_base;

	if (p_info == NULL || p_hdl == NULL) {
		return ;
	}

	p_base = &p_info->info;
	if (p_base == NULL) {
		return ;
	}

	/* 3dnr */
	if (p_base->ime_ctrl.tplnr_enable == DISABLE ||
		p_base->ime_ctrl.tplnr_in_ref_addr[0] == 0 ||
		p_base->ime_ctrl.tplnr_out_mv_addr == 0 ||
		p_base->ime_ctrl.tplnr_out_ms_addr == 0) {
		p_hdl->last_3dnr_disable_frm = p_hdl->proc_end_cnt;
	}

	/* pattern paste */
	if (p_base->ime_ctrl.pat_paste_enable) {
		p_hdl->last_pattern_paste_frm = p_hdl->proc_end_cnt;
	}
}

void ctl_ipp_direct_set_tsk_en(BOOL en)
{
	ctl_ipp_dir_tsk_en = en;
}

BOOL ctl_ipp_direct_get_tsk_en(void)
{
	return ctl_ipp_dir_tsk_en;
}

#if 0
#endif


static CTL_IPP_HANDLE *ctl_ipp_get_hdl_by_kdf_hdl(UINT32 kdf_hdl)
{
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	vos_list_for_each_safe(p_list, n, &ctl_ipp_handle_pool.used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		if (p_hdl->kdf_hdl == kdf_hdl) {
			unl_cpu(loc_cpu_flg);
			return p_hdl;
		}
	}
	unl_cpu(loc_cpu_flg);

	return NULL;
}

static INT32 ctl_ipp_proc_time_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_DBG_TS_LOG ts_log;
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(msg);
	CTL_IPP_EVT_TIME_INFO *t = (CTL_IPP_EVT_TIME_INFO *)in;
	VDO_FRAME *p_frm;

	if (ctrl_hdl != NULL) {
		CTL_IPP_INFO_LIST_ITEM *p_info;

		p_info = ctl_ipp_info_get_entry_by_info_addr(t->proc_data_addr);
		memset((void *)&ts_log, 0, sizeof(CTL_IPP_DBG_TS_LOG));
		ts_log.handle = (UINT32)ctrl_hdl;
		if (p_info != NULL) {
			if(p_info->input_header_addr) {
				p_frm = (VDO_FRAME *)p_info->input_header_addr;
				ts_log.cnt = p_frm->count;
			} else if (ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
				ts_log.cnt = p_info->input_buf_id;
			}

			ts_log.ts[CTL_IPP_DBG_TS_SND] = p_info->ctl_snd_evt_time;
			ts_log.ts[CTL_IPP_DBG_TS_RCV] = p_info->ctl_rev_evt_time;
		}
		if (ctrl_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
			ts_log.ts[CTL_IPP_DBG_TS_KDF_SND] = t->snd;
			ts_log.ts[CTL_IPP_DBG_TS_KDF_RCV] = t->rev;
		} else {
			ts_log.ts[CTL_IPP_DBG_TS_KDF_SND] = ts_log.ts[CTL_IPP_DBG_TS_SND];
			ts_log.ts[CTL_IPP_DBG_TS_KDF_RCV] = ts_log.ts[CTL_IPP_DBG_TS_RCV];
		}
		ts_log.ts[CTL_IPP_DBG_TS_PROCSTART] = t->start;
		ts_log.ts[CTL_IPP_DBG_TS_TRIG] = t->trig;
		ts_log.ts[CTL_IPP_DBG_TS_BP] = t->ime_bp;
		ts_log.ts[CTL_IPP_DBG_TS_PROCEND] = t->end;
		ts_log.err_msg = t->err_msg;

		ctl_ipp_dbg_ts_set(ts_log);
	}

	return 0;
}

static INT32 ctl_ipp_proc_end_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_EVT ctrl_proc_end_evt;
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(msg);
	UINT32 ctl_ipp_info_addr;
	UINT32 ref_pi;

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	// flush in direct mode will wait proc end flag. need set flag before check 3dnr ref path handle (next few line)
	ctl_ipp_handle_set_proc_end(ctrl_hdl->lock);

	/* check 3dnr reference exist
		if not exist, use previous reference output
		d2d:	use last ready to replace current info
		direct:
			current_info exist, used current info to replace last_cfg
			current_info not exist, used last_readty to replace last_cfg
			note that tag failed can not handle
	*/
	ref_pi = ctrl_hdl->ctrl_info.ime_ctrl.tplnr_in_ref_path;
	if (in == NULL) {
		if ((ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) &&
			(ctrl_hdl->p_last_cfg_ctrl_info != NULL) &&
			(ctrl_hdl->p_last_cfg_ctrl_info->info.ime_ctrl.tplnr_enable) &&
			(ctrl_hdl->ctrl_info.ime_ctrl.out_img[ref_pi].enable)) {
			if (ctrl_hdl->p_last_rdy_ctrl_info != NULL) {
				ctl_ipp_buf_3dnr_refout_handle((UINT32)ctrl_hdl->evt_outbuf_fp, ctrl_hdl->p_last_rdy_ctrl_info, ctrl_hdl->p_last_cfg_ctrl_info);
			}
		}
		return CTL_IPP_E_OK;
	}

	ctrl_hdl->proc_end_cnt += 1;

	ctl_ipp_info_addr = *(UINT32 *)in;
	p_ctrl_info = ctl_ipp_info_get_entry_by_info_addr(ctl_ipp_info_addr);
	if ((p_ctrl_info != NULL) && (p_ctrl_info->info.ime_ctrl.tplnr_enable)) {
		if (ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
			if (ctrl_hdl->p_last_cfg_ctrl_info != NULL && (ctrl_hdl->ctrl_info.ime_ctrl.out_img[ref_pi].enable)) {
				ctl_ipp_buf_3dnr_refout_handle((UINT32)ctrl_hdl->evt_outbuf_fp, p_ctrl_info, ctrl_hdl->p_last_cfg_ctrl_info);
			}
		} else {
			if (ctrl_hdl->p_last_rdy_ctrl_info != NULL) {
				ctl_ipp_buf_3dnr_refout_handle((UINT32)ctrl_hdl->evt_outbuf_fp, ctrl_hdl->p_last_rdy_ctrl_info, p_ctrl_info);
			}
		}
	}


	if (ctrl_hdl->p_last_rdy_ctrl_info != NULL) {
		#if defined (__KERNEL__)
		/* release fastboot 3dnr reference buffer */
		UINT32 ref_path_sel = ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.tplnr_in_ref_path;

		if (kdrv_builtin_is_fastboot() &&
			ctrl_hdl->p_last_rdy_ctrl_info->info.is_fastboot_addr &&
			ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.tplnr_enable &&
			ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.out_img[ref_path_sel].addr[0]) {

			//DBG_DUMP("unlock fastboot blk 0x%.8x\r\n", ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.out_img[ref_path_sel].addr[0]);
			nvtmpp_unlock_fastboot_blk(ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.out_img[ref_path_sel].addr[0]);
			ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.out_img[ref_path_sel].addr[0] = 0;
		}
		#endif

		if (ctrl_hdl->flow ==  CTL_IPP_FLOW_VR360 && ctl_ipp_get_vr360_posotion(ctrl_hdl->p_last_rdy_ctrl_info) == CTL_IPP_VR360_TOP) {
			ctl_ipp_info_release(ctrl_hdl->p_last_rdy_ctrl_info);
		} else {
			/* trigger ctl_ipp_buf task to release buffer */
			if (ctl_ipp_buf_msg_snd(CTL_IPP_BUF_MSG_RELEASE, (UINT32)ctrl_hdl->evt_outbuf_fp, (UINT32)ctrl_hdl->p_last_rdy_ctrl_info, 0) != E_OK) {
				ctl_ipp_procend_sndbufevt_err_handle(ctrl_hdl, CTL_IPP_BUF_MSG_RELEASE);
			}
		}

		// debug dma write protect for previous frame reference buffer. finish protecting at frame end
		ctl_ipp_dbg_dma_wp_proc_end_ref(ctrl_hdl->p_last_rdy_ctrl_info);
		ctrl_hdl->p_last_rdy_ctrl_info = NULL;
	}

	/* update last ready ctrl info to handle */
	ctrl_hdl->p_last_rdy_ctrl_info = ctl_ipp_info_get_entry_by_info_addr(ctl_ipp_info_addr);
	if (ctrl_hdl->p_last_rdy_ctrl_info != NULL) {
		ctl_ipp_info_lock(ctrl_hdl->p_last_rdy_ctrl_info);

		/* udpate debug info */
		ctl_ipp_procend_update_debug_info(ctrl_hdl, ctrl_hdl->p_last_rdy_ctrl_info);

		// debug dma write protect for frame buffer and subout. finish protecting at frame end
		ctl_ipp_dbg_dma_wp_proc_end(ctrl_hdl->p_last_rdy_ctrl_info, TRUE);
		// debug dma write protect for privacy mask. finish protecting at frame end
		if (ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.pm_in_addr) {
			// if "previous" pm function is enable, then release "current" pm_in_addr. this is equal to "current pm_in_addr != 0"
			ctl_ipp_dbg_dma_wp_task_end(CTRL_IPP_BUF_ITEM_PM, CTL_IPP_BUF_ADDR_MAKE(ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.pm_in_addr, 0));
		}

		if (ctrl_hdl->flow ==  CTL_IPP_FLOW_VR360 && ctl_ipp_get_vr360_posotion(ctrl_hdl->p_last_rdy_ctrl_info) == CTL_IPP_VR360_TOP) {
			/* release ctrl info for allocate lock */
			ctl_ipp_info_release(ctrl_hdl->p_last_rdy_ctrl_info);
		} else if (CTL_IPP_IS_CAPTURE_FLOW(ctrl_hdl->flow) && ctrl_hdl->p_last_rdy_ctrl_info->evt_tag == CTL_IPP_EVT_TAG_SUBOUT_FOR_CAP) {
			/* release ctrl info for allocate lock */
			ctl_ipp_info_release(ctrl_hdl->p_last_rdy_ctrl_info);
		} else {
			/* inform frame end to out buffer cb */
			ctl_ipp_buf_frm_end((UINT32)ctrl_hdl->evt_outbuf_fp, (UINT32)ctrl_hdl->p_last_rdy_ctrl_info, 0);

			// debug dma write protect for yuv buffer. finish protecting at proc end task
			ctl_ipp_dbg_dma_wp_proc_end_push(ctrl_hdl->p_last_rdy_ctrl_info, TRUE);

			/* lock 3dnr ref buffer at proc end task instead of buf task (ctl_ipp_buf_push()) to prevent next frame's proc end may cause 3dnr ref buf be unlocked too early */
			if (ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.tplnr_enable) {
				CTL_IPP_OUT_BUF_INFO *buf_info = ctrl_hdl->p_last_rdy_ctrl_info->buf_info;
				CTL_IPP_IME_CTRL *ime_ctrl = &ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl;

				if (buf_info[ime_ctrl->tplnr_in_ref_path].buf_addr != 0 && ime_ctrl->out_img[ime_ctrl->tplnr_in_ref_path].dma_enable) {
					ctl_ipp_buf_fp_wrapper(p_ctrl_info->owner, (UINT32)ctrl_hdl->evt_outbuf_fp, CTL_IPP_BUF_IO_LOCK, &buf_info[ime_ctrl->tplnr_in_ref_path]);
				}
			}

			/* trigger ctl_ipp_buf task to push ready output */
			if (ctl_ipp_buf_msg_snd(CTL_IPP_BUF_MSG_PUSH, (UINT32)ctrl_hdl->evt_outbuf_fp, (UINT32)ctrl_hdl->p_last_rdy_ctrl_info, 0) != E_OK) {
				ctl_ipp_procend_sndbufevt_err_handle(ctrl_hdl, CTL_IPP_BUF_MSG_RELEASE);
			}

			/* trigger ctl_ipp_ise task to scale image */
			if (CTL_IPP_IS_CAPTURE_FLOW(ctrl_hdl->flow)) {
				/* do nothing, capture will call ise scale in primask callback */
			} else {
				if (ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.lca_out_enable && ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.pm_pixel_enable) {
					ctl_ipp_info_lock(ctrl_hdl->p_last_rdy_ctrl_info);
					if (ctl_ipp_ise_msg_snd(CTL_IPP_ISE_MSG_PROCESS, CTL_IPP_ISE_PM, (UINT32)ctrl_hdl->p_last_rdy_ctrl_info, (UINT32)ctrl_hdl) != E_OK) {
						ctl_ipp_info_release(ctrl_hdl->p_last_rdy_ctrl_info);
					}
				}
			}

			if (ctrl_hdl->evt_inbuf_fp != NULL) {
				ctrl_proc_end_evt.buf_id = ctrl_hdl->p_last_rdy_ctrl_info->input_buf_id;
				ctrl_proc_end_evt.data_addr = ctrl_hdl->p_last_rdy_ctrl_info->input_header_addr;
				ctrl_proc_end_evt.rev = 0;
				ctrl_proc_end_evt.err_msg = CTL_IPP_E_OK;
				if (ctrl_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
					/* normal mode */
					ctl_ipp_inbuf_fp_wrapper((UINT32)ctrl_hdl, (UINT32)ctrl_hdl->evt_inbuf_fp, CTL_IPP_CBEVT_IN_BUF_PROCEND, &ctrl_proc_end_evt);
				} else {
					/* do nothing */
				}
			}
		}

	}

	return 0;
}

static INT32 ctl_ipp_drop_cb(UINT32 msg, void *in, void *out)
{
	unsigned long loc_cpu_flg;
	KDF_IPP_EVT *kdf_drop_evt;
	CTL_IPP_HANDLE *ctrl_hdl;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_EVT ctrl_drop_evt;

	kdf_drop_evt = (KDF_IPP_EVT *)in;
	ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(msg);
	ctrl_info = ctl_ipp_info_get_entry_by_info_addr(kdf_drop_evt->data_addr);

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}
	if (ctrl_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW ||
		(ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW && kdf_drop_evt->err_msg == CTL_IPP_E_FLUSH) ||
		(ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW && kdf_drop_evt->err_msg == CTL_IPP_E_DIR_DROP)) {
		loc_cpu(loc_cpu_flg);
		ctrl_hdl->drop_cnt += 1;
		unl_cpu(loc_cpu_flg);

		/* release input buffer */
		if (ctrl_hdl->evt_inbuf_fp != NULL && ctrl_info != NULL) {
			if (CTL_IPP_IS_CAPTURE_FLOW(ctrl_hdl->flow) && ctrl_info->evt_tag == CTL_IPP_EVT_TAG_SUBOUT_FOR_CAP) {
				/* do nothing, capture only release input buffer at main event*/
			} else {
				ctrl_drop_evt.buf_id = ctrl_info->input_buf_id;
				ctrl_drop_evt.data_addr = ctrl_info->input_header_addr;
				ctrl_drop_evt.rev = 0;
				ctrl_drop_evt.err_msg = kdf_drop_evt->err_msg;
				if (ctrl_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
					ctl_ipp_inbuf_fp_wrapper((UINT32)ctrl_hdl, (UINT32)ctrl_hdl->evt_inbuf_fp, CTL_IPP_CBEVT_IN_BUF_DROP, &ctrl_drop_evt);
				} else {
					/* do nothing */
				}
			}
		} else {
			CTL_IPP_DBG_IND("drop_cb inbuf_cb failed, inbuf_fp 0x%.8x, ctrl_info 0x%.8x\r\n", (unsigned int)ctrl_hdl->evt_inbuf_fp, (unsigned int)ctrl_info);
		}
	}

	/* release output buffer */
	if (ctrl_info != NULL) {
		// debug dma write protect drop protect job
		ctl_ipp_dbg_dma_wp_drop(ctrl_info);

		ctl_ipp_buf_release((UINT32)ctrl_hdl->evt_outbuf_fp, (UINT32)ctrl_info, 0, kdf_drop_evt->err_msg);
	} else {
		CTL_IPP_DBG_IND("drop_cb buf_release failed, ctrl_info 0x%.8x\r\n", (unsigned int)ctrl_info);
	}

	return 0;
}

static INT32 ctl_ipp_proc_start_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_EVT ctrl_proc_start_evt;
	CTL_IPP_INFO_LIST_ITEM *p_info;
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(msg);
	UINT32 ctl_ipp_info_addr = *(UINT32 *)in;

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	p_info = ctl_ipp_info_get_entry_by_info_addr(ctl_ipp_info_addr);

	/* proc start event callback */
	if (ctrl_hdl->evt_inbuf_fp != NULL && p_info != NULL) {
		if (CTL_IPP_IS_CAPTURE_FLOW(ctrl_hdl->flow) && p_info->evt_tag == CTL_IPP_EVT_TAG_SUBOUT_FOR_CAP) {
			/* do nothing, capture onyl callback at main event */
		} else {
			ctrl_proc_start_evt.buf_id = p_info->input_buf_id;
			ctrl_proc_start_evt.data_addr = p_info->input_header_addr;
			ctrl_proc_start_evt.rev = 0;
			ctrl_proc_start_evt.err_msg = CTL_IPP_E_OK;
			if (ctrl_proc_start_evt.data_addr != 0){
				ctl_ipp_inbuf_fp_wrapper((UINT32)ctrl_hdl, (UINT32)ctrl_hdl->evt_inbuf_fp, CTL_IPP_CBEVT_IN_BUF_PROCSTART, &ctrl_proc_start_evt);
			}
		}
	}

	return 0;
}

static INT32 ctl_ipp_frm_start_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(msg);
	UINT32 ctl_ipp_info_addr = *(UINT32 *)in;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	UINT32 i;

	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_IND("ctl hdl NULL\r\n");
		return CTL_IPP_E_ID;
	}

	ctrl_hdl->ctl_frm_str_cnt += 1;
	ctrl_info = ctl_ipp_info_get_entry_by_info_addr(ctl_ipp_info_addr);
	if (ctrl_info != NULL) {
		/* direct mode set time stamp */
		for(i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			if(ctrl_info->buf_info[i].buf_addr != 0 && ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
				ctrl_info->buf_info[i].vdo_frm.timestamp = ctl_ipp_util_get_syst_timestamp();
			}
		}
		/* frm start event callback for outbuf */
		ctl_ipp_buf_frm_start((UINT32)ctrl_hdl->evt_outbuf_fp, (UINT32)ctrl_info, 0);
	}



	return 0;
}

static INT32 ctl_ipp_low_delay_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_INFO_LIST_ITEM *p_info;
	CTL_IPP_HANDLE *ctrl_hdl;
	UINT32 ctl_ipp_info_addr;

	ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(msg);
	ctl_ipp_info_addr = *(UINT32 *)in;
	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	p_info = ctl_ipp_info_get_entry_by_info_addr(ctl_ipp_info_addr);

	if (p_info != NULL) {
		#if CTL_IPP_LOW_DLY_PUSH_THROUGH_THREAD
		ctl_ipp_buf_msg_snd(CTL_IPP_BUF_MSG_PUSH_LOWDLY, (UINT32)ctrl_hdl->evt_outbuf_fp, (UINT32)p_info, 0);
		#else
		ctl_ipp_buf_push_low_delay((UINT32)ctrl_hdl->evt_outbuf_fp, (UINT32)p_info, 0);
		#endif
	}

	return 0;
}

static INT32 ctl_ipp_rhe_isr_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(*(UINT32 *)in);

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	if (ctrl_hdl->ipp_evt_hdl != 0) {
		ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_ENG_RHE_ISR, (void *)&msg, NULL);
	}

	return 0;
}

static INT32 ctl_ipp_ife_isr_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(*(UINT32 *)in);

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	if (msg & (KDRV_IFE_INTE_BUFOVFL | KDRV_IFE_INTE_FRAME_ERR)) {
		if (ctrl_hdl->func_en & CTL_IPP_FUNC_DIRECT_SCL_UP) {
			UINT32 i = 0, max_out_id = 0;
			USIZE max_size = {0};

			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				if (ctrl_hdl->ctrl_info.ime_ctrl.out_img[i].enable
				&& (ctrl_hdl->ctrl_info.ime_ctrl.out_img[i].size.w * ctrl_hdl->ctrl_info.ime_ctrl.out_img[i].size.h) > (max_size.w * max_size.h)) {
					max_out_id = i;
					max_size.w = ctrl_hdl->ctrl_info.ime_ctrl.out_img[i].size.w;
					max_size.h = ctrl_hdl->ctrl_info.ime_ctrl.out_img[i].size.h;
				}
			}

			if ((max_size.w*max_size.h) > (ctrl_hdl->ctrl_info.ime_ctrl.in_size.w * ctrl_hdl->ctrl_info.ime_ctrl.in_size.h)) {
				CTL_IPP_DBG_ERR_FREQ(120,"IME DIRECT_SCL_UP EN, ime_in(%d, %d), out_path %d, size (%d, %d)\r\n",
					ctrl_hdl->ctrl_info.ime_ctrl.in_size.w, ctrl_hdl->ctrl_info.ime_ctrl.in_size.h,
					max_out_id + 1, max_size.w, max_size.h);
				}
		}
	}

	if (ctrl_hdl->ipp_evt_hdl != 0) {
		ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_ENG_IFE_ISR, (void *)&msg, NULL);
	}

	return 0;
}

static INT32 ctl_ipp_dce_isr_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(*(UINT32 *)in);

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	if (ctrl_hdl->ipp_evt_hdl != 0) {
		ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_ENG_DCE_ISR, (void *)&msg, NULL);
	}

	return 0;
}

static INT32 ctl_ipp_ipe_isr_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(*(UINT32 *)in);

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	if (ctrl_hdl->ipp_evt_hdl != 0) {
		ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_ENG_IPE_ISR, (void *)&msg, NULL);
	}

	return 0;
}

static INT32 ctl_ipp_ime_isr_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(*(UINT32 *)in);

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	if (ctrl_hdl->ipp_evt_hdl != 0) {
		ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_ENG_IME_ISR, (void *)&msg, NULL);
	}


	return 0;
}

static INT32 ctl_ipp_ife2_isr_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_HANDLE *ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(*(UINT32 *)in);

	if (ctrl_hdl == NULL) {
		return CTL_IPP_E_ID;
	}

	if (ctrl_hdl->ipp_evt_hdl != 0) {
		ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_ENG_IFE2_ISR, (void *)&msg, NULL);
	}


	return 0;
}

static void ctl_ipp_update_reference(CTL_IPP_HANDLE *hdl, CTL_IPP_INFO_LIST_ITEM *ctl_info)
{
	UINT32 pi;
	CTL_IPP_BASEINFO *base_info;
	CTL_IPP_BASEINFO *prev_info;
	CTL_IPP_INFO_LIST_ITEM* p_last_ctl;

	if (hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
		p_last_ctl = hdl->p_last_cfg_ctrl_info;
	} else {
		p_last_ctl = hdl->p_last_rdy_ctrl_info;
	}

	if (p_last_ctl == NULL) {
		/* no previous image */
		base_info = &ctl_info->info;
		if (hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
			/* set dummy data for IFE2 */
			base_info->ife2_ctrl.in_addr = ctl_info->info.ime_ctrl.lca_out_addr;
			base_info->ife2_ctrl.in_size = ctl_info->info.ime_ctrl.lca_out_size;
			base_info->ife2_ctrl.in_lofs = ctl_info->info.ime_ctrl.lca_out_lofs;
		}

		/* set dummy data for pixelation. first frame at first open ipp may get garbage mosaic result. second open ipp without hdal uninit should retain last frame mosaic effect */
		base_info->ime_ctrl.pm_in_addr = ctl_ipp_buf_pri_search(hdl, 0, CTRL_IPP_BUF_ITEM_PM, (UINT32 *)hdl->private_buf.buf_item.pm);
		// set pm_in_lofs = 0 to inform kdf_ipp_config_ime_privacy_mask() that set pm size/lofs/blk_size by itself (because these info. are provided by KDF_IPP_CBEVT_PRIMASK)
		base_info->ime_ctrl.pm_in_lofs = 0;
		return ;
	}

	base_info = &ctl_info->info;
	prev_info = &p_last_ctl->info;

	/* DCE */
	base_info->dce_ctrl.wdr_in_addr = prev_info->dce_ctrl.wdr_out_addr;

	/* IPE */
	base_info->ipe_ctrl.defog_in_addr = prev_info->ipe_ctrl.defog_out_addr;

	/* IME */
	base_info->ime_ctrl.lca_in_size = prev_info->ime_ctrl.lca_out_size;
	base_info->ime_ctrl.lca_in_lofs = prev_info->ime_ctrl.lca_out_lofs;

	if (base_info->ime_ctrl.tplnr_enable) {
		pi = base_info->ime_ctrl.tplnr_in_ref_path;
		if (pi >= CTL_IPP_OUT_PATH_ID_MAX) {
			CTL_IPP_DBG_WRN("ref_path out of range %d\r\n", (int)(pi));
		} else {
			CTL_IPP_OUT_BUF_INFO *p_ref_buf;
			BOOL ref_img_exist;

			p_ref_buf = &p_last_ctl->buf_info[pi];
			/* check buffer due to buffer may be flushed
				special case for fastboot first handle, first frame
			*/
			if (p_ref_buf->buf_addr != 0 &&
				prev_info->ime_ctrl.out_img[pi].dma_enable == ENABLE) {
				ref_img_exist = TRUE;
			} else {
				ref_img_exist = FALSE;
			}
			#if defined (__KERNEL__)
			if (kdrv_builtin_is_fastboot() &&
				prev_info->is_fastboot_addr &&
				prev_info->ime_ctrl.out_img[pi].addr[0]) {
				ref_img_exist = TRUE;
			}
			#endif

			if (ref_img_exist) {
				BOOL ref_img_pass = TRUE;

				/* check size */
				if (prev_info->ime_ctrl.out_img[pi].crp_window.w != base_info->ime_ctrl.in_size.w ||
					prev_info->ime_ctrl.out_img[pi].crp_window.h != base_info->ime_ctrl.in_size.h) {
					ref_img_pass = FALSE;
					CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "3dnr size not match, ref_path_%d, ref(%d, %d), ime_in(%d, %d)\r\n",
						(int)(pi + 1), (int)prev_info->ime_ctrl.out_img[pi].crp_window.w, (int)prev_info->ime_ctrl.out_img[pi].crp_window.h,
						(int)base_info->ime_ctrl.in_size.w, (int)base_info->ime_ctrl.in_size.h);
				}

				if (prev_info->ime_ctrl.out_img[pi].fmt != VDO_PXLFMT_YUV420 &&
					prev_info->ime_ctrl.out_img[pi].fmt != VDO_PXLFMT_Y8 &&
					prev_info->ime_ctrl.out_img[pi].fmt != VDO_PXLFMT_520_IR8 &&
					VDO_PXLFMT_CLASS(prev_info->ime_ctrl.out_img[pi].fmt) != VDO_PXLFMT_CLASS_NVX) {
					ref_img_pass = FALSE;
					CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "3dnr unsupport fmt 0x%.8x\r\n", (unsigned int)prev_info->ime_ctrl.out_img[pi].fmt);
				}

				if (ref_img_pass) {
					base_info->ime_ctrl.tplnr_in_ref_addr[0] = prev_info->ime_ctrl.out_img[pi].addr[0];
					base_info->ime_ctrl.tplnr_in_ref_addr[1] = prev_info->ime_ctrl.out_img[pi].addr[1];
					base_info->ime_ctrl.tplnr_in_ref_addr[2] = prev_info->ime_ctrl.out_img[pi].addr[2];
					base_info->ime_ctrl.tplnr_in_ref_pa[0] = prev_info->ime_ctrl.out_img[pi].pa[0];
					base_info->ime_ctrl.tplnr_in_ref_pa[1] = prev_info->ime_ctrl.out_img[pi].pa[1];
					base_info->ime_ctrl.tplnr_in_ref_pa[2] = prev_info->ime_ctrl.out_img[pi].pa[2];
					base_info->ime_ctrl.tplnr_in_ref_lofs[0] = prev_info->ime_ctrl.out_img[pi].lofs[0];
					base_info->ime_ctrl.tplnr_in_ref_lofs[1] = prev_info->ime_ctrl.out_img[pi].lofs[1];
					base_info->ime_ctrl.tplnr_in_ref_lofs[2] = prev_info->ime_ctrl.out_img[pi].lofs[2];
					base_info->ime_ctrl.tplnr_in_ref_flip_enable = prev_info->ime_ctrl.out_img[pi].flip_enable;

					base_info->ime_ctrl.tplnr_in_mv_addr = prev_info->ime_ctrl.tplnr_out_mv_addr;
					base_info->ime_ctrl.tplnr_in_ms_addr = prev_info->ime_ctrl.tplnr_out_ms_addr;

					if ((VDO_PXLFMT_CLASS(prev_info->ime_ctrl.out_img[pi].fmt) == VDO_PXLFMT_CLASS_NVX)) {
						base_info->ime_ctrl.tplnr_in_dec_enable = ENABLE;
					} else {
						base_info->ime_ctrl.tplnr_in_dec_enable = DISABLE;
					}

					if ((prev_info->ime_ctrl.out_img[pi].fmt == VDO_PXLFMT_Y8) || (prev_info->ime_ctrl.out_img[pi].fmt == VDO_PXLFMT_520_IR8)) {
						base_info->ime_ctrl.tplnr_in_ref_addr[1] = base_info->ime_ctrl.tplnr_in_ref_addr[0];
						base_info->ime_ctrl.tplnr_in_ref_addr[2] = base_info->ime_ctrl.tplnr_in_ref_addr[0];
						base_info->ime_ctrl.tplnr_in_ref_lofs[1] = base_info->ime_ctrl.tplnr_in_ref_lofs[0];
						base_info->ime_ctrl.tplnr_in_ref_lofs[2] = base_info->ime_ctrl.tplnr_in_ref_lofs[0];
					}
				}
			} else {
				CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "3dnr disable due to reference image not exist evt_count(%d)\r\n", p_last_ctl->kdf_evt_count);
			}
		}
	}

	/* get pm last ready image
		check if ime input size change --> disable pixelation by setting pm_in_addr to 0
	*/
	if (base_info->ime_ctrl.pm_enable && base_info->ime_ctrl.pm_pixel_enable) {
		CTL_IPP_ISE_OUT_INFO pm_last_rdy;

		pm_last_rdy.hdl_addr = (UINT32)hdl;
		if (ctl_ipp_ise_get_last_rdy(CTL_IPP_ISE_PM, &pm_last_rdy) == E_OK) {
			base_info->ime_ctrl.pm_in_addr = pm_last_rdy.addr;
			base_info->ime_ctrl.pm_in_lofs = pm_last_rdy.lofs;
			base_info->ime_ctrl.pm_in_size = pm_last_rdy.size;
			base_info->ime_ctrl.pm_pxl_blk = pm_last_rdy.pixel_blk;
			if (prev_info->ime_ctrl.in_size.w != base_info->ime_ctrl.in_size.w ||
				prev_info->ime_ctrl.in_size.h != base_info->ime_ctrl.in_size.h) {
				base_info->ime_ctrl.pm_in_addr = 0;
			}
		} else {
			CTL_IPP_DBG_WRN("get pm last rdy info fail (fcnt %u)\r\n", ctl_info->kdf_evt_count);
			base_info->ime_ctrl.pm_in_addr = 0;
			base_info->ime_ctrl.pm_in_lofs = 0;
			base_info->ime_ctrl.pm_in_size.w = 0;
			base_info->ime_ctrl.pm_in_size.h = 0;
			base_info->ime_ctrl.pm_pxl_blk = 0;
		}
	}

	/* IFE2 */
	base_info->ife2_ctrl.in_addr = prev_info->ime_ctrl.lca_out_addr;
	base_info->ife2_ctrl.in_size = prev_info->ime_ctrl.lca_out_size;
	base_info->ife2_ctrl.in_lofs = prev_info->ime_ctrl.lca_out_lofs;
	base_info->ife2_ctrl.gray_avg_u = prev_info->ife2_ctrl.gray_avg_u;
	base_info->ife2_ctrl.gray_avg_v = prev_info->ife2_ctrl.gray_avg_v;

	if ((base_info->ife_ctrl.flip & CTL_IPP_FLIP_H) != (prev_info->ife_ctrl.flip & CTL_IPP_FLIP_H)) {
		CTL_IPP_DBG_TRC("ife mirror type change, Set ref buf 0\r\n");

		base_info->dce_ctrl.wdr_in_addr = 0;
		base_info->ipe_ctrl.defog_in_addr = 0;
		base_info->ime_ctrl.tplnr_in_ref_addr[0] = 0;
		base_info->ime_ctrl.tplnr_in_ref_addr[1] = 0;
		base_info->ime_ctrl.tplnr_in_ref_addr[2] = 0;
		base_info->ime_ctrl.tplnr_in_mv_addr = 0;
		base_info->ime_ctrl.tplnr_in_ms_addr = 0;
		base_info->ife2_ctrl.in_addr = 0;
	}
}


INT32 ctl_ipp_preset_cb(UINT32 msg, void *in, void *out)
{
	/* PRESET callback for KDF layer
		update referece info from last ready
		update private buffer output address
		allocate public buffer
	*/

	CTL_IPP_HANDLE *ctrl_hdl;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;

	ctrl_info = ctl_ipp_info_get_entry_by_info_addr((UINT32)in);
	if (ctrl_info == NULL) {
		return CTL_IPP_E_ID;
	}

	ctrl_hdl = (CTL_IPP_HANDLE *)ctrl_info->owner;
	if (ctl_ipp_handle_validate(ctrl_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	if (ctrl_hdl->evt_outbuf_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return CTL_IPP_E_ID;
	}

	/* vr360 proccess for bottom half */
	if (ctrl_hdl->flow == CTL_IPP_FLOW_VR360) {
		if (ctl_ipp_get_vr360_posotion(ctrl_info) == CTL_IPP_VR360_BOTTOM) {
			ctl_ipp_vr360_adjust(ctrl_hdl, ctrl_info);
			return CTL_IPP_E_OK;
		}
	}

	/* creat dummy last rdy info for fastboot to hdal first frame */
	#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot()) {
		KDRV_IPP_BUILTIN_CTL_INFO *p_builtin_info = NULL;
		CTL_IPP_INFO_LIST_ITEM *p_dummy_info = NULL;
		UINT32 i;

		/* replace output addr info base on fastboot info */
		if (CTL_IPP_IS_DIRECT_FLOW(ctrl_hdl->flow)) {
			/* replace first last cfg info(already exist when start) for direct mode */
			if (ctrl_hdl->p_last_cfg_ctrl_info && ctrl_hdl->p_last_cfg_ctrl_info->kdf_evt_count == 0) {
				p_dummy_info = ctrl_hdl->p_last_cfg_ctrl_info;
				p_builtin_info = kdrv_ipp_builtin_get_ctl_info(ctrl_hdl->name);
			}
		} else {
			/* alloc dummy info for dram mode first frame */
			if (ctrl_hdl->ctl_frm_str_cnt == 0) {
				p_builtin_info = kdrv_ipp_builtin_get_ctl_info(ctrl_hdl->name);
				if (p_builtin_info) {
					p_dummy_info = ctl_ipp_info_alloc((UINT32)ctrl_hdl);
				}
			}
		}
		if (p_dummy_info && p_builtin_info) {
			UINT32 last_ctl_frm_sta_count = p_dummy_info->info.ctl_frm_sta_count;

			memcpy((void *)&p_dummy_info->info, (void *)&ctrl_info->info, sizeof(CTL_IPP_BASEINFO));

			// fix issue that checking in kdf_ipp_direct_datain_drop(KDF_IPP_FRAME_BP) will fail caused by old job's ctl_frm_sta_count be updated by memcpy above
			// save & recover old job's ctl_frm_sta_count to fix this issue
			if (CTL_IPP_IS_DIRECT_FLOW(ctrl_hdl->flow)) {
				p_dummy_info->info.ctl_frm_sta_count = last_ctl_frm_sta_count;
			}

			p_dummy_info->info.dce_ctrl.wdr_out_addr = nvtmpp_sys_pa2va(p_builtin_info->wdr_addr);
			p_dummy_info->info.ipe_ctrl.defog_out_addr = nvtmpp_sys_pa2va(p_builtin_info->defog_addr);
			p_dummy_info->info.ime_ctrl.lca_out_addr = nvtmpp_sys_pa2va(p_builtin_info->lca_addr);
			p_dummy_info->info.ime_ctrl.tplnr_out_mv_addr = nvtmpp_sys_pa2va(p_builtin_info->_3dnr_mv_addr);
			p_dummy_info->info.ime_ctrl.tplnr_out_ms_addr = nvtmpp_sys_pa2va(p_builtin_info->_3dnr_ms_addr);
			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				p_dummy_info->info.ime_ctrl.out_img[i].pa[0] = p_builtin_info->path_addr_y[i];
				p_dummy_info->info.ime_ctrl.out_img[i].pa[1] = p_builtin_info->path_addr_u[i];
				p_dummy_info->info.ime_ctrl.out_img[i].pa[2] = p_builtin_info->path_addr_v[i];
				p_dummy_info->info.ime_ctrl.out_img[i].addr[0] = nvtmpp_sys_pa2va(p_builtin_info->path_addr_y[i]);
				p_dummy_info->info.ime_ctrl.out_img[i].addr[1] = nvtmpp_sys_pa2va(p_builtin_info->path_addr_u[i]);
				p_dummy_info->info.ime_ctrl.out_img[i].addr[2] = nvtmpp_sys_pa2va(p_builtin_info->path_addr_v[i]);
			}
			p_dummy_info->info.is_fastboot_addr = TRUE;

			p_dummy_info->info.ife2_ctrl.gray_avg_u = p_builtin_info->gray_avg_u;
			p_dummy_info->info.ife2_ctrl.gray_avg_v = p_builtin_info->gray_avg_v;
			//DBG_DUMP("%s replace info from fastboot\r\n", ctrl_hdl->name);

			if (!CTL_IPP_IS_DIRECT_FLOW(ctrl_hdl->flow)) {
				/* replace last rdy info for dram mode */
				ctrl_hdl->p_last_rdy_ctrl_info = p_dummy_info;
			}
		}
	}

	#endif

	if (ctl_ipp_region_chk(ctrl_info) != CTL_IPP_E_OK) {
		return CTL_IPP_E_PAR;
	}

	/* update address from inner buffer */
	ctl_ipp_buf_pri_alloc(ctrl_hdl, ctrl_info);

	/* update reference info from last ready */
	ctl_ipp_update_reference(ctrl_hdl, ctrl_info);

	/* allocate Buffer by bufio_cb for ipl output */
	ctl_ipp_buf_alloc(ctrl_hdl, ctrl_info);

	/* capture flow or pattern paste adjust */
	if ((CTL_IPP_IS_CAPTURE_FLOW(ctrl_hdl->flow) && ctrl_info->evt_tag == CTL_IPP_EVT_TAG_SUBOUT_FOR_CAP) ||
		(!ctrl_info->info.ime_ctrl.pat_paste_enable && // current frame is back to normal
			ctrl_hdl->p_last_rdy_ctrl_info && ctrl_hdl->p_last_rdy_ctrl_info->info.ime_ctrl.pat_paste_enable)) { // previous frame is pattern paste
		CTL_IPP_BASEINFO *base_info;

		CTL_IPP_DBG_TRC("Set ref buf 0\r\n");

		base_info = &ctrl_info->info;
		base_info->dce_ctrl.wdr_in_addr = 0;
		base_info->ipe_ctrl.defog_in_addr = 0;
		base_info->ime_ctrl.tplnr_in_ref_addr[0] = 0;
		base_info->ime_ctrl.tplnr_in_ref_addr[1] = 0;
		base_info->ime_ctrl.tplnr_in_ref_addr[2] = 0;
		base_info->ime_ctrl.tplnr_in_mv_addr = 0;
		base_info->ime_ctrl.tplnr_in_ms_addr = 0;
		base_info->ime_ctrl.pm_in_addr = 0;
		base_info->ife2_ctrl.in_addr = 0;
	}

	/* vr360 proccess for top half */
	if (ctrl_hdl->flow == CTL_IPP_FLOW_VR360) {
		ctl_ipp_vr360_adjust(ctrl_hdl, ctrl_info);
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_datastamp_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_DS_CB_INPUT_INFO *dscb_in;
	CTL_IPP_HANDLE *ctrl_hdl;
	CTL_IPP_INFO_LIST_ITEM *p_info;

	dscb_in = (CTL_IPP_DS_CB_INPUT_INFO *)in;
	p_info = ctl_ipp_info_get_entry_by_info_addr(msg);
	if (p_info == NULL) {
		return CTL_IPP_E_PAR;
	}

	ctrl_hdl = (CTL_IPP_HANDLE *)p_info->owner;
	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_WRN("CTRL HANDLE NULL\r\n");
		return CTL_IPP_E_ID;
	}

	if (ctrl_hdl->ipp_evt_hdl != 0) {
		if (CTL_IPP_IS_CAPTURE_FLOW(ctrl_hdl->flow)) {
			if (p_info->evt_tag == CTL_IPP_EVT_TAG_MAINFRM_FOR_CAP) {
				dscb_in->ctl_ipp_handle = (UINT32)ctrl_hdl;
				ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_DATASTAMP, in, out);
			}
		} else {
			dscb_in->ctl_ipp_handle = (UINT32)ctrl_hdl;
			ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_DATASTAMP, in, out);
		}
	}

	return 0;
}

INT32 ctl_ipp_primask_cb_trig_ise(CTL_IPP_INFO_LIST_ITEM *p_info, CTL_IPP_HANDLE *ctrl_hdl, CTL_IPP_PM_CB_OUTPUT_INFO *p_pmcb_out_info)
{
	CTL_IPP_INFO_LIST_ITEM *p_info_prev;
	CTL_IPP_ISE_OUT_INFO ise_out = {0};

	p_info_prev = ctrl_hdl->p_last_rdy_ctrl_info;
	if (p_info_prev != NULL) {
		p_info_prev->info.ime_ctrl.pm_pxl_blk = p_pmcb_out_info->pxl_blk_size;
		ctl_ipp_ise_process_oneshot(CTL_IPP_ISE_PM, (UINT32)p_info_prev, &ise_out);

		p_info->info.ime_ctrl.pm_in_addr = ise_out.addr;
		p_info->info.ime_ctrl.pm_in_lofs = ise_out.lofs;
		p_info->info.ime_ctrl.pm_in_size = ise_out.size;
		p_info->info.ime_ctrl.pm_pxl_blk = ise_out.pixel_blk;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_primask_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_PM_CB_INPUT_INFO *dscb_in;
	CTL_IPP_HANDLE *ctrl_hdl;
	CTL_IPP_INFO_LIST_ITEM *p_info;

	dscb_in = (CTL_IPP_PM_CB_INPUT_INFO *)in;
	p_info = ctl_ipp_info_get_entry_by_info_addr(msg);
	if (p_info == NULL) {
		return CTL_IPP_E_PAR;
	}

	ctrl_hdl = (CTL_IPP_HANDLE *)p_info->owner;
	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_WRN("CTRL HANDLE NULL\r\n");
		return CTL_IPP_E_ID;
	}

	dscb_in->ctl_ipp_handle = (UINT32)ctrl_hdl;
	if (ctrl_hdl->ipp_evt_hdl != 0) {
		if (CTL_IPP_IS_CAPTURE_FLOW(ctrl_hdl->flow)) {
			if (p_info->evt_tag == CTL_IPP_EVT_TAG_MAINFRM_FOR_CAP) {
				ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_PRIMASK, in, out);
				if (p_info->info.ime_ctrl.pm_pixel_enable == ENABLE) {
					ctl_ipp_primask_cb_trig_ise(p_info, ctrl_hdl, (CTL_IPP_PM_CB_OUTPUT_INFO *)out);
				}
			}
		} else {
			ipp_event_proc_isr(ctrl_hdl->ipp_evt_hdl, CTL_IPP_CBEVT_PRIMASK, in, out);
		}
	}

	/* debug privacy mask callback */
	ctl_ipp_dbg_primask_cb(CTL_IPP_CBEVT_PRIMASK, in, out);

	return 0;
}

INT32 ctl_ipp_isp_cb(UINT32 msg, void *in, void *out)
{
	KDF_IPP_ISP_CB_INFO *iqcb_in;
	CTL_IPP_HANDLE *ctrl_hdl;
	CTL_IPP_INFO_LIST_ITEM *p_info;
	INT32 rt;

	ctrl_hdl = ctl_ipp_get_hdl_by_kdf_hdl(msg);
	iqcb_in = (KDF_IPP_ISP_CB_INFO *)in;
	rt = E_OK;

	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_WRN("CTRL HANDLE NULL\r\n");
		return CTL_IPP_E_ID;
	}

	if (iqcb_in->type == KDF_IPP_ISP_CB_TYPE_IQ_TRIG) {
		/* do not trigger iq when direct mode in stop status */
		if (ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
			if (ctrl_hdl->sts != CTL_IPP_HANDLE_STS_DIR_START && ctrl_hdl->sts != CTL_IPP_HANDLE_STS_DIR_READY) {
				return CTL_IPP_E_OK;
			}
		}
		p_info = ctl_ipp_info_get_entry_by_info_addr(iqcb_in->base_info_addr);

		/* do not trigger iq when direct mode start switch from fastboot */
		if ((p_info != NULL) && (p_info->evt_tag == CTL_IPP_EVT_TAG_SKIP_ISP_FOR_FASTBOOT)) {
			return CTL_IPP_E_OK;
		}
		rt |= ctl_ipp_isp_event_cb(ctrl_hdl->isp_id[CTL_IPP_ALGID_IQ], iqcb_in->item, p_info, NULL);
	} else if (iqcb_in->type == KDF_IPP_ISP_CB_TYPE_IQ_GET) {
		rt |= ctl_ipp_isp_int_get(ctrl_hdl, iqcb_in->item, (void *)iqcb_in->data_addr);
	} else if (iqcb_in->type == KDF_IPP_ISP_CB_TYPE_INT_EVT) {
		rt |= ctl_ipp_isp_int_event_cb(ctrl_hdl->isp_id[CTL_IPP_ALGID_IQ], iqcb_in->item);
	} else {
		CTL_IPP_DBG_WRN("Unknown isp callback type %d\r\n", (int)(iqcb_in->type));
		rt = CTL_IPP_E_PAR;
	}

	return rt;
}


#if 0
#endif

/*
    internal used function
*/
static void ctl_ipp_handle_pool_lock(void)
{
	FLGPTN flg;

	vos_flag_wait(&flg, ctl_ipp_handle_pool.flg_id[0], CTL_IPP_HANDLE_POOL_LOCK, TWF_CLR);
}

static void ctl_ipp_handle_pool_unlock(void)
{
	vos_flag_set(ctl_ipp_handle_pool.flg_id[0], CTL_IPP_HANDLE_POOL_LOCK);
}

static CTL_IPP_HANDLE_LOCKINFO ctl_ipp_handle_get_lockinfo(UINT32 lock)
{
	CTL_IPP_HANDLE_LOCKINFO rt;

	rt.flag = (UINT32)(lock / CTL_IPP_UTIL_FLAG_PAGE_SIZE);
	rt.ptn = 1 << (lock % CTL_IPP_UTIL_FLAG_PAGE_SIZE);

	return rt;
}

void ctl_ipp_handle_lock(UINT32 lock)
{
	FLGPTN flg;
	CTL_IPP_HANDLE_LOCKINFO lock_info;

	lock_info = ctl_ipp_handle_get_lockinfo(lock);

	vos_flag_wait(&flg, ctl_ipp_handle_pool.flg_id[lock_info.flag], lock_info.ptn, TWF_CLR);
}

void ctl_ipp_handle_unlock(UINT32 lock)
{
	CTL_IPP_HANDLE_LOCKINFO lock_info;

	lock_info = ctl_ipp_handle_get_lockinfo(lock);

	vos_flag_set(ctl_ipp_handle_pool.flg_id[lock_info.flag], lock_info.ptn);
}

void ctl_ipp_handle_wait_proc_end(UINT32 lock, BOOL bclr_flag, BOOL b_timeout_en)
{
	FLGPTN flg;
	CTL_IPP_HANDLE_LOCKINFO lock_info;
	UINT32 proc_end_mask;
	ER wai_rt;

	if (ctl_ipp_is_init) {
		lock_info = ctl_ipp_handle_get_lockinfo(lock);
		proc_end_mask = (lock_info.ptn << CTL_IPP_RPO_END_SHIFT);
		if (bclr_flag) {
			vos_flag_clr(ctl_ipp_handle_pool.pro_end_flg_id[lock_info.flag], proc_end_mask);
		}
		if (b_timeout_en) {
			wai_rt = vos_flag_wait_timeout(&flg, ctl_ipp_handle_pool.pro_end_flg_id[lock_info.flag], proc_end_mask, TWF_CLR, vos_util_msec_to_tick(CTL_IPP_HANDLE_LOCK_TIMEOUT_MS));
			if (wai_rt != E_OK) {
				if (wai_rt == E_TMOUT) {
					CTL_IPP_DBG_ERR("wait proc_end timeout(%d ms)\r\n", (int)(KDF_IPP_HANDLE_LOCK_TIMEOUT_MS));
				}
			}
		} else {
			vos_flag_wait(&flg, ctl_ipp_handle_pool.pro_end_flg_id[lock_info.flag], proc_end_mask, TWF_CLR);
		}
	} else {
		CTL_IPP_DBG_WRN("ctl ipp should init first\r\n");
	}
}

void ctl_ipp_handle_set_proc_end(UINT32 lock)
{
	CTL_IPP_HANDLE_LOCKINFO lock_info;
	UINT32 proc_end_mask;

	lock_info = ctl_ipp_handle_get_lockinfo(lock);
	proc_end_mask = lock_info.ptn << CTL_IPP_RPO_END_SHIFT;
	vos_flag_iset(ctl_ipp_handle_pool.pro_end_flg_id[lock_info.flag], proc_end_mask);
}

UINT32 ctl_ipp_handle_validate(CTL_IPP_HANDLE *p_hdl)
{
	UINT32 i;

	for (i = 0; i < ctl_ipp_handle_pool.handle_num; i++) {
		if ((UINT32)p_hdl == (UINT32)&ctl_ipp_handle_pool.handle[i]) {
			return CTL_IPP_E_OK;
		}
	}

	CTL_IPP_DBG_IND("ctl invalid handle(0x%.8x)\r\n", (unsigned int)p_hdl);
	return CTL_IPP_E_ID;
}

CTL_IPP_HANDLE *ctl_ipp_get_hdl_by_ispid(UINT32 id)
{
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	vos_list_for_each_safe(p_list, n, &ctl_ipp_handle_pool.used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		if (p_hdl->isp_id[CTL_IPP_ALGID_IQ] == id && p_hdl->sts != CTL_IPP_HANDLE_STS_FREE) {
			unl_cpu(loc_cpu_flg);
			return p_hdl;
		}
	}
	unl_cpu(loc_cpu_flg);

	return NULL;
}

UINT32 ctl_ipp_get_hdl_sts_by_ispid(UINT32 id)
{
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	vos_list_for_each_safe(p_list, n, &ctl_ipp_handle_pool.used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		if (p_hdl->isp_id[CTL_IPP_ALGID_IQ] == id) {
			unl_cpu(loc_cpu_flg);
			return p_hdl->sts;
		}
	}
	unl_cpu(loc_cpu_flg);

	return 0;
}

static CTL_IPP_HANDLE *ctl_ipp_handle_alloc(void)
{
	CTL_IPP_HANDLE *p_handle;

	if (vos_list_empty(&ctl_ipp_handle_pool.free_head)) {
		return NULL;
	}

	p_handle = vos_list_entry(ctl_ipp_handle_pool.free_head.next, CTL_IPP_HANDLE, list);
	ctl_ipp_handle_reset(p_handle);
	vos_list_del(&p_handle->list);
	vos_list_add_tail(&p_handle->list, &ctl_ipp_handle_pool.used_head);

	return p_handle;
}

static void ctl_ipp_handle_release(CTL_IPP_HANDLE *p_hdl)
{
	p_hdl->sts = CTL_IPP_HANDLE_STS_FREE;
	vos_list_del(&p_hdl->list);
	vos_list_add_tail(&p_hdl->list, &ctl_ipp_handle_pool.free_head);
}

static void ctl_ipp_set_direct_handle(CTL_IPP_HANDLE *p_hdl)
{
	if (p_hdl != NULL && p_ctl_ipp_direct_handle != NULL) {
		CTL_IPP_DBG_WRN("direct handle not null. 0x%.8x\r\n", (unsigned int)p_ctl_ipp_direct_handle);
	}

	p_ctl_ipp_direct_handle = p_hdl;
}

static CTL_IPP_HANDLE *ctl_ipp_get_direct_handle(void)
{
	return p_ctl_ipp_direct_handle;
}

static INT32 ctl_ipp_dir_wait_pause_end(void)
{
	return kdf_ipp_dir_wait_pause_end();
}

static void ctl_ipp_direct_set_sts(CTL_IPP_HANDLE *p_hdl, UINT32 sts)
{
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	dir_prv_sts = p_hdl->sts;
	p_hdl->sts = sts;
	unl_cpu(loc_cpu_flg);
}

INT32 ctl_ipp_direct_start(UINT32 hdl)
{
	CTL_IPP_HANDLE *p_hdl;
	INT32 rt = CTL_IPP_E_OK;

	dir_prv_sts = CTL_IPP_HANDLE_STS_MASK;
	p_hdl = (CTL_IPP_HANDLE *)hdl;

	if (ctl_ipp_get_direct_handle() != p_hdl) {
		CTL_IPP_DBG_WRN("direct only surpport one hdl 0x%x, 0x%x\r\n", (unsigned int)((UINT32) ctl_ipp_get_direct_handle()), (unsigned int)(hdl));
		return CTL_IPP_E_ID;
	}

	if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_STOP_S2) {
		if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3) {
			CTL_IPP_DBG_WRN("Direct start sts error 0x%.8x. Must do close->open after stop.\r\n", (unsigned int)(p_hdl->sts));
		} else {
			CTL_IPP_DBG_WRN("Direct start sts error 0x%.8x\r\n", (unsigned int)(p_hdl->sts));
		}
		return CTL_IPP_E_STATE;
	}

	CTL_IPP_DBG_IND("[CTL_IPL] process hdl 0x%x\r\n", (unsigned int)(hdl));

	if (ctl_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		CTL_IPP_DBG_WRN("direct_start fail(0x%.8x)\r\n", (unsigned int)hdl);
		return CTL_IPP_E_ID;
	}

	ctl_ipp_direct_set_sts(p_hdl, CTL_IPP_HANDLE_STS_DIR_START);

	return rt;

}

INT32 ctl_ipp_direct_stop(UINT32 hdl)
{
	CTL_IPP_HANDLE *p_hdl;
	INT32 rt = CTL_IPP_E_OK;
	p_hdl = (CTL_IPP_HANDLE *)hdl;

	if (ctl_ipp_get_direct_handle() != p_hdl) {
		CTL_IPP_DBG_WRN("direct only surpport one hdl 0x%x, 0x%x\r\n", (unsigned int)((UINT32) ctl_ipp_get_direct_handle()), (unsigned int)(hdl));
		return CTL_IPP_E_ID;
	}

	if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_READY && p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_START) {
		CTL_IPP_DBG_WRN("Direct stop sts error 0x%.8x\r\n", (unsigned int)(p_hdl->sts));
		return CTL_IPP_E_STATE;
	}

	CTL_IPP_DBG_IND("[DIR_STOP] process hdl 0x%x\r\n", (unsigned int)(hdl));

	if (ctl_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		CTL_IPP_DBG_WRN("direct_stop fail(0x%.8x)\r\n", (unsigned int)hdl);
		return CTL_IPP_E_ID;
	}

	ctl_ipp_direct_set_sts(p_hdl, CTL_IPP_HANDLE_STS_DIR_STOP_S1);

	/* wait ipp pause end */
	rt = ctl_ipp_dir_wait_pause_end();

	ctl_ipp_direct_set_sts(p_hdl, CTL_IPP_HANDLE_STS_DIR_STOP_S3);
	CTL_IPP_DBG_WRN("s3\r\n");

	/* flush isp event */
	ctl_ipp_isp_set_pause(TRUE, FALSE);
	ctl_ipp_isp_erase_queue(p_hdl->isp_id[CTL_IPP_ALGID_IQ]);
	ctl_ipp_isp_set_resume(FALSE);

	return rt;

}

static INT32 ctl_ipp_open_kdf(CTL_IPP_HANDLE *p_hdl)
{
	INT32 rt = CTL_IPP_E_OK;
	KDF_IPP_REG_CB_INFO cb_info;

	switch (p_hdl->flow) {
	case CTL_IPP_FLOW_RAW:
	case CTL_IPP_FLOW_VR360:
	case CTL_IPP_FLOW_CAPTURE_RAW:
		p_hdl->kdf_hdl = kdf_ipp_open(p_hdl->name, KDF_IPP_FLOW_RAW);
		break;

	case CTL_IPP_FLOW_IME_D2D:
		p_hdl->kdf_hdl = kdf_ipp_open(p_hdl->name, KDF_IPP_FLOW_IME_D2D);
		break;

	case CTL_IPP_FLOW_CCIR:
	case CTL_IPP_FLOW_CAPTURE_CCIR:
		p_hdl->kdf_hdl = kdf_ipp_open(p_hdl->name, KDF_IPP_FLOW_CCIR);
		break;

	case CTL_IPP_FLOW_IPE_D2D:
		p_hdl->kdf_hdl = kdf_ipp_open(p_hdl->name, KDF_IPP_FLOW_IPE_D2D);
		break;

	case CTL_IPP_FLOW_DIRECT_RAW:
		p_hdl->kdf_hdl = kdf_ipp_open(p_hdl->name, KDF_IPP_FLOW_DIRECT_RAW);
		break;

	case CTL_IPP_FLOW_DIRECT_CCIR:
		p_hdl->kdf_hdl = kdf_ipp_open(p_hdl->name, KDF_IPP_FLOW_DIRECT_CCIR);
		break;

	case CTL_IPP_FLOW_DCE_D2D:
		p_hdl->kdf_hdl = kdf_ipp_open(p_hdl->name, KDF_IPP_FLOW_DCE_D2D);
		break;

	default:
		p_hdl->kdf_hdl = 0;
		CTL_IPP_DBG_ERR("Unknown flow %d\r\n", (int)(p_hdl->flow));
	}

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_ERR("KDF Handle open failed\r\n");
		return CTL_IPP_E_SYS;
	}

	cb_info.cbevt = KDF_IPP_CBEVT_ENG_RHE_ISR;
	cb_info.fp = ctl_ipp_rhe_isr_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_ENG_IFE_ISR;
	cb_info.fp = ctl_ipp_ife_isr_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_ENG_DCE_ISR;
	cb_info.fp = ctl_ipp_dce_isr_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_ENG_IPE_ISR;
	cb_info.fp = ctl_ipp_ipe_isr_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_ENG_IME_ISR;
	cb_info.fp = ctl_ipp_ime_isr_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_ENG_IFE2_ISR;
	cb_info.fp = ctl_ipp_ife2_isr_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_PROCEND;
	cb_info.fp = ctl_ipp_proc_end_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_DROP;
	cb_info.fp = ctl_ipp_drop_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_PRESET;
	cb_info.fp = ctl_ipp_preset_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_PROC_TIME;
	cb_info.fp = ctl_ipp_proc_time_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_DATASTAMP;
	cb_info.fp = ctl_ipp_datastamp_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_PRIMASK;
	cb_info.fp = ctl_ipp_primask_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_ISP;
	cb_info.fp = ctl_ipp_isp_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_PROCSTART;
	cb_info.fp = ctl_ipp_proc_start_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_FRAMESTART;
	cb_info.fp = ctl_ipp_frm_start_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	cb_info.cbevt = KDF_IPP_CBEVT_LOWDELAY;
	cb_info.fp = ctl_ipp_low_delay_cb;
	rt |= kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_REG_CB, (void *)&cb_info);

	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_ERR("KDF callback register failed\r\n");
		return CTL_IPP_E_SYS;
	}

	return CTL_IPP_E_OK;
}

static void ctl_ipp_close_kdf(CTL_IPP_HANDLE *p_hdl)
{
	if (p_hdl->kdf_hdl != 0) {
		if (kdf_ipp_close(p_hdl->kdf_hdl) == CTL_IPP_E_OK) {
			p_hdl->kdf_hdl = 0;
		} else {
			CTL_IPP_DBG_ERR("KDF Handle 0x%.8x close failed\r\n", (unsigned int)p_hdl->kdf_hdl);
		}
	}
}

static UINT32 ctl_ipp_open_ippevt(CTL_IPP_HANDLE *p_hdl)
{
	p_hdl->ipp_evt_hdl = ipp_event_open(p_hdl->name);
	if (p_hdl->ipp_evt_hdl == 0) {
		CTL_IPP_DBG_ERR("IPP EVENT HANDLLE open failed\r\n");
		return CTL_IPP_E_SYS;
	}

	return CTL_IPP_E_OK;
}

static void ctl_ipp_close_ippevt(CTL_IPP_HANDLE *p_hdl)
{
	if (p_hdl->ipp_evt_hdl != 0) {
		if (ipp_event_close(p_hdl->ipp_evt_hdl) == IPP_EVENT_OK) {
			p_hdl->ipp_evt_hdl = 0;
		} else {
			CTL_IPP_DBG_ERR("IPP EVENT HANDLE 0x%.8x close failed\r\n", (unsigned int)p_hdl->ipp_evt_hdl);
		}
	}
}


INT32 ctl_ipp_process(UINT32 hdl, UINT32 data_addr, UINT32 buf_id, UINT64 snd_time, UINT32 proc_cmd, UINT32 dir_evt)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_HANDLE *p_hdl;
	CTL_IPP_INFO_LIST_ITEM *p_dummy_info;
	UINT64 rev_time;
	INT32 rt;

	rev_time = ctl_ipp_util_get_syst_counter();

	p_hdl = (CTL_IPP_HANDLE *)hdl;

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[CTL_IPL] process hdl 0x%x, header 0x%.8x, buf_id %d, dir_evt 0x%.8x\r\n", (unsigned int)hdl, (unsigned int)data_addr, (int)buf_id, (unsigned int)dir_evt);

	if (ctl_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	/* ctrl process based on flow */
	ctl_ipp_handle_lock(p_hdl->lock);

	p_hdl->snd_evt_time = snd_time;
	p_hdl->rev_evt_time = rev_time;

	if (p_hdl->sts == CTL_IPP_HANDLE_STS_FREE) {
		ctl_ipp_handle_unlock(p_hdl->lock);
		CTL_IPP_DBG_WRN("illegal handle(0x%.8x), status(0x%.8x)\r\n", (unsigned int)hdl, (unsigned int)p_hdl->sts);
		return CTL_IPP_E_STATE;
	}

    /* iq reset event for non-direct mode */
    if ((p_hdl->kdf_snd_evt_cnt == 0) && !(CTL_IPP_IS_DIRECT_FLOW(p_hdl->flow))) {
		p_dummy_info = ctl_ipp_info_alloc((UINT32)p_hdl);
		if (p_dummy_info) {
			memcpy((void *)&p_dummy_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));
		}
		ctl_ipp_isp_event_cb(p_hdl->isp_id[CTL_IPP_ALGID_IQ], ISP_EVENT_PARAM_RST, p_dummy_info, (void *)p_hdl->name);
		if (p_dummy_info) {
			ctl_ipp_info_release(p_dummy_info);
		}
    }

	rt = CTL_IPP_E_OK;
	if (proc_cmd == CTL_IPP_MSG_PROCESS_PATTERN_PASTE) {
		rt = ctl_ipp_process_flow_pattern_paste(p_hdl, data_addr, buf_id);
	} else {
		switch (p_hdl->flow) {
		case CTL_IPP_FLOW_RAW:
		case CTL_IPP_FLOW_CAPTURE_RAW:
			rt = ctl_ipp_process_raw(p_hdl, data_addr, buf_id);
			break;

		case CTL_IPP_FLOW_IME_D2D:
			rt = ctl_ipp_process_ime_d2d(p_hdl, data_addr, buf_id);
			break;

		case CTL_IPP_FLOW_CCIR:
		case CTL_IPP_FLOW_CAPTURE_CCIR:
			rt = ctl_ipp_process_ccir(p_hdl, data_addr, buf_id);
			break;

		case CTL_IPP_FLOW_IPE_D2D:
			rt = ctl_ipp_process_ipe_d2d(p_hdl, data_addr, buf_id);
			break;

		case CTL_IPP_FLOW_VR360:
			rt = ctl_ipp_process_vr360(p_hdl, data_addr, buf_id);
			break;

		case CTL_IPP_FLOW_DCE_D2D:
			rt = ctl_ipp_process_dce_d2d(p_hdl, data_addr, buf_id);
			break;

		case CTL_IPP_FLOW_DIRECT_RAW:
			switch (dir_evt) {
			case CTL_IPP_DIRECT_PROCESS:
				if ((p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3 && dir_prv_sts == CTL_IPP_HANDLE_STS_DIR_STOP_S1) ||
					(p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_SLEEP && dir_prv_sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3)){
					rt = CTL_IPP_E_STATE;
				} else if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_READY && p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_STOP_S1) {
					CTL_IPP_DBG_WRN("[Pro] illegal status 0X%.8x\r\n", (unsigned int)p_hdl->sts);
					rt = CTL_IPP_E_STATE;
				} else {
					rt = ctl_ipp_process_direct(p_hdl, data_addr, 0);
				}
				break;

			case CTL_IPP_DIRECT_STOP:
				if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_STOP_S2 && p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_STOP_S3 && p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_SLEEP) {
					CTL_IPP_DBG_WRN("[Stop] illegal status 0X%.8x\r\n", (unsigned int)p_hdl->sts);
					rt = CTL_IPP_E_STATE;
				}
				break;

			default:
				;
			}

			if (rt != E_OK || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S2 || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3) {
				if (rt != E_OK) {
					CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "dir drop (%d)\r\n", rt);
				}
				loc_cpu(loc_cpu_flg);
				p_hdl->drop_cnt += 1;
				unl_cpu(loc_cpu_flg);
			}

			// set dir_tsk_sts to idle, to indicate the next bp3 event can be sent
			vk_spin_lock_irqsave(&p_hdl->dir_tsk_lock, loc_cpu_flg);
			p_hdl->dir_tsk_sts = CTL_IPP_HANDLE_STS_DIR_TSK_IDLE;
			vk_spin_unlock_irqrestore(&p_hdl->dir_tsk_lock, loc_cpu_flg);

			break;

		default:
			rt = CTL_IPP_E_PAR;
			CTL_IPP_DBG_WRN("Unknown Flow %d\r\n", (int)(p_hdl->flow));
			break;
		}
	}

	if (rt == CTL_IPP_E_OK && p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) { // direct mode already increase kdf_snd_evt_cnt in ctl_ipp_process_direct, skip here
		p_hdl->kdf_snd_evt_cnt += 1;
	}
	ctl_ipp_handle_unlock(p_hdl->lock);

	return rt;
}

INT32 ctl_ipp_drop(UINT32 hdl, UINT32 data_addr, UINT32 buf_id, IPP_EVENT_FP drop_fp, INT32 err_msg)
{
	CTL_IPP_DBG_TS_LOG ts_log;
	CTL_IPP_EVT drop_evt;
	CTL_IPP_HANDLE *p_hdl;
	VDO_FRAME *p_frm;

	p_hdl = (CTL_IPP_HANDLE *)hdl;

	CTL_IPP_DBG_TRC("[CTL_IPL] drop hdl 0x%x, header 0x%.8x, buf_id %d\r\n", (unsigned int)hdl, (unsigned int)data_addr, (int)buf_id);

	if (ctl_ipp_handle_validate(p_hdl) == CTL_IPP_E_OK) {
		unsigned long loc_cpu_flg;

		loc_cpu(loc_cpu_flg);
		p_hdl->drop_cnt += 1;
		unl_cpu(loc_cpu_flg);
	}

	memset((void *)&ts_log, 0, sizeof(CTL_IPP_DBG_TS_LOG));
	ts_log.handle = hdl;
	ts_log.cnt = p_hdl->proc_end_cnt + p_hdl->drop_cnt;
	ts_log.err_msg = err_msg;
	ts_log.ts[CTL_IPP_DBG_TS_SND] = p_hdl->snd_evt_time;
	ts_log.ts[CTL_IPP_DBG_TS_RCV] = p_hdl->rev_evt_time;
	ctl_ipp_dbg_ts_set(ts_log);

	if (drop_fp == NULL) {
		return CTL_IPP_E_PAR;
	}

	drop_evt.buf_id = buf_id;
	drop_evt.data_addr = data_addr;
	drop_evt.rev = 0;
	drop_evt.err_msg = err_msg;
	if (p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
		ctl_ipp_inbuf_fp_wrapper((UINT32)p_hdl, (UINT32)drop_fp, CTL_IPP_CBEVT_IN_BUF_DROP, &drop_evt);
	} else {
		p_frm = (VDO_FRAME *)data_addr;
		if (p_frm->reserved[3] != 0) {
			ctl_ipp_inbuf_fp_wrapper((UINT32)p_hdl, (UINT32)drop_fp, CTL_IPP_CBEVT_IN_BUF_DIR_DROP, &drop_evt);
		}
	}

	return CTL_IPP_E_OK;
}


#if 0
#endif

ER ctl_ipp_open_tsk(void)
{
	if (ctl_ipp_task_opened) {
		CTL_IPP_DBG_ERR("re-open\r\n");
		return E_SYS;
	}

	if (g_ctl_ipp_flg_id == 0) {
		CTL_IPP_DBG_ERR("g_ctl_ipp_flg_id = %d\r\n", (int)(g_ctl_ipp_flg_id));
		return E_SYS;
	}

	/* open ctrl_buf task */
	if (ctl_ipp_buf_open_tsk() != E_OK) {
		CTL_IPP_DBG_ERR("Open ctrl_buf_tsk fail\r\n");
		return FALSE;
	}

	/* open ctrl_ise task */
	if (ctl_ipp_ise_open_tsk() != E_OK) {
		CTL_IPP_DBG_ERR("Open ctrl_ise_tsk fail\r\n");
		return FALSE;
	}

	/* open ctrl_isp task */
	if (ctl_ipp_isp_open_tsk() != E_OK) {
		CTL_IPP_DBG_ERR("Open ctrl_isp_tsk fail\r\n");
		return FALSE;
	}

	/* reset configs */
	ctl_ipp_task_pause_count = 0;
	vos_flag_clr(g_ctl_ipp_flg_id, FLGPTN_BIT_ALL);
	vos_flag_set(g_ctl_ipp_flg_id, CTL_IPP_PROC_INIT);
	ctl_ipp_msg_reset_queue();

	/* start ctrl task */
	THREAD_CREATE(g_ctl_ipp_tsk_id, ctl_ipp_tsk, NULL, "ctl_ipp_tsk");
	if (g_ctl_ipp_tsk_id == 0) {
		CTL_IPP_DBG_ERR("Open ctl ipp tsk fail\r\n");
		return E_SYS;
	}
	THREAD_SET_PRIORITY(g_ctl_ipp_tsk_id, CTL_IPP_TSK_PRI);
	THREAD_RESUME(g_ctl_ipp_tsk_id);

	ctl_ipp_task_opened = TRUE;

	/* set ctrl task to processing */
	ctl_ipp_set_resume(TRUE);

	return E_OK;
}

ER ctl_ipp_close_tsk(void)
{
	FLGPTN wait_flg = 0;

	/* close ctrl buf task */
	if (ctl_ipp_buf_close_tsk() != E_OK) {
		CTL_IPP_DBG_ERR("Close ctrl_buf_tsk fail\r\n");
		return E_SYS;
	}

	/* close ctrl ise task */
	if (ctl_ipp_ise_close_tsk() != E_OK) {
		CTL_IPP_DBG_ERR("Close ctrl_ise_tsk fail\r\n");
		return E_SYS;
	}

	/* close ctrl isp task */
	if (ctl_ipp_isp_close_tsk() != E_OK) {
		CTL_IPP_DBG_ERR("Close ctrl_isp_tsk fail\r\n");
		return E_SYS;
	}

	if (!ctl_ipp_task_opened) {
		CTL_IPP_DBG_ERR("re-close\r\n");
		return E_SYS;
	}

	while (ctl_ipp_task_pause_count) {
		ctl_ipp_set_pause(ENABLE, ENABLE);
	}

	vos_flag_set(g_ctl_ipp_flg_id, CTL_IPP_TASK_RES);
	if (vos_flag_wait(&wait_flg, g_ctl_ipp_flg_id, CTL_IPP_TASK_EXIT_END, (TWF_ORW | TWF_CLR)) != E_OK) {
		/* if wait not ok, force destroy task */
		THREAD_DESTROY(g_ctl_ipp_tsk_id);
	}
	ctl_ipp_task_opened = FALSE;
	return E_OK;
}

THREAD_DECLARE(ctl_ipp_tsk, p1)
{
	ER err;
	UINT32 cmd, param[3];
	FLGPTN wait_flg = 0;
	IPP_EVENT_FP drop_fp;
	UINT64 time;

	THREAD_ENTRY();
	goto CTL_IPP_PAUSE;

CTL_IPP_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(g_ctl_ipp_flg_id, (CTL_IPP_TASK_IDLE | CTL_IPP_TASK_RESUME_END));
		PROFILE_TASK_IDLE();
		drop_fp = NULL;
		err = ctl_ipp_msg_rcv(&cmd, &param[0], &param[1], &param[2], &drop_fp, &time);
		PROFILE_TASK_BUSY();
		vos_flag_clr(g_ctl_ipp_flg_id, CTL_IPP_TASK_IDLE);

		if (err != E_OK) {
			CTL_IPP_DBG_IND("[CTL_IPL]Rcv Msg error (%d)\r\n", (int)(err));
		} else {
			/* process event */
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[CTL_IPL]P Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));

			vos_flag_set(g_ctl_ipp_flg_id, CTL_IPP_TASK_CHK);
			vos_flag_wait(&wait_flg, g_ctl_ipp_flg_id, (CTL_IPP_TASK_PAUSE | CTL_IPP_TASK_CHK), (TWF_ORW | TWF_CLR));

			if (cmd == CTL_IPP_MSG_IGNORE) {
				/* do nothing */
			} else if (cmd == CTL_IPP_MSG_PROCESS || cmd == CTL_IPP_MSG_PROCESS_PATTERN_PASTE) {
				INT32 rt;

				rt = ctl_ipp_process(param[0], param[1], param[2], time, cmd, 0);
				if (rt != CTL_IPP_E_OK) {
					/* process failed, inform drop */
					ctl_ipp_drop(param[0], param[1], param[2], drop_fp, rt);
				}
			} else if (cmd == CTL_IPP_MSG_PROCESS_DIRECT) {
				ctl_ipp_process(param[0], param[1], 0, time, cmd, param[2]); // direct mode use p_vdoin_info->count, no need buf_id
				// direct task mode drop frame in ctl_ipp_process when error. dont drop here
			} else if (cmd == CTL_IPP_MSG_DROP) {
				ctl_ipp_drop(param[0], param[1], param[2], drop_fp, CTL_IPP_E_OK);
			} else {
				CTL_IPP_DBG_WRN("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));
			}

			/* check pause after cmd is processed */
			if (wait_flg & CTL_IPP_TASK_PAUSE) {
				break;
			}
		}
	}

CTL_IPP_PAUSE:
	vos_flag_clr(g_ctl_ipp_flg_id, (CTL_IPP_TASK_RESUME_END));
	vos_flag_set(g_ctl_ipp_flg_id, CTL_IPP_TASK_PAUSE_END);
	vos_flag_wait(&wait_flg, g_ctl_ipp_flg_id, (CTL_IPP_TASK_RES | CTL_IPP_TASK_RESUME | CTL_IPP_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flg & CTL_IPP_TASK_RES) {
		goto CTL_IPP_DESTROY;
	}

	if (wait_flg & CTL_IPP_TASK_RESUME) {
		vos_flag_clr(g_ctl_ipp_flg_id, CTL_IPP_TASK_PAUSE_END);
		if (wait_flg & CTL_IPP_TASK_FLUSH) {
			ctl_ipp_msg_flush();
		}
		goto CTL_IPP_PROC;
	}

CTL_IPP_DESTROY:
	vos_flag_set(g_ctl_ipp_flg_id, CTL_IPP_TASK_EXIT_END);
	THREAD_RETURN(0);
}

ER ctl_ipp_set_resume(BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!ctl_ipp_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_task_pause_count == 0) {
		ctl_ipp_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
		if (b_flush_evt == ENABLE) {
			vos_flag_set(g_ctl_ipp_flg_id, (CTL_IPP_TASK_RESUME | CTL_IPP_TASK_FLUSH));
		} else {
			vos_flag_set(g_ctl_ipp_flg_id, CTL_IPP_TASK_RESUME);
		}

		vos_flag_wait(&wait_flg, g_ctl_ipp_flg_id, CTL_IPP_TASK_RESUME_END, TWF_ORW);
	} else {
		ctl_ipp_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
	}

	return E_OK;
}

ER ctl_ipp_set_pause(BOOL b_wait_end, BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!ctl_ipp_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_task_pause_count != 0) {
		ctl_ipp_task_pause_count -= 1;

		if (ctl_ipp_task_pause_count == 0) {
			unl_cpu(loc_cpu_flg);
			wait_flg = CTL_IPP_TASK_PAUSE;

			vos_flag_set(g_ctl_ipp_flg_id, wait_flg);
			/* send dummy msg, kdf_tsk must rcv msg to pause */
			ctl_ipp_msg_snd(CTL_IPP_MSG_IGNORE, 0, 0, 0, NULL);

			if (b_wait_end == ENABLE) {
				if (vos_flag_wait(&wait_flg, g_ctl_ipp_flg_id, CTL_IPP_TASK_PAUSE_END, TWF_ORW) == E_OK) {
					if (b_flush_evt == TRUE) {
						ctl_ipp_msg_flush();
					}
				}
			}
		} else {
			unl_cpu(loc_cpu_flg);
		}
	} else {
		CTL_IPP_DBG_WRN("task pause cnt already 0\r\n");
		unl_cpu(loc_cpu_flg);
	}

	return E_OK;
}

ER ctl_ipp_wait_pause_end(void)
{
	FLGPTN wait_flg;

	if (!ctl_ipp_task_opened) {
		return E_SYS;
	}

	vos_flag_wait(&wait_flg, g_ctl_ipp_flg_id, CTL_IPP_TASK_PAUSE_END, TWF_ORW);
	return E_OK;
}

#if 0
#endif

static void ctl_ipp_baseinfo_reset(CTL_IPP_BASEINFO *p_info)
{
	UINT32 i;

	memset((void *)p_info, 0, sizeof(CTL_IPP_BASEINFO));

	/* scale method default auto */
	p_info->ime_ctrl.out_scl_method_sel.scl_th = 0x00010001;
	p_info->ime_ctrl.out_scl_method_sel.method_l = CTL_IPP_SCL_AUTO;
	p_info->ime_ctrl.out_scl_method_sel.method_h = CTL_IPP_SCL_AUTO;

	/* reset path enable to initital state */
	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		p_info->ime_ctrl.out_img[i].enable = DISABLE;
	}
}

void ctl_ipp_handle_reset(CTL_IPP_HANDLE *p_hdl)
{
	UINT32 i;

	memset((void *)&p_hdl->name[0], '\0', sizeof(CHAR) * CTL_IPP_HANDLE_NAME_MAX);
	p_hdl->sts = CTL_IPP_HANDLE_STS_FREE;
	p_hdl->dir_tsk_sts = CTL_IPP_HANDLE_STS_DIR_TSK_IDLE;
	p_hdl->flow = CTL_IPP_FLOW_UNKNOWN;
	p_hdl->evt_outbuf_fp = NULL;
	p_hdl->evt_inbuf_fp = NULL;
	p_hdl->kdf_hdl = 0;
	p_hdl->ipp_evt_hdl = 0;
	p_hdl->ctl_snd_evt_cnt = 0;
	p_hdl->proc_end_cnt = 0;
	p_hdl->drop_cnt = 0;
	p_hdl->kdf_snd_evt_cnt = 0;
	p_hdl->buf_push_evt_cnt = 0;
	p_hdl->ctl_frm_str_cnt = 0;
	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		p_hdl->buf_new_fail_cnt[i] = 0;
		p_hdl->buf_tag_fail_cnt[i] = 0;
		p_hdl->buf_io_started[i] = 0;
	}
	p_hdl->in_buf_drop_cnt = 0;
	p_hdl->in_buf_re_cnt = 0;
	p_hdl->in_buf_re_cb_cnt = 0;
	p_hdl->in_pro_skip_cnt = 0;
	p_hdl->last_3dnr_disable_frm = 0;
	p_hdl->last_3dnr_ref_shift_frm = 0;
	p_hdl->_3dnr_ref_shift_cnt = 0;
	p_hdl->max_3dnr_ref_shift_diff = 0;
	p_hdl->last_pattern_paste_frm = 0;
	p_hdl->p_last_rdy_ctrl_info = NULL;
	p_hdl->p_last_cfg_ctrl_info= NULL;
	p_hdl->bufcfg.start_addr = 0;
	p_hdl->bufcfg.size = 0;
	p_hdl->dir_proc_bit = 0;
	p_hdl->is_first_handle = 0;
	ctl_ipp_baseinfo_reset(&p_hdl->ctrl_info);
	ctl_ipp_baseinfo_reset(&p_hdl->rtc_info);
	ctl_ipp_buf_pri_init((UINT32)&p_hdl->private_buf);

	for (i = 0; i < CTL_IPP_ALGID_MAX; i++) {
		p_hdl->isp_id[i] = 0xFFFFFFFF;
	}

	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		p_hdl->buf_push_order[i] = 0;
		p_hdl->buf_push_pid_sequence[i] = i;
	}
}

UINT32 ctl_ipp_handle_pool_init(UINT32 num, UINT32 buf_addr, UINT32 is_query)
{
	UINT32 i, lock_val, buf_size;
	void *p_buf;

	/* buffer alloc
		1. handle buffer
		2. flag id buffer
	*/
	ctl_ipp_handle_pool.handle_num = num;
	ctl_ipp_handle_pool.flg_num = ALIGN_CEIL((num + CTL_IPP_HANDLE_FREE_FLAG_START), CTL_IPP_UTIL_FLAG_PAGE_SIZE) / CTL_IPP_UTIL_FLAG_PAGE_SIZE;
	ctl_ipp_handle_pool.pro_end_flg_num = ALIGN_CEIL((num * CTL_IPP_RPO_END_USE_BIT), CTL_IPP_UTIL_FLAG_PAGE_SIZE) / CTL_IPP_UTIL_FLAG_PAGE_SIZE;

	buf_size = (sizeof(CTL_IPP_HANDLE) * num) + (ctl_ipp_handle_pool.flg_num * sizeof(ID)) + (ctl_ipp_handle_pool.pro_end_flg_num * sizeof(ID));
	if (is_query) {
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_HANDLE", CTL_IPP_DBG_CTX_BUF_QUERY, buf_size, buf_addr, num);
		return buf_size;
	}

	p_buf = (void *)buf_addr;
	if (p_buf == NULL) {
		ctl_ipp_handle_pool.handle_num = 0;
		ctl_ipp_handle_pool.flg_num = 0;
		ctl_ipp_handle_pool.pro_end_flg_num = 0;
		CTL_IPP_DBG_ERR("alloc ctl_handle pool memory failed, num %d, want_size 0x%.8x\r\n", (int)num, (unsigned int)buf_size);
		return CTL_IPP_E_NOMEM;
	}
	ctl_ipp_handle_pool.flg_id = p_buf;
	ctl_ipp_handle_pool.pro_end_flg_id = (ID *)((UINT32)p_buf + ctl_ipp_handle_pool.flg_num * sizeof(ID));
	ctl_ipp_handle_pool.handle = (CTL_IPP_HANDLE *)((UINT32)p_buf + ctl_ipp_handle_pool.flg_num * sizeof(ID) + ctl_ipp_handle_pool.pro_end_flg_num * sizeof(ID));
	CTL_IPP_DBG_TRC("alloc ctl_handle pool memory, num %d, flg_num %d, p_flg_num %d, size 0x%.8x, addr 0x%.8x, \r\n",
		(int)ctl_ipp_handle_pool.handle_num, (int)ctl_ipp_handle_pool.flg_num, (int)ctl_ipp_handle_pool.pro_end_flg_num, (unsigned int)buf_size, (unsigned int)p_buf);

	/* reset & create free handle pool list */
	VOS_INIT_LIST_HEAD(&ctl_ipp_handle_pool.free_head);
	VOS_INIT_LIST_HEAD(&ctl_ipp_handle_pool.used_head);

	lock_val = CTL_IPP_HANDLE_FREE_FLAG_START;
	for (i = 0; i < ctl_ipp_handle_pool.handle_num; i++) {
		ctl_ipp_handle_reset(&ctl_ipp_handle_pool.handle[i]);
		ctl_ipp_handle_pool.handle[i].lock = lock_val;
		ctl_ipp_handle_pool.handle[i].pro_end_lock = i * CTL_IPP_RPO_END_USE_BIT;
		vk_spin_lock_init(&ctl_ipp_handle_pool.handle[i].spinlock);
		vk_spin_lock_init(&ctl_ipp_handle_pool.handle[i].dir_tsk_lock);
		vos_list_add_tail(&ctl_ipp_handle_pool.handle[i].list, &ctl_ipp_handle_pool.free_head);
		lock_val += 1;
	}

	/* reset handle flag */
	for (i = 0; i < ctl_ipp_handle_pool.flg_num; i++) {
		OS_CONFIG_FLAG((ctl_ipp_handle_pool.flg_id[i]));
		if (i == 0) {
			vos_flag_set(ctl_ipp_handle_pool.flg_id[i], (FLGPTN_BIT_ALL & ~(CTL_IPP_HANDLE_POOL_LOCK)));
		} else {
			vos_flag_set(ctl_ipp_handle_pool.flg_id[i], FLGPTN_BIT_ALL);
		}
	}

	/* reset handle process end flag */
	for (i = 0; i < ctl_ipp_handle_pool.pro_end_flg_num; i++) {
		OS_CONFIG_FLAG(ctl_ipp_handle_pool.pro_end_flg_id[i]);
		vos_flag_set(ctl_ipp_handle_pool.pro_end_flg_id[i], FLGPTN_BIT_ALL);
	}

	/* reset direct handle */
	p_ctl_ipp_direct_handle = NULL;

	/* unlock pool, set debug info */
	ctl_ipp_handle_pool_unlock();
	ctl_ipp_dbg_hdl_head_set(&ctl_ipp_handle_pool.used_head);

	ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_HANDLE", CTL_IPP_DBG_CTX_BUF_ALLOC, buf_size, buf_addr, num);

	return buf_size;
}

void ctl_ipp_handle_pool_free(void)
{
	UINT32 i;

	if (ctl_ipp_handle_pool.flg_id != NULL) {
		CTL_IPP_DBG_TRC("free ctl_handle memory, addr 0x%.8x\r\n", (unsigned int)ctl_ipp_handle_pool.flg_id);
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_HANDLE", CTL_IPP_DBG_CTX_BUF_FREE, 0, (UINT32)ctl_ipp_handle_pool.flg_id, 0);

		for (i = 0; i < ctl_ipp_handle_pool.flg_num; i++) {
			rel_flg(ctl_ipp_handle_pool.flg_id[i]);
		}

		for (i = 0; i < ctl_ipp_handle_pool.pro_end_flg_num; i++) {
			rel_flg(ctl_ipp_handle_pool.pro_end_flg_id[i]);
		}

		ctl_ipp_handle_pool.flg_num = 0;
		ctl_ipp_handle_pool.handle_num = 0;
		ctl_ipp_handle_pool.flg_id = NULL;
		ctl_ipp_handle_pool.handle = NULL;
		ctl_ipp_handle_pool.pro_end_flg_num = 0;
		ctl_ipp_handle_pool.pro_end_flg_id = NULL;
	}
}

UINT32 ctl_ipp_query(CTL_IPP_CTX_BUF_CFG ctx_buf_cfg)
{
	UINT32 buf_size;

	CTL_IPP_DBG_IND("n %u\r\n", ctx_buf_cfg.n);

	buf_size = 0;

	if (ctx_buf_cfg.n == 0) {
		return 0;
	}

	/* ctl handle pool init */
	buf_size += ctl_ipp_handle_pool_init(ctx_buf_cfg.n, 0, TRUE);

	/* ctl info pool init */
	buf_size += ctl_ipp_info_pool_init(ctx_buf_cfg.n, 0, TRUE);

	/* ctl ipp event pool init */
	buf_size += ipp_event_init(ctx_buf_cfg.n, 0, TRUE);

	/* ctl ipp ise pool init */
	buf_size += ctl_ipp_ise_pool_init(ctx_buf_cfg.n, 0, TRUE);

	/* ctl ipp isp init */
	buf_size += ctl_ipp_isp_init(ctx_buf_cfg.n, 0, TRUE);

	/* kdf ipp init */
	buf_size += kdf_ipp_init(ctx_buf_cfg.n, 0, TRUE);

	return buf_size;
}

INT32 ctl_ipp_init(CTL_IPP_CTX_BUF_CFG ctx_buf_cfg, UINT32 buf_addr, UINT32 buf_size)
{
	UINT32 num, buf_addr_start, buf_addr_end;

	CTL_IPP_DBG_IND("n %u addr 0x%x size 0x%x\r\n", ctx_buf_cfg.n, buf_addr, buf_size);

	if (ctl_ipp_is_init) {
		CTL_IPP_DBG_ERR("IPP IS ALREADY INIT\r\n");
		return CTL_IPP_E_STATE;
	}

	num = ctx_buf_cfg.n;
	buf_addr_start = buf_addr;
	buf_addr_end = buf_addr + buf_size;

	if (num == 0) {
		CTL_IPP_DBG_ERR("ipp init number = 0\r\n");
		return CTL_IPP_E_PAR;
	}

	if (buf_addr == 0) {
		CTL_IPP_DBG_ERR("ipp init buf_addr = 0\r\n");
		return CTL_IPP_E_NOMEM;
	}

	/* Install id and Open task */
	ipp_event_install_id();
	ctl_ipp_install_id();
	if (ctl_ipp_open_tsk() != E_OK) {
		ctl_ipp_uninit();
		return CTL_IPP_E_SYS;
	}

	/* ctl handle pool init */
	buf_addr += ctl_ipp_handle_pool_init(num, buf_addr, FALSE);

	/* ctl info pool init */
	if (buf_addr <= buf_addr_end) {
		buf_addr += ctl_ipp_info_pool_init(num, buf_addr, FALSE);
	}

	/* ctl ipp event pool init */
	if (buf_addr <= buf_addr_end) {
		buf_addr += ipp_event_init(num, buf_addr, FALSE);
	}

	/* ctl ipp ise pool init */
	if (buf_addr <= buf_addr_end) {
		buf_addr += ctl_ipp_ise_pool_init(num, buf_addr, FALSE);
	}

	/* ctl ipp isp init */
	if (buf_addr <= buf_addr_end) {
		buf_addr += ctl_ipp_isp_init(num, buf_addr, FALSE);
	}

	/* kdf ipp init */
	if (buf_addr <= buf_addr_end) {
		buf_addr += kdf_ipp_init(num, buf_addr, FALSE);
	}

	/* release buffer if init failed */
	if (buf_addr > buf_addr_end) {
		CTL_IPP_DBG_ERR("kflow ipp init failed, buf 0x%.8x out of range 0x%.8x ~ 0x%.8x\r\n",
			(unsigned int)buf_addr, (unsigned int)buf_addr_start, (unsigned int)buf_addr_end);

		ctl_ipp_uninit();
		return CTL_IPP_E_NOMEM;
	}

	/* init debug base info */
	ctl_ipp_dbg_set_baseinfo(0,TRUE);
	ctl_ipp_dbg_dtsi_init();

	ctl_ipp_is_init = TRUE;

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_uninit(void)
{
	CTL_IPP_DBG_IND("\r\n");

	if (ctl_ipp_is_init) {
		/* check if all handle released */
		if (vos_list_empty(&ctl_ipp_handle_pool.used_head) == FALSE) {
			CTL_IPP_DBG_ERR("handle still in used, close all handle before uninit\r\n");
			return CTL_IPP_E_STATE;
		}

		/* release init_buffer */
		ctl_ipp_handle_pool_free();
		ctl_ipp_isp_uninit();
		ctl_ipp_info_pool_free();
		ipp_event_uninit();
		ctl_ipp_ise_pool_free();
		kdf_ipp_uninit();

		/* close task and uninstall id */
		ctl_ipp_close_tsk();
		ipp_event_uninstall_id();
		ctl_ipp_uninstall_id();

		ctl_ipp_is_init = 0;
	} else {
		CTL_IPP_DBG_ERR("IPP IS NOT INIT\r\n");
	}

	return CTL_IPP_E_OK;
}

UINT32 ctl_ipp_open(CHAR *name, CTL_IPP_FLOW_TYPE flow)
{
	static UINT32 is_first_handle = TRUE;
	unsigned long loc_cpu_flg;
	UINT32 rt;
	UINT32 first_hdl;
	UINT32 handle = 0;
	CTL_IPP_HANDLE *p_hdl;

	CTL_IPP_DBG_IND("%s flow%u\r\n", name, flow);

	if (ctl_ipp_is_init == 0) {
		CTL_IPP_DBG_ERR("ipp is not init\r\n");
		return 0;
	}

	if (flow >= CTL_IPP_FLOW_MAX) {
		CTL_IPP_DBG_ERR("open with unknown flow %d\r\n", (int)(flow));
		return 0;
	}

	if (ctl_ipp_get_direct_handle() != NULL) {
		CTL_IPP_DBG_ERR("can not re-open when direct mode is open\r\n");
		return 0;
	}

	if (flow == CTL_IPP_FLOW_DIRECT_RAW || flow == CTL_IPP_FLOW_DIRECT_CCIR) {
		if (vos_list_empty(&ctl_ipp_handle_pool.used_head) == FALSE) {
			CTL_IPP_DBG_ERR("direct mode only surpport one hdl\r\n");
			return 0;
		}
	}

	ctl_ipp_handle_pool_lock();

	/* set dma priority if first handle */
	first_hdl = vos_list_empty(&ctl_ipp_handle_pool.used_head);
	p_hdl = ctl_ipp_handle_alloc();
	if (p_hdl != NULL) {
		if (first_hdl) {
			#if defined(_BSP_NA51000_)
			#else
			if (flow == CTL_IPP_FLOW_DIRECT_CCIR || flow == CTL_IPP_FLOW_DIRECT_RAW) {
				arb_set_priority(TRUE);
			} else {
				arb_set_priority(FALSE);
			}
			#endif
		}

		ctl_ipp_handle_lock(p_hdl->lock);

		/* reset handle then set new status */
		ctl_ipp_handle_reset(p_hdl);
		if (name != NULL) {
			strncpy(p_hdl->name, name, (CTL_IPP_HANDLE_NAME_MAX - 1));
		}

		loc_cpu(loc_cpu_flg);
		if (flow == CTL_IPP_FLOW_DIRECT_CCIR || flow == CTL_IPP_FLOW_DIRECT_RAW) {
			p_hdl->sts = CTL_IPP_HANDLE_STS_DIR_STOP_S2;
		} else {
			p_hdl->sts = CTL_IPP_HANDLE_STS_READY;
		}
		unl_cpu(loc_cpu_flg);
		p_hdl->flow = flow;

		rt = ctl_ipp_open_kdf(p_hdl);
		rt |= ctl_ipp_open_ippevt(p_hdl);
		ctl_ipp_handle_unlock(p_hdl->lock);

		/* kdf open failed
			may failed at open_kdf or open_ippevt
		*/
		if (rt != CTL_IPP_E_OK) {
			ctl_ipp_close_kdf(p_hdl);
			ctl_ipp_close_ippevt(p_hdl);

			loc_cpu(loc_cpu_flg);
			ctl_ipp_handle_release(p_hdl);
			unl_cpu(loc_cpu_flg);
			handle = 0;
		} else {
			/* set direct handle */
			if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW ||
				p_hdl->flow == CTL_IPP_FLOW_DIRECT_CCIR) {
				ctl_ipp_set_direct_handle(p_hdl);
			}

			if (is_first_handle) {
				p_hdl->is_first_handle = is_first_handle;
				kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_FIRST_HDL, NULL);
				is_first_handle = FALSE;
			}
			handle = (UINT32)p_hdl;
		}
	}

	ctl_ipp_handle_pool_unlock();
	return handle;
}

INT32 ctl_ipp_close(UINT32 hdl)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_HANDLE *p_hdl;
	FLGPTN wait_flg;
	UINT32 pid, isp_id;

	p_hdl = (CTL_IPP_HANDLE *)hdl;

	if (ctl_ipp_is_init == 0) {
		CTL_IPP_DBG_ERR("ipp is not init\r\n");
		return CTL_IPP_E_SYS;
	}

	if (ctl_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	CTL_IPP_DBG_IND("%s\r\n", p_hdl->name);

	if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_SLEEP) {
		CTL_IPP_DBG_ERR("ipp can't close when sleep \r\n");
	}

	/* task flow lock */
	vos_flag_wait(&wait_flg, g_ctl_ipp_flg_id, CTL_IPP_TASK_LOCK, TWF_CLR);

	// debug dma write protect. clear all protection at ipp close
	ctl_ipp_dbg_dma_wp_clear();

	/* erase ctl ipp queue */
	ctl_ipp_set_pause(TRUE, FALSE);

	/* reset private buffer */
	ctl_ipp_ise_set_pause(TRUE, FALSE);
	ctl_ipp_ise_erase_queue(hdl); // prevent ise task get old info (before ipp close) at next ipp open and process second frame (PM disable at first frame)
	p_hdl->private_buf.buf_info.max_size.w = 0;
	p_hdl->private_buf.buf_info.max_size.h = 0;
	ctl_ipp_ise_set_resume(FALSE);

	ctl_ipp_handle_lock(p_hdl->lock);
	if (p_hdl->sts == CTL_IPP_HANDLE_STS_FREE) {
		CTL_IPP_DBG_ERR("illegal handle(0x%.8x), status(0x%.8x)\r\n", (unsigned int)hdl, (unsigned int)p_hdl->sts);
		ctl_ipp_handle_unlock(p_hdl->lock);
		ctl_ipp_set_resume(FALSE);
		return E_OBJ;
	}
	ctl_ipp_close_kdf(p_hdl);
	if (p_hdl->kdf_hdl != 0) {
		return E_SYS;
	}
	ctl_ipp_close_ippevt(p_hdl);
	if (p_hdl->ipp_evt_hdl != 0) {
		return E_SYS;
	}

	/* reset direct handle to NULL */
	if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW ||
		p_hdl->flow == CTL_IPP_FLOW_DIRECT_CCIR) {
		ctl_ipp_set_direct_handle(NULL);
	}

	/* trigger ctl_ipp_buf task to release buffer */
	if (p_hdl->p_last_rdy_ctrl_info != NULL) {
		ctl_ipp_buf_msg_snd(CTL_IPP_BUF_MSG_RELEASE, (UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl->p_last_rdy_ctrl_info, TRUE);
		p_hdl->p_last_rdy_ctrl_info = NULL;
	}

	/* relase isp ctrl info, flush isp event
		set isp_id to 0xFFFFFFFF to prevent new isp event callback
	*/
	loc_cpu(loc_cpu_flg);
	isp_id = p_hdl->isp_id[CTL_IPP_ALGID_IQ];
	p_hdl->isp_id[CTL_IPP_ALGID_IQ] = 0xFFFFFFFF;
	unl_cpu(loc_cpu_flg);
	ctl_ipp_isp_release_int_info(isp_id);
	ctl_ipp_isp_set_pause(TRUE, FALSE);
	ctl_ipp_isp_erase_queue(isp_id);
	ctl_ipp_isp_set_resume(FALSE);

	ctl_ipp_handle_pool_lock();
	loc_cpu(loc_cpu_flg);
	ctl_ipp_handle_release(p_hdl);
	unl_cpu(loc_cpu_flg);
	ctl_ipp_handle_pool_unlock();

	ctl_ipp_dbg_set_baseinfo(hdl, FALSE);

	/* incase handle is not flushed */
	ctl_ipp_erase_queue(hdl);

	ctl_ipp_buf_set_pause(TRUE, FALSE);
	ctl_ipp_buf_erase_queue((UINT32)p_hdl);
	for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
		if (p_hdl->buf_io_started[pid] == 1) {
			ctl_ipp_buf_iostop_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl, pid);
		}
	}
	ctl_ipp_buf_set_resume(FALSE);

	ctl_ipp_handle_unlock(p_hdl->lock);
	ctl_ipp_set_resume(FALSE);

	/* task flow unlock */
	vos_flag_set(g_ctl_ipp_flg_id, CTL_IPP_TASK_LOCK);

	return E_OK;
}

#if 0
#endif

static INT32 ctl_ipp_ioctl_sndevt(CTL_IPP_HANDLE *p_hdl, void *data)
{
	ER rt;
	CTL_IPP_EVT *p_evt;
	unsigned long loc_cpu_flg;

	if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW || p_hdl->flow == CTL_IPP_FLOW_DIRECT_CCIR) {
		CTL_IPP_DBG_WRN("direct don't surpport ctl send evt\r\n");
	} else {
		loc_cpu(loc_cpu_flg);
		if (p_hdl->sts != CTL_IPP_HANDLE_STS_READY) {
			unl_cpu(loc_cpu_flg);
			CTL_IPP_DBG_WRN("send event failed due to illegal handle(0x%.8x) status\r\n", (unsigned int)p_hdl);
			return CTL_IPP_E_STATE;
		}
		p_evt = (CTL_IPP_EVT *)data;
		rt = ctl_ipp_msg_snd(CTL_IPP_MSG_PROCESS, (UINT32)p_hdl, p_evt->data_addr, p_evt->buf_id, p_hdl->evt_inbuf_fp);
		unl_cpu(loc_cpu_flg);

		if (rt != E_OK) {
			CTL_IPP_DBG_WRN("send event failed\r\n");
			return CTL_IPP_E_QOVR;
		}

		p_hdl->ctl_snd_evt_cnt += 1;

	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_ioctl_sndevt_hdr(CTL_IPP_HANDLE *p_hdl, void *data)
{
	ER rt;
	CTL_IPP_EVT_HDR *p_evt;
	VDO_FRAME *p_main_frm;
	UINT32 frm_num;
	UINT32 buf_id;
	UINT32 i;
	unsigned long loc_cpu_flg;

	if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW || p_hdl->flow == CTL_IPP_FLOW_DIRECT_CCIR) {
		CTL_IPP_DBG_WRN("direct don't surpport ctl send evt\r\n");
	} else {
		p_evt = (CTL_IPP_EVT_HDR *)data;
		p_main_frm = (VDO_FRAME *)p_evt->data_addr[0];

		/* combine hdr frame info */
		if (VDO_PXLFMT_CLASS(p_main_frm->pxlfmt) == VDO_PXLFMT_CLASS_RAW ||
			VDO_PXLFMT_CLASS(p_main_frm->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
			frm_num = VDO_PXLFMT_PLANE(p_main_frm->pxlfmt);
		} else {
			frm_num = 1;
		}

		if (frm_num > CTL_IPP_HDR_MAX_FRAME_NUM) {
			CTL_IPP_DBG_ERR("illegal format 0x%.8x, frm_num %d > max support frm_num = %d\r\n",
				(unsigned int)p_main_frm->pxlfmt, (int)frm_num, CTL_IPP_HDR_MAX_FRAME_NUM);
			return CTL_IPP_E_PAR;
		}

		buf_id = p_evt->buf_id[0];
		if (frm_num > 1) {
			for (i = 1; i < frm_num; i++) {
				p_main_frm->addr[i] = p_evt->data_addr[i];
				buf_id |= p_evt->buf_id[i] << (8 * i);
			}
		}

		loc_cpu(loc_cpu_flg);
		if (p_hdl->sts != CTL_IPP_HANDLE_STS_READY) {
			unl_cpu(loc_cpu_flg);
			CTL_IPP_DBG_WRN("send event failed due to illegal handle(0x%.8x) status\r\n", (unsigned int)p_hdl);
			return CTL_IPP_E_STATE;
		}
		rt = ctl_ipp_msg_snd(CTL_IPP_MSG_PROCESS, (UINT32)p_hdl, p_evt->data_addr[0], buf_id, p_hdl->evt_inbuf_fp);
		unl_cpu(loc_cpu_flg);

		if (rt != E_OK) {
			CTL_IPP_DBG_WRN("send event failed\r\n");
			return CTL_IPP_E_QOVR;
		}

		p_hdl->ctl_snd_evt_cnt += 1;

	}

	return CTL_IPP_E_OK;
}


static INT32 ctl_ipp_ioctl_sndstart(CTL_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;

	rt = ctl_ipp_direct_start((UINT32)p_hdl);
	return rt;
}

static INT32 ctl_ipp_ioctl_sndstop(CTL_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;

	rt = ctl_ipp_direct_stop((UINT32)p_hdl);
	return rt;
}

static INT32 ctl_ipp_ioctl_sndwakeup(CTL_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;

	if (p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
		CTL_IPP_DBG_WRN("wakeup only surpport in direct mode \r\n");
	}

	if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_SLEEP) {
		CTL_IPP_DBG_WRN("ctl wakeup sts error 0x%x\r\n", (int)(p_hdl->sts));
	}

	rt = kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_DIRECT_WAKEUP, NULL);
	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_WRN("kdf send wakeup fail %d\r\n", rt);
		return rt;
	} else {
		ctl_ipp_direct_set_sts(p_hdl, CTL_IPP_HANDLE_STS_DIR_STOP_S3);
	}
	rt = ctl_ipp_direct_start((UINT32)p_hdl);

	return rt;
}

static INT32 ctl_ipp_ioctl_sndsleep(CTL_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;

	if (p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
		CTL_IPP_DBG_WRN("sleep only surpport in direct mode \r\n");
	}

	rt = ctl_ipp_direct_stop((UINT32)p_hdl);
	if (rt != CTL_IPP_E_OK) {
		return rt;
	}
	rt = kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_DIRECT_SLEEP, NULL);
	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_WRN("kdf send sleep fail %d\r\n", rt);
	} else {
		ctl_ipp_direct_set_sts(p_hdl, CTL_IPP_HANDLE_STS_DIR_SLEEP);
	}
	return rt;
}

static INT32 ctl_ipp_ioctl_sndevt_pattern_paste(CTL_IPP_HANDLE *p_hdl, void *data)
{
	ER rt;
	CTL_IPP_EVT *p_evt;
	unsigned long loc_cpu_flg;

	if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW || p_hdl->flow == CTL_IPP_FLOW_DIRECT_CCIR) {
		CTL_IPP_DBG_WRN("direct don't surpport ctl send evt\r\n");
	} else {
		loc_cpu(loc_cpu_flg);
		if (p_hdl->sts != CTL_IPP_HANDLE_STS_READY) {
			unl_cpu(loc_cpu_flg);
			CTL_IPP_DBG_WRN("send event failed due to illegal handle(0x%.8x) status\r\n", (unsigned int)p_hdl);
			return CTL_IPP_E_STATE;
		}
		p_evt = (CTL_IPP_EVT *)data;
		rt = ctl_ipp_msg_snd(CTL_IPP_MSG_PROCESS_PATTERN_PASTE, (UINT32)p_hdl, p_evt->data_addr, p_evt->buf_id, p_hdl->evt_inbuf_fp);
		unl_cpu(loc_cpu_flg);

		if (rt != E_OK) {
			CTL_IPP_DBG_WRN("send event failed\r\n");
			return CTL_IPP_E_QOVR;
		}

		p_hdl->ctl_snd_evt_cnt += 1;

	}

	return CTL_IPP_E_OK;
}

static CTL_IPP_IOCTL_FP ctl_ipp_ioctl_tab[CTL_IPP_IOCTL_MAX] = {
	ctl_ipp_ioctl_sndevt,
	ctl_ipp_ioctl_sndstart,
	ctl_ipp_ioctl_sndstop,
	ctl_ipp_ioctl_sndevt_hdr,
	ctl_ipp_ioctl_sndwakeup,
	ctl_ipp_ioctl_sndsleep,
	ctl_ipp_ioctl_sndevt_pattern_paste,
};

INT32 ctl_ipp_ioctl(UINT32 hdl, UINT32 item, void *data)
{
	CTL_IPP_HANDLE *p_hdl;
	INT32 rt;

	rt = CTL_IPP_E_OK;
	p_hdl = (CTL_IPP_HANDLE *)hdl;

	if (ctl_ipp_is_init == 0) {
		CTL_IPP_DBG_ERR("ipp is not init\r\n");
		return CTL_IPP_E_SYS;
	}

	if (ctl_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	if (item >= CTL_IPP_IOCTL_MAX) {
		return CTL_IPP_E_PAR;
	}

	if (ctl_ipp_ioctl_tab[item] != NULL) {
		rt = ctl_ipp_ioctl_tab[item](p_hdl, data);
	}

	return rt;
}

#if 0
#endif

static INT32 ctl_ipp_set_reg_cb(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CHAR *cbevt_name_str[CTL_IPP_CBEVT_MAX] = {
		"IN_BUF",
		"OUT_BUF",
		"RHE_ISR",
		"IFE_ISR",
		"DCE_ISR",
		"IPE_ISR",
		"IME_ISR",
		"IFE2_ISR",
		"DATASTAMP",
		"PRIMASK"
	};
	CTL_IPP_REG_CB_INFO *p_data = (CTL_IPP_REG_CB_INFO *)data;
	UINT32 cbevt_idx = (p_data->cbevt & ~(IPP_EVENT_ISR_TAG));

	if (cbevt_idx >= CTL_IPP_CBEVT_MAX) {
		CTL_IPP_DBG_WRN("Illegal cbevt 0x%.8x\r\n", (unsigned int)p_data->cbevt);
		return CTL_IPP_E_PAR;
	}

	if (p_data->fp == NULL) {
		CTL_IPP_DBG_WRN("Null function pointer\r\n");
		return CTL_IPP_E_PAR;
	}

	/* bufio callback do not used ipp event interface */
	if (p_data->cbevt == CTL_IPP_CBEVT_OUT_BUF) {
		p_hdl->evt_outbuf_fp = p_data->fp;
	} else if (p_data->cbevt == CTL_IPP_CBEVT_IN_BUF) {
		p_hdl->evt_inbuf_fp = p_data->fp;
	} else {
		if (p_hdl->ipp_evt_hdl == 0) {
			CTL_IPP_DBG_WRN("ipp_evt_hdl 0x%.8x\r\n", (unsigned int)p_data->cbevt);
			return CTL_IPP_E_ID;
		}

		ipp_event_register(p_hdl->ipp_evt_hdl, p_data->cbevt, p_data->fp, cbevt_name_str[cbevt_idx]);
	}
	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_unreg_cb(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_REG_CB_INFO *p_data = (CTL_IPP_REG_CB_INFO *)data;
	UINT32 cbevt_idx = (p_data->cbevt & ~(IPP_EVENT_ISR_TAG));

	if (cbevt_idx >= CTL_IPP_CBEVT_MAX) {
		CTL_IPP_DBG_WRN("Illegal cbevt 0x%.8x\r\n", (unsigned int)p_data->cbevt);
		return CTL_IPP_E_PAR;
	}

	if (p_data->cbevt == CTL_IPP_CBEVT_OUT_BUF) {
		p_hdl->evt_outbuf_fp = NULL;
	} else if (p_data->cbevt == CTL_IPP_CBEVT_IN_BUF) {
		p_hdl->evt_inbuf_fp = NULL;
	} else {
		if (p_hdl->ipp_evt_hdl == 0) {
			CTL_IPP_DBG_WRN("ipp_evt_hdl 0x%.8x\r\n", (unsigned int)p_hdl->ipp_evt_hdl);
			return CTL_IPP_E_ID;
		}

		ipl_event_unregister(p_hdl->ipp_evt_hdl, p_data->cbevt, p_data->fp);
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_algid(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_ALGID *p_algid = (CTL_IPP_ALGID *)data;

	if (p_algid->type < CTL_IPP_ALGID_MAX) {
		/* check if repeat alg id */
		if (ctl_ipp_isp_int_id_validate(p_algid->id) == TRUE) {
			CTL_IPP_HANDLE *p_used_hdl;

			p_used_hdl = ctl_ipp_get_hdl_by_ispid(p_algid->id);
			if (p_used_hdl != NULL) {
				CTL_IPP_DBG_ERR("id %d already used by handle %s\r\n", (int)(p_algid->id), p_used_hdl->name);
				return CTL_IPP_E_PAR;
			}

			if (ctl_ipp_isp_int_id_validate(p_hdl->isp_id[p_algid->type]) == TRUE) {
				/* handle already has a isp_id */
				ctl_ipp_isp_update_int_info(p_hdl->isp_id[p_algid->type], p_algid->id);
			} else {
				/* handle have no isp_id*/
				ctl_ipp_isp_alloc_int_info(p_algid->id);
			}

			/* check if iq callback registered, only print error */
			ctl_ipp_isp_int_chk_callback_registerd();
		}
		p_hdl->isp_id[p_algid->type] = p_algid->id;
	} else {
		CTL_IPP_DBG_WRN("Unsupport type %d\r\n", (int)(p_algid->type));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_bufcfg(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_BUFCFG *p_buf_cfg = (CTL_IPP_BUFCFG *)data;

	if (p_buf_cfg->start_addr == 0) {
		return CTL_IPP_E_PAR;
	}

	if (p_buf_cfg->size < p_hdl->private_buf.buf_info.buf_size) {
		return CTL_IPP_E_NOMEM;
	}

	p_hdl->bufcfg = *p_buf_cfg;

	/* initial dummy uv line buffer */
	{
		UINT32 i;
		UINT32 buf_size;
		UINT32 buf_addr;

		buf_size = ALIGN_CEIL(p_hdl->private_buf.buf_info.max_size.w, VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_DUMMY_UV_NUM; i++) {
			if (p_hdl->private_buf.buf_item.dummy_uv[i].lofs != CTL_IPP_BUF_NO_USE) {
				buf_addr = p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.dummy_uv[i].lofs;
				memset((void *)buf_addr, 128, buf_size);
				vos_cpu_dcache_sync(buf_addr, buf_size, VOS_DMA_TO_DEVICE);
			}
		}
	}

	return CTL_IPP_E_OK;
}


static INT32 ctl_ipp_set_in_crop(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_IN_CROP *crp_info = (CTL_IPP_IN_CROP *)data;

	if (p_hdl->flow == CTL_IPP_FLOW_RAW || p_hdl->flow == CTL_IPP_FLOW_VR360 || p_hdl->flow == CTL_IPP_FLOW_CAPTURE_RAW) {
		if (crp_info->mode >= CTL_IPP_IN_CROP_MODE_MAX) {
			return CTL_IPP_E_PAR;
		}

		p_hdl->rtc_info.ife_ctrl.in_crp_mode = crp_info->mode;
		if (p_hdl->rtc_info.ife_ctrl.in_crp_mode == CTL_IPP_IN_CROP_USER) {
			p_hdl->rtc_info.ife_ctrl.in_crp_window = crp_info->crp_window;
		}
	} else {
		if (p_hdl->rtc_info.ife_ctrl.in_crp_mode != CTL_IPP_IN_CROP_NONE) {
			CTL_IPP_DBG_WRN("Unsupport flow %d\r\n", (int)(p_hdl->flow));
			return CTL_IPP_E_PAR;
		}
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_out_path(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH *p_out_path;
	CTL_IPP_IME_OUT_IMG *p_rtc_info;

	p_out_path = (CTL_IPP_OUT_PATH *)data;

	if (p_hdl->flow == CTL_IPP_FLOW_IPE_D2D && p_out_path->pid != CTL_IPP_OUT_PATH_ID_1) {
		CTL_IPP_DBG_WRN("IPE_D2D only support CTL_IPP_OUT_PATH_ID_1\r\n");
		return CTL_IPP_E_PAR;
	}

	if (p_hdl->flow == CTL_IPP_FLOW_DCE_D2D && p_out_path->pid != CTL_IPP_OUT_PATH_ID_1) {
		CTL_IPP_DBG_WRN("DCE_D2D only support CTL_IPP_OUT_PATH_ID_1\r\n");
		return CTL_IPP_E_PAR;
	}

	if (p_out_path->enable == ENABLE) {
		if (p_hdl->flow == CTL_IPP_FLOW_IPE_D2D) {
			if (p_out_path->fmt != VDO_PXLFMT_YUV444 &&
				p_out_path->fmt != VDO_PXLFMT_YUV422 &&
				p_out_path->fmt != VDO_PXLFMT_YUV420) {
				CTL_IPP_DBG_WRN("IPE D2D not support fmt 0x%.8x\r\n", (unsigned int)p_out_path->fmt);
				return CTL_IPP_E_PAR;
			}
		}

		if (p_hdl->flow == CTL_IPP_FLOW_DCE_D2D) {
			if (p_out_path->fmt != VDO_PXLFMT_YUV422 &&
				p_out_path->fmt != VDO_PXLFMT_YUV420) {
				CTL_IPP_DBG_WRN("DCE D2D not support fmt 0x%.8x\r\n", (unsigned int)p_out_path->fmt);
				return CTL_IPP_E_PAR;
			}
		}

		if (p_out_path->fmt == VDO_PXLFMT_RGB888_PLANAR && p_out_path->pid != CTL_IPP_OUT_PATH_ID_1) {
			CTL_IPP_DBG_WRN("Only Path1 support VDO_PXLFMT_RGB888\r\n");
			return CTL_IPP_E_PAR;
		}

		if (VDO_PXLFMT_CLASS(p_out_path->fmt) == VDO_PXLFMT_CLASS_NVX) {
			if (p_out_path->pid != CTL_IPP_OUT_PATH_ID_1 && p_out_path->pid != CTL_IPP_OUT_PATH_ID_5) {
				CTL_IPP_DBG_WRN("Only Path1/5 support compress\r\n");
				return CTL_IPP_E_PAR;
			}
		}

		if (p_out_path->fmt == VDO_PXLFMT_520_IR8 || p_out_path->fmt == VDO_PXLFMT_520_IR16) {
			if (p_out_path->pid != CTL_IPP_OUT_PATH_ID_6) {
				CTL_IPP_DBG_WRN("Only Path6 support bayer(IR) subout\r\n");
				return CTL_IPP_E_PAR;
			}
			if (p_hdl->flow != CTL_IPP_FLOW_RAW && p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
				CTL_IPP_DBG_WRN("Only raw/direct_raw flow support bayer(IR) subout\r\n");
				return CTL_IPP_E_PAR;
			}
		}

		if ((p_out_path->fmt & 0xfff) == 0x420 || (p_out_path->fmt & 0xfff) == 0x422) {
			/* check crop x, y is 2 align */
			if ((p_out_path->crp_window.x & 1) || (p_out_path->crp_window.y & 1)) {
				CTL_IPP_DBG_WRN("crop start(%d, %d) should be 2x align when fmt 0x%.8x\r\n",
					p_out_path->crp_window.x, p_out_path->crp_window.y, p_out_path->fmt);
				return CTL_IPP_E_PAR;
			}
		}
	}

	if (p_out_path->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_rtc_info = &p_hdl->rtc_info.ime_ctrl.out_img[p_out_path->pid];

		/* chk bufio status for io_start */
		if (p_rtc_info->enable == DISABLE && p_out_path->enable == ENABLE) {
			if (p_hdl->buf_io_started[p_out_path->pid] == 0) {
				ctl_ipp_buf_iostart_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl, p_out_path->pid);
			} else {
				CTL_IPP_DBG_ERR("set path_%d failed, must flush before restart\r\n", (int)p_out_path->pid);
				return CTL_IPP_E_STATE;
			}
		}

		p_rtc_info->enable = p_out_path->enable;
		p_rtc_info->dma_enable = p_rtc_info->enable;
		if (p_rtc_info->enable == ENABLE) {
			p_rtc_info->fmt = p_out_path->fmt;
			p_rtc_info->lofs[0] = p_out_path->lofs;
			p_rtc_info->lofs[1] = ctl_ipp_util_y2uvlof(p_rtc_info->fmt, p_out_path->lofs);
			p_rtc_info->lofs[2] = p_rtc_info->lofs[1];
			p_rtc_info->size = p_out_path->size;
			p_rtc_info->crp_window = p_out_path->crp_window;
		}
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_out_path->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_func_en(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_FUNC func_mask[CTL_IPP_FLOW_MAX] = {
		/* UNKNOWN	*/	CTL_IPP_FUNC_NONE,
		/* RAW		*/	CTL_IPP_FUNC_WDR | CTL_IPP_FUNC_SHDR | CTL_IPP_FUNC_DEFOG | CTL_IPP_FUNC_3DNR | CTL_IPP_FUNC_3DNR_STA | CTL_IPP_FUNC_DATASTAMP | CTL_IPP_FUNC_PRIMASK | CTL_IPP_FUNC_PM_PIXELIZTION | CTL_IPP_FUNC_YUV_SUBOUT | CTL_IPP_FUNC_VA_SUBOUT | CTL_IPP_FUNC_GDC | CTL_IPP_FUNC_DIRECT_SCL_UP,
		/* DIRECT	*/	CTL_IPP_FUNC_WDR | CTL_IPP_FUNC_SHDR | CTL_IPP_FUNC_DEFOG | CTL_IPP_FUNC_3DNR | CTL_IPP_FUNC_3DNR_STA | CTL_IPP_FUNC_DATASTAMP | CTL_IPP_FUNC_PRIMASK | CTL_IPP_FUNC_PM_PIXELIZTION | CTL_IPP_FUNC_YUV_SUBOUT | CTL_IPP_FUNC_VA_SUBOUT | CTL_IPP_FUNC_GDC | CTL_IPP_FUNC_DIRECT_SCL_UP,
		/* CCIR		*/	CTL_IPP_FUNC_WDR | CTL_IPP_FUNC_DEFOG | CTL_IPP_FUNC_3DNR | CTL_IPP_FUNC_3DNR_STA | CTL_IPP_FUNC_DATASTAMP | CTL_IPP_FUNC_PRIMASK | CTL_IPP_FUNC_PM_PIXELIZTION | CTL_IPP_FUNC_YUV_SUBOUT | CTL_IPP_FUNC_VA_SUBOUT | CTL_IPP_FUNC_GDC | CTL_IPP_FUNC_DIRECT_SCL_UP,
		/* DIR CCIR	*/	CTL_IPP_FUNC_NONE | CTL_IPP_FUNC_DIRECT_SCL_UP,
		/* IME D2D	*/	CTL_IPP_FUNC_3DNR | CTL_IPP_FUNC_3DNR_STA | CTL_IPP_FUNC_DATASTAMP | CTL_IPP_FUNC_PRIMASK | CTL_IPP_FUNC_PM_PIXELIZTION | CTL_IPP_FUNC_YUV_SUBOUT | CTL_IPP_FUNC_DIRECT_SCL_UP,
		/* IPE D2D	*/	CTL_IPP_FUNC_DEFOG | CTL_IPP_FUNC_VA_SUBOUT,
		/* VR360	*/	CTL_IPP_FUNC_GDC,
		/* DCE D2D	*/	CTL_IPP_FUNC_WDR | CTL_IPP_FUNC_GDC,
		/* CAP RAW	*/	CTL_IPP_FUNC_WDR | CTL_IPP_FUNC_SHDR | CTL_IPP_FUNC_DEFOG | CTL_IPP_FUNC_3DNR | CTL_IPP_FUNC_3DNR_STA | CTL_IPP_FUNC_DATASTAMP | CTL_IPP_FUNC_PRIMASK | CTL_IPP_FUNC_PM_PIXELIZTION | CTL_IPP_FUNC_YUV_SUBOUT | CTL_IPP_FUNC_VA_SUBOUT | CTL_IPP_FUNC_GDC | CTL_IPP_FUNC_DIRECT_SCL_UP,
		/* CAP CCIR	*/	CTL_IPP_FUNC_WDR | CTL_IPP_FUNC_DEFOG | CTL_IPP_FUNC_3DNR | CTL_IPP_FUNC_3DNR_STA | CTL_IPP_FUNC_DATASTAMP | CTL_IPP_FUNC_PRIMASK | CTL_IPP_FUNC_PM_PIXELIZTION | CTL_IPP_FUNC_YUV_SUBOUT | CTL_IPP_FUNC_VA_SUBOUT | CTL_IPP_FUNC_GDC | CTL_IPP_FUNC_DIRECT_SCL_UP,
	};
	CTL_IPP_FUNC p_func = *(CTL_IPP_FUNC *)data;

	/* keep func_en to handle */
	p_hdl->func_en = p_func & func_mask[p_hdl->flow];
	if (p_hdl->func_en != p_func) {
		CTL_IPP_DBG_WRN("not all func support 0x%.8x, flow %d support func 0x%.8x\r\n", (unsigned int)p_func, (int)p_hdl->flow, (unsigned int)func_mask[p_hdl->flow]);
	}

	/* map func_en to baseinfo */
	if (p_hdl->func_en & CTL_IPP_FUNC_SHDR) {
		p_hdl->rtc_info.ife_ctrl.shdr_enable = ENABLE;
	} else {
		p_hdl->rtc_info.ife_ctrl.shdr_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_WDR) {
		p_hdl->rtc_info.dce_ctrl.wdr_enable = ENABLE;
	} else {
		p_hdl->rtc_info.dce_ctrl.wdr_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_DEFOG) {
		p_hdl->rtc_info.ipe_ctrl.defog_enable = ENABLE;
	} else {
		p_hdl->rtc_info.ipe_ctrl.defog_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_VA_SUBOUT) {
		p_hdl->rtc_info.ipe_ctrl.va_enable = ENABLE;
	} else {
		p_hdl->rtc_info.ipe_ctrl.va_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_3DNR) {
		p_hdl->rtc_info.ime_ctrl.tplnr_enable = ENABLE;
		p_hdl->rtc_info.ime_ctrl.tplnr_out_ms_roi_enable = ENABLE;
	} else {
		p_hdl->rtc_info.ime_ctrl.tplnr_enable = DISABLE;
		p_hdl->rtc_info.ime_ctrl.tplnr_out_ms_roi_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_DATASTAMP) {
		p_hdl->rtc_info.ime_ctrl.ds_enable = ENABLE;
	} else {
		p_hdl->rtc_info.ime_ctrl.ds_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_PRIMASK) {
		p_hdl->rtc_info.ime_ctrl.pm_enable = ENABLE;
	} else {
		p_hdl->rtc_info.ime_ctrl.pm_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_PM_PIXELIZTION) {
		if (p_hdl->rtc_info.ime_ctrl.pm_enable) {
			p_hdl->rtc_info.ime_ctrl.pm_pixel_enable = ENABLE;
		} else {
			CTL_IPP_DBG_WRN("CTL_IPP_FUNC_PM_PIXELIZTION must enable with CTL_IPP_FUNC_PRIMASK\r\n");
			p_hdl->rtc_info.ime_ctrl.pm_pixel_enable = DISABLE;
		}
	} else {
		p_hdl->rtc_info.ime_ctrl.pm_pixel_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_YUV_SUBOUT) {
		p_hdl->rtc_info.ime_ctrl.lca_out_enable = ENABLE;
	} else {
		p_hdl->rtc_info.ime_ctrl.lca_out_enable = DISABLE;
	}

	if (p_hdl->func_en & CTL_IPP_FUNC_GDC) {
		p_hdl->rtc_info.dce_ctrl.gdc_enable = ENABLE;
	} else {
		p_hdl->rtc_info.dce_ctrl.gdc_enable = DISABLE;
	}

#if (CTL_IPP_STRP_RULE_SELECT_ENABLE == DISABLE)
	if (p_hdl->func_en & CTL_IPP_FUNC_GDC)
		p_hdl->rtc_info.dce_ctrl.strp_rule = CTL_IPP_STRP_RULE_1;
	else
		p_hdl->rtc_info.dce_ctrl.strp_rule = CTL_IPP_STRP_RULE_2;
#endif

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_scl_method(CTL_IPP_HANDLE *p_hdl, void *data)
{
	p_hdl->rtc_info.ime_ctrl.out_scl_method_sel = *(CTL_IPP_SCL_METHOD_SEL *)data;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_flip(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_FLIP_TYPE *flip = (CTL_IPP_FLIP_TYPE *)data;

	if (p_hdl->flow == CTL_IPP_FLOW_RAW || p_hdl->flow == CTL_IPP_FLOW_VR360 || p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW ||
		p_hdl->flow == CTL_IPP_FLOW_CAPTURE_RAW || p_hdl->flow == CTL_IPP_FLOW_CAPTURE_CCIR) {
		if (*flip >= CTL_IPP_FLIP_MAX) {
			return CTL_IPP_E_PAR;
		}
		p_hdl->rtc_info.ife_ctrl.flip = *flip;
	} else {
		if (*flip != CTL_IPP_FLIP_NONE) {
			CTL_IPP_DBG_WRN("Unsupport flow %d\r\n", (int)(p_hdl->flow));
		}
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_3dnr_refpath_sel(CTL_IPP_HANDLE *p_hdl, void *data)
{
	UINT32 path;

	path = *(UINT32 *)data;

	if (path >= CTL_IPP_OUT_PATH_ID_MAX) {
		CTL_IPP_DBG_WRN("illegal ref_path select %d\r\n", (int)(path));
		return CTL_IPP_E_PAR;
	}
	p_hdl->rtc_info.ime_ctrl.tplnr_in_ref_path = path;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_out_colorspace(CTL_IPP_HANDLE *p_hdl, void *data)
{
	p_hdl->rtc_info.ime_ctrl.out_color_space_sel = *(CTL_IPP_OUT_COLOR_SPACE *)data;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_apply(CTL_IPP_HANDLE *p_hdl, void *data)
{
	unsigned long loc_cpu_flg;

#if 0
	/* check if need fake bufio_start/stop */
	UINT32 i;
	CTL_IPP_OUT_BUF_INFO buf_info;
	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		if (p_hdl->ctrl_info.ime_ctrl.out_img[i].enable == CTL_IPP_ENABLE_INITIAL) {
			if (p_hdl->rtc_info.ime_ctrl.out_img[i].enable != CTL_IPP_ENABLE_INITIAL) {
				buf_info.pid = i;
				ctl_ipp_buf_fp_wrapper((UINT32)p_hdl, (UINT32)p_hdl->evt_outbuf_fp, CTL_IPP_BUF_IO_START, &buf_info);
				ctl_ipp_buf_fp_wrapper((UINT32)p_hdl, (UINT32)p_hdl->evt_outbuf_fp, CTL_IPP_BUF_IO_STOP, &buf_info);
			}
		}
	}
#endif

	/* Changes should apply at next trigger frame */
	vk_spin_lock_irqsave(&p_hdl->spinlock, loc_cpu_flg);
	memcpy((void *)&p_hdl->ctrl_info, (void *)&p_hdl->rtc_info, sizeof(CTL_IPP_BASEINFO));
	*(UINT32 *)data = p_hdl->kdf_snd_evt_cnt;
	vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_flush(CTL_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt = CTL_IPP_E_OK;
	UINT32 pid;
	CTL_IPP_FLUSH_CONFIG *p_cfg;

	/* pause task must unlock handle to prevent dead lock */
	ctl_ipp_handle_unlock(p_hdl->lock);

	if (p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
		rt = ctl_ipp_set_pause(TRUE, FALSE);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("pause ctl task failed\r\n");
			return CTL_IPP_E_SYS;
		}

		rt = kdf_ipp_set_pause(TRUE, FALSE);
		if (rt != CTL_IPP_E_OK) {
			ctl_ipp_set_resume(FALSE);
			CTL_IPP_DBG_WRN("pause kdf task failed\r\n");
			return CTL_IPP_E_SYS;
		}
	}

	ctl_ipp_buf_set_pause(TRUE, FALSE);
	ctl_ipp_handle_lock(p_hdl->lock);

	if(p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
		if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_READY) {
			/* runtime flush, direct mode should wait path disable apply */
			ctl_ipp_handle_wait_proc_end(p_hdl->lock, TRUE, TRUE);
			ctl_ipp_handle_wait_proc_end(p_hdl->lock, TRUE, TRUE);
		} else if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S1) {
			CTL_IPP_DBG_WRN("direct under pause, don't surpport flush\r\n");
			rt = CTL_IPP_E_STATE;
		}
	}

	/* check path enable */
	p_cfg = (CTL_IPP_FLUSH_CONFIG *)data;
	if (p_cfg == NULL) {
		for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
			if (p_hdl->ctrl_info.ime_ctrl.out_img[pid].enable == ENABLE) {
				CTL_IPP_DBG_ERR("flush must be called after path disable, path %d is now enable\r\n", (int)(pid));
				rt = CTL_IPP_E_STATE;
			}
		}
	} else {
		if (p_cfg->pid >= CTL_IPP_OUT_PATH_ID_MAX) {
			CTL_IPP_DBG_WRN("pid %d out of range\r\n", (int)(p_cfg->pid));
			rt = CTL_IPP_E_PAR;
		}  else if (p_hdl->ctrl_info.ime_ctrl.out_img[p_cfg->pid].enable == ENABLE) {
			CTL_IPP_DBG_ERR("flush must be called after path disable, path %d is now enable\r\n", (int)(p_cfg->pid));
			rt = CTL_IPP_E_STATE;
		}
	}


	if (rt != CTL_IPP_E_OK) {
		ctl_ipp_buf_set_resume(FALSE);
		if (p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
			kdf_ipp_set_resume(FALSE);
			ctl_ipp_set_resume(FALSE);
		}
		return rt;
	}

	/* flush */
	if (p_cfg == NULL) {
		ctl_ipp_erase_queue((UINT32)p_hdl);
		kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_FLUSH, NULL);
		if (p_hdl->p_last_rdy_ctrl_info != NULL) {
			for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
				ctl_ipp_buf_release_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl->p_last_rdy_ctrl_info, TRUE, pid, CTL_IPP_E_FLUSH);
			}
		}

		if (p_hdl->p_last_cfg_ctrl_info != NULL) {
			for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
				ctl_ipp_buf_release_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl->p_last_cfg_ctrl_info, TRUE, pid, CTL_IPP_E_FLUSH);
			}
		}

		ctl_ipp_buf_erase_queue((UINT32)p_hdl);

		for (pid = CTL_IPP_OUT_PATH_ID_1; pid < CTL_IPP_OUT_PATH_ID_MAX; pid++) {
			if (p_hdl->buf_io_started[pid] == 1) {
				ctl_ipp_buf_iostop_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl, pid);
			}
		}
		/* reset buf_evt count */
		p_hdl->buf_push_evt_cnt = 0;
	} else {
		/* flush specific path with apply_id of handle */
		KDF_IPP_FLUSH_CONFIG kdf_flush_cfg = {0};

		kdf_flush_cfg.pid = p_cfg->pid;
		kdf_ipp_set(p_hdl->kdf_hdl, KDF_IPP_ITEM_FLUSH, (void *)&kdf_flush_cfg);
		ctl_ipp_buf_erase_path_in_queue((UINT32)p_hdl, p_cfg->pid);

		if (p_hdl->p_last_rdy_ctrl_info != NULL) {
			ctl_ipp_buf_release_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl->p_last_rdy_ctrl_info, TRUE, p_cfg->pid, CTL_IPP_E_FLUSH);
		}

		if (p_hdl->p_last_cfg_ctrl_info != NULL) {
			ctl_ipp_buf_release_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl->p_last_cfg_ctrl_info, TRUE, p_cfg->pid, CTL_IPP_E_FLUSH);
		}

		if (p_hdl->buf_io_started[p_cfg->pid] == 1) {
			ctl_ipp_buf_iostop_path((UINT32)p_hdl->evt_outbuf_fp, (UINT32)p_hdl, p_cfg->pid);
		}
	}

	ctl_ipp_buf_set_resume(FALSE);
	if (p_hdl->flow != CTL_IPP_FLOW_DIRECT_RAW) {
		kdf_ipp_set_resume(FALSE);
		ctl_ipp_set_resume(FALSE);
	}

	// direct mode need do close or flush after stop, to prevent un-finish job in queue be processed after next start
	// set status to stop s2 to pass the checking of next start
	if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
		if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3) {
			ctl_ipp_direct_set_sts(p_hdl, CTL_IPP_HANDLE_STS_DIR_STOP_S2);
		}
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_out_path_flip(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_FLIP *p_flip;

	p_flip = (CTL_IPP_OUT_PATH_FLIP *)data;
	if (p_flip->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_hdl->rtc_info.ime_ctrl.out_img[p_flip->pid].flip_enable = p_flip->enable;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_flip->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_low_delay_path(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_ID path;

	path = *(CTL_IPP_OUT_PATH_ID *)data;
	if (path < CTL_IPP_OUT_PATH_ID_MAX) {
		if (path == CTL_IPP_OUT_PATH_ID_4 || path == CTL_IPP_OUT_PATH_ID_6) {
			CTL_IPP_DBG_WRN("CTL_IPP_OUT_PATH_ID_4/6 not support low delay mode\r\n");
			p_hdl->rtc_info.ime_ctrl.low_delay_enable = DISABLE;
			p_hdl->rtc_info.ime_ctrl.low_delay_path = path;
		} else {
			p_hdl->rtc_info.ime_ctrl.low_delay_enable = ENABLE;
			p_hdl->rtc_info.ime_ctrl.low_delay_path = path;
		}
	} else {
		p_hdl->rtc_info.ime_ctrl.low_delay_enable = DISABLE;
		p_hdl->rtc_info.ime_ctrl.low_delay_path = path;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_low_delay_bp(CTL_IPP_HANDLE *p_hdl, void *data)
{
	UINT32 bp;

	bp = *(UINT32 *)data;
	p_hdl->rtc_info.ime_ctrl.low_delay_bp = bp;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_out_path_bufmode(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_BUFMODE *p_bufmode;

	p_bufmode = (CTL_IPP_OUT_PATH_BUFMODE *)data;
	if (p_bufmode->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_hdl->rtc_info.ime_ctrl.out_img[p_bufmode->pid].one_buf_mode_enable = p_bufmode->one_buf_mode_en;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_bufmode->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_out_path_md(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_MD *p_md;

	p_md = (CTL_IPP_OUT_PATH_MD *)data;
	if (p_md->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_hdl->rtc_info.ime_ctrl.out_img[p_md->pid].md_enable = p_md->enable;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_md->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_strp_rule(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_STRP_RULE_SELECT rule;

	rule = *(CTL_IPP_STRP_RULE_SELECT *)data;
	if (rule < CTL_IPP_STRP_RULE_MAX) {
		p_hdl->rtc_info.dce_ctrl.strp_rule = rule;
	} else {
		CTL_IPP_DBG_WRN("strp rule out of bound %d >= %d\r\n", (int)rule, CTL_IPP_STRP_RULE_MAX);
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_out_path_halign(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_HALIGN *p_aln;

	p_aln = (CTL_IPP_OUT_PATH_HALIGN *)data;
	if (p_aln->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_hdl->rtc_info.ime_ctrl.out_img[p_aln->pid].h_align = p_aln->align;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_aln->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_path_order(CTL_IPP_HANDLE *p_hdl, void *data)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_OUT_PATH_ORDER *p_cfg;
	UINT8 temp;
	UINT32 i, j;

	p_cfg = (CTL_IPP_OUT_PATH_ORDER *)data;
	if (p_cfg->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_hdl->buf_push_order[p_cfg->pid] = p_cfg->order;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_cfg->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	/* update pid sequence based on new order
		simple bubble sort with path order
	*/
	vk_spin_lock_irqsave(&p_hdl->spinlock, loc_cpu_flg);
    for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
        for (j = 0; j < CTL_IPP_OUT_PATH_ID_MAX - i - 1; j++) {
			if (p_hdl->buf_push_order[p_hdl->buf_push_pid_sequence[j + 1]] < p_hdl->buf_push_order[p_hdl->buf_push_pid_sequence[j]]) {
                temp = p_hdl->buf_push_pid_sequence[j];
                p_hdl->buf_push_pid_sequence[j] = p_hdl->buf_push_pid_sequence[j + 1];
                p_hdl->buf_push_pid_sequence[j + 1] = temp;
            }
        }
    }
	vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_path_region(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_REGION *p_region;

	p_region = (CTL_IPP_OUT_PATH_REGION *)data;

	if (p_region->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_hdl->rtc_info.ime_ctrl.out_img[p_region->pid].region.enable = p_region->enable;
		p_hdl->rtc_info.ime_ctrl.out_img[p_region->pid].region.bgn_lofs = p_region->bgn_lofs;
		p_hdl->rtc_info.ime_ctrl.out_img[p_region->pid].region.bgn_size = p_region->bgn_size;
		p_hdl->rtc_info.ime_ctrl.out_img[p_region->pid].region.region_ofs = p_region->region_ofs;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_region->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_set_pattern_paste(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_PATTERN_PASTE_INFO *p_pat_paste;
	UINT32 i;

	p_pat_paste = (CTL_IPP_PATTERN_PASTE_INFO *)data;

	if (p_pat_paste->pat_win.x > 100 || p_pat_paste->pat_win.y > 100) {
		CTL_IPP_DBG_ERR("pattern pos (x(%u)%%, y(%u)%%) exceed 100%%\r\n", (unsigned int)(p_pat_paste->pat_win.x), (unsigned int)(p_pat_paste->pat_win.y));
		return CTL_IPP_E_PAR;
	}

	if (p_pat_paste->pat_win.w == 0 || p_pat_paste->pat_win.h == 0) {
		CTL_IPP_DBG_ERR("pattern size (w(%u)%%, h(%u)%%) can't be 0%%\r\n", (unsigned int)(p_pat_paste->pat_win.w), (unsigned int)(p_pat_paste->pat_win.h));
		return CTL_IPP_E_PAR;
	}

	if (p_pat_paste->pat_win.x + p_pat_paste->pat_win.w > 100) {
		CTL_IPP_DBG_ERR("pattern pos x(%u)%% + size w(%u)%% exceed 100%%\r\n", (unsigned int)(p_pat_paste->pat_win.x), (unsigned int)(p_pat_paste->pat_win.w));
		return CTL_IPP_E_PAR;
	}

	if (p_pat_paste->pat_win.y + p_pat_paste->pat_win.h > 100) {
		CTL_IPP_DBG_ERR("pattern pos y(%u)%% + size h(%u)%% exceed 100%%\r\n", (unsigned int)(p_pat_paste->pat_win.y), (unsigned int)(p_pat_paste->pat_win.h));
		return CTL_IPP_E_PAR;
	}

	if (p_pat_paste->pat_info.func_en == 0) {
		CTL_IPP_DBG_WRN("pattern paste must enable data stamp func\r\n");
	}

	for (i = 0; i < 3; i++) {
		p_hdl->rtc_info.ime_ctrl.pat_paste_bgn_color[i] = p_pat_paste->bgn_color[i];
	}
	p_hdl->rtc_info.ime_ctrl.pat_paste_win = p_pat_paste->pat_win;
	p_hdl->rtc_info.ime_ctrl.pat_paste_cst_info = p_pat_paste->pat_cst_info;
	p_hdl->rtc_info.ime_ctrl.pat_paste_plt_info = p_pat_paste->pat_plt_info;
	p_hdl->rtc_info.ime_ctrl.pat_paste_info = p_pat_paste->pat_info;

	return CTL_IPP_E_OK;
}

static CTL_IPP_SET_FP ctl_ipp_set_tab[CTL_IPP_ITEM_MAX] = {
	NULL,
	ctl_ipp_set_reg_cb,
	ctl_ipp_set_unreg_cb,
	ctl_ipp_set_algid,
	ctl_ipp_set_bufcfg,
	ctl_ipp_set_in_crop,
	ctl_ipp_set_out_path,
	ctl_ipp_set_func_en,
	ctl_ipp_set_scl_method,
	ctl_ipp_set_flip,
	ctl_ipp_set_3dnr_refpath_sel,
	ctl_ipp_set_out_colorspace,
	ctl_ipp_set_apply,
	NULL,
	ctl_ipp_set_flush,
	NULL,
	ctl_ipp_set_out_path_flip,
	ctl_ipp_set_low_delay_path,
	ctl_ipp_set_low_delay_bp,
	ctl_ipp_set_out_path_bufmode,
	ctl_ipp_set_out_path_md,
	ctl_ipp_set_strp_rule,
	ctl_ipp_set_out_path_halign,
	ctl_ipp_set_path_order,
	ctl_ipp_set_path_region,
	ctl_ipp_set_pattern_paste,
};

INT32 ctl_ipp_set(UINT32 hdl, UINT32 item, void *data)
{
	CTL_IPP_HANDLE *p_hdl;
	FLGPTN wait_flg;
	INT32 rt = CTL_IPP_E_OK;

	p_hdl = (CTL_IPP_HANDLE *)hdl;

	if (ctl_ipp_is_init == 0) {
		CTL_IPP_DBG_ERR("ipp is not init\r\n");
		return CTL_IPP_E_SYS;
	}

	if (ctl_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	if (item >= CTL_IPP_ITEM_MAX) {
		return CTL_IPP_E_PAR;
	}

	if (ctl_ipp_set_tab[item] != NULL) {
		if (item == CTL_IPP_ITEM_FLUSH) {
			/* task flow lock */
			vos_flag_wait(&wait_flg, g_ctl_ipp_flg_id, CTL_IPP_TASK_LOCK, TWF_CLR);
		}
		ctl_ipp_handle_lock(p_hdl->lock);
		if (p_hdl->sts == CTL_IPP_HANDLE_STS_FREE) {
			CTL_IPP_DBG_WRN("illegal handle(0x%.8x), status(0x%.8x)\r\n", (unsigned int)hdl, (unsigned int)p_hdl->sts);
			ctl_ipp_handle_unlock(p_hdl->lock);
			return CTL_IPP_E_STATE;
		}

		rt = ctl_ipp_set_tab[item](p_hdl, data);
		ctl_ipp_handle_unlock(p_hdl->lock);

		if (item == CTL_IPP_ITEM_FLUSH) {
			/* task flow unlock */
			vos_flag_set(g_ctl_ipp_flg_id, CTL_IPP_TASK_LOCK);
		}
	}

	return rt;
}

#if 0
#endif

static INT32 ctl_ipp_get_flow(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_FLOW_TYPE *p_flow = (CTL_IPP_FLOW_TYPE *)data;
	*p_flow = p_hdl->flow;
	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_algid(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_ALGID *p_algid = (CTL_IPP_ALGID *)data;

	if (p_algid->type < CTL_IPP_ALGID_MAX) {
		p_algid->id = p_hdl->isp_id[p_algid->type];
	} else {
		CTL_IPP_DBG_WRN("Unsupport type %d\r\n", (int)(p_algid->type));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_bufcfg(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_BUFCFG *p_buf_cfg = (CTL_IPP_BUFCFG *)data;

	*p_buf_cfg = p_hdl->bufcfg;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_in_crop(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_IN_CROP *crp_info = (CTL_IPP_IN_CROP *)data;

	if (p_hdl->flow == CTL_IPP_FLOW_RAW) {
		crp_info->mode = p_hdl->ctrl_info.ife_ctrl.in_crp_mode;
		crp_info->crp_window = p_hdl->ctrl_info.ife_ctrl.in_crp_window;
	} else {
		CTL_IPP_DBG_WRN("Unsupport flow %d\r\n", (int)(p_hdl->flow));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_out_path(CTL_IPP_HANDLE *p_hdl, void *data)
{
	/* todo: get path info based on flow type */
	CTL_IPP_OUT_PATH *p_out_path;
	CTL_IPP_IME_OUT_IMG *p_ctrl_info;

	p_out_path = (CTL_IPP_OUT_PATH *)data;
	if (p_out_path->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_ctrl_info = &p_hdl->rtc_info.ime_ctrl.out_img[p_out_path->pid];

		p_out_path->enable = p_ctrl_info->enable;
		p_out_path->fmt = p_ctrl_info->fmt;
		p_out_path->lofs = p_ctrl_info->lofs[0];
		p_out_path->size = p_ctrl_info->size;
		p_out_path->crp_window = p_ctrl_info->crp_window;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_out_path->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}


static INT32 ctl_ipp_get_func_en(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_FUNC *p_func = (CTL_IPP_FUNC *)data;

	*p_func = p_hdl->func_en;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_scl_method(CTL_IPP_HANDLE *p_hdl, void *data)
{
	*(CTL_IPP_SCL_METHOD_SEL *)data = p_hdl->rtc_info.ime_ctrl.out_scl_method_sel;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_flip(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_FLIP_TYPE *flip = (CTL_IPP_FLIP_TYPE *)data;

	*flip = CTL_IPP_FLIP_NONE;
	if (p_hdl->flow == CTL_IPP_FLOW_RAW) {
		*flip = p_hdl->rtc_info.ife_ctrl.flip;
	} else {
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_3dnr_refpath_sel(CTL_IPP_HANDLE *p_hdl, void *data)
{
	UINT32 *path = (UINT32 *)data;

	*path = p_hdl->rtc_info.ime_ctrl.tplnr_in_ref_path;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_out_colorspace(CTL_IPP_HANDLE *p_hdl, void *data)
{
	*(CTL_IPP_OUT_COLOR_SPACE *)data = p_hdl->rtc_info.ime_ctrl.out_color_space_sel;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_buf_pri_query(CTL_IPP_HANDLE *p_hdl, void *data)
{

	UINT32 addr_sta = 0, addr;
	UINT32 width, height, buf_size;
	UINT32 i;
	USIZE size;
	CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_ratio;
	CTL_IPP_PRIVATE_BUF *info = (CTL_IPP_PRIVATE_BUF *)data;
	CHAR *buf_name[CTL_IPP_DBG_BUF_MAX_CNT] = {
		"LCA1",
		"LCA2",
		"LCA3",
		"",
		"3DNR_MO1",
		"3DNR_MO2",
		"3DNR_MO3",
		"",
		"3DNR_MV1",
		"3DNR_MV2",
		"3DNR_MV3",
		"",
		"DEFOG1",
		"DEFOG2",
		"DEFOG3",
		"",
		"PM1",
		"PM2",
		"PM3",
		"",
		"YOUT1",
		"YOUT2",
		"YOUT3",
		"YOUT4",
		"",
		"3DNR_MS_ROI1",
		"3DNR_MS_ROI2",
		"3DNR_MS_ROI3",
		"",
		"3DNR_MS1",
		"3DNR_MS2",
		"3DNR_MS3",
		"",
		"3DNR_STA1",
		"3DNR_STA2",
		"3DNR_STA3",
		"",
		"WDR1",
		"WDR2",
		"WDR3",
		"",
		"VA1",
		"VA2",
		"VA3",
		"",
		"DUMMY_UV1",
		"DUMMY_UV2",
		"DUMMY_UV3",
		"",
	};

	addr = addr_sta;

	if (info == NULL) {
		CTL_IPP_DBG_WRN("Buffer data is NULL\r\n");
		return CTL_IPP_E_PAR;;
	}

	p_hdl->private_buf.buf_info.max_size = info->max_size;
	p_hdl->private_buf.buf_info.func_en = info->func_en;
	p_hdl->private_buf.buf_info.pxlfmt = info->pxlfmt;

	/* lca buffer */
	if (info->func_en & CTL_IPP_FUNC_YUV_SUBOUT) {
		lca_ratio.ratio = CTL_IPP_ISP_IQ_LCA_RATIO;
		lca_ratio.ratio_base = CTL_IPP_RATIO_UNIT_DFT;
		size = ctl_ipp_util_lca_subout_size(info->max_size.w, info->max_size.h, lca_ratio, 0);
		buf_size = ctl_ipp_util_yuvsize(VDO_PXLFMT_YUV444_ONE, size.w, size.h);
		buf_size = ALIGN_CEIL(buf_size, VOS_ALIGN_BYTES);

		for (i = 0; i < CTL_IPP_BUF_LCA_NUM; i++) {
			p_hdl->private_buf.buf_item.lca[i].lofs = addr;
			p_hdl->private_buf.buf_item.lca[i].name = buf_name[CTL_IPP_BUF_LCA_1 + i];
			p_hdl->private_buf.buf_item.lca[i].size = buf_size;
			#if CTL_IPP_SINGLE_BUFFER_ENABLE
			if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW || p_hdl->flow == CTL_IPP_FLOW_DIRECT_CCIR){
				if (i == (CTL_IPP_BUF_LCA_NUM - 1)) {
					addr += buf_size;
				}
			} else {
				addr += buf_size;
			}
			#else
			addr += buf_size;
			#endif
		}
	}

	if (info->func_en & CTL_IPP_FUNC_3DNR) {
		KDRV_IME_TMNR_BUF_SIZE_INFO kdrv_ime_buf_info;

		/* set to 0, in case kdrv ime get failed */
		memset((void *)&kdrv_ime_buf_info, 0, sizeof(KDRV_IME_TMNR_BUF_SIZE_INFO));
		kdrv_ime_buf_info.in_size_h = info->max_size.w;
		kdrv_ime_buf_info.in_size_v = info->max_size.h;
		kdrv_ime_buf_info.in_sta_max_num = CTL_IPP_ISP_IQ_3DNR_STA_MAX_NUM;
		kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, (KDF_IPP_HANDLE *)p_hdl->kdf_hdl, KDRV_IME_PARAM_IPL_TMNR_BUF_SIZE, (void *)&kdrv_ime_buf_info);

		/* mv */
		buf_size = ALIGN_CEIL(kdrv_ime_buf_info.get_mv_size, VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
			addr = ALIGN_CEIL_32(addr); /*mv address need 8word align*/
			p_hdl->private_buf.buf_item.mv[i].lofs = addr;
			p_hdl->private_buf.buf_item.mv[i].name = buf_name[CTL_IPP_BUF_MV_1 + i];
			p_hdl->private_buf.buf_item.mv[i].size = buf_size;
			#if CTL_IPP_SINGLE_BUFFER_ENABLE
			if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW || p_hdl->flow == CTL_IPP_FLOW_DIRECT_CCIR){
				if (i == (CTL_IPP_BUF_3DNR_NUM - 1)) {
					addr += buf_size;
				}
			} else {
				addr += buf_size;
			}
			#else
				addr += buf_size;
			#endif
		}

		/* ms */
		buf_size = ALIGN_CEIL(kdrv_ime_buf_info.get_ms_size, VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
			p_hdl->private_buf.buf_item.ms[i].lofs = addr;
			p_hdl->private_buf.buf_item.ms[i].name = buf_name[CTL_IPP_BUF_MS_1 + i];
			p_hdl->private_buf.buf_item.ms[i].size = buf_size;
			#if CTL_IPP_SINGLE_BUFFER_ENABLE
			if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW || p_hdl->flow == CTL_IPP_FLOW_DIRECT_CCIR){
				if (i == (CTL_IPP_BUF_3DNR_NUM - 1)) {
					addr += buf_size;
				}
			} else {
				addr += buf_size;
			}
			#else
			addr += buf_size;
			#endif
		}

		/* ms roi */
		buf_size = ALIGN_CEIL(kdrv_ime_buf_info.get_ms_roi_size, VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
			p_hdl->private_buf.buf_item.ms_roi[i].lofs = addr;
			p_hdl->private_buf.buf_item.ms_roi[i].name = buf_name[CTL_IPP_BUF_MS_ROI_1 + i];
			p_hdl->private_buf.buf_item.ms_roi[i].size = buf_size;
			addr += buf_size;
		}

		if (info->func_en & CTL_IPP_FUNC_3DNR_STA) {
			/* sta, fix buffer size(not related to image size) */
			buf_size = CTL_IPP_ISP_IQ_3DNR_STA_MAX_NUM * 5;
			buf_size = ALIGN_CEIL(buf_size, VOS_ALIGN_BYTES);
			for (i = 0; i < CTL_IPP_BUF_3DNR_STA_NUM; i++) {
				p_hdl->private_buf.buf_item._3dnr_sta[i].lofs = addr;
				p_hdl->private_buf.buf_item._3dnr_sta[i].name = buf_name[CTL_IPP_BUF_3DNR_STA_1 + i];
				p_hdl->private_buf.buf_item._3dnr_sta[i].size = buf_size;
				addr += buf_size;
			}
		}
	}

	/* always need defog buffer for IPE_LCE Function */
	{
		/* defog fix buffer size, 32x32@32bit */
		buf_size = ALIGN_CEIL((32 * 32 * 4), VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_DFG_NUM; i++) {
			p_hdl->private_buf.buf_item.defog_subout[i].lofs = addr;
			p_hdl->private_buf.buf_item.defog_subout[i].name = buf_name[CTL_IPP_BUF_DFG_1 + i];
			p_hdl->private_buf.buf_item.defog_subout[i].size = buf_size;
			addr += buf_size;
		}
	}

	if (info->func_en & CTL_IPP_FUNC_PM_PIXELIZTION) {
		/* privacy mask */
		width = p_hdl->private_buf.buf_info.max_size.w;
		height = p_hdl->private_buf.buf_info.max_size.h;
		ctl_ipp_ise_cal_pm_size(CTL_IPP_PM_PXL_BLK_08, &width, &height);
		width = ALIGN_CEIL(width*3, 4); // calculate for VDO_PXLFMT_YUV444_ONE lofs (= 3 x width). lofs must align 4
		buf_size = ALIGN_CEIL(width * height, VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_PM_NUM; i++) {
			p_hdl->private_buf.buf_item.pm[i].lofs = addr;
			p_hdl->private_buf.buf_item.pm[i].name = buf_name[CTL_IPP_BUF_PM_1 + i];
			p_hdl->private_buf.buf_item.pm[i].size = buf_size;
			addr += buf_size;
		}
	}

	if (info->func_en & CTL_IPP_FUNC_WDR) {
		/* wdr fix buffer size, 32x32@64bit */
		buf_size = ALIGN_CEIL((32 * 32 * 8), VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_WDR_NUM; i++) {
			p_hdl->private_buf.buf_item.wdr_subout[i].lofs = addr;
			p_hdl->private_buf.buf_item.wdr_subout[i].name = buf_name[CTL_IPP_BUF_WDR_1 + i];
			p_hdl->private_buf.buf_item.wdr_subout[i].size = buf_size;
			addr += buf_size;
		}
	}

	if (info->func_en & CTL_IPP_FUNC_VA_SUBOUT) {
		/* va max 8x8 windows with 2 group @2word(8byte) */
		buf_size = ALIGN_CEIL((8 * 8 * 2 * 8), VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_VA_NUM; i++) {
			p_hdl->private_buf.buf_item.va[i].lofs = addr;
			p_hdl->private_buf.buf_item.va[i].name = buf_name[CTL_IPP_BUF_VA_1 + i];
			p_hdl->private_buf.buf_item.va[i].size = buf_size;
			addr += buf_size;
		}
	}

	if (p_hdl->flow == CTL_IPP_FLOW_CCIR || p_hdl->flow == CTL_IPP_FLOW_DCE_D2D) {
		buf_size = ALIGN_CEIL(info->max_size.w, VOS_ALIGN_BYTES);
		for (i = 0; i < CTL_IPP_BUF_DUMMY_UV_NUM; i++) {
			p_hdl->private_buf.buf_item.dummy_uv[i].lofs = addr;
			p_hdl->private_buf.buf_item.dummy_uv[i].name = buf_name[CTL_IPP_BUF_DUMMY_UV_1 + i];
			p_hdl->private_buf.buf_item.dummy_uv[i].size = buf_size;
			addr += buf_size;
		}
	}

	p_hdl->private_buf.buf_info.buf_size = addr - addr_sta;
	info->buf_size = addr - addr_sta;

	/* stripe num calculation for 3dnr one buffer mode
		only support direct/raw/ccir/dce_d2d flow for calculation
	*/
	if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW || p_hdl->flow == CTL_IPP_FLOW_RAW || p_hdl->flow == CTL_IPP_FLOW_CCIR || p_hdl->flow == CTL_IPP_FLOW_DCE_D2D) {
		KDF_IPP_STRIPE_NUM_INFO strp_num_info = {0};

		strp_num_info.in_size = info->max_size;
		strp_num_info.gdc_enable = (info->func_en & CTL_IPP_FUNC_GDC) ? ENABLE : DISABLE;
		if (kdf_ipp_get(p_hdl->kdf_hdl, KDF_IPP_ITEM_STRIPE_NUM, (void *)&strp_num_info) == CTL_IPP_E_OK) {
			info->max_strp_num = strp_num_info.stripe_num;
		} else {
			CTL_IPP_DBG_WRN("kdf get stripe number failed\r\n");
			info->max_strp_num = 0;
		}
	} else {
		info->max_strp_num = 0;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_push_evt_in_queue(CTL_IPP_HANDLE *p_hdl, void *data)
{
	*(UINT32 *)data = p_hdl->buf_push_evt_cnt;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_out_path_flip(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_FLIP *p_flip;

	p_flip = (CTL_IPP_OUT_PATH_FLIP *)data;
	if (p_flip->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_flip->enable = p_hdl->rtc_info.ime_ctrl.out_img[p_flip->pid].flip_enable;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_flip->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_low_delay_path(CTL_IPP_HANDLE *p_hdl, void *data)
{
	*(UINT32 *)data = p_hdl->rtc_info.ime_ctrl.low_delay_path;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_low_delay_bp(CTL_IPP_HANDLE *p_hdl, void *data)
{
	*(UINT32 *)data = p_hdl->rtc_info.ime_ctrl.low_delay_bp;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_out_path_bufmode(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_BUFMODE *p_bufmode;

	p_bufmode = (CTL_IPP_OUT_PATH_BUFMODE *)data;
	if (p_bufmode->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_bufmode->one_buf_mode_en = p_hdl->rtc_info.ime_ctrl.out_img[p_bufmode->pid].one_buf_mode_enable;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_bufmode->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_out_path_md(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_MD *p_md;

	p_md = (CTL_IPP_OUT_PATH_MD *)data;
	if (p_md->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_md->enable = p_hdl->rtc_info.ime_ctrl.out_img[p_md->pid].md_enable;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_md->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_strp_rule(CTL_IPP_HANDLE *p_hdl, void *data)
{
	*(CTL_IPP_STRP_RULE_SELECT *)data = p_hdl->rtc_info.dce_ctrl.strp_rule;

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_out_path_halign(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_HALIGN *p_aln;

	p_aln = (CTL_IPP_OUT_PATH_HALIGN *)data;
	if (p_aln->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_aln->align = p_hdl->rtc_info.ime_ctrl.out_img[p_aln->pid].h_align;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_aln->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_path_order(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_ORDER *p_cfg;

	p_cfg = (CTL_IPP_OUT_PATH_ORDER *)data;
	if (p_cfg->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_cfg->order = p_hdl->buf_push_order[p_cfg->pid];
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_cfg->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_path_region(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_OUT_PATH_REGION *p_region;

	p_region = (CTL_IPP_OUT_PATH_REGION *)data;
	if (p_region->pid < CTL_IPP_OUT_PATH_ID_MAX) {
		p_region->enable = p_hdl->rtc_info.ime_ctrl.out_img[p_region->pid].region.enable;
		p_region->bgn_lofs = p_hdl->rtc_info.ime_ctrl.out_img[p_region->pid].region.bgn_lofs;
		p_region->bgn_size = p_hdl->rtc_info.ime_ctrl.out_img[p_region->pid].region.bgn_size;
		p_region->region_ofs = p_hdl->rtc_info.ime_ctrl.out_img[p_region->pid].region.region_ofs;
	} else {
		CTL_IPP_DBG_WRN("pid %u out of bound > %u\r\n", (unsigned int)((UINT32)p_region->pid), (unsigned int)(CTL_IPP_OUT_PATH_ID_MAX));
		return CTL_IPP_E_PAR;
	}

	return CTL_IPP_E_OK;
}

static INT32 ctl_ipp_get_pattern_paste(CTL_IPP_HANDLE *p_hdl, void *data)
{
	CTL_IPP_PATTERN_PASTE_INFO *p_pat_paste;
	UINT32 i;

	p_pat_paste = (CTL_IPP_PATTERN_PASTE_INFO *)data;

	for (i = 0; i < 3; i++) {
		p_pat_paste->bgn_color[i] = p_hdl->rtc_info.ime_ctrl.pat_paste_bgn_color[i];
	}
	p_pat_paste->pat_win = p_hdl->rtc_info.ime_ctrl.pat_paste_win;
	p_pat_paste->pat_cst_info = p_hdl->rtc_info.ime_ctrl.pat_paste_cst_info;
	p_pat_paste->pat_plt_info = p_hdl->rtc_info.ime_ctrl.pat_paste_plt_info;
	p_pat_paste->pat_info = p_hdl->rtc_info.ime_ctrl.pat_paste_info;

	return CTL_IPP_E_OK;
}

static CTL_IPP_GET_FP ctl_ipp_get_tab[CTL_IPP_ITEM_MAX] = {
	ctl_ipp_get_flow,
	NULL,
	NULL,
	ctl_ipp_get_algid,
	ctl_ipp_get_bufcfg,
	ctl_ipp_get_in_crop,
	ctl_ipp_get_out_path,
	ctl_ipp_get_func_en,
	ctl_ipp_get_scl_method,
	ctl_ipp_get_flip,
	ctl_ipp_get_3dnr_refpath_sel,
	ctl_ipp_get_out_colorspace,
	NULL,
	ctl_ipp_get_buf_pri_query,
	NULL,
	ctl_ipp_get_push_evt_in_queue,
	ctl_ipp_get_out_path_flip,
	ctl_ipp_get_low_delay_path,
	ctl_ipp_get_low_delay_bp,
	ctl_ipp_get_out_path_bufmode,
	ctl_ipp_get_out_path_md,
	ctl_ipp_get_strp_rule,
	ctl_ipp_get_out_path_halign,
	ctl_ipp_get_path_order,
	ctl_ipp_get_path_region,
	ctl_ipp_get_pattern_paste,
};

INT32 ctl_ipp_get(UINT32 hdl, UINT32 item, void *data)
{
	CTL_IPP_HANDLE *p_hdl;
	INT32 rt = CTL_IPP_E_OK;

	p_hdl = (CTL_IPP_HANDLE *)hdl;

	if (ctl_ipp_is_init == 0) {
		CTL_IPP_DBG_ERR("ipp is not init\r\n");
		return CTL_IPP_E_SYS;
	}

	if (ctl_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	if (item >= CTL_IPP_ITEM_MAX) {
		return CTL_IPP_E_PAR;
	}

	if (ctl_ipp_get_tab[item] != NULL) {
		ctl_ipp_handle_lock(p_hdl->lock);
		if (p_hdl->sts == CTL_IPP_HANDLE_STS_FREE) {
			CTL_IPP_DBG_WRN("illegal handle(0x%.8x), status(0x%.8x)\r\n", (unsigned int)hdl, (unsigned int)p_hdl->sts);
			ctl_ipp_handle_unlock(p_hdl->lock);
			return CTL_IPP_E_STATE;
		}
		rt = ctl_ipp_get_tab[item](p_hdl, data);
		ctl_ipp_handle_unlock(p_hdl->lock);
	}

	return rt;
}

#if 0
#endif

UINT32 ctl_ipp_direct_hdr(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 evt)
{
	VDO_FRAME *p_vdoin_info;
	UINT32 hdr_proc_bit;
	UINT32 frm_num;
	static VDO_FRAME vdoin[CTL_IPP_SIE_ID_MAX_NUM];
	/* collect all vdo frame */
	static VDO_FRAME *p_vdoin_c;
	static UINT32 sie2_addr = 0;
	static UINT32 sie2_lofs = 0;
	static UINT32 sie2_ring_num = 0;
	static VDO_PXLFMT sie2_fmt;
	static UINT32 sie2_decode_ratio;
	unsigned long loc_cpu_flg;

	p_vdoin_info = (VDO_FRAME *)header_addr;
	if (CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_info->reserved[4]) == CTL_IPP_SIE_ID_2){
		loc_cpu(loc_cpu_flg);
		sie2_addr = p_vdoin_info->addr[0];
		sie2_lofs = p_vdoin_info->loff[0];
		sie2_ring_num = p_vdoin_info->reserved[5];
		sie2_fmt = p_vdoin_info->pxlfmt;
		sie2_decode_ratio = p_vdoin_info->reserved[7];
		unl_cpu(loc_cpu_flg);
	}

	if (evt & CTL_IPP_DIRECT_START) {
		hdr_proc_bit = (1 << CTL_IPP_SIE_ID_1) | (1 << CTL_IPP_SIE_ID_2);
		frm_num = VDO_PXLFMT_PLANE(p_vdoin_info->pxlfmt);
		if (frm_num <= 1) {
			CTL_IPP_DBG_WRN("frame num should > 1 when HDR\r\n");
		}

		if (CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_info->reserved[4]) >= CTL_IPP_SIE_ID_MAX_NUM) {
			CTL_IPP_DBG_WRN("sie id overflow %d\r\n", (int)(CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_info->reserved[4])));
		}

		loc_cpu(loc_cpu_flg);
		p_hdl->dir_proc_bit |= (1 << CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_info->reserved[4]));

		memcpy((void *)&vdoin[CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_info->reserved[4])], (void *)p_vdoin_info, sizeof(VDO_FRAME));
		if (p_vdoin_info->reserved[3] != 0) {
			if (p_hdl->dir_proc_bit == hdr_proc_bit) {
				p_vdoin_c->addr[1] = (UINT32) p_vdoin_info;
			} else {
				p_vdoin_c = p_vdoin_info;
			}
		} else {
			if (p_hdl->dir_proc_bit == hdr_proc_bit) {
				p_vdoin_c->addr[1] = (UINT32) &vdoin[CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_info->reserved[4])];
			} else {
				p_vdoin_c = &vdoin[CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_info->reserved[4])];
			}

		}
		unl_cpu(loc_cpu_flg);
		if (p_hdl->dir_proc_bit == hdr_proc_bit) {
			VDO_FRAME *p_vdoin_tmp;
			loc_cpu(loc_cpu_flg);
			if(CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_c->reserved[4]) == CTL_IPP_SIE_ID_2) {
				p_vdoin_tmp = p_vdoin_c;
				p_vdoin_c = (VDO_FRAME *)p_vdoin_c->addr[1];
				p_vdoin_c->addr[1] = (UINT32) p_vdoin_tmp;
			}
			p_hdl->dir_proc_bit = 0;
			unl_cpu(loc_cpu_flg);
			CTL_IPP_DBG_IND("hdr start ready\r\n");
			return (UINT32) p_vdoin_c;
		}
	} else {
		if (CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_info->reserved[4]) == CTL_IPP_SIE_ID_1) {
			loc_cpu(loc_cpu_flg);
			memcpy((void *)&vdoin[CTL_IPP_SIE_ID_1], (void *)p_vdoin_info, sizeof(VDO_FRAME));
			vdoin[CTL_IPP_SIE_ID_1].addr[0] = sie2_addr;
			vdoin[CTL_IPP_SIE_ID_1].loff[0] = sie2_lofs;
			vdoin[CTL_IPP_SIE_ID_1].reserved[5] = sie2_ring_num;
			vdoin[CTL_IPP_SIE_ID_1].pxlfmt = sie2_fmt;
			vdoin[CTL_IPP_SIE_ID_1].reserved[7] = sie2_decode_ratio;
			p_vdoin_c = &vdoin[CTL_IPP_SIE_ID_1];
			unl_cpu(loc_cpu_flg);
			return (UINT32) p_vdoin_c;
		} else {
			return 0;
		}
	}
	return 0;
}

INT32 ctl_ipp_direct_flow_cb(UINT32 event, void *p_in, void *p_out)
{
	/**
		vdo_frm reserved info (sync with CTL_SIE_HEADER_INFO)
		resv1: direct mode get raw flag
		reserved[0]: [52X]:for proc cmd out buf wp using (ipp_dis_wp_en << 31 | sie_wp_en << 30 | set_idx << 4 | ddr_id)
		reserved[1]:
		reserved[2]: [52X]:public buf id(direct mode only)
		reserved[3]: [52X]:public buf addr(direct mode only)
		reserved[4]:
	        - bit 0..7 : [52X] ctl_sie_id (direct mode only)
	        - bit 8 : [52X] ife_chk_data (direct mode only, CTL_SIE_DIRECT_TRIG_START only)
	            * sie1 case : ife_chk_mode(52x/56x hw only support 0, 530 hw support 0,1 default 1)
	            * sie2~x case :ife_chk_en
		reserved[5]: [52X]:direct: ring buffer length, dram: dest crop construction of (x << 16 | y)
		reserved[6]: [52X]:dest crop construction of (w << 16 | h)
		reserved[7]: [52X]:encode rate
	*/

	unsigned long loc_cpu_flg;
	CTL_IPP_HANDLE *p_hdl;
	CTL_IPP_INFO_LIST_ITEM *p_dummy_info;
	INT32 rt = CTL_IPP_E_OK;
	VDO_FRAME *p_vdoin_info;
	UINT32 vdo_in;
	static VDO_FRAME vdoin[CTL_IPP_SIE_ID_MAX_NUM]; // for direct task. vdoin in ctl_ipp_direct_hdr is used in ipp task, which can be interrupted by bp3 isr ctl_ipp_direct_flow_cb, so need another variable to prevent racing
	UINT32 i;


	p_hdl = ctl_ipp_get_direct_handle();
	if (ctl_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		CTL_IPP_DBG_WRN("illegal hdl 0x%.8x\r\n", (unsigned int)p_hdl);
		return CTL_IPP_E_ID;
	}
	if(p_in == NULL) {
		if (event != CTL_IPP_DIRECT_SKIP) {
			CTL_IPP_DBG_WRN("NULL VDO frame %x\r\n", event);
			return CTL_IPP_E_PAR;
		}
	}
	p_vdoin_info = (VDO_FRAME *)p_in;

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "evt 0x%x\r\n", (unsigned int) event);
	if (event == CTL_IPP_DIRECT_IN_RE || event == CTL_IPP_DIRECT_IN_DROP) {
		CTL_IPP_EVT input_re_evt;

		loc_cpu(loc_cpu_flg);
		if(event == CTL_IPP_DIRECT_IN_RE) {
			p_hdl->in_buf_re_cnt++;
		} else {
			p_hdl->in_buf_drop_cnt++;
		}
		unl_cpu(loc_cpu_flg);
		/* release input buffer */
		if (p_vdoin_info->reserved[3] != 0) {
			input_re_evt.buf_id = p_vdoin_info->reserved[2];
			input_re_evt.data_addr = (UINT32) p_in;
			input_re_evt.rev = 0;
			input_re_evt.err_msg = CTL_IPP_E_OK;
			if (p_hdl->evt_inbuf_fp != NULL) {
				ctl_ipp_inbuf_fp_wrapper((UINT32)p_hdl, (UINT32)p_hdl->evt_inbuf_fp, CTL_IPP_CBEVT_IN_BUF_DIR_PROCEND, &input_re_evt);
			} else {
				rt = CTL_IPP_E_NULL_FNNC;
			}
		}
	} else if (event == CTL_IPP_DIRECT_SKIP) {
		loc_cpu(loc_cpu_flg);
		p_hdl->in_pro_skip_cnt++;
		unl_cpu(loc_cpu_flg);
	} else {
	    /* if shdr satrt, collect all vdo frame and trigger IPL */
		if (p_hdl->func_en & CTL_IPP_FUNC_SHDR) {
			vdo_in = ctl_ipp_direct_hdr(p_hdl, (UINT32) p_in, event);
			if (vdo_in == 0) {
				/* reset iq parameters */
				if (event == CTL_IPP_DIRECT_START) {
					p_dummy_info = ctl_ipp_info_alloc((UINT32)p_hdl);
					if (p_dummy_info) {
						memcpy((void *)&p_dummy_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));
					}
					ctl_ipp_isp_event_cb(p_hdl->isp_id[CTL_IPP_ALGID_IQ], ISP_EVENT_PARAM_RST, p_dummy_info, (void *)p_hdl->name);
					if (p_dummy_info) {
						ctl_ipp_info_release(p_dummy_info);
					}
				}
				return 0;
			}
		} else {
			/* reset iq parameters */
			if (event == CTL_IPP_DIRECT_START) {
				p_dummy_info = ctl_ipp_info_alloc((UINT32)p_hdl);
				if (p_dummy_info) {
					memcpy((void *)&p_dummy_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));
				}
				ctl_ipp_isp_event_cb(p_hdl->isp_id[CTL_IPP_ALGID_IQ], ISP_EVENT_PARAM_RST, p_dummy_info, (void *)p_hdl->name);
				if (p_dummy_info) {
					ctl_ipp_info_release(p_dummy_info);
				}
			}
			vdo_in = (UINT32) p_in;
		}

		if (ctl_ipp_direct_get_tsk_en()) { // direct process in ipp task
			// check if ctl_ipp_process is done
			vk_spin_lock_irqsave(&p_hdl->dir_tsk_lock, loc_cpu_flg);
			if (p_hdl->dir_tsk_sts != CTL_IPP_HANDLE_STS_DIR_TSK_IDLE) { // drop frame
				vk_spin_unlock_irqrestore(&p_hdl->dir_tsk_lock, loc_cpu_flg);

				CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "%s direct task not done yet (evt 0x%x). drop\r\n", p_hdl->name, (unsigned int)event);

				loc_cpu(loc_cpu_flg);
				p_hdl->drop_cnt += 1;
				unl_cpu(loc_cpu_flg);

				return rt;
			}
			p_hdl->dir_tsk_sts = CTL_IPP_HANDLE_STS_DIR_TSK_PROC;

			// copy vdo_in for ipp task using
			vdoin[CTL_IPP_SIE_ID_1] = *(VDO_FRAME *)vdo_in;
			if (p_hdl->func_en & CTL_IPP_FUNC_SHDR) {
				for (i = CTL_IPP_SIE_ID_2; i < CTL_IPP_HDR_MAX_FRAME_NUM; i++) {
					vdoin[i] = *(VDO_FRAME *)((VDO_FRAME *)vdo_in)->addr[i];
				}
			}

			vk_spin_unlock_irqrestore(&p_hdl->dir_tsk_lock, loc_cpu_flg);

			p_hdl->ctl_snd_evt_cnt += 1;
			// p_hdl->snd_evt_time is configured in ctl_ipp_process when direct task mode

			if (event == CTL_IPP_DIRECT_START) { // direct start must finish in cb to confirm ipp start -> sie start. hdal will start ipp in thread, so no remap in isr issue
				if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_START) {
					CTL_IPP_DBG_WRN("[Start] illegal status 0X%.8x\r\n", (unsigned int)p_hdl->sts);
					rt = CTL_IPP_E_STATE;
				} else {
					rt = ctl_ipp_process_direct(p_hdl, (UINT32)&vdoin[0], 0);
					if (rt == CTL_IPP_E_OK) {
						ctl_ipp_direct_set_sts(p_hdl, CTL_IPP_HANDLE_STS_DIR_READY);
					}

					if (rt != E_OK || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S2 || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3) {
						if (rt != E_OK) {
							CTL_IPP_DBG_ERR("dir start drop (%d)\r\n", rt);
						}
						loc_cpu(loc_cpu_flg);
						p_hdl->drop_cnt += 1;
						unl_cpu(loc_cpu_flg);
					}

					// set dir_tsk_sts to idle in direct start, to indicate the next bp3 event can be sent
					vk_spin_lock_irqsave(&p_hdl->dir_tsk_lock, loc_cpu_flg);
					p_hdl->dir_tsk_sts = CTL_IPP_HANDLE_STS_DIR_TSK_IDLE;
					vk_spin_unlock_irqrestore(&p_hdl->dir_tsk_lock, loc_cpu_flg);
				}
			} else { // set ipp in task
				rt = ctl_ipp_msg_snd(CTL_IPP_MSG_PROCESS_DIRECT, (UINT32)p_hdl, (UINT32)&vdoin[0], event, p_hdl->evt_inbuf_fp);
			}

		} else { // ctl_ipp_dir_tsk_en == 0, direct process in bp3 isr
			p_hdl->ctl_snd_evt_cnt += 1;
			p_hdl->snd_evt_time = ctl_ipp_util_get_syst_counter();

			switch (event) {
			case CTL_IPP_DIRECT_START:
				if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_START) {
					CTL_IPP_DBG_WRN("[Start] illegal status 0X%.8x\r\n", (unsigned int)p_hdl->sts);
					rt = CTL_IPP_E_STATE;
				} else {
					rt = ctl_ipp_process_direct(p_hdl, vdo_in, 0);
					if (rt == CTL_IPP_E_OK) {
						ctl_ipp_direct_set_sts(p_hdl, CTL_IPP_HANDLE_STS_DIR_READY);
					}
				}
				break;

			case CTL_IPP_DIRECT_PROCESS:
				if ((p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3 && dir_prv_sts == CTL_IPP_HANDLE_STS_DIR_STOP_S1) ||
					(p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_SLEEP && dir_prv_sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3)){
					rt = CTL_IPP_E_STATE;
				} else if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_READY && p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_STOP_S1) {
					CTL_IPP_DBG_WRN("[Pro] illegal status 0X%.8x\r\n", (unsigned int)p_hdl->sts);
					rt = CTL_IPP_E_STATE;
				} else {
					rt = ctl_ipp_process_direct(p_hdl, vdo_in, 0);
				}
				break;

			case CTL_IPP_DIRECT_STOP:
				if (p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_STOP_S2 && p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_STOP_S3 && p_hdl->sts != CTL_IPP_HANDLE_STS_DIR_SLEEP) {
					CTL_IPP_DBG_WRN("[Stop] illegal status 0X%.8x\r\n", (unsigned int)p_hdl->sts);
					rt = CTL_IPP_E_STATE;
				}
				break;
			}

			if (rt != E_OK || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S2 || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S3) {
				loc_cpu(loc_cpu_flg);
				p_hdl->drop_cnt += 1;
				unl_cpu(loc_cpu_flg);
			}
		}

	}
	return rt;
}

UINT32 ctl_ipp_get_dir_fp(void *data)
{
	return (UINT32)ctl_ipp_direct_flow_cb;
}

