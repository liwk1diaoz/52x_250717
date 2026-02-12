/*
    Build in audio codec driver

    This file is the driver for embedded audio codec.
    Default disable ALC, enable MIC BOOST, enable HP power always ON.

    @file       AudCodecEmbd.C
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

*/
#ifdef __KERNEL__
#include "kwrap/type.h"//a header for basic variable type
#include <mach/rcw_macro.h>
#include "dai.h"
#include "Audio.h"
#include "AudioCodec.h"
#include "plat/pad.h"
#elif defined(__FREERTOS)
#include "kwrap/type.h"
#include "rcw_macro.h"
#include "dai.h"
#include "Audio.h"
#include "AudioCodec.h"
#include "plat/pad.h"

#endif
#include "kwrap/spinlock.h"
static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)
#include "AudCodecEmbd_int.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"
#define pinmux_getPinmux(x) x
#define PIN_AUDIO_CFG_DMIC 0x100
#define PIN_FUNC_AUDIO 1


static BOOL                 b_aud_codec_opened	= FALSE;
static BOOL                 b_boost_en			= FALSE;
static AUDCODEC_PARAM       audio_codec_param = {
	AUDCODEC_STATE_STOP,                                    // AudState
	AUDCODEC_STATE_STOP,                                    // PowerState
	AUDIO_OUTPUT_HP,                                        // OutPath
	AUDIO_CH_STEREO,                                        // OutChannel
	AUDIO_CH_STEREO,                                        // InChannel
	AUDIO_GAIN_2,                                           // RecordGain
	AUDIO_VOL_2,                                            // PlayVolume
	EAC_PGABOOST_SEL_20DB,                                  // BoostValue
	{EAC_NG_THRESHOLD_N69P0_DB, EAC_NG_THRESHOLD_N69P0_DB}, // NoiseGateTH
	0xC3,                                                   // uiDigiGain2
	AUDIO_SR_8000,                                          // AudSampleRate
	AUDIO_SR_32000,                                         // PlaySampleRate
	FALSE,                                                  // bChkPlySampleRate
	FALSE,                                                  // bSpkrPwrAlwaysOn
	FALSE,                                                  // bHPPwrAlwaysOn
	FALSE,                                                  // bRecordPreset
	450,                                                    // uiRecordDelay
	FALSE,                                                  // bHPDepopEn
	FALSE,                                                  // bHPDepopDone
	AUDIO_DA_SW_DISCHARGE,                                  // uiNeedDisCharge
	0xC3,                                                   // uiPlayDigiGain
	FALSE                                                   // advcm_always_on
};

static AUDIO_CODEC_FUNC     embed_codec_fp        = {
	AUDIO_CODEC_TYPE_EMBEDDED,                              // codecType

	NULL,                                                   // Set Device Object
	audcodec_init,                                          // Init
	audcodec_set_record_source,                             // Set Record Source
	audcodec_set_output,                                    // Set Output
	audcodec_set_samplerate,                                // Set Sampling rate
	audcodec_set_tx_channel,                                // Set Playback Channel
	audcodec_set_rx_channel,                                // Set Record Channel
	audcodec_set_volume,                                    // Set Volume (Playback)
	audcodec_set_gain,                                      // Set gain (Record)
	audcodec_set_gaindb,                                    // Set GainDB (Record)
	NULL,                                                   // Set Effect
	audcodec_set_feature,                                   // Set feature
	audcodec_stop_record,                                   // Stop Record
	audcodec_stop_play,                                     // Stop Playback
	audcodec_preset_record,                                 // Record Preset
	audcodec_record,                                        // Record
	audcodec_playback,                                      // Playback
	NULL,                                                   // Set I2S Format
	NULL,                                                   // Set I2S clock ratio
	NULL,                                                   // Send command
	NULL,                                                   // Read data
	audcodec_set_parameter,                                 // Set Parameter
	audcodec_chk_samplerate,                                // Check Sampling rate
	audcodec_open,                                          // Open
	audcodec_close,                                         // Close
};


#if _FPGA_EMULATION_
extern void tc680_init(void);
extern UINT32 tc680_writeReg(UINT32 uiOffset, UINT32 uiValue);
extern UINT32 tc680_readReg(UINT32 uiOffset, UINT32 *puiValue);

#endif

#if defined (__KERNEL__)
static BOOL aud_codec_first_boot = TRUE;
#endif

/*
    Get audio codec function pointer of the embedded audio codec

    This function will return audio code function object of the embedded
    audio codec.

    @param[out]  p_aud_codec_func      Audio codec function pointer

    @return void
*/
void audcodec_get_fp(PAUDIO_CODEC_FUNC p_aud_codec_func)
{
	memcpy((void *)p_aud_codec_func, (const void *)&embed_codec_fp, sizeof(AUDIO_CODEC_FUNC));
}

#if 1
/*
    Enable/Disable the embedded audio codec source clocks
*/

static void audcodec_enable_clk(BOOL en)
{
	eac_enableclk(en);
}
static void audcodec_enable_dacclk(BOOL en)
{
	eac_enabledacclk(en);
}

static void audcodec_enable_adcclk(BOOL en)
{
	eac_enableadcclk(en);
}

/*
    Enable/Disable AD Analog Power
*/
static void audcodec_enable_adcpwr(BOOL en)
{
	unsigned long flag;
#if _FPGA_EMULATION_
	UINT32 reg08, reg09;

	tc680_readReg(0x0008, &reg08);
	tc680_readReg(0x0009, &reg09);

	if (en) {
		loc_cpu(flag);
		audio_codec_param.power_state |= AUDCODEC_STATE_RECORD;

		unl_cpu(flag);

		eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_EN,       ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_LVL,       TRUE);

		// Enable AD power
		switch (audio_codec_param.input_channel) {
		case AUDIO_CH_LEFT: {
				eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_L,   TRUE);

				reg08 &= ~0xEE;
			}
			break;

		case AUDIO_CH_RIGHT: {
				eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_R,   TRUE);

				reg08 &= ~0xDD;
			}
			break;

		//case AUDIO_CH_STEREO:
		//case AUDIO_CH_MONO: // Mono Expand
		default : {
				eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_L,   TRUE);
				eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_R,   TRUE);

				reg08 &= ~0xFF;
			}
			break;

		}

		eac_set_ad_config(EAC_CONFIG_AD_PDREF_BUF,        FALSE);
        eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       FALSE);
	} else {
		eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_L,   FALSE);
		eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_R,   FALSE);
		eac_set_ad_config(EAC_CONFIG_AD_PDREF_BUF,        TRUE);
        eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       TRUE);

		reg08 |= 0x7F;

		eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_EN,       DISABLE);
		eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_LVL,      FALSE);

		loc_cpu(flag);
		audio_codec_param.power_state &= ~AUDCODEC_STATE_RECORD;
		unl_cpu(flag);

	}

	tc680_writeReg(0x0009, reg09 | 0x800); //MIC Bias
	tc680_writeReg(0x0008, reg08);

