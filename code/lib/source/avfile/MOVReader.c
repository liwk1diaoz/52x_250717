/**
    MOV file reader

    It's the MOV file reader

    @file       MOVReader.c
    @ingroup    mIAVMOV
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/

#if defined (__UITRON) || defined (__ECOS)
#include <stdio.h>
#include <stdlib.h>
#include "kernel.h"
#include "cache.h"
#include "MOVLib.h"
#include "movPlay.h"
#include "MOVReadTag.h"
#include "Debug.h"
//#include "MPEG4.h"
//#include "MPEG4Drv.h"

#define __MODULE__          MOVR
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include "avfile/MOVLib.h"
#include "movPlay.h"
#include "MOVReadTag.h"
#define __MODULE__          MOVR
#define __DBGLVL__          2
#include "kwrap/debug.h"
#include "kwrap/error_no.h"

//#define __MODULE__          MOVR
//#define __DBGLVL__          2
unsigned int MOVRead_debug_level = NVT_DBG_WRN;
#endif

//#NT#2007/03/28#Corey Ke -begin
//#Enable ASF to play all files without DCF
UINT8               g_uiMovPlayCommand;
BOOL                g_bPlayAllMovFiles = FALSE;
//#NT#2007/03/28#Corey Ke -end
volatile UINT32                     gMOVFilePos;


MOVPLAY_Track                       g_movReadTrack;
MOVPLAY_Context                     g_movReadCon;
MOV_Ientry                          g_movReadEntry[0x10];
/** \addtogroup mIAVMOV */
//@{


UINT32          g_movTempDecObjSize[0x3c] = {0x47b0, 0x47B0, 0x47B0, 0x10AB, 0x0D5A, 0x0EDB, 0x15A3, 0x25A1, 0x28A3, 0x2965,
											 0x2795, 0x1A74, 0x252C, 0x26B5, 0x27FA, 0x2BCC, 0x2CF9, 0x218A,
											 0x28ED, 0x2DA0, 0x2B13, 0x2940, 0x18AF, 0x2767, 0x2A65, 0x2542,
											 0x1B82, 0x2639, 0x28F4, 0x4A91, 0x1F0B, 0x29D4, 0x2B92, 0x2B96,
											 0x2288, 0x2B0A, 0x2CE8, 0x2830, 0x1949, 0x2219, 0x1C2F, 0x1F5D,
											 0x29A4, 0x2A36, 0x287E, 0x21FD, 0x180C, 0x0FB5, 0x0CFF, 0x1232,
											 0x0CAD, 0x0B9A, 0x1110, 0x0CCC, 0x0C8A, 0x0CE5, 0x5EB6, 0x0F09,
											 0x0E73, 0x0F9B
											};
UINT32          g_movMoovErrorReason = 0, g_movOldOffsetFrame, g_movOldFrameSize;

extern UINT16  gMovWidth;
extern UINT16  gMovHeight;
UINT8 *gMovFileBuffer;
//void MOV_cbShowErrMsg(char *fmtstr, ...);

CONTAINERPARSER gMovReaderContainer;

ER MovReadLib_RegisterObjCB(void *pobj)
{
	CONTAINERPARSER *pContainer;
	pContainer = (CONTAINERPARSER *)pobj;
	if (pContainer->checkID != MOVREADLIB_CHECKID) {
		return E_NOSPT;
	}
	gMovReaderContainer.cbReadFile = (pContainer->cbReadFile);
	gMovReaderContainer.cbShowErrMsg = (pContainer->cbShowErrMsg);

	//if (gMovReaderContainer.cbShowErrMsg)
	//    (gMovReaderContainer.cbShowErrMsg)("Create Obj OK\r\n");
	return E_OK;
}

#if 0
#pragma mark -
#endif

void MOVRead_SetMoovErrorReason(UINT32 reason)
{
	g_movMoovErrorReason |= reason;
	DBG_ERR("Mov header error: parse MOOV error 0x%lx\r\n", g_movMoovErrorReason);
}

