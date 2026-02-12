/*
    @file       WavStudioRecordTsk.c
    @ingroup    mILIBWAVSTUDIO

    @brief      Wav Studio Task.

                Task body of Wav Studio task.
                Record a WAV file.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

/**
    @addtogroup mILIBWAVSTUDIO
*/
//@{

#include "wavstudio_int.h"
#include "wavstudio_aud_intf.h"
#include "wavstudio_id.h"

#if WAVSTUD_FIX_BUF == ENABLE
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)
#endif


typedef struct _WAVSTUD_CTRL {
	AUDIO_BUF_QUEUE    *pQueuePrev;
	AUDIO_BUF_QUEUE    *pQueueCurr;
	UINT32              uiDmaPrev;
	UINT32              uiDmaCurr;
	UINT32              uiRemainAddr;
	UINT32              uiRemainSize;
} WAVSTUD_CTRL, *PWAVSTUD_CTRL;

extern WAVSTUD_OBJ gWavStudObj;
BOOL gWriteFail = FALSE;
extern SEM_HANDLE SEM_ID_WAVSTUD_RECORD_QUE;
//UINT32 gWavStudRecHITCount = 0;
UINT32 gWavStudRecFrmRate = 30;
static WAVSTUD_CTRL m_WavStudCtrl = {0};
static WAVSTUD_CTRL m_WavStudCtrl_lb = {0};
UINT32 g_uiWavStudProcSize = WAVSTUD_PROCSIZE;
UINT32 gWavStudApplyAddr = 0;
UINT32 gWavStudApplySize = 0;
UINT64 wavstud_rec_timpstamp = 0;
UINT32 wavstud_timpstamp_curr_addr = 0;
UINT32 gWavStudTimerID = 0;
BOOL recordtsk_start = FALSE;
BOOL aec_skip = FALSE;
UINT32 wavdbg = 0;
#if defined (__KERNEL__)
BOOL wavstudio_first_boot = TRUE;
#endif

#if defined __UITRON || defined __ECOS
#if 0//WAVSTUD_FIX_BUF == DISABLE
static void WavStud_RecTimerHdlr(UINT32 nouse)
{
	FLGPTN uiFlag = FLG_ID_WAVSTUD_EVT_TCHIT;

	if (uiFlag) {
		set_flg(FLG_ID_WAVSTUD_RECORD, uiFlag);
	}
}
#endif
#endif

#if 0
static void WavStud_RecEvtHdlr(UINT32 uiEventID)
{
	FLGPTN uiFlag = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_REC]);
	UINT32 tcTrigVal = pActObj->currentCount;
	UINT32 addCount;
	//UINT32 hit_n = 0;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	PAUDIO_BUF_QUEUE pAudBufQue = 0;
	PAUDIO_BUF_QUEUE pAudBufQueDone = 0;
	UINT32 idx = 0;
	UINT32 valid_size = 0;

	DBG_IND(":evt=0x%x,currentCount=%u,stopCount=%llu\r\n", uiEventID, pActObj->currentCount, pActObj->stopCount);

	if (uiEventID & AUDIO_EVENT_TCHIT) {
		if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH_FILE) {
			//Add recorded sample count
			addCount = pActObj->unitSize / (pActObj->audInfo.bitpersample >> 3);
			if (AUDIO_CH_STEREO == pActObj->audInfo.aud_ch) {
				addCount /= 2;
			}
			tcTrigVal += 2 * addCount;
			pActObj->currentCount += addCount;

			if (tcTrigVal <= pActObj->stopCount) {
				WavStud_AudDrvSetTCTrigger(WAVSTUD_ACT_REC, tcTrigVal);
			}

		}
		uiFlag |= FLG_ID_WAVSTUD_EVT_TCHIT;
	}

	if (uiEventID & AUDIO_EVENT_DMADONE) {
		//uiFlag |= FLG_ID_WAVSTUD_EVT_BUF_READY;
		pAudBufQue = pActObj->pAudBufQueNext;
		pAudBufQueDone = 0;


		loc_cpu();
		pAudBufQueDone = WavStud_AudDrvGetDoneBufQue(pActObj->actId, pActObj->audInfo.aud_ch);

		if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
			idx = WavStud_BufToIdx(pActObj->actId, pAudBufQueDone);
			valid_size = pAudBufQueDone->uiValidSize;
		}
		unl_cpu();

		//fix for CID 43122 & 43123 - begin
		if ((pAudBufQue == NULL) || (pAudBufQueDone == NULL)) {
			//DBG_ERR("Write Buff Que is NULL!\r\n");
			uiFlag |= FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_BUF_FULL;
		}
		//fix for CID 43122 & 43123 - end
		if (pAudBufQue && (pAudBufQue->pNext == pAudBufQueDone)) {
			//DBG_ERR("Write Buffer Que Full\r\n");
			uiFlag |= FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_BUF_FULL;
		}
		if (pAudBufQueDone && (pAudBufQueDone->pNext == pAudBufQue)) {
			//DBG_ERR("Write Buffer Que Empty\r\n");
			uiFlag |= FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_BUF_EMPTY;
		}

		//if (pAudBufQue->uiSize != pActRecObj->unitSize) {
		//	DBG_ERR("uiSize = %x\r\n", pAudBufQue->uiSize);
		//	WavStud_DmpModuleInfo();
		//}

		if (WavStud_IsQLock(WAVSTUD_ACT_REC, pAudBufQue)) {
			//Next buffer is still locked. Add temp buffer.
			WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_REC, audCh, pActObj->pAudBufQueTmp);
		} else {
			pAudBufQue->uiValidSize = 0;
			WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_REC, audCh, pAudBufQue);
			pActObj->pAudBufQueNext = pAudBufQue->pNext;
		}

		if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {

			pAudBufQueDone = WavStud_IdxToBuf(WAVSTUD_ACT_REC, idx);
			pAudBufQueDone->uiValidSize = valid_size;

			if (WavStud_AddDoneQue(WAVSTUD_ACT_REC, pAudBufQueDone)) {
				set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_TCHIT);
			} else {
				//DBG_WRN("Output queue is full. Drop buffer!\r\n");
			}
		}
	}
	if (uiEventID & AUDIO_EVENT_DMADONE2) {
		//uiFlag |= FLG_ID_WAVSTUD_EVT_BUF_READY2;
		pAudBufQue = pActObj->pAudBufQueNext;
		pAudBufQueDone = 0;

		loc_cpu();
		pAudBufQueDone = WavStud_AudDrvGetDoneBufQue(pActObj->actId, pActObj->audInfo.aud_ch);

		if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
			idx = WavStud_BufToIdx(pActObj->actId, pAudBufQueDone);
			valid_size = pAudBufQueDone->uiValidSize;
		}
		unl_cpu();

		//fix for CID 43122 & 43123 - begin
		if ((pAudBufQue == NULL) || (pAudBufQueDone == NULL)) {
			//DBG_ERR("Write Buff Que is NULL!\r\n");
			uiFlag |= FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_BUF_FULL2;
		}
		//fix for CID 43122 & 43123 - end
		if (pAudBufQue && (pAudBufQue->pNext == pAudBufQueDone)) {
			//DBG_ERR("Write Buffer Que Full\r\n");
			uiFlag |= FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_BUF_FULL2;
		}
		if (pAudBufQueDone && (pAudBufQueDone->pNext == pAudBufQue)) {
			//DBG_ERR("Write Buffer Que Empty\r\n");
			uiFlag |= FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_BUF_EMPTY2;
		}

		//if (pAudBufQue->uiSize != pActRecObj->unitSize) {
		//	DBG_ERR("uiSize = %x\r\n", pAudBufQue->uiSize);
		//	WavStud_DmpModuleInfo();
		//}

		if (WavStud_IsQLock(WAVSTUD_ACT_REC2, pAudBufQue)) {
			//Next buffer is still locked. Add temp buffer.
			WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_REC2, audCh, pActObj->pAudBufQueTmp);
		} else {
			pAudBufQue->uiValidSize = 0;
			WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_REC2, audCh, pAudBufQue);
			pActObj->pAudBufQueNext = pAudBufQue->pNext;
		}

		if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {

			pAudBufQueDone = WavStud_IdxToBuf(WAVSTUD_ACT_REC2, idx);
			pAudBufQueDone->uiValidSize = valid_size;

			//if (WavStud_AddDoneQue(WAVSTUD_ACT_REC2, pAudBufQueDone)) {
			//	set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_TCHIT);
			//} else {
				//DBG_WRN("Output queue is full. Drop buffer!\r\n");
			//}
		}
	}
	if (uiEventID & (AUDIO_EVENT_BUF_FULL | AUDIO_EVENT_DONEBUF_FULL)) {
		DBG_ERR("Aud Buf Full:0x%x\r\n", uiEventID);
		uiFlag |= (FLG_ID_WAVSTUD_EVT_BUF_FULL | FLG_ID_WAVSTUD_CMD_STOP);
	}
	if (uiEventID & (AUDIO_EVENT_FIFO_ERROR | AUDIO_EVENT_FIFO_ERROR2)) {
		DBG_ERR("AUDIO_EVENT_FIFO_ERROR,Stop Record\r\n");
		uiFlag |= FLG_ID_WAVSTUD_CMD_STOP;
	}
	if (uiEventID & (AUDIO_EVENT_BUF_FULL2 | AUDIO_EVENT_DONEBUF_FULL2)) {
		DBG_ERR("Aud Buf Full2:0x%x\r\n", uiEventID);
		uiFlag |= (FLG_ID_WAVSTUD_EVT_BUF_FULL2 | FLG_ID_WAVSTUD_CMD_STOP);
	}

	if (uiFlag) {
		set_flg(FLG_ID_WAVSTUD_RECORD, uiFlag);
	}
}

