#include <kwrap/cmdsys.h>
#include "kwrap/type.h"
#include "kwrap/sxcmd.h"
#include <stdlib.h>
#include "string.h"
#include "kdrv_videoprocess/kdrv_ipp_utility.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "ipp_sim_drv.h"
#include "ipp_sim_dbg.h"
#include "ipp_sim_api.h"

static BOOL cmd_ipp_sim_dump_info(unsigned char argc, char **argv)
{
	return TRUE;
}

static BOOL cmd_ipp_direct_sim_test(unsigned char argc, char **argv)
{
	char ife_in_mode[8] = "";
	sprintf(ife_in_mode, "%d", KDRV_IFE_IN_MODE_DIRECT);
	strcpy(argv[2], ife_in_mode);
	nvt_kdrv_ipp_flow02_test(NULL, argc, argv);
	return TRUE;
}

static BOOL cmd_ipp_ife_in_d2d_sim_test(unsigned char argc, char **argv)
{
	char ife_in_mode[8] = "";
	sprintf(ife_in_mode, "%d", KDRV_IFE_IN_MODE_IPP);
	strcpy(argv[2], ife_in_mode);
	nvt_kdrv_ipp_flow02_test(NULL, argc, argv);
	return TRUE;
}

static BOOL cmd_ipp_dce_in_d2d_sim_test(unsigned char argc, char **argv)
{
	nvt_kdrv_ipp_flow1_test(NULL, argc, argv);
	return TRUE;
}

static SXCMD_BEGIN(ipp_sim_cmd_tbl, "kdrv_ipp_sim")
SXCMD_ITEM("info",       cmd_ipp_sim_dump_info,         "dump kdrv_ipp_sim info")
SXCMD_ITEM("direct %s",  cmd_ipp_direct_sim_test,       "test all-direct flow")
SXCMD_ITEM("fmd2d %s",   cmd_ipp_ife_in_d2d_sim_test,   "test ife in flow")
SXCMD_ITEM("dmd2d %s",   cmd_ipp_dce_in_d2d_sim_test,   "test dce in flow")
SXCMD_END()

int ipp_sim_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(ipp_sim_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "IPP sim");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", ipp_sim_cmd_tbl[loop].p_name, ipp_sim_cmd_tbl[loop].p_desc);
	}
	return 0;
}


#if defined(__LINUX)
int ipp_sim_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(ipp_sim_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[0], "?", 2) == 0) {
		ipp_sim_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], ipp_sim_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = ipp_sim_cmd_tbl[loop].p_func(argc, argv);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		ipp_sim_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#else
MAINFUNC_ENTRY(kdrv_ipp_sim, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(ipp_sim_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		ipp_sim_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], ipp_sim_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = ipp_sim_cmd_tbl[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		ipp_sim_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#endif



