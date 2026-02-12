#include "ctl_ipp_int.h"
#include "ctl_ipp_flow_task_int.h"
#include "ctl_ipp_buf_int.h"
#include "ctl_ipp_id_int.h"
#include "ipp_debug_int.h"
#include "kdrv_gfx2d/kdrv_ise.h"
#include "kdrv_type.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

#if 0
#endif
#define CTL_IPP_ISE_LAST_RDY_USED 0xFFFFFFFF	// used for preventing ise_rdy_info racing in case of multi-entry of ctl_ipp_ise_process()
#define CTL_IPP_ISE_MSG_Q_NUM   32
static CTL_IPP_LIST_HEAD ise_free_msg_list_head;
static CTL_IPP_LIST_HEAD ise_proc_msg_list_head;
static UINT32 ctl_ipp_ise_msg_queue_free;
static CTL_IPP_ISE_MSG_EVENT ctl_ipp_ise_msg_pool[CTL_IPP_ISE_MSG_Q_NUM];
static UINT32 ise_rdy_info_num;
static CTL_IPP_ISE_OUT_INFO *ise_rdy_info[CTL_IPP_ISE_MAX];
static UINT32 ctl_ipp_ise_snd_pro_cnt;
static UINT32 ctl_ipp_ise_snd_drop_cnt;
static UINT32 ctl_ipp_ise_rcv_pro_cnt;
static UINT32 ctl_ipp_ise_rcv_drop_cnt;

static CTL_IPP_ISE_MSG_EVENT *ctl_ipp_ise_msg_pool_get_msg(UINT32 idx)
{
	if (idx >= CTL_IPP_ISE_MSG_Q_NUM) {
		CTL_IPP_DBG_WRN("msg idx = %d overflow\r\n", (int)(idx));
		return NULL;
	}
	return &ctl_ipp_ise_msg_pool[idx];
}

static void ctl_ipp_ise_msg_reset(CTL_IPP_ISE_MSG_EVENT *p_event)
{
	if (p_event) {
		p_event->cmd = 0;
		p_event->param[0] = 0;
		p_event->param[1] = 0;
		p_event->param[2] = 0;
		p_event->rev[0] = CTL_IPP_ISE_MSG_STS_FREE;
	}
}

static CTL_IPP_ISE_MSG_EVENT *ctl_ipp_ise_msg_lock(void)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_ISE_MSG_EVENT *p_event = NULL;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&ise_free_msg_list_head)) {
		p_event = vos_list_entry(ise_free_msg_list_head.next, CTL_IPP_ISE_MSG_EVENT, list);
		vos_list_del(&p_event->list);

		p_event->rev[0] |= CTL_IPP_ISE_MSG_STS_LOCK;
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISE, "[CTL_IPP_ISE]Get free queue idx = %d\r\n", (int)(p_event->rev[1]));
		ctl_ipp_ise_msg_queue_free -= 1;

		unl_cpu(loc_cpu_flg);
		return p_event;
	}

	unl_cpu(loc_cpu_flg);
	return p_event;
}

static void ctl_ipp_ise_msg_unlock(CTL_IPP_ISE_MSG_EVENT *p_event)
{
	unsigned long loc_cpu_flg;

	if ((p_event->rev[0] & CTL_IPP_ISE_MSG_STS_LOCK) != CTL_IPP_ISE_MSG_STS_LOCK) {
		CTL_IPP_DBG_IND("event status error (%d)\r\n", (int)(p_event->rev[0]));
	} else {

		ctl_ipp_ise_msg_reset(p_event);

		loc_cpu(loc_cpu_flg);
		vos_list_add_tail(&p_event->list, &ise_free_msg_list_head);
		ctl_ipp_ise_msg_queue_free += 1;
		unl_cpu(loc_cpu_flg);
	}
}

UINT32 ctl_ipp_ise_get_free_queue_num(void)
{
	return ctl_ipp_ise_msg_queue_free;
}

ER ctl_ipp_ise_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_ISE_MSG_EVENT *p_event;

	p_event = ctl_ipp_ise_msg_lock();

	if (p_event == NULL) {
		return E_SYS;
	}

	if (p2 != 0) {
		ctl_ipp_ise_msg_snd_chkdrop(p1, p2);
	}

	p_event->cmd = cmd;
	p_event->param[0] = p1;
	p_event->param[1] = p2;
	p_event->param[2] = p3;
	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISE, "[CTL_IPP_ISE]S Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(p1), (unsigned int)(p2), (unsigned int)(p3));

	/* add to proc list & trig proc task */
	loc_cpu(loc_cpu_flg);
	if (cmd == CTL_IPP_ISE_MSG_PROCESS) {
		ctl_ipp_ise_snd_pro_cnt += 1;
	} else if (cmd == CTL_IPP_ISE_MSG_DROP) {
		ctl_ipp_ise_snd_drop_cnt += 1;
	}
	vos_list_add_tail(&p_event->list, &ise_proc_msg_list_head);
	unl_cpu(loc_cpu_flg);
	vos_flag_iset(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_QUEUE_PROC);
	return E_OK;
}

