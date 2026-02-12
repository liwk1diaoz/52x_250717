#include "ipe_kit.h"
#include "ipe_platform.h"
#include <kwrap/cmdsys.h>
#include "kwrap/sxcmd.h"

#if (__IPE_DBG_CMD__ == 1)
static BOOL cmd_ipe_dump_info(unsigned char argc, char **argv)
{
	DBG_IND("dump ipe info\r\n");
	kdrv_api_ipe_dump_info(argc, argv);
	return TRUE;
}

static BOOL cmd_ipe_dump_d2d(unsigned char argc, char **argv)
{
	kdrv_api_ipe_test(argc, argv);
	return TRUE;
}

static BOOL cmd_ipe_dump_iq_test(unsigned char argc, char **argv)
{
	kdrv_api_ipe_iq_debug(argc, argv);
	return TRUE;
}

static BOOL cmd_ipe_set_dbg_cmd(unsigned char argc, char **argv)
{
	kdrv_api_ipe_set_dbg_cmd(argc, argv);
	return TRUE;
}

static BOOL cmd_ipe_set_dbg_level(unsigned char argc, char **argv)
{
	DBG_IND("modify kdrv_ipe debug level\r\n");
	kdrv_api_ipe_set_dbg_level(argc, argv);
	return TRUE;
}
#endif

#if !defined (CONFIG_NVT_SMALL_HDAL)

static BOOL cmd_ipe_hw_power_saving(unsigned char argc, char **argv)
{
	nvt_ipe_hw_power_saving(argc, argv);
	return TRUE;
}

static BOOL cmd_ipe_fw_power_saving(unsigned char argc, char **argv)
{
	nvt_ipe_fw_power_saving(argc, argv);
	return TRUE;
}
#endif


static SXCMD_BEGIN(ipe_cmd_tbl, "kdrv_ipe")
#if (__IPE_DBG_CMD__ == 1)
SXCMD_ITEM("info",        cmd_ipe_dump_info,         "dump kdrv_ipe info")
SXCMD_ITEM("d2d %s",      cmd_ipe_dump_d2d,          "test kdrv_ipe ")
SXCMD_ITEM("iq %s",       cmd_ipe_dump_iq_test,      "kdrv_ipe iq debug")
SXCMD_ITEM("dbg %s",      cmd_ipe_set_dbg_cmd,       "kdrv_ipe set debug cmd")
SXCMD_ITEM("dbg_lv %d",   cmd_ipe_set_dbg_level,       "kdrv_ipe set debug cmd")
#endif

#if !defined (CONFIG_NVT_SMALL_HDAL)
SXCMD_ITEM("ipe_hw_pwsv %s",  cmd_ipe_hw_power_saving,	 "Enable/Disable HW power saving")
SXCMD_ITEM("ipe_fw_pwsv %s",  cmd_ipe_fw_power_saving,	 "Enable/Disable FW power saving")
#endif
SXCMD_END()

int ipe_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(ipe_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "ipe");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", ipe_cmd_tbl[loop].p_name, ipe_cmd_tbl[loop].p_desc);
	}
	return 0;
}


#if defined(__LINUX)
int ipe_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(ipe_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[0], "?", 2) == 0) {
		ipe_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], ipe_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = ipe_cmd_tbl[loop].p_func(argc, argv);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		ipe_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#else
MAINFUNC_ENTRY(kdrv_ipe, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(ipe_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		ipe_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], ipe_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = ipe_cmd_tbl[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		ipe_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#endif


