//Self
#include "GxDisplay.h"
#include "GxDisplay_int.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include <kwrap/stdio.h>
#include "DL.h"

BOOL Cmd_gxdisp_dump(unsigned char argc, char **argv);
BOOL Cmd_gxdisp_dumpbuf(unsigned char argc, char **argv);
BOOL Cmd_gxdisp_on(unsigned char argc, char **argv);
BOOL Cmd_gxdisp_off(unsigned char argc, char **argv);

static SXCMD_BEGIN(gxdisp_cmd_tbl, "video device input control")
SXCMD_ITEM("dump", Cmd_gxdisp_dump, "dump info")
SXCMD_ITEM("dumpbuf %", Cmd_gxdisp_dumpbuf, "dump buffer devl layer")
SXCMD_ITEM("on %", Cmd_gxdisp_on, "enable dev layer")
SXCMD_ITEM("off %", Cmd_gxdisp_off, "disable dev layer")
SXCMD_END()

BOOL Cmd_gxdisp_dump(unsigned char argc, char **argv)
{
	//GxDisplay_DumpInfo();
	return TRUE;
}
BOOL Cmd_gxdisp_dumpbuf(unsigned char argc, char **argv)
{
	UINT32 dev_id, layer_id;
    INT32 ret=0;
	sscanf_s(argv[0], "%lu", (UINT32 *)&dev_id);
	sscanf_s(argv[1], "%lu", (UINT32 *)&layer_id);
	DBG_DUMP("dump buffer: dev[%d], layer[%d]\n\r", dev_id, layer_id);
	if ((dev_id >= 2) || (layer_id >= 4)) {
		return TRUE;
	}
	ret=_DL_DumpBuf(_LayerID(dev_id, layer_id));
    if(ret!=0){
        return FALSE;
    } else {
	    return TRUE;
    }
}
BOOL Cmd_gxdisp_on(unsigned char argc, char **argv)
{
	UINT32 dev_id, layer_id;
	sscanf_s(argv[0], "%lu", (UINT32 *)&dev_id);
	sscanf_s(argv[1], "%lu", (UINT32 *)&layer_id);
	DBG_DUMP("enable: dev[%d], layer[%d]\n\r", dev_id, layer_id);
	if ((dev_id >= 2) || (layer_id >= 4)) {
		return TRUE;
	}
	_DL_SetEnable(_LayerID(dev_id, layer_id), TRUE);
	return TRUE;
}
BOOL Cmd_gxdisp_off(unsigned char argc, char **argv)
{
	UINT32 dev_id, layer_id;
	sscanf_s(argv[0], "%lu", (UINT32 *)&dev_id);
	sscanf_s(argv[1], "%lu", (UINT32 *)&layer_id);
	DBG_DUMP("disable: dev[%d], layer[%d]\n\r", dev_id, layer_id);
	if ((dev_id >= 2) || (layer_id >= 4)) {
		return TRUE;
	}
	_DL_SetEnable(_LayerID(dev_id, layer_id), FALSE);
	return TRUE;
}

static int gxdisp_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(gxdisp_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "ker");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", gxdisp_cmd_tbl[loop].p_name, gxdisp_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(gxdisp, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(gxdisp_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		gxdisp_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], gxdisp_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = gxdisp_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}

	return 0;
}