ER ctl_ipp_ise_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_ISE_MSG_EVENT *p_event;
	FLGPTN flag;
	UINT32 vos_list_empty_flag;

	while (1) {
		/* check empty or not */
		loc_cpu(loc_cpu_flg);
		vos_list_empty_flag = vos_list_empty(&ise_proc_msg_list_head);
		if (!vos_list_empty_flag) {
			p_event = vos_list_entry(ise_proc_msg_list_head.next, CTL_IPP_ISE_MSG_EVENT, list);
			vos_list_del(&p_event->list);

			if (p_event->cmd == CTL_IPP_ISE_MSG_PROCESS) {
				ctl_ipp_ise_rcv_pro_cnt += 1;
			} else if (p_event->cmd == CTL_IPP_ISE_MSG_DROP) {
				ctl_ipp_ise_rcv_drop_cnt += 1;
			}
		}
		unl_cpu(loc_cpu_flg);
		/* is empty enter wait mode */
		if (vos_list_empty_flag) {
			vos_flag_wait(&flag, g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_QUEUE_PROC, TWF_CLR);
		} else {
			break;
		}
	}

	*cmd = p_event->cmd;
	*p1 = p_event->param[0];
	*p2 = p_event->param[1];
	*p3 = p_event->param[2];
	ctl_ipp_ise_msg_unlock(p_event);

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISE, "[CTL_IPP_ISE]R Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(*cmd), (unsigned int)(*p1), (unsigned int)(*p2), (unsigned int)(*p3));
	return E_OK;
}

ER ctl_ipp_ise_msg_flush(void)
{
	UINT32 cmd, param[3];
	ER err = E_OK;

	while (!vos_list_empty(&ise_proc_msg_list_head)) {
		err = ctl_ipp_ise_msg_rcv(&cmd, &param[0], &param[1], &param[2]);
		if (err != E_OK) {
			CTL_IPP_DBG_IND("flush fail (%d)\r\n", (int)(err));
		}

		/* Unlock buffer when flush */
		if (cmd != CTL_IPP_ISE_MSG_IGNORE) {
			ctl_ipp_ise_drop(param[0], param[1], param[2]);
		}
	}

	return err;
}

ER ctl_ipp_ise_erase_queue(UINT32 handle)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_ISE_MSG_EVENT *p_event;
	UINT32 i, j;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&ise_proc_msg_list_head)) {
		vos_list_for_each(p_list, &ise_proc_msg_list_head) {
			p_event = vos_list_entry(p_list, CTL_IPP_ISE_MSG_EVENT, list);
			if (p_event->cmd != CTL_IPP_ISE_MSG_IGNORE) {
				UINT32 ctrl_hdl;

				ctrl_hdl = p_event->param[2];
				if (ctrl_hdl == handle) {
					p_event->cmd = CTL_IPP_ISE_MSG_IGNORE;
				}
			}
		}
	}

	// reset ise_rdy_info to prevent next ipp open -> process get invalid info
	for (i = 0; i < CTL_IPP_ISE_MAX; i++) {
		for (j = 0; j < ise_rdy_info_num; j++) {
			if (ise_rdy_info[i][j].hdl_addr == handle) {
				ise_rdy_info[i][j].hdl_addr = 0;
				ise_rdy_info[i][j].size.w = 0;
				ise_rdy_info[i][j].size.h = 0;
				ise_rdy_info[i][j].lofs = 0;
				ise_rdy_info[i][j].addr = 0;
				ise_rdy_info[i][j].pixel_blk = 0;
			}
		}
	}

	unl_cpu(loc_cpu_flg);

	return E_OK;
}

ER ctl_ipp_ise_msg_reset_queue(void)
{
	UINT32 i;
	CTL_IPP_ISE_MSG_EVENT *free_event;

	/* init free & proc list head */
	VOS_INIT_LIST_HEAD(&ise_free_msg_list_head);
	VOS_INIT_LIST_HEAD(&ise_proc_msg_list_head);

	for (i = 0; i < CTL_IPP_ISE_MSG_Q_NUM; i++) {
		free_event = ctl_ipp_ise_msg_pool_get_msg(i);
		ctl_ipp_ise_msg_reset(free_event);
		free_event->rev[1] = i;
		vos_list_add_tail(&free_event->list, &ise_free_msg_list_head);
	}

	ctl_ipp_ise_msg_queue_free = CTL_IPP_ISE_MSG_Q_NUM;
	ctl_ipp_ise_snd_pro_cnt = 0;
	ctl_ipp_ise_snd_drop_cnt = 0;
	ctl_ipp_ise_rcv_pro_cnt = 0;
	ctl_ipp_ise_rcv_drop_cnt = 0;
	return E_OK;
}

