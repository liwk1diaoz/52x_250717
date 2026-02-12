/*
    Media Audio Recorder Task.

    This file is the task of media audio recorder.

    @file       nmediarec_audenc_tsk.c
    @date       2018/09/05

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/stdio.h"
//#include "kwrap/sxcmd.h"
#include "nmediarec_audenc_tsk.h"
#include "nmediarec_api.h"
#include "audio_common/include/Audio.h"
#include "kflow_audioenc/isf_audenc.h"
#include "sxcmd_wrapper.h"

// perf
#define Perf_GetCurrent(args...)     0

#define PERF_CHK DISABLE

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          nmediarec_audenc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int nmediarec_audenc_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////

/**
    @addtogroup mIAPPNMEDIAREC
*/
//@{
#define PRI_NMR_AUDENC                          8 //7 to 8 2013/12/4 mp4+multi vid buf full
#define STKSIZE_NMR_AUDENC                      16384 // for AAC encoding
#define FLG_NMR_AUDENC_IDLE                     FLGPTN_BIT(0) //0x00000001
#define FLG_NMR_AUDENC_ENCODE                   FLGPTN_BIT(1) //0x00000002
#define FLG_NMR_AUDENC_STOP                     FLGPTN_BIT(2) //0x00000004
#define FLG_NMR_AUDENC_STOP_DONE			    FLGPTN_BIT(3) //0x00000008

#define NMR_AUDENC_MAX_PATH                     16
#define NMR_AUDENC_MAX_CLUSIZE                  0x40000
#define NMR_AUDENC_BR_16000                     16000
#define NMR_AUDENC_BR_32000                     32000
#define NMR_AUDENC_BR_48000                     48000
#define NMR_AUDENC_BR_64000                     64000
#define NMR_AUDENC_BR_80000                     80000
#define NMR_AUDENC_BR_96000                     96000
#define NMR_AUDENC_BR_112000                    112000
#define NMR_AUDENC_BR_128000                    128000
#define NMR_AUDENC_BR_144000                    144000
#define NMR_AUDENC_BR_160000                    160000
#define NMR_AUDENC_BR_192000                    192000

#define NMR_AUDENC_MAX_FIXED_SAMPLE				2048

UINT32 g_audenc_path_max_count = 0;

#if NMR_AUDENC_DYNAMIC_CONTEXT
NMR_AUDENC_OBJ                                  *gNMRAudEncObj = NULL;
#else
NMR_AUDENC_OBJ                                  gNMRAudEncObj[NMR_AUDENC_MAX_PATH] = {0};
#endif

NMR_AUDENC_JOBQ                                 gNMRAudEncJobQ = {0};

THREAD_HANDLE                                   NMR_AUDENC_TSK_ID = 0;
ID                                              FLG_ID_NMR_AUDENC = 0;
SEM_HANDLE                                      NMR_AUDENC_SEM_ID[NMR_AUDENC_MAX_PATH] = {0};
SEM_HANDLE                                      NMR_AUDENC_COMMON_SEM_ID = {0};
UINT32                                          gNMRAudEncPathCount = 0;
UINT8                                           gNMRAudEncOpened = FALSE;

#if PERF_CHK == ENABLE
#include "kwrap/perf.h"
static VOS_TICK enc_time = 0;
static UINT32 enc_count = 0;
#endif

UINT32 _nmr_audenc_query_context_size(void)
{
#if NMR_AUDENC_DYNAMIC_CONTEXT
	return (PATH_MAX_COUNT * sizeof(NMR_AUDENC_OBJ));
#else
	return 0;
#endif
}

void _nmr_audenc_assign_context(UINT32 addr)
{
#if NMR_AUDENC_DYNAMIC_CONTEXT
	gNMRAudEncObj = (NMR_AUDENC_OBJ *)addr;
	memset(gNMRAudEncObj, 0, _nmr_audenc_query_context_size());
#endif
	return;
}

void _nmr_audenc_free_context(void)
{
#if NMR_AUDENC_DYNAMIC_CONTEXT
	gNMRAudEncObj = NULL;
#endif
	return;
}

BOOL _nmr_audenc_is_init(void)
{
#if NMR_AUDENC_DYNAMIC_CONTEXT
	return (gNMRAudEncObj)? TRUE:FALSE;
#else
	return TRUE;
#endif
}

static ER _NMR_AudEnc_CreateEncObj(UINT32 pathID)
{
	UINT32 tempAddr;

	memset(&(gNMRAudEncObj[pathID].Encoder), 0, sizeof(MP_AUDENC_ENCODER));

	if (gNMRAudEncObj[pathID].pEncoder == 0) {
		return E_SYS;
	}

	memcpy((void *) &(gNMRAudEncObj[pathID].Encoder), (void *)gNMRAudEncObj[pathID].pEncoder, sizeof(MP_AUDENC_ENCODER));
	(gNMRAudEncObj[pathID].Encoder.GetInfo)(MP_AUDENC_GETINFO_PARAM, (UINT32 *)&tempAddr, 0, 0);
	gNMRAudEncObj[pathID].pEncParam = (MP_AUDENC_PARAM *)tempAddr;
	return E_OK;
}

static ER _NMR_AudEnc_Close(void)
{
	FLGPTN uiFlag;

	if (gNMRAudEncOpened == FALSE) {
		return E_SYS;
	}

	wai_flg(&uiFlag, FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_IDLE, TWF_ORW);

#if 1
	set_flg(FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_STOP);
	wai_flg(&uiFlag, FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_STOP_DONE, TWF_ORW | TWF_CLR);
#endif

	#if 0
	THREAD_DESTROY(NMR_AUDENC_TSK_ID); //ter_tsk(NMR_AUDENC_TSK_ID);
	#endif

#if MP_AUDENC_SHOW_MSG
	DBG_DUMP("[AUDENC] wait close finished\r\n");
#endif

	gNMRAudEncOpened = FALSE;
	return E_OK;
}

static void _NMR_AudEnc_InitJobQ(void)
{
	memset(&gNMRAudEncJobQ, 0, sizeof(NMR_AUDENC_JOBQ));
}

static ER _NMR_AudEnc_PutJob(UINT32 pathID)
{
	SEM_WAIT(NMR_AUDENC_COMMON_SEM_ID);

	if ((gNMRAudEncJobQ.Front == gNMRAudEncJobQ.Rear) && (gNMRAudEncJobQ.bFull == TRUE)) {
		static UINT32 sCount;
		SEM_SIGNAL(NMR_AUDENC_COMMON_SEM_ID);
		if (sCount % 128 == 0) {
			DBG_WRN("[AUDENC] Job Queue is Full, Job Id = %d\r\n", pathID);
		}
		sCount++;
		return E_SYS;
	} else {
		gNMRAudEncJobQ.Queue[gNMRAudEncJobQ.Rear] = pathID;
		gNMRAudEncJobQ.Rear = (gNMRAudEncJobQ.Rear + 1) % NMR_AUDENC_JOBQ_MAX;
		if (gNMRAudEncJobQ.Front == gNMRAudEncJobQ.Rear) { // Check Queue full
			gNMRAudEncJobQ.bFull = TRUE;
		}
		SEM_SIGNAL(NMR_AUDENC_COMMON_SEM_ID);
		return E_OK;
	}
}

