/*
    @file       WavStudioUtil.c
    @ingroup    mILIBWAVSTUDIO

    @brief      Internal functions of WavStudio task.

                Internal functions of WavStudio task.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

/**
    @addtogroup mILIBWAVSTUDIO
*/
//@{

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
//#include "audio_common/include/Audio.h"
#include "wavstudio_int.h"
#include "wavstudio_aud_intf.h"
#include "wavstudio_id.h"

#if WAVSTUD_FIX_BUF == ENABLE
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)
#endif

extern WAVSTUD_OBJ gWavStudObj;
extern UINT32 g_uiWavStudProcSize;

UINT32 WavStud_GetUnitSize(AUDIO_SR audSR, UINT32 ch_num, WAVSTUD_BITS_PER_SAM bps, UINT32 sample_cnt)
{
	#if WAVSTUD_FIX_BUF == DISABLE
	UINT32 unitSize = audSR;
	#else
	UINT32 unitSize = sample_cnt;
	#endif

	/*if (ch_num == 2) {
		unitSize <<= 1;
	} else if (ch_num != 1) {
		DBG_ERR("invalid channel number=%d\r\n", ch_num);
	}*/

	if ((((ch_num%2) != 0) && (ch_num != 1)) || (ch_num == 0) ) {
		DBG_ERR("invalid channel number=%d\r\n", ch_num);
	} else if (ch_num != 1) {
		unitSize *= ch_num;
	}

	//if (WAVSTUD_BITS_PER_SAM_16 == bps)//skip for 8bit convert-to 16bit
	{
		unitSize <<= 1;
	}

	#if WAVSTUD_FIX_BUF == DISABLE
	unitSize = (unitSize / 10) * unitTime;
	#endif

	//make sure unitSize word alignment
	unitSize = ALIGN_CEIL_64(unitSize);
	DBG_IND("unitSize=%x\r\n", unitSize);

	return unitSize;
}

UINT32 WavStud_GetTmpSize(AUDIO_SR audSR, UINT32 ch_num, WAVSTUD_BITS_PER_SAM bps)
{
	UINT32 tmp_size = 0;

	tmp_size = ALIGN_FLOOR_64((audSR*ch_num*(bps>>3))/25);

	return tmp_size;
}

