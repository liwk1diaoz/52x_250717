/*
    Media Plugin - aLaw Decoder.

    This file is the media plugin library - aLaw decoder.

    @file       MP_aLawDecoder.c
    @ingroup    mIMPDEC
    @note       Nothing.
    @version    V1.00.000
    @date       2015/08/17

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/
#ifdef __KERNEL__
#include "audio_decode.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "mp_alaw_decoder.h"
#include "kwrap/error_no.h"
#include <linux/string.h>
#else
#include "audio_decode.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "mp_alaw_decoder.h"
#include "kwrap/error_no.h"
#include <string.h>
#endif

#define __MODULE__          alaw_dec
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int alaw_dec_debug_level = NVT_DBG_WRN;

//#define ALAW_DECODEID   0x616c6177 // "alaw"

UINT32          g_aLawOutputRawAddr[MP_AUDDEC_ID_MAX] = {0}, g_aLawInBSAddr[MP_AUDDEC_ID_MAX] = {0}, g_aLawInBSSize[MP_AUDDEC_ID_MAX] = {0};
AUDIO_PLAYINFO  g_aLawAudPlayInfo[MP_AUDDEC_ID_MAX] = {0};
ALAW_ADDRINFO   g_aLawAddrInfo[MP_AUDDEC_ID_MAX] = {0};
UINT8           g_aLawFakeDecode[MP_AUDDEC_ID_MAX] = {0};
BOOL            gbMp_aLawDec_RawOutMode[MP_AUDDEC_ID_MAX] = {0};
UINT32          guiMp_aLawDec_OutRawDataAddr[MP_AUDDEC_ID_MAX] = {0};

static KDRV_AUDIOLIB_FUNC* alaw_dec_obj = NULL;

/*MP_AUDDEC_DECODER g_aLawAudDecObj = {
	MP_aLawDec_initialize,  //Initialize
	MP_aLawDec_getInfo,     //GetInfo
	MP_aLawDec_setInfo,     //SetInfo
	MP_aLawDec_decodeOne,   //DecodeOne
	MP_aLawDec_waitDecodeDone,//WaitDecodeDone
	NULL, //CustomizeFunc
	ALAW_DECODEID           //checkID
};*/

/**
    Get aac decoding object.

    Get aac decoding object.

    @return audio codec decoding object
*/
/*PMP_AUDDEC_DECODER MP_aLawDec_getDecodeObject(void)
{
	return &g_aLawAudDecObj;
}*/

ER MP_aLawDec_init(MP_AUDDEC_ID AudDecId, AUDIO_PLAYINFO *pobj)
{
	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	if (alaw_dec_obj == NULL) {
		alaw_dec_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_G711);
		if (alaw_dec_obj == NULL) {
			DBG_ERR("alaw object is NULL\r\n");
			return E_NOEXS;
		}
	}

	g_aLawOutputRawAddr[AudDecId] = 0;
	g_aLawFakeDecode[AudDecId] = 0;
	g_aLawAudPlayInfo[AudDecId].sampleRate = pobj->sampleRate;
	g_aLawAudPlayInfo[AudDecId].channels   = pobj->channels;
	g_aLawAudPlayInfo[AudDecId].chunksize  = (pobj->sampleRate) * (pobj->channels) * 2;
	return E_OK;
}

ER MP_aLawDec_getInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3)
{
	ER value = E_SYS;
	UINT32 step1 = 0, nowA, sizeA, maxA, halfsec;

	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDDEC_GETINFO_NOWOUTRAWADDR:
		if ((pparam1 == 0) || (pparam2 == 0)) {
			//DBG_ERR("GETINFO_NOWOUTRAWADDR err: p1 or p2 is zero!!\r\n");
			break;
		}
		nowA = g_aLawAddrInfo[AudDecId].usedAddr;
		halfsec = g_aLawAudPlayInfo[AudDecId].chunksize / 5;
		sizeA = g_aLawAddrInfo[AudDecId].outputRawSize;
		if (sizeA == 0) {
			// There may have 2 cases for (g_aLawAddrInfo.outputRawSize == 0) condition
			// 1. DecodeOne is not called. May the audio data has been ended.
			// 2. Forward/Backward case. The application is not read and decode data.
			// solution: continue to play the same audio data
			*pparam1 = nowA;
			*pparam2 = halfsec;
			memset((void *)nowA, 0, halfsec); // Clean to zero for avoiding nosie
			value = E_OK;
			break;
			//DBG_ERR("MP_aLawDec_getInfo:GETINFO_NOWOUTRAWADDR err: sizeA is zero!!\r\n");
		}
		if (sizeA > halfsec) {
			sizeA = halfsec;
		}
		maxA = g_aLawAddrInfo[AudDecId].maxAddr;

		*pparam1 = nowA;
		if ((nowA + sizeA) > maxA) {
			step1 = maxA - nowA;
			//step2 = sizeA - step1;
		} else {
			step1 = sizeA;
		}
		*pparam2 = step1;

		g_aLawAddrInfo[AudDecId].usedAddr = nowA + step1;
		if (g_aLawAddrInfo[AudDecId].usedAddr == maxA) { //usedAddr = maxAddr, reset to start
			g_aLawAddrInfo[AudDecId].usedAddr = g_aLawAddrInfo[AudDecId].startAddr;
		}
		g_aLawAddrInfo[AudDecId].outputRawSize -= step1;
		//DBG_IND("lasting raw = 0x%lx!\r\n", g_aLawAddrInfo.outputRawSize);
		value = E_OK;
		break;

	case MP_AUDDEC_GETINFO_SECOND2FN:
		*pparam3 = *pparam1;
		value = E_OK;
		break;

	//#NT#2012/02/08#Hideo Lin -begin
	//#NT#Add the parameter for getting audio frame number in next second start
	case MP_AUDDEC_GETINFO_NEXTSECOND2FN:
		*pparam2 = *pparam1 + 1;
		value = E_OK;
		break;
	//#NT#2012/02/08#Hideo Lin -end

	case MP_AUDDEC_GETINFO_RAWSTARTADDR:
		if ((pparam1 == 0)) {
			break;
		}
		*pparam1 = g_aLawAddrInfo[AudDecId].startAddr;
		break;

	default:
		break;
	}

	return value;
}

