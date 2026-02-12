#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_vfm
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_vfm_debug_level = __DBGLVL__;
//module_param_named(isf_flow_vfm_debug_level, isf_flow_vfm_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_vfm_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

static CHAR portstr[2][4] = {"out","in"};

//init
ISF_RV _vdofm_init(struct _ISF_DATA *p_data, UINT32 version)
{
	p_data->h_data = 0;
	return ISF_OK;
}

//verify
ISF_RV _vdofm_verify(struct _ISF_DATA *p_data)
{
	return ISF_OK;
}

void debug_read_vdoframe(VDO_FRAME* p_vdoframe)
{
}

//load from file
ISF_RV _vdofm_load(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
	return ISF_OK;
}

void debug_write_vdoframe(VDO_FRAME* p_vdoframe)
{
	UINT32 bufaddr, bufsize;

	if (!p_vdoframe) {
		DBG_ERR("vdoframe is null!\r\n");
		return;
	}
	if (p_vdoframe->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("vdoframe is invalid!\r\n");
		return;
	}

	DBG_MSG("[dump]p_vdoframe w = %d, h = %d, fmt = %08x\r\n", 
		p_vdoframe->size.w, p_vdoframe->size.h, (UINT32)(p_vdoframe->pxlfmt));
	DBG_MSG("[dump]p_vdoframe pw = %d %d %d %d, ph = %d %d %d %d\r\n",
		p_vdoframe->pw[0], p_vdoframe->pw[1], p_vdoframe->pw[2], p_vdoframe->pw[3], 
		p_vdoframe->ph[0], p_vdoframe->ph[1], p_vdoframe->ph[2], p_vdoframe->ph[3]);
	DBG_MSG("[dump]p_vdoframe addr = 0x%08X 0x%08X 0x%08X 0x%08X, loff = %u %u %u %u\r\n",
		p_vdoframe->addr[0], p_vdoframe->addr[1], p_vdoframe->addr[2], p_vdoframe->addr[3], 
		p_vdoframe->loff[0], p_vdoframe->loff[1], p_vdoframe->loff[2], p_vdoframe->loff[3]);

	switch (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt)) {
	case VDO_PXLFMT_CLASS_YUV:
		switch ((UINT32)(p_vdoframe->pxlfmt)) {
		case VDO_PXLFMT_YUV444_PLANAR:
		case VDO_PXLFMT_YUV422_PLANAR:
		case VDO_PXLFMT_YUV420_PLANAR:
			// Y
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_Y];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_Y] * p_vdoframe->ph[VDO_PINDEX_Y]);
				DBG_MSG("[Y] bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			// U
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_U];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_U] * p_vdoframe->ph[VDO_PINDEX_U]);
				DBG_MSG("[U] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			// V
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_V];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_V] * p_vdoframe->ph[VDO_PINDEX_V]);
				DBG_MSG("[V] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			break;
		case VDO_PXLFMT_YUV422:
		case VDO_PXLFMT_YUV420:
		case VDO_PXLFMT_YUV422|VDO_PIX_YCC_BT601:
		case VDO_PXLFMT_YUV420|VDO_PIX_YCC_BT601:
		case VDO_PXLFMT_YUV422|VDO_PIX_YCC_BT709:
		case VDO_PXLFMT_YUV420|VDO_PIX_YCC_BT709:
			// Y
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_Y];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_Y] * p_vdoframe->ph[VDO_PINDEX_Y]);
				DBG_MSG("[Y] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			// UV packed
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_UV];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_UV] * p_vdoframe->ph[VDO_PINDEX_UV]);
				DBG_MSG("[UV] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			break;
		case VDO_PXLFMT_YUV400:
			// Y
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_Y];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_Y] * p_vdoframe->ph[VDO_PINDEX_Y]);
				DBG_MSG("[Y] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			break;

		default:
			DBG_ERR("not support pxlfmt %08x!\r\n", p_vdoframe->pxlfmt);
			break;
		}
		break;
	case VDO_PXLFMT_CLASS_NVX:
		switch (p_vdoframe->pxlfmt) {
		case VDO_PXLFMT_YUV420_NVX1_H264:
		case VDO_PXLFMT_YUV420_NVX1_H265:
			// NVX1
			// Y
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_Y];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_Y] * p_vdoframe->ph[VDO_PINDEX_Y]);
				DBG_MSG("[NVX1-Y] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			// UV packed
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_UV];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_UV] * p_vdoframe->ph[VDO_PINDEX_UV]);
				DBG_MSG("[NVX1-UV] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			break;
		case VDO_PXLFMT_YUV420_NVX2:
			// NVX2
			// Y
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_Y];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_Y] * p_vdoframe->ph[VDO_PINDEX_Y]);
				DBG_MSG("[NVX2-Y] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			// UV packed
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_UV];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_UV] * p_vdoframe->ph[VDO_PINDEX_UV]);
				DBG_MSG("[NVX2-UV] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			break;

		default:
			DBG_ERR("not support pxlfmt %08x!\r\n", p_vdoframe->pxlfmt);
			break;
		}
		break;
	case VDO_PXLFMT_CLASS_RAW:
			// RAW
			{
				UINT32 p = VDO_PXLFMT_PLANE(p_vdoframe->pxlfmt);
				UINT32 i;
				for(i=0; i<p; i++) {
					bufaddr   = p_vdoframe->addr[VDO_PINDEX_RAW+i];
					bufsize   = (p_vdoframe->loff[VDO_PINDEX_RAW+i] * p_vdoframe->ph[VDO_PINDEX_RAW+i]);
					DBG_MSG("[RAW%d] bufaddr = 0x%08x, bufsize = %u\r\n", i, bufaddr, bufsize);
					debug_write_data((void*)bufaddr, bufsize);
				}
			}
		break;
	case VDO_PXLFMT_CLASS_NRX:
			// NRX
			{
				UINT32 p = VDO_PXLFMT_PLANE(p_vdoframe->pxlfmt);
				UINT32 i;
				for(i=0; i<p; i++) {
					bufaddr   = p_vdoframe->addr[VDO_PINDEX_RAW+i];
					bufsize   = (p_vdoframe->loff[VDO_PINDEX_RAW+i] * p_vdoframe->ph[VDO_PINDEX_RAW+i]);
					DBG_MSG("[NRX%d] bufaddr = 0x%08x, bufsize = %u\r\n", i, bufaddr, bufsize);
					debug_write_data((void*)bufaddr, bufsize);
				}
			}
		break;
