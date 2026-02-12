/**
    MOV tag reader

    It's the MOV tag reader

    @file       MOVReadTag.c
    @ingroup    mIAVMOV
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/

#if defined (__UITRON) || defined (__ECOS)
#include <string.h>

#include "kernel.h"
#include "FileSysTsk.h"

#include "MOVReadTag.h"
#include "MOVLib.h"
#include "movPlay.h"
#include "Debug.h"

#define __MODULE__          MOVRTag
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
#else
#include <string.h>
#include "FileSysTsk.h"

#include "MOVReadTag.h"
#include "avfile/MOVLib.h"
#include "movPlay.h"

#define __MODULE__          MOVRTag
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
#endif


UINT32  gTempReadBufBase = 0;
UINT8   gAtomNumber = 0, gMovStscFix = 0;
MOV_Offset  g_MoveReadOffset = 0, g_MOVMoovSize = 0;
MOV_Atom    g_ReadAtom[MOV_ATOM_MAX];
MOVPLAY_Track   g_MovTagReadTrack[MOV_TRACK_MAX];//video and audio
MOVPLAY_Context g_MovTagReadCon[MOV_TRACK_MAX];

MOVPLAY_Track   *gpMovReadTrack, *gpMovReadAudioTrack;
MOV_Ientry      *gp_movReadEntry, *gp_movReadAudioEntry;
UINT32          g_movReadEntryMax = 0, g_movReadAudioEntryMax = 0, g_movReadMdatSize = 0;
UINT32          g_movReadAudioSize = 0, g_movReadMaxIFrameSize = 0, g_movReadIFrameCnt = 0;
UINT32          g_movReadVideoSearchIndex = 0, g_movReadAudioSearchIndex = 0;
UINT32          g_movReadVideoFR = 0, g_movReadAudSecPerChunk = 0;
UINT16          gMovWidth = 0, gMovHeight = 0, gMov264VidDescLen = 0;
UINT8           gMovH264IPB = 0;
UINT32          g_movReadAudioFR = 0; //2012/11/21 Meg
char            gMov264VidDesc[0x40];
//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
UINT32          gMovReadTag_UserDataOffset, gMovReadTag_UserDataSize;
//#NT#2012/08/21#Calvin Chang -end
//#NT#2013/10/18#Calvin Chang#Get track width and height for sample aspect ratio -begin
UINT16          gMovTkWidth = 0, gMovTkHeight = 0;
//#NT#2013/10/18#Calvin Chang -end
extern BOOL     g_bCo64;

UINT16 MOV_Read_mvhd(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_tkhd(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_mdhd(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_hdlr(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_stsd(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_stsz(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_stss(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_stco(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_stts(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_Read_stsc(UINT8 *pb, MOV_Atom *pAtom);
//#NT#2008/06/09#JR Huang -begin
UINT16 MOV_Read_udta(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_ReadFirst_stsd(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_ReadFirst_stsz(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_ReadFirst_stco(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_ReadFirst_ctts(UINT8 *pb, MOV_Atom *pAtom);//2010/08/10 Meg Lin

UINT16 MOV_ReadVidEntry_stsz(UINT8 *pb, MOV_Atom *pAtom);
UINT16 MOV_ReadVidEntry_stco(UINT8 *pb, MOV_Atom *pAtom);

//record model name from user data
#define ATOM_PARSE_MAX  30//add to 28 //2010/08/10 Meg Lin
//#NT#2008/06/09#JR Huang -end
static const MOV_Tag mov_default_parse_table[ATOM_PARSE_MAX] = {
	/* mp4 atoms */
	{  0xFFFFFFFF, 0, 0, 0, NULL },//null
	{  MOV_mdat, 0, 0, 0, NULL },//mdat, 1
	{  MOV_moov, 1, 0, 0, NULL },//moov, 2
	{  MOV_mvhd, 0, 0, 0, MOV_Read_mvhd },//mvhd, 3
	{  MOV_trak, 1, 0, 0, NULL },//trak, 4
	{  MOV_tkhd, 0, 0, 0, MOV_Read_tkhd },//tkhd, 5
	{  MOV_mdia, 1, 0, 0, NULL },//mdia, 6
	{  MOV_mdhd, 0, 0, 0, MOV_Read_mdhd },//mdhd, 7
	{  MOV_hdlr, 0, 0, 0, MOV_Read_hdlr },//hdlr, 8
	{  MOV_minf, 1, 0, 0, NULL },//minf, 9
	{  MOV_vmhd, 0, 0, 0, NULL },//vmhd, 10
	{  MOV_dinf, 1, 0, 0, NULL },//dinf, 11
	{  MOV_vmhd, 0, 0, 0, NULL },//dref, 12
	{  MOV_stbl, 1, 0, 0, NULL },//stbl, 13
	{  MOV_stsd, 1, 0, 0, MOV_Read_stsd },//stsd, 14
	{  MOV_stts, 0, 0, 0, MOV_Read_stts },//stts, 15
	{  MOV_stss, 0, 0, 0, MOV_Read_stss },//stss, 16
	{  MOV_stsc, 0, 0, 0, MOV_Read_stsc },//stsc, 17
	{  MOV_stsz, 0, 0, 0, MOV_Read_stsz },//stsz, 18
	{  MOV_stco, 0, 0, 0, MOV_Read_stco },//stco, 19
	{  MOV_enda, 0, 0, 0, NULL },//enda, 20
	{  MOV_wave, 0, 0, 0, NULL },//wave, 21
	{  MOV_esds, 0, 0, 0, NULL },//esds, 22
	{  MOV_mp4v, 1, 0, 0, NULL },//mp4v, 23
	{  MOV_ADPCM, 0, 0, 0, NULL },//MicrosoftADPCM 0x6D730002
	{  MOV_mjpa, 0, 0, 0, NULL}, //mjpa, 25
//#NT#2008/06/09#JR Huang -begin
//record model name from user data
	{  MOV_udta, 0, 0, 0, MOV_Read_udta}, //udta, 26
//#NT#2008/06/09#JR Huang -end
//#NT#2008/07/04#Meg Lin -begin
//add parsing skip atom
	{  MOV_skip, 0, 0, 0, NULL}, //mjpa, 27
//#NT#2008/07/04#Meg Lin -end
	{  MOV_ctts, 0, 0, 0, NULL},  //ctts, 28//2010/08/10 Meg Lin
	{  MOV_co64, 0, 0, 0, MOV_Read_stco} //co64, 29
};
static const MOV_Tag mov_video_parse_table[ATOM_PARSE_MAX] = {
	/* mp4 atoms */
	{  0xFFFFFFFF, 0, 0, 0, NULL },//null
	{  MOV_mdat, 0, 0, 0, NULL },//mdat, 1
	{  MOV_moov, 1, 0, 0, NULL },//moov, 2
	{  MOV_mvhd, 0, 0, 0, MOV_Read_mvhd },//mvhd, 3
	{  MOV_trak, 1, 0, 0, NULL },//trak, 4
	{  MOV_tkhd, 0, 0, 0, MOV_Read_tkhd },//tkhd, 5
	{  MOV_mdia, 1, 0, 0, NULL },//mdia, 6
	{  MOV_mdhd, 0, 0, 0, NULL },//mdhd, 7
	{  MOV_hdlr, 0, 0, 0, NULL },//hdlr, 8
	{  MOV_minf, 1, 0, 0, NULL },//minf, 9
	{  MOV_vmhd, 0, 0, 0, NULL },//vmhd, 10
	{  MOV_dinf, 1, 0, 0, NULL },//dinf, 11
	{  MOV_vmhd, 0, 0, 0, NULL },//dref, 12
	{  MOV_stbl, 1, 0, 0, NULL },//stbl, 13
	{  MOV_stsd, 1, 0, 0, MOV_Read_stsd },//stsd, 14
	{  MOV_stts, 0, 0, 0, NULL },//stts, 15
	{  MOV_stss, 0, 0, 0, NULL },//stss, 16
	{  MOV_stsc, 0, 0, 0, NULL },//stsc, 17
	{  MOV_stsz, 0, 0, 0, MOV_ReadFirst_stsz },//stsz, 18
	{  MOV_stco, 0, 0, 0, MOV_ReadFirst_stco },//stco, 19
	{  MOV_enda, 0, 0, 0, NULL },//enda, 20
	{  MOV_wave, 0, 0, 0, NULL },//wave, 21
	{  MOV_esds, 0, 0, 0, NULL },//esds, 22
	{  MOV_mp4v, 1, 0, 0, NULL },//mp4v, 23
	{  MOV_ADPCM, 0, 0, 0, NULL },//MicrosoftADPCM 0x6D730002
	{  MOV_mjpa, 0, 0, 0, NULL}, //mjpa, 25
//#NT#2008/06/09#JR Huang -begin
//record model name from user data
	{  MOV_udta, 0, 0, 0, MOV_Read_udta}, //udta, 26
//#NT#2008/06/09#JR Huang -end
//#NT#2008/07/04#Meg Lin -begin
//add parsing skip atom
	{  MOV_skip, 0, 0, 0, NULL}, //mjpa, 27
//#NT#2008/07/04#Meg Lin -end
	{  MOV_ctts, 0, 0, 0, MOV_ReadFirst_ctts},  //ctts, 28//2010/08/10 Meg Lin
	{  MOV_co64, 0, 0, 0, MOV_ReadFirst_stco} //co64, 29
};

static const MOV_Tag mov_videoentry_parse_table[ATOM_PARSE_MAX] = {
	/* mp4 atoms */
	{  0xFFFFFFFF, 0, 0, 0, NULL },//null
	{  MOV_mdat, 0, 0, 0, NULL },//mdat, 1
	{  MOV_moov, 1, 0, 0, NULL },//moov, 2
	{  MOV_mvhd, 0, 0, 0, MOV_Read_mvhd },//mvhd, 3
	{  MOV_trak, 1, 0, 0, NULL },//trak, 4
	{  MOV_tkhd, 0, 0, 0, MOV_Read_tkhd },//tkhd, 5
	{  MOV_mdia, 1, 0, 0, NULL },//mdia, 6
	{  MOV_mdhd, 0, 0, 0, NULL },//mdhd, 7
	{  MOV_hdlr, 0, 0, 0, NULL },//hdlr, 8
	{  MOV_minf, 1, 0, 0, NULL },//minf, 9
	{  MOV_vmhd, 0, 0, 0, NULL },//vmhd, 10
	{  MOV_dinf, 1, 0, 0, NULL },//dinf, 11
	{  MOV_vmhd, 0, 0, 0, NULL },//dref, 12
	{  MOV_stbl, 1, 0, 0, NULL },//stbl, 13
	{  MOV_stsd, 1, 0, 0, MOV_Read_stsd },//stsd, 14
	{  MOV_stts, 0, 0, 0, NULL },//stts, 15
	{  MOV_stss, 0, 0, 0, MOV_Read_stss },//stss, 16
	{  MOV_stsc, 0, 0, 0, NULL },//stsc, 17
	{  MOV_stsz, 0, 0, 0, MOV_ReadVidEntry_stsz },//stsz, 18
	{  MOV_stco, 0, 0, 0, MOV_ReadVidEntry_stco },//stco, 19
	{  MOV_enda, 0, 0, 0, NULL },//enda, 20
	{  MOV_wave, 0, 0, 0, NULL },//wave, 21
	{  MOV_esds, 0, 0, 0, NULL },//esds, 22
	{  MOV_mp4v, 1, 0, 0, NULL },//mp4v, 23
	{  MOV_ADPCM, 0, 0, 0, NULL },//MicrosoftADPCM 0x6D730002
	{  MOV_mjpa, 0, 0, 0, NULL}, //mjpa, 25
//#NT#2008/06/09#JR Huang -begin
//record model name from user data
	{  MOV_udta, 0, 0, 0, MOV_Read_udta}, //udta, 26
//#NT#2008/06/09#JR Huang -end
//#NT#2008/07/04#Meg Lin -begin
//add parsing skip atom
	{  MOV_skip, 0, 0, 0, NULL}, //mjpa, 27
//#NT#2008/07/04#Meg Lin -end
	{  MOV_ctts, 0, 0, 0, MOV_ReadFirst_ctts}  //ctts, 28//2010/08/10 Meg Lin
};

#if 0
void MOV_SetTrackNumber(UINT8 number)
{

	//?? gpMovReadTrack = &g_MovTagReadTrack[number];

}
#endif

