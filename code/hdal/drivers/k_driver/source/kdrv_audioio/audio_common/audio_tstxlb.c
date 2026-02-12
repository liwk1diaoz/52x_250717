/*
    Audio Transcrive Channel Playback Loopback Channel driver

    This file is the driver of audio transmit(playback) Loopback channel.

    @file       audio_tstxlb.c
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

VK_DEFINE_SPINLOCK(my_lock_txlb);
#define loc_cpu_txlb(myflags)   vk_spin_lock_irqsave(&my_lock_txlb, myflags)
#define unl_cpu_txlb(myflags)   vk_spin_unlock_irqrestore(&my_lock_txlb, myflags)

#define TIMEOUT_CNT 100


static BOOL     audTxlbOpened           = FALSE;
static BOOL     bAudioExpandTxlb        = FALSE;
AUDIO_CH        AudioTxlbCH             = AUDIO_CH_STEREO;

UINT32          uibuffer_insert_number_txlb  = 0;
UINT32			tmp_count_txlb =0;


#if defined (__KERNEL__)
static BOOL audtstxlb_first_boot = TRUE;
#endif

#if 1

//
//  Internal APIs
//

/*

*/
static void aud_set_chcfg_txlb(void)
{

	switch (AudioTxlbCH) {
	case AUDIO_CH_LEFT: {
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,               DAI_CH_MONO_LEFT);

			if (bAudioExpandTxlb == TRUE) {
				dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_STEREO);
			} else {
				dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
			}
		}
		break;

	case AUDIO_CH_RIGHT: {
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,               DAI_CH_MONO_RIGHT);

			if (bAudioExpandTxlb == TRUE) {
				dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_STEREO);
			} else {
				dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
			}
		}
		break;

	case AUDIO_CH_DUAL_MONO: {
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,               DAI_CH_DUAL_MONO);

			if (bAudioExpandTxlb == TRUE) {
				dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_STEREO);
			} else {
				dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,            DAI_DRAMPCM_MONO);
			}
		}
		break;

	case AUDIO_CH_STEREO:
	default: {
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,                DAI_DRAMPCM_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,               DAI_CH_STEREO);
		}
		break;

	}

}

#endif
#if 1
/*

*/
ER aud_open_txlb(void)
{
	if (audTxlbOpened == TRUE) {
		DBG_WRN("audTs Obj TXLB already opened!\r\n");
		return E_OK;
	}

	// Add Audio Open here
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && audtstxlb_first_boot) {
		UINT32 builtin_init = 0;
		UINT32 builtin_txlb_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);
		builtin_init = param[0];

		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_FIRST_QUEUE_TXLB, (UINT32*)&param[0]);
		builtin_txlb_init = param[0];

		if (builtin_init && (builtin_txlb_init != 0)) {
			audTxlbOpened = TRUE;
			return E_OK;
		} else {
			audtstxlb_first_boot = FALSE;
		}
	}
#endif

	audTxlbOpened = TRUE;
	return E_OK;
}

/*

*/
ER aud_close_txlb(void)
{
	unsigned long flag;

	if (!audTxlbOpened) {
		return E_OK;
	}

	audTxlbOpened = FALSE;
	loc_cpu_txlb(flag);
    uibuffer_insert_number_txlb = 0;
	unl_cpu_txlb(flag);
#if defined (__KERNEL__)
	audtstxlb_first_boot = FALSE;
#endif
	return E_OK;
}

/*

*/
BOOL aud_is_opened_txlb(void)
{
	return audTxlbOpened;
}

/*
    Check if RX record is Busy
*/
BOOL aud_is_busy_txlb(void)
{
	return (dai_is_txlb_enable() || dai_is_txlb_dma_enable());
}
#endif
#if 1

/*

*/
ER aud_playback_txlb(void)
{
	DBG_ERR("No Spport Playback!\r\n");
	return E_NOSPT;
}