static void WavStud_LBRecEvtHdlr(UINT32 uiEventID)
{
	FLGPTN uiFlag = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_LB]);
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueNext;
	PAUDIO_BUF_QUEUE pAudBufQueDone = 0;
	UINT32 idx = 0;
	UINT32 valid_size = 0;

	if (uiEventID & AUDIO_EVENT_DMADONE) {
		//uiFlag |= FLG_ID_WAVSTUD_EVT_LB_BUF_READY;

		loc_cpu();
		pAudBufQueDone = WavStud_AudDrvGetDoneBufQue(pActObj->actId, pActObj->audInfo.aud_ch);

		if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
			idx = WavStud_BufToIdx(pActObj->actId, pAudBufQueDone);
			valid_size = pAudBufQueDone->uiValidSize;
		}
		unl_cpu();

		if ((pAudBufQue == NULL) || (pAudBufQueDone == NULL)) {
			//DBG_ERR("LB Buff Que is NULL!\r\n");
			uiFlag |= FLG_ID_WAVSTUD_CMD_STOP;
		}

		//if (pAudBufQue->uiSize != pActObj->unitSize) {
		//	DBG_ERR("size = %x\r\n", pAudBufQue->uiSize);
		//	WavStud_DmpModuleInfo();
		//}

		if (WavStud_IsQLock(WAVSTUD_ACT_LB, pAudBufQue)) {
			//Next buffer is still locked. Add temp buffer.
			//DBG_ERR("lock_addr=%x, uiAddress=%x, unitSize=%x\r\n", pActObj->lock_addr, pAudBufQue->uiAddress, pAudBufQue->uiSize);
			//DBG_ERR("uiAddress=%x, unitSize=%x\r\n", pAudBufQue->uiAddress, pAudBufQue->uiSize);
			//DBG_ERR("Buffer to be added is locked!\r\n");
			WavStud_AudDrvAddBuffQue(pActObj->actId, pActObj->audInfo.aud_ch, pActObj->pAudBufQueTmp);
		} else {
			pAudBufQue->uiValidSize = 0;
			WavStud_AudDrvAddBuffQue(pActObj->actId, pActObj->audInfo.aud_ch, pAudBufQue);
			pActObj->pAudBufQueNext = pAudBufQue->pNext;
		}

		if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {

			pAudBufQueDone = WavStud_IdxToBuf(WAVSTUD_ACT_LB, idx);
			pAudBufQueDone->uiValidSize = valid_size;

			if (!WavStud_AddDoneQue(pActObj->actId, pAudBufQueDone)) {
				DBG_WRN("Output queue is full. Drop buffer!\r\n");
			}
		}
	}
	if (uiEventID & (AUDIO_EVENT_BUF_FULL | AUDIO_EVENT_DONEBUF_FULL)) {
		DBG_ERR("Aud Buf Full:0x%x\r\n", uiEventID);
		uiFlag |= (FLG_ID_WAVSTUD_EVT_LB_BUF_FULL | FLG_ID_WAVSTUD_CMD_STOP);
	}
	if (uiFlag) {
		set_flg(FLG_ID_WAVSTUD_RECORD, uiFlag);
	}
}
#endif

#if defined (__KERNEL__)
static void wavstud_trigger_cb(UINT32 curr_addr)
{
	UINT32 idx = 0;
	PAUDIO_BUF_QUEUE pAudBufQue = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_REC]);
	PWAVSTUD_ACT_OBJ pActObjLB = &(gWavStudObj.actObj[WAVSTUD_ACT_LB]);
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	AUDIO_CH audChLB = pActObjLB->audInfo.aud_ch;
	FLGPTN uiFlag = FLG_ID_WAVSTUD_EVT_TCHIT;

	idx = WavStud_AddrToIdx(WAVSTUD_ACT_REC, curr_addr);

	if (gWavStudObj.aec_en) {
		pAudBufQue = WavStud_IdxToBuf(WAVSTUD_ACT_LB, idx);

		pAudBufQue = pAudBufQue->pNext;
		WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_LB, audChLB, pAudBufQue, 0);

		pAudBufQue = pAudBufQue->pNext;
		WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_LB, audChLB, pAudBufQue, 1);

		pActObjLB->pAudBufQueNext = pAudBufQue->pNext;
	}

	pAudBufQue = WavStud_IdxToBuf(WAVSTUD_ACT_REC, idx);

	pAudBufQue = pAudBufQue->pNext;
	WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_REC, audCh, pAudBufQue, 0);

	pAudBufQue = pAudBufQue->pNext;
	WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_REC, audCh, pAudBufQue, 1);

	pActObj->pAudBufQueNext = pAudBufQue->pNext;

	if (kdrv_builtin_is_fastboot() && wavstudio_first_boot) {
		if (gWavStudObj.aec_en) {
			audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_DONE_QUEUE_TXLB, (UINT32*)&(pActObjLB->wavAudQue));
		}
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_DONE_QUEUE, (UINT32*)&(pActObj->wavAudQue));
		wavstudio_first_boot = FALSE;
	}

	set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, uiFlag);
}
#endif

INT32 wavstud_rec_buf_cb(VOID *callback_info, VOID *user_data)
{
	FLGPTN uiFlag = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_REC]);
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	PAUDIO_BUF_QUEUE pAudBufQue = 0;
	PAUDIO_BUF_QUEUE pAudBufQueDone = 0;
	UINT32 idx = 0;
	UINT32 valid_size = 0;
	unsigned long flags;

	clr_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE);

	if (!recordtsk_start) {
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE);
		return 0;
	}

	pAudBufQueDone = (AUDIO_BUF_QUEUE*)(*(UINT32 *)user_data);

	if (pAudBufQueDone == 0) {
		DBG_ERR("Done buffer is null\r\n");
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE);
		return 0;
	}

	loc_cpu(flags);

	idx = WavStud_BufToIdx(pActObj->actId, pActObj->pAudBufQueNext);
	pAudBufQue = WavStud_IdxToBuf(pActObj->actId, idx);

	if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
		idx = WavStud_BufToIdx(pActObj->actId, pAudBufQueDone);
		pAudBufQueDone = WavStud_IdxToBuf(WAVSTUD_ACT_REC, idx);
		valid_size = pAudBufQueDone->uiSize;
	}
	unl_cpu(flags);

	//fix for CID 43122 & 43123 - begin
	if (pAudBufQue == NULL) {
		DBG_ERR("Write Buff Que is NULL!\r\n");
		uiFlag |= FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_BUF_FULL | FLG_ID_WAVSTUD_STS_ISR_IDLE;
		set_flg(FLG_ID_WAVSTUD_RECORD, uiFlag);
		return 0;
	}

	//if (pAudBufQue->uiSize != pActRecObj->unitSize) {
	//	DBG_ERR("uiSize = %x\r\n", pAudBufQue->uiSize);
	//	WavStud_DmpModuleInfo();
	//}

	if (WavStud_IsQLock(WAVSTUD_ACT_REC, pAudBufQue)) {
		//Next buffer is still locked. Add temp buffer.
		WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_REC, audCh, pActObj->pAudBufQueTmp, 1);
		uiFlag |= FLG_ID_WAVSTUD_EVT_BUF_FULL;
	} else {
		pAudBufQue->uiValidSize = 0;
		WavStud_AudDrvAddBuffQue(WAVSTUD_ACT_REC, audCh, pAudBufQue, 1);
		WavStud_LockQ(pActObj->actId, pAudBufQue);
		pActObj->pAudBufQueNext = pAudBufQue->pNext;
		uiFlag |= FLG_ID_WAVSTUD_EVT_BUF_READY;
	}

	if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
		pAudBufQueDone->uiValidSize = valid_size;

		if (WavStud_AddDoneQue(WAVSTUD_ACT_REC, pAudBufQueDone)) {
			uiFlag |= FLG_ID_WAVSTUD_EVT_TCHIT;
		} else {
			//DBG_WRN("Output queue is full. Drop buffer!\r\n");
		}
	}

	set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, uiFlag);
	set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE);
	return 0;
}

