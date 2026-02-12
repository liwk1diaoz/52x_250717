/**
    @brief 		Header file of fileout library.

    @file 		bsmux_tsk.h

    @ingroup 	mBsMux

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _BSMUX_TSK_H
#define _BSMUX_TSK_H

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
extern void ISF_BsMux_Tsk(void);

/**
    Tsk: trigger tsk motion
*/
extern void bsmux_tsk_init(void);
extern void bsmux_tsk_start(void);
extern void bsmux_tsk_bs_in(void);
extern void bsmux_tsk_getall(void);
extern void bsmux_tsk_stop(void);
extern void bsmux_tsk_wait_idle(void);
extern void bsmux_tsk_destroy(void);

/**
    Tsk: set tsk status init/idle/resume/run/suspend
*/
extern ER bsmux_tsk_status_init(UINT32 id);
extern ER bsmux_tsk_status_idle(UINT32 id);
extern ER bsmux_tsk_status_resume(UINT32 id);
extern ER bsmux_tsk_status_run(UINT32 id);
extern ER bsmux_tsk_status_suspend(UINT32 id);
extern ER bsmux_tsk_status_dbg(UINT32 value);
extern ER bsmux_tsk_action_dbg(UINT32 value);

/**
    Tsk: trig tsk do getall/cutfile
*/
extern ER bsmux_tsk_trig_getall(UINT32 id);
extern ER bsmux_tsk_trig_cutfile(UINT32 id);

extern ER bsmux_tsk_still(UINT32 id);
extern ER bsmux_tsk_awake(UINT32 id);

#endif //_BSMUX_TSK_H
