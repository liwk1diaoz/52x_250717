#include "ctl_sie_buf_int.h"
#include "ctl_sie_isp_int.h"
#include "ctl_sie_id_int.h"
#include "ctl_sie_debug_int.h"
#include "comm/ddr_arb.h"
#include "kflow_common/nvtmpp.h"

#if 0
#endif
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

static VK_DEFINE_SPINLOCK(ctl_sie_buf_lock);
#define ctl_sie_buf_loc_cpu(flags) vk_spin_lock_irqsave(&ctl_sie_buf_lock, flags)
#define ctl_sie_buf_unl_cpu(flags) vk_spin_unlock_irqrestore(&ctl_sie_buf_lock, flags)

#define CTL_SIE_BUF_MSG_Q_NUM   (CTL_SIE_MAX_SUPPORT_ID*12)
static CTL_SIE_LIST_HEAD ctl_sie_buf_free_msg_list_head;
static CTL_SIE_LIST_HEAD ctl_sie_buf_proc_msg_list_head;
static UINT32 ctl_sie_buf_msg_queue_free;
static CTL_SIE_BUF_MSG_EVENT ctl_sie_buf_msg_pool[CTL_SIE_BUF_MSG_Q_NUM];
static BOOL ctl_sie_buf_task_opened = FALSE;
static UINT32 ctl_sie_buf_task_pause_count;
UINT32 ctl_sie_buf_wp_set = 0;
UINT64 ctl_sie_fc_chk[CTL_SIE_ID_MAX_NUM] = {0};

/*
    static function
*/

static UINT32 __ctl_sie_buf_typecast_sie_str_pix(CTL_SIE_RAW_PIX pix)
{
	switch (pix) {
	case CTL_SIE_RGGB_PIX_R:
		return VDO_PIX_RGGB_R;

	case CTL_SIE_RGGB_PIX_GR:
		return VDO_PIX_RGGB_GR;

	case CTL_SIE_RGGB_PIX_GB:
		return VDO_PIX_RGGB_GB;

	case CTL_SIE_RGGB_PIX_B:
		return VDO_PIX_RGGB_B;

	case CTL_SIE_RGBIR_PIX_RGBG_GIGI:
		return VDO_PIX_RGBIR44_RGBG_GIGI;

	case CTL_SIE_RGBIR_PIX_GBGR_IGIG:
		return VDO_PIX_RGBIR44_GBGR_IGIG;

	case CTL_SIE_RGBIR_PIX_GIGI_BGRG:
		return VDO_PIX_RGBIR44_GIGI_BGRG;

	case CTL_SIE_RGBIR_PIX_IGIG_GRGB:
		return VDO_PIX_RGBIR44_IGIG_GRGB;

	case CTL_SIE_RGBIR_PIX_BGRG_GIGI:
		return VDO_PIX_RGBIR44_BGRG_GIGI;

	case CTL_SIE_RGBIR_PIX_GRGB_IGIG:
		return VDO_PIX_RGBIR44_GRGB_IGIG;

	case CTL_SIE_RGBIR_PIX_GIGI_RGBG:
		return VDO_PIX_RGBIR44_GIGI_RGBG;

	case CTL_SIE_RGBIR_PIX_IGIG_GBGR:
		return VDO_PIX_RGBIR44_IGIG_GBGR;

	case CTL_SIE_RCCB_PIX_RC:
		return VDO_PIX_RCCB_RC;

	case CTL_SIE_RCCB_PIX_CR:
		return VDO_PIX_RCCB_CR;

	case CTL_SIE_RCCB_PIX_CB:
		return VDO_PIX_RCCB_CB;

	case CTL_SIE_RCCB_PIX_BC:
		return VDO_PIX_RCCB_BC;

	default:
		ctl_sie_dbg_err("pix err 0x%.8x\r\n", pix);
		return VDO_PIX_RGGB_R;
	}
}

static void __ctl_sie_buf_typecast_pxfmt(CTL_SIE_HDL *ctrl_hdl, UINT32 head_idx)
{
	UINT32 bitdepth, out_frm_num = 1;

	switch (ctrl_hdl->param.data_fmt) {
	case CTL_SIE_BAYER_8:
		bitdepth = 8;
		break;

	case CTL_SIE_BAYER_10:
		bitdepth = 10;
		break;

	case CTL_SIE_BAYER_12:
		bitdepth = 12;
		break;

	case CTL_SIE_BAYER_16:
		bitdepth = 16;
		break;

	case CTL_SIE_YUV_422_SPT:
	case CTL_SIE_YUV_422_NOSPT:
		bitdepth = VDO_PXLFMT_YUV422;
		break;

	case CTL_SIE_YUV_420_SPT:
		bitdepth = VDO_PXLFMT_YUV420;
		break;

	default:
		ctl_sie_dbg_err("data fmt err %d\r\n", (int)(ctrl_hdl->param.data_fmt));
		bitdepth = 8;
		break;
	}

	if (ctl_sie_typecast_raw(ctrl_hdl->param.data_fmt)) {
		out_frm_num = 1;
		if (ctrl_hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_SHDR] && ctrl_hdl->ctrl.total_frame_num > 1) {
			out_frm_num = ctrl_hdl->ctrl.total_frame_num;
 		}
		if (ctrl_hdl->param.encode_en) {
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.pxlfmt = VDO_PXLFMT_MAKE_NRX(bitdepth, out_frm_num, __ctl_sie_buf_typecast_sie_str_pix(ctrl_hdl->ctrl.rtc_obj.rawcfa_crp));
		} else {
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.pxlfmt = VDO_PXLFMT_MAKE_RAW(bitdepth, out_frm_num, __ctl_sie_buf_typecast_sie_str_pix(ctrl_hdl->ctrl.rtc_obj.rawcfa_crp));
		}
	} else {
		if (ctrl_hdl->param.ccir_info_int.dvi_mode == CTL_DVI_MODE_SD) {
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.pxlfmt = bitdepth | VDO_PIX_YCC_BT601;
		} else {
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.pxlfmt = bitdepth | VDO_PIX_YCC_BT709;
		}
	}
}

static CTL_SIE_BUF_MSG_EVENT *__ctl_sie_buf_msg_pool_get_msg(UINT32 idx)
{
	if (idx >= CTL_SIE_BUF_MSG_Q_NUM) {
		ctl_sie_dbg_err("msg idx %d ovf\r\n", (int)(idx));
		return NULL;
	}
	return &ctl_sie_buf_msg_pool[idx];
}

static void __ctl_sie_buf_msg_reset(CTL_SIE_BUF_MSG_EVENT *p_event)
{
	if (p_event) {
		p_event->cmd = 0;
		p_event->param[0] = 0;
		p_event->param[1] = 0;
		p_event->param[2] = 0;
		p_event->rev[0] = CTL_SIE_BUF_MSG_STS_FREE;
	}
}

