/*
    Audio Filtering library Driver file.

    This file is the driver of Audio Filtering library.

    @file       audFilt.c
    @ingroup    mIAudFilt
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifdef __KERNEL__
#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "kdrv_audioio/audlib_filt.h"
#include "iir.h"
#include "audlib_filt_dbg.h"

#define FILT_DESIFN_SUPPORT DISABLE
#else
#include <string.h>
#include "kdrv_audioio/audlib_filt.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "iir.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#if IM_DBG
#define __MODULE__ rtos_audfilt
#define __DBGLVL__ 8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"
#include "kwrap/debug.h"
unsigned int rtos_audfilt_debug_level = NVT_DBG_WRN;

#define FILT_DESIFN_SUPPORT ENABLE
#endif


#endif

/**
    @addtogroup mIAudFilt
*/
//@{

#ifndef __KERNEL__
//NVTVER_VERSION_ENTRY(AudFilt, 2, 00, 000, 00)
#endif

#define AUDFILT_BUFSIZE         512 // 512x4 = 2K Bytes
#define AUDFILT_MSW_VERSION     0x0205

static BOOL     gbAudFiltOpened = FALSE;
static UINT32   gbAudFiltStepSize = 1024;

static INT32    guiAudFiltBuf[AUDFILT_BUFSIZE];

static BOOL     gbIirEnable 	= DISABLE;
static BOOL     gbEqEnable 		= DISABLE;
static UINT32   guiEqBandNumber = 0;

static KDRV_AUDIOLIB_FUNC filter_func =
{
	.filter.open				= audlib_filt_open,
	.filter.is_open				= audlib_filt_is_opened,
	.filter.close				= audlib_filt_close,
	.filter.init				= audlib_filt_init,
	.filter.set_config			= audlib_filt_set_config,
	.filter.enable_filt			= audlib_filt_enable_filt,
	.filter.design_filt			= audlib_filt_design_filt,
	.filter.run					= audlib_filt_run,
	.filter.enable_eq			= audlib_filt_enable_eq,
	.filter.design_eq			= audlib_filt_design_eq,
};


#if 1

/**
    Open the audio filter library.

    This API must be called to initialize the audio filter basic settings.

    @param[in] p_filt_init    The audio filter initial parameters. Please refer to structure AUDFILT_INIT for reference.
    @return
     - @b TRUE:  Open Done and success.
     - @b FALSE: Open Operation fail.
*/
BOOL     audlib_filt_open(PAUDFILT_INIT p_filt_init)
{
	EN_AUD_IIR_CHANNEL enIIRChannel;
	BOOL b8EnableSmooth;
	u32 u32InternalBufsize;
	s32 *ps32InternalBufAddr = (s32 *)(guiAudFiltBuf);

	if (AUD_IIR_GetVersion() != AUDFILT_MSW_VERSION) {
		DBG_ERR("audlib_IIR.a Version Error!(V.%04X) Pls use V.%04X\r\n", AUD_IIR_GetVersion(), AUDFILT_MSW_VERSION);
		//return FALSE;
	}

	if (gbAudFiltOpened) {
		return TRUE;
	}

	switch (p_filt_init->filt_ch) {
	case AUDFILT_CH_MONO:
		gbAudFiltStepSize = 512;
		enIIRChannel = EN_AUD_IIR_CHANNEL_MONO;
		break;
	case AUDFILT_CH_STEREO:
		gbAudFiltStepSize = 1024;
		enIIRChannel = EN_AUD_IIR_CHANNEL_STEREO;
		break;
	default:
		return FALSE;
		break;
	}

	b8EnableSmooth = p_filt_init->smooth_enable;
	u32InternalBufsize = (AUDFILT_BUFSIZE << 2);

	if (!AUD_IIR_Open(enIIRChannel, ps32InternalBufAddr, u32InternalBufsize, b8EnableSmooth)) {
		return FALSE;
	}

	gbIirEnable 	= 0;
	gbAudFiltOpened = TRUE;
	return TRUE;
}

/**
    Check if the audio filter library is opened or not.

    Check if the audio filter library is opened or not.

    @return
     - @b TRUE:  Already Opened.
     - @b FALSE: Have not opened.
*/
BOOL     audlib_filt_is_opened(void)
{
	return gbAudFiltOpened;
}

