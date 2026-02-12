#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_abs
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_abs_debug_level = __DBGLVL__;
//module_param_named(isf_flow_abs_debug_level, isf_flow_abs_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_abs_debug_level, "low debug level");
///////////////////////////////////////////////////////////////////////////////

static CHAR portstr[2][4] = {"out","in"};

//init
ISF_RV _audbs_init(struct _ISF_DATA *p_data, UINT32 version)
{
	p_data->h_data = 0;
	return ISF_OK;
}

//verify
ISF_RV _audbs_verify(struct _ISF_DATA *p_data)
{
	return ISF_OK;
}

void debug_read_audbs(AUD_BITSTREAM* p_audbs)
{
}

//load from file
ISF_RV _audbs_load(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
	return ISF_OK;
}

void debug_write_audbs(AUD_BITSTREAM* p_audbs)
{
}

//save to file
ISF_RV _audbs_save(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi) 
{
#if defined (__UITRON)
	#define DUMP_DISK "A:\\"
	#define MAX_FILENAME_LEN	80
#else
	#define DUMP_DISK "/mnt/sd/"
	#define MAX_FILENAME_LEN	80
#endif
	static CHAR filename[MAX_FILENAME_LEN] = {0};
	AUD_BITSTREAM *p_audbs = (AUD_BITSTREAM *)(p_data->desc);
	snprintf(filename,MAX_FILENAME_LEN-1,DUMP_DISK"isf_%s_%s[%d]_c%lld.bsa",
		p_thisunit->unit_name, portstr[pd], pi, p_audbs->count);
		filename[MAX_FILENAME_LEN-1] = 0;
	DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- h=%08x t=%08x%08x (ASTM: %08x %d)"_ANSI_0_"\r\n", 
	    p_thisunit->unit_name, portstr[pd], pi, 
	    p_data->h_data, (UINT32)(p_audbs->timestamp >> 32), (UINT32)(p_audbs->timestamp),
	    p_audbs->DataAddr, p_audbs->DataSize);
	debug_write_open((CHAR *)filename);
	debug_write_audbs(p_audbs);
	debug_write_close();
	DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- %s ok\r\n"_ANSI_0_,
	    p_thisunit->unit_name, portstr[pd], pi, filename); 
	return ISF_OK;
}

//log
ISF_RV _audbs_probe(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi, CHAR* act)
{
    /// probe
    AUD_BITSTREAM *p_audbs = (AUD_BITSTREAM *)(p_data->desc);
	DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] - %s - data -- h=%08x t=%08x%08x (ASTM: %08x %d)"_ANSI_0_"\r\n", 
	    p_thisunit->unit_name, portstr[pd], pi, act,
	    p_data->h_data, (UINT32)(p_audbs->timestamp >> 32), (UINT32)(p_audbs->timestamp),
	    p_audbs->DataAddr, p_audbs->DataSize);
	return ISF_OK;
}

#define ISF_PERF	1

#if ISF_PERF
static UINT64 abps_t1, abps_t2;
static INT32 abps_diff,abps_count=0;
static UINT32 abps_unit;
#endif
/*
	    //reset
		abps_t1 = abps_t2 = 0;
		abps_diff = abps_count = 0;
*/
//perf
static ISF_RV _audbs_perf(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
	#define ONE_SECOND 1000000
	AUD_BITSTREAM *p_audbs = (AUD_BITSTREAM *)(p_data->desc);
	if (abps_count == 0) {
		//abps_t1 = p_data->timestamp; //first time
		abps_unit = 1;
	}
	abps_count+=(p_audbs->DataSize*abps_unit); //each time
	abps_t2 = p_audbs->timestamp;
	abps_diff = abps_t2 - abps_t1;
	if (abps_diff >= ONE_SECOND) {
		UINT32 abps = _isf_div64(((UINT64)abps_count) * ONE_SECOND + (abps_diff>>1), abps_diff);
		abps = abps/1024;
		DBG_DUMP(_ANSI_M_"hd: \"%s\".%s[%d] - perf -- (AudioBs) %u KByte/sec"_ANSI_0_"\r\n", 
		    p_thisunit->unit_name, portstr[pd], pi, abps);
		//reset
		abps_count = 0;
		abps_t1 = abps_t2;
	}
	return ISF_OK;
}

ISF_DATA_CLASS _audbs_base_data =
{
	.this = MAKEFOURCC('A','S','T','M'),
	.base = MAKEFOURCC('C','O','M','M'),
	
	.do_init = _audbs_init,
	.do_verify = _audbs_verify,
	.do_load = _audbs_load,
	.do_save = _audbs_save,
	.do_probe = _audbs_probe,
	.do_perf = _audbs_perf,
};

