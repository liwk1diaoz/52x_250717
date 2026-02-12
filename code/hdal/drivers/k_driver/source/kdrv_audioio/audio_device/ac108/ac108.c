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

#include "audext_ac108_dbg.h"
#include "Audio.h"
#include "ac108.h"
#endif

#ifdef __KERNEL__

#ifndef I2C_NAME
#define I2C_NAME    "audext_ac108"
#endif
#ifndef I2C_ADDR
#define I2C_ADDR 0x3B
#endif

static struct i2c_board_info audext_i2c_device = {
	.type = I2C_NAME,
	.addr = I2C_ADDR,
};


typedef struct {
	struct i2c_client  *iic_client;
	struct i2c_adapter *iic_adapter;
} AUXEXT_I2C_INFO;

static AUXEXT_I2C_INFO *audext_i2c_info;

static int audext_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	audext_i2c_info = NULL;
	audext_i2c_info = kzalloc(sizeof(AUXEXT_I2C_INFO), GFP_KERNEL);
	if (audext_i2c_info == NULL) {
		printk("%s fail: kzalloc not OK.\n", __FUNCTION__);
		return -ENOMEM;
	}

	audext_i2c_info->iic_client  = client;
	audext_i2c_info->iic_adapter = client->adapter;

	i2c_set_clientdata(client, audext_i2c_info);

	return 0;
}

static int audext_i2c_remove(struct i2c_client *client)
{
	kfree(audext_i2c_info);
	audext_i2c_info = NULL;
	return 0;
}


static const struct i2c_device_id audext_i2c_id[] = {
	{ I2C_NAME, 0 },
	{ }
};

static struct i2c_driver audext_i2c_driver = {
	.driver = {
		.name  = I2C_NAME,
		.owner = THIS_MODULE,
	},
	.probe    = audext_i2c_probe,
	.remove   = audext_i2c_remove,
	.id_table = audext_i2c_id
};


#else
static PI2C_OBJ		p_i2cobj_ac108;
static I2C_SESSION	i2csess_ac108 = I2C_TOTAL_SESSION;
#endif

static BOOL			ac108_gain_balance = FALSE;
static UINT32		digital_gain = 0xA0;



static AUDIO_CODEC_FUNC     ext_emu_codec_fp    = {
	AUDIO_CODEC_TYPE_EXTERNAL,

	// Set Device Object
	audcodec_set_device_object,
	// Init
	(void *)AUDIO_CODEC_FUNC_USE_DEFAULT,
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
	(void *)AUDIO_CODEC_FUNC_USE_DEFAULT,
	// Set gain (Record)
	audcodec_set_gain,
	// Set GainDB (Record)
	audcodec_set_gaindb,
	// Set effect
	audcodec_set_effect,
	// Set feature
	audcodec_set_feature,
	// StopRecord
	audcodec_stop_record,
	// StopPlayback
	(void *)AUDIO_CODEC_FUNC_USE_DEFAULT,
	// Record Preset
	NULL,
	// Record
	audcodec_record,
	// Playback
	(void *)AUDIO_CODEC_FUNC_USE_DEFAULT,
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
	(void *)AUDIO_CODEC_FUNC_USE_DEFAULT,
	// Close
	(void *)AUDIO_CODEC_FUNC_USE_DEFAULT,
};