ER ctl_ipp_ise_msg_snd_chkdrop(UINT32 msg, UINT32 addr)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_ISE_MSG_EVENT *p_event = NULL;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info_new;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info_old;

	ctrl_info_new = (CTL_IPP_INFO_LIST_ITEM *)addr;
	loc_cpu(loc_cpu_flg);
	if (msg == CTL_IPP_ISE_PM) {
		vos_list_for_each_safe(p_list, n, &ise_proc_msg_list_head) {
			p_event = vos_list_entry(p_list, CTL_IPP_ISE_MSG_EVENT, list);

			ctrl_info_old = (CTL_IPP_INFO_LIST_ITEM *)p_event->param[1];
			if (p_event->cmd == CTL_IPP_ISE_MSG_PROCESS && msg == p_event->param[0] && ctrl_info_new->owner == ctrl_info_old->owner) {
				p_event->cmd = CTL_IPP_ISE_MSG_DROP;
			}
		}
	}
	unl_cpu(loc_cpu_flg);
	return E_OK;
}

#if 0
#endif
/*
	global variables
*/
static BOOL ctl_ipp_ise_task_opened = FALSE;
static UINT32 ctl_ipp_ise_task_pause_count;
void ctl_ipp_ise_dumpinfo(int (*dump)(const char *fmt, ...))
{
	UINT32 i, j;

	CHAR *msg[] = {
		"PM",
		""
	};

	dump("---- ise task ----\r\n");
	dump("%7s %9s %12s %12s %10s %10s %10s\r\n", "message", "que_num", "hdl_addr", "out_addr", "size_w", "size_h", "pixel_blk");
	for (i = 0; i < CTL_IPP_ISE_MAX; i++) {
		for (j = 0; j < ise_rdy_info_num; j++) {
			if (ise_rdy_info[i][j].hdl_addr != 0) {
				dump("%7s %9d   0x%.8x   0x%.8x %10d %10d %10d\r\n",
					msg[i],
					(int)j,
					(unsigned int)ise_rdy_info[i][j].hdl_addr,
					(unsigned int)ise_rdy_info[i][j].addr,
					(int)ise_rdy_info[i][j].size.w,
					(int)ise_rdy_info[i][j].size.h,
					(int)ise_rdy_info[i][j].pixel_blk);
			}
		}
	}
	dump("\r\n");
	dump("%11s %13s %13s %13s\r\n", "snd_pro_cnt", "snd_drop_cnt", "rev_pro_cnt", "rev_drop_cnt");
	dump("%11d %13d %13d %13d\r\n", (int)ctl_ipp_ise_snd_pro_cnt, (int)ctl_ipp_ise_snd_drop_cnt, (int)ctl_ipp_ise_rcv_pro_cnt, (int)ctl_ipp_ise_rcv_drop_cnt);
}

void ctl_ipp_ise_cal_pm_size(CTL_IPP_PM_PXL_BLK pixel_blk, UINT32 *size_h, UINT32 *size_v)
{
	UINT32 pixelation_base;

	if (pixel_blk >= CTL_IPP_PM_PXL_BLK_MAX) {
		CTL_IPP_DBG_WRN("pixel blk overflow %d\r\n", (int)(pixel_blk));
		pixel_blk = CTL_IPP_PM_PXL_BLK_08;
	}
	pixelation_base = 8 << pixel_blk;
	*size_h = ALIGN_CEIL(*size_h, pixelation_base) / pixelation_base;
	*size_v = ALIGN_CEIL(*size_v, pixelation_base) / pixelation_base;

}

ER ctl_ipp_ise_get_last_rdy(UINT32 msg, CTL_IPP_ISE_OUT_INFO *p_last_rdy)
{
	unsigned long loc_cpu_flg;
	UINT32 i;
	INT32 rt;

	if (p_last_rdy == NULL) {
		CTL_IPP_DBG_WRN("last ready pointer null \r\n");
		return E_SYS;
	}

	p_last_rdy->size.w = 0;
	p_last_rdy->size.h = 0;
	p_last_rdy->lofs =  0;
	p_last_rdy->addr =  0;
	p_last_rdy->pixel_blk = 0;

	if (msg >= CTL_IPP_ISE_MAX) {
		CTL_IPP_DBG_WRN("ISE msg error %d\r\n", (int)(msg));
		return E_SYS;
	}

	/* check handle valid or not*/
	if (ctl_ipp_handle_validate((CTL_IPP_HANDLE *)p_last_rdy->hdl_addr) != CTL_IPP_E_OK) {
		return E_ID;
	}

	rt = E_OK;
	if (msg == CTL_IPP_ISE_PM) {
		UINT32 is_find;

		is_find = FALSE;
		loc_cpu(loc_cpu_flg);
		for (i = 0; i < ise_rdy_info_num; i++) {
			if (p_last_rdy->hdl_addr == ise_rdy_info[CTL_IPP_ISE_PM][i].hdl_addr) {
				p_last_rdy->size.w = ise_rdy_info[CTL_IPP_ISE_PM][i].size.w;
				p_last_rdy->size.h = ise_rdy_info[CTL_IPP_ISE_PM][i].size.h;
				p_last_rdy->lofs =  ise_rdy_info[CTL_IPP_ISE_PM][i].lofs;
				p_last_rdy->addr =  ise_rdy_info[CTL_IPP_ISE_PM][i].addr;
				p_last_rdy->pixel_blk = ise_rdy_info[CTL_IPP_ISE_PM][i].pixel_blk;
				is_find = TRUE;
				break;
			}
		}
		unl_cpu(loc_cpu_flg);
		if (is_find == FALSE) {
			rt = E_ID;
		}
	}

	return rt;
}