void MOV_MakeReadAtomInit(void *pMovBaseAddr)
{

	gTempReadBufBase = (UINT32)pMovBaseAddr;

	g_MoveReadOffset = 0;
	gAtomNumber = 0;
	g_MOVMoovSize = 0;
	g_movReadMaxIFrameSize = 0;
	g_movReadIFrameCnt = 0;
	g_movReadVideoSearchIndex = 0;
	g_movReadAudioSearchIndex = 0;
	gpMovReadTrack = &g_MovTagReadTrack[0];
	gpMovReadAudioTrack = &g_MovTagReadTrack[1];

	memset((UINT8 *)gpMovReadTrack, 0, sizeof(MOVPLAY_Track)*MOV_TRACK_MAX);

	g_MovTagReadTrack[0].context = &g_MovTagReadCon[0];
	g_MovTagReadTrack[1].context = &g_MovTagReadCon[1];
	memset((UINT8 *)&g_MovTagReadCon[0], 0, sizeof(MOVPLAY_Context)*MOV_TRACK_MAX);

}


static void MOV_ReadSeekOffset(UINT8 *pb, MOV_Offset pos, UINT16 type)
{
	if (type == KFS_SEEK_SET) {
		g_MoveReadOffset = pos;
	} else if (type == KFS_SEEK_CUR) {

		g_MoveReadOffset += pos;
	}
}

static void MOV_ReadSkip(UINT8 *pb, MOV_Offset size)
{
	MOV_ReadSeekOffset(pb, size, KFS_SEEK_CUR);


}

static MOV_Offset MOV_ReadTellOffset(UINT8 *pb)
{
	return g_MoveReadOffset;
}

static void MOV_ReadB64Bits(UINT8 *pb, UINT64 *pvalue)//big endian
{

	pb = (UINT8 *)gTempReadBufBase;
	*pvalue = (UINT64)(*(pb + g_MoveReadOffset)) << 56;
	*pvalue += (UINT64)(*(pb + g_MoveReadOffset + 1)) << 48;
	*pvalue += (UINT64)(*(pb + g_MoveReadOffset + 2)) << 40;
	*pvalue += (UINT64)(*(pb + g_MoveReadOffset + 3)) << 32;
	*pvalue += (UINT64)(*(pb + g_MoveReadOffset + 4)) << 24;
	*pvalue += (UINT64)(*(pb + g_MoveReadOffset + 5)) << 16;
	*pvalue += (UINT64)(*(pb + g_MoveReadOffset + 6)) << 8;
	*pvalue += (UINT64)(*(pb + g_MoveReadOffset + 7));

	g_MoveReadOffset += 8;
}

static void MOV_ReadB32Bits(UINT8 *pb, UINT32 *pvalue)//big endian
{

	pb = (UINT8 *)gTempReadBufBase;
	*pvalue = (*(pb + g_MoveReadOffset)) << 24;
	*pvalue += (*(pb + g_MoveReadOffset + 1)) << 16;
	*pvalue += (*(pb + g_MoveReadOffset + 2)) << 8;
	*pvalue += (*(pb + g_MoveReadOffset + 3)) ;

	g_MoveReadOffset += 4;
}

static void MOV_ReadB16Bits(UINT8 *pb, UINT16 *pvalue)//big endian
{

	pb = (UINT8 *)gTempReadBufBase;
	*pvalue = (*(pb + g_MoveReadOffset)) << 8;
	*pvalue += (*(pb + g_MoveReadOffset + 1)) ;

	g_MoveReadOffset += 2;
}
static void MOV_ReadB8Bits(UINT8 *pb, UINT8 *pvalue)//big endian
{

	pb = (UINT8 *)gTempReadBufBase;
	*pvalue = *(pb + g_MoveReadOffset);

	g_MoveReadOffset += 1;
}

#if 0
void MOV_ReadTag(UINT8 *pb, char *pString)//big endian
{

	pb = (UINT8 *)gTempReadBufBase;
	*pString++ = *(pb + g_MoveReadOffset);
	*pString++ = *(pb + g_MoveReadOffset + 1);
	*pString++ = *(pb + g_MoveReadOffset + 2) ;
	*pString++ = *(pb + g_MoveReadOffset + 3) ;

	g_MoveReadOffset += 4;
}
#endif

static UINT8 MOV_CheckType(UINT32 type)
{
	UINT8 i = 0;
	for (i = 0; i < ATOM_PARSE_MAX; i++) {

		if (type == mov_default_parse_table[i].tag) {
			return i;
		}
	}
	return 0;
}
void MOV_ReadSpecificB32Bits(UINT8 *pb, UINT32 *pvalue)//big endian
{

	*pvalue = (*(pb)) << 24;
	*pvalue += (*(pb + 1)) << 16;
	*pvalue += (*(pb + 2)) << 8;
	*pvalue += (*(pb + 3)) ;


}



static void MOV_SetIndexCodeType(MOV_Atom *pAtom, UINT8 type)
{
	MOV_Atom *pParentAtom, *pFirstAtom, *pChildAtom;
	UINT8    trakIndex = 0, i;

	if (type == PLAY_VIDEO_TRACK) { //no need to set isAudio
		return;
	}
	pAtom->isAudio = type;

	pFirstAtom = &g_ReadAtom[0];
	pParentAtom = &g_ReadAtom[0] + pAtom->parentIndex;
	//search above
	while (pParentAtom) {
		pParentAtom = pFirstAtom + pAtom->parentIndex;
		if (pParentAtom->type == MOV_trak) {
			pParentAtom->isAudio = type;
			trakIndex = pParentAtom->index;
			break;
		} else {
			break;
		}
	}
	//search down

	if (trakIndex) {
		for (i = 0; i <= gAtomNumber; i++) {
			pChildAtom = pFirstAtom + i;
			pParentAtom = pFirstAtom + (pChildAtom->parentIndex);
			if (pParentAtom->isAudio) { //parent is Audio
				pChildAtom->isAudio = type;
			}
		}

	}
}


#if 0
#pragma mark -
#endif

static UINT8 MOV_ReadParseAtom(UINT8 *pb, MOV_Atom *pAtom, UINT8 *newAtom)
{
	UINT8 notFinished = 1, first = 0, tag;
	UINT32 size, type, parentIndex;
	MOV_Offset pos, pos_max, next_atom_pos, start_pos;

	start_pos = pAtom->offset;
	MOV_ReadSeekOffset(pb, start_pos, KFS_SEEK_SET);
	pos_max = pAtom->size - 8; //minus pAtom size, type
	parentIndex = pAtom->index;
	while (notFinished) {
		MOV_ReadB32Bits(0, &size);
		MOV_ReadB32Bits(0, &type);
		tag = MOV_CheckType(type);

		if (size && tag) {
			gAtomNumber += 1;
			if (first == 0) {
				first = gAtomNumber;
			}

			if(gAtomNumber >= MOV_ATOM_MAX){
				DBG_ERR("atom out of index(%lu)\r\n", gAtomNumber);
			}

			g_ReadAtom[gAtomNumber].size = size;
			g_ReadAtom[gAtomNumber].type = type;
			pos = MOV_ReadTellOffset(pb);
			g_ReadAtom[gAtomNumber].offset = pos;
			g_ReadAtom[gAtomNumber].index = gAtomNumber;
			g_ReadAtom[gAtomNumber].tag = tag;
			g_ReadAtom[gAtomNumber].parentIndex = parentIndex;
			//#NT#2008/07/01#Meg Lin -begin
			g_ReadAtom[gAtomNumber].isAudio = 0;
			//#NT#2008/07/01#Meg Lin -end
		} else if (size) { //only size, unknown atom
			;//do nothing
		} else { //no size, must not atom, finish..
			break;
		}
		pos = MOV_ReadTellOffset(pb);
		next_atom_pos = pos + size - 8;
		if (next_atom_pos >= (start_pos + pos_max)) {
			break;
		} else {
			MOV_ReadSeekOffset(pb, next_atom_pos, KFS_SEEK_SET);
		}
	}
	*newAtom = first;
	return (gAtomNumber - pAtom->index);


}

static UINT32 MOV_ReadMoovAtom(UINT8 *pb)
{
	UINT32 size, type;
	UINT8  tag;
	MOV_Offset pos;
	gAtomNumber = 1;

	MOV_ReadB32Bits(0, &size);
	MOV_ReadB32Bits(0, &type);
	if (type == 0x6D6F6F76) {
		g_MOVMoovSize = size;
	} else {
		return 0;//not moov
	}
	tag = MOV_CheckType(type);

	if (size && tag) {

		if(gAtomNumber >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", gAtomNumber);
		}

		g_ReadAtom[gAtomNumber].size = size;
		g_ReadAtom[gAtomNumber].type = type;
		pos = MOV_ReadTellOffset(pb);
		g_ReadAtom[gAtomNumber].offset = pos;
		g_ReadAtom[gAtomNumber].index = 1;
		g_ReadAtom[gAtomNumber].tag = tag;
		g_ReadAtom[gAtomNumber].parentIndex = 0;
		return 1;
	}
	return 0;
}


UINT32 MOV_Read_Header(UINT8 *pb)
{

	UINT32 moov_yes;
	UINT8  tagindex, i;
	UINT8  now_index, new_atom_index_L1, new_atom_index_L2;

	g_movReadVideoFR = 0;
	g_movReadAudioFR = 0;//2012/11/21 Meg
	gMovH264IPB = 0;
	moov_yes = MOV_ReadMoovAtom(pb);
	if (moov_yes) {
		MOV_ReadParseAtom(pb, &g_ReadAtom[1], &new_atom_index_L1);
	} else { //no moov atom
		MOVRead_SetMoovErrorReason(MOV_MOOVERROR_MOOVNOFOUND);
		return MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT;
	}

	now_index = new_atom_index_L1;
	//new_atom_index_L2 = new_atom_index_L1;
	while (now_index <= gAtomNumber) {

		if(now_index >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", now_index);
		}

		MOV_ReadParseAtom(pb, &g_ReadAtom[now_index], &new_atom_index_L2);
		now_index += 1;
	}


	for (i = 0; i <= gAtomNumber; i++) {

		if(i >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", i);
		}

		tagindex = g_ReadAtom[i].tag;
		if (tagindex) {
			DBG_IND("parse tagindex=%lx!\r\n", tagindex);
			if (mov_default_parse_table[tagindex].pFunc) {
				mov_default_parse_table[tagindex].pFunc(pb, &g_ReadAtom[i]);
			}
		}
		DBG_IND("parse %d finished!\r\n", i);
	}


	return 0;

}
//#NT#2008/07/01#Meg Lin -begin

UINT32 MOV_Read_Header_WithLimit(UINT8 *pb, UINT8 number)
{

	UINT32 moov_yes;
	UINT8  tagindex, i, num = 0;
	UINT8  now_index, new_atom_index_L1, new_atom_index_L2;

	g_movReadVideoFR = 0;
	g_movReadAudioFR = 0;

	moov_yes = MOV_ReadMoovAtom(pb);
	if (moov_yes) {
		MOV_ReadParseAtom(pb, &g_ReadAtom[1], &new_atom_index_L1);
	} else { //no moov atom
		MOVRead_SetMoovErrorReason(MOV_MOOVERROR_MOOVNOFOUND);
		return MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT;
	}

	now_index = new_atom_index_L1;
	//new_atom_index_L2 = new_atom_index_L1;
	while (now_index <= gAtomNumber) {

		if(now_index >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", now_index);
		}

		MOV_ReadParseAtom(pb, &g_ReadAtom[now_index], &new_atom_index_L2);
		now_index += 1;
	}
	if (gAtomNumber > number) {
		num = number;
	}

	for (i = 0; i <= num; i++) {

		if(i >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", i);
		}

		tagindex = g_ReadAtom[i].tag;
		if (tagindex) {
			DBG_IND("parse tagindex=%lx!\r\n", tagindex);
			if (mov_default_parse_table[tagindex].pFunc) {
				mov_default_parse_table[tagindex].pFunc(pb, &g_ReadAtom[i]);
			}
		}
		DBG_IND("parse %d finished!\r\n", i);
	}


	return 0;

}
//#NT#2008/07/01#Meg Lin -end
UINT32 MOV_Read_Header_FirstVideo(UINT8 *pb)
{

	UINT32 moov_yes;
	UINT8  tagindex, i;
	UINT8  now_index, new_atom_index_L1, new_atom_index_L2;

	g_movReadVideoFR = 0;
	g_movReadAudioFR = 0;

	moov_yes = MOV_ReadMoovAtom(pb);
	if (moov_yes) {
		MOV_ReadParseAtom(pb, &g_ReadAtom[1], &new_atom_index_L1);
	} else { //no moov atom
		MOVRead_SetMoovErrorReason(MOV_MOOVERROR_MOOVNOFOUND);
		return MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT;
	}

	now_index = new_atom_index_L1;
	//new_atom_index_L2 = new_atom_index_L1;
	while (now_index <= gAtomNumber) {

		if(now_index >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", now_index);
		}

		MOV_ReadParseAtom(pb, &g_ReadAtom[now_index], &new_atom_index_L2);
		now_index += 1;
	}


	for (i = 0; i <= gAtomNumber; i++) {

		if(i >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", i);
		}

		tagindex = g_ReadAtom[i].tag;
		if (tagindex) {
			//DBG_MSG("parse tagindex=%lx!\r\n", tagindex);
			if (mov_video_parse_table[tagindex].pFunc) {
				mov_video_parse_table[tagindex].pFunc(pb, &g_ReadAtom[i]);
			}
		}
		//DBG_MSG("parse %d finished!\r\n", i);
	}


	return 0;

}

