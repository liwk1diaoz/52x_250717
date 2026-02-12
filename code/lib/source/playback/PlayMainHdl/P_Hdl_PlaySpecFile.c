#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"
#include "SizeConvert.h"
#if _TODO

#include "GxVideo.h"
#endif

#if _TODO
static INT32 m_iErrChk;
static VDO_FRAME m_DstImg4SpecFile;
static JPG_DEC_INFO    m_DecJPG = {0};
static JPGHEAD_DEC_CFG m_JPGCfg = {0};
#endif

void PB_Hdl_PlaySpecFileHandle(void)
{
#if _TODO
	INT32 Error_check = DECODE_JPG_DONE;
	UINT32 SrcWidth, SrcHeight, FileFormat;
	//UINT16 usPBDisplayWidth, usPBDisplayHeight;
	//UINT32 uiYSize;
	UINT32 FileType;
	UINT64 BufSize = FST_READ_THUMB_BUF_SIZE;
	UINT8  ucOrientation;
	PURECT pOutRect = &(gPBCmdObjPlaySpecFile).PlayRect;
	VDO_FRAME TmpImg;
	IRECT    DstRegion = {0};
	SIZECONVERT_INFO ScaleInfo = {0};
	UINT32 index;
	PJPGHEAD_DEC_CFG pJPGInfo = gMenuPlayInfo.pJPGInfo;
	BOOL bSpecPath4PBZoom;
	PVDO_FRAME pRawImgBuf;
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;

	if (0 != gMenuPlayInfo.ZoomIndex) {
		bSpecPath4PBZoom = TRUE;
	} else {
		bSpecPath4PBZoom = FALSE;
	}

	//usPBDisplayWidth = g_PBDispInfo.uiDisplayW;
	//usPBDisplayHeight = g_PBDispInfo.uiDisplayH;
	//uiYSize = usPBDisplayWidth * usPBDisplayHeight;

	PB_GetDefaultFileBufAddr();

	index = gMenuPlayInfo.DispFileSeq[gMenuPlayInfo.CurFileIndex - 1];
	pFlist->SeekIndex(index, PBX_FLIST_SEEK_SET);
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILETYPE, &FileType, NULL);

	FileFormat = pJPGInfo->fileformat;
	SrcWidth  = pJPGInfo->imagewidth;
	SrcHeight = pJPGInfo->imageheight;

	if (gPBCmdObjPlaySpecFile.bDisplayWoReDec) {
		// for accelerating 2nd same spec file
		goto JustDisplay;
	}

	if (PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)guiPlayFileBuf, (UINT32)BufSize, 0, 0) != E_OK) {
		Error_check = DECODE_JPG_READERROR;
	}
//#NT#2016/05/30#Scottie -begin
//#NT#Remove for Coverity: 60437 (Logically dead code)
#if 0
	else {
		if (BufSize == 0) {
			Error_check = DECODE_JPG_READERROR;
		}
	}
