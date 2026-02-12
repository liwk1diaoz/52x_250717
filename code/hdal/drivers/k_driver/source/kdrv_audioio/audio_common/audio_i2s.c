/*
    Audio I2S driver

    This file is the driver of audio I2S.

    @file       audio_i2s.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifdef __KERNEL__
#include "kwrap/type.h"//a header for basic variable type
#include <mach/rcw_macro.h>
#elif defined(__FREERTOS)
#include "kwrap/type.h"//a header for basic variable type
#include "rcw_macro.h"
#endif
#include "audio_int.h"
/**
    @addtogroup mISYSAud
*/
//@{

#if 1
static AUDIO_DEFAUUT_SETTING AudDefSettings[6] = {
// AUDIO_DEFSET_10DB:
	{
		ENABLE,                     // AUDIO_FEATURE_NG_WITH_MICBOOST, Boost Compendation
		ENABLE,                     // AUDIO_FEATURE_NOISEGATE_EN

		EAC_PGABOOST_SEL_10DB,      // AUDIO_PARAMETER_BOOST_GAIN
		EAC_ALC_MAXGAIN_P25P5_DB,   // AUDIO_PARAMETER_ALC_MAXGAIN
		EAC_ALC_MINGAIN_N15P0_DB,   // AUDIO_PARAMETER_ALC_MINGAIN
		0x2,                        // AUDIO_PARAMETER_ALC_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALC_DECAY_TIME
		0x0,                        // AUDIO_PARAMETER_ALC_HOLD_TIME
		EAC_ALC_TRESO_BASIS_15000US, // AUDIO_PARAMETER_ALC_TIME_RESOLUTION
		EAC_ALC_TRESO_BASIS_45000US,// AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION
		EAC_NG_THRESHOLD_N67P5_DB,  // AUDIO_PARAMETER_NOISETHD_WITH_BOOST
		0x2,                        // AUDIO_PARAMETER_NOISEGAIN
		0x4,                        // AUDIO_PARAMETER_ALCNG_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALCNG_DECAY_TIME
	},

// AUDIO_DEFSET_20DB:
	{
		ENABLE,                     // AUDIO_FEATURE_NG_WITH_MICBOOST, Boost Compendation
		ENABLE,                     // AUDIO_FEATURE_NOISEGATE_EN

		EAC_PGABOOST_SEL_20DB,      // AUDIO_PARAMETER_BOOST_GAIN
		EAC_ALC_MAXGAIN_P21P0_DB,   // AUDIO_PARAMETER_ALC_MAXGAIN
		EAC_ALC_MINGAIN_N16P5_DB,   // AUDIO_PARAMETER_ALC_MINGAIN
		0x2,                        // AUDIO_PARAMETER_ALC_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALC_DECAY_TIME
		0x0,                        // AUDIO_PARAMETER_ALC_HOLD_TIME
		EAC_ALC_TRESO_BASIS_15000US, // AUDIO_PARAMETER_ALC_TIME_RESOLUTION
		EAC_ALC_TRESO_BASIS_45000US,// AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION
		EAC_NG_THRESHOLD_N58P5_DB,  // AUDIO_PARAMETER_NOISETHD_WITH_BOOST
		0x1,                        // AUDIO_PARAMETER_NOISEGAIN
		0x4,                        // AUDIO_PARAMETER_ALCNG_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALCNG_DECAY_TIME
	},

// AUDIO_DEFSET_30DB:
	{
		ENABLE,                    // AUDIO_FEATURE_NG_WITH_MICBOOST, Boost Compendation
		ENABLE,                     // AUDIO_FEATURE_NOISEGATE_EN

		EAC_PGABOOST_SEL_30DB,      // AUDIO_PARAMETER_BOOST_GAIN
		EAC_ALC_MAXGAIN_P25P5_DB,   // AUDIO_PARAMETER_ALC_MAXGAIN
		EAC_ALC_MINGAIN_N18P0_DB,   // AUDIO_PARAMETER_ALC_MINGAIN
		0x2,                        // AUDIO_PARAMETER_ALC_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALC_DECAY_TIME
		0x0,                        // AUDIO_PARAMETER_ALC_HOLD_TIME
		EAC_ALC_TRESO_BASIS_15000US, // AUDIO_PARAMETER_ALC_TIME_RESOLUTION
		EAC_ALC_TRESO_BASIS_45000US,// AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION
		EAC_NG_THRESHOLD_N40P5_DB,  // AUDIO_PARAMETER_NOISETHD_WITH_BOOST
		0x1,                        // AUDIO_PARAMETER_NOISEGAIN
		0x6,                        // AUDIO_PARAMETER_ALCNG_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALCNG_DECAY_TIME
	},

// AUDIO_DEFSET_0DB:
	{
		DISABLE,                     // AUDIO_FEATURE_NG_WITH_MICBOOST, Boost Compendation
		ENABLE,                     // AUDIO_FEATURE_NOISEGATE_EN

		EAC_PGABOOST_SEL_0DB,       // AUDIO_PARAMETER_BOOST_GAIN
		EAC_ALC_MAXGAIN_P25P5_DB,   // AUDIO_PARAMETER_ALC_MAXGAIN
		EAC_ALC_MINGAIN_N18P0_DB,   // AUDIO_PARAMETER_ALC_MINGAIN
		0x2,                        // AUDIO_PARAMETER_ALC_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALC_DECAY_TIME
		0x0,                        // AUDIO_PARAMETER_ALC_HOLD_TIME
		EAC_ALC_TRESO_BASIS_15000US,// AUDIO_PARAMETER_ALC_TIME_RESOLUTION
		EAC_ALC_TRESO_BASIS_45000US,// AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION
		EAC_NG_THRESHOLD_N30P0_DB,  // AUDIO_PARAMETER_NOISETHD_WITH_BOOST
		0x2,                        // AUDIO_PARAMETER_NOISEGAIN
		0x4,                        // AUDIO_PARAMETER_ALCNG_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALCNG_DECAY_TIME
	},

// AUDIO_DEFSET_ALCOFF:
	{
		ENABLE,                    // AUDIO_FEATURE_NG_WITH_MICBOOST, Boost Compendation
		ENABLE,                     // AUDIO_FEATURE_NOISEGATE_EN

		EAC_PGABOOST_SEL_0DB,       // AUDIO_PARAMETER_BOOST_GAIN
		EAC_ALC_MAXGAIN_P25P5_DB,   // AUDIO_PARAMETER_ALC_MAXGAIN
		EAC_ALC_MINGAIN_N18P0_DB,   // AUDIO_PARAMETER_ALC_MINGAIN
		0x2,                        // AUDIO_PARAMETER_ALC_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALC_DECAY_TIME
		0x0,                        // AUDIO_PARAMETER_ALC_HOLD_TIME
		EAC_ALC_TRESO_BASIS_15000US, // AUDIO_PARAMETER_ALC_TIME_RESOLUTION
		EAC_ALC_TRESO_BASIS_45000US,// AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION
		EAC_NG_THRESHOLD_N30P0_DB,  // AUDIO_PARAMETER_NOISETHD_WITH_BOOST
		0x2,                        // AUDIO_PARAMETER_NOISEGAIN
		0x4,                        // AUDIO_PARAMETER_ALCNG_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALCNG_DECAY_TIME
	},

// AUDIO_DEFSET_DMIC:
	{
		ENABLE,                     // AUDIO_FEATURE_NG_WITH_MICBOOST, Boost Compendation
		ENABLE,                     // AUDIO_FEATURE_NOISEGATE_EN

		EAC_PGABOOST_SEL_20DB,      // AUDIO_PARAMETER_BOOST_GAIN
		EAC_ALC_MAXGAIN_P21P0_DB,   // AUDIO_PARAMETER_ALC_MAXGAIN
		EAC_ALC_MINGAIN_N16P5_DB,   // AUDIO_PARAMETER_ALC_MINGAIN
		0x2,                        // AUDIO_PARAMETER_ALC_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALC_DECAY_TIME
		0x0,                        // AUDIO_PARAMETER_ALC_HOLD_TIME
		EAC_ALC_TRESO_BASIS_15000US, // AUDIO_PARAMETER_ALC_TIME_RESOLUTION
		EAC_ALC_TRESO_BASIS_45000US,// AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION
		EAC_NG_THRESHOLD_N58P5_DB,  // AUDIO_PARAMETER_NOISETHD_WITH_BOOST
		0x1,                        // AUDIO_PARAMETER_NOISEGAIN
		0x4,                        // AUDIO_PARAMETER_ALCNG_ATTACK_TIME
		0x3,                        // AUDIO_PARAMETER_ALCNG_DECAY_TIME
	},		

};


