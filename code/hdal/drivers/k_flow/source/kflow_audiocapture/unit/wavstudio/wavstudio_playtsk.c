/*
    @file       WavStudioTsk.c
    @ingroup    mILIBWAVSTUDIO

    @brief      Wav Studio Task.

                Task body of Wav Studio task.
                Record / Play WAV file.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

/**
    @addtogroup mILIBWAVSTUDIO
*/
//@{

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "wavstudio_int.h"
#include "wavstudio_aud_intf.h"
#include "wavstudio_id.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

extern WAVSTUD_OBJ gWavStudObj;
extern SEM_HANDLE SEM_ID_WAVSTUD_PLAY_QUE;

static BOOL playbacktsk_start = FALSE;

#if 0
static void WavStud_PlayEvtHdlr(UINT32 uiEventID)
{
	FLGPTN uiFlag = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_PLAY]);
	UINT32 tcTrigVal = pActObj->currentCount;
	UINT32 addCount = pActObj->unitSize / (pActObj->audInfo.bitpersample >> 3);

	addCount = (AUDIO_CH_STEREO == pActObj->audInfo.aud_ch) ? addCount / 2 : addCount;
	tcTrigVal += 2 * addCount;

	DBG_IND(":evt=0x%x,currentCount=%u,tc=%u,stopCount=%u\r\n", uiEventID, pActObj->currentCount, tcTrigVal, (UINT32)pActObj->stopCount);
	if (uiEventID & AUDIO_EVENT_TCHIT) {
		tcTrigVal = (tcTrigVal > pActObj->stopCount) ? pActObj->stopCount : tcTrigVal;
		WavStud_AudDrvSetTCTrigger(WAVSTUD_ACT_PLAY, tcTrigVal);
		uiFlag |= FLG_ID_WAVSTUD_EVT_TCHIT;
	}

	if (uiEventID & AUDIO_EVENT_DMADONE) {
		uiFlag |= FLG_ID_WAVSTUD_EVT_BUF_READY;
	}
	if (uiEventID & (AUDIO_EVENT_BUF_FULL | AUDIO_EVENT_DONEBUF_FULL)) {
		DBG_ERR("Aud Buf Full:0x%x\r\n", uiEventID);
		uiFlag |= (FLG_ID_WAVSTUD_EVT_BUF_FULL | FLG_ID_WAVSTUD_CMD_STOP);
	}
	if (uiEventID & AUDIO_EVENT_BUF_EMPTY) {
		DBG_IND("Aud Buf Empty:0x%x, %u, %u\r\n", uiEventID, (UINT32)pActObj->stopCount, pActObj->currentCount + addCount);
		if (pActObj->stopCount > (pActObj->currentCount + addCount) && (pActObj->stopCount != WAVSTUD_NON_STOP_COUNT)) {
			uiFlag |= (FLG_ID_WAVSTUD_EVT_BUF_EMPTY | FLG_ID_WAVSTUD_CMD_STOP);
		}
	}
	if (uiEventID & AUDIO_EVENT_BUF_RTEMPTY) {
		DBG_IND("Aud Buf Runtime Empty:0x%x\r\n", uiEventID);
	}
	if (uiFlag) {
		set_flg(FLG_ID_WAVSTUD_PLAY, uiFlag);
	}
}
#endif

INT32 wavstud_play_buf_cb(VOID *callback_info, VOID *user_data)
{
	FLGPTN uiFlag = 0;
	unsigned long flags;
	PAUDIO_BUF_QUEUE pAudBufQueDone = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_PLAY]);

	clr_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_ISR_IDLE);

	if (!playbacktsk_start) {
		set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_ISR_IDLE);
		return 0;
	}

	pAudBufQueDone = (AUDIO_BUF_QUEUE*)(*(UINT32 *)user_data);

	if (pAudBufQueDone) {
		if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
			loc_cpu(flags);
			if (pActObj->queEntryNum >= 1) {
				pActObj->queEntryNum--;
			}
			unl_cpu(flags);
			pActObj->done_size += pAudBufQueDone->uiSize;
		} else {
			loc_cpu(flags);
			if (pActObj->tmp_num >= 1) {
				pActObj->tmp_num--;
			}
			unl_cpu(flags);
		}
	}

	uiFlag |= FLG_ID_WAVSTUD_EVT_BUF_READY | FLG_ID_WAVSTUD_STS_ISR_IDLE;

	WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_BUF_DONE, pActObj->done_size);

	set_flg(FLG_ID_WAVSTUD_PLAY, uiFlag);

	return 0;
}