#endif
//#NT#2016/05/30#Scottie -end
	if (Error_check > 0) {
		//if ((FileType & PBX_FLIST_FILE_TYPE_AVI)||(FileType & PBX_FLIST_FILE_TYPE_MOV))
		if (FileType & (PBX_FLIST_FILE_TYPE_AVI | PBX_FLIST_FILE_TYPE_MOV | PBX_FLIST_FILE_TYPE_MP4 | PBX_FLIST_FILE_TYPE_TS)) {
			if (FileType & PBX_FLIST_FILE_TYPE_AVI) {
				FileFormat = PBFMT_AVI;
			} else if (FileType & PBX_FLIST_FILE_TYPE_MOV) {
				FileFormat = PBFMT_MOVMJPG;
			} else if (FileType & PBX_FLIST_FILE_TYPE_TS) {
				FileFormat = PBFMT_TS;
			} else {
				FileFormat = PBFMT_MP4;
			}
			Error_check = PB_DecodeWhichFile(PB_DEC_THUMBNAIL, FileFormat);
		} else {
			if (PLAY_SPEC_FILE_IN_VIDEO_1 == gPBCmdObjPlaySpecFile.PlayFileVideo) {
				Error_check = E_SYS;
			} else {
				if (TRUE == bSpecPath4PBZoom) {
					m_DecJPG.DecodeType  = DEC_THUMBNAIL;
					m_DecJPG.pSrcAddr    = (UINT8 *)guiPlayFileBuf;
					m_DecJPG.JPGFileSize = FST_READ_THUMB_BUF_SIZE;
					m_DecJPG.pDstAddr    = (UINT8 *)g_pThumb1ImgBuf->PxlAddr[0];
					m_DecJPG.jdcfg_p     = &m_JPGCfg;
					Error_check = jpg_dec_one(&m_DecJPG);
					if (Error_check == E_OK) {
						/* Update decoded image buffer */
						UINT32  LineOffset[3] = {0}, PxlAddr[3] = {0};
						UINT16 uiImgWidth, uiImgHeight;
						UINT32 PxlFmt;

						uiImgWidth          = m_DecJPG.jdcfg_p->imagewidth;
						uiImgHeight         = m_DecJPG.jdcfg_p->imageheight;
						if (JPG_FMT_YUV422 == m_DecJPG.jdcfg_p->fileformat) {
							PxlFmt = VDO_PXLFMT_YUV422;
						} else { //if(JPG_FMT_YUV420 == m_DecJPG.jdcfg_p->fileformat)
							PxlFmt = VDO_PXLFMT_YUV420;
						}
						PxlAddr[0] = m_DecJPG.uiOutAddrY;
						PxlAddr[1] = m_DecJPG.uiOutAddrCb;
						PxlAddr[2] = m_DecJPG.uiOutAddrCr;
						LineOffset[0] = m_DecJPG.jdcfg_p->lineoffsetY;
						LineOffset[1] = m_DecJPG.jdcfg_p->lineoffsetUV;
						LineOffset[2] = m_DecJPG.jdcfg_p->lineoffsetUV;
						gximg_init_buf_ex(g_pThumb1ImgBuf, uiImgWidth, uiImgHeight, PxlFmt, LineOffset, PxlAddr);
					}
					xPB_UpdateEXIFOrientation(Error_check);

					if (E_OK != Error_check) {
						DBG_ERR("Dec thumbnail error(1)!\r\n");

						if (TRUE == gMenuPlayInfo.DecodePrimaryOK) {
							if (pJPGInfo->imagewidth > PLAY_IMETMP_WIDTH && gScaleTwice) {
								// Need to do 2 pass scale down for Primary image
								gximg_scale_data_fine(g_pDecodedImgBuf, GXIMG_REGION_MATCH_IMG, g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG);
								gximg_scale_data_fine(g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG, g_pThumb1ImgBuf, GXIMG_REGION_MATCH_IMG);
							} else {
								gximg_scale_data_fine(g_pDecodedImgBuf, GXIMG_REGION_MATCH_IMG, g_pThumb1ImgBuf, GXIMG_REGION_MATCH_IMG);
							}

							Error_check = E_OK;
						}
					} else {
						pJPGInfo = m_DecJPG.jdcfg_p;
					}
				} else {
					gDecOneJPGInfo.DecodeType  = DEC_THUMBNAIL;
					gDecOneJPGInfo.pSrcAddr    = (UINT8 *)guiPlayFileBuf;
					gDecOneJPGInfo.JPGFileSize = FST_READ_THUMB_BUF_SIZE;
					//Error_check = PB_DecodeOneImg(&gDecOneJPGInfo);
					Error_check = PB_DecodeOneImg(NULL);
					xPB_UpdateEXIFOrientation(Error_check);
					if (E_OK == Error_check) {
						gMenuPlayInfo.DecodePrimaryOK = FALSE;
						if (g_fpDecImageCB && FALSE == g_fpDecImageCB(PB_DECIMG_THUMBNAIL)) {
							Error_check = DEC_CALLBACK_ERROR;
						}
					} else if (JPG_HEADER_ER_SOI_NF == Error_check) { //no FFD8
						Error_check = DECODE_JPG_CUSTOMERROR;
					} else {
						DBG_ERR("Dec thumbnail error(2)!\r\n");
					}
				}
			}

			if ((Error_check != E_OK) && (Error_check != DECODE_JPG_CUSTOMERROR)) {
				pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &BufSize, NULL);
				if ((UINT32)BufSize > guiPlayMaxFileBufSize) {
					Error_check = DECODE_JPG_BIGFILE;
				} else if ((UINT32)BufSize > FST_READ_THUMB_BUF_SIZE) {
					////////////////////////////////////////////////////////////
					// Alignment for File System performance
					////////////////////////////////////////////////////////////
					guiPlayFileBuf = ALIGN_FLOOR_16(g_uiPBBufEnd - (UINT32)BufSize);

					// Read continue file
					if (PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)guiPlayFileBuf, (UINT32)BufSize, 0, 0) != E_OK) {
//#NT#2016/05/30#Scottie -begin
//#NT#Remove for Coverity: 61078 (Unused value)
//                        Error_check = DECODE_JPG_READERROR;
//#NT#2016/05/30#Scottie -end
					}

					// Decode primary-image of this file
					gDecOneJPGInfo.DecodeType = PB_DEC_PRIMARY;
					gDecOneJPGInfo.pSrcAddr = (UINT8 *)guiPlayFileBuf;
					gDecOneJPGInfo.JPGFileSize = (UINT32)BufSize;
					//Error_check = PB_DecodeOneImg(&gDecOneJPGInfo);
					Error_check = PB_DecodeOneImg(NULL);
					xPB_UpdateEXIFOrientation(Error_check);
					if (Error_check != E_OK) {
						Error_check = DECODE_JPG_DECODEERROR;
					} else {
						UINT32 uiRawSize, uiThumbBuff;
						UINT32 uiTmpW, uiTmpH;

						gMenuPlayInfo.DecodePrimaryOK = TRUE;
						if (FileFormat == PB_JPG_FMT_YUV211) {
							uiRawSize = (SrcWidth * SrcHeight) * 2;
						} else { // if(FileFormat == JPG_FMT_YUV420)
							uiRawSize = (SrcWidth * SrcHeight) * 3 / 2;
						}
						uiThumbBuff = g_pDecodedImgBuf->PxlAddr[0] + uiRawSize;

						if (PLAY_SPEC_FILE_IN_VIDEO_1 == gPBCmdObjPlaySpecFile.PlayFileVideo) {
							uiTmpW = g_stPBCSNInfo.uiDstWidth;
							uiTmpH = g_stPBCSNInfo.uiDstHeight;
						} else {
							uiTmpW = 160;
							uiTmpH = 120;
						}
						gximg_init_buf(&TmpImg, uiTmpW, uiTmpH, VDO_PXLFMT_YUV422, uiTmpW, uiThumbBuff, uiTmpW * uiTmpH * 2);

						if (pJPGInfo->imagewidth > PLAY_IMETMP_WIDTH && gScaleTwice) {
							// Need to do 2 pass scale down for Primary image
							// 1. Scale primary image to PLAY_IMETMP_WIDTHxPLAY_IMETMP_HEIGHT
							gximg_scale_data_fine(g_pDecodedImgBuf, GXIMG_REGION_MATCH_IMG, g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG);

							// 2. Scale MIDDLE_SIZE_WxMIDDLE_SIZE_H to thumbnail size
							gximg_scale_data_fine(g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG, &TmpImg, GXIMG_REGION_MATCH_IMG);
						} else {
							gximg_scale_data_fine(g_pDecodedImgBuf, GXIMG_REGION_MATCH_IMG, &TmpImg, GXIMG_REGION_MATCH_IMG);
						}

						// Update JPEG related info
						SrcWidth  = uiTmpW;
//#NT#2016/05/30#Scottie -begin
//#NT#Remove for Coverity: 61077 (Unused value)
//                        SrcHeight = uiTmpH;
//#NT#2016/05/30#Scottie -end

						Error_check = DECODE_JPG_PRIMARY;
					}
				} else {
					Error_check = DECODE_JPG_DECODEERROR;
				}
			} else if (Error_check == E_OK) {
				if ((pJPGInfo->ori_imagewidth != DCF_THUMB_PIXEL_H) || (pJPGInfo->ori_imageheight != DCF_THUMB_PIXEL_V)) {
					// Thumbnail NG
					Error_check = DECODE_JPG_PRIMARY;
				} else {
					Error_check = DECODE_JPG_THUMBOK;
				}
			}
		}
	}

	// for accelerating 2nd same spec file
	m_iErrChk = Error_check;

