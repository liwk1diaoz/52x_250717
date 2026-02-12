/*
    Audio module driver

    This file is the driver of audio module

    @file       Audio.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifdef __KERNEL__
#include "kwrap/type.h"//a header for basic variable type
#include <mach/rcw_macro.h>
//#include <linux/clk.h>
#include "audio_dbg.h"
#elif defined(__FREERTOS)
#include "kwrap/type.h"//a header for basic variable type
#include "rcw_macro.h"
#endif
#include "audio_int.h"
#include "audio_platform.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"

#if defined(__FREERTOS)
unsigned int rtos_audio_debug_level = NVT_DBG_WRN;
#endif


extern vk_spinlock_t my_lock_rx;
extern vk_spinlock_t my_lock_tx1;
extern vk_spinlock_t my_lock_tx2;
extern vk_spinlock_t my_lock_txlb;

#define loc_cpu_rx(myflags)   vk_spin_lock_irqsave(&my_lock_rx, myflags)
#define unl_cpu_rx(myflags)   vk_spin_unlock_irqrestore(&my_lock_rx, myflags)

#define loc_cpu_tx1(myflags)   vk_spin_lock_irqsave(&my_lock_tx1, myflags)
#define unl_cpu_tx1(myflags)   vk_spin_unlock_irqrestore(&my_lock_tx1, myflags)

#define loc_cpu_tx2(myflags)   vk_spin_lock_irqsave(&my_lock_tx2, myflags)
#define unl_cpu_tx2(myflags)   vk_spin_unlock_irqrestore(&my_lock_tx2, myflags)

#define loc_cpu_txlb(myflags)   vk_spin_lock_irqsave(&my_lock_txlb, myflags)
#define unl_cpu_txlb(myflags)   vk_spin_unlock_irqrestore(&my_lock_txlb, myflags)






//
// Internal variables
//
static AUDTS_OBJ gpAudTsObj[AUDTS_CH_TOT] = {
	{AUDTS_CH_TX1,  aud_open_tx1,  aud_close_tx1,  aud_is_opened_tx1,  aud_is_busy_tx1,  aud_playback_tx1,  aud_record_preset_tx1,  aud_record_tx1,  aud_stop_tx1,  aud_pause_tx1,  aud_resume_tx1,  aud_set_channel_tx1,  aud_set_tdm_channel_tx1,  aud_set_samplerate_tx1,  aud_set_resampleinfo_tx1,  aud_set_feature_tx1,  aud_set_config_tx1,  aud_get_config_tx1,  aud_reset_buf_queue_tx1,  aud_add_buf_to_queue_tx1,  aud_is_buf_queue_full_tx1,  aud_get_done_buf_from_queue_tx1,  aud_get_cur_buf_from_queue_tx1 },
	{AUDTS_CH_TX2,  aud_open_tx2,  aud_close_tx2,  aud_is_opened_tx2,  aud_is_busy_tx2,  aud_playback_tx2,  aud_record_preset_tx2,  aud_record_tx2,  aud_stop_tx2,  aud_pause_tx2,  aud_resume_tx2,  aud_set_channel_tx2,  aud_set_tdm_channel_tx2,  aud_set_samplerate_tx2,  aud_set_resampleinfo_tx2,  aud_set_feature_tx2,  aud_set_config_tx2,  aud_get_config_tx2,  aud_reset_buf_queue_tx2,  aud_add_buf_to_queue_tx2,  aud_is_buf_queue_full_tx2,  aud_get_done_buf_from_queue_tx2,  aud_get_cur_buf_from_queue_tx2 },
	{AUDTS_CH_RX,   aud_open_rx,   aud_close_rx,   aud_is_opened_rx,   aud_is_busy_rx,   aud_playback_rx,   aud_record_preset_rx,   aud_record_rx,   aud_stop_rx,   aud_pause_rx,   aud_resume_rx,   aud_set_channel_rx,   aud_set_tdm_channel_rx,   aud_set_samplerate_rx,   aud_set_resampleinfo_rx,   aud_set_feature_rx,   aud_set_config_rx,   aud_get_config_rx,   aud_reset_buf_queue_rx,   aud_add_buf_to_queue_rx,   aud_is_buf_queue_full_rx,   aud_get_done_buf_from_queue_rx,   aud_get_cur_buf_from_queue_rx  },
	{AUDTS_CH_TXLB, aud_open_txlb, aud_close_txlb, aud_is_opened_txlb, aud_is_busy_txlb, aud_playback_txlb, aud_record_preset_txlb, aud_record_txlb, aud_stop_txlb, aud_pause_txlb, aud_resume_txlb, aud_set_channel_txlb, aud_set_tdm_channel_txlb, aud_set_samplerate_txlb, aud_set_resampleinfo_txlb, aud_set_feature_txlb, aud_set_config_txlb, aud_get_config_txlb, aud_reset_buf_queue_txlb, aud_add_buf_to_queue_txlb, aud_is_buf_queue_full_txlb, aud_get_done_buf_from_queue_txlb, aud_get_cur_buf_from_queue_txlb}
};


AUDIO_CODECSEL              AudioSelectedCodec  = AUDIO_CODECSEL_DEFAULT;
AUDIO_CODEC_FUNC            AudioCodecFunc[AUDIO_MAX_CODEC_NUM] = {
	// Default codec
	{
		AUDIO_CODEC_TYPE_EMBEDDED,

		NULL,           // setDevObj
		NULL,           // init
		NULL,           // setRecordSource
		NULL,           // setOutput
		NULL,           // setSamplingRate
		NULL,           // setTxChannel
		NULL,           // setRxChannel
		NULL,           // setVolume
		NULL,           // setGain
		NULL,           // setGainDB
		NULL,           // setEffect
		NULL,           // setFeature
		NULL,           // stop Record
		NULL,           // stop Playback
		NULL,           // record preset
		NULL,           // record
		NULL,           // playback
		NULL,           // setFormat
		NULL,           // setClockRatio
		NULL,           // sendCommand
		NULL,           // readData
		NULL,           // setParameter
		NULL,           // chkSamplingRate
		NULL,           // open
		NULL            // close
	},
	// Extended codec 0
	{
		AUDIO_CODEC_TYPE_HDMI,

		NULL,           // setDevObj
		NULL,           // init
		NULL,           // setRecordSource
		NULL,           // setOutput
		NULL,           // setSamplingRate
		NULL,           // setTxChannel
		NULL,           // setRxChannel
		NULL,           // setVolume
		NULL,           // setGain
		NULL,           // setGainDB
		NULL,           // setEffect
		NULL,           // setFeature
		NULL,           // stop Record
		NULL,           // stop Playback
		NULL,           // record preset
		NULL,           // record
		NULL,           // playback
		NULL,           // setFormat
		NULL,           // setClockRatio
		NULL,           // sendCommand
		NULL,           // readData
		NULL,           // setParameter
		NULL,           // chkSamplingRate
		NULL,           // open
		NULL            // close
	}
};

static AUDIO_DEVICE_OBJ     AudioDeviceObj = {
	0,                  // GPIO pin number for Cold Reset
	AUDIO_I2SCTRL_SIF,  // I2S control interface
	2,                  // I2S SIF channel / ADC channel
	0,                  // GPIO pin number for Data
	0,                  // GPIO pin number for Clk
	0,                  // GPIO pin number for CS
	0                   // ADC value of PCM data = 0
};

static UINT32           AudioLockStatus     = FALSE;

AUDIO_STATE             AudioState          = AUDIO_STATE_IDLE;

AUDIO_CB                pAudEventHandleTx1  = NULL, pAudEventHandleTx2 = NULL, pAudEventHandleRx  = NULL, pAudEventHandleTxlb  = NULL;
AUDIO_MSG_LVL           audioMsgLevel       = AUDIO_MSG_LVL_0;
BOOL                    bPlay2AllCodec      = FALSE;
//AUDIO_PLAYDGAIN       gAudioPlayGain      = AUDIO_PLAYDGAIN_0DB;

static AUDIO_SR         AudCurCodecSR       = 0;             // Current Setting Real SampleRate
static AUDIO_SR         AudCurHdmiSR        = 0;             // Current Setting Real SampleRate
BOOL                    bConnHDMI           = TRUE;          // FALSE: HDMI is disconnected from DAI (DAI and HDMI frequency can be different)
// TRUE: HDMI is connected with DAI (DAI and HDMI should has the same frequency)
UINT32                  uiDbgBufAddr        = 0;             // unit: byte, align: word
UINT32                  uiDbgBufSize        = 0;             // unit: byte, align: word


static AUDIO_RECG_LEVEL gAudio_total_Gainlel= AUDIO_RECG_LEVEL8;
static AUDIO_VOL_LEVEL  gAudio_total_Vollel = AUDIO_VOL_LEVEL8;
static AUDIO_VOL        gAudio_curr_volume  = AUDIO_VOL_2;
static AUDIO_GAIN       gAudio_curr_gain    = AUDIO_GAIN_2;
AUDIO_RECSRC     gAudioRecordSrc     = AUDIO_RECSRC_MIC;
static AUDIO_OUTPUT     gAudioOutputDst     = AUDIO_OUTPUT_SPK;

static BOOL             bExtCodecInstalled  = FALSE; // TRUE: default codec in AudioCodecFunc[] is overwritten; FALSE: aud_init() should use default codec in AudioCodecFunc[]

static AUDIO_INT_FLAG   AudioIntFlag        = AUDIO_INT_FLAG_NONE;

#if defined (__KERNEL__)
static BOOL audio_comm_first_boot = TRUE;
#endif

#if (AUDIO_EAC_GAIN_MONITOR||AUDIO_DAI_ADDR_MONITOR)
SWTIMER_ID AudSwTimerID;

static void aud_sw_timer_cb(UINT32 uiEvent)
{
#if AUDIO_EAC_GAIN_MONITOR
	{
		UINT16 uiLeft, uiRight;
		if (eac_get_cur_pgagain(&uiLeft, &uiRight))
		{
			DBG_WRN("LG=0x%02X RG=0x%02X\r\n", (unsigned int)uiLeft, (unsigned int)uiRight);
		}
	}
#endif

#if AUDIO_DAI_ADDR_MONITOR
	{
		BOOL  Tx1En, Tx2En, RxEn, TxlbEn;

		Tx1En  = dai_is_tx_enable(DAI_TXCH_TX1);
		Tx2En  = dai_is_tx_enable(DAI_TXCH_TX2);
		RxEn   = dai_is_rx_enable();
		TxlbEn = dai_is_txlb_enable();

		if (Tx1En || Tx2En || RxEn || TxlbEn)
		{
			DBG_WRN("T1=0x%08X T2=0x%08X R1=0x%08X R2=0x%08X TL=0x%08X\r\n", (unsigned int)dai_get_tx_dma_curaddr(DAI_TXCH_TX1),
			(unsigned int)dai_get_tx_dma_curaddr(DAI_TXCH_TX2), (unsigned int)dai_get_rx_dma_curaddr(0), (unsigned int)dai_get_rx_dma_curaddr(1), (unsigned int)dai_get_txlb_dma_curaddr());
		}
	}
#endif

}
#endif

#if 1
//
//  Internal APIs
//

/*
    AUDIO ISR

    AUDIO Interrupt Service Routine hook on DAI ISR

    @return void
*/
void aud_isr_handler(UINT32 uiAudioIntRegStatus)
{
	UINT32 uiAudioEventStatus;
	unsigned long flag_rx;
	unsigned long flag_tx1;
	unsigned long flag_tx2;
	unsigned long flag_txlb;

	//
	//  Handle RX1&RX2 Events
	//
	uiAudioEventStatus = 0;

	// Time Code Hit interrupt
	if (uiAudioIntRegStatus & DAI_RXTCHIT_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_TCHIT;
	}

	// Time Code Latch interrupt
	if (uiAudioIntRegStatus & DAI_RXTCLATCH_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_TCLATCH;
	}

	if (uiAudioIntRegStatus & DAI_RX1BWERR_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_FIFO_ERROR;
	}

	if (uiAudioIntRegStatus & DAI_RX2BWERR_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_FIFO_ERROR2;
	}

	// RX1 DMA done interrupt
	if (uiAudioIntRegStatus & DAI_RX1DMADONE_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_DMADONE;

		// Buffer Full or Empty
		if (pAudioRealBufferRear[TSRX1]->pNext == pAudioRealBufferFront[TSRX1]) {

			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSRX1]->uiValidSize   = pAudioPseudoBufferRear[TSRX1]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSRX1]                = pAudioPseudoBufferRear[TSRX1]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSRX1]->uiValidSize   = pAudioRealBufferRear[TSRX1]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSRX1]                = pAudioRealBufferRear[TSRX1]->pNext;

			loc_cpu_rx(flag_rx);
			if( uibuffer_insert_number_rx > 0){
                uibuffer_insert_number_rx--;
			}
			unl_cpu_rx(flag_rx);

			uiAudioEventStatus |= AUDIO_EVENT_BUF_FULL;
		} else {
			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSRX1]->uiValidSize   = pAudioPseudoBufferRear[TSRX1]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSRX1]                = pAudioPseudoBufferRear[TSRX1]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSRX1]->uiValidSize   = pAudioRealBufferRear[TSRX1]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSRX1]                = pAudioRealBufferRear[TSRX1]->pNext;

			// Set new DMA starting address and size
			loc_cpu_rx(flag_rx);
			if (pAudioRealBufferRear[TSRX1]->pNext != pAudioRealBufferFront[TSRX1]) {
				unl_cpu_rx(flag_rx);
				dai_set_rx_dma_para(0, pAudioRealBufferRear[TSRX1]->pNext->uiAddress, pAudioRealBufferRear[TSRX1]->pNext->uiSize >> 2);
			} else {
				unl_cpu_rx(flag_rx);
				uiAudioEventStatus |= AUDIO_EVENT_BUF_FULL;
				loc_cpu_rx(flag_rx);
				if( uibuffer_insert_number_rx > 0){
                	uibuffer_insert_number_rx--;
				}
				unl_cpu_rx(flag_rx);
			}

		}

		// Check done queue
		if (pAudioRealBufferRear[TSRX1]->pNext == pAudioRealBufferDone[TSRX1]) {
			uiAudioEventStatus |= AUDIO_EVENT_DONEBUF_FULL;
		}

	}

	// RX2 DMA done interrupt
	if (uiAudioIntRegStatus & DAI_RX2DMADONE_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_DMADONE2;

		// Buffer Full or Empty
		if (pAudioRealBufferRear[TSRX2]->pNext == pAudioRealBufferFront[TSRX2]) {

			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSRX2]->uiValidSize   = pAudioPseudoBufferRear[TSRX2]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSRX2]                = pAudioPseudoBufferRear[TSRX2]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSRX2]->uiValidSize   = pAudioRealBufferRear[TSRX2]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSRX2]                = pAudioRealBufferRear[TSRX2]->pNext;

			uiAudioEventStatus |= AUDIO_EVENT_BUF_FULL2;
		} else {
			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSRX2]->uiValidSize   = pAudioPseudoBufferRear[TSRX2]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSRX2]                = pAudioPseudoBufferRear[TSRX2]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSRX2]->uiValidSize   = pAudioRealBufferRear[TSRX2]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSRX2]                = pAudioRealBufferRear[TSRX2]->pNext;

			// Set new DMA starting address and size
			if (pAudioRealBufferRear[TSRX2]->pNext != pAudioRealBufferFront[TSRX2]) {
				dai_set_rx_dma_para(1, pAudioRealBufferRear[TSRX2]->pNext->uiAddress, pAudioRealBufferRear[TSRX2]->pNext->uiSize >> 2);
			} else {
				uiAudioEventStatus |= AUDIO_EVENT_BUF_FULL2;
			}
		}

		// Check done queue
		if (pAudioRealBufferRear[TSRX2]->pNext == pAudioRealBufferDone[TSRX2]) {
			uiAudioEventStatus |= AUDIO_EVENT_DONEBUF_FULL2;
		}

	}


	// Call the event handler
	if (pAudEventHandleRx != NULL) {
		if (uiAudioEventStatus != 0) {
			pAudEventHandleRx(uiAudioEventStatus);
		}
	}


	//
	//  Handle TXLB Events
	//
	uiAudioEventStatus = 0;

	if (uiAudioIntRegStatus & DAI_TXLBBWERR_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_FIFO_ERROR;
	}

	// TXLB DMA done interrupt
	if (uiAudioIntRegStatus & DAI_TXLBDMADONE_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_DMADONE;

		// Buffer Full or Empty
		if (pAudioRealBufferRear[TSTXLB]->pNext == pAudioRealBufferFront[TSTXLB]) {
			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSTXLB]->uiValidSize  = pAudioPseudoBufferRear[TSTXLB]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSTXLB]               = pAudioPseudoBufferRear[TSTXLB]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSTXLB]->uiValidSize    = pAudioRealBufferRear[TSTXLB]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSTXLB]                 = pAudioRealBufferRear[TSTXLB]->pNext;

			loc_cpu_txlb(flag_txlb);
			if(uibuffer_insert_number_txlb > 0){
                uibuffer_insert_number_txlb--;
			}
			unl_cpu_txlb(flag_txlb);

			uiAudioEventStatus |= AUDIO_EVENT_BUF_FULL;
		} else {
			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSTXLB]->uiValidSize  = pAudioPseudoBufferRear[TSTXLB]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSTXLB]               = pAudioPseudoBufferRear[TSTXLB]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSTXLB]->uiValidSize    = pAudioRealBufferRear[TSTXLB]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSTXLB]                 = pAudioRealBufferRear[TSTXLB]->pNext;

			// Set new DMA starting address and size
			loc_cpu_txlb(flag_txlb);
			if (pAudioRealBufferRear[TSTXLB]->pNext     != pAudioRealBufferFront[TSTXLB]) {
				unl_cpu_txlb(flag_txlb);
				dai_set_txlb_dma_para(pAudioRealBufferRear[TSTXLB]->pNext->uiAddress, pAudioRealBufferRear[TSTXLB]->pNext->uiSize >> 2);
			} else {
				unl_cpu_txlb(flag_txlb);
				uiAudioEventStatus |= AUDIO_EVENT_BUF_FULL;
				loc_cpu_txlb(flag_txlb);
				if(uibuffer_insert_number_txlb > 0){
                	uibuffer_insert_number_txlb--;
				}
				unl_cpu_txlb(flag_txlb);
			}
		}

		// Check done queue
		if (pAudioRealBufferRear[TSTXLB]->pNext == pAudioRealBufferDone[TSTXLB]) {
			uiAudioEventStatus |= AUDIO_EVENT_DONEBUF_FULL;
		}

	}

	// Call the event handler
	if (pAudEventHandleTxlb != NULL) {
		if (uiAudioEventStatus != 0) {
			pAudEventHandleTxlb(uiAudioEventStatus);
		}
	}


	//
	//  Handle TX1 Events
	//
	uiAudioEventStatus = 0;

	if (pAudEventHandleTx1 != NULL) {

	// Time Code Hit interrupt
	if (uiAudioIntRegStatus & DAI_TX1TCHIT_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_TCHIT;
	}

	if (uiAudioIntRegStatus & DAI_TX1BWERR_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_FIFO_ERROR;
	}


	// TX1 DMA done interrupt
	if (uiAudioIntRegStatus & DAI_TX1DMADONE_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_DMADONE;

		// Buffer Full or Empty
		if (pAudioRealBufferRear[TSTX1]->pNext == pAudioRealBufferFront[TSTX1]) {

			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSTX1]->uiValidSize   = pAudioPseudoBufferRear[TSTX1]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSTX1]                = pAudioPseudoBufferRear[TSTX1]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSTX1]->uiValidSize   = pAudioRealBufferRear[TSTX1]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSTX1]                = pAudioRealBufferRear[TSTX1]->pNext;

			loc_cpu_tx1(flag_tx1);
			if (uibuffer_insert_number_tx1 > 0) {
            	uibuffer_insert_number_tx1--;
			}
			unl_cpu_tx1(flag_tx1);

			uiAudioEventStatus |= AUDIO_EVENT_BUF_EMPTY;
		} else {
			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSTX1]->uiValidSize   = pAudioPseudoBufferRear[TSTX1]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSTX1]                = pAudioPseudoBufferRear[TSTX1]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSTX1]->uiValidSize   = pAudioRealBufferRear[TSTX1]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSTX1]                = pAudioRealBufferRear[TSTX1]->pNext;

			// Set new DMA starting address and size
			loc_cpu_tx1(flag_tx1);
			if (pAudioRealBufferRear[TSTX1]->pNext != pAudioRealBufferFront[TSTX1]) {
				unl_cpu_tx1(flag_tx1);
				dai_set_tx_dma_para(0, pAudioRealBufferRear[TSTX1]->pNext->uiAddress, pAudioRealBufferRear[TSTX1]->pNext->uiSize >> 2);
			} else {
				unl_cpu_tx1(flag_tx1);
				uiAudioEventStatus |= AUDIO_EVENT_BUF_RTEMPTY;
				loc_cpu_tx1(flag_tx1);
				if (uibuffer_insert_number_tx1 > 0) {
                	uibuffer_insert_number_tx1--;
				}
				unl_cpu_tx1(flag_tx1);
			}
		}

		// Check done queue
		if (pAudioRealBufferRear[TSTX1]->pNext == pAudioRealBufferDone[TSTX1]) {
			uiAudioEventStatus |= AUDIO_EVENT_DONEBUF_FULL;
		}

	}


	// Call the event handler
		if (uiAudioEventStatus != 0) {
			pAudEventHandleTx1(uiAudioEventStatus);
		}
	}


	//
	//  Handle TX2 Events
	//
	uiAudioEventStatus = 0;

	if (uiAudioIntRegStatus & DAI_TX2BWERR_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_FIFO_ERROR;
	}


	// TX2 DMA done interrupt
	if (uiAudioIntRegStatus & DAI_TX2DMADONE_INT) {
		uiAudioEventStatus |= AUDIO_EVENT_DMADONE;

		// Buffer Full or Empty
		if (pAudioRealBufferRear[TSTX2]->pNext == pAudioRealBufferFront[TSTX2]) {

			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSTX2]->uiValidSize  = pAudioPseudoBufferRear[TSTX2]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSTX2]               = pAudioPseudoBufferRear[TSTX2]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSTX2]->uiValidSize    = pAudioRealBufferRear[TSTX2]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSTX2]                 = pAudioRealBufferRear[TSTX2]->pNext;

			loc_cpu_tx2(flag_tx2);
			if (uibuffer_insert_number_tx2 > 0) {
                uibuffer_insert_number_tx2--;
			}
			unl_cpu_tx2(flag_tx2);

			uiAudioEventStatus |= AUDIO_EVENT_BUF_EMPTY;
		} else {
			// For pseudo operation using(upper layer using)
			// Save valid PCM data size
			pAudioPseudoBufferRear[TSTX2]->uiValidSize  = pAudioPseudoBufferRear[TSTX2]->uiSize;

			// Move to next queue
			pAudioPseudoBufferRear[TSTX2]               = pAudioPseudoBufferRear[TSTX2]->pNext;

			// For real operation using(audio driver operation using)
			// Save valid PCM data size
			pAudioRealBufferRear[TSTX2]->uiValidSize    = pAudioRealBufferRear[TSTX2]->uiSize;

			// Move to next queue
			pAudioRealBufferRear[TSTX2]                 = pAudioRealBufferRear[TSTX2]->pNext;

			// Set new DMA starting address and size
			loc_cpu_tx2(flag_tx2);
			if (pAudioRealBufferRear[TSTX2]->pNext     != pAudioRealBufferFront[TSTX2]) {
				unl_cpu_tx2(flag_tx2);
				dai_set_tx_dma_para(1, pAudioRealBufferRear[TSTX2]->pNext->uiAddress, pAudioRealBufferRear[TSTX2]->pNext->uiSize >> 2);
			} else {
				unl_cpu_tx2(flag_tx2);
				uiAudioEventStatus |= AUDIO_EVENT_BUF_RTEMPTY;
				loc_cpu_tx2(flag_tx2);
				if (uibuffer_insert_number_tx2 > 0) {
                	uibuffer_insert_number_tx2--;
				}
				unl_cpu_tx2(flag_tx2);

			}

		}

		// Check done queue
		if (pAudioRealBufferRear[TSTX2]->pNext == pAudioRealBufferDone[TSTX2]) {
			uiAudioEventStatus |= AUDIO_EVENT_DONEBUF_FULL;
		}

	}


	// Call the event handler
	if (pAudEventHandleTx2 != NULL) {
		if (uiAudioEventStatus != 0) {
			pAudEventHandleTx2(uiAudioEventStatus);
		}
	}


}

