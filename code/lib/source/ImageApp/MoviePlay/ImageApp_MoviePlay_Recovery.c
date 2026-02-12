/*
    SMedia Play Recovery.

    This file is the recovery of bad media files.

    @file       ImageApp_MoviePlay_Recovery.c
    @ingroup    mIAPPMEDIAPLAY
    @note       Nothing.
    @version    V1.01.005
    @date       2012/08/15

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#include "avfile/MediaWriteLib.h"
#include "avfile/AVFile_MakerMov.h"
#include "FileSysTsk.h"
#include "kwrap/error_no.h"
#include <string.h>
//#include "SMediaPlayTsk.h"
//#include "MP_AVI.h"

#define MODULE_DBGLVL           6 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#define __MODULE__          MediaRecover
#define __DBGLVL__          ((THIS_DBGLVL >= MODULE_DBGLVL) ? THIS_DBGLVL : MODULE_DBGLVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[RECOVER]" // media recovery message
#include "kwrap/debug.h"

// perf
#define Perf_GetCurrent(args...)     0

#define MEDIARECR_FILE_TYPE_NUM            3
#define MEDIARECR_FILE_EXT_LEN             3
#define MEDIARECR_FULL_FILE_PATH_LEN      100    // "A:\DCIM\100NVTIM\IMAG0001.AVI & '\0' //2018/07/26 long name
// "A:\CARDV\MOVIE\2000_0113_205605_001.MOV & '\0'

typedef struct {
	UINT32          Type;
	CHAR            ExtStr[MEDIARECR_FILE_EXT_LEN + 1];
} MEDIARECR_FILE_TYPE;

CONTAINERMAKER               gMediaRecvryMaker     = {0};
FST_FILE                     ghRecvryfilehdl       = NULL;
static MEDIARECR_FILE_TYPE   gMediaRecrFiletype[MEDIARECR_FILE_TYPE_NUM] = {
	{MEDIA_FILEFORMAT_MOV, "MOV"},
	{MEDIA_FILEFORMAT_MP4, "MP4"},
	{MEDIA_FILEFORMAT_AVI, "AVI"}
};
UINT32 gMediaPlayRvyMsg = 0;

static void SMediaRecvry_FileSysCB(UINT32 MessageID, UINT32 Param1, UINT32 Param2, UINT32 Param3)
{
	if (MessageID == (UINT32)FST_STA_ERROR) {
		DBG_ERR("FS read error msgID = %d!!!\r\n", MessageID);
	}
}

static ER SMediaRecvryCBReadFile(UINT8 *type, UINT64 pos, UINT32 size, UINT32 addr)
{
	ER result;

	if (ghRecvryfilehdl) {
		result = FileSys_SeekFile(ghRecvryfilehdl, pos, FST_SEEK_SET);
		if (result != FST_STA_OK) {
			DBG_ERR("seek file failed\r\n");
			return E_PAR;
		} else {
			return FileSys_ReadFile(ghRecvryfilehdl, (UINT8 *)addr, &size, 0, SMediaRecvry_FileSysCB);
		}
	} else {
		return E_PAR;
	}
}

static ER SMediaRecvry_CreateContainerObj(UINT32 type)
{
	ER createSuccess = E_OK;
	PCONTAINERMAKER ptr;

	if (type == 0) {
		return E_NOSPT;
	}

	switch (type) {
	case MEDIA_FILEFORMAT_MOV:
	case MEDIA_FILEFORMAT_MP4: {
			ptr = mov_getContainerMaker();
			if (ptr == NULL) { // fix coverity CID:32145, by Adam 2018/10/18
				DBG_ERR("get container is NULL\r\n");
				return E_SYS;
			} else {
				gMediaRecvryMaker.Initialize    = ptr->Initialize;
				gMediaRecvryMaker.GetInfo       = ptr->GetInfo;
				gMediaRecvryMaker.SetInfo       = ptr->SetInfo;
				gMediaRecvryMaker.CustomizeFunc = ptr->CustomizeFunc;
				gMediaRecvryMaker.checkID       = ptr->checkID;
				gMediaRecvryMaker.cbShowErrMsg  = vk_printk;
				gMediaRecvryMaker.cbReadFile    = SMediaRecvryCBReadFile;
				gMediaRecvryMaker.id = ptr->id;
				createSuccess = MovWriteLib_RegisterObjCB(&gMediaRecvryMaker);
			}
			break;
		}
	}

	return createSuccess;
}

static ER SMediaRecvry_SetInfo(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	ER returnV;

	if (gMediaRecvryMaker.SetInfo) {
		returnV = (gMediaRecvryMaker.SetInfo)(type, p1, p2, p3);
	} else {
		returnV = E_NOSPT;
	}

	return returnV;
}

static ER SMediaRecvry_GetInfo(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	ER returnV;

	if (gMediaRecvryMaker.GetInfo) {
		returnV = (gMediaRecvryMaker.GetInfo)(type, p1, p2, p3);
	} else {
		returnV = E_NOSPT;
	}

	return returnV;
}

static void SMediaRecvry_Initialize(void)
{
	if (gMediaRecvryMaker.Initialize) {
		(gMediaRecvryMaker.Initialize)(gMediaRecvryMaker.id);
	}
}

static void SMediaRecvry_SetMemBuf(UINT32 memAddr, UINT32 size)
{
	if (gMediaRecvryMaker.SetInfo) {
		(gMediaRecvryMaker.SetInfo)(MEDIAWRITE_SETINFO_RECVRY_MEMBUF, memAddr, size, 0);
	}
}

static ER SMediaRecvry_MP_FileRecovery(void)
{
	if (gMediaRecvryMaker.CustomizeFunc) {
		return (gMediaRecvryMaker.CustomizeFunc)(MEDIAWRITELIB_CUSTOM_FUNC_IDXTLB_RECOVERY, 0);
	} else {
		return E_NOSPT;
	}
}

static UINT32 SMediaRecvry_GetFileType(char *pFilePath)
{
	UINT32 i = 0;
	UINT32 uifiletype = 0;
	CHAR   pExtName[MEDIARECR_FILE_EXT_LEN + 1];

	// Get File ExtName
	for (i = 0; i < MEDIARECR_FULL_FILE_PATH_LEN; i++) {
		if (pFilePath[i] == '\0') {
			break;
		}
	}

	if (i == MEDIARECR_FULL_FILE_PATH_LEN) {
		return 0;
	}

	memcpy(pExtName, &pFilePath[i - MEDIARECR_FILE_EXT_LEN], MEDIARECR_FILE_EXT_LEN);
	pExtName[MEDIARECR_FILE_EXT_LEN] = '\0';

	for (i = 0; i < MEDIARECR_FILE_TYPE_NUM; i++) {
		if (strstr(gMediaRecrFiletype[i].ExtStr, pExtName) != 0) {
			uifiletype = gMediaRecrFiletype[i].Type;
			break;
		}
	}

	return uifiletype;
}


/**
    @addtogroup mIAPPMEDIAPLAY
*/
//@{


