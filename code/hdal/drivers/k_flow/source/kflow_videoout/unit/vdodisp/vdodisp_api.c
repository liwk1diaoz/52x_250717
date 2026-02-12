/**
    vdodisp, major APIs implementation.

    @file       vdodisp_api.c
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "vdodisp_int.h"
#include "vdodisp_id.h"
#include "vdodisp_run.h"
#include "vdodisp_sx_cmd.h"
#include "../isf_vdoout_platform.h"

#if !defined(__LINUX)
#include <string.h>
_ALIGNED(4) static UINT8 m_y_black[8] = {0};
_ALIGNED(4) static UINT8 m_uv_black[8] = {0};
#endif

// fake get_tid() function,alwayse is other task
static unsigned int get_tid (THREAD_HANDLE *p_tskid )
{
    *p_tskid = 0;
    return 0;
}

VDODISP_ER vdodisp_init(VDODISP_DEVID id,const VDODISP_INIT *p_Init)
{
	{
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(id);

		if (p_Init->ui_api_ver != VDODISP_API_VERSION) {
			return x_vdodisp_err(id, VDODISP_ER_API_VERSION);
		}
#if 0  //no uninit,therefor can not check init_key
        if (p_ctrl->cond.ui_init_key == CFG_VDODISP_INIT_KEY) {
			x_vdodisp_wrn(id, VDODISP_WR_INIT_TWICE);
			return VDODISP_ER_OK;
		}
#endif
		memset(p_ctrl, 0, sizeof(VDODISP_CTRL));
        #if defined(__LINUX)
		p_ctrl->black_buf.m_y = vdoout_alloc(BLACK_BUF_SIZE);
		p_ctrl->black_buf.m_uv = vdoout_alloc(BLACK_BUF_SIZE);
        #else
		p_ctrl->black_buf.m_y = m_y_black;
		p_ctrl->black_buf.m_uv = m_uv_black;
        #endif
		if ((p_ctrl->black_buf.m_y == NULL)||(p_ctrl->black_buf.m_uv == NULL)) {
			DBG_ERR("kalloc %d fail\r\n", BLACK_BUF_SIZE);
			return x_vdodisp_err(VDODISP_DEVID_0, VDODISP_ER_NOMEM);
		}
		p_ctrl->init = *p_Init;
		switch (id) {
		case VDODISP_DEVID_0:
			p_ctrl->cond.task_id = VDODISP_TSK_ID_0;
			p_ctrl->cond.sem_id = &VDODISP_SEM_ID_0;
			p_ctrl->cond.flag_id = VDODISP_FLG_ID_0;
			p_ctrl->cond.ide_id = VDODISP_IDE_ID_0;
			break;
#if (DEVICE_COUNT >= 2)
		case VDODISP_DEVID_1:
			p_ctrl->cond.task_id = VDODISP_TSK_ID_1;
			p_ctrl->cond.sem_id = &VDODISP_SEM_ID_1;
			p_ctrl->cond.flag_id = VDODISP_FLG_ID_1;
			p_ctrl->cond.ide_id = VDODISP_IDE_ID_1;
			break;
#endif
		default:
			DBG_ERR("cannot handle device_id %d > 1.\r\n", id);
			return x_vdodisp_err(id, VDODISP_ER_SYS);
		}
        if(p_ctrl->cond.task_id==NULL)
            return x_vdodisp_err(id, VDODISP_ER_SYS);

		p_ctrl->cmd_ctrl.p_func_tbl = x_vdodisp_get_call_tbl(&p_ctrl->cmd_ctrl.ui_num_func);
		p_ctrl->cond.ui_init_key = CFG_VDODISP_INIT_KEY;
	}
	#if 0
	x_vdodisp_sx_cmd_install_cmd();
	#endif
	return VDODISP_ER_OK;
}

VDODISP_ER vdodisp_open(VDODISP_DEVID id)
{
	{
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(id);
		x_vdodisp_lock(id);
		if (x_vdodisp_get_state(id) != VDODISP_STATE_UNKNOWN) {
			x_vdodisp_unlock(id);
			return x_vdodisp_err(id, VDODISP_ER_STATE);
		}

		clr_flg(p_ctrl->cond.flag_id, 0xFFFF);//for VOS flag reserved
		p_ctrl->cond.b_opened = FALSE;
		p_ctrl->cond.b_suspended = FALSE;
		p_ctrl->cond.b_stopped = FALSE;
		p_ctrl->ide.bStop = FALSE;
#if 0

		if (THREAD_RESUME(p_ctrl->cond.task_id) != E_OK) {
			x_vdodisp_unlock(id);
			return x_vdodisp_err(id, VDODISP_ER_STA_TASK);
		}
        #if (IDE_TASK==ENABLE)
		if (THREAD_RESUME(p_ctrl->cond.ide_id) != E_OK) {
			x_vdodisp_unlock(id);
			return x_vdodisp_err(id, VDODISP_ER_STA_TASK);
		}
        #endif

#else
    	THREAD_SET_PRIORITY(p_ctrl->cond.task_id, PRI_VDODISP);
		THREAD_RESUME(p_ctrl->cond.task_id);
        #if (IDE_TASK==ENABLE)
    	THREAD_SET_PRIORITY(p_ctrl->cond.task_id, PRI_VDODISP);
		THREAD_RESUME(p_ctrl->cond.ide_id);
        #endif
#endif
		p_ctrl->cond.b_opened = TRUE;
		x_vdodisp_unlock(id);
	}

	return VDODISP_ER_OK;
}

VDODISP_ER vdodisp_close(VDODISP_DEVID id)
{
    #if (IDE_TASK==ENABLE)
	INT32 n = 10;
    #endif
	{
		THREAD_HANDLE tid;
		FLGPTN flg_ptn;
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(id);

		get_tid(&tid);
		if (tid == p_ctrl->cond.task_id) {
			return x_vdodisp_err(id, VDODISP_ER_INVALID_CALL);
		}
		x_vdodisp_lock(id);
		if (p_ctrl->cond.b_opened == FALSE) {
			x_vdodisp_unlock(id);
			x_vdodisp_wrn(id, VDODISP_WR_ALREADY_CLOSED);
			return VDODISP_ER_OK;
		}
		//x_vdodisp_wait_idle(id);
		p_ctrl->ide.bStop = TRUE;
		set_flg(p_ctrl->cond.flag_id, FLGVDODISP_CLOSE);
	    if (wai_flg(&flg_ptn, p_ctrl->cond.flag_id, FLGVDODISP_STOPPED, TWF_CLR) != E_OK) {
		    return x_vdodisp_err((id), VDODISP_ER_WAIT_STOPED);
	    }
		x_vdodisp_set_state(id, VDODISP_STATE_UNKNOWN);
		p_ctrl->cond.b_opened = FALSE;
		//THREAD_DESTROY(p_ctrl->cond.task_id);
        #if (IDE_TASK==ENABLE)
		while (n-- && p_ctrl->ide.bStop) {
			vos_util_delay_ms(33);
		}
		if (p_ctrl->ide.bStop) { // if not clean
			DBG_ERR("cannot stop ide task.\r\n");
			return x_vdodisp_err(id, VDODISP_ER_SYS);
		}
        #endif

        #if defined(__LINUX)
		vdoout_free(p_ctrl->black_buf.m_y);
		vdoout_free(p_ctrl->black_buf.m_uv);
        #endif
		p_ctrl->black_buf.m_y = NULL;
		p_ctrl->black_buf.m_uv = NULL;

		x_vdodisp_unlock(id);
	}
	return VDODISP_ER_OK;
}

VDODISP_ER vdodisp_suspend(VDODISP_DEVID id)
{
	{
		THREAD_HANDLE tid;
		FLGPTN flg_ptn;
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(id);

		get_tid(&tid);
		if (tid == p_ctrl->cond.task_id) {
			return x_vdodisp_run_suspend(id);
		}
		x_vdodisp_lock(id);
		if (p_ctrl->cond.b_opened == FALSE) {
			x_vdodisp_unlock(id);
			return x_vdodisp_err(id, VDODISP_ER_STATE);
		}
		x_vdodisp_wait_idle(id);
		set_flg(p_ctrl->cond.flag_id, FLGVDODISP_SUSPEND);
		wai_flg(&flg_ptn, p_ctrl->cond.flag_id, FLGVDODISP_SUSPENDED, TWF_CLR);
		x_vdodisp_unlock(id);
	}
	return VDODISP_ER_OK;
}

VDODISP_ER vdodisp_resume(VDODISP_DEVID id)
{
	{
		THREAD_HANDLE tid;
		FLGPTN flg_ptn;
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(id);

		get_tid(&tid);
		if (tid == p_ctrl->cond.task_id) {
			return x_vdodisp_run_resume(id);
		}
		x_vdodisp_lock(id);
		if (p_ctrl->cond.b_opened == FALSE) {
			x_vdodisp_unlock(id);
			return x_vdodisp_err(id, VDODISP_ER_STATE);
		}
		x_vdodisp_wait_idle(id);
		set_flg(p_ctrl->cond.flag_id, FLGVDODISP_RESUME);
		wai_flg(&flg_ptn, p_ctrl->cond.flag_id, FLGVDODISP_RESUMED, TWF_CLR);
		x_vdodisp_unlock(id);
	}
	return VDODISP_ER_OK;
}

VDODISP_ER vdodisp_cmd(const VDODISP_CMD *p_cmd)
{
	THREAD_HANDLE tid;
	VDODISP_DEVID device_id = p_cmd->device_id;
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);
	const VDODISP_CMD_DESC *p_desc = &p_ctrl->cmd_ctrl.p_func_tbl[p_cmd->idx];
	VDODISP_ER er = VDODISP_ER_OK;

	if (p_cmd->idx != p_desc->idx) {
		return x_vdodisp_err(device_id, VDODISP_ER_CMD_NOT_MATCH);
	}

	if (p_cmd->in.ui_num_byte != p_desc->ui_num_byte_in) {
		return x_vdodisp_err(device_id, VDODISP_ER_CMD_IN_DATA);
	}

	if (p_cmd->out.ui_num_byte != p_desc->ui_num_byte_out) {
		return x_vdodisp_err(device_id, VDODISP_ER_CMD_OUT_DATA);
	}

	get_tid(&tid);
	//DBG_ERR("******** tid %x %x\r\n",tid,p_ctrl->cond.task_id);
	if (tid == p_ctrl->cond.task_id) {
		p_ctrl->dbg_info.last_caller_task = tid;
		#if _TODO
		p_ctrl->dbg_info.last_caller_addr = (UINT32)__CALL__ - 8;
		#endif
		return x_vdodisp_run_cmd(device_id, p_cmd);
	}
	x_vdodisp_lock(device_id);
	if (p_ctrl->cond.b_opened == FALSE) {
		x_vdodisp_unlock(device_id);
		return x_vdodisp_err(device_id, VDODISP_ER_STATE);
	}

	if (p_cmd->prop.b_enter_only_idle) {
		FLGPTN flag = 0;
		set_flg(p_ctrl->cond.flag_id, FLGVDODISP_POLLING);
		wai_flg(&flag, p_ctrl->cond.flag_id, FLGVDODISP_POLLING | FLGVDODISP_IDLE, TWF_ORW | TWF_CLR);

		if ((flag & FLGVDODISP_IDLE) == 0) {
			x_vdodisp_unlock(device_id);
			return VDODISP_ER_NOT_IN_IDLE;
		}
	} else {
		x_vdodisp_wait_idle(device_id);
	}

	p_ctrl->cmd_ctrl.cmd = *p_cmd;

	if (p_cmd->prop.b_exit_cmd_finish == FALSE) {
		if ((p_cmd->out.p_data != NULL) && (p_cmd->prop.fp_finish_cb == NULL)) {
			x_vdodisp_set_idle(device_id);
			x_vdodisp_unlock(device_id);
			return x_vdodisp_err(device_id, VDODISP_ER_OUT_DATA_VOLATILE);
		}

		if (sizeof(VDODISP_CMD_MAXDATA) >= p_cmd->in.ui_num_byte) {
			memcpy(&p_ctrl->cmd_ctrl.max_in_buf, p_cmd->in.p_data, (size_t)p_cmd->in.ui_num_byte);
			p_ctrl->cmd_ctrl.cmd.in.p_data = &p_ctrl->cmd_ctrl.max_in_buf;
		} else {
			x_vdodisp_set_idle(device_id);
			x_vdodisp_unlock(device_id);
			return x_vdodisp_err(device_id, VDODISP_ER_CMD_MAXDATA);
		}
	}

	p_ctrl->dbg_info.last_caller_task = tid;
	#if _TODO
	p_ctrl->dbg_info.last_caller_addr = (UINT32)__CALL__ - 8;
	#endif
	set_flg(p_ctrl->cond.flag_id, FLGVDODISP_CMD);

	if (p_cmd->prop.b_exit_cmd_finish) {
		x_vdodisp_wait_idle(device_id);
		er = p_ctrl->cmd_ctrl.er_cmd;
		x_vdodisp_set_idle(device_id);
	}
	x_vdodisp_unlock(device_id);
	return er;
}

VDODISP_ER vdodisp_async_suspend(VOID)
{
	INT32 i;
	for (i = 0; i < VDODISP_DEVID_MAX_NUM; i++) {
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(i);

		x_vdodisp_lock(i);
		if (p_ctrl->cond.b_opened == FALSE) {
			x_vdodisp_unlock(i);
			return x_vdodisp_err(i, VDODISP_ER_STATE);
		}

		set_flg(p_ctrl->cond.flag_id, FLGVDODISP_SUSPEND);
		x_vdodisp_unlock(i);
	}
	return VDODISP_ER_OK;
}

VDODISP_ER vdodisp_async_close(VOID)
{
	INT32 i;
	for (i = 0; i < VDODISP_DEVID_MAX_NUM; i++) {
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(i);

		x_vdodisp_lock(i);
		if (p_ctrl->cond.b_opened == FALSE) {
			x_vdodisp_unlock(i);
			return x_vdodisp_err(i, VDODISP_ER_STATE);
		}

		set_flg(p_ctrl->cond.flag_id, FLGVDODISP_CLOSE);
		x_vdodisp_unlock(i);
	}
	return VDODISP_ER_OK;
}

VOID vdodisp_dump_info(int (*dump)(const char *fmt, ...))
{
	INT32 i;
	UINT32 ui;
	BOOL   bOK;
	for (i = 0; i < VDODISP_DEVID_MAX_NUM; i++) {
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(i);
		VDODISP_DBGINFO  *p_info = &p_ctrl->dbg_info;

		//dump all debug inforation to uart
		dump("vdodisp[%d]:\r\n", i);
		dump("Api Ver %08X\r\n", VDODISP_API_VERSION);
		dump("Last Error: %d\r\n", p_info->last_er);
		dump("Last Warning: %d\r\n", p_info->last_wr);
		dump("Last Cmd: %d\r\n", p_info->last_cmd_idx);
		dump("Last Caller Task ID = %d\r\n", p_info->last_caller_task);
		dump("Last Caller Address = 0x%08X\r\n", p_info->last_caller_addr);
		dump("Current State: %d\r\n", p_ctrl->state);
        #if IDE_DUMP
        dump("ide status %x\r\n",x_vdodisp_get_addr((void *)IDE_ADDR) );
        dump("ide vdo addr %x\r\n",x_vdodisp_get_addr((void *)(IDE_ADDR+IDE_V1_DB0_Y_OFS)) );
        #endif

		bOK = TRUE;
		for (ui = 0; ui < p_ctrl->cmd_ctrl.ui_num_func; ui++) {
			if (p_ctrl->cmd_ctrl.p_func_tbl[ui].idx != ui) {
				bOK = FALSE;
				break;
			}
		}
		dump("Check FuncTable...%s\r\n", (bOK) ? "PASS" : "FAIL");

		bOK = TRUE;
		for (ui = 0; ui < p_ctrl->cmd_ctrl.ui_num_func; ui++) {
			if (p_ctrl->cmd_ctrl.p_func_tbl[ui].ui_num_byte_in > sizeof(VDODISP_CMD_MAXDATA)) {
				bOK = FALSE;
				break;
			}
		}
		dump("Check VDODISP_CMD_MAXDATA...%s\r\n", (bOK) ? "PASS" : "FAIL");
		if (p_ctrl->flip_ctrl.p_img) {
			dump("Current Image: Y=0x%08X,UV=0x%08X,Pitch=%d,w=%d, h=%d, fmt=0x%08X\r\n"
				 , p_ctrl->flip_ctrl.p_img->addr[0]
				 , p_ctrl->flip_ctrl.p_img->addr[1]
				 , p_ctrl->flip_ctrl.p_img->loff[0]
				 , p_ctrl->flip_ctrl.p_img->size.w
				 , p_ctrl->flip_ctrl.p_img->size.h
				 , p_ctrl->flip_ctrl.p_img->pxlfmt);
		} else {
			dump("No Current Image.\r\n");
		}

		dump("Display Window: {%d, %d, %d, %d}\r\n"
			 , p_ctrl->flip_ctrl.disp_desc.wnd.x
			 , p_ctrl->flip_ctrl.disp_desc.wnd.y
			 , p_ctrl->flip_ctrl.disp_desc.wnd.w
			 , p_ctrl->flip_ctrl.disp_desc.wnd.h);

		dump("Device Size: {%d, %d}\r\n"
			 , p_ctrl->flip_ctrl.disp_desc.device_size.w
			 , p_ctrl->flip_ctrl.disp_desc.device_size.h);

		dump("Aspect Ratio: {%d, %d}\r\n"
			 , p_ctrl->flip_ctrl.disp_desc.aspect_ratio.w
			 , p_ctrl->flip_ctrl.disp_desc.aspect_ratio.h);

		dump("\r\n");
	}
}