/*
    Lock AUDIO

    This function lock Audio module

    @return
        - @b E_OK: lock success
        - @b Else: lock fail (wait semaphore fail)
*/
static ER aud_lock(void)
{
	AudioLockStatus++;
	return E_OK;
}

/*
    Unlock AUDIO

    This function unlock Audio module

    @return
        - @b E_OK: unlock success
        - @b Else: unlock fail (signal semaphore fail)
*/
static ER aud_unlock(void)
{
	if (AudioLockStatus) {
		AudioLockStatus--;
	}

	return E_OK;
}

/*
    Check whether audio is opened or not

    If audio is opened this function will return TRUE. Otherwise, this function
    will return FALSE and output error message to UART.

    @return
        - @b TRUE: audio driver is opened
        - @b FALSE: audio driver is NOT opened
*/
BOOL aud_is_opened(void)
{
	return AudioLockStatus > 0;
}

/*
    Install codec function

    Check if codec function is ever installed
    If is never installed, install embedded codec.

    @return void
*/
static void aud_install_codec_function(void)
{
	if (bExtCodecInstalled == FALSE) {
		UINT32 i;
		UINT32 *pDefault;
		UINT32 *pDst;

		audcodec_get_fp(&AudioCodecFunc[AUDIO_CODECSEL_DEFAULT]);

		//audcodecHDMI_getFP(&AudioCodecFunc[AUDIO_CODECSEL_HDMI]);

		// Sync table from default to HDMI
		pDefault    = (UINT32 *) & (AudioCodecFunc[AUDIO_CODECSEL_DEFAULT]);

		// Only support AUDIO_CODECSEL_HDMI, mark to reduce code size
		pDst        = (UINT32 *) & (AudioCodecFunc[AUDIO_CODECSEL_HDMI]);

		for (i = 0; i < (sizeof(AUDIO_CODEC_FUNC) >> 2); i++) {
			// Copy default function pointer to destination
			if (*(pDst + i) == AUDIO_CODEC_FUNC_USE_DEFAULT) {
				*(pDst + i) = *(pDefault + i);
			}
		}

		bExtCodecInstalled = TRUE;
	}

}