/*

*/
ER aud_record_preset_txlb(void)
{
	if ((aud_is_opened() == FALSE) || (audTxlbOpened == FALSE)) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	if (dai_is_txlb_enable()) {
		DBG_ERR("Txlb is busy\r\n");
		return E_OACV;
	}

	return E_OK;
}

/*

*/
ER aud_record_txlb(void)
{
	unsigned long flag;

	if ((aud_is_opened() == FALSE) || (audTxlbOpened == FALSE)) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && audtstxlb_first_boot) {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (builtin_init) {
			pAudioRealBufferRear[TSTXLB]    = pAudioPseudoBufferRear[TSTXLB];
			pAudioRealBufferDone[TSTXLB]    = pAudioPseudoBufferDone[TSTXLB];
			pAudioRealBufferFront[TSTXLB]   = pAudioPseudoBufferFront[TSTXLB];

			// Set DMA starting address and size
			//dai_set_txlb_dma_para(pAudioRealBufferRear[TSTXLB]->uiAddress, pAudioRealBufferRear[TSTXLB]->uiSize >> 2);
			loc_cpu_txlb(flag);
			uibuffer_insert_number_txlb++;
			unl_cpu_txlb(flag);

			// Set next DMA starting address and size
			if (pAudioRealBufferRear[TSTXLB]->pNext != pAudioRealBufferFront[TSTXLB]) {
				//dai_set_txlb_dma_para(pAudioRealBufferRear[TSTXLB]->pNext->uiAddress, pAudioRealBufferRear[TSTXLB]->pNext->uiSize >> 2);
				loc_cpu_txlb(flag);
				uibuffer_insert_number_txlb++;
				unl_cpu_txlb(flag);
			}

			// Set Audio operation mode to "Transmit Loopback" mode
			AudioState |= AUDIO_STATE_TXLB;

			return E_OK;
		}
	}
#endif

	if (dai_is_txlb_dma_enable()) {
		DBG_ERR("is busy\r\n");
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	pAudioRealBufferRear[TSTXLB]    = pAudioPseudoBufferRear[TSTXLB];
	pAudioRealBufferDone[TSTXLB]    = pAudioPseudoBufferDone[TSTXLB];
	pAudioRealBufferFront[TSTXLB]   = pAudioPseudoBufferFront[TSTXLB];

	if ((dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN)) && (dai_get_i2s_config(DAI_I2SCONFIG_ID_OPMODE) == DAI_OP_SLAVE) && (!dai_get_i2s_config(DAI_I2SCONFIG_ID_SLAVEMATCH))) {
		DBG_ERR("I2S CKRatio Not Match\r\n");
	}

	// Set DMA starting address and size
	dai_set_txlb_dma_para(pAudioRealBufferRear[TSTXLB]->uiAddress, pAudioRealBufferRear[TSTXLB]->uiSize >> 2);
	loc_cpu_txlb(flag);
    uibuffer_insert_number_txlb++;
	unl_cpu_txlb(flag);

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


	// Set next DMA starting address and size
	if (pAudioRealBufferRear[TSTXLB]->pNext != pAudioRealBufferFront[TSTXLB]) {
		dai_set_txlb_dma_para(pAudioRealBufferRear[TSTXLB]->pNext->uiAddress, pAudioRealBufferRear[TSTXLB]->pNext->uiSize >> 2);
		loc_cpu_txlb(flag);
        uibuffer_insert_number_txlb++;
		unl_cpu_txlb(flag);
	}

	// Enable Interrupt
	dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TXLBBWERR_INT | DAI_TXLBDMADONE_INT);

	// Enable playback loopback
	dai_enable_txlb(TRUE);

	// Set Audio operation mode to "Transmit Loopback" mode
	AudioState |= AUDIO_STATE_TXLB;

	return E_OK;
}


