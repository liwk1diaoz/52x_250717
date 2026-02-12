/*
    Emaltion External audio codec driver

    This file is the driver for Emaltion extended audio codec.

    @file       AudExtEMU.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

/** \addtogroup mISYSAud */
//@{


#ifdef __KERNEL__
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "audext_emu_dbg.h"
#include "../../audio_common/include/Audio.h"
#else
#include "AudExtEMU.h"
#include "pll.h"
#include "sif.h"
#include "i2c.h"
#include "gpio.h"
#include "top.h"
#include "Utility.h"
#include "dai.h"
#include "string.h"
#include "KerCPU.h"
#include "Debug.h"
#endif

static void audcodec_set_device_object(PAUDIO_DEVICE_OBJ p_aud_dev_obj);
static void audcodec_init(PAUDIO_SETTING p_audio_setting);
static void audcodec_set_record_source(AUDIO_RECSRC rec_source);
static void audcodec_set_output(AUDIO_OUTPUT output);
static void audcodec_set_samplerate(AUDIO_SR sample_rate);
static void audcodec_set_channel(AUDIO_CH channel);
static void audcodec_set_volume(AUDIO_VOL volume);
static void audcodec_set_gain(AUDIO_GAIN gain);
static void audcodec_set_effect(AUDIO_EFFECT effect);
static BOOL audcodec_set_feature(AUDIO_FEATURE feature, BOOL en);
static void audcodec_stop_record(void);
static void audcodec_stop_playback(void);
static void audcodec_record(void);
static void audcodec_playback(void);
static void audcodec_set_format(AUDIO_I2SFMT i2s_fmt);
static void audcodec_set_clock_ratio(AUDIO_I2SCLKR i2s_clk_ratio);
static BOOL audcodec_send_command(UINT32 reg, UINT32 data);
static BOOL audcodec_read_data(UINT32 reg, UINT32 *p_data);
static BOOL audcodec_set_parameter(AUDIO_PARAMETER parameter, UINT32 setting);
static BOOL audcodec_chk_samplerate(void);

static AUDIO_CODEC_FUNC     ext_emu_coedc_fp    = {
	AUDIO_CODEC_TYPE_EXTERNAL,

	// Set Device Object
	audcodec_set_device_object,
	// Init
	audcodec_init,
	// Set Record Source
	audcodec_set_record_source,
	// Set output
	audcodec_set_output,
	// Set Sampling rate
	audcodec_set_samplerate,
	// Set TxChannel
	audcodec_set_channel,
	// Set RxChannel
	audcodec_set_channel,
	// Set Volume (Playback)
	audcodec_set_volume,
	// Set gain (Record)
	audcodec_set_gain,
	// Set GainDB (Record)
	NULL,
	// Set effect
	audcodec_set_effect,
	// Set feature
	audcodec_set_feature,
	// StopRecord
	audcodec_stop_record,
	// StopPlayback
	audcodec_stop_playback,
	// Record Preset
	NULL,
	// Record
	audcodec_record,
	// Playback
	audcodec_playback,
	// Set I2S Format
	audcodec_set_format,
	// Set I2S clock ratio
	audcodec_set_clock_ratio,
	// Send command
	audcodec_send_command,
	// Read data
	audcodec_read_data,
	// Set parameter
	audcodec_set_parameter,
	// Check Sampling rate
	audcodec_chk_samplerate,
	// Open
	NULL,
	// Close
	NULL,
};

void aud_ext_codec_emu_get_fp(PAUDIO_CODEC_FUNC p_aud_codec_func)
{
	memcpy((void *)p_aud_codec_func, (const void *)&ext_emu_coedc_fp, sizeof(AUDIO_CODEC_FUNC));
}
static void audcodec_set_device_object(PAUDIO_DEVICE_OBJ p_aud_dev_obj)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_init(PAUDIO_SETTING p_audio_setting)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_record_source(AUDIO_RECSRC rec_source)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_output(AUDIO_OUTPUT output)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_samplerate(AUDIO_SR sample_rate)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_channel(AUDIO_CH channel)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_volume(AUDIO_VOL volume)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_gain(AUDIO_GAIN gain)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_effect(AUDIO_EFFECT effect)
{	DBG_UNIT("%s", __func__);
}
static BOOL audcodec_set_feature(AUDIO_FEATURE feature, BOOL en)
{
	DBG_UNIT("%s", __func__);
	return TRUE;
}
static void audcodec_stop_playback(void)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_stop_record(void)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_record(void)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_playback(void)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_format(AUDIO_I2SFMT i2s_fmt)
{	DBG_UNIT("%s", __func__);
}
static void audcodec_set_clock_ratio(AUDIO_I2SCLKR i2s_clk_ratio)
{	DBG_UNIT("%s", __func__);
}
static BOOL audcodec_send_command(UINT32 reg, UINT32 data)
{
	DBG_UNIT("%s", __func__);
	return TRUE;
}
static BOOL audcodec_read_data(UINT32 reg, UINT32 *p_data)
{
	DBG_UNIT("%s", __func__);
	return TRUE;
}
static BOOL audcodec_set_parameter(AUDIO_PARAMETER parameter, UINT32 setting)
{
	DBG_UNIT("%s", __func__);
	return TRUE;
}
static BOOL audcodec_chk_samplerate(void)
{
	DBG_UNIT("%s", __func__);
	return TRUE;
}


#ifdef __KERNEL__
EXPORT_SYMBOL(aud_ext_codec_emu_get_fp);
#endif
//@}
