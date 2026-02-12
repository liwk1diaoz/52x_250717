#include "isf_vdoprc_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_pl
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_pl_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_pl_debug_level, isf_vdoprc_pl_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_pl_debug_level, "vdoprc_pl debug level");

#if (ISF_NEW_PULL == ENABLE)
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)
#endif

///////////////////////////////////////////////////////////////////////////////

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

static ISF_RV _isf_queue_pre_start(PISF_PULL_QUEUE p_queue)
{
	unsigned long flags;
	if((!p_queue)) {
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
	DBG_IND("\"%s\".queue[%d] pre start --- begin!\r\n", p_queue->unit_name, p_queue->id);
	loc_cpu(flags);
	p_queue->en = 3; //pre start
	unl_cpu(flags);
	DBG_IND("\"%s\".queue[%d] pre start --- end!\r\n", p_queue->unit_name, p_queue->id);
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

static BOOL _isf_queue_is_empty(PISF_PULL_QUEUE p_queue)
{
	unsigned long flags;
	BOOL is_empty = FALSE;
	if((!p_queue))
		return FALSE;
	if(!(p_queue->p_sem_q))
		return FALSE;
	loc_cpu(flags);
	is_empty = ((p_queue->head == p_queue->tail) && (p_queue->is_full == FALSE));
	unl_cpu(flags);
	DBG_IND("\"%s\".queue[%d] is empty %d\r\n", p_queue->unit_name, p_queue->id, is_empty);
	return is_empty;
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
	DBG_IND("\"%s\".queue[%d] is full %d\r\n", p_queue->unit_name, p_queue->id, is_full);
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
	DBG_IND("\"%s\".queue[%d] put data --- begin\r\n", p_queue->unit_name, p_queue->id);
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
			DBG_IND("\"%s\".queue[%d] put data --- set full=1 !!!\r\n", p_queue->unit_name, p_queue->id);
			p_queue->is_full = TRUE;
		}
		unl_cpu(flags);
		SEM_SIGNAL(((SEM_HANDLE*)p_sem_q)[0]); // unlock
		DBG_IND("\"%s\".queue[%d] put data %d ---  end\r\n", p_queue->unit_name, p_queue->id, p_queue->cnt);
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
	DBG_IND("\"%s\".queue[%d] get data (%d) ms -- begin\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	if (wait_ms < 0) {
try_wait_1:
		// blocking (wait until data available) , if success Q - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(((SEM_HANDLE*)p_sem_q)[0])) {
			return ISF_ERR_FAILED;
		}
		DBG_IND("\"%s\".queue[%d] get data --- (%d) ms ready!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	} else  {
try_wait_2:
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success Q - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_sem_q)[0], vos_util_msec_to_tick(wait_ms))) {
			DBG_IND("\"%s\".queue[%d] get data --- (%d) ms timeout!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
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
		DBG_IND("\"%s\".queue[%d] get data --- (%d) ms ready!\r\n", p_queue->unit_name, p_queue->id, wait_ms);
	}
try_get:
	if(p_queue->en == 0) {
		// enter before close, and force quit by close!
		DBG_IND("\"%s\".queue[%d] get data --- not open %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
		return ISF_ERR_NOT_OPEN_YET;
	}

	if(!(p_sem_q)) {
		DBG_IND("\"%s\".queue[%d] get data --- invalid state %d!\r\n", p_queue->unit_name, p_queue->id, p_queue->en);
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
		DBG_IND("\"%s\".queue[%d] get data --- is empty !!!\r\n", p_queue->unit_name, p_queue->id);   // this case should be enter if force unlock
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

		if (p_queue->head == p_queue->tail) { // Check data full
			DBG_IND("\"%s\".queue[%d] get data --- set full=0 !!!\r\n", p_queue->unit_name, p_queue->id);
			p_queue->is_full = FALSE;
		}
		DBG_IND("\"%s\".queue[%d] get data %d --- end\r\n", p_queue->unit_name, p_queue->id, p_queue->cnt);   // this case should be enter if force unlock
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

#if 0
typedef struct _VDOENC_MMAP_MEM_INFO {
	UINT32          addr_virtual;           ///< memory start addr (virtual)
	UINT32          addr_physical;          ///< memory start addr (physical)
	UINT32          size;                   ///< size
} VDOENC_MMAP_MEM_INFO, *PVDOENC_MMAP_MEM_INFO;

#define VDOENC_VIRT2PHY(pathID, virt_addr) gISF_VdoEncMem_MmapInfo[pathID].addr_physical + (virt_addr - gISF_VdoEncMem_MmapInfo[pathID].addr_virtual)
static VDOENC_MMAP_MEM_INFO   gISF_VdoEncMem_MmapInfo[ISF_VDOENC_OUT_NUM]                       = {0};

_ISF_VdoEnc_StreamReadyCB()
		// convert to physical addr
		pVStrmBuf->resv[11]  = VDOENC_VIRT2PHY(pMem->PathID, pMem->Addr);          // bs  phy
		pVStrmBuf->resv[12]  = VDOENC_VIRT2PHY(pMem->PathID, pVStrmBuf->resv[0]);  // sps phy
		pVStrmBuf->resv[13]  = VDOENC_VIRT2PHY(pMem->PathID, pVStrmBuf->resv[2]);  // pps phy
		pVStrmBuf->resv[14]  = VDOENC_VIRT2PHY(pMem->PathID, pVStrmBuf->resv[4]);  // vps phy
_ISF_VdoEnc_AllocMem()
			// set mmap info
			gISF_VdoEncMem_MmapInfo[id].addr_virtual  = uiBufAddr;
			gISF_VdoEncMem_MmapInfo[id].addr_physical = nvtmpp_vb_blk2pa(gISF_VdoEncMaxMemBlk->h_data);
			gISF_VdoEncMem_MmapInfo[id].size          = gISF_VdoEncMaxMemSize[id];
		// set mmap info
		gISF_VdoEncMem_MmapInfo[id].addr_virtual  = uiBufAddr;
		gISF_VdoEncMem_MmapInfo[id].addr_physical = nvtmpp_vb_blk2pa(gISF_VdoEncMemBlk->h_data);
		gISF_VdoEncMem_MmapInfo[id].size          = uiBufSize;
ISF_VdoEnc_GetPortParam()
	case VDOENC_PARAM_BUFINFO_PHYADDR:
		value = gISF_VdoEncMem_MmapInfo[nPort - ISF_OUT_BASE].addr_physical;
		break;

	case VDOENC_PARAM_BUFINFO_SIZE:
		value = gISF_VdoEncMem_MmapInfo[nPort - ISF_OUT_BASE].size;
		break;


#endif


#if (USE_PULL == ENABLE)

//extern UINT32 _vdoprc_oport_releasedata(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 j);
void _isf_vdoprc_oqueue_do_open(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
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
		&(p_ctx->ISF_VDOPRC_OUTQ_SEM_ID[i]));
#else
	_isf_queue2_open(&(p_ctx->pullq.output[i]),
		&(p_ctx->ISF_VDOPRC_OUT_SEM_ID[i]),
		&(p_ctx->ISF_VDOPRC_OUTQ_SEM_ID[i]));
#endif
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue open - end\r\n", p_thisunit->unit_name, i);
	}
#endif
}

static void _isf_vdoprc_oqueue_clear(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
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

void _isf_vdoprc_oqueue_do_start(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
	PISF_PULL_QUEUE p_queue;

#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue start - begin\r\n", p_thisunit->unit_name, i);
	}
#endif
	p_ctx->pullq.num[i] = p_vdoinfo->buffercount;
	if(p_ctx->pullq.num[i] > ISF_VDOPRC_OUTQ_MAX) {
		p_ctx->pullq.num[i] = ISF_VDOPRC_OUTQ_MAX;
	}
	if (p_ctx->combine_mode) {
		p_ctx->pullq.num[i] = 1;
	}
	p_ctx->start_mask |= (1<<i);
	p_queue = &p_ctx->pullq.output[i];
	//start->start and queue number changed, need to clear queue
	if (p_queue->en == 2 && p_queue->max != p_ctx->pullq.num[i]) {
		//DBG_ERR("clear queue\r\n");
		_isf_vdoprc_oqueue_clear(p_thisunit, oport);
	}
#if (ISF_NEW_PULL == ENABLE)
	_isf_queue_start(&(p_ctx->pullq.output[i]),
		p_ctx->pullq.num[i], (p_ctx->pullq.data[i]));
#else
	_isf_queue2_start(&(p_ctx->pullq.output[i]),
		p_ctx->pullq.num[i], (p_ctx->pullq.data[i]));
#endif

#if 0 //(USE_OUT_EXT == ENABLE)
	///////////////////////////////////////////
	// physical out - dispatch to related extend out
	if(i < ISF_VDOPRC_PHY_OUT_NUM) {
		UINT32 ext_pid;
		for (ext_pid = ISF_VDOPRC_PHY_OUT_NUM; ext_pid < ISF_VDOPRC_OUT_NUM; ext_pid++) {
			ISF_PORT_PATH* p_path = p_thisunit->port_path + ext_pid;
 			if ((p_path->oport == oport) && (p_path->flag == 1)) {
				_isf_vdoprc_oqueue_do_start(p_thisunit, ISF_OUT(ext_pid));
			}
		}
	}
#endif
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue start - end\r\n", p_thisunit->unit_name, i);
	}
#endif
}

