#include "GxInput.h"
#include "DxInput.h"
#include "Gesture.h"
#include "Calibrate.h"
//#include "Debug.h"

#include <kwrap/sxcmd.h>
#include <kwrap/stdio.h>
#include "kwrap/cmdsys.h"
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxTouch
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

extern BOOL       g_bDumpTouch;

BOOL Cmd_gxtouch_dump(unsigned char argc, char **argv);


static SXCMD_BEGIN(gxtouch_cmd_tbl, "GxTouch")
SXCMD_ITEM("dump", Cmd_gxtouch_dump, "Dump touch value")
SXCMD_END()

BOOL Cmd_gxtouch_dump(unsigned char argc, char **argv)
{
	if (g_bDumpTouch) { //toggle the debug message
		g_bDumpTouch = FALSE;
	} else {
		g_bDumpTouch = TRUE;
	}
	DBG_DUMP("g_bDumpTouch = %d\r\n", g_bDumpTouch);
	return TRUE;
}

static int gxtouch_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(gxtouch_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "gxtouch");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", gxtouch_cmd_tbl[loop].p_name, gxtouch_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(gxtouch, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(gxtouch_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		gxtouch_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], gxtouch_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = gxtouch_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	return 0;
}