/**
    Set Audio Record/Playback default settings

    This api is used to configure the audio record/playback default configurations.
    This must be called after aud_open().

    @param[in] DefSet  Default setting select. Please refer to AUDIO_DEFSET for details.

    @return E_OK: configuration done.
*/
ER aud_set_default_setting(AUDIO_DEFSET DefSet)
{
	UINT32          selItem;
#ifdef __KERNEL__
	AUDIO_IIRCOEF   IirCoef;
#endif
	selItem = DefSet / 5;

    if(selItem != 4){
    	aud_set_feature(AUDIO_FEATURE_ALC,                           ENABLE);
    	aud_set_feature(AUDIO_FEATURE_MICBOOST,                      ENABLE);
    } else {
        aud_set_feature(AUDIO_FEATURE_ALC,                           DISABLE);
    	aud_set_feature(AUDIO_FEATURE_MICBOOST,                      DISABLE);
    }

	aud_set_feature(AUDIO_FEATURE_NG_WITH_MICBOOST,              AudDefSettings[selItem].BoostCompEn);
	aud_set_feature(AUDIO_FEATURE_NOISEGATE_EN,                  AudDefSettings[selItem].NoiseGateEn);

	aud_set_parameter(AUDIO_PARAMETER_BOOST_GAIN,                AudDefSettings[selItem].BoostGain);
	aud_set_parameter(AUDIO_PARAMETER_ALC_MAXGAIN,               AudDefSettings[selItem].AlcMaxGain);
	aud_set_parameter(AUDIO_PARAMETER_ALC_MINGAIN,               AudDefSettings[selItem].AlcMinGain);
	aud_set_parameter(AUDIO_PARAMETER_ALC_ATTACK_TIME,           AudDefSettings[selItem].AlcAttack);
	aud_set_parameter(AUDIO_PARAMETER_ALC_DECAY_TIME,            AudDefSettings[selItem].AlcDecay);
	aud_set_parameter(AUDIO_PARAMETER_ALC_HOLD_TIME,             AudDefSettings[selItem].AlcHold);
	aud_set_parameter(AUDIO_PARAMETER_ALC_TIME_RESOLUTION,       AudDefSettings[selItem].AlcTimeReso);
	aud_set_parameter(AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION, AudDefSettings[selItem].NgTimeReso);
    if(AudDefSettings[selItem].BoostGain != EAC_PGABOOST_SEL_0DB){
		aud_set_parameter(AUDIO_PARAMETER_NOISETHD_WITH_BOOST,       AudDefSettings[selItem].NgTHD);
    }else{
        aud_set_parameter(AUDIO_PARAMETER_NOISETHD_WITHOUT_BOOST,    AudDefSettings[selItem].NgTHD);
    }
	aud_set_parameter(AUDIO_PARAMETER_NOISEGAIN,                 AudDefSettings[selItem].NgTarget);
	aud_set_parameter(AUDIO_PARAMETER_ALCNG_ATTACK_TIME,         AudDefSettings[selItem].NgAttack);
	aud_set_parameter(AUDIO_PARAMETER_ALCNG_DECAY_TIME,          AudDefSettings[selItem].NgDecay);

#ifdef __KERNEL__

    // Set Default IIR HighPass 3dB to 200Hz for SR48KHz.
    IirCoef.fTotalGain      =  AUDIO_IIR_COEF(648.28896326809217);
    IirCoef.fSectionGain    =  AUDIO_IIR_COEF(0.0015002783536477247);
    IirCoef.fCoefB0         =  AUDIO_IIR_COEF(1.0);
    IirCoef.fCoefB1         =  AUDIO_IIR_COEF(-2.0);
    IirCoef.fCoefB2         =  AUDIO_IIR_COEF(1.0);
    IirCoef.fCoefA0         =  AUDIO_IIR_COEF(1.0);
    IirCoef.fCoefA1         =  AUDIO_IIR_COEF(-1.9444776577670932);
    IirCoef.fCoefA2         =  AUDIO_IIR_COEF(0.94597793623228121);
    if( DefSet%5 == 4)
    {
    	// Set Default IIR HighPass 3dB to 300Hz for SR48KHz.
    	IirCoef.fTotalGain      =  AUDIO_IIR_COEF(648.28896326809217);
    	IirCoef.fSectionGain    =  AUDIO_IIR_COEF(0.0015002783536477247);
   		IirCoef.fCoefB0         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefB1         =  AUDIO_IIR_COEF(-2.0);
   		IirCoef.fCoefB2         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefA0         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefA1         =  AUDIO_IIR_COEF(-1.9444776577670932);
   		IirCoef.fCoefA2         =  AUDIO_IIR_COEF(0.94597793623228121);
    }else if(DefSet%5 == 3)
    {
   		// Set Default IIR HighPass 3dB to 300Hz for SR32KHz.
   		IirCoef.fTotalGain      =  AUDIO_IIR_COEF(288.03592899375218);
   		IirCoef.fSectionGain    =  AUDIO_IIR_COEF(0.0033301510439662668);
   		IirCoef.fCoefB0         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefB1         =  AUDIO_IIR_COEF(-2.0);
   		IirCoef.fCoefB2         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefA0         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefA1         =  AUDIO_IIR_COEF(-1.916741223157622);
   		IirCoef.fCoefA2         =  AUDIO_IIR_COEF(0.92007137539572681);
    }else if(DefSet%5 == 2)
    {
    	if(selItem == 5){
	    	// Set Default IIR Low Pass 3dB to 4500Hz for SR16KHz.
	    	IirCoef.fTotalGain      =  AUDIO_IIR_COEF(4.0340522436963617);
	   		IirCoef.fSectionGain    =  AUDIO_IIR_COEF(0.12881465635846334);
	   		IirCoef.fCoefB0         =  AUDIO_IIR_COEF(1.0);
	   		IirCoef.fCoefB1         =  AUDIO_IIR_COEF(0.0);
	   		IirCoef.fCoefB2         =  AUDIO_IIR_COEF(-1.0);
	    	IirCoef.fCoefA0         =  AUDIO_IIR_COEF(1.0);
	    	IirCoef.fCoefA1         =  AUDIO_IIR_COEF(-0.83189523663386711);
	   		IirCoef.fCoefA2         =  AUDIO_IIR_COEF(-0.039290107007699522);			
		} else {
	    	// Set Default IIR HighPass 3dB to 300Hz for SR16KHz.
	    	IirCoef.fTotalGain      =  AUDIO_IIR_COEF(71.884445301605183);
	   		IirCoef.fSectionGain    =  AUDIO_IIR_COEF(0.012799238480158693);
	   		IirCoef.fCoefB0         =  AUDIO_IIR_COEF(1.0);
	   		IirCoef.fCoefB1         =  AUDIO_IIR_COEF(-2.0);
	   		IirCoef.fCoefB2         =  AUDIO_IIR_COEF(1.0);
	    	IirCoef.fCoefA0         =  AUDIO_IIR_COEF(1.0);
	    	IirCoef.fCoefA1         =  AUDIO_IIR_COEF(-1.8337326589246481);
	   		IirCoef.fCoefA2         =  AUDIO_IIR_COEF(0.84653197479202402);
		}
     }else if(DefSet%5 == 1)
     {
    	// Set Default IIR HighPass 3dB to 300Hz for SR8KHz.
    	IirCoef.fTotalGain      =  AUDIO_IIR_COEF(17.847971014101539);
   		IirCoef.fSectionGain    =  AUDIO_IIR_COEF(0.047426077364203303);
    	IirCoef.fCoefB0         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefB1         =  AUDIO_IIR_COEF(-2.0);
   		IirCoef.fCoefB2         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefA0         =  AUDIO_IIR_COEF(1.0);
   		IirCoef.fCoefA1         =  AUDIO_IIR_COEF(-1.6692031429311931);
   		IirCoef.fCoefA2         =  AUDIO_IIR_COEF(0.71663387350415764);
     }

	aud_set_parameter(AUDIO_PARAMETER_IIRCOEF_L, (UINT32)&IirCoef);
	aud_set_parameter(AUDIO_PARAMETER_IIRCOEF_R, (UINT32)&IirCoef);
#endif
	if (DefSet % 5 != 0) {
		aud_set_feature(AUDIO_FEATURE_ALC_IIR_EN,            ENABLE);
		aud_set_feature(AUDIO_FEATURE_OUTPUT_IIR_EN,         ENABLE);
	} else {
		aud_set_feature(AUDIO_FEATURE_ALC_IIR_EN,            DISABLE);
		aud_set_feature(AUDIO_FEATURE_OUTPUT_IIR_EN,         DISABLE);
	}

	if(selItem == 5){ // dmic setting always IIR on
		aud_set_feature(AUDIO_FEATURE_ALC_IIR_EN,            ENABLE);
		aud_set_feature(AUDIO_FEATURE_OUTPUT_IIR_EN,         ENABLE);		
	}

	return E_OK;
}