static ER _NMR_AudEnc_GetJob(UINT32 *pPathID)
{
	SEM_WAIT(NMR_AUDENC_COMMON_SEM_ID);
	if ((gNMRAudEncJobQ.Front == gNMRAudEncJobQ.Rear) && (gNMRAudEncJobQ.bFull == FALSE)) {
		SEM_SIGNAL(NMR_AUDENC_COMMON_SEM_ID);
		DBG_ERR("[AUDENC] Job Queue is Empty!\r\n");
		return E_SYS;
	} else {
		*pPathID = gNMRAudEncJobQ.Queue[gNMRAudEncJobQ.Front];
		gNMRAudEncJobQ.Front = (gNMRAudEncJobQ.Front + 1) % NMR_AUDENC_JOBQ_MAX;
		if (gNMRAudEncJobQ.Front == gNMRAudEncJobQ.Rear) { // Check Queue full
			gNMRAudEncJobQ.bFull = FALSE;
		}
		SEM_SIGNAL(NMR_AUDENC_COMMON_SEM_ID);
		return E_OK;
	}
}

static UINT32 _NMR_AudEnc_GetJobCount(void)
{
	UINT32 front, rear, full, sq = 0;
	SEM_WAIT(NMR_AUDENC_COMMON_SEM_ID);
	front = gNMRAudEncJobQ.Front;
	rear = gNMRAudEncJobQ.Rear;
	full = gNMRAudEncJobQ.bFull;
	SEM_SIGNAL(NMR_AUDENC_COMMON_SEM_ID);
	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_AUDENC_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_AUDENC_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}

static UINT32 _NMR_AudEnc_GetBSCount(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	PNMR_AUDENC_BSQ pObj;

	SEM_WAIT(NMR_AUDENC_SEM_ID[pathID]);
	pObj = &(gNMRAudEncObj[pathID].bsQueue);

	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_AUDENC_BSQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_AUDENC_BSQ_MAX;
	} else {
		sq = 0;
	}

	return sq;
}

static BOOL _NMR_AudEnc_CheckBS(UINT32 pathID, UINT32 *uiAddr, UINT32 *uiSize)
{
	UINT32 front, rear, full;
	PNMR_AUDENC_BSQ pObj;
	BOOL bResult = FALSE;
	static UINT32 uiOriAddr = 0, uiOriSize = 0;

	if (gNMRAudEncObj[pathID].uiMsgLevel & NMR_AUDENC_MSG_RING_BUF) {
		uiOriAddr = *uiAddr;
		uiOriSize = *uiSize;
	}

	SEM_WAIT(NMR_AUDENC_SEM_ID[pathID]);
	pObj = &(gNMRAudEncObj[pathID].bsQueue);
	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);

	if (front != rear) {
		if (rear == 0) {
			rear = NMR_AUDENC_BSQ_MAX - 1;
		} else {
			rear = rear - 1;
		}

		if (pObj->Queue[front].Addr <= pObj->Queue[rear].Addr) { //check free space of left side or right side
			if (*uiAddr + *uiSize <= pObj->Queue[front].Addr) {
				*uiSize = pObj->Queue[front].Addr - *uiAddr;
				bResult = TRUE;
			} else if ((*uiAddr >= pObj->Queue[rear].Addr + pObj->Queue[rear].Size) && (*uiAddr + *uiSize <= gNMRAudEncObj[pathID].memInfo.uiBSEnd)) {
				*uiSize = gNMRAudEncObj[pathID].memInfo.uiBSEnd - *uiAddr;
				bResult = TRUE;
			} else if (pObj->Queue[front].Addr - gNMRAudEncObj[pathID].memInfo.uiBSStart >= *uiSize) {
				*uiAddr = gNMRAudEncObj[pathID].memInfo.uiBSStart;
				*uiSize = pObj->Queue[front].Addr - gNMRAudEncObj[pathID].memInfo.uiBSStart;
				bResult = TRUE;
			}
		} else if (*uiAddr >= pObj->Queue[rear].Addr + pObj->Queue[rear].Size && *uiAddr + *uiSize <= pObj->Queue[front].Addr) {  //check middle free space
			*uiSize = pObj->Queue[front].Addr - *uiAddr;
			bResult = TRUE;
		}
	} else if (front == rear && full == TRUE) {
		bResult = FALSE;
	} else {
		if (*uiAddr + *uiSize <= gNMRAudEncObj[pathID].memInfo.uiBSEnd) {
			*uiSize = gNMRAudEncObj[pathID].memInfo.uiBSEnd - *uiAddr;
		} else {
			*uiAddr = gNMRAudEncObj[pathID].memInfo.uiBSStart;
			*uiSize = gNMRAudEncObj[pathID].memInfo.uiBSEnd - gNMRAudEncObj[pathID].memInfo.uiBSStart;
		}
		bResult = TRUE; //no lock bs
	}

	if (gNMRAudEncObj[pathID].uiMsgLevel & NMR_AUDENC_MSG_RING_BUF) {
		if (bResult == FALSE) {
			DBG_ERR("[AUDENC][%d] check bs err, assigned enc buf (0x%08x, %d), ori enc buf (0x%08x, %d), front item = (%d, 0x%08x, %d), rear item = (%d, 0x%08x, %d)\r\n",
				pathID, *uiAddr, *uiSize, uiOriAddr, uiOriSize, pObj->Front, pObj->Queue[front].Addr, pObj->Queue[front].Size, pObj->Rear, pObj->Queue[rear].Addr, pObj->Queue[rear].Size);
		} else {
			DBG_DUMP("[AUDENC][%d] check bs ok, assigned enc buf (0x%08x, %d), ori enc buf (0x%08x, %d), front item = (%d, 0x%08x, %d), rear item = (%d, 0x%08x, %d)\r\n",
				pathID, *uiAddr, *uiSize, uiOriAddr, uiOriSize, pObj->Front, pObj->Queue[front].Addr, pObj->Queue[front].Size, pObj->Rear, pObj->Queue[rear].Addr, pObj->Queue[rear].Size);
		}
	}

	return bResult;
}

static BOOL _NMR_AudEnc_LockBS(UINT32 pathID, UINT32 uiAddr, UINT32 uiSize)
{
	PNMR_AUDENC_BSQ pObj;

	if (uiSize == 0) {
		DBG_ERR("[AUDENC][%d] BS Lock Size is 0\r\n", pathID);
		return FALSE;
	}

	SEM_WAIT(NMR_AUDENC_SEM_ID[pathID]);
	pObj = &(gNMRAudEncObj[pathID].bsQueue);
	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		DBG_ERR("[AUDENC][%d] BS Lock Queue is Full!\r\n", pathID);
		SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
		return FALSE;
	} else {
		pObj->Queue[pObj->Rear].Addr = uiAddr;
		pObj->Queue[pObj->Rear].Size = uiSize;

		pObj->Queue[pObj->Rear].Rel  = FALSE;

		pObj->Rear = (pObj->Rear + 1) % NMR_AUDENC_BSQ_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
		return TRUE;
	}
}

static BOOL _NMR_AudEnc_UnlockBS(UINT32 pathID, UINT32 uiAddr)
{
	PNMR_AUDENC_BSQ   pObj;

	//DBG_DUMP("[AUDENC][%d] BS Unlock addr = 0x%08x\r\n", pathID, uiAddr);
	if (uiAddr == 0) {
		return FALSE;
	}

	SEM_WAIT(NMR_AUDENC_SEM_ID[pathID]);
	pObj = &(gNMRAudEncObj[pathID].bsQueue);
	//DBG_DUMP("[AUDENC][%d] UnlockBS front = %d, rear = %d,  full = %d\r\n",
	//  pathID, pObj->Front, pObj->Rear, (UINT32)pObj->bFull);
	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
		DBG_ERR("[AUDENC][%d] BS Lock Queue is Empty!\r\n", pathID);
		return FALSE;
	} else {
		if (uiAddr == pObj->Queue[pObj->Front].Addr) {

			pObj->Front = (pObj->Front + 1) % NMR_AUDENC_BSQ_MAX;

			//check if next queue is released before
			while (pObj->Queue[pObj->Front].Rel == TRUE) {
				pObj->Queue[pObj->Front].Rel = FALSE;
				pObj->Front = (pObj->Front + 1) % NMR_AUDENC_BSQ_MAX;
			}

			if (pObj->Front == pObj->Rear) { // Check Queue full
				pObj->bFull = FALSE;
			}
		} else {
			UINT32 index = pObj->Front;

			while (index != pObj->Rear) {
				if (uiAddr == pObj->Queue[index].Addr) {
					pObj->Queue[index].Rel = TRUE;
					break;
				}

				index = (index + 1) % NMR_AUDENC_BSQ_MAX;
			}

			SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
			DBG_ERR("[AUDENC][%d] BS Unlock %d not in head, wait addr = 0x%08x size = %d front = %d rear = %d\r\n", pathID, index, pObj->Queue[pObj->Front].Addr, pObj->Queue[pObj->Front].Size, pObj->Front, pObj->Rear);
			return FALSE;
		}

		SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
		return TRUE;
	}
}