static WAVSTUD_CB_EVT WavStud_StartPlay(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_CB_EVT rv = WAVSTUD_CB_EVT_OK;
	//SEM_HANDLE SEM_ID_QUE = SEM_ID_WAVSTUD_PLAY_QUE;
	DBG_IND("++\r\n");


	if (WAVSTUD_BITS_PER_SAM_8 == pActObj->audInfo.bitpersample) {
		DBG_ERR("Not Support resolution=%d\r\n", pActObj->audInfo.bitpersample);
		return WAVSTUD_CB_EVT_FAIL;
	}

	pActObj->unitSize = WavStud_GetUnitSize(pActObj->audInfo.aud_sr, pActObj->audInfo.ch_num, pActObj->audInfo.bitpersample, pActObj->buf_sample_cnt);
	SEM_WAIT(SEM_ID_WAVSTUD_PLAY_QUE);
	pActObj->queEntryNum = 0;
	pActObj->remain_entry_num = 0;
	pActObj->tmp_num = 0;
	SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);

	playbacktsk_start = TRUE;
	set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_ISR_IDLE);

	WavStud_AudDrvPlay(WAVSTUD_ACT_PLAY);

	DBG_IND("--\r\n");
	return rv;
}
static WAVSTUD_CB_EVT WavStud_StopPlay(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_CB_EVT rv = WAVSTUD_CB_EVT_OK;
	FLGPTN uiFlag = 0;
	DBG_IND("++\r\n");

	WavStud_AudDrvStop(WAVSTUD_ACT_PLAY);

	playbacktsk_start = FALSE;

	wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_ISR_IDLE, TWF_ORW);

	clr_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_ALL);
	set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_STOPPED);
	DBG_IND("--\r\n");
	return rv;
}