#endif

#if 1

/************************************************************************/
/*                                                                      */
/* AUDIO I2S codec command API                                          */
/*                                                                      */
/************************************************************************/


/**
    Set I2S TDM Interface is 16 or 32 bits per audio channel

    Set I2S TDM Interface is 16 or 32 bits per audio channel.
    This can only be used when clock ratio using AUDIO_I2SCLKR_256FS_64BIT / AUDIO_I2SCLKR_256FS_128BIT
    / AUDIO_I2SCLKR_256FS_256BIT only.

    @param[in] ChBits Use AUDIO_I2SCH_BITS_16 or AUDIO_I2SCH_BITS_32

    @return void
*/
void aud_set_i2s_chbits(AUDIO_I2SCH_BITS ChBits)
{
	dai_set_i2s_config(DAI_I2SCONFIG_ID_CHANNEL_LEN, (UINT32)ChBits);
}


/**
    Set I2S format

    This function set I2S format
    The result is codec dependent.

    @param[in] I2SFmt   I2S format. Available values are below:
                        - @b AUDIO_I2SFMT_STANDARD: I2S Standard
                        - @b AUDIO_I2SFMT_LIKE_MSB: I2S Like, MSB justified
                        - @b AUDIO_I2SFMT_LIKE_LSB: I2S Like, LSB justified
    @return void
*/
void aud_set_i2s_format(AUDIO_I2SFMT I2SFmt)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)I2SFmt);
	}



	switch (I2SFmt) {
	case AUDIO_I2SFMT_STANDARD: {
			dai_set_i2s_config(DAI_I2SCONFIG_ID_FORMAT, DAI_I2SFMT_STANDARD);
		}
		break;

	default: {
			DBG_ERR("Only AUDIO_I2SFMT_STANDARD is valid\r\n");
		}
		break;
	}



	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setFormat != NULL) {
			AudioCodecFunc[i].setFormat(I2SFmt);
		}
	}


}

