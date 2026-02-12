/*
    Audio module driver

    This file is the driver of audio module

    @file       Audio.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifdef __KERNEL__
#include "kwrap/type.h"
#include <mach/rcw_macro.h>
#elif defined(__FREERTOS)
#include "kwrap/type.h"
#include "rcw_macro.h"
#include <string.h>
#endif
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
ID     FLG_ID_KDRV_AUD;
SEM_HANDLE SEMID_KDRV_AUD;
#include "kwrap/spinlock.h"
static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)


#include "kdrv_audioio_int.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"


static BOOL                 gkdrv_audio_init = FALSE;
static KDRV_AUDIO_INFO      aud_info[KDRV_HANDLE_MAX]={{0},{0},{0},{0}};
static AUDIO_CODEC_FUNC     gkdrv_codec_function = {0};
static AUDIO_SR             gkdrv_samplerate = AUDIO_SR_48000;
static AUDIO_DEFSET         gkdrv_defset = AUDIO_DEFSET_20DB;
static KDRV_CALLBACK_FUNC   gkdrv_audio_timecode_hit_cb[KDRV_HANDLE_MAX]={{0},{0},{0},{0}};
static KDRV_AUDIO_INT_FLAG  gkdrv_int_flag = KDRV_AUDIO_INT_FLAG_NONE;
static UINT32               gkdrv_rx_trigger_delay = 0;
static KDRV_AUDIO_LIST_NODE gkdrv_list_node[KDRV_HANDLE_MAX][KDRV_LIST_NODE_NUM];
static KDRV_AUDIO_LIST_NODE *gpkdrv_list_node[KDRV_HANDLE_MAX];
static LIST_HEAD            gkdrv_list_head[KDRV_HANDLE_MAX];
static UINT32               gkdrv_list_node_number[KDRV_HANDLE_MAX] = {0};
static AUDIO_BUF_QUEUE      gkdrv_aud_buf_queue[KDRV_HANDLE_MAX];
static AUDIO_BUF_QUEUE      *gpkdrv_aud_buf_queue[KDRV_HANDLE_MAX];
static UINT32               gkdrv_i2s_rx = 0, gkdrv_i2s_tx = 0;
static AUDIO_DEVICE_OBJ     aud_device =
{
	.uiGPIOColdReset    = 0,
	.uiI2SCtrl          = AUDIO_I2SCTRL_NONE,
	.uiChannel          = 0,
	.uiGPIOData         = 0,
	.uiGPIOClk          = 0,
	.uiGPIOCS           = 0,
	.uiADCZero          = 0
};

static AUDIO_SETTING        aud_setting = {
	.Clock.bClkExt      = FALSE,
	.Clock.Clk          = 0,
	.I2S.bMaster        = TRUE,
	.I2S.I2SFmt         = AUDIO_I2SFMT_STANDARD,
	.I2S.ClkRatio       = AUDIO_I2SCLKR_256FS_32BIT,
	.Fmt                = AUDIO_FMT_I2S,
	.SamplingRate       = AUDIO_SR_48000,
	.Channel            = AUDIO_CH_STEREO,
	.RecSrc             = AUDIO_RECSRC_MIC,
	.Output             = AUDIO_OUTPUT_NONE,
	.bEmbedded          = TRUE
};

KDRV_AUDIOLIB_FUNC* audlib_func[KDRV_AUDIOLIB_ID_MAX] = {0};


#if defined (__KERNEL__)
static BOOL kdrv_first_boot = TRUE;
#endif


#if 1 // kdrv internal used API

void kdrv_audio_create_resource(void)
{
	OS_CONFIG_FLAG(FLG_ID_KDRV_AUD);
#ifdef __KERNEL__
	SEM_CREATE(SEMID_KDRV_AUD, 1);
#else
	OS_CONFIG_SEMPHORE(SEMID_KDRV_AUD, 0, 1, 1);
#endif

}

void kdrv_audio_release_resource(void)
{
	rel_flg(FLG_ID_KDRV_AUD);
	SEM_DESTROY(SEMID_KDRV_AUD);

}

void kdrv_audio_init(void)
{
	kdrv_audio_create_resource();
}

void kdrv_audio_exit(void)
{
	kdrv_audio_release_resource();
}


/*
    Lock KDRV_AUD module

    Use semaphore lock for the KDRV_AUD module

    @return
	@b E_OK: success
	@b Else: fail
*/
ER kdrv_audio_lock(void)
{
	ER er_ret;

	er_ret        = SEM_WAIT(SEMID_KDRV_AUD);
	if (er_ret != E_OK) {
		audio_msg("wait semaphore fail\r\n");
		return er_ret;
	}

	return E_OK;
}

/*
    Unlock KDRV_AUD module

    Release semaphore lock for the KDRV_AUD module

    @return
	@b E_OK: success
	@b Else: fail
*/
ER kdrv_audio_unlock(void)
{
	SEM_SIGNAL(SEMID_KDRV_AUD);
	return E_OK;
}




UINT32 _kdrv_get_handler(UINT32 id)
{
    UINT32 engine,handler;

    engine = KDRV_DEV_ID_ENGINE(id);

    switch (engine){
        case KDRV_AUDCAP_ENGINE0:{
                handler = KDRV_HANDLE_RX1;
            }break;
        case KDRV_AUDCAP_ENGINE1:{
                handler = KDRV_HANDLE_TXLB;
            }break;
        case KDRV_AUDOUT_ENGINE0:{
                handler = KDRV_HANDLE_TX1;
            }break;
        case KDRV_AUDOUT_ENGINE1:{
                handler = KDRV_HANDLE_TX2;
            }break;
        default:{
                audio_msg("audio engine Id 0x%x is not valid \r\n",(unsigned int)id);
             return -1;
            }
    }

    return handler;
}

void _kdrv_set_volume(UINT32 handler, UINT32 volume)
{
    UINT32 addition_digi_gain = 0;

    if ( volume == 0 ) {
        if (handler == KDRV_HANDLE_RX1)
            aud_set_gain(AUDIO_GAIN_MUTE);
        else if ( handler == KDRV_HANDLE_TX1 ||  handler == KDRV_HANDLE_TX2)
            aud_set_volume(AUDIO_VOL_MUTE);

        return;
    }

    if ( volume > 100) {
        addition_digi_gain = volume - 100;
        if (handler == KDRV_HANDLE_RX1) {
            if(aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
				aud_set_gain(AUDIO_GAIN_7);
			} if(aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {
				aud_set_gain(AUDIO_GAIN_15);
			} else {
				aud_set_gain(AUDIO_GAIN_31);
			}
            eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0xC3 + addition_digi_gain);
            eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0xC3 + addition_digi_gain);
        } else if ( handler == KDRV_HANDLE_TX1 ||  handler == KDRV_HANDLE_TX2) {
            aud_set_volume(AUDIO_VOL_63);
            eac_set_da_config(EAC_CONFIG_DA_DGAIN_L, 0xBF + addition_digi_gain); // the initial value needs to refer to EAC's init value
            eac_set_da_config(EAC_CONFIG_DA_DGAIN_R, 0xBF + addition_digi_gain); // refer to AudCodecEmbd.c audio_codec_param.play_digital_gain
        }
		eac_set_load();
    } else if ( volume > 0){
        if (handler == KDRV_HANDLE_RX1) {
            if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
				volume /=12; // separate to 8 level.
			} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {
				volume /=6; // separate to 16 level.
			} else {
				volume /=3; // separate to 32 level.
			}

            aud_set_gain(volume+1);
        } else if ( handler == KDRV_HANDLE_TX1 ||  handler == KDRV_HANDLE_TX2) {
            volume <<=1;
            volume /=3;
            volume ++;// scale down to around 64 level and prevent mute.
            if(volume > AUDIO_VOL_63) {
                volume = AUDIO_VOL_63;
            }
            aud_set_volume(volume);
        }
    }
    return;
}

void _kdrv_set_lr_volume(UINT32 handler, UINT32 volume,UINT32 sel){

	UINT32 addition_digi_gain = 0;
	UINT32 gain = 0;

	if(volume == 0){
        if (handler == KDRV_HANDLE_RX1){
			if(sel == 0){
				eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0x00);
			}else{
				eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0x00);
			}
		} else if ( handler == KDRV_HANDLE_TX1 ||  handler == KDRV_HANDLE_TX2){
			if(sel == 0){
				eac_set_da_config(EAC_CONFIG_DA_DGAIN_L, 0x00);
			}else{
				eac_set_da_config(EAC_CONFIG_DA_DGAIN_R, 0x00);
			}
		}
		eac_set_load();
        return;
	}

	if(volume > 100){
		addition_digi_gain = volume - 100;
		if (handler == KDRV_HANDLE_RX1){
			if(sel == 0){
				if (eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)) {
					// Set ALC target gain
					eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_L, EAC_ALC_TARGET_N6P0_DB);//max
				} else {
					// Set ADC PGA gain
					eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_L, EAC_AD_PGAGAIN_P25P5_DB);//max
				}
				eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0xC3+addition_digi_gain);

			}else{
				if (eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)) {
					// Set ALC target gain
					eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_R, EAC_ALC_TARGET_N6P0_DB);//max
				} else {
					// Set ADC PGA gain
					eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_R, EAC_AD_PGAGAIN_P25P5_DB);//max
				}
				eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0xC3+addition_digi_gain);

			}
		} else if ( handler == KDRV_HANDLE_TX1 ||  handler == KDRV_HANDLE_TX2){
			if(sel == 0){
				eac_set_da_config(EAC_CONFIG_DA_DGAIN_L, 0xD0+volume); //from volume = AUDIO_VOL_63 to add up
			}else{
				eac_set_da_config(EAC_CONFIG_DA_DGAIN_R, 0xD0+volume);
			}
		}
	} else if (volume > 0) { // volume 0~100

		if (handler == KDRV_HANDLE_RX1){
			if(sel == 0){
				if (eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)) {
					// Set ALC target gain
					if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
						volume /=12; // separate to 8 level.
					} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {
						volume /=6; // separate to 16 level.
					} else {
						volume /=3; // separate to 32 level.
					}

					gain = (volume) + 1;

					if ((int)gain > (int)aud_get_Total_gain_level()) {
				
						gain = (int)aud_get_Total_gain_level();
					}

					if(aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
						gain = 1 + ((gain - AUDIO_GAIN_0) << 1);
					} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {// AUDIO_RECG_LEVEL16
						gain = 0 + (gain - AUDIO_GAIN_0);
					} else {
						gain = (gain - AUDIO_GAIN_0) >> 1;
					}

					eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_L, gain);//map 1~6:1 7~13:2 ... 91~97:14 98~100:15(max)
				} else {
					// Set ADC PGA gain
					if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
						volume /=12; // separate to 8 level.
					} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {
						volume /=6; // separate to 16 level.
					} else {
						volume /=3; // separate to 32 level.
					}

					gain = (volume) + 1;

					if ((int)gain > (int)aud_get_Total_gain_level()) {
						
						gain = (int)aud_get_Total_gain_level();
					}

					if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
						gain = 2 + ((gain - AUDIO_GAIN_0) << 2);
					} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {// AUDIO_RECG_LEVEL16
						gain = 0 + ((gain - AUDIO_GAIN_0) << 1);
					} else {// AUDIO_RECG_LEVEL32
						gain = (gain - AUDIO_GAIN_0);
					}
					eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_L, gain);//max
				}
				eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_L, 0xC3);

			}else{
				if (eac_get_ad_config(EAC_CONFIG_AD_ALC_EN)) {
					// Set ALC target gain
					if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
						volume /=12; // separate to 8 level.
					} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {
						volume /=6; // separate to 16 level.
					} else {
						volume /=3; // separate to 32 level.
					}

					gain = (volume) + 1;

					if ((int)gain > (int)aud_get_Total_gain_level()) {
						
						gain = (int)aud_get_Total_gain_level();
					}

					if(aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
						gain = 1 + ((gain - AUDIO_GAIN_0) << 1);
					} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {// AUDIO_RECG_LEVEL16
						gain = 0 + (gain - AUDIO_GAIN_0);
					} else {
						gain = (gain - AUDIO_GAIN_0) >> 1;
					}

					eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET_R, gain);//map 1~6:1 7~13:2 ... 91~97:14 98~100:15(max)
				} else {
					// Set ADC PGA gain
					if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
						volume /=12; // separate to 8 level.
					} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {
						volume /=6; // separate to 16 level.
					} else {
						volume /=3; // separate to 32 level.
					}

					gain = (volume) + 1;

					if ((int)gain > (int)aud_get_Total_gain_level()) {
						
						gain = (int)aud_get_Total_gain_level();
					}

					if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL8) {
						gain = 2 + ((gain - AUDIO_GAIN_0) << 2);
					} else if (aud_get_Total_gain_level() == AUDIO_RECG_LEVEL16) {// AUDIO_RECG_LEVEL16
						gain = 0 + ((gain - AUDIO_GAIN_0) << 1);
					} else {// AUDIO_RECG_LEVEL32
						gain = (gain - AUDIO_GAIN_0);
					}
					eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_R, gain);//max

				}
				eac_set_ad_config(EAC_CONFIG_AD_DGAIN2_R, 0xC3);

			}
		} else if ( handler == KDRV_HANDLE_TX1 ||  handler == KDRV_HANDLE_TX2){
			volume *=6;
			volume /=10;
			// -31 ~ +6.5 dB
			volume = 0x85 + ((volume - AUDIO_VOL_0) * 6 / 5);
			if(sel == 0){
				eac_set_da_config(EAC_CONFIG_DA_DGAIN_L, volume);
			}else{
				eac_set_da_config(EAC_CONFIG_DA_DGAIN_R, volume);
			}
		}
	}

	eac_set_load();
	return;

}



