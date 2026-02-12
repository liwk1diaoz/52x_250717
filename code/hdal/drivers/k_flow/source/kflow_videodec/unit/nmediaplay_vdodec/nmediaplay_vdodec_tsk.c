/*
    Novatek Media Video Decode Task.

    This file is the task of novatek media video decoder.

    @file       NMediaPlayVdoDecTsk.c
    @ingroup    mIAPPMEDIAPLAY
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include "sxcmd_wrapper.h"
#include "kflow_common/type_vdo.h"
#include "kflow_common/nvtmpp.h"
#include "video_codec_mjpg.h"
#include "video_codec_h264.h"
#include "video_codec_h265.h"
#include "kflow_videodec/isf_vdodec.h"
#include "isf_vdodec_internal.h"
#if !defined(__LINUX)
#include "kwrap/cmdsys.h"
#endif

#include "nmediaplay_api.h"
#include "nmediaplay_vdodec_tsk.h"
#include "nmediaplay_vdodec_platform.h"

#include "jpg_header.h"   // for JPG_FMT_YUV420 define

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          nmediaplay_vdodec
#define __DBGLVL__          8
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int nmediaplay_vdodec_debug_level = NVT_DBG_WRN;
#ifdef __KERNEL__
module_param_named(debug_level_nmediaplay_vdodec, nmediaplay_vdodec_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_nmediaplay_vdodec, "nmediaplay_vdodec debug level");
#else
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif
///////////////////////////////////////////////////////////////////////////////

#define _I_FRM_COPY_TEST                        0

/**
    @addtogroup mIAPPNMEDIAPLAY
*/
//@{
#define PRI_NMP_VDODEC                          3
#define STKSIZE_NMP_VDODEC                      2048

/*
    @name flags for NMP Video Decoder

    flags for NMP video decoder.
*/
#define FLG_NMP_VDODEC_IDLE						FLGPTN_BIT(0) //0x00000001
#define FLG_NMP_VDODEC_DECODE					FLGPTN_BIT(1) //0x00000002
#define FLG_NMP_VDODEC_STOP                     FLGPTN_BIT(2) //0x00000004
#define FLG_NMP_VDODEC_STOP_DONE                FLGPTN_BIT(3) //0x00000008

#define NMP_VDODEC_MAX_PATH                     16
#define NMP_VDODEC_DUMP_DEC_FRAME				0

// debug
static BOOL 									vdodec_debug = FALSE;            // debug message open/close

#if NMP_VDODEC_DYNAMIC_CONTEXT
NMP_VDODEC_OBJ           						*gNMPVdoDecObj = NULL;
NMP_VDODEC_CONFIG								*gNMPVdoDecConfig = NULL;
#else
NMP_VDODEC_OBJ           						gNMPVdoDecObj[NMP_VDODEC_MAX_PATH] = {0};
NMP_VDODEC_CONFIG								gNMPVdoDecConfig[NMP_VDODEC_MAX_PATH] = {0};
#endif

THREAD_HANDLE                                   NMP_VDODEC_TSK_ID = 0;
ID                                         FLG_ID_NMP_VDODEC = 0;
SEM_HANDLE                                      NMP_VDODEC_SEM_ID[NMP_VDODEC_MAX_PATH] = {0};
#ifdef VDODEC_LL
THREAD_HANDLE                                   NMP_VDODEC_TSK_ID_WRAPRAW = 0;
ID                                              FLG_ID_NMP_VDODEC_WRAPRAW = 0;
SEM_HANDLE                                      NMP_VDODEC_TRIG_SEM_ID[NMP_VDODEC_MAX_PATH] = {0};
#endif

#ifdef __KERNEL__
SEM_HANDLE                                      NMP_VDODEC_COMMON_SEM_ID = {0};
#else
SEM_HANDLE                                      NMP_VDODEC_COMMON_SEM_ID = 0;
#endif

UINT8                                           gNMPVdoDecMsg = 0;
UINT32                                          gNMPVdoDecPathCount	= 0;
UINT8                                           gNMPVdoDecPath[NMP_VDODEC_MAX_PATH] = {0};
UINT32                                          gNMPVdoDecInterrupt[NMP_VDODEC_MAX_PATH] = {0};
BOOL                                            gbNMPVdoDecPartial[NMP_VDODEC_MAX_PATH] = {FALSE};
BOOL                                            gbNMPVdoDecHeaderInBs[NMP_VDODEC_MAX_PATH] = {FALSE};

#ifdef VDODEC_LL
UINT8                                           gNMPVdoDecOpened_WrapRAW = FALSE;
NMP_VDODEC_WRAPBS_JOBQ                                gNMPVdoDecJobQ_WrapRAW = {0};
UINT8                                           gNMPVidTrigOpened_WrapRAW = FALSE;
#endif

#if (!_USE_COMMON_FOR_RAW)
UINT32											gNMPVdoDecRawQueIdx[NMP_VDODEC_MAX_PATH] = {0};
#endif
//UINT32											gNMPVdoDecFrameNum[NMP_VDODEC_MAX_PATH] = {0};
NMI_VDODEC_RAWDATA_SRC							gNMPVdoDec_RawAddr[NMP_VDODEC_MAX_PATH][NMP_VDODEC_RAW_BLOCK_NUMBER] = {0};
UINT32											gNMPVdoDec_RawBlkSize[NMP_VDODEC_MAX_PATH] = {0};
//BOOL                                            decoder_initialized[NMP_VDODEC_MAX_PATH] = {0};

#if _I_FRM_COPY_TEST
UINT32                                          ifrm_copy_start[NMP_VDODEC_MAX_PATH] = {0};
UINT32                                          ifrm_copy_end[NMP_VDODEC_MAX_PATH] = {0};
UINT32                                          ifrm_copy_size[NMP_VDODEC_MAX_PATH] = {0};
UINT32                                          ifrm_copy_curr[NMP_VDODEC_MAX_PATH] = {0};
#define IFRM_COPY_BUF_SIZE                      0x400000
#endif

#define VDODEC_DBG(fmtstr, args...) if (vdodec_debug) DBG_DUMP(fmtstr, ##args)

extern void VdoCodec_H26x_Lock(void);
extern void VdoCodec_H26x_UnLock(void);
extern void VdoCodec_JPEG_Lock(void);
extern void VdoCodec_JPEG_UnLock(void);
#ifdef VDODEC_LL
static INT32 NMP_VdoDec_WrapRaw(void *info, void *usr_data);
#endif

UINT32 _nmp_vdodec_query_context_obj_size(void)
{
#if NMP_VDODEC_DYNAMIC_CONTEXT
	return (PATH_MAX_COUNT * sizeof(NMP_VDODEC_OBJ));
#else
	return 0;
#endif
}

UINT32 _nmp_vdodec_query_context_cfg_size(void)
{
#if NMP_VDODEC_DYNAMIC_CONTEXT
	return (PATH_MAX_COUNT * sizeof(NMP_VDODEC_CONFIG));
#else
	return 0;
#endif
}

void _nmp_vdodec_assign_context_obj(UINT32 addr)
{
#if NMP_VDODEC_DYNAMIC_CONTEXT
	gNMPVdoDecObj = (NMP_VDODEC_OBJ *)addr;
	memset(gNMPVdoDecObj, 0, _nmp_vdodec_query_context_obj_size());
#endif
	return;
}

void _nmp_vdodec_assign_context_cfg(UINT32 addr)
{
#if NMP_VDODEC_DYNAMIC_CONTEXT
	gNMPVdoDecConfig= (NMP_VDODEC_CONFIG *)addr;
	memset(gNMPVdoDecConfig, 0, _nmp_vdodec_query_context_cfg_size());
#endif
	return;
}

void _nmp_vdodec_free_context(void)
{
#if NMP_VDODEC_DYNAMIC_CONTEXT
	gNMPVdoDecObj = NULL;
	gNMPVdoDecConfig= NULL;
#endif
	return;
}

BOOL _nmp_vdodec_is_init(void)
{
#if NMP_VDODEC_DYNAMIC_CONTEXT
	return (gNMPVdoDecObj && gNMPVdoDecConfig) ? TRUE:FALSE;
#else
	return TRUE;
#endif
}

static UINT32 _NMP_VdoDec_GetCodecSize(UINT32 pathID)
{
	MP_VDODEC_MEMINFO memInfo = {0};
	UINT32 uiWidth = 0, uiHeight = 0, codecSize = 0;

	if (gNMPVdoDecConfig[pathID].uiWidth == 0 || gNMPVdoDecConfig[pathID].uiHeight == 0) {
		DBG_ERR("[VDODEC][%d] Get Video Codec Size error, width = %d, height = %d\r\n", pathID, gNMPVdoDecConfig[pathID].uiWidth, gNMPVdoDecConfig[pathID].uiHeight);
		return 0;
	}

	if (gNMPVdoDecObj[pathID].pMpVideoDecoder == NULL) {
		DBG_ERR("[VDODEC][%d] Get codec size, but codec not support\r\n", pathID);
		return 0;
	}

	uiWidth = ALIGN_CEIL_64(gNMPVdoDecConfig[pathID].uiWidth);
	uiHeight = ALIGN_CEIL_64(gNMPVdoDecConfig[pathID].uiHeight);

	if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
		memInfo.width = uiWidth;
		memInfo.height = uiHeight;
		if ((MP_H265Dec_getVideoObject((MP_VDODEC_ID)pathID)) && (MP_H265Dec_getVideoObject((MP_VDODEC_ID)pathID)->GetInfo)) {
			(MP_H265Dec_getVideoObject((MP_VDODEC_ID)pathID)->GetInfo)(VDODEC_GET_MEM_SIZE, (UINT32 *)&memInfo, 0, 0);
		}
		codecSize = memInfo.size;
	} else if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264) {
		memInfo.width = uiWidth;
		memInfo.height = uiHeight;
		if ((MP_H264Dec_getVideoObject((MP_VDODEC_ID)pathID)) && (MP_H264Dec_getVideoObject((MP_VDODEC_ID)pathID)->GetInfo)) {
			(MP_H264Dec_getVideoObject((MP_VDODEC_ID)pathID)->GetInfo)(VDODEC_GET_MEM_SIZE, (UINT32 *)&memInfo, 0, 0);
		}
		codecSize = memInfo.size;
	} else {
		codecSize = 0;
	}

	return codecSize;
}

static UINT32 _NMP_VdoDec_GetBufSize(UINT32 pathID)
{
	UINT32 uiTotalBufSize;

#if _USE_COMMON_FOR_RAW
	gNMPVdoDec_RawBlkSize[pathID] = 0; // yuv use common buffer, so private raw size set to 0.
#else
	{
		MP_VDODEC_RECYUV_WH rec_yuv = {0};
		MP_VDODEC_GETINFO_TYPE type;
		type = gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG ? VDODEC_GET_JPEG_RECYUV_WH : VDODEC_GET_RECYUV_WH;
		rec_yuv.align_w = gNMPVdoDecConfig[pathID].uiWidth;
		rec_yuv.align_h = gNMPVdoDecConfig[pathID].uiHeight;
		(gNMPVdoDecObj[pathID].pMpVideoDecoder->GetInfo)(type, (UINT32 *)&rec_yuv, 0, 0);
		gNMPVdoDec_RawBlkSize[pathID] = rec_yuv.align_w * rec_yuv.align_h * 3 / 2;
	}
#endif

	// Total buffer size
	uiTotalBufSize = gNMPVdoDec_RawBlkSize[pathID] * NMP_VDODEC_RAW_BLOCK_NUMBER;

#if _I_FRM_COPY_TEST
	if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264 || gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
		uiTotalBufSize += IFRM_COPY_BUF_SIZE; // 2 MB for h26x I-frame copy test
	}
#endif

	VDODEC_DBG("[VDODEC][%d] GetBufSize: w=%d, h=%d, RawSize=0x%x, TotalBuf=0x%x\r\n",
			pathID,
			gNMPVdoDecConfig[pathID].uiWidth,
			gNMPVdoDecConfig[pathID].uiHeight,
			gNMPVdoDec_RawBlkSize[pathID],
			uiTotalBufSize);

	return uiTotalBufSize;
}

