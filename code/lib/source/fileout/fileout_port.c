/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "fileout_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static UINT64 g_max_pop_size = FILEOUT_DEFAULT_MAX_POP_SIZE;
static UINT64 g_max_file_size = FILEOUT_QUEUE_MAX_FILE_SIZE;
static CHAR g_cur_drive_info[FILEOUT_QUEUE_NUM] = {
		FILEOUT_DRV_NAME_FIRST,
		FILEOUT_DRV_NAME_FIRST,
		FILEOUT_DRV_NAME_FIRST,
		FILEOUT_DRV_NAME_FIRST};

static FILEOUT_QUEUE_INFO g_queue_info[FILEOUT_QUEUE_NUM] = {0};
static UINT64 g_file_need_size[FILEOUT_QUEUE_NUM] = {0};

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
static FILEOUT_QUEUE_INFO* fileout_get_qinfo(INT32 queue_idx)
{
	if (queue_idx < 0 || queue_idx >= FILEOUT_QUEUE_NUM) {
		DBG_ERR("Invalid queue_idx %d\r\n", queue_idx);
		return NULL;
	}

	return &g_queue_info[queue_idx];
}

static INT32 fileout_queue_clean(INT32 queue_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	UINT32 num;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		DBG_ERR("get queue NULL\r\n");
		return -1;
	}

	num = QUE_NUM(p_qinfo);
	if (num == 0) {
		DBG_DUMP("num is 0. return.\r\n");
		return 0;
	}

	// Pop all to notify the previous module to release buffers
	#if 0
	memset(p_qinfo, 0, sizeof(FILEOUT_QUEUE_INFO));
	#else
	{
		ISF_FILEOUT_BUF cur_buf = {0};
		CHAR fpath[FILEOUT_PATH_LEN];
		INT32 port_idx;
		INT32 ret;
		while (QUE_NUM(p_qinfo)) {
			//loop until queue is empty
			memset(&cur_buf, 0, sizeof(cur_buf));
			memset(fpath, 0, sizeof(fpath));
			cur_buf.p_fpath = fpath;
			cur_buf.fpath_size = sizeof(fpath);
			fileout_queue_pop(&cur_buf, queue_idx);
			if (cur_buf.fp_pushed) {
				DBG_IND("fp_pushed 0x%X before\r\n", cur_buf.fp_pushed);
				cur_buf.fp_pushed(&cur_buf);
				DBG_IND("fp_pushed 0x%X after\r\n", cur_buf.fp_pushed);
			}
		}
		if (p_qinfo->is_active && p_qinfo->need_to_rel && IS_QUE_EMPTY(p_qinfo)) {
			port_idx = fileout_queue_get_port_idx(queue_idx);
			if (port_idx < 0) {
				DBG_ERR("get port idx failed, queue_idx %d\r\n", port_idx);
				return -1;
			}
			ret = fileout_port_rel_queue(port_idx);
			if (ret < 0) {
				DBG_ERR("rel_queue failed, iPort %d\r\n", FILEOUT_IPORT(port_idx));
				return -1;
			}
			p_qinfo->is_active = 0;
			p_qinfo->need_to_rel = 0;
			FILEOUT_DUMP("Port %d clean success.\r\n", FILEOUT_IPORT(port_idx));
		}
	}
	#endif

	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
//return queue idx
INT32 fileout_port_get_queue(INT32 port_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	INT32 cur_qidx;
	INT32 free_qidx = -1;
	INT32 ret_qidx = -1;

	fileout_qhandle_lock();

	for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {
		p_qinfo = &g_queue_info[cur_qidx];

		if (p_qinfo->is_active) {
			if (p_qinfo->port_idx == port_idx) {
				//found an active port which already exists
				ret_qidx = cur_qidx;
				goto label_fileout_get_queue_idx_end;
			}
		} else {
			//not active
			if (-1 == free_qidx) {
				//update free_qidx
				free_qidx = cur_qidx;
			}
		}
	}

	if (free_qidx == -1) {
		//not found
		DBG_ERR("No free queue for port_idx %d\r\n", port_idx);
	} else {
		//use free queue
		ret_qidx = free_qidx;
		p_qinfo = &g_queue_info[ret_qidx];
		memset(p_qinfo, 0, sizeof(FILEOUT_QUEUE_INFO));
		p_qinfo->is_active = 1;
		p_qinfo->port_idx = port_idx;

		FILEOUT_DUMP("port %d get free queue %d success.\r\n", FILEOUT_IPORT(port_idx), ret_qidx);
	}

label_fileout_get_queue_idx_end:
	fileout_qhandle_unlock();
	return ret_qidx;
}

