

#if defined (__UITRON) || defined (__ECOS)

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Type.h"
#include "kernel.h"

#elif defined (__FREERTOS )

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <kwrap/file.h>

#include <malloc.h>
#include "kwrap/type.h"
#include "kwrap/platform.h"


#elif defined (__KERNEL__) || defined (__LINUX)
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
//#include <comm/nvtmem.h>

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include <kwrap/file.h>

#include "mach/rcw_macro.h"
#include "mach/nvt-io.h"
#include "kwrap/type.h"
#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>

#include "ime_drv.h"

#endif



#include <kwrap/cmdsys.h>
#include "kwrap/sxcmd.h"

#include "ime_dbg.h"
#include "ime_kit.h"
#include "ime_dbg_cmd.h"

#if (__IME_DBG_CMD__ == 1)
static BOOL cmd_ime_dump_info(unsigned char argc, char **argv)
{
	//exam_msg("kdrv_ime test\r\n");

	DBG_DUMP("kdrv_ime test\r\n");
	nvt_ime_api_dump(argc, argv);
	return TRUE;
}

static BOOL cmd_ime_dump_test(unsigned char argc, char **argv)
{
	nvt_kdrv_ime_test(argc, argv);
	return TRUE;
}
#endif


#if !defined (CONFIG_NVT_SMALL_HDAL)

static BOOL cmd_ime_hw_power_saving(unsigned char argc, char **argv)
{
	nvt_ime_hw_power_saving(argc, argv);
	return TRUE;
}

static BOOL cmd_ime_fw_power_saving(unsigned char argc, char **argv)
{
	nvt_ime_fw_power_saving(argc, argv);
	return TRUE;
}
#endif



static SXCMD_BEGIN(ime_cmd_tbl, "kdrv_ime")
#if (__IME_DBG_CMD__ == 1)
SXCMD_ITEM("info",        cmd_ime_dump_info,         "dump kdrv_ime info")
SXCMD_ITEM("test %s",     cmd_ime_dump_test,         "test kdrv_ime ")
#endif

#if !defined (CONFIG_NVT_SMALL_HDAL)
SXCMD_ITEM("ime_hw_pwsv %s",  cmd_ime_hw_power_saving,	 "Enable/Disable HW power saving")
SXCMD_ITEM("ime_fw_pwsv %s",  cmd_ime_fw_power_saving,	 "Enable/Disable FW power saving")
#endif
SXCMD_END()

int ime_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(ime_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "ime");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", ime_cmd_tbl[loop].p_name, ime_cmd_tbl[loop].p_desc);
	}
	return 0;
}


#if defined (__KERNEL__) || defined(__LINUX)
int ime_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(ime_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[0], "?", 2) == 0) {
		ime_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], ime_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = ime_cmd_tbl[loop].p_func(argc, argv);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		ime_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#else
MAINFUNC_ENTRY(kdrv_ime, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(ime_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		ime_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], ime_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = ime_cmd_tbl[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		ime_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#endif