#if 1
static void audext_ac108_write(UINT32 addr, UINT32 value)
{
#ifdef __KERNEL__
	struct i2c_msg  msgs;

	unsigned char	buf[2];

	buf[0]     = addr  & 0xFF;
	buf[1]     = value & 0xFF;
	msgs.addr  = I2C_ADDR;
	msgs.flags = 0;//w
	msgs.len   = 2;
	msgs.buf   = buf;

	if (i2c_transfer(audext_i2c_info->iic_adapter, &msgs, 1) != 1) {
		printk("%s fail: i2c_transfer not OK \n", __FUNCTION__);
	}

#else
	I2C_DATA    i2c_data;
	I2C_BYTE    i2c_byte[4];

	i2c_data.VersionInfo     = DRV_VER_96680;
	i2c_data.pByte           = i2c_byte;
	i2c_data.ByteCount       = I2C_BYTE_CNT_3;

	i2c_byte[0].uiValue      = (0x3B<<1);
	i2c_byte[0].Param        = I2C_BYTE_PARAM_START;
	i2c_byte[1].uiValue      = addr & 0xFF;
	i2c_byte[1].Param        = I2C_BYTE_PARAM_NONE;
	i2c_byte[2].uiValue      = value & 0xFF;
	i2c_byte[2].Param        = I2C_BYTE_PARAM_STOP;

	p_i2cobj_ac108->lock(i2csess_ac108);

	if (p_i2cobj_ac108->transmit(&i2c_data) != I2C_STS_OK) {
		p_i2cobj_ac108->unlock(i2csess_ac108);
		DBG_DUMP("i2c transmit err 1!\r\n");
		return;
	}

	p_i2cobj_ac108->unlock(i2csess_ac108);
#endif
}

static UINT32 audext_ac108_read(UINT32 addr)
{
#ifdef __KERNEL__
		struct i2c_msg  msgs[2];
		unsigned char   buf[1], buf2[1];

		buf[0]        = addr & 0xFF;
		msgs[0].addr  = I2C_ADDR;
		msgs[0].flags = 0;//w
		msgs[0].len   = 1;
		msgs[0].buf   = buf;

		buf2[0]       = 0;
		msgs[1].addr  = I2C_ADDR;
		msgs[1].flags = 1;//r
		msgs[1].len   = 1;
		msgs[1].buf   = buf2;

		if (i2c_transfer(audext_i2c_info->iic_adapter, msgs, 2) != 2) {
			printk("%s fail: i2c_transfer not OK \n", __FUNCTION__);
		}

		return (UINT32)buf2[0];
#else
	I2C_DATA    i2c_data;
	I2C_BYTE    i2c_byte[4];

	i2c_data.VersionInfo     = DRV_VER_96680;
	i2c_data.pByte           = i2c_byte;
	i2c_data.ByteCount       = I2C_BYTE_CNT_2;

	i2c_byte[0].uiValue      = (0x3B<<1);
	i2c_byte[0].Param        = I2C_BYTE_PARAM_START;
	i2c_byte[1].uiValue      = addr & 0xFF;
	i2c_byte[1].Param        = I2C_BYTE_PARAM_NONE;

	p_i2cobj_ac108->lock(i2csess_ac108);

	if (p_i2cobj_ac108->transmit(&i2c_data) != I2C_STS_OK) {
		p_i2cobj_ac108->unlock(i2csess_ac108);
		DBG_DUMP("i2c transmit err 1!\r\n");
		return 0;
	}

	i2c_data.ByteCount       = I2C_BYTE_CNT_1;

	i2c_byte[0].uiValue      = (0x3B<<1) + 1;
	i2c_byte[0].Param        = I2C_BYTE_PARAM_START;
	if (p_i2cobj_ac108->transmit(&i2c_data) != I2C_STS_OK) {
		p_i2cobj_ac108->unlock(i2csess_ac108);
		DBG_DUMP("i2c transmit err 2!\r\n");
		return 0;
	}

	i2c_data.ByteCount       = I2C_BYTE_CNT_1;
	i2c_byte[0].Param		 = I2C_BYTE_PARAM_NACK | I2C_BYTE_PARAM_STOP;
	p_i2cobj_ac108->receive(&i2c_data);

	p_i2cobj_ac108->unlock(i2csess_ac108);

	return i2c_byte[0].uiValue;
#endif
}

static void audext_ac108_update(UINT32 addr, UINT32 mask, UINT32 value)
{
	UINT32 temp;

	temp = audext_ac108_read(addr);
	temp &= (~mask);
	audext_ac108_write(addr, temp|(value&mask));
}
#endif

