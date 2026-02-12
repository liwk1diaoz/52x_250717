/*
    Audio Queue Handler Driver

    This file is the driver of Audio Queue Handler.

    @file       audio_queue.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifdef __KERNEL__
#include "kwrap/type.h"//a header for basic variable type
#include <mach/rcw_macro.h>
#include <mach/fmem.h>
#elif defined(__FREERTOS)
#include "kwrap/error_no.h"
#include "kwrap/flag.h"
#endif
#include "kwrap/spinlock.h"
static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#include "audio_int.h"

/**
    @addtogroup mISYSAud
*/
//@{

AUDIO_BUF_QUEUE  AudioRealBufferQueue[AUDTSCH_TOT][AUDIO_BUF_BLKNUM];

PAUDIO_BUF_QUEUE pAudioRealBufferFront[AUDTSCH_TOT]   = {NULL, NULL, NULL, NULL};
PAUDIO_BUF_QUEUE pAudioRealBufferRear[AUDTSCH_TOT]    = {NULL, NULL, NULL, NULL};
PAUDIO_BUF_QUEUE pAudioRealBufferDone[AUDTSCH_TOT]    = {NULL, NULL, NULL, NULL};

AUDIO_BUF_QUEUE  AudioPseudoBufferQueue[AUDTSCH_TOT][AUDIO_BUF_BLKNUM];

PAUDIO_BUF_QUEUE pAudioPseudoBufferFront[AUDTSCH_TOT] = {NULL, NULL, NULL, NULL};
PAUDIO_BUF_QUEUE pAudioPseudoBufferRear[AUDTSCH_TOT]  = {NULL, NULL, NULL, NULL};
PAUDIO_BUF_QUEUE pAudioPseudoBufferDone[AUDTSCH_TOT]  = {NULL, NULL, NULL, NULL};

extern ER               dai_lock(void);
extern ER               dai_unlock(void);


/*
    internal API for audio queue initialization
*/
void aud_init_queue(void)
{
	UINT32 i, j;
	unsigned long flag;

	loc_cpu(flag);

	for (j = 0; j < AUDTSCH_TOT; j++) {
		// Initialize buffer queue
		for (i = 0; i < AUDIO_BUF_BLKNUM; i++) {
			AudioRealBufferQueue[j][i].pNext =   &AudioRealBufferQueue[j][(i + 1) % AUDIO_BUF_BLKNUM];
			AudioPseudoBufferQueue[j][i].pNext = &AudioPseudoBufferQueue[j][(i + 1) % AUDIO_BUF_BLKNUM];
		}
	}

	unl_cpu(flag);
}

#if 1
/*
    Reset Specified audio channel queue
*/
ER aud_reset_queue(AUDTSCH tsCH)
{
	unsigned long flag;
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: CH(%d)\r\n", __func__, (int)tsCH);
	}

	loc_cpu(flag);

	pAudioRealBufferFront[tsCH]   =
		pAudioRealBufferRear[tsCH]    =
			pAudioRealBufferDone[tsCH]    = &AudioRealBufferQueue[tsCH][0];

	pAudioPseudoBufferFront[tsCH] =
		pAudioPseudoBufferRear[tsCH]  =
			pAudioPseudoBufferDone[tsCH]  = &AudioPseudoBufferQueue[tsCH][0];

	unl_cpu(flag);

	return E_OK;
}

/*
    Reset Specified audio channel queue
*/
ER aud_is_queue_full(AUDTSCH tsCH)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return FALSE;
	}

	return (BOOL)(pAudioRealBufferFront[tsCH]->pNext == pAudioRealBufferRear[tsCH]);
}

/*
    Add Audio Buffer to Queue
*/
ER aud_add_to_queue(AUDTSCH tsCH, PAUDIO_BUF_QUEUE pAudioBufQueue)
{
	unsigned long flag;
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return FALSE;
	}

	loc_cpu(flag);

	if ((pAudioRealBufferFront[tsCH] == NULL) || (pAudioPseudoBufferFront[tsCH] == NULL)) {
		unl_cpu(flag);
		DBG_ERR("aud_reset_queue() not invoked yet\r\n");
		return FALSE;
	}

	if (pAudioRealBufferFront[tsCH]->pNext == NULL) {
		unl_cpu(flag);
		DBG_ERR("aud_init() not invoked yet\r\n");
		return FALSE;
	}

	// Queue is Full
	if (pAudioRealBufferFront[tsCH]->pNext == pAudioRealBufferRear[tsCH]) {
		unl_cpu(flag);
		return FALSE;
	}

	// For pseudo operation using(upper layer using)
	pAudioPseudoBufferFront[tsCH]->uiAddress    = pAudioBufQueue->uiAddress;
	pAudioPseudoBufferFront[tsCH]->uiSize       = pAudioBufQueue->uiSize;
	pAudioPseudoBufferFront[tsCH]               = pAudioPseudoBufferFront[tsCH]->pNext;

	unl_cpu(flag);
	return TRUE;
}


