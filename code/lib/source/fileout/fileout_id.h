/**
    @brief 		Header file of fileout library.

    @file 		fileout_id.h

    @ingroup 	mFileOut

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _FILEOUT_ID_H
#define _FILEOUT_ID_H

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define PRI_ISF_FILEOUT             10
#define STKSIZE_ISF_FILEOUT         4 * 1024
#define FILEOUT_DRV_NUM             2
#define FILEOUT_DRV_NAME_FIRST      'A'
#define FILEOUT_DRV_NAME_LAST       (FILEOUT_DRV_NAME_FIRST + FILEOUT_DRV_NUM - 1)

//tsk flag definitions
#define FILEOUT_FLG_TSK_OPS_IN      FLGPTN_BIT(0)
#define FILEOUT_FLG_TSK_STOP        FLGPTN_BIT(1)
#define FILEOUT_FLG_TSK_READY       FLGPTN_BIT(2)
#define FILEOUT_FLG_TSK_RELQUE      FLGPTN_BIT(3)
#define FILEOUT_FLG_TSK_STOPPED     FLGPTN_BIT(4)

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/
typedef struct {
	THREAD_HANDLE TaskID;
	ID FlagID;
	SEM_HANDLE SemID;
} FILEOUT_CTRL_OBJ, *PFILEOUT_CTRL_OBJ;

/*-----------------------------------------------------------------------------*/
/* Extern Variables                                                            */
/*-----------------------------------------------------------------------------*/
extern FILEOUT_CTRL_OBJ _SECTION(".kercfg_data") gFileOutCtrlObj[FILEOUT_DRV_NUM];
extern ID _SECTION(".kercfg_data") FILEOUT_FLGID_PORT_CTL;
extern SEM_HANDLE _SECTION(".kercfg_data") FILEOUT_SEMID_QUEUE_CTL;
extern SEM_HANDLE _SECTION(".kercfg_data") FILEOUT_SEMID_PATH_CTL;
extern SEM_HANDLE _SECTION(".kercfg_data") FILEOUT_SEMID_OPS_CTL;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
PFILEOUT_CTRL_OBJ fileout_get_ctrl_obj(CHAR drive);

void fileout_queue_lock(UINT32 queue_idx);
void fileout_queue_unlock(UINT32 queue_idx);

void fileout_qhandle_lock(void);
void fileout_qhandle_unlock(void);

void fileout_path_lock(void);
void fileout_path_unlock(void);

void fileout_ops_lock(void);
void fileout_ops_unlock(void);

extern void fileout_uninstall_id(void);
extern void fileout_install_id(void) _SECTION(".kercfg_text");
#endif //_FILEOUT_ID_H