void _isf_vdoprc_oqueue_pre_start(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
#if (ISF_NEW_PULL == ENABLE)
	_isf_queue_pre_start(&(p_ctx->pullq.output[i]));
#else
	_isf_queue2_pre_start(&(p_ctx->pullq.output[i]));
#endif
}

void _isf_vdoprc_oqueue_force_stop(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
#if (ISF_NEW_PULL == ENABLE)
	_isf_queue_force_stop(&(p_ctx->pullq.output[i]));
#else
	_isf_queue2_force_stop(&(p_ctx->pullq.output[i]));
#endif
}

void _isf_vdoprc_oqueue_do_stop(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	UINT32 i = oport - ISF_OUT_BASE;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue stop - begin\r\n", p_thisunit->unit_name, i);
	}
#endif
	_isf_vdoprc_oqueue_clear(p_thisunit, oport);
	p_ctx->pullq.num[i] = 0; //clear
	p_ctx->start_mask &= ~(1<<i);

#if 0 //(USE_OUT_EXT == ENABLE)
	///////////////////////////////////////////
	// physical out - dispatch to related extend out
	if(i < ISF_VDOPRC_PHY_OUT_NUM) {
		UINT32 ext_pid;
		for (ext_pid = ISF_VDOPRC_PHY_OUT_NUM; ext_pid < ISF_VDOPRC_OUT_NUM; ext_pid++) {
			ISF_PORT_PATH* p_path = p_thisunit->port_path + ext_pid;
			//if ((p_path->oport == oport) && (p_path->flag == 1)) {
			if (p_path->oport == oport) {
				_isf_vdoprc_oqueue_do_stop(p_thisunit, ISF_OUT(ext_pid));
			}
		}
	}
