/*
    @file       ps_fs.c
    @ingroup    mISYSPStore

    @brief      PStore mount on filesystem function interface

    @note

    @version    V1.00.000
    @author     CL Wang
    @date       2006/10/25

    Copyright   Novatek Microelectronics Corp. 2006.  All rights reserved.

*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/vfs.h>    /* or <sys/statfs.h> */
#include <kwrap/type.h>
#include "PStore.h"
//#include "FileSysTsk.h"
#include "ps_int.h"
#include "ps_fs.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          PS_FS
#define __DBGLVL__          1 // 0=OFF, 1=ERROR, 2=TRACE
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////
#define PS_FS_PATH_LEN  (24)
#define BYTE_ALIGN      (0x1F)

static int partition_id = -1;
static char g_emmc_ps_dev_node[48] = {0};

ER  ps_fs_Open(STORAGE_OBJ *pThisStrgObj,PSTORE_INIT_PARAM* pInitParam)
{
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
	char cmd[128];
    ER result = E_PS_OK;
	BOOL isEmmc = FALSE;
	
	partition_id = -1;
	
	fp = fopen("/proc/mtd", "r");
    if (fp == NULL){
		//DBG_ERR("fail to open /proc/mtd\n");
        //return -1;
        DBG_DUMP("%s: no /proc/mtd, shoule be emmc device\n", __func__);
        isEmmc = TRUE;
	}

    if (isEmmc) {
        if (g_emmc_ps_dev_node[0] == 0) {
            DBG_ERR("eMMc: Please cat /proc/nvt_info/emmc and set the /dev/mmcblk2pXX for pstore\r\n");
        return -1;
	}
    }

	if(!isEmmc) {
    while ((read = getline(&line, &len, fp)) != -1) {
		if(strstr(line, "pstore")){
			sscanf(line, "mtd%d:", &partition_id);
			break;
		}
    }

		if(fp) {
    fclose(fp);
		}

	if (line)
		free(line);
		
	if(partition_id == -1){
		DBG_ERR("fail to find pstore mtd partition\n");
		return -1;
	}
	
	snprintf(cmd, sizeof(cmd), "mount -t yaffs2 /dev/mtdblock%d /mnt/pstore", partition_id);
	result = system(cmd);
	if(result)
		DBG_ERR("mount pstore fail with %d\n", result);
	} else { // isEmmc = TRUE.
		snprintf(cmd, sizeof(cmd), "mount -t ext4 /dev/mmcblk2p12 /mnt/pstore");
		result = system(cmd);
		if(result)
			DBG_ERR("mount pstore fail with %d\n", result);
	}

    return result;
}

ER  ps_fs_Close(void)
{
	int ret = system("umount /mnt/pstore");
	
	if(ret)
		DBG_ERR("umount pstore fail with %d\n", ret);
	
    return ret;
}


PSTORE_SECTION_HANDLE*  ps_fs_OpenSection(char *pSecName, UINT32 RWOperation)
{
    char pFilePath[PS_FS_PATH_LEN];
    UINT32 Flag =0;
	int fd;

    memset(pFilePath,0,PS_FS_PATH_LEN);

    if(strlen(pSecName)<= SEC_NAME_LEN)
        snprintf(pFilePath,PS_FS_PATH_LEN-1,"/mnt/pstore/%s",pSecName);
    else{
        DBG_ERR("%s exceed %d \r\n",pSecName,SEC_NAME_LEN);
		return E_PS_SECHDLER;
	}

	if((RWOperation & (PS_RDONLY | PS_WRONLY)) == (PS_RDONLY | PS_WRONLY))
        Flag |= O_RDWR;
    else if((RWOperation & PS_RDONLY) == PS_RDONLY)
        Flag |= O_RDONLY;
    else if((RWOperation & PS_WRONLY) == PS_WRONLY)
        Flag |= O_WRONLY;

    if((RWOperation & PS_CREATE) == PS_CREATE)
        Flag |= O_CREAT;
    //if((RWOperation & PS_UPDATE) == PS_UPDATE)
    //    Flag |= FST_OPEN_ALWAYS|FST_OPEN_WRITE;

    DBG_IND("pFilePath %s Flag 0x%X\r\n",pFilePath,Flag);

    fd = open(pFilePath, Flag, 0644);
	if(fd < 0){
		DBG_ERR("open(%s,%x) fail with %d\n", pFilePath, Flag, fd);
		return E_PS_SECHDLER;
	}

    return (PSTORE_SECTION_HANDLE*)fd;
}


ER  ps_fs_CloseSection(PSTORE_SECTION_HANDLE* pSectionHandle)
{
	int fd = (int)pSectionHandle;

	if(fd < 0){
		DBG_ERR("invalid pSectionHandle(%x)\n", pSectionHandle);
		return -1;
	}
	
	close(fd);
	
	return 0;
}


