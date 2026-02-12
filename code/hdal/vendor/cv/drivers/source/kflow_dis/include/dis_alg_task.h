/**
	@brief Source file of kflow_ai_net.

	@file kflow_ai_core.h

	@ingroup kflow_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/

#ifndef _DIS_ALG_TASK_H_
#define _DIS_ALG_TASK_H_

#include "dis_alg_lib.h"

#define KFLOW_DIS_TSK_PRI       2

#define FLGPTN_INIT           FLGPTN_BIT(0)
#define FLGPTN_UNINIT         FLGPTN_BIT(1)
#define FLGPTN_PROC           FLGPTN_BIT(2)
#define FLGPTN_PAUSE          FLGPTN_BIT(3)
#define FLGPTN_RESUME         FLGPTN_BIT(4)
#define FLGPTN_STOP           FLGPTN_BIT(5)
#define FLGPTN_IDLE           FLGPTN_BIT(31)


extern THREAD_HANDLE _SECTION(".kercfg_data") KFLOW_DIS_TSK_ID;
extern THREAD_DECLARE(kflow_dis_task, arglist);
extern SEM_HANDLE _SECTION(".kercfg_data") dis_semi_id;


#endif //_DIS_ALG_TASK_H_