static ER _NMP_VdoDec_ConfigureObj(UINT32 pathID)
{
	ER						EResult = E_OK;
	//MP_VDODEC_DISPLAY_INFO	VdoPlayinfo = {0};
#if (!_USE_COMMON_FOR_RAW)
	UINT32					i, uiAddr, uiSize, uiRes;
#endif
	MP_VDODEC_INIT          mp_vdodec_init = {0};


	// move the code below to isf_vdodec SetPortParam::VDODEC_PARAM_CODEC
	/*switch (gNMPVdoDecConfig[pathID].uiVideoDecType) {
	case MEDIAPLAY_VIDEO_MJPG:
		//gNMPVdoDecObj[pathID].pMpVideoDecoder = MP_MjpgDec_getVideoDecodeObject();
		gNMPVdoDecObj[pathID].pMpVideoDecoder = MP_MjpgDec_getVideoObject((MP_VDODEC_ID)pathID);
		break;
	case MEDIAPLAY_VIDEO_H264:
		//gNMPVdoDecObj[pathID].pMpVideoDecoder = MP_H264Dec_getVideoDecodeObject();
		gNMPVdoDecObj[pathID].pMpVideoDecoder = MP_H264Dec_getVideoObject((MP_VDODEC_ID)pathID);
		break;
	case MEDIAPLAY_VIDEO_H265:
		//gNMPVdoDecObj[pathID].pMpVideoDecoder = MP_H265Dec_getVideoDecodeObject();
		gNMPVdoDecObj[pathID].pMpVideoDecoder = MP_H265Dec_getVideoObject((MP_VDODEC_ID)pathID);
		break;
	default:
		DBG_ERR("[%d] get mp object, unknown codec type = %d\n\r", pathID, gNMPVdoDecConfig[pathID].uiVideoDecType);
		//gNMPVdoDecObj[pathID].pMpVideoDecoder = NULL;
		//break;
		return E_SYS;
	}*/

	// Initialize & setup Media Plug-in Object
	//if (gNMPVdoDecObj[pathID].pMpVideoDecoder) {
	{
		// Set Info
		/*if (gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo) {
			// Set Decoder internal buffer address
			if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264 || gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
				(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_BUF_ADDR, gNMPVdoDecConfig[pathID].uiDecoderBufAddr, gNMPVdoDecConfig[pathID].uiDecoderBufSize, 0);
			}

#if _TODO // MP layer will use 15 as default value
			// Set GOP Number
			if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264) {
				(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_H264GOPNUM, gNMPVdoDecConfig[pathID].uiGOPNumber, 0,	0);
			} else if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
				(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_H265GOPNUM, gNMPVdoDecConfig[pathID].uiGOPNumber, 0,	0);
			}
#endif

			// Set Frame rate
			(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_FRAMERATE, gNMPVdoDecConfig[pathID].uiFrameRate, 0, 0);

			(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_OUTPUTRAWMODE, TRUE, 0, 0);
		}*/

		/*VdoPlayinfo.rawWidth = gNMPVdoDecConfig[pathID].uiWidth;
		VdoPlayinfo.rawHeight = gNMPVdoDecConfig[pathID].uiHeight;
		VdoPlayinfo.DescAddr = gNMPVdoDecConfig[pathID].uiDescAddr;
		VdoPlayinfo.DescLen = gNMPVdoDecConfig[pathID].uiDescSize;

		DBG_DUMP("[VDODEC][%d] mp init, first_pos=0x%x, w=%d, h=%d, raw_type=%d, raw_size=%d\r\n",
				pathID,
				VdoPlayinfo.firstFramePos,
				VdoPlayinfo.rawWidth,
				VdoPlayinfo.rawHeight,
				VdoPlayinfo.rawType,
				VdoPlayinfo.rawSize);

		EResult = (gNMPVdoDecObj[pathID].pMpVideoDecoder->Initialize)(0, &VdoPlayinfo);*/

		// config mp init
		mp_vdodec_init.desc_addr = gNMPVdoDecConfig[pathID].uiDescAddr;
		mp_vdodec_init.desc_size = gNMPVdoDecConfig[pathID].uiDescSize;
		mp_vdodec_init.buf_addr = gNMPVdoDecConfig[pathID].uiDecoderBufAddr;
		mp_vdodec_init.buf_size = gNMPVdoDecConfig[pathID].uiDecoderBufSize;
		mp_vdodec_init.width = gNMPVdoDecConfig[pathID].uiWidth;
		mp_vdodec_init.height = gNMPVdoDecConfig[pathID].uiHeight;
		mp_vdodec_init.display_width = mp_vdodec_init.width;

		EResult = (gNMPVdoDecObj[pathID].pMpVideoDecoder->Initialize)(&mp_vdodec_init);

		// get h26x parsing result
		if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264 || gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
			gNMPVdoDecConfig[pathID].uiImageWidth = mp_vdodec_init.width;
			gNMPVdoDecConfig[pathID].uiImageHeight = mp_vdodec_init.height;
			gNMPVdoDecConfig[pathID].uiYLineOffset = mp_vdodec_init.uiYLineoffset;
			gNMPVdoDecConfig[pathID].uiUVLineOffset = mp_vdodec_init.uiUVLineoffset;
		}

		if (EResult != E_OK) {
			DBG_ERR("Media Plug-in initialize is FAILED!!\n\r");
			return EResult;
		}
	} /*else {
		DBG_ERR("[%d] Video Decoder is NULL, please set codec first!!\r\n", pathID);
		return E_SYS;
	}*/
#ifdef VDODEC_LL
	gNMPVdoDecObj[pathID].Dec_CallBack.callback = NMP_VdoDec_WrapRaw;
#endif
	// Assign RAW Data buffer
#if (!_USE_COMMON_FOR_RAW)
	if (gNMPVdoDecConfig[pathID].uiRawStartAddr) {
		MP_VDODEC_RECYUV_WH rec_yuv = {0};
		MP_VDODEC_GETINFO_TYPE type;
		uiAddr = gNMPVdoDecConfig[pathID].uiRawStartAddr;
		uiSize = gNMPVdoDec_RawBlkSize[pathID];
		uiRes = (uiSize & 0x3);
		uiSize = (uiRes) ? (uiSize + 4 - uiRes) : uiSize;

		for (i = 0; i < NMP_VDODEC_RAW_BLOCK_NUMBER; i++) {
			gNMPVdoDec_RawAddr[pathID][i].Y_Addr = uiAddr;
			rec_yuv.align_w = gNMPVdoDecConfig[pathID].uiWidth;
			rec_yuv.align_h = gNMPVdoDecConfig[pathID].uiHeight;
			type = gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG ? VDODEC_GET_JPEG_RECYUV_WH : VDODEC_GET_RECYUV_WH;
			(gNMPVdoDecObj[pathID].pMpVideoDecoder->GetInfo)(type, (UINT32 *)&rec_yuv, 0, 0);
			gNMPVdoDec_RawAddr[pathID][i].UV_Addr = gNMPVdoDec_RawAddr[pathID][i].Y_Addr + (rec_yuv.align_w * rec_yuv.align_h);
			uiAddr += uiSize;
		}
	} else {
		DBG_ERR("RAW Data Buffer Start Address is NULL\n\r");
		return E_SYS;
	}
#endif

	//memset(&(gNMPVdoDecObj[pathID].BsQueue), 0 , sizeof(NMP_VDODEC_BSQ));

	// Initial the variables
#if (!_USE_COMMON_FOR_RAW)
	gNMPVdoDecRawQueIdx[pathID] = 0;
#endif
	//gNMPVdoDecFrameNum[pathID] = 0;

	return EResult;
}

static ER _NMP_VdoDec_TskStop(UINT32 pathID)
{
	FLGPTN uiFlag;

	VDODEC_DBG("[VDODEC]tskstop wait idle ..\r\n");
	wai_flg(&uiFlag, FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_IDLE, TWF_ORW);
	VDODEC_DBG("[VDODEC]tskstop wait idle OK\r\n");

	// close decoder
	/*if (gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo) {
		(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_CLOSEBUS, 0, 0, 0);
	} else {
		DBG_ERR("MP decoder is NULL\r\n");
	}*/

//#if __KERNEL__
#if 1
	set_flg(FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_STOP);
	wai_flg(&uiFlag, FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_STOP_DONE, TWF_ORW | TWF_CLR);
#endif
	// Terminate Task
	//THREAD_DESTROY(NMP_VDODEC_TSK_ID); //ter_tsk(NMP_VDODEC_TSK_ID);

	return E_OK;
}

static UINT32 _NMP_VdoDec_HowManyInQ(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	PNMP_VDODEC_BSQ pObj;

	SEM_WAIT(NMP_VDODEC_SEM_ID[pathID]); //wai_sem(NMP_VDODEC_SEM_ID[pathID]);
	pObj = &gNMPVdoDecObj[pathID].BsQueue;
	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	SEM_SIGNAL(NMP_VDODEC_SEM_ID[pathID]); //sig_sem(NMP_VDODEC_SEM_ID[pathID]);
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = NMP_VDODEC_BSQ_MAX - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = NMP_VDODEC_BSQ_MAX;
	} else {
		sq = 0;
	}

	VDODEC_DBG("%s:(line%d) pathID=%d, sq=%d\r\n", __func__, __LINE__, pathID, sq);

	return sq;
}

static void _NMP_VdoDec_InitQ(UINT32 pathID)
{
	memset(&(gNMPVdoDecObj[pathID].BsQueue), 0 , sizeof(NMP_VDODEC_BSQ));
}

/*static ER _NMP_VdoDec_PutQ(UINT32 pathID, PNMI_VDODEC_BS_SRC pBsSrc)
{
	PNMP_VDODEC_BSQ pBsQueue;
	PNMI_VDODEC_BS_SRC pBsQueSrc;

	pBsQueue = &gNMPVdoDecObj[pathID].BsQueue;

	if ((pBsQueue->Front == pBsQueue->Rear) && (pBsQueue->bFull == TRUE)) {
		DBG_ERR("NMP Video Decoder BS Queue is Full!\n\r");
		return FALSE;
	} else {
		pBsQueue->Rear = (pBsQueue->Rear + 1) % NMP_VDODEC_BS_QUEUE_SIZE;

		pBsQueSrc = &(pBsQueue->Queue[pBsQueue->Rear]);

		if (pBsSrc) {
			memcpy((VOID *)pBsQueSrc, (VOID *)pBsSrc, sizeof(NMI_VDODEC_BS_SRC));
		}

		if (pBsQueue->Front == pBsQueue->Rear) { // Check Queue full
			pBsQueue->bFull = TRUE;
		}

		return TRUE;
	}
}*/
static ER _NMP_VdoDec_PutQ(UINT32 pathID, NMI_VDODEC_BS_INFO *pBsInfo)
{
	NMP_VDODEC_BSQ *pObj;

	SEM_WAIT(NMP_VDODEC_SEM_ID[pathID]);

	pObj = &(gNMPVdoDecObj[pathID].BsQueue);

	VDODEC_DBG("%s:(line%d) pathID=%d, f=%d, r=%d, full=%d\r\n", __func__, __LINE__, pathID, pObj->Front, pObj->Rear, pObj->bFull);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		SEM_SIGNAL(NMP_VDODEC_SEM_ID[pathID]);
		DBG_ERR("[%d] add bs queue full!\r\n", pathID);
		return E_SYS;
	} else {
		pObj->Queue[pObj->Rear].uiBSAddr = pBsInfo->uiBSAddr;
		pObj->Queue[pObj->Rear].uiBSSize = pBsInfo->uiBSSize;
		pObj->Queue[pObj->Rear].uiRawAddr = pBsInfo->uiRawAddr;
		pObj->Queue[pObj->Rear].uiRawSize = pBsInfo->uiRawSize;
		//pObj->Queue[pObj->Rear].BufID = pBsInfo->BufID;
		pObj->Queue[pObj->Rear].BsBufID = pBsInfo->BsBufID;
		pObj->Queue[pObj->Rear].RawBufID = pBsInfo->RawBufID;
		pObj->Queue[pObj->Rear].uiThisFrmIdx = pBsInfo->uiThisFrmIdx;
		pObj->Queue[pObj->Rear].bIsEOF = pBsInfo->bIsEOF;
		pObj->Queue[pObj->Rear].TimeStamp = pBsInfo->TimeStamp;
		pObj->Queue[pObj->Rear].uiCommBufBlkID = pBsInfo->uiCommBufBlkID;
		pObj->Queue[pObj->Rear].uiBSTotalSizeOneFrm = pBsInfo->uiBSTotalSizeOneFrm;
		pObj->Queue[pObj->Rear].WaitMs = pBsInfo->WaitMs;
		pObj->Rear = (pObj->Rear + 1) % NMP_VDODEC_BSQ_MAX;
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		SEM_SIGNAL(NMP_VDODEC_SEM_ID[pathID]);
		return E_OK;
	}
}

/*static BOOL _NMP_VdoDec_GetQ(UINT32 pathID, PNMI_VDODEC_BS_SRC pBsSrc)
{
	PNMP_VDODEC_BSQ pBsQueue;
	PNMI_VDODEC_BS_SRC pBsQueSrc;

	pBsQueue = &gNMPVdoDecObj[pathID].BsQueue;

	if ((pBsQueue->Front == pBsQueue->Rear) && (pBsQueue->bFull == FALSE)) {
		DBG_ERR("NMP Video Decoder BS Queue is Empty!\n\r");
		return FALSE;
	} else {
		pBsQueue->Front = (pBsQueue->Front + 1) % NMP_VDODEC_BS_QUEUE_SIZE;

		pBsQueSrc = &(pBsQueue->Queue[pBsQueue->Front]);

		if (pBsSrc) {
			memcpy((VOID *)pBsSrc, (VOID *)pBsQueSrc, sizeof(NMI_VDODEC_BS_SRC));
		}

		if (pBsQueue->Front == pBsQueue->Rear) { // Check Queue empty
			pBsQueue->bFull = FALSE;
		}

		return TRUE;
	}
}*/
static ER _NMP_VdoDec_GetQ(UINT32 pathID, NMI_VDODEC_BS_INFO *pBsInfo)
{
	NMP_VDODEC_BSQ  *pObj;

	SEM_WAIT(NMP_VDODEC_SEM_ID[pathID]);

	pObj = &(gNMPVdoDecObj[pathID].BsQueue);

	VDODEC_DBG("%s:(line%d) pathID=%d, f=%d, r=%d, full=%d\r\n", __func__, __LINE__, pathID, pObj->Front, pObj->Rear, pObj->bFull);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(NMP_VDODEC_SEM_ID[pathID]);
		DBG_ERR("[%d] get bs queue empty!\r\n", pathID);
		return E_SYS;
	} else {
		pBsInfo->uiBSAddr = pObj->Queue[pObj->Front].uiBSAddr;
		pBsInfo->uiBSSize = pObj->Queue[pObj->Front].uiBSSize;
		pBsInfo->uiRawAddr = pObj->Queue[pObj->Front].uiRawAddr;
		pBsInfo->uiRawSize = pObj->Queue[pObj->Front].uiRawSize;
		//pBsInfo->BufID = pObj->Queue[pObj->Front].BufID;
		pBsInfo->BsBufID = pObj->Queue[pObj->Front].BsBufID;
		pBsInfo->RawBufID = pObj->Queue[pObj->Front].RawBufID;
		pBsInfo->uiThisFrmIdx = pObj->Queue[pObj->Front].uiThisFrmIdx;
		pBsInfo->bIsEOF = pObj->Queue[pObj->Front].bIsEOF;
		pBsInfo->TimeStamp = pObj->Queue[pObj->Front].TimeStamp;
		pBsInfo->uiCommBufBlkID = pObj->Queue[pObj->Front].uiCommBufBlkID;
		pBsInfo->uiBSTotalSizeOneFrm = pObj->Queue[pObj->Front].uiBSTotalSizeOneFrm;
		pBsInfo->WaitMs = pObj->Queue[pObj->Front].WaitMs;
		pObj->Front = (pObj->Front + 1) % NMP_VDODEC_BSQ_MAX;
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}
		SEM_SIGNAL(NMP_VDODEC_SEM_ID[pathID]);
		return E_OK;
	}
}

