#include "../isf_vdoout_api.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_vdoout_dbg.h"
#include "../isf_vdoout_int.h"
#include "kwrap/semaphore.h"

//============================================================================
// Function define
//============================================================================
static int isf_vdoout_seq_printf(const char *fmtstr, ...)
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
extern BOOL _isf_vdoout_api_info(unsigned char argc, char **pargv)
{
	UINT32 dev;
	SEM_WAIT(ISF_VDOOUT_PROC_SEM_ID);
	//dump info of all devices
	debug_log_cb(1); //show hdal version
	for(dev = 0; dev < 2; dev++) {
		UINT32 uid = ISF_UNIT_VDOOUT + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);
		//dump bind, state and param settings of 1 device
		debug_log_cb(uid);
		//dump work status of 1 device
		isf_vdoout_dump_status(isf_vdoout_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_VDOOUT_PROC_SEM_ID);
	return 1;
}
