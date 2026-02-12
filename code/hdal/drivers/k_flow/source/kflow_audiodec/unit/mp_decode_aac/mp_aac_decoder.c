/*
    Media Plugin - AAC Decoder.

    This file is the media plugin library - aac decoder.

    @file       mp_aac_decoder.c
    @date       2018/09/26

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifdef __KERNEL__
#include "audio_decode.h"
#include "audio_common/include/Audio.h"
#include "mp_aac_decoder.h"
#include "kwrap/error_no.h"
#include <linux/string.h>
#include "kdrv_audioio/kdrv_audioio.h"
#else
#include "audio_decode.h"
#include "audio_common/include/Audio.h"
#include "mp_aac_decoder.h"
#include "kwrap/error_no.h"
#include <string.h>
#include "kdrv_audioio/kdrv_audioio.h"
#endif

#define __MODULE__          aac_dec
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int aac_dec_debug_level = NVT_DBG_WRN;

AUDIO_PLAYINFO gAACAudPlayInfo[MP_AUDDEC_ID_MAX];
AAC_ADDRINFO   gAACAddrInfo[MP_AUDDEC_ID_MAX];
UINT32 gAACInBSAddr[MP_AUDDEC_ID_MAX], gAACInBSSize[MP_AUDDEC_ID_MAX];
UINT8 gAACFakeDecode[MP_AUDDEC_ID_MAX] = {0};

//#NT#2014/03/14#Calvin Chang#Raw Data Buffer handle by application/project -begin
BOOL   gbMp_AACDec_RawOutMode[MP_AUDDEC_ID_MAX] = {0};
UINT32 guiMp_AACDec_OutRawDataAddr[MP_AUDDEC_ID_MAX] = {0};
//#NT#2014/03/14#Calvin Chang -end
BOOL gAACCodingType[MP_AUDDEC_ID_MAX] = {0};

static KDRV_AUDIOLIB_FUNC* aac_dec_obj = NULL;

/*AUDIO_DECODER gAACAudDecObj = {
	MP_AACDec_initialize,  //Initialize
	MP_AACDec_getInfo,     //GetInfo
	MP_AACDec_setInfo,     //SetInfo
	MP_AACDec_decodeOne,   //DecodeOne
	MP_AACDec_waitDecodeDone,//WaitDecodeDone
	NULL, //CustomizeFunc
	AAC_DECODEID           //checkID
};*/

/**
    Get aac decoding object.

    Get aac decoding object.

    @return audio codec decoding object
*/
/*PAUDIO_DECODER MP_AACDec_getAudioDecodeObject(void)
{
	PAUDIO_DECODER pDecObj;


	pDecObj = &gAACAudDecObj;

	return pDecObj;
}*/

ER MP_AACDec_init(MP_AUDDEC_ID AudDecId, AUDIO_PLAYINFO *pobj)
{
	AUDLIB_AAC_CFG aac_config;
	INT32          err_code;

	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	if (aac_dec_obj == NULL) {
		aac_dec_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_AAC);
		if (aac_dec_obj == NULL) {
			DBG_ERR("AAC object is NULL\r\n");
			return E_NOEXS;
		}
	}

	gAACFakeDecode[AudDecId] = 0;
	gAACAudPlayInfo[AudDecId].sampleRate  = pobj->sampleRate;
	gAACAudPlayInfo[AudDecId].channels    = pobj->channels;
	gAACAudPlayInfo[AudDecId].chunksize   = (pobj->sampleRate) * (pobj->channels) * 2;

	// audio sampling rate
	aac_config.sample_rate = pobj->sampleRate;

	// audio channel number
	aac_config.channel_number = pobj->channels;

	// coding type
	aac_config.header_enable = gAACCodingType[AudDecId];

	// init AAC decoder
	if (aac_dec_obj->aac.decode_init) {
		if ((err_code = aac_dec_obj->aac.decode_init(&aac_config)) != 0) {
			DBG_ERR("aac decoder config error %d!\r\n", err_code);
			return E_SYS;
		}
	} else {
		DBG_ERR("aac decoder init is NULL\r\n");
		return E_SYS;
	}


	return E_OK;
}

