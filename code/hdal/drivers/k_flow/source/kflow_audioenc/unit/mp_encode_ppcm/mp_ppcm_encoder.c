/*
    Media Plugin - Packed-PCM Encoder.

    This file is the media plugin library - Packed-PCM encoder.

    @file       mp_ppcm_encoder.c
    @date       2018/08/30

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifdef __KERNEL__
#include "audio_encode.h"
#include "mp_ppcm_encoder.h"
#include "kwrap/error_no.h"
#include <linux/string.h>
#else
#include "audio_encode.h"
#include "mp_ppcm_encoder.h"
#include "kwrap/error_no.h"
#include <string.h>
#endif

#define __MODULE__          ppcm_enc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int ppcm_enc_debug_level = NVT_DBG_WRN;

MP_AUDENC_PARAM  gAudEncParam_PPCM[MP_AUDENC_ID_MAX];

//#define PPCM_ENCODEID 0x5050434D //PPCM
/*MP_AUDENC_ENCODER gPPCMAudioEncObj = {
	MP_PPCMEnc_init,  //Initialize
	MP_PPCMEnc_getInfo,     //GetInfo
	MP_PPCMEnc_setInfo,     //SetInfo
	MP_PPCMEnc_encodeOne,   //EncodeOne
	MP_PPCMEnc_customizeFunc,   //CustomizeFunc
	PPCM_ENCODEID           //checkID
};*/

//ER MP_PPCMEnc_initPPCM(BOOL bHeaderEn);

/**
    Get aac encoding object.

    Get aac encoding object.

    @return audio codec encoding object
*/
/*PMP_AUDENC_ENCODER MP_PPCMEnc_getEncodeObject(void)
{
	PMP_AUDENC_ENCODER pEncObj;


	pEncObj = &gPPCMAudioEncObj;

	return pEncObj;
}*/

ER MP_PPCMEnc_init(MP_AUDENC_ID AudEncId)
{
	return E_OK;
}

ER MP_PPCMEnc_getInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	ER resultV = E_PAR;

	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDENC_GETINFO_PARAM:
		if (p1 == 0) {
			DBG_ERR("MP_PPCMEnc_getInfo error p1 = 0!\r\n");
			return E_SYS;
		}
		*p1 = (UINT32)&gAudEncParam_PPCM[AudEncId];
		resultV = E_OK;
		break;

	case MP_AUDENC_GETINFO_BLOCK:
		if (p1 == 0) {
			DBG_ERR("MP_PPCMEnc_getInfo error p1 = 0!\r\n");
			return E_SYS;
		}
		//the same as chunksize
		gAudEncParam_PPCM[AudEncId].chunksize = MP_AUDENC_AAC_RAW_BLOCK * gAudEncParam_PPCM[AudEncId].nChannels * 2;
		gAudEncParam_PPCM[AudEncId].encOutSize = gAudEncParam_PPCM[AudEncId].chunksize;
		*p1 = gAudEncParam_PPCM[AudEncId].encOutSize;

		resultV = E_OK;
		break;

	default:
		DBG_ERR("PPCMEnc Get Info ERROR! type=%d!\r\n", type);
		break;
	}
	return resultV;
}

ER MP_PPCMEnc_setInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDENC_SETINFO_RAWADDR:
		if (param1 == 0) {
			DBG_ERR("MP_PPCMEnc_setInfo:setinfo AUDRAWADDR ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_PPCM[AudEncId].rawAddr = param1;
		break;

	case MP_AUDENC_SETINFO_SAMPLERATE:
		if ((param1 == 0)) {
			DBG_ERR("MP_PPCMEnc_setInfo:setinfo ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_PPCM[AudEncId].sampleRate = param1;
		break;

	case MP_AUDENC_SETINFO_BITRATE:
		if ((param1 == 0)) {
			DBG_ERR("MP_PPCMEnc_setInfo:setinfo ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_PPCM[AudEncId].bitRate = param1;
		break;

	case MP_AUDENC_SETINFO_CHANNEL:
		if ((param1 == 0) || (param1 > 2)) {
			DBG_ERR("MP_PPCMEnc_setInfo:setinfo ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_PPCM[AudEncId].nChannels = param1;
		break;

	default:
		break;
	}

	return E_OK;
}

ER MP_PPCMEnc_encodeOne(MP_AUDENC_ID AudEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr)
{
	if (AudEncId >= MP_AUDENC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudEncId);
		return E_PAR;
	}

	// encode one frame
#if 1
	memcpy((UINT8 *)outputAddr, (UINT8 *)gAudEncParam_PPCM[AudEncId].rawAddr, gAudEncParam_PPCM[AudEncId].encOutSize);
#else // pure linux not support hwmem_memcpy, Adam 2018/08/30
	hwmem_open();
	hwmem_memcpy(outputAddr, gAudEncParam_PPCM.rawAddr, gAudEncParam_PPCM.encOutSize);
	hwmem_close();
#endif

	ptr->thisSize = gAudEncParam_PPCM[AudEncId].encOutSize;
	ptr->usedRawSize = gAudEncParam_PPCM[AudEncId].encOutSize;//used = input size
	return E_OK;
}

/*ER MP_PPCMEnc_customizeFunc(UINT32 type, void *ptr)
{
	MP_AUDENC_CUSTOM_LAST_INFO *pinfo;
	switch (type) {
	case MP_AUDENC_TYPE_ENCLAST:

		pinfo = (MP_AUDENC_CUSTOM_LAST_INFO *)ptr;

		//DBG_DUMP("srcaddr=0x%x, dst=0x%x, size=0x%x!\r\n", gAudEncParam_PCM.rawAddr, outputAddr, gAudEncParam_PCM.chunksize);
		memcpy((UINT8 *)pinfo->bsOutputAddr, (UINT8 *)pinfo->inputAddr, pinfo->inputSize);
		pinfo->thisSize = pinfo->inputSize;
		pinfo->usedRawSize = pinfo->inputSize;//used = input size
		//DBG_DUMP("PCM size=0x%x!\n", gAudEncParam_PCM.encOutSize);
		break;

	default:
		break;
	}
	return E_OK;
}*/


