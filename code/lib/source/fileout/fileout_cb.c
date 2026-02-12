/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "fileout_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct {
	CHAR file_path[FILEOUT_PATH_LEN];
	UINT32 is_used;
	UINT32 is_fs_err;
} FILEOUT_PATH_INFO;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static FILEOUT_PATH_INFO g_path_info[FILEOUT_OP_BUF_NUM] = {0};

static FILEOUT_EVENT_CB g_event_cb = NULL;

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
static FILEOUT_PATH_INFO * fileout_get_empty_path(void)
{
	FILEOUT_PATH_INFO	*path_info = NULL;
	INT32 idx;

	fileout_path_lock();

	for (idx = 0; idx < FILEOUT_OP_BUF_NUM; idx++) {
		path_info = &g_path_info[idx];
		if (0 == path_info->is_used) {
			memset(path_info->file_path, 0, FILEOUT_PATH_LEN);
			path_info->is_used = 1;
			path_info->is_fs_err = 0;
			fileout_path_unlock();
			return path_info;
		}
	}

	DBG_ERR("No empty path\r\n");

	fileout_path_unlock();
	return NULL;
}

static INT32 fileout_release_path(CHAR *file_path)
{
	FILEOUT_PATH_INFO *path_info = NULL;
	INT32 idx;

	fileout_path_lock();

	idx = ((INT32)file_path - (INT32)g_path_info) / (sizeof(FILEOUT_PATH_INFO));
	DBG_IND("Index %d\r\n", idx);
	path_info = &g_path_info[idx];

	if (file_path != path_info->file_path) {
		DBG_ERR("Invalid address 0x%X\r\n", (unsigned int)file_path);
		fileout_path_unlock();
		return -1;
	}

	if (0 == path_info->is_used){
		DBG_ERR("Invalid state\r\n");
		fileout_path_unlock();
		return -1;
	}

	// release
	path_info->is_used = 0;
	path_info->is_fs_err = 0;
	memset(path_info->file_path, 0, sizeof(path_info->file_path));

	fileout_path_unlock();
	return 0;
}

static INT32 fileout_handle_path_is_fs_err(CHAR *file_path)
{
	FILEOUT_PATH_INFO *path_info = NULL;
	INT32 idx;

	fileout_path_lock();

	idx = ((INT32)file_path - (INT32)g_path_info) / (sizeof(FILEOUT_PATH_INFO));
	DBG_IND("Index %d\r\n", idx);
	path_info = &g_path_info[idx];

	if (file_path != path_info->file_path) {
		DBG_ERR("Invalid address 0x%X\r\n", (unsigned int)file_path);
		fileout_path_unlock();
		return -1;
	}

	if (0 == path_info->is_fs_err){
		DBG_WRN("Callback Errcode Of Path(%s)\r\n", file_path);
		path_info->is_fs_err = 1;
		fileout_path_unlock();
		return 1;
	}

	fileout_path_unlock();
	return 0;
}

