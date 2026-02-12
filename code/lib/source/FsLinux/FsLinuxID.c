#include "FsLinuxDebug.h"
#include "FsLinuxID.h"
#include "FsLinuxTsk.h"

#define PRI_FSLINUX                10
#define STKSIZE_FSLINUX            4*1024

#if FSLINUX_USE_IPC
ID FSLINUX_COMMON_FLG_ID = 0;
ID FSLINUX_COMMON_TSK_ID = 0;
#endif

FSLINUX_CTRL_OBJ gFsLinuxCtrlObj[FSLINUX_DRIVE_NUM] = {0};

void FsLinux_InstallID(void)
{
	UINT32 i;
#if FSLINUX_USE_IPC
	OS_CONFIG_FLAG(FSLINUX_COMMON_FLG_ID);
#endif
	for (i = 0; i < FSLINUX_DRIVE_NUM; i++) {
		OS_CONFIG_FLAG(gFsLinuxCtrlObj[i].FlagID);
		OS_CONFIG_SEMPHORE(gFsLinuxCtrlObj[i].SemID, 0, 1, 1);
	}

}

void FsLinux_UninstallID(void)
{
	UINT32 i;
#if FSLINUX_USE_IPC
	rel_flg(FSLINUX_COMMON_FLG_ID);
#endif
	for (i = 0; i < FSLINUX_DRIVE_NUM; i++) {
		rel_flg(gFsLinuxCtrlObj[i].FlagID);
		SEM_DESTROY(gFsLinuxCtrlObj[i].SemID);
	}
}

PFSLINUX_CTRL_OBJ FsLinux_GetCtrlObjByDrive(CHAR Drive)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;

	if (Drive < FSLINUX_DRIVE_NAME_FIRST || Drive > FSLINUX_DRIVE_NAME_LAST) {
		DBG_ERR("Invalid Drive %c\r\n", Drive);
		return NULL;
	}

	pCtrlObj = &gFsLinuxCtrlObj[Drive - FSLINUX_DRIVE_NAME_FIRST];

	if (0 == pCtrlObj->SemID) {
		DBG_ERR("Please call FsLinux_InstallID() first\r\n");
		return NULL;
	}

	return pCtrlObj;
}