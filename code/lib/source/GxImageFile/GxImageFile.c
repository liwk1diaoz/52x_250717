#include <string.h>
#include "GxImageFile.h"
#include "exif/Exif.h"
#include "exif/ExifDef.h"
//#include "JpgEnc.h"
//#include "JpgHeader.h"
#include "BinaryFormat.h"
//#include "jpeg.h"
//#include "JpgDec.h"
//#include "jpeg_lmt.h"
#include "kwrap/error_no.h"
#include <kwrap/verinfo.h>
#include "Utility/Color.h"
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxImgFile
#define __DBGLVL__          THIS_DBGLVL
//#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[IO]"
#define __DBGFLT__          "[CHK]"
#include <kwrap/debug.h>


#define GXIMGFILE_ALIGNED_VALUE    (4-1)

VOS_MODULE_VERSION(GxImgFile, 1, 00, 000, 00)


ER GxImgFile_CombineJPG(PMEM_RANGE pExifData, PMEM_RANGE pPrimaryData, PMEM_RANGE pScreennailData, PMEM_RANGE pDstJpgFile)
{
	MAKERNOTE_INFO MakernoteInfo;
	UINT32 AlignOffset = 0, App1Length, RedundantLength = 0;
	UINT32 PrimaryAddr, PrimarySize = 0, ExifSize = 0, ScreennailSize = 0;
	DBG_FUNC_BEGIN("[IO]\r\n");
	if (pPrimaryData == NULL || pPrimaryData->size == 0) {
		DBG_ERR("Parameter ERROR!");
		return E_SYS;
	}
	DBG_MSG("[CHK]pPrimaryData Addr=0x%X, pPrimaryData=0x%X\r\n", pPrimaryData->addr, pPrimaryData->size);
	//Do NOT modified input papameter!
	PrimaryAddr = pPrimaryData->addr;
	PrimarySize = pPrimaryData->size;
	pDstJpgFile->addr = PrimaryAddr;
	if (pExifData && pExifData->size) {
		ExifSize = pExifData->size;
		DBG_MSG("[CHK]pExifData Addr=0x%X, Size=0x%X\r\n", pExifData->addr, ExifSize);
		if (Get16BitsData(pPrimaryData->addr, TRUE) == JPEG_MARKER_SOI) {
#if 0
			//ignore redundant SOI
			PrimaryAddr += 2;
			PrimarySize -= 2;
#endif
			RedundantLength = 2;
		}
		pDstJpgFile->addr = PrimaryAddr - ExifSize;
		//padding 0xFF to make the pDstJpgFile->addr word-aligned
		AlignOffset = (pDstJpgFile->addr & GXIMGFILE_ALIGNED_VALUE);
		pDstJpgFile->addr -= AlignOffset;
		if (pDstJpgFile->addr != pExifData->addr) {
			if (pExifData->addr < pDstJpgFile->addr && pExifData->addr + ExifSize > pDstJpgFile->addr) {
				DBG_ERR("Exif data will overlap! Please rearrange buffer.");
				return E_SYS;
			}
			DBG_MSG("Copy Exif data ...");
			//650 ben to do use hw copy
			memcpy((void *)pDstJpgFile->addr, (void *) pExifData->addr, ExifSize);
			//hwmem_open();
			//hwmem_memcpy(pDstJpgFile->addr, pExifData->addr, ExifSize);
			//hwmem_close();
		}
		if (AlignOffset + RedundantLength) {
			memset((void *)(pDstJpgFile->addr + ExifSize), 0xFF, AlignOffset + RedundantLength);
			//revise App1 length to avoid "unexpect data" showing in ExifTagVerifier
			App1Length = Get16BitsData(pDstJpgFile->addr + APP1_SIZE_OFFSET, FALSE);
			Set16BitsData(pDstJpgFile->addr + APP1_SIZE_OFFSET, App1Length + AlignOffset + RedundantLength, FALSE);
			DBG_MSG("[CHK]Ori App1Length = 0x%X, new App1Length = 0x%X\r\n", App1Length, App1Length + AlignOffset + RedundantLength);
		}
		DBG_MSG("[CHK]AlignOffset = %d, RedundantLength = %d\r\n", AlignOffset, RedundantLength);
		if (pScreennailData && pScreennailData->size) {
			if (EXIF_ER_OK == EXIF_GetInfo(EXIF_GetHandleID(pExifData->addr), EXIFINFO_MAKERNOTE, &MakernoteInfo, sizeof(MakernoteInfo))) {
				UINT32 ScreennailPading;
				MakernoteInfo.uiMakerNoteAddr = pDstJpgFile->addr + MakernoteInfo.uiMakerNoteOffset;
				//force to word-aligned for FST reading
				ScreennailPading = ALIGN_CEIL_4(ExifSize + AlignOffset + PrimarySize) - (ExifSize + AlignOffset + PrimarySize);
				AlignOffset += ScreennailPading;
				MakernoteInfo.uiScreennailOffset = ExifSize + AlignOffset + PrimarySize;
				MakernoteInfo.uiScreennailSize = pScreennailData->size;
				DBG_MSG("[CHK]MakerNote Info Addr=0x%X, Size=0x%X, Offset=0x%X, ScreenOffset=0x%X,ScreenSize=0x%X\r\n", MakernoteInfo.uiMakerNoteAddr, MakernoteInfo.uiMakerNoteSize, MakernoteInfo.uiMakerNoteOffset, MakernoteInfo.uiScreennailOffset, MakernoteInfo.uiScreennailSize);
				if (EXIF_ER_OK == EXIF_UpdateInfo(EXIF_GetHandleID(pExifData->addr), EXIFINFO_MAKERNOTE, &MakernoteInfo, sizeof(MakernoteInfo))) {
					ScreennailSize = pScreennailData->size;
					if ((pDstJpgFile->addr + MakernoteInfo.uiScreennailOffset) != pScreennailData->addr) {
						//DBGH(pDstJpgFile->addr+MakernoteInfo.uiScreennailOffset);
						//DBGH(pScreennailData->addr);
						//650 ben to do use hw copy
#if 1
						memcpy((void *)(pDstJpgFile->addr + MakernoteInfo.uiScreennailOffset), (void *) pScreennailData->addr, ScreennailSize);
#else
						hwmem_open();
						hwmem_memcpy((pDstJpgFile->addr + MakernoteInfo.uiScreennailOffset), pScreennailData->addr, ScreennailSize);
						hwmem_close();
#endif
					}
					DBG_MSG("[CHK]pScreennailData Addr=0x%X, Size=0x%X\r\n", pScreennailData->addr, ScreennailSize);
				} else {
					DBG_WRN("pScreennailData will NOT be combined because the callback event, UPDATE_MAKERNOTE, about screennail info is NOT complete.\r\n");
				}
			} else {
				DBG_WRN("pScreennailData will NOT be combined because of MakerNote error.\r\n");
			}


		}
	} else if (pScreennailData && pScreennailData->size) {
		DBG_WRN("pScreennailData will NOT be combined since there is no EXIF data to record screennail info.\r\n");
	}
	pDstJpgFile->size = ExifSize + AlignOffset + PrimarySize + ScreennailSize;
	DBG_MSG("[CHK]pDstJpgFile Addr=0x%X, Size=0x%X\r\n", pDstJpgFile->addr, pDstJpgFile->size);
	DBG_FUNC_END("[IO]\r\n");
	return E_OK;
}
