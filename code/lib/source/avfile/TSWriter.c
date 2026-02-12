/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       TSWriter.c
    @ingroup    mIAVMOV

    @brief      TS file writer
                It's the TS file writer, support multi/single payload now

    @note       Nothing.
    @version    V1.00.000
    @date       2019/05/05
*/

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
#include <string.h>

#include "kernel.h"

#include "NvtVerInfo.h"
#include "FileSysTsk.h"
#include "MediaWriteLib.h"
#include "AVFile_MakerTS.h"
#include "TSRec.h"

#define __MODULE__          TSWrite
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"

NVTVER_VERSION_ENTRY(MP_TsWriteLib, 1, 00, 003, 00)
#else
#include <string.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include <kwrap/verinfo.h>

#define __MODULE__          TSWrite
#define __DBGLVL__          2
#include "kwrap/debug.h"
unsigned int TSWrite_debug_level = NVT_DBG_WRN;

#include "avfile/MediaWriteLib.h"
#include "avfile/AVFile_MakerTS.h"

// INCLUDE PROTECTED
#include "TSRec.h"

VOS_MODULE_VERSION(MP_TsWriteLib, 1, 00, 001, 00);
#endif

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
UINT64              g_videoPts[TSWRITER_COUNT] = {0};
UINT64              g_audioPts[TSWRITER_COUNT] = {0};
UINT32              g_pesHeaderSrcSize[TSWRITER_COUNT] = {0};
UINT32              g_pesHeaderSrcAddr[TSWRITER_COUNT] = {0};
UINT32              g_pesHeaderPayloadSize[TSWRITER_COUNT] = {0};
UINT32              g_tsWriteLib_vfr[TSWRITER_COUNT] = {0};
UINT32              g_tsWriteLib_asr[TSWRITER_COUNT] = {0};
UINT32              g_tsWriteLib_codec[TSWRITER_COUNT] = {0};
UINT32              g_tsWriteLib_isKey[TSWRITER_COUNT] = {0};
MOV_USER_MAKERINFO  gTsUserMakerInfo = {0};
UINT8               g_tsWriteUsed[TSWRITER_COUNT] = {0};

CONTAINERMAKER gTSContainerMaker = {
	TSWriteLib_Initialize,  //Initialize
	TSWriteLib_SetMemBuf,   //SetMemBuf
	NULL,                   //MakeHeader
	NULL,                   //UpdateHeader
	TSWriteLib_GetInfo,     //GetInfo
	TSWriteLib_SetInfo,     //SetInfo
	NULL,                   //CustomizeFunc
	NULL,                   //cbWriteFile
	NULL,                   //cbReadFile
	NULL,                   //cbShowErrMsg
	0xBC191380, //MOVWRITELIB_CHECKID,//checkID
	0, //id
};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
PCONTAINERMAKER ts_getContainerMaker(void)
{
	PCONTAINERMAKER pConMaker;
	UINT8 i;

	pConMaker = &gTSContainerMaker;

	for (i = 0; i < TSWRITER_COUNT; i++) {
		if (g_tsWriteUsed[i] == 0) {
			pConMaker->id = i;
			g_tsWriteUsed[i] = 1;
			break;
		}

	}

	return pConMaker;
}

void ts_ResetContainerMaker(void)
{
	UINT8 i;
	for (i = 0; i < TSWRITER_COUNT; i++) {
		g_tsWriteUsed[i] = 0;//unused
	}
}

ER TSWriteLib_Initialize(UINT32 idnum)
{
	g_tsWriteLib_vfr[idnum] = 30;
	g_tsWriteLib_isKey[idnum] = 0;

	return E_OK;
}

ER TSWriteLib_SetMemBuf(UINT32 idnum, UINT32 startAddr, UINT32 size)
{
	return E_OK;
}

ER TSWriteLib_GetInfo(MEDIAWRITE_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3)
{
	switch (type) {
	case MEDIAWRITE_GETINFO_USERDATA:
		*pparam1 = (UINT32)&gTsUserMakerInfo;

		break;

	default:
		break;
	}
	return E_OK;
}