static CTL_SIE_BUF_MSG_EVENT *__ctl_sie_buf_msg_lock(void)
{
	CTL_SIE_BUF_MSG_EVENT *p_event = NULL;
	unsigned long flags;

	loc_cpu(flags);
	if (!vos_list_empty(&ctl_sie_buf_free_msg_list_head)) {
		p_event = vos_list_entry(ctl_sie_buf_free_msg_list_head.next, CTL_SIE_BUF_MSG_EVENT, list);
		vos_list_del_init(&p_event->list);

		p_event->rev[0] |= CTL_SIE_BUF_MSG_STS_LOCK;
//		ctl_sie_dbg_trc("SIE_BUF Get free queue idx = %d\r\n", (int)(p_event->rev[1]));
		ctl_sie_buf_msg_queue_free -= 1;
	}
	unl_cpu(flags);
	//ctl_sie_dbg_err("Get free queue fail\r\n");
	return p_event;
}

static void __ctl_sie_buf_msg_unlock(CTL_SIE_BUF_MSG_EVENT *p_event)
{
	unsigned long flags;

	if ((p_event->rev[0] & CTL_SIE_BUF_MSG_STS_LOCK) != CTL_SIE_BUF_MSG_STS_LOCK) {
		ctl_sie_dbg_err("event sts err (%d)\r\n", (int)(p_event->rev[0]));
	} else {

		__ctl_sie_buf_msg_reset(p_event);

		loc_cpu(flags);
		vos_list_add_tail(&p_event->list, &ctl_sie_buf_free_msg_list_head);
		ctl_sie_buf_msg_queue_free += 1;
		unl_cpu(flags);
	}
}

static UINT32 __ctl_sie_buf_dbg_buf_chk(CTL_SIE_HDL *ctrl_hdl, UINT32 buf_addr, UINT32 out_h, UINT32 lofs, BOOL chk_flg)
{
	UINT32 ch0_last_line_addr;
	UINT32 out_pixel[2] = {0};
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	if ((dbg_tab != NULL) && (dbg_tab->chk_msg_type != NULL) && dbg_tab->chk_msg_type(ctrl_hdl->id, CTL_SIE_DBG_MSG_BUF_CHK)) {
		ch0_last_line_addr = buf_addr + lofs * (out_h - 1);

		if (chk_flg == FALSE) {	//ch0 out buffer flag add
			*((volatile UINT32 *)buf_addr) = KFLOW_SIE_BUF_FLAG;
			*((volatile UINT32 *)(ch0_last_line_addr)) = KFLOW_SIE_BUF_FLAG;
			vos_cpu_dcache_sync(buf_addr, lofs, VOS_DMA_TO_DEVICE);
			vos_cpu_dcache_sync(ch0_last_line_addr, lofs, VOS_DMA_TO_DEVICE);
		} else {	//check flag
			vos_cpu_dcache_sync(buf_addr, sizeof(KFLOW_SIE_BUF_FLAG), VOS_DMA_FROM_DEVICE);
			vos_cpu_dcache_sync(ch0_last_line_addr, sizeof(KFLOW_SIE_BUF_FLAG), VOS_DMA_FROM_DEVICE);
			out_pixel[0] = *((volatile UINT32 *)buf_addr);
			out_pixel[1] = *((volatile UINT32 *)ch0_last_line_addr);
			if (out_pixel[0] == KFLOW_SIE_BUF_FLAG || out_pixel[1] == KFLOW_SIE_BUF_FLAG) {
				DBG_ERR("out addr %x, out value fail %x %x\r\n", buf_addr, out_pixel[0], out_pixel[1]);
				return CTL_SIE_E_PAR;
			}
		}
	}
	return CTL_SIE_E_OK;
}

static void __ctl_sie_buf_dbg_buf_wp(CTL_SIE_HDL *ctrl_hdl, CTL_SIE_HEAD_IDX head_idx, BOOL protect_en)
{
    DMA_WRITEPROT_ATTR attrib = {0};
	UINT32 ddr_id = DDR_ARB_1;
	UINT32 disable_wp_by_sie = 0, protect_set = 0;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	if ((dbg_tab != NULL) && (dbg_tab->chk_msg_type != NULL) && (dbg_tab->chk_msg_type(ctrl_hdl->id, CTL_SIE_DBG_MSG_BUF_DMA_WP))) {
		dbg_tab->get_buf_wp_info(ctrl_hdl->id, &ddr_id, &disable_wp_by_sie);
	} else {
		return;
	}

	if (protect_en) {
	    memset((void *)&attrib.mask, 0xff, sizeof(DMA_CH_MSK));
		switch (ctrl_hdl->id) {
		case CTL_SIE_ID_1:
			attrib.mask.SIE_0 = 0;
			break;

		case CTL_SIE_ID_2:
			attrib.mask.SIE2_0 = 0;
			break;

		case CTL_SIE_ID_3:
			attrib.mask.SIE3_0 = 0;
			break;

		case CTL_SIE_ID_4:
			attrib.mask.SIE4_0 = 0;
			break;

		case CTL_SIE_ID_5:
			attrib.mask.SIE5_0 = 0;
			break;

		default:
			break;
		}
	    attrib.level = DMA_WPLEL_DETECT;
		if (ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_0] != 0) {
		    attrib.protect_rgn_attr[DMA_PROT_RGN0].en = ENABLE;
		    attrib.protect_rgn_attr[DMA_PROT_RGN0].starting_addr = nvtmpp_sys_va2pa(ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_0]);
	    	attrib.protect_rgn_attr[DMA_PROT_RGN0].size = ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].data_size;

			protect_set = ((ctl_sie_buf_wp_set++) % WPSET_COUNT);
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[0] = ((((~disable_wp_by_sie) & 0x1) << 31) | ((protect_en & 0x1) << 30) | ((protect_set & 0xf) << 4) | (ddr_id & 0xf));
		    arb_enable_wp((DDR_ARB)ddr_id, protect_set, &attrib);
		}
	} else {
		if ((((ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[0] >> 31) & 0x1) == 0) && (((ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[0] >> 30) & 0x1) == 1) && (protect_set < WPSET_COUNT)) {
	    		arb_disable_wp((DDR_ARB)ddr_id, (ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[0] >> 4) & 0xf);
		}
	}
}

#if 0
#endif
/*
	public api
*/

UINT32 ctl_sie_buf_get_free_queue_num(void)
{
	unsigned long flags;
	UINT32 rt;

	loc_cpu(flags);
	rt = ctl_sie_buf_msg_queue_free;
	unl_cpu(flags);

	return rt;
}

UINT32 ctl_sie_buf_get_ca_out_size(CTL_SIE_ID id)
{
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);

	if (sie_limit) {
		return ALIGN_CEIL((((sie_limit->ca_win_max_num.w * sie_limit->ca_win_max_num.h) * 5) << 1), VOS_ALIGN_BYTES);
	} else {
		return 0;
	}
}

