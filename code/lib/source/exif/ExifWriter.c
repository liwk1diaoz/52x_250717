/*
    @file       ExifWriter.c
    @ingroup    mILIBEXIFWrite

    @brief      exif header writer
    @version    V1.01.007
    @date       2011/08/30

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#include <string.h>
#include "ExifTemp.h"
#include "ExifRW.h"
#include "ExifWriter.h"
#include "exif/ExifDef.h"
#include "exif/Exif.h"
//#include "fs_file_op.h"
#define EXIFW_SEEK_SET 0 /* start of stream  */
#define EXIFW_SEEK_CUR 1 /* current position in stream  */
#define EXIFW_SEEK_END 2 /* end of stream  */



#define HW_COPY_THRESHOLD   32
#define PAD_DATA_ALIGNMENT   2

typedef enum {
	OVERWRITE_IF_EXIST = 0,
	IGNORE_IF_EXIST,
	ENUM_DUMMY4WORD(INSERT_TYPE)
} INSERT_TYPE;


static void xExif_SeekOffset(PEXIF_CTRL pCtrl, Exif_Offset pos, UINT16 type)
{
	if (type == EXIFW_SEEK_SET) {
		pCtrl->WriteOffset = pos;
	}
}

static void xExif_WriteBuffer(PEXIF_CTRL pCtrl, UINT8 *pSrc, UINT32 len)
{
	UINT8 *pb;
	UINT32 offset;
	UINT8 pad = 0, mod = 0;
	//ExifTagVerifier will check if the offset is even or not.
	pb = (UINT8 *)pCtrl->WriteBufBase;
	offset = pCtrl->WriteOffset;
	mod = len & 1;
	if (mod) { //pad to 2
		pad = 2 - mod;
	}
	#if 1
	memcpy((void *)(pb + offset), (const void *) pSrc, len);
	#else
	if (len >= HW_COPY_THRESHOLD) {
		hwmem_open();
		hwmem_memcpy((UINT32)(pb + offset), (UINT32) pSrc, len);
		hwmem_close();
	} else {
		memcpy((void *)(pb + offset), (const void *) pSrc, len);
	}
	#endif
	memset((void *)(pb + offset + len), 0, pad);
	pCtrl->WriteOffset += (len + pad);
}
static void xExif_WriteB8Bits(PEXIF_CTRL pCtrl, UINT8 value)//big endian
{
	UINT8 *pb;
	pb = (UINT8 *)pCtrl->WriteBufBase;
	*(pb + pCtrl->WriteOffset) = value;
	pCtrl->WriteOffset += 1;
}

static void xExif_WriteB16Bits(PEXIF_CTRL pCtrl, UINT16 value)//big endian
{
	UINT8 *pb;
	UINT32 offset;
	offset = pCtrl->WriteOffset;
	pb = (UINT8 *)pCtrl->WriteBufBase;
	*(pb + offset) = (value >> 8);
	*(pb + offset + 1) = (value >> 0);
	pCtrl->WriteOffset += 2;
}
static void xExif_WriteL16Bits(PEXIF_CTRL pCtrl, UINT16 value)//little endian
{
	UINT8 *pb;
	UINT32 offset;
	offset = pCtrl->WriteOffset;
	pb = (UINT8 *)pCtrl->WriteBufBase;
	*(pb + offset) = (value & 0xff);
	*(pb + offset + 1) = (value >> 8) & 0xFF;
	pCtrl->WriteOffset += 2;
}
#if 0
static void xExif_WriteB32Bits(PEXIF_CTRL pCtrl, UINT32 value)//big endian
{
	UINT8 *pb;
	UINT32 offset;
	offset = pCtrl->WriteOffset;
	pb = (UINT8 *)pCtrl->WriteBufBase;
	*(pb + offset) = (value >> 24);
	*(pb + offset + 1) = (value >> 16);
	*(pb + offset + 2) = (value >> 8);
	*(pb + offset + 3) = (value);
	pCtrl->WriteOffset += 4;
}
#endif
static void xExif_WriteL32Bits(PEXIF_CTRL pCtrl, UINT32 value)//little endian
{
	UINT8 *pb;
	UINT32 offset;
	offset = pCtrl->WriteOffset;
	pb = (UINT8 *)pCtrl->WriteBufBase;
	*(pb + offset) = (value & 0xff);
	*(pb + offset + 1) = (value >> 8) & 0xFF;
	*(pb + offset + 2) = (value >> 16) & 0xFF;
	*(pb + offset + 3) = (value >> 24) & 0xFF;
	pCtrl->WriteOffset += 4;
}
#if 0
static void xExif_WriteB64Bits(PEXIF_CTRL pCtrl, UINT64 value)//big endian
{
	UINT8 *pb;
	UINT32 offset;
	offset = pCtrl->WriteOffset;
	pb = (UINT8 *)pCtrl->WriteBufBase;

	*(pb + offset) = (value >> 56);
	*(pb + offset + 1) = (value >> 48);
	*(pb + offset + 2) = (value >> 40);
	*(pb + offset + 3) = (value >> 32);
	*(pb + offset + 4) = (value >> 24);
	*(pb + offset + 5) = (value >> 16);
	*(pb + offset + 6) = (value >> 8);
	*(pb + offset + 7) = (value);
	pCtrl->WriteOffset += 8;
}
//#NT#2011/06/30#Meg Lin -begin
static UINT32 xExif_GetTypeByte(UINT32 type)
{
	UINT32 returnV = 0;
	switch (type) {

	case TypeBYTE:
	case TypeASCII:
	case TypeUNDEFINED:
	default:
		returnV = 1;
		break;
	case TypeSHORT:
		returnV = 2;
		break;
	case TypeLONG:
	case TypeSLONG:
		returnV = 4;
		break;
	case TypeRATIONAL:
	case TypeSRATIONAL:
		returnV = 8;
		break;

	}
	return returnV;
}
//#NT#2011/06/30#Meg Lin -end
#endif

