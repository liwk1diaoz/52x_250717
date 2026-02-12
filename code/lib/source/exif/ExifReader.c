/*
    @file       ExifReader.c
    @ingroup    mILIBEXIFRead

    @brief      exif header reader
    @version    V1.01.005
    @date       2011/07/12

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#include <string.h>
#include "ExifTemp.h"
#include "ExifWriter.h"
#include "ExifReader.h"
#include "ExifRW.h"
#include "exif/ExifDef.h"

//#include "fs_file_op.h"
#define EXIFR_SEEK_SET 0 /* start of stream  */
#define EXIFR_SEEK_CUR 1 /* current position in stream  */
#define EXIFR_SEEK_END 2 /* end of stream  */

#define EXIF_ERRORTYPE   0xF0000000//2011/01/13 Meg Lin
#define MPO_IDENTIFIER   0x0046504D  //little endian
#define MPO_MAX_IFD_CNT   2



/**
    @addtogroup mILIBEXIFRead
*/
//@{

//  Initialize exif reader and set exif header address.
static void ExifR_MakeInit(PEXIF_CTRL pCtrl, UINT32 BaseAddr)
{

	pCtrl->ReadBufBase = BaseAddr;
	pCtrl->ReadOffset = 0;
	pCtrl->ReadTagIndex = 0;
	pCtrl->MpoOffsetBase = 0;
	pCtrl->TiffOffsetBase = APP1_TIFF_OFFSET_BASE;
	pCtrl->App1SizeOffset = APP1_SIZE_OFFSET;
	memset((char *)&pCtrl->FileInfo, 0, sizeof(pCtrl->FileInfo));
	memset((char *)&pCtrl->MakernoteInfo, 0, sizeof(pCtrl->MakernoteInfo));

}
static void ExifR_ReadSeekOffset(PEXIF_CTRL pCtrl, Exif_Offset pos, UINT16 type)
{
	if (type == EXIFR_SEEK_SET) {
		pCtrl->ReadOffset = pos;
	} else if (type == EXIFR_SEEK_CUR) {

		pCtrl->ReadOffset += pos;
	}
}

static void ExifR_ReadSkip(PEXIF_CTRL pCtrl, Exif_Offset size)
{
	ExifR_ReadSeekOffset(pCtrl, size, EXIFR_SEEK_CUR);
}
static void ExifR_ReadB16Bits(PEXIF_CTRL pCtrl, UINT16 *pvalue)//little
{
	UINT32 offset;
	UINT8 *pb;
	offset = pCtrl->ReadOffset;
	pb = (UINT8 *)pCtrl->ReadBufBase;
	*pvalue = *(pb + offset) << 8;
	*pvalue |= *(pb + offset + 1);

	pCtrl->ReadOffset += 2;
}
static void ExifR_ReadB32Bits(PEXIF_CTRL pCtrl, UINT32 *pvalue)//little
{
	UINT32 offset;
	UINT8 *pb;
	offset = pCtrl->ReadOffset;
	pb = (UINT8 *)pCtrl->ReadBufBase;
	*pvalue = (*(pb + offset)) << 24;
	*pvalue |= (*(pb + offset + 1)) << 16;
	*pvalue |= (*(pb + offset + 2)) << 8;
	*pvalue |= *(pb + offset + 3);

	pCtrl->ReadOffset += 4;
}

