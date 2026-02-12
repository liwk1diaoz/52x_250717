/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_debug_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_util.h"
#include "hd_debug_menu.h"
#define HD_MODULE_NAME HD_OSG
#include "hd_int.h"
#include <string.h>

static int hd_osg_mem_show_videoprocess(void)
{
	system("cat /proc/osg/videoprocess");
	return 0;
}

static int hd_osg_mem_show_videoenc(void)
{
	system("cat /proc/osg/videoenc");
	return 0;
}

static int hd_osg_mem_show_videoout(void)
{
	system("cat /proc/osg/videoout");
	return 0;
}

static HD_DBG_MENU osg_debug_menu[] = {
	{0x01, "dump videoprocess osg", hd_osg_mem_show_videoprocess,  TRUE},
	{0x02, "dump videoenc osg",     hd_osg_mem_show_videoenc,      TRUE},
	{0x03, "dump videoout osg",     hd_osg_mem_show_videoout,      TRUE},
	// escape muse be last
	{-1,   "",                      NULL,                          FALSE},
};

HD_RESULT hd_osg_menu(void)
{
	return hd_debug_menu_entry_p(osg_debug_menu, "OSG");
}