/*
    Initialize audio codec

    Handle dai/pll for audio codec

    @param[in] pAudCodec        audio codec to be initialized
    @param[in] pAudio           Audio setting pointer

    @return void
*/
static void aud_init_codec(PAUDIO_CODEC_FUNC pAudCodec, PAUDIO_SETTING pAudio)
{


	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: 0x%x, init hdl 0x%p\r\n", __func__, (unsigned int)pAudCodec->codecType, pAudCodec->init);
	}
#if 0 // clk related
    BOOL bClkEn;
	bClkEn = pll_isClockEnabled(DAI_CLK);

	// Open DAI Clock to configure DAI and Codec because the audio may not opened.
	if (bClkEn == FALSE) {
		pll_enableClock(DAI_CLK);
	}
#endif

	/* Configure DAI for codec interface */
	if (pAudCodec->codecType == AUDIO_CODEC_TYPE_EMBEDDED) {
		// Mux DAI to internal codec
		dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN,    FALSE);
		dai_set_config(DAI_CONFIG_ID_RX_SRC_SEL ,DAI_RX_SRC_EMBEDDED);
	} else if (pAudCodec->codecType == AUDIO_CODEC_TYPE_EXTERNAL) {
		// Mux DAI to external codec
		dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN,    TRUE);
		dai_set_config(DAI_CONFIG_ID_RX_SRC_SEL ,DAI_RX_SRC_I2S);
	} else if (pAudCodec->codecType == AUDIO_CODEC_TYPE_HDMI) {
		// Set HDMI Audio interface ON/OFF
		if (bConnHDMI) {
			dai_set_config(DAI_CONFIG_ID_HDMI_TXEN,  TRUE);
		} else {
			dai_set_config(DAI_CONFIG_ID_HDMI_TXEN,  FALSE);
		}
	}

	// Call Codec Init
	if (pAudCodec->init != NULL) {
		pAudCodec->init(pAudio);
	}
#if 0 // clk related
	// Close DAI Clock if audio is not opened yet.
	if (bClkEn == FALSE) {
		pll_disableClock(DAI_CLK);
	}
#endif

}

/*
    TRUE:  Allow to change Sample Rate
    FALSE: Not Allow or Need not change.
*/
static BOOL aud_is_change_samplerate_allow(PAUDIO_CODEC_FUNC pAudCodec, AUDIO_SR newSR)
{
	if ((pAudCodec->codecType != AUDIO_CODEC_TYPE_HDMI) && (newSR == AudCurCodecSR)) {
		return FALSE;
	} else if ((pAudCodec->codecType == AUDIO_CODEC_TYPE_HDMI) && (newSR == AudCurHdmiSR)) {
		return FALSE;
	}


	if (dai_is_tx_dma_enable(DAI_TXCH_TX1)) {
		DBG_ERR("Playback CH1 is working. Change Sample Rate Fail.");
		return FALSE;
	}

	if (dai_is_tx_dma_enable(DAI_TXCH_TX2)) {
		DBG_ERR("Playback CH2 is working. Change Sample Rate Fail.");
		return FALSE;
	}

	if (dai_is_rx_dma_enable()) {
		DBG_ERR("Record is working. Change Sample Rate Fail.");
		return FALSE;
	}

	return TRUE;
}


