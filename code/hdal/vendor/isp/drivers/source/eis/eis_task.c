#include "eis_int.h"
///////////////////////////////////////////////////////////////////////////////


static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)
///////////////////////////////////////////////////////////////////////////////
#define EIS_TSK_PRI       3
#define EIS_TSK_STKSIZE   4096


#define FLG_ID_EIS_ALL            0xFFFFFFFF
#define FLG_ID_EIS_DATA_IN        FLGPTN_BIT(0)
#define FLG_ID_EIS_DATA_OUT       FLGPTN_BIT(1)
#define FLG_ID_EIS_DATA_OUT2      FLGPTN_BIT(2)
#define FLG_ID_EIS_DATA_OUT_STOP  FLGPTN_BIT(3)
#define FLG_ID_EIS_DATA_OUT2_STOP FLGPTN_BIT(4)
#define FLG_ID_EIS_GYRO_LATENCY   FLGPTN_BIT(5)
#define FLG_ID_EIS_DEBUG          FLGPTN_BIT(29)
#define FLG_ID_EIS_STOP           FLGPTN_BIT(30)
#define FLG_ID_EIS_IDLE           FLGPTN_BIT(31)


#define EIS_MSG_STS_FREE    0x00000000
#define EIS_MSG_STS_LOCK    0x00000001

#define EIS_MSG_Q_DEPTH   (MAX_LATENCY_NUM + 2)


typedef struct vos_list_head EIS_LIST_HEAD;
typedef struct _EIS_CONTEXT {
	VDOPRC_EIS_PROC_INFO proc_info;
	EIS_LIST_HEAD list;
	UINT32 rev[2];
} EIS_CONTEXT;

typedef struct _EIS_OUT {
	UINT64 count;					///< frame count
	UINT32 lut2d_buf[MAX_EIS_LUT2D_SIZE];
	UINT32 lut2d_size;
	UINT32 frame_time;
	EIS_LIST_HEAD list;
	UINT32 rev[2];
} EIS_OUT;


static EIS_LIST_HEAD free_msg_list_head;
static EIS_LIST_HEAD proc_msg_list_head;
static EIS_CONTEXT eis_msg_queue[EIS_MSG_Q_DEPTH];

#define EIS_OUT_STS_FREE    0x00000000
#define EIS_OUT_STS_LOCK    0x00000001

#define EIS_OUT_MAX_PATH 2
#define EIS_OUT_Q_DEPTH   (MAX_LATENCY_NUM + 2)
typedef struct _EIS_OUT_Q_INFO {
	EIS_LIST_HEAD free_head;
	EIS_LIST_HEAD used_head;
	EIS_OUT out[EIS_OUT_Q_DEPTH];
} EIS_OUT_Q_INFO;
static EIS_OUT_Q_INFO eis_out_q[EIS_OUT_MAX_PATH] = {0};

static EIS_PATH_INFO path_info[EIS_OUT_MAX_PATH] = {0};

static UINT32 gyro_dbg_size = 0;//gyro data appended to 2dlut data


static UINT32 eis_dbg_id = 0;
static UINT32 eis_dbg_value = 0;

static UINT32 eis_gyro_latency = 0;

ID FLG_ID_EIS = 0;
#if 0 //TASK_IN_KERNEL
THREAD_HANDLE EIS_TSK_ID = 0;
#endif
#if 0
static void DumpMem(ULONG Addr, UINT32 Size, UINT32 Alignment)
{
	UINT32 i;
	UINT8 *pBuf = (UINT8 *)Addr;
	for (i = 0; i < Size; i++) {
		if (i > 0 && i % Alignment == 0) {
			DBG_DUMP("\r\n");
		}
		DBG_DUMP("0x%02X ", *(pBuf + i));
	}
	DBG_DUMP("\r\n");
}
#endif

static UINT32 g_eis_open = 0;

