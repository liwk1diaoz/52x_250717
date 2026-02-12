/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "fileout_init.h"

static BOOL fileout_sxcmd_openlog(unsigned char argc, char **argv)
{
	INT32 type = 1;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&type);

	if (type < FILEOUT_LOGTYPE_BASE || type >= FILEOUT_LOGTYPE_MAX) {
		DBG_ERR("set type error\r\n");
		return FALSE;
	}

	fileout_util_set_log(type, TRUE);

	return TRUE;
}

static BOOL fileout_sxcmd_closelog(unsigned char argc, char **argv)
{
	INT32 type = 1;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&type);

	if (type < FILEOUT_LOGTYPE_BASE || type >= FILEOUT_LOGTYPE_MAX) {
		DBG_ERR("set type error\r\n");
		return FALSE;
	}

	fileout_util_set_log(type, FALSE);

	return TRUE;
}

static BOOL fileout_sxcmd_slow(unsigned char argc, char **argv)
{
	INT32 dur_ms = 0;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&dur_ms);

	if (dur_ms < 0) {
		DBG_ERR("set dur_ms error\r\n");
		return FALSE;
	}

	fileout_util_set_slowcard(dur_ms, (dur_ms > 0));

	return TRUE;
}

static BOOL fileout_sxcmd_falloc(unsigned char argc, char **argv)
{
	INT32 is_on = 0;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&is_on);

	fileout_util_set_fallocate(is_on);

	return TRUE;
}

static BOOL fileout_sxcmd_wait_ready(unsigned char argc, char **argv)
{
	INT32 is_on;
	sscanf(argv[0], "%d", (int *)&is_on);

	fileout_util_set_wait_ready(is_on);

	return TRUE;
}

static BOOL fileout_sxcmd_memblk(unsigned char argc, char **argv)
{
	INT32 is_on;
	sscanf(argv[0], "%d", (int *)&is_on);

	fileout_util_set_usememblk(is_on);

	return TRUE;
}

static BOOL fileout_sxcmd_append(unsigned char argc, char **argv)
{
	INT32 is_on;
	UINT32 size;

	if (argc < 2) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&is_on);
	sscanf(argv[1], "%ld", (UINT32 *)&size);

	fileout_util_set_append(is_on);
	fileout_util_set_append_size(size);

	return TRUE;
}

static BOOL fileout_sxcmd_skip_ops(unsigned char argc, char **argv)
{
	INT32 qidx = 0, is_on = 0;
	sscanf(argv[0], "%d", (int *)&qidx);
	sscanf(argv[1], "%d", (int *)&is_on);

	fileout_util_set_skip_ops(qidx, is_on);

	return TRUE;
}

static BOOL fileout_sxcmd_set_ctrl(unsigned char argc, char **argv)
{
	HD_FILEOUT_CONFIG fout_cfg = {0};
	HD_PATH_ID fileout_ctrl = 0;
	HD_RESULT hd_ret = 0;
	UINT32 type, value;

	sscanf(argv[0], "%d", (int *)&type);
	sscanf(argv[1], "%d", (int *)&value);

	if ((hd_ret = hd_fileout_open(0, HD_FILEOUT_CTRL(0), &fileout_ctrl)) != HD_OK) {
		DBG_ERR("hd_fileout_open(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
	}

	if ((hd_ret = hd_fileout_get(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
		DBG_ERR("hd_fileout_get(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
	}

	if (type == 1) { //max_pop_size
		fout_cfg.max_pop_size = (UINT64)value;
	} else if (type == 2) { //format_free
		fout_cfg.format_free = (UINT32) value;
	} else if (type == 3) { //use_mem_blk
		fout_cfg.use_mem_blk = (UINT32) value;
	} else if (type == 4) { //wait_ready
		fout_cfg.wait_ready = (UINT32) value;
	} else if (type == 5) { //close_on_exec
		fout_cfg.close_on_exec = (UINT32) value;
	} else if (type == 6) { //max_file_size
		fout_cfg.max_file_size = (UINT64) value;
	} else {
		DBG_ERR("set type error\r\n");
	}

	if ((hd_ret = hd_fileout_set(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
		DBG_ERR("hd_fileout_set(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
	}

	if ((hd_ret = hd_fileout_close(fileout_ctrl)) != HD_OK) {
		DBG_ERR("hd_fileout_close(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
	}

	return TRUE;
}

static BOOL fileout_sxcmd_dump(unsigned char argc, char **argv)
{
	INT32 index;
	for (index = 0; index < FILEOUT_QUEUE_NUM; index++) {
		fileout_util_show_queinfo(index);
	}

	fileout_util_show_quesize();

	return TRUE;
}

static SXCMD_BEGIN(fileout_cmd_tbl, "fileout_cmd_tbl")
SXCMD_ITEM("on %",      fileout_sxcmd_openlog,     "open log [type] 1:size 2:queue 3:filedb 4:dur_ms 5:buf")
SXCMD_ITEM("off %",     fileout_sxcmd_closelog,    "close log [type] 1:size 2:queue 3:filedb 4:dur_ms 5:buf")
SXCMD_ITEM("slow %",    fileout_sxcmd_slow,        "set fake slow card [duration] ms")
SXCMD_ITEM("falloc %",  fileout_sxcmd_falloc,      "set fallocate on/off 1:on 0:off")
SXCMD_ITEM("wait %",    fileout_sxcmd_wait_ready,  "write file until ready [1:on 0:off]")
SXCMD_ITEM("memblk %",  fileout_sxcmd_memblk,      "set memblk on/off 1:on 0:off")
SXCMD_ITEM("append %",  fileout_sxcmd_append,      "set append [on/off][size] 1:on 0:off")
SXCMD_ITEM("skip %",    fileout_sxcmd_skip_ops,    "set skip write card ops [qidx][1:on 0:off]")
SXCMD_ITEM("ctrl %",    fileout_sxcmd_set_ctrl,    "set ctrl path")
SXCMD_ITEM("dump",      fileout_sxcmd_dump,        "dump")
SXCMD_END()

static int fileout_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(fileout_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "fileout (hd_fileout_lib)");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", fileout_cmd_tbl[loop].p_name, fileout_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(fileout, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(fileout_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		fileout_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], fileout_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = fileout_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	return 0;
}
