#include "ctl_ipp_int.h"
#include "ctl_ipp_id_int.h"
#include "ctl_ipp_isp_int.h"
#include "ctl_ipp_buf_int.h"
#include "kdf_ipp_int.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

#if 0
#endif
#define CTL_IPP_MSG_Q_NUM   32
static CTL_IPP_LIST_HEAD free_msg_list_head;
static CTL_IPP_LIST_HEAD proc_msg_list_head;
static UINT32 ctl_ipp_msg_queue_free;
static CTL_IPP_MSG_EVENT ctl_ipp_msg_pool[CTL_IPP_MSG_Q_NUM];
VDO_FRAME *p_dir_vdoin;

static CTL_IPP_MSG_EVENT *ctl_ipp_msg_pool_get_msg(UINT32 idx)
{
	if (idx >= CTL_IPP_MSG_Q_NUM) {
		CTL_IPP_DBG_IND("msg idx = %d overflow\r\n", (int)(idx));
		return NULL;
	}
	return &ctl_ipp_msg_pool[idx];
}

static void ctl_ipp_msg_reset(CTL_IPP_MSG_EVENT *p_event)
{
	if (p_event) {
		p_event->cmd = 0;
		p_event->param[0] = 0;
		p_event->param[1] = 0;
		p_event->param[2] = 0;
		p_event->rev[0] = CTL_IPP_MSG_STS_FREE;
	}
}

static CTL_IPP_MSG_EVENT *ctl_ipp_msg_lock(void)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_MSG_EVENT *p_event = NULL;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&free_msg_list_head)) {
		p_event = vos_list_entry(free_msg_list_head.next, CTL_IPP_MSG_EVENT, list);
		vos_list_del(&p_event->list);

		p_event->rev[0] |= CTL_IPP_MSG_STS_LOCK;
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[CTL_IPL]Get free queue idx = %d\r\n", (int)(p_event->rev[1]));
		ctl_ipp_msg_queue_free -= 1;

		unl_cpu(loc_cpu_flg);
		return p_event;
	}

	unl_cpu(loc_cpu_flg);
	return p_event;
}

static void ctl_ipp_msg_unlock(CTL_IPP_MSG_EVENT *p_event)
{
	unsigned long loc_cpu_flg;

	if ((p_event->rev[0] & CTL_IPP_MSG_STS_LOCK) != CTL_IPP_MSG_STS_LOCK) {
		CTL_IPP_DBG_IND("event status error (%d)\r\n", (int)(p_event->rev[0]));
	} else {

		ctl_ipp_msg_reset(p_event);

		loc_cpu(loc_cpu_flg);
		vos_list_add_tail(&p_event->list, &free_msg_list_head);
		ctl_ipp_msg_queue_free += 1;
		unl_cpu(loc_cpu_flg);
	}
}

UINT32 ctl_ipp_get_free_queue_num(void)
{
	return ctl_ipp_msg_queue_free;
}

ER ctl_ipp_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, IPP_EVENT_FP drop_fp)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_MSG_EVENT *p_event;

	p_event = ctl_ipp_msg_lock();

	if (p_event == NULL) {
		return E_SYS;
	}

	p_event->cmd = cmd;
	p_event->param[0] = p1;
	p_event->param[1] = p2;
	p_event->param[2] = p3;
	p_event->drop_fp = drop_fp;
	p_event->snd_time = ctl_ipp_util_get_syst_counter();
	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[CTL_IPL]S Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(p1), (unsigned int)(p2), (unsigned int)(p3));

	/* add to proc list & trig proc task */
	loc_cpu(loc_cpu_flg);
	vos_list_add_tail(&p_event->list, &proc_msg_list_head);
	unl_cpu(loc_cpu_flg);
	vos_flag_iset(g_ctl_ipp_flg_id, CTL_IPP_QUEUE_PROC);
	return E_OK;
}

ER ctl_ipp_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, IPP_EVENT_FP *drop_fp, UINT64 *snd_t)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_MSG_EVENT *p_event;
	FLGPTN flag;
	UINT32 vos_list_empty_flag;

	while (1) {
		/* check empty or not */
		loc_cpu(loc_cpu_flg);
		vos_list_empty_flag = vos_list_empty(&proc_msg_list_head);
		if (!vos_list_empty_flag) {
			p_event = vos_list_entry(proc_msg_list_head.next, CTL_IPP_MSG_EVENT, list);
			vos_list_del(&p_event->list);
		}
		unl_cpu(loc_cpu_flg);

		/* is empty enter wait mode */
		if (vos_list_empty_flag) {
			vos_flag_wait(&flag, g_ctl_ipp_flg_id, CTL_IPP_QUEUE_PROC, TWF_CLR);
		} else {
			break;
		}
	}

	*cmd = p_event->cmd;
	*p1 = p_event->param[0];
	*p2 = p_event->param[1];
	*p3 = p_event->param[2];
	*drop_fp = p_event->drop_fp;
	*snd_t = p_event->snd_time;
	ctl_ipp_msg_unlock(p_event);

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[CTL_IPL]R Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(*cmd), (unsigned int)(*p1), (unsigned int)(*p2), (unsigned int)(*p3));
	return E_OK;
}

ER ctl_ipp_msg_flush(void)
{
	IPP_EVENT_FP drop_fp;
	UINT32 cmd, param[3];
	UINT64 time;
	ER err = E_OK;

	while (!vos_list_empty(&proc_msg_list_head)) {
		drop_fp = NULL;
		err = ctl_ipp_msg_rcv(&cmd, &param[0], &param[1], &param[2], &drop_fp, &time);

		if (err != E_OK) {
			CTL_IPP_DBG_IND("flush fail (%d)\r\n", (int)(err));
		}

		/* drop event callback */
		if (cmd != CTL_IPP_MSG_IGNORE) {
			ctl_ipp_drop(param[0], param[1], param[2], drop_fp, CTL_IPP_E_FLUSH);
		}
	}

	return err;
}

ER ctl_ipp_erase_queue(UINT32 handle)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_MSG_EVENT *p_event;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&proc_msg_list_head)) {
		vos_list_for_each(p_list, &proc_msg_list_head) {
			p_event = vos_list_entry(p_list, CTL_IPP_MSG_EVENT, list);
			if (p_event->cmd != CTL_IPP_MSG_IGNORE) {
				if (p_event->param[0] == handle) {
					ctl_ipp_drop(p_event->param[0], p_event->param[1], p_event->param[2], p_event->drop_fp, CTL_IPP_E_FLUSH);
					p_event->cmd = CTL_IPP_MSG_IGNORE;
				}
			}
		}
	}

	unl_cpu(loc_cpu_flg);

	return E_OK;
}

ER ctl_ipp_msg_reset_queue(void)
{
	UINT32 i;
	CTL_IPP_MSG_EVENT *free_event;

	/* init free & proc list head */
	VOS_INIT_LIST_HEAD(&free_msg_list_head);
	VOS_INIT_LIST_HEAD(&proc_msg_list_head);

	for (i = 0; i < CTL_IPP_MSG_Q_NUM; i++) {
		free_event = ctl_ipp_msg_pool_get_msg(i);
		ctl_ipp_msg_reset(free_event);
		free_event->rev[1] = i;
		vos_list_add_tail(&free_event->list, &free_msg_list_head);
	}
	ctl_ipp_msg_queue_free = CTL_IPP_MSG_Q_NUM;
	return E_OK;
}

#if 0
#endif
static CTL_IPP_INFO_POOL ctl_ipp_info_pool;

static UINT32 ctl_ipp_info_validate(CTL_IPP_INFO_LIST_ITEM *p_info)
{
	UINT32 i;

	for (i = 0; i < ctl_ipp_info_pool.count; i++) {
		if ((UINT32)p_info == (UINT32)&ctl_ipp_info_pool.info[i]) {
			return CTL_IPP_E_OK;
		}
	}

	CTL_IPP_DBG_IND("invalid info(0x%.8x)\r\n", (unsigned int)p_info);
	return CTL_IPP_E_ID;
}

static void ctl_ipp_info_reset(CTL_IPP_INFO_LIST_ITEM *p_info)
{
	UINT32 i;

	p_info->owner = 0;
	p_info->lock_cnt = 0;
	p_info->input_buf_id = 0;
	p_info->input_header_addr = 0;
	p_info->kdf_evt_count = 0;
	p_info->reserved = 0;
	p_info->evt_tag = 0;
	p_info->tplnr_ref_buf_sts = 0;
	memset((void *)&p_info->info, 0, sizeof(CTL_IPP_BASEINFO));
	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
	       memset((void *)&p_info->buf_info[i], 0, sizeof(CTL_IPP_OUT_BUF_INFO));
	}
}

UINT32 ctl_ipp_info_pool_init(UINT32 num, UINT32 buf_addr, UINT32 is_query)
{
	UINT32 i, buf_size, in_num;

	/* adjust buffer number */
	in_num = num;
	num = num * 4;

	/* buffer allocation*/
	buf_size = sizeof(CTL_IPP_INFO_LIST_ITEM) * num;
	if (is_query) {
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_INFO", CTL_IPP_DBG_CTX_BUF_QUERY, buf_size, buf_addr, in_num);
		return buf_size;
	}

	ctl_ipp_info_pool.info = (void *)buf_addr;
	if (ctl_ipp_info_pool.info == NULL) {
		CTL_IPP_DBG_ERR("alloc ctl_info memory failed, num %d, want_size 0x%.8x\r\n", (int)num, (unsigned int)buf_size);
		return CTL_IPP_E_NOMEM;
	}
	ctl_ipp_info_pool.count = num;
	CTL_IPP_DBG_TRC("alloc ctl_info memory, num %d, size 0x%.8x, addr 0x%.8x\r\n", (int)num, (unsigned int)buf_size, (unsigned int)ctl_ipp_info_pool.info);

	/* reset & create free ctrl_info pool list */
	VOS_INIT_LIST_HEAD(&ctl_ipp_info_pool.free_head);
	VOS_INIT_LIST_HEAD(&ctl_ipp_info_pool.used_head);

	for (i = 0; i < ctl_ipp_info_pool.count; i++) {
		ctl_ipp_info_reset(&ctl_ipp_info_pool.info[i]);
		vos_list_add_tail(&ctl_ipp_info_pool.info[i].list, &ctl_ipp_info_pool.free_head);
	}

	ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_INFO", CTL_IPP_DBG_CTX_BUF_ALLOC, buf_size, buf_addr, in_num);

	return buf_size;
}

void ctl_ipp_info_pool_free(void)
{
	if (ctl_ipp_info_pool.info != NULL) {
		CTL_IPP_DBG_TRC("free ctl_info memory, addr 0x%.8x\r\n", (unsigned int)ctl_ipp_info_pool.info);
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_INFO", CTL_IPP_DBG_CTX_BUF_FREE, 0, (UINT32)ctl_ipp_info_pool.info, 0);
		ctl_ipp_info_pool.count = 0;
		ctl_ipp_info_pool.info = NULL;
	}
}

void ctl_ipp_info_release(CTL_IPP_INFO_LIST_ITEM *p_info)
{
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_info_validate(p_info) == CTL_IPP_E_OK) {
		if (p_info->lock_cnt == 0) {
			CTL_IPP_DBG_WRN("ipp_info 0x%.8x already freed\r\n", (unsigned int)p_info);
		} else {
			p_info->lock_cnt -= 1;

			if (p_info->lock_cnt == 0) {
				p_info->owner = 0;
				vos_list_del(&p_info->list);
				vos_list_add_tail(&p_info->list, &ctl_ipp_info_pool.free_head);
			}
		}
	}
	unl_cpu(loc_cpu_flg);
}

CTL_IPP_INFO_LIST_ITEM *ctl_ipp_info_get_entry_by_info_addr(UINT32 addr)
{
	UINT32 entry_addr;
	CTL_IPP_INFO_LIST_ITEM *p_rt;

	if (addr == 0) {
		return NULL;
	}

	entry_addr = addr - CTL_IPP_UTIL_OFFSETOF(CTL_IPP_INFO_LIST_ITEM, info);
	p_rt = (CTL_IPP_INFO_LIST_ITEM *)entry_addr;
	if (ctl_ipp_info_validate(p_rt) != CTL_IPP_E_OK) {
		return NULL;
	}

	if (p_rt->lock_cnt == 0 || p_rt->owner == 0) {
		CTL_IPP_DBG_IND("illegal ctrl_info(0x%.8x), lock_cnt = %d owner = 0x%.8x\r\n", (unsigned int)entry_addr, (int)p_rt->lock_cnt, (unsigned int)p_rt->owner);
		return NULL;
	}

	return ((CTL_IPP_INFO_LIST_ITEM *)entry_addr);
}