/**
    Close the audio filter library.

    Close the audio filter library.
    After closing the audio filter library, any accessing for the audio filter APIs are forbidden except audlib_filt_open().

    @return
     - @b TRUE:  Close Done and success.
     - @b FALSE: Close Operation fail.
*/
BOOL     audlib_filt_close(void)
{
	if (!gbAudFiltOpened) {
		return TRUE;
	}

	if (!AUD_IIR_End()) {
		return FALSE;
	}

	gbIirEnable 	= 0;
	gbEqEnable 		= DISABLE;
	guiEqBandNumber = 0;

	gbAudFiltOpened = FALSE;
	return TRUE;
}

/**
    Set specified audio filter configuration.

    This API is used to set the specified audio filter configuration
    such as Filter-Order / Filter-Coeficients.
    The basic implementaion of the filter is the IIR section, so the filter parameters for one section is 6 entries.
    The format is like H(z) = (Section-Gain) x (B0 + B1*Z1 + B2*Z2) / (A0 + A1*Z1 + A2*Z2).
    The six entries are (B0,B1,B2,A0,A1,A2) in order. And the format for the six entries are Q4.27, which means the allowed
    floating point coefficient value range is from -16.0 to 15.99999.
    The format for the Section input Gain and the Total output Gain is Q15.16, which means the allowed
    floating point coefficient value range is from -32768.0 to 32767.99999.

    @param[in] filt_sel      Select which of the filter is selected for configuring.
    @param[in] p_filt_config  The configurations of the specified audio filter.

    @return
     - @b TRUE:  setConfig Done and success.
     - @b FALSE: setConfig Operation fail.
*/
BOOL     audlib_filt_set_config(AUDFILT_SELECTION filt_sel, PAUDFILT_CONFIG p_filt_config)
{
	unsigned int i = 0;
	EN_AUD_IIR_TYPE enIIRType;
	ST_AUD_IIR_FILTER_CONFIG stIIRFilterCfg;
	PST_AUD_IIR_FILTER_CONFIG pstIIRFilterCfg = &stIIRFilterCfg;
	u32 u32FilterOrder;

	if (!gbAudFiltOpened) {
		return FALSE;
	}

	if (p_filt_config == NULL) {
		return FALSE;
	}

	u32FilterOrder = p_filt_config->filter_order;

	if (u32FilterOrder > IIR_MAX_FILTER_ORDER || ((u32FilterOrder & 0x1) != 0)) {
		return FALSE;
	}

	switch (filt_sel) {
	case AUDFILT_SEL_NOTCH1:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_1;
		break;
	case AUDFILT_SEL_NOTCH2:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_2;
		break;
	case AUDFILT_SEL_NOTCH3:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_3;
		break;
	case AUDFILT_SEL_NOTCH4:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_4;
		break;
	case AUDFILT_SEL_NOTCH5:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_5;
		break;
	case AUDFILT_SEL_HIGHPASS1:
		enIIRType = EN_AUD_IIR_TYPE_HIGH_PASS_1;
		break;
	case AUDFILT_SEL_BANDSTOP1:
		enIIRType = EN_AUD_IIR_TYPE_BAND_STOP_1;
		break;
	case AUDFILT_SEL_LOWPASS1:
		enIIRType = EN_AUD_IIR_TYPE_LOW_PASS_1;
		break;
	default:
		return FALSE;
		break;
	}

	memset(pstIIRFilterCfg, 0, sizeof(ST_AUD_IIR_FILTER_CONFIG));

	pstIIRFilterCfg->enIIRType = enIIRType;
	pstIIRFilterCfg->u32FilterOrder = u32FilterOrder;
	pstIIRFilterCfg->s32totalGain = p_filt_config->total_gain;

	for (i = 0; i < (pstIIRFilterCfg->u32FilterOrder / 2); i++) {
		pstIIRFilterCfg->as32Gain[i] = p_filt_config->p_section_gain[i];
		pstIIRFilterCfg->as32Coef[i][0] = p_filt_config->p_filt_coef[i][0];
		pstIIRFilterCfg->as32Coef[i][1] = p_filt_config->p_filt_coef[i][1];
		pstIIRFilterCfg->as32Coef[i][2] = p_filt_config->p_filt_coef[i][2];
		pstIIRFilterCfg->as32Coef[i][3] = p_filt_config->p_filt_coef[i][3];
		pstIIRFilterCfg->as32Coef[i][4] = p_filt_config->p_filt_coef[i][4];
		pstIIRFilterCfg->as32Coef[i][5] = p_filt_config->p_filt_coef[i][5];
	}

	if (!AUD_IIR_SetParams(enIIRType, pstIIRFilterCfg)) {
		return FALSE;
	}

	return TRUE;
}

