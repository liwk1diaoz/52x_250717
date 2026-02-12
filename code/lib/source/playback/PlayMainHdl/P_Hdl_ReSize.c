//#include "FileSysTsk.h"
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#if _TODO
#include "PBXEdit.h"
#include "GxImageFile.h"
#endif

#if _TODO
// Scale primary image to IMETmp buffer
static void PB_ScaleAndKeepImgRatio4ReSize(UINT32 uiSrcWidth, UINT32 uiSrcHeight, UINT32 uiFileFormat, UINT32 uiDstWidth, UINT32 uiDstHeight)
{
	#if _TODO
	UINT32 uiOriDstWidth; //uiOriDstHeight
	PB_KEEPAR ImageRatioTrans;
	PBX_EDIT_PARAM  EditParam = {0};
	UINT32          Param[2] = {0};
	//IRECT           DstRegion = {0};

	uiOriDstWidth  = uiDstWidth;
	//uiOriDstHeight = uiDstHeight;

	//#NT#2011/01/27#Ben Wang -begin
	//#NT#check available buffer size
	//if(guiPlayIMETmpBuf_Cr + (uiOriDstWidth * uiOriDstHeight >> 1) > guiPlayDecoded_Y)
	//if (GxImg_InitBuf(&g_uiPBIMETmpBuf, uiDstWidth, uiDstHeight, GX_IMAGE_PIXEL_FMT_YUV422, GXIMAGE_LINEOFFSET_ALIGN(16), guiPlayIMETmpBufAddr, guiPlayIMETmpBufSize)!=E_OK)
	//{
	//    DBG_ERR("Buffer is NOT enough for keeping image ratio!!! \r\n");
	//    return;
	//}
	//#NT#2011/01/27#Ben Wang -end

	ImageRatioTrans = PB_GetImageRatioTrans(uiSrcWidth, uiSrcHeight, uiDstWidth, uiDstHeight);
	if ((ImageRatioTrans) || (uiSrcWidth < uiSrcHeight)) {
		if (ImageRatioTrans == PB_KEEPAR_LETTERBOXING) {
			uiDstHeight = uiOriDstWidth * uiSrcHeight / uiSrcWidth;
			//DstRegion.x = ((uiOriDstHeight - uiDstHeight)>>1);
		} else if ((ImageRatioTrans == PB_KEEPAR_PILLARBOXING) || (uiSrcWidth < uiSrcHeight)) {
			////////////////////////////////////////////////////////////////////
			// Alignment for IME Scaling function
			////////////////////////////////////////////////////////////////////
			uiDstWidth = ALIGN_CEIL_8(uiDstHeight * uiSrcWidth / uiSrcHeight);
			//DstRegion.y = ((uiOriDstWidth - uiDstWidth)>> 1);
		}
		// Clear buffer
		//GxImg_FillData(g_pPlayIMETmpImgBuf, REGION_MATCH_IMG, COLOR_YUV_BLACK);
	}
	//DstRegion.w = uiDstWidth;
	//DstRegion.h = uiDstHeight;

	//GxImg_ScaleData(g_pDecodedImgBuf, REGION_MATCH_IMG ,g_pPlayIMETmpImgBuf, &DstRegion, SCALE_BY_IME);
	EditParam.pSrcImg = g_pDecodedImgBuf;
	EditParam.pOutImg = g_pPlayIMETmpImgBuf;
	EditParam.workBuff = g_uiPBIMETmpBuf;
	EditParam.workBufSize = guiPlayIMETmpBufSize;
	EditParam.paramNum = 2;
	Param[0] = uiDstWidth;
	Param[1] = uiDstHeight;
	EditParam.param = Param;
	PBXEdit_SetFunc(EDIT_Resize);
	if (PBXEdit_Apply(&EditParam) != E_OK) {
		DBG_ERR("Buffer is NOT enough for keeping image ratio!!! \r\n");
	}
	#endif
}
#endif