static void _NMP_VdoDec_WaitTskIdle(void)
{
	FLGPTN uiFlag;

	// wait for video task idle
	wai_flg(&uiFlag, FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_IDLE, TWF_ORW);

//#if __KERNEL__
//#if 1
//	set_flg(FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_STOP);
//#endif

#if MP_VDODEC_SHOW_MSG
	DBG_DUMP("[VDODEC] wait idle finished\r\n");
#endif
}

static ER _NMP_VdoDec_Start(UINT32 pathID)
{

	VDODEC_DBG("%s:(line%d) pathID=%d\r\n", __func__, __LINE__, pathID);

	if (_NMP_VdoDec_ConfigureObj(pathID) != E_OK) {
		DBG_ERR("[VDODEC][%d] config obj fail\r\n", pathID);
		return E_SYS;
	}

	_NMP_VdoDec_InitQ(pathID);

	return E_OK;
}

static void _NMP_VdoDec_Stop(UINT32 pathID)
{
	NMI_VDODEC_BS_INFO			BsSrc	= {0};
	NMI_VDODEC_RAW_INFO			VdoRawInfo = {0};

	VDODEC_DBG("%s:(line%d) pathID=%d\r\n", __func__, __LINE__, pathID);

	while (_NMP_VdoDec_HowManyInQ(pathID)) {
		_NMP_VdoDec_GetQ(pathID, &BsSrc);
		//gNMPVdoDecObj[pathID].BsQueue.bFull = 0;
		//gNMPVdoDecObj[pathID].BsQueue.Rear  = 0;
		//gNMPVdoDecObj[pathID].BsQueue.Front = 0;

		// release remain user blk
		VdoRawInfo.PathID = pathID;
		VdoRawInfo.isStop = TRUE;
		VdoRawInfo.RawBufID = BsSrc.RawBufID;
		VdoRawInfo.uiCommBufBlkID= BsSrc.uiCommBufBlkID;
		if (gNMPVdoDecObj[pathID].CallBackFunc) {
			(gNMPVdoDecObj[pathID].CallBackFunc)(NMI_VDODEC_EVENT_RAW_READY, (UINT32) &VdoRawInfo);
		}
	}

	//decoder_initialized[pathID] = FALSE;

#if _I_FRM_COPY_TEST
	ifrm_copy_start[pathID] = 0;
	ifrm_copy_end[pathID] = 0;
	ifrm_copy_size[pathID] = 0;
	ifrm_copy_curr[pathID] = 0;
#endif
}

static ER _NMP_VdoDec_GetDescLen(UINT32 codec, UINT32 addr, UINT32 size, UINT32 *p_desc_len)
{
	ER r = E_OK;
	UINT32 start_code = 0, count = 0;
	UINT8 *ptr8 = NULL;

	if (!addr) {
		DBG_ERR("addr is null\r\n");
		return E_SYS;
	}
	if (!size) {
		DBG_ERR("size is null\r\n");
		return E_SYS;
	}
	if (!p_desc_len) {
		DBG_ERR("p_out is null\r\n");
		return E_SYS;
	}

	ptr8 = (UINT8 *)addr;
	count = NMI_VDODEC_H26X_NAL_MAXSIZE; // max desc len

	if (codec == MEDIAPLAY_VIDEO_H264) {
		while (count--) { //while (size--) {
			// search start code to skip (sps, pps)
			if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 2) == 0x00) && (*(ptr8 + 3) == 0x01)) {
	            start_code++;
			}
			if (start_code == 3) {
				if ((*(ptr8 + 4) & 0x1F) == 5) { // type: b'00101 = 5 (IDR)
					// calc desc size
					*p_desc_len = (UINT32)ptr8 - addr; // dst_addr - src_addr
					// calc frm size without desc
					//p_out->size = size - desc_size;
					r = E_OK;
					break;
				} else {
					//p_desc_len = 0;
					DBG_ERR("h264 can't find IDR\r\n");
	                r = E_SYS;
					break;
				}
			}
			ptr8++;
		}
	} else if (codec == MEDIAPLAY_VIDEO_H265) {
		while (count--) {
			// search start code to skip (vps, sps, pps)
			if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 2) == 0x00) && (*(ptr8 + 3) == 0x01)) {
	            start_code++;
			}
			if (start_code == 4) {
				if ((*(ptr8 + 4) & 0x7E) >> 1 == 19) { // type: b'010011 = 19 (IDR)
					// calc desc size
					*p_desc_len = (UINT32)ptr8 - addr; // dst_addr - src_addr
					// calc frm size without desc
					//p_out->size = size - desc_size;
					r = E_OK;
					break;
				} else {
					//p_desc_len = 0;
					DBG_ERR("h265 can't find IDR\r\n");
	                r = E_SYS;
					break;
				}
			}
			ptr8++;
		}
	} else {
		DBG_ERR("unknown codec (%d)\r\n", codec);
		return E_SYS;
	}

	return r;
}
#ifdef VDODEC_LL
UINT32 _NMP_VdoDec_yuv_kdrv2kflow(UINT32 yuv_fmt)
{
	switch (yuv_fmt)
	{
		case KDRV_VDODEC_YUV420:    return VDO_PXLFMT_YUV420;
		case KDRV_VDODEC_YUV422:    return VDO_PXLFMT_YUV422;
		default:
			DBG_ERR("unknown yuv format(%d)\r\n", yuv_fmt);
			return (0xFFFFFFFF);
	}
}

static INT32 NMP_VdoDec_WrapRaw(void *info, void *usr_data)
{
	NMI_VDODEC_RAW_INFO			*pUserData;
	UINT32						pathID = 0;

	MP_VDODEC_PARAM 			*p_vdodec_param;
	p_vdodec_param = (MP_VDODEC_PARAM *)info;
	pUserData = (NMI_VDODEC_RAW_INFO *)usr_data;
	pathID = pUserData->PathID;

	NMP_VdoDec_PutJob_WrapRAW(pathID);
	NMP_VdoDec_TrigWrapRAW();

	return 0;
}

static INT32 NMP_VdoDec_TrigAndWrapRawInfo(UINT32 pathID)
{
	NMI_VDODEC_RAW_INFO         *VdoRawInfo;
	MP_VDODEC_PARAM             *p_vdodec_param;
	ER                          ER_Code;

	VdoRawInfo = &(gNMPVdoDecObj[pathID].VdoRawInfo);
	p_vdodec_param = &(gNMPVdoDecObj[pathID].mp_vdodec_param);

	ER_Code = p_vdodec_param->errorcode == 0 ? E_OK : E_SYS;
	gNMPVdoDecObj[pathID].mp_vdodec_param.errorcode = ER_Code;

	if (gbNMPVdoDecPartial[pathID]) {
		if (gNMPVdoDecInterrupt[pathID] == 0x10) {
			gNMPVdoDecObj[pathID].bReleaseBsOnly = TRUE;
		} else if (gNMPVdoDecInterrupt[pathID] == 0x1) {
			gNMPVdoDecObj[pathID].bReleaseBsOnly = FALSE;
		}
	} else {
		if (ER_Code != E_OK) {
			DBG_ERR("[VDODEC][%d] Decode ERROR !! i = %llu\r\n", pathID, gNMPVdoDecObj[pathID].BsSrc.uiThisFrmIdx);
			if (gNMPVdoDecConfig[pathID].uiVideoDecType != MEDIAPLAY_VIDEO_MJPG) {
				if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264) {
					UINT32 uiRawYPhyAddr = nvtmpp_sys_va2pa(gNMPVdoDecObj[pathID].mp_vdodec_param.y_addr);
					gNMPVdoDecObj[pathID].vRefFrmCb.VdoDec_RefFrmDo(pathID, uiRawYPhyAddr, FALSE);
				} else {
					gNMPVdoDecObj[pathID].vRefFrmCb.VdoDec_RefFrmDo(pathID, gNMPVdoDecObj[pathID].mp_vdodec_param.y_addr, FALSE);
				}
			}
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
		}
	}

	// Update Frame Number
	gNMPVdoDecObj[pathID].uiDecFrmNum = gNMPVdoDecObj[pathID].BsSrc.uiThisFrmIdx;

	if (gNMPVdoDecObj[pathID].BsSrc.bIsEOF) {
		VDODEC_DBG("Decode last frame Index = %llu\r\n", gNMPVdoDecObj[pathID].BsSrc.uiThisFrmIdx);
	}

	// Set RAW output
	VdoRawInfo->PathID		= pathID;
	VdoRawInfo->BsBufID		= gNMPVdoDecObj[pathID].BsSrc.BsBufID;
	VdoRawInfo->RawBufID 	= gNMPVdoDecObj[pathID].BsSrc.RawBufID;
	VdoRawInfo->uiRawAddr	= gNMPVdoDecObj[pathID].uiYAddr;
	VdoRawInfo->uiRawSize	= gNMPVdoDecObj[pathID].uiRawSize;
	VdoRawInfo->uiYAddr		= gNMPVdoDecObj[pathID].uiYAddr;
	VdoRawInfo->uiUVAddr 	= gNMPVdoDecObj[pathID].uiUVAddr;

	if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
		VdoRawInfo->uiWidth        = gNMPVdoDecObj[pathID].mp_vdodec_param.uiWidth;
		VdoRawInfo->uiHeight       = gNMPVdoDecObj[pathID].mp_vdodec_param.uiHeight;
		VdoRawInfo->uiYLineOffset  = VdoRawInfo->uiWidth;
		VdoRawInfo->uiUVLineOffset = VdoRawInfo->uiWidth;
		VdoRawInfo->uiYuvFmt       = gNMPVdoDecObj[pathID].mp_vdodec_param.yuv_fmt;
		VdoRawInfo->uiYuvFmt       = _NMP_VdoDec_yuv_kdrv2kflow(VdoRawInfo->uiYuvFmt);
	} else {
		VdoRawInfo->uiWidth        = gNMPVdoDecConfig[pathID].uiImageWidth;
		VdoRawInfo->uiHeight       = gNMPVdoDecConfig[pathID].uiImageHeight;
		VdoRawInfo->uiYLineOffset  = gNMPVdoDecConfig[pathID].uiYLineOffset;
		VdoRawInfo->uiUVLineOffset = gNMPVdoDecConfig[pathID].uiUVLineOffset;
		VdoRawInfo->uiYuvFmt       = VDO_PXLFMT_YUV420;
	}
	VdoRawInfo->uiThisFrmIdx = gNMPVdoDecObj[pathID].BsSrc.uiThisFrmIdx;
	VdoRawInfo->bIsEOF		= gNMPVdoDecObj[pathID].BsSrc.bIsEOF;
	VdoRawInfo->Occupied 	= 0; // maybe vdodec doesn't need this ?
	VdoRawInfo->TimeStamp	= gNMPVdoDecObj[pathID].BsSrc.TimeStamp;
	VdoRawInfo->dec_er		= gNMPVdoDecObj[pathID].mp_vdodec_param.errorcode;
	VdoRawInfo->uiCommBufBlkID = gNMPVdoDecObj[pathID].BsSrc.uiCommBufBlkID;
	VdoRawInfo->isStop = FALSE;
	VdoRawInfo->bReleaseBsOnly = gNMPVdoDecObj[pathID].bReleaseBsOnly;