UINT32 ctl_sie_buf_get_la_out_size(CTL_SIE_ID id)
{
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);

	if (sie_limit) {
		return ALIGN_CEIL(((((sie_limit->la_win_max_num.w * sie_limit->la_win_max_num.h) << 1) << 1) + (64 << 1)), VOS_ALIGN_BYTES);
	} else {
		return 0;
	}
}

INT32 ctl_sie_buf_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3)
{
	CTL_SIE_BUF_MSG_EVENT *p_event;
	CTL_SIE_HEADER_INFO *int_head = NULL;
	CTL_SIE_HEADER_INFO *snd_out_header;
	CTL_SIE_CTRL_OBJ *sie_ctrl_info = NULL;
	unsigned long flags;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	p_event = __ctl_sie_buf_msg_lock();
	if (p_event == NULL) {
		ctl_sie_dbg_err("cmd %d, fail, buffer will occupy by sie\r\n", (int)(cmd));
		return CTL_SIE_E_QOVR;
	}
	p_event->cmd = cmd;

	if (cmd == CTL_SIE_BUF_MSG_IGNORE || cmd == CTL_SIE_BUF_MSG_GET_GYRO) {
		p_event->param[0] = p1;
		p_event->param[1] = p2;
		p_event->param[2] = p3;
	} else { // if ((cmd & CTL_SIE_BUF_MSG_PUSH) || (cmd & CTL_SIE_BUF_MSG_UNLOCK)) {
		int_head = (CTL_SIE_HEADER_INFO *)p3;
		sie_ctrl_info = (CTL_SIE_CTRL_OBJ *)p2;
		if (dbg_tab != NULL && dbg_tab->set_ts != NULL) {
			if (cmd & CTL_SIE_BUF_MSG_PUSH) {
				dbg_tab->set_ts(p1, CTL_SIE_DBG_TS_DRAMEND, ctl_sie_get_fc(p1));
			}
			if (cmd & CTL_SIE_BUF_MSG_UNLOCK) {
				dbg_tab->set_ts(p1, CTL_SIE_DBG_TS_UNLOCK, ctl_sie_get_fc(p1));
			}
		}
		loc_cpu(flags);
		if (int_head->buf_addr == 0) {
			unl_cpu(flags);
			__ctl_sie_buf_msg_unlock(p_event);
			return CTL_SIE_E_OK;
		} else {
			//update internal header info to public buffer
			snd_out_header = (CTL_SIE_HEADER_INFO *)int_head->buf_addr;
			snd_out_header->buf_id = int_head->buf_id;
			snd_out_header->buf_addr = int_head->buf_addr;
			memcpy(&snd_out_header->vdo_frm, &int_head->vdo_frm, sizeof(VDO_FRAME));
			vos_cpu_dcache_sync((VOS_ADDR)snd_out_header, ALIGN_CEIL(sizeof(CTL_SIE_HEADER_INFO), VOS_ALIGN_BYTES), VOS_DMA_TO_DEVICE); // flush [IVOT_N12085_CO-8]
			if (!(cmd & CTL_SIE_BUF_MSG_LOCK)) {
				int_head->buf_addr = 0;	//clear buf_addr
			}
			p_event->param[0] = p1;
			if (sie_ctrl_info != NULL && sie_ctrl_info->bufiocb.cb_fp != NULL) {
				p_event->param[1] = (UINT32)sie_ctrl_info->bufiocb.cb_fp;
			} else {
				p_event->param[1] = 0;
			}
			p_event->param[2] = (UINT32)snd_out_header;
		}
		unl_cpu(flags);
	}

//	ctl_sie_dbg_trc("[BUF][Snd]Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(p1), (unsigned int)(p2), (unsigned int)(p3));

	//add to proc list & trig proc task
	loc_cpu(flags);
	vos_list_add_tail(&p_event->list, &ctl_sie_buf_proc_msg_list_head);
	unl_cpu(flags);
	vos_flag_iset(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_QUEUE_PROC);
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_buf_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	CTL_SIE_BUF_MSG_EVENT *p_event;
	FLGPTN wait_flg;
	UINT32 list_empty_flag;
	unsigned long flags;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	while (1) {
		//check empty or not
		loc_cpu(flags);
		list_empty_flag = vos_list_empty(&ctl_sie_buf_proc_msg_list_head);
		if (!list_empty_flag) {
			p_event = vos_list_entry(ctl_sie_buf_proc_msg_list_head.next, CTL_SIE_BUF_MSG_EVENT, list);
			vos_list_del_init(&p_event->list);
		}
		unl_cpu(flags);

		//is empty enter wait mode
		if (list_empty_flag) {
			vos_flag_wait(&wait_flg, g_ctl_sie_buf_flg_id, CTL_SIE_BUF_QUEUE_PROC, TWF_CLR);
		} else {
			break;
		}
	}

	*cmd = p_event->cmd;
	*p1 = p_event->param[0];
	*p2 = p_event->param[1];
	*p3 = p_event->param[2];
	if (p_event->cmd != CTL_SIE_BUF_MSG_IGNORE) {
		// performance profile
		if (dbg_tab != NULL && dbg_tab->set_ts != NULL) {
			dbg_tab->set_ts(*p1, CTL_SIE_DBG_TS_RCVMSG, ctl_sie_get_fc(*p1));
		}
	}
	__ctl_sie_buf_msg_unlock(p_event);
//	ctl_sie_dbg_trc("[BUF][Rcv]Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(*cmd), (unsigned int)(*p1), (unsigned int)(*p2), (unsigned int)(*p3));
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_buf_msg_flush(void)
{
	UINT32 cmd, param[3];
	INT32 rt = CTL_SIE_E_OK;

	while (!vos_list_empty(&ctl_sie_buf_proc_msg_list_head)) {
		rt = ctl_sie_buf_msg_rcv(&cmd, &param[0], &param[1], &param[2]);
		// todo: inform outside for flush? use callback?

		if (rt != CTL_SIE_E_OK) {
			ctl_sie_dbg_err("flush fail (%d)\r\n", (int)(rt));
		}
		/* Unlock buffer when flush */
		if (cmd != CTL_SIE_BUF_MSG_IGNORE) {
			ctl_sie_buf_io_cfg(param[0], CTL_SIE_BUF_IO_UNLOCK, param[1], param[2]);
		}
	}

	return rt;
}

INT32 ctl_sie_buf_erase_queue(UINT32 handle)
{
	CTL_SIE_LIST_HEAD *p_list;
	CTL_SIE_BUF_MSG_EVENT *p_event;
	unsigned long flags;
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(handle);

	loc_cpu(flags);
	if (!vos_list_empty(&ctl_sie_buf_proc_msg_list_head)) {
		vos_list_for_each(p_list, &ctl_sie_buf_proc_msg_list_head) {
			p_event = vos_list_entry(p_list, CTL_SIE_BUF_MSG_EVENT, list);
			if (p_event->param[0] == handle) {
				if (p_event->cmd != CTL_SIE_BUF_MSG_IGNORE && p_event->cmd != CTL_SIE_BUF_MSG_GET_GYRO) {
					if (hdl != NULL) {
						hdl->ctrl.frame_ctl_info.flush_cnt++;
					}
					ctl_sie_buf_io_cfg(p_event->param[0], CTL_SIE_BUF_IO_UNLOCK, p_event->param[1], p_event->param[2]);
				}
				p_event->cmd = CTL_SIE_BUF_MSG_IGNORE;
			}
		}
	}
	unl_cpu(flags);

	return CTL_SIE_E_OK;
}

INT32 ctl_sie_buf_msg_reset_queue(void)
{
	UINT32 i;
	CTL_SIE_BUF_MSG_EVENT *free_event;

	//init free & proc list head
	VOS_INIT_LIST_HEAD(&ctl_sie_buf_free_msg_list_head);
	VOS_INIT_LIST_HEAD(&ctl_sie_buf_proc_msg_list_head);

	for (i = 0; i < CTL_SIE_BUF_MSG_Q_NUM; i++) {
		free_event = __ctl_sie_buf_msg_pool_get_msg(i);
		__ctl_sie_buf_msg_reset(free_event);
		free_event->rev[1] = i;
		vos_list_add_tail(&free_event->list, &ctl_sie_buf_free_msg_list_head);
	}
	ctl_sie_buf_msg_queue_free = CTL_SIE_BUF_MSG_Q_NUM;
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_buf_open_tsk(void)
{
	if (ctl_sie_buf_task_opened) {
		ctl_sie_dbg_err("re-open\r\n");
		return CTL_SIE_E_SYS;
	}

	if (g_ctl_sie_buf_flg_id == 0) {
		ctl_sie_dbg_err("buf_flg_id %d\r\n", (int)(g_ctl_sie_buf_flg_id));
		return CTL_SIE_E_SYS;
	}

	/* ctrl buf init */
	ctl_sie_buf_task_pause_count = 0;

	/* reset flag */
	vos_flag_clr(g_ctl_sie_buf_flg_id, FLGPTN_BIT_ALL);
	vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_PROC_INIT);

	/* reset queue */
	ctl_sie_buf_msg_reset_queue();

	/* start ctrl task */
	THREAD_CREATE(g_ctl_sie_buf_tsk_id, ctl_sie_buf_tsk, NULL, "ctl_sie_buf_tsk");
	if (g_ctl_sie_buf_tsk_id == 0) {
		ctl_sie_dbg_err("thread create fail tsk_id %d\r\n", (int)g_ctl_sie_buf_tsk_id);
		return CTL_SIE_E_ID;
	}

	THREAD_SET_PRIORITY(g_ctl_sie_buf_tsk_id, CTL_SIE_BUF_TSK_PRI);
	THREAD_RESUME(g_ctl_sie_buf_tsk_id);

	ctl_sie_buf_task_opened = TRUE;

	/* set ctrl buf task to processing */
	ctl_sie_buf_set_resume(TRUE);
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_buf_close_tsk(void)
{
	FLGPTN wait_flg = 0;

	if (!ctl_sie_buf_task_opened) {
		ctl_sie_dbg_err("re-close\r\n");
		return CTL_SIE_E_SYS;
	}

	while (ctl_sie_buf_task_pause_count) {
		ctl_sie_buf_set_pause(ENABLE, ENABLE);
	}

	vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_RES);
	if (vos_flag_wait_timeout(&wait_flg, g_ctl_sie_buf_flg_id, CTL_SIE_BUF_EXIT_END, (TWF_ORW | TWF_CLR), vos_util_msec_to_tick(1000)) != E_OK) {
		/* if wait not ok, force destroy task */
		ctl_sie_dbg_err("wait buf tsk exit timeout, cur_flg = 0x%x, force destroy\r\n", vos_flag_chk(g_ctl_sie_buf_flg_id, 0xffffffff));
		THREAD_DESTROY(g_ctl_sie_buf_tsk_id);
	}

	ctl_sie_buf_task_opened = FALSE;
	return CTL_SIE_E_OK;
}

THREAD_DECLARE(ctl_sie_buf_tsk, p1)
{
	INT32 rt;
	UINT32 cmd, param[3];
	FLGPTN wait_flag = 0;

	THREAD_ENTRY();
	goto CTL_SIE_BUF_PAUSE;

CTL_SIE_BUF_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_IDLE | CTL_SIE_BUF_TASK_RESUME_END);
		PROFILE_TASK_IDLE();
		rt = ctl_sie_buf_msg_rcv(&cmd, &param[0], &param[1], &param[2]);
		PROFILE_TASK_BUSY();
		vos_flag_clr(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_IDLE);

		if (rt != CTL_SIE_E_OK) {
			ctl_sie_dbg_err("sie_buf Rcv Msg error (%d)\r\n", (int)(rt));
		} else {
			/* process event */
//			ctl_sie_dbg_trc("[BUF][TSK]Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));

			vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_CHK);
			vos_flag_wait(&wait_flag, g_ctl_sie_buf_flg_id, (CTL_SIE_BUF_TASK_PAUSE | CTL_SIE_BUF_TASK_CHK), (TWF_ORW | TWF_CLR));

			if (cmd == CTL_SIE_BUF_MSG_IGNORE) {
				/* do nothing */
			} else {
				if (cmd & CTL_SIE_BUF_MSG_GET_GYRO) {
					ctl_sie_module_get_gyro_data((UINT32)param[0], param[1], param[2]);
				}

				if (cmd & CTL_SIE_BUF_MSG_LOCK) {
					ctl_sie_buf_io_cfg(param[0], CTL_SIE_BUF_IO_LOCK, param[1], param[2]);
				}

				if (cmd & CTL_SIE_BUF_MSG_PUSH) {
					ctl_sie_buf_io_cfg(param[0], CTL_SIE_BUF_IO_PUSH, param[1], param[2]);
				}

				if (cmd & CTL_SIE_BUF_MSG_UNLOCK) {
					ctl_sie_buf_io_cfg(param[0], CTL_SIE_BUF_IO_UNLOCK, param[1], param[2]);
				}
			}

			if (wait_flag & CTL_SIE_BUF_TASK_PAUSE) {
				break;
			}
			//else {
//				ctl_sie_dbg_err("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));
//			}
		}
	}

