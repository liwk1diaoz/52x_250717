/*
    @file       WavStudioAPI.c
    @ingroup    mILIBWAVSTUDIO

    @brief      Wave studio task API.

                This file includes all exposed APIs of wave studio task.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

/**
    @addtogroup mILIBWAVSTUDIO
*/
//@{
#include "wavstudio_int.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "wavstudio_id.h"
#include "wavstudio_aud_intf.h"
#include "ctl_aud_int.h"
UINT32 gWavStudDxSndHdl = 0;

#ifdef DEBUG
unsigned int wavstudio_debug_level = THIS_DBGLVL;
#endif

WAVSTUD_OBJ gWavStudObj = {0};
extern SEM_HANDLE SEM_ID_WAVSTUD_PLAY_QUE;
extern UINT32 gWavStudApplyAddr;
extern UINT32 gWavStudApplySize;
static AUDIO_CH gWavStudLbCh = 0;
#if defined (__KERNEL__)
extern BOOL wavstudio_first_boot;
#endif

__inline UINT32 WavStud_GetNeedMemSize(PWAVSTUD_INFO_SET pWavInfoSet, WAVSTUD_ACT actType);



//====== Dbg ======
#if (2 <= THIS_DBGLVL)
void WavStud_DmpModuleInfo(void)
{
	PWAVSTUD_ACT_OBJ pActObj;
	PAUDIO_BUF_QUEUE pAudBufQue;
	UINT32 i = 0, j = 0;
	DBG_IND("=== WavStudio ===================================== \r\n");
	//DBG_DUMP("pWavStudCB    = 0x%x \r\n",   gWavStudObj.pWavStudCB     );
	DBG_DUMP("playOutDev    = 0x%x \r\n",   gWavStudObj.playOutDev);
	for (i = 0; i < WAVSTUD_ACT_MAX; i++) {
		pActObj = &(gWavStudObj.actObj[i]);
		if (!pActObj->opened) {
			continue;
		}
		DBG_DUMP("pActObj[%d]===\r\n", i);
		DBG_DUMP("  uiAddr    =0x%x\r\n", pActObj->mem.uiAddr);
		DBG_DUMP("  uiSize    =0x%x\r\n", pActObj->mem.uiSize);
		DBG_DUMP("  audSR     =0x%x\r\n", pActObj->audInfo.aud_sr);
		DBG_DUMP("  audCh     =0x%x\r\n", pActObj->audInfo.aud_ch);
		DBG_DUMP("  bitPerSample =0x%x\r\n", pActObj->audInfo.bitpersample);
		DBG_DUMP("  audVol    =0x%x\r\n", pActObj->audVol);
		DBG_DUMP("  actId     =0x%x\r\n", pActObj->actId);
		DBG_DUMP("  mode      =0x%x\r\n", pActObj->mode);
		DBG_DUMP("  currentCount      =0x%x\r\n", pActObj->currentCount);
		DBG_DUMP("  stopCount  =0x%llx\r\n", pActObj->stopCount);
		DBG_DUMP("  unitSize    =0x%x\r\n", pActObj->unitSize);
		DBG_DUMP("  pAudBufQueNext    =0x%x\r\n", (UINT32)pActObj->pAudBufQueNext);
		DBG_DUMP("  unitTime    =0x%x\r\n", pActObj->unitTime);
		DBG_DUMP("  opened    =0x%x\r\n", pActObj->opened);
		pAudBufQue = pActObj->pAudBufQueFirst;
		for (j = 0; j < pActObj->audBufQueCnt; j++) {
			if (pAudBufQue) {
				DBG_DUMP("AudBufQue[%d]=0x%x, 0x%x, 0x%x, 0x%x\r\n", j, (UINT32)pAudBufQue, pAudBufQue->uiAddress, pAudBufQue->uiSize, pAudBufQue->uiValidSize);
				pAudBufQue = pAudBufQue->pNext;
			} else {
				DBG_ERR("idx=%d NULL over %d\r\n", j, pActObj->audBufQueCnt);
			}
		}
		#if WAVSTUD_FIX_BUF == ENABLE
		{
			DBG_DUMP("AudTmpBufQue=0x%x, 0x%x, 0x%x, 0x%x\r\n", (UINT32)pActObj->pAudBufQueTmp, pActObj->pAudBufQueTmp->uiAddress, pActObj->pAudBufQueTmp->uiSize, pActObj->pAudBufQueTmp->uiValidSize);
		}
		#endif
		DBG_DUMP("======================================== \r\n");
	}
}
#else
void WavStud_DmpModuleInfo(void) {}
#endif

#if 0
#pragma mark -
#endif

void wavstudio_dump_module_info(WAVSTUD_ACT act_type)
{
	PAUDIO_BUF_QUEUE pAudBufQue;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[act_type]);
	UINT32 i = 0;
	UINT32 status;

	status = wavstudio_get_config(WAVSTUD_CFG_GET_STATUS, act_type, 0);

	if (WAVSTUD_STS_GOING != status) {
		return;
	}

	if (act_type == WAVSTUD_ACT_LB && gWavStudObj.aec_en == 0) {
		return;
	}

	pActObj = &(gWavStudObj.actObj[act_type]);

	DBG_DUMP("pActObj[%d]\r\n", act_type);
	DBG_DUMP("addr             = 0x%x\r\n", pActObj->mem.uiAddr);
	DBG_DUMP("size             = 0x%x\r\n", pActObj->mem.uiSize);
	DBG_DUMP("unitSize         = 0x%x\r\n", pActObj->unitSize);
	DBG_DUMP("audSR            = %d\r\n", pActObj->audInfo.aud_sr);
	DBG_DUMP("audCh            = %d\r\n", pActObj->audInfo.aud_ch);
	DBG_DUMP("audch_num        = %d\r\n", pActObj->audInfo.ch_num);
	DBG_DUMP("audBPS           = %d\r\n", pActObj->audInfo.bitpersample);
	DBG_DUMP("buf_sample_cnt   = %d\r\n", pActObj->buf_sample_cnt);
	DBG_DUMP("queEntryNum      = %d\r\n", pActObj->queEntryNum);
	DBG_DUMP("remain_entry_num = %d\r\n", pActObj->remain_entry_num);
	DBG_DUMP("currentBufRemain = 0x%x\r\n", pActObj->currentBufRemain);
	DBG_DUMP("audChs           = %d\r\n", pActObj->audChs);
	DBG_DUMP("lock_addr        = 0x%x\r\n", pActObj->lock_addr);
	DBG_DUMP("buf_sample_cnt   = %d\r\n", pActObj->buf_sample_cnt);
	DBG_DUMP("pAudBufQueNext   = 0x%x, 0x%x\r\n", (UINT32)pActObj->pAudBufQueNext, (pActObj->pAudBufQueNext)? (UINT32)pActObj->pAudBufQueNext->uiAddress : 0);
	DBG_DUMP("pAudBufQueHead   = 0x%x, 0x%x\r\n", (UINT32)pActObj->pAudBufQueHead, (pActObj->pAudBufQueHead)? (UINT32)pActObj->pAudBufQueHead->uiAddress : 0);
	DBG_DUMP("pAudBufQueFirst  = 0x%x, 0x%x\r\n", (UINT32)pActObj->pAudBufQueFirst, (pActObj->pAudBufQueFirst)? (UINT32)pActObj->pAudBufQueFirst->uiAddress : 0);
	DBG_DUMP("pAudBufQueTail   = 0x%x, 0x%x\r\n", (UINT32)pActObj->pAudBufQueTail, (pActObj->pAudBufQueTail)? (UINT32)pActObj->pAudBufQueTail->uiAddress : 0);
	pAudBufQue = pActObj->pAudBufQueFirst;

	if (pAudBufQue) {
		for (i = 0; i < (pActObj->audBufQueCnt-1); i++) {
			if (pAudBufQue) {
				DBG_DUMP("AudBufQue[%d] = 0x%x, 0x%x, 0x%x, 0x%x, lock=%d\r\n", i, (UINT32)pAudBufQue, pAudBufQue->uiAddress, pAudBufQue->uiSize, pAudBufQue->uiValidSize, pActObj->wavAudQue.pDoneQueLock[i]);
				pAudBufQue = pAudBufQue->pNext;
			} else {
				DBG_ERR("idx=%d NULL over %d\r\n", i, pActObj->audBufQueCnt);
			}
		}
	}
	#if WAVSTUD_FIX_BUF == ENABLE
	{
		DBG_DUMP("AudTmpBufQue = 0x%x, 0x%x, 0x%x, 0x%x\r\n", (UINT32)pActObj->pAudBufQueTmp, pActObj->pAudBufQueTmp->uiAddress, pActObj->pAudBufQueTmp->uiSize, pActObj->pAudBufQueTmp->uiValidSize);
	}
	#endif
}


