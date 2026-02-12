#include "kwrap/cpu.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "ctl_ipp_int.h"
#include "ctl_ipp_buf_int.h"
#include "ctl_ipp_id_int.h"
#include "ctl_ipp_isp_int.h"
#include "ipp_debug_int.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

// for buf task racing issue
static VK_DEFINE_SPINLOCK(my_lock2);
#define loc_cpu2(flags)   vk_spin_lock_irqsave(&my_lock2, flags)
#define unl_cpu2(flags)   vk_spin_unlock_irqrestore(&my_lock2, flags)

static VK_DEFINE_SPINLOCK(my_3dnr_ref_lock);
#define loc_cpu_3dnr_ref(flags)   vk_spin_lock_irqsave(&my_3dnr_ref_lock, flags)
#define unl_cpu_3dnr_ref(flags)   vk_spin_unlock_irqrestore(&my_3dnr_ref_lock, flags)

#if 0
#endif
#define CTL_IPP_BUF_MSG_Q_NUM   32
static CTL_IPP_LIST_HEAD free_msg_list_head;
static CTL_IPP_LIST_HEAD proc_msg_list_head;
static UINT32 ctl_ipp_buf_msg_queue_free;
static CTL_IPP_BUF_MSG_EVENT ctl_ipp_buf_msg_pool[CTL_IPP_BUF_MSG_Q_NUM];
static UINT32 ctl_ipp_buf_snd_push_cnt;
static UINT32 ctl_ipp_buf_snd_rls_cnt;
static UINT32 ctl_ipp_buf_snd_fail_cnt;
static UINT32 ctl_ipp_buf_rcv_push_cnt;
static UINT32 ctl_ipp_buf_rcv_rls_cnt;
static UINT32 ctl_ipp_buf_rcv_flush_cnt;

_INLINE void ctl_ipp_buf_hdl_count_update(CTL_IPP_INFO_LIST_ITEM *info, BOOL is_evt_end)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_HANDLE *p_hdl;

	p_hdl = (CTL_IPP_HANDLE *)info->owner;
	if (p_hdl != NULL) {
		loc_cpu2(loc_cpu_flg);
		if (is_evt_end) {
			if (p_hdl->buf_push_evt_cnt == 0) {
				CTL_IPP_DBG_ERR("ctl_hdl(0x%.8x) buf_push_evt_cnt already 0\r\n", (unsigned int)p_hdl);
			} else {
				p_hdl->buf_push_evt_cnt -= 1;
			}
		} else {
			p_hdl->buf_push_evt_cnt += 1;
		}
		unl_cpu2(loc_cpu_flg);
	} else {
		CTL_IPP_DBG_WRN("null handle\r\n");
	}
}

_INLINE void ctl_ipp_buf_tag_set(CTL_IPP_OUT_BUF_INFO *p_buf)
{
	UINT32 buf_addr = p_buf->buf_addr;
	UINT32 yuv_size;

	if (buf_addr != 0) {
		yuv_size = ctl_ipp_util_yuvsize(p_buf->vdo_frm.pxlfmt, p_buf->vdo_frm.loff[0], p_buf->vdo_frm.size.h);
		yuv_size = ALIGN_CEIL(yuv_size, VOS_ALIGN_BYTES);
		*((UINT32 *)buf_addr) = CTL_IPP_BUF_CHK_TAG;
		vos_cpu_dcache_sync(buf_addr, VOS_ALIGN_BYTES, VOS_DMA_BIDIRECTIONAL);

		buf_addr = buf_addr + yuv_size - VOS_ALIGN_BYTES;
		*((UINT32 *)buf_addr) = CTL_IPP_BUF_CHK_TAG;
		vos_cpu_dcache_sync(buf_addr, VOS_ALIGN_BYTES, VOS_DMA_BIDIRECTIONAL);
	}
}

_INLINE BOOL ctl_ipp_buf_tag_chk_buf_is_rdy(UINT32 hdl, CTL_IPP_OUT_BUF_INFO *p_buf)
{
	/* if tag exist, means engine is not output to dram
		return TRUE when engine output overwrite the tag
	*/
	UINT32 buf_addr, buf_addr_end;
	UINT32 st_tag, ed_tag;
	UINT32 yuv_size;

	if (ctl_ipp_dbg_dma_wp_get_enable()) {
		return TRUE;
	}

	yuv_size = ctl_ipp_util_yuvsize(p_buf->vdo_frm.pxlfmt, p_buf->vdo_frm.loff[0], p_buf->vdo_frm.size.h);
	yuv_size = ALIGN_CEIL(yuv_size, VOS_ALIGN_BYTES);
	buf_addr = p_buf->buf_addr;
	buf_addr_end = buf_addr + yuv_size - VOS_ALIGN_BYTES;
	if (buf_addr != 0) {
		vos_cpu_dcache_sync(buf_addr, VOS_ALIGN_BYTES, VOS_DMA_FROM_DEVICE);
		vos_cpu_dcache_sync(buf_addr_end, VOS_ALIGN_BYTES, VOS_DMA_FROM_DEVICE);

		st_tag = *((UINT32 *)buf_addr);
		ed_tag = *((UINT32 *)buf_addr_end);
		if (st_tag != CTL_IPP_BUF_CHK_TAG && ed_tag != CTL_IPP_BUF_CHK_TAG) {
			return TRUE;
		} else {
			CTL_IPP_DBG_IND("ipp buf(0x%.8x) size(0x%.8x) not output, st tag 0x%.8x, ed tag 0x%.8x\r\n",
				(unsigned int)buf_addr, (unsigned int)p_buf->buf_size, (unsigned int)st_tag, (unsigned int)ed_tag);
		}
	}
	return FALSE;
}

_INLINE BOOL ctl_ipp_buf_upd_3dnr_ref_sts_push(CTL_IPP_INFO_LIST_ITEM *p_info)
{
	unsigned long loc_flg;
	BOOL allow_push;

	loc_cpu_3dnr_ref(loc_flg);
	if (p_info->tplnr_ref_buf_sts & CTL_IPP_TPLNR_REF_BUF_WAIT_PUSH) {
		/* push buffer */
		p_info->tplnr_ref_buf_sts &= ~(CTL_IPP_TPLNR_REF_BUF_WAIT_PUSH);
		allow_push = TRUE;
	} else {
		/* skip push */
		allow_push = FALSE;
	}
	unl_cpu_3dnr_ref(loc_flg);

	return allow_push;
}

_INLINE BOOL ctl_ipp_buf_upd_3dnr_ref_sts_rls(CTL_IPP_INFO_LIST_ITEM *p_info)
{
	unsigned long loc_flg;
	BOOL allow_rls;

	loc_cpu_3dnr_ref(loc_flg);
	if (p_info->tplnr_ref_buf_sts & CTL_IPP_TPLNR_REF_BUF_WAIT_RELEASE) {
		/* push buffer */
		p_info->tplnr_ref_buf_sts &= ~(CTL_IPP_TPLNR_REF_BUF_WAIT_RELEASE);
		allow_rls = TRUE;
	} else {
		/* skip push */
		allow_rls = FALSE;
	}
	unl_cpu_3dnr_ref(loc_flg);

	return allow_rls;
}

#if 0
#endif

static void ctl_ipp_buf_msg_snd_count_update(UINT32 cmd)
{
	unsigned long loc_cpu_flg;

	loc_cpu2(loc_cpu_flg);
	if (cmd == CTL_IPP_BUF_MSG_PUSH) {
		ctl_ipp_buf_snd_push_cnt += 1;
	} else if (cmd == CTL_IPP_BUF_MSG_RELEASE) {
		ctl_ipp_buf_snd_rls_cnt += 1;
	}
	unl_cpu2(loc_cpu_flg);
}

static void ctl_ipp_buf_msg_rcv_count_update(UINT32 cmd)
{
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	if (cmd == CTL_IPP_BUF_MSG_PUSH) {
		ctl_ipp_buf_rcv_push_cnt += 1;
	} else if (cmd == CTL_IPP_BUF_MSG_RELEASE) {
		ctl_ipp_buf_rcv_rls_cnt += 1;
	}
	unl_cpu(loc_cpu_flg);
}

static CTL_IPP_BUF_MSG_EVENT *ctl_ipp_buf_msg_pool_get_msg(UINT32 idx)
{
	if (idx >= CTL_IPP_BUF_MSG_Q_NUM) {
		CTL_IPP_DBG_IND("msg idx = %d overflow\r\n", (int)(idx));
		return NULL;
	}
	return &ctl_ipp_buf_msg_pool[idx];
}

static void ctl_ipp_buf_msg_reset(CTL_IPP_BUF_MSG_EVENT *p_event)
{
	if (p_event) {
		p_event->cmd = 0;
		p_event->param[0] = 0;
		p_event->param[1] = 0;
		p_event->param[2] = 0;
		p_event->rev[0] = CTL_IPP_BUF_MSG_STS_FREE;
	}
}

static CTL_IPP_BUF_MSG_EVENT *ctl_ipp_buf_msg_lock(void)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_BUF_MSG_EVENT *p_event = NULL;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&free_msg_list_head)) {
		p_event = vos_list_entry(free_msg_list_head.next, CTL_IPP_BUF_MSG_EVENT, list);
		vos_list_del(&p_event->list);

		p_event->rev[0] |= CTL_IPP_BUF_MSG_STS_LOCK;
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "[CTL_IPP_BUF]Get free queue idx = %d\r\n", (int)(p_event->rev[1]));
		ctl_ipp_buf_msg_queue_free -= 1;
		unl_cpu(loc_cpu_flg);
		return p_event;
	}
	ctl_ipp_buf_snd_fail_cnt += 1;
	unl_cpu(loc_cpu_flg);

	return p_event;
}

static void ctl_ipp_buf_msg_unlock(CTL_IPP_BUF_MSG_EVENT *p_event)
{
	unsigned long loc_cpu_flg;

	if ((p_event->rev[0] & CTL_IPP_BUF_MSG_STS_LOCK) != CTL_IPP_BUF_MSG_STS_LOCK) {
		CTL_IPP_DBG_IND("event status error (%d)\r\n", (int)(p_event->rev[0]));
	} else {

		ctl_ipp_buf_msg_reset(p_event);

		loc_cpu(loc_cpu_flg);
		vos_list_add_tail(&p_event->list, &free_msg_list_head);
		ctl_ipp_buf_msg_queue_free += 1;
		unl_cpu(loc_cpu_flg);
	}
}