UINT32 gXResolVal[2] = {72, 1};
UINT32 gYResolVal[2] = {72, 1};

#define TAG_ID_EXIF_IFD_PTR                 0x8769
#define TAG_ID_GPS_IFD_PTR                  0x8825
//#define TAG_ID_MAKER_NOTE_OFFSET            0x927D
#define TAG_ID_0TH_INT_IFD_PTR              0xA005
#define TAG_ID_COMPRESSION                  0x0103


UINT16 gEmbededTag[] = {
	TAG_ID_EXIF_IFD_PTR,
	TAG_ID_GPS_IFD_PTR,
	TAG_ID_0TH_INT_IFD_PTR,
	TAG_ID_COMPRESSION,
	TAG_ID_INTERCHANGE_FORMAT,
	TAG_ID_INTERCHANGE_FORMAT_LENGTH
};
//only remained mandatory tag
EXIF_INSERTTAG  gExif0thIFDTag[DEFAULT_0THIFD_NUM] = {
	{EXIF_0thIFD, TagXResolution,        TypeRATIONAL,   TagXResolLen, (UINT32 *) &gXResolVal[0],       0},
	{EXIF_0thIFD, TagYResolution,        TypeRATIONAL,   TagYResolLen, (UINT32 *) &gYResolVal[0],       0},
	{EXIF_0thIFD, TagResolutionUnit,     TypeSHORT,      TagResolUnitLen, (UINT32 *)2,                            0},
	{EXIF_0thIFD, TagYCbCrPositioning,   TypeSHORT,      TagYCbCrPosLen, (UINT32 *)2,                            0},
	{EXIF_0thIFD, TagExifIFDPtr,         TypeLONG,       TagExifPtrLen, (UINT32 *)0,                       0},
};

EXIF_INSERTTAG  gExifIFDTag[DEFAULT_EXIFIFD_NUM] = {
	{EXIF_ExifIFD, TagExifVersion,        TypeUNDEFINED,  TagExifVerLen, (UINT32 *)0x30323230,                   0},
	{EXIF_ExifIFD, TagComponentsCfg,      TypeUNDEFINED,  TagCompCfgLen, (UINT32 *)0x00030201,                   0},
	{EXIF_ExifIFD, TagFlashPixVersion,    TypeUNDEFINED,  TagFlashVerLen, (UINT32 *)0x30303130,                   0},
	{EXIF_ExifIFD, TagColorspace,         TypeSHORT,      TagColorSpcLen, (UINT32 *)ColorSpaceSRGB,               0},
	{EXIF_ExifIFD, TagPixelXDimension,    TypeSHORT,      TagPXDimenLen, (UINT32 *)160,                          0}, //20
	{EXIF_ExifIFD, TagPixelYDimension,    TypeSHORT,      TagPYDimenLen, (UINT32 *)120,                          0},
};


EXIF_INSERTTAG  g0thIntOpeIFDTag[DEFAULT_0THINTOPE_NUM] = {
	{EXIF_0thIntIFD, TagInterOpIndex,     TypeASCII,      TagInterOpIndexLen, (UINT32 *)0x00383952,     0}, //R38
	{EXIF_0thIntIFD, TagExifR98Ver,       TypeUNDEFINED,  TagExifR98VerLen, (UINT32 *)0x30303130,     0}
};//0100

EXIF_INSERTTAG  gExif1thIFDTag[DEFAULT_1STIFD_NUM] = {
	{EXIF_1stIFD, TagCompression,        TypeSHORT,      TagCompressionLen, (UINT32 *)JPEGCompression,           0},
	{EXIF_1stIFD, TagXResolution,        TypeRATIONAL,   TagXResolLen, (UINT32 *) &gXResolVal[0],    0},
	{EXIF_1stIFD, TagYResolution,        TypeRATIONAL,   TagYResolLen, (UINT32 *) &gYResolVal[0],    0},
	{EXIF_1stIFD, TagResolutionUnit,     TypeSHORT,      TagResolUnitLen, (UINT32 *)2,                         0},
	{EXIF_1stIFD, TAG_ID_INTERCHANGE_FORMAT,       TypeLONG, TagJPEGInterLen, (UINT32 *)0,       0},
	{EXIF_1stIFD, TAG_ID_INTERCHANGE_FORMAT_LENGTH, TypeLONG, TagJPEGLengthLen, (UINT32 *)5508,    0}
};



/**
    @addtogroup mILIBEXIFWrite
*/
//@{


/**
    Clear all exif tags.

    Clear all exif tags.

    @return void
*/
static void xExif_ClearInsertTag(PEXIF_CTRL pCtrl)
{
	UINT32 i;
	for (i = 0; i < EXIF_INSERT_MAX; i++) {
		pCtrl->InsertTag[i].tagLen = 0;

	}
	pCtrl->Add0thTagNum = 0;
	pCtrl->AddExifTagNum = 0;
	pCtrl->Add1stTagNum = 0;
	pCtrl->AddTagTotal = 0;
	pCtrl->AddGpsTagNum = 0;
	pCtrl->Add0thIntTagNum = 0;
	for (i = 0; i < EXIF_PADDATA_MAX; i++) {
		pCtrl->IFDPaddingData[i].tagLen = 0;

	}
}

