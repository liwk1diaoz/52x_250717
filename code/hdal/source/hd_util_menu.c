/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_videoenc_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_util.h"
#include "hd_debug_menu.h"
#include "hd_util.h"
#define HD_MODULE_NAME HD_UTIL
#include "hd_int.h"
#include <string.h>

static int hd_util_show_status_p(void)
{
	printf("------------------------- UTIL ------------------------------------------------\r\n");
	printf("current time (ms)\r\n");
	printf("%-10d\r\n",	(int)hd_gettime_ms());

	return 0;
}

static HD_DBG_MENU util_debug_menu[] = {
	{0x01, "dump status", hd_util_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_util_menu(void)
{
	return hd_debug_menu_entry_p(util_debug_menu, "UTIL");
}