void aud_ext_ac108_get_fp(PAUDIO_CODEC_FUNC p_aud_codec_func)
{
	memcpy((void *)p_aud_codec_func, (const void *)&ext_emu_codec_fp, sizeof(AUDIO_CODEC_FUNC));
}
static void audcodec_set_device_object(PAUDIO_DEVICE_OBJ p_aud_dev_obj)
{
#ifdef __KERNEL__
	if (i2c_new_device(i2c_get_adapter(p_aud_dev_obj->uiGPIOData), &audext_i2c_device) == NULL) {
		DBG_ERR("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
		return;
	}
	if (i2c_add_driver(&audext_i2c_driver) != 0) {
		DBG_ERR("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
		return;
	}
	DBG_DUMP("add i2c dev done\n");

	audcodec_init(NULL);
#else
	p_i2cobj_ac108 = i2c_getDrvObject(p_aud_dev_obj->uiGPIOData);
	if (p_i2cobj_ac108 == NULL) {
		DBG_DUMP("i2c_getDrvObject failed\r\n");
	}

	if (p_i2cobj_ac108->open(&i2csess_ac108) != E_OK) {
		DBG_DUMP("i2c open failed\r\n");
	}

	p_i2cobj_ac108->setConfig(i2csess_ac108, I2C_CONFIG_ID_PINMUX,       p_aud_dev_obj->uiGPIOClk);
	p_i2cobj_ac108->setConfig(i2csess_ac108, I2C_CONFIG_ID_BUSCLOCK,     I2C_BUS_CLOCK_100KHZ);
	p_i2cobj_ac108->setConfig(i2csess_ac108, I2C_CONFIG_ID_HANDLE_NACK,  TRUE);
#endif
}

static void audcodec_init(PAUDIO_SETTING p_audio_setting)
{
	UINT32 temp, vec;
	UINT32 m1, m2, n, k1, k2;

#ifdef __KERNEL__
	if ((audext_i2c_info == NULL) || ((audext_i2c_info->iic_adapter) == NULL)) {
		DBG_ERR("I2C channel Init Failed\r\n");
		return;
	}
#else
	if ((p_i2cobj_ac108 == NULL) || (i2csess_ac108 == I2C_TOTAL_SESSION)) {
		DBG_ERR("I2C channel Init Failed\r\n");
		return;
	}
#endif

	aud_set_i2s_chbits(AUDIO_I2SCH_BITS_32);

	//reset
	//audext_ac108_write(CHIP_RST, CHIP_RST_VAL);
	//Delay_DelayMs(3);


	#if 1
	// SYSCLK_SRC_PLL
	audext_ac108_update(SYSCLK_CTRL, 0x1 << SYSCLK_SRC, SYSCLK_SRC_PLL << SYSCLK_SRC);
	#else
	// SYSCLK_SRC_MCLK
	audext_ac108_update(SYSCLK_CTRL, 0x1 << SYSCLK_SRC, SYSCLK_SRC_MCLK << SYSCLK_SRC);
	#endif


	/*
		ac108_set_fmt
	*/

	#if 1
	// slave mode
	// 0x30:chip is slave mode, BCLK & LRCK input,enable SDO1_EN and
	//  SDO2_EN, Transmitter Block Enable, Globe Enable
	audext_ac108_update(I2S_CTRL, 0x03 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN,
						  0x00 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN);
	#else
	// master mode
	audext_ac108_update(I2S_CTRL, 0x03 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN,
					  0x00 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN);
	/* multi_chips: only one chip set as Master, and the others also need to set as Slave */
	audext_ac108_update(I2S_CTRL, 0x3 << LRCK_IOEN, 0x3 << LRCK_IOEN);
	#endif


	/* ac108_configure_power */
	//0x06:Enable Analog LDO
	audext_ac108_update(PWR_CTRL6, 0x01 << LDO33ANA_ENABLE, 0x01 << LDO33ANA_ENABLE);
	// 0x07:
	// Control VREF output and micbias voltage ?
	// REF faststart disable, enable Enable VREF (needed for Analog
	// LDO and MICBIAS)
	audext_ac108_update(PWR_CTRL7, 0x1f << VREF_SEL | 0x01 << VREF_FASTSTART_ENABLE | 0x01 << VREF_ENABLE,
					   0x13 << VREF_SEL | 0x00 << VREF_FASTSTART_ENABLE | 0x01 << VREF_ENABLE);
	// 0x09:
	// Disable fast-start circuit on VREFP
	// VREFP_RESCTRL=00=1 MOhm
	// IGEN_TRIM=100=+25%
	// Enable VREFP (needed by all audio input channels)
	audext_ac108_update(PWR_CTRL9, 0x01 << VREFP_FASTSTART_ENABLE | 0x03 << VREFP_RESCTRL | 0x07 << IGEN_TRIM | 0x01 << VREFP_ENABLE,
					   0x00 << VREFP_FASTSTART_ENABLE | 0x00 << VREFP_RESCTRL | 0x04 << IGEN_TRIM | 0x01 << VREFP_ENABLE);


	//0x31: 0: normal mode, negative edge drive and positive edge sample
	//1: invert mode, positive edge drive and negative edge sample
	audext_ac108_update(I2S_BCLK_CTRL,  0x01 << BCLK_POLARITY, BCLK_NORMAL_DRIVE_N_SAMPLE_P << BCLK_POLARITY);
	// 0x32: same as 0x31
	audext_ac108_update(I2S_LRCK_CTRL1, 0x01 << LRCK_POLARITY, LRCK_LEFT_HIGH_RIGHT_LOW << LRCK_POLARITY);

	// 0x34:Encoding Mode Selection,Mode
	// Selection,data is offset by 1 BCLKs to LRCK
	// normal mode for the last half cycle of BCLK in the slot ?
	// turn to hi-z state (TDM) when not transferring slot ?
	audext_ac108_update(I2S_FMT_CTRL1,	0x01 << ENCD_SEL | 0x03 << MODE_SEL | 0x01 << TX2_OFFSET |
						0x01 << TX1_OFFSET | 0x01 << TX_SLOT_HIZ | 0x01 << TX_STATE,
								0 << ENCD_SEL	|
								LEFT_JUSTIFIED_FORMAT << MODE_SEL	|
								1 << TX2_OFFSET	|
								1 << TX1_OFFSET	|
								0x00 << TX_SLOT_HIZ	|
								0x01 << TX_STATE);

	// 0x60:
	// MSB / LSB First Select: This driver only support MSB First Select .
	// OUT2_MUTE,OUT1_MUTE shoule be set in widget.
	// LRCK = 1 BCLK width
	// Linear PCM
	//  TODO:pcm mode, bit[0:1] and bit[2] is special
	audext_ac108_update(I2S_FMT_CTRL3,	0x01 << TX_MLS | 0x03 << SEXT  | 0x01 << LRCK_WIDTH | 0x03 << TX_PDM,
						0x00 << TX_MLS | 0x03 << SEXT  | 0x00 << LRCK_WIDTH | 0x00 << TX_PDM);

	audext_ac108_write(HPF_EN, 0x00);

	m1 = 4;
	m2 = 0;
	n  = 128;
	k1 = 24;
	k2 = 0;

	/*
		ac108_hw_params
	*/

	// TDM mode or normal mode
	audext_ac108_write(I2S_LRCK_CTRL2, 128 - 1);
	audext_ac108_update(I2S_LRCK_CTRL1, 0x03 << 0, 0x00);

	/**
	 * 0x35:
	 * TX Encoding mode will add  4bits to mark channel number
	 * TODO: need a chat to explain this
	 */
	audext_ac108_update(I2S_FMT_CTRL2, 0x07 << SAMPLE_RESOLUTION | 0x07 << SLOT_WIDTH_SEL,
						7 << SAMPLE_RESOLUTION
							| 7 << SLOT_WIDTH_SEL);

	/**
	 * 0x60:
	 * ADC Sample Rate synchronised with I2S1 clock zone
	 */
	audext_ac108_update(ADC_SPRC, 0x0f << ADC_FS_I2S1, 8 << ADC_FS_I2S1);
	audext_ac108_write(HPF_EN, 0x0F);

	/*ac108_config_pll(ac10x, ac108_sample_rate[rate].real_val, 0); */
	/* 0x11,0x12,0x13,0x14: Config PLL DIV param M1/M2/N/K1/K2 */
	audext_ac108_update(PLL_CTRL5, 0x1f << PLL_POSTDIV1 | 0x01 << PLL_POSTDIV2,
					   k1 << PLL_POSTDIV1 | k2 << PLL_POSTDIV2);
	audext_ac108_update(PLL_CTRL4, 0xff << PLL_LOOPDIV_LSB, (unsigned char)n << PLL_LOOPDIV_LSB);
	audext_ac108_update(PLL_CTRL3, 0x03 << PLL_LOOPDIV_MSB, (n >> 8) << PLL_LOOPDIV_MSB);
	audext_ac108_update(PLL_CTRL2, 0x1f << PLL_PREDIV1 | 0x01 << PLL_PREDIV2,
					    m1 << PLL_PREDIV1 | m2 << PLL_PREDIV2);

	/*0x18: PLL clk lock enable*/
	audext_ac108_update(PLL_LOCK_CTRL, 0x1 << PLL_LOCK_EN, 0x1 << PLL_LOCK_EN);

	/*0x10: PLL Common voltage Enable, PLL Enable,PLL loop divider factor detection enable*/
	audext_ac108_update(PLL_CTRL1, 0x01 << PLL_EN | 0x01 << PLL_COM_EN | 0x01 << PLL_NDET,
					   0x01 << PLL_EN | 0x01 << PLL_COM_EN | 0x01 << PLL_NDET);

	/**
	 * 0x20: enable pll, pll source from mclk/bclk, sysclk source from pll, enable sysclk
	 */
	audext_ac108_update(SYSCLK_CTRL, 0x01 << PLLCLK_EN | 0x03  << PLLCLK_SRC | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN,
					     0x01 << PLLCLK_EN |0<< PLLCLK_SRC | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN);

	audext_ac108_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 2 << BCLKDIV);

	/*
		ac108_multi_chips_slots
	*/
	vec = 0xFUL;
	//audext_ac108_write(I2S_TX1_CTRL1, 8 - 1);
	audext_ac108_write(I2S_TX1_CTRL1, 4 - 1);
	audext_ac108_write(I2S_TX1_CTRL2, (vec >> 0) & 0xFF);
	audext_ac108_write(I2S_TX1_CTRL3, (vec >> 8) & 0xFF);

#if 0
	vec = (0x2 << 0 | 0x3 << 2 | 0x0 << 4 | 0x1 << 6);
#else
	DBG_DUMP("AC108 Set to 2CH Mode\r\n");
	//2CH TEST
	vec = (0x2 << 0 | 0x0 << 2 | 0x3 << 4 | 0x1 << 6);
#endif
	audext_ac108_write(I2S_TX1_CHMP_CTRL1, (vec >>  0) & 0xFF);
	audext_ac108_write(I2S_TX1_CHMP_CTRL2, (vec >>  8) & 0xFF);
	audext_ac108_write(I2S_TX1_CHMP_CTRL3, (vec >> 16) & 0xFF);
	audext_ac108_write(I2S_TX1_CHMP_CTRL4, (vec >> 24) & 0xFF);

	/* Digital gain Default value */
	audext_ac108_write(ADC1_DVOL_CTRL, 0xA7);//CH3
	audext_ac108_write(ADC2_DVOL_CTRL, 0xA7);//CH2
	audext_ac108_write(ADC3_DVOL_CTRL, 0xA7);//CH1
	audext_ac108_write(ADC4_DVOL_CTRL, 0x9C);//CH0
	//audext_ac108_write(ADC1_DVOL_CTRL, 0x98);//CH3
	//audext_ac108_write(ADC2_DVOL_CTRL, 0x98);//CH2
	//audext_ac108_write(ADC3_DVOL_CTRL, 0x98);//CH1
	//audext_ac108_write(ADC4_DVOL_CTRL, 0x98);//CH0


	/* PGA gain Default value */
	audext_ac108_write(ANA_PGA1_CTRL, 0x00<<1);
	audext_ac108_write(ANA_PGA2_CTRL, 0x00<<1);
	audext_ac108_write(ANA_PGA3_CTRL, 0x00<<1);
	audext_ac108_write(ANA_PGA4_CTRL, 0x00<<1);

	// MIC-BIAS ON
	audext_ac108_update(ANA_ADC1_CTRL1, 0x07 << ADC1_MICBIAS_EN,  0x07 << ADC1_MICBIAS_EN);
	audext_ac108_update(ANA_ADC2_CTRL1, 0x07 << ADC2_MICBIAS_EN,  0x07 << ADC2_MICBIAS_EN);
	audext_ac108_update(ANA_ADC3_CTRL1, 0x07 << ADC3_MICBIAS_EN,  0x07 << ADC3_MICBIAS_EN);
	audext_ac108_update(ANA_ADC4_CTRL1, 0x07 << ADC4_MICBIAS_EN,  0x07 << ADC4_MICBIAS_EN);

	audext_ac108_write(ADC_DIG_EN,		0x1F);
	audext_ac108_write(ANA_ADC4_CTRL7,	0x0F);
	audext_ac108_write(ANA_ADC4_CTRL6,	0x20);

	/*
		ac108_trigger
	*/
	temp = audext_ac108_read(I2S_CTRL);
	if ((temp & (0x02 << LRCK_IOEN)) && (temp & (0x01 << LRCK_IOEN)) == 0) {
		/* disable global clock */
		audext_ac108_update(I2S_CTRL, 0x1 << TXEN | 0x1 << GEN, 0x1 << TXEN | 0x0 << GEN);
	}
	/*0x21: Module clock enable<I2S, ADC digital, MIC offset Calibration, ADC analog>*/
	audext_ac108_write(MOD_CLK_EN, 1 << _I2S | 1 << ADC_DIGITAL | 1 << MIC_OFFSET_CALIBRATION | 1 << ADC_ANALOG);
	/*0x22: Module reset de-asserted<I2S, ADC digital, MIC offset Calibration, ADC analog>*/
	audext_ac108_write(MOD_RST_CTRL, 1 << _I2S | 1 << ADC_DIGITAL | 1 << MIC_OFFSET_CALIBRATION | 1 << ADC_ANALOG);

	// AC108 as Master!!
	audext_ac108_write(I2S_CTRL, audext_ac108_read(I2S_CTRL)|0xC0);

	//audext_ac108_write(0x20, 0x89);
	//audext_ac108_write(0x38, 0x03);
	//audext_ac108_write(0x60, 0x03);//*****not set!

	#if 0
	{
		UINT32 i;

		debug_msg("\r\n      0x0  0x1  0x2  0x3  0x4  0x5  0x6  0x7  0x8  0x9  0xA  0xB  0xC  0xD  0xE  0xF");
		for (i = 0; i < 256; i++) {
			if (!(i&0xF))
				debug_msg("\r\n0x%02X ", i);
			debug_msg("0x%02X ", audext_ac108_read(i));
		}
		debug_msg("\r\n");
	}

	#endif

}

static void audcodec_set_record_source(AUDIO_RECSRC rec_source)
{}
static void audcodec_set_output(AUDIO_OUTPUT output)
{}
static void audcodec_set_samplerate(AUDIO_SR sample_rate)
{

	if(sample_rate == AUDIO_SR_48000) {
		audext_ac108_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 2 << BCLKDIV);
	} else if (sample_rate == AUDIO_SR_16000) {
		audext_ac108_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 4 << BCLKDIV);
	} else if (sample_rate == AUDIO_SR_8000) {
		audext_ac108_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 6 << BCLKDIV);
	} else if (sample_rate == AUDIO_SR_24000) {
		audext_ac108_update(I2S_BCLK_CTRL, 0x0F << BCLKDIV, 3 << BCLKDIV);
	}

}
static void audcodec_set_channel(AUDIO_CH channel)
{}