#else

	if (en) {
		loc_cpu(flag);
		audio_codec_param.power_state |= AUDCODEC_STATE_RECORD;

		unl_cpu(flag);

		eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_EN,       ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_LVL,       TRUE);

		// Enable AD power
		switch (audio_codec_param.input_channel) {
		case AUDIO_CH_LEFT: {
				eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_L,   TRUE);
#if _EMULATION_
                UINT32 ret_L,ret_R;
                ret_R = eac_get_ad_config(EAC_CONFIG_AD_POWER_EN_R);
                ret_L = eac_get_ad_config(EAC_CONFIG_AD_POWER_EN_L);
                DBG_DUMP("AD LEFT ONLY  Right = [%d] left = [%d]\r\n",(int)ret_R,(int)ret_L);
#endif
			}
			break;

		case AUDIO_CH_RIGHT: {
				eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_R,   TRUE);
#if _EMULATION_
                UINT32 ret_L,ret_R;
                ret_R = eac_get_ad_config(EAC_CONFIG_AD_POWER_EN_R);
                ret_L = eac_get_ad_config(EAC_CONFIG_AD_POWER_EN_L);
                DBG_DUMP("AD RIGHT ONLY  Right = [%d] left = [%d]\r\n",(int)ret_R,(int)ret_L);
#endif
			}
			break;

		//case AUDIO_CH_STEREO:
		//case AUDIO_CH_MONO: // Mono Expand
		default : {
				eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_L,   TRUE);
				eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_R,   TRUE);
#if _EMULATION_
                UINT32 ret_L,ret_R;
                ret_R = eac_get_ad_config(EAC_CONFIG_AD_POWER_EN_R);
                ret_L = eac_get_ad_config(EAC_CONFIG_AD_POWER_EN_L);
                DBG_DUMP("AD STEREO  Right = [%d] left = [%d]\r\n",(int)ret_R,(int)ret_L);
#endif
			}
			break;

		}

        eac_set_ad_config(EAC_CONFIG_AD_PDREF_BUF,        FALSE);
        eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       FALSE);
        eac_set_ad_config(EAC_CONFIG_AD_PD_VCMBIAS,       FALSE);

		eac_set_phypower(ENABLE);
	} else {
		eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_L,   FALSE);
		eac_set_ad_config(EAC_CONFIG_AD_POWER_EN_R,   FALSE);
        eac_set_ad_config(EAC_CONFIG_AD_PDREF_BUF,         TRUE);
		if(!(audio_codec_param.audio_state& AUDCODEC_STATE_PLAY))
        	eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,        TRUE);

		eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_EN,       DISABLE);
	 	eac_set_ad_config(EAC_CONFIG_AD_MICBIAS_LVL,       FALSE);

		loc_cpu(flag);
		audio_codec_param.power_state &= ~AUDCODEC_STATE_RECORD;
		if (audio_codec_param.power_state == AUDCODEC_STATE_STOP) {
			eac_set_ad_config(EAC_CONFIG_AD_PD_VCMBIAS,       TRUE);
			eac_set_phypower(DISABLE);
		}
		unl_cpu(flag);

	}
#endif

}

/*
    Enable/Disable DA Analog Power
*/
static void audcodec_enable_dacpwr(BOOL en)
{
	unsigned long flag;
#if _FPGA_EMULATION_
	UINT32 reg09;

	tc680_readReg(0x0009, &reg09);

	if (en) {
		loc_cpu(flag);
		audio_codec_param.power_state |= AUDCODEC_STATE_PLAY;
		unl_cpu(flag);

		if ((audio_codec_param.b_spk_power_always_on) || (audio_codec_param.output_path == AUDIO_OUTPUT_SPK)) {
			eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, TRUE);
			eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, TRUE);

			reg09 &= ~0x107;
		} else {
			// Enable AD power
			switch (audio_codec_param.output_channel) {
			case AUDIO_CH_LEFT: {
					eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, TRUE);

					reg09 &= ~0x106;
				}
				break;

			case AUDIO_CH_RIGHT: {
					eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, TRUE);

					reg09 &= ~0x105;
				}
				break;

			//case AUDIO_CH_STEREO:
			//case AUDIO_CH_MONO:
			default : {
					eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, TRUE);
					eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, TRUE);

					reg09 &= ~0x107;
				}
				break;

			}
		}
		eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       FALSE);
	} else {
		eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, FALSE);
		eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, FALSE);
		eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       TRUE);

		reg09 |= 0x107;

		loc_cpu(flag);
		audio_codec_param.power_state &= ~AUDCODEC_STATE_PLAY;
		unl_cpu(flag);
	}

	tc680_writeReg(0x0009, reg09);

#else
	if (en) {
		loc_cpu(flag);
		audio_codec_param.power_state |= AUDCODEC_STATE_PLAY;
		unl_cpu(flag);

		if ((audio_codec_param.b_spk_power_always_on) || (audio_codec_param.output_path == AUDIO_OUTPUT_SPK)) {
			eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, TRUE);
			eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, TRUE);
		} else {
			// Enable DA power
			switch (audio_codec_param.output_channel) {
			case AUDIO_CH_LEFT: {
					eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, TRUE);
#if _EMULATION_
					DBG_DUMP("DA LEFT ONLY\r\n");
#endif
				}
				break;
			case AUDIO_CH_RIGHT: {
					eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, TRUE);
#if _EMULATION_
					DBG_DUMP("DA RIGHT ONLY\r\n");
#endif
				}
				break;

			//case AUDIO_CH_STEREO:
			//case AUDIO_CH_MONO:
			default : {
					eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, TRUE);
					eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, TRUE);
#if _EMULATION_
					DBG_DUMP("DA STEREO\r\n");
#endif
				}
				break;

			}
		}

        if(eac_get_da_config(EAC_CONFIG_DA_DEPOP_EN))
        {   // need to set these Regs by this order for the depop flow
            audcodec_debug(("Depop_on\r\n"));
            eac_set_phypower(ENABLE);
            eac_set_phydacpower(ENABLE);

            if(!audio_codec_param.advcm_always_on){
                eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       FALSE);
                eac_set_ad_config(EAC_CONFIG_AD_PD_VCMBIAS,       FALSE);
            }
        }
        else
        {
            audcodec_debug(("Depop_off\r\n"));
            if(!audio_codec_param.advcm_always_on){
                eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       FALSE);
                eac_set_ad_config(EAC_CONFIG_AD_PD_VCMBIAS,       FALSE);
            }
            eac_set_phypower(ENABLE);
            eac_set_phydacpower(ENABLE);


        }

		eac_set_da_config(EAC_CONFIG_DA_RESET, FALSE);

	} else {
		eac_set_da_config(EAC_CONFIG_DA_POWER_EN_L, FALSE);
		eac_set_da_config(EAC_CONFIG_DA_POWER_EN_R, FALSE);
        if((!audio_codec_param.advcm_always_on)&&(!(audio_codec_param.audio_state & AUDCODEC_STATE_RECORD))){
                eac_set_ad_config(EAC_CONFIG_AD_PDREF_BIAS,       TRUE);
        }
		eac_set_da_config(EAC_CONFIG_DA_RESET, TRUE);
		eac_set_phydacpower(DISABLE);

		loc_cpu(flag);
		audio_codec_param.power_state &= ~AUDCODEC_STATE_PLAY;
		if (audio_codec_param.power_state == AUDCODEC_STATE_STOP) {
            if(!audio_codec_param.advcm_always_on){
                eac_set_ad_config(EAC_CONFIG_AD_PD_VCMBIAS,       TRUE);
            }
			eac_set_phypower(DISABLE);
		}
		unl_cpu(flag);

	}
