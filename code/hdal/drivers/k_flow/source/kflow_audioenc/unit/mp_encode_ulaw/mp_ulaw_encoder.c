/*
    Media Plugin - uLaw Encoder.

    This file is the media plugin library - uLaw encoder.

    @file       mp_ulaw_encoder.c
    @date       2018/08/30

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifdef __KERNEL__
#include "audio_encode.h"
#include "mp_ulaw_encoder.h"
#include "kwrap/error_no.h"
#include "kdrv_audioio/kdrv_audioio.h"
#else
#include "audio_encode.h"
#include "mp_ulaw_encoder.h"
#include "kwrap/error_no.h"
#include "kdrv_audioio/kdrv_audioio.h"
#endif

#define __MODULE__          ulaw_enc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int ulaw_enc_debug_level = NVT_DBG_WRN;

MP_AUDENC_PARAM   gAudEncParam_uLaw[MP_AUDENC_ID_MAX];

static KDRV_AUDIOLIB_FUNC* ulaw_enc_obj = NULL;

//#define ULAW_ENCODEID   0x756c6177 // "ulaw"

/*MP_AUDENC_ENCODER g_uLawAudioEncObj = {
	MP_uLawEnc_init,            //Initialize
	MP_uLawEnc_getInfo,         //GetInfo
	MP_uLawEnc_setInfo,         //SetInfo
	MP_uLawEnc_encodeOne,       //EncodeOne
	MP_uLawEnc_customizeFunc,   //CustomizeFunc
	ULAW_ENCODEID               //checkID
};*/

/**
    Get uLaw encoding object.

    Get uLaw encoding object.

    @return audio codec encoding object
*/
/*PMP_AUDENC_ENCODER MP_uLawEnc_getEncodeObject(void)
{
	return &g_uLawAudioEncObj;
}*/

ER MP_uLawEnc_init(MP_AUDENC_ID AudEncId)
{
	if (ulaw_enc_obj == NULL) {
		ulaw_enc_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_G711);
		if (ulaw_enc_obj == NULL) {
			DBG_ERR("ulaw object is NULL\r\n");
			return E_NOEXS;
		}
	}

	return E_OK;
}

ER MP_uLawEnc_getInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	ER resultV = E_PAR;

	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDENC_GETINFO_PARAM:
		*p1 = (UINT32)&gAudEncParam_uLaw[AudEncId];
		resultV = E_OK;
		break;

	case MP_AUDENC_GETINFO_BLOCK:
		*p1 = MP_AUDENC_AAC_RAW_BLOCK * gAudEncParam_uLaw[AudEncId].nChannels * 2;
		resultV = E_OK;
		break;

	default:
		DBG_ERR("uLawEnc Get Info ERROR! type=%d!\r\n", type);
		break;
	}

	return resultV;
}

ER MP_uLawEnc_setInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
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
		gAudEncParam_uLaw[AudEncId].rawAddr = param1;
		break;

	case MP_AUDENC_SETINFO_SAMPLERATE:
		if (param1 == 0) {
			DBG_ERR("set MP_AUDENC_SETINFO_SAMPLERATE error! p1 = %d\r\n", param1);
			break;
		}
		gAudEncParam_uLaw[AudEncId].sampleRate = param1;
		break;

	case MP_AUDENC_SETINFO_BITRATE:
		if (param1 == 0) {
			DBG_ERR("set MP_AUDENC_SETINFO_BITRATE error! p1 = %d\r\n", param1);
			break;
		}
		gAudEncParam_uLaw[AudEncId].bitRate = param1;
		break;

	case MP_AUDENC_SETINFO_CHANNEL:
		if ((param1 == 0) || (param1 > 2)) {
			DBG_ERR("set MP_AUDENC_SETINFO_CHANNEL error! p1 = %d\r\n", param1);
			break;
		}
		gAudEncParam_uLaw[AudEncId].nChannels = param1;
		break;

	default:
		break;
	}

	return E_OK;
}

ER MP_uLawEnc_encodeOne(MP_AUDENC_ID AudEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr)
{
	ER  ErrCode;

	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	if (ulaw_enc_obj == NULL) {
		DBG_ERR("alaw object is NULL\r\n");
		return E_NOEXS;
	}

	if (ulaw_enc_obj->g711.ulaw_encode == NULL) {
		DBG_ERR("ulaw encode is NULL\r\n");
		ptr->thisSize = 0;
		return E_SYS;
	}

	if ((ErrCode = ulaw_enc_obj->g711.ulaw_encode((INT16 *)ptr->rawAddr, (UINT8 *)outputAddr, (ptr->usedRawSize / 2), FALSE)) != E_OK) {
		DBG_ERR("uLaw encode error: %d\r\n", ErrCode);
		return E_SYS;
	}

	// g711 alg shall output half of size.
	ptr->thisSize = ptr->usedRawSize / 2;

	return E_OK;
}