UINT32 ctl_ipp_buf_get_free_queue_num(void)
{
	unsigned long loc_cpu_flg;
	UINT32 rt;

	loc_cpu(loc_cpu_flg);
	rt = ctl_ipp_buf_msg_queue_free;
	unl_cpu(loc_cpu_flg);

	return rt;
}

ER ctl_ipp_buf_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_BUF_MSG_EVENT *p_event;

	p_event = ctl_ipp_buf_msg_lock();

	if (p_event == NULL) {
		CTL_IPP_DBG_IND("send message fail\r\n");
		return E_SYS;
	}

	p_event->cmd = cmd;
	p_event->param[0] = p1;
	p_event->param[1] = p2;
	p_event->param[2] = p3;
	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "[CTL_IPP_BUF]S Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(p1), (unsigned int)(p2), (unsigned int)(p3));

	/* add to proc list & trig proc task */
	loc_cpu(loc_cpu_flg);

	if (cmd == CTL_IPP_BUF_MSG_PUSH_LOWDLY) {
		vos_list_add(&p_event->list, &proc_msg_list_head);
	} else {
		vos_list_add_tail(&p_event->list, &proc_msg_list_head);
	}

	ctl_ipp_buf_msg_snd_count_update(cmd);

	if (cmd == CTL_IPP_BUF_MSG_PUSH) { // update global variable in critical section with list_add() together, to prevent race condition (ex. NA51102-1765, IVOT_N11000-932)
		ctl_ipp_buf_hdl_count_update((CTL_IPP_INFO_LIST_ITEM *)p2, FALSE);
	}

	unl_cpu(loc_cpu_flg);

	vos_flag_iset(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_QUEUE_PROC);
	return E_OK;
}

ER ctl_ipp_buf_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_BUF_MSG_EVENT *p_event;
	FLGPTN flag;
	UINT32 vos_list_empty_flag;

	while (1) {
		/* check empty or not */
		loc_cpu(loc_cpu_flg);
		vos_list_empty_flag = vos_list_empty(&proc_msg_list_head);
		if (!vos_list_empty_flag) {
			p_event = vos_list_entry(proc_msg_list_head.next, CTL_IPP_BUF_MSG_EVENT, list);
			vos_list_del(&p_event->list);
		}
		unl_cpu(loc_cpu_flg);

		/* is empty enter wait mode */
		if (vos_list_empty_flag) {
			vos_flag_wait(&flag, g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_QUEUE_PROC, TWF_CLR);
		} else {
			break;
		}
	}

	ctl_ipp_buf_msg_rcv_count_update(p_event->cmd);

	*cmd = p_event->cmd;
	*p1 = p_event->param[0];
	*p2 = p_event->param[1];
	*p3 = p_event->param[2];
	ctl_ipp_buf_msg_unlock(p_event);

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "[CTL_IPP_BUF]R Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(*cmd), (unsigned int)(*p1), (unsigned int)(*p2), (unsigned int)(*p3));
	return E_OK;
}

ER ctl_ipp_buf_msg_flush(void)
{
	UINT32 cmd, param[3];
	ER err = E_OK;

	while (!vos_list_empty(&proc_msg_list_head)) {
		err = ctl_ipp_buf_msg_rcv(&cmd, &param[0], &param[1], &param[2]);
		if (err != E_OK) {
			CTL_IPP_DBG_IND("flush fail (%d)\r\n", (int)(err));
		}

		/* Unlock buffer when flush */
		if (cmd != CTL_IPP_BUF_MSG_IGNORE) {
			ctl_ipp_buf_rcv_flush_cnt += 1;
			ctl_ipp_buf_release(param[0], param[1], param[2], CTL_IPP_E_FLUSH);
		}
	}

	return err;
}

ER ctl_ipp_buf_erase_queue(UINT32 handle)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *p_list_end;
	CTL_IPP_BUF_MSG_EVENT *p_event;

	if (!vos_list_empty(&proc_msg_list_head)) {
		/* keep last node, do not process over last node to prevent new node add in isr cause racing */
		loc_cpu(loc_cpu_flg);
		p_list_end = proc_msg_list_head.prev;
		unl_cpu(loc_cpu_flg);

		vos_list_for_each(p_list, &proc_msg_list_head) {
			p_event = vos_list_entry(p_list, CTL_IPP_BUF_MSG_EVENT, list);

			if (p_event->cmd != CTL_IPP_BUF_MSG_IGNORE) {
				CTL_IPP_INFO_LIST_ITEM *ctrl_info;

				ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)p_event->param[1];
				if (ctrl_info != NULL) {
					if (ctrl_info->owner == handle) {
						ctl_ipp_buf_release(p_event->param[0], p_event->param[1], p_event->param[2], CTL_IPP_E_FLUSH);
						ctl_ipp_buf_rcv_flush_cnt += 1;
						p_event->cmd = CTL_IPP_BUF_MSG_IGNORE;
					}
				}
			}

			if (p_list == p_list_end) {
				break;
			}
		}
	}

	return E_OK;
}

ER ctl_ipp_buf_erase_path_in_queue(UINT32 handle, UINT32 pid)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *p_list_end;
	CTL_IPP_BUF_MSG_EVENT *p_event;

	if (!vos_list_empty(&proc_msg_list_head)) {
		/* keep last node, do not process over last node to prevent new node add in isr cause racing */
		loc_cpu(loc_cpu_flg);
		p_list_end = proc_msg_list_head.prev;
		unl_cpu(loc_cpu_flg);

		vos_list_for_each(p_list, &proc_msg_list_head) {
			p_event = vos_list_entry(p_list, CTL_IPP_BUF_MSG_EVENT, list);

			if (p_event->cmd != CTL_IPP_BUF_MSG_IGNORE) {
				CTL_IPP_INFO_LIST_ITEM *ctrl_info;

				ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)p_event->param[1];
				if ((ctrl_info != NULL) && (ctrl_info->owner == handle)) {
					ctl_ipp_buf_release_path(p_event->param[0], p_event->param[1], p_event->param[2], pid, CTL_IPP_E_FLUSH);
				}
			}

			if (p_list == p_list_end) {
				break;
			}
		}
	}

	return E_OK;
}

ER ctl_ipp_buf_msg_reset_queue(void)
{
	UINT32 i;
	CTL_IPP_BUF_MSG_EVENT *free_event;

	/* init free & proc list head */
	VOS_INIT_LIST_HEAD(&free_msg_list_head);
	VOS_INIT_LIST_HEAD(&proc_msg_list_head);

	for (i = 0; i < CTL_IPP_BUF_MSG_Q_NUM; i++) {
		free_event = ctl_ipp_buf_msg_pool_get_msg(i);
		ctl_ipp_buf_msg_reset(free_event);
		free_event->rev[1] = i;
		vos_list_add_tail(&free_event->list, &free_msg_list_head);
	}
	ctl_ipp_buf_msg_queue_free = CTL_IPP_BUF_MSG_Q_NUM;
	ctl_ipp_buf_snd_push_cnt = 0;
	ctl_ipp_buf_snd_rls_cnt = 0;
	ctl_ipp_buf_snd_fail_cnt = 0;
	ctl_ipp_buf_rcv_push_cnt = 0;
	ctl_ipp_buf_rcv_rls_cnt = 0;
	ctl_ipp_buf_rcv_flush_cnt = 0;
	return E_OK;
}

void ctl_ipp_buf_msg_dump(int (*dump)(const char *fmt, ...))
{
	FLGPTN flg;

	flg = vos_flag_chk(g_ctl_ipp_buf_flg_id, FLGPTN_BIT_ALL);

	dump("\r\n---- ctl_ipp_buf task information ----\r\n");
	dump("%10s  %16s%16s%16s%16s%16s%16s\r\n", "flg", "snd_push_evt", "snd_rls_evt", "snd_failed", "rcv_push_evt", "rcv_rls_evt", "rcv_flush_evt");
	dump("0x%.8x  %16d%16d%16d%16d%16d%16d\r\n", (unsigned int)flg,
		(int)ctl_ipp_buf_snd_push_cnt, (int)ctl_ipp_buf_snd_rls_cnt, (int)ctl_ipp_buf_snd_fail_cnt, (int)ctl_ipp_buf_rcv_push_cnt, (int)ctl_ipp_buf_rcv_rls_cnt, (int)ctl_ipp_buf_rcv_flush_cnt);
}

#if 0
#endif
/*
	global variables
*/
static BOOL ctl_ipp_buf_task_opened = FALSE;
static UINT32 ctl_ipp_buf_task_pause_count;

static void ctl_ipp_buf_set_nvx1info(VDO_FRAME *dst, CTL_IPP_BASEINFO *p_base)
{
#if 0
	VDO_NVX1_INFO *p_nvx1_info;
	UINT32 ipe_ovlp;
	UINT32 ime_ovlp;
	UINT32 strp[4];
	UINT32 i;

	p_nvx1_info = (VDO_NVX1_INFO *)&dst->reserved[0];
	p_nvx1_info->sideinfo_addr = p_base->ime_ctrl.enc_si_addr;
	p_nvx1_info->ktab0 = p_base->ime_ctrl.enc_in_k_tbl[0];
	p_nvx1_info->ktab1 = p_base->ime_ctrl.enc_in_k_tbl[1];
	p_nvx1_info->ktab2 = p_base->ime_ctrl.enc_in_k_tbl[2];

	/* check stripe number, codec support max 4stripe */
	if (p_base->dce_ctrl.strp_h_arr[4] != 0) {
		CTL_IPP_DBG_WRN("ipp stripe number > 4, codec only support max 4 stripe yuv-compress\r\n");
	}

	/* nvx1info is for yuv-compress, overlap is fixed */
	ipe_ovlp = 16;
	ime_ovlp = 256;
	p_nvx1_info->strp_num = 0;
	for (i = 0; i < 4; i++) {
		if (p_base->dce_ctrl.strp_h_arr[i] == 0) {
			/* find stripe number */
			if (p_nvx1_info->strp_num == 0) {
				p_nvx1_info->strp_num = i;
			}
			strp[i] = 0;
		} else {
			/* check first or last strp */
			if (i == 0 || p_base->dce_ctrl.strp_h_arr[i+1] == 0) {
				strp[i] = p_base->dce_ctrl.strp_h_arr[i] - (ipe_ovlp / 2) - (ime_ovlp / 2);
			} else {
				strp[i] = p_base->dce_ctrl.strp_h_arr[i] - ipe_ovlp - ime_ovlp;
			}
		}
	}
	p_nvx1_info->strp_size_01 = (strp[1] << 16) | strp[0];
	p_nvx1_info->strp_size_23 = (strp[3] << 16) | strp[2];

	CTL_IPP_DBG_TRC("%s: si_addr(0x%.8x), katb(0x%.8x, 0x%.8x, 0x%.8x), strp_num %d, strp_size(0x%.8x, 0x%.8x)\r\n",
		__func__, p_nvx1_info->sideinfo_addr, p_nvx1_info->ktab0, p_nvx1_info->ktab1, p_nvx1_info->ktab2,
		p_nvx1_info->strp_num, p_nvx1_info->strp_size_01, p_nvx1_info->strp_size_23);
#endif
}