CTL_SIE_BUF_PAUSE:
	vos_flag_clr(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_RESUME_END);
	vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_PAUSE_END);
	vos_flag_wait(&wait_flag, g_ctl_sie_buf_flg_id, (CTL_SIE_BUF_TASK_RES | CTL_SIE_BUF_TASK_RESUME | CTL_SIE_BUF_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flag & CTL_SIE_BUF_TASK_RES) {
		goto CTL_SIE_BUF_DESTROY;
	}

	if (wait_flag & CTL_SIE_BUF_TASK_RESUME) {
		vos_flag_clr(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_PAUSE_END);
		if (wait_flag & CTL_SIE_BUF_TASK_FLUSH) {
			ctl_sie_buf_msg_flush();
		}
		goto CTL_SIE_BUF_PROC;
	}

CTL_SIE_BUF_DESTROY:
	vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_EXIT_END);
	THREAD_RETURN(0);
}

INT32 ctl_sie_buf_set_resume(BOOL b_flush_evt)
{
	FLGPTN wait_flag = 0;
	unsigned long flags;

	if (!ctl_sie_buf_task_opened) {
		return CTL_SIE_E_SYS;
	}
	loc_cpu(flags);
	if (ctl_sie_buf_task_pause_count == 0) {
		ctl_sie_buf_task_pause_count = 1;
		unl_cpu(flags);
		if (b_flush_evt) {
			vos_flag_set(g_ctl_sie_buf_flg_id, (CTL_SIE_BUF_TASK_RESUME | CTL_SIE_BUF_TASK_FLUSH));
		} else {
			vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_RESUME);
		}
		if (vos_flag_wait_timeout(&wait_flag, g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_RESUME_END, TWF_ORW, vos_util_msec_to_tick(1000)) != 0) {
			ctl_sie_dbg_err("wait buf tsk resume end timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_sie_buf_flg_id, 0xffffffff), CTL_SIE_BUF_TASK_RESUME_END);
		}
	} else {
		unl_cpu(flags);
	}

	return CTL_SIE_E_OK;
}

