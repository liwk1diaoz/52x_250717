#include <kwrap/type.h>
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include <kwrap/stdio.h>
#include "GxVideo.h"
#include "GxVideo_int.h"

BOOL Cmd_gxvideo_dump(unsigned char argc, char **argv)
{
	GxVideo_DumpInfo();
	return TRUE;
}
BOOL Cmd_gxvideo_dumpbuf(unsigned char argc, char **argv)
{
	UINT8 dev_id;
	sscanf_s(argv[0], "%lu", (UINT32 *)&dev_id);
	DBG_DUMP("dev id = %d \n\r", dev_id);
	//_DD_DumpBuf(dev_id);
	return TRUE;
}

BOOL Cmd_gxvideo_bon(unsigned char argc, char **argv)
{
	// Enable LCD backlight
	GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_BACKLIGHT, TRUE);
	return TRUE;
}
BOOL Cmd_gxvideo_blv(unsigned char argc, char **argv)
{
	UINT8 level;
	sscanf_s(argv[0], "%lu", (UINT32 *)&level); //fix for CID 44064
	DBG_DUMP("LCD level = %d \n\r", level);
	GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_BRIGHTLVL, level);
	return TRUE;
}
BOOL Cmd_gxvideo_boff(unsigned char argc, char **argv)
{
	// Disable LCD backlight
	GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_BACKLIGHT, FALSE);
	return TRUE;
}
BOOL Cmd_gxvideo_sleep(unsigned char argc, char **argv)
{
	UINT32 bsleep;
	sscanf_s(argv[0], "%lu", (UINT32 *)&bsleep); //fix for CID 44064
	GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SLEEP, bsleep);
	return TRUE;
}

static SXCMD_BEGIN(gxvideo_tbl, "video device control")
SXCMD_ITEM("dump", Cmd_gxvideo_dump, "dump info")
SXCMD_ITEM("dumpbuf", Cmd_gxvideo_dumpbuf, "dump buffer")
SXCMD_ITEM("bon", Cmd_gxvideo_bon, "backlight turn on")
SXCMD_ITEM("blv %", Cmd_gxvideo_blv, "backlight set level")
SXCMD_ITEM("boff", Cmd_gxvideo_boff, "backlight turn off")
SXCMD_ITEM("sleep %", Cmd_gxvideo_sleep, "lcd sleep")
SXCMD_END()

static int gxvideo_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(gxvideo_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "ker");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", gxvideo_tbl[loop].p_name, gxvideo_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(gxvideo, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(gxvideo_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		gxvideo_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], gxvideo_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = gxvideo_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}

	return 0;
}