INT32 wavstud_lb_buf_cb(VOID *callback_info, VOID *user_data)
{
	FLGPTN uiFlag = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_LB]);
	PAUDIO_BUF_QUEUE pAudBufQue = 0;
	PAUDIO_BUF_QUEUE pAudBufQueDone = 0;
	UINT32 idx = 0;
	UINT32 valid_size = 0;
	unsigned long flags;

	clr_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE2);

	if (!recordtsk_start) {
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE2);
		return 0;
	}

	pAudBufQueDone = (AUDIO_BUF_QUEUE*)(*(UINT32 *)user_data);

	if (pAudBufQueDone == 0) {
		DBG_ERR("Done buffer is null\r\n");
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE2);
		return 0;
	}

	loc_cpu(flags);
	idx = WavStud_BufToIdx(pActObj->actId, pActObj->pAudBufQueNext);
	pAudBufQue = WavStud_IdxToBuf(pActObj->actId, idx);

	if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
		idx = WavStud_BufToIdx(pActObj->actId, pAudBufQueDone);
		pAudBufQueDone = WavStud_IdxToBuf(WAVSTUD_ACT_LB, idx);
		valid_size = pAudBufQueDone->uiSize;
	}
	unl_cpu(flags);

	if (pAudBufQue == NULL) {
		DBG_ERR("LB Buff Que is NULL!\r\n");
		uiFlag |= FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_STS_ISR_IDLE2;
		set_flg(FLG_ID_WAVSTUD_RECORD, uiFlag);
		return 0;
	}

	//if (pAudBufQue->uiSize != pActObj->unitSize) {
	//	DBG_ERR("size = %x\r\n", pAudBufQue->uiSize);
	//	WavStud_DmpModuleInfo();
	//}

	if (WavStud_IsQLock(WAVSTUD_ACT_LB, pAudBufQue)) {
		//Next buffer is still locked. Add temp buffer.
		//DBG_ERR("lock_addr=%x, uiAddress=%x, unitSize=%x\r\n", pActObj->lock_addr, pAudBufQue->uiAddress, pAudBufQue->uiSize);
		//DBG_ERR("uiAddress=%x, unitSize=%x\r\n", pAudBufQue->uiAddress, pAudBufQue->uiSize);
		//DBG_ERR("Buffer to be added is locked!\r\n");
		WavStud_AudDrvAddBuffQue(pActObj->actId, pActObj->audInfo.aud_ch, pActObj->pAudBufQueTmp, 1);
	} else {
		pAudBufQue->uiValidSize = 0;
		WavStud_AudDrvAddBuffQue(pActObj->actId, pActObj->audInfo.aud_ch, pAudBufQue, 1);
		WavStud_LockQ(pActObj->actId, pAudBufQue);
		pActObj->pAudBufQueNext = pAudBufQue->pNext;
	}

	if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
		pAudBufQueDone->uiValidSize = valid_size;

		if (WavStud_AddDoneQue(pActObj->actId, pAudBufQueDone)) {
			uiFlag |= FLG_ID_WAVSTUD_EVT_TCHIT;
		} else {
			//DBG_WRN("Output queue is full. Drop buffer!\r\n");
		}
	}

	set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, uiFlag);
	set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE2);

	return 0;
}

static WAVSTUD_CB_EVT WavStud_StartRec(PWAVSTUD_ACT_OBJ pActObj)
{
	PWAVSTUD_ACT_OBJ pActObj2 = &(gWavStudObj.actObj[WAVSTUD_ACT_REC2]);
	WAVSTUD_CB_EVT rv = WAVSTUD_CB_EVT_OK;
	DBG_IND("++\r\n");

	gWriteFail = FALSE;

	if (WAVSTUD_BITS_PER_SAM_8 == pActObj->audInfo.bitpersample) {
		DBG_ERR("Not Support resolution=%d\r\n", pActObj->audInfo.bitpersample);
		return WAVSTUD_CB_EVT_FAIL;
	}

	pActObj->unitSize = pActObj2->unitSize = WavStud_GetUnitSize(pActObj->audInfo.aud_sr, pActObj->audInfo.ch_num, pActObj->audInfo.bitpersample, pActObj->buf_sample_cnt);

	#if WAVSTUD_FIX_BUF == DISABLE
	g_uiWavStudProcSize = WAVSTUD_PROCSIZE * (pActObj->audInfo.bitpersample >> 3) * pActObj->audChs;
	#endif

	if (gWavStudObj.aec_en) {
		#if WAVSTUD_FIX_BUF == DISABLE
		gWavStudObj.actObj[WAVSTUD_ACT_LB].unitSize = pActObj->unitSize;

		/*if (gWavStudObj.paecObj->start) {
			gWavStudObj.paecObj->start((INT32)pActObj->audInfo.aud_sr, (pActObj->audInfo.aud_ch == AUDIO_CH_STEREO) ? 2 : 1, (gWavStudObj.lb_ch == AUDIO_CH_STEREO) ? 2 : 1);
		}*/
		#else
		PWAVSTUD_ACT_OBJ pActObjlb = &(gWavStudObj.actObj[WAVSTUD_ACT_LB]);

		pActObjlb->unitSize = WavStud_GetUnitSize(pActObjlb->audInfo.aud_sr, pActObjlb->audInfo.ch_num, pActObjlb->audInfo.bitpersample, pActObjlb->buf_sample_cnt);

		/*if (gWavStudObj.paecObj->start) {
			gWavStudObj.paecObj->start((INT32)pActObj->audInfo.aud_sr, pActObj->audInfo.ch_num, pActObjlb->audInfo.ch_num);
		}*/
		#endif

	}
	recordtsk_start = TRUE;
	set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE | FLG_ID_WAVSTUD_STS_ISR_IDLE2);
	if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH) {
		set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_STS_GOING);
		clr_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_STS_STOPPED | FLG_ID_WAVSTUD_EVT_TCHIT);
#if WAVSTUD_FIX_BUF == DISABLE
		#if defined __UITRON || defined __ECOS
		if (timer_open((TIMER_ID *)&gWavStudTimerID, WavStud_RecTimerHdlr) != E_OK) {
			DBG_ERR("WavStudio rec trigger timer open failed\r\n");
			return WAVSTUD_CB_EVT_FAIL;
		}
		#endif
#endif
	}

	WavStud_AudDrvRec(WAVSTUD_ACT_REC);

	DBG_IND("--\r\n");
	return rv;
}
static WAVSTUD_CB_EVT WavStud_StopRec(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_CB_EVT rv = WAVSTUD_CB_EVT_OK;
	FLGPTN uiFlag;
	DBG_IND("++\r\n");

	if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH) {
		#if defined __UITRON || defined __ECOS
		timer_close(gWavStudTimerID);
		#endif
		clr_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_TCHIT | FLG_ID_WAVSTUD_STS_GOING);
		set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_STS_STOPPED);
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_RWIDLE, TWF_ORW);
	}

	if (gWavStudObj.aec_en) {
		WavStud_AudDrvStop(WAVSTUD_ACT_LB);
		/*if (gWavStudObj.paecObj->stop) {
			gWavStudObj.paecObj->stop();
		}*/
	}

	WavStud_AudDrvStop(WAVSTUD_ACT_REC);
	DBG_IND("currentCount=%u, SR=%d, Bps=%d\r\n", pActObj->currentCount, pActObj->audInfo.aud_sr, pActObj->audInfo.bitpersample);

	recordtsk_start = FALSE;

	wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE, TWF_ORW);
	if (gWavStudObj.aec_en) {
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_ISR_IDLE2, TWF_ORW);
	}

	//wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_EVT_RWIDLE, TWF_ORW);

	clr_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_ALL);
	set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_STOPPED);

	gWriteFail = FALSE;
	//gWavStudRecHITCount = 0;
	memset((void *)&m_WavStudCtrl, 0, sizeof(WAVSTUD_CTRL));
	memset((void *)&m_WavStudCtrl_lb, 0, sizeof(WAVSTUD_CTRL));
	gWavStudApplyAddr = 0;
	gWavStudApplySize = 0;

	DBG_IND("--\r\n");
	return rv;
}

static void WavStud_TrigRec(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_ACT actType = pActObj->actId;
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueFirst;
	PAUDIO_BUF_QUEUE pAudBufQueFirst = pActObj->pAudBufQueFirst;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	PWAVSTUD_ACT_OBJ pActRecObj = &(gWavStudObj.actObj[actType]);
	//SEM_HANDLE *SEM_ID_QUE = &SEM_ID_WAVSTUD_RECORD_QUE;
	UINT32 insertQuecnt = 0;
	unsigned long flags;

	pActObj->lock_addr = pAudBufQue->uiAddress;

	SEM_WAIT(SEM_ID_WAVSTUD_RECORD_QUE);
	pActRecObj->pAudBufQueHead = pAudBufQueFirst;
	pActRecObj->pAudBufQueTail = pAudBufQueFirst;
	SEM_SIGNAL(SEM_ID_WAVSTUD_RECORD_QUE);

	DBG_IND(":audSR=%d, actType=%d\r\n", pActObj->audInfo.aud_sr, actType);
	while (insertQuecnt < 2) {
		DBG_IND("pAudBufQue=0x%x\r\n", pAudBufQue->uiAddress);
		loc_cpu(flags);
		pActObj->pAudBufQueNext = pAudBufQue->pNext;
		unl_cpu(flags);

		if (insertQuecnt == 1) {
			WavStud_AudDrvAddBuffQue(actType, audCh, pAudBufQue, 1);
			WavStud_LockQ(pActObj->actId, pAudBufQue);
		} else {
			WavStud_AudDrvAddBuffQue(actType, audCh, pAudBufQue, 0);
			WavStud_LockQ(pActObj->actId, pAudBufQue);
		}
		pAudBufQue = pAudBufQue->pNext;

		insertQuecnt ++;
		DBG_IND("unitSize=0x%x,insertCnt=%d\r\n", pActObj->unitSize, insertQuecnt);
	}

	insertQuecnt = 0;
	while (insertQuecnt < 2) {
		WAVSTUD_REC_CB(WAVSTUD_CB_EVT_NEW_ENTER, 0);
		WAVSTUD_REC_CB(WAVSTUD_CB_EVT_NEW_OK, 0);
		insertQuecnt ++;
	}

	DBG_IND(":audCh=%d,unitSize=0x%x,SR=%d\r\n", audCh, pActObj->unitSize, pActObj->audInfo.aud_sr);
}

static void WavStud_TrigLBRec(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_ACT actType = pActObj->actId;
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueFirst;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	UINT32 insertQuecnt = 0;

	SEM_WAIT(SEM_ID_WAVSTUD_RECORD_QUE);
	pActObj->pAudBufQueHead = pAudBufQue;
	pActObj->pAudBufQueTail = pAudBufQue;
	SEM_SIGNAL(SEM_ID_WAVSTUD_RECORD_QUE);

	DBG_IND(":audSR=%d, actType=%d\r\n", pActObj->audInfo.aud_sr, actType);
	while (insertQuecnt < 2) {
		DBG_IND("pAudBufQue=0x%x\r\n", pAudBufQue->uiAddress);
		WavStud_AudDrvAddBuffQue(actType, audCh, pAudBufQue, 1);
		WavStud_LockQ(pActObj->actId, pAudBufQue);
		pAudBufQue = pAudBufQue->pNext;
		insertQuecnt ++;
	}
	pActObj->pAudBufQueNext = pAudBufQue;

	DBG_IND(":audCh=%d,unitSize=0x%x,SR=%d\r\n", audCh, pActObj->unitSize, pActObj->audInfo.aud_sr);
}