void ctl_ipp_ise_process_oneshot(UINT32 msg, UINT32 ctrl_info_addr, CTL_IPP_ISE_OUT_INFO *p_ise_out)
{
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_PM_PXL_BLK pixel_blk;
	KDRV_ISE_MODE ise_param = {0};
	KDRV_ISE_TRIGGER_PARAM ise_trig_type = {0};
	USIZE out;
	UINT32 out_lofs, out_addr;
	UINT32 id, ratio_w;
	ER rt = E_OK, kdrv_rt = E_OK;

	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;
	if (ctrl_info == NULL) {
		CTL_IPP_DBG_WRN("ctrl info null \r\n");
		return;
	}

	/* check handle valid or not*/
	if (ctl_ipp_handle_validate((CTL_IPP_HANDLE *)ctrl_info->owner) != CTL_IPP_E_OK) {
		rt = E_ID;
	}

	if (msg >= CTL_IPP_ISE_MAX) {
		CTL_IPP_DBG_WRN("ISE msg error %d\r\n", (int)(msg));
		rt = E_SYS;
	}

	/* collect scale infomation by msg */
	out_addr = 0;
	out_lofs = 0;
	pixel_blk = 0;
	if (msg == CTL_IPP_ISE_PM) {
		if (ctrl_info->info.ime_ctrl.lca_out_addr == 0 || ctrl_info->info.ime_ctrl.lca_out_size.h * ctrl_info->info.ime_ctrl.lca_out_size.w * ctrl_info->info.ime_ctrl.in_size.w * ctrl_info->info.ime_ctrl.in_size.h == 0) {
			CTL_IPP_DBG_WRN("input para error 0x%.8x %d %d %d %d \r\n", (unsigned int)ctrl_info->info.ime_ctrl.lca_out_addr,
				(int)ctrl_info->info.ime_ctrl.lca_out_size.w, (int)ctrl_info->info.ime_ctrl.lca_out_size.h,
				(int)ctrl_info->info.ime_ctrl.in_size.w, (int)ctrl_info->info.ime_ctrl.in_size.h);
			rt = E_SYS;
		}

		pixel_blk = ctrl_info->info.ime_ctrl.pm_pxl_blk;
		out.w = ctrl_info->info.ime_ctrl.in_size.w;
		out.h = ctrl_info->info.ime_ctrl.in_size.h;
		ctl_ipp_ise_cal_pm_size(pixel_blk, &out.w, &out.h);
		out_lofs = ALIGN_CEIL(out.w*3, 4);
		out_addr = ctl_ipp_buf_task_alloc(CTRL_IPP_BUF_ITEM_PM, (UINT32) ctrl_info, 0, out);

		/* input information */
		ise_param.in_addr = ctrl_info->info.ime_ctrl.lca_out_addr;
		ise_param.in_width = ctrl_info->info.ime_ctrl.lca_out_size.w;
		ise_param.in_height = ctrl_info->info.ime_ctrl.lca_out_size.h;
		ise_param.in_lofs = ctrl_info->info.ime_ctrl.lca_out_size.w*3;
		ise_param.in_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
		ise_param.io_pack_fmt = KDRV_ISE_YUVP;
	}

	if (out_addr == 0 || rt != E_OK) {
		/* close ise */
		CTL_IPP_DBG_WRN("output addr 0x%.8x, rt %d \r\n", (unsigned int)out_addr, (int)rt);
		return;
	}

	/* output information */
	ise_param.out_addr = out_addr;
	ise_param.out_width = out.w;
	ise_param.out_height = out.h;
	ise_param.out_lofs = out_lofs;
	ise_param.out_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;

	/* check scale limitation
		1. check scale up/down
		2. check scale down intergration limitation
	*/
	if ((ise_param.in_width <= ise_param.out_width) || (ise_param.in_height <= ise_param.out_height))  {
		ise_param.scl_method = KDRV_ISE_BILINEAR;
	} else {
		if (((ise_param.in_width-1)*1000/(ise_param.out_width-1) >= 16000) || ((ise_param.in_height-1)*1000/(ise_param.out_height-1) >= 16000)) {
			ise_param.scl_method = KDRV_ISE_BILINEAR;
		} else {
			ise_param.scl_method = KDRV_ISE_INTEGRATION;
		}
	}

	/* check scale ratio is available for integration scale method */
	if (ise_param.in_width > 2112) {
		ratio_w = (ise_param.in_width * 100) / ise_param.out_width;
		if (ratio_w < 207) {
			ise_param.scl_method = KDRV_ISE_BILINEAR;
		}
	} else {
		if (ise_param.out_width > 1024) {
			ise_param.scl_method = KDRV_ISE_BILINEAR;
		}
	}

	/* open ise  */
	id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE0, 0);
	if (kdrv_ise_open(KDRV_CHIP0, KDRV_GFX2D_ISE0) != E_OK) {
		CTL_IPP_DBG_WRN("open ise fail \r\n");
		return ;
	}

	/* set parameters */
	kdrv_rt = kdrv_ise_set(id, KDRV_ISE_PARAM_MODE, (void *) &ise_param);
	if (kdrv_rt != E_OK) {
		CTL_IPP_DBG_WRN("setmode ise fail \r\n");
	}

	/* set trigger */
	if (kdrv_rt == E_OK) {
		ise_trig_type.wait_end = 1;
		ise_trig_type.time_out_ms = 0;
		kdrv_rt = kdrv_ise_trigger(id, &ise_trig_type, NULL, NULL);
		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("trig ise fail \r\n");
		}
	}

	/* close ise */
	if (kdrv_ise_close(KDRV_CHIP0, KDRV_GFX2D_ISE0) != E_OK) {
		CTL_IPP_DBG_WRN("close ise fail \r\n");
	}

	if (kdrv_rt == E_OK) {
		p_ise_out->size.w = out.w;
		p_ise_out->size.h = out.h;
		p_ise_out->lofs = out_lofs;
		p_ise_out->addr = out_addr;
		p_ise_out->pixel_blk = pixel_blk;
	}
}