static UINT32 _NMR_AudEnc_HowManyinQ(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	NMR_AUDENC_AUDQ *pObj;

	SEM_WAIT(NMR_AUDENC_SEM_ID[pathID]);
	pObj = &(gNMRAudEncObj[pathID].rawQueue);
	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = NMR_AUDENC_RAWQ_MAX - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = NMR_AUDENC_RAWQ_MAX;
	} else {
		sq = 0;
	}

	return sq;
}

static ER _NMR_AudEnc_Init(UINT32 pathID)
{
	ER ret = E_SYS;

	if (gNMRAudEncObj[pathID].Encoder.Initialize) {
		ret = (gNMRAudEncObj[pathID].Encoder.Initialize)();
		if (ret != E_OK) {
			DBG_ERR("[%d] Media Plug-in init fail\n\r", pathID);
		}
	}

	return ret;
}

static void _NMR_AudEnc_InitQ(UINT32 pathID)
{
	memset(&(gNMRAudEncObj[pathID].rawQueue), 0, sizeof(NMR_AUDENC_AUDQ));
	memset(&(gNMRAudEncObj[pathID].bsQueue), 0, sizeof(NMR_AUDENC_BSQ));
}

static ER _NMR_AudEnc_PutRaw(UINT32 pathID, NMI_AUDENC_RAW_INFO *pRawInfo)
{
	NMR_AUDENC_AUDQ *pObj;

	SEM_WAIT(NMR_AUDENC_SEM_ID[pathID]);

	pObj = &(gNMRAudEncObj[pathID].rawQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
		DBG_ERR("[AUDENC][%d] Add Raw Queue is Full!\r\n", pathID);
		return E_SYS;
	} else {
		pObj->Queue[pObj->Rear].Addr = pRawInfo->Addr;
		pObj->Queue[pObj->Rear].Size = pRawInfo->Size;
		pObj->Queue[pObj->Rear].BufID = pRawInfo->BufID;
		pObj->Queue[pObj->Rear].TimeStamp = pRawInfo->TimeStamp;
		pObj->Rear = (pObj->Rear + 1) % NMR_AUDENC_RAWQ_MAX;
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
		return E_OK;
	}
}

static ER _NMR_AudEnc_GetRaw(UINT32 pathID, NMI_AUDENC_RAW_INFO *pRawInfo)
{
	NMR_AUDENC_AUDQ  *pObj;

	SEM_WAIT(NMR_AUDENC_SEM_ID[pathID]);
	pObj = &(gNMRAudEncObj[pathID].rawQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
		DBG_ERR("[AUDENC][%d] Get Raw Queue is Empty!\r\n", pathID);
		return E_SYS;
	} else {
		pRawInfo->Addr = pObj->Queue[pObj->Front].Addr;
		pRawInfo->Size = pObj->Queue[pObj->Front].Size;
		pRawInfo->BufID = pObj->Queue[pObj->Front].BufID;
		pRawInfo->TimeStamp = pObj->Queue[pObj->Front].TimeStamp;
		pObj->Front = (pObj->Front + 1) % NMR_AUDENC_RAWQ_MAX;
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}

		SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
		return E_OK;
	}
}

/*static UINT32 _NMR_AudEnc_GetQCount(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	NMR_AUDENC_AUDQ *pObj;

	SEM_WAIT(NMR_AUDENC_SEM_ID[pathID]);
	pObj = &(gNMRAudEncObj[pathID].rawQueue);

	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	SEM_SIGNAL(NMR_AUDENC_SEM_ID[pathID]);
	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_AUDENC_RAWQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_AUDENC_RAWQ_MAX;
	} else {
		sq = 0;
	}

	return sq;
}*/

static UINT32 _NMR_AudEnc_GetBufSize(UINT32 pathID)
{
	UINT32 value;
	value = gNMRAudEncObj[pathID].memInfo.uiChunkSize * gNMRAudEncObj[pathID].memInfo.uiAudBSBufMs/1000;
	//DBG_DUMP("^C[AUDENC][%d] Get alloc size, codec = %d, millisec = %d, chunksize = %d, enc buf = %d\r\n",
	//    pathID, gNMRAudEncObj[pathID].uiCodec, gNMRAudEncObj[pathID].memInfo.uiAudBSBufMs, gNMRAudEncObj[pathID].memInfo.uiChunkSize, value);
	return value;
}

