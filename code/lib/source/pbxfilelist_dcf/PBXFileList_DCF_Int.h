#ifndef __PBXFILELIST_DCFINT_H
#define __PBXFILELIST_DCFINT_H

#if defined(__KERNEL__)
#include "kwrap/type.h"
#include "kwrap/error_no.h"

#define Perf_Mark()
#define Perf_GetDuration()
#else
#include "kwrap/type.h"
#include "kwrap/error_no.h"

#define kent_tsk()
#define Perf_Mark()
#define Perf_GetDuration()

#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

ER PBXFList_DCF_Config(UINT32 ConfigID, UINT32 param1, UINT32 param2);
ER PBXFList_DCF_Init(void);
ER PBXFList_DCF_UnInit(void);
ER PBXFList_DCF_GetInfo(UINT32  InfoID, VOID *pparam1, VOID *pparam2);
ER PBXFList_DCF_SetInfo(UINT32  InfoID, UINT32 param1, UINT32 param2);
ER PBXFList_DCF_SeekIndex(INT32 offset, PBX_FLIST_SEEK_CMD seekCmd);
ER PBXFList_DCF_MakeFilePath(UINT32  fileType, CHAR *path);
ER PBXFList_DCF_AddFile(CHAR *path);
ER PBXFList_DCF_Refresh(void);
#endif