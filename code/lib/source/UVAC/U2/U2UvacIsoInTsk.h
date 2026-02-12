
#ifndef _U2UVACISOINTSK_H_
#define _U2UVACISOINTSK_H_

#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/util.h"
#include "kwrap/perf.h"
#include <kwrap/cpu.h>
#include <kwrap/verinfo.h>

#include "../uvac_compat.h"
#include "UVAC.h"
#include "U2UvacDbg.h"


#define USB_MAX_TXF_SIZE_ONE_PACKET     (UVC_ISOIN_HS_PACKET_SIZE * gU2UvcIsoInBandWidth)   //0x200       //0x1FFFF   //
#define USB_MAX_TXF_SIZE_LIMIT          0x700000
#define UVAC_TXF_MAX_QUECNT             60
#define ISF_LATENCY_DEBUG               0
#define ISF_AUDIO_LATENCY_DEBUG         0

typedef enum _UVAC_EP_INTR_ {
	UVAC_EP_INTR_V1,
	UVAC_EP_INTR_V2,
	UVAC_EP_INTR_MAX
} UVAC_EP_INTR;

typedef enum _UVAC_TXF_QUE_ {
	UVAC_TXF_QUE_V1 = 0,
	UVAC_TXF_QUE_V2,
	UVAC_TXF_QUE_A1,
	UVAC_TXF_QUE_A2,
	UVAC_TXF_QUE_IMG,
	UVAC_TXF_QUE_MAX
} UVAC_TXF_QUE;

typedef enum _UVAC_RXF_QUE_ {
	UVAC_RXF_QUE_A1 = 0,
	UVAC_RXF_QUE_MAX
} UVAC_RXF_QUE;

typedef struct _UVAC_TXF_INFO_ {
	UINT32 sAddr;
	UINT32 size;
	UINT32 oriAddr;
	UINT16 txfCnt;
	UINT8  usbEP; //USB_EP
	UINT8  streamType;
	UINT64 timestamp;
} UVAC_TXF_INFO;

typedef enum _UVAC_TXF_STREAM_TYPE_ {
	UVAC_TXF_STREAM_VID = 0,
	UVAC_TXF_STREAM_AUD,
	UVAC_TXF_STREAM_STILLIMG, //2
	UVAC_TXF_STREAM_TYPE_MAX
} UVAC_TXF_STREAM_TYPE;

typedef enum _UVC_ISOIN_TXF_ACT_ {
	UVC_ISOIN_TXF_ACT_STOP = 0,
	UVC_ISOIN_TXF_ACT_START,
	UVC_ISOIN_TXF_ACT_TXF
} UVC_ISOIN_TXF_ACT;

typedef enum _UVC_ISOIN_TXF_STATE_ {
	UVC_ISOIN_TXF_STATE_STOP = 0,
	UVC_ISOIN_TXF_STATE_START,
	UVC_ISOIN_TXF_STATE_TXFING,
} UVC_ISOIN_TXF_STATE;

extern void U2UVAC_IsoInTxfStateMachine(UVC_ISOIN_TXF_ACT txfAct);
//extern THREAD_DECLARE(U2UVAC_IsoInTsk, arglist);
extern THREAD_DECLARE(U2UVAC_IsoInVdo1Tsk, arglist);
extern THREAD_DECLARE(U2UVAC_IsoInVdo2Tsk, arglist);
extern THREAD_DECLARE(U2UVAC_IsoInAud1Tsk, arglist);
extern THREAD_DECLARE(U2UVAC_IsoInAud2Tsk, arglist);
extern void U2UVAC_ResetTxfPara(void);
extern ER U2UVAC_AddIsoInTxfInfo(UVAC_TXF_INFO *pTxfInfo, UVAC_TXF_QUE queIdx);
extern UINT32 U2UVAC_RemoveTxfInfo(UVAC_TXF_QUE queIdx);
extern void U2UVAC_DbgDmp_TxfInfoByQue(UINT32 queIdx);
extern void U2UVAC_DbgDmp_ChkIsoInQueEmpty(UINT32 queIdx);
extern void U2UVAC_DbgDmp_TxfEPInfo(void);
extern void U2UVAC_DbgDmp_ChkTxfQueAlmostFull(UINT32 queIdx, UINT32 level, BOOL dmpFirst, BOOL dmpQue);
extern void U2UVAC_DbgDmp_TimestampNumReset(void);

#endif
