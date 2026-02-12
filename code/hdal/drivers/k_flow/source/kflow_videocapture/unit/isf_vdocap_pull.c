#include "isf_vdocap_int.h"
#include "isf_vdocap_dbg.h"
#include "kwrap/util.h"

#if (ISF_NEW_PULL == ENABLE)
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)
#endif

#define USE_DROP_AFTER_PROC	ENABLE

#define WATCH_PORT			ISF_OUT(0)
#define FORCE_DUMP_DATA		DISABLE

#if (ISF_NEW_PULL == ENABLE)

static ISF_RV _isf_queue_open(PISF_PULL_QUEUE p_queue, void* p_sem_q)
{
	unsigned long flags;
	if((!p_queue) || !(p_sem_q)) {
		return ISF_ERR_NO_CONFIG;
	}
	DBG_IND("\"%s\".queue[%d] open --- begin!\r\n", p_queue->unit_name, p_queue->id);
	loc_cpu(flags);
	//SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); //force unlock
	//memset(p_queue, 0, sizeof(ISF_DATA_QUEUE));
	DBG_IND("\"%s\".queue[%d] open --- set full=0 !!!\r\n", p_queue->unit_name, p_queue->id);
	p_queue->en = 1; //close->open
	p_queue->max = 0;
	p_queue->cnt = 0;
	p_queue->data = 0;
	p_queue->p_sem_q = (SEM_HANDLE*)p_sem_q;
	p_queue->is_full = FALSE;
	p_queue->head = 0;
	p_queue->tail = 0;
	//SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem_q)[0]); //initial with lock
	unl_cpu(flags);
	DBG_IND("\"%s\".queue[%d] open --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}

static ISF_RV _isf_queue_start(PISF_PULL_QUEUE p_queue, UINT32 n, ISF_DATA* p_data)
{
	unsigned long flags;
	if((!p_queue) || !(p_data)) {
		return ISF_ERR_NO_CONFIG;
	}
	if(p_queue->en == 0) {
		DBG_IND("\"%s\".queue[%d] start --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] start --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	DBG_IND("\"%s\".queue[%d] start --- begin!\r\n", p_queue->unit_name, p_queue->id);
	loc_cpu(flags);
	if (p_queue->en == 2) {//start->start
		p_queue->max = n;
		p_queue->data = p_data;
	} else { //open->start
		memset(p_data, 0, sizeof(ISF_DATA)*n);
		p_queue->en = 2; //open->start
		p_queue->max = n;
		p_queue->data = p_data;
		p_queue->is_full = FALSE;
		p_queue->head = 0;
		p_queue->tail = 0;
	}
	unl_cpu(flags);
	DBG_IND("\"%s\".queue[%d] start --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
static BOOL _isf_queue_is_full(PISF_PULL_QUEUE p_queue)
{
	unsigned long flags;
	BOOL is_full = FALSE;
	if((!p_queue))
		return FALSE;
	if(!(p_queue->p_sem_q))
		return FALSE;
	loc_cpu(flags);
	is_full = ((p_queue->head == p_queue->tail) && (p_queue->is_full == TRUE));
	unl_cpu(flags);
	//DBG_IND("\"%s\".queue[%d] is full %d\r\n", p_queue->unit_name, p_queue->id, is_full);
	return is_full;
}
/*
static BOOL _isf_queue_is_full_and_get(PISF_PULL_QUEUE p_queue, ISF_DATA *data_out)
{
	unsigned long flags;
	BOOL is_full = FALSE;
	if((!p_queue))
		return FALSE;
	if(!(p_queue->p_sem_q))
		return FALSE;

	loc_cpu(flags);
	is_full = ((p_queue->head == p_queue->tail) && (p_queue->is_full == TRUE));
	if (is_full) {
		DBG_IND("\"%s\".queue[%d] is full %d\r\n", p_queue->unit_name, p_queue->id, is_full);

		memcpy(data_out, &p_queue->data[p_queue->head], sizeof(ISF_DATA));

		p_queue->head= (p_queue->head+ 1) % p_queue->max;
		p_queue->cnt --;

		if (p_queue->head == p_queue->tail) { // Check data full
			DBG_IND("\"%s\".queue[%d] get data --- set full=0 !!!\r\n", p_queue->unit_name, p_queue->id);
			p_queue->is_full = FALSE;
		}
		DBG_IND("\"%s\".queue[%d] get data %d --- end\r\n", p_queue->unit_name, p_queue->id, p_queue->cnt);   // this case should be enter if force unlock
	}
	unl_cpu(flags);
	return is_full;
}
*/

static ISF_RV _isf_queue_put_data(PISF_PULL_QUEUE p_queue, ISF_DATA *data_in)
{
	unsigned long flags;
	void* p_sem_q;

	if((!p_queue) || !(data_in)) {
		return ISF_ERR_NO_CONFIG;
	}
	if(p_queue->en == 0) {
		DBG_IND("\"%s\".queue[%d] put data --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	p_sem_q = p_queue->p_sem_q;
	if(!(p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] put data --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if (((p_queue->en != 2) && (p_queue->en != 3)) || (p_queue->data == NULL)) {
		return ISF_ERR_NOT_START;
	}
	//DBG_IND("\"%s\".queue[%d] put data --- begin\r\n", p_queue->unit_name, p_queue->id);
	loc_cpu(flags);
	if ((p_queue->head == p_queue->tail) && (p_queue->is_full == TRUE)) {
		DBG_IND("\"%s\".queue[%d] put data --- is full!\r\n", p_queue->unit_name, p_queue->id);
		unl_cpu(flags);
		return ISF_ERR_QUEUE_FULL;
	} else {
		if(p_queue->max == 0) {
			DBG_IND("\"%s\".queue[%d] put data --- depth = 0!\r\n", p_queue->unit_name, p_queue->id);
			unl_cpu(flags);
			return ISF_ERR_NOT_AVAIL;
		}
		memcpy(&p_queue->data[p_queue->tail], data_in, sizeof(ISF_DATA));

		p_queue->tail = (p_queue->tail + 1) % p_queue->max;
		p_queue->cnt++;

		if (p_queue->head == p_queue->tail) { // Check data full
			//DBG_IND("\"%s\".queue[%d] put data --- set full=1 !!!\r\n", p_queue->unit_name, p_queue->id);
			p_queue->is_full = TRUE;
		}
		unl_cpu(flags);
		SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); // unlock
		//DBG_IND("\"%s\".queue[%d] put data %d ---  end\r\n", p_queue->unit_name, p_queue->id, p_queue->cnt);
		return ISF_OK;
	}
}

static ISF_RV _isf_queue_get_data(PISF_PULL_QUEUE p_queue, ISF_DATA *data_out, INT32 wait_ms)
{
	unsigned long flags;
	UINT32 early_en  = 0;
	void* p_sem_q;

	if((!p_queue) || !(data_out)) {
		return ISF_ERR_NO_CONFIG;
	}
	early_en = p_queue->en;
	if(p_queue->en == 0) {
		DBG_IND("\"%s\".queue[%d] get data --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	p_sem_q = p_queue->p_sem_q;
	if(!(p_sem_q)) {
		DBG_IND("\"%s\".queue[%d] get data --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}

	//NOTE: to allow get after stop!
	//if ((p_queue->en != 2) || (p_queue->data == NULL)) {
	//	return ISF_ERR_NOT_START;
	//}
	//DBG_IND("\"%s\".queue[%d] get data (%d) ms -- begin\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	if (wait_ms < 0) {
try_wait_1:
		// blocking (wait until data available) , if success Q - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(((SEM_HANDLE*)p_sem_q)[0])) {
			return ISF_ERR_FAILED;
		}
		//DBG_IND("\"%s\".queue[%d] get data --- (%d) ms ready!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	} else  {
try_wait_2:
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success Q - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_sem_q)[0], vos_util_msec_to_tick(wait_ms))) {
			//DBG_IND("\"%s\".queue[%d] get data --- (%d) ms timeout!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
			if (p_queue->en == 0) {
				return ISF_ERR_NOT_OPEN_YET;
			}
			//NOTE: to allow get after stop!
			//if ((p_queue->en != 2) || (p_queue->data == NULL)) {
			//	return ISF_ERR_NOT_START;
			//}
			loc_cpu(flags);
			if (p_queue->cnt) {
				//DBG_DUMP("get_data x: cnt=%d head=%d tail=%d full=%d\r\n", p_queue->cnt, p_queue->head, p_queue->tail, p_queue->is_full);
				unl_cpu(flags);
				goto try_get;
			}
			unl_cpu(flags);
			if (wait_ms == 0) {
				return ISF_ERR_QUEUE_EMPTY;
			}
			return ISF_ERR_WAIT_TIMEOUT;
		}
		//DBG_IND("\"%s\".queue[%d] get data --- (%d) ms ready!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	}
try_get:
	if(p_queue->en == 0) {
		// enter before close, and force quit by close!
		//DBG_IND("\"%s\".queue[%d] get data --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}

	//NOTE: to allow get after stop!
	//if ((p_queue->en != 2) || (p_queue->data == NULL)) {
	//	return ISF_ERR_NOT_START;
	//}
	loc_cpu(flags);
	if ((p_queue->head == p_queue->tail) && (p_queue->is_full == FALSE)) {
		unl_cpu(flags);
		if (early_en == 3) {
			//this is a early pull() after start(), before queue start, should try again!
			if (wait_ms < 0)
				goto try_wait_1;
			if (wait_ms > 0)
				goto try_wait_2;
		} else {
			if (wait_ms < 0 && p_queue->p_sem_q && p_queue->max)
				goto try_wait_1;
		}
		//DBG_IND("\"%s\".queue[%d] get data --- is empty !!!\r\n", p_queue->unit_name, p_queue->id);   // this case should be enter if force unlock
		return ISF_ERR_QUEUE_EMPTY;
	} else {
		#if 0
		if(data_out == 0) DBG_ERR("queue %08x get data --- data_out=0 !!!\r\n", (UINT32)p_queue);
		if(p_queue == 0) DBG_ERR("queue %08x get data --- p_queue=0 !!!\r\n", (UINT32)p_queue);
		if(p_queue->data == 0) DBG_ERR("queue %08x get data --- p_queue->data=0 !!!\r\n", (UINT32)p_queue);
		if((&p_queue->data[p_queue->head]) == 0) DBG_ERR("queue %08x get data --- &(p_queue->data[%d])=0 !!!\r\n", (UINT32)p_queue, p_queue->head);
		#endif
		if (0 == p_queue->cnt) {
			DBG_WRN("cnt=%d head=%d tail=%d full=%d\r\n", p_queue->cnt, p_queue->head, p_queue->tail, p_queue->is_full);
			unl_cpu(flags);
			return ISF_ERR_QUEUE_EMPTY;
		}
		memcpy(data_out, &p_queue->data[p_queue->head], sizeof(ISF_DATA));

		p_queue->head= (p_queue->head+ 1) % p_queue->max;
		p_queue->cnt --;

		#if 1
		if (p_queue->head == p_queue->tail) { // Check data full
			//DBG_IND("\"%s\".queue[%d] get data --- set full=0 !!!\r\n", p_queue->unit_name, p_queue->id);
			p_queue->is_full = FALSE;
		}
		#else
		p_queue->is_full = FALSE;
		#endif
		//DBG_IND("\"%s\".queue[%d] get data %d --- end\r\n", p_queue->unit_name, p_queue->id, p_queue->cnt);   // this case should be enter if force unlock
		unl_cpu(flags);
		return ISF_OK;
	}
}

static ISF_RV _isf_queue_pre_stop(PISF_PULL_QUEUE p_queue)
{
	unsigned long flags;
	if((!p_queue)) {
		return ISF_ERR_NO_CONFIG;
	}
	if ((p_queue->en == 0)) {
		DBG_ERR("\"%s\".queue[%d] pre stop --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] pre stop --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if (((p_queue->en != 2) && (p_queue->en != 3)) || (p_queue->data == NULL)) {
		return ISF_ERR_NOT_START;
	}
	DBG_IND("\"%s\".queue[%d] pre stop --- begin!\r\n", p_queue->unit_name, p_queue->id);
	loc_cpu(flags);
	p_queue->en = 1; //start->open
	unl_cpu(flags);
	DBG_IND("\"%s\".queue[%d] pre stop --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}

static ISF_RV _isf_queue_stop(PISF_PULL_QUEUE p_queue)
{
	unsigned long flags;
	void* p_sem_q;

	if((!p_queue)) {
		return ISF_ERR_NO_CONFIG;
	}
	if ((p_queue->en == 0)) {
		DBG_ERR("\"%s\".queue[%d] stop --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	p_sem_q = p_queue->p_sem_q;
	if(!(p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] stop --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	//NOTE: to allow stop after pre_stop!
	//if ((p_queue->en != 2) || (p_queue->data == NULL)) {
	//	return ISF_ERR_NOT_START;
	//}
	DBG_IND("\"%s\".queue[%d] stop --- begin!\r\n", p_queue->unit_name, p_queue->id);
	loc_cpu(flags);
	if((p_queue->max == 0) && (p_queue->data == NULL)) {
		unl_cpu(flags);
		DBG_ERR("\"%s\".queue[%d] stop --- invalid state-2!\r\n", p_queue->unit_name, p_queue->id);
		return ISF_ERR_NOT_START;
	}
	if (p_queue->cnt != 0) {
		DBG_WRN("\"%s\".queue[%d] stop --- %d data still remain!\r\n", p_queue->unit_name, p_queue->id, p_queue->cnt);
	}
	p_queue->en = 1; //start->open
	p_queue->head = 0;
	p_queue->tail = 0;
	p_queue->is_full = FALSE;
	p_queue->max = 0; //destory
	p_queue->cnt = 0;
	p_queue->data = NULL;
	//SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem_q)[0]); // force unlock
	unl_cpu(flags);
	if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_sem_q)[0], vos_util_msec_to_tick(0))) {
		DBG_IND("\"%s\".queue[%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", p_queue->unit_name, p_queue->id);
	}
	SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); // force unlock
	DBG_IND("\"%s\".queue[%d] stop --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}

static ISF_RV _isf_queue_force_stop(PISF_PULL_QUEUE p_queue)
{
	unsigned long flags;
	void* p_sem_q;

	if((!p_queue)) {
		return ISF_ERR_NO_CONFIG;
	}
	p_sem_q = p_queue->p_sem_q;
	if(!(p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] force_stop --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(p_queue->en == 3) {
		//this time is fast start/stop! do complete stop

		DBG_IND("\"%s\".queue[%d] ext_stop - begin\r\n", p_queue->unit_name, p_queue->id);
		loc_cpu(flags);
		if (p_queue->cnt != 0) {
			DBG_WRN("\"%s\".queue[%d] ext_stop --- %d data still remain!\r\n", p_queue->unit_name, p_queue->id, p_queue->cnt);
		}
		p_queue->en = 1; //start->open
		p_queue->head = 0;
		p_queue->tail = 0;
		p_queue->is_full = FALSE;
		p_queue->max = 0; //destory
		p_queue->cnt = 0;
		p_queue->data = NULL;
		unl_cpu(flags);
		//wait for keep sem count not change
		if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_sem_q)[0], vos_util_msec_to_tick(0))) {
			DBG_IND("\"%s\".queue[%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", p_queue->unit_name, p_queue->id);
		}
		SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); // force unlock

		DBG_IND("\"%s\".queue[%d] ext_stop --- end!\r\n", p_queue->unit_name, p_queue->id);
		return ISF_OK;
	}

	DBG_IND("\"%s\".queue[%d] force_stop - begin\r\n", p_queue->unit_name, p_queue->id);
	//if (p_queue->data == NULL) {
	if (1) {
		//wait for keep sem count not change
		if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_sem_q)[0], vos_util_msec_to_tick(0))) {
			DBG_IND("\"%s\".queue[%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", p_queue->unit_name, p_queue->id);
		}
		SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); // force unlock
	}
	DBG_IND("\"%s\".queue[%d] force_stop - end\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}

static ISF_RV _isf_queue_close(PISF_PULL_QUEUE p_queue)
{
	unsigned long flags;
	void* p_sem_q;
	if(!p_queue) {
		return ISF_ERR_NO_CONFIG;
	}
	if ((p_queue->en == 0)) {
		DBG_ERR("\"%s\".queue[%d] close --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] close --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if ((p_queue->en != 1) || (p_queue->data != NULL)) {
		DBG_ERR("\"%s\".queue[%d] close --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_ALREADY_START;
	}
	/*
	if(p_queue->data != NULL) {
		_isf_queue_stop(p_queue);
	}
	*/
	p_sem_q = p_queue->p_sem_q;
	DBG_IND("\"%s\".queue[%d] close --- begin!\r\n", p_queue->unit_name, p_queue->id);
	loc_cpu(flags);
	if((p_queue->max != 0) || (p_queue->data != NULL)) {
		unl_cpu(flags);
		DBG_ERR("\"%s\".queue[%d] close --- invalid state-2!\r\n", p_queue->unit_name, p_queue->id);
		return ISF_ERR_ALREADY_START;
	}
	p_queue->en = 0; //open->close
	p_queue->p_sem_q = NULL;
	unl_cpu(flags);
	//wait for keep sem count not change
	if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_sem_q)[0], vos_util_msec_to_tick(0))) {
		DBG_IND("\"%s\".queue[%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", p_queue->unit_name, p_queue->id);
	}
	SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); //force unlock
	DBG_IND("\"%s\".queue[%d] close --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
#endif


extern UINT32 _vdocap_oport_releasedata(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 j);

void _isf_vdocap_oqueue_do_open(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue open - begin\r\n", p_thisunit->unit_name, i);
	}
#endif
	p_ctx->pullq.output[i].sign = ISF_SIGN_QUEUE;
	p_ctx->pullq.output[i].unit_name = p_thisunit->unit_name;
	p_ctx->pullq.output[i].id = i;
#if (ISF_NEW_PULL == ENABLE)
	_isf_queue_open(&(p_ctx->pullq.output[i]),
		&(p_ctx->ISF_VDOCAP_OUTQ_SEM_ID[i]));
#else
	_isf_queue2_open(&(p_ctx->pullq.output[i]),
		&(p_ctx->ISF_VDOCAP_OUT_SEM_ID[i]),
		&(p_ctx->ISF_VDOCAP_OUTQ_SEM_ID[i]));
#endif
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue open - end\r\n", p_thisunit->unit_name, i);
	}
#endif
}

static void _isf_vdocap_oqueue_clear(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	BOOL ret;
	ISF_DATA data;
	ISF_DATA* p_data = &data;

#if (ISF_NEW_PULL == ENABLE)
	_isf_queue_pre_stop(&(p_ctx->pullq.output[i])); //reject put
#else
	_isf_queue2_pre_stop(&(p_ctx->pullq.output[i])); //reject put
#endif
#if (ISF_NEW_PULL == ENABLE)
	while ((ret = _isf_queue_get_data(&(p_ctx->pullq.output[i]), p_data, 0)) == ISF_OK) { //remove all data in queue
#else
	while ((ret = _isf_queue2_get_data(&(p_ctx->pullq.output[i]), p_data, 0)) == ISF_OK) { //remove all data in queue
#endif
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue drop -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_data->h_data);
	}
#endif
		//UINT32 j = p_data->func;
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_USER_PROBE_PULL);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_SKIP, ISF_ERR_QUEUE_DROP);
		//_vdoprc_oport_releasedata(p_thisunit, oport, j);
	}
#if (ISF_NEW_PULL == ENABLE)
	_isf_queue_stop(&(p_ctx->pullq.output[i]));
#else
	_isf_queue2_stop(&(p_ctx->pullq.output[i]));
#endif
}
void _isf_vdocap_oqueue_do_start(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	PISF_PULL_QUEUE p_queue;

#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue start - begin\r\n", p_thisunit->unit_name, i);
	}
#endif
	p_queue = &p_ctx->pullq.output[i];
	//start->start and queue number changed, need to clear queue
	if (p_queue->en == 2 && p_queue->max != p_ctx->pullq.num[i]) {
		//DBG_ERR("clear queue\r\n");
		_isf_vdocap_oqueue_clear(p_thisunit, oport);
	}
#if (ISF_NEW_PULL == ENABLE)
	_isf_queue_start(&(p_ctx->pullq.output[i]),
		p_ctx->pullq.num[i], (p_ctx->pullq.data[i]));
#else
	_isf_queue2_start(&(p_ctx->pullq.output[i]),
		p_ctx->pullq.num[i], (p_ctx->pullq.data[i]));
#endif
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue start - end\r\n", p_thisunit->unit_name, i);
	}
#endif
}

