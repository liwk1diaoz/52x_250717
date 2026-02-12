#if defined(__FREERTOS)
#include <stdio.h>
#include <string.h>
#endif
#include "kwrap/util.h"
#include "comm/hwclock.h"
#include "kdrv_ise_int_tsk.h"
#include "kdrv_ise_int.h"
#include "kdrv_ise_int_dbg.h"

static KDRV_ISE_CTL g_kdrv_ise_ctl;

static KDRV_ISE_HANDLE* kdrv_ise_int_get_handle(UINT32 chip_id, UINT32 eng_id)
{
	UINT32 idx;

	/* todo: chip_id & eng_id mapping */
	if (chip_id == KDRV_CHIP0 && eng_id == KDRV_GFX2D_ISE0) {
		idx = 0;
	} else {
		DBG_ERR("unsupport chip_id %d, eng_id 0x%.8x\r\n", (int)chip_id, (unsigned int)eng_id);
		return NULL;
	}
	if (idx < g_kdrv_ise_ctl.total_ch) {
		return &g_kdrv_ise_ctl.p_hdl[idx];
	}
	return NULL;
}

static KDRV_ISE_JOB_HEAD* kdrv_ise_int_job_head_alloc(void)
{
	KDRV_ISE_MEM_POOL *p_mem_pool;
	KDRV_ISE_JOB_HEAD *p_job_head;
	unsigned long loc_flg;
	UINT32 i;

	p_mem_pool = &g_kdrv_ise_ctl.job_head_pool;
	p_job_head = NULL;

	vk_spin_lock_irqsave(&p_mem_pool->lock, loc_flg);
	if (vos_list_empty(&p_mem_pool->blk_free_list_root) == FALSE) {
		p_job_head = vos_list_first_entry(&p_mem_pool->blk_free_list_root, KDRV_ISE_JOB_HEAD, pool_list);
		vos_list_del_init(&p_job_head->pool_list);
		vos_list_add_tail(&p_job_head->pool_list, &p_mem_pool->blk_used_list_root);
		p_mem_pool->cur_free_num -= 1;

		if ((p_mem_pool->blk_num - p_mem_pool->cur_free_num) > p_mem_pool->max_used_num) {
			p_mem_pool->max_used_num = p_mem_pool->blk_num - p_mem_pool->cur_free_num;
		}
	}
	vk_spin_unlock_irqrestore(&p_mem_pool->lock, loc_flg);

	if (p_job_head != NULL) {
		p_job_head->chip_id = 0;
		p_job_head->eng_id = 0;
		p_job_head->job_id = 0;
		p_job_head->job_num = 0;
		p_job_head->err_num = 0;
		p_job_head->done_num = 0;
		p_job_head->proc_num = 0;
		p_job_head->p_last_cfg = NULL;
		p_job_head->cb_fp.callback = NULL;
		p_job_head->cb_fp.reserve_buf = NULL;
		p_job_head->cb_fp.free_buf = NULL;

		VOS_INIT_LIST_HEAD(&p_job_head->job_cfg_root);
		VOS_INIT_LIST_HEAD(&p_job_head->ll_blk_root);
		VOS_INIT_LIST_HEAD(&p_job_head->proc_list);
		VOS_INIT_LIST_HEAD(&p_job_head->cb_list);

		for (i = 1; i < KDRV_ISE_JOB_TS_MAX; i++) {
			p_job_head->timestamp[i] = 0;
		}
		p_job_head->timestamp[KDRV_ISE_JOB_TS_GENNODE] = hwclock_get_counter();
	}

	return p_job_head;
}

static void kdrv_ise_int_job_head_free(KDRV_ISE_JOB_HEAD *p_job_head)
{
	KDRV_ISE_MEM_POOL *p_mem_pool;
	unsigned long loc_flg;

	p_mem_pool = &g_kdrv_ise_ctl.job_head_pool;
	vk_spin_lock_irqsave(&p_mem_pool->lock, loc_flg);
	vos_list_del_init(&p_job_head->pool_list);
	vos_list_add_tail(&p_job_head->pool_list, &p_mem_pool->blk_free_list_root);
	p_mem_pool->cur_free_num += 1;
	vk_spin_unlock_irqrestore(&p_mem_pool->lock, loc_flg);
}

static KDRV_ISE_JOB_CFG* kdrv_ise_int_job_cfg_alloc(void)
{
	KDRV_ISE_MEM_POOL *p_mem_pool;
	KDRV_ISE_JOB_CFG *p_job_cfg;
	unsigned long loc_flg;

	p_mem_pool = &g_kdrv_ise_ctl.job_cfg_pool;
	p_job_cfg = NULL;

	vk_spin_lock_irqsave(&p_mem_pool->lock, loc_flg);
	if (vos_list_empty(&p_mem_pool->blk_free_list_root) == FALSE) {
		p_job_cfg = vos_list_first_entry(&p_mem_pool->blk_free_list_root, KDRV_ISE_JOB_CFG, pool_list);
		vos_list_del_init(&p_job_cfg->pool_list);
		vos_list_add_tail(&p_job_cfg->pool_list, &p_mem_pool->blk_used_list_root);
		p_mem_pool->cur_free_num -= 1;

		if ((p_mem_pool->blk_num - p_mem_pool->cur_free_num) > p_mem_pool->max_used_num) {
			p_mem_pool->max_used_num = p_mem_pool->blk_num - p_mem_pool->cur_free_num;
		}
	}
	vk_spin_unlock_irqrestore(&p_mem_pool->lock, loc_flg);

	if (p_job_cfg != NULL) {
		memset((void *)&p_job_cfg->io_cfg, 0, sizeof(KDRV_ISE_IO_CFG));
		VOS_INIT_LIST_HEAD(&p_job_cfg->cfg_list);
	}

	return p_job_cfg;
}

static void kdrv_ise_int_job_cfg_free(KDRV_ISE_JOB_CFG *p_cfg)
{
	KDRV_ISE_MEM_POOL *p_mem_pool;
	unsigned long loc_flg;

	p_mem_pool = &g_kdrv_ise_ctl.job_cfg_pool;
	vk_spin_lock_irqsave(&p_mem_pool->lock, loc_flg);
	vos_list_del_init(&p_cfg->pool_list);
	vos_list_add_tail(&p_cfg->pool_list, &p_mem_pool->blk_free_list_root);
	p_mem_pool->cur_free_num += 1;
	vk_spin_unlock_irqrestore(&p_mem_pool->lock, loc_flg);
}

static KDRV_ISE_LL_BLK* kdrv_ise_int_ll_blk_alloc(void)
{
	KDRV_ISE_MEM_POOL *p_mem_pool;
	KDRV_ISE_LL_BLK *p_ll_blk;
	unsigned long loc_flg;
	FLGPTN wait_flg;

	p_mem_pool = &g_kdrv_ise_ctl.ll_blk_pool;
	p_ll_blk = NULL;

	while (1) {
		p_ll_blk = NULL;
		vk_spin_lock_irqsave(&p_mem_pool->lock, loc_flg);
		if (vos_list_empty(&p_mem_pool->blk_free_list_root) == FALSE) {
			p_ll_blk = vos_list_first_entry(&p_mem_pool->blk_free_list_root, KDRV_ISE_LL_BLK, pool_list);
			vos_list_del_init(&p_ll_blk->pool_list);
			vos_list_add_tail(&p_ll_blk->pool_list, &p_mem_pool->blk_used_list_root);
			p_mem_pool->cur_free_num -= 1;

			if ((p_mem_pool->blk_num - p_mem_pool->cur_free_num) > p_mem_pool->max_used_num) {
				p_mem_pool->max_used_num = p_mem_pool->blk_num - p_mem_pool->cur_free_num;
			}
		}
		vk_spin_unlock_irqrestore(&p_mem_pool->lock, loc_flg);

		if (p_ll_blk == NULL) {
			vos_flag_wait(&wait_flg, g_kdrv_ise_ctl.pool_flg_id, KDRV_ISE_POOL_LL_BLK_EXIST, (TWF_ORW | TWF_CLR));
		} else {
			break;
		}
	}

	if (p_ll_blk != NULL) {
		p_ll_blk->cur_cmd_idx = 0;
		p_ll_blk->out_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
		VOS_INIT_LIST_HEAD(&p_ll_blk->ll_wait_list);
		VOS_INIT_LIST_HEAD(&p_ll_blk->grp_job_list);
	}

	return p_ll_blk;
}

static void kdrv_ise_int_ll_blk_free(KDRV_ISE_LL_BLK *p_ll_blk)
{
	KDRV_ISE_MEM_POOL *p_mem_pool;
	unsigned long loc_flg;

	p_mem_pool = &g_kdrv_ise_ctl.ll_blk_pool;
	vk_spin_lock_irqsave(&p_mem_pool->lock, loc_flg);
	vos_list_del_init(&p_ll_blk->pool_list);
	vos_list_add_tail(&p_ll_blk->pool_list, &p_mem_pool->blk_free_list_root);
	p_mem_pool->cur_free_num += 1;
	vk_spin_unlock_irqrestore(&p_mem_pool->lock, loc_flg);

	vos_flag_set(g_kdrv_ise_ctl.pool_flg_id, KDRV_ISE_POOL_LL_BLK_EXIST);
}

static UINT32 kdrv_ise_int_ll_blk_get_max_num(void)
{
	return g_kdrv_ise_ctl.ll_blk_pool.blk_num;
}

static UINT32 kdrv_ise_int_get_job_id(KDRV_ISE_HANDLE *p_hdl)
{
	UINT32 rt;

	rt = p_hdl->cur_job_id;
	p_hdl->cur_job_id++;
	if (p_hdl->cur_job_id > KDRV_ISE_JOB_ID_MAX) {
		p_hdl->cur_job_id = 0;
	}

	return rt;
}

