#if defined(__KERNEL__)
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include "kwrap/debug.h"
#include "kwrap/util.h"
#include "kwrap/spinlock.h"
#include "kwrap/perf.h"
#include "audcap_builtin.h"
#include "dai.h"
#include "eac.h"
#include "audcap_builtin_platform.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

#define AUDIO_RX_FLAGALL                (DAI_RXSTOP_INT|DAI_RX1BWERR_INT|DAI_RX2BWERR_INT|DAI_RX1DMADONE_INT|DAI_RX2DMADONE_INT|DAI_RXTCHIT_INT|DAI_RXTCLATCH_INT|DAI_RXDMALOAD_INT)
#define WAV_HEADER_BUF_SIZE         0x00000200      ///<  512 Bytes
#define AUDIO_IIR_COEF(x) ((INT32)(.5+(x)*(((INT32)1)<<(15))))

typedef enum {
	AUDIO_CH_LEFT,              ///< Left
	AUDIO_CH_RIGHT,             ///< Right
	AUDIO_CH_STEREO,            ///< Stereo
	AUDIO_CH_MONO,              ///< Mono two channel. Obselete. Shall not use this option.
	AUDIO_CH_DUAL_MONO,         ///< Dual Mono Channels. Valid for record(RX) only.

	ENUM_DUMMY4WORD(AUDIO_CH)
} AUDIO_CH;

typedef enum {
	AUDIO_SR_8000   = 8000,     ///< 8 KHz
	AUDIO_SR_11025  = 11025,    ///< 11.025 KHz
	AUDIO_SR_12000  = 12000,    ///< 12 KHz
	AUDIO_SR_16000  = 16000,    ///< 16 KHz
	AUDIO_SR_22050  = 22050,    ///< 22.05 KHz
	AUDIO_SR_24000  = 24000,    ///< 24 KHz
	AUDIO_SR_32000  = 32000,    ///< 32 KHz
	AUDIO_SR_44100  = 44100,    ///< 44.1 KHz
	AUDIO_SR_48000  = 48000,    ///< 48 KHz

	ENUM_DUMMY4WORD(AUDIO_SR)
} AUDIO_SR;

typedef enum {
	AUDIO_GAIN_MUTE,            ///< Gain mute
	AUDIO_GAIN_0,               ///< Gain 0. (ALC target level to -27.0 dBFS)
	AUDIO_GAIN_1,               ///< Gain 1. (ALC target level to -24.0 dBFS)
	AUDIO_GAIN_2,               ///< Gain 2. (ALC target level to -21.0 dBFS) (Default)
	AUDIO_GAIN_3,               ///< Gain 3. (ALC target level to -18.0 dBFS)
	AUDIO_GAIN_4,               ///< Gain 4. (ALC target level to -15.0 dBFS)
	AUDIO_GAIN_5,               ///< Gain 5. (ALC target level to -12.0 dBFS)
	AUDIO_GAIN_6,               ///< Gain 6. (ALC target level to -9.0 dBFS)
	AUDIO_GAIN_7,               ///< Gain 7. (ALC target level to -6.0 dBFS)

	ENUM_DUMMY4WORD(AUDIO_GAIN)
} AUDIO_GAIN;

typedef struct _AUDIO_BUF_QUEUE {
	UINT32                  uiAddress;  ///< Buffer Starting Address (Unit: byte) (Word alignment)
	UINT32                  uiSize;     ///< Buffer Size (Unit: byte) (Word Alignment)
	UINT32                  uiValidSize;///< Valid PCM data size (Unit: byte).
										///< Returend by aud_getDoneBufferFromQueue()
	struct _AUDIO_BUF_QUEUE *pNext;     ///< Next queue element
} AUDIO_BUF_QUEUE, *PAUDIO_BUF_QUEUE;

typedef struct _AUDCAP_BUILTIN_AUDQUE {
	AUDIO_BUF_QUEUE    *pAudQue;
	UINT64             timestamp;
} AUDCAP_BUILTIN_AUDQUE, *PAUDCAP_BUILTIN_AUDQUE;

typedef struct {
	UINT32                          Front;              ///< Front pointer
	UINT32                          Rear;               ///< Rear pointer
	UINT32                          bFull;              ///< Full flag
	BOOL                           *pDoneQueLock;
	PAUDCAP_BUILTIN_AUDQUE          pDoneQueue;
} AUDCAP_BUILTIN_AUDQ, *PAUDCAP_BUILTIN_AUDQ;

typedef struct _AUDCAP_BUILTIN_INFO {
	UINT32              mem_addr;
	UINT32              mem_size;
	UINT32              aud_sr;                ///< Sample Rate
	UINT32              aud_ch;                ///< Channel
	UINT32              aud_bitpersample;      ///< bit Per Sample
	UINT32              aud_ch_num;			  ///< Channel number
	UINT32              aud_vol;
	UINT32              aud_que_cnt;
	UINT32              aud_buf_sample_cnt;    ///< buffer sample count
	UINT32              unitSize;
	PAUDIO_BUF_QUEUE    pAudBufQueFirst;
	PAUDIO_BUF_QUEUE    pAudBufQueNext;
	PAUDIO_BUF_QUEUE    pAudBufQueTmp; //temp queue for no idle buffer
	PAUDIO_BUF_QUEUE    pAudBufQueHead;
	PAUDIO_BUF_QUEUE    pAudBufQueTail;
	UINT32              procUnitSize;
	UINT32              lock_addr;
	AUDCAP_BUILTIN_AUDQ wavAudQue;
	DRV_CB              p_isr_handler;
	UINT32              rec_src;
	UINT32              init;
	AUDCAP_BUILTIN_TRIGGER_CB p_trigger_cb;
#if AUDCAP_BUILTIN_LOOPBACK
	BOOL                aec_en;
	UINT32              aud_txch;                ///< Channel
	UINT32              aud_txch_num;			  ///< Channel number
#endif
	UINT32              default_setting;     ///< default setting
} AUDCAP_BUILTIN_INFO, *PAUDCAP_BUILTIN_INFO;


// Hidden API from DAI
extern DAI_INTERRUPT        dai_wait_interrupt(DAI_INTERRUPT WaitedFlag);
extern void nvt_bootts_add_ts(char *name);

static UINT32 count = 0;
static AUDCAP_BUILTIN_INFO audcap_info = {0};

static BOOL _audcap_builtin_add_buf(UINT32 addr, UINT32 size);

#if AUDCAP_BUILTIN_LOOPBACK
static AUDCAP_BUILTIN_INFO audcap_info_txlb = {0};

static BOOL _audcap_builtin_add_buf_txlb(UINT32 addr, UINT32 size);
#endif

