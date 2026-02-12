/*
    Audio Transcrive Channel Playback Channel 1 (TX1) driver

    This file is the driver of audio transmit(playback) channel 1.

    @file       audio_tsrx.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifdef __KERNEL__
#include "kwrap/type.h"//a header for basic variable type
#include <mach/rcw_macro.h>
#endif
#include "audio_int.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"

/**
    @addtogroup mISYSAud
*/
//@{

VK_DEFINE_SPINLOCK(my_lock_rx);
#define loc_cpu_rx(myflags)   vk_spin_lock_irqsave(&my_lock_rx, myflags)
#define unl_cpu_rx(myflags)   vk_spin_unlock_irqrestore(&my_lock_rx, myflags)

#define TIMEOUT_CNT 100


static BOOL     audRxOpened             = FALSE;

static UINT32   uiBakRxTimeCode         = 0;    // backup time code value after pause
static UINT32   uiLastRxTimeCode        = 0;    // backup time code for reference after aud_stop()

static UINT32   uiAudRxTimeCodeTrig     = 0;    // store time code trigger pass to aud_setTimecodeTrigger()
static UINT32   uiAudRxTimeCodeOffset   = 0;    // store time code offset pass to aud_setTimecodeOffset()

static BOOL     bAudioExpandRx          = FALSE;

UINT32          uibuffer_insert_number_rx  = 0;
UINT32			tmp_count_rx = 0;


AUDIO_CH        AudioRxCH               = AUDIO_CH_STEREO;
AUDIO_SR        AudSrcSRRx              = AUDIO_SR_8000;
extern ER               dai_lock(void);
extern ER               dai_unlock(void);
extern AUDIO_RECSRC     gAudioRecordSrc;

#if defined (__KERNEL__)
static BOOL audtsrx_first_boot = TRUE;
#endif

#if 1
//
//  Internal APIs
//


/*
    Set TX1 Channel Number Config
*/
static void aud_set_chcfg_rx(void)
{
	switch (AudioRxCH) {
	case AUDIO_CH_LEFT: {
			dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,                   DAI_CH_MONO_LEFT);

			if (bAudioExpandRx == TRUE) {
				dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_STEREO);
			} else {
				dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_MONO);
			}
		}
		break;

	case AUDIO_CH_RIGHT: {
			dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,                   DAI_CH_MONO_RIGHT);

			if (bAudioExpandRx == TRUE) {
				dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_STEREO);
			} else {
				dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_MONO);
			}
		}
		break;

	case AUDIO_CH_DUAL_MONO: {
			dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,                   DAI_CH_DUAL_MONO);

			if (bAudioExpandRx == TRUE) {
				dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_STEREO);
			} else {
				dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_MONO);
			}
		}
		break;

	case AUDIO_CH_STEREO:
	default: {
			dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,                DAI_DRAMPCM_STEREO);
			dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,               DAI_CH_STEREO);
		}
		break;

	}

	aud_set_codec_channel(TSRX1, AudioRxCH);
}

#endif
#if 1
/*
    Open RX record
*/
ER aud_open_rx(void)
{
	if (audRxOpened == TRUE) {
		DBG_WRN("audTs Obj RX already opened!\r\n");
		return E_OK;
	}

	// Add Audio Open here
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && audtsrx_first_boot) {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (builtin_init) {
			audRxOpened = TRUE;
			return E_OK;
		}
	}
#endif

	// Reset TC Offset
	dai_set_rx_config(DAI_RXCFG_ID_TIMECODE_OFS, 0);

	audRxOpened = TRUE;
	return E_OK;
}

/*
    Close RX record
*/
ER aud_close_rx(void)
{
	unsigned long flag;

	if (!audRxOpened) {
		return E_OK;
	}

	audRxOpened = FALSE;
	loc_cpu_rx(flag);
    uibuffer_insert_number_rx = 0;
	unl_cpu_rx(flag);

#if defined (__KERNEL__)
	audtsrx_first_boot = FALSE;
#endif
	return E_OK;
}

