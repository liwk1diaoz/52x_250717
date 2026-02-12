//#include "FileSysTsk.h"
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#if _TODO
#include "GxImageFile.h"
#endif

//#NT#2012/08/13#Scottie -begin
//#NT#Add temporarily path for FPGA (V6)
#define _PB_FORCE_SW DISABLE
#if (ENABLE == _PB_FORCE_SW)
#include <string.h>
#endif
//#NT#2012/08/13#Scottie -end

/**
    Jpeg resize update header.
*/
#if _TODO
static void xReSizeUpdateHeader(PJPGHEAD_DEC_CFG pJPGInfo, UINT32 NewWidth, UINT32 NewHeight, UINT32 NewFormat)
{
	#if _TODO
	//UINT32  NewWidth1st, NewWidth2nd, NewHeight1st, NewHeight2nd;
	UINT8   *pJPGHeader;
	EXIF_GETTAG exifTag;
	BOOL bIsLittleEndian = FALSE;

	// get new image info
	pJPGInfo->imagewidth  = NewWidth;
	pJPGInfo->imageheight = NewHeight;
	pJPGInfo->fileformat  = NewFormat;

	// update new info into header & exif header
	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));
	exifTag.uiTagIfd = EXIF_IFD_EXIF;
	exifTag.uiTagId = TAG_ID_PIXEL_X_DIMENSION;
	if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
		Set32BitsData((UINT32)pJPGInfo->inbuf + exifTag.uiTagOffset, NewWidth, bIsLittleEndian);

	}
	exifTag.uiTagId = (UINT16)TAG_ID_PIXEL_Y_DIMENSION;
	if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
		Set32BitsData((UINT32)pJPGInfo->inbuf + exifTag.uiTagOffset, NewHeight, bIsLittleEndian);
	}
	pJPGHeader      = (UINT8 *)(pJPGInfo->pTagSOF);
	//pJPGHeader      += 3;
	*pJPGHeader++   = (UINT8)((NewHeight >> 8) & 0xFF);
	*pJPGHeader++   = (UINT8)(NewHeight & 0xFF);
	*pJPGHeader++   = (UINT8)((NewWidth >> 8) & 0xFF);
	*pJPGHeader     = (UINT8)(NewWidth & 0xFF);

	pJPGHeader += 3;
	if (NewFormat == PB_JPG_FMT_YUV420) {
		// format YUV420
		*pJPGHeader = 0x22;
	} else if (NewFormat == PB_JPG_FMT_YUV211) {
		// format YUV422
		*pJPGHeader = 0x21;
	}
//#NT#2009/06/22#Corey Ke -begin
//#NT#Support JPEG 211 format
	else if (NewFormat == PB_JPG_FMT_YUV211V) {
		// format YUV422
		*pJPGHeader = 0x12;
	}
//#NT#2009/06/22#Corey Ke -end
	else if (NewFormat == JPG_FMT_YUV444) {
		// format YUV422
		*pJPGHeader = 0x11;
	}
	//pJPGHeader += 3;
	//*pJPGHeader = 0x11;
	//pJPGHeader += 3;
	//*pJPGHeader = 0x11;
	#endif
}
#endif