#if 1
static void WavStud_TrigPlay(PWAVSTUD_ACT_OBJ pActObj)
{
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueFirst;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	WAVSTUD_ACT actType = pActObj->actId;
	PWAVSTUD_ACT_OBJ pActPlayObj = &(gWavStudObj.actObj[actType]);
	UINT32 i;
	//SEM_HANDLE SEM_ID_QUE = SEM_ID_WAVSTUD_PLAY_QUE;

	SEM_WAIT(SEM_ID_WAVSTUD_PLAY_QUE);

	pActPlayObj->pAudBufQueTail = pActObj->pAudBufQueFirst;
	pActPlayObj->pAudBufQueHead = pActObj->pAudBufQueFirst;
	pActObj->pAudBufQueNext     = pActObj->pAudBufQueFirst;

	pActObj->currentBufRemain = pActObj->unitSize;

	memset((void *)pActObj->pAudBufQueTmp->uiAddress, 0, pActObj->pAudBufQueTmp->uiSize);

	if (pActObj->wait_push == TRUE) {
		set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_WAIT_PRELOAD);
	}

	clr_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_STOPPED);
	set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_GOING);

	WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_STARTED, 0);

	//DBG_DUMP("Head=%x,Tail=%x++\r\n", pActObj->pAudBufQueHead->uiAddress, pActObj->pAudBufQueTail->uiAddress);

	DBG_IND("Unit Size=%d\r\n", pAudBufQue->uiSize);

	if (pActObj->wait_push == TRUE) {
		FLGPTN uiFlag = 0;
		UINT32 idx;
		unsigned long flags = 0;

		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);

		while (pActObj->queEntryNum < 3) {
			wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_EVT_QUE_READY | FLG_ID_WAVSTUD_CMD_STOP, TWF_ORW | TWF_CLR);
			if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
				break;
			}
		}

		clr_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_WAIT_PRELOAD);

		if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
			DBG_IND("Stop play before start!\r\n");
			set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_CMD_STOP);
			return;
		}

		SEM_WAIT(SEM_ID_WAVSTUD_PLAY_QUE);

		for (i = 0; i < 3; i++) {
			idx = WavStud_BufToIdx(pActObj->actId, pActObj->pAudBufQueNext);
			pAudBufQue = WavStud_IdxToBuf(pActObj->actId, idx);

			//Add buffer to queue
			if (WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_PLAY, audCh, pAudBufQue, 1)) {
				if (pAudBufQue->pNext != NULL) {
					pActObj->pAudBufQueNext = pAudBufQue->pNext;
					loc_cpu(flags);
					pActObj->remain_entry_num--;
					unl_cpu(flags);
				}
			}
		}

		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
	} else {
		pActPlayObj->tmp_num = 2;
		for (i = 0; i < 2; i++) {

			pAudBufQue = pActObj->pAudBufQueTmp;

			//pAudBufQue->uiSize = pActObj->unitSize;
			//memset((void *)pAudBufQue->uiAddress, 0, pAudBufQue->uiSize);

			//pActPlayObj->queEntryNum++;

			//DBG_IND("Preload 0 buffer\r\n");
			if (gWavStudObj.pWavStudRecvCB) {
				(gWavStudObj.pWavStudRecvCB)(WAVSTUD_ACT_PLAY, 0, 0);
			}

			//Add buffer to queue
			//WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_PLAY, audCh, pAudBufQue, 1);
			if (i == 1) {
				WavStud_AudDrvAddBuffQue(actType, audCh, pAudBufQue, 1);
			} else {
				WavStud_AudDrvAddBuffQue(actType, audCh, pAudBufQue, 0);
			}


			//pAudBufQue = pAudBufQue->pNext;

#if (THIS_DBGLVL >= 5)
			DBG_IND(":unitSize=0x%x,insertCnt=%d\r\n", pActObj->unitSize, pActObj->queEntryNum);
#endif

		}
		//pActObj->pAudBufQueNext = pAudBufQue;
		//pActObj->pAudBufQueHead = pAudBufQue;

		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
	}

	DBG_IND(":audCh=%d,unitSize=0x%x,SR=%d\r\n", audCh, pActObj->unitSize, pActObj->audInfo.aud_sr);
}
static WAVSTUD_CB_EVT WavStud_NextPlay(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_CB_EVT retV = WAVSTUD_CB_EVT_OK;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	PAUDIO_BUF_QUEUE pAudBufQue = 0;
	UINT32 addCount = 0;
	WAVSTUD_ACT actType = pActObj->actId;
	PWAVSTUD_ACT_OBJ pActPlayObj = &(gWavStudObj.actObj[actType]);
	PAUDIO_BUF_QUEUE pAudBufQueHead;
	UINT32 idx = 0;
	unsigned long flags;
	BOOL trigger = TRUE;
	//SEM_HANDLE SEM_ID_QUE = SEM_ID_WAVSTUD_PLAY_QUE;
	UINT32 add_que = 0;

	do {
		SEM_WAIT(SEM_ID_WAVSTUD_PLAY_QUE);

		idx = WavStud_BufToIdx(pActObj->actId, pActObj->pAudBufQueHead);
		pAudBufQueHead = WavStud_IdxToBuf(pActObj->actId, idx);

		idx = WavStud_BufToIdx(pActObj->actId, pActObj->pAudBufQueNext);
		pAudBufQue = WavStud_IdxToBuf(pActObj->actId, idx);

		pActPlayObj->pAudBufQueTail = pActPlayObj->pAudBufQueTail->pNext;

		//DBG_DUMP("Head=%x,Next=%x,Tail=%x++\r\n", pActObj->pAudBufQueHead->uiAddress, pActObj->pAudBufQueNext->uiAddress, pActObj->pAudBufQueTail->uiAddress);

		DBG_IND("pAudBufQue=%x,pAudBufQueHead=%x\r\n", pAudBufQue->uiAddress, pAudBufQueHead->uiAddress);

		//queue empty
		if (pAudBufQue == pAudBufQueHead && pActObj->remain_entry_num == 0) {
			DBG_IND("Queue empty\r\n");

			//no data, play 0
			DBG_IND("No data, play 0\r\n");
			if (pActObj->queEntryNum < 2) { //keep at least 2 buffer in driver
				if (pActObj->currentBufRemain != pActObj->unitSize && pActObj->currentBufRemain != 0) {
					DBG_IND("unit size=%d, remain size=%d\r\n", pActObj->unitSize, pActObj->currentBufRemain);
					memset((void *)(pAudBufQue->uiAddress + (pActObj->unitSize - pActObj->currentBufRemain)), 0, pActObj->currentBufRemain);
					if (pActObj->filt_cb) {
						pActObj->filt_cb(pAudBufQue->uiAddress, pAudBufQue->uiAddress, pActObj->unitSize);
					}
					pActObj->pAudBufQueHead = pAudBufQueHead->pNext;
					loc_cpu(flags);
					pActObj->currentBufRemain = pActObj->unitSize;
					pActObj->queEntryNum++;
					pActObj->remain_entry_num++;
					unl_cpu(flags);
				} else {
					if (pActObj->tmp_num >= 2) {
						pAudBufQue = NULL;
						trigger = FALSE;
					} else {
						pAudBufQue = pActObj->pAudBufQueTmp;
						loc_cpu(flags);
						pActObj->tmp_num++;
						unl_cpu(flags);
					}
				}
				if (pActObj->stopCount != WAVSTUD_NON_STOP_COUNT) {
					pActObj->stopCount += addCount/pActObj->unitTime;
				}

				if (gWavStudObj.pWavStudRecvCB) {
					(gWavStudObj.pWavStudRecvCB)(WAVSTUD_ACT_PLAY, 0, 0);
				}
				if (!((pAudBufQue == pActObj->pAudBufQueTmp) && (pActObj->tmp_num < 2))) {
					trigger = FALSE;
				}
			} else {
				pAudBufQue = NULL;
				trigger = FALSE;
			}
		}

		if (pAudBufQue) {
			pAudBufQue->uiValidSize = 0;

			//Add buffer to queue
			if (WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_PLAY, audCh, pAudBufQue, 1)) {
				if (pAudBufQue->pNext != NULL) {
					pActObj->pAudBufQueNext = pAudBufQue->pNext;
					loc_cpu(flags);
					pActObj->remain_entry_num--;
					unl_cpu(flags);

					if (pActObj->remain_entry_num == 0) {
						trigger = FALSE;
					}
				}
				if (trigger) {
					if (pActObj->pAudBufQueNext == pActObj->pAudBufQueHead && pActObj->queEntryNum != (pActObj->audBufQueCnt-1) && !((pAudBufQue == pActObj->pAudBufQueTmp) && (pActObj->tmp_num < 2))) {
						trigger = FALSE;
					}
				}

				add_que++;
			} else {
				trigger = FALSE;
			}
		}
		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
	} while (trigger);

	//if (add_que == 0) {
	if (1) {
		WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_PLAY, audCh, NULL, 1);
	}

	return retV;
}
#else
static void WavStud_TrigPlay(PWAVSTUD_ACT_OBJ pActObj)
{
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueFirst;
	PAUDIO_BUF_QUEUE pAudBufQueFirst = pActObj->pAudBufQueFirst;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	UINT32 addCount = pActObj->unitSize / (pActObj->audInfo.bitpersample >> 3);
	UINT32 tcTrigVal = 0;
	//UINT32 addQuecnt = 0;
	WAVSTUD_ACT actType = pActObj->actId;
	PWAVSTUD_ACT_OBJ pActPlayObj = &(gWavStudObj.actObj[actType]);
	FLGPTN uiFlag;
	//SEM_HANDLE SEM_ID_QUE = SEM_ID_WAVSTUD_PLAY_QUE;


	SEM_WAIT(SEM_ID_WAVSTUD_PLAY_QUE);

	pActPlayObj->pAudBufQueTail =
		pActPlayObj->pAudBufQueHead =
			pActObj->pAudBufQueFirst;
	pActObj->currentBufRemain = pActObj->unitSize;

	#if PRELOAD_ZERO == DISABLE
	SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
	#endif

	set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_GOING);
	clr_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_STOPPED);

	//DBG_DUMP("Head=%x,Tail=%x++\r\n", pActObj->pAudBufQueHead->uiAddress, pActObj->pAudBufQueTail->uiAddress);

	addCount = (AUDIO_CH_STEREO == pActObj->audInfo.aud_ch) ? addCount / 2 : addCount;
	tcTrigVal = (addCount > pActObj->stopCount) ? pActObj->stopCount : addCount;

	DBG_IND(":audSR=%d,stopCnt=%u,tcVal=%d\r\n", pActObj->audInfo.aud_sr, (UINT32)pActObj->stopCount, tcTrigVal);
	DBG_IND("Unit Size=%d\r\n", pAudBufQue->uiSize);

	#if PRELOAD_ZERO == DISABLE
	if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH) {
		//wait first queue ready
		while (pActObj->queEntryNum < 3) {
			wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_EVT_QUE_READY | FLG_ID_WAVSTUD_CMD_STOP, TWF_ORW | TWF_CLR);
			if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
				break;
			}
		}

		if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
			DBG_IND("Stop play before start!\r\n");
			set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_CMD_STOP);
			return;
		}
	}

	SEM_WAIT(SEM_ID_WAVSTUD_PLAY_QUE);
	#endif

	while ((pAudBufQue->pNext != pAudBufQueFirst) && \
		   (FALSE == WavStud_AudDrvIsBuffQueFull(WAVSTUD_ACT_PLAY, audCh))) {
		if (pActObj->dataMode == WAVSTUD_DATA_MODE_PULL) {
			//Read and Decode Data for the first n second
			if (FALSE == pActObj->fpDataCB(pAudBufQue, 0, pActObj->actId, 0)) {
				DBG_ERR("Read WavData Fail\r\n");
				set_flg(FLG_ID_WAVSTUD_PLAY, (FLG_ID_WAVSTUD_EVT_RWERR | FLG_ID_WAVSTUD_CMD_STOP));
				break;
			}
			if (0 == pAudBufQue->uiSize) {
				break;
			}
		} else {
			#if PRELOAD_ZERO == ENABLE
			pAudBufQue->uiSize = pActObj->unitSize;
			memset((void *)pAudBufQue->uiAddress, 0, pAudBufQue->uiSize);

			pActPlayObj->queEntryNum++;

			//DBG_IND("Preload 0 buffer\r\n");
			if (gWavStudObj.pWavStudRecvCB) {
				(gWavStudObj.pWavStudRecvCB)(WAVSTUD_ACT_PLAY, 0, 0);
			}
			#else
			pActObj->pAudBufQueTail = pActObj->pAudBufQueTail->pNext;
			#endif
		}

		WavStud_Decode(pActObj, pAudBufQue);

		//Add buffer to queue
		WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_PLAY, audCh, pAudBufQue);

		pAudBufQue = pAudBufQue->pNext;

