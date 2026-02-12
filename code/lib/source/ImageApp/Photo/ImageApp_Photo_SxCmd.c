#include "ImageApp/ImageApp_Photo.h"
#include "ImageApp_Photo_int.h"
#include <kwrap/sxcmd.h>
#include <kwrap/stdio.h>
#include "kwrap/cmdsys.h"
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          IA_PHOTO_SXCMD
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////



static BOOL cmd_disp_link(unsigned char argc, char **argv)
{
	UINT32 disp_id;
	UINT32 action;
	UINT32 allow_pull;

	sscanf_s(argv[0], "%d %d %d", &disp_id,&action,&allow_pull);
	DBG_DUMP("disp_id=%d, action=%d, allow_pull=%d\r\n",disp_id,action,allow_pull);

	//ImageApp_Photo_UpdateImgLinkForDisp((PHOTO_DISP_ID)disp_id, action, (BOOL)allow_pull);
	return TRUE;

}
static BOOL cmd_wifi_link(unsigned char argc, char **argv)
{
	UINT32 strm_id;
	UINT32 action;
	UINT32 allow_pull;

	sscanf_s(argv[0], "%d %d %d", &strm_id,&action,&allow_pull);
	DBG_DUMP("strm_id=%d, action=%d, allow_pull=%d\r\n",strm_id,action,allow_pull);

	//ImageApp_Photo_UpdateImgLinkForStrm((PHOTO_STRM_ID)strm_id, action, (BOOL)allow_pull);
	return TRUE;

}
static BOOL cmd_trigger_cap(unsigned char argc, char **argv)
{
	DBG_DUMP("trigger cap\r\n");
	ImageApp_Photo_CapStart();
	return TRUE;

}

static BOOL cmd_set_format_free(unsigned char argc, char **argv)
{
	UINT32 is_format_free = TRUE;
	ImageApp_Photo_SetFormatFree(is_format_free);
	return TRUE;

}

static SXCMD_BEGIN(iaphoto_cmd_tbl, "iaphoto_cmd_tbl")
SXCMD_ITEM("displink %",    cmd_disp_link,  "change disp link")
SXCMD_ITEM("wifilink %",    cmd_wifi_link,  "change wifi link")
SXCMD_ITEM("cap",    cmd_trigger_cap,  "trigger cap")
SXCMD_ITEM("setfmtfree",    cmd_set_format_free,  "set format free")
SXCMD_END()

static int iaphoto_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(iaphoto_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "photo (ImageApp_Photo)");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", iaphoto_cmd_tbl[loop].p_name, iaphoto_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(iaphoto, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(iaphoto_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		iaphoto_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], iaphoto_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = iaphoto_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	return 0;
}


// EOF