//======================== Internal ==============================
__inline UINT32 WavStud_GetNeedMemSize(PWAVSTUD_INFO_SET pWavInfoSet, WAVSTUD_ACT actType)
{
	UINT32 memSize = 0;
	UINT32 unitSize = 0;
	UINT32 count = pWavInfoSet->obj_count;
	UINT32 tmp_size;

	if (WAVSTUD_UNIT_TIME_0 == pWavInfoSet->unit_time || WAVSTUD_UNIT_TIME_1000MS < pWavInfoSet->unit_time) {
		pWavInfoSet->unit_time = WAVSTUD_UNIT_TIME_1000MS;
		DBG_WRN("Invalid Buffer unit time. Set to 1 sec\r\n");
	}
	unitSize = WavStud_GetUnitSize(pWavInfoSet->aud_info.aud_sr, pWavInfoSet->aud_info.ch_num, pWavInfoSet->aud_info.bitpersample, pWavInfoSet->aud_info.buf_sample_cnt);
	tmp_size = WavStud_GetTmpSize(pWavInfoSet->aud_info.aud_sr, pWavInfoSet->aud_info.ch_num, pWavInfoSet->aud_info.bitpersample);

	memSize = (WAVSTUD_HEADER_BUF_SIZE + \
				/*unitSize*WAVSTUD_TMP_DATA_BUF_CNT +\*/
				ALIGN_CEIL_64(sizeof(AUDIO_BUF_QUEUE)) * pWavInfoSet->buf_count + \
				ALIGN_CEIL_64(unitSize) * (pWavInfoSet->buf_count-1) + \
				ALIGN_CEIL_64(pWavInfoSet->aud_info.buf_sample_cnt) * pWavInfoSet->aud_info.ch_num * 2 + \
				tmp_size)*\
				count;

	#if WAVSTUD_FIX_BUF == ENABLE
	memSize += ALIGN_CEIL_64(sizeof(BOOL)) * pWavInfoSet->buf_count + \
				ALIGN_CEIL_64(sizeof(WAVSTUD_AUDQUE)) * pWavInfoSet->buf_count;
	#endif

	if (WAVSTUD_ACT_REC == actType && gWavStudObj.aec_en) {
		memSize += memSize / count;
	}

	DBG_IND("actType=%d, memNeed=0x%x\r\n", actType, memSize);
	return memSize;
}
static UINT32 WavStud_GetStatus(WAVSTUD_ACT actType)
{
	FLGPTN uiFlag = 0;
	UINT32 sts = WAVSTUD_STS_CLOSED;

	if (gWavStudObj.actObj[actType].opened) {
		if (WAVSTUD_ACT_PLAY == actType) {
			uiFlag = kchk_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_ALL);
		} else if (WAVSTUD_ACT_PLAY2 == actType) {
			uiFlag = kchk_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_ALL);
		} else if ((WAVSTUD_ACT_REC == actType) || (WAVSTUD_ACT_REC2 == actType) || (WAVSTUD_ACT_LB == actType)) {
			uiFlag = kchk_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_ALL);
		} else {
			DBG_ERR(":%d not support\r\n", actType);
		}
		if (uiFlag) {
			if (uiFlag & FLG_ID_WAVSTUD_STS_GOING) {
				sts = WAVSTUD_STS_GOING;
			} else if (uiFlag & FLG_ID_WAVSTUD_STS_PAUSED) {
				sts = WAVSTUD_STS_PAUSED;
			} else if (uiFlag & FLG_ID_WAVSTUD_STS_STOPPED) {
				sts = WAVSTUD_STS_STOPPED;
			} else {
				sts = WAVSTUD_STS_SAVING;
			}
		} else {
			DBG_ERR("WavStudio Unknown Sts\r\n");
			sts = WAVSTUD_STS_UNKNONWN;
		}
	} else {
		//DBG_ERR("WavStudio Not Opened!!\r\n");
	}
	DBG_IND("[%d]=0x%x\r\n", actType, sts);
	return sts;
}