UINT32 MOV_Read_Header_VideoInfo(UINT8 *pb)
{

	UINT32 moov_yes;
	UINT8  tagindex, i;
	UINT8  now_index, new_atom_index_L1, new_atom_index_L2;

	g_movReadVideoFR = 0;
	g_movReadAudioFR = 0;

	moov_yes = MOV_ReadMoovAtom(pb);
	if (moov_yes) {
		MOV_ReadParseAtom(pb, &g_ReadAtom[1], &new_atom_index_L1);
	} else { //no moov atom
		MOVRead_SetMoovErrorReason(MOV_MOOVERROR_MOOVNOFOUND);
		return MOV_DEC_FIRSTFRAME_ERR_MOVFORMAT;
	}

	now_index = new_atom_index_L1;
	//new_atom_index_L2 = new_atom_index_L1;
	while (now_index <= gAtomNumber) {

		if(now_index >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", now_index);
		}

		MOV_ReadParseAtom(pb, &g_ReadAtom[now_index], &new_atom_index_L2);
		now_index += 1;
	}


	for (i = 0; i <= gAtomNumber; i++) {

		if(i >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", i);
		}

		tagindex = g_ReadAtom[i].tag;
		if (tagindex) {
			//DBG_MSG("parse tagindex=%lx!\r\n", tagindex);
			if (mov_videoentry_parse_table[tagindex].pFunc) {
				mov_videoentry_parse_table[tagindex].pFunc(pb, &g_ReadAtom[i]);
			}
		}
		//DBG_MSG("parse %d finished!\r\n", i);
	}


	return 0;

}

UINT16 MOV_Read_mvhd(UINT8 *pb, MOV_Atom *pAtom)
{

	UINT32 rvalue, rvalue2;
	UINT32 version;
	if (pAtom->type != MOV_mvhd) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadB32Bits(pb, &version);//version and flag
	if (version & 0x01000000) { //version 1, time should be 64 bytes
		version = 1;//??
	} else {
		version = 0;
		MOV_ReadB32Bits(pb, &gpMovReadTrack->ctime);// creation time
		MOV_ReadB32Bits(pb, &gpMovReadTrack->mtime);// modification time

	}
	MOV_ReadB32Bits(pb, &gpMovReadTrack->timescale);// timescale

	if (version) {
		MOV_ReadB32Bits(pb, &rvalue);
		MOV_ReadB32Bits(pb, &rvalue2);
		//?? warning gpMovReadTrack->trackDuration = (rvalue * 0x100000000L)||(rvalue2);
	} else {
		MOV_ReadB32Bits(pb, &rvalue);
		gpMovReadTrack->trackDuration = rvalue;//track duration
#if MOV_PLAYDEBUG
		DBG_IND("dur=%d sec\r\n", gpMovReadTrack->trackDuration / gpMovReadTrack->timescale);
#endif
	}
	MOV_ReadSkip(pb, 16);//rate, volume, reserved
	MOV_ReadSkip(pb, 36);//matrix


	MOV_ReadSkip(pb, 24);//preview time, current time...
	MOV_ReadB32Bits(pb, &rvalue);//next trackID
	if (rvalue == 0) {
		return MOVATOMERR_TRACKID;
	}



	return MOVATOMERR_NOERROR;

}
UINT16 MOV_Read_tkhd(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 rvalue, rvalue2;
	UINT32 version, volume;
	UINT32 trackduration = 0;
	if (pAtom->type != MOV_tkhd) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadB32Bits(pb, &version);//version and flag
	if (version & 0x01000000) { //version 1, time should be 64 bytes
		version = 1;//??
	} else {
		version = 0;
		MOV_ReadB32Bits(pb, &gpMovReadTrack->ctime);// creation time
		MOV_ReadB32Bits(pb, &gpMovReadTrack->mtime);// modification time

	}

	MOV_ReadB32Bits(pb, &gpMovReadTrack->trackID);// trackID
	if (gpMovReadTrack->trackID == 0) {
		return MOVATOMERR_TRACKID;
	}
	MOV_ReadSkip(pb, 4);//reversed
	if (version) {
		MOV_ReadB32Bits(pb, &rvalue);
		MOV_ReadB32Bits(pb, &rvalue2);
		//?? warning gpMovReadTrack->trackDuration = (rvalue * 0x100000000L)||(rvalue2);
	} else {
		MOV_ReadB32Bits(pb, &rvalue);
		//gpMovReadTrack->trackDuration = rvalue;//track duration
		trackduration = rvalue;
	}
	MOV_ReadSkip(pb, 8);//reversed
	MOV_ReadSkip(pb, 4);//layer, alternate group

	MOV_ReadB32Bits(pb, &volume);//volume
	if (volume == 0) { //video track
		//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
		/*
		   normal
		    [ 1 0 0
		      0 1 0
		      0 0 1]
		   90«×
		    [  0  1 0
		      -1  0 0
		       0  0 1]
		   180 «×
		    [-1   0 0
		     0  -1 0
		     0   0 1]
		   270 «×
		    [ 0 -1 0
		      1  0 0
		      0  0 1]
		*/
		MOV_ReadB32Bits(pb, &rvalue);
		if (rvalue == 0x00010000) { //1: 0 degree
			g_MovTagReadTrack[0].uiRotateInfo = 0; // 0
			MOV_ReadSkip(pb, 32);//skip 32 bytes
		} else if (rvalue == 0xFFFF0000) { // -1: 180 degree
			g_MovTagReadTrack[0].uiRotateInfo = 180; // 180
			MOV_ReadSkip(pb, 32);//skip 32 bytes
		} else { // 0: 90 or 270 degree
			//90 or 270
			MOV_ReadB32Bits(pb, &rvalue);
			if (rvalue == 0xFFFF0000) {
				g_MovTagReadTrack[0].uiRotateInfo = 270;    // 270
			} else {
				g_MovTagReadTrack[0].uiRotateInfo = 90;    // 90
			}

			MOV_ReadSkip(pb, 28);//skip
		}
		//#NT#2013/04/17#Calvin Chang -end

		//#NT#2013/10/18#Calvin Chang#Get track width and height for sample aspect ratio -begin
		MOV_ReadB16Bits(pb, &gMovTkWidth);// track width
		MOV_ReadSkip(pb, 2);//reversed
		MOV_ReadB16Bits(pb, &gMovTkHeight);// track height
		MOV_ReadSkip(pb, 2);//reversed
		//#NT#2013/10/18#Calvin Chang -end
		g_MovTagReadTrack[0].trackDuration = trackduration; // video
	} else { //if (volume)//audio track
		MOV_ReadSkip(pb, 36);//skip matrix in audio track
		g_MovTagReadTrack[1].uiRotateInfo = 0;
		MOV_SetIndexCodeType(pAtom, PLAY_AUDIO_TRACK);
		g_MovTagReadTrack[1].trackDuration = trackduration; // audio
	}
	return MOVATOMERR_NOERROR;

}

UINT16 MOV_Read_mdhd(UINT8 *pb, MOV_Atom *pAtom)
{

	UINT32 version, newtimescale, newduration;
	if (pAtom->type != MOV_mdhd) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadB32Bits(pb, &version);//version and flag
	if (version & 0x01000000) { //version 1, time should be 64 bytes
		version = 1;//??
	} else {
		version = 0;
		MOV_ReadB32Bits(pb, &gpMovReadTrack->ctime);// creation time
		MOV_ReadB32Bits(pb, &gpMovReadTrack->mtime);// modification time

	}
	MOV_ReadB32Bits(pb, &newtimescale);// timescale
	MOV_ReadB32Bits(pb, &newduration);// duration
	if ((gpMovReadTrack->timescale != 0) && (newtimescale != gpMovReadTrack->timescale)) {
		if (newduration == 25) { //25 frames per second
			gpMovReadTrack->entry = newduration;
		}
	}
	MOV_ReadB16Bits(pb, &gpMovReadTrack->language);//lauguage
	MOV_ReadB16Bits(pb, &gpMovReadTrack->quality);//quality



	return MOVATOMERR_NOERROR;

}

UINT16 MOV_Read_hdlr(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 rvalue, rvalue2;
	UINT32 version;
	if (pAtom->type != MOV_hdlr) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadB32Bits(pb, &version);//version and flag
	MOV_ReadB32Bits(pb, &rvalue);//component type
	if (rvalue == MOV_mhlr) { //media handler
		MOV_ReadB32Bits(pb, &rvalue2);//sub type
		if (rvalue2 == MOV_vide) {
			gpMovReadTrack->context->codec_type = PLAY_VIDEO_TRACK;
		} else if (rvalue2 == MOV_soun) {
			gpMovReadAudioTrack->context->codec_type = PLAY_AUDIO_TRACK;
		}
	}
	//else MOV_dhlr data handler
	return MOVATOMERR_NOERROR;

}
UINT16 MOV_Read_stsd(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 rvalue, i, j;
	UINT16 sampleRate, h264DescLen, bitPerSample;
	//#NT#2012/04/26#Calvin Chang -begin
	//#NT#Modify to fit 1 SPS + N PPS
	UINT16 uiSPSSize = 0, uiPPSSize = 0;
	UINT8  uiPPSNum = 0;
	//#NT#2012/04/26#Calvin Chang -end
	UINT32 uiavcCType = 0; //avcC box type
	UINT8 byteV;
	char *ptr;
	if (pAtom->type != MOV_stsd) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 8);//version and flag , reserved
	MOV_ReadB32Bits(pb, &rvalue);//if mp4v atom, size of mp4v
	MOV_ReadB32Bits(pb, &rvalue);//if mp4v atom, tag of mp4v
#if MOV_PLAYDEBUG
	DBG_IND("rvalue=0x%lx!\r\n", rvalue);