static void kdrv_ise_int_put_job(KDRV_ISE_HANDLE *p_hdl)
{
	unsigned long loc_flg;

	/* add to proc list */
	vk_spin_lock_irqsave(&p_hdl->job_list_lock, loc_flg);
	vos_list_add_tail(&p_hdl->p_cur_job->proc_list, &p_hdl->job_list_root);
	vk_spin_unlock_irqrestore(&p_hdl->job_list_lock, loc_flg);

	/* add to cb list */
	vk_spin_lock_irqsave(&p_hdl->cb_list_lock, loc_flg);
	vos_list_add_tail(&p_hdl->p_cur_job->cb_list, &p_hdl->cb_list_root);
	vk_spin_unlock_irqrestore(&p_hdl->cb_list_lock, loc_flg);

	p_hdl->p_cur_job->timestamp[KDRV_ISE_JOB_TS_PUTJOB] = hwclock_get_counter();

	/* update info in handle */
	p_hdl->p_cur_job = NULL;
	p_hdl->job_in_cnt += 1;
}

static KDRV_ISE_JOB_HEAD* kdrv_ise_int_get_proc_job(KDRV_ISE_HANDLE *p_hdl)
{
	unsigned long loc_flg;
	KDRV_ISE_JOB_HEAD *p_job;

	vk_spin_lock_irqsave(&p_hdl->job_list_lock, loc_flg);
	p_job = vos_list_first_entry_or_null(&p_hdl->job_list_root, KDRV_ISE_JOB_HEAD, proc_list);
	vk_spin_unlock_irqrestore(&p_hdl->job_list_lock, loc_flg);

	return p_job;
}

static void kdrv_ise_int_del_proc_job(KDRV_ISE_HANDLE *p_hdl, KDRV_ISE_JOB_HEAD *p_job)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_hdl->job_list_lock, loc_flg);
	vos_list_del_init(&p_job->proc_list);
	vk_spin_unlock_irqrestore(&p_hdl->job_list_lock, loc_flg);
}

static void kdrv_ise_int_flush_proc_job(KDRV_ISE_HANDLE *p_hdl)
{
	KDRV_ISE_JOB_HEAD *p_job;

	while ((p_job = kdrv_ise_int_get_proc_job(p_hdl)) != NULL) {
		DBG_DUMP("flush proc job 0x%.8x\r\n", (unsigned int)p_job);
		kdrv_ise_int_del_proc_job(p_hdl, p_job);
	}
}

static KDRV_ISE_JOB_HEAD* kdrv_ise_int_get_cb_job(KDRV_ISE_HANDLE *p_hdl)
{
	unsigned long loc_flg;
	KDRV_ISE_JOB_HEAD *p_job;

	vk_spin_lock_irqsave(&p_hdl->cb_list_lock, loc_flg);
	p_job = vos_list_first_entry_or_null(&p_hdl->cb_list_root, KDRV_ISE_JOB_HEAD, cb_list);
	vk_spin_unlock_irqrestore(&p_hdl->cb_list_lock, loc_flg);

	return p_job;
}

static void kdrv_ise_int_del_cb_job(KDRV_ISE_HANDLE *p_hdl, KDRV_ISE_JOB_HEAD *p_job)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_hdl->cb_list_lock, loc_flg);
	vos_list_del_init(&p_job->cb_list);
	vk_spin_unlock_irqrestore(&p_hdl->cb_list_lock, loc_flg);
}

static void kdrv_ise_int_put_ll_to_wait_list(KDRV_ISE_HANDLE *p_hdl, KDRV_ISE_LL_BLK *p_ll_blk)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_hdl->ll_wait_list_lock, loc_flg);
	vos_list_add_tail(&p_ll_blk->ll_wait_list, &p_hdl->ll_wait_list_root);
	vk_spin_unlock_irqrestore(&p_hdl->ll_wait_list_lock, loc_flg);
}

static KDRV_ISE_LL_BLK* kdrv_ise_int_get_wait_ll_blk(KDRV_ISE_HANDLE *p_hdl)
{
	unsigned long loc_flg;
	KDRV_ISE_LL_BLK *p_ll_blk;

	vk_spin_lock_irqsave(&p_hdl->ll_wait_list_lock, loc_flg);
	p_ll_blk = vos_list_first_entry_or_null(&p_hdl->ll_wait_list_root, KDRV_ISE_LL_BLK, ll_wait_list);
	vk_spin_unlock_irqrestore(&p_hdl->ll_wait_list_lock, loc_flg);

	return p_ll_blk;
}

static void kdrv_ise_int_del_wait_ll_blk(KDRV_ISE_HANDLE *p_hdl, KDRV_ISE_LL_BLK *p_ll_blk)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_hdl->ll_wait_list_lock, loc_flg);
	vos_list_del_init(&p_ll_blk->ll_wait_list);
	vk_spin_unlock_irqrestore(&p_hdl->ll_wait_list_lock, loc_flg);
}

static KDRV_ISE_JOB_HEAD* kdrv_ise_int_flush_cb_job(KDRV_ISE_HANDLE *p_hdl)
{
	KDRV_ISE_JOB_HEAD *p_job;
	KDRV_ISE_JOB_CFG *p_cfg;
	KDRV_ISE_LL_BLK *p_ll_blk;
	KDRV_ISE_CALLBACK_INFO cb_info;

	while ((p_job = kdrv_ise_int_get_cb_job(p_hdl)) != NULL) {
		DBG_DUMP("flush cb job 0x%.8x\r\n", (unsigned int)p_job);
		kdrv_ise_int_del_cb_job(p_hdl, p_job);
		p_hdl->job_out_cnt += 1;
		if (p_job->cb_fp.callback != NULL) {
			cb_info.job_id = p_job->job_id;
			cb_info.job_num = p_job->job_num;
			cb_info.err_num = p_job->err_num;
			cb_info.done_num = p_job->done_num;
			p_job->cb_fp.callback(&cb_info, NULL);
		}

		/* release job head */
		while (!vos_list_empty(&p_job->ll_blk_root)) {
			p_ll_blk = vos_list_first_entry(&p_job->ll_blk_root, KDRV_ISE_LL_BLK, grp_job_list);
			vos_list_del_init(&p_ll_blk->grp_job_list);
			kdrv_ise_int_ll_blk_free(p_ll_blk);
		}

		while (!vos_list_empty(&p_job->job_cfg_root)) {
			p_cfg = vos_list_first_entry(&p_job->job_cfg_root, KDRV_ISE_JOB_CFG, cfg_list);
			vos_list_del_init(&p_cfg->cfg_list);
			kdrv_ise_int_job_cfg_free(p_cfg);
		}
		kdrv_ise_int_job_head_free(p_job);
	}

	return p_job;
}

static void kdrv_ise_int_set_eng_status(KDRV_ISE_HANDLE *p_hdl, UINT32 sts)
{
	vos_flag_iset(p_hdl->eng_sts_flg_id, sts);
}

static INT32 kdrv_ise_int_wait_eng_status(KDRV_ISE_HANDLE *p_hdl, UINT32 sts, UINT32 timeout, UINT32 b_clr)
{
	FLGPTN wait_flg;
	UINT32 wait_mode;

	wait_mode = b_clr ? (TWF_ORW | TWF_CLR) : TWF_ORW;

	if (timeout) {
		return vos_flag_wait_timeout(&wait_flg, p_hdl->eng_sts_flg_id, sts, wait_mode, vos_util_msec_to_tick(timeout));
	}

	return vos_flag_wait(&wait_flg, p_hdl->eng_sts_flg_id, sts, wait_mode);
}

static void kdrv_ise_int_add_job_proc_cnt(KDRV_ISE_JOB_HEAD *p_job, UINT32 val)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_job->lock, loc_flg);
	p_job->proc_num += val;
	vk_spin_unlock_irqrestore(&p_job->lock, loc_flg);
}

static void kdrv_ise_int_add_job_err_cnt(KDRV_ISE_JOB_HEAD *p_job, UINT32 val)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_job->lock, loc_flg);
	p_job->err_num += val;
	vk_spin_unlock_irqrestore(&p_job->lock, loc_flg);
}

static void kdrv_ise_int_add_job_done_cnt(KDRV_ISE_JOB_HEAD *p_job, UINT32 val)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&p_job->lock, loc_flg);
	p_job->done_num += val;
	vk_spin_unlock_irqrestore(&p_job->lock, loc_flg);
}

#if 0
static UINT32 kdrv_ise_int_get_job_proc_cnt(KDRV_ISE_JOB_HEAD *p_job)
{
	unsigned long loc_flg;
	UINT32 rt;

	vk_spin_lock_irqsave(&p_job->lock, loc_flg);
	rt = p_job->proc_num;
	vk_spin_unlock_irqrestore(&p_job->lock, loc_flg);

	return rt;
}
#endif

static UINT32 kdrv_ise_int_get_job_err_cnt(KDRV_ISE_JOB_HEAD *p_job)
{
	unsigned long loc_flg;
	UINT32 rt;

	vk_spin_lock_irqsave(&p_job->lock, loc_flg);
	rt = p_job->err_num;
	vk_spin_unlock_irqrestore(&p_job->lock, loc_flg);

	return rt;
}

static UINT32 kdrv_ise_int_get_job_done_cnt(KDRV_ISE_JOB_HEAD *p_job)
{
	unsigned long loc_flg;
	UINT32 rt;

	vk_spin_lock_irqsave(&p_job->lock, loc_flg);
	rt = p_job->done_num;
	vk_spin_unlock_irqrestore(&p_job->lock, loc_flg);

	return rt;
}