#if (THIS_DBGLVL >= 5)
		DBG_IND(":unitSize=0x%x,insertCnt=%d\r\n", pActObj->unitSize, pActObj->queEntryNum);
#endif

		#if PRELOAD_ZERO == DISABLE
		if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH) {
			if (pActObj->queEntryNum == 0) {
				break;
			}
		}
		#endif
	}
	pActObj->pAudBufQueNext = pAudBufQue;
	#if PRELOAD_ZERO == ENABLE
	pActObj->pAudBufQueHead = pAudBufQue;
	#endif
	//DBG_DUMP("Head=%x,Tail=%x,Next=%x--\r\n", pActObj->pAudBufQueHead->uiAddress, pActObj->pAudBufQueTail->uiAddress, pActObj->pAudBufQueNext->uiAddress);
	SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
	#if PRELOAD_ZERO == ENABLE
	if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH) {
		//wait first queue ready
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_EVT_QUE_READY | FLG_ID_WAVSTUD_CMD_STOP, TWF_ORW | TWF_CLR);
		if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
			DBG_IND("Stop play before start!\r\n");
			set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_CMD_STOP);
		}
	}
	#endif
	if (pActObj->pWavStudEvtCB) {
		pActObj->pWavStudEvtCB(pActObj->actId, WAVSTUD_CB_EVT_PRELOAD_DONE, 0);
	}

	WavStud_AudDrvSetTCTrigger(WAVSTUD_ACT_PLAY, tcTrigVal);
	WavStud_AudDrvPlay();

	DBG_IND(":audCh=%d,unitSize=0x%x,SR=%d\r\n", audCh, pActObj->unitSize, pActObj->audInfo.aud_sr);
}
static WAVSTUD_CB_EVT WavStud_NextPlay(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_CB_EVT retV = WAVSTUD_CB_EVT_OK;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueNext;
	PAUDIO_BUF_QUEUE pAudBufQueDone = WavStud_AudDrvGetDoneBufQue(WAVSTUD_ACT_PLAY, audCh);
	UINT32 addCount = 0;
	UINT32 tmpSize = 0;
	WAVSTUD_ACT actType = pActObj->actId;
	PWAVSTUD_ACT_OBJ pActPlayObj = &(gWavStudObj.actObj[actType]);
	//SEM_HANDLE SEM_ID_QUE = SEM_ID_WAVSTUD_PLAY_QUE;

	SEM_WAIT(SEM_ID_WAVSTUD_PLAY_QUE);
	pActPlayObj->pAudBufQueTail = pActPlayObj->pAudBufQueTail->pNext;
	pActObj->queEntryNum--;

	//Add played sample count
	addCount = pActObj->unitSize / (pActObj->audInfo.bitpersample >> 3);
	addCount = (AUDIO_CH_STEREO == pActObj->audInfo.aud_ch) ? addCount / 2 : addCount;
	pActObj->currentCount += addCount;
	DBG_IND(":Current=%u,Stop=%u\r\n", pActObj->currentCount, (UINT32)pActObj->stopCount);

	if ((WAVSTUD_NON_STOP_COUNT != pActObj->stopCount) && (pActObj->currentCount >= pActObj->stopCount)) {
		pActObj->currentCount = pActObj->stopCount;
		set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_CMD_STOP);
		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
		DBG_IND("Play done!\r\n");
		return retV;
	}

	//fix for CID 43121 - begin
	if ((pAudBufQue == NULL) || (pAudBufQueDone == NULL)) {
		DBG_ERR("Read Buff Que is NULL!\r\n");
		set_flg(FLG_ID_WAVSTUD_PLAY, (FLG_ID_WAVSTUD_EVT_BUF_FULL | FLG_ID_WAVSTUD_CMD_STOP));
		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
		return WAVSTUD_CB_EVT_FAIL;
	}
	//fix for CID 43121 - end
	if (pAudBufQue && (pAudBufQue->pNext == pAudBufQueDone)) {
		DBG_ERR("Read Buff Que Full\r\n");
		set_flg(FLG_ID_WAVSTUD_PLAY, (FLG_ID_WAVSTUD_EVT_BUF_FULL | FLG_ID_WAVSTUD_CMD_STOP));
		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
		return WAVSTUD_CB_EVT_BUF_FULL;
	}
	if (pAudBufQueDone && (pAudBufQueDone->pNext == pAudBufQue)) {
		DBG_ERR("Read Buff Que Empty\r\n");
		set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_EVT_BUF_EMPTY);
		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
		return WAVSTUD_CB_EVT_BUF_EMPTY;
	}

	tmpSize = pAudBufQue->uiSize;
	//DBG_DUMP("Head=%x,Next=%x,Tail=%x++\r\n", pActObj->pAudBufQueHead->uiAddress, pActObj->pAudBufQueNext->uiAddress, pActObj->pAudBufQueTail->uiAddress);

	if (pActObj->dataMode == WAVSTUD_DATA_MODE_PULL) {
		//Read data size should <= uiSize
		if (FALSE == pActObj->fpDataCB(pAudBufQue, 0, pActObj->actId, 0)) {
			DBG_ERR("Read WavData Fail\r\n");
			set_flg(FLG_ID_WAVSTUD_PLAY, (FLG_ID_WAVSTUD_EVT_RWERR | FLG_ID_WAVSTUD_CMD_STOP));
			retV = WAVSTUD_CB_EVT_READ_FAIL;
			SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
			return retV;
		}
		if (0 == pAudBufQue->uiSize) {
			retV = WAVSTUD_CB_EVT_READ_LAST;
			SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
			return retV;
		} else if (tmpSize < pAudBufQue->uiSize) {
			DBG_ERR("Buffer size too large\r\n");
			set_flg(FLG_ID_WAVSTUD_PLAY, (FLG_ID_WAVSTUD_EVT_RWERR | FLG_ID_WAVSTUD_CMD_STOP));
			retV = WAVSTUD_CB_EVT_READ_FAIL;
			SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
			return retV;
		}
	} else {
		PAUDIO_BUF_QUEUE pAudBufQueHead = pActObj->pAudBufQueHead;
		DBG_IND("pAudBufQue=%x,pAudBufQueHead=%x\r\n", pAudBufQue->uiAddress, pAudBufQueHead->uiAddress);

		//queue empty
		if (pAudBufQue == pAudBufQueHead) {
			DBG_IND("Queue empty\r\n");
			if (FALSE == pActObj->fpDataCB(pAudBufQue, 0, pActObj->actId, 0)) {
				if (0 == pAudBufQue->uiSize) {
					DBG_ERR("Read WavData Fail\r\n");
					set_flg(FLG_ID_WAVSTUD_PLAY, (FLG_ID_WAVSTUD_EVT_RWERR | FLG_ID_WAVSTUD_CMD_STOP));
					retV = WAVSTUD_CB_EVT_READ_FAIL;
					SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
					return retV;
				} else {
					//no data, play 0
					DBG_IND("No data, play 0\r\n");
					if (pActObj->currentBufRemain != pActObj->unitSize && pActObj->currentBufRemain != 0) {
						DBG_IND("unit size=%d, remain size=%d\r\n", pActObj->unitSize, pActObj->currentBufRemain);
						memset((void *)(pAudBufQue->uiAddress + (pActObj->unitSize - pActObj->currentBufRemain)), 0, pActObj->currentBufRemain);
					} else {
						pAudBufQue = &empty_queue;
						pAudBufQue->uiAddress = (UINT32)empty_buff;
						pAudBufQue->uiSize = tmpSize/pActObj->unitTime;
					}
					//memset((void *)(pAudBufQue->uiAddress + (pActObj->unitSize - pActObj->currentBufRemain)), 0, pActObj->currentBufRemain);
					if (pActObj->stopCount != WAVSTUD_NON_STOP_COUNT) {
						pActObj->stopCount += addCount/pActObj->unitTime;
					}

					pActObj->queEntryNum++;

					if (gWavStudObj.pWavStudRecvCB) {
						(gWavStudObj.pWavStudRecvCB)(WAVSTUD_ACT_PLAY, 0, 0);
					}
				}
			}
		}

		//read last
		if (0 == pAudBufQue->uiSize) {
			retV = WAVSTUD_CB_EVT_READ_LAST;
			SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
			return retV;
		}
		//buffer size is larger than expected size
		else if (tmpSize < pActObj->unitSize) {
			DBG_ERR("Buffer size too large\r\n");
			set_flg(FLG_ID_WAVSTUD_PLAY, (FLG_ID_WAVSTUD_EVT_RWERR | FLG_ID_WAVSTUD_CMD_STOP));
			retV = WAVSTUD_CB_EVT_READ_FAIL;
			SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
			return retV;
		}


		if (pAudBufQue == pAudBufQueHead) {
			pActObj->pAudBufQueHead = pAudBufQueHead->pNext;
			//pActObj->queEntryNum++;
			pActObj->currentBufRemain = pActObj->unitSize;
			//DBG_DUMP("Empty, Head=%x\r\n", pActObj->pAudBufQueHead->uiAddress);
		}

	}

	pAudBufQue->uiValidSize = 0;

	WavStud_Decode(pActObj, pAudBufQue);

	//Add buffer to queue
	WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_PLAY, audCh, pAudBufQue);
	if (pAudBufQue->pNext != NULL)
		pActObj->pAudBufQueNext = pAudBufQue->pNext;
	//DBG_DUMP("Head=%x,Next=%x,Tail=%x--\r\n", pActObj->pAudBufQueHead->uiAddress, pActObj->pAudBufQueNext->uiAddress, pActObj->pAudBufQueTail->uiAddress);
	SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
	return retV;
}
#endif

