#include "../include/isf_audcap_dbg.h"
#include "../isf_audcap/isf_audcap_int.h"

//============================================================================
// Function define
//============================================================================
static int isf_audcap_seq_printf(const char *fmtstr, ...)
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
extern BOOL _isf_audcap_api_info(unsigned char argc, char **pargv)
{
	#define ISF_AUDCAP_DEV_NUM     1

	UINT32 dev;
	SEM_WAIT(ISF_AUDCAP_PROC_SEM_ID);
	//dump info of all devices
	debug_log_cb(1); //show hdal version
	for(dev = 0; dev < ISF_AUDCAP_DEV_NUM; dev++) {
		UINT32 uid = ISF_UNIT_AUDCAP + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);
		//dump bind, state and param settings of 1 device
		debug_log_cb(uid);
		//dump work status of 1 device
		isf_audcap_dump_status(isf_audcap_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_AUDCAP_PROC_SEM_ID);
	return 1;
}

