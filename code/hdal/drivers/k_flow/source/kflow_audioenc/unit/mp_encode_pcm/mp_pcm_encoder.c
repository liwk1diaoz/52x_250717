/*
    Media Plugin - PCM encoder.

    This file is the media plugin library - pcm encoder.

    @file       mp_pcm_encoder.c
    @date       2018/08/30

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifdef __KERNEL__
#include "audio_encode.h"
#include "mp_pcm_encoder.h"
#include "kwrap/error_no.h"
#include <linux/string.h>
#else
#include "audio_encode.h"
#include "mp_pcm_encoder.h"
#include "kwrap/error_no.h"
#include <string.h>
#endif

#define __MODULE__          pcm_enc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int pcm_enc_debug_level = NVT_DBG_WRN;

//=====================================================================

MP_AUDENC_PARAM  gAudEncParam_PCM[MP_AUDENC_ID_MAX];

//#define PCM_ENCODEID 0x736f7774

/*MP_AUDENC_ENCODER gPCMAudioEncObj = {
	MP_PCMEnc_init,  //Initialize
	MP_PCMEnc_getInfo,     //GetInfo
	MP_PCMEnc_setInfo,     //SetInfo
	MP_PCMEnc_encodeOne,   //EncodeOne
	MP_PCMEnc_customizeFunc, //CustomizeFunc
	PCM_ENCODEID           //checkID
};*/


/**
    Get pcm encoding object.

    Get pcm encoding object.

    @return audio codec encoding object
*/
/*PMP_AUDENC_ENCODER MP_PCMEnc_getEncodeObject(void)
{
	PMP_AUDENC_ENCODER pEncObj;

	pEncObj = &gPCMAudioEncObj;

	return pEncObj;
}*/

ER MP_PCMEnc_init(MP_AUDENC_ID AudEncId)
{
	return E_OK;
}

ER MP_PCMEnc_getInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	ER resultV = E_PAR;
	//UINT8 p1outsize=0, p2outsize=0, p3outsize=0;

	switch (type) {
	case MP_AUDENC_GETINFO_PARAM:
		if (p1 == 0) {
			DBG_ERR("MP_PCMEnc_getInfo error p1 = 0!\r\n");
			return E_SYS;
		}
		*p1 = (UINT32)&gAudEncParam_PCM[AudEncId];
		resultV = E_OK;
		break;

	case MP_AUDENC_GETINFO_BLOCK:
		if (p1 == 0) {
			DBG_ERR("MP_PCMEnc_getInfo error p1 = 0!\r\n");
			return E_SYS;
		}
		//the same as chunksize
		gAudEncParam_PCM[AudEncId].chunksize = MP_AUDENC_AAC_RAW_BLOCK * gAudEncParam_PCM[AudEncId].nChannels * 2;
		//#NT#2012/08/30#Meg Lin -begin
		gAudEncParam_PCM[AudEncId].encOutSize = gAudEncParam_PCM[AudEncId].chunksize;
		//#NT#2012/08/30#Meg Lin -end

		*p1 = gAudEncParam_PCM[AudEncId].encOutSize;
		resultV = E_OK;
		break;

	default:
		DBG_ERR("PCMEnc Get Info ERROR! type=%d!\r\n", type);
		break;
	}

	return resultV;
}

ER MP_PCMEnc_setInfo(MP_AUDENC_ID AudEncId, MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	switch (type) {
	case MP_AUDENC_SETINFO_RAWADDR:
		if (param1 == 0) {
			DBG_ERR("setinfo AUDRAWADDR ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_PCM[AudEncId].rawAddr = param1;
		break;

	case MP_AUDENC_SETINFO_SAMPLERATE:
		if ((param1 == 0)) {
			DBG_ERR("setinfo ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_PCM[AudEncId].sampleRate = param1;
		break;

	case MP_AUDENC_SETINFO_BITRATE:
		if ((param1 == 0)) {
			DBG_ERR("setinfo ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_PCM[AudEncId].bitRate = param1;
		break;

	case MP_AUDENC_SETINFO_CHANNEL:
		if ((param1 == 0) || (param1 > 2)) {
			DBG_ERR("setinfo ERROR! type=%d, p1=0x%x!\r\n", type, param1);
			break;
		}
		gAudEncParam_PCM[AudEncId].nChannels = param1;
		break;

	default:
		break;
	}

	return E_OK;
}

ER MP_PCMEnc_encodeOne(MP_AUDENC_ID AudEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr)
{
	//DBG_IND("inaddr=0x%x, out=0x%x, size=0x%x!\r\n", gAudEncParam_PCM.rawAddr, outputAddr, gAudEncParam_PCM.chunksize);
#if 1
	memcpy((UINT8 *)outputAddr, (UINT8 *)gAudEncParam_PCM[AudEncId].rawAddr, gAudEncParam_PCM[AudEncId].encOutSize);
#else // pure linux not support hwmem_memcpy, Adam 2018/08/30
	hwmem_open();
	hwmem_memcpy(outputAddr, gAudEncParam_PCM.rawAddr, gAudEncParam_PCM.encOutSize);
	hwmem_close();
#endif

	ptr->thisSize = gAudEncParam_PCM[AudEncId].encOutSize;
	ptr->usedRawSize = gAudEncParam_PCM[AudEncId].encOutSize;//used = input size
	//DBG_IND("PCM size=0x%x!\n", gAudEncParam_PCM.encOutSize);

	return E_OK;
}

/*ER MP_PCMEnc_customizeFunc(UINT32 type, void *ptr)//2012/07/09 Meg
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