/**
	Audio Filtering Parameters Design

	Design Filter paramters. This API supports HighPass/Lowpass/Peaking/HighShelf/LowShelf/Notch filter type design.
	The user needs only input the sample-rate/filter-type/filter-gain/Q/frequency to retrive the filter paramter settings.
	The default designed filter order is fixed order 2, and using design_ctrl can making designed order become 4.

	@param[in] 	p_eq_param		Target design filter settings.
	@param[out] p_filt_config		Designed Output filter settings for audlib_filt_set_config() usages.
	@param[in]  design_ctrl		Design control. This can control the designed filter order and making filter response more sharped.

	@return
     - @b TRUE:  DesignFilter Done and success.
     - @b FALSE: DesignFilter Operation fail.
*/
BOOL audlib_filt_design_filt(PAUDFILT_EQPARAM p_eq_param, PAUDFILT_CONFIG p_filt_config, AUDFILT_DESGIN_CTRL design_ctrl)
{
#if FILT_DESIFN_SUPPORT
	ST_AUD_IIR_FILTER_CONFIG stIIRFilterCfg;
	PST_AUD_IIR_FILTER_CONFIG pstIIRFilterCfg = &stIIRFilterCfg;
	AUDFILT_IIRDESIGN stIIRDesign;
	PAUDFILT_IIRDESIGN pstIIRDesign = &stIIRDesign;
	s32 *s32DestTableAddr = NULL;
	UINT32 i=0;

	if (!gbAudFiltOpened) {
		return FALSE;
	}

    memset(pstIIRFilterCfg, 0, sizeof(ST_AUD_IIR_FILTER_CONFIG));
    memset(pstIIRDesign,    0, sizeof(AUDFILT_IIRDESIGN));

	if(p_eq_param->Q < 0.03125) {
		p_eq_param->Q = 0.03125;
	}else if (p_eq_param->Q > 10.0) {
		p_eq_param->Q = 10.0;
	}

	if(p_eq_param->gain_db < -60) {
		p_eq_param->gain_db = -60;
	}else if (p_eq_param->gain_db > 12.0) {
		p_eq_param->gain_db = 12.0;
	}

    pstIIRFilterCfg->u32FilterOrder = 2;
    pstIIRFilterCfg->s32totalGain 	= APU_AUD_POST_FXP32(1.000000, GAIN_FORMAT);
    pstIIRFilterCfg->as32Gain[0] 	= APU_AUD_POST_FXP32(1.000000, GAIN_FORMAT);

	if (p_eq_param->filt_type == AUDFILT_DESIGNTYPE_NOTCH) {
		p_eq_param->filt_type = AUDFILT_DESIGNTYPE_PEAKING;

		if(p_eq_param->gain_db > 0)
			p_eq_param->gain_db = -p_eq_param->gain_db;
	}

	switch (p_eq_param->filt_type) {
	case AUDFILT_DESIGNTYPE_BYPASS:
		pstIIRDesign->FiltType = AUDFILT_TYPE_BYPASS;
		break;
	case AUDFILT_DESIGNTYPE_LOWPASS:
		pstIIRDesign->FiltType = AUDFILT_TYPE_LOWPASS;
		break;
	case AUDFILT_DESIGNTYPE_HIGHPASS:
		pstIIRDesign->FiltType = AUDFILT_TYPE_HIGHPASS;
		break;
	case AUDFILT_DESIGNTYPE_PEAKING:
		pstIIRDesign->FiltType = AUDFILT_TYPE_PEAKING;
		break;
	case AUDFILT_DESIGNTYPE_HIGHSHELF:
		pstIIRDesign->FiltType = AUDFILT_TYPE_HIGHSHELF;
		break;
	case AUDFILT_DESIGNTYPE_LOWSHELF:
		pstIIRDesign->FiltType = AUDFILT_TYPE_LOWSHELF;
		break;
	default:
		return FALSE;
		break;
	}

	s32DestTableAddr = (s32 *)pstIIRFilterCfg->as32Coef[0];

	FiltDesign_FilterDesign(pstIIRDesign, (double) p_eq_param->frequency, (double) p_eq_param->sample_rate,
							(double) p_eq_param->gain_db, (double) p_eq_param->Q);

	FiltDesign_FilterTblFloat2FXP(pstIIRDesign, s32DestTableAddr, COEF_FORMAT);

	if(design_ctrl > AUDFILT_DESGIN_CTRL_MAX)
		design_ctrl = AUDFILT_DESGIN_CTRL_MAX;
	if(design_ctrl < AUDFILT_DESGIN_CTRL_DEFAULT)
		design_ctrl = AUDFILT_DESGIN_CTRL_DEFAULT;

	p_filt_config->filter_order = pstIIRFilterCfg->u32FilterOrder*design_ctrl;
	p_filt_config->total_gain  = pstIIRFilterCfg->s32totalGain;
	for (i = 0; i < design_ctrl; i++) {
		p_filt_config->p_section_gain[i] = pstIIRFilterCfg->as32Gain[0];
		p_filt_config->p_filt_coef[i][0] = pstIIRFilterCfg->as32Coef[0][0];
		p_filt_config->p_filt_coef[i][1] = pstIIRFilterCfg->as32Coef[0][1];
		p_filt_config->p_filt_coef[i][2] = pstIIRFilterCfg->as32Coef[0][2];
		p_filt_config->p_filt_coef[i][3] = pstIIRFilterCfg->as32Coef[0][3];
		p_filt_config->p_filt_coef[i][4] = pstIIRFilterCfg->as32Coef[0][4];
		p_filt_config->p_filt_coef[i][5] = pstIIRFilterCfg->as32Coef[0][5];
	}

	return TRUE;
#else
	DBG_WRN("kernel mode support float operation. filter design is not support.\r\n");
	return FALSE;
#endif

}