#if 0
#endif

void kdrv_ise_int_trig_proc_tsk(KDRV_ISE_HANDLE *p_hdl)
{
	vos_flag_iset(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_TRIGGER);
}

INT32 kdrv_ise_int_pause_proc_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_PAUSE);
	if (wait_end != 0) {
		rt = vos_flag_wait_timeout(&wait_flg, p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(wait_end));
		if (rt < 0) {
			vos_flag_clr(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_PAUSE);
		}
	} else {
		rt = E_OK;
	}
	return rt;
}

INT32 kdrv_ise_int_resume_proc_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_RESUME);
	if (wait_end != 0) {
		rt = vos_flag_wait_timeout(&wait_flg, p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_RESUME_END, TWF_ORW, vos_util_msec_to_tick(wait_end));
		if (rt < 0) {
			vos_flag_clr(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_RESUME);
		}
	} else {
		rt = E_OK;
	}
	return rt;
}

INT32 kdrv_ise_int_close_proc_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_EXIT);
	if (wait_end != 0) {
		rt = vos_flag_wait_timeout(&wait_flg, p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_EXIT_END, TWF_ORW, vos_util_msec_to_tick(wait_end));
		if (rt < 0) {
			vos_flag_clr(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_EXIT);
		}
	} else {
		rt = E_OK;
	}

	return rt;
}

void kdrv_ise_int_trig_cb_tsk(KDRV_ISE_HANDLE *p_hdl)
{
	vos_flag_iset(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_TRIGGER);
}

INT32 kdrv_ise_int_pause_cb_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_PAUSE);
	if (wait_end != 0) {
		rt = vos_flag_wait_timeout(&wait_flg, p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(wait_end));
		if (rt < 0) {
			vos_flag_clr(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_PAUSE);
		}
	} else {
		rt = E_OK;
	}

	return rt;
}

INT32 kdrv_ise_int_resume_cb_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_RESUME);
	if (wait_end != 0) {
		rt = vos_flag_wait_timeout(&wait_flg, p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_RESUME_END, TWF_ORW, vos_util_msec_to_tick(wait_end));
		if (rt < 0) {
			vos_flag_clr(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_RESUME);
		}
	} else {
		rt = E_OK;
	}
	return rt;
}

INT32 kdrv_ise_int_close_cb_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end)
{
	FLGPTN wait_flg;
	INT32 rt;

	vos_flag_set(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_EXIT);
	if (wait_end != 0) {
		rt = vos_flag_wait_timeout(&wait_flg, p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_EXIT_END, TWF_ORW, vos_util_msec_to_tick(wait_end));
		if (rt < 0) {
			vos_flag_clr(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_EXIT);
		}
	} else {
		rt = E_OK;
	}

	return rt;
}

#if 0
#endif

void kdrv_ise_isr(void *eng, UINT32 status, void *reserve)
{
	unsigned long loc_flg;
	ISE_ENG_HANDLE *p_eng;
	KDRV_ISE_HANDLE *p_hdl;
	KDRV_ISE_JOB_HEAD *p_job;
	KDRV_ISE_LL_BLK *p_ll_blk;

	if (eng == NULL) {
		return ;
	}

	p_eng = (ISE_ENG_HANDLE *)eng;
	p_hdl = kdrv_ise_int_get_handle(p_eng->chip_id, p_eng->eng_id);
	p_job = NULL;
	if (p_hdl == NULL) {
		return ;
	}

	/*  cpu write mode use FMD
		ll write mode use llend(all subjob done)
	*/
	if (status & ISE_ENG_INTERRUPT_FMD) {
		vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
		if (p_hdl->p_fired_cfg != NULL) {
			p_job = p_hdl->p_fired_cfg->p_parent_job_head;
			p_hdl->p_fired_cfg = NULL;
		}
		vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

		if (p_job != NULL) {
			p_job->timestamp[KDRV_ISE_JOB_TS_ENG_END] = hwclock_get_counter();
			kdrv_ise_int_add_job_done_cnt(p_job, 1);
			kdrv_ise_int_trig_cb_tsk(p_hdl);
		} else {
			DBG_ERR("get null job at frame_end\r\n");
		}

		ise_eng_close(p_eng);
	}

	if (status & ISE_ENG_INTERRUPT_LLEND) {
		p_ll_blk = kdrv_ise_int_get_wait_ll_blk(p_hdl);
		if (p_ll_blk != NULL) {
			UINT32 addr = 0;
			vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
			if (p_hdl->p_fired_ll != NULL) {
				p_job = p_hdl->p_fired_ll->p_parent_job_head;
			}
			p_hdl->p_fired_ll = p_ll_blk;
			vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

			kdrv_ise_int_del_wait_ll_blk(p_hdl, p_ll_blk);

			addr = vos_cpu_get_phy_addr((VOS_ADDR)p_ll_blk->cmd_buf_addr);
			if((INT32) addr < 0 )
				DBG_ERR("wait va_to_pa fail:%x\n", p_ll_blk->cmd_buf_addr);
			else
				ise_eng_trig_ll(p_hdl->p_eng, addr);

			p_ll_blk->p_parent_job_head->timestamp[KDRV_ISE_JOB_TS_ENG_LOAD] = hwclock_get_counter();
		} else {
			vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
			if (p_hdl->p_fired_ll != NULL) {
				p_job = p_hdl->p_fired_ll->p_parent_job_head;
			}
			p_hdl->p_fired_ll = NULL;
			vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

			ise_eng_close(p_eng);
		}

		if (p_job != NULL) {
			p_job->timestamp[KDRV_ISE_JOB_TS_ENG_END] = hwclock_get_counter();
			kdrv_ise_int_add_job_done_cnt(p_job, p_job->job_num);
			kdrv_ise_int_trig_cb_tsk(p_hdl);
		} else {
			DBG_ERR("get null job at ll_end\r\n");
		}
	}

	kdrv_ise_int_set_eng_status(p_hdl, status);
}

