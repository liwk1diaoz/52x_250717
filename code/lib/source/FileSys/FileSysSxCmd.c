#include <stdio.h>
#include "FileSysTsk.h"
#include "FileSysInt.h"
#include "kwrap/sxcmd.h"

#define THIS_DBGLVL         1 //0=OFF, 1=ERROR, 2=TRACE
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          filesys_sxcmd
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int filesys_sxcmd_debug_level;

#if FS_SXCMD_ENABLE

#define FS_FIXED_DRV        'A'
#define FS_TEMP_BUF_SIZE    0x1800000

//print a long file name
static void PrintLFN(UINT16 *pLongName)
{
	if (pLongName == 0) {
		DBG_ERR("pLongName = 0\r\n");
		return;//skip null pointer
	}

	DBG_DUMP("LFN ");
	while (*pLongName != 0) {
		DBG_DUMP("%c", *pLongName++);
	}
	DBG_DUMP("\r\n");
}

static UINT32 IsExist(char *fullpath)
{
	FST_FILE    filehdl;

	if (NULL == fullpath) {
		DBG_ERR("filefullpath is NULL\r\n");
		return FALSE;
	}

	filehdl = FileSys_OpenFile(fullpath, FST_OPEN_READ);

	if (NULL == filehdl) {
		//not exist
		return FALSE;
	} else {
		//exist
		if (FST_STA_OK != FileSys_CloseFile(filehdl)) {
			DBG_ERR("close %s failed\r\n", fullpath);
		}
		return TRUE;
	}
}

