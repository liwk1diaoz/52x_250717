#include "eis_api.h"
#include "eis_int.h"

#if defined(_BSP_NA51000_)
#define SXCMD_NEW
#include "kwrap/sxcmd.h"
#else
#include "kwrap/sxcmd.h"
#endif
#include "kwrap/debug.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include "kwrap/cmdsys.h"
#define simple_strtoul(param1, param2, param3) strtoul(param1, param2, param3)
#include "stdlib.h"
#endif

#define EIS_PROC_TEST_CMD 0

#if EIS_PROC_TEST_CMD
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

static BOOL _eis_api_trig(unsigned char argc, char **pargv)
{
	static UINT64 vd_cnt = 0;
	VDOPRC_EIS_PROC_INFO proc_info = {0};
	UINT32 i;

	DBG_IND("\n");
	DBG_DUMP("argc=%d\n", argc);

    for (i = 0; i < argc; i++) {
		proc_info.frame_count = vd_cnt;
		//proc_info.frame_exposure_time = 12345;
		proc_info.angular_rate_x[0] = 0x11111111;
		proc_info.angular_rate_x[MAX_GYRO_DATA_NUM-1] = 0xAAAAAAAA;
		//proc_info.acceleration_rate_z[0] = 0x66666666;
		//proc_info.acceleration_rate_z[MAX_GYRO_DATA_NUM-1] = 0xbbbbbbbb;
		vd_cnt++;

		_eis_tsk_trigger_proc(&proc_info, FALSE);
	}

	return 1;
}
static BOOL _eis_api_get(unsigned char argc, char **pargv)
{
	static UINT64 frm_cnt = 0;
	ISF_RV ret = ISF_OK;
	INT32 wait_ms;
	static CHAR eis_tmp_buf[MAX_EIS_LUT2D_SIZE];
	VDOPRC_EIS_2DLUT eis_2dlut = {0};

	if (argc == 1) {
		wait_ms = 1500;
	} else if (argc == 2) {
		wait_ms = 0;
	} else {
		wait_ms = -1;
	}
	DBG_DUMP("GET OUT TEST wait_ms=%d\r\n", wait_ms);

	eis_2dlut.frame_count = frm_cnt;
	eis_2dlut.path_id = 0;
	eis_2dlut.buf_addr = (ULONG)eis_tmp_buf;
	eis_2dlut.buf_size = sizeof(eis_tmp_buf);
	ret = _eis_tsk_get_output(&eis_2dlut);
	DBG_DUMP("[0]get frm(%llu) result=%d\r\n", eis_2dlut.frame_count, ret);
	if (ret == E_OK) {
		DBG_DUMP("===== 2dlut(%d) =====\r\n", eis_2dlut.buf_size);
		DumpMem(eis_2dlut.buf_addr, eis_2dlut.buf_size, 16);
	}


	eis_2dlut.frame_count = frm_cnt;
	eis_2dlut.path_id = 1;
	eis_2dlut.buf_addr = (ULONG)eis_tmp_buf;
	eis_2dlut.buf_size = sizeof(eis_tmp_buf);
	ret = _eis_tsk_get_output(&eis_2dlut);
	DBG_DUMP("[1]get frm(%llu) result=%d\r\n", eis_2dlut.frame_count, ret);
	if (ret == E_OK) {
		DBG_DUMP("===== 2dlut(%d) =====\r\n", eis_2dlut.buf_size);
		DumpMem(eis_2dlut.buf_addr, eis_2dlut.buf_size, 16);
	}


	frm_cnt++;
	return 1;
}
#endif

static BOOL _eis_api_dump(unsigned char argc, char **pargv)
{
	DBG_IND("\n");
	_eis_msg_dump();
	_eis_out_dump();
	return 1;
}
static BOOL _eis_api_dbg(unsigned char argc, char **pargv)
{
	UINT32 dbg_cmd =0;
	UINT32 dbg_value = 0;
	unsigned char idx = 1;

	DBG_IND("\n");
	if (argc > 1) {
		sscanf(pargv[idx++], "%d", (int *)&dbg_cmd);
	}
	if (argc > 2) {
		sscanf(pargv[idx++], "%d", (int *)&dbg_value);
	}
	_eis_tsk_set_dbg_cmd(dbg_cmd, dbg_value);
	//DBG_DUMP("eis rsc dbg cmd %d, %d\r\n", dbg_cmd, dbg_value);
	return 1;
}

static SXCMD_BEGIN(eis_cmd, "eis")
SXCMD_ITEM("dump", 	_eis_api_dump,		"dump queue")
SXCMD_ITEM("dbg",_eis_api_dbg,		"debug cmd for eis rsc lib")
#if EIS_PROC_TEST_CMD
SXCMD_ITEM("trig", 	_eis_api_trig,		"trigger test")
SXCMD_ITEM("get", 	_eis_api_get,		"get output")
#endif
SXCMD_END()

int eis_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(eis_cmd);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "eis");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", eis_cmd[loop].p_name, eis_cmd[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int eis_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(eis, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(eis_cmd);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	//here, we follow standard c main() function, argv[0] should be a "program"
	//argv[0] = "vprc"
	//argv[1] = "(cmd)"
	if (strncmp(argv[1], "?", 2) == 0) {
		eis_cmd_showhelp(debug_msg);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], eis_cmd[loop].p_name, strlen(argv[1])) == 0) {
			ret = eis_cmd[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		eis_cmd_showhelp(debug_msg);
		return -1;
	}
	return 0;
}
