/*
    Novatek Media Audio Decode Task.

    This file is the task of novatek media audio decoder.

    @file       nmediaplay_auddec_tsk.c
    @date       2018/09/25

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/stdio.h"
//#include "kwrap/sxcmd.h"
#include "nmediaplay_auddec_tsk.h"
#include "nmediaplay_api.h"
#include "audio_common/include/Audio.h"
#include "audio_codec_pcm.h"
#include "audio_codec_ppcm.h"
#include "audio_codec_aac.h"
#include "audio_codec_alaw.h"
#include "audio_codec_ulaw.h"
#include "kflow_audiodec/isf_auddec.h"
#include "sxcmd_wrapper.h"

// perf
#define Perf_GetCurrent(args...)     0

#define PERF_CHK DISABLE

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          nmediaplay_auddec
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int nmediaplay_auddec_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////

/**
    @addtogroup mIAPPNMEDIAPLAY
*/
//@{
#define PRI_NMP_AUDDEC                          7
#define STKSIZE_NMP_AUDDEC                      2048
#define FLG_NMP_AUDDEC_IDLE						FLGPTN_BIT(0) //0x00000001
#define FLG_NMP_AUDDEC_DECODE					FLGPTN_BIT(1) //0x00000002
#define FLG_NMP_AUDDEC_STOP					    FLGPTN_BIT(2) //0x00000004
#define FLG_NMP_AUDDEC_STOP_DONE			    FLGPTN_BIT(3) //0x00000008

#define NMP_AUDDEC_MAX_PATH                     16

UINT32 g_auddec_path_max_count = 0;

#if NMP_AUDDEC_DYNAMIC_CONTEXT
NMP_AUDDEC_OBJ                                  *gNMPAudDecObj = NULL;
NMI_AUDDEC_CONFIG								*gNMPAudDecConfig = NULL;
#else
NMP_AUDDEC_OBJ           						gNMPAudDecObj[NMP_AUDDEC_MAX_PATH]			                            = {0};
NMI_AUDDEC_CONFIG								gNMPAudDecConfig[NMP_AUDDEC_MAX_PATH]		                            = {0};
#endif
//NMI_AUDDEC_BSSRC_SET							gNMPAudDec_BSSetQueue[NMP_AUDDEC_MAX_PATH][NMP_AUDDEC_BSSET_QUEUE_SIZE] = {0};
UINT32											gNMPAudDecRawStartAddr[NMP_AUDDEC_MAX_PATH]	                            = {0};
UINT32											gNMPAudDecRawEndAddr[NMP_AUDDEC_MAX_PATH]	                            = {0};
UINT32											gNMPAudDecRawQueIdx[NMP_AUDDEC_MAX_PATH]		                        = {0};
UINT32											gNMPAudDec_RawAddr[NMP_AUDDEC_MAX_PATH][NMP_AUDDEC_RAW_BLOCK_NUMBER]    = {0};
UINT32											gNMPAudDec_RawBlkSize[NMP_AUDDEC_MAX_PATH]                              = {0};          ///< raw block size (1/5 sec)
UINT32											gNMPAudDec_PreRawBlkSize[NMP_AUDDEC_MAX_PATH]                              = {0};          ///< raw block size (1/5 sec)

NMP_AUDDEC_JOBQ                                 gNMPAudDecJobQ           = {0};
THREAD_HANDLE                                   NMP_AUDDEC_TSK_ID		 = 0;
ID                                              FLG_ID_NMP_AUDDEC		 = 0;
SEM_HANDLE                                      NMP_AUDDEC_SEM_ID[NMP_AUDDEC_MAX_PATH] = {0};
SEM_HANDLE                                      NMP_AUDDEC_COMMON_SEM_ID = {0};
UINT32                                          gNMPAudDecPathCount		 = 0;
UINT8                                           gNMPAudDecOpened         = FALSE;

// debug
static BOOL 									auddec_debug = FALSE;            // debug message open/close

#if PERF_CHK == ENABLE
#include "kwrap/perf.h"
static VOS_TICK dec_time = 0;
static UINT32 dec_count = 0;
#endif

#define AUDDEC_DBG(fmtstr, args...) if (auddec_debug) DBG_DUMP(fmtstr, ##args)

UINT32 _nmp_auddec_query_context_obj_size(void)
{
#if NMP_AUDDEC_DYNAMIC_CONTEXT
	return (PATH_MAX_COUNT * sizeof(NMP_AUDDEC_OBJ));
#else
	return 0;
#endif
}

void _nmp_auddec_assign_context_obj(UINT32 addr)
{
#if NMP_AUDDEC_DYNAMIC_CONTEXT
	gNMPAudDecObj = (NMP_AUDDEC_OBJ *)addr;
	memset(gNMPAudDecObj, 0, _nmp_auddec_query_context_obj_size());
#endif
	return;
}

UINT32 _nmp_auddec_query_context_cfg_size(void)
{
#if NMP_AUDDEC_DYNAMIC_CONTEXT
	return (PATH_MAX_COUNT * sizeof(NMI_AUDDEC_CONFIG));
#else
	return 0;
#endif
}

void _nmp_auddec_assign_context_cfg(UINT32 addr)
{
#if NMP_AUDDEC_DYNAMIC_CONTEXT
	gNMPAudDecConfig = (NMI_AUDDEC_CONFIG *)addr;
	memset(gNMPAudDecConfig, 0, _nmp_auddec_query_context_cfg_size());
#endif
	return;
}

void _nmp_auddec_free_context(void)
{
#if NMP_AUDDEC_DYNAMIC_CONTEXT
	gNMPAudDecObj = NULL;
	gNMPAudDecConfig = NULL;
#endif
	return;
}

BOOL _nmp_auddec_is_init(void)
{
#if NMP_AUDDEC_DYNAMIC_CONTEXT
	return (gNMPAudDecObj)? TRUE:FALSE;
#else
	return TRUE;
#endif
}

