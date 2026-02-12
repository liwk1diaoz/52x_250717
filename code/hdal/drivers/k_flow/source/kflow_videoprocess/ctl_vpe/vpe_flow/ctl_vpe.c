#include "kdrv_type.h"
#include "kwrap/file.h"
#include "kflow_videoprocess/ctl_vpe.h"
#include "ctl_vpe_int.h"
#include "ctl_vpe_isp_int.h"

#if CTL_VPE_MODULE_ENABLE
typedef INT32(*CTL_VPE_IOCTL_FP)(CTL_VPE_HANDLE *, void *);
typedef INT32(*CTL_VPE_SET_FP)(CTL_VPE_HANDLE *, void *);
typedef INT32(*CTL_VPE_GET_FP)(CTL_VPE_HANDLE *, void *);

static CTL_VPE_CTL g_ctl_vpe_ctl;

#if 0
#endif

CTL_VPE_HANDLE *ctl_vpe_int_get_handle_by_isp_id(UINT32 isp_id)
{
	unsigned long loc_flg;
	CTL_VPE_LIST_HEAD *p_list;
	CTL_VPE_LIST_HEAD *n;
	CTL_VPE_HANDLE *p_hdl;

	vk_spin_lock_irqsave(&g_ctl_vpe_ctl.hdl_pool.lock, loc_flg);
	vos_list_for_each_safe(p_list, n, &g_ctl_vpe_ctl.hdl_pool.used_list_head) {
		p_hdl = list_entry(p_list, CTL_VPE_HANDLE, list);
		if (p_hdl->isp_id == isp_id) {
			vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.hdl_pool.lock, loc_flg);
			return p_hdl;
		}
	}
	vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.hdl_pool.lock, loc_flg);

	return NULL;
}

static BOOL ctl_vpe_int_valid_handle(CTL_VPE_HANDLE *p_hdl)
{
	unsigned long loc_flg;
	BOOL rt;

	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	rt = (p_hdl->tag == MAKEFOURCC('V', 'P', 'E', 'V'));
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

	return rt;
}

static void ctl_vpe_int_update_out_job_inq_cnt(CTL_VPE_HANDLE *p_hdl, INT32 n)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	if (n == 1) {
		p_hdl->out_job_inq_cnt += 1;
	} else if (n == -1) {
		if (p_hdl->out_job_inq_cnt == 0) {
			DBG_WRN("handle %s, pushevt_inq num already 0\r\n", p_hdl->name);
		} else {
			p_hdl->out_job_inq_cnt -= 1;
		}
	}
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
}

static void ctl_vpe_int_add_out_job_cnt(CTL_VPE_HANDLE *p_hdl, INT32 err)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	p_hdl->out_job_cnt += 1;
	if (err != CTL_VPE_E_OK)
		p_hdl->out_job_err_cnt += 1;
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
}

static CTL_VPE_HANDLE *ctl_vpe_int_alloc_handle(void)
{
	CTL_VPE_MEM_POOL *p_pool;
	CTL_VPE_HANDLE *p_hdl;
	unsigned long loc_flg;

	p_pool = &g_ctl_vpe_ctl.hdl_pool;
	p_hdl = NULL;

	vk_spin_lock_irqsave(&p_pool->lock, loc_flg);
	p_hdl = vos_list_first_entry_or_null(&p_pool->free_list_head, CTL_VPE_HANDLE, list);
	if (p_hdl) {
		vos_list_del_init(&p_hdl->list);
		vos_list_add_tail(&p_hdl->list, &p_pool->used_list_head);
		p_pool->cur_free_num -= 1;
		if ((p_pool->blk_num - p_pool->cur_free_num) > p_pool->max_used_num) {
			p_pool->max_used_num = p_pool->blk_num - p_pool->cur_free_num;
		}
	}
	vk_spin_unlock_irqrestore(&p_pool->lock, loc_flg);

	return p_hdl;
}

static void ctl_vpe_int_free_handle(CTL_VPE_HANDLE *p_hdl)
{
	unsigned long loc_flg;
	CTL_VPE_MEM_POOL *p_pool;

	p_pool = &g_ctl_vpe_ctl.hdl_pool;
	vk_spin_lock_irqsave(&p_pool->lock, loc_flg);
	vos_list_del_init(&p_hdl->list);
	vos_list_add_tail(&p_hdl->list, &p_pool->free_list_head);
	p_pool->cur_free_num++;
	vk_spin_unlock_irqrestore(&p_pool->lock, loc_flg);
}

static BOOL ctl_vpe_int_valid_job(CTL_VPE_JOB *p_job)
{
	return (p_job->tag == MAKEFOURCC('J', 'O', 'B', 'V'));
}

static CTL_VPE_JOB *ctl_vpe_int_get_job_by_kdrv_job(UINT32 kdrv_job_addr)
{
	UINT32 ctl_job_addr;
	CTL_VPE_JOB *p_job;

	ctl_job_addr = kdrv_job_addr - CTL_VPE_UTIL_OFFSETOF(CTL_VPE_JOB, kdrv_job);
	p_job = (CTL_VPE_JOB *)ctl_job_addr;
	if (ctl_vpe_int_valid_job(p_job))
		return p_job;

	return NULL;
}

static CTL_VPE_JOB *ctl_vpe_int_alloc_job(void)
{
	CTL_VPE_MEM_POOL *p_pool;
	CTL_VPE_JOB *p_job;
	unsigned long loc_flg;

	p_pool = &g_ctl_vpe_ctl.job_pool;
	p_job = NULL;

	vk_spin_lock_irqsave(&p_pool->lock, loc_flg);
	p_job = vos_list_first_entry_or_null(&p_pool->free_list_head, CTL_VPE_JOB, list);
	if (p_job) {
		vos_list_del_init(&p_job->list);
		vos_list_add_tail(&p_job->list, &p_pool->used_list_head);
		p_pool->cur_free_num -= 1;
		if ((p_pool->blk_num - p_pool->cur_free_num) > p_pool->max_used_num) {
			p_pool->max_used_num = p_pool->blk_num - p_pool->cur_free_num;
		}
	}
	vk_spin_unlock_irqrestore(&p_pool->lock, loc_flg);

	return p_job;
}

static void ctl_vpe_int_free_job(CTL_VPE_JOB *p_job)
{
	unsigned long loc_flg;
	CTL_VPE_MEM_POOL *p_pool;

	p_pool = &g_ctl_vpe_ctl.job_pool;
	vk_spin_lock_irqsave(&p_pool->lock, loc_flg);
	vos_list_del_init(&p_job->list);
	vos_list_add_tail(&p_job->list, &p_pool->free_list_head);
	p_pool->cur_free_num++;
	vk_spin_unlock_irqrestore(&p_pool->lock, loc_flg);
}

static INT32 ctl_vpe_int_proc_tsk_resume(void)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_RESUME);

	/* wait without timeout */
	rt = vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_RESUME_END, TWF_ORW, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS));
	if (rt < 0) {
		ctl_vpe_dbg_err("wait flag timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.proc_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_RESUME_END);
		vos_flag_clr(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_RESUME);
	}

	return rt;
}

static INT32 ctl_vpe_int_proc_tsk_pause(void)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_PAUSE);

	/* send dummy msg, kdf_tsk must rcv msg to pause */
	ctl_vpe_msg_snd(CTL_VPE_MSG_IGNORE, 0, 0, 0, &g_ctl_vpe_ctl.in_evt_que);
	rt = vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS));
	if (rt < 0) {
		ctl_vpe_dbg_err("wait flag timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.proc_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_PAUSE_END);
		vos_flag_clr(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_PAUSE);
	}

	return rt;
}

static INT32 ctl_vpe_int_proc_tsk_close(void)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_EXIT);
	rt = vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_EXIT_END, TWF_ORW, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS));
	if (rt < 0) {
		ctl_vpe_dbg_err("wait flag timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.proc_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_EXIT_END);
		vos_flag_clr(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_EXIT);
	}

	return rt;
}

static INT32 ctl_vpe_int_cb_tsk_resume(void)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_RESUME);
	rt = vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_RESUME_END, TWF_ORW, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS));
	if (rt < 0) {
		ctl_vpe_dbg_err("wait flag timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.cb_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_RESUME_END);
		vos_flag_clr(g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_RESUME);
	}

	return rt;
}

static INT32 ctl_vpe_int_cb_tsk_pause(void)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_PAUSE);

	/* send dummy msg, kdf_tsk must rcv msg to pause */
	ctl_vpe_msg_snd(CTL_VPE_MSG_IGNORE, 0, 0, 0, &g_ctl_vpe_ctl.cb_evt_que);
	rt = vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS));
	if (rt < 0) {
		ctl_vpe_dbg_err("wait flag timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.cb_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_PAUSE_END);
		vos_flag_clr(g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_PAUSE);
	}

	return rt;
}

static INT32 ctl_vpe_int_cb_tsk_close(void)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_EXIT);
	rt = vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_EXIT_END, TWF_ORW, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS));
	if (rt < 0) {
		ctl_vpe_dbg_err("wait flag timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.cb_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_EXIT_END);
		vos_flag_clr(g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_EXIT);
	}

	return rt;
}

static INT32 ctl_vpe_int_kdrv_open(void)
{
	unsigned long loc_flg;
	UINT32 open_cnt;

	vk_spin_lock_irqsave(&g_ctl_vpe_ctl.common_lock, loc_flg);
	open_cnt = g_ctl_vpe_ctl.kdrv_open_cnt;
	g_ctl_vpe_ctl.kdrv_open_cnt += 1;
	vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.common_lock, loc_flg);

	if (open_cnt == 0) {
		kdrv_vpe_if_open(KDRV_CHIP0, KDRV_VIDEOPROCS_VPE_ENGINE0);
	}

	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_int_kdrv_close(void)
{
	unsigned long loc_flg;
	UINT32 open_cnt;

	vk_spin_lock_irqsave(&g_ctl_vpe_ctl.common_lock, loc_flg);
	if (g_ctl_vpe_ctl.kdrv_open_cnt > 0) {
		g_ctl_vpe_ctl.kdrv_open_cnt -= 1;
		open_cnt = g_ctl_vpe_ctl.kdrv_open_cnt;
	} else {
		open_cnt = 0xff;
		DBG_ERR("close kdrv_vpe when cnt = 0\r\n");
	}
	vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.common_lock, loc_flg);

	if (open_cnt == 0) {
		kdrv_vpe_if_close(0);
	}

	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_int_simple_r_lock(UINT32 *rwlock, vk_spinlock_t *spinlock)
{
	/* bit0: writer lock, bit4~30: reader count, bit31: protect bit */
	unsigned long loc_flg;
	INT32 rt = E_OK;

	vk_spin_lock_irqsave(spinlock, loc_flg);
	/* chk writer lock */
	if (*rwlock & 0x1) {
		rt = E_SYS;
	} else {
		/* add reader count */
		*rwlock += (1 << 4);

		/* chk reader count overflow */
		if (*rwlock & (0x80000000)) {
			*rwlock -= (1 << 4);
			rt = E_NOMEM;
		}
	}
	vk_spin_unlock_irqrestore(spinlock, loc_flg);

	return rt;
}