/**
    Set I2S clock ratio

    This function set I2S clock ratio
    The result is codec dependent.

    @param[in] I2SCLKRatio  I2S Clock Ratio. Available values are below:
                            - @b AUDIO_I2SCLKR_256FS_32BIT: SystemClk = 256 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_256FS_64BIT: SystemClk = 256 FrameSync, FrameSync = 64 BitClk
                            - @b AUDIO_I2SCLKR_384FS_32BIT: SystemClk = 384 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_384FS_48BIT: SystemClk = 384 FrameSync, FrameSync = 48 BitClk
                            - @b AUDIO_I2SCLKR_384FS_96BIT: SystemClk = 384 FrameSync, FrameSync = 96 BitClk
                            - @b AUDIO_I2SCLKR_512FS_32BIT: SystemClk = 512 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_512FS_64BIT: SystemClk = 512 FrameSync, FrameSync = 64 BitClk
                            - @b AUDIO_I2SCLKR_512FS_128BIT: SystemClk = 512 FrameSync, FrameSync = 128 BitClk
                            - @b AUDIO_I2SCLKR_768FS_32BIT: SystemClk = 768 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_768FS_48BIT: SystemClk = 768 FrameSync, FrameSync = 48 BitClk
                            - @b AUDIO_I2SCLKR_768FS_64BIT: SystemClk = 768 FrameSync, FrameSync = 64 BitClk
                            - @b AUDIO_I2SCLKR_768FS_192BIT: SystemClk = 768 FrameSync, FrameSync = 192 BitClk
                            - @b AUDIO_I2SCLKR_1024FS_32BIT: SystemClk = 1024 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_1024FS_64BIT: SystemClk = 1024 FrameSync, FrameSync = 64 BitClk
                            - @b AUDIO_I2SCLKR_1024FS_128BIT: SystemClk = 1024 FrameSync, FrameSync = 128 BitClk
                            - @b AUDIO_I2SCLKR_1024FS_256BIT: SystemClk = 1024 FrameSync, FrameSync = 256 BitClk
    @return void
*/
void aud_set_i2s_clkratio(AUDIO_I2SCLKR I2SCLKRatio)
{
	UINT32 i;

	if (aud_is_opened() == FALSE) {
		DBG_ERR("is closed\r\n");
		return;
	}

	if (audioMsgLevel == AUDIO_MSG_LVL_1) {
		DBG_DUMP("%s: %d\r\n", __func__, (int)I2SCLKRatio);
	}


	dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, I2SCLKRatio);


	for (i = 0; i < dim(AudioCodecFunc); i++) {
		if (AudioCodecFunc[i].setClockRatio != NULL) {
			AudioCodecFunc[i].setClockRatio(I2SCLKRatio);
		}
	}

}