/*
    Set sampling rate of codec

    This function set the sampling rate
    It is the sampling rate of playback/record PCM data on DRAM
    (The result is codec dependent)

    @param[in] pAudCodec    audio codec pointer
    @param[in] SamplingRate Sampling rate. Available values are below:
                            - @b AUDIO_SR_8000:     8 KHz
                            - @b AUDIO_SR_11025:    11.025 KHz
                            - @b AUDIO_SR_12000:    12 KHz
                            - @b AUDIO_SR_16000:    16 KHz
                            - @b AUDIO_SR_22050:    22.05 KHz
                            - @b AUDIO_SR_24000:    24 KHz
                            - @b AUDIO_SR_32000:    32 KHz
                            - @b AUDIO_SR_44100:    44.1 KHz
                            - @b AUDIO_SR_48000:    48 KHz
    @return
     -@b TRUE:  Change Sample Rate success.
     -@b FALSE: Not Allow or Need not change.
*/
ER aud_set_codec_samplerate(PAUDIO_CODEC_FUNC pAudCodec, AUDIO_SR SamplingRate)
{
	ER Ret;


	Ret = aud_is_change_samplerate_allow(pAudCodec, SamplingRate);

	if (Ret) {
		if (pAudCodec->codecType != AUDIO_CODEC_TYPE_HDMI) {
			AudCurCodecSR = SamplingRate;
		} else {
			AudCurHdmiSR  = SamplingRate;
		}


		switch (SamplingRate) {
		case AUDIO_SR_11025:
		case AUDIO_SR_22050:
		case AUDIO_SR_44100: {
#ifdef __KERNEL__
          	 _audio_platform_set_samplerate((int)SamplingRate);
#else
			if (pll_getPLLFreq(PLL_ID_7) != AUDIO_MPLL_44P1K_SRCFREQ) {
				pll_setPLLEn(PLL_ID_7, TRUE);
				pll_setDrvPLL(PLL_ID_7, AUDIO_MPLL_44P1K_RATIO);
			}
#endif
		} break;

		case AUDIO_SR_8000:
		case AUDIO_SR_12000:
		case AUDIO_SR_16000:
		case AUDIO_SR_24000:
		case AUDIO_SR_32000:
		case AUDIO_SR_48000:
		default: {
#ifdef __KERNEL__
			_audio_platform_set_samplerate((int)SamplingRate);
#else
			if (pll_getPLLFreq(PLL_ID_7) != AUDIO_MPLL_48K_SRCFREQ) {
				pll_setPLLEn(PLL_ID_7, TRUE);
				pll_setDrvPLL(PLL_ID_7, AUDIO_MPLL_48K_RATIO);
			}
#endif
		} break;

		}


		switch (SamplingRate) {
		// 8 KHz
		case AUDIO_SR_8000: {
				if (pAudCodec->codecType != AUDIO_CODEC_TYPE_HDMI) {
#if _FPGA_EMULATION_
					eac_setdacclkrate(FPGA_CLK_RATIO(2048000));
					dai_setclkrate(FPGA_CLK_RATIO(2048000));
#else
#ifdef __KERNEL__
					eac_setdacclkrate(16384000);
                    dai_setclkrate(2048000);
					eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x1024);
#else
					pll_setClockFreq(ADOOSRCLK_FREQ,	16384000);
					pll_setClockFreq(ADOCLK_FREQ,       2048000);
					eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x1024);
#endif
#endif
				}
			}
			break;

		// 11.025 KHz
		case AUDIO_SR_11025: {
				if (pAudCodec->codecType != AUDIO_CODEC_TYPE_HDMI) {
#if _FPGA_EMULATION_
					eac_setdacclkrate(FPGA_CLK_RATIO(2822400));
					dai_setclkrate(FPGA_CLK_RATIO(2822400));
#else
#ifdef __KERNEL__
					eac_setdacclkrate(2822400);
                    dai_setclkrate(2822400);
                    eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x128);
#else
					pll_setClockFreq(ADOCLK_FREQ,       2822400);
#endif
#endif
				}
			}
			break;

		// 12 KHz
		case AUDIO_SR_12000: {
				if (pAudCodec->codecType != AUDIO_CODEC_TYPE_HDMI) {
#if _FPGA_EMULATION_
					eac_setdacclkrate(FPGA_CLK_RATIO(3072000));
					dai_setclkrate(FPGA_CLK_RATIO(3072000));
#else
#ifdef __KERNEL__
					eac_setdacclkrate(3072000);
					dai_setclkrate(3072000);
					eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x128);
#else
					pll_setClockFreq(ADOCLK_FREQ,       3072000);
#endif
#endif
				}
			}
			break;

		// 16 KHz
		case AUDIO_SR_16000: {
				if (pAudCodec->codecType != AUDIO_CODEC_TYPE_HDMI) {
#if _FPGA_EMULATION_
					eac_setdacclkrate(FPGA_CLK_RATIO(4096000));
					dai_setclkrate(FPGA_CLK_RATIO(4096000));
#else
#ifdef __KERNEL__
					eac_setdacclkrate(16384000);
                    dai_setclkrate(4096000);
					eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x512);
#else
					pll_setClockFreq(ADOOSRCLK_FREQ,	16384000);
					pll_setClockFreq(ADOCLK_FREQ,       4096000);
					eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x512);
#endif
#endif
				}
			}
			break;

		// 22.05 KHz
		case AUDIO_SR_22050: {
				if (pAudCodec->codecType != AUDIO_CODEC_TYPE_HDMI) {
#if _FPGA_EMULATION_
					eac_setdacclkrate(FPGA_CLK_RATIO(5644800));
					dai_setclkrate(FPGA_CLK_RATIO(5644800));
#else
#ifdef __KERNEL__
					eac_setdacclkrate(5644800);
                    dai_setclkrate(5644800);
                    eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x128);
#else
					pll_setClockFreq(ADOCLK_FREQ,       5644800);
#endif
#endif
				}
			}
			break;

		// 24 KHz
		case AUDIO_SR_24000: {
				if (pAudCodec->codecType != AUDIO_CODEC_TYPE_HDMI) {
#if _FPGA_EMULATION_
					eac_setdacclkrate(FPGA_CLK_RATIO(6144000));
					dai_setclkrate(FPGA_CLK_RATIO(6144000));
#else
#ifdef __KERNEL__
					eac_setdacclkrate(6144000);
                    dai_setclkrate(6144000);
                    eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x128);
#else
					pll_setClockFreq(ADOCLK_FREQ,       6144000);
#endif
#endif
				}
			}
			break;

		// 32 KHz
		case AUDIO_SR_32000: {
#if _FPGA_EMULATION_
				eac_setdacclkrate(FPGA_CLK_RATIO(8192000));
				dai_setclkrate(FPGA_CLK_RATIO(8192000));
#else
#ifdef __KERNEL__
				eac_setdacclkrate(8192000);
                dai_setclkrate(8192000);
                eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x128);
#else
					pll_setClockFreq(ADOCLK_FREQ,       8192000);
#endif
#endif
			}
			break;

		// 44.1 KHz
		case AUDIO_SR_44100: {
#if _FPGA_EMULATION_
				eac_setdacclkrate(FPGA_CLK_RATIO(11289600));
				dai_setclkrate(FPGA_CLK_RATIO(11289600));
#else
#ifdef __KERNEL__
				eac_setdacclkrate(11289600);
                dai_setclkrate(11289600);
                eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x128);
#else
					pll_setClockFreq(ADOCLK_FREQ,       11289600);
#endif
#endif
			}
			break;

		// 48 KHz
		default: {
#if _FPGA_EMULATION_
				eac_setdacclkrate(FPGA_CLK_RATIO(12288000));// set faster rate when FPGA pll7 is not as fast as real chip
				dai_setclkrate(FPGA_CLK_RATIO(12288000));
#else
#ifdef __KERNEL__
				eac_setdacclkrate(12288000);
                dai_setclkrate(12288000);
                eac_set_da_config(EAC_CONFIG_DA_OSR_SEL,EAC_OSR_SEL_x128);
#else
					pll_setClockFreq(ADOCLK_FREQ,       12288000);
#endif
#endif
			}
			break;
		}
	}

	if (pAudCodec->setSamplingRate != NULL) {
		pAudCodec->setSamplingRate(SamplingRate);
	}

	return Ret;
}

/*

*/
ER aud_set_codec_channel(AUDTSCH tsCH, AUDIO_CH Channel)
{
	static AUDIO_CH Tx1CH = AUDIO_CH_STEREO, Tx2CH = AUDIO_CH_STEREO; //,RxCH = AUDIO_CH_STEREO;
	AUDIO_CH        CfgCH = AUDIO_CH_STEREO;
	UINT32          i;

	switch (tsCH) {
	case TSTX1:
	case TSTX2: {
			if (tsCH == TSTX1) {
				Tx1CH = Channel;
			} else {
				Tx2CH = Channel;
			}

			if (Tx1CH == Tx2CH) {
				CfgCH = Tx1CH;
			} else if (((0x1 << Tx1CH) | (0x1 << Tx2CH)) >= 0x3) {
				CfgCH = AUDIO_CH_STEREO;
			}

			for (i = 0; i < dim(AudioCodecFunc); i++) {
				if (AudioCodecFunc[i].setTxChannel != NULL) {
					//DBG_DUMP("TX1=%d  TX2=%d Use=%d\r\n",Tx1CH,Tx2CH,CfgCH);
					AudioCodecFunc[i].setTxChannel(CfgCH);
				}
			}
		}
		break;

	case TSRX1:
	case TSRX2: {
			//RxCH  = Channel;
			CfgCH = Channel;

			for (i = 0; i < dim(AudioCodecFunc); i++) {
				if (AudioCodecFunc[i].setRxChannel != NULL) {
					AudioCodecFunc[i].setRxChannel(CfgCH);
				}
			}
		}
		break;

	default:
		return E_NOSPT;

	}

	return E_OK;
}


#endif

/**
    Initialize AUDIO driver

    Initialize the AUDIO driver based on parameter.
    This function should be call once after power on.

    @param[in] pAudio       Audio setting pointer

    @return void
*/
void aud_init(PAUDIO_SETTING pAudio)
{
	UINT32 uiTCValue, uiTCTrigger, i;


	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && audio_comm_first_boot) {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (builtin_init) {
			// Initialize buffer queue
			aud_init_queue();
			return;
		}
	}