#endif



/**
    Set specified audio filter enable/disable.

    Set specified audio filter enable/disable. If the smooth function is enabled, the audio sample would be
    fade-in/out in the next audio 256 samples after the transition of the filter enable/disable.

    @param[in] filt_sel      Select which of the filter is selected for configuring.
    @param[in] enable
     - @b TRUE:  Enable the specified auido filter.
     - @b FALSE: Disable the specified auido filter.

    @return
     - @b TRUE:  setEnable Done and success.
     - @b FALSE: setEnable Operation fail.
*/
BOOL     audlib_filt_enable_filt(AUDFILT_SELECTION filt_sel, BOOL enable)
{
	EN_AUD_IIR_TYPE enIIRType;

	if (!gbAudFiltOpened) {
		return FALSE;
	}

	switch (filt_sel) {
	case AUDFILT_SEL_NOTCH1:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_1;
		break;
	case AUDFILT_SEL_NOTCH2:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_2;
		break;
	case AUDFILT_SEL_NOTCH3:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_3;
		break;
	case AUDFILT_SEL_NOTCH4:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_4;
		break;
	case AUDFILT_SEL_NOTCH5:
		enIIRType = EN_AUD_IIR_TYPE_NOTCH_5;
		break;
	case AUDFILT_SEL_HIGHPASS1:
		enIIRType = EN_AUD_IIR_TYPE_HIGH_PASS_1;
		break;
	case AUDFILT_SEL_BANDSTOP1:
		enIIRType = EN_AUD_IIR_TYPE_BAND_STOP_1;
		break;
	case AUDFILT_SEL_LOWPASS1:
		enIIRType = EN_AUD_IIR_TYPE_LOW_PASS_1;
		break;
	default:
		return FALSE;
		break;
	}

	if (enable) {
		AUD_IIR_Enable(enIIRType);
		gbIirEnable |= (0x1 << filt_sel);

	} else {
		AUD_IIR_Disable(enIIRType);
		gbIirEnable &= ~(0x1 << filt_sel);
	}

	return TRUE;
}

/**
	ENABLE/DISABLE Audio EQ

	ENABLE/DISABLE Audio EQ with specified band number during audlib_filt_run().
	The EQ band design configurations are using audlib_filt_design_eq() API to implement.

	@param[in] enable		EQ function ENABLE/DISABLE.
	@param[in] band_num	EQ band numbers. Valid value is 1~12.

	@return void
*/
void audlib_filt_enable_eq(BOOL enable, UINT32 band_num)
{
	if (!gbAudFiltOpened) {
		DBG_ERR("AudFilt on opened.\r\n");
		return;
	}

	if(band_num > AUDFILT_EQBAND_MAX) {
		DBG_ERR("EQ Max bands is %d < %d\r\n",(int)AUDFILT_EQBAND_MAX,(int)band_num);
		return;
	}

	guiEqBandNumber = band_num;

	if (guiEqBandNumber) {
		gbEqEnable      = enable;
	} else {
		gbEqEnable 		= DISABLE;
		guiEqBandNumber	= 0;
	}
}