INT32 ctl_sie_buf_set_pause(BOOL b_wait_end, BOOL b_flush_evt)
{
	FLGPTN wait_flg = 0;
	unsigned long flags;
	UINT32 buf_msg_par = 0;

	if (!ctl_sie_buf_task_opened) {
		return CTL_SIE_E_SYS;
	}

	loc_cpu(flags);
	if (ctl_sie_buf_task_pause_count != 0) {
		ctl_sie_buf_task_pause_count = 0;
		unl_cpu(flags);
		vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_PAUSE);
		/* send dummy msg, ctl_sie_buf_tsk must rcv msg to pause */
		ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_IGNORE, buf_msg_par, buf_msg_par, buf_msg_par);

		if (b_wait_end) {
			if (vos_flag_wait_timeout(&wait_flg, g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(1000)) != 0) {
				ctl_sie_dbg_wrn("wait buf tsk pause end timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_sie_buf_flg_id, 0xffffffff), CTL_SIE_BUF_TASK_PAUSE_END);
			}

			if (b_flush_evt) {
				ctl_sie_buf_msg_flush();
			}
		}
	} else {
		unl_cpu(flags);
	}
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_buf_wait_pause_end(void)
{
	FLGPTN wait_flg = 0;

	if (!ctl_sie_buf_task_opened) {
		return CTL_SIE_E_SYS;
	}

	if (vos_flag_wait_timeout(&wait_flg, g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(1000)) != 0) {
		ctl_sie_dbg_err("wait buf tsk pause end timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_sie_buf_flg_id, 0xffffffff), CTL_SIE_BUF_TASK_PAUSE_END);
		return CTL_SIE_E_SYS;
	}
	return CTL_SIE_E_OK;
}

void ctl_sie_buf_queue_flush(CTL_SIE_ID id)
{
	FLGPTN wait_flag = 0;

	if (vos_flag_wait_timeout(&wait_flag, g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_LOCK, TWF_CLR, vos_util_msec_to_tick(1000)) != 0) {
		ctl_sie_dbg_err("wait buf tsk que flush end timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_sie_buf_flg_id, 0xffffffff), CTL_SIE_BUF_TASK_LOCK);
	}

	ctl_sie_buf_set_pause(TRUE, FALSE);
	ctl_sie_buf_erase_queue(id);
	ctl_sie_buf_set_resume(FALSE);

	vos_flag_set(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_TASK_LOCK);
}

#if 0
#endif

void ctl_sie_buf_io_cfg(UINT32 id, CTL_SIE_BUF_IO_CFG io_cfg, UINT32 req_size, UINT32 sie_header_addr)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_HEADER_INFO *head_info;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();
	unsigned long flags;

	if (hdl == NULL || hdl->ctrl.bufiocb.cb_fp == NULL) {
		ctl_sie_dbg_err("HDL/BUFIO_CB NULL\r\n");
		return;
	}

	if (sie_header_addr == 0) {
		return;
	}
	head_info = (CTL_SIE_HEADER_INFO *)sie_header_addr;

	switch (io_cfg) {
	case CTL_SIE_BUF_IO_NEW:
		if (ctl_sie_fc_chk[id] == 0 || ctl_sie_fc_chk[id] != head_info->vdo_frm.count) {
			ctl_sie_fc_chk[id] = head_info->vdo_frm.count;
			vos_flag_iset(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
			hdl->ctrl.bufiocb.cb_fp(CTL_SIE_BUF_IO_NEW, (void *)&req_size, (void *)sie_header_addr);
			vos_flag_iclr(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
			if (head_info->buf_id != 0xffffffff) {
				if (head_info->buf_addr == 0) {
					hdl->ctrl.frame_ctl_info.new_fail_cnt++;
				} else {
					hdl->ctrl.frame_ctl_info.new_ok_cnt++;
				}
			}
		} else {
			ctl_sie_dbg_ind("id %d, skip new buf by fc check cur: %lld, prev: %lld\r\n", id, ctl_sie_fc_chk[id], head_info->vdo_frm.count);
		}
		if (dbg_tab != NULL && dbg_tab->dump_buf_io != NULL) {
			dbg_tab->dump_buf_io(id, io_cfg, req_size, sie_header_addr);
		}
		break;

	case CTL_SIE_BUF_IO_PUSH:
		if (head_info->buf_addr != 0) {
			if (ctl_sie_isp_int_pull_img_out(id, sie_header_addr)) {
				vos_flag_iset(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
				hdl->ctrl.bufiocb.cb_fp(CTL_SIE_BUF_IO_LOCK, (void *)sie_header_addr, NULL);
				vos_flag_iclr(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
			}
			hdl->ctrl.frame_ctl_info.push_cnt++;//update push out counter
			if (dbg_tab != NULL && dbg_tab->dump_buf_io != NULL) {
				dbg_tab->dump_buf_io(id, io_cfg, req_size, sie_header_addr);
			}
			__ctl_sie_buf_dbg_buf_wp(hdl, 0, FALSE);
			if (__ctl_sie_buf_dbg_buf_chk(hdl, head_info->vdo_frm.addr[0], (UINT32)head_info->vdo_frm.size.h, head_info->vdo_frm.loff[0], TRUE) != CTL_SIE_E_OK) {
				ctl_sie_dbg_err("sie %d, fc %llu check fail\r\n", (int)(id+1), head_info->vdo_frm.count);
			}
			vos_flag_iset(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
			hdl->ctrl.bufiocb.cb_fp(io_cfg, (void *)sie_header_addr, NULL);
			vos_flag_iclr(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
		}
		break;

	case CTL_SIE_BUF_IO_LOCK:
		if (head_info->buf_addr != 0) {
			vos_flag_iset(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
			hdl->ctrl.bufiocb.cb_fp(io_cfg, (void *)sie_header_addr, NULL);
			vos_flag_iclr(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
			if (dbg_tab != NULL && dbg_tab->dump_buf_io != NULL) {
				dbg_tab->dump_buf_io(id, io_cfg, req_size, sie_header_addr);
			}
		}
		break;

	case CTL_SIE_BUF_IO_UNLOCK:
		if (head_info->buf_addr != 0) {
			if (head_info->buf_id != CTL_SIE_BUF_CB_IGNOR) {
				hdl->ctrl.frame_ctl_info.drop_cnt++;
			}
			__ctl_sie_buf_dbg_buf_wp(hdl, 0, FALSE);
			vos_flag_iset(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
			hdl->ctrl.bufiocb.cb_fp(io_cfg, (void *)sie_header_addr, NULL);
			vos_flag_iclr(g_ctl_sie_buf_flg_id, CTL_SIE_BUF_CB_OUT);
			if (dbg_tab != NULL && dbg_tab->dump_buf_io != NULL) {
				dbg_tab->dump_buf_io(id, io_cfg, req_size, sie_header_addr);
			}
			ctl_sie_buf_loc_cpu(flags);
			head_info->buf_addr = 0;
			ctl_sie_buf_unl_cpu(flags);
		}
		break;

	default:
		break;
	}
}

void ctl_sie_buf_update_out_size(CTL_SIE_HDL *ctrl_hdl)
{
	UINT32 i;

	memset((void *)&ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0], 0, sizeof(CTL_SIE_CH_OUT_INFO) * CTL_SIE_CH_MAX);
	//update buffer size
	//put header/ca/la buffer on top for nvtmpp ioremap
	//ch2 la, PreGamma Lum/PostGamma Lum @16bit, Histogram 64bin@16bit(680)/128bin@16bit(520)
	if (ctrl_hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE]) {
		ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_2].out_enable = ENABLE;
		ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_2].data_size = ctl_sie_buf_get_la_out_size(ctrl_hdl->id);

	}
	if (ctl_sie_typecast_raw(ctrl_hdl->param.data_fmt)) {
		//ch1 ca, R/G/B/Cnt/IRth @16bit
		if (ctrl_hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB]) {
			ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].out_enable = ENABLE;
			ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].data_size = ctl_sie_buf_get_ca_out_size(ctrl_hdl->id);
		}
		//ch0 raw output, lineoffset * height
		if (ctrl_hdl->param.out_dest == CTL_SIE_OUT_DEST_DRAM || ctrl_hdl->param.out_dest == CTL_SIE_OUT_DEST_BOTH) {
			ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].out_enable = ENABLE;
			ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].data_size = ALIGN_CEIL(ctrl_hdl->param.io_size_info.size_info.sie_scl_out.h * ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_0], VOS_ALIGN_BYTES);
		}
	} else {
		//ccir
		ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].out_enable = ENABLE;
		ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].data_size = ALIGN_CEIL(ctrl_hdl->param.io_size_info.size_info.sie_crp.h * ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_0], VOS_ALIGN_BYTES);
		if (ctrl_hdl->param.data_fmt == CTL_SIE_YUV_422_SPT) {
			ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].out_enable = ENABLE;
			ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].data_size = ALIGN_CEIL(ctrl_hdl->param.io_size_info.size_info.sie_crp.h * ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_1], VOS_ALIGN_BYTES);
		} else if (ctrl_hdl->param.data_fmt == CTL_SIE_YUV_420_SPT) {
			ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].out_enable = ENABLE;
			ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].data_size = ALIGN_CEIL((ctrl_hdl->param.io_size_info.size_info.sie_crp.h>> 1) * ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_1], VOS_ALIGN_BYTES);
		}
	}

	/* gyro data */
	if (ctrl_hdl->ctrl.gyro_ctl_info.gyro_cfg.en) {
		CTL_SIE_EXT_GYRO_DATA gyro_data;

		ctrl_hdl->ctrl.gyro_ctl_info.gyro_buf_sz = ALIGN_CEIL(sizeof(*gyro_data.data_num) + sizeof(*gyro_data.t_crop_start)+ sizeof(*gyro_data.t_crp_end) + ctrl_hdl->ctrl.gyro_ctl_info.gyro_cfg.data_num * GYRO_DATA_ITEM_MAX * sizeof(INT32), VOS_ALIGN_BYTES);
	} else {
		ctrl_hdl->ctrl.gyro_ctl_info.gyro_buf_sz = 0;
	}

	ctrl_hdl->ctrl.buf_info.total_size = 0;
 	ctrl_hdl->ctrl.buf_info.total_size += ctrl_hdl->ctrl.gyro_ctl_info.gyro_buf_sz;
	for (i = 0; i < CTL_SIE_CH_MAX; i++) {
		ctrl_hdl->ctrl.buf_info.total_size += ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0 + i].data_size;
	}