/*
    Re-down-resoultion function.
    This is internal API.

    @param[in] IfOverWrite
    @return E_OK: OK, E_SYS: NG
*/
INT32 PB_ReResoultHandle(UINT32 IfOverWrite)
{
	#if _TODO
	UINT32 OriFileStartAddr, OriFileHeadSize;
	//UINT32 NewFileStartAddr, NewFileBitsAddr;
	//INT32  NewFileSize;
	UINT32 NewImgWidth, NewImgHeight; //NewImgFormat
	UINT32 OriImgWidth, OriImgHeight, OriImgFormat;
	INT32  ErrorCheck;
//#NT#2010/08/27#Ben Wang -begin
//#Place Hidden image after primary image
	// UINT32 TempHiddenAddr = 0;
//#NT#2010/08/27#Ben Wang -end
	//UINT32 DecStartAddr, DecEndAddr;
	PJPGHEAD_DEC_CFG pJPGInfo = gMenuPlayInfo.pJPGInfo;
	MEM_RANGE TmpBistream = {0}, ScreennailJPG = {0}, ExifData = {0}, PrimaryJPG = {0};
	BOOL bIsLittleEndian;
	EXIF_GETTAG exifTag;
	MAKERNOTE_INFO MakernoteInfo = {0};
	ER Ret;

	// (1) get current image info
	OriImgWidth  = pJPGInfo->imagewidth;
	OriImgHeight = pJPGInfo->imageheight;
	OriImgFormat = pJPGInfo->fileformat;

	//fix two problems
	//(1) When the raw data is very large, it would overwrite the JPEG Header.
	//And read the JPEG Header to fix the error Header.
	//(2) Input file may use nonstandard HT table. But encoding the raw data, use standard HT table
	// replace the standard HT to JPEG Header.
#if 0
	if (!pJPGInfo->bstandardHT) {
		UINT8 *pOriJPGHeader = (UINT8 *)((UINT32)(pJPGInfo->inbuf + pJPGInfo->headerlen + 15) & 0xfffffff0);

		PB_ReadFileByFileSys(PB_FILE_READ_CONTINUE, (INT8 *)pOriJPGHeader, pJPGInfo->headerlen, 0, gMenuPlayInfo.JumpOffset);
		JpegReSizeGenerateJpegHeader(pJPGInfo, pOriJPGHeader);
		Jpg_ParseHeader(pJPGInfo, PB_DEC_PRIMARY);
	} else {
		PB_ReadFileByFileSys(PB_FILE_READ_CONTINUE, (INT8 *)pJPGInfo->inbuf, pJPGInfo->headerlen, 0, gMenuPlayInfo.JumpOffset);
	}
#endif
	OriFileHeadSize = pJPGInfo->headerlen;
	OriFileStartAddr = (UINT32)pJPGInfo->inbuf;

	NewImgWidth  = gusPlayResizeNewWidth;
	NewImgHeight = gusPlayResizeNewHeight;
	//NewImgFormat = OriImgFormat;

	// (2) do not re-encode scale-up file
	if ((NewImgWidth >= OriImgWidth) || (NewImgHeight >= OriImgHeight)) {
		return E_SYS;
	}

	//#NT#2011/01/27#Ben Wang -begin
	//#NT#keep image ratio by padding
	PB_ScaleAndKeepImgRatio4ReSize(OriImgWidth, OriImgHeight, OriImgFormat, NewImgWidth, NewImgHeight);
	// Update the new-primary image info
	pJPGInfo->imagewidth  = NewImgWidth;
	pJPGInfo->imageheight = NewImgHeight;
	pJPGInfo->lineoffsetY  = NewImgWidth;
	pJPGInfo->lineoffsetUV = NewImgWidth >> 1;
	//guiPlayDecoded_Y  = guiPlayIMETmpBuf_Y;
	//guiPlayDecoded_Cb = guiPlayIMETmpBuf_Cb;
	//guiPlayDecoded_Cr = guiPlayIMETmpBuf_Cr;
	memcpy(g_pDecodedImgBuf, g_pPlayIMETmpImgBuf, sizeof(VDO_FRAME));
	//#NT#2011/01/27#Ben Wang -end
	// (3) re-encode jpg file
	//NewFileStartAddr = OriFileStartAddr;
	//NewFileBitsAddr  = OriFileStartAddr + OriFileHeadSize;
	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_MAKERNOTE, &MakernoteInfo, sizeof(MakernoteInfo));
	//Let ScreennailJPG pointer to the original hidden JPG
	ScreennailJPG.Addr = OriFileStartAddr + MakernoteInfo.uiScreennailOffset;
	ScreennailJPG.Size = MakernoteInfo.uiScreennailSize;
	//Let ExifData pointer to the original EXIF
	ExifData.Addr = OriFileStartAddr;
	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_EXIF_HEADER_SIZE, &ExifData.Size, sizeof(ExifData.Size));
	PrimaryJPG.Addr = ExifData.Addr + ExifData.Size;
	PrimaryJPG.Size = (g_uiPBBufEnd - guiPlayFileBuf - OriFileHeadSize - MakernoteInfo.uiScreennailSize);
#if 0
	PrimaryJPG.Size = PB_ReEncodeHandle(NewImgWidth, NewImgHeight, PrimaryJPG.Addr, PrimaryJPG.Size);
	if (PrimaryJPG.Size == E_SYS) {
		return E_SYS;
	}
#endif
	//if(E_OK != GxImgFile_Encode(g_pDecodedImgBuf, &PrimaryJPG))
	if (E_OK != PB_JpgBRCHandler(g_pDecodedImgBuf, &PrimaryJPG)) {
		DBG_ERR("ReEnc primary data NG!\r\n");
		return E_SYS;
	}
	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));

	exifTag.uiTagIfd = EXIF_IFD_EXIF;
	exifTag.uiTagId = (UINT16)TAG_ID_PIXEL_X_DIMENSION;
	Ret = EXIF_GetTag(EXIF_HDL_ID_1, &exifTag);
	if (EXIF_ER_OK == Ret) {
		Set32BitsData(ExifData.Addr + exifTag.uiTagOffset, NewImgWidth, bIsLittleEndian);
		exifTag.uiTagId = (UINT16)TAG_ID_PIXEL_Y_DIMENSION;
		Ret = EXIF_GetTag(EXIF_HDL_ID_1, &exifTag);
		if (EXIF_ER_OK == Ret) {
			Set32BitsData(ExifData.Addr + exifTag.uiTagOffset, NewImgHeight, bIsLittleEndian);
		}
	}
	if (EXIF_ER_OK != Ret) {
		ExifData.Size = 0;
		ScreennailJPG.Size = 0;
		DBG_ERR("Exif data error ! Just save primary data.\r\n");
	}
	GxImgFile_CombineJPG(&ExifData, &PrimaryJPG, &ScreennailJPG, &TmpBistream);
	ErrorCheck = PB_WriteFileByFileSys(TmpBistream.Addr, TmpBistream.Size, IfOverWrite, FALSE);

	if (ErrorCheck == E_OK) {
		return E_OK;
	} else {
		return E_SYS;
	}
	#else
	return E_OK;
	#endif
}

