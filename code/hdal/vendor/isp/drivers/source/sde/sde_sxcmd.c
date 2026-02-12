#include <stdio.h>
#include <string.h>

#include "kwrap/nvt_type.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"

#include "sde_alg.h"
#include "sdet_api_int.h"
#include "sde_dbg.h"
#include "sde_main.h"
#include "sde_version.h"

// NOTE: temporal

static BOOL sde_sxcmd_dump_info(unsigned char argc, char **argv)
{

	return 0;
}

static BOOL sde_sxcmd_get_buffer_size(unsigned char argc, char **argv)
{

	return 0;
}

static BOOL sde_sxcmd_get_param(unsigned char argc, char **argv)
{

	return 0;
}

static BOOL sde_sxcmd_get_ui_param(unsigned char argc, char **argv)
{

	return 0;
}

static BOOL sde_sxcmd_get_cfg_data(unsigned char argc, char **argv)
{
	return 0;
}

static BOOL sde_sxcmd_set_dbg(unsigned char argc, char **argv)
{
	SDE_ID id;
	UINT32 dbg_lv;

	if (argc < 2) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		DBG_DUMP("please set (id, dbg_lv) \r\n");
		return -1;
	}

	sscanf(argv[0], "%x", &id);
	sscanf(argv[1], "%x", (int *)&dbg_lv);

	DBG_DUMP("set sde(%d) dbg level(0x%x) \r\n", id, dbg_lv);
	sde_dbg_set_dbg_mode(id, dbg_lv);

	return 0;
}

static SXCMD_BEGIN(sde_cmd_tbl, "sde")
SXCMD_ITEM("info",              sde_sxcmd_dump_info,               "dump sde info")

SXCMD_ITEM("get_buffer_size",   sde_sxcmd_get_buffer_size,         "get sde buffer size")
SXCMD_ITEM("get_param",         sde_sxcmd_get_param,               "get sde param, param1 is sde_id(0~2), param2 is param_id(0~15)")
SXCMD_ITEM("get_ui_param",      sde_sxcmd_get_ui_param,            "get sde ui parameter, param1 is sde_id(0~2)")
SXCMD_ITEM("get_cfg_data",      sde_sxcmd_get_cfg_data,            "get sde cfg file name, param1 is sde_id(0~2)")

SXCMD_ITEM("set_dbg",           sde_sxcmd_set_dbg,                 "set sde dbg level, param1 is sde_id(0~2), param2 is dbg_lv")
SXCMD_END()

int sde_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	// TODO: SDE
	//UINT32 cmd_num = SXCMD_NUM(sde_cmd_tbl);
	//UINT32 loop;

	DBG_DUMP("1. 'sde info' will show all the sde info \r\n");

	return 0;
}

MAINFUNC_ENTRY(sde, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(sde_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		sde_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], sde_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = sde_cmd_tbl[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !! \r\n");
		sde_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}