INT32 fileout_port_cre_queue(INT32 port_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	INT32 cur_qidx;
	INT32 ret_qidx = -1;

	fileout_qhandle_lock();

	for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {
		p_qinfo = &g_queue_info[cur_qidx];

		if (p_qinfo->is_active && p_qinfo->port_idx == port_idx) {
			if (p_qinfo->need_to_rel && IS_QUE_EMPTY(p_qinfo)) {
				ret_qidx = cur_qidx;
				memset(p_qinfo, 0, sizeof(FILEOUT_QUEUE_INFO));
				p_qinfo->is_active = 1;
				p_qinfo->port_idx = port_idx;

				FILEOUT_DUMP("port %d use existed queue %d success.\r\n", port_idx+ISF_IN_BASE, ret_qidx);
				goto label_fileout_get_queue_idx_end;
			}
		}

		if (!p_qinfo->is_active) {
			ret_qidx = cur_qidx;
			p_qinfo = &g_queue_info[ret_qidx];
			memset(p_qinfo, 0, sizeof(FILEOUT_QUEUE_INFO));
			p_qinfo->is_active = 1;
			p_qinfo->port_idx = port_idx;

			FILEOUT_DUMP("port %d create free queue %d success.\r\n", FILEOUT_IPORT(port_idx), ret_qidx);
			goto label_fileout_get_queue_idx_end;
		}
		//fix push in first
		if ((p_qinfo->is_active) && (p_qinfo->port_idx == port_idx)) {
			ret_qidx = cur_qidx;
			DBG_DUMP("port %d found queue %d which already exists\r\n", FILEOUT_IPORT(port_idx), ret_qidx);
			goto label_fileout_get_queue_idx_end;
		}
	}

	if (ret_qidx == -1) {
		//not found
		DBG_ERR("No free queue for port_idx %d\r\n", port_idx);
	}

label_fileout_get_queue_idx_end:
	fileout_qhandle_unlock();
	return ret_qidx;
}


INT32 fileout_port_rel_queue(INT32 port_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	INT32 cur_qidx;

	fileout_qhandle_lock();

	for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {
		p_qinfo = &g_queue_info[cur_qidx];
		if (p_qinfo->is_active && p_qinfo->port_idx == port_idx) {
			if (!IS_QUE_EMPTY(p_qinfo)) {
				DBG_WRN("cur_qidx %d is not empty, port_idx %d\r\n", cur_qidx, port_idx);
			}
			p_qinfo->is_active = 0;

			FILEOUT_DUMP("port %d release queue %d success.\r\n", FILEOUT_IPORT(port_idx), cur_qidx);

			break;
		}
	}

	fileout_qhandle_unlock();

	return 0;
}

INT32 fileout_port_is_queue_empty(void)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	INT32 cur_qidx;
	INT32 ret = 0;

	//fileout_qhandle_lock();

	for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {
		p_qinfo = &g_queue_info[cur_qidx];
		if (!IS_QUE_EMPTY(p_qinfo)) {
			if (p_qinfo->is_active && !p_qinfo->need_to_rel) {
				DBG_DUMP("[FILEOUT] Q[%d] active.\r\n", cur_qidx);
				continue;
			}
			DBG_DUMP("[FILEOUT] Q[%d] not empty. Previous rec buf might wrong.\r\n", cur_qidx);
			if (0 != fileout_queue_clean(cur_qidx)) {
				DBG_ERR("Q[%d] clean failed\r\n", cur_qidx);
				ret = -1;
			} else {
				DBG_DUMP("^G [FILEOUT] Q[%d] clean.\r\n", cur_qidx);
			}
		}
	}

	//fileout_qhandle_unlock();

	return ret;
}