static void ExifR_ReadL16Bits(PEXIF_CTRL pCtrl, UINT16 *pvalue)//little
{
	UINT32 offset;
	UINT8 *pb;
	offset = pCtrl->ReadOffset;
	pb = (UINT8 *)pCtrl->ReadBufBase;
	*pvalue = *(pb + offset);
	*pvalue |= (*(pb + offset + 1)) << 8;

	pCtrl->ReadOffset += 2;
}
static void ExifR_ReadL32Bits(PEXIF_CTRL pCtrl, UINT32 *pvalue)//little
{
	UINT32 offset;
	UINT8 *pb;
	offset = pCtrl->ReadOffset;
	pb = (UINT8 *)pCtrl->ReadBufBase;
	*pvalue = *(pb + offset);
	*pvalue |= (*(pb + offset + 1)) << 8;
	*pvalue |= (*(pb + offset + 2)) << 16;
	*pvalue |= (*(pb + offset + 3)) << 24;

	pCtrl->ReadOffset += 4;
}
static void ExifR_Get16Bits(PEXIF_CTRL pCtrl, UINT16 *pvalue)
{
	if (pCtrl->ByteOrder == BYTE_ORDER_LITTLE_ENDIAN) {
		ExifR_ReadL16Bits(pCtrl, pvalue);
	} else {
		ExifR_ReadB16Bits(pCtrl, pvalue);
	}
}
static void ExifR_Get32Bits(PEXIF_CTRL pCtrl, UINT32 *pvalue)
{
	if (pCtrl->ByteOrder == BYTE_ORDER_LITTLE_ENDIAN) {
		ExifR_ReadL32Bits(pCtrl, pvalue);
	} else {
		ExifR_ReadB32Bits(pCtrl, pvalue);
	}
}