#if defined(_BSP_NA51055_)
	case 0xe:
		switch (p_vdoframe->pxlfmt) {
		case VDO_PXLFMT_520_IR8:
		case VDO_PXLFMT_520_IR16:
			// IR
			{
				bufaddr   = p_vdoframe->addr[VDO_PINDEX_Y];
				bufsize   = (p_vdoframe->loff[VDO_PINDEX_Y] * p_vdoframe->ph[VDO_PINDEX_Y]);
				DBG_MSG("[IR] saved. bufaddr = 0x%08x, bufsize = %u\r\n", bufaddr, bufsize);
				debug_write_data((void*)bufaddr, bufsize);
			}
			break;

		default:
			DBG_ERR("not support pxlfmt %08x!\r\n", p_vdoframe->pxlfmt);
			break;
		}
		break;
#endif
	default:
		DBG_ERR("not support pxlfmt %08x!\r\n", p_vdoframe->pxlfmt);
		break;
	}
}

//save to file
ISF_RV _vdofm_save(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi) 
{
#if defined (__UITRON)
	#define DUMP_DISK "A:\\"
	#define MAX_FILENAME_LEN	80
#else
	#define DUMP_DISK "/mnt/sd/"
	#define MAX_FILENAME_LEN	80
#endif
	static CHAR filename[MAX_FILENAME_LEN] = {0};
    	VDO_FRAME *p_vdoframe = (VDO_FRAME *)(p_data->desc);
    	if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_YUV) {
		snprintf(filename,MAX_FILENAME_LEN-1,DUMP_DISK"isf_%s_%s[%d]_%08x_%d_%d_%d_c%lld.vdo", 
			p_thisunit->unit_name, portstr[pd], pi, 
			p_vdoframe->pxlfmt, p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->loff[0], p_vdoframe->count);
		filename[MAX_FILENAME_LEN-1] = 0;
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- h=%08x t=%08x%08x (YUV: %dx%d.%08x %08x %08x %d %d)"_ANSI_0_"\r\n", 
		    p_thisunit->unit_name, portstr[pd], pi, 
		    p_data->h_data, (UINT32)(p_vdoframe->timestamp >> 32), (UINT32)(p_vdoframe->timestamp),
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt, p_vdoframe->addr[0], p_vdoframe->addr[1], p_vdoframe->loff[0], p_vdoframe->loff[1]);
		debug_write_open((CHAR *)filename);
		debug_write_vdoframe(p_vdoframe);
		debug_write_close();
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- %s ok\r\n"_ANSI_0_,
		    p_thisunit->unit_name, portstr[pd], pi, filename); 
    	} else if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_NVX) {
		snprintf(filename,MAX_FILENAME_LEN-1,DUMP_DISK"isf_%s_%s[%d]_%08x_%d_%d_c%lld.nvx", 
			p_thisunit->unit_name, portstr[pd], pi, 
			p_vdoframe->pxlfmt, p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->count);
		filename[MAX_FILENAME_LEN-1] = 0;
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- h=%08x t=%08x%08x (NVX: %dx%d.%08x)"_ANSI_0_"\r\n", 
		    p_thisunit->unit_name, portstr[pd], pi, 
		    p_data->h_data, (UINT32)(p_vdoframe->timestamp >> 32), (UINT32)(p_vdoframe->timestamp),
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt);
		debug_write_open((CHAR *)filename);
		debug_write_vdoframe(p_vdoframe);
		debug_write_close();
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- %s ok"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, filename); 
    } else if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_RAW) {
		snprintf(filename,MAX_FILENAME_LEN-1,DUMP_DISK"isf_%s_%s[%d]_%08x_%d_%d_c%lld.raw", 
			p_thisunit->unit_name, portstr[pd], pi, 
			p_vdoframe->pxlfmt, p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->count);
		filename[MAX_FILENAME_LEN-1] = 0;
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- h=%08x t=%08x%08x (RAW: %dx%d.%08x)"_ANSI_0_"\r\n", 
		    p_thisunit->unit_name, portstr[pd], pi, 
		    p_data->h_data, (UINT32)(p_vdoframe->timestamp >> 32), (UINT32)(p_vdoframe->timestamp),
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt);
		debug_write_open((CHAR *)filename);
		debug_write_vdoframe(p_vdoframe);
		debug_write_close();
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- %s ok"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, filename); 
    } else if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
		snprintf(filename,MAX_FILENAME_LEN-1,DUMP_DISK"isf_%s_%s[%d]_%08x_%d_%d_c%lld.nrx", 
			p_thisunit->unit_name, portstr[pd], pi, 
			p_vdoframe->pxlfmt, p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->count);
		filename[MAX_FILENAME_LEN-1] = 0;
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- h=%08x t=%08x%08x (NRX: %dx%d.%08x)"_ANSI_0_"\r\n", 
		    p_thisunit->unit_name, portstr[pd], pi, 
		    p_data->h_data, (UINT32)(p_vdoframe->timestamp >> 32), (UINT32)(p_vdoframe->timestamp),
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt);
		debug_write_open((CHAR *)filename);
		debug_write_vdoframe(p_vdoframe);
		debug_write_close();
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- %s ok"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, filename); 
#if defined(_BSP_NA51055_)
    } else if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == 0xe) {
		snprintf(filename,MAX_FILENAME_LEN-1,DUMP_DISK"isf_%s_%s[%d]_%08x_%d_%d_c%lld.xda", 
			p_thisunit->unit_name, portstr[pd], pi, 
			p_vdoframe->pxlfmt, p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->count);
		filename[MAX_FILENAME_LEN-1] = 0;
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- h=%08x t=%08x%08x (XDA: %dx%d.%08x)"_ANSI_0_"\r\n", 
		    p_thisunit->unit_name, portstr[pd], pi, 
		    p_data->h_data, (UINT32)(p_vdoframe->timestamp >> 32), (UINT32)(p_vdoframe->timestamp),
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt);
		debug_write_open((CHAR *)filename);
		debug_write_vdoframe(p_vdoframe);
		debug_write_close();
		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] - save -- %s ok"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, filename); 