static void ctl_ipp_buf_set_lowdlyinfo(VDO_FRAME *dst, CTL_IPP_BASEINFO *p_base)
{
	VDO_LOW_DLY_INFO *p_lowdly;
	UINT32 i;

	p_lowdly = (VDO_LOW_DLY_INFO *)&dst->reserved[0];

	i = 0;
	while (p_base->dce_ctrl.strp_h_arr[i] != 0 && i < CTL_IPP_DCE_STRP_NUM_MAX) {
		if (i < 3) {
			p_lowdly->strp_size[i] = p_base->dce_ctrl.strp_h_arr[i];
		}
		i += 1;
	}
	p_lowdly->strp_num = i;

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "lowdly, strp_num %d, strp_size %d %d %d\r\n", (int)p_lowdly->strp_num,
		(int)p_lowdly->strp_size[0], (int)p_lowdly->strp_size[1], (int)p_lowdly->strp_size[2]);
}

static void ctl_ipp_buf_set_mdinfo(VDO_FRAME *dst, CTL_IPP_BASEINFO *p_base)
{
	VDO_MD_INFO *p_md;

	if (dst->p_next != NULL) {
		p_md = (VDO_MD_INFO *)((UINT32)dst->p_next + sizeof(VDO_METADATA));
        memset(p_md,0,sizeof(VDO_MD_INFO));
		p_md->src_img_size.w = p_base->ime_ctrl.in_size.w;
		p_md->src_img_size.h = p_base->ime_ctrl.in_size.h;
		p_md->md_size.w = ctl_ipp_util_3dnr_ms_roi_width(p_md->src_img_size.w);
		p_md->md_size.h = ctl_ipp_util_3dnr_ms_roi_height(p_md->src_img_size.h);
		p_md->md_lofs = ctl_ipp_util_3dnr_ms_roi_lofs(p_md->src_img_size.w);

		/* check if 3dnr disable -> no md output */
		if (p_base->ime_ctrl.tplnr_enable == DISABLE ||
			p_base->ime_ctrl.tplnr_out_ms_roi_enable == DISABLE ||
			p_base->ime_ctrl.tplnr_in_ref_addr[0] == 0) {
			dst->p_next = NULL;
		}
	}
}

void ctl_ipp_buf_fp_wrapper(UINT32 hdl, UINT32 bufio_fp_addr, CTL_IPP_BUF_IO_CFG cfg, CTL_IPP_OUT_BUF_INFO *p_buf)
{
	CTL_IPP_HANDLE *p_hdl;
	IPP_EVENT_FP bufio_fp;
	UINT64 ts_start, ts_end;

	/* bufio callback */
	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	if (p_buf == NULL) {
		CTL_IPP_DBG_WRN("p_buf NULL\r\n");
		return ;
	}

	/* check io_start/stop status */
	p_hdl = (CTL_IPP_HANDLE *)hdl;
	if (p_hdl == NULL) {
		CTL_IPP_DBG_WRN("IPP HANDLE NULL\r\n");
		return ;
	}

	/* buf_io_started should be protect by handle lock
		io_start only in set_out_path
		io_stop only in set_flush
	*/
	if (cfg == CTL_IPP_BUF_IO_START) {
		if (p_hdl->buf_io_started[p_buf->pid] != 0) {
			CTL_IPP_DBG_WRN("handle 0x%.8x, path_%d already started\r\n", (unsigned int)hdl, (int)p_buf->pid);
			return ;
		}
	} else if (cfg == CTL_IPP_BUF_IO_STOP) {
		if (p_hdl->buf_io_started[p_buf->pid] != 1) {
			CTL_IPP_DBG_WRN("handle 0x%.8x, path_%d already stopped\r\n", (unsigned int)hdl, (int)p_buf->pid);
			return ;
		}
		p_hdl->buf_io_started[p_buf->pid] = 0;
	} else {
		if (p_hdl->buf_io_started[p_buf->pid] != 1) {
			CTL_IPP_DBG_WRN("handle 0x%.8x, path_%d, op %d is not started\r\n", (unsigned int)hdl, (int)p_buf->pid, (int)cfg);
		}
	}

	/* debug save yuv */
	if (cfg == CTL_IPP_BUF_IO_PUSH) {
		ctl_ipp_dbg_saveyuv(hdl, p_buf);
	}

	/* buf io callback */
	ts_start = ctl_ipp_util_get_syst_counter();
	bufio_fp(cfg, (void *)p_buf, NULL);
	ts_end = ctl_ipp_util_get_syst_counter();

	/* update io_start/stop status for direct mode check status */
	if (cfg == CTL_IPP_BUF_IO_START) {
		p_hdl->buf_io_started[p_buf->pid] = 1;
	} else if (cfg == CTL_IPP_BUF_IO_STOP) {
		p_hdl->buf_io_started[p_buf->pid] = 0;
	}

	/* debug information */
	ctl_ipp_dbg_outbuf_log_set(hdl, cfg, p_buf, ts_start, ts_end);

	/* update lock count */
	if (cfg == CTL_IPP_BUF_IO_NEW) {
		if (p_buf->buf_addr != 0) {
			p_buf->lock_cnt = 1;
		}
	} else if (cfg == CTL_IPP_BUF_IO_LOCK) {
		p_buf->lock_cnt += 1;
	} else if (cfg == CTL_IPP_BUF_IO_PUSH || cfg == CTL_IPP_BUF_IO_UNLOCK) {
		if (p_buf->lock_cnt == 0) {
			CTL_IPP_DBG_WRN("buf 0x%.8x, lock_cnt already 0, illegal op %d\r\n", (unsigned int)p_buf->buf_addr, (int)cfg);
		} else {
			p_buf->lock_cnt -= 1;
		}
	}
}

void ctl_ipp_buf_3dnr_refout_handle(UINT32 bufio_fp_addr, CTL_IPP_INFO_LIST_ITEM *p_prv_info, CTL_IPP_INFO_LIST_ITEM *p_cur_info)
{
	/* skip this error handle, already drop frame when new buffer failed */
	return ;

#if 0
	unsigned long loc_flg;
	UINT32 cur_ref_pi, prv_ref_pi;
	VDO_FRAME *p_cur_in_frm;
	CTL_IPP_HANDLE *p_hdl;
	CTL_IPP_OUT_BUF_INFO *p_cur_buf;
	CTL_IPP_OUT_BUF_INFO *p_prv_buf;
	CTL_IPP_OUT_BUF_INFO tmp_buf;
	IPP_EVENT_FP bufio_fp;
	BOOL keep_prv_ref;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	p_hdl = (CTL_IPP_HANDLE *)p_cur_info->owner;
	if (p_hdl == NULL) {
		CTL_IPP_DBG_WRN("IPP HANDLE NULL\r\n");
		return ;
	}

	memset((void *)&tmp_buf, 0, sizeof(CTL_IPP_OUT_BUF_INFO));
	keep_prv_ref = FALSE;

	cur_ref_pi = p_cur_info->info.ime_ctrl.tplnr_in_ref_path;
	p_cur_buf = &p_cur_info->buf_info[cur_ref_pi];

	prv_ref_pi = p_prv_info->info.ime_ctrl.tplnr_in_ref_path;
	p_prv_buf = &p_prv_info->buf_info[prv_ref_pi];

	/* check previous buffer valid
		1. buf addr
		2. dma enable
		3. buf tag check
	*/
	if ((p_prv_buf->buf_addr == 0) ||
		(p_prv_info->info.ime_ctrl.out_img[prv_ref_pi].dma_enable != ENABLE) ||
		(ctl_ipp_buf_tag_chk_buf_is_rdy(p_prv_info->owner, p_prv_buf) != TRUE)) {

		if (p_cur_buf->buf_addr == 0) {
			CTL_IPP_DBG_WRN("prv and cur both invalud %d, prv 0x%.8x, %d %d\r\n", p_hdl->proc_end_cnt,
				p_prv_buf->buf_addr, p_prv_info->info.ime_ctrl.out_img[prv_ref_pi].dma_enable, ctl_ipp_buf_tag_chk_buf_is_rdy(p_prv_info->owner, p_prv_buf));
		}
		return ;
	}

	loc_cpu_3dnr_ref(loc_flg);
	if (p_cur_buf->buf_addr == 0) {
		/* new buffer failed */
		memcpy((void *)p_cur_buf, (void *)p_prv_buf, sizeof(CTL_IPP_OUT_BUF_INFO));
		p_cur_info->info.ime_ctrl.out_img[cur_ref_pi] = p_prv_info->info.ime_ctrl.out_img[prv_ref_pi];
		p_cur_info->tplnr_ref_buf_sts = CTL_IPP_TPLNR_REF_BUF_WAIT_RELEASE;
		keep_prv_ref = TRUE;
	} else {
		/* new buffer success, but no dram output
			need to release buffer first
			only d2d mode can handle this situation
			do nothing in direct mode
		*/
		if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
			/* do nothing */
		} else {
			if ((p_cur_info->info.ime_ctrl.out_img[cur_ref_pi].dma_enable != ENABLE) ||
				(ctl_ipp_buf_tag_chk_buf_is_rdy(p_cur_info->owner, p_cur_buf) != TRUE)) {
				memcpy((void *)&tmp_buf, (void *)p_cur_buf, sizeof(CTL_IPP_OUT_BUF_INFO));
				memcpy((void *)p_cur_buf, (void *)p_prv_buf, sizeof(CTL_IPP_OUT_BUF_INFO));

				p_cur_info->info.ime_ctrl.out_img[cur_ref_pi] = p_prv_info->info.ime_ctrl.out_img[prv_ref_pi];
				p_cur_info->tplnr_ref_buf_sts = CTL_IPP_TPLNR_REF_BUF_WAIT_RELEASE;
				keep_prv_ref = TRUE;
			}
		}
	}
	unl_cpu_3dnr_ref(loc_flg);

	/* release buffer if new success but no output */
	while (tmp_buf.lock_cnt != 0) {
		/* dma disable when buf exist --> illegal parameters cause kdf to disable output */
		if (p_cur_info->info.ime_ctrl.out_img[cur_ref_pi].dma_enable == DISABLE) {
			p_cur_buf->err_msg = CTL_IPP_E_PAR;
		}
		ctl_ipp_buf_fp_wrapper(p_cur_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_UNLOCK, &tmp_buf);
	}


	//p_hdl = (CTL_IPP_HANDLE *)p_cur_info->owner;
	if (keep_prv_ref) {
		UINT32 frm_diff;

		/* reset lock cnt and lock for cur buffer */
		p_cur_buf->lock_cnt = 0;
		ctl_ipp_buf_fp_wrapper(p_cur_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_LOCK, p_cur_buf);

		/* update debug info */
		p_hdl->last_3dnr_ref_shift_frm = p_hdl->proc_end_cnt;
		p_cur_in_frm = (VDO_FRAME *)p_cur_info->input_header_addr;
		if (p_cur_in_frm != NULL) {
			frm_diff = p_cur_in_frm->count - p_cur_buf->vdo_frm.count;
		} else {
			if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
				frm_diff = p_cur_info->input_buf_id - p_cur_buf->vdo_frm.count;
			} else {
				frm_diff = -1;
			}
		}
		if (p_hdl->_3dnr_ref_shift_cnt % 60 == 5) {
			CTL_IPP_DBG_DUMP("%s keep using previous 3dnr reference image at frm %d(0x%.8x), ref_image_frm_diff(%d)\r\n",
				p_hdl->name, (int)p_hdl->proc_end_cnt, (unsigned int)p_cur_buf->buf_addr, (int)(frm_diff));
		} else {
			CTL_IPP_DBG_WRN("%s keep using previous 3dnr reference image at frm %d(0x%.8x), ref_image_frm_diff(%d)\r\n",
				p_hdl->name, (int)p_hdl->proc_end_cnt, (unsigned int)p_cur_buf->buf_addr, (int)(frm_diff));
		}

		p_hdl->_3dnr_ref_shift_cnt += 1;
		if (frm_diff > p_hdl->max_3dnr_ref_shift_diff) {
			p_hdl->max_3dnr_ref_shift_diff = frm_diff;
		}
	}