INT32 kdrv_ise_sys_init(KDRV_ISE_CTL *p_ise_ctl)
{
	KDRV_ISE_HANDLE *p_hdl;
	UINT32 i;

	if (g_kdrv_ise_ctl.p_hdl != NULL) {
		DBG_ERR("reinit\r\n");
		return -1;
	}

	g_kdrv_ise_ctl.p_hdl = kdrv_ise_platform_malloc(sizeof(KDRV_ISE_HANDLE) * p_ise_ctl->total_ch);
	if (g_kdrv_ise_ctl.p_hdl == NULL) {
		DBG_ERR("alloc buf(%d) failed\r\n", sizeof(KDRV_ISE_HANDLE) * p_ise_ctl->total_ch);
		return -1;
	}

	/* init kdrv ctl */
	g_kdrv_ise_ctl.chip_num = p_ise_ctl->chip_num;
	g_kdrv_ise_ctl.eng_num = p_ise_ctl->eng_num;
	g_kdrv_ise_ctl.total_ch = p_ise_ctl->total_ch;

	/* job head pool init */
	snprintf(g_kdrv_ise_ctl.job_head_pool.name, 16, "job_head_pool");
	g_kdrv_ise_ctl.job_head_pool.blk_size = sizeof(KDRV_ISE_JOB_HEAD);
	g_kdrv_ise_ctl.job_head_pool.blk_num = KDRV_ISE_JOB_HEAD_MAX_NUM;
	g_kdrv_ise_ctl.job_head_pool.cur_free_num = KDRV_ISE_JOB_HEAD_MAX_NUM;
	g_kdrv_ise_ctl.job_head_pool.max_used_num = 0;
	g_kdrv_ise_ctl.job_head_pool.total_size = g_kdrv_ise_ctl.job_head_pool.blk_size * g_kdrv_ise_ctl.job_head_pool.blk_num;
	g_kdrv_ise_ctl.job_head_pool.start_addr = (UINT32)kdrv_ise_platform_malloc(g_kdrv_ise_ctl.job_head_pool.total_size);
	if ((void *)g_kdrv_ise_ctl.job_head_pool.start_addr == NULL) {
		DBG_ERR("alloc buf(%d) failed\r\n", g_kdrv_ise_ctl.job_head_pool.total_size);
		return -1;
	}
	vk_spin_lock_init(&g_kdrv_ise_ctl.job_head_pool.lock);
	VOS_INIT_LIST_HEAD(&g_kdrv_ise_ctl.job_head_pool.blk_free_list_root);
	VOS_INIT_LIST_HEAD(&g_kdrv_ise_ctl.job_head_pool.blk_used_list_root);
	for (i = 0; i < g_kdrv_ise_ctl.job_head_pool.blk_num; i++) {
		KDRV_ISE_JOB_HEAD *p_job_head;

		p_job_head = (KDRV_ISE_JOB_HEAD *)(g_kdrv_ise_ctl.job_head_pool.start_addr + (g_kdrv_ise_ctl.job_head_pool.blk_size * i));
		vk_spin_lock_init(&p_job_head->lock);
		vos_list_add_tail(&p_job_head->pool_list, &g_kdrv_ise_ctl.job_head_pool.blk_free_list_root);
	}

	/* job cfg pool init */
	snprintf(g_kdrv_ise_ctl.job_cfg_pool.name, 16, "job_cfg_pool");
	g_kdrv_ise_ctl.job_cfg_pool.blk_size = sizeof(KDRV_ISE_JOB_CFG);
	g_kdrv_ise_ctl.job_cfg_pool.blk_num = KDRV_ISE_JOB_CFG_MAX_NUM;
	g_kdrv_ise_ctl.job_cfg_pool.cur_free_num = KDRV_ISE_JOB_CFG_MAX_NUM;
	g_kdrv_ise_ctl.job_cfg_pool.max_used_num = 0;
	g_kdrv_ise_ctl.job_cfg_pool.total_size = g_kdrv_ise_ctl.job_cfg_pool.blk_size * g_kdrv_ise_ctl.job_cfg_pool.blk_num;
	g_kdrv_ise_ctl.job_cfg_pool.start_addr = (UINT32)kdrv_ise_platform_malloc(g_kdrv_ise_ctl.job_cfg_pool.total_size);
	if ((void *)g_kdrv_ise_ctl.job_cfg_pool.start_addr == NULL) {
		DBG_ERR("alloc buf(%d) failed\r\n", g_kdrv_ise_ctl.job_cfg_pool.total_size);
		return -1;
	}
	vk_spin_lock_init(&g_kdrv_ise_ctl.job_cfg_pool.lock);
	VOS_INIT_LIST_HEAD(&g_kdrv_ise_ctl.job_cfg_pool.blk_free_list_root);
	VOS_INIT_LIST_HEAD(&g_kdrv_ise_ctl.job_cfg_pool.blk_used_list_root);
	for (i = 0; i < g_kdrv_ise_ctl.job_cfg_pool.blk_num; i++) {
		KDRV_ISE_JOB_CFG *p_job;

		p_job = (KDRV_ISE_JOB_CFG *)(g_kdrv_ise_ctl.job_cfg_pool.start_addr + (g_kdrv_ise_ctl.job_cfg_pool.blk_size * i));
		vos_list_add_tail(&p_job->pool_list, &g_kdrv_ise_ctl.job_cfg_pool.blk_free_list_root);
	}

	/* ll blk pool init
		need cache align due to ll is the input of engine
		ll_blk struct + ll_buf + dummy
		dummy is for addr cache align
	*/
	snprintf(g_kdrv_ise_ctl.ll_blk_pool.name, 16, "ll_blk_pool");
	g_kdrv_ise_ctl.ll_blk_pool.blk_size = ALIGN_CEIL(sizeof(KDRV_ISE_LL_BLK), VOS_ALIGN_BYTES);
	g_kdrv_ise_ctl.ll_blk_pool.blk_size += ALIGN_CEIL((KDRV_ISE_LL_CMD_SIZE * (KDRV_ISE_LL_REG_NUMS + 1)), VOS_ALIGN_BYTES);
	g_kdrv_ise_ctl.ll_blk_pool.blk_size += VOS_ALIGN_BYTES;
	g_kdrv_ise_ctl.ll_blk_pool.blk_num = KDRV_ISE_LL_BLK_MAX_NUM;
	g_kdrv_ise_ctl.ll_blk_pool.cur_free_num = KDRV_ISE_LL_BLK_MAX_NUM;
	g_kdrv_ise_ctl.ll_blk_pool.max_used_num = 0;
	g_kdrv_ise_ctl.ll_blk_pool.total_size = g_kdrv_ise_ctl.ll_blk_pool.blk_size * g_kdrv_ise_ctl.ll_blk_pool.blk_num;
	g_kdrv_ise_ctl.ll_blk_pool.start_addr = (UINT32)kdrv_ise_platform_malloc(g_kdrv_ise_ctl.ll_blk_pool.total_size);
	if ((void *)g_kdrv_ise_ctl.ll_blk_pool.start_addr == NULL) {
		DBG_ERR("alloc buf(%d) failed\r\n", g_kdrv_ise_ctl.ll_blk_pool.total_size);
		return -1;
	}
	vk_spin_lock_init(&g_kdrv_ise_ctl.ll_blk_pool.lock);
	VOS_INIT_LIST_HEAD(&g_kdrv_ise_ctl.ll_blk_pool.blk_free_list_root);
	VOS_INIT_LIST_HEAD(&g_kdrv_ise_ctl.ll_blk_pool.blk_used_list_root);
	for (i = 0; i < g_kdrv_ise_ctl.ll_blk_pool.blk_num; i++) {
		KDRV_ISE_LL_BLK *p_ll_blk;

		p_ll_blk = (KDRV_ISE_LL_BLK *)(g_kdrv_ise_ctl.ll_blk_pool.start_addr + (g_kdrv_ise_ctl.ll_blk_pool.blk_size * i));
		p_ll_blk->cmd_buf_addr = ALIGN_CEIL((UINT32)p_ll_blk + sizeof(KDRV_ISE_LL_BLK), VOS_ALIGN_BYTES);
		p_ll_blk->max_cmd_num = KDRV_ISE_LL_REG_NUMS + 1;
		vos_list_add_tail(&p_ll_blk->pool_list, &g_kdrv_ise_ctl.ll_blk_pool.blk_free_list_root);
	}

	/* pool flag */
	vos_flag_create(&g_kdrv_ise_ctl.pool_flg_id, NULL, "ise_pool_flg");

	/* init kdrv handle */
	for (i = 0; i < g_kdrv_ise_ctl.total_ch; i++) {
		p_hdl = &g_kdrv_ise_ctl.p_hdl[i];
		memset((void *)p_hdl, 0, sizeof(KDRV_ISE_HANDLE));
		p_hdl->chip_id = p_ise_ctl->p_hdl[i].chip_id;
		p_hdl->eng_id = p_ise_ctl->p_hdl[i].eng_id;
	}

	return 0;
}

INT32 kdrv_ise_sys_uninit(KDRV_ISE_CTL *p_ise_ctl)
{
	vos_flag_destroy(g_kdrv_ise_ctl.pool_flg_id);
	kdrv_ise_platform_free(g_kdrv_ise_ctl.p_hdl);
	kdrv_ise_platform_free((void *)g_kdrv_ise_ctl.job_head_pool.start_addr);
	kdrv_ise_platform_free((void *)g_kdrv_ise_ctl.job_cfg_pool.start_addr);
	kdrv_ise_platform_free((void *)g_kdrv_ise_ctl.ll_blk_pool.start_addr);
	memset((void *)&g_kdrv_ise_ctl, 0, sizeof(KDRV_ISE_CTL));

	return 0;
}

#if 0
#endif

static INT32 kdrv_ise_job_done_cache_handle(KDRV_ISE_JOB_HEAD *p_job)
{
	KDRV_ISE_LIST_HEAD *p_list;
	KDRV_ISE_LIST_HEAD *n;
	KDRV_ISE_JOB_CFG *p_cfg;
	KDRV_ISE_LL_BLK *p_ll_blk;
	UINT32 buf_size;

	/* flush buffer after hw done */
	if (p_job->trig_param.mode == KDRV_ISE_PROC_MODE_LINKLIST) {
		vos_list_for_each_safe(p_list, n, &p_job->ll_blk_root) {
			p_ll_blk = vos_list_entry(p_list, KDRV_ISE_LL_BLK, grp_job_list);
			if (p_ll_blk->out_buf_flush == KDRV_ISE_DO_BUF_FLUSH) {
				vos_cpu_dcache_sync(p_ll_blk->out_buf_addr, p_ll_blk->out_buf_size, VOS_DMA_FROM_DEVICE);
			}
		}
	} else {
		vos_list_for_each_safe(p_list, n, &p_job->job_cfg_root) {
			p_cfg = vos_list_first_entry(p_list, KDRV_ISE_JOB_CFG, cfg_list);
			if (p_cfg->io_cfg.out_buf_flush == KDRV_ISE_DO_BUF_FLUSH) {
				buf_size = p_cfg->io_cfg.out_lofs * p_cfg->io_cfg.out_height;
				buf_size = ALIGN_CEIL(buf_size, VOS_ALIGN_BYTES);
				vos_cpu_dcache_sync(p_cfg->io_cfg.out_addr, buf_size, VOS_DMA_FROM_DEVICE);
			}
		}
	}

	return E_OK;
}