#if 1//defined(__UITRON)
static void WavStud_SetMem(PWAVSTUD_APPOBJ pWavObj, PWAVSTUD_INFO_SET pWavInfoSet, WAVSTUD_ACT actType)
{
	PWAVSTUD_ACT_OBJ pActObj;
	UINT32 i = 0;//, j = 0;
	UINT32 tmpSize = 0;
	UINT32 unitSize = 0;
	UINT32 tmpDataAddr;
	UINT32 sAddr = pWavObj->mem.uiAddr;
	//UINT32 maxVolLvl = 0;
	UINT32 count = pWavInfoSet->obj_count;
	UINT32 tmp_buf_size;

	sAddr = ALIGN_CEIL_4(sAddr);
	//gWavStudObj.mem.uiAddr = sAddr;
	//gWavStudObj.mem.uiSize = pWavObj->mem.uiSize;

	//Set memory

	unitSize = WavStud_GetUnitSize(pWavInfoSet->aud_info.aud_sr, pWavInfoSet->aud_info.ch_num, pWavInfoSet->aud_info.bitpersample, pWavInfoSet->aud_info.buf_sample_cnt);
	tmp_buf_size = WavStud_GetTmpSize(pWavInfoSet->aud_info.aud_sr, pWavInfoSet->aud_info.ch_num, pWavInfoSet->aud_info.bitpersample);

	tmpSize = WAVSTUD_HEADER_BUF_SIZE + \
			  /*unitSize*WAVSTUD_TMP_DATA_BUF_CNT +\*/
			  ALIGN_CEIL_64(sizeof(AUDIO_BUF_QUEUE)) * pWavInfoSet->buf_count + \
			  ALIGN_CEIL_64(unitSize) * (pWavInfoSet->buf_count-1) + \
			  ALIGN_CEIL_64(pWavInfoSet->aud_info.buf_sample_cnt)* pWavInfoSet->aud_info.ch_num * 2 + \
			  tmp_buf_size;

	#if WAVSTUD_FIX_BUF == ENABLE
	tmpSize += ALIGN_CEIL_64(sizeof(BOOL)) * pWavInfoSet->buf_count + \
				ALIGN_CEIL_64(sizeof(WAVSTUD_AUDQUE)) * pWavInfoSet->buf_count;
	#endif

	for (i = 0; i < count; i++) {
		if (actType == WAVSTUD_ACT_PLAY) {
			pActObj = &(gWavStudObj.actObj[actType]);
			gWavStudObj.pWavStudCB[actType] = pWavObj->wavstud_cb;
		} else if (actType == WAVSTUD_ACT_PLAY2) {
			pActObj = &(gWavStudObj.actObj[actType]);
			gWavStudObj.pWavStudCB[actType] = pWavObj->wavstud_cb;
		} else {
			pActObj = &(gWavStudObj.actObj[actType + i]);
			gWavStudObj.pWavStudCB[actType + i] = pWavObj->wavstud_cb;
		}
		//pActObj->procUnitSize = WAVSTUD_PROCSIZE;
		pActObj->mem.uiAddr = sAddr;
		pActObj->mem.uiSize = tmpSize;
		pActObj->audBufQueCnt = pWavInfoSet->buf_count;
		tmpDataAddr = sAddr + WAVSTUD_HEADER_BUF_SIZE;

		//pig pong buffer for encode or decode data
		/*for (j = 0; j < WAVSTUD_TMP_DATA_BUF_CNT; j++)
		{
		    pActObj->memTmpData[j].uiAddr = tmpDataAddr;
		    pActObj->memTmpData[j].uiSize = unitSize;
		    tmpDataAddr += unitSize;
		}*/

		//set pointer pAudBufQueFirst
		pActObj->pAudBufQueFirst = (PAUDIO_BUF_QUEUE)tmpDataAddr;

		//move to the next audBufQue
		sAddr += tmpSize;

		//Set default audio-setting for those non-set.
		pActObj->audInfo.aud_sr = pWavInfoSet->aud_info.aud_sr;
		pActObj->audInfo.bitpersample = pWavInfoSet->aud_info.bitpersample;
		pActObj->audInfo.aud_ch = pWavInfoSet->aud_info.aud_ch;
		pActObj->audChs = pActObj->audInfo.ch_num = pWavInfoSet->aud_info.ch_num;

		/*maxVolLvl = 100;
		if (pActObj->audVol > maxVolLvl) {
			DBG_WRN("Vol too large=%d, scale down to 100\r\n", pActObj->audVol);
			pActObj->audVol = 100;
		}*/

		if (actType == WAVSTUD_ACT_PLAY) {
			pActObj->actId = actType;
		} else {
			pActObj->actId = actType + i;
		}
		pActObj->unitSize = unitSize;

		if (WavStud_MakeAudBufQue(pActObj) == 0) {
			DBG_ERR("Make buffer queue error, Mem Not Enough: 0x%x, 0x%x\r\n", sAddr, (pWavObj->mem.uiAddr + pWavObj->mem.uiSize));
		}
	}

	if (WAVSTUD_ACT_REC == actType && gWavStudObj.aec_en) {
		pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_LB]);

		#if WAVSTUD_FIX_BUF == ENABLE
		unitSize = WavStud_GetUnitSize(pWavInfoSet->aud_info.aud_sr, (gWavStudLbCh == AUDIO_CH_STEREO) ? 2 : 1, pWavInfoSet->aud_info.bitpersample, pWavInfoSet->aud_info.buf_sample_cnt);
		#endif

		//Loopback channel does not need header buffer
		//tmpSize -= WAVSTUD_HEADER_BUF_SIZE;

		//pActObj->procUnitSize = WAVSTUD_PROCSIZE;
		pActObj->mem.uiAddr = sAddr;
		pActObj->mem.uiSize = tmpSize;
		pActObj->audBufQueCnt = pWavInfoSet->buf_count;
		tmpDataAddr = sAddr;

		//set pointer pAudBufQueFirst
		pActObj->pAudBufQueFirst = (PAUDIO_BUF_QUEUE)tmpDataAddr;

		//move to the next audBufQue
		sAddr += tmpSize;

		//Set default audio-setting for those non-set.
		pActObj->audInfo.aud_sr = pWavInfoSet->aud_info.aud_sr;
		pActObj->audInfo.bitpersample = pWavInfoSet->aud_info.bitpersample;
		pActObj->audInfo.aud_ch = pWavInfoSet->aud_info.aud_ch;
		pActObj->audChs = pWavInfoSet->aud_info.ch_num;

		/*maxVolLvl = 100;
		if (pActObj->audVol > maxVolLvl) {
			DBG_WRN("Vol too large=%d, scale down to 100\r\n", pActObj->audVol);
			pActObj->audVol = 100;
		}*/
		pActObj->actId = WAVSTUD_ACT_LB;
		pActObj->unitSize = unitSize;

		if (WavStud_MakeAudBufQue(pActObj) == 0) {
			DBG_ERR("Make buffer queue error, Mem Not Enough: 0x%x, 0x%x\r\n", sAddr, (pWavObj->mem.uiAddr + pWavObj->mem.uiSize));
		}
	}

	if (sAddr > (pWavObj->mem.uiAddr + pWavObj->mem.uiSize)) {
		DBG_ERR("Mem Not Enough: 0x%x, 0x%x\r\n", sAddr, (pWavObj->mem.uiAddr + pWavObj->mem.uiSize));
	}
	#if (7 <= THIS_DBGLVL)
	WavStud_DmpModuleInfo();
	#endif
}

static BOOL WavStud_CheckInfo(PWAVSTUD_AUD_INFO info)
{
	DBG_IND("CH=%d,SR=%d,bitPerSamp:%d\r\n", info->aud_ch, info->aud_sr, info->bitpersample);
	if ((info->aud_sr > AUDIO_SR_48000) || (info->aud_sr < AUDIO_SR_8000)) {
		DBG_ERR("Invalid Sample rate=%d\r\n", info->aud_sr);
		return FALSE;
	}
	if (info->aud_ch > AUDIO_CH_DUAL_MONO) {
		DBG_ERR("Invalid Channel=%d\r\n", info->aud_ch);
		return FALSE;
	}
	if ((info->bitpersample != WAVSTUD_BITS_PER_SAM_8) && (info->bitpersample != WAVSTUD_BITS_PER_SAM_16)) {
		DBG_ERR("Invalid bit per sample=%d\r\n", info->bitpersample);
		return FALSE;
	}
	if ((((info->ch_num % 2) != 0) && info->ch_num != 1) || info->ch_num == 0) {
		DBG_ERR("Invalid Channel number=%d\r\n", info->ch_num);
		return FALSE;
	}
	return TRUE;
}
#endif
//======================== Export API ==============================