/*
    Get Audio Done Buffer From Queue
*/
PAUDIO_BUF_QUEUE aud_get_done_queue(AUDTSCH tsCH)
{
	PAUDIO_BUF_QUEUE    pRetQueue;
	unsigned long flag;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return NULL;
	}

	if (pAudioRealBufferDone[tsCH] == NULL) {
		return NULL;
	}

	loc_cpu(flag);

	// Done Queue is empty
	if (pAudioRealBufferDone[tsCH] == pAudioRealBufferRear[tsCH]) {
		// Audio is NOT stopped
		if ((tsCH == TSTX1) && (dai_is_tx_dma_enable(DAI_TXCH_TX1) == TRUE)) {
			unl_cpu(flag);
			return NULL;
		} else if ((tsCH == TSTX2) && (dai_is_tx_dma_enable(DAI_TXCH_TX2) == TRUE)) {
			unl_cpu(flag);
			return NULL;
		} else if (((tsCH == TSRX1) || (tsCH == TSRX2)) && (dai_is_rx_dma_enable() == TRUE)) {
			unl_cpu(flag);
			return NULL;
		}
		// Audio is stopped
		else {
			pRetQueue               = pAudioPseudoBufferDone[tsCH];
			pAudioPseudoBufferDone[tsCH]  = NULL;
			pAudioRealBufferDone[tsCH]    = NULL;
		}
	} else {
		pRetQueue                   = pAudioPseudoBufferDone[tsCH];
		// Move done queue to next
		pAudioPseudoBufferDone[tsCH]      = pAudioPseudoBufferDone[tsCH]->pNext;
		pAudioRealBufferDone[tsCH]        = pAudioRealBufferDone[tsCH]->pNext;
	}
#ifdef __KERNEL__
	fmem_dcache_sync((void *)pRetQueue->uiAddress, pRetQueue->uiValidSize, DMA_BIDIRECTIONAL);
#elif defined(__FREERTOS)
    dma_flushReadCache(pRetQueue->uiAddress, pRetQueue->uiValidSize);
#endif

	unl_cpu(flag);

	// Return the queue
	return pRetQueue;
}


/*
    Get Current buffer from Audio buffer queue
*/
PAUDIO_BUF_QUEUE aud_get_cur_queue(AUDTSCH tsCH)
{
	unsigned long flag;
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return NULL;
	}

	loc_cpu(flag);

	if (tsCH == TSRX1) {
		pAudioPseudoBufferRear[tsCH]->uiValidSize =
			pAudioRealBufferRear[tsCH]->uiValidSize   = dai_get_rx_dma_curaddr(0) - pAudioRealBufferRear[tsCH]->uiAddress;
	} else if (tsCH == TSRX2) {
		pAudioPseudoBufferRear[tsCH]->uiValidSize =
			pAudioRealBufferRear[tsCH]->uiValidSize   = dai_get_rx_dma_curaddr(1) - pAudioRealBufferRear[tsCH]->uiAddress;
	} else if (tsCH == TSTXLB) {
		pAudioPseudoBufferRear[tsCH]->uiValidSize =
			pAudioRealBufferRear[tsCH]->uiValidSize   = dai_get_txlb_dma_curaddr() - pAudioRealBufferRear[tsCH]->uiAddress;
	} else if (tsCH == TSTX1) {
		pAudioPseudoBufferRear[tsCH]->uiValidSize =
			pAudioRealBufferRear[tsCH]->uiValidSize   = dai_get_tx_dma_curaddr(0) - pAudioRealBufferRear[tsCH]->uiAddress;
	} else if (tsCH == TSTX2) {
		pAudioPseudoBufferRear[tsCH]->uiValidSize =
			pAudioRealBufferRear[tsCH]->uiValidSize   = dai_get_tx_dma_curaddr(1) - pAudioRealBufferRear[tsCH]->uiAddress;
	}

	unl_cpu(flag);

	return pAudioPseudoBufferRear[tsCH];
}

#endif

//@}