UINT32 WavStud_MakeAudBufQue(PWAVSTUD_ACT_OBJ pActObj)
{
	UINT32 unitCnt = 0, tmpSize = 0, tmpData = 0, unitSize = 0;
	PAUDIO_BUF_QUEUE pAudBufQue = 0;
	UINT32 sAddr = pActObj->mem.uiAddr + WAVSTUD_HEADER_BUF_SIZE;

	//Loopback channel does not need header buffer
	/*if (pActObj->actId == WAVSTUD_ACT_LB) {
		sAddr -= WAVSTUD_HEADER_BUF_SIZE;
	}*/

	unitCnt = pActObj->audBufQueCnt;

	#if WAVSTUD_FIX_BUF == ENABLE
	tmpSize = sizeof(BOOL);//done queue lock status
	pActObj->wavAudQue.pDoneQueLock = (BOOL*)sAddr;
	sAddr += tmpSize * unitCnt;

	tmpSize = sizeof(WAVSTUD_AUDQUE);//done queue
	pActObj->wavAudQue.pDoneQueue = (PWAVSTUD_AUDQUE)sAddr;
	sAddr += tmpSize * unitCnt;
	#endif

	pAudBufQue = (PAUDIO_BUF_QUEUE)sAddr;
	pActObj->pAudBufQueFirst = (PAUDIO_BUF_QUEUE)sAddr;
	tmpSize = pActObj->mem.uiAddr + pActObj->mem.uiSize - sAddr;
	unitSize = pActObj->unitSize;

	if (unitSize & 0x00000001) {
		unitSize *= 4;
	} else if (unitSize & 0x00000002) {
		unitSize *= 2;
	}

	if (unitCnt * unitSize > tmpSize) {
		DBG_ERR("Memory size must >= %d, but %d\r\n", unitCnt * unitSize, tmpSize);
		return 0;
	}

	DBG_IND("SR=%d,CH=%d,bps=%d,unitSize=%d\r\n", pActObj->audInfo.aud_sr, pActObj->audInfo.aud_ch, pActObj->audInfo.bitpersample, unitSize);

	tmpSize = sizeof(AUDIO_BUF_QUEUE);
	tmpData = sAddr + tmpSize * unitCnt + pActObj->unitSize;
	//make sure data-addr word alignment
	//tmpData = ALIGN_CEIL_4(tmpData);
	tmpData = ALIGN_CEIL_64(tmpData);

	DBG_IND("hdrAddr=0x%x, totalSize=0x%x, hdrSize=0x%x, BufQueAddr=0x%x, BufQueUnitSize=0x%x, DataAddr=0x%x\r\n", \
			pActObj->mem.uiAddr, pActObj->mem.uiSize, WAVSTUD_HEADER_BUF_SIZE, \
			sAddr, tmpSize, tmpData);
	DBG_IND("unitCnt=%d, unitSize=0x%x\r\n", unitCnt, unitSize);

	#if WAVSTUD_FIX_BUF == ENABLE
	{
		//reserve 1 buffer for temp buffer
		unitCnt--;
	}
	#endif

	while (--unitCnt) {
		pAudBufQue->uiAddress = tmpData;
		pAudBufQue->uiSize = unitSize;
		pAudBufQue->uiValidSize = 0;
		tmpData += unitSize;
		sAddr += tmpSize;
		pAudBufQue->pNext = (PAUDIO_BUF_QUEUE)sAddr;
		pAudBufQue = pAudBufQue->pNext;
	}
	pAudBufQue->uiAddress = tmpData;
	pAudBufQue->uiSize = unitSize;
	pAudBufQue->uiValidSize = 0;

	DBG_IND("Act=%d, last addr=%x, last size=%x, end=%x\r\n", pActObj->actId, pAudBufQue->uiAddress, pAudBufQue->uiSize, pAudBufQue->uiAddress+pAudBufQue->uiSize);

	pAudBufQue->pNext = pActObj->pAudBufQueFirst; //last link to the first

	#if WAVSTUD_FIX_BUF == ENABLE
	{
		sAddr += tmpSize;
		tmpData += unitSize;
		pAudBufQue = (PAUDIO_BUF_QUEUE)sAddr;
		pAudBufQue->uiAddress = tmpData;
		//pAudBufQue->uiSize = unitSize;
		//pAudBufQue->uiSize = (unitSize > empty_size)? empty_size : unitSize;
		pAudBufQue->uiSize = WavStud_GetTmpSize(pActObj->audInfo.aud_sr, pActObj->audInfo.ch_num, pActObj->audInfo.bitpersample);
		pAudBufQue->uiValidSize = 0;
		pAudBufQue->pNext = 0;
		pActObj->pAudBufQueTmp = pAudBufQue;
	}
	#endif

	return pActObj->audBufQueCnt;
}

UINT32 WavStud_BufToIdx(WAVSTUD_ACT actType, PAUDIO_BUF_QUEUE pAudCurBuf)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
	PAUDIO_BUF_QUEUE pAudFirstQue = pActObj->pAudBufQueFirst;
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueFirst;
	UINT32 i = 0;

	do {
		if (pAudBufQue->uiAddress == pAudCurBuf->uiAddress) {
			break;
		}

		pAudBufQue = pAudBufQue->pNext;
		i++;
	} while (pAudBufQue->uiAddress != pAudFirstQue->uiAddress);

	return i;
}

PAUDIO_BUF_QUEUE WavStud_IdxToBuf(WAVSTUD_ACT actType, UINT32 idx)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
	PAUDIO_BUF_QUEUE pAudBufQue = pActObj->pAudBufQueFirst;
	UINT32 i = 0;

	while (i < idx) {
		pAudBufQue = pAudBufQue->pNext;
		i++;
	}

	return pAudBufQue;
}

UINT32 WavStud_AddrToIdx(WAVSTUD_ACT actType, UINT32 addr)
{
	AUDIO_BUF_QUEUE que = {0};
	UINT32 idx = 0;

	que.uiAddress = addr;
	idx = WavStud_BufToIdx(actType, &que);

	return idx;
}

UINT64 wavstudio_do_div(UINT64 dividend, UINT64 divisor)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	dividend = (dividend / divisor);
#else
	do_div(dividend, divisor);
#endif
	return dividend;
}