#if FILEOUT_TEST_FILE_CONTENT
static INT32 fileout_handle_path_content(CHAR *file_path)
{
	FST_FILE filehdl2 = NULL;
	UINT64 file_size, file_offset;
	UINT32 size = 16, i;
	char use_buf[20] = {0};
	INT32  ret_fs;

	file_size = FileSys_GetFileLen(file_path);
	FILEOUT_DUMP("path %s len %d\r\n", file_path, (int)file_size);

	filehdl2 = FileSys_OpenFile(file_path, FST_OPEN_READ);
	if (NULL == filehdl2) {
		DBG_ERR("path[%s] open failed\r\n", file_path);
		return -1;
	}
	file_offset = FileSys_TellFile(filehdl2);
	FILEOUT_DUMP("path %s offset %d\r\n", file_path, (int)file_offset);

	ret_fs = FileSys_SeekFile(filehdl2, (file_size - 16), FST_SEEK_SET);
	if (ret_fs != FST_STA_OK) {
		DBG_ERR("path[%s] seek1 failed\r\n", file_path);
		return -1;
	}
	ret_fs = FileSys_ReadFile(filehdl2, (UINT8 *)use_buf, &size, 0, NULL);
	if (ret_fs != FST_STA_OK) {
		DBG_ERR("path[%s] read failed\r\n", file_path);
		return -1;
	}
	ret_fs = FileSys_SeekFile(filehdl2, file_offset, FST_SEEK_SET);
	if (ret_fs != FST_STA_OK) {
		DBG_ERR("path[%s] seek2 failed\r\n", file_path);
	}

	for (i = 0; i < size; i++) {
		DBG_DUMP("0x%x ", use_buf[i]);
	}
	FILEOUT_DUMP("\r\n");

	ret_fs = FileSys_FlushFile(filehdl2);
	if (ret_fs != FST_STA_OK) {
		DBG_ERR("path[%s] flush failed\r\n", file_path);
	}

	ret_fs = FileSys_CloseFile(filehdl2);
	if (ret_fs != FST_STA_OK) {
		DBG_ERR("path[%s] close failed\r\n", file_path);
	}

	return 0;
}
#endif

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void fileout_cb_register(FILEOUT_EVENT_CB new_cb)
{
	if (NULL == new_cb) {
		DBG_ERR("new_cb is NULL, skip\r\n");
		return;
	}

	g_event_cb = new_cb;
}

INT32 fileout_cb_store_path(ISF_FILEOUT_BUF *p_buf, const CHAR *p_req_path)
{
	FILEOUT_PATH_INFO *path_info = NULL;

	if (NULL == p_buf) {
		DBG_ERR("Invalid param\r\n");
		return -1;
	}

	path_info = fileout_get_empty_path();
	if (NULL == path_info) {
		DBG_ERR("Get path info failed\r\n");
		return -1;
	}
	p_buf->p_fpath = path_info->file_path;
	p_buf->fpath_size = sizeof(path_info->file_path);

	strncpy(p_buf->p_fpath, p_req_path, sizeof(p_buf->p_fpath)-1);
	p_buf->p_fpath[sizeof(p_buf->p_fpath)-1] = '\0';

	return 0;
}

