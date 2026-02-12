#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Open JPEG-engine to decode JPEG file.
    This is internal API.

    @param[in] pDecJPG the structure info of decoding one JPEG file
    @return ER (E_OK/E_SYS)
*/
ER PB_DecodeOneImg(PPB_HD_DEC_INFO pHDDecJPG)
{
    HD_RESULT ret = HD_OK;
	
	if (!pHDDecJPG){
		return E_SYS;
	}
	
	if ((pHDDecJPG->p_in_video_bs->phy_addr) && (pHDDecJPG->p_out_video_frame->phy_addr[0])){
		ret = hd_videodec_push_in_buf(*pHDDecJPG->p_vdec_path, pHDDecJPG->p_in_video_bs, pHDDecJPG->p_out_video_frame, -1);
		if (ret != HD_OK) {
			DBG_ERR("push_in error for videodec(%d) !!\r\n", ret);
			return E_SYS;
		}				
		
		ret = hd_videodec_pull_out_buf(*pHDDecJPG->p_vdec_path, pHDDecJPG->p_out_video_frame, -1);
		if (ret != HD_OK){
			DBG_ERR("decode error for videodec(%d) !!\r\n", ret);
			return E_SYS;
		}
		
		ret = hd_videodec_release_out_buf(*pHDDecJPG->p_vdec_path, pHDDecJPG->p_out_video_frame); //match with hd_videodec_push_in_buf
		if (ret != HD_OK) {
			DBG_ERR("release_ouf_buf fail, ret(%d)\r\n", ret);
		}		
	}
	return E_OK;
}
