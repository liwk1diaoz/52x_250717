#include "dce_kit.h"
#include "dce_platform.h"
#include <kwrap/cmdsys.h>
#include "kwrap/sxcmd.h"

static BOOL cmd_dce_dump_info(unsigned char argc, char **argv)
{
	DBG_DUMP("dump dce info\r\n");
	kdrv_api_dce_dump_info(argc, argv);
	return TRUE;
}

static BOOL cmd_dce_d2d_test(unsigned char argc, char **argv)
{
	DBG_DUMP("kdrv_dce d2d test\r\n");
	kdrv_api_dce_test(argc, argv);
	return TRUE;
}

static BOOL cmd_dce_dbg_level(unsigned char argc, char **argv)
{
	DBG_DUMP("modify kdrv_dce debug level\r\n");
	kdrv_api_dce_set_dbg_level(argc, argv);
	return TRUE;
}

static BOOL cmd_dce_hw_power_saving(unsigned char argc, char **argv)
{
	DBG_DUMP("HW power saving\r\n");
	kdrv_api_dce_hw_power_saving(argc, argv);
	return TRUE;
}

static BOOL cmd_dce_fw_power_saving(unsigned char argc, char **argv)
{
	DBG_DUMP("FW power saving\r\n");
	kdrv_api_dce_fw_power_saving(argc, argv);
	return TRUE;
}

static SXCMD_BEGIN(dce_cmd_tbl, "kdrv_dce")
SXCMD_ITEM("info",        cmd_dce_dump_info,         "dump kdrv_dce info")
SXCMD_ITEM("d2d %s",      cmd_dce_d2d_test,          "test kdrv_dce run d2d")
SXCMD_ITEM("dbg_lv %s",   cmd_dce_dbg_level,         "modify kdrv_dce debug level")
SXCMD_ITEM("hw_pwsv %s",  cmd_dce_hw_power_saving,   "Enable/Disable HW power saving")
SXCMD_ITEM("fw_pwsv %s",  cmd_dce_fw_power_saving,   "Enable/Disable FW power saving")
SXCMD_END()

int dce_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(dce_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "dce");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", dce_cmd_tbl[loop].p_name, dce_cmd_tbl[loop].p_desc);
	}
	return 0;
}


#if defined(__LINUX)
int dce_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(dce_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[0], "?", 2) == 0) {
		dce_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], dce_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = dce_cmd_tbl[loop].p_func(argc, argv);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		dce_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#else
MAINFUNC_ENTRY(kdrv_dce, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(dce_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		dce_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], dce_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = dce_cmd_tbl[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		dce_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#endif