#endif
    }
	return ISF_OK;
}

//log
ISF_RV _vdofm_probe(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi, CHAR* act)
{
    /// probe
    VDO_FRAME *p_vdoframe = (VDO_FRAME *)(p_data->desc);
    if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_YUV) {
		DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] - %s - data -- h=%08x t=%llu (YUV: %dx%d.%08x %08x %08x %d %d)"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, act,
		    p_data->h_data, p_vdoframe->timestamp,
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt, p_vdoframe->addr[0], p_vdoframe->addr[1], p_vdoframe->loff[0], p_vdoframe->loff[1]);
    } else if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_NVX) {
		DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] - %s - data -- h=%08x t=%llu (NVX: %dx%d.%08x)"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, act,
		    p_data->h_data, p_vdoframe->timestamp,
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt);
    } else if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_RAW) {
		DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] - %s - data -- h=%08x t=%llu (RAW: %dx%d.%08x)"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, act,
		    p_data->h_data, p_vdoframe->timestamp,
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt);
    } else if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
		DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] - %s - data -- h=%08x t=%llu (NRX: %dx%d.%08x)"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, act,
		    p_data->h_data, p_vdoframe->timestamp,
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt);
#if defined(_BSP_NA51055_)
    } else if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == 0xe) {
		DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] - %s - data -- h=%08x t=%llu (XDA: %dx%d.%08x)"_ANSI_0_"\r\n",
		    p_thisunit->unit_name, portstr[pd], pi, act,
		    p_data->h_data, p_vdoframe->timestamp,
		    p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pxlfmt);