#if 0
static void _audcap_builtin_dump_info(PAUDCAP_BUILTIN_INFO info)
{
	PAUDIO_BUF_QUEUE pAudBufQue;
	UINT32 j = 0;

	DBG_DUMP("  uiAddr    =0x%x\r\n", info->mem_addr);
	DBG_DUMP("  uiSize    =0x%x\r\n", info->mem_size);
	DBG_DUMP("  audSR     =%d\r\n", info->aud_sr);
	DBG_DUMP("  audCh     =%d\r\n", info->aud_ch);
	DBG_DUMP("  ch_num    =%d\r\n", info->aud_ch_num);
	DBG_DUMP("  bitPerSample =%d\r\n", info->aud_bitpersample);
	DBG_DUMP("  audVol    =%d\r\n", info->aud_vol);
	DBG_DUMP("  unitSize    =0x%x\r\n", info->unitSize);
	DBG_DUMP("pAudBufQueNext= 0x%x, 0x%x, 0x%x\r\n", (UINT32)info->pAudBufQueNext, info->pAudBufQueNext->uiAddress, info->pAudBufQueNext->uiSize);
	DBG_DUMP("pAudBufQueFirst= 0x%x, 0x%x, 0x%x\r\n", (UINT32)info->pAudBufQueFirst, info->pAudBufQueFirst->uiAddress, info->pAudBufQueFirst->uiSize);
	pAudBufQue = info->pAudBufQueFirst;
	for (j = 0; j < (info->aud_que_cnt - 1); j++) {
		if (pAudBufQue) {
			DBG_DUMP("AudBufQue[%d]=0x%x, 0x%x, 0x%x\r\n", j, (UINT32)pAudBufQue, pAudBufQue->uiAddress, pAudBufQue->uiSize);
			pAudBufQue = pAudBufQue->pNext;
		} else {
			DBG_ERR("idx=%d NULL over %d\r\n", j, info->aud_que_cnt);
		}
	}

	DBG_DUMP("AudTmpBufQue= 0x%x, 0x%x, 0x%x\r\n", (UINT32)info->pAudBufQueTmp, info->pAudBufQueTmp->uiAddress, info->pAudBufQueTmp->uiSize);
}
#endif

static UINT32 _audcap_builtin_buf_to_idx(PAUDCAP_BUILTIN_INFO info, PAUDIO_BUF_QUEUE pAudCurBuf)
{
	PAUDIO_BUF_QUEUE pAudFirstQue = info->pAudBufQueFirst;
	PAUDIO_BUF_QUEUE pAudBufQue = info->pAudBufQueFirst;
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

PAUDIO_BUF_QUEUE _audcap_builtin_idx_to_buf(PAUDCAP_BUILTIN_INFO info, UINT32 idx)
{
	PAUDIO_BUF_QUEUE pAudBufQue = info->pAudBufQueFirst;
	UINT32 i = 0;

	while (i < idx) {
		pAudBufQue = pAudBufQue->pNext;
		i++;
	}

	return pAudBufQue;
}

static void _audcap_builtin_lock_que(PAUDCAP_BUILTIN_INFO info, PAUDIO_BUF_QUEUE pAudCurBuf)
{
	UINT32 idx = 0;
	unsigned long flags;

	idx = _audcap_builtin_buf_to_idx(info, pAudCurBuf);
	loc_cpu(flags);
	info->wavAudQue.pDoneQueLock[idx] = TRUE;
	unl_cpu(flags);
}

static BOOL _audcap_builtin_add_done_que(PAUDCAP_BUILTIN_INFO info, PAUDIO_BUF_QUEUE pAudQue)
{
	AUDCAP_BUILTIN_AUDQ *pObj;
	unsigned long flags;
	VOS_TICK timestamp;
	//UINT32 num = 0;

	pObj = &(info->wavAudQue);

	//SEM_WAIT(SEM_ID_WAVSTUD_LOCK);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		//DBG_ERR("[Record] Add Raw Queue is Full!\r\n");
		return FALSE;
	} else {

		pObj->pDoneQueue[pObj->Rear].pAudQue = pAudQue;
		//pObj->pDoneQueue[pObj->Rear].timestamp = hwclock_get_longcounter();
		vos_perf_mark(&timestamp);
		pObj->pDoneQueue[pObj->Rear].timestamp = (UINT64)timestamp;

		loc_cpu(flags);
		pObj->Rear = (pObj->Rear + 1) % (info->aud_que_cnt-1);

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		unl_cpu(flags);

		_audcap_builtin_lock_que(info, pAudQue);
		//SEM_SIGNAL(SEM_ID_WAVSTUD_LOCK);

		return TRUE;
	}
}

static void _audcap_builtin_init_que(void)
{
	memset(audcap_info.wavAudQue.pDoneQueue, 0, sizeof(AUDCAP_BUILTIN_AUDQUE)*audcap_info.aud_que_cnt);
	memset(audcap_info.wavAudQue.pDoneQueLock, 0, sizeof(BOOL)*audcap_info.aud_que_cnt);
	audcap_info.wavAudQue.bFull = 0;
	audcap_info.wavAudQue.Front= 0;
	audcap_info.wavAudQue.Rear= 0;
#if AUDCAP_BUILTIN_LOOPBACK
	if (audcap_info.aec_en) {
		memset(audcap_info_txlb.wavAudQue.pDoneQueue, 0, sizeof(AUDCAP_BUILTIN_AUDQUE)*audcap_info_txlb.aud_que_cnt);
		memset(audcap_info_txlb.wavAudQue.pDoneQueLock, 0, sizeof(BOOL)*audcap_info_txlb.aud_que_cnt);
		audcap_info_txlb.wavAudQue.bFull = 0;
		audcap_info_txlb.wavAudQue.Front= 0;
		audcap_info_txlb.wavAudQue.Rear= 0;
	}
#endif
}

#if AUDCAP_BUILTIN_LOOPBACK
static void _audcap_builtin_reset_que(void)
{
	audcap_info.pAudBufQueHead = audcap_info.pAudBufQueFirst;
	audcap_info.pAudBufQueTail = audcap_info.pAudBufQueFirst;
	audcap_info.pAudBufQueNext = audcap_info.pAudBufQueFirst;
	if (audcap_info.aec_en) {
		audcap_info_txlb.pAudBufQueHead = audcap_info_txlb.pAudBufQueFirst;
		audcap_info_txlb.pAudBufQueTail = audcap_info_txlb.pAudBufQueFirst;
		audcap_info_txlb.pAudBufQueNext = audcap_info_txlb.pAudBufQueFirst;
	}
}
#endif

static UINT32 _audcap_builtin_get_unit_size(AUDIO_SR audSR, UINT32 ch_num, UINT32 bps, UINT32 sample_cnt)
{
	UINT32 unitSize = sample_cnt;

	if (ch_num == 2) {
		unitSize <<= 1;
	} else if (ch_num != 1) {
		DBG_ERR("invalid channel number=%d\r\n", ch_num);
	}

	unitSize <<= 1;

	//make sure unitSize word alignment
	unitSize = ALIGN_CEIL_64(unitSize);
	DBG_IND("unitSize=%x\r\n", unitSize);

	return unitSize;
}