//	ctl_sie_dbg_trc("sie ch_0 sz %#x, ch_1 sz %#x, ch_2 sz %#x\r\n", ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].data_size, ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].data_size, ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_2].data_size);
	if (ctrl_hdl->ctrl.buf_info.total_size != 0) {
		ctrl_hdl->ctrl.buf_info.total_size += ALIGN_CEIL(sizeof(CTL_SIE_HEADER_INFO), VOS_ALIGN_BYTES);
	}
}

void ctl_sie_buf_update_out_addr(CTL_SIE_HDL *ctrl_hdl, CTL_SIE_HEAD_IDX head_idx)
{
	UINT32 addr = ctrl_hdl->ctrl.head_info[head_idx].buf_addr + ALIGN_CEIL(sizeof(CTL_SIE_HEADER_INFO), VOS_ALIGN_BYTES);
	UINT64 update_item = 0;
	CTL_SIE_SINGLE_OUT_CTRL single_out_ctl = {DISABLE, DISABLE, DISABLE};

	//set header sign
	ctl_sie_header_set(CTL_SIE_HEAD_ITEM_SIGN, ctrl_hdl, head_idx);

	//update buffer size
	if (ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].out_enable) {
		//update output address info
		ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_0] = addr;
		addr += ALIGN_CEIL(ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].data_size, VOS_ALIGN_BYTES);
		single_out_ctl.ch0_single_out_en = ENABLE;
		update_item |= 1ULL << CTL_SIE_INT_ITEM_CH0_ADDR;
		ctl_sie_header_set(CTL_SIE_HEAD_ITEM_MAIN_OUT_ADDR, ctrl_hdl, head_idx);
	} else {
		ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_0] = 0;
	}

	//ch1 ca
	if (ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].out_enable) {
		ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_1] = addr;
		addr += ALIGN_CEIL(ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].data_size, VOS_ALIGN_BYTES);
		single_out_ctl.ch1_single_out_en = ENABLE;
		update_item |= 1ULL << CTL_SIE_INT_ITEM_CH1_ADDR;
		if (ctl_sie_typecast_raw(ctrl_hdl->param.data_fmt)) {
			ctl_sie_header_set(CTL_SIE_HEAD_ITEM_CH1_ADDR, ctrl_hdl, head_idx);
		} else {
			ctl_sie_header_set(CTL_SIE_HEAD_ITEM_MAIN_OUT_ADDR, ctrl_hdl, head_idx);
		}
	}

	//ch2 la
	if (ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_2].out_enable) {
		ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_2] = addr;
		addr += ALIGN_CEIL(ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_2].data_size, VOS_ALIGN_BYTES);
		single_out_ctl.ch2_single_out_en = ENABLE;
		update_item |= 1ULL << CTL_SIE_INT_ITEM_CH2_ADDR;
		ctl_sie_header_set(CTL_SIE_HEAD_ITEM_CH2_ADDR, ctrl_hdl, head_idx);
	}

	/* gyro data */
	if (ctrl_hdl->ctrl.gyro_ctl_info.gyro_cfg.en) {
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[1] = addr;
//		addr += ctrl_hdl->ctrl.gyro_ctl_info.gyro_buf_sz;	//no next int. buf
	}

	//update single out control
	ctrl_hdl->ctrl.sin_out_ctl = single_out_ctl;
	update_item |= 1ULL << CTL_SIE_INT_ITEM_SINGLE_OUT_CTL;
	ctl_sie_hdl_update_item(ctrl_hdl->id, update_item, TRUE);
}