UINT32 wavstudio_get_config(WAVSTUD_CFG cfg, UINT32 p1, UINT32 p2)
{
	UINT32 retV = 0;
	switch (cfg) {
	case WAVSTUD_CFG_MODE:
		if (WAVSTUD_ACT_MAX > p1) {
			retV = (UINT32)gWavStudObj.actObj[p1].mode;
		}
		break;
	case WAVSTUD_CFG_MEM:
		if (0 != p1) {
			retV = WavStud_GetNeedMemSize((PWAVSTUD_INFO_SET)p1, p2);
		} else {
			retV = 0;
			DBG_ERR("Cfg[%d] p1=%d is NULL, v2=%d\r\n", cfg, p1, p2);
		}
		break;
	case WAVSTUD_CFG_VOL:
		if (WAVSTUD_ACT_MAX > p1) {
			retV = gWavStudObj.actObj[p1].audVol;
		} else {
			retV = 0;
			DBG_ERR("Cfg[%d] p1=%d >= WAVSTUD_ACT_MAX=%d, v2=%d\r\n", cfg, p1, WAVSTUD_ACT_MAX, p2);
		}
		break;
	case WAVSTUD_CFG_AUD_SR:
		if (WAVSTUD_ACT_MAX > p1) {
			retV = gWavStudObj.actObj[p1].audInfo.aud_sr;
		} else {
			retV = 0;
			DBG_ERR("Cfg[%d] p1=%d >= WAVSTUD_ACT_MAX=%d, v2=%d\r\n", cfg, p1, WAVSTUD_ACT_MAX, p2);
		}
		break;
	case WAVSTUD_CFG_AUD_CH:
		if (WAVSTUD_ACT_MAX > p1) {
			retV = gWavStudObj.actObj[p1].audInfo.aud_ch;
		} else {
			retV = 0;
			DBG_ERR("Cfg[%d] p1=%d >= WAVSTUD_ACT_MAX=%d, v2=%d\r\n", cfg, p1, WAVSTUD_ACT_MAX, p2);
		}
		break;
	case WAVSTUD_CFG_AUD_BIT_PER_SEC:
		if (WAVSTUD_ACT_MAX > p1) {
			retV = gWavStudObj.actObj[p1].audInfo.bitpersample;
		} else {
			retV = 0;
			DBG_ERR("Cfg[%d] p1=%d >= WAVSTUD_ACT_MAX=%d, v2=%d\r\n", cfg, p1, WAVSTUD_ACT_MAX, p2);
		}
		break;

	case WAVSTUD_CFG_PLAY_OUT_DEV:
		retV = (UINT32)gWavStudObj.playOutDev;
		break;
	case WAVSTUD_CFG_GET_STATUS:
		retV = WavStud_GetStatus((WAVSTUD_ACT)p1);
		break;
	case WAVSTUD_CFG_HDR_ADDR:
		if (gWavStudObj.actObj[p1].opened) {
			retV = gWavStudObj.actObj[p1].mem.uiAddr;
		} else {
			DBG_ERR("Not opened\r\n");
			retV = 0;
		}
		break;
	case WAVSTUD_CFG_ALLOC_MEM:
		if (gWavStudObj.actObj[p1].opened) {
			retV = gWavStudObj.actObj[p1].mem.uiAddr;
		} else {
			DBG_ERR("Not opened\r\n");
			retV = 0;
		}
		break;
	case WAVSTUD_CFG_QUEUE_ENTRY_NUM:
		if (((WAVSTUD_ACT_PLAY == p1) || (WAVSTUD_ACT_PLAY2 == p1)) && (gWavStudObj.actObj[p1].dataMode != WAVSTUD_DATA_MODE_PULL)) {
			retV = gWavStudObj.actObj[p1].queEntryNum;
		} else {
			retV = 0;
			DBG_ERR("Cfg[%d] only supported in passive play mode p1=%d, p2=%d\r\n", cfg, p1, p2);
		}
		break;

	case WAVSTUD_CFG_BUFF_UNIT_SIZE:
		if (WAVSTUD_ACT_MAX > p1) {
			retV = gWavStudObj.actObj[p1].unitSize;
		} else {
			retV = 0;
			DBG_ERR("Cfg[%d] p1=%d >= WAVSTUD_ACT_MAX=%d, p2=%d\r\n", cfg, p1, WAVSTUD_ACT_MAX, p2);
		}
		break;
	case WAVSTUD_CFG_AUD_CODEC:
		retV = gWavStudObj.codec;
		break;
	case WAVSTUD_CFG_AEC_EN: {
			{
				retV = gWavStudObj.aec_en;
			}
			break;
		}
	case WAVSTUD_CFG_DEFAULT_SETTING:
		retV = gWavStudObj.default_set;
		break;

	case WAVSTUD_CFG_EMPTY_ENTRY_NUM:
		if (((WAVSTUD_ACT_PLAY == p1) || (WAVSTUD_ACT_PLAY2 == p1)) && (gWavStudObj.actObj[p1].dataMode != WAVSTUD_DATA_MODE_PULL)) {
			retV = gWavStudObj.actObj[p1].tmp_num;
		} else {
			retV = 0;
			DBG_ERR("Cfg[%d] only supported in passive play mode p1=%d, p2=%d\r\n", cfg, p1, p2);
		}
		break;

	default:
		DBG_ERR("Not supported cfg=%d, 0x%x, 0x%x", cfg, p1, p2);
		break;
	}
	DBG_IND("cfg=%d, p1=0x%x, 0x%x, retV=0x%x --\r\n", cfg, p1, p2, retV);
	return retV;
}
UINT32 wavstudio_set_config(WAVSTUD_CFG cfg, UINT32 p1, UINT32 p2)
{
	UINT32 retV = E_OK;

	DBG_IND("cfg=%d, p1=0x%x, p2=0x%x++\r\n", cfg, p1, p2);
	switch (cfg) {
	case WAVSTUD_CFG_VOL:
		if (WAVSTUD_ACT_MAX > p1) {
			UINT32 maxVolLvl = 160;
			UINT32 vol = p2 & 0xFFFF;
			FLGPTN status = 0;

			gWavStudObj.actObj[p1].audVol = vol;
			gWavStudObj.actObj[p1].global_vol = TRUE;

			if (p1 == WAVSTUD_ACT_PLAY) {
				status = kchk_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS);
			} else if (p1 == WAVSTUD_ACT_PLAY2) {
				status = kchk_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_STS);
			} else {
				status = kchk_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS);
			}
			if (status & (FLG_ID_WAVSTUD_STS_GOING | FLG_ID_WAVSTUD_STS_PAUSED)) {
				if (vol > maxVolLvl) {
					DBG_WRN("Vol too large=%d, scale down to 100\r\n", vol);
					vol = 100;
				}
				retV = WavStud_AudDrvSetVol(p1, WAVSTUD_PORT_GLOBAL, vol);
			}
		} else {
			retV = E_PAR;
			DBG_ERR("Cfg[%d] p1=%d >= WAVSTUD_ACT_MAX=%d, p2=%d\r\n", cfg, p1, WAVSTUD_ACT_MAX, p2);
		}
		break;
	case WAVSTUD_CFG_PLAY_OUT_DEV:
		gWavStudObj.playOutDev = p1;
		break;
	case WAVSTUD_CFG_RECV_CB: {
			gWavStudObj.pWavStudRecvCB = (WAVSTUD_CB)p1;
			break;
		}
	case WAVSTUD_CFG_PROC_UNIT_SIZE: {
			if (gWavStudObj.actObj[p1].opened) {
				gWavStudObj.actObj[p1].procUnitSize = p2;
			} else {
				DBG_ERR("Not opened\r\n");
				retV = 0;
			}
			break;
		}
	case WAVSTUD_CFG_AEC_EN: {
			{
				gWavStudObj.aec_en= (BOOL)p1;
			}
			break;
		}
	case WAVSTUD_CFG_UNLOCK_ADDR:
		if (p1 != WAVSTUD_ACT_REC) {
			DBG_ERR("Unsupported act type=%d\r\n", p1);
		} else {
			if (gWavStudObj.actObj[p1].opened) {
				#if WAVSTUD_FIX_BUF == ENABLE
				WavStud_UnlockQ(WAVSTUD_ACT_REC, p2);
				if (gWavStudObj.aec_en) {
					UINT32 idx;
					PAUDIO_BUF_QUEUE p_que = 0;
					idx = WavStud_AddrToIdx(WAVSTUD_ACT_REC, p2);
					p_que = WavStud_IdxToBuf(WAVSTUD_ACT_LB, idx);
					WavStud_UnlockQ(WAVSTUD_ACT_LB, p_que->uiAddress);
				}

				gWavStudObj.actObj[p1].lock_addr = p2;
				#else
				gWavStudObj.actObj[p1].lock_addr = p2;
				#endif
			}
		}
		break;
	case WAVSTUD_CFG_PLAY_LB_CHANNEL: {
			if (p1 > AUDIO_CH_STEREO) {
				DBG_ERR("Invalid channel=%d\r\n", p1);
			} else {
				gWavStudObj.lb_ch = p1;
				gWavStudLbCh = p1;
			}
			break;
		}
	case WAVSTUD_CFG_AUD_EVT_CB: {
			gWavStudObj.actObj[p1].pWavStudEvtCB = (WAVSTUD_CB)p2;
			break;
		}
	case WAVSTUD_CFG_AUD_CODEC: {
			if (p1 > 0) {
				gWavStudObj.codec = WAVSTUD_AUD_CODEC_EXTERNAL;
			} else {
				gWavStudObj.codec = WAVSTUD_AUD_CODEC_EMBEDDED;
			}
			break;
		}
	case WAVSTUD_CFG_FILT_CB: {
			gWavStudObj.actObj[p1].filt_cb = (WAVSTUD_FILTCB)p2;
			break;
		}
	case WAVSTUD_CFG_DEFAULT_SETTING: {
			gWavStudObj.default_set = p1;
			break;
		}
	case WAVSTUD_CFG_NG_THRESHOLD: {
			gWavStudObj.ng_thd = *(INT32*)p1;
			break;
		}
	case WAVSTUD_CFG_PLAY_WAIT_PUSH: {
			gWavStudObj.actObj[p1].wait_push = (BOOL)p2;
			break;
		}
	case WAVSTUD_CFG_ALC_EN: {
			gWavStudObj.alc_en= *(INT32*)p1;
			break;
		}
	case WAVSTUD_CFG_REC_SRC: {
			gWavStudObj.rec_src= *(UINT32*)p1;
			break;
		}
	case WAVSTUD_CFG_VOL_P0:
		if (WAVSTUD_ACT_MAX > p1) {
			UINT32 maxVolLvl = 160;
			UINT32 vol = p2 & 0xFFFF;
			FLGPTN status = 0;

			gWavStudObj.actObj[p1].p0_vol = vol;
			gWavStudObj.actObj[p1].global_vol = FALSE;

			if (p1 == WAVSTUD_ACT_PLAY) {
				status = kchk_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS);
			} else if (p1 == WAVSTUD_ACT_PLAY2) {
				status = kchk_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_STS);
			} else {
				status = kchk_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS);
			}
			if (status & (FLG_ID_WAVSTUD_STS_GOING | FLG_ID_WAVSTUD_STS_PAUSED)) {
				if (vol > maxVolLvl) {
					DBG_WRN("Vol too large=%d, scale down to 100\r\n", vol);
					vol = 100;
				}
				retV = WavStud_AudDrvSetVol(p1, WAVSTUD_PORT_0, vol);
			}
		} else {
			retV = E_PAR;
			DBG_ERR("Cfg[%d] p1=%d >= WAVSTUD_ACT_MAX=%d, p2=%d\r\n", cfg, p1, WAVSTUD_ACT_MAX, p2);
		}
		break;
	case WAVSTUD_CFG_VOL_P1:
		if (WAVSTUD_ACT_MAX > p1) {
			UINT32 maxVolLvl = 160;
			UINT32 vol = p2 & 0xFFFF;
			FLGPTN status = 0;

			gWavStudObj.actObj[p1].p1_vol = vol;
			gWavStudObj.actObj[p1].global_vol = FALSE;

			if (p1 == WAVSTUD_ACT_PLAY) {
				status = kchk_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS);
			} else if (p1 == WAVSTUD_ACT_PLAY2) {
				status = kchk_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_STS);
			} else {
				status = kchk_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS);
			}
			if (status & (FLG_ID_WAVSTUD_STS_GOING | FLG_ID_WAVSTUD_STS_PAUSED)) {
				if (vol > maxVolLvl) {
					DBG_WRN("Vol too large=%d, scale down to 100\r\n", vol);
					vol = 100;
				}
				retV = WavStud_AudDrvSetVol(p1, WAVSTUD_PORT_1, vol);
			}
		} else {
			retV = E_PAR;
			DBG_ERR("Cfg[%d] p1=%d >= WAVSTUD_ACT_MAX=%d, p2=%d\r\n", cfg, p1, WAVSTUD_ACT_MAX, p2);
		}
		break;
	case WAVSTUD_CFG_REC_GAIN_LEVEL: {
			gWavStudObj.rec_gain_level = *(UINT32*)p1;
			break;
		}
	case WAVSTUD_CFG_ALC_DECAY_TIME: {
			gWavStudObj.alc_decay_time = *(UINT32*)p1;
			break;
		}
	case WAVSTUD_CFG_ALC_ATTACK_TIME: {
			gWavStudObj.alc_attack_time = *(UINT32*)p1;
			break;
		}
	case WAVSTUD_CFG_ALC_MAXGAIN: {
			gWavStudObj.alc_max_gain = *(INT32*)p1;
			break;
		}
	case WAVSTUD_CFG_ALC_MINGAIN: {
			gWavStudObj.alc_min_gain = *(INT32*)p1;
			break;
		}
	case WAVSTUD_CFG_ALC_CFG_EN: {
			gWavStudObj.alc_cfg_en= *(INT32*)p1;
			break;
		}
	default:
		retV = E_PAR;
		DBG_ERR("Not supported cfg=%d, 0x%x, 0x%x", cfg, p1, p2);
		break;
	}
	DBG_IND(" cfg=%d, 0x%x, 0x%x, retV=0x%x\r\n", cfg, p1, p2, retV);
	return retV;
}