/*
    Check if RX record is Opened
*/
BOOL aud_is_opened_rx(void)
{
	return audRxOpened;
}

/*
    Check if RX record is Busy
*/
BOOL aud_is_busy_rx(void)
{
	return (dai_is_rx_enable() || dai_is_rx_dma_enable());
}
#endif
#if 1

/*
    Start RX playback
*/
ER aud_playback_rx(void)
{
	DBG_ERR("No Spport Playback!\r\n");
	return E_NOSPT;
}

/*
    Start RX record Preset
*/
ER aud_record_preset_rx(void)
{
	if ((aud_is_opened() == FALSE) || (audRxOpened == FALSE)) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	if (dai_is_rx_enable()) {
		DBG_ERR("Rx is busy\r\n");
		return E_OACV;
	}

	// Enable Codec to Record
	if (AudioCodecFunc[AudioSelectedCodec].recordPreset != NULL) {
		AudioCodecFunc[AudioSelectedCodec].recordPreset();
	}

	return E_OK;
}

/*
    Start RX record
*/
ER aud_record_rx(void)
{
	unsigned long flag;

	dai_lock();
	if ((aud_is_opened() == FALSE) || (audRxOpened == FALSE)) {
		DBG_ERR("is closed\r\n");
		dai_unlock();
		return E_OACV;
	}

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && audtsrx_first_boot) {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (builtin_init) {
			pAudioRealBufferRear[TSRX1]    = pAudioPseudoBufferRear[TSRX1];
			pAudioRealBufferDone[TSRX1]    = pAudioPseudoBufferDone[TSRX1];
			pAudioRealBufferFront[TSRX1]   = pAudioPseudoBufferFront[TSRX1];

			pAudioRealBufferRear[TSRX2]    = pAudioPseudoBufferRear[TSRX2];
			pAudioRealBufferDone[TSRX2]    = pAudioPseudoBufferDone[TSRX2];
			pAudioRealBufferFront[TSRX2]   = pAudioPseudoBufferFront[TSRX2];

			// Set DMA starting address and size
			//dai_set_rx_dma_para(0, pAudioRealBufferRear[TSRX1]->uiAddress, pAudioRealBufferRear[TSRX1]->uiSize >> 2);
			loc_cpu_rx(flag);
		    uibuffer_insert_number_rx++;
			unl_cpu_rx(flag);

			// Set next DMA starting address and size
			if (pAudioRealBufferRear[TSRX1]->pNext != pAudioRealBufferFront[TSRX1]) {
				//dai_set_rx_dma_para(0, pAudioRealBufferRear[TSRX1]->pNext->uiAddress, pAudioRealBufferRear[TSRX1]->pNext->uiSize >> 2);
				loc_cpu_rx(flag);
		        uibuffer_insert_number_rx++;
				unl_cpu_rx(flag);
			}

			// Set Audio operation mode to "Receive" (Record) mode
			AudioState |= AUDIO_STATE_RX;

			dai_unlock();
			return E_OK;
		}
	}