#endif
    }
	return ISF_OK;
}

#define ISF_PERF	1

#if ISF_PERF
static UINT64 fps_t1, fps_t2;
static INT32 fps_diff,fps_count=0;
#endif
/*
	    //reset
		fps_t1 = fps_t2 = 0;
		fps_count=0;
*/
//perf
static ISF_RV _vdofm_perf(struct _ISF_DATA *p_data, ISF_UNIT *p_thisunit, INT32 pd, INT32 pi)
{
	#define ONE_SECOND 1000000
	VDO_FRAME *p_vdoframe = (VDO_FRAME *)(p_data->desc);
	//if (fps_count == 0)
	//    fps_t1 = p_data->timestamp; //first time
	fps_count++; //each time
	fps_t2 = p_vdoframe->timestamp;
	fps_diff = fps_t2 - fps_t1;
	if (fps_diff >= ONE_SECOND) {
		UINT32 fps = _isf_div64(((UINT64)fps_count) * ONE_SECOND + (fps_diff>>1), fps_diff);
		DBG_DUMP(_ANSI_M_"hd: \"%s\".%s[%d] - perf -- (Video) %u Frame/sec"_ANSI_0_"\r\n", 
		    p_thisunit->unit_name, portstr[pd], pi, fps);
		//reset
		fps_count = 0;
		fps_t1 = fps_t2;
	}
	return ISF_OK;
}

ISF_DATA_CLASS _vdofm_base_data =
{
	.this = MAKEFOURCC('V','F','R','M'),
	.base = MAKEFOURCC('C','O','M','M'),
	
	.do_init = _vdofm_init,
	.do_verify = _vdofm_verify,
	.do_load = _vdofm_load,
	.do_save = _vdofm_save,
	.do_probe = _vdofm_probe,
	.do_perf = _vdofm_perf,
};