static INT32 ctl_vpe_int_simple_r_unlock(UINT32 *rwlock, vk_spinlock_t *spinlock)
{
	unsigned long loc_flg;
	INT32 rt = E_OK;

	vk_spin_lock_irqsave(spinlock, loc_flg);
	if ((*rwlock & 0xFFFFFFF0) == 0) {
		DBG_ERR("reader count already zero, 0x%.8x\r\n", (unsigned int)*rwlock);
	} else {
		*rwlock -= (1 << 4);
	}
	vk_spin_unlock_irqrestore(spinlock, loc_flg);

	return rt;
}

static INT32 ctl_vpe_int_simple_w_lock(UINT32 *rwlock, vk_spinlock_t *spinlock)
{
	unsigned long loc_flg;
	INT32 rt = E_OK;

	/* polling to lock bit0, then wait reader count = 0 */
	while (1) {
		vk_spin_lock_irqsave(spinlock, loc_flg);
		if ((*rwlock & 0x1) == 0) {
			*rwlock |= 0x1;
			vk_spin_unlock_irqrestore(spinlock, loc_flg);
			break;
		}
		vk_spin_unlock_irqrestore(spinlock, loc_flg);
		vos_util_delay_us(100);
	}

	while (1) {
		vk_spin_lock_irqsave(spinlock, loc_flg);
		if ((*rwlock & 0xFFFFFFF0) == 0) {
			vk_spin_unlock_irqrestore(spinlock, loc_flg);
			break;
		}
		vk_spin_unlock_irqrestore(spinlock, loc_flg);
		vos_util_delay_us(100);
	}

	return rt;
}

static INT32 ctl_vpe_int_simple_w_unlock(UINT32 *rwlock, vk_spinlock_t *spinlock)
{
	unsigned long loc_flg;
	INT32 rt = E_OK;

	vk_spin_lock_irqsave(spinlock, loc_flg);
	if (*rwlock & 0x1) {
		*rwlock &= ~(0x1);
	} else {
		DBG_ERR("writer lock already unlocked\r\n");
	}
	vk_spin_unlock_irqrestore(spinlock, loc_flg);

	return rt;
}

#if 0
#endif

static CTL_VPE_DBG_TS_NODE* ctl_vpe_dbg_get_job_ts_node(void)
{
	unsigned long loc_flg;
	CTL_VPE_DBG_TS_NODE *p_node;
	UINT32 is_in_dump_process;

	vk_spin_lock_irqsave(&g_ctl_vpe_ctl.common_lock, loc_flg);
	is_in_dump_process = g_ctl_vpe_ctl.dbg_ts_pool.cur_free_num;
	vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.common_lock, loc_flg);

	if (is_in_dump_process)
		return NULL;

	vk_spin_lock_irqsave(&g_ctl_vpe_ctl.dbg_ts_pool.lock, loc_flg);
	p_node = vos_list_first_entry_or_null(&g_ctl_vpe_ctl.dbg_ts_pool.used_list_head, CTL_VPE_DBG_TS_NODE, list);
	if (p_node) {
		vos_list_del_init(&p_node->list);
		p_node->handle = 0;
		p_node->input_frm_count = 0;
		p_node->status = 0;
		memset((void *)&p_node->ts_flow[0], 0, sizeof(UINT32) * CTL_VPE_DBG_TS_MAX);
		vos_list_add_tail(&p_node->list, &g_ctl_vpe_ctl.dbg_ts_pool.used_list_head);
	}
	vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.dbg_ts_pool.lock, loc_flg);

	return p_node;
}

static void ctl_vpe_dbg_set_job_ts(CTL_VPE_DBG_TS_NODE* p_node, UINT32 evt, UINT32 timestamp)
{
	if (p_node && (evt < CTL_VPE_DBG_TS_MAX)) {
		if (timestamp == 0) {
			p_node->ts_flow[evt] = hwclock_get_counter();
		} else {
			p_node->ts_flow[evt] = timestamp;
		}
	}
}

static void ctl_vpe_dbg_set_job_ts_done(CTL_VPE_DBG_TS_NODE* p_node, INT32 err)
{
	if (p_node) {
		p_node->status = 1;
		p_node->err = err;
	}
}

static void ctl_vpe_dbg_save_yuv(CTL_VPE_HANDLE *p_hdl, CTL_VPE_JOB *p_job)
{
	CTL_VPE_DBG_SAVEYUV_CFG *p_cfg;
	VOS_FILE fp;
	CHAR f_name[64];
	UINT32 i;

	p_cfg = &g_ctl_vpe_ctl.dbg_saveyuv_cfg;
	if (p_cfg->enable != ENABLE) {
		return ;
	}

	if (strcmp(p_hdl->name, p_cfg->name) == 0) {
		VDO_FRAME *p_frm;
		UINT32 buf_height;
		UINT32 buf_size;

		if (p_cfg->save_input) {
			p_frm = (VDO_FRAME *)p_job->in_evt.data_addr;
			buf_height = p_frm->size.h;
			if (p_frm->reserved[0] == MAKEFOURCC('H', 'A', 'L', 'N')) {
				buf_height = p_frm->reserved[1];
			}
			snprintf(f_name, 64, "%svpe_input_%dx%d_buf_h%d_fmt0x%.8x_f%llu.RAW",
				p_cfg->path, (int)p_frm->size.w, (int)p_frm->size.h, (int)buf_height,
				(unsigned int)p_frm->pxlfmt, p_frm->count);

			buf_size = ctl_vpe_util_yuv_size(p_frm->pxlfmt, p_frm->size.w, buf_height);
			fp = vos_file_open(f_name, O_CREAT|O_WRONLY|O_SYNC, 0);
			if (fp == (VOS_FILE)(-1)) {
				DBG_ERR("failed in file open:%s\n", f_name);
			} else {
				vos_file_write(fp, (void *)p_frm->addr[0], buf_size);
				vos_file_close(fp);
				DBG_DUMP("save file %s\r\n", f_name);
			}
		}

		for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
			if (p_cfg->save_output & (1 << i)) {
				if (p_job->buf_info[i].buf_addr) {
					p_frm = &p_job->buf_info[i].vdo_frm;
					buf_height = p_frm->size.h;
					if (p_frm->reserved[0] == MAKEFOURCC('H', 'A', 'L', 'N')) {
						buf_height = p_frm->reserved[1];
					}
					snprintf(f_name, 64, "%svpe_output_%dx%d_buf_h%d_fmt0x%.8x_f%llu.RAW",
						p_cfg->path, (int)p_frm->size.w, (int)p_frm->size.h, (int)buf_height,
						(unsigned int)p_frm->pxlfmt, p_frm->count);

					buf_size = p_job->buf_info[i].buf_size;
					fp = vos_file_open(f_name, O_CREAT|O_WRONLY|O_SYNC, 0);
					if (fp == (VOS_FILE)(-1)) {
						DBG_ERR("failed in file open:%s\n", f_name);
					} else {
						vos_file_write(fp, (void *)p_frm->addr[0], buf_size);
						vos_file_close(fp);
						DBG_DUMP("save file %s\r\n", f_name);
					}
				} else {
					DBG_ERR("vpe save output path %d failed, buf_addr = 0\r\n", i);
				}
			}
		}

		memset((void *)p_cfg, 0, sizeof(CTL_VPE_DBG_SAVEYUV_CFG));
	}
}

#if 0
#endif

static INT32 ctl_vpe_cb_evt_flush_by_path(CTL_VPE_MSG_MODIFY_CB_DATA *p_data)
{
	/* push/unlock path buffer */
	CTL_VPE_HANDLE *p_hdl;
	CTL_VPE_JOB *p_job;
	CTL_VPE_FLUSH_CONFIG *p_flush_cfg;
	UINT32 pid;

	p_hdl = (CTL_VPE_HANDLE *)p_data->hdl;
	p_job = (CTL_VPE_JOB *)p_data->data;
	p_flush_cfg = (CTL_VPE_FLUSH_CONFIG *)p_data->p_modify_info;

	pid = p_flush_cfg->pid;
	if (p_job->buf_info[pid].buf_addr != 0) {
		p_job->buf_info[pid].err_msg = p_data->err;

		if (p_job->buf_info[pid].err_msg == CTL_VPE_E_OK) {
			ctl_vpe_outbuf_cb_wrapper(p_hdl->cb_fp[CTL_VPE_CBEVT_OUT_BUF], &p_job->buf_info[pid], CTL_VPE_BUF_PUSH);
		} else {
			ctl_vpe_outbuf_cb_wrapper(p_hdl->cb_fp[CTL_VPE_CBEVT_OUT_BUF], &p_job->buf_info[pid], CTL_VPE_BUF_UNLOCK);
		}
		p_job->buf_info[pid].buf_addr = 0;
	}

	return CTL_VPE_E_OK;
}