void MOVRead_ParseHeader(UINT32 addr, void *ptr)
{
	UINT32 size, tag, moovError, temp, tempV, tempA, codecID, duration, tempChs, value;//2009/12/21 Meg
	MEDIA_HEADINFO *pHeader;

	pHeader = (MEDIA_HEADINFO *)ptr;
	gMOVFilePos = 0;
	if (pHeader->checkID != MOVREADLIB_CHECKID) {
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("ERROR!! check ID ERROR!!!\r\n");
		}
		return;
	}

	g_movMoovErrorReason = 0;
	gMovFileBuffer = (UINT8 *)addr;
	MOV_ReadSpecificB32Bits((UINT8 *)gMovFileBuffer, &size);
	MOV_ReadSpecificB32Bits((UINT8 *)(gMovFileBuffer + 4), &tag);

	if (tag != MOV_moov) {
		//maybe other atom, we should read continue.....
		g_movMoovErrorReason = MOV_MOOVERROR_MDATNOFOUND;
		DBG_ERR("Mov header error: MDAT tag not found at 0x04 !\r\n");
		return;// MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT;
	}

	MOV_MakeReadAtomInit(gMovFileBuffer);
	DBG_IND("read header start\r\n");
	moovError = MOV_Read_Header((UINT8 *)gMovFileBuffer);
	if (moovError) {
		return;// moovError;
	}
	//if (gMovReaderContainer.cbShowErrMsg)
	//{
	//    (gMovReaderContainer.cbShowErrMsg)("read header ok\r\n");
	//}
	MOV_Read_GetParam(MOVPARAM_FIRSTVIDEOOFFSET, &(pHeader->uiFirstVideoOffset));
	MOV_Read_GetParam(MOVPARAM_FIRSTAUDIOOFFSET, &(pHeader->uiFirstAudioOffset));
	MOV_Read_GetParam(MOVPARAM_TOTAL_AFRAMES, &tempA);
	MOV_Read_GetParam(MOVPARAM_TOTAL_VFRAMES, &tempV);
	MOV_Read_GetParam(MOVPARAM_AUDIOCODECID, &codecID);
	pHeader->uiTotalAudioFrames = tempA;
	pHeader->uiTotalVideoFrames = tempV;
	if ((tempA)) { //&&(codecID == MEDIAAUDIO_CODEC_SOWT))
		pHeader->bAudioSupport = TRUE;
	} else {
		pHeader->bAudioSupport = FALSE;
	}
	pHeader->bVideoSupport = TRUE;
	//MOV_Read_GetParam(MOVPARAM_MVHDDURATION, &duration);
	MOV_Read_GetParam(MOVPARAM_VIDTKHDDURATION, &duration);//video tkhd duration //2013/01/29 Calvin
	MOV_Read_GetParam(MOVPARAM_VIDEOTIMESCALE, &tempV);
	pHeader->uiTotalSecs = duration / tempV; //timescale = MOV_TIMESCALE
	pHeader->uiVidScale = tempV;
	MOV_Read_GetParam(MOVPARAM_TOTAL_VFRAMES, &temp);
	if (duration % tempV == 0) {
		pHeader->uiVidRate = temp / (pHeader->uiTotalSecs);
	} else {
		//#NT#2010/11/04#Meg Lin -begin
		//decode arcsoft mov fails
		UINT32 scale, totalV;
#if 1
		MOV_Read_GetParam(MOVPARAM_VIDEOTIMESCALE, &scale);
		MOV_Read_GetParam(MOVPARAM_TOTAL_VFRAMES, &totalV);
		// since timescale = 30000, divide 25 to avoid data overflow in huge frame number.
		value = (scale / 25) * totalV;
		value /= duration / 25;
		pHeader->uiVidRate = value;
		//pHeader->uiVidRate = scale*totalV/duration;
#else
		pHeader->uiVidRate = temp / (pHeader->uiTotalSecs);
		if (pHeader->uiVidRate > 22) {
			pHeader->uiVidRate = 30;
		} else {
			pHeader->uiVidRate = 15;
		}
#endif
		//#NT#2010/11/04#Meg Lin -end
	}
	//if (gMovReaderContainer.cbShowErrMsg)
	//{
	//    (gMovReaderContainer.cbShowErrMsg)("duration=%ld  seconds!\r\n", pHeader->uiTotalSecs);
	//}

	//DBG_ERR("duration=%ld  seconds!\r\n", pHeader->uiTotalSecs);
	MOV_Read_GetParam(MOVPARAM_SAMPLERATE, &temp);
	pHeader->uiAudSampleRate = temp;
	MOV_Read_GetParam(MOVPARAM_AUDIO_BITSPERSAMPLE, &tempA);
	//#NT#2009/12/21#Meg Lin -begin
	//#NT#new feature: play stereo
	MOV_Read_GetParam(MOVPARAM_AUDCHANNELS, &tempChs);
	pHeader->ucAudChannels = tempChs;
	pHeader->uiAudBytesPerSec = temp * tempChs * (tempA / 8);
	//#NT#2009/12/21#Meg Lin -end
	pHeader->ucAudBitsPerSample = tempA;
	MOV_Read_GetParam(MOVPARAM_AUDIOSIZE, &tempV);
	pHeader->uiAudWavTotalSamples = tempV;
	MOV_Read_GetParam(MOVPARAM_AUDIOTYPE, &tempV);
	pHeader->uiAudType = tempV;

}
//#NT#2008/04/15#Meg Lin -begin
//add MovMjpg Parse total seconds