static void _NMP_AudDec_InitJobQ(void)
{
	memset(&gNMPAudDecJobQ, 0, sizeof(NMP_AUDDEC_JOBQ));
}

static ER _NMP_AudDec_PutJob(UINT32 pathID)
{
	SEM_WAIT(NMP_AUDDEC_COMMON_SEM_ID);

	if ((gNMPAudDecJobQ.Front == gNMPAudDecJobQ.Rear) && (gNMPAudDecJobQ.bFull == TRUE)) {
		static UINT32 sCount;
		SEM_SIGNAL(NMP_AUDDEC_COMMON_SEM_ID);
		if (sCount % 128 == 0) {
			DBG_WRN("[AUDDEC] Job Queue is Full, Job Id = %d\r\n", pathID);
		}
		sCount++;
		return E_SYS;
	} else {
		gNMPAudDecJobQ.Queue[gNMPAudDecJobQ.Rear] = pathID;
		gNMPAudDecJobQ.Rear = (gNMPAudDecJobQ.Rear + 1) % NMP_AUDDEC_JOBQ_MAX;
		if (gNMPAudDecJobQ.Front == gNMPAudDecJobQ.Rear) { // Check Queue full
			gNMPAudDecJobQ.bFull = TRUE;
		}
		SEM_SIGNAL(NMP_AUDDEC_COMMON_SEM_ID);
		return E_OK;
	}
}

static ER _NMP_AudDec_GetJob(UINT32 *pPathID)
{
	SEM_WAIT(NMP_AUDDEC_COMMON_SEM_ID);
	if ((gNMPAudDecJobQ.Front == gNMPAudDecJobQ.Rear) && (gNMPAudDecJobQ.bFull == FALSE)) {
		SEM_SIGNAL(NMP_AUDDEC_COMMON_SEM_ID);
		DBG_ERR("[AUDDEC] Job Queue is Empty!\r\n");
		return E_SYS;
	} else {
		*pPathID = gNMPAudDecJobQ.Queue[gNMPAudDecJobQ.Front];
		gNMPAudDecJobQ.Front = (gNMPAudDecJobQ.Front + 1) % NMP_AUDDEC_JOBQ_MAX;
		if (gNMPAudDecJobQ.Front == gNMPAudDecJobQ.Rear) { // Check Queue full
			gNMPAudDecJobQ.bFull = FALSE;
		}
		SEM_SIGNAL(NMP_AUDDEC_COMMON_SEM_ID);
		return E_OK;
	}
}

static UINT32 _NMP_AudDec_GetJobCount(void)
{
	UINT32 front, rear, full, sq = 0;
	SEM_WAIT(NMP_AUDDEC_COMMON_SEM_ID);
	front = gNMPAudDecJobQ.Front;
	rear = gNMPAudDecJobQ.Rear;
	full = gNMPAudDecJobQ.bFull;
	SEM_SIGNAL(NMP_AUDDEC_COMMON_SEM_ID);
	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMP_AUDDEC_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMP_AUDDEC_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}

static void _NMP_AudDec_SetInfo(UINT32 pathID)
{
	UINT32 raw_blk_num = NMP_AUDDEC_RAW_BLOCK_NUMBER;

	//gNMPAudDec_RawBlkSize[pathID] = gNMPAudDecConfig[pathID].uiAudioSampleRate * gNMPAudDecConfig[pathID].uiAudioChannels * 2 / raw_blk_num;
	if (gNMPAudDec_PreRawBlkSize[pathID] != 0) {
		gNMPAudDec_RawBlkSize[pathID] = gNMPAudDec_PreRawBlkSize[pathID];
	} else {
		gNMPAudDec_RawBlkSize[pathID] = gNMPAudDecConfig[pathID].uiAudioSampleRate * gNMPAudDecConfig[pathID].uiAudioChannels * gNMPAudDecConfig[pathID].uiAudioBits / 8 / raw_blk_num;
	}

	if (gNMPAudDec_RawBlkSize[pathID] == 0) {
		DBG_ERR("raw_blk_size = 0\r\n");
		return;
	}
}

static UINT32 _NMP_AudDec_GetBufSize(UINT32 pathID)
{
	UINT32 tt_buf_size, raw_blk_size;
	UINT32 raw_blk_num = NMP_AUDDEC_RAW_BLOCK_NUMBER;

	// 1/5 second per block
	//raw_blk_size = gNMPAudDecConfig[pathID].uiAudioSampleRate * gNMPAudDecConfig[pathID].uiAudioChannels * 2 / NMI_AUDDEC_DECODE_BLOCK_CNT;
	if (gNMPAudDec_PreRawBlkSize[pathID] != 0) {
		raw_blk_size = gNMPAudDec_PreRawBlkSize[pathID];
	} else {
		raw_blk_size = ALIGN_CEIL_4(gNMPAudDecConfig[pathID].uiAudioSampleRate * gNMPAudDecConfig[pathID].uiAudioChannels * gNMPAudDecConfig[pathID].uiAudioBits / 8 / NMI_AUDDEC_DECODE_BLOCK_CNT);
	}

	// Total buffer size
	tt_buf_size = raw_blk_size * raw_blk_num;

	AUDDEC_DBG("[AUDDEC]sr=%d, ch=%d, bit=%d, blk_size=0x%x, blk_num=%d, TotalBuf=0x%x\r\n",
			gNMPAudDecConfig[pathID].uiAudioSampleRate,
			gNMPAudDecConfig[pathID].uiAudioChannels,
			gNMPAudDecConfig[pathID].uiAudioBits,
			raw_blk_size,
			raw_blk_num,
			tt_buf_size);

	return tt_buf_size;
}