#endif
}

void ctl_ipp_buf_push(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 rev)
{
	unsigned long loc_flg;
	UINT32 sort_pid[CTL_IPP_OUT_PATH_ID_MAX];
	UINT32 i, pid;
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_OUT_BUF_INFO *buf_info;
	CTL_IPP_HANDLE *p_hdl;
	BOOL buf_ready;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;
	p_hdl = (CTL_IPP_HANDLE *)ctrl_info->owner;

	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	/* get push sequence */
	vk_spin_lock_irqsave(&p_hdl->spinlock, loc_flg);
    for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		sort_pid[i] = p_hdl->buf_push_pid_sequence[i];
    }
	vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_flg);

	/* push ime output buffer */
	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		pid = sort_pid[i];
		buf_info = &ctrl_info->buf_info[pid];
		if (buf_info->buf_addr != 0) {
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "buf push, ctrl_info 0x%.8x, buf_info_%d, addr 0x%.8x, dma_en %d\r\n",
				(unsigned int)ctrl_info_addr, (int)pid, (unsigned int)buf_info->vdo_frm.addr[0], (int)ctrl_info->info.ime_ctrl.out_img[pid].dma_enable);

			if (ctrl_info->info.ime_ctrl.out_img[pid].region.enable) {
				// don't check tag when output region enable
				buf_ready = TRUE;
			} else {
				buf_ready = ctl_ipp_buf_tag_chk_buf_is_rdy(ctrl_info->owner, buf_info);
			}
			if (ctrl_info->info.ime_ctrl.out_img[pid].dma_enable && buf_ready) {

				/* skip push buffer if reference out replace by previous frame */
				if (ctrl_info->info.ime_ctrl.tplnr_enable && ctrl_info->info.ime_ctrl.tplnr_in_ref_path == pid) {
					if (ctl_ipp_buf_upd_3dnr_ref_sts_push(ctrl_info) == FALSE) {
						continue;
					}
				}

				/* set vdo_frame info if needed */
				if (ctrl_info->info.ime_ctrl.out_img[pid].fmt == VDO_PXLFMT_YUV420_NVX1_H264 ||
					ctrl_info->info.ime_ctrl.out_img[pid].fmt == VDO_PXLFMT_YUV420_NVX1_H265) {
					ctl_ipp_buf_set_nvx1info(&buf_info->vdo_frm, &ctrl_info->info);
				}

				if (ctrl_info->info.ime_ctrl.out_img[pid].md_enable) {
					ctl_ipp_buf_set_mdinfo(&buf_info->vdo_frm, &ctrl_info->info);
				}

				// move lock 3dnr into proc end task (ctl_ipp_proc_end_cb()) instead of buf task to prevent next frame's proc end may cause 3dnr ref buf be unlocked too early */
#if 0
				/* lock 3dnr ref buffer */
				if (ctrl_info->info.ime_ctrl.tplnr_enable && ctrl_info->info.ime_ctrl.tplnr_in_ref_path == pid) {
					ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_LOCK, buf_info);
				}
#endif

				/* set yuv buffer for isp get flow */
				ctl_ipp_isp_int_set_yuv_out((CTL_IPP_HANDLE *)ctrl_info->owner, buf_info);

				/* low delay mode already push before frame end */
				if (ctrl_info->info.ime_ctrl.low_delay_enable == ENABLE &&
					ctrl_info->info.ime_ctrl.low_delay_path == pid) {
					/* unlock low delay lock at bp */
					ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_UNLOCK, buf_info);
				} else {
					/* push buffer */
					ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_PUSH, buf_info);
				}
			} else {
				/* add tag failed count */
				if (ctrl_info->info.ime_ctrl.out_img[pid].dma_enable && (buf_ready == FALSE)) {
					CTL_IPP_HANDLE *p_hdl;

					p_hdl = (CTL_IPP_HANDLE *)ctrl_info->owner;
					p_hdl->buf_tag_fail_cnt[buf_info->pid] += 1;
				}

				if (ctrl_info->info.ime_ctrl.tplnr_enable && ctrl_info->info.ime_ctrl.tplnr_in_ref_path == pid) {
					/* do not release buffer, direct mode still use it for reference input
						dram mode should release in proc_end isr
					*/
				} else {
					while (buf_info->lock_cnt != 0) {
						/* dma disable when buf exist --> illegal parameters cause kdf to disable output */
						if (ctrl_info->info.ime_ctrl.out_img[pid].dma_enable == DISABLE) {
							buf_info->err_msg = CTL_IPP_E_PAR;
						}
						ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_UNLOCK, buf_info);
					}
				}
			}

			if (buf_info->lock_cnt == 0) {
				memset((void *)buf_info, 0, sizeof(CTL_IPP_OUT_BUF_INFO));
			}
		}
	}

	/* update ctrl handle info */
	ctl_ipp_buf_hdl_count_update(ctrl_info, TRUE);

	/* release ctrl_info */
	ctl_ipp_info_release(ctrl_info);
}

void ctl_ipp_buf_push_low_delay(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 rev)
{
	UINT32 i;
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_OUT_BUF_INFO *buf_info;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;

	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	/* push ime output buffer */
	if (ctrl_info->info.ime_ctrl.low_delay_enable == ENABLE &&
		ctrl_info->info.ime_ctrl.low_delay_path < CTL_IPP_OUT_PATH_ID_MAX) {
		i = ctrl_info->info.ime_ctrl.low_delay_path;
		buf_info = &ctrl_info->buf_info[i];
		if (buf_info->buf_addr != 0) {
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "buf push low delay, ctrl_info 0x%.8x, buf_info_%d, addr 0x%.8x, dma_en %d\r\n",
				(unsigned int)ctrl_info_addr, (int)i, (unsigned int)buf_info->vdo_frm.addr[0], (int)ctrl_info->info.ime_ctrl.out_img[i].dma_enable);

			/* no need to check single out tag */
			if (ctrl_info->info.ime_ctrl.out_img[i].dma_enable) {
				/* set vdo_frame info if needed */
				if (ctrl_info->info.ime_ctrl.out_img[i].fmt == VDO_PXLFMT_YUV420_NVX1_H264 ||
					ctrl_info->info.ime_ctrl.out_img[i].fmt == VDO_PXLFMT_YUV420_NVX1_H265) {
					ctl_ipp_buf_set_nvx1info(&buf_info->vdo_frm, &ctrl_info->info);
				}

				/* low delay info, nvx1 should never happened(both used reserved) */
				ctl_ipp_buf_set_lowdlyinfo(&buf_info->vdo_frm, &ctrl_info->info);

				/* lock low delay buffer, unlock when frame end push evt */
				ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_LOCK, buf_info);

				/* push low delay buffer */
				ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_PUSH, buf_info);
			} else {
				ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_UNLOCK, buf_info);
			}
		}
	}
}

void ctl_ipp_buf_release(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 bufio_stop, INT32 err_msg)
{
	UINT32 i;
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_OUT_BUF_INFO *buf_info;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;

	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	/* unlock ime output buffer */
	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		buf_info = &ctrl_info->buf_info[i];
		if (buf_info->buf_addr != 0) {
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "buf unlock, ctrl_info 0x%.8x, buf_info_%d, addr 0x%.8x\r\n",
				(unsigned int)ctrl_info_addr, (int)i, (unsigned int)buf_info->vdo_frm.addr[0]);

			/* skip if 3dnr reference keep for next frame */
			if (ctrl_info->info.ime_ctrl.tplnr_enable && ctrl_info->info.ime_ctrl.tplnr_in_ref_path == i) {
				if (ctl_ipp_buf_upd_3dnr_ref_sts_rls(ctrl_info) == FALSE) {
					continue;
				}
			}

			buf_info->err_msg = err_msg;
			while (buf_info->lock_cnt != 0) {
				ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_UNLOCK, buf_info);
			}
			memset((void *)buf_info, 0, sizeof(CTL_IPP_OUT_BUF_INFO));
		}
	}

	/* release ctrl_info */
	ctl_ipp_info_release(ctrl_info);
}