/**
    Open WAV studio task.

    Start WAV studio task.

    @param[in] pWavObj  Wav Studio application object
    @return Open status
        - @b E_SYS: Task is already opened
        - @b E_NOMEM: Memory size is not enough
        - @b E_OK: No error
*/


#if defined __UITRON || defined __ECOS
ER wavstudio_open(WAVSTUD_ACT act_type, PWAVSTUD_APPOBJ wav_obj, DX_HANDLE dxsnd_hdl, PWAVSTUD_INFO_SET wavinfo_set)
#else
ER wavstudio_open(WAVSTUD_ACT act_type, PWAVSTUD_APPOBJ wav_obj, UINT32 dxsnd_hdl, PWAVSTUD_INFO_SET wavinfo_set)
#endif
{
#if 1//defined(__UITRON)
	UINT32 memNeed = 0;
	UINT32 count = wavinfo_set->obj_count;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[act_type]);
	PWAVSTUD_ACT_OBJ pActObj2 = NULL;
	PWAVSTUD_ACT_OBJ pActObjlb = NULL;

	if (act_type != WAVSTUD_ACT_REC && act_type != WAVSTUD_ACT_PLAY && act_type != WAVSTUD_ACT_PLAY2) {
		DBG_ERR(" Not supported act type\r\n");
		return FALSE;
	}

	DBG_IND("addr=0x%x, 0x%x, dxSndHdl=0x%x\r\n", wav_obj->mem.uiAddr, wav_obj->mem.uiSize, dxsnd_hdl);

	#if defined __UITRON || defined __ECOS
	if (FALSE == WavStudio_ChkModInstalled()) {
		DBG_ERR("module Not installed\r\n");
		return E_SYS;
	}
	#endif

	//1.Check module is opened or not.
	if (pActObj->opened) {
		DBG_ERR("Already Opened\r\n");
		return E_SYS;
	}

	if (wavinfo_set->buf_count == 0) {
		DBG_ERR("Record buffer count is zero\r\n");
		return E_SYS;
	}

	if (FALSE == WavStud_CheckInfo(&wavinfo_set->aud_info)) {
		DBG_ERR("Invalid audio info\r\n");
		return E_SYS;
	}

	if (act_type == WAVSTUD_ACT_PLAY || act_type == WAVSTUD_ACT_PLAY2) {
		if (count != 1) {
			DBG_ERR("Invalid act play object count=%d. Should be 1\r\n", count);
			return E_SYS;
		}
		pActObj->mode = WAVSTUD_MODE_1P;
		pActObj->dataMode = wavinfo_set->data_mode;
	} else {
		if (wavinfo_set->data_mode == WAVSTUD_DATA_MODE_PULL) {
			DBG_ERR("Record does not support pull mode\r\n");
			return E_SYS;
		}
		if (count == 1) {
			pActObj->mode = WAVSTUD_MODE_1R;
			pActObj->dataMode = wavinfo_set->data_mode;
		} else if (count == 2) {
			pActObj2 = &(gWavStudObj.actObj[act_type + 1]);
			pActObj->mode = WAVSTUD_MODE_2R;
			pActObj2->mode = WAVSTUD_MODE_2R;
			pActObj->dataMode = wavinfo_set->data_mode;
			pActObj2->dataMode = wavinfo_set->data_mode;
		} else {
			DBG_ERR("Invalid act rec object count=%d. Should be 1 or 2\r\n", count);
			return E_SYS;
		}
	}

	if (WAVSTUD_UNIT_TIME_0 == wavinfo_set->unit_time || WAVSTUD_UNIT_TIME_1000MS < wavinfo_set->unit_time) {
		pActObj->unitTime = WAVSTUD_UNIT_TIME_1000MS;
		if (pActObj2) {
			pActObj2->unitTime = WAVSTUD_UNIT_TIME_1000MS;
		}
		wavinfo_set->unit_time = WAVSTUD_UNIT_TIME_1000MS;
		DBG_WRN("Invalid Buffer unit time. Set to 1 sec\r\n");
	} else {
		pActObj->unitTime = wavinfo_set->unit_time;
		if (pActObj2) {
			pActObj2->unitTime = wavinfo_set->unit_time;
		}
	}

	if (WAVSTUD_ACT_REC == act_type && gWavStudObj.aec_en) {
		pActObjlb = &(gWavStudObj.actObj[WAVSTUD_ACT_LB]);
		pActObjlb->unitTime = wavinfo_set->unit_time;
	}

	//2. Check memory size
	memNeed = WavStud_GetNeedMemSize(wavinfo_set, act_type);
	if (wav_obj->mem.uiSize < memNeed || memNeed == 0) {
		DBG_ERR("WAV: Memory size must >= %d, but %d\r\n", memNeed, wav_obj->mem.uiSize);
		return E_NOMEM;
	} else {
		wav_obj->mem.uiSize = memNeed;
	}

	#if defined __UITRON || defined __ECOS
	//3. Keep DxSound Handler
	if (gWavStudDxSndHdl == NULL) {
		gWavStudDxSndHdl = dxsnd_hdl;
	}
	#endif


	//4. Setup memory