#endif
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue stop - end\r\n", p_thisunit->unit_name, i);
	}
#endif
}

void _isf_vdoprc_oqueue_do_close(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue close - begin\r\n", p_thisunit->unit_name, i);
	}
#endif
	if(1) {
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

ISF_RV _isf_vdoprc_oqueue_do_push(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	ISF_RV ret = FALSE;
	if(!p_data)
		return ISF_ERR_NULL_POINTER;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue push -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_data->h_data);
	}
#endif
#if (ISF_NEW_PULL == ENABLE)
	ret = _isf_queue_put_data(&(p_ctx->pullq.output[i]), p_data);
#else
	ret = _isf_queue2_put_data(&(p_ctx->pullq.output[i]), p_data);
#endif

	if (ret == ISF_OK) {
		if (p_ctx->phy_mask & (1<<i)) { //physical path
			//XDATA
			if (p_ctx->poll_mask & (1<<i)) { // this path is inside user poll mask
				//DBG_DUMP("%s.out[%d]! enqueue %08x (%08x %08x)\r\n",
				//	p_thisunit->unit_name, i,
				//	(1<<i), p_ctx->result_mask, ((p_ctx->start_mask & p_ctx->phy_mask) & p_ctx->poll_mask));
				p_ctx->result_mask |= (1<<i); //this physical path is ready
				if ((p_ctx->result_mask & p_ctx->phy_mask) == ((p_ctx->start_mask & p_ctx->phy_mask) & p_ctx->poll_mask)) {  //is all physcial path ready?
					SEM_SIGNAL(((SEM_HANDLE*)p_ctx->p_sem_poll)[0]); //unlock poll
				}
			}
		} else {
			if (p_ctx->poll_mask & (1<<i)) { // this path is inside user poll mask
				SEM_SIGNAL(((SEM_HANDLE*)p_ctx->p_sem_poll)[0]); //unlock poll
			}
		}
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

ISF_RV _isf_vdoprc_oqueue_do_pull(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
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
		if (p_ctx->combine_mode) {
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_USER_PROBE_PULL);
		}
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

ISF_RV _isf_vdoprc_oqueue_wait_for_push_in(ISF_UNIT *p_thisunit, UINT32 oport, INT32 wait_ms)
{
#if (USE_PULL == ENABLE)
#if (USE_DROP_AFTER_PROC != ENABLE)
	ISF_RV r = ISF_OK;
	UINT32 i =  oport - ISF_OUT_BASE;
	//drop before proc
#if (ISF_NEW_PULL == ENABLE)
	if (TRUE == _isf_queue_is_full(&(p_ctx->pullq.output[i]))) {
		return ISF_ERR_QUEUE_FULL;
	}
#else
	if (TRUE == _isf_queue2_is_full(&(p_ctx->pullq.output[i]))) {
		return ISF_ERR_QUEUE_FULL;
	}
#endif
#endif
#endif
	return ISF_OK;
}

ISF_RV _isf_vdoprc_oqueue_do_push_with_clean(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 keep_this)
{
	ISF_RV rr = ISF_ERR_QUEUE_EMPTY;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
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
			//_vdoprc_oport_releasedata(p_thisunit, oport, j);
		}
*/
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
				//_vdoprc_oport_releasedata(p_thisunit, oport, j);
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
				//_vdoprc_oport_releasedata(p_thisunit, oport, j);
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
		rr = _isf_vdoprc_oqueue_do_push(p_thisunit, oport, p_data);
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

ISF_RV _isf_vdoprc_oqueue_do_push_wait(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, UINT32 wait_ms)
{
	ISF_RV rr = ISF_ERR_QUEUE_EMPTY;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i =  oport - ISF_OUT_BASE;
	UINT32 retry_cnt = 0;
	UINT32 timer_resolution = 20;

	if (p_ctx->pullq.num[i]) {
		//drop after proc
#if (ISF_NEW_PULL == ENABLE)
		while (_isf_queue_is_full(&(p_ctx->pullq.output[i]))) {
#else
		while (_isf_queue2_is_full(&(p_ctx->pullq.output[i]))) {
#endif
			if (wait_ms == 0) {
				return ISF_ERR_QUEUE_FULL;
			}
			retry_cnt++;
			//retry until queue is not full
			vos_util_delay_ms(timer_resolution);
			if (retry_cnt > wait_ms/timer_resolution) {
				DBG_WRN("%s.out[%d]! wait for oqueue timeout(%d)! Droped.\r\n", p_thisunit->unit_name, i, wait_ms);
				return ISF_ERR_QUEUE_FULL;
			}
		}
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Out Queue add -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_data->h_data);
	}
#endif
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);

		rr = _isf_vdoprc_oqueue_do_push(p_thisunit, oport, p_data);
		if(rr != ISF_OK) {
#if (FORCE_DUMP_DATA == ENABLE)
			if (oport == WATCH_PORT) {
				DBG_DUMP("%s.out[%d]! Out Queue release -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, i, p_data->h_data);
			}
#endif
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, rr);
			DBG_WRN("%s.out[%d]! push to oqueue fail, r=%d, drop.\r\n", p_thisunit->unit_name, i, rr);
		}
	}
	return rr;
}