static void WavStud_PausePlay(WAVSTUD_ACT actType)
{
	ID flgid = (WAVSTUD_ACT_PLAY == actType) ? FLG_ID_WAVSTUD_PLAY : FLG_ID_WAVSTUD_RECORD;
	DBG_IND(":actType=%d\r\n", actType);
	WavStud_AudDrvPause(actType);
	clr_flg(flgid, FLG_ID_WAVSTUD_STS_GOING);
	set_flg(flgid, FLG_ID_WAVSTUD_STS_PAUSED);
}
static void WavStud_ResumePlay(WAVSTUD_ACT actType)
{
	ID flgid = (WAVSTUD_ACT_PLAY == actType) ? FLG_ID_WAVSTUD_PLAY : FLG_ID_WAVSTUD_RECORD;
	DBG_IND(":actType=%d\r\n", actType);
	WavStud_AudDrvResume(actType);
	clr_flg(flgid, FLG_ID_WAVSTUD_STS_PAUSED);
	set_flg(flgid, FLG_ID_WAVSTUD_STS_GOING);
}

THREAD_DECLARE(WavStudio_PlayTsk, arglist)
{
	FLGPTN uiFlag = 0, status = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_PLAY]);
	WAVSTUD_CB_EVT cbSts = WAVSTUD_CB_EVT_OK;

	kent_tsk();

	while (1) {
		DBG_IND(":f=0x%x,status=0x%x +\r\n", uiFlag, status);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_WAIT, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		status = kchk_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS);
		DBG_IND(":f=0x%x,status=0x%x -\r\n", uiFlag, status);

		if (uiFlag & FLG_ID_WAVSTUD_EVT_CLOSE) {
			break;
		}

		if (status & FLG_ID_WAVSTUD_STS_GOING) {
			if (uiFlag & FLG_ID_WAVSTUD_EVT_TCHIT) {
				WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_TCHIT, pActObj->currentCount);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_FULL) {
				DBG_IND("Buf Full\r\n");
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_EMPTY) {
				DBG_IND("Buf Empty\r\n");
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_RWERR) {
				DBG_WRN("Read Storage Fail\r\n");
				WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_READ_FAIL, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
				WavStud_StopPlay(pActObj);
				WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_STOPPED, 0);
				continue;
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_READY) {
				WavStud_NextPlay(pActObj);
			}
			if (uiFlag & FLG_ID_WAVSTUD_CMD_PAUSE) {
				WavStud_PausePlay(WAVSTUD_ACT_PLAY);
				WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_PAUSED, 0);
			}
		}
		if (status & FLG_ID_WAVSTUD_STS_PAUSED) {
			if (uiFlag & FLG_ID_WAVSTUD_CMD_RESUME) {
				WavStud_ResumePlay(WAVSTUD_ACT_PLAY);
				WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_RESUMED, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
				WavStud_StopPlay(pActObj);
				WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_STOPPED, 0);
				continue;
			}
		}
		if ((uiFlag & FLG_ID_WAVSTUD_CMD_START) && (status & FLG_ID_WAVSTUD_STS_STOPPED)) {
			cbSts = WavStud_StartPlay(pActObj);
			if (WAVSTUD_CB_EVT_OK != cbSts) {
				DBG_ERR("Play Fail:%d\r\n", cbSts);
				WAVSTUD_PLAY_CB(cbSts, 0);
				continue;
			} else {
				//Make Aud-Buf-Que link-list
				if (WavStud_MakeAudBufQue(pActObj) == 0) {
					DBG_ERR("Make buffer queue error, Mem Not Enough\r\n");
					WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_FAIL, 0);
					continue;
				}
				#if (7 <= THIS_DBGLVL)
				WavStud_DmpModuleInfo();
				#endif
				//Init. AudDrv for playing
				WavStud_AudDrvInit(WAVSTUD_ACT_PLAY, &pActObj->audInfo, pActObj->audVol, 0);

				DBG_IND("[AudOut] SR:%d,CH:%d,bps:%d,outDevice:%d\r\n", \
						 pActObj->audInfo.aud_sr, pActObj->audInfo.aud_ch, pActObj->audInfo.bitpersample, gWavStudObj.playOutDev);
				//Add-Buf-Que and Play
				WavStud_TrigPlay(pActObj);
			}
		}
	}

	set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS_IDLE);
	THREAD_RETURN(0);
}