static UINT32 _audcap_builtin_get_tmp_size(AUDIO_SR audSR, UINT32 ch_num, UINT32 bps){
	UINT32 tmp_size = 0;

	tmp_size = ALIGN_FLOOR_64((audSR*ch_num*(bps>>3))/25);

	return tmp_size;
}

static UINT32 _audcap_builtin_make_audque(PAUDCAP_BUILTIN_INFO info)
{
	UINT32 unitCnt = 0, tmpSize = 0, tmpData = 0, unitSize = 0;
	PAUDIO_BUF_QUEUE pAudBufQue = 0;
	UINT32 sAddr = info->mem_addr + WAV_HEADER_BUF_SIZE;

	unitCnt = info->aud_que_cnt;

	tmpSize = sizeof(BOOL);//done queue lock status
	info->wavAudQue.pDoneQueLock = (BOOL*)sAddr;
	sAddr += tmpSize * unitCnt;

	tmpSize = sizeof(AUDCAP_BUILTIN_AUDQUE);//done queue
	info->wavAudQue.pDoneQueue = (PAUDCAP_BUILTIN_AUDQUE)sAddr;
	sAddr += tmpSize * unitCnt;

	pAudBufQue = (PAUDIO_BUF_QUEUE)sAddr;
	info->pAudBufQueFirst = (PAUDIO_BUF_QUEUE)sAddr;
	tmpSize = info->mem_addr + info->mem_size - sAddr;
	unitSize = info->unitSize;

	if (unitSize & 0x00000001) {
		unitSize *= 4;
	} else if (unitSize & 0x00000002) {
		unitSize *= 2;
	}

	if (unitCnt * unitSize > tmpSize) {
		DBG_ERR("Memory size must >= %d, but %d\r\n", unitCnt * unitSize, tmpSize);
		return 0;
	}

	DBG_IND("SR=%d,CH=%d,bps=%d,unitSize=%d\r\n", info->aud_sr, info->aud_ch, info->aud_bitpersample, unitSize);

	tmpSize = sizeof(AUDIO_BUF_QUEUE);
	tmpData = sAddr + tmpSize * unitCnt + info->unitSize;
	tmpData = ALIGN_CEIL_64(tmpData);

	DBG_IND("hdrAddr=0x%x, totalSize=0x%x, hdrSize=0x%x, BufQueAddr=0x%x, BufQueUnitSize=0x%x, DataAddr=0x%x\r\n", \
			info->mem_addr, info->mem_size, WAV_HEADER_BUF_SIZE, \
			sAddr, tmpSize, tmpData);
	DBG_IND("unitCnt=%d, unitSize=0x%x\r\n", unitCnt, unitSize);

	//reserve 1 buffer for temp buffer
	unitCnt--;


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

	pAudBufQue->pNext = info->pAudBufQueFirst; //last link to the first

	{
		//UINT32 empty_size = ALIGN_FLOOR_64((info->aud_sr/30)*info->aud_ch_num*(info->aud_bitpersample>>3));

		sAddr += tmpSize;
		tmpData += unitSize;
		pAudBufQue = (PAUDIO_BUF_QUEUE)sAddr;
		pAudBufQue->uiAddress = tmpData;
		//pAudBufQue->uiSize = unitSize;
		//pAudBufQue->uiSize = (unitSize > empty_size)? empty_size : unitSize;
		pAudBufQue->uiSize = _audcap_builtin_get_tmp_size(info->aud_sr, info->aud_ch_num, info->aud_bitpersample);
		pAudBufQue->uiValidSize = 0;
		pAudBufQue->pNext = 0;
		info->pAudBufQueTmp = pAudBufQue;
	}

	return info->aud_que_cnt;
}

static void audcap_builtin_cb(UINT32 uiEventID)
{
	PAUDIO_BUF_QUEUE pAudBufQueDone = 0;
	PAUDIO_BUF_QUEUE pAudBufQue = 0;
	UINT32 idx = 0;
	UINT32 valid_size = 0;
	unsigned long flags;

#if AUDCAP_BUILTIN_LOOPBACK
	// TXLB DMA done interrupt
	if (uiEventID & DAI_TXLBDMADONE_INT) {

	if ((audcap_info.aud_que_cnt > 5 && (count >= audcap_info.aud_que_cnt - 5))) {
		// do nothing
	} else {

		pAudBufQueDone = audcap_info_txlb.pAudBufQueTail;

		loc_cpu(flags);

		idx = _audcap_builtin_buf_to_idx(&audcap_info_txlb, audcap_info_txlb.pAudBufQueNext);
		pAudBufQue = _audcap_builtin_idx_to_buf(&audcap_info_txlb, idx);

		if (pAudBufQueDone->uiAddress != audcap_info_txlb.pAudBufQueTmp->uiAddress) {
			idx = _audcap_builtin_buf_to_idx(&audcap_info_txlb, pAudBufQueDone);
			pAudBufQueDone = _audcap_builtin_idx_to_buf(&audcap_info_txlb, idx);
			valid_size = pAudBufQueDone->uiSize;
		}
		unl_cpu(flags);

		if (0) {
			//Next buffer is still locked. Add temp buffer.
			_audcap_builtin_add_buf_txlb(audcap_info_txlb.pAudBufQueTmp->uiAddress, audcap_info_txlb.pAudBufQueTmp->uiSize);
		} else {
			pAudBufQue->uiValidSize = 0;
			_audcap_builtin_add_buf_txlb(audcap_info_txlb.pAudBufQueNext->uiAddress, audcap_info_txlb.pAudBufQueNext->uiSize);
			audcap_info_txlb.pAudBufQueNext = pAudBufQue->pNext;
		}

		if (pAudBufQueDone->uiAddress != audcap_info_txlb.pAudBufQueTmp->uiAddress) {

			pAudBufQueDone = _audcap_builtin_idx_to_buf(&audcap_info_txlb, idx);
			pAudBufQueDone->uiValidSize = valid_size;

			if (!_audcap_builtin_add_done_que(&audcap_info_txlb, pAudBufQueDone)) {
				//DBG_WRN("Output queue is full. Drop buffer!\r\n");
			}

			audcap_info_txlb.pAudBufQueTail = audcap_info_txlb.pAudBufQueTail->pNext;
		}

	}

	}
#endif

	// RX1 DMA done interrupt
	if (uiEventID & DAI_RX1DMADONE_INT) {

		if (count == 0) {
			nvt_bootts_add_ts("ACAP"); //end
		}

		pAudBufQueDone = audcap_info.pAudBufQueTail;

		if ((audcap_info.aud_que_cnt <= 5) || (audcap_info.aud_que_cnt > 5 && (count >= audcap_info.aud_que_cnt - 5))) {
			//change DAI callback
			if (audcap_info.p_isr_handler != NULL) {
				if (audcap_info.p_trigger_cb != NULL) {
					audcap_info.p_trigger_cb(pAudBufQueDone->uiAddress);
					dai_set_config(DAI_CONFIG_ID_ISRCB, (UINT32)(audcap_info.p_isr_handler));
				}
			}
			return;
		}

		loc_cpu(flags);

		idx = _audcap_builtin_buf_to_idx(&audcap_info, audcap_info.pAudBufQueNext);
		pAudBufQue = _audcap_builtin_idx_to_buf(&audcap_info, idx);

		if (pAudBufQueDone->uiAddress != audcap_info.pAudBufQueTmp->uiAddress) {
			idx = _audcap_builtin_buf_to_idx(&audcap_info, pAudBufQueDone);
			pAudBufQueDone = _audcap_builtin_idx_to_buf(&audcap_info, idx);
			valid_size = pAudBufQueDone->uiSize;
		}
		unl_cpu(flags);

		if (0) {
			//Next buffer is still locked. Add temp buffer.
			_audcap_builtin_add_buf(audcap_info.pAudBufQueTmp->uiAddress, audcap_info.pAudBufQueTmp->uiSize);
		} else {
			pAudBufQue->uiValidSize = 0;
			_audcap_builtin_add_buf(audcap_info.pAudBufQueNext->uiAddress, audcap_info.pAudBufQueNext->uiSize);
			audcap_info.pAudBufQueNext = pAudBufQue->pNext;
		}

		if (pAudBufQueDone->uiAddress != audcap_info.pAudBufQueTmp->uiAddress) {
			pAudBufQueDone->uiValidSize = valid_size;

			if (_audcap_builtin_add_done_que(&audcap_info, pAudBufQueDone)) {
			} else {
				//DBG_WRN("Output queue is full. Drop buffer!\r\n");
			}

			audcap_info.pAudBufQueTail = audcap_info.pAudBufQueTail->pNext;
		}

		count++;

		if (audcap_info.p_isr_handler != NULL) {
			if (audcap_info.p_trigger_cb != NULL) {
				audcap_info.p_trigger_cb(pAudBufQueDone->uiAddress);
				dai_set_config(DAI_CONFIG_ID_ISRCB, (UINT32)(audcap_info.p_isr_handler));
			}
		}
	}

}

