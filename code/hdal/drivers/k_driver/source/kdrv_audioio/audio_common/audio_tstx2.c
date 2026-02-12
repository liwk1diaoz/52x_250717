/*
    Audio Transcrive Channel Playback Channel 2 (TX2) driver

    This file is the driver of audio transmit(playback) channel 2.

    @file       audio_tstx2.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifdef __KERNEL__
#include "kwrap/type.h"//a header for basic variable type
#include <mach/rcw_macro.h>
#endif
#include "audio_int.h"

/**
    @addtogroup mISYSAud
*/
//@{

VK_DEFINE_SPINLOCK(my_lock_tx2);
#define loc_cpu_tx2(myflags)   vk_spin_lock_irqsave(&my_lock_tx2, myflags)
#define unl_cpu_tx2(myflags)   vk_spin_unlock_irqrestore(&my_lock_tx2, myflags)

#define TIMEOUT_CNT 100


static BOOL     audTx2Opened            = FALSE;

static UINT32   uiLastTimeCode2          = 0;  // backup time code for reference after aud_stop()

#if _EMULATION_
static BOOL     bAudioExpandTx2         = FALSE;
#else
static BOOL     bAudioExpandTx2         = TRUE;
#endif

AUDIO_CH        AudioTx2CH              = AUDIO_CH_STEREO;
AUDIO_SR        AudSrcSRTx2             = AUDIO_SR_8000;

UINT32          uibuffer_insert_number_tx2  = 0;
UINT32			tmp_count_tx2 =0;

extern ER       dai_lock(void);
extern ER       dai_unlock(void);

#if 1

//
//  Internal APIs
//

/*
    Set TX2 Channel Number Config
*/
static void aud_set_chcfg_tx2(void)
{
	AUDIO_CH    Channel;

	Channel = AudioTx2CH;

	switch (AudioTx2CH) {
	case AUDIO_CH_LEFT: {
			dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,      DAI_DRAMPCM_MONO);

			if (bAudioExpandTx2 == TRUE) {
				dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_STEREO);

				Channel = AUDIO_CH_STEREO;
			} else {
				dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_LEFT);
			}
		}
		break;

	case AUDIO_CH_RIGHT: {
			dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,      DAI_DRAMPCM_MONO);

			if (bAudioExpandTx2 == TRUE) {
				dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_STEREO);

				Channel = AUDIO_CH_STEREO;
			} else {
				dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, DAI_CH_MONO_RIGHT);
			}
		}
		break;

	case AUDIO_CH_STEREO:
	default: {
			dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,      DAI_DRAMPCM_STEREO);
			dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL,     DAI_CH_STEREO);
		}
		break;

	}

	aud_set_codec_channel(TSTX2, Channel);
}

#endif

#if 1
/*
    Open TX2 TsObj
*/
ER aud_open_tx2(void)
{
	if (audTx2Opened == TRUE) {
		DBG_WRN("audTs Obj TX2 already opened!\r\n");
		return E_OK;
	}

	// Add Audio Open here
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}




	audTx2Opened = TRUE;
	return E_OK;
}

/*
    Close TX2 TsObj
*/
ER aud_close_tx2(void)
{
	unsigned long flag;

	if (!audTx2Opened) {
		return E_OK;
	}

	audTx2Opened = FALSE;
	loc_cpu_tx2(flag);
    uibuffer_insert_number_tx2 = 0;
	unl_cpu_tx2(flag);
	return E_OK;
}

/*
    Check if TX2 TsObj is Opened
*/
BOOL aud_is_opened_tx2(void)
{
	return audTx2Opened;
}

/*
    Check if TX2 TsObj is Busy
*/
BOOL aud_is_busy_tx2(void)
{
	return (dai_is_tx_enable(DAI_TXCH_TX2) || dai_is_tx_dma_enable(DAI_TXCH_TX2));
}
#endif
#if 1