#endif

	// Only default codec will call this function,
	// If project assign extend codec, default and extend codec function
	// pointer will be set in aud_set_extcodec()
	aud_install_codec_function();

	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setDevObj != NULL) {
			AudioCodecFunc[i].setDevObj(&AudioDeviceObj);
		}
	}

	// Initialize buffer queue
	aud_init_queue();

	// Avoid time code hit interrupt
	// for Tx
	uiTCValue   = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_VAL);
	uiTCTrigger = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_TRIG);
	if (uiTCValue == uiTCTrigger) {
		dai_set_rx_config(DAI_RXCFG_ID_TIMECODE_TRIG, uiTCTrigger - 1);
	}

	uiTCValue   = dai_get_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TIMECODE_VAL);
	uiTCTrigger = dai_get_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TIMECODE_TRIG);
	if (uiTCValue == uiTCTrigger) {
		dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TIMECODE_TRIG, uiTCTrigger - 1);
	}


	// Set DRAM PCM length(16 bits)
	dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_PCMLEN, DAI_DRAMPCM_16);
	dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_PCMLEN, DAI_DRAMPCM_16);
	dai_set_rx_config(DAI_RXCFG_ID_PCMLEN,               DAI_DRAMPCM_16);
	dai_set_txlb_config(DAI_TXLBCFG_ID_PCMLEN,           DAI_DRAMPCM_16);

	if (pAudio->Channel == AUDIO_CH_DUAL_MONO) {
		pAudio->Channel = AUDIO_CH_STEREO;
	}

	// Set Channel
	AudioTx1CH = pAudio->Channel;
	AudioTx2CH = pAudio->Channel;
	AudioRxCH  = pAudio->Channel;
	AudioTxlbCH = pAudio->Channel;

	switch (pAudio->Channel) {
	//case AUDIO_CH_LEFT:
	//{
	//    dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
	//    dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
	//    dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_MONO);
	//    dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
	//    dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_LEFT);
	//    dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_LEFT);
	//    dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,               DAI_CH_MONO_LEFT);
	//    dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_MONO_LEFT);
	//}
	//break;
	//case AUDIO_CH_RIGHT:
	//{
	//    dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
	//    dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
	//    dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_MONO);
	//    dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
	//    dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_RIGHT);
	//    dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_RIGHT);
	//    dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,               DAI_CH_MONO_RIGHT);
	//    dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_MONO_RIGHT);
	//}
	//break;

	case AUDIO_CH_STEREO:
	default: {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
			dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
			dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_STEREO);

			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL, DAI_CH_STEREO);
			dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_STEREO);
			dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,               DAI_CH_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,           DAI_CH_STEREO);
		}
		break;
	}


#ifndef __KERNEL__
	// Set audio clock rate
	if (pll_getPLLEn(PLL_ID_7) == FALSE) {
		pll_setPLLEn(PLL_ID_7, TRUE);
	}
#elif defined __UITRON || defined __ECOS
	pll_setClockRate(PLL_CLKSEL_ADO_CLKSEL,         PLL_CLKSEL_ADO_CLKSEL_CLKDIV);
#if _FPGA_EMULATION_
	pll_setClockRate(PLL_CLKSEL_HDMI_ADO_CLKSEL,    PLL_CLKSEL_HDMI_ADO_CLKSEL_CLKDIV);
#else
	pll_setClockRate(PLL_CLKSEL_HDMI_ADO_CLKSEL,    PLL_CLKSEL_HDMI_ADO_CLKSEL_CLKMUX);
#endif

#endif

	AudSrcSRTx1 = pAudio->SamplingRate;
	AudSrcSRTx2 = pAudio->SamplingRate;
	AudSrcSRRx  = pAudio->SamplingRate;

	for (i = 0; i < dim(AudioCodecFunc); i++) {
		aud_set_codec_samplerate(&AudioCodecFunc[i], pAudio->SamplingRate);
	}
	// Set I2S Master/Slave
	dai_set_i2s_config(DAI_I2SCONFIG_ID_OPMODE, pAudio->I2S.bMaster);

	// Set I2S Clock Ratio
	dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, pAudio->I2S.ClkRatio);

	// Set I2S format
	switch (pAudio->I2S.I2SFmt) {
	case AUDIO_I2SFMT_STANDARD: {
			dai_set_i2s_config(DAI_I2SCONFIG_ID_FORMAT, DAI_I2SFMT_STANDARD);
		}
		break;

	case AUDIO_I2SFMT_LIKE_MSB:
	case AUDIO_I2SFMT_LIKE_LSB:
	default: {
			DBG_WRN("I2S format not support: 0x%x\r\n", (unsigned int)pAudio->I2S.I2SFmt);
		}
		break;
	}

	// Initialize Codec
	for (i = 0; i < dim(AudioCodecFunc); i++) {
		aud_init_codec(&AudioCodecFunc[i], pAudio);
	}

}


/**
    Open AUDIO driver

    This function should be called before calling any other functions,
    except the following:
    (1) aud_get_device_object()
    (2) aud_get_lock_status()
    (3) aud_init()

    If you want to use GPIO to control I2S codec,
    you should set pDevObj->uiI2SCtrl to AUDIO_I2SCTRL_GPIO_SIF or AUDIO_I2SCTRL_GPIO_I2C,
    and set pDevObj->uiGPIOData, pDevObj->uiGPIOClk, pDevObj->uiGPIOCS to correct GPIO pin number.
    Don't forget to change pinmux of those pins to GPIO by your application.

    @return
        - @b E_OK: open success
        - @b Else: open fail
*/
ER aud_open(void)
{
	UINT32  i;



	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	if (AudioLockStatus > 0) {
		// Increment lock counter
		AudioLockStatus++;

		if (audioMsgLevel == AUDIO_MSG_LVL_1) {
			DBG_DUMP("AUDLOCK CNT=%d\r\n", (int)AudioLockStatus);
		}

		return E_OK;
	}


	// Lock Audio
	aud_lock();

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && audio_comm_first_boot) {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (builtin_init) {
			AudioState = AUDIO_STATE_IDLE;
			return E_OK;
		}
	}
#endif

	// Open DAI Driver. Even if clock always on.
	// Just open it would not result in error op.

	dai_open(&aud_isr_handler);
	// Make sure codec functions are installed before access it

	aud_install_codec_function();


	// Open each codec library
	for (i = 0; i < dim(AudioCodecFunc); i++) {

		if (AudioCodecFunc[i].codecType == AUDIO_CODEC_TYPE_EXTERNAL) {
			if (AudioIntFlag & AUDIO_INT_FLAG_MCLKAUTOPINMUX) {
				dai_select_mclk_pinmux(TRUE);
			}
			if (AudioIntFlag & AUDIO_INT_FLAG_AUTOPINMUX) {
				dai_select_pinmux(TRUE);
			}
		}

		if (AudioCodecFunc[i].open != NULL) {
			AudioCodecFunc[i].open();
		}
	}

	// DAI module enable
	dai_enable_dai(TRUE);

	// Open GPIO driver
	//gpio_open();

#if (AUDIO_EAC_GAIN_MONITOR||AUDIO_DAI_ADDR_MONITOR)
	SwTimer_Open(&AudSwTimerID, aud_sw_timer_cb);
	SwTimer_Cfg(AudSwTimerID, 500, SWTIMER_MODE_FREE_RUN);
	SwTimer_Start(AudSwTimerID);
#endif

	AudioState = AUDIO_STATE_IDLE;

	return E_OK;
}

/**
    Close AUDIO driver

    Release AUDIO driver and let other application use AUDIO

    @return
        - @b E_OK: close success
        - @b Else: close fail
*/
ER aud_close(void)
{
	UINT32  i;

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	// Return immediatelly if is not opened
	if (AudioLockStatus == 0) {
		return E_OK;
	}

	aud_unlock();

	// Not all closed. Keep audio open.
	if (AudioLockStatus > 0) {
		return E_OK;
	}


	if (dai_is_tx_dma_enable(DAI_TXCH_TX1) == TRUE) {
		DBG_WRN("TX1 still play. Force Off\r\n");
		aud_stop_tx1();
	}

	if (dai_is_tx_dma_enable(DAI_TXCH_TX2) == TRUE) {
		DBG_WRN("TX2 still play. Force Off\r\n");
		aud_stop_tx2();
	}

	if (dai_is_rx_dma_enable() == TRUE) {
		DBG_WRN("RX still record. Force Off\r\n");
		aud_stop_rx();
	}

	// Audio clock is not always ON, turn off clock
	if (!(AudioIntFlag & AUDIO_INT_FLAG_CLKALWAYS_ON)) {
		// DAI module disable
		dai_enable_dai(FALSE);

		// Release interrupt
		dai_close();
	}

	// Close each codec library
	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].close != NULL) {
			AudioCodecFunc[i].close();
		}

		if (AudioCodecFunc[i].codecType == AUDIO_CODEC_TYPE_EXTERNAL) {
			if (AudioIntFlag & AUDIO_INT_FLAG_AUTOPINMUX) {
				dai_select_pinmux(FALSE);
			}

			if (AudioIntFlag & AUDIO_INT_FLAG_MCLKAUTOPINMUX) {
				dai_select_mclk_pinmux(FALSE);
			}
		}
	}

	// Close GPIO driver
	//gpio_close();

#if (AUDIO_EAC_GAIN_MONITOR||AUDIO_DAI_ADDR_MONITOR)
	SwTimer_Stop(AudSwTimerID);
	SwTimer_Close(AudSwTimerID);
#endif

#if defined (__KERNEL__)
	audio_comm_first_boot = FALSE;
#endif

	return E_OK;
}

/**
    Get the lock status of AUDIO

    This function return the lock status of AUDIO. Return value is as following:
        NO_TASK_LOCKED  :AUDIO is free, no application is using AUDIO
        TASK_LOCKED     :AUDIO is locked by some application

    @return
        - @b FALSE: AUDIO is free
        - @b TRUE: AUDIO is locked by some application
*/
BOOL aud_get_lock_status(void)
{
	return AudioLockStatus > 0;
}