#if NMP_VDODEC_DUMP_DEC_FRAME
	static BOOL bFirst = TRUE;
	static FST_FILE fp = NULL;
	if (bFirst) {
		char sFileName[260];
		DBG_DUMP("[VDODEC][%d] dump decoded yuv frame, w = %d, h = %d, raw addr = 0x%08x, raw size = %d\r\n", pathID, gNMPVdoDecConfig[pathID].uiWidth, gNMPVdoDecConfig[pathID].uiHeight, gNMPVdoDecObj[pathID].uiYAddr, gNMPVdoDecObj[pathID].uiRawSize);
		snprintf(sFileName, sizeof(sFileName), "A:\\%d_%d.yuv", gNMPVdoDecConfig[pathID].uiWidth, gNMPVdoDecConfig[pathID].uiHeight);
		fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fp != NULL) {
			if (FileSys_WriteFile(fp, (UINT8 *)gNMPVdoDecObj[pathID].uiYAddr, &(gNMPVdoDecObj[pathID].uiRawSize), 0, NULL) != FST_STA_OK) {
				DBG_ERR("[VDODEC][%d] dump decoded yuv frame err, write file fail\r\n", pathID);
			} else {
				DBG_DUMP("[VDODEC][%d] write file ok\r\n", pathID);
			}
			FileSys_CloseFile(fp);
		} else {
			DBG_ERR("[VDODEC][%d] dump decoded yuv frame err, invalid file handle\r\n", pathID);
		}

		bFirst = FALSE;
	}
#endif

	// Callback output raw data
	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_IN_PROBE_PROC,  ISF_OK); // [IN]	PROC_OK
	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_OK); // [OUT] PROC_OK

	// callback to isf_vdodec
	if (gNMPVdoDecObj[pathID].CallBackFunc) {
		(gNMPVdoDecObj[pathID].CallBackFunc)(NMI_VDODEC_EVENT_RAW_READY, (UINT32) VdoRawInfo);
	}

	// Update RAW Index
#if (!_USE_COMMON_FOR_RAW)
	gNMPVdoDecRawQueIdx[pathID]++;
	gNMPVdoDecRawQueIdx[pathID] %= NMP_VDODEC_RAW_BLOCK_NUMBER;
#endif
	//gNMPVdoDecFrameNum[pathID]++;
	SEM_SIGNAL(NMP_VDODEC_TRIG_SEM_ID[pathID]);

	return E_OK;
}

static ER _NMP_VdoDec_Decode(UINT32 pathID)
{
	ER					r = E_OK;
	UINT32					uiBSAddr = 0, uiBSSize = 0;
	NMI_VDODEC_BSNXT_INFO			BsNextSrc = {0};
	MP_VDODEC_RECYUV_WH			rec_yuv = {0};
	MP_VDODEC_GETINFO_TYPE			type;
	UINT32					desc_len = 0;
	UINT8					 *ptrbs;

	SEM_WAIT(NMP_VDODEC_TRIG_SEM_ID[pathID]);

	VDODEC_DBG("%s:(line%d) pathID=%d\r\n", __func__, __LINE__, pathID);

	// Get BS Stream Set
	//if (!_NMP_VdoDec_GetQ(pathID, &BsSrc)) {
	if (_NMP_VdoDec_GetQ(pathID, &(gNMPVdoDecObj[pathID].BsSrc)) != E_OK) {
		SEM_SIGNAL(NMP_VDODEC_TRIG_SEM_ID[pathID]);
		DBG_ERR("Get BS Data is FAILED!!\r\n");
		return E_SYS;
	}

	ptrbs = (UINT8 *)gNMPVdoDecObj[pathID].BsSrc.uiBSAddr;

	if (*(ptrbs) == 0x0 && *(ptrbs+1) == 0x0 && *(ptrbs+2) == 0x0 && *(ptrbs+3) == 0x1) {
		gbNMPVdoDecHeaderInBs[pathID] = TRUE;
	} else {
		gbNMPVdoDecHeaderInBs[pathID] = FALSE;
	}

	if (gNMPVdoDecObj[pathID].BsSrc.WaitMs == -99) {
		gbNMPVdoDecPartial[pathID] = TRUE;
	} else {
		gbNMPVdoDecPartial[pathID] = FALSE;
	}

	// lock video frame for H264 and H265, do unlock in kdrv if not be reference frame or decode error
	if (gbNMPVdoDecPartial[pathID]) {
		if (gbNMPVdoDecHeaderInBs[pathID]) {
			if (gNMPVdoDecConfig[pathID].uiVideoDecType != MEDIAPLAY_VIDEO_MJPG) {
				//gNMPVdoDecObj[pathID].vRefFrmCb.VdoDec_RefFrmDo(pathID, BsSrc.uiRawAddr, TRUE);
				// todo, ltr scheme
			}
		}
	} else {
		if (gNMPVdoDecConfig[pathID].uiVideoDecType != MEDIAPLAY_VIDEO_MJPG) {
			gNMPVdoDecObj[pathID].vRefFrmCb.VdoDec_RefFrmDo(pathID, gNMPVdoDecObj[pathID].BsSrc.uiRawAddr, TRUE);
		}
	}
#if _USE_COMMON_FOR_RAW
	// use common buffer for raw
	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_ENTER); // [OUT] NEW_ENTER
	gNMPVdoDecObj[pathID].uiYAddr = gNMPVdoDecObj[pathID].BsSrc.uiRawAddr;
	rec_yuv.align_w = gNMPVdoDecConfig[pathID].uiWidth;
	rec_yuv.align_h = gNMPVdoDecConfig[pathID].uiHeight;
	type = gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG ? VDODEC_GET_JPEG_RECYUV_WH : VDODEC_GET_RECYUV_WH;
	(gNMPVdoDecObj[pathID].pMpVideoDecoder->GetInfo)(type, (UINT32 *)&rec_yuv, 0, 0);
	gNMPVdoDecObj[pathID].uiUVAddr = gNMPVdoDecObj[pathID].uiYAddr + (rec_yuv.align_w * rec_yuv.align_h);
	gNMPVdoDecObj[pathID].uiRawSize = gNMPVdoDecObj[pathID].BsSrc.uiRawSize;
#else
	// use private buffer for raw
	gNMPVdoDecObj[pathID].uiYAddr = gNMPVdoDec_RawAddr[pathID][gNMPVdoDecRawQueIdx[pathID]].Y_Addr;
	gNMPVdoDecObj[pathID].uiUVAddr = gNMPVdoDec_RawAddr[pathID][gNMPVdoDecRawQueIdx[pathID]].UV_Addr;
	gNMPVdoDecObj[pathID].uiRawSize = gNMPVdoDec_RawBlkSize[pathID];
#endif

	if (!gNMPVdoDecObj[pathID].uiYAddr || !gNMPVdoDecObj[pathID].uiUVAddr || !gNMPVdoDecObj[pathID].uiRawSize) {
		DBG_ERR("[%d] invalid raw addr, Y(0x%x) UV(0x%x) size(%d)\r\n", pathID, gNMPVdoDecObj[pathID].uiYAddr, gNMPVdoDecObj[pathID].uiUVAddr, gNMPVdoDecObj[pathID].uiRawSize);
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_ERR, ISF_ERR_QUEUE_ERROR); // [OUT] NEW_ERR
		SEM_SIGNAL(NMP_VDODEC_TRIG_SEM_ID[pathID]);
		return E_SYS;
	} else {
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_OK); // [OUT] NEW_OK
	}

	/*if (gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo) {
		(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_RAWDATAADDR, uiYAddr, uiUVAddr, 0);
	}*/

	// h26x desc filtering, remove -> user AP should handle this by itself
	if (0) {//(gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264 || gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
		if ((r = _NMP_VdoDec_GetDescLen(gNMPVdoDecConfig[pathID].uiVideoDecType, gNMPVdoDecObj[pathID].BsSrc.uiBSAddr, gNMPVdoDecObj[pathID].BsSrc.uiBSSize, &desc_len)) != E_OK) {
			SEM_SIGNAL(NMP_VDODEC_TRIG_SEM_ID[pathID]);
			return r;
		}
		if (desc_len) { // i-frame (skip desc)
			uiBSAddr = gNMPVdoDecObj[pathID].BsSrc.uiBSAddr + desc_len;
			uiBSSize = gNMPVdoDecObj[pathID].BsSrc.uiBSSize - desc_len;
		} else { // p-frame
			uiBSAddr = gNMPVdoDecObj[pathID].BsSrc.uiBSAddr;
			uiBSSize = gNMPVdoDecObj[pathID].BsSrc.uiBSSize;
		}
	} else {
		uiBSAddr = gNMPVdoDecObj[pathID].BsSrc.uiBSAddr;
		uiBSSize = gNMPVdoDecObj[pathID].BsSrc.uiBSSize;
	}

	// addr need word align -> copy to tmp buf
#if _I_FRM_COPY_TEST
	if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264 || gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
		if (!ifrm_copy_start[pathID] || !ifrm_copy_size[pathID]) {
			SEM_SIGNAL(NMP_VDODEC_TRIG_SEM_ID[pathID]);
			DBG_ERR("ifrm copy buf a=0x%x, s=%d\r\n", ifrm_copy_start[pathID], ifrm_copy_size[pathID]);
			return E_SYS;
		}

		if ((ifrm_copy_curr[pathID] + uiBSSize) > ifrm_copy_end[pathID]) {
			ifrm_copy_curr[pathID] = ifrm_copy_start[pathID]; // rollback
		}

		DBG_DUMP("[%d] copy 0x%x to 0x%x, size: %d\r\n", pathID, uiBSAddr, ifrm_copy_curr[pathID], uiBSSize);
		memcpy((VOID *)ifrm_copy_curr[pathID], (VOID *)uiBSAddr, uiBSSize);

		gNMPVdoDecObj[pathID].mp_vdodec_param.bs_addr = ifrm_copy_curr[pathID];

		// update current
		ifrm_copy_curr[pathID] += ALIGN_CEIL_4(uiBSSize);
	} else {
		gNMPVdoDecObj[pathID].mp_vdodec_param.bs_addr = uiBSAddr; // jpeg do not use copy buf
	}
#else
	// config mp param
	gNMPVdoDecObj[pathID].mp_vdodec_param.bs_addr = uiBSAddr;
