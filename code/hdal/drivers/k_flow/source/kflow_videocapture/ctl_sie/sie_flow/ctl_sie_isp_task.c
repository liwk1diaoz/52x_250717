#include "ctl_sie_isp_int.h"
#include "ctl_sie_isp_task_int.h"
#include "ctl_sie_id_int.h"

#if 0
#endif
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)
#define CTL_SIE_ISP_MSG_Q_NUM   (CTL_SIE_MAX_SUPPORT_ID*8)
static CTL_SIE_LIST_HEAD ctl_sie_isp_free_msg_list_head;
static CTL_SIE_LIST_HEAD ctl_sie_isp_proc_msg_list_head;
static UINT32 ctl_sie_isp_msg_queue_free;
static CTL_SIE_ISP_MSG_EVENT ctl_sie_isp_msg_pool[CTL_SIE_ISP_MSG_Q_NUM];

/*
    static function
*/

static CTL_SIE_ISP_MSG_EVENT *__ctl_sie_isp_msg_pool_get_msg(UINT32 idx)
{
	if (idx >= CTL_SIE_ISP_MSG_Q_NUM) {
		ctl_sie_dbg_err("msg idx = %d overflow\r\n", (int)(idx));
		return NULL;
	}
	return &ctl_sie_isp_msg_pool[idx];
}

static void __ctl_sie_isp_msg_reset(CTL_SIE_ISP_MSG_EVENT *p_event)
{
	if (p_event) {
		p_event->cmd = 0;
		p_event->param[0] = 0;
		p_event->param[1] = 0;
		p_event->param[2] = 0;
		p_event->rev[0] = CTL_SIE_ISP_MSG_STS_FREE;
	}
}

static CTL_SIE_ISP_MSG_EVENT *__ctl_sie_isp_msg_lock(void)
{
	CTL_SIE_ISP_MSG_EVENT *p_event = NULL;
	unsigned long flags;

	loc_cpu(flags);
	if (!vos_list_empty(&ctl_sie_isp_free_msg_list_head)) {
		p_event = vos_list_entry(ctl_sie_isp_free_msg_list_head.next, CTL_SIE_ISP_MSG_EVENT, list);
		vos_list_del_init(&p_event->list);

		p_event->rev[0] |= CTL_SIE_ISP_MSG_STS_LOCK;
//		ctl_sie_dbg_trc("[ISP]Get free queue idx = %d\r\n", (int)(p_event->rev[1]));
		ctl_sie_isp_msg_queue_free -= 1;
	}

	unl_cpu(flags);
//	ctl_sie_dbg_err("Get free queue fail\r\n");
	return p_event;
}

static void __ctl_sie_isp_msg_unlock(CTL_SIE_ISP_MSG_EVENT *p_event)
{
	unsigned long flags;

	if ((p_event->rev[0] & CTL_SIE_ISP_MSG_STS_LOCK) != CTL_SIE_ISP_MSG_STS_LOCK) {
		ctl_sie_dbg_err("event status error (%d)\r\n", (int)(p_event->rev[0]));
	} else {
		__ctl_sie_isp_msg_reset(p_event);
		loc_cpu(flags);
		vos_list_add_tail(&p_event->list, &ctl_sie_isp_free_msg_list_head);
		ctl_sie_isp_msg_queue_free += 1;
		unl_cpu(flags);
	}
}

#if 0
#endif
/*
	global variables
*/

UINT32 ctl_sie_isp_get_free_queue_num(void)
{
	return ctl_sie_isp_msg_queue_free;
}