static void _eis_msg_reset(EIS_CONTEXT *p_event)
{
	if (p_event) {
		p_event->rev[0] = EIS_MSG_STS_FREE;
	}
}
static EIS_CONTEXT *_eis_msg_get_used(void)
{
	unsigned long loc_cpu_flg;
	EIS_CONTEXT *p_event = NULL;

	DBG_IND("++\r\n");
	loc_cpu(loc_cpu_flg);
	if (!vos_list_empty(&proc_msg_list_head)) {
		p_event = vos_list_entry(proc_msg_list_head.next, EIS_CONTEXT, list);
		DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->proc_info.frame_count, p_event->rev[0], p_event->rev[1]);
		vos_list_del(&p_event->list);
	}
	unl_cpu(loc_cpu_flg);
	DBG_IND("--\r\n");
	return p_event;
}
static void _eis_msg_release_used(EIS_CONTEXT *p_event)
{
	unsigned long loc_cpu_flg;

	DBG_IND("++\r\n");
	DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->proc_info.frame_count, p_event->rev[0], p_event->rev[1]);
	if ((p_event->rev[0] & EIS_MSG_STS_LOCK) != EIS_MSG_STS_LOCK) {
		DBG_ERR("event status error\r\n");
		DBG_DUMP("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->proc_info.frame_count, p_event->rev[0], p_event->rev[1]);
	}

	_eis_msg_reset(p_event);
	loc_cpu(loc_cpu_flg);
	vos_list_add_tail(&p_event->list, &free_msg_list_head);
	unl_cpu(loc_cpu_flg);

	DBG_IND("--\r\n");
}
static EIS_CONTEXT *_eis_msg_get_free(void)
{
	unsigned long loc_cpu_flg;
	EIS_CONTEXT *p_event = NULL;

	DBG_IND("++\r\n");
	loc_cpu(loc_cpu_flg);
	if (!vos_list_empty(&free_msg_list_head)) {
		p_event = vos_list_entry(free_msg_list_head.next, EIS_CONTEXT, list);
		vos_list_del(&p_event->list);

		p_event->rev[0] |= EIS_MSG_STS_LOCK;
		DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->proc_info.frame_count, p_event->rev[0], p_event->rev[1]);

		unl_cpu(loc_cpu_flg);
		DBG_IND("--\r\n");
		return p_event;
	}
	unl_cpu(loc_cpu_flg);
	//force drop the oldest one
	p_event = _eis_msg_get_used();
	if (p_event) {
		DBG_IND("DROP msg%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->proc_info.frame_count, p_event->rev[0], p_event->rev[1]);
		_eis_msg_reset(p_event);
	} else {
		DBG_ERR("No free/used msg Q!?\r\n");
	}

	DBG_IND("--\r\n");
	return p_event;
}

#if 0//TASK_IN_KERNEL
static void _eis_msg_release_all_used(void)
{
	EIS_CONTEXT *p_event;
	unsigned long loc_cpu_flg;

	DBG_IND("++\r\n");
	loc_cpu(loc_cpu_flg);
	while (!vos_list_empty(&proc_msg_list_head)) {
		p_event = vos_list_entry(proc_msg_list_head.next, EIS_CONTEXT, list);
		DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->count, p_event->rev[0], p_event->rev[1]);
		vos_list_del(&p_event->list);
		_eis_msg_release_used(p_event);
	}
	unl_cpu(loc_cpu_flg);
	DBG_IND("--\r\n");
}
#endif
static void _eis_msg_reset_queue(void)
{
	UINT32 i;
	EIS_CONTEXT *free_event;

	/* init free & proc list head */
	VOS_INIT_LIST_HEAD(&free_msg_list_head);
	VOS_INIT_LIST_HEAD(&proc_msg_list_head);
	DBG_IND("===== EIS MSG Q INIT =====\r\n");
	DBG_MSG("%10s%12s%12s%12s\r\n", "info_addr", "count", "rev[0]", "rev[1]");
	for (i = 0; i < EIS_MSG_Q_DEPTH; i++) {
		free_event = &eis_msg_queue[i];
		_eis_msg_reset(free_event);
		free_event->rev[1] = i;
		DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)free_event, free_event->proc_info.frame_count, free_event->rev[0], free_event->rev[1]);
		vos_list_add_tail(&free_event->list, &free_msg_list_head);
	}
}