#if WAVSTUD_FIX_BUF == ENABLE
static WAVSTUD_CB_EVT WavStud_NextRec(PWAVSTUD_ACT_OBJ pActObj)
{
	FLGPTN setptn = 0;
	WAVSTUD_ACT actType = pActObj->actId;
	WAVSTUD_CB_EVT retV = WAVSTUD_CB_EVT_OK;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueNext;
	PAUDIO_BUF_QUEUE pAudBufQueDone = 0;//WavStud_AudDrvGetDoneBufQue(actType, audCh);
	PWAVSTUD_ACT_OBJ pActRecObj = &(gWavStudObj.actObj[actType]);
	UINT32 idx = 0;
	UINT32 valid_size = 0;
	unsigned long flags;

	loc_cpu(flags);
	pAudBufQueDone = WavStud_AudDrvGetDoneBufQue(pActObj->actId, pActObj->audInfo.aud_ch);

	if (pAudBufQueDone == NULL) {
		unl_cpu(flags);
		DBG_ERR("Done buff is NULL!\r\n");
		setptn = FLG_ID_WAVSTUD_CMD_STOP;
		set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		return WAVSTUD_CB_EVT_FAIL;
	}

	if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
		idx = WavStud_BufToIdx(pActObj->actId, pAudBufQueDone);
		valid_size = pAudBufQueDone->uiValidSize;
	}
	unl_cpu(flags);

	//fix for CID 43122 & 43123 - begin
	if ((pAudBufQue == NULL) || (pAudBufQueDone == NULL)) {
		DBG_ERR("Write Buff Que is NULL!\r\n");
		setptn = FLG_ID_WAVSTUD_CMD_STOP;
		if (WAVSTUD_ACT_REC == actType) {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_FULL;
		} else {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_FULL2;
		}
		set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		return WAVSTUD_CB_EVT_FAIL;
	}
	//fix for CID 43122 & 43123 - end
	if (pAudBufQue && (pAudBufQue->pNext == pAudBufQueDone)) {
		DBG_ERR("Write Buffer Que Full\r\n");
		setptn = FLG_ID_WAVSTUD_CMD_STOP;
		if (WAVSTUD_ACT_REC == actType) {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_FULL;
		} else {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_FULL2;
		}
		set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		return WAVSTUD_CB_EVT_BUF_FULL;
	}
	if (pAudBufQueDone && (pAudBufQueDone->pNext == pAudBufQue)) {
		DBG_ERR("Write Buffer Que Empty\r\n");
		setptn = FLG_ID_WAVSTUD_CMD_STOP;
		if (WAVSTUD_ACT_REC == actType) {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_EMPTY;
		} else {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_EMPTY2;
		}
		set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		return WAVSTUD_CB_EVT_BUF_EMPTY;
	}

	if (pAudBufQue->uiSize != pActRecObj->unitSize) {
		DBG_ERR("uiSize = %x\r\n", pAudBufQue->uiSize);
		WavStud_DmpModuleInfo();
	}

	if (WavStud_IsQLock(WAVSTUD_ACT_REC, pAudBufQue)) {
		//Next buffer is still locked. Add temp buffer.
		//DBG_ERR("lock_addr=%x, uiAddress=%x, unitSize=%x\r\n", pActObj->lock_addr, pAudBufQue->uiAddress, pAudBufQue->uiSize);
		//DBG_ERR("uiAddress=%x, unitSize=%x\r\n", pAudBufQue->uiAddress, pAudBufQue->uiSize);
		//DBG_ERR("Buffer to be added is locked!\r\n");
		//DBGD(WavStud_HowManyinQ(pActObj->actId));
		WavStud_AudDrvAddBuffQue(actType, audCh, pActObj->pAudBufQueTmp, 1);
		return retV;
	} else {
		pAudBufQue->uiValidSize = 0;
		WavStud_AudDrvAddBuffQue(actType, audCh, pAudBufQue, 1);
		pActObj->pAudBufQueNext = pAudBufQue->pNext;
	}

	if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {

		pAudBufQueDone = WavStud_IdxToBuf(WAVSTUD_ACT_REC, idx);
		pAudBufQueDone->uiValidSize = valid_size;

		if (WavStud_AddDoneQue(actType, pAudBufQueDone)) {
			set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_TCHIT);
		} else {
			DBG_WRN("Output queue is full. Drop buffer!\r\n");
		}
	}

	return retV;
}

static WAVSTUD_CB_EVT WavStud_NextLB(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_CB_EVT retV = WAVSTUD_CB_EVT_OK;
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueNext;
	PAUDIO_BUF_QUEUE pAudBufQueDone = 0;//WavStud_AudDrvGetDoneBufQue(pActObj->actId, pActObj->audInfo.audCh);
	UINT32 idx = 0;
	UINT32 valid_size = 0;
	unsigned long flags;

	loc_cpu(flags);
	pAudBufQueDone = WavStud_AudDrvGetDoneBufQue(pActObj->actId, pActObj->audInfo.aud_ch);

	if (pAudBufQueDone == NULL) {
		unl_cpu(flags);
		DBG_ERR("Done buff is NULL!\r\n");
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_CMD_STOP);
		return WAVSTUD_CB_EVT_FAIL;
	}

	if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {
		idx = WavStud_BufToIdx(pActObj->actId, pAudBufQueDone);
		valid_size = pAudBufQueDone->uiValidSize;
	}
	unl_cpu(flags);

	if ((pAudBufQue == NULL) || (pAudBufQueDone == NULL)) {
		DBG_ERR("LB Buff Que is NULL!\r\n");
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_CMD_STOP);
		retV = WAVSTUD_CB_EVT_FAIL;
		return retV;
	}

	if (pAudBufQue->uiSize != pActObj->unitSize) {
		DBG_ERR("size = %x\r\n", pAudBufQue->uiSize);
		WavStud_DmpModuleInfo();
	}

	if (WavStud_IsQLock(WAVSTUD_ACT_LB, pAudBufQue)) {
		//Next buffer is still locked. Add temp buffer.
		//DBG_ERR("lock_addr=%x, uiAddress=%x, unitSize=%x\r\n", pActObj->lock_addr, pAudBufQue->uiAddress, pAudBufQue->uiSize);
		//DBG_ERR("uiAddress=%x, unitSize=%x\r\n", pAudBufQue->uiAddress, pAudBufQue->uiSize);
		//DBG_ERR("Buffer to be added is locked!\r\n");
		WavStud_AudDrvAddBuffQue(pActObj->actId, pActObj->audInfo.aud_ch, pActObj->pAudBufQueTmp, 1);
		return retV;
	} else {
		pAudBufQue->uiValidSize = 0;
		WavStud_AudDrvAddBuffQue(pActObj->actId, pActObj->audInfo.aud_ch, pAudBufQue, 1);
		pActObj->pAudBufQueNext = pAudBufQue->pNext;
	}

	if (pAudBufQueDone->uiAddress != pActObj->pAudBufQueTmp->uiAddress) {

		pAudBufQueDone = WavStud_IdxToBuf(WAVSTUD_ACT_LB, idx);
		pAudBufQueDone->uiValidSize = valid_size;

		if (!WavStud_AddDoneQue(pActObj->actId, pAudBufQueDone)) {
			DBG_WRN("Output queue is full. Drop buffer!\r\n");
		}
	}

	return retV;
}
#else
/*static WAVSTUD_CB_EVT WavStud_NextRec(PWAVSTUD_ACT_OBJ pActObj)
{
	FLGPTN setptn = 0;
	WAVSTUD_ACT actType = pActObj->actId;
	WAVSTUD_CB_EVT retV = WAVSTUD_CB_EVT_OK;
	AUDIO_CH audCh = pActObj->audInfo.aud_ch;
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueNext;
	PAUDIO_BUF_QUEUE pAudBufQueDone = WavStud_AudDrvGetDoneBufQue(actType, audCh);
	PWAVSTUD_ACT_OBJ pActRecObj = &(gWavStudObj.actObj[actType]);
	//SEM_HANDLE SEM_ID_QUE = SEM_ID_WAVSTUD_RECORD_QUE;

	//fix for CID 43122 & 43123 - begin
	if ((pAudBufQue == NULL) || (pAudBufQueDone == NULL)) {
		DBG_ERR("Write Buff Que is NULL!\r\n");
		setptn = FLG_ID_WAVSTUD_CMD_STOP;
		if (WAVSTUD_ACT_REC == actType) {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_FULL;
		} else {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_FULL2;
		}
		set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		return WAVSTUD_CB_EVT_FAIL;
	}
	//fix for CID 43122 & 43123 - end
	if (pAudBufQue && (pAudBufQue->pNext == pAudBufQueDone)) {
		DBG_ERR("Write Buffer Que Full\r\n");
		setptn = FLG_ID_WAVSTUD_CMD_STOP;
		if (WAVSTUD_ACT_REC == actType) {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_FULL;
		} else {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_FULL2;
		}
		set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		return WAVSTUD_CB_EVT_BUF_FULL;
	}
	if (pAudBufQueDone && (pAudBufQueDone->pNext == pAudBufQue)) {
		DBG_ERR("Write Buffer Que Empty\r\n");
		setptn = FLG_ID_WAVSTUD_CMD_STOP;
		if (WAVSTUD_ACT_REC == actType) {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_EMPTY;
		} else {
			setptn |= FLG_ID_WAVSTUD_EVT_BUF_EMPTY2;
		}
		set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		return WAVSTUD_CB_EVT_BUF_EMPTY;
	}

	SEM_WAIT(SEM_ID_WAVSTUD_RECORD_QUE);
	pActRecObj->pAudBufQueHead->uiValidSize = pAudBufQueDone->uiSize;
	pActRecObj->pAudBufQueHead = pActRecObj->pAudBufQueHead->pNext;
	SEM_SIGNAL(SEM_ID_WAVSTUD_RECORD_QUE);

	pAudBufQue->uiValidSize = 0;

	if (pAudBufQue->uiSize != pActRecObj->unitSize) {
		DBG_ERR("uiSize = %x\r\n", pAudBufQue->uiSize);
		WavStud_DmpModuleInfo();
	}

	if (pActObj->lock_addr >= pAudBufQue->uiAddress && pActObj->lock_addr < pAudBufQue->uiAddress + pAudBufQue->uiValidSize) {
		DBG_ERR("lock_addr=%x, uiAddress=%x, unitSize=%x\r\n", pActObj->lock_addr, pAudBufQue->uiAddress, pAudBufQue->uiValidSize);
		DBG_ERR("Buffer to be added is locked!\r\n");
	}

	WavStud_AudDrvAddBuffQue(actType, audCh, pAudBufQue);
	pActObj->pAudBufQueNext = pAudBufQue->pNext;

	if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH_FILE) {
		if (WAVSTUD_ACT_REC == actType) {
			setptn |= FLG_ID_WAVSTUD_EVT_RW;
		} else {
			setptn |= FLG_ID_WAVSTUD_EVT_RW2;
		}
		set_flg(FLG_ID_WAVSTUD_RECORDWRITE, setptn);
	} else {
		SEM_WAIT(SEM_ID_WAVSTUD_RECORD_QUE);
		pActRecObj->pAudBufQueTail = pActRecObj->pAudBufQueTail->pNext;
		SEM_SIGNAL(SEM_ID_WAVSTUD_RECORD_QUE);
	}

	DBG_IND(":Current=%u,Stop=%llu\r\n", pActObj->currentCount, pActObj->stopCount);

	if ((WAVSTUD_NON_STOP_COUNT != pActObj->stopCount) && (pActObj->currentCount >= pActObj->stopCount)) {
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_CMD_STOP);
		return retV;
	}
	return retV;
}

static WAVSTUD_CB_EVT WavStud_NextLB(PWAVSTUD_ACT_OBJ pActObj)
{
	WAVSTUD_CB_EVT retV = WAVSTUD_CB_EVT_OK;
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueNext;
	PAUDIO_BUF_QUEUE pAudBufQueDone = WavStud_AudDrvGetDoneBufQue(pActObj->actId, pActObj->audInfo.aud_ch);

	if ((pAudBufQue == NULL) || (pAudBufQueDone == NULL)) {
		DBG_ERR("LB Buff Que is NULL!\r\n");
		set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_CMD_STOP);
		retV = WAVSTUD_CB_EVT_FAIL;
		return retV;
	}

	if (pAudBufQue->uiSize != pActObj->unitSize) {
		DBG_ERR("size = %x\r\n", pAudBufQue->uiSize);
		WavStud_DmpModuleInfo();
	}

	pAudBufQue->uiValidSize = 0;
	WavStud_AudDrvAddBuffQue(pActObj->actId, pActObj->audInfo.aud_ch, pAudBufQue);
	pActObj->pAudBufQueNext = pAudBufQue->pNext;

	return retV;
}*/
#endif