#if defined (__KERNEL__)
	if (!(kdrv_builtin_is_fastboot() && wavstudio_first_boot && act_type == WAVSTUD_ACT_REC)) {
		WavStud_SetMem(wav_obj, wavinfo_set, act_type);
	} else {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);
		builtin_init = param[0];
		if (builtin_init) {
			if (gWavStudObj.aec_en) {
				UINT32 unitSize = WavStud_GetUnitSize(wavinfo_set->aud_info.aud_sr,
					wavinfo_set->aud_info.ch_num, wavinfo_set->aud_info.bitpersample, wavinfo_set->aud_info.buf_sample_cnt);
				UINT32 tmpSize = WAVSTUD_HEADER_BUF_SIZE + \
					/*unitSize*WAVSTUD_TMP_DATA_BUF_CNT +\*/
					ALIGN_CEIL_64(sizeof(AUDIO_BUF_QUEUE)) * wavinfo_set->buf_count + \
					ALIGN_CEIL_64(unitSize) * wavinfo_set->buf_count + \
					ALIGN_CEIL_64(wavinfo_set->aud_info.buf_sample_cnt)* 2 * 2;
				#if WAVSTUD_FIX_BUF == ENABLE
				tmpSize += ALIGN_CEIL_64(sizeof(BOOL)) * wavinfo_set->buf_count + \
							ALIGN_CEIL_64(sizeof(WAVSTUD_AUDQUE)) * wavinfo_set->buf_count;
				#endif
				pActObj->actId = WAVSTUD_ACT_REC;
				pActObj->mem.uiAddr = wav_obj->mem.uiAddr;
				pActObj->mem.uiSize = tmpSize;
				pActObj->audBufQueCnt = wavinfo_set->buf_count;
				pActObjlb->actId = WAVSTUD_ACT_LB;
				pActObjlb->mem.uiAddr = wav_obj->mem.uiAddr + tmpSize;
				pActObjlb->mem.uiSize = tmpSize;
				pActObjlb->audBufQueCnt = wavinfo_set->buf_count;
			} else {
				pActObj->actId = WAVSTUD_ACT_REC;
				pActObj->mem.uiAddr = wav_obj->mem.uiAddr;
				pActObj->mem.uiSize = wav_obj->mem.uiSize;
				pActObj->audBufQueCnt = wavinfo_set->buf_count;
			}
		} else {
			WavStud_SetMem(wav_obj, wavinfo_set, act_type);
			wavstudio_first_boot = FALSE;
		}
	}
#else
	WavStud_SetMem(wav_obj, wavinfo_set, act_type);
#endif

	// Clr, Set flag
	if (act_type == WAVSTUD_ACT_PLAY) {
		clr_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_ALL);
		set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_STOPPED);
		THREAD_CREATE(WAVSTUD_PLAYTSK_ID, WavStudio_PlayTsk, NULL, "wav_play_tsk");
		if (WAVSTUD_PLAYTSK_ID == 0) {
			DBG_ERR("WAVSTUD_PLAYTSK_ID = %d\r\n", (unsigned int)WAVSTUD_PLAYTSK_ID);
			return E_SYS;
		}
		THREAD_SET_PRIORITY(WAVSTUD_PLAYTSK_ID, PRI_WAVSTUD_PLAYTSK);
		THREAD_RESUME(WAVSTUD_PLAYTSK_ID);
	} else if (act_type == WAVSTUD_ACT_PLAY2) {
		clr_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_ALL);
		set_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_STS_STOPPED);
		THREAD_CREATE(WAVSTUD_PLAYTSK2_ID, WavStudio_PlayTsk2, NULL, "wav_play2_tsk");
		if (WAVSTUD_PLAYTSK2_ID == 0) {
			DBG_ERR("WAVSTUD_PLAYTSK2_ID = %d\r\n", (unsigned int)WAVSTUD_PLAYTSK2_ID);
			return E_SYS;
		}
		THREAD_SET_PRIORITY(WAVSTUD_PLAYTSK2_ID, PRI_WAVSTUD_PLAYTSK);
		THREAD_RESUME(WAVSTUD_PLAYTSK2_ID);
	} else {
		clr_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_ALL);
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_STOPPED);
		clr_flg(FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_ALL);
		clr_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_ALL);
		set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_STS_STOPPED);

		THREAD_CREATE(WAVSTUD_RECORDTSK_ID, WavStudio_RecordTsk, NULL, "wav_rec_tsk");
		if (WAVSTUD_RECORDTSK_ID == 0) {
			DBG_ERR("WAVSTUD_RECORDTSK_ID = %d\r\n", (unsigned int)WAVSTUD_RECORDTSK_ID);
			return E_SYS;
		}
		THREAD_SET_PRIORITY(WAVSTUD_RECORDTSK_ID, PRI_WAVSTUD_RECORDTSK);
		THREAD_RESUME(WAVSTUD_RECORDTSK_ID);
		//THREAD_CREATE(WAVSTUD_RECORDWRITETSK_ID, WavStudio_RecordWriteTsk, NULL, "wavstudio_record_write_tsk");
		//THREAD_RESUME(WAVSTUD_RECORDWRITETSK_ID);
		THREAD_CREATE(WAVSTUD_RECORDUPDATETSK_ID, WavStudio_RecordUpdateTsk, NULL, "wav_rec_update_tsk");
		if (WAVSTUD_RECORDUPDATETSK_ID == 0) {
			DBG_ERR("WAVSTUD_RECORDUPDATETSK_ID = %d\r\n", (unsigned int)WAVSTUD_RECORDUPDATETSK_ID);
			return E_SYS;
		}
		THREAD_SET_PRIORITY(WAVSTUD_RECORDUPDATETSK_ID, PRI_WAVSTUD_RECORDUPDATETSK);
		THREAD_RESUME(WAVSTUD_RECORDUPDATETSK_ID);
	}

	pActObj->opened = TRUE;
	if (pActObj2) { //2 Rec
		pActObj2->opened = TRUE;
	}

	if (WAVSTUD_ACT_REC == act_type && gWavStudObj.aec_en) {
		pActObjlb->opened = TRUE;
	}

	/*if (act_type == WAVSTUD_ACT_REC && gWavStudObj.paecObj) {
		if (gWavStudObj.paecObj->open) {
			gWavStudObj.paecObj->open();
		}
	}*/

	#if defined __UITRON || defined __ECOS
	xWavStudio_InstallCmd();
	#endif