UINT32 audcap_builtin_set_param(UINT32 Param, UINT32 *pValue)
{
	switch (Param) {
	case BUILTIN_AUDCAP_PARAM_ISR_CB:
		audcap_info.p_isr_handler = (DRV_CB)*pValue;
		break;
	case BUILTIN_AUDCAP_PARAM_TRIGGER_CB:
		audcap_info.p_trigger_cb = (AUDCAP_BUILTIN_TRIGGER_CB)*pValue;
		break;
	default:
		DBG_ERR("[AUDCAPBUILTIN] Set invalid param = %d\r\n", Param);
		return -1;
	}
	return 0;
}

UINT32 audcap_builtin_get_param(UINT32 Param, UINT32 *pValue)
{
	switch (Param) {
	#if 0
	case BUILTIN_AUDCAP_PARAM_INFO: {
		AUDCAP_BUILTIN_INIT_INFO* init_info = (AUDCAP_BUILTIN_INIT_INFO*)pValue;

		init_info->aud_sr = audcap_info.aud_sr;
		init_info->aud_ch = audcap_info.aud_ch;
		init_info->aud_bitpersample = audcap_info.aud_bitpersample;
		init_info->aud_ch_num = audcap_info.aud_ch_num;
		init_info->aud_vol = audcap_info.aud_vol;
		init_info->aud_que_cnt = audcap_info.aud_que_cnt;
		init_info->aud_buf_sample_cnt = audcap_info.aud_buf_sample_cnt;
		init_info->ctrl_blk_addr = audcap_info.mem_addr;
		init_info->ctrl_blk_size = audcap_info.mem_size;
		break;
	}
	#endif
	case BUILTIN_AUDCAP_PARAM_FIRST_QUEUE: {
		*pValue = (UINT32)audcap_info.pAudBufQueFirst;
		break;
	}
	case BUILTIN_AUDCAP_PARAM_TEMP_QUEUE: {
		*pValue = (UINT32)audcap_info.pAudBufQueTmp;
		break;
	}
	case BUILTIN_AUDCAP_PARAM_DONE_QUEUE:
		memcpy((AUDCAP_BUILTIN_AUDQ *)pValue, &audcap_info.wavAudQue, sizeof(AUDCAP_BUILTIN_AUDQ));
		break;
#if AUDCAP_BUILTIN_LOOPBACK
	case BUILTIN_AUDCAP_PARAM_FIRST_QUEUE_TXLB: {
		*pValue = (UINT32)audcap_info_txlb.pAudBufQueFirst;
		break;
	}
	case BUILTIN_AUDCAP_PARAM_TEMP_QUEUE_TXLB: {
		*pValue = (UINT32)audcap_info_txlb.pAudBufQueTmp;
		break;
	}
	case BUILTIN_AUDCAP_PARAM_DONE_QUEUE_TXLB: {
		memcpy((AUDCAP_BUILTIN_AUDQ *)pValue, &audcap_info_txlb.wavAudQue, sizeof(AUDCAP_BUILTIN_AUDQ));
		break;
	}
#endif
	case BUILTIN_AUDCAP_PARAM_INIT_DONE:
		*pValue = audcap_info.init;
		break;
	default:
		DBG_ERR("[AUDCAPBUILTIN] Get invalid param = %d\r\n", Param);
		return -1;
	}
	return 0;
}

// install task
void audcap_builtin_install_id(void)
{

}

void audcap_builtin_uninstall_id(void)
{

}


/*
    Enable/Disable AD Analog Power
*/
static void _audcap_builtin_enable_adcpwr(void)
{
	eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_EN,       ENABLE);
	eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_LVL,       TRUE);

	// Enable AD power
	switch (audcap_info.aud_ch) {
	case AUDIO_CH_LEFT: {
			eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_L,   TRUE);
		}
		break;

	case AUDIO_CH_RIGHT: {
			eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_R,   TRUE);
		}
		break;

	//case AUDIO_CH_STEREO:
	//case AUDIO_CH_MONO: // Mono Expand
	default : {
			eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_L,   TRUE);
			eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_R,   TRUE);
		}
		break;

	}

    eac_set_ad_config(EAC_CONFIG_AD_PDREF_BUF,        FALSE);
    eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       FALSE);
    eac_set_ad_config(EAC_CONFIG_AD_PD_VCMBIAS,       FALSE);

	eac_set_phypower(ENABLE);
}