CTL_IPP_INFO_LIST_ITEM *ctl_ipp_info_alloc(UINT32 owner)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_INFO_LIST_ITEM *p_info = NULL;

	loc_cpu(loc_cpu_flg);
	if (!vos_list_empty(&ctl_ipp_info_pool.free_head)) {
		p_info = vos_list_entry(ctl_ipp_info_pool.free_head.next, CTL_IPP_INFO_LIST_ITEM, list);
		ctl_ipp_info_reset(p_info);
		p_info->owner = owner;
		p_info->lock_cnt = 1;
		vos_list_del(&p_info->list);
		vos_list_add_tail(&p_info->list, &ctl_ipp_info_pool.used_head);
	}
	unl_cpu(loc_cpu_flg);

	return p_info;
}

void ctl_ipp_info_lock(CTL_IPP_INFO_LIST_ITEM *p_info)
{
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_info_validate(p_info) == CTL_IPP_E_OK) {
		p_info->lock_cnt += 1;
	}
	unl_cpu(loc_cpu_flg);
}

void ctl_ipp_info_dump(int (*dump)(const char *fmt, ...))
{
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_INFO_LIST_ITEM *p_info;

	loc_cpu(loc_cpu_flg);
	dump("\r\n---- USED LIST ----\r\n");
	dump("%12s%12s%12s%12s%12s\r\n", "info_addr", "owner", "lock_cnt", "buf_id", "head_addr");
	vos_list_for_each_safe(p_list, n, &ctl_ipp_info_pool.used_head) {
		p_info = vos_list_entry(p_list, CTL_IPP_INFO_LIST_ITEM, list);
		dump("%#.8x  %#.8x  %12d%12d  %#.8x\r\n", (unsigned int)p_info, (unsigned int)p_info->owner,
			(int)p_info->lock_cnt, (int)p_info->input_buf_id, (unsigned int)p_info->input_header_addr);
	}

	dump("\r\n---- FREE LIST ----\r\n");
	dump("%12s%12s%12s%12s%12s\r\n", "info_addr", "owner", "lock_cnt", "buf_id", "head_addr");
	vos_list_for_each_safe(p_list, n, &ctl_ipp_info_pool.free_head) {
		p_info = vos_list_entry(p_list, CTL_IPP_INFO_LIST_ITEM, list);
		dump("%#.8x  %#.8x  %12d%12d  %#.8x\r\n", (unsigned int)p_info, (unsigned int)p_info->owner,
			(int)p_info->lock_cnt, (int)p_info->input_buf_id, (unsigned int)p_info->input_header_addr);
	}
	unl_cpu(loc_cpu_flg);
}

#if 0
#endif

