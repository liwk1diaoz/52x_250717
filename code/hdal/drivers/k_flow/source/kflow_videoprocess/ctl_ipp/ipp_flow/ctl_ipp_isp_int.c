#include "ctl_ipp_int.h"
#include "ctl_ipp_id_int.h"
#include "ctl_ipp_isp_int.h"
#include "ipp_debug_int.h"
#include "kdrv_gfx2d/kdrv_ise.h"
#include "kdrv_type.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)


#if 0
#endif
#define CTL_IPP_ISP_MSG_Q_NUM   32
static CTL_IPP_LIST_HEAD ctl_isp_free_msg_list_head;
static CTL_IPP_LIST_HEAD ctl_isp_proc_msg_list_head;
static UINT32 ctl_ipp_isp_msg_queue_free;
static CTL_IPP_ISP_MSG_EVENT ctl_ipp_isp_msg_pool[CTL_IPP_ISP_MSG_Q_NUM];
static UINT32 ctl_ipp_isp_msg_snd_cnt;
static UINT32 ctl_ipp_isp_msg_snd_fail_cnt;
static UINT32 ctl_ipp_isp_msg_rcv_cnt;

static CTL_IPP_ISP_MSG_EVENT *ctl_ipp_isp_msg_pool_get_msg(UINT32 idx)
{
	if (idx >= CTL_IPP_ISP_MSG_Q_NUM) {
		CTL_IPP_DBG_IND("msg idx = %d overflow\r\n", (int)(idx));
		return NULL;
	}
	return &ctl_ipp_isp_msg_pool[idx];
}

static void ctl_ipp_isp_msg_reset(CTL_IPP_ISP_MSG_EVENT *p_event)
{
	if (p_event) {
		p_event->cmd = 0;
		p_event->param[0] = 0;
		p_event->param[1] = 0;
		p_event->param[2] = 0;
		p_event->rev[0] = CTL_IPP_ISP_MSG_STS_FREE;
	}
}

static CTL_IPP_ISP_MSG_EVENT *ctl_ipp_isp_msg_lock(void)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_ISP_MSG_EVENT *p_event = NULL;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&ctl_isp_free_msg_list_head)) {
		p_event = vos_list_entry(ctl_isp_free_msg_list_head.next, CTL_IPP_ISP_MSG_EVENT, list);
		vos_list_del(&p_event->list);

		p_event->rev[0] |= CTL_IPP_ISP_MSG_STS_LOCK;
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "[CTL_IPP_ISP]Get free queue idx = %d\r\n", (int)(p_event->rev[1]));
		ctl_ipp_isp_msg_queue_free -= 1;

		unl_cpu(loc_cpu_flg);
		return p_event;
	}

	unl_cpu(loc_cpu_flg);
	return p_event;
}

static void ctl_ipp_isp_msg_unlock(CTL_IPP_ISP_MSG_EVENT *p_event)
{
	unsigned long loc_cpu_flg;

	if ((p_event->rev[0] & CTL_IPP_ISP_MSG_STS_LOCK) != CTL_IPP_ISP_MSG_STS_LOCK) {
		CTL_IPP_DBG_IND("event status error (%d)\r\n", (int)(p_event->rev[0]));
	} else {

		ctl_ipp_isp_msg_reset(p_event);

		loc_cpu(loc_cpu_flg);
		vos_list_add_tail(&p_event->list, &ctl_isp_free_msg_list_head);
		ctl_ipp_isp_msg_queue_free += 1;
		unl_cpu(loc_cpu_flg);
	}
}

UINT32 ctl_ipp_isp_get_free_queue_num(void)
{
	return ctl_ipp_isp_msg_queue_free;
}

ER ctl_ipp_isp_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_ISP_MSG_EVENT *p_event;

	p_event = ctl_ipp_isp_msg_lock();

	if (p_event == NULL) {
		ctl_ipp_isp_msg_snd_fail_cnt += 1;
		return E_SYS;
	}

	p_event->cmd = cmd;
	p_event->param[0] = p1;
	p_event->param[1] = p2;
	p_event->param[2] = p3;
	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "[CTL_IPP_ISP]S Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(p1), (unsigned int)(p2), (unsigned int)(p3));

	/* add to proc list & trig proc task */
	loc_cpu(loc_cpu_flg);
	ctl_ipp_isp_msg_snd_cnt += 1;
	vos_list_add_tail(&p_event->list, &ctl_isp_proc_msg_list_head);
	unl_cpu(loc_cpu_flg);
	vos_flag_iset(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_QUEUE_PROC);
	return E_OK;
}