void ctl_ipp_ise_process(UINT32 msg, UINT32 ctrl_info_addr, UINT32 rev)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;
	CTL_IPP_PM_PXL_BLK pixel_blk;
	KDRV_ISE_MODE ise_param = {0};
	KDRV_ISE_TRIGGER_PARAM ise_trig_type = {0};
	USIZE out;
	UINT32 out_lofs, out_addr = 0, hdl_addr = 0;
	UINT32 i, id, ratio_w;
	ER rt = E_OK, kdrv_rt = E_OK;

	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;
	if (ctrl_info == NULL) {
		CTL_IPP_DBG_WRN("ctrl info null \r\n");
		return;
	}

	/* check handle valid or not*/
	if (ctl_ipp_handle_validate((CTL_IPP_HANDLE *)ctrl_info->owner) != CTL_IPP_E_OK) {
		rt = E_ID;
	}

	if (msg >= CTL_IPP_ISE_MAX) {
		CTL_IPP_DBG_WRN("ISE msg error %d\r\n", (int)(msg));
		rt = E_SYS;
	}

	if (rt != E_OK) {
		ctl_ipp_info_release(ctrl_info);
		return;
	}

	/* search last ready output addr */
	loc_cpu(loc_cpu_flg);
	for (i = 0; i < ise_rdy_info_num; i++) {
		if (ise_rdy_info[msg][i].hdl_addr == ctrl_info->owner) {
			hdl_addr = ctrl_info->owner;
			out_addr = ise_rdy_info[msg][i].addr;
			break;
		} else if (ise_rdy_info[msg][i].hdl_addr == 0) { // ise handle queue must look like (hdl_1 hdl_2 ... 0 0 ...), so just find first zero handle
			ise_rdy_info[msg][i].hdl_addr = CTL_IPP_ISE_LAST_RDY_USED; // temporarily modify hdl_addr to indicate this rdy_info is occupied by this handle, preventing other handle use this rdy_info caused by hdl_addr == 0
			hdl_addr = ctrl_info->owner;
			out_addr = 0; // no previous output addr
			break;
		}
	}
	unl_cpu(loc_cpu_flg);

	if (i == ise_rdy_info_num) {
		CTL_IPP_DBG_WRN("search last addr fail\r\n");
		i = 0;
		rt = E_SYS;
	}

	/* collect scale infomation by msg */
	if (msg == CTL_IPP_ISE_PM) {
		if (ctrl_info->info.ime_ctrl.lca_out_addr == 0 || ctrl_info->info.ime_ctrl.lca_out_size.h * ctrl_info->info.ime_ctrl.lca_out_size.w * ctrl_info->info.ime_ctrl.in_size.w * ctrl_info->info.ime_ctrl.in_size.h == 0) {
			CTL_IPP_DBG_WRN("input para error 0x%.8x %d %d %d %d \r\n", (unsigned int)ctrl_info->info.ime_ctrl.lca_out_addr,
				(int)ctrl_info->info.ime_ctrl.lca_out_size.w, (int)ctrl_info->info.ime_ctrl.lca_out_size.h,
				(int)ctrl_info->info.ime_ctrl.in_size.w, (int)ctrl_info->info.ime_ctrl.in_size.h);
			rt = E_SYS;
		}

		pixel_blk = ctrl_info->info.ime_ctrl.pm_pxl_blk;
		out.w = ctrl_info->info.ime_ctrl.in_size.w;
		out.h = ctrl_info->info.ime_ctrl.in_size.h;
		ctl_ipp_ise_cal_pm_size(pixel_blk, &out.w, &out.h);
		out_lofs = ALIGN_CEIL(out.w*3, 4);
		out_addr = ctl_ipp_buf_task_alloc(CTRL_IPP_BUF_ITEM_PM, (UINT32) ctrl_info, out_addr, out);

		/* input information */
		ise_param.in_addr = ctrl_info->info.ime_ctrl.lca_out_addr;
		ise_param.in_width = ctrl_info->info.ime_ctrl.lca_out_size.w;
		ise_param.in_height = ctrl_info->info.ime_ctrl.lca_out_size.h;
		ise_param.in_lofs = ctrl_info->info.ime_ctrl.lca_out_size.w*3;
		ise_param.in_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
		ise_param.io_pack_fmt = KDRV_ISE_YUVP;
	}

	if (out_addr == 0 || rt != E_OK) {
		CTL_IPP_DBG_WRN("output addr 0x%.8x, rt %d \r\n", (unsigned int)out_addr, (int)rt);
		ctl_ipp_info_release(ctrl_info);
		return;
	}

	// debug dma write protect for privacy mask. start protecting after allocate
	ctl_ipp_dbg_dma_wp_task_start(CTRL_IPP_BUF_ITEM_PM, CTL_IPP_BUF_ADDR_MAKE(out_addr, 0), ((CTL_IPP_HANDLE *)ctrl_info->owner)->private_buf.buf_item.pm[0].size);

	/* output information */
	ise_param.out_addr = out_addr;
	ise_param.out_width = out.w;
	ise_param.out_height = out.h;
	ise_param.out_lofs = out_lofs;
	ise_param.out_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;

	/* check scale limitation
		1. check scale up/down
		2. check scale down intergration limitation
	*/
	if ((ise_param.in_width <= ise_param.out_width) || (ise_param.in_height <= ise_param.out_height))  {
		ise_param.scl_method = KDRV_ISE_BILINEAR;
	} else {
		if (((ise_param.in_width-1)*1000/(ise_param.out_width-1) >= 16000) || ((ise_param.in_height-1)*1000/(ise_param.out_height-1) >= 16000)) {
			ise_param.scl_method = KDRV_ISE_BILINEAR;
		} else {
			ise_param.scl_method = KDRV_ISE_INTEGRATION;
		}
	}

	/* check scale ratio is available for integration scale method */
	if (ise_param.in_width > 2112) {
		ratio_w = (ise_param.in_width * 100) / ise_param.out_width;
		if (ratio_w < 207) {
			ise_param.scl_method = KDRV_ISE_BILINEAR;
		}
	} else {
		if (ise_param.out_width > 1024) {
			ise_param.scl_method = KDRV_ISE_BILINEAR;
		}
	}

	/* open ise  */
	id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE0, 0);
	if (kdrv_ise_open(KDRV_CHIP0, KDRV_GFX2D_ISE0) != E_OK) {
		CTL_IPP_DBG_WRN("open ise fail \r\n");
		ctl_ipp_info_release(ctrl_info);
		return ;
	}

	/* set parameters */
	kdrv_rt = kdrv_ise_set(id, KDRV_ISE_PARAM_MODE, (void *) &ise_param);
	if (kdrv_rt != E_OK) {
		CTL_IPP_DBG_WRN("setmode ise fail \r\n");
	}

	/* set trigger */
	if (kdrv_rt == E_OK) {
		ise_trig_type.wait_end = 1;
		ise_trig_type.time_out_ms = 0;
		kdrv_rt = kdrv_ise_trigger(id, &ise_trig_type, NULL, NULL);
		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("trig ise fail \r\n");
		}
	}

	/* close ise */
	if (kdrv_ise_close(KDRV_CHIP0, KDRV_GFX2D_ISE0) != E_OK) {
		CTL_IPP_DBG_WRN("close ise fail \r\n");
	}

	if (kdrv_rt == E_OK) {
		loc_cpu(loc_cpu_flg);
		ise_rdy_info[msg][i].hdl_addr = hdl_addr;
		ise_rdy_info[msg][i].size.w = out.w;
		ise_rdy_info[msg][i].size.h = out.h;
		ise_rdy_info[msg][i].lofs = out_lofs;
		ise_rdy_info[msg][i].addr = out_addr;
		ise_rdy_info[msg][i].pixel_blk = pixel_blk;
		unl_cpu(loc_cpu_flg);
	} else {
		CTL_IPP_DBG_WRN("search ready queue fail %d \r\n", (int)(msg));
	}

	/* release ctrl_info */
	ctl_ipp_info_release(ctrl_info);
}