//random pick idx
INT32 fileout_queue_pick_idx(CHAR drive, INT32 qidx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	ISF_FILEOUT_BUF *p_buf;
	UINT32 puf_head;
	INT32 ret_qidx = -1;
	INT32 cur_qidx;
	INT32 cur_index;

	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", drive);
		return -1;
	}

	for (cur_index = 1; cur_index <= FILEOUT_QUEUE_NUM; cur_index++) {
		cur_qidx = qidx + cur_index;
		if (cur_qidx >= FILEOUT_QUEUE_NUM) {
			cur_qidx -= FILEOUT_QUEUE_NUM;
		}

		p_qinfo = &g_queue_info[cur_qidx];
		puf_head = p_qinfo->queue_head;
		p_buf = &p_qinfo->op_buf[puf_head];

		if (p_qinfo->is_active && !IS_QUE_EMPTY(p_qinfo) && (p_buf != NULL)) {
			if ((p_buf->p_fpath != NULL) && (p_buf->p_fpath[0] == drive)) {
				ret_qidx = cur_qidx;
				DBG_IND("Drive %c pick Q[%d] NUM[%d]\r\n", drive, ret_qidx, QUE_NUM(p_qinfo));
				return ret_qidx;
			}
		}
	}

	return ret_qidx;
}

INT32 fileout_port_get_active(CHAR drive, UINT32 *port_num)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	INT32 cur_qidx;
	UINT32 port_active = 0;

	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", drive);
		return -1;
	}

	for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {

		p_qinfo = &g_queue_info[cur_qidx];

		if (p_qinfo->is_active) {
			if ((p_qinfo->p_path != NULL) && (p_qinfo->p_path[0] == drive)) {
				port_active++;
			}
		}
	}

	DBG_IND("Port active %d\r\n", port_active);

	*port_num = (UINT32)port_active;

	return 0;
}

INT32 fileout_port_start(INT32 port_idx)
{
	INT32 ret_qidx;
	UINT32 addr = 0;

	ret_qidx = fileout_port_cre_queue(port_idx);
	if (ret_qidx < 0) {
		DBG_ERR("get_queue failed, iPort %d\r\n", FILEOUT_IPORT(port_idx));
		return -1;
	}

	if (fileout_util_is_usememblk()) {
		addr = fileout_util_get_blk_addr();
		if (addr == 0) {
			if ((-1) == fileout_util_cre_mem_blk(FILEOUT_DEFAULT_MAX_POP_SIZE)) {
				DBG_WRN("cre_mem_blk failed, iPort %d\r\n", FILEOUT_IPORT(port_idx));
				return -1;
			}
		}
		fileout_tsk_sysinfo_init(g_cur_drive_info[ret_qidx]);
	}

	if (fileout_util_is_append()) {
		addr = fileout_util_get_blk_addr();
		if (addr == 0) {
			if ((-1) == fileout_util_cre_mem_blk(FILEOUT_DEFAULT_MAX_POP_SIZE)) {
				DBG_WRN("cre_mem_blk failed, iPort %d\r\n", FILEOUT_IPORT(port_idx));
				return -1;
			}
		}
		addr = fileout_util_get_blk_addr();
		if (addr) { // error handling
			memset((void *)addr, 0x0, FILEOUT_DEFAULT_MAX_POP_SIZE);
		}
	}

	return 0;
}

INT32 fileout_port_close(INT32 port_idx)
{
	INT32 ret_qidx;
	FILEOUT_QUEUE_INFO *p_qinfo;
	CHAR drive;

	ret_qidx = fileout_port_get_queue(port_idx);
	if (ret_qidx < 0) {
		DBG_ERR("get_queue failed, iPort %d\r\n", FILEOUT_IPORT(port_idx));
		return -1;
	}

	p_qinfo = fileout_get_qinfo(ret_qidx);
	if (NULL == p_qinfo) {
		DBG_ERR("get queue NULL\r\n");
		return -1;
	}
	if (0 == p_qinfo->is_active) {
		DBG_ERR("queue is already close %d\r\n", ret_qidx);
		return -1;
	}

	fileout_qhandle_lock();

	p_qinfo->need_to_rel = 1;
	drive = g_cur_drive_info[ret_qidx];

	fileout_qhandle_unlock();

	// trigger tsk
	if (0 != fileout_tsk_trigger_ops(drive)) {
		DBG_ERR("trigger ops failed\r\n");
		return -1;
	}

	// trigger relque
	if (0 != fileout_tsk_trigger_relque(drive)) {
		DBG_ERR("trigger ops failed\r\n");
		return -1;
	}

	return 0;
}

