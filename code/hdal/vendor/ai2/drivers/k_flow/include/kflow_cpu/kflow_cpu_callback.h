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

#ifndef _KFLOW_AI_CPU_CALLBACK_H_
#define _KFLOW_AI_CPU_CALLBACK_H_

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/error_no.h"

#include "kflow_cpu/kflow_cpu.h"

extern int kflow_ai_cpu_init(UINT32 proc_id);
extern int kflow_ai_cpu_exit(UINT32 proc_id);
extern void kflow_ai_cpu_cb(UINT32 proc_id, KFLOW_AI_JOB* p_job); //proc
#if NN_DLI
extern void kflow_ai_cpu_cb2(UINT32 proc_id, KFLOW_AI_JOB* p_job); //start
extern void kflow_ai_cpu_cb3(UINT32 proc_id, KFLOW_AI_JOB* p_job); //end
extern KFLOW_AI_JOB* kflow_ai_cpu_wait(UINT32 proc_id, UINT32* p_event);
#else
extern KFLOW_AI_JOB* kflow_ai_cpu_wait(UINT32 proc_id);
#endif
extern void kflow_ai_cpu_sig(UINT32 proc_id, KFLOW_AI_JOB* p_job);
extern int kflow_ai_cpu_init_cb(VOID);
extern int kflow_ai_cpu_uninit_cb(VOID);
extern int kflow_ai_cpu_uninit_cb_path(UINT32 net_id);

#endif //_KFLOW_AI_CPU_CALLBACK_H_
