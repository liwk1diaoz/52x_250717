#include "isf_vdoout_int.h"
#include "isf_vdoout_dbg.h"
#include "kflow_videoout/isf_vdoout.h"
#include "kflow_common/type_vdo.h"
#include "videosprite/videosprite_vo.h"
#include "comm/hwclock.h"

void _isf_vdoout_do_output_osd(ISF_UNIT *p_thisunit, UINT32 oport, void *in, void *out)
{
	VDO_FRAME *p_in             = (VDO_FRAME*)in;
	VDOOUT_CONTEXT* p_ctx       = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
	VDS_TO_VO_STAMP *p_vs_ctx;
	UINT64 start = hwclock_get_longcounter();
	
	p_vs_ctx   = vds_lock_context(VDS_QS_VO_STAMP, p_ctx->dev, NULL);

	// do lock
	//DBG_IND("[Lock][%8s][Port=%d][devID=%d]\r\n", p_thisunit->unit_name, oport,p_ctx->dev);

	// stamp (Graph or COE)
	//DBG_IND(", uiYAddr = %08x, uiCbAddr = %08x, uiCrAddr = %08x, uiYLineOffset = %d, uiWidth = %d, uiHeight = %d\r\n",
	//	p_in->addr[0], p_in->addr[1], p_in->addr[0], p_in->loff[0], p_in->pw[0], p_in->ph[0]);

	if(p_in && p_vs_ctx && p_vs_ctx->dirty) {
		vds_render_vo(VDS_QS_VO_STAMP, p_in->addr[0], p_in->addr[1], 
						p_vs_ctx, p_in->pw[0], p_in->ph[0], 
						vds_get_vo_palette(p_ctx->dev), p_in->loff, p_in->pxlfmt);
	 	p_vs_ctx->dirty = 0;
	}

	vds_save_latency(VDS_QS_VO_STAMP, p_ctx->dev, 0, hwclock_get_longcounter() - start);
	vds_unlock_context(VDS_QS_VO_STAMP, oport, 0);
}