#endif
	if (gbNMPVdoDecPartial[pathID] && gbNMPVdoDecHeaderInBs[pathID]) {
		gNMPVdoDecObj[pathID].mp_vdodec_param.bs_size = 0x800000;
		gNMPVdoDecObj[pathID].mp_vdodec_param.cur_bs_size = gNMPVdoDecObj[pathID].BsSrc.uiBSSize;
	} else {
		gNMPVdoDecObj[pathID].mp_vdodec_param.bs_size = uiBSSize;
	}
	gNMPVdoDecObj[pathID].mp_vdodec_param.y_addr = gNMPVdoDecObj[pathID].uiYAddr;
	gNMPVdoDecObj[pathID].mp_vdodec_param.c_addr = gNMPVdoDecObj[pathID].uiUVAddr;
	gNMPVdoDecObj[pathID].mp_vdodec_param.vRefFrmCb = gNMPVdoDecObj[pathID].vRefFrmCb;

	// Callback current used bs addr & size ---> remove, only for carDV
	/*if (gNMPVdoDecObj[pathID].CallBackFunc && (uiBSAddr > 0) && (uiBSSize > 0)) {
		MEM_RANGE cur_bs;
		cur_bs.addr = uiBSAddr;
		cur_bs.size = uiBSSize;
		(gNMPVdoDecObj[pathID].CallBackFunc)(NMI_VDODEC_EVENT_CUR_VDOBS, (UINT32) &cur_bs);
	}*/


	if (gNMPVdoDecObj[pathID].uiMsgLevel & NMP_VDODEC_MSG_INPUT) {
		UINT8 *ptr = (UINT8 *)gNMPVdoDecObj[pathID].mp_vdodec_param.bs_addr;
		DBG_DUMP("[VDODEC][%d] i=%llu, Addr=0x%x, Size=%d\r\n", pathID, gNMPVdoDecObj[pathID].BsSrc.uiThisFrmIdx, gNMPVdoDecObj[pathID].mp_vdodec_param.bs_addr, uiBSSize);

		DBG_DUMP("bs = ");
		DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
				*ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7), *(ptr+8), *(ptr+9));

		DBG_DUMP("\r\n...\r\n");

		DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
				*(ptr+uiBSSize-10), *(ptr+uiBSSize-9), *(ptr+uiBSSize-8), *(ptr+uiBSSize-7), *(ptr+uiBSSize-6), *(ptr+uiBSSize-5), *(ptr+uiBSSize-4), *(ptr+uiBSSize-3), *(ptr+uiBSSize-2), *(ptr+uiBSSize-1));

		DBG_DUMP("\r\n");
	}

	if (uiBSSize && gNMPVdoDecObj[pathID].pMpVideoDecoder) {
		// Decode Video
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_IN_PROBE_PUSH, ISF_OK);	  // [IN]  PUSH_OK
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_IN_PROBE_PROC, ISF_ENTER);  // [IN]  PROC_ENTER
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_ENTER); // [OUT] PROC_ENTER

		if (gbNMPVdoDecPartial[pathID] && !gbNMPVdoDecHeaderInBs[pathID]) {
			BsNextSrc.uiBSNXTAddr = gNMPVdoDecObj[pathID].mp_vdodec_param.bs_addr;
			BsNextSrc.uiBSNXTSize = gNMPVdoDecObj[pathID].mp_vdodec_param.bs_size;
			BsNextSrc.uiTotalBsSize = gNMPVdoDecObj[pathID].BsSrc.uiBSTotalSizeOneFrm;
			r = (gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(VDODEC_SET_NXT_BS, (UINT32)&BsNextSrc, 0, 0);
			if (r != E_OK) {
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
			}

			r = (gNMPVdoDecObj[pathID].pMpVideoDecoder->GetInfo)(VDODEC_GET_H26X_INTERRUPT, (UINT32 *)&gNMPVdoDecInterrupt[pathID], 0, 0);
			if (r != E_OK) {
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
			}

			if (gNMPVdoDecInterrupt[pathID] != 0x1 && gNMPVdoDecInterrupt[pathID] != 0x10) {
				gNMPVdoDecObj[pathID].mp_vdodec_param.errorcode = E_SYS;
			}
		} else {
			gNMPVdoDecObj[pathID].VdoRawInfo.PathID = pathID;
			r = (gNMPVdoDecObj[pathID].pMpVideoDecoder->DecodeOne)(0, &(gNMPVdoDecObj[pathID].mp_vdodec_param), &(gNMPVdoDecObj[pathID].Dec_CallBack), &(gNMPVdoDecObj[pathID].VdoRawInfo));
			gNMPVdoDecInterrupt[pathID] = gNMPVdoDecObj[pathID].mp_vdodec_param.interrupt;
		}
	} else {
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INCOMPLETE_DATA);	   // [IN] PUSH_ERR
		SEM_SIGNAL(NMP_VDODEC_TRIG_SEM_ID[pathID]);
		DBG_ERR("NMP Video Decoder BS Size is 0!\n\r");
		return E_SYS;
	}

	return r;
}
#else
static ER _NMP_VdoDec_Decode(UINT32 pathID)
{
	ER					r = E_OK;
	UINT32					uiYAddr = 0, uiUVAddr = 0, uiRawSize = 0;
	UINT32					uiBSAddr = 0, uiBSSize = 0;
	//NMI_VDODEC_BS_SRC 		BsSrc = {0};
	NMI_VDODEC_BS_INFO			BsSrc = {0};
	NMI_VDODEC_BSNXT_INFO			BsNextSrc = {0};
	NMI_VDODEC_RAW_INFO			VdoRawInfo = {0};
	MP_VDODEC_PARAM 			mp_vdodec_param = {0};
	MP_VDODEC_RECYUV_WH			rec_yuv = {0};
	MP_VDODEC_GETINFO_TYPE			type;
	UINT32					desc_len = 0;
	UINT8					 *ptrbs;
	BOOL					 bReleaseBsOnly = FALSE;

	VDODEC_DBG("%s:(line%d) pathID=%d\r\n", __func__, __LINE__, pathID);

	// Get BS Stream Set
	//if (!_NMP_VdoDec_GetQ(pathID, &BsSrc)) {
	if (_NMP_VdoDec_GetQ(pathID, &BsSrc) != E_OK) {
		DBG_ERR("Get BS Data is FAILED!!\r\n");
		return E_SYS;
	}

	ptrbs = (UINT8 *)BsSrc.uiBSAddr;

	if (*(ptrbs) == 0x0 && *(ptrbs+1) == 0x0 && *(ptrbs+2) == 0x0 && *(ptrbs+3) == 0x1) {
		gbNMPVdoDecHeaderInBs[pathID] = TRUE;
	} else {
		gbNMPVdoDecHeaderInBs[pathID] = FALSE;
	}

	if (BsSrc.WaitMs == -99) {
		gbNMPVdoDecPartial[pathID] = TRUE;
	} else {
		gbNMPVdoDecPartial[pathID] = FALSE;
	}

	// lock video frame for H264 and H265, do unlock in kdrv if not be reference frame or decode error
	if (gbNMPVdoDecPartial[pathID]) {
		if (gbNMPVdoDecHeaderInBs[pathID]) {
			if (gNMPVdoDecConfig[pathID].uiVideoDecType != MEDIAPLAY_VIDEO_MJPG) {
				//gNMPVdoDecObj[pathID].vRefFrmCb.VdoDec_RefFrmDo(pathID, BsSrc.uiRawAddr, TRUE);
				// todo, ltr scheme
			}
		}
	} else {
		if (gNMPVdoDecConfig[pathID].uiVideoDecType != MEDIAPLAY_VIDEO_MJPG) {
			gNMPVdoDecObj[pathID].vRefFrmCb.VdoDec_RefFrmDo(pathID, BsSrc.uiRawAddr, TRUE);
		}
	}
#if _USE_COMMON_FOR_RAW
	// use common buffer for raw
	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_ENTER); // [OUT] NEW_ENTER
	uiYAddr = BsSrc.uiRawAddr;
	rec_yuv.align_w = gNMPVdoDecConfig[pathID].uiWidth;
	rec_yuv.align_h = gNMPVdoDecConfig[pathID].uiHeight;
	type = gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG ? VDODEC_GET_JPEG_RECYUV_WH : VDODEC_GET_RECYUV_WH;
	(gNMPVdoDecObj[pathID].pMpVideoDecoder->GetInfo)(type, (UINT32 *)&rec_yuv, 0, 0);
	uiUVAddr = uiYAddr + (rec_yuv.align_w * rec_yuv.align_h);
	uiRawSize = BsSrc.uiRawSize;
#else
	// use private buffer for raw
	uiYAddr = gNMPVdoDec_RawAddr[pathID][gNMPVdoDecRawQueIdx[pathID]].Y_Addr;
	uiUVAddr = gNMPVdoDec_RawAddr[pathID][gNMPVdoDecRawQueIdx[pathID]].UV_Addr;
	uiRawSize = gNMPVdoDec_RawBlkSize[pathID];
#endif

	if (!uiYAddr || !uiUVAddr || !uiRawSize) {
		DBG_ERR("[%d] invalid raw addr, Y(0x%x) UV(0x%x) size(%d)\r\n", pathID, uiYAddr, uiUVAddr, uiRawSize);
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_ERR, ISF_ERR_QUEUE_ERROR); // [OUT] NEW_ERR
		return E_SYS;
	} else {
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_OK); // [OUT] NEW_OK
	}

	/*if (gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo) {
		(gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(MP_VDODEC_SETINFO_RAWDATAADDR, uiYAddr, uiUVAddr, 0);
	}*/

	// h26x desc filtering, remove -> user AP should handle this by itself
	if (0) {//(gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264 || gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
		if ((r = _NMP_VdoDec_GetDescLen(gNMPVdoDecConfig[pathID].uiVideoDecType, BsSrc.uiBSAddr, BsSrc.uiBSSize, &desc_len)) != E_OK) {
			return r;
		}
		if (desc_len) { // i-frame (skip desc)
			uiBSAddr = BsSrc.uiBSAddr + desc_len;
			uiBSSize = BsSrc.uiBSSize - desc_len;
		} else { // p-frame
			uiBSAddr = BsSrc.uiBSAddr;
			uiBSSize = BsSrc.uiBSSize;
		}
	} else {
		uiBSAddr = BsSrc.uiBSAddr;
		uiBSSize = BsSrc.uiBSSize;
	}

	// addr need word align -> copy to tmp buf
#if _I_FRM_COPY_TEST
	if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264 || gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
		if (!ifrm_copy_start[pathID] || !ifrm_copy_size[pathID]) {
			DBG_ERR("ifrm copy buf a=0x%x, s=%d\r\n", ifrm_copy_start[pathID], ifrm_copy_size[pathID]);
			return E_SYS;
		}

		if ((ifrm_copy_curr[pathID] + uiBSSize) > ifrm_copy_end[pathID]) {
			ifrm_copy_curr[pathID] = ifrm_copy_start[pathID]; // rollback
		}

		DBG_DUMP("[%d] copy 0x%x to 0x%x, size: %d\r\n", pathID, uiBSAddr, ifrm_copy_curr[pathID], uiBSSize);
		memcpy((VOID *)ifrm_copy_curr[pathID], (VOID *)uiBSAddr, uiBSSize);

		mp_vdodec_param.bs_addr = ifrm_copy_curr[pathID];

		// update current
		ifrm_copy_curr[pathID] += ALIGN_CEIL_4(uiBSSize);
	} else {
		mp_vdodec_param.bs_addr = uiBSAddr; // jpeg do not use copy buf
	}
#else
	// config mp param
	mp_vdodec_param.bs_addr = uiBSAddr;
#endif
	if (gbNMPVdoDecPartial[pathID] && gbNMPVdoDecHeaderInBs[pathID]) {
		mp_vdodec_param.bs_size = 0x800000;
		mp_vdodec_param.cur_bs_size = BsSrc.uiBSSize;
	} else {
		mp_vdodec_param.bs_size = uiBSSize;
	}
	mp_vdodec_param.y_addr = uiYAddr;
	mp_vdodec_param.c_addr = uiUVAddr;
	mp_vdodec_param.vRefFrmCb = gNMPVdoDecObj[pathID].vRefFrmCb;

	// Callback current used bs addr & size ---> remove, only for carDV
	/*if (gNMPVdoDecObj[pathID].CallBackFunc && (uiBSAddr > 0) && (uiBSSize > 0)) {
		MEM_RANGE cur_bs;
		cur_bs.addr = uiBSAddr;
		cur_bs.size = uiBSSize;
		(gNMPVdoDecObj[pathID].CallBackFunc)(NMI_VDODEC_EVENT_CUR_VDOBS, (UINT32) &cur_bs);
	}*/

	if (gNMPVdoDecObj[pathID].uiMsgLevel & NMP_VDODEC_MSG_INPUT) {
		UINT8 *ptr = (UINT8 *)mp_vdodec_param.bs_addr;
		DBG_DUMP("[VDODEC][%d] i=%llu, Addr=0x%x, Size=%d\r\n", pathID, BsSrc.uiThisFrmIdx, mp_vdodec_param.bs_addr, uiBSSize);

		DBG_DUMP("bs = ");
		DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
				*ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7), *(ptr+8), *(ptr+9));

		DBG_DUMP("\r\n...\r\n");

		DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
				*(ptr+uiBSSize-10), *(ptr+uiBSSize-9), *(ptr+uiBSSize-8), *(ptr+uiBSSize-7), *(ptr+uiBSSize-6), *(ptr+uiBSSize-5), *(ptr+uiBSSize-4), *(ptr+uiBSSize-3), *(ptr+uiBSSize-2), *(ptr+uiBSSize-1));

		DBG_DUMP("\r\n");
	}

	if (uiBSSize && gNMPVdoDecObj[pathID].pMpVideoDecoder) {
		// Decode Video
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_IN_PROBE_PUSH, ISF_OK);	  // [IN]  PUSH_OK
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_IN_PROBE_PROC, ISF_ENTER);  // [IN]  PROC_ENTER
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_ENTER); // [OUT] PROC_ENTER

		if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
			VdoCodec_JPEG_Lock();
		} else {
			VdoCodec_H26x_Lock();
		}

		if (gbNMPVdoDecPartial[pathID] && !gbNMPVdoDecHeaderInBs[pathID]) {
			BsNextSrc.uiBSNXTAddr = mp_vdodec_param.bs_addr;
			BsNextSrc.uiBSNXTSize = mp_vdodec_param.bs_size;
			BsNextSrc.uiTotalBsSize = BsSrc.uiBSTotalSizeOneFrm;
			r = (gNMPVdoDecObj[pathID].pMpVideoDecoder->SetInfo)(VDODEC_SET_NXT_BS, (UINT32)&BsNextSrc, 0, 0);
			if (r != E_OK) {
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
			}

			r = (gNMPVdoDecObj[pathID].pMpVideoDecoder->GetInfo)(VDODEC_GET_H26X_INTERRUPT, (UINT32 *)&gNMPVdoDecInterrupt[pathID], 0, 0);
			if (r != E_OK) {
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
			}

			if (gNMPVdoDecInterrupt[pathID] != 0x1 && gNMPVdoDecInterrupt[pathID] != 0x10) {
				mp_vdodec_param.errorcode = E_SYS;
			}
		} else {
			r = (gNMPVdoDecObj[pathID].pMpVideoDecoder->DecodeOne)(0, 0, 0, &mp_vdodec_param);
			gNMPVdoDecInterrupt[pathID] = mp_vdodec_param.interrupt;
		}

		if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
			VdoCodec_JPEG_UnLock();
		} else {
			VdoCodec_H26x_UnLock();
		}

		if (gbNMPVdoDecPartial[pathID]) {
			if (gNMPVdoDecInterrupt[pathID] == 0x10) {
				bReleaseBsOnly = TRUE;
			} else if (gNMPVdoDecInterrupt[pathID] == 0x1) {
				bReleaseBsOnly = FALSE;
			}
		} else {
			if (r != E_OK) {
				if (gNMPVdoDecObj[pathID].uiMsgLevel & NMP_VDODEC_MSG_INPUT) {
					UINT8 *ptr = (UINT8 *)mp_vdodec_param.bs_addr;
					DBG_ERR("[VDODEC][%d] Decode ERROR !! i = %llu\r\n", pathID, BsSrc.uiThisFrmIdx);
					DBG_DUMP("[VDODEC][%d] i=%llu, Addr=0x%x, Size=%d\r\n", pathID, BsSrc.uiThisFrmIdx, mp_vdodec_param.bs_addr, uiBSSize);

					DBG_DUMP("bs = ");
					DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
							*ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7), *(ptr+8), *(ptr+9));

					DBG_DUMP("\r\n...\r\n");

					DBG_DUMP("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
							*(ptr+uiBSSize-10), *(ptr+uiBSSize-9), *(ptr+uiBSSize-8), *(ptr+uiBSSize-7), *(ptr+uiBSSize-6), *(ptr+uiBSSize-5), *(ptr+uiBSSize-4), *(ptr+uiBSSize-3), *(ptr+uiBSSize-2), *(ptr+uiBSSize-1));

					DBG_DUMP("\r\n");
				}

				if (gNMPVdoDecConfig[pathID].uiVideoDecType != MEDIAPLAY_VIDEO_MJPG) {
					if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_H264) {
						UINT32 uiRawYPhyAddr = nvtmpp_sys_va2pa(mp_vdodec_param.y_addr);
						gNMPVdoDecObj[pathID].vRefFrmCb.VdoDec_RefFrmDo(pathID, uiRawYPhyAddr, FALSE);
					} else {
						gNMPVdoDecObj[pathID].vRefFrmCb.VdoDec_RefFrmDo(pathID, mp_vdodec_param.y_addr, FALSE);
					}
				}
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
			}
		}

		// Update Frame Number
		gNMPVdoDecObj[pathID].uiDecFrmNum = BsSrc.uiThisFrmIdx;

		if (BsSrc.bIsEOF) {
			VDODEC_DBG("Decode last frame Index = %llu\r\n", BsSrc.uiThisFrmIdx);
		}
	} else {
		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INCOMPLETE_DATA);	   // [IN] PUSH_ERR

		DBG_ERR("NMP Video Decoder BS Size is 0!\n\r");
		return E_SYS;
	}

	// Set RAW output
	VdoRawInfo.PathID		= pathID;
	VdoRawInfo.BsBufID		= BsSrc.BsBufID;
	VdoRawInfo.RawBufID 	= BsSrc.RawBufID;
	VdoRawInfo.uiRawAddr	= uiYAddr;
	VdoRawInfo.uiRawSize	= uiRawSize;
	VdoRawInfo.uiYAddr		= uiYAddr;
	VdoRawInfo.uiUVAddr 	= uiUVAddr;

	if (gNMPVdoDecConfig[pathID].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
		VdoRawInfo.uiWidth        = mp_vdodec_param.uiWidth;
		VdoRawInfo.uiHeight       = mp_vdodec_param.uiHeight;
		VdoRawInfo.uiYLineOffset  = VdoRawInfo.uiWidth;
		VdoRawInfo.uiUVLineOffset = VdoRawInfo.uiWidth;
		VdoRawInfo.uiYuvFmt       = mp_vdodec_param.yuv_fmt;
	} else {
		VdoRawInfo.uiWidth        = gNMPVdoDecConfig[pathID].uiImageWidth;
		VdoRawInfo.uiHeight       = gNMPVdoDecConfig[pathID].uiImageHeight;
		VdoRawInfo.uiYLineOffset  = gNMPVdoDecConfig[pathID].uiYLineOffset;
		VdoRawInfo.uiUVLineOffset = gNMPVdoDecConfig[pathID].uiUVLineOffset;
		VdoRawInfo.uiYuvFmt       = VDO_PXLFMT_YUV420;
	}
	VdoRawInfo.uiThisFrmIdx = BsSrc.uiThisFrmIdx;
	VdoRawInfo.bIsEOF		= BsSrc.bIsEOF;
	VdoRawInfo.Occupied 	= 0; // maybe vdodec doesn't need this ?
	VdoRawInfo.TimeStamp	= BsSrc.TimeStamp;
	VdoRawInfo.dec_er		= mp_vdodec_param.errorcode;
	VdoRawInfo.uiCommBufBlkID = BsSrc.uiCommBufBlkID;
	VdoRawInfo.isStop = FALSE;
	VdoRawInfo.bReleaseBsOnly = bReleaseBsOnly;