ER TSWriteLib_SetInfo(MEDIAWRITE_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	UINT32 fstskid = param3;
	switch (type) {
	case MEDIAWRITE_SETINFO_PESHEADERADDR:
		g_pesHeaderSrcAddr[fstskid] = param1;
		g_pesHeaderPayloadSize[fstskid] = param2;
		break;

	case MEDIAWRITE_SETINFO_MAKEPESHEADER:
		g_tsWriteLib_isKey[fstskid] = param2;
		TSWriteLib_MakePesHeader(fstskid, g_pesHeaderSrcAddr[fstskid], g_pesHeaderPayloadSize[fstskid], param1);
		break;

	case MEDIAWRITE_SETINFO_RESETPTS:
        if(g_tsWriteLib_vfr[fstskid]&&g_tsWriteLib_asr[fstskid]) {
    		g_videoPts[fstskid] = 90000 / g_tsWriteLib_vfr[fstskid];
    		g_audioPts[fstskid] = (90000 * TS_AUDIO_SAMPLEPERFAME) / g_tsWriteLib_asr[fstskid];;
    		DBG_DUMP("reset PTS,p[%d] v:%lld a:%lld\r\n", fstskid,g_videoPts[fstskid],g_audioPts[fstskid]);
        } else {
            DBG_ERR("reset PTS fail, p[%d] %d %d\r\n", fstskid,g_tsWriteLib_vfr[fstskid],g_tsWriteLib_asr[fstskid]);
        }
		break;

	case MEDIAWRITE_SETINFO_VIDEOFRAMERATE:
		g_tsWriteLib_vfr[fstskid] = param1;
		g_tsWriteLib_codec[fstskid] = param2;
		break;
	case MEDIAWRITE_SETINFO_AUD_SAMPLERATE:
		g_tsWriteLib_asr[fstskid] = param1;
		break;

	case MEDIAWRITE_SETINFO_PESHEADERSIZE:
		g_pesHeaderSrcSize[fstskid] = param1;
		break;

	default:
		break;
	}
	return E_OK;
}

void TSWriteLib_UserMakerModelData(MOV_USER_MAKERINFO *pMaker)
{
	//DBG_DUMP("maker=%s, model=%s\r\n", maker, model);
	gTsUserMakerInfo.pMaker = pMaker->pMaker;
	gTsUserMakerInfo.makerLen = pMaker->makerLen;
	gTsUserMakerInfo.pModel = pMaker->pModel;
	gTsUserMakerInfo.modelLen = pMaker->modelLen;
	gTsUserMakerInfo.ouputAddr = pMaker->ouputAddr;
}
/*
PES
Packet_start_code_prefix¡V(0x000001)
Stream_id:110xxxxx (0xE0) audio stream number xxxxx
		  1110xxxx (0xC0) video stream number xxxx
PES_packet_length : number of bytes in the PES packet following the last byte of the field
(6 bytes =(start_code 3 bytes +Stream_id 1 byte +PES_packet_length 2 bytes)).
flag1:all config is zoro
flag2:PTS_DTS_flags '10'->PTS only
*/
void TSWriteLib_MakePesHeader(UINT32 fstskid, UINT32 srcAddr, UINT32 payload_size, UINT32 idtype)
{
	//DBG_DUMP("in coming address is 0x%x!\r\n", pSrc->uiBSMemAddr);
	UINT8 *pChar = NULL;
	UINT32 len = 0;
	UINT32 videoPtsIncrement;
	UINT32 audioPtsIncrement;
	UINT32 reserved_len = 0, stuffing_len = 0;

	videoPtsIncrement = 90000 / g_tsWriteLib_vfr[fstskid];
	audioPtsIncrement = (90000 * TS_AUDIO_SAMPLEPERFAME) / g_tsWriteLib_asr[fstskid];
	pChar = (UINT8 *)(srcAddr);

	//count stuffing size
	if (g_pesHeaderSrcSize[fstskid]) {
		if (idtype == TS_IDTYPE_VIDPATHID) {
			if (g_tsWriteLib_codec[fstskid]== TS_CODEC_H264) {
				reserved_len = TS_VIDEOPES_HEADERLENGTH + TS_VIDEO_264_NAL_LENGTH; // 9 + 5(PTS) + NAL(6)
			} else {
				reserved_len = TS_VIDEOPES_HEADERLENGTH + TS_VIDEO_265_NAL_LENGTH; // 9 + 5(PTS) + NAL(7)
			}
		} else if (idtype == TS_IDTYPE_AUDPATHID) {
			reserved_len = TS_AUDIOPES_HEADERLENGTH + TS_AUDIO_ADTS_LENGTH; // 9 + 5(PTS)
		}
		if (g_pesHeaderSrcSize[fstskid] > reserved_len) {
			stuffing_len = g_pesHeaderSrcSize[fstskid] - reserved_len;
		} else {
			stuffing_len = 0;
		}
	}

	//PES start code
	pChar[0] = 0x00;
	pChar[1] = 0x00;
	pChar[2] = 0x01;

	//stream id
	if (idtype == TS_IDTYPE_VIDPATHID) {
		pChar[3] = 0xE0;
	} else if (idtype == TS_IDTYPE_AUDPATHID) {
		pChar[3] = 0xC0;
	}

	len = payload_size + TS_AUDIOPES_HEADERLENGTH + TS_AUDIO_ADTS_LENGTH - TS_PES_PACKET_LENGTH_END + stuffing_len;

	//PES packet length
	if (idtype == TS_IDTYPE_VIDPATHID) {
	//PES packet length is set to zero, the PES packet can be of any length.
		pChar[4] = 0x00;
		pChar[5] = 0x00;
	} else if (idtype == TS_IDTYPE_AUDPATHID) {
		pChar[4] = len >> 8;
		pChar[5] = len;
	}


	//flag1
	pChar[6] = 0x80;
	//flag2
	pChar[7] = 0x80;

	//PES header data length
	pChar[8] = TS_PTS_LENGTH + stuffing_len;  //PTS is 5 bytes (ref to TS_Write_PTS)

	if (idtype == TS_IDTYPE_VIDPATHID) {
		TS_Write_PTS(&pChar[9], g_videoPts[fstskid]);
		if (stuffing_len) {
			TS_Write_Stuffing(&pChar[9 + TS_PTS_LENGTH], stuffing_len);
		}
		TS_Write_VidNAL(fstskid, &pChar[9 + TS_PTS_LENGTH + stuffing_len]);
		g_videoPts[fstskid] += videoPtsIncrement;
	} else if (idtype == TS_IDTYPE_AUDPATHID) {
		TS_Write_PTS(&pChar[9], g_audioPts[fstskid]);
		if (stuffing_len) {
			TS_Write_Stuffing(&pChar[9 + TS_PTS_LENGTH], stuffing_len);
		}
		//add ADTS in AudEnc,remove hardcode ADTS
		//TS_Write_AudADTS(&pChar[9 + TS_PTS_LENGTH], payload_size);
		g_audioPts[fstskid] += audioPtsIncrement;
	}
}