#if (7 <= THIS_DBGLVL)
	WavStud_DmpModuleInfo();
#endif

#else

#endif
	return E_OK;
}

/**
    Close WAV studio task.

    Close WAV studio task.

    @return Close status
        - @b E_SYS: Task is already closed
        - @b E_OK: No error
*/
ER wavstudio_close(WAVSTUD_ACT act_type)
{
#if 1//defined(__UITRON)
	FLGPTN uiFlag;

	if (act_type != WAVSTUD_ACT_REC && act_type != WAVSTUD_ACT_PLAY && act_type != WAVSTUD_ACT_PLAY2) {
		DBG_ERR(" Not supported act type\r\n");
		return FALSE;
	}

	if (FALSE == gWavStudObj.actObj[act_type].opened) {
		DBG_ERR(" Not Opened\r\n");
		return E_SYS;
	}

	if (act_type == WAVSTUD_ACT_PLAY) {
		if (kchk_flg(FLG_ID_WAVSTUD_PLAY, (FLG_ID_WAVSTUD_STS_GOING | FLG_ID_WAVSTUD_STS_PAUSED))) {
			DBG_IND("PlaySTS\r\n");
			set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_CMD_STOP);
		}
	} else if (act_type == WAVSTUD_ACT_PLAY2) {
		if (kchk_flg(FLG_ID_WAVSTUD_PLAY2, (FLG_ID_WAVSTUD_STS_GOING | FLG_ID_WAVSTUD_STS_PAUSED))) {
			DBG_IND("PlaySTS\r\n");
			set_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_CMD_STOP);
		}
	} else {
		if (kchk_flg(FLG_ID_WAVSTUD_RECORD, (FLG_ID_WAVSTUD_STS_GOING | FLG_ID_WAVSTUD_STS_PAUSED))) {
			DBG_IND("RecordSTS\r\n");
			set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_CMD_STOP);
		}
	}


	//Wait for stopped and trigger task to do ext_tsk()
	if (act_type == WAVSTUD_ACT_PLAY) {
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_STOPPED, TWF_CLR);
		set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_EVT_CLOSE);
	} else if (act_type == WAVSTUD_ACT_PLAY2) {
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_STS_STOPPED, TWF_CLR);
		set_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_EVT_CLOSE);
	} else {
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_STOPPED, TWF_CLR);
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_EVT_CLOSE);

		//wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_EVT_RWIDLE, TWF_CLR);
		//set_flg(FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_EVT_CLOSE);

		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_STS_STOPPED, TWF_CLR);
		set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_CLOSE);
	}

	//Wait for ext_tsk()
	if (act_type == WAVSTUD_ACT_PLAY) {
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_IDLE, TWF_CLR);
		clr_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_ALL);
	} else if (act_type == WAVSTUD_ACT_PLAY2) {
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_STS_IDLE, TWF_CLR);
		clr_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_ALL);
	} else {
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_IDLE, TWF_CLR);
		clr_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_ALL);

		//wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_STS_IDLE, TWF_CLR);
		//clr_flg(FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_ALL);

		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_STS_IDLE, TWF_CLR);
		clr_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_ALL);
	}

	#if 0
	if (act_type == WAVSTUD_ACT_PLAY) {
		THREAD_DESTROY(WAVSTUD_PLAYTSK_ID);
	} else if (act_type == WAVSTUD_ACT_PLAY2) {
		THREAD_DESTROY(WAVSTUD_PLAYTSK2_ID);
	} else {
		THREAD_DESTROY(WAVSTUD_RECORDTSK_ID);
		//THREAD_DESTROY(WAVSTUD_RECORDWRITETSK_ID);
		THREAD_DESTROY(WAVSTUD_RECORDUPDATETSK_ID);
	}
	#endif

	gWavStudObj.actObj[act_type].opened = FALSE;
	if (gWavStudObj.actObj[act_type].mode == WAVSTUD_MODE_2R && act_type == WAVSTUD_ACT_REC) {
		gWavStudObj.actObj[act_type + 1].opened = FALSE;
	}

	/*if (act_type == WAVSTUD_ACT_REC && gWavStudObj.paecObj) {
		if (gWavStudObj.paecObj->close) {
			gWavStudObj.paecObj->close();
		}
	}*/
#else

#endif

	return E_OK;
}

/**
    Start to record or play

    Start to record or play.

    @return
        - @b TRUE: Start to record
        - @b FALSE: WAV studio task is not in stopped status
*/
BOOL wavstudio_start(WAVSTUD_ACT act_type, PWAVSTUD_AUD_INFO info, UINT64 stopcount, BOOL (*data_cb)(PAUDIO_BUF_QUEUE, PAUDIO_BUF_QUEUE, UINT32, UINT64, UINT64))
{
#if 1//defined(__UITRON)
	ID flgid;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[act_type]);

	if (WAVSTUD_ACT_PLAY == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY;
	} else if (WAVSTUD_ACT_PLAY2 == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY2;
	} else {
		flgid = FLG_ID_WAVSTUD_RECORD;
	}

	if (act_type != WAVSTUD_ACT_REC && act_type != WAVSTUD_ACT_PLAY && act_type != WAVSTUD_ACT_PLAY2) {
		DBG_ERR(" Not supported act type\r\n");
		return FALSE;
	}

	if (FALSE == pActObj->opened) {
		DBG_ERR(" Not Opened\r\n");
		return FALSE;
	}

	if (0 == pActObj->mem.uiAddr) {
		DBG_ERR("Act Object is NULL\r\n");
		return FALSE;
	}

	if (FALSE == WavStud_CheckInfo(info)) {
		DBG_ERR(":Invalid audio info\r\n");
		return FALSE;
	}

	if (kchk_flg(flgid, FLG_ID_WAVSTUD_STS_STOPPED)) {
		pActObj->audInfo.aud_ch = info->aud_ch;
		pActObj->audInfo.aud_sr = info->aud_sr;
		pActObj->audInfo.bitpersample = info->bitpersample;
		pActObj->audChs = pActObj->audInfo.ch_num = info->ch_num;
		pActObj->currentCount = 0;
		pActObj->stopCount = stopcount;
		pActObj->fpDataCB = data_cb;
		pActObj->buf_sample_cnt = info->buf_sample_cnt;
		pActObj->done_size = 0;
		DBG_IND("actType=%d,CH=%d,SR=%d,bitPerSamp:%d\r\n", pActObj->actId, pActObj->audInfo.aud_ch, pActObj->audInfo.aud_sr, pActObj->audInfo.bitpersample);

		if (WAVSTUD_ACT_REC == act_type && gWavStudObj.aec_en) {
			pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_LB]);
			pActObj->audInfo.aud_ch = gWavStudLbCh;
			pActObj->audInfo.aud_sr = info->aud_sr;
			pActObj->audInfo.bitpersample = info->bitpersample;
			pActObj->audChs = pActObj->audInfo.ch_num = (pActObj->audInfo.aud_ch == AUDIO_CH_STEREO)? 2 : 1;
			pActObj->buf_sample_cnt = info->buf_sample_cnt;
		}

		if (pActObj->mode == WAVSTUD_MODE_2R) {

			PWAVSTUD_ACT_OBJ pActObj2 = &(gWavStudObj.actObj[WAVSTUD_ACT_REC2]);
			pActObj2->audInfo.aud_ch = info->aud_ch;
			pActObj2->audInfo.aud_sr = info->aud_sr;
			pActObj2->audInfo.bitpersample = info->bitpersample;
			pActObj2->audChs = pActObj2->audInfo.ch_num;
			pActObj2->currentCount = 0;
			pActObj2->stopCount = stopcount;
			pActObj2->fpDataCB = data_cb;
			pActObj->buf_sample_cnt = info->buf_sample_cnt;

			if (AUDIO_CH_DUAL_MONO != gWavStudObj.actObj[WAVSTUD_ACT_REC].audInfo.aud_ch) {
				DBG_ERR("DualRec:Set Ch to AUDIO_CH_DUAL_MONO, Ori=%d\r\n", gWavStudObj.actObj[WAVSTUD_ACT_REC].audInfo.aud_ch);
				gWavStudObj.actObj[WAVSTUD_ACT_REC].audInfo.aud_ch = AUDIO_CH_DUAL_MONO;
				gWavStudObj.actObj[WAVSTUD_ACT_REC].audChs = 1;
			}
			if (AUDIO_CH_DUAL_MONO != gWavStudObj.actObj[WAVSTUD_ACT_REC2].audInfo.aud_ch) {
				DBG_ERR("DualRec2:Set Ch to AUDIO_CH_DUAL_MONO, Ori=%d\r\n", gWavStudObj.actObj[WAVSTUD_ACT_REC2].audInfo.aud_ch);
				gWavStudObj.actObj[WAVSTUD_ACT_REC2].audInfo.aud_ch = AUDIO_CH_DUAL_MONO;
				gWavStudObj.actObj[WAVSTUD_ACT_REC2].audChs = 1;
			}
		}
		set_flg(flgid, FLG_ID_WAVSTUD_CMD_START);

		return TRUE;
	} else {
		DBG_ERR("Already Going:%d, %d\r\n", flgid, act_type);
		return FALSE;
	}