#if NMP_VDODEC_DUMP_DEC_FRAME
	static BOOL bFirst = TRUE;
	static FST_FILE fp = NULL;
	if (bFirst) {
		char sFileName[260];
		DBG_DUMP("[VDODEC][%d] dump decoded yuv frame, w = %d, h = %d, raw addr = 0x%08x, raw size = %d\r\n", pathID, gNMPVdoDecConfig[pathID].uiWidth, gNMPVdoDecConfig[pathID].uiHeight, uiYAddr, uiRawSize);
		snprintf(sFileName, sizeof(sFileName), "A:\\%d_%d.yuv", gNMPVdoDecConfig[pathID].uiWidth, gNMPVdoDecConfig[pathID].uiHeight);
		fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fp != NULL) {
			if (FileSys_WriteFile(fp, (UINT8 *)uiYAddr, &uiRawSize, 0, NULL) != FST_STA_OK) {
				DBG_ERR("[VDODEC][%d] dump decoded yuv frame err, write file fail\r\n", pathID);
			} else {
				DBG_DUMP("[VDODEC][%d] write file ok\r\n", pathID);
			}
			FileSys_CloseFile(fp);
		} else {
			DBG_ERR("[VDODEC][%d] dump decoded yuv frame err, invalid file handle\r\n", pathID);
		}

		bFirst = FALSE;
	}
#endif

	// Callback output raw data
	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pathID),  ISF_IN_PROBE_PROC,  ISF_OK); // [IN]	PROC_OK
	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_OK); // [OUT] PROC_OK

	// callback to isf_vdodec
	if (gNMPVdoDecObj[pathID].CallBackFunc) {
		(gNMPVdoDecObj[pathID].CallBackFunc)(NMI_VDODEC_EVENT_RAW_READY, (UINT32) &VdoRawInfo);
	}

	// Update RAW Index
#if (!_USE_COMMON_FOR_RAW)
	gNMPVdoDecRawQueIdx[pathID]++;
	gNMPVdoDecRawQueIdx[pathID] %= NMP_VDODEC_RAW_BLOCK_NUMBER;
#endif
	//gNMPVdoDecFrameNum[pathID]++;

	return r;
}
#endif

THREAD_DECLARE(_NMP_VdoDec_Tsk, arglist) //static void _NMP_VdoDec_Tsk(void)
{
	FLGPTN			uiFlag = 0;
	UINT32			i;

	VDODEC_DBG("_NMP_VdoDec_Tsk() start\r\n");

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_DECODE | FLG_NMP_VDODEC_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_IDLE);

		if (uiFlag & FLG_NMP_VDODEC_STOP) {
			break;
		}

		if (uiFlag & FLG_NMP_VDODEC_DECODE) {
			for (i = 0; i < NMP_VDODEC_MAX_PATH; i++) { // i as pathID
				if (_NMP_VdoDec_HowManyInQ(i) > 0 && gNMPVdoDecPath[i] == 1) {
					_NMP_VdoDec_Decode(i);
				}
			}

			for (i = 0; i < NMP_VDODEC_MAX_PATH; i++) { // i as pathID
				if (gNMPVdoDecPath[i] == 1) {
					if (_NMP_VdoDec_HowManyInQ(i) > 0) { // continue to decode
						set_flg(FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_DECODE);
					}
				}
			}
		}
	} // end of while loop

	set_flg(FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_STOP_DONE);

	THREAD_RETURN(0);
}

static ER _NMP_VdoDec_TskStart(void)
{
	// start tsk
	THREAD_CREATE(NMP_VDODEC_TSK_ID, _NMP_VdoDec_Tsk, NULL, "_NMP_VdoDec_Tsk"); //sta_tsk(NMP_VDODEC_TSK_ID, 0);
	if (NMP_VDODEC_TSK_ID == 0) {
		DBG_ERR("Invalid NMP_VDODEC_TSK_ID\r\n");
		return E_OACV;
	}
	THREAD_SET_PRIORITY(NMP_VDODEC_TSK_ID, PRI_NMP_VDODEC);
	THREAD_RESUME(NMP_VDODEC_TSK_ID);
	VDODEC_DBG("[VDODEC]video decoding task start...\r\n");

	return E_OK;
}

#ifdef VDODEC_LL
static void _NMP_VdoDec_Tsk_WrapRAW(void)
{
	FLGPTN uiFlag;
	UINT32 pathID = 0;

	THREAD_ENTRY();

	clr_flg(FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_DECODE | FLG_NMP_VDODEC_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_IDLE);

		if (uiFlag & FLG_NMP_VDODEC_STOP) {
			break;
		}

		if (uiFlag & FLG_NMP_VDODEC_DECODE) {
			if (NMP_VdoDec_GetJobCount_WrapRAW() > 0) {
				NMP_VdoDec_GetJob_WrapRAW(&pathID);
				NMP_VdoDec_TrigAndWrapRawInfo(pathID);
			}
		}//if enc

		if (NMP_VdoDec_GetJobCount_WrapRAW() > 0) { //continue to trig job
			set_flg(FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_DECODE);
		}
	}   //  end of while loop
}

THREAD_DECLARE(NMP_VdoDec_Tsk_WrapRAW, p1)
{
	_NMP_VdoDec_Tsk_WrapRAW();
	set_flg(FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_STOP_DONE);

    THREAD_RETURN(0);
}

static ER _NMP_VdoDec_Open_Wrap(void)
{
	ER r = E_OK;

	if (gNMPVdoDecOpened_WrapRAW) {
		DBG_WRN("Wrap RAW already opened...\r\n");
		return E_OK;
	}
	r = NMP_VdoDec_Start_WrapRAW();
	gNMPVdoDecOpened_WrapRAW = TRUE;
	return r;
}

static ER _NMP_VdoDec_Close_WrapRAW(void)
{
	if (gNMPVdoDecOpened_WrapRAW== FALSE) {
		DBG_WRN("Wrap RAW already closed...\r\n");
		return E_OK;
	}

#if MP_VDODEC_SHOW_MSG
	DBG_IND("[VDODEC] close Wrap RAW task\r\n");
#endif

	NMP_VdoDec_Close_WrapRAW();
	gNMPVdoDecOpened_WrapRAW = FALSE;

	return E_OK;
}

ER NMP_VdoDec_TrigWrapRAW(void)
{
	set_flg(FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_DECODE);
	return E_OK;
}

ER NMP_VdoDec_Start_WrapRAW(void)
{
	if (gNMPVidTrigOpened_WrapRAW) {
		DBG_WRN("wrap raw task already started...\r\n");
		return E_OK;
	}

	// Start Task (wrap raw)
	THREAD_CREATE(NMP_VDODEC_TSK_ID_WRAPRAW, NMP_VdoDec_Tsk_WrapRAW, NULL, "NMP_VdoDec_Tsk_WrapRAW");
	if (NMP_VDODEC_TSK_ID_WRAPRAW == NULL) {
		DBG_ERR("NMP_VDODEC_TSK_ID_WRAPRAW = NULL\r\n");
		return E_OACV;
	}
	THREAD_SET_PRIORITY(NMP_VDODEC_TSK_ID_WRAPRAW, PRI_NMP_VDODEC);
	THREAD_RESUME(NMP_VDODEC_TSK_ID_WRAPRAW);

	gNMPVidTrigOpened_WrapRAW = TRUE;
	return E_OK;
}

ER NMP_VdoDec_Close_WrapRAW(void)
{
	FLGPTN uiFlag;

	if (gNMPVidTrigOpened_WrapRAW== FALSE) {
		DBG_WRN("Wrap raw task already stoped...\r\n");
		return E_OK;
	}

	// wait for wrap bs task idle
	wai_flg(&uiFlag, FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_IDLE, TWF_ORW);

//#ifdef __KERNEL__
#if 1
	set_flg(FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_STOP);
	wai_flg(&uiFlag, FLG_ID_NMP_VDODEC_WRAPRAW, FLG_NMP_VDODEC_STOP_DONE, TWF_ORW | TWF_CLR);
#endif

#if MP_VDODEC_SHOW_MSG
	DBG_IND("[VDODEC] wait Wrap RAW close finished\r\n");
#endif

	gNMPVidTrigOpened_WrapRAW = FALSE;
	return E_OK;
}

void NMP_VdoDec_InitJobQ_WrapRAW(void)
{
	memset(&gNMPVdoDecJobQ_WrapRAW, 0, sizeof(NMP_VDODEC_WRAPBS_JOBQ));
}

ER NMP_VdoDec_PutJob_WrapRAW(UINT32 pathID)
{
	unsigned long flags = 0;

	NMP_VdoDec_Lock_cpu(&flags);

	if ((gNMPVdoDecJobQ_WrapRAW.Front == gNMPVdoDecJobQ_WrapRAW.Rear) && (gNMPVdoDecJobQ_WrapRAW.bFull == TRUE)) {
		static UINT32 sCount;
		NMP_VdoDec_Unlock_cpu(&flags);
		if (sCount % 128 == 0) {
			DBG_WRN("[VDOTRIG] Wrap BS Job Queue is Full, Job Id = %d\r\n", pathID);
		}
		sCount++;
		return E_SYS;
	} else {
		gNMPVdoDecJobQ_WrapRAW.Queue[gNMPVdoDecJobQ_WrapRAW.Rear] = pathID;
		gNMPVdoDecJobQ_WrapRAW.Rear = (gNMPVdoDecJobQ_WrapRAW.Rear + 1) % NMP_VDODEC_WRAP_JOBQ_MAX;
		if (gNMPVdoDecJobQ_WrapRAW.Front == gNMPVdoDecJobQ_WrapRAW.Rear) { // Check Queue full
			gNMPVdoDecJobQ_WrapRAW.bFull = TRUE;
		}
		NMP_VdoDec_Unlock_cpu(&flags);
		return E_OK;
	}
}