#endif
}

/*
    Set pre-assigned output path power

    Set pre-assigned output path power

    @return void
*/
static void audcodec_preassign_pwr(void)
{
#if _FPGA_EMULATION_
	UINT32 reg09;

	tc680_readReg(0x0009, &reg09);

	if ((audio_codec_param.b_spk_power_always_on) && (audio_codec_param.output_path == AUDIO_OUTPUT_SPK)) {
		// enable speaker path
		eac_set_dac_output(EAC_OUTPUT_SPK,        TRUE);

		reg09 &= ~0x20;
		tc680_writeReg(0x0009, reg09);

		// enable DAC power
		audcodec_enable_dacpwr(TRUE);

	} else if ((audio_codec_param.b_headphone_power_always_on) && ((audio_codec_param.output_path == AUDIO_OUTPUT_LINE) || (audio_codec_param.output_path == AUDIO_OUTPUT_HP))) {
		// Depop time: (0x1+0x1FFFF)*208.3ns*100 = 2.7s
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_PERIOD_H,   0x1);
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_PERIOD_L,   0x1FFFF);
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_PERIOD_CYC, 100);

		// Depop Enable
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         DISABLE);
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         ENABLE);

		reg09 |= 0x600;
		tc680_writeReg(0x0009, reg09);

		// enable lineout/HP path
		eac_set_dac_output(EAC_OUTPUT_LINE,               TRUE);

		// enable DAC power
		audcodec_enable_dacpwr(TRUE);

		tc680_readReg(0x0009, &reg09);
		reg09 &= ~0x418;
		tc680_writeReg(0x0009, reg09);

	} else if ((audio_codec_param.audio_state & AUDCODEC_STATE_PLAY) == AUDCODEC_STATE_STOP) {
		reg09 |= 0x38;
		tc680_writeReg(0x0009, reg09);

		// disable DAC power
		audcodec_enable_dacpwr(FALSE);

		eac_set_dac_output(EAC_OUTPUT_LINE,       FALSE);
		eac_set_dac_output(EAC_OUTPUT_SPK,        FALSE);
	}

#else

	if ((audio_codec_param.b_spk_power_always_on) && (audio_codec_param.output_path == AUDIO_OUTPUT_SPK)) {
        // enable speaker path
		eac_set_dac_output(EAC_OUTPUT_SPK,        TRUE);
		// enable DAC power
		audcodec_enable_dacpwr(TRUE);
	} else if ((audio_codec_param.b_headphone_power_always_on) && ((audio_codec_param.output_path == AUDIO_OUTPUT_LINE) || (audio_codec_param.output_path == AUDIO_OUTPUT_HP))) {

		// Depop Enable
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         DISABLE);
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         ENABLE);

		// enable lineout/HP path
		eac_set_dac_output(EAC_OUTPUT_LINE,               TRUE);

		// enable DAC power
		audcodec_enable_dacpwr(TRUE);
	} else if ((audio_codec_param.output_path == AUDIO_OUTPUT_ALL)&&((audio_codec_param.b_headphone_power_always_on) || (audio_codec_param.b_spk_power_always_on))){

		// Depop Enable
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         DISABLE);
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         ENABLE);

		// enable lineout/HP path
		eac_set_dac_output(EAC_OUTPUT_ALL,               TRUE);

		// enable DAC power
		audcodec_enable_dacpwr(TRUE);


	} else if ((audio_codec_param.audio_state & AUDCODEC_STATE_PLAY) == AUDCODEC_STATE_STOP) {
        // disable DAC power
		audcodec_enable_dacpwr(FALSE);

		eac_set_dac_output(EAC_OUTPUT_LINE,       FALSE);
		eac_set_dac_output(EAC_OUTPUT_SPK,        FALSE);
		eac_set_dac_output(EAC_OUTPUT_ALL,        FALSE);
	}
#endif
}
#endif



#if 1
/*
    Open Embedded Audio Codec Basic installation

    Open Embedded Audio Codec Basic installation, such as source clocks.
*/
static void audcodec_open(void)
{
	audcodec_debug(("AudOpen\r\n"));

	b_aud_codec_opened = TRUE;

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && aud_codec_first_boot) {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (builtin_init) {
			return;
		}
	}
#endif

	// Open the Embedded Audio Codec needed clock sources.
	audcodec_enable_dacclk(ENABLE);
	audcodec_enable_adcclk(ENABLE);
	audcodec_enable_clk(ENABLE);
}

/*
    Close Embedded Audio Codec Basic installation

    Close Embedded Audio Codec Basic installation, such as source clocks.
*/
static void audcodec_close(void)
{
	audcodec_debug(("AudClose\r\n"));

	if (audio_codec_param.audio_state != AUDCODEC_STATE_STOP) {
		DBG_ERR("Audio Close but function not OFF: %d\r\n", (int)audio_codec_param.audio_state);
	}

	b_aud_codec_opened = FALSE;

	if (!((audio_codec_param.b_spk_power_always_on && (audio_codec_param.output_path == AUDIO_OUTPUT_SPK)) ||
		  ((audio_codec_param.b_headphone_power_always_on) && ((audio_codec_param.output_path == AUDIO_OUTPUT_LINE) || (audio_codec_param.output_path == AUDIO_OUTPUT_HP))))) {
		audcodec_enable_dacclk(DISABLE);
		audcodec_enable_adcclk(DISABLE);
		audcodec_enable_clk(DISABLE);
	}

#if defined (__KERNEL__)
	aud_codec_first_boot = FALSE;
#endif
}

/*
    Initialize API for audio module

    This function is the initialize API for audio module
    It will set the following parameters if required:
		(1) Sampling Rate
		(2) Channel
		(3) Record Source
		(4) I2S Clock Ratio
		(5) I2S Format
		(6) Master/Slave Mode
		(7) Output Path
		(8) Other Codec related settings (Power on/down, Control interface...)

    @param[in] p_audio_setting   Audio setting

    @return void
*/
static void audcodec_init(PAUDIO_SETTING p_audio_setting)
{
	static BOOL         init;
	PAD_POWER_STRUCT    pad_power;

	if (init) {
		return;
	}

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && aud_codec_first_boot) {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (builtin_init) {
			return;
		}
	}
#endif

#if _FPGA_EMULATION_
	tc680_init();

	// TOP to Audio Codec
	tc680_writeReg(0x0000, 0x2A000006);
	// Turn ON LDO bypass
	tc680_writeReg(0x0002, 0x00000021);//Use 0x21(bypass) or 0x29(LDO)

	// AD Default settings
	tc680_writeReg(0x0008, 0x0053587F);

	// DA Default settngs
	tc680_writeReg(0x0009, 0x0000093F);