static UINT32 _NMP_AudDec_mallocRAW(UINT32 Id, UINT32 uiSize)
{
	UINT32 uiAddr, uiTmp;

	uiTmp = ALIGN_CEIL_4(uiSize);

	uiAddr = gNMPAudDecRawStartAddr[Id];
	gNMPAudDecRawStartAddr[Id] += uiTmp;
	if (gNMPAudDecRawStartAddr[Id] > gNMPAudDecRawEndAddr[Id]) {
		DBG_ERR("RAW buffer not enough start=0x%x, end=0x%x!!!!!\r\n", gNMPAudDecRawStartAddr[Id], gNMPAudDecRawEndAddr[Id]);
		return 0;
	}

	return uiAddr;
}

static ER _NMP_AudDec_CreateDecObj(UINT32 pathID)
{
	//UINT32 tempAddr;

	memset(&(gNMPAudDecObj[pathID].Decoder), 0, sizeof(MP_AUDDEC_DECODER));

	if (gNMPAudDecObj[pathID].pDecoder == 0) {
		DBG_ERR("[%d] pDecoder is null\r\n", pathID);
		return E_SYS;
	}

	memcpy((void *) &(gNMPAudDecObj[pathID].Decoder), (void *)gNMPAudDecObj[pathID].pDecoder, sizeof(MP_AUDDEC_DECODER));

	// need this ?
	//(gNMPAudDecObj[pathID].Decoder.GetInfo)(MP_AUDDEC_GETINFO_PARAM, (UINT32 *)&tempAddr, 0, 0);
	//gNMRAudEncObj[pathID].pEncParam = (MP_AUDENC_PARAM *)tempAddr;
	return E_OK;
}

static void _NMP_AudDec_SetEncParam(UINT32 pathID)
{
	// Set Output RAW Mode
	if (gNMPAudDecObj[pathID].Decoder.SetInfo) {
		(gNMPAudDecObj[pathID].Decoder.SetInfo)(MP_AUDDEC_SETINFO_OUTPUTRAWMODE, TRUE, 0, 0);
		(gNMPAudDecObj[pathID].Decoder.SetInfo)(MP_AUDDEC_SETINFO_ADTS_EN, gNMPAudDecConfig[pathID].uiADTSHeader, 0, 0);
	}
}

static ER _NMP_AudDec_Init(UINT32 pathID)
{
	AUDIO_PLAYINFO AudPlayinfo = {0};
	ER ret;
	UINT32 i = 0, uiAddr = 0;

	if (gNMPAudDecConfig[pathID].uiAudioSampleRate == 0 ||
		gNMPAudDecConfig[pathID].uiAudioChannels == 0) {
		DBG_ERR("[%d] Invalid aud info: sr=%d, ch=%d\r\n", pathID, gNMPAudDecConfig[pathID].uiAudioSampleRate, gNMPAudDecConfig[pathID].uiAudioChannels);
		return E_PAR;
	}

	AudPlayinfo.sampleRate = gNMPAudDecConfig[pathID].uiAudioSampleRate;
	AudPlayinfo.channels   = gNMPAudDecConfig[pathID].uiAudioChannels;

	if (gNMPAudDecObj[pathID].Decoder.Init) {
		ret = (gNMPAudDecObj[pathID].Decoder.Init)(&AudPlayinfo);
		if (ret != E_OK) {
			DBG_ERR("[%d] Media Plug-in init fail\n\r", pathID);
			return ret;
		}
	}

	// Assign RAW Data buffer
	if (gNMPAudDecConfig[pathID].uiRawStartAddr) {
		gNMPAudDecRawStartAddr[pathID] = gNMPAudDecConfig[pathID].uiRawStartAddr;
		gNMPAudDecRawEndAddr[pathID]   = gNMPAudDecRawStartAddr[pathID] + gNMPAudDecConfig[pathID].uiRawBufSize;

		for (i = 0; i < NMP_AUDDEC_RAW_BLOCK_NUMBER; i++) {
			uiAddr = _NMP_AudDec_mallocRAW(pathID, gNMPAudDec_RawBlkSize[pathID]);

			if (uiAddr) {
				gNMPAudDec_RawAddr[pathID][i] = uiAddr;
			}
		}
	} else {
		DBG_ERR("[%d] RAW Data Buffer Start Address is NULL\n\r", pathID);
		return E_PAR;
	}

	// Initial the variables
	gNMPAudDecRawQueIdx[pathID] = 0;

	return E_OK;
}

static void _NMP_AudDec_InitQ(UINT32 pathID)
{
	memset(&(gNMPAudDecObj[pathID].bsQueue), 0, sizeof(NMP_AUDDEC_BSQ));
	memset(&(gNMPAudDecObj[pathID].rawQueue), 0, sizeof(NMP_AUDDEC_RAWQ));
}