// PTS  5 bytes
/*
    '0011'        4
    PTS [32..30]  3
    marker_bit    1
    PTS [29..15] 15
    marker_bit    1
    PTS [14..0]  15
    marker_bit    1
*/
void TS_Write_PTS(UINT8 *q, UINT64 pts)
{
	UINT32 val;
	val = 0x02 << 4 | (((pts >> 30) & 0x07) << 1) | 1;
	*q++ = val;
	val  = (((pts >> 15) & 0x7fff) << 1) | 1;
	*q++ = val >> 8;
	*q++ = val;
	val  = (((pts) & 0x7fff) << 1) | 1;
	*q++ = val >> 8;
	*q++ = val;
}

// Stuffing (0xFF)
void TS_Write_Stuffing(UINT8 *q, UINT32 size)
{
	UINT32 i;
	for (i = 0; i < size; i++) {
		*q++ = 0xFF;
	}
}

//	TS_VIDEO_264_NAL_LENGTH        6 bytes
//	TS_VIDEO_265_NAL_LENGTH        7 bytes

/* https://jusonqiu.github.io/2017/09/05/h264/


NAL (Network Abstraction Layer)unit

start byte 0-2 (00 00 00 01)
NAL Unit Header :
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
F:forbidden_zero_bit
NRI: nal_ref_idc
TYPE:
5  Picture parameter set
7  Sequence parameter set
8  Picture parameter set
9  Access unit delimiter

*/

void TS_Write_VidNAL(UINT32 fstskid, UINT8 *q)
{
	if (g_tsWriteLib_isKey[fstskid]) {
		*q++ = 0x00;
		*q++ = 0x00;
		*q++ = 0x00;
		*q++ = 0x01;
        if(g_tsWriteLib_codec[fstskid]== TS_CODEC_H264){
		    *q++ = 0x09; //F:0,NRI:0,TYPE:9
        } else {
			*q++ = 0x46; //F:0,TYPE:35,layer_id:0,temporal_id:1
			*q++ = 0x01;
	    }

		*q++ = 0x10;
	} else {
		*q++ = 0x00;
		*q++ = 0x00;
		*q++ = 0x00;
		*q++ = 0x01;
        if(g_tsWriteLib_codec[fstskid]== TS_CODEC_H264){
		    *q++ = 0x09; //F:0,NRI:0,TYPE:9
        } else {
			*q++ = 0x46; //F:0,TYPE:35,layer_id:0,temporal_id:1
			*q++ = 0x01;
	    }
		*q++ = 0x30;
	}
}
/*
TS_AUDIO_ADTS_LENGTH       7 bytes
https://wiki.multimedia.cx/index.php/ADTS
*/
#if 0 //ADTS is add by audio enc
void TS_Write_AudADTS(UINT8 *q, UINT32 bsSize)
{
	UINT32 full_frame_size = bsSize + TS_AUDIO_ADTS_LENGTH;
	UINT32 val = 0;

	*q++ = 0xFF;
	*q++ = 0xF9;
	//todo: AAC profile, sample rate
	*q++ = 0x54;

	val = (0x04 << 4) | ((full_frame_size >> 11) & 0x03);
	*q++ = val;
	val = (full_frame_size >> 3) & 0xFF;
	*q++ = val;
	val = ((full_frame_size << 5) & 0xE0) | 0x1F;
	*q++ = val;
	*q++ = 0xFC;
}
#endif
