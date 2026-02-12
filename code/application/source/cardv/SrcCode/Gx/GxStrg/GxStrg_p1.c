#include "GxStrg.h"
#include "GxStrgInt.h"
#include "FileSysTsk.h"

// Strg callback function pointer
GX_CALLBACK_PTR g_fpStrgCB = NULL;

// Current storage object pointer (pointer from struct DX_OBJECT)
DX_HANDLE       g_pCurStrgDXH[GXSTRG_STRG_NUM] = {0};

// Current storage object pointer (pointer from struct STORAGE_OBJ)
DX_HANDLE       g_pCurStrgOBJ[GXSTRG_STRG_NUM] = {0};

FST_FS_TYPE     g_FsType = FST_FS_TYPE_UITRON;
UINT32          g_FsOpsHdl = 0;

FILE_TSK_INIT_PARAM g_FSInitParam[] = {
	{{0, 0, FALSE, {0}, 0, 0}, (FileSys_CB)GxStrg_InitCB0},
	{{0, 0, FALSE, {0}, 0, 0}, (FileSys_CB)GxStrg_InitCB1},
	{{0, 0, FALSE, {0}, 0, 0}, (FileSys_CB)GxStrg_InitCB2},
	{{0, 0, FALSE, {0}, 0, 0}, (FileSys_CB)GxStrg_InitCB3},
	{{0, 0, FALSE, {0}, 0, 0}, (FileSys_CB)GxStrg_InitCB4},
};

STATIC_ASSERT(GXSTRG_STRG_NUM == sizeof(g_FSInitParam) / sizeof(FILE_TSK_INIT_PARAM));

BOOL GxStrg_IsValidDevID(UINT32 DevID)
{
	if (DevID < GXSTRG_STRG_NUM) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/*
  FILE_CFG_DEVICE1:
  Initialize card storage object
  @param pStrgObj Pointer to Card's STRG_TAB type object.

  FILE_CFG_DEVICE2:
  Initialize nand storage object
  @param pStrgObj Pointer to Nand's STRG_TAB type object.
*/
void GxStrg_SetConfigEx(UINT32 DevID, UINT32 cfg, UINT32 value)
{
	if (FALSE == GxStrg_IsValidDevID(DevID)) {
		return;
	}

	switch (cfg) {
	case FILE_CFG_BUF:
		if (value) {
			MEM_RANGE *pBuf = (MEM_RANGE *)value;
			g_FSInitParam[DevID].FSParam.WorkBuf = pBuf->addr;
			g_FSInitParam[DevID].FSParam.WorkBufSize = pBuf->size;
		}
		break;
	case FILE_CFG_SUPPORT_EXFAT:
		g_FSInitParam[DevID].FSParam.bSupportExFAT = ((value != 0) ? TRUE : FALSE);
		break;
	case FILE_CFG_FS_TYPE:
		g_FsOpsHdl = value;
		g_FsType = FileSys_GetType(g_FsOpsHdl);
		break;
	case FILE_CFG_MOUNT_PATH:
		strncpy(g_FSInitParam[DevID].FSParam.szMountPath, (char *)value, KFS_LONGNAME_PATH_MAX_LENG - 1);
		g_FSInitParam[DevID].FSParam.szMountPath[KFS_LONGNAME_PATH_MAX_LENG - 1] = '\0';
		break;
	case FILE_CFG_MAX_OPEN_FILE:
		g_FSInitParam[DevID].FSParam.MaxOpenedFileNum = value;
		break;
	case FILE_CFG_ALIGNED_SIZE:
		g_FSInitParam[DevID].FSParam.AlignedSize = value;
		break;
	case FILE_CFG_STRG_OBJECT:
		g_pCurStrgDXH[DevID] = (DX_HANDLE)value;
		break;
	default:
		DBG_ERR("Invalid cfg %d\r\n", cfg);
		break;
	}
}


void GxStrg_RegCB(GX_CALLBACK_PTR fpStrgCB)
{
	g_fpStrgCB = fpStrgCB;
}

