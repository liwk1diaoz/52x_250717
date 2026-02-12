#ifndef _LVIEWNVT_INT_H
#define _LVIEWNVT_INT_H
#include "kwrap/type.h"
#include "LviewNvt/LviewNvtAPI.h"

// LView flag define
#define FLG_LVIEWNVT_START            0x00000001
#define FLG_LVIEWNVT_STOP             0x00000002
#define FLG_LVIEWNVT_RDY              0x00000004
#define FLG_LVIEWNVT_ALL              (FLG_LVIEWNVT_START|FLG_LVIEWNVT_STOP|FLG_LVIEWNVT_RDY)

#define FLG_LVIEWNVT_AUD_TC_HIT       0x00010000
#define FLG_LVIEWNVT_AUD_DMADONE      0x00020000
#define FLG_LVIEWNVT_AUD_ONE_SEC      0x00040000
#define FLG_LVIEWNVT_AUD_STOP         0x00080000
#define FLG_LVIEWNVT_AUD_IDLE         0x00100000


// LVIEWD flag define
#define FLG_LVIEWD_START              0x00000001
#define FLG_LVIEWD_STOP               0x00000002
#define FLG_LVIEWD_IPC_CMD            0x00000004

#define FLG_LVIEWD_STARTED            0x00000008
#define FLG_LVIEWD_STOPPED            0x00000010

#define FLG_LVIEWD_IDLE               0x00000080

#ifdef _NETWORK_ON_CPU2_
#define LVIEW_USE_IPC                 1
#else
#define LVIEW_USE_IPC                 0
#endif

#define LVIEW_DEBUG_LATENCY           0

typedef struct _LVIEWD_CTRL {
	UINT32                uiInitKey;   ///< indicate module is initail
	UINT32                FlagID;
	LVIEWNVT_DAEMON_INFO  Open;
	//CHAR                  startDaemonCmd[128];
	// IPC releated
	INT32                 msqid;
} LVIEWD_CTRL;


extern void       LviewNvt_VideoTsk(void);
//extern void       Lviewd_MainTsk(void);
#endif //_LVIEWNVT_INT_H
