//#include "FileSysTsk.h"
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
//#include "JpgEnc.h"

/*
    Re-down-qty function.
    This is internal API.

    @param[in] IfOverWrite:
    @return E_OK: OK, E_SYS: NG
*/
INT32 PB_ReQualityHandle(UINT32 IfOverWrite)
{
#if 0//_MIPS_TODO
	PJPGHEAD_DEC_CFG    pJPGInfo;
	UINT32  OriFileStartAddr, OriFileHeadSize;
	UINT32  NewFileStartAddr, NewFileBitsAddr, NewFileSize;
	UINT32  NewQualityIndex;
	//RE_ENCODE_INFO  ReEncodeInfo;
	INT32   ErrorCheck;
//#NT#2010/08/27#Ben Wang -begin
//#Place Hidden image after primary image
	UINT32 TempHiddenAddr = 0;
//#NT#2010/08/27#Ben Wang -end
	GXIMG_ENCODE_INFO GxImgEnInfo = {0};

	// (1) get current image info
	pJPGInfo = (PJPGHEAD_DEC_CFG)gMenuPlayInfo.pJPGInfo;

	// (2) check if image created by ourself
	if ((pJPGInfo->pExifInfo == NULL) || (pJPGInfo->pExifInfo->JPGQuality == 0)) {
		return E_SYS;
	}

	// (3) check if quality down
	if (gucPlayReqtyQuality > 94) {
		NewQualityIndex = Quality_SuperHigh;
	} else if (gucPlayReqtyQuality > 89) {
		NewQualityIndex = Quality_High;
	} else {
		NewQualityIndex = Quality_Economy;
	}

	// 1->SuperHigh; 2->High; 3->Economy
	if (NewQualityIndex < pJPGInfo->pExifInfo->JPGQuality) {
		return E_SYS;
	}

	// (4) update new Q-table into header
	JpegReQtyUpdateHeader(pJPGInfo, gucPlayReqtyQuality);

	// (5) re-encode new Q-table JPG
	OriFileHeadSize  = pJPGInfo->headerlen;
	OriFileStartAddr = (UINT32)pJPGInfo->inbuf;
	NewFileStartAddr = OriFileStartAddr;
	NewFileBitsAddr  = OriFileStartAddr + OriFileHeadSize;
//#NT#2010/08/27#Ben Wang -begin
//#Place Hidden image after primary image
	NewFileSize = (g_uiPBBufEnd - guiPlayFileBuf - OriFileHeadSize - pJPGInfo->pExifInfo->ScreeNailSize); // bitstream buffer size
//#NT#2012/01/03#Scottie -begin
//#NT#Porting GxImg to PB
//    TempHiddenAddr = guiPlayDecoded_Cb + pJPGInfo->imagewidth*pJPGInfo->imageheight;
	TempHiddenAddr = gDecodedImgBuf.PxlAddr[0];
	if (GX_IMAGE_PIXEL_FMT_YUV420 == gDecodedImgBuf.PxlFmt) {
		TempHiddenAddr += (gDecodedImgBuf.LineOffset[0] * gDecodedImgBuf.Height * 3 / 2);
	} else {
		TempHiddenAddr += (gDecodedImgBuf.LineOffset[0] * gDecodedImgBuf.Height * 2);
	}
//#NT#2012/01/03#Scottie -end
	if ((TempHiddenAddr + (PB_SCREENNAIL_WIDTH * PB_SCREENNAIL_HEIGHT * 2)) >= guiPlayFileBuf) {
		DBG_ERR("Buffer is NOT enough for re-encoding hidden image!!! \r\n");
		pJPGInfo->pExifInfo->OffsetHiddenJPG = 0;
	}
	if (pJPGInfo->pExifInfo->OffsetHiddenJPG != 0) {
		hwmem_open();
		//backup original hidden JPG
		hwmem_memcpy(TempHiddenAddr,
					 OriFileStartAddr + pJPGInfo->pExifInfo->OffsetHiddenJPG,
					 pJPGInfo->pExifInfo->ScreeNailSize);
		hwmem_close();
	}
//#NT#2010/08/27#Ben Wang -end

	/*
	ReEncodeInfo.uiBufAddrY    = guiPlayDecoded_Y;
	ReEncodeInfo.uiBufAddrCb   = guiPlayDecoded_Cb;
	ReEncodeInfo.uiBufAddrCr   = guiPlayDecoded_Cr;
	ReEncodeInfo.uiOutBufAddr  = NewFileBitsAddr;
	ReEncodeInfo.uiOutBufSize  = NewFileSize;
	ReEncodeInfo.pQTabY        = pJPGInfo->pQTabY;
	ReEncodeInfo.pQTabUV       = pJPGInfo->pQTabUV;
	ReEncodeInfo.usImgWidth    = pJPGInfo->imagewidth;
	ReEncodeInfo.usImgHeight   = pJPGInfo->imageheight;
	ReEncodeInfo.ucImgFormat   = pJPGInfo->fileformat;
	ReEncodeInfo.ucTPOSEnable  = JPGROTATE_DISABLE;
	ReEncodeInfo.usRstInterval = pJPGInfo->rstinterval;
	if( JpegReEncodeHWSetting(&ReEncodeInfo) != E_OK )
	{
	    return E_SYS;
	}
	NewFileSize = ReEncodeInfo.uiOutBufSize;
	*/
	GxImgEnInfo.pSrcImg = &gDecodedImgBuf;
	GxImgEnInfo.outBufAddr = NewFileBitsAddr;
	GxImgEnInfo.param[0]  = pJPGInfo->pQTabY;
	GxImgEnInfo.param[1] = pJPGInfo->pQTabUV;
	GxImgEnInfo.outBufSize = NewFileSize;
	if (GxImg_Encode(&GxImgEnInfo) != E_OK) {
		return E_SYS;
	}
	NewFileSize = GxImgEnInfo.outBufSize;

	NewFileSize += (OriFileHeadSize);
//#NT#2010/08/27#Ben Wang -begin
//#Place Hidden image after primary image
	if (pJPGInfo->pExifInfo->OffsetHiddenJPG != 0) {
		hwmem_open();
		// copy ori-screennail to file end
		hwmem_memcpy(NewFileStartAddr + NewFileSize, TempHiddenAddr, pJPGInfo->pExifInfo->ScreeNailSize);
		hwmem_close();


		// update screennail related header info
		if (pJPGInfo->pExifInfo->OffsetPLAYPIC_OFFSET) {
			UINT8 *pTemp;
			pTemp = (UINT8 *)(NewFileStartAddr + pJPGInfo->pExifInfo->OffsetPLAYPIC_OFFSET);
			*pTemp++ = (UINT8)(NewFileSize & 0xFF);
			*pTemp++ = (UINT8)((NewFileSize >> 8) & 0xFF);
			*pTemp++ = (UINT8)((NewFileSize >> 16) & 0xFF);
			*pTemp   = (UINT8)((NewFileSize >> 24) & 0xFF);
		}
		NewFileSize += pJPGInfo->pExifInfo->ScreeNailSize;

	}
//#NT#2010/08/27#Ben Wang -end

	// (6) store file, write to the same\other file
//#NT#2009/11/23#Scottie -begin
//#NT#The write file related code is unified into PB_WriteFileByFileSys function
	ErrorCheck = PB_WriteFileByFileSys(NewFileStartAddr, NewFileSize, IfOverWrite, FALSE);
//#NT#2009/11/23#Scottie -end

	if (ErrorCheck == E_OK) {
		return E_OK;
	} else {
		return E_SYS;
	}
#else
	return E_SYS;
#endif//_MIPS_TODO
}