/**
	Audio EQ Filter Design

	Design EQ Filter paramters. This API supports HighPass/Lowpass/Peaking/HighShelf/LowShelf/Notch filter type design.
	The user needs only input the sample-rate/filter-type/filter-gain/Q/frequency to retrive the filter paramter settings.

	@param[in]  BandIndex		Designed EQ band index.
	@param[in] 	p_eq_param		Target EQ band filter settings.

	@return
     - @b TRUE:  DesignEQ band Done and success.
     - @b FALSE: DesignEQ band Operation fail.
*/
BOOL audlib_filt_design_eq(AUDFILT_EQBAND BandIndex, PAUDFILT_EQPARAM p_eq_param)
{
#if FILT_DESIFN_SUPPORT
	ST_AUD_IIR_FILTER_CONFIG stIIRFilterCfg;
	PST_AUD_IIR_FILTER_CONFIG pstIIRFilterCfg = &stIIRFilterCfg;
	AUDFILT_IIRDESIGN stIIRDesign;
	PAUDFILT_IIRDESIGN pstIIRDesign = &stIIRDesign;
	s32 *s32DestTableAddr = NULL;

	if(BandIndex >= guiEqBandNumber) {
		DBG_ERR("Idx-%d exceed band number %d\r\n",(int)BandIndex+1,(int)guiEqBandNumber);
		return FALSE;
	}

    memset(pstIIRFilterCfg, 0, sizeof(ST_AUD_IIR_FILTER_CONFIG));
    memset(pstIIRDesign,    0, sizeof(AUDFILT_IIRDESIGN));

	if(p_eq_param->Q < 0.03125) {
		p_eq_param->Q = 0.03125;
	}else if (p_eq_param->Q > 10.0) {
		p_eq_param->Q = 10.0;
	}

	if(p_eq_param->gain_db < -60) {
		p_eq_param->gain_db = -60;
	}else if (p_eq_param->gain_db > 12.0) {
		p_eq_param->gain_db = 12.0;
	}

    pstIIRFilterCfg->u32FilterOrder = 2;
    pstIIRFilterCfg->s32totalGain 	= APU_AUD_POST_FXP32(1.000000, GAIN_FORMAT);
    pstIIRFilterCfg->as32Gain[0] 	= APU_AUD_POST_FXP32(1.000000, GAIN_FORMAT);

	if (p_eq_param->filt_type == AUDFILT_DESIGNTYPE_NOTCH) {
		p_eq_param->filt_type = AUDFILT_DESIGNTYPE_PEAKING;

		if(p_eq_param->gain_db > 0)
			p_eq_param->gain_db = -p_eq_param->gain_db;
	}

	switch (p_eq_param->filt_type) {
	case AUDFILT_DESIGNTYPE_BYPASS:
		pstIIRDesign->FiltType = AUDFILT_TYPE_BYPASS;
		break;
	case AUDFILT_DESIGNTYPE_LOWPASS:
		pstIIRDesign->FiltType = AUDFILT_TYPE_LOWPASS;
		break;
	case AUDFILT_DESIGNTYPE_HIGHPASS:
		pstIIRDesign->FiltType = AUDFILT_TYPE_HIGHPASS;
		break;
	case AUDFILT_DESIGNTYPE_PEAKING:
		pstIIRDesign->FiltType = AUDFILT_TYPE_PEAKING;
		break;
	case AUDFILT_DESIGNTYPE_HIGHSHELF:
		pstIIRDesign->FiltType = AUDFILT_TYPE_HIGHSHELF;
		break;
	case AUDFILT_DESIGNTYPE_LOWSHELF:
		pstIIRDesign->FiltType = AUDFILT_TYPE_LOWSHELF;
		break;
	default:
		return FALSE;
		break;
	}

	s32DestTableAddr = (s32 *)pstIIRFilterCfg->as32Coef[0];

	FiltDesign_FilterDesign(pstIIRDesign, (double) p_eq_param->frequency, (double) p_eq_param->sample_rate,
							(double) p_eq_param->gain_db, (double) p_eq_param->Q);

	FiltDesign_FilterTblFloat2FXP(pstIIRDesign, s32DestTableAddr, COEF_FORMAT);

	AUD_PEQ_SetParams(BandIndex, guiEqBandNumber, pstIIRFilterCfg);

	AUD_PEQ_Enable(BandIndex, guiEqBandNumber);

	return TRUE;
#else
	DBG_WRN("kernel mode support float operation. filter design is not support.\r\n");
	return FALSE;
#endif

}

