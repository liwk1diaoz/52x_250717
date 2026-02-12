#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFile.h"
#include "PBXFileList.h"

typedef struct {
	UINT16    Tag;
	UINT16    Type;
	UINT16    Count_LoWord;
	UINT16    Count_HiWord;
	UINT16    Offset_LoWord;
	UINT16    Offset_HiWord;
} FIELD;


static CHAR cFileName[34];

#if 0//temp reserved for verification
static void SetCurSHORT(UINT8 **buf_p, BOOL Swap, UINT16 usData)
{
	if (Swap == 0) {
		**buf_p = (usData >> 8);
		*buf_p += 1;
		**buf_p = usData;
	} else { // if(Swap==0)
		**buf_p = usData;
		*buf_p += 1;
		**buf_p = (usData >> 8);
	}

	// Default move to next byte
	*buf_p += 1;
}
#endif
//#NT#2010/08/04#Ben Wang -begin
//#NT#Refine code for EXIF rotation
BOOL PB_SetExif_IFD0_Orientation(UINT8 **buf_p)
{
#if 0//temp reserved for verification
	UINT32 i, temp, ByteOrder;
	UINT32 TagNums, Offset_0thIFD;
	UINT8 *pTIFF_base, *pTagLoc;

	if (GetCurSHORT(buf_p, 0) != 0xFFD8) {
		return FALSE;       // no SOI marker at start of bitstream
	}

	temp = GetCurSHORT(buf_p, 0);

	if ((temp != 0xFFE1) && (temp != 0xFFE0)) {
		return FALSE;       // no SOI marker at start of bitstream
	} else if (temp == 0xFFE0) {
		temp = GetCurSHORT(buf_p, 0);   // APP0-length
		*buf_p += (temp - 2);
		temp = GetCurSHORT(buf_p, 0);
		if (temp != 0xFFE1) {
			return FALSE;    // no SOI marker at start of bitstream
		}
	}

	EXIFHeaderSize = (UINT32) GetCurSHORT(buf_p, 0);  // APP1-length
	*buf_p += 6;

	// (1).Find TIFF header (base address), get Byte-Order-Mode
	pTIFF_base = *buf_p;
	ByteOrder = GetCurSHORT(buf_p, 0);

	if (ByteOrder == BYTE_ORDER_LITTLE_ENDIAN) {
		ByteOrder = 1;  // Hi,Low byte swap
		ExifByteOrder = 1;
	} else if (ByteOrder == BYTE_ORDER_BIG_ENDIAN) {
		ByteOrder = 0;  // Hi,Low byte No swap
		ExifByteOrder = 0;
	}
	temp = GetCurSHORT(buf_p, ByteOrder);   // TIFFIdentify
	Offset_0thIFD =   GetCurUINT(buf_p, ByteOrder);

	// (2).Find 0th IFD
	*buf_p = pTIFF_base + Offset_0thIFD;
	TagNums = GetCurSHORT(buf_p, ByteOrder);// 0th-IFD Tag-Numbers

	*buf_p += (sizeof(FIELD) * TagNums);

	pTagLoc = *buf_p - sizeof(FIELD);       // the last tag ptr;

	// (2-1). Find Exif tag and Jump to EXIF section. Here we search from tail to head. since
	// In normal case, this tag is the last tag.
	i = 0;

	while (1) {
		temp = GetCurSHORT(&pTagLoc, ByteOrder);
		if (temp == TAG_ID_ORIENTATION) {
			UINT16  usData;

			pTagLoc += 6;
			*buf_p = pTagLoc;
			usData = (UINT16) gPB_EXIFOri.ucEXIFOri;
			SetCurSHORT(&pTagLoc, ByteOrder, usData);

			return TRUE;
		} else {
			pTagLoc  -= (sizeof(FIELD) + 2);
		}

		i++;

		if (i == TagNums) {
			break;
		}
	}

	return FALSE;
#else
	EXIF_GETTAG exifTag;
	BOOL bIsLittleEndian = FALSE;

	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));

	exifTag.uiTagIfd = EXIF_IFD_0TH;
	exifTag.uiTagId = (UINT16)TAG_ID_ORIENTATION;
	if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
		*buf_p += exifTag.uiTagOffset;
		Set16BitsData((UINT32)(*buf_p), gPB_EXIFOri.ucEXIFOri,  bIsLittleEndian);
		return TRUE;

	} else {
		return FALSE;
	}