INT32 PB_SaveCurrRawBuf(UINT32 SrcStart_X, UINT32 SrcStart_Y, UINT32 SrcWidth, UINT32 SrcHeight, UINT32 IfOverWrite)
{
	#if _TODO
	UINT32 OriFileStartAddr, OriFileHeadSize;
	UINT32 NewFileStartAddr, NewFileBitsAddr;
	UINT32 NewImgWidth, NewImgHeight, NewImgFormat;
	UINT32 OriImgFormat; //OriImgWidth, OriImgHeight
	//UINT32 UVDataSize;
	UINT32 Buf_Thumb, *pThumbBuffer, i, ThumbSize;
	UINT32 tmpThumbWidth, tmpThumbHeight, ThumbStartX, ThumbStartY;
	float  dstRatio, srcRatio;
	UINT32 NewApp1Size = 0;
//#NT#2009/06/03#Ben Wang -begin
//#Place Hidden image after primary image
	UINT32 ThumbWidth = 0, ThumbHeight = 0, ThumbCnt, j = 0;
	INT32  ErrorCheck;
	UINT32 ScreenNailWidth = 0, ScreenNailHeight = 0;
//#NT#2009/06/03#Ben Wang -end
	//UINT32 tmpBuf_Y, tmpBuf_Cb, tmpBuf_Cr;
	PJPGHEAD_DEC_CFG pJPGInfo = gMenuPlayInfo.pJPGInfo;
	//PPLAY_SCALE_INFO pScale = &gPlayScaleInfo;
	VDO_FRAME   ThumbImg = {0};
	IRECT     SrcRegion = {0};
	IRECT     DstRegion = {0};
	UINT32    DecStartAddr, DecEndAddr = 0;
	MEM_RANGE TmpBistream = {0}, ScreennailJPG = {0}, ExifData = {0}, PrimaryJPG = {0};
	//UINT32 uiCompressedRatio, uiTargetSize;
	BOOL bIsLittleEndian;
	EXIF_GETTAG exifTag;
	MAKERNOTE_INFO MakernoteInfo = {0};

	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));


	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_MAKERNOTE, &MakernoteInfo, sizeof(MakernoteInfo));
	if (MakernoteInfo.uiScreennailOffset) {
		//#NT#2013/03/06#Ben Wang -begin
		//#NT#Jpg_ParseHeader() only supports primary and thumbnail image.
#if 0
		BOOL bTmpVar = pJPGInfo->bIfSpeedUpSN;
		pJPGInfo->bIfSpeedUpSN = FALSE;
		Jpg_ParseHeader(pJPGInfo, DEC_HIDDEN);
		ScreenNailWidth = pJPGInfo->imagewidth;
		ScreenNailHeight = pJPGInfo->imageheight;
		pJPGInfo->bIfSpeedUpSN = bTmpVar;
		Jpg_ParseHeader(pJPGInfo, DEC_PRIMARY);
#else
		MEM_RANGE Screennail;
		GXIMG_HEADER_INFO HeaderInfo = {0};
		Screennail.Addr = (UINT32)pJPGInfo->inbuf + MakernoteInfo.uiScreennailOffset;
		Screennail.Size = MakernoteInfo.uiScreennailSize;
		GxImgFile_ParseHeader(&Screennail, &HeaderInfo);
		ScreenNailWidth = HeaderInfo.uiImageWidth;
		ScreenNailHeight = HeaderInfo.uiImageHeight;
#endif
		//#NT#2013/03/06#Ben Wang -end
	}
	//OriImgWidth = pJPGInfo->imagewidth;
	//OriImgHeight = pJPGInfo->imageheight;
	OriImgFormat = pJPGInfo->fileformat;
	OriFileHeadSize = pJPGInfo->headerlen;
	OriFileStartAddr = (UINT32)pJPGInfo->inbuf;

	NewImgWidth  = SrcWidth;
	NewImgHeight = SrcHeight;
	NewImgFormat = OriImgFormat;

	/*if(OriImgFormat == PB_JPG_FMT_YUV211)
	{
	    UVDataSize = ((OriImgWidth * OriImgHeight) >> 1);
	}
	else //if(OriImgFormat == PB_JPG_FMT_YUV420)
	{
	    UVDataSize = ((OriImgWidth * OriImgHeight) >> 2);
	}*/

	NewFileStartAddr = g_uiPBTmpBuf;
	NewFileBitsAddr = NewFileStartAddr + OriFileHeadSize;

	// (2) update new header
	xReSizeUpdateHeader(pJPGInfo, NewImgWidth, NewImgHeight, NewImgFormat);
	// (3) move header to temp buffer
#if (DISABLE == _PB_FORCE_SW)
	hwmem_open();
	hwmem_memcpy(NewFileStartAddr, OriFileStartAddr, OriFileHeadSize);
	hwmem_close();
#else
	memcpy((void *)NewFileStartAddr, (void *)OriFileStartAddr, OriFileHeadSize);