#endif

	if (dai_is_rx_dma_enable()) {
		DBG_ERR("is busy\r\n");
		dai_unlock();
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	// Enable Codec to Record
	if (AudioCodecFunc[AudioSelectedCodec].record != NULL) {
		AudioCodecFunc[AudioSelectedCodec].record();
	}

	pAudioRealBufferRear[TSRX1]    = pAudioPseudoBufferRear[TSRX1];
	pAudioRealBufferDone[TSRX1]    = pAudioPseudoBufferDone[TSRX1];
	pAudioRealBufferFront[TSRX1]   = pAudioPseudoBufferFront[TSRX1];

	pAudioRealBufferRear[TSRX2]    = pAudioPseudoBufferRear[TSRX2];
	pAudioRealBufferDone[TSRX2]    = pAudioPseudoBufferDone[TSRX2];
	pAudioRealBufferFront[TSRX2]   = pAudioPseudoBufferFront[TSRX2];

	if ((dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN)) && (dai_get_i2s_config(DAI_I2SCONFIG_ID_OPMODE) == DAI_OP_SLAVE) && (!dai_get_i2s_config(DAI_I2SCONFIG_ID_SLAVEMATCH))) {
		DBG_ERR("I2S CKRatio Not Match\r\n");
	}

	// Set DMA starting address and size
	dai_set_rx_dma_para(0, pAudioRealBufferRear[TSRX1]->uiAddress, pAudioRealBufferRear[TSRX1]->uiSize >> 2);
	loc_cpu_rx(flag);
    uibuffer_insert_number_rx++;
	unl_cpu_rx(flag);

	if (dai_get_rx_config(DAI_RXCFG_ID_CHANNEL) == DAI_CH_DUAL_MONO) {
		dai_set_rx_dma_para(1, pAudioRealBufferRear[TSRX2]->uiAddress, pAudioRealBufferRear[TSRX2]->uiSize >> 2);
	}

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


	// Set next DMA starting address and size
	if (pAudioRealBufferRear[TSRX1]->pNext != pAudioRealBufferFront[TSRX1]) {
		dai_set_rx_dma_para(0, pAudioRealBufferRear[TSRX1]->pNext->uiAddress, pAudioRealBufferRear[TSRX1]->pNext->uiSize >> 2);
		loc_cpu_rx(flag);
        uibuffer_insert_number_rx++;
		unl_cpu_rx(flag);
	}

	if (dai_get_rx_config(DAI_RXCFG_ID_CHANNEL) == DAI_CH_DUAL_MONO) {
		if (pAudioRealBufferRear[TSRX2]->pNext != pAudioRealBufferFront[TSRX2]) {
			dai_set_rx_dma_para(1, pAudioRealBufferRear[TSRX2]->pNext->uiAddress, pAudioRealBufferRear[TSRX2]->pNext->uiSize >> 2);
		}
	}

	// Enable Interrupt
	dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_RX1BWERR_INT | DAI_RX2BWERR_INT | DAI_RX1DMADONE_INT | DAI_RX2DMADONE_INT | DAI_RXTCLATCH_INT);

	// Enable record
	dai_enable_rx(TRUE);

	// Set Audio operation mode to "Receive" (Record) mode
	AudioState |= AUDIO_STATE_RX;

	dai_unlock();
	return E_OK;
}


/*
    Stop RX record
*/
ER aud_stop_rx(void)
{
	int timeout_cnt;

	dai_lock();
	// Prevent calling aud_stop() multiple times
	if ((aud_is_opened() == FALSE) || (dai_is_rx_dma_enable() == FALSE)) {
		DBG_ERR("is closed\r\n");
		dai_unlock();
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	dai_clr_flg(DAI_RXSTOP_INT);

	// Disable Audio
	if (dai_is_rx_enable() == TRUE) {
		UINT32 uiStart = 0;

		// Prevent Stop after Start immediately
		dai_get_rx_dma_para(0, &uiStart, NULL);
		while (uiStart == dai_get_rx_dma_curaddr(0));

		// Enable RX Stop interrupt enable
		dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_RXSTOP_INT);

		// Disable RX and then wait STOP interrupt
		dai_enable_rx(FALSE);
#ifdef __KERNEL__
		timeout_cnt = 0;
		while(dai_is_rx_enable()){
			timeout_cnt++;
			vos_util_delay_ms(10);
			if(timeout_cnt > TIMEOUT_CNT){
				DBG_ERR("dai wait Rx disable timeout!!\r\n");
				break;
			}

		};
#else
		dai_wait_interrupt(DAI_RXSTOP_INT);
#endif
	}

	// Time code value will be cleared after DMA is disabled
	// Backup it to variable for later reference of aud_getTimecodeValue()
	uiLastRxTimeCode = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_VAL);

	// Disable DMA
	dai_enable_rx_dma(FALSE);
	timeout_cnt = 0;
	while (dai_is_rx_dma_enable()){
		timeout_cnt++;
		vos_util_delay_ms(10);
		if(timeout_cnt > TIMEOUT_CNT){
			DBG_ERR("dai wait Rx dma disable timeout!!\r\n");
			break;
		}
	};

	// Stop Codec
	if (AudioCodecFunc[AudioSelectedCodec].stopRecord != NULL) {
		AudioCodecFunc[AudioSelectedCodec].stopRecord();
	}

	// Save valid PCM data size of last buffer in record mode
	pAudioPseudoBufferRear[TSRX1]->uiValidSize =
		pAudioRealBufferRear[TSRX1]->uiValidSize   = dai_get_rx_dma_curaddr(0) - pAudioRealBufferRear[TSRX1]->uiAddress;

	if (dai_get_rx_config(DAI_RXCFG_ID_CHANNEL) == DAI_CH_DUAL_MONO) {
		pAudioPseudoBufferRear[TSRX2]->uiValidSize =
			pAudioRealBufferRear[TSRX2]->uiValidSize   = dai_get_rx_dma_curaddr(1) - pAudioRealBufferRear[TSRX2]->uiAddress;
	}

	// Disable RX ALL Interrupt
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN,  AUDIO_RX_FLAGALL);
	dai_set_config(DAI_CONFIG_ID_CLR_INTSTS, AUDIO_RX_FLAGALL);

	AudioState &= ~AUDIO_STATE_RX;

	dai_unlock();
	return E_OK;
}