void ctl_ipp_buf_release_path(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 bufio_stop, UINT32 pid, INT32 err_msg)
{
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_OUT_BUF_INFO *buf_info;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;

	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	/* skip if 3dnr reference keep for next frame */
	if (ctrl_info->info.ime_ctrl.tplnr_enable && ctrl_info->info.ime_ctrl.tplnr_in_ref_path == pid) {
		if (ctl_ipp_buf_upd_3dnr_ref_sts_rls(ctrl_info) == FALSE) {
			return ;
		}
	}

	/* unlock ime output buffer */
	buf_info = &ctrl_info->buf_info[pid];
	if (buf_info->buf_addr != 0) {
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "buf unlock, ctrl_info 0x%.8x, buf_info_%d, addr 0x%.8x\r\n",
			(unsigned int)ctrl_info_addr, (int)pid, (unsigned int)buf_info->vdo_frm.addr[0]);

		buf_info->err_msg = err_msg;
		while (buf_info->lock_cnt != 0) {
			ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BUF_IO_UNLOCK, buf_info);
		}
		memset((void *)buf_info, 0, sizeof(CTL_IPP_OUT_BUF_INFO));
	}
}

void ctl_ipp_buf_iostop_path(UINT32 bufio_fp_addr, UINT32 hdl, UINT32 pid)
{
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_OUT_BUF_INFO buf_info;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	memset((void *)&buf_info, 0, sizeof(CTL_IPP_OUT_BUF_INFO));
	buf_info.pid = pid;
	ctl_ipp_buf_fp_wrapper(hdl, bufio_fp_addr, CTL_IPP_BUF_IO_STOP, &buf_info);
}

void ctl_ipp_buf_iostart_path(UINT32 bufio_fp_addr, UINT32 hdl, UINT32 pid)
{
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_OUT_BUF_INFO buf_info;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	memset((void *)&buf_info, 0, sizeof(CTL_IPP_OUT_BUF_INFO));
	buf_info.pid = pid;
	ctl_ipp_buf_fp_wrapper(hdl, bufio_fp_addr, CTL_IPP_BUF_IO_START, &buf_info);
}

void ctl_ipp_buf_frm_end(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 rev)
{
	UINT32 i;
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_OUT_BUF_INFO *buf_info;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;

	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		buf_info = &ctrl_info->buf_info[i];
		if (buf_info->buf_addr != 0) {
			ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BFU_IO_DRAM_END, buf_info);
		}
	}
}

void ctl_ipp_buf_frm_start(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 rev)
{
	UINT32 i;
	IPP_EVENT_FP bufio_fp;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_OUT_BUF_INFO *buf_info;

	bufio_fp = (IPP_EVENT_FP)bufio_fp_addr;
	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;

	if (bufio_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		buf_info = &ctrl_info->buf_info[i];
		if (buf_info->buf_addr != 0) {
			ctl_ipp_buf_fp_wrapper(ctrl_info->owner, bufio_fp_addr, CTL_IPP_BFU_IO_DRAM_START, buf_info);
		}
	}
}

ER ctl_ipp_buf_open_tsk(void)
{
	if (ctl_ipp_buf_task_opened) {
		CTL_IPP_DBG_ERR("re-open\r\n");
		return E_SYS;
	}

	if (g_ctl_ipp_buf_flg_id == 0) {
		CTL_IPP_DBG_ERR("g_ctl_ipp_buf_flg_id = %d\r\n", (int)(g_ctl_ipp_buf_flg_id));
		return E_SYS;
	}

	/* ctrl buf init */
	ctl_ipp_buf_task_pause_count = 0;

	/* reset flag */
	vos_flag_clr(g_ctl_ipp_buf_flg_id, FLGPTN_BIT_ALL);
	vos_flag_set(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_PROC_INIT);

	/* reset queue */
	ctl_ipp_buf_msg_reset_queue();

	/* start ctrl task */
	THREAD_CREATE(g_ctl_ipp_buf_tsk_id, ctl_ipp_buf_tsk, NULL, "ctl_ipp_buf_tsk");
	if (g_ctl_ipp_buf_tsk_id == 0) {
		return E_SYS;
	}
	THREAD_SET_PRIORITY(g_ctl_ipp_buf_tsk_id, CTL_IPP_BUF_TSK_PRI);
	THREAD_RESUME(g_ctl_ipp_buf_tsk_id);

	ctl_ipp_buf_task_opened = TRUE;

	/* set ctrl buf task to processing */
	ctl_ipp_buf_set_resume(TRUE);
	return E_OK;
}

ER ctl_ipp_buf_close_tsk(void)
{
	FLGPTN wait_flg = 0;

	if (!ctl_ipp_buf_task_opened) {
		CTL_IPP_DBG_ERR("re-close\r\n");
		return E_SYS;
	}

	while (ctl_ipp_buf_task_pause_count) {
		ctl_ipp_buf_set_pause(ENABLE, ENABLE);
	}

	vos_flag_set(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_RES);
	if (vos_flag_wait(&wait_flg, g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_EXIT_END, (TWF_ORW | TWF_CLR)) != E_OK) {
		/* if wait not ok, force destroy task */
		THREAD_DESTROY(g_ctl_ipp_buf_tsk_id);
	}

	ctl_ipp_buf_task_opened = FALSE;
	return E_OK;
}

THREAD_DECLARE(ctl_ipp_buf_tsk, p1)
{
	ER err;
	UINT32 cmd, param[3];
	FLGPTN wait_flg = 0;

	THREAD_ENTRY();
	goto CTL_IPP_BUF_PAUSE;

CTL_IPP_BUF_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(g_ctl_ipp_buf_flg_id, (CTL_IPP_BUF_TASK_IDLE | CTL_IPP_BUF_TASK_RESUME_END));
		PROFILE_TASK_IDLE();
		err = ctl_ipp_buf_msg_rcv(&cmd, &param[0], &param[1], &param[2]);
		PROFILE_TASK_BUSY();
		vos_flag_clr(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_IDLE);

		if (err != E_OK) {
			CTL_IPP_DBG_IND("[CTL_IPP_BUF]Rcv Msg error (%d)\r\n", (int)(err));
		} else {
			/* process event */
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "[CTL_IPP_BUF]P Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));

			vos_flag_set(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_CHK);
			vos_flag_wait(&wait_flg, g_ctl_ipp_buf_flg_id, (CTL_IPP_BUF_TASK_PAUSE | CTL_IPP_BUF_TASK_CHK), (TWF_ORW | TWF_CLR));

			if (cmd == CTL_IPP_BUF_MSG_IGNORE) {
				/* do nothing */
			} else if (cmd == CTL_IPP_BUF_MSG_PUSH) {
				ctl_ipp_buf_push(param[0], param[1], param[2]);
			} else if (cmd == CTL_IPP_BUF_MSG_RELEASE) {
				ctl_ipp_buf_release(param[0], param[1], param[2], CTL_IPP_E_OK);
			} else if (cmd == CTL_IPP_BUF_MSG_PUSH_LOWDLY) {
				ctl_ipp_buf_push_low_delay(param[0], param[1], param[2]);
			} else {
				CTL_IPP_DBG_WRN("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));
			}

			/* check pause after cmd is processed */
			if (wait_flg & CTL_IPP_BUF_TASK_PAUSE) {
				break;
			}
		}
	}

CTL_IPP_BUF_PAUSE:

	vos_flag_clr(g_ctl_ipp_buf_flg_id, (CTL_IPP_BUF_TASK_RESUME_END));
	vos_flag_set(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_PAUSE_END);
	vos_flag_wait(&wait_flg, g_ctl_ipp_buf_flg_id, (CTL_IPP_BUF_TASK_RES | CTL_IPP_BUF_TASK_RESUME | CTL_IPP_BUF_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flg & CTL_IPP_BUF_TASK_RES) {
		goto CTL_IPP_BUF_DESTROY;
	}

	if (wait_flg & CTL_IPP_BUF_TASK_RESUME) {
		vos_flag_clr(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_PAUSE_END);
		if (wait_flg & CTL_IPP_BUF_TASK_FLUSH) {
			ctl_ipp_buf_msg_flush();
		}
		goto CTL_IPP_BUF_PROC;
	}

CTL_IPP_BUF_DESTROY:
	vos_flag_set(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_EXIT_END);
	THREAD_RETURN(0);
}

ER ctl_ipp_buf_set_resume(BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!ctl_ipp_buf_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_buf_task_pause_count == 0) {
		ctl_ipp_buf_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
		if (b_flush_evt == ENABLE) {
			vos_flag_set(g_ctl_ipp_buf_flg_id, (CTL_IPP_BUF_TASK_RESUME | CTL_IPP_BUF_TASK_FLUSH));
		} else {
			vos_flag_set(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_RESUME);
		}

		vos_flag_wait(&wait_flg, g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_RESUME_END, TWF_ORW);
	} else {
		ctl_ipp_buf_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
	}

	return E_OK;
}