static void WavStud_PauseRec(WAVSTUD_ACT actType)
{
	ID flgid = (WAVSTUD_ACT_PLAY == actType) ? FLG_ID_WAVSTUD_PLAY : FLG_ID_WAVSTUD_RECORD;
	DBG_IND(":actType=%d\r\n", actType);
	WavStud_AudDrvPause(actType);
	clr_flg(flgid, FLG_ID_WAVSTUD_STS_GOING);
	set_flg(flgid, FLG_ID_WAVSTUD_STS_PAUSED);
}
static void WavStud_ResumeRec(WAVSTUD_ACT actType)
{
	ID flgid = (WAVSTUD_ACT_PLAY == actType) ? FLG_ID_WAVSTUD_PLAY : FLG_ID_WAVSTUD_RECORD;
	DBG_IND(":actType=%d\r\n", actType);
	WavStud_AudDrvResume(actType);
	clr_flg(flgid, FLG_ID_WAVSTUD_STS_PAUSED);
	set_flg(flgid, FLG_ID_WAVSTUD_STS_GOING);
}

static WAVSTUD_CB_EVT WavStud_RecWriteData(PWAVSTUD_ACT_OBJ pActObj, PAUDIO_BUF_QUEUE pAudBufQue)
{
	FLGPTN setptn = 0;
	WAVSTUD_ACT actType = pActObj->actId;
	WAVSTUD_CB_EVT retV = WAVSTUD_CB_EVT_OK;

	if (FALSE == pActObj->fpDataCB(pAudBufQue, 0, pActObj->actId, 0, 0)) {
		setptn = FLG_ID_WAVSTUD_CMD_STOP;
		if (WAVSTUD_ACT_REC == actType) {
			setptn |= FLG_ID_WAVSTUD_EVT_RWERR;
		} else {
			setptn |= FLG_ID_WAVSTUD_EVT_RWERR2;
		}
		set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		retV = WAVSTUD_CB_EVT_WRITE_FAIL;
	}

	return retV;
}

