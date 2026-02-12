#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_afm
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_afm_debug_level = __DBGLVL__;
//module_param_named(isf_flow_afm_debug_level, isf_flow_afm_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_afm_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

static CHAR portstr[2][4] = {"out","in"};

//init
ISF_RV _audfm_init(struct _ISF_DATA *p_data, UINT32 version)
{
	p_data->h_data = 0;
	return ISF_OK;
}

//verify
ISF_RV _audfm_verify(struct _ISF_DATA *p_data)
{
	return ISF_OK;
}

void debug_read_audframe(AUD_FRAME* p_audframe)
{
}

//load from file
ISF_RV _audfm_load(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
	return ISF_OK;
}

void debug_write_audframe(AUD_FRAME* p_audframe)
{
}

//save to file
ISF_RV _audfm_save(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi) 
{
#if defined (__UITRON)
	#define DUMP_DISK "A:\\"
	#define MAX_FILENAME_LEN	80
#else
	#define DUMP_DISK "/mnt/sd/"
	#define MAX_FILENAME_LEN	80
#endif
	static CHAR filename[MAX_FILENAME_LEN] = {0};
	AUD_FRAME *p_audframe = (AUD_FRAME *)(p_data->desc);
	snprintf(filename,MAX_FILENAME_LEN-1,DUMP_DISK"isf_%s_%s[%d]_%d_%d_%d_c%lld.aud", 
		p_thisunit->unit_name, portstr[pd], pi,  
		p_audframe->bit_width, p_audframe->sound_mode, p_audframe->sample_rate, p_audframe->count);
	filename[MAX_FILENAME_LEN-1] = 0;
	DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- h=%08x t=%08x%08x (ARAW: %d.%d.%d %08x %08x %d)"_ANSI_0_"\r\n", 
	    p_thisunit->unit_name, portstr[pd], pi, 
	    p_data->h_data, (UINT32)(p_audframe->timestamp >> 32), (UINT32)(p_audframe->timestamp),
	    p_audframe->bit_width, p_audframe->sound_mode, p_audframe->sample_rate, p_audframe->addr[0], p_audframe->addr[1], p_audframe->size);
	debug_write_open((CHAR *)filename);
	debug_write_audframe(p_audframe);
	debug_write_close();
	DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- %s ok\r\n"_ANSI_0_,
	    p_thisunit->unit_name, portstr[pd], pi, filename); 
	return ISF_OK;
}

//log
ISF_RV _audfm_probe(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi, CHAR* act)
{
    /// probe
    AUD_FRAME *p_audframe = (AUD_FRAME *)(p_data->desc);
	DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] - %s - data -- h=%08x t=%08x%08x (AFRM: %d.%d.%d %08x %08x %d)"_ANSI_0_"\r\n", 
	    p_thisunit->unit_name, portstr[pd], pi, act,
	    p_data->h_data, (UINT32)(p_audframe->timestamp >> 32), (UINT32)(p_audframe->timestamp),
	    p_audframe->bit_width, p_audframe->sound_mode, p_audframe->sample_rate, p_audframe->addr[0], p_audframe->addr[1], p_audframe->size);
	return ISF_OK;
}

#define ISF_PERF	1

#if ISF_PERF
static UINT64 sps_t1, sps_t2;
static INT32 sps_diff,sps_count=0;
static UINT32 sps_unit;
#endif
/*
	    //reset
		sps_t1 = sps_t2 = 0;
		sps_count=0;
*/		
//perf
static ISF_RV _audfm_perf(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
	#define ONE_SECOND 1000000
	AUD_FRAME *p_audframe = (AUD_FRAME *)(p_data->desc);
	if (sps_count == 0) {
		//sps_t1 = p_data->timestamp; //first time
		sps_unit = (AUD_CHANNEL_COUNT(p_audframe->sound_mode) << (p_audframe->bit_width));
	}
	sps_count+=(p_audframe->size/sps_unit); //each time
	sps_t2 = p_audframe->timestamp;
	sps_diff = sps_t2 - sps_t1;
	if (sps_diff >= ONE_SECOND) {
		UINT32 sps = _isf_div64(((UINT64)sps_count) * ONE_SECOND + (sps_diff>>1), sps_diff);
		//sps = sps;
		DBG_DUMP(_ANSI_M_"hd: \"%s\".%s[%d] - perf -- (Audio) %u KSample/sec"_ANSI_0_"\r\n", 
		    p_thisunit->unit_name, portstr[pd], pi, sps/1000);
		//reset
		sps_count = 0;
		sps_t1 = sps_t2;
	}
	return ISF_OK;
}

ISF_DATA_CLASS _audfm_base_data =
{
	.this = MAKEFOURCC('A','F','R','M'),
	.base = MAKEFOURCC('C','O','M','M'),
	
	.do_init = _audfm_init,
	.do_verify = _audfm_verify,
	.do_load = _audfm_load,
	.do_save = _audfm_save,
	.do_probe = _audfm_probe,
	.do_perf = _audfm_perf,
};