void _eis_msg_dump(void)
{
	unsigned long loc_cpu_flg;
	EIS_LIST_HEAD *p_list;
	EIS_CONTEXT *p_info;

	DBG_IND("");
	if (g_eis_open == 0) {
		DBG_ERR("vendor_eis NOT opened!\r\n");
	}
	loc_cpu(loc_cpu_flg);
	DBG_DUMP("\r\n---- USED LIST ----\r\n");
	DBG_DUMP("%10s%12s%12s%12s\r\n", "info_addr", "count", "rev[0]", "rev[1]");
	vos_list_for_each(p_list, &proc_msg_list_head) {
		p_info = vos_list_entry(p_list, EIS_CONTEXT, list);
		DBG_DUMP("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_info, p_info->proc_info.frame_count, p_info->rev[0], p_info->rev[1]);
	}

	DBG_DUMP("\r\n---- FREE LIST ----\r\n");
	DBG_DUMP("%10s%12s%12s%12s\r\n", "info_addr", "count", "rev[0]", "rev[1]");
	vos_list_for_each(p_list, &free_msg_list_head) {
		p_info = vos_list_entry(p_list, EIS_CONTEXT, list);
		DBG_DUMP("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_info, p_info->proc_info.frame_count, p_info->rev[0], p_info->rev[1]);
	}
	unl_cpu(loc_cpu_flg);
	DBG_DUMP("\r\n");
}

static void _eis_out_reset(EIS_OUT *p_event)
{
	if (p_event) {
		p_event->rev[0] = EIS_OUT_STS_FREE;
	}
}
static EIS_OUT *_eis_out_get_used_by_frm_cnt(UINT32 id, UINT64 count)
{
	unsigned long loc_cpu_flg;
	EIS_OUT *p_event = NULL;
	EIS_LIST_HEAD *p_list;

	DBG_IND("[%d]count=%llu\r\n", id, count);
	loc_cpu(loc_cpu_flg);
	if (!vos_list_empty(&eis_out_q[id].used_head)) {
		vos_list_for_each(p_list, &eis_out_q[id].used_head) {
			p_event = vos_list_entry(p_list, EIS_OUT, list);
			DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->count, p_event->rev[0], p_event->rev[1]);
			if (p_event->count == count) {
				vos_list_del(&p_event->list);
				break;
			} else {
				p_event = NULL;
			}
		}
	}
	unl_cpu(loc_cpu_flg);
	return p_event;
}
static EIS_OUT *_eis_out_get_used(UINT32 id)
{
	unsigned long loc_cpu_flg;
	EIS_OUT *p_event = NULL;

	loc_cpu(loc_cpu_flg);
	if (!vos_list_empty(&eis_out_q[id].used_head)) {
		p_event = vos_list_entry(eis_out_q[id].used_head.next, EIS_OUT, list);
		//DBG_DUMP("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->count, p_event->rev[0], p_event->rev[1]);
		vos_list_del(&p_event->list);
	}
	unl_cpu(loc_cpu_flg);
	return p_event;
}
static void _eis_out_release_used(UINT32 id, EIS_OUT *p_event)
{
	unsigned long loc_cpu_flg;

	DBG_IND("[%d]++\r\n", id);
	DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->count, p_event->rev[0], p_event->rev[1]);
	if ((p_event->rev[0] & EIS_MSG_STS_LOCK) != EIS_MSG_STS_LOCK) {
		DBG_ERR("event status error\r\n");
		DBG_DUMP("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->count, p_event->rev[0], p_event->rev[1]);
	}

	_eis_out_reset(p_event);
	loc_cpu(loc_cpu_flg);
	vos_list_add_tail(&p_event->list, &eis_out_q[id].free_head);
	unl_cpu(loc_cpu_flg);

	DBG_IND("--\r\n");
}
static EIS_OUT *_eis_out_get_free(UINT32 id)
{
	unsigned long loc_cpu_flg;
	EIS_OUT *p_event = NULL;

	DBG_IND("[%d]++\r\n", id);
	loc_cpu(loc_cpu_flg);
	if (!vos_list_empty(&eis_out_q[id].free_head)) {
		p_event = vos_list_entry(eis_out_q[id].free_head.next, EIS_OUT, list);
		vos_list_del(&p_event->list);

		p_event->rev[0] |= EIS_OUT_STS_LOCK;
		DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->count, p_event->rev[0], p_event->rev[1]);

		unl_cpu(loc_cpu_flg);
		DBG_IND("[%d]--\r\n", id);
		return p_event;
	}
	unl_cpu(loc_cpu_flg);
	//force drop the oldest one
	p_event = _eis_out_get_used(id);
	if (p_event) {
		DBG_IND("DROP out[%d]%#.8x%12llu%12d%12d\r\n", id, (unsigned int)p_event, p_event->count, p_event->rev[0], p_event->rev[1]);
		_eis_out_reset(p_event);
		p_event->rev[0] |= EIS_OUT_STS_LOCK;
	} else {
		DBG_ERR("No free/used Q!?\r\n");
	}

	DBG_IND("[%d]--\r\n", id);
	return p_event;
}
#if 0
static void _eis_out_release_all_used(void)
{
	EIS_OUT *p_event;
	unsigned long loc_cpu_flg;
	UINT32 id;

	DBG_IND("++\r\n");
	loc_cpu(loc_cpu_flg);
	for (id = 0; id < EIS_OUT_MAX_PATH; id++) {
		while (!vos_list_empty(&eis_out_q[id].used_head)) {
			p_event = vos_list_entry(eis_out_q[id].used_head.next, EIS_OUT, list);
			DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_event, p_event->count, p_event->rev[0], p_event->rev[1]);
			vos_list_del(&p_event->list);
			_eis_out_release_used(id, p_event);
		}
	}
	unl_cpu(loc_cpu_flg);
	DBG_IND("--\r\n");
}
#endif
static void _eis_out_reset_queue(void)
{
	UINT32 i, id;
	EIS_OUT *free_event;

	for (id = 0; id < EIS_OUT_MAX_PATH; id++) {
		DBG_IND("===== EIS OUT[%d] Q INIT =====\r\n", id);
		/* init free & proc list head */
		VOS_INIT_LIST_HEAD(&eis_out_q[id].free_head);
		VOS_INIT_LIST_HEAD(&eis_out_q[id].used_head);
		DBG_MSG("%10s%12s%12s%12s\r\n", "info_addr", "count", "rev[0]", "rev[1]");
		for (i = 0; i < EIS_OUT_Q_DEPTH; i++) {
			free_event = &eis_out_q[id].out[i];
			_eis_out_reset(free_event);
			free_event->rev[1] = i;
			DBG_MSG("%#.8x%12llu%12d%12d\r\n", (unsigned int)free_event, free_event->count, free_event->rev[0], free_event->rev[1]);
			vos_list_add_tail(&free_event->list, &eis_out_q[id].free_head);
		}
	}
}

