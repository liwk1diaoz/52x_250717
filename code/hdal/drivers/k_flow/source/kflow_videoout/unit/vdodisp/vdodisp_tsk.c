/**
   vdodisp, Task control implementation

    @file       vdodisp_tsk.c
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "vdodisp_int.h"
#include "vdodisp_run.h"
#include "kdrv_videoout/kdrv_vdoout.h"




THREAD_DECLARE(vdodisp_tsk, arglist)
{
    VDODISP_DEVID device_id = (VDODISP_DEVID)arglist;
	FLGPTN flg_ptn = 0;
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);
	ID flag_id = p_ctrl->cond.flag_id;
	FLGPTN Mask = FLGVDODISP_CLOSE | FLGVDODISP_SUSPEND | FLGVDODISP_RESUME | FLGVDODISP_CMD;

	THREAD_ENTRY();

	x_vdodisp_run_open(device_id);
	set_flg(flag_id, FLGVDODISP_OPENED);

	while (1) {
		x_vdodisp_set_idle(device_id);
		PROFILE_TASK_IDLE();
		wai_flg(&flg_ptn, flag_id, Mask, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		if ((flg_ptn & FLGVDODISP_CLOSE) != 0U) {
			x_vdodisp_run_close(device_id);
			set_flg(flag_id, FLGVDODISP_STOPPED);
			break;
		}

		if ((flg_ptn & FLGVDODISP_SUSPEND) != 0U) {
			if (p_ctrl->cond.b_suspended) {
				x_vdodisp_wrn(device_id, VDODISP_WR_SUSPEND_TWICE);
			} else {
				x_vdodisp_run_suspend(device_id);
			}
			set_flg(flag_id, FLGVDODISP_SUSPENDED);
			continue;
		}

		if ((flg_ptn & FLGVDODISP_RESUME) != 0U) {
			if (p_ctrl->cond.b_suspended == FALSE) {
				x_vdodisp_wrn(device_id, VDODISP_WR_NOT_IN_SUSPEND);
			} else {
				x_vdodisp_run_resume(device_id);
			}
			set_flg(flag_id, FLGVDODISP_RESUMED);
		}

		if ((p_ctrl->cond.b_stopped == TRUE) || (p_ctrl->cond.b_suspended == TRUE)) {
			x_vdodisp_wrn(device_id, VDODISP_WR_CMD_SKIP);
        	if ((p_ctrl->event_cb.fp_release)&&(p_ctrl->cmd_ctrl.cmd.idx ==VDODISP_CMD_IDX_SET_FLIP)) {
                VDO_FRAME *p_img_next = *(VDO_FRAME **)p_ctrl->cmd_ctrl.cmd.in.p_data;
                if(p_img_next){
        		    p_ctrl->event_cb.fp_release(device_id, p_img_next);
                }
        	}
			continue;
		}

		if ((flg_ptn & FLGVDODISP_CMD) != 0U) {
			x_vdodisp_run_cmd(device_id, &p_ctrl->cmd_ctrl.cmd);
		}
	}

	THREAD_RETURN(0);

}
THREAD_DECLARE(vdodisp_ide, arglist)
{
    VDODISP_DEVID device_id = (VDODISP_DEVID)arglist;
#if (DEVICE_COUNT >= 2)
static KDRV_DEV_ENGINE m_tbl_disp_eng_id[2] ={KDRV_VDOOUT_ENGINE0,KDRV_VDOOUT_ENGINE1};
#else
static KDRV_DEV_ENGINE m_tbl_disp_eng_id[2] ={KDRV_VDOOUT_ENGINE0,KDRV_VDOOUT_ENGINE0};
#endif

	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);
	UINT32 engine_id = m_tbl_disp_eng_id[device_id];
	VDODISP_FP_FRM_END_CB fp = NULL;
	while (!p_ctrl->ide.bStop) {
		PROFILE_TASK_IDLE();
		//p_dispobj->wait_frm_end(TRUE);
		if(kdrv_vddo_set(KDRV_DEV_ID(0, engine_id, 0), VDDO_DISPCTRL_WAIT_FRM_END, NULL) != 0)  {
			;
		}
		PROFILE_TASK_BUSY();
		fp = p_ctrl->event_cb.fp_frm_end;
		if (fp) {
			fp(device_id);
		}
	}
	p_ctrl->ide.bStop = FALSE;
	THREAD_RETURN(0);
}
THREAD_DECLARE(vdodisp_ide_0, arglist)
{
	return vdodisp_ide((void *)VDODISP_DEVID_0);
}
#if (DEVICE_COUNT >= 2)
THREAD_DECLARE(vdodisp_ide_1, arglist)
{
	return vdodisp_ide((void *)VDODISP_DEVID_1);
}
#endif

THREAD_DECLARE(vdodisp_tsk_0, arglist)
{
	return vdodisp_tsk((void *)VDODISP_DEVID_0);
}
#if (DEVICE_COUNT >= 2)
THREAD_DECLARE(vdodisp_tsk_1, arglist)
{
	return vdodisp_tsk((void *)VDODISP_DEVID_1);
}
#endif