ER ctl_ipp_isp_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_ISP_MSG_EVENT *p_event;
	FLGPTN flag;
	UINT32 vos_list_empty_flag;

	while (1) {
		/* check empty or not */
		loc_cpu(loc_cpu_flg);
		vos_list_empty_flag = vos_list_empty(&ctl_isp_proc_msg_list_head);
		if (!vos_list_empty_flag) {
			p_event = vos_list_entry(ctl_isp_proc_msg_list_head.next, CTL_IPP_ISP_MSG_EVENT, list);
			vos_list_del(&p_event->list);
			ctl_ipp_isp_msg_rcv_cnt += 1;
		}
		unl_cpu(loc_cpu_flg);
		/* is empty enter wait mode */
		if (vos_list_empty_flag) {
			vos_flag_wait(&flag, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_QUEUE_PROC, TWF_CLR);
		} else {
			break;
		}
	}

	*cmd = p_event->cmd;
	*p1 = p_event->param[0];
	*p2 = p_event->param[1];
	*p3 = p_event->param[2];
	ctl_ipp_isp_msg_unlock(p_event);

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "[CTL_IPP_ISP]R Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(*cmd), (unsigned int)(*p1), (unsigned int)(*p2), (unsigned int)(*p3));
	return E_OK;
}

ER ctl_ipp_isp_msg_flush(void)
{
	UINT32 cmd, param[3];
	ER err = E_OK;

	while (!vos_list_empty(&ctl_isp_proc_msg_list_head)) {
		err = ctl_ipp_isp_msg_rcv(&cmd, &param[0], &param[1], &param[2]);
		if (err != E_OK) {
			CTL_IPP_DBG_IND("flush fail (%d)\r\n", (int)(err));
		}

		/* Unlock buffer when flush */
		if (cmd != CTL_IPP_ISP_MSG_IGNORE) {
			ctl_ipp_isp_drop(param[0], param[1], param[2]);
		}
	}

	return err;
}

ER ctl_ipp_isp_erase_queue(UINT32 id)
{
	unsigned long loc_flg;
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_ISP_MSG_EVENT *p_event;

	loc_cpu(loc_flg);
	if (!vos_list_empty(&ctl_isp_proc_msg_list_head)) {
		vos_list_for_each(p_list, &ctl_isp_proc_msg_list_head) {
			p_event = vos_list_entry(p_list, CTL_IPP_ISP_MSG_EVENT, list);
			if (p_event->param[0] == id) {
				p_event->cmd = CTL_IPP_MSG_IGNORE;
			}
		}
	}
	unl_cpu(loc_flg);

	return E_OK;
}

ER ctl_ipp_isp_msg_reset_queue(void)
{
	UINT32 i;
	CTL_IPP_ISP_MSG_EVENT *free_event;

	/* init free & proc list head */
	VOS_INIT_LIST_HEAD(&ctl_isp_free_msg_list_head);
	VOS_INIT_LIST_HEAD(&ctl_isp_proc_msg_list_head);

	for (i = 0; i < CTL_IPP_ISP_MSG_Q_NUM; i++) {
		free_event = ctl_ipp_isp_msg_pool_get_msg(i);
		ctl_ipp_isp_msg_reset(free_event);
		free_event->rev[1] = i;
		vos_list_add_tail(&free_event->list, &ctl_isp_free_msg_list_head);
	}

	ctl_ipp_isp_msg_queue_free = CTL_IPP_ISP_MSG_Q_NUM;
	ctl_ipp_isp_msg_snd_cnt = 0;
	ctl_ipp_isp_msg_snd_fail_cnt = 0;
	ctl_ipp_isp_msg_rcv_cnt = 0;
	return E_OK;
}