/*
    Video File Recovery Function API

    File recovery API for power-cut porection.

    @param[in] pFilePath   video file full path name
    @param[in] memAddr     memory address
    @param[in] size        memory size
    @return
     - @b E_OK:     Recover the video file successfully.
     - @b E_SYS:    Recovery function is failed.
*/

ER SMediaPlay_FileRecovery(char *pFilePath, UINT32 memAddr, UINT32 size)
{
	INT32    ret;
	ER		 result;
	UINT32   uifiletype = 0;
	UINT32   MediaRecvry_newidx1Addr = 0, uiidx1size = 0;
	UINT32   MediaRecvry_Back1sthead = 0, uiheadsize = 0;
	UINT32   uiClusterSize, id, newtruncatesize = 0, newtrun_h = 0;
	UINT32   tt1, tt2, tt3;
	UINT64   uiTotalFileSize64 = 0;
	UINT32   size_l = 0, size_h = 0;


	if (gMediaPlayRvyMsg) {
		DBG_DUMP("memAddr=0x%lx, size=0x%lx!\r\n", memAddr, size);
	}

	//
	// Open File to Read
	//
	ghRecvryfilehdl = FileSys_OpenFile(pFilePath, FST_OPEN_READ);
	if (!ghRecvryfilehdl) {
		return E_SYS;
	}

	// Seek the end of file to get the file size (position)
	result = FileSys_SeekFile(ghRecvryfilehdl, 0, FST_SEEK_END);
	if (result != FST_STA_OK) {
		DBG_ERR("seek file failed\r\n");
	}
	//uiTotalFileSize = FileSys_TellFile(ghRecvryfilehdl);
	uiTotalFileSize64 = FileSys_TellFile(ghRecvryfilehdl);
	if (gMediaPlayRvyMsg) {
		DBG_DUMP("^M uiTotalFileSize64 = 0x%llx\r\n", uiTotalFileSize64);
	}
	result = FileSys_SeekFile(ghRecvryfilehdl, 0, FST_SEEK_SET); // set to file start
	if (result != FST_STA_OK) {
		DBG_ERR("seek file failed\r\n");
	}

	//
	// Check File Type
	//
	uifiletype = SMediaRecvry_GetFileType(pFilePath);
	if (!uifiletype) {
		DBG_ERR("File type get error!!%s\r\n", pFilePath);
		FileSys_CloseFile(ghRecvryfilehdl);
		return E_SYS;
	}

	// Create File Container Object
	ret = SMediaRecvry_CreateContainerObj(uifiletype);
	if (ret != E_OK) {
		DBG_ERR("Create File Container fails...\r\n");
		FileSys_CloseFile(ghRecvryfilehdl);
		return ret;
	}

	//
	// Set Info
	//
	SMediaRecvry_Initialize();
	SMediaRecvry_SetMemBuf(memAddr, size);

	uiClusterSize = FileSys_GetDiskInfo(FST_INFO_CLUSTER_SIZE);
	id = 0;
	if (gMediaPlayRvyMsg) {

		DBG_DUMP("^M 22uiTotalFileSize64 = 0x%llx\r\n", uiTotalFileSize64);
	}
	size_l = (UINT32)uiTotalFileSize64;
	size_h = (UINT32)(uiTotalFileSize64 >> 32);
	//SMediaRecvry_SetInfo(MEDIAWRITE_SETINFO_RECVRY_FILESIZE, uiTotalFileSize, 0, 0);    // File Size
	SMediaRecvry_SetInfo(MEDIAWRITE_SETINFO_RECVRY_FILESIZE, size_l, size_h, 0);    // File Size
	SMediaRecvry_SetInfo(MEDIAWRITE_SETINFO_CLUSTERSIZE, uiClusterSize, 0, id);   // Cluster Size
	SMediaRecvry_SetInfo(MEDIAWRITE_SETINFO_MOVHDR_REVSIZE, 0x40000, 0, 0);   // MOVREC_AVI_HDR//2014/08/22 Meg
	if (size < 0x800000) // reference PB_MAX_FILE_SIZE
	{
		SMediaRecvry_SetInfo(MEDIAWRITE_SETINFO_RECVRY_BLKSIZE, 0x200000, 0, 0);
		SMediaRecvry_SetInfo(MEDIAWRITE_SETINFO_RECVRY_MAX_DUR, (10*60), 0, 0); //sec
	}
	else
	{
		SMediaRecvry_SetInfo(MEDIAWRITE_SETINFO_RECVRY_BLKSIZE, 0x400000, 0, 0);
	}

	//
	// Call File Revcovery in Media Lib
	//
	if (gMediaPlayRvyMsg) {

		DBG_DUMP("[RECOVER]start to make new idx1!\r\n");
	}
	tt1 = Perf_GetCurrent();
	ret = SMediaRecvry_MP_FileRecovery();
	if (ret != E_OK) {
		DBG_ERR("SMediaRecvry_MP_FileRecovery function fails...\r\n");
		FileSys_CloseFile(ghRecvryfilehdl); // Close File
		ghRecvryfilehdl = NULL;
		return ret;
	}
	tt2 = Perf_GetCurrent();
	if (gMediaPlayRvyMsg) {
		DBG_DUMP("RCVY step1 = %d ms\r\n", (tt2 - tt1) / 1000);
		DBG_DUMP("RCVY step1 = %d sec\r\n", (tt2 - tt1) / 1000000);
	}
	//
	// Open File to Recover
	//
	FileSys_CloseFile(ghRecvryfilehdl);
	ghRecvryfilehdl = NULL;

	ghRecvryfilehdl = FileSys_OpenFile(pFilePath, FST_OPEN_ALWAYS | FST_OPEN_WRITE);
	if (!ghRecvryfilehdl) {
		FileSys_CloseFile(ghRecvryfilehdl); // Close File
		return E_SYS;
	}

	// For AVI file
	// Get Buffer position and size
	SMediaRecvry_GetInfo(MEDIAWRITE_GETINFO_NEWIDX1_BUF, &MediaRecvry_newidx1Addr, &uiidx1size, 0);
	if (gMediaPlayRvyMsg) {
		DBG_DUMP("new moov 0x%lx, size0x%lx\r\n", MediaRecvry_newidx1Addr, uiidx1size);
	}
	SMediaRecvry_GetInfo(MEDIAWRITE_GETINFO_BACK1STHEAD_BUF, &MediaRecvry_Back1sthead, &uiheadsize, 0);
	SMediaRecvry_GetInfo(MEDIAWRITE_GETINFO_TRUNCATESIZE, &newtruncatesize, 0, 0);
	SMediaRecvry_GetInfo(MEDIAWRITE_GETINFO_TRUNCATESIZE_H, &newtrun_h, 0, 0);
	if (newtruncatesize) {
		uiTotalFileSize64 = ((UINT64)newtrun_h << 32) + (UINT64)newtruncatesize;
		DBG_DUMP("version 3.15 truncate size = 0x%llx!!\r\n", uiTotalFileSize64);
		//#NT#2020/08/28#Willy Su -begin
		//#NT# Fix FileSize Issue (Remove AllocFile)
		#if 0
		FileSys_AllocFile(ghRecvryfilehdl, (UINT64)uiTotalFileSize64, 0, 0);
		#endif
		//#NT#2020/08/28#Willy Su -end
	}
	if (gMediaPlayRvyMsg) {
		DBG_DUMP("^M 22uiTotalFileSize64 = 0x%llx\r\n", uiTotalFileSize64);
	}
	// Append the Index table
	result  = FileSys_SeekFile(ghRecvryfilehdl, uiTotalFileSize64, FST_SEEK_SET);
	if (result != FST_STA_OK) {
		DBG_ERR("seek file failed\r\n");
	}

	ret = FileSys_WriteFile(ghRecvryfilehdl, (UINT8 *)MediaRecvry_newidx1Addr, &uiidx1size, 0, SMediaRecvry_FileSysCB);
	if (gMediaPlayRvyMsg) {
		DBG_DUMP("^M write mem = 0x%lx size=0x%lx\r\n", MediaRecvry_newidx1Addr, uiidx1size);
	}

	if (ret != E_OK) {
		FileSys_CloseFile(ghRecvryfilehdl); // Close File
		return E_SYS;
	}
	// Update Header
	result = FileSys_SeekFile(ghRecvryfilehdl, 0, FST_SEEK_SET);
	if (result != FST_STA_OK) {
		DBG_ERR("seek file failed\r\n");
	}

	ret = FileSys_WriteFile(ghRecvryfilehdl, (UINT8 *)MediaRecvry_Back1sthead, &uiheadsize, 0, SMediaRecvry_FileSysCB);
	if (ret != E_OK) {
		FileSys_CloseFile(ghRecvryfilehdl); // Close File
		return E_SYS;
	}

	// Close File
	FileSys_CloseFile(ghRecvryfilehdl);
	tt3 = Perf_GetCurrent();
	if (gMediaPlayRvyMsg) {
		DBG_DUMP("RCVY step2 = %d ms\r\n", (tt3 - tt2) / 1000);
		DBG_DUMP("RCVY step2 = %d sec\r\n", (tt3 - tt2) / 1000000);
	}

	ghRecvryfilehdl = NULL;

	return E_OK;
}
//@}


