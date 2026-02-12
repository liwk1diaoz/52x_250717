#include "ife_api.h"
#include <kwrap/cmdsys.h>
#include "kwrap/sxcmd.h"

static BOOL cmd_kdrv_ife_dump_info(unsigned char argc, char **argv)
{
	nvt_ife_api_dump(argc, argv);
	return TRUE;
}

static BOOL cmd_kdrv_ife_test_flow(unsigned char argc, char **argv)
{
	nvt_kdrv_ipp_api_ife_test(argc, argv);
	return TRUE;
}

static SXCMD_BEGIN(kdrv_ife_cmd_tbl, "kdrv_ife")
SXCMD_ITEM("info",             cmd_kdrv_ife_dump_info,         "kdrv ife dump info")
SXCMD_ITEM("test_flow",             cmd_kdrv_ife_test_flow,         "kdrv ife test flow")
SXCMD_END()

int kdrv_ife_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_ife_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "kdrv_ife");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", kdrv_ife_cmd_tbl[loop].p_name, kdrv_ife_cmd_tbl[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int kdrv_ife_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_ife_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[0], "?", 2) == 0) {
		kdrv_ife_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], kdrv_ife_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = kdrv_ife_cmd_tbl[loop].p_func(argc, argv);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		kdrv_ife_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#else
MAINFUNC_ENTRY(kdrv_ife, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_ife_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		kdrv_ife_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], kdrv_ife_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = kdrv_ife_cmd_tbl[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		kdrv_ife_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#endif