#if 0
#endif
/*
	global variables
*/
static BOOL ctl_ipp_isp_task_opened = FALSE;
static UINT32 ctl_ipp_isp_task_pause_count;
void ctl_ipp_isp_task_dumpinfo(int (*dump)(const char *fmt, ...))
{
	FLGPTN flg;

	flg = vos_flag_chk(g_ctl_ipp_isp_flg_id, FLGPTN_BIT_ALL);

	dump("\r\n---- ctl_ipp_isp task information ----\r\n");
	dump("%10s  %16s%16s%16s\r\n", "flg", "snd_evt", "snd_fail", "rcv_evt");
	dump("0x%.8x  %16d%16d%16d\r\n", (unsigned int)flg, (int)ctl_ipp_isp_msg_snd_cnt, (int)ctl_ipp_isp_msg_snd_fail_cnt, (int)ctl_ipp_isp_msg_rcv_cnt);

}

void ctl_ipp_isp_process(UINT32 id, UINT32 event, UINT32 raw_frame_cnt)
{
	ctl_ipp_isp_event_cb_proc(id, event, raw_frame_cnt);
}

void ctl_ipp_isp_drop(UINT32 id, UINT32 event, UINT32 raw_frame_cnt)
{

}

ER ctl_ipp_isp_open_tsk(void)
{
	if (ctl_ipp_isp_task_opened) {
		CTL_IPP_DBG_ERR("re-open\r\n");
		return E_SYS;
	}

	if (g_ctl_ipp_isp_flg_id == 0) {
		CTL_IPP_DBG_ERR("g_ctl_ipp_isp_flg_id = %d\r\n", (int)(g_ctl_ipp_isp_flg_id));
		return E_SYS;
	}

	/* ctrl isp init */
	ctl_ipp_isp_task_pause_count = 0;

	/* reset flag */
	vos_flag_clr(g_ctl_ipp_isp_flg_id, FLGPTN_BIT_ALL);
	vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_PROC_INIT);

	/* reset queue */
	ctl_ipp_isp_msg_reset_queue();

	/* start ctrl task */
	THREAD_CREATE(g_ctl_ipp_isp_tsk_id, ctl_ipp_isp_tsk, NULL, "ctl_ipp_isp_tsk");
	if (g_ctl_ipp_isp_tsk_id == 0) {
		return E_SYS;
	}
	THREAD_SET_PRIORITY(g_ctl_ipp_isp_tsk_id, CTL_IPP_ISP_TSK_PRI);
	THREAD_RESUME(g_ctl_ipp_isp_tsk_id);

	ctl_ipp_isp_task_opened = TRUE;

	/* set ctrl ise task to processing */
	ctl_ipp_isp_set_resume(TRUE);
	return E_OK;
}

ER ctl_ipp_isp_close_tsk(void)
{
	FLGPTN wait_flg = 0;

	if (!ctl_ipp_isp_task_opened) {
		CTL_IPP_DBG_ERR("re-close\r\n");
		return E_SYS;
	}

	while (ctl_ipp_isp_task_pause_count) {
		ctl_ipp_isp_set_pause(ENABLE, ENABLE);
	}

	vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_RES);
	if (vos_flag_wait(&wait_flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_EXIT_END, (TWF_ORW | TWF_CLR))) {
		/* if wait not ok, force destroy task */
		THREAD_DESTROY(g_ctl_ipp_isp_tsk_id);
	}

	ctl_ipp_isp_task_opened = FALSE;
	return E_OK;
}