#endif
	if (pAtom->isAudio == 0) { //video
		//DBG_MSG("rvalue=0x%lx!\r\n", rvalue);
		if (rvalue == MOV_avc1) {
			//#NT#2012/04/26#Calvin Chang -begin
			//#NT#Modify to fit 1 SPS + N PPS
			// 'avc1' type (H.264 video)
			g_MovTagReadTrack[0].type = MEDIAPLAY_VIDEO_H264;

			MOV_ReadSkip(pb, 0x04); //reserved
			MOV_ReadSkip(pb, 0x04); //data-reference index
			MOV_ReadSkip(pb, 0x02); //version
			MOV_ReadSkip(pb, 0x02); //level
			MOV_ReadSkip(pb, 0x04); //vendor reserved
			MOV_ReadSkip(pb, 0x04); //Temporal Quality
			MOV_ReadSkip(pb, 0x04); //Spatial Quality
			MOV_ReadB16Bits(pb, &gMovWidth);// width
			MOV_ReadB16Bits(pb, &gMovHeight);// height
			MOV_ReadSkip(pb, 0x04); //Horizontal resolution 72dpi
			MOV_ReadSkip(pb, 0x04); //Vertical resolution 72dpi
			MOV_ReadSkip(pb, 0x04); //Data size, must be 0
			MOV_ReadSkip(pb, 0x02); //frame count, 1
			MOV_ReadSkip(pb, 0x20); //codec name
			MOV_ReadSkip(pb, 0x02); //depth, 32
			MOV_ReadSkip(pb, 0x02); //default color table, (-1)

			// AVC Decoder Configuration Atom ('avcC')
			MOV_ReadSkip(pb, 0x04); // size
			//#NT#2014/03/10#Calvin Chang#Identify 'avcC' type to fix CPU exception when other company mov file read -begin
			//MOV_ReadSkip(pb, 0x04); // avcC box type
			MOV_ReadB32Bits(pb, &uiavcCType);// avcC box
			if (uiavcCType != 0x61766343) { //avcC atom type
				DBG_ERR("H264 atom type is not correct 0x%X!\r\n", uiavcCType);
				gMov264VidDescLen = 0;
				return MOVATOMERR_TAGERR;
			}
			//#NT#2014/03/10#Calvin Chang -end
			MOV_ReadSkip(pb, 0x01); // version
			MOV_ReadSkip(pb, 0x01); // AVC profile indication
			MOV_ReadSkip(pb, 0x01); // profile compatibility
			MOV_ReadSkip(pb, 0x01); // AVC level indication
			MOV_ReadSkip(pb, 0x02); // 0xFF: NALU length, 0xE1: SPS number (lower 5-bit)
			MOV_ReadB16Bits(pb, &uiSPSSize);// SPS size

			ptr = gMov264VidDesc;

			*ptr++ = 0x00;
			*ptr++ = 0x00;
			*ptr++ = 0x00;
			*ptr++ = 0x01; //start code

			if (uiSPSSize > (0x40 - 4)) {
				DBG_ERR("SPS size is too large %d!\r\n", uiSPSSize);
				gMov264VidDescLen = 0;
				return MOVATOMERR_TAGERR;
			}

			for (i = 0; i < uiSPSSize; i++) {
				MOV_ReadB8Bits(pb, &byteV);
				*ptr++ = byteV;
			}
			h264DescLen = uiSPSSize + 4;

			MOV_ReadB8Bits(pb, &uiPPSNum);// PPS number
			for (i = 0; i < uiPPSNum; i++) {
				MOV_ReadB16Bits(pb, &uiPPSSize); // PPS size

				if ((h264DescLen + uiPPSSize) > 0x40) {
					DBG_ERR("SPS + PPS size is too large 0x%x!\r\n", (h264DescLen + uiPPSSize));
					gMov264VidDescLen = 0;
					return MOVATOMERR_TAGERR;
				}

				*ptr++ = 0x00;
				*ptr++ = 0x00;
				*ptr++ = 0x00;
				*ptr++ = 0x01; //start code

				for (j = 0; j < uiPPSSize; j++) {
					MOV_ReadB8Bits(pb, &byteV);
					*ptr++ = byteV;
				}
				h264DescLen += uiPPSSize + 4;
			}

			gMov264VidDescLen = h264DescLen;
			//#NT#2012/04/26#Calvin Chang -end

		} else if (rvalue == MOV_hvc1) { // 'hvc1' type (H.265 video)
			//#NT#2017/02/14#Adam Su -begin
			//#NT#Support H.265 codec type
			g_MovTagReadTrack[0].type = MEDIAPLAY_VIDEO_H265;

			MOV_ReadSkip(pb, 0x04); //reserved
			MOV_ReadSkip(pb, 0x04); //data-reference index
			MOV_ReadSkip(pb, 0x02); //version
			MOV_ReadSkip(pb, 0x02); //level
			MOV_ReadSkip(pb, 0x04); //vendor reserved
			MOV_ReadSkip(pb, 0x04); //Temporal Quality
			MOV_ReadSkip(pb, 0x04); //Spatial Quality
			MOV_ReadB16Bits(pb, &gMovWidth);// width
			MOV_ReadB16Bits(pb, &gMovHeight);// height
			//#NT#2017/02/14#Adam Su -end
		} else if ((rvalue == MOV_mjpa) || (rvalue == MOV_jpeg)) {
			MOV_ReadSkip(pb, 24);
			MOV_ReadB16Bits(pb, &gMovWidth);// width
			MOV_ReadB16Bits(pb, &gMovHeight);// width
			//gMovHeight = gMovHeight/2;
			DBG_IND("wid=%d, hei=%d!\r\n", gMovWidth, gMovHeight);
			g_MovTagReadTrack[0].type = MEDIAPLAY_VIDEO_MJPG;
		} else {
			MOVRead_SetMoovErrorReason(MOV_MOOVERROR_MDATNOTMP4S);
			return MOVATOMERR_NOTMP4V;
		}
	} else { //audio
		if ((rvalue == MOV_sowt) || (rvalue == MOV_raw_)) {
			MOV_ReadSkip(pb, 0x10);//other info
			//2 byte channels
			MOV_ReadB16Bits(pb, &bitPerSample);// channels
			g_MovTagReadTrack[1].context->nchannel = bitPerSample;
			//2 byte bitPerSample
			MOV_ReadB16Bits(pb, &bitPerSample);// bitPerSample
			g_MovTagReadTrack[1].context->wBitsPerSample = bitPerSample;
			MOV_ReadSkip(pb, 0x4);//other info
			MOV_ReadB16Bits(pb, &sampleRate);// width
			g_MovTagReadTrack[1].context->nSamplePerSecond = sampleRate;
			if (bitPerSample == 0x8) {
				g_MovTagReadTrack[1].context->codec_id = MEDIAAUDIO_CODEC_RAW8;
			} else {
				g_MovTagReadTrack[1].context->codec_id = MEDIAAUDIO_CODEC_SOWT;
			}
		} else if (rvalue == MOV_mp4a) {
			MOV_ReadSkip(pb, 0x10);//other info
			//2 byte channels
			MOV_ReadB16Bits(pb, &bitPerSample);// channels
			g_MovTagReadTrack[1].context->nchannel = bitPerSample;
			//2 byte bitPerSample
			MOV_ReadB16Bits(pb, &bitPerSample);// bitPerSample
			g_MovTagReadTrack[1].context->wBitsPerSample = bitPerSample;
			MOV_ReadSkip(pb, 0x4);//other info
			MOV_ReadB16Bits(pb, &sampleRate);// width
			g_MovTagReadTrack[1].context->nSamplePerSecond = sampleRate;
			g_MovTagReadTrack[1].context->codec_id = MEDIAAUDIO_CODEC_MP4A;
		} else {
			g_MovTagReadTrack[1].context->codec_id = MEDIAAUDIO_CODEC_XX;
		}
	}
	return MOVATOMERR_NOERROR;
}
UINT16 MOV_Read_stsc(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 frame_num, i, j, old_start = 0, start = 0, index, number;
	UINT32 diff_duration = 0, chunksize, sampleRate;
	MOV_Ientry   *pEntry = 0;
	if (pAtom->type != MOV_stsc) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved
	MOV_ReadB32Bits(pb, &frame_num);//if stsc, get frame number
	if (pAtom->isAudio == 1) { //audio
		if (frame_num > g_movReadAudioEntryMax) {
			frame_num = g_movReadAudioEntryMax;
		}

		index = 0;
		MOV_ReadB32Bits(pb, &old_start);//must be 1
		MOV_ReadB32Bits(pb, &diff_duration);
		MOV_ReadSkip(pb, 4);//desc ID
		chunksize = diff_duration;//1st duration is chunksize

		//#NT#2008/04/15#Meg Lin -begin
		//add MovMjpg Parse total seconds
		if (frame_num > 0) {
			for (i = 0; i < frame_num - 1; i++) {
				MOV_ReadB32Bits(pb, &start);//must be 1

				number = start - old_start;
				for (j = 0; j < number; j++) {
					pEntry = gp_movReadAudioEntry + index;
					if (g_MovTagReadTrack[1].context->codec_id == MEDIAAUDIO_CODEC_SOWT) {
						//#NT#2009/12/21#Meg Lin -begin
						//#NT#new feature: rec stereo
						pEntry->size = diff_duration * 2 * g_MovTagReadTrack[1].context->nchannel; //sample to bytes!!
						//#NT#2009/12/21#Meg Lin -end
					} else if (g_MovTagReadTrack[1].context->codec_id == MEDIAAUDIO_CODEC_RAW8) {
						pEntry->size = diff_duration;
					}
					//DBG_MSG("index =%ld, duration=%ld\r\n", index, diff_duration);
					index += 1;
				}
				old_start = start;
				MOV_ReadB32Bits(pb, &diff_duration);
				MOV_ReadSkip(pb, 4);//desc ID

			}
		}
		//#NT#2008/04/15#Meg Lin -end
		sampleRate = g_MovTagReadTrack[1].context->nSamplePerSecond;
		if (sampleRate != 0) {
			if ((chunksize > sampleRate) && (chunksize % sampleRate != 0)) {
				DBG_ERR("this file parsing error!!!!! 1=%d, 2=%d!\r\n", chunksize, sampleRate);
			} else {
				g_movReadAudSecPerChunk = chunksize / sampleRate;
			}
		} else
			DBG_ERR("this file parsing error!!!!! sampleRate=%d!\r\n", sampleRate);

		//last one
		pEntry = gp_movReadAudioEntry + index;
		if (g_MovTagReadTrack[1].context->codec_id == MEDIAAUDIO_CODEC_SOWT) {
			//#NT#2009/12/21#Meg Lin -begin
			//#NT#new feature: rec stereo
			pEntry->size = diff_duration * 2 * g_MovTagReadTrack[1].context->nchannel;
			//#NT#2009/12/21#Meg Lin -end
		} else if (g_MovTagReadTrack[1].context->codec_id == MEDIAAUDIO_CODEC_RAW8) {
			pEntry->size = diff_duration;
		}
	} else { //video
		UINT32 num1, num2;
		if (frame_num > g_movReadAudioEntryMax) {
			frame_num = g_movReadAudioEntryMax;
		}

		index = 0;
		MOV_ReadB32Bits(pb, &old_start);//must be 1
		MOV_ReadB32Bits(pb, &num1);
		MOV_ReadSkip(pb, 4);//desc ID
		if (frame_num == 1) {
			gMovStscFix = num1; //all seconds with the same frame num
		}
		frame_num -= 1;

		while (frame_num > 0) {

			MOV_ReadB32Bits(pb, &start);//must be 1
			MOV_ReadB32Bits(pb, &num2);
			MOV_ReadSkip(pb, 4);//desc ID
			frame_num -= 1;

			for (j = 0; j < (start - old_start); j++) {
				for (i = 0; i < num1; i++) {
					pEntry = gp_movReadEntry + index + i;
					pEntry->pos = old_start + j;
				}
				index += num1;
			}
			old_start = start;
			num1 = num2;
		}
		//last one
		for (i = 0; i < num1; i++) {
			pEntry = gp_movReadEntry + index + i;
			pEntry->pos = old_start;
		}
		index += num1;


		//debug
#if 0
		for (i = 0; i < index; i++) {
			pEntry = gp_movReadEntry + i;
			//DBG_MSG("pEntry->pos = 0x%lx\r\n", pEntry->pos);
		}
#endif
	}

	return MOVATOMERR_NOERROR;
}
UINT16 MOV_Read_stsz(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 frame_num, i, diff_size;
	MOV_Ientry   *pEntry = 0;
	if (pAtom->type != MOV_stsz) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved


	if (pAtom->isAudio == 0) { //video
		MOV_ReadSkip(pb, 4);//reserved

		MOV_ReadB32Bits(pb, &frame_num);//if stsz, get frame number
		//DBG_MSG("g_movReadEntryMax=%ld, frame_num=%ld!\r\n",g_movReadEntryMax, frame_num);
		if (frame_num > g_movReadEntryMax) {
			frame_num = g_movReadEntryMax;
			DBG_ERR("framenum not enough! max=%d, frameN=%d!\r\n", g_movReadEntryMax, frame_num);
			DBG_ERR("need more size for MEDIAREADLIB_SETINFO_ENTRYBUF!! \r\n");
		}
		g_MovTagReadTrack[0].frameNumber = frame_num;
		for (i = 0; i < frame_num; i++) {
			pEntry = gp_movReadEntry + i;
			MOV_ReadB32Bits(pb, &diff_size);
			pEntry->size = diff_size;
		}
	} else if (g_MovTagReadTrack[1].context->codec_id == MEDIAAUDIO_CODEC_MP4A) {
		MOV_ReadSkip(pb, 4);//reserved

		MOV_ReadB32Bits(pb, &frame_num);//if stsz, get frame number
		//DBG_MSG("g_movReadEntryMax=%ld, frame_num=%ld!\r\n",g_movReadEntryMax, frame_num);
		if (frame_num > g_movReadAudioEntryMax) {
			frame_num = g_movReadAudioEntryMax;
			DBG_ERR("framenum not enough! max=%d, frameN=%d!\r\n", g_movReadEntryMax, frame_num);
			DBG_ERR("need more size for MEDIAREADLIB_SETINFO_ENTRYBUF!! \r\n");
		}
		g_MovTagReadTrack[1].frameNumber = frame_num;
		for (i = 0; i < frame_num; i++) {
			pEntry = gp_movReadAudioEntry + i;
			MOV_ReadB32Bits(pb, &diff_size);
			pEntry->size = diff_size;
		}
	} else {
		MOV_ReadB32Bits(pb, &frame_num);//if stsz, get frame number
#if MOV_PLAYDEBUG
		DBG_ERR("g_movReadAudioEntryMax=%ld, frame_num=%ld!\r\n", g_movReadAudioEntryMax, frame_num);
#endif

		if (frame_num > g_movReadAudioEntryMax) {
			frame_num = g_movReadAudioEntryMax;
			DBG_ERR("framenum not enough! max=%d, frameN=%d!\r\n", g_movReadEntryMax, frame_num);
			DBG_ERR("need more size for MEDIAREADLIB_SETINFO_ENTRYBUF!! \r\n");
		}
		if (frame_num == 1) { //our mov
			MOV_ReadB32Bits(pb, &diff_size);
			g_MovTagReadTrack[1].audioSize = diff_size;
		}
	}

	return MOVATOMERR_NOERROR;
}
UINT16 MOV_Read_stss(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 sys_num, i, frame_num;
	UINT8 f1 = 0, f2 = 0, f3 = 0;
	MOV_Ientry   *pEntry = 0;
	if (pAtom->type != MOV_stss) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved
	MOV_ReadB32Bits(pb, &sys_num);//if stss, get sys number
	if (sys_num > g_movReadEntryMax) {
		sys_num = g_movReadEntryMax;
	}
	g_movReadIFrameCnt = sys_num;

	for (i = 0; i < sys_num; i++) {
		MOV_ReadB32Bits(pb, &frame_num);
		if (i == 0) {
			f1 = frame_num;
		} else if (i == 1) {
			f2 = frame_num;
		} else if (i == 2) {
			f3 = frame_num;
		}
		pEntry = gp_movReadEntry + (frame_num - 1);
		pEntry->key_frame = 1;//I-frame
	}
	if ((f3 - f2) != (f2 - f1)) { //IPB, 1st GOP less 2 video frame
		gMovH264IPB = 1;
	}
	return MOVATOMERR_NOERROR;
}

