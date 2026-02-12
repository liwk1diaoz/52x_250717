/*
    Header file of mov format reader.

    Exported header file of mov format reader.

    @file       movRec.h
    @ingroup    mIAVMOV
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _MOVPLAY_H
#define _MOVPLAY_H

#if defined (__UITRON) || defined (__ECOS)
#include "MediaReadLib.h"
#include "AVFile_ParserMov.h"
#else
#include "avfile/MediaReadLib.h"
#include "avfile/AVFile_ParserMov.h"
#endif

#define MOV_PLAYDEBUG  0

#define MOV_TRACK_MAX   2
#define MOV_ATOM_MAX 0x40

#define PLAY_VIDEO_TRACK 0
#define PLAY_AUDIO_TRACK 1

#define MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT 0x01

#define MOV_MOOVERROR_MDATSIZEERROR  0x00000001
#define MOV_MOOVERROR_MDATNOFOUND    0x00000002
#define MOV_MOOVERROR_MOOVNOFOUND    0x00000004
#define MOV_MOOVERROR_IMAGESIZEERROR 0x00000008
#define MOV_MOOVERROR_MDATNOTMP4S    0x00000010

// p32TagData
#define MOV32_SKIP 0x70696B73//skip
#define MOV32_MDAT 0x7461646D//mdat
#define MOV32_MOOV 0x766F6F6D//moov
#define MOV32_FRE1 0x31657266//fre1 //2010/01/29 Meg
//#NT#2012/08/24#Hideo Lin -begin
//#NT#For MOV thumbnail atom (customized atom)
#define MOV32_THUM 0x6D756874//thum
//#NT#2012/08/24#Hideo Lin -end
#define MOV32_FREA 0x61657266//frea
#define MOV32_TIMA 0x616D6974//tima
#define MOV32_THMA 0x616D6874//thma
#define MOV32_SCRA 0x61726373//scra

#define MOVREADLIB_SETINFO_FILESIZE         1//filesize, param1= size
#define MOVREADLIB_SETINFO_MOVENTRYBUF      2//moov Entry buf, p1= buf, p2=size

typedef struct  {
	UINT16 width;
	UINT16 height;//coded_width/height

	//wave atom
	UINT32 nSamplePerSecond; // samples per second, sampleRate
	UINT32 nAvgBytesPerSecond;
	UINT16 nBlockAlign;//bytes per packet
	UINT16 nchannel;//audio only
	UINT16 wBitsPerSample;//
	UINT16 cbSize;//extra data size
	UINT16 nSamplePerBlock;// frame size
	UINT16 nNumCoeff;//coeff number

	char *codec_name;
	UINT8 codec_type; // see CODEC_TYPE_xxx
	UINT8 codec_id; // see CODEC_ID_xxx

	char *rc_eq;//rate control equation
	int rc_max_rate;//maximum bitrate
	int rc_min_rate;//minimum bitrate
	int rc_buffer_size;//decoder bitstream buffer size
} MOVPLAY_Context;

typedef struct {
	UINT32      pos;
	UINT32      size;
	UINT32      duration;
	UINT16      key_frame;
	UINT16      rev;
} MOVPLAY_Ientry;//16 byte


typedef struct {
	UINT32      mode;
	UINT32      entry;
	UINT32      timescale;
	UINT32      ctime;//create time
	UINT32      mtime;//modify time
	INT64       trackDuration;
	UINT32      sampleCount;
	UINT32      sampleSize;
	UINT32      hasKeyframes;
	UINT32      hasBframes;
	UINT16      language;
	UINT16      quality;
	UINT32      trackID;
	UINT32      tag;
	UINT32      audioSize;//total samples
	UINT32      frameNumber;
	MOVPLAY_Context *context;

	UINT32      vosLen;
	UINT8       *vosData;
	MOV_Ientry  *cluster;
	UINT32      audio_vbr;
	UINT32      type;//video: mjpg, audio: wave
	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	UINT32      uiRotateInfo;
	//#NT#2013/04/17#Calvin Chang -end
} MOVPLAY_Track;

typedef struct {
	UINT32      type;
	UINT32      offset;
	UINT32      size;
	UINT8       index;
	UINT8       parentIndex;
	UINT8       tag;
	UINT8       isAudio;
} MOV_Atom;

typedef struct {
	UINT32 tag;
	UINT8 leaf;
	UINT8 rev;
	UINT16 rev2;
	UINT16(*pFunc)(UINT8 *pb, MOV_Atom *pAtom);
} MOV_Tag;



extern MOVPLAY_Track   g_MovTagReadTrack[MOV_TRACK_MAX];//video and audio
extern CONTAINERPARSER gMovReaderContainer;
extern MEDIA_FIRST_INFO gMovFirstVInfo;
extern UINT8 gMovStscFix;
extern UINT32 g_movReadMaxIFrameSize;
extern UINT32 g_movReadIFrameCnt;

extern void MOVRead_GetMaxIFrameSize(UINT32 *psize);
extern void MOV_Read_SetVidEntryAddr(UINT32 addr, UINT32 size);//2012/11/21 Meg
extern void MOV_Read_SetAudEntryAddr(UINT32 addr, UINT32 size);//2012/11/21 Meg
extern UINT32 MOVRead_GetIFrameTotalPacketCnt(void);
extern UINT64 MOVRead_GetSecond2FilePos(UINT32 seconds);
extern void MOVRead_ParseHeader(UINT32 addr, void *ptr);
extern void MOVRead_GetVideoPosAndOffset(UINT32 findex, UINT64 *pos, UINT32 *size);
extern void MOVRead_GetAudioPosAndOffset(UINT32 findex, UINT64 *pos, UINT32 *size);
extern void MOVRead_GetVideoEntry(UINT32 findex, MOV_Ientry *ptr);
extern void MOVRead_GetAudioEntry(UINT32 findex, MOV_Ientry *ptr);
extern void MOVRead_ParseHeaderSmallSize(UINT32 addr, UINT32 *pTotalSec, UINT32 readsize);
extern UINT32 MOV_Read_SearchAudioSize(UINT64 offset);
extern void MOVRead_SetMoovErrorReason(UINT32 reason);
extern void MOV_Read_SearchMax_I_FrameSize(void);
extern UINT8 MOVRead_GetUdtaPosSize(UINT64 *ppos, UINT32 *psize);



#endif