/*
    Start TX2 playback
*/
ER aud_playback_tx2(void)
{
	unsigned long flag;

    dai_lock();

	if ((aud_is_opened() == FALSE) || (audTx2Opened == FALSE)) {
		DBG_ERR("is closed\r\n");
        dai_unlock();
		return E_OACV;
	}

	if (dai_is_tx_dma_enable(DAI_TXCH_TX2)) {
		DBG_ERR("is busy\r\n");
        dai_unlock();
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	if (bPlay2AllCodec == FALSE) {
		if (AudioCodecFunc[AudioSelectedCodec].chkSamplingRate != NULL) {
			if ((AudioSelectedCodec != AUDIO_CODECSEL_HDMI) ||
				(bConnHDMI)) {
				if (AudioCodecFunc[AudioSelectedCodec].chkSamplingRate() == FALSE) {
                    dai_unlock();
                    return E_SYS;
				}
			}
		}
	} else {
		UINT32 i;

		for (i = 0; i < dim(AudioCodecFunc); i++) {
			if (AudioCodecFunc[i].chkSamplingRate != NULL) {
				if ((i != AUDIO_CODECSEL_HDMI) ||
					(bConnHDMI)) {
					if (AudioCodecFunc[i].chkSamplingRate() == FALSE) {
                        dai_unlock();
						return E_SYS;
					}
				}
			}
		}
	}

	// Enable Codec to playback
	if (bPlay2AllCodec == FALSE) {
		if (AudioCodecFunc[AudioSelectedCodec].playback != NULL) {
			AudioCodecFunc[AudioSelectedCodec].playback();
		}
	} else {
		UINT32 i;

		for (i = 0; i < dim(AudioCodecFunc); i++) {
			if (AudioCodecFunc[i].playback != NULL) {
				AudioCodecFunc[i].playback();
			}
		}
	}

	if ((dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN)) && (dai_get_i2s_config(DAI_I2SCONFIG_ID_OPMODE) == DAI_OP_SLAVE) && (!dai_get_i2s_config(DAI_I2SCONFIG_ID_SLAVEMATCH))) {
		DBG_ERR("I2S CKRatio Not Match\r\n");
	}

	// Set DMA starting address and size
	dai_set_tx_dma_para(1, pAudioRealBufferRear[TSTX2]->uiAddress, pAudioRealBufferRear[TSTX2]->uiSize >> 2);
	loc_cpu_tx2(flag);
    uibuffer_insert_number_tx2++;
	unl_cpu_tx2(flag);


	// Clear interrupt status, and interrupt flag
	dai_set_config(DAI_CONFIG_ID_CLR_INTSTS, AUDIO_TX2_FLAGALL);

	dai_clr_flg(AUDIO_TX2_FLAGALL);

	dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TX2DMALOAD_INT);

	// Enable DMA
	dai_enable_tx_dma(DAI_TXCH_TX2, TRUE);

	// Wait for DMA transfer start
	dai_wait_interrupt(DAI_TX2DMALOAD_INT);
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN, DAI_TX2DMALOAD_INT);


	// Set next DMA starting address and size
	if (pAudioRealBufferRear[TSTX2]->pNext != pAudioRealBufferFront[TSTX2]) {
		dai_set_tx_dma_para(1, pAudioRealBufferRear[TSTX2]->pNext->uiAddress, pAudioRealBufferRear[TSTX2]->pNext->uiSize >> 2);
		loc_cpu_tx2(flag);
        uibuffer_insert_number_tx2++;
		unl_cpu_tx2(flag);
	}

	// Enable Interrupt
	dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TX2BWERR_INT | DAI_TX2DMADONE_INT);

	// Enable playback
	dai_enable_tx(DAI_TXCH_TX2, TRUE);

	// Set Audio operation mode to "Transmit" (Playback) mode
	AudioState |= AUDIO_STATE_TX2;

    dai_unlock();

	return E_OK;
}

/*
    Start TX2 record Preset
*/
ER aud_record_preset_tx2(void)
{
	DBG_ERR("No Spport Record!\r\n");
	return E_NOSPT;
}

/*
    Start TX2 record
*/
ER aud_record_tx2(void)
{
	DBG_ERR("No Spport Record!\r\n");
	return E_NOSPT;
}


