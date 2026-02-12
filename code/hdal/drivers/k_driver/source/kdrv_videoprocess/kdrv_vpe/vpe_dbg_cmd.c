

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

#include "vpe_drv.h"

#endif



#include <kwrap/cmdsys.h>
#include "kwrap/sxcmd.h"

#include "vpe_dbg.h"
#include "vpe_kit.h"
#include "vpe_dbg_cmd.h"


static BOOL cmd_vpe_dump_info(unsigned char argc, char **argv)
{
	//exam_msg("kdrv_vpe test\r\n");

	DBG_DUMP("kdrv_vpe test\r\n");
	nvt_vpe_api_dump(argc, argv);
	return TRUE;
}

static BOOL cmd_vpe_dump_test(unsigned char argc, char **argv)
{
	nvt_kdrv_vpe_test(argc, argv);
	return TRUE;
}

static SXCMD_BEGIN(vpe_cmd_tbl, "kdrv_vpe")
SXCMD_ITEM("info",        cmd_vpe_dump_info,         "dump kdrv_vpe info")
SXCMD_ITEM("test %s",     cmd_vpe_dump_test,         "test kdrv_vpe ")
SXCMD_END()

int vpe_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(vpe_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "vpe");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", vpe_cmd_tbl[loop].p_name, vpe_cmd_tbl[loop].p_desc);
	}
	return 0;
}


#if defined (__KERNEL__) || defined(__LINUX)
int vpe_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(vpe_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[0], "?", 2) == 0) {
		vpe_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], vpe_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = vpe_cmd_tbl[loop].p_func(argc, argv);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		vpe_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#else
MAINFUNC_ENTRY(kdrv_vpe, argc, argv)
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
		DBG_ERR("Invalid CMD !!\r\n");
		vpe_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}
#endif




