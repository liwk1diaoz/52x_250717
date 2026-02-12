#include <string.h>
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "GxVideoFile.h"
#include "kwrap/type.h"
#include "FileSysTsk.h"
#include "avfile/media_def.h"
#include "avfile/AVFile_ParserTs.h"

#define VIDEO_PARSING_WORK_BUF  0x500000

static UINT64 m_uiWholeVdoFileSize;
extern UINT32 gRealFileType;
BOOL bIsdcrypt = FALSE;
BOOL bTrigdcrypt = TRUE;

ER xPB_VdoReadCB(UINT64 pos, UINT32 size, UINT32 addr)
{
	ER result;
	UINT32 u32DebugMsg = FALSE;

    PB_GetParam(PBPRMID_ENABLE_DEBUG_MSG, &u32DebugMsg);
    if (u32DebugMsg) {
		DBG_DUMP("xPB_VdoReadCB  pos=0x%llx, size=0x%x, addr=0x%x\r\n", pos, size, addr);
    }

	if (size == 0) {
		DBG_ERR("size = 0\r\n");
		return E_PAR;
	}
	if ((pos + size) > m_uiWholeVdoFileSize) {
		DBG_ERR("File Read (Pos + Size) is over the file total size!!\r\n");
		return E_PAR;
		//size = m_uiWholeVdoFileSize - pos;
	}

//	result = PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)addr, size, pos, gMenuPlayInfo.JumpOffset);

	if (g_PBSetting.bEnableVidDecrypt) {

		// Read first 256 byte, and do decrypto
		if (pos == 0 && bIsdcrypt == FALSE && bTrigdcrypt) {

			result = PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC,(INT8 *)g_PBSetting.VidDecryptBuf.va, g_PBSetting.VidDecryptBuf.blk_size, pos, gMenuPlayInfo.JumpOffset);
			if (result != E_OK) {
				DBG_ERR("Read file error %d\r\n", result);
				return result;
			}

			GxVidFile_SetParam(GXVIDFILE_PARAM_IS_DECRYPT, TRUE);
			GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_KEY, (UINT32)g_PBSetting.VidDecryptKey);
			GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_MODE, (UINT32)g_PBSetting.VidDecryptMode);
			GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_POS, (UINT32)g_PBSetting.VidDecryptPos);

			result = GxVideoFile_Decrypto((UINT32)g_PBSetting.VidDecryptBuf.va, g_PBSetting.VidDecryptBuf.blk_size);
			if (result != E_OK) {
				DBG_ERR("Decrypto error %d\r\n", result);
				GxVidFile_SetParam(GXVIDFILE_PARAM_IS_DECRYPT, FALSE);
				return result;
			}

			bIsdcrypt = TRUE;
		}

		if (pos < g_PBSetting.VidDecryptBuf.blk_size && bTrigdcrypt) {
			UINT32 srcAddr = (UINT32)(g_PBSetting.VidDecryptBuf.va + pos);
			UINT32 copySize = g_PBSetting.VidDecryptBuf.blk_size - pos;

			if (bIsdcrypt == FALSE) {
				DBG_ERR("Please do decrypto first!!!\r\n");
				return E_SYS;
			}

			if (copySize == 0) {
				DBG_ERR("copySize = %d\r\n", copySize);
				return E_SYS;
			}
			// Copy decrypted data
			memcpy((void *)addr, (void *)srcAddr, copySize);

			// Check is it left data to read
			if (size > g_PBSetting.VidDecryptBuf.blk_size) {
				result = PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)(addr+copySize), (size-copySize), (pos+copySize), gMenuPlayInfo.JumpOffset);
			}
		} else {
			result = PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC,(INT8 *)addr, size, pos, gMenuPlayInfo.JumpOffset);
		}
	} else {

		GxVidFile_SetParam(GXVIDFILE_PARAM_IS_DECRYPT, FALSE);
		result = PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC,(INT8 *)addr, size, pos, gMenuPlayInfo.JumpOffset);
	}


	return result;
}