ER  ps_fs_DeleteSection(char *pSecName)
{
    char pFilePath[PS_FS_PATH_LEN];
    ER result = 0;

    memset(pFilePath,0,PS_FS_PATH_LEN);

    if(strlen(pSecName)<= SEC_NAME_LEN)
        snprintf(pFilePath,PS_FS_PATH_LEN-1,"/mnt/pstore/%s",pSecName);
    else{
        DBG_ERR("%s exceed %d \r\n",pSecName,SEC_NAME_LEN);
		return -1;
	}

    result = unlink(pFilePath);
	if(result){
		DBG_ERR("unlink(%s) fail with %d\n", pFilePath, result);
		result = -1;
	}

    return result;
}


ER  ps_fs_ReadSection(UINT8 *pcBuffer,UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle)
{
	int fd = (int)pSectionHandle;
    ER result =0;

    DBG_IND("pcBuffer 0x%x,ulStartAddress %d, ulDataLength %d, pSectionHandle 0x%x\r\n",pcBuffer,ulStartAddress, ulDataLength, pSectionHandle);
	
	if(fd < 0){
		DBG_ERR("invalid pSectionHandle(%x)\n", pSectionHandle);
		return -1;
	}
	
	//if(ulStartAddress){
		result = lseek(fd, ulStartAddress, SEEK_SET);
		if(result != (int)ulStartAddress){
			DBG_ERR("lseek(%d,%d) fail with %d\n", fd, ulStartAddress, result);
			return -1;
		}
	//}
	
    result = read  (fd, pcBuffer, (ssize_t) ulDataLength);
	if(result < 0){
		DBG_ERR("read (%d,%x,%d) fail with %d\n", fd, pcBuffer, ulDataLength, result);
		result = -1;
	}
	else
		result = 0;
	
    return result;
}


ER  ps_fs_WriteSection(UINT8 *pcBuffer, UINT32 ulStartAddress, UINT32 ulDataLength, PSTORE_SECTION_HANDLE* pSectionHandle)
{
	int fd = (int)pSectionHandle;
    ER result =0;

    DBG_IND("pcBuffer 0x%x,ulStartAddress %d, ulDataLength %d, pSectionHandle 0x%x\r\n",pcBuffer,ulStartAddress, ulDataLength, pSectionHandle);
	
	if(fd < 0){
		DBG_ERR("invalid pSectionHandle(%x)\n", pSectionHandle);
		return -1;
	}
	
	//if(ulStartAddress){
		result = lseek(fd, ulStartAddress, SEEK_SET);
		if(result != (int)ulStartAddress){
			DBG_ERR("lseek(%d,%d) fail with %d\n", fd, ulStartAddress, result);
			return -1;
		}
	//}
	
    result = write (fd, pcBuffer, (ssize_t) ulDataLength);
	if(result == (int)ulDataLength)
		result = 0;
	else{
		DBG_ERR("write(%d,%x,%d) fail with %d\n", fd, pcBuffer, ulDataLength, result);
		result = -1;
	}
	
    return result;
}

#if 0
/**
    Dump PSTORE structure and exist PSTORE_SECTION_DESC and PSTORE_SECTION_HANDLE
*/
void  ps_fs_DumpPStoreInfo(void)
{

    if(NULL == g_pCurPS)
    {
        DBG_WRN(" ps_fs_Init not ready\r\n");
        return;
    }

    if(g_pCurPS->DumpPStoreInfo)
        g_pCurPS->DumpPStoreInfo();
    else
        DBG_WRN("op not sup\r\n");

}
#endif


ER  ps_fs_Format(PSFMT *pFmtStruct)
{
    ER result =E_SYS;
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
	char cmd[128];
	BOOL isEmmc = FALSE;
	
	partition_id = -1;
	
	fp = fopen("/proc/mtd", "r");
    if (fp == NULL){
		//DBG_ERR("fail to open /proc/mtd\n");
        //return -1;
        //return -1;
        DBG_DUMP("%s: no /proc/mtd, shoule be emmc device\n", __func__);
        isEmmc = TRUE;
	}

    if (isEmmc) {
        if (g_emmc_ps_dev_node[0] == 0) {
            DBG_ERR("eMMc: Please cat /proc/nvt_info/emmc and set the /dev/mmcblk2pXX for pstore\r\n");
        return -1;
	}
    }

	if(!isEmmc) {
    while ((read = getline(&line, &len, fp)) != -1) {
		if(strstr(line, "pstore")){
			sscanf(line, "mtd%d:", &partition_id);
			break;
		}
    }

    fclose(fp);
	if (line)
		free(line);
		
	if(partition_id == -1){
		DBG_ERR("fail to find pstore mtd partition\n");
		return -1;
	}
	
	snprintf(cmd, sizeof(cmd), "flash_eraseall /dev/mtd%d", partition_id);
	result = system(cmd);
	if(result){
		DBG_ERR("flash_eraseall fail with %d\n", result);
		return result;
	}
	
	snprintf(cmd, sizeof(cmd), "mount -t yaffs2 /dev/mtdblock%d /mnt/pstore", partition_id);
	result = system(cmd);
	if(result)
		DBG_ERR("mount pstore fail with %d\n", result);
	} else { // isEmmc = TRUE.
	 	snprintf(cmd, sizeof(cmd), "yes | mkfs.ext4 /dev/mmcblk2p12");
		result = system(cmd);
		if(result){
			DBG_ERR("mkfs.ext4 fail with %d\n", result);
			return result;
		}

		DBG_DUMP("mkfs.ext4 finished\r\n");

		snprintf(cmd, sizeof(cmd), "mount -t ext4 /dev/mmcblk2p12 /mnt/pstore");
		result = system(cmd);
		if(result)
			DBG_ERR("mount pstore fail with %d\n", result);
		else
			DBG_DUMP("mount pstore finished\r\n");
	}

    return result;
}

