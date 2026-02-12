#include "../include/isf_audout_dbg.h"
#include "../isf_audout_int.h"

//============================================================================
// Function define
//============================================================================
static int isf_audout_seq_printf(const char *fmtstr, ...)
{
	char    buf[512];
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);
	if (len > 0)
		DBG_DUMP(buf);
	return 0;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
extern void debug_log_cb(UINT32 uid);
extern BOOL _isf_audout_api_info(unsigned char argc, char **pargv)
{
	UINT32 dev;

	SEM_WAIT(ISF_AUDOUT_PROC_SEM_ID);

	debug_log_cb(1);

	for(dev = 0; dev < 2; dev++) {
		UINT32 uid = ISF_UNIT_AUDOUT + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);

		debug_log_cb(uid);
		isf_audout_dump_status(isf_audout_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_AUDOUT_PROC_SEM_ID);
	return 1;
}