INT32 ctl_sie_isp_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, UINT32 p4)
{
	CTL_SIE_ISP_MSG_EVENT *p_event;
   unsigned long flags;


	p_event = __ctl_sie_isp_msg_lock();

	//skip trigger isp
	if (p_event == NULL) {
		ctl_sie_dbg_ind("snd msg: %d fail\r\n", (int)(cmd));
		return CTL_SIE_E_QOVR;
	}

	p_event->cmd = cmd;
	p_event->param[0] = p1;
	p_event->param[1] = p2;
	p_event->param[2] = p3;
	p_event->param[3] = p4;

//	ctl_sie_dbg_trc("[ISP][Snd]Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(p1), (unsigned int)(p2), (unsigned int)(p3));

	//add to proc list & trig proc task
	loc_cpu(flags);
	vos_list_add_tail(&p_event->list, &ctl_sie_isp_proc_msg_list_head);
	unl_cpu(flags);
	vos_flag_iset(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_QUEUE_PROC);
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_isp_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, UINT32 *p4)
{
	CTL_SIE_ISP_MSG_EVENT *p_event;
	FLGPTN wait_flag;
	UINT32 list_empty_flag;
   unsigned long flags;

	while (1) {
		//check empty or not
		loc_cpu(flags);
		list_empty_flag = vos_list_empty(&ctl_sie_isp_proc_msg_list_head);
		if (!list_empty_flag) {
			p_event = vos_list_entry(ctl_sie_isp_proc_msg_list_head.next, CTL_SIE_ISP_MSG_EVENT, list);
			vos_list_del_init(&p_event->list);
		}
		unl_cpu(flags);

		//is empty enter wait mode
		if (list_empty_flag) {
			vos_flag_wait(&wait_flag, g_ctl_sie_isp_flg_id, CTL_SIE_ISP_QUEUE_PROC, TWF_CLR);
		} else {
			break;
		}
	}

	*cmd = p_event->cmd;
	*p1 = p_event->param[0];
	*p2 = p_event->param[1];
	*p3 = p_event->param[2];
	*p4 = p_event->param[3];

	__ctl_sie_isp_msg_unlock(p_event);

//	ctl_sie_dbg_trc("[ISP][Rcv]Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(*cmd), (unsigned int)(*p1), (unsigned int)(*p2), (unsigned int)(*p3));
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_isp_msg_flush(void)
{
	UINT32 cmd, param[4];
	INT32 rt = CTL_SIE_E_OK;

	if (!vos_list_empty(&ctl_sie_isp_proc_msg_list_head)) {
		rt = ctl_sie_isp_msg_rcv(&cmd, &param[0], &param[1], &param[2], &param[3]);
		if (rt != CTL_SIE_E_OK) {
			ctl_sie_dbg_err("flush fail (%d)\r\n", (int)(rt));
		}
	}

	return rt;
}

INT32 ctl_sie_isp_erase_queue(UINT32 handle)
{
	CTL_SIE_LIST_HEAD *p_list;
	CTL_SIE_ISP_MSG_EVENT *p_event;
	unsigned long flags;

	loc_cpu(flags);
	if (!vos_list_empty(&ctl_sie_isp_proc_msg_list_head)) {
		vos_list_for_each(p_list, &ctl_sie_isp_proc_msg_list_head) {
			p_event = vos_list_entry(p_list, CTL_SIE_ISP_MSG_EVENT, list);
			if (p_event->cmd != CTL_SIE_ISP_MSG_IGNORE) {
				if (p_event->param[0] == handle) {
					p_event->cmd = CTL_SIE_ISP_MSG_IGNORE;
				}
			}
		}
	}
	unl_cpu(flags);

	return CTL_SIE_E_OK;
}

INT32 ctl_sie_isp_msg_reset_queue(void)
{
	UINT32 i;
	CTL_SIE_ISP_MSG_EVENT *free_event;

	//init free & proc list head
	VOS_INIT_LIST_HEAD(&ctl_sie_isp_free_msg_list_head);
	VOS_INIT_LIST_HEAD(&ctl_sie_isp_proc_msg_list_head);

	for (i = 0; i < CTL_SIE_ISP_MSG_Q_NUM; i++) {
		free_event = __ctl_sie_isp_msg_pool_get_msg(i);
		__ctl_sie_isp_msg_reset(free_event);
		free_event->rev[1] = i;
		vos_list_add_tail(&free_event->list, &ctl_sie_isp_free_msg_list_head);
	}
	ctl_sie_isp_msg_queue_free = CTL_SIE_ISP_MSG_Q_NUM;
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_isp_proc(UINT32 id, UINT32 evt, UINT32 cb_fp, UINT32 frame_cnt)
{
	ISP_EVENT_FP isp_event_fp  = (ISP_EVENT_FP)cb_fp;
	UINT32 ts_start, ts_end;
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	if (isp_event_fp != NULL) {
		ts_start = ctl_sie_util_get_timestamp();
		vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_CB_OUT);
		rt = isp_event_fp((ISP_ID)id, evt, frame_cnt, 0);
		vos_flag_clr(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_CB_OUT);
		ts_end = ctl_sie_util_get_timestamp();
		if (dbg_tab != NULL && dbg_tab->set_isp_cb_t_log != NULL) {
			dbg_tab->set_isp_cb_t_log((ISP_ID)id, evt, frame_cnt, ts_start, ts_end);
		}
	}
	return rt;
}

typedef INT32 (*SYNC_EVENT_FP)(UINT32 p0, UINT32 p1, UINT32 p2);

INT32 ctl_sie_sync_proc(UINT32 p0, UINT32 p1, UINT32 p2, UINT32 p3)
{
	SYNC_EVENT_FP sync_event_fp  = (SYNC_EVENT_FP)p3;
	INT32 rt = CTL_SIE_E_OK;

	if (sync_event_fp != NULL) {
		rt = sync_event_fp(p0, p1, p2);
	}
	return rt;
}

static BOOL ctl_sie_isp_task_opened = FALSE;
static UINT32 ctl_sie_isp_task_pause_count;

INT32 ctl_sie_isp_open_tsk(void)
{
	if (ctl_sie_isp_task_opened) {
		ctl_sie_dbg_err("re-open\r\n");
		return CTL_SIE_E_SYS;
	}

	if (g_ctl_sie_isp_flg_id == 0) {
		ctl_sie_dbg_err("g_ctl_sie_isp_flg_id = %d\r\n", (int)(g_ctl_sie_isp_flg_id));
		return CTL_SIE_E_ID;
	}

	/* ctrl isp init */
	ctl_sie_isp_task_pause_count = 0;

	/* reset flag */
	vos_flag_clr(g_ctl_sie_isp_flg_id, FLGPTN_BIT_ALL);
	vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_PROC_INIT);

	/* reset queue */
	ctl_sie_isp_msg_reset_queue();

	/* start ctrl task */
	THREAD_CREATE(g_ctl_sie_isp_tsk_id, ctl_sie_isp_tsk, NULL, "ctl_sie_isp_tsk");
	if (g_ctl_sie_isp_tsk_id == 0) {
		ctl_sie_dbg_err("thread create fail tsk_id %d\r\n", (int)g_ctl_sie_isp_tsk_id);
		return CTL_SIE_E_ID;
	}
	THREAD_SET_PRIORITY(g_ctl_sie_isp_tsk_id, CTL_SIE_ISP_TSK_PRI);
	THREAD_RESUME(g_ctl_sie_isp_tsk_id);

	ctl_sie_isp_task_opened = TRUE;

	/* set ctrl isp task to processing */
	ctl_sie_isp_task_set_resume(TRUE);
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_isp_close_tsk(void)
{
	FLGPTN wait_flg = 0;

	if (!ctl_sie_isp_task_opened) {
		ctl_sie_dbg_err("re-close\r\n");
		return CTL_SIE_E_SYS;
	}

	while (ctl_sie_isp_task_pause_count) {
		ctl_sie_isp_task_set_pause(ENABLE, ENABLE);
	}

	vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_RES);
	if (vos_flag_wait_timeout(&wait_flg, g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_EXIT_END, (TWF_ORW | TWF_CLR), vos_util_msec_to_tick(1000)) != E_OK) {
		/* if wait not ok, force destroy task */
		ctl_sie_dbg_err("wait isp tsk exit timeout, cur_flg = 0x%x, force destroy\r\n", vos_flag_chk(g_ctl_sie_isp_flg_id, 0xffffffff));
		THREAD_DESTROY(g_ctl_sie_isp_tsk_id);
	}

	ctl_sie_isp_task_opened = FALSE;
	return CTL_SIE_E_OK;
}