static UINT32 ExifR_GetExifTag(PEXIF_CTRL pCtrl, UINT32 index, UINT32 tagPos, UINT32 offsetBase)
{
	EXIF_PARSETAG *ptr;
	UINT32 offset, dataAdd = 0;
	//#NT#2011/11/07#Meg Lin -begin
	//fixbug: word-align wrong
	Exif_Offset old_offset;

	if (index >= MAX_EXIFREADTAG) {
		DBG_ERR("Exif Tag number over flow!\r\n");
		return EXIF_ERRORTYPE;
	}


	ptr = &pCtrl->ReadTag[index];

	ptr->tagPos = tagPos;
	ExifR_Get16Bits(pCtrl, &(ptr->tagId));
	ExifR_Get16Bits(pCtrl, &(ptr->tagType));
	//#NT#2011/01/13#Meg Lin -begin
	//fixbug: parsing exif-error files hangs up
	if (ptr->tagType < TypeBYTE || ptr->tagType > TypeSRATIONAL) {
		DBG_ERR(" Type Error!  IFD=%d, ID=0x%X, Type=%d\r\n", tagPos, ptr->tagId, ptr->tagType);
		return EXIF_ERRORTYPE;
	}
	//#NT#2011/01/13#Meg Lin -end
	ExifR_Get32Bits(pCtrl, &(ptr->tagLen));
	ExifR_Get32Bits(pCtrl, &offset);

	if ((ptr->tagType == TypeRATIONAL) || (ptr->tagType == TypeSRATIONAL)) {
		ptr->tagLen = 8;
		ptr->tagDataOffset = offset + offsetBase;
		dataAdd = 8;
	} else if ((offset >= 4) && (ptr->tagLen > 4)) {
		ptr->tagDataOffset = offset + offsetBase;
		dataAdd = ptr->tagLen;
	} else {
		ptr->tagLen = 4;
		ptr->tagDataOffset = offset;
	}
	ptr->tagOffset = pCtrl->ReadOffset - 4;
	DBG_MSG("[0x%04x][%02d][Len=%04dB][Value(offset)=0x%04x][tagDataOffset=%05d][tagOffset=0x%04x]\r\n", ptr->tagId, ptr->tagType, ptr->tagLen, offset, ptr->tagDataOffset, ptr->tagOffset);
	old_offset = pCtrl->ReadOffset;
	ExifR_ReadSeekOffset(pCtrl, ptr->tagDataOffset, EXIFR_SEEK_SET);
	//special case
	switch (ptr->tagId) {
	case TagExifIFDPtr:
		ptr->tagDataOffset += offsetBase;
		pCtrl->FileInfo.exifIFDOffset = ptr->tagDataOffset;
		break;
	case TagGPSIFDPtr:
		if (offset != 0) {
			ptr->tagDataOffset += offsetBase;
			pCtrl->FileInfo.gpsIFDOffset = ptr->tagDataOffset;
		}
		break;
	case TAG_ID_MAKER_NOTE:
		DBG_MSG("makerNote Pos = 0x%x!\r\n", ptr->tagDataOffset);
		pCtrl->MakernoteInfo.uiMakerNoteAddr = ptr->tagDataOffset + pCtrl->ReadBufBase;
		pCtrl->MakernoteInfo.uiMakerNoteSize = ptr->tagLen;
		pCtrl->MakernoteInfo.uiMakerNoteOffset = ptr->tagDataOffset;

		if (pCtrl->fpExifCB) {
			MEM_RANGE MakerNote;
			UINT32 temp[2];
			ER Ret;
			MakerNote.addr = pCtrl->MakernoteInfo.uiMakerNoteAddr;
			MakerNote.size = pCtrl->MakernoteInfo.uiMakerNoteSize;
			Ret = pCtrl->fpExifCB(PARSE_MAKERNOTE, &MakerNote, 2, temp);
			if (EXIF_ER_OK == Ret) {

				pCtrl->MakernoteInfo.uiScreennailOffset = temp[0];
				pCtrl->MakernoteInfo.uiScreennailSize = temp[1];
			} else {
				//memset((void *)&pCtrl->MakernoteInfo, 0, sizeof(pCtrl->MakernoteInfo));
				pCtrl->MakernoteInfo.uiScreennailOffset = 0;
				pCtrl->MakernoteInfo.uiScreennailSize = 0;
			}
			//DBG_MSG("ScreennailInfo AddrOffset=0x%x, SizeOffset=0x%x, PicOffset=0x%x, PicSize=0x%x\r\n", pCtrl->MakernoteInfo.uiAddrOffset,pCtrl->MakernoteInfo.uiSizeOffset, pCtrl->MakernoteInfo.uiScreennailOffset, pCtrl->MakernoteInfo.uiScreennailSize);
		}
		DBG_MSG("MakerNote Addr=0x%X, Size=0x%X,   ScreennailInfo  PicOffset=0x%X, PicSize=0x%X\r\n", pCtrl->MakernoteInfo.uiMakerNoteAddr, pCtrl->MakernoteInfo.uiMakerNoteSize, pCtrl->MakernoteInfo.uiScreennailOffset, pCtrl->MakernoteInfo.uiScreennailSize);
		break;
	case TagInterOperateIFDPtr:
		ptr->tagDataOffset += offsetBase;
		pCtrl->FileInfo.interOprPos = ptr->tagDataOffset;
		break;
	case TAG_ID_INTERCHANGE_FORMAT:
		ptr->tagDataOffset += offsetBase;
		break;
	}
	ExifR_ReadSeekOffset(pCtrl, old_offset, EXIFR_SEEK_SET);
	return dataAdd;
	//#NT#2011/11/07#Meg Lin -end


}