INT32 fileout_port_need_to_rel(CHAR drive)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	INT32 cur_qidx;
	INT32 port_idx;
	INT32 ret = -1;

	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", drive);
		return -1;
	}

	for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {

		p_qinfo = &g_queue_info[cur_qidx];

		if (p_qinfo->is_active && p_qinfo->need_to_rel && IS_QUE_EMPTY(p_qinfo) && (drive == g_cur_drive_info[cur_qidx])) {

			port_idx = fileout_queue_get_port_idx(cur_qidx);
			if (port_idx < 0) {
				DBG_ERR("get port idx failed, queue_idx %d\r\n", cur_qidx);
				return -1;
			}

			ret = fileout_port_rel_queue(port_idx);
			if (ret < 0) {
				DBG_ERR("rel_queue failed, iPort %d\r\n", FILEOUT_IPORT(port_idx));
				return -1;
			}

			p_qinfo->is_active = 0;
			p_qinfo->need_to_rel = 0;

			FILEOUT_DUMP("Port %d release success.\r\n", FILEOUT_IPORT(port_idx));

		}
		else if (p_qinfo->is_active == 0) {
			ret = 0;
		}
	}

	if (fileout_util_is_usememblk()) {
		UINT32 rel_flag = 0;
		for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {
			p_qinfo = &g_queue_info[cur_qidx];
			if (!p_qinfo->is_active) {
				rel_flag = 1;
			} else {
				rel_flag = 0;
			}
		}
		if (rel_flag && fileout_util_get_blk_addr()) { // error handling
			fileout_util_rel_mem_blk();
			fileout_tsk_sysinfo_uninit(drive);
		}
	}

	if (fileout_util_is_append()) {
		UINT32 rel_flag = 0;
		for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {
			p_qinfo = &g_queue_info[cur_qidx];
			if (!p_qinfo->is_active) {
				rel_flag = 1;
			} else {
				rel_flag = 0;
			}
		}
		if (rel_flag && fileout_util_get_blk_addr()) { // error handling
			fileout_util_rel_mem_blk();
		}
	}

	return ret;
}

INT32 fileout_queue_get_remain_size(CHAR drive, UINT32 *p_size)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	ISF_FILEOUT_BUF *p_buf;
	UINT32 puf_head;
	INT32 cur_qidx;
	UINT64 remain_size = 0;

	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", drive);
		return -1;
	}

    for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {

		p_qinfo = &g_queue_info[cur_qidx];
		puf_head = p_qinfo->queue_head;
		p_buf = &p_qinfo->op_buf[puf_head];

#if 0
		if (p_qinfo->is_active && p_buf->p_fpath[0] == drive && !IS_QUE_EMPTY(p_qinfo)) {
			remain_size += (p_qinfo->size_total - p_qinfo->size_written);
		}
#else
		if (p_qinfo->is_active && (p_buf != NULL)) {
			if ((p_buf->p_fpath != NULL) && p_buf->p_fpath[0] == drive) {
				remain_size += (p_qinfo->size_written);
			}
		}
#endif
	}

	DBG_IND("drive %c, remain_size %llu\r\n", drive, remain_size);

	if (remain_size > 0xFFFFFFFF) {
		DBG_ERR("remain_size > UINT32 value, drive %c\r\n", drive);
		return -1;
	}

	*p_size = (UINT32)remain_size;

	return 0;
}