static INT32 ctl_vpe_cb_evt_process(UINT32 hdl, UINT32 data, UINT32 buf_id, INT32 err)
{
	CTL_VPE_HANDLE *p_hdl;
	CTL_VPE_JOB *p_job;
	UINT32 i;

	p_hdl = (CTL_VPE_HANDLE *)hdl;
	p_job = (CTL_VPE_JOB *)data;

	ctl_vpe_dbg_set_job_ts(p_job->p_dbg_node, CTL_VPE_DBG_TS_CB_TSK_ST, 0);
	if (err == CTL_VPE_E_OK) {
		/* check job status*/
		if (p_job->kdrv_cfg.job_status != KDRV_VPE_STATUS_DONE) {
			atomic_inc(&p_hdl->dbg_err_accu_info.err_kdrv_drop);
			err = CTL_VPE_E_KDRV_DROP;
		}
	}

	/* debug save yuv flow */
	ctl_vpe_dbg_save_yuv(p_hdl, p_job);

	/* input buffer release callback */
	p_job->in_evt.err_msg = err;
	ctl_vpe_inbuf_cb_wrapper(p_hdl->cb_fp[CTL_VPE_CBEVT_IN_BUF], &p_job->in_evt, CTL_VPE_BUF_UNLOCK);

	/* output buffer push callback */
	for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
		if (p_job->buf_info[i].buf_addr != 0) {
			p_job->buf_info[i].err_msg = err;

			if (err == CTL_VPE_E_OK) {
				ctl_vpe_outbuf_cb_wrapper(p_hdl->cb_fp[CTL_VPE_CBEVT_OUT_BUF], &p_job->buf_info[i], CTL_VPE_BUF_PUSH);
			} else {
				ctl_vpe_outbuf_cb_wrapper(p_hdl->cb_fp[CTL_VPE_CBEVT_OUT_BUF], &p_job->buf_info[i], CTL_VPE_BUF_UNLOCK);
			}
		}
	}
	ctl_vpe_dbg_set_job_ts(p_job->p_dbg_node, CTL_VPE_DBG_TS_CB_TSK_ED, 0);
	ctl_vpe_dbg_set_job_ts_done(p_job->p_dbg_node, err);

	ctl_vpe_int_update_out_job_inq_cnt(p_hdl, -1);
	ctl_vpe_int_add_out_job_cnt(p_hdl, err);
	ctl_vpe_int_free_job(p_job);

	return CTL_VPE_E_OK;
}

static void ctl_vpe_jobdone_cb(void *param)
{
	CTL_VPE_JOB *p_job;
	INT32 rt;

	/* param is pointer of vpe_job_list_s*/
	p_job = ctl_vpe_int_get_job_by_kdrv_job((UINT32)param);
	if (p_job == NULL) {
		DBG_ERR("get job error\r\n");
		return ;
	}
	ctl_vpe_dbg_set_job_ts(p_job->p_dbg_node, CTL_VPE_DBG_TS_END_ISR, 0);

	ctl_vpe_int_update_out_job_inq_cnt((CTL_VPE_HANDLE *)p_job->owner, 1);
	rt = ctl_vpe_msg_snd(CTL_VPE_MSG_PROCESS, (UINT32)p_job->owner, (UINT32)p_job, 0, &g_ctl_vpe_ctl.cb_evt_que);
	if (rt != CTL_VPE_E_OK) {
		/* snd event failed, call process api directly */
		DBG_ERR("snd drop event failed\r\n");
		ctl_vpe_cb_evt_process((UINT32)p_job->owner, (UINT32)p_job, 0, rt);
	}

	vos_flag_iset(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_JOBDONE);
}

static INT32 ctl_vpe_in_evt_process(UINT32 hdl, UINT32 data, UINT32 buf_id, INT32 err)
{
	CTL_VPE_HANDLE *p_hdl;
	CTL_VPE_JOB *p_job;
	VDO_FRAME *p_in_frm;
	CTL_VPE_EVT rls_evt;
	INT32 rt, snd_evt_rt;

	p_hdl = (CTL_VPE_HANDLE *)hdl;

	/* err is not ok, set rt to flush and drop input buffer */
	rt = CTL_VPE_E_OK;
	if (err != CTL_VPE_E_OK) {
		rt = CTL_VPE_E_FLUSH;
	}

	/* alloca job */
	p_job = ctl_vpe_int_alloc_job();
	if (p_job == NULL) {
		atomic_inc(&p_hdl->dbg_err_accu_info.err_nomem);
		DBG_ERR("alloc job fail\r\n");
		rt = CTL_VPE_E_NOMEM;
	}

	/* error handle drop event, consider wait job available?
		always use CTL_VPE_E_DROP_INPUT_ONLY to this drop input only for hdal errpr handle
	*/
	if (rt != CTL_VPE_E_OK) {
		if (p_job != NULL) {
			p_job->p_dbg_node = ctl_vpe_dbg_get_job_ts_node();
			if(p_job->p_dbg_node) {
				p_in_frm = (VDO_FRAME *)data;
				p_job->p_dbg_node->handle = (UINT32)p_hdl;
				p_job->p_dbg_node->input_frm_count = p_in_frm->count;
			}
			ctl_vpe_dbg_set_job_ts(p_job->p_dbg_node, CTL_VPE_DBG_TS_SNDEVT, p_hdl->reserved[0]);
			ctl_vpe_dbg_set_job_ts(p_job->p_dbg_node, CTL_VPE_DBG_TS_ALLOC, 0);
			ctl_vpe_dbg_set_job_ts_done(p_job->p_dbg_node, CTL_VPE_E_DROP_INPUT_ONLY);
			ctl_vpe_int_free_job(p_job);
		}
		rls_evt.buf_id = buf_id;
		rls_evt.data_addr = data;
		rls_evt.err_msg = CTL_VPE_E_DROP_INPUT_ONLY;
		ctl_vpe_inbuf_cb_wrapper(p_hdl->cb_fp[CTL_VPE_CBEVT_IN_BUF], &rls_evt, CTL_VPE_BUF_UNLOCK);
		ctl_vpe_int_add_out_job_cnt(p_hdl, rt);
		DBG_WRN("drop in buf, hdl %s, data 0x%.8x, buf_id %d, rt %d\r\n", p_hdl->name, (unsigned int)data, (int)buf_id, (int)rls_evt.err_msg);
		return rt;
	}

	/* job process */
	p_job->owner = (void *)p_hdl;
	p_job->in_evt.buf_id = buf_id;
	p_job->in_evt.data_addr = data;
	p_job->in_evt.err_msg = CTL_VPE_E_OK;
	p_job->p_dbg_node = ctl_vpe_dbg_get_job_ts_node();
	ctl_vpe_dbg_set_job_ts(p_job->p_dbg_node, CTL_VPE_DBG_TS_SNDEVT, p_hdl->reserved[0]);
	ctl_vpe_dbg_set_job_ts(p_job->p_dbg_node, CTL_VPE_DBG_TS_ALLOC, 0);
	rt = ctl_vpe_process_d2d(p_hdl, p_job, ctl_vpe_jobdone_cb);
	ctl_vpe_dbg_set_job_ts(p_job->p_dbg_node, CTL_VPE_DBG_TS_CONFIG_ED, 0);

	if (rt != CTL_VPE_E_OK) {
		/* inform callback thread to drop */
		ctl_vpe_int_update_out_job_inq_cnt((CTL_VPE_HANDLE *)p_job->owner, 1);
		snd_evt_rt = ctl_vpe_msg_snd(CTL_VPE_MSG_DROP, (UINT32)p_job->owner, (UINT32)p_job, rt, &g_ctl_vpe_ctl.cb_evt_que);
		if (snd_evt_rt != CTL_VPE_E_OK) {
			/* snd event failed, call process api directly */
			DBG_ERR("snd drop event failed\r\n");
			ctl_vpe_cb_evt_process((UINT32)p_job->owner, (UINT32)p_job, 0, rt);
		}
	} else {
		p_hdl->p_fired_job = p_job;
	}

	return rt;
}

static INT32 ctl_vpe_in_event_wait_job_done(CTL_VPE_CTL *p_ctl, CTL_VPE_HANDLE *p_hdl)
{
	FLGPTN wait_flg = 0;
	UINT32 timeout_ms;
	INT32 err;

	/* process success, wait engine process end
		minimum timeout_ms = CTL_VPE_JOB_TIMEOUT_MS
	*/
	timeout_ms = (p_ctl->dbg_eng_hang.timeout_ms < CTL_VPE_JOB_TIMEOUT_MS) ? CTL_VPE_JOB_TIMEOUT_MS : p_ctl->dbg_eng_hang.timeout_ms;
	err = vos_flag_wait_timeout(&wait_flg, p_ctl->proc_tsk_flg_id, CTL_VPE_TASK_JOBDONE, TWF_CLR, vos_util_msec_to_tick(timeout_ms));
	if (err != E_OK) {
		DBG_ERR("%s@0x%.8x wait process end timeout, check if engine hang\r\n", p_hdl->name, (unsigned int)p_hdl);

		if (p_ctl->dbg_eng_hang.dump_info) {
			DBG_ERR("dump current config, may have difference between kflow and kdrv config due to timing issue\r\n");
			ctl_vpe_dump_all(ctl_vpe_int_printf);
			ctl_vpe_process_dbg_dump_kdrv_cfg(p_hdl->p_fired_job, ctl_vpe_int_printf);
			debug_dumpmem(0xf0cd0000, 0x1200);
		}

		if (p_ctl->dbg_eng_hang.dump_file) {
			ctl_vpe_save_yuv_cfg(p_hdl->name, "/tmp/", 0x1, 0xf);
			ctl_vpe_dbg_save_yuv(p_hdl, p_hdl->p_fired_job);
		}

		/* wait forever
			todo: driver hw reset?
		*/
		vos_flag_wait(&wait_flg, p_ctl->proc_tsk_flg_id, CTL_VPE_TASK_OOPS, TWF_ORW);
	} else {
		p_hdl->p_fired_job = NULL;
	}

	return E_OK;
}