#endif


	//Enable AD LDO

	pad_power.pad_power_id     = PAD_POWERID_ADC;
	pad_power.pad_power		   = PAD_1P8V;
	pad_power.bias_current	   = FALSE;
	pad_power.opa_gain		   = FALSE;
	pad_power.pull_down		   = FALSE;
	pad_power.enable		   = ENABLE;
	pad_power.pad_vad		   = PAD_VAD_3P0V;
	pad_set_power(&pad_power);



	// embedded audio codec initiation
	eac_init();

	// Set Init Sampling Rate
	audcodec_set_samplerate(p_audio_setting->SamplingRate);

	// Set Init Channel Number
	audcodec_set_tx_channel(p_audio_setting->Channel);
	audcodec_set_rx_channel(p_audio_setting->Channel);
	// Set Init Output path
	audcodec_set_output(p_audio_setting->Output);

	if (p_audio_setting->RecSrc == AUDIO_RECSRC_DMIC) {

        audcodec_debug(("Using D-MIC\r\n"));
		audcodec_set_record_source(AUDIO_RECSRC_DMIC);
	}

	init = 1;

}

/*
    Set audio module Record Source

    This function is the Set Record Source API for audio module

    @param[in] RecSrc   Record Source. Only AUDIO_RECSRC_MIC or AUDIO_RECSRC_DMIC is valid settings.

    @return void
*/
static void audcodec_set_record_source(AUDIO_RECSRC recsrc)
{
	if (recsrc == AUDIO_RECSRC_DMIC) {
		eac_set_ad_config(EAC_CONFIG_AD_DMIC_EN,          ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_DMIC_CLK_EN,      ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_ALC_MODE_DGAIN,   ENABLE);
		eac_set_ad_config(EAC_CONFIG_AD_DMIC_PATH_MUX,    ENABLE);

		audcodec_set_feature(AUDIO_FEATURE_ALC_IIR_EN,    ENABLE);
		audcodec_set_feature(AUDIO_FEATURE_OUTPUT_IIR_EN, ENABLE);
	} else if (recsrc == AUDIO_RECSRC_MIC){
		eac_set_ad_config(EAC_CONFIG_AD_DMIC_EN,          DISABLE);
		eac_set_ad_config(EAC_CONFIG_AD_DMIC_CLK_EN,      DISABLE);
		eac_set_ad_config(EAC_CONFIG_AD_ALC_MODE_DGAIN,   DISABLE);
		eac_set_ad_config(EAC_CONFIG_AD_DMIC_PATH_MUX,    DISABLE);

		audcodec_set_feature(AUDIO_FEATURE_ALC_IIR_EN,    DISABLE);
		audcodec_set_feature(AUDIO_FEATURE_OUTPUT_IIR_EN, DISABLE);
	}

}

/*
    Set Output Path API for audio module

    This function is the Set Output Path API for audio module

    @param[in] Output   Output Path

    @return void
*/
static void audcodec_set_output(AUDIO_OUTPUT Output)
{
	// In playing, and Make sure he EAC is rellay at playback state.
	if (eac_is_dac_enable() == TRUE) {
		// Disable previous output path
		audcodec_stop_play();

		// Save new output path
		audio_codec_param.output_path = Output;

		audcodec_preassign_pwr();

		// Enable new output path
		audcodec_playback();
	} else {
		audio_codec_param.output_path = Output;

		audcodec_preassign_pwr();
	}
}

/*
    Set Sampling Rate API for audio module

    This function is the Set Sampling Rate API for audio module

    @param[in] SamplingRate     Sampling Rate

    @return void
*/
static void audcodec_set_samplerate(AUDIO_SR SamplingRate)
{
	audio_codec_param.audio_sample_rate = SamplingRate;
}

/*
    Set Channel API for audio playback

    This function is the Set Channel API for audio playback

    @param[in] Channel      Channel Mode

    @return void
*/
static void audcodec_set_tx_channel(AUDIO_CH Channel)
{
	// Save output(DA) channel number
	audio_codec_param.output_channel = Channel;
}

/*
    Set Channel API for audio record

    This function is the Set Channel API for audio record

    @param[in] Channel      Channel Mode

    @return void
*/
static void audcodec_set_rx_channel(AUDIO_CH Channel)
{
	// Save input(AD) channel number
	audio_codec_param.input_channel = Channel;
}

/*
    Set PCM Volume API for audio module

    This function is the Set PCM Volume API for audio module

    @param[in] volume          Volume

    @return void
*/
static void audcodec_set_volume(AUDIO_VOL volume)
{
	UINT32 data;

	if (volume > (AUDIO_VOL)aud_get_total_vol_level()) {
		//coverity[mixed_enums]
		volume = aud_get_total_vol_level();
	}

	audio_codec_param.play_volume = volume;

	// Translate the DA volume Levels to physical PGA Gain.
	if (aud_get_total_vol_level() == AUDIO_VOL_LEVEL8) {
		//  -29 ~ +6 dB
		data = 0x89 + ((volume - AUDIO_VOL_0) * 10);
	} else {
		// -31 ~ +6.5 dB
		data = 0x85 + ((volume - AUDIO_VOL_0) * 6 / 5);
	}

	if (data > 0xFF) {
		data = 0X000000FF;
	}

	if (volume == AUDIO_VOL_MUTE) {
		data = 0x00;
	}

	eac_set_da_config(EAC_CONFIG_DA_DGAIN_L, data);
	eac_set_da_config(EAC_CONFIG_DA_DGAIN_R, data);


	// If the volume is changed during playback, set load.
	// If no, the load would be set during playback start.
	if (eac_is_dac_enable()) {
		eac_set_load();
	}

}

/*
    Set Record gain API for audio module

    This function is the Set Record gain API for audio module.
    If the Auto-Level-Control(ALC) is enabled, this would set to control the ALC gain target.
    If the Auto-Level-Control(ALC) is disabled, this would set to control the PGA gain factor.

    @param[in] gain         gain control factor. Refer to Audio.h for reference.

    @return void
*/
static void audcodec_set_gain(AUDIO_GAIN gain)
{
	UINT32 data;


	if ((int)gain > (int)aud_get_Total_gain_level()) {
		//coverity[mixed_enums]
		gain = (AUDIO_GAIN)aud_get_Total_gain_level();
	}

	audio_codec_param.record_gain = gain;

	if (gain == AUDIO_GAIN_MUTE) {
		// If record gain mute, we must set mute to D-Gain2 to guarantee the output is 0x0.
		// Because the D-Gain1 is still under the ALC control and may increment.
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0x00);
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0x00);
	} else {
		// If ALC is enabled,  the record gain is set to ALC target.
		// If ALC is disabled, the record gain is set to AD PGA gain.
		if (eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)) {
			// Set ALC target gain
			//data = 1 + ((gain - AUDIO_GAIN_0) << 1);
			if(aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
				data = 1 + ((gain - AUDIO_GAIN_0) << 1);
			} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {// AUDIO_RECG_LEVEL16
				data = 0 + (gain - AUDIO_GAIN_0);
			} else {
				data = (gain - AUDIO_GAIN_0) >> 1;
			}

			eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_L, data);
			eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_R, data);
		} else {
			// Set ADC PGA gain
			//data = 2 + ((gain - AUDIO_GAIN_0) << 2);
			if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
				data = 2 + ((gain - AUDIO_GAIN_0) << 2);
			} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {// AUDIO_RECG_LEVEL16
				data = 0 + ((gain - AUDIO_GAIN_0) << 1);
			} else {// AUDIO_RECG_LEVEL32
				data = (gain - AUDIO_GAIN_0);
			}
			eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_L, data);
			eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_R, data);
		}

		// Restore the original D-Gain2 if previously is mute.
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, audio_codec_param.digital_gain2);
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, audio_codec_param.digital_gain2);
	}

	eac_set_load();

}


