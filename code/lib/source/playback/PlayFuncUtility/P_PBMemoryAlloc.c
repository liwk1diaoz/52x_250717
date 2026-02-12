#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "hdal.h"
#include "hd_type.h"

/*
    Memory allocation for playback.
    This is internal API.

    @return E_OK: OK, E_SYS: NG
*/
INT32 PB_AllocPBMemory(UINT32 uiFB_W, UINT32 uiFB_H)
{
	//UINT32 uiTmpBuf;
	UINT32 uiPanelSZ = g_uiMAXPanelSize;
	UINT32 ime_tmp_w, ime_tmp_h;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 pa,va, blk_size;
	HD_RESULT hd_ret;
	UINT32 uiMaxDecW = 0, uiMaxDecH = 0, uiPixFmt;

	if (uiFB_W > MAX_TWO_PASS_SCALE_W && uiFB_H > MAX_TWO_PASS_SCALE_H) {
		ime_tmp_w = uiFB_W;
		ime_tmp_h = uiFB_H;
		gScaleTwice = FALSE;
	} else {
		ime_tmp_w = uiFB_W*2;
		ime_tmp_h = uiFB_H*2;
		gScaleTwice = TRUE;
	}

	guiPlayIMETmpBufSize = ime_tmp_w*ime_tmp_h*2;

    if (g_pPbHdInfo->hd_vdo_pix_fmt == HD_VIDEO_PXLFMT_YUV422){
		blk_size = uiPanelSZ*2;
	}else if (g_pPbHdInfo->hd_vdo_pix_fmt == HD_VIDEO_PXLFMT_YUV420){
		blk_size = uiPanelSZ*3/2;
    }

	//g_uiPBBufStart = va;	
	hd_ret = PB_get_hd_common_buf(&blk, &pa, &va, blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("Alloc FBBuf fail\r\n");
		return E_SYS;
	}
	g_uiPBFBBuf  = va;
	g_hd_tmp1buf.pa = pa;
	g_hd_tmp1buf.va = va;
	g_hd_tmp1buf.blk_size = blk_size;
	g_hd_tmp1buf.blk = blk;
	
	hd_ret = PB_get_hd_common_buf(&blk, &pa, &va, blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("Alloc TmpBuf fail\r\n");
		return E_SYS;
	}
	g_uiPBTmpBuf = va;  
	g_hd_tmp2buf.pa = pa;
	g_hd_tmp2buf.va = va;
	g_hd_tmp2buf.blk_size = blk_size;
	g_hd_tmp2buf.blk = blk;

	hd_ret = PB_get_hd_common_buf(&blk, &pa, &va, blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("Alloc IMETmpBuf fail\r\n");
		return E_SYS;
	}	
	g_uiPBIMETmpBuf = va;
	g_hd_tmp3buf.pa = pa;
	g_hd_tmp3buf.va = va;
	g_hd_tmp3buf.blk_size = blk_size;
	g_hd_tmp3buf.blk = blk;	

    /*--- File buffer and bitstream allocation for hd_videodec: Begin ---*/
    // alloc file buffer 
    PB_GetParam(PBPRMID_MAX_FILE_SIZE,  &blk_size);
	hd_ret = PB_get_hd_common_buf(&blk, &pa, &va, blk_size);
	g_hd_file_buf.pa = pa;
	g_hd_file_buf.va = va;
	g_hd_file_buf.blk_size = blk_size;
	g_hd_file_buf.blk = blk;	
	if (hd_ret != HD_OK){
		DBG_ERR("Alloc file buffer fail\r\n");
		return E_SYS;
	}		
	g_uiPBBufEnd = va + blk_size ;	
	PB_GetDefaultFileBufAddr(); // reserved for guiPlayFileBuf
	guiPlayMaxFileBufSize = blk_size;
	// config video bitstream
	g_pPbHdInfo->hd_in_video_bs.sign		    = MAKEFOURCC('V','S','T','M');
	g_pPbHdInfo->hd_in_video_bs.p_next	    = NULL;
	g_pPbHdInfo->hd_in_video_bs.ddr_id	    = DDR_ID0;
	g_pPbHdInfo->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_JPEG; //Default
	g_pPbHdInfo->hd_in_video_bs.timestamp     = hd_gettime_us();
	g_pPbHdInfo->hd_in_video_bs.blk		    = blk;
	g_pPbHdInfo->hd_in_video_bs.count 	    = 0;
	g_pPbHdInfo->hd_in_video_bs.phy_addr	    = pa;
	g_pPbHdInfo->hd_in_video_bs.size		    = blk_size; //Default	
	/*--- File buffer and bitstream allocation for hd_videodec: End ---*/

	/*--- Configure max decode buffer for hd_videodec: Begin ---*/
	PB_GetParam(PBPRMID_MAX_DECODE_WIDTH,  &uiMaxDecW);
	PB_GetParam(PBPRMID_MAX_DECODE_HEIGHT, &uiMaxDecH);
	PB_GetParam(PBPRMID_PIXEL_FORMAT, &uiPixFmt);   
	if (uiPixFmt == HD_VIDEO_PXLFMT_YUV422){
	   blk_size = uiMaxDecW*uiMaxDecH*2;
	}else if (uiPixFmt == HD_VIDEO_PXLFMT_YUV420){
	   blk_size = uiMaxDecW*uiMaxDecH*3/2;
	}
	
    PB_get_hd_common_buf(&blk, &pa, &va, blk_size);
	g_hd_raw_buf.pa = pa;
	g_hd_raw_buf.va = va;
	g_hd_raw_buf.blk_size = blk_size;
	g_hd_raw_buf.blk = blk;   
	guiPlayRawBuf = va;
	g_pPbHdInfo->hd_out_video_frame.sign = MAKEFOURCC('V', 'F', 'R', 'M');
	g_pPbHdInfo->hd_out_video_frame.ddr_id = DDR_ID0;
	g_pPbHdInfo->hd_out_video_frame.pxlfmt = uiPixFmt;
	g_pPbHdInfo->hd_out_video_frame.dim.w = uiMaxDecW;
	g_pPbHdInfo->hd_out_video_frame.dim.h = uiMaxDecH;
	g_pPbHdInfo->hd_out_video_frame.phy_addr[0] = pa;
	g_pPbHdInfo->hd_out_video_frame.blk = blk;     
	g_pPbHdInfo->hd_uiMaxRawSize = blk_size;
   /*--- Configure max decode buffer for hd_videodec: end ---*/


    /*--- Configure exif buffer for decoded thumbnail: Begin ---*/
    blk_size = MAX_APP1_SIZE;
    PB_get_hd_common_buf(&blk, &pa, &va, blk_size);
    g_hd_exif_buf.pa = pa;
	g_hd_exif_buf.va = va;
	g_hd_exif_buf.blk_size = blk_size;
	g_hd_exif_buf.blk = blk; 	
    /*--- Configure exif buffer for decoded thumbnail: End ---*/

	blk_size = VID_DECRYPT_LEN * sizeof(UINT8);
	PB_get_hd_common_buf(&blk, &pa, &va, blk_size);
	g_PBSetting.VidDecryptBuf.va = va;
	g_PBSetting.VidDecryptBuf.pa = pa;
	g_PBSetting.VidDecryptBuf.blk_size = blk_size;
	g_PBSetting.VidDecryptBuf.blk = blk;

	return E_OK;
}