INT32 fileout_queue_push(const ISF_FILEOUT_BUF *p_in_buf, INT32 queue_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	ISF_FILEOUT_BUF *p_new_buf;
	UINT32 new_head;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		DBG_ERR("get queue NULL\r\n");
		return -1;
	}
	if (0 == p_qinfo->is_active) {
		DBG_ERR("queue is not active %d\r\n", queue_idx);
		return -1;
	}

	//queue lock
	fileout_qhandle_lock();

	//calc new head idx
	new_head = p_qinfo->queue_head + 1;
	if (new_head == FILEOUT_OP_BUF_NUM) {
		new_head = 0;
	}

	//check the queue is full
	if (new_head == p_qinfo->queue_tail) {
		DBG_ERR("port_idx %d queue full\r\n", p_qinfo->port_idx);
		//queue unlock
		fileout_qhandle_unlock();
		return -1;
	}

	//copy data to head buf
	p_new_buf = &p_qinfo->op_buf[new_head];
	memcpy(p_new_buf, p_in_buf, sizeof(ISF_FILEOUT_BUF));

	if (p_new_buf->fileop & ISF_FILEOUT_FOP_CREATE) {
		if (p_in_buf->p_fpath) {
			//the path is specified, copy to the queue
			if (0 != fileout_cb_store_path(p_new_buf, p_in_buf->p_fpath)) {
				DBG_ERR("store_path failed, port_idx %d\r\n", p_qinfo->port_idx);
				//queue unlock
				fileout_qhandle_unlock();
				goto label_naming_error;
			}
		} else {
			//get a new path by callback
			if (0 != fileout_cb_call_naming(queue_idx, p_new_buf)) {
				DBG_ERR("call_naming failed, port_idx %d\r\n", p_qinfo->port_idx);
				//queue unlock
				fileout_qhandle_unlock();
				goto label_naming_error;
			}
		}

		if (p_new_buf->fileop != ISF_FILEOUT_FOP_SNAPSHOT) {
			p_qinfo->p_path = p_new_buf->p_fpath;
			p_qinfo->path_size = p_new_buf->fpath_size;
			g_cur_drive_info[queue_idx] = p_qinfo->p_path[0];
		}
	}
	else {
		p_new_buf->p_fpath = p_qinfo->p_path;
		p_new_buf->fpath_size = p_qinfo->path_size;

		//specific usage
		if (p_new_buf->event == FILEOUT_FEVENT_BSINCARD) {
			if (fileout_util_is_usememblk()) {
				p_new_buf->p_fpath = &g_cur_drive_info[queue_idx];
				p_new_buf->fpath_size = FILEOUT_PATH_LEN;
			} else {
				DBG_WRN("FILEOUT_PARAM_READ_STRGBLK not set\r\n");
			}
		}

		if (p_new_buf->p_fpath == NULL) {
			DBG_ERR("get naming failed, port_idx %d\r\n", p_qinfo->port_idx);
			//queue unlock
			fileout_qhandle_unlock();
			goto label_naming_error;
		}
	}

	//showinfo
	fileout_util_show_bufinfo(p_new_buf);

	//update p_qinfo in the end
	if (p_new_buf->fileop != ISF_FILEOUT_FOP_SNAPSHOT) {
		//only handle the non-SNAPSHOT case
		if (p_new_buf->fileop & (ISF_FILEOUT_FOP_CREATE)) {
			p_qinfo->size_path = 0;
		}
		//#NT#2020/09/22#Willy Su -begin
		//#NT# Since file_size is not added, write_size of SEEK_WRITE is not calculated
		#if 0
		if (p_new_buf->fileop & (ISF_FILEOUT_FOP_CONT_WRITE | ISF_FILEOUT_FOP_SEEK_WRITE)) {
		#else
		if (p_new_buf->fileop & (ISF_FILEOUT_FOP_CONT_WRITE)) {
		#endif
			p_qinfo->size_total += p_new_buf->size;
			p_qinfo->size_path += p_new_buf->size;
		}
		//#NT#2020/09/22#Willy Su -end

		//specific usage
		if (fileout_util_is_usememblk()) {
			if (p_new_buf->event == FILEOUT_FEVENT_BSINCARD) {
				p_qinfo->size_total = 0;
				p_qinfo->size_path = 0;
			}
			if (p_new_buf->fileop & (ISF_FILEOUT_FOP_READ)) {
				p_qinfo->size_total += p_new_buf->size;
				p_qinfo->size_path += p_new_buf->size;
			}
		}
	}

	if (p_new_buf->fileop == ISF_FILEOUT_FOP_DISCARD) {
		DBG_IND("^G [FOUT][%d] handle discard\r\n", queue_idx);
		if (NULL == p_new_buf->p_fpath) {
			DBG_DUMP("^G [FOUT][%d] cancel discard\r\n", queue_idx);
			if (p_new_buf->fp_pushed) {
				DBG_IND("fp_pushed 0x%X before\r\n", p_new_buf->fp_pushed);
				p_new_buf->fp_pushed(p_new_buf);
				DBG_IND("fp_pushed 0x%X after\r\n", p_new_buf->fp_pushed);
			}
			//queue unlock
			fileout_qhandle_unlock();
			return 0;
		}
		if (0 != fileout_queue_discard(queue_idx)) {
			DBG_ERR("Q[%d] discard failed\r\n", queue_idx);
			//queue unlock
			fileout_qhandle_unlock();
			return -1;
		}
	}

	p_qinfo->queue_head = new_head;

	//queue unlock
	fileout_qhandle_unlock();

	DBG_IND("[FOUT%d] file size %lu\r\n", queue_idx, p_qinfo->size_path);

	if (p_qinfo->size_path > g_max_file_size) {
		DBG_ERR("[FILEOUT] file size(0x%lx) > max size(0x%lx)\r\n",
			(unsigned long)p_qinfo->size_path, (unsigned long)g_max_file_size);
		if (0 != fileout_cb_call_fs_err(queue_idx, p_new_buf, FILEOUT_CB_ERRCODE_LOOPREC_FULL)) {
			DBG_ERR("looprec full callback error\r\n");
		}
	}

	// trigger tsk
	if (0 != fileout_tsk_trigger_ops(p_new_buf->p_fpath[0])) {
		DBG_ERR("trigger ops failed\r\n");
		return -1;
	}

	DBG_IND("Port[%d] Queue[%d] OP 0x%X\r\n", p_qinfo->port_idx, queue_idx, p_in_buf->fileop);

	return 0;

label_naming_error:
	// fix cb_naming fail but using path last time
	p_qinfo->p_path = NULL;
	p_qinfo->path_size = 0;
	DBG_IND("^G [FOUT][%d] naming NULL\r\n", queue_idx);
	if (p_new_buf->fp_pushed) {
		DBG_IND("fp_pushed 0x%X before\r\n", p_new_buf->fp_pushed);
		p_new_buf->fp_pushed(p_new_buf);
		DBG_IND("fp_pushed 0x%X after\r\n", p_new_buf->fp_pushed);
	}
	return -1;
}