#if 0
static ER _NMP_AudDec_ConfigureObj(UINT32 Id, PNMI_AUDDEC_CONFIG pAudDecConfig)
{
	ER						EResult = E_OK;
	AUDIO_PLAYINFO			AudPlayinfo;
	UINT32					i, uiAddr;

	if (pAudDecConfig) {
		memcpy((VOID *)&gNMPAudDecConfig[Id], (VOID *)pAudDecConfig, sizeof(NMI_AUDDEC_CONFIG));
	} else {
		DBG_ERR("NMP_AUDDEC_CONFIG is Null\n\r");
		return E_PAR;
	}

	// Create Media Plug-in
	switch (gNMPAudDecConfig[Id].uiAudioDecType) {
	case MEDIAAUDIO_CODEC_MP4A:
		gNMPAudDecObj[Id].Decoder = MP_AACDec_getAudioObject(Id);
		break;
	case MEDIAAUDIO_CODEC_ULAW:
		gNMPAudDecObj[Id].Decoder = MP_uLawDec_getAudioObject(Id);
		break;
	case MEDIAAUDIO_CODEC_ALAW:
		gNMPAudDecObj[Id].Decoder = MP_aLawDec_getAudioObject(Id);
		break;
	default:
		gNMPAudDecObj[Id].Decoder = MP_PCMDec_getAudioObject(Id);
		break;
	}

	// Initialize & setup Media Plug-in Object
	if (gNMPAudDecObj[Id].Decoder) {
		// Set Output RAW Mode
		if (gNMPAudDecObj[Id].Decoder->SetInfo) {
			(gNMPAudDecObj[Id].Decoder->SetInfo)(MP_AUDDEC_SETINFO_OUTPUTRAWMODE, TRUE, 0, 0);
		}

		AudPlayinfo.sampleRate = gNMPAudDecConfig[Id].uiAudioSampleRate;
		AudPlayinfo.channels   = gNMPAudDecConfig[Id].uiAudioChannels;

		EResult = (gNMPAudDecObj[Id].Decoder->Init)(&AudPlayinfo);

		if (EResult != E_OK) {
			DBG_ERR("Media Plug-in initialize is FAILED!!\n\r");
			return EResult;
		}
	} else {
		DBG_ERR("Audio Decoder is NULL\n\r");
		return E_SYS;
	}

	// Assign RAW Data buffer
	if (gNMPAudDecConfig[Id].uiRawStartAddr) {
		gNMPAudDecRawStartAddr[Id] = gNMPAudDecConfig[Id].uiRawStartAddr;
		gNMPAudDecRawEndAddr[Id]   = gNMPAudDecRawStartAddr[Id] + gNMPAudDecConfig[Id].uiRawBufSize;

		for (i = 0; i < NMP_AUDDEC_RAW_BLOCK_NUMBER; i++) {
			uiAddr = _NMP_AudDec_mallocRAW(Id, gNMPAudDec_RawBlkSize[Id]);

			if (uiAddr) {
				gNMPAudDec_RawAddr[Id][i] = uiAddr;
			}
		}
	} else {
		DBG_ERR("RAW Data Buffer Start Address is NULL\n\r");
		return E_SYS;
	}

	// Set the bit stream queue
	//gNMPAudDecObj[Id].BSSetQueue.pBSSetSrc = gNMPAudDec_BSSetQueue[Id];

	// Initial the variables
	gNMPAudDecRawQueIdx[Id] = 0;

	// Set status
	//gNMPAudDecObj[Id].Status = NMP_AUDDEC_STATUS_CONFIGURE;

	return EResult;
}
#endif

static UINT32 _NMP_AudDec_HowManyInQ(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	NMP_AUDDEC_BSQ *pObj;

	SEM_WAIT(NMP_AUDDEC_SEM_ID[pathID]);
	pObj = &gNMPAudDecObj[pathID].bsQueue;
	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	SEM_SIGNAL(NMP_AUDDEC_SEM_ID[pathID]);
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = NMP_AUDDEC_BSQ_MAX - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = NMP_AUDDEC_BSQ_MAX;
	} else {
		sq = 0;
	}

	return sq;
}

static ER _NMP_AudDec_PutBs(UINT32 pathID, NMI_AUDDEC_BS_INFO *pBsInfo)
{
	NMP_AUDDEC_BSQ *pObj;

	SEM_WAIT(NMP_AUDDEC_SEM_ID[pathID]);

	pObj = &(gNMPAudDecObj[pathID].bsQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		SEM_SIGNAL(NMP_AUDDEC_SEM_ID[pathID]);
		DBG_ERR("[AUDDEC][%d] Add Bs Queue is Full!\r\n", pathID);
		return E_SYS;
	} else {
		pObj->Queue[pObj->Rear].Addr = pBsInfo->Addr;
		pObj->Queue[pObj->Rear].Size = pBsInfo->Size;
		pObj->Queue[pObj->Rear].BufID = pBsInfo->BufID;
		pObj->Queue[pObj->Rear].TimeStamp = pBsInfo->TimeStamp;
		pObj->Rear = (pObj->Rear + 1) % NMP_AUDDEC_BSQ_MAX;
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		SEM_SIGNAL(NMP_AUDDEC_SEM_ID[pathID]);
		return E_OK;
	}
}

static BOOL _NMP_AudDec_GetBs(UINT32 pathID, NMI_AUDDEC_BS_INFO *pBsInfo)
{
	NMP_AUDDEC_BSQ  *pObj;

	SEM_WAIT(NMP_AUDDEC_SEM_ID[pathID]);
	pObj = &(gNMPAudDecObj[pathID].bsQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(NMP_AUDDEC_SEM_ID[pathID]);
		DBG_ERR("[AUDDEC][%d] Get Bs Queue is Empty!\r\n", pathID);
		return E_SYS;
	} else {
		pBsInfo->Addr = pObj->Queue[pObj->Front].Addr;
		pBsInfo->Size = pObj->Queue[pObj->Front].Size;
		pBsInfo->BufID = pObj->Queue[pObj->Front].BufID;
		pBsInfo->TimeStamp = pObj->Queue[pObj->Front].TimeStamp;
		pObj->Front = (pObj->Front + 1) % NMP_AUDDEC_BSQ_MAX;
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}

		SEM_SIGNAL(NMP_AUDDEC_SEM_ID[pathID]);
		return E_OK;
	}
}

static void _NMP_AudDec_Stop(UINT32 pathID)
{
	NMI_AUDDEC_BS_INFO              BsInfo = {0};
	NMI_AUDDEC_RAW_INFO             RawInfo = {0};

	while (_NMP_AudDec_HowManyInQ(pathID)) {
		_NMP_AudDec_GetBs(pathID, &BsInfo);
		RawInfo.PathID = pathID;
		RawInfo.BufID = BsInfo.BufID;
		RawInfo.Addr = 0;
		RawInfo.Size = 0;
		RawInfo.Occupied = 0;
		RawInfo.TimeStamp = 0;
		if (gNMPAudDecObj[pathID].CallBackFunc) {
			(gNMPAudDecObj[pathID].CallBackFunc)(NMI_AUDDEC_EVENT_RAW_CB, (UINT32) &RawInfo);
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pathID), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FLUSH_DROP); // [IN] PUSH_DROP
		}
	}
}