/*

*/
ER aud_stop_txlb(void)
{
	int timeout_cnt;

	
	// Prevent calling aud_stop() multiple times
	if ((aud_is_opened() == FALSE) || (dai_is_txlb_dma_enable() == FALSE)) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}

	dai_clr_flg(DAI_TXLBSTOP_INT);

	// Disable Audio
	if (dai_is_txlb_enable() == TRUE) {
		UINT32 uiStart = 0;

		// Prevent Stop after Start immediately
		dai_get_txlb_dma_para(&uiStart, NULL);
		while (uiStart == dai_get_txlb_dma_curaddr());

		// Enable RX Stop interrupt enable
		dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TXLBSTOP_INT);

		// Disable TXLB and then wait STOP interrupt
		dai_enable_txlb(FALSE);
#ifdef __KERNEL__
		timeout_cnt = 0;
		while(dai_is_txlb_enable()){
			timeout_cnt++;
			vos_util_delay_ms(10);
			if(timeout_cnt > TIMEOUT_CNT){
				DBG_ERR("dai wait TxLB disable timeout!!\r\n");
				break;
			}

		};		
#else
		dai_wait_interrupt(DAI_TXLBSTOP_INT);
#endif
	}

	// Disable DMA
	dai_enable_txlb_dma(FALSE);
	timeout_cnt = 0;
	while (dai_is_txlb_dma_enable()){
		timeout_cnt++;
		vos_util_delay_ms(10);
		if(timeout_cnt > TIMEOUT_CNT){
			DBG_ERR("dai wait Rx dma disable timeout!!\r\n");
			break;
		}
	};	

	// Save valid PCM data size of last buffer in record mode
	pAudioPseudoBufferRear[TSTXLB]->uiValidSize =
		pAudioRealBufferRear[TSTXLB]->uiValidSize   = dai_get_txlb_dma_curaddr() - pAudioRealBufferRear[TSTXLB]->uiAddress;

	// Disable TXLB ALL Interrupt
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN,  DAI_INTERRUPT_TXLB_ALL);
	dai_set_config(DAI_CONFIG_ID_CLR_INTSTS, DAI_INTERRUPT_TXLB_ALL);

	AudioState &= ~AUDIO_STATE_TXLB;
	return E_OK;
}

/*

*/
ER aud_pause_txlb(void)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	// If already disable. No need to pause. Just return.
	if (dai_is_txlb_enable() == FALSE) {
		return E_OK;
	}

	// Output Debug Msg
	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s\r\n", __func__);
	}


	dai_clr_flg(DAI_TXLBSTOP_INT);

	// Enable STOP interrupt
	dai_set_config(DAI_CONFIG_ID_SET_INTEN,  DAI_TXLBSTOP_INT);

	// Diable Audio
	dai_enable_txlb(FALSE);

	// Wait for Audio really stopped
	dai_wait_interrupt(DAI_TXLBSTOP_INT);

	// Make Sure really STOP
	while (dai_is_txlb_enable());

	// Disable STOP interrupt
	dai_set_config(DAI_CONFIG_ID_CLR_INTEN,  DAI_TXLBSTOP_INT);

#if _EMULATION_
	DBG_WRN("TXLB pause addr = 0x%08X\r\n", (unsigned int)dai_get_txlb_dma_curaddr());
#endif

	return E_OK;
}

ER aud_resume_txlb(void)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return E_OACV;
	}

	if (dai_is_txlb_enable() == TRUE) {
		return E_OK;
	}

	// Enable TXLB
	dai_enable_txlb(TRUE);
	return E_OK;
}

#endif

/*

*/
void aud_set_channel_txlb(AUDIO_CH Channel)
{
	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)Channel);
	}

	AudioTxlbCH = Channel;

	// Configure DAI&Codec Channels Settings
	aud_set_chcfg_txlb();
}

/*
    Set sampling rate
*/
ER aud_set_samplerate_txlb(AUDIO_SR SamplingRate)
{
	return E_OK;
}

/*
	Set External Audio Codec TDM total channel number
*/
void aud_set_tdm_channel_txlb(AUDIO_TDMCH TDM)
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

	dai_set_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH, TDM);
}