#else
	return TRUE;
#endif
}

/**
    Stop recording

    Stop recording

    @return
        - @b TRUE: Stop recording
        - @b FALSE: WAV studio task doesn't recording
*/
BOOL wavstudio_stop(WAVSTUD_ACT act_type)
{
#if 1//defined(__UITRON)
	ID flgid;

	if (act_type != WAVSTUD_ACT_REC && act_type != WAVSTUD_ACT_PLAY && act_type != WAVSTUD_ACT_PLAY2) {
		DBG_ERR(" Not supported act type\r\n");
		return FALSE;
	}

	if (WAVSTUD_ACT_PLAY == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY;
	} else if (WAVSTUD_ACT_PLAY2 == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY2;
	} else {
		flgid = FLG_ID_WAVSTUD_RECORD;
	}

	if (FALSE == gWavStudObj.actObj[act_type].opened) {
		DBG_ERR(" Not Opened\r\n");
		return FALSE;
	}
	if (kchk_flg(flgid, (FLG_ID_WAVSTUD_STS_GOING | FLG_ID_WAVSTUD_STS_PAUSED))) {
		set_flg(flgid, FLG_ID_WAVSTUD_CMD_STOP);
		return TRUE;
	} else {
		DBG_ERR("Not Any Going or Paused:%d\r\n", flgid);
		return FALSE;
	}
#else
	return TRUE;
#endif

}
/**
    Pause recording / playing.

    Pause recording / playing.

    @return
        -@ TRUE: Pause recording / playing
        -@ FALSE: Not recording / playing
*/
BOOL wavstudio_pause(WAVSTUD_ACT act_type)
{
#if 1//defined(__UITRON)
	ID flgid;

	if (act_type != WAVSTUD_ACT_REC && act_type != WAVSTUD_ACT_PLAY && act_type != WAVSTUD_ACT_PLAY2) {
		DBG_ERR(" Not supported act type\r\n");
		return FALSE;
	}

	if (WAVSTUD_ACT_PLAY == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY;
	} else if (WAVSTUD_ACT_PLAY2 == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY2;
	} else {
		flgid = FLG_ID_WAVSTUD_RECORD;
	}

	if (FALSE == gWavStudObj.actObj[act_type].opened) {
		DBG_ERR(" Not Opened\r\n");
		return FALSE;
	}
	if (kchk_flg(flgid, FLG_ID_WAVSTUD_STS_GOING)) {
		set_flg(flgid, FLG_ID_WAVSTUD_CMD_PAUSE);
		return TRUE;
	} else {
		DBG_ERR("Not Any Going=%d\r\n", flgid);
		return FALSE;
	}
#else
	return TRUE;
#endif

}
/**
    Resume recording / playing.

    Resume recording / playing from pause status.

    @return
        - @b TRUE: Resume from paused status
        - @b FALSE: Not in paused status
*/
BOOL wavstudio_resume(WAVSTUD_ACT act_type)
{
#if 1//defined(__UITRON)
	ID flgid;

	if (act_type != WAVSTUD_ACT_REC && act_type != WAVSTUD_ACT_PLAY && act_type != WAVSTUD_ACT_PLAY2) {
		DBG_ERR(" Not supported act type\r\n");
		return FALSE;
	}

	if (WAVSTUD_ACT_PLAY == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY;
	} else if (WAVSTUD_ACT_PLAY2 == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY2;
	} else {
		flgid = FLG_ID_WAVSTUD_RECORD;
	}

	if (FALSE == gWavStudObj.actObj[act_type].opened) {
		DBG_ERR(" Not Opened\r\n");
		return FALSE;
	}
	if (kchk_flg(flgid, FLG_ID_WAVSTUD_STS_PAUSED)) {
		set_flg(flgid, FLG_ID_WAVSTUD_CMD_RESUME);
		return TRUE;
	} else {
		DBG_ERR("Not Any Paused=0x%x\r\n", flgid);
		return FALSE;
	}
#else
	return TRUE;
#endif

}

void wavstudio_wait_start(WAVSTUD_ACT act_type)
{
#if 1//defined(__UITRON)
	ID flgid;
	FLGPTN uiFlag;

	if (WAVSTUD_ACT_PLAY == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY;
	} else if (WAVSTUD_ACT_PLAY2 == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY2;
	} else {
		flgid = FLG_ID_WAVSTUD_RECORD;
	}

	wai_flg(&uiFlag, flgid, FLG_ID_WAVSTUD_STS_GOING, TWF_ORW);
	return;
#else
	return;
#endif

}

void wavstudio_wait_stop(WAVSTUD_ACT act_type)
{
#if 1//defined(__UITRON)
	ID flgid;
	FLGPTN uiFlag;

	if (WAVSTUD_ACT_PLAY == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY;
	} else if (WAVSTUD_ACT_PLAY2 == act_type) {
		flgid = FLG_ID_WAVSTUD_PLAY2;
	} else {
		flgid = FLG_ID_WAVSTUD_RECORD;
	}

	wai_flg(&uiFlag, flgid, FLG_ID_WAVSTUD_STS_STOPPED, TWF_ORW);

	#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && wavstudio_first_boot && act_type == WAVSTUD_ACT_REC) {
		wavstudio_first_boot = FALSE;
	}
	#endif

	return;
#else
	return;
#endif

}

