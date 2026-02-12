/*
    Media Plugin - PCM Decoder.

    This file is the media plugin library - pcm decoder.

    @file       mp_pcm_decoder.c
    @date       2018/09/26

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifdef __KERNEL__
#include "audio_decode.h"
#include "mp_pcm_decoder.h"
#include "kwrap/error_no.h"
#include <linux/string.h>
#else
#include <string.h>
#include "audio_decode.h"
#include "mp_pcm_decoder.h"
#include "kwrap/error_no.h"
#include <string.h>
#endif

#define __MODULE__          pcm_dec
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int pcm_dec_debug_level = NVT_DBG_WRN;

UINT32 gPCMOutputRawAddr, gPCMInBSAddr, gPCMInBSSize;
AUDIO_PLAYINFO gPCMAudPlayInfo;
PCM_ADDRINFO   gPCMAddrInfo;
UINT8 gPCMFakeDecode = 0;

//#NT#2014/03/14#Calvin Chang#Raw Data Buffer handle by application/project -begin
BOOL   gbMp_PCMDec_RawOutMode              = FALSE;
UINT32 guiMp_PCMDec_OutRawDataAddr         = 0;
//#NT#2014/03/14#Calvin Chang -end

/*MP_AUDDEC_DECODER gPCMAudDecObj = {
	MP_PCMDec_initialize,  //Initialize
	MP_PCMDec_getInfo,     //GetInfo
	MP_PCMDec_setInfo,     //SetInfo
	MP_PCMDec_decodeOne,   //DecodeOne
	MP_PCMDec_waitDecodeDone,//WaitDecodeDone
	NULL, //CustomizeFunc
	PCM_DECODEID           //checkID
};*/

/**
    Get pcm decoding object.

    Get pcm decoding object.

    @return audio codec decoding object
*/
/*PMP_AUDDEC_DECODER MP_PCMDec_getAudioDecodeObject(void)
{
	PMP_AUDDEC_DECODER pDecObj;

	pDecObj = &gPCMAudDecObj;

	return pDecObj;
}*/

ER MP_PCMDec_init(MP_AUDDEC_ID AudDecId, AUDIO_PLAYINFO *pobj)
{
	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	gPCMOutputRawAddr = 0;
	gPCMFakeDecode = 0;
	gPCMAudPlayInfo.sampleRate = pobj->sampleRate;
	gPCMAudPlayInfo.channels   = pobj->channels;
	gPCMAudPlayInfo.chunksize = (pobj->sampleRate) * (pobj->channels) * 2;
	return E_OK;
}

ER MP_PCMDec_getInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3)
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
			//DBG_ERR("MP_PCMDec_getInfo:GETINFO_NOWOUTRAWADDR err: p1 or p2 is zero!!\r\n");
			break;
		}
		nowA = gPCMAddrInfo.usedAddr;
		halfsec = gPCMAudPlayInfo.chunksize / 5;
		sizeA = gPCMAddrInfo.outputRawSize;
		if (sizeA == 0) {
			// There may have 2 cases for (gPCMAddrInfo.outputRawSize == 0) condition
			// 1. DecodeOne is not called. May the audio data has been ended.
			// 2. Forward/Backward case. The application is not read and decode data.
			// solution: continue to play the same audio data
			*pparam1 = nowA;
			*pparam2 = halfsec;
			memset((void *)nowA, 0, halfsec); // Clean to zero for avoiding nosie
			value = E_OK;
			break;
			//DBG_ERR("MP_PCMDec_getInfo:GETINFO_NOWOUTRAWADDR err: sizeA is zero!!\r\n");
		}
		if (sizeA > halfsec) {
			sizeA = halfsec;
		}
		maxA = gPCMAddrInfo.maxAddr;

		*pparam1 = nowA;
		if ((nowA + sizeA) > maxA) {
			step1 = maxA - nowA;
			//step2 = sizeA - step1;
		} else {
			step1 = sizeA;
		}
		*pparam2 = step1;

		gPCMAddrInfo.usedAddr = nowA + step1;
		if (gPCMAddrInfo.usedAddr == maxA) { //usedAddr = maxAddr, reset to start
			gPCMAddrInfo.usedAddr = gPCMAddrInfo.startAddr;
		}
		gPCMAddrInfo.outputRawSize -= step1;
		//DBG_IND("lasting raw = 0x%x!\r\n", gPCMAddrInfo.outputRawSize);
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
		break;
	//#NT#2012/02/08#Hideo Lin -end

	case MP_AUDDEC_GETINFO_RAWSTARTADDR:
		if ((pparam1 == 0)) {
			break;
		}
		*pparam1 = gPCMAddrInfo.startAddr;
		break;

	default:
		break;
	}

	return value;
}