#endif

ISF_RV _isf_vdoprc_oqueue_do_poll_list(ISF_UNIT *p_thisunit, VDOPRC_POLL_LIST *p_poll_list)
{
	UINT32 dev;
	UINT32 path_id;
	UINT32 dev_mask;
	UINT32 start_cnt;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}

	p_thisunit->p_base->do_trace(DEV_UNIT(0), ISF_CTRL, ISF_OP_PARAM_CTRL, "[POLL_FRM] Set POLL_LIST, path_mask(0x%08x), timeout(%d)", p_poll_list->path_mask, p_poll_list->wait_ms);

	// [1] reset event mask & reset semaphore
	dev_mask = 0;
	for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
		VDOPRC_CONTEXT* p_ctx2 = (VDOPRC_CONTEXT*)(DEV_UNIT(dev)->refdata);
		p_ctx2->poll_mask = p_poll_list->path_mask[dev];
		p_ctx2->result_mask = 0;
	}
	while (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_ctx->p_sem_poll)[0], vos_util_msec_to_tick(0)) == 0) ;  // consume all semaphore count

	// [2] check if any path already has data available, if so, return NOW.
	for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
		VDOPRC_CONTEXT* p_ctx2 = (VDOPRC_CONTEXT*)(DEV_UNIT(dev)->refdata);
		for (path_id=0; path_id<ISF_VDOPRC_OUT_NUM; path_id++) {
			if (p_poll_list->path_mask[dev] & (1<<path_id)) {
				// have to check this path (by user given check mask)
#if (ISF_NEW_PULL == ENABLE)
				p_ctx2->result_mask |= (_isf_queue_is_empty(&(p_ctx2->pullq.output[path_id])) ? (0):(1<<path_id));
#else
				p_ctx2->result_mask |= (_isf_queue2_is_empty(&(p_ctx2->pullq.output[path_id])) ? (0):(1<<path_id));
#endif
			}
		}
		if (p_ctx2->result_mask > 0) {
			dev_mask |= (1<<dev);
		}
	}
	if (dev_mask > 0) { // any check path has data available in PullQ
		p_thisunit->p_base->do_trace(DEV_UNIT(0), ISF_CTRL, ISF_OP_PARAM_CTRL, "[POLL_FRM] already has data available");
		return ISF_OK;
	}

	// [3] if all path are stopped, just return
	start_cnt = 0;
	for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
		for (path_id=0; path_id<ISF_VDOPRC_OUT_NUM; path_id++) {
			if (DEV_UNIT(dev)->port_outstate[path_id]->state == ISF_PORT_STATE_RUN) start_cnt++;
		}
	}
	if (start_cnt == 0) {
		p_thisunit->p_base->do_trace(DEV_UNIT(0), ISF_CTRL, ISF_OP_PARAM_CTRL, "[POLL_FRM] all path stopped, ignore poll_list !!");
		return ISF_ERR_IGNORE;
	}

	// [4] NO data available => have to semaphore wait for data
	if (p_poll_list->wait_ms< 0) {
		// blocking (wait until data available)
		if (SEM_WAIT_INTERRUPTIBLE(((SEM_HANDLE*)p_ctx->p_sem_poll)[0])) {
			p_thisunit->p_base->do_trace(DEV_UNIT(0), ISF_CTRL, ISF_OP_PARAM_CTRL, "[POLL_FRM] wait data ignore !!");
			return ISF_ERR_IGNORE;
		}
	} else  {
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0)
		if (SEM_WAIT_TIMEOUT(((SEM_HANDLE*)p_ctx->p_sem_poll)[0], vos_util_msec_to_tick(p_poll_list->wait_ms))) {
			p_thisunit->p_base->do_trace(DEV_UNIT(0), ISF_CTRL, ISF_OP_PARAM_CTRL, "[POLL_FRM] wait data timeout(%d).....", p_poll_list->wait_ms);
			return ISF_ERR_WAIT_TIMEOUT;
		}
	}

	// [5] wait data success(or auto-unlock) => update event mask again
	dev_mask = 0;
	for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
		VDOPRC_CONTEXT* p_ctx2 = (VDOPRC_CONTEXT*)(DEV_UNIT(dev)->refdata);
		for (path_id=0; path_id<ISF_VDOPRC_OUT_NUM; path_id++) {
			if (p_poll_list->path_mask[dev] & (1<<path_id)) {
				// have to check this path (by user given check mask)
#if (ISF_NEW_PULL == ENABLE)
				p_ctx2->result_mask |= (_isf_queue_is_empty(&(p_ctx2->pullq.output[path_id])) ? (0):(1<<path_id));
#else
				p_ctx2->result_mask |= (_isf_queue2_is_empty(&(p_ctx2->pullq.output[path_id])) ? (0):(1<<path_id));
#endif
			}
		}
		if (p_ctx2->result_mask > 0) {
			dev_mask |= (1<<dev);
		}
	}

	if (dev_mask > 0) { // any check path has data available in PullQ
		p_thisunit->p_base->do_trace(DEV_UNIT(0), ISF_CTRL, ISF_OP_PARAM_CTRL, "[POLL_FRM] wait data success!!");
		return ISF_OK;
	} else {
		p_thisunit->p_base->do_trace(DEV_UNIT(0), ISF_CTRL, ISF_OP_PARAM_CTRL, "[POLL_FRM] auto-unlock poll_list !!");
		return ISF_ERR_IGNORE;
	}
}

ISF_RV _isf_vdoprc_oqueue_get_poll_mask(ISF_UNIT *p_thisunit, VDOPRC_POLL_LIST *p_poll_list)
{
	UINT32 dev;
	for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
		VDOPRC_CONTEXT* p_ctx2 = (VDOPRC_CONTEXT*)(DEV_UNIT(dev)->refdata);
		p_poll_list->path_mask[dev] = p_ctx2->result_mask;
	}
	p_poll_list->wait_ms = 0;

	return ISF_OK;
}

void _isf_vdoprc_oqueue_cancel_poll(ISF_UNIT *p_thisunit)
{
	UINT32 dev;
	UINT32 path_id;
	UINT32 start_cnt;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	start_cnt = 0;
	for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
		for (path_id=0; path_id<ISF_VDOPRC_OUT_NUM; path_id++) {
			if (DEV_UNIT(dev)->port_outstate[path_id]->state == ISF_PORT_STATE_RUN) start_cnt++;
		}
	}
	if (start_cnt == 0) {
		SEM_SIGNAL(((SEM_HANDLE*)p_ctx->p_sem_poll)[0]); // let poll_list(blocking_mode) auto-unlock, if all path are stopped
	}
}