static INT32 kdrv_ise_job_process(KDRV_ISE_JOB_HEAD *p_job, KDRV_ISE_HANDLE *p_hdl)
{
	unsigned long loc_flg;
	KDRV_ISE_LIST_HEAD *p_list;
	KDRV_ISE_LIST_HEAD *n;
	KDRV_ISE_JOB_CFG *p_cfg;
	KDRV_ISE_LL_BLK *p_ll_blk;
	KDRV_ISE_LL_BLK *p_nxt_ll_blk;
	UINT32 ll_max_num;
	UINT32 ll_fire;
	UINT32 i;
	INT32 rt = E_OK;

	p_job->timestamp[KDRV_ISE_JOB_TS_START] = hwclock_get_counter();

	switch (p_job->trig_param.mode) {
	case KDRV_ISE_PROC_MODE_CPU:
		/* one config at a time */
		if (p_job->p_last_cfg == NULL) {
			p_cfg = vos_list_first_entry(&p_job->job_cfg_root, KDRV_ISE_JOB_CFG, cfg_list);
		} else {
			p_cfg = vos_list_next_entry(p_job->p_last_cfg, cfg_list);
		}
		p_cfg->p_parent_job_head = (void *)p_job;
		p_job->p_last_cfg = p_cfg;
		if (vos_list_is_last(&p_job->p_last_cfg->cfg_list, &p_job->job_cfg_root)) {
			kdrv_ise_int_del_proc_job(p_hdl, p_job);
		}

		rt = kdrv_ise_int_wait_eng_status(p_hdl, ISE_ENG_INTERRUPT_FMD, KDRV_ISE_TIMEOUT_MS, TRUE);
		if (rt == E_OK) {
			rt = ise_eng_open(p_hdl->p_eng);
			if (rt == E_OK) {
				vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
				p_hdl->p_fired_cfg = p_job->p_last_cfg;
				vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

				kdrv_ise_int_add_job_proc_cnt(p_job, 1);
				rt = kdrv_ise_job_process_cpu(p_job->p_last_cfg, p_hdl->p_eng);
				p_job->timestamp[KDRV_ISE_JOB_TS_ENG_LOAD] = hwclock_get_counter();
			}
		}

		if (rt != E_OK) {
			DBG_ERR(" wait ISE_ENG_INTERRUPT_FMD fail\r\n");
			kdrv_ise_int_add_job_err_cnt(p_job, 1);
		}
		break;

	case KDRV_ISE_PROC_MODE_LINKLIST:
		/* parse all cfg to ll blk */
		kdrv_ise_int_del_proc_job(p_hdl, p_job);
		ll_max_num = kdrv_ise_int_ll_blk_get_max_num();
		if (ll_max_num < p_job->job_num) {
			DBG_ERR("allock ll blk failed, job_num %d > alloc_blk_num %d\r\n", p_job->job_num, ll_max_num);
			kdrv_ise_int_add_job_err_cnt(p_job, p_job->job_num);
			break;
		}
		for (i = 0; i < p_job->job_num; i++) {
			p_ll_blk = kdrv_ise_int_ll_blk_alloc();
			if (p_ll_blk != NULL) {
				p_ll_blk->p_parent_job_head = p_job;
				p_ll_blk->blk_idx = ((p_job->job_id & 0xF) << 4) | i;
				vos_list_add_tail(&p_ll_blk->grp_job_list, &p_job->ll_blk_root);
			}
		}

		p_cfg = NULL;
		p_ll_blk = vos_list_first_entry(&p_job->ll_blk_root, KDRV_ISE_LL_BLK, grp_job_list);
		vos_list_for_each_safe(p_list, n, &p_job->job_cfg_root) {
			p_cfg = vos_list_entry(p_list, KDRV_ISE_JOB_CFG, cfg_list);

			if (p_ll_blk != NULL) {
				if (vos_list_is_last(&p_ll_blk->grp_job_list, &p_job->ll_blk_root)) {
					p_nxt_ll_blk = NULL;
				} else {
					p_nxt_ll_blk = vos_list_next_entry(p_ll_blk, grp_job_list);
				}

				kdrv_ise_int_add_job_proc_cnt(p_job, 1);
				rt = kdrv_ise_job_process_ll(p_cfg, p_hdl->p_eng, p_ll_blk, p_nxt_ll_blk);
				p_ll_blk = p_nxt_ll_blk;

				if (rt != E_OK) {
					DBG_ERR(" kdrv_ise_job_process_ll fail\r\n");
					kdrv_ise_int_add_job_err_cnt(p_job, 1);
				}
			} else {
				kdrv_ise_int_add_job_err_cnt(p_job, 1);
			}

			/* release job cfg */
			vos_list_del_init(&p_cfg->cfg_list);
			kdrv_ise_int_job_cfg_free(p_cfg);
		}

		/* check if trigger or add to wait_queue */
		if (kdrv_ise_int_get_job_err_cnt(p_job) == 0) {
			p_ll_blk = vos_list_first_entry(&p_job->ll_blk_root, KDRV_ISE_LL_BLK, grp_job_list);
			vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
			if (p_hdl->p_fired_ll == NULL) {
				p_hdl->p_fired_ll = p_ll_blk;
				ll_fire = TRUE;
			} else {
				ll_fire = FALSE;
			}
			vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

			if (ll_fire) {
				rt = ise_eng_open(p_hdl->p_eng);
				if (rt == E_OK && p_cfg != NULL) {
					UINT32 addr = 0;
					ise_eng_reg_isr_callback(p_hdl->p_eng, p_cfg->eng_isr_cb);

					addr = vos_cpu_get_phy_addr((VOS_ADDR)p_ll_blk->cmd_buf_addr);

					if( (INT32)addr < 0){
						DBG_ERR("trig va_to_pa fail:%x\n", p_ll_blk->cmd_buf_addr);
						rt = E_PAR;
					}
					else
						ise_eng_trig_ll(p_hdl->p_eng, addr);
					p_job->timestamp[KDRV_ISE_JOB_TS_ENG_LOAD] = hwclock_get_counter();
				} else {
					DBG_ERR("open engine timeout, add ll to wait list\r\n");
					kdrv_ise_int_put_ll_to_wait_list(p_hdl, p_ll_blk);
				}
			} else {
				kdrv_ise_int_put_ll_to_wait_list(p_hdl, p_ll_blk);
			}
		} else {
			rt = E_PAR;
		}

		break;

	default:
		rt = E_PAR;
		DBG_WRN("Unsupport trigger mode %d\r\n", p_job->trig_param.mode);
	}

	return rt;
}

THREAD_DECLARE(kdrv_ise_proc_task, p1)
{
	FLGPTN wait_flg = 0;
	KDRV_ISE_HANDLE *p_hdl;
	KDRV_ISE_JOB_HEAD *p_job;
	INT32 rt;

	p_hdl = p1;
	if (p_hdl == NULL) {
		goto ISE_TSK_DESTROY;
	}

	THREAD_ENTRY();
	goto ISE_TSK_PAUSE;

ISE_TSK_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_RESUME_END);
		PROFILE_TASK_IDLE();

		/* check pause flag */
		vos_flag_set(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_CHK);
		vos_flag_wait(&wait_flg, p_hdl->proc_tsk_flg_id, (KDRV_ISE_TSK_PAUSE | KDRV_ISE_TSK_CHK),(TWF_ORW | TWF_CLR));
		if (wait_flg & KDRV_ISE_TSK_PAUSE) {
			goto ISE_TSK_PAUSE;
		}

		/* receive job from list */
		while (1) {
			p_job = kdrv_ise_int_get_proc_job(p_hdl);

			if (p_job == NULL) {
				vos_flag_wait(&wait_flg, p_hdl->proc_tsk_flg_id, (KDRV_ISE_TSK_TRIGGER | KDRV_ISE_TSK_PAUSE),(TWF_ORW | TWF_CLR));
				if (wait_flg & KDRV_ISE_TSK_PAUSE) {
					goto ISE_TSK_PAUSE;
				}
			} else {
				break;
			}
		}

		/* process job */
		PROFILE_TASK_BUSY();
		rt = kdrv_ise_job_process(p_job, p_hdl);
		if (rt != E_OK) {
			/* err handle, prevent cb task not handle event due to err(no frame end) */
			kdrv_ise_int_trig_cb_tsk(p_hdl);
		}
	}

ISE_TSK_PAUSE:
	vos_flag_clr(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_RESUME_END);
	vos_flag_set(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_PAUSE_END);
	vos_flag_wait(&wait_flg, p_hdl->proc_tsk_flg_id, (KDRV_ISE_TSK_EXIT | KDRV_ISE_TSK_RESUME), (TWF_ORW | TWF_CLR));

	if (wait_flg & KDRV_ISE_TSK_EXIT) {
		goto ISE_TSK_DESTROY;
	}

	if (wait_flg & KDRV_ISE_TSK_RESUME) {
		vos_flag_clr(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_PAUSE_END);
		goto ISE_TSK_PROC;
	}

ISE_TSK_DESTROY:
	if (p_hdl) {
		vos_flag_set(p_hdl->proc_tsk_flg_id, KDRV_ISE_TSK_EXIT_END);
	}
	THREAD_RETURN(0);
}

THREAD_DECLARE(kdrv_ise_cb_task, p1)
{
	FLGPTN wait_flg = 0;
	KDRV_ISE_HANDLE *p_hdl;
	KDRV_ISE_JOB_HEAD *p_job;
	KDRV_ISE_JOB_CFG *p_cfg;
	KDRV_ISE_LL_BLK *p_ll_blk;
	KDRV_ISE_CALLBACK_INFO cb_info;
	UINT32 err_num, done_num;

	p_hdl = p1;
	if (p_hdl == NULL) {
		goto ISE_TSK_DESTROY;
	}

	THREAD_ENTRY();
	goto ISE_TSK_PAUSE;

ISE_TSK_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_RESUME_END);
		PROFILE_TASK_IDLE();

		/* wait pause/trigger flag */
		vos_flag_wait(&wait_flg, p_hdl->cb_tsk_flg_id, (KDRV_ISE_TSK_TRIGGER | KDRV_ISE_TSK_PAUSE),(TWF_ORW | TWF_CLR));

		/* receive job from list */
		if (wait_flg & KDRV_ISE_TSK_TRIGGER) {
			while ((p_job = kdrv_ise_int_get_cb_job(p_hdl)) != NULL) {
				err_num = kdrv_ise_int_get_job_err_cnt(p_job);
				done_num = kdrv_ise_int_get_job_done_cnt(p_job);

				if ((err_num + done_num) == p_job->job_num) {
					kdrv_ise_dump_ts(p_job);
					kdrv_ise_int_del_cb_job(p_hdl, p_job);
					p_hdl->job_out_cnt += 1;

					/* flush output buffer */
					if (err_num == 0) {
						kdrv_ise_job_done_cache_handle(p_job);
					}

					/* job done callback */
					if (p_job->cb_fp.callback != NULL) {
						cb_info.job_id = p_job->job_id;
						cb_info.job_num = p_job->job_num;
						cb_info.err_num = err_num;
						cb_info.done_num = done_num;
						p_job->cb_fp.callback(&cb_info, NULL);
					}

					/* release job head */
					while (!vos_list_empty(&p_job->ll_blk_root)) {
						p_ll_blk = vos_list_first_entry(&p_job->ll_blk_root, KDRV_ISE_LL_BLK, grp_job_list);
						vos_list_del_init(&p_ll_blk->grp_job_list);
						kdrv_ise_int_ll_blk_free(p_ll_blk);
					}

					while (!vos_list_empty(&p_job->job_cfg_root)) {
						p_cfg = vos_list_first_entry(&p_job->job_cfg_root, KDRV_ISE_JOB_CFG, cfg_list);
						vos_list_del_init(&p_cfg->cfg_list);
						kdrv_ise_int_job_cfg_free(p_cfg);
					}
					kdrv_ise_int_job_head_free(p_job);
				} else {
					break;
				}
			}
		}

		if (wait_flg & KDRV_ISE_TSK_PAUSE) {
			goto ISE_TSK_PAUSE;
		}

	}