static void audcodec_set_gain(AUDIO_GAIN gain)
{
	UINT32 value;

	//debug_msg("audcodec_set_gain %d\r\n",gain);

	if (gain == AUDIO_GAIN_MUTE) {
		value = 0;

		audext_ac108_write(ADC1_DVOL_CTRL, 0x00);
		audext_ac108_write(ADC2_DVOL_CTRL, 0x00);
		audext_ac108_write(ADC3_DVOL_CTRL, 0x00);
		audext_ac108_write(ADC4_DVOL_CTRL, 0x00);

		audext_ac108_write(ANA_PGA1_CTRL, value<<1);
		audext_ac108_write(ANA_PGA2_CTRL, value<<1);
		audext_ac108_write(ANA_PGA3_CTRL, value<<1);
		audext_ac108_write(ANA_PGA4_CTRL, value<<1);

	} else {

		value = ((gain - AUDIO_GAIN_0)<<2)+3;

		audext_ac108_write(ADC1_DVOL_CTRL, digital_gain);
		audext_ac108_write(ADC2_DVOL_CTRL, digital_gain);
		audext_ac108_write(ADC3_DVOL_CTRL, digital_gain);
		audext_ac108_write(ADC4_DVOL_CTRL, digital_gain);

		/* PGA gain Max 0x1F */
		audext_ac108_write(ANA_PGA1_CTRL, value<<1); // Our Ch3

		audext_ac108_write(ANA_PGA3_CTRL, value<<1); // Our CH1

		if (ac108_gain_balance) {
			//making volume balance
			if (value > 6)
				value -= 6;
			else
				value = 0;
		}
		audext_ac108_write(ANA_PGA4_CTRL, value<<1); // Our CH0
		audext_ac108_write(ANA_PGA2_CTRL, value<<1); // Our CH2
	}

}
static void audcodec_set_gaindb(INT32 db)
{
	UINT32 value;

	DBG_DUMP("audcodec_set_gaindb %d\r\n", db);
	if (db > 31)
		db = 31;
	else if (db < 0)
		db = 0;
	value = (UINT32)db;

	/* PGA gain Max 0x1F */
	audext_ac108_write(ANA_PGA1_CTRL, value<<1); // Our Ch3
	audext_ac108_write(ANA_PGA3_CTRL, value<<1); // Our CH1

	if (ac108_gain_balance) {
		//making volume balance
		if (value > 6)
			value -= 6;
		else
			value = 0;
	}
	audext_ac108_write(ANA_PGA4_CTRL, value<<1); // Our CH0
	audext_ac108_write(ANA_PGA2_CTRL, value<<1); // Our CH2

}