static UINT32 _NMR_AudEnc_Encode(UINT32 pathID)
{
	UINT32 uiCount = 0, i;
	UINT32 uiEncodeRawAddr = 0, uiEncodeRawSize = 0;
	UINT32 uiOutBSAddr = 0, uiOutBSSize = 0, enctime = 0;
	ER     ER_Code;
	MP_AUDENC_PARAM       	*pAudParam;
	NMR_AUDENC_MEMINFO    	*pAudEncMem;
	NMI_AUDENC_RAW_INFO  	RawInfo = {0};
	NMI_AUDENC_BS_INFO      BsInfo = {0};
	char   *s = NULL;
	char   *p = NULL;
	#if PERF_CHK == ENABLE
	VOS_TICK proc_tick_begin, proc_tick_end;
	#endif


	pAudParam = gNMRAudEncObj[pathID].pEncParam;//for aud codec
	pAudEncMem = &(gNMRAudEncObj[pathID].memInfo);
	if (gNMRAudEncObj[pathID].Encoder.SetInfo == NULL) {
		DBG_ERR("[AUDENC][%d] Audio codec type = %d, set info is null\r\n", pathID, gNMRAudEncObj[pathID].uiCodec);
		return 0;
	}

	if (gNMRAudEncObj[pathID].bStart == FALSE) {
		return 0;
	}

	if (_NMR_AudEnc_GetRaw(pathID, &RawInfo) != E_OK) {
		DBG_ERR("[AUDENC][%d] get raw failed\r\n", pathID);
		return 0;
	}

	// G711 ulaw/alaw can encode with different frame sample which get Raw block size from Raw queue pushed frame size
	if (gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_ULAW || gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_ALAW) {
		pAudEncMem->uiRawBlocksize = RawInfo.Size;
	}

	if (RawInfo.Size >= pAudEncMem->uiRawBlocksize) { //raw enough to encode 1 audio BS
		// encode aac
		uiEncodeRawAddr = RawInfo.Addr;
		uiEncodeRawSize = 0;
		uiOutBSAddr     = pAudEncMem->uiPrevBSAddr;

		if (pAudEncMem->uiRawBlocksize > 0) {
			uiCount = RawInfo.Size / (pAudEncMem->uiRawBlocksize);
		}

		for (i = 0; i < uiCount; i++) {
			if (gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_AAC) {
				uiOutBSSize = gNMRAudEncObj[pathID].uiBitRate / 16;
			} else {
				uiOutBSSize = pAudEncMem->uiRawBlocksize;
			}

			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_ENTER); // [OUT] NEW_ENTER

			if (gNMRAudEncObj[pathID].uiMaxFrameQueue != 0 && _NMR_AudEnc_GetBSCount(pathID) >= gNMRAudEncObj[pathID].uiMaxFrameQueue) {
				static UINT32 sCount[NMR_AUDENC_MAX_PATH];
				BsInfo.PathID = pathID;
				BsInfo.BufID = RawInfo.BufID;
				BsInfo.Addr = 0;
				BsInfo.Size = 0;
				BsInfo.Occupied = 0;
				BsInfo.TimeStamp = RawInfo.TimeStamp;
				if (gNMRAudEncObj[pathID].CallBackFunc) {
					(gNMRAudEncObj[pathID].CallBackFunc)(NMI_AUDENC_EVENT_BS_CB, (UINT32) &BsInfo);
					(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
				}
				if (sCount[pathID] % 300 == 0) {
					DBG_WRN("[AUDENC][%d] queue frame count >= %d, time = %d us\r\n", pathID, gNMRAudEncObj[pathID].uiMaxFrameQueue, Perf_GetCurrent());
				}
				sCount[pathID]++;

				(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL); // [OUT] NEW_WRN

				return 0;
			}

			if (_NMR_AudEnc_CheckBS(pathID, &uiOutBSAddr, &uiOutBSSize) != TRUE) {
				static UINT32 sCount[NMR_AUDENC_MAX_PATH];
				BsInfo.PathID = pathID;
				BsInfo.BufID = RawInfo.BufID;
				BsInfo.Addr = 0;
				BsInfo.Size = 0;
				BsInfo.Occupied = 0;
				BsInfo.TimeStamp = RawInfo.TimeStamp;
				if (gNMRAudEncObj[pathID].CallBackFunc) {
					(gNMRAudEncObj[pathID].CallBackFunc)(NMI_AUDENC_EVENT_BS_CB, (UINT32) &BsInfo);
					(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
				}

				if (sCount[pathID] % 300 == 0) {
					DBG_WRN("[AUDENC][%d] size not enough, drop frame, time = %d us\r\n", pathID, Perf_GetCurrent());
				}
				sCount[pathID]++;

				(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL); // [OUT] NEW_WRN

				return 0;
			}

			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_OK); // [OUT] NEW_OK

			if (gNMRAudEncObj[pathID].uiMsgLevel & NMR_AUDENC_MSG_ENCTIME) {
				enctime = Perf_GetCurrent();
			}

			pAudParam->rawAddr = uiEncodeRawAddr;
			(gNMRAudEncObj[pathID].Encoder.SetInfo)(MP_AUDENC_SETINFO_RAWADDR, uiEncodeRawAddr, 0, 0);

			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pathID),  ISF_IN_PROBE_PUSH, ISF_OK);     // [IN] PUSH_OK
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pathID),  ISF_IN_PROBE_PROC, ISF_ENTER);  // [IN] PUSH_ENTER
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_ENTER); // [OUT] PROC_ENTER

			if (gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_ULAW || gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_ALAW) {
				pAudParam->usedRawSize = RawInfo.Size;
			}

			#if PERF_CHK == ENABLE
			if (enc_count < 200) {
				vos_perf_mark(&proc_tick_begin);
			}
			#endif

			// trigger encode and wait for ready
			ER_Code = (gNMRAudEncObj[pathID].Encoder.EncodeOne)(0, uiOutBSAddr, &uiOutBSSize, pAudParam);

			#if PERF_CHK == ENABLE
			if (enc_count < 200) {
				vos_perf_mark(&proc_tick_end);
				enc_time += vos_perf_duration(proc_tick_begin, proc_tick_end);
				enc_count++;

				if (enc_count == 200) {
					UINT32 sample_cnt = MP_AUDENC_AAC_RAW_BLOCK;
					UINT32 cpu_usage = ((enc_time/200) * (gNMRAudEncObj[pathID].uiSampleRate/1000) * 10) / sample_cnt;

					DBG_DUMP("[isf_audenc] Encode average process usage = %d,%d%%\r\n", cpu_usage/100, cpu_usage%100);
				}
			}
			#endif

			if (ER_Code == E_OK) {
				BsInfo.PathID = pathID;
				BsInfo.BufID = RawInfo.BufID;
				BsInfo.Addr = uiOutBSAddr;
				BsInfo.Size = pAudParam->thisSize;

				if (pAudParam->thisSize > uiOutBSSize) {
					DBG_ERR("outsize is too large. this = %d, out = %d\r\n", pAudParam->thisSize, uiOutBSSize);
				}

				if (i == uiCount - 1) {
					BsInfo.Occupied = 0;
				} else {
					BsInfo.Occupied = 1;
				}
				BsInfo.TimeStamp = RawInfo.TimeStamp;

				if (gNMRAudEncObj[pathID].uiADTSHeader && gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_AAC && gNMRAudEncObj[pathID].uiRecFormat == MEDIAREC_LIVEVIEW && gNMRAudEncObj[pathID].uiAacVer == 0) {
					p = (char *)BsInfo.Addr;
					*(p+1) = 0xF1;  //set ADTS mpeg version to 0
				}

				if (gNMRAudEncObj[pathID].uiMsgLevel & NMR_AUDENC_MSG_ENCTIME) {
					s = (char *)uiEncodeRawAddr;
					p = (char *)BsInfo.Addr;
					DBG_DUMP("[AUDENC][%d] Enc time = %d us, UsedRawSize = %d, Output = (0x%08x, %d), TimeStamp = %lld, src = 0x%02x%02x%02x%02x%02x%02x%02x%02x, data = 0x%02x%02x%02x%02x%02x%02x%02x%02x, time = %d us\r\n",
						pathID, Perf_GetCurrent()-enctime, pAudParam->usedRawSize, BsInfo.Addr, BsInfo.Size, BsInfo.TimeStamp,
						*s, *(s+1), *(s+2), *(s+3), *(s+4), *(s+5), *(s+6), *(s+7),
						*p, *(p+1), *(p+2), *(p+3), *(p+4), *(p+5), *(p+6), *(p+7), Perf_GetCurrent());
				}

				// Encode OK callback to unit
				(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pathID),  ISF_IN_PROBE_PROC,  ISF_OK); // [IN]  PROC_OK
				(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_OK); // [OUT] PROC_OK

				if (gNMRAudEncObj[pathID].CallBackFunc) {
					_NMR_AudEnc_LockBS(pathID, BsInfo.Addr, BsInfo.Size);
					(gNMRAudEncObj[pathID].CallBackFunc)(NMI_AUDENC_EVENT_BS_CB, (UINT32) &BsInfo); // the do_probe REL will do on StreamReadyCB, with (Occupy = 0 & Addr !=0)
				}
			} else {
				DBG_ERR("[AUDENC][%d] Encode Error\r\n", pathID);
				BsInfo.PathID = pathID;
				BsInfo.BufID = RawInfo.BufID;
				BsInfo.Addr = 0;
				BsInfo.Size = 0;
				BsInfo.Occupied = 0;
				BsInfo.TimeStamp = RawInfo.TimeStamp;
				if (gNMRAudEncObj[pathID].CallBackFunc) {
					(gNMRAudEncObj[pathID].CallBackFunc)(NMI_AUDENC_EVENT_BS_CB, (UINT32) &BsInfo);
					(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
					(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
				}
				return 0;
			}

			uiOutBSAddr += ALIGN_CEIL_4(pAudParam->thisSize) + ALIGN_CEIL_4(gNMRAudEncObj[pathID].uiBsReservedSize);

			uiEncodeRawAddr += pAudParam->usedRawSize;
			if (uiOutBSAddr > pAudEncMem->uiBSMax) {
				//rollback
				if (pAudEncMem->uiLastLeftSize != 0 && pAudEncMem->uiLastLeftAddr != 0) {
					memcpy((void *) pAudEncMem->uiBSStart, (void *) pAudEncMem->uiLastLeftAddr, pAudEncMem->uiLastLeftSize);
					uiOutBSAddr = pAudEncMem->uiBSStart + pAudEncMem->uiLastLeftSize;
				} else {
					uiOutBSAddr = pAudEncMem->uiBSStart;
				}
			}
			pAudEncMem->uiPrevBSAddr = uiOutBSAddr;//2015/03/03
			uiEncodeRawSize += pAudParam->usedRawSize;
		}
	} else {
		BsInfo.PathID = pathID;
		BsInfo.BufID = RawInfo.BufID;
		BsInfo.Addr = 0;
		BsInfo.Size = 0;
		BsInfo.Occupied = 0;
		BsInfo.TimeStamp = RawInfo.TimeStamp;
		if (gNMRAudEncObj[pathID].CallBackFunc) {
			(gNMRAudEncObj[pathID].CallBackFunc)(NMI_AUDENC_EVENT_BS_CB, (UINT32) &BsInfo);
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN

		}
		uiEncodeRawSize = 0;
	}

	return uiEncodeRawSize;
}