/*

*/
BOOL aud_set_resampleinfo_txlb(PAUDIO_RESAMPLE_INFO pResampleInfo)
{
	DBG_ERR("Reord No Spport Resample!\r\n");
	return FALSE;
}


/*

*/
void  aud_set_feature_txlb(AUDTS_FEATURE Feature, BOOL bEnable)
{
	if (!audTxlbOpened) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: Feature=%d  EN=%d\r\n", __func__, (int)Feature, (int)bEnable);
	}

	switch (Feature) {
	case AUDTS_FEATURE_RECORD_PCM_EXPAND: {
			bAudioExpandTxlb = bEnable;

			// Configure DAI&Codec Channels Settings
			aud_set_chcfg_txlb();
		}
		break;


	default:
		DBG_WRN("function no support! (%d)\r\n", (int)Feature);
		break;
	}

}

/*

*/
void aud_set_config_txlb(AUDTS_CFG_ID CfgSel, UINT32 uiSetting)
{
	if (!audTxlbOpened) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: Param=%d  Setting=%d\r\n", __func__, (int)CfgSel, (int)uiSetting);
	}

	switch (CfgSel) {
	case AUDTS_CFG_ID_EVENT_HANDLE: {
			pAudEventHandleTxlb = (AUDIO_CB)uiSetting;
		}
		break;


	default:
		DBG_WRN("function no support! (%d)\r\n", (int)CfgSel);
		break;
	}

}


/*

*/
UINT32 aud_get_config_txlb(AUDTS_CFG_ID CfgSel)
{
	UINT32 Ret = 0;

	switch (CfgSel) {
	case AUDTS_CFG_ID_EVENT_HANDLE: {
			Ret = (UINT32)pAudEventHandleTxlb;
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

void aud_reset_buf_queue_txlb(AUDIO_QUEUE_SEL QueueSel)
{
	aud_reset_queue(TSTXLB);
}

BOOL aud_add_buf_to_queue_txlb(AUDIO_QUEUE_SEL QueueSel, PAUDIO_BUF_QUEUE pAudioBufQueue)
{
	BOOL Ret;
	unsigned long flag;

	Ret = aud_add_to_queue(TSTXLB, pAudioBufQueue);

	if (Ret == TRUE) {
		pAudioRealBufferFront[TSTXLB]->uiAddress    = pAudioBufQueue->uiAddress;
		pAudioRealBufferFront[TSTXLB]->uiSize       = pAudioBufQueue->uiSize;

		if (pAudioBufQueue->uiSize & 0x03) {
			DBG_ERR("buf size not word align: 0x%x\r\n", (unsigned int)pAudioBufQueue->uiSize);
		}

		loc_cpu_txlb(flag);
		pAudioRealBufferFront[TSTXLB]                   = pAudioRealBufferFront[TSTXLB]->pNext;
		tmp_count_txlb = uibuffer_insert_number_txlb;
		unl_cpu_txlb(flag);

        if(tmp_count_txlb == 1) {
            dai_set_txlb_dma_para(pAudioBufQueue->uiAddress, pAudioBufQueue->uiSize >> 2);
			loc_cpu_txlb(flag);
            uibuffer_insert_number_txlb++;
			unl_cpu_txlb(flag);
        }
	}

	return Ret;
}

BOOL aud_is_buf_queue_full_txlb(AUDIO_QUEUE_SEL QueueSel)
{
	return aud_is_queue_full(TSTXLB);
}

PAUDIO_BUF_QUEUE aud_get_done_buf_from_queue_txlb(AUDIO_QUEUE_SEL QueueSel)
{
	return aud_get_done_queue(TSTXLB);
}

PAUDIO_BUF_QUEUE aud_get_cur_buf_from_queue_txlb(AUDIO_QUEUE_SEL QueueSel)
{
	return aud_get_cur_queue(TSTXLB);
}

#endif

//@}
