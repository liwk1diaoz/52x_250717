#include "msdcnvt_int.h"
#include "msdcnvt_id.h"

void msdcnvt_mem_push_host_to_bk(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_BACKGROUND_CTRL *p_bk = &p_ctrl->bk;

	p_bk->trans_size = p_host->lb_length;
	memcpy(p_bk->p_pool, p_host->p_pool, p_bk->trans_size);
}

void msdcnvt_mem_pop_bk_to_host(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_BACKGROUND_CTRL *p_bk = &p_ctrl->bk;

	memcpy(p_host->p_pool, p_bk->p_pool, p_bk->trans_size);
}

BOOL msdcnvt_bk_run_cmd(void (*p_call)(void))
{
	MSDCNVT_BACKGROUND_CTRL *p_bk = &msdcnvt_get_ctrl()->bk;

	if (p_bk->b_cmd_running) {
		DBG_ERR("Is Running\r\n");
		return FALSE;
	}

	p_bk->b_cmd_running = TRUE;
	p_bk->fp_call = p_call;

#if defined(__UITRON)
	INT32 n_retry = 10;
	while (sta_tsk(p_bk->task_id, 0) != E_OK && n_retry-- && p_bk->b_cmd_running) {
		swtimer_delayms(10);
	}
	if (n_retry <= 0) {
		return FALSE;
	}
#else
	THREAD_CREATE(m_msdcnvt_tsk_id, msdcnvt_tsk, NULL, "msdcnvt_tsk");
	THREAD_RESUME(m_msdcnvt_tsk_id);
#endif

	return TRUE;
}

BOOL msdcnvt_bk_is_finish(void)
{
	if (msdcnvt_get_ctrl()->bk.b_cmd_running) {
		return FALSE;
	}

	return TRUE;
}

BOOL msdcnvt_bk_host_lock(void)
{
	MSDCNVT_BACKGROUND_CTRL *p_bk = &msdcnvt_get_ctrl()->bk;

	if (p_bk->b_service_lock) {
		DBG_ERR("already locked!\r\n");
		return FALSE;
	}

	p_bk->b_service_lock = TRUE;
	return TRUE;
}

BOOL msdcnvt_bk_host_unlock(void)
{
	MSDCNVT_BACKGROUND_CTRL *p_bk = &msdcnvt_get_ctrl()->bk;

	if (!p_bk->b_service_lock) {
		DBG_ERR("not locked!\r\n");
		return FALSE;
	} else if (p_bk->b_cmd_running) {
		DBG_ERR("Bk is Running\r\n");
		return FALSE;
	}

	p_bk->b_service_lock = FALSE;
	return TRUE;
}

BOOL msdcnvt_bk_host_is_lock(void)
{
	return msdcnvt_get_ctrl()->bk.b_service_lock;
}

THREAD_DECLARE(msdcnvt_tsk, arglist)
{
	MSDCNVT_BACKGROUND_CTRL *p_bk = &msdcnvt_get_ctrl()->bk;

#if defined(__UITRON)
	kent_tsk();
	PROFILE_TASK_IDLE();
#endif

	if (p_bk->fp_call) {
		p_bk->fp_call();
	}

#if defined(__UITRON)
	PROFILE_TASK_BUSY();
#endif

	p_bk->b_cmd_running = FALSE;
	THREAD_RETURN(0);
}