void ctl_ipp_ise_drop(UINT32 msg, UINT32 ctrl_info_addr, UINT32 rev)
{
	CTL_IPP_INFO_LIST_ITEM *ctrl_info;

	ctrl_info = (CTL_IPP_INFO_LIST_ITEM *)ctrl_info_addr;
	/* release ctrl_info */
	ctl_ipp_info_release(ctrl_info);
}

ER ctl_ipp_ise_open_tsk(void)
{
	if (ctl_ipp_ise_task_opened) {
		CTL_IPP_DBG_ERR("re-open\r\n");
		return E_SYS;
	}

	if (g_ctl_ipp_ise_flg_id == 0) {
		CTL_IPP_DBG_ERR("g_ctl_ipp_ise_flg_id = %d\r\n", (int)(g_ctl_ipp_ise_flg_id));
		return E_SYS;
	}

	/* ctrl buf init */
	ctl_ipp_ise_task_pause_count = 0;

	/* reset flag */
	vos_flag_clr(g_ctl_ipp_ise_flg_id, FLGPTN_BIT_ALL);
	vos_flag_set(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_PROC_INIT);

	/* reset queue */
	ctl_ipp_ise_msg_reset_queue();

	/* start ctrl task */
	THREAD_CREATE(g_ctl_ipp_ise_tsk_id, ctl_ipp_ise_tsk, NULL, "ctl_ipp_ise_tsk");
	if (g_ctl_ipp_ise_tsk_id == 0) {
		return E_SYS;
	}
	THREAD_SET_PRIORITY(g_ctl_ipp_ise_tsk_id, CTL_IPP_ISE_TSK_PRI);
	THREAD_RESUME(g_ctl_ipp_ise_tsk_id);

	ctl_ipp_ise_task_opened = TRUE;

	/* set ctrl ise task to processing */
	ctl_ipp_ise_set_resume(TRUE);
	return E_OK;
}