#if 1 // linux no float
/*
    Set Record gain dB gain API for audio module

    This function is the Set Record gain API for audio module.
    If the Auto-Level-Control(ALC) is enabled, this API controls the ALC target value in dB unit.
    The value range for setting ALC target is -6 to -28.5 (dB).
    If the Auto-Level-Control(ALC) is disabled, this API controls the Record PGA gain factor for both Left/Right.
    The value range for settig PGA gain value is -21 to +25.5 (dB).
    If the setting value is less than -100, then it would map to MUTE state.

    @param[in] db   gain control factor in decibels.

    @return void
*/
static void audcodec_set_gaindb(INT32 db)
{
	UINT32 data;


	if (db <= -200) {
		// If record gain mute, we must set mute to D-Gain2 to guarantee the output is 0x0.
		// Because the D-Gain1 is still under the ALC control and may increment.
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0x00);
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0x00);
	} else {
		// If ALC is enabled,  the record gain is set to ALC target.
		// If ALC is disabled, the record gain is set to AD PGA gain.
		if (eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)) {
			if (db < -57) {
				db = -57;
			}

			if (db > -12) {
				db = -12;
			}

			data = (UINT32)((db + 57) / 3);

			eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_L, data);
			eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_R, data);
		} else {
			if (db < -42) {
				db = -42;
			}

			if (db > 51) {
				db = 51;
			}

			data = (UINT32)((db + 42) / 3);
			eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_L, data);
			eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_R, data);

#if _EMULATION_
			DBG_DUMP("Set PGA gain = 0x%02X (%d dB)\r\n", (unsigned int)data, (int)db/2);
#endif
		}

		// Restore the original D-Gain2 if previously is mute.
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, audio_codec_param.digital_gain2);
		eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, audio_codec_param.digital_gain2);
	}

	eac_set_load();
}
#endif

/*
    Set special feature

    This function set the special feature to the audio codec.
    The result is codec dependent.

    call audcodec_set_feature(AUDIO_FEATURE_ALC,TRUE) to open ALC
    call audcodec_set_feature(AUDIO_FEATURE_MICBOOST_A,TRUE) to open Analog boost.
    call audcodec_set_feature(AUDIO_FEATURE_MICBOOST_D,TRUE) to open Digital boost.

    Ex:
    If using external boost dac, please only and must call
    audcodec_set_feature(AUDIO_FEATURE_MICBOOST_D,TRUE) to open Digital boost.

    If Not using any external boost dac, please call
    audcodec_set_feature(AUDIO_FEATURE_MICBOOST_A,TRUE) to open Analog boost.
    audcodec_set_feature(AUDIO_FEATURE_MICBOOST_D,TRUE) to open Digital boost.


    @param[in] feature      Audio feature
     - @b AUDIO_FEATURE_ALC
     - @b AUDIO_FEATURE_MICBOOST
     - @b AUDIO_FEATURE_SPK_PWR_ALWAYSON
     - @b AUDIO_FEATURE_LINE_PWR_ALWAYSON
    @param[in] b_en      Enable/Disable the feature
     - @b TRUE: enable feature
     - @b FALSE: disable feature

    @return
	- @b FALSE: input feature not support
	- @b TRUE: input feature handle ok
*/
static BOOL audcodec_set_feature(AUDIO_FEATURE feature, BOOL b_en)
{

	switch (feature) {
	case AUDIO_FEATURE_ALC: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_EN, b_en);
		}
		break;

	case AUDIO_FEATURE_MICBOOST: {
			b_boost_en = b_en;

			if (b_en) {
				eac_set_ad_config(EAC_CONFIG_AD_PGABOOST_L, audio_codec_param.boost_value);
				eac_set_ad_config(EAC_CONFIG_AD_PGABOOST_R, audio_codec_param.boost_value);
			} else {
				eac_set_ad_config(EAC_CONFIG_AD_PGABOOST_L, EAC_PGABOOST_SEL_0DB);
				eac_set_ad_config(EAC_CONFIG_AD_PGABOOST_R, EAC_PGABOOST_SEL_0DB);
			}

			// Also modify Noise Gate
			eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD_L, audio_codec_param.noise_gate_threshold[b_en]);
			eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD_R, audio_codec_param.noise_gate_threshold[b_en]);
		}
		break;

	case AUDIO_FEATURE_NG_WITH_MICBOOST: {
			//eac_set_ad_config(EAC_CONFIG_AD_BOOST_COMPENSATE, b_en);
		}
		break;

	case AUDIO_FEATURE_SPK_PWR_ALWAYSON: {
			audio_codec_param.b_spk_power_always_on  = b_en;

			audcodec_preassign_pwr();
		}
		break;

	case AUDIO_FEATURE_HP_PWR_ALWAYSON:
	case AUDIO_FEATURE_LINE_PWR_ALWAYSON: {
			audio_codec_param.b_headphone_power_always_on  = b_en;

			audcodec_preassign_pwr();
		}
		break;

	case AUDIO_FEATURE_HP_DEPOP_EN:
	case AUDIO_FEATURE_LINE_DEPOP_EN: {
			if ((audio_codec_param.b_headphone_depop_en == FALSE) && (b_en == TRUE)) {
				audio_codec_param.b_headphone_depop_done = FALSE;
			}

			audio_codec_param.b_headphone_depop_en     = b_en;
		}
		break;

	case AUDIO_FEATURE_SWDISCHARGE_EN:
		break;


	case AUDIO_FEATURE_CHECK_PLAY_SAMPLING_RATE: {
			audio_codec_param.b_chk_play_sample_rate = b_en;
		}
		break;

	case AUDIO_FEATURE_NOISEGATE_EN: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_EN, b_en);
		}
		break;

	case AUDIO_FEATURE_ALC_IIR_EN: {
			if (pinmux_getPinmux(PIN_FUNC_AUDIO) == PIN_AUDIO_CFG_DMIC) {
				b_en =  ENABLE;
			}
			eac_set_ad_config(EAC_CONFIG_AD_IIR_ALC_L, b_en);
			eac_set_ad_config(EAC_CONFIG_AD_IIR_ALC_R, b_en);
		}
		break;
	case AUDIO_FEATURE_OUTPUT_IIR_EN: {
			if (pinmux_getPinmux(PIN_FUNC_AUDIO) == PIN_AUDIO_CFG_DMIC) {
				b_en =  ENABLE;
			}

			eac_set_ad_config(EAC_CONFIG_AD_IIR_OUT_L, b_en);
			eac_set_ad_config(EAC_CONFIG_AD_IIR_OUT_R, b_en);
		}
		break;

    case AUDIO_FEATURE_PDVCMBIAS_ALWAYSON: {

            audio_codec_param.advcm_always_on = b_en;
            audcodec_enable_adcpwr(b_en);
        }
        break;

	case AUDIO_FEATURE_CLASSD_OUT_EN: {
			DBG_WRN("No Support ClassD\r\n");
		}
		break;


	//case AUDIO_FEATURE_SPK_LEFT_EN:
	//case AUDIO_FEATURE_SPK_RIGHT_EN:
	default : {
			return FALSE;
		}
		break;

	}

	return TRUE;
}


