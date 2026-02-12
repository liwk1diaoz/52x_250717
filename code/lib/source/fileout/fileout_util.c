/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "fileout_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static INT32 g_log_quesize_is_on = 0;
static INT32 g_log_queinfo_is_on = 0;
static INT32 g_log_fdbinfo_is_on = 0;
static INT32 g_log_durinfo_is_on = 0;
static INT32 g_log_bufinfo_is_on = 0;
INT32 g_log_dbginfo_is_on = 0;
static INT32 g_set_slowcard_is_on = 0;
static INT32 g_set_fallocate_is_on = 0;
static INT32 g_set_formatfree_is_on = 0;
static INT32 g_set_wait_ready_is_on = 0;
static INT32 g_set_close_on_exec_is_on = 0;
static INT32 g_set_usememblk_is_on = 0;
static INT32 g_set_append_is_on = 0;
static UINT32 g_set_append_size = 0;
static INT32 g_set_skip_ops_is_on[FILEOUT_QUEUE_NUM] = {0};
static INT32 g_set_slowcard_ms = 0;
UINT32       g_tsk_bs_buf_padr  = 0;
UINT32       g_tsk_bs_buf_addr  = 0;
UINT32       g_tsk_bs_buf_size  = 0;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT32 fileout_util_cre_mem_blk(UINT32 blk_size)
{
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32            phy_addr;
	void              *vrt_addr;
	UINT32            size, addr;
	HD_RESULT         rv;

	size = (blk_size < FILEOUT_DEFAULT_MAX_POP_SIZE) ? blk_size : FILEOUT_DEFAULT_MAX_POP_SIZE;

	// create mempool
	rv = hd_common_mem_alloc("FileOut", &phy_addr, (void **)&vrt_addr, size, ddr_id);
	if (rv != HD_OK) {
		DBG_ERR("FileOut alloc fail size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)ddr_id);
		return -1;
	}
	addr = (UINT32)vrt_addr;

	g_tsk_bs_buf_padr = phy_addr;
	g_tsk_bs_buf_addr = (UINT32)vrt_addr;
	g_tsk_bs_buf_size = size;

	return addr;
}

INT32 fileout_util_rel_mem_blk(void)
{
	HD_RESULT rv;

	rv = hd_common_mem_free((UINT32)g_tsk_bs_buf_padr, (void *)g_tsk_bs_buf_addr);
	if (rv != HD_OK) {
		DBG_ERR("BsMux release blk failed! (%d)\r\n", rv);
		return -1;
	}

	g_tsk_bs_buf_padr = 0;
	g_tsk_bs_buf_addr = 0;
	g_tsk_bs_buf_size = 0;

	return 0;
}

INT32 fileout_util_get_blk_addr(void)
{
	return g_tsk_bs_buf_addr;
}

INT32 fileout_util_get_blk_size(void)
{
	return g_tsk_bs_buf_size;
}

void fileout_util_time_delay_ms(void)
{
	if (g_set_slowcard_is_on) {
		vos_util_delay_ms(g_set_slowcard_ms);
	}

	return ;
}

void fileout_util_time_mark(UINT32 *p_time)
{
	vos_perf_mark(p_time);
}

void fileout_util_time_dur_ms(UINT32 old_time, UINT32 new_time, UINT32 *p_time)
{
	UINT32 dur_ms;

	dur_ms = vos_perf_duration(old_time, new_time) / 1000;
	*p_time = dur_ms;

	return ;
}

INT32 fileout_util_is_null_term(CHAR *p_path, UINT32 max_size)
{
	CHAR *p_char = p_path + max_size - 1;

	for ( ; p_char >= p_path; p_char--) {
		if ('\0' == *p_char) {
			return 1;
		}
	}

	return 0;
}

void fileout_util_show_bufinfo(const ISF_FILEOUT_BUF *p_buf)
{
	if (g_log_bufinfo_is_on) {
		DBG_DUMP("-------------------------- Fileout Buf Info --------------------------\r\n");
		FILEOUT_DUMP("path %s\r\n", p_buf->p_fpath);
		FILEOUT_DUMP("event 0x%X, type 0x%X\r\n",
			p_buf->event, p_buf->type);
		FILEOUT_DUMP("fileop 0x%X, addr 0x%X, size %lld, pos %lld\r\n",
			p_buf->fileop, p_buf->addr, p_buf->size, p_buf->pos);
		DBG_DUMP("-----------------------------------------------------------------------\r\n");
	}

	return ;
}

void fileout_util_show_quesize(void)
{
	FILEOUT_QUEUE_SIZE_INFO size_qinfo = {0};
	INT32 cur_qidx;
	INT32 ret;

	if (g_log_quesize_is_on) {

		DBG_DUMP("-------------------------- Fileout Size Info --------------------------\r\n");
		for (cur_qidx = 0; cur_qidx < FILEOUT_QUEUE_NUM; cur_qidx++) {
			ret = fileout_queue_get_size_info(cur_qidx, &size_qinfo);
			if (ret == 0) {
				FILEOUT_DUMP("Q[%d] Num %d Remain %lld Written %lld Total %lld\r\n",
					cur_qidx, size_qinfo.buf_remain, size_qinfo.size_remain,
					size_qinfo.size_written, size_qinfo.size_total);
			}
		}
		DBG_DUMP("-----------------------------------------------------------------------\r\n");
	}

	return ;
}

void fileout_util_show_queinfo(INT32 queue_idx)
{
	FILEOUT_QUEUE_SIZE_INFO size_qinfo = {0};
	ISF_FILEOUT_BUF buf = {0};
	FST_FILE filehdl;
	UINT32 buf_pos;
	UINT32 cur_pos;
	INT32 port_idx;
	UINT32 index;
	INT32 ret;

	if (g_log_queinfo_is_on) {

		DBG_DUMP("------------------------ Fileout Queue[%d] Info ------------------------\r\n", queue_idx);

		ret = fileout_queue_get_size_info(queue_idx, &size_qinfo);
		if (ret != 0) {
			return ;
		}
		buf_pos = size_qinfo.buf_pos;
		port_idx = fileout_queue_get_port_idx(queue_idx);
		filehdl = fileout_queue_get_filehdl(queue_idx);

		FILEOUT_DUMP("Port[%d]\r\n", port_idx);
		FILEOUT_DUMP("Filehdl %d\r\n", (int)filehdl);
		FILEOUT_DUMP("Buff remain %d\r\n", size_qinfo.buf_remain);
		FILEOUT_DUMP("Size remain %lld, written %lld, total %lld\r\n",
			size_qinfo.size_remain, size_qinfo.size_written, size_qinfo.size_total);

		FILEOUT_DUMP("Fileop\r\n");
		for (index = 0; index < size_qinfo.buf_remain; index++) {
			cur_pos = buf_pos + index;
			if (cur_pos >=  FILEOUT_OP_BUF_NUM) {
				cur_pos -= FILEOUT_OP_BUF_NUM;
			}
			fileout_queue_get_buf_info(queue_idx, cur_pos, &buf);
			FILEOUT_DUMP("fileop 0x%X, addr 0x%X, size %lld, pos %lld.\r\n",
				buf.fileop, buf.addr, buf.size, buf.pos);
		}
		FILEOUT_DUMP("\r\n");

		DBG_DUMP("-----------------------------------------------------------------------\r\n");
	}

	return ;
}

void fileout_util_show_fdbinfo(void)
{
	if (g_log_fdbinfo_is_on) {

	}
	return ;
}

void fileout_util_show_opsinfo(UINT32 dms, UINT32 type, ISF_FILEOUT_BUF *p_buf, INT32 qidx)
{
	UINT32 limit = 0;
	CHAR *p_type = NULL;
	UINT32 write_size = 0;
	UINT64 write_pos = 0;
	CHAR *write_path = NULL;

	switch (type) {
	case ISF_FILEOUT_FOP_CREATE:
		limit = FILEOUT_DUR_LIMIT_CREATE;
		p_type = "CREATE";
		write_path = p_buf->p_fpath;
		if (g_log_durinfo_is_on) {
			FILEOUT_DUMP("Q[%d] %s %s %d ms\r\n", qidx, p_type, write_path, dms);
		}
		else {
			if (dms > limit) {
				DBG_DUMP("^G Q[%d] %s %d > %d ms\r\n", qidx, p_type, dms, limit);
			}
			FILEOUT_DUMP("Q[%d] %s %s\r\n", qidx, p_type, write_path);
		}
		break;
	case ISF_FILEOUT_FOP_CLOSE:
		limit = FILEOUT_DUR_LIMIT_CLOSE;
		p_type = "CLOSE";
		write_path = p_buf->p_fpath;
		if (g_log_durinfo_is_on) {
			FILEOUT_DUMP("Q[%d] %s %s %d ms\r\n", qidx, p_type, write_path, dms);
		}
		else {
			if (dms > limit) {
				DBG_DUMP("^G Q[%d] %s %d > %d ms\r\n", qidx, p_type, dms, limit);
			}
			FILEOUT_DUMP("Q[%d] %s %s\r\n", qidx, p_type, write_path);
		}
		break;
	case ISF_FILEOUT_FOP_CONT_WRITE:
		limit = FILEOUT_DUR_LIMIT_WRITE;
		p_type = "CONT_WRITE";
		write_size = p_buf->size;
		if (g_log_durinfo_is_on) {
			FILEOUT_DUMP("Q[%d] %s %u %d ms\r\n", qidx, p_type, write_size, dms);
		}
		else {
			if (dms > limit) {
				DBG_DUMP("^G Q[%d] %s (%u) %d > %d ms\r\n", qidx, p_type, write_size, dms, limit);
			}
			//FILEOUT_DUMP("Q[%d] %s %u\r\n", qidx, p_type, write_size);
		}
		break;
	case ISF_FILEOUT_FOP_SEEK_WRITE:
		limit = FILEOUT_DUR_LIMIT_WRITE;
		p_type = "SEEK_WRITE";
		write_size = p_buf->size;
		write_pos = p_buf->pos;
		if (g_log_durinfo_is_on) {
			FILEOUT_DUMP("Q[%d] %s %u pos %llu %d ms\r\n", qidx, p_type, write_size, write_pos, dms);
		}
		else {
			if (dms > limit) {
				DBG_DUMP("^G Q[%d] %s %d > %d ms\r\n", qidx, p_type, dms, limit);
			}
			//FILEOUT_DUMP("Q[%d] %s %u pos %llu\r\n", qidx, p_type, write_size, write_pos);
		}
		break;
	case ISF_FILEOUT_FOP_FLUSH:
		limit = FILEOUT_DUR_LIMIT_FLUSH;
		p_type = "FLUSH";
		if (g_log_durinfo_is_on) {
			FILEOUT_DUMP("Q[%d] %s %d ms\r\n", qidx, p_type, dms);
		}
		else {
			if (dms > limit) {
				DBG_DUMP("^G Q[%d] %s %d > %d ms\r\n", qidx, p_type, dms, limit);
			}
			//FILEOUT_DUMP("Q[%d] %s\r\n", qidx, p_type);
		}
		break;
	case ISF_FILEOUT_FOP_SNAPSHOT:
		limit = FILEOUT_DUR_LIMIT_SNAPSHOT;
		p_type = "SNAPSHOT";
		write_path = p_buf->p_fpath;
		if (g_log_durinfo_is_on) {
			FILEOUT_DUMP("Q[%d] %s %s %d ms\r\n", qidx, p_type, write_path, dms);
		}
		else {
			if (dms > limit) {
				DBG_DUMP("^G Q[%d] %s %d > %d ms\r\n", qidx, p_type, dms, limit);
			}
			FILEOUT_DUMP("Q[%d] %s %s\r\n", qidx, p_type, write_path);
		}
		break;
	default:
		break;
	}

	// callback
	if (dms > limit) {
		/* store original */
		UINT32 buf_event = p_buf->event;
		UINT32 buf_fileop = p_buf->fileop;
		/* callback */
		p_buf->event = dms;
		p_buf->fileop = type;
		if (0 != fileout_cb_call_fs_err(qidx, p_buf, FILEOUT_CB_ERRCODE_FOP_SLOW)) {
			DBG_ERR("fop flow callback error\r\n");
		}
		/* put back */
		p_buf->event = buf_event;
		p_buf->fileop = buf_fileop;
	}

	return ;
}

void fileout_util_set_log(INT32 log_type, INT32 is_on)
{
	switch (log_type) {
	case FILEOUT_LOGTYPE_QUE_SIZE:
		g_log_quesize_is_on = is_on;
		break;
	case FILEOUT_LOGTYPE_QUE_INFO:
		g_log_queinfo_is_on = is_on;
		break;
	case FILEOUT_LOGTYPE_FDB_INFO:
		g_log_fdbinfo_is_on = is_on;
		break;
	case FILEOUT_LOGTYPE_DUR_INFO:
		g_log_durinfo_is_on = is_on;
		break;
	case FILEOUT_LOGTYPE_BUF_INFO:
		g_log_bufinfo_is_on = is_on;
		break;
	case FILEOUT_LOGTYPE_DBG_INFO:
		g_log_dbginfo_is_on = is_on;
		break;
	default:
		break;
	}
	return ;
}

void fileout_util_set_slowcard(INT32 dur_ms, INT32 is_on)
{
	g_set_slowcard_is_on = is_on;
	g_set_slowcard_ms = dur_ms;
	return ;
}

INT32 fileout_util_is_slowcard(void)
{
	return g_set_slowcard_is_on;
}

void fileout_util_set_fallocate(INT32 is_on)
{
	g_set_fallocate_is_on = is_on;
	return ;
}

INT32 fileout_util_is_fallocate(void)
{
	return g_set_fallocate_is_on;
}

void fileout_util_set_formatfree(INT32 is_on)
{
	g_set_formatfree_is_on = is_on;
	return ;
}

INT32 fileout_util_is_formatfree(void)
{
	return g_set_formatfree_is_on;
}

void fileout_util_set_wait_ready(INT32 is_on)
{
	g_set_wait_ready_is_on = is_on;
	return ;
}

INT32 fileout_util_is_wait_ready(void)
{
	return g_set_wait_ready_is_on;
}

void fileout_util_set_close_on_exec(INT32 is_on)
{
	g_set_close_on_exec_is_on = is_on;
	return ;
}

INT32 fileout_util_is_close_on_exec(void)
{
	return g_set_close_on_exec_is_on;
}

void fileout_util_set_usememblk(INT32 is_on)
{
	g_set_usememblk_is_on = is_on;
	return ;
}

INT32 fileout_util_is_usememblk(void)
{
	return g_set_usememblk_is_on;
}

void fileout_util_set_append(INT32 is_on)
{
	g_set_append_is_on = is_on;
	return ;
}

void fileout_util_set_append_size(UINT32 size)
{
	g_set_append_size = size;
	DBG_DUMP("size=0x%lx\r\n", (unsigned long)g_set_append_size);
	return ;
}

INT32 fileout_util_is_append(void)
{
	return g_set_append_is_on;
}

UINT32 fileout_util_get_append_size(void)
{
	return g_set_append_size;
}

void fileout_util_set_skip_ops(INT32 queue_idx, INT32 is_on)
{
	g_set_skip_ops_is_on[queue_idx] = is_on;
	return ;
}

INT32 fileout_util_is_skip_ops(INT32 queue_idx)
{
	return g_set_skip_ops_is_on[queue_idx];
}

INT32 fileout_util_init(void)
{
	g_log_quesize_is_on = 0;
	g_log_queinfo_is_on = 0;
	g_log_fdbinfo_is_on = 0;
	g_log_durinfo_is_on = 0;
	g_log_bufinfo_is_on = 0;
	g_log_dbginfo_is_on = 0;
	g_set_slowcard_is_on = 0;
	g_set_fallocate_is_on = 0;
	g_set_formatfree_is_on = 0;
	g_set_wait_ready_is_on = 0;
	g_set_close_on_exec_is_on = 0;
	g_set_usememblk_is_on = 0;
	g_set_append_is_on = 0;
	g_set_append_size = 0;
	g_set_skip_ops_is_on[0] = 0;
	g_set_skip_ops_is_on[1] = 0;
	g_set_skip_ops_is_on[2] = 0;
	g_set_skip_ops_is_on[3] = 0;
	g_set_slowcard_ms = 0;
	return 0;
}