INT32 ctl_sie_header_create(CTL_SIE_HDL *ctrl_hdl, CTL_SIE_HEAD_IDX head_idx)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

#if defined(__KERNEL__)
	int fastboot = kdrv_builtin_is_fastboot();
#endif

	if (ctrl_hdl->ctrl.bufiocb.cb_fp == NULL) {
		ctl_sie_dbg_err("BUFIO_CB NULL\r\n");
		return CTL_SIE_E_NULL_FP;
	}

	ctl_sie_header_set(CTL_SIE_HEAD_ITEM_MAIN_OUT_IMG, ctrl_hdl, head_idx);
	ctl_sie_header_set(CTL_SIE_HEAD_ITEM_DEST_CROP, ctrl_hdl, head_idx);

	if (ctrl_hdl->flow_type == CTL_SIE_FLOW_SIG_DUPL && ctl_sie_get_fc(ctrl_hdl->id) <= ctrl_hdl->ctrl.frame_ctl_info.frame_cnt) {
		rt = CTL_SIE_E_STATE;
	} else {
		ctl_sie_header_set(CTL_SIE_HEAD_ITEM_FC, ctrl_hdl, head_idx);
		if (ctrl_hdl->ctrl.rst_fc_sts == CTL_SIE_RST_FC_DONE) {
			if (ctrl_hdl->ctrl.buf_info.total_size != 0) {
				//check buffer queue
				if (ctl_sie_buf_get_free_queue_num() == 0) {
					ctrl_hdl->ctrl.frame_ctl_info.buf_queue_full_cnt++;
					rt = CTL_SIE_E_QOVR;
				} else {
					//new buffer
					ctrl_hdl->ctrl.head_info[head_idx].buf_id = ctrl_hdl->ctrl.frame_ctl_info.frame_cnt;	//for unit layer hdr frame sync
#if defined(__KERNEL__)
					if (fastboot && !ctrl_hdl->fb_obj.normal_hd_create) {
						ctrl_hdl->fb_obj.normal_hd_create = TRUE;
#else
					if (0) {
#endif
						//skip first frame new buffer
						if (ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_0].out_enable) {
							ctrl_hdl->ctrl.sin_out_ctl.ch0_single_out_en = ENABLE;
						}
						if (ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_1].out_enable) {
							ctrl_hdl->ctrl.sin_out_ctl.ch1_single_out_en = ENABLE;
						}
						if (ctrl_hdl->ctrl.buf_info.ch_info[CTL_SIE_CH_2].out_enable) {
							ctrl_hdl->ctrl.sin_out_ctl.ch2_single_out_en = ENABLE;
						}
						ctl_sie_hdl_update_item(ctrl_hdl->id, 1ULL << CTL_SIE_INT_ITEM_SINGLE_OUT_CTL, TRUE);
					} else {
						ctl_sie_buf_io_cfg(ctrl_hdl->id, CTL_SIE_BUF_IO_NEW, ctrl_hdl->ctrl.buf_info.total_size, (UINT32)&ctrl_hdl->ctrl.head_info[head_idx]);
						if (ctrl_hdl->ctrl.head_info[head_idx].buf_addr == 0) {
							rt = CTL_SIE_E_NOMEM;
						} else {
							//upadte output address and header info
							ctl_sie_buf_update_out_addr(ctrl_hdl, head_idx);
							__ctl_sie_buf_dbg_buf_chk(ctrl_hdl, ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_0], ctrl_hdl->param.io_size_info.size_info.sie_scl_out.h,ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_0], FALSE);
							__ctl_sie_buf_dbg_buf_wp(ctrl_hdl, head_idx, TRUE);

							if (ctrl_hdl->param.out_dest == CTL_SIE_OUT_DEST_BOTH) {
								ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.resv1 = 1;	//use for direct mode get image
							} else {
								ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.resv1 = 0;
							}
						}
					}
				}
			}

			if (dbg_tab != NULL && dbg_tab->set_ts != NULL) {
				dbg_tab->set_ts(ctrl_hdl->id, CTL_SIE_DBG_TS_ADDR_RDY, ctl_sie_get_fc(ctrl_hdl->id));
			}
		}
	}

	ctl_sie_header_set(CTL_SIE_HEAD_ITEM_DIRECT_CTL, ctrl_hdl, head_idx);
	if (ctrl_hdl->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
		//force enable single out for isr timing check(VD will auto clear)
		ctrl_hdl->ctrl.sin_out_ctl.ch0_single_out_en = ENABLE;
		ctl_sie_hdl_update_item(ctrl_hdl->id, 1ULL << CTL_SIE_INT_ITEM_SINGLE_OUT_CTL, TRUE);
		//set SIE2 ring buffer address and single out enable
		if (ctrl_hdl->id == CTL_SIE_ID_2) {
			ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_0] = ctrl_hdl->ctrl.ring_buf_ctl.buf_addr;
			ctl_sie_hdl_update_item(ctrl_hdl->id, 1ULL << CTL_SIE_INT_ITEM_CH0_ADDR, TRUE);
			ctl_sie_header_set(CTL_SIE_HEAD_ITEM_MAIN_OUT_ADDR, ctrl_hdl, head_idx);
		}
	}
	return rt;
}

