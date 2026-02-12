#include <stdio.h>
#include <string.h>

#include "kwrap/nvt_type.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"

#include "vpe_alg.h"
#include "vpet_api_int.h"
#include "vpe_dbg.h"
#include "vpe_main.h"
#include "vpe_version.h"

// NOTE: temporal


static BOOL vpe_sxcmd_dump_info(unsigned char argc, char **argv)
{

	return 0;
}

static BOOL vpe_sxcmd_get_buffer_size(unsigned char argc, char **argv)
{

	return 0;
}

static BOOL vpe_sxcmd_get_param(unsigned char argc, char **argv)
{

	return 0;
}

static BOOL vpe_sxcmd_get_ui_param(unsigned char argc, char **argv)
{

	return 0;
}

static BOOL vpe_sxcmd_get_cfg_data(unsigned char argc, char **argv)
{
	return 0;
}

static BOOL vpe_sxcmd_set_dbg(unsigned char argc, char **argv)
{
	VPE_ID id;
	UINT32 dbg_lv;

	if (argc < 2) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		DBG_DUMP("please set (id, dbg_lv) \r\n");
		return -1;
	}

	sscanf(argv[0], "%x", &id);
	sscanf(argv[1], "%x", (int *)&dbg_lv);

	DBG_DUMP("set vpe(%d) dbg level(0x%x) \r\n", id, dbg_lv);
	vpe_dbg_set_dbg_mode(id, dbg_lv);

	return 0;
}

static SXCMD_BEGIN(vpe_cmd_tbl, "vpe")
SXCMD_ITEM("info",              vpe_sxcmd_dump_info,               "dump vpe info")

SXCMD_ITEM("get_buffer_size",   vpe_sxcmd_get_buffer_size,         "get vpe buffer size")
SXCMD_ITEM("get_param",         vpe_sxcmd_get_param,               "get vpe param, param1 is vpe_id(0~2), param2 is param_id(0~15)")
SXCMD_ITEM("get_ui_param",      vpe_sxcmd_get_ui_param,            "get vpe ui parameter, param1 is vpe_id(0~2)")
SXCMD_ITEM("get_cfg_data",      vpe_sxcmd_get_cfg_data,            "get vpe cfg file name, param1 is vpe_id(0~2)")

SXCMD_ITEM("set_dbg",           vpe_sxcmd_set_dbg,                 "set vpe dbg level, param1 is vpe_id(0~2), param2 is dbg_lv")
SXCMD_END()

int vpe_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	// TODO: VPE
	//UINT32 cmd_num = SXCMD_NUM(vpe_cmd_tbl);
	//UINT32 loop;

	DBG_DUMP("1. 'vpe info' will show all the vpe info \r\n");

	return 0;
}

MAINFUNC_ENTRY(vpe, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(vpe_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		vpe_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], vpe_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = vpe_cmd_tbl[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !! \r\n");
		vpe_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}

