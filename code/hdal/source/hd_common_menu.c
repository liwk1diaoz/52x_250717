/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_debug_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hdal.h"
#include "hd_version.h"
#include "hd_debug_menu.h"
#include <string.h>

#define HD_MODULE_NAME HD_COMMON

#define DBG_ERR(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _ERR)("\033[1;31m" fmtstr "\033[0m", ##args)
#define DBG_WRN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _WRN)("\033[1;33m" fmtstr "\033[0m", ##args)
#define DBG_IND(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _IND)(fmtstr, ##args)
#define DBG_DUMP(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _MSG)(fmtstr, ##args)
#define DBG_FUNC_BEGIN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("BEGIN: " fmtstr, ##args)
#define DBG_FUNC_END(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("END: " fmtstr, ##args)

static int hd_common_dump_flow_p(void)
{
	system("cat /proc/hdal/flow");
	return 0;
}

static int hd_common_dump_err_p(void)
{
	system("cat /proc/hdal/err");
	return 0;
}

static int hd_common_mem_show_info_p(void)
{
	system("cat /proc/hdal/comm/info");
	return 0;
}

static int hd_common_show_task_p(void)
{
	system("cat /proc/hdal/comm/task");
	return 0;
}

static int hd_common_show_sem_p(void)
{
	system("cat /proc/hdal/comm/sem");
	return 0;
}

static int hd_common_mem_dump_getblk_fail_p(void)
{
	system("echo nvtmpp showmsg 1 > /proc/hdal/comm/cmd");
	return 0;
}

static int hd_common_mem_check_corruption_p(void)
{
	system("echo nvtmpp chkmem_corrupt 1 > /proc/hdal/comm/cmd");
	return 0;
}

static int hd_common_dram1_dma_dump_usage_p(void)
{
	system("cat /proc/nvt_drv_sys/dram1_info");
	return 0;
}

static int hd_common_dram2_dma_dump_usage_p(void)
{
	system("cat /proc/nvt_drv_sys/dram2_info");
	return 0;
}

static int hd_common_version_p(void)
{
	DBG_DUMP("HDAL: Version: v%x.%x.%x\r\n",
		(HDAL_VERSION&0xF00000) >> 20,
		(HDAL_VERSION&0x0FFFF0) >> 4,
		(HDAL_VERSION&0x00000F));
	return 0;
}


static HD_DBG_MENU common_debug_menu[] = {
	{0x01, "dump hdal version",                               hd_common_version_p,                      TRUE},
	{0x02, "dump hdal flow setting",                          hd_common_dump_flow_p,                    TRUE},
	{0x03, "dump media memory info",                          hd_common_mem_show_info_p,                TRUE},
	{0x04, "dump media memory info when get block fail",      hd_common_mem_dump_getblk_fail_p,         TRUE},
	{0x05, "check hdal memory block corruption periodically", hd_common_mem_check_corruption_p,         TRUE},
	{0x06, "dump media tasks info",                           hd_common_show_task_p,                    TRUE},
	{0x07, "dump media semaphores info",                      hd_common_show_sem_p,                     TRUE},
	{0x08, "dump dram 1 dma usage",                           hd_common_dram1_dma_dump_usage_p,         TRUE},
	{0x09, "dump dram 2 dma usage",                           hd_common_dram2_dma_dump_usage_p,         TRUE},
	{0x0a, "dump hdal error",                                 hd_common_dump_err_p,                     TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_common_menu(void)
{
	return hd_debug_menu_entry_p(common_debug_menu, "COMMON");
}