UINT16 MOV_Read_stco(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 frame_num, i, tmpOffset = 0;
	UINT64 diff_offset = 0;
	MOV_Ientry   *pEntry = 0;
	if (pAtom->type != MOV_stco && pAtom->type != MOV_co64) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved
	MOV_ReadB32Bits(pb, &frame_num);//if stco, get frame number


	if (pAtom->isAudio == 0) { //video
		//special case
		if (frame_num != g_MovTagReadTrack[0].frameNumber) { //stco stsz frameNum different

			if (gMovStscFix) {
				UINT16 count = 0, index = 0;
				UINT32 offset = 0;
				count = gMovStscFix;
				while (frame_num > 0) {
					if (pAtom->type == MOV_stco) {
						MOV_ReadB32Bits(pb, &tmpOffset);
						diff_offset = tmpOffset;
					} else {
						MOV_ReadB64Bits(pb, &diff_offset);
					}
					for (i = 0; i < count; i++) {
						pEntry = gp_movReadEntry + index + i;
						pEntry->pos = diff_offset + offset; //1st pos
						//DBG_MSG("[%d] pos0 =0x%lx! ", index+i, diff_offset+offset);
						offset += pEntry->size;//1st size
						//DBG_MSG("thisisze=0x%lx!\r\n", pEntry->size);

					}
					index += gMovStscFix;
					frame_num -= 1;
					offset = 0;
				}
			} else {
				UINT32 old_framenum, index = 0, next, offset;
				MOV_Ientry   *pNextEntry = 0;
				UINT16 count;

				index = 0;
				offset = 0;
				while (frame_num > 0) {
					if (pAtom->type == MOV_stco) {
						MOV_ReadB32Bits(pb, &tmpOffset);
						diff_offset = tmpOffset;
					} else {
						MOV_ReadB64Bits(pb, &diff_offset);
					}
					pEntry = gp_movReadEntry + index;
					old_framenum = pEntry->pos;
					pEntry->pos = diff_offset;
					//DBG_MSG("[%d] pos0 =0x%lx!\r\n", index, diff_offset);
					count = 1;
					offset = pEntry->size;
					while (1) {

						pNextEntry = gp_movReadEntry + index + count;
						next = pNextEntry->pos;
						if (next == old_framenum) {
							pNextEntry->pos = diff_offset + offset;
							//DBG_MSG("[%d] pos1 =0x%lx!\r\n", index+count, diff_offset+offset);
							offset += pNextEntry->size;
							count += 1;
							if ((index + count) > g_MovTagReadTrack[0].frameNumber) {
								break;
							}
						} else {
							break;
						}
					}
					offset = 0;
					index += count;
					frame_num -= 1;
				}
			}
		} else {

			if (frame_num > g_movReadEntryMax) {
				frame_num = g_movReadEntryMax;
			}

			for (i = 0; i < frame_num; i++) {
				pEntry = gp_movReadEntry + i;
				if (pAtom->type == MOV_stco) {
					MOV_ReadB32Bits(pb, &tmpOffset);
					diff_offset = tmpOffset;
				} else {
					MOV_ReadB64Bits(pb, &diff_offset);
				}
				pEntry->pos = diff_offset;
			}
		}
	} else if (g_MovTagReadTrack[1].context->codec_id == MEDIAAUDIO_CODEC_MP4A) {
		//special case
		if (frame_num != g_MovTagReadTrack[1].frameNumber) { //stco stsz frameNum different

			if (gMovStscFix) {
				UINT16 count = 0, index = 0;
				UINT32 offset = 0;
				count = gMovStscFix;
				while (frame_num > 0) {
					if (pAtom->type == MOV_stco) {
						MOV_ReadB32Bits(pb, &tmpOffset);
						diff_offset = tmpOffset;
					} else {
						MOV_ReadB64Bits(pb, &diff_offset);
					}
					for (i = 0; i < count; i++) {
						pEntry = gp_movReadAudioEntry + index + i;
						pEntry->pos = diff_offset + offset; //1st pos
						//DBG_MSG("[%d] pos0 =0x%lx! ", index+i, diff_offset+offset);
						offset += pEntry->size;//1st size
						//DBG_MSG("thisisze=0x%lx!\r\n", pEntry->size);

					}
					index += gMovStscFix;
					frame_num -= 1;
					offset = 0;
				}
			} else {
				UINT32 old_framenum, index = 0, next, offset;
				MOV_Ientry   *pNextEntry = 0;
				UINT16 count;

				index = 0;
				offset = 0;
				while (frame_num > 0) {
					if (pAtom->type == MOV_stco) {
						MOV_ReadB32Bits(pb, &tmpOffset);
						diff_offset = tmpOffset;
					} else {
						MOV_ReadB64Bits(pb, &diff_offset);
					}
					pEntry = gp_movReadAudioEntry + index;
					old_framenum = pEntry->pos;
					pEntry->pos = diff_offset;
					//DBG_MSG("[%d] pos0 =0x%lx!\r\n", index, diff_offset);
					count = 1;
					offset = pEntry->size;
					while (1) {

						pNextEntry = gp_movReadAudioEntry + index + count;
						next = pNextEntry->pos;
						if (next == old_framenum) {
							pNextEntry->pos = diff_offset + offset;
							//DBG_MSG("[%d] pos1 =0x%lx!\r\n", index+count, diff_offset+offset);
							offset += pNextEntry->size;
							count += 1;
							if ((index + count) > g_MovTagReadTrack[1].frameNumber) {
								break;
							}
						} else {
							break;
						}
					}
					offset = 0;
					index += count;
					frame_num -= 1;
				}
			}
		} else {

			if (frame_num > g_movReadAudioEntryMax) {
				frame_num = g_movReadAudioEntryMax;
			}

			for (i = 0; i < frame_num; i++) {
				pEntry = gp_movReadAudioEntry + i;
				if (pAtom->type == MOV_stco) {
					MOV_ReadB32Bits(pb, &tmpOffset);
					diff_offset = tmpOffset;
				} else {
					MOV_ReadB64Bits(pb, &diff_offset);
				}
				pEntry->pos = diff_offset;
			}
		}
	} else {
		if (frame_num > g_movReadAudioEntryMax) {
			frame_num = g_movReadAudioEntryMax;
		}
		g_MovTagReadTrack[1].frameNumber = frame_num;
		for (i = 0; i < frame_num; i++) {
			pEntry = gp_movReadAudioEntry + i;
			if (pAtom->type == MOV_stco) {
				MOV_ReadB32Bits(pb, &tmpOffset);
				diff_offset = tmpOffset;
			} else {
				MOV_ReadB64Bits(pb, &diff_offset);
			}
			pEntry->pos = diff_offset;
		}
	}
	return MOVATOMERR_NOERROR;
}
UINT16 MOV_Read_stts(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 frame_num, i, j, number, index;
	UINT32 diff_duration = 0, total_duration = 0;
	if (pAtom->type != MOV_stts) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved
	MOV_ReadB32Bits(pb, &frame_num);//if stco, get frame number
	if (pAtom->isAudio == 0) { //video
		if (frame_num > g_movReadEntryMax) {
			frame_num = g_movReadEntryMax;
		}

		index = 0;
		for (i = 0; i < frame_num; i++) {
			MOV_ReadB32Bits(pb, &number);
			MOV_ReadB32Bits(pb, &diff_duration);
			for (j = 0; j < number; j++) {
				total_duration += diff_duration;
				index += 1;
			}
		}
	}

	return MOVATOMERR_NOERROR;
}

UINT16 MOV_Read_udta(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 uisize, uitag;

	if (pAtom->type != MOV_udta) {
		return MOVATOMERR_TAGERR;
	}
	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);

	MOV_ReadB32Bits(pb, &uisize);
	MOV_ReadB32Bits(pb, &uitag);

	//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
	if (uitag == ATOM_USER) {
		gMovReadTag_UserDataOffset = MOV_ReadTellOffset(pb);
		gMovReadTag_UserDataSize   = uisize - 8;
	} else {
		gMovReadTag_UserDataOffset = 0;
		gMovReadTag_UserDataSize   = 0;
	}
	//#NT#2012/08/21#Calvin Chang -end

#if 0
	if (uitag == 0xa9666d74) {
		MOV_ReadSkip(pb, (uisize - 8));
		MOV_ReadB32Bits(pb, &uisize);
		MOV_ReadB32Bits(pb, &uitag);

		if (uitag == 0xa9696e66) {

			MOV_ReadSkip(pb, 4);
			/*
			if((uisize -12) > 60)
			{
			    memcpy(&MOVModelName[0],(pb + g_MoveReadOffset),60);
			}
			else
			{
			    memcpy(&MOVModelName[0],(pb + g_MoveReadOffset),(uisize -12));
			}
			*/
		}
	}