ER MP_AACDec_getInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3)
{
	ER value = E_SYS;
	UINT32 step1 = 0, step2 = 0, nowA, sizeA, maxA, halfsec, sec, shiftbyte;

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
		nowA = gAACAddrInfo[AudDecId].usedAddr;
		halfsec = gAACAudPlayInfo[AudDecId].chunksize / 5;
		sizeA = gAACAddrInfo[AudDecId].outputRawSize;
		if (sizeA > halfsec) {
			sizeA = halfsec;
		}
		maxA = gAACAddrInfo[AudDecId].maxAddr;
		if (sizeA == 0) {
			// There may have 2 cases for (gPCMAddrInfo.outputRawSize == 0) condition
			// 1. DecodeOne is not called. May the audio data has been ended.
			// 2. Forward/Backward case. The application is not read and decode data.
			// solution: continue to play the same audio data
			*pparam1 = nowA;
			*pparam2 = halfsec;
			memset((void *)nowA, 0, halfsec); // Clean to zero for avoiding nosie
			//DBG_ERR("GETINFO_NOWOUTRAWADDR err: sizeA is zero!!\r\n");
			break;
		}
		*pparam1 = nowA;
		if ((nowA + sizeA) > maxA) {
			step1 = maxA - nowA;
			//step2 = sizeA - step1;
		} else {
			step1 = sizeA;
		}
		*pparam2 = step1;

		gAACAddrInfo[AudDecId].usedAddr = nowA + step1;
		if (gAACAddrInfo[AudDecId].usedAddr == maxA) { //usedAddr = maxAddr, reset to start
			gAACAddrInfo[AudDecId].usedAddr = gAACAddrInfo[AudDecId].startAddr;
		}
		gAACAddrInfo[AudDecId].outputRawSize -= step1;
		//DBG_IND("get 0x%x, size=0x%x, lasting raw = 0x%x!\r\n", nowA, step1, gAACAddrInfo.outputRawSize);
		value = E_OK;
		break;

	case MP_AUDDEC_GETINFO_SECOND2FN:
		sec = *pparam1;
		shiftbyte = *pparam2;
		step1 = (sec * gAACAudPlayInfo[AudDecId].sampleRate) / AAC_DECODE_BLOCK; //count sec
		nowA = gAACAudPlayInfo[AudDecId].sampleRate / AAC_DECODE_BLOCK;
		step2 = shiftbyte / 2 / gAACAudPlayInfo[AudDecId].channels / AAC_DECODE_BLOCK; //count offset
		*pparam3 = step1 + nowA - step2;
		value = E_OK;
		//DBG_IND("SECOND2FN:sec=%d, shift=%d, step1=%d, nowA=%d, step2=%d\r\n", sec, shiftbyte, step1, nowA, step2);
		break;

	//#NT#2012/02/08#Hideo Lin -begin
	//#NT#Add the parameter for getting audio frame number in next second start
	case MP_AUDDEC_GETINFO_NEXTSECOND2FN:
		nowA = *pparam1;                            // param1 = current audio frame
		sizeA = nowA * AAC_DECODE_BLOCK;            // audio samples for current audio frame
		if ((sizeA % gAACAudPlayInfo[AudDecId].sampleRate) || (nowA == 0)) {
			sec = sizeA / gAACAudPlayInfo[AudDecId].sampleRate;       // current second
		} else {
			sec = sizeA / gAACAudPlayInfo[AudDecId].sampleRate - 1;   // current second
		}
		sec++;                                      // next second
		sizeA = sec * gAACAudPlayInfo[AudDecId].sampleRate;   // audio samples for next second
		*pparam2 = sizeA / AAC_DECODE_BLOCK + 1;    // audio frame for next second
		value = E_OK;
		//DBG_IND("NEXTSECOND2FN:NextSec=%d, NextAud=%d\r\n", sec, *pparam2);
		break;
	//#NT#2012/02/08#Hideo Lin -end

	case MP_AUDDEC_GETINFO_RAWSTARTADDR:
		if ((pparam1 == 0)) {
			break;
		}
		*pparam1 = gAACAddrInfo[AudDecId].startAddr;
		value = E_OK;
		break;

	default:
		break;
	}
	return value;
}