static void _audcap_builtin_codec_record(void)
{
	UINT32 filterdelay;

	// Enable AD power
	_audcap_builtin_enable_adcpwr();

	// Disable AD reset
	eac_set_ad_config(EAC_CONFIG_AD_RESET, FALSE);
	eac_set_ad_enable(TRUE);

	filterdelay = 2125 * 8000;
    filterdelay /= audcap_info.aud_sr;
	vos_util_delay_us(filterdelay);
}

static BOOL _audcap_builtin_record(void)
{

	//codec record
	_audcap_builtin_codec_record();

	//add first buffer
	_audcap_builtin_add_buf(audcap_info.pAudBufQueNext->uiAddress, audcap_info.pAudBufQueNext->uiSize);
	audcap_info.pAudBufQueNext = audcap_info.pAudBufQueNext->pNext;

	// Clear interrupt status, and interrupt flag
	dai_set_config(DAI_CONFIG_ID_CLR_INTSTS, AUDIO_RX_FLAGALL);

	dai_clr_flg(AUDIO_RX_FLAGALL);

	dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_RXDMALOAD_INT);

	// Enable RX DMA
	dai_enable_rx_dma(TRUE);

	// Wait for DMA transfer start
	dai_wait_interrupt(DAI_RXDMALOAD_INT);

	// Clear DMA Start INT
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN, DAI_RXDMALOAD_INT);

	//add second buffer
	_audcap_builtin_add_buf(audcap_info.pAudBufQueNext->uiAddress, audcap_info.pAudBufQueNext->uiSize);
	audcap_info.pAudBufQueNext = audcap_info.pAudBufQueNext->pNext;

	// Enable Interrupt
	dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_RX1BWERR_INT | DAI_RX2BWERR_INT | DAI_RX1DMADONE_INT | DAI_RX2DMADONE_INT | DAI_RXTCLATCH_INT);

	// Enable record
	dai_enable_rx(TRUE);

	return 0;
}

#if AUDCAP_BUILTIN_LOOPBACK
static BOOL _audcap_builtin_loopback(void)
{

	//add first buffer
	_audcap_builtin_add_buf_txlb(audcap_info_txlb.pAudBufQueNext->uiAddress, audcap_info_txlb.pAudBufQueNext->uiSize);
	audcap_info_txlb.pAudBufQueNext = audcap_info_txlb.pAudBufQueNext->pNext;

	// Clear interrupt status, and interrupt flag
	dai_set_config(DAI_CONFIG_ID_CLR_INTSTS, DAI_INTERRUPT_TXLB_ALL);

	dai_clr_flg(DAI_INTERRUPT_TXLB_ALL);

	dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TXLBDMALOAD_INT);

	// Enable TXLB DMA
	dai_enable_txlb_dma(TRUE);

	// Wait for DMA transfer start
	dai_wait_interrupt(DAI_TXLBDMALOAD_INT);

	// Clear DMA Start INT
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN, DAI_TXLBDMALOAD_INT);

	//add second buffer
	_audcap_builtin_add_buf_txlb(audcap_info_txlb.pAudBufQueNext->uiAddress, audcap_info_txlb.pAudBufQueNext->uiSize);
	audcap_info_txlb.pAudBufQueNext = audcap_info_txlb.pAudBufQueNext->pNext;

	// Enable Interrupt
	dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TXLBBWERR_INT | DAI_TXLBDMADONE_INT);

	// Enable playback loopback
	dai_enable_txlb(TRUE);

	return 0;
}
#endif

static BOOL _audcap_builtin_add_buf(UINT32 addr, UINT32 size)
{
	//DBG_DUMP("(rx) add addr=0x%x, size=0x%x\r\n", addr, size);
	dai_set_rx_dma_para(0, addr, size >> 2);

	return 0;
}

#if AUDCAP_BUILTIN_LOOPBACK
static BOOL _audcap_builtin_add_buf_txlb(UINT32 addr, UINT32 size)
{
	//DBG_DUMP("(txlb) add addr=0x%x, size=0x%x\r\n", addr, size);
	dai_set_txlb_dma_para(addr, size >> 2);

	return 0;
}
#endif

static void _audcap_builtin_set_gain(UINT32 gain)
{
	UINT32 data;


	if (gain > 8) {
		gain = 8;
	}

	if (gain == 0) {
		// If record gain mute, we must set mute to D-Gain2 to guarantee the output is 0x0.
		// Because the D-Gain1 is still under the ALC control and may increment.
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0x00);
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0x00);
	} else {
		// If ALC is enabled,  the record gain is set to ALC target.
		// If ALC is disabled, the record gain is set to AD PGA gain.
		if (eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)) {
			// Set ALC target gain
			data = 1 + ((gain - AUDIO_GAIN_0) << 1);

			eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_L, data);
			eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_R, data);
		} else {
			// Set ADC PGA gain
			data = 2 + ((gain - AUDIO_GAIN_0) << 2);
			eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_L, data);
			eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_R, data);
		}

		// Restore the original D-Gain2 if previously is mute.
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0xC3);
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0xC3);
	}

	eac_set_load();

}

void _audcap_builtin_set_volume(UINT32 volume)
{
	UINT32 addition_digi_gain = 0;

	if (volume == 0 ) {
		_audcap_builtin_set_gain(AUDIO_GAIN_MUTE);
		return;
	}

	if ( volume > 100) {
		addition_digi_gain = volume - 100;

		_audcap_builtin_set_gain(AUDIO_GAIN_7);
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0xC3 + addition_digi_gain);
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0xC3 + addition_digi_gain);
		eac_set_load();
	} else if (volume > 0){
		volume /=12; // separate to 8 level.
		_audcap_builtin_set_gain(volume+1);
	}

	return;
}

static int _audcap_builtin_set_sample_rate(int SamplingRate)
{
	int Ret = 0;

	{
		_audcap_builtin_set_clk(SamplingRate);

		switch (SamplingRate) {
		// 8 KHz
		case AUDIO_SR_8000: {
				eac_setdacclkrate(2048000);
                dai_setclkrate(2048000);
			}
			break;

		// 11.025 KHz
		case AUDIO_SR_11025: {
				eac_setdacclkrate(2822400);
                dai_setclkrate(2822400);
			}
			break;

		// 12 KHz
		case AUDIO_SR_12000: {
				eac_setdacclkrate(3072000);
				dai_setclkrate(3072000);
			}
			break;

		// 16 KHz
		case AUDIO_SR_16000: {
				eac_setdacclkrate(4096000);
                dai_setclkrate(4096000);
			}
			break;

		// 22.05 KHz
		case AUDIO_SR_22050: {
				eac_setdacclkrate(5644800);
                dai_setclkrate(5644800);
			}
			break;

		// 24 KHz
		case AUDIO_SR_24000: {
				eac_setdacclkrate(6144000);
                dai_setclkrate(6144000);
			}
			break;

		// 32 KHz
		case AUDIO_SR_32000: {
				eac_setdacclkrate(8192000);
                dai_setclkrate(8192000);
			}
			break;

		// 44.1 KHz
		case AUDIO_SR_44100: {
				eac_setdacclkrate(11289600);
                dai_setclkrate(11289600);
			}
			break;

		// 48 KHz
		default: {
				eac_setdacclkrate(12288000);
                dai_setclkrate(12288000);
			}
			break;
		}
	}

	return Ret;
}

