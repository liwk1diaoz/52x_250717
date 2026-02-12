
#include "kwrap/semaphore.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "nmediarec_vdoenc.h"
//#include "kflow_videoenc/isf_vdoenc.h"

///////////////////////////////////////////////////////////////////////////////
#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          isf_vdoenc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"
extern unsigned int isf_vdoenc_debug_level;
///////////////////////////////////////////////////////////////////////////////

//============================================================================
// Function define
//============================================================================
static int isf_vdoenc_seq_printf(const char *fmtstr, ...)
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
extern SEM_HANDLE ISF_VDOENC_PROC_SEM_ID;
extern void isf_vdoenc_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit);
extern BOOL _isf_vdoenc_api_info(CHAR *strCmd)
{
	#define ISF_VDOENC_DEV_NUM     1
	UINT32 dev;
	SEM_WAIT(ISF_VDOENC_PROC_SEM_ID);
	//dump info of all devices
	debug_log_cb(1); //show hdal version
	for(dev = 0; dev < ISF_VDOENC_DEV_NUM; dev++) {
		UINT32 uid = ISF_UNIT_VDOENC + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);
		//dump bind, state and param settings of 1 device
		debug_log_cb(uid);
		//dump work status of 1 device
		isf_vdoenc_dump_status(isf_vdoenc_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_VDOENC_PROC_SEM_ID);
	return 1;
}

extern BOOL _isf_vdoenc_api_top(CHAR *strCmd)
{
	#define ISF_VDOENC_DEV_NUM     1
	UINT32 dev;
	SEM_WAIT(ISF_VDOENC_PROC_SEM_ID);
	//dump info of all devices
	for(dev = 0; dev < ISF_VDOENC_DEV_NUM; dev++) {
		//dump bind, state and param settings of 1 device
		//dump work status of 1 device
		NMR_VdoEnc_Top(isf_vdoenc_seq_printf);
	}
	SEM_SIGNAL(ISF_VDOENC_PROC_SEM_ID);
	return 1;
}