BOOL wavstudio_push_buf_to_queue(UINT32 addr, UINT32 *size)
{
#if 1//defined(__UITRON)
	PWAVSTUD_ACT_OBJ pActPlayObj = &(gWavStudObj.actObj[WAVSTUD_ACT_PLAY]);
	FLGPTN status = 0;
	UINT32 pushSize = *size;
	//SEM_HANDLE SEM_ID_QUE = SEM_ID_WAVSTUD_PLAY_QUE;

	//UINT32 block = 0;       //needed block number
	UINT32 sizeToEnd = 0;   //size to the end of current buffer which is not full
	//UINT32 remainBlock = 0; //remain block number of play queue
	UINT32 offset = 0;
	UINT32 copy = 0;
	UINT32 uiSize = 0;
	unsigned long flags;

	status = kchk_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS);

	if (pActPlayObj->dataMode == WAVSTUD_DATA_MODE_PULL) {
		DBG_IND("Play task is not passive mode\r\n");
		//*size = 0;
		return FALSE;
	}

	if (status & FLG_ID_WAVSTUD_STS_STOPPED) {
		DBG_IND("Play task is not started\r\n");
		//*size = 0;
		return FALSE;
	}

	if (pushSize == 0) {
		DBG_IND("Push size is 0!\r\n");
		return FALSE;
	}

	SEM_WAIT(SEM_ID_WAVSTUD_PLAY_QUE);

	//remainBlock = pActPlayObj->audBufQueCnt - pActPlayObj->queEntryNum;
	//DBG_DUMP("audBufQueCnt=%d,queEntryNum=%d\r\n", pActPlayObj->audBufQueCnt, pActPlayObj->queEntryNum);
	sizeToEnd = pActPlayObj->currentBufRemain;
	offset = pActPlayObj->unitSize - sizeToEnd;

	//calculate needed buffer block
	#if 0
	if (sizeToEnd < pushSize) {
		#if 1
		block = (pushSize - sizeToEnd) / pActPlayObj->unitSize + 1;
		if (pushSize > (block * sizeToEnd)) {
			block++;
		}
		#else //for block push
		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
		return FALSE;
		#endif
	} else {
		block = 1;
	}
	#else
	if (pushSize > wavstudio_get_remain_buf(WAVSTUD_ACT_PLAY)) {
		//buffer is not enough
		SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);
		return FALSE;
	}
	#endif

	//DBG_DUMP("unit size=%d,push size=%d,need block=%d,remain block =%d, currBufRemain=%d\r\n", pActPlayObj->unitSize, pushSize, block, remainBlock, sizeToEnd);

	DBG_IND(" pAudBufQueHead=%x++\r\n", pActPlayObj->pAudBufQueHead->uiAddress);

	{
		while (pushSize) {
			uiSize = (pActPlayObj->currentBufRemain > pushSize) ? pushSize : pActPlayObj->currentBufRemain;
			memcpy((void *)(pActPlayObj->pAudBufQueHead->uiAddress + offset), (const void *)(addr + copy), uiSize);
			pActPlayObj->currentBufRemain -= uiSize;
			if (pActPlayObj->currentBufRemain == 0) { //buffer full, head moves to next buffer
				pActPlayObj->pAudBufQueHead->uiSize = pActPlayObj->unitSize;
				pActPlayObj->pAudBufQueHead->uiValidSize = 0;
				if (pActPlayObj->filt_cb) {
					pActPlayObj->filt_cb(pActPlayObj->pAudBufQueHead->uiAddress, pActPlayObj->pAudBufQueHead->uiAddress, pActPlayObj->unitSize);
				}
				pActPlayObj->pAudBufQueHead = pActPlayObj->pAudBufQueHead->pNext;
				loc_cpu(flags);
				pActPlayObj->queEntryNum++;
				pActPlayObj->remain_entry_num++;
				pActPlayObj->currentBufRemain = pActPlayObj->unitSize;
				unl_cpu(flags);
				offset = 0;

				if (status & FLG_ID_WAVSTUD_STS_WAIT_PRELOAD) {
					if (pActPlayObj->remain_entry_num >= 3) {
						WAVSTUD_PLAY_CB(WAVSTUD_CB_EVT_PRELOAD_DONE, 0);
					}
				}
				set_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_EVT_QUE_READY);
				//DBG_DUMP("Filled block\r\n");
			} else {
				offset += uiSize;
			}
			pushSize -= uiSize;
			copy += uiSize;
			//remainBlock--;
		}
		*size = copy;
	}

	SEM_SIGNAL(SEM_ID_WAVSTUD_PLAY_QUE);

	DBG_IND(" pAudBufQueHead=%x--\r\n", pActPlayObj->pAudBufQueHead->uiAddress);

	if (copy == 0) {
		return FALSE;
	} else {
		if (gWavStudObj.pWavStudRecvCB) {
			(gWavStudObj.pWavStudRecvCB)(WAVSTUD_ACT_PLAY, 1, 0);
		}

		return TRUE;
	}
