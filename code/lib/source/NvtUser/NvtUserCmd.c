#include "kwrap/type.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include "kwrap/debug.h"
#include "kwrap/stdio.h"
#include <string.h>
#include "NvtUser/NvtUser.h"
static BOOL Cmd_NvtUser_Dump(unsigned char argc, char **argv)
{
	Ux_DumpEvents();
	return TRUE;
}
static BOOL Cmd_NvtUser_FpsEnable(unsigned char argc, char **argv)
{
	UINT32 enable =0;
	UINT32 cnt=0;

	sscanf_s(argv[0], "%d ", &enable);
	sscanf_s(argv[1], "%d ", &cnt);

	Ux_CalPostFps(enable,cnt);
	return TRUE;
}
static SXCMD_BEGIN(NvtUser_cmd_tbl, "NvtUser_cmd_tbl")
SXCMD_ITEM("dump", Cmd_NvtUser_Dump, "dump event")
SXCMD_ITEM("fps %", Cmd_NvtUser_FpsEnable, "fps:enable")
SXCMD_END()

static int NvtUser_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(NvtUser_cmd_tbl);
	UINT32 loop = 1;

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", NvtUser_cmd_tbl[loop].p_name, NvtUser_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(NvtUser, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(NvtUser_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		NvtUser_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], NvtUser_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = NvtUser_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}

	return 0;
}