/*
    Stop TX2 playback
*/
ER aud_stop_tx2(void)
{
	int timeout_cnt;

    dai_lock();

	// Prevent calling aud_stop() multiple times
	if ((aud_is_opened() == FALSE) || (dai_is_tx_dma_enable(DAI_TXCH_TX2) == FALSE)) {
		DBG_ERR("is closed\r\n");
        dai_unlock();
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	dai_clr_flg(DAI_TX2STOP_INT);

	// Diable Audio
	if (dai_is_tx_enable(DAI_TXCH_TX2) == TRUE) {
		UINT32 uiStart = 0;

		// Prevent Stop after Start immediately
		dai_get_tx_dma_para(DAI_TXCH_TX2, &uiStart, NULL);
		while (uiStart == dai_get_tx_dma_curaddr(DAI_TXCH_TX2));

		// Enable TX1 Stop interrupt enable
		dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TX2STOP_INT);

		// Disable TX1 and then wait STOP interrupt
		dai_enable_tx(DAI_TXCH_TX2, FALSE);

#ifdef __KERNEL__
		timeout_cnt = 0;
		while(dai_is_tx_enable(DAI_TXCH_TX2)){
			timeout_cnt++;
			vos_util_delay_ms(10);
			if(timeout_cnt > TIMEOUT_CNT){
				DBG_ERR("dai wait Tx2 disable timeout!!\r\n");
				break;
			}
		};
#else
		dai_wait_interrupt(DAI_TX2STOP_INT);
#endif
	}

	// Time code value will be cleared after DMA is disabled
	// Backup it to variable for later reference of aud_getTimecodeValue()
	uiLastTimeCode2 = dai_get_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_TIMECODE_VAL);//dai_getTimecodeValue();

	// Disable DMA
	dai_enable_tx_dma(DAI_TXCH_TX2, FALSE);
	timeout_cnt = 0;
	while (dai_is_tx_dma_enable(DAI_TXCH_TX2)){
		timeout_cnt++;
		vos_util_delay_ms(10);
		if(timeout_cnt > TIMEOUT_CNT){
			DBG_ERR("dai wait Tx2 dma disable timeout!!\r\n");
			break;
		}
	};

	// Disable TX1 ALL Interrupt
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN,  AUDIO_TX2_FLAGALL);
	dai_set_config(DAI_CONFIG_ID_CLR_INTSTS, AUDIO_TX2_FLAGALL);

	AudioState &= ~AUDIO_STATE_TX2;

	if ((AudioState & (AUDIO_STATE_TX1 | AUDIO_STATE_TX2)) == 0x0) {
		// Stop Codec
		if (AudioCodecFunc[AudioSelectedCodec].stopPlay != NULL) {
			AudioCodecFunc[AudioSelectedCodec].stopPlay();
		}
	}

    dai_unlock();

	return E_OK;
}

/*
    Pause TX2 playback
*/
ER aud_pause_tx2(void)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	// If already disable. No need to pause. Just return.
	if (dai_is_tx_enable(DAI_TXCH_TX2) == FALSE) {
		return E_OK;
	}

	// Output Debug Msg
	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}


	dai_clr_flg(DAI_TX2STOP_INT);

	// Enable STOP interrupt
	dai_set_config(DAI_CONFIG_ID_SET_INTEN,  DAI_TX2STOP_INT);

	// Diable Audio
	dai_enable_tx(DAI_TXCH_TX2, FALSE);

	// Wait for Audio really stopped
	dai_wait_interrupt(DAI_TX2STOP_INT);

	// Make Sure really STOP
	while (dai_is_tx_enable(DAI_TXCH_TX2));

	// Disable STOP interrupt
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN,  DAI_TX2STOP_INT);

#if _EMULATION_
	DBG_WRN("TX2 pause addr = 0x%08X\r\n", (unsigned int)dai_get_tx_dma_curaddr(1));
#endif

	return E_OK;
}

/*
    Resume TX2 playback
*/
ER aud_resume_tx2(void)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (dai_is_tx_enable(DAI_TXCH_TX2) == TRUE) {
		return E_OK;
	}



	// Enable TX1
	dai_enable_tx(DAI_TXCH_TX2, TRUE);
	return E_OK;
}

#endif

/*
    Configure TX2 playback channel number
*/
void aud_set_channel_tx2(AUDIO_CH Channel)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)Channel);
	}


	if (Channel != AUDIO_CH_DUAL_MONO) {
		AudioTx2CH = Channel;
	} else {
		DBG_ERR("Playback has no Dual Mono mode.\r\n");
	}


	// Configure DAI&Codec Channels Settings
	aud_set_chcfg_tx2();
}

/*
	Set External Audio Codec TDM total channel number
*/
void aud_set_tdm_channel_tx2(AUDIO_TDMCH TDM)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if ((!dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN)) && (TDM != AUDIO_TDMCH_2CH)) {
		/* Internal audio codec */
		DBG_WRN("Embedded codec only support 2CH\r\n");
		TDM = AUDIO_TDMCH_2CH;
	}

	dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_TOTAL_CH, TDM);
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
ER aud_set_samplerate_tx2(AUDIO_SR SamplingRate)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)SamplingRate);
	}

	AudSrcSRTx2 = SamplingRate;

	for (i = 0; i < dim(AudioCodecFunc); i++) {
		aud_set_codec_samplerate(&AudioCodecFunc[i], SamplingRate);
	}

	return E_OK;
}


/*
    Configure TX2 playback ReSample Information
*/
BOOL aud_set_resampleinfo_tx2(PAUDIO_RESAMPLE_INFO pResampleInfo)
{
	DBG_ERR("Resample is removed from audio driver\r\n");
	return TRUE;
}