THREAD_DECLARE(ctl_sie_isp_tsk, p1)
{
	INT32 rt;
	UINT32 cmd, param[4];
	FLGPTN wait_flag = 0;

	THREAD_ENTRY();
	goto CTL_SIE_ISP_PAUSE;

CTL_SIE_ISP_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_IDLE|CTL_SIE_ISP_TASK_RESUME_END);
		PROFILE_TASK_IDLE();
		rt = ctl_sie_isp_msg_rcv(&cmd, &param[0], &param[1], &param[2], &param[3]);
		PROFILE_TASK_BUSY();
		vos_flag_clr(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_IDLE);

		if (rt != CTL_SIE_E_OK) {
			ctl_sie_dbg_err("[CTL_SIE_ISP]Rcv Msg error (%d)\r\n", (int)(rt));
		} else {
			/* process event */
//			ctl_sie_dbg_trc("[ISP][TSK] Cmd:%d P1:0x%x P2:0x%x P3:0x%x P4:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]), (unsigned int)(param[3]));

			vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_CHK);
			vos_flag_wait(&wait_flag, g_ctl_sie_isp_flg_id, (CTL_SIE_ISP_TASK_PAUSE | CTL_SIE_ISP_TASK_CHK), (TWF_ORW | TWF_CLR));

			if (cmd == CTL_SIE_ISP_MSG_IGNORE) {
				/* do nothing */
			} else if (cmd == CTL_SIE_ISP_MSG_CBEVT_ISP) {
				ctl_sie_isp_proc(param[0], param[1], param[2], param[3]);

			} else if (cmd == CTL_SIE_MSG_SYNC) {
				ctl_sie_sync_proc(param[0], param[1], param[2], param[3]);

			} else {
				ctl_sie_dbg_err("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x P4:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]), (unsigned int)(param[3]));
			}

			if (wait_flag & CTL_SIE_ISP_TASK_PAUSE) {
				break;
			}
		}
	}

