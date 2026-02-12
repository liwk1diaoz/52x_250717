#ifndef _GXSTRGINT_H
#define _GXSTRGINT_H

#include "GxStrg.h"
#include "FileSysTsk.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxStrg
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

//e.g. DevID 0 => Drive 'A', DevID 1 => Drive 'B', etc.
#define GXSTRG_ID2DRV(DevID) ((UINT32)DevID + 'A')

#define GXSTRG_DRVNAME_FIRST    'A'
#define GXSTRG_DRVNAME_LAST     (GXSTRG_DRVNAME_FIRST + GXSTRG_STRG_NUM - 1)

typedef struct _GXSTRG_LINUX_STATUS {
	BOOL    IsInserted;
	BOOL    IsReadOnly;
	BOOL    IsFormatted;
} GXSTRG_LINUX_STATUS;

#define DET_CARD_REMOVED            0
#define DET_CARD_INSERTED           1
#define DET_CARD_UNKNOWN            0xFFFFFFFF

extern FILE_TSK_INIT_PARAM g_FSInitParam[];

// File system init callback function pointer
extern GX_CALLBACK_PTR g_fpStrgCB;

// Current storage object pointer (pointer from struct DX_OBJECT)
extern DX_HANDLE g_pCurStrgDXH[GXSTRG_STRG_NUM];

// Current storage object pointer (pointer from struct STORAGE_OBJ)
extern DX_HANDLE g_pCurStrgOBJ[GXSTRG_STRG_NUM];

// File System Type
extern FST_FS_TYPE g_FsType;

extern UINT32 g_FsOpsHdl;

// Linux storage status
extern GXSTRG_LINUX_STATUS  g_LnxStrgStatus[GXSTRG_STRG_NUM];
extern INT32 GxStrgLnx_Det(UINT32 DevId);

//internal functions
extern BOOL GxStrg_IsValidDevID(UINT32 DevID);
extern void GxStrg_SendMountStatus(UINT32 DevId, UINT32 MsgId);
extern void GxStrg_SetMountStatus(UINT32 DevId, UINT32 MsgId, BOOL bSet);
extern void GxStrg_InitCB0(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP);
extern void GxStrg_InitCB1(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP);
extern void GxStrg_InitCB2(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP);
extern void GxStrg_InitCB3(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP);
extern void GxStrg_InitCB4(UINT32 uiMsgID, UINT32 uiP1, UINT32 uiP2, UINT32 uiP);

#endif
