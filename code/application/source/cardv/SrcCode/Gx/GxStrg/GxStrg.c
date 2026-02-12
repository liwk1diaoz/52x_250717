#include "GxStrg.h"
#include "FileSysTsk.h"
#include "GxStrgInt.h"

GXSTRG_LINUX_STATUS g_LnxStrgStatus[GXSTRG_STRG_NUM] = {0};

/*
    Initialize file system task

    Initialize file system task according to current active storage.

    @param[in] uiStorageType    Current active storage (Refer to PrimaryTsk.h)
    @return                     Init status
        - @b TRUE:              Init process OK
        - @b FALSE:             No storage card object or init file system fail
*/
BOOL GxStrg_OpenDevice(UINT32 DevID, DX_HANDLE NewStrgDXH)
{
	DBG_DUMP("\r\nfilesys_init b\r\n");

	if (FALSE == GxStrg_IsValidDevID(DevID)) {
		DBG_ERR("Invalid DevID(%d)\r\n", DevID);
		return FALSE;
	}

	if (FST_FS_TYPE_LINUX == g_FsType) {
		if (0 == g_FSInitParam[DevID].FSParam.szMountPath[0]) {
			DBG_ERR("Please set FILE_CFG_MOUNT_PATH for Linux\r\n");
			return FALSE;
		}
	} else if (FST_FS_TYPE_UITRON == g_FsType) {
		if (NULL == NewStrgDXH) {
			DBG_ERR("NewStrgDXH is NULL\r\n");
			return FALSE;
		}
	} else {
		DBG_ERR("Invalid FsType %d\r\n", g_FsType);
		return FALSE;
	}
	if (FST_STA_OK != FileSys_Init(g_FsOpsHdl)) {
		DBG_ERR("FileSys_Init 0x%X failed\r\n", g_FsOpsHdl);
		return FALSE;
	}

	if (g_FSInitParam[DevID].FSParam.WorkBuf == 0) {
		DBG_WRN("WorkBuf is 0\r\n");
		return FALSE;
	}

	if (FileSys_OpenEx(GXSTRG_ID2DRV(DevID), (FS_HANDLE)NewStrgDXH, &g_FSInitParam[DevID]) != FST_STA_OK) {
		DBG_WRN("FileSys_Open(DevID %d)\r\n", DevID);
		return FALSE;
	}

	g_pCurStrgOBJ[DevID] = NewStrgDXH;

	DBG_IND("Storage - 0x%.8X\r\n", g_pCurStrgOBJ[DevID]);

	FileSys_WaitFinishEx(GXSTRG_ID2DRV(DevID));
	DBG_DUMP("\r\nfilesys_init e\r\n");

	return TRUE;
}

BOOL GxStrg_CloseDevice(UINT32 DevID)
{
	if (FALSE == GxStrg_IsValidDevID(DevID)) {
		DBG_WRN("Invalid DevID(%d)\r\n", DevID);
		return FALSE;
	}

	if (g_pCurStrgOBJ[DevID] == 0) {
		return TRUE;//not open
	}

	FileSys_WaitFinishEx(GXSTRG_ID2DRV(DevID));
	FileSys_CloseEx(GXSTRG_ID2DRV(DevID), FST_TIME_INFINITE);
	if (g_fpStrgCB) {
		g_fpStrgCB(STRG_CB_UNMOUNT_FINISH, DevID, 0);
	}

	g_pCurStrgOBJ[DevID] = 0;

	return TRUE;
}

/*
    File system callback function

    File system callback function for the initialize status.

    @param[in] uiMsgID      File systam callback message ID
    @param[in] uiP1         File systam callback parameter 1
    @param[in] uiP2         File systam callback parameter 2
    @param[in] uiP3         File systam callback parameter 3
    @return void
*/
void GxStrg_SendMountStatus(UINT32 DevId, UINT32 MsgId)
{
	UINT32 MountStatus;

	if (FST_FS_TYPE_LINUX == g_FsType) {
		if (g_LnxStrgStatus[DevId].IsFormatted) {
			MountStatus = FST_STA_OK;
		} else {
			MountStatus = FST_STA_DISK_UNFORMAT;
		}
	} else {
		MountStatus = MsgId;
	}

	if (g_fpStrgCB) {
		g_fpStrgCB(STRG_CB_MOUNT_FINISH, DevId, MountStatus);
	}

	// Initialize OK, set to be current storage
	DBG_IND("DevId %d MountStatus %d\r\n", DevId, MountStatus);
}

void GxStrg_SetMountStatus(UINT32 DevId, UINT32 MsgId, BOOL bSet)
{
	if (!bSet) {
		return;
	}
	if (FST_FS_TYPE_LINUX == g_FsType) {
		g_LnxStrgStatus[0].IsFormatted = (MsgId == FST_STA_OK);
	}
}

void GxStrg_InitCB0(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP)
{
	GxStrg_SetMountStatus(0, uiMsgID, uiP1);
	GxStrg_SendMountStatus(0, uiMsgID);
}

void GxStrg_InitCB1(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP)
{
	GxStrg_SetMountStatus(1, uiMsgID, uiP1);
	GxStrg_SendMountStatus(1, uiMsgID);
}

void GxStrg_InitCB2(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP)
{
	GxStrg_SetMountStatus(2, uiMsgID, uiP1);
	GxStrg_SendMountStatus(2, uiMsgID);
}

void GxStrg_InitCB3(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP)
{
	GxStrg_SetMountStatus(3, uiMsgID, uiP1);
	GxStrg_SendMountStatus(3, uiMsgID);
}

void GxStrg_InitCB4(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP)
{
	GxStrg_SetMountStatus(4, uiMsgID, uiP1);
	GxStrg_SendMountStatus(4, uiMsgID);
}

/*
  Get storage type

  Get storage object of specified storage type

  @param StorageID Storage type
    PRIMARY_STGTYPE_NAND    (Nand flash)
    PRIMARY_STGTYPE_CARD    (SD/CF/MS/SM card)
  @return PSTRG_TAB Pointer to storage object, NULL if not a supported
*/
DX_HANDLE GxStrg_GetDevice(UINT32 DevID)
{
	if (FALSE == GxStrg_IsValidDevID(DevID)) {
		DBG_ERR("Invalid DevID(%d)\r\n", DevID);
		return NULL;
	}

	return g_pCurStrgOBJ[DevID];
}

/*
  Set storage mount status

  This is onl for fsck (disk checking) function.
 */
void GxStrg_SetStrgMountStatus(UINT32 DevID, BOOL bMountOK)
{
    if (bMountOK) {
        g_LnxStrgStatus[DevID].IsFormatted = TRUE;
    } else {
        g_LnxStrgStatus[DevID].IsFormatted = FALSE;
    }
}