static int _audcap_builtin_drv_init(void)
{
	UINT32 channel = audcap_info.aud_ch;
	UINT32 txchannel = audcap_info.aud_txch;
	UINT32 uiTCValue, uiTCTrigger;

	//audio open
	dai_open(&audcap_builtin_cb);

	eac_enabledacclk(TRUE);
	eac_enableadcclk(TRUE);
	eac_enableclk(TRUE);

	// DAI module enable
	dai_enable_dai(TRUE);


	//Audio init
	// Avoid time code hit interrupt
	// for Tx
	uiTCValue   = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_VAL);
	uiTCTrigger = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_TRIG);
	if (uiTCValue == uiTCTrigger) {
		dai_set_rx_config(DAI_RXCFG_ID_TIMECODE_TRIG, uiTCTrigger - 1);
	}

	if (!(dai_is_tx_enable(DAI_TXCH_TX1) || dai_is_tx_dma_enable(DAI_TXCH_TX1))) {
		uiTCValue   = dai_get_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TIMECODE_VAL);
		uiTCTrigger = dai_get_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TIMECODE_TRIG);
		if (uiTCValue == uiTCTrigger) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TIMECODE_TRIG, uiTCTrigger - 1);
		}
	}


	// Set DRAM PCM length(16 bits)
	if (!(dai_is_tx_enable(DAI_TXCH_TX1) || dai_is_tx_dma_enable(DAI_TXCH_TX1))) {
		dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_PCMLEN, DAI_DRAMPCM_16);
		dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_PCMLEN, DAI_DRAMPCM_16);
	}
	dai_set_rx_config(DAI_RXCFG_ID_PCMLEN,               DAI_DRAMPCM_16);
	dai_set_txlb_config(DAI_TXLBCFG_ID_PCMLEN,           DAI_DRAMPCM_16);

	switch (channel) {
	case AUDIO_CH_LEFT:
	{
		//dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
		//dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
		dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_MONO);
		//dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
		//dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_LEFT);
		//dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_LEFT);
		dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,               DAI_CH_MONO_LEFT);
		//dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_MONO_LEFT);
	}
	break;
	case AUDIO_CH_RIGHT:
	{
		//dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
		//dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
		dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_MONO);
		//dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
		//dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_RIGHT);
		//dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_RIGHT);
		dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,               DAI_CH_MONO_RIGHT);
		//dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_MONO_RIGHT);
	}
	break;

	case AUDIO_CH_STEREO:
	default: {
		//dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
		//dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
		dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_STEREO);
		//dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_STEREO);

		//dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL, DAI_CH_STEREO);
		//dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_STEREO);
		dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,               DAI_CH_STEREO);
		//dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_STEREO);
	}
	break;
	}

#if AUDCAP_BUILTIN_LOOPBACK
	if (audcap_info.aec_en) {
		switch (txchannel) {
		case AUDIO_CH_LEFT:
		{
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_MONO_LEFT);
		}
		break;
		case AUDIO_CH_RIGHT:
		{
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_MONO_RIGHT);
		}
		break;

		case AUDIO_CH_STEREO:
		default: {
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_STEREO);
		}
		break;
		}
	}
#endif

	if (!(dai_is_tx_enable(DAI_TXCH_TX1) || dai_is_tx_dma_enable(DAI_TXCH_TX1))) {
		_audcap_builtin_set_sample_rate(audcap_info.aud_sr);
	}

	// Set I2S Master/Slave
	dai_set_i2s_config(DAI_I2SCONFIG_ID_OPMODE, TRUE);

	// Set I2S Clock Ratio
	dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, 0);

	// Set I2S format
	dai_set_i2s_config(DAI_I2SCONFIG_ID_FORMAT, DAI_I2SFMT_STANDARD);

	_audcap_builtin_set_pad();


	if (!(dai_is_tx_enable(DAI_TXCH_TX1) || dai_is_tx_dma_enable(DAI_TXCH_TX1))) {
		// embedded audio codec initiation
		eac_init();

		// Set Init Channel Number
		// Set Init Output path
		// disable DAC power
		eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, FALSE);
		eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, FALSE);
	    eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       TRUE);
		eac_set_da_config(EAC_CONFIG_DA_RESET, TRUE);
		eac_set_phydacpower(DISABLE);
		eac_set_ad_config(EAC_CONFIG_AD_PD_VCMBIAS,       TRUE);
	    eac_set_phypower(DISABLE);

		eac_set_dac_output(EAC_OUTPUT_LINE,       FALSE);
		eac_set_dac_output(EAC_OUTPUT_SPK,        FALSE);
		eac_set_dac_output(EAC_OUTPUT_ALL,        FALSE);
	}


	// Reset TC Offset
	dai_set_rx_config(DAI_RXCFG_ID_TIMECODE_OFS, 0);


	//set default setting
	eac_set_ad_config(EAC_CONFIG_AD_ALC_EN, TRUE);

	eac_set_ad_config(EAC_CONFIG_AD_PGABOOST_L, EAC_PGABOOST_SEL_20DB);
	eac_set_ad_config(EAC_CONFIG_AD_PGABOOST_R, EAC_PGABOOST_SEL_20DB);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_EN, TRUE);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_L,    EAC_ALC_MAXGAIN_P21P0_DB);
	eac_set_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_R,    EAC_ALC_MAXGAIN_P21P0_DB);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_L,    EAC_ALC_MINGAIN_N16P5_DB);
	eac_set_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_R,    EAC_ALC_MINGAIN_N16P5_DB);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_ATTACK_TIME,  0x2);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_DECAY_TIME,   0x3);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_HOLD_TIME,    0x0);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_TRESO,        EAC_ALC_TRESO_BASIS_15000US);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_TRESO,     EAC_ALC_TRESO_BASIS_45000US);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD_L, EAC_NG_THRESHOLD_N58P5_DB);
	eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD_R, EAC_NG_THRESHOLD_N58P5_DB);

	eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_TARGET_L,  0x1);
	eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_TARGET_R,  0x1);


	eac_set_ad_config(EAC_CONFIG_AD_NG_ATTACK_TIME,   0x4);
	eac_set_ad_config(EAC_CONFIG_AD_NG_DECAY_TIME,    0x3);


	eac_set_ad_config(EAC_CONFIG_AD_IIR_ALC_L, FALSE);
	eac_set_ad_config(EAC_CONFIG_AD_IIR_ALC_R, FALSE);

	eac_set_ad_config(EAC_CONFIG_AD_IIR_OUT_L, FALSE);
	eac_set_ad_config(EAC_CONFIG_AD_IIR_OUT_R, FALSE);


	//set TDM
	dai_set_rx_config(DAI_RXCFG_ID_TOTAL_CH, 0);
