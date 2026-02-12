#include "isf_vdoprc_int.h"
#if (USE_VDS == ENABLE)
#include "videosprite/videosprite_ime.h"
#endif
#include "comm/hwclock.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_oo
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_oo_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////

void _isf_vdoprc_do_output_osd(ISF_UNIT *p_thisunit, UINT32 oport, void *in, void *out)
{
	#if (USE_VDS == ENABLE)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);  
	UINT32 dev_id = p_ctx->dev;
	_IPP_DS_CB_INPUT_INFO *p_in      = (_IPP_DS_CB_INPUT_INFO*)in;
	_IPP_DS_CB_OUTPUT_INFO *p_out    = (_IPP_DS_CB_OUTPUT_INFO*)out;
	VDS_TO_IME_EXT_STAMP *p_vs_ctx;
	UINT64 start = hwclock_get_longcounter();
	
	p_vs_ctx   = vds_lock_context(VDS_QS_IME_EXT_STAMP, (dev_id << 16) + oport, NULL);
	
	if(p_in && p_out && p_vs_ctx && p_vs_ctx->dirty) {
		vds_render_ime_context(VDS_QS_IME_EXT_STAMP, p_out->p_vdoframe->addr[VDO_PINDEX_Y], 
			p_out->p_vdoframe->addr[VDO_PINDEX_UV], p_vs_ctx, p_in->img_size.w, p_in->img_size.h, 
			vds_get_ime_palette(dev_id), p_out->p_vdoframe->loff);
	 	p_vs_ctx->dirty = 0;
	}

	vds_save_latency(VDS_QS_IME_EXT_STAMP, (dev_id << 16) + oport, 0, hwclock_get_longcounter() - start);
	vds_unlock_context(VDS_QS_IME_EXT_STAMP, oport, 0);
	#endif
}
