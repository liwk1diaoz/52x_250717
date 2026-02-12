#include "isf_flow_int.h"
//#include "vcodec/media_def.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_vbs
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_vbs_debug_level = __DBGLVL__;
//module_param_named(isf_flow_vbs_debug_level, isf_flow_vbs_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_vbs_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

static CHAR portstr[2][4] = {"out","in"};

//init
ISF_RV _vdobs_init(struct _ISF_DATA *p_data, UINT32 version)
{
	p_data->h_data = 0;
	return ISF_OK;
}

//verify
ISF_RV _vdobs_verify(struct _ISF_DATA *p_data)
{
	return ISF_OK;
}

void debug_read_vdobs(VDO_BITSTREAM* p_vdobs)
{
}

//load from file
ISF_RV _vdobs_load(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
	return ISF_OK;
}

void debug_write_vdobs(VDO_BITSTREAM* p_vdobs)
{
	UINT32 bufaddr, bufsize;

	if (!p_vdobs) {
		DBG_ERR("vdobs is null!\r\n");
		return;
	}
	if (p_vdobs->sign != MAKEFOURCC('V','S','T','M')) {
		DBG_ERR("vdobs is invalid!\r\n");
		return;
	}

	DBG_MSG("[dump]p_vdobs w = %d, h = %d, codec = %d, framenum = %lld\r\n",
		p_vdobs->Width, p_vdobs->Height, p_vdobs->CodecType, p_vdobs->count);
	DBG_MSG("[dump]p_vdobs frame_type = %d, IsIDR2Cut = %d, SVCSize = %d, TemporalId = %d, IsKey = %d\r\n",
		p_vdobs->resv[6], p_vdobs->resv[7], p_vdobs->resv[8], p_vdobs->resv[9], p_vdobs->resv[10]);

	switch (p_vdobs->CodecType) {
	case MEDIAVIDENC_MJPG:
		// BS
		{
			bufaddr   = p_vdobs->DataAddr;
			bufsize   = p_vdobs->DataSize;
			DBG_MSG("[BS] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
			debug_write_data((void*)bufaddr, bufsize);
		}
		break;
	case MEDIAVIDENC_H264:
		// SPS
		{
			bufaddr   = p_vdobs->resv[0];
			bufsize   = p_vdobs->resv[1];
			DBG_MSG("[SPS] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
			debug_write_data((void*)bufaddr, bufsize);
		}
		// PPS
		{
			bufaddr   = p_vdobs->resv[2];
			bufsize   = p_vdobs->resv[3];
			DBG_MSG("[PPS] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
			debug_write_data((void*)bufaddr, bufsize);
		}
		// BS
		{
			bufaddr   = p_vdobs->DataAddr;
			bufsize   = p_vdobs->DataSize;
			DBG_MSG("[BS] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
			debug_write_data((void*)bufaddr, bufsize);
		}
		break;
	case MEDIAVIDENC_H265:
		// VPS
		{
			bufaddr   = p_vdobs->resv[4];
			bufsize   = p_vdobs->resv[5];
			DBG_MSG("[VPS] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
			debug_write_data((void*)bufaddr, bufsize);
		}
		// SPS
		{
			bufaddr   = p_vdobs->resv[0];
			bufsize   = p_vdobs->resv[1];
			DBG_MSG("[SPS] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
			debug_write_data((void*)bufaddr, bufsize);
		}
		// PPS
		{
			bufaddr   = p_vdobs->resv[2];
			bufsize   = p_vdobs->resv[3];
			DBG_MSG("[PPS] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
			debug_write_data((void*)bufaddr, bufsize);
		}
		// BS
		{
			bufaddr   = p_vdobs->DataAddr;
			bufsize   = p_vdobs->DataSize;
			DBG_MSG("[BS] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
			debug_write_data((void*)bufaddr, bufsize);
		}
		break;
	default:
		DBG_ERR("not support codec_type %08x!\r\n", p_vdobs->CodecType);
		break;
	}
}

//save to file
ISF_RV _vdobs_save(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
#if defined (__UITRON)
	#define DUMP_DISK "A:\\"
	#define MAX_FILENAME_LEN	80
#else
	#define DUMP_DISK "/mnt/sd/"
	#define MAX_FILENAME_LEN	80
#endif
	static CHAR filename[MAX_FILENAME_LEN] = {0};
	VDO_BITSTREAM *p_vdobs = (VDO_BITSTREAM *)(p_data->desc);
	snprintf(filename,MAX_FILENAME_LEN-1,DUMP_DISK"isf_%s_%s[%d]_c%lld.bsv",
		p_thisunit->unit_name, portstr[pd], pi, p_vdobs->count);
	filename[MAX_FILENAME_LEN-1] = 0;
	DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- h=%08x t=%08x%08x (VSTM: %08x %d)"_ANSI_0_"\r\n",
	    p_thisunit->unit_name, portstr[pd], pi,
	    p_data->h_data, (UINT32)(p_vdobs->timestamp >> 32), (UINT32)(p_vdobs->timestamp),
	    p_vdobs->DataAddr, p_vdobs->DataSize);
	debug_write_open((CHAR *)filename);
	debug_write_vdobs(p_vdobs);
	debug_write_close();
	DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- %s ok\r\n"_ANSI_0_,
	    p_thisunit->unit_name, portstr[pd], pi, filename);
	return ISF_OK;
}

//log
ISF_RV _vdobs_probe(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi, CHAR* act)
{
    /// probe
    VDO_BITSTREAM *p_vdobs = (VDO_BITSTREAM *)(p_data->desc);
	DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] - %s - data -- h=%08x t=%08x%08x (VSTM: %08x %d)"_ANSI_0_"\r\n",
	    p_thisunit->unit_name, portstr[pd], pi, act,
	    p_data->h_data, (UINT32)(p_vdobs->timestamp >> 32), (UINT32)(p_vdobs->timestamp),
	    p_vdobs->DataAddr, p_vdobs->DataSize);
	return ISF_OK;
}

#define ISF_PERF	1

#if ISF_PERF
static UINT64 vbps_t1, vbps_t2;
static INT32 vbps_diff,vbps_count=0;
static UINT32 vbps_unit;
#endif
/*
	    //reset
		vbps_t1 = vbps_t2 = 0;
		vbps_count=0;
*/
//perf
static ISF_RV _vdobs_perf(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
	#define ONE_SECOND 1000000
	VDO_BITSTREAM *p_vdobs = (VDO_BITSTREAM *)(p_data->desc);
	if (vbps_count == 0) {
	    //vbps_t1 = p_data->timestamp; //first time
	    vbps_unit = 1;
	}
	vbps_count+=(p_vdobs->DataSize*vbps_unit); //each time
	vbps_t2 = p_vdobs->timestamp;
	vbps_diff = vbps_t2 - vbps_t1;
	if (vbps_diff >= ONE_SECOND) {
		UINT32 vbps = _isf_div64(((UINT64)vbps_count) * ONE_SECOND + (vbps_diff>>1), vbps_diff);
		vbps = vbps/1024;
		DBG_DUMP(_ANSI_M_"hd: \"%s\".%s[%d] - perf -- (VideoBs) %u KByte/sec"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, vbps);
		//reset
		vbps_count = 0;
		vbps_t1 = vbps_t2;
	}
	return ISF_OK;
}

ISF_DATA_CLASS _vdobs_base_data =
{
	.this = MAKEFOURCC('V','S','T','M'),
	.base = MAKEFOURCC('C','O','M','M'),

	.do_init = _vdobs_init,
	.do_verify = _vdobs_verify,
	.do_load = _vdobs_load,
	.do_save = _vdobs_save,
	.do_probe = _vdobs_probe,
	.do_perf = _vdobs_perf,
};