static void xExif_InsertTag(PEXIF_CTRL pCtrl, EXIF_INSERTTAG *pExifTag, INSERT_TYPE InsertType)
{
	UINT32 i;
	BOOL bTagExist = FALSE;
	//#NT#2010/12/22#Meg Lin -end
	//search empty
	for (i = 0; i < EXIF_INSERT_MAX; i++) {
		if (pCtrl->InsertTag[i].tagLen == 0) {
			break;
		}
		//search old
		if (pCtrl->InsertTag[i].tagId == pExifTag->tagId && pCtrl->InsertTag[i].tagPos == pExifTag->tagPos) {
			bTagExist = TRUE;
			if (InsertType == IGNORE_IF_EXIST) {
				return;
			} else {
				break;
			}
		}
	}
	if (i == EXIF_INSERT_MAX) {
		DBG_ERR("Exif Insert tag queue full!! No tag can be inserted!!");
		return;
	}


	memcpy(&pCtrl->InsertTag[i], pExifTag, sizeof(EXIF_INSERTTAG));
	if (bTagExist == FALSE) {
		pCtrl->AddTagTotal += 1;

		if (pCtrl->InsertTag[i].tagPos == EXIF_0thIFD) {
			pCtrl->Add0thTagNum += 1;
		} else if (pCtrl->InsertTag[i].tagPos == EXIF_ExifIFD) {
			pCtrl->AddExifTagNum += 1;
		} else if (pCtrl->InsertTag[i].tagPos == EXIF_1stIFD) {
			pCtrl->Add1stTagNum += 1;
		} else if (pCtrl->InsertTag[i].tagPos == EXIF_GPSIFD) {
			pCtrl->AddGpsTagNum += 1;
		} else if (pCtrl->InsertTag[i].tagPos == EXIF_0thIntIFD) {
			pCtrl->Add0thIntTagNum += 1;
		}

	}
}
//#NT#2011/10/18#Meg Lin -end

static void xExif_InsertMandatoryTag(PEXIF_CTRL pCtrl)
{
	UINT32 i;
	for (i = 0; i < DEFAULT_0THIFD_NUM; i++) {
		xExif_InsertTag(pCtrl, &gExif0thIFDTag[i], IGNORE_IF_EXIST);
	}
	for (i = 0; i < DEFAULT_EXIFIFD_NUM; i++) {
		xExif_InsertTag(pCtrl, &gExifIFDTag[i], IGNORE_IF_EXIST);
	}
	for (i = 0; i < DEFAULT_1STIFD_NUM; i++) {
		xExif_InsertTag(pCtrl, &gExif1thIFDTag[i], IGNORE_IF_EXIST);
	}
}
static void xExif_InsertSpectialTag(PEXIF_CTRL pCtrl, UINT32 uiMaxExifSize)
{
	if (pCtrl->AddGpsTagNum) {
		EXIF_INSERTTAG TagGpsIfdPtr = {0};
		TagGpsIfdPtr.tagPos = EXIF_0thIFD;
		TagGpsIfdPtr.tagId = TagGPSIFDPtr;
		TagGpsIfdPtr.tagType = TypeLONG;
		TagGpsIfdPtr.tagLen = 1;
		xExif_InsertTag(pCtrl, &TagGpsIfdPtr, OVERWRITE_IF_EXIST);

	}
	if (pCtrl->Add0thIntTagNum) {
		EXIF_INSERTTAG Tag0thIntIfdPtr = {0};
		//{EXIF_ExifIFD, TagInterOperateIFDPtr, TypeLONG,       TagOpIFDPtrLen, (UINT32 *)0,                            0},
		Tag0thIntIfdPtr.tagPos = EXIF_ExifIFD;
		Tag0thIntIfdPtr.tagId = TagInterOperateIFDPtr;
		Tag0thIntIfdPtr.tagType = TypeLONG;
		Tag0thIntIfdPtr.tagLen = 1;
		xExif_InsertTag(pCtrl, &Tag0thIntIfdPtr, OVERWRITE_IF_EXIST);

	}
	if (pCtrl->fpExifCB) {
		MEM_RANGE MakerNote = {0};
		UINT32 WorkBuf, MaxMakeNoteSize;//, Screennail[2] = {0};
		//ER ExifCbRet = EXIF_ER_SYS;

		if (uiMaxExifSize < xExif_GetRestWorkingBufSize(pCtrl)) {
			MaxMakeNoteSize = uiMaxExifSize - pCtrl->ThumbSize;
		} else {
			MaxMakeNoteSize = xExif_GetRestWorkingBufSize(pCtrl);
		}

		WorkBuf = xExif_GetWorkingBuf(pCtrl, MaxMakeNoteSize);
		if (WorkBuf) {
			MakerNote.addr = WorkBuf;
			MakerNote.size = MaxMakeNoteSize;
			//ExifCbRet = pCtrl->fpExifCB(CREATE_MAKERNOTE, &MakerNote, 0, NULL);
			pCtrl->fpExifCB(CREATE_MAKERNOTE, &MakerNote, 0, NULL);
#if 0
			DBG_MSG("ExifCB Ret=%d, ScreennailInfo[0] =0x%x, ScreennailInfo[1] =0x%x\r\n", ExifCbRet, Screennail[0], Screennail[1]);
			if (ExifCbRet == EXIF_ER_OK && Screennail[0] != 0 && Screennail[1] != 0) {
				//Temporarily let ScreenailInfo be the "offset" from the start of makernote data
				pCtrl->MakernoteInfo.uiAddrOffset = Screennail[0] - MakerNote.addr;
				pCtrl->MakernoteInfo.uiSizeOffset = Screennail[1] - MakerNote.addr;
			}
#endif
			if (MakerNote.size) {
				EXIF_INSERTTAG MakerNoteTag = {0};
				MakerNoteTag.tagPos = EXIF_ExifIFD;
				MakerNoteTag.tagId = TAG_ID_MAKER_NOTE;
				MakerNoteTag.tagType = TypeUNDEFINED;
				MakerNoteTag.tagLen = MakerNote.size;
				if (MakerNoteTag.tagLen <= 4) {
					pCtrl->TempValue = Get32BitsData(MakerNote.addr, TRUE);
					MakerNoteTag.ptagDataAddr = (UINT32 *)pCtrl->TempValue; // normal
				} else {
					MakerNoteTag.ptagDataAddr = (UINT32 *)MakerNote.addr;
				}
				xExif_InsertTag(pCtrl, &MakerNoteTag, OVERWRITE_IF_EXIST);
				pCtrl->MakernoteInfo.uiMakerNoteAddr = MakerNote.addr;
				pCtrl->MakernoteInfo.uiMakerNoteSize = MakerNote.size;
			}
		}
	}
}
static void xExif_SortInsertedTag(PEXIF_CTRL pCtrl)
{
	INT32 i, j;
	UINT16 TagID1, TagID2;
	EXIF_INSERTTAG  TempTag;

	for (i = 1; i < pCtrl->AddTagTotal; i++) {
		TagID1 = pCtrl->InsertTag[i].tagId;
		for (j = i; j > 0; j--) {
			TagID2 = pCtrl->InsertTag[j - 1].tagId;
			if (TagID1 >= TagID2) {
				break;
			}
			//exchange [j],[j-1]
			memcpy(&TempTag, &pCtrl->InsertTag[j - 1], sizeof(EXIF_INSERTTAG));
			memcpy(&pCtrl->InsertTag[j - 1], &pCtrl->InsertTag[j], sizeof(EXIF_INSERTTAG));
			memcpy(&pCtrl->InsertTag[j], &TempTag, sizeof(EXIF_INSERTTAG));
		}
	}
}