#if AUDCAP_BUILTIN_LOOPBACK
	if (audcap_info.aec_en) {
		dai_set_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH, 0);
	}
#endif

	_audcap_builtin_set_volume(audcap_info.aud_vol);

	if (audcap_info.rec_src == 1) {
		eac_set_ad_config(EAC_CONFIG_AD_DMIC_EN,          ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_DMIC_CLK_EN,      ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_ALC_MODE_DGAIN,   ENABLE);

		eac_set_ad_config(EAC_CONFIG_AD_IIR_ALC_L, ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_IIR_ALC_R, ENABLE);

		eac_set_ad_config(EAC_CONFIG_AD_IIR_OUT_L, ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_IIR_OUT_R, ENABLE);
	}

	if (((audcap_info.default_setting/5) == 5) && ((audcap_info.default_setting%5) == 2)){
		INT32           gain;
		INT32           coef[7];

		gain =  AUDIO_IIR_COEF(4.0340522436963617) * AUDIO_IIR_COEF(0.12881465635846334); //TotalGain*SectionGain
		gain >>= 15;

		if ((gain >= (2*32768)) || (gain < (-2*32768))) {
			DBG_ERR("UnSupported Filt Coef. 6 [%d]\r\n",(int)gain);
		}

		coef[0] = (AUDIO_IIR_COEF(1.0));                      //CoefB0
		coef[1] = (AUDIO_IIR_COEF(0.0));                      //CoefB1
		coef[2] = (AUDIO_IIR_COEF(-1.0));                     //CoefB2
		coef[3] = (AUDIO_IIR_COEF(1.0));                      //CoefA0
		coef[4] = (-(AUDIO_IIR_COEF(-0.83189523663386711)));  //CoefA1
		coef[5] = (-(AUDIO_IIR_COEF(-0.039290107007699522))); //CoefA2
		coef[6] = (gain);


		eac_set_iir_coef(EAC_IIRCH_LEFT , coef);
		eac_set_iir_coef(EAC_IIRCH_RIGHT, coef);
	}

	return 0;
}


#if AUDCAP_BUILTIN_LOOPBACK
static void _audcap_builtin_set_dafault_value(PAUDCAP_BUILTIN_INFO info)
{
	info->aud_sr = 16000;
	info->aud_ch = AUDIO_CH_RIGHT;
	info->aud_bitpersample = 16;
	info->aud_buf_sample_cnt = 1024;
	info->aud_que_cnt = 50;
	info->aud_ch_num = 1;
	info->aud_vol = 100;
	info->rec_src = 0;
	info->aec_en = 0;
	info->default_setting = 0;
}

static int _audcap_builtin_parsing_dtsi(PAUDCAP_BUILTIN_INFO info)
{
	info->aud_sr = audcap_builtin_get_samplerate();
	if (info->aud_sr == -1) {
		DBG_ERR("parse sample rate failed\r\n");
		return -1;
	}
	info->aud_ch = audcap_builtin_get_channel();
	if (info->aud_ch == -1) {
		DBG_ERR("parse channel failed\r\n");
		return -1;
	}
	info->aud_que_cnt = audcap_builtin_get_bufcount();
	if (info->aud_que_cnt == -1) {
		DBG_ERR("parse queue count failed\r\n");
		return -1;
	}
	info->aud_buf_sample_cnt = audcap_builtin_get_bufsamplecnt();
	if (info->aud_buf_sample_cnt == -1) {
		DBG_ERR("parse buffer sample count failed\r\n");
		return -1;
	}
	info->rec_src = audcap_builtin_get_rec_src();
	if (info->rec_src == -1) {
		DBG_ERR("parse rec source failed\r\n");
		return -1;
	}
	info->aud_vol = audcap_builtin_get_vol();
	if (info->aud_vol == -1) {
		DBG_ERR("parse volume failed\r\n");
		return -1;
	}
	info->aud_ch_num = (info->aud_ch == AUDIO_CH_STEREO)? 2 : 1;
	info->aec_en = audcap_builtin_get_aec_en();
	if (info->aec_en == -1) {
		DBG_ERR("parse AEC enable failed\r\n");
		return -1;
	}
	if (info->aec_en) {
		info->aud_txch = audcap_builtin_get_txchannel();
		if (info->aud_txch == -1) {
			DBG_ERR("parse TX channel failed\r\n");
			return -1;
		}
		info->aud_txch_num = (info->aud_txch == AUDIO_CH_STEREO)? 2 : 1;
	}

	info->default_setting = audcap_builtin_get_default_setting();
	if (info->default_setting == -1) {
		DBG_ERR("parse default setting failed\r\n");
		return -1;
	}

	return 0;
}

static void _audcap_builtin_set_mem(AUDCAP_BUILTIN_INIT_INFO *p_info)
{
	PAUDCAP_BUILTIN_INFO info;
	UINT32 tmpSize = 0;
	UINT32 unitSize = 0;
	UINT32 sAddr;
	UINT32 tmp_buf_size;

	sAddr = ALIGN_CEIL_4(p_info->ctrl_blk_addr);

	// rx
	{
		info = &audcap_info;

		unitSize = _audcap_builtin_get_unit_size(info->aud_sr, info->aud_ch_num, info->aud_bitpersample, info->aud_buf_sample_cnt);
		tmp_buf_size = _audcap_builtin_get_tmp_size(info->aud_sr, info->aud_ch_num, info->aud_bitpersample);

		tmpSize = WAV_HEADER_BUF_SIZE + \
			      ALIGN_CEIL_64(sizeof(AUDIO_BUF_QUEUE)) * info->aud_que_cnt + \
			      ALIGN_CEIL_64(unitSize) * (info->aud_que_cnt-1) + \
			      ALIGN_CEIL_64(info->aud_buf_sample_cnt)* info->aud_ch_num * 2+ \
			      tmp_buf_size;
		tmpSize += ALIGN_CEIL_64(sizeof(BOOL)) * info->aud_que_cnt + \
				   ALIGN_CEIL_64(sizeof(AUDCAP_BUILTIN_AUDQUE)) * info->aud_que_cnt;

		info->mem_addr = sAddr;
		info->mem_size = tmpSize;
		info->unitSize = unitSize;

		sAddr += tmpSize;

		if (_audcap_builtin_make_audque(info) == 0) {
			DBG_ERR("make buffer failed (rx)\r\n");
		}
	}

	// txlb
	if (audcap_info.aec_en)
	{
		info = &audcap_info_txlb;

		unitSize = _audcap_builtin_get_unit_size(info->aud_sr, info->aud_ch_num, info->aud_bitpersample, info->aud_buf_sample_cnt);
		tmp_buf_size = _audcap_builtin_get_tmp_size(info->aud_sr, info->aud_ch_num, info->aud_bitpersample);

		tmpSize = WAV_HEADER_BUF_SIZE + \
			      ALIGN_CEIL_64(sizeof(AUDIO_BUF_QUEUE)) * info->aud_que_cnt + \
			      ALIGN_CEIL_64(unitSize) * (info->aud_que_cnt-1) + \
			      ALIGN_CEIL_64(info->aud_buf_sample_cnt)* info->aud_ch_num * 2+ \
			      tmp_buf_size;
		tmpSize += ALIGN_CEIL_64(sizeof(BOOL)) * info->aud_que_cnt + \
				   ALIGN_CEIL_64(sizeof(AUDCAP_BUILTIN_AUDQUE)) * info->aud_que_cnt;

		info->mem_addr = sAddr;
		info->mem_size = tmpSize;
		info->unitSize = unitSize;

		sAddr += tmpSize;

		if (_audcap_builtin_make_audque(info) == 0) {
			DBG_ERR("make buffer failed (txlb)\r\n");
		}
	}

	if (sAddr > (p_info->ctrl_blk_addr + p_info->ctrl_blk_size)) {
		DBG_ERR("Mem Not Enough: 0x%x, 0x%x\r\n", sAddr, (p_info->ctrl_blk_addr + p_info->ctrl_blk_size));
	}
}