void _isf_vdocap_oqueue_force_stop(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
#if (ISF_NEW_PULL == ENABLE)
	_isf_queue_force_stop(&(p_ctx->pullq.output[i]));
#else
	_isf_queue2_force_stop(&(p_ctx->pullq.output[i]));
#endif
}

void _isf_vdocap_oqueue_do_stop(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue stop - begin\r\n", p_thisunit->unit_name, i);
	}
#endif
	_isf_vdocap_oqueue_clear(p_thisunit, oport);
	p_ctx->pullq.num[i] = 0; //clear


#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue stop - end\r\n", p_thisunit->unit_name, i);
	}
#endif
}
void _isf_vdocap_oqueue_do_close(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue close - begin\r\n", p_thisunit->unit_name, i);
	}
#endif
	if(i < ISF_VDOCAP_OUT_NUM) {
		#if (ISF_NEW_PULL == ENABLE)
		_isf_queue_close(&(p_ctx->pullq.output[i]));
		#else
		_isf_queue2_close(&(p_ctx->pullq.output[i]));
		#endif
	}
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue close - end\r\n", p_thisunit->unit_name, i);
	}
#endif
}
ISF_RV _isf_vdocap_oqueue_do_push(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	ISF_RV ret = FALSE;
	if(!p_data)
		return ISF_ERR_NULL_POINTER;

#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue push -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_data->h_data);
	}