UINT32 ExifR_ReadExif(EXIF_HANDLE_ID HandleID, UINT32 addr, UINT32 size)
{
	UINT16 temp16_1, temp16_2, i, j;
	UINT32 temp32_1;
	UINT32 zerothDataPos, dataAdd;
	UINT32 Mpo1stTagIndex;
	UINT32 WorkBuf;
	UINT16 App2Len;
	PEXIF_CTRL pCtrl;

	xExif_lock(HandleID);

	pCtrl = xExif_GetCtrl(HandleID);
	if (size > pCtrl->WorkBufSize) {
		size = pCtrl->WorkBufSize;
	}
	WorkBuf = xExif_GetWorkingBuf(pCtrl, size);
	if (WorkBuf) {
		#if 1
		memcpy((char *)WorkBuf, (char *)addr, size);
		#else
		hwmem_open();
		hwmem_memcpy(WorkBuf, addr, size);
		hwmem_close();
		#endif
		addr = WorkBuf;
	}
	ExifR_MakeInit(pCtrl, addr);
	ExifR_ReadL16Bits(pCtrl, &temp16_1);
	if (JPEG_MARKER_SOI != temp16_1) {
		DBG_ERR("SOI marker not found!\r\n");
		xExif_unlock(HandleID);
		return EXIF_ER_SOI_NF;
	}
	ExifR_ReadL16Bits(pCtrl, &temp16_1);
	if (JPEG_MARKER_APP1 != temp16_1) {
		if (temp16_1 != JPEG_MARKER_APP0) {
			DBG_WRN("Appx marker not found! No APP0 or APP1\r\n");
			xExif_unlock(HandleID);
			return EXIF_ER_APP1_NF;
		}
		//read app0 len
		ExifR_ReadB16Bits(pCtrl, &temp16_2);
		pCtrl->TiffOffsetBase = APP1_TIFF_OFFSET_BASE + temp16_2 + 2;//plus marker 2 bytes
		pCtrl->App1SizeOffset = APP1_SIZE_OFFSET +  + temp16_2 + 2;//plus marker 2 bytes;
		ExifR_ReadSkip(pCtrl, (Exif_Offset)temp16_2 - 2); //minus len (2B)
		ExifR_ReadL16Bits(pCtrl, &temp16_1);
		if (temp16_1 != JPEG_MARKER_APP1) {
			DBG_WRN("no APP1\r\n");
			xExif_unlock(HandleID);
			return EXIF_ER_APP1_NF;
		}
	}
	//pCtrl->FileInfo.app1LenPos = pCtrl->ReadOffset;
	ExifR_ReadB16Bits(pCtrl, &temp16_2);
	pCtrl->FileInfo.app1Len = temp16_2;

	//skip exif
	ExifR_ReadSkip(pCtrl, 6);
	//start TIFF header
	zerothDataPos = pCtrl->ReadOffset;
	ExifR_ReadB16Bits(pCtrl, &temp16_1);
	pCtrl->ByteOrder = temp16_1;
	ExifR_ReadSkip(pCtrl, 6);//TIFF header
	//start 0th IFD
	ExifR_Get16Bits(pCtrl, &temp16_1);
	pCtrl->FileInfo.zeroThIFDnum = 0;
	//zerothDataPos = pCtrl->ReadOffset + temp16_1 *0xc+4;//TIFF(8)+tagnum(2)+tag*N(c*N)+4
	for (i = 0; i < temp16_1; i++) {
		dataAdd = ExifR_GetExifTag(pCtrl, pCtrl->ReadTagIndex, EXIF_0thIFD, zerothDataPos);
		if (dataAdd == EXIF_ERRORTYPE) {
			xExif_unlock(HandleID);
			return EXIF_ER_TAG_NUM_OV;
		}
		pCtrl->ReadTagIndex += 1;
		pCtrl->FileInfo.zeroThIFDnum ++;
		pCtrl->FileInfo.totalTagNum ++;
	}
	ExifR_Get32Bits(pCtrl, &temp32_1);
	pCtrl->FileInfo.firstIFDPos = temp32_1 + zerothDataPos;
	//DBG_MSG("1stID pos =0x%lx+ 0x%lx=0x%lx!\r\n", temp32_1, zerothDataPos, pCtrl->FileInfo.firstIFDPos);
	//jump to ExifIFD
	if (pCtrl->FileInfo.exifIFDOffset) {
		ExifR_ReadSeekOffset(pCtrl, pCtrl->FileInfo.exifIFDOffset, EXIFR_SEEK_SET);
	} else {
		DBG_ERR("Exif IFD error, data/length not match\r\n");
		xExif_unlock(HandleID);
		return EXIF_ER_NO_EXIF_IFD;
	}
	DBG_MSG("exifIFD pos =0x%lx!\r\n", pCtrl->FileInfo.exifIFDOffset);

	//start Exif IFD
	ExifR_Get16Bits(pCtrl, &temp16_1);
	dataAdd = 0;

	pCtrl->FileInfo.exifIFDnum = 0;
	for (i = 0; i < temp16_1; i++) {
		dataAdd = ExifR_GetExifTag(pCtrl, pCtrl->ReadTagIndex, EXIF_ExifIFD, zerothDataPos);
		if (dataAdd == EXIF_ERRORTYPE) {
			xExif_unlock(HandleID);
			return EXIF_ER_TAG_NUM_OV;
		}
		pCtrl->ReadTagIndex += 1;
		pCtrl->FileInfo.exifIFDnum++;
		pCtrl->FileInfo.totalTagNum++;
	}
	ExifR_ReadSkip(pCtrl, 4);//exif end

	//jump to GPSIFD
	if (pCtrl->FileInfo.gpsIFDOffset) {
		ExifR_ReadSeekOffset(pCtrl, pCtrl->FileInfo.gpsIFDOffset, EXIFR_SEEK_SET);
		DBG_MSG("GPSIfd pos =0x%lx!\r\n", pCtrl->FileInfo.gpsIFDOffset);

		//start Exif IFD
		ExifR_Get16Bits(pCtrl, &temp16_1);
		if (temp16_1 > 0x20) {
			DBG_ERR("some error!! GPS IFD num = %d !\r\n", temp16_1);
			temp16_1 = 0x20;
		}
		dataAdd = 0;
		pCtrl->FileInfo.gpsIFDnum = 0;
		for (i = 0; i < temp16_1; i++) {
			//dataAdd += ExifR_GetGPSTag(pb, gGPSReadTagIndex, EXIF_GPSIFD, zerothDataPos);
			//gGPSReadTagIndex+=1;
			dataAdd = ExifR_GetExifTag(pCtrl, pCtrl->ReadTagIndex, EXIF_GPSIFD, zerothDataPos);
			if (dataAdd == EXIF_ERRORTYPE) {
				xExif_unlock(HandleID);
				return EXIF_ER_TAG_NUM_OV;
			}
			pCtrl->ReadTagIndex += 1;
			pCtrl->FileInfo.gpsIFDnum++;
			pCtrl->FileInfo.totalTagNum++;
		}
		ExifR_ReadSkip(pCtrl, 4);//exif end
	}

	//parse inter IFD
	DBG_MSG("interOprPos  =0x%lx!\r\n", pCtrl->FileInfo.interOprPos);
	if (pCtrl->FileInfo.interOprPos) {

	}
	if (pCtrl->FileInfo.firstIFDPos) {
		ExifR_ReadSeekOffset(pCtrl, pCtrl->FileInfo.firstIFDPos, EXIFR_SEEK_SET);
	}

	//parse 1st IFD
	ExifR_Get16Bits(pCtrl, &temp16_1);

	dataAdd = 0;
	pCtrl->FileInfo.firstIFDnum = 0;
	DBG_MSG("1stIFDpos pos =0x%lx!num=%d!!\r\n", pCtrl->FileInfo.firstIFDPos, temp16_1);
	for (i = 0; i < temp16_1; i++) {
		dataAdd = ExifR_GetExifTag(pCtrl, pCtrl->ReadTagIndex, EXIF_1stIFD, zerothDataPos);
		if (dataAdd == EXIF_ERRORTYPE) {
			xExif_unlock(HandleID);
			return EXIF_ER_TAG_NUM_OV;
		}
		pCtrl->ReadTagIndex += 1;
		pCtrl->FileInfo.firstIFDnum++;
		pCtrl->FileInfo.totalTagNum++;
	}
	ExifR_ReadSkip(pCtrl, 4);//exif end


	//parse MPO begin
	//check APP2 marker
	ExifR_ReadSeekOffset(pCtrl, pCtrl->FileInfo.app1Len + 4, EXIFR_SEEK_SET);

	pCtrl->HeaderSize = pCtrl->ReadOffset;
	ExifR_ReadL16Bits(pCtrl, &temp16_1);
	if (JPEG_MARKER_APP2 != temp16_1) {
		DBG_MSG("NO APP2 Marker!(curr=0x%x)\r\n", temp16_1);
		xExif_unlock(HandleID);
		return 0;
	}
	//APP2 length
	ExifR_ReadB16Bits(pCtrl, &App2Len);
	//ExifR_ReadSkip(pCtrl, 2);
	pCtrl->HeaderSize += (App2Len + 2);
	//check MPO identifier
	ExifR_ReadL32Bits(pCtrl, &temp32_1);
	if (MPO_IDENTIFIER != temp32_1) {
		DBG_MSG("NO MPO Identifier(0x0046504D)!(curr=0x%x)\r\n", temp32_1);
		xExif_unlock(HandleID);
		return 0;
	}
	//MPO offset base
	pCtrl->MpoOffsetBase = pCtrl->ReadOffset;
	DBG_MSG("pCtrl->MpoOffsetBase =0x%lx!\r\n", pCtrl->MpoOffsetBase);

	//MP Endian
	ExifR_ReadL32Bits(pCtrl, &temp32_1);
	temp32_1 &= 0xFFFF;
	if (pCtrl->ByteOrder != temp32_1) {
		DBG_ERR("MPO byte order should be the same as APP1\r\n");
		xExif_unlock(HandleID);
		return 0;
	}

	Mpo1stTagIndex = pCtrl->ReadTagIndex;
	//offset of MPO 1st IFD
	ExifR_Get32Bits(pCtrl, &temp32_1);
	ExifR_ReadSeekOffset(pCtrl, temp32_1 + pCtrl->MpoOffsetBase, EXIFR_SEEK_SET);
	for (j = 0; j < MPO_MAX_IFD_CNT; j++) {
		//start XXX IFD
		ExifR_Get16Bits(pCtrl, &temp16_1);
		pCtrl->FileInfo.totalTagNum += temp16_1;
		for (i = 0; i < temp16_1; i++) {
			dataAdd = ExifR_GetExifTag(pCtrl, pCtrl->ReadTagIndex, EXIF_IFD_MPO_ATTR, pCtrl->MpoOffsetBase);
			if (dataAdd == EXIF_ERRORTYPE) {
				xExif_unlock(HandleID);
				return EXIF_ER_TAG_NUM_OV;
			}
			pCtrl->ReadTagIndex += 1;

		}
		//next IFD offset
		ExifR_Get32Bits(pCtrl, &temp32_1);
		if (j == 0 && temp32_1 != 0) {
			//this IFD should be EXIF_IFD_MPO_INDEX, revising the IFD in pCtrl->ReadTag
			UINT32 k;
			for (k = Mpo1stTagIndex; k < pCtrl->ReadTagIndex; k++) {
				pCtrl->ReadTag[k].tagPos = EXIF_IFD_MPO_INDEX;
			}
			ExifR_ReadSeekOffset(pCtrl, temp32_1 + pCtrl->MpoOffsetBase, EXIFR_SEEK_SET);
		} else {
			break;
		}
	}

	xExif_unlock(HandleID);
	return 0;
}