static INT32 fileout_is_allow_merge(ISF_FILEOUT_BUF *p_org, ISF_FILEOUT_BUF *p_be_merged)
{
	UINT32 allowed_op = ISF_FILEOUT_FOP_CONT_WRITE | ISF_FILEOUT_FOP_FLUSH;

	if (p_org->fileop & (~allowed_op)) {
		return 0; //only merge for allowed_op
	}

	if (p_be_merged->sign != p_org->sign ||
	    p_be_merged->event != p_org->event ||
	    p_be_merged->fileop != p_org->fileop ||
	    p_be_merged->pos != p_org->pos ||
	    p_be_merged->type != p_org->type ||
	    p_be_merged->addr != (p_org->addr + p_org->size)) {
		return 0;
	}

	return 1;
}

INT32 fileout_queue_pop(ISF_FILEOUT_BUF *p_ret_buf, INT32 queue_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	ISF_FILEOUT_BUF *p_buf;
	UINT32 new_tail;
	CHAR *p_tmp_fpath;
	UINT32 tmp_fpath_len;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		DBG_ERR("get queue NULL\r\n");
		return -1;
	}
	if (0 == p_qinfo->is_active) {
		DBG_ERR("queue is not active %d\r\n", queue_idx);
		return -1;
	}

	if (IS_QUE_EMPTY(p_qinfo)) {
		DBG_ERR("Queue is empty %d\r\n", queue_idx);
		return -1;
	}

	//backup the original path
	p_tmp_fpath = p_ret_buf->p_fpath;
	tmp_fpath_len = p_ret_buf->fpath_size;
	if (NULL == p_tmp_fpath || tmp_fpath_len != FILEOUT_PATH_LEN) {
		DBG_ERR("wrong arg: p_fpath 0x%X, fpath_size %d\r\n", (unsigned int)p_tmp_fpath, tmp_fpath_len);
		return -1;
	}

	//queue lock
	fileout_qhandle_lock();

	new_tail = p_qinfo->queue_tail + 1;
	if (new_tail == FILEOUT_OP_BUF_NUM) {
		new_tail = 0;
	}
	p_buf = &p_qinfo->op_buf[new_tail];
	if (p_buf== NULL) {
		DBG_ERR("p_buf NULL\r\n");
		//queue unlock
		fileout_qhandle_unlock();
		return -1;
	}

	//NOTE:
	//1. only dequeue the data, do not update size_written
	//2. we must take care the PATH PART and the BUF PART separately

	//copy path
	memcpy(p_tmp_fpath, p_buf->p_fpath, tmp_fpath_len);

	//copy data
	memcpy(p_ret_buf, &p_qinfo->op_buf[new_tail], sizeof(ISF_FILEOUT_BUF));
	p_ret_buf->fpath_size = tmp_fpath_len; //restore the path info
	p_ret_buf->p_fpath = p_buf->p_fpath; //restore the path info

	p_qinfo->queue_tail = new_tail;

	//try to merge more buf
	while (!IS_QUE_EMPTY(p_qinfo) && p_ret_buf->size < g_max_pop_size && (fileout_util_is_slowcard() == 0)) {
		ISF_FILEOUT_BUF *p_new_tail;

		new_tail = p_qinfo->queue_tail + 1;
		if (new_tail == FILEOUT_OP_BUF_NUM) {
			new_tail = 0;
		}
		p_new_tail = &p_qinfo->op_buf[new_tail];

		if (!fileout_is_allow_merge(p_ret_buf, p_new_tail)) {
		    break;
		}

		//merge it
		p_ret_buf->size += p_new_tail->size;
		p_qinfo->queue_tail = new_tail;

		DBG_IND("merged Q[%d] OP 0x%X Size %llu\r\n", queue_idx, p_new_tail->fileop, p_new_tail->size);
	}

	//queue unlock
	fileout_qhandle_unlock();

	DBG_IND("Port[%d] Queue[%d] OP 0x%X Size %d\r\n", p_qinfo->port_idx, queue_idx, p_ret_buf->fileop, p_ret_buf->size);

	return 0;
}