ER MP_PCMDec_setInfo(MP_AUDDEC_ID AudDecId, MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	ER setOK = E_OK;
	UINT32 audioblk = 0;

	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	switch (type) {
	case MP_AUDDEC_SETINFO_USEDADDR:
		if ((param1 == 0) || (param2 == 0)) {
			DBG_ERR("MP_PCMDec_setInfo:setinfo error p1= 0x%x, p2=0x%x!\r\n", param1, param2);
			setOK = E_PAR;
		} else {
			gPCMAddrInfo.startAddr = param1;
			gPCMAddrInfo.endAddr = param1 + param2;
			gPCMAddrInfo.nowAddr = gPCMAddrInfo.startAddr;
			gPCMAddrInfo.usedAddr = gPCMAddrInfo.startAddr;
			//maxAddr align to chunksize
			gPCMAddrInfo.maxAddr = gPCMAddrInfo.endAddr - (param2 % gPCMAudPlayInfo.chunksize);
			gPCMAddrInfo.outputRawSize = 0;
			//#NT#2010/08/03#Meg Lin -begin
			gPCMAddrInfo.maxRawSize = param2;
			//#NT#2010/08/03#Meg Lin -end
			DBG_IND("PCM raw start=0x%x, maxAddr=0x%x!\r\n", gPCMAddrInfo.startAddr, gPCMAddrInfo.maxAddr);
		}
		break;

	case MP_AUDDEC_SETINFO_RESETRAW:
		gPCMAddrInfo.nowAddr = gPCMAddrInfo.startAddr;
		gPCMAddrInfo.usedAddr = gPCMAddrInfo.startAddr;
		gPCMAddrInfo.outputRawSize = 0;
		break;

	case MP_AUDDEC_SETINFO_FAKEDECODE:
		gPCMFakeDecode = 1;
		break;

	case MP_AUDDEC_SETINFO_REALDECODE:
		gPCMFakeDecode = 0;
		break;

	//#NT#2012/12/18#Calvin Chang#Shift the start position of audio raw data out address -begin
	case MP_AUDDEC_SETINFO_USEDADDR_SHIFT:
		// The exact byte shift MUST be audio block aligned!
		audioblk = gPCMAudPlayInfo.chunksize / 5;
		gPCMAddrInfo.usedAddr += ((param1 / audioblk) * audioblk);
		break;
	//#NT#2012/12/18#Calvin Chang -end

	//#NT#2014/02/27#Calvin Chang#Raw Data Buffer handle by application/project -begin
	case MP_AUDDEC_SETINFO_OUTPUTRAWMODE:
		gbMp_PCMDec_RawOutMode = param1;
		break;
	case MP_AUDDEC_SETINFO_RAWDATAADDR:
		if (gbMp_PCMDec_RawOutMode) {
			guiMp_PCMDec_OutRawDataAddr  = param1;
		} else {
			DBG_ERR("RAW Data Out Mode is not set!!!\r\n");
			guiMp_PCMDec_OutRawDataAddr  = 0;
		}
		break;
	//#NT#2014/02/27#Calvin Chang -end

	default:
		break;
	}

	return setOK;
}

