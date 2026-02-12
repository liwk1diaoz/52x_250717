#include <string.h>
#include <sdio.h>
#include <strg_def.h>
#include <hdal.h>
#include <FileSysTsk.h>
#include "sys_filesys.h"
#include "sys_mempool.h"

#define MAX_OPENED_FILE_NUM 10
void filesys_init(void)
{
	UINT32 uiPoolAddr;
	int    ret;

	FILE_TSK_INIT_PARAM     Param = {0};
	FS_HANDLE               StrgDXH;

	printf("filesys_init b\r\n");
	memset(&Param, 0, sizeof(FILE_TSK_INIT_PARAM));
	StrgDXH = (FS_HANDLE)sdio_getStorageObject(STRG_OBJ_FAT1);

	uiPoolAddr = mempool_filesys;
	Param.FSParam.WorkBuf = uiPoolAddr;
	Param.FSParam.WorkBufSize = (POOL_SIZE_FILESYS);
	// support exFAT
	Param.FSParam.bSupportExFAT   = TRUE;
	//Param.pDiskErrCB = (FileSys_CB)Card_InitCB;
	strncpy(Param.FSParam.szMountPath, "/mnt/sd", sizeof(Param.FSParam.szMountPath) - 1); //only used by FsLinux
	Param.FSParam.szMountPath[sizeof(Param.FSParam.szMountPath) - 1] = '\0';
	Param.FSParam.MaxOpenedFileNum = MAX_OPENED_FILE_NUM;
	FileSys_InstallID(FileSys_GetOPS_Linux());
	if (FST_STA_OK != FileSys_Init(FileSys_GetOPS_Linux())) {
		printf("FileSys_Init failed\r\n");
	}
	ret = FileSys_OpenEx('A', StrgDXH, &Param);
	if (FST_STA_OK != ret) {
		printf("FileSys_Open err %d\r\n", ret);
	}
	// call the function to wait init finish
	FileSys_WaitFinishEx('A');
	printf("filesys_init e\r\n");
}