INT32 fileout_cb_call_naming(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf)
{
	FILEOUT_CB_EVDATA_NAMING cb_param = {0};
	FILEOUT_PATH_INFO *path_info = NULL;
	INT32 port_idx;
	INT32 ret;
	UINT32 old_time, new_time;
	UINT32 dur_ms = 0;

	if (NULL == p_buf) {
		DBG_ERR("Invalid param\r\n");
		return -1;
	}

	if (NULL == g_event_cb) {
		DBG_ERR("g_event_cb not registered\r\n");
		return -1;
	}

	port_idx = fileout_queue_get_port_idx(queue_idx);
	if (port_idx < 0) {
		DBG_ERR("get port idx failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	//find file path table
	path_info = fileout_get_empty_path();
	if (NULL == path_info) {
		DBG_ERR("Get path info failed\r\n");
		return -1;
	}
	p_buf->p_fpath = path_info->file_path;
	p_buf->fpath_size = sizeof(path_info->file_path);

	cb_param.iport = FILEOUT_IPORT(port_idx);
	cb_param.event = p_buf->event;
	cb_param.type = p_buf->type;
	cb_param.p_fpath = p_buf->p_fpath;
	cb_param.fpath_size = p_buf->fpath_size;
	cb_param.is_reuse = fileout_util_is_formatfree();

	if (0 == fileout_util_is_null_term(cb_param.p_fpath, cb_param.fpath_size)) {
		DBG_ERR("path not null terminated\r\n");
		return -1;
	}

	cb_param.cb_event = FILEOUT_CB_EVENT_NAMING;
	fileout_util_time_mark(&old_time);
	ret = g_event_cb("FileOut", &cb_param, NULL);
	fileout_util_time_mark(&new_time);
	if (0 != ret) {
		DBG_ERR("cb naming failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
	if (dur_ms > FILEOUT_DUR_LIMIT_CB_NAMING) {
		DBG_WRN("Q[%d] CB_NAMING %d > %d ms\r\n", queue_idx, dur_ms, FILEOUT_DUR_LIMIT_CB_NAMING);
	}

	if (strlen(cb_param.p_fpath) <= 0) {
		DBG_ERR("get path failed\r\n");
		return -1;
	}

	DBG_IND("new path [%s]\r\n", cb_param.p_fpath);

	return 0;
}

INT32 fileout_cb_call_opened(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf)
{
	FILEOUT_CB_EVDATA_OPENED cb_param = {0};
	INT32 port_idx;
	INT32 ret;

	if (NULL == p_buf) {
		DBG_ERR("Invalid param\r\n");
		return -1;
	}

	if (NULL == g_event_cb) {
		DBG_ERR("g_event_cb not registered\r\n");
		return -1;
	}

	if (fileout_util_is_formatfree()) {
		return 0;
	}

	port_idx = fileout_queue_get_port_idx(queue_idx);
	if (port_idx < 0) {
		DBG_ERR("get port idx failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	cb_param.iport = FILEOUT_IPORT(port_idx);
	cb_param.p_fpath = p_buf->p_fpath;
	cb_param.fpath_size = p_buf->fpath_size;
	cb_param.alloc_size = 0; //init
	cb_param.drive = p_buf->p_fpath[0];

	cb_param.cb_event = FILEOUT_CB_EVENT_OPENED;
	ret = g_event_cb("FileOut", &cb_param, NULL);
	if (0 != ret) {
		DBG_ERR("cb opened failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	if (0 != fileout_queue_set_need_size(queue_idx, cb_param.alloc_size)) {
		DBG_ERR("set need size failed\r\n");
		return -1;
	}

	return 0;
}

INT32 fileout_cb_call_closed(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf)
{
	FILEOUT_CB_EVDATA_CLOSED cb_param = {0};
	INT32 port_idx;
	INT32 ret;
	UINT32 old_time, new_time;
	UINT32 dur_ms = 0;

	if (NULL == g_event_cb) {
		DBG_ERR("g_event_cb not registered\r\n");
		return -1;
	}

	port_idx = fileout_queue_get_port_idx(queue_idx);
	if (port_idx < 0) {
		DBG_ERR("get port idx failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	if (fileout_util_is_skip_ops(queue_idx)) {
		goto label_fileout_cb_call_closed_release_path;
	}

	cb_param.iport = FILEOUT_IPORT(port_idx);
	cb_param.p_fpath = p_buf->p_fpath;
	cb_param.fpath_size = p_buf->fpath_size;
	cb_param.drive = p_buf->p_fpath[0];

#if FILEOUT_TEST_FILE_CONTENT
	fileout_handle_path_content(p_buf->p_fpath);
#endif

	cb_param.cb_event = FILEOUT_CB_EVENT_CLOSED;
	fileout_util_time_mark(&old_time);
	ret = g_event_cb("FileOut", &cb_param, NULL);
	fileout_util_time_mark(&new_time);
	if (0 != ret) {
		DBG_ERR("cb closed failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
	if (dur_ms > FILEOUT_DUR_LIMIT_CB_CLOSED) {
		DBG_WRN("Q[%d] CB_CLOSED %d > %d ms\r\n", queue_idx, dur_ms, FILEOUT_DUR_LIMIT_CB_CLOSED);
	}

label_fileout_cb_call_closed_release_path:
	//release path
	DBG_IND("release path %d\r\n", (INT32)p_buf->p_fpath);
	if (0 != fileout_release_path(p_buf->p_fpath)) {
		DBG_ERR("fileout_release_path failed\r\n");
		return -1;
	}

	return 0;
}

INT32 fileout_cb_call_delete(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf)
{
	FILEOUT_CB_EVDATA_DELETE cb_param = {0};
	INT32 port_idx;
	UINT32 remain_size;
	UINT32 port_num = 0;
	CHAR drive;
	INT32 ret;
	UINT32 old_time, new_time;
	UINT32 dur_ms = 0;

	if (NULL == g_event_cb) {
		DBG_ERR("g_event_cb not registered\r\n");
		return -1;
	}

	if (fileout_util_is_formatfree()) {
		return 0;
	}

	drive = p_buf->p_fpath[0];

	port_idx = fileout_queue_get_port_idx(queue_idx);
	if (port_idx < 0) {
		DBG_ERR("get port idx failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	if (0 != fileout_queue_get_remain_size(drive, &remain_size)) {
		DBG_ERR("get_remain failed, queue_idx %d drive %c\r\n", queue_idx, drive);
		return -1;
	}

	if (0 != fileout_port_get_active(drive, &port_num)) {
		DBG_ERR("get_port_num failed, queue_idx %d port_num %d\r\n", queue_idx, port_num);
		return -1;
	}

	cb_param.drive = drive;
	cb_param.remain_size = remain_size;
	cb_param.port_num = port_num;
	cb_param.iport = FILEOUT_IPORT(port_idx);
	cb_param.p_fpath = p_buf->p_fpath;
	cb_param.fpath_size = p_buf->fpath_size;

	cb_param.cb_event = FILEOUT_CB_EVENT_DELETE;
	fileout_util_time_mark(&old_time);
	ret = g_event_cb("FileOut", &cb_param, NULL);
	fileout_util_time_mark(&new_time);
	if (0 != ret) {
		DBG_ERR("cb delete failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
	if (dur_ms > FILEOUT_DUR_LIMIT_CB_DELETE) {
		DBG_WRN("Q[%d] CB_DELETE %d > %d ms\r\n", queue_idx, dur_ms, FILEOUT_DUR_LIMIT_CB_DELETE);
	}

	if (cb_param.ret_val == FILEOUT_CB_RETVAL_NOFREE_SPACE) {
		DBG_WRN("no more free space, drive %c\r\n", drive);
		return FST_STA_NOFREE_SPACE;
	}

	if (cb_param.ret_val == FILEOUT_CB_RETVAL_DELETE_FAIL) {
		DBG_WRN("Q[%d] CB_DELETE ret_val (%d), drive %c\r\n", queue_idx, cb_param.ret_val, drive);
	}

	return 0;
}

INT32 fileout_cb_call_fs_err(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf, INT32 err_code)
{
	FILEOUT_CB_EVDATA_FS_ERR cb_param = {0};
	INT32 port_idx;
	CHAR drive;
	INT32 ret;

	if (NULL == g_event_cb) {
		DBG_ERR("g_event_cb not registered\r\n");
		return -1;
	}

	drive = p_buf->p_fpath[0];
	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", (UINT32)drive);
		return -1;
	}

	port_idx = fileout_queue_get_port_idx(queue_idx);
	if (port_idx < 0) {
		DBG_ERR("get port idx failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	if (FILEOUT_CB_ERRCODE_CARD_WR_ERR == err_code) {
	if (fileout_handle_path_is_fs_err(p_buf->p_fpath) != 1) {
		DBG_DUMP("No Need To Callback Errcode Of Path(%s)\r\n", p_buf->p_fpath);
		return 0;
	}
	}

	cb_param.iport = FILEOUT_IPORT(port_idx);
	cb_param.errcode = err_code;
	cb_param.drive = drive;
	cb_param.p_fpath = p_buf->p_fpath;
	cb_param.fpath_size = p_buf->fpath_size;

	if (FILEOUT_CB_ERRCODE_FOP_SLOW == err_code) {
		cb_param.fop = p_buf->fileop;
		cb_param.dur = p_buf->event;
	}

	cb_param.cb_event = FILEOUT_CB_EVENT_FS_ERR;
	ret = g_event_cb("FileOut", &cb_param, NULL);
	if (0 != ret) {
		DBG_ERR("cb fs err failed, queue_idx %d\r\n", queue_idx);
		return -1;
	}

	return 0;
}

INT32 fileout_cb_init(void)
{
	memset((void *)g_path_info, 0, sizeof(FILEOUT_PATH_INFO) * FILEOUT_OP_BUF_NUM);
	g_event_cb = NULL;
	return 0;
}