#if WAVSTUD_FIX_BUF == ENABLE
static void WavStud_RecPushBuf(void)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_REC]);
	WAVSTUD_CTRL *pCtrl = &m_WavStudCtrl;
	AUDIO_BUF_QUEUE audioQue = {0};
	AUDIO_BUF_QUEUE audioQue_aec = {0};
	FLGPTN setptn = 0;
	UINT64 timestamp = 0;
	UINT64 timestamp_aec = 0;

	WAVSTUD_CTRL *pCtrl_lb = &m_WavStudCtrl_lb;
	UINT32 idx = 0;
	//UINT32 time_1, time_2;

	PWAVSTUD_AUDQUE pWavAudQue = 0;

	if (gWavStudObj.aec_en) {
		if (WavStud_HowManyinQ(WAVSTUD_ACT_LB) == 0) {
			DBG_IND("LB is empty\r\n");
			return;
		}
	}

	pWavAudQue = WavStud_GetDoneQue(pActObj->actId);

	while (pWavAudQue != NULL) {
		pCtrl->pQueueCurr = pWavAudQue->pAudQue;

		if (gWavStudObj.aec_en) {
			PWAVSTUD_AUDQUE pWavAudQuelb = WavStud_GetDoneQue(WAVSTUD_ACT_LB);
			if (pWavAudQuelb == 0) {
				DBG_ERR("LB done queue is empty\r\n");
			} else {
				UINT32 idx2 = 0;

				idx = WavStud_BufToIdx(WAVSTUD_ACT_REC, pCtrl->pQueueCurr);
				pCtrl_lb->pQueueCurr = pWavAudQuelb->pAudQue;
				idx2 = WavStud_BufToIdx(WAVSTUD_ACT_LB, pCtrl_lb->pQueueCurr);

				timestamp_aec = pWavAudQuelb->timestamp;

				if (idx != idx2) {
					DBG_ERR("Buffer not match. %d, %d\r\n", idx, idx2);
				}
			}
		}

		if (wavdbg & 1) {
			idx = WavStud_BufToIdx(WAVSTUD_ACT_REC, pActObj->pAudBufQueNext);
			DBG_DUMP("TX idx=%d\r\n", idx);
			idx = WavStud_BufToIdx(WAVSTUD_ACT_LB, gWavStudObj.actObj[WAVSTUD_ACT_LB].pAudBufQueNext);
			DBG_DUMP("LB idx=%d\r\n", idx);
		}

		gWavStudApplyAddr = pCtrl->pQueueCurr->uiAddress;

		gWavStudApplySize = pCtrl->pQueueCurr->uiValidSize;

		if (gWavStudObj.aec_en) {
			audioQue_aec.uiAddress = pCtrl_lb->pQueueCurr->uiAddress;
			audioQue_aec.uiValidSize = pCtrl_lb->pQueueCurr->uiValidSize;
		}

		if (gWavStudApplySize > 0) {

			audioQue.uiAddress = gWavStudApplyAddr;
			audioQue.uiValidSize = gWavStudApplySize;

			timestamp = pWavAudQue->timestamp;

			if (FALSE == pActObj->fpDataCB(&audioQue, &audioQue_aec, pActObj->actId, timestamp, timestamp_aec)) {
				setptn = FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_STREAMERR;
				set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
			}
		}

		/*Get next queue*/
		if (gWavStudObj.aec_en) {
			if (WavStud_HowManyinQ(WAVSTUD_ACT_LB) == 0) {
				pWavAudQue = NULL;
			} else {
				pWavAudQue = WavStud_GetDoneQue(pActObj->actId);
			}
		} else {
			pWavAudQue = WavStud_GetDoneQue(pActObj->actId);
		}
	}
}
#else
/*static void WavStud_RecUpdateCurrBuf(void)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_REC]);
	UINT32  uiBegin, uiEnd, uiSize;
	UINT32  uiBegin_lb = 0, uiEnd_lb = 0;
	BOOL    bDataCopy;
	WAVSTUD_CTRL *pCtrl = &m_WavStudCtrl;
	AUDIO_BUF_QUEUE audioQue = {0};
	FLGPTN setptn = 0;
	UINT64 timestamp = 0;
	UINT64 time_diff = 0;
	UINT32 ts_high = 0, ts_low = 0, diff_high = 0, diff_low = 0;

	WAVSTUD_CTRL *pCtrl_lb = &m_WavStudCtrl_lb;
	UINT32 idx = 0;
	UINT32 count = 0;
	UINT32 time_1, time_2;

	UINT32 buf_end = 0;
	PAUDIO_BUF_QUEUE ts_queue = 0;

	pCtrl->pQueueCurr = WavStud_AudDrvGetCurrBufQue(WAVSTUD_ACT_REC, pActObj->audInfo.aud_ch);
	idx = WavStud_BufToIdx(WAVSTUD_ACT_REC, pCtrl->pQueueCurr);
	pCtrl->pQueueCurr = WavStud_IdxToBuf(WAVSTUD_ACT_REC, idx);

	pCtrl->uiDmaCurr = WavStud_AudDrvGetCurrDMA(WAVSTUD_ACT_REC, pActObj->audInfo.aud_ch);

	//Get timestamp here
	#if defined __UITRON || defined __ECOS
	wavstud_rec_timpstamp = HwClock_GetLongCounter();
	#endif
	wavstud_timpstamp_curr_addr = pCtrl->uiDmaCurr;

	while (!((pCtrl->uiDmaCurr < pCtrl->pQueueCurr->uiAddress + pCtrl->pQueueCurr->uiSize) && (pCtrl->uiDmaCurr >= pCtrl->pQueueCurr->uiAddress))) {
		pCtrl->pQueueCurr = pCtrl->pQueueCurr->pNext;
	}

	ts_queue = pCtrl->pQueueCurr;

	if (gWavStudObj.paecObj) {
		idx = WavStud_BufToIdx(WAVSTUD_ACT_REC, pCtrl->pQueueCurr);

		pCtrl_lb->pQueueCurr = WavStud_IdxToBuf(WAVSTUD_ACT_LB, idx);
		pCtrl_lb->uiDmaCurr = pCtrl_lb->pQueueCurr->uiAddress + (pCtrl->uiDmaCurr - pCtrl->pQueueCurr->uiAddress);
	}

	bDataCopy = FALSE;

	// 1st time trigger
	if (g_bStarting) {
		g_bStarting = FALSE;

		uiBegin = pCtrl->pQueueCurr->uiAddress;

		// the process size should be the multiple of 1KB
		uiSize = ((pCtrl->uiDmaCurr - uiBegin) / g_uiWavStudProcSize) * g_uiWavStudProcSize;
		pCtrl->uiDmaCurr = uiBegin + uiSize;

		if (gWavStudObj.paecObj) {
			uiBegin_lb = pCtrl_lb->pQueueCurr->uiAddress;
			pCtrl_lb->uiDmaCurr = uiBegin_lb + uiSize;
		}

		DBG_MSG("1st time, proc addr 0x%X, size 0x%X, Q start 0x%X\r\n", uiBegin, uiSize, pCtrl->pQueueCurr->uiAddress);
	}
	else if (pCtrl->pQueuePrev != pCtrl->pQueueCurr && pCtrl->pQueuePrev->pNext != pCtrl->pQueueCurr) {

		//Queue skipped
		PAUDIO_BUF_QUEUE paudioQue = pCtrl->pQueueCurr;

		while (pCtrl->pQueuePrev->pNext != paudioQue) {
			paudioQue = paudioQue->pNext;
			count++;
		}

		DBG_WRN("%d queue skipped \r\n", count);

		pCtrl->uiRemainSize = 0;
		pCtrl_lb->uiRemainSize = 0;
		uiBegin = pCtrl->pQueueCurr->uiAddress;
		if (gWavStudObj.paecObj) {
			uiBegin_lb = pCtrl_lb->pQueueCurr->uiAddress;
		}

		// the process size should be the multiple of 1KB
		uiSize = ((pCtrl->uiDmaCurr - uiBegin) / g_uiWavStudProcSize) * g_uiWavStudProcSize;
		pCtrl->uiDmaCurr = uiBegin + uiSize;

		if (gWavStudObj.paecObj) {
			pCtrl_lb->uiDmaCurr = uiBegin_lb + uiSize;
		}
		DBG_MSG("Queue skipped, proc addr 0x%X, size 0x%X, Q start 0x%X\r\n", uiBegin, uiSize, pCtrl->pQueueCurr->uiAddress);
	} else if (pCtrl->pQueuePrev == pCtrl->pQueueCurr && pCtrl->uiDmaCurr < pCtrl->uiDmaPrev) {
		//Queue skipped
		pCtrl->uiRemainSize = 0;
		pCtrl_lb->uiRemainSize = 0;
		uiBegin = pCtrl->pQueueCurr->uiAddress;
		if (gWavStudObj.paecObj) {
			uiBegin_lb = pCtrl_lb->pQueueCurr->uiAddress;
		}

		// the process size should be the multiple of 1KB
		uiSize = ((pCtrl->uiDmaCurr - uiBegin) / g_uiWavStudProcSize) * g_uiWavStudProcSize;
		pCtrl->uiDmaCurr = uiBegin + uiSize;

		if (gWavStudObj.paecObj) {
			pCtrl_lb->uiDmaCurr = uiBegin_lb + uiSize;
		}
		DBG_MSG("Queue skipped, proc addr 0x%X, size 0x%X, Q start 0x%X\r\n", uiBegin, uiSize, pCtrl->pQueueCurr->uiAddress);
	}
	// audio buffer had been changed
	else if (pCtrl->pQueuePrev != pCtrl->pQueueCurr) {
		uiBegin = pCtrl->uiDmaPrev;

		// process to the previous buffer end
		uiEnd = pCtrl->pQueuePrev->uiAddress + pCtrl->pQueuePrev->uiSize;

		// remaining size for processing on next time
		pCtrl->uiRemainSize = (uiEnd - uiBegin) % g_uiWavStudProcSize;
		pCtrl->uiRemainAddr = uiEnd - pCtrl->uiRemainSize;

		if (pCtrl->uiRemainSize > g_uiWavStudProcSize) {
			DBG_ERR("remain=%d", g_uiWavStudProcSize);
		}

		if (gWavStudObj.paecObj) {
			uiBegin_lb = pCtrl_lb->uiDmaPrev;
			uiEnd_lb = pCtrl_lb->pQueuePrev->uiAddress + pCtrl_lb->pQueuePrev->uiSize;
			pCtrl_lb->uiRemainSize = (uiEnd_lb - uiBegin_lb) % g_uiWavStudProcSize;
			pCtrl_lb->uiRemainAddr = uiEnd_lb - pCtrl_lb->uiRemainSize;
		}

		// the process size should be the multiple of 1KB
		uiSize = pCtrl->uiRemainAddr - uiBegin;

		if (pCtrl->uiRemainSize == 0) {
			// update processed address
			pCtrl->uiDmaCurr = pCtrl->pQueueCurr->uiAddress;
			if (gWavStudObj.paecObj) {
				pCtrl_lb->uiDmaCurr = pCtrl_lb->pQueueCurr->uiAddress;
			}
		}

		DBG_MSG("Q change, proc addr 0x%X, size 0x%X, remain 0x%X\r\n", uiBegin, uiSize, pCtrl->uiRemainSize);
	}
	// normal condition
	else {
		if (pCtrl->uiRemainSize) {
			// copy the remaining data to the front of current audio buffer
			uiBegin = pCtrl->pQueueCurr->uiAddress - pCtrl->uiRemainSize;
			if (gWavStudObj.paecObj) {
				uiBegin_lb = pCtrl_lb->pQueueCurr->uiAddress - pCtrl_lb->uiRemainSize;
			}
			if (uiBegin != pCtrl->uiRemainAddr) {
				memcpy((UINT8 *)uiBegin, (UINT8 *)pCtrl->uiRemainAddr, pCtrl->uiRemainSize);

				if (gWavStudObj.paecObj) {
					memcpy((UINT8 *)uiBegin_lb, (UINT8 *)pCtrl_lb->uiRemainAddr, pCtrl_lb->uiRemainSize);
				}
				DBG_IND("uiBegin=0x%x, uiRemainAddr=0x%x, uiRemainSize=0x%x\r\n", uiBegin, pCtrl->uiRemainAddr, pCtrl->uiRemainSize);
			}
			bDataCopy = TRUE;
			DBG_MSG("Data copy, addr 0x%X, size 0x%X\r\n", uiBegin, pCtrl->uiRemainSize);
		} else {
			uiBegin = pCtrl->uiDmaPrev;
			if (gWavStudObj.paecObj) {
				uiBegin_lb = pCtrl_lb->uiDmaPrev;
			}
		}

		// the process size should be the multiple of 1KB
		uiSize = ((pCtrl->uiDmaCurr - uiBegin) / g_uiWavStudProcSize) * g_uiWavStudProcSize;
		pCtrl->uiDmaCurr = uiBegin + uiSize;

		if (gWavStudObj.paecObj) {
			pCtrl_lb->uiDmaCurr = uiBegin_lb + uiSize;
		}
		DBG_MSG("Same Q, proc addr 0x%X, size 0x%X, Q start 0x%X\r\n", uiBegin, uiSize, pCtrl->pQueueCurr->uiAddress);
	}

	//if (gWavStudApplyAddr != 0)
	//    DBG_WRN("gWavStudApplyAddr is not be got, overwrite!!\r\n");
	gWavStudApplyAddr = uiBegin;
	//if (gWavStudApplySize != 0)
	//    DBG_WRN("gWavStudApplySize is not be got, overwrite!!\r\n");
	gWavStudApplySize = uiSize;

	if (bDataCopy == TRUE) {
		pCtrl->uiRemainSize = 0;
		if (gWavStudObj.paecObj) {
			pCtrl_lb->uiRemainSize = 0;
		}
	}

	pCtrl->pQueuePrev = pCtrl->pQueueCurr;
	pCtrl->uiDmaPrev = pCtrl->uiDmaCurr;

	if (gWavStudObj.paecObj) {
		pCtrl_lb->pQueuePrev = pCtrl_lb->pQueueCurr;
		pCtrl_lb->uiDmaPrev = pCtrl_lb->uiDmaCurr;
	}

	//Drop frame if size is too large
	if (gWavStudApplySize > 0x10000) {
		DBG_WRN("Frame Size=0x%x\r\n", gWavStudApplySize);
		DBG_WRN("Drop frame!\r\n");
		gWavStudApplySize = 0x1000;
	}

	if (gWavStudObj.paecObj && gWavStudObj.paecObj->apply) {
		time_1 = Perf_GetCurrent();

		if (!aec_skip)
			gWavStudObj.paecObj->apply(gWavStudApplyAddr, uiBegin_lb, gWavStudApplyAddr, gWavStudApplySize);

		aec_skip = FALSE;
		time_2 = Perf_GetCurrent();

		if (time_2 - time_1 > 33000) {
			DBG_IND("AEC process time is %d ms. Size=%x\r\n", (time_2 - time_1)/1000, gWavStudApplySize);
		}
	}

	if (gWavStudApplySize > 0) {

		audioQue.uiAddress = gWavStudApplyAddr;
		audioQue.uiValidSize = gWavStudApplySize;
		buf_end = gWavStudApplyAddr + gWavStudApplySize;

		//Caluculate time difference
		if (wavstud_timpstamp_curr_addr == buf_end) {
			timestamp = wavstud_rec_timpstamp;
		} else if (wavstud_timpstamp_curr_addr > buf_end){
			time_diff = wavstudio_do_div((((UINT64)((wavstud_timpstamp_curr_addr - buf_end) / (pActObj->audInfo.bitpersample >> 3) / pActObj->audChs)) * 1000000), ((UINT64)pActObj->audInfo.aud_sr));
			ts_high   = (UINT32)(wavstud_rec_timpstamp >> 32);
			ts_low    = (UINT32)(wavstud_rec_timpstamp & 0xffffffff);
			diff_high = (UINT32)wavstudio_do_div(time_diff, 1000000);
			diff_low  = (UINT32)(time_diff - (diff_high*1000000));

			if (ts_low < diff_low) {
				ts_high--;
				ts_low += 1000000;
			}
			timestamp = (((UINT64)(ts_high - diff_high)) << 32) | ((UINT64)(ts_low - diff_low));
		} else {
			UINT32 diff = wavstud_timpstamp_curr_addr - ts_queue->uiAddress + pCtrl->uiRemainSize;

			time_diff = wavstudio_do_div((((UINT64)(diff / (pActObj->audInfo.bitpersample >> 3) / pActObj->audChs)) * 1000000), ((UINT64)pActObj->audInfo.aud_sr));
			ts_high   = (UINT32)(wavstud_rec_timpstamp >> 32);
			ts_low    = (UINT32)(wavstud_rec_timpstamp & 0xffffffff);
			diff_high = (UINT32)wavstudio_do_div(time_diff, 1000000);
			diff_low  = (UINT32)(time_diff - (diff_high*1000000));

			if (ts_low < diff_low) {
				ts_high--;
				ts_low += 1000000;
			}
			timestamp = (((UINT64)(ts_high - diff_high)) << 32) | ((UINT64)(ts_low - diff_low));
		}

		if (FALSE == pActObj->fpDataCB(&audioQue, 0, pActObj->actId, timestamp)) {
			setptn = FLG_ID_WAVSTUD_CMD_STOP | FLG_ID_WAVSTUD_EVT_STREAMERR;
			set_flg(FLG_ID_WAVSTUD_RECORD, setptn);
		}
	}

}*/
#endif
THREAD_DECLARE(WavStudio_RecordTsk, arglist)
{
	FLGPTN uiFlag = 0, status = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_REC]);
	PWAVSTUD_ACT_OBJ pActObj2 = &(gWavStudObj.actObj[WAVSTUD_ACT_REC2]);
	PWAVSTUD_ACT_OBJ pActObj_LB = &(gWavStudObj.actObj[WAVSTUD_ACT_LB]);

	THREAD_ENTRY();

	while (1) {
		DBG_IND(":f=0x%x,status=0x%x +\r\n", uiFlag, status);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_WAIT, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		status = kchk_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS);
		DBG_IND(":f=0x%x,status=0x%x -\r\n", uiFlag, status);

		if (uiFlag & FLG_ID_WAVSTUD_EVT_CLOSE) {
			break;
		}

		if (status & FLG_ID_WAVSTUD_STS_GOING) {
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_FULL) {
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_BUF_FULL, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_FULL2) {
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_BUF_FULL, 1);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_RWERR) {
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_WRITE_FAIL, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_RWERR2) {
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_WRITE_FAIL, 1);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_EMPTY) {
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_BUF_EMPTY, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_EMPTY2) {
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_BUF_EMPTY, 1);
			}
			if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
				WavStud_StopRec(pActObj);
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_STOPPED, 0);
				continue;
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_READY) {
				WavStud_NextRec(pActObj);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_BUF_READY2) {
				WavStud_NextRec(pActObj2);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_LB_BUF_READY) {
				WavStud_NextLB(pActObj_LB);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_TCHIT) {
				if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH) {
					set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_TCHIT);
				}
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_TCHIT, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_CMD_PAUSE) {
				WavStud_PauseRec(WAVSTUD_ACT_REC);
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_PAUSED, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_EVT_STREAMERR) {
				//WAVSTUD_REC_CB(FLG_ID_WAVSTUD_EVT_STREAMERR, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_CMD_RESTART) {
				WavStud_StopRec(pActObj);
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_STOPPED, 0);
				set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_CMD_START);
				DBG_WRN("Record task restarted\r\n");
			}
		}
		if (status & FLG_ID_WAVSTUD_STS_PAUSED) {
			if (uiFlag & FLG_ID_WAVSTUD_CMD_RESUME) {
				WavStud_ResumeRec(WAVSTUD_ACT_REC);
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_RESUMED, 0);
			}
			if (uiFlag & FLG_ID_WAVSTUD_CMD_STOP) {
				WavStud_StopRec(pActObj);
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_STOPPED, 0);
				continue;
			}
		}

		if ((uiFlag & FLG_ID_WAVSTUD_CMD_START) && (status & FLG_ID_WAVSTUD_STS_STOPPED)) {
			WAVSTUD_CB_EVT rv = WAVSTUD_CB_EVT_OK;
			rv = WavStud_StartRec(pActObj);

			if (WAVSTUD_CB_EVT_OK != rv) {
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_FAIL, 0);
				DBG_ERR("Start fail\r\n");
				continue;
			}