void _eis_out_dump(void)
{
	unsigned long loc_cpu_flg;
	EIS_LIST_HEAD *p_list;
	EIS_OUT *p_info;
	UINT32 id;

	DBG_IND("");
	if (g_eis_open == 0) {
		DBG_ERR("vendor_eis NOT opened!\r\n");
	}
	loc_cpu(loc_cpu_flg);
	for (id = 0; id < EIS_OUT_MAX_PATH; id++) {
		DBG_DUMP("\r\n---- USED LIST[%d] ----\r\n", id);
		DBG_DUMP("%10s%12s%12s%12s\r\n", "info_addr", "count", "rev[0]", "rev[1]");
		vos_list_for_each(p_list, &eis_out_q[id].used_head) {
			p_info = vos_list_entry(p_list, EIS_OUT, list);
			DBG_DUMP("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_info, p_info->count, p_info->rev[0], p_info->rev[1]);
		}

		DBG_DUMP("\r\n---- FREE LIST[%d] ----\r\n", id);
		DBG_DUMP("%10s%12s%12s%12s\r\n", "info_addr", "count", "rev[0]", "rev[1]");
		vos_list_for_each(p_list, &eis_out_q[id].free_head) {
			p_info = vos_list_entry(p_list, EIS_OUT, list);
			DBG_DUMP("%#.8x%12llu%12d%12d\r\n", (unsigned int)p_info, p_info->count, p_info->rev[0], p_info->rev[1]);
		}
	}
	unl_cpu(loc_cpu_flg);
	DBG_DUMP("\r\n");
}

ER _eis_tsk_open(void)
{
	DBG_IND("++\r\n");
	if (g_eis_open) {
		g_eis_open ++;
		return E_OK;
	}
	OS_CONFIG_FLAG(FLG_ID_EIS);
	_eis_msg_reset_queue();
	_eis_out_reset_queue();

    vos_flag_clr(FLG_ID_EIS, FLG_ID_EIS_ALL); //clear all flag
    #if 0//TASK_IN_KERNEL
    THREAD_CREATE(EIS_TSK_ID, _eis_tsk, NULL, "eis_task");
	if (EIS_TSK_ID == 0) {
		DBG_ERR("task create fail!\r\n");
		return E_SYS;
	}
    THREAD_SET_PRIORITY(EIS_TSK_ID, EIS_TSK_PRI);
    THREAD_RESUME(EIS_TSK_ID);
    #endif
	g_eis_open ++;
	DBG_IND("--\r\n");
	return E_OK;
}
ER _eis_tsk_idle(void)
{
	DBG_IND("++\r\n");
	if (g_eis_open == 0) {
		DBG_ERR("already close or not open yet!\r\n");
		return E_SYS;
	}
	vos_flag_clr(FLG_ID_EIS, FLG_ID_EIS_IDLE); //clear IDLE before send cmd
	vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_STOP); //send STOP cmd
	DBG_IND("--\r\n");
	return E_OK;
}
ER _eis_tsk_close(void)
{
	#if 0//TASK_IN_KERNEL
	FLGPTN          flag = 0;
	#endif
	DBG_IND("++\r\n");
	if (g_eis_open == 0) {
		DBG_ERR("already close or not open yet!\r\n");
		return E_SYS;
	}
	if (g_eis_open > 1) {
		g_eis_open--;
		return E_OK;
	}
	#if 0//TASK_IN_KERNEL
	vos_flag_clr(FLG_ID_EIS, FLG_ID_EIS_IDLE); //clear IDLE before send cmd
	vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_STOP); //send STOP cmd
	if (vos_flag_wait(&flag, FLG_ID_EIS, FLG_ID_EIS_IDLE, TWF_ORW) != E_OK) { //wait IDLE (stopped)
		DBG_WRN("ctrl-c break, wai_flg() is forced quit!\r\n");
	}
	//THREAD_DESTROY(EIS_TSK_ID);  //already call THREAD_RETURN() in task, do not call THREAD_DESTROY() here.
	EIS_TSK_ID = 0; //clear task id
	#endif
	g_eis_open--;
	vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_DATA_OUT_STOP);
	vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_DATA_OUT2_STOP);
	rel_flg(FLG_ID_EIS);
	memset((void *)path_info, 0, sizeof(path_info));
	DBG_IND("--\r\n");
	return E_OK;
}
ER _eis_tsk_get_output(VDOPRC_EIS_2DLUT *p_param)
{
	ER ret = E_OK;
	FLGPTN flag = 0;
	FLGPTN wai_flag, stop_flag;
	EIS_OUT *p_info;
	UINT32 id = p_param->path_id;

	if (g_eis_open == 0) {
		return E_SYS;
	}

	if (id == 0) {
		stop_flag = FLG_ID_EIS_DATA_OUT_STOP;
		wai_flag = FLG_ID_EIS_DATA_OUT | stop_flag;
	} else {
		stop_flag = FLG_ID_EIS_DATA_OUT2_STOP;
		wai_flag = FLG_ID_EIS_DATA_OUT2 | stop_flag;
	}

	p_info = _eis_out_get_used_by_frm_cnt(id, p_param->frame_count);
	vos_flag_clr(FLG_ID_EIS, wai_flag);
	if (NULL == p_info) {
		ER wai_rt;

		wai_rt = vos_flag_wait_timeout(&flag, FLG_ID_EIS, wai_flag, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(p_param->wait_ms));
		if (wai_rt != E_OK) {
			if (wai_rt == E_TMOUT) {
				return E_TMOUT;
			} else {
				DBG_ERR("flag wait err(%d)\r\n", wai_rt);
				return E_QOVR;
			}
		}
		if (flag == 0) {
			if (p_param->wait_ms == 0) {
				return E_QOVR;
			} else {
				return E_TMOUT;
			}
		}
		if (flag & stop_flag) {
			return E_RLWAI;
		}
		p_info = _eis_out_get_used_by_frm_cnt(id, p_param->frame_count);
		if (NULL == p_info) {
			DBG_WRN("[%d]No data for count=%llu\r\n", id, p_param->frame_count);
			return E_QOVR;
		}
	}

	//prepare data here
	//DBG_IND("[%d]%#.8x%12llu%12d%12d\r\n", id, (unsigned int)p_info, p_info->count, p_info->rev[0], p_info->rev[1]);
	DBG_IND("[%d] frm(%llu) size(%d)\r\n", id,  p_info->count, p_info->lut2d_size);

	if (p_info->lut2d_size > p_param->buf_size) {
		DBG_ERR("[%d]buf_size(%d) < lut2d(%d)\r\n", id, p_param->buf_size, p_info->lut2d_size);
		return E_NOMEM;
	}
	memcpy((void *)p_param->buf_addr, (void *)p_info->lut2d_buf, p_info->lut2d_size);
	_eis_out_release_used(id, p_info);
	p_param->buf_size = p_info->lut2d_size;
	p_param->frame_time = p_info->frame_time;
	return ret;
}