static void _NMR_AudEnc_ResetMem(UINT32 pathID)
{
	NMR_AUDENC_MEMINFO *pAudInfo;

	pAudInfo = &gNMRAudEncObj[pathID].memInfo;
	if(gNMRAudEncObj[pathID].uiBsReservedSize > 0) {
		pAudInfo->uiBSStart += ALIGN_CEIL_4(gNMRAudEncObj[pathID].uiBsReservedSize);
	}
	pAudInfo->uiPrevBSAddr = pAudInfo->uiBSStart;
	pAudInfo->uiLastLeftAddr = 0;
	pAudInfo->uiLastLeftSize = 0;
}

static void _NMR_AudEnc_SetEncParam(UINT32 pathID)
{
	UINT32 rawblocksize = 0;

	if (gNMRAudEncObj[pathID].Encoder.SetInfo == 0) {
		return ;
	}
	(gNMRAudEncObj[pathID].Encoder.SetInfo)(MP_AUDENC_SETINFO_SAMPLERATE, gNMRAudEncObj[pathID].uiSampleRate, 0, 0);
	(gNMRAudEncObj[pathID].Encoder.SetInfo)(MP_AUDENC_SETINFO_BITRATE, gNMRAudEncObj[pathID].uiBitRate, 0, 0);
	(gNMRAudEncObj[pathID].Encoder.SetInfo)(MP_AUDENC_SETINFO_CHANNEL, gNMRAudEncObj[pathID].uiChannels, 0, 0);
	(gNMRAudEncObj[pathID].Encoder.GetInfo)(MP_AUDENC_GETINFO_BLOCK, &rawblocksize, 0, 0);

	gNMRAudEncObj[pathID].memInfo.uiRawBlocksize = rawblocksize;

	//initialize
	if (gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_AAC) {
		// AAC stop frequency should be set before AAC encoder initialization
		(gNMRAudEncObj[pathID].Encoder.SetInfo)(MP_AUDENC_SETINFO_AAC_STOP_FREQ, gNMRAudEncObj[pathID].uStopFreq, 0, 0);
		(gNMRAudEncObj[pathID].Encoder.SetInfo)(MP_AUDENC_SETINFO_AAC_SET_CFG, gNMRAudEncObj[pathID].uiADTSHeader, 0, 0);
		DBG_IND("[AUDENC][%d] Media_AAC_AAC ok\r\n", pathID);
	}

	//DBG_DUMP("[AUDENC][%d] RawBlocksize = %d\r\n", pathID, gNMRAudEncObj[pathID].memInfo.uiRawBlocksize);
}

static void _NMR_AudEnc_SetInfo(UINT32 pathID)
{
	UINT32 SR, chs, in_bits, BperSec, chunksize, outBitRate, stopFreq;

	if (gNMRAudEncObj[pathID].uiBits == 0) {
		gNMRAudEncObj[pathID].uiBits = 16;
	}

	if (gNMRAudEncObj[pathID].uiCodec == 0 || gNMRAudEncObj[pathID].uiSampleRate == 0 || gNMRAudEncObj[pathID].uiChannels == 0 || gNMRAudEncObj[pathID].uiBits == 0) {
		DBG_ERR("[AUDENC][%d] invalid param, codec = %d,  sample rate = %d, aud channel = %d, aud bits = %d\r\n",
				pathID,
				gNMRAudEncObj[pathID].uiCodec,
				gNMRAudEncObj[pathID].uiSampleRate,
				gNMRAudEncObj[pathID].uiChannels,
				gNMRAudEncObj[pathID].uiBits);
		return;
	}

	SR = gNMRAudEncObj[pathID].uiSampleRate;
	chs = gNMRAudEncObj[pathID].uiChannels;
	in_bits = gNMRAudEncObj[pathID].uiBits;

	if (gNMRAudEncObj[pathID].uiRecFormat == MEDIAREC_LIVEVIEW) {
		if (gNMRAudEncObj[pathID].memInfo.uiAudBSBufMs == 0) {
			gNMRAudEncObj[pathID].memInfo.uiAudBSBufMs = NMR_AUDENC_RESERVED_MIN_MS;
		}
	} else {
		if (gNMRAudEncObj[pathID].memInfo.uiAudBSBufMs == 0) {
			gNMRAudEncObj[pathID].memInfo.uiAudBSBufMs = NMR_AUDENC_RESERVED_MAX_MS;
		}
	}

	//#NT#2013/11/08#Hideo Lin -begin
	//#NT#Suggested bit rate according to new AAC lib
	if (gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_AAC && gNMRAudEncObj[pathID].uiPresetBitRate != 0) {
		switch (SR) {
		case AUDIO_SR_11025:
		case AUDIO_SR_12000:
			if (chs == 1) { // mono
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_64000) {
					outBitRate = NMR_AUDENC_BR_48000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			} else {
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_80000) {
					outBitRate = NMR_AUDENC_BR_64000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			}
			break;

		case AUDIO_SR_16000:
			if (chs == 1) { // mono
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_80000) {
					outBitRate = NMR_AUDENC_BR_64000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			} else {
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_112000) {
					outBitRate = NMR_AUDENC_BR_80000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			}
			break;

		case AUDIO_SR_22050:
		case AUDIO_SR_24000:
			if (chs == 1) { // mono
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_128000) {
					outBitRate = NMR_AUDENC_BR_64000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			} else {
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_160000) {
					outBitRate = NMR_AUDENC_BR_96000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			}
			break;

		case AUDIO_SR_32000:
			if (chs == 1) { // mono
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_160000) {
					outBitRate = NMR_AUDENC_BR_96000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			} else {
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_192000) {
					outBitRate = NMR_AUDENC_BR_128000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			}
			break;

		case AUDIO_SR_44100:
		case AUDIO_SR_48000:
			if (chs == 1) { // mono
				outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
			} else {
				outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
			}
			break;

		case AUDIO_SR_8000:
		default:
			if (chs == 1) { // mono
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_32000) {
					outBitRate = NMR_AUDENC_BR_32000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			} else {
				if (gNMRAudEncObj[pathID].uiPresetBitRate > NMR_AUDENC_BR_48000) {
					outBitRate = NMR_AUDENC_BR_48000;
					DBG_ERR("[AUDENC][%d] invalid bitrate = %d for sample rate = %d, aud channel = %d\r\n", pathID, gNMRAudEncObj[pathID].uiPresetBitRate, gNMRAudEncObj[pathID].uiSampleRate, gNMRAudEncObj[pathID].uiChannels);
				} else {
					outBitRate = gNMRAudEncObj[pathID].uiPresetBitRate;
				}
			}
			break;
		}

	} else {
		switch (SR) {
		case AUDIO_SR_11025:
		case AUDIO_SR_12000:
			if (chs == 1) { // mono
				outBitRate = NMR_AUDENC_BR_48000;
			} else {
				outBitRate = NMR_AUDENC_BR_64000;
			}
			break;

		case AUDIO_SR_16000:
			if (chs == 1) { // mono
				outBitRate = NMR_AUDENC_BR_64000;
			} else {
				outBitRate = NMR_AUDENC_BR_80000;
			}
			break;

		case AUDIO_SR_22050:
		case AUDIO_SR_24000:
			if (chs == 1) { // mono
				outBitRate = NMR_AUDENC_BR_64000;
			} else {
				outBitRate = NMR_AUDENC_BR_96000;
			}
			break;

		case AUDIO_SR_32000:
			if (chs == 1) { // mono
				outBitRate = NMR_AUDENC_BR_96000;
			} else {
				outBitRate = NMR_AUDENC_BR_128000;
			}
			break;

		case AUDIO_SR_44100:
		case AUDIO_SR_48000:
			if (chs == 1) { // mono
				outBitRate = NMR_AUDENC_BR_112000;
			} else {
				outBitRate = NMR_AUDENC_BR_160000;
			}
			break;

		case AUDIO_SR_8000:
		default:
			if (chs == 1) { // mono
				outBitRate = NMR_AUDENC_BR_32000;
			} else {
				outBitRate = NMR_AUDENC_BR_48000;
			}
			break;
		}
	}

	//#NT#2013/11/08#Hideo Lin -end

	//#NT#2014/01/13#Hideo Lin -begin
	//#NT#Set AAC stop frequency (suggested)
	switch (SR) {
	case AUDIO_SR_22050:
	case AUDIO_SR_24000:
		stopFreq = 12000; // 12KHz
		break;

	case AUDIO_SR_32000:
		stopFreq = 16000; // 16KHz
		break;

	case AUDIO_SR_44100:
	case AUDIO_SR_48000:
		stopFreq = 24000; // 24KHz
		break;

	case AUDIO_SR_8000:
	case AUDIO_SR_11025:
	case AUDIO_SR_12000:
	case AUDIO_SR_16000:
	default:
		stopFreq = 8000; // 8KHz (minimum stop frequency of AAC lib)
		break;
	}
	//#NT#2014/01/13#Hideo Lin -end

	if (gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_PCM || gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_PPCM) {
		outBitRate = SR * in_bits;
		if (chs == 2) { // mono
			outBitRate *= 2;
		}
	} else if ((gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_ULAW)
			   || (gNMRAudEncObj[pathID].uiCodec == NMEDIAREC_ENC_ALAW)) {
		outBitRate = SR * in_bits / 2;
		if (chs == 2) { // mono
			outBitRate *= 2;
		}
	}

	gNMRAudEncObj[pathID].uiBitRate = outBitRate;
	gNMRAudEncObj[pathID].uStopFreq = stopFreq;

	BperSec = outBitRate / 8; //chs * (in_bits / 8) * SR;
	chunksize = ALIGN_CEIL_4(BperSec);
	gNMRAudEncObj[pathID].memInfo.uiChunkSize = chunksize;
}

