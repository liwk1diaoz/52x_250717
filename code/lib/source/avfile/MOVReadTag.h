/**
    MOV file utility functions

    MOV file utility functions

    @file       MOVReadTag.h
    @ingroup    mIAVMOV
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/
#ifndef _MOVREADTAG_H
#define _MOVREADTAG_H

#if defined (__UITRON) || defined (__ECOS)
#include "Type.h"
#else
#include "kwrap/type.h"
#endif

#define MOV_mdat 0x6D646174 //1
#define MOV_moov 0x6D6F6F76 //2
#define MOV_mvhd 0x6D766864 //3
#define MOV_trak 0x7472616B //4
#define MOV_tkhd 0x746B6864 //5
#define MOV_mdia 0x6D646961 //6
#define MOV_mdhd 0x6D646864 //7
#define MOV_hdlr 0x68646C72 //8
#define MOV_minf 0x6D696E66 //9
#define MOV_vmhd 0x766D6864 //10
#define MOV_dinf 0x64696E66 //11
#define MOV_dref 0x64726566 //12
#define MOV_stbl 0x7374626C //13
#define MOV_stsd 0x73747364 //14
#define MOV_stts 0x73747473 //15
#define MOV_stss 0x73747373 //16
#define MOV_stsc 0x73747363 //17
#define MOV_stsz 0x7374737A //18
#define MOV_stco 0x7374636F //19
#define MOV_enda 0x656E6461 //20
#define MOV_wave 0x77617665 //21
#define MOV_esds 0x65736473 //22
#define MOV_mp4v 0x6D703476 //23
#define MOV_ADPCM 0x6d730002 //24
#define MOV_mjpa 0x6D6a7061 //25
//#NT#2008/06/09#JR Huang -begin
//record model name from user data
#define MOV_udta 0x75647461 //2//26
//#NT#2008/06/09#JR Huang -end
#define MOV_jpeg 0x6A706567
#define MOV_avc1 0x61766331 //avc1
#define MOV_co64 0x636f3634
//#NT#2017/02/14#Adam Su -begin
//#NT#Support H265 codec type
#define MOV_hvc1 0x68766331 // hvc1
//#NT#2017/02/14#Adam Su -end

#define MOV_skip 0x736b6970 //2008/07/04 Meg
#define MOV_mhlr 0x6D686C72
#define MOV_vide 0x76696465
#define MOV_soun 0x736F756E
#define MOV_sowt 0x736F7774 //little endian
#define MOV_twos 0x74776F73 //big endian
#define MOV_avc1 0x61766331
#define MOV_raw_ 0x72617720 //raw and 0x20
#define MOV_mp4a 0x6d703461 //aac
#define MOV_ctts 0x63747473 //ignore B frame //2010/08/10 Meg Lin

#define MOV_Probe_pnot 0x746F6E70
#define MOV_Probe_wide 0x65646977
#define MOV_Probe_mdat 0x7461646D
#define MOV_Probe_ftyp 0x70797466

#define MOVATOMERR_NOERROR  0x0000
#define MOVATOMERR_TAGERR   0x0001
#define MOVATOMERR_TRACKID  0x0002
#define MOVATOMERR_NOTMP4V  0x1000//we cannot decode not mp4v

#define MOVPARAM_MDATSIZE           0x0001
#define MOVPARAM_AUDIOSIZE          0x0002
#define MOVPARAM_MVHDDURATION       0x0003
#define MOVPARAM_SAMPLERATE         0x0004
#define MOVPARAM_FIRSTVIDEOOFFSET   0x0005
#define MOVPARAM_FIRSTAUDIOOFFSET   0x0006
#define MOVPARAM_TOTAL_VFRAMES      0x0007
#define MOVPARAM_TOTAL_AFRAMES      0x0008
#define MOVPARAM_VIDEOTIMESCALE     0x0009
#define MOVPARAM_WIDTH              0x000A
#define MOVPARAM_HEIGHT             0x000B
#define MOVPARAM_AUDIO_BITSPERSAMPLE 0x000D
#define MOVPARAM_AUDIOCODECID       0x000E
#define MOVPARAM_VIDEOTYPE          0x000F
#define MOVPARAM_VIDEOFR            0x0010//video frame rate
#define MOVPARAM_AUDCHANNELS        0x0011
#define MOVPARAM_AUDIOTYPE          0x0012//audio type, SOWT or MP4A
#define MOVPARAM_AUDIOFR            0x0013//audio frame rate//2012/11/21 Meg
#define MOVPARAM_VIDTKHDDURATION    0x0014//video tkhd duration //2013/01/29 Calvin
#define MOVPARAM_AUDTKHDDURATION    0x0015//audio tkhd duration //2013/01/29 Calvin
#define MOVPARAM_VIDEOROTATE        0x0016//video rotation information //2014/04/17 Calvin
//#NT#2013/06/11#Calvin Chang#Support Create/Modified Data inMov/Mp4 File format -begin
#define MOVPARAM_CTIME              0x0017
#define MOVPARAM_MTIME              0x0018
#define MOVPARAM_AUDFRAMENUM        0x0019
//#NT#2013/06/11#Calvin Chang -end

extern char    gMov264VidDesc[];
extern UINT16  gMov264VidDescLen;



void MOV_ReadSpecificB32Bits(UINT8 *pb, UINT32 *pvalue);//big endian
void MOV_MakeReadAtomInit(void *pMovBaseAddr);
UINT32 MOV_Read_Header(UINT8 *pb);
UINT32 MOV_Read_SearchAtom(UINT32 type, UINT32 *pNum);
void MOV_Read_SetMdatSize(UINT32 size);
UINT32 MOV_Read_GetParam(UINT32 type, UINT32 *pParam);
UINT32 MOV_Read_SearchSize(UINT64 offset, UINT8 *pKey);
//#NT#2007/09/06#Meg Lin -begin
//fixbug: if video > audio, add pure audio frame
UINT32 MOV_Read_SearchAudioFrameSize(UINT64 offset);
//#NT#2007/09/06#Meg Lin -end
//#NT#2008/07/01#Meg Lin -begin
extern UINT32 MOV_Read_Header_WithLimit(UINT8 *pb, UINT8 number);
//#NT#2008/07/01#Meg Lin -end
extern UINT32 MOV_Read_Header_FirstVideo(UINT8 *pb);
extern UINT32 MOV_Read_Header_VideoInfo(UINT8 *pb);
extern void MOVRead_ShowAllSize(void);
extern void MOVRead_ParseHeaderFirstVideo(UINT32 addr, void *pObj);
extern void MOVRead_ParseHeaderVideoInfo(UINT32 addr, void *pobj);
extern UINT32 MOVRead_SearchNextIFrame(UINT32 now, UINT32 uiSkipNum);
extern UINT32 MOVRead_SearchPrevIFrame(UINT32 now, UINT32 uiSkipNum);
extern UINT32 MOVRead_SearchNextPFrame(UINT32 now);
extern UINT32 MOVRead_GetH264IPB(void);
extern UINT32 MOVRead_GetTotalDuration(void);
extern UINT32 MOVRead_GetVidEntryAddr(void);
extern UINT32 MOVRead_GetAudEntryAddr(void);
extern UINT32 MovReadLib_Swap(UINT32 value);

#endif
