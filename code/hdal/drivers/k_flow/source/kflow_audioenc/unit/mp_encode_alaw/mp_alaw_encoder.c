/*
    Media Plugin - aLaw Encoder.

    This file is the media plugin library - aLaw encoder.

    @file       mp_alaw_encoder.c
    @date       2018/08/30

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifdef __KERNEL__
#include "audio_encode.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "mp_alaw_encoder.h"
#include "kwrap/error_no.h"
#else
#include "audio_encode.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "mp_alaw_encoder.h"
#include "kwrap/error_no.h"
#endif

#define __MODULE__          alaw_enc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int alaw_enc_debug_level = NVT_DBG_WRN;

MP_AUDENC_PARAM   gAudEncParam_aLaw[MP_AUDENC_ID_MAX];

static KDRV_AUDIOLIB_FUNC* alaw_enc_obj = NULL;

//#define ALAW_ENCODEID   0x616c6177 // "alaw"

/*MP_AUDENC_ENCODER g_aLawAudioEncObj = {
	MP_aLawEnc_init,            //Initialize
	MP_aLawEnc_getInfo,         //GetInfo
	MP_aLawEnc_setInfo,         //SetInfo
	MP_aLawEnc_encodeOne,       //EncodeOne
	MP_aLawEnc_customizeFunc,   //CustomizeFunc
	ALAW_ENCODEID               //checkID
};*/

/**
    Get aLaw encoding object.

    Get aLaw encoding object.

    @return audio codec encoding object
*/
/*PMP_AUDENC_ENCODER MP_aLawEnc_getEncodeObject(void)
{
	return &g_aLawAudioEncObj;
}*/

ER MP_aLawEnc_init(MP_AUDENC_ID AudEncId)
{
	if (alaw_enc_obj == NULL) {
		alaw_enc_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_G711);
		if (alaw_enc_obj == NULL) {
			DBG_ERR("alaw object is NULL\r\n");
			return E_NOEXS;
		}
	}

	return E_OK;
}

ER MP_aLawEnc_getInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	ER resultV = E_PAR;

	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDENC_GETINFO_PARAM:
		*p1 = (UINT32)&gAudEncParam_aLaw[AudEncId];
		resultV = E_OK;
		break;

	case MP_AUDENC_GETINFO_BLOCK:
		*p1 = MP_AUDENC_AAC_RAW_BLOCK * gAudEncParam_aLaw[AudEncId].nChannels * 2;
		resultV = E_OK;
		break;

	default:
		DBG_ERR("aLawEnc Get Info ERROR! type=%d!\r\n", type);
		break;
	}

	return resultV;
}

ER MP_aLawEnc_setInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDENC_SETINFO_RAWADDR:
		if (param1 == 0) {
			DBG_ERR("set MP_AUDENC_SETINFO_RAWADDR error! p1 = %d\r\n", param1);
			break;
		}
		gAudEncParam_aLaw[AudEncId].rawAddr = param1;
		break;

	case MP_AUDENC_SETINFO_SAMPLERATE:
		if (param1 == 0) {
			DBG_ERR("set MP_AUDENC_SETINFO_SAMPLERATE error! p1 = %d\r\n", param1);
			break;
		}
		gAudEncParam_aLaw[AudEncId].sampleRate = param1;
		break;

	case MP_AUDENC_SETINFO_BITRATE:
		if (param1 == 0) {
			DBG_ERR("set MP_AUDENC_SETINFO_BITRATE error! p1 = %d\r\n", param1);
			break;
		}
		gAudEncParam_aLaw[AudEncId].bitRate = param1;
		break;

	case MP_AUDENC_SETINFO_CHANNEL:
		if ((param1 == 0) || (param1 > 2)) {
			DBG_ERR("set MP_AUDENC_SETINFO_CHANNEL error! p1 = %d\r\n", param1);
			break;
		}
		gAudEncParam_aLaw[AudEncId].nChannels = param1;
		break;

	default:
		break;
	}

	return E_OK;
}

ER MP_aLawEnc_encodeOne(MP_AUDENC_ID AudEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr)
{
	ER  ErrCode;

	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	if (alaw_enc_obj == NULL) {
		DBG_ERR("alaw object is NULL\r\n");
		return E_NOEXS;
	}

	if (alaw_enc_obj->g711.alaw_encode == NULL) {
		DBG_ERR("alaw encode is NULL\r\n");
		ptr->thisSize = 0;
		return E_SYS;
	}

	if ((ErrCode = alaw_enc_obj->g711.alaw_encode((INT16 *)ptr->rawAddr, (UINT8 *)outputAddr, (ptr->usedRawSize / 2), FALSE)) != E_OK) {
		DBG_ERR("aLaw encode error: %d\r\n", ErrCode);
		return E_SYS;
	}

	// g711 alg shall output half of size.
	ptr->thisSize = ptr->usedRawSize / 2;

	return E_OK;
}