INT32 ctl_ipp_int_dce_dummy_uv(CTL_IPP_INFO_LIST_ITEM *p_ctrl_info)
{
	/* special case, set lofs[1] = 0, addr[1] as dummy uv line buffer for input image only has one plane */
	if ((p_ctrl_info->info.dce_ctrl.in_fmt == VDO_PXLFMT_Y8) || (p_ctrl_info->info.dce_ctrl.in_fmt == VDO_PXLFMT_520_IR8)) {
		p_ctrl_info->info.dce_ctrl.in_lofs[1] = 0;
		p_ctrl_info->info.dce_ctrl.in_addr[1] = ctl_ipp_buf_task_alloc(CTRL_IPP_BUF_ITEM_DUMMY_UV, (UINT32)p_ctrl_info, 0, p_ctrl_info->info.dce_ctrl.in_size);
		if (p_ctrl_info->info.dce_ctrl.in_addr[1] == 0) {
			CTL_IPP_DBG_WRN("no dummy uv buffer for input format 0x%.8x\r\n", (unsigned int)p_ctrl_info->info.dce_ctrl.in_fmt);
			return CTL_IPP_E_NOMEM;
		}
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_int_ime_flip(CTL_IPP_INFO_LIST_ITEM *p_ctrl_info)
{
	/* adjust flip to ime output, note that out flip can be set seperatly at other cmd */
	if (p_ctrl_info->info.ife_ctrl.flip & CTL_IPP_FLIP_V) {
		UINT32 i;

		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			if (p_ctrl_info->info.ime_ctrl.out_img[i].flip_enable) {
				p_ctrl_info->info.ime_ctrl.out_img[i].flip_enable = DISABLE;
			} else {
				p_ctrl_info->info.ime_ctrl.out_img[i].flip_enable = ENABLE;
			}
		}
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_process_raw(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id)
{
	KDF_IPP_EVT evt;
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info_arr[2];
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	VDO_FRAME *p_vdoin_info;
	VDO_PXLFMT pix_fmt;
	UINT32 i, kdf_qnum;

	p_vdoin_info = (VDO_FRAME *)header_addr;

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_WRN("Ctrl handle 0x%.8x have no KDF handle\r\n", (unsigned int)p_hdl);
		return CTL_IPP_E_ID;
	}

	/* Check header info */
	if (p_vdoin_info->addr[0] == 0 ||
		p_vdoin_info->size.w == 0 ||
		p_vdoin_info->size.h == 0 ||
		p_vdoin_info->loff[0] == 0) {
		CTL_IPP_DBG_WRN("vdoin info error, addr 0x%.8x, size (%d, %d), loff %d\r\n",
			(unsigned int)p_vdoin_info->addr[0], (int)p_vdoin_info->size.w, (int)p_vdoin_info->size.h, (int)p_vdoin_info->loff[0]);
		return CTL_IPP_E_INDATA;
	}

	/* Update SIE header info to ctrl info */

	/* IFE Ctrl */
	{
		UINT32 frm_num;

		p_hdl->ctrl_info.ife_ctrl.in_addr[0] = p_vdoin_info->addr[0];
		p_hdl->ctrl_info.ife_ctrl.in_lofs[0] = p_vdoin_info->loff[0];
		p_hdl->ctrl_info.ife_ctrl.in_size.w = (UINT32)p_vdoin_info->size.w;
		p_hdl->ctrl_info.ife_ctrl.in_size.h = (UINT32)p_vdoin_info->size.h;

		/* crop window */
		if (p_hdl->ctrl_info.ife_ctrl.in_crp_mode == CTL_IPP_IN_CROP_AUTO) {
			/* crop window from sie */
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.x = (p_vdoin_info->reserved[5] & 0xffff0000) >> 16;
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.y = (p_vdoin_info->reserved[5] & 0x0000ffff);
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.w = (p_vdoin_info->reserved[6] & 0xffff0000) >> 16;
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.h = (p_vdoin_info->reserved[6] & 0x0000ffff);
		} else if (p_hdl->ctrl_info.ife_ctrl.in_crp_mode == CTL_IPP_IN_CROP_NONE) {
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.x = 0;
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.y = 0;
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.w = p_hdl->ctrl_info.ife_ctrl.in_size.w;
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.h = p_hdl->ctrl_info.ife_ctrl.in_size.h;
		}

		if ((p_hdl->ctrl_info.ife_ctrl.in_crp_window.x + p_hdl->ctrl_info.ife_ctrl.in_crp_window.w > p_hdl->ctrl_info.ife_ctrl.in_size.w) ||
			(p_hdl->ctrl_info.ife_ctrl.in_crp_window.y + p_hdl->ctrl_info.ife_ctrl.in_crp_window.h > p_hdl->ctrl_info.ife_ctrl.in_size.h) ||
			(p_hdl->ctrl_info.ife_ctrl.in_crp_window.w == 0) ||
			(p_hdl->ctrl_info.ife_ctrl.in_crp_window.h == 0)) {
			CTL_IPP_DBG_WRN("crop window(%d %d %d %d) error, in_size(%d %d)\r\n",
				(int)p_hdl->ctrl_info.ife_ctrl.in_crp_window.x, (int)p_hdl->ctrl_info.ife_ctrl.in_crp_window.y,
				(int)p_hdl->ctrl_info.ife_ctrl.in_crp_window.w, (int)p_hdl->ctrl_info.ife_ctrl.in_crp_window.h,
				(int)p_hdl->ctrl_info.ife_ctrl.in_size.w, (int)p_hdl->ctrl_info.ife_ctrl.in_size.h);

			p_hdl->ctrl_info.ife_ctrl.in_crp_window.x = 0;
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.y = 0;
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.w = p_hdl->ctrl_info.ife_ctrl.in_size.w;
			p_hdl->ctrl_info.ife_ctrl.in_crp_window.h = p_hdl->ctrl_info.ife_ctrl.in_size.h;
		}
		p_hdl->ctrl_info.ife_ctrl.in_fmt = ctl_ipp_info_pxlfmt_by_crop(p_vdoin_info->pxlfmt, p_hdl->ctrl_info.ife_ctrl.in_crp_window.x, p_hdl->ctrl_info.ife_ctrl.in_crp_window.y);

		/* raw decode */
		if (VDO_PXLFMT_CLASS(p_vdoin_info->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
			p_hdl->ctrl_info.ife_ctrl.decode_enable = ENABLE;
			p_hdl->ctrl_info.ife_ctrl.decode_ratio = p_vdoin_info->reserved[7];
		} else {
			p_hdl->ctrl_info.ife_ctrl.decode_enable = DISABLE;
		}

		/* SHDR	*/
		frm_num = VDO_PXLFMT_PLANE(p_vdoin_info->pxlfmt);
		if (frm_num > 1) {
			UINT32 i;
			VDO_FRAME *p_vdoin_tmp;

			for (i = 1; i < frm_num; i++) {
				if (i >= 2) {
					CTL_IPP_DBG_WRN("only support 2 frame hdr, input format 0x%.8x\r\n", (unsigned int)p_vdoin_info->pxlfmt);
					break;
				}
				if (p_vdoin_info->addr[i] != 0) {
					p_vdoin_tmp = (VDO_FRAME *)p_vdoin_info->addr[i];

					if (p_vdoin_tmp->addr[0] == 0) {
						CTL_IPP_DBG_WRN("PXLFMT 0x%.8x, frame_number = %d, addr[%d] = 0\r\n",
							(unsigned int)p_vdoin_tmp->pxlfmt, (int)frm_num, (int)i);
						return CTL_IPP_E_INDATA;
					}

					if (p_vdoin_tmp->size.w != (INT32)p_hdl->ctrl_info.ife_ctrl.in_size.w ||
						p_vdoin_tmp->size.h != (INT32)p_hdl->ctrl_info.ife_ctrl.in_size.h) {
						CTL_IPP_DBG_WRN("PXLFMT 0x%.8x, frame_number = %d, size[%d] (%d %d) not match to first frame(%d %d)\r\n",
							(unsigned int)p_vdoin_info->pxlfmt, (int)frm_num, (int)i, (int)p_vdoin_tmp->size.w, (int)p_vdoin_tmp->size.h,
							(int)p_hdl->ctrl_info.ife_ctrl.in_size.w, (int)p_hdl->ctrl_info.ife_ctrl.in_size.h);
						return CTL_IPP_E_INDATA;
					}

					if (p_vdoin_tmp->pxlfmt != p_vdoin_info->pxlfmt) {
						CTL_IPP_DBG_WRN("PXLFMT 0x%.8x, frame_number = %d, format 0x%.8x not match to first frame(0x%.8x)\r\n",
							(unsigned int)p_vdoin_info->pxlfmt, (int)frm_num, (unsigned int)p_vdoin_tmp->pxlfmt, (unsigned int)p_vdoin_info->pxlfmt);
						return CTL_IPP_E_INDATA;
					}

					p_hdl->ctrl_info.ife_ctrl.in_addr[i] = p_vdoin_tmp->addr[0];
					p_hdl->ctrl_info.ife_ctrl.in_lofs[i] = p_vdoin_tmp->loff[0];
				} else {
					CTL_IPP_DBG_WRN("PXLFMT 0x%.8x, frame_number = %d, vdofrm_addr[%d] = 0\r\n",
						(unsigned int)p_vdoin_info->pxlfmt, (int)frm_num, (int)i);
					return CTL_IPP_E_INDATA;
				}
			}
		}
	}

	/* change pixel start by flip
		ife only support mirror, flip is done at ime output
	*/
	pix_fmt = (VDO_PXLFMT) ctl_ipp_info_pxlfmt_by_flip((UINT32)p_hdl->ctrl_info.ife_ctrl.in_fmt, (p_hdl->ctrl_info.ife_ctrl.flip & CTL_IPP_FLIP_H));

	/* DCE Ctrl */
	{
		p_hdl->ctrl_info.dce_ctrl.in_fmt = pix_fmt;
		p_hdl->ctrl_info.dce_ctrl.in_size.w = p_hdl->ctrl_info.ife_ctrl.in_crp_window.w;
		p_hdl->ctrl_info.dce_ctrl.in_size.h = p_hdl->ctrl_info.ife_ctrl.in_crp_window.h;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.x = 0;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.y = 0;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.w = p_hdl->ctrl_info.dce_ctrl.in_size.w;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.h = p_hdl->ctrl_info.dce_ctrl.in_size.h;
	}

	/* IPE Ctrl */
	{
		p_hdl->ctrl_info.ipe_ctrl.in_size.w = p_hdl->ctrl_info.ife_ctrl.in_crp_window.w;
		p_hdl->ctrl_info.ipe_ctrl.in_size.h = p_hdl->ctrl_info.ife_ctrl.in_crp_window.h;
		p_hdl->ctrl_info.ipe_ctrl.in_lofs[0] = p_hdl->ctrl_info.ipe_ctrl.in_size.w;
		p_hdl->ctrl_info.ipe_ctrl.in_lofs[1] = p_hdl->ctrl_info.ipe_ctrl.in_size.w;

		/* eth, check buffer size */
		ctl_ipp_isp_int_get(p_hdl, CTL_IPP_ISP_ITEM_ETH_PARAM, (void *)&p_hdl->ctrl_info.ipe_ctrl.eth);
		p_hdl->ctrl_info.ipe_ctrl.eth.out_size = p_hdl->ctrl_info.ipe_ctrl.in_size;
		if (p_hdl->ctrl_info.ipe_ctrl.eth.enable) {
			UINT32 eth_want_size;

			/* check orignal */
			eth_want_size = ctl_ipp_util_ethsize(p_hdl->ctrl_info.ipe_ctrl.in_size.w, p_hdl->ctrl_info.ipe_ctrl.in_size.h,
											p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel, DISABLE);

			if (p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_ORIGINAL ||
				p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_BOTH) {
				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0] < eth_want_size) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to buffer size 0x%.8x not enough, ipe img(%d %d), eth out_bit_sel %d, out_sel %d, want_size 0x%.8x\r\n",
						(unsigned int)p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0], (int)p_hdl->ctrl_info.ipe_ctrl.in_size.w, (int)p_hdl->ctrl_info.ipe_ctrl.in_size.h,
						(int)p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel,(int)p_hdl->ctrl_info.ipe_ctrl.eth.out_sel, (unsigned int)eth_want_size);
				}

				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_addr[0] == 0) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to addr 0\r\n");
				}
			}

			/* check downsample */
			eth_want_size = ctl_ipp_util_ethsize(p_hdl->ctrl_info.ipe_ctrl.in_size.w, p_hdl->ctrl_info.ipe_ctrl.in_size.h,
											p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel, ENABLE);

			if (p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE ||
				p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_BOTH) {
				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[1] < eth_want_size) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to buffer size 0x%.8x not enough, ipe img(%d %d), eth out_bit_sel %d, out_sel %d, want_size 0x%.8x\r\n",
						(unsigned int)p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0], (int)p_hdl->ctrl_info.ipe_ctrl.in_size.w, (int)p_hdl->ctrl_info.ipe_ctrl.in_size.h,
						(int)p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel,(int) p_hdl->ctrl_info.ipe_ctrl.eth.out_sel, (unsigned int)eth_want_size);
				}

				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_addr[1] == 0) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to addr 0\r\n");
				}
			}

			/* set frm cnt and timestamp */
			p_hdl->ctrl_info.ipe_ctrl.eth.frm_cnt = p_vdoin_info->count;
			p_hdl->ctrl_info.ipe_ctrl.eth.timestamp = p_vdoin_info->timestamp;
		}
	}

	/* IME Ctrl */
	{
		p_hdl->ctrl_info.ime_ctrl.in_size.w = p_hdl->ctrl_info.ife_ctrl.in_crp_window.w;
		p_hdl->ctrl_info.ime_ctrl.in_size.h = p_hdl->ctrl_info.ife_ctrl.in_crp_window.h;

		/* 3dnr sta info */
		if (p_hdl->ctrl_info.ime_ctrl.tplnr_enable == ENABLE) {
			ctl_ipp_isp_int_get(p_hdl, CTL_IPP_ISP_ITEM_3DNR_STA, (void *)&p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info);
			if (p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.enable == ENABLE) {
				if (p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.buf_addr == 0) {
					p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.enable = DISABLE;
					CTL_IPP_DBG_IND("3dnr sta disable due to addr 0\r\n");
				}
			}
		}

		/* set path6 size base on input */
		if (p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].enable == ENABLE) {
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.w = p_hdl->ctrl_info.ime_ctrl.in_size.w >> 1;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.h = p_hdl->ctrl_info.ime_ctrl.in_size.h >> 1;
			if (p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].fmt == VDO_PXLFMT_520_IR16) {
				p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = p_hdl->ctrl_info.ime_ctrl.in_size.w;
			} else {
				p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = p_hdl->ctrl_info.ime_ctrl.in_size.w >> 1;
			}
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = ALIGN_CEIL_4(p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0]);

			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.x = 0;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.y = 0;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.w = p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.w;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.h = p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.h;
		}
	}

	if (p_hdl->flow == CTL_IPP_FLOW_RAW) {
		/* Get ctl_ipp_info from pool */
		p_ctrl_info = ctl_ipp_info_alloc((UINT32)p_hdl);
		if (p_ctrl_info == NULL) {
			return CTL_IPP_E_QOVR;
		}
		memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));

		/* IME Ctrl */
		ctl_ipp_int_ime_flip(p_ctrl_info);

		p_ctrl_info->input_header_addr = header_addr;
		p_ctrl_info->input_buf_id = buf_id;
		p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
		p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
		p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;

		/* Send KDF Event */
		evt.data_addr = (UINT32)&p_ctrl_info->info;
		evt.count = p_ctrl_info->kdf_evt_count;
		evt.rev = 0;
		if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
			ctl_ipp_info_release(p_ctrl_info);
			CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
			return CTL_IPP_E_QOVR;
		}
	} else if (p_hdl->flow == CTL_IPP_FLOW_CAPTURE_RAW) {
		kdf_qnum = 0;
		kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_FREE_QUEUE, (void *)&kdf_qnum);
		if (kdf_qnum < 2) {
			CTL_IPP_DBG_WRN("KDF msg queue overflow, free queue num = %d\r\n", (int)kdf_qnum);
			return CTL_IPP_E_QOVR;
		}

		/* Get ctl_ipp_info from pool */
		p_ctrl_info_arr[0] = ctl_ipp_info_alloc((UINT32)p_hdl);
		p_ctrl_info_arr[1] = ctl_ipp_info_alloc((UINT32)p_hdl);
		if (p_ctrl_info_arr[0] == NULL || p_ctrl_info_arr[1] == NULL) {
			CTL_IPP_DBG_WRN("ctl_info alloc overflow\r\n");
			if (p_ctrl_info_arr[0] != NULL) {
				ctl_ipp_info_release(p_ctrl_info_arr[0]);
			}
			return CTL_IPP_E_QOVR;
		}

		for (i = 0; i < 2; i++) {
			UINT32 j;

			p_ctrl_info = p_ctrl_info_arr[i];
			memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));

			if (i == 0) {
				for (j = 0; j < CTL_IPP_OUT_PATH_ID_MAX; j++) {
					if (p_ctrl_info->info.ime_ctrl.tplnr_enable && (j == p_ctrl_info->info.ime_ctrl.tplnr_in_ref_path)) {
						// do nothing. 3dnr ref path need output ref image, can not be disable
					} else {
						p_ctrl_info->info.ime_ctrl.out_img[j].dma_enable = DISABLE;
					}
				}
			}

			/* adjust flip to ime output, note that out flip can be set seperatly at other cmd */
			ctl_ipp_int_ime_flip(p_ctrl_info);

			p_ctrl_info->input_header_addr = header_addr;
			p_ctrl_info->input_buf_id = buf_id;
			p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
			p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
			p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;
			p_ctrl_info->evt_tag = (i == 0) ? CTL_IPP_EVT_TAG_SUBOUT_FOR_CAP : CTL_IPP_EVT_TAG_MAINFRM_FOR_CAP;


			/* Send KDF Event */
			evt.data_addr = (UINT32)&p_ctrl_info->info;
			evt.count = p_ctrl_info->kdf_evt_count;
			evt.rev = 0;
			if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
				ctl_ipp_info_release(p_ctrl_info);
				CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
				return CTL_IPP_E_QOVR;
			}
		}
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_process_ccir(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id)
{
	KDF_IPP_EVT evt;
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info_arr[2];
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	VDO_FRAME *p_vdoin_info;
	UINT32 i, kdf_qnum;
	INT32 rt;

	p_vdoin_info = (VDO_FRAME *)header_addr;

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_WRN("Ctrl handle 0x%.8x have no KDF handle\r\n", (unsigned int)p_hdl);
		return CTL_IPP_E_ID;
	}

	/* Check header info */
	if (p_vdoin_info->addr[0] == 0 ||
		p_vdoin_info->size.w == 0 ||
		p_vdoin_info->size.h == 0 ||
		p_vdoin_info->loff[0] == 0) {
		CTL_IPP_DBG_WRN("vdoin info error, addr 0x%.8x, size (%d, %d), loff %d\r\n",
			(unsigned int)p_vdoin_info->addr[0], (int)p_vdoin_info->size.w,(int) p_vdoin_info->size.h, (int)p_vdoin_info->loff[0]);
		return CTL_IPP_E_INDATA;
	}

	/* Update SIE header info to ctrl info */

	/* DCE Ctrl */
	{
		p_hdl->ctrl_info.dce_ctrl.in_fmt = p_vdoin_info->pxlfmt;
		p_hdl->ctrl_info.dce_ctrl.in_addr[0] = p_vdoin_info->addr[0];
		p_hdl->ctrl_info.dce_ctrl.in_addr[1] = p_vdoin_info->addr[1];
		p_hdl->ctrl_info.dce_ctrl.in_lofs[0] = p_vdoin_info->loff[0];
		p_hdl->ctrl_info.dce_ctrl.in_lofs[1] = p_vdoin_info->loff[1];
		p_hdl->ctrl_info.dce_ctrl.in_size.w = p_vdoin_info->size.w;
		p_hdl->ctrl_info.dce_ctrl.in_size.h = p_vdoin_info->size.h;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.x = 0;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.y = 0;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.w = p_hdl->ctrl_info.dce_ctrl.in_size.w;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.h = p_hdl->ctrl_info.dce_ctrl.in_size.h;
		p_hdl->ctrl_info.dce_ctrl.out_lofs[0] = p_hdl->ctrl_info.dce_ctrl.out_crp_window.w;
		p_hdl->ctrl_info.dce_ctrl.out_lofs[1] = ctl_ipp_util_y2uvlof(p_hdl->ctrl_info.dce_ctrl.in_fmt, p_hdl->ctrl_info.dce_ctrl.out_lofs[0]);
	}

	/* IPE Ctrl */
	{
		p_hdl->ctrl_info.ipe_ctrl.in_size.w = p_hdl->ctrl_info.dce_ctrl.out_crp_window.w;
		p_hdl->ctrl_info.ipe_ctrl.in_size.h = p_hdl->ctrl_info.dce_ctrl.out_crp_window.h;
		p_hdl->ctrl_info.ipe_ctrl.in_lofs[0] = p_hdl->ctrl_info.ipe_ctrl.in_size.w;
		p_hdl->ctrl_info.ipe_ctrl.in_lofs[1] = p_hdl->ctrl_info.ipe_ctrl.in_size.w;

		/* eth, check buffer size */
		ctl_ipp_isp_int_get(p_hdl, CTL_IPP_ISP_ITEM_ETH_PARAM, (void *)&p_hdl->ctrl_info.ipe_ctrl.eth);
		p_hdl->ctrl_info.ipe_ctrl.eth.out_size = p_hdl->ctrl_info.ipe_ctrl.in_size;
		if (p_hdl->ctrl_info.ipe_ctrl.eth.enable) {
			UINT32 eth_want_size;

			/* check orignal */
			eth_want_size = ctl_ipp_util_ethsize(p_hdl->ctrl_info.ipe_ctrl.in_size.w, p_hdl->ctrl_info.ipe_ctrl.in_size.h,
											p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel, DISABLE);

			if (p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_ORIGINAL ||
				p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_BOTH) {
				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0] < eth_want_size) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to buffer size 0x%.8x not enough, ipe img(%d %d), eth out_bit_sel %d, out_sel %d, want_size 0x%.8x\r\n",
						(unsigned int)p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0], (int)p_hdl->ctrl_info.ipe_ctrl.in_size.w, (int)p_hdl->ctrl_info.ipe_ctrl.in_size.h,
						(int)p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel, (int)p_hdl->ctrl_info.ipe_ctrl.eth.out_sel, (unsigned int)eth_want_size);
				}

				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_addr[0] == 0) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to addr 0\r\n");
				}
			}

			/* check downsample */
			eth_want_size = ctl_ipp_util_ethsize(p_hdl->ctrl_info.ipe_ctrl.in_size.w, p_hdl->ctrl_info.ipe_ctrl.in_size.h,
											p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel, ENABLE);

			if (p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE ||
				p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_BOTH) {
				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[1] < eth_want_size) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to buffer size 0x%.8x not enough, ipe img(%d %d), eth out_bit_sel %d, out_sel %d, want_size 0x%.8x\r\n",
						(unsigned int)p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0], (int)p_hdl->ctrl_info.ipe_ctrl.in_size.w, (int)p_hdl->ctrl_info.ipe_ctrl.in_size.h,
						(int)p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel, (int)p_hdl->ctrl_info.ipe_ctrl.eth.out_sel, (unsigned int)eth_want_size);
				}

				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_addr[1] == 0) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to addr 0\r\n");
				}
			}

			/* set frm cnt and timestamp */
			p_hdl->ctrl_info.ipe_ctrl.eth.frm_cnt = p_vdoin_info->count;
			p_hdl->ctrl_info.ipe_ctrl.eth.timestamp = p_vdoin_info->timestamp;
		}
	}

	/* IME Ctrl */
	{
		p_hdl->ctrl_info.ime_ctrl.in_size.w = p_hdl->ctrl_info.dce_ctrl.out_crp_window.w;
		p_hdl->ctrl_info.ime_ctrl.in_size.h = p_hdl->ctrl_info.dce_ctrl.out_crp_window.h;

		/* 3dnr sta info */
		if (p_hdl->ctrl_info.ime_ctrl.tplnr_enable == ENABLE) {
			ctl_ipp_isp_int_get(p_hdl, CTL_IPP_ISP_ITEM_3DNR_STA, (void *)&p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info);
			if (p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.enable == ENABLE) {
				if (p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.buf_addr == 0) {
					p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.enable = DISABLE;
					CTL_IPP_DBG_IND("3dnr sta disable due to addr 0\r\n");
				}
			}
		}

		/* set path6 size base on input */
		if (p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].enable == ENABLE) {
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.w = p_hdl->ctrl_info.ime_ctrl.in_size.w >> 1;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.h = p_hdl->ctrl_info.ime_ctrl.in_size.h >> 1;
			if (p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].fmt == VDO_PXLFMT_520_IR16) {
				p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = p_hdl->ctrl_info.ime_ctrl.in_size.w;
			} else {
				p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = p_hdl->ctrl_info.ime_ctrl.in_size.w >> 1;
			}
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = ALIGN_CEIL_4(p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0]);

			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.x = 0;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.y = 0;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.w = p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.w;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.h = p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.h;
		}
	}

	if (p_hdl->flow == CTL_IPP_FLOW_CCIR) {
		/* Get ctl_ipp_info from pool */
		p_ctrl_info = ctl_ipp_info_alloc((UINT32)p_hdl);
		if (p_ctrl_info == NULL) {
			return CTL_IPP_E_QOVR;
		}
		memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));

		/* DCE Ctrl */
		rt = ctl_ipp_int_dce_dummy_uv(p_ctrl_info);
		if (rt != CTL_IPP_E_OK) {
			return rt;
		}

		/* IME Ctrl */
		ctl_ipp_int_ime_flip(p_ctrl_info);

		p_ctrl_info->input_header_addr = header_addr;
		p_ctrl_info->input_buf_id = buf_id;
		p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
		p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
		p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;


		/* Send KDF Event */
		evt.data_addr = (UINT32)&p_ctrl_info->info;
		evt.count = p_ctrl_info->kdf_evt_count;
		evt.rev = 0;
		if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
			ctl_ipp_info_release(p_ctrl_info);
			CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
			return CTL_IPP_E_QOVR;
		}
	} else if (p_hdl->flow == CTL_IPP_FLOW_CAPTURE_CCIR) {
		kdf_qnum = 0;
		kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_FREE_QUEUE, (void *)&kdf_qnum);
		if (kdf_qnum < 2) {
			CTL_IPP_DBG_WRN("KDF msg queue overflow, free queue num = %d\r\n", (int)kdf_qnum);
			return CTL_IPP_E_QOVR;
		}

		/* Get ctl_ipp_info from pool */
		p_ctrl_info_arr[0] = ctl_ipp_info_alloc((UINT32)p_hdl);
		p_ctrl_info_arr[1] = ctl_ipp_info_alloc((UINT32)p_hdl);
		if (p_ctrl_info_arr[0] == NULL || p_ctrl_info_arr[1] == NULL) {
			CTL_IPP_DBG_WRN("ctl_info alloc overflow\r\n");
			if (p_ctrl_info_arr[0] != NULL) {
				ctl_ipp_info_release(p_ctrl_info_arr[0]);
			}
			return CTL_IPP_E_QOVR;
		}

		for (i = 0; i < 2; i++) {
			UINT32 j;

			p_ctrl_info = p_ctrl_info_arr[i];
			memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));

			if (i == 0) {
				for (j = 0; j < CTL_IPP_OUT_PATH_ID_MAX; j++) {
					if (p_ctrl_info->info.ime_ctrl.tplnr_enable && (j == p_ctrl_info->info.ime_ctrl.tplnr_in_ref_path)) {
						// do nothing. 3dnr ref path need output ref image, can not be disable
					} else {
						p_ctrl_info->info.ime_ctrl.out_img[j].dma_enable = DISABLE;
					}
				}
			}

			/* DCE Ctrl */
			rt = ctl_ipp_int_dce_dummy_uv(p_ctrl_info);
			if (rt != CTL_IPP_E_OK) {
				return rt;
			}

			/* adjust flip to ime output, note that out flip can be set seperatly at other cmd */
			ctl_ipp_int_ime_flip(p_ctrl_info);

			p_ctrl_info->input_header_addr = header_addr;
			p_ctrl_info->input_buf_id = buf_id;
			p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
			p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
			p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;
			p_ctrl_info->evt_tag = (i == 0) ? CTL_IPP_EVT_TAG_SUBOUT_FOR_CAP : CTL_IPP_EVT_TAG_MAINFRM_FOR_CAP;


			/* Send KDF Event */
			evt.data_addr = (UINT32)&p_ctrl_info->info;
			evt.count = p_ctrl_info->kdf_evt_count;
			evt.rev = 0;
			if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
				ctl_ipp_info_release(p_ctrl_info);
				CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
				return CTL_IPP_E_QOVR;
			}
		}
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_process_ime_d2d(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id)
{
	KDF_IPP_EVT evt;
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	VDO_FRAME *p_vdoin_info;
	UINT32 i;

	p_vdoin_info = (VDO_FRAME *)header_addr;

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_WRN("Ctrl handle 0x%.8x have no KDF handle\r\n", (unsigned int)p_hdl);
		return CTL_IPP_E_ID;
	}

	/* Check header info */
	if (p_vdoin_info->addr[0] == 0 ||
		p_vdoin_info->size.w == 0 ||
		p_vdoin_info->size.h == 0 ||
		p_vdoin_info->loff[0] == 0) {
		CTL_IPP_DBG_WRN("vdoin info error, addr 0x%.8x, size (%d, %d), loff %d\r\n",
			(unsigned int)p_vdoin_info->addr[0], (int)p_vdoin_info->size.w, (int)p_vdoin_info->size.h, (int)p_vdoin_info->loff[0]);
		return CTL_IPP_E_INDATA;
	}

	/* IME Ctrl */
	p_hdl->ctrl_info.ime_ctrl.in_addr[0] = p_vdoin_info->addr[0];
	p_hdl->ctrl_info.ime_ctrl.in_addr[1] = p_vdoin_info->addr[1];
	p_hdl->ctrl_info.ime_ctrl.in_addr[2] = p_vdoin_info->addr[2];
	p_hdl->ctrl_info.ime_ctrl.in_fmt = p_vdoin_info->pxlfmt;
	p_hdl->ctrl_info.ime_ctrl.in_size.w = (UINT32)p_vdoin_info->size.w;
	p_hdl->ctrl_info.ime_ctrl.in_size.h = (UINT32)p_vdoin_info->size.h;
	p_hdl->ctrl_info.ime_ctrl.in_lofs[0] = p_vdoin_info->loff[0];
	p_hdl->ctrl_info.ime_ctrl.in_lofs[1] = p_vdoin_info->loff[1];
	p_hdl->ctrl_info.ime_ctrl.in_lofs[2] = p_vdoin_info->loff[2];

	/* check output RGB */
	if (p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1].fmt == VDO_PXLFMT_RGB888_PLANAR) {
		for (i = CTL_IPP_OUT_PATH_ID_2; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			if (p_hdl->ctrl_info.ime_ctrl.out_img[i].enable == ENABLE) {
				CTL_IPP_DBG_WRN("force disable path%d, due to path1 output RGB\r\n", (int)(i));
				p_hdl->ctrl_info.ime_ctrl.out_img[i].enable = DISABLE;
			}
		}
	}

	/* 3dnr sta info */
	if (p_hdl->ctrl_info.ime_ctrl.tplnr_enable == ENABLE) {
		ctl_ipp_isp_int_get(p_hdl, CTL_IPP_ISP_ITEM_3DNR_STA, (void *)&p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info);
		if (p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.enable == ENABLE) {
			if (p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.buf_addr == 0) {
				p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.enable = DISABLE;
				CTL_IPP_DBG_IND("3dnr sta disable due to addr 0\r\n");
			}
		}
	}

	/* Get ctl_ipp_info from pool */
	p_ctrl_info = ctl_ipp_info_alloc((UINT32)p_hdl);
	if (p_ctrl_info == NULL) {
		return CTL_IPP_E_QOVR;
	}
	memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));

	/* IME Ctrl */
	ctl_ipp_int_ime_flip(p_ctrl_info);

	p_ctrl_info->input_header_addr = header_addr;
	p_ctrl_info->input_buf_id = buf_id;
	p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
	p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
	p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;

	/* Send KDF Event */
	evt.data_addr = (UINT32)&p_ctrl_info->info;
	evt.count = p_ctrl_info->kdf_evt_count;
	evt.rev = 0;
	if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
		ctl_ipp_info_release(p_ctrl_info);
		CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
		return CTL_IPP_E_QOVR;
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_process_ipe_d2d(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id)
{
	KDF_IPP_EVT evt;

	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	VDO_FRAME *p_vdoin_info;

	p_vdoin_info = (VDO_FRAME *)header_addr;

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_WRN("Ctrl handle 0x%.8x have no KDF handle\r\n", (unsigned int)p_hdl);
		return CTL_IPP_E_ID;
	}

	/* Check header info */
	if (p_vdoin_info->addr[0] == 0 ||
		p_vdoin_info->size.w == 0 ||
		p_vdoin_info->size.h == 0 ||
		p_vdoin_info->loff[0] == 0 ||
		p_vdoin_info->loff[1] == 0) {
		CTL_IPP_DBG_WRN("vdoin info error, addr 0x%.8x, size(%d, %d), loff(%d, %d)\r\n",
			(unsigned int)p_vdoin_info->addr[0], (int)p_vdoin_info->size.w, (int)p_vdoin_info->size.h, (int)p_vdoin_info->loff[0], (int)p_vdoin_info->loff[1]);
		return CTL_IPP_E_INDATA;
	}

	/* IPE Ctrl */
	p_hdl->ctrl_info.ipe_ctrl.in_addr[0] = p_vdoin_info->addr[0];
	p_hdl->ctrl_info.ipe_ctrl.in_addr[1] = p_vdoin_info->addr[1];
	p_hdl->ctrl_info.ipe_ctrl.in_lofs[0] = p_vdoin_info->loff[0];
	p_hdl->ctrl_info.ipe_ctrl.in_lofs[1] = p_vdoin_info->loff[1];
	p_hdl->ctrl_info.ipe_ctrl.in_fmt = p_vdoin_info->pxlfmt;
	p_hdl->ctrl_info.ipe_ctrl.in_size.w = p_vdoin_info->size.w;
	p_hdl->ctrl_info.ipe_ctrl.in_size.h = p_vdoin_info->size.h;

	p_hdl->ctrl_info.ipe_ctrl.out_enable = p_hdl->ctrl_info.ime_ctrl.out_img[0].enable;
	p_hdl->ctrl_info.ipe_ctrl.out_fmt = p_hdl->ctrl_info.ime_ctrl.out_img[0].fmt;

	if (p_hdl->ctrl_info.ipe_ctrl.out_enable != ENABLE) {
		CTL_IPP_DBG_WRN("IPE D2D not support path1 disable\r\n");
		return CTL_IPP_E_PAR;
	}

	if (p_hdl->ctrl_info.ime_ctrl.out_img[0].size.w != p_hdl->ctrl_info.ipe_ctrl.in_size.w ||
		p_hdl->ctrl_info.ime_ctrl.out_img[0].size.h != p_hdl->ctrl_info.ipe_ctrl.in_size.h) {
		CTL_IPP_DBG_WRN("IPE D2D not support scale, input(%d %d) output(%d %d)\r\n",
			(int)p_hdl->ctrl_info.ipe_ctrl.in_size.w, (int)p_hdl->ctrl_info.ipe_ctrl.in_size.h,
			(int)p_hdl->ctrl_info.ime_ctrl.out_img[0].size.w, (int)p_hdl->ctrl_info.ime_ctrl.out_img[0].size.h);
		return CTL_IPP_E_PAR;
	}

	/* Get ctl_ipp_info from pool */
	p_ctrl_info = ctl_ipp_info_alloc((UINT32)p_hdl);
	if (p_ctrl_info == NULL) {
		return CTL_IPP_E_QOVR;
	}
	memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));
	p_ctrl_info->input_header_addr = header_addr;
	p_ctrl_info->input_buf_id = buf_id;
	p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
	p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
	p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;

	/* Send KDF Event */
	evt.data_addr = (UINT32)&p_ctrl_info->info;
	evt.count = p_ctrl_info->kdf_evt_count;
	evt.rev = 0;
	if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
		ctl_ipp_info_release(p_ctrl_info);
		CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
		return CTL_IPP_E_QOVR;
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_process_dce_d2d(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id)
{
	KDF_IPP_EVT evt;
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	VDO_FRAME *p_vdoin_info;
	INT32 rt;

	p_vdoin_info = (VDO_FRAME *)header_addr;

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_WRN("Ctrl handle 0x%.8x have no KDF handle\r\n", (unsigned int)p_hdl);
		return CTL_IPP_E_ID;
	}

	/* Check header info */
	if (p_vdoin_info->addr[0] == 0 ||
		p_vdoin_info->size.w == 0 ||
		p_vdoin_info->size.h == 0 ||
		p_vdoin_info->loff[0] == 0 ||
		p_vdoin_info->loff[1] == 0) {
		CTL_IPP_DBG_WRN("vdoin info error, addr 0x%.8x, size(%d, %d), loff(%d, %d)\r\n",
			(unsigned int)p_vdoin_info->addr[0], (int)p_vdoin_info->size.w, (int)p_vdoin_info->size.h, (int)p_vdoin_info->loff[0], (int)p_vdoin_info->loff[1]);
		return CTL_IPP_E_INDATA;
	}

	if ((p_vdoin_info->size.w % 4) != 0 ||
		(p_vdoin_info->loff[0] % 4) != 0 ||
		(p_vdoin_info->loff[1] % 4) != 0) {
		CTL_IPP_DBG_WRN("dce d2d in width must be 4 align, size(%d, %d), loff(%d, %d)\r\n",
			(int)p_vdoin_info->size.w, (int)p_vdoin_info->size.h, (int)p_vdoin_info->loff[0], (int)p_vdoin_info->loff[1]);
		return CTL_IPP_E_INDATA;
	}

	/* DCE Ctrl */
	p_hdl->ctrl_info.dce_ctrl.in_fmt = p_vdoin_info->pxlfmt;
	p_hdl->ctrl_info.dce_ctrl.in_addr[0] = p_vdoin_info->addr[0];
	p_hdl->ctrl_info.dce_ctrl.in_addr[1] = p_vdoin_info->addr[1];
	p_hdl->ctrl_info.dce_ctrl.in_lofs[0] = p_vdoin_info->loff[0];
	p_hdl->ctrl_info.dce_ctrl.in_lofs[1] = p_vdoin_info->loff[1];
	p_hdl->ctrl_info.dce_ctrl.in_size.w = p_vdoin_info->size.w;
	p_hdl->ctrl_info.dce_ctrl.in_size.h = p_vdoin_info->size.h;
	p_hdl->ctrl_info.dce_ctrl.out_crp_window.x = 0;
	p_hdl->ctrl_info.dce_ctrl.out_crp_window.y = 0;
	p_hdl->ctrl_info.dce_ctrl.out_crp_window.w = p_hdl->ctrl_info.dce_ctrl.in_size.w;
	p_hdl->ctrl_info.dce_ctrl.out_crp_window.h = p_hdl->ctrl_info.dce_ctrl.in_size.h;
	p_hdl->ctrl_info.dce_ctrl.out_lofs[0] = p_hdl->ctrl_info.dce_ctrl.out_crp_window.w;
	p_hdl->ctrl_info.dce_ctrl.out_lofs[1] = ctl_ipp_util_y2uvlof(p_hdl->ctrl_info.dce_ctrl.in_fmt, p_hdl->ctrl_info.dce_ctrl.out_lofs[0]);

	if (p_hdl->ctrl_info.ime_ctrl.out_img[0].enable != ENABLE) {
		CTL_IPP_DBG_WRN("DCE D2D not support path1 disable\r\n");
		return CTL_IPP_E_PAR;
	}

	if (p_hdl->ctrl_info.ime_ctrl.out_img[0].fmt != p_hdl->ctrl_info.dce_ctrl.in_fmt) {
		CTL_IPP_DBG_WRN("DCE D2D output format(0x%.8x) must be same as input format(0x%.8x)\r\n",
			(unsigned int)p_hdl->ctrl_info.ime_ctrl.out_img[0].fmt, (unsigned int)p_hdl->ctrl_info.dce_ctrl.in_fmt);
		return CTL_IPP_E_PAR;
	}

	if (p_hdl->ctrl_info.ime_ctrl.out_img[0].size.w != p_hdl->ctrl_info.dce_ctrl.in_size.w ||
		p_hdl->ctrl_info.ime_ctrl.out_img[0].size.h != p_hdl->ctrl_info.dce_ctrl.in_size.h) {
		CTL_IPP_DBG_WRN("DCE D2D not support scale, input(%d %d) output(%d %d)\r\n",
			(int)p_hdl->ctrl_info.ipe_ctrl.in_size.w, (int)p_hdl->ctrl_info.ipe_ctrl.in_size.h,
			(int)p_hdl->ctrl_info.ime_ctrl.out_img[0].size.w, (int)p_hdl->ctrl_info.ime_ctrl.out_img[0].size.h);
		return CTL_IPP_E_PAR;
	}

	/* Get ctl_ipp_info from pool */
	p_ctrl_info = ctl_ipp_info_alloc((UINT32)p_hdl);
	if (p_ctrl_info == NULL) {
		return CTL_IPP_E_QOVR;
	}
	memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));

	/* special case, set lofs[1] = 0, addr[1] as dummy uv line buffer for input image only has one plane */
	rt = ctl_ipp_int_dce_dummy_uv(p_ctrl_info);
	if (rt != CTL_IPP_E_OK) {
		return rt;
	}

	p_ctrl_info->input_header_addr = header_addr;
	p_ctrl_info->input_buf_id = buf_id;
	p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
	p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
	p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;

	/* Send KDF Event */
	evt.data_addr = (UINT32)&p_ctrl_info->info;
	evt.count = p_ctrl_info->kdf_evt_count;
	evt.rev = 0;
	if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
		ctl_ipp_info_release(p_ctrl_info);
		CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
		return CTL_IPP_E_QOVR;
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_process_vr360(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id)
{
#if 0
	KDF_IPP_EVT evt;

	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	VDO_FRAME *p_vdoin_info;
	VDO_PXLFMT pix_fmt;
	UINT32 i, queue_num = 0;

	p_vdoin_info = (VDO_FRAME *)header_addr;

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_WRN("Ctrl handle 0x%.8x have no KDF handle\r\n", (UINT32)p_hdl);
		return CTL_IPP_E_ID;
	}

	/* Check header info */
	if (p_vdoin_info->addr[0] == 0 ||
		p_vdoin_info->size.w == 0 ||
		p_vdoin_info->size.h == 0 ||
		p_vdoin_info->loff[0] == 0) {
		CTL_IPP_DBG_WRN("vdoin info error, addr 0x%.8x, size (%d, %d), loff %d\r\n",
			p_vdoin_info->addr[0], p_vdoin_info->size.w, p_vdoin_info->size.h, p_vdoin_info->loff[0]);
		return CTL_IPP_E_INDATA;
	}

	for (i = 0; i < 2; i++) {
		/* Update SIE header info to ctrl info */
		/* RHE Ctrl */
		{
			UINT32 frm_num;

			p_hdl->ctrl_info.rhe_ctrl.in_addr[0] = p_vdoin_info->addr[0];
			p_hdl->ctrl_info.rhe_ctrl.in_size.w = (UINT32)p_vdoin_info->size.w;
			p_hdl->ctrl_info.rhe_ctrl.in_size.h = (UINT32)p_vdoin_info->size.h;
			p_hdl->ctrl_info.rhe_ctrl.in_lofs = p_vdoin_info->loff[0];
			p_hdl->ctrl_info.rhe_ctrl.in_fmt = p_vdoin_info->pxlfmt;

			/* sub input */
			p_hdl->ctrl_info.rhe_ctrl.subin_addr[0] = p_vdoin_info->reserved[2];
			p_hdl->ctrl_info.rhe_ctrl.subin_img_size.h = p_vdoin_info->reserved[7] & 0xff;
			p_hdl->ctrl_info.rhe_ctrl.subin_img_size.w = (p_vdoin_info->reserved[7] >> 8) & 0xff;
			p_hdl->ctrl_info.rhe_ctrl.subin_blk_size.h = (p_vdoin_info->reserved[7] >> 16) & 0xff;
			p_hdl->ctrl_info.rhe_ctrl.subin_blk_size.w = (p_vdoin_info->reserved[7] >> 24) & 0xff;
			p_hdl->ctrl_info.rhe_ctrl.subin_lofs = p_vdoin_info->loff[3];

			/* crop window */
			if (p_hdl->ctrl_info.rhe_ctrl.in_crp_mode == CTL_IPP_IN_CROP_AUTO) {
				/* crop window from sie */
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.x = (p_vdoin_info->reserved[5] & 0xffff0000) >> 16;
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.y = (p_vdoin_info->reserved[5] & 0x0000ffff);
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w = (p_vdoin_info->reserved[6] & 0xffff0000) >> 16;
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h = (p_vdoin_info->reserved[6] & 0x0000ffff);
			} else if (p_hdl->ctrl_info.rhe_ctrl.in_crp_mode == CTL_IPP_IN_CROP_NONE) {
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.x = 0;
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.y = 0;
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w = p_hdl->ctrl_info.rhe_ctrl.in_size.w;
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h = p_hdl->ctrl_info.rhe_ctrl.in_size.h;
			}

			if ((p_hdl->ctrl_info.rhe_ctrl.in_crp_window.x + p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w > p_hdl->ctrl_info.rhe_ctrl.in_size.w) ||
				(p_hdl->ctrl_info.rhe_ctrl.in_crp_window.y + p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h > p_hdl->ctrl_info.rhe_ctrl.in_size.h) ||
				(p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w == 0) ||
				(p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h == 0)) {
				CTL_IPP_DBG_WRN("crop window(%d %d %d %d) error, in_size(%d %d)\r\n",
					p_hdl->ctrl_info.rhe_ctrl.in_crp_window.x, p_hdl->ctrl_info.rhe_ctrl.in_crp_window.y,
					p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w, p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h,
					p_hdl->ctrl_info.rhe_ctrl.in_size.w, p_hdl->ctrl_info.rhe_ctrl.in_size.h);

				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.x = 0;
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.y = 0;
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w = p_hdl->ctrl_info.rhe_ctrl.in_size.w;
				p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h = p_hdl->ctrl_info.rhe_ctrl.in_size.h;
			}

			/* raw decode */
			if (VDO_PXLFMT_CLASS(p_vdoin_info->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
				if (p_vdoin_info->reserved[4] == 0) {
					p_hdl->ctrl_info.rhe_ctrl.decode_enable = DISABLE;
					CTL_IPP_DBG_WRN("FMT = 0x%.8x, but side_info address = 0\r\n", p_vdoin_info->pxlfmt);
					return CTL_IPP_E_INDATA;
				}
				p_hdl->ctrl_info.rhe_ctrl.decode_enable = ENABLE;
				p_hdl->ctrl_info.rhe_ctrl.decode_si_addr[0] = p_vdoin_info->reserved[4];
			} else {
				p_hdl->ctrl_info.rhe_ctrl.decode_enable = DISABLE;
			}

			/* SHDR	*/
			frm_num = VDO_PXLFMT_PLANE(p_vdoin_info->pxlfmt);
			if (frm_num > 1) {
				UINT32 i;
				VDO_FRAME *p_vdoin_tmp;

				for (i = 1; i < frm_num; i++) {
					if (p_vdoin_info->addr[i] != 0) {
						p_vdoin_tmp = (VDO_FRAME *)p_vdoin_info->addr[i];

						p_hdl->ctrl_info.rhe_ctrl.in_addr[i] = p_vdoin_tmp->addr[0];
						p_hdl->ctrl_info.rhe_ctrl.subin_addr[i] = p_vdoin_tmp->reserved[2];
						p_hdl->ctrl_info.rhe_ctrl.decode_si_addr[i] = p_vdoin_tmp->reserved[4];
					} else {
						CTL_IPP_DBG_WRN("PXLFMT 0x%.8x, frame_number = %d, addr[%d] = 0\r\n",
							p_vdoin_info->pxlfmt, frm_num, i);
						return CTL_IPP_E_INDATA;
					}
				}
			}
		}

		/* change pixel start by flip */
		pix_fmt = (VDO_PXLFMT) ctl_ipp_info_pxlfmt_by_flip((UINT32) p_vdoin_info->pxlfmt, p_hdl->ctrl_info.rhe_ctrl.flip);

		/* IFE Ctrl */
		{
			p_hdl->ctrl_info.ife_ctrl.in_size.w = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w;
			p_hdl->ctrl_info.ife_ctrl.in_size.h = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h;
			p_hdl->ctrl_info.ife_ctrl.in_fmt = pix_fmt;
		}

		/* DCE Ctrl */
		{
			p_hdl->ctrl_info.dce_ctrl.in_fmt = pix_fmt;
			p_hdl->ctrl_info.dce_ctrl.in_size.w = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w;
			p_hdl->ctrl_info.dce_ctrl.in_size.h = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h;
			p_hdl->ctrl_info.dce_ctrl.out_crp_window.x = 0;
			p_hdl->ctrl_info.dce_ctrl.out_crp_window.y = 0;
			p_hdl->ctrl_info.dce_ctrl.out_crp_window.w = p_hdl->ctrl_info.dce_ctrl.in_size.w;
			p_hdl->ctrl_info.dce_ctrl.out_crp_window.h = p_hdl->ctrl_info.dce_ctrl.in_size.h;
		}

		/* IPE Ctrl */
		{
			p_hdl->ctrl_info.ipe_ctrl.in_size.w = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w;
			p_hdl->ctrl_info.ipe_ctrl.in_size.h = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h;
		}

		/* IME Ctrl */
		{
			p_hdl->ctrl_info.ime_ctrl.in_size.w = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.w;
			p_hdl->ctrl_info.ime_ctrl.in_size.h = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h;

			if (p_hdl->ctrl_info.ime_ctrl.tplnr_enable == ENABLE) {
				p_hdl->ctrl_info.ime_ctrl.tplnr_out_mo_lofs = ALIGN_CEIL((p_hdl->ctrl_info.ime_ctrl.in_size.w >> 2), 4);
			}
		}

		/* Get ctl_ipp_info from pool */
		p_ctrl_info = ctl_ipp_info_alloc((UINT32)p_hdl);
		if (p_ctrl_info == NULL) {
			return CTL_IPP_E_QOVR;
		}
		memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));

		if (i == 0) {
			/* for top half */
			p_ctrl_info->info.rhe_ctrl.in_crp_window.h = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h / 2;
		} else {
			/* for bottom half */
			p_ctrl_info->info.rhe_ctrl.in_crp_window.h = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.h / 2;
			p_ctrl_info->info.rhe_ctrl.in_crp_window.y = p_hdl->ctrl_info.rhe_ctrl.in_crp_window.y + p_ctrl_info->info.rhe_ctrl.in_crp_window.h;
			p_ctrl_info->info.rhe_ctrl.flip = CTL_IPP_FLIP_H_V;
			/* change pixel start by flip */
			pix_fmt = (VDO_PXLFMT) ctl_ipp_info_pxlfmt_by_flip((UINT32) p_vdoin_info->pxlfmt, p_ctrl_info->info.rhe_ctrl.flip);
			p_ctrl_info->info.ife_ctrl.in_fmt = pix_fmt;
			p_ctrl_info->info.dce_ctrl.in_fmt = pix_fmt;
		}

		/* change engine input size height */
		p_ctrl_info->info.ife_ctrl.in_size.h = p_ctrl_info->info.rhe_ctrl.in_crp_window.h;
		p_ctrl_info->info.dce_ctrl.in_size.h = p_ctrl_info->info.rhe_ctrl.in_crp_window.h;
		p_ctrl_info->info.dce_ctrl.out_crp_window.h = p_ctrl_info->info.dce_ctrl.in_size.h;
		p_ctrl_info->info.ipe_ctrl.in_size.h = p_ctrl_info->info.rhe_ctrl.in_crp_window.h;
		p_ctrl_info->info.ime_ctrl.in_size.h = p_ctrl_info->info.rhe_ctrl.in_crp_window.h;

		p_ctrl_info->input_header_addr = header_addr;
		p_ctrl_info->input_buf_id = buf_id;
		p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
		p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;

		/* check KDF msg queue number*/
		kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_FREE_QUEUE, (void *)&queue_num);
		if (i == 0 && queue_num < 2) {
			CTL_IPP_DBG_WRN("KDF msg queue overflow %d\r\n", (int)(queue_num));
			ctl_ipp_info_release(p_ctrl_info);
			return CTL_IPP_E_QOVR;
		}

		/* Send KDF Event */
		evt.data_addr = (UINT32)&p_ctrl_info->info;
		evt.rev = 0;
		if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT, (void *)&evt) != CTL_IPP_E_OK) {
			ctl_ipp_info_release(p_ctrl_info);
			CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
			return CTL_IPP_E_QOVR;
		}
	}