ER MP_AACDec_setInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	ER setOK = E_OK;

	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDDEC_SETINFO_USEDADDR:
		if ((param1 == 0) || (param2 == 0)) {
			//DBG_ERR("setinfo error p1= 0x%x, p2=0x%x!\r\n", param1, param2);
			setOK = E_PAR;
		} else {
			gAACAddrInfo[AudDecId].startAddr = param1;
			gAACAddrInfo[AudDecId].endAddr = param1 + param2;
			gAACAddrInfo[AudDecId].nowAddr = gAACAddrInfo[AudDecId].startAddr;
			gAACAddrInfo[AudDecId].usedAddr = gAACAddrInfo[AudDecId].startAddr;
			//maxAddr align to chunksize
			gAACAddrInfo[AudDecId].maxAddr = gAACAddrInfo[AudDecId].endAddr - (param2 % gAACAudPlayInfo[AudDecId].chunksize);
			gAACAddrInfo[AudDecId].outputRawSize = 0;
			//#NT#2010/08/03#Meg Lin -begin
			gAACAddrInfo[AudDecId].maxRawSize = param2;
			//#NT#2010/08/03#Meg Lin -end
			DBG_IND("PCM raw start=0x%x, maxAddr=0x%x!\r\n", gAACAddrInfo[AudDecId].startAddr, gAACAddrInfo[AudDecId].maxAddr);
		}
		break;

	case MP_AUDDEC_SETINFO_RESETRAW:
		gAACAddrInfo[AudDecId].nowAddr = gAACAddrInfo[AudDecId].startAddr;
		gAACAddrInfo[AudDecId].usedAddr = gAACAddrInfo[AudDecId].startAddr;
		gAACAddrInfo[AudDecId].outputRawSize = 0;
		break;

	case MP_AUDDEC_SETINFO_FAKEDECODE:
		gAACFakeDecode[AudDecId] = 1;
		break;

	case MP_AUDDEC_SETINFO_REALDECODE:
		gAACFakeDecode[AudDecId] = 0;
		break;

	//#NT#2012/12/18#Calvin Chang#Shift the start position of audio raw data out address -begin
	case MP_AUDDEC_SETINFO_USEDADDR_SHIFT:
		// The AAC Audio Frame is 1024 samples, don't need to do byte shift
		break;
	//#NT#2012/12/18#Calvin Chang -end

	//#NT#2014/02/27#Calvin Chang#Raw Data Buffer handle by application/project -begin
	case MP_AUDDEC_SETINFO_OUTPUTRAWMODE:
		gbMp_AACDec_RawOutMode[AudDecId] = param1;
		break;
	case MP_AUDDEC_SETINFO_RAWDATAADDR:
		if (gbMp_AACDec_RawOutMode[AudDecId]) {
			guiMp_AACDec_OutRawDataAddr[AudDecId]  = param1;
		} else {
			DBG_ERR("RAW Data Out Mode is not set!!!\r\n");
			guiMp_AACDec_OutRawDataAddr[AudDecId]  = 0;
		}
		break;
	//#NT#2014/02/27#Calvin Chang -end
	case MP_AUDDEC_SETINFO_ADTS_EN:
		gAACCodingType[AudDecId] = (BOOL)param1;
		break;
	default:
		break;
	}

	return setOK;
}

ER MP_AACDec_decodeOne(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	gAACInBSAddr[AudDecId] = BsAddr;
	gAACInBSSize[AudDecId] = BsSize;
	return E_OK;
}