#if defined (__KERNEL__)
			if (!(kdrv_builtin_is_fastboot() && wavstudio_first_boot)) {
				//Make Aud-Buf-Que link-list
				if (WavStud_MakeAudBufQue(pActObj) == 0) {
					DBG_ERR("Make buffer queue error, Mem Not Enough\r\n");
					//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_FAIL, 0);
					continue;
				}
			}
#else
			//Make Aud-Buf-Que link-list
			if (WavStud_MakeAudBufQue(pActObj) == 0) {
				DBG_ERR("Make buffer queue error, Mem Not Enough\r\n");
				//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_FAIL, 0);
				continue;
			}
#endif

			if (WAVSTUD_MODE_2R == pActObj->mode) {
				if (WavStud_MakeAudBufQue(pActObj2) == 0) {
					DBG_ERR("Make buffer queue error, Mem Not Enough\r\n");
					//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_FAIL, 0);
					continue;
				}
			}

			#if WAVSTUD_FIX_BUF == ENABLE
#if defined (__KERNEL__)
			if (!(kdrv_builtin_is_fastboot() && wavstudio_first_boot)) {
				WavStud_InitQ(pActObj->actId);
			}
#else
			WavStud_InitQ(pActObj->actId);
#endif
			#endif

			//Init. AudDrv for Record
			WavStud_AudDrvInit(WAVSTUD_ACT_REC, &pActObj->audInfo, pActObj->audVol, 0);
			if (WAVSTUD_MODE_2R == pActObj->mode) {
				WavStud_AudDrvInit(WAVSTUD_ACT_REC2, &pActObj2->audInfo, pActObj2->audVol, 0);
			}

			if (gWavStudObj.aec_en) {
#if defined (__KERNEL__)
				if (!(kdrv_builtin_is_fastboot() && wavstudio_first_boot)) {
					if (WavStud_MakeAudBufQue(&(gWavStudObj.actObj[WAVSTUD_ACT_LB])) == 0) {
						DBG_ERR("Make buffer queue error, Mem Not Enough\r\n");
						//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_FAIL, 0);
						continue;
					}
					#if WAVSTUD_FIX_BUF == ENABLE
					WavStud_InitQ(WAVSTUD_ACT_LB);
					#endif
				}
#else
				if (WavStud_MakeAudBufQue(&(gWavStudObj.actObj[WAVSTUD_ACT_LB])) == 0) {
					DBG_ERR("Make buffer queue error, Mem Not Enough\r\n");
					//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_FAIL, 0);
					continue;
				}
				#if WAVSTUD_FIX_BUF == ENABLE
				WavStud_InitQ(WAVSTUD_ACT_LB);
				#endif
#endif
				WavStud_AudLBDrvInit(gWavStudObj.actObj[WAVSTUD_ACT_LB].audInfo.aud_sr, gWavStudObj.actObj[WAVSTUD_ACT_LB].audInfo.aud_ch, 0);
			}

			#if (0)
			WavStud_DmpModuleInfo();
			#endif

			//Add-Buf-Que
			if (gWavStudObj.aec_en) {
#if defined (__KERNEL__)
				if (kdrv_builtin_is_fastboot() && wavstudio_first_boot) {
					WAVSTUD_ACT actType = WAVSTUD_ACT_LB;

					audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_FIRST_QUEUE_TXLB, (UINT32*)&(pActObj_LB->pAudBufQueFirst));
					audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_TEMP_QUEUE_TXLB, (UINT32*)&(pActObj_LB->pAudBufQueTmp));

					pActObj_LB->lock_addr = pActObj_LB->pAudBufQueFirst->uiAddress;

					DBG_IND(":audSR=%d, actType=%d\r\n", pActObj_LB->audInfo.aud_sr, actType);
				} else {
					WavStud_TrigLBRec(&gWavStudObj.actObj[WAVSTUD_ACT_LB]);
					//WavStud_AudDrvLBRec();
				}