static void _NMP_AudDec_SetRawDataAddr(UINT32 pathID, UINT32 uiAddr)
{
	if (gNMPAudDecObj[pathID].Decoder.SetInfo) {
		(gNMPAudDecObj[pathID].Decoder.SetInfo)(MP_AUDDEC_SETINFO_RAWDATAADDR, uiAddr, 0, 0);
	}
}

static ER _NMP_AudDec_DecodeOne(UINT32 pathID, UINT32 uiBSAddr, UINT32 uiBSSize)
{
	if (gNMPAudDecObj[pathID].Decoder.DecodeOne) {
		(gNMPAudDecObj[pathID].Decoder.DecodeOne)(0, uiBSAddr, uiBSSize);
	}

	return E_OK;
}

static ER _NMP_AudDec_WaitDecodeDone(UINT32 pathID, AUDIO_OUTRAW *pOutRaw)
{
	UINT32 value = E_SYS;
	#if PERF_CHK == ENABLE
	VOS_TICK proc_tick_begin, proc_tick_end;
	#endif

	#if PERF_CHK == ENABLE
	if (dec_count < 200) {
		vos_perf_mark(&proc_tick_begin);
	}
	#endif

	if (gNMPAudDecObj[pathID].Decoder.WaitDecodeDone) {
		value = (gNMPAudDecObj[pathID].Decoder.WaitDecodeDone)(0, 0, (UINT32 *)pOutRaw, 0);
	}

	#if PERF_CHK == ENABLE
	if (dec_count < 200) {
		vos_perf_mark(&proc_tick_end);
		dec_time += vos_perf_duration(proc_tick_begin, proc_tick_end);
		dec_count++;

		if (dec_count == 200) {
			UINT32 sample_cnt = pOutRaw->outRawSize / gNMPAudDecConfig[pathID].uiAudioChannels / (gNMPAudDecConfig[pathID].uiAudioBits/8);
			UINT32 cpu_usage = ((dec_time/200) * (gNMPAudDecConfig[pathID].uiAudioSampleRate/1000) * 10) / sample_cnt;

			DBG_DUMP("[isf_auddec] Decode average process usage = %d,%d%%\r\n", cpu_usage/100, cpu_usage%100);
		}
	}
	#endif

	return value;
}

static ER _NMP_AudDec_Decode(UINT32 pathID)
{
	ER							EResult = E_OK;
	UINT32						uiRawAddr = 0, uiRawSize = 0;
	NMI_AUDDEC_BS_INFO          BsInfo = {0};  // input data
	NMI_AUDDEC_RAW_INFO  	    RawInfo = {0}; // output data
	AUDIO_OUTRAW				outRaw = {0, 0};

	if (gNMPAudDecObj[pathID].Decoder.SetInfo == NULL) {
		DBG_ERR("[%d] Audio codec type = %d, set info is null\r\n", pathID, gNMPAudDecObj[pathID].uiCodec);
		return E_SYS;
	}

	if (gNMPAudDecObj[pathID].bStart == FALSE) {
		return E_SYS;
	}

	_NMP_AudDec_GetBs(pathID, &BsInfo);

	if (BsInfo.Addr == 0 || BsInfo.Size == 0) {
		DBG_ERR("[%d] invalid bs data, a=0x%x, s=%d\r\n", pathID, BsInfo.Addr, BsInfo.Size);
		return E_SYS;
	}

	// Get RAW Address
	(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_ENTER); // [OUT] NEW_ENTER
	uiRawAddr = gNMPAudDec_RawAddr[pathID][gNMPAudDecRawQueIdx[pathID]];

	if (!uiRawAddr) {
		DBG_ERR("[%d] RAW buffer is NULL!!\r\n", pathID);
		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_ERR, ISF_ERR_QUEUE_ERROR); // [OUT] NEW_ERR
		return E_SYS;
	}

	_NMP_AudDec_SetRawDataAddr(pathID, uiRawAddr);
	(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_OK); // [OUT] NEW_OK

	if (BsInfo.Size) {
		// Decode Audio
		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pathID),  ISF_IN_PROBE_PUSH, ISF_OK);     // [IN] PUSH_OK
		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pathID),  ISF_IN_PROBE_PROC, ISF_ENTER);  // [IN] PUSH_ENTER
		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_ENTER); // [OUT] PROC_ENTER

		if (_NMP_AudDec_DecodeOne(pathID, BsInfo.Addr, BsInfo.Size) == E_OK) {
			// Wait Decode Done
			EResult = _NMP_AudDec_WaitDecodeDone(pathID, &outRaw);

			if (EResult == E_OK) {
				if (outRaw.outRawSize) {
					uiRawSize += outRaw.outRawSize;

					if (uiRawSize > gNMPAudDec_RawBlkSize[pathID]) {
						DBG_ERR("[%d] Decode RAW size 0x%x is exceeded maximum size 0x%x!\n\r", pathID, uiRawSize, gNMPAudDec_RawBlkSize[pathID]);
						uiRawSize = gNMPAudDec_RawBlkSize[pathID];
					}
				} else {
					DBG_ERR("[%d] Decode output RAW size is 0, src: a=0x%x, s=0x%x\n\r", pathID, BsInfo.Addr, BsInfo.Size);
#if 0
					UINT8 *dump = (UINT8 *)pAudBSSrc->uiBSAddr;
					UINT32 idx;
					for (idx = 0; idx < pAudBSSrc->uiBSSize; idx++) {
						DBG_DUMP("^R%x ", *(dump+idx));
					}
					DBG_DUMP("^R\r\n");
#endif
					RawInfo.PathID    = pathID;
					RawInfo.BufID     = BsInfo.BufID;
					RawInfo.Addr      = 0;
					RawInfo.Size      = 0;
					RawInfo.TimeStamp = BsInfo.TimeStamp;

					(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
					(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR

					// callback 0 to unit
					if (gNMPAudDecObj[pathID].CallBackFunc) {
						(gNMPAudDecObj[pathID].CallBackFunc)(NMI_AUDDEC_EVENT_RAW_CB, (UINT32) &RawInfo);
					}
					return E_SYS;
				}
			} else {
				DBG_ERR("[%d] Wait Decode Done ERROR !!\r\n", pathID);
				(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
				(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
			}
		} else {
			DBG_ERR("[%d] Decode AUDIO ERROR !!\r\n", pathID);

			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR

			EResult = E_SYS;
		}

	} else {
		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pathID),  ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INCOMPLETE_DATA);     // [IN] PUSH_ERR

		DBG_ERR("[%d] BS Size is 0!\n\r", pathID);
		return E_SYS;
	}

	// Set RAW output
	RawInfo.PathID    = pathID;
	RawInfo.BufID     = BsInfo.BufID;
	RawInfo.Addr      = uiRawAddr;
	RawInfo.Size      = uiRawSize;
	RawInfo.TimeStamp = BsInfo.TimeStamp;

	// Decode OK callback to unit
	(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pathID),  ISF_IN_PROBE_PROC,  ISF_OK); // [IN]  PROC_OK
	(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_OK); // [OUT] PROC_OK

	if (gNMPAudDecObj[pathID].CallBackFunc && RawInfo.Size > 0) {
		(gNMPAudDecObj[pathID].CallBackFunc)(NMI_AUDDEC_EVENT_RAW_CB, (UINT32) &RawInfo);
	}

	// Update RAW Index
	gNMPAudDecRawQueIdx[pathID]++;
	gNMPAudDecRawQueIdx[pathID] %= NMP_AUDDEC_RAW_BLOCK_NUMBER;

	return EResult;
}

