/*
    Media Mdat Task.

    This file is the task of media recorder.

    @file       NMediaRecVdoTrigTsk.c
    @ingroup    mIAPPMEDIAREC
    @note       add MEDIANAMINGRULE
    @version    V1.03.001
    @date       2013/02/26

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"
#include "kwrap/spinlock.h"
#include <kwrap/cpu.h>
#include "comm/hwclock.h"
#include "isf_vdoenc_internal.h"
#include "kflow_videoenc/isf_vdoenc_int.h"

#include "nmediarec_api.h"
#include "nmediarec_vdoenc.h"
#include "nmediarec_vdoenc_tsk.h"
#include "nmediarec_vdotrig_tsk.h"
#include "nmediarec_vdoenc_platform.h"

#include "video_codec_mjpg.h"

#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/vdoenc_builtin.h"

#include "kdrv_videoenc/kdrv_videoenc_lmt.h"

#ifdef __KERNEL__
#include <linux/io.h>
#endif

#if ISF_LATENCY_DEBUG
#include "HwClock.h"
#endif

#if _TODO
// bc_getforeground_v2 : #define none first
#else
// bc_getforeground_v2
#define bc_getforeground_v2(args...)
#endif

#define Perf_GetCurrent(x)  hwclock_get_counter(x)

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          nmediarec_vidtrig
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int nmediarec_vidtrig_debug_level = NVT_DBG_WRN;
module_param_named(debug_level_nmediarec_vidtrig, nmediarec_vidtrig_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_nmediarec_vidtrig, "nmediarec_vidtrig debug level");
///////////////////////////////////////////////////////////////////////////////


#define NMR_VDOTRIG_DUMP_ENC_FRAME              0
#define NMR_VDOTRIG_DUMP_ENC_FRAME_KERNEL_VER   0		// PureLinux Version
#define NMR_VDOTRIG_DUMP_SNAPSHOT               0
#define NMR_VDOTRIG_DUMP_SNAPSHOT_KERNEL_VER    0		// PureLinux Version
#define NMR_VDOTRIG_DUMP_LOCK_INFO				0
#define NMR_VDOTRIG_RING_BUF_TEST				0

#define NMR_VDOTRIG_ACT_NORMAL                  0
#define NMR_VDOTRIG_ACT_PRELOAD                 1

#define ABSVALUE(s)  ((s) > 0 ? (s) : -(s))

#ifdef __KERNEL__
#if NMR_VDOTRIG_DUMP_ENC_FRAME_KERNEL_VER | NMR_VDOTRIG_DUMP_SNAPSHOT_KERNEL_VER
#include <kwrap/file.h>
#endif
#endif

#if NMR_VDOENC_DYNAMIC_CONTEXT
NMR_VDOTRIG_OBJ                                 *gNMRVdoTrigObj = NULL;
extern NMR_VDOENC_OBJ                           *gNMRVdoEncObj;
#else
NMR_VDOTRIG_OBJ                                 gNMRVdoTrigObj[NMR_VDOENC_MAX_PATH] = {0};
extern NMR_VDOENC_OBJ                           gNMRVdoEncObj[];
#endif
NMR_VDOTRIG_JOBQ                                gNMRVdoTrigJobQ_H26X = {0};
NMR_VDOTRIG_JOBQ                                gNMRVdoTrigJobQ_H26X_Snapshot = {0};
NMR_VDOTRIG_JOBQ                                gNMRVdoTrigJobQ_JPEG = {0};
#ifdef VDOENC_LL
NMR_VDOTRIG_JOBQ                                gNMRVdoTrigJobQ_WrapBS = {0};
#endif
UINT32                                          gNMRVdoTrigFakeIPLStop[NMR_VDOENC_MAX_PATH] = {0};
UINT8                                           gNMRVdoTrigMsg = 0;
UINT8                                           gNMRVidTrigOpened_H26X = FALSE;
UINT8                                           gNMRVidTrigOpened_JPEG = FALSE;
#ifdef VDOENC_LL
UINT8                                           gNMRVidTrigOpened_WrapBS = FALSE;
BOOL                                            gNMRVidTrigStop[NMR_VDOENC_MAX_PATH] = {0};
#endif
#ifdef __KERNEL__
BOOL                                            gNMRVidTrig1stopen[BUILTIN_VDOENC_PATH_ID_MAX] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};
BOOL                                            gNMRVidTrigBuiltinBSDone[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
#endif
extern UINT32                                   gNMRVdoEncFirstPathID;

#if NMR_VDOTRIG_RING_BUF_TEST
static UINT32 									uiPreAddr[NMR_VDOENC_MAX_PATH] = {0};
static UINT32 									uiPreAddr2[NMR_VDOENC_MAX_PATH] = {0};
static UINT32 									uiPreAddr3[NMR_VDOENC_MAX_PATH] = {0};
#endif

#ifdef __KERNEL__
extern void NMR_VdoEnc_GetBuiltinBsData(UINT32 pathID);
#endif

UINT32 _nmr_vdotrig_query_context_size(void)
{
#if NMR_VDOENC_DYNAMIC_CONTEXT
	return (PATH_MAX_COUNT * sizeof(NMR_VDOTRIG_OBJ));
#else
	return 0;
#endif
}

void _nmr_vdotrig_assign_context(UINT32 addr)
{
#if NMR_VDOENC_DYNAMIC_CONTEXT
	gNMRVdoTrigObj = (NMR_VDOTRIG_OBJ*)addr;
#endif
	return;
}

void _nmr_vdotrig_free_context(void)
{
#if NMR_VDOENC_DYNAMIC_CONTEXT
	gNMRVdoTrigObj = NULL;
#endif
	return;
}

static UINT32 _nmr_vdotrig_nalu_num_max(void)
{
	// TODO : nvt_get_chip_id() to use different chip define for H26XE_SLICE_MAX_NUM
	return H26XE_SLICE_MAX_NUM;
}

static UINT32 _nmr_vdotrig_nalu_size_info_max_size(void)
{
	return 64 + ALIGN_CEIL_64(sizeof(UINT32) + _nmr_vdotrig_nalu_num_max() *(sizeof(UINT32)+sizeof(UINT32)));   // nalu_num + 176 * (nalu_addr + nalu_size) , 176 is HW limit for 52x
}


static UINT32 _NMR_VdoTrig_InitQ(UINT32 pathID)
{
	if (pathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOTRIG][%d] ID is invalid\r\n", pathID);
		return 0;
	}

	memset(&(gNMRVdoTrigObj[pathID].rawQueue), 0, sizeof(NMR_VDOTRIG_YUVQ));
	memset(&(gNMRVdoTrigObj[pathID].bsQueue), 0, sizeof(NMR_VDOTRIG_BSQ));
    memset(&(gNMRVdoTrigObj[pathID].smartroiQueue), 0, sizeof(NMR_VDOTRIG_SMARTROIQ));
#if NMR_VDOTRIG_RING_BUF_TEST
	uiPreAddr[pathID] = 0;
	uiPreAddr2[pathID] = 0;
	uiPreAddr3[pathID] = 0;
#endif
	return 0;
}

static UINT32 _NMR_VdoTrig_InitTop(UINT32 pathID)
{
	UINT32 i = 0, j = 0;

	for (i = 0; i < NMI_VDOENC_TOP_MAX; i++) {
		gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMin = 0;
		gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMax = 0;
		for (j = 0; j < 4; j++) {
			gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiCur[j] = 0;
		}
	}

	return 0;
}

UINT32 _NMR_VdoTrig_YUV_TMOUT_ResourceUnlock(UINT32 pathID)
{
	// consume semaphore first
	if (SEM_WAIT_TIMEOUT(NMR_VDOENC_YUV_SEM_ID[pathID], vos_util_msec_to_tick(0))) {
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[VDOTRIG][%d] Input YUV_TMOUT, Unlock, sem 0->0->1\r\n", pathID);
		}
	}else{
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[VDOTRIG][%d] Input YUV_TMOUT, Unlock, sem 1->0->1\r\n", pathID);
		}
	}

	// semaphore +1
	SEM_SIGNAL(NMR_VDOENC_YUV_SEM_ID[pathID]);

	return 0;
}

static BOOL _NMR_VdoTrig_YUV_TMOUT_RetryCheck(UINT32 pathID, MP_VDOENC_YUV_SRC *p_YuvSrc)
{
	UINT32 pass_time_ms = (Perf_GetCurrent() - p_YuvSrc->uiInputTime)/1000;

	if (p_YuvSrc->iInputTimeout < 0) {
		//blocking mode
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[VDOTRIG][%d] Input YUV[%u]_TMOUT, wait_ms(blocking), resource waiting....\r\n", pathID, p_YuvSrc->uiRawCount);
		}

		if (SEM_WAIT_INTERRUPTIBLE(NMR_VDOENC_YUV_SEM_ID[pathID])) return FALSE;

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[VDOTRIG][%d] Input YUV[%u]_TMOUT, wait_ms(blocking), resource OK !!\r\n", pathID, p_YuvSrc->uiRawCount);
		}

		if (gNMRVdoEncObj[pathID].bStart == TRUE)
			return TRUE; // should try again

	} else if (pass_time_ms < (UINT32)p_YuvSrc->iInputTimeout) {
		// timeout mode
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[VDOTRIG][%d] Input YUV[%u]_TMOUT, wait_ms(%d), resource waiting....\r\n", pathID, p_YuvSrc->uiRawCount, p_YuvSrc->iInputTimeout);
		}

		if (SEM_WAIT_TIMEOUT(NMR_VDOENC_YUV_SEM_ID[pathID], vos_util_msec_to_tick(p_YuvSrc->iInputTimeout - pass_time_ms))) {
			if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_YUV_TMOUT) {
				DBG_DUMP("[VDOTRIG][%d] Input YUV[%u]_TMOUT, wait_ms(%d), pass_time(%u), timeout !!\r\n", pathID, p_YuvSrc->uiRawCount, p_YuvSrc->iInputTimeout, (Perf_GetCurrent() - p_YuvSrc->uiInputTime)/1000);
			}
		} else {
			if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_YUV_TMOUT) {
				DBG_DUMP("[VDOTRIG][%d] Input YUV[%u]_TMOUT, wait_ms(%d), resource OK !!\r\n", pathID, p_YuvSrc->uiRawCount, p_YuvSrc->iInputTimeout);
			}

			if (gNMRVdoEncObj[pathID].bStart == TRUE)
				return TRUE; // should try again
		}
	}

	return FALSE; // should NOT try again
}

static UINT32 _NMR_VdoTrig_TopTime(UINT32 pathID, UINT32 *top_time, UINT32 frm_idx)
{
	UINT32 i = 0;
	UINT32 idx = 0;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu(&flags);

	idx = frm_idx%4;

	for (i = 0; i < NMI_VDOENC_TOP_MAX; i++) {
		//get current process time
		gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiCur[idx] = top_time[i];

		//get min process time
		if (gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMin == 0) {
			gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMin = top_time[i];
		} else {
			if (top_time[i] < gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMin) {
				gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMin = top_time[i];
			}
		}

		//get max process time
		if (top_time[i] > gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMax) {
			gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMax = top_time[i];
		}
	}
	NMR_VdoEnc_Unlock_cpu(&flags);
	return 0;
}

#ifdef VDOENC_LL
static INT32 NMR_VdoTrig_WrapBS(void *info, void *usr_data)
{
	NMI_VDOENC_BS_INFO			*pUserData;
	UINT32						pathID = 0;

	MP_VDOENC_PARAM 			*pVidEncParam;
	pVidEncParam = (MP_VDOENC_PARAM *)info;
	pUserData = (NMI_VDOENC_BS_INFO *)usr_data;
	pathID = pUserData->PathID;

	NMR_VdoTrig_PutJob_WrapBS(pathID);
	NMR_VdoTrig_TrigWrapBS();

	return 0;
}

static INT32 _NMR_VdoTrig_TrigAndWrapBSInfo(UINT32 pathID)
{

	MP_VDOENC_PARAM       	    *pVidEncParam = 0;
	NMR_VDOTRIG_OBJ             *pVidTrigObj;
	UINT32                      IsIFrame = 0, IsIDR2Cut = 0, sec = 0;
//	UINT32                      osdtime = 0;
	MP_VDOENC_YUV_SRC           *p_YuvSrc;
	NMI_VDOENC_BS_INFO          *vidBSinfo;
	ER                          ER_Code;

	p_YuvSrc = gNMRVdoEncObj[pathID].pYuvSrc;
	pVidEncParam = &(gNMRVdoTrigObj[pathID].EncParam);
	pVidTrigObj =  &(gNMRVdoTrigObj[pathID]);
	vidBSinfo = &(gNMRVdoTrigObj[pathID].vidBSinfo);//(NMI_VDOENC_BS_INFO *)usr_data;
	ER_Code = pVidEncParam->encode_err ? E_SYS : E_OK;

	if (gNMRVidTrigStop[pathID] == 1) {
		vidBSinfo->PathID = pathID;
		vidBSinfo->BufID = gNMRVdoEncObj[pathID].pYuvSrc->uiBufID;
		vidBSinfo->Occupied = 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) vidBSinfo);
		}

		return 0;
	}

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_PROCESS) {
		p_YuvSrc->uiProcessTime = Perf_GetCurrent() - p_YuvSrc->uiProcessTime;
	}

	#ifdef ISF_TS
	if (p_YuvSrc) {
		vidBSinfo->PathID = pathID;
		vidBSinfo->BufID = p_YuvSrc->uiBufID;
		vidBSinfo->Occupied = 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_ENC_END, (UINT32) vidBSinfo);
		}
	}
	#endif

	pVidTrigObj->EncTime = Perf_GetCurrent() - pVidTrigObj->EncTime;

	if (ER_Code == E_OK) {
		pVidTrigObj->uiEncStatus = NMR_VDOENC_ENC_STATUS_ENCODE_OK_DONE;

		if (gNMRVdoTrigFakeIPLStop[pathID]) {
			DBG_DUMP("gNMRVdoTrigFakeIPLStop[%d] no save entry\r\n", pathID);
			goto TRIG_AND_WAIT_EXIT;
		}

		if (pVidEncParam->bs_addr_1 != pVidTrigObj->BsNow) { //last enc space not enough, re-encode and reset output addr
			pVidTrigObj->BsNow = pVidEncParam->bs_addr_1;
		}

		vidBSinfo->PathID = pathID;
		vidBSinfo->BufID = p_YuvSrc->uiBufID;
		vidBSinfo->TimeStamp = p_YuvSrc->TimeStamp;

		if ((gNMRVdoTrigObj[pathID].Encoder.GetInfo)(VDOENC_GET_ISIFRAME, &IsIFrame, 0, 0) != E_OK) {
			DBG_ERR("[VDOTRIG][%d] GETINFO_ISIFRAME fail\r\n", pathID);
		}

		if (pVidTrigObj->SyncFrameN != 0 && pVidTrigObj->FrameCount % pVidTrigObj->SyncFrameN == 0) {
			IsIDR2Cut = 1;
		}

		vidBSinfo->IsKey = IsIFrame;
		vidBSinfo->IsIDR2Cut = IsIDR2Cut;  //sec aligned

		if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
			if (IsIFrame) { //h.264 or live view doesn't need VPS+SPS+PPS combined with IDR frame, but TS need
				if (gNMRVdoEncObj[pathID].uiRecFormat != MEDIAREC_LIVEVIEW && gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_TS) {
					vidBSinfo->Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift; //skip mod part, keep vid desc and enc data
					vidBSinfo->Size = pVidEncParam->bs_size_1; //vid desc + enc data
				} else {
					vidBSinfo->Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift + gNMRVdoEncObj[pathID].uiDescSize;
					vidBSinfo->Size = pVidEncParam->bs_size_1 - gNMRVdoEncObj[pathID].uiDescSize;
				}
				pVidEncParam->bs_size_1 += pVidEncParam->bs_shift;
			} else {
				vidBSinfo->Addr = pVidTrigObj->BsNow;
				vidBSinfo->Size = pVidEncParam->bs_size_1;
			}
		} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
			if (IsIFrame) {
				if (gNMRVdoEncObj[pathID].uiRecFormat == MEDIAREC_LIVEVIEW) { //live view doesn't need VPS+SPS+PPS combined with IDR frame
					vidBSinfo->Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift + gNMRVdoEncObj[pathID].uiDescSize;
					vidBSinfo->Size = pVidEncParam->bs_size_1 - gNMRVdoEncObj[pathID].uiDescSize;
				} else {
					vidBSinfo->Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift; //skip mod part, keep vid desc and enc data
					vidBSinfo->Size = pVidEncParam->bs_size_1; //vid desc + enc data
				}
				pVidEncParam->bs_size_1 += pVidEncParam->bs_shift;
			} else {
				vidBSinfo->Addr = pVidTrigObj->BsNow;
				vidBSinfo->Size = pVidEncParam->bs_size_1;
			}
		} else {
			vidBSinfo->Addr = pVidTrigObj->BsNow;
			vidBSinfo->Size = pVidEncParam->bs_size_1;
		}

		vidBSinfo->RawYAddr   = pVidEncParam->raw_y_addr;
		vidBSinfo->SVCSize    = pVidEncParam->svc_hdr_size;
		vidBSinfo->TemporalId = pVidEncParam->temproal_id;
		vidBSinfo->FrameType  = pVidEncParam->frm_type;
		vidBSinfo->uiBaseQP   = pVidEncParam->base_qp;
		vidBSinfo->uiMotionRatio = pVidEncParam->motion_ratio;
		vidBSinfo->y_mse      = pVidEncParam->y_mse;
		vidBSinfo->u_mse      = pVidEncParam->u_mse;
		vidBSinfo->v_mse      = pVidEncParam->v_mse;
		vidBSinfo->nalu_info_addr = ALIGN_CEIL_64(pVidTrigObj->BsNow + pVidEncParam->bs_size_1); // this must sync with following
		vidBSinfo->evbr_state = pVidEncParam->evbr_still_flag;

		if (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER) {
			if (gNMRVdoTrigObj[pathID].QueueCount > 1) { //can release current frame
				vidBSinfo->Occupied = 0;
			} else { //check again if input new frame when encoding
				gNMRVdoTrigObj[pathID].QueueCount = NMR_VdoTrig_GetYuvCount(pathID);
				if (gNMRVdoTrigObj[pathID].QueueCount > 1) {
					NMR_VdoTrig_GetYuv(pathID, &p_YuvSrc, FALSE);
					vidBSinfo->Occupied = 0;
				} else {
					vidBSinfo->Occupied = 1;
					p_YuvSrc->uiEncoded = 1;
				}
			}
		} else {
			vidBSinfo->Occupied = 0;
		}

		if (vidBSinfo->SrcOutWidth) {
			vidBSinfo->SrcOutYAddr  = pVidEncParam->src_out_y_addr;
			vidBSinfo->SrcOutUVAddr = pVidEncParam->src_out_c_addr;
			vidBSinfo->SrcOutYLoft  = pVidEncParam->src_out_y_line_offset;
			vidBSinfo->SrcOutUVLoft = pVidEncParam->src_out_c_line_offset;
		}

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_PROCESS) {
			static UINT32 out_size[NMR_VDOENC_MAX_PATH][240] = {0};  //assume max framerate = 240
			static UINT32 cur_idx[NMR_VDOENC_MAX_PATH] = {0};
			UINT32 avg_byterate = 0, i=0;
			UINT32 fr = gNMRVdoEncObj[pathID].InitInfo.frame_rate/1000;
			if (gNMRVdoEncObj[pathID].InitInfo.frame_rate < 1000) fr = 1;

			out_size[pathID][cur_idx[pathID]] = vidBSinfo->Size;
			cur_idx[pathID] = (cur_idx[pathID] + 1) % fr;
			for (i=0; i<fr; i++) {
				avg_byterate += out_size[pathID][i];
			}
			if (gNMRVdoEncObj[pathID].InitInfo.frame_rate < 1000) {
#ifdef __KERNEL__
				avg_byterate = (UINT32)div_u64((UINT64)avg_byterate * gNMRVdoEncObj[pathID].InitInfo.frame_rate, 1000);
#else
				avg_byterate = (UINT32)((UINT64)avg_byterate * gNMRVdoEncObj[pathID].InitInfo.frame_rate / 1000);
#endif
			}

			DBG_DUMP("[VDOTRIG][%d] Output Size = %7d, IsIFrame = %d, FrameType = %d, Process time = %6d us, QP = %2u, Bitrate = %8u\r\n",
                pathID, vidBSinfo->Size, vidBSinfo->IsKey, vidBSinfo->FrameType, p_YuvSrc->uiProcessTime, vidBSinfo->uiBaseQP, avg_byterate * 8);
		}

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OUTPUT) {
			char *p = (char *) vidBSinfo->Addr;
			DBG_DUMP("[VDOTRIG][%d] Output Addr = 0x%08x, Size = %d, Occupied = %d, IsIFrame = %d, FrameType = %d, Start Code = 0x%02x%02x%02x%02x%02x%02x%02x%02x, time = %u us\r\n",
                pathID, vidBSinfo->Addr, vidBSinfo->Size, vidBSinfo->Occupied, vidBSinfo->IsKey, vidBSinfo->FrameType, *p, *(p+1), *(p+2), *(p+3), *(p+4), *(p+5), *(p+6), *(p+7), Perf_GetCurrent());
		}

		gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_SW_TIME] = pVidTrigObj->EncTime - pVidEncParam->encode_time;
		gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_HW_TIME] = pVidEncParam->encode_time;
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_ENCTIME) {
			DBG_DUMP("[VDOTRIG][%d] Enc time(total) = %d us, HW enc time = %d us (diff = %d us), Size = %d, IsIFrame = %d, FrameType = %d, TimeStamp = %lld\r\n",
				pathID, pVidTrigObj->EncTime, pVidEncParam->encode_time, (pVidTrigObj->EncTime - pVidEncParam->encode_time), vidBSinfo->Size, vidBSinfo->IsKey, vidBSinfo->FrameType, vidBSinfo->TimeStamp);
		}

#if NMR_VDOTRIG_DUMP_ENC_FRAME
		static BOOL bFirst = TRUE;
		static FST_FILE fp = NULL;
		if (bFirst) {
			char sFileName[260];
			snprintf(sFileName, sizeof(sFileName), "A:\\%d_%d.jpg", gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height);
			fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
			FileSys_WriteFile(fp, (UINT8 *)vidBSinfo->Addr, &vidBSinfo->Size, 0, NULL);
			FileSys_CloseFile(fp);
			bFirst = FALSE;
		}
#endif

#ifdef __KERNEL__
#if NMR_VDOTRIG_DUMP_ENC_FRAME_KERNEL_VER
		{
			static int iCount = 0;
			int g_wo_fp = 0;
			int len = 0;

			if (iCount % 30 == 0) {   // dump BS to file every 30 frame
				char filename[260];
				snprintf(filename, sizeof(filename), "/mnt/sd/VDOENC_%d_%d_%03d.jpg", gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height, iCount/30);

				// OPEN
				g_wo_fp = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);

				if ((VOS_FILE)(-1) != g_wo_fp) {
					// WRITE
					len = vos_file_write(g_wo_fp, (void *)vidBSinfo->Addr, vidBSinfo->Size);

					// CLOSE
					vos_file_close(g_wo_fp);
				} else {
					DBG_ERR("[VDOTRIG] DUMP_ENC_FRAME open file failed:%s\n", filename);
				}
			}
			iCount++;
		}
#endif
#endif
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID),  ISF_IN_PROBE_PROC,  ISF_OK); // [IN]  PROC_OK
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_OK); // [OUT] PROC_OK

#if NMR_VDOTRIG_DUMP_SNAPSHOT
		if (vidBSinfo->SrcOutYAddr != 0) {
			static BOOL bSnapshotFirst = TRUE;
			if (bSnapshotFirst) {
				char sSnapshotFileName[260];
				FST_FILE fpSnapshot = NULL;
				snprintf(sSnapshotFileName, sizeof(sSnapshotFileName), "A:\\%d_%d.yuv", vidBSinfo->SrcOutYLoft, (vidBSinfo->SrcOutUVAddr - vidBSinfo->SrcOutYAddr)/vidBSinfo->SrcOutYLoft);
				fpSnapshot = FileSys_OpenFile(sSnapshotFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
				FileSys_WriteFile(fpSnapshot, (UINT8 *)gNMRVdoEncObj[pathID].MemInfo.uiSnapshotAddr, &gNMRVdoEncObj[pathID].MemInfo.uiSnapshotSize, 0, NULL);
				FileSys_CloseFile(fpSnapshot);
				bSnapshotFirst = FALSE;
			}
		}
#endif

#ifdef __KERNEL__
#if NMR_VDOTRIG_DUMP_SNAPSHOT_KERNEL_VER
		if (vidBSinfo->SrcOutYAddr != 0) {
			int g_wo_fp = 0;
			int len = 0;
			UINT32 w = gNMRVdoEncObj[pathID].InitInfo.width;
			UINT32 h = gNMRVdoEncObj[pathID].InitInfo.height;

			char filename[260];
			snprintf(filename, sizeof(filename), "/mnt/sd/VDOENC_%d_%d.yuv", gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height);
			//DBG_DUMP("[VDOTRIG] SrcOut , Y = 0x%08x, UV = 0x%08x, W = %u, H= %u, Y_loff = %u, UV_loff = %u\r\n", vidBSinfo.SrcOutYAddr, vidBSinfo.SrcOutUVAddr, vidBSinfo.SrcOutWidth, vidBSinfo.SrcOutHeight, vidBSinfo.SrcOutYLoft, vidBSinfo.SrcOutUVLoft);

			// OPEN
			g_wo_fp = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);

			if ((VOS_FILE)(-1) != g_wo_fp) {
				// WRITE
				len = vos_file_write(g_wo_fp, (void *)vidBSinfo->SrcOutYAddr , (w*h));
				len = vos_file_write(g_wo_fp, (void *)vidBSinfo->SrcOutUVAddr, (w*h/2));

				// CLOSE
				vos_file_close(g_wo_fp);
				DBG_DUMP("[VDOTRIG] DUMP_SNAPSHOT success !! (%s)\r\n", filename);
			} else {
				DBG_ERR("[VDOTRIG] DUMP_SNAPSHOT_FRAME open file failed:%s\n", filename);
			}
		}
#endif
#endif

		// Update BSOutNow & Size
		pVidTrigObj->BsNow = ALIGN_CEIL_64(pVidTrigObj->BsNow + pVidEncParam->bs_size_1); // 1st BStream Frame starts at H264 Description in AVI file.

		// nalu info, Jira : IVOT_N01004_CO-865
		if ((gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) || (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265))
		{
			static UINT32 sCountNaluInfo[NMR_VDOENC_MAX_PATH] = {0};
			UINT32 idx = 0;
			UINT32 bs_addr = 0;
			UINT32 *nalu_len_addr = (UINT32*)pVidEncParam->nalu_size_addr;
			UINT32 nalu_num = pVidEncParam->nalu_num;
			UINT32 total_size = 0;

			// nalu_num check
			if (nalu_num > _nmr_vdotrig_nalu_num_max()) {
				if (sCountNaluInfo[pathID]++ % 300 == 0) {
					DBG_WRN("[VDOTRIG][%d] nalu_num(%d) exceed limit(%d), SKIP nalu info output !!\r\n", pathID, nalu_num, _nmr_vdotrig_nalu_num_max());
				}
				nalu_num = 0; // only "nalu_num = 0" later
			}

			// nalu_num
			*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = nalu_num;
			total_size += sizeof(UINT32);

			// nalu addr/size
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				bs_addr = (gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_GET_BS_START_PA, (UINT32) vidBSinfo);
			}
			for (idx=0; idx<nalu_num; idx++) {
				// nalu_addr
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr;
				total_size += sizeof(UINT32);

				// nalu_size
				if ((IsIFrame) && (idx==0)) {
					*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize);
					bs_addr += (nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize); // update bs_addr
				}else{
					*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = nalu_len_addr[idx];
					bs_addr += nalu_len_addr[idx]; // update bs_addr
				}
				total_size += sizeof(UINT32);
			}

			total_size = ALIGN_CEIL_64(total_size);

			// flush cache
			#if 1  // don't need to flush, because we let nalu_addr be 64x , and size be 64x  => (Update) with BS_QUICK_ROLLBACK maybe wrong, so enable again
			vos_cpu_dcache_sync((UINT32)pVidTrigObj->BsNow, total_size, VOS_DMA_TO_DEVICE);
			#endif

			// update BsNow
			pVidTrigObj->BsNow += total_size;
		} else {
			UINT32 bs_addr = 0;
			UINT32 total_size = 0;

			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				bs_addr = (gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_GET_BS_START_PA, (UINT32) vidBSinfo);
			}

			// nalu_num
			*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = 1;
			total_size += sizeof(UINT32);

			// nalu_addr
			*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr;
			total_size += sizeof(UINT32);

			// nalu_size
			*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = vidBSinfo->Size;
			total_size += sizeof(UINT32);

			total_size = ALIGN_CEIL_64(total_size);

			// flush cache
			#if 1  // don't need to flush, because we let nalu_addr be 64x , and size be 64x  => (Update) with BS_QUICK_ROLLBACK maybe wrong, so enable again
			vos_cpu_dcache_sync((UINT32)pVidTrigObj->BsNow, total_size, VOS_DMA_TO_DEVICE);
			#endif

			// update BsNow
			pVidTrigObj->BsNow += total_size;
		}

		// meta info, Jira : NA51089-793
		if (gNMRVdoEncObj[pathID].pMetaAllocInfo) {
			NMI_VDOENC_META_ALLOC_INFO *p_user_meta_data = (NMI_VDOENC_META_ALLOC_INFO *)&gNMRVdoEncObj[pathID].pMetaAllocInfo[0];
			UINT32 header_size = 0, buffer_size = 0;
			UINT32 total_size = 0;

			while (p_user_meta_data != NULL) {
				header_size = p_user_meta_data->header_size;
				buffer_size = p_user_meta_data->data_size;

				// meta signature
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = p_user_meta_data->sign;
				total_size += sizeof(UINT32);

				// meta p_next => pa
				if (p_user_meta_data->p_next) {
					*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (nvtmpp_sys_va2pa(pVidTrigObj->BsNow) + total_size - sizeof(UINT32) + header_size + buffer_size);
				} else {
					*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = 0;
				}
				total_size += sizeof(UINT32);

				// meta header_size
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = header_size;
				total_size += sizeof(UINT32);

				// meta buffer_size
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = buffer_size;
				total_size += sizeof(UINT32);

				//meta header
				//memset((void *)(pVidTrigObj->BsNow + total_size), 0x00, (header_size - sizeof(UINT32)*4));
				total_size += (header_size - sizeof(UINT32)*4);

				//meta buffer
				//memset((void *)(pVidTrigObj->BsNow + total_size), 0x00, buffer_size);
				total_size += buffer_size;

				//next pointer
				p_user_meta_data = p_user_meta_data->p_next;
			}

			// flush cache
	#if 1  // don't need to flush, because we let nalu_addr be 64x , and size be 64x  => (Update) with BS_QUICK_ROLLBACK maybe wrong, so enable again
			vos_cpu_dcache_sync((UINT32)pVidTrigObj->BsNow, total_size, VOS_DMA_TO_DEVICE);
	#endif

			// update BsNow
			pVidTrigObj->BsNow += total_size;
		}

		// callback isf_vdoenc to put PullQueue
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			NMR_VdoTrig_LockBS(pathID, vidBSinfo->Addr, vidBSinfo->Size);
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) vidBSinfo); // the do_probe REL will do on StreamReadyCB, with (Occupy = 0 & Addr !=0)
		}
		pVidTrigObj->uiEncStatus = NMR_VDOENC_ENC_STATUS_BS_CB_DONE;

		// Reserve size for next bs
		if (gNMRVdoEncObj[pathID].uiRecFormat != MEDIAREC_LIVEVIEW && gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_TS) {
			pVidTrigObj->BsNow += ALIGN_CEIL_4(NMEDIAREC_TS_PES_HEADER_LENGTH + NMEDIAREC_TS_NAL_LENGTH);
		}

		if (gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_JPG) {
			pVidTrigObj->BsNow += ALIGN_CEIL_4(NMEDIAREC_JPG_EXIF_HEADER_LENGTH);
		}

		if (gNMRVdoEncObj[pathID].uiBsReservedSize > 0) {
			pVidTrigObj->BsNow += ALIGN_CEIL_4(gNMRVdoEncObj[pathID].uiBsReservedSize);
		}

		if (pVidTrigObj->BsNow >= pVidTrigObj->BsMax && pVidTrigObj->WantRollback == FALSE) {
			if (gNMRVdoTrigMsg) {
				DBG_DUMP("Rollback, now=%x, max=%x\r\n", pVidTrigObj->BsNow, pVidTrigObj->BsMax);
			}
			pVidTrigObj->WantRollback = TRUE;
		}

		pVidTrigObj->FrameCount++;
		if (gNMRVdoEncObj[pathID].bSkipFrm) {
			pVidTrigObj->SkipModeFrameIdx ++;
		}
		gNMRVdoEncObj[pathID].uiEncOutCount++;
		if (pVidEncParam->re_encode_en) {
			gNMRVdoEncObj[pathID].uiEncReCount++;
		}

		if (pVidTrigObj->WantRollback) {
			pVidTrigObj->BsNow = pVidTrigObj->BsStart;
			pVidTrigObj->WantRollback = FALSE;
		}

		// Sync Bitstream data address
		if (IsIDR2Cut && pVidTrigObj->FrameCount != 0) {
			gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_RESET;
			if (pathID == gNMRVdoEncFirstPathID) {
				sec = pVidTrigObj->FrameCount / pVidTrigObj->SyncFrameN;
				if (gNMRVdoEncObj[pathID].CallBackFunc) {
					(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_SEC_CB, sec);
				}
			}
		}//if match sync number
	}//if encode ok
	else {
		pVidTrigObj->uiEncStatus = NMR_VDOENC_ENC_STATUS_ENCODE_FAIL_DONE;

		DBG_IND("[VDOTRIG][%d] Encode Error, Y Addr = 0x%08x, UV Addr = 0x%08x, YLoft = %d, UVLoft = %d\r\n", pathID, p_YuvSrc->uiYAddr, p_YuvSrc->uiCbAddr, p_YuvSrc->uiYLineOffset, p_YuvSrc->uiUVLineOffset);
		vidBSinfo->PathID = pathID;
		vidBSinfo->BufID = p_YuvSrc->uiBufID;
		vidBSinfo->Occupied = (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER && gNMRVdoTrigObj[pathID].QueueCount == 1) ? 1 : 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) vidBSinfo);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID),  ISF_IN_PROBE_PROC_ERR,  ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
		}

		pVidTrigObj->BsNow = pVidTrigObj->BsStart;
		gNMRVdoEncObj[pathID].uiEncErrCount++;

		// JIRA:IVOT_N01002_CO-565.  Encode failed, rec buffer had been half-overwritten by this failed frame..... Have to do reset-I for next frame
		gNMRVdoEncObj[pathID].bResetIFrame = TRUE;

		// set srcout for next frame ( because this frame encode fail, try again next frame )
		if (pVidEncParam->src_out_en == 1) {
			gNMRVdoEncObj[pathID].bSnapshot = 1;
		}
	}

TRIG_AND_WAIT_EXIT:
#if 0
	// unlock OSD & MASK
	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OSD) {
		osdtime = Perf_GetCurrent();
	}
	_isf_vdoenc_finish_input_mask(&isf_vdoenc, ISF_IN(pathID));
	_isf_vdoenc_finish_input_osd(&isf_vdoenc, ISF_IN(pathID));
	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OSD) {
		DBG_DUMP("[VDOTRIG][%d] finish_osd %d us\r\n", pathID, Perf_GetCurrent() - osdtime);
	}
#endif
	_NMR_VdoTrig_TopTime(pathID, gNMRVdoEncObj[pathID].uiEncTopTime, p_YuvSrc->uiRawCount);

	SEM_SIGNAL(NMR_VDOENC_TRIG_SEM_ID[pathID]);

	return E_OK;
}

static void _NMR_VdoTrig_TrigAndWait(UINT32 pathID, UINT32 action)
{
    MP_VDOENC_SMART_ROI_INFO    SmartRoi = {0};
	MP_VDOENC_PARAM       		*pVidEncParam = 0;
	NMR_VDOTRIG_OBJ             *pVidTrigObj;
	NMI_VDOENC_BS_INFO          *pvidBSinfo;
	UINT32                      vidDescAddr, vidDescSize;
//	UINT32                      osdtime = 0;

	SEM_WAIT(NMR_VDOENC_TRIG_SEM_ID[pathID]);

	// Check Smart Roi
    if (gNMRVdoEncObj[pathID].bWaitSmartRoi == TRUE) {
        if (NMR_VdoTrig_GetSmartRoiCount(pathID) > 0) {
            NMR_VdoTrig_GetSmartRoi(pathID, &SmartRoi);
            (gNMRVdoTrigObj[pathID].Encoder.SetInfo)(VDOENC_SET_SMART_ROI, (UINT32)&SmartRoi, 0, 0);
        } else {
		SEM_SIGNAL(NMR_VDOENC_TRIG_SEM_ID[pathID]);
        	return;
        }
    }

	gNMRVdoTrigObj[pathID].QueueCount = NMR_VdoTrig_GetYuvCount(pathID);
	pvidBSinfo = &(gNMRVdoTrigObj[pathID].vidBSinfo);

	if (gNMRVdoEncObj[pathID].bStart == FALSE && gNMRVdoTrigObj[pathID].QueueCount > 0) {
		NMR_VdoTrig_GetYuv(pathID, &gNMRVdoEncObj[pathID].pYuvSrc, FALSE);
		if (gNMRVdoEncObj[pathID].pYuvSrc) {
			pvidBSinfo->PathID = pathID;
			pvidBSinfo->BufID = gNMRVdoEncObj[pathID].pYuvSrc->uiBufID;
			pvidBSinfo->Occupied = 0;
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) pvidBSinfo);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
			}
		}
		DBG_DUMP("[VDOTRIG][%d] stopping, remaining queue count = %d\r\n", pathID, gNMRVdoTrigObj[pathID].QueueCount);
		SEM_SIGNAL(NMR_VDOENC_TRIG_SEM_ID[pathID]);
		return;
	}

	if (gNMRVdoTrigObj[pathID].QueueCount == 1 && gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER) {
		NMR_VdoTrig_GetYuv(pathID, &gNMRVdoEncObj[pathID].pYuvSrc, TRUE);
	} else {  //dvcam queue count > 1 or liveview mode
		NMR_VdoTrig_GetYuv(pathID, &gNMRVdoEncObj[pathID].pYuvSrc, FALSE);

		if (gNMRVdoEncObj[pathID].pYuvSrc && gNMRVdoEncObj[pathID].pYuvSrc->uiEncoded) {  //already encoded, release directly (Note: only TIMER mode could let uiEncoded=TRUE)
			pvidBSinfo->PathID = pathID;
			pvidBSinfo->BufID = gNMRVdoEncObj[pathID].pYuvSrc->uiBufID;
			pvidBSinfo->Occupied = 0;
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_REL, ISF_ENTER); // [IN] REL_ENTER <== last time callback with Occupied=1 , so the YUV is not release yet. and now do the normal release ! (but with Addr=0, so have to count here!!!)
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) pvidBSinfo);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_REL, ISF_OK);    // [IN] REL_OK    <== last time callback with Occupied=1 , so the YUV is not release yet. and now do the normal release ! (but with Addr=0, so have to count here!!!)
			}
			gNMRVdoTrigObj[pathID].QueueCount = NMR_VdoTrig_GetYuvCount(pathID);  // it's possible that one extra yuv is coming after the prior one is released !!
			NMR_VdoTrig_GetYuv(pathID, &gNMRVdoEncObj[pathID].pYuvSrc, ((gNMRVdoTrigObj[pathID].QueueCount == 1) ? TRUE : FALSE));
			//DBG_DUMP("[VDOTRIG][%d] release encoded frame, current queue count = %d\r\n", pathID, gNMRVdoTrigObj[pathID].QueueCount);
		}
	}

	if (!gNMRVdoEncObj[pathID].pYuvSrc) {
		if (gNMRVdoEncObj[pathID].bStart == TRUE) {
			DBG_ERR("[VDOTRIG][%d] count = %d, yuv src is null\r\n", pathID, gNMRVdoTrigObj[pathID].QueueCount);
		}
		SEM_SIGNAL(NMR_VDOENC_TRIG_SEM_ID[pathID]);
		return;
	}

	if (gNMRVdoEncObj[pathID].bResetIFrameByTimeStamp) {
		if (gNMRVdoEncObj[pathID].pYuvSrc->TimeStamp > gNMRVdoEncObj[pathID].uiTargetTimeStamp) {
			DBG_ERR("[VDOTRIG][%d] Reset I frame fail, YuvSrc TimeStamp %lld > uiTargetTimeStamp %lld\r\n", pathID, gNMRVdoEncObj[pathID].pYuvSrc->TimeStamp, gNMRVdoEncObj[pathID].uiTargetTimeStamp);
			gNMRVdoEncObj[pathID].bResetIFrameByTimeStamp = FALSE;
			gNMRVdoEncObj[pathID].uiTargetTimeStamp = 0;
		} else if (gNMRVdoEncObj[pathID].pYuvSrc->TimeStamp == gNMRVdoEncObj[pathID].uiTargetTimeStamp) {
			MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);
			if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_REQ_I) {
				DBG_DUMP("[VDOTRIG][%d] Reset I frame, YuvSrc TimeStamp %lld, uiTargetTimeStamp %lld\r\n", pathID, gNMRVdoEncObj[pathID].pYuvSrc->TimeStamp, gNMRVdoEncObj[pathID].uiTargetTimeStamp);
			}
			gNMRVdoEncObj[pathID].bResetIFrameByTimeStamp = FALSE;
			gNMRVdoEncObj[pathID].uiTargetTimeStamp = 0;
			DBG_IND("[VDOTRIG][%d] Reset I frame, time = %u us\r\n", pathID, Perf_GetCurrent());
			if ((pVidEnc) && (pVidEnc->SetInfo)) {
				(pVidEnc->SetInfo)(VDOENC_SET_RESET_IFRAME, 0, 0, 0);
			}
			if (gNMRVdoTrigObj[pathID].SyncFrameN != 0 && gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN != 0) {
				gNMRVdoTrigObj[pathID].FrameCount += gNMRVdoTrigObj[pathID].SyncFrameN - (gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN);
				if (gNMRVdoEncObj[pathID].bSkipFrm) {
					gNMRVdoTrigObj[pathID].SkipModeFrameIdx += gNMRVdoTrigObj[pathID].SyncFrameN - (gNMRVdoTrigObj[pathID].SkipModeFrameIdx % gNMRVdoTrigObj[pathID].SyncFrameN);
				}
			}
			gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type = NMR_VDOENC_IDR_SLICE;
		} else { //if (gNMRVdoEncObj[pathID].pYuvSrc->TimeStamp < gNMRVdoEncObj[pathID].uiTargetTimeStamp)
			if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_REQ_I) {
				DBG_DUMP("[VDOTRIG][%d] YuvSrc TimeStamp %lld, uiTargetTimeStamp %lld\r\n", pathID, gNMRVdoEncObj[pathID].pYuvSrc->TimeStamp, gNMRVdoEncObj[pathID].uiTargetTimeStamp);
			}
		}
	}

	//notify image unit by SIE frame count
	{
		pvidBSinfo->PathID = pathID;
		pvidBSinfo->FrameCount = gNMRVdoEncObj[pathID].pYuvSrc->uiRawCount;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_RAW_FRAME_COUNT, (UINT32) pvidBSinfo);
		}
		memset(pvidBSinfo, 0, sizeof(NMI_VDOENC_BS_INFO));
	}

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_SMART_ROI) {
		DBG_DUMP("[VDOTRIG][%d] YUV time = %lld, Smart Roi time = %lld\r\n", pathID, gNMRVdoEncObj[pathID].pYuvSrc->TimeStamp, SmartRoi.TimeStamp);
	}

	if (gNMRVdoEncObj[pathID].bSkipLoffCheck == FALSE) {
		if (gNMRVdoEncObj[pathID].pYuvSrc->uiWidth != gNMRVdoEncObj[pathID].InitInfo.width) {
			pvidBSinfo->PathID = pathID;
			pvidBSinfo->BufID = gNMRVdoEncObj[pathID].pYuvSrc->uiBufID;
			pvidBSinfo->Occupied = (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER && gNMRVdoTrigObj[pathID].QueueCount == 1) ? 1 : 0;
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (ULONG) pvidBSinfo);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA); // [IN] PUSH_ERR
			}
			DBG_DUMP("[VDOTRIG][%d] drop mismatch YUV width = %d, venc width = %d\r\n", pathID, gNMRVdoEncObj[pathID].pYuvSrc->uiWidth, gNMRVdoEncObj[pathID].InitInfo.width);
			SEM_SIGNAL(NMR_VDOENC_TRIG_SEM_ID[pathID]);
			return;
		}
	}

	if (action & NMR_VDOTRIG_ACT_PRELOAD) {
		if (gNMRVdoTrigMsg) {
			DBG_DUMP("preload imgage..\r\n");
		}
		//(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(MP_VDOENC_SETINFO_PRELOAD, 0, 0, 0);
		SEM_SIGNAL(NMR_VDOENC_TRIG_SEM_ID[pathID]);
		return;
	}

	pVidEncParam = &(gNMRVdoTrigObj[pathID].EncParam);
	pVidTrigObj =  &(gNMRVdoTrigObj[pathID]);
	NMR_VdoEnc_GetDesc(pathID, &vidDescAddr, &vidDescSize);

	//notify image unit by frame interval
	if (gNMRVdoEncObj[pathID].uiFrameInterval != 0 && pVidTrigObj->FrameCount % gNMRVdoEncObj[pathID].uiFrameInterval == 0) {
		pvidBSinfo->PathID = pathID;
		pvidBSinfo->FrameCount = pVidTrigObj->FrameCount;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_FRAME_INTERVAL, (UINT32) pvidBSinfo);
		}
		memset(pvidBSinfo, 0, sizeof(NMI_VDOENC_BS_INFO));
	}

	pVidEncParam->y_addr = gNMRVdoEncObj[pathID].pYuvSrc->uiYAddr;
	pVidEncParam->c_addr = gNMRVdoEncObj[pathID].pYuvSrc->uiCbAddr;
	pVidEncParam->y_line_offset = gNMRVdoEncObj[pathID].pYuvSrc->uiYLineOffset;
	pVidEncParam->c_line_offset = gNMRVdoEncObj[pathID].pYuvSrc->uiUVLineOffset;
#if defined(_BSP_NA51000_)
	// 680 use this
	pVidEncParam->uiSrcCompression = gNMRVdoEncObj[pathID].pYuvSrc->uiSrcCompression;
	if (pVidEncParam->uiSrcCompression) {
		memcpy((void *) & (pVidEncParam->SrcCompressInfo), (void *) & (gNMRVdoEncObj[pathID].pYuvSrc->SrcCompressInfo), sizeof(MP_VDOENC_SRC_COMPRESS_INFO));
	}
#elif defined(_BSP_NA51055_)
	// 520 use this
	pVidEncParam->st_sdc.enable = gNMRVdoEncObj[pathID].pYuvSrc->uiSrcCompression;

	if (pVidEncParam->st_sdc.enable) {
		pVidEncParam->st_sdc.width   = gNMRVdoEncObj[pathID].pYuvSrc->SrcCompressInfo.width;
		pVidEncParam->st_sdc.height  = gNMRVdoEncObj[pathID].pYuvSrc->SrcCompressInfo.height;
		pVidEncParam->st_sdc.y_lofst = gNMRVdoEncObj[pathID].pYuvSrc->SrcCompressInfo.y_lofst;
		pVidEncParam->st_sdc.c_lofst = gNMRVdoEncObj[pathID].pYuvSrc->SrcCompressInfo.c_lofst;
	}
#endif

	// Low Latency Mode
	pVidEncParam->src_d2d_en = gNMRVdoEncObj[pathID].pYuvSrc->LowLatencyInfo.enable;

	if (pVidEncParam->src_d2d_en) {
		pVidEncParam->src_d2d_mode         = (gNMRVdoEncObj[pathID].pYuvSrc->LowLatencyInfo.strp_num - 1);
		pVidEncParam->src_d2d_strp_size[0] = gNMRVdoEncObj[pathID].pYuvSrc->LowLatencyInfo.strp_size[0];
		pVidEncParam->src_d2d_strp_size[1] = gNMRVdoEncObj[pathID].pYuvSrc->LowLatencyInfo.strp_size[1];
		pVidEncParam->src_d2d_strp_size[2] = gNMRVdoEncObj[pathID].pYuvSrc->LowLatencyInfo.strp_size[2];
	}

	// Backup current BS output address for
	// 1. update size for AVI file (00dc+size)
	// 2. restore start address for H264 2 B-frames encoding
	//check whether enc buf is enough
	if (gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type == NMR_VDOENC_IDR_SLICE) {
		pVidTrigObj->BsSize = gNMRVdoEncObj[pathID].MemInfo.uiMinISize;
	} else {
		pVidTrigObj->BsSize = gNMRVdoEncObj[pathID].MemInfo.uiMinPSize;
	}

	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_ENTER); // [OUT] NEW_ENTER

TRY_FIND_FRAME_QUEUE:
	if (gNMRVdoEncObj[pathID].uiMaxFrameQueue != 0 && NMR_VdoTrig_GetBSCount(pathID) >= gNMRVdoEncObj[pathID].uiMaxFrameQueue) {
		static UINT32 sCount[NMR_VDOENC_MAX_PATH];

		if (TRUE == _NMR_VdoTrig_YUV_TMOUT_RetryCheck(pathID, gNMRVdoEncObj[pathID].pYuvSrc))
			goto TRY_FIND_FRAME_QUEUE; // try again

		// max frame queue exceed, give up & release this YUV
		pvidBSinfo->PathID = pathID;
		pvidBSinfo->BufID = gNMRVdoEncObj[pathID].pYuvSrc->uiBufID;
		pvidBSinfo->Occupied = (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER && gNMRVdoTrigObj[pathID].QueueCount == 1) ? 1 : 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) pvidBSinfo);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
		}

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_DROP) {
			DBG_WRN("[VDOTRIG][%d] queue frame count >= %d, drop frame idx = %lld, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiMaxFrameQueue, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		}

		if (sCount[pathID] % 30 == 0) {
			DBG_WRN("[VDOTRIG][%d] queue frame count >= %d, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiMaxFrameQueue, Perf_GetCurrent());
		}
		sCount[pathID]++;

		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL); // [OUT] NEW_WRN

		SEM_SIGNAL(NMR_VDOENC_TRIG_SEM_ID[pathID]);

		return;
	}

TRY_FIND_BS_QUEUE:
	if (NMR_VdoTrig_CheckBS(pathID, &(pVidTrigObj->BsNow), &(pVidTrigObj->BsSize), &(pVidTrigObj->BsStart)) != TRUE) {
		static UINT32 sCount[NMR_VDOENC_MAX_PATH];

		if (TRUE == _NMR_VdoTrig_YUV_TMOUT_RetryCheck(pathID, gNMRVdoEncObj[pathID].pYuvSrc))
			goto TRY_FIND_BS_QUEUE; // try again

		// can't find bs buffer, give up $ release this YUV
		pvidBSinfo->PathID = pathID;
		pvidBSinfo->BufID = gNMRVdoEncObj[pathID].pYuvSrc->uiBufID;
		pvidBSinfo->Occupied = (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER && gNMRVdoTrigObj[pathID].QueueCount == 1) ? 1 : 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) pvidBSinfo);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
		}

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_DROP) {
			DBG_WRN("[VDOTRIG][%d] size not enough(0x%08x, %d), drop frame idx = %lld, time = %u us\r\n", pathID, pVidTrigObj->BsNow, pVidTrigObj->BsSize, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		}

		if (sCount[pathID] % 30 == 0) {
			DBG_WRN("[VDOTRIG][%d] input frame idx = %lld, size not enough, drop frame, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		}
		sCount[pathID]++;

		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL); // [OUT] NEW_WRN

		SEM_SIGNAL(NMR_VDOENC_TRIG_SEM_ID[pathID]);

		return;
	}

	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_OK); // [OUT] NEW_OK

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OUTPUT) {
		DBG_DUMP("[VDOTRIG][%d] Next Frame type = %d, Bs Addr = 0x%08x, Size = %d\r\n",
                pathID, gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type, pVidTrigObj->BsNow, pVidTrigObj->BsSize);
	}

	//set info
	pVidEncParam->bs_start_addr = pVidTrigObj->BsStart;
	pVidEncParam->bs_addr_1     = pVidTrigObj->BsNow;
	pVidEncParam->bs_size_1     = pVidTrigObj->BsSize;
	pVidEncParam->bs_end_addr   = pVidTrigObj->BsEnd;

	if (gNMRVdoEncObj[pathID].bSkipFrm) {
		// ex. 10->30, 15->30
		if (gNMRVdoEncObj[pathID].uiSkipFrmTargetFr % gNMRVdoEncObj[pathID].uiSkipFrmInputCnt == 0) {
			UINT32 ratio = gNMRVdoEncObj[pathID].uiSkipFrmTargetFr / gNMRVdoEncObj[pathID].uiSkipFrmInputCnt;
			if (gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type == NMR_VDOENC_IDR_SLICE) {
				gNMRVdoTrigObj[pathID].SkipModeFrameIdx = 0;
			}
			if (gNMRVdoTrigObj[pathID].SkipModeFrameIdx % ratio == 0 || gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type == NMR_VDOENC_IDR_SLICE) {
				pVidEncParam->skip_frm_en = FALSE;
			} else {
				pVidEncParam->skip_frm_en = TRUE;
			}
		// ex. 20->30, 25->30
		} else if (gNMRVdoEncObj[pathID].uiSkipFrmTargetFr % (gNMRVdoEncObj[pathID].uiSkipFrmTargetFr - gNMRVdoEncObj[pathID].uiSkipFrmInputCnt) == 0) {
			UINT32 ratio = gNMRVdoEncObj[pathID].uiSkipFrmTargetFr / (gNMRVdoEncObj[pathID].uiSkipFrmTargetFr - gNMRVdoEncObj[pathID].uiSkipFrmInputCnt);
			if (gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type == NMR_VDOENC_IDR_SLICE) {
				gNMRVdoTrigObj[pathID].SkipModeFrameIdx = 0;
			}
			if ((gNMRVdoTrigObj[pathID].SkipModeFrameIdx + 1) % ratio == 0 && gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type != NMR_VDOENC_IDR_SLICE) {
				pVidEncParam->skip_frm_en = TRUE;
			} else {
				pVidEncParam->skip_frm_en = FALSE;
			}
		}
	} else {
		pVidEncParam->skip_frm_en = FALSE;
	}

	// reserve size for nalu info
	if (pVidEncParam->bs_size_1 > _nmr_vdotrig_nalu_size_info_max_size()) {
		pVidEncParam->bs_size_1 -= _nmr_vdotrig_nalu_size_info_max_size();
	} else {
		DBG_WRN("[VDOTRIG][%d] input frame idx = %lld, nalu size not enough, drop frame, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		return;
	}

#if defined(_BSP_NA51000_)
	// Set Md
	if (gNMRVdoEncObj[pathID].bStartSmartRoi && gNMRVdoEncObj[pathID].RoiInfo.uiRoiCount == 0)
	{
		UINT8 ucBCBlkSize;

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_MD) {
			pVidTrigObj->EncTime = Perf_GetCurrent();
		}
		#if 0
		IMG_BUF sImgBuf = {0};
		bc_getforeground(&sImgBuf);
		gNMRVdoEncObj[pathID].MdInfo.uiMbWidthNum = sImgBuf.Width;
		gNMRVdoEncObj[pathID].MdInfo.uiMbHeightNum = sImgBuf.Height;
		gNMRVdoEncObj[pathID].MdInfo.uiMbWidth = 1;
		gNMRVdoEncObj[pathID].MdInfo.uiMbHeight = 1;
		gNMRVdoEncObj[pathID].MdInfo.pMDBitMap = (UINT8 *)sImgBuf.PxlAddr[0];
		#else
		ucBCBlkSize = 32;
		gNMRVdoEncObj[pathID].MdInfo.mb_width_num = gNMRVdoEncObj[pathID].pYuvSrc->uiWidth/ucBCBlkSize;
		gNMRVdoEncObj[pathID].MdInfo.mb_height_num = gNMRVdoEncObj[pathID].pYuvSrc->uiHeight/ucBCBlkSize;
		gNMRVdoEncObj[pathID].MdInfo.mb_width = ucBCBlkSize;
		gNMRVdoEncObj[pathID].MdInfo.mb_height = ucBCBlkSize;
		bc_getforeground_v2(gNMRVdoEncObj[pathID].MdInfo.p_md_bitmap, gNMRVdoEncObj[pathID].MdInfo.mb_width_num, gNMRVdoEncObj[pathID].MdInfo.mb_height_num, 255, 0);
		#endif
		gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_MD_TIME] = Perf_GetCurrent() - pVidTrigObj->EncTime;
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_MD) {
			DBG_DUMP("[VDOTRIG][%d] Num = (%d, %d), size = (%d, %d), convert time = %d us\r\n",
				pathID, gNMRVdoEncObj[pathID].MdInfo.mb_width_num, gNMRVdoEncObj[pathID].MdInfo.mb_height_num, gNMRVdoEncObj[pathID].MdInfo.mb_width, gNMRVdoEncObj[pathID].MdInfo.mb_height, gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_MD_TIME]);
		}
		(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(VDOENC_SET_MD, (UINT32)&gNMRVdoEncObj[pathID].MdInfo, 0, 0);
	}
#elif defined(_BSP_NA51055_)
	// Set Md
	if (gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.md_buf_adr != 0)
	{
		gNMRVdoEncObj[pathID].MdInfo.md_width = gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.md_width;
		gNMRVdoEncObj[pathID].MdInfo.md_height = gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.md_height;
		gNMRVdoEncObj[pathID].MdInfo.md_lofs = gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.md_lofs;
		gNMRVdoEncObj[pathID].MdInfo.md_buf_adr = gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.md_buf_adr;
		if (gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.rotation != 0) {
			gNMRVdoEncObj[pathID].MdInfo.rotation = gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.rotation;
		} else {
			gNMRVdoEncObj[pathID].MdInfo.rotation = gNMRVdoEncObj[pathID].InitInfo.rotate;
		}
		gNMRVdoEncObj[pathID].MdInfo.roi_xy = gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.roi_xy;
		gNMRVdoEncObj[pathID].MdInfo.roi_wh = gNMRVdoEncObj[pathID].pYuvSrc->MDInfo.roi_wh;

		pVidTrigObj->EncTime = Perf_GetCurrent();

		(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(VDOENC_SET_MD, (UINT32)&gNMRVdoEncObj[pathID].MdInfo, 0, 0);
		gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_MD_TIME] = Perf_GetCurrent() - pVidTrigObj->EncTime;
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_MD) {
			DBG_DUMP("[VDOTRIG][%d] size = (%d, %d), loffset = %d, buf_adr = 0x%x, rotation = 0x%x, roi_xy = 0x%x, roi_wh = 0x%x, convert time = %d us\r\n",
				pathID, gNMRVdoEncObj[pathID].MdInfo.md_width, gNMRVdoEncObj[pathID].MdInfo.md_height, gNMRVdoEncObj[pathID].MdInfo.md_lofs, gNMRVdoEncObj[pathID].MdInfo.md_buf_adr, gNMRVdoEncObj[pathID].MdInfo.rotation, gNMRVdoEncObj[pathID].MdInfo.roi_xy, gNMRVdoEncObj[pathID].MdInfo.roi_wh, Perf_GetCurrent() - pVidTrigObj->EncTime);
		}
	} else {
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_MD) {
			DBG_DUMP("[VDOTRIG][%d] MD Buff Address is 0 !!!\r\n", pathID);
		}
	}
#endif

#if defined(_BSP_NA51000_)  // 680 have to call reset between each frame
	(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(MP_VDOENC_SETINFO_RESET, 0, 0, 0);
#endif
#if 0
#if 0
	if (gNMRVdoEncObj[pathID].CallBackFunc) {
		(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_STAMP_CB, (UINT32) gNMRVdoEncObj[pathID].pYuvSrc);
	}
#else
	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OSD) {
		osdtime = Perf_GetCurrent();
	}
	_isf_vdoenc_do_input_mask(&isf_vdoenc, ISF_IN(pathID), (void *) gNMRVdoEncObj[pathID].pYuvSrc, NULL, FALSE);
	_isf_vdoenc_do_input_osd(&isf_vdoenc, ISF_IN(pathID), (void *) gNMRVdoEncObj[pathID].pYuvSrc, NULL, FALSE);
	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OSD) {
		DBG_DUMP("[VDOTRIG][%d] do_osd %d us\r\n", pathID, Perf_GetCurrent() - osdtime);
	}
#endif
#endif
	if (gNMRVdoEncObj[pathID].bSnapshot) {
		if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
			if ((gNMRVdoEncObj[pathID].InitInfo.rotate == 0) || (gNMRVdoEncObj[pathID].InitInfo.rotate == 3)) {
				pvidBSinfo->SrcOutWidth = gNMRVdoEncObj[pathID].InitInfo.width;
				pvidBSinfo->SrcOutHeight = gNMRVdoEncObj[pathID].InitInfo.height;
				pvidBSinfo->SrcOutYLoft = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.width);
				pvidBSinfo->SrcOutUVLoft = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.width);
			} else {
				pvidBSinfo->SrcOutWidth = gNMRVdoEncObj[pathID].InitInfo.height;
				pvidBSinfo->SrcOutHeight = gNMRVdoEncObj[pathID].InitInfo.width;
				pvidBSinfo->SrcOutYLoft = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.height);
				pvidBSinfo->SrcOutUVLoft = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.height);
			}
		} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264){
#if 0
			if (pVidEncParam->st_sdc.enable == 1 && gNMRVdoEncObj[pathID].InitInfo.rotate != 0) {
				DBG_ERR("[VDOTRIG][%d] H.264 not support rotating src out in src compressed mode\r\n", pathID);
				gNMRVdoEncObj[pathID].bSnapshot = FALSE;
			} else
#endif
			{
				if ((gNMRVdoEncObj[pathID].InitInfo.rotate == 0) || (gNMRVdoEncObj[pathID].InitInfo.rotate == 3)) {
					pvidBSinfo->SrcOutWidth = gNMRVdoEncObj[pathID].InitInfo.width;
					pvidBSinfo->SrcOutHeight = gNMRVdoEncObj[pathID].InitInfo.height;
					pvidBSinfo->SrcOutYLoft = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.width);
					pvidBSinfo->SrcOutUVLoft = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.width);
				} else {
					pvidBSinfo->SrcOutWidth = gNMRVdoEncObj[pathID].InitInfo.height;
					pvidBSinfo->SrcOutHeight = gNMRVdoEncObj[pathID].InitInfo.width;
					pvidBSinfo->SrcOutYLoft = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.height);
					pvidBSinfo->SrcOutUVLoft = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.height);
				}
			}
		} else {
			DBG_ERR("[VDOTRIG][%d] Snapshot unsupported codec format = %d\r\n", pathID, gNMRVdoEncObj[pathID].uiVidCodec);
			gNMRVdoEncObj[pathID].bSnapshot = FALSE;
		}

		if (gNMRVdoEncObj[pathID].bSnapshot) {
			pvidBSinfo->SrcOutYAddr = gNMRVdoEncObj[pathID].MemInfo.uiSnapshotAddr;
			if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
				pvidBSinfo->SrcOutUVAddr = pvidBSinfo->SrcOutYAddr + (pvidBSinfo->SrcOutYLoft * ALIGN_CEIL_64(pvidBSinfo->SrcOutHeight));
			} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
				if ((gNMRVdoEncObj[pathID].InitInfo.rotate == 0)||(gNMRVdoEncObj[pathID].InitInfo.rotate == 3))
					pvidBSinfo->SrcOutUVAddr = pvidBSinfo->SrcOutYAddr + (pvidBSinfo->SrcOutYLoft * ALIGN_CEIL_16(pvidBSinfo->SrcOutHeight));
				else
					pvidBSinfo->SrcOutUVAddr = pvidBSinfo->SrcOutYAddr + (pvidBSinfo->SrcOutYLoft * ALIGN_CEIL_32(pvidBSinfo->SrcOutHeight));
			}

			pVidEncParam->src_out_en = 1;
#if (!NMR_VDOENC_SRCOUT_USE_REF_BUF)
			//#NT#2018/10/26#Adam Su -begin
			//#NT#IVOT_N00017_CO-144: Need SrcOut buffer to avoid ImgCap yuv corruption
			if (gNMRVdoEncObj[pathID].bAllocSnapshotBuf) {
				pVidEncParam->src_out_y_addr        = pvidBSinfo->SrcOutYAddr;
				pVidEncParam->src_out_c_addr        = pvidBSinfo->SrcOutUVAddr;
				pVidEncParam->src_out_y_line_offset = pvidBSinfo->SrcOutYLoft;
				pVidEncParam->src_out_c_line_offset = pvidBSinfo->SrcOutUVLoft;
			} else {
				pVidEncParam->src_out_y_addr        = 0;
				pVidEncParam->src_out_c_addr        = 0;
				pVidEncParam->src_out_y_line_offset = 0;
				pVidEncParam->src_out_c_line_offset = 0;
			}
			//#NT#2018/10/26#Adam Su -end
#else
			pVidEncParam->src_out_y_addr        = 0;
			pVidEncParam->src_out_c_addr        = 0;
			pVidEncParam->src_out_y_line_offset = 0;
			pVidEncParam->src_out_c_line_offset = 0;
#endif
			gNMRVdoEncObj[pathID].bSnapshot = FALSE;
		}
	} else {
		pVidEncParam->src_out_en            = 0;  // [520] kdrv will not auto-clean ( [680] was at dal_h26x ), have to clean here
		pVidEncParam->src_out_y_addr        = 0;
		pVidEncParam->src_out_c_addr        = 0;
		pVidEncParam->src_out_y_line_offset = 0;
		pVidEncParam->src_out_c_line_offset = 0;
	}

	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID),  ISF_IN_PROBE_PUSH, ISF_OK);     // [IN] PUSH_OK
	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID),  ISF_IN_PROBE_PROC, ISF_ENTER);  // [IN] PROC_ENTER
	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_ENTER); // [OUT] PROC_ENTER

	// trigger encode and wait for ready
	#ifdef ISF_TS
	if (gNMRVdoEncObj[pathID].pYuvSrc) {
		pvidBSinfo->PathID = pathID;
		pvidBSinfo->BufID = gNMRVdoEncObj[pathID].pYuvSrc->uiBufID;
		pvidBSinfo->Occupied = 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_ENC_BEGIN, (UINT32) pvidBSinfo);
		}
	}
	#endif

	pVidTrigObj->EncTime = Perf_GetCurrent();

#if ISF_LATENCY_DEBUG
	pvidBSinfo->enc_timestamp_start = HwClock_GetLongCounter();
#endif

	pvidBSinfo->PathID = pathID;
	pVidTrigObj->uiEncStatus = NMR_VDOENC_ENC_STATUS_ENCODE_TRIG;

	(gNMRVdoTrigObj[pathID].Encoder.EncodeOne)(0, pVidEncParam, &pVidTrigObj->Enc_CallBack, pvidBSinfo);

#if ISF_LATENCY_DEBUG
	pvidBSinfo->enc_timestamp_end = HwClock_GetLongCounter();
#endif

	return;
}
#else
static void _NMR_VdoTrig_TrigAndWait(UINT32 pathID, UINT32 action)
{
	ER                          ER_Code;
	MP_VDOENC_YUV_SRC     		*p_YuvSrc = 0;
    MP_VDOENC_SMART_ROI_INFO    SmartRoi = {0};
	MP_VDOENC_PARAM       		*pVidEncParam = 0;
	NMR_VDOTRIG_OBJ             *pVidTrigObj;
	NMI_VDOENC_BS_INFO          vidBSinfo = {0};
	UINT32                      sec;
	UINT32                      vidDescAddr, vidDescSize;
	UINT32                      uiQueueCount = 0;
	UINT32                      IsIFrame = 0;
	UINT32                      IsIDR2Cut = 0;
	UINT32                      enctime = 0;
	UINT32                      osdtime = 0;
	UINT32                      top_time[NMI_VDOENC_TOP_MAX] = {0};
	UINT32                      len1 = 0, len2 = 0;

#ifdef __KERNEL__
	if (kdrv_builtin_is_fastboot() && gNMRVidTrig1stopen[pathID] && !gNMRVidTrigBuiltinBSDone[pathID]) {
		VdoEnc_Builtin_CheckBuiltinStop(pathID);
		NMR_VdoEnc_GetBuiltinBsData(pathID);
		gNMRVidTrigBuiltinBSDone[pathID] = 1;
	}
#endif

	// Check Smart Roi
    if (gNMRVdoEncObj[pathID].bWaitSmartRoi == TRUE) {
        if (NMR_VdoTrig_GetSmartRoiCount(pathID) > 0) {
            NMR_VdoTrig_GetSmartRoi(pathID, &SmartRoi);
            (gNMRVdoTrigObj[pathID].Encoder.SetInfo)(VDOENC_SET_SMART_ROI, (UINT32)&SmartRoi, 0, 0);
        } else {
        	return;
        }
    }

	uiQueueCount = NMR_VdoTrig_GetYuvCount(pathID);

	if (gNMRVdoEncObj[pathID].bStart == FALSE && uiQueueCount > 0) {
		NMR_VdoTrig_GetYuv(pathID, &p_YuvSrc, FALSE);
		if (p_YuvSrc) {
			vidBSinfo.PathID = pathID;
			vidBSinfo.BufID = p_YuvSrc->uiBufID;
			vidBSinfo.Occupied = 0;
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
			}
		}
		DBG_DUMP("[VDOTRIG][%d] stopping, remaining queue count = %d\r\n", pathID, uiQueueCount);
		return;
	}

	if (uiQueueCount == 1 && gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER) {
		NMR_VdoTrig_GetYuv(pathID, &p_YuvSrc, TRUE);
	} else {  //dvcam queue count > 1 or liveview mode
		NMR_VdoTrig_GetYuv(pathID, &p_YuvSrc, FALSE);

		if (p_YuvSrc && p_YuvSrc->uiEncoded) {  //already encoded, release directly (Note: only TIMER mode could let uiEncoded=TRUE)
			vidBSinfo.PathID = pathID;
			vidBSinfo.BufID = p_YuvSrc->uiBufID;
			vidBSinfo.Occupied = 0;
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_REL, ISF_ENTER); // [IN] REL_ENTER <== last time callback with Occupied=1 , so the YUV is not release yet. and now do the normal release ! (but with Addr=0, so have to count here!!!)
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_REL, ISF_OK);    // [IN] REL_OK    <== last time callback with Occupied=1 , so the YUV is not release yet. and now do the normal release ! (but with Addr=0, so have to count here!!!)
			}
			uiQueueCount = NMR_VdoTrig_GetYuvCount(pathID);  // it's possible that one extra yuv is coming after the prior one is released !!
			NMR_VdoTrig_GetYuv(pathID, &p_YuvSrc, ((uiQueueCount == 1) ? TRUE : FALSE));
			//DBG_DUMP("[VDOTRIG][%d] release encoded frame, current queue count = %d\r\n", pathID, uiQueueCount);
		}
	}

	if (!p_YuvSrc) {
		if (gNMRVdoEncObj[pathID].bStart == TRUE) {
			DBG_ERR("[VDOTRIG][%d] count = %d, yuv src is null\r\n", pathID, uiQueueCount);
		}
		return;
	}

	if (gNMRVdoEncObj[pathID].bResetIFrameByTimeStamp) {
		if (p_YuvSrc->TimeStamp > gNMRVdoEncObj[pathID].uiTargetTimeStamp) {
			DBG_ERR("[VDOTRIG][%d] Reset I frame fail, YuvSrc TimeStamp %lld > uiTargetTimeStamp %lld\r\n", pathID, p_YuvSrc->TimeStamp, gNMRVdoEncObj[pathID].uiTargetTimeStamp);
			gNMRVdoEncObj[pathID].bResetIFrameByTimeStamp = FALSE;
			gNMRVdoEncObj[pathID].uiTargetTimeStamp = 0;
		} else if (p_YuvSrc->TimeStamp == gNMRVdoEncObj[pathID].uiTargetTimeStamp) {
			MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);
			if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_REQ_I) {
				DBG_DUMP("[VDOTRIG][%d] Reset I frame, YuvSrc TimeStamp %lld, uiTargetTimeStamp %lld\r\n", pathID, p_YuvSrc->TimeStamp, gNMRVdoEncObj[pathID].uiTargetTimeStamp);
			}
			gNMRVdoEncObj[pathID].bResetIFrameByTimeStamp = FALSE;
			gNMRVdoEncObj[pathID].uiTargetTimeStamp = 0;
			DBG_IND("[VDOTRIG][%d] Reset I frame, time = %u us\r\n", pathID, Perf_GetCurrent());
			if ((pVidEnc) && (pVidEnc->SetInfo)) {
				(pVidEnc->SetInfo)(VDOENC_SET_RESET_IFRAME, 0, 0, 0);
			}
			if (gNMRVdoTrigObj[pathID].SyncFrameN != 0 && gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN != 0) {
				gNMRVdoTrigObj[pathID].FrameCount += gNMRVdoTrigObj[pathID].SyncFrameN - (gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN);
				if (gNMRVdoEncObj[pathID].bSkipFrm) {
					gNMRVdoTrigObj[pathID].SkipModeFrameIdx += gNMRVdoTrigObj[pathID].SyncFrameN - (gNMRVdoTrigObj[pathID].SkipModeFrameIdx % gNMRVdoTrigObj[pathID].SyncFrameN);
				}
			}
			gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type = NMR_VDOENC_IDR_SLICE;
		} else { //if (p_YuvSrc->TimeStamp < gNMRVdoEncObj[pathID].uiTargetTimeStamp)
			if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_REQ_I) {
				DBG_DUMP("[VDOTRIG][%d] YuvSrc TimeStamp %lld, uiTargetTimeStamp %lld\r\n", pathID, p_YuvSrc->TimeStamp, gNMRVdoEncObj[pathID].uiTargetTimeStamp);
			}
		}
	}

	//notify image unit by SIE frame count
	{
		vidBSinfo.PathID = pathID;
		vidBSinfo.FrameCount = p_YuvSrc->uiRawCount;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_RAW_FRAME_COUNT, (UINT32) &vidBSinfo);
		}
		memset(&vidBSinfo, 0, sizeof(NMI_VDOENC_BS_INFO));
	}

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_SMART_ROI) {
		DBG_DUMP("[VDOTRIG][%d] YUV time = %lld, Smart Roi time = %lld\r\n", pathID, p_YuvSrc->TimeStamp, SmartRoi.TimeStamp);
	}

	if (gNMRVdoEncObj[pathID].bSkipLoffCheck == FALSE) {
		if (p_YuvSrc->uiWidth != gNMRVdoEncObj[pathID].InitInfo.width) {
			vidBSinfo.PathID = pathID;
			vidBSinfo.BufID = p_YuvSrc->uiBufID;
			vidBSinfo.Occupied = (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER && uiQueueCount == 1) ? 1 : 0;
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (ULONG) &vidBSinfo);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA); // [IN] PUSH_ERR
			}
			DBG_DUMP("[VDOTRIG][%d] drop mismatch YUV width = %d, venc width = %d\r\n", pathID, p_YuvSrc->uiWidth, gNMRVdoEncObj[pathID].InitInfo.width);
			return;
		}
	}

	if (action & NMR_VDOTRIG_ACT_PRELOAD) {
		if (gNMRVdoTrigMsg) {
			DBG_DUMP("preload imgage..\r\n");
		}
		//(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(MP_VDOENC_SETINFO_PRELOAD, 0, 0, 0);
		return;
	}

	pVidEncParam = &(gNMRVdoTrigObj[pathID].EncParam);
	pVidTrigObj =  &(gNMRVdoTrigObj[pathID]);
	NMR_VdoEnc_GetDesc(pathID, &vidDescAddr, &vidDescSize);

	//notify image unit by frame interval
	if (gNMRVdoEncObj[pathID].uiFrameInterval != 0 && pVidTrigObj->FrameCount % gNMRVdoEncObj[pathID].uiFrameInterval == 0) {
		vidBSinfo.PathID = pathID;
		vidBSinfo.FrameCount = pVidTrigObj->FrameCount;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_FRAME_INTERVAL, (UINT32) &vidBSinfo);
		}
		memset(&vidBSinfo, 0, sizeof(NMI_VDOENC_BS_INFO));
	}

	pVidEncParam->y_addr = p_YuvSrc->uiYAddr;
	pVidEncParam->c_addr = p_YuvSrc->uiCbAddr;
	pVidEncParam->y_line_offset = p_YuvSrc->uiYLineOffset;
	pVidEncParam->c_line_offset = p_YuvSrc->uiUVLineOffset;
#if defined(_BSP_NA51000_)
	// 680 use this
	pVidEncParam->uiSrcCompression = p_YuvSrc->uiSrcCompression;
	if (pVidEncParam->uiSrcCompression) {
		memcpy((void *) & (pVidEncParam->SrcCompressInfo), (void *) & (p_YuvSrc->SrcCompressInfo), sizeof(MP_VDOENC_SRC_COMPRESS_INFO));
	}
#elif defined(_BSP_NA51055_)
	// 520 use this
	pVidEncParam->st_sdc.enable = p_YuvSrc->uiSrcCompression;

	if (pVidEncParam->st_sdc.enable) {
		pVidEncParam->st_sdc.width   = p_YuvSrc->SrcCompressInfo.width;
		pVidEncParam->st_sdc.height  = p_YuvSrc->SrcCompressInfo.height;
		pVidEncParam->st_sdc.y_lofst = p_YuvSrc->SrcCompressInfo.y_lofst;
		pVidEncParam->st_sdc.c_lofst = p_YuvSrc->SrcCompressInfo.c_lofst;
	}
#endif

	// Low Latency Mode
	pVidEncParam->src_d2d_en = p_YuvSrc->LowLatencyInfo.enable;

	if (pVidEncParam->src_d2d_en) {
		pVidEncParam->src_d2d_mode         = (p_YuvSrc->LowLatencyInfo.strp_num - 1);
		pVidEncParam->src_d2d_strp_size[0] = p_YuvSrc->LowLatencyInfo.strp_size[0];
		pVidEncParam->src_d2d_strp_size[1] = p_YuvSrc->LowLatencyInfo.strp_size[1];
		pVidEncParam->src_d2d_strp_size[2] = p_YuvSrc->LowLatencyInfo.strp_size[2];
	}

	// Backup current BS output address for
	// 1. update size for AVI file (00dc+size)
	// 2. restore start address for H264 2 B-frames encoding
	//check whether enc buf is enough
	if (gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type == NMR_VDOENC_IDR_SLICE) {
		pVidTrigObj->BsSize = gNMRVdoEncObj[pathID].MemInfo.uiMinISize;
	} else {
		pVidTrigObj->BsSize = gNMRVdoEncObj[pathID].MemInfo.uiMinPSize;
	}

	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_ENTER); // [OUT] NEW_ENTER

TRY_FIND_FRAME_QUEUE:
	if (gNMRVdoEncObj[pathID].uiMaxFrameQueue != 0 && NMR_VdoTrig_GetBSCount(pathID) >= gNMRVdoEncObj[pathID].uiMaxFrameQueue) {
		static UINT32 sCount[NMR_VDOENC_MAX_PATH];

		if (TRUE == _NMR_VdoTrig_YUV_TMOUT_RetryCheck(pathID, p_YuvSrc))
			goto TRY_FIND_FRAME_QUEUE; // try again

		// max frame queue exceed, give up & release this YUV
		vidBSinfo.PathID = pathID;
		vidBSinfo.BufID = p_YuvSrc->uiBufID;
		vidBSinfo.Occupied = (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER && uiQueueCount == 1) ? 1 : 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
		}

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_DROP) {
			DBG_WRN("[VDOTRIG][%d] queue frame count >= %d, drop frame idx = %lld, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiMaxFrameQueue, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		}

		if (sCount[pathID] % 30 == 0) {
			DBG_WRN("[VDOTRIG][%d] queue frame count >= %d, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiMaxFrameQueue, Perf_GetCurrent());
		}
		sCount[pathID]++;

		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL); // [OUT] NEW_WRN

		return;
	}

TRY_FIND_BS_QUEUE:
	if (NMR_VdoTrig_CheckBS(pathID, &(pVidTrigObj->BsNow), &(pVidTrigObj->BsSize), &(pVidTrigObj->BsNow2), &(pVidTrigObj->BsSize2), &(pVidTrigObj->BsStart)) != TRUE) {
		static UINT32 sCount[NMR_VDOENC_MAX_PATH];

		if (TRUE == _NMR_VdoTrig_YUV_TMOUT_RetryCheck(pathID, p_YuvSrc))
			goto TRY_FIND_BS_QUEUE; // try again

		// can't find bs buffer, give up $ release this YUV
		vidBSinfo.PathID = pathID;
		vidBSinfo.BufID = p_YuvSrc->uiBufID;
		vidBSinfo.Occupied = (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER && uiQueueCount == 1) ? 1 : 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
		}

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_DROP) {
			DBG_WRN("[VDOTRIG][%d] size not enough(0x%08x, %d), drop frame idx = %lld, time = %u us\r\n", pathID, pVidTrigObj->BsNow, pVidTrigObj->BsSize, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		}

		if (sCount[pathID] % 30 == 0) {
			DBG_WRN("[VDOTRIG][%d] input frame idx = %lld, size not enough, drop frame, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		}
		sCount[pathID]++;

		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL); // [OUT] NEW_WRN

		return;
	}

	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_NEW, ISF_OK); // [OUT] NEW_OK

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OUTPUT) {
		DBG_DUMP("[VDOTRIG][%d] Next Frame type = %d, Bs Addr = 0x%08x, Size = %d\r\n",
                pathID, gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type, pVidTrigObj->BsNow, pVidTrigObj->BsSize);
	}

	//set info
	pVidEncParam->bs_start_addr = pVidTrigObj->BsStart;
	pVidEncParam->bs_addr_1     = pVidTrigObj->BsNow;
	pVidEncParam->bs_size_1     = pVidTrigObj->BsSize;
	pVidEncParam->bs_end_addr   = pVidTrigObj->BsEnd;
	if (gNMRVdoEncObj[pathID].MemInfo.bBSRingMode == TRUE) {
		pVidEncParam->bs_addr_2 = pVidTrigObj->BsNow2;
		pVidEncParam->bs_size_2 = pVidTrigObj->BsSize2;
	}

	if (gNMRVdoEncObj[pathID].bSkipFrm) {
		// ex. 10->30, 15->30
		if (gNMRVdoEncObj[pathID].uiSkipFrmTargetFr % gNMRVdoEncObj[pathID].uiSkipFrmInputCnt == 0) {
			UINT32 ratio = gNMRVdoEncObj[pathID].uiSkipFrmTargetFr / gNMRVdoEncObj[pathID].uiSkipFrmInputCnt;
			if (gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type == NMR_VDOENC_IDR_SLICE) {
				gNMRVdoTrigObj[pathID].SkipModeFrameIdx = 0;
			}
			if (gNMRVdoTrigObj[pathID].SkipModeFrameIdx % ratio == 0 || gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type == NMR_VDOENC_IDR_SLICE) {
				pVidEncParam->skip_frm_en = FALSE;
			} else {
				pVidEncParam->skip_frm_en = TRUE;
			}
		// ex. 20->30, 25->30
		} else if (gNMRVdoEncObj[pathID].uiSkipFrmTargetFr % (gNMRVdoEncObj[pathID].uiSkipFrmTargetFr - gNMRVdoEncObj[pathID].uiSkipFrmInputCnt) == 0) {
			UINT32 ratio = gNMRVdoEncObj[pathID].uiSkipFrmTargetFr / (gNMRVdoEncObj[pathID].uiSkipFrmTargetFr - gNMRVdoEncObj[pathID].uiSkipFrmInputCnt);
			if (gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type == NMR_VDOENC_IDR_SLICE) {
				gNMRVdoTrigObj[pathID].SkipModeFrameIdx = 0;
			}
			if ((gNMRVdoTrigObj[pathID].SkipModeFrameIdx + 1) % ratio == 0 && gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type != NMR_VDOENC_IDR_SLICE) {
				pVidEncParam->skip_frm_en = TRUE;
			} else {
				pVidEncParam->skip_frm_en = FALSE;
			}
		}
	} else {
		pVidEncParam->skip_frm_en = FALSE;
	}

	// reserve size for nalu info
	if (pVidEncParam->bs_size_1 > _nmr_vdotrig_nalu_size_info_max_size()) {
		pVidEncParam->bs_size_1 -= _nmr_vdotrig_nalu_size_info_max_size();
	} else {
		DBG_WRN("[VDOTRIG][%d] input frame idx = %lld, nalu size not enough, drop frame, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		return;
	}
	if (gNMRVdoEncObj[pathID].MemInfo.bBSRingMode == TRUE && pVidEncParam->bs_addr_2 != 0 && pVidEncParam->bs_size_2 != 0) {
		if (pVidEncParam->bs_size_2 > _nmr_vdotrig_nalu_size_info_max_size()) {
			pVidEncParam->bs_size_2 -= _nmr_vdotrig_nalu_size_info_max_size();
		} else {
			pVidEncParam->bs_addr_2 = 0;
			pVidEncParam->bs_size_2 = 0;
			//DBG_WRN("[VDOTRIG][%d] input frame idx = %lld, bs_size2 not enough for nalu info, use bs_size1 only, time = %u us\r\n", pathID, gNMRVdoEncObj[pathID].uiEncInCount-1, Perf_GetCurrent());
		}
	}
#if defined(_BSP_NA51000_)
	// Set Md
	if (gNMRVdoEncObj[pathID].bStartSmartRoi && gNMRVdoEncObj[pathID].RoiInfo.uiRoiCount == 0)
	{
		UINT8 ucBCBlkSize;

		enctime = Perf_GetCurrent();

		#if 0
		IMG_BUF sImgBuf = {0};
		bc_getforeground(&sImgBuf);
		gNMRVdoEncObj[pathID].MdInfo.uiMbWidthNum = sImgBuf.Width;
		gNMRVdoEncObj[pathID].MdInfo.uiMbHeightNum = sImgBuf.Height;
		gNMRVdoEncObj[pathID].MdInfo.uiMbWidth = 1;
		gNMRVdoEncObj[pathID].MdInfo.uiMbHeight = 1;
		gNMRVdoEncObj[pathID].MdInfo.pMDBitMap = (UINT8 *)sImgBuf.PxlAddr[0];
		#else
		ucBCBlkSize = 32;
		gNMRVdoEncObj[pathID].MdInfo.mb_width_num = p_YuvSrc->uiWidth/ucBCBlkSize;
		gNMRVdoEncObj[pathID].MdInfo.mb_height_num = p_YuvSrc->uiHeight/ucBCBlkSize;
		gNMRVdoEncObj[pathID].MdInfo.mb_width = ucBCBlkSize;
		gNMRVdoEncObj[pathID].MdInfo.mb_height = ucBCBlkSize;
		bc_getforeground_v2(gNMRVdoEncObj[pathID].MdInfo.p_md_bitmap, gNMRVdoEncObj[pathID].MdInfo.mb_width_num, gNMRVdoEncObj[pathID].MdInfo.mb_height_num, 255, 0);
		#endif
		top_time[NMI_VDOENC_TOP_MD_TIME] = Perf_GetCurrent() - enctime;
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_MD) {
			DBG_DUMP("[VDOTRIG][%d] Num = (%d, %d), size = (%d, %d), convert time = %d us\r\n",
				pathID, gNMRVdoEncObj[pathID].MdInfo.mb_width_num, gNMRVdoEncObj[pathID].MdInfo.mb_height_num, gNMRVdoEncObj[pathID].MdInfo.mb_width, gNMRVdoEncObj[pathID].MdInfo.mb_height, top_time[NMI_VDOENC_TOP_MD_TIME]);
		}
		(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(VDOENC_SET_MD, (UINT32)&gNMRVdoEncObj[pathID].MdInfo, 0, 0);
	}
#elif defined(_BSP_NA51055_)
	// Set Md
	if (p_YuvSrc->MDInfo.md_buf_adr != 0)
	{
		gNMRVdoEncObj[pathID].MdInfo.md_width = p_YuvSrc->MDInfo.md_width;
		gNMRVdoEncObj[pathID].MdInfo.md_height = p_YuvSrc->MDInfo.md_height;
		gNMRVdoEncObj[pathID].MdInfo.md_lofs = p_YuvSrc->MDInfo.md_lofs;
		gNMRVdoEncObj[pathID].MdInfo.md_buf_adr = p_YuvSrc->MDInfo.md_buf_adr;
		if (p_YuvSrc->MDInfo.rotation != 0) {
			gNMRVdoEncObj[pathID].MdInfo.rotation = p_YuvSrc->MDInfo.rotation;
		} else {
			gNMRVdoEncObj[pathID].MdInfo.rotation = gNMRVdoEncObj[pathID].InitInfo.rotate;
		}
		gNMRVdoEncObj[pathID].MdInfo.roi_xy = p_YuvSrc->MDInfo.roi_xy;
		gNMRVdoEncObj[pathID].MdInfo.roi_wh = p_YuvSrc->MDInfo.roi_wh;

		enctime = Perf_GetCurrent();

		(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(VDOENC_SET_MD, (UINT32)&gNMRVdoEncObj[pathID].MdInfo, 0, 0);
		top_time[NMI_VDOENC_TOP_MD_TIME] = Perf_GetCurrent() - enctime;
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_MD) {
			DBG_DUMP("[VDOTRIG][%d] size = (%d, %d), loffset = %d, buf_adr = 0x%x, rotation = 0x%x, roi_xy = 0x%x, roi_wh = 0x%x, convert time = %d us\r\n",
				pathID, gNMRVdoEncObj[pathID].MdInfo.md_width, gNMRVdoEncObj[pathID].MdInfo.md_height, gNMRVdoEncObj[pathID].MdInfo.md_lofs, gNMRVdoEncObj[pathID].MdInfo.md_buf_adr, gNMRVdoEncObj[pathID].MdInfo.rotation, gNMRVdoEncObj[pathID].MdInfo.roi_xy, gNMRVdoEncObj[pathID].MdInfo.roi_wh, top_time[NMI_VDOENC_TOP_MD_TIME]);
		}
	} else {
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_MD) {
			DBG_DUMP("[VDOTRIG][%d] MD Buff Address is 0 !!!\r\n", pathID);
		}
	}
#endif

	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG) {
		VdoCodec_JPEG_Lock();
	} else {
		VdoCodec_H26x_Lock();
	}
#if defined(_BSP_NA51000_)  // 680 have to call reset between each frame
	(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(MP_VDOENC_SETINFO_RESET, 0, 0, 0);
#endif

#if 0
	if (gNMRVdoEncObj[pathID].CallBackFunc) {
		(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_STAMP_CB, (UINT32) p_YuvSrc);
	}
#else
	osdtime = Perf_GetCurrent();

	_isf_vdoenc_do_input_mask(&isf_vdoenc, ISF_IN(pathID), (void *) p_YuvSrc, NULL, FALSE);
	_isf_vdoenc_do_input_osd(&isf_vdoenc, ISF_IN(pathID), (void *) p_YuvSrc, NULL, FALSE);

	top_time[NMI_VDOENC_TOP_OSD_INPUT_TIME] = Perf_GetCurrent() - osdtime;

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OSD) {
		DBG_DUMP("[VDOTRIG][%d] do_osd %d us\r\n", pathID, top_time[NMI_VDOENC_TOP_OSD_INPUT_TIME]);
	}
#endif

	if (gNMRVdoEncObj[pathID].bSnapshot) {
		if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
			if ((gNMRVdoEncObj[pathID].InitInfo.rotate == 0) || (gNMRVdoEncObj[pathID].InitInfo.rotate == 3)) {
				vidBSinfo.SrcOutWidth = gNMRVdoEncObj[pathID].InitInfo.width;
				vidBSinfo.SrcOutHeight = gNMRVdoEncObj[pathID].InitInfo.height;
				vidBSinfo.SrcOutYLoft = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.width);
				vidBSinfo.SrcOutUVLoft = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.width);
			} else {
				vidBSinfo.SrcOutWidth = gNMRVdoEncObj[pathID].InitInfo.height;
				vidBSinfo.SrcOutHeight = gNMRVdoEncObj[pathID].InitInfo.width;
				vidBSinfo.SrcOutYLoft = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.height);
				vidBSinfo.SrcOutUVLoft = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.height);
			}
		} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264){
#if 0
			if (pVidEncParam->st_sdc.enable == 1 && gNMRVdoEncObj[pathID].InitInfo.rotate != 0) {
				DBG_ERR("[VDOTRIG][%d] H.264 not support rotating src out in src compressed mode\r\n", pathID);
				gNMRVdoEncObj[pathID].bSnapshot = FALSE;
			} else
#endif
			{
				if ((gNMRVdoEncObj[pathID].InitInfo.rotate == 0) || (gNMRVdoEncObj[pathID].InitInfo.rotate == 3)) {
					vidBSinfo.SrcOutWidth = gNMRVdoEncObj[pathID].InitInfo.width;
					vidBSinfo.SrcOutHeight = gNMRVdoEncObj[pathID].InitInfo.height;
					vidBSinfo.SrcOutYLoft = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.width);
					vidBSinfo.SrcOutUVLoft = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.width);
				} else {
					vidBSinfo.SrcOutWidth = gNMRVdoEncObj[pathID].InitInfo.height;
					vidBSinfo.SrcOutHeight = gNMRVdoEncObj[pathID].InitInfo.width;
					vidBSinfo.SrcOutYLoft = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.height);
					vidBSinfo.SrcOutUVLoft = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.height);
				}
			}
		} else {
			DBG_ERR("[VDOTRIG][%d] Snapshot unsupported codec format = %d\r\n", pathID, gNMRVdoEncObj[pathID].uiVidCodec);
			gNMRVdoEncObj[pathID].bSnapshot = FALSE;
		}

		if (gNMRVdoEncObj[pathID].bSnapshot) {
			vidBSinfo.SrcOutYAddr = gNMRVdoEncObj[pathID].MemInfo.uiSnapshotAddr;
			if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
				vidBSinfo.SrcOutUVAddr = vidBSinfo.SrcOutYAddr + (vidBSinfo.SrcOutYLoft * ALIGN_CEIL_64(vidBSinfo.SrcOutHeight));
			} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
				if ((gNMRVdoEncObj[pathID].InitInfo.rotate == 0)||(gNMRVdoEncObj[pathID].InitInfo.rotate == 3))
					vidBSinfo.SrcOutUVAddr = vidBSinfo.SrcOutYAddr + (vidBSinfo.SrcOutYLoft * ALIGN_CEIL_16(vidBSinfo.SrcOutHeight));
				else
					vidBSinfo.SrcOutUVAddr = vidBSinfo.SrcOutYAddr + (vidBSinfo.SrcOutYLoft * ALIGN_CEIL_32(vidBSinfo.SrcOutHeight));
			}

			pVidEncParam->src_out_en = 1;
#if (!NMR_VDOENC_SRCOUT_USE_REF_BUF)
			//#NT#2018/10/26#Adam Su -begin
			//#NT#IVOT_N00017_CO-144: Need SrcOut buffer to avoid ImgCap yuv corruption
			if (gNMRVdoEncObj[pathID].bAllocSnapshotBuf) {
				pVidEncParam->src_out_y_addr        = vidBSinfo.SrcOutYAddr;
				pVidEncParam->src_out_c_addr        = vidBSinfo.SrcOutUVAddr;
				pVidEncParam->src_out_y_line_offset = vidBSinfo.SrcOutYLoft;
				pVidEncParam->src_out_c_line_offset = vidBSinfo.SrcOutUVLoft;
			} else {
				pVidEncParam->src_out_y_addr        = 0;
				pVidEncParam->src_out_c_addr        = 0;
				pVidEncParam->src_out_y_line_offset = 0;
				pVidEncParam->src_out_c_line_offset = 0;
			}
			//#NT#2018/10/26#Adam Su -end
#else
			pVidEncParam->src_out_y_addr        = 0;
			pVidEncParam->src_out_c_addr        = 0;
			pVidEncParam->src_out_y_line_offset = 0;
			pVidEncParam->src_out_c_line_offset = 0;
#endif
			gNMRVdoEncObj[pathID].bSnapshot = FALSE;
		}
	} else {
		pVidEncParam->src_out_en            = 0;  // [520] kdrv will not auto-clean ( [680] was at dal_h26x ), have to clean here
		pVidEncParam->src_out_y_addr        = 0;
		pVidEncParam->src_out_c_addr        = 0;
		pVidEncParam->src_out_y_line_offset = 0;
		pVidEncParam->src_out_c_line_offset = 0;
	}

	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID),  ISF_IN_PROBE_PUSH, ISF_OK);     // [IN] PUSH_OK
	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID),  ISF_IN_PROBE_PROC, ISF_ENTER);  // [IN] PROC_ENTER
	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_ENTER); // [OUT] PROC_ENTER

	// trigger encode and wait for ready
	#ifdef ISF_TS
	if (p_YuvSrc) {
		vidBSinfo.PathID = pathID;
		vidBSinfo.BufID = p_YuvSrc->uiBufID;
		vidBSinfo.Occupied = 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_ENC_BEGIN, (UINT32) &vidBSinfo);
		}
	}
	#endif

	enctime = Perf_GetCurrent();


#if ISF_LATENCY_DEBUG
	vidBSinfo.enc_timestamp_start = HwClock_GetLongCounter();
#endif

	pVidTrigObj->uiEncStatus = NMR_VDOENC_ENC_STATUS_ENCODE_TRIG;

	ER_Code = (gNMRVdoTrigObj[pathID].Encoder.EncodeOne)(0, 0, 0, pVidEncParam);
	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG) {
		VdoCodec_JPEG_UnLock();
	} else {
		VdoCodec_H26x_UnLock();
	}

#if ISF_LATENCY_DEBUG
	vidBSinfo.enc_timestamp_end = HwClock_GetLongCounter();
#endif

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_PROCESS) {
		p_YuvSrc->uiProcessTime = Perf_GetCurrent() - p_YuvSrc->uiProcessTime;
	}

	#ifdef ISF_TS
	if (p_YuvSrc) {
		vidBSinfo.PathID = pathID;
		vidBSinfo.BufID = p_YuvSrc->uiBufID;
		vidBSinfo.Occupied = 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_ENC_END, (UINT32) &vidBSinfo);
		}
	}
	#endif

	enctime = Perf_GetCurrent() - enctime;


	if (ER_Code == E_OK) {
		pVidTrigObj->uiEncStatus = NMR_VDOENC_ENC_STATUS_ENCODE_OK_DONE;

		if (gNMRVdoTrigFakeIPLStop[pathID]) {
			DBG_DUMP("gNMRVdoTrigFakeIPLStop[%d] no save entry\r\n", pathID);
			goto TRIG_AND_WAIT_EXIT;
		}

		if (pVidEncParam->bs_addr_1 != pVidTrigObj->BsNow) { //last enc space not enough, re-encode and reset output addr
			pVidTrigObj->BsNow = pVidEncParam->bs_addr_1;
		}

		vidBSinfo.PathID = pathID;
		vidBSinfo.BufID = p_YuvSrc->uiBufID;
		vidBSinfo.TimeStamp = p_YuvSrc->TimeStamp;

		if ((gNMRVdoTrigObj[pathID].Encoder.GetInfo)(VDOENC_GET_ISIFRAME, &IsIFrame, 0, 0) != E_OK) {
			DBG_ERR("[VDOTRIG][%d] GETINFO_ISIFRAME fail\r\n", pathID);
		}

		if (pVidTrigObj->SyncFrameN != 0 && pVidTrigObj->FrameCount % pVidTrigObj->SyncFrameN == 0) {
			IsIDR2Cut = 1;
		}

		vidBSinfo.IsKey = IsIFrame;
		vidBSinfo.IsIDR2Cut = IsIDR2Cut;  //sec aligned

		if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG) {
			pVidEncParam->re_trigger = 0;
		}

		if (pVidEncParam->re_trigger) {
			if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264 || gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
				len1 = ALIGN_FLOOR(gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - pVidTrigObj->BsNow - _nmr_vdotrig_nalu_size_info_max_size(), 128);
				len2 = pVidEncParam->bs_size_1 - len1;
			} else {
				len1 = pVidEncParam->first_bs_length;
				len2 = pVidEncParam->bs_size_1 - len1;
			}
		}

		if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
			if (IsIFrame) { //h.264 or live view doesn't need VPS+SPS+PPS combined with IDR frame, but TS need
				if (pVidEncParam->re_trigger) {
					if (gNMRVdoEncObj[pathID].uiRecFormat != MEDIAREC_LIVEVIEW && gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_TS) {
						vidBSinfo.Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift;
						vidBSinfo.Size = len1;
						vidBSinfo.Addr2 = pVidTrigObj->BsStart;
						vidBSinfo.Size2 = len2;
					} else {
						vidBSinfo.Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift + gNMRVdoEncObj[pathID].uiDescSize;
						vidBSinfo.Size = len1 - gNMRVdoEncObj[pathID].uiDescSize;
						vidBSinfo.Addr2 = pVidTrigObj->BsStart;
						vidBSinfo.Size2 = len2;
					}
				} else {
					if (gNMRVdoEncObj[pathID].uiRecFormat != MEDIAREC_LIVEVIEW && gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_TS) {
						vidBSinfo.Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift; //skip mod part, keep vid desc and enc data
						vidBSinfo.Size = pVidEncParam->bs_size_1; //vid desc + enc data
					} else {
						vidBSinfo.Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift + gNMRVdoEncObj[pathID].uiDescSize;
						vidBSinfo.Size = pVidEncParam->bs_size_1 - gNMRVdoEncObj[pathID].uiDescSize;
					}
					pVidEncParam->bs_size_1 += pVidEncParam->bs_shift;
					vidBSinfo.Addr2 = 0;
					vidBSinfo.Size2 = 0;
				}
			} else {
				if (pVidEncParam->re_trigger) {
					vidBSinfo.Addr = pVidTrigObj->BsNow;
					vidBSinfo.Size = len1;
					vidBSinfo.Addr2 = pVidTrigObj->BsStart;
					vidBSinfo.Size2 = len2;
				} else {
					vidBSinfo.Addr = pVidTrigObj->BsNow;
					vidBSinfo.Size = pVidEncParam->bs_size_1;
					vidBSinfo.Addr2 = 0;
					vidBSinfo.Size2 = 0;
				}
			}
		} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
			if (IsIFrame) {
				if (pVidEncParam->re_trigger) {
					if (gNMRVdoEncObj[pathID].uiRecFormat == MEDIAREC_LIVEVIEW) { //live view doesn't need VPS+SPS+PPS combined with IDR frame
						vidBSinfo.Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift + gNMRVdoEncObj[pathID].uiDescSize;
						vidBSinfo.Size = len1 - gNMRVdoEncObj[pathID].uiDescSize;
						vidBSinfo.Addr2 = pVidTrigObj->BsStart;
						vidBSinfo.Size2 = len2;
					} else {
						vidBSinfo.Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift;
						vidBSinfo.Size = len1;
						vidBSinfo.Addr2 = pVidTrigObj->BsStart;
						vidBSinfo.Size2 = len2;
					}
				} else {
					if (gNMRVdoEncObj[pathID].uiRecFormat == MEDIAREC_LIVEVIEW) { //live view doesn't need VPS+SPS+PPS combined with IDR frame
						vidBSinfo.Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift + gNMRVdoEncObj[pathID].uiDescSize;
						vidBSinfo.Size = pVidEncParam->bs_size_1 - gNMRVdoEncObj[pathID].uiDescSize;
					} else {
						vidBSinfo.Addr = pVidTrigObj->BsNow + pVidEncParam->bs_shift; //skip mod part, keep vid desc and enc data
						vidBSinfo.Size = pVidEncParam->bs_size_1; //vid desc + enc data
					}
					pVidEncParam->bs_size_1 += pVidEncParam->bs_shift;
					vidBSinfo.Addr2 = 0;
					vidBSinfo.Size2 = 0;
				}
			} else {
				if (pVidEncParam->re_trigger) {
					vidBSinfo.Addr = pVidTrigObj->BsNow;
					vidBSinfo.Size = len1;
					vidBSinfo.Addr2 = pVidTrigObj->BsStart;
					vidBSinfo.Size2 = len2;
				} else {
					vidBSinfo.Addr = pVidTrigObj->BsNow;
					vidBSinfo.Size = pVidEncParam->bs_size_1;
					vidBSinfo.Addr2 = 0;
					vidBSinfo.Size2 = 0;
				}
			}
		} else {
			if (pVidEncParam->re_trigger) {
				vidBSinfo.Addr = pVidTrigObj->BsNow;
				vidBSinfo.Size = len1;
				vidBSinfo.Addr2 = pVidTrigObj->BsStart;
				vidBSinfo.Size2 = len2;
			} else {
				vidBSinfo.Addr = pVidTrigObj->BsNow;
				vidBSinfo.Size = pVidEncParam->bs_size_1;
				vidBSinfo.Addr2 = 0;
				vidBSinfo.Size2 = 0;
			}
		}

		vidBSinfo.RawYAddr   = pVidEncParam->raw_y_addr;
		vidBSinfo.SVCSize    = pVidEncParam->svc_hdr_size;
		vidBSinfo.TemporalId = pVidEncParam->temproal_id;
		vidBSinfo.FrameType  = pVidEncParam->frm_type;
		vidBSinfo.uiBaseQP   = pVidEncParam->base_qp;
		vidBSinfo.uiMotionRatio = pVidEncParam->motion_ratio;
		vidBSinfo.y_mse      = pVidEncParam->y_mse;
		vidBSinfo.u_mse      = pVidEncParam->u_mse;
		vidBSinfo.v_mse      = pVidEncParam->v_mse;
		if (pVidEncParam->re_trigger) {
			vidBSinfo.nalu_info_addr = ALIGN_CEIL_64(pVidTrigObj->BsStart + len2); // this must sync with following
		} else {
			vidBSinfo.nalu_info_addr = ALIGN_CEIL_64(pVidTrigObj->BsNow + pVidEncParam->bs_size_1); // this must sync with following
		}
		vidBSinfo.evbr_state = pVidEncParam->evbr_still_flag;

		if (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER) {
			if (uiQueueCount > 1) { //can release current frame
				vidBSinfo.Occupied = 0;
			} else { //check again if input new frame when encoding
				uiQueueCount = NMR_VdoTrig_GetYuvCount(pathID);
				if (uiQueueCount > 1) {
					NMR_VdoTrig_GetYuv(pathID, &p_YuvSrc, FALSE);
					vidBSinfo.Occupied = 0;
				} else {
					vidBSinfo.Occupied = 1;
					p_YuvSrc->uiEncoded = 1;
				}
			}
		} else {
			vidBSinfo.Occupied = 0;
		}

		if (vidBSinfo.SrcOutWidth) {
			vidBSinfo.SrcOutYAddr  = pVidEncParam->src_out_y_addr;
			vidBSinfo.SrcOutUVAddr = pVidEncParam->src_out_c_addr;
			vidBSinfo.SrcOutYLoft  = pVidEncParam->src_out_y_line_offset;
			vidBSinfo.SrcOutUVLoft = pVidEncParam->src_out_c_line_offset;
		}

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_PROCESS) {
			static UINT32 out_size[NMR_VDOENC_MAX_PATH][240] = {0};  //assume max framerate = 240
			static UINT32 cur_idx[NMR_VDOENC_MAX_PATH] = {0};
			UINT32 avg_byterate = 0, i=0;
			UINT32 fr = gNMRVdoEncObj[pathID].InitInfo.frame_rate/1000;
			if (gNMRVdoEncObj[pathID].InitInfo.frame_rate < 1000) fr = 1;

			out_size[pathID][cur_idx[pathID]] = vidBSinfo.Size;
			cur_idx[pathID] = (cur_idx[pathID] + 1) % fr;
			for (i=0; i<fr; i++) {
				avg_byterate += out_size[pathID][i];
			}
			if (gNMRVdoEncObj[pathID].InitInfo.frame_rate < 1000) {
#ifdef __KERNEL__
				avg_byterate = (UINT32)div_u64((UINT64)avg_byterate * gNMRVdoEncObj[pathID].InitInfo.frame_rate, 1000);
#else
				avg_byterate = (UINT32)((UINT64)avg_byterate * gNMRVdoEncObj[pathID].InitInfo.frame_rate / 1000);
#endif
			}
			DBG_DUMP("[VDOTRIG][%d] Output Size = %7d, IsIFrame = %d, FrameType = %d, Process time = %6d us, QP = %2u, Bitrate = %8u\r\n",
                pathID, vidBSinfo.Size, vidBSinfo.IsKey, vidBSinfo.FrameType, p_YuvSrc->uiProcessTime, vidBSinfo.uiBaseQP, avg_byterate * 8);
		}

		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OUTPUT) {
			char *p = (char *) vidBSinfo.Addr;
			DBG_DUMP("[VDOTRIG][%d] Output Addr = 0x%08x, Size = %d, Occupied = %d, IsIFrame = %d, FrameType = %d, Start Code = 0x%02x%02x%02x%02x%02x%02x%02x%02x, time = %u us\r\n",
                pathID, vidBSinfo.Addr, vidBSinfo.Size, vidBSinfo.Occupied, vidBSinfo.IsKey, vidBSinfo.FrameType, *p, *(p+1), *(p+2), *(p+3), *(p+4), *(p+5), *(p+6), *(p+7), Perf_GetCurrent());
		}

		top_time[NMI_VDOENC_TOP_SW_TIME] = enctime - pVidEncParam->encode_time;
		top_time[NMI_VDOENC_TOP_HW_TIME] = pVidEncParam->encode_time;
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_ENCTIME) {
			DBG_DUMP("[VDOTRIG][%d] Enc time(total) = %d us, HW enc time = %d us (diff = %d us), Size = %d, IsIFrame = %d, FrameType = %d, TimeStamp = %lld\r\n",
				pathID, enctime, pVidEncParam->encode_time, (enctime - pVidEncParam->encode_time), vidBSinfo.Size, vidBSinfo.IsKey, vidBSinfo.FrameType, vidBSinfo.TimeStamp);
		}

#if NMR_VDOTRIG_DUMP_ENC_FRAME
		static BOOL bFirst = TRUE;
		static FST_FILE fp = NULL;
		if (bFirst) {
			char sFileName[260];
			snprintf(sFileName, sizeof(sFileName), "A:\\%d_%d.jpg", gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height);
			fp = FileSys_OpenFile(sFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
			FileSys_WriteFile(fp, (UINT8 *)vidBSinfo.Addr, &vidBSinfo.Size, 0, NULL);
			FileSys_CloseFile(fp);
			bFirst = FALSE;
		}
#endif

#ifdef __KERNEL__
#if NMR_VDOTRIG_DUMP_ENC_FRAME_KERNEL_VER
		{
			static int iCount = 0;
			int g_wo_fp = 0;
			int len = 0;

			if (iCount % 30 == 0) {   // dump BS to file every 30 frame
				char filename[260];
				snprintf(filename, sizeof(filename), "/mnt/sd/VDOENC_%d_%d_%03d.jpg", gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height, iCount/30);

				// OPEN
				g_wo_fp = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);

				if ((VOS_FILE)(-1) != g_wo_fp) {
					// WRITE
					len = vos_file_write(g_wo_fp, (void *)vidBSinfo.Addr, vidBSinfo.Size);

					// CLOSE
					vos_file_close(g_wo_fp);
				} else {
					DBG_ERR("[VDOTRIG] DUMP_ENC_FRAME open file failed:%s\n", filename);
				}
			}
			iCount++;
		}
#endif
#endif
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID),  ISF_IN_PROBE_PROC,  ISF_OK); // [IN]  PROC_OK
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC, ISF_OK); // [OUT] PROC_OK

#if NMR_VDOTRIG_DUMP_SNAPSHOT
		if (vidBSinfo.SrcOutYAddr != 0) {
			static BOOL bSnapshotFirst = TRUE;
			if (bSnapshotFirst) {
				char sSnapshotFileName[260];
				FST_FILE fpSnapshot = NULL;
				snprintf(sSnapshotFileName, sizeof(sSnapshotFileName), "A:\\%d_%d.yuv", vidBSinfo.SrcOutYLoft, (vidBSinfo.SrcOutUVAddr - vidBSinfo.SrcOutYAddr)/vidBSinfo.SrcOutYLoft);
				fpSnapshot = FileSys_OpenFile(sSnapshotFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
				FileSys_WriteFile(fpSnapshot, (UINT8 *)gNMRVdoEncObj[pathID].MemInfo.uiSnapshotAddr, &gNMRVdoEncObj[pathID].MemInfo.uiSnapshotSize, 0, NULL);
				FileSys_CloseFile(fpSnapshot);
				bSnapshotFirst = FALSE;
			}
		}
#endif

#ifdef __KERNEL__
#if NMR_VDOTRIG_DUMP_SNAPSHOT_KERNEL_VER
		if (vidBSinfo.SrcOutYAddr != 0) {
			int g_wo_fp = 0;
			int len = 0;
			UINT32 w = gNMRVdoEncObj[pathID].InitInfo.width;
			UINT32 h = gNMRVdoEncObj[pathID].InitInfo.height;

			char filename[260];
			snprintf(filename, sizeof(filename), "/mnt/sd/VDOENC_%d_%d.yuv", gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height);
			//DBG_DUMP("[VDOTRIG] SrcOut , Y = 0x%08x, UV = 0x%08x, W = %u, H= %u, Y_loff = %u, UV_loff = %u\r\n", vidBSinfo.SrcOutYAddr, vidBSinfo.SrcOutUVAddr, vidBSinfo.SrcOutWidth, vidBSinfo.SrcOutHeight, vidBSinfo.SrcOutYLoft, vidBSinfo.SrcOutUVLoft);

			// OPEN
			g_wo_fp = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);

			if ((VOS_FILE)(-1) != g_wo_fp) {
				// WRITE
				len = vos_file_write(g_wo_fp, (void *)vidBSinfo.SrcOutYAddr , (w*h));
				len = vos_file_write(g_wo_fp, (void *)vidBSinfo.SrcOutUVAddr, (w*h/2));

				// CLOSE
				vos_file_close(g_wo_fp);
				DBG_DUMP("[VDOTRIG] DUMP_SNAPSHOT success !! (%s)\r\n", filename);
			} else {
				DBG_ERR("[VDOTRIG] DUMP_SNAPSHOT_FRAME open file failed:%s\n", filename);
			}
		}
#endif
#endif

		// Update BSOutNow & Size
		if (pVidEncParam->re_trigger) {
			pVidTrigObj->BsNow = ALIGN_CEIL_64(pVidTrigObj->BsStart + len2); // 1st BStream Frame starts at H264 Description in AVI file.
		} else {
			pVidTrigObj->BsNow = ALIGN_CEIL_64(pVidTrigObj->BsNow + pVidEncParam->bs_size_1); // 1st BStream Frame starts at H264 Description in AVI file.
		}

		// nalu info, Jira : IVOT_N01004_CO-865
		if ((gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) || (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265))
		{
			static UINT32 sCountNaluInfo[NMR_VDOENC_MAX_PATH] = {0};
			UINT32 idx = 0;
			UINT32 bs_addr_pa = 0, bs_addr_pa2 = 0;
			UINT32 bs_addr_va = 0;
			UINT32 *nalu_len_addr = (UINT32*)pVidEncParam->nalu_size_addr;
			UINT32 nalu_num = pVidEncParam->nalu_num;
			UINT32 total_size = 0;

			// nalu_num check
			if (nalu_num > _nmr_vdotrig_nalu_num_max()) {
				if (sCountNaluInfo[pathID]++ % 300 == 0) {
					DBG_WRN("[VDOTRIG][%d] nalu_num(%d) exceed limit(%d), SKIP nalu info output !!\r\n", pathID, nalu_num, _nmr_vdotrig_nalu_num_max());
				}
				nalu_num = 0; // only "nalu_num = 0" later
			}

			// nalu_num
			if (pVidEncParam->re_trigger) {
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = nalu_num + 1;
				bs_addr_va = vidBSinfo.Addr;
			} else {
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = nalu_num;
			}
			total_size += sizeof(UINT32);

			// nalu addr/size
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				bs_addr_pa = (gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_GET_BS_START_PA, (UINT32) &vidBSinfo);
				if (pVidEncParam->re_trigger) {
					bs_addr_pa2 = (gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_GET_BS_START_PA2, (UINT32) &vidBSinfo);
				}
			}

			for (idx=0; idx<nalu_num; idx++) {
				if (pVidEncParam->re_trigger) {
					if ((IsIFrame) && (idx==0)) {
						if (bs_addr_va + nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize > vidBSinfo.Addr + vidBSinfo.Size) {
							// nalu_addr
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr_pa;
							total_size += sizeof(UINT32);
							// nalu_size
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (UINT32)(vidBSinfo.Addr + vidBSinfo.Size - bs_addr_va);
							total_size += sizeof(UINT32);

							// nalu_addr
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr_pa2;
							total_size += sizeof(UINT32);
							// nalu_size
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (UINT32)(nalu_len_addr[idx] - (vidBSinfo.Addr + vidBSinfo.Size - bs_addr_va) - gNMRVdoEncObj[pathID].uiDescSize);
							total_size += sizeof(UINT32);

							// update bs_addr_pa, bs_addr_va
							bs_addr_pa = bs_addr_pa2 + (nalu_len_addr[idx] - (vidBSinfo.Addr + vidBSinfo.Size - bs_addr_va) - gNMRVdoEncObj[pathID].uiDescSize);
							bs_addr_va = vidBSinfo.Addr2 + (nalu_len_addr[idx] - (vidBSinfo.Addr + vidBSinfo.Size - bs_addr_va) - gNMRVdoEncObj[pathID].uiDescSize);
						} else {
							// nalu_addr
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr_pa;
							total_size += sizeof(UINT32);
							// nalu_size
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (UINT32)(nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize);
							total_size += sizeof(UINT32);

							// update bs_addr_pa, bs_addr_va
							bs_addr_pa += nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize;
							bs_addr_va += nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize; // update bs_addr_va
						}
					} else {
						if (bs_addr_va + nalu_len_addr[idx] > vidBSinfo.Addr + vidBSinfo.Size) {
							// nalu_addr
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr_pa;
							total_size += sizeof(UINT32);
							// nalu_size
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (UINT32)(vidBSinfo.Addr + vidBSinfo.Size - bs_addr_va);
							total_size += sizeof(UINT32);

							// nalu_addr
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr_pa2;
							total_size += sizeof(UINT32);
							// nalu_size
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (UINT32)(nalu_len_addr[idx] - (vidBSinfo.Addr + vidBSinfo.Size - bs_addr_va));
							total_size += sizeof(UINT32);

							// update bs_addr_pa, bs_addr_va
							bs_addr_pa = bs_addr_pa2 + (nalu_len_addr[idx] - (vidBSinfo.Addr + vidBSinfo.Size - bs_addr_va));
							bs_addr_va = vidBSinfo.Addr2 + (nalu_len_addr[idx] - (vidBSinfo.Addr + vidBSinfo.Size - bs_addr_va));
						} else {
							// nalu_addr
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr_pa;
							total_size += sizeof(UINT32);
							// nalu_size
							*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (UINT32)(nalu_len_addr[idx]);
							total_size += sizeof(UINT32);

							// update bs_addr_pa, bs_addr_va
							bs_addr_pa += nalu_len_addr[idx];
							bs_addr_va += nalu_len_addr[idx]; // update bs_addr_va
						}
					}
				} else {
					// nalu_addr
					*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr_pa;
					total_size += sizeof(UINT32);

					// nalu_size
					if ((IsIFrame) && (idx==0)) {
						*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize);
						bs_addr_pa += (nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize); // update bs_addr
					}else{
						*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = nalu_len_addr[idx];
						bs_addr_pa += nalu_len_addr[idx]; // update bs_addr
					}
					total_size += sizeof(UINT32);
				}
			}

			total_size = ALIGN_CEIL_64(total_size);

			// flush cache
			#if 1  // don't need to flush, because we let nalu_addr be 64x , and size be 64x  => (Update) with BS_QUICK_ROLLBACK maybe wrong, so enable again
			vos_cpu_dcache_sync((UINT32)pVidTrigObj->BsNow, total_size, VOS_DMA_TO_DEVICE);
			#endif

			// update BsNow
			pVidTrigObj->BsNow += total_size;
		} else {
			UINT32 bs_addr = 0, bs_addr2 = 0;
			UINT32 total_size = 0;

			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				bs_addr = (gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_GET_BS_START_PA, (UINT32) &vidBSinfo);
				if (pVidEncParam->re_trigger) {
					bs_addr2 = (gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_GET_BS_START_PA2, (UINT32) &vidBSinfo);
				}
			}

			// nalu_num
			*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = 1;
			total_size += sizeof(UINT32);

			// nalu_addr
			*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr;
			total_size += sizeof(UINT32);

			// nalu_size
			*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = vidBSinfo.Size;
			total_size += sizeof(UINT32);

			if (pVidEncParam->re_trigger) {
				// nalu_addr2
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = bs_addr2;
				total_size += sizeof(UINT32);

				// nalu_size2
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = vidBSinfo.Size2;
				total_size += sizeof(UINT32);
			}

			total_size = ALIGN_CEIL_64(total_size);

			// flush cache
			#if 1  // don't need to flush, because we let nalu_addr be 64x , and size be 64x  => (Update) with BS_QUICK_ROLLBACK maybe wrong, so enable again
			vos_cpu_dcache_sync((UINT32)pVidTrigObj->BsNow, total_size, VOS_DMA_TO_DEVICE);
			#endif

			// update BsNow
			pVidTrigObj->BsNow += total_size;
		}

		// meta info, Jira : NA51089-793
		if (gNMRVdoEncObj[pathID].pMetaAllocInfo) {
			NMI_VDOENC_META_ALLOC_INFO *p_user_meta_data = (NMI_VDOENC_META_ALLOC_INFO *)&gNMRVdoEncObj[pathID].pMetaAllocInfo[0];
			UINT32 header_size = 0, buffer_size = 0;
			UINT32 total_size = 0;

			while (p_user_meta_data != NULL) {
				header_size = p_user_meta_data->header_size;
				buffer_size = p_user_meta_data->data_size;

				// meta signature
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = p_user_meta_data->sign;
				total_size += sizeof(UINT32);

				// meta p_next => pa
				if (p_user_meta_data->p_next) {
					*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = (nvtmpp_sys_va2pa(pVidTrigObj->BsNow) + total_size - sizeof(UINT32) + header_size + buffer_size);
				} else {
					*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = 0;
				}
				total_size += sizeof(UINT32);

				// meta header_size
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = header_size;
				total_size += sizeof(UINT32);

				// meta buffer_size
				*((UINT32 *)(pVidTrigObj->BsNow + total_size)) = buffer_size;
				total_size += sizeof(UINT32);

				//meta header
				//memset((void *)(pVidTrigObj->BsNow + total_size), 0x00, (header_size - sizeof(UINT32)*4));
				total_size += (header_size - sizeof(UINT32)*4);

				//meta buffer
				//memset((void *)(pVidTrigObj->BsNow + total_size), 0x00, buffer_size);
				total_size += buffer_size;

				//next pointer
				p_user_meta_data = p_user_meta_data->p_next;
			}

			// flush cache
	#if 1  // don't need to flush, because we let nalu_addr be 64x , and size be 64x  => (Update) with BS_QUICK_ROLLBACK maybe wrong, so enable again
			vos_cpu_dcache_sync((UINT32)pVidTrigObj->BsNow, total_size, VOS_DMA_TO_DEVICE);
	#endif

			// update BsNow
			pVidTrigObj->BsNow += total_size;
		}

		// callback isf_vdoenc to put PullQueue
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			NMR_VdoTrig_LockBS(pathID, vidBSinfo.Addr, vidBSinfo.Size);
			if (gNMRVdoEncObj[pathID].MemInfo.bBSRingMode == TRUE && pVidEncParam->re_trigger) {
				NMR_VdoTrig_LockBS(pathID, vidBSinfo.Addr2, vidBSinfo.Size2);
			}
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo); // the do_probe REL will do on StreamReadyCB, with (Occupy = 0 & Addr !=0)
		}
		pVidTrigObj->uiEncStatus = NMR_VDOENC_ENC_STATUS_BS_CB_DONE;

		// Reserve size for next bs
		if (gNMRVdoEncObj[pathID].uiRecFormat != MEDIAREC_LIVEVIEW && gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_TS) {
			pVidTrigObj->BsNow += ALIGN_CEIL_4(NMEDIAREC_TS_PES_HEADER_LENGTH + NMEDIAREC_TS_NAL_LENGTH);
		}

		if (gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_JPG) {
			pVidTrigObj->BsNow += ALIGN_CEIL_4(NMEDIAREC_JPG_EXIF_HEADER_LENGTH);
		}

		if (gNMRVdoEncObj[pathID].uiBsReservedSize > 0) {
			pVidTrigObj->BsNow += ALIGN_CEIL_4(gNMRVdoEncObj[pathID].uiBsReservedSize);
		}

		if (pVidTrigObj->BsNow >= pVidTrigObj->BsMax && pVidTrigObj->WantRollback == FALSE) {
			if (gNMRVdoTrigMsg) {
				DBG_DUMP("Rollback, now=%x, max=%x\r\n", pVidTrigObj->BsNow, pVidTrigObj->BsMax);
			}
			pVidTrigObj->WantRollback = TRUE;
		}

		pVidTrigObj->FrameCount++;
		if (gNMRVdoEncObj[pathID].bSkipFrm) {
			pVidTrigObj->SkipModeFrameIdx ++;
		}
		gNMRVdoEncObj[pathID].uiEncOutCount++;
		if (pVidEncParam->re_encode_en) {
			gNMRVdoEncObj[pathID].uiEncReCount++;
		}

		if (pVidTrigObj->WantRollback) {
			pVidTrigObj->BsNow = pVidTrigObj->BsStart;
			pVidTrigObj->WantRollback = FALSE;
		}

		// Sync Bitstream data address
		if (IsIDR2Cut && pVidTrigObj->FrameCount != 0) {
			gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_RESET;
			if (pathID == gNMRVdoEncFirstPathID) {
				sec = pVidTrigObj->FrameCount / pVidTrigObj->SyncFrameN;
				if (gNMRVdoEncObj[pathID].CallBackFunc) {
					(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_SEC_CB, sec);
				}
			}
		}//if match sync number
	}//if encode ok
	else {
		pVidTrigObj->uiEncStatus = NMR_VDOENC_ENC_STATUS_ENCODE_FAIL_DONE;

		DBG_IND("[VDOTRIG][%d] Encode Error, Y Addr = 0x%08x, UV Addr = 0x%08x, YLoft = %d, UVLoft = %d\r\n", pathID, p_YuvSrc->uiYAddr, p_YuvSrc->uiCbAddr, p_YuvSrc->uiYLineOffset, p_YuvSrc->uiUVLineOffset);
		vidBSinfo.PathID = pathID;
		vidBSinfo.BufID = p_YuvSrc->uiBufID;
		vidBSinfo.Occupied = (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER && uiQueueCount == 1) ? 1 : 0;
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID),  ISF_IN_PROBE_PROC_ERR,  ISF_ERR_PROCESS_FAIL); // [IN]  PROC_ERR
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID), ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PROC_ERR
		}

		pVidTrigObj->BsNow = pVidTrigObj->BsStart;
		gNMRVdoEncObj[pathID].uiEncErrCount++;

		// JIRA:IVOT_N01002_CO-565.  Encode failed, rec buffer had been half-overwritten by this failed frame..... Have to do reset-I for next frame
		gNMRVdoEncObj[pathID].bResetIFrame = TRUE;

		// set srcout for next frame ( because this frame encode fail, try again next frame )
		if (pVidEncParam->src_out_en == 1) {
			gNMRVdoEncObj[pathID].bSnapshot = 1;
		}
	}

TRIG_AND_WAIT_EXIT:
#if 1
	// unlock OSD & MASK
	osdtime = Perf_GetCurrent();
	_isf_vdoenc_finish_input_mask(&isf_vdoenc, ISF_IN(pathID));
	_isf_vdoenc_finish_input_osd(&isf_vdoenc, ISF_IN(pathID));
	top_time[NMI_VDOENC_TOP_OSD_FINISH_TIME] = Perf_GetCurrent() - osdtime;
	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OSD) {
		DBG_DUMP("[VDOTRIG][%d] finish_osd %d us\r\n", pathID, Perf_GetCurrent() - top_time[NMI_VDOENC_TOP_OSD_FINISH_TIME]);
	}
#endif

	_NMR_VdoTrig_TopTime(pathID, top_time, p_YuvSrc->uiRawCount);

	return;
}
#endif

static void _NMR_VdoTrig_Trig(UINT32 pathID, UINT32 action)
{
	gNMRVdoTrigObj[pathID].uiEncStatus = NMR_VDOENC_ENC_STATUS_CHECK_VALID;

	if (gNMRVdoTrigObj[pathID].Encoder.SetInfo == NULL) {
		DBG_ERR("[VDOTRIG][%d] codec type = %d not support\r\n", pathID, gNMRVdoEncObj[pathID].uiVidCodec);
		return;
	}

	if (gNMRVdoEncObj[pathID].bStart == FALSE) {
		return;
	}

	if (NMR_VdoTrig_GetYuvCount(pathID) > 0) {
		if (gNMRVdoEncObj[pathID].bResetIFrame) {
			MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);
			gNMRVdoEncObj[pathID].bResetIFrame = FALSE;
			DBG_IND("[VDOTRIG][%d] Reset I frame, time = %u us\r\n", pathID, Perf_GetCurrent());
			if ((pVidEnc) && (pVidEnc->SetInfo)) {
				(pVidEnc->SetInfo)(VDOENC_SET_RESET_IFRAME, 0, 0, 0);
			}
			if (gNMRVdoTrigObj[pathID].SyncFrameN != 0 && gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN != 0) {
				gNMRVdoTrigObj[pathID].FrameCount += gNMRVdoTrigObj[pathID].SyncFrameN - (gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN);
				if (gNMRVdoEncObj[pathID].bSkipFrm) {
					gNMRVdoTrigObj[pathID].SkipModeFrameIdx += gNMRVdoTrigObj[pathID].SyncFrameN - (gNMRVdoTrigObj[pathID].SkipModeFrameIdx % gNMRVdoTrigObj[pathID].SyncFrameN);
				}
			}
			gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type = NMR_VDOENC_IDR_SLICE;
		}

		if (gNMRVdoEncObj[pathID].uiSetRCMode != 0) {
			MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);
			if (gNMRVdoEncObj[pathID].uiSetRCMode == NMR_VDOENC_RC_CBR) {
				if ((pVidEnc) && (pVidEnc->SetInfo)) {
					(pVidEnc->SetInfo)(VDOENC_SET_CBR, (UINT32)&gNMRVdoEncObj[pathID].CbrInfo, 0, 0);
				}
			} else if (gNMRVdoEncObj[pathID].uiSetRCMode == NMR_VDOENC_RC_VBR) {
				if ((pVidEnc) && (pVidEnc->SetInfo)) {
					(pVidEnc->SetInfo)(VDOENC_SET_VBR, (UINT32)&gNMRVdoEncObj[pathID].VbrInfo, 0, 0);
				}
			} else if (gNMRVdoEncObj[pathID].uiSetRCMode == NMR_VDOENC_RC_EVBR) {
				if ((pVidEnc) && (pVidEnc->SetInfo)) {
					(pVidEnc->SetInfo)(VDOENC_SET_EVBR, (UINT32)&gNMRVdoEncObj[pathID].EVbrInfo, 0, 0);
				}
			}

			gNMRVdoEncObj[pathID].uiSetRCMode = 0;
		}

		_NMR_VdoTrig_TrigAndWait(pathID, action);

		if (gNMRVdoEncObj[pathID].bStart && gNMRVdoEncObj[pathID].bResetEncoder) {
			MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);
			gNMRVdoEncObj[pathID].bResetEncoder = FALSE;
			DBG_DUMP("[VDOTRIG][%d] Reset Encoder\r\n", pathID);
			if ((pVidEnc) && (pVidEnc->Close)) {
				(pVidEnc->Close)();
			}
			NMR_VdoEnc_SetEncodeParam(pathID);
			NMR_VdoEnc_SetDesc(pathID);
			if (gNMRVdoTrigObj[pathID].SyncFrameN != 0 && gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN != 0) {
				gNMRVdoTrigObj[pathID].FrameCount += gNMRVdoTrigObj[pathID].SyncFrameN - (gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN);
				if (gNMRVdoEncObj[pathID].bSkipFrm) {
					gNMRVdoTrigObj[pathID].SkipModeFrameIdx += gNMRVdoTrigObj[pathID].SyncFrameN - (gNMRVdoTrigObj[pathID].SkipModeFrameIdx % gNMRVdoTrigObj[pathID].SyncFrameN);
				}
			}
		}

		if (gNMRVdoEncObj[pathID].bResetSec) {
			gNMRVdoEncObj[pathID].bResetSec = FALSE;
			if (gNMRVdoTrigObj[pathID].SyncFrameN != 0) {
				gNMRVdoTrigObj[pathID].FrameCount = gNMRVdoTrigObj[pathID].FrameCount % gNMRVdoTrigObj[pathID].SyncFrameN;
				if (gNMRVdoEncObj[pathID].bSkipFrm) {
					gNMRVdoTrigObj[pathID].SkipModeFrameIdx = gNMRVdoTrigObj[pathID].SkipModeFrameIdx % gNMRVdoTrigObj[pathID].SyncFrameN;
				}
			}
		}
	} else {
		static UINT32 sCount;
		if (sCount % 30 == 0) {
			DBG_DUMP("[VDOTRIG][%d] vid queue is empty\r\n", pathID);
		}
		sCount++;
	}
}

static void _NMR_VdoTrig_Tsk_H26X(void)
{
	FLGPTN uiFlag = 0;
	UINT32 pathID = 0;

	THREAD_ENTRY();

	clr_flg(FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_IDLE);
		gNMRVdoTrigObj[pathID].uiEncStatus = NMR_VDOENC_ENC_STATUS_IDLE;

		PROFILE_TASK_IDLE();
		//DBG_DUMP("[VDOTRIG] VdoTrigTsk wait flag before\r\n");
		wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_ENCODE | FLG_NMR_VDOENC_PRELOAD | FLG_NMR_VDOENC_STOP, TWF_ORW | TWF_CLR);
		//DBG_DUMP("[VDOTRIG] VdoTrigTsk wait flag after\r\n");
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_IDLE);

		if (uiFlag & FLG_NMR_VDOENC_STOP) {
			break;
		}

		if (uiFlag & FLG_NMR_VDOENC_ENCODE) {
			if (NMR_VdoTrig_GetJobCount_H26X() > 0) {
				NMR_VdoTrig_GetJob_H26X(&pathID);
				_NMR_VdoTrig_Trig(pathID, NMR_VDOTRIG_ACT_NORMAL);
			}
		}//if enc

		if (NMR_VdoTrig_GetJobCount_H26X() > 0) { //continue to trig job
			set_flg(FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_ENCODE);
		}
	}   //  end of while loop
}

THREAD_DECLARE(NMR_VdoTrig_D2DTsk_H26X, p1)
{
	_NMR_VdoTrig_Tsk_H26X();
	set_flg(FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_STOP_DONE);

    THREAD_RETURN(0);
}

static void _NMR_VdoTrig_Tsk_JPEG(void)
{
	FLGPTN uiFlag = 0;
	UINT32 pathID = 0;

	THREAD_ENTRY();

	clr_flg(FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_IDLE);
		gNMRVdoTrigObj[pathID].uiEncStatus = NMR_VDOENC_ENC_STATUS_IDLE;

		PROFILE_TASK_IDLE();
		//DBG_DUMP("[VDOTRIG] VdoTrigTsk wait flag before\r\n");
		wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_ENCODE | FLG_NMR_VDOENC_PRELOAD | FLG_NMR_VDOENC_STOP, TWF_ORW | TWF_CLR);
		//DBG_DUMP("[VDOTRIG] VdoTrigTsk wait flag after\r\n");
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_IDLE);

		if (uiFlag & FLG_NMR_VDOENC_STOP) {
			break;
		}

		if (uiFlag & FLG_NMR_VDOENC_ENCODE) {
			if (NMR_VdoTrig_GetJobCount_JPEG() > 0) {
				NMR_VdoTrig_GetJob_JPEG(&pathID);
				_NMR_VdoTrig_Trig(pathID, NMR_VDOTRIG_ACT_NORMAL);
			}
		}//if enc

		if (NMR_VdoTrig_GetJobCount_JPEG() > 0) { //continue to trig job
			set_flg(FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_ENCODE);
		}
	}   //  end of while loop
}

THREAD_DECLARE(NMR_VdoTrig_D2DTsk_JPEG, p1)
{
	_NMR_VdoTrig_Tsk_JPEG();
	set_flg(FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_STOP_DONE);

    THREAD_RETURN(0);
}

static void _NMR_VdoTrig_Snapshot_TrigAndWait(UINT32 pathID)
{
	MP_VDOENC_YUV_SRCOUT *p_srcout = NULL;
	UINT32 enctime=0;

	if (FALSE == NMR_VdoTrig_GetSrcOut(pathID, &p_srcout)) {
		DBG_ERR("[VDOTRIG][%d] get srcout YUV fail !!\r\n", pathID);
		return;
	}

	if (gNMRVdoEncObj[pathID].bCommSrcOutNoJPGEnc) {
		// callback isf_vdoenc
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_SNAPSHOT_CB, (UINT32) p_srcout);
		}
	} else {
		PMP_VDOENC_ENCODER p_video_obj = MP_MjpgEnc_getVideoObject(KDRV_VDOENC_ID_MAX); // get MP_JPEG, pseudo max_path+1 engine
		MP_VDOENC_INIT mp_init = {0};
		MP_VDOENC_PARAM mp_param = {0};

		// Initialize
		{
			mp_init.byte_rate       = 0;
			mp_init.frame_rate      = 0;
			mp_init.jpeg_yuv_format = 1; //yuv420
			mp_init.width           = p_srcout->width;
			mp_init.height          = p_srcout->height;
			(p_video_obj->Initialize)(&mp_init);
		}
		// Encode
		{
			mp_param.quality        = p_srcout->quality;

			mp_param.y_addr         = p_srcout->y_addr;
			mp_param.c_addr         = p_srcout->c_addr;
			mp_param.y_line_offset  = p_srcout->y_line_offset;
			mp_param.c_line_offset  = p_srcout->c_line_offset;

			mp_param.bs_start_addr  = p_srcout->snap_addr;
			mp_param.bs_addr_1      = p_srcout->snap_addr;
			mp_param.bs_size_1      = p_srcout->snap_size;
			mp_param.bs_end_addr    = p_srcout->snap_addr + p_srcout->snap_size;

			VdoCodec_JPEG_Lock();

			if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_SRCOUT) {
				DBG_DUMP("[VDOTRIG][%d] before jpg encode, w/h = %u/%u, Y/UV addr = %08x/%08x, Y/UV loff = %u/%u, (%08x, %08x, %u, %08x), timestamp = %llu\r\n", pathID, mp_init.width, mp_init.height, mp_param.y_addr, mp_param.c_addr, mp_param.y_line_offset, mp_param.c_line_offset, mp_param.bs_start_addr, mp_param.bs_addr_1, mp_param.bs_size_1, mp_param.bs_end_addr, p_srcout->timestamp);
				enctime = Perf_GetCurrent();
			}

#ifdef VDOENC_LL
			if (E_OK == (p_video_obj->EncodeOne)(0, &mp_param, 0, 0)) {
#else
			if (E_OK == (p_video_obj->EncodeOne)(0, 0, 0, &mp_param)) {
#endif
				p_srcout->snap_size = mp_param.bs_size_1;

				if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_SRCOUT) {
					enctime = Perf_GetCurrent()- enctime;
					DBG_DUMP("[VDOTRIG][%d] Enc time(total) = %d us, Size = %d, SrcoutTimeStamp = %lld\r\n", pathID, enctime, p_srcout->snap_size, p_srcout->timestamp);
				}
			} else {
				p_srcout->snap_size = 0;
				p_srcout->timestamp = 0;
				DBG_ERR("[VDOTRIG][%d] snapshot encode fail...provided buffer is not enough\r\n", pathID);
			}
			VdoCodec_JPEG_UnLock();

			DBG_IND("[VDOTRIG][%d] after jpg encode\r\n", pathID);

			// callback isf_vdoenc
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_SNAPSHOT_CB, (UINT32) p_srcout);
			}
		}
		// Close
		{
			(p_video_obj->Close)();
		}
	}
}

static void _NMR_VdoTrig_Tsk_H26X_Snapshot(void)
{
	FLGPTN uiFlag = 0;
	UINT32 pathID = 0;

	THREAD_ENTRY();

	clr_flg(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_IDLE);

		PROFILE_TASK_IDLE();
		//DBG_DUMP("[VDOTRIG] VdoTrigTsk H26x_Snapshot wait flag before\r\n");
		wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_ENCODE | FLG_NMR_VDOENC_STOP, TWF_ORW | TWF_CLR);
		//DBG_DUMP("[VDOTRIG] VdoTrigTsk H26x_Snapshot wait flag after\r\n");
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_IDLE);

		if (uiFlag & FLG_NMR_VDOENC_STOP) {
			break;
		}

		if (uiFlag & FLG_NMR_VDOENC_ENCODE) {
			if (NMR_VdoTrig_GetJobCount_H26X_Snapshot() > 0) {
				NMR_VdoTrig_GetJob_H26X_Snapshot(&pathID);
				_NMR_VdoTrig_Snapshot_TrigAndWait(pathID);
			}
		}//if enc

		if (NMR_VdoTrig_GetJobCount_H26X_Snapshot() > 0) { //continue to trig job
			set_flg(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_ENCODE);
		}
	}   //  end of while loop
}

THREAD_DECLARE(NMR_VdoTrig_D2DTsk_H26X_Snapshot, p1)
{
	_NMR_VdoTrig_Tsk_H26X_Snapshot();
	set_flg(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_STOP_DONE);

    THREAD_RETURN(0);
}

#ifdef VDOENC_LL
static void _NMR_VdoTrig_Tsk_WrapBS(void)
{
	FLGPTN uiFlag;
	UINT32 pathID = 0;

	THREAD_ENTRY();

	clr_flg(FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_IDLE);

		PROFILE_TASK_IDLE();
		//DBG_DUMP("[VDOTRIG] VdoTrigTsk wrap bs wait flag before\r\n");
		wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_ENCODE | FLG_NMR_VDOENC_STOP, TWF_ORW | TWF_CLR);
		//DBG_DUMP("[VDOTRIG] VdoTrigTsk wrap bs wait flag after\r\n");
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_IDLE);

		if (uiFlag & FLG_NMR_VDOENC_STOP) {
			break;
		}

		if (uiFlag & FLG_NMR_VDOENC_ENCODE) {
			if (NMR_VdoTrig_GetJobCount_WrapBS() > 0) {
				NMR_VdoTrig_GetJob_WrapBS(&pathID);
				_NMR_VdoTrig_TrigAndWrapBSInfo(pathID);
			}
		}//if enc

		if (NMR_VdoTrig_GetJobCount_WrapBS() > 0) { //continue to trig job
			set_flg(FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_ENCODE);
		}
	}   //  end of while loop
}

THREAD_DECLARE(NMR_VdoTrig_D2DTsk_WrapBS, p1)
{
	_NMR_VdoTrig_Tsk_WrapBS();
	set_flg(FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_STOP_DONE);

    THREAD_RETURN(0);
}

#endif

void NMR_VdoTrig_WaitTskIdle(UINT32 pathID)
{
	FLGPTN uiFlag;

	// wait for video task idle
	if (gNMRVdoEncObj[pathID].uiVidCodec == MEDIAVIDENC_MJPG) {
		wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_IDLE, TWF_ORW);
	} else {
		wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_IDLE, TWF_ORW);
		wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_IDLE, TWF_ORW);
	}
#if MP_VDOENC_SHOW_MSG
#if 0
	DBG_DUMP("[VDOTRIG][%u] wait idle finished\r\n", pathID);
#else
	(&isf_vdoenc)->p_base->do_trace(&isf_vdoenc, ISF_OUT(pathID), ISF_OP_STATE, "[VDOTRIG][%u] wait idle finished\r\n", pathID);
#endif
#endif
}


ER NMR_VdoTrig_TrigEncode(UINT32 pathID)
{
	if (gNMRVdoEncObj[pathID].uiVidCodec == MEDIAVIDENC_MJPG) {
		set_flg(FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_ENCODE);
	} else {
		set_flg(FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_ENCODE);
	}

	return E_OK;
}

ER NMR_VdoTrig_TrigSnapshotEncode(void)
{
	set_flg(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_ENCODE);
	return E_OK;
}

#ifdef VDOENC_LL
ER NMR_VdoTrig_TrigWrapBS(void)
{
	set_flg(FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_ENCODE);
	return E_OK;
}
#endif

ER NMR_VdoTrig_Start_H26X(void)
{
	if (gNMRVidTrigOpened_H26X) {
		DBG_WRN("H26X task already started...\r\n");
		return E_OK;
	}
	// Start Task (h26x main)
	THREAD_CREATE(NMR_VDOTRIG_D2DTSK_ID_H26X, NMR_VdoTrig_D2DTsk_H26X, NULL, "NMR_VdoTrig_D2DTsk_H26X");
	if (NMR_VDOTRIG_D2DTSK_ID_H26X == NULL) {
		DBG_ERR("NMR_VDOTRIG_D2DTSK_ID_H26X = NULL\r\n");
		return E_OACV;
	}
	THREAD_SET_PRIORITY(NMR_VDOTRIG_D2DTSK_ID_H26X, NMR_VDOTRIG_D2DTSK_PRI);
	THREAD_RESUME(NMR_VDOTRIG_D2DTSK_ID_H26X);       //sta_tsk(NMR_VDOTRIG_D2DTSK_ID, 0);

	// Start Task (h26x source_out snapshot)
	THREAD_CREATE(NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT, NMR_VdoTrig_D2DTsk_H26X_Snapshot, NULL, "NMR_VdoTrig_D2DTsk_H26X_Snapshot");
	if (NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT == NULL) {
		DBG_ERR("NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT = NULL\r\n");
		return E_OACV;
	}
	THREAD_SET_PRIORITY(NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT, NMR_VDOTRIG_D2DTSK_PRI);
	THREAD_RESUME(NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT);       //sta_tsk(NMR_VDOTRIG_D2DTSK_ID, 0);

	gNMRVidTrigOpened_H26X = TRUE;
	return E_OK;
}

ER NMR_VdoTrig_Start_JPEG(void)
{
	if (gNMRVidTrigOpened_JPEG) {
		DBG_WRN("JPEG task already started...\r\n");
		return E_OK;
	}
	// Start Task
	THREAD_CREATE(NMR_VDOTRIG_D2DTSK_ID_JPEG, NMR_VdoTrig_D2DTsk_JPEG, NULL, "NMR_VdoTrig_D2DTsk_JPEG");
	if (NMR_VDOTRIG_D2DTSK_ID_JPEG == NULL) {
		DBG_ERR("NMR_VDOTRIG_D2DTSK_ID_JPEG = NULL\r\n");
		return E_OACV;
	}
	THREAD_SET_PRIORITY(NMR_VDOTRIG_D2DTSK_ID_JPEG, NMR_VDOTRIG_D2DTSK_PRI);
	THREAD_RESUME(NMR_VDOTRIG_D2DTSK_ID_JPEG);       //sta_tsk(NMR_VDOTRIG_D2DTSK_ID, 0);

	gNMRVidTrigOpened_JPEG = TRUE;
	return E_OK;
}

#ifdef VDOENC_LL
ER NMR_VdoTrig_Start_WrapBS(void)
{
	if (gNMRVidTrigOpened_WrapBS) {
		DBG_WRN("H26X task already started...\r\n");
		return E_OK;
	}

	// Start Task (wrap bs)
	THREAD_CREATE(NMR_VDOTRIG_D2DTSK_ID_WRAPBS, NMR_VdoTrig_D2DTsk_WrapBS, NULL, "NMR_VdoTrig_D2DTsk_WrapBS");
	if (NMR_VDOTRIG_D2DTSK_ID_WRAPBS== NULL) {
		DBG_ERR("NMR_VDOTRIG_D2DTSK_ID_WRAPBS = NULL\r\n");
		return E_OACV;
	}
	THREAD_SET_PRIORITY(NMR_VDOTRIG_D2DTSK_ID_WRAPBS, NMR_VDOTRIG_D2DTSK_PRI);
	THREAD_RESUME(NMR_VDOTRIG_D2DTSK_ID_WRAPBS);       //sta_tsk(NMR_VDOTRIG_D2DTSK_ID, 0);

	gNMRVidTrigOpened_WrapBS = TRUE;
	return E_OK;
}
#endif

ER NMR_VdoTrig_Close_H26X(void)
{
	FLGPTN uiFlag;
	UINT32 i;

	if (gNMRVidTrigOpened_H26X == FALSE) {
		DBG_WRN("H26X task already stoped...\r\n");
		return E_OK;
	}

	// wait for video task idle
	wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_IDLE, TWF_ORW);
	wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_IDLE, TWF_ORW);

//#ifdef __KERNEL__
#if 1
	set_flg(FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_STOP);
	wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_STOP_DONE, TWF_ORW | TWF_CLR);

	set_flg(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_STOP);
	wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT, FLG_NMR_VDOENC_STOP_DONE, TWF_ORW | TWF_CLR);
#endif
	//THREAD_DESTROY(NMR_VDOTRIG_D2DTSK_ID_H26X);  //ter_tsk(NMR_VDOTRIG_D2DTSK_ID_H26X);
	//THREAD_DESTROY(NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT);  //ter_tsk(NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT);

#if MP_VDOENC_SHOW_MSG
	DBG_IND("[VDOTRIG] wait h26x close finished\r\n");
#endif
	for (i = 0; i < PATH_MAX_COUNT; i++) { //2015/05/11
		gNMRVdoTrigFakeIPLStop[i] = 0;
	}



	gNMRVidTrigOpened_H26X = FALSE;
	return E_OK;
}

ER NMR_VdoTrig_Close_JPEG(void)
{
	FLGPTN uiFlag;
	UINT32 i;

	if (gNMRVidTrigOpened_JPEG == FALSE) {
		DBG_WRN("JPEG task already stoped...\r\n");
		return E_OK;
	}

	// wait for video task idle
	wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_IDLE, TWF_ORW);

//#ifdef __KERNEL__
#if 1
	set_flg(FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_STOP);
	wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_JPEG, FLG_NMR_VDOENC_STOP_DONE, TWF_ORW | TWF_CLR);
#endif
	//THREAD_DESTROY(NMR_VDOTRIG_D2DTSK_ID_JPEG);  //ter_tsk(NMR_VDOTRIG_D2DTSK_ID_JPEG);

#if MP_VDOENC_SHOW_MSG
	DBG_IND("[VDOTRIG] wait jpeg close finished\r\n");
#endif
	for (i = 0; i < PATH_MAX_COUNT; i++) { //2015/05/11
		gNMRVdoTrigFakeIPLStop[i] = 0;
	}



	gNMRVidTrigOpened_JPEG = FALSE;
	return E_OK;
}

#ifdef VDOENC_LL
ER NMR_VdoTrig_Close_WrapBS(void)
{
	FLGPTN uiFlag;
	UINT32 i;

	if (gNMRVidTrigOpened_WrapBS== FALSE) {
		DBG_WRN("Wrap BS task already stoped...\r\n");
		return E_OK;
	}

	// wait for wrap bs task idle
	wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_IDLE, TWF_ORW);

//#ifdef __KERNEL__
#if 1
	set_flg(FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_STOP);
	wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_WRAPBS, FLG_NMR_VDOENC_STOP_DONE, TWF_ORW | TWF_CLR);
#endif
	//THREAD_DESTROY(NMR_VDOTRIG_D2DTSK_ID_H26X);  //ter_tsk(NMR_VDOTRIG_D2DTSK_ID_H26X);
	//THREAD_DESTROY(NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT);  //ter_tsk(NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT);

#if MP_VDOENC_SHOW_MSG
	DBG_IND("[VDOTRIG] wait Wrap BS close finished\r\n");
#endif
	for (i = 0; i < PATH_MAX_COUNT; i++) { //2015/05/11
		gNMRVdoTrigFakeIPLStop[i] = 0;
	}

	gNMRVidTrigOpened_WrapBS= FALSE;
	return E_OK;
}
#endif

ER NMR_VdoTrig_CreatePluginObject(UINT32 pathID)
{
	memset(&(gNMRVdoTrigObj[pathID].Encoder), 0, sizeof(MP_VDOENC_ENCODER));

	if (gNMRVdoEncObj[pathID].pEncoder == 0) { //coverity 49805
		DBG_ERR("[VDOTRIG][%d] pObj Zero!\r\n", pathID);
		return E_SYS;
	}

	memcpy((void *) &(gNMRVdoTrigObj[pathID].Encoder), (void *)gNMRVdoEncObj[pathID].pEncoder, sizeof(MP_VDOENC_ENCODER));
	return E_OK;
}

ER NMR_VdoTrig_InitParam(UINT32 pathID)
{
	NMR_VDOTRIG_OBJ *pVidtribobj;
	PNMR_VDOTRIG_BSQ pObj;
	if (pathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOTRIG][%d] Path ID is invalid\r\n", pathID);
		return E_ID;
	}

	pVidtribobj = &gNMRVdoTrigObj[pathID];
	pObj = &(gNMRVdoTrigObj[pathID].bsQueue);
	if (gNMRVdoEncObj[pathID].uiRecFormat != MEDIAREC_LIVEVIEW && gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_TS)	{
		pVidtribobj->BsStart = gNMRVdoEncObj[pathID].MemInfo.uiBSStart + ALIGN_CEIL_4(NMEDIAREC_TS_PES_HEADER_LENGTH + NMEDIAREC_TS_NAL_LENGTH);
	} else if (gNMRVdoEncObj[pathID].uiFileType == NMEDIAREC_JPG) {
		pVidtribobj->BsStart = gNMRVdoEncObj[pathID].MemInfo.uiBSStart + ALIGN_CEIL_4(NMEDIAREC_JPG_EXIF_HEADER_LENGTH);
	} else {
		pVidtribobj->BsStart = gNMRVdoEncObj[pathID].MemInfo.uiBSStart + ALIGN_CEIL_4(gNMRVdoEncObj[pathID].uiBsReservedSize);
	}
	pVidtribobj->BsEnd   = gNMRVdoEncObj[pathID].MemInfo.uiBSEnd;
	pVidtribobj->BsMax   = gNMRVdoEncObj[pathID].MemInfo.uiBSMax;
	pVidtribobj->BsNow   = pVidtribobj->BsStart;
	pVidtribobj->FrameCount = 0;
	pVidtribobj->SyncFrameN = gNMRVdoEncObj[pathID].InitInfo.frame_rate/1000;
	pVidtribobj->WantRollback = FALSE;
	pVidtribobj->SkipModeFrameIdx = 0;
#ifdef VDOENC_LL
	pVidtribobj->Enc_CallBack.callback = NMR_VdoTrig_WrapBS;
	gNMRVidTrigStop[pathID] = 0;
#endif

	_NMR_VdoTrig_InitQ(pathID);
	gNMRVdoEncObj[pathID].uiMaxFrameQueue = NMR_VDOTRIG_BSQ_MAX;
	pObj->Queue = (NMI_VDOENC_MEM_RANGE *)gNMRVdoEncObj[pathID].uiBSQAddr;

	_NMR_VdoTrig_InitTop(pathID);

	return E_OK;
}

BOOL NMR_VdoTrig_PutYuv(UINT32 pathID, MP_VDOENC_YUV_SRC *pYuvSrc)
{
    PNMR_VDOTRIG_YUVQ pObj;
    MP_VDOENC_YUV_SRC *pQueueSrc;
	static UINT32 sCount;
	unsigned long flags = 0;

	if (!pYuvSrc) {
		DBG_ERR("[VDOTRIG][%d] Put Yuv Src is NULL\r\n", pathID);
		return FALSE;
	}

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_PROCESS) {
		pYuvSrc->uiProcessTime = Perf_GetCurrent();
	}

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_INPUT) {
		char *p = (char *) pYuvSrc->uiYAddr;
		DBG_DUMP("[VDOTRIG][%d] Input Width = %d, Height = %d, YAddr = 0x%08x, UVAddr = 0x%08x, Ylot = %d, UVlot = %d, SrcCompress = %d, Y Data = 0x%02x%02x%02x%02x%02x%02x%02x%02x, time = %u us\r\n",
			pathID, pYuvSrc->uiWidth, pYuvSrc->uiHeight, pYuvSrc->uiYAddr, pYuvSrc->uiCbAddr, pYuvSrc->uiYLineOffset, pYuvSrc->uiUVLineOffset, pYuvSrc->uiSrcCompression, *p, *(p+1), *(p+2), *(p+3), *(p+4), *(p+5), *(p+6), *(p+7), Perf_GetCurrent());
	}

	if (gNMRVdoEncObj[pathID].uiSkipFrame > 0) {
		gNMRVdoEncObj[pathID].uiSkipFrame--;
		return FALSE;
	}

	if (gNMRVdoEncObj[pathID].bSkipLoffCheck == FALSE) {
		if (pYuvSrc->uiWidth != gNMRVdoEncObj[pathID].InitInfo.width) {
			sCount++;
			if (sCount % 128 == 0) {
				DBG_ERR("[VDOTRIG][%d] YUV width(%d) must be equal to venc width(%d)\r\n", pathID, pYuvSrc->uiYLineOffset, gNMRVdoEncObj[pathID].InitInfo.width);
			}
			return FALSE;
		}
	}

	NMR_VdoEnc_Lock_cpu2(&flags);
	if (gNMRVdoTrigObj[pathID].BsStart == 0 || gNMRVdoEncObj[pathID].bStart == FALSE) {
		sCount++;
		if (sCount % 128 == 0) {
			DBG_WRN("[VDOTRIG][%d] Enc Bs Addr = 0x%08x, start = %d\r\n", pathID, gNMRVdoTrigObj[pathID].BsStart, gNMRVdoEncObj[pathID].bStart);
		}
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return FALSE;
	}
	pObj = &(gNMRVdoTrigObj[pathID].rawQueue);
	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		DBG_ERR("[VDOTRIG][%d] YUV Queue is Full!\r\n", pathID);
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return FALSE;
	} else {
		pQueueSrc = &pObj->Queue[pObj->Rear];
		pYuvSrc->uiVidPathID = pathID;
		memcpy((void *)pQueueSrc, (void *)pYuvSrc, sizeof(MP_VDOENC_YUV_SRC));

		pObj->Rear = (pObj->Rear + 1) % NMR_VDOTRIG_YUVQ_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		gNMRVdoEncObj[pathID].uiEncInCount++;
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return TRUE;
	}
}

BOOL NMR_VdoTrig_GetYuv(UINT32 pathID, MP_VDOENC_YUV_SRC **pYuvSrc, BOOL bKeepElement)
{
	PNMR_VDOTRIG_YUVQ pObj;
	unsigned long flags = 0;

	if (gNMRVdoTrigObj[pathID].BsStart == 0) {
		DBG_DUMP("[VDOTRIG][%d] Get YUV Stream not Start\r\n", pathID);
		return FALSE;
	}

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].rawQueue);
	//DBG_DUMP("[VDOTRIG][%d] GetYuvQ front = %d, rear = %d,  full = %d, keep = %d\r\n",
	//  pathID, pObj->Front, pObj->Rear, (UINT32)pObj->bFull, (UINT32)bKeepElement);
	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu2(&flags);
		if (gNMRVdoEncObj[pathID].bStart == TRUE) {
			DBG_ERR("[VDOTRIG][%d] YUV Queue is Empty!\r\n", pathID);
		} else {
			DBG_DUMP("[VDOTRIG][%d] stopping, YUV Queue is cleared\r\n", pathID);
		}
		return FALSE;
	} else {
		*pYuvSrc = &pObj->Queue[pObj->Front];

		if (!bKeepElement) {
			pObj->Front = (pObj->Front + 1) % NMR_VDOTRIG_YUVQ_MAX;

			if (pObj->Front == pObj->Rear) { // Check Queue full
				pObj->bFull = FALSE;
			}
		}

		NMR_VdoEnc_Unlock_cpu2(&flags);
		return TRUE;
	}
}

UINT32 NMR_VdoTrig_GetYuvCount(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	PNMR_VDOTRIG_YUVQ pObj;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].rawQueue);

	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	NMR_VdoEnc_Unlock_cpu2(&flags);
	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_VDOTRIG_YUVQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_VDOTRIG_YUVQ_MAX;
	} else {
		sq = 0;
	}

	return sq;
}

BOOL NMR_VdoTrig_PutSrcOut(UINT32 pathID, MP_VDOENC_YUV_SRCOUT *pSrcOut)
{
    PNMR_VDOTRIG_SRCOUTQ pObj;
    MP_VDOENC_YUV_SRCOUT *pQueueSrc;
	unsigned long flags = 0;

	if (!pSrcOut) {
		DBG_ERR("[VDOTRIG][%d] Put SrcOut Src is NULL\r\n", pathID);
		return FALSE;
	}

	if (gNMRVdoTrigObj[pathID].BsStart == 0) {
		DBG_DUMP("[VDOTRIG][%d] Put SrcOut Stream not Start\r\n", pathID);
		return FALSE;
	}

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_SRCOUT) {
		char *p = (char *) pSrcOut->y_addr;
		DBG_DUMP("[VDOTRIG][%d] PutSrcOut Input Width = %d, Height = %d, YAddr = 0x%08x, UVAddr = 0x%08x, Ylot = %d, UVlot = %d, Y Data = 0x%02x%02x%02x%02x%02x%02x%02x%02x, timestamp = %llu, time = %u us\r\n",
			pathID, pSrcOut->width, pSrcOut->height, pSrcOut->y_addr, pSrcOut->c_addr, pSrcOut->y_line_offset, pSrcOut->c_line_offset, *p, *(p+1), *(p+2), *(p+3), *(p+4), *(p+5), *(p+6), *(p+7), pSrcOut->timestamp, Perf_GetCurrent());
	}

	NMR_VdoEnc_Lock_cpu2(&flags);

	pObj = &(gNMRVdoTrigObj[pathID].srcoutQueue);
	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		DBG_ERR("[VDOTRIG][%d] SrcOut Queue is Full!\r\n", pathID);
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return FALSE;
	} else {
		pQueueSrc = &pObj->Queue[pObj->Rear];
		pSrcOut->path_id = pathID;
		memcpy((void *)pQueueSrc, (void *)pSrcOut, sizeof(MP_VDOENC_YUV_SRCOUT));

		pObj->Rear = (pObj->Rear + 1) % NMR_VDOTRIG_SRCOUTQ_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return TRUE;
	}
}

BOOL NMR_VdoTrig_GetSrcOut(UINT32 pathID, MP_VDOENC_YUV_SRCOUT **pSrcOut)
{
	PNMR_VDOTRIG_SRCOUTQ pObj;
	unsigned long flags = 0;

	if (gNMRVdoTrigObj[pathID].BsStart == 0) {
		DBG_DUMP("[VDOTRIG][%d] Get SrcOut Stream not Start\r\n", pathID);
		return FALSE;
	}

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].srcoutQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu2(&flags);
		DBG_ERR("[VDOTRIG][%d] SrcOut Queue is Empty!\r\n", pathID);
		return FALSE;
	} else {
		*pSrcOut = &pObj->Queue[pObj->Front];

		pObj->Front = (pObj->Front + 1) % NMR_VDOTRIG_SRCOUTQ_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}

		NMR_VdoEnc_Unlock_cpu2(&flags);
		return TRUE;
	}
}

BOOL NMR_VdoTrig_PutSmartRoi(UINT32 pathID, MP_VDOENC_SMART_ROI_INFO *pSmartRoi)
{
    PNMR_VDOTRIG_SMARTROIQ pObj;
    MP_VDOENC_SMART_ROI_INFO *pQueueSrc;
	unsigned long flags = 0;

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_SMART_ROI) {
		DBG_DUMP("[VDOTRIG][%d] Smart Roi Count = %d, Smart Roi time stamp = %lld, Put time = %u us\r\n", pathID, pSmartRoi->uiRoiNum, pSmartRoi->TimeStamp, Perf_GetCurrent());
	}

	if (gNMRVdoTrigObj[pathID].BsStart == 0 || gNMRVdoEncObj[pathID].bStart == FALSE) {
		DBG_ERR("[VDOTRIG][%d] Set Smart Roi Stream not Start\r\n", pathID);
		return FALSE;
	}

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].smartroiQueue);
	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		DBG_ERR("[VDOTRIG][%d] Smart Roi Queue is Full!\r\n", pathID);
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return FALSE;
	} else {
		pQueueSrc = &pObj->Queue[pObj->Rear];
		if (pSmartRoi) {
			memcpy((void *)pQueueSrc, (void *)pSmartRoi, sizeof(MP_VDOENC_SMART_ROI_INFO));
		}

		pObj->Rear = (pObj->Rear + 1) % NMR_VDOTRIG_SMARTROIQ_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return TRUE;
	}
}

BOOL NMR_VdoTrig_GetSmartRoi(UINT32 pathID, MP_VDOENC_SMART_ROI_INFO *pSmartRoi)
{
	PNMR_VDOTRIG_SMARTROIQ pObj;
	MP_VDOENC_SMART_ROI_INFO *pQueueSrc;
	unsigned long flags=0;

	if (gNMRVdoTrigObj[pathID].BsStart == 0) {
		DBG_DUMP("[VDOTRIG][%d] Get Smart Roi Stream not Start\r\n", pathID);
		return FALSE;
	}

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].smartroiQueue);
	//DBG_DUMP("[VDOTRIG][%d] GetYuvQ front = %d, rear = %d,  full = %d, keep = %d\r\n",
	//  pathID, pObj->Front, pObj->Rear, (UINT32)pObj->bFull, (UINT32)bKeepElement);
	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu2(&flags);
		DBG_ERR("[VDOTRIG][%d] Smart Roi Queue is Empty!\r\n", pathID);
		return FALSE;
	} else {
		pQueueSrc = &pObj->Queue[pObj->Front];

		if (pSmartRoi) {
			memcpy((void *)pSmartRoi, (void *)pQueueSrc, sizeof(MP_VDOENC_SMART_ROI_INFO));
		}

		pObj->Front = (pObj->Front + 1) % NMR_VDOTRIG_SMARTROIQ_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}

		NMR_VdoEnc_Unlock_cpu2(&flags);
		return TRUE;
	}
}

UINT32 NMR_VdoTrig_GetSmartRoiCount(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	PNMR_VDOTRIG_SMARTROIQ pObj;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].smartroiQueue);

	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	NMR_VdoEnc_Unlock_cpu2(&flags);
	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_VDOTRIG_SMARTROIQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_VDOTRIG_SMARTROIQ_MAX;
	} else {
		sq = 0;
	}

	return sq;
}

PMP_VDOENC_ENCODER NMR_VdoTrig_GetVidEncoder(UINT32 pathID)
{
	return &gNMRVdoTrigObj[pathID].Encoder;
}

void NMR_VdoTrig_InitJobQ_H26X(void)
{
	memset(&gNMRVdoTrigJobQ_H26X, 0, sizeof(NMR_VDOTRIG_JOBQ));
}

void NMR_VdoTrig_InitJobQ_H26X_Snapshot(void)
{
	memset(&gNMRVdoTrigJobQ_H26X_Snapshot, 0, sizeof(NMR_VDOTRIG_JOBQ));
}

void NMR_VdoTrig_InitJobQ_JPEG(void)
{
	memset(&gNMRVdoTrigJobQ_JPEG, 0, sizeof(NMR_VDOTRIG_JOBQ));
}

#ifdef VDOENC_LL
void NMR_VdoTrig_InitJobQ_WrapBS(void)
{
	memset(&gNMRVdoTrigJobQ_WrapBS, 0, sizeof(NMR_VDOTRIG_WRAPBS_JOBQ));
}
#endif

ER _NMR_VdoTrig_PutJob_H26X(UINT32 pathID)
{
	unsigned long flags = 0;
	NMR_VdoEnc_Lock_cpu(&flags);

	if ((gNMRVdoTrigJobQ_H26X.Front == gNMRVdoTrigJobQ_H26X.Rear) && (gNMRVdoTrigJobQ_H26X.bFull == TRUE)) {
		static UINT32 sCount;
		NMR_VdoEnc_Unlock_cpu(&flags);
		if (sCount % 128 == 0) {
			DBG_WRN("[VDOTRIG] Job Queue(H26x) is Full, Job Id = %d\r\n", pathID);
		}
		sCount++;
		return E_SYS;
	} else {
		gNMRVdoTrigJobQ_H26X.Queue[gNMRVdoTrigJobQ_H26X.Rear] = pathID;
		gNMRVdoTrigJobQ_H26X.Rear = (gNMRVdoTrigJobQ_H26X.Rear + 1) % NMR_VDOTRIG_JOBQ_MAX;
		if (gNMRVdoTrigJobQ_H26X.Front == gNMRVdoTrigJobQ_H26X.Rear) { // Check Queue full
			gNMRVdoTrigJobQ_H26X.bFull = TRUE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
		return E_OK;
	}
}

ER _NMR_VdoTrig_PutJob_JPEG(UINT32 pathID)
{
	unsigned long flags = 0;
	NMR_VdoEnc_Lock_cpu(&flags);

	if ((gNMRVdoTrigJobQ_JPEG.Front == gNMRVdoTrigJobQ_JPEG.Rear) && (gNMRVdoTrigJobQ_JPEG.bFull == TRUE)) {
		static UINT32 sCount;
		NMR_VdoEnc_Unlock_cpu(&flags);
		if (sCount % 128 == 0) {
			DBG_WRN("[VDOTRIG] Job Queue(Jpeg) is Full, Job Id = %d\r\n", pathID);
		}
		sCount++;
		return E_SYS;
	} else {
		gNMRVdoTrigJobQ_JPEG.Queue[gNMRVdoTrigJobQ_JPEG.Rear] = pathID;
		gNMRVdoTrigJobQ_JPEG.Rear = (gNMRVdoTrigJobQ_JPEG.Rear + 1) % NMR_VDOTRIG_JOBQ_MAX;
		if (gNMRVdoTrigJobQ_JPEG.Front == gNMRVdoTrigJobQ_JPEG.Rear) { // Check Queue full
			gNMRVdoTrigJobQ_JPEG.bFull = TRUE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
		return E_OK;
	}
}

ER NMR_VdoTrig_PutJob(UINT32 pathID)
{
	if (gNMRVdoEncObj[pathID].uiVidCodec == MEDIAVIDENC_MJPG) {
		return _NMR_VdoTrig_PutJob_JPEG(pathID);
	} else {
		return _NMR_VdoTrig_PutJob_H26X(pathID);
	}
}

ER NMR_VdoTrig_PutJob_H26X_Snapshot(UINT32 pathID)
{
	unsigned long flags = 0;
	NMR_VdoEnc_Lock_cpu(&flags);

	if ((gNMRVdoTrigJobQ_H26X_Snapshot.Front == gNMRVdoTrigJobQ_H26X_Snapshot.Rear) && (gNMRVdoTrigJobQ_H26X_Snapshot.bFull == TRUE)) {
		static UINT32 sCount;
		NMR_VdoEnc_Unlock_cpu(&flags);
		if (sCount % 128 == 0) {
			DBG_WRN("[VDOTRIG] Job Queue(H26x_Snapshot) is Full, Job Id = %d\r\n", pathID);
		}
		sCount++;
		return E_SYS;
	} else {
		gNMRVdoTrigJobQ_H26X_Snapshot.Queue[gNMRVdoTrigJobQ_H26X_Snapshot.Rear] = pathID;
		gNMRVdoTrigJobQ_H26X_Snapshot.Rear = (gNMRVdoTrigJobQ_H26X_Snapshot.Rear + 1) % NMR_VDOTRIG_JOBQ_MAX;
		if (gNMRVdoTrigJobQ_H26X_Snapshot.Front == gNMRVdoTrigJobQ_H26X_Snapshot.Rear) { // Check Queue full
			gNMRVdoTrigJobQ_H26X_Snapshot.bFull = TRUE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
		return E_OK;
	}
}

#ifdef VDOENC_LL
ER NMR_VdoTrig_PutJob_WrapBS(UINT32 pathID)
{
	unsigned long flags = 0;
	NMR_VdoEnc_Lock_cpu(&flags);

	if ((gNMRVdoTrigJobQ_WrapBS.Front == gNMRVdoTrigJobQ_WrapBS.Rear) && (gNMRVdoTrigJobQ_WrapBS.bFull == TRUE)) {
		static UINT32 sCount;
		NMR_VdoEnc_Unlock_cpu(&flags);
		if (sCount % 128 == 0) {
			DBG_WRN("[VDOTRIG] Wrap BS Job Queue is Full, Job Id = %d\r\n", pathID);
		}
		sCount++;
		return E_SYS;
	} else {
		gNMRVdoTrigJobQ_WrapBS.Queue[gNMRVdoTrigJobQ_WrapBS.Rear] = pathID;
		gNMRVdoTrigJobQ_WrapBS.Rear = (gNMRVdoTrigJobQ_WrapBS.Rear + 1) % NMR_VDOTRIG_JOBQ_MAX;
		if (gNMRVdoTrigJobQ_WrapBS.Front == gNMRVdoTrigJobQ_WrapBS.Rear) { // Check Queue full
			gNMRVdoTrigJobQ_WrapBS.bFull = TRUE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
		return E_OK;
	}

}
#endif

ER NMR_VdoTrig_GetJob_H26X(UINT32 *pPathID)
{
	unsigned long flags = 0;
	NMR_VdoEnc_Lock_cpu(&flags);

	if ((gNMRVdoTrigJobQ_H26X.Front == gNMRVdoTrigJobQ_H26X.Rear) && (gNMRVdoTrigJobQ_H26X.bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu(&flags);
		DBG_ERR("[VDOTRIG] Job Queue(H26x) is Empty!\r\n");
		return E_SYS;
	} else {
		*pPathID = gNMRVdoTrigJobQ_H26X.Queue[gNMRVdoTrigJobQ_H26X.Front];
		gNMRVdoTrigJobQ_H26X.Front = (gNMRVdoTrigJobQ_H26X.Front + 1) % NMR_VDOTRIG_JOBQ_MAX;
		if (gNMRVdoTrigJobQ_H26X.Front == gNMRVdoTrigJobQ_H26X.Rear) { // Check Queue full
			gNMRVdoTrigJobQ_H26X.bFull = FALSE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
		return E_OK;
	}
}

ER NMR_VdoTrig_GetJob_JPEG(UINT32 *pPathID)
{
	unsigned long flags = 0;
	NMR_VdoEnc_Lock_cpu(&flags);

	if ((gNMRVdoTrigJobQ_JPEG.Front == gNMRVdoTrigJobQ_JPEG.Rear) && (gNMRVdoTrigJobQ_JPEG.bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu(&flags);
		DBG_ERR("[VDOTRIG] Job Queue(Jpeg) is Empty!\r\n");
		return E_SYS;
	} else {
		*pPathID = gNMRVdoTrigJobQ_JPEG.Queue[gNMRVdoTrigJobQ_JPEG.Front];
		gNMRVdoTrigJobQ_JPEG.Front = (gNMRVdoTrigJobQ_JPEG.Front + 1) % NMR_VDOTRIG_JOBQ_MAX;
		if (gNMRVdoTrigJobQ_JPEG.Front == gNMRVdoTrigJobQ_JPEG.Rear) { // Check Queue full
			gNMRVdoTrigJobQ_JPEG.bFull = FALSE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
		return E_OK;
	}
}

ER NMR_VdoTrig_GetJob_H26X_Snapshot(UINT32 *pPathID)
{
	unsigned long flags = 0;
	NMR_VdoEnc_Lock_cpu(&flags);

	if ((gNMRVdoTrigJobQ_H26X_Snapshot.Front == gNMRVdoTrigJobQ_H26X_Snapshot.Rear) && (gNMRVdoTrigJobQ_H26X_Snapshot.bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu(&flags);
		DBG_ERR("[VDOTRIG] Job Queue(H26x_Snapshot) is Empty!\r\n");
		return E_SYS;
	} else {
		*pPathID = gNMRVdoTrigJobQ_H26X_Snapshot.Queue[gNMRVdoTrigJobQ_H26X_Snapshot.Front];
		gNMRVdoTrigJobQ_H26X_Snapshot.Front = (gNMRVdoTrigJobQ_H26X_Snapshot.Front + 1) % NMR_VDOTRIG_JOBQ_MAX;
		if (gNMRVdoTrigJobQ_H26X_Snapshot.Front == gNMRVdoTrigJobQ_H26X_Snapshot.Rear) { // Check Queue full
			gNMRVdoTrigJobQ_H26X_Snapshot.bFull = FALSE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
		return E_OK;
	}
}

#ifdef VDOENC_LL
ER NMR_VdoTrig_GetJob_WrapBS(UINT32 *pPathID)
{
	unsigned long flags = 0;
	NMR_VdoEnc_Lock_cpu(&flags);

	if ((gNMRVdoTrigJobQ_WrapBS.Front == gNMRVdoTrigJobQ_WrapBS.Rear) && (gNMRVdoTrigJobQ_WrapBS.bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu(&flags);
		DBG_ERR("[VDOTRIG] Wrap BS Job Queue is Empty!\r\n");
		return E_SYS;
	} else {
		*pPathID = gNMRVdoTrigJobQ_WrapBS.Queue[gNMRVdoTrigJobQ_WrapBS.Front];
		gNMRVdoTrigJobQ_WrapBS.Front = (gNMRVdoTrigJobQ_WrapBS.Front + 1) % NMR_VDOTRIG_JOBQ_MAX;
		if (gNMRVdoTrigJobQ_WrapBS.Front == gNMRVdoTrigJobQ_WrapBS.Rear) { // Check Queue full
			gNMRVdoTrigJobQ_WrapBS.bFull = FALSE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
		return E_OK;
	}
}
#endif

UINT32 NMR_VdoTrig_GetJobCount_H26X(void)
{
	UINT32 front, rear, full, sq = 0;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu(&flags);
	front = gNMRVdoTrigJobQ_H26X.Front;
	rear = gNMRVdoTrigJobQ_H26X.Rear;
	full = gNMRVdoTrigJobQ_H26X.bFull;
	NMR_VdoEnc_Unlock_cpu(&flags);

	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_VDOTRIG_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_VDOTRIG_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}

UINT32 NMR_VdoTrig_GetJobCount_JPEG(void)
{
	UINT32 front, rear, full, sq = 0;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu(&flags);
	front = gNMRVdoTrigJobQ_JPEG.Front;
	rear = gNMRVdoTrigJobQ_JPEG.Rear;
	full = gNMRVdoTrigJobQ_JPEG.bFull;
	NMR_VdoEnc_Unlock_cpu(&flags);

	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_VDOTRIG_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_VDOTRIG_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}

UINT32 NMR_VdoTrig_GetJobCount_H26X_Snapshot(void)
{
	UINT32 front, rear, full, sq = 0;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu(&flags);
	front = gNMRVdoTrigJobQ_H26X_Snapshot.Front;
	rear = gNMRVdoTrigJobQ_H26X_Snapshot.Rear;
	full = gNMRVdoTrigJobQ_H26X_Snapshot.bFull;
	NMR_VdoEnc_Unlock_cpu(&flags);

	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_VDOTRIG_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_VDOTRIG_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}

#ifdef VDOENC_LL
UINT32 NMR_VdoTrig_GetJobCount_WrapBS(void)
{
	UINT32 front, rear, full, sq = 0;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu(&flags);
	front = gNMRVdoTrigJobQ_WrapBS.Front;
	rear = gNMRVdoTrigJobQ_WrapBS.Rear;
	full = gNMRVdoTrigJobQ_WrapBS.bFull;
	NMR_VdoEnc_Unlock_cpu(&flags);

	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (NMR_VDOTRIG_JOBQ_MAX - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = NMR_VDOTRIG_JOBQ_MAX;
	} else {
		sq = 0;
	}
	return sq;
}
#endif

UINT32 NMR_VdoTrig_GetBSCount(UINT32 pathID)
{
	UINT32 front, rear, full, sq = 0;
	PNMR_VDOTRIG_BSQ pObj;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].bsQueue);

	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	NMR_VdoEnc_Unlock_cpu2(&flags);
	if (front < rear) {
		sq = (rear - front);
	} else if (front > rear) {
		sq = (gNMRVdoEncObj[pathID].uiMaxFrameQueue - (front - rear));
	} else if (front == rear && full == TRUE) {
		sq = gNMRVdoEncObj[pathID].uiMaxFrameQueue;
	} else {
		sq = 0;
	}

	return sq;
}

BOOL NMR_VdoTrig_CheckBS(UINT32 pathID, UINT32 *uiAddr, UINT32 *uiSize, UINT32 *uiAddr2, UINT32 *uiSize2, UINT32 *uiStartAddr)
{
	UINT32 front, rear, full;
	PNMR_VDOTRIG_BSQ pObj;
	BOOL bResult = FALSE;
	static UINT32 uiOriAddr = 0, uiOriSize = 0;
	unsigned long flags = 0;

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_RING_BUF) {
		uiOriAddr = *uiAddr;
		uiOriSize = *uiSize;
	}

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].bsQueue);
	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	NMR_VdoEnc_Unlock_cpu2(&flags);

	if (front != rear) {
		if (rear == 0) {
			rear = gNMRVdoEncObj[pathID].uiMaxFrameQueue - 1;
		} else {
			rear = rear - 1;
		}

		if (pObj->Queue[front].Addr <= pObj->Queue[rear].Addr) { //check free space of left side or right side
			if (gNMRVdoEncObj[pathID].MemInfo.bBSRingMode == TRUE) {
				if (*uiAddr + *uiSize <= pObj->Queue[front].Addr) {
					*uiSize = pObj->Queue[front].Addr - *uiAddr;
					*uiAddr2 = 0;
					*uiSize2 = 0;
					bResult = TRUE;
				} else if ((*uiAddr >= pObj->Queue[rear].Addr + pObj->Queue[rear].Size) &&
							((gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - *uiAddr) > (1024 + _nmr_vdotrig_nalu_size_info_max_size())) &&
							(((gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - *uiAddr) + (pObj->Queue[front].Addr - *uiStartAddr)) >= *uiSize)) {
					*uiSize = gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - *uiAddr;
					*uiAddr2 = *uiStartAddr;
					*uiSize2 = pObj->Queue[front].Addr - *uiStartAddr;
					bResult = TRUE;
				}
			} else {
				if (*uiAddr + *uiSize <= pObj->Queue[front].Addr) {
					*uiSize = pObj->Queue[front].Addr - *uiAddr;
					*uiAddr2 = 0;
					*uiSize2 = 0;
					bResult = TRUE;
				} else if ((*uiAddr >= pObj->Queue[rear].Addr + pObj->Queue[rear].Size) && (*uiAddr + *uiSize <= gNMRVdoEncObj[pathID].MemInfo.uiBSEnd)) {
					*uiSize = gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - *uiAddr;
					*uiAddr2 = 0;
					*uiSize2 = 0;
					bResult = TRUE;
				} else if (pObj->Queue[front].Addr - *uiStartAddr >= *uiSize) {
					*uiAddr = *uiStartAddr;
					*uiSize = pObj->Queue[front].Addr - *uiStartAddr;
					*uiAddr2 = 0;
					*uiSize2 = 0;
					bResult = TRUE;
				}
			}
		} else if (*uiAddr >= pObj->Queue[rear].Addr + pObj->Queue[rear].Size && *uiAddr + *uiSize <= pObj->Queue[front].Addr) {  //check middle free space
			*uiSize = pObj->Queue[front].Addr - *uiAddr;
			*uiAddr2 = 0;
			*uiSize2 = 0;
			bResult = TRUE;
		}
	} else if (front == rear && full == TRUE) {
		bResult = FALSE;
	} else {
		if (gNMRVdoEncObj[pathID].bBsQuickRollback == FALSE) {
			if (*uiAddr + *uiSize <= gNMRVdoEncObj[pathID].MemInfo.uiBSEnd) {
				*uiSize = gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - *uiAddr;
			} else {
				*uiAddr = *uiStartAddr;
				*uiSize = gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - *uiStartAddr;
			}
			*uiAddr2 = 0;
			*uiSize2 = 0;
		} else {
			// bs quick rollback mode => because bs buffer is all empty, just use starting address
			*uiAddr = *uiStartAddr;
			*uiSize = gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - *uiStartAddr;
			*uiAddr2 = 0;
			*uiSize2 = 0;
		}
		bResult = TRUE; //no lock bs
	}

	if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_RING_BUF) {
		if (bResult == FALSE) {
			DBG_ERR("[VDOTRIG][%d] check bs err, assigned enc buf (0x%08x, %d), ori enc buf (0x%08x, %d), front item = (%d, 0x%08x, %d), rear item = (%d, 0x%08x, %d)\r\n",
				pathID, *uiAddr, *uiSize, uiOriAddr, uiOriSize, pObj->Front, pObj->Queue[front].Addr, pObj->Queue[front].Size, pObj->Rear, pObj->Queue[rear].Addr, pObj->Queue[rear].Size);
		} else {
			DBG_DUMP("[VDOTRIG][%d] check bs ok, assigned enc buf (0x%08x, %d), ori enc buf (0x%08x, %d), front item = (%d, 0x%08x, %d), rear item = (%d, 0x%08x, %d)\r\n",
				pathID, *uiAddr, *uiSize, uiOriAddr, uiOriSize, pObj->Front, pObj->Queue[front].Addr, pObj->Queue[front].Size, pObj->Rear, pObj->Queue[rear].Addr, pObj->Queue[rear].Size);
		}
	}

	return bResult;
}

BOOL NMR_VdoTrig_LockBS(UINT32 pathID, UINT32 uiAddr, UINT32 uiSize)
{
	PNMR_VDOTRIG_BSQ pObj;
	unsigned long flags = 0;

	if (uiSize == 0) {
		DBG_ERR("[VDOTRIG][%d] BS Lock Size is 0\r\n", pathID);
		return FALSE;
	}

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].bsQueue);

#if NMR_VDOTRIG_DUMP_LOCK_INFO
	DBG_DUMP("[VDOTRIG][%d] lockBS addr = 0x%08x, size = %d, front = %d, rear = %d,  full = %d\r\n",
	  pathID, uiAddr, uiSize, pObj->Front, pObj->Rear, (UINT32)pObj->bFull);
#endif

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		DBG_WRN("[VDOTRIG][%d] BS Lock Queue is Full!\r\n", pathID);
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return FALSE;
	} else {
		pObj->Queue[pObj->Rear].Addr = uiAddr;
		pObj->Queue[pObj->Rear].Size = uiSize;

		pObj->Rear = (pObj->Rear + 1) % gNMRVdoEncObj[pathID].uiMaxFrameQueue;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		NMR_VdoEnc_Unlock_cpu2(&flags);
		return TRUE;
	}
}

BOOL NMR_VdoTrig_UnlockBS(UINT32 pathID, UINT32 uiAddr)
{
	PNMR_VDOTRIG_BSQ   pObj;
	unsigned long flags=0;

	if (uiAddr == 0) {
		return FALSE;
	}

#if NMR_VDOTRIG_RING_BUF_TEST
	if (uiPreAddr[pathID] == 0) {
		uiPreAddr[pathID] = uiAddr;
		return TRUE;
	} else if (uiPreAddr2[pathID] == 0) {
		uiPreAddr2[pathID] = uiAddr;
		return TRUE;
	} else {
		uiPreAddr3[pathID] = uiAddr;
		uiAddr = uiPreAddr[pathID];
	}
#endif

	NMR_VdoEnc_Lock_cpu2(&flags);
	pObj = &(gNMRVdoTrigObj[pathID].bsQueue);

#if NMR_VDOTRIG_DUMP_LOCK_INFO
	DBG_DUMP("[VDOTRIG][%d] UnlockBS addr = 0x%08x, front = %d, rear = %d,  full = %d\r\n",
	  pathID, uiAddr, pObj->Front, pObj->Rear, (UINT32)pObj->bFull);
#endif

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu2(&flags);
		DBG_WRN("[VDOTRIG][%d] BS Lock Queue is Empty!\r\n", pathID);
		return FALSE;
	} else {
		if (uiAddr == pObj->Queue[pObj->Front].Addr) {
			pObj->Front = (pObj->Front + 1) % gNMRVdoEncObj[pathID].uiMaxFrameQueue;

			if (pObj->Front == pObj->Rear) { // Check Queue full
				pObj->bFull = FALSE;
			}
		} else {
			UINT32 index = pObj->Front;

			while (index != pObj->Rear) {
				if (uiAddr == pObj->Queue[index].Addr) {
					break;
				}

				index = (index + 1) % gNMRVdoEncObj[pathID].uiMaxFrameQueue;
			}
			NMR_VdoEnc_Unlock_cpu2(&flags);
			DBG_WRN("[VDOTRIG][%d] BS Unlock addr = 0x%08x not in head (idx = %d), wait addr = 0x%08x size = %d front = %d rear = %d\r\n", pathID, uiAddr, index, pObj->Queue[pObj->Front].Addr, pObj->Queue[pObj->Front].Size, pObj->Front, pObj->Rear);
			return FALSE;
		}

		NMR_VdoEnc_Unlock_cpu2(&flags);

#if NMR_VDOTRIG_RING_BUF_TEST
		uiPreAddr[pathID] = uiPreAddr2[pathID];
		uiPreAddr2[pathID] = uiPreAddr3[pathID];
#endif

		return TRUE;
	}
}

void NMR_VdoTrig_Stop(UINT32 pathID)
{
	MP_VDOENC_YUV_SRC     *p_YuvSrc = 0;
#ifdef VDOENC_LL
	NMI_VDOENC_BS_INFO    *pvidBSinfo;
	pvidBSinfo = &(gNMRVdoTrigObj[pathID].vidBSinfo);
	gNMRVidTrigStop[pathID] = 1;
#else
	NMI_VDOENC_BS_INFO    vidBSinfo = {0};
#endif

	while (NMR_VdoTrig_GetYuvCount(pathID)) {
		NMR_VdoTrig_GetYuv(pathID, &p_YuvSrc, FALSE);
		if (p_YuvSrc) {
#ifdef VDOENC_LL
			pvidBSinfo->PathID = pathID;
			pvidBSinfo->BufID = p_YuvSrc->uiBufID;
			pvidBSinfo->Occupied = 0;
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) pvidBSinfo);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FLUSH_DROP); // [IN] PUSH_DROP
			}
#else
			vidBSinfo.PathID = pathID;
			vidBSinfo.BufID = p_YuvSrc->uiBufID;
			vidBSinfo.Occupied = 0;
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FLUSH_DROP); // [IN] PUSH_DROP
			}
#endif
		}
	}
#ifdef __KERNEL__
	gNMRVidTrig1stopen[pathID] = FALSE;
#endif
}

void NMR_VdoTrig_CancelH26xTask(void)
{
	UINT32 pathID = 0;
	FLGPTN uiFlag;
	MP_VDOENC_YUV_SRC  *p_YuvSrc = 0;
#ifdef VDOENC_LL
	NMI_VDOENC_BS_INFO  *pvidBSinfo;
#else
	NMI_VDOENC_BS_INFO  vidBSinfo = {0};
#endif

	// remove all h26x job queue & release YUV
	while (NMR_VdoTrig_GetJobCount_H26X()) {
		if (E_OK == NMR_VdoTrig_GetJob_H26X(&pathID)) {
			if (NMR_VdoTrig_GetYuvCount(pathID) > 0) {
				NMR_VdoTrig_GetYuv(pathID, &p_YuvSrc, FALSE);
#ifdef VDOENC_LL
				pvidBSinfo = &(gNMRVdoTrigObj[pathID].vidBSinfo);
				if (p_YuvSrc) {
					DBG_WRN("[VDOTRIG][%d] Cancel Job & Release YUV(0x%08x, 0x%08x) !!\r\n", pathID, p_YuvSrc->uiYAddr, p_YuvSrc->uiCbAddr);
					pvidBSinfo->PathID = pathID;
					pvidBSinfo->BufID = p_YuvSrc->uiBufID;
					pvidBSinfo->Occupied = 0;
					if (gNMRVdoEncObj[pathID].CallBackFunc) {
						(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) pvidBSinfo);
						(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FLUSH_DROP); // [IN] PUSH_DROP
					}
				}
#else
				if (p_YuvSrc) {
					DBG_WRN("[VDOTRIG][%d] Cancel Job & Release YUV(0x%08x, 0x%08x) !!\r\n", pathID, p_YuvSrc->uiYAddr, p_YuvSrc->uiCbAddr);
					vidBSinfo.PathID = pathID;
					vidBSinfo.BufID = p_YuvSrc->uiBufID;
					vidBSinfo.Occupied = 0;
					if (gNMRVdoEncObj[pathID].CallBackFunc) {
						(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo);
						(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(pathID), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FLUSH_DROP); // [IN] PUSH_DROP
					}
				}
#endif
			}
		}
	}

	// cancel current encoding h26x task if need
	for (pathID = 0; pathID < PATH_MAX_COUNT; pathID++) {
		if ((gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) || (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265)) {
			if (gNMRVdoTrigObj[pathID].uiEncStatus == NMR_VDOENC_ENC_STATUS_ENCODE_TRIG) {
				DBG_WRN("[VDOTRIG][%d] Cancel encoding task .. begin\r\n", pathID);
				if (gNMRVdoTrigObj[pathID].Encoder.SetInfo) {
					(gNMRVdoTrigObj[pathID].Encoder.SetInfo)(VDOENC_SET_RESET, 0, 0, 0);
				}
				wai_flg(&uiFlag, FLG_ID_NMR_VDOTRIG_H26X, FLG_NMR_VDOENC_IDLE, TWF_ORW);

				DBG_WRN("[VDOTRIG][%d] Cancel encoding task .. end\r\n", pathID);
			}
		}
	}

}