/**
    After reading exif header, get information of a specific exif tag.

    After reading exif header, get information of a specific exif tag.
    ExifR_ReadExif( ) should be called firstly.

    @param[in] pTag     pointer of exif header information

     @return void
*/

void ExifR_GetTagData(PEXIF_CTRL pCtrl, EXIF_GETTAG *pTag)
{
	UINT32 i = 0;
	for (i = 0; i < pCtrl->FileInfo.totalTagNum; i++) {
		if (pTag->uiTagId == pCtrl->ReadTag[i].tagId && pTag->uiTagIfd == pCtrl->ReadTag[i].tagPos) {
			pTag->uiTagLen = pCtrl->ReadTag[i].tagLen;
			if (pCtrl->ReadTag[i].tagType == TypeRATIONAL || pCtrl->ReadTag[i].tagType == TypeSRATIONAL || pCtrl->ReadTag[i].tagLen > 4) {
				pTag->uiTagDataAddr = pCtrl->ReadTag[i].tagDataOffset;
			} else {
				pTag->uiTagDataAddr = pCtrl->ReadTag[i].tagOffset;
			}
			pTag->uiTagDataAddr += pCtrl->ReadBufBase;
			pTag->uiTagOffset = pCtrl->ReadTag[i].tagOffset;
			DBG_MSG("[0x%04x][%02d][Len=%04dB][uiTagDataAddrOffset=0x%x][tagOffset=0x%x]\r\n",
					pTag->uiTagId, pCtrl->ReadTag[i].tagType, pTag->uiTagLen, pTag->uiTagDataAddr - pCtrl->ReadBufBase, pTag->uiTagOffset);
			break;
		}
	}
	//no this tag
	if (i == pCtrl->FileInfo.totalTagNum) {
		pTag->uiTagLen = 0;
		DBG_WRN("Can't find this Tag (IFD=%d, ID=0x%04x)!\r\n", pTag->uiTagIfd, pTag->uiTagId);
	}

}
//=================================
EXIF_ER EXIF_ParseExif(EXIF_HANDLE_ID HandleID, PMEM_RANGE pExifData)
{
	EXIF_ER  Ret;
	PEXIF_CTRL pCtrl;

	if (FALSE == xExif_CheckHandleID(HandleID)) {
		return EXIF_ER_SYS;
	}

	pCtrl = xExif_GetCtrl(HandleID);

	if (FALSE == xExif_CheckInited(pCtrl)) {
		return EXIF_ER_SYS;
	}

	pCtrl->ExifDataAddr = pExifData->addr;

	Ret = ExifR_ReadExif(HandleID, pExifData->addr, pExifData->size);
	xExif_ResetWorkingBuf(pCtrl);
	return Ret;
}