ER MP_AACDec_waitDecodeDone(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	AUDIO_OUTRAW        *pOut;
	UINT32              output_raw, over_max;
	AUDLIB_AAC_BUFINFO  buf_info;
	AUDLIB_AACD_RTNINFO rtn_info;
	INT32               err_code;
#if 0 // Dump AAC input buffer
	UINT8*	d;
	UINT32	c = 0;
#endif

	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	if (aac_dec_obj == NULL) {
		DBG_ERR("AAC object is NULL\r\n");
		return E_NOEXS;
	}

	pOut = (AUDIO_OUTRAW *)p2;

	//#NT#2014/02/27#Calvin Chang#Raw Data Buffer handle by application/project -begin
	if (gbMp_AACDec_RawOutMode[AudDecId]) {
		if (aac_dec_obj->aac.decode_one_frame == NULL) {
			pOut->outRawAddr = guiMp_AACDec_OutRawDataAddr[AudDecId];
			pOut->outRawSize = 0;
			return E_OK;
		}

		if (!guiMp_AACDec_OutRawDataAddr[AudDecId]) {
			DBG_ERR("RAW Data Out Address is not set!!!\r\n");
			return E_SYS;
		}

		buf_info.bitstram_buffer_in = gAACInBSAddr[AudDecId];
		buf_info.bitstram_buffer_out = guiMp_AACDec_OutRawDataAddr[AudDecId];
		buf_info.bitstram_buffer_length = AAC_DECODE_BLOCK * 2 * 2; // max 2 channels, 2 bytes per sample

// Dump AAC input buffer
#if 0
	d = (UINT8*)buf_info.bitstram_buffer_in;

	DBG_DUMP("dump bs s = %d\r\n", gAACInBSSize);
	for (; c<gAACInBSSize;)
	{
		DBG_DUMP("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x ",
			*(d+c), *(d+c+1), *(d+c+2), *(d+c+3), *(d+c+4), *(d+c+5), *(d+c+6), *(d+c+7), *(d+c+8), *(d+c+9), *(d+c+10), *(d+c+11), *(d+c+12), *(d+c+13), *(d+c+14), *(d+c+15));
		c+=16;
	}
	DBG_DUMP("\r\n\r\n");
#endif

		// decode one frame
		if ((err_code = aac_dec_obj->aac.decode_one_frame(&buf_info, &rtn_info)) != 0) {
			DBG_ERR("aac decode error: %d\r\n", err_code);
			output_raw = 0;
		} else {
			output_raw = rtn_info.output_size * rtn_info.channel_number * 2; // 2 bytes per sample
		}

		pOut->outRawAddr = guiMp_AACDec_OutRawDataAddr[AudDecId];
		pOut->outRawSize = output_raw;

		//#NT#2014/02/27#Calvin Chang -end
	} else {
		if (aac_dec_obj->aac.decode_one_frame == NULL) {
			pOut->outRawAddr = gAACAddrInfo[AudDecId].nowAddr;
			pOut->outRawSize = 0;
			return E_OK;
		}

		if (gAACFakeDecode[AudDecId] == 0) { //real decoding
			buf_info.bitstram_buffer_in = gAACInBSAddr[AudDecId];
			buf_info.bitstram_buffer_out = gAACAddrInfo[AudDecId].nowAddr;
			buf_info.bitstram_buffer_length = AAC_DECODE_BLOCK * 2 * 2; // max 2 channels, 2 bytes per sample

			err_code = aac_dec_obj->aac.decode_one_frame(&buf_info, &rtn_info);

			output_raw = rtn_info.output_size * rtn_info.channel_number * 2; // 2 bytes per sample

			//#NT#2012/02/17#Hideo Lin -begin
			//#NT#Reset audio data to 0 as AAC decode error to avoid pop noise
			if (err_code != 0) {
				DBG_ERR("aac decode error %d!\r\n", err_code);
				memset((void *)gAACAddrInfo[AudDecId].nowAddr, 0, output_raw);
			}
			//#NT#2012/02/17#Hideo Lin -end
		} else { //FWD, BWD, fake decode
			output_raw = AAC_DECODE_BLOCK * gAACAudPlayInfo[AudDecId].channels; // why don't need to x 2 (2 bytes per sample)?
		}

		pOut->outRawAddr = gAACAddrInfo[AudDecId].nowAddr;
		pOut->outRawSize = output_raw;
		DBG_MSG("outRaw addr 0x%x, size 0x%x\r\n", pOut->outRawAddr, pOut->outRawSize);
		gAACAddrInfo[AudDecId].nowAddr += output_raw;
		gAACAddrInfo[AudDecId].outputRawSize += output_raw;
		DBG_MSG("decoded raw size 0x%x\r\n", gAACAddrInfo[AudDecId].outputRawSize);

		//#NT#2010/08/03#Meg Lin -begin
		if (gAACAddrInfo[AudDecId].outputRawSize > gAACAddrInfo[AudDecId].maxRawSize) {
			DBG_ERR("unused audio data BufferFULL!! unused=0x%x, buffersize=0x%x!\r\n", gAACAddrInfo[AudDecId].outputRawSize, gAACAddrInfo[AudDecId].maxRawSize);
		}
		//#NT#2010/08/03#Meg Lin -end

		if (gAACAddrInfo[AudDecId].nowAddr > gAACAddrInfo[AudDecId].maxAddr) {
			DBG_MSG("AACD: oldnow 0x%x\r\n", gAACAddrInfo[AudDecId].nowAddr);

			over_max = gAACAddrInfo[AudDecId].nowAddr - gAACAddrInfo[AudDecId].maxAddr;
			memcpy((UINT8 *)gAACAddrInfo[AudDecId].startAddr, (UINT8 *)gAACAddrInfo[AudDecId].maxAddr, over_max);
			DBG_MSG("AACD: copy from 0x%x to 0x%x\r\n", gAACAddrInfo[AudDecId].nowAddr, gAACAddrInfo[AudDecId].startAddr);

			gAACAddrInfo[AudDecId].nowAddr = gAACAddrInfo[AudDecId].startAddr + over_max;
			DBG_MSG("AACD: newnow 0x%x\r\n", gAACAddrInfo[AudDecId].nowAddr);
		}
	}

	return E_OK;
}

ER MP_AACDec_parseADTS(UINT32 bsAddr, UINT32 *pHeader_length, UINT32 *pStream_length)
{
	UINT8	*ptr = (UINT8 *)bsAddr;
	UINT32	syncword = 0;
	UINT32	protection_absent = 0;
	UINT32	frame_length = 0;
	ER		EResult = E_SYS;

	// check syncword
	syncword = (*ptr & 0xff) | (*(ptr+1) & 0xf0) << 4;

	if (syncword == 0xfff)
	{
		// parse adts_header_length
		protection_absent = *(ptr+1) & 0x01;
		*pHeader_length = (protection_absent == 1)? 7 : 9;

		// parse aac_frame_length
		frame_length = ((*(ptr+3) & 0x03) << 11) | (*(ptr+4) << 3) | ((*(ptr+5) & 0xe0) >> 5);

		if (frame_length > 0)
		{
			// get aac_stream_length
			*pStream_length = frame_length - *pHeader_length;
			EResult = E_OK;
		}
		else
		{
			DBG_ERR("Invalid aac_frame_length! %d\r\n", frame_length);
			EResult = E_PAR;
		}
	}

	return EResult;
}