/*
    Pause RX record
*/
ER aud_pause_rx(void)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	// If already disable. No need to pause. Just return.
	if (dai_is_rx_enable() == FALSE) {
		return E_OK;
	}

	// Output Debug Msg
	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}


	dai_clr_flg(DAI_RXSTOP_INT);

	// Enable STOP interrupt
	dai_set_config(DAI_CONFIG_ID_SET_INTEN,  DAI_RXSTOP_INT);

	// Diable Audio
	dai_enable_rx(FALSE);

	// Wait for Audio really stopped
	dai_wait_interrupt(DAI_RXSTOP_INT);

	// Make Sure really STOP
	while (dai_is_rx_enable());

	// Disable STOP interrupt
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN,  DAI_RXSTOP_INT);

	// Store Current TimeCode for resume usages
	uiBakRxTimeCode = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_VAL);

#if _EMULATION_
	DBG_WRN("RX1 pause addr = 0x%08X\r\n", (unsigned int)dai_get_rx_dma_curaddr(0));
	DBG_WRN("RX2 pause addr = 0x%08X\r\n", (unsigned int)dai_get_rx_dma_curaddr(1));
#endif

	return E_OK;
}

/*
    Resume RX record
*/
ER aud_resume_rx(void)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (dai_is_rx_enable() == TRUE) {
		return E_OK;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)uiBakRxTimeCode);
	}


	// Restore TimeCode Offset
	dai_set_rx_config(DAI_RXCFG_ID_TIMECODE_OFS, uiBakRxTimeCode);

	// Enable RX
	dai_enable_rx(TRUE);
	return E_OK;
}

#endif

/*
    Configure RX record channel number
*/
void aud_set_channel_rx(AUDIO_CH Channel)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)Channel);
	}

	AudioRxCH = Channel;

	// Configure DAI&Codec Channels Settings
	aud_set_chcfg_rx();
}

/*
	Set External Audio Codec TDM total channel number
*/
void aud_set_tdm_channel_rx(AUDIO_TDMCH TDM)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if ((!dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN)) && (TDM != AUDIO_TDMCH_2CH) && (gAudioRecordSrc != AUDIO_RECSRC_DMIC)) {
		/* Internal audio codec */
		DBG_WRN("Embedded codec only support 2CH\r\n");
		TDM = AUDIO_TDMCH_2CH;
	}

	dai_set_rx_config(DAI_RXCFG_ID_TOTAL_CH, TDM);
}

/*
    Set sampling rate

    This function set the sampling rate
    It is the sampling rate of playback/record PCM data on DRAM
    (The result is codec dependent)

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

    @return void
*/
ER aud_set_samplerate_rx(AUDIO_SR SamplingRate)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)SamplingRate);
	}

	AudSrcSRRx = SamplingRate;

	for (i = 0; i < dim(AudioCodecFunc); i++) {
		aud_set_codec_samplerate(&AudioCodecFunc[i], SamplingRate);
	}

	return E_OK;
}


