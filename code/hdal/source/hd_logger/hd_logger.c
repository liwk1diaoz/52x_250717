/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_logger.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_type.h"
#include "hd_logger.h"
#include "hd_logger_p.h"
#include <stdarg.h>

#define CFG_USE_KERNEL_PRINTF 0

unsigned int g_hd_mask_err = (unsigned int)-1;
unsigned int g_hd_mask_wrn = (unsigned int)-1;
unsigned int g_hd_mask_ind = (unsigned int)-1;
unsigned int g_hd_mask_msg = (unsigned int)-1;
unsigned int g_hd_mask_func = (unsigned int)-1;

void hd_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
#if (CFG_USE_KERNEL_PRINTF)
	vprintm_p(fmt, ap);
#else
	vprintf(fmt, ap);
#endif
	va_end(ap);
}