#endif

	return MOVATOMERR_NOERROR;
}
MEDIA_FIRST_INFO gMovFirstVInfo;
UINT16 MOV_ReadFirst_stsd(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 rvalue, i;
	UINT16 h264DescLen;
	UINT8 byteV;
	char   *ptr;
	//UINT16 sampleRate, tempW, tempH;
	if (pAtom->type != MOV_stsd) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 8);//version and flag , reserved
	MOV_ReadB32Bits(pb, &rvalue);//if mp4v atom, size of mp4v
	MOV_ReadB32Bits(pb, &rvalue);//if mp4v atom, tag of mp4v

	if (pAtom->isAudio == 0) { //video
		//DBG_MSG("rvalue=0x%lx!\r\n", rvalue);
		if (rvalue == MOV_mp4v) {

			MOV_ReadSkip(pb, 24);//tempQ, spatialQ
			MOV_ReadB16Bits(pb, &gMovWidth);// width
			MOV_ReadB16Bits(pb, &gMovHeight);// width
			gMovFirstVInfo.wid = gMovWidth;
			gMovFirstVInfo.hei = gMovHeight;
			gMovFirstVInfo.type = MOV_mp4v;
		} else if ((rvalue == MOV_mjpa) || (rvalue == MOV_jpeg)) {
			MOV_ReadSkip(pb, 24);
			MOV_ReadB16Bits(pb, &gMovWidth);// width
			MOV_ReadB16Bits(pb, &gMovHeight);// width
			//?? gMovHeight = gMovHeight/2;
			DBG_IND("wid=%d, hei=%d!\r\n", gMovWidth, gMovHeight);
			gMovFirstVInfo.wid = gMovWidth;
			gMovFirstVInfo.hei = gMovHeight;
			gMovFirstVInfo.type = MOV_mjpa;

		} else if (rvalue == MOV_avc1) {
			MOV_ReadSkip(pb, 0x18);
			MOV_ReadB16Bits(pb, &gMovWidth);// width
			MOV_ReadB16Bits(pb, &gMovHeight);// width
			MOV_ReadSkip(pb, 0x40);
			MOV_ReadB16Bits(pb, &h264DescLen);// width

			ptr = gMov264VidDesc;
			*ptr++ = 0x00;
			*ptr++ = 0x00;
			*ptr++ = 0x00;
			*ptr++ = 0x01;
			for (i = 0; i < h264DescLen; i++) {
				MOV_ReadB8Bits(pb, &byteV);
				*ptr++ = byteV;
			}
			*ptr++ = 0x00;
			*ptr++ = 0x00;
			*ptr++ = 0x00;
			*ptr++ = 0x01;
			MOV_ReadSkip(pb, 0x3);
			for (i = 0; i < 4; i++) {
				MOV_ReadB8Bits(pb, &byteV);
				*ptr++ = byteV;
			}
			gMov264VidDescLen = h264DescLen + 12;
		} else {
			MOVRead_SetMoovErrorReason(MOV_MOOVERROR_MDATNOTMP4S);
			return MOVATOMERR_NOTMP4V;
		}
	}
	return MOVATOMERR_NOERROR;
}

UINT16 MOV_ReadFirst_stsz(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 frame_num, diff_size;
	//#NT#2012/04/06#Calvin Chang -begin
	//Pre-decode frames for new H264 driver
	UINT32 pre_decfrm = 0;
	//#NT#2012/04/06#Calvin Chang -end
	//MOV_Ientry   *pEntry = 0;
	if (pAtom->type != MOV_stsz) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved


	if (pAtom->isAudio == 0) { //video
		MOV_ReadSkip(pb, 4);//reserved

		MOV_ReadB32Bits(pb, &frame_num);//if stsz, get frame number
		g_MovTagReadTrack[0].frameNumber = frame_num;
		//DBG_MSG("video frame_num=%ld!\r\n", frame_num);

		MOV_ReadB32Bits(pb, &diff_size);
		gMovFirstVInfo.size = diff_size;

		//#NT#2012/04/06#Calvin Chang -begin
		//Pre-decode frames for new H264 driver
		gMovFirstVInfo.vidfrm_size[pre_decfrm] = gMovFirstVInfo.size; // First Frame
		pre_decfrm++;
		while (pre_decfrm < MEDIAREADLIB_PREDEC_FRMNUM) {
			if (pre_decfrm >= g_MovTagReadTrack[0].frameNumber) {
				gMovFirstVInfo.vidfrm_size[pre_decfrm] = 0;
				break;
			}
			MOV_ReadB32Bits(pb, &diff_size);
			gMovFirstVInfo.vidfrm_size[pre_decfrm] = diff_size;
			pre_decfrm++;
		}
		//#NT#2012/04/06#Calvin Chang -end

		//DBG_MSG("1st size = 0x%lx!!!\r\n", diff_size);
	} else if (g_MovTagReadTrack[1].context->codec_id == MEDIAAUDIO_CODEC_MP4A){
		MOV_ReadSkip(pb, 4);//reserved

		MOV_ReadB32Bits(pb, &frame_num);//if stsz, get frame number
		//DBG_MSG("audio frame_num=%ld!\r\n", frame_num);
		g_MovTagReadTrack[1].frameNumber = frame_num;
	}
	return MOVATOMERR_NOERROR;
}
UINT16 MOV_ReadFirst_stco(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 frame_num, tmpOffset = 0;
	UINT64 diff_offset = 0;
	//#NT#2012/04/06#Calvin Chang -begin
	//Pre-decode frames for new H264 driver
	UINT32 pre_decfrm = 0;
	//#NT#2012/04/06#Calvin Chang -end
	//MOV_Ientry   *pEntry = 0;
	if (pAtom->type != MOV_stco && pAtom->type != MOV_co64) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved
	MOV_ReadB32Bits(pb, &frame_num);//if stco, get frame number


	if (pAtom->isAudio == 0) { //video
		if (frame_num > g_movReadEntryMax) {
			frame_num = g_movReadEntryMax;
		}

		if (pAtom->type == MOV_stco) {
			MOV_ReadB32Bits(pb, &tmpOffset);
			diff_offset = tmpOffset;
		} else {
			MOV_ReadB64Bits(pb, &diff_offset);
		}
		gMovFirstVInfo.pos = diff_offset;

		//#NT#2012/04/06#Calvin Chang -begin
		//Pre-decode frames for new H264 driver
		gMovFirstVInfo.vidfrm_pos[pre_decfrm] = gMovFirstVInfo.pos; // First Frame
		pre_decfrm++;
		while (pre_decfrm < MEDIAREADLIB_PREDEC_FRMNUM) {
			if (pAtom->type == MOV_stco) {
				MOV_ReadB32Bits(pb, &tmpOffset);
				diff_offset = tmpOffset;
			} else {
				MOV_ReadB64Bits(pb, &diff_offset);
			}
			gMovFirstVInfo.vidfrm_pos[pre_decfrm] = diff_offset;
			pre_decfrm++;
		}
		//#NT#2012/04/06#Calvin Chang -end
		//DBG_MSG("1st pos = 0x%lx!!!\r\n", diff_offset);
	} else if (pAtom->isAudio == 1) { //audio//2012/11/21 Meg
		g_MovTagReadTrack[1].frameNumber = frame_num;
	}
	return MOVATOMERR_NOERROR;
}

UINT16 MOV_ReadVidEntry_stsz(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 frame_num, diff_size;
	MOV_Ientry   *pEntry = 0;
	UINT32       i = 0;

	if (pAtom->type != MOV_stsz) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved


	if (pAtom->isAudio == 0) { //video
		MOV_ReadSkip(pb, 4);//reserved

		MOV_ReadB32Bits(pb, &frame_num);//if stsz, get frame number
		g_MovTagReadTrack[0].frameNumber = frame_num;
		//DBG_MSG("g_movReadEntryMax=%ld, frame_num=%ld!\r\n",g_movReadEntryMax, frame_num);
#if 1
		if (frame_num > g_movReadEntryMax) {
			frame_num = g_movReadEntryMax;
			DBG_ERR("framenum not enough! max=%d, frameN=%d!\r\n", g_movReadEntryMax, frame_num);
			DBG_ERR("need more size for MEDIAREADLIB_SETINFO_ENTRYBUF!! \r\n");
		}

		g_MovTagReadTrack[0].frameNumber = frame_num;
		for (i = 0; i < frame_num; i++) {
			pEntry = gp_movReadEntry + i;
			MOV_ReadB32Bits(pb, &diff_size);
			pEntry->size = diff_size;

			if (i == 0) {
				gMovFirstVInfo.size = diff_size;
			}

			if (i < MEDIAREADLIB_PREDEC_FRMNUM) {
				gMovFirstVInfo.vidfrm_size[i] = diff_size;
			}

		}

#else

		if (frame_num > g_movReadEntryMax) {
			frame_num = g_movReadEntryMax;
		}

		MOV_ReadB32Bits(pb, &diff_size);
		gMovFirstVInfo.size = diff_size;

		//#NT#2012/04/06#Calvin Chang -begin
		//Pre-decode frames for new H264 driver
		gMovFirstVInfo.vidfrm_size[pre_decfrm] = gMovFirstVInfo.size; // First Frame
		pre_decfrm++;
		while (pre_decfrm < MEDIAREADLIB_PREDEC_FRMNUM) {
			if (pre_decfrm >= g_MovTagReadTrack[0].frameNumber) {
				gMovFirstVInfo.vidfrm_size[pre_decfrm] = 0;
				break;
			}
			MOV_ReadB32Bits(pb, &diff_size);
			gMovFirstVInfo.vidfrm_size[pre_decfrm] = diff_size;
			pre_decfrm++;
		}
		//#NT#2012/04/06#Calvin Chang -end

		//DBG_MSG("1st size = 0x%lx!!!\r\n", diff_size);
#endif
	}
	return MOVATOMERR_NOERROR;
}
UINT16 MOV_ReadVidEntry_stco(UINT8 *pb, MOV_Atom *pAtom)
{
	UINT32 frame_num, diff_offset;
	MOV_Ientry   *pEntry = 0;
	UINT32       i = 0;
	if (pAtom->type != MOV_stco) {
		return MOVATOMERR_TAGERR;
	}

	MOV_ReadSeekOffset(pb, pAtom->offset, KFS_SEEK_SET);
	MOV_ReadSkip(pb, 4);//version and flag , reserved
	MOV_ReadB32Bits(pb, &frame_num);//if stco, get frame number


	if (pAtom->isAudio == 0) { //video
		if (frame_num > g_movReadEntryMax) {
			frame_num = g_movReadEntryMax;
		}

#if 1

		for (i = 0; i < frame_num; i++) {
			pEntry = gp_movReadEntry + i;
			MOV_ReadB32Bits(pb, &diff_offset);
			pEntry->pos = diff_offset;

			if (i == 0) {
				gMovFirstVInfo.pos = diff_offset;
			}

			if (i < MEDIAREADLIB_PREDEC_FRMNUM) {
				gMovFirstVInfo.vidfrm_pos[i] = diff_offset;
			}

		}

#else
		MOV_ReadB32Bits(pb, &diff_offset);
		gMovFirstVInfo.pos = diff_offset;

		//#NT#2012/04/06#Calvin Chang -begin
		//Pre-decode frames for new H264 driver
		gMovFirstVInfo.vidfrm_pos[pre_decfrm] = gMovFirstVInfo.pos; // First Frame
		pre_decfrm++;
		while (pre_decfrm < MEDIAREADLIB_PREDEC_FRMNUM) {
			MOV_ReadB32Bits(pb, &diff_offset);
			gMovFirstVInfo.vidfrm_pos[pre_decfrm] = diff_offset;
			pre_decfrm++;
		}
		//#NT#2012/04/06#Calvin Chang -end
		//DBG_MSG("1st pos = 0x%lx!!!\r\n", diff_offset);
#endif
	} else if (pAtom->isAudio == 1) { //audio//2012/11/21 Meg
		g_MovTagReadTrack[1].frameNumber = frame_num;
	}
	return MOVATOMERR_NOERROR;
}

#if 0
//#NT#2009/11/13#Meg Lin#[0007720] -begin
//#NT#add for user data
UINT16 MOV_ReadFirst_udta(UINT8 *pb, MOV_Atom *pAtom)
{
	if (pAtom->type != MOV_udta) {
		return MOVATOMERR_TAGERR;
	} else {
		return MOVATOMERR_NOERROR;
	}

}
//#NT#2009/11/13#Meg Lin#[0007720] -end
#endif
//#NT#2010/08/10#Meg Lin# -begin
//ignore IPB frames
UINT16 MOV_ReadFirst_ctts(UINT8 *pb, MOV_Atom *pAtom)
{
	if (pAtom->type != MOV_ctts) {
		return MOVATOMERR_TAGERR;
	}
	//#NT#2012/07/10#Calvin Chang#support IPB frame -begin
	else {
		return MOVATOMERR_NOERROR;
	}
#if 0
	//ignore this file if b-frames exist!!
	DBG_ERR("IPB frames unsupported!!\r\n");
	gpMovReadTrack->trackDuration = 0;
	return MOVATOMERR_NOERROR;
#endif
	//#NT#2012/07/10#Calvin Chang -end

}
//#NT#2010/08/10#Meg Lin# -end