UINT32 ps_fs_GetInfo(PS_INFO_ID info_id)
{
	struct statfs s;
	
	if (statfs("/mnt/pstore", &s) != 0) {
		DBG_ERR("statfs(/mnt/pstore) fail\n");
		return 0xFFFFFFFF;
    }
	
    switch (info_id)
    {
        case PS_INFO_TOT_SIZE:
            return (s.f_blocks * s.f_bsize);

        case PS_INFO_FREE_SPACE:
             return (s.f_bfree * s.f_bsize);

        case PS_INFO_BLK_SIZE:
             return s.f_bsize;

        default:
             DBG_ERR("info_id  %d not support\r\n",info_id);
             return 0;
    }
}
void ps_fs_DumpPStoreInfo(void)
{
	DBG_ERR("pstore dump is not supported in linux\n");
}
ER ps_fs_CheckSection(char *pSecName,UINT32 *ErrorSection)
{
    char pFilePath[PS_FS_PATH_LEN];
	struct stat dummy;
    ER result = 0;

    memset(pFilePath,0,PS_FS_PATH_LEN);

    if(strlen(pSecName)<= SEC_NAME_LEN)
        snprintf(pFilePath,PS_FS_PATH_LEN-1,"/mnt/pstore/%s",pSecName);
    else{
        DBG_ERR("%s exceed %d \r\n",pSecName,SEC_NAME_LEN);
		return -1;
	}

    result = stat(pFilePath, &dummy);
	if(ErrorSection){
		if(result)
			*ErrorSection = -1;
		else
			*ErrorSection = 0;
	}

    return 0;
}

ER ps_fs_SetDevNode(char *pDevNode)
{
    unsigned int len = 0;

    memset(g_emmc_ps_dev_node, 0, sizeof(g_emmc_ps_dev_node));

    len = strlen(pDevNode);
    if ((len+1) > sizeof(g_emmc_ps_dev_node)) {
        DBG_ERR("emmc_ps dev path too long, (len+1)=%d > %d\r\n", (len+1), sizeof(g_emmc_ps_dev_node));
        return -1;
    }

    memcpy(g_emmc_ps_dev_node, pDevNode, len);
    return 0;
}

#if 0
BOOL  ps_fs_SearchSectionOpen(PSTORE_SEARCH_HANDLE *pFindData)
{
    BOOL bFound = FALSE;
    if(NULL == g_pCurPS)
    {
        DBG_WRN(" ps_fs_Init not ready\r\n");
        return FALSE;
    }

    if(g_pCurPS->SearchSectionOpen)
        bFound = g_pCurPS->SearchSectionOpen(pFindData);
    else
        DBG_WRN("op not sup\r\n");

    return bFound;

}

BOOL  ps_fs_SearchSection(PSTORE_SEARCH_HANDLE *pFindData)
{
    BOOL bFound = FALSE;
    if(NULL == g_pCurPS)
    {
        DBG_WRN(" ps_fs_Init not ready\r\n");
        return FALSE;
    }

    if(g_pCurPS->SearchSection)
        bFound = g_pCurPS->SearchSection(pFindData);
    else
        DBG_WRN("op not sup\r\n");

    return bFound;

}

ER  ps_fs_SearchSectionClose(void)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN(" ps_fs_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->SearchSectionClose)
        Result = g_pCurPS->SearchSectionClose();
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}

ER  ps_fs_ReadOnly(char *pSecName,BOOL Enable)
{
    ER Result =E_PS_OPFAIL;
    if(NULL == g_pCurPS)
    {
        DBG_WRN(" ps_fs_Init not ready\r\n");
        return E_PS_INIT;
    }

    if(g_pCurPS->ReadOnly)
        Result = g_pCurPS->ReadOnly(pSecName,Enable);
    else
        DBG_WRN("op not sup\r\n");

    return Result;

}
#endif
/**
@}   //addtogroup
*/