ER _eis_tsk_trigger_proc(VDOPRC_EIS_PROC_INFO *p_proc_info, BOOL in_isr)
{
	unsigned long loc_cpu_flg;
	EIS_CONTEXT *p_event;

	if (g_eis_open == 0) {
		DBG_ERR("vendor_eis NOT opened!\r\n");
		return E_SYS;
	}
	if (p_proc_info == NULL) {
		return E_PAR;
	}

	p_event = _eis_msg_get_free();

	if (p_event == NULL) {
		return E_QOVR;
	}

	memcpy((void *)&p_event->proc_info, (void *)p_proc_info, sizeof(p_event->proc_info));

	/* add to proc list & trig proc task */
	loc_cpu(loc_cpu_flg);
	vos_list_add_tail(&p_event->list, &proc_msg_list_head);
	unl_cpu(loc_cpu_flg);
	// trigger task to process data
	if (in_isr) {
		vos_flag_iset(FLG_ID_EIS, FLG_ID_EIS_DATA_IN);
	} else {
		vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_DATA_IN);
	}
	return E_OK;
}

ER _eis_tsk_gyro_latency(UINT32 gyro_latency)
{
	eis_gyro_latency = gyro_latency;
	vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_GYRO_LATENCY);
	return E_OK;
}

void _eis_tsk_get_msg(EIS_WAIT_PROC_INFO *p_user)
{
	FLGPTN flag = 0;
	FLGPTN wai_flag;
	EIS_CONTEXT *p_event;

	p_user->result = EIS_E_OK;

	if (g_eis_open == 0) {
		p_user->result = EIS_E_STATE;
		return;
	}

	wai_flag = FLG_ID_EIS_DATA_IN | FLG_ID_EIS_STOP | FLG_ID_EIS_DEBUG | FLG_ID_EIS_GYRO_LATENCY;
	p_event = _eis_msg_get_used();
	vos_flag_clr(FLG_ID_EIS, wai_flag);
	if (NULL == p_event) {
		ER wai_rt;

		wai_rt = vos_flag_wait_timeout(&flag, FLG_ID_EIS, wai_flag, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(p_user->wait_ms));
		if (wai_rt != E_OK) {
			if (wai_rt == E_TMOUT) {
				p_user->result = EIS_E_TIMEOUT;
			} else {
				DBG_ERR("flag wait err(%d)\r\n", wai_rt);
				p_user->result = EIS_E_FAIL;
			}
			return;
		}
		if (flag == 0) {
			if (p_user->wait_ms == 0) {
				p_user->result = EIS_E_FAIL;
				return;
			} else {
				p_user->result = EIS_E_TIMEOUT;
				return;
			}
		}
		if (flag & FLG_ID_EIS_STOP) {
			p_user->result = EIS_E_ABORT;
			return;
		}
		if (flag & FLG_ID_EIS_DEBUG) {
			p_user->result = EIS_E_DEBUG;
			p_user->angular_rate_x[0] = eis_dbg_id;
			p_user->angular_rate_x[1] = eis_dbg_value;
			return;
		}
		if (flag & FLG_ID_EIS_GYRO_LATENCY) {
			p_user->result = EIS_E_GYRO_LATENCY;
			p_user->angular_rate_x[0] = eis_gyro_latency;
			return;
		}
		p_event = _eis_msg_get_used();
		if (NULL == p_event) {
			DBG_WRN("No msg\r\n");
			p_user->result = EIS_E_FAIL;
			return;
		}
	}
	memcpy((void *)p_user, (void *)p_event, sizeof(p_event->proc_info));
	//DBG_MSG("==== get_msg[%llu](%d) ====\r\n", p_user->frame_count, sizeof(EIS_WAIT_PROC_INFO));
	//DumpMem((UINT32)p_user, sizeof(EIS_WAIT_PROC_INFO), 16);
	_eis_msg_release_used(p_event);
}
void _eis_tsk_put_out(EIS_PUSH_DATA *p_data, OS_COPY_CB copy)
{
	unsigned long loc_cpu_flg;
	UINT32 id;
	EIS_OUT *p_event = NULL;

	DBG_IND("\r\n");
	if (g_eis_open == 0) {
		return;
	}
	if (p_data->path_id >= EIS_OUT_MAX_PATH) {
		DBG_ERR("path id(%d) error\r\n", p_data->path_id);
		return;
	} else {
		id = p_data->path_id;
	}

	p_event = _eis_out_get_free(id);
	if (NULL == p_event) {
		return;
	}
	if (p_data->buf_size > sizeof(p_event->lut2d_buf)) {
		DBG_ERR("lut2d_buf(%d) > (%d) overflow!\r\n", p_data->buf_size, sizeof(p_event->lut2d_buf));
		return;
	}
	if (copy) {
		copy(p_event->lut2d_buf, (void *)p_data->lut2d_buf, p_data->buf_size);
	} else {
		memcpy((void *)p_event->lut2d_buf, (void *)p_data->lut2d_buf, sizeof(p_event->lut2d_buf));
	}

	p_event->lut2d_size = p_data->buf_size;
	p_event->count = p_data->frame_count;
	p_event->frame_time = p_data->frame_time;
	//DBG_MSG("==== PUT DATA[%llu](%d) ====\r\n", p_event->count, p_event->lut2d_size);
	//DumpMem((UINT32)p_event->lut2d_buf, p_event->lut2d_size, 16);

	loc_cpu(loc_cpu_flg);
	vos_list_add_tail(&p_event->list, &eis_out_q[id].used_head);
	unl_cpu(loc_cpu_flg);
	if (id == 0) {
		vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_DATA_OUT);
	} else if (id == 1) {
		vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_DATA_OUT2);
	}
}
void _eis_tsk_set_path_info(EIS_PATH_INFO *p_data)
{
	UINT32 id;

	DBG_IND("\r\n");
	if (g_eis_open == 0) {
		return;
	}
	if (p_data->path_id >= EIS_OUT_MAX_PATH) {
		DBG_ERR("path id(%d) error\r\n", p_data->path_id);
		return;
	} else {
		id = p_data->path_id;
	}

	path_info[id].path_id = id;
	path_info[id].lut2d_buf_size = p_data->lut2d_buf_size;
	path_info[id].lut2d_size_sel = p_data->lut2d_size_sel;
	path_info[id].frame_latency = p_data->frame_latency;
}
ER _eis_tsk_get_path_info(VDOPRC_EIS_PATH_INFO *p_data)
{
	UINT32 id;

	DBG_IND("\r\n");
	if (g_eis_open == 0) {
		return E_SYS;
	}
	if (p_data->path_id >= EIS_OUT_MAX_PATH) {
		DBG_ERR("path id(%d) error\r\n", p_data->path_id);
		return E_PAR;
	} else {
		id = p_data->path_id;
	}
	if (path_info[id].lut2d_buf_size) {
		p_data->lut2d_buf_size = path_info[id].lut2d_buf_size + gyro_dbg_size;
	} else {
		p_data->lut2d_buf_size = 0;
	}
	p_data->lut2d_size_sel = path_info[id].lut2d_size_sel;
	p_data->frame_latency = path_info[id].frame_latency;
	return E_OK;
}
void _eis_tsk_set_debug_size(UINT32 size)
{
	DBG_IND("\r\n");

	gyro_dbg_size = size;
}
void _eis_tsk_set_dbg_cmd(UINT32 param_id, UINT32 value)
{
	DBG_IND("\r\n");
	if (g_eis_open == 0) {
		return;
	}
	eis_dbg_id = param_id;
	eis_dbg_value = value;
	vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_DEBUG); //send STOP cmd
}
#if 0//TASK_IN_KERNEL
static void _eis_tsk_proc_data(EIS_CONTEXT *p_data)
{
	unsigned long loc_cpu_flg;
	UINT32 id;
	EIS_OUT *p_event[EIS_OUT_MAX_PATH] = {NULL};

	DBG_IND("\r\n");

	for (id = 0; id < EIS_OUT_MAX_PATH; id++) {
		p_event[id] = _eis_out_get_free(id);
	}

	//EIS lib
	//...
	p_event[0]->count = p_data->count;
	p_event[1]->count = p_data->count;

	loc_cpu(loc_cpu_flg);
	for (id = 0; id < EIS_OUT_MAX_PATH; id++) {
		if (p_event[id] == NULL) {
			continue;
		}
		//to do
		if (1) {//if eis result OK, add to used list
			vos_list_add_tail(&p_event[id]->list, &eis_out_q[id].used_head);
		} else {//put back to free list
			vos_list_add_tail(&p_event[id]->list, &eis_out_q[id].free_head);
		}
	}
	unl_cpu(loc_cpu_flg);
	//to do
	if (1) {
		vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_DATA_OUT);
	}
	if (1) {
		vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_DATA_OUT2);
	}
}

THREAD_DECLARE(_eis_tsk, arglist)
{
	FLGPTN flag = 0;
	BOOL is_continue = 1;
	EIS_CONTEXT *p_event;

	THREAD_ENTRY();
	while (is_continue) {
		PROFILE_TASK_IDLE();
		vos_flag_wait(&flag, FLG_ID_EIS, FLG_ID_EIS_DATA_IN | FLG_ID_EIS_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		DBG_IND("flag=0x%x\r\n", flag);
		if (flag & FLG_ID_EIS_STOP) {
			_eis_msg_release_all_used();
			is_continue = 0;
			continue;
		}
		if (flag & FLG_ID_EIS_DATA_IN) {
			while (1) {
				p_event = _eis_msg_get_used();
				if (p_event == NULL) {
					break;
				}
				//process EIS lib
				_eis_tsk_proc_data(p_event);
				//copy event first !?
				_eis_msg_release_used(p_event);
			}

		}
	}
	vos_flag_set(FLG_ID_EIS, FLG_ID_EIS_IDLE);

	THREAD_RETURN(0);
}
#endif