/**
    Check whether the Audio module is busy recording/playbacking or not

    This function check whether the Audio module is busy recording/playbacking or not.

    @return audio module busy state:
        - @b TRUE: Busy recording/playbacking
        - @b FALSE: idle or wait for VD from SIE to start recording
*/
BOOL aud_is_busy(void)
{
	return (dai_is_tx_enable(DAI_TXCH_TX1) || dai_is_tx_enable(DAI_TXCH_TX2) || dai_is_rx_enable());
}

/**
    Get Audio TranSceive Object

    Get Audio TranSceive Object. This object is used to control the audio playback/record.

    @param[in] TsCH     TranSceive Object ID.

    @return PAUDTS_OBJ The transceive object pointer.
*/
PAUDTS_OBJ aud_get_transceive_object(AUDTS_CH TsCH)
{

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && audio_comm_first_boot) {
		return &gpAudTsObj[TsCH];
	}
#endif

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return NULL;
	}

#if 1//_EMULATION_
	if (TsCH > AUDTS_CH_TOT)
#else
	if (TsCH >= AUDTS_CH_TOT)
#endif
	{
		DBG_ERR("No such Obj\r\n");
		return NULL;
	}

	return &gpAudTsObj[TsCH];
}

#if 1
/**
    Set extended audio codec

    Set extended audio codec. This function should be called once before aud_init().
    If you need to use external audio codec instead of internal audio codec, you should invoke this function to install external audio codec.
    For example, following sequence can install external audio codec:
    (1) audExtCodec_getFP(&gExtCodecFunc);
    (2) aud_set_extcodec(&gExtCodecFunc);

    @param[in] pAudioCodecFunc      Audio codec function pointer
      - @b NULL: uninstall external codec (instead use embedded codec)
      - @b Else: install external codec
    @return void
*/
void aud_set_extcodec(PAUDIO_CODEC_FUNC pAudioCodecFunc)
{
	UINT32 i;
	UINT32 *pDst, *pDefault;
    AUDIO_CODEC_FUNC	AudCodecEmbd;

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: 0x%p\r\n", __func__, pAudioCodecFunc);
	}

	bExtCodecInstalled = TRUE;

	// Get function pointer for default codec
	if (pAudioCodecFunc == NULL) {
		audcodec_get_fp(&AudioCodecFunc[AUDIO_CODECSEL_DEFAULT]);
	} else {
		audcodec_get_fp(&AudCodecEmbd);
		memcpy((void *)&AudioCodecFunc[AUDIO_CODECSEL_DEFAULT], (const void *)pAudioCodecFunc, sizeof(AUDIO_CODEC_FUNC));

		pDefault    = (UINT32 *) &AudCodecEmbd;
		pDst        = (UINT32 *) &(AudioCodecFunc[AUDIO_CODECSEL_DEFAULT]);
		for (i = 0; i < (sizeof(AUDIO_CODEC_FUNC) >> 2); i++) {
			// Copy default function pointer to destination
			if (*(pDst + i) == AUDIO_CODEC_FUNC_USE_DEFAULT) {
				*(pDst + i) = *(pDefault + i);
                DBG_ERR("copy default function now ... \r\n");
			}
		}
	}
}

/**
    Switch audio codec

    Switch audio codec.
    You can use this function to switch between embedded/external audio codec and HDMI audio.
    For example, you should invoke aud_switch_codec(AUDIO_CODECSEL_HDMI) if HDMI plug is detected.
    After HDMI cable is removed, you may invoke aud_switch_codec(AUDIO_CODECSEL_DEFAULT) to switch back to embedded/external audio codec.

    @param[in] AudioCodec           Codec ID
                                    - @b AUDIO_CODECSEL_DEFAULT:    Default audio codec
                                    - @b AUDIO_CODECSEL_HDMI:       Extended audio codec 0
    @return void
*/
void aud_switch_codec(AUDIO_CODECSEL AudioCodec)
{
	AUDIO_CODECSEL  AudioPreSelectedCodec;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)AudioCodec);
	}


	// Save previous codec
	AudioPreSelectedCodec   = AudioSelectedCodec;

	// Assign new codec
	AudioSelectedCodec      = AudioCodec;

	// If in Playing and not play2all, we should stop previous one and start the new one.
	if ((bPlay2AllCodec == FALSE) && ((dai_is_tx_dma_enable(DAI_TXCH_TX1) == TRUE) || (dai_is_tx_dma_enable(DAI_TXCH_TX2) == TRUE))) {
		// If new codec's playback function != previous codec's playback function then inform new codec to playback
		if ((AudioCodecFunc[AudioSelectedCodec].playback != NULL) &&
			(AudioCodecFunc[AudioSelectedCodec].playback != AudioCodecFunc[AudioPreSelectedCodec].playback)) {
			AudioCodecFunc[AudioSelectedCodec].playback();
		}

		// If new codec's stop function != previous codec's stop function then stop previous codec
		if ((AudioCodecFunc[AudioPreSelectedCodec].stopPlay != NULL) &&
			(AudioCodecFunc[AudioPreSelectedCodec].stopPlay != AudioCodecFunc[AudioSelectedCodec].stopPlay)) {
			AudioCodecFunc[AudioPreSelectedCodec].stopPlay();
		}

	}

	// Recording
	if (dai_is_rx_dma_enable() == TRUE) {
		// If new codec's record function != previous codec's record function then inform new codec to record
		if ((AudioCodecFunc[AudioSelectedCodec].record != NULL) &&
			(AudioCodecFunc[AudioSelectedCodec].record != AudioCodecFunc[AudioPreSelectedCodec].record)) {
			AudioCodecFunc[AudioSelectedCodec].record();

			// Only stop previous codec if new codec support record function
			if (AudioCodecFunc[AudioPreSelectedCodec].stopRecord != NULL) {
				AudioCodecFunc[AudioPreSelectedCodec].stopRecord();
			}
		}
	}

}

/**
    Set Audio Device Control Object

    Set Audio Device Control Object. This device object provides the information
    of the external audio codec control interface.

    @param[in] pDevObj Audio Device Control Object. Please refer to PAUDIO_DEVICE_OBJ for details.

    @return void
*/
void aud_set_device_object(PAUDIO_DEVICE_OBJ pDevObj)
{
	// Save Audio Device Object setting
	memcpy((void *)&AudioDeviceObj, (void *)pDevObj, sizeof(AUDIO_DEVICE_OBJ));
}


/**
    Get AUDIO Device Object

    Get AUDIO Device Object

    @param[out] pDevObj     Return current AUDIO Device Object

    @return void
*/
void aud_get_device_object(PAUDIO_DEVICE_OBJ pDevObj)
{
	memcpy((void *)pDevObj, (void *)&AudioDeviceObj, sizeof(AUDIO_DEVICE_OBJ));
}

/**
    Select output path

    This function select the output path.
    The result is codec dependent.

    @param[in] Output       Output path. Available values are below:
                            - @b AUDIO_OUTPUT_SPK: output to (Speaker)
                            - @b AUDIO_OUTPUT_HP: output to (Headphone)
                            - @b AUDIO_OUTPUT_LINE: output to (Line Out)
                            - @b AUDIO_OUTPUT_MONO: output to (Mono Out)
                            - @b AUDIO_OUTPUT_NONE: no output
    @return void
*/
void aud_set_output(AUDIO_OUTPUT Output)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	gAudioOutputDst = Output;

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)Output);
	}

	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setOutput != NULL) {
			AudioCodecFunc[i].setOutput(Output);
		}
	}
}

/**
    Set special feature

    This function set the special feature to the audio codec.
    The result is codec dependent.
    You should call this function after aud_init().

    @param[in] Feature  Audio feature
    @param[in] bEnable  Enable/Disable the feature
                        - @b Enable Feature
                        - @b Disable Feature
    @return void
*/
void aud_set_feature(AUDIO_FEATURE Feature, BOOL bEnable)
{
	UINT32 i;
	BOOL bHandled = FALSE;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d %d\r\n", __func__, (int)Feature, (int)bEnable);
	}


	switch (Feature) {
	/*
	    Playback Related Features
	*/
	case AUDIO_FEATURE_PLAY2ALLCODEC: {
			bPlay2AllCodec = bEnable;
		}
		break;
	case AUDIO_FEATURE_INTERFACE_ALWAYS_ACTIVE: {
			AudioIntFlag &= ~AUDIO_INT_FLAG_CLKALWAYS_ON;
			if (bEnable) {
				AudioIntFlag |= AUDIO_INT_FLAG_CLKALWAYS_ON;
			}
		}
		break;
	case AUDIO_FEATURE_DISCONNECT_HDMI: {
			bConnHDMI = !bEnable;

			if (bConnHDMI) {
				dai_set_config(DAI_CONFIG_ID_HDMI_TXEN, TRUE);
			} else {
				dai_set_config(DAI_CONFIG_ID_HDMI_TXEN, FALSE);
			}
		}
		break;


	/*
	    MISC Features
	*/
	case AUDIO_FEATURE_AUTOPINMUX: {
			AudioIntFlag &= ~AUDIO_INT_FLAG_AUTOPINMUX;
			if (bEnable) {
				AudioIntFlag |= AUDIO_INT_FLAG_AUTOPINMUX;
			}
		}
		break;
	case AUDIO_FEATURE_MCLK_AUTOPINMUX: {
			AudioIntFlag &= ~AUDIO_INT_FLAG_MCLKAUTOPINMUX;
			if (bEnable) {
				AudioIntFlag |= AUDIO_INT_FLAG_MCLKAUTOPINMUX;
			}
		}
		break;


	// For emulation
	case AUDIO_FEATURE_DEBUG_MODE_EN: {
		}
		break;

	default: {
			if (Feature == AUDIO_FEATURE_LINE_PWR_ALWAYSON) {
				AudioIntFlag &= ~AUDIO_INT_FLAG_LINEPWR_ALWAYSON;
				if (bEnable) {
					AudioIntFlag |= AUDIO_INT_FLAG_LINEPWR_ALWAYSON;
				}
			} else if (Feature == AUDIO_FEATURE_SPK_PWR_ALWAYSON) {
				AudioIntFlag &= ~AUDIO_INT_FLAG_SPKPWR_ALWAYSON;
				if (bEnable) {
					AudioIntFlag |= AUDIO_INT_FLAG_SPKPWR_ALWAYSON;
				}
			} else if (Feature == AUDIO_FEATURE_HP_PWR_ALWAYSON) {
				AudioIntFlag &= ~AUDIO_INT_FLAG_HPPWR_ALWAYSON;
				if (bEnable) {
					AudioIntFlag |= AUDIO_INT_FLAG_HPPWR_ALWAYSON;
				}
			}

			for (i = 0; i < dim(AudioCodecFunc); i++) {
				if (AudioCodecFunc[i].setFeature != NULL) {
					if (AudioCodecFunc[i].setFeature(Feature, bEnable) == TRUE) {
						bHandled = TRUE;
					}
				}
			}

			if (bHandled == FALSE) {
				DBG_WRN("feature 0x%x not handled\r\n", (unsigned int)Feature);
			}
		}
		break;
	}
}