/*
    Stop API for audio module

    This function is the Stop API for audio module

    @return void
*/
static void audcodec_stop_record(void)
{
	unsigned long flag;
	audcodec_debug(("AudStop\r\n"));

	/*
	    Handle ADC OFF
	*/
	// Enable AD reset
	eac_set_ad_config(EAC_CONFIG_AD_RESET, TRUE);

	// Disable AD power
	if(!audio_codec_param.advcm_always_on){
		audcodec_enable_adcpwr(FALSE);
	}
	// Turn Off AD Digital
	eac_set_ad_enable(FALSE);

	// After AD Disabled, We can Toggle ALC Enable to Reset ALC State Machine.
	// This can make ALC start from 0dB-PGA gain at next record start.
	// Otherwise, the ALC would start from final PGA-gain value from previous record end.
	if (eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)) {
		eac_set_ad_config(EAC_CONFIG_AD_ALC_EN, DISABLE);
		eac_set_ad_config(EAC_CONFIG_AD_ALC_EN, ENABLE);
	}
	// disable ADC clk
	// audcodec_enable_adcclk(DISABLE);

	loc_cpu(flag);
	audio_codec_param.b_record_preset = FALSE;
	audio_codec_param.audio_state &= ~AUDCODEC_STATE_RECORD;
	unl_cpu(flag);
}

/*
    Stop API for audio module

    This function is the Stop API for audio module

    @return void
*/
static void audcodec_stop_play(void)
{
	unsigned long flag;
	audcodec_debug(("AudStop\r\n"));

	/*
	    Handle DAC OFF
	*/

	if (!((audio_codec_param.b_spk_power_always_on && (audio_codec_param.output_path == AUDIO_OUTPUT_SPK)) ||
		  ((audio_codec_param.b_headphone_power_always_on) && ((audio_codec_param.output_path == AUDIO_OUTPUT_LINE) || (audio_codec_param.output_path == AUDIO_OUTPUT_HP) || (audio_codec_param.output_path == AUDIO_OUTPUT_ALL))))) {
#if _FPGA_EMULATION_
		UINT32 reg09;

		tc680_readReg(0x0009, &reg09);
		reg09 |= 0x138;
		tc680_writeReg(0x0009, reg09);
#endif

		/* Dynamic ON&OFF Path */

		// disable DAC SPK output path
		eac_set_dac_output(EAC_OUTPUT_SPK,        FALSE);

		// disable DAC LINE output path
		eac_set_dac_output(EAC_OUTPUT_LINE,       FALSE);

		// disable DAC power
		audcodec_enable_dacpwr(FALSE);

		// enable DAC reset
		eac_set_da_config(EAC_CONFIG_DA_RESET, TRUE);
	}

	eac_set_da_enable(FALSE);

	// disable DAC clk
	//audcodec_enable_dacclk(DISABLE);

	loc_cpu(flag);
	audio_codec_param.audio_state &= ~AUDCODEC_STATE_PLAY;
	unl_cpu(flag);
}


/*
    Pre Open AD Analog Block

    This function is used to pre-open AD Analog Block,
    the user must notice that the audcodec_record() must not be called in 200ms after preset.

    @return void
*/
static void audcodec_preset_record(void)
{
	unsigned long flag;
	audcodec_debug(("AudPreset\r\n"));

	loc_cpu(flag);
	audio_codec_param.audio_state |= AUDCODEC_STATE_RECORD;
	unl_cpu(flag);

	// Enable AD power
	audcodec_enable_adcpwr(TRUE);

	// Disable AD reset
	eac_set_ad_config(EAC_CONFIG_AD_RESET, FALSE);

	audio_codec_param.b_record_preset = TRUE;
}

/*
    Record API for audio module

    This function is the Record API for audio module

    @return void
*/
static void audcodec_record(void)
{
	UINT32 filterdelay;
	unsigned long flag;

	audcodec_debug(("AudRecord\r\n"));

	// enable ADC clk
	//audcodec_enable_adcclk(ENABLE);

	// If Analog is not preset, open analog block and add delay.
	// Otherwiese, skip these and start recording immediately
	if ((audio_codec_param.b_record_preset == FALSE)&& (audio_codec_param.advcm_always_on == FALSE)) {
		loc_cpu(flag);
		audio_codec_param.audio_state |= AUDCODEC_STATE_RECORD;
		unl_cpu(flag);

		// Enable AD power
		audcodec_enable_adcpwr(TRUE);

		// Disable AD reset
		eac_set_ad_config(EAC_CONFIG_AD_RESET, FALSE);

		// Anti-Pop-Noise
		// The external HW circuit would add a voltage regulator capacitor 10uF.
		// It's stable time is about 200ms and is much larger than the analog block stable time 2ms.
		// This default stable time may be not enough and the user can use audcodec_preset_record
		// to generate longer delay time for regulator capacitor.
		Delay_DelayMs(audio_codec_param.record_delay);
	} else {
		eac_set_ad_config(EAC_CONFIG_AD_RESET, FALSE);
	}

	eac_set_ad_enable(TRUE);

	// After Record Digital Enable, the Filter latency is 16T.
	filterdelay = 2125 * AUDIO_SR_8000;
    filterdelay /= audio_codec_param.audio_sample_rate;
	Delay_DelayUs(filterdelay);

	// Clear Preset status
	audio_codec_param.b_record_preset = FALSE;

}