#endif
}

/*
    Update EXIF-Orientation tag function.
    This is internal API.

    @param[in] bIfOverWrite
    @return E_OK: OK, E_SYS: NG
*/
INT32 PB_UpdateEXIFHandle(BOOL bIfOverWrite)
{
	INT32 iErrorCheck;
	UINT8 *pucFileBuf;
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;

	//(1) Open file for write
	if (bIfOverWrite) {
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILEPATH, (UINT32 *)cFileName, NULL);
	}

	//(2) Modify EXIF-Orientation tag
	pucFileBuf = (UINT8 *)guiPlayFileBuf;
	if (PB_SetExif_IFD0_Orientation(&pucFileBuf) == FALSE) {
		DBG_ERR("Update EXIF FAIL!\r\n");
		return E_SYS;
	}

	//(3) Update file
	if (bIfOverWrite == FALSE) {
		UINT64 uiFileSize;
		UINT64 ui64StorageRemainSize;
		UINT32 uiErrCheck;
		PBXFile_Access_Info64 fileAccess = {0};

		// get disk-info, remain storage size
		ui64StorageRemainSize = PBXFile_GetFreeSpace();
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &uiFileSize, NULL);

		if (ui64StorageRemainSize <= uiFileSize) {
			return E_SYS;
		}
		uiErrCheck = pFlist->MakeFilePath(PBX_FLIST_FILE_TYPE_JPG, cFileName);
		if (uiErrCheck != E_OK) {
			DBG_ERR("MakePath fail!\r\n");
			return E_SYS;
		}
		fileAccess.fileCmd = PBX_FILE_CMD_WRITE;
		fileAccess.fileName = (UINT8 *)cFileName;
		fileAccess.pBuf = (UINT8 *)guiPlayFileBuf;
		fileAccess.bufSize = (UINT32)uiFileSize;
		fileAccess.filePos = 0;
		iErrorCheck = PBXFile_Access64(&fileAccess);
		if (iErrorCheck == E_OK) {
			pFlist->AddFile(cFileName);
			//#NT#2012/06/25#Lincy Lin -begin
			//#NT#Fix read wrong file bug
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.CurIndexOfTotal, NULL);
			pFlist->GetInfo(PBX_FLIST_GETINFO_DIRID, &gMenuPlayInfo.DispDirName[gMenuPlayInfo.CurFileIndex - 1], NULL);
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILEID, &gMenuPlayInfo.DispFileName[gMenuPlayInfo.CurFileIndex - 1], NULL);
			gMenuPlayInfo.DispFileSeq[gMenuPlayInfo.CurFileIndex - 1] = gMenuPlayInfo.CurIndexOfTotal;
			//#NT#2012/06/25#Lincy Lin -end
		}
	} else {
		UINT32 uiFilePos, uiFileSize = 2;
		PBXFile_Access_Info64  fileAccess = {0};

		uiFilePos = (UINT32)pucFileBuf - guiPlayFileBuf;
		fileAccess.fileCmd = PBX_FILE_CMD_UPDATE;
		fileAccess.fileName = (UINT8 *)cFileName;
		fileAccess.pBuf = (UINT8 *)pucFileBuf;
		fileAccess.bufSize = uiFileSize;
		fileAccess.filePos = uiFilePos;
		iErrorCheck = PBXFile_Access64(&fileAccess);
	}

	if (iErrorCheck == E_OK) {
		return E_OK;
	} else {
		DBG_ERR("Update File Error!\r\n");
		return E_SYS;
	}
}
//#NT#2010/08/04#Ben Wang -end