void ctl_sie_header_set_cur_head(CTL_SIE_HDL *ctrl_hdl)
{
	ctrl_hdl->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR] = ctrl_hdl->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT];
	memset((void *)&ctrl_hdl->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT], 0, sizeof(CTL_SIE_HEADER_INFO));
	ctl_sie_header_set(CTL_SIE_HEAD_ITEM_TS, ctrl_hdl, CTL_SIE_HEAD_IDX_CUR);
}

void ctl_sie_header_set(CTL_SIE_HEAD_ITEM set_item, CTL_SIE_HDL *ctrl_hdl, CTL_SIE_HEAD_IDX head_idx)
{
	UINT32 sen_frm_num = 0;

	switch (set_item) {
	case CTL_SIE_HEAD_ITEM_SIGN:
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.sign = MAKEFOURCC('V', 'F', 'R', 'M');
		break;

	case CTL_SIE_HEAD_ITEM_FC:
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.count = ctrl_hdl->ctrl.frame_ctl_info.frame_cnt = ctl_sie_get_fc(ctrl_hdl->id);
		break;

	case CTL_SIE_HEAD_ITEM_TS:
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.timestamp = ctl_sie_util_get_syst_timestamp();
		break;

	case CTL_SIE_HEAD_ITEM_MAIN_OUT_IMG:
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.size.w = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.w;
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.size.h = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.h;
		if (ctl_sie_typecast_raw(ctrl_hdl->param.data_fmt)) {
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.pw[VDO_PINDEX_RAW] = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.w;
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.ph[VDO_PINDEX_RAW] = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.h;
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.loff[VDO_PINDEX_RAW] = ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_0];
			if (ctrl_hdl->ctrl.total_frame_num == 0) {
				ctrl_hdl->ctrl.total_frame_num = 1;
			}
			if (ctrl_hdl->param.encode_en) {
				switch (ctrl_hdl->param.enc_rate) {
				case CTL_SIE_ENC_50:
					ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[7] = 50;
					break;

				case CTL_SIE_ENC_58:
					ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[7] = 58;
					break;

				case CTL_SIE_ENC_66:
				default:
					ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[7] = 66;
					break;
				}
			}
		} else {    //ccir
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.pw[VDO_PINDEX_Y] = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.w;
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.ph[VDO_PINDEX_Y] = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.h;
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.loff[VDO_PINDEX_Y] = ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_0];
			switch (ctrl_hdl->param.data_fmt) {
			case CTL_SIE_YUV_422_SPT:
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.pw[VDO_PINDEX_UV] = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.w;
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.ph[VDO_PINDEX_UV] = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.h;
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.loff[VDO_PINDEX_UV] = ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_1];
				break;

			case CTL_SIE_YUV_420_SPT:
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.pw[VDO_PINDEX_UV] = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.w;
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.ph[VDO_PINDEX_UV] = ctrl_hdl->param.io_size_info.size_info.sie_scl_out.h/2;
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.loff[VDO_PINDEX_UV] = ctrl_hdl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_1];
				break;

			case CTL_SIE_YUV_422_NOSPT:
			default:
				break;
			}
		}
		__ctl_sie_buf_typecast_pxfmt(ctrl_hdl, head_idx);
		break;

	case CTL_SIE_HEAD_ITEM_MAIN_OUT_ADDR:
		if (ctl_sie_typecast_raw(ctrl_hdl->param.data_fmt)) {
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.addr[VDO_PINDEX_RAW] = ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_0];
		} else {    //ccir
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.addr[VDO_PINDEX_Y] = ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_0];
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.addr[VDO_PINDEX_UV] = ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_1];
		}
		break;

	case CTL_SIE_HEAD_ITEM_CH1_ADDR:
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.addr[VDO_PINDEX_RAW2] = ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_1];
		break;

	case CTL_SIE_HEAD_ITEM_CH2_ADDR:
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.addr[VDO_PINDEX_RAW3] = ctrl_hdl->ctrl.out_ch_addr[CTL_SIE_CH_2];
		break;

	case CTL_SIE_HEAD_ITEM_DIRECT_CTL:
		if (ctrl_hdl->param.out_dest == CTL_SIE_OUT_DEST_DIRECT || ctrl_hdl->param.out_dest == CTL_SIE_OUT_DEST_BOTH) {
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[2] = ctrl_hdl->ctrl.head_info[head_idx].buf_id;		//public buffer id
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[3] = ctrl_hdl->ctrl.head_info[head_idx].buf_addr;	//public buffer addr
			ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[4] = ctl_sie_vdofrm_reserved_mask_sieid(ctrl_hdl->id, ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[4]);
			if (ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.resv1 == 1) {
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[5] = 0;			//ring buffer line number
			} else {
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[5] = ctrl_hdl->ctrl.ring_buf_ctl.buf_len;			//ring buffer line number
			}
		}

		if (ctrl_hdl->flow_type != CTL_SIE_FLOW_PATGEN) {
			if (ctl_sie_get_sen_cfg(ctrl_hdl->id, CTL_SEN_CFGID_GET_FRAME_NUMBER, (void *)&sen_frm_num) == CTL_SIE_E_OK) {
				ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[4] = ctl_sie_vdofrm_reserved_mask_senfc(sen_frm_num, ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[4]);
			}
		}
		break;

	case CTL_SIE_HEAD_ITEM_DEST_CROP:
		// dest. crop RECT
		//reserved[5]: crop start, construction of (x << 16 | y)
		//reserved[6]: crop size, construction of (w << 16 | h)
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[5] = CTL_SIE_BUF_CONV2_SIZE(ctrl_hdl->param.io_size_info.size_info.dest_crp.x, ctrl_hdl->param.io_size_info.size_info.dest_crp.y);
		ctrl_hdl->ctrl.head_info[head_idx].vdo_frm.reserved[6] = CTL_SIE_BUF_CONV2_SIZE(ctrl_hdl->param.io_size_info.size_info.dest_crp.w, ctrl_hdl->param.io_size_info.size_info.dest_crp.h);
		break;

	default:
		ctl_sie_dbg_err("buf item err %d\r\n", (int)(set_item));
		break;
	}
}