/*
    Configure RX record ReSample Information
*/
BOOL aud_set_resampleinfo_rx(PAUDIO_RESAMPLE_INFO pResampleInfo)
{
	DBG_ERR("Reord No Spport Resample!\r\n");
	return FALSE;
}


/*
    Configure RX Feature
*/
void  aud_set_feature_rx(AUDTS_FEATURE Feature, BOOL bEnable)
{
	if (!audRxOpened) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: Feature=%d  EN=%d\r\n", __func__, (int)Feature, (int)bEnable);
	}

	switch (Feature) {
	case AUDTS_FEATURE_TIMECODE_HIT: {
			if (bEnable) {
				dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_RXTCHIT_INT);
			} else {
				dai_set_config(DAI_CONFIG_ID_CLR_INTEN, DAI_RXTCHIT_INT);
			}
		}
		break;

	case AUDTS_FEATURE_TIMECODE_LATCH: {
			if (bEnable) {
				dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_RXTCLATCH_INT);
			} else {
				dai_set_config(DAI_CONFIG_ID_CLR_INTEN, DAI_RXTCLATCH_INT);
			}
		}
		break;

	case AUDTS_FEATURE_AVSYNC: {
			dai_set_config(DAI_CONFIG_ID_AVSYNC_EN, bEnable);
		}
		break;

	case AUDTS_FEATURE_RECORD_PCM_EXPAND: {
			bAudioExpandRx = bEnable;

			// Configure DAI&Codec Channels Settings
			aud_set_chcfg_rx();
		}
		break;


	default:
		DBG_WRN("function no support! (%d)\r\n", (int)Feature);
		break;
	}

}

/*
    Set RX config settings
*/
void aud_set_config_rx(AUDTS_CFG_ID CfgSel, UINT32 uiSetting)
{
	if (!audRxOpened) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: Param=%d  Setting=%d\r\n", __func__, (int)CfgSel, (int)uiSetting);
	}

	switch (CfgSel) {
	case AUDTS_CFG_ID_TIMECODE_TRIGGER: {
			uiAudRxTimeCodeTrig = uiSetting;

			dai_set_rx_config(DAI_RXCFG_ID_TIMECODE_TRIG, uiSetting);
		}
		break;

	case AUDTS_CFG_ID_TIMECODE_OFFSET: {
			uiAudRxTimeCodeOffset = uiSetting;

			dai_set_rx_config(DAI_RXCFG_ID_TIMECODE_OFS, uiSetting);
		}
		break;

	case AUDTS_CFG_ID_EVENT_HANDLE: {
			pAudEventHandleRx = (AUDIO_CB)uiSetting;
		}
		break;

	case AUDTS_CFG_ID_PCM_BITLEN: {
			dai_set_rx_config(DAI_RXCFG_ID_PCMLEN,        uiSetting);
		}
		break;

	default:
		DBG_WRN("function no support! (%d)\r\n", (int)CfgSel);
		break;
	}

}

/*
    Get RX config settings
*/
UINT32 aud_get_config_rx(AUDTS_CFG_ID CfgSel)
{
	UINT32 Ret = 0;

	switch (CfgSel) {
	case AUDTS_CFG_ID_TIMECODE_TRIGGER: {
			Ret = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_TRIG);
		}
		break;

	case AUDTS_CFG_ID_TIMECODE_OFFSET: {
			Ret = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_OFS);
		}
		break;

	case AUDTS_CFG_ID_TIMECODE_VALUE: {
			if (dai_is_rx_dma_enable() == FALSE) {
				// If DMA is disabled, time code read from DAI will always zero
				// Refer to time code backup time aud_stop()
				Ret = uiLastRxTimeCode;
			} else {
				Ret = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_VAL);
			}
		}
		break;

	case AUDTS_CFG_ID_EVENT_HANDLE: {
			Ret = (UINT32)pAudEventHandleRx;
		}
		break;


	default:
		break;
	}


	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: Param=%d  Setting=%d\r\n", __func__, (int)CfgSel, (int)Ret);
	}

	return Ret;
}