#endif
	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_process_direct(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id)
{

	KDF_IPP_EVT evt;
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	VDO_FRAME *p_vdoin_info;
	VDO_PXLFMT pix_fmt;
	INT32 rt = CTL_IPP_E_OK;
	UINT32 i;
	unsigned long loc_cpu_flg;
	INT32 max_stripe_w;

	if (ctl_ipp_direct_get_tsk_en() == 0) { // rev_evt_time is configured in ctl_ipp_process when direct task mode
		p_hdl->rev_evt_time = ctl_ipp_util_get_syst_counter();
	}
	p_vdoin_info = (VDO_FRAME *)header_addr;

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "Ctrl handle 0x%.8x have no KDF handle\r\n", (unsigned int)p_hdl);
		return CTL_IPP_E_ID;
	}

	/* Check header info */
	if (p_vdoin_info->size.w == 0 ||
		p_vdoin_info->size.h == 0 ||
		p_vdoin_info->loff[0] == 0) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "vdoin info error,size (%d, %d), loff %d\r\n",
			(int)p_vdoin_info->size.w, (int)p_vdoin_info->size.h, (int)p_vdoin_info->loff[0]);
		return CTL_IPP_E_INDATA;
	}

	/* check direct max surpport size */
	max_stripe_w = ctl_ipp_util_get_max_stripe();
	if (p_vdoin_info->size.w > max_stripe_w) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "direct don't surpport width over %d(%d)\r\n", (int)(max_stripe_w), (int)(p_vdoin_info->size.w));
		return CTL_IPP_E_INDATA;
	}

	/* Get ctl_ipp_info from pool */
	p_ctrl_info = ctl_ipp_info_alloc((UINT32)p_hdl);
	vk_spin_lock_irqsave(&p_hdl->spinlock, loc_cpu_flg);
	/* Update SIE header info to ctrl info */
	/* IFE Ctrl */
	{
		UINT32 frm_num;

		p_hdl->ctrl_info.ife_ctrl.in_addr[0] = 0;
		p_hdl->ctrl_info.ife_ctrl.in_lofs[0] = 0;
		p_hdl->ctrl_info.ife_ctrl.in_size.w = (UINT32)p_vdoin_info->size.w;
		p_hdl->ctrl_info.ife_ctrl.in_size.h = (UINT32)p_vdoin_info->size.h;
		/* direct mode don't surpport crop */
		p_hdl->ctrl_info.ife_ctrl.in_crp_window.x = 0;
		p_hdl->ctrl_info.ife_ctrl.in_crp_window.y = 0;
		p_hdl->ctrl_info.ife_ctrl.in_crp_window.w = p_hdl->ctrl_info.ife_ctrl.in_size.w;
		p_hdl->ctrl_info.ife_ctrl.in_crp_window.h = p_hdl->ctrl_info.ife_ctrl.in_size.h;
		p_hdl->ctrl_info.ife_ctrl.in_fmt = p_vdoin_info->pxlfmt;

		/* SHDR	*/
		if (p_hdl->func_en & CTL_IPP_FUNC_SHDR) {
			/* ife check mode info store in sie1 header. only hdr mode need grab dram data, so don't care in linear mode */
			if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START) {
				/* update to both rtc_info and ctrl_info prevent set_apply cause error */
				p_hdl->rtc_info.ife_ctrl.hdr_ref_chk_bit |= (CTL_IPP_VDOFRM_RES_MSK_IFECHK(p_vdoin_info->reserved[4]) << 0);
				p_hdl->ctrl_info.ife_ctrl.hdr_ref_chk_bit |= (CTL_IPP_VDOFRM_RES_MSK_IFECHK(p_vdoin_info->reserved[4]) << 0);
			}

			frm_num = VDO_PXLFMT_PLANE(p_vdoin_info->pxlfmt);
			if (frm_num > 1) {
				UINT32 i;
				VDO_FRAME *p_vdoin_tmp;

				for (i = 1; i < frm_num; i++) {
					if (i >= 2) {
						CTL_IPP_DBG_WRN("only support 2 frame hdr, input format 0x%.8x\r\n", (unsigned int)p_vdoin_info->pxlfmt);
						break;
					}
					if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START) {
						if (p_vdoin_info->addr[i] != 0) {
							p_vdoin_tmp = (VDO_FRAME *)p_vdoin_info->addr[i];

							if (p_vdoin_tmp->addr[0] == 0) {
								CTL_IPP_DBG_WRN("PXLFMT 0x%.8x, frame_number = %d, addr[%d] = 0\r\n",
									(unsigned int)p_vdoin_tmp->pxlfmt, (int)frm_num, (int)i);
								vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);
								return CTL_IPP_E_INDATA;
							}

							if (p_vdoin_tmp->size.w != (INT32)p_hdl->ctrl_info.ife_ctrl.in_size.w ||
								p_vdoin_tmp->size.h != (INT32)p_hdl->ctrl_info.ife_ctrl.in_size.h) {
								CTL_IPP_DBG_WRN("PXLFMT 0x%.8x, frame_number = %d, size[%d] (%d %d) not match to first frame(%d %d)\r\n",
									(unsigned int)p_vdoin_info->pxlfmt, (int)frm_num, (int)i, (int)p_vdoin_tmp->size.w, (int)p_vdoin_tmp->size.h,
									(int)p_hdl->ctrl_info.ife_ctrl.in_size.w, (int)p_hdl->ctrl_info.ife_ctrl.in_size.h);
								vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);
								return CTL_IPP_E_INDATA;
							}

							/* update to both rtc_info and ctrl_info prevent set_apply cause error */
							p_hdl->rtc_info.ife_ctrl.in_addr[i] = p_vdoin_tmp->addr[0];
							p_hdl->rtc_info.ife_ctrl.in_lofs[i] = p_vdoin_tmp->loff[0];

							if(CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_tmp->reserved[4]) == CTL_IPP_SIE_ID_2) {
								p_hdl->rtc_info.ife_ctrl.dir_buf_num = p_vdoin_tmp->reserved[5];
								p_hdl->rtc_info.ife_ctrl.in_fmt = p_vdoin_info->pxlfmt;
								/* raw decode */
								if (VDO_PXLFMT_CLASS(p_vdoin_tmp->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
									p_hdl->rtc_info.ife_ctrl.decode_enable = ENABLE;
									p_hdl->rtc_info.ife_ctrl.decode_ratio = p_vdoin_tmp->reserved[7];
								} else {
									p_hdl->rtc_info.ife_ctrl.decode_enable = DISABLE;
								}
							}

							/* update ife check reference sie info */
							p_hdl->rtc_info.ife_ctrl.hdr_ref_chk_bit |= (CTL_IPP_VDOFRM_RES_MSK_IFECHK(p_vdoin_tmp->reserved[4]) << i);

							p_hdl->ctrl_info.ife_ctrl.in_addr[i] = p_vdoin_tmp->addr[0];
							p_hdl->ctrl_info.ife_ctrl.in_lofs[i] = p_vdoin_tmp->loff[0];

							if(CTL_IPP_VDOFRM_RES_MSK_SIEID(p_vdoin_tmp->reserved[4]) == CTL_IPP_SIE_ID_2) {
								p_hdl->ctrl_info.ife_ctrl.dir_buf_num = p_vdoin_tmp->reserved[5];
								p_hdl->ctrl_info.ife_ctrl.in_fmt = p_vdoin_info->pxlfmt;
								/* raw decode */
								if (VDO_PXLFMT_CLASS(p_vdoin_tmp->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
									p_hdl->ctrl_info.ife_ctrl.decode_enable = ENABLE;
									p_hdl->ctrl_info.ife_ctrl.decode_ratio = p_vdoin_tmp->reserved[7];
								} else {
									p_hdl->ctrl_info.ife_ctrl.decode_enable = DISABLE;
								}
							}

							/* update ife check reference sie info */
							p_hdl->ctrl_info.ife_ctrl.hdr_ref_chk_bit |= (CTL_IPP_VDOFRM_RES_MSK_IFECHK(p_vdoin_tmp->reserved[4]) << i);

							//DBG_DUMP("start %d %d %d %d \r\n",i, p_hdl->ctrl_info.ife_ctrl.decode_enable, p_hdl->ctrl_info.ife_ctrl.decode_ratio, VDO_PXLFMT_CLASS(p_vdoin_tmp->pxlfmt));
						} else {
							CTL_IPP_DBG_WRN("PXLFMT 0x%.8x, frame_number = %d, vdofrm_addr[%d] = 0\r\n",
								(unsigned int)p_vdoin_info->pxlfmt, (int)frm_num, (int)i);
							vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);
							return CTL_IPP_E_INDATA;
						}
					} else {
							p_hdl->ctrl_info.ife_ctrl.in_addr[i] = p_vdoin_info->addr[0];
							p_hdl->ctrl_info.ife_ctrl.in_lofs[i] = p_vdoin_info->loff[0];
							p_hdl->ctrl_info.ife_ctrl.dir_buf_num = p_vdoin_info->reserved[5];
							/* raw decode */
							if (VDO_PXLFMT_CLASS(p_vdoin_info->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
								p_hdl->ctrl_info.ife_ctrl.decode_enable = ENABLE;
								p_hdl->ctrl_info.ife_ctrl.decode_ratio = p_vdoin_info->reserved[7];
							} else {
								p_hdl->ctrl_info.ife_ctrl.decode_enable = DISABLE;
							}
							//DBG_DUMP(" %d %d %d %d \r\n",i, p_hdl->ctrl_info.ife_ctrl.decode_enable, p_hdl->ctrl_info.ife_ctrl.decode_ratio, VDO_PXLFMT_CLASS(p_vdoin_info->pxlfmt));
					}
				}
			}
		}
	}

	/* DCE Ctrl */
	{
		p_hdl->ctrl_info.dce_ctrl.in_fmt = p_vdoin_info->pxlfmt;
		p_hdl->ctrl_info.dce_ctrl.in_size.w = p_hdl->ctrl_info.ife_ctrl.in_crp_window.w;
		p_hdl->ctrl_info.dce_ctrl.in_size.h = p_hdl->ctrl_info.ife_ctrl.in_crp_window.h;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.x = 0;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.y = 0;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.w = p_hdl->ctrl_info.dce_ctrl.in_size.w;
		p_hdl->ctrl_info.dce_ctrl.out_crp_window.h = p_hdl->ctrl_info.dce_ctrl.in_size.h;
	}

	/* IPE Ctrl */
	{
		p_hdl->ctrl_info.ipe_ctrl.in_size.w = p_hdl->ctrl_info.ife_ctrl.in_crp_window.w;
		p_hdl->ctrl_info.ipe_ctrl.in_size.h = p_hdl->ctrl_info.ife_ctrl.in_crp_window.h;
		p_hdl->ctrl_info.ipe_ctrl.in_lofs[0] = p_hdl->ctrl_info.ipe_ctrl.in_size.w;
		p_hdl->ctrl_info.ipe_ctrl.in_lofs[1] = p_hdl->ctrl_info.ipe_ctrl.in_size.w;

		/* eth, check buffer size */
		ctl_ipp_isp_int_get(p_hdl, CTL_IPP_ISP_ITEM_ETH_PARAM, (void *)&p_hdl->ctrl_info.ipe_ctrl.eth);
		p_hdl->ctrl_info.ipe_ctrl.eth.out_size = p_hdl->ctrl_info.ipe_ctrl.in_size;
		if (p_hdl->ctrl_info.ipe_ctrl.eth.enable) {
			UINT32 eth_want_size;

			/* check orignal */
			eth_want_size = ctl_ipp_util_ethsize(p_hdl->ctrl_info.ipe_ctrl.in_size.w, p_hdl->ctrl_info.ipe_ctrl.in_size.h,
											p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel, DISABLE);

			if (p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_ORIGINAL ||
				p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_BOTH) {
				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0] < eth_want_size) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to buffer size 0x%.8x not enough, ipe img(%d %d), eth out_bit_sel %d, out_sel %d, want_size 0x%.8x\r\n",
						(unsigned int)p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0], (int)p_hdl->ctrl_info.ipe_ctrl.in_size.w, (int)p_hdl->ctrl_info.ipe_ctrl.in_size.h,
						(int)p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel,(int)p_hdl->ctrl_info.ipe_ctrl.eth.out_sel, (unsigned int)eth_want_size);
				}

				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_addr[0] == 0) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to addr 0\r\n");
				}
			}

			/* check downsample */
			eth_want_size = ctl_ipp_util_ethsize(p_hdl->ctrl_info.ipe_ctrl.in_size.w, p_hdl->ctrl_info.ipe_ctrl.in_size.h,
											p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel, ENABLE);

			if (p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE ||
				p_hdl->ctrl_info.ipe_ctrl.eth.out_sel == CTL_IPP_ISP_ETH_OUT_BOTH) {
				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[1] < eth_want_size) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to buffer size 0x%.8x not enough, ipe img(%d %d), eth out_bit_sel %d, out_sel %d, want_size 0x%.8x\r\n",
						(unsigned int)p_hdl->ctrl_info.ipe_ctrl.eth.buf_size[0], (int)p_hdl->ctrl_info.ipe_ctrl.in_size.w, (int)p_hdl->ctrl_info.ipe_ctrl.in_size.h,
						(int)p_hdl->ctrl_info.ipe_ctrl.eth.out_bit_sel,(int) p_hdl->ctrl_info.ipe_ctrl.eth.out_sel, (unsigned int)eth_want_size);
				}

				if (p_hdl->ctrl_info.ipe_ctrl.eth.buf_addr[1] == 0) {
					p_hdl->ctrl_info.ipe_ctrl.eth.enable = DISABLE;
					CTL_IPP_DBG_IND("eth disable due to addr 0\r\n");
				}
			}

			/* set frm cnt and timestamp */
			p_hdl->ctrl_info.ipe_ctrl.eth.frm_cnt = p_vdoin_info->count;
			p_hdl->ctrl_info.ipe_ctrl.eth.timestamp = p_vdoin_info->timestamp;
		}
	}

	/* IME Ctrl */
	{
		p_hdl->ctrl_info.ime_ctrl.in_size.w = p_hdl->ctrl_info.ife_ctrl.in_crp_window.w;
		p_hdl->ctrl_info.ime_ctrl.in_size.h = p_hdl->ctrl_info.ife_ctrl.in_crp_window.h;

		/* 3dnr sta info */
		if (p_hdl->ctrl_info.ime_ctrl.tplnr_enable == ENABLE) {
			ctl_ipp_isp_int_get(p_hdl, CTL_IPP_ISP_ITEM_3DNR_STA, (void *)&p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info);
			if (p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.enable == ENABLE) {
				if (p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.buf_addr == 0) {
					p_hdl->ctrl_info.ime_ctrl.tplnr_sta_info.enable = DISABLE;
					CTL_IPP_DBG_IND("3dnr sta disable due to addr 0\r\n");
				}
			}
		}

		/* set path6 size base on input */
		if (p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].enable == ENABLE) {
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.w = p_hdl->ctrl_info.ime_ctrl.in_size.w >> 1;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.h = p_hdl->ctrl_info.ime_ctrl.in_size.h >> 1;
			if (p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].fmt == VDO_PXLFMT_520_IR16) {
				p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = p_hdl->ctrl_info.ime_ctrl.in_size.w;
			} else {
				p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = p_hdl->ctrl_info.ime_ctrl.in_size.w >> 1;
			}
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0] = ALIGN_CEIL_4(p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].lofs[0]);

			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.x = 0;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.y = 0;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.w = p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.w;
			p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].crp_window.h = p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6].size.h;
		}
	}

	if (p_ctrl_info == NULL) {
		vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);
		CTL_IPP_DBG_WRN("null\r\n");
		return CTL_IPP_E_QOVR;
	}
	memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));
	vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);


	/* in shdr case, sie is not synchronize in open, ipp shoule skip open data */
	#if defined (__KERNEL__)
		if (kdrv_builtin_is_fastboot() && p_hdl->is_first_handle) {
			/* skip isp callback for direct mode first frame from fastboot */
			if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START) {
				p_ctrl_info->evt_tag = CTL_IPP_EVT_TAG_SKIP_ISP_FOR_FASTBOOT;
			}
		} else {
			if ((p_hdl->func_en & CTL_IPP_FUNC_SHDR) && (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START)) {
				for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
					p_ctrl_info->info.ime_ctrl.out_img[i].dma_enable= DISABLE;
				}
			}

		}
	#else
		if ((p_hdl->func_en & CTL_IPP_FUNC_SHDR) && (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START)) {
			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				p_ctrl_info->info.ime_ctrl.out_img[i].dma_enable= DISABLE;
			}
		}
	#endif

	/* change pixel start by flip
		ife only support mirror, flip is done at ime output
	*/
	pix_fmt = (VDO_PXLFMT) ctl_ipp_info_pxlfmt_by_flip((UINT32) p_vdoin_info->pxlfmt, (p_hdl->ctrl_info.ife_ctrl.flip & CTL_IPP_FLIP_H));
	p_ctrl_info->info.dce_ctrl.in_fmt = pix_fmt;

	if (p_hdl->func_en & CTL_IPP_FUNC_DIRECT_SCL_UP) {
	} else {
		/* check ime path scale up */
		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			if (p_ctrl_info->info.ime_ctrl.out_img[i].enable == ENABLE) {
				if ((p_ctrl_info->info.ime_ctrl.in_size.w < p_ctrl_info->info.ime_ctrl.out_img[i].size.w) ||
					p_ctrl_info->info.ime_ctrl.in_size.h < p_ctrl_info->info.ime_ctrl.out_img[i].size.h) {
					CTL_IPP_DBG_WRN("ime path(%d) dont surpport scale up, in(%d,%d) out(%d %d)\r\n", (int)(i + 1),
						(int)p_ctrl_info->info.ime_ctrl.in_size.w, (int)p_ctrl_info->info.ime_ctrl.in_size.h,
						(int)p_ctrl_info->info.ime_ctrl.out_img[i].size.w, (int)p_ctrl_info->info.ime_ctrl.out_img[i].size.h);

					p_ctrl_info->info.ime_ctrl.out_img[i].dma_enable = FALSE;
				}
			}
		}
	}

	/* when direct mode stop, disable 3dnr & ime output dma channel */
	if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S1) {
		p_ctrl_info->info.ime_ctrl.tplnr_enable = DISABLE;
		for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			p_ctrl_info->info.ime_ctrl.out_img[i].dma_enable = FALSE;
		}
	}

	/* set flip to ime output, note that out flip can be set seperatly at other cmd */
	ctl_ipp_int_ime_flip(p_ctrl_info);

	/* GET IME LCA SUBOUT SIZE */
	{
		CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_size_ratio;
		KDF_IPP_ISP_CB_INFO iqcb_info;

		memset((void *)&lca_size_ratio, 0, sizeof(CTL_IPP_ISP_IME_LCA_SIZE_RATIO));
		iqcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_GET;
		iqcb_info.item = CTL_IPP_ISP_ITEM_IME_LCA_SIZE;
		iqcb_info.data_addr = (UINT32)&lca_size_ratio;
		ctl_ipp_isp_cb((UINT32)p_hdl->kdf_hdl, (void *)&iqcb_info, NULL);

		p_ctrl_info->info.ime_ctrl.lca_out_size = ctl_ipp_util_lca_subout_size(p_ctrl_info->info.ime_ctrl.in_size.w, p_ctrl_info->info.ime_ctrl.in_size.h, lca_size_ratio, 0xFFFFFFFF);
		p_ctrl_info->info.ime_ctrl.lca_out_lofs = p_ctrl_info->info.ime_ctrl.lca_out_size.w * 3;

		if (p_ctrl_info->info.ime_ctrl.lca_out_enable) {
			if (p_ctrl_info->info.ime_ctrl.lca_out_size.w < CTL_IPP_LCA_H_MIN || p_ctrl_info->info.ime_ctrl.lca_out_size.h < CTL_IPP_LCA_V_MIN) {
				CTL_IPP_DBG_WRN("lca out must > (%d,%d) (%d, %d)\r\n", CTL_IPP_LCA_H_MIN, CTL_IPP_LCA_V_MIN, p_ctrl_info->info.ime_ctrl.lca_out_size.w, p_ctrl_info->info.ime_ctrl.lca_out_size.h);
				ctl_ipp_info_release(p_ctrl_info);
				rt = CTL_IPP_E_PAR;
				return rt;
			}
		}
	}

	p_ctrl_info->input_header_addr = 0;
	p_ctrl_info->input_buf_id = p_vdoin_info->count;
	p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
	p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
	p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;

	if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_READY || p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S1) {

		// ========================================================================================================
		// patch fix buffer occupy issue (IVOT_N11000-932), which caused by call-stack overflow issue (NA51055-1609)
		rt = kdf_ipp_handle_validate((KDF_IPP_HANDLE *)p_hdl->kdf_hdl);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "invalid handle(0x%.8x)\r\n", (unsigned int)p_hdl->kdf_hdl);
			return rt;
		}

		// check engine idle
		if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START) {
			rt = kdf_ipp_dir_eng_lock((KDF_IPP_HANDLE *)p_hdl->kdf_hdl);
			if (rt != CTL_IPP_E_OK) {
				return rt;
			}
		}

		// drop old job before new blk, to prevent new blk fail causing drop old job not be processed
		p_ctrl_info->info.ctl_frm_sta_count = ((KDF_IPP_HANDLE *)p_hdl->kdf_hdl)->fra_sta_with_sop_cnt;
		kdf_ipp_direct_datain_drop((KDF_IPP_HANDLE *)p_hdl->kdf_hdl, KDF_IPP_FRAME_BP_START, 0);
		// ========================================== patch done ==================================================

		{
			/* GET OUTPUT BUFFER */
			UINT32 i;
			CTL_IPP_IME_OUT_IMG *p_ime_path;
			CTL_IPP_IME_CTRL *p_ime_ctrl = &p_ctrl_info->info.ime_ctrl;

			if (ctl_ipp_preset_cb(0, (void *)&p_ctrl_info->info, NULL) != CTL_IPP_E_OK) {
				/* DISABLE Output when get buffer failed */
				for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
					p_ime_path = &p_ime_ctrl->out_img[i];
					p_ime_path->dma_enable = FALSE;
				}
			}
		}
		evt.data_addr = (UINT32)&p_ctrl_info->info;
		evt.count = p_ctrl_info->kdf_evt_count;
		evt.rev = 0;

		if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START) {
			rt = kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_DIRECT_START, (void *)&evt);
		} else if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_READY) {
			rt = kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_DIRECT_CFG, (void *)&evt);
		} else if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_STOP_S1) {
			rt = kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_DIRECT_STOP, (void *)&evt);
		}

		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("direct kdf start fail %d evt= %x\r\n", rt, p_hdl->sts);
		}
	} else {
		ctl_ipp_info_release(p_ctrl_info);
		rt = CTL_IPP_E_STATE;
		CTL_IPP_DBG_WRN("Unknown status 0x%.8x\r\n", (unsigned int)p_hdl->kdf_hdl);
	}