#if WAVSTUD_FIX_BUF == ENABLE
BOOL WavStud_AddDoneQue(WAVSTUD_ACT actType, PAUDIO_BUF_QUEUE pAudQue)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
	WAVSTUD_AUDQ *pObj;
	unsigned long flags;
	//UINT32 num = 0;

	pObj = &(pActObj->wavAudQue);

	//SEM_WAIT(SEM_ID_WAVSTUD_LOCK);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		//DBG_ERR("[Record] Add Raw Queue is Full!\r\n");
		//num = WavStud_HowManyinQ(actType);
		//DBGD(num);
		//SEM_SIGNAL(SEM_ID_WAVSTUD_LOCK);
		return FALSE;
	} else {

		pObj->pDoneQueue[pObj->Rear].pAudQue = pAudQue;
		pObj->pDoneQueue[pObj->Rear].timestamp = hwclock_get_longcounter();

		loc_cpu(flags);
		pObj->Rear = (pObj->Rear + 1) % (pActObj->audBufQueCnt-1);

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		unl_cpu(flags);

		//SEM_SIGNAL(SEM_ID_WAVSTUD_LOCK);

		return TRUE;
	}
}

PWAVSTUD_AUDQUE WavStud_GetDoneQue(WAVSTUD_ACT actType)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
	WAVSTUD_AUDQ  *pObj;
	WAVSTUD_AUDQUE *pQue;
	unsigned long flags;

	pObj = &(pActObj->wavAudQue);

	//SEM_WAIT(SEM_ID_WAVSTUD_LOCK);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		//SEM_SIGNAL(SEM_ID_WAVSTUD_LOCK);
		DBG_IND("[Record] Get Raw Queue is Empty!\r\n");
		return 0;
	} else {
		pQue = &pObj->pDoneQueue[pObj->Front];

		pObj->Front = (pObj->Front + 1) % (pActObj->audBufQueCnt-1);
		loc_cpu(flags);
		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}
		unl_cpu(flags);
		//SEM_SIGNAL(SEM_ID_WAVSTUD_LOCK);
		return pQue;
	}
}

UINT32 WavStud_HowManyinQ(WAVSTUD_ACT actType)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
	UINT32 front, rear, full, sq = 0;
	WAVSTUD_AUDQ *pObj;
	unsigned long flags;

	//SEM_WAIT(SEM_ID_WAVSTUD_LOCK);

	loc_cpu(flags);
	pObj = &(pActObj->wavAudQue);
	front = pObj->Front;
	rear = pObj->Rear;
	full = pObj->bFull;
	unl_cpu(flags);
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = (pActObj->audBufQueCnt-1) - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = (pActObj->audBufQueCnt-1);
	} else {
		sq = 0;
	}

	//SEM_SIGNAL(SEM_ID_WAVSTUD_LOCK);

	return sq;
}


void WavStud_InitQ(WAVSTUD_ACT actType)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);

	memset(pActObj->wavAudQue.pDoneQueue, 0, sizeof(WAVSTUD_AUDQUE)*pActObj->audBufQueCnt);
	memset(pActObj->wavAudQue.pDoneQueLock, 0, sizeof(BOOL)*pActObj->audBufQueCnt);
	pActObj->wavAudQue.bFull = 0;
	pActObj->wavAudQue.Front= 0;
	pActObj->wavAudQue.Rear= 0;
}

void WavStud_LockQ(WAVSTUD_ACT actType, PAUDIO_BUF_QUEUE pAudCurBuf)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
	UINT32 idx = 0;
	unsigned long flags;

	idx = WavStud_BufToIdx(actType, pAudCurBuf);
	loc_cpu(flags);
	pActObj->wavAudQue.pDoneQueLock[idx] = TRUE;
	unl_cpu(flags);
}

void WavStud_UnlockQ(WAVSTUD_ACT actType, UINT32 Addr)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
	AUDIO_BUF_QUEUE que = {0};
	UINT32 idx = 0;
	unsigned long flags;

	que.uiAddress = Addr;
	idx = WavStud_BufToIdx(actType, &que);
	loc_cpu(flags);
	pActObj->wavAudQue.pDoneQueLock[idx] = FALSE;
	unl_cpu(flags);
}

BOOL WavStud_IsQLock(WAVSTUD_ACT actType, PAUDIO_BUF_QUEUE pAudCurBuf)
{
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
	UINT32 idx = 0;

	idx = WavStud_BufToIdx(actType, pAudCurBuf);

	return pActObj->wavAudQue.pDoneQueLock[idx];
}
#endif

//@}
