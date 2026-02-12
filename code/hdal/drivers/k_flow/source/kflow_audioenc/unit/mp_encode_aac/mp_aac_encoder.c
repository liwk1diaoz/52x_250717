/*
    Media Plugin - AAC Encoder.

    This file is the media plugin library - aac encoder.

    @file       mp_aac_encoder.c
    @date       2018/08/30

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifdef __KERNEL__
#include "audio_encode.h"
#include "audio_common/include/Audio.h"
#include "mp_aac_encoder.h"
#include "kwrap/error_no.h"
#include "kdrv_audioio/kdrv_audioio.h"
#else
#include "audio_encode.h"
#include "audio_common/include/Audio.h"
#include "mp_aac_encoder.h"
#include "kwrap/error_no.h"
#include "kdrv_audioio/kdrv_audioio.h"
#endif

#define __MODULE__          aac_enc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int aac_enc_debug_level = NVT_DBG_WRN;

//#define AAC_ENCODEID 0xFFF96C40

MP_AUDENC_PARAM  gAudEncParam_AAC[MP_AUDENC_ID_MAX];

static KDRV_AUDIOLIB_FUNC* aac_enc_obj = NULL;

/*MP_AUDENC_ENCODER gAACAudioEncObj = {
	MP_AACEnc_init,        //Initialize
	MP_AACEnc_getInfo,     //GetInfo
	MP_AACEnc_setInfo,     //SetInfo
	MP_AACEnc_encodeOne,   //EncodeOne
	NULL,
	AAC_ENCODEID           //checkID
};*/

ER MP_AACEnc_initAAC(MP_AUDENC_ID AudEncId, BOOL bHeaderEn);

ER MP_AACEnc_init(MP_AUDENC_ID AudEncId)
{
	if (aac_enc_obj == NULL) {
		aac_enc_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_AAC);
		if (aac_enc_obj == NULL) {
			DBG_ERR("AAC object is NULL\r\n");
			return E_NOEXS;
		}
	}

	return E_OK;
}

ER MP_AACEnc_getInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	ER resultV = E_PAR;

	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDENC_GETINFO_PARAM:
		*p1 = (UINT32)&gAudEncParam_AAC[AudEncId];
		resultV = E_OK;
		break;

	case MP_AUDENC_GETINFO_BLOCK:
		*p1 = MP_AUDENC_AAC_RAW_BLOCK * gAudEncParam_AAC[AudEncId].nChannels * 2;
		resultV = E_OK;
		break;

	default:
		DBG_ERR("AACEnc Get Info ERROR! type=%d!\r\n", type);
		break;
	}

	return resultV;
}

ER MP_AACEnc_setInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDENC_SETINFO_RAWADDR:
		if (param1 == 0) {
			DBG_ERR("MP_AACEnc_setInfo:setinfo AUDRAWADDR ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_AAC[AudEncId].rawAddr = param1;
		break;

	case MP_AUDENC_SETINFO_SAMPLERATE:
		if ((param1 == 0)) {
			DBG_ERR("MP_AACEnc_setInfo:setinfo SAMPLERATE ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_AAC[AudEncId].sampleRate = param1;
		break;

	case MP_AUDENC_SETINFO_BITRATE:
		if ((param1 == 0)) {
			DBG_ERR("MP_AACEnc_setInfo:setinfo BITRATE ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_AAC[AudEncId].bitRate = param1;
		break;

	case MP_AUDENC_SETINFO_CHANNEL:
		if ((param1 == 0) || (param1 > 2)) {
			DBG_ERR("MP_AACEnc_setInfo:setinfo CHANNEL ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_AAC[AudEncId].nChannels = param1;
		break;

	case MP_AUDENC_SETINFO_AAC_SET_CFG:
		MP_AACEnc_initAAC(AudEncId, param1);
		break;

	case MP_AUDENC_SETINFO_AAC_STOP_FREQ:
		gAudEncParam_AAC[AudEncId].stopFreq = param1;
		break;

	default:
		break;
	}

	return E_OK;
}

ER MP_AACEnc_initAAC(MP_AUDENC_ID AudEncId, BOOL bHeaderEn)
{
	AUDLIB_AAC_CFG aac_config;
	INT32          err_code;

	if (aac_enc_obj == NULL) {
		DBG_ERR("AAC object is NULL\r\n");
		return E_NOEXS;
	}

	DBG_IND("inin AAC: chs=%d, sr=%d, br=%d, h_en=%d, stop_f=%d\r\n",
			gAudEncParam_AAC[AudEncId].nChannels, gAudEncParam_AAC[AudEncId].sampleRate, gAudEncParam_AAC[AudEncId].bitRate, bHeaderEn, gAudEncParam_AAC[AudEncId].stopFreq);

	// audio channel number
	aac_config.channel_number = gAudEncParam_AAC[AudEncId].nChannels;

	// audio sampling rate
	aac_config.sample_rate = gAudEncParam_AAC[AudEncId].sampleRate;

	// target bit rate
	aac_config.encode_bit_rate = gAudEncParam_AAC[AudEncId].bitRate;

	// with/without AAC header output (1:with, 0:without)
	aac_config.header_enable = bHeaderEn;

	// stop frequency
	aac_config.encode_stop_freq = gAudEncParam_AAC[AudEncId].stopFreq;

	// config AAC encoder
	if (aac_enc_obj->aac.encode_init) {
		if ((err_code = aac_enc_obj->aac.encode_init(&aac_config)) != 0) {
			DBG_ERR("aac encoder config error: %d\r\n", err_code);
			return E_SYS;
		}
	} else {
		DBG_ERR("aac encoder init is NULL\r\n");
		return E_SYS;
	}

	return E_OK;
}

ER MP_AACEnc_encodeOne(MP_AUDENC_ID AudEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr)
{
	AUDLIB_AAC_BUFINFO  buf_info;
	AUDLIB_AACE_RTNINFO rtn_info;
	INT32               err_code;

	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	if (aac_enc_obj == NULL) {
		DBG_ERR("AAC object is NULL\r\n");
		return E_NOEXS;
	}

	if (aac_enc_obj->aac.encode_one_frame == NULL) {
		DBG_ERR("aac decode one is NULL\r\n");
		ptr->thisSize = 0;
		return E_SYS;
	}

	// whatever encode OK or not, update used audio raw data size
	ptr->usedRawSize = MP_AUDENC_AAC_RAW_BLOCK * gAudEncParam_AAC[AudEncId].nChannels * 2;

	// init AAC encoder buffer info
	buf_info.bitstram_buffer_length = MP_AUDENC_AAC_RAW_BLOCK * gAudEncParam_AAC[AudEncId].nChannels;
	buf_info.bitstram_buffer_in     = ptr->rawAddr;
	buf_info.bitstram_buffer_out    = outputAddr;

	DBG_MSG("encode buf_info, len = 0x%x, in_addr = 0x%x, out_addr = 0x%x\r\n", buf_info.bitstram_buffer_length, buf_info.bitstram_buffer_in, buf_info.bitstram_buffer_out);

	// encode one frame
	if ((err_code = aac_enc_obj->aac.encode_one_frame(&buf_info, &rtn_info)) != 0) {
		DBG_ERR("aac encode error: %d\r\n", err_code);
		return E_SYS;
	}

	DBG_MSG("aace: outsize=0x%x\r\n", rtn_info.output_size);

	ptr->thisSize = rtn_info.output_size;

	return E_OK;
}