//#NT#2011/06/30#Meg Lin -end


static UINT32 xExif_WriteTag(PEXIF_CTRL pCtrl, EXIF_INSERTTAG *pTag, UINT32 dataPos, UINT32 dataCount)
{
	UINT32 newDataLen = 0;
	UINT8 mod = 0, pad4 = 0;
	if (dataCount >= EXIF_PADDATA_MAX) {
		DBG_ERR("Pading data reaches MAX!!(IFD =%d)\r\n", pTag->tagPos);
		dataCount = EXIF_PADDATA_MAX - 1;
	}
	xExif_WriteL16Bits(pCtrl, pTag->tagId);
	xExif_WriteL16Bits(pCtrl, pTag->tagType);
	//DBG_MSG("tagID = 0x%x !", pTag->tagId);
	if ((pTag->tagType == TypeUNDEFINED) ||
		(pTag->tagType == TypeASCII) ||
		(pTag->tagType == TypeBYTE)) {
		pad4 = pTag->tagLen & (PAD_DATA_ALIGNMENT - 1);
		if (pad4) {
			mod = PAD_DATA_ALIGNMENT - pad4;
		}

		//xExif_WriteL32Bits(pCtrl, pTag->tagLen+mod);
		xExif_WriteL32Bits(pCtrl, pTag->tagLen);
		if (pTag->tagLen <= 4) { //write continuously
			xExif_WriteL32Bits(pCtrl, (UINT32)pTag->ptagDataAddr);
			newDataLen = 0;
		} else {
			pTag->offset = dataPos;
			xExif_WriteL32Bits(pCtrl, pTag->offset);
			pCtrl->IFDPaddingData[dataCount].ptagAddr = pTag->ptagDataAddr;
			pCtrl->IFDPaddingData[dataCount].tagLen = pTag->tagLen;//+mod;
			pCtrl->IFDPaddingData[dataCount].tagtype = pTag->tagType;
			pCtrl->IFDPaddingData[dataCount].tagId = pTag->tagId;
			newDataLen = pTag->tagLen + mod;
		}
	} else if (pTag->tagType == TypeSHORT) {
		mod = pTag->tagLen & 1;
		if (pTag->tagLen <= 2) {
			xExif_WriteL32Bits(pCtrl, pTag->tagLen);
			xExif_WriteL32Bits(pCtrl, (UINT32)pTag->ptagDataAddr);
			newDataLen = 0;
		} else { //pad data
			xExif_WriteL32Bits(pCtrl, pTag->tagLen + mod);
			pTag->offset = dataPos;
			xExif_WriteL32Bits(pCtrl, pTag->offset);
			pCtrl->IFDPaddingData[dataCount].ptagAddr = pTag->ptagDataAddr;
			pCtrl->IFDPaddingData[dataCount].tagLen = (pTag->tagLen + mod) * 2;

			pCtrl->IFDPaddingData[dataCount].tagtype = pTag->tagType;
			pCtrl->IFDPaddingData[dataCount].tagId = pTag->tagId;
			newDataLen = pCtrl->IFDPaddingData[dataCount].tagLen;
		}
	} else if (pTag->tagType == TypeLONG) {
		if (pTag->tagLen <= 1) {
			xExif_WriteL32Bits(pCtrl, pTag->tagLen);
			xExif_WriteL32Bits(pCtrl, (UINT32)pTag->ptagDataAddr);
			newDataLen = 0;
		} else { //pad data
			xExif_WriteL32Bits(pCtrl, pTag->tagLen);
			pTag->offset = dataPos;
			xExif_WriteL32Bits(pCtrl, pTag->offset);
			pCtrl->IFDPaddingData[dataCount].ptagAddr = pTag->ptagDataAddr;
			pCtrl->IFDPaddingData[dataCount].tagLen = pTag->tagLen * 4;

			pCtrl->IFDPaddingData[dataCount].tagtype = pTag->tagType;
			pCtrl->IFDPaddingData[dataCount].tagId = pTag->tagId;
			newDataLen = pCtrl->IFDPaddingData[dataCount].tagLen;
		}
	} else {
		xExif_WriteL32Bits(pCtrl, pTag->tagLen);
		pTag->offset = dataPos;
		xExif_WriteL32Bits(pCtrl, pTag->offset);
		pCtrl->IFDPaddingData[dataCount].ptagAddr = pTag->ptagDataAddr;
		if ((pTag->tagType == TypeRATIONAL) ||
			(pTag->tagType == TypeSRATIONAL)) {
			pCtrl->IFDPaddingData[dataCount].tagLen = pTag->tagLen * 8;
		} else {
			pCtrl->IFDPaddingData[dataCount].tagLen = pTag->tagLen;
		}

		pCtrl->IFDPaddingData[dataCount].tagtype = pTag->tagType;
		pCtrl->IFDPaddingData[dataCount].tagId = pTag->tagId;
		newDataLen = pCtrl->IFDPaddingData[dataCount].tagLen;
	}
	//DBG_MSG("write: newLen = 0x%lx!\r\n", newDataLen);
	DBG_MSG("[%d][%04x][%02d][%04d][newDataLen=%d, tagDataAddr=0x%08x]\r\n",
			pTag->tagPos, pTag->tagId, pTag->tagType, pTag->tagLen, newDataLen, (UINT32)pTag->ptagDataAddr);
	return newDataLen;
}
static void xExif_WriteTagData(PEXIF_CTRL pCtrl, UINT32 dataCount)
{
	UINT32 i;
	if (dataCount) {
		for (i = 0; i < dataCount; i++) {
			DBG_MSG("tagname =0x%x, taglen = 0x%lx, offset=0x%lx!\r\n", pCtrl->IFDPaddingData[i].tagId, pCtrl->IFDPaddingData[i].tagLen, pCtrl->WriteOffset);
#if 1
			if (pCtrl->IFDPaddingData[i].tagId == TAG_ID_MAKER_NOTE) {
				pCtrl->MakernoteInfo.uiMakerNoteOffset = pCtrl->WriteOffset;
			}
#endif
#if 0//little endian don't need this
			if ((pCtrl->IFDPaddingData[i].tagtype == TypeRATIONAL) ||
				(pCtrl->IFDPaddingData[i].tagtype == TypeSRATIONAL)) {
				UINT32 *ptr32;
				ptr32 = (UINT32 *)pCtrl->IFDPaddingData[i].ptagAddr;
				xExif_WriteL32Bits(0, *ptr32++);
				xExif_WriteL32Bits(0, *ptr32++);
			} else
#endif
				xExif_WriteBuffer(pCtrl, (UINT8 *)pCtrl->IFDPaddingData[i].ptagAddr, pCtrl->IFDPaddingData[i].tagLen);
		}
	}
}
#if 0
static void DumpInsertData(void)
{
	UINT32 i;
	DBG_DUMP("^B##########Dump insert data Total TagNum = %d ##########\r\n", pCtrl->AddTagTotal);
	for (i = 0; i < pCtrl->AddTagTotal; i++) {
		DBG_DUMP("[%d][%04x][%02d][Cnt=%03d][DataAddr=0x%08X]\r\n",
				 pCtrl->InsertTag[i].tagPos, pCtrl->InsertTag[i].tagId, pCtrl->InsertTag[i].tagType, pCtrl->InsertTag[i].tagLen, (UINT32)pCtrl->InsertTag[i].ptagDataAddr);
	}
	DBG_DUMP("^B#######################################################\r\n");
}
#endif