ER ctl_ipp_buf_set_pause(BOOL b_wait_end, BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!ctl_ipp_buf_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_buf_task_pause_count != 0) {
		ctl_ipp_buf_task_pause_count -= 1;

		if (ctl_ipp_buf_task_pause_count == 0) {
			unl_cpu(loc_cpu_flg);
			vos_flag_set(g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_PAUSE);

			/* send dummy msg, kdf_tsk must rcv msg to pause */
			ctl_ipp_buf_msg_snd(CTL_IPP_BUF_MSG_IGNORE, 0, 0, 0);
			if (b_wait_end == ENABLE) {
				if (vos_flag_wait(&wait_flg, g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_PAUSE_END, TWF_ORW) == E_OK) {
					if (b_flush_evt == TRUE) {
						ctl_ipp_buf_msg_flush();
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

ER ctl_ipp_buf_wait_pause_end(void)
{
	FLGPTN wait_flg;

	if (!ctl_ipp_buf_task_opened) {
		return E_SYS;
	}

	vos_flag_wait(&wait_flg, g_ctl_ipp_buf_flg_id, CTL_IPP_BUF_TASK_PAUSE_END, TWF_ORW);
	return E_OK;
}

#if 0
#endif
void ctl_ipp_buf_alloc(CTL_IPP_HANDLE *ctrl_hdl, CTL_IPP_INFO_LIST_ITEM *ctrl_info)
{
	UINT32 i, max_path_number, free_q_num;
	CTL_IPP_OUT_BUF_INFO *p_buf_info;
	CTL_IPP_IME_OUT_IMG *p_ime_path;
	VDO_FRAME *p_in_vdofrm;
	UINT32 y_height;
	UINT32 uv_height;
	UINT32 yuv_size = 0;
	UINT32 md_header_size = 0;
	UINT32 md_info_size = 0;
	UINT32 region_uv_lofs, region_uv_height;
	BOOL fastboot_skip_new_fail_msg = FALSE;

	if (ctrl_hdl->evt_outbuf_fp == NULL) {
		CTL_IPP_DBG_WRN("BUFIO_CB NULL\r\n");
		return ;
	}

	p_in_vdofrm = NULL;
	if (ctrl_info->input_header_addr != 0) {
		p_in_vdofrm = (VDO_FRAME *)ctrl_info->input_header_addr;
	}

	/* ipe/dce d2d only need to allocate path1 */
	if (ctrl_hdl->flow == CTL_IPP_FLOW_IPE_D2D || ctrl_hdl->flow == CTL_IPP_FLOW_DCE_D2D) {
		max_path_number = 1;
	} else {
		max_path_number = CTL_IPP_OUT_PATH_ID_MAX;
	}

	/* check buffer free queue number, 1 fram_end will send 2 event to queue */
	free_q_num = ctl_ipp_buf_get_free_queue_num();
	if (free_q_num <= 2) {
		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			ctrl_info->info.ime_ctrl.out_img[i].dma_enable = DISABLE;
		}
		return ;
	}

	/* allocate output buffer */
	for (i = 0; i < max_path_number; i++) {
		p_ime_path = &ctrl_info->info.ime_ctrl.out_img[i];
		if (p_ime_path->enable == ENABLE && p_ime_path->dma_enable == ENABLE) {
			p_buf_info = &ctrl_info->buf_info[i];
			memset((void *)p_buf_info, 0, sizeof(CTL_IPP_OUT_BUF_INFO));

			if (p_ime_path->region.enable) {
				// use background height as y height
				y_height = p_ime_path->region.bgn_size.h;

				// replace path lofs with background lofs (ignore original path lofs setting)
				p_ime_path->lofs[VDO_PINDEX_Y] = p_ime_path->region.bgn_lofs;
				p_ime_path->lofs[VDO_PINDEX_U] = ctl_ipp_util_y2uvlof(p_ime_path->fmt, p_ime_path->region.bgn_lofs);
				p_ime_path->lofs[VDO_PINDEX_V] = p_ime_path->lofs[VDO_PINDEX_U];

				CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "[region] path%d bgn (%d, %d) region (%d, %d, %d, %d)\r\n", (int)i, (int)p_ime_path->region.bgn_lofs, (int)p_ime_path->region.bgn_size.h,
					p_ime_path->region.region_ofs.x, p_ime_path->region.region_ofs.y, p_ime_path->crp_window.w, p_ime_path->crp_window.h);
			} else {
				y_height = p_ime_path->crp_window.h;
			}
			if (p_ime_path->h_align) {
				y_height = ALIGN_CEIL(y_height, p_ime_path->h_align);
			}
			yuv_size = ctl_ipp_util_yuvsize(p_ime_path->fmt, p_ime_path->lofs[VDO_PINDEX_Y], y_height);
			yuv_size = ALIGN_CEIL(yuv_size, VOS_ALIGN_BYTES);
			p_buf_info->pid = i;
			p_buf_info->buf_size = yuv_size;
			if (p_ime_path->md_enable) {
				/* add METADATA, MD_HEADER, MD_INFO Size*/
				md_header_size = ALIGN_CEIL((sizeof(VDO_METADATA) + sizeof(VDO_MD_INFO)), VOS_ALIGN_BYTES);
				md_info_size = ALIGN_CEIL(ctl_ipp_util_3dnr_ms_roi_size(ctrl_info->info.ime_ctrl.in_size.w, ctrl_info->info.ime_ctrl.in_size.h), VOS_ALIGN_BYTES);
				p_buf_info->buf_size += (md_header_size + md_info_size);
			}
			if (p_buf_info->buf_size == 0) {
				/* yuv size calculation fail -> Unsupport format, no need to new buffer */
				p_ime_path->dma_enable = DISABLE;
				CTL_IPP_DBG_WRN("Unsupport yuv fmt 0x%.8x, size calculation fail\r\n", (unsigned int)p_ime_path->fmt);
				continue;
			}

			if (p_in_vdofrm != NULL) {
				p_buf_info->vdo_frm.count = p_in_vdofrm->count;
				p_buf_info->vdo_frm.timestamp = p_in_vdofrm->timestamp;
			} else {
				if (ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
					p_buf_info->vdo_frm.count = ctrl_info->input_buf_id;
				}
			}

			#if defined (__KERNEL__)
				/* fastboot first frame skip new buffer for direct mode */
				if (kdrv_builtin_is_fastboot() && ctrl_hdl->is_first_handle && ctrl_info->kdf_evt_count == 0 && CTL_IPP_IS_DIRECT_FLOW(ctrl_hdl->flow)) {
					/* do nothing, next frame buffer is config by fastboot flow */
					fastboot_skip_new_fail_msg = TRUE;
				} else {
					ctl_ipp_buf_fp_wrapper((UINT32)ctrl_hdl, (UINT32)ctrl_hdl->evt_outbuf_fp, CTL_IPP_BUF_IO_NEW, p_buf_info);
				}
			#else
				ctl_ipp_buf_fp_wrapper((UINT32)ctrl_hdl, (UINT32)ctrl_hdl->evt_outbuf_fp, CTL_IPP_BUF_IO_NEW, p_buf_info);
			#endif

			if (p_buf_info->buf_addr != 0) {
				p_buf_info->vdo_frm.sign = MAKEFOURCC('V', 'F', 'R', 'M');
				p_buf_info->vdo_frm.pxlfmt = p_ime_path->fmt;

				if (p_ime_path->region.enable) { // user & hdal need vdo_frame info to get background size
					p_buf_info->vdo_frm.size.w = p_ime_path->region.bgn_size.w;
					p_buf_info->vdo_frm.size.h = p_ime_path->region.bgn_size.h;
				} else {
					p_buf_info->vdo_frm.size.w = p_ime_path->crp_window.w;
					p_buf_info->vdo_frm.size.h = p_ime_path->crp_window.h;
				}

				p_buf_info->vdo_frm.pw[VDO_PINDEX_Y] = p_buf_info->vdo_frm.size.w;
				p_buf_info->vdo_frm.pw[VDO_PINDEX_U] = ctl_ipp_util_y2uvwidth(p_buf_info->vdo_frm.pxlfmt, p_buf_info->vdo_frm.size.w);
				p_buf_info->vdo_frm.pw[VDO_PINDEX_V] = p_buf_info->vdo_frm.pw[VDO_PINDEX_U];

				p_buf_info->vdo_frm.ph[VDO_PINDEX_Y] = p_buf_info->vdo_frm.size.h;
				p_buf_info->vdo_frm.ph[VDO_PINDEX_U] = ctl_ipp_util_y2uvheight(p_buf_info->vdo_frm.pxlfmt, p_buf_info->vdo_frm.size.h);
				p_buf_info->vdo_frm.ph[VDO_PINDEX_V] = p_buf_info->vdo_frm.ph[VDO_PINDEX_U];

				uv_height = ctl_ipp_util_y2uvheight(p_buf_info->vdo_frm.pxlfmt, y_height);

				if (VDO_PXLFMT_CLASS(p_buf_info->vdo_frm.pxlfmt) == VDO_PXLFMT_CLASS_NVX) {
					if (p_buf_info->vdo_frm.pxlfmt != VDO_PXLFMT_YUV420_NVX2) {
						CTL_IPP_DBG_WRN("Unsupport yuv format 0x%.8x\r\n", (unsigned int)p_buf_info->vdo_frm.pxlfmt);
					}

					p_buf_info->vdo_frm.loff[VDO_PINDEX_Y] = ALIGN_CEIL(p_buf_info->vdo_frm.size.w, 16) * 3 / 4;
					p_buf_info->vdo_frm.loff[VDO_PINDEX_U] = p_buf_info->vdo_frm.loff[VDO_PINDEX_Y];
					p_buf_info->vdo_frm.loff[VDO_PINDEX_V] = p_buf_info->vdo_frm.loff[VDO_PINDEX_U];

					p_buf_info->vdo_frm.addr[VDO_PINDEX_Y] = p_buf_info->buf_addr;
					p_buf_info->vdo_frm.addr[VDO_PINDEX_U] = p_buf_info->vdo_frm.addr[VDO_PINDEX_Y] + (p_buf_info->vdo_frm.loff[VDO_PINDEX_Y] * y_height);
					p_buf_info->vdo_frm.addr[VDO_PINDEX_V] = p_buf_info->vdo_frm.addr[VDO_PINDEX_U] + (p_buf_info->vdo_frm.loff[VDO_PINDEX_U] * uv_height);

					p_ime_path->lofs[VDO_PINDEX_Y] = p_buf_info->vdo_frm.loff[VDO_PINDEX_Y];
					p_ime_path->lofs[VDO_PINDEX_U] = p_buf_info->vdo_frm.loff[VDO_PINDEX_U];
					p_ime_path->lofs[VDO_PINDEX_V] = p_buf_info->vdo_frm.loff[VDO_PINDEX_V];
				} else {
					p_buf_info->vdo_frm.loff[VDO_PINDEX_Y] = p_ime_path->lofs[VDO_PINDEX_Y];
					p_buf_info->vdo_frm.loff[VDO_PINDEX_U] = p_ime_path->lofs[VDO_PINDEX_U];
					p_buf_info->vdo_frm.loff[VDO_PINDEX_V] = p_ime_path->lofs[VDO_PINDEX_V];

					p_buf_info->vdo_frm.addr[VDO_PINDEX_Y] = p_buf_info->buf_addr;
					p_buf_info->vdo_frm.addr[VDO_PINDEX_U] = p_buf_info->vdo_frm.addr[VDO_PINDEX_Y] + (p_buf_info->vdo_frm.loff[VDO_PINDEX_Y] * y_height);
					p_buf_info->vdo_frm.addr[VDO_PINDEX_V] = p_buf_info->vdo_frm.addr[VDO_PINDEX_U] + (p_buf_info->vdo_frm.loff[VDO_PINDEX_U] * uv_height);
				}

				if (p_ime_path->region.enable) { // calculate eng output address by region ofs
					region_uv_lofs = ctl_ipp_util_y2uvlof(p_buf_info->vdo_frm.pxlfmt, p_ime_path->region.region_ofs.x);
					region_uv_height = ctl_ipp_util_y2uvheight(p_buf_info->vdo_frm.pxlfmt, p_ime_path->region.region_ofs.y);
					p_ime_path->addr[0] = p_buf_info->buf_addr + p_ime_path->region.region_ofs.x + (p_buf_info->vdo_frm.loff[VDO_PINDEX_Y] * p_ime_path->region.region_ofs.y);
					p_ime_path->addr[1] = p_buf_info->buf_addr + region_uv_lofs + (p_buf_info->vdo_frm.loff[VDO_PINDEX_Y] * y_height) + (p_buf_info->vdo_frm.loff[VDO_PINDEX_U] * region_uv_height);
					p_ime_path->addr[2] = p_buf_info->buf_addr + region_uv_lofs + (p_buf_info->vdo_frm.loff[VDO_PINDEX_Y] * y_height) + (p_buf_info->vdo_frm.loff[VDO_PINDEX_U] * uv_height) + (p_buf_info->vdo_frm.loff[VDO_PINDEX_V] * region_uv_height);
					p_ime_path->pa[0] = vos_cpu_get_phy_addr(p_ime_path->addr[0]);
					p_ime_path->pa[1] = vos_cpu_get_phy_addr(p_ime_path->addr[1]);
					p_ime_path->pa[2] = vos_cpu_get_phy_addr(p_ime_path->addr[2]);
				} else {
					p_ime_path->addr[0] = p_buf_info->vdo_frm.addr[VDO_PINDEX_Y];
					p_ime_path->addr[1] = p_buf_info->vdo_frm.addr[VDO_PINDEX_U];
					p_ime_path->addr[2] = p_buf_info->vdo_frm.addr[VDO_PINDEX_V];
					p_ime_path->pa[0] = vos_cpu_get_phy_addr(p_ime_path->addr[0]);
					p_ime_path->pa[1] = vos_cpu_get_phy_addr(p_ime_path->addr[1]);
					p_ime_path->pa[2] = vos_cpu_get_phy_addr(p_ime_path->addr[2]);
				}

				if ((p_ime_path->pa[0] != VOS_ADDR_INVALID) && (p_ime_path->pa[1] != VOS_ADDR_INVALID) && (p_ime_path->pa[2] != VOS_ADDR_INVALID)) {
					/* alignment info */
					if (p_ime_path->h_align > 0) {
						p_buf_info->vdo_frm.reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N');
						p_buf_info->vdo_frm.reserved[1] = y_height;
					}

					/* md meta data config */
					if (p_ime_path->md_enable) {
						p_buf_info->vdo_frm.p_next = (VDO_METADATA *)(p_buf_info->buf_addr + yuv_size);
						p_buf_info->vdo_frm.p_next->sign = MAKEFOURCC('M','V','I','F');
						p_buf_info->vdo_frm.p_next->p_next = NULL;
						p_ime_path->md_addr = p_buf_info->buf_addr + yuv_size + md_header_size;
					}
					//ctrl_info->buf_info[i] = buf_info;

					/* add tag to first & last word of yuv buffer for single out check
						direct mode with one buffer, do not set tag prevent misdrop in one buffer mode due to set_tag timing
					   buffer controlled by outside when region enable, don't add tag
					*/
					if ((ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW && p_ime_path->one_buf_mode_enable == ENABLE) || p_ime_path->region.enable) {
						// do nothing
					} else {
						ctl_ipp_buf_tag_set(p_buf_info);
					}

					CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_BUF, "buffer alloc, ctrl_info 0x%.8x, buf_info_%d, addr 0x%.8x\r\n",
						(unsigned int)ctrl_info, (int)i, (unsigned int)ctrl_info->buf_info[i].buf_addr);
				} else {
					/* Addr va2pa failed, DISABLE Output */
					p_ime_path->dma_enable = DISABLE;
					ctrl_hdl->buf_new_fail_cnt[i] += 1;
					CTL_IPP_DBG_ERR_FREQ(120, "IPP Path_%d, va2pa failed y(0x%08x 0x%08x) u(0x%08x 0x%08x) v(0x%08x 0x%08x)\r\n", (int)(i + 1), p_ime_path->addr[0], p_ime_path->pa[0], p_ime_path->addr[1], p_ime_path->pa[1], p_ime_path->addr[2], p_ime_path->pa[2]);
					memset((void *)p_buf_info, 0, sizeof(CTL_IPP_OUT_BUF_INFO));
				}
			} else {
				/* New buffer failed, DISABLE Output */
				p_ime_path->dma_enable = DISABLE;
				ctrl_hdl->buf_new_fail_cnt[i] += 1;

				if (p_buf_info->err_msg == CTL_IPP_E_DROP) {
					// HDAL use CTL_IPP_E_DROP to notify IPP that new blk fail this time is for frame rate control, so dont print msg by default
					CTL_IPP_DBG_IND("[FRC] IPP Path_%d, drop frame cnt %u\r\n", (int)(i + 1), ctrl_info->kdf_evt_count);
				} else if (fastboot_skip_new_fail_msg) {
					// fastboot first frame skip new buffer for direct mode. dont print error msg
					CTL_IPP_DBG_IND("[FASTBOOT] IPP Path_%d, skip new buffer\r\n", (int)(i + 1));
				} else {
					CTL_IPP_DBG_DUMP_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "WRN: IPP Path_%d, new buffer failed (%u), want_size 0x%.8x\r\n", (int)(i + 1), ctrl_hdl->buf_new_fail_cnt[i], (unsigned int)p_buf_info->buf_size);
				}

				memset((void *)p_buf_info, 0, sizeof(CTL_IPP_OUT_BUF_INFO));
			}
		}
	}

	/* 3dnr reference buffer status init */
	if (ctrl_info->info.ime_ctrl.tplnr_enable == ENABLE) {
		UINT32 ref_pi;

		ref_pi = ctrl_info->info.ime_ctrl.tplnr_in_ref_path;
		if (ref_pi < CTL_IPP_OUT_PATH_ID_MAX) {
			if (ctrl_info->buf_info[ref_pi].buf_addr != 0) {
				ctrl_info->tplnr_ref_buf_sts = CTL_IPP_TPLNR_REF_BUF_WAIT_PUSH | CTL_IPP_TPLNR_REF_BUF_WAIT_RELEASE;
			}
		}
	}
}

UINT32 ctl_ipp_buf_task_alloc(CTRL_IPP_BUF_ITEM item, UINT32 ctrl_info, UINT32 out_addr, USIZE out)
{
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	CTL_IPP_HANDLE *p_ctrl_hdl;
	UINT32 addr;
	UINT32 size;

	if (item >= CTRL_IPP_BUF_ITEM_MAX) {
		CTL_IPP_DBG_WRN("buf item error %d\r\n", (int)(item));
		return 0;
	}

	p_ctrl_info = (CTL_IPP_INFO_LIST_ITEM *) ctrl_info;
	if (p_ctrl_info == NULL) {
		CTL_IPP_DBG_WRN("ctl info null\r\n");
		return 0;
	}

	p_ctrl_hdl = (CTL_IPP_HANDLE *) p_ctrl_info->owner;
	if (p_ctrl_hdl == NULL) {
		CTL_IPP_DBG_WRN("Ctrl Handle is NULL\r\n");
		return 0;
	}

	if (p_ctrl_hdl->private_buf.buf_info.max_size.w * p_ctrl_hdl->private_buf.buf_info.max_size.h == 0) {
		return 0;
	}

	if (p_ctrl_info->info.ife_ctrl.in_crp_window.w > p_ctrl_hdl->private_buf.buf_info.max_size.w || p_ctrl_info->info.ife_ctrl.in_crp_window.h > p_ctrl_hdl->private_buf.buf_info.max_size.h) {
		CTL_IPP_DBG_WRN("buf overflow in_size (%d,%d), max size (%d,%d)\r\n", (int)(p_ctrl_info->info.ife_ctrl.in_crp_window.w), (int)(p_ctrl_info->info.ife_ctrl.in_crp_window.h), (int)(p_ctrl_hdl->private_buf.buf_info.max_size.w), (int)(p_ctrl_hdl->private_buf.buf_info.max_size.h));
		return 0;
	}

	/* privacy mask */
	addr = 0;
	if (item == CTRL_IPP_BUF_ITEM_PM) {
		size = ctl_ipp_util_yuvsize(VDO_PXLFMT_YUV444_ONE, out.w, out.h);
		if (size > p_ctrl_hdl->private_buf.buf_item.pm[0].size) {
			CTL_IPP_DBG_WRN("PM buf overflow\r\n");
			return 0;
		}
		addr = ctl_ipp_buf_pri_search(p_ctrl_hdl, out_addr, CTRL_IPP_BUF_ITEM_PM, (UINT32 *)p_ctrl_hdl->private_buf.buf_item.pm);
	}

	/* yout */
	if (item == CTRL_IPP_BUF_ITEM_YOUT) {
		size = ctl_ipp_util_youtsize(out.w, out.h);
		if (size > p_ctrl_hdl->private_buf.buf_item.yout[0].size) {
			CTL_IPP_DBG_WRN("yout buf overflow\r\n");
			return 0;
		}
		addr = ctl_ipp_buf_pri_search(p_ctrl_hdl, out_addr, CTRL_IPP_BUF_ITEM_YOUT, (UINT32 *)&p_ctrl_hdl->private_buf.buf_item.yout);
	}

	/* 3dnr sta */
	if (item == CTRL_IPP_BUF_ITEM_3DNR_STA) {
		addr = ctl_ipp_buf_pri_search(p_ctrl_hdl, out_addr, CTRL_IPP_BUF_ITEM_3DNR_STA, (UINT32 *)p_ctrl_hdl->private_buf.buf_item._3dnr_sta);
	}

	/* dummy uv */
	if (item == CTRL_IPP_BUF_ITEM_DUMMY_UV) {
		size = p_ctrl_info->info.dce_ctrl.in_size.w;
		if (size > p_ctrl_hdl->private_buf.buf_item.dummy_uv[0].size) {
			CTL_IPP_DBG_WRN("dummy uv  buf overflow\r\n");
			return 0;
		}
		addr = ctl_ipp_buf_pri_search(p_ctrl_hdl, out_addr, CTRL_IPP_BUF_ITEM_DUMMY_UV, (UINT32 *)p_ctrl_hdl->private_buf.buf_item.dummy_uv);
	}

	return addr;
}

void ctl_ipp_buf_pri_alloc(CTL_IPP_HANDLE *ctrl_hdl, CTL_IPP_INFO_LIST_ITEM *ctrl_info)
{
	UINT32 addr ;
	UINT32 size;
	CTL_IPP_INFO_LIST_ITEM *p_last;

	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_WRN("Ctrl Handle is NULL\r\n");
		return;
	}

	if (ctrl_info == NULL) {
		CTL_IPP_DBG_WRN("Ctrl info is NULL\r\n");
		return;
	}

	if ((ctrl_info->info.ife_ctrl.in_crp_window.w > ctrl_hdl->private_buf.buf_info.max_size.w) || (ctrl_info->info.ife_ctrl.in_crp_window.h > ctrl_hdl->private_buf.buf_info.max_size.h)) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "%s private buf overflow, real input size (%d,%d), buffer max size (%d,%d)\r\n",
			ctrl_hdl->name, (int)(ctrl_info->info.ife_ctrl.in_crp_window.w), (int)(ctrl_info->info.ife_ctrl.in_crp_window.h), (int)(ctrl_hdl->private_buf.buf_info.max_size.w), (int)(ctrl_hdl->private_buf.buf_info.max_size.h));
		return;
	}

	if (ctrl_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
		p_last = ctrl_hdl->p_last_cfg_ctrl_info;
	} else {
		p_last = ctrl_hdl->p_last_rdy_ctrl_info;
	}
	/* wdr */
	if (ctrl_info->info.dce_ctrl.wdr_enable == TRUE) {
		if (p_last != NULL) {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, p_last->info.dce_ctrl.wdr_out_addr, CTRL_IPP_BUF_ITEM_WDR, (UINT32 *)ctrl_hdl->private_buf.buf_item.wdr_subout);
		} else {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, 0, CTRL_IPP_BUF_ITEM_WDR, (UINT32 *)ctrl_hdl->private_buf.buf_item.wdr_subout);
		}
		ctrl_info->info.dce_ctrl.wdr_out_addr = addr;
	}

	/* defog
		always get buffer due to IPE_LCE function
	*/
	{
		if (p_last != NULL) {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, p_last->info.ipe_ctrl.defog_out_addr, CTRL_IPP_BUF_ITEM_DFG, (UINT32 *)ctrl_hdl->private_buf.buf_item.defog_subout);
		} else {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, 0, CTRL_IPP_BUF_ITEM_DFG, (UINT32 *)ctrl_hdl->private_buf.buf_item.defog_subout);
		}
		ctrl_info->info.ipe_ctrl.defog_out_addr = addr;
	}

	/* va */
	if (ctrl_info->info.ipe_ctrl.va_enable == TRUE) {
		if (p_last != NULL) {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, p_last->info.ipe_ctrl.va_out_addr, CTRL_IPP_BUF_ITEM_VA, (UINT32 *)ctrl_hdl->private_buf.buf_item.va);
		} else {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, 0, CTRL_IPP_BUF_ITEM_VA, (UINT32 *)ctrl_hdl->private_buf.buf_item.va);
		}
		ctrl_info->info.ipe_ctrl.va_out_addr = addr;
	}

	/* LCA */
	if (ctrl_info->info.ime_ctrl.lca_out_enable == TRUE) {
		if (p_last != NULL) {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, p_last->info.ime_ctrl.lca_out_addr, CTRL_IPP_BUF_ITEM_LCA, (UINT32 *)ctrl_hdl->private_buf.buf_item.lca);
		} else {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, 0, CTRL_IPP_BUF_ITEM_LCA, (UINT32 *)ctrl_hdl->private_buf.buf_item.lca);
		}
		/* check LCA buffer size */
		size = ctl_ipp_util_yuvsize(VDO_PXLFMT_YUV444_ONE, ctrl_info->info.ime_ctrl.lca_out_size.w, ctrl_info->info.ime_ctrl.lca_out_size.h);
		if (size > ctrl_hdl->private_buf.buf_item.lca[0].size) {
			ctrl_info->info.ime_ctrl.lca_out_addr = 0;
			CTL_IPP_DBG_WRN("LCA buf overflow\r\n");
		} else {
			ctrl_info->info.ime_ctrl.lca_out_addr = addr;
		}
	}

	/* 3DNR */
	if (ctrl_info->info.ime_ctrl.tplnr_enable == TRUE) {
		/* MV */
		if (p_last != NULL) {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, p_last->info.ime_ctrl.tplnr_out_mv_addr, CTRL_IPP_BUF_ITEM_MV, (UINT32 *)ctrl_hdl->private_buf.buf_item.mv);
		} else {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, 0, CTRL_IPP_BUF_ITEM_MV, (UINT32 *)ctrl_hdl->private_buf.buf_item.mv);
		}
		ctrl_info->info.ime_ctrl.tplnr_out_mv_addr = addr;

		/* MS */
		if (p_last != NULL) {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, p_last->info.ime_ctrl.tplnr_out_ms_addr, CTRL_IPP_BUF_ITEM_MS, (UINT32 *)ctrl_hdl->private_buf.buf_item.ms);
		} else {
			addr = ctl_ipp_buf_pri_search(ctrl_hdl, 0, CTRL_IPP_BUF_ITEM_MS, (UINT32 *)ctrl_hdl->private_buf.buf_item.ms);
		}
		ctrl_info->info.ime_ctrl.tplnr_out_ms_addr = addr;

		/* MS_ROI */
		if (ctrl_info->info.ime_ctrl.tplnr_out_ms_roi_enable) {
			if (p_last != NULL) {
				addr = ctl_ipp_buf_pri_search(ctrl_hdl, p_last->info.ime_ctrl.tplnr_out_ms_roi_addr, CTRL_IPP_BUF_ITEM_MS_ROI, (UINT32 *)ctrl_hdl->private_buf.buf_item.ms_roi);
			} else {
				addr = ctl_ipp_buf_pri_search(ctrl_hdl, 0, CTRL_IPP_BUF_ITEM_MS_ROI, (UINT32 *)ctrl_hdl->private_buf.buf_item.ms_roi);
			}
			ctrl_info->info.ime_ctrl.tplnr_out_ms_roi_addr = addr;
		}
	}
}