#else
	return TRUE;
#endif

}

UINT32 wavstudio_get_remain_buf(WAVSTUD_ACT act_type)
{
	PWAVSTUD_ACT_OBJ pActPlayObj = &(gWavStudObj.actObj[act_type]);
	UINT32 sizeToEnd = 0;   //size to the end of current buffer which is not full
	UINT32 remainBlock = 0; //remain block number of play queue
	UINT32 size = 0;
	FLGPTN status = 0;
	unsigned long flags;

	if (act_type != WAVSTUD_ACT_PLAY && act_type != WAVSTUD_ACT_PLAY2) {
		DBG_ERR("Not supported act type = %d\r\n", act_type);
		return 0;
	}

	if (act_type == WAVSTUD_ACT_PLAY) {
		status = kchk_flg(FLG_ID_WAVSTUD_PLAY, FLG_ID_WAVSTUD_STS);
	} else {
		status = kchk_flg(FLG_ID_WAVSTUD_PLAY2, FLG_ID_WAVSTUD_STS);
	}

	if (status & FLG_ID_WAVSTUD_STS_STOPPED) {
		DBG_IND("Play task is not started\r\n");
		return 0;
	}

	loc_cpu(flags);
	remainBlock = (pActPlayObj->audBufQueCnt-1) - pActPlayObj->queEntryNum;
	sizeToEnd = pActPlayObj->currentBufRemain;
	unl_cpu(flags);

	if (sizeToEnd == pActPlayObj->unitSize && remainBlock == 0) {
		//queue full
		sizeToEnd = 0;
	} else if (remainBlock > 0) {
		remainBlock--;
	}

	size = remainBlock * pActPlayObj->unitSize + sizeToEnd;

	return size;
}

//@}