static void _NMP_AudDec_WaitTskIdle(void)
{
	FLGPTN uiFlag;

	// wait for audiodec task idle
	wai_flg(&uiFlag, FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_IDLE, TWF_ORW);
}

#if 0
static void _NMP_AudDec_DoDec(UINT32 pathID)
{
	/*if ( kchk_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_IDLE) ) {
		set_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_DECODE);
	} else {
		AUDDEC_DBG("^Y[AUDDEC]Busy!!!!!!!!!!!!!!!!\r\n");
	}*/

	//#NT#2018/01/29#Adam Su -begin
    //#NT#always set flag decode, no matter what task idle arrive.
	if (!kchk_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_IDLE)) {
		AUDDEC_DBG("^Y[AUDDEC]Busy!!!!!!!!!!!!!!!!\r\n");
	}
	set_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_DECODE);
	//#NT#2018/01/29#Adam Su -end
}
#endif

THREAD_DECLARE(_NMP_AudDec_Tsk, arglist)
{
	FLGPTN			uiFlag = 0;
	UINT32          pathID = 0;

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_DECODE | FLG_NMP_AUDDEC_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_IDLE);

		if (uiFlag & FLG_NMP_AUDDEC_STOP) {
			break;
		}

		if (uiFlag & FLG_NMP_AUDDEC_DECODE) {
			if (_NMP_AudDec_GetJobCount() > 0) {
				_NMP_AudDec_GetJob(&pathID);
				_NMP_AudDec_Decode(pathID);
			}
		}//if dec

		if (_NMP_AudDec_GetJobCount() > 0) { //continue to trig job
			set_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_DECODE);
		}
	}   // end of while loop

	set_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_STOP_DONE);

	THREAD_RETURN(0);
}

static ER _NMP_AudDec_Open(void)
{
	if (gNMPAudDecOpened) {
		return E_SYS;
	}

	// start tsk
	THREAD_CREATE(NMP_AUDDEC_TSK_ID, _NMP_AudDec_Tsk, NULL, "_NMP_AudDec_Tsk");
	if (NMP_AUDDEC_TSK_ID == 0) {
		DBG_ERR("NMP_AUDDEC_TSK_ID = %d\r\n", (unsigned int)NMP_AUDDEC_TSK_ID);
		return E_SYS;
	}
	THREAD_RESUME(NMP_AUDDEC_TSK_ID);

	gNMPAudDecOpened = TRUE;
	return E_OK;
}

static ER _NMP_AudDec_Close(void)
{
	FLGPTN uiFlag;

	if (gNMPAudDecOpened == FALSE) {
		return E_SYS;
	}

	wai_flg(&uiFlag, FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_IDLE, TWF_ORW);

#if 1
	set_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_STOP);
	wai_flg(&uiFlag, FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_STOP_DONE, TWF_ORW | TWF_CLR);
#endif

	#if 0
	THREAD_DESTROY(NMP_AUDDEC_TSK_ID); //ter_tsk(NMP_AUDDEC_TSK_ID);
	#endif

	gNMPAudDecOpened = FALSE;
	return E_OK;
}

/**
    @name   NMediaPlay AudDec NMI API
*/
static ER NMI_AudDec_Open(UINT32 Id, UINT32 *pContext)
{
	DBG_IND("[AUDDEC]Open\r\n");

	gNMPAudDecPathCount++;
	if (gNMPAudDecPathCount == 1) {
		//create dec & trig tsk first
		if (_NMP_AudDec_Open() != E_OK) {
			gNMPAudDecPathCount--;
			return E_SYS;
		}
		_NMP_AudDec_InitJobQ();
	}
	return E_OK;
}