static UINT32 xExif_WriteExif(EXIF_HANDLE_ID HandleID, PMEM_RANGE pExifData)
{
	Exif_Offset  app1LenPos, zerothDataPos;
	UINT16 ZerothIFD_Num, i, exifIFD_Num, ifdNum, zeroIfdNum;
	//UINT32 *ptr32;
	UINT32  dataCount = 0;
	UINT32 newAddLen, exifIFDPos = 0, exifIFDAddr, interOperPos = 0, now, firstIFDPos;
	UINT32 totalExifsize;
	UINT32 gpsIFDPos = 0;
	//MEM_RANGE MakerNote = {0};
	//ER ExifCbRet = EXIF_ER_SYS;
	PEXIF_CTRL pCtrl;

	pCtrl = xExif_GetCtrl(HandleID);

	//xExif_MakeInit((UINT32 )pEXIFIMG->ExifAddr);

	pCtrl->WriteBufBase = pExifData->addr;

	pCtrl->WriteOffset = 0;

	memset((VOID *)&pCtrl->MakernoteInfo, 0, sizeof(pCtrl->MakernoteInfo));

	xExif_WriteL16Bits(pCtrl, JPEG_MARKER_SOI);
	xExif_WriteL16Bits(pCtrl, JPEG_MARKER_APP1);
	app1LenPos = pCtrl->WriteOffset;
	xExif_WriteL16Bits(pCtrl, 0);//len reserved

	xExif_WriteBuffer(pCtrl, (UINT8 *)"Exif\0", 6);

	//TIFF header
	xExif_WriteL16Bits(pCtrl, BYTE_ORDER_LITTLE_ENDIAN);
	xExif_WriteL16Bits(pCtrl, TIFF_IDENTIFY);
	xExif_WriteL32Bits(pCtrl, 8);

	xExif_InsertMandatoryTag(pCtrl);
	xExif_InsertSpectialTag(pCtrl, pExifData->size);
	//DumpInsertData();
	xExif_SortInsertedTag(pCtrl);
	//DumpInsertData();
	//0th IFD
	zeroIfdNum = 0;
	ZerothIFD_Num = zeroIfdNum + pCtrl->Add0thTagNum;

	xExif_WriteL16Bits(pCtrl, ZerothIFD_Num);
	DBG_MSG("===== Write 0th IFD, TagNum=%d =====\r\n", ZerothIFD_Num);

	zerothDataPos = 0xa + ZerothIFD_Num * 0xc + 4; //TIFF(8)+tagnum(2)+tag*N(c*N)+4
	for (i = 0; i < pCtrl->AddTagTotal; i++) {
		if (pCtrl->InsertTag[i].tagPos == EXIF_0thIFD) {
			if (pCtrl->InsertTag[i].tagId == TagExifIFDPtr) {
				exifIFDPos = pCtrl->WriteOffset + 8; //tagName(2)+tagtype(2)+len(4)
			} else if (pCtrl->InsertTag[i].tagId == TagGPSIFDPtr) {
				gpsIFDPos = pCtrl->WriteOffset + 8; //tagName(2)+tagtype(2)+len(4)
			}
			newAddLen = xExif_WriteTag(pCtrl, &pCtrl->InsertTag[i], zerothDataPos, dataCount);
			if (newAddLen) {
				zerothDataPos += newAddLen;
				dataCount += 1;
			}
		}
	}
	firstIFDPos = pCtrl->WriteOffset;
	xExif_WriteL32Bits(pCtrl, 0);//1st IFD pos
	//pad 0th IFD data
	xExif_WriteTagData(pCtrl, dataCount);
	dataCount = 0;//reset pCtrl->IFDPaddingData

	//goto exifIFDPos, write Exif tag position
	if (pCtrl->WriteOffset > 0xc) { //no meaning, just add this condition to pass coverity issue.
		now = pCtrl->WriteOffset;
		exifIFDAddr = pCtrl->WriteOffset - 0xc;
		xExif_SeekOffset(pCtrl, exifIFDPos, EXIFW_SEEK_SET);
		xExif_WriteL32Bits(pCtrl, exifIFDAddr);
		//recover
		xExif_SeekOffset(pCtrl, now, EXIFW_SEEK_SET);
	}
	//write Exif IFD
///////////////////////////////////////////
	ifdNum = 0;
	exifIFD_Num = ifdNum + pCtrl->AddExifTagNum;
	zerothDataPos += (2 + exifIFD_Num * 0xc + 4); //tagnum(2)+tag*N(c*N)+4
	xExif_WriteL16Bits(pCtrl, exifIFD_Num);
	DBG_MSG("===== Write EXIF IFD, TagNum=%d =====\r\n", exifIFD_Num);

	for (i = 0; i < pCtrl->AddTagTotal; i++) {
		if (pCtrl->InsertTag[i].tagPos == EXIF_ExifIFD) {
			if (pCtrl->InsertTag[i].tagId == TagInterOperateIFDPtr) {
				interOperPos = pCtrl->WriteOffset + 8; //tagName(2)+tagtype(2)+len(4)
			}
			newAddLen = xExif_WriteTag(pCtrl, &pCtrl->InsertTag[i], zerothDataPos, dataCount);
			//if add data to pCtrl->IFDPaddingData, update
			if (newAddLen) {
				zerothDataPos += newAddLen;
				dataCount += 1;
			}
		}
	}

	xExif_WriteL32Bits(pCtrl, 0);
	//pad data
	xExif_WriteTagData(pCtrl, dataCount);
	dataCount = 0;//reset pCtrl->IFDPaddingData
	if (pCtrl->AddGpsTagNum) {
		//#if _Support_GPSInfo_Tag
		//goto gpsIFDPos, write gpsIFDPos tag position
		now = pCtrl->WriteOffset;
		xExif_SeekOffset(pCtrl, gpsIFDPos, EXIFW_SEEK_SET);
		xExif_WriteL32Bits(pCtrl, now - 0xc);
		//recover
		xExif_SeekOffset(pCtrl, now, EXIFW_SEEK_SET);
		//write gps IFD
		exifIFD_Num = pCtrl->AddGpsTagNum;
		zerothDataPos += (2 + exifIFD_Num * 0xc + 4); //tagnum(2)+tag*N(c*N)+4

		xExif_WriteL16Bits(pCtrl, pCtrl->AddGpsTagNum);
		DBG_MSG("===== Write GPS IFD, TagNum=%d =====\r\n", pCtrl->AddGpsTagNum);
		for (i = 0; i < pCtrl->AddTagTotal; i++) {
			if (pCtrl->InsertTag[i].tagPos == EXIF_GPSIFD) {
				newAddLen = xExif_WriteTag(pCtrl, &pCtrl->InsertTag[i], zerothDataPos, dataCount);
				//if add data to pCtrl->IFDPaddingData, update
				if (newAddLen) {
					zerothDataPos += newAddLen;
					dataCount += 1;
				}
			}
		}
		xExif_WriteL32Bits(pCtrl, 0);
		//pad data
		xExif_WriteTagData(pCtrl, dataCount);
		dataCount = 0;//reset pCtrl->IFDPaddingData


		//#endif
	}//if (gxExif_setGPS)
	if (pCtrl->Add0thIntTagNum) {
		//goto interOperPos, write interOper tag position
		now = pCtrl->WriteOffset;
		xExif_SeekOffset(pCtrl, interOperPos, EXIFW_SEEK_SET);
		xExif_WriteL32Bits(pCtrl, now - 0xc);
		//recover
		xExif_SeekOffset(pCtrl, now, EXIFW_SEEK_SET);


		//write interoperability IFD
		exifIFD_Num = pCtrl->Add0thIntTagNum;
		zerothDataPos += (2 + exifIFD_Num * 0xc + 4); //tagnum(2)+tag*N(c*N)+4

		xExif_WriteL16Bits(pCtrl, pCtrl->Add0thIntTagNum);
		DBG_MSG("===== Write 0thInt IFD, TagNum=%d =====\r\n", pCtrl->Add0thIntTagNum);
		for (i = 0; i < pCtrl->AddTagTotal; i++) {
			if (pCtrl->InsertTag[i].tagPos == EXIF_0thIntIFD) {
				newAddLen = xExif_WriteTag(pCtrl, &pCtrl->InsertTag[i], zerothDataPos, dataCount);
				//if add data to pCtrl->IFDPaddingData, update
				if (newAddLen) {
					DBG_ERR("Pad 0thInterIFD data ERROR!!\r\n");
				}
			}
		}
		xExif_WriteL32Bits(pCtrl, 0);
	}

	now = pCtrl->WriteOffset;
	xExif_SeekOffset(pCtrl, firstIFDPos, EXIFW_SEEK_SET);
	xExif_WriteL32Bits(pCtrl, now - 0xc);
	//recover
	xExif_SeekOffset(pCtrl, now, EXIFW_SEEK_SET);

	dataCount = 0;//reset pCtrl->IFDPaddingData
	//write 1st IFD
	ifdNum = 0;
	exifIFD_Num = ifdNum + pCtrl->Add1stTagNum;
	zerothDataPos += (2 + exifIFD_Num * 0xc + 4); //tagnum(2)+tag*N(c*N)+4

	xExif_WriteL16Bits(pCtrl, exifIFD_Num);
	DBG_MSG("===== Write 1st IFD, TagNum=%d =====\r\n", exifIFD_Num);
	for (i = 0; i < pCtrl->AddTagTotal; i++) {
		if (pCtrl->InsertTag[i].tagPos == EXIF_1stIFD) {
			if (pCtrl->InsertTag[i].tagId == TAG_ID_INTERCHANGE_FORMAT) {
				pCtrl->InsertTag[i].ptagDataAddr = (UINT32 *)zerothDataPos;
			} else if (pCtrl->InsertTag[i].tagId == TAG_ID_INTERCHANGE_FORMAT_LENGTH) {
				pCtrl->InsertTag[i].ptagDataAddr = (UINT32 *)pCtrl->ThumbSize;
			}
			newAddLen = xExif_WriteTag(pCtrl, &pCtrl->InsertTag[i], zerothDataPos, dataCount);
			//if add data to pCtrl->IFDPaddingData, update
			if (newAddLen) {
				zerothDataPos += newAddLen;
				dataCount += 1;
			}
		}
	}
	xExif_WriteL32Bits(pCtrl, 0);
	//pad data
	xExif_WriteTagData(pCtrl, dataCount);
	DBG_MSG("exif thumb pos = 0x%lx! size=0x%lx!\r\n", pCtrl->WriteOffset, pCtrl->ThumbSize);

#if 0
	hwmem_open();
	hwmem_memcpy((UINT32)(pCtrl->WriteBufBase + pCtrl->WriteOffset), (UINT32)(pCtrl->ThumbAddr), (UINT32)(pCtrl->ThumbSize));
	hwmem_close();
#else
	memcpy((void *)(pCtrl->WriteBufBase + pCtrl->WriteOffset), (void *) pCtrl->ThumbAddr,  pCtrl->ThumbSize);
#endif
	pCtrl->WriteOffset += pCtrl->ThumbSize;
	DBG_MSG("exif header size = 0x%lx!\r\n", pCtrl->WriteOffset);
	//#NT#2011/08/30#Meg Lin -begin

	//padding data to let EXIF size be word-alignment
	while (pCtrl->WriteOffset & 0x3) {
		xExif_WriteB8Bits(pCtrl, 0xFF);
	}

	totalExifsize = pCtrl->WriteOffset;
	//#NT#2011/08/30#Meg Lin -end

	xExif_SeekOffset(pCtrl, app1LenPos, EXIFW_SEEK_SET);
	//xExif_WriteB16Bits(pCtrl, totalExifsize-2);//minus FFd8 2bytes
	xExif_WriteB16Bits(pCtrl, totalExifsize - APP1_SIZE_OFFSET);//minus FF D8 FF E1 4bytes

	//clear buffer after complete EXIF data !
	xExif_ClearInsertTag(pCtrl);
	xExif_ResetWorkingBuf(pCtrl);
	return totalExifsize;
}