ISE_TSK_PAUSE:
	vos_flag_clr(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_RESUME_END);
	vos_flag_set(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_PAUSE_END);
	vos_flag_wait(&wait_flg, p_hdl->cb_tsk_flg_id, (KDRV_ISE_TSK_EXIT | KDRV_ISE_TSK_RESUME), (TWF_ORW | TWF_CLR));

	if (wait_flg & KDRV_ISE_TSK_EXIT) {
		goto ISE_TSK_DESTROY;
	}

	if (wait_flg & KDRV_ISE_TSK_RESUME) {
		vos_flag_clr(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_PAUSE_END);
		goto ISE_TSK_PROC;
	}

ISE_TSK_DESTROY:
	if (p_hdl) {
		vos_flag_set(p_hdl->cb_tsk_flg_id, KDRV_ISE_TSK_EXIT_END);
	}
	THREAD_RETURN(0);
}

INT32 kdrv_ise_init(KDRV_ISE_CTX_BUF_CFG ctx_buf_cfg, UINT32 buf_addr, UINT32 buf_size)
{
	CHAR name[32];
	KDRV_ISE_HANDLE *p_hdl;
	UINT32 i;

	for (i = 0; i < g_kdrv_ise_ctl.total_ch; i++) {
		p_hdl = &g_kdrv_ise_ctl.p_hdl[i];
		p_hdl->cur_job_id = 0;
		p_hdl->is_opened= 0;

		snprintf(name, 32, "kdrv_ise_int_sem_%d", (int)i);
		vos_sem_create(&p_hdl->internal_sem, 1, name);

		snprintf(name, 32, "kdrv_ise_sem_%d", (int)i);
		vos_sem_create(&p_hdl->sem, 1, name);

		vk_spin_lock_init(&p_hdl->lock);
		vk_spin_lock_init(&p_hdl->job_list_lock);
		vk_spin_lock_init(&p_hdl->cb_list_lock);
		vk_spin_lock_init(&p_hdl->ll_wait_list_lock);

		VOS_INIT_LIST_HEAD(&p_hdl->job_list_root);
		VOS_INIT_LIST_HEAD(&p_hdl->cb_list_root);
		VOS_INIT_LIST_HEAD(&p_hdl->ll_wait_list_root);

		snprintf(name, 32, "kdrv_ise_eng_sts_flg_%d", (int)i);
		vos_flag_create(&p_hdl->eng_sts_flg_id, NULL, name);

		snprintf(name, 32, "kdrv_ise_proc_tsk_flg_%d", (int)i);
		vos_flag_create(&p_hdl->proc_tsk_flg_id, NULL, name);

		snprintf(name, 32, "kdrv_ise_cb_tsk_flg_%d", (int)i);
		vos_flag_create(&p_hdl->cb_tsk_flg_id, NULL, name);

		snprintf(name, 32, "kdrv_ise_proc_tsk_%d", (int)i);
		THREAD_CREATE(p_hdl->proc_tsk_id, kdrv_ise_proc_task, p_hdl, name);
		if (p_hdl->proc_tsk_id == 0) {
			DBG_ERR("Open %s fail\r\n", name);
			return E_SYS;
		}
		THREAD_SET_PRIORITY(p_hdl->proc_tsk_id, KDRV_ISE_TSK_PRI);
		THREAD_RESUME(p_hdl->proc_tsk_id);

		snprintf(name, 32, "kdrv_ise_cb_tsk_%d", (int)i);
		THREAD_CREATE(p_hdl->cb_tsk_id, kdrv_ise_cb_task, p_hdl, name);
		if (p_hdl->cb_tsk_id == 0) {
			DBG_ERR("Open %s fail\r\n", name);
			return E_SYS;
		}
		THREAD_SET_PRIORITY(p_hdl->cb_tsk_id, KDRV_ISE_TSK_PRI);
		THREAD_RESUME(p_hdl->cb_tsk_id);
	}

	kdrv_ise_dbg_init(&g_kdrv_ise_ctl);

	return E_OK;
}

INT32 kdrv_ise_uninit(void)
{
	KDRV_ISE_HANDLE *p_hdl;
	UINT32 i;

	kdrv_ise_dbg_uninit();

	for (i = 0; i < g_kdrv_ise_ctl.total_ch; i++) {
		p_hdl = &g_kdrv_ise_ctl.p_hdl[i];

		/* destroy task */
		if (kdrv_ise_int_close_proc_tsk(p_hdl, -1) < 0) {
			THREAD_DESTROY(p_hdl->proc_tsk_id);
		}

		if (kdrv_ise_int_close_cb_tsk(p_hdl, -1) < 0) {
			THREAD_DESTROY(p_hdl->cb_tsk_id);
		}

		vos_sem_destroy(p_hdl->sem);
		vos_sem_destroy(p_hdl->internal_sem);

		vos_flag_destroy(p_hdl->proc_tsk_flg_id);
		vos_flag_destroy(p_hdl->cb_tsk_flg_id);
		vos_flag_destroy(p_hdl->eng_sts_flg_id);
	}

	return E_OK;
}

INT32 kdrv_ise_open(UINT32 chip, UINT32 engine)
{
	KDRV_ISE_HANDLE *p_hdl;

	p_hdl = kdrv_ise_int_get_handle(chip, engine);
	if (p_hdl == NULL) {
		DBG_ERR("open failed, null handle of chip 0x%.8x, engine 0x%.8x\r\n", (unsigned int)chip, (unsigned int)engine);
		return E_PAR;
	}

	/* wait sem for user, release when close */
	vos_sem_wait(p_hdl->sem);

	/* wait sem for internal api protection */
	vos_sem_wait(p_hdl->internal_sem);
	p_hdl->p_eng = ise_eng_get_handle(chip, engine);
	if (p_hdl->p_eng == NULL) {
		DBG_ERR("get ise_eng handle failed, chip 0x%.8x, engine 0x%.8x\r\n", (unsigned int)chip, (unsigned int)engine);
		vos_sem_sig(p_hdl->internal_sem);
		vos_sem_sig(p_hdl->sem);
		return E_SYS;
	} else {
		p_hdl->p_cur_job = NULL;
		p_hdl->p_fired_cfg = NULL;
		p_hdl->p_fired_ll = NULL;
		kdrv_ise_int_set_eng_status(p_hdl, ISE_ENG_INTERRUPT_FMD);

		/* resume thread, err should never happend */
		if (kdrv_ise_int_resume_proc_tsk(p_hdl, -1) < 0) {
			DBG_ERR("resume proc task err\r\n");
			vos_sem_sig(p_hdl->internal_sem);
			vos_sem_sig(p_hdl->sem);
			return E_SYS;
		}

		if (kdrv_ise_int_resume_cb_tsk(p_hdl, -1) < 0) {
			DBG_ERR("resume cb task err\r\n");
			vos_sem_sig(p_hdl->internal_sem);
			vos_sem_sig(p_hdl->sem);
			return E_SYS;
		}

		p_hdl->is_opened = TRUE;
	}
	vos_sem_sig(p_hdl->internal_sem);

	return E_OK;
}

INT32 kdrv_ise_close(UINT32 chip, UINT32 engine)
{
	unsigned long loc_flg;
	KDRV_ISE_HANDLE *p_hdl;

	p_hdl = kdrv_ise_int_get_handle(chip, engine);
	if (p_hdl == NULL) {
		DBG_ERR("close failed, null handle of chip 0x%.8x, engine 0x%.8x\r\n", (unsigned int)chip, (unsigned int)engine);
		return E_PAR;
	}

	/* wait sem for internal api protection */
	vos_sem_wait(p_hdl->internal_sem);

	if (p_hdl->is_opened == FALSE) {
		DBG_ERR("already closed\r\n");
		vos_sem_sig(p_hdl->internal_sem);
		return E_SYS;
	}

	if (p_hdl->p_eng == NULL) {
		DBG_ERR("must open before close\r\n");
		vos_sem_sig(p_hdl->internal_sem);
		return E_SYS;
	}

	/* pause thread */
	if (kdrv_ise_int_pause_proc_tsk(p_hdl, -1) < 0) {
		DBG_ERR("pause proc task err\r\n");
		vos_sem_sig(p_hdl->internal_sem);
		return E_SYS;
	}

	if (kdrv_ise_int_pause_cb_tsk(p_hdl, -1) < 0) {
		DBG_ERR("pause cb task err\r\n");
		vos_sem_sig(p_hdl->internal_sem);
		return E_SYS;
	}

	/* lock handle to prevent other api get in when flush */
	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	kdrv_ise_int_flush_proc_job(p_hdl);
	kdrv_ise_int_flush_cb_job(p_hdl);
	if (p_hdl->p_cur_job != NULL) {
		/* release job head */
		KDRV_ISE_JOB_HEAD *p_job;
		KDRV_ISE_JOB_CFG *p_cfg;

		p_job = p_hdl->p_cur_job;
		while (!vos_list_empty(&p_job->job_cfg_root)) {
			p_cfg = vos_list_first_entry(&p_job->job_cfg_root, KDRV_ISE_JOB_CFG, cfg_list);
			vos_list_del_init(&p_cfg->cfg_list);
			kdrv_ise_int_job_cfg_free(p_cfg);
		}
		kdrv_ise_int_job_head_free(p_job);
	}
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

	p_hdl->p_eng = NULL;
	p_hdl->is_opened = FALSE;
	vos_sem_sig(p_hdl->internal_sem);

	/* release kdrv_ise sem for next user */
	vos_sem_sig(p_hdl->sem);

	return 0;
}