//#NT#2012/11/21#Meg Lin -begin
void MOV_Read_SetVidEntryAddr(UINT32 addr, UINT32 size)
{
	UINT32 max;

	max = size / (sizeof(MOV_Ientry));
	if (addr) {
		gp_movReadEntry = (MOV_Ientry *)addr;
		g_movReadEntryMax = max;
		memset((UINT8 *)gp_movReadEntry, 0, size);
		g_MovTagReadTrack[0].cluster = gp_movReadEntry;
	} else {
		gp_movReadEntry = 0;
	}
#if MOV_PLAYDEBUG
	DBG_IND("g_movReadEntryMax=0x%lx!\r\n", g_movReadEntryMax);
#endif
}

void MOV_Read_SetAudEntryAddr(UINT32 addr, UINT32 size)
{
	UINT32 max;

	max = size / (sizeof(MOV_Ientry));
	if (addr) {
		gp_movReadAudioEntry = (MOV_Ientry *)addr;
		g_movReadAudioEntryMax = max;
		memset((UINT8 *)gp_movReadAudioEntry, 0, size);
		g_MovTagReadTrack[1].cluster = gp_movReadAudioEntry;

	} else {
		gp_movReadAudioEntry = 0;
	}
#if MOV_PLAYDEBUG
	DBG_IND("g_movReadAudioEntryMax=0x%lx!\r\n", g_movReadAudioEntryMax);
#endif
}
//#NT#2012/11/21#Meg Lin -end

//we know this frame offset in file, want this frame size
UINT32 MOV_Read_SearchSize(UINT64 offset, UINT8 *pKey)
{
	UINT16 i, find = 0;
	MOV_Ientry   *pEntry = 0, *pEntryPrev = 0;

	//DBG_MSG("vsearch =%ld \r\n", g_movReadVideoSearchIndex);
	for (i = g_movReadVideoSearchIndex; i < g_movReadEntryMax; i++) {
		pEntry = gp_movReadEntry + i;
		if (pEntry->pos >= offset) {
			if (i == 0) {
				find = 1;
				g_movReadVideoSearchIndex = i;
				break;
			} else {
				pEntryPrev = gp_movReadEntry + i - 1;
				if (pEntryPrev->pos < offset) {
					find = 1;
					g_movReadVideoSearchIndex = i;
					break;
				}

			}
		}

	}
	if (find == 0) {
		//DBG_MSG("searchagain!!\r\n");
		for (i = 0; i < g_movReadEntryMax; i++) {
			pEntry = gp_movReadEntry + i;
			if (pEntry->pos >= offset) {
				//find =1;
				g_movReadVideoSearchIndex = i;
				break;
			}

		}
	}
	if (pEntry->key_frame) {
		*pKey = 1;
	} else {
		*pKey = 0;
	}
	//DBG_MSG("vsearchFF =%ld \r\n", g_movReadVideoSearchIndex);

	return pEntry->size;
}

//we know this frame offset in file, want this frame size
UINT32  g_tempMovAudioGetSize = 0;

//#NT#2007/09/06#Meg Lin -begin
//fixbug, modify searching audio pad
UINT32 MOV_Read_SearchAudioSize(UINT64 offset)
{
	UINT16 i, find = 0;
	MOV_Ientry   *pEntry = 0;

	//firstly, search from last
	//("searchAA =%ld \r\n", g_movReadAudioSearchIndex);
	for (i = g_movReadAudioSearchIndex; i < g_movReadAudioEntryMax; i++) {
		pEntry = gp_movReadAudioEntry + i;
		if (pEntry->pos == offset) {
			if (i == 0) {
				find = 1;
				g_movReadVideoSearchIndex = i;
				break;
			} else {
				//if (pEntryPrev->pos < offset)
				{
					find = 1;
					//DBG_MSG("audio11=0x%lx\r\n", pEntry->pos);
					g_movReadAudioSearchIndex = i;
					break;
				}
			}
		}

	}
	//then, search from begin
	if (find == 0) {
		//DBG_MSG("searchagainAA!!\r\n");

		for (i = 0; i < g_movReadAudioEntryMax; i++) {
			pEntry = gp_movReadAudioEntry + i;
			if (pEntry->pos == offset) {
				//find =1;
				g_movReadAudioSearchIndex = i;
				//DBG_MSG("audio22=0x%lx\r\n", pEntry->pos));
				break;
			}

		}
	}
	//DBG_MSG("vsearchAA =%ld \r\n", g_movReadAudioSearchIndex);
	g_tempMovAudioGetSize += pEntry->size;
	//DBG_MSG("pos=0x%lX,i=0x%lX, 1=0x%lX, size=0x%lX\r\n", pEntry->pos, g_movReadAudioSearchIndex, pEntry->size,g_tempMovAudioGetSize);
	return pEntry->size;
}
//#NT#2007/09/06#Meg Lin -end

//#NT#2007/09/06#Meg Lin -begin
//fixbug: if video > audio, add pure audio frame

UINT32 MOV_Read_SearchAudioFrameSize(UINT64 offset)
{
	UINT16 i, find = 0;
	MOV_Ientry   *pEntry = 0;

	//firstly, search from last
	//DBG_MSG("searchIndexAA =%ld \r\n", g_movReadAudioSearchIndex);
	for (i = g_movReadAudioSearchIndex; i < g_movReadAudioEntryMax; i++) {
		pEntry = gp_movReadAudioEntry + i;
		if (pEntry->pos == offset) {
			if (i == 0) {
				find = 1;
				g_movReadVideoSearchIndex = i;
				break;
			} else {
				find = 1;
				//DBG_MSG("audio11=0x%lx\r\n", pEntry->pos);
				g_movReadAudioSearchIndex = i;
				break;
			}
		}

	}
	//then, search from begin
	if (find == 0) {
		//DBG_MSG("searchagainAA!!\r\n");

		for (i = 0; i < g_movReadAudioEntryMax; i++) {
			pEntry = gp_movReadAudioEntry + i;
			if (pEntry->pos == offset) {
				//find =1;
				g_movReadAudioSearchIndex = i;
				//DBG_MSG("audio22=0x%lx\r\n", pEntry->pos);
				break;
			}

		}
	}
	//DBG_MSG("vsearchAA =%ld \r\n", g_movReadAudioSearchIndex);

	//because MOV_Read_SearchAudioFrameSize called twice, so g_tempMovAudioGetSize is wrong!
	g_tempMovAudioGetSize += pEntry->size;

	//DBG_MSG("pos=0x%lX,i=0x%lX, size=0x%lX, totalsize=0x%lX\r\n", pEntry->pos, g_movReadAudioSearchIndex, pEntry->size,g_tempMovAudioGetSize);
	return pEntry->size;
}
//#NT#2007/09/06#Meg Lin -end

UINT32 MOV_Read_GetParam(UINT32 type, UINT32 *pParam)
{
	UINT32 value = 0;
	MOV_Ientry  *ptr;
	switch (type) {
	case MOVPARAM_MDATSIZE:
		value = g_movReadMdatSize;
		break;
	case MOVPARAM_AUDIOSIZE:
		value = g_MovTagReadTrack[1].audioSize;
		break;
	case MOVPARAM_MVHDDURATION:
		value = gpMovReadTrack->trackDuration;
#if MOV_PLAYDEBUG
		DBG_IND("MVHD duration =%ld!\r\n", value);
#endif
		break;
	case MOVPARAM_VIDTKHDDURATION:
		value = g_MovTagReadTrack[0].trackDuration;
		break;
	case MOVPARAM_AUDTKHDDURATION:
		value = g_MovTagReadTrack[1].trackDuration;
		break;
	case MOVPARAM_SAMPLERATE:
		value = g_MovTagReadTrack[1].context->nSamplePerSecond;
#if MOV_PLAYDEBUG
		DBG_IND("wave sample rate =%ld!\r\n", value);
#endif
		break;
	case MOVPARAM_FIRSTVIDEOOFFSET:
		ptr = gp_movReadEntry + 0;
		value = ptr->pos;
		break;
	case MOVPARAM_FIRSTAUDIOOFFSET:
		ptr = gp_movReadAudioEntry + 0;
		value = ptr->pos;
		break;
	case MOVPARAM_TOTAL_VFRAMES:
		value = g_MovTagReadTrack[0].frameNumber;//0 for video

		break;
	case MOVPARAM_TOTAL_AFRAMES:
		value = g_MovTagReadTrack[1].frameNumber;//1 for audio
		break;
	case MOVPARAM_VIDEOTIMESCALE:
		value = gpMovReadTrack->timescale;
		break;
	case MOVPARAM_WIDTH:
		value = gMovWidth;
		break;
	case MOVPARAM_HEIGHT:
		value = gMovHeight;
		break;
	case MOVPARAM_AUDIO_BITSPERSAMPLE:
		value = 0x10;
		break;
	case MOVPARAM_AUDIOCODECID:
		value = g_MovTagReadTrack[1].context->codec_id;
		break;
	case MOVPARAM_VIDEOTYPE:
		value = g_MovTagReadTrack[0].type;
		break;

	case MOVPARAM_VIDEOFR:
		if ((g_movReadVideoFR == 0) && (g_MovTagReadTrack[0].trackDuration != 0)) { //reset value//2010/08/10 Meg Lin
			// since timescale = 30000, divide 25 to avoid data overflow in huge frame number
			value = (gpMovReadTrack->timescale / 25) * g_MovTagReadTrack[0].frameNumber;
			value /= g_MovTagReadTrack[0].trackDuration / 25;
			//value = (gpMovReadTrack->timescale)*g_MovTagReadTrack[0].frameNumber/g_MovTagReadTrack[0].trackDuration;
			g_movReadVideoFR = value;
		} else {
			value = g_movReadVideoFR;
		}
		break;
	case MOVPARAM_AUDFRAMENUM:
		value = g_MovTagReadTrack[1].frameNumber;
		break;
	case MOVPARAM_AUDCHANNELS:
		value = g_MovTagReadTrack[1].context->nchannel;
#if MOV_PLAYDEBUG
		DBG_IND("audio channels =%d!\r\n", value);
#endif
		break;
	case MOVPARAM_AUDIOTYPE:
		value = g_MovTagReadTrack[1].context->codec_id;
		break;

	//#NT#2012/11/21#Meg Lin -begin
	case MOVPARAM_AUDIOFR:
		if ((g_movReadAudioFR == 0) && (g_MovTagReadTrack[1].trackDuration != 0)) { //reset value//2010/08/10 Meg Lin
			value = (UINT32)((UINT64)(gpMovReadTrack->timescale) * g_MovTagReadTrack[1].frameNumber / g_MovTagReadTrack[1].trackDuration);
			g_movReadAudioFR = value;
		} else {
			value = g_movReadAudioFR;
		}
		break;
	//#NT#2012/11/21#Meg Lin -end
	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	case MOVPARAM_VIDEOROTATE:
		value = g_MovTagReadTrack[0].uiRotateInfo;
		break;
	//#NT#2013/04/17#Calvin Chang -end

	//#NT#2013/06/11#Calvin Chang#Support Create/Modified Data inMov/Mp4 File format -begin
	case MOVPARAM_CTIME:
		value = g_MovTagReadTrack[0].ctime;
		break;
	case MOVPARAM_MTIME:
		value = g_MovTagReadTrack[0].mtime;
		break;
	//#NT#2013/06/11#Calvin Chang -end

	default:
		value = 0;
		break;
	}
	*pParam = value;
	return 0;
}

UINT32 MOV_Read_SearchAtom(UINT32 type, UINT32 *pNum)
{
	UINT32 tagindex;
	UINT8  i;

	for (i = 0; i <= gAtomNumber; i++) {

		if(i >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", i);
		}

		tagindex = g_ReadAtom[i].tag;
		if (tagindex == type) {
			*pNum = i;
			return 1;
		}


	}
	return 0;
}

void MOV_Read_SetMdatSize(UINT32 size)
{
	g_movReadMdatSize = size;

}