#endif
//#NT#2011/01/11#Scottie -begin
//#NT#Add to avoid useless processing
	SrcRegion.x = SrcStart_X;
	SrcRegion.y = SrcStart_Y;
	SrcRegion.w = SrcWidth;
	SrcRegion.h = SrcHeight;

	//Buf_Thumb = guiPlayDecoded_Cr + UVDataSize;
	gximg_get_buf_addr(g_pDecodedImgBuf, &DecStartAddr, &DecEndAddr);
	Buf_Thumb = DecEndAddr;

	if (g_bExifExist) {
		pThumbBuffer = (UINT32 *)Buf_Thumb;

		if (MakernoteInfo.uiScreennailOffset != 0) {
			ThumbCnt = 2;       // hidden jpg (PB_SCREENNAIL_WIDTH*PB_SCREENNAIL_HEIGHT)
		} else {
			ThumbCnt = 1;
		}

		//-----------------------------------------
		// rotate & encode 1 or 2 thumbnail image
		//-----------------------------------------
		for (j = 0; j < ThumbCnt; j++) {
			if (j == 0) {
				// Thumbnail
				ThumbWidth  = 160;
				ThumbHeight = 120;
				Buf_Thumb = (UINT32)pThumbBuffer;

				ThumbSize = (ThumbWidth * ThumbHeight);

				for (i = 0; i < ((ThumbSize) >> 2); i++) { // 4pixels
					*pThumbBuffer++ = 0;
				}
				for (i = 0; i < ((ThumbSize) >> 2); i++) { // 4pixels
					*pThumbBuffer++ = 0x80808080;
				}

				if (NewImgWidth < NewImgHeight) {
					tmpThumbHeight = ThumbHeight;
					tmpThumbWidth = ALIGN_CEIL_16((SrcWidth * tmpThumbHeight) / SrcHeight);
					ThumbStartX = ((ThumbWidth - tmpThumbWidth) >> 1);
					ThumbStartY = 0;
				} else {
					srcRatio = (float)SrcWidth / (float)SrcHeight;
					dstRatio = (float)ThumbWidth / (float)ThumbHeight;
					srcRatio = (INT32)(srcRatio * 100);
					dstRatio = (INT32)(dstRatio * 100);
					if ((srcRatio > dstRatio) && ((srcRatio - dstRatio) > 10)) {
						tmpThumbWidth = ThumbWidth;
						tmpThumbHeight = ALIGN_CEIL_8(tmpThumbWidth * SrcHeight / SrcWidth);

						ThumbStartX = 0;
						ThumbStartY = (ThumbHeight - tmpThumbHeight) >> 1;
					} else if ((srcRatio < dstRatio) && ((dstRatio - srcRatio) > 10)) {
						tmpThumbHeight = ThumbHeight;
						tmpThumbWidth = ALIGN_CEIL_16(tmpThumbHeight * SrcWidth / SrcHeight);

						ThumbStartY = 0;
						ThumbStartX = (ThumbWidth - tmpThumbWidth) >> 1;
					} else {
						tmpThumbWidth = ThumbWidth;
						tmpThumbHeight = ThumbHeight;
						ThumbStartX = 0;
						ThumbStartY = 0;
					}
				}
			} else { // if(j == 1)
				// Screennail
				//#NT#2009/06/03#Ben Wang -begin
				//#Place Hidden image after primary image
				if (NewImgWidth < NewImgHeight) {
					ThumbWidth  = ScreenNailHeight;
					ThumbHeight = ScreenNailWidth;
				} else {
					ThumbWidth  = ScreenNailWidth;
					ThumbHeight = ScreenNailHeight;
				}
				//#NT#2009/06/03#Ben Wang -end
				Buf_Thumb = (UINT32)pThumbBuffer;
				ThumbSize = (ThumbWidth * ThumbHeight);

				for (i = 0; i < ((ThumbSize) >> 2); i++) { // 4pixels
					*pThumbBuffer++ = 0;
				}
				for (i = 0; i < ((ThumbSize) >> 2); i++) { // 4pixels
					*pThumbBuffer++ = 0x80808080;
				}

				srcRatio = (float)SrcWidth / (float)SrcHeight;
//#NT#2011/01/10#Scottie -begin
//#NT#Add to remove warning [414] of PCLint
				if (0 == ThumbHeight) {
					DBG_ERR("division by 0 (1) !\r\n");
					return E_SYS;
				} else
//#NT#2011/01/10#Scottie -end
				{
					dstRatio = (float)ThumbWidth / (float)ThumbHeight;
				}

				srcRatio = (INT32)(srcRatio * 100);
				dstRatio = (INT32)(dstRatio * 100);
				if ((srcRatio > dstRatio) && ((srcRatio - dstRatio) > 10)) {
					tmpThumbWidth = ThumbWidth;
					tmpThumbHeight = ALIGN_CEIL_8(tmpThumbWidth * SrcHeight / SrcWidth);

					ThumbStartX = 0;
					ThumbStartY = (ThumbHeight - tmpThumbHeight) >> 1;
				} else if ((srcRatio < dstRatio) && ((dstRatio - srcRatio) > 10)) {
					tmpThumbHeight = ThumbHeight;
					tmpThumbWidth = ALIGN_CEIL_16(tmpThumbHeight * SrcWidth / SrcHeight);

					ThumbStartY = 0;
					ThumbStartX = (ThumbWidth - tmpThumbWidth) >> 1;
				} else {
					tmpThumbWidth = ThumbWidth;
					tmpThumbHeight = ThumbHeight;
					ThumbStartX = 0;
					ThumbStartY = 0;
				}

				//pScale->out_format = UT_IMAGE_FMT_YUV422;
			}

//#NT#2009/01/05#Corey Ke -begin
//#Bug fix for IME 32 multiple limitition
//#NT#2011/01/10#Scottie -begin
//#NT#Add to remove warning [414] of PCLint
			if (0 == tmpThumbWidth) {
				DBG_ERR("division by 0 (2) !\r\n");
				return E_SYS;
			} else
//#NT#2011/01/10#Scottie -end
				if ((SrcWidth / tmpThumbWidth) > 31) {

					VDO_FRAME   TmpImg;


					gximg_init_buf(&TmpImg, tmpThumbWidth << 1, tmpThumbHeight << 1, VDO_PXLFMT_YUV422, tmpThumbWidth << 1, (UINT32)pThumbBuffer, ThumbSize << 3);
					gximg_init_buf(&ThumbImg, ThumbWidth, ThumbHeight, VDO_PXLFMT_YUV422, ThumbWidth, Buf_Thumb, ThumbSize << 1);
					//#NT#2012/03/06#Lincy Lin -begin
					//#NT#change GxImg scale function interface
					//GxImg_ScaleData(g_pDecodedImgBuf, &SrcRegion ,&TmpImg, REGION_MATCH_IMG, SCALE_BY_IME);
					gximg_scale_data_fine(g_pDecodedImgBuf, &SrcRegion, &TmpImg, GXIMG_REGION_MATCH_IMG);
					//#NT#2012/03/06#Lincy Lin -end
					DstRegion.w = tmpThumbWidth;
					DstRegion.h = tmpThumbHeight;
					DstRegion.x = ThumbStartX;
					DstRegion.y = ThumbStartY;
					//#NT#2012/03/06#Lincy Lin -begin
					//#NT#change GxImg scale function interface
					//GxImg_ScaleData(&TmpImg, REGION_MATCH_IMG ,&ThumbImg, &DstRegion, SCALE_BY_IME);
					gximg_scale_data_fine(&TmpImg, GXIMG_REGION_MATCH_IMG, &ThumbImg, &DstRegion);
					//#NT#2012/03/06#Lincy Lin -end
				} else {

					gximg_init_buf(&ThumbImg, ThumbWidth, ThumbHeight, VDO_PXLFMT_YUV422, ThumbWidth, (UINT32)Buf_Thumb, ThumbSize << 1);
					DstRegion.w = tmpThumbWidth;
					DstRegion.h = tmpThumbHeight;
					DstRegion.x = ThumbStartX;
					DstRegion.y = ThumbStartY;
					//#NT#2012/03/06#Lincy Lin -begin
					//#NT#change GxImg scale function interface
					//GxImg_ScaleData(g_pDecodedImgBuf, &SrcRegion ,&ThumbImg, &DstRegion, SCALE_BY_IME);
					gximg_scale_data_fine(g_pDecodedImgBuf, &SrcRegion, &ThumbImg, &DstRegion);
					//#NT#2012/03/06#Lincy Lin -end
				}
//#NT#2009/01/05#Corey Ke -end
			//set available bitstream size
			if (j == 0) {
				TmpBistream.Size = MAX_APP1_SIZE;
			} else { // if(j == 1)
				TmpBistream.Size = (PB_DEFAULT_FILEBUFSIZE - FST_READ_THUMB_BUF_SIZE);
			}
			TmpBistream.Addr = (UINT32)pThumbBuffer;
#if 0
			uiCompressedRatio = GxImgFile_CalcCompressedRatio(&ThumbImg, uiTargetSize);
			if (E_OK != GxImgFile_EncodeByBitrate(&ThumbImg, &TmpBistream, uiCompressedRatio))
#endif
				//#NT#2012/10/25#Ben Wang -begin
				//#NT#E_OBJ means BRC reach limitation or max retry count, but the bitstream might be correct.
				ErrorCheck = PB_JpgBRCHandler(&ThumbImg, &TmpBistream);
			if (E_OK != ErrorCheck && E_OBJ != ErrorCheck)
				//#NT#2012/10/25#Ben Wang -end
			{
				DBG_ERR("Crop ReEnc Thumb/SN NG!\r\n");
				return E_SYS;
			}
			if (j == 0) {
				UINT32 ThumbnailOffset, uiApp1SizeOffset, uiTiffOffsetBase;
				//copy thumbnail JPG
				exifTag.uiTagIfd = EXIF_IFD_1ST;
				exifTag.uiTagId = TAG_ID_INTERCHANGE_FORMAT;
				EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_TIFF_OFFSET_BASE, &uiTiffOffsetBase, sizeof(uiTiffOffsetBase));
				if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
					ThumbnailOffset = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian) + uiTiffOffsetBase;
				} else {
					DBG_ERR("No Thumbnail!\r\n");
					return E_SYS;
				}
