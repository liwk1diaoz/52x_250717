#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_q
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_q_debug_level = __DBGLVL__;
//module_param_named(isf_flow_q_debug_level, isf_flow_q_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_q_debug_level, "flow_q debug level");
///////////////////////////////////////////////////////////////////////////////

ISF_RV _isf_queue2_open(PISF_DATA_QUEUE p_queue, void* p_sem, void* p_sem_q)
{
	if((!p_queue) || !(p_sem) || !(p_sem_q)) {
		return ISF_ERR_NO_CONFIG;
	}
	DBG_IND("\"%s\".queue[%d] open --- begin!\r\n", p_queue->unit_name, p_queue->id);
	SEM_WAIT(((SEM_HANDLE*)p_sem)[0]);
 	//SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); //force unlock
	//memset(p_queue, 0, sizeof(ISF_DATA_QUEUE));
	DBG_IND("\"%s\".queue[%d] open --- set full=0 !!!\r\n", p_queue->unit_name, p_queue->id);
	p_queue->en = 1; //close->open
	p_queue->max = 0;
	p_queue->cnt = 0;
	p_queue->data = 0;
	p_queue->p_sem = (SEM_HANDLE*)p_sem;
	p_queue->p_sem_q = (SEM_HANDLE*)p_sem_q;
	p_queue->is_full = FALSE;
	p_queue->head = 0;
	p_queue->tail = 0;
	//SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem_q)[0]); //initial with lock
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	DBG_IND("\"%s\".queue[%d] open --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
EXPORT_SYMBOL(_isf_queue2_open);

ISF_RV _isf_queue2_pre_start(PISF_DATA_QUEUE p_queue)
{
	if((!p_queue)) {
		return ISF_ERR_NO_CONFIG;
	}
	if(p_queue->en == 0) {
		DBG_IND("\"%s\".queue[%d] start --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] start --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	DBG_IND("\"%s\".queue[%d] pre start --- begin!\r\n", p_queue->unit_name, p_queue->id);
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	p_queue->en = 3; //pre start
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	DBG_IND("\"%s\".queue[%d] pre start --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
EXPORT_SYMBOL(_isf_queue2_pre_start);

ISF_RV _isf_queue2_start(PISF_DATA_QUEUE p_queue, UINT32 n, ISF_DATA* p_data)
{
	if((!p_queue) || !(p_data)) {
		return ISF_ERR_NO_CONFIG;
	}
	if(p_queue->en == 0) {
		DBG_IND("\"%s\".queue[%d] start --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] start --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	DBG_IND("\"%s\".queue[%d] start --- begin!\r\n", p_queue->unit_name, p_queue->id);
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	memset(p_data, 0, sizeof(ISF_DATA)*n);
	p_queue->en = 2; //open->start
	p_queue->max = n;
	p_queue->data = p_data;
	p_queue->is_full = FALSE;
	p_queue->head = 0;
	p_queue->tail = 0;
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	DBG_IND("\"%s\".queue[%d] start --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
EXPORT_SYMBOL(_isf_queue2_start);

BOOL _isf_queue2_is_empty(PISF_DATA_QUEUE p_queue)
{
	BOOL is_empty = FALSE;
	if((!p_queue))
		return FALSE;
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q))
		return FALSE;
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	is_empty = ((p_queue->head == p_queue->tail) && (p_queue->is_full == FALSE));
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	return is_empty;
}
EXPORT_SYMBOL(_isf_queue2_is_empty);

BOOL _isf_queue2_is_full(PISF_DATA_QUEUE p_queue)
{
	BOOL is_full = FALSE;
	if((!p_queue)) 
		return FALSE;
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q))
		return FALSE;
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	is_full = ((p_queue->head == p_queue->tail) && (p_queue->is_full == TRUE));
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	return is_full;
}
EXPORT_SYMBOL(_isf_queue2_is_full);

ISF_RV _isf_queue2_put_data(PISF_DATA_QUEUE p_queue, ISF_DATA *data_in)
{
	if((!p_queue) || !(data_in)) {
		return ISF_ERR_NO_CONFIG;
	}
	if(p_queue->en == 0) {
		DBG_IND("\"%s\".queue[%d] put data --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] put data --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if (((p_queue->en != 2) && (p_queue->en != 3)) || (p_queue->data == NULL)) {
		return ISF_ERR_NOT_START;
	}
	DBG_IND("\"%s\".queue[%d] put data\r\n", p_queue->unit_name, p_queue->id);
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	if ((p_queue->head == p_queue->tail) && (p_queue->is_full == TRUE)) {
		DBG_IND("\"%s\".queue[%d] put data --- is full!\r\n", p_queue->unit_name, p_queue->id);
		SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
		return ISF_ERR_QUEUE_FULL;
	} else {
		if(p_queue->max == 0) {
			DBG_IND("\"%s\".queue[%d] get data --- depth = 0!\r\n", p_queue->unit_name, p_queue->id);
			SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
			return ISF_ERR_QUEUE_FULL;
		}
		memcpy(&p_queue->data[p_queue->tail], data_in, sizeof(ISF_DATA));

		p_queue->tail = (p_queue->tail + 1) % p_queue->max;
		p_queue->cnt++;

		if (p_queue->head == p_queue->tail) { // Check data full
			DBG_IND("\"%s\".queue[%d] put data --- set full=1 !!!\r\n", p_queue->unit_name, p_queue->id);
			p_queue->is_full = TRUE;
		}
		SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem_q)[0]); // unlock
		SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
		return ISF_OK;
	}
}
EXPORT_SYMBOL(_isf_queue2_put_data);

ISF_RV _isf_queue2_get_data(PISF_DATA_QUEUE p_queue, ISF_DATA *data_out, INT32 wait_ms)
{
	UINT32 early_en  = 0;
	
	if((!p_queue) || !(data_out)) {
		return ISF_ERR_NO_CONFIG;
	}
	early_en = p_queue->en;
	if(p_queue->en == 0) {
		DBG_IND("\"%s\".queue[%d] get data --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_IND("\"%s\".queue[%d] get data --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}

	//NOTE: to allow get after stop!
	//if ((p_queue->en != 2) || (p_queue->data == NULL)) {
	//	return ISF_ERR_NOT_START;
	//}
	DBG_IND("\"%s\".queue[%d] get data (%d) ms\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	if (wait_ms < 0) {
try_wait_1:
		// blocking (wait until data available) , if success Q - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(((SEM_HANDLE*)p_queue->p_sem_q)[0])) {
			return ISF_ERR_FAILED;
		}
		DBG_IND("\"%s\".queue[%d] get data --- (%d) ms ready!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	} else  {
try_wait_2:
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success Q - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_queue->p_sem_q)[0], vos_util_msec_to_tick(wait_ms))) {
			DBG_IND("\"%s\".queue[%d] get data --- (%d) ms timeout!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
			if (p_queue->en == 0) {
				return ISF_ERR_NOT_OPEN_YET;
			}
			//NOTE: to allow get after stop!
			//if ((p_queue->en != 2) || (p_queue->data == NULL)) {
			//	return ISF_ERR_NOT_START;
			//}
			if (wait_ms == 0) {
				return ISF_ERR_QUEUE_EMPTY;
			}
			return ISF_ERR_WAIT_TIMEOUT;
		}
		DBG_IND("\"%s\".queue[%d] get data --- (%d) ms ready!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	}

	if(p_queue->en == 0) {
		// enter before close, and force quit by close!
		DBG_IND("\"%s\".queue[%d] get data --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}

	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_IND("\"%s\".queue[%d] get data --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}

	//NOTE: to allow get after stop!
	//if ((p_queue->en != 2) || (p_queue->data == NULL)) {
	//	return ISF_ERR_NOT_START;
	//}
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	if ((p_queue->head == p_queue->tail) && (p_queue->is_full == FALSE)) {
		SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
		if (early_en == 3) {
			//this is a early pull() after start(), before queue start, should try again!
			if (wait_ms < 0)
				goto try_wait_1;
			if (wait_ms > 0)
				goto try_wait_2;
		}
		DBG_IND("\"%s\".queue[%d] get data --- is empty !!!\r\n", p_queue->unit_name, p_queue->id);   // this case should be enter if force unlock
		return ISF_ERR_QUEUE_EMPTY;
	} else {
		#if 0
		if(data_out == 0) DBG_ERR("queue %08x get data --- data_out=0 !!!\r\n", (UINT32)p_queue);
		if(p_queue == 0) DBG_ERR("queue %08x get data --- p_queue=0 !!!\r\n", (UINT32)p_queue);
		if(p_queue->data == 0) DBG_ERR("queue %08x get data --- p_queue->data=0 !!!\r\n", (UINT32)p_queue);
		if((&p_queue->data[p_queue->head]) == 0) DBG_ERR("queue %08x get data --- &(p_queue->data[%d])=0 !!!\r\n", (UINT32)p_queue, p_queue->head);
		#endif
		memcpy(data_out, &p_queue->data[p_queue->head], sizeof(ISF_DATA));

		p_queue->head= (p_queue->head+ 1) % p_queue->max;
		p_queue->cnt --;
		
		if (p_queue->head == p_queue->tail) { // Check data full
			DBG_IND("\"%s\".queue[%d] get data --- set full=0 !!!\r\n", p_queue->unit_name, p_queue->id);
			p_queue->is_full = FALSE;
		}
		SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
		return ISF_OK;
	}
}
EXPORT_SYMBOL(_isf_queue2_get_data);

ISF_RV _isf_queue2_pre_stop(PISF_DATA_QUEUE p_queue)
{
	if((!p_queue)) {
		return ISF_ERR_NO_CONFIG;
	}
	if ((p_queue->en == 0)) {
		DBG_ERR("\"%s\".queue[%d] pre stop --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] pre stop --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if (((p_queue->en != 2) && (p_queue->en != 3)) || (p_queue->data == NULL)) {
		return ISF_ERR_NOT_START;
	}
	DBG_IND("\"%s\".queue[%d] pre stop --- begin!\r\n", p_queue->unit_name, p_queue->id);
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	p_queue->en = 1; //start->open
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	DBG_IND("\"%s\".queue[%d] pre stop --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
EXPORT_SYMBOL(_isf_queue2_pre_stop);

ISF_RV _isf_queue2_stop(PISF_DATA_QUEUE p_queue)
{
	if((!p_queue)) {
		return ISF_ERR_NO_CONFIG;
	}
	if ((p_queue->en == 0)) {
		DBG_ERR("\"%s\".queue[%d] stop --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] stop --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	//NOTE: to allow stop after pre_stop!
	//if ((p_queue->en != 2) || (p_queue->data == NULL)) {
	//	return ISF_ERR_NOT_START;
	//}
	DBG_IND("\"%s\".queue[%d] stop --- begin!\r\n", p_queue->unit_name, p_queue->id);
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	if((p_queue->max == 0) && (p_queue->data == NULL)) {
		SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
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
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	DBG_IND("\"%s\".queue[%d] stop --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
EXPORT_SYMBOL(_isf_queue2_stop);

ISF_RV _isf_queue2_force_stop(PISF_DATA_QUEUE p_queue)
{
	if((!p_queue)) {
		return ISF_ERR_NO_CONFIG;
	}
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] force_stop --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	
	if(p_queue->en == 3) {
		//this time is fast start/stop! do complete stop
		
		DBG_IND("\"%s\".queue[%d] ext_stop - begin\r\n", p_queue->unit_name, p_queue->id);
		SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
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
		//wait for keep sem count not change
		if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_queue->p_sem_q)[0], vos_util_msec_to_tick(0))) {
			DBG_IND("\"%s\".queue[%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", p_queue->unit_name, p_queue->id);
		}
	 	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem_q)[0]); // force unlock
		SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
		DBG_IND("\"%s\".queue[%d] ext_stop --- end!\r\n", p_queue->unit_name, p_queue->id);
		return ISF_OK;
	}
	
	DBG_IND("\"%s\".queue[%d] force_stop - begin\r\n", p_queue->unit_name, p_queue->id);
	//if (p_queue->data == NULL) {
	if (1) {
	SEM_WAIT(((SEM_HANDLE*)p_queue->p_sem)[0]);
	//wait for keep sem count not change
	if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_queue->p_sem_q)[0], vos_util_msec_to_tick(0))) {
		DBG_IND("\"%s\".queue[%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", p_queue->unit_name, p_queue->id);
	}
 	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem_q)[0]); // force unlock
	SEM_SIGNAL(((SEM_HANDLE*)p_queue->p_sem)[0]);
	} 
	DBG_IND("\"%s\".queue[%d] force_stop - end\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
EXPORT_SYMBOL(_isf_queue2_force_stop);

ISF_RV _isf_queue2_close(PISF_DATA_QUEUE p_queue)
{
	void* p_sem;
	void* p_sem_q;
	if(!p_queue) {
		return ISF_ERR_NO_CONFIG;
	}
	if ((p_queue->en == 0)) {
		DBG_ERR("\"%s\".queue[%d] close --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if(!(p_queue->p_sem) || !(p_queue->p_sem_q)) {
		DBG_ERR("\"%s\".queue[%d] close --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}
	if ((p_queue->en != 1) || (p_queue->data != NULL)) {
		DBG_ERR("\"%s\".queue[%d] close --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_ALREADY_START;
	}
	/*
	if(p_queue->data != NULL) {
		_isf_queue2_stop(p_queue);
	}
	*/
	p_sem = p_queue->p_sem;
	p_sem_q = p_queue->p_sem_q;
	DBG_IND("\"%s\".queue[%d] close --- begin!\r\n", p_queue->unit_name, p_queue->id);
	SEM_WAIT(((SEM_HANDLE*)p_sem)[0]);
	if((p_queue->max != 0) || (p_queue->data != NULL)) {
		SEM_SIGNAL(((SEM_HANDLE*)p_sem)[0]);
		DBG_ERR("\"%s\".queue[%d] close --- invalid state-2!\r\n", p_queue->unit_name, p_queue->id);
		return ISF_ERR_ALREADY_START;
	}
	p_queue->en = 0; //open->close
	p_queue->p_sem = NULL;
	p_queue->p_sem_q = NULL;
	//wait for keep sem count not change
	if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_sem_q)[0], vos_util_msec_to_tick(0))) {
		DBG_IND("\"%s\".queue[%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", p_queue->unit_name, p_queue->id);
	}
	SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); //force unlock
	SEM_SIGNAL(((SEM_HANDLE*)p_sem)[0]);
	DBG_IND("\"%s\".queue[%d] close --- end!\r\n", p_queue->unit_name, p_queue->id);
	return ISF_OK;
}
EXPORT_SYMBOL(_isf_queue2_close);