#endif
#if (ISF_NEW_PULL == ENABLE)
	ret = _isf_queue_put_data(&(p_ctx->pullq.output[i]), p_data);
#else
	ret = _isf_queue2_put_data(&(p_ctx->pullq.output[i]), p_data);
#endif

	if (ret == ISF_OK) {
#if (FORCE_DUMP_DATA == ENABLE)
		if (oport == WATCH_PORT) {
			DBG_DUMP("%s.out[%d]! Out Queue push ok\r\n", p_thisunit->unit_name, i);
		}
#endif
		return ISF_OK;
	}

#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue push fail -- %d\r\n", p_thisunit->unit_name, i, ret);
	}
#endif
	return ret;
}

ISF_RV _isf_vdocap_oqueue_do_pull(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	ISF_RV ret;
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}
	if(!p_data)
		return ISF_ERR_NULL_POINTER;

	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL, ISF_ENTER);

	//DBGD(wait_ms);
#if (ISF_NEW_PULL == ENABLE)
	ret = _isf_queue_get_data(&(p_ctx->pullq.output[i]), p_data, wait_ms);
#else
	ret = _isf_queue2_get_data(&(p_ctx->pullq.output[i]), p_data, wait_ms);
#endif

	if (ret == ISF_OK) {
		//UINT32 j = p_data->func;
		//_vdoprc_oport_releasedata(p_thisunit, oport, j);
		//p_thisunit->p_base->do_release(p_thisunit, oport, p_data, 0);
		//p_thisunit->p_base->do_probe(p_thisunit, oport, 0, ISF_OK);
		//p_data->func = 0;
#if (FORCE_DUMP_DATA == ENABLE)
		if (oport == WATCH_PORT) {
			DBG_DUMP("%s.out[%d]! Out Queue pull ok  -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_data->h_data);
		}
#endif
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL, ISF_OK);
		return ISF_OK;
	}

	if (wait_ms > 0) {
		// wait_ms > 0 : using timeout
		// wait_ms == 0 : not wait
		// wait_ms < 0 : blocking
#if (FORCE_DUMP_DATA == ENABLE)
		if (oport == WATCH_PORT) {
			DBG_DUMP("%s.out[%d]! Out Queue pull fail -- timeout\r\n", p_thisunit->unit_name, i);
		}
#endif
		if(ret == ISF_ERR_WAIT_TIMEOUT) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_SKIP, ISF_ERR_WAIT_TIMEOUT);
		} else if(ret == ISF_ERR_NOT_START) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_WRN, ISF_ERR_NOT_START);
		} else if(ret == ISF_ERR_NOT_OPEN_YET) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_WRN, ISF_ERR_NOT_START);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_ERR, ret);
		}
		return ret;
	}