//#NT#2012/08/13#Scottie -begin
//#NT#Add temporarily path for FPGA
#if (DISABLE == _PB_FORCE_SW)
				hwmem_open();
				hwmem_memcpy(NewFileStartAddr + ThumbnailOffset, TmpBistream.Addr, TmpBistream.Size);
				hwmem_close();
#else
				memcpy((void *)NewFileStartAddr + ThumbnailOffset, (void *)TmpBistream.Addr, TmpBistream.Size);
#endif
//#NT#2012/08/13#Scottie -end
				//update thumbnail size
				exifTag.uiTagIfd = EXIF_IFD_1ST;
				exifTag.uiTagId = TAG_ID_INTERCHANGE_FORMAT_LENGTH;
				if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
					Set16BitsData(NewFileStartAddr + exifTag.uiTagOffset, TmpBistream.Size, bIsLittleEndian);
					DBG_MSG("ThumbnailOffset=0x%X, OffsetThumbJPGSize=0x%x, TmpBitstream Addr=0x%x,Size=%d\r\n", ThumbnailOffset, exifTag.uiTagOffset, TmpBistream.Addr, TmpBistream.Size);
				} else {
					DBG_ERR("No Thumbnail!\r\n");
					return E_SYS;
				}

				ExifData.Addr = NewFileStartAddr;
				ExifData.Size = ThumbnailOffset + TmpBistream.Size;
				NewFileBitsAddr = ALIGN_CEIL_4(ExifData.Addr + ExifData.Size);//
				//update App1 size