ER MP_PCMDec_decodeOne(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	gPCMInBSAddr = BsAddr;
	gPCMInBSSize = BsSize;
	return E_OK;
}

ER MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID AudDecId, UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	AUDIO_OUTRAW *pOut;
	//#NT#2010/11/02#Meg Lin -begin
	//fixbug: audio data lost since buffersize not match sampleRate after pause
	UINT32 step1, step2 = 0;

	if (AudDecId >= MP_AUDDEC_ID_MAX) {
		DBG_ERR("invalid id: %d\r\n", AudDecId);
		return E_PAR;
	}

	pOut = (AUDIO_OUTRAW *)p2;

	//#NT#2014/02/27#Calvin Chang#Raw Data Buffer handle by application/project -begin
	if (gbMp_PCMDec_RawOutMode) {
		if (guiMp_PCMDec_OutRawDataAddr) {
			memcpy((UINT8 *)guiMp_PCMDec_OutRawDataAddr, (UINT8 *)gPCMInBSAddr, gPCMInBSSize);
		} else {
			DBG_ERR("RAW Data Out Address is not set!!!\r\n");
		}

		pOut->outRawAddr = guiMp_PCMDec_OutRawDataAddr;
		pOut->outRawSize = gPCMInBSSize;

	} else
		//#NT#2014/02/27#Calvin Chang -end
	{
		if ((gPCMAddrInfo.nowAddr + gPCMInBSSize) > gPCMAddrInfo.maxAddr) {
			step2 = gPCMAddrInfo.nowAddr + gPCMInBSSize - gPCMAddrInfo.maxAddr;
			step1 = gPCMInBSSize - step2;
		} else {
			step1 = gPCMInBSSize;
		}

		pOut->outRawAddr = gPCMAddrInfo.nowAddr;
		pOut->outRawSize = gPCMInBSSize;
		//DBG_IND("from 0x%x, outRAw addr=0x%x, size=0x%x.\r\n", gPCMInBSAddr, pOut->outRawAddr, pOut->outRawSize);
		if (gPCMFakeDecode == 0) { //real decode
			memcpy((UINT8 *)gPCMAddrInfo.nowAddr, (UINT8 *)gPCMInBSAddr, step1);
			gPCMAddrInfo.nowAddr += step1;
			if (step2) {
				gPCMAddrInfo.nowAddr = gPCMAddrInfo.startAddr;
				memcpy((UINT8 *)gPCMAddrInfo.nowAddr, (UINT8 *)gPCMInBSAddr + step1, step2);
				//DBG_IND("from 0x%x, outRAw addr=0x%x, step2=0x%x.\r\n", gPCMInBSAddr+step1, gPCMAddrInfo.nowAddr, step2);
				gPCMAddrInfo.nowAddr += step2;
			}
		}
		//else, donothing
		//DBG_IND("from 0x%x to 0x%x, size 0x%x!\r\n", gPCMInBSAddr, gPCMAddrInfo.nowAddr, gPCMInBSSize);

		if (gPCMAddrInfo.nowAddr == gPCMAddrInfo.maxAddr) {
			gPCMAddrInfo.nowAddr = gPCMAddrInfo.startAddr;
		}
		//#NT#2010/11/02#Meg Lin -end
		gPCMAddrInfo.outputRawSize += gPCMInBSSize;
		//DBG_IND("decodeRaw = 0x%x!\r\n", gPCMAddrInfo.outputRawSize);

		//#NT#2010/08/03#Meg Lin -begin
		if (gPCMAddrInfo.outputRawSize > gPCMAddrInfo.maxRawSize) {
			DBG_IND("unused audio data BufferFULL!!unused = 0x%x, buffersize=0x%x!\r\n", gPCMAddrInfo.outputRawSize, gPCMAddrInfo.maxRawSize);
		}
		//#NT#2010/08/03#Meg Lin -end
	}

	return E_OK;
}