ER ctl_ipp_ise_close_tsk(void)
{
	FLGPTN wait_flg = 0;

	if (!ctl_ipp_ise_task_opened) {
		CTL_IPP_DBG_ERR("re-close\r\n");
		return E_SYS;
	}

	while (ctl_ipp_ise_task_pause_count) {
		ctl_ipp_ise_set_pause(ENABLE, ENABLE);
	}

	vos_flag_set(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_RES);
	if (vos_flag_wait(&wait_flg, g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_EXIT_END, (TWF_ORW | TWF_CLR))) {
		/* if wait not ok, force destroy task */
		THREAD_DESTROY(g_ctl_ipp_ise_tsk_id);
	}

	ctl_ipp_ise_task_opened = FALSE;
	return E_OK;
}

THREAD_DECLARE(ctl_ipp_ise_tsk, p1)
{
	ER err;
	UINT32 cmd, param[3];
	FLGPTN wait_flg = 0;

	THREAD_ENTRY();
	goto CTL_IPP_ISE_PAUSE;

CTL_IPP_ISE_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(g_ctl_ipp_ise_flg_id, (CTL_IPP_ISE_TASK_IDLE | CTL_IPP_ISE_TASK_RESUME_END));
		PROFILE_TASK_IDLE();
		err = ctl_ipp_ise_msg_rcv(&cmd, &param[0], &param[1], &param[2]);
		PROFILE_TASK_BUSY();
		vos_flag_clr(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_IDLE | CTL_IPP_ISE_TASK_TRIG_END);

		if (err != E_OK) {
			CTL_IPP_DBG_IND("[CTL_IPP_ISE]Rcv Msg error (%d)\r\n", (int)(err));
		} else {
			/* process event */
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISE, "[CTL_IPP_ISE]P Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));

			vos_flag_set(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_CHK);
			vos_flag_wait(&wait_flg, g_ctl_ipp_ise_flg_id, (CTL_IPP_ISE_TASK_PAUSE | CTL_IPP_ISE_TASK_CHK), (TWF_ORW | TWF_CLR));

			if (cmd == CTL_IPP_ISE_MSG_IGNORE) {
				/* do nothing */
			} else if (cmd == CTL_IPP_ISE_MSG_PROCESS) {
				ctl_ipp_ise_process(param[0], param[1], param[2]);
			} else if (cmd == CTL_IPP_ISE_MSG_DROP) {
				ctl_ipp_ise_drop(param[0], param[1], param[2]);
			} else {
				CTL_IPP_DBG_WRN("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));
			}

			vos_flag_set(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_TRIG_END);
			/* check pause after cmd is processed */
			if (wait_flg & CTL_IPP_ISE_TASK_PAUSE) {
				break;
			}
		}
	}

CTL_IPP_ISE_PAUSE:

	vos_flag_clr(g_ctl_ipp_ise_flg_id, (CTL_IPP_ISE_TASK_RESUME_END));
	vos_flag_set(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_PAUSE_END);
	vos_flag_wait(&wait_flg, g_ctl_ipp_ise_flg_id, (CTL_IPP_ISE_TASK_RES | CTL_IPP_ISE_TASK_RESUME | CTL_IPP_ISE_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flg & CTL_IPP_ISE_TASK_RES) {
		goto CTL_IPP_ISE_DESTROY;
	}

	if (wait_flg & CTL_IPP_ISE_TASK_RESUME) {
		vos_flag_clr(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_PAUSE_END);
		if (wait_flg & CTL_IPP_ISE_TASK_FLUSH) {
			ctl_ipp_ise_msg_flush();
		}
		goto CTL_IPP_ISE_PROC;
	}

CTL_IPP_ISE_DESTROY:
	vos_flag_set(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_EXIT_END);
	THREAD_RETURN(0);
}

ER ctl_ipp_ise_set_resume(BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!ctl_ipp_ise_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_ise_task_pause_count == 0) {
		ctl_ipp_ise_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
		if (b_flush_evt == ENABLE) {
			vos_flag_set(g_ctl_ipp_ise_flg_id, (CTL_IPP_ISE_TASK_RESUME | CTL_IPP_ISE_TASK_FLUSH));
		} else {
			vos_flag_set(g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_RESUME);
		}

		vos_flag_wait(&wait_flg, g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_RESUME_END, TWF_ORW);
	} else {
		ctl_ipp_ise_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
	}

	return E_OK;
}

