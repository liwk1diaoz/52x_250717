/*
 *   @file   vg_log_pif.c
 *
 *   @brief  The Log System to record debug information.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */
#include <comm/log.h>
#define __MODULE__ vg_log
#include <kwrap/debug.h>

static void printm_if(const char *fmt, ...)
{
	printm(NULL, fmt);
}

void printm_pif_init(void)
{
	debug_kmsg_register(printm_if);
}

void printm_pif_uninit(void)
{
	debug_kmsg_register(NULL);
}