//discard will affect the queue push or pop, should be called after locked
//this requirement is not must, mark out temporarily
INT32 fileout_queue_discard(INT32 queue_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	ISF_FILEOUT_BUF *p_cur_op;
	UINT32 new_head, new_tail;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		DBG_ERR("get qinfo failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	if (IS_QUE_EMPTY(p_qinfo)) {
		DBG_IND("Queue is empty %d\r\n", queue_idx);
		return 0;
	}

	new_head = p_qinfo->queue_head; //start from head

	while(new_head != p_qinfo->queue_tail) {
		p_cur_op = &p_qinfo->op_buf[new_head];
		if (p_cur_op->fileop & ISF_FILEOUT_FOP_CREATE) {
			//discard until create op is found
			//record new_head to skip new ops
			break;
		}

		if (new_head == 0) {
			new_head = FILEOUT_OP_BUF_NUM;
		}
		new_head--;
	}

	new_tail = p_qinfo->queue_head; //start from head

	while(new_tail != p_qinfo->queue_tail) {
		p_cur_op = &p_qinfo->op_buf[new_tail];
		DBG_IND("^G [%d] orignal op: 0x%X\r\n", new_tail, p_cur_op->fileop);
		p_cur_op->fileop = 0;
		p_cur_op->fileop |= ISF_FILEOUT_FOP_DISCARD;
		DBG_IND("^G [%d] modified op: 0x%X\r\n", new_tail, p_cur_op->fileop);

		if (new_tail == new_head) {
			break;
		}
		if (new_tail == 0) {
			new_tail = FILEOUT_OP_BUF_NUM;
		}
		new_tail--;
	}

	return 0;
}


INT32 fileout_queue_written(INT32 queue_idx, UINT64 size)
{
	FILEOUT_QUEUE_INFO *p_qinfo;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		DBG_ERR("get queue NULL\r\n");
		return -1;
	}
	if (0 == p_qinfo->is_active) {
		DBG_ERR("queue is not active %d\r\n", queue_idx);
		return -1;
	}

	if (size == 0) { // clear
		p_qinfo->size_total -= p_qinfo->size_written;
		p_qinfo->size_written = 0;
	} else {
		p_qinfo->size_written += size;
		if (p_qinfo->size_written > p_qinfo->size_total) {
			DBG_ERR("written(%llu) > total(%llu)\r\n", p_qinfo->size_written, p_qinfo->size_total);
			return -1;
		}
	}

	return 0;
}

