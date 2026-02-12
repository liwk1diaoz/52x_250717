#include "isf_vdoenc_internal.h"
#include "video_encode.h"
#include "nmediarec_api.h"
#include "kflow_videoenc/isf_vdoenc.h"
#include "videosprite/videosprite_enc.h"
#include "comm/hwclock.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__    	  isf_vdoenc_im
#define __DBGLVL__		  8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoenc_im_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////

void _isf_vdoenc_do_input_mask(ISF_UNIT *p_thisunit, UINT32 iport, void *in, void *out, BOOL b_before_rotate)
{
	MP_VDOENC_YUV_SRC *p_in = (MP_VDOENC_YUV_SRC *)in;
	UINT32 oport = iport - ISF_IN_BASE + ISF_OUT_BASE;
	UINT32 codec_type;
	VDS_TO_ENC_EXT_MASK *p_vs_enc;
	UINT32 loff[2];
	UINT64 start = hwclock_get_longcounter();
	
	p_in->uiVidPathID = oport;
	codec_type = p_thisunit->do_getportparam(p_thisunit, oport, VDOENC_PARAM_CODEC);

	// do lock
	DBG_IND("[Lock][%8s][Port=%d][Codec=%d]\r\n", p_thisunit->unit_name, iport, codec_type);
	
	// stamp (Graph or COE)
	DBG_IND("VidPathID = %d, uiYAddr = %08x, uiCbAddr = %08x, uiCrAddr = %08x, uiYLineOffset = %d, uiWidth = %d, uiHeight = %d\r\n",
		p_in->uiVidPathID, p_in->uiYAddr, p_in->uiCbAddr, p_in->uiCrAddr, p_in->uiYLineOffset, p_in->uiWidth, p_in->uiHeight);

	if(codec_type != NMEDIAREC_ENC_MJPG || b_before_rotate){
		p_vs_enc = vds_lock_context(VDS_QS_ENC_EXT_MASK, p_in->uiVidPathID, NULL);
		if(p_vs_enc && p_vs_enc->dirty) {
			loff[0] = p_in->uiYLineOffset;
			loff[1] = p_in->uiUVLineOffset;
			vds_render_enc_ext(VDS_QS_ENC_EXT_MASK, p_in->uiYAddr, p_in->uiCbAddr, p_vs_enc, p_in->uiWidth, p_in->uiHeight, NULL, loff);
			p_vs_enc->dirty = 0;
		}
		vds_save_latency(VDS_QS_ENC_EXT_MASK, p_in->uiVidPathID, 0, hwclock_get_longcounter() - start);
		vds_unlock_context(VDS_QS_ENC_EXT_MASK, p_in->uiVidPathID, 0);
	}
}

void _isf_vdoenc_finish_input_mask(ISF_UNIT *p_thisunit, UINT32 iport)
{
	// do unlock
	DBG_IND("[Unlock][%8s][Port=%d]\r\n", p_thisunit->unit_name, iport);
}