ER NMP_VdoDec_GetJob_WrapRAW(UINT32 *pPathID)
{
	unsigned long flags = 0;

	NMP_VdoDec_Lock_cpu(&flags);

	if ((gNMPVdoDecJobQ_WrapRAW.Front == gNMPVdoDecJobQ_WrapRAW.Rear) && (gNMPVdoDecJobQ_WrapRAW.bFull == FALSE)) {
		NMP_VdoDec_Unlock_cpu(&flags);
		DBG_ERR("[VDOTRIG] Wrap BS Job Queue is Empty!\r\n");
		return E_SYS;
	} else {
		*pPathID = gNMPVdoDecJobQ_WrapRAW.Queue[gNMPVdoDecJobQ_WrapRAW.Front];
		gNMPVdoDecJobQ_WrapRAW.Front = (gNMPVdoDecJobQ_WrapRAW.Front + 1) % NMP_VDODEC_WRAP_JOBQ_MAX;
		if (gNMPVdoDecJobQ_WrapRAW.Front == gNMPVdoDecJobQ_WrapRAW.Rear) { // Check Queue full
			gNMPVdoDecJobQ_WrapRAW.bFull = FALSE;
		}
		NMP_VdoDec_Unlock_cpu(&flags);
		return E_OK;
	}
}

UINT32 NMP_VdoDec_GetJobCount_WrapRAW(void)
{
	UINT32 front, rear, full, sq = 0;
	unsigned long flags = 0;

	NMP_VdoDec_Lock_cpu(&flags);
	front = gNMPVdoDecJobQ_WrapRAW.Front;
	rear = gNMPVdoDecJobQ_WrapRAW.Rear;
	full = gNMPVdoDecJobQ_WrapRAW.bFull;
	NMP_VdoDec_Unlock_cpu(&flags);

	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMP_VDODEC_WRAP_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMP_VDODEC_WRAP_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}
#endif

void nmp_vdodec_showmsg(UINT32 pathID, UINT32 msgLevel)
{
	if (pathID >= NMP_VDODEC_MAX_PATH) {
		DBG_ERR("[VDODEC] showmsg, invalid path index = %d\r\n", pathID);
		return;
	}
	gNMPVdoDecObj[pathID].uiMsgLevel = msgLevel;

}

/**
    @name   NMediaPlay VdoDec NMI API
*/
static ER NMI_VdoDec_Open(UINT32 Id, UINT32 *pContext)
{
	ER r = E_OK;

	VDODEC_DBG("%s:(line%d) pathID=%d\r\n", __func__, __LINE__, Id);
	gNMPVdoDecPath[Id] = 1;
	gNMPVdoDecPathCount++;
	if (gNMPVdoDecPathCount == 1) {
		if ((r = _NMP_VdoDec_TskStart()) != E_OK) {
			goto _OPEN_VDEC_FAIL;
		}

#ifdef VDODEC_LL
		if ((r = _NMP_VdoDec_Open_Wrap()) != E_OK) {
			goto _OPEN_WRAP_FAIL;
		}
		NMP_VdoDec_InitJobQ_WrapRAW();
#endif
	}
	return E_OK;

#ifdef VDODEC_LL
_OPEN_WRAP_FAIL:
	 _NMP_VdoDec_TskStop(Id);
#endif
_OPEN_VDEC_FAIL:
	return r;
}

static ER NMI_VdoDec_SetParam(UINT32 Id, UINT32 Param, UINT32 Value)
{
	switch (Param) {
		case NMI_VDODEC_PARAM_DECODER_OBJ:
			gNMPVdoDecObj[Id].pMpVideoDecoder = (MP_VDODEC_DECODER *)Value;
			break;

		case NMI_VDODEC_PARAM_MEM_RANGE:
		{
			UINT32 uiAddr = 0;
			NMI_VDODEC_MEM_RANGE *pMem = (NMI_VDODEC_MEM_RANGE *) Value;

			if (pMem->Addr == 0 || pMem->Size == 0) {
				DBG_ERR("[VDODEC][%d] Invalid mem range, addr = 0x%08x, size = %d\r\n", Id, pMem->Addr, pMem->Size);
				return E_PAR;
			}
			else {
				VDODEC_DBG("[VDODEC][%d] MemAddr = 0x%x, Size = 0x%x\r\n", Id, pMem->Addr, pMem->Size);
				uiAddr = pMem->Addr;
				if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_H264) {
					gNMPVdoDecConfig[Id].uiDecoderBufAddr = uiAddr;
					gNMPVdoDecConfig[Id].uiDecoderBufSize = NMP_VDODEC_H264_CODEC_SIZE;
					//uiAddr += NMP_VDODEC_H264_CODEC_SIZE;
#if _I_FRM_COPY_TEST
					ifrm_copy_start[Id] = uiAddr + NMP_VDODEC_H264_CODEC_SIZE;
					ifrm_copy_curr[Id] = ifrm_copy_start[Id];
					ifrm_copy_size[Id] = IFRM_COPY_BUF_SIZE;
					ifrm_copy_end[Id] = ifrm_copy_start[Id] + ifrm_copy_size[Id];
					DBG_DUMP("[%d] copy_buf: (0x%x ~ 0x%x), s=0x%x, curr=0x%x\r\n",
							Id, ifrm_copy_start[Id], ifrm_copy_end[Id], ifrm_copy_size[Id], ifrm_copy_curr[Id]);
#endif

#if (!_USE_COMMON_FOR_RAW) // use comm buf, do not alloc this
					gNMPVdoDecConfig[Id].uiRawStartAddr = uiAddr + NMP_VDODEC_H264_CODEC_SIZE;
					gNMPVdoDecConfig[Id].uiRawBufSize	= pMem->Size - NMP_VDODEC_H264_CODEC_SIZE;
#endif
				} else if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
					gNMPVdoDecConfig[Id].uiDecoderBufAddr = uiAddr;
					gNMPVdoDecConfig[Id].uiDecoderBufSize = NMP_VDODEC_H265_CODEC_SIZE;
					//uiAddr += NMP_VDODEC_H265_CODEC_SIZE;
#if _I_FRM_COPY_TEST
					ifrm_copy_start[Id] = uiAddr;
					ifrm_copy_curr[Id] = ifrm_copy_start[Id];
					ifrm_copy_size[Id] = IFRM_COPY_BUF_SIZE;
					ifrm_copy_end[Id] = ifrm_copy_start[Id] + ifrm_copy_size[Id];
					DBG_DUMP("[%d] copy_buf: (0x%x ~ 0x%x), s=0x%x, curr=0x%x\r\n",
							Id, ifrm_copy_start[Id], ifrm_copy_end[Id], ifrm_copy_size[Id], ifrm_copy_curr[Id]);
#endif

#if (!_USE_COMMON_FOR_RAW) // use comm buf, do not alloc this
					gNMPVdoDecConfig[Id].uiRawStartAddr = uiAddr;
					gNMPVdoDecConfig[Id].uiRawBufSize	= pMem->Size - NMP_VDODEC_H265_CODEC_SIZE;
#endif
				} else if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
#if (!_USE_COMMON_FOR_RAW) // use comm buf, do not alloc this
					gNMPVdoDecConfig[Id].uiRawStartAddr = uiAddr;
					gNMPVdoDecConfig[Id].uiRawBufSize = pMem->Size;
#endif
				} else {
					DBG_ERR("[%d] set mem range, unknown codec type(%d)\r\n", Id, gNMPVdoDecConfig[Id].uiVideoDecType);
					return E_NOSPT;
				}
			}
			break;
		}

		case NMI_VDODEC_PARAM_MAX_MEM_INFO:
		{
			//UINT32 uiVdoCodec, uiWidth, uiHeight;
			NMI_VDODEC_MAX_MEM_INFO *pMaxMemInfo = (NMI_VDODEC_MAX_MEM_INFO *) Value;
			UINT32 uiCodecsize= 0;

			// keep dynamic param to temp(removed)  -> use max_mem value only!!
			/*uiVdoCodec = gNMPVdoDecConfig[Id].uiVideoDecType;
			uiWidth = gNMPVdoDecConfig[Id].uiWidth;
			uiHeight = gNMPVdoDecConfig[Id].uiHeight;*/

			// get max buf info
			gNMPVdoDecConfig[Id].uiVideoDecType = pMaxMemInfo->uiVdoCodec;
			if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_H264 || gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
				gNMPVdoDecConfig[Id].uiWidth = pMaxMemInfo->uiWidth;
				gNMPVdoDecConfig[Id].uiHeight = pMaxMemInfo->uiHeight;
			}  else if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
				gNMPVdoDecConfig[Id].uiWidth = ALIGN_CEIL_16(pMaxMemInfo->uiWidth);
				gNMPVdoDecConfig[Id].uiHeight = ALIGN_CEIL_16(pMaxMemInfo->uiHeight);
			}
#if 1
			if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
				// if MJPG ... just calculate it
				uiCodecsize = _NMP_VdoDec_GetCodecSize(Id);
			} else {
				// if H264/H265 ... calculate both and use larger one (for max memory way....h264/h265 need memory is difference for settings, sometimes h264 need more, sometime h265 need more)
				// also we have to try (bD2dEn = 0/1) + (rotate = 0/1) , get MAX one
				UINT32 uiH264Need = 0, uiH265Need = 0;
				UINT32 uiCodecsize_max = 0;
				UINT32 backup_codec = gNMPVdoDecConfig[Id].uiVideoDecType;

				gNMPVdoDecConfig[Id].uiVideoDecType = MEDIAPLAY_VIDEO_H264;
				uiH264Need = _NMP_VdoDec_GetCodecSize(Id);
				gNMPVdoDecConfig[Id].uiVideoDecType = MEDIAPLAY_VIDEO_H265;
				uiH265Need = _NMP_VdoDec_GetCodecSize(Id);

				uiCodecsize_max = (uiCodecsize_max < uiH264Need)? uiH264Need:uiCodecsize_max;
				uiCodecsize_max = (uiCodecsize_max < uiH265Need)? uiH265Need:uiCodecsize_max;
				DBG_IND("[VDODEC][%d] H264 need = %u, H265 need = %u\r\n", Id, uiH264Need, uiH265Need);


				uiCodecsize = uiCodecsize_max;
				DBG_IND("[VDODEC][%d] uiCodecsize = %u\r\n", Id, uiCodecsize);
				gNMPVdoDecConfig[Id].uiVideoDecType = backup_codec;
			}
			pMaxMemInfo->uiDecsize = uiCodecsize + _NMP_VdoDec_GetBufSize(Id);
#else
			if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_H264) {
				pMaxMemInfo->uiDecsize = NMP_VDODEC_H264_CODEC_SIZE + _NMP_VdoDec_GetBufSize(Id);
			} else if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
				pMaxMemInfo->uiDecsize = NMP_VDODEC_H265_CODEC_SIZE + _NMP_VdoDec_GetBufSize(Id);
			} else if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
				pMaxMemInfo->uiDecsize = _NMP_VdoDec_GetBufSize(Id);
			} else {
				DBG_ERR("[%d] set max mem, unknown codec type(%d)\r\n", Id, gNMPVdoDecConfig[Id].uiVideoDecType);
				return E_NOSPT;
			}
#endif
			VDODEC_DBG("[VDODEC][%d] Set max alloc size, codec=%d, w=%d, h=%d, total buf=0x%x\r\n",
					Id, gNMPVdoDecConfig[Id].uiVideoDecType, gNMPVdoDecConfig[Id].uiWidth, gNMPVdoDecConfig[Id].uiHeight, pMaxMemInfo->uiDecsize);

			// set dynamic param back (removed)  -> use max_mem value only!!
			/*gNMPVdoDecConfig[Id].uiVideoDecType = uiVdoCodec;
			gNMPVdoDecConfig[Id].uiWidth = uiWidth;
			gNMPVdoDecConfig[Id].uiHeight = uiHeight;*/
			break;
		}

		case NMI_VDODEC_PARAM_CODEC:
			VDODEC_DBG("[VDODEC][%d] Codec = %d\r\n", Id, Value);
			gNMPVdoDecConfig[Id].uiVideoDecType = Value;
			break;

		case NMI_VDODEC_PARAM_DESC_ADDR:
			VDODEC_DBG("[VDODEC][%d] uiDescAddr = 0x%x\r\n", Id, Value);
			gNMPVdoDecConfig[Id].uiDescAddr = Value;
			break;

		case NMI_VDODEC_PARAM_DESC_LEN:
			VDODEC_DBG("[VDODEC][%d] uiDescSize = %d\r\n", Id, Value);
			gNMPVdoDecConfig[Id].uiDescSize = Value;
			break;

		// set gNMPVdoDecConfig.uiWidth, gNMPVdoDecConfig.uiHeight only by NMI_VDODEC_PARAM_MAX_MEM_INFO
		/*case NMI_VDODEC_PARAM_WIDTH:
			VDODEC_DBG("[VDODEC][%d] width = %d\r\n", Id, Value);
			gNMPVdoDecConfig[Id].uiWidth = Value;
			break;

		case NMI_VDODEC_PARAM_HEIGHT:
			VDODEC_DBG("[VDODEC][%d] height = %d\r\n", Id, Value);
			gNMPVdoDecConfig[Id].uiHeight = Value;
			break;*/

		case NMI_VDODEC_PARAM_GOP:
			VDODEC_DBG("[VDODEC][%d] GOP = %d\r\n", Id, Value);
			gNMPVdoDecConfig[Id].uiGOPNumber = Value;
			break;

		case NMI_VDODEC_PARAM_REG_CB:
			VDODEC_DBG("[VDODEC][%d] reg callback = 0x%08x\r\n", Id, Value);
			gNMPVdoDecObj[Id].CallBackFunc = (NMI_VDODEC_CB *)Value;
			break;

		/*case NMI_VDODEC_PARAM_REFRESH_VDO:
			VDODEC_DBG("[VDODEC][%d] refresh video queue\r\n", Id);
			_NMP_VdoDec_ResetQ(Id);
			break;*/

		case NMI_VDODEC_PARAM_DROP_FRAME:
			// [ToDo]
			break;

		case NMI_VDODEC_PARAM_REFFRM_CB:
			gNMPVdoDecObj[Id].vRefFrmCb.id = Id;
			gNMPVdoDecObj[Id].vRefFrmCb.VdoDec_RefFrmDo = (MEDIAPLAY_REFFRMCB) Value;
			break;

		default:
			DBG_ERR("[VDODEC][%d] Set invalid param = %d\r\n", Id, Param);
			return E_PAR;
	}
	return E_OK;
}

