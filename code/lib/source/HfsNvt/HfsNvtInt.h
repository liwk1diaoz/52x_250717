#ifndef _HFSNVT_INT_H
#define _HFSNVT_INT_H
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "HfsNvt/HfsNvtAPI.h"


#define HFS_USE_IPC                 0


#define FLG_HFSNVT_START            0x00000001
#define FLG_HFSNVT_STOP             0x00000002
#define FLG_HFSNVT_SERVER_STARTED   0x00000004
#define FLG_HFSNVT_SERVER_STOPPED   0x00000008

#define FLG_HFSNVT_IDLE             0x00008000



typedef struct _HFSNVT_CTRL {
	UINT32          uiInitKey;   ///< indicate module is initail
	UINT32          FlagID;
	HFSNVT_OPEN     Open;
	INT32           msqid;
} HFSNVT_CTRL;

#endif //_HFSNVT_INT_H