UINT32 ctl_ipp_buf_pri_search(CTL_IPP_HANDLE *ctrl_hdl, UINT32 last_rdy_addr, UINT32 buf_item, UINT32 *addr)
{
	UINT32 i, addr_tmp;
	UINT index;
	CTL_IPP_BUF_PRI_LOF_INFO *lofs_info = (CTL_IPP_BUF_PRI_LOF_INFO *)addr;
	UINT32 buf_num[CTRL_IPP_BUF_ITEM_MAX] = {
		CTL_IPP_BUF_LCA_NUM,
		CTL_IPP_BUF_3DNR_NUM,
		CTL_IPP_BUF_3DNR_NUM,
		CTL_IPP_BUF_DFG_NUM,
		CTL_IPP_BUF_PM_NUM,
		CTL_IPP_BUF_YOUT_NUM,
		CTL_IPP_BUF_3DNR_NUM,
		CTL_IPP_BUF_3DNR_NUM,
		CTL_IPP_BUF_3DNR_NUM,
		CTL_IPP_BUF_WDR_NUM,
		CTL_IPP_BUF_VA_NUM,
		};
	CHAR *buf_name[] = {
		"LCA",
		"3DNR_MO",
		"3DNR_MV",
		"DFG",
		"PM",
		"YOUT",
		"3DNR_MS_ROI",
		"3DNR_MS",
		"3DNR_STA",
		"WDR",
		"VA",
		""
	};

	if (ctrl_hdl->bufcfg.start_addr == 0) {
		CTL_IPP_DBG_WRN("start addr can not be sezo\r\n");
		return 0;
	}

	if (last_rdy_addr != 0) {
		for (i = 0; i < buf_num[buf_item]; i++) {
			addr_tmp = ctrl_hdl->bufcfg.start_addr + lofs_info[i].lofs;
			if (last_rdy_addr == addr_tmp) {
				break;
			}
		}
		if (i == buf_num[buf_item]) {
			CTL_IPP_DBG_WRN("%s buffer search fail, 0x%.8x\r\n", buf_name[buf_item], last_rdy_addr);
			index = 0;
		} else {
			index = (i+1)%buf_num[buf_item];
		}
	} else {
		index = 0;
	}

	if (lofs_info[index].lofs == CTL_IPP_BUF_NO_USE) {
		CTL_IPP_DBG_WRN("buf_item %d, %s idx(%d) = no use \r\n", (int)(buf_item), lofs_info[index].name, (int)(index));
		return 0;
	} else {
		return ctrl_hdl->bufcfg.start_addr + lofs_info[index].lofs;
	}

}
void ctl_ipp_buf_pri_init(UINT32 buf_info)
{
	CTL_IPP_BUF_PRI_INFO *p_buf_info;
	UINT32 i;

	p_buf_info = (CTL_IPP_BUF_PRI_INFO *) buf_info;
	for (i = 0; i < CTL_IPP_BUF_LCA_NUM; i++) {
		p_buf_info->buf_item.lca[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.lca[i].name = "";
		p_buf_info->buf_item.lca[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
		p_buf_info->buf_item.mo[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.mo[i].name = "";
		p_buf_info->buf_item.mo[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
		p_buf_info->buf_item.mv[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.mv[i].name = "";
		p_buf_info->buf_item.mv[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_DFG_NUM; i++) {
		p_buf_info->buf_item.defog_subout[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.defog_subout[i].name = "";
		p_buf_info->buf_item.defog_subout[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_YOUT_NUM; i++) {
		p_buf_info->buf_item.yout[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.yout[i].name = "";
		p_buf_info->buf_item.yout[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
		p_buf_info->buf_item.ms[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.ms[i].name = "";
		p_buf_info->buf_item.ms[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
		p_buf_info->buf_item.ms_roi[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.ms_roi[i].name = "";
		p_buf_info->buf_item.ms_roi[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_3DNR_STA_NUM; i++) {
		p_buf_info->buf_item._3dnr_sta[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item._3dnr_sta[i].name = "";
		p_buf_info->buf_item._3dnr_sta[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_PM_NUM; i++) {
		p_buf_info->buf_item.pm[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.pm[i].name = "";
		p_buf_info->buf_item.pm[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_WDR_NUM; i++) {
		p_buf_info->buf_item.wdr_subout[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.wdr_subout[i].name = "";
		p_buf_info->buf_item.wdr_subout[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_VA_NUM; i++) {
		p_buf_info->buf_item.va[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.va[i].name = "";
		p_buf_info->buf_item.va[i].size = 0;
	}

	for (i = 0; i < CTL_IPP_BUF_DUMMY_UV_NUM; i++) {
		p_buf_info->buf_item.dummy_uv[i].lofs = CTL_IPP_BUF_NO_USE;
		p_buf_info->buf_item.dummy_uv[i].name = "";
		p_buf_info->buf_item.dummy_uv[i].size = 0;
	}

	return;

}
