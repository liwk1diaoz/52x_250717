/**
	@brief Source file of vendor net flow sample.

	@file net_flow_sample.c

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "kwrap/util.h"
#include "kflow_ai_net/kflow_ai_net_queue.h"

//=============================================================
#define __CLASS__ 				"[ai][kflow][queue]"
#include "kflow_ai_debug.h"
//=============================================================

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
ER nvt_ai_proc_queue_open(PAI_PROC_QUEUE p_queue, UINT32 proc_id, void* p_sem, void* p_sem_q)
{
	if((!p_queue) || !(p_sem_q)) {
		return E_SYS;
	}
	SEM_WAIT(((SEM_HANDLE*)p_sem)[0]);
	p_queue->proc_id = proc_id;
#if 0
	DBG_DUMP("- proc[%d] - queue open\r\n", (int)p_queue->proc_id);
#endif
	p_queue->en = 1; //close->open
	p_queue->data = 0;
	p_queue->p_sem = (SEM_HANDLE*)p_sem;
	p_queue->p_sem_q = (SEM_HANDLE*)p_sem_q;
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem_q)[0]); //initial with lock
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	return E_OK;
}

ER nvt_ai_proc_queue_put(PAI_PROC_QUEUE p_queue, UINT32 data)
{
	if((!p_queue)) {
		return E_SYS;
	}
	if(p_queue->en == 0) {
		return E_OBJ;
	}
	
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
#if 0
	DBG_DUMP("- proc[%d] - queue put\r\n", (int)p_queue->proc_id);
#endif
	p_queue->data = data;
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem_q)[0]); // unlock
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	return E_OK;
}

ER nvt_ai_proc_queue_cancel(PAI_PROC_QUEUE p_queue)
{
	if((!p_queue)) {
		return E_SYS;
	}
	if(p_queue->en == 0) {
		return E_OBJ;
	}
	
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
#if 0
	DBG_DUMP("- proc[%d] - queue cancel\r\n", (int)p_queue->proc_id);
#endif
	p_queue->data = 0;
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem_q)[0]); // unlock
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	return E_OK;
}

//-1: sync, 0: async, >0: timeout
ER nvt_ai_proc_queue_get(PAI_PROC_QUEUE p_queue, UINT32* p_data, INT32 wait_ms)
{
	if((!p_queue)) {
		return E_SYS;
	}
	if(p_queue->en == 0) {
		return E_OBJ;
	}

#if 0
	DBG_DUMP("- proc[%d] - queue get\r\n", (int)p_queue->proc_id);
#endif
	if (wait_ms < 0) {
		// blocking (wait until data available) , if success Q - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(((SEM_HANDLE*)p_queue->p_sem_q)[0])) {
			return E_RLWAI;
		}
	} else  {
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success Q - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_queue->p_sem_q)[0], vos_util_msec_to_tick(wait_ms))) {
			if (p_queue->en == 0) {
				return E_OBJ;
			}
			if (wait_ms == 0) {
				return E_NOEXS;
			}
			return E_TMOUT;
		}
	}
	if(p_queue->en == 0) {
		return E_OBJ;
	}

	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	if(p_queue->data == 0) {
#if 0
		DBG_DUMP("- proc[%d] - queue get: CANCEL\r\n", (int)p_queue->proc_id);
#endif
		SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
		return E_NOEXS;
	}
#if 0
	DBG_DUMP("- proc[%d] - queue get: OK\r\n", (int)p_queue->proc_id);
#endif
	if (p_data) p_data[0] = p_queue->data;
	p_queue->data = 0;
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	return E_OK;
}

ER nvt_ai_proc_queue_close(PAI_PROC_QUEUE p_queue)
{
	void* p_sem;
	void* p_sem_q;
	if(!p_queue) {
		return E_SYS;
	}
	if ((p_queue->en == 0)) {
		return E_OBJ;
	}
	
#if 0
	DBG_DUMP("- proc[%d] - queue get: CLOSE\r\n", (int)p_queue->proc_id);
#endif
	p_queue->en = 0; //open->close
	p_sem = p_queue->p_sem;
	p_sem_q = p_queue->p_sem_q;
	SEM_WAIT(((SEM_HANDLE*)p_sem)[0]);
	p_queue->p_sem = NULL;
	p_queue->p_sem_q = NULL;
	SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); //force unlock
	SEM_SIGNAL(((SEM_HANDLE*)p_sem)[0]);
	return E_OK;
}