static void audcodec_set_effect(AUDIO_EFFECT effect)
{}
static BOOL audcodec_set_feature(AUDIO_FEATURE feature, BOOL en)
{
	//if(feature == AUDIO_FEATURE_MICBOOST) {
	//	ac108_gain_balance = en;
	//	DBG_DUMP("set Pga gain balance %s\r\n",ac108_gain_balance?"ENABLE":"DISABLE");
	//}

	return TRUE;
}
static void audcodec_stop_record(void)
{}
static void audcodec_record(void)
{}
static void audcodec_set_format(AUDIO_I2SFMT i2s_fmt)
{}
static void audcodec_set_clock_ratio(AUDIO_I2SCLKR i2s_clk_ratio)
{}
static BOOL audcodec_send_command(UINT32 reg, UINT32 data)
{
	audext_ac108_write(reg, data);
	return TRUE;
}
static BOOL audcodec_read_data(UINT32 reg, UINT32 *p_data)
{
	*p_data = audext_ac108_read(reg);
	return TRUE;
}
static BOOL audcodec_set_parameter(AUDIO_PARAMETER parameter, UINT32 setting)
{
	if (parameter == AUDIO_PARAMETER_RECORD_DIGITAL_GAIN) {
		digital_gain = setting;
		audext_ac108_write(ADC1_DVOL_CTRL, digital_gain);
		audext_ac108_write(ADC2_DVOL_CTRL, digital_gain);
		audext_ac108_write(ADC3_DVOL_CTRL, digital_gain);
		audext_ac108_write(ADC4_DVOL_CTRL, digital_gain);
		DBG_DUMP("SetDGAIN=0x%02X\r\n", digital_gain);
	}


	return TRUE;
}
static BOOL audcodec_chk_samplerate(void)
{
	return TRUE;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(aud_ext_ac108_get_fp);
#endif

//@}