THREAD_DECLARE(ctl_ipp_isp_tsk, p1)
{
	ER err;
	UINT32 cmd, param[3];
	FLGPTN wait_flg = 0;

	THREAD_ENTRY();
	goto CTL_IPP_ISP_PAUSE;

CTL_IPP_ISP_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(g_ctl_ipp_isp_flg_id, (CTL_IPP_ISP_TASK_IDLE | CTL_IPP_ISP_TASK_RESUME_END));
		PROFILE_TASK_IDLE();
		err = ctl_ipp_isp_msg_rcv(&cmd, &param[0], &param[1], &param[2]);
		PROFILE_TASK_BUSY();
		vos_flag_clr(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_IDLE | CTL_IPP_ISP_TASK_TRIG_END);

		if (err != E_OK) {
			CTL_IPP_DBG_IND("[CTL_IPP_ISP]Rcv Msg error (%d)\r\n", (int)(err));
		} else {
			/* process event */
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "[CTL_IPP_ISP]P Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));

			vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_CHK);
			vos_flag_wait(&wait_flg, g_ctl_ipp_isp_flg_id, (CTL_IPP_ISP_TASK_PAUSE | CTL_IPP_ISP_TASK_CHK), (TWF_ORW | TWF_CLR));

			if (cmd == CTL_IPP_ISP_MSG_IGNORE) {
				/* do nothing */
			} else if (cmd == CTL_IPP_ISP_MSG_PROCESS) {
				ctl_ipp_isp_process(param[0], param[1], param[2]);
			} else if (cmd == CTL_IPP_ISP_MSG_DROP) {
				ctl_ipp_isp_drop(param[0], param[1], param[2]);
			} else {
				CTL_IPP_DBG_WRN("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));
			}

			vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_TRIG_END);
			/* check pause after cmd is processed */
			if (wait_flg & CTL_IPP_ISP_TASK_PAUSE) {
				break;
			}
		}
	}

CTL_IPP_ISP_PAUSE:

	vos_flag_clr(g_ctl_ipp_isp_flg_id, (CTL_IPP_ISP_TASK_RESUME_END));
	vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_PAUSE_END);
	vos_flag_wait(&wait_flg, g_ctl_ipp_isp_flg_id, (CTL_IPP_ISP_TASK_RES | CTL_IPP_ISP_TASK_RESUME | CTL_IPP_ISP_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flg & CTL_IPP_ISP_TASK_RES) {
		goto CTL_IPP_ISP_DESTROY;
	}

	if (wait_flg & CTL_IPP_ISP_TASK_RESUME) {
		vos_flag_clr(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_PAUSE_END);
		if (wait_flg & CTL_IPP_ISP_TASK_FLUSH) {
			ctl_ipp_isp_msg_flush();
		}
		goto CTL_IPP_ISP_PROC;
	}

CTL_IPP_ISP_DESTROY:
	vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_EXIT_END);
	THREAD_RETURN(0);
}

ER ctl_ipp_isp_set_resume(BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!ctl_ipp_isp_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_isp_task_pause_count == 0) {
		ctl_ipp_isp_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
		if (b_flush_evt == ENABLE) {
			vos_flag_set(g_ctl_ipp_isp_flg_id, (CTL_IPP_ISP_TASK_RESUME | CTL_IPP_ISP_TASK_FLUSH));
		} else {
			vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_RESUME);
		}

		vos_flag_wait(&wait_flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_RESUME_END, TWF_ORW);
	} else {
		ctl_ipp_isp_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
	}

	return E_OK;
}

ER ctl_ipp_isp_set_pause(BOOL b_wait_end, BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!ctl_ipp_isp_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (ctl_ipp_isp_task_pause_count != 0) {
		ctl_ipp_isp_task_pause_count -= 1;

		if (ctl_ipp_isp_task_pause_count == 0) {
			unl_cpu(loc_cpu_flg);
			wait_flg = CTL_IPP_ISP_TASK_PAUSE;

			vos_flag_set(g_ctl_ipp_isp_flg_id, wait_flg);
			/* send dummy msg, kdf_tsk must rcv msg to pause */
			ctl_ipp_isp_msg_snd(CTL_IPP_ISP_MSG_IGNORE, 0, 0, 0);

			if (b_wait_end == ENABLE) {
				if (vos_flag_wait(&wait_flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_PAUSE_END, TWF_ORW) == E_OK) {
					if (b_flush_evt == TRUE) {
						ctl_ipp_isp_msg_flush();
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

ER ctl_ipp_isp_wait_pause_end(void)
{
	FLGPTN wait_flg;

	if (!ctl_ipp_isp_task_opened) {
		return E_SYS;
	}

	vos_flag_wait(&wait_flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_PAUSE_END, TWF_ORW);
	return E_OK;
}

ER ctl_ipp_isp_wait_process_end(void)
{
	FLGPTN wait_flg;

	if (!ctl_ipp_isp_task_opened) {
		return E_SYS;
	}

	vos_flag_wait(&wait_flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_TASK_TRIG_END, TWF_ORW);
	return E_OK;
}

#if 0
#endif