/*
    Playback API for audio module

    This function is the Playback API for audio module

    @return void
*/
static void audcodec_playback(void)
{
	unsigned long flag;
	if (audio_codec_param.audio_state & AUDCODEC_STATE_PLAY) {
		audcodec_debug(("AudPlay Rep\r\n"));
		return;
	}



    // enable DAC clk
    //audcodec_enable_dacclk(ENABLE);


	audcodec_debug(("AudPlay %d\r\n", audio_codec_param.output_path));

	if (!((audio_codec_param.b_spk_power_always_on && (audio_codec_param.output_path == AUDIO_OUTPUT_SPK)) ||
		  ((audio_codec_param.b_headphone_power_always_on) && ((audio_codec_param.output_path == AUDIO_OUTPUT_LINE) || (audio_codec_param.output_path == AUDIO_OUTPUT_HP) || (audio_codec_param.output_path == AUDIO_OUTPUT_ALL))))) {
		// If ever play another not pwr always on path. We need depop again at next time.
		audio_codec_param.b_headphone_depop_done = FALSE;

#if _FPGA_EMULATION_
		{
			UINT32 reg09;

			tc680_readReg(0x0009, &reg09);

			// Enable output path and L/R DAC
			switch (audio_codec_param.output_path) {

			// Headphone
			// Lineout
			case AUDIO_OUTPUT_HP:
			case AUDIO_OUTPUT_LINE: {
					// enable DAC LINE output path
					eac_set_dac_output(EAC_OUTPUT_LINE, TRUE);

					reg09 &= ~0x118;
					tc680_writeReg(0x0009, reg09);

					// enable DAC power
					audcodec_enable_dacpwr(TRUE);

					// disable DAC reset
					eac_set_da_config(EAC_CONFIG_DA_RESET, FALSE);
				}
				break;

			// Speaker
			case AUDIO_OUTPUT_SPK:
			default: {
					// enable DAC SPK ClassAB output path
					eac_set_dac_output(EAC_OUTPUT_SPK, TRUE);

					reg09 &= ~0x120;
					tc680_writeReg(0x0009, reg09);

					// enable DAC power
					audcodec_enable_dacpwr(TRUE);

					// disable ClassAB DAC reset
					eac_set_da_config(EAC_CONFIG_DA_RESET, FALSE);
				}
				break;

			}


		}
#else
		// Enable output path and L/R DAC
		switch (audio_codec_param.output_path) {

		case AUDIO_OUTPUT_ALL: {
				eac_set_dac_output(EAC_OUTPUT_ALL, TRUE);

				// enable DAC power
				audcodec_enable_dacpwr(TRUE);

				// disable DAC reset
				eac_set_da_config(EAC_CONFIG_DA_RESET, FALSE);
			}
			break;

		// Headphone
		// Lineout
		case AUDIO_OUTPUT_HP:
		case AUDIO_OUTPUT_LINE: {
				// enable DAC LINE output path
				eac_set_dac_output(EAC_OUTPUT_LINE, TRUE);

		        // Depop Enable
		        eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         DISABLE);
		        eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         ENABLE);

				// enable DAC power
				audcodec_enable_dacpwr(TRUE);

				// disable DAC reset
				eac_set_da_config(EAC_CONFIG_DA_RESET, FALSE);
			}
			break;

        case AUDIO_OUTPUT_NONE: {
                // disable all output
				eac_set_dac_output(EAC_OUTPUT_ALL, FALSE);

				// enable DAC power
				audcodec_enable_dacpwr(FALSE);

				// disable DAC reset
				eac_set_da_config(EAC_CONFIG_DA_RESET, FALSE);
			}
			break;

		// Speaker
		case AUDIO_OUTPUT_SPK:
		default: {
				// enable DAC SPK ClassAB output path
				eac_set_dac_output(EAC_OUTPUT_SPK, TRUE);

		        // Depop Enable
		        eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         DISABLE);
		        eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         ENABLE);

				// enable DAC power
				audcodec_enable_dacpwr(TRUE);

				// disable ClassAB DAC reset
				eac_set_da_config(EAC_CONFIG_DA_RESET, FALSE);
			}
			break;

		}
#endif


		eac_set_da_enable(TRUE);

	} else {
		/* Pwr Always ON path */


		// Clear/Abort Depop
		eac_set_da_config(EAC_CONFIG_DA_DEPOP_EN,         DISABLE);

		// enable DAC power
		audcodec_enable_dacpwr(TRUE);

		// disable DAC reset
		eac_set_da_config(EAC_CONFIG_DA_RESET, FALSE);

		eac_set_da_enable(TRUE);

	}

	// If Output to Speaker and the DAI play source is Stereo, Use DataMixer-Average, Others as independent.
	if (audio_codec_param.output_path == AUDIO_OUTPUT_ALL) {
		eac_set_da_config(EAC_CONFIG_DA_DATAMIXER, EAC_DA_MIXER_LR_INDEPENDENT);
	}else if ((eac_get_dac_output(EAC_OUTPUT_SPK)) && (audio_codec_param.output_channel == AUDIO_CH_STEREO)) {
		eac_set_da_config(EAC_CONFIG_DA_DATAMIXER, EAC_DA_MIXER_LR_AVERAGE);
	} else {
		eac_set_da_config(EAC_CONFIG_DA_DATAMIXER, EAC_DA_MIXER_LR_INDEPENDENT);
	}


	loc_cpu(flag);
	audio_codec_param.audio_state |= AUDCODEC_STATE_PLAY;
	unl_cpu(flag);

	eac_set_load();

}

