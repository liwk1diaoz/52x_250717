#ifndef __PBXFILELIST_FDBINT_H
#define __PBXFILELIST_FDBINT_H

#if defined(__KERNEL__)
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/stdio.h"
#include <string.h>

#define Perf_Mark()
#define Perf_GetDuration()
#else
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/stdio.h"
#include <string.h>

#define kent_tsk()
#define Perf_Mark()
#define Perf_GetDuration()

#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

ER PBXFList_FDB_Config(UINT32 ConfigID, UINT32 param1, UINT32 param2);
ER PBXFList_FDB_Init(void);
ER PBXFList_FDB_UnInit(void);
ER PBXFList_FDB_GetInfo(UINT32  InfoID, VOID *pparam1, VOID *pparam2);
ER PBXFList_FDB_SetInfo(UINT32  InfoID, UINT32 param1, UINT32 param2);
ER PBXFList_FDB_SeekIndex(INT32 offset, PBX_FLIST_SEEK_CMD seekCmd);
ER PBXFList_FDB_MakeFilePath(UINT32  fileType, CHAR *path);
ER PBXFList_FDB_AddFile(CHAR *path);
ER PBXFList_FDB_Refresh(void);
#endif
