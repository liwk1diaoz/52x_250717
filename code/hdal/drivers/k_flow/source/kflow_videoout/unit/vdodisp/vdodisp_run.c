/**
    vdodisp, Task running operation implementation

    @file       vdodisp_run.c
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "vdodisp_run.h"

/**
    Task open extension action

    if there's something to do when task start (such as initial some variable).
    Please implemnt here.

    @return error code.
*/
VDODISP_ER x_vdodisp_run_open(VDODISP_DEVID device_id)
{
	//VDODISP_CTRL* p_ctrl = x_vdodisp_get_ctrl();

	x_vdodisp_set_state(device_id, VDODISP_STATE_OPEN_BEGIN);

	//Do something extension here
	//.................


	x_vdodisp_set_state(device_id, VDODISP_STATE_OPEN_END);
	return VDODISP_ER_OK;
}

/**
    Task close extension action

    if there's something to do when task close. Please implemnt here.

    @return error code.
*/
VDODISP_ER x_vdodisp_run_close(VDODISP_DEVID device_id)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);

	x_vdodisp_set_state(device_id, VDODISP_STATE_CLOSE_BEGIN);

	//Do something extension here
	//.................

	p_ctrl->cond.b_stopped = TRUE;
	x_vdodisp_set_state(device_id, VDODISP_STATE_CLOSE_END);
	return VDODISP_ER_OK;
}

/**
    Task suspend extension action

    if there's something to do when task suspend. Please implemnt here.

    @return error code.
*/
VDODISP_ER x_vdodisp_run_suspend(VDODISP_DEVID device_id)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);

	x_vdodisp_set_state(device_id, VDODISP_STATE_SUSPEND_BEGIN);

	//Do something extension here
	//.................

	p_ctrl->cond.b_suspended = TRUE;
	x_vdodisp_set_state(device_id, VDODISP_STATE_SUSPEND_END);
	return VDODISP_ER_OK;
}

/**
    Task resume extension action

    if there's something to do when task resume. Please implemnt here.

    @return error code.
*/
VDODISP_ER x_vdodisp_run_resume(VDODISP_DEVID device_id)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);

	x_vdodisp_set_state(device_id, VDODISP_STATE_RESUME_BEGIN);

	//Do something here
	//.................


	p_ctrl->cond.b_suspended = FALSE;
	x_vdodisp_set_state(device_id, VDODISP_STATE_RESUME_END);
	return VDODISP_ER_OK;
}

/**
    Task command manager

    any command will be assigned an action function via this manager. don't
    modify here if you want to do some extension job.

    @return error code.
*/
VDODISP_ER x_vdodisp_run_cmd(VDODISP_DEVID device_id, const VDODISP_CMD *p_cmd)
{
	VDODISP_CTRL  *p_ctrl = x_vdodisp_get_ctrl(device_id);
	VDODISP_FP_CMD fp_cmd = p_ctrl->cmd_ctrl.p_func_tbl[p_cmd->idx].fp_cmd;

	p_ctrl->cmd_ctrl.er_cmd = VDODISP_ER_OK;
	p_ctrl->dbg_info.last_cmd_idx = p_cmd->idx;

	if (p_cmd->idx >= VDODISP_CMD_IDX_MAX_NUM) {
		p_ctrl->cmd_ctrl.er_cmd = VDODISP_ER_INVALID_CMD_IDX;
		goto quit;
	}

	if (fp_cmd == NULL) {
		p_ctrl->cmd_ctrl.er_cmd = VDODISP_ER_CMD_MAP_NULL;
		goto quit;
	}

	x_vdodisp_set_state(device_id, VDODISP_STATE_CMD_BEGIN);
	p_ctrl->cmd_ctrl.er_cmd = fp_cmd(p_cmd);
	x_vdodisp_set_state(device_id, VDODISP_STATE_CMD_END);

quit:
	if (p_cmd->prop.fp_finish_cb != NULL) {
		VDODISP_FINISH FinishInfo = {0};
		FinishInfo.idx = p_cmd->idx;
		FinishInfo.er = p_ctrl->cmd_ctrl.er_cmd;
		FinishInfo.p_user_data = p_cmd->prop.p_user_data;
		x_vdodisp_set_state(device_id, VDODISP_STATE_CMD_CB_BEGIN);
		p_cmd->prop.fp_finish_cb(&FinishInfo);
		x_vdodisp_set_state(device_id, VDODISP_STATE_CMD_CB_END);
	}

	if (p_ctrl->cmd_ctrl.er_cmd == VDODISP_ER_OK) {
		return p_ctrl->cmd_ctrl.er_cmd;
	}

	return x_vdodisp_err(device_id, p_ctrl->cmd_ctrl.er_cmd);
}