FST_FILE fileout_queue_get_filehdl(INT32 queue_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		DBG_ERR("get queue NULL\r\n");
		return NULL;
	}
	if (0 == p_qinfo->is_active) {
		DBG_ERR("queue is not active %d\r\n", queue_idx);
		return NULL;
	}

	return p_qinfo->filehdl;
}

INT32 fileout_queue_set_filehdl(INT32 queue_idx, FST_FILE new_filehdl)
{
	FILEOUT_QUEUE_INFO *p_qinfo;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		DBG_ERR("get queue NULL\r\n");
		return -1;
	}
	if (0 == p_qinfo->is_active) {
		DBG_ERR("queue is not active %d\r\n", queue_idx);
		return -1;
	}

	p_qinfo->filehdl = new_filehdl;

	return 0;
}

INT32 fileout_queue_get_port_idx(INT32 queue_idx)
{
	FILEOUT_QUEUE_INFO *p_qinfo;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		DBG_ERR("get queue NULL\r\n");
		return -1;
	}
	if (0 == p_qinfo->is_active) {
		DBG_ERR("queue is not active %d\r\n", queue_idx);
		return -1;
	}

	return p_qinfo->port_idx;
}

INT32 fileout_queue_get_size_info(INT32 queue_idx, FILEOUT_QUEUE_SIZE_INFO *p_size_qinfo)
{
	FILEOUT_QUEUE_INFO *p_qinfo;
	UINT64 size_remain = 0;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		return -1;
	}
	if (0 == p_qinfo->is_active) {
		return -1;
	}

	if (p_qinfo->is_active) {
		size_remain = p_qinfo->size_total - p_qinfo->size_written;

		p_size_qinfo->buf_remain = QUE_NUM(p_qinfo);
		p_size_qinfo->buf_pos = p_qinfo->queue_tail;
		p_size_qinfo->size_remain = size_remain;
		p_size_qinfo->size_total = p_qinfo->size_total;
		p_size_qinfo->size_written = p_qinfo->size_written;
	}

	return 0;
}

INT32 fileout_queue_get_buf_info(INT32 queue_idx, UINT32 buf_pos, ISF_FILEOUT_BUF *p_buf)
{
	FILEOUT_QUEUE_INFO *p_qinfo;

	p_qinfo = fileout_get_qinfo(queue_idx);
	if (NULL == p_qinfo) {
		return -1;
	}
	if (0 == p_qinfo->is_active) {
		return -1;
	}
	if (IS_QUE_EMPTY(p_qinfo)) {
		return -1;
	}

	p_buf = &p_qinfo->op_buf[buf_pos];

	return 0;
}

INT32 fileout_queue_set_max_pop_size(UINT64 size)
{
	g_max_pop_size = size;
	return 0;
}

INT32 fileout_queue_get_max_pop_size(UINT64 *p_size)
{
	if (NULL == p_size)
		return -1;

	*p_size = g_max_pop_size;
	return 0;
}

INT32 fileout_queue_set_max_file_size(UINT64 size)
{
	g_max_file_size = size;
	return 0;
}

INT32 fileout_queue_get_max_file_size(UINT64 *p_size)
{
	if (NULL == p_size)
		return -1;

	*p_size = g_max_file_size;
	return 0;
}

INT32 fileout_queue_set_need_size(INT32 queue_idx, UINT64 size)
{
	g_file_need_size[queue_idx] = size;
	return 0;
}

INT32 fileout_queue_get_need_size(INT32 queue_idx, UINT64 *p_size)
{
	if (NULL == p_size) {
		return -1;
	}

	*p_size = g_file_need_size[queue_idx];
	return 0;
}

INT32 fileout_port_init(void)
{
	INT32 i;
	g_max_pop_size = FILEOUT_DEFAULT_MAX_POP_SIZE;
	g_max_file_size = FILEOUT_QUEUE_MAX_FILE_SIZE;
	for (i = 0; i < FILEOUT_QUEUE_NUM; i++) {
		g_cur_drive_info[i] = FILEOUT_DRV_NAME_FIRST;
	}
	memset((void *)g_queue_info, 0, sizeof(FILEOUT_QUEUE_INFO)*FILEOUT_QUEUE_NUM);
	memset((void *)g_file_need_size, 0, sizeof(UINT64)*FILEOUT_QUEUE_NUM);
	return 0;
}