CTL_SIE_ISP_PAUSE:
	vos_flag_clr(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_RESUME_END);
	vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_PAUSE_END);
	vos_flag_wait(&wait_flag, g_ctl_sie_isp_flg_id, (CTL_SIE_ISP_TASK_RES | CTL_SIE_ISP_TASK_RESUME | CTL_SIE_ISP_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flag & CTL_SIE_ISP_TASK_RES) {
		goto CTL_SIE_ISP_DESTROY;
	}

	if (wait_flag & CTL_SIE_ISP_TASK_RESUME) {
		vos_flag_clr(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_PAUSE_END);
		if (wait_flag & CTL_SIE_ISP_TASK_FLUSH) {
			ctl_sie_isp_msg_flush();
		}
		goto CTL_SIE_ISP_PROC;
	}

CTL_SIE_ISP_DESTROY:
	vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_EXIT_END);
	THREAD_RETURN(0);
}

INT32 ctl_sie_isp_task_set_resume(BOOL b_flush_evt)
{
	FLGPTN wait_flag = 0;
	unsigned long flags;

	if (!ctl_sie_isp_task_opened) {
		return CTL_SIE_E_SYS;
	}

	loc_cpu(flags);
	if (ctl_sie_isp_task_pause_count == 0) {
		ctl_sie_isp_task_pause_count = 1;
		unl_cpu(flags);
		if (b_flush_evt) {
			vos_flag_set(g_ctl_sie_isp_flg_id, (CTL_SIE_ISP_TASK_RESUME | CTL_SIE_ISP_TASK_FLUSH));
		} else {
			vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_RESUME);
		}
		if (vos_flag_wait_timeout(&wait_flag, g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_RESUME_END, TWF_ORW, vos_util_msec_to_tick(1000)) != 0) {
			ctl_sie_dbg_err("wait isp tsk resume end timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_sie_isp_flg_id, 0xffffffff), CTL_SIE_ISP_TASK_RESUME_END);
		}
	} else {
		unl_cpu(flags);
	}
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_isp_task_set_pause(BOOL b_wait_end, BOOL b_flush_evt)
{
	FLGPTN wait_flg = 0;
	unsigned long flags;

	if (!ctl_sie_isp_task_opened) {
		return CTL_SIE_E_SYS;
	}

	loc_cpu(flags);
	if (ctl_sie_isp_task_pause_count != 0) {
		ctl_sie_isp_task_pause_count = 0;
		unl_cpu(flags);
		vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_PAUSE);
		/* send dummy msg, td_tsk must rcv msg to pause */
		ctl_sie_isp_msg_snd(CTL_SIE_ISP_MSG_IGNORE, 0, 0, 0, 0);

		if (b_wait_end) {
			if (vos_flag_wait_timeout(&wait_flg, g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(1000)) != 0) {
				ctl_sie_dbg_err("wait isp tsk pause end timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_sie_isp_flg_id, 0xffffffff), CTL_SIE_ISP_TASK_PAUSE_END);
			}

			if (b_flush_evt) {
				ctl_sie_isp_msg_flush();
			}
		}
		return CTL_SIE_E_OK;
	}
	unl_cpu(flags);
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_isp_task_wait_pause_end(void)
{
	FLGPTN wait_flg = 0;

	if (!ctl_sie_isp_task_opened) {
		return CTL_SIE_E_SYS;
	}

	if (vos_flag_wait_timeout(&wait_flg, g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(1000)) != 0) {
		ctl_sie_dbg_err("wait isp tsk pause end timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_sie_isp_flg_id, 0xffffffff), CTL_SIE_ISP_TASK_PAUSE_END);
		return CTL_SIE_E_SYS;
	}
	return CTL_SIE_E_OK;
}

void ctl_sie_isp_queue_flush(CTL_SIE_ID id)
{
	FLGPTN wait_flag = 0;

	if (vos_flag_wait_timeout(&wait_flag, g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_LOCK, TWF_CLR, vos_util_msec_to_tick(1000)) != 0) {
		ctl_sie_dbg_err("wait isp tsk que flush end timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_sie_isp_flg_id, 0xffffffff), CTL_SIE_ISP_TASK_LOCK);
	}
	ctl_sie_isp_task_set_pause(TRUE, FALSE);
	ctl_sie_isp_erase_queue(id);
	ctl_sie_isp_task_set_resume(FALSE);
	vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_LOCK);
}