/**
    Apply audio filtering to the input audio bit stream

    Apply specified audio filtering to the input audio bit stream.
    If the smooth function is enabled and one of the audio filter is enabled/disabled, the first 256 samples
    in the audio bit stream would be fade in/out to the incoming audio stream.
    In the current implementation, the input bit stream length must be multiple of 512 Bytes for MONO channel
    and multiple of 1024 Bytes for Stereo channel.
    The input BitStream format support for PCM-16Bits per sample only.

    @param[in] p_filt_io    The input BitStream information.

    @return
     - @b TRUE:  applyFiltering Done and success.
     - @b FALSE: applyFiltering Operation fail.
*/
BOOL     audlib_filt_run(PAUDFILT_BITSTREAM  p_filt_io)
{
	ST_AUD_IIR_INPUT_CONFIG     stIIRInputCfg;
	PST_AUD_IIR_INPUT_CONFIG    pstIIRInputCfg = &stIIRInputCfg;
	UINT32                      uiProc, uiOrig;

	if (!gbAudFiltOpened) {
		return FALSE;
	}

	if (p_filt_io == NULL) {
		return FALSE;
	}

	uiProc = 0;
	uiOrig = p_filt_io->bitstram_buffer_length;

#if IM_DBG
	if (uiOrig & (gbAudFiltStepSize - 1)) {
		uiOrig &= ~(gbAudFiltStepSize - 1);
		DBG_WRN("Length must be multiples of %d!\r\n", (int)gbAudFiltStepSize);
	}
#endif

	while (uiOrig - uiProc) {
		pstIIRInputCfg->ps16IPBufAddr = (s16 *)(p_filt_io->bitstram_buffer_in + uiProc);
		pstIIRInputCfg->ps16OPBufAddr = (s16 *)(p_filt_io->bitstram_buffer_out + uiProc);

		if ((uiOrig - uiProc) > gbAudFiltStepSize) {
			pstIIRInputCfg->u32Len = gbAudFiltStepSize;
		} else {
			pstIIRInputCfg->u32Len = uiOrig - uiProc;
		}

		uiProc += pstIIRInputCfg->u32Len;

		if(gbIirEnable) {
			if (!AUD_IIR_Main(pstIIRInputCfg)) {
				DBG_ERR("FILT ERR!\r\n");
				return FALSE;
			}
		}

		if (gbEqEnable) {
	        if(!AUD_PEQ_Main (guiEqBandNumber, pstIIRInputCfg))
	        {
				DBG_ERR("EQILT ERR!\r\n");
	            return FALSE;
	        }
		}

	}

	return TRUE;
}

/**
    Initialize the audio filtering before recording start

    This API would reset the filtering internal temp buffer.
    The user must call this before any new filtering BitStream.

    @return
     - @b TRUE:  Done and success.
     - @b FALSE: Operation fail.
*/
BOOL     audlib_filt_init(void)
{
	if (!gbAudFiltOpened) {
		return FALSE;
	}

	if (!AUD_IIR_Initial()) {
		return FALSE;
	}

	return TRUE;
}

int kdrv_audlib_filter_init(void) {
	int ret;

	ret = kdrv_audioio_reg_audiolib(KDRV_AUDIOLIB_ID_FILT, &filter_func);

	return ret;
}

//@}

#ifdef __KERNEL__
EXPORT_SYMBOL(audlib_filt_open);
EXPORT_SYMBOL(audlib_filt_is_opened);
EXPORT_SYMBOL(audlib_filt_close);
EXPORT_SYMBOL(audlib_filt_set_config);
EXPORT_SYMBOL(audlib_filt_enable_filt);
EXPORT_SYMBOL(audlib_filt_enable_eq);
EXPORT_SYMBOL(audlib_filt_run);
EXPORT_SYMBOL(audlib_filt_init);

EXPORT_SYMBOL(audlib_filt_design_filt);
EXPORT_SYMBOL(audlib_filt_design_eq);
#endif