static ER _NMR_AudEnc_SetSaveBuf(UINT32 pathID, UINT32 addr, UINT32 size)
{
	if (addr == 0 || size == 0) {
		gNMRAudEncObj[pathID].memInfo.uiBSStart = 0;
		gNMRAudEncObj[pathID].memInfo.uiBSEnd = 0;
		gNMRAudEncObj[pathID].memInfo.uiBSMax = 0;
	} else {
		gNMRAudEncObj[pathID].memInfo.uiBSStart = addr;
		gNMRAudEncObj[pathID].memInfo.uiBSEnd = addr + size;
		gNMRAudEncObj[pathID].memInfo.uiBSMax = gNMRAudEncObj[pathID].memInfo.uiBSEnd - gNMRAudEncObj[pathID].memInfo.uiChunkSize / 2; //reserved half sec audio size (bytes)
	}

	return E_OK;
}

static void _NMR_AudEnc_Stop(UINT32 pathID)
{
	NMI_AUDENC_RAW_INFO             RawInfo = {0};
	NMI_AUDENC_BS_INFO              BsInfo = {0};

	while (_NMR_AudEnc_HowManyinQ(pathID)) {
		_NMR_AudEnc_GetRaw(pathID, &RawInfo);
		BsInfo.PathID = pathID;
		BsInfo.BufID = RawInfo.BufID;
		BsInfo.Addr = 0;
		BsInfo.Size = 0;
		BsInfo.Occupied = 0;
		if (gNMRAudEncObj[pathID].CallBackFunc) {
			(gNMRAudEncObj[pathID].CallBackFunc)(NMI_AUDENC_EVENT_BS_CB, (UINT32) &BsInfo);
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FLUSH_DROP); // [IN] PUSH_DROP
		}
	}
}

static void _NMR_AudEnc_WaitTskIdle(void)
{
	FLGPTN uiFlag;

	// wait for audioenc task idle
	wai_flg(&uiFlag, FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_IDLE, TWF_ORW);
#if MP_AUDENC_SHOW_MSG
	DBG_DUMP("[AUDENC] wait idle finished\r\n");
#endif
}

THREAD_DECLARE(_NMR_AudEnc_Tsk, arglist)
{
	FLGPTN uiFlag = 0;
	UINT32 pathID = 0;

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_ENCODE | FLG_NMR_AUDENC_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_IDLE);

		if (uiFlag & FLG_NMR_AUDENC_STOP) {
			break;
		}

		if (uiFlag & FLG_NMR_AUDENC_ENCODE) {
			if (_NMR_AudEnc_GetJobCount() > 0) {
				if (_NMR_AudEnc_GetJob(&pathID) == E_OK) {
					_NMR_AudEnc_Encode(pathID);
				} else {
					DBG_ERR("[AUDENC][%d] get job failed\r\n", pathID);
				}
			}
		}

		if (_NMR_AudEnc_GetJobCount() > 0) { //continue to trig job
			set_flg(FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_ENCODE);
		}
	}   //  end of while loop

	set_flg(FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_STOP_DONE);

	THREAD_RETURN(0);
}

static ER _NMR_AudEnc_Open(void)
{
	if (gNMRAudEncOpened) {
		return E_SYS;
	}

	// start tsk
	THREAD_CREATE(NMR_AUDENC_TSK_ID, _NMR_AudEnc_Tsk, NULL, "_NMR_AudEnc_Tsk");
	if (NMR_AUDENC_TSK_ID == 0) {
		DBG_ERR("NMR_AUDENC_TSK_ID = %d\r\n", (unsigned int)NMR_AUDENC_TSK_ID);
		return E_SYS;
	}
	THREAD_RESUME(NMR_AUDENC_TSK_ID);

	gNMRAudEncOpened = TRUE;

	return E_OK;
}

/**
    @name   NMediaRec AudEnc Export API
*/
void NMR_AudEnc_ShowMsg(UINT32 pathID, UINT32 msgLevel)
{
	if (pathID < PATH_MAX_COUNT) {
		gNMRAudEncObj[pathID].uiMsgLevel = msgLevel;
	}
}