static ER NMI_AudDec_SetParam(UINT32 Id, UINT32 Param, UINT32 Value)
{
	NMI_AUDDEC_MEM_RANGE *pMem = NULL;

	switch (Param) {
		case NMI_AUDDEC_PARAM_MEM_RANGE:
			pMem = (NMI_AUDDEC_MEM_RANGE *) Value;
			if (pMem->Addr == 0 || pMem->Size == 0) {
				DBG_ERR("[AUDDEC][%d] Invalid mem range, addr = 0x%08x, size = %d\r\n", Id, pMem->Addr, pMem->Size);
				return E_PAR;
			}
			else {
				DBG_IND("[AUDDEC] MemRange a=0x%x, s=0x%x\r\n", pMem->Addr, pMem->Size);
				gNMPAudDecConfig[Id].uiRawStartAddr = pMem->Addr;
				gNMPAudDecConfig[Id].uiRawBufSize	= pMem->Size;
			}
			break;

		case NMI_AUDDEC_PARAM_DECODER_OBJ:
			DBG_IND("[%d] pDecoder = 0x%x\r\n", Id, Value);
			gNMPAudDecObj[Id].pDecoder = (MP_AUDDEC_DECODER *)Value;
			break;

		case NMI_AUDDEC_PARAM_MAX_MEM_INFO:
		{
			UINT32 uiAudCodec, uiAudChannels, uiAudioSR, uiAudioBits, uiDecSize;
			NMI_AUDDEC_MAX_MEM_INFO *pMaxMemInfo = (NMI_AUDDEC_MAX_MEM_INFO *) Value;

			// keep dynamic param to temp
			uiAudCodec = gNMPAudDecConfig[Id].uiAudioDecType;
			uiAudChannels = gNMPAudDecConfig[Id].uiAudioChannels;
			uiAudioSR = gNMPAudDecConfig[Id].uiAudioSampleRate;
			uiAudioBits = gNMPAudDecConfig[Id].uiAudioBits;

			// get max buf info
			gNMPAudDecConfig[Id].uiAudioDecType = pMaxMemInfo->uiAudCodec;
			gNMPAudDecConfig[Id].uiAudioChannels = pMaxMemInfo->uiAudChannels;
			gNMPAudDecConfig[Id].uiAudioSampleRate = pMaxMemInfo->uiAudioSR;
			gNMPAudDecConfig[Id].uiAudioBits = pMaxMemInfo->uiAudioBits;

			_NMP_AudDec_SetInfo(Id);
			uiDecSize = _NMP_AudDec_GetBufSize(Id);

			DBG_IND("[AUDDEC][%d] Set max alloc size, codec=%d, ch=%d, sr=%d, bits=%d, total buf=0x%x\r\n",
					Id, gNMPAudDecConfig[Id].uiAudioDecType, gNMPAudDecConfig[Id].uiAudioChannels, gNMPAudDecConfig[Id].uiAudioSampleRate, gNMPAudDecConfig[Id].uiAudioBits, uiDecSize);

			// set dynamic param back
			gNMPAudDecConfig[Id].uiAudioDecType = uiAudCodec;
			gNMPAudDecConfig[Id].uiAudioChannels = uiAudChannels;
			gNMPAudDecConfig[Id].uiAudioSampleRate = uiAudioSR;
			gNMPAudDecConfig[Id].uiAudioBits = uiAudioBits;
            pMaxMemInfo->uiDecsize = uiDecSize; // set calculated buf size to ImageUnit_AudDec
			break;
		}

		case NMI_AUDDEC_PARAM_DECTYPE:
			gNMPAudDecConfig[Id].uiAudioDecType = Value;
			break;

		case NMI_AUDDEC_PARAM_SAMPLERATE:
			DBG_IND("[%d] set sr = %d\r\n", Id, Value);
			gNMPAudDecConfig[Id].uiAudioSampleRate = Value;
			break;

		case NMI_AUDDEC_PARAM_CHANNELS:
			DBG_IND("[%d] set ch = %d\r\n", Id, Value);
			gNMPAudDecConfig[Id].uiAudioChannels = Value;
			break;

		case NMI_AUDDEC_PARAM_BITS:
			gNMPAudDecConfig[Id].uiAudioBits = Value;
			break;

		case NMI_AUDDEC_PARAM_REG_CB:
			gNMPAudDecObj[Id].CallBackFunc = (NMI_AUDDEC_CB *)Value;
			break;

		/*case NMI_AUDDEC_PARAM_RAW_STARADDR:
			gNMPAudDecConfig[Id].uiRawStartAddr = Value;
			break;

		case NMI_AUDDEC_PARAM_RAW_BUFSIZE:
			gNMPAudDecConfig[Id].uiRawBufSize = Value;
			break;*/

		case NMI_AUDDEC_PARAM_DROP_FRAME:
			// [ToDo]
			break;

		case NMI_AUDDEC_PARAM_ADTS_EN:
			gNMPAudDecConfig[Id].uiADTSHeader = (Value == 0)? FALSE : TRUE;
			break;

		case NMI_AUDDEC_PARAM_RAW_BLOCK_SIZE:
			gNMPAudDec_PreRawBlkSize[Id] = Value;
			break;

		default:
			DBG_ERR("[AUDDEC][%d] Set invalid param = %d\r\n", Id, Param);
			return E_PAR;
	}
	return E_OK;
}

static ER NMI_AudDec_GetParam(UINT32 Id, UINT32 Param, UINT32 *pValue)
{
	ER status = E_OK;

	switch (Param) {
		case NMI_AUDDEC_PARAM_ALLOC_SIZE:
			*pValue = _NMP_AudDec_GetBufSize(Id);
			break;

		case NMI_AUDDEC_PARAM_DECTYPE:
			*pValue = gNMPAudDecConfig[Id].uiAudioDecType;
			break;

		case NMI_AUDDEC_PARAM_CHANNELS:
			*pValue = gNMPAudDecConfig[Id].uiAudioChannels;
			break;

		case NMI_AUDDEC_PARAM_SAMPLERATE:
			*pValue = gNMPAudDecConfig[Id].uiAudioSampleRate;
			break;

		case NMI_AUDDEC_PARAM_BITS:
			*pValue = gNMPAudDecConfig[Id].uiAudioBits;
			break;

		default:
			DBG_ERR("[AUDDEC][%d] Get invalid param = %d\r\n", Id, Param);
			status = E_PAR;
			break;
	}
	return status;
}