#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue pull fail -- %d\r\n", p_thisunit->unit_name, i, ret);
	}
#endif
	if(ret == ISF_ERR_QUEUE_EMPTY) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_SKIP, ISF_ERR_QUEUE_EMPTY);
	} else if(ret == ISF_ERR_NOT_START) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_WRN, ISF_ERR_NOT_START);
	} else if(ret == ISF_ERR_NOT_OPEN_YET) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_WRN, ISF_ERR_NOT_START);
	} else {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_ERR, ret);
	}
	return ret;
}

ISF_RV _isf_vdocap_oqueue_do_push_with_clean(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 keep_this)
{
	ISF_RV rr = ISF_ERR_QUEUE_EMPTY;
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	UINT32 i =  oport - ISF_OUT_BASE;
	if (p_ctx->pullq.num[i]) {
#if (USE_DROP_AFTER_PROC == ENABLE)
		//drop after proc
#if (ISF_NEW_PULL == ENABLE)
/*
		ISF_DATA inner_data;
		ISF_DATA* p_inner_data = &inner_data;
		if (TRUE == _isf_queue_is_full_and_get(&(p_ctx->pullq.output[i]), p_inner_data)) { //pull queue is full, remove data in queue
			//UINT32 j = p_inner_data->func;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue release -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_inner_data->h_data);
	}
#endif
			p_thisunit->p_base->do_release(p_thisunit, oport, p_inner_data, ISF_USER_PROBE_PULL);
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_SKIP, ISF_ERR_QUEUE_FULL);
			//_vdocap_oport_releasedata(p_thisunit, oport, j);
		}
*/
//DBG_DUMP("do push: cnt=%d head=%d tail=%d full=%d\r\n", p_ctx->pullq.output[i].cnt, p_ctx->pullq.output[i].head, p_ctx->pullq.output[i].tail, p_ctx->pullq.output[i].is_full);
		if (TRUE == _isf_queue_is_full(&(p_ctx->pullq.output[i]))) { //pull queue is full, not allow push
			ISF_DATA inner_data;
			ISF_DATA* p_inner_data = &inner_data;
			//while(_isf_queue_get_data(&(p_ctx->pullq.output[i]), p_inner_data, 0) == ISF_OK) { //remove all data in queue
			if (_isf_queue_get_data(&(p_ctx->pullq.output[i]), p_inner_data, 0) == ISF_OK) { //remove all data in queue
				//UINT32 j = p_inner_data->func;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue release -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_inner_data->h_data);
	}
#endif
				p_thisunit->p_base->do_release(p_thisunit, oport, p_inner_data, ISF_USER_PROBE_PULL);
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_SKIP, ISF_ERR_QUEUE_FULL);
				//_vdocap_oport_releasedata(p_thisunit, oport, j);
				/*
				if (p_ctx->queue_scheme) {
					break;
				}
				*/
			}
		} else {
		}