#endif

int audcap_builtin_init(AUDCAP_BUILTIN_INIT_INFO *p_info)
{
	#if AUDCAP_BUILTIN_LOOPBACK
	int ret;
	#endif

	nvt_bootts_add_ts("ACAP"); //begin

	//default value
#if AUDCAP_BUILTIN_LOOPBACK
	_audcap_builtin_set_dafault_value(&audcap_info);
	if (audcap_info.aec_en) {
		_audcap_builtin_set_dafault_value(&audcap_info_txlb);
	}
#else
	audcap_info.aud_sr = 16000;
	audcap_info.aud_ch = AUDIO_CH_RIGHT;
	audcap_info.aud_bitpersample = 16;
	audcap_info.aud_buf_sample_cnt = 1024;
	audcap_info.aud_que_cnt = 50;
	audcap_info.aud_ch_num = 1;
	audcap_info.aud_vol = 100;
	audcap_info.rec_src = 0;
#endif

	//parsing dtsi
#if AUDCAP_BUILTIN_LOOPBACK
	ret = _audcap_builtin_parsing_dtsi(&audcap_info);
	if (ret != 0) {
		return 0;
	}
	if (audcap_info.aec_en) {
		ret = _audcap_builtin_parsing_dtsi(&audcap_info_txlb);
		if (ret != 0) {
			return 0;
		}
	}
#else
	audcap_info.aud_sr = audcap_builtin_get_samplerate();
	if (audcap_info.aud_sr == -1) {
		DBG_ERR("parse sample rate failed\r\n");
		return 0;
	}
	audcap_info.aud_ch = audcap_builtin_get_channel();
	if (audcap_info.aud_ch == -1) {
		DBG_ERR("parse channel failed\r\n");
		return 0;
	}
	audcap_info.aud_que_cnt = audcap_builtin_get_bufcount();
	if (audcap_info.aud_que_cnt == -1) {
		DBG_ERR("parse queue count failed\r\n");
		return 0;
	}
	audcap_info.aud_buf_sample_cnt = audcap_builtin_get_bufsamplecnt();
	if (audcap_info.aud_buf_sample_cnt == -1) {
		DBG_ERR("parse buffer sample count failed\r\n");
		return 0;
	}
	audcap_info.rec_src = audcap_builtin_get_rec_src();
	if (audcap_info.rec_src == -1) {
		DBG_ERR("parse rec source failed\r\n");
		return 0;
	}
	audcap_info.aud_vol = audcap_builtin_get_vol();
	if (audcap_info.aud_vol == -1) {
		DBG_ERR("parse volume failed\r\n");
		return 0;
	}
	audcap_info.aud_ch_num = (audcap_info.aud_ch == AUDIO_CH_STEREO)? 2 : 1;
#endif

	#if 0
	audcap_info.aud_sr = p_info->aud_sr;                ///< Sample Rate
	audcap_info.aud_ch = p_info->aud_ch;                ///< Channel
	audcap_info.aud_bitpersample = p_info->aud_bitpersample;      ///< bit Per Sample
	audcap_info.aud_ch_num = p_info->aud_ch_num;			  ///< Channel number
	audcap_info.aud_vol = p_info->aud_vol;
	audcap_info.aud_que_cnt = p_info->aud_que_cnt;
	audcap_info.aud_buf_sample_cnt = p_info->aud_buf_sample_cnt;    ///< buffer sample count
	#endif
	///////////////////////////////////////

	if (p_info->ctrl_blk_addr == 0 || p_info->ctrl_blk_size == 0) {
		DBG_ERR("Invalid buffer addr=0x%x, size=%d\r\n", p_info->ctrl_blk_addr, p_info->ctrl_blk_size);
		return 0;
	}

#if AUDCAP_BUILTIN_LOOPBACK
	// move to set_mem
#else
	audcap_info.mem_addr = p_info->ctrl_blk_addr;
	audcap_info.mem_size = p_info->ctrl_blk_size;
#endif

	audcap_builtin_install_id();

#if AUDCAP_BUILTIN_LOOPBACK
	_audcap_builtin_set_mem(p_info);
#else
	audcap_info.unitSize = _audcap_builtin_get_unit_size(audcap_info.aud_sr, audcap_info.aud_ch_num, audcap_info.aud_bitpersample, audcap_info.aud_buf_sample_cnt);

	if (_audcap_builtin_make_audque(&audcap_info) == 0) {
		DBG_ERR("make buffer failed\r\n");
		return 0;
	}
#endif

	_audcap_builtin_drv_init();

	_audcap_builtin_init_que();

#if AUDCAP_BUILTIN_LOOPBACK
	_audcap_builtin_reset_que();
#else
	audcap_info.pAudBufQueHead = audcap_info.pAudBufQueFirst;
	audcap_info.pAudBufQueTail = audcap_info.pAudBufQueFirst;
	audcap_info.pAudBufQueNext = audcap_info.pAudBufQueFirst;
#endif

	audcap_builtin_start();

	audcap_info.init = TRUE;

	return 0;
}

int audcap_builtin_start(void)
{
#if AUDCAP_BUILTIN_LOOPBACK
	if (audcap_info.aec_en) {
		_audcap_builtin_loopback();
	}
#endif
	_audcap_builtin_record();

	return 0;
}
#endif