ER ctl_ipp_ise_set_pause(BOOL b_wait_end, BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!ctl_ipp_ise_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_ise_task_pause_count != 0) {
		ctl_ipp_ise_task_pause_count -= 1;

		if (ctl_ipp_ise_task_pause_count == 0) {
			unl_cpu(loc_cpu_flg);
			wait_flg = CTL_IPP_ISE_TASK_PAUSE;

			vos_flag_set(g_ctl_ipp_ise_flg_id, wait_flg);
			/* send dummy msg, kdf_tsk must rcv msg to pause */
			ctl_ipp_ise_msg_snd(CTL_IPP_ISE_MSG_IGNORE, 0, 0, 0);

			if (b_wait_end == ENABLE) {
				if (vos_flag_wait(&wait_flg, g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_PAUSE_END, TWF_ORW) == E_OK) {
					if (b_flush_evt == TRUE) {
						ctl_ipp_ise_msg_flush();
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

ER ctl_ipp_ise_wait_pause_end(void)
{
	FLGPTN wait_flg;

	if (!ctl_ipp_ise_task_opened) {
		return E_SYS;
	}

	vos_flag_wait(&wait_flg, g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_PAUSE_END, TWF_ORW);
	return E_OK;
}

ER ctl_ipp_ise_wait_process_end(void)
{
	FLGPTN wait_flg;

	if (!ctl_ipp_ise_task_opened) {
		return E_SYS;
	}

	vos_flag_wait(&wait_flg, g_ctl_ipp_ise_flg_id, CTL_IPP_ISE_TASK_TRIG_END, TWF_ORW);
	return E_OK;
}

UINT32 ctl_ipp_ise_pool_init(UINT32 num, UINT32 buf_addr, UINT32 is_query)
{
	UINT32 i, j, buf_size;
	void *p_buf;

	buf_size = sizeof(CTL_IPP_ISE_OUT_INFO) * num * CTL_IPP_ISE_MAX;
	if (is_query) {
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_ISE", CTL_IPP_DBG_CTX_BUF_QUERY, buf_size, buf_addr, num);
		return buf_size;
	}

	p_buf = (void *)buf_addr;
	if (p_buf == NULL) {
		CTL_IPP_DBG_ERR("alloc ipp_ise pool memory failed, num %d, want_size 0x%.8x\r\n", (int)num, (unsigned int)buf_size);
		return CTL_IPP_E_NOMEM;
	}
	CTL_IPP_DBG_TRC("alloc ipp_ise pool memory, num %d, size 0x%.8x, addr 0x%.8x\r\n", (int)num, (unsigned int)buf_size, (unsigned int)p_buf);

	ise_rdy_info_num = num;

	for (i = 0; i < CTL_IPP_ISE_MAX; i++) {
		/* for cim issue when CTL_IPP_ISE_MAX == 1 */
		#if 0
		if (i == 0) {
			ise_rdy_info[i] = (CTL_IPP_ISE_OUT_INFO *)p_buf;
		} else {
			ise_rdy_info[i] = (CTL_IPP_ISE_OUT_INFO *)((UINT32)ise_rdy_info[i - 1] + sizeof(CTL_IPP_ISE_OUT_INFO) * num);
		}
		#endif
		ise_rdy_info[i] = (CTL_IPP_ISE_OUT_INFO *)p_buf;
		for (j = 0; j < ise_rdy_info_num; j++) {
			ise_rdy_info[i][j].hdl_addr = 0;
			ise_rdy_info[i][j].size.w = 0;
			ise_rdy_info[i][j].size.h = 0;
			ise_rdy_info[i][j].lofs = 0;
			ise_rdy_info[i][j].addr = 0;
			ise_rdy_info[i][j].pixel_blk = 0;
		}
	}

	ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_ISE", CTL_IPP_DBG_CTX_BUF_ALLOC, buf_size, buf_addr, num);

	return buf_size;
}

void ctl_ipp_ise_pool_free(void)
{
	UINT32 i;

	if (ise_rdy_info[0] != NULL) {
		CTL_IPP_DBG_TRC("free ipp_ise memory, addr 0x%.8x\r\n", (unsigned int)ise_rdy_info[0]);
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_ISE", CTL_IPP_DBG_CTX_BUF_FREE, 0, (UINT32)ise_rdy_info[0], 0);

		for (i = 0; i < ise_rdy_info_num; i++) {
			ise_rdy_info[i] = NULL;
		}
		ise_rdy_info_num = 0;
	}
}

#if 0
#endif