static ER NMI_AudDec_Action(UINT32 Id, UINT32 Action)
{
	ER status = E_SYS;

	AUDDEC_DBG("[AUDDEC][%d] Action = %d\r\n", Id, Action);

	switch (Action) {
		case NMI_AUDDEC_ACTION_START:
			if (gNMPAudDecObj[Id].bStart == FALSE) {

				/*if (_NMP_AudDec_ConfigureObj(Id, &gNMPAudDecConfig[Id]) == E_OK) {
					gNMPAudDecObj[Id].bStart = TRUE;
					status = E_OK;
				} else {
					DBG_ERR("Configure audio decode object fail!\r\n");
					status = E_SYS;
				}*/
				_NMP_AudDec_CreateDecObj(Id);
				_NMP_AudDec_SetEncParam(Id);
				status = _NMP_AudDec_Init(Id);
				if (status != E_OK) {
					DBG_ERR("[AUDDEC][%d] init failed\r\n", Id);
				} else {
					_NMP_AudDec_InitQ(Id);
					gNMPAudDecObj[Id].bStart = TRUE;
				}

				#if PERF_CHK == ENABLE
				dec_time = 0;
				dec_count = 0;
				#endif

			} else {
				DBG_ERR("[AUDDEC][%d] invalid operation, start again before stopping", Id);
				status = E_SYS;
			}
			break;

		case NMI_AUDDEC_ACTION_STOP:
			gNMPAudDecObj[Id].bStart = FALSE;
			_NMP_AudDec_Stop(Id);
			_NMP_AudDec_WaitTskIdle();
			status = E_OK;
			break;

		default:
			DBG_ERR("[AUDDEC][%d] Invalid action = %d\r\n", Id, Action);
			break;
	}

	return status;
}

static ER NMI_AudDec_In(UINT32 Id, UINT32 *pBuf)
{
	DBG_IND("[AUDDEC] Input data(%d)\r\n", Id);

	if (gNMPAudDecObj[Id].bStart == FALSE) {
		return E_SYS;
	}

	if (_NMP_AudDec_PutBs(Id, (NMI_AUDDEC_BS_INFO *)pBuf) != E_OK) {
		return E_SYS;
	}

	_NMP_AudDec_PutJob(Id);
	set_flg(FLG_ID_NMP_AUDDEC, FLG_NMP_AUDDEC_DECODE);

	return E_OK;
}

static ER NMI_AudDec_Close(UINT32 Id)
{
	gNMPAudDecPathCount--;
	if (gNMPAudDecPathCount == 0) {
		_NMP_AudDec_Close();
	}

	return E_OK;
}

NMI_UNIT NMI_AudDec = {
	.Name           = "AudDec",
	.Open           = NMI_AudDec_Open,
	.SetParam       = NMI_AudDec_SetParam,
	.GetParam       = NMI_AudDec_GetParam,
	.Action         = NMI_AudDec_Action,
	.In             = NMI_AudDec_In,
	.Close          = NMI_AudDec_Close,
};

// install task
void nmp_auddec_install_id(void)
{
	UINT32 i = 0;

#if defined __UITRON || defined __ECOS
	OS_CONFIG_TASK(NMP_AUDDEC_TSK_ID, PRI_NMP_AUDDEC, STKSIZE_NMP_AUDDEC, _NMP_AudDec_Tsk);
#endif

	OS_CONFIG_FLAG(FLG_ID_NMP_AUDDEC);

    for (i = 0; i < PATH_MAX_COUNT; i++) {
        OS_CONFIG_SEMPHORE(NMP_AUDDEC_SEM_ID[i], 0, 1, 1);
    }

	 OS_CONFIG_SEMPHORE(NMP_AUDDEC_COMMON_SEM_ID, 0, 1, 1);
}

void nmp_auddec_uninstall_id(void)
{
	UINT32 i = 0;

	rel_flg(FLG_ID_NMP_AUDDEC);
    for (i = 0; i < PATH_MAX_COUNT; i++) {
		SEM_DESTROY(NMP_AUDDEC_SEM_ID[i]);
    }
	SEM_DESTROY(NMP_AUDDEC_COMMON_SEM_ID);
}


//
// test commands
//
/*static BOOL Cmd_AudDec_SetDebug(CHAR *strCmd)
{
	UINT32 dbg_msg;

	sscanf_s(strCmd, "%d", &dbg_msg);

	auddec_debug = (dbg_msg > 0);

	return TRUE;
}*/

SXCMD_BEGIN(auddec, "NMI aud dec module")
//SXCMD_ITEM("dbg %d",	Cmd_AudDec_SetDebug, 	"set debug msg (Param: 1->on, 0->off)")
SXCMD_END()

void NMP_AudDec_AddUnit(void)
{
	NMI_AddUnit(&NMI_AudDec);
	sxcmd_addtable(auddec);
}

#ifdef __KERNEL__

int _isf_auddec_cmd_auddec_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(auddec);
	UINT32 loop=1;

	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "auddec");
	DBG_DUMP("=====================================================================\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", auddec[loop].p_name, auddec[loop].p_desc);
	}

	return 0;
}

int _isf_auddec_cmd_auddec(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(auddec);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_isf_auddec_cmd_auddec_showhelp();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, auddec[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = auddec[loop].p_func(cmd_args);
			break;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/hdal/adec/help\" for help.\r\n");
		return -1;
	}


	return 0;
}
#endif