/**
    @name   NMediaRec AudEnc NMI API
*/
static ER NMI_AudEnc_Open(UINT32 Id, UINT32 *pContext)
{
	gNMRAudEncPathCount++;
	if (gNMRAudEncPathCount == 1) {
		//create enc & trig tsk first
		if (_NMR_AudEnc_Open() != E_OK) {
			gNMRAudEncPathCount--;
			return E_SYS;
		}
		_NMR_AudEnc_InitJobQ();
	}
	return E_OK;
}

static ER NMI_AudEnc_SetParam(UINT32 Id, UINT32 Param, UINT32 Value)
{
	NMI_AUDENC_MEM_RANGE *pMem = NULL;

	switch (Param) {
	case NMI_AUDENC_PARAM_UNLOCK_BS_ADDR:
		_NMR_AudEnc_UnlockBS(Id, Value);
		break;

	case NMI_AUDENC_PARAM_ENCODER_OBJ:
		gNMRAudEncObj[Id].pEncoder = (MP_AUDENC_ENCODER *)Value;
		break;

	case NMI_AUDENC_PARAM_MEM_RANGE:
		//gNMRAudEncObj[Id].uiBits = 16;  // <--- [kflow] user need to use NMI_AudEnc_SetParam(...NMI_AUDENC_PARAM_BITS...) to set value
		pMem = (NMI_AUDENC_MEM_RANGE *) Value;
		_NMR_AudEnc_SetSaveBuf(Id, pMem->Addr, pMem->Size);
		break;

	case NMI_AUDENC_PARAM_MAX_MEM_INFO:
	{
		UINT32 uiCodec, uiRecFormat, uiAudChannels, uiAudioSR, uiAudBits, uiEncsize;
		NMI_AUDENC_MAX_MEM_INFO* pMaxMemInfo = (NMI_AUDENC_MAX_MEM_INFO*) Value;

		uiCodec = gNMRAudEncObj[Id].uiCodec;
		uiRecFormat = gNMRAudEncObj[Id].uiRecFormat;
		uiAudChannels = gNMRAudEncObj[Id].uiChannels;
		uiAudioSR = gNMRAudEncObj[Id].uiSampleRate;
		uiAudBits = gNMRAudEncObj[Id].uiBits;

		gNMRAudEncObj[Id].uiCodec = pMaxMemInfo->uiAudCodec;
		gNMRAudEncObj[Id].uiRecFormat = pMaxMemInfo->uiRecFormat;
		gNMRAudEncObj[Id].uiChannels = pMaxMemInfo->uiAudChannels;
		gNMRAudEncObj[Id].uiSampleRate = pMaxMemInfo->uiAudioSR;
		gNMRAudEncObj[Id].uiBits = pMaxMemInfo->uiAudBits;

		_NMR_AudEnc_SetInfo(Id);
		uiEncsize = _NMR_AudEnc_GetBufSize(Id);
#if MP_AUDENC_SHOW_MSG
		DBG_DUMP("[AUDENC][%d] Set max alloc size, codec = %d, rec format = %d, ch = %d, sr = %d, bits = %d, enc buf size = %d\r\n",
				Id, gNMRAudEncObj[Id].uiCodec, gNMRAudEncObj[Id].uiRecFormat, gNMRAudEncObj[Id].uiChannels, gNMRAudEncObj[Id].uiSampleRate, gNMRAudEncObj[Id].uiBits, uiEncsize);
#endif
		gNMRAudEncObj[Id].uiCodec = uiCodec;
		gNMRAudEncObj[Id].uiRecFormat = uiRecFormat;
		gNMRAudEncObj[Id].uiChannels = uiAudChannels;
		gNMRAudEncObj[Id].uiSampleRate = uiAudioSR;
		gNMRAudEncObj[Id].uiBits = uiAudBits;
		pMaxMemInfo->uiEncsize = uiEncsize;
		break;
	}

	case NMI_AUDENC_REG_CB:
		gNMRAudEncObj[Id].CallBackFunc = (NMI_AUDENC_CB *) Value;
		break;

	case NMI_AUDENC_PARAM_FILETYPE:
		gNMRAudEncObj[Id].filetype = Value;
		break;

	case NMI_AUDENC_PARAM_RECFORMAT:
		gNMRAudEncObj[Id].uiRecFormat = Value;
		break;

	case NMI_AUDENC_PARAM_CODEC:
		gNMRAudEncObj[Id].uiCodec = Value;
		break;

	case NMI_AUDENC_PARAM_CHS:
		gNMRAudEncObj[Id].uiChannels = Value;
		break;

	case NMI_AUDENC_PARAM_SAMPLERATE:
		gNMRAudEncObj[Id].uiSampleRate = Value;
		break;

	case NMI_AUDENC_PARAM_BITS:
		gNMRAudEncObj[Id].uiBits = Value;
		break;

	case NMI_AUDENC_PARAM_AAC_ADTS_HEADER:
		gNMRAudEncObj[Id].uiADTSHeader = Value;
		break;

	case NMI_AUDENC_PARAM_FIXED_SAMPLE:
		if (Value > NMR_AUDENC_MAX_FIXED_SAMPLE) {
			DBG_ERR("[AUDENC][%d] invalid fixed samples = %d > max(%d)\r\n", Id, Value, NMR_AUDENC_MAX_FIXED_SAMPLE);
		} else {
			gNMRAudEncObj[Id].uiFixSamples = Value;
		}
		break;

	case NMI_AUDENC_PARAM_MAX_FRAME_QUEUE:
		if (Value <= NMR_AUDENC_BSQ_MAX) {
			gNMRAudEncObj[Id].uiMaxFrameQueue = Value;
		} else {
			DBG_ERR("[AUDENC][%d] max queue count(%d) > %d\r\n", Id, Value, NMR_AUDENC_BSQ_MAX);
		}
		break;

	case NMI_AUDENC_PARAM_DROP_FRAME:
		// [ToDo]
		break;

	case NMI_AUDENC_PARAM_ENCBUF_MS:
		gNMRAudEncObj[Id].memInfo.uiAudBSBufMs = Value;
		break;

	case NMI_AUDENC_PARAM_BS_RESERVED_SIZE:
		gNMRAudEncObj[Id].uiBsReservedSize = Value;
		break;

	case NMI_AUDENC_PARAM_AAC_VER:
		gNMRAudEncObj[Id].uiAacVer = Value;
		break;

	case NMI_AUDENC_PARAM_TARGETRATE:
		if (Value != NMR_AUDENC_BR_16000  &&
			Value != NMR_AUDENC_BR_32000  &&
			Value != NMR_AUDENC_BR_48000  &&
			Value != NMR_AUDENC_BR_64000  &&
			Value != NMR_AUDENC_BR_80000  &&
			Value != NMR_AUDENC_BR_96000  &&
			Value != NMR_AUDENC_BR_112000 &&
			Value != NMR_AUDENC_BR_128000 &&
			Value != NMR_AUDENC_BR_144000 &&
			Value != NMR_AUDENC_BR_160000 &&
			Value != NMR_AUDENC_BR_192000) {

			return E_PAR;
		} else {
			gNMRAudEncObj[Id].uiPresetBitRate = Value;
		}
		break;

	default:
		DBG_ERR("[AUDENC][%d] Set invalid param = %d\r\n", Id, Param);
		return E_PAR;
	}
	return E_OK;
}