void MOVRead_ParseHeaderSmallSize(UINT32 addr, UINT32 *pTotalSec, UINT32 readsize)
{
	UINT32 size, tag, moovError, temp, tempV;
	gMOVFilePos = 0;

	g_movMoovErrorReason = 0;
	gMovFileBuffer = (UINT8 *)addr;
	MOV_ReadSpecificB32Bits((UINT8 *)gMovFileBuffer, &size);
	MOV_ReadSpecificB32Bits((UINT8 *)(gMovFileBuffer + 4), &tag);

	if (tag != MOV_moov) {
		//maybe other atom, we should read continue.....
		g_movMoovErrorReason = MOV_MOOVERROR_MDATNOFOUND;
		DBG_ERR("Mov header error: MDAT tag not found at 0x04 !\r\n");
		return;// MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT;
	}
	MOV_MakeReadAtomInit(gMovFileBuffer);
	DBG_IND("read header start\r\n");
	//#NT#2008/07/01#Meg Lin -begin
	//moovError = MOV_Read_Header_WithLimit((char *)gMovFileBuffer, 10);
	moovError = MOV_Read_Header_WithLimit((UINT8 *)gMovFileBuffer, 10);

	//#NT#2008/07/01#Meg Lin -end
	if (moovError) {
		DBG_ERR("moovError, so finished!!!\r\n");
		return;// moovError;
	}
	DBG_IND("read header ok\r\n");
	//MOV_Read_GetParam(MOVPARAM_MVHDDURATION, &temp);
	MOV_Read_GetParam(MOVPARAM_VIDTKHDDURATION, &temp);//video tkhd duration //2013/01/29 Calvin
	MOV_Read_GetParam(MOVPARAM_VIDEOTIMESCALE, &tempV);
	*pTotalSec = temp / tempV; //timescale = MOV_TIMESCALE
	DBG_IND("duration=%ld  seconds!\r\n", *pTotalSec);
}
//#NT#2008/04/15#Meg Lin -end

void MOVRead_ParseHeaderFirstVideo(UINT32 addr, void *pobj)
{
	UINT32 size, tag, moovError;
	gMOVFilePos = 0;

	g_movMoovErrorReason = 0;
	gMovFileBuffer = (UINT8 *)addr;
	MOV_ReadSpecificB32Bits((UINT8 *)gMovFileBuffer, &size);
	MOV_ReadSpecificB32Bits((UINT8 *)(gMovFileBuffer + 4), &tag);

	if (tag != MOV_moov) {
		//maybe other atom, we should read continue.....
		g_movMoovErrorReason = MOV_MOOVERROR_MDATNOFOUND;
		DBG_ERR("Mov header error: MDAT tag not found at 0x04 !\r\n");
		return;// MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT;
	}
	MOV_MakeReadAtomInit(gMovFileBuffer);
	DBG_IND("read header start\r\n");
	//#NT#2008/07/01#Meg Lin -begin
	//moovError = MOV_Read_Header_WithLimit((char *)gMovFileBuffer, 10);
	moovError = MOV_Read_Header_FirstVideo((UINT8 *)gMovFileBuffer);

	//#NT#2008/07/01#Meg Lin -end
	if (moovError) {
		DBG_ERR("moovError, so finished!!!\r\n");
		return;// moovError;
	}
#if MOV_PLAYDEBUG
	DBG_ERR("1st Video size=0x%lx, pos=0x%lx\r\n ", gMovFirstVInfo.size, gMovFirstVInfo.pos);
#endif
}

void MOVRead_ParseHeaderVideoInfo(UINT32 addr, void *pobj)
{
	UINT32 size, tag, moovError;
	gMOVFilePos = 0;

	g_movMoovErrorReason = 0;
	gMovFileBuffer = (UINT8 *)addr;
	MOV_ReadSpecificB32Bits((UINT8 *)gMovFileBuffer, &size);
	MOV_ReadSpecificB32Bits((UINT8 *)(gMovFileBuffer + 4), &tag);

	if (tag != MOV_moov) {
		//maybe other atom, we should read continue.....
		g_movMoovErrorReason = MOV_MOOVERROR_MDATNOFOUND;
		DBG_ERR("Mov header error: MDAT tag not found at 0x04 !\r\n");
		return;// MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT;
	}
	MOV_MakeReadAtomInit(gMovFileBuffer);
	DBG_IND("read header start\r\n");
	//#NT#2008/07/01#Meg Lin -begin
	moovError = MOV_Read_Header_VideoInfo((UINT8 *)gMovFileBuffer);

	//#NT#2008/07/01#Meg Lin -end
	if (moovError) {
		DBG_ERR("moovError, so finished!!!\r\n");
		return;// moovError;
	}
#if MOV_PLAYDEBUG
	DBG_ERR("1st Video size=0x%lx, pos=0x%lx\r\n ", gMovFirstVInfo.size, gMovFirstVInfo.pos);
#endif
}

//@}