EXIF_ER EXIF_SetTag(EXIF_HANDLE_ID HandleID, PEXIF_SETTAG pTag)
{
	EXIF_INSERTTAG insertTag1;
	//UINT32 typelen;
	UINT32 WorkBuf;
	PEXIF_CTRL pCtrl;

	if (FALSE == xExif_CheckHandleID(HandleID)) {
		return EXIF_ER_SYS;
	}

	pCtrl = xExif_GetCtrl(HandleID);

	if (FALSE == xExif_CheckInited(pCtrl)) {
		return EXIF_ER_SYS;
	}

	if (0 == pTag->uiTagDataLen) {
		DBG_ERR("Parameter Error! uiTagDataLen = 0\r\n");
		return EXIF_ER_SYS;
	}
	xExif_lock(HandleID);
	if (pTag->uiTagId == TAG_ID_MAKER_NOTE) {
		DBG_ERR("Users should complete your makernote in the callback event CREATE_MAKERNOTE!\r\n");
		xExif_unlock(HandleID);
		return EXIF_ER_SYS;
	} else if (pTag->uiTagIfd >= EXIF_IFD_MPO_INDEX) {
		DBG_ERR("NOT support MPO write!\r\n");
		xExif_unlock(HandleID);
		return EXIF_ER_SYS;
	} else {
		UINT32 i;
		for (i = 0; i < sizeof(gEmbededTag) / sizeof(UINT16); i++) {
			if (gEmbededTag[i] == pTag->uiTagId) {
				DBG_ERR("Users don't have to set this embeded tag!\r\n");
				xExif_unlock(HandleID);
				return EXIF_ER_SYS;
			}
		}
	}
	//typelen = xExif_GetTypeByte(pTag->uiTagType);
	insertTag1.tagPos       = pTag->uiTagIfd;
	insertTag1.tagId        = pTag->uiTagId;
	insertTag1.tagType      = pTag->uiTagType;
	//insertTag1.ptagDataAddr = pTag->uiTagDataAddr;
	insertTag1.tagLen       = pTag->uiTagCount;
	if (pTag->uiTagDataLen <= sizeof(UINT32)) {
		UINT32 value = 0;
		UINT8 *pb = (UINT8 *)pTag->uiTagDataAddr;
		UINT32 i;
		for (i = 0; i < pTag->uiTagDataLen; i++) {
			value |= (*(pb + i)) << (8 * i);
		}
		insertTag1.ptagDataAddr = (UINT32 *)value;
	} else {
		WorkBuf = xExif_GetWorkingBuf(pCtrl, pTag->uiTagDataLen);
		if (WorkBuf) {
			//Don't have do use hwmemcpy, since MakerNote tag won't go here.
			memcpy((char *)WorkBuf, (char *)pTag->uiTagDataAddr, pTag->uiTagDataLen);
			insertTag1.ptagDataAddr = (UINT32 *)WorkBuf;
		} else {
			insertTag1.ptagDataAddr = (UINT32 *)pTag->uiTagDataAddr;
		}
	}
	xExif_InsertTag(pCtrl, &insertTag1, OVERWRITE_IF_EXIST);
	xExif_unlock(HandleID);
	return EXIF_ER_OK;
}