static HD_RESULT PB_start_dec_path(HD_PATH_ID video_dec_path)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videodec_start(video_dec_path);
	if (ret != HD_OK) {
		DBG_ERR("hd_videodec_start error(%d) !!\r\n", ret);
		return HD_ERR_NG;
	}

	 return ret;
}

static HD_RESULT PB_stop_dec_path(HD_PATH_ID video_dec_path)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videodec_stop(video_dec_path);
	if (ret != HD_OK) {
		DBG_ERR("hd_videodec_stop error(%d) !!\r\n", ret);
		return HD_ERR_NG;
	}

    return ret;
}

static HD_RESULT PB_set_dec_param(HD_PATH_ID video_dec_path, HD_VIDEODEC_IN *p_video_in_param)
{
	HD_RESULT ret = HD_OK;
    HD_H26XDEC_DESC desc_info = {0};

	//--- HD_VIDEODEC_PARAM_IN ---
	ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN, p_video_in_param);
	if (ret != HD_OK) {
		DBG_ERR("set HD_VIDEODEC_PARAM_IN error(%d) !!\r\n", ret);
		return HD_ERR_NG;
	}

   //--- HD_VIDEODEC_PARAM_IN_DESC ---
    if ((p_video_in_param->codec_type == HD_CODEC_TYPE_H264) || (p_video_in_param->codec_type == HD_CODEC_TYPE_H265)){
	    desc_info.addr = g_PBVideoInfo.uiH264DescAddr;
		desc_info.len = g_PBVideoInfo.uiH264DescSize;
		ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN_DESC, &desc_info);
		if (ret != HD_OK) {
			DBG_ERR("set h.26x desc error(%d) !!\r\n", ret);
			return HD_ERR_NG;
		}
    }

	return ret;
}

/* IVOT_N00028-343 support multi-tile */
static void PB_set_h26x_start_code(UINT32 u32Addr, UINT32 bsSize)
{
	UINT8 *ptr = (UINT8 *)u32Addr;
	UINT32 tile_size = 0;
	UINT32 offset = 0;
	UINT8 tile_cnt = 0;
	UINT32 bs_size_count = 0;

	do {

		tile_size = (((*ptr) << 24) | ((*(ptr + 1))<<16) | ((*(ptr + 2))<< 8) | (*(ptr + 3)));

		offset = (4 + tile_size);

		*ptr = 0x00;
		*(ptr+1) = 0x00;
		*(ptr+2) = 0x00;
		*(ptr+3) = 0x01;

		ptr += offset;
		bs_size_count += offset;
		tile_cnt++;

	} while(bs_size_count < bsSize);

	DBG_IND("tile_cnt=%u\r\n", tile_cnt);

}

EXIF_ER PB_DecodeExifParsing (MEM_RANGE *pExifData, UINT32 *pThunmSize, UINT32 *pThumbOffset)
{
	UINT32 ThumbSize = 0;
	MEM_RANGE ExifData;
	EXIF_GETTAG exifTag;
	BOOL bIsLittleEndian = 0;
	UINT32 uiTiffOffsetBase = 0;
	UINT32 ThumbOffset = 0;
	EXIF_ER ret = EXIF_ER_OK;

	ExifData.addr = guiPlayFileBuf;
	ExifData.size = MAX_APP1_SIZE;

	pExifData->addr = ExifData.addr;
	pExifData->size = ExifData.size;

	ret = EXIF_ParseExif(EXIF_HDL_ID_1, &ExifData);
	if (EXIF_ER_OK == ret) {
		EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));
		EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_TIFF_OFFSET_BASE, &uiTiffOffsetBase, sizeof(uiTiffOffsetBase));
		//find thumbnail
		exifTag.uiTagIfd = EXIF_IFD_1ST;
		exifTag.uiTagId = TAG_ID_INTERCHANGE_FORMAT;

		ret = EXIF_GetTag(EXIF_HDL_ID_1, &exifTag);
		if (EXIF_ER_OK == ret) {
			ThumbOffset = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian) + uiTiffOffsetBase;
			exifTag.uiTagId = TAG_ID_INTERCHANGE_FORMAT_LENGTH;

			ret = EXIF_GetTag(EXIF_HDL_ID_1, &exifTag);
			if (EXIF_ER_OK == ret) {
				ThumbSize = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
				*pThunmSize = ThumbSize;
				*pThumbOffset = ThumbOffset;
			} else {
				return ret;
			}

		} else {
			return ret;
		}
	} else {
		return ret;
	}

    return ret;
}