/**
    Set special parameter

    This function set the special parameter to the audio codec.
    The result is codec dependent.
    You should call this function after aud_init().

    @param[in] Parameter    Audio parameter. Available values are below:
    @param[in] uiSetting    parameter setting of Parameter.

    @return void
*/
void aud_set_parameter(AUDIO_PARAMETER Parameter, UINT32 uiSetting)
{
	UINT32 i;
	BOOL bHandled = FALSE;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d %d\r\n", __func__, (int)Parameter, (int)uiSetting);
	}


	switch (Parameter) {
	case AUDIO_PARAMETER_DBG_MSG_LEVEL: {
			audioMsgLevel = uiSetting;
		}
		break;
	case AUDIO_PARAMETER_DEBUG_BUF_ADDR: {
			uiDbgBufAddr = uiSetting;
		}
		break;
	case AUDIO_PARAMETER_DEBUG_BUF_SIZE: {
			uiDbgBufSize = uiSetting;
		}
		break;

	default: {
			for (i = 0; i < dim(AudioCodecFunc); i++) {
				if (AudioCodecFunc[i].setParameter != NULL) {
					if (AudioCodecFunc[i].setParameter(Parameter, uiSetting) == TRUE) {
						bHandled = TRUE;
					}
				}
			}

			if (bHandled == FALSE) {
				DBG_WRN("Parameter 0x%x not handled\r\n", (unsigned int)Parameter);
			}
		}
		break;
	}
}

/**
    Set PCM out total volume level

    This function set PCM out total volume level.The result is codec dependent.
    When the parameter "Audio_VolLevel" is set to AUDIO_VOL_LEVEL8, the valid
    parameter "vol" range of API aud_set_volume() is from AUDIO_VOL_MUTE ~ AUDIO_VOL_7.
    When the parameter "Audio_VolLevel" is set to AUDIO_VOL_LEVEL64, the valid parmeter
    "vol" range of API aud_set_volume() is from AUDIO_VOL_MUTE ~ AUDIO_VOL_63.

    @param[in] Audio_VolLevel   Total volume level. Available values are below:
                                - @b AUDIO_VOL_LEVEL8: Total volume level = 8
                                - @b AUDIO_VOL_LEVEL64: Total volume level = 64

    @return void
*/
void aud_set_total_vol_level(AUDIO_VOL_LEVEL Audio_VolLevel)
{
	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)Audio_VolLevel);
	}

	gAudio_total_Vollel = Audio_VolLevel;
}

/**
    Set PCM out total volume level

    This function get PCM out total volume level.The result is codec dependent.

    @return
        - @b AUDIO_VOL_LEVEL8: Total 8 volume level
        - @b AUDIO_VOL_LEVEL64: Total 64 volume level
*/
AUDIO_VOL_LEVEL aud_get_total_vol_level(void)
{
	return gAudio_total_Vollel;
}

/**
    Set PCM out volume

    This function set PCM out volume
    The result is codec dependent.

    @param[in] Vol      PCM out volume
    @return void
*/
void aud_set_volume(AUDIO_VOL Vol)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)Vol);
	}


	gAudio_curr_volume = Vol;

	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setVolume != NULL) {
			AudioCodecFunc[i].setVolume(Vol);
		}
	}
}

/**
    Get PCM out volume

    This function get PCM out volume
    The result is codec dependent.

    @return PCM out volume
*/
AUDIO_VOL aud_get_volume(void)
{
	if (aud_is_opened() == FALSE) {
		DBG_WRN("is closed\r\n");
	}

	return gAudio_curr_volume ;
}

/**
    Set PCM record total gain level

    This function set PCM out total volume level.The result is codec dependent.
    When the parameter "AUDIO_RECG_LEVEL" is set to AUDIO_RECG_LEVEL8, the valid
    parameter "Gain" range of API aud_setGain() is from AUDIO_GAIN_MUTE ~ AUDIO_GAIN_8.
    When the parameter "Audio_VolLevel" is set to AUDIO_VOL_LEVEL64, the valid parmeter
    "Gain" range of API aud_setGain() is from AUDIO_GAIN_MUTE ~ AUDIO_GAIN_20.

    @param[in] Audio_VolLevel   Total volume level. Available values are below:
                                - @b AUDIO_RECG_LEVEL8:  Total Record gain level = 8
                                - @b AUDIO_RECG_LEVEL20: Total Record gain level = 20

    @return void
*/
void aud_set_total_record_gain_level(AUDIO_RECG_LEVEL audio_gainlevel)
{
	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, audio_gainlevel);
	}

	gAudio_total_Gainlel = audio_gainlevel;
}

/**
    Set PCM record gain level

    This function get PCM record gain level.The result is codec dependent.

    @return
        - @b AUDIO_RECG_LEVEL8: Total 8 gain level
        - @b AUDIO_RECG_LEVEL20: Total 20 gain level
*/
AUDIO_RECG_LEVEL aud_get_Total_gain_level(void)
{
	return gAudio_total_Gainlel;
}

/**
    Set record gain

    This function set record gain
    The result is codec dependent.

    @param[in] Gain     Record gain. Available values are below:
                        - @b AUDIO_GAIN_0: Gain 0
                        - @b AUDIO_GAIN_1: Gain 1
                        - @b AUDIO_GAIN_2: Gain 2
                        - @b AUDIO_GAIN_3: Gain 3
                        - @b AUDIO_GAIN_4: Gain 4
                        - @b AUDIO_GAIN_5: Gain 5
                        - @b AUDIO_GAIN_6: Gain 6
                        - @b AUDIO_GAIN_7: Gain 7
                        - @b AUDIO_GAIN_MUTE: Mute
    @return void
*/
void aud_set_gain(AUDIO_GAIN Gain)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	gAudio_curr_gain = Gain;

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)Gain);
	}


	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setGain != NULL) {
			AudioCodecFunc[i].setGain(Gain);
		}
	}
}



/**
    Set record gain DB

    This function set record gain with db unit
    The result is codec dependent.

    @param[in] fDB     Record gain. (unit: db)

    @return void
*/
void aud_set_gaindb(INT32 fDB)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)fDB/2);
	}


	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setGainDB != NULL) {
			AudioCodecFunc[i].setGainDB(fDB);
		}
	}
}


/**
    Set sound effect

    This function set the sound effect when playback
    The result is codec dependent.

    @param[in] Effect   Sound effect. Available values are below:
                        - @b AUDIO_EFFECT_NONE
                        - @b AUDIO_EFFECT_3D_HALF
                        - @b AUDIO_EFFECT_3D_FULL
                        - @b AUDIO_EFFECT_ROCK
                        - @b AUDIO_EFFECT_POP
                        - @b AUDIO_EFFECT_JAZZ
                        - @b AUDIO_EFFECT_CLASSICAL
                        - @b AUDIO_EFFECT_DNACE
                        - @b AUDIO_EFFECT_HEAVY
                        - @b AUDIO_EFFECT_DISCO
                        - @b AUDIO_EFFECT_SOFT
                        - @b AUDIO_EFFECT_LIVE
                        - @b AUDIO_EFFECT_HALL
    @return void
*/
void aud_set_effect(AUDIO_EFFECT Effect)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)Effect);
	}


	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setEffect != NULL) {
			AudioCodecFunc[i].setEffect(Effect);
		}
	}
}

/**
    Select record source

    This function select the record source.
    The result is codec dependent.

    @param[in] RecSrc       Record source. Available values are below:
                            - @b AUDIO_RECSRC_MIC: record from (Microphone)
                            - @b AUDIO_RECSRC_CD: record from (CD In)
                            - @b AUDIO_RECSRC_VIDEO: record from (Video In)
                            - @b AUDIO_RECSRC_AUX: record from (Aux In)
                            - @b AUDIO_RECSRC_LINE: record from (Line In)
                            - @b AUDIO_RECSRC_STEREO_MIX: record from (Stereo Mix)
                            - @b AUDIO_RECSRC_MONO_MIX: record from (Mono Mix)
                            - @b AUDIO_RECSRC_PHONE: record from (Phone In)
    @return void
*/
void aud_set_record_source(AUDIO_RECSRC RecSrc)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	gAudioRecordSrc = RecSrc;

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)RecSrc);
	}

	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setRecordSource != NULL) {
			AudioCodecFunc[i].setRecordSource(RecSrc);
		}
	}
}


#endif

#if 1 // dbg command, maybe can use probe instead
//
//  Audio UART command table
//