/*
    Configure TX2 playback Feature
*/
void  aud_set_feature_tx2(AUDTS_FEATURE Feature, BOOL bEnable)
{
	if (!audTx2Opened) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: Feature=%d  EN=%d\r\n", __func__, (int)Feature, (int)bEnable);
	}

	switch (Feature) {

	case AUDTS_FEATURE_PLAYBACK_PCM_EXPAND: {
			bAudioExpandTx2 = bEnable;

			// Configure DAI&Codec Channels Settings
			aud_set_chcfg_tx2();
		}
		break;


	default:
		DBG_WRN("function no support! (%d)\r\n", (int)Feature);
		break;
	}



}

/*
    Set TX2 config settings
*/
void aud_set_config_tx2(AUDTS_CFG_ID CfgSel, UINT32 uiSetting)
{
	if (!audTx2Opened) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: Param=%d  Setting=%d\r\n", __func__, (int)CfgSel, (int)uiSetting);
	}

	switch (CfgSel) {
	case AUDTS_CFG_ID_EVENT_HANDLE: {
			pAudEventHandleTx2 = (AUDIO_CB)uiSetting;
		}
		break;

	case AUDTS_CFG_ID_PCM_BITLEN: {
			dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_PCMLEN, uiSetting);
		}
		break;

	default:
		DBG_WRN("function no support! (%d)\r\n", (int)CfgSel);
		break;
	}

}


/*
    Get TX2 config settings
*/
UINT32 aud_get_config_tx2(AUDTS_CFG_ID CfgSel)
{
	UINT32 Ret = 0;

	switch (CfgSel) {
	case AUDTS_CFG_ID_EVENT_HANDLE: {
			Ret = (UINT32)pAudEventHandleTx2;
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
    Reset TX2 Audio Buffer Queue
*/
void aud_reset_buf_queue_tx2(AUDIO_QUEUE_SEL QueueSel)
{
	aud_reset_queue(TSTX2);
}

/*
    Add TX2 Audio Buffer Queue
*/
BOOL aud_add_buf_to_queue_tx2(AUDIO_QUEUE_SEL QueueSel, PAUDIO_BUF_QUEUE pAudioBufQueue)
{
	BOOL Ret;
	unsigned long flag;

	Ret = aud_add_to_queue(TSTX2, pAudioBufQueue);

	if (Ret == TRUE) {

		pAudioRealBufferFront[TSTX2]->uiAddress    = pAudioBufQueue->uiAddress;
		pAudioRealBufferFront[TSTX2]->uiSize       = pAudioBufQueue->uiSize;

		if (pAudioBufQueue->uiSize & 0x03) {
			DBG_ERR("buf size not word align: 0x%x\r\n", (unsigned int)pAudioBufQueue->uiSize);
		}

		loc_cpu_tx2(flag);
		pAudioRealBufferFront[TSTX2]                   = pAudioRealBufferFront[TSTX2]->pNext;
		tmp_count_tx2 = uibuffer_insert_number_tx2;
		unl_cpu_tx2(flag);

        if(tmp_count_tx2 == 1) {
            dai_set_tx_dma_para(1, pAudioBufQueue->uiAddress, pAudioBufQueue->uiSize >> 2);
			loc_cpu_tx2(flag);
            uibuffer_insert_number_tx2++;
			unl_cpu_tx2(flag);
        }

	}

	return Ret;
}

/*
    Check if TX2 Audio Buffer Queue is FULL
*/
BOOL aud_is_buf_queue_full_tx2(AUDIO_QUEUE_SEL QueueSel)
{
	return aud_is_queue_full(TSTX2);
}

/*
    Get TX2 Audio Done Buffer Queue
*/
PAUDIO_BUF_QUEUE aud_get_done_buf_from_queue_tx2(AUDIO_QUEUE_SEL QueueSel)
{
	return aud_get_done_queue(TSTX2);
}

/*
    Get TX2 Current Audio Buffer Working Queue
*/
PAUDIO_BUF_QUEUE aud_get_cur_buf_from_queue_tx2(AUDIO_QUEUE_SEL QueueSel)
{
	return aud_get_cur_queue(TSTX2);
}





#endif

/*
    Dump TX2 playback information
*/
void aud_print_tstx2(void)
{
    DBG_DUMP("---------------Tx2 = %s----------------\r\n", dai_is_tx_enable(DAI_TXCH_TX2)?"ENABLE":"DISABLE");
	DBG_DUMP("@ CHANNEL=%d | EXPAND=%d | Opened=%d\r\n", (int)AudioTx2CH,(int)bAudioExpandTx2,(int)audTx2Opened);
	DBG_DUMP("@ uibuffer_insert_number_tx2 = %d\r\n", (int)uibuffer_insert_number_tx2);
}

//@}