/*
    Do parsing current file format.
    This is internal API.

    @param[in] IsDecodeThumb if decode thumbnail
    @param[in] tmpCurFileFormat current file format
    @return result (DECODE_JPG_READERROR/DECODE_JPG_DONE)
*/
INT32 PB_DecodeWhichFile(PB_JPG_DEC_TYPE IsDecodeThumb, UINT32 tmpCurFileFormat)
{
	INT32  Error_check = DECODE_JPG_DECODEERROR;
	UINT64 tmpFileSize;
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;
	UINT32 uiMaxDecW = 0, uiMaxDecH = 0;
	UINT8 *ptr = NULL;
	HD_VIDEODEC_IN video_in_param = {0};
	UINT32 ThumbSize = 0, ThumbOffset = 0;
	MEM_RANGE ExifData = {0};
	UINT32 u32DebugMsg = FALSE;

    PB_GetParam(PBPRMID_ENABLE_DEBUG_MSG, &u32DebugMsg);
    if (u32DebugMsg) {
		DBG_DUMP("%s: IsDecodeThumb=%d, tmpCurFileFormat=0x%x\r\n", __func__, IsDecodeThumb, tmpCurFileFormat);
    }

	g_HdDecInfo.p_in_video_bs = &g_pPbHdInfo->hd_in_video_bs;
	g_HdDecInfo.p_out_video_frame = &g_pPbHdInfo->hd_out_video_frame;
	PB_GetParam(PBPRMID_MAX_DECODE_WIDTH, &uiMaxDecW);
	PB_GetParam(PBPRMID_MAX_DECODE_HEIGHT, &uiMaxDecH);
	g_HdDecInfo.p_out_video_frame->dim.w = uiMaxDecW; //reset to max decoded width
	g_HdDecInfo.p_out_video_frame->dim.h = uiMaxDecH; //reset to max decoded height

	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &tmpFileSize, NULL);
    if (tmpFileSize == 0){
    	return DECODE_JPG_DECODEERROR;
	}

	if (tmpCurFileFormat & PBFMT_JPG) {
		g_HdDecInfo.p_vdec_path = &g_pPbHdInfo->p_hd_vdec_path[0];
		PB_stop_dec_path(g_pPbHdInfo->p_hd_vdec_path[0]);
		video_in_param.codec_type = HD_CODEC_TYPE_JPEG;
		PB_set_dec_param(g_pPbHdInfo->p_hd_vdec_path[0], &video_in_param);
		PB_start_dec_path(g_pPbHdInfo->p_hd_vdec_path[0]);

        // config file info
        if (IsDecodeThumb == PB_DEC_PRIMARY){

        	EXIF_ER exif_ret;

			g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)guiPlayFileBuf);
			g_pPbHdInfo->hd_in_video_bs.size			= tmpFileSize;


			exif_ret = PB_DecodeExifParsing(&ExifData, &ThumbSize, &ThumbOffset);
        	if(exif_ret == EXIF_ER_OK){

        		/* check pic resolution */
        		EXIF_GETTAG exifTag;
        		UINT32 photo_w, photo_h;
        		UINT32 max_w, max_h;

        		exifTag.uiTagIfd = EXIF_IFD_EXIF;
        		exifTag.uiTagId = TAG_ID_PIXEL_X_DIMENSION;
        		EXIF_GetTag(EXIF_HDL_ID_1, &exifTag);
        		photo_w = *((UINT32*)exifTag.uiTagDataAddr);
        		DBG_IND("xres %lu\r\n", photo_w);

        		exifTag.uiTagId = TAG_ID_PIXEL_Y_DIMENSION;
        		EXIF_GetTag(EXIF_HDL_ID_1, &exifTag);
        		photo_h = *((UINT32*)exifTag.uiTagDataAddr);
        		DBG_IND("y res %lu\r\n", photo_h);

        		PB_GetParam(PBPRMID_MAX_DECODE_WIDTH, &max_w);
        		PB_GetParam(PBPRMID_MAX_DECODE_HEIGHT, &max_h);

        		if((max_w * max_h) < (photo_w * photo_h)){

        			DBG_WRN("decode primary image out of buffer, use screennail instead\n");

            		/* decode screennail if exist */
        			MAKERNOTE_INFO MakernoteInfo;
        			if((EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_MAKERNOTE, &MakernoteInfo, sizeof(MakernoteInfo)) == E_OK) &&  MakernoteInfo.uiScreennailSize && MakernoteInfo.uiScreennailOffset){
            			DBG_IND("Screennail offset = %lx size=%lx\r\n", MakernoteInfo.uiScreennailOffset, MakernoteInfo.uiScreennailSize);
            			g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)guiPlayFileBuf) + MakernoteInfo.uiScreennailOffset;
            			g_pPbHdInfo->hd_in_video_bs.size			= MakernoteInfo.uiScreennailSize;
        			}
        			else{
        				DBG_ERR("screennail not found!\n");
        				return DECODE_JPG_DECODEERROR;
        			}
        		}
        	}
        	/* invalid exif  */
        	else if(exif_ret != EXIF_ER_APP1_NF){
        		return DECODE_JPG_DECODEERROR;
        	}

			g_pPbHdInfo->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_JPEG;
			if (PB_DecodeOneImg(&g_HdDecInfo) != E_OK){
				DBG_ERR("Decode error\r\n");
				return DECODE_JPG_DECODEERROR;
			}
			Error_check = E_OK;
        } else if ((IsDecodeThumb == PB_DEC_THUMBNAIL) || (IsDecodeThumb == PB_DEC_THUMB_ONLY)) {

			// IVOT_N12020_CO-946
			UINT32 parsing_exif = TRUE;
			if(g_PBSetting.EnableFlags & PB_ENABLE_THUMB_JPG_FORCE_DECODE_PRIMARY) {
				parsing_exif = FALSE;
			}

            if (parsing_exif && PB_DecodeExifParsing(&ExifData, &ThumbSize, &ThumbOffset) == E_OK) {
				g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)(guiPlayFileBuf + ThumbOffset));
				g_pPbHdInfo->hd_in_video_bs.size			= ThumbSize;
            } else {
				/*************************************************
				* Whole jpeg file needs to be decoded if exif not found, but the content of guiPlayFileBuf may be incomplete data
				* due to jpeg file size is larger than the size already read(FST_READ_THUMB_BUF_SIZE).
				*
				* The workaround is simply reload again with jpeg file size.
				*************************************************/
				PB_ReadFileByFileSys(PB_FILE_READ_CONTINUE, (INT8 *)guiPlayFileBuf, tmpFileSize, 0, 0);
				g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)guiPlayFileBuf);
				g_pPbHdInfo->hd_in_video_bs.size			= tmpFileSize;
            }

			g_pPbHdInfo->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_JPEG;
			if (PB_DecodeOneImg(&g_HdDecInfo) != E_OK){
				DBG_ERR("Decode error\r\n");
				return DECODE_JPG_DECODEERROR;
			}
			Error_check = E_OK;

        } else if (IsDecodeThumb == PB_DEC_HIDDEN) {

        }
	}else if (tmpCurFileFormat & (PBFMT_MOVMJPG | PBFMT_AVI | PBFMT_MP4 | PBFMT_TS)){
		UINT32	uiBufferNeeded;
		UINT32 uiNewPlayFileBuf = 0;
		MEM_RANGE WorkBuf;

		m_uiWholeVdoFileSize = tmpFileSize;

DecVideoFrame:
		WorkBuf.addr = guiPlayFileBuf;
		WorkBuf.size = VIDEO_PARSING_WORK_BUF;
        uiNewPlayFileBuf = guiPlayFileBuf + VIDEO_PARSING_WORK_BUF;

		bIsdcrypt = FALSE;
		bTrigdcrypt = FALSE;

		if (tmpCurFileFormat & PBFMT_TS) {
            //coverity[assigned_value]
			IsDecodeThumb = PB_DEC_PRIMARY;
		}

		//always trigger decript
		if (g_PBSetting.bEnableVidDecrypt && (g_PBSetting.VidDecryptPos & PB_DECRYPT_TYPE_CONTAINER)) {
			GxVidFile_SetParam(GXVIDFILE_PARAM_IS_DECRYPT, TRUE);
			GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_KEY, (UINT32)g_PBSetting.VidDecryptKey);
			GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_MODE, (UINT32)g_PBSetting.VidDecryptMode);
			GxVidFile_SetParam(GXVIDFILE_PARAM_DECRYPT_POS, (UINT32)g_PBSetting.VidDecryptPos);
		}

		if (IsDecodeThumb == PB_DEC_THUMB_ONLY || IsDecodeThumb == PB_DEC_THUMBNAIL) {

			BOOL is_decrypted = FALSE;

			GxVidFile_QueryThumbWkBufSize(&uiBufferNeeded, m_uiWholeVdoFileSize);

			Error_check = GxVidFile_ParseThumbInfo(xPB_VdoReadCB, &WorkBuf, m_uiWholeVdoFileSize, &g_PBVideoInfo);

			if (GXVIDEO_PRSERR_OK != Error_check) {

				/* file header is either encrytped or broken, try to decrypt file and parsing again */
				if (g_PBSetting.bEnableVidDecrypt) {
					bTrigdcrypt = TRUE;
					Error_check = GxVidFile_ParseThumbInfo(xPB_VdoReadCB, &WorkBuf, m_uiWholeVdoFileSize, &g_PBVideoInfo);
				}

				/* file header may be broken */
				if(Error_check != GXVIDEO_PRSERR_OK){
					DBG_ERR("ParseVideoInfo NG!\r\n");
					return	DECODE_JPG_DECODEERROR;
				}
				/* file header is decrytpted successfully (GXVIDEO_DECRYPT_TYPE_CONTAINER) */
				else{
					is_decrypted = TRUE;
				}
			}

			if ((g_PBVideoInfo.uiThumbOffset == 0) || (g_PBVideoInfo.uiThumbSize == 0)) { // no thumbnail
				IsDecodeThumb = PB_DEC_PRIMARY; // try to decode 1st frame
				goto DecVideoFrame;
			}

			/* remember to using %llx for UINT64 printing */
			DBG_IND("uiThumbOffset = %llx\r\n", g_PBVideoInfo.uiThumbOffset);
			DBG_IND("uiThumbSize = %lu \r\n", g_PBVideoInfo.uiThumbSize);

			g_HdDecInfo.p_vdec_path = &g_pPbHdInfo->p_hd_vdec_path[0];
			PB_stop_dec_path(g_pPbHdInfo->p_hd_vdec_path[0]);
			video_in_param.codec_type = HD_CODEC_TYPE_JPEG;
			PB_set_dec_param(g_pPbHdInfo->p_hd_vdec_path[0], &video_in_param);
			PB_start_dec_path(g_pPbHdInfo->p_hd_vdec_path[0]);
			g_pPbHdInfo->hd_in_video_bs.vcodec_format	= HD_CODEC_TYPE_JPEG;
			g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)uiNewPlayFileBuf);
			g_pPbHdInfo->hd_in_video_bs.size			= g_PBVideoInfo.uiThumbSize;

			/* check is file header really encrypted */
			if(is_decrypted == TRUE){

				/* encrypted file header also encrypt a part of jpeg data, cp decrypted data from VidDecryptBuf to read data  */
				UINT32 jpeg_cp_size = VID_DECRYPT_LEN - g_PBVideoInfo.uiThumbOffset;
				ptr = (UINT8*)g_PBSetting.VidDecryptBuf.va + g_PBVideoInfo.uiThumbOffset;

				PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)uiNewPlayFileBuf, g_PBVideoInfo.uiThumbSize, g_PBVideoInfo.uiThumbOffset, 0);
				memcpy((void*)uiNewPlayFileBuf, ptr, jpeg_cp_size);
				hd_common_mem_flush_cache((void *)uiNewPlayFileBuf, g_PBVideoInfo.uiThumbSize);
			}
			else{
				PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)uiNewPlayFileBuf, g_PBVideoInfo.uiThumbSize, g_PBVideoInfo.uiThumbOffset, 0);
			}

			ptr = (UINT8 *)uiNewPlayFileBuf;
			if (PB_DecodeOneImg(&g_HdDecInfo) != E_OK){
				DBG_ERR("Decode error\r\n");
				return DECODE_JPG_DECODEERROR;
			}

			Error_check = E_OK;

		} else {
			GxVidFile_Query1stFrameWkBufSize(&uiBufferNeeded, m_uiWholeVdoFileSize);

			Error_check =  GxVidFile_ParseVideoInfo64(xPB_VdoReadCB, &WorkBuf, m_uiWholeVdoFileSize, &g_PBVideoInfo);

			//parse video info for single mode only
			if (GXVIDEO_PRSERR_OK != Error_check) {

				if (g_PBSetting.bEnableVidDecrypt) {
					bTrigdcrypt = TRUE;
					Error_check = GxVidFile_ParseVideoInfo64(xPB_VdoReadCB, &WorkBuf, m_uiWholeVdoFileSize, &g_PBVideoInfo);
				}

				if(Error_check != GXVIDEO_PRSERR_OK){
					DBG_ERR("ParseVideoInfo NG!\r\n");
					return	DECODE_JPG_DECODEERROR;
				}
			}

            if (g_PBVideoInfo.uiVidType == MEDIAPLAY_VIDEO_H264){
				g_HdDecInfo.p_vdec_path = & g_pPbHdInfo->p_hd_vdec_path[1];
				PB_stop_dec_path(g_pPbHdInfo->p_hd_vdec_path[1]);
				video_in_param.codec_type = HD_CODEC_TYPE_H264;
            	PB_set_dec_param(g_pPbHdInfo->p_hd_vdec_path[1], &video_in_param);
				PB_start_dec_path(g_pPbHdInfo->p_hd_vdec_path[1]);
				g_pPbHdInfo->hd_in_video_bs.vcodec_format   = HD_CODEC_TYPE_H264;
				if (tmpCurFileFormat & PBFMT_TS) {
					uiNewPlayFileBuf = guiPlayFileBuf + GXVIDEO_H26X_WORK_BUFFER_SIZE + GXVIDEO_VID_ENTRY_BUFFER_SIZE + GXVIDEO_AUD_ENTRY_BUFFER_SIZE;
					uiNewPlayFileBuf += g_PBVideoInfo.ui1stFramePos;
					g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)uiNewPlayFileBuf);
					g_pPbHdInfo->hd_in_video_bs.size			= g_PBVideoInfo.ui1stFrameSize;
					ptr = (UINT8 *)uiNewPlayFileBuf;
				} else {
					g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)uiNewPlayFileBuf);
					g_pPbHdInfo->hd_in_video_bs.size			= g_PBVideoInfo.ui1stFrameSize;
					PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)uiNewPlayFileBuf, g_PBVideoInfo.ui1stFrameSize, g_PBVideoInfo.ui1stFramePos, 0);
					ptr = (UINT8 *)uiNewPlayFileBuf;
					/* IVOT_N00028-343 support multi-tile */
					PB_set_h26x_start_code((UINT32)ptr, g_pPbHdInfo->hd_in_video_bs.size);
				}
            }else if (g_PBVideoInfo.uiVidType == MEDIAPLAY_VIDEO_H265){
				g_HdDecInfo.p_vdec_path = & g_pPbHdInfo->p_hd_vdec_path[2];
    			PB_stop_dec_path(g_pPbHdInfo->p_hd_vdec_path[2]);
				video_in_param.codec_type = HD_CODEC_TYPE_H265;
            	PB_set_dec_param(g_pPbHdInfo->p_hd_vdec_path[2], &video_in_param);
				PB_start_dec_path(g_pPbHdInfo->p_hd_vdec_path[2]);
				g_pPbHdInfo->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_H265;
				if (tmpCurFileFormat & PBFMT_TS) {
					uiNewPlayFileBuf = guiPlayFileBuf + GXVIDEO_H26X_WORK_BUFFER_SIZE + GXVIDEO_VID_ENTRY_BUFFER_SIZE + GXVIDEO_AUD_ENTRY_BUFFER_SIZE;
					uiNewPlayFileBuf += g_PBVideoInfo.ui1stFramePos;
					g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)uiNewPlayFileBuf);
					g_pPbHdInfo->hd_in_video_bs.size			= g_PBVideoInfo.ui1stFrameSize;
					ptr = (UINT8 *)uiNewPlayFileBuf;
				} else {
					PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)uiNewPlayFileBuf, g_PBVideoInfo.ui1stFrameSize, g_PBVideoInfo.ui1stFramePos, 0);
					ptr = (UINT8 *)uiNewPlayFileBuf;
					ptr= ptr + g_PBVideoInfo.uiH264DescSize;

					g_pPbHdInfo->hd_in_video_bs.phy_addr		= PB_get_hd_phy_addr((void *)ptr);
					g_pPbHdInfo->hd_in_video_bs.size			= g_PBVideoInfo.ui1stFrameSize - g_PBVideoInfo.uiH264DescSize;
					/* IVOT_N00028-343 support multi-tile */
					PB_set_h26x_start_code((UINT32)ptr, g_pPbHdInfo->hd_in_video_bs.size);
				}
            }

        	// decrypt
            BOOL bIsDecrypto = FALSE;
            UINT32 DecryptoPos = 0, Decryptoaddr = 0;

        	GxVidFile_GetParam(GXVIDFILE_PARAM_IS_DECRYPT, (UINT32*)&bIsDecrypto);
        	GxVidFile_GetParam(GXVIDFILE_PARAM_DECRYPT_POS, (UINT32*)&DecryptoPos);

        	if (bIsDecrypto && (DecryptoPos & GXVIDEO_DECRYPT_TYPE_I_FRAME)) {

        		Decryptoaddr = (UINT32)(ptr + 16);
        		GxVideoFile_Decrypto((UINT32)Decryptoaddr, VID_DECRYPT_LEN);
        	}

			if (PB_DecodeOneImg(&g_HdDecInfo) != E_OK){
				DBG_ERR("Decode error\r\n");
				return DECODE_JPG_DECODEERROR;
			} else {
				if (u32DebugMsg) {
					DBG_DUMP("%s: Decode OK\r\n", __func__);
				}
			}

			Error_check = E_OK;
		}
	}

		if (Error_check != E_OK) {
			Error_check = DECODE_JPG_DECODEERROR;
		} else {
		Error_check = DECODE_JPG_PRIMARY;
	}

    return Error_check;
}