/*
    print eac info to uart
*/
void eac_dump_info(void)
{
    int value;

/*
    AD part
*/
	DBG_WRN("===============AD = %s================\r\n", eac_get_ad_enable()?"ENABLE":"DISABLE");

    DBG_WRN("@ ALC                  = [%s]\r\n",eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)?"ENABLE":"DISABLE");
    value = -285+15*eac_get_ad_config(EAC_CONFIG_AD_ALC_TARGET_R);
    if(value%10)
        DBG_WRN("   -ALC-Target         = [%d.5]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_TARGET_R));
    else
        DBG_WRN("   -ALC-Target         = [%d.0]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_TARGET_R));

    value = -210+15*eac_get_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_R);
    if(value%10)
        DBG_WRN("   -ALC-MaxGain        = [%d.5]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_R));
    else
        DBG_WRN("   -ALC-MaxGain        = [%d.0]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_R));

    value = -210+15*eac_get_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_R);
    if(value%10)
        DBG_WRN("   -ALC-MinGain        = [%d.5]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_R));
    else
        DBG_WRN("   -ALC-MinGain        = [%d.0]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_R));

    DBG_WRN("   -ALC-TimeReso       = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_TRESO));
    DBG_WRN("   -ALC-Attack         = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_ATTACK_TIME));
    DBG_WRN("   -ALC-Decay          = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_DECAY_TIME));
    DBG_WRN("   -ALC-Hold           = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_HOLD_TIME));

    DBG_WRN("@ Noise Gate           = [%s]\r\n",eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_EN)?"ENABLE":"DISABLE");

    value = -765+15*eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_THD_R);
    if(value%10)
        DBG_WRN("   -NG-Threshold       = [%d.5]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_THD_R));
    else
        DBG_WRN("   -NG-Threshold       = [%d.0]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_THD_R));

    DBG_WRN("   -NG-RampSpeed       = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_TARGET_R));
    DBG_WRN("   -NG-TimeReso        = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_TRESO));
    DBG_WRN("   -NG-Attack          = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_NG_ATTACK_TIME));
    DBG_WRN("   -NG-Decay           = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_NG_DECAY_TIME));
    DBG_WRN("   -NG-Hold            = [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_NG_HOLD_TIME));

    DBG_WRN("@ DC Cancellation      = [%s]\r\n",eac_get_ad_config(EAC_CONFIG_AD_DCCAN_EN)?"ENABLE":"DISABLE");
    DBG_WRN("@ IIR filter           = [%s]\r\n",eac_get_ad_config(EAC_CONFIG_AD_IIR_OUT_R)?"ENABLE":"DISABLE");
    DBG_WRN("@ Boost                = [%d0] dB\r\n",(int)eac_get_ad_config(EAC_CONFIG_AD_PGABOOST_R));
    DBG_WRN("   -Boost Compensation = [%s]\r\n",eac_get_ad_config(EAC_CONFIG_AD_IIR_OUT_R)?"ENABLE":"DISABLE");

    if(eac_get_ad_config(EAC_CONFIG_AD_DGAIN1_R)){
        value = -975+5*eac_get_ad_config(EAC_CONFIG_AD_DGAIN1_R);
        if(value%10)
            DBG_WRN("@ Digital Gain 1       = [%d.5] dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_DGAIN1_R));
        else
            DBG_WRN("@ Digital Gain 1       = [%d.0] dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_DGAIN1_R));
    }else{
        DBG_WRN("@ Digital Gain 1       = digital mute [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_DGAIN1_R));
    }


    if(eac_get_ad_config(EAC_CONFIG_AD_DGAIN2_R)){
        value = -975+5*eac_get_ad_config(EAC_CONFIG_AD_DGAIN2_R);
        if(value%10)
            DBG_WRN("@ Digital Gain 2       = [%d.5] dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_DGAIN2_R));
        else
            DBG_WRN("@ Digital Gain 2       = [%d.0] dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_DGAIN2_R));
    }else{
        DBG_WRN("@ Digital Gain 2       = digital mute [0x%x]\r\n",(unsigned int)eac_get_ad_config(EAC_CONFIG_AD_DGAIN2_R));
    }
    DBG_WRN("=============================================\r\n");

    DBG_WRN("\r\n");

/*
    DA part
*/
    DBG_WRN("===============DA = %s================\r\n", eac_get_da_enable()?"ENABLE":"DISABLE");
    DBG_WRN("@ DC Cancellation      = [%s]\r\n",eac_get_da_config(EAC_CONFIG_DA_DCCAN_EN)?"ENABLE":"DISABLE");
    DBG_WRN("@ LR Data Mixer        = [%s]\r\n",eac_get_da_config(EAC_CONFIG_DA_DATAMIXER)?"ENABLE":"DISABLE");
    DBG_WRN("@ Zero Crossing        = [%s]\r\n",eac_get_da_config(EAC_CONFIG_DA_ZC_EN)?"ENABLE":"DISABLE");
    DBG_WRN("@ Speaker Mono         = [%s]\r\n",eac_get_da_config(EAC_CONFIG_DA_SPKR_MONO_EN)?"ENABLE":"DISABLE");

    if(eac_get_da_config(EAC_CONFIG_DA_DGAIN_R)){
        value = -975+5*eac_get_da_config(EAC_CONFIG_DA_DGAIN_R);
        if(value%10)
            DBG_WRN("@ Digital Gain         = [%d.5]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_da_config(EAC_CONFIG_DA_DGAIN_R));
        else
            DBG_WRN("@ Digital Gain         = [%d.0]dB [0x%x]\r\n",(int)value/10,(unsigned int)eac_get_da_config(EAC_CONFIG_DA_DGAIN_R));
    }else{
        DBG_WRN("@ Digital Gain         = digital mute [0x%x]\r\n",(unsigned int)eac_get_da_config(EAC_CONFIG_DA_DGAIN_R));
    }


    DBG_WRN("@ PGA Gain lineout     = [%x]\r\n",(unsigned int)eac_get_da_config(EAC_CONFIG_DA_LINEOUT_GAIN));
    DBG_WRN("@ PGA Gain speaker     = [%x]\r\n",(unsigned int)eac_get_da_config(EAC_CONFIG_DA_SPEAKER_GAIN));
	DBG_WRN("@ Output [SpkAB,SpkD,LineL,LineR] = [%d,%d,%d,%d]\r\n", (int)eac_get_dac_output(EAC_OUTPUT_SPK), (int)eac_get_dac_output(EAC_OUTPUT_CLSD_SPK), (int)eac_get_dac_output(EAC_OUTPUT_LINE_L), (int)eac_get_dac_output(EAC_OUTPUT_LINE_R));
    DBG_WRN("=============================================\r\n");
}

/**
    Dump audio library settings

    Dump audio library state and settings

    @return void
*/
void aud_printSetting(void)
{
    DBG_WRN("===============AUDIO INFORMATION============\r\n");
    DBG_WRN("@ Default Codec = ");
	if (AudioCodecFunc[AUDIO_CODECSEL_DEFAULT].codecType == AUDIO_CODEC_TYPE_EMBEDDED) {
		DBG_WRN("Embedded\r\n");
		DBG_WRN("@ Active codec  = %s\r\n", AudioSelectedCodec ? "HDMI" : "Embedded");
	} else {
		DBG_WRN("External\r\n");
		DBG_WRN("@ Active codec = %s\r\n", AudioSelectedCodec ? "HDMI" : "External");
	}

	DBG_WRN("@ Total volume %d, current volume %d\r\n", (int)gAudio_total_Vollel, (int)gAudio_curr_volume);
	DBG_WRN("@ Record gain lvl = %d\r\n", (int)gAudio_curr_gain);
    switch(gAudioOutputDst){
        case AUDIO_OUTPUT_SPK: DBG_WRN("@ Output to AUDIO_OUTPUT_SPK\r\n"); break;
        case AUDIO_OUTPUT_LINE: DBG_WRN("@ Output to AUDIO_OUTPUT_LINE\r\n"); break;
        default: DBG_WRN("@ Output to 0x%x\r\n", (unsigned int)gAudioOutputDst); break;
    }


	DBG_WRN("@ Sampling rate: aud-%d hdmi-%d\t\r\n", (int)AudCurCodecSR, (int)AudCurHdmiSR);
    DBG_WRN("============================================\r\n");
    DBG_WRN("\r\n");

    DBG_WRN("===============AUDIO FEATURE SETTING========\r\n");
	DBG_WRN("@ AUDIO_FEATURE_INTERFACE_ALWAYS_ACTIVE: %d\r\n", (AudioIntFlag & AUDIO_INT_FLAG_CLKALWAYS_ON) > 0);
	DBG_WRN("@ AUDIO_FEATURE_PLAY2ALLCODEC:           %d\r\n", (int)bPlay2AllCodec);
	DBG_WRN("@ AUDIO_FEATURE_DISCONNECT_HDMI:         %d\r\n", !bConnHDMI);
	DBG_WRN("@ AUDIO_FEATURE_LINE_PWR_ALWAYSON:       %d\r\n", (AudioIntFlag & AUDIO_INT_FLAG_LINEPWR_ALWAYSON) > 0);
	DBG_WRN("@ AUDIO_FEATURE_HP_PWR_ALWAYSON:         %d\r\n", (AudioIntFlag & AUDIO_INT_FLAG_HPPWR_ALWAYSON) > 0);
	DBG_WRN("@ AUDIO_FEATURE_SPK_PWR_ALWAYSON:        %d\r\n", (AudioIntFlag & AUDIO_INT_FLAG_SPKPWR_ALWAYSON) > 0);
	DBG_WRN("@ AUDIO_FEATURE_AUTOPINMUX:              %d\r\n", (AudioIntFlag & AUDIO_INT_FLAG_AUTOPINMUX) > 0);
	DBG_WRN("@ AUDIO_FEATURE_MCLK_AUTOPINMUX:         %d\r\n", (AudioIntFlag & AUDIO_INT_FLAG_MCLKAUTOPINMUX) > 0);
    DBG_WRN("============================================\r\n");
    DBG_WRN("\r\n");

    DBG_WRN("===============AUDIO Transit Object========\r\n");
	aud_print_tstx1();
	aud_print_tstx2();
	aud_print_tsrx();
    DBG_WRN("============================================\r\n");
    DBG_WRN("\r\n");

	if (AudioCodecFunc[AUDIO_CODECSEL_DEFAULT].codecType == AUDIO_CODEC_TYPE_EMBEDDED) {
		eac_dump_info();
	}

}

void aud_printSetting_obj_releated(void)
{
    DBG_WRN("===============AUDIO Transit Object========\r\n");
	aud_print_tstx1();
	aud_print_tstx2();
	aud_print_tsrx();
    DBG_WRN("============================================\r\n");
}


#endif

#if 0
/*
    print audio info to uart
*/
static BOOL aud_printInfoToUart(CHAR *strCmd)
{
	//aud_printSetting();
	return TRUE;
}
#endif