#if defined (__KERNEL__)
	if (rt == CTL_IPP_E_OK) {
		if (p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START && p_hdl->func_en & CTL_IPP_FUNC_SHDR) {
			/* only skip start+hdr case for normal boot, due to sie frame cnt sync flow, first frame is not valid
				fastboot has no frame cnt sync flow, must keep last-cfg for reference info
			*/
			if (kdrv_builtin_is_fastboot() && p_hdl->is_first_handle) {
				p_hdl->p_last_cfg_ctrl_info = p_ctrl_info;
			}
		} else {
			p_hdl->p_last_cfg_ctrl_info = p_ctrl_info;
		}
	}
#else
	if (rt == CTL_IPP_E_OK && !(p_hdl->sts == CTL_IPP_HANDLE_STS_DIR_START && p_hdl->func_en & CTL_IPP_FUNC_SHDR)) {
		p_hdl->p_last_cfg_ctrl_info = p_ctrl_info;
	}
#endif

	if (rt == CTL_IPP_E_OK) {
		p_hdl->kdf_snd_evt_cnt += 1;
	}

	return rt;
}

INT32 ctl_ipp_process_flow_pattern_paste(CTL_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 buf_id)
{
	KDF_IPP_EVT evt;
	CTL_IPP_INFO_LIST_ITEM *p_ctrl_info;
	VDO_FRAME *p_vdoin_info;
	UINT32 i, w, h;

	p_vdoin_info = (VDO_FRAME *)header_addr;

	if (p_hdl->kdf_hdl == 0) {
		CTL_IPP_DBG_WRN("Ctrl handle 0x%.8x have no KDF handle\r\n", (unsigned int)p_hdl);
		return CTL_IPP_E_ID;
	}

	/* Check header info */
	if (p_vdoin_info->addr[0] == 0 ||
		p_vdoin_info->size.w == 0 ||
		p_vdoin_info->size.h == 0) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "vdoin info error, addr 0x%.8x, size (%d, %d)\r\n",
			(unsigned int)p_vdoin_info->addr[0], (int)p_vdoin_info->size.w, (int)p_vdoin_info->size.h);
		return CTL_IPP_E_INDATA;
	}

	if (p_hdl->ctrl_info.ime_ctrl.pat_paste_win.w == 0 || p_hdl->ctrl_info.ime_ctrl.pat_paste_win.h == 0) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "pattern size (w(%u)%%, h(%u)%%) can't be 0%%\r\n", (unsigned int)(p_hdl->ctrl_info.ime_ctrl.pat_paste_win.w), (unsigned int)(p_hdl->ctrl_info.ime_ctrl.pat_paste_win.h));
		return CTL_IPP_E_PAR;
	}

	w = CTL_IPP_PATTERN_PASTE_SIZE(p_hdl->ctrl_info.ime_ctrl.pat_paste_info.img_info.size.w, p_hdl->ctrl_info.ime_ctrl.pat_paste_win.w);
	h = CTL_IPP_PATTERN_PASTE_SIZE(p_hdl->ctrl_info.ime_ctrl.pat_paste_info.img_info.size.h, p_hdl->ctrl_info.ime_ctrl.pat_paste_win.h);

	if ((UINT32)p_vdoin_info->size.w != w || (UINT32)p_vdoin_info->size.h != h) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "input size (%u, %u) must equal to pattern background (%u, %u)\r\n",
			(unsigned int)(p_vdoin_info->size.w), (unsigned int)(p_vdoin_info->size.h), (unsigned int)(w), (unsigned int)(h));
		return CTL_IPP_E_PAR;
	}

	// only for checking if buffer is enough. later will set to 0 to read the same line
	if (p_vdoin_info->loff[0] < w) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "input lofs (%u) must >= width (%u)\r\n", (unsigned int)(p_vdoin_info->loff[0]), (unsigned int)(w));
		return CTL_IPP_E_PAR;
	}

	/* IME Ctrl */
	p_hdl->ctrl_info.ime_ctrl.in_addr[0] = p_vdoin_info->addr[0];
	p_hdl->ctrl_info.ime_ctrl.in_addr[1] = p_vdoin_info->addr[0]; // uv read same buffer as y
	p_hdl->ctrl_info.ime_ctrl.in_addr[2] = p_vdoin_info->addr[0];
	p_hdl->ctrl_info.ime_ctrl.in_fmt = VDO_PXLFMT_YUV420; // fix ime behavior as read yuv420 data
	p_hdl->ctrl_info.ime_ctrl.in_size.w = (UINT32)p_vdoin_info->size.w;
	p_hdl->ctrl_info.ime_ctrl.in_size.h = (UINT32)p_vdoin_info->size.h;
	p_hdl->ctrl_info.ime_ctrl.in_lofs[0] = 0; // yuv read same line
	p_hdl->ctrl_info.ime_ctrl.in_lofs[1] = 0;
	p_hdl->ctrl_info.ime_ctrl.in_lofs[2] = 0;

	/* check output RGB */
	if (p_hdl->ctrl_info.ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1].fmt == VDO_PXLFMT_RGB888_PLANAR) {
		for (i = CTL_IPP_OUT_PATH_ID_2; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
			if (p_hdl->ctrl_info.ime_ctrl.out_img[i].enable == ENABLE) {
				CTL_IPP_DBG_WRN("force disable path%d, due to path1 output RGB\r\n", (int)(i));
				p_hdl->ctrl_info.ime_ctrl.out_img[i].enable = DISABLE;
			}
		}
	}

	/* Get ctl_ipp_info from pool */
	p_ctrl_info = ctl_ipp_info_alloc((UINT32)p_hdl);
	if (p_ctrl_info == NULL) {
		return CTL_IPP_E_QOVR;
	}
	memcpy((void *)&p_ctrl_info->info, (void *)&p_hdl->ctrl_info, sizeof(CTL_IPP_BASEINFO));

	// enable pattern paste
	p_ctrl_info->info.ime_ctrl.pat_paste_enable = ENABLE;
	p_ctrl_info->info.ime_ctrl.pat_paste_info.func_en = ENABLE;

	// disable function enable
	p_ctrl_info->info.ime_ctrl.lca_out_enable = DISABLE;
	p_ctrl_info->info.ime_ctrl.tplnr_enable = DISABLE;
	p_ctrl_info->info.ime_ctrl.tplnr_out_ms_roi_enable = DISABLE;

	/* IME Ctrl */
	ctl_ipp_int_ime_flip(p_ctrl_info);

	p_ctrl_info->input_header_addr = header_addr;
	p_ctrl_info->input_buf_id = buf_id;
	p_ctrl_info->ctl_snd_evt_time = p_hdl->snd_evt_time;
	p_ctrl_info->ctl_rev_evt_time = p_hdl->rev_evt_time;
	p_ctrl_info->kdf_evt_count = p_hdl->kdf_snd_evt_cnt;

	/* Send KDF Event */
	evt.data_addr = (UINT32)&p_ctrl_info->info;
	evt.count = p_ctrl_info->kdf_evt_count;
	evt.rev = 0;
	if (kdf_ipp_ioctl(p_hdl->kdf_hdl, KDF_IPP_IOCTL_SNDEVT_PATTERN_PASTE, (void *)&evt) != CTL_IPP_E_OK) {
		ctl_ipp_info_release(p_ctrl_info);
		CTL_IPP_DBG_WRN("Send kdf evt fail\r\n");
		return CTL_IPP_E_QOVR;
	}

	return CTL_IPP_E_OK;
}