THREAD_DECLARE(ctl_vpe_proc_tsk, p1)
{
	INT32 err;
	UINT32 cmd, param[3], time;
	FLGPTN wait_flg = 0;
	CTL_VPE_CTL *p_ctl;
	CTL_VPE_HANDLE *p_hdl;

	p_ctl = &g_ctl_vpe_ctl;
	THREAD_ENTRY();
	goto CTL_VPE_PAUSE;

CTL_VPE_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(p_ctl->proc_tsk_flg_id, (CTL_VPE_TASK_IDLE | CTL_VPE_TASK_RESUME_END));
		PROFILE_TASK_IDLE();
		err = ctl_vpe_msg_rcv(&cmd, &param[0], &param[1], &param[2], &time, &p_ctl->in_evt_que);
		PROFILE_TASK_BUSY();
		vos_flag_clr(p_ctl->proc_tsk_flg_id, CTL_VPE_TASK_IDLE);

		if (err != CTL_VPE_E_OK) {
			DBG_ERR("[CTL_VPE]Rcv Msg error (%d)\r\n", (int)(err));
		} else {
			/* process event */
			DBG_IND("[CTL_VPE] PROCTSK P Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));

			if (cmd == CTL_VPE_MSG_IGNORE) {
				/* do nothing */
			} else if (cmd == CTL_VPE_MSG_PROCESS) {
				vos_flag_clr(p_ctl->proc_tsk_flg_id, CTL_VPE_TASK_JOBDONE);
				p_hdl = (CTL_VPE_HANDLE *)param[0];
				p_hdl->reserved[0] = time;
				err = ctl_vpe_in_evt_process(param[0], param[1], param[2], CTL_VPE_E_OK);
				if (err == CTL_VPE_E_OK) {
					/* wait job done callback */
					ctl_vpe_in_event_wait_job_done(p_ctl, p_hdl);
				}
			} else {
				DBG_WRN("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));
			}

			/* check pause after cmd is processed */
			vos_flag_set(p_ctl->proc_tsk_flg_id, CTL_VPE_TASK_CHK);
			vos_flag_wait(&wait_flg, p_ctl->proc_tsk_flg_id, (CTL_VPE_TASK_PAUSE | CTL_VPE_TASK_CHK), (TWF_ORW | TWF_CLR));
			if (wait_flg & CTL_VPE_TASK_PAUSE) {
				break;
			}
		}
	}

CTL_VPE_PAUSE:
	vos_flag_clr(p_ctl->proc_tsk_flg_id, (CTL_VPE_TASK_RESUME_END));
	vos_flag_set(p_ctl->proc_tsk_flg_id, CTL_VPE_TASK_PAUSE_END);
	vos_flag_wait(&wait_flg, p_ctl->proc_tsk_flg_id, (CTL_VPE_TASK_EXIT | CTL_VPE_TASK_RESUME | CTL_VPE_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flg & CTL_VPE_TASK_EXIT) {
		goto CTL_VPE_DESTROY;
	}

	if (wait_flg & CTL_VPE_TASK_RESUME) {
		vos_flag_clr(p_ctl->proc_tsk_flg_id, CTL_VPE_TASK_PAUSE_END);
		if (wait_flg & CTL_VPE_TASK_FLUSH) {
			//ctl_ipp_msg_flush();
		}
		goto CTL_VPE_PROC;
	}

CTL_VPE_DESTROY:
	vos_flag_set(p_ctl->proc_tsk_flg_id, CTL_VPE_TASK_EXIT_END);
	THREAD_RETURN(0);
}

THREAD_DECLARE(ctl_vpe_cb_tsk, p1)
{
	INT32 err;
	UINT32 cmd, param[3], time;
	FLGPTN wait_flg = 0;
	CTL_VPE_CTL *p_ctl;

	p_ctl = &g_ctl_vpe_ctl;
	THREAD_ENTRY();
	goto CTL_VPE_PAUSE;

CTL_VPE_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(p_ctl->cb_tsk_flg_id, (CTL_VPE_TASK_IDLE | CTL_VPE_TASK_RESUME_END));
		PROFILE_TASK_IDLE();
		err = ctl_vpe_msg_rcv(&cmd, &param[0], &param[1], &param[2], &time, &p_ctl->cb_evt_que);
		PROFILE_TASK_BUSY();
		vos_flag_clr(p_ctl->cb_tsk_flg_id, CTL_VPE_TASK_IDLE);

		if (err != CTL_VPE_E_OK) {
			DBG_ERR("[CTL_VPE]Rcv Msg error (%d)\r\n", (int)(err));
		} else {
			/* process event */
			DBG_IND("[CTL_VPE] CBTSK P Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));

			if (cmd == CTL_VPE_MSG_IGNORE) {
				/* do nothing */
			} else if (cmd == CTL_VPE_MSG_PROCESS) {
				ctl_vpe_cb_evt_process(param[0], param[1], 0, CTL_VPE_E_OK);
			} else if (cmd == CTL_VPE_MSG_DROP) {
				ctl_vpe_cb_evt_process(param[0], param[1], 0, param[2]);
			} else {
				DBG_WRN("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));
			}

			/* check pause after cmd is processed */
			vos_flag_set(p_ctl->cb_tsk_flg_id, CTL_VPE_TASK_CHK);
			vos_flag_wait(&wait_flg, p_ctl->cb_tsk_flg_id, (CTL_VPE_TASK_PAUSE | CTL_VPE_TASK_CHK), (TWF_ORW | TWF_CLR));
			if (wait_flg & CTL_VPE_TASK_PAUSE) {
				break;
			}
		}
	}

CTL_VPE_PAUSE:
	vos_flag_clr(p_ctl->cb_tsk_flg_id, (CTL_VPE_TASK_RESUME_END));
	vos_flag_set(p_ctl->cb_tsk_flg_id, CTL_VPE_TASK_PAUSE_END);
	vos_flag_wait(&wait_flg, p_ctl->cb_tsk_flg_id, (CTL_VPE_TASK_EXIT | CTL_VPE_TASK_RESUME | CTL_VPE_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flg & CTL_VPE_TASK_EXIT) {
		goto CTL_VPE_DESTROY;
	}

	if (wait_flg & CTL_VPE_TASK_RESUME) {
		vos_flag_clr(p_ctl->cb_tsk_flg_id, CTL_VPE_TASK_PAUSE_END);
		if (wait_flg & CTL_VPE_TASK_FLUSH) {
			//ctl_ipp_msg_flush();
		}
		goto CTL_VPE_PROC;
	}

CTL_VPE_DESTROY:
	vos_flag_set(p_ctl->cb_tsk_flg_id, CTL_VPE_TASK_EXIT_END);
	THREAD_RETURN(0);
}

#if 0
#endif

UINT32 ctl_vpe_query(CTL_VPE_CTX_BUF_CFG ctx_buf_cfg)
{
	UINT32 buf_addr = 0;

	/* vpe handle buffer */
	buf_addr += sizeof(CTL_VPE_HANDLE) * ctx_buf_cfg.handle_num;

	/* input queue buffer */
	buf_addr += sizeof(CTL_VPE_MSG_EVENT) * ctx_buf_cfg.queue_num;

	/* callback queue buffer */
	buf_addr += sizeof(CTL_VPE_MSG_EVENT) * ctx_buf_cfg.queue_num;

	/* job pool buffer */
	buf_addr += sizeof(CTL_VPE_JOB) * ctx_buf_cfg.queue_num;

	/* debug */
	buf_addr += sizeof(CTL_VPE_DBG_TS_NODE) * CTL_VPE_DBG_TS_NODE_MAX_NUM;

	return buf_addr;
}

INT32 ctl_vpe_init(CTL_VPE_CTX_BUF_CFG ctx_buf_cfg, UINT32 buf_addr, UINT32 buf_size)
{
	UINT32 i;
	UINT32 cur_buf_addr;
	CHAR name[16] = {0};
	CTL_VPE_CTL *p_ctl;

	p_ctl = &g_ctl_vpe_ctl;
	cur_buf_addr = buf_addr;

	vk_spin_lock_init(&p_ctl->common_lock);
	p_ctl->kdrv_open_cnt = 0;

	/* handle pool init	*/
	snprintf(p_ctl->hdl_pool.name, 16, "handle pool");
	vk_spin_lock_init(&p_ctl->hdl_pool.lock);
	p_ctl->hdl_pool.blk_num = ctx_buf_cfg.handle_num;
	p_ctl->hdl_pool.blk_size = sizeof(CTL_VPE_HANDLE);
	p_ctl->hdl_pool.total_size = p_ctl->hdl_pool.blk_num * p_ctl->hdl_pool.blk_size;
	p_ctl->hdl_pool.cur_free_num = p_ctl->hdl_pool.blk_num;
	p_ctl->hdl_pool.max_used_num = 0;
	p_ctl->hdl_pool.start_addr = cur_buf_addr;
	cur_buf_addr += p_ctl->hdl_pool.total_size;

	VOS_INIT_LIST_HEAD(&p_ctl->hdl_pool.free_list_head);
	VOS_INIT_LIST_HEAD(&p_ctl->hdl_pool.used_list_head);
	for (i = 0; i < p_ctl->hdl_pool.blk_num; i++) {
		CTL_VPE_HANDLE *p_hdl;

		p_hdl = (CTL_VPE_HANDLE *)(p_ctl->hdl_pool.start_addr + (p_ctl->hdl_pool.blk_size * i));
		memset((void *)p_hdl, 0, sizeof(CTL_VPE_HANDLE));
		snprintf(p_hdl->name, 16, "none");

		snprintf(name, 16, "ctl_vpe_sem_%d", (int)i);
		vos_sem_create(&p_hdl->sem, 1, name);
		vk_spin_lock_init(&p_hdl->lock);
		vk_spin_lock_init(&p_hdl->isp_lock);
		VOS_INIT_LIST_HEAD(&p_hdl->list);
		vos_list_add_tail(&p_hdl->list, &p_ctl->hdl_pool.free_list_head);
		p_hdl->sndevt_rwlock = 0;
	}

	/* input queue init */
	snprintf(p_ctl->in_evt_que.name, 16, "in_queue");
	vk_spin_lock_init(&p_ctl->in_evt_que.lock);
	vos_flag_create(&p_ctl->in_evt_que.flg_id, NULL, "ctl_vpe_in_que_flg");
	p_ctl->in_evt_que.blk_num = ctx_buf_cfg.queue_num;
	p_ctl->in_evt_que.blk_size = sizeof(CTL_VPE_MSG_EVENT);
	p_ctl->in_evt_que.total_size = p_ctl->in_evt_que.blk_num * p_ctl->in_evt_que.blk_size;
	p_ctl->in_evt_que.cur_free_num = p_ctl->in_evt_que.blk_num;
	p_ctl->in_evt_que.max_used_num = 0;
	p_ctl->in_evt_que.start_addr = cur_buf_addr;
	cur_buf_addr += p_ctl->in_evt_que.total_size;
	ctl_vpe_msg_init_queue(&p_ctl->in_evt_que);

	/* callback queue init */
	snprintf(p_ctl->cb_evt_que.name, 16, "cb_queue");
	vk_spin_lock_init(&p_ctl->cb_evt_que.lock);
	vos_flag_create(&p_ctl->cb_evt_que.flg_id, NULL, "ctl_vpe_cb_que_flg");
	p_ctl->cb_evt_que.blk_num = ctx_buf_cfg.queue_num;
	p_ctl->cb_evt_que.blk_size = sizeof(CTL_VPE_MSG_EVENT);
	p_ctl->cb_evt_que.total_size = p_ctl->cb_evt_que.blk_num * p_ctl->cb_evt_que.blk_size;
	p_ctl->cb_evt_que.cur_free_num = p_ctl->cb_evt_que.blk_num;
	p_ctl->cb_evt_que.max_used_num = 0;
	p_ctl->cb_evt_que.start_addr = cur_buf_addr;
	cur_buf_addr += p_ctl->cb_evt_que.total_size;
	ctl_vpe_msg_init_queue(&p_ctl->cb_evt_que);

	/* job pool init */
	snprintf(p_ctl->job_pool.name, 16, "job pool");
	vk_spin_lock_init(&p_ctl->job_pool.lock);
	p_ctl->job_pool.blk_num = ctx_buf_cfg.queue_num;
	p_ctl->job_pool.blk_size = sizeof(CTL_VPE_JOB);
	p_ctl->job_pool.total_size = p_ctl->job_pool.blk_num * p_ctl->job_pool.blk_size;
	p_ctl->job_pool.cur_free_num = p_ctl->job_pool.blk_num;
	p_ctl->job_pool.max_used_num = 0;
	p_ctl->job_pool.start_addr = cur_buf_addr;
	cur_buf_addr += p_ctl->job_pool.total_size;

	VOS_INIT_LIST_HEAD(&p_ctl->job_pool.free_list_head);
	VOS_INIT_LIST_HEAD(&p_ctl->job_pool.used_list_head);
	for (i = 0; i < p_ctl->job_pool.blk_num; i++) {
		CTL_VPE_JOB *p_job;

		p_job = (CTL_VPE_JOB *)(p_ctl->job_pool.start_addr + (p_ctl->job_pool.blk_size * i));
		p_job->tag = MAKEFOURCC('J', 'O', 'B', 'V');
		VOS_INIT_LIST_HEAD(&p_job->list);
		vos_list_add_tail(&p_job->list, &p_ctl->job_pool.free_list_head);
	}

	/* debug information */
	snprintf(p_ctl->dbg_ts_pool.name, 16, "dbg ts node");
	vk_spin_lock_init(&p_ctl->dbg_ts_pool.lock);
	p_ctl->dbg_ts_pool.blk_num = CTL_VPE_DBG_TS_NODE_MAX_NUM;
	p_ctl->dbg_ts_pool.blk_size = sizeof(CTL_VPE_DBG_TS_NODE);
	p_ctl->dbg_ts_pool.total_size = p_ctl->dbg_ts_pool.blk_num * p_ctl->dbg_ts_pool.blk_size;
	p_ctl->dbg_ts_pool.cur_free_num = 0;
	p_ctl->dbg_ts_pool.max_used_num = CTL_VPE_DBG_TS_NODE_MAX_NUM;
	p_ctl->dbg_ts_pool.start_addr = cur_buf_addr;
	cur_buf_addr += p_ctl->dbg_ts_pool.total_size;

	VOS_INIT_LIST_HEAD(&p_ctl->dbg_ts_pool.free_list_head);
	VOS_INIT_LIST_HEAD(&p_ctl->dbg_ts_pool.used_list_head);
	for (i = 0; i < p_ctl->dbg_ts_pool.blk_num; i++) {
		CTL_VPE_DBG_TS_NODE *p_node;

		p_node = (CTL_VPE_DBG_TS_NODE *)(p_ctl->dbg_ts_pool.start_addr + (p_ctl->dbg_ts_pool.blk_size * i));
		memset((void *)p_node, 0, sizeof(CTL_VPE_DBG_TS_NODE));
		VOS_INIT_LIST_HEAD(&p_node->list);
		vos_list_add_tail(&p_node->list, &p_ctl->dbg_ts_pool.used_list_head);
	}

	/* default debug config */
	p_ctl->dbg_eng_hang.dump_info = TRUE;

	/* check buffer usage */
	if ((buf_addr + buf_size) < cur_buf_addr) {
		DBG_ERR("buffer overflow\r\n");
		return CTL_VPE_E_NOMEM;
	}

	/* thread resuem */
	THREAD_CREATE(p_ctl->proc_tsk_id, ctl_vpe_proc_tsk, NULL, "ctl_vpe_proc_tsk");
	if (p_ctl->proc_tsk_id == 0) {
		DBG_ERR("proc thread create failed\r\n");
		return CTL_VPE_E_SYS;
	}
	vos_flag_create(&p_ctl->proc_tsk_flg_id, NULL, "ctl_vpe_proc_tsk_flg");
	vos_flag_set(p_ctl->proc_tsk_flg_id, CTL_VPE_PROC_TASK_INIT);
	THREAD_SET_PRIORITY(p_ctl->proc_tsk_id, CTL_VPE_TASK_PRIORITY);
	THREAD_RESUME(p_ctl->proc_tsk_id);
	if (ctl_vpe_int_proc_tsk_resume() < 0) {
		DBG_ERR("proc thread resume failed\r\n");
		return CTL_VPE_E_SYS;
	}

	THREAD_CREATE(p_ctl->cb_tsk_id, ctl_vpe_cb_tsk, NULL, "ctl_vpe_cb_tsk");
	if (p_ctl->proc_tsk_id == 0) {
		DBG_ERR("cb thread create failed\r\n");
		return CTL_VPE_E_SYS;
	}
	vos_flag_create(&p_ctl->cb_tsk_flg_id, NULL, "ctl_vpe_cb_tsk_flg");
	vos_flag_set(p_ctl->cb_tsk_flg_id, CTL_VPE_CB_TASK_INIT);
	THREAD_SET_PRIORITY(p_ctl->cb_tsk_id, CTL_VPE_TASK_PRIORITY);
	THREAD_RESUME(p_ctl->cb_tsk_id);
	if (ctl_vpe_int_cb_tsk_resume() < 0) {
		DBG_ERR("cb thread resume failed\r\n");
		return CTL_VPE_E_SYS;
	}

	return CTL_VPE_E_OK;
}

INT32 ctl_vpe_uninit(void)
{
	UINT32 i;
	CTL_VPE_CTL *p_ctl;
	CTL_VPE_HANDLE *p_hdl;

	p_ctl = &g_ctl_vpe_ctl;

	if (ctl_vpe_int_proc_tsk_pause() < 0) {
		DBG_ERR("proc thread pause failed\r\n");
		return CTL_VPE_E_SYS;
	}
	if (ctl_vpe_int_cb_tsk_pause() < 0) {
		DBG_ERR("cb thread pause failed\r\n");
		return CTL_VPE_E_SYS;
	}

	if (ctl_vpe_int_proc_tsk_close() < 0) {
		THREAD_DESTROY(p_ctl->proc_tsk_id);
	}
	if (ctl_vpe_int_cb_tsk_close() < 0) {
		THREAD_DESTROY(p_ctl->cb_tsk_id);
	}

	vos_flag_destroy(p_ctl->in_evt_que.flg_id);
	vos_flag_destroy(p_ctl->cb_evt_que.flg_id);
	vos_flag_destroy(p_ctl->proc_tsk_flg_id);
	vos_flag_destroy(p_ctl->cb_tsk_flg_id);

	for (i = 0; i < p_ctl->hdl_pool.blk_num; i++) {
		p_hdl = (CTL_VPE_HANDLE *)(p_ctl->hdl_pool.start_addr + (p_ctl->hdl_pool.blk_size * i));
		vos_sem_destroy(p_hdl->sem);
		memset((void *)p_hdl, 0, sizeof(CTL_VPE_HANDLE));
	}
	p_ctl->hdl_pool.start_addr = 0;
	VOS_INIT_LIST_HEAD(&p_ctl->hdl_pool.free_list_head);
	VOS_INIT_LIST_HEAD(&p_ctl->hdl_pool.used_list_head);

	p_ctl->job_pool.start_addr = 0;
	VOS_INIT_LIST_HEAD(&p_ctl->job_pool.free_list_head);
	VOS_INIT_LIST_HEAD(&p_ctl->job_pool.used_list_head);

	p_ctl->in_evt_que.start_addr = 0;
	VOS_INIT_LIST_HEAD(&p_ctl->in_evt_que.free_list_head);
	VOS_INIT_LIST_HEAD(&p_ctl->in_evt_que.used_list_head);

	p_ctl->cb_evt_que.start_addr = 0;
	VOS_INIT_LIST_HEAD(&p_ctl->cb_evt_que.free_list_head);
	VOS_INIT_LIST_HEAD(&p_ctl->cb_evt_que.used_list_head);

	return CTL_VPE_E_OK;
}

UINT32 ctl_vpe_open(CHAR *name)
{
	unsigned long loc_flg;
	CTL_VPE_HANDLE *p_hdl;

	if (kdrv_vpe_if_is_init() == FALSE) {
		DBG_ERR("kdrv_vpe not init\r\n");
		return 0;
	}

	p_hdl = ctl_vpe_int_alloc_handle();
	if (p_hdl) {
		vos_sem_wait(p_hdl->sem);
		vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
		p_hdl->tag = MAKEFOURCC('V', 'P', 'E', 'V');
		snprintf(p_hdl->name, 16, name);
		p_hdl->p_fired_job = NULL;
		p_hdl->in_job_cnt = 0;
		p_hdl->out_job_cnt = 0;
		p_hdl->out_job_err_cnt = 0;
		p_hdl->out_job_inq_cnt = 0;
		atomic_set(&p_hdl->dbg_err_accu_info.err_sys, 0);
		atomic_set(&p_hdl->dbg_err_accu_info.err_par, 0);
		atomic_set(&p_hdl->dbg_err_accu_info.err_isp_par, 0);
		atomic_set(&p_hdl->dbg_err_accu_info.err_nomem, 0);
		atomic_set(&p_hdl->dbg_err_accu_info.err_indata, 0);
		atomic_set(&p_hdl->dbg_err_accu_info.err_kdrv_trig, 0);
		atomic_set(&p_hdl->dbg_err_accu_info.err_kdrv_drop, 0);
		p_hdl->isp_id = CTL_VPE_ISP_ID_UNUSED;
		memset((void *)&p_hdl->rtc_info, 0, sizeof(CTL_VPE_BASEINFO));
		memset((void *)&p_hdl->ctl_info, 0, sizeof(CTL_VPE_BASEINFO));
		memset((void *)&p_hdl->isp_param, 0, sizeof(CTL_VPE_ISP_PARAM));
		p_hdl->isp_param.dce_2dlut_param.p_lut = NULL;
		p_hdl->kdrv_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOPROCS_VPE_ENGINE0, 0);
		vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

		ctl_vpe_int_kdrv_open();
		vos_sem_sig(p_hdl->sem);
	}

	return (UINT32)p_hdl;
}

INT32 ctl_vpe_close(UINT32 hdl)
{
	unsigned long loc_flg;
	CTL_VPE_HANDLE *p_hdl;
	INT32 rt;
	FLGPTN wait_flg = 0;

	p_hdl = (CTL_VPE_HANDLE *)hdl;
	if (ctl_vpe_int_valid_handle(p_hdl) == FALSE) {
		DBG_ERR("invalid handle\r\n");
		return CTL_VPE_E_ID;
	}

	vos_sem_wait(p_hdl->sem);
	/* set handle tag 0 to block set/get/ioctl */
	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	p_hdl->tag = 0;
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

	/* task flow lock */
	if (vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_LOCK, TWF_CLR, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS)) != 0) {
		ctl_vpe_dbg_err("wait proc tsk timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.proc_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_LOCK);
	}
	if (vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_LOCK, TWF_CLR, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS)) != 0) {
		ctl_vpe_dbg_err("wait cb tsk timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.cb_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_LOCK);
	}

	/* flush queue */
	ctl_vpe_int_simple_w_lock(&p_hdl->sndevt_rwlock, &p_hdl->lock);
	rt = ctl_vpe_int_proc_tsk_pause();
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("pause proc task failed\r\n");
		ctl_vpe_int_simple_w_unlock(&p_hdl->sndevt_rwlock, &p_hdl->lock);
		goto CTL_VPE_CLOSE_FLUSH_END;
	}
	rt = ctl_vpe_int_cb_tsk_pause();
	if (rt != CTL_VPE_E_OK) {
		if (ctl_vpe_int_proc_tsk_resume() != CTL_VPE_E_OK) {
			DBG_ERR("resume proc task failed\r\n");
		}
		DBG_ERR("pause cb task failed\r\n");
		ctl_vpe_int_simple_w_unlock(&p_hdl->sndevt_rwlock, &p_hdl->lock);
		goto CTL_VPE_CLOSE_FLUSH_END;
	}
	ctl_vpe_int_simple_w_unlock(&p_hdl->sndevt_rwlock, &p_hdl->lock);

	/* flush all event & buffer */
	ctl_vpe_msg_flush(&g_ctl_vpe_ctl.in_evt_que, p_hdl, ctl_vpe_in_evt_process);
	ctl_vpe_msg_flush(&g_ctl_vpe_ctl.cb_evt_que, p_hdl, ctl_vpe_cb_evt_process);

	rt = ctl_vpe_int_cb_tsk_resume();
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("resume cb task failed\r\n");
		goto CTL_VPE_CLOSE_FLUSH_END;
	}
	rt = ctl_vpe_int_proc_tsk_resume();
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("resume proc task failed\r\n");
		goto CTL_VPE_CLOSE_FLUSH_END;
	}

	/* task flow unlock */
	vos_flag_set(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_LOCK);
	vos_flag_set(g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_LOCK);

CTL_VPE_CLOSE_FLUSH_END:
	/* release handle */
	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	p_hdl->isp_param.dce_2dlut_param.p_lut = NULL;
	ctl_vpe_int_free_handle(p_hdl);
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

	/* close kdrv_vpe */
	ctl_vpe_int_kdrv_close();

	vos_sem_sig(p_hdl->sem);

	return CTL_VPE_E_OK;
}

#if 0
#endif

static INT32 ctl_vpe_set_reg_cb(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_REG_CB_INFO *p_cb;

	p_cb = (CTL_VPE_REG_CB_INFO *)data;

	if (p_cb->cbevt >= CTL_VPE_CBEVT_MAX) {
		DBG_WRN("reg cb evt %d overflow, max is %d\n", p_cb->cbevt, CTL_VPE_CBEVT_MAX);
		return CTL_VPE_E_PAR;
	}

	p_hdl->cb_fp[p_cb->cbevt] = p_cb->fp;

	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_set_isp_id(CTL_VPE_HANDLE *p_hdl, void *data)
{
	p_hdl->isp_id = *(UINT32 *)data;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_set_flush(CTL_VPE_HANDLE *p_hdl, void *data)
{
	/*
		1. flush in_evt queue, pause task first
		2. flush cb_evt queue, pause task first
		3. need to wait job in kdrv done, kflow used kdrv as blocking mode
		4. flush specific path
			in_evt: release input buffer only
			cb_evt: unlock input buffer & path buffer & job
		to pause task and wait job done, unlock handle first
	*/
	unsigned long loc_flg;
	CTL_VPE_FLUSH_CONFIG *p_cfg;
	UINT32 i;
	FLGPTN wait_flg = 0;
	INT32 rt = CTL_VPE_E_OK;

	if (data) {
		p_cfg = (CTL_VPE_FLUSH_CONFIG *)data;
		if (p_cfg->pid >= CTL_VPE_OUT_PATH_ID_MAX) {
			return CTL_VPE_E_PAR;
		}
	} else {
		p_cfg = NULL;
	}

	/* check path disable first, only flush when path disable */
	rt = CTL_VPE_E_OK;
	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	if (p_cfg) {
		if (p_hdl->ctl_info.out_path[p_cfg->pid].enable) {
			DBG_ERR("flush pid %d, must disable path before flush\r\n", p_cfg->pid);
			rt = CTL_VPE_E_SYS;
		}
	} else {
		for (i = 0; i < CTL_VPE_OUT_PATH_ID_MAX; i++) {
			if (p_hdl->ctl_info.out_path[i].enable) {
				DBG_ERR("flush all, must disable all path before flush, path %d is now enable\r\n", i);
				rt = CTL_VPE_E_SYS;
			}
		}
	}
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
	if (rt != CTL_VPE_E_OK) {
		return rt;
	}

	/* task flow lock */
	if (vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_LOCK, TWF_CLR, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS)) != 0) {
		ctl_vpe_dbg_err("wait proc tsk timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.proc_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_LOCK);
	}
	if (vos_flag_wait_timeout(&wait_flg, g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_LOCK, TWF_CLR, vos_util_msec_to_tick(CTL_VPE_TASK_TIMEOUT_MS)) != 0) {
		ctl_vpe_dbg_err("wait cb tsk timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(g_ctl_vpe_ctl.cb_tsk_flg_id, 0xffffffff), CTL_VPE_TASK_LOCK);
	}

	/* pause task */
	ctl_vpe_int_simple_w_lock(&p_hdl->sndevt_rwlock, &p_hdl->lock);
	rt = ctl_vpe_int_proc_tsk_pause();
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("pause proc task failed\r\n");
		ctl_vpe_int_simple_w_unlock(&p_hdl->sndevt_rwlock, &p_hdl->lock);
		return CTL_VPE_E_SYS;
	}

	rt = ctl_vpe_int_cb_tsk_pause();
	if (rt != CTL_VPE_E_OK) {
		ctl_vpe_int_proc_tsk_resume();
		DBG_ERR("pause cb task failed\r\n");
		ctl_vpe_int_simple_w_unlock(&p_hdl->sndevt_rwlock, &p_hdl->lock);
		return CTL_VPE_E_SYS;
	}
	ctl_vpe_int_simple_w_unlock(&p_hdl->sndevt_rwlock, &p_hdl->lock);

	/* flush event & buffer */
	if (p_cfg) {
		/* flush path */
		ctl_vpe_msg_modify(&g_ctl_vpe_ctl.cb_evt_que, p_hdl, ctl_vpe_cb_evt_flush_by_path, p_cfg);
	} else {
		/* flush all */
		ctl_vpe_msg_flush(&g_ctl_vpe_ctl.in_evt_que, p_hdl, ctl_vpe_in_evt_process);
		ctl_vpe_msg_flush(&g_ctl_vpe_ctl.cb_evt_que, p_hdl, ctl_vpe_cb_evt_process);
	}

	/* resume task */
	rt = ctl_vpe_int_cb_tsk_resume();
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("resume cb task failed\r\n");
		return CTL_VPE_E_SYS;
	}

	rt = ctl_vpe_int_proc_tsk_resume();
	if (rt != CTL_VPE_E_OK) {
		DBG_ERR("resume proc task failed\r\n");
		return CTL_VPE_E_SYS;
	}

	/* task flow unlock */
	vos_flag_set(g_ctl_vpe_ctl.proc_tsk_flg_id, CTL_VPE_TASK_LOCK);
	vos_flag_set(g_ctl_vpe_ctl.cb_tsk_flg_id, CTL_VPE_TASK_LOCK);

	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_set_apply(CTL_VPE_HANDLE *p_hdl, void *data)
{
	p_hdl->ctl_info = p_hdl->rtc_info;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_set_in_crop(CTL_VPE_HANDLE *p_hdl, void *data)
{
	p_hdl->rtc_info.in_crop = *(CTL_VPE_IN_CROP *)data;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_set_out_path(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_OUT_PATH *p_path = (CTL_VPE_OUT_PATH *)data;

	if (p_path->pid >= CTL_VPE_OUT_PATH_ID_MAX) {
		return CTL_VPE_E_PAR;
	}

	p_hdl->rtc_info.out_path[p_path->pid] = *p_path;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_set_private_buf(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_PRIVATE_BUF *p_buf = (CTL_VPE_PRIVATE_BUF *)data;

	p_hdl->isp_param.dce_2dlut_param.lut_buf_sz = p_buf->buf_size;
	p_hdl->isp_param.dce_2dlut_param.p_lut = (UINT32 *)p_buf->buf_addr;

	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_set_out_path_halign(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_OUT_PATH_HALIGN *p_path = (CTL_VPE_OUT_PATH_HALIGN *)data;

	if (p_path->pid >= CTL_VPE_OUT_PATH_ID_MAX) {
		return CTL_VPE_E_PAR;
	}

	p_hdl->rtc_info.out_path_h_align[p_path->pid] = p_path->align;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_set_dcs(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_DCS_MODE dcs_mode = *(CTL_VPE_DCS_MODE *)data;

	if (dcs_mode < CTL_VPE_DCS_MAX) {
		p_hdl->rtc_info.dcs_mode = dcs_mode;
		return CTL_VPE_E_OK;
	} else {
		DBG_ERR("dcs mode %d overflow, max (%d)\r\n", dcs_mode, CTL_VPE_DCS_MAX);
		return CTL_VPE_E_PAR;
	}
}

static CTL_VPE_SET_FP g_ctl_vpe_set_tab[CTL_VPE_ITEM_MAX] = {
	ctl_vpe_set_reg_cb,
	ctl_vpe_set_isp_id,
	ctl_vpe_set_flush,
	ctl_vpe_set_apply,
	ctl_vpe_set_in_crop,
	ctl_vpe_set_out_path,
	NULL,
	ctl_vpe_set_private_buf,
	ctl_vpe_set_out_path_halign,
	ctl_vpe_set_dcs,
};

INT32 ctl_vpe_set(UINT32 hdl, UINT32 item, void *data)
{
	unsigned long loc_flg;
	CTL_VPE_HANDLE *p_hdl;
	INT32 rt;

	p_hdl = (CTL_VPE_HANDLE *)hdl;

	if (ctl_vpe_int_valid_handle(p_hdl) == FALSE) {
		return CTL_VPE_E_ID;
	}

	if (item >= CTL_VPE_ITEM_MAX) {
		return CTL_VPE_E_PAR;
	}

	vos_sem_wait(p_hdl->sem);
	rt = CTL_VPE_E_OK;
	if (g_ctl_vpe_set_tab[item]) {
		if (item == CTL_VPE_ITEM_FLUSH) {
			/* special case for flush, lock control in flush_api */
			rt = g_ctl_vpe_set_tab[item](p_hdl, data);
		} else {
			vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
			rt = g_ctl_vpe_set_tab[item](p_hdl, data);
			vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
		}
	}

	/* isp reset callback, not in set api to prevent lock and callback */
	if (item == CTL_VPE_ITEM_ALGID_IMM && rt == CTL_VPE_E_OK) {
		ctl_vpe_isp_event_cb(p_hdl->isp_id, ISP_EVENT_PARAM_RST, NULL, NULL);
	}
	vos_sem_sig(p_hdl->sem);

	return rt;
}

#if 0
#endif

static INT32 ctl_vpe_get_isp_id(CTL_VPE_HANDLE *p_hdl, void *data)
{
	*(UINT32 *)data = p_hdl->isp_id;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_get_in_crop(CTL_VPE_HANDLE *p_hdl, void *data)
{
	*(CTL_VPE_IN_CROP *)data = p_hdl->ctl_info.in_crop;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_get_out_path(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_OUT_PATH *p_path = (CTL_VPE_OUT_PATH *)data;

	if (p_path->pid >= CTL_VPE_OUT_PATH_ID_MAX) {
		return CTL_VPE_E_PAR;
	}
	*p_path = p_hdl->ctl_info.out_path[p_path->pid];

	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_get_pushevt_inq(CTL_VPE_HANDLE *p_hdl, void *data)
{
	*(UINT32 *)data = p_hdl->out_job_inq_cnt;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_get_private_buf(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_PRIVATE_BUF *p_buf = (CTL_VPE_PRIVATE_BUF *)data;

	p_buf->buf_size = 0;

	/* Calculate 2dlut buffer base on isp param */
	/* prepare 2dlut private buf only when dce mode select 2dlut only by IVOT_N12126_CO-322 */
	if (p_hdl->isp_param.dce_ctl.dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY) {
		switch (p_hdl->isp_param.dce_2dlut_param.lut_sz) {
		case CTL_VPE_ISP_2DLUT_SZ_9X9:
			p_buf->buf_size += ALIGN_CEIL((12*9*4), VOS_ALIGN_BYTES);
			break;

		case CTL_VPE_ISP_2DLUT_SZ_65X65:
			p_buf->buf_size += ALIGN_CEIL((68*65*4), VOS_ALIGN_BYTES);
			break;

		case CTL_VPE_ISP_2DLUT_SZ_129X129:
			p_buf->buf_size += ALIGN_CEIL((132*129*4), VOS_ALIGN_BYTES);
			break;

		case CTL_VPE_ISP_2DLUT_SZ_257X257:
			p_buf->buf_size += ALIGN_CEIL((260*257*4), VOS_ALIGN_BYTES);
			break;

		default:
			DBG_ERR("unknown 2dlut size select %d\r\n", p_hdl->isp_param.dce_2dlut_param.lut_sz);
			return CTL_VPE_E_PAR;
		}
	}

	/* set private buf cfg parameters to handle for debug dump */
	p_hdl->pri_buf_cfg_param.dce_en = p_hdl->isp_param.dce_ctl.enable;
	p_hdl->pri_buf_cfg_param.dce_mode = p_hdl->isp_param.dce_ctl.dce_mode;
	p_hdl->pri_buf_cfg_param.lut_sz = p_hdl->isp_param.dce_2dlut_param.lut_sz;
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_get_out_path_halign(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_OUT_PATH_HALIGN *p_path = (CTL_VPE_OUT_PATH_HALIGN *)data;

	if (p_path->pid >= CTL_VPE_OUT_PATH_ID_MAX) {
		return CTL_VPE_E_PAR;
	}

	p_path->align = p_hdl->ctl_info.out_path_h_align[p_path->pid];
	return CTL_VPE_E_OK;
}

static INT32 ctl_vpe_get_dcs(CTL_VPE_HANDLE *p_hdl, void *data)
{
	*(UINT32 *)data = p_hdl->ctl_info.dcs_mode;
	return CTL_VPE_E_OK;
}

static CTL_VPE_GET_FP g_ctl_vpe_get_tab[CTL_VPE_ITEM_MAX] = {
	NULL,
	ctl_vpe_get_isp_id,
	NULL,
	NULL,
	ctl_vpe_get_in_crop,
	ctl_vpe_get_out_path,
	ctl_vpe_get_pushevt_inq,
	ctl_vpe_get_private_buf,
	ctl_vpe_get_out_path_halign,
	ctl_vpe_get_dcs,
};

INT32 ctl_vpe_get(UINT32 hdl, UINT32 item, void *data)
{
	unsigned long loc_flg;
	CTL_VPE_HANDLE *p_hdl;
	INT32 rt;

	p_hdl = (CTL_VPE_HANDLE *)hdl;

	if (ctl_vpe_int_valid_handle(p_hdl) == FALSE) {
		return CTL_VPE_E_ID;
	}

	if (item >= CTL_VPE_ITEM_MAX) {
		return CTL_VPE_E_PAR;
	}

	vos_sem_wait(p_hdl->sem);
	rt = CTL_VPE_E_OK;
	if (g_ctl_vpe_get_tab[item]) {
		vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
		rt = g_ctl_vpe_get_tab[item](p_hdl, data);
		vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
	}
	vos_sem_sig(p_hdl->sem);

	return rt;
}

#if 0
#endif

static INT32 ctl_vpe_ioctl_sndevt(CTL_VPE_HANDLE *p_hdl, void *data)
{
	CTL_VPE_EVT *p_evt;
	INT32 rt;

	rt = ctl_vpe_int_simple_r_lock(&p_hdl->sndevt_rwlock, &p_hdl->lock);
	if (rt != E_OK) {
		return CTL_VPE_E_SYS;
	}
	p_evt = (CTL_VPE_EVT *)data;
	rt = ctl_vpe_msg_snd(CTL_VPE_MSG_PROCESS, (UINT32)p_hdl, p_evt->data_addr, p_evt->buf_id, &g_ctl_vpe_ctl.in_evt_que);
	if (rt == CTL_VPE_E_OK) {
		p_hdl->in_job_cnt++;
	}
	ctl_vpe_int_simple_r_unlock(&p_hdl->sndevt_rwlock, &p_hdl->lock);

	return rt;
}

static CTL_VPE_IOCTL_FP g_ctl_vpe_ioctl_tab[CTL_VPE_IOCTL_MAX] = {
	ctl_vpe_ioctl_sndevt,
};

INT32 ctl_vpe_ioctl(UINT32 hdl, UINT32 item, void *data)
{
	CTL_VPE_HANDLE *p_hdl;
	INT32 rt;

	p_hdl = (CTL_VPE_HANDLE *)hdl;

	if (ctl_vpe_int_valid_handle(p_hdl) == FALSE) {
		return CTL_VPE_E_ID;
	}

	if (item >= CTL_VPE_IOCTL_MAX) {
		return CTL_VPE_E_PAR;
	}

	rt = CTL_VPE_E_OK;
	if (g_ctl_vpe_ioctl_tab[item]) {
		rt = g_ctl_vpe_ioctl_tab[item](p_hdl, data);
	}

	return rt;
}

#if 0
#endif

void ctl_vpe_dump_all(int (*dump)(const char *fmt, ...))
{
	unsigned long loc_flg;
	CTL_VPE_MEM_POOL *p_pool[5];
	CTL_VPE_LIST_HEAD *p_list;
	CTL_VPE_LIST_HEAD *n;
	UINT32 i;

	if (dump == NULL) {
		dump = ctl_vpe_int_printf;
	}

	dump("\r\n----- ctl_vpe dump all -----\r\n");
	/* mem pool info */
	p_pool[0] = &g_ctl_vpe_ctl.in_evt_que;
	p_pool[1] = &g_ctl_vpe_ctl.cb_evt_que;
	p_pool[2] = &g_ctl_vpe_ctl.hdl_pool;
	p_pool[3] = &g_ctl_vpe_ctl.job_pool;
	p_pool[4] = &g_ctl_vpe_ctl.dbg_ts_pool;
	dump("%16s %16s %16s %16s %16s %16s %16s\r\n", "resource", "start_addr", "total_size", "blk_size", "blk_num", "max_used_num", "cur_free_num");
	for (i = 0; i < 5; i++) {
		dump("%16s %6s0x%.8x %6s0x%.8x %6s0x%.8x %16d %16d %16d\r\n",
			p_pool[i]->name,
			"", (unsigned int)p_pool[i]->start_addr,
			"", (unsigned int)p_pool[i]->total_size,
			"", (unsigned int)p_pool[i]->blk_size,
			(int)p_pool[i]->blk_num,
			(int)p_pool[i]->max_used_num,
			(int)p_pool[i]->cur_free_num);
	}
	dump("\r\n");

	if (g_ctl_vpe_ctl.in_evt_que.start_addr != 0) {
		vk_spin_lock_irqsave(&g_ctl_vpe_ctl.in_evt_que.lock, loc_flg);
		dump("---- input queue ----\r\n");
		dump("%16s %8s %16s %16s %16s %10s\r\n", "name", "buf_id", "vd_cnt", "vd_ts", "snd_ts", "addr");
		list_for_each_safe(p_list, n, &g_ctl_vpe_ctl.in_evt_que.used_list_head) {
			CTL_VPE_MSG_EVENT *p_evt;
			CTL_VPE_HANDLE *p_hdl;
			VDO_FRAME *p_frm;

			p_evt = list_entry(p_list, CTL_VPE_MSG_EVENT, list);
			p_hdl = (CTL_VPE_HANDLE *)p_evt->param[0];
			p_frm = (VDO_FRAME *)p_evt->param[1];
			dump("%16s %8u %16u %16u %16u 0x%.8x\r\n",
				p_hdl->name, p_evt->param[2], (unsigned int)p_frm->count, (unsigned int)p_frm->timestamp, (unsigned int)p_evt->timestamp, (unsigned int)p_frm->addr[0]);
		}
		vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.in_evt_que.lock, loc_flg);
		dump("\r\n");
	}

	if (g_ctl_vpe_ctl.cb_evt_que.start_addr != 0) {
		vk_spin_lock_irqsave(&g_ctl_vpe_ctl.cb_evt_que.lock, loc_flg);
		dump("---- output queue ----\r\n");
		dump("%16s %8s %16s %16s %16s %10s\r\n", "name", "buf_id", "vd_cnt", "vd_ts", "snd_ts", "addr");
		list_for_each_safe(p_list, n, &g_ctl_vpe_ctl.cb_evt_que.used_list_head) {
			CTL_VPE_MSG_EVENT *p_evt;
			CTL_VPE_HANDLE *p_hdl;
			CTL_VPE_JOB *p_job;
			VDO_FRAME *p_frm;

			p_evt = list_entry(p_list, CTL_VPE_MSG_EVENT, list);
			p_hdl = (CTL_VPE_HANDLE *)p_evt->param[0];
			p_job = (CTL_VPE_JOB *)p_evt->param[1];
			p_frm = (VDO_FRAME *)p_job->in_evt.data_addr;
			dump("%16s %8u %16u %16u %16u 0x%.8x\r\n",
				p_hdl->name, p_job->in_evt.buf_id, (unsigned int)p_frm->count, (unsigned int)p_frm->timestamp, (unsigned int)p_evt->timestamp, (unsigned int)p_frm->addr[0]);
		}
		vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.cb_evt_que.lock, loc_flg);
		dump("\r\n");
	}

	if (g_ctl_vpe_ctl.hdl_pool.start_addr != 0) {
		vk_spin_lock_irqsave(&g_ctl_vpe_ctl.hdl_pool.lock, loc_flg);
		dump("---- handle ----\r\n");
		dump("%16s %12s %12s(%12s) %12s %10s err(%8s %8s %8s %8s %8s %10s %10s)\r\n", "name", "in_job_cnt", "out_job_cnt", "out_job_err", "out_job_inq", "isp_id",
		"sys", "par", "isp_par", "nomem", "indata", "kdrv_trig", "kdrv_drop");
		dump("used handle list\r\n");
		vos_list_for_each_safe(p_list, n, &g_ctl_vpe_ctl.hdl_pool.used_list_head) {
			CTL_VPE_HANDLE *p_hdl;

			p_hdl = list_entry(p_list, CTL_VPE_HANDLE, list);
			dump("%16s %12d %12d(%12d) %12d 0x%.8x err(%8d %8d %8d %8d %8d %10d %10d)\r\n",
				p_hdl->name, p_hdl->in_job_cnt, p_hdl->out_job_cnt, p_hdl->out_job_err_cnt, p_hdl->out_job_inq_cnt, (unsigned int)p_hdl->isp_id,
				atomic_read(&p_hdl->dbg_err_accu_info.err_sys), 	atomic_read(&p_hdl->dbg_err_accu_info.err_par), 	atomic_read(&p_hdl->dbg_err_accu_info.err_isp_par),
				atomic_read(&p_hdl->dbg_err_accu_info.err_nomem),	atomic_read(&p_hdl->dbg_err_accu_info.err_indata),	atomic_read(&p_hdl->dbg_err_accu_info.err_kdrv_trig),
				atomic_read(&p_hdl->dbg_err_accu_info.err_kdrv_drop));
		}
		dump("\r\n");
		dump("free handle list\r\n");
		vos_list_for_each_safe(p_list, n, &g_ctl_vpe_ctl.hdl_pool.free_list_head) {
			CTL_VPE_HANDLE *p_hdl;

			p_hdl = list_entry(p_list, CTL_VPE_HANDLE, list);
			dump("%16s %16d %16d(%16d) %16d 0x%.8x\r\n",
				p_hdl->name, p_hdl->in_job_cnt, p_hdl->out_job_cnt, p_hdl->out_job_err_cnt, p_hdl->out_job_inq_cnt, (unsigned int)p_hdl->isp_id);
		}

		dump("\r\n");
		vos_list_for_each_safe(p_list, n, &g_ctl_vpe_ctl.hdl_pool.used_list_head) {
			CTL_VPE_HANDLE *p_hdl;

			p_hdl = list_entry(p_list, CTL_VPE_HANDLE, list);
			dump("----- %s@0x%.8x dump config -----\r\n", p_hdl->name, (unsigned int)(p_hdl));

			dump("\r\n----- private buf cfg param -----\r\n");
			dump("%16s %10s %8s %10s %10s\r\n", "dce_en", "dce_mode", "lut_sz", "addr", "size");
			dump("%16d %10d %8d 0x%.8x 0x%.8x\r\n",
				p_hdl->pri_buf_cfg_param.dce_en,
				p_hdl->pri_buf_cfg_param.dce_mode,
				p_hdl->pri_buf_cfg_param.lut_sz,
				(unsigned int)p_hdl->isp_param.dce_2dlut_param.p_lut,
				(unsigned int)p_hdl->isp_param.dce_2dlut_param.lut_buf_sz);

			ctl_vpe_process_dbg_dump_kflow_cfg(&p_hdl->ctl_info, dump);
			ctl_vpe_process_dbg_dump_kflow_isp_param(&p_hdl->isp_param, dump);
		}
		vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.hdl_pool.lock, loc_flg);
	}
	dump("\r\n");

	ctl_vpe_isp_dump(dump);
	ctl_vpe_dump_job_ts(dump);
	dump("\r\n----- ctl_vpe dump all end -----\r\n");
}

void ctl_vpe_dump_job_ts(int (*dump)(const char *fmt, ...))
{
	unsigned long loc_flg;
	CTL_VPE_LIST_HEAD *p_list;
	CTL_VPE_LIST_HEAD *n;
	CTL_VPE_HANDLE *p_hdl;
	CTL_VPE_DBG_TS_NODE *p_node;
	UINT32 diff[CTL_VPE_DBG_TS_MAX - 1];
	UINT32 i;

	if (dump == NULL) {
		dump = ctl_vpe_int_printf;
	}

	/* special used to lock set_job_ts flow */
	vk_spin_lock_irqsave(&g_ctl_vpe_ctl.common_lock, loc_flg);
	g_ctl_vpe_ctl.dbg_ts_pool.cur_free_num = 1;
	vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.common_lock, loc_flg);

	/* dump timestamp */
	vk_spin_lock_irqsave(&g_ctl_vpe_ctl.dbg_ts_pool.lock, loc_flg);
	dump("\r\n----- ctl vpe dump timestamp -----\r\n");
	dump("%16s : %4s %8s %12s %20s %20s %20s %20s %20s\r\n",
				"name", "err", "count", "sndevt", "job_alloc", "config_end", "job_end", "cb_task_start", "cb_task_end");
	vos_list_for_each_safe(p_list, n, &g_ctl_vpe_ctl.dbg_ts_pool.used_list_head) {
		p_node = list_entry(p_list, CTL_VPE_DBG_TS_NODE, list);
		p_hdl = (CTL_VPE_HANDLE *)p_node->handle;

		if (p_node->status == 0)
			continue;

		for (i = 1; i < CTL_VPE_DBG_TS_MAX; i++) {
			if (p_node->ts_flow[i] == 0) {
				p_node->ts_flow[i] = p_node->ts_flow[i - 1];
			}
		}

		for (i = 0; i < (CTL_VPE_DBG_TS_MAX - 1); i++) {
			if (p_node->ts_flow[i + 1] < p_node->ts_flow[i])
				diff[i] = 0;
			else
				diff[i] = p_node->ts_flow[i + 1] - p_node->ts_flow[i];
		}

		dump("%16s : %4d %8u %12u %12u(%-+6d) %12u(%-+6d) %12u(%-+6d) %12u(%-+6d) %12u(%-+6d)\r\n",
				p_hdl->name,
				(int)p_node->err,
				(unsigned int)p_node->input_frm_count,
				(unsigned int)p_node->ts_flow[0],
				(unsigned int)p_node->ts_flow[1], (int)diff[0],
				(unsigned int)p_node->ts_flow[2], (int)diff[1],
				(unsigned int)p_node->ts_flow[3], (int)diff[2],
				(unsigned int)p_node->ts_flow[4], (int)diff[3],
				(unsigned int)p_node->ts_flow[5], (int)diff[4]);
	}
	vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.dbg_ts_pool.lock, loc_flg);

	/* special used to unlock set_job_ts flow */
	vk_spin_lock_irqsave(&g_ctl_vpe_ctl.common_lock, loc_flg);
	g_ctl_vpe_ctl.dbg_ts_pool.cur_free_num = 0;
	vk_spin_unlock_irqrestore(&g_ctl_vpe_ctl.common_lock, loc_flg);
}

void ctl_vpe_save_yuv_cfg(CHAR *name, CHAR *path, UINT32 save_input, UINT32 save_output)
{
	CTL_VPE_DBG_SAVEYUV_CFG *p_cfg;

	p_cfg = &g_ctl_vpe_ctl.dbg_saveyuv_cfg;
	p_cfg->enable = ENABLE;
	snprintf(p_cfg->name, 16, name);
	snprintf(p_cfg->path, 16, path);
	p_cfg->save_input = save_input;
	p_cfg->save_output = save_output;
}

void ctl_vpe_dbg_eng_hang_cfg(CTL_VPE_DBG_ENG_HANG *p_eng_hang)
{
	g_ctl_vpe_ctl.dbg_eng_hang = *p_eng_hang;
}
#endif