static ER NMI_VdoDec_GetParam(UINT32 Id, UINT32 Param, UINT32 *pValue)
{
	ER status = E_OK;

	switch (Param) {
		case NMI_VDODEC_PARAM_ALLOC_SIZE:
		{
			UINT32 uiCodecsize = 0;
#if 1
		if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
			// if MJPG ... just calculate it
			uiCodecsize = _NMP_VdoDec_GetCodecSize(Id);
		} else {
			// if H264/H265 ... calculate both and use larger one (for max memory way....h264/h265 need memory is difference for settings, sometimes h264 need more, sometime h265 need more)
			// also we have to try (bD2dEn = 0/1) + (rotate = 0/1) , get MAX one
				UINT32 uiH264Need = 0, uiH265Need = 0;
				UINT32 uiCodecsize_max = 0;
				UINT32 backup_codec = gNMPVdoDecConfig[Id].uiVideoDecType;

				gNMPVdoDecConfig[Id].uiVideoDecType = MEDIAPLAY_VIDEO_H264;
				uiH264Need = _NMP_VdoDec_GetCodecSize(Id);
				gNMPVdoDecConfig[Id].uiVideoDecType = MEDIAPLAY_VIDEO_H265;
				uiH265Need = _NMP_VdoDec_GetCodecSize(Id);

				uiCodecsize_max = (uiCodecsize_max < uiH264Need)? uiH264Need:uiCodecsize_max;
				uiCodecsize_max = (uiCodecsize_max < uiH265Need)? uiH265Need:uiCodecsize_max;
				DBG_IND("[VDODEC][%d] H264 need = %u, H265 need = %u\r\n", Id, uiH264Need, uiH265Need);


				uiCodecsize = uiCodecsize_max;
				DBG_IND("[VDODEC][%d] uiCodecsize = %u\r\n", Id, uiCodecsize);
				gNMPVdoDecConfig[Id].uiVideoDecType = backup_codec;
			}
			*pValue = uiCodecsize + _NMP_VdoDec_GetBufSize(Id);
		}
#else

			if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_H264) {
				*pValue = NMP_VDODEC_H264_CODEC_SIZE + _NMP_VdoDec_GetBufSize(Id);
			} else if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_H265) {
				*pValue = NMP_VDODEC_H265_CODEC_SIZE + _NMP_VdoDec_GetBufSize(Id);
			} else if (gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG) {
				*pValue =  _NMP_VdoDec_GetBufSize(Id);
			} else {
				DBG_ERR("[%d] get alloc size, unknown codec type(%d)\r\n", Id, gNMPVdoDecConfig[Id].uiVideoDecType);
				status = E_NOSPT;
			}
#endif
			break;

		case NMI_VDODEC_PARAM_CODEC:
			*pValue = gNMPVdoDecConfig[Id].uiVideoDecType;
			break;

		case NMI_VDODEC_PARAM_WIDTH:
			*pValue = gNMPVdoDecConfig[Id].uiWidth;
			break;

		case NMI_VDODEC_PARAM_HEIGHT:
			*pValue = gNMPVdoDecConfig[Id].uiHeight;
			break;

		case NMI_VDODEC_PARAM_GOP:
			*pValue = gNMPVdoDecConfig[Id].uiGOPNumber;
			break;

		case NMI_VDODEC_PARAM_YUV_SIZE:
			{
				MP_VDODEC_RECYUV_WH rec_yuv = {0};
				MP_VDODEC_GETINFO_TYPE type;
				rec_yuv.align_w = gNMPVdoDecConfig[Id].uiWidth;
				rec_yuv.align_h = gNMPVdoDecConfig[Id].uiHeight;
				type = gNMPVdoDecConfig[Id].uiVideoDecType == MEDIAPLAY_VIDEO_MJPG ? VDODEC_GET_JPEG_RECYUV_WH : VDODEC_GET_RECYUV_WH;
				(gNMPVdoDecObj[Id].pMpVideoDecoder->GetInfo)(type, (UINT32 *)&rec_yuv, 0, 0);
				*pValue = rec_yuv.align_w * rec_yuv.align_h * 3 / 2;
			}
			break;

		case NMI_VDODEC_PARAM_VUI_INFO:
			(gNMPVdoDecObj[Id].pMpVideoDecoder->GetInfo)(VDODEC_GET_VUI_INFO, pValue, 0, 0);
			break;
		case NMI_VDODEC_PARAM_JPEGINFO:
			//(gNMPVdoDecObj[Id].pMpVideoDecoder->GetInfo)(VDODEC_GET_JPEG_INFO, pValue, 0, 0);
			break;
		default:
			DBG_ERR("[VDODEC][%d] Get invalid param = %d\r\n", Id, Param);
			status = E_PAR;
			break;
	}
	return status;
}

static ER NMI_VdoDec_Action(UINT32 Id, UINT32 Action)
{
	ER status = E_SYS;
	UINT32 i = 0;

	VDODEC_DBG("%s:(line%d) pathID=%d, Action=%d\r\n", __func__, __LINE__, Id, Action);

	switch (Action) {
		case NMI_VDODEC_ACTION_START:
			if (gNMPVdoDecObj[Id].bStart == FALSE) {
				if ((status = _NMP_VdoDec_Start(Id)) != E_OK) {
					return status;
				}
				gNMPVdoDecObj[Id].bStart = TRUE;
				status = E_OK;
			} else {
				DBG_ERR("[VDODEC][%d] invalid operation, start again before stopping", Id);
				status = E_SYS;
			}
			break;

		case NMI_VDODEC_ACTION_STOP:
			while (gNMPVdoDecObj[Id].bStart == FALSE && i < 10) {
				DELAY_M_SEC(100);
				i++;
			}
			if (gNMPVdoDecObj[Id].bStart == TRUE) {
				gNMPVdoDecObj[Id].bStart = FALSE;

				_NMP_VdoDec_Stop(Id);

				// wait video task idle, then close H26x Driver
				_NMP_VdoDec_WaitTskIdle();

				if ((gNMPVdoDecObj[Id].pMpVideoDecoder) && (gNMPVdoDecObj[Id].pMpVideoDecoder->Close)) {
					(gNMPVdoDecObj[Id].pMpVideoDecoder->Close)();
					status = E_OK;
				} else {
					DBG_ERR("[%d] close fail, mp_dec is null\r\n", Id);
					status = E_SYS;
				}

			} else {
				DBG_DUMP("[%d] Invalid Action = %d, not start yet\r\n", Id, Action);
				status = E_SYS;
			}
			break;

		default:
			DBG_ERR("[VDODEC][%d] Invalid action = %d\r\n", Id, Action);
			break;
	}

	return status;
}

static ER NMI_VdoDec_In(UINT32 Id, UINT32 *pBuf)
{
	ER status;
	NMI_VDODEC_BS_INFO *p_bs;

	VDODEC_DBG("%s:(line%d) pathID=%d\r\n", __func__, __LINE__, Id);

	if (gNMPVdoDecObj[Id].bStart == FALSE) {
		return E_SYS;
	}

	p_bs = (NMI_VDODEC_BS_INFO *)pBuf;

	DBG_IND("In: bs_addr=0x%x, bs_size=0x%x\r\n", p_bs->uiBSAddr, p_bs->uiBSSize);

	// h26x decoder needs I-frame to init first
	/*if (decoder_initialized[Id] == FALSE) {
		gNMPVdoDecConfig[Id].uiDescAddr = p_bs->uiBSAddr;
		gNMPVdoDecConfig[Id].uiDescSize = p_bs->uiBSSize;
		if ((status = _NMP_VdoDec_ConfigureObj(Id)) != E_OK) {
			DBG_ERR("dec init fail\r\n");
			return status;
		}
		decoder_initialized[Id] = TRUE;
	}*/

	status = _NMP_VdoDec_PutQ(Id, (NMI_VDODEC_BS_INFO *)pBuf);
	set_flg(FLG_ID_NMP_VDODEC, FLG_NMP_VDODEC_DECODE);

	return status;
}

static ER NMI_VdoDec_Close(UINT32 Id)
{
	VDODEC_DBG("%s:(line%d) pathID=%d\r\n", __func__, __LINE__, Id);
	gNMPVdoDecPath[Id] = 0;
	gNMPVdoDecPathCount--;
	if (gNMPVdoDecPathCount == 0) {
		_NMP_VdoDec_TskStop(Id);
#ifdef VDODEC_LL
		_NMP_VdoDec_Close_WrapRAW();
#endif
	}

	return E_OK;
}

NMI_UNIT NMI_VdoDec = {
	.Name           = "VdoDec",
	.Open           = NMI_VdoDec_Open,
	.SetParam       = NMI_VdoDec_SetParam,
	.GetParam       = NMI_VdoDec_GetParam,
	.Action         = NMI_VdoDec_Action,
	.In             = NMI_VdoDec_In,
	.Close          = NMI_VdoDec_Close,
};

// install task
void nmp_vdodec_install_id(void)
{
	UINT32 i = 0;

#if defined __UITRON || defined __ECOS
	OS_CONFIG_TASK(NMP_VDODEC_TSK_ID, PRI_NMP_VDODEC, STKSIZE_NMP_VDODEC, _NMP_VdoDec_Tsk);
#ifdef VDODEC_LL
	OS_CONFIG_TASK(NMP_VDODEC_TSK_ID_WRAPRAW,        PRI_NMP_VDODEC,    STKSIZE_NMP_VDODEC,      NMP_VdoDec_Tsk_WrapRAW);
#endif
#endif

	OS_CONFIG_FLAG(FLG_ID_NMP_VDODEC);
#ifdef VDODEC_LL
	OS_CONFIG_FLAG(FLG_ID_NMP_VDODEC_WRAPRAW);
#endif

    for (i = 0; i < NMP_VDODEC_MAX_PATH; i++) {
#if defined __UITRON || defined __ECOS
        OS_CONFIG_SEMPHORE(NMP_VDODEC_SEM_ID[i], 0, 1, 1);
#ifdef VDODEC_LL
        OS_CONFIG_SEMPHORE(NMP_VDODEC_TRIG_SEM_ID[i], 0, 1, 1);
#endif
#else
		SEM_CREATE(NMP_VDODEC_SEM_ID[i], 1);
#ifdef VDODEC_LL
		SEM_CREATE(NMP_VDODEC_TRIG_SEM_ID[i], 1);
#endif
#endif
    }
}

void nmp_vdodec_uninstall_id(void)
{
	UINT32 i = 0;

	rel_flg(FLG_ID_NMP_VDODEC);
#ifdef VDODEC_LL
	rel_flg(FLG_ID_NMP_VDODEC_WRAPRAW);
#endif

    for (i = 0; i < NMP_VDODEC_MAX_PATH; i++) {
		SEM_DESTROY(NMP_VDODEC_SEM_ID[i]);
#ifdef VDODEC_LL
		SEM_DESTROY(NMP_VDODEC_TRIG_SEM_ID[i]);
#endif
    }
}

//
// test commands
//
static BOOL Cmd_VdoDec_SetDebug(CHAR *strCmd)
{
	UINT32 dbg_msg;

	sscanf_s(strCmd, "%d", &dbg_msg);

	vdodec_debug = (dbg_msg > 0);

	return TRUE;
}

static BOOL cmd_vdodec_showmsg(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiMsgLevel = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiMsgLevel);
#if NMP_VDODEC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	nmp_vdodec_showmsg(uiPathID, uiMsgLevel);
	return TRUE;
}

SXCMD_BEGIN(vdodec, "NMI vdo dec module")
SXCMD_ITEM("dbg %d",	Cmd_VdoDec_SetDebug, 	"set debug msg (Param: 1->on, 0->off)")
SXCMD_ITEM("showmsg %s",			cmd_vdodec_showmsg, 			 "show msg (Param: PathId Level(0:Disable, 1:Input bs info) => showmsg 0 1)")
SXCMD_END()

void NMP_VdoDec_AddUnit(void)
{
	NMI_AddUnit(&NMI_VdoDec);
	sxcmd_addtable(vdodec);
}

#ifdef __KERNEL__

int _isf_vdodec_cmd_vdodec_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(vdodec);
	UINT32 loop=1;

	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "vdodec");
	DBG_DUMP("=====================================================================\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", vdodec[loop].p_name, vdodec[loop].p_desc);
	}

	return 0;
}

int _isf_vdodec_cmd_vdodec(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(vdodec);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_isf_vdodec_cmd_vdodec_showhelp();
		return 0;
	}

	if (FALSE == _nmp_vdodec_is_init()) {
		DBG_ERR(" NOT init yet, could not use debug command !!\r\n");
		return -1;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, vdodec[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = vdodec[loop].p_func(cmd_args);
			break;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/hdal/vdec/help\" for help.\r\n");
		return -1;
	}


	return 0;
}
#endif