void xPB_UpdateEXIFOrientation(INT32 iErrCode)
{
	if (E_OK == iErrCode) {
		EXIF_GETTAG exifTag;
		BOOL bIsLittleEndian = FALSE;

		EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));
		exifTag.uiTagIfd = EXIF_IFD_0TH;
		exifTag.uiTagId  = TAG_ID_ORIENTATION;
		if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
			g_uiExifOrientation = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
			g_uiExifOrientation &= 0xFF;
		} else {
			g_uiExifOrientation = JPEG_EXIF_ORI_DEFAULT;
		}

		if (g_PBSetting.bEnableAutoRotate == FALSE) {
			// Disable Auto-rotate function
			g_uiExifOrientation = JPEG_EXIF_ORI_DEFAULT;
		} else {
			if ((g_uiExifOrientation != JPEG_EXIF_ORI_DEFAULT) &&
				(g_uiExifOrientation != JPEG_EXIF_ORI_ROTATE_90) &&
				(g_uiExifOrientation != JPEG_EXIF_ORI_ROTATE_180) &&
				(g_uiExifOrientation != JPEG_EXIF_ORI_ROTATE_270)) {
				// something wrong, set Orientation tag to default value
				g_uiExifOrientation = JPEG_EXIF_ORI_DEFAULT;
			}
		}
	} else {
		g_uiExifOrientation = JPEG_EXIF_ORI_DEFAULT;
	}
}

