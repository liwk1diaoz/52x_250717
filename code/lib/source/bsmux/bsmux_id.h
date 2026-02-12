/**
    @brief 		Header file of bsmux library.

    @file 		bsmux_id.h

    @ingroup 	mBsMux

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _BSMUX_ID_H
#define _BSMUX_ID_H

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define PRI_ISF_BSMUX             	8
#define STKSIZE_ISF_BSMUX         	32768

//tsk flag definitions
#define BSMUX_FLG_TSK_READY         FLGPTN_BIT(0)
#define BSMUX_FLG_TSK_BS_IN         FLGPTN_BIT(1)
#define BSMUX_FLG_TSK_STOP          FLGPTN_BIT(2)
#define BSMUX_FLG_TSK_STOPPED       FLGPTN_BIT(3)
#define BSMUX_FLG_TSK_CLOSE         FLGPTN_BIT(4)
#define BSMUX_FLG_TSK_SUSOK         FLGPTN_BIT(5)
#define BSMUX_FLG_TSK_GETALL        FLGPTN_BIT(6)
#define BSMUX_FLG_TSK_IDLE          FLGPTN_BIT(7)
#define BSMUX_FLG_TSK_GETALLDONE    FLGPTN_BIT(8)

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/
typedef struct {
	THREAD_HANDLE TaskID;
	ID FlagID;
	SEM_HANDLE SemID;
} BSMUX_CTRL_OBJ, *PBSMUX_CTRL_OBJ;

/*-----------------------------------------------------------------------------*/
/* Extern Variables                                                            */
/*-----------------------------------------------------------------------------*/
extern BSMUX_CTRL_OBJ _SECTION(".kercfg_data") gBsMuxCtrlObj;
extern SEM_HANDLE     _SECTION(".kercfg_data") BSMUX_SEMID_SAVEQ_CTL;
extern SEM_HANDLE     _SECTION(".kercfg_data") BSMUX_SEMID_MKHDR_CTL;
extern SEM_HANDLE     _SECTION(".kercfg_data") BSMUX_SEMID_STATUS_CTL;
extern SEM_HANDLE     _SECTION(".kercfg_data") BSMUX_SEMID_ACTION_CTL;
extern SEM_HANDLE     _SECTION(".kercfg_data") BSMUX_SEMID_BUFFER_CTL;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
extern PBSMUX_CTRL_OBJ bsmux_get_ctrl_obj(void);
extern void bsmux_saveq_lock(void);
extern void bsmux_saveq_unlock(void);
extern void bsmux_mkhdr_lock(void);
extern void bsmux_mkhdr_unlock(void);
extern void bsmux_status_lock(void);
extern void bsmux_status_unlock(void);
extern void bsmux_action_lock(void);
extern void bsmux_action_unlock(void);
extern void bsmux_buffer_lock(void);
extern void bsmux_buffer_unlock(void);
extern void bsmux_uninstall_id(void);
extern void bsmux_install_id(void) _SECTION(".kercfg_text");

#endif //_BSMUX_ID_H