#if 0//temp reserved for verification
				NewApp1Size = ExifData.Size - APP1_SIZE_OFFSET;
				Set16BitsData(NewFileStartAddr + APP1_SIZE_OFFSET, NewApp1Size, FALSE);
#else
				EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_APP1_SIZE_OFFSET, &uiApp1SizeOffset, sizeof(uiApp1SizeOffset));
				NewApp1Size = ExifData.Size - uiApp1SizeOffset;
				Set16BitsData(NewFileStartAddr + uiApp1SizeOffset, NewApp1Size, FALSE);
#endif
				DBG_MSG("NewApp1Size=%d\r\n", NewApp1Size);


			} else { //(j == 1)
				ScreennailJPG.Addr = TmpBistream.Addr;
				ScreennailJPG.Size = TmpBistream.Size;
			}
		}   // End of for(j=0; j<ThumbCnt; j++)
	}
//#NT#2009/06/03#Ben Wang -end

	// (5) re-encode new rotated-JPG
	PrimaryJPG.Addr = NewFileBitsAddr;
	PrimaryJPG.Size = guiPlayFileBuf - NewFileStartAddr;
	//#NT#2012/10/25#Ben Wang -begin
	//#NT#E_OBJ means BRC reach limitation or max retry count, but the bitstream might be correct.
	ErrorCheck = PB_JpgBRCHandler(g_pDecodedImgBuf, &PrimaryJPG);
	if (E_OK != ErrorCheck && E_OBJ != ErrorCheck)
		//#NT#2012/10/25#Ben Wang -end
	{
		DBG_ERR("ReEnc primary data NG!\r\n");
		return E_SYS;
	}
	GxImgFile_CombineJPG(&ExifData, &PrimaryJPG, &ScreennailJPG, &TmpBistream);
	ErrorCheck = PB_WriteFileByFileSys(TmpBistream.Addr, TmpBistream.Size, IfOverWrite, FALSE);
	if (ErrorCheck == E_OK) {
		return E_OK;
	} else {
		DBG_ERR("Crop Fail(5)\r\n");
		return E_SYS;
	}
	#else
	return E_OK;
	#endif
}