void MOV_Read_SearchMax_I_FrameSize(void)//search video track max iframe size
{
	UINT32 i, frame_num, size = 0;
	MOV_Ientry   *pEntry = 0;


	frame_num = g_MovTagReadTrack[0].frameNumber;
	pEntry = gp_movReadEntry;
	//the first frame must be i-frame
	if ((frame_num) && (pEntry->key_frame == 1)) {
		for (i = 0; i < frame_num; i++) {
			pEntry = gp_movReadEntry + i;
			if (pEntry->key_frame) {
				if (pEntry->size > size) {
					size = pEntry->size;
				}
			}
		}

	}
	g_movReadMaxIFrameSize = size;
#if MOV_PLAYDEBUG
	DBG_IND("max framenum=0x%lX, size=0x%lX!\r\n", frame_num, size);
#endif
}
#if 0
#pragma mark -
#endif

void MOVRead_GetMaxIFrameSize(UINT32 *psize)
{
	*psize = g_movReadMaxIFrameSize;
#if MOV_PLAYDEBUG
	DBG_IND("maxsize=0x%lX!\r\n", g_movReadMaxIFrameSize);
#endif
}


UINT32 MOVRead_GetIFrameTotalPacketCnt(void)
{

	return g_movReadIFrameCnt;
}

UINT64 MOVRead_GetSecond2FilePos(UINT32 seconds)
{
	UINT32 i, frame_num, index = 0;
	MOV_Ientry   *pEntry = 0;

	frame_num = g_MovTagReadTrack[0].frameNumber;
	pEntry = gp_movReadEntry;
	//the first frame must be i-frame
	if ((frame_num) && (pEntry->key_frame == 1)) {
		index = 0;
		for (i = 0; i < frame_num; i++) {
			pEntry = gp_movReadEntry + i;
			if (pEntry->key_frame) {
				if (index == seconds) {
#if MOV_PLAYDEBUG
					DBG_IND("get second = %d, framesize=0x%llX, filepos=0x%lX!\r\n", seconds, pEntry->size, pEntry->pos);
#endif
					return pEntry->pos;
				}
				index += 1; //key_frame count, start from 0 (or 1 ??)
			}
		}

	}
#if MOV_PLAYDEBUG
	DBG_IND("get second = %d, filepos=0x%lX!\r\n", seconds, 0);
#endif
	return 0;
}

void MOVRead_ShowAllSize(void)
{

	UINT32  audSize = 0, i, lastsize = 0;
	UINT64  lastpos = 0;
	MOV_Ientry   *pEntry = 0;
	MOV_Ientry   *pEntry2 = 0;
	for (i = 0; i < g_movReadEntryMax; i++) {
		pEntry = gp_movReadEntry + i;
		pEntry2 = gp_movReadEntry + i - 1;
		lastpos = pEntry2->pos;
		lastsize = pEntry2->size;
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("V[%d] pos=0x%llX,size=0x%lX\r\n", i, pEntry->pos, pEntry->size);
		}

		//DBG_MSG("i=%lX,1=0x%lX, size=0x%lX\r\n", i, pEntry->size, audSize);
		if (pEntry->size == 0) {
			break;
		}
		if (i % 30 != 0) {
			if (lastpos + lastsize != pEntry->pos) {
				DBG_ERR("ERR!!! 1=0x%llx, 2=0x%lx, 3=0x%llx!\r\n", lastpos, lastsize, pEntry->pos);
			}
		}

	}
	for (i = 0; i < g_movReadAudioEntryMax; i++) {
		pEntry = gp_movReadAudioEntry + i;

		audSize += pEntry->size;
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("A[%d] pos=0x%llX,size=0x%lX\r\n", i, pEntry->pos, pEntry->size);
		}

		//DBG_MSG("i=%lX,1=0x%lX, size=0x%lX\r\n", i, pEntry->size, audSize);
		if (pEntry->size == 0) {
			break;
		}



	}
}
void MOVRead_GetVideoPosAndOffset(UINT32 findex, UINT64 *pos, UINT32 *size)
{
	MOV_Ientry   *pEntry = 0;
	pEntry = gp_movReadEntry + findex;
	*pos = pEntry->pos;
	*size = pEntry->size;
	//DBG_MSG("index=%d, pos=%lx, size=%lx!\r\n", findex, *pos, *size);
}
void MOVRead_GetAudioPosAndOffset(UINT32 findex, UINT64 *pos, UINT32 *size)
{
	MOV_Ientry   *pEntry = 0;
	pEntry = gp_movReadAudioEntry + findex;
	*pos = pEntry->pos;
	*size = pEntry->size;
	//DBG_MSG("AAindex=%d, pos=%lx, size=%lx!\r\n", findex, *pos, *size);
}
void MOVRead_GetVideoEntry(UINT32 findex, MOV_Ientry *ptr)
{
	MOV_Ientry   *pEntry = 0;
	if (findex >= g_MovTagReadTrack[0].frameNumber) {
		ptr->pos = 0;
		ptr->size = 0;
		return;
	}
	pEntry = gp_movReadEntry + findex;
	memcpy(ptr, pEntry, sizeof(MOV_Ientry));
}
#if 0
UINT32 MOVRead_SearchNextIFrame(UINT32 now)
{
	UINT32 next = 0;
	MOV_Ientry   *pEntry = 0;

	if (g_MovTagReadTrack[0].type == MEDIAPLAY_VIDEO_MJPG) {
		return 0;
	}

	//if now = i frame, search next
	now += 1;

	while (now < g_MovTagReadTrack[0].frameNumber) {
		pEntry = gp_movReadEntry + now;
		if (pEntry->key_frame == 1) {
			next = now;
			break;
		}
		now++;
	}
#if MOV_PLAYDEBUG
	if (gMovReaderContainer.cbShowErrMsg) {
		(gMovReaderContainer.cbShowErrMsg)("next iframe=%d!\r\n", next);
	}
#endif

	if (next == 0) { //next to end
		next = g_MovTagReadTrack[0].frameNumber;
	}
	return next;
}
#else // Hideo test for skip some I frame
UINT32 MOVRead_SearchNextIFrame(UINT32 now, UINT32 uiSkipNum)
{
	UINT32 next = 0, uiINum = 0;
	MOV_Ientry   *pEntry = 0;

	if (g_MovTagReadTrack[0].type == MEDIAPLAY_VIDEO_MJPG) {
		return 0;
	}

	//if (uiSkipNum > 3) // max skip 3 I
	//    uiSkipNum = 0; // no skip

	//if now = i frame, search next
	now += 1;

	while (now < g_MovTagReadTrack[0].frameNumber) {
		pEntry = gp_movReadEntry + now;
		if (pEntry->key_frame == 1) {
			if (uiINum == uiSkipNum) {
				next = now;
				break;
			}
			uiINum++;
		}
		now++;
	}
#if MOV_PLAYDEBUG
	if (gMovReaderContainer.cbShowErrMsg) {
		(gMovReaderContainer.cbShowErrMsg)("next iframe=%d!\r\n", next);
	}
#endif

	if (next == 0) { //next to end
		next = g_MovTagReadTrack[0].frameNumber;
	}
	return next;
}
#endif
UINT32 MOVRead_SearchNextPFrame(UINT32 now)
{
	UINT32 next = 0;
	//MOV_Ientry   *pEntry = 0;

	if (gMovH264IPB == 0) {
		//Delay_DelayMs(100); do not work
		//return MOVRead_SearchNextIFrame(now);
		if (gMovReaderContainer.cbShowErrMsg) {
			(gMovReaderContainer.cbShowErrMsg)("This MOV is not a IPB file..\r\n");
		}
		return 0;
	}
	//if now = i frame, search next
	if (now % 3 == 1) {
		next = now + 3;
	} else {
		next = (now - now % 3) + 4;
	}
#if MOV_PLAYDEBUG
	if (gMovReaderContainer.cbShowErrMsg) {
		(gMovReaderContainer.cbShowErrMsg)("next pframe=%d!\r\n", next);
	}
#endif
	return next;
}
#if 0
UINT32 MOVRead_SearchPrevIFrame(UINT32 now)
{
	UINT32 prev = 0;
	MOV_Ientry   *pEntry = 0;

	if (g_MovTagReadTrack[0].type == MEDIAPLAY_VIDEO_MJPG) {
		return 0;
	}


	//if now = i frame, search next
	if (now > 1) {
		now -= 1;
	}

	while (now > 0) {
		pEntry = gp_movReadEntry + now;
		if (pEntry->key_frame == 1) {
			prev = now;
			break;
		}
		now--;
	}
#if MOV_PLAYDEBUG
	if (gMovReaderContainer.cbShowErrMsg) {
		(gMovReaderContainer.cbShowErrMsg)("prev iframe=%d!\r\n", prev);
	}
#endif
	return prev;
}
#else // Hideo test for skip some I frame
UINT32 MOVRead_SearchPrevIFrame(UINT32 now, UINT32 uiSkipNum)
{
	UINT32 prev = 0, uiINum = 0;
	MOV_Ientry   *pEntry = 0;

	if (g_MovTagReadTrack[0].type == MEDIAPLAY_VIDEO_MJPG) {
		return 0;
	}

	//if (uiSkipNum > 3) // max skip 3 I
	//    uiSkipNum = 0; // no skip

	//if now = i frame, search next
	if (now > 1) {
		now -= 1;
	}

	while (now > 0) {
		pEntry = gp_movReadEntry + now;
		if (pEntry->key_frame == 1) {
			if (uiINum == uiSkipNum) {
				prev = now;
				break;
			}
			uiINum++;
		}
		now--;
	}
#if MOV_PLAYDEBUG
	if (gMovReaderContainer.cbShowErrMsg) {
		(gMovReaderContainer.cbShowErrMsg)("prev iframe=%d!\r\n", prev);
	}
#endif
	return prev;
}
#endif
UINT32 MOVRead_GetH264IPB(void)
{
	return (UINT32)gMovH264IPB;
}
UINT32 MOVRead_GetVidEntryAddr(void)
{
	return (UINT32)gp_movReadEntry;
}
UINT32 MOVRead_GetAudEntryAddr(void)
{
	return (UINT32)gp_movReadAudioEntry;
}
void MOVRead_GetAudioEntry(UINT32 findex, MOV_Ientry *ptr)
{
	MOV_Ientry   *pEntry = 0;
	if (findex >= g_MovTagReadTrack[1].frameNumber) {
		ptr->pos = 0;
		ptr->size = 0;
		return;
	}
	pEntry = gp_movReadAudioEntry + findex;
	memcpy(ptr, pEntry, sizeof(MOV_Ientry));
}

UINT32 MOVRead_GetTotalDuration(void)
{
	return gpMovReadTrack->trackDuration / gpMovReadTrack->timescale;
}
//#NT#2009/11/13#Meg Lin#[0007720] -begin
//#NT#add for user data
UINT8 MOVRead_GetUdtaPosSize(UINT64 *ppos, UINT32 *psize)
{
	UINT32 i;
	for (i = 0; i < MOV_ATOM_MAX; i++) {

		if(i >= MOV_ATOM_MAX){
			DBG_ERR("atom out of index(%lu)\r\n", i);
		}

		if (g_ReadAtom[i].type == MOV_udta) {
			//#NT#2012/08/21#Calvin Chang#Add for User Data & Thumbnail -begin
			if (gMovReadTag_UserDataSize) {
				// 'Udta' atom has dafault user atom inside, return dafault user atome data offset
				*ppos  = gMovReadTag_UserDataOffset;
				*psize = gMovReadTag_UserDataSize;
			} else {
				// Return the offset of data of 'udta' atom and project has to parse the custom atom information
				*ppos = g_ReadAtom[i].offset;
				*psize = g_ReadAtom[i].size - 8;
			}
			//#NT#2012/08/21#Calvin Chang -end

#if MOV_PLAYDEBUG
			DBG_IND("user data pos=0x%llx! size=0x%lx!\r\n", *ppos, *psize);
#endif
			return 0;
		}

	}
	*ppos = 0;
	*psize = 0;
	return 0xFF;

}
//#NT#2009/11/13#Meg Lin#[0007720] -end

UINT32 MovReadLib_Swap(UINT32 value)
{
	UINT32 temp = 0;
	temp = (value & 0xff) << 24;
	temp |= (value & 0xFF00) << 8;
	temp |= (value & 0xFF0000) >> 8;
	temp |= (value & 0xFF000000) >> 24;
	return temp;

}