#else
				WavStud_TrigLBRec(&gWavStudObj.actObj[WAVSTUD_ACT_LB]);
				//WavStud_AudDrvLBRec();
#endif
			}

#if defined (__KERNEL__)
			if (kdrv_builtin_is_fastboot() && wavstudio_first_boot) {
				WAVSTUD_ACT actType = pActObj->actId;
				UINT32 cb_pointer;
				UINT32 eid = 0;

				audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_FIRST_QUEUE, (UINT32*)&(pActObj->pAudBufQueFirst));
				audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_TEMP_QUEUE, (UINT32*)&(pActObj->pAudBufQueTmp));

				pActObj->lock_addr = pActObj->pAudBufQueFirst->uiAddress;

				eid = WavStud_GetAudObj(actType);
				kdrv_audioio_get(eid, KDRV_AUDIOIO_GLOBAL_ISR_CB, &cb_pointer);
				audcap_builtin_set_param(BUILTIN_AUDCAP_PARAM_ISR_CB, &cb_pointer);

				cb_pointer = (UINT32)wavstud_trigger_cb;

				audcap_builtin_set_param(BUILTIN_AUDCAP_PARAM_TRIGGER_CB, &cb_pointer);

				DBG_IND(":audSR=%d, actType=%d\r\n", pActObj->audInfo.aud_sr, actType);
			} else {
				WavStud_TrigRec(pActObj);
			}
#else
			WavStud_TrigRec(pActObj);
#endif

			if (WAVSTUD_MODE_2R == pActObj->mode) {
				WavStud_TrigRec(pActObj2);
			}
			DBG_IND("[AudIn] SR:%d,CH:%d,CH num:%d,bps:%d,outDevice:%d\r\n", \
					 pActObj->audInfo.aud_sr, pActObj->audInfo.aud_ch, pActObj->audInfo.ch_num, pActObj->audInfo.bitpersample, gWavStudObj.playOutDev);

			//Start to record
			//WavStud_AudDrvRec();

			#if defined __UITRON || defined __ECOS
			#if WAVSTUD_FIX_BUF == DISABLE
			if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH) {
				timer_cfg(gWavStudTimerID, 1000000/gWavStudRecFrmRate, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
			}
			#endif
			#endif

			clr_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_STOPPED);
			set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_GOING);
			//WAVSTUD_REC_CB(WAVSTUD_CB_EVT_STARTED, 0);
		}
	}

	set_flg(FLG_ID_WAVSTUD_RECORD, FLG_ID_WAVSTUD_STS_IDLE);
	THREAD_RETURN(0);
}

THREAD_DECLARE(WavStudio_RecordWriteTsk, arglist)
{
	FLGPTN uiFlag = 0;
	PWAVSTUD_ACT_OBJ pActRecObj = &(gWavStudObj.actObj[WAVSTUD_ACT_REC]);
	PWAVSTUD_ACT_OBJ pActRecObj2 = &(gWavStudObj.actObj[WAVSTUD_ACT_REC2]);
	PAUDIO_BUF_QUEUE pAudBufQueDone;
	WAVSTUD_CB_EVT retV = WAVSTUD_CB_EVT_OK;
	//SEM_HANDLE SEM_ID_QUE = SEM_ID_WAVSTUD_RECORD_QUE;

	THREAD_ENTRY();

	while (1) {
		DBG_IND(":f=0x%x +\r\n", uiFlag);
		set_flg(FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_EVT_RWIDLE);
		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_EVT_RW | FLG_ID_WAVSTUD_EVT_RW2 | FLG_ID_WAVSTUD_EVT_CLOSE, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		clr_flg(FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_EVT_RWIDLE);

		DBG_IND(":f=0x%x -\r\n", uiFlag);

		if (uiFlag & FLG_ID_WAVSTUD_EVT_CLOSE) {
			break;
		}

		if (gWriteFail) {
			continue;
		}

		if (uiFlag & FLG_ID_WAVSTUD_EVT_RW) {
			while (pActRecObj->pAudBufQueTail->pNext != pActRecObj->pAudBufQueHead->pNext) {
				//DBG_DUMP("W1+\r\n");
				pAudBufQueDone = pActRecObj->pAudBufQueTail;
				retV = WavStud_RecWriteData(pActRecObj, pAudBufQueDone);
				//DBG_DUMP("W1-\r\n");
				SEM_WAIT(SEM_ID_WAVSTUD_RECORD_QUE);
				pActRecObj->pAudBufQueTail = pActRecObj->pAudBufQueTail->pNext;
				SEM_SIGNAL(SEM_ID_WAVSTUD_RECORD_QUE);
				if (retV != WAVSTUD_CB_EVT_OK) {
					gWriteFail = TRUE;
					break;
				}
			}
			if (retV != WAVSTUD_CB_EVT_OK) {
				continue;
			}
		}
		if (uiFlag & FLG_ID_WAVSTUD_EVT_RW2) {
			while (pActRecObj2->pAudBufQueTail->pNext != pActRecObj2->pAudBufQueHead->pNext) {
				//DBG_DUMP("W2+\r\n");
				pAudBufQueDone = pActRecObj2->pAudBufQueTail;
				retV = WavStud_RecWriteData(pActRecObj2, pAudBufQueDone);
				//DBG_DUMP("W2-\r\n");
				SEM_WAIT(SEM_ID_WAVSTUD_RECORD_QUE);
				pActRecObj2->pAudBufQueTail = pActRecObj2->pAudBufQueTail->pNext;
				SEM_SIGNAL(SEM_ID_WAVSTUD_RECORD_QUE);
				if (retV != WAVSTUD_CB_EVT_OK) {
					gWriteFail = TRUE;
					break;
				}
			}
			if (retV != WAVSTUD_CB_EVT_OK) {
				continue;
			}
		}
	}

	set_flg(FLG_ID_WAVSTUD_RECORDWRITE, FLG_ID_WAVSTUD_STS_IDLE);
	THREAD_RETURN(0);
}

THREAD_DECLARE(WavStudio_RecordUpdateTsk, arglist)
{
	FLGPTN uiFlag = 0;
	FLGPTN status = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[WAVSTUD_ACT_REC]);
	FLGPTN wait_flag = FLG_ID_WAVSTUD_EVT_TCHIT | FLG_ID_WAVSTUD_EVT_CLOSE | FLG_ID_WAVSTUD_EVT_BUF_FULL | FLG_ID_WAVSTUD_EVT_BUF_READY;

	THREAD_ENTRY();

	while (1) {
		DBG_IND(":f=0x%x+\r\n", uiFlag);
		set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_RWIDLE);
		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_WAVSTUD_RECORDUPDATE, wait_flag, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		clr_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_EVT_RWIDLE);
		status = kchk_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_STS);
		DBG_IND(":f=0x%x-\r\n", uiFlag);

		if (uiFlag & FLG_ID_WAVSTUD_EVT_CLOSE) {
			break;
		}

		if ((uiFlag & FLG_ID_WAVSTUD_EVT_TCHIT) && (status & FLG_ID_WAVSTUD_STS_GOING)) {
			if (pActObj->dataMode == WAVSTUD_DATA_MODE_PUSH) {
				WAVSTUD_REC_CB(WAVSTUD_CB_EVT_PROC_ENTER, 0);
				WAVSTUD_REC_CB(WAVSTUD_CB_EVT_PROC_OK, 0);

				#if WAVSTUD_FIX_BUF == ENABLE
				WavStud_RecPushBuf();
				#else
				WavStud_RecUpdateCurrBuf();
				#endif
			}
		}
		if ((uiFlag & FLG_ID_WAVSTUD_EVT_BUF_FULL) && (status & FLG_ID_WAVSTUD_STS_GOING)) {
			WAVSTUD_REC_CB(WAVSTUD_CB_EVT_NEW_ENTER, 0);
			WAVSTUD_REC_CB(WAVSTUD_CB_EVT_NEW_FAIL, 0);
		}

		if ((uiFlag & FLG_ID_WAVSTUD_EVT_BUF_READY) && (status & FLG_ID_WAVSTUD_STS_GOING)) {
			WAVSTUD_REC_CB(WAVSTUD_CB_EVT_NEW_ENTER, 0);
			WAVSTUD_REC_CB(WAVSTUD_CB_EVT_NEW_OK, 0);
		}

	}

	set_flg(FLG_ID_WAVSTUD_RECORDUPDATE, FLG_ID_WAVSTUD_STS_IDLE);
	THREAD_RETURN(0);
}

//@}