/*
return 0 on valid config, -1 on invalid config
*/
INT32 _kdrv_chk_set_config(UINT32 handler, KDRV_AUDIOIO_PARAM_ID param_id, VOID *param)
{
    switch (handler) {
        case KDRV_HANDLE_RX1:
        case KDRV_HANDLE_TXLB:{
                if(param_id >= KDRV_AUDIOIO_OUT_BASE)
                    return -1;
            }break;
        case KDRV_HANDLE_TX1:
        case KDRV_HANDLE_TX2:{
                if((param_id >= KDRV_AUDIOIO_CAP_BASE) && (param_id < KDRV_AUDIOIO_OUT_BASE))
                    return -1;
            }break;
    }


    switch (param_id) {
        case KDRV_AUDIOIO_GLOBAL_SAMPLE_RATE : {
                switch (*(AUDIO_SR *)param) {
                        case AUDIO_SR_8000:
                        case AUDIO_SR_11025:
                        case AUDIO_SR_12000:
                        case AUDIO_SR_16000:
                        case AUDIO_SR_22050:
                        case AUDIO_SR_24000:
                        case AUDIO_SR_32000:
                        case AUDIO_SR_44100:
                        case AUDIO_SR_48000:
                            break;

                        default : {
                                audio_msg("invalid audio sample rate %d needs to be 8000/11025/12000/16000/22050/24000/32000/44100/48000\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_GLOBAL_IS_BUSY : {
            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_BIT_WIDTH : {
                switch (*(UINT32 *)param) {
                        case 16:
                        case 32:
                            break;

                        default : {
                                audio_msg("invalid audio i2s bit width %d needs to be 16/32\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_BITCLK_RATIO : {
                switch (*(UINT32 *)param) {
                        case 32:
                        case 64:
                        case 128:
                        case 256:
                            break;

                        default : {
                                audio_msg("invalid audio i2s bit clock ratio %d needs to be 32/64/128/256\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_OPMODE : {
                switch (*(KDRV_AUDIO_I2S_OPMODE *)param) {
                        case KDRV_AUDIO_I2S_OPMODE_SLAVE:
                        case KDRV_AUDIO_I2S_OPMODE_MASTER:
                            break;

                        default : {
                                audio_msg("invalid audio i2s clock mode  %d plz refer to KDRV_AUDIO_I2S_OPMODE\r\n",*(int *)param);
                                return -1;
                            }
                }
            }break;

		case KDRV_AUDIOIO_GLOBAL_I2S_DATA_ORDER : {
			}break;

        case KDRV_AUDIOIO_GLOBAL_CLOCK_ALWAYS_ON : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio clk always on setting %d need to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_PATH : {
                switch (*(KDRV_AUDIO_CAP_PATH *)param) {
                        case KDRV_AUDIO_CAP_PATH_AMIC:
                        case KDRV_AUDIO_CAP_PATH_DMIC:
                        case KDRV_AUDIO_CAP_PATH_I2S:
                            break;

                        default : {
                                audio_msg("invalid audio cap path setting %d plz refer to KDRV_AUDIO_CAP_PATH\r\n",*(int *)param);
                                return -1;
                            }
                    }

            }break;

        case KDRV_AUDIOIO_CAP_DEFAULT_CFG : {
                switch (*(KDRV_AUDIO_CAP_DEFSET *)param) {
                        case KDRV_AUDIO_CAP_DEFSET_10DB:
                        case KDRV_AUDIO_CAP_DEFSET_10DB_HP_8K:
                        case KDRV_AUDIO_CAP_DEFSET_10DB_HP_16K:
						case KDRV_AUDIO_CAP_DEFSET_10DB_HP_32K:
                        case KDRV_AUDIO_CAP_DEFSET_10DB_HP_48K:
                        case KDRV_AUDIO_CAP_DEFSET_20DB:
                        case KDRV_AUDIO_CAP_DEFSET_20DB_HP_8K:
                        case KDRV_AUDIO_CAP_DEFSET_20DB_HP_16K:
						case KDRV_AUDIO_CAP_DEFSET_20DB_HP_32K:
                        case KDRV_AUDIO_CAP_DEFSET_20DB_HP_48K:
                        case KDRV_AUDIO_CAP_DEFSET_30DB:
                        case KDRV_AUDIO_CAP_DEFSET_30DB_HP_8K:
                        case KDRV_AUDIO_CAP_DEFSET_30DB_HP_16K:
						case KDRV_AUDIO_CAP_DEFSET_30DB_HP_32K:
                        case KDRV_AUDIO_CAP_DEFSET_30DB_HP_48K:
						case KDRV_AUDIO_CAP_DEFSET_0DB:
						case KDRV_AUDIO_CAP_DEFSET_0DB_HP_8K:
						case KDRV_AUDIO_CAP_DEFSET_0DB_HP_16K:
						case KDRV_AUDIO_CAP_DEFSET_0DB_HP_32K:
						case KDRV_AUDIO_CAP_DEFSET_0DB_HP_48K:
						case KDRV_AUDIO_CAP_DEFSET_ALCOFF:
						case KDRV_AUDIO_CAP_DEFSET_ALCOFF_HP_8K:
						case KDRV_AUDIO_CAP_DEFSET_ALCOFF_HP_16K:
						case KDRV_AUDIO_CAP_DEFSET_ALCOFF_HP_32K:
						case KDRV_AUDIO_CAP_DEFSET_ALCOFF_HP_48K:
						case KDRV_AUDIO_CAP_DEFSET_DMIC:
						case KDRV_AUDIO_CAP_DEFSET_DMIC_LP_8K:
						case KDRV_AUDIO_CAP_DEFSET_DMIC_LP_16K:
						case KDRV_AUDIO_CAP_DEFSET_DMIC_LP_32K:
						case KDRV_AUDIO_CAP_DEFSET_DMIC_LP_48K:
                            break;

                        default : {
                                audio_msg("invalid audio cap default setting %d plz refer to KDRV_AUDIO_CAP_DEFSET\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_CHANNEL_NUMBER :
        case KDRV_AUDIOIO_CAP_CHANNEL_NUMBER : {
                switch (*(UINT32 *)param) {
                        case 6:
                        case 8:
                            if(!dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN)){
                                audio_msg("invalid audio cap/out channel number %d when not using i2s interface\r\n",*(int *)param);
                                return -1;
                            }
                        case 4:
                            if(!dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN) && aud_setting.RecSrc != AUDIO_RECSRC_DMIC){
                                audio_msg("invalid audio cap/out channel number %d when not using i2s interface or DMIC\r\n",*(int *)param);
                                return -1;
                            }
                        case 1:
                        case 2:
                            break;

                        default : {
                                audio_msg("invalid audio cap/out channel number %d\r\n",*(int *)param);
                                return -1;
                            }
                    }

            }break;

		case KDRV_AUDIOIO_CAP_DMIC_CH : {
                switch (*(UINT32 *)param) {
                        case 2:
                        case 4:
                            break;

                        default : {
                                audio_msg("invalid audio D-mic channel number %d\r\n",*(int *)param);
                                return -1;
                            }
                    }
			}break;

        case KDRV_AUDIOIO_CAP_MONO_SEL : {
                switch (*(KDRV_AUDIO_CAP_MONO *)param) {
                        case KDRV_AUDIO_CAP_MONO_LEFT:
                        case KDRV_AUDIO_CAP_MONO_RIGHT:
                            break;

                        default : {
                                audio_msg("invalid audio cap mono ch select %d plz refer to KDRV_AUDIO_CAP_MONO\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_MONO_EXPAND :
        case KDRV_AUDIOIO_CAP_MONO_EXPAND : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio cap/out mono expand setting %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_BIT_WIDTH :
        case KDRV_AUDIOIO_CAP_BIT_WIDTH : {
                switch (*(UINT32 *)param) {
                        case 8:
                        case 16:
                        case 32:
                            break;

                        default : {
                                audio_msg("invalid audio cap/out bit width %d needs to be 8/16/32\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_VOLUME :
		case KDRV_AUDIOIO_OUT_VOLUME_LEFT:
		case KDRV_AUDIOIO_OUT_VOLUME_RIGHT:
		case KDRV_AUDIOIO_CAP_VOLUME_LEFT:
		case KDRV_AUDIOIO_CAP_VOLUME_RIGHT:
        case KDRV_AUDIOIO_CAP_VOLUME : {
                if(*(INT32 *)param < 0){
                        audio_msg("invalid audio cap/out volume %d needs to >= 0\r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_DCCAN_RESOLUTION : {
                if(*(INT32 *)param < 0 || *(INT32 *)param >7){
                        audio_msg("invalid audio cap dc cancellation resolution %d needs to be 0~7\r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_TRIGGER_DELAY : {
                if(*(INT32 *)param < 0){
                        audio_msg("invalid audio cap delay setting %d needs to >= 0 \r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_AMIC_BOOST : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 10:
                        case 20:
                        case 30:
                            break;

                        default : {
                                audio_msg("invalid audio cap AMIC boost value %d needs to be 0/10/20/30\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_EN : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio cap alc enable setting %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_MAXGAIN : {
                if(*(INT32 *)param < 0 || *(INT32 *)param > EAC_ALC_MAXGAIN_MAX) {
                        audio_msg("invalid audio cap alc max gain %d needs to be 0x00~0x1F \r\n",*(INT32 *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_MINGAIN : {
                if(*(INT32 *)param < 0 || *(INT32 *)param > EAC_ALC_MINGAIN_MAX) {
                        audio_msg("invalid audio cap alc min gain %d needs to be 0x00~0x1F \r\n",*(INT32 *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_ATTACK_TIME :{
                if(*(INT32 *)param < 0 || *(INT32 *)param >10){
                        audio_msg("invalid audio cap alc attack time %d needs to be 0~10\r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_DECAY_TIME :{
                if(*(INT32 *)param < 0 || *(INT32 *)param >10){
                        audio_msg("invalid audio cap alc decay time %d needs to be 0~10\r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_HOLD_TIME :{
                if(*(INT32 *)param < 0 || *(INT32 *)param >15){
                        audio_msg("invalid audio cap alc hold time %d needs to be 0~15\r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_TIME_RESOLUTION : {
                switch (*(KDRV_AUDIO_CAP_TRESO_BASIS *)param) {
                        case KDRV_AUDIO_CAP_TRESO_BASIS_800US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_1000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_2000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_5000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_10000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_15000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_45000US:
                            break;

                        default : {
                                audio_msg("invalid audio cap alc time resolution %d plz refer to KDRV_AUDIO_CAP_TRESO_BASIS\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_EN : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio cap noise gate enable setting %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_THRESHOLD : {
                if(*(INT32 *)param < -77 || *(INT32 *)param > -30) {
                        audio_msg("invalid audio cap alc noise gate threshold %d needs to be -77~-30 \r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_NG_BOOST_COMPENSATION : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio cap alc noise gate compensation  %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGAIN : {
                if(*(INT32 *)param < 0 || *(INT32 *)param > 0xf) {
                        audio_msg("invalid audio cap alc noise gain %x needs to be 0x0~0xF \r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALCNG_ATTACK_TIME : {
                if(*(INT32 *)param < 0 || *(INT32 *)param >10){
                        audio_msg("invalid audio cap alc noise gate attack time %d needs to be 0~10\r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALCNG_DECAY_TIME : {
                if(*(INT32 *)param < 0 || *(INT32 *)param >10){
                        audio_msg("invalid audio cap alc noise gate decay time %d needs to be 0~10\r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_TIME_RESOLUTION : {
                 switch (*(KDRV_AUDIO_CAP_TRESO_BASIS *)param) {
                        case KDRV_AUDIO_CAP_TRESO_BASIS_800US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_1000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_2000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_5000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_10000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_15000US:
                        case KDRV_AUDIO_CAP_TRESO_BASIS_45000US:
                            break;

                        default : {
                                audio_msg("invalid audio cap alc noise gate time resolution %d plz refer to KDRV_AUDIO_CAP_TRESO_BASIS\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_IIR_EN : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio cap alc iir enable   %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_OUTPUT_IIR_EN : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio cap iir output enable   %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_IIRCOEF_R :
        case KDRV_AUDIOIO_CAP_IIRCOEF_L : {
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_HIT_EN :
        case KDRV_AUDIOIO_CAP_TIMECODE_HIT_EN : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio cap/out timecode hit enable   %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_HIT_CB :
        case KDRV_AUDIOIO_CAP_TIMECODE_HIT_CB : {
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_TRIGGER :
        case KDRV_AUDIOIO_CAP_TIMECODE_TRIGGER : {
                switch (*(AUDIO_SR *)param) {
                        case AUDIO_SR_8000:
                        case AUDIO_SR_11025:
                        case AUDIO_SR_12000:
                        case AUDIO_SR_16000:
                        case AUDIO_SR_22050:
                        case AUDIO_SR_24000:
                        case AUDIO_SR_32000:
                        case AUDIO_SR_44100:
                        case AUDIO_SR_48000:
                            break;

                        default : {
                                audio_msg("invalid audio cap/out time code hit value %d needs to be 8000/11025/12000/16000/22050/24000/32000/44100/48000\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_OFFSET :
        case KDRV_AUDIOIO_CAP_TIMECODE_OFFSET : {
                if(*(INT32 *)param < 0){
                        audio_msg("invalid audio cap/out timecode offset value %d needs to >= 0\r\n",*(int *)param);
                        return -1;
                    }
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_VALUE :
        case KDRV_AUDIOIO_CAP_TIMECODE_VALUE : {
            }break;

        case KDRV_AUDIOIO_OUT_PATH : {
                switch (*(KDRV_AUDIO_OUT_PATH *)param) {
                        case KDRV_AUDIO_OUT_PATH_NONE:
                        case KDRV_AUDIO_OUT_PATH_SPEAKER:
                        case KDRV_AUDIO_OUT_PATH_LINEOUT:
                        case KDRV_AUDIO_OUT_PATH_I2S:
                        case KDRV_AUDIO_OUT_PATH_I2S_ONLY:
						case KDRV_AUDIO_OUT_PATH_ALL:
                            break;

                        default : {
                                audio_msg("invalid audio out path %d plz refer to KDRV_AUDIO_OUT_PATH\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_HDMI_PATH_EN : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio out HDMI enable   %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_MONO_SEL : {
                switch (*(KDRV_AUDIO_OUT_MONO *)param) {
                        case KDRV_AUDIO_OUT_MONO_LEFT:
                        case KDRV_AUDIO_OUT_MONO_RIGHT:
                            break;

                        default : {
                                audio_msg("invalid audio out mono ch select %d plz refer to KDRV_AUDIO_OUT_MONO\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_SPK_PWR_ALWAYS_ON : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio out spk power alyways on enable   %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_OUT_LINE_PWR_ALWAYS_ON : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio out lineout power alyways on enable   %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

        case KDRV_AUDIOIO_CAP_PDVCMBIAS_ALWAYS_ON : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio in mic ad Vcm power alyways on enable   %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

		case KDRV_AUDIOIO_OUT_STOP_TX : {
                switch (*(UINT32 *)param) {
                        case 0:
                        case 1:
                            break;

                        default : {
                                audio_msg("invalid audio out stop tx   %d needs to be 1/0\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;

		case KDRV_AUDIOIO_CAP_GAIN_LEVEL : {
                switch (*(UINT32 *)param) {
                        case KDRV_AUDIO_CAP_GAIN_LEVEL8:
                        case KDRV_AUDIO_CAP_GAIN_LEVEL16:
                        case KDRV_AUDIO_CAP_GAIN_LEVEL32:
                            break;



                        default : {
                                audio_msg("invalid audio in mic gain level %d plz refer to KDRV_AUDIO_CAP_GAIN_LEVEL\r\n",*(int *)param);
                                return -1;
                            }
                    }
            }break;




        default:{
                audio_msg("invalid audio parameter param_id %d \r\n",param_id);
                return -1;
            }
    }




    return 0;

}

INT32 _kdrv_chk_get_config(UINT32 handler, KDRV_AUDIOIO_PARAM_ID param_id)
{
    switch (handler) {
        case KDRV_HANDLE_RX1:
        case KDRV_HANDLE_TXLB:{
                if(param_id >= KDRV_AUDIOIO_OUT_BASE)
                    return -1;
            }break;
        case KDRV_HANDLE_TX1:
        case KDRV_HANDLE_TX2:{
                if((param_id >= KDRV_AUDIOIO_CAP_BASE) && (param_id < KDRV_AUDIOIO_OUT_BASE))
                    return -1;
            }break;
    }
    return 0;

}

INT32 _kdrv_get_valid_list_node(UINT32 handler)
{
    UINT32 idx;

    for (idx = 0; idx < KDRV_LIST_NODE_NUM ;idx ++) {
        if(gkdrv_list_node[handler][idx].b_used == 0){
                gkdrv_list_node[handler][idx].b_used=1;
                gpkdrv_list_node[handler] =  &gkdrv_list_node[handler][idx];
                return 0;
            }
    }
    return -1;
}

void _kdrv_add_node(UINT32 handler, KDRV_AUDIO_LIST_NODE * node)
{
	unsigned long flag;
	loc_cpu(flag);
    if (gkdrv_list_node_number[handler] < KDRV_LIST_NODE_NUM){
        list_add_tail(&node->list,&gkdrv_list_head[handler]);
        gkdrv_list_node_number[handler]++;
        //audio_msg("gkdrv_list_node_number =  %d \r\n",gkdrv_list_node_number[handler]);
    }
	unl_cpu(flag);
}

void _kdrv_remove_node(UINT32 handler, KDRV_AUDIO_LIST_NODE * node)
{
	unsigned long flag;
	loc_cpu(flag);
    if (gkdrv_list_node_number[handler] != 0){
        list_del_init(&node->list);
        gkdrv_list_node_number[handler]--;
        //audio_msg("gkdrv_list_node_number =  %d \r\n",gkdrv_list_node_number[handler]);
    }
	unl_cpu(flag);
}

void _kdrv_reset_node_list(UINT32 handler)
{
	gkdrv_list_node_number[handler] = 0;
	INIT_LIST_HEAD(&gkdrv_list_head[handler]);
	memset(gkdrv_list_node[handler], 0, sizeof(KDRV_AUDIO_LIST_NODE)*KDRV_LIST_NODE_NUM);
}


#endif

#if 1 // call back handle

void kdrv_audio_event_cb_rx(UINT32 event)
{
    KDRV_AUDIO_LIST_NODE *pnode;
    PAUDTS_OBJ            pkdrv_tsobj;
    UINT32               cb_info;
	unsigned long 		 flag;

    cb_info = KDRV_HANDLE_RX1;

	clr_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_RX_ISR_IDLE);

	if (!aud_info[KDRV_HANDLE_RX1].opened) {
		set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_RX_ISR_IDLE);
		return;
	}


    pkdrv_tsobj = aud_info[KDRV_HANDLE_RX1].paud_drv_obj;


	if (event & AUDIO_EVENT_DMADONE) {

		if(!list_empty(&gkdrv_list_head[KDRV_HANDLE_RX1])){
			loc_cpu(flag);
		    pnode = list_entry(gkdrv_list_head[KDRV_HANDLE_RX1].next, KDRV_AUDIO_LIST_NODE, list);
			unl_cpu(flag);
		    //audio_msg("rx event status = %x \r\n",event);
		} else {
			pkdrv_tsobj->getDoneBufferFromQueue(0);
			audio_msg("empty node with dma done rx \r\n");
			set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_RX_ISR_IDLE);
			return;
		}
        pkdrv_tsobj->getDoneBufferFromQueue(0);

        _kdrv_remove_node(KDRV_HANDLE_RX1,pnode);
        pnode->cb_func.callback(&cb_info,&pnode->user_data);
        pnode->cb_func.reserve_buf(pnode->buffer_addr.addr_pa);
        pnode->cb_func.free_buf(pnode->buffer_addr.addr_pa);
        pnode->b_used = 0;

        if(list_empty(&gkdrv_list_head[KDRV_HANDLE_RX1])){
                pkdrv_tsobj->stop();
                audio_msg("record stop \r\n");
            }
	}

	if (event & AUDIO_EVENT_TCHIT) {
        gkdrv_audio_timecode_hit_cb[KDRV_HANDLE_RX1].reserve_buf(event);
	}

	if (event & AUDIO_EVENT_TCLATCH) {
	}

	if (event & KDRV_ERRFLAG_RX) {
	//	audio_msg("Rx ERROR! Flag=0x%08X\r\n", (event & KDRV_ERRFLAG_RX));
	}

	set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_RX_ISR_IDLE);

}

void kdrv_audio_event_cb_txlb(UINT32 event)
{
    KDRV_AUDIO_LIST_NODE *pnode;
    PAUDTS_OBJ            pkdrv_tsobj;
    UINT32               cb_info;
	unsigned long 		 flag;

    cb_info = KDRV_HANDLE_TXLB;

	clr_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TXLB_ISR_IDLE);

	if (!aud_info[KDRV_HANDLE_TXLB].opened) {
		set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TXLB_ISR_IDLE);
		return;
	}

    pkdrv_tsobj = aud_info[KDRV_HANDLE_TXLB].paud_drv_obj;

	if (event & AUDIO_EVENT_DMADONE) {

		if(!list_empty(&gkdrv_list_head[KDRV_HANDLE_TXLB])){
			loc_cpu(flag);
		    pnode = list_entry(gkdrv_list_head[KDRV_HANDLE_TXLB].next, KDRV_AUDIO_LIST_NODE, list);
			unl_cpu(flag);
		    //audio_msg("txlb event status = %x \r\n",event);
		} else {
			pkdrv_tsobj->getDoneBufferFromQueue(0);
			audio_msg("empty node with dma done txlb \r\n");
			set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TXLB_ISR_IDLE);
			return;
		}

        pkdrv_tsobj->getDoneBufferFromQueue(0);

        _kdrv_remove_node(KDRV_HANDLE_TXLB,pnode);
        pnode->cb_func.callback(&cb_info,&pnode->user_data);
        pnode->cb_func.reserve_buf(pnode->buffer_addr.addr_pa);
        pnode->cb_func.free_buf(pnode->buffer_addr.addr_pa);
        pnode->b_used = 0;

        if(list_empty(&gkdrv_list_head[KDRV_HANDLE_TXLB])){
                pkdrv_tsobj->stop();
                audio_msg("txlb stop \r\n");
            }
	}
	if (event & KDRV_ERRFLAG_RX) {
	//	audio_msg("TxLB ERROR! Flag=0x%08X\r\n", (event & KDRV_ERRFLAG_RX));
	}

	set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TXLB_ISR_IDLE);
}

void kdrv_audio_event_cb_tx1(UINT32 event)
{
    KDRV_AUDIO_LIST_NODE *pnode;
    PAUDTS_OBJ            pkdrv_tsobj;
    UINT32               cb_info;
	unsigned long 		 flag;

    cb_info = KDRV_HANDLE_TX1;

	clr_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX1_ISR_IDLE);

	if (!aud_info[KDRV_HANDLE_TX1].opened) {
		set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX1_ISR_IDLE);
		return;
	}

    pkdrv_tsobj = aud_info[KDRV_HANDLE_TX1].paud_drv_obj;

	if (event & AUDIO_EVENT_DMADONE) {

		if(!list_empty(&gkdrv_list_head[KDRV_HANDLE_TX1])){
			loc_cpu(flag);
		    pnode = list_entry(gkdrv_list_head[KDRV_HANDLE_TX1].next, KDRV_AUDIO_LIST_NODE, list);
			unl_cpu(flag);
		    //audio_msg("tx1 event status = %x \r\n",event);
		} else {
			pkdrv_tsobj->getDoneBufferFromQueue(0);
			audio_msg("empty node with dma done tx1 \r\n");
			set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX1_ISR_IDLE);
			return;
		}
        pkdrv_tsobj->getDoneBufferFromQueue(0);

        _kdrv_remove_node(KDRV_HANDLE_TX1,pnode);

        if(list_empty(&gkdrv_list_head[KDRV_HANDLE_TX1])){
                pkdrv_tsobj->stop();
                //audio_msg("playback stop \r\n");
            }

        pnode->cb_func.callback(&cb_info,&pnode->user_data);
        pnode->cb_func.reserve_buf(pnode->buffer_addr.addr_pa);
        pnode->cb_func.free_buf(pnode->buffer_addr.addr_pa);
        pnode->b_used = 0;

	}

	if (event & AUDIO_EVENT_TCHIT) {
	}

	if (event & AUDIO_EVENT_TCLATCH) {
	}

	if (event & KDRV_ERRFLAG_TX) {
	//	audio_msg("Tx ERROR! Flag=0x%08X\r\n", (event & KDRV_ERRFLAG_TX));
	}

	set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX1_ISR_IDLE);
}

void kdrv_audio_event_cb_tx2(UINT32 event)
{
    KDRV_AUDIO_LIST_NODE *pnode;
    PAUDTS_OBJ            pkdrv_tsobj;
	unsigned long 		 flag;

	clr_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX2_ISR_IDLE);

	if (!aud_info[KDRV_HANDLE_TX2].opened) {
		set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX2_ISR_IDLE);
		return;
	}


    pkdrv_tsobj = aud_info[KDRV_HANDLE_TX2].paud_drv_obj;


	if (event & AUDIO_EVENT_DMADONE) {
		if(!list_empty(&gkdrv_list_head[KDRV_HANDLE_TX2])){
			loc_cpu(flag);
		    pnode = list_entry(gkdrv_list_head[KDRV_HANDLE_TX2].next, KDRV_AUDIO_LIST_NODE, list);
			unl_cpu(flag);
		    //audio_msg("tx2 event status = %x \r\n",event);
		} else {
			pkdrv_tsobj->getDoneBufferFromQueue(0);
			audio_msg("empty node with dma done tx2 \r\n");
			set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX2_ISR_IDLE);
			return;
		}
        pkdrv_tsobj->getDoneBufferFromQueue(0);

        _kdrv_remove_node(KDRV_HANDLE_TX2,pnode);

        if(list_empty(&gkdrv_list_head[KDRV_HANDLE_TX2])){
                pkdrv_tsobj->stop();
                //audio_msg("playback2 stop \r\n");
            }

        pnode->cb_func.callback(&event,&pnode->user_data);
        pnode->cb_func.reserve_buf(pnode->buffer_addr.addr_pa);
        pnode->cb_func.free_buf(pnode->buffer_addr.addr_pa);
        pnode->b_used = 0;

	}

	if (event & KDRV_ERRFLAG_TX) {
	//	audio_msg("Tx2 ERROR! Flag=0x%08X\r\n", (event & KDRV_ERRFLAG_TX));
	}

	set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX2_ISR_IDLE);
}


static void *kdrv_audio_event_hanle[KDRV_HANDLE_MAX]= {kdrv_audio_event_cb_rx,kdrv_audio_event_cb_txlb,kdrv_audio_event_cb_tx1,kdrv_audio_event_cb_tx2};

#endif

#if 1 // kdrv main API

/*!
 * @fn INT32 kdrv_audioio_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware. Please use KDRV_AUDCAP_ENGINE0 / KDRV_AUDCAP_ENGINE1 / KDRV_AUDOUT_ENGINE0 / KDRV_AUDOUT_ENGINE1.
 * @return return 0 on success, -1 on error.
 */
UINT32 kdrv_audioio_open(UINT32 chip, UINT32 engine)
{
    UINT32       ret;
    UINT32       handler;
    PAUDTS_OBJ   pkdrv_tsobj;

	kdrv_audio_lock();

    if(gkdrv_audio_init == FALSE) {

        ret = aud_open();

        if(ret != E_OK) {
            audio_msg("audio open fail : %d \r\n",(int)ret);
			kdrv_audio_unlock();
            return -1;
        }
        aud_setting.SamplingRate = gkdrv_samplerate;
    	aud_set_device_object((PAUDIO_DEVICE_OBJ)&aud_device);
    	aud_init((PAUDIO_SETTING)&aud_setting);
		audcodec_get_fp(&gkdrv_codec_function);
		aud_set_extcodec(&gkdrv_codec_function);
        aud_set_total_vol_level(AUDIO_VOL_LEVEL64);
		dai_set_i2s_config(DAI_I2SCONFIG_ID_DATA_ORDER, DAI_I2S_DATAORDER_TYPE1);
        for(handler = 0;handler<KDRV_HANDLE_MAX;handler++)
            INIT_LIST_HEAD(&gkdrv_list_head[handler]);
        gkdrv_audio_init = TRUE;
    }
    switch (engine){
        case KDRV_AUDCAP_ENGINE0:{
                handler = KDRV_HANDLE_RX1;
                aud_info[handler].paud_drv_obj = aud_get_transceive_object(AUDTS_CH_RX);
				set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_RX_ISR_IDLE);
            }break;
        case KDRV_AUDCAP_ENGINE1:{
                handler = KDRV_HANDLE_TXLB;
                aud_info[handler].paud_drv_obj = aud_get_transceive_object(AUDTS_CH_TXLB);
				set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TXLB_ISR_IDLE);
            }break;
        case KDRV_AUDOUT_ENGINE0:{
                handler = KDRV_HANDLE_TX1;
                aud_info[handler].paud_drv_obj = aud_get_transceive_object(AUDTS_CH_TX1);
				set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX1_ISR_IDLE);
            }break;
        case KDRV_AUDOUT_ENGINE1:{
                handler = KDRV_HANDLE_TX2;
                aud_info[handler].paud_drv_obj = aud_get_transceive_object(AUDTS_CH_TX2);
				set_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX2_ISR_IDLE);
            }break;
        default:{
                audio_msg("audio engine Id 0x%x is not valid \r\n",(unsigned int)engine);
				kdrv_audio_unlock();
             return -1;
            }
    }

    if (aud_info[handler].opened == TRUE) {
        audio_msg("audio engine Id 0x%x is already opened \r\n",(unsigned int)engine);
		kdrv_audio_unlock();
        return -1;
    }

    pkdrv_tsobj = aud_info[handler].paud_drv_obj;

    ret = pkdrv_tsobj->open();

    if (ret != E_OK) {
        audio_msg("audio tsobj open fail : %d \r\n",(int)ret);
		kdrv_audio_unlock();
        return -1;
    }

    aud_info[handler].opened         = TRUE;
    aud_info[handler].sample_rate    = gkdrv_samplerate;
    aud_info[handler].volume         = 100;
    aud_info[handler].channel        = AUDIO_CH_STEREO;
    aud_info[handler].tdm_channel    = AUDIO_TDMCH_2CH;
    aud_info[handler].bit_per_sample = AUDIO_PCMLEN_16BITS;

#if defined (__KERNEL__)
	{
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (!(kdrv_builtin_is_fastboot() && kdrv_first_boot && builtin_init)) {
			if(handler == KDRV_HANDLE_RX1){
				aud_set_default_setting(gkdrv_defset);
			}

			pkdrv_tsobj->setChannel(aud_info[handler].channel);
			pkdrv_tsobj->setTdmChannel(aud_info[handler].tdm_channel);
			if (gkdrv_audio_init == FALSE) {
				pkdrv_tsobj->setSamplingRate(aud_info[handler].sample_rate);
			}
		    if(handler != KDRV_HANDLE_TXLB) {
		    	pkdrv_tsobj->setConfig(AUDTS_CFG_ID_PCM_BITLEN, aud_info[handler].bit_per_sample);
		    }
		}
	}
#else
	if(handler == KDRV_HANDLE_RX1){
		aud_set_default_setting(gkdrv_defset);
	}

	pkdrv_tsobj->setChannel(aud_info[handler].channel);
	pkdrv_tsobj->setTdmChannel(aud_info[handler].tdm_channel);
	if (gkdrv_audio_init == FALSE) {
		pkdrv_tsobj->setSamplingRate(aud_info[handler].sample_rate);
	}
    if(handler != KDRV_HANDLE_TXLB) {
    	pkdrv_tsobj->setConfig(AUDTS_CFG_ID_PCM_BITLEN, aud_info[handler].bit_per_sample);
    }
#endif

    pkdrv_tsobj->setConfig(AUDTS_CFG_ID_EVENT_HANDLE, (UINT32)kdrv_audio_event_hanle[handler]);
    pkdrv_tsobj->resetBufferQueue(0);

	_kdrv_reset_node_list(handler);

	kdrv_audio_unlock();

    return 0;
}

/*!
 * @fn INT32 kdrv_audioio_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware. Please use KDRV_AUDCAP_ENGINE0 / KDRV_AUDCAP_ENGINE1 / KDRV_AUDOUT_ENGINE0 / KDRV_AUDOUT_ENGINE1.
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_audioio_close(UINT32 chip, UINT32 engine)
{
    UINT32        handler;
    UINT32        ret=0;
    PAUDTS_OBJ    pkdrv_tsobj;
	FLGPTN		  uiFlag = 0;

	kdrv_audio_lock();

    switch (engine){
        case KDRV_AUDCAP_ENGINE0:{
                handler = KDRV_HANDLE_RX1;
            }break;
        case KDRV_AUDCAP_ENGINE1:{
                handler = KDRV_HANDLE_TXLB;
            }break;
        case KDRV_AUDOUT_ENGINE0:{
                handler = KDRV_HANDLE_TX1;
            }break;
        case KDRV_AUDOUT_ENGINE1:{
                handler = KDRV_HANDLE_TX2;
            }break;
        default:{
                audio_msg("audio engine Id 0x%x is not valid \r\n",(unsigned int)engine);
				kdrv_audio_unlock();
	            return 0;
            }
    }

    if (!aud_info[handler].opened){
        audio_msg("audio engine Id 0x%x is already closed \r\n",(unsigned int)engine);
		kdrv_audio_unlock();
        return 0;
    }
    pkdrv_tsobj = aud_info[handler].paud_drv_obj;



    if(pkdrv_tsobj->isBusy()){
        pkdrv_tsobj->stop();
    }

	aud_info[handler].opened = FALSE;

	// wait flag (wait ISR handle done)

	switch (engine){
		case KDRV_AUDCAP_ENGINE0:{
			wai_flg(&uiFlag, FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_RX_ISR_IDLE, TWF_ORW);
		}break;
		case KDRV_AUDCAP_ENGINE1:{
			wai_flg(&uiFlag, FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TXLB_ISR_IDLE, TWF_ORW);
		}break;
		case KDRV_AUDOUT_ENGINE0:{
			wai_flg(&uiFlag, FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX1_ISR_IDLE, TWF_ORW);
		}break;
		case KDRV_AUDOUT_ENGINE1:{
			wai_flg(&uiFlag, FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX2_ISR_IDLE, TWF_ORW);
		}break;
	}


    pkdrv_tsobj->close();


    if (!(aud_info[KDRV_HANDLE_RX1].opened || aud_info[KDRV_HANDLE_TXLB].opened ||aud_info[KDRV_HANDLE_TX1].opened ||aud_info[KDRV_HANDLE_TX2].opened)) {
        ret = aud_close();
        gkdrv_audio_init = FALSE;

        if (ret != E_OK){
            audio_msg("audio engine Id 0x%x close failed ,ret = %d \r\n",(unsigned int)engine,(int)ret);
			kdrv_audio_unlock();
            return -1;
        }
    }

	// clr flag
	switch (engine){
		case KDRV_AUDCAP_ENGINE0:{
			clr_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_RX_ISR_IDLE);
		}break;
		case KDRV_AUDCAP_ENGINE1:{
			clr_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TXLB_ISR_IDLE);
		}break;
		case KDRV_AUDOUT_ENGINE0:{
			clr_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX1_ISR_IDLE);
		}break;
		case KDRV_AUDOUT_ENGINE1:{
			clr_flg(FLG_ID_KDRV_AUD, KDRV_FLG_ID_AUD_STS_TX2_ISR_IDLE);
		}break;
	}

	kdrv_audio_unlock();


    return 0;

}

/*!
 * @fn INT32 kdrv_audioio_set(UINT32 handler, KDRV_AUDIOIO_PARAM_ID id, VOID *param)
 * @brief set parameters to hardware engine
 * @param id	    the handler of hardware
 * @param param_id 	the param_id of parameters
 * @param param 	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 __kdrv_audioio_set(UINT32 id, KDRV_AUDIOIO_PARAM_ID param_id, VOID *param)
{
    INT32 ret;
    PAUDTS_OBJ pkdrv_tsobj;
    UINT32 handler;

    handler = _kdrv_get_handler(id);

    if (!gkdrv_audio_init) {
		if(param_id != KDRV_AUDIOIO_CAP_PDVCMBIAS_ALWAYS_ON && param_id != KDRV_AUDIOIO_OUT_STOP_TX){
	        audio_msg("audio driver have not initialized \r\n");
	        audio_msg("Plz use kdrv_audioio_open to get the handler first. \r\n");
	        return -1;
		}
    }

    ret = _kdrv_chk_set_config(handler,param_id,param);

    if (ret == -1) {
        audio_msg("audio set config invalid error \r\n");
        return -1;
    }

    pkdrv_tsobj = aud_info[handler].paud_drv_obj;

    switch (param_id) {
        case KDRV_AUDIOIO_GLOBAL_SAMPLE_RATE: {
                gkdrv_samplerate = *(AUDIO_SR *)param;
                aud_info[handler].sample_rate    = gkdrv_samplerate;
                aud_set_codec_samplerate(&gkdrv_codec_function, gkdrv_samplerate);

            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_BIT_WIDTH : {
                if(*(UINT32 *)param == 16)
                    dai_set_i2s_config(DAI_I2SCONFIG_ID_CHANNEL_LEN,DAI_I2SCHLEN_16BITS);
                else
                    dai_set_i2s_config(DAI_I2SCONFIG_ID_CHANNEL_LEN,DAI_I2SCHLEN_32BITS);
            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_BITCLK_RATIO : {
                switch (*(UINT32 *)param) {
                        case 32 : dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_32BIT); break;
                        case 64 : dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_64BIT); break;
                        case 128 : dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_128BIT); break;
                        case 256 : dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_256BIT); break;
                        default:
                            break;
                    }
            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_OPMODE : {
                if (*(KDRV_AUDIO_I2S_OPMODE *)param == KDRV_AUDIO_I2S_OPMODE_MASTER)
                    dai_set_i2s_config(DAI_I2SCONFIG_ID_OPMODE, DAI_OP_MASTER);
                else
                    dai_set_i2s_config(DAI_I2SCONFIG_ID_OPMODE, DAI_OP_SLAVE);
            }break;

		case KDRV_AUDIOIO_GLOBAL_I2S_DATA_ORDER : {
				if(*(KDRV_AUDIO_I2S_DATA_ORDER *)param == KDRV_AUDIO_I2S_DATA_ORDER_TYPE1) {
					dai_set_i2s_config(DAI_I2SCONFIG_ID_DATA_ORDER, DAI_I2S_DATAORDER_TYPE1);
				} else if (*(KDRV_AUDIO_I2S_DATA_ORDER *)param == KDRV_AUDIO_I2S_DATA_ORDER_TYPE2) {
					dai_set_i2s_config(DAI_I2SCONFIG_ID_DATA_ORDER, DAI_I2S_DATAORDER_TYPE2);
				} else {
					dai_set_i2s_config(DAI_I2SCONFIG_ID_DATA_ORDER, *(UINT32 *)param);
				}

			}break;

        case KDRV_AUDIOIO_GLOBAL_CLOCK_ALWAYS_ON : {
                gkdrv_int_flag &= ~KDRV_AUDIO_INT_FLAG_CLKALWAYS_ON;
                aud_set_feature(AUDIO_FEATURE_INTERFACE_ALWAYS_ACTIVE, *(BOOL *)param);
                if(*(BOOL *) param)
                    gkdrv_int_flag |= KDRV_AUDIO_INT_FLAG_CLKALWAYS_ON;
            }break;

        case KDRV_AUDIOIO_GLOBAL_IS_BUSY : {
                audio_msg("this param_id[%d] is for kdrv_audioio_get() \r\n",(int)param_id);
                return -1;
            }break;

        case KDRV_AUDIOIO_CAP_PATH : {
                switch (*(KDRV_AUDIO_CAP_PATH *)param) {
                        case KDRV_AUDIO_CAP_PATH_AMIC : {
								if (gkdrv_i2s_tx == 0) {
									dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN, 0);
								}
								gkdrv_i2s_rx = 0;
								dai_set_config(DAI_CONFIG_ID_RX_SRC_SEL ,DAI_RX_SRC_EMBEDDED);
								aud_set_record_source(AUDIO_RECSRC_MIC);
								//gkdrv_codec_function.setRecordSource(AUDIO_RECSRC_MIC);
                            }break;
                        case KDRV_AUDIO_CAP_PATH_DMIC : {
								aud_set_record_source(AUDIO_RECSRC_DMIC);
								//gkdrv_codec_function.setRecordSource(AUDIO_RECSRC_DMIC);
								aud_setting.RecSrc = AUDIO_RECSRC_DMIC;
                                audio_msg("Used Digital-mic \r\n");
                            }break;
                        case KDRV_AUDIO_CAP_PATH_I2S: {
                                dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN, 1);
								dai_set_config(DAI_CONFIG_ID_RX_SRC_SEL ,DAI_RX_SRC_I2S);
								/*
								aud_ext_codec_emu_get_fp(&gkdrv_codec_function);
								aud_set_extcodec(&gkdrv_codec_function);*/
								gkdrv_i2s_rx = 1;
                            }break;
                        default:
                            break;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_DEFAULT_CFG : {
                aud_set_default_setting(*(AUDIO_DEFSET *) param);
            }break;

        case KDRV_AUDIOIO_OUT_CHANNEL_NUMBER :
        case KDRV_AUDIOIO_CAP_CHANNEL_NUMBER :{
                switch (*(UINT32 *)param) {
                        case 1 : pkdrv_tsobj->setChannel(AUDIO_CH_LEFT);break;
                        case 2 :
                                if( dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN))
                                    pkdrv_tsobj->setTdmChannel(AUDIO_TDMCH_2CH);
                                 pkdrv_tsobj->setChannel(AUDIO_CH_STEREO);
                                break;
                        case 4 : pkdrv_tsobj->setTdmChannel(AUDIO_TDMCH_4CH);break;
                        case 6 : pkdrv_tsobj->setTdmChannel(AUDIO_TDMCH_6CH);break;
                        case 8 : pkdrv_tsobj->setTdmChannel(AUDIO_TDMCH_8CH);break;
                        default:
                            break;
                    }

            }break;

		case KDRV_AUDIOIO_CAP_DMIC_CH : {
				switch (*(UINT32 *)param) {
						case 2: eac_set_ad_config(EAC_CONFIG_AD_DMIC_CHANNEL,0);break;
						case 4:	eac_set_ad_config(EAC_CONFIG_AD_DMIC_CHANNEL,1);break;
						default:
							break;
					}

			}break;

        case KDRV_AUDIOIO_CAP_MONO_SEL : {
                if(*(KDRV_AUDIO_CAP_MONO *)param == KDRV_AUDIO_CAP_MONO_LEFT)
                    pkdrv_tsobj->setChannel(AUDIO_CH_LEFT);
                else
                    pkdrv_tsobj->setChannel(AUDIO_CH_RIGHT);
            }break;

        case KDRV_AUDIOIO_OUT_MONO_EXPAND : {
                gkdrv_int_flag &= ~KDRV_AUDIO_INT_FLAG_TX_EXPAND;
                pkdrv_tsobj->setFeature(AUDTS_FEATURE_PLAYBACK_PCM_EXPAND,*(BOOL *)param);
                if(*(BOOL *) param)
                    gkdrv_int_flag |= KDRV_AUDIO_INT_FLAG_TX_EXPAND;
            }break;

        case KDRV_AUDIOIO_CAP_MONO_EXPAND : {
                gkdrv_int_flag &= ~KDRV_AUDIO_INT_FLAG_RX_EXPAND;
                pkdrv_tsobj->setFeature(AUDTS_FEATURE_RECORD_PCM_EXPAND,*(BOOL *)param);
                if(*(BOOL *) param)
                    gkdrv_int_flag |= KDRV_AUDIO_INT_FLAG_RX_EXPAND;
            }break;

        case KDRV_AUDIOIO_OUT_BIT_WIDTH :
        case KDRV_AUDIOIO_CAP_BIT_WIDTH : {
                switch (*(UINT32 *)param) {
                        case 8:  pkdrv_tsobj->setConfig(AUDTS_CFG_ID_PCM_BITLEN,AUDIO_PCMLEN_8BITS); break;
                        case 16: pkdrv_tsobj->setConfig(AUDTS_CFG_ID_PCM_BITLEN,AUDIO_PCMLEN_16BITS); break;
                        case 32: pkdrv_tsobj->setConfig(AUDTS_CFG_ID_PCM_BITLEN,AUDIO_PCMLEN_32BITS); break;
                        default:
                            break;
                    }

            }break;

        case KDRV_AUDIOIO_OUT_VOLUME :
        case KDRV_AUDIOIO_CAP_VOLUME : {
                _kdrv_set_volume(handler, *(UINT32 *)param);
                aud_info[handler].volume = *(UINT32 *)param;
            }break;

		case KDRV_AUDIOIO_CAP_VOLUME_LEFT:
		case KDRV_AUDIOIO_OUT_VOLUME_LEFT: {
				_kdrv_set_lr_volume(handler, *(UINT32 *)param, 0);
			}break;

		case KDRV_AUDIOIO_CAP_VOLUME_RIGHT:
		case KDRV_AUDIOIO_OUT_VOLUME_RIGHT: {
				_kdrv_set_lr_volume(handler, *(UINT32 *)param, 1);
			}break;

        case KDRV_AUDIOIO_CAP_DCCAN_RESOLUTION : {
                aud_set_parameter(AUDIO_PARAMETER_DCCAN_RESOLUTION, EAC_DCCAN_RESO_1024_SAMPLES+ (*(UINT32 *)param));
            }break;

        case KDRV_AUDIOIO_CAP_TRIGGER_DELAY :{
                aud_set_parameter(AUDIO_PARAMETER_RECORD_DELAY, *(UINT32 *)param);
                gkdrv_rx_trigger_delay = *(UINT32 *) param;
            }break;

        case KDRV_AUDIOIO_CAP_AMIC_BOOST :{
                aud_set_parameter(AUDIO_PARAMETER_BOOST_GAIN, (*(UINT32 *)param) / 10);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_EN : {
                aud_set_feature(AUDIO_FEATURE_ALC, *(BOOL *)param);
                _kdrv_set_volume(handler, aud_info[handler].volume); // set volume to make sure PGA gain at right setting after turn off ALC
            }break;

        case KDRV_AUDIOIO_CAP_ALC_MAXGAIN : {
                aud_set_parameter(AUDIO_PARAMETER_ALC_MAXGAIN, *(UINT32 *) param);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_MINGAIN : {
                aud_set_parameter(AUDIO_PARAMETER_ALC_MINGAIN, *(UINT32 *) param);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_ATTACK_TIME : {
                aud_set_parameter(AUDIO_PARAMETER_ALC_ATTACK_TIME, *(UINT32 *) param);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_DECAY_TIME : {
                aud_set_parameter(AUDIO_PARAMETER_ALC_DECAY_TIME, *(UINT32 *) param);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_HOLD_TIME : {
                aud_set_parameter(AUDIO_PARAMETER_ALC_HOLD_TIME, *(UINT32 *) param);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_TIME_RESOLUTION : {
                aud_set_parameter(AUDIO_PARAMETER_ALC_TIME_RESOLUTION,*(UINT32 *)param);
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_EN : {
                aud_set_feature(AUDIO_FEATURE_NOISEGATE_EN, *(BOOL *) param);
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_THRESHOLD: {
                if (eac_get_ad_config(EAC_CONFIG_AD_PGABOOST_L != 0 ))
                    aud_set_parameter(AUDIO_PARAMETER_NOISETHD_WITH_BOOST, (((*(INT32 *)param) + 77) * 2 / 3));
                else
                    aud_set_parameter(AUDIO_PARAMETER_NOISETHD_WITHOUT_BOOST, (((*(INT32 *)param)+ 77) * 2 / 3));
            }break;

        case KDRV_AUDIOIO_CAP_NG_BOOST_COMPENSATION : {
                aud_set_feature(AUDIO_FEATURE_NG_WITH_MICBOOST, *(BOOL *) param);
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGAIN : {
                aud_set_parameter(AUDIO_PARAMETER_NOISEGAIN,*(UINT32 *)param);
            }break;

        case KDRV_AUDIOIO_CAP_ALCNG_ATTACK_TIME : {
                aud_set_parameter(AUDIO_PARAMETER_ALCNG_ATTACK_TIME, *(UINT32 *) param);
            }break;

        case KDRV_AUDIOIO_CAP_ALCNG_DECAY_TIME : {
                aud_set_parameter(AUDIO_PARAMETER_ALCNG_DECAY_TIME, *(UINT32 *) param);
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_TIME_RESOLUTION : {
                aud_set_parameter(AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION,*(UINT32 *)param);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_IIR_EN : {
                aud_set_feature(AUDIO_FEATURE_ALC_IIR_EN,*(BOOL *)param);
            }break;

        case KDRV_AUDIOIO_CAP_OUTPUT_IIR_EN : {
                aud_set_feature(AUDIO_FEATURE_OUTPUT_IIR_EN,*(BOOL*)param);
            }break;

        case KDRV_AUDIOIO_CAP_IIRCOEF_L : {
                aud_set_parameter(AUDIO_PARAMETER_IIRCOEF_L, *(UINT32 *)&param);
            }break;

        case KDRV_AUDIOIO_CAP_IIRCOEF_R : {
                aud_set_parameter(AUDIO_PARAMETER_IIRCOEF_R, *(UINT32 *)&param);
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_HIT_EN : {
                gkdrv_int_flag &= ~KDRV_AUDIO_INT_FLAG_TX_TIMECODE_HIT;
                pkdrv_tsobj->setFeature(AUDTS_FEATURE_TIMECODE_HIT, *(BOOL *)param);
                if(*(BOOL *)param)
                    gkdrv_int_flag |= KDRV_AUDIO_INT_FLAG_TX_TIMECODE_HIT;
            }break;

        case KDRV_AUDIOIO_CAP_TIMECODE_HIT_EN : {
                gkdrv_int_flag &= ~KDRV_AUDIO_INT_FLAG_RX_TIMECODE_HIT;
                pkdrv_tsobj->setFeature(AUDTS_FEATURE_TIMECODE_HIT, *(BOOL *)param);
                if(*(BOOL *)param)
                    gkdrv_int_flag |= KDRV_AUDIO_INT_FLAG_RX_TIMECODE_HIT;
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_HIT_CB :
        case KDRV_AUDIOIO_CAP_TIMECODE_HIT_CB : {
                memcpy((void *)&gkdrv_audio_timecode_hit_cb[handler], (const void *)param, sizeof(KDRV_CALLBACK_FUNC));
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_TRIGGER :
        case KDRV_AUDIOIO_CAP_TIMECODE_TRIGGER : {
                if(*(AUDIO_SR *)param == gkdrv_samplerate)
                    pkdrv_tsobj->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, *(UINT32 *)param);
                else
                    audio_msg("audio timecode value needs to be the same as samplerate = %d \r\n",(int)gkdrv_samplerate);
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_OFFSET :
        case KDRV_AUDIOIO_CAP_TIMECODE_OFFSET : {
                pkdrv_tsobj->setConfig(AUDTS_CFG_ID_TIMECODE_OFFSET, *(UINT32 *)param);
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_VALUE :
        case KDRV_AUDIOIO_CAP_TIMECODE_VALUE : {
                audio_msg("this param_id[%d] is for kdrv_audioio_get() \r\n",(int)param_id);
                return -1;
            }break;

        case KDRV_AUDIOIO_OUT_PATH : {
                switch (*(KDRV_AUDIO_OUT_PATH *)param) {
						case KDRV_AUDIO_OUT_PATH_ALL : {
								if (gkdrv_i2s_rx == 0) {
									dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN, 0);
								}
                                aud_set_output(AUDIO_OUTPUT_ALL);
								gkdrv_i2s_tx = 0;
							}break;
                        case KDRV_AUDIO_OUT_PATH_NONE : {
								if (gkdrv_i2s_rx == 0) {
									dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN, 0);
								}
                                aud_set_output(AUDIO_OUTPUT_NONE);
								gkdrv_i2s_tx = 0;
                            }break;
                        case KDRV_AUDIO_OUT_PATH_SPEAKER : {
								if (gkdrv_i2s_rx == 0) {
									dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN, 0);
								}
                                aud_set_output(AUDIO_OUTPUT_SPK);
								gkdrv_i2s_tx = 0;
                            }break;
                        case KDRV_AUDIO_OUT_PATH_LINEOUT: {
								if (gkdrv_i2s_rx == 0) {
									dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN, 0);
								}
                                aud_set_output(AUDIO_OUTPUT_LINE);
								gkdrv_i2s_tx = 0;
                            }break;
                        case KDRV_AUDIO_OUT_PATH_I2S : {
                                dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN, 1);
                                aud_set_output(AUDIO_OUTPUT_SPK);
                                gkdrv_i2s_tx = 1;
                            }break;
                        case KDRV_AUDIO_OUT_PATH_I2S_ONLY : {
                                dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN, 1);
                                aud_set_output(AUDIO_OUTPUT_NONE);
                                gkdrv_i2s_tx = 1;
                            }break;
                        default:
                            break;
                    }
            }break;

        case KDRV_AUDIOIO_OUT_HDMI_PATH_EN : {
                aud_set_feature(AUDIO_FEATURE_DISCONNECT_HDMI, !(*(BOOL *)param));
            }break;

        case KDRV_AUDIOIO_OUT_MONO_SEL : {
                if(*(KDRV_AUDIO_OUT_MONO *)param == KDRV_AUDIO_OUT_MONO_LEFT)
                    pkdrv_tsobj->setChannel(AUDIO_CH_LEFT);
                else
                    pkdrv_tsobj->setChannel(AUDIO_CH_RIGHT);
            }break;

        case KDRV_AUDIOIO_OUT_SPK_PWR_ALWAYS_ON : {
                gkdrv_int_flag &= ~KDRV_AUDIO_INT_FLAG_SPKPWR_ALWAYSON;
                aud_set_feature(AUDIO_FEATURE_SPK_PWR_ALWAYSON,  *(BOOL *)param);
                if (*(BOOL *)param)
                    gkdrv_int_flag |= KDRV_AUDIO_INT_FLAG_SPKPWR_ALWAYSON;
            }break;

        case KDRV_AUDIOIO_OUT_LINE_PWR_ALWAYS_ON : {
                gkdrv_int_flag &= ~KDRV_AUDIO_INT_FLAG_LINEPWR_ALWAYSON;
                aud_set_feature(AUDIO_FEATURE_LINE_PWR_ALWAYSON,  *(BOOL *)param);
                if (*(BOOL *)param)
                    gkdrv_int_flag |= KDRV_AUDIO_INT_FLAG_LINEPWR_ALWAYSON;
            }break;

		case KDRV_AUDIOIO_CAP_PDVCMBIAS_ALWAYS_ON : {
                gkdrv_int_flag &= ~KDRV_AUDIO_INT_FLAG_ADVCMPWR_ALWAYSON;
                aud_set_feature(AUDIO_FEATURE_PDVCMBIAS_ALWAYSON,  *(BOOL *)param);
                if (*(BOOL *)param)
                    gkdrv_int_flag |= KDRV_AUDIO_INT_FLAG_ADVCMPWR_ALWAYSON;
			}break;


		case KDRV_AUDIOIO_OUT_STOP_TX : {
				if (*(BOOL *)param) {
					if (handler == KDRV_HANDLE_TX1) {
						pkdrv_tsobj = aud_get_transceive_object(AUDTS_CH_TX1);
					} else if (handler == KDRV_HANDLE_TX2) {
						pkdrv_tsobj = aud_get_transceive_object(AUDTS_CH_TX2);
					}
					if(pkdrv_tsobj->isBusy()) {
						pkdrv_tsobj->stop();
					}
				}
			}break;
		case KDRV_AUDIOIO_CAP_GAIN_LEVEL : {
                aud_set_total_record_gain_level(*(UINT32 *)param);
			}break;






        default:{
            audio_msg("audio set parameter Id 0x%x is not valid \r\n",(unsigned int)param_id);
            return -1;
            }
    }

    return 0;
}

/*!
 * @fn INT32 kdrv_audioio_get(UINT32 handler, KDRV_AUDIOIO_PARAM_ID param_id, VOID *param)
 * @brief set parameters to hardware engine
 * @param id	    the handler of hardware
 * @param param_id 	the param_id of parameters
 * @param param 	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 __kdrv_audioio_get(UINT32 id, KDRV_AUDIOIO_PARAM_ID param_id, VOID *param)
{
    INT32 ret;
    UINT32 temp = 0;
    UINT32 handler;

    handler = _kdrv_get_handler(id);


    if (!gkdrv_audio_init) {
        audio_msg("audio driver have not initialized \r\n");
        audio_msg("Plz use kdrv_audioio_open to get the handler first. \r\n");
        return -1;
    }

    ret = _kdrv_chk_get_config(handler,param_id);
    if (ret == -1) {
        audio_msg("audio invalid get parameter param_id [0x%x] with handler [%d] \r\n",(unsigned int)param_id,(int)handler);
        return -1;
    }


    switch (param_id) {
        case KDRV_AUDIOIO_GLOBAL_SAMPLE_RATE : {
                *(AUDIO_SR *)param = gkdrv_samplerate;
            }break;

        case KDRV_AUDIOIO_GLOBAL_IS_BUSY : {
                *(BOOL *)param = aud_is_busy();
            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_BIT_WIDTH : {
                temp = dai_get_i2s_config(DAI_I2SCONFIG_ID_CHANNEL_LEN);
                if(temp == DAI_I2SCHLEN_16BITS)
                    *(UINT32 *)param = 16;
                else
                    *(UINT32 *)param = 32;
            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_BITCLK_RATIO : {
                temp = dai_get_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO);
                switch (temp) {
                        case DAI_I2SCLKR_256FS_32BIT  : *(UINT32 *)param = 32; break;
                        case DAI_I2SCLKR_256FS_64BIT  : *(UINT32 *)param = 64; break;
                        case DAI_I2SCLKR_256FS_128BIT : *(UINT32 *)param = 128; break;
                        case DAI_I2SCLKR_256FS_256BIT : *(UINT32 *)param = 256; break;
                    }
            }break;

        case KDRV_AUDIOIO_GLOBAL_I2S_OPMODE : {
                temp = dai_get_i2s_config(DAI_I2SCONFIG_ID_OPMODE);
                if(temp == DAI_OP_SLAVE)
                    *(KDRV_AUDIO_I2S_OPMODE *)param = KDRV_AUDIO_I2S_OPMODE_SLAVE;
                else
                    *(KDRV_AUDIO_I2S_OPMODE *)param = KDRV_AUDIO_I2S_OPMODE_MASTER;
            }break;

		case KDRV_AUDIOIO_GLOBAL_I2S_DATA_ORDER : {
				*(UINT32 *)param = dai_get_i2s_config(DAI_I2SCONFIG_ID_DATA_ORDER);
			}break;

        case KDRV_AUDIOIO_GLOBAL_CLOCK_ALWAYS_ON : {
                if (gkdrv_int_flag & KDRV_AUDIO_INT_FLAG_CLKALWAYS_ON)
                    *(UINT32 *)param = 1;
                else
                    *(UINT32 *)param = 0;
            }break;
		case KDRV_AUDIOIO_GLOBAL_ISR_CB : {
                *(UINT32 *)param = (UINT32)aud_isr_handler;
            }break;

        case KDRV_AUDIOIO_CAP_PATH : {
                temp = dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN);
                if(temp == 0)
                    *(KDRV_AUDIO_CAP_PATH *)param = KDRV_AUDIO_CAP_PATH_AMIC;
                else
                    *(KDRV_AUDIO_CAP_PATH *)param = KDRV_AUDIO_CAP_PATH_I2S;
            }break;

        case KDRV_AUDIOIO_CAP_CHANNEL_NUMBER : {
                if( dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN)) {
                        if(handler == KDRV_HANDLE_RX1)
                            temp = dai_get_rx_config(DAI_RXCFG_ID_TOTAL_CH);
                        else if(handler == KDRV_HANDLE_TXLB)
                            temp = dai_get_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH);
                        switch (temp) {
                                case DAI_TOTCH_2CH: *(UINT32 *)param = 2; break;
                                case DAI_TOTCH_4CH: *(UINT32 *)param = 4; break;
                                case DAI_TOTCH_6CH: *(UINT32 *)param = 6; break;
                                case DAI_TOTCH_8CH: *(UINT32 *)param = 8; break;
                            }
                    } else {
                        if(handler == KDRV_HANDLE_RX1)
                            temp = dai_get_rx_config(DAI_RXCFG_ID_CHANNEL);
                        else if(handler == KDRV_HANDLE_TXLB)
                            temp = dai_get_txlb_config(DAI_TXLBCFG_ID_CHANNEL);
                        if (temp != DAI_CH_STEREO)
                            *(UINT32 *)param = 1;
                        else
                            *(UINT32 *)param = 2;
                    }
            }break;

		case KDRV_AUDIOIO_CAP_DMIC_CH : {
				temp = eac_get_ad_config(EAC_CONFIG_AD_DMIC_CHANNEL);
				if(temp == 0) {
					*(UINT32 *)param = 2;
				} else if(temp ==1){
					*(UINT32 *)param = 4;
				}
			}break;

        case KDRV_AUDIOIO_CAP_MONO_SEL: {
                if(handler == KDRV_HANDLE_RX1)
                    temp = dai_get_rx_config(DAI_RXCFG_ID_CHANNEL);
                else if (handler == KDRV_HANDLE_TXLB)
                    temp = dai_get_txlb_config(DAI_TXLBCFG_ID_CHANNEL);
                if(temp == DAI_CH_MONO_LEFT)
                    *(KDRV_AUDIO_CAP_MONO *)param = KDRV_AUDIO_CAP_MONO_LEFT;
                else if (temp == DAI_CH_MONO_RIGHT)
                    *(KDRV_AUDIO_CAP_MONO *)param = KDRV_AUDIO_CAP_MONO_RIGHT;
            }break;

        case KDRV_AUDIOIO_CAP_MONO_EXPAND : {
                if (gkdrv_int_flag & KDRV_AUDIO_INT_FLAG_RX_EXPAND)
                    *(UINT32 *)param = 1;
                else
                    *(UINT32 *)param = 0;
            }break;

        case KDRV_AUDIOIO_CAP_BIT_WIDTH : {
                if(handler == KDRV_HANDLE_RX1)
                    temp = dai_get_rx_config(DAI_RXCFG_ID_PCMLEN);
                else if (handler == KDRV_HANDLE_TXLB)
                    temp = dai_get_txlb_config(DAI_TXLBCFG_ID_PCMLEN);
                switch(temp) {
                        case AUDIO_PCMLEN_8BITS: *(UINT32 *)param  =  8; break;
                        case AUDIO_PCMLEN_16BITS: *(UINT32 *)param = 16; break;
                        case AUDIO_PCMLEN_32BITS: *(UINT32 *)param = 32; break;
                    }
            }break;

        case KDRV_AUDIOIO_OUT_VOLUME :
        case KDRV_AUDIOIO_CAP_VOLUME : {
                *(UINT32 *)param = aud_info[handler].volume;
            }break;

        case KDRV_AUDIOIO_CAP_DCCAN_RESOLUTION : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_DCCAN_RESO);
            }break;

        case KDRV_AUDIOIO_CAP_TRIGGER_DELAY : {
                *(UINT32 *)param = gkdrv_rx_trigger_delay;
            }break;

        case KDRV_AUDIOIO_CAP_AMIC_BOOST : {
                temp = eac_get_ad_config(EAC_CONFIG_AD_PGABOOST_L);
                switch (temp) {
                        case EAC_PGABOOST_SEL_0DB:  *(UINT32 *)param =  0; break;
                        case EAC_PGABOOST_SEL_10DB: *(UINT32 *)param = 10; break;
                        case EAC_PGABOOST_SEL_20DB: *(UINT32 *)param = 20; break;
                        case EAC_PGABOOST_SEL_30DB: *(UINT32 *)param = 30; break;
                    }
            }break;

        case KDRV_AUDIOIO_CAP_ALC_EN : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_EN);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_MAXGAIN : {
                *(INT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_L);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_MINGAIN : {
                *(INT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_L);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_ATTACK_TIME : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_ATTACK_TIME);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_DECAY_TIME : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_DECAY_TIME);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_HOLD_TIME : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_HOLD_TIME);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_TIME_RESOLUTION : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_HOLD_TIME);
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_EN: {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_EN);
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_THRESHOLD : {
                *(INT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_THD_L);
                *(INT32 *)param = (*(INT32 *)param *3/2)-77;
                if(*(INT32 *)param < -77)
                    *(INT32 *)param = -77;
                else if (*(INT32 *)param > -30)
                    *(INT32 *)param = -30;
            }break;

        case KDRV_AUDIOIO_CAP_NG_BOOST_COMPENSATION : {
                //*(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_BOOST_COMPENSATE);
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGAIN : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_TARGET_L);
            }break;

        case KDRV_AUDIOIO_CAP_ALCNG_ATTACK_TIME : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_NG_ATTACK_TIME);
            }break;

        case KDRV_AUDIOIO_CAP_ALCNG_DECAY_TIME : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_NG_DECAY_TIME);
            }break;

        case KDRV_AUDIOIO_CAP_NOISEGATE_TIME_RESOLUTION : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_ALC_NG_TRESO);
            }break;

        case KDRV_AUDIOIO_CAP_ALC_IIR_EN : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_IIR_ALC_L);
            }break;

        case KDRV_AUDIOIO_CAP_OUTPUT_IIR_EN : {
                *(UINT32 *)param = eac_get_ad_config(EAC_CONFIG_AD_IIR_OUT_L);
            }break;

        case KDRV_AUDIOIO_CAP_TIMECODE_HIT_EN : {
                if (gkdrv_int_flag & KDRV_AUDIO_INT_FLAG_RX_TIMECODE_HIT)
                    *(UINT32 *)param = 1;
                else
                    *(UINT32 *)param = 0;
            }break;

        case KDRV_AUDIOIO_CAP_TIMECODE_TRIGGER : {
                if(handler == KDRV_HANDLE_RX1)
                    *(UINT32 *)param = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_TRIG);
                else if (handler == KDRV_HANDLE_TXLB)
                    audio_msg("Unsupported parameter param_id [%d] with handler [%d] \r\n",(int)param_id,(int)handler);
            }break;

        case KDRV_AUDIOIO_CAP_TIMECODE_OFFSET : {
                if(handler == KDRV_HANDLE_RX1)
                    *(UINT32 *)param = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_OFS);
                else if (handler == KDRV_HANDLE_TXLB)
                    audio_msg("Unsupported parameter param_id [%d] with handler [%d] \r\n",(int)param_id,(int)handler);
            }break;

        case KDRV_AUDIOIO_CAP_TIMECODE_VALUE : {
                if(handler == KDRV_HANDLE_RX1)
                    *(UINT32 *)param = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_VAL);
                else if (handler == KDRV_HANDLE_TXLB)
                    audio_msg("Unsupported parameter param_id [%d] with handler [%d] \r\n",(int)param_id,(int)handler);
            }break;

		case KDRV_AUDIOIO_CAP_GAIN_LEVEL : {
                *(UINT32 *)param = aud_get_Total_gain_level();
			}break;

        case KDRV_AUDIOIO_OUT_PATH : {
                temp = dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN);
                if(temp == 1)
                    *(KDRV_AUDIO_OUT_PATH *)param = KDRV_AUDIO_OUT_PATH_I2S;
				else if (eac_get_dac_output(EAC_OUTPUT_SPK)&& eac_get_dac_output(EAC_OUTPUT_LINE))
					*(KDRV_AUDIO_OUT_PATH *)param = KDRV_AUDIO_OUT_PATH_ALL;
                else if (eac_get_dac_output(EAC_OUTPUT_SPK))
                    *(KDRV_AUDIO_OUT_PATH *)param = KDRV_AUDIO_OUT_PATH_SPEAKER;
                else if (eac_get_dac_output(EAC_OUTPUT_LINE))
                    *(KDRV_AUDIO_OUT_PATH *)param = KDRV_AUDIO_OUT_PATH_LINEOUT;
                else
                    *(KDRV_AUDIO_OUT_PATH *)param = KDRV_AUDIO_OUT_PATH_NONE;
            }break;

        case KDRV_AUDIOIO_OUT_HDMI_PATH_EN : {
                *(UINT32 *)param = dai_get_config(DAI_CONFIG_ID_HDMI_TXEN);
            }break;

        case KDRV_AUDIOIO_OUT_CHANNEL_NUMBER : {
                if( dai_get_config(DAI_CONFIG_ID_EXTCODEC_EN)) {
                        if(handler == KDRV_HANDLE_TX1)
                            temp = dai_get_tx_config(DAI_TXCH_TX1,DAI_RXCFG_ID_TOTAL_CH);
                        else if (handler == KDRV_HANDLE_TX2)
                            temp = dai_get_tx_config(DAI_TXCH_TX2,DAI_RXCFG_ID_TOTAL_CH);
                        switch (temp) {
                                case DAI_TOTCH_2CH: *(UINT32 *)param = 2; break;
                                case DAI_TOTCH_4CH: *(UINT32 *)param = 4; break;
                                case DAI_TOTCH_6CH: *(UINT32 *)param = 6; break;
                                case DAI_TOTCH_8CH: *(UINT32 *)param = 8; break;
                            }
                    } else {
                        if(handler == KDRV_HANDLE_TX1)
                            temp = dai_get_tx_config(DAI_TXCH_TX1,DAI_RXCFG_ID_CHANNEL);
                        else if (handler == KDRV_HANDLE_TX2)
                            temp = dai_get_tx_config(DAI_TXCH_TX2,DAI_RXCFG_ID_CHANNEL);
                        if (temp != DAI_CH_STEREO)
                            *(UINT32 *)param = 1;
                        else
                            *(UINT32 *)param = 2;
                    }
            }break;

        case KDRV_AUDIOIO_OUT_MONO_SEL : {
                if(handler == KDRV_HANDLE_TX1)
                    temp = dai_get_tx_config(DAI_TXCH_TX1,DAI_TXCFG_ID_CHANNEL);
                else if (handler == KDRV_HANDLE_TX2)
                    temp = dai_get_tx_config(DAI_TXCH_TX2,DAI_TXCFG_ID_CHANNEL);
                if(temp == DAI_CH_MONO_LEFT)
                    *(KDRV_AUDIO_CAP_MONO *)param = KDRV_AUDIO_CAP_MONO_LEFT;
                else if (temp == DAI_CH_MONO_RIGHT)
                    *(KDRV_AUDIO_CAP_MONO *)param = KDRV_AUDIO_CAP_MONO_RIGHT;
            }break;

        case KDRV_AUDIOIO_OUT_MONO_EXPAND : {
                if (gkdrv_int_flag & KDRV_AUDIO_INT_FLAG_TX_EXPAND)
                    *(UINT32 *)param = 1;
                else
                    *(UINT32 *)param = 0;
            }break;

        case KDRV_AUDIOIO_OUT_BIT_WIDTH : {
                if(handler == KDRV_HANDLE_TX1)
                    temp = dai_get_tx_config(DAI_TXCH_TX1,DAI_TXCFG_ID_PCMLEN);
                else if (handler == KDRV_HANDLE_TX2)
                    temp = dai_get_tx_config(DAI_TXCH_TX2,DAI_TXCFG_ID_PCMLEN);
                switch(temp) {
                        case AUDIO_PCMLEN_8BITS: *(UINT32 *)param  =  8; break;
                        case AUDIO_PCMLEN_16BITS: *(UINT32 *)param = 16; break;
                        case AUDIO_PCMLEN_32BITS: *(UINT32 *)param = 32; break;
                    }
            }break;

        case KDRV_AUDIOIO_OUT_SPK_PWR_ALWAYS_ON : {
                if (gkdrv_int_flag & KDRV_AUDIO_INT_FLAG_SPKPWR_ALWAYSON)
                    *(UINT32 *)param = 1;
                else
                    *(UINT32 *)param = 0;
            }break;

        case KDRV_AUDIOIO_OUT_LINE_PWR_ALWAYS_ON : {
                if (gkdrv_int_flag & KDRV_AUDIO_INT_FLAG_LINEPWR_ALWAYSON)
                    *(UINT32 *)param = 1;
                else
                    *(UINT32 *)param = 0;
            }break;

        case KDRV_AUDIOIO_CAP_PDVCMBIAS_ALWAYS_ON : {
                if (gkdrv_int_flag & KDRV_AUDIO_INT_FLAG_ADVCMPWR_ALWAYSON)
                    *(UINT32 *)param = 1;
                else
                    *(UINT32 *)param = 0;
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_HIT_EN : {
                if (gkdrv_int_flag & KDRV_AUDIO_INT_FLAG_TX_TIMECODE_HIT)
                    *(UINT32 *)param = 1;
                else
                    *(UINT32 *)param = 0;
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_TRIGGER : {
                if(handler == KDRV_HANDLE_TX1)
                    *(UINT32 *)param = dai_get_tx_config(DAI_TXCH_TX1,DAI_TXCFG_ID_TIMECODE_TRIG);
                else if (handler == KDRV_HANDLE_TX2)
                    audio_msg("Unsupported parameter param_id [%d] with handler [%d] \r\n",(int)param_id,(int)handler);
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_OFFSET: {
                if(handler == KDRV_HANDLE_TX1)
                    *(UINT32 *)param = dai_get_tx_config(DAI_TXCH_TX1,DAI_TXCFG_ID_TIMECODE_OFS);
                else if (handler == KDRV_HANDLE_TX2)
                    audio_msg("Unsupported parameter param_id [%d] with handler [%d] \r\n",(int)param_id,(int)handler);
            }break;

        case KDRV_AUDIOIO_OUT_TIMECODE_VALUE: {
                if(handler == KDRV_HANDLE_TX1)
                    *(UINT32 *)param = dai_get_tx_config(DAI_TXCH_TX1,DAI_TXCFG_ID_TIMECODE_VAL);
                else if (handler == KDRV_HANDLE_TX2)
                    audio_msg("Unsupported parameter param_id [%d] with handler [%d] \r\n",(int)param_id,(int)handler);
            }break;


        default:{
            audio_msg("audio get parameter Id 0x%x is not valid \r\n",(unsigned int)param_id);
            return -1;
            }
    }

    return 0;
}





/*!
 * @fn INT32 kdrv_audioio_trigger(UINT32 handler,
							KDRV_BUFFER_INFO *p_au_frame_buffer,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *user_data);
 * @brief trigger hardware engine
 * @param id     				the handler of hardware
 * @param p_in_au_frame_buffer	the input audio frame buffer
 * @param p_cb_func 			the callback function
 * @param user_data 			the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_audioio_trigger(UINT32 id,
							KDRV_BUFFER_INFO *p_au_frame_buffer,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *user_data)
{
    INT32                  ret;
    PAUDTS_OBJ             pkdrv_tsobj;
    UINT32 handler;

    handler = _kdrv_get_handler(id);


    if (!gkdrv_audio_init) {
        audio_msg("audio driver have not initialized \r\n");
        audio_msg("Plz use kdrv_audioio_open to get the handler first. \r\n");
        return -1;
    }

    if(!aud_info[handler].opened) {
        audio_msg("engine havn't opened yet. [%d] \r\n",(int)handler);
        return -1;
    }

    pkdrv_tsobj = aud_info[handler].paud_drv_obj;

	if(p_au_frame_buffer == NULL){
	//audio_msg("audio driver buffer queue full, insert buffer failed \r\n");

		if(pkdrv_tsobj->isBusy()== 0) {
			if(handler == KDRV_HANDLE_RX1 || handler == KDRV_HANDLE_TXLB){
				pkdrv_tsobj->record();
				//audio_msg("record start \r\n");
			} else {
				pkdrv_tsobj->playback();
				//audio_msg("playback start 2\r\n");
			}

		}
		return -1;
	}

    ret = _kdrv_get_valid_list_node(handler);

    if(ret == -1){
        //audio_msg("audio driver buffer queue full, insert buffer failed \r\n");
        return -1;
    }


    //memcpy((void *)&(gkdrv_list_node[0].cb_func), (const void *)p_cb_func, sizeof(KDRV_CALLBACK_FUNC));
    gpkdrv_list_node[handler]->cb_func     = *(KDRV_CALLBACK_FUNC *)p_cb_func;
    gpkdrv_list_node[handler]->buffer_addr = *(KDRV_BUFFER_INFO *)p_au_frame_buffer;
    gpkdrv_list_node[handler]->user_data   = *(UINT32 *)user_data;




    _kdrv_add_node(handler,gpkdrv_list_node[handler]);


    gpkdrv_aud_buf_queue[handler] = &gkdrv_aud_buf_queue[handler];
    gpkdrv_aud_buf_queue[handler]->uiAddress = gpkdrv_list_node[handler]->buffer_addr.addr_pa;
    gpkdrv_aud_buf_queue[handler]->uiSize    = gpkdrv_list_node[handler]->buffer_addr.size;




    if (pkdrv_tsobj->isBufferQueueFull(0) == FALSE){

        pkdrv_tsobj->addBufferToQueue(0,gpkdrv_aud_buf_queue[handler]);
        //audio_msg("buf addr = 0x%x \r\n",gpkdrv_aud_buf_queue[handler]->uiAddress);

    } else {
    	_kdrv_remove_node(handler,  gpkdrv_list_node[handler]);
		gpkdrv_list_node[handler]->b_used = 0;
		return -1;
    }

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && kdrv_first_boot && (handler == KDRV_HANDLE_RX1 || handler == KDRV_HANDLE_TXLB)) {
		UINT32 builtin_init = 0;
		UINT32 param[5];
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &param[0]);

		builtin_init = param[0];

		if (builtin_init) {
	         pkdrv_tsobj->record();
	        //audio_msg("record start \r\n");

			if (handler == KDRV_HANDLE_RX1) {
				kdrv_first_boot = FALSE;
			}
			return 0;
		} else {
			kdrv_first_boot = FALSE;
		}
	}
#endif

    if(pkdrv_tsobj->isBusy()== 0) {
        if(handler == KDRV_HANDLE_RX1 || handler == KDRV_HANDLE_TXLB){
                pkdrv_tsobj->record();
                //audio_msg("record start \r\n");
            } else {
                pkdrv_tsobj->playback();
                //audio_msg("playback start \r\n");
            }

    }


    return 0;
}

/*!
 * @fn INT32 kdrv_audioio_trigger_not_start(UINT32 handler,
							KDRV_BUFFER_INFO *p_au_frame_buffer,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *user_data);
 * @brief trigger hardware engine but only insert buffer and won't start engine
 * @param id     				the handler of hardware
 * @param p_in_au_frame_buffer	the input audio frame buffer
 * @param p_cb_func 			the callback function
 * @param user_data 			the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_audioio_trigger_not_start(UINT32 id,
							KDRV_BUFFER_INFO *p_au_frame_buffer,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *user_data)
{
    INT32                  ret;
    PAUDTS_OBJ             pkdrv_tsobj;
    UINT32 handler;

    handler = _kdrv_get_handler(id);


    if (!gkdrv_audio_init) {
        audio_msg("audio driver have not initialized \r\n");
        audio_msg("Plz use kdrv_audioio_open to get the handler first. \r\n");
        return -1;
    }

    if(!aud_info[handler].opened) {
        audio_msg("engine havn't opened yet. [%d] \r\n",handler);
        return -1;
    }

    pkdrv_tsobj = aud_info[handler].paud_drv_obj;

    ret = _kdrv_get_valid_list_node(handler);

    if(ret == -1){
        //audio_msg("audio driver buffer queue full, insert buffer failed \r\n");
        return -1;
    }


    //memcpy((void *)&(gkdrv_list_node[0].cb_func), (const void *)p_cb_func, sizeof(KDRV_CALLBACK_FUNC));
    gpkdrv_list_node[handler]->cb_func     = *(KDRV_CALLBACK_FUNC *)p_cb_func;
    gpkdrv_list_node[handler]->buffer_addr = *(KDRV_BUFFER_INFO *)p_au_frame_buffer;
    gpkdrv_list_node[handler]->user_data   = *(UINT32 *)user_data;




    _kdrv_add_node(handler,gpkdrv_list_node[handler]);


    gpkdrv_aud_buf_queue[handler] = &gkdrv_aud_buf_queue[handler];
    gpkdrv_aud_buf_queue[handler]->uiAddress = gpkdrv_list_node[handler]->buffer_addr.addr_pa;
    gpkdrv_aud_buf_queue[handler]->uiSize    = gpkdrv_list_node[handler]->buffer_addr.size;




    if (pkdrv_tsobj->isBufferQueueFull(0) == FALSE){

        pkdrv_tsobj->addBufferToQueue(0,gpkdrv_aud_buf_queue[handler]);
        //audio_msg("buf addr = 0x%x \r\n",gpkdrv_aud_buf_queue[handler]->uiAddress);

    } else {
    	_kdrv_remove_node(handler,  gpkdrv_list_node[handler]);
		gpkdrv_list_node[handler]->b_used = 0;
		return -1;
    }
    return 0;
}


INT32 kdrv_audioio_reg_audiolib(KDRV_AUDIOLIB_ID id, KDRV_AUDIOLIB_FUNC *p_func)
{
	if (id >= KDRV_AUDIOLIB_ID_MAX) {
		audio_msg("Invalid id = %d\r\n",id);
		return -1;
	}
	audlib_func[id] = p_func;

	return 0;
}

KDRV_AUDIOLIB_FUNC* kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID id)
{
	if (id >= KDRV_AUDIOLIB_ID_MAX) {
		audio_msg("Invalid id = %d\r\n",id);
		return 0;
	}

	return audlib_func[id];
}

#endif