#else
		if (TRUE == _isf_queue2_is_full(&(p_ctx->pullq.output[i]))) { //pull queue is full, not allow push
			ISF_DATA inner_data;
			ISF_DATA* p_inner_data = &inner_data;
			while(_isf_queue2_get_data(&(p_ctx->pullq.output[i]), p_inner_data, 0) == ISF_OK) { //remove all data in queue
				//UINT32 j = p_inner_data->func;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue release -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_inner_data->h_data);
	}
#endif
				p_thisunit->p_base->do_release(p_thisunit, oport, p_inner_data, ISF_USER_PROBE_PULL);
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL_SKIP, ISF_ERR_QUEUE_FULL);
				//_vdocap_oport_releasedata(p_thisunit, oport, j);
				if (p_ctx->queue_scheme) {
					break;
				}
			}
		} else {
		}
#endif
#endif
	if (keep_this) {
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue add -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_data->h_data);
	}
#endif
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
	}
		rr = _isf_vdocap_oqueue_do_push(p_thisunit, oport, p_data);
		if(rr != ISF_OK) {
			if (keep_this) {
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue release -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_data->h_data);
	}
#endif
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, rr);
			}
#if (USE_DROP_AFTER_PROC == ENABLE)
			DBG_WRN("%s.out[%d]! push to oqueue fail, r=%d, drop.\r\n", p_thisunit->unit_name, i, rr);
#endif
		}
	}
	return rr;
}