/*
    Set Audio Codec control parameters

    This function set the special parameter to the audio codec.
    The result is codec dependent.
    You should call this function after aud_init().

    @param[in] Parameter    Audio parameter. Available values are below:
     - @b AUDIO_PARAMETER_NOISEGAIN:                Set Parameter of Noise Gate Target Level.
     - @b AUDIO_PARAMETER_NOISETHD_WITH_BOOST:      Set Parameter of Noise Threshold with PGA Boost enabled.
     - @b AUDIO_PARAMETER_NOISETHD_WITHOUT_BOOST:   Set Parameter of Noise Threshold with PGA Boost disabled.
     - @b AUDIO_PARAMETER_ALC_MAXGAIN:              Set Parameter of ALC Maximum gain Factor.

    @param[in] uiSetting    parameter setting of Parameter. Below is the case by case descriptions.
     - @b AUDIO_PARAMETER_NOISEGAIN:                The Noise gate Target Level = ALC_LEVEL - (NOISE_GATE_THRESHOLD - SIGNAL_LEVEL)*(1+uiSetting).
     - @b AUDIO_PARAMETER_NOISETHD_WITH_BOOST:      Valid Rabge from 0x0~0x1F. The NG Threshold is ((1.5*uiSetting)-76.5)dB.
     - @b AUDIO_PARAMETER_NOISETHD_WITHOUT_BOOST:   Valid Rabge from 0x0~0x1F. The NG Threshold is ((1.5*uiSetting)-76.5)dB.
     - @b AUDIO_PARAMETER_ALC_MAXGAIN:              Valid Rabge from 0x0~0x7. The MAX gain is (6*uiSetting-4.5)dB. But 0x7 is the +36dB

    @return
        - @b FALSE: input parameter not support
        - @b TRUE: input parameter handle ok
*/
static BOOL audcodec_set_parameter(AUDIO_PARAMETER Parameter, UINT32 uiSetting)
{

	switch (Parameter) {
	case AUDIO_PARAMETER_NOISEGAIN: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_TARGET_L,  uiSetting);
			eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_TARGET_R,  uiSetting);
		}
		break;

	case AUDIO_PARAMETER_NOISETHD_WITH_BOOST: {
			audio_codec_param.noise_gate_threshold[ENABLE] = uiSetting;

			if (eac_get_ad_config(EAC_CONFIG_AD_PGABOOST_L) != EAC_PGABOOST_SEL_0DB) {
				eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD_L, audio_codec_param.noise_gate_threshold[ENABLE]);
			}
			if (eac_get_ad_config(EAC_CONFIG_AD_PGABOOST_R) != EAC_PGABOOST_SEL_0DB) {
				eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD_R, audio_codec_param.noise_gate_threshold[ENABLE]);
			}

		}
		break;

	case AUDIO_PARAMETER_NOISETHD_WITHOUT_BOOST: {
			audio_codec_param.noise_gate_threshold[DISABLE] = uiSetting;

			if (eac_get_ad_config(EAC_CONFIG_AD_PGABOOST_L) == EAC_PGABOOST_SEL_0DB) {
				eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD_L, audio_codec_param.noise_gate_threshold[DISABLE]);
			}

			if (eac_get_ad_config(EAC_CONFIG_AD_PGABOOST_R) == EAC_PGABOOST_SEL_0DB) {
				eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD_R, audio_codec_param.noise_gate_threshold[DISABLE]);
			}
		}
		break;

	case AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_TRESO,     uiSetting);
		}
		break;

	case AUDIO_PARAMETER_ALC_MAXGAIN: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_L,    uiSetting);
			eac_set_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_R,    uiSetting);
		}
		break;

	case AUDIO_PARAMETER_ALC_MINGAIN: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_L,    uiSetting);
			eac_set_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_R,    uiSetting);
		}
		break;

	case AUDIO_PARAMETER_ALC_ATTACK_TIME: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_ATTACK_TIME,  uiSetting);
		}
		break;

	case AUDIO_PARAMETER_ALC_DECAY_TIME: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_DECAY_TIME,   uiSetting);
		}
		break;

	case AUDIO_PARAMETER_ALC_HOLD_TIME: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_HOLD_TIME,    uiSetting);
		}
		break;

	case AUDIO_PARAMETER_ALCNG_ATTACK_TIME: {
			eac_set_ad_config(EAC_CONFIG_AD_NG_ATTACK_TIME,   uiSetting);
		}
		break;

	case AUDIO_PARAMETER_ALCNG_DECAY_TIME: {
			eac_set_ad_config(EAC_CONFIG_AD_NG_DECAY_TIME,    uiSetting);
		}
		break;

	case AUDIO_PARAMETER_ALC_TIME_RESOLUTION: {
			eac_set_ad_config(EAC_CONFIG_AD_ALC_TRESO,        uiSetting);
		}
		break;

	case AUDIO_PARAMETER_CHECKED_PLAY_SAMPLING_RATE: {
			audio_codec_param.play_sample_rate                  = uiSetting;
		}
		break;

	case AUDIO_PARAMETER_BOOST_GAIN: {
			audio_codec_param.boost_value                      = uiSetting;

			if (b_boost_en) {
				eac_set_ad_config(EAC_CONFIG_AD_PGABOOST_L,   audio_codec_param.boost_value);
				eac_set_ad_config(EAC_CONFIG_AD_PGABOOST_R,   audio_codec_param.boost_value);
			}

			if (pinmux_getPinmux(PIN_FUNC_AUDIO) == PIN_AUDIO_CFG_DMIC) {
				if (b_boost_en) {
					eac_set_ad_config(EAC_CONFIG_AD_DGAIN1_L, (20 * audio_codec_param.boost_value) + 0x73);
					eac_set_ad_config(EAC_CONFIG_AD_DGAIN1_R, (20 * audio_codec_param.boost_value) + 0x73);
				} else {
					eac_set_ad_config(EAC_CONFIG_AD_DGAIN1_L,         0x73);
					eac_set_ad_config(EAC_CONFIG_AD_DGAIN1_R,         0x73);
				}
			}
		}
		break;

	case AUDIO_PARAMETER_RECORD_DIGITAL_GAIN: {
			audio_codec_param.digital_gain2                     = uiSetting;
		}
		break;

	case AUDIO_PARAMETER_DCCAN_RESOLUTION: {
			eac_set_ad_config(EAC_CONFIG_AD_DCCAN_RESO,       uiSetting);
		}
		break;

	case AUDIO_PARAMETER_RECORD_DELAY: {
			audio_codec_param.record_delay                   = uiSetting;
		}
		break;

	case AUDIO_PARAMETER_MAXMINGAIN_OFS: {
			eac_set_ad_config(EAC_CONFIG_AD_DGAIN1_L,         uiSetting);
			eac_set_ad_config(EAC_CONFIG_AD_DGAIN1_R,         uiSetting);
			eac_set_load();
		}
		break;

	case AUDIO_PARAMETER_DAGAIN: {
			audio_codec_param.play_digital_gain                  = uiSetting;

			eac_set_da_config(EAC_CONFIG_DA_DGAIN_L,          audio_codec_param.play_digital_gain);
			eac_set_da_config(EAC_CONFIG_DA_DGAIN_R,          audio_codec_param.play_digital_gain);
			eac_set_load();
		}
		break;

	case AUDIO_PARAMETER_IIRCOEF_L:
	case AUDIO_PARAMETER_IIRCOEF_R: {
#ifdef __KERNEL__
			PAUDIO_IIRCOEF  pCoef;
			INT32           fGain;
			INT32           iCoef[7];

			if (pinmux_getPinmux(PIN_FUNC_AUDIO) == PIN_AUDIO_CFG_DMIC) {
				DBG_WRN("IIR is not valid for DMIC application\r\n");
				return FALSE;
			}

			pCoef = (PAUDIO_IIRCOEF)uiSetting;

			fGain =  pCoef->fTotalGain * pCoef->fSectionGain;
            fGain >>=15 ;

			if ((fGain >= (2*32768)) || (fGain < (-2*32768))) {
				DBG_ERR("UnSupported Filt Coef. 6 [%d]\r\n",(int)fGain);
			}

			// Transfer function is H(z) = ((B0 + B1*Z1 + B2*Z2) / (A0 - A1*Z1 - A2*Z2)) x (Total-gain) For EAC,
			// But is H(z) = ((B0 + B1*Z1 + B2*Z2) / (A0 + A1*Z1 + A2*Z2)) x (Total-gain) For matlab.
			// We must handle signness for A1 / A2 parameters.
			iCoef[0] = (pCoef->fCoefB0);
			iCoef[1] = (pCoef->fCoefB1);
			iCoef[2] = (pCoef->fCoefB2);
			iCoef[3] = (pCoef->fCoefA0);
			iCoef[4] = (-(pCoef->fCoefA1));
			iCoef[5] = (-(pCoef->fCoefA2));
			iCoef[6] = (fGain);


			eac_set_iir_coef((EAC_IIRCH_LEFT << (Parameter - AUDIO_PARAMETER_IIRCOEF_L)), iCoef);
#endif
		}
		break;

	default: {
			//DBG_WRN("set Param ID Err!\r\n");
			return FALSE;
		}

	}

	return TRUE;
}

/*
    Check Sampling Rate API for audio module

    This function check sampling rate set by feature AUDIO_PARAMETER_CHECKED_PLAY_SAMPLING_RATE

    @return
        - @b TRUE: sampling rate is valid. OR not require to check
        - @b FALSE: sampling rate is invalid
*/
static BOOL audcodec_chk_samplerate(void)
{
	if (audio_codec_param.b_chk_play_sample_rate) {
		if (audio_codec_param.play_sample_rate != audio_codec_param.audio_sample_rate) {
			DBG_ERR("installed SR %d, but current SR is %d\r\n",
					(int)audio_codec_param.play_sample_rate, (int)audio_codec_param.audio_sample_rate);
			return FALSE;
		}
	}

	return TRUE;
}
#endif

#ifdef __KERNEL__
EXPORT_SYMBOL(audcodec_get_fp);
#endif

//@}