#if 0
#endif

static INT32 kdrv_ise_set_gen_node(KDRV_ISE_HANDLE *p_hdl, void *p_data)
{
	KDRV_ISE_JOB_CFG *p_cfg;

	if (p_hdl->p_cur_job) {
		p_cfg = kdrv_ise_int_job_cfg_alloc();
		if (p_cfg) {
			p_hdl->p_cur_job->job_num += 1;
			p_cfg->eng_isr_cb = kdrv_ise_isr;
			vos_list_add_tail(&p_cfg->cfg_list, &p_hdl->p_cur_job->job_cfg_root);
		} else {
			DBG_ERR("allocate job_cfg failed\r\n");
			return E_NOMEM;
		}
	} else {
		p_hdl->p_cur_job = kdrv_ise_int_job_head_alloc();
		p_cfg = kdrv_ise_int_job_cfg_alloc();

		if (p_hdl->p_cur_job && p_cfg) {
			p_hdl->p_cur_job->chip_id = p_hdl->chip_id;
			p_hdl->p_cur_job->eng_id = p_hdl->eng_id;
			p_hdl->p_cur_job->job_id = kdrv_ise_int_get_job_id(p_hdl);
			p_hdl->p_cur_job->job_num = 1;
			p_cfg->eng_isr_cb = kdrv_ise_isr;
			vos_list_add_tail(&p_cfg->cfg_list, &p_hdl->p_cur_job->job_cfg_root);
		} else {
			if (p_hdl->p_cur_job == NULL) {
				DBG_ERR("allocate job_head failed\r\n");
			} else {
				kdrv_ise_int_job_head_free(p_hdl->p_cur_job);
				p_hdl->p_cur_job = NULL;
			}

			if (p_cfg == NULL) {
				DBG_ERR("allocate job_cfg failed\r\n");
			} else {
				kdrv_ise_int_job_cfg_free(p_cfg);
			}

			return E_NOMEM;
		}
	}

	return E_OK;
}

static INT32 kdrv_ise_set_iocfg(KDRV_ISE_HANDLE *p_hdl, void *p_data)
{
	KDRV_ISE_JOB_CFG *p_cfg;

	p_cfg = vos_list_last_entry(&p_hdl->p_cur_job->job_cfg_root, KDRV_ISE_JOB_CFG, cfg_list);
	if (p_cfg != NULL) {
		memcpy(&p_cfg->io_cfg, p_data, sizeof(KDRV_ISE_IO_CFG));
	}

	return E_OK;
}

static INT32 kdrv_ise_set_flush_job(KDRV_ISE_HANDLE *p_hdl, void *p_data)
{
	KDRV_ISE_JOB_HEAD *p_job;
	KDRV_ISE_JOB_CFG *p_cfg;

	if (p_hdl->p_cur_job != NULL) {
		p_job = p_hdl->p_cur_job;
		while (!vos_list_empty(&p_job->job_cfg_root)) {
			p_cfg = vos_list_first_entry(&p_job->job_cfg_root, KDRV_ISE_JOB_CFG, cfg_list);
			vos_list_del_init(&p_cfg->cfg_list);
			kdrv_ise_int_job_cfg_free(p_cfg);
		}
		kdrv_ise_int_job_head_free(p_job);

		p_hdl->p_cur_job = NULL;
	}

	return E_OK;
}

static INT32 kdrv_ise_set_flush_cfg(KDRV_ISE_HANDLE *p_hdl, void *p_data)
{
	KDRV_ISE_JOB_HEAD *p_job;
	KDRV_ISE_JOB_CFG *p_cfg;

	if (p_hdl->p_cur_job != NULL) {
		p_job = p_hdl->p_cur_job;

		p_cfg = vos_list_first_entry(&p_job->job_cfg_root, KDRV_ISE_JOB_CFG, cfg_list);
		vos_list_del_init(&p_cfg->cfg_list);
		kdrv_ise_int_job_cfg_free(p_cfg);
		p_job->job_num -= 1;

		if (p_job->job_num == 0) {
			kdrv_ise_int_job_head_free(p_job);
			p_hdl->p_cur_job = NULL;
		}
	}

	return E_OK;
}

static KDRV_ISE_FUNC_ITEM g_kdrv_ise_func_tab[KDRV_ISE_PARAM_ID_MAX] = {
	{kdrv_ise_set_gen_node,	0, 0, NULL, 0, 0, "gen node"},
	{kdrv_ise_set_iocfg,	1, 1, NULL, 0, 0, "io config"},
	{kdrv_ise_set_flush_job,1, 1, NULL, 0, 0, "flush job"},
	{kdrv_ise_set_flush_cfg,1, 1, NULL, 0, 0, "flush config"},
};

/* backward compatible, support old ise param id ctrl flow */
INT32 kdrv_ise_set_obsolete(UINT32 id, UINT32 param_id, VOID *p_data)
{
	unsigned long loc_flg;
	KDRV_ISE_HANDLE *p_hdl;
	KDRV_ISE_IO_CFG iocfg;
	KDRV_ISE_MODE *p_mode_cfg;
	KDRV_ISE_MODE *p_cfg_arr;
	UINT32 i;
	INT32 rt = E_OK;

	if (param_id == KDRV_ISE_PARAM_OPENCFG) {
		/* do nothing */
		return E_OK;
	}

	p_hdl = kdrv_ise_int_get_handle(KDRV_DEV_ID_CHIP(id), KDRV_DEV_ID_ENGINE(id));
	if (p_hdl == NULL) {
		DBG_ERR("null handle of id 0x%.8x\r\n", (unsigned int)id);
		return E_PAR;
	}

	/* wait sem for internal api protection */
	vos_sem_wait(p_hdl->internal_sem);
	if (p_hdl->is_opened == FALSE) {
		DBG_ERR("already closed\r\n");
		vos_sem_sig(p_hdl->internal_sem);
		return E_SYS;
	}

	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);

	if (p_hdl->p_cur_job != NULL) {
		DBG_ERR("obselete mode not support group job\r\n");
		rt = E_SYS;
		goto err;
	}
	p_hdl->is_obsolete_mode = TRUE;

	switch (param_id) {
	case KDRV_ISE_PARAM_MODE:
		p_mode_cfg = (KDRV_ISE_MODE *)p_data;

		iocfg.io_pack_fmt = p_mode_cfg->io_pack_fmt;
		iocfg.argb_out_mode = p_mode_cfg->argb_out_mode;
		iocfg.scl_method = p_mode_cfg->scl_method;
		iocfg.in_width = p_mode_cfg->in_width;
		iocfg.in_height = p_mode_cfg->in_height;
		iocfg.in_lofs = p_mode_cfg->in_lofs;
		iocfg.in_addr = p_mode_cfg->in_addr;
		iocfg.in_buf_flush = p_mode_cfg->in_buf_flush;
		iocfg.out_width = p_mode_cfg->out_width;
		iocfg.out_height = p_mode_cfg->out_height;
		iocfg.out_lofs = p_mode_cfg->out_lofs;
		iocfg.out_addr = p_mode_cfg->out_addr;
		iocfg.out_buf_flush = p_mode_cfg->out_buf_flush;
		iocfg.phy_addr_en = p_mode_cfg->phy_addr_en;

		/* gen node + set cfg */
		rt = g_kdrv_ise_func_tab[KDRV_ISE_PARAM_GEN_NODE].set_fp(p_hdl, NULL);
		if (rt == E_OK) {
			rt = g_kdrv_ise_func_tab[KDRV_ISE_PARAM_IO_CFG].set_fp(p_hdl, (void *)&iocfg);
		}

		break;

	case KDRV_ISE_LL_PARAM_MODE:
		p_cfg_arr = (KDRV_ISE_MODE *)p_data;

		for (i = 0; i < p_cfg_arr[0].ll_job_nums; i++) {
			p_mode_cfg = &p_cfg_arr[i];

			iocfg.io_pack_fmt = p_mode_cfg->io_pack_fmt;
			iocfg.argb_out_mode = p_mode_cfg->argb_out_mode;
			iocfg.scl_method = p_mode_cfg->scl_method;
			iocfg.in_width = p_mode_cfg->in_width;
			iocfg.in_height = p_mode_cfg->in_height;
			iocfg.in_lofs = p_mode_cfg->in_lofs;
			iocfg.in_addr = p_mode_cfg->in_addr;
			iocfg.in_buf_flush = p_mode_cfg->in_buf_flush;
			iocfg.out_width = p_mode_cfg->out_width;
			iocfg.out_height = p_mode_cfg->out_height;
			iocfg.out_lofs = p_mode_cfg->out_lofs;
			iocfg.out_addr = p_mode_cfg->out_addr;
			iocfg.out_buf_flush = p_mode_cfg->out_buf_flush;

			rt = g_kdrv_ise_func_tab[KDRV_ISE_PARAM_GEN_NODE].set_fp(p_hdl, NULL);
			if (rt == E_OK) {
				rt = g_kdrv_ise_func_tab[KDRV_ISE_PARAM_IO_CFG].set_fp(p_hdl, (void *)&iocfg);
			} else {
				break;
			}
		}

		break;

	default:
		break;
	}