static BOOL createfile(char *path, UINT64 FixedSize)
{
	FST_FILE    filehdl;
	UINT32      WriteSize;

	if (NULL == path) {
		DBG_ERR("filefullpath is NULL\r\n");
		return FALSE;
	}

	WriteSize = strlen(path);

	filehdl = FileSys_OpenFile(path, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	if (NULL == filehdl) {
		DBG_ERR("Create %s fail (open)\r\n", path);
		return FALSE;
	}

	if (FST_STA_OK != FileSys_WriteFile(filehdl, (UINT8 *)path, &WriteSize, 0, NULL)) {
		DBG_ERR("Create %s fail (write)\r\n", path);
	}

	if (FixedSize) {
		if (FST_STA_OK != FileSys_AllocFile(filehdl, FixedSize, 0, 0)) {
			DBG_ERR("Create %s fail (alloc)\r\n", path);
		}
	}

	if (FST_STA_OK != FileSys_CloseFile(filehdl)) {
		DBG_ERR("Create %s fail (close)\r\n", path);
		return FALSE;
	}

	DBG_DUMP("%s created\r\n", path);
	return TRUE;
}

static BOOL dualrec(CHAR Drive)
{
#define HEADER_SIZE  (0x40000)
#define PATH1_SIZE   (1*1024*1024)
#define PATH2_SIZE   (1*1024*1024)
#define WRITE_LOOP   (140)
#define ALIGN_SIZE   (32*1024*1024)

#define DIRNAME     "DUALREC"
#define FNAME1      "PATH1.REC"
#define FNAME2      "PATH2.REC"

	CHAR            szTestDir[20] = {0};
	CHAR            szPath1[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	CHAR            szPath2[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	INT32           result1 = FSS_OK, result2 = FSS_OK;
	FST_FILE        pFile1 = NULL, pFile2 = NULL;
	UINT8           *pBuf = NULL;
	UINT32          LoopCount;
	UINT32          WriteSize;
	UINT32          PreAllocSize1 = 0, PreAllocSize2 = 0;

	//delete the old test folder
	snprintf(szTestDir, sizeof(szTestDir), "%c:\\%s", Drive, DIRNAME);
	if (IsExist(szTestDir)) {
		if (FSS_OK != FileSys_DeleteDir(szTestDir)) {
			DBG_ERR("DeleteDir %s failed\r\n", szTestDir);
			return FALSE;
		}
	}

	pBuf = (UINT8 *)SxCmd_GetTempMem(FS_TEMP_BUF_SIZE);
	if (!pBuf) {
		DBG_ERR("pBuf is NULL\r\n");
		return FALSE;
	}

	snprintf(szPath1, sizeof(szPath1), "%s\\%s", szTestDir, FNAME1);
	snprintf(szPath2, sizeof(szPath2), "%s\\%s", szTestDir, FNAME2);

	DBG_DUMP("O");
	pFile1 = FileSys_OpenFile(szPath1, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	if (NULL == pFile1) {
		DBG_ERR("Open %s failed\r\n", szPath1);
		result1 = FSS_FAIL;
		goto L_dualrec_END;
	}

	DBG_DUMP("o");
	pFile2 = FileSys_OpenFile(szPath2, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	if (NULL == pFile2) {
		DBG_ERR("Open %s failed\r\n", szPath2);
		result2 = FSS_FAIL;
		goto L_dualrec_END;
	}

	//path1
	DBG_DUMP("W");
	WriteSize = HEADER_SIZE;
	if (PreAllocSize1 < WriteSize) {
		if (FST_STA_OK != FileSys_AllocFile(pFile1, FileSys_TellFile(pFile1) + ALIGN_SIZE, 0, 0)) {
			DBG_ERR("Alloc %s failed\r\n", szPath1);
		}
		PreAllocSize1 += ALIGN_SIZE;
	}
	if (FST_STA_OK != FileSys_WriteFile(pFile1, pBuf, &WriteSize, FST_FLAG_NONE, NULL)) {
		DBG_ERR("Write %s failed\r\n", szPath1);
		result1 = FSS_FAIL;
		goto L_dualrec_END;
	}
	PreAllocSize1 -= WriteSize;

	//path2
	DBG_DUMP("w");
	WriteSize = HEADER_SIZE;
	if (PreAllocSize2 < WriteSize) {
		if (FST_STA_OK != FileSys_AllocFile(pFile2, FileSys_TellFile(pFile2) + ALIGN_SIZE, 0, 0)) {
			DBG_ERR("Alloc %s failed\r\n", szPath2);
		}
		PreAllocSize2 += ALIGN_SIZE;
	}
	if (FST_STA_OK != FileSys_WriteFile(pFile2, pBuf, &WriteSize, FST_FLAG_NONE, NULL)) {
		result2 = FSS_FAIL;
		goto L_dualrec_END;
	}
	PreAllocSize2 -= WriteSize;

	LoopCount = WRITE_LOOP;
	while (LoopCount--) {
		//path1
		DBG_DUMP("W");
		WriteSize = PATH1_SIZE;
		if (PreAllocSize1 < WriteSize) {
			if (FST_STA_OK != FileSys_AllocFile(pFile1, FileSys_TellFile(pFile1) + ALIGN_SIZE, 0, 0)) {
				DBG_ERR("Alloc %s failed\r\n", szPath1);
			}
			PreAllocSize1 += ALIGN_SIZE;
		}
		if (FST_STA_OK != FileSys_WriteFile(pFile1, pBuf, &WriteSize, FST_FLAG_NONE, NULL)) {
			DBG_ERR("Write %s failed\r\n", szPath1);
			result1 = FSS_FAIL;
			goto L_dualrec_END;
		}
		PreAllocSize1 -= WriteSize;

		//path2
		DBG_DUMP("w");
		WriteSize = PATH2_SIZE;
		if (PreAllocSize2 < WriteSize) {
			if (FST_STA_OK != FileSys_AllocFile(pFile2, FileSys_TellFile(pFile2) + ALIGN_SIZE, 0, 0)) {
				DBG_ERR("Alloc %s failed\r\n", szPath2);
			}
			PreAllocSize2 += ALIGN_SIZE;
		}
		if (FST_STA_OK != FileSys_WriteFile(pFile2, pBuf, &WriteSize, FST_FLAG_NONE, NULL)) {
			DBG_ERR("Write %s failed\r\n", szPath2);
			result2 = FSS_FAIL;
			goto L_dualrec_END;
		}
		PreAllocSize2 -= WriteSize;

		//path1
		DBG_DUMP("W");
		WriteSize = PATH1_SIZE;
		if (PreAllocSize1 < WriteSize) {
			if (FST_STA_OK != FileSys_AllocFile(pFile1, FileSys_TellFile(pFile1) + ALIGN_SIZE, 0, 0)) {
				DBG_ERR("Alloc %s failed\r\n", szPath1);
			}
			PreAllocSize1 += ALIGN_SIZE;
		}
		if (FST_STA_OK != FileSys_WriteFile(pFile1, pBuf, &WriteSize, FST_FLAG_NONE, NULL)) {
			DBG_ERR("Write %s failed\r\n", szPath1);
			result1 = FSS_FAIL;
			goto L_dualrec_END;
		}
		PreAllocSize1 -= WriteSize;

		//path1
		DBG_DUMP("F");
		if (FST_STA_OK != FileSys_FlushFile(pFile1)) {
			DBG_ERR("Flush %s failed\r\n", szPath1);
			result1 = FSS_FAIL;
			goto L_dualrec_END;
		}
		//path2
		DBG_DUMP("f");
		if (FST_STA_OK != FileSys_FlushFile(pFile2)) {
			DBG_ERR("Flush %s failed\r\n", szPath2);
			result2 = FSS_FAIL;
			goto L_dualrec_END;
		}
	}
L_dualrec_END:
	if (pBuf)
		SxCmd_RelTempMem((UINT32)pBuf);

	if (pFile1) {
		DBG_DUMP("C");
		if (FSS_OK != FileSys_CloseFile(pFile1)) {
			DBG_ERR("Close %s failed\r\n", szPath1);
			result1 = FSS_FAIL;
		}
	}
	if (pFile2) {
		DBG_DUMP("c");
		if (FSS_OK != FileSys_CloseFile(pFile2)) {
			DBG_ERR("Close %s failed\r\n", szPath2);
			result2 = FSS_FAIL;
		}
	}
	DBG_DUMP("\r\n");

	/*
	if(FSS_OK != FileSys_DeleteDir(szTestDir))
	{
	    DBG_ERR("rmdir %s failed\r\n", szTestDir);
	    return FALSE;
	}
	*/

	if (FSS_OK != result1 || FSS_OK != result2) {
		return FALSE;
	} else {
		return TRUE;
	}
}

static BOOL fs_sxcmd_createfile(CHAR *strCmd)
{
	static char path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	static char tmpfilename[KFS_LONGNAME_PATH_MAX_LENG];
	UINT64 FileSize = 0;
	UINT32 i, FileNum = 0;

	sscanf_s(strCmd, "%s %d %llu",  path, sizeof(path),
			 &FileNum,
			 &FileSize);
	if (0 == FileNum) {
		createfile(path, FileSize);
	} else {
		for (i = 1; i <= FileNum; i++) {
			snprintf(tmpfilename, sizeof(tmpfilename), "%s\\F%07d.JPG", path, i);
			createfile(tmpfilename, FileSize);
		}
	}
	return TRUE;
}

static BOOL fs_sxcmd_format(CHAR *strCmd)
{
	FS_HANDLE StrgDXH;

	if (FST_STA_OK != FileSys_GetStrgObj(&StrgDXH)) {
		DBG_ERR("FileSys_GetStrgObj failed\r\n");
		return FALSE;
	}

	if (FST_STA_OK != FileSys_FormatAndLabel(FS_FIXED_DRV, StrgDXH, FALSE, NULL)) {
		DBG_ERR("FileSys_FormatAndLabel failed\r\n");
		return FALSE;
	}

	return TRUE;
}


static BOOL fs_sxcmd_benchmark(CHAR *strCmd)
{
	UINT8 *pBuf = NULL;

	// benchmark size should over 16MB
	pBuf = (UINT8 *)SxCmd_GetTempMem(FS_TEMP_BUF_SIZE);
	if (!pBuf) {
		DBG_ERR("pBuf is NULL\r\n");
		return TRUE;
	}

	if (FST_STA_OK != FileSys_BenchmarkEx(FS_FIXED_DRV, 0, pBuf, FS_TEMP_BUF_SIZE))
		DBG_ERR("FileSys_BenchmarkEx failed\r\n");

	SxCmd_RelTempMem((UINT32)pBuf);

	return TRUE;
}

static BOOL fs_sxcmd_showinfo(CHAR *strCmd)
{
	CHAR szLabel[KFS_FILENAME_MAX_LENG] = {0};

	DBG_DUMP("DiskReady   = %lld\r\n", FileSys_GetDiskInfoEx(FS_FIXED_DRV, FST_INFO_DISK_RDY));
	DBG_DUMP("DiskSize    = %lld KB\r\n", FileSys_GetDiskInfoEx(FS_FIXED_DRV, FST_INFO_DISK_SIZE) / 1024);
	DBG_DUMP("freeSpace   = %lld KB\r\n", FileSys_GetDiskInfoEx(FS_FIXED_DRV, FST_INFO_FREE_SPACE) / 1024);
	DBG_DUMP("VolumeID    = 0x%08llX\r\n", FileSys_GetDiskInfoEx(FS_FIXED_DRV, FST_INFO_VOLUME_ID));
	DBG_DUMP("ClusterSize = %lld KB\r\n", FileSys_GetDiskInfoEx(FS_FIXED_DRV, FST_INFO_CLUSTER_SIZE));

	FileSys_GetLabel(FS_FIXED_DRV, szLabel);
	DBG_DUMP("Label       = %s\r\n", szLabel);

	return TRUE;
}

static BOOL fs_sxcmd_mkdir(CHAR *strCmd)
{
	static char path[KFS_LONGNAME_PATH_MAX_LENG] = {0};

	sscanf_s(strCmd, "%s %d",  path, sizeof(path));

	if (FST_STA_OK != FileSys_MakeDir(path)) {
		DBG_ERR("FileSys_MakeDir %s failed\r\n", path);
		return FALSE;
	}

	return TRUE;
}

static BOOL fs_sxcmd_rmdir(CHAR *strCmd)
{
	static char path[KFS_LONGNAME_PATH_MAX_LENG] = {0};

	sscanf_s(strCmd, "%s %d",  path, sizeof(path));

	if (FST_STA_OK != FileSys_DeleteDir(path)) {
		DBG_ERR("FileSys_DeleteDir %s failed\r\n", path);
		return FALSE;
	}

	return TRUE;
}

static BOOL fs_sxcmd_rm(CHAR *strCmd)
{
	static char path[KFS_LONGNAME_PATH_MAX_LENG] = {0};

	sscanf_s(strCmd, "%s %d",  path, sizeof(path));

	if (FST_STA_OK != FileSys_DeleteFile(path)) {
		DBG_ERR("FileSys_DeleteFile %s failed\r\n", path);
		return FALSE;
	}

	return TRUE;
}

static BOOL fs_sxcmd_list(CHAR *strCmd)
{
	static char path[KFS_LONGNAME_PATH_MAX_LENG] = {0};

	sscanf_s(strCmd, "%s %d %llu",  path, sizeof(path));

	FS_SEARCH_HDL pSrhDir;
	static FIND_DATA findDir;
	static char szFATName[8 + 1 + 3 + 1] = {0}; //8 + ' ' + 3 + '\0'
	static UINT16 wszFileName[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	int result = 0;

	sscanf_s(strCmd, "%s", path, sizeof(path));

	pSrhDir = fs_SearchFileOpen(path, &findDir, KFS_HANDLEONLY_SEARCH, wszFileName); //only get the handle
	if (NULL == pSrhDir) {
		DBG_ERR("Open dir %s failed\r\n", path);
		return FALSE;
	}

	while (pSrhDir) {
		result = fs_SearchFile(pSrhDir, &findDir, KFS_FORWARD_SEARCH, wszFileName);//get long name
		if (result != FSS_OK) {
			DBG_IND("No object:%d\r\n", result);
			fs_SearchFileClose(pSrhDir);
			break;
		} else {
			memcpy(szFATName, findDir.FATMainName, 8); // 0~7 : main name
			szFATName[8] = ' '; // 8 : space
			memcpy(szFATName + 9, findDir.FATExtName, 3); // 9~11 : ext name
			szFATName[12] = '\0'; // 12 : null terminated
			DBG_DUMP("SFN %s, attr 0x%X, size %12lld, ", szFATName, findDir.attrib, (UINT64)findDir.fileSize + ((UINT64)findDir.fileSizeHI << 32));
			PrintLFN(wszFileName);
		}
	}
	return TRUE;
}

static BOOL fs_sxcmd_dualrec(CHAR *strCmd)
{
	if (TRUE != dualrec(FS_FIXED_DRV)) {
		DBG_ERR("dualrec failed\r\n");
	}

	return TRUE;
}

static BOOL fs_sxcmd_test(CHAR *strCmd)
{
	DBG_DUMP("fs_sxcmd_test\r\n");
	return TRUE;
}

static SXCMD_BEGIN(fs, "File System SxCmd")
SXCMD_ITEM("format %", fs_sxcmd_format, "Format the disk")
SXCMD_ITEM("benchmark %", fs_sxcmd_benchmark, "Access speed test")
SXCMD_ITEM("showinfo %", fs_sxcmd_showinfo, "Show disk info")
SXCMD_ITEM("createfile %", fs_sxcmd_createfile, "Create files: [num] [size]")
SXCMD_ITEM("rm %", fs_sxcmd_rm, "Remove a file: [path]")
SXCMD_ITEM("mkdir %", fs_sxcmd_mkdir, "Create a dir: [path]")
SXCMD_ITEM("rmdir %", fs_sxcmd_rmdir, "Remove a dir: [path]")
SXCMD_ITEM("list %", fs_sxcmd_list, "List files of a dir: [path]")
SXCMD_ITEM("dualrec %", fs_sxcmd_dualrec, "Run dualrec simulation")
SXCMD_ITEM("test %", fs_sxcmd_test, "test")
SXCMD_END()

#endif //FS_SXCMD_ENABLE

void xFileSys_InstallCmd(void)
{
#if FS_SXCMD_ENABLE
	static BOOL bInstalled = FALSE;
	if (!bInstalled) {
		SxCmd_AddTable(fs);
		bInstalled = TRUE;
	}
#endif
}