#if 1
/*
    Reset RX Audio Buffer Queue
*/
void aud_reset_buf_queue_rx(AUDIO_QUEUE_SEL QueueSel)
{
	if (QueueSel > 2) {
		DBG_ERR("Rx CH must be 0 / 1 / 2\r\n");
		return;
	}

	aud_reset_queue((AUDTSCH)QueueSel);
}

/*
    Add RX Audio Buffer Queue
*/
BOOL aud_add_buf_to_queue_rx(AUDIO_QUEUE_SEL QueueSel, PAUDIO_BUF_QUEUE pAudioBufQueue)
{
	BOOL Ret;
	unsigned long flag;

	if (QueueSel > 2) {
		DBG_ERR("Rx CH must be 0 / 1 / 2\r\n");
		return FALSE;
	}

	Ret = aud_add_to_queue((AUDTSCH)QueueSel, pAudioBufQueue);

	if (Ret == TRUE) {
		pAudioRealBufferFront[QueueSel]->uiAddress    = pAudioBufQueue->uiAddress;
		pAudioRealBufferFront[QueueSel]->uiSize       = pAudioBufQueue->uiSize;

		if (pAudioBufQueue->uiSize & 0x03) {
			DBG_ERR("buf size not word align: 0x%x\r\n", (unsigned int)pAudioBufQueue->uiSize);
		}
		loc_cpu_rx(flag);
		pAudioRealBufferFront[QueueSel]                   = pAudioRealBufferFront[QueueSel]->pNext;
		tmp_count_rx= uibuffer_insert_number_rx;
		unl_cpu_rx(flag);

        if(tmp_count_rx == 1) {
            dai_set_rx_dma_para(0, pAudioBufQueue->uiAddress, pAudioBufQueue->uiSize >> 2);
			loc_cpu_rx(flag);
            uibuffer_insert_number_rx++;
			unl_cpu_rx(flag);
        }
	}

	return Ret;
}

/*
    Check if RX Audio Buffer Queue is FULL
*/
BOOL aud_is_buf_queue_full_rx(AUDIO_QUEUE_SEL QueueSel)
{
	if (QueueSel > 2) {
		DBG_ERR("Rx CH must be 0 / 1 / 2\r\n");
		return FALSE;
	}

	return aud_is_queue_full((AUDTSCH)QueueSel);
}

/*
    Get RX Audio Done Buffer Queue
*/
PAUDIO_BUF_QUEUE aud_get_done_buf_from_queue_rx(AUDIO_QUEUE_SEL QueueSel)
{
	if (QueueSel > 2) {
		DBG_ERR("Rx CH must be 0 / 1 / 2\r\n");
		return NULL;
	}

	return aud_get_done_queue((AUDTSCH)QueueSel);
}

/*
    Get RX Current Audio Buffer Working Queue
*/
PAUDIO_BUF_QUEUE aud_get_cur_buf_from_queue_rx(AUDIO_QUEUE_SEL QueueSel)
{
	if (QueueSel > 2) {
		DBG_ERR("Rx CH must be 0 / 1 / 2\r\n");
		return NULL;
	}

	return aud_get_cur_queue((AUDTSCH)QueueSel);
}

#endif

/*
    Dump RX record information
*/
void aud_print_tsrx(void)
{
    DBG_DUMP("---------------Rx  = %s----------------\r\n", dai_is_rx_enable()?"ENABLE":"DISABLE");
	DBG_DUMP("@ CHANNEL=%d | EXPAND=%d | Opened=%d\r\n", (int)AudioRxCH,bAudioExpandRx,(int)audRxOpened);
	DBG_DUMP("@ TimeCode Trig=0x%X Offset=0x%X\r\n", (unsigned int)uiAudRxTimeCodeTrig, (unsigned int)uiAudRxTimeCodeOffset);
	DBG_DUMP("@ uibuffer_insert_number_rx = %d\r\n", (int)uibuffer_insert_number_rx);
}

//@}