/**
    Send command to I2S codec

    Send command to I2S codec.
    (OBSOLETE)

    @param[in] uiRegIdx I2S codec register index
    @param[in] uiData   Data you want to send

    @return void
*/
BOOL aud_set_i2s_sentcommand(UINT32 uiRegIdx, UINT32 uiData)
{
	if ((AudioCodecFunc[AudioSelectedCodec].sendCommand != NULL) &&
		(aud_is_opened() == TRUE)) {
		if (AudioCodecFunc[AudioSelectedCodec].sendCommand != NULL) {
			return AudioCodecFunc[AudioSelectedCodec].sendCommand(uiRegIdx, uiData);
		}
	}

	return FALSE;
}

/**
  Read register data from I2S codec

  Read register data from I2S codec.
  (OBSOLETE)

  @param[in] uiRegIdx   I2S codec register index you want to read
  @param[out] puiData   Register data

  @return
        - @b TRUE: read register successfully
        - @b FALSE: read register fail
*/
BOOL aud_get_i2s_readdata(UINT32 uiRegIdx, UINT32 *puiData)
{
	if ((AudioCodecFunc[AudioSelectedCodec].readData != NULL) &&
		(aud_is_opened() == TRUE)) {
		if (AudioCodecFunc[AudioSelectedCodec].readData != NULL) {
			return AudioCodecFunc[AudioSelectedCodec].readData(uiRegIdx, puiData);
		}
	}

	*puiData = 0;
	return FALSE;
}

#endif