static ER NMI_AudEnc_GetParam(UINT32 Id, UINT32 Param, UINT32 *pValue)
{
	switch (Param) {
	case NMI_AUDENC_PARAM_ALLOC_SIZE:
		_NMR_AudEnc_SetInfo(Id);
		*pValue = _NMR_AudEnc_GetBufSize(Id);
		break;

	case NMI_AUDENC_PARAM_FILETYPE:
		*pValue = gNMRAudEncObj[Id].filetype;
		break;

	case NMI_AUDENC_PARAM_RECFORMAT:
		*pValue = gNMRAudEncObj[Id].uiRecFormat;
		break;

	case NMI_AUDENC_PARAM_CODEC:
		*pValue = gNMRAudEncObj[Id].uiCodec;
		break;

	case NMI_AUDENC_PARAM_CHS:
		*pValue = gNMRAudEncObj[Id].uiChannels;
		break;

	case NMI_AUDENC_PARAM_SAMPLERATE:
		*pValue = gNMRAudEncObj[Id].uiSampleRate;
		break;

	case NMI_AUDENC_PARAM_BITS:
		*pValue = gNMRAudEncObj[Id].uiBits;
		break;

	case NMI_AUDENC_PARAM_AAC_ADTS_HEADER:
		*pValue = gNMRAudEncObj[Id].uiADTSHeader;
		break;

	case NMI_AUDENC_PARAM_FIXED_SAMPLE:
		*pValue = gNMRAudEncObj[Id].uiFixSamples;
		break;

	case NMI_AUDENC_PARAM_ENCBUF_MS:
		*pValue = gNMRAudEncObj[Id].memInfo.uiAudBSBufMs;
		break;

	case NMI_AUDENC_PARAM_BS_RESERVED_SIZE:
		*pValue = gNMRAudEncObj[Id].uiBsReservedSize;
		break;

	case NMI_AUDENC_PARAM_AAC_VER:
		*pValue = gNMRAudEncObj[Id].uiAacVer;
		break;

	default:
		DBG_ERR("[AUDENC][%d] Get invalid param = %d\r\n", Id, Param);
		return E_PAR;
	}
	return E_OK;
}

static ER NMI_AudEnc_Action(UINT32 Id, UINT32 Action)
{
	ER status = E_SYS;

#if MP_AUDENC_SHOW_MSG
	DBG_DUMP("[AUDENC][%d] Action = %d, Codec = %d, Mode = %d, Sr = %d, Ch = %d, Bits = %d, Fix Sample = %d, ADTS = %d, Buf = (0x%08x, 0x%08x, 0x%08x)\r\n",
		Id,
		Action,
		gNMRAudEncObj[Id].uiCodec,
		gNMRAudEncObj[Id].uiRecFormat,
		gNMRAudEncObj[Id].uiSampleRate,
		gNMRAudEncObj[Id].uiChannels,
		gNMRAudEncObj[Id].uiBits,
		gNMRAudEncObj[Id].uiFixSamples,
		gNMRAudEncObj[Id].uiADTSHeader,
		gNMRAudEncObj[Id].memInfo.uiBSStart,
		gNMRAudEncObj[Id].memInfo.uiBSEnd,
		gNMRAudEncObj[Id].memInfo.uiBSMax);
#endif

	switch (Action) {
	case NMI_AUDENC_ACTION_START:
		if (gNMRAudEncObj[Id].bStart == FALSE) {
			_NMR_AudEnc_CreateEncObj(Id);
			status = _NMR_AudEnc_Init(Id);
			if (status != E_OK) {
				DBG_ERR("[AUDENC][%d] init failed\r\n", Id);
				return status;
			}
			_NMR_AudEnc_SetEncParam(Id);
			_NMR_AudEnc_ResetMem(Id);
			_NMR_AudEnc_InitQ(Id);
			gNMRAudEncObj[Id].bStart = TRUE;
			status = E_OK;

			#if PERF_CHK == ENABLE
			enc_time = 0;
			enc_count = 0;
			#endif
		} else {
			DBG_ERR("[AUDENC][%d] invalid operation, start again before stopping", Id);
			status = E_SYS;
		}
		break;

	case NMI_AUDENC_ACTION_STOP:
		gNMRAudEncObj[Id].bStart = FALSE;
		_NMR_AudEnc_Stop(Id);
		_NMR_AudEnc_WaitTskIdle();
		status = E_OK;
		break;

	default:
		DBG_ERR("[AUDENC][%d] Invalid action = %d\r\n", Id, Action);
		break;
	}

	return status;
}

static ER NMI_AudEnc_In(UINT32 Id, UINT32 *pBuf)
{
	DBG_MSG("[AUDENC] Input data(%d)\r\n", Id);

	if (gNMRAudEncObj[Id].bStart == FALSE) {
		return E_SYS;
	}

	if (_NMR_AudEnc_PutRaw(Id, (NMI_AUDENC_RAW_INFO *) pBuf) != E_OK) {
		return E_SYS;
	}

	if (_NMR_AudEnc_PutJob(Id) != E_OK) {
		return E_SYS;
	}

	set_flg(FLG_ID_NMR_AUDENC, FLG_NMR_AUDENC_ENCODE);

	return E_OK;
}

static ER NMI_AudEnc_Close(UINT32 Id)
{
	gNMRAudEncPathCount--;
	if (gNMRAudEncPathCount == 0) {
		_NMR_AudEnc_Close();
	}
	return E_OK;
}

NMI_UNIT NMI_AudEnc = {
	.Name           = "AudEnc",
	.Open           = NMI_AudEnc_Open,
	.SetParam       = NMI_AudEnc_SetParam,
	.GetParam       = NMI_AudEnc_GetParam,
	.Action         = NMI_AudEnc_Action,
	.In             = NMI_AudEnc_In,
	.Close          = NMI_AudEnc_Close,
};

void nmr_audenc_install_id(void)
{
	UINT32 i = 0;

#if defined __UITRON || defined __ECOS
	OS_CONFIG_TASK(NMR_AUDENC_TSK_ID,     PRI_NMR_AUDENC,      STKSIZE_NMR_AUDENC,      _NMR_AudEnc_Tsk);
#endif

	OS_CONFIG_FLAG(FLG_ID_NMR_AUDENC);

	for (i = 0; i < PATH_MAX_COUNT; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(NMR_AUDENC_SEM_ID[i], 0, 1, 1);
#else
		SEM_CREATE(NMR_AUDENC_SEM_ID[i], 1);
#endif
	}
	OS_CONFIG_SEMPHORE(NMR_AUDENC_COMMON_SEM_ID, 0, 1, 1);
}

void nmr_audenc_uninstall_id(void)
{
	UINT32 i = 0;

	rel_flg(FLG_ID_NMR_AUDENC);

	for (i = 0; i < PATH_MAX_COUNT; i++) {
		SEM_DESTROY(NMR_AUDENC_SEM_ID[i]);
	}

	SEM_DESTROY(NMR_AUDENC_COMMON_SEM_ID);
}

static BOOL Cmd_AudEnc_ShowMsg(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiMsgLevel = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiMsgLevel);
	NMR_AudEnc_ShowMsg(uiPathID, uiMsgLevel);
	return TRUE;
}

SXCMD_BEGIN(audenc,                 "NMI aud enc module")
SXCMD_ITEM("showmsg %s",		    Cmd_AudEnc_ShowMsg, 		     "show msg (Param: PathId Level(0:Disable, 1:Enc time) => showmsg 0 1)")
SXCMD_END()

void NMR_AudEnc_AddUnit(void)
{
	NMI_AddUnit(&NMI_AudEnc);
	sxcmd_addtable(audenc);
}

#ifdef __KERNEL__

int _isf_audenc_cmd_audenc_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(audenc);
	UINT32 loop=1;

	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "audenc");
	DBG_DUMP("=====================================================================\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", audenc[loop].p_name, audenc[loop].p_desc);
	}

	return 0;
}

int _isf_audenc_cmd_audenc(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(audenc);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_isf_audenc_cmd_audenc_showhelp();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, audenc[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = audenc[loop].p_func(cmd_args);
			break;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/hdal/aenc/help\" for help.\r\n");
		return -1;
	}


	return 0;
}
#endif