EXIF_ER EXIF_CreateExif(EXIF_HANDLE_ID HandleID, PMEM_RANGE pExifData, PMEM_RANGE pThumbnail)
{
	PEXIF_CTRL pCtrl;

	if (FALSE == xExif_CheckHandleID(HandleID)) {
		return EXIF_ER_SYS;
	}

	pCtrl = xExif_GetCtrl(HandleID);

	if (FALSE == xExif_CheckInited(pCtrl)) {
		return EXIF_ER_SYS;
	}

	if (pThumbnail == NULL || pThumbnail->addr == 0 || pThumbnail->size == 0 || pThumbnail->size > MAX_APP1_SIZE) {
		DBG_ERR("Thumbnail data ERROR!!!!!\r\n");
		return EXIF_ER_SYS;
	}
	if (pExifData->size < pThumbnail->size) {
		DBG_ERR("ExifData size too small!\r\n");
		return EXIF_ER_SYS;
	}
	xExif_lock(HandleID);

	pCtrl->ThumbAddr = pThumbnail->addr;
	pCtrl->ThumbSize = pThumbnail->size;
	DBG_MSG("HandleID[%d]:thumb addr = 0x%lx, size = 0x%lx!\r\n", HandleID, pCtrl->ThumbAddr, pCtrl->ThumbSize);

	pCtrl->ExifDataAddr = pExifData->addr;

	pExifData->size = xExif_WriteExif(HandleID, pExifData);
	if (pExifData->size > MAX_APP1_SIZE) {
		DBG_ERR("EXIF size ERROR!!!!!\r\n");
		xExif_unlock(HandleID);
		return EXIF_ER_SYS;
	}
	xExif_unlock(HandleID);
	return EXIF_ER_OK;
}
EXIF_ER EXIF_UpdateInfo(EXIF_HANDLE_ID HandleID, EXIF_INFO Info, VOID *pData, UINT32  uiNumByte)
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

	xExif_lock(HandleID);
	switch (Info) {
	case EXIFINFO_MAKERNOTE:
		if (uiNumByte == sizeof(MAKERNOTE_INFO)) {
			MEM_RANGE MakerNote;
			UINT32 temp[2];
			memcpy((VOID *)&pCtrl->MakernoteInfo, (VOID *)pData, sizeof(pCtrl->MakernoteInfo));
			MakerNote.addr = pCtrl->MakernoteInfo.uiMakerNoteAddr;
			MakerNote.size = pCtrl->MakernoteInfo.uiMakerNoteSize;
			temp[0] = pCtrl->MakernoteInfo.uiScreennailOffset;
			temp[1] = pCtrl->MakernoteInfo.uiScreennailSize;
			if (pCtrl->fpExifCB) {
				Ret = pCtrl->fpExifCB(UPDATE_MAKERNOTE, &MakerNote, 2, temp);
			}
		}
		break;
	default:
		break;

	}
	if (Ret != EXIF_ER_OK) {
		DBG_ERR("Parameter error! Info=%d, NumByte = %d\r\n", Info, uiNumByte);
	}
	xExif_unlock(HandleID);
	return Ret;
}
//@}