ER MP_aLawDec_setInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	ER setOK = E_OK;

	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDDEC_SETINFO_USEDADDR:
		if ((param1 == 0) || (param2 == 0)) {
			DBG_ERR("setinfo error p1=0x%x, p2=0x%x!\r\n", param1, param2);
			setOK = E_PAR;
		} else {
			g_aLawAddrInfo[AudDecId].startAddr     = param1;
			g_aLawAddrInfo[AudDecId].endAddr       = param1 + param2;
			g_aLawAddrInfo[AudDecId].nowAddr       = g_aLawAddrInfo[AudDecId].startAddr;
			g_aLawAddrInfo[AudDecId].usedAddr      = g_aLawAddrInfo[AudDecId].startAddr;
			//maxAddr align to chunksize
			g_aLawAddrInfo[AudDecId].maxAddr       = g_aLawAddrInfo[AudDecId].endAddr - (param2 % g_aLawAudPlayInfo[AudDecId].chunksize);
			g_aLawAddrInfo[AudDecId].outputRawSize = 0;
			g_aLawAddrInfo[AudDecId].maxRawSize    = param2;
			DBG_IND("u-Law raw start=0x%x, maxAddr=0x%x!\r\n", g_aLawAddrInfo[AudDecId].startAddr, g_aLawAddrInfo[AudDecId].maxAddr);
		}
		break;

	case MP_AUDDEC_SETINFO_RESETRAW:
		g_aLawAddrInfo[AudDecId].nowAddr       = g_aLawAddrInfo[AudDecId].startAddr;
		g_aLawAddrInfo[AudDecId].usedAddr      = g_aLawAddrInfo[AudDecId].startAddr;
		g_aLawAddrInfo[AudDecId].outputRawSize = 0;
		break;

	case MP_AUDDEC_SETINFO_FAKEDECODE:
		g_aLawFakeDecode[AudDecId] = 1;
		break;

	case MP_AUDDEC_SETINFO_REALDECODE:
		g_aLawFakeDecode[AudDecId] = 0;
		break;

	//#NT#2012/12/18#Calvin Chang#Shift the start position of audio raw data out address -begin
	case MP_AUDDEC_SETINFO_USEDADDR_SHIFT:
		// The AAC Audio Frame is 1024 samples, don't need to do byte shift
		break;
	//#NT#2012/12/18#Calvin Chang -end

	//#NT#2014/02/27#Calvin Chang#Raw Data Buffer handle by application/project -begin
	case MP_AUDDEC_SETINFO_OUTPUTRAWMODE:
		gbMp_aLawDec_RawOutMode[AudDecId] = param1;
		break;

	case MP_AUDDEC_SETINFO_RAWDATAADDR:
		if (gbMp_aLawDec_RawOutMode[AudDecId]) {
			guiMp_aLawDec_OutRawDataAddr[AudDecId] = param1;
		} else {
			DBG_ERR("RAW Data Out Mode is not set!!!\r\n");
			guiMp_aLawDec_OutRawDataAddr[AudDecId]  = 0;
		}
		break;
	//#NT#2014/02/27#Calvin Chang -end

	default:
		break;
	}

	return setOK;
}

ER MP_aLawDec_decodeOne(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	g_aLawInBSAddr[AudDecId] = BsAddr;
	g_aLawInBSSize[AudDecId] = BsSize;
	return E_OK;
}

ER MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	AUDIO_OUTRAW    *pOut;
	UINT32          outputRaw;
	ER              ErrorCode;

	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	if (alaw_dec_obj == NULL) {
		DBG_ERR("alaw object is NULL\r\n");
		return E_NOEXS;
	}

	pOut = (AUDIO_OUTRAW *)p2;

	//#NT#2014/02/27#Calvin Chang#Raw Data Buffer handle by application/project -begin
	if (gbMp_aLawDec_RawOutMode[AudDecId]) {
		if (!guiMp_aLawDec_OutRawDataAddr[AudDecId]) {
			DBG_ERR("RAW Data Out Address is not set!!!\r\n");
			return E_SYS;
		}

		if (alaw_dec_obj->g711.alaw_decode == NULL) {
			pOut->outRawAddr = guiMp_aLawDec_OutRawDataAddr[AudDecId];
			pOut->outRawSize = 0;
			return E_OK;
		}

		ErrorCode = alaw_dec_obj->g711.alaw_decode((UINT8 *)g_aLawInBSAddr[AudDecId], (INT16 *)guiMp_aLawDec_OutRawDataAddr[AudDecId], g_aLawInBSSize[AudDecId], FALSE, FALSE);

		if (ErrorCode == E_OK) {
			outputRaw = g_aLawInBSSize[AudDecId] * 2; // 2 bytes per audio raw data
		} else {
			DBG_ERR("u-Law decode error %d!\r\n", ErrorCode);
			outputRaw = 0;
		}

		pOut->outRawAddr = guiMp_aLawDec_OutRawDataAddr[AudDecId];
		pOut->outRawSize = outputRaw;

	} else
		//#NT#2014/02/27#Calvin Chang -end
	{
		// to do: need to implement u-Law decode flow which handles data buffer by itself
	}

	return E_OK;
}