EXIF_ER EXIF_GetTag(EXIF_HANDLE_ID HandleID, PEXIF_GETTAG pTag)
{
	PEXIF_CTRL pCtrl;

	if (FALSE == xExif_CheckHandleID(HandleID)) {
		return EXIF_ER_SYS;
	}

	pCtrl = xExif_GetCtrl(HandleID);

	if (FALSE == xExif_CheckInited(pCtrl)) {
		return EXIF_ER_SYS;
	}

	//xExif_lock(HandleID);
	ExifR_GetTagData(pCtrl, pTag);
	//xExif_unlock(HandleID);
	if (pTag->uiTagLen != 0) {
		return EXIF_ER_OK;
	} else {
		return EXIF_ER_SYS;
	}
}
EXIF_ER EXIF_GetInfo(EXIF_HANDLE_ID HandleID, EXIF_INFO Info, VOID *pData, UINT32  uiNumByte)
{
	EXIF_ER Ret = EXIF_ER_SYS;
	PEXIF_CTRL pCtrl;

	if (FALSE == xExif_CheckHandleID(HandleID)) {
		return EXIF_ER_SYS;
	}

	pCtrl = xExif_GetCtrl(HandleID);

	if (FALSE == xExif_CheckInited(pCtrl)) {
		return EXIF_ER_SYS;
	}

	//xExif_lock(HandleID);
	switch (Info) {
	case EXIFINFO_BYTEORDER:
		if (uiNumByte == sizeof(BOOL)) {
			if (pCtrl->ByteOrder == BYTE_ORDER_LITTLE_ENDIAN) {
				*(BOOL *)pData = TRUE;
			} else {
				*(BOOL *)pData = FALSE;
			}
			Ret = EXIF_ER_OK;
		}
		break;
	case EXIFINFO_MPO_OFFSET_BASE:
		if (uiNumByte == sizeof(UINT32)) {
			*(UINT32 *)pData = (UINT32)pCtrl->MpoOffsetBase;
			Ret = EXIF_ER_OK;
		}
		break;
	case EXIFINFO_MAKERNOTE:
		if (uiNumByte == sizeof(MAKERNOTE_INFO)) {
			memcpy((VOID *)pData, (VOID *)&pCtrl->MakernoteInfo, sizeof(pCtrl->MakernoteInfo));
			Ret = EXIF_ER_OK;
		}
		break;
	case EXIFINFO_EXIF_HEADER_SIZE:
		if (uiNumByte == sizeof(UINT32)) {
			*(UINT32 *)pData = pCtrl->HeaderSize;
			Ret = EXIF_ER_OK;
		}
		break;
	case EXIFINFO_TIFF_OFFSET_BASE:
		if (uiNumByte == sizeof(UINT32)) {
			*(UINT32 *)pData = pCtrl->TiffOffsetBase;
			Ret = EXIF_ER_OK;
		}
		break;
	case EXIFINFO_APP1_SIZE_OFFSET:
		if (uiNumByte == sizeof(UINT32)) {
			*(UINT32 *)pData = pCtrl->App1SizeOffset;
			Ret = EXIF_ER_OK;
		}
		break;
	default:
		break;

	}
	if (Ret != EXIF_ER_OK) {
		DBG_ERR("Parameter error! Info=%d, NumByte = %d\r\n", Info, uiNumByte);
	}
	//xExif_unlock(HandleID);
	return Ret;
}
void EXIF_DumpData(EXIF_HANDLE_ID HandleID)
{
	EXIF_GETTAG Tag;
	UINT32 i;
	PEXIF_CTRL pCtrl;

	if (FALSE == xExif_CheckHandleID(HandleID)) {
		return;
	}

	pCtrl = xExif_GetCtrl(HandleID);

	if (FALSE == xExif_CheckInited(pCtrl)) {
		return;
	}

	DBG_DUMP("Total TagNum = %d\r\n", pCtrl->FileInfo.totalTagNum);
	xExif_lock(HandleID);

	for (i = 0; i < pCtrl->FileInfo.totalTagNum; i++) {
		Tag.uiTagIfd = pCtrl->ReadTag[i].tagPos;
		Tag.uiTagLen = pCtrl->ReadTag[i].tagLen;
		Tag.uiTagId = pCtrl->ReadTag[i].tagId;
		if (pCtrl->ReadTag[i].tagType == TypeRATIONAL || pCtrl->ReadTag[i].tagType == TypeSRATIONAL || pCtrl->ReadTag[i].tagLen > 4) {
			Tag.uiTagDataAddr = pCtrl->ReadTag[i].tagDataOffset;
		} else {
			Tag.uiTagDataAddr = pCtrl->ReadTag[i].tagOffset;
		}
		Tag.uiTagDataAddr += pCtrl->ReadBufBase;
		Tag.uiTagOffset = pCtrl->ReadTag[i].tagOffset;
		DBG_DUMP("[%d][%04x][%02d][%04dByte][DataOffset=0x%08x,value=0x%08x][tagOffset=0x%x]\r\n",
				 Tag.uiTagIfd, Tag.uiTagId, pCtrl->ReadTag[i].tagType, Tag.uiTagLen, Tag.uiTagDataAddr - pCtrl->ReadBufBase, Get32BitsData(Tag.uiTagDataAddr, TRUE), Tag.uiTagOffset);
	}
	xExif_unlock(HandleID);
}
//=================================
//@}