JustDisplay:
	if (TRUE == bSpecPath4PBZoom) {
		pRawImgBuf = g_pThumb1ImgBuf;
	} else {
		pRawImgBuf = g_pDecodedImgBuf;
	}

	if ((gPBCmdObjPlaySpecFile.PlayFileClearBuf == PLAY_SPEC_FILE_WITH_CLEAR_BUF) && (FileType & PBX_FLIST_FILE_TYPE_JPG)) {
		gximg_fill_data(g_pPlayTmpImgBuf, GXIMG_REGION_MATCH_IMG, g_uiDefaultBGColor);
	}

	if (((FileType & PBX_FLIST_FILE_TYPE_AVI) || (FileType & PBX_FLIST_FILE_TYPE_MOV)) &&
		(gPBCmdObjPlaySpecFile.bDisplayWoReDec)) {
		// Avoid screen flashing, switch to another buffer
		memcpy((void *)&m_DstImg4SpecFile, (const void *)g_pPlayIMETmpImgBuf, sizeof(VDO_FRAME));
	} else {
		memcpy((void *)&m_DstImg4SpecFile, (const void *)g_pPlayTmpImgBuf, sizeof(VDO_FRAME));
	}

	// Use DisplaySizeConvert() to calc. the DstWidth & DstHeight
	ScaleInfo.uiSrcWidth  = pJPGInfo->imagewidth;
	ScaleInfo.uiSrcHeight = pJPGInfo->imageheight;
	ScaleInfo.uiDstWidth  = pOutRect->w;
	ScaleInfo.uiDstHeight = pOutRect->h;
	ScaleInfo.uiDstWRatio = g_PBDispInfo.uiARatioW;
	ScaleInfo.uiDstHRatio = g_PBDispInfo.uiARatioH;

	if (g_PBSetting.bEnableAutoRotate == TRUE) {
		ucOrientation = g_uiExifOrientation;

		if ((ucOrientation != JPEG_EXIF_ORI_DEFAULT) && (ucOrientation != JPEG_EXIF_ORI_ROTATE_90) &&
			(ucOrientation != JPEG_EXIF_ORI_ROTATE_180) && (ucOrientation != JPEG_EXIF_ORI_ROTATE_270)) {
			// something wrong, set Orientation tag to default value
			ucOrientation = g_uiExifOrientation = JPEG_EXIF_ORI_DEFAULT;
		}
	} else {
		ucOrientation = JPEG_EXIF_ORI_DEFAULT;
	}

	if ((ucOrientation != JPEG_EXIF_ORI_DEFAULT) && (m_iErrChk > 0)) {
		UINT32 uiGxRotateDir;

		switch (ucOrientation) {
		case JPEG_EXIF_ORI_ROTATE_180:
			uiGxRotateDir = VDO_DIR_ROTATE_180;
			break;

		case JPEG_EXIF_ORI_ROTATE_90:
			uiGxRotateDir = VDO_DIR_ROTATE_90;
			break;

//#NT#2015/10/05#Scottie -begin
//#NT#Remove this Coverity defect: Logically dead code (cannot be 2 or 4)
#if 0
		case JPEG_EXIF_ORI_FLIP_HORIZONTAL:
			uiGxRotateDir = GX_IMAGE_ROTATE_HRZ;
			break;

		case JPEG_EXIF_ORI_FLIP_VERTICAL:
			uiGxRotateDir = GX_IMAGE_ROTATE_VTC;
			break;
#endif
//#NT#2015/10/05#Scottie -end

		case JPEG_EXIF_ORI_ROTATE_270:
		default:
			uiGxRotateDir = VDO_DIR_ROTATE_270;
			break;
		}
		gximg_rotate_data(pRawImgBuf, g_uiPBIMETmpBuf, guiPlayIMETmpBufSize, uiGxRotateDir, g_pPlayIMETmpImgBuf);

		// (2) Scale rotated image to DstBuf..
		if ((ucOrientation == JPEG_EXIF_ORI_ROTATE_90) || (ucOrientation == JPEG_EXIF_ORI_ROTATE_270)) {
			ScaleInfo.uiSrcWidth  = pJPGInfo->imageheight;
			ScaleInfo.uiSrcHeight = pJPGInfo->imagewidth;
		}
		ScaleInfo.alignType = SIZECONVERT_ALIGN_FLOOR_0;
		DisplaySizeConvert(&ScaleInfo);

		DstRegion.x = pOutRect->x + ScaleInfo.uiOutX;
		DstRegion.y = pOutRect->y + ScaleInfo.uiOutY;
		DstRegion.w = ScaleInfo.uiOutWidth;
		DstRegion.h = ScaleInfo.uiOutHeight;
		gximg_scale_data(g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG, &m_DstImg4SpecFile, &DstRegion);
	} else {
		// Dec NG then show black frame
		ScaleInfo.alignType = SIZECONVERT_ALIGN_FLOOR_16;
		DisplaySizeConvert(&ScaleInfo);

		DstRegion.x = pOutRect->x + ScaleInfo.uiOutX;
		DstRegion.y = pOutRect->y + ScaleInfo.uiOutY;
		DstRegion.w = ScaleInfo.uiOutWidth;
		DstRegion.h = ScaleInfo.uiOutHeight;
		if ((m_iErrChk > 0) && (m_iErrChk != DECODE_JPG_WAVEOK)) {
			//Ok, display the image..
			if ((SrcWidth > PLAY_IMETMP_WIDTH) && (FileType & PBX_FLIST_FILE_TYPE_JPG) && gScaleTwice) {
				// Need to do 2 pass scale down for Primary image
				// 1. Scale primary image to PLAY_IMETMP_WIDTHxPLAY_IMETMP_HEIGHT
				gximg_scale_data_fine(pRawImgBuf, GXIMG_REGION_MATCH_IMG, g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG);
				// 2. Scale MIDDLE_SIZE_WxMIDDLE_SIZE_H to thumbnail size
				gximg_scale_data_fine(g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG, &m_DstImg4SpecFile, &DstRegion);
			} else {
				gximg_scale_data(pRawImgBuf, GXIMG_REGION_MATCH_IMG, &m_DstImg4SpecFile, &DstRegion);
			}
		} else {
			// Clear 1st frame for NG files
			gximg_fill_data(&m_DstImg4SpecFile, &DstRegion, g_uiDefaultBGColor);
		}
	}

	#if 0
	if (gPBCmdObjPlaySpecFile.PlayFileVideo == PLAY_SPEC_FILE_IN_VIDEO_2) {
		IRECT   WindowRegin;
		ISIZE            devsize = {0};
		devsize = GxVideo_GetDeviceSize(DOUT1);
		WindowRegin.x = DstRegion.x * devsize.w / usPBDisplayWidth;
		WindowRegin.y = DstRegion.y * devsize.h / usPBDisplayHeight;
		WindowRegin.w = DstRegion.w * devsize.w / usPBDisplayWidth;
		WindowRegin.h = DstRegion.h * devsize.h / usPBDisplayHeight;
		GxDisp_SetData(GXDISP_VIDEO2, &m_DstImg4SpecFile, &DstRegion, &WindowRegin);
	}
	#endif

#endif //_TODO

	PB_SetFinishFlag(DECODE_JPG_DONE);
}