err:
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
	vos_sem_sig(p_hdl->internal_sem);
	return rt;
}

INT32 kdrv_ise_set(UINT32 id, UINT32 param_id, VOID *p_data)
{
	KDRV_ISE_HANDLE *p_hdl;
	unsigned long loc_flg;
	INT32 rt;

	if (KDRV_ISE_IS_OLD_PARAM_ID(param_id)) {
		return kdrv_ise_set_obsolete(id, param_id, p_data);
	}

	if (param_id >= KDRV_ISE_PARAM_ID_MAX) {
		DBG_ERR("param_id %d overflow\r\n", (int)param_id);
		return E_PAR;
	}

	p_hdl = kdrv_ise_int_get_handle(KDRV_DEV_ID_CHIP(id), KDRV_DEV_ID_ENGINE(id));
	if (p_hdl == NULL) {
		DBG_ERR("null handle of id 0x%.8x\r\n", (unsigned int)id);
		return E_PAR;
	}

	/* wait sem for internal api protection */
	vos_sem_wait(p_hdl->internal_sem);
	if (p_hdl->is_opened == FALSE) {
		DBG_ERR("already closed\r\n");
		vos_sem_sig(p_hdl->internal_sem);
		return E_SYS;
	}

	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	rt = E_SYS;
	if (g_kdrv_ise_func_tab[param_id].set_chk_data) {
		if (p_data == NULL) {
			DBG_ERR("p_data is null pointer\r\n");
			rt = E_PAR;
			goto err;
		}
	}

	if (g_kdrv_ise_func_tab[param_id].set_chk_cur_job) {
		if (p_hdl->p_cur_job == NULL) {
			DBG_ERR("must gen_node first\r\n");
			rt = E_PAR;
			goto err;
		}
	}

	if (g_kdrv_ise_func_tab[param_id].set_fp) {
		rt = g_kdrv_ise_func_tab[param_id].set_fp(p_hdl, p_data);
		if (rt < 0) {
			DBG_ERR("%s err, rt %d\r\n", g_kdrv_ise_func_tab[param_id].msg, rt);
		}
	}

err:
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
	vos_sem_sig(p_hdl->internal_sem);
	return rt;
}

INT32 kdrv_ise_get(UINT32 id, UINT32 param_id, VOID *p_data)
{
	KDRV_ISE_HANDLE *p_hdl;
	unsigned long loc_flg;
	INT32 rt;

	if (param_id >= KDRV_ISE_PARAM_ID_MAX) {
		DBG_ERR("param_id %d overflow\r\n", (int)param_id);
		return E_PAR;
	}

	p_hdl = kdrv_ise_int_get_handle(KDRV_DEV_ID_CHIP(id), KDRV_DEV_ID_ENGINE(id));
	if (p_hdl == NULL) {
		DBG_ERR("null handle of id 0x%.8x\r\n", (unsigned int)id);
		return E_PAR;
	}

	/* wait sem for internal api protection */
	vos_sem_wait(p_hdl->internal_sem);
	if (p_hdl->is_opened == FALSE) {
		DBG_ERR("already closed\r\n");
		vos_sem_sig(p_hdl->internal_sem);
		return E_SYS;
	}

	vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
	rt = E_SYS;
	if (g_kdrv_ise_func_tab[param_id].get_chk_data) {
		if (p_data == NULL) {
			DBG_ERR("p_data is null pointer\r\n");
			rt = E_PAR;
			goto err;
		}
	}

	if (g_kdrv_ise_func_tab[param_id].get_chk_cur_job) {
		if (p_hdl->p_cur_job == NULL) {
			DBG_ERR("must gen_node first\r\n");
			rt = E_PAR;
			goto err;
		}
	}

	if (g_kdrv_ise_func_tab[param_id].get_fp) {
		rt = g_kdrv_ise_func_tab[param_id].get_fp(p_hdl, p_data);
		if (rt < 0) {
			DBG_ERR("%s err, rt %d\r\n", g_kdrv_ise_func_tab[param_id].msg, rt);
		}
	}

err:
	vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
	vos_sem_sig(p_hdl->internal_sem);
	return rt;
}

INT32 kdrv_ise_obsolete_cb(VOID *callback_info, VOID *user_data)
{
	KDRV_ISE_HANDLE *p_hdl;
	KDRV_ISE_CALLBACK_INFO *p_cb_info;
	FLGPTN flgptn;

	p_hdl = kdrv_ise_int_get_handle(KDRV_CHIP0, KDRV_GFX2D_ISE0);
	if (p_hdl) {
		flgptn = KDRV_ISE_TSK_JOB_FAIL;
		if (callback_info) {
			p_cb_info = (KDRV_ISE_CALLBACK_INFO *)callback_info;
			if (p_cb_info->done_num == p_cb_info->job_num) {
				flgptn = KDRV_ISE_TSK_JOB_DONE;
			}
		}
		vos_flag_iset(p_hdl->cb_tsk_flg_id, flgptn);
	} else {
		DBG_ERR("null handle\r\n");
	}

	return 0;
}

INT32 kdrv_ise_trigger(UINT32 id,  void *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	KDRV_ISE_HANDLE *p_hdl;
	unsigned long loc_flg;
	INT32 rt;

	p_hdl = kdrv_ise_int_get_handle(KDRV_DEV_ID_CHIP(id), KDRV_DEV_ID_ENGINE(id));
	if (p_hdl == NULL) {
		DBG_ERR("null handle of id 0x%.8x\r\n", (unsigned int)id);
		return E_PAR;
	}

	if (p_param == NULL) {
		DBG_ERR("null param\r\n");
		return E_PAR;
	}

	/* wait sem for internal api protection */
	vos_sem_wait(p_hdl->internal_sem);
	if (p_hdl->is_opened == FALSE) {
		DBG_ERR("already closed\r\n");
		vos_sem_sig(p_hdl->internal_sem);
		return E_SYS;
	}

	/* check if obselete flow or new flow for backward compatible */
	if (p_hdl->is_obsolete_mode) {
		vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
		if (!vos_list_empty(&p_hdl->job_list_root)) {
			DBG_ERR("proc queue no empty, obsolete mode not support queue mode\r\n");
			rt = E_SYS;
			vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
		} else {
			if (p_hdl->p_cur_job) {
				KDRV_ISE_TRIGGER_PARAM *p_trig_param;

				/* send job and clear obselete flag */
				p_hdl->p_cur_job->trig_param.mode = KDRV_ISE_PROC_MODE_LINKLIST;
				p_hdl->p_cur_job->cb_fp.callback = kdrv_ise_obsolete_cb;
				kdrv_ise_int_put_job(p_hdl);
				p_hdl->is_obsolete_mode = FALSE;
				vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

				/* set flag trigger proc_task */
				kdrv_ise_int_trig_proc_tsk(p_hdl);

				/* wait cb jobdone */
				p_trig_param = (KDRV_ISE_TRIGGER_PARAM *)p_param;
				if (p_trig_param->wait_end) {
					FLGPTN wait_flg;

					if (p_trig_param->time_out_ms != 0) {
						rt = vos_flag_wait_timeout(&wait_flg, p_hdl->cb_tsk_flg_id, (KDRV_ISE_TSK_JOB_DONE | KDRV_ISE_TSK_JOB_FAIL), TWF_ORW, vos_util_msec_to_tick(p_trig_param->time_out_ms));
					} else {
						rt = vos_flag_wait(&wait_flg, p_hdl->cb_tsk_flg_id, (KDRV_ISE_TSK_JOB_DONE | KDRV_ISE_TSK_JOB_FAIL), TWF_ORW);
					}

					if (rt == E_OK) {
						if (wait_flg == KDRV_ISE_TSK_JOB_FAIL) {
							DBG_ERR("flag = KDR_ISE_TSK_JOB_FAIL \r\n");
							rt = E_SYS;
						}
					}
				} else {
					rt = E_OK;
				}

				/* clr cb jobdone status for next trigger */
				vos_flag_clr(p_hdl->cb_tsk_flg_id, (KDRV_ISE_TSK_JOB_DONE | KDRV_ISE_TSK_JOB_FAIL));
			} else {
				DBG_ERR("cur job is null\r\n");
				rt = E_SYS;
				vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
			}
		}
	} else {
		vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
		if (p_hdl->p_cur_job) {
			/* put job head to proc_list & cb_list */
			rt = p_hdl->p_cur_job->job_id;
			p_hdl->p_cur_job->trig_param = *(KDRV_ISE_TRIG_PARAM *)p_param;
			if (p_cb_func != NULL) {
				p_hdl->p_cur_job->cb_fp = *p_cb_func;
			}

			kdrv_ise_int_put_job(p_hdl);
			vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);

			/* set flag trigger proc_task */
			kdrv_ise_int_trig_proc_tsk(p_hdl);
		} else {
			DBG_ERR("cur job is null\r\n");
			rt = E_SYS;
			vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
		}
	}

	vos_sem_sig(p_hdl->internal_sem);
	return rt;
}

#if 0
#endif

INT32 kdrv_ise_linked_list_trigger(UINT32 id)
{
	/* obsolete api */
	INT32 rt ;
	KDRV_ISE_TRIGGER_PARAM trig_param;

	trig_param.wait_end = 1;
	trig_param.time_out_ms = KDRV_ISE_TIMEOUT_MS;
	rt = kdrv_ise_trigger(id, &trig_param, NULL, NULL);

	return rt;
}

UINT32 kdrv_ise_get_linked_list_buf_size(UINT32 id, UINT32 proc_job_nums)
{
	/* obsolete api, linklist buffer is alloc by kdrv_ise */
	return 0;
}

