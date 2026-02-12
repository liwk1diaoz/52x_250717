/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       MOVWriteTag.c
    @ingroup    mIAVMOVREC

    @brief      MOV tag writer
    @version    V1.00.000
    @date       2019/05/05
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

*/

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
#include <string.h>

#include "kernel.h"
#include "FileSysTsk.h"
#include "MediaWriteLib.h"
#include "AVFile_MakerMov.h"
#include "MOVLib.h"
#include "movRec.h"
#include "MovWriteLib.h"
#include "Audio.h"
#include "NvtVerInfo.h"
#include "HwMem.h"
#include "FileSysTsk.h"

#define __MODULE__          MOVWTag
#define __DBGLVL__          1 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
#else
#include <string.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include <kwrap/verinfo.h>

#define __MODULE__          MOVWTag
#define __DBGLVL__          2
#include "kwrap/debug.h"
unsigned int MOVWTag_debug_level = NVT_DBG_WRN;

#include <time.h>
#include "FileSysTsk.h"
#include "avfile/MediaWriteLib.h"
#include "avfile/AVFile_MakerMov.h"
#include "avfile/MOVLib.h"

// INCLUDE PROTECTED
#include "movRec.h"
#include "MovWriteLib.h"

typedef enum {
	AUDIO_SR_8000   = 8000,     ///< 8 KHz
	AUDIO_SR_11025  = 11025,    ///< 11.025 KHz
	AUDIO_SR_12000  = 12000,    ///< 12 KHz
	AUDIO_SR_16000  = 16000,    ///< 16 KHz
	AUDIO_SR_22050  = 22050,    ///< 22.05 KHz
	AUDIO_SR_24000  = 24000,    ///< 24 KHz
	AUDIO_SR_32000  = 32000,    ///< 32 KHz
	AUDIO_SR_44100  = 44100,    ///< 44.1 KHz
	AUDIO_SR_48000  = 48000,    ///< 48 KHz
	ENUM_DUMMY4WORD(AUDIO_SR)
} AUDIO_SR;
#endif

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions & Types Declarations                             */
/*-----------------------------------------------------------------------------*/
#define MOV_GPSENTRY_VERSION 0x101 //1.01

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
UINT32  gTempPacketBufBase = 0, gTempPacketBufBase2 = 0, gTempPacketBufBase3 = 0;
//CONTAINERMAKER gMovWriteContainer;//2012/07/13 Meg
MOVWRITELIB_INFO gMovWriteLib_Info[MOVWRITER_COUNT];

UINT32  gMediaVersionAddr = 0;
MOV_Offset  g_MoveOffset = 0, g_MoveOffset2 = 0, g_MoveOffset3 = 0;
char        g_tempChar_d[0x10];
char        g_tempChar_h[0x10];
char        g_tempChar_t[0x10];
char        g_264VidDesc[MOVWRITER_COUNT][0x60]; // enlarge H264 description size to support multi-PPS header
char        g_265VidDesc[MOVWRITER_COUNT][MEDIAWRITE_H265_STSD_MAXLEN]; // enlarge H265 description size to support multi-PPS header
UINT32      g_movh265len[MOVWRITER_COUNT];
//add for sub-stream -begin
char        g_sub_264VidDesc[MOVWRITER_COUNT][0x60];
char        g_sub_265VidDesc[MOVWRITER_COUNT][MEDIAWRITE_H265_STSD_MAXLEN];
UINT32      g_sub_movh265len[MOVWRITER_COUNT];
//add for sub-stream -end
//#NT#2009/11/24#Meg Lin -begin
#if 0
UINT8       g_movFtyp[MOV_FTYP_SIZE] = {0x00, 0x00, 0x00, 0x18, 'f', 't', 'y', 'p', 'q', 't', ' ', ' ',
										0x00, 0x00, 0x00, 0x00, 'q', 't', ' ', ' ', 0x00, 0x00, 0x00, 0x00
									   };
#endif
UINT8       g_movFtyp[MOV_FTYP_SIZE] = {0x00, 0x00, 0x00, 0x1C, 'f', 't', 'y', 'p', 'q', 't', ' ', ' ',
										0x00, 0x00, 0x00, 0x00, 'q', 't', ' ', ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
									   };
//#NT#2009/11/24#Meg Lin -end
//#NT#2012/06/26#Hideo Lin -begin
//#NT#To support MP4 file type
UINT8       g_mp4Ftyp[MP4_FTYP_SIZE] = {0x00, 0x00, 0x00, 0x1C, 'f', 't', 'y', 'p', 'm', 'p', '4', '2',
										0x00, 0x00, 0x00, 0x00, 'i', 's', 'o', 'm', 'a', 'v', 'c', '1', 'm', 'p', '4', '2'
									   };
//#NT#2012/06/26#Hideo Lin -end

extern UINT8  gMovSeamlessCut;
extern UINT32 gMovRecvy_Back1sthead;
extern UINT32 gMovRecvy_newMoovAddr;
extern UINT32 gMovRecvy_newMoovSize;
extern UINT32 g_movGPSEntryAddr[MOVWRITER_COUNT];
extern UINT32 g_movGPSEntrySize[MOVWRITER_COUNT];
extern UINT32 g_movGPSEntryTotal[MOVWRITER_COUNT];
extern UINT64 gMovRcvy_NewTruncatesize;
UINT32 gMovWriteMoovTempAdr = 0;
UINT32 g_mov_moovlayer1adr = 0, g_mov_moovlayer1size = 0;


#if 1	//add for meta -begin
extern UINT32 g_mov_meta_num[MOVWRITER_COUNT];
extern UINT32 g_mov_meta_entry_sign[MOVWRITER_COUNT][MOV_META_COUNT];
extern UINT32 g_mov_meta_entry_max[MOVWRITER_COUNT][MOV_META_COUNT];
extern UINT32 g_mov_meta_entry_addr[MOVWRITER_COUNT][MOV_META_COUNT];
extern UINT32 g_mov_meta_entry_size[MOVWRITER_COUNT][MOV_META_COUNT];
extern UINT32 g_mov_meta_entry_total[MOVWRITER_COUNT][MOV_META_COUNT];
#endif	//add for meta -end

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
UINT32 Mov_Swap(UINT32 value);

void MOV_MakeAtomInit(void *pMovBaseAddr)
{

	gTempPacketBufBase = (UINT32)pMovBaseAddr;

	g_MoveOffset = 0;
}

static void MOV_MakeAtomInit2(void *pMovBaseAddr)
{

	gTempPacketBufBase2 = (UINT32)pMovBaseAddr;

	g_MoveOffset2 = 0;
}

static void MOV_MakeAtomInit3(void *pMovBaseAddr)
{
	gTempPacketBufBase3 = (UINT32)pMovBaseAddr;

	g_MoveOffset3 = 0;
}

void MOV_SeekOffset(char *pb, MOV_Offset pos, UINT16 type)
{
	if (type == KFS_SEEK_SET) {
		g_MoveOffset = pos;
	}
}

static void MOV_SeekOffset3(char *pb, MOV_Offset pos, UINT16 type)
{
	if (type == KFS_SEEK_SET) {
		g_MoveOffset3 = pos;
	}
}

static MOV_Offset MOV_TellOffset(char *pb)
{
	return g_MoveOffset;
}

static MOV_Offset MOV_TellOffset3(char *pb)
{
	return g_MoveOffset3;
}

static void MOV_WriteBuffer(char *pb, char *pSrc, UINT32 len)
{
	UINT32 i;
	pb = (char *)gTempPacketBufBase;

	if (pSrc == 0) {
		DBG_DUMP("WriteBufferERR:pSrc = 0x%lx!\r\n", (unsigned long)pSrc);
		return;
	}
	if (len > 0x10000000) {
		DBG_DUMP("WriteBufferERR:len = 0x%lx!\r\n", len);
		return;
	}
	for (i = 0; i < len; i++) {
		*(pb + g_MoveOffset + i) = *(pSrc + i);
	}
	g_MoveOffset += len;
}

static void MOV_WriteBuffer2(char *pb, char *pSrc, UINT32 len)
{
#if 0
	UINT32 i;
	pb = (char *)gTempPacketBufBase2;

	if (pSrc == 0) {
		DBG_DUMP("WriteBuffer2ERR:pSrc = 0x%lx!\r\n", pSrc);
		return;
	}
	if (len > 0x10000000) {
		DBG_DUMP("WriteBuffer2ERR:len = 0x%lx!\r\n", len);
		return;
	}
	for (i = 0; i < len; i++) {
		*(pb + g_MoveOffset2 + i) = *(pSrc + i);
	}
	g_MoveOffset2 += len;
#endif

	//#NT#2016/03/24#KCHong -begin
	//#NT#Use graph2 to copy data
	pb = (char *)gTempPacketBufBase2;
#if defined (__UITRON) || defined (__ECOS)
	hwmem2_open();
	hwmem2_memcpy((UINT32)(pb + g_MoveOffset2), (UINT32)pSrc, len);
	hwmem2_close();
#else
	memcpy((void *)(pb + g_MoveOffset2), (const void *)pSrc, len);
#endif
	g_MoveOffset2 += len;
	//#NT#2016/03/24#KCHong -end
}

static void MOV_WriteBuffer3(char *pb, char *pSrc, UINT32 len)
{
	UINT32 i;
	pb = (char *)gTempPacketBufBase3;

	if (pSrc == 0) {
		DBG_DUMP("WriteBufferERR3:pSrc = 0x%lx!\r\n", (unsigned long)pSrc);
		return;
	}
	if (len > 0x10000000) {
		DBG_DUMP("WriteBufferERR3:len = 0x%lx!\r\n", len);
		return;
	}
	for (i = 0; i < len; i++) {
		*(pb + g_MoveOffset3 + i) = *(pSrc + i);
	}
	g_MoveOffset3 += len;
}

static void MOV_WriteB8Bits(char *pb, UINT8 value)//big endian
{
	pb = (char *)gTempPacketBufBase;
	*(pb + g_MoveOffset) = value;
	g_MoveOffset += 1;
}

static void MOV_WriteB16Bits(char *pb, UINT16 value)//big endian
{
	pb = (char *)gTempPacketBufBase;
	*(pb + g_MoveOffset) = (value >> 8);
	*(pb + g_MoveOffset + 1) = (value >> 0);
	g_MoveOffset += 2;
}
static void MOV_WriteL16Bits(char *pb, UINT16 value)//little endian
{
	pb = (char *)gTempPacketBufBase;
	*(pb + g_MoveOffset) = (value & 0xff);
	*(pb + g_MoveOffset + 1) = (value >> 8) & 0xFF;
	g_MoveOffset += 2;
}
void MOV_WriteB32Bits(char *pb, UINT32 value)//big endian
{
	pb = (char *)gTempPacketBufBase;
	*(pb + g_MoveOffset) = (value >> 24);
	*(pb + g_MoveOffset + 1) = (value >> 16);
	*(pb + g_MoveOffset + 2) = (value >> 8);
	*(pb + g_MoveOffset + 3) = (value);
	g_MoveOffset += 4;
}
static void MOV_WriteB32Bits2(char *pb, UINT32 value)//big endian
{
	pb = (char *)gTempPacketBufBase2;
	*(pb + g_MoveOffset2) = (value >> 24);
	*(pb + g_MoveOffset2 + 1) = (value >> 16);
	*(pb + g_MoveOffset2 + 2) = (value >> 8);
	*(pb + g_MoveOffset2 + 3) = (value);
	g_MoveOffset2 += 4;
}
static void MOV_WriteB32Bits3(char *pb, UINT32 value)//big endian
{
	pb = (char *)gTempPacketBufBase3;
	*(pb + g_MoveOffset3) = (value >> 24);
	*(pb + g_MoveOffset3 + 1) = (value >> 16);
	*(pb + g_MoveOffset3 + 2) = (value >> 8);
	*(pb + g_MoveOffset3 + 3) = (value);
	g_MoveOffset3 += 4;
}
static void MOV_WriteL32Bits(char *pb, UINT32 value)//little endian
{
	pb = (char *)gTempPacketBufBase;
	*(pb + g_MoveOffset) = (value & 0xff);
	*(pb + g_MoveOffset + 1) = (value >> 8) & 0xFF;
	*(pb + g_MoveOffset + 2) = (value >> 16) & 0xFF;
	*(pb + g_MoveOffset + 3) = (value >> 24) & 0xFF;
	g_MoveOffset += 4;
}
static void MOV_WriteL32Bits2(char *pb, UINT32 value)//little endian
{
	pb = (char *)gTempPacketBufBase2;
	*(pb + g_MoveOffset2) = (value & 0xff);
	*(pb + g_MoveOffset2 + 1) = (value >> 8) & 0xFF;
	*(pb + g_MoveOffset2 + 2) = (value >> 16) & 0xFF;
	*(pb + g_MoveOffset2 + 3) = (value >> 24) & 0xFF;
	g_MoveOffset2 += 4;
}
static void MOV_WriteL32Bits3(char *pb, UINT32 value)//little endian
{
	pb = (char *)gTempPacketBufBase3;
	*(pb + g_MoveOffset3) = (value & 0xff);
	*(pb + g_MoveOffset3 + 1) = (value >> 8) & 0xFF;
	*(pb + g_MoveOffset3 + 2) = (value >> 16) & 0xFF;
	*(pb + g_MoveOffset3 + 3) = (value >> 24) & 0xFF;
	g_MoveOffset3 += 4;
}
void MOV_WriteB64Bits(char *pb, UINT64 value)//big endian
{
	pb = (char *)gTempPacketBufBase;

	*(pb + g_MoveOffset) = (value >> 56);
	*(pb + g_MoveOffset + 1) = (value >> 48);
	*(pb + g_MoveOffset + 2) = (value >> 40);
	*(pb + g_MoveOffset + 3) = (value >> 32);
	*(pb + g_MoveOffset + 4) = (value >> 24);
	*(pb + g_MoveOffset + 5) = (value >> 16);
	*(pb + g_MoveOffset + 6) = (value >> 8);
	*(pb + g_MoveOffset + 7) = (value);
	g_MoveOffset += 8;
}

static void MOV_WriteL64Bits(char *pb, UINT64 value)//big endian
{
	pb = (char *)gTempPacketBufBase;

	*(pb + g_MoveOffset) = (value & 0xFF);
	*(pb + g_MoveOffset + 1) = (value >> 8);
	*(pb + g_MoveOffset + 2) = (value >> 16);
	*(pb + g_MoveOffset + 3) = (value >> 24);
	*(pb + g_MoveOffset + 4) = (value >> 32);
	*(pb + g_MoveOffset + 5) = (value >> 40);
	*(pb + g_MoveOffset + 6) = (value >> 48);
	*(pb + g_MoveOffset + 7) = (value >> 56);
	g_MoveOffset += 8;
}
void MOV_WriteB32Tag(char *pb, char *string)//big endian
{
	pb = (char *)gTempPacketBufBase;

	*(pb + g_MoveOffset) = *string++;
	*(pb + g_MoveOffset + 1) = *string++;
	*(pb + g_MoveOffset + 2) = *string++;
	*(pb + g_MoveOffset + 3) = *string++;
	g_MoveOffset += 4;
}
static void MOV_WriteB32Tag2(char *pb, char *string)//big endian
{
	pb = (char *)gTempPacketBufBase2;

	*(pb + g_MoveOffset2) = *string++;
	*(pb + g_MoveOffset2 + 1) = *string++;
	*(pb + g_MoveOffset2 + 2) = *string++;
	*(pb + g_MoveOffset2 + 3) = *string++;
	g_MoveOffset2 += 4;
}
static void MOV_WriteB32Tag3(char *pb, char *string)//big endian
{
	pb = (char *)gTempPacketBufBase3;

	*(pb + g_MoveOffset3) = *string++;
	*(pb + g_MoveOffset3 + 1) = *string++;
	*(pb + g_MoveOffset3 + 2) = *string++;
	*(pb + g_MoveOffset3 + 3) = *string++;
	g_MoveOffset3 += 4;
}

static void MOV_WriteBytes(char *pb, char *pData, UINT32 size)
{
	pb = (char *)(gTempPacketBufBase + g_MoveOffset);

	memcpy((void *)pb, (void *)pData, size);
	g_MoveOffset += size;
}
/*
static UINT32 MOV_TellTotalAddr(char *pb)
{
    return (gTempPacketBufBase+g_MoveOffset);
}
*/
#if 0
#pragma mark -
#endif


/*
  Compact sparse data into continuous data

  This function write a 32-bit data at pos

  @param pb  buffer pointer
  @param pos insert pos
  @return continuous data total length in bytes
*/
static MOV_Offset MOVWriteUpdateData(char *pb, MOV_Offset pos)
{
	MOV_Offset curpos;

	curpos = MOV_TellOffset(pb);
	MOV_SeekOffset(pb, pos, KFS_SEEK_SET);
	MOV_WriteB32Bits(pb, curpos - pos);
	MOV_SeekOffset(pb, curpos, KFS_SEEK_SET);
	return curpos - pos;
}
static MOV_Offset MOVWriteUpdateData3(char *pb, MOV_Offset pos)
{
	MOV_Offset curpos;

	curpos = MOV_TellOffset3(pb);
	MOV_SeekOffset3(pb, pos, KFS_SEEK_SET);
	MOV_WriteB32Bits3(pb, curpos - pos);
	MOV_SeekOffset3(pb, curpos, KFS_SEEK_SET);
	return curpos - pos;
}

UINT32 MOVWriteFREATag(char *pb, MEDIAREC_HDR_OBJ *pHdr, UINT32 outAddr)//2013/05/28 Meg
{

	UINT32 thumbsize4, screensize4, len;

	MOV_Offset  pos;

	MOV_MakeAtomInit3((void *)outAddr);
	pos = MOV_TellOffset3(pb);
	//DBG_DUMP("FREA addr=0x%lx!\r\n", outAddr);
	MOV_WriteB32Bits3(pb, 0);//size

	if (g_mov_frea_box[pHdr->uiMakerID]) {
		DBG_IND("[%d] use frea atom\r\n", pHdr->uiMakerID);
		MOV_WriteB32Tag3(pb, "frea");
		//tima
		MOV_WriteB32Bits3(pb, 24);//size
		MOV_WriteB32Tag3(pb, "tima");
		MOV_WriteL32Bits3(pb, g_mov_nidx_timeid[pHdr->uiMakerID]);//updateHDR will fill
		MOV_WriteB32Bits3(pb, pHdr->uiWidth);//wid
		MOV_WriteB32Bits3(pb, pHdr->uiHeight);//hei
		MOV_WriteB32Bits3(pb, pHdr->uiVidFrameRate);//fps//2017/03/10
	} else {
		DBG_IND("[%d] use skip atom\r\n", pHdr->uiMakerID);
		MOV_WriteB32Tag3(pb, "skip");//modified for TV playing, old: frea 2017/08/03
		//tima
		MOV_WriteB32Bits3(pb, 12);//size
		MOV_WriteB32Tag3(pb, "tima");
		MOV_WriteL32Bits3(pb, g_mov_nidx_timeid[pHdr->uiMakerID]);//updateHDR will fill
	}

	//version of media
	if (gMediaVersionAddr) { //2015/11/20
		MOV_WriteB32Bits3(pb, VERSION_LENGTH + 8); //size
		MOV_WriteB32Tag3(pb, "ver ");
		MOV_WriteBuffer3(pb, (char *)gMediaVersionAddr, VERSION_LENGTH);//
	}
	//thma
	thumbsize4 = ALIGN_CEIL_4(pHdr->uiThumbSize);
	//#NT#2019/10/15#Willy Su -begin
	//#NT#To support write 'frea'/'skip' tag without thumbnail
	if (thumbsize4)
	{
		MOV_WriteB32Bits3(pb, thumbsize4 + 8); //size+len+tag
		MOV_WriteB32Tag3(pb, "thma");
		MOV_WriteBuffer3(pb, (char *)(pHdr->uiThumbAddr), thumbsize4);
	}
	//#NT#2019/10/15#Willy Su -end

	//scra
	screensize4 = ALIGN_CEIL_4(pHdr->uiScreenSize);
	if (screensize4) {
		MOV_WriteB32Bits3(pb, screensize4 + 8); //size+len+tag
		MOV_WriteB32Tag3(pb, "scra");
		MOV_WriteBuffer3(pb, (char *)(pHdr->uiScreenAddr), screensize4);
	}
	MOV_TellOffset3(pb);

	len = MOVWriteUpdateData3(pb, pos);
	//DBG_DUMP("FREA len=0x%lx!curpos=0x%lx!\r\n", len, curpos);
	return len;

}
/*
  Write STCO chunk offset atom
  size + tag + flag +
  numberEntries +
  table of (chunkInFilePosition) : ChunkOffsetTable

  @param pb     buffer pointer
  @param ptrack track infomation
  @return  this atom size
*/
UINT32 MOVWriteSTCOTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 i, mode64 = 0, newpos, lastsize = 0, lastpos = 0;
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_Ientry      *pEntry = 0;
	MOV_Ientry      *p_head = 0;
	MOV_Ientry      *p_max = 0;
	p_head = ptrack->entry_head;
	p_max = ptrack->entry_max;

	MOV_WriteB32Bits(pb, 0);//size

	if (gMovCo64) {
		mode64 = 1;
		MOV_WriteB32Tag(pb, "co64");
	} else {
		MOV_WriteB32Tag(pb, "stco");
	}
	MOV_WriteB32Bits(pb, 0);//version and flags
	MOV_WriteB32Bits(pb, ptrack->entry); //entry count
	for (i = 0; i < ptrack->entry; i++) {
		if (g_mov_sync_pos) {
			pEntry = p_head;
		} else {
			pEntry = (ptrack->cluster) + i;
		}

		if (mode64 == 1) {
			MOV_WriteB64Bits(pb, pEntry->pos);
		} else {
			if ((pEntry->pos != 0)) { //&&(i% gMovRecFileInfo.VideoFrameRate == 0))
				MOV_WriteB32Bits(pb, pEntry->pos);
				if (0) { //(i==0)
					DBG_DUMP("[%d]pos=0x%llx   \r\n", i, ptrack->cluster[i].pos);
				}
			} else {
				newpos = lastsize + lastpos; //(ptrack->cluster[i-1].pos)+(ptrack->cluster[i-1].size);
				MOV_WriteB32Bits(pb, newpos);
				pEntry->pos = newpos;
				if (0) { //(i==0)
					DBG_DUMP("last=0x%lx, size=0x%lx!\r\n", lastpos, lastsize);
				}
			}
			lastsize = pEntry->size;
			lastpos = pEntry->pos;
		}

		if (g_mov_sync_pos) {
			if (p_head == p_max) {
				p_head = ptrack->cluster;
			} else {
				p_head++;
			}
		}
	}
	return MOVWriteUpdateData(pb, pos);
}
/*
  Write STSZ sample size atom
  size + tag + flag +
  sampleSize +
  numberEntries +
  table of (each sample size) : SampleSizeTable (if sampleSize not equal to 0, need this table)

  @param pb     buffer pointer
  @param ptrack track infomation
  @return this atom size
*/
static UINT32 MOVWriteSTSZTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 i;
	INT32  tst, oldtst = -1, sampleSize;
	UINT32 equalSampleSize = 1;
	MOV_Ientry      *p_head = 0;
	MOV_Ientry      *p_max = 0;
	p_head = ptrack->entry_head;
	p_max = ptrack->entry_max;
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "stsz");
	MOV_WriteB32Bits(pb, 0);//version and flags
	//DBG_DUMP("TOTLptrack->entry=%d\r\n", ptrack->entry);
	if (ptrack->context->codec_type == REC_VIDEO_TRACK) {

		for (i = 0; i < ptrack->entry; i++) {
			if (g_mov_sync_pos) {
				tst = p_head->size;
			} else {
				tst = (ptrack->cluster[i].size);// /(ptrack->cluster[i].entries);
			}
			if ((oldtst != -1) && (tst != oldtst)) {
				equalSampleSize = 0;
			}
			oldtst = tst;
			//entries += ptrack->cluster[i].entries;

			if (g_mov_sync_pos) {
				if (p_head == p_max) {
					p_head = ptrack->cluster;
				} else {
					p_head++;
				}
			}
		}
		//2013/06/14 Meg add
		if (ptrack->entry == 1) { //timelapse only 1 frame, should be non-equal form
			equalSampleSize = 0;
		}
		if (equalSampleSize) {
			sampleSize = (ptrack->cluster[0].size);// /(ptrack->cluster[0].entries);
			MOV_WriteB32Bits(pb, sampleSize); //size
			MOV_WriteB32Bits(pb, ptrack->entry); //entry count
		} else {
			MOV_WriteB32Bits(pb, 0); //size = 0, if sampleSizes not equal
			MOV_WriteB32Bits(pb, ptrack->entry); //entry count
			//DBG_DUMP("^R firstsize=%d!\r\n", ptrack->cluster[0].size);
			p_head = ptrack->entry_head;
			for (i = 0; i < ptrack->entry; i++) {
				//for (j=0; j<ptrack->cluster[i].entries; j++)
				{
					if (g_mov_sync_pos) {
						sampleSize = (p_head->size);
					} else {
						sampleSize = (ptrack->cluster[i].size);// /(ptrack->cluster[i].entries);
					}
					MOV_WriteB32Bits(pb, sampleSize); //size
				}

				if (g_mov_sync_pos) {
					if (p_head == p_max) {
						p_head = ptrack->cluster;
					} else {
						p_head++;
					}
				}
			}
		}
	} else if ((ptrack->context->codec_type == REC_AUDIO_TRACK)
			   && (ptrack->tag == MOV_AUDFMT_AAC)) { //aac, mp4a
		MOV_WriteB32Bits(pb, 0); //size = 0, if sampleSizes not equal
		MOV_WriteB32Bits(pb, ptrack->entry); //entry count
		//DBG_DUMP("audCount!!!!!=%d!!!!!!", ptrack->entry);
		for (i = 0; i < ptrack->entry; i++) {
			//for (j=0; j<ptrack->cluster[i].entries; j++)
			{
				if (g_mov_sync_pos) {
					sampleSize = (p_head->size);// /(ptrack->cluster[i].entries);
				} else {
					sampleSize = (ptrack->cluster[i].size);// /(ptrack->cluster[i].entries);
				}
				MOV_WriteB32Bits(pb, sampleSize); //size

				if (g_mov_sync_pos) {
					if (p_head == p_max) {
						p_head = ptrack->cluster;
					} else {
						p_head++;
					}
				}
			}
		}
	} else { //audio, PCM
		// audioSize is original audio raw data size, in unit of byte, stsz size is in unit of sample

		UINT32 oneAudRawSize = 2; // 1 audio raw data size is fixed to 16-bit (2 bytes)

		MOV_WriteB32Bits(pb, 1); //num
		//#NT#2009/12/21#Meg Lin -begin
		//#NT#new feature: rec stereo
		MOV_WriteB32Bits(pb, (ptrack->audioSize / ptrack->context->nchannel) / oneAudRawSize); // total samples, not bytes
		//#NT#2009/12/21#Meg Lin -end
		//DBG_DUMP("1=0x%lx, 2=0x%lx!3=0x%lx!\r\n", ptrack->audioSize,ptrack->context->nchannel,ptrack->context->wBitsPerSample);

	}
	return MOVWriteUpdateData(pb, pos);
}

static UINT32 MOV_GetTimeToSample(UINT32 vfr)
{
#if 0

	UINT32  uiTimeToSample;

	if (gMovRecFileInfo.VideoFrameRate < 10) { // MEDIAREC_FAST_FWD
		uiTimeToSample = MOV_TIMESCALE / 30; // play as 30 fps
	} else {
		uiTimeToSample = MOV_TIMESCALE / gMovRecFileInfo.VideoFrameRate;
	}
	return uiTimeToSample;

#else // since we have time lapse recording mode, don't need to set special value (such as fixed to play 30fps)
	UINT32 timescale = MOV_Wrtie_GetTimeScale(vfr);
	if (vfr > 1000) {
		return (timescale * 100 / vfr);
	} else {
		return (timescale / vfr);
	}

#endif
}

static UINT32 MOVWriteCTTSTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 i, cttsV;

#if 0 //IPBR
	INT32 temp[4] = {3000, 0, -2000, -1000};//scale = 1000

	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "ctts");


	{
		MOV_WriteB32Bits(pb, 0); //size = 0, if sampleSizes not equal
		MOV_WriteB32Bits(pb, ptrack->entry); //entry count
		for (i = 0; i < ptrack->entry; i++) {
			MOV_WriteB32Bits(pb, 0x01); //count = 1
			if (i == 0) {
				cttsV = 0;
			} else {
				cttsV = temp[(i - 1) % 4];
			}
			MOV_WriteB32Bits(pb, cttsV);
		}
	}
	return MOVWriteUpdateData(pb, pos);
#elif 0 //I PBB PBB
	if (gMovRecFileInfo.uiH264GopType == MEDIAREC_GOP_IPB) { // Hideo@120322
		INT32 temp[3] = {2000, -1000, -1000};//scale = 1000

		MOV_Offset  pos = MOV_TellOffset(pb);
		MOV_WriteB32Bits(pb, 0);//size

		MOV_WriteB32Tag(pb, "ctts");


		{
			MOV_WriteB32Bits(pb, 1000); //size = 0, if sampleSizes not equal
			MOV_WriteB32Bits(pb, ptrack->entry); //entry count
			for (i = 0; i < ptrack->entry; i++) {
				MOV_WriteB32Bits(pb, 0x01); //count = 1
				if (i == 0) {
					cttsV = 0;
				} else {
					cttsV = temp[(i - 1) % 3];
				}
				MOV_WriteB32Bits(pb, cttsV);
			}
		}
		return MOVWriteUpdateData(pb, pos);
	}
#elif 1 //IBB PBB PBB
	if (ptrack->uiH264GopType == MEDIAREC_GOP_IPB) { // Hideo@120322
		INT32   temp[3] = {2, -1, -1};
		UINT32  uiTimeToSample;

		MOV_Offset  pos = MOV_TellOffset(pb);
		MOV_WriteB32Bits(pb, 0);//size

		MOV_WriteB32Tag(pb, "ctts");

		uiTimeToSample = MOV_GetTimeToSample(ptrack->frameRate);

		for (i = 0; i < sizeof(temp) / sizeof(INT32); i++) {
			temp[i] *= uiTimeToSample;
		}

		MOV_WriteB32Bits(pb, 0); //size = 0, if sampleSizes not equal
		MOV_WriteB32Bits(pb, ptrack->entry); //entry count
		for (i = 0; i < ptrack->entry; i++) {
			MOV_WriteB32Bits(pb, 0x01); //count = 1
			cttsV = temp[i % 3];
			MOV_WriteB32Bits(pb, cttsV);
		}

		return MOVWriteUpdateData(pb, pos);
	}
#else //I PRBB PRBB
	INT32 temp[4] = {3000, 0, -2000, -1000};//scale = 1000

	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "ctts");


	{
		MOV_WriteB32Bits(pb, 1000); //size = 0, if sampleSizes not equal
		MOV_WriteB32Bits(pb, ptrack->entry); //entry count
		for (i = 0; i < ptrack->entry; i++) {
			MOV_WriteB32Bits(pb, 0x01); //count = 1
			if (i == 0) {
				cttsV = 0;
			} else {
				cttsV = temp[(i - 1) % 4];
			}
			MOV_WriteB32Bits(pb, cttsV);
		}
	}
	return MOVWriteUpdateData(pb, pos);
#endif

	return 0;
}

/*
  Write STSC sample to chunk atom
  size + tag + flag +
  numberEntries +
  table of (first chunk + samplesInChunk + sampleDescID) : SampleToChunkTable


  @param pb      buffer pointer
  @param ptrack  track infomation
  @return this atom size
*/
static UINT32 MOVWriteSTSCTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 i;
	UINT8  bytePerSample;
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_Offset  curpos;
	MOV_Ientry      *p_head = 0;
	MOV_Ientry      *p_max = 0;
	p_head = ptrack->entry_head;
	p_max = ptrack->entry_max;
	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "stsc");

	MOV_WriteB32Bits(pb, 0);//version and flags

	MOV_TellOffset(pb);

	if (ptrack->context->codec_type == REC_VIDEO_TRACK) {
#if 1
		MOV_WriteB32Bits(pb, 1);//number must be 1
		/*for (i=0; i< ptrack->entry; i++)
		{
		    nowval = ptrack->cluster[i].samplesInChunk;
		    if (oldval != nowval)
		    {

		        MOV_WriteB32Bits(pb, i+1);//first chunk number using this table
		        MOV_WriteB32Bits(pb, nowval);//samplesInChunk
		        MOV_WriteB32Bits(pb, 0x01);//sample desc ID, must be 1
		        oldval = nowval;
		        index ++;
		    }
		}*/
		MOV_WriteB32Bits(pb, 1);//first chunk number using this table
		MOV_WriteB32Bits(pb, 1);//samplesInChunk
		MOV_WriteB32Bits(pb, 0x01);//sample desc ID, must be 1

		//update index number
		curpos = MOV_TellOffset(pb);
		//MOV_SeekOffset(pb, entryPos, KFS_SEEK_SET);
		//MOV_WriteB32Bits(pb, index);
		MOV_SeekOffset(pb, curpos, KFS_SEEK_SET);
#else//FAKE_264
		MOV_WriteB32Bits(pb, 1);//number must be 1
		MOV_WriteB32Bits(pb, 1);//start from 1
		MOV_WriteB32Bits(pb, 1);//13 frames
		MOV_WriteB32Bits(pb, 1);//13 frames
		//MOV_WriteB32Bits(pb, 1);//number must be 1
		//MOV_WriteB32Bits(pb, 2);//start from 2
		//MOV_WriteB32Bits(pb, 0x10);//16 frames
		//MOV_WriteB32Bits(pb, 1);//number must be 1
#endif
	} else if ((ptrack->context->codec_type == REC_AUDIO_TRACK)
			   && (ptrack->tag == MOV_AUDFMT_AAC)) { //aac, mp4a
		MOV_WriteB32Bits(pb, 1);//number must be 1
		MOV_WriteB32Bits(pb, 1);//first chunk number using this table
		MOV_WriteB32Bits(pb, 1);//samplesInChunk
		MOV_WriteB32Bits(pb, 0x01);//sample desc ID, must be 1

	} else { //audio, pcm
		bytePerSample = ptrack->context->wBitsPerSample / 8;
		MOV_WriteB32Bits(pb, ptrack->entry);//entry number
		for (i = 0; i < ptrack->entry; i++) {
			MOV_WriteB32Bits(pb, i + 1); //first chunk number using this table
			//#NT#2009/12/21#Meg Lin -begin
			//#NT#new feature: rec stereo
			if (g_mov_sync_pos) {
				MOV_WriteB32Bits(pb, (p_head->size / ptrack->context->nchannel) / bytePerSample); //samplePerChunk
			} else {
				MOV_WriteB32Bits(pb, (ptrack->cluster[i].size / ptrack->context->nchannel) / bytePerSample); //samplePerChunk
			}
			//#NT#2009/12/21#Meg Lin -end

			MOV_WriteB32Bits(pb, 1);//desc ID

			if (g_mov_sync_pos) {
				if (p_head == p_max) {
					p_head = ptrack->cluster;
				} else {
					p_head++;
				}
			}
		}
	}

	return MOVWriteUpdateData(pb, pos);
}

#if 1  //variable-rate supported
static UINT32 MOVWriteSTTSTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 entries;
	UINT32 atom_size;

	if (ptrack->context->codec_type == REC_VIDEO_TRACK) {
		MOV_Ientry      *pEntry = 0;
		UINT32 atom_entry_count = 0;
		UINT32 sample_count = 0;
		UINT32 i;
		UINT32 rate = 0, next_rate, first_rate = 0;

		if (g_mov_sync_pos) {
			entries = ptrack->entry;
			atom_size = 0x18;
			MOV_WriteB32Bits(pb, atom_size); // size
			MOV_WriteB32Tag(pb, "stts");
			MOV_WriteB32Bits(pb, 0); // version & flags
			MOV_WriteB32Bits(pb, 1);
			MOV_WriteB32Bits(pb, entries);//count
			if (ptrack->frameRate == 0) {
				ptrack->frameRate = 30;
			}
			MOV_WriteB32Bits(pb, MOV_GetTimeToSample(ptrack->frameRate));
			return atom_size;
		}

		entries = ptrack->entry;

		for (i = 0; i < entries; i++) {
			pEntry = (ptrack->cluster) + i;
			if (pEntry->key_frame) {
				next_rate = pEntry->rev;
				pEntry->updated = 0;
				if (next_rate != rate) {
					if (i != 0) {
						pEntry = (ptrack->cluster) + (i-1);
						pEntry->updated = rate;
						atom_entry_count++;
						DBG_IND("[%d] rate=%d, i=%d\r\n", atom_entry_count, pEntry->updated, i);
					} else {
						first_rate = next_rate;
					}
					rate = next_rate;
				}
			} else {
				pEntry->updated = 0;
			}
		}
		if (i == entries) {
			pEntry = (ptrack->cluster) + (i-1);
			pEntry->updated = rate;
			if (pEntry->updated) {
				atom_entry_count++;
			}
			DBG_IND("[%d] rate=%d, i=%d\r\n", atom_entry_count, pEntry->updated, i);
		}
		DBG_IND("atom_entry_count=%d\r\n", atom_entry_count);

		if (atom_entry_count) {
			atom_size = 0x10;
			atom_size += 8 * atom_entry_count;
			MOV_WriteB32Bits(pb, atom_size); // size
			MOV_WriteB32Tag(pb, "stts");
			MOV_WriteB32Bits(pb, 0); // version & flags
			MOV_WriteB32Bits(pb, atom_entry_count);
			rate = first_rate;
			for (i = 0; i < entries; i++) {
				sample_count++;
				pEntry = (ptrack->cluster) + i;
				if (pEntry->updated) {
					MOV_WriteB32Bits(pb, sample_count);//count
					MOV_WriteB32Bits(pb, MOV_GetTimeToSample(pEntry->updated));
					sample_count = 0;
					rate = pEntry->updated;
					pEntry->updated = 0;
				} else {
					//do nothing
				}
			}
		} else {
			atom_size = 0x18;
			MOV_WriteB32Bits(pb, atom_size); // size
			MOV_WriteB32Tag(pb, "stts");
			MOV_WriteB32Bits(pb, 0); // version & flags
			MOV_WriteB32Bits(pb, 1); //
			MOV_WriteB32Bits(pb, entries);//count
			if (ptrack->frameRate == 0) {
				ptrack->frameRate = 30;
			}
			MOV_WriteB32Bits(pb, MOV_GetTimeToSample(ptrack->frameRate));
		}

		return atom_size;

	} else if ((ptrack->context->codec_type == REC_AUDIO_TRACK)
			   && (ptrack->tag == MOV_AUDFMT_AAC)) { //aac, mp4a
		MOV_WriteB32Bits(pb, 0x18); // size
		MOV_WriteB32Tag(pb, "stts");
		MOV_WriteB32Bits(pb, 0); // version & flags
		MOV_WriteB32Bits(pb, 1); //size = 0, if sampleSizes not equal
		MOV_WriteB32Bits(pb, ptrack->entry); //entry count
		MOV_WriteB32Bits(pb, 0x400); //size
	} else if (ptrack->context->codec_type == REC_AUDIO_TRACK) {
		// audioSize is original audio raw data size, in unit of byte, stsz size is in unit of sample

		UINT32 oneAudRawSize = 2; // 1 audio raw data size is fixed to 16-bit (2 bytes)

		MOV_WriteB32Bits(pb, 0x18); // size
		MOV_WriteB32Tag(pb, "stts");
		MOV_WriteB32Bits(pb, 0); // version & flags
		MOV_WriteB32Bits(pb, 1); //
		//for (i=0; i<entries; i++) {
		//#NT#2009/12/21#Meg Lin -begin
		//#NT#new feature: rec stereo
		MOV_WriteB32Bits(pb, (ptrack->audioSize / ptrack->context->nchannel) / oneAudRawSize); // total samples, not bytes
		//#NT#2009/12/21#Meg Lin -end
		MOV_WriteB32Bits(pb, 1);//duration = 1
		//}
		return 0x18;
	}
	return 0x18;
}
#else
static UINT32 MOVWriteSTTSTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 entries;
	UINT32 atom_size;

	if (ptrack->context->codec_type == REC_VIDEO_TRACK) {
#if 1
		//#NT#2009/11/24#Meg Lin -begin

		entries = ptrack->entry;
		atom_size = 0x18;
		MOV_WriteB32Bits(pb, atom_size); // size
		MOV_WriteB32Tag(pb, "stts");
		MOV_WriteB32Bits(pb, 0); // version & flags
		MOV_WriteB32Bits(pb, 1); //
		//MOV_WriteB32Bits(pb, entries); //
		//for (i=0; i<entries; i++) {
		//    MOV_WriteB32Bits(pb, 1);//count
		//    MOV_WriteB32Bits(pb, ptrack->cluster[i].duration);//duration
		//}

		MOV_WriteB32Bits(pb, entries);//count
		//MOV_WriteB32Bits(pb, ptrack->cluster[0].duration);//duration
#if 0
		if (gMovRecFileInfo.VideoFrameRate < 10) { // MEDIAREC_FAST_FWD
			MOV_WriteB32Bits(pb, MOV_TIMESCALE / 30); //play as 30 fps
		} else {
			MOV_WriteB32Bits(pb, MOV_TIMESCALE / gMovRecFileInfo.VideoFrameRate); //duration
		}
#else
		if (ptrack->frameRate == 0) {
			ptrack->frameRate = 30;
		}
		MOV_WriteB32Bits(pb, MOV_GetTimeToSample(ptrack->frameRate));
#endif
		//#NT#2009/11/24#Meg Lin -end
		return atom_size;
#else//FAKE_264
		entries = ptrack->entry;
		atom_size = 0x18;
		MOV_WriteB32Bits(pb, atom_size); // size
		MOV_WriteB32Tag(pb, "stts");
		MOV_WriteB32Bits(pb, 0); // version & flags
		MOV_WriteB32Bits(pb, 1); // one entry
		MOV_WriteB32Bits(pb, entries);//count
		//MOV_WriteB32Bits(pb, ptrack->cluster[1].duration);//duration
		MOV_WriteB32Bits(pb, 1000);//duration FAKE_264
		return atom_size;
#endif
	} else if ((ptrack->context->codec_type == REC_AUDIO_TRACK)
			   && (ptrack->tag == MOV_AUDFMT_AAC)) { //aac, mp4a
		MOV_WriteB32Bits(pb, 0x18); // size
		MOV_WriteB32Tag(pb, "stts");
		MOV_WriteB32Bits(pb, 0); // version & flags
		MOV_WriteB32Bits(pb, 1); //size = 0, if sampleSizes not equal
		MOV_WriteB32Bits(pb, ptrack->entry); //entry count
		MOV_WriteB32Bits(pb, 0x400); //size
	} else if (ptrack->context->codec_type == REC_AUDIO_TRACK) {
		// audioSize is original audio raw data size, in unit of byte, stsz size is in unit of sample

		UINT32 oneAudRawSize = 2; // 1 audio raw data size is fixed to 16-bit (2 bytes)

		MOV_WriteB32Bits(pb, 0x18); // size
		MOV_WriteB32Tag(pb, "stts");
		MOV_WriteB32Bits(pb, 0); // version & flags
		MOV_WriteB32Bits(pb, 1); //
		//for (i=0; i<entries; i++) {
		//#NT#2009/12/21#Meg Lin -begin
		//#NT#new feature: rec stereo
		MOV_WriteB32Bits(pb, (ptrack->audioSize / ptrack->context->nchannel) / oneAudRawSize); // total samples, not bytes
		//#NT#2009/12/21#Meg Lin -end
		MOV_WriteB32Bits(pb, 1);//duration = 1
		//}
		return 0x18;
	}
	return 0x18;
}
#endif //variable-rate supported
/*
  Write STSS sync sample atom
  size + tag + flag +
  numberEntries +
  table of (chunkInFilePosition) : syncSampleTable

  @param pb      buffer pointer
  @param ptrack  track infomation
  @return this atom size
*/
UINT32 MOVWriteSTSSTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 i;
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_Offset  entryPos, curpos;
	INT32   index = 0;
	MOV_Ientry      *p_head = 0;
	MOV_Ientry      *p_max = 0;
	p_head = ptrack->entry_head;
	p_max = ptrack->entry_max;

	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "stss");
	MOV_WriteB32Bits(pb, 0);//version and flags

	entryPos = MOV_TellOffset(pb);
#if 1
	MOV_WriteB32Bits(pb, ptrack->entry);//entry number
	for (i = 0; i < ptrack->entry; i++) {

		if (g_mov_sync_pos) {
			if (p_head->key_frame == 1) {
				//DBG_DUMP("[%d] =i-frame\r\n", i);
				MOV_WriteB32Bits(pb, i + 1); //this is a key_frame
				index ++;
			}
		} else {
			if (ptrack->cluster[i].key_frame == 1) {
				//DBG_DUMP("[%d] =i-frame\r\n", i);
				MOV_WriteB32Bits(pb, i + 1); //this is a key_frame
				index ++;
			}
		}

		if (g_mov_sync_pos) {
			if (p_head == p_max) {
				p_head = ptrack->cluster;
			} else {
				p_head++;
			}
		}
	}
	//update index number
	curpos = MOV_TellOffset(pb);
	MOV_SeekOffset(pb, entryPos, KFS_SEEK_SET);
	MOV_WriteB32Bits(pb, index);
	MOV_SeekOffset(pb, curpos, KFS_SEEK_SET);
#else//FAKE_264

#if 1//I PRBB
	MOV_WriteB32Bits(pb, 5);//entry number
	MOV_WriteB32Bits(pb, 1);//this is a key_frame
	MOV_WriteB32Bits(pb, 0xe);//this is a key_frame
	MOV_WriteB32Bits(pb, 0x1e);//this is a key_frame
	MOV_WriteB32Bits(pb, 0x2e);//this is a key_frame
	MOV_WriteB32Bits(pb, 0x3e);//this is a key_frame
#else//I PBB
	MOV_WriteB32Bits(pb, 5);//entry number
	MOV_WriteB32Bits(pb, 1);//this is a key_frame
	MOV_WriteB32Bits(pb, 0xe);//this is a key_frame
	MOV_WriteB32Bits(pb, 0x1d);//this is a key_frame
	MOV_WriteB32Bits(pb, 0x2c);//this is a key_frame
	MOV_WriteB32Bits(pb, 0x3b);//this is a key_frame


#endif
#endif

	return MOVWriteUpdateData(pb, pos);
}
#if 0
static UINT32 MOVWriteSDTPTag(char *pb, MOVREC_Track *ptrack)
{
#if 0//IRPB
	UINT32 i, cttsV;
	UINT32 temp[4] = {0, 0, 8, 8};//scale = 22

	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "sdtp");
	MOV_WriteB32Bits(pb, 0); //version

	{
		MOV_WriteB8Bits(pb, 0); //size = 0, if sampleSizes not equal
		for (i = 1; i < ptrack->entry; i++) {
			cttsV = temp[(i - 1) % 4];
			MOV_WriteB8Bits(pb, cttsV);
		}
	}
	return MOVWriteUpdateData(pb, pos);
#else
#if 1//I PBB
	UINT32 i, cttsV;
	UINT32 temp[3] = {0, 8, 8};//scale = 22

	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "sdtp");
	MOV_WriteB32Bits(pb, 0); //version

	{
		MOV_WriteB8Bits(pb, 0); //size = 0, if sampleSizes not equal
		for (i = 1; i < ptrack->entry; i++) {
			cttsV = temp[(i - 1) % 3];
			MOV_WriteB8Bits(pb, cttsV);
		}
	}
	return MOVWriteUpdateData(pb, pos);
#else//I PRBB
	UINT32 i, cttsV;
	UINT32 temp[4] = {0, 0, 8, 8};//scale = 22

	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "sdtp");
	MOV_WriteB32Bits(pb, 0); //version

	{
		MOV_WriteB8Bits(pb, 0); //size = 0, if sampleSizes not equal
		for (i = 1; i < ptrack->entry; i++) {
			cttsV = temp[(i - 1) % 4];
			MOV_WriteB8Bits(pb, cttsV);
		}
	}
	return MOVWriteUpdateData(pb, pos);
#endif
#endif
}
#endif

#if 0
/*
  Write ENDA endian atom

  @param pb      buffer pointer
  @param ptrack  track infomation
  @return this atom size
*/
static UINT32 MOVWriteENDATag(char *pb, MOVREC_Track *ptrack)
{

	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "enda");
	MOV_WriteB16Bits(pb, 1);//little endian

	return 0xA;
}
#endif

/*
  Write PCMWAVE wave atom
  size + tag +


  @param pb     buffer pointer
  @param ptrack track infomation
  @return this atom size
*/
static UINT32 MOVWritePCMWAVETag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);

	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Bits(pb, 0x6D730002); //MicrosoftADPCM 0x6D730002
	{
		MOV_WriteL16Bits(pb, 0x02);//tag
		MOV_WriteL16Bits(pb, ptrack->context->nchannel);
		MOV_WriteL32Bits(pb, ptrack->context->nSamplePerSecond);//track.timescale
		MOV_WriteL32Bits(pb, ptrack->context->nAvgBytesPerSecond);
		MOV_WriteL16Bits(pb, ptrack->context->nBlockAlign);
		MOV_WriteL16Bits(pb, ptrack->context->wBitsPerSample);
		MOV_WriteL16Bits(pb, ptrack->context->cbSize);
		MOV_WriteL16Bits(pb, ptrack->context->nSamplePerBlock);
		MOV_WriteL16Bits(pb, ptrack->context->nNumCoeff);
		//coeff table 32 byte
		MOV_WriteL32Bits(pb, 0x00000100);
		MOV_WriteL32Bits(pb, 0);
		MOV_WriteL32Bits(pb, 0);
		MOV_WriteL32Bits(pb, 0);
		MOV_WriteL32Bits(pb, 0);
		MOV_WriteL32Bits(pb, 0);
		MOV_WriteL32Bits(pb, 0);
	}
	return MOVWriteUpdateData(pb, pos);
}
/*
  Write WAVE wave atom
  size + tag +
  frma atom (size 0x0c + tag) +
  ADPCM atom

  @param pb      buffer pointer
  @param ptrack  track infomation
  @return this atom size
*/
static UINT32 MOVWriteWAVETag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);

	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "wave");

	MOV_WriteB32Bits(pb, 12);//size
	MOV_WriteB32Tag(pb, "frma");

	MOV_WriteB32Bits(pb, ptrack->tag);
	if (ptrack->tag == 0x6D730002) { //MicrosoftADPCM 0x6D730002
		MOVWritePCMWAVETag(pb, ptrack);
	}
	MOV_WriteL32Bits(pb, 0x08000000);//reserved atom
	MOV_WriteL32Bits(pb, 0);//reserved

	return MOVWriteUpdateData(pb, pos);
}
/*
  Write sound sample desc atom
  size + tag +
  sound sample info +
  wave atom

  @param pb      buffer pointer
  @param ptrack  track infomation
  @return this atom size
*/
static UINT32 MOVWriteSoundDescTag(char *pb, MOVREC_Track *ptrack)
{
	UINT8 Byte0, Byte1;
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_Offset  pos2;
	MOV_WriteB32Bits(pb, 0);//size

	if (ptrack->tag == 0x6D730002) { //MicrosoftADPCM 0x6D730002
		MOV_WriteB32Bits(pb, 0x6D730002); //MicrosoftADPCM 0x6D730002
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 1);//data-reference index

		MOV_WriteB16Bits(pb, 1);//version
		MOV_WriteB16Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 0);//reserved

		MOV_WriteB16Bits(pb, ptrack->context->nchannel);
		MOV_WriteB16Bits(pb, 0x10);//should be bitsPerSample but not sync in ADPCM?
		MOV_WriteB16Bits(pb, 0);//compressID
		MOV_WriteB16Bits(pb, 0);//reserved
		MOV_WriteB16Bits(pb, ptrack->context->nSamplePerSecond);//track.timescale
		MOV_WriteB16Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, ptrack->context->nSamplePerBlock);//sample per packet
		MOV_WriteB32Bits(pb, ptrack->context->nBlockAlign);//bytes per packet
		MOV_WriteB32Bits(pb, ptrack->context->nBlockAlign);//bytes per frame
		MOV_WriteB32Bits(pb, 2);//should be bytsPerSample but not sync in ADPCM?
		MOVWriteWAVETag(pb, ptrack);
	} else if (ptrack->tag == MOV_AUDFMT_AAC) { //mp4a
		MOV_WriteB32Tag(pb, "mp4a");
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 1);//data-reference index

		MOV_WriteB16Bits(pb, 0);//version
		MOV_WriteB16Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 0);//reserved

		MOV_WriteB16Bits(pb, ptrack->context->nchannel);
		MOV_WriteB16Bits(pb, 0x10);//should be bitsPerSample but not sync in ADPCM?
		MOV_WriteB8Bits(pb, 0xFF);//compressID
		MOV_WriteB8Bits(pb, 0xFE);//reserved
		MOV_WriteB32Bits(pb, ptrack->context->nSamplePerSecond);//track.timescale
		MOV_WriteB16Bits(pb, 0);//reserved
		pos2 = MOV_TellOffset(pb);
		MOV_WriteB32Bits(pb, 0);//size

		MOV_WriteB32Tag(pb, "esds");
		MOV_WriteB32Bits(pb, 0);//flag and version

		MOV_WriteB8Bits(pb, 3);//tag1 : 0x03
		MOV_WriteB8Bits(pb, 0x19);//len

		MOV_WriteB16Bits(pb, 0);
		MOV_WriteB8Bits(pb, 0x0);//reseved

		MOV_WriteB8Bits(pb, 4);//tag2 : 0x04
		MOV_WriteB8Bits(pb, 0x11);//len
		MOV_WriteB8Bits(pb, 0x40);//codec_id

		MOV_WriteB8Bits(pb, 0x15);//audio track
		MOV_WriteB8Bits(pb, 0x10);//buffer size byte2
		MOV_WriteB16Bits(pb, 0x00);//buffer size byte1, 0

		MOV_WriteB32Bits(pb, ptrack->context->rc_max_rate);
		MOV_WriteB32Bits(pb, ptrack->context->rc_max_rate);//avg rate

		MOV_WriteB8Bits(pb, 5);//tag3 : 0x05
		MOV_WriteB8Bits(pb, 2);//len

		//Byte 0, aaaaa = 0001 0 , LC profile
		//        bbbb  sampleRate
		//        cccc  channels
		//   96000 0x0, 88200 0x1, 64000 0x2, 48000 0x3
		//   44100 0x4, 32000 0x5, 24000 0x6, 22050 0x7
		//   16000 0x8, 12000 0x9, 11025 0xa, 8000  0xb
		//   0xc~0xf reserved

		//ex 8000, ch1  = 0001 0101 1000 1000
		//                1    5    8    8
		Byte0 = 0x10;
		Byte1 = 0;
		switch (ptrack->context->nSamplePerSecond) {
		case AUDIO_SR_8000:
			Byte0 |= (0xb) >> 1;
			Byte1 |= (0x1) << 7; //fix coverity old:(0xb)<<7
			break;
		case AUDIO_SR_11025:
			Byte0 |= (0xa) >> 1;
			Byte1 |= (0x0) << 7; //fix coverity old:(0xa)<<7
			break;
		case AUDIO_SR_12000:
			Byte0 |= (0x9) >> 1;
			Byte1 |= (0x1) << 7; //fix coverity old:(0x9)<<7
			break;
		case AUDIO_SR_16000:
			Byte0 |= (0x8) >> 1;
			Byte1 |= (0x0) << 7; //fix coverity old:(0x8)<<7
			break;
		case AUDIO_SR_22050:
			Byte0 |= (0x7) >> 1;
			Byte1 |= (0x1) << 7; //fix coverity old:(0x7)<<7
			break;
		case AUDIO_SR_24000:
			Byte0 |= (0x6) >> 1;
			Byte1 |= (0x0) << 7; //fix coverity old:(0x6)<<7
			break;
		case AUDIO_SR_32000:
			Byte0 |= (0x5) >> 1;
			Byte1 |= (0x1) << 7; //fix coverity old:(0x5)<<7
			break;
		case AUDIO_SR_44100:
			Byte0 |= (0x4) >> 1;
			Byte1 |= (0x0) << 7; //fix coverity old:(0x4)<<7
			break;
		default:
		case AUDIO_SR_48000:
			Byte0 |= (0x3) >> 1;
			Byte1 |= (0x1) << 7; //fix coverity old:(0x3)<<7
			break;


		}
		Byte1 |= (ptrack->context->nchannel << 3);
		MOV_WriteB8Bits(pb, Byte0);
		MOV_WriteB8Bits(pb, Byte1);

		MOV_WriteB8Bits(pb, 6);//tag4 : 0x06
		MOV_WriteB8Bits(pb, 1);//size

		MOV_WriteB8Bits(pb, 2);//data, must be 2

		MOVWriteUpdateData(pb, pos2);
	} else if ((ptrack->tag == MOV_AUDFMT_ULAW) || (ptrack->tag == MOV_AUDFMT_ALAW)) { // ulaw, alaw
		if (ptrack->tag == MOV_AUDFMT_ULAW) {
			MOV_WriteB32Tag(pb, "ulaw");
		} else {
			MOV_WriteB32Tag(pb, "alaw");
		}
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 1);//data-reference index
		MOV_WriteB32Bits(pb, 0);//version
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB16Bits(pb, ptrack->context->nchannel);
		MOV_WriteB16Bits(pb, 0x08);//should be bitsPerSample but not sync ?
		MOV_WriteB16Bits(pb, 0);//compressID
		MOV_WriteB16Bits(pb, 0);//reserved
		MOV_WriteB16Bits(pb, ptrack->context->nSamplePerSecond);//11025
		MOV_WriteB16Bits(pb, 0);//reserved
	} else { //sowt little-endian, twos big-endian
		MOV_WriteB32Tag(pb, "sowt");
		MOV_WriteB32Bits(pb, 0);//flag and version
		MOV_WriteB32Bits(pb, 1);//index
		MOV_WriteB32Bits(pb, 0);//version
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB16Bits(pb, ptrack->context->nchannel);
		MOV_WriteB16Bits(pb, 0x10);//should be bitsPerSample but not sync ?
		MOV_WriteB16Bits(pb, 0);//compressID
		MOV_WriteB16Bits(pb, 0);//reserved
		MOV_WriteB16Bits(pb, ptrack->context->nSamplePerSecond);//11025
		MOV_WriteB16Bits(pb, 0);//reserved

	}
	return MOVWriteUpdateData(pb, pos);
}

/*
  Write ESDS atom
  size + tag +

  @param pb     buffer pointer
  @param ptrack track infomation
  @return this atom size
*/
static UINT32 MOVWriteESDSTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);

	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "esds");
	MOV_WriteB32Bits(pb, 0);//flag and version

	MOV_WriteB8Bits(pb, 3);//tag1 : 0x03
	MOV_WriteB8Bits(pb, 3 + 0x0f + ptrack->vosLen + 2 + 3); //reserved

	MOV_WriteB16Bits(pb, ptrack->trackID);
	MOV_WriteB8Bits(pb, 0x1f);//reseved

	MOV_WriteB8Bits(pb, 4);//tag2 : 0x04
	MOV_WriteB8Bits(pb, 0x0D + 2 + ptrack->vosLen); //reserved
	MOV_WriteB8Bits(pb, 0x20);//codec_id , 0x20 ASV1

	if (ptrack->context->codec_type == REC_AUDIO_TRACK) {
		MOV_WriteB8Bits(pb, 0x15);
	} else {
		MOV_WriteB8Bits(pb, 0x11);
	}
	MOV_WriteB8Bits(pb, ptrack->context->rc_buffer_size >> (3 + 16)); //buffersize, in bytes

	MOV_WriteB16Bits(pb, (ptrack->context->rc_buffer_size >> 3) & 0xFFFF); //buffersize

	MOV_WriteL32Bits(pb, ptrack->context->rc_max_rate);
	MOV_WriteL32Bits(pb, ptrack->context->rc_max_rate);//avg rate

	if (ptrack->vosLen) {
		MOV_WriteB8Bits(pb, 5);//tag3 : 0x05
		MOV_WriteB8Bits(pb, ptrack->vosLen);//reserved
		MOV_WriteBuffer(pb, (char *)ptrack->vosData, ptrack->vosLen);
	}
	MOV_WriteB8Bits(pb, 6);//tag4 : 0x06
	MOV_WriteB8Bits(pb, 1);//size

	MOV_WriteB8Bits(pb, 2);//data, must be 2




	return MOVWriteUpdateData(pb, pos);
}
/*
  Write video desc atom
  size + tag +

  @param pb     buffer pointer
  @param ptrack track infomation
  @return this atom size
*/
static UINT32 MOVWriteVideoDescTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 i, space;
	char codec_name[0x40];
	MOV_Offset  pos = MOV_TellOffset(pb);

	MOV_WriteB32Bits(pb, 0);//size

	if (ptrack->tag == 0x6D703476) { //mp4v
		MOV_WriteB32Tag(pb, "mp4v");
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 1);//data-reference index

		MOV_WriteB16Bits(pb, 2);//version
		MOV_WriteB16Bits(pb, 1);//level
		MOV_WriteB32Bits(pb, 0);//vendor reserved

		MOV_WriteB32Bits(pb, 0x400);//Temporal Quality
		MOV_WriteB32Bits(pb, 0x400);//Spatial Quality

		MOV_WriteB16Bits(pb, ptrack->context->width);
		//if (ptrack->context->width = 320)
		//{
		//     MOV_WriteB16Bits(pb, ptrack->context->height/2);
		//}
		//else
		{
			MOV_WriteB16Bits(pb, ptrack->context->height);
		}
		MOV_WriteB32Bits(pb, 0x00480000);//Horizontal resolution 72dpi
		MOV_WriteB32Bits(pb, 0x00480000);//Vertical resolution 72dpi

		MOV_WriteB32Bits(pb, 0x0);//Data size, must be 0
		MOV_WriteB16Bits(pb, 0x1);//frame count, 1
		memcpy((char *)codec_name, ptrack->context->codec_name, 31);
		MOV_WriteB8Bits(pb, strlen(codec_name));//len
		MOV_WriteBuffer(pb, codec_name, strlen(codec_name));

		space = 0x20 - 1 - strlen(codec_name);
		for (i = 0; i < space; i++) {
			MOV_WriteB8Bits(pb, 0x0);//add zero
		}
		MOV_WriteB16Bits(pb, 0x18);//depth, 32
		MOV_WriteB16Bits(pb, 0xFFFF);//default color table, (-1)

		MOVWriteESDSTag(pb, ptrack);
	}
	//else if (ptrack->tag == 0x6D6A7061)//mjpa (Motion JPEG A)
	else if (ptrack->tag == 0x6A706567) { //jpeg (Photo JPEG)
		//MOV_WriteB32Tag(pb, "mjpa");
		MOV_WriteB32Tag(pb, "jpeg");
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 1);//data-reference index

		MOV_WriteB16Bits(pb, 1);//version
		MOV_WriteB16Bits(pb, 1);//level
		MOV_WriteB32Tag(pb, "appl");//vendor reserved

		MOV_WriteB32Bits(pb, 0x100);//Temporal Quality
		MOV_WriteB32Bits(pb, 0x100);//Spatial Quality

		MOV_WriteB16Bits(pb, ptrack->context->width);
		//if (ptrack->context->width = 320)
		//{
		//    MOV_WriteB16Bits(pb, ptrack->context->height/2);
		//}
		//else
		{
			MOV_WriteB16Bits(pb, ptrack->context->height);
		}
		MOV_WriteB32Bits(pb, 0x00480000);//Horizontal resolution 72dpi
		MOV_WriteB32Bits(pb, 0x00480000);//Vertical resolution 72dpi

		MOV_WriteB32Bits(pb, 0x0);//Data size, must be 0
		MOV_WriteB16Bits(pb, 0x1);//frame count, 1
		memcpy((char *)codec_name, ptrack->context->codec_name, 31);
		MOV_WriteB8Bits(pb, strlen(codec_name));//len
		MOV_WriteBuffer(pb, codec_name, strlen(codec_name));

		space = 0x20 - 1 - strlen(codec_name);
		for (i = 0; i < space; i++) {
			MOV_WriteB8Bits(pb, 0x0);//add zero
		}
		MOV_WriteB16Bits(pb, 0x18);//depth, 32
		MOV_WriteB16Bits(pb, 0xFFFF);//default color table, (-1)

		MOV_WriteB32Bits(pb, 0xA);//Data len
		MOV_WriteB32Tag(pb, "fram");
		MOV_WriteB8Bits(pb, 0x2);//len
		MOV_WriteB8Bits(pb, 0x1);//default data
		MOV_WriteB32Bits(pb, 0x0);//pad
	} else if (ptrack->tag == 0x61766331) { //avc1
		MOV_WriteB32Tag(pb, "avc1");
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 1);//data-reference index

		MOV_WriteB16Bits(pb, 0);//version
		MOV_WriteB16Bits(pb, 0);//level
		MOV_WriteB32Bits(pb, 0);//vendor reserved

		MOV_WriteB32Bits(pb, 0x000);//Temporal Quality
		MOV_WriteB32Bits(pb, 0x000);//Spatial Quality

		MOV_WriteB16Bits(pb, ptrack->context->width);
		//if (ptrack->context->width = 320)
		//{
		//     MOV_WriteB16Bits(pb, ptrack->context->height/2);
		//}
		//else
		{
			MOV_WriteB16Bits(pb, ptrack->context->height);
		}
		MOV_WriteB32Bits(pb, 0x00480000);//Horizontal resolution 72dpi
		MOV_WriteB32Bits(pb, 0x00480000);//Vertical resolution 72dpi

		MOV_WriteB32Bits(pb, 0x0);//Data size, must be 0
		MOV_WriteB16Bits(pb, 0x1);//frame count, 1
		memcpy((char *)codec_name, ptrack->context->codec_name, 31);
		MOV_WriteB8Bits(pb, strlen(codec_name));//len
		MOV_WriteBuffer(pb, codec_name, strlen(codec_name));

		space = 0x20 - 1 - strlen(codec_name);
		for (i = 0; i < space; i++) {
			MOV_WriteB8Bits(pb, 0x0);//add zero
		}
		MOV_WriteB16Bits(pb, 0x18);//depth, 32
		MOV_WriteB16Bits(pb, 0xFFFF);//default color table, (-1)
#if 0//康西
		MOV_WriteB32Bits(pb, 0x2d);//Data len
		MOV_WriteB32Tag(pb, "avcC");
		MOV_WriteB32Bits(pb, 0x0142c033);
		MOV_WriteB32Bits(pb, 0xFFE10016);
		MOV_WriteB32Bits(pb, 0x6742c033);
		MOV_WriteB32Bits(pb, 0xAB40501E);
		MOV_WriteB32Bits(pb, 0xD0800000);
		MOV_WriteB32Bits(pb, 0x03008000);
		MOV_WriteB32Bits(pb, 0x0019478c);

		MOV_WriteB32Bits(pb, 0x19500100);
		MOV_WriteB32Bits(pb, 0x0468ce3c);
		MOV_WriteB8Bits(pb, 0x80);
#endif
#if 0//母魚
		MOV_WriteB32Bits(pb, 0x32);//Data len
		MOV_WriteB32Tag(pb, "avcC");
		MOV_WriteB32Bits(pb, 0x0142c033);
		MOV_WriteB32Bits(pb, 0xFFE1001B);
		MOV_WriteB32Bits(pb, 0x6742c033);
		MOV_WriteB32Bits(pb, 0xAB40501E);
		MOV_WriteB32Bits(pb, 0xDFF80228);
		MOV_WriteB32Bits(pb, 0x01A08000);
		MOV_WriteB32Bits(pb, 0x00030080);
		MOV_WriteB32Bits(pb, 0x00001947);

		MOV_WriteB32Bits(pb, 0x8c195001);
		MOV_WriteB32Bits(pb, 0x000468ce);
		MOV_WriteB16Bits(pb, 0x3c80);
#endif
#if 0//our 264 slice
		MOV_WriteB32Bits(pb, 0x20);//Data len
		MOV_WriteB32Tag(pb, "avcC");
		MOV_WriteB32Bits(pb, 0x014dc032);
		MOV_WriteB32Bits(pb, 0xFFE10009);
		MOV_WriteB32Bits(pb, 0x674d0032);
		MOV_WriteB32Bits(pb, 0xB8860b04);
		MOV_WriteB32Bits(pb, 0xb2010004);
		MOV_WriteB32Bits(pb, 0x68ce3c80);
		MOV_WriteB32Bits(pb, 0x00000014);
		MOV_WriteB32Bits(pb, 0x62747274);
		MOV_WriteB32Bits(pb, 0x00008252);
		MOV_WriteB32Bits(pb, 0x002b9f28);
		MOV_WriteB32Bits(pb, 0x00203428);
#endif
#if 0//our 264 I+P, 352x288
		MOV_WriteB32Bits(pb, 0x20);//Data len
		MOV_WriteB32Tag(pb, "avcC");
		MOV_WriteB32Bits(pb, 0x01420032);//sps and pps
		MOV_WriteB32Bits(pb, 0xFFE10009);
		MOV_WriteB32Bits(pb, 0x67420032);
		MOV_WriteB32Bits(pb, 0xB9D02c12);
		MOV_WriteB32Bits(pb, 0xc8010004);
		MOV_WriteB32Bits(pb, 0x68ce3880);
		//  btrt atom: bit rate, 1: buffer size, 2: avg bit rate, 3: max bit rate
		/*        MOV_WriteB32Bits(pb, 0x00000014);
		        MOV_WriteB32Bits(pb, 0x62747274);
		        MOV_WriteB32Bits(pb, 0x00007da9);
		        MOV_WriteB32Bits(pb, 0x0024d700);
		        MOV_WriteB32Bits(pb, 0x0017a580);  */ //
#endif
#if 0//Axxx
		MOV_WriteB32Bits(pb, 0x33);//Data len
		MOV_WriteB32Tag(pb, "avcC");
		MOV_WriteB32Bits(pb, 0x014d4029);//sps and pps
		MOV_WriteB32Bits(pb, 0xFFE1001c);
		MOV_WriteB32Bits(pb, 0x274d4029);
		MOV_WriteB32Bits(pb, 0x9a6280a0);
		MOV_WriteB32Bits(pb, 0x0b7603ee);
		MOV_WriteB32Bits(pb, 0x020203e0);
		MOV_WriteB32Bits(pb, 0x00007d20);
		MOV_WriteB32Bits(pb, 0x001d4c12);
		MOV_WriteB32Bits(pb, 0x80000000);
		MOV_WriteB32Bits(pb, 0x01000428);
		MOV_WriteB16Bits(pb, 0xee3c80);
#endif
#if 0//our 264 I+P, 640x480 //FAKE_264
#if 0
		if (gMovRecFileInfo.ImageWidth == 640) { //I PBB
			MOV_WriteB32Bits(pb, 0x20);//Data len
			MOV_WriteB32Tag(pb, "avcC");
			MOV_WriteB32Bits(pb, 0x014d0032);//sps and pps
			MOV_WriteB32Bits(pb, 0xFFE10009);
			MOV_WriteB32Bits(pb, 0x674d0032);
			MOV_WriteB32Bits(pb, 0xB9d81407);
			MOV_WriteB32Bits(pb, 0xb2010004);
			MOV_WriteB32Bits(pb, 0x68ce3880);
			//MOV_WriteB32Bits(pb, 0xec800000);
			//MOV_WriteB32Bits(pb, 0x00010004);
			//MOV_WriteB32Bits(pb, 0x68ce3880);
		}
#else//I PRBB
		if (gMovRecFileInfo.ImageWidth == 640) { //I PBB
			MOV_WriteB32Bits(pb, 0x21);//Data len
			MOV_WriteB32Tag(pb, "avcC");
			MOV_WriteB32Bits(pb, 0x014d0032);//sps and pps
			MOV_WriteB32Bits(pb, 0xFFE1000a);
			MOV_WriteB32Bits(pb, 0x674d0032);
			MOV_WriteB32Bits(pb, 0xB9ca0501);
			MOV_WriteB32Bits(pb, 0xec800100);
			MOV_WriteB32Bits(pb, 0x0468ce38);
			MOV_WriteB8Bits(pb, 0x80);

		}
#endif
		//  btrt atom: bit rate, 1: buffer size, 2: avg bit rate, 3: max bit rate
		//MOV_WriteB32Bits(pb, 0x00000014);
		//MOV_WriteB32Bits(pb, 0x62747274);
		//MOV_WriteB32Bits(pb, 0x00007da9);
		//MOV_WriteB32Bits(pb, 0x0024d700);
		//MOV_WriteB32Bits(pb, 0x0017a580);//
#endif
#if 0//our 264 I+P, 640x480, transcode
		MOV_WriteB32Bits(pb, 0x2d);//Data len
		MOV_WriteB32Tag(pb, "avcC");
		MOV_WriteB32Bits(pb, 0x01420032);//sps and pps
		MOV_WriteB32Bits(pb, 0xFEE10016);
		MOV_WriteB32Bits(pb, 0x6742c033);
		MOV_WriteB32Bits(pb, 0xab40501e);
		MOV_WriteB32Bits(pb, 0xd0800000);
		MOV_WriteB32Bits(pb, 0x03008000);
		MOV_WriteB32Bits(pb, 0x0019478c);
		MOV_WriteB32Bits(pb, 0x19500100);
		MOV_WriteB32Bits(pb, 0x0468ce3c);
		MOV_WriteB8Bits(pb, 0x80);
		//  btrt atom: bit rate, 1: buffer size, 2: avg bit rate, 3: max bit rate
		//MOV_WriteB32Bits(pb, 0x00000014);
		//MOV_WriteB32Bits(pb, 0x62747274);
		//MOV_WriteB32Bits(pb, 0x00010f87);
		//MOV_WriteB32Bits(pb, 0x0039bcf0);
		//MOV_WriteB32Bits(pb, 0x002827f8);//
#endif
#if 1//our 264 I+P, 1280x720
		//if (gMovRecFileInfo.ImageWidth == 1280)
		{
			UINT32 descLen = 0, *ptrDesc;
			/*
			    MOV_WriteB32Bits(pb, 0x21);//Data len
			    MOV_WriteB32Tag(pb, "avcC");
			    MOV_WriteB32Bits(pb, 0x014d0032);//sps and pps
			    MOV_WriteB32Bits(pb, 0xFfE1000A);
			    MOV_WriteB32Bits(pb, 0x674d0032);
			    MOV_WriteB32Bits(pb, 0xB8D40280);
			    MOV_WriteB32Bits(pb, 0x2dc80100);
			    MOV_WriteB32Bits(pb, 0x0468ee38);
			    MOV_WriteB8Bits(pb, 0x80);
			    */
			ptrDesc = (UINT32 *)ptrack->pH264Desc;
			//DBG_DUMP("ptr=0x%lx\r\n", ptrDesc);
			descLen = Mov_Swap(*ptrDesc);
			//DBG_DUMP("len=0x%lx\r\n", descLen);
			MOV_WriteBuffer(pb, (char *)ptrack->pH264Desc, descLen);

		}
		// *  btrt atom: bit rate, 1: buffer size, 2: avg bit rate, 3: max bit rate
		//MOV_WriteB32Bits(pb, 0x00000014);
		//MOV_WriteB32Bits(pb, 0x62747274);
		//MOV_WriteB32Bits(pb, 0x00038381);
		//MOV_WriteB32Bits(pb, 0x01912d28);
		//MOV_WriteB32Bits(pb, 0x01722de0);//
#endif

	}
#if 0//old version
	else if (ptrack->tag == 0x68766331) { //hvc1
		MOV_WriteB32Tag(pb, "hvc1");
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 1);//data-reference index

		MOV_WriteB16Bits(pb, 0);//version
		MOV_WriteB16Bits(pb, 0);//level
		MOV_WriteB32Bits(pb, 0);//vendor reserved

		MOV_WriteB32Bits(pb, 0x000);//Temporal Quality
		MOV_WriteB32Bits(pb, 0x000);//Spatial Quality

		MOV_WriteB16Bits(pb, ptrack->context->width);
		MOV_WriteB16Bits(pb, ptrack->context->height);
		MOV_WriteB32Bits(pb, 0x00480000);//Horizontal resolution 72dpi
		MOV_WriteB32Bits(pb, 0x00480000);//Vertical resolution 72dpi
		MOV_WriteB32Bits(pb, 0x0);//Data size, must be 0
		MOV_WriteB16Bits(pb, 0x1);//frame count, 1
		MOV_WriteB16Bits(pb, 0);

		MOV_WriteB32Bits(pb, 0x0);// 0x44
		MOV_WriteB32Bits(pb, 0x0);// 0x48
		MOV_WriteB32Bits(pb, 0x0);// 0x4c
		MOV_WriteB32Bits(pb, 0x0);// 0x50
		MOV_WriteB32Bits(pb, 0x0);// 0x54
		MOV_WriteB32Bits(pb, 0x0);// 0x58
		MOV_WriteB32Bits(pb, 0x0);// 0x5c
		MOV_WriteB16Bits(pb, 0);// 0x60
		MOV_WriteB16Bits(pb, 0x18);//depth, 32
		MOV_WriteB16Bits(pb, 0xFFFF);//default color table, (-1)
		MOV_WriteB32Bits(pb, 0x8);//
		MOV_WriteB32Tag(pb, "hvcC");
		//MOV_WriteB8Bits(pb, 0x0);// 0x6E
	}
#else //mac version
	else if (ptrack->tag == 0x68766331) { //hvc1
		UINT32 descLen = ptrack->h265Len;
		MOV_WriteB32Tag(pb, "hvc1");
		MOV_WriteB32Bits(pb, 0);//reserved
		MOV_WriteB32Bits(pb, 1);//data-reference index

		MOV_WriteB16Bits(pb, 0);//version
		MOV_WriteB16Bits(pb, 0);//level
		//0x10
		MOV_WriteB32Bits(pb, 0);//vendor reserved

		MOV_WriteB32Bits(pb, 0x000);//Temporal Quality
		MOV_WriteB32Bits(pb, 0x000);//Spatial Quality

		MOV_WriteB16Bits(pb, ptrack->context->width);
		MOV_WriteB16Bits(pb, ptrack->context->height);
		//0x20
		MOV_WriteB32Bits(pb, 0x00480000);//Horizontal resolution 72dpi
		MOV_WriteB32Bits(pb, 0x00480000);//Vertical resolution 72dpi
		MOV_WriteB32Bits(pb, 0x0);//Data size, must be 0
		MOV_WriteB16Bits(pb, 0x1);//frame count, 1
		MOV_WriteB16Bits(pb, 0);
		//0x30
		MOV_WriteB32Bits(pb, 0x0);// 0x44
		MOV_WriteB32Bits(pb, 0x0);// 0x48
		MOV_WriteB32Bits(pb, 0x0);// 0x4c
		MOV_WriteB32Bits(pb, 0x0);// 0x50
		MOV_WriteB32Bits(pb, 0x0);// 0x54
		MOV_WriteB32Bits(pb, 0x0);// 0x58
		MOV_WriteB32Bits(pb, 0x0);// 0x5c
		MOV_WriteB16Bits(pb, 0);// 0x60
		MOV_WriteB16Bits(pb, 0x18);//depth, 32
		//0x50
		MOV_WriteB16Bits(pb, 0xFFFF);//default color table, (-1)
#if 1  //H265 MULTI-TILE
		MOV_WriteB32Bits(pb, descLen + 0x1F);
#else
		MOV_WriteB32Bits(pb, descLen + 0x22); //
#endif //H265 MULTI-TILE
		MOV_WriteB32Tag(pb, "hvcC");


		{

#if 1  //H265 MULTI-TILE
			MOV_WriteB16Bits(pb, 0x0101);//Data len
#else
			MOV_WriteB16Bits(pb, 0x0121);//Data len
#endif //H265 MULTI-TILE
			MOV_WriteB32Bits(pb, 0x60000000);//sps and pps
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x000096F0);
			MOV_WriteB32Bits(pb, 0x00FCFDF8);
			MOV_WriteB32Bits(pb, 0xF800000F);
#if 1  //H265 MULTI-TILE
			MOV_WriteB8Bits(pb, 0x03);
#else
			MOV_WriteB32Bits(pb, 0x03200001);
#endif //H265 MULTI-TILE
			//ptrDesc = (UINT32 *)ptrack->pH265Desc;
			MOV_WriteBuffer(pb, (char *)ptrack->pH265Desc, descLen);

		}
	}//if (ptrack->tag == 0x68766331) { //hvc1
#endif
	return MOVWriteUpdateData(pb, pos);
}

/*
  Write STSD atom
  size + tag +

  @param pb      buffer pointer
  @param ptrack  track infomation
  @return this atom size
*/
UINT32 MOVWriteSTSDTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);

	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Tag(pb, "stsd");
	MOV_WriteB32Bits(pb, 0);//flag and version
	MOV_WriteB32Bits(pb, 1);//number of entry

	if (ptrack->context->codec_type == REC_AUDIO_TRACK) {
		MOVWriteSoundDescTag(pb, ptrack);
	} else if (ptrack->context->codec_type == REC_VIDEO_TRACK) {
		MOVWriteVideoDescTag(pb, ptrack);
	}
	return MOVWriteUpdateData(pb, pos);
}
static UINT32 MOVWriteSTBLTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Tag(pb, "stbl");
	MOVWriteSTSDTag(pb, ptrack);
	MOVWriteSTTSTag(pb, ptrack);

#if 1
	if (ptrack->context->codec_type == REC_VIDEO_TRACK) { //FAKE_264 // Hideo: why FAKE???
		MOVWriteCTTSTag(pb, ptrack);
	}
#endif

	if (ptrack->context->codec_type == REC_VIDEO_TRACK &&
		ptrack->hasKeyframes < ptrack->entry) {
		MOVWriteSTSSTag(pb, ptrack);
	}
	MOVWriteSTSCTag(pb, ptrack);
	MOVWriteSTSZTag(pb, ptrack);
	MOVWriteSTCOTag(pb, ptrack);
	//MOVWriteSDTPTag(pb, ptrack);    //FAKE_264
	return MOVWriteUpdateData(pb, pos);
}
static UINT32 MOVWriteDREFTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_WriteB32Bits(pb, 0x1C); // size
	MOV_WriteB32Tag(pb, "dref");
	MOV_WriteB32Bits(pb, 0); // version & flags
	MOV_WriteB32Bits(pb, 1); // entry count

	MOV_WriteB32Bits(pb, 0xC); // size
	MOV_WriteB32Tag(pb, "url ");
	MOV_WriteB32Bits(pb, 1); // version & flags

	return 0x1C;
}

static UINT32 MOVWriteDINFTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);

	MOV_WriteB32Bits(pb, 0); // size
	MOV_WriteB32Tag(pb, "dinf");
	MOVWriteDREFTag(pb, ptrack);
	return MOVWriteUpdateData(pb, pos);
}

static UINT32 MOVWriteHDLRTag(char *pb, MOVREC_Track *ptrack)
{
	char *descr, *hdlr, *hdlr_type;
	MOV_Offset  pos = MOV_TellOffset(pb);

	//hdlr = &g_tempChar_h[0]; fix coverity
	//hdlr_type = &g_tempChar_t[0]; fix coverity
	//descr = &g_tempChar_d[0];  fix coverity

	if (!ptrack) { // no media --> data handler
		hdlr = "dhlr";
		hdlr_type = "url ";
		descr = "DataHandler";
	} else {
		hdlr = "mhlr";
		if (ptrack->context->codec_type == REC_VIDEO_TRACK) {
			hdlr = "mhlr";
			hdlr_type = "vide";
			descr = "VideoHandler";
		} else {
			hdlr_type = "soun";
			descr = "SoundHandler";
		}
	}

	MOV_WriteB32Bits(pb, 0); // size
	MOV_WriteB32Tag(pb, "hdlr");
	MOV_WriteB32Bits(pb, 0); // Version & flags
	MOV_WriteBuffer(pb, hdlr, 4); // handler
	MOV_WriteB32Tag(pb, hdlr_type); // handler type
	MOV_WriteB32Bits(pb, 0); // reserved
	MOV_WriteB32Bits(pb, 0); // reserved
	MOV_WriteB32Bits(pb, 0); // reserved
	MOV_WriteB8Bits(pb, strlen(descr)); // string counter
	MOV_WriteBuffer(pb, descr, strlen(descr)); // handler description
	return MOVWriteUpdateData(pb, pos);
}

static UINT32 MOVWriteVMHDTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_WriteB32Bits(pb, 0x14); // size (always 0x14)
	MOV_WriteB32Tag(pb, "vmhd");
	MOV_WriteB32Bits(pb, 0x01); // version & flags
	MOV_WriteB32Bits(pb, 0); // reserved (graphics mode = copy)
	MOV_WriteB32Bits(pb, 0); // reserved (graphics mode = copy)
	return 0x14;
}
static UINT32 MOVWriteSMHDTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_WriteB32Bits(pb, 0x10); // size (always 0x10)
	MOV_WriteB32Tag(pb, "smhd");
	MOV_WriteB32Bits(pb, 0x00); // version & flags
	MOV_WriteB32Bits(pb, 0); // reserved
	return 0x10;
}

static UINT32 MOVWriteMINFTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Tag(pb, "minf");
	if (ptrack->context->codec_type == REC_VIDEO_TRACK) {
		MOVWriteVMHDTag(pb, ptrack);
	} else { //audio
		MOVWriteSMHDTag(pb, ptrack);
	}
	MOVWriteHDLRTag(pb, NULL);
	MOVWriteDINFTag(pb, ptrack);
	MOVWriteSTBLTag(pb, ptrack);
	return MOVWriteUpdateData(pb, pos);
}
static UINT32 MOVWriteMDHDTag(char *pb, MOVREC_Track *ptrack)
{
	UINT32 dur;
	MOV_WriteB32Bits(pb, 0x20);//size
	MOV_WriteB32Tag(pb, "mdhd");
	MOV_WriteB32Bits(pb, 0x0);//flag and version
	MOV_WriteB32Bits(pb, ptrack->ctime); // creation time
	MOV_WriteB32Bits(pb, ptrack->mtime); // modification time

	if (ptrack->context->codec_type == REC_VIDEO_TRACK) {
		MOV_WriteB32Bits(pb, ptrack->timescale); // time scale
		MOV_WriteB32Bits(pb, ptrack->trackDuration); //total duration
		//DBG_DUMP("VID scale=%d, dur=%d!\r\n", ptrack->timescale, ptrack->trackDuration);
	} else { //AUDIO
#if 0//2010/01/11 Meg
		MOV_WriteB32Bits(pb, ptrack->context->nSamplePerSecond); //samplePerSecond
		//#NT#2009/12/21#Meg Lin -begin
		//#NT#new feature: rec stereo
		MOV_WriteB32Bits(pb, (ptrack->audioSize / ptrack->context->nchannel) * 8 / ptrack->context->wBitsPerSample); // total samples, not bytes
		//#NT#2009/12/21#Meg Lin -end
#else
		MOV_WriteB32Bits(pb, ptrack->context->nSamplePerSecond); // time scale (sample rate for audio)
		dur = ptrack->context->nSamplePerSecond * ptrack->trackDuration / ptrack->timescale;
		//DBG_DUMP("AUD trackdur=%d, sc=%d!\r\n", ptrack->trackDuration, ptrack->timescale);
		MOV_WriteB32Bits(pb, dur); // duration
		//DBG_DUMP("AUD scale=%d, dur=%d!\r\n", ptrack->context->nSamplePerSecond, dur);
#endif
	}
	MOV_WriteB16Bits(pb, ptrack->language); // language
	MOV_WriteB16Bits(pb, 0); // reserved (quality)
	return 32;
}
static UINT32 MOVWriteMDIATag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Tag(pb, "mdia");
	MOVWriteMDHDTag(pb, ptrack);
	MOVWriteHDLRTag(pb, ptrack);
	MOVWriteMINFTag(pb, ptrack);
	return MOVWriteUpdateData(pb, pos);
}

static UINT32 MOVWriteTKHDTag(char *pb, MOVREC_Track *ptrack)
{

	MOV_WriteB32Bits(pb, 0x5C);//size
	MOV_WriteB32Tag(pb, "tkhd");
	MOV_WriteB32Bits(pb, 0xf); // flags (track enabled)
	MOV_WriteB32Bits(pb, ptrack->ctime); // creation time
	MOV_WriteB32Bits(pb, ptrack->mtime); // modification time

	MOV_WriteB32Bits(pb, ptrack->trackID); // track-id
	MOV_WriteB32Bits(pb, 0); // reserved
	MOV_WriteB32Bits(pb, ptrack->trackDuration);
	if (ptrack->trackDuration == 0) {
		DBG_DUMP("^R dur = ZERO\r\n");
		return 0;
	}

	MOV_WriteB32Bits(pb, 0); // reserved
	MOV_WriteB32Bits(pb, 0); // reserved
	MOV_WriteB32Bits(pb, 0x0); // reserved (Layer & Alternate group)
	// Volume, only for audio
	if (ptrack->context->codec_type == REC_AUDIO_TRACK) {
		MOV_WriteB16Bits(pb, 0x0100);
	} else {
		MOV_WriteB16Bits(pb, 0);
	}
	MOV_WriteB16Bits(pb, 0); // reserved

	// Matrix structure
	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	switch (ptrack->uiRotateInfo) {
	default:
	case 0: {
			/*
			normal
			[ 1 0 0
			  0 1 0
			  0 0 1]
			*/
			MOV_WriteB32Bits(pb, 0x00010000);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x00010000);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x40000000);
		}
		break;

	case 90: {
			/*
			90度
			[  0  1 0
			  -1  0 0
			   0  0 1]
			*/
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x00010000);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0xFFFF0000);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x40000000);
		}
		break;

	case 180: {
			/*
			180 度
			[-1   0 0
			 0  -1 0
			 0   0 1]
			*/
			MOV_WriteB32Bits(pb, 0xFFFF0000);//2013/07/04 Meg, fixed
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0xFFFF0000);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x40000000);
		}
		break;

	case 270: {
			/*
			270 度
			[ 0 -1 0
			  1  0 0
			  0  0 1]
			*/
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0xFFFF0000);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x00010000);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x0);
			MOV_WriteB32Bits(pb, 0x40000000);
		}
		break;
	}
	//MOV_WriteB32Bits(pb, 0x00010000); // reserved
	//MOV_WriteB32Bits(pb, 0x0); // reserved
	//MOV_WriteB32Bits(pb, 0x0); // reserved
	//MOV_WriteB32Bits(pb, 0x0); // reserved
	//MOV_WriteB32Bits(pb, 0x00010000); // reserved
	//MOV_WriteB32Bits(pb, 0x0); // reserved
	//MOV_WriteB32Bits(pb, 0x0); // reserved
	//MOV_WriteB32Bits(pb, 0x0); // reserved
	//MOV_WriteB32Bits(pb, 0x40000000); // reserved
	//#NT#2013/04/17#Calvin Chang -end

	// Track width and height, for visual only
	if (ptrack->context->codec_type == REC_VIDEO_TRACK) {
		UINT32  uiWidth = ptrack->context->width;
		UINT32  uiHeight = ptrack->context->height;

		if (ptrack->uiDAR == MEDIAREC_DAR_16_9) {
			uiWidth = (uiHeight * 16) / 9;
		}
		MOV_WriteB32Bits(pb, uiWidth * 0x10000);
		MOV_WriteB32Bits(pb, uiHeight * 0x10000);
	} else {
		MOV_WriteB32Bits(pb, 0);
		MOV_WriteB32Bits(pb, 0);
	}
	return 0x5c;
}
static UINT32 MOVWriteEDTSTag(char *pb, MOVREC_Track *ptrack)//edit atom
{
	//MOV_Offset  pos = MOV_TellOffset(pb);

	MOV_WriteB32Bits(pb, 0x24);//size
	MOV_WriteB32Tag(pb, "edts");
	MOV_WriteB32Bits(pb, 0x1C);//size
	MOV_WriteB32Tag(pb, "elst");//edit list atom
	MOV_WriteB32Bits(pb, 0);//flag and version
	MOV_WriteB32Bits(pb, 1);//number of entry
	//DBG_DUMP("dur = %d!\r\n", ptrack->trackDuration);
	MOV_WriteB32Bits(pb, ptrack->trackDuration);//duration
	MOV_WriteB32Bits(pb, 0x0);//
	MOV_WriteB32Bits(pb, 0x00010000);//
	return 0x24;
}
static UINT32 MOVWriteUUIDTagPSP(char *pb, MOVREC_Track *ptrack)//edit atom
{
	//MOV_Offset  pos = MOV_TellOffset(pb);

	MOV_WriteB32Bits(pb, 0x34);//size
	MOV_WriteB32Tag(pb, "uuid");
	MOV_WriteB32Tag(pb, "USMT");
	MOV_WriteB32Bits(pb, 0x21D24FCE);//GUID
	MOV_WriteB32Bits(pb, 0xBB88695C);//
	MOV_WriteB32Bits(pb, 0xFAC9C740);//
	MOV_WriteB32Bits(pb, 0x1C);//flag and version
	MOV_WriteB32Tag(pb, "MTDT");

	MOV_WriteB32Bits(pb, 0x00010012);//
	MOV_WriteB32Bits(pb, 0xa);//
	MOV_WriteB32Bits(pb, 0x55C40000);//
	MOV_WriteB32Bits(pb, 0x1);//
	MOV_WriteB32Bits(pb, 0x0);//
	return 0x34;
}
#if 0
#pragma mark -
#endif

static UINT32 MOVWriteTRAKTag(char *pb, MOVREC_Track *ptrack)
{
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Tag(pb, "trak");
	MOVWriteTKHDTag(pb, ptrack);
	if (ptrack->mode == MOVMODE_PSP) {
		MOVWriteEDTSTag(pb, ptrack);
	}
	MOVWriteMDIATag(pb, ptrack);
	if (ptrack->mode == MOVMODE_PSP) {
		MOVWriteUUIDTagPSP(pb, ptrack);
	}
	return MOVWriteUpdateData(pb, pos);
}
static UINT32 MOVWriteMVHDTag(char *pb, MOVREC_Track *ptrack, UINT32 track_num)
{
	if (ptrack->trackDuration == 0) {
		DBG_DUMP("MVHD:ptrack->trackDuration ZERO!!!");
		return 0;
	}

	MOV_WriteB32Bits(pb, 0x6C);//size
	MOV_WriteB32Tag(pb, "mvhd");
	MOV_WriteB32Bits(pb, 0); // flags
	MOV_WriteB32Bits(pb, ptrack->ctime); // creation time
	MOV_WriteB32Bits(pb, ptrack->mtime); // modification time
	MOV_WriteB32Bits(pb, ptrack->timescale); // time scale (sample rate for audio)
	MOV_WriteB32Bits(pb, ptrack->trackDuration); // duration

	MOV_WriteB32Bits(pb, 0x00010000); // reserved (preferred rate) 1.0 = normal
	MOV_WriteB16Bits(pb, 0x0100); // reserved (preferred volume) 1.0 = normal
	MOV_WriteB16Bits(pb, 0); // reserved
	MOV_WriteB32Bits(pb, 0); // reserved
	MOV_WriteB32Bits(pb, 0); // reserved

	// Matrix structure
	MOV_WriteB32Bits(pb, 0x00010000); // reserved
	MOV_WriteB32Bits(pb, 0x0); // reserved
	MOV_WriteB32Bits(pb, 0x0); // reserved
	MOV_WriteB32Bits(pb, 0x0); // reserved
	MOV_WriteB32Bits(pb, 0x00010000); // reserved
	MOV_WriteB32Bits(pb, 0x0); // reserved
	MOV_WriteB32Bits(pb, 0x0); // reserved
	MOV_WriteB32Bits(pb, 0x0); // reserved
	MOV_WriteB32Bits(pb, 0x40000000); // reserved

	MOV_WriteB32Bits(pb, 0); // reserved (preview time)
	MOV_WriteB32Bits(pb, 0); // reserved (preview duration)
	MOV_WriteB32Bits(pb, 0); // reserved (poster time)
	MOV_WriteB32Bits(pb, 0); // reserved (selection time)
	MOV_WriteB32Bits(pb, 0); // reserved (selection duration)
	MOV_WriteB32Bits(pb, 0); // reserved (current time)
	MOV_WriteB32Bits(pb, track_num + 1); // Next track id
	return 0x6c;
}
static UINT32 MOVWriteUUIDUSMTTag(char *pb, MOVREC_Track *ptrack)//edit atom
{
	UINT32 size = 0, len = 0;
	MOV_Offset  pos = MOV_TellOffset(pb);


	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Tag(pb, "uuid");
	MOV_WriteB32Tag(pb, "USMT");
	MOV_WriteB32Bits(pb, 0x21D24FCE);//GUID
	MOV_WriteB32Bits(pb, 0xBB88695C);//
	MOV_WriteB32Bits(pb, 0xFAC9C740);//
	size += 24;

	MOV_WriteB32Bits(pb, 0);//size place, need write!!
	MOV_WriteB32Tag(pb, "MTDT");
	MOV_WriteB16Bits(pb, 0x4);
	size += 10;

	MOV_WriteB16Bits(pb, 0x0C);
	MOV_WriteB32Bits(pb, 0x0B);
	MOV_WriteB16Bits(pb, 0x55C4);//language id of und
	MOV_WriteB32Bits(pb, 0x21C);
	size += 12;

	len = 8;//MOVMPEG4
	MOV_WriteB16Bits(pb, 0x1A);//8*2 + 10 = 26
	MOV_WriteB32Bits(pb, 0x4);
	MOV_WriteB16Bits(pb, 0x15C7);//language id of eng
	MOV_WriteB16Bits(pb, 0x1);
	MOV_WriteB32Bits(pb, 0x004D004F);//MOVMPEG4
	MOV_WriteB32Bits(pb, 0x0056004D);
	MOV_WriteB32Bits(pb, 0x00500045);
	MOV_WriteB32Bits(pb, 0x00470034);
	size += (len * 2 + 10);

	len = 4;//title TEST
	MOV_WriteB16Bits(pb, 0x12);//4*2 + 10 = 18
	MOV_WriteB32Bits(pb, 0x1);
	MOV_WriteB16Bits(pb, 0x15C7);//language id of eng
	MOV_WriteB16Bits(pb, 0x1);
	MOV_WriteB32Bits(pb, 0x00540045);//TEST
	MOV_WriteB32Bits(pb, 0x00530054);
	size += (len * 2 + 10);

	len = 20;//2006/01/01 11:11:11
	MOV_WriteB16Bits(pb, 0x32);//20*2 + 10 = 50
	MOV_WriteB32Bits(pb, 0x3);
	MOV_WriteB16Bits(pb, 0x55C4);//language id of und
	MOV_WriteB16Bits(pb, 0x1);
	MOV_WriteB32Bits(pb, 0x00320030);//2006/01/01 11:11:11
	MOV_WriteB32Bits(pb, 0x00300036);
	MOV_WriteB32Bits(pb, 0x002F0030);
	MOV_WriteB32Bits(pb, 0x0031002F);
	MOV_WriteB32Bits(pb, 0x00300031);
	MOV_WriteB32Bits(pb, 0x00200031);//11:11:11
	MOV_WriteB32Bits(pb, 0x0031003A);
	MOV_WriteB32Bits(pb, 0x00310031);
	MOV_WriteB32Bits(pb, 0x003A0031);
	MOV_WriteB32Bits(pb, 0x00310000);
	size += (len * 2 + 10);
	MOV_SeekOffset(pb, pos, KFS_SEEK_SET);
	MOV_WriteB32Bits(pb, size);
	MOV_SeekOffset(pb, pos + 24, KFS_SEEK_SET);
	MOV_WriteB32Bits(pb, size - 24);
	MOV_SeekOffset(pb, pos + size, KFS_SEEK_SET);
	return size;
}
static UINT32 MOVWriteUDTATag(char *pb, MOVREC_Track *ptrack)//edit atom
{
	//#NT#2012/08/22#Hideo Lin -begin
	//#NT#For movie thumbnail
#if 1
	//#NT#2012/09/05#Hideo Lin -begin
	//#NT#The data in 'udta' must be atom format
	UINT32  uiSize;

	if (ptrack->udta_size == 0) { // size = 0 means doesn't need the udta atom
		return 0;
	}

	uiSize = 8; // 'udta' atom size (4 bytes) + ID (4 bytes)

	if (ptrack->bCustomUdata) { // customized user data, don't care the contain (user need to take care by themselves)
		uiSize += ptrack->udta_size;

		MOV_WriteB32Bits(pb, uiSize);
		MOV_WriteB32Tag(pb, "udta");
		MOV_WriteBytes(pb, (char *)ptrack->udta_addr, ptrack->udta_size); // data
	} else { // default user data format, should follow the atom rule
		uiSize += (ptrack->udta_size + 8); // 8 bytes are 'user' atom size + ID (our definition)

		MOV_WriteB32Bits(pb, uiSize);
		MOV_WriteB32Tag(pb, "udta");

		MOV_WriteB32Bits(pb, ptrack->udta_size + 8); // size (8 bytes are 'user' atom size + ID)
		MOV_WriteB32Bits(pb, ATOM_USER); // 'user' ID
		MOV_WriteBytes(pb, (char *)ptrack->udta_addr, ptrack->udta_size); // data
	}

	return uiSize;
	//#NT#2012/09/05#Hideo Lin -end
#else
	UINT32  uiSize;

	if (ptrack->udta_size == 0 && ptrack->uiThumbSize == 0) {
		return 0;
	}

	uiSize = 8; // 'udta' atom ID (4 bytes) + size (4 bytes)

	// user data size
	if (ptrack->udta_size) {
		uiSize += (ptrack->udta_size + 8); // 8 bytes are 'user' atom ID + size (our definition)
	}

	// thumbnail data size
	if (ptrack->uiThumbSize) {
		uiSize += (ptrack->uiThumbSize + 8); // 8 bytes are 'thum' atom ID + size (our definition)
	}

	MOV_WriteB32Bits(pb, uiSize); // size
	MOV_WriteB32Tag(pb, "udta"); // 'udta' ID

	// user data
	if (ptrack->udta_size) {
		MOV_WriteL32Bits(pb, ATOM_USER); // 'user' ID
		MOV_WriteL32Bits(pb, ptrack->udta_size); // size
		MOV_WriteBytes(pb, (char *)ptrack->udta_addr, ptrack->udta_size); // data
	}

	// thumbnail data
	if (ptrack->uiThumbSize) {
		MOV_WriteL32Bits(pb, ATOM_THUM); // 'thum' ID
		MOV_WriteL32Bits(pb, ptrack->uiThumbSize); // size
		MOV_WriteBytes(pb, (char *)ptrack->uiThumbAddr, ptrack->uiThumbSize); // data
	}

	return uiSize;
#endif
	//#NT#2012/08/22#Hideo Lin -end
}

static UINT32 MOVWriteGPSTag(char *pb, MOVREC_Track *ptrack)//edit atom
{
	UINT32  i, fileid, total;
	MOV_Ientry      *pEntry = 0;
	MOV_Offset  pos = MOV_TellOffset(pb);

	if (ptrack->gpstag_addr == 0) { // size = 0 means doesn't need the udta atom
		DBG_ERR("WritrGPSTag fail, addr=zero\r\n");
		return 0;
	}
	fileid = ptrack->fileid;
	total = g_movGPSEntryTotal[fileid];
	MOV_WriteB32Bits(pb, 0);//size

	MOV_WriteB32Tag(pb, "gps ");
	MOV_WriteB32Bits(pb, MOV_GPSENTRY_VERSION);//version and flags
	MOV_WriteB32Bits(pb, total);//total
	//DBG_DUMP("WritrGPSTag OK, total=%d\r\n", total);

	pEntry = (MOV_Ientry *)g_movGPSEntryAddr[fileid];
	for (i = 0; i < total; i++) {
		MOV_WriteB32Bits(pb, pEntry->pos); //pos
		MOV_WriteB32Bits(pb, pEntry->size); //size
		pEntry++;
	}
	return MOVWriteUpdateData(pb, pos);

}

#if 1	//add for meta -begin
static UINT32 MOVWriteMetaTag(char *pb, MOVREC_Track *ptrack, UINT32 metaidx)//edit atom
{
	UINT32  i, metaid, total;
	MOV_Ientry      *pEntry = 0;
	MOV_Offset  pos = MOV_TellOffset(pb);
	UINT32 metasign;
	MOV_Offset  moovSize = 0;

	if (ptrack->metatag_addr == 0) {
		return 0;
	}

	metaid = ptrack->metaid;

	{
		DBG_IND("[%d][%d]g_mov_meta_entry_total=%d\r\n", metaid, metaidx, g_mov_meta_entry_total[metaid][metaidx]);
		total = g_mov_meta_entry_total[metaid][metaidx];
		MOV_WriteB32Bits(pb, 0);//size

		metasign = g_mov_meta_entry_sign[metaid][metaidx];
		MOV_WriteL32Bits(pb, metasign);
		//MOV_WriteB32Tag(pb, "meta");
		MOV_WriteB32Bits(pb, MOV_GPSENTRY_VERSION);//version and flags
		MOV_WriteB32Bits(pb, total);//total

		pEntry = (MOV_Ientry *)g_mov_meta_entry_addr[metaid][metaidx];
		for (i = 0; i < total; i++) {
			MOV_WriteB32Bits(pb, pEntry->pos); //pos
			MOV_WriteB32Bits(pb, pEntry->size); //size
			MOV_WriteB32Bits(pb, pEntry->key_frame); //video index
			MOV_WriteB32Bits(pb, 0); //reserved
			pEntry++;
		}
		moovSize += MOVWriteUpdateData(pb, pos);

		g_mov_meta_entry_total[metaid][metaidx] = 0;
	}

	return moovSize;

}
#endif	//add for meta -end

UINT32 MOVWriteCUSTOMDataTag(char *pb, MOVREC_Track *ptrack)//edit atom
{
	UINT32  uiSize;

	if (ptrack->cudta_size == 0) { // size = 0 means doesn't need the udta atom
		return 0;
	}

	uiSize = 8; // 'abcd' atom size (4 bytes) + ID (4 bytes)

	if (ptrack->cudta_addr) { // customized data, don't care the contain (user need to take care by themselves)
		uiSize += ptrack->cudta_size;
		MOV_WriteB32Bits(pb, uiSize);
		MOV_WriteB32Bits(pb, ptrack->cudta_tag);
		MOV_WriteBytes(pb, (char *)ptrack->cudta_addr, ptrack->cudta_size); // data
	}

	return uiSize;
}
#if 0
//#NT#2010/01/29#Meg Lin# -begin
static UINT32 MOVWriteFRE1Tag(char *pb, MOVREC_Track *ptrack)//edit atom
{
	UINT32 i;
	UINT8 *ptr;

	MOV_WriteB32Bits(pb, g_movTrack.fre1_size + 8); //size
	MOV_WriteB32Tag(pb, "fre1");
	ptr = (UINT8 *)g_movTrack.fre1_addr;
	for (i = 0; i < g_movTrack.fre1_size; i++) {
		MOV_WriteB8Bits(pb, *(ptr + i));
	}
	return g_movTrack.fre1_size + 8;
}
//#NT#2010/01/29#Meg Lin# -end
#endif

UINT32 MOVWriteMOOVTag(char *pb, MOVREC_Track *ptrack, MOVREC_Track *pAudtrack, UINT8 trackIndex)
{
	MOV_Offset  moovSize;
	MOV_Offset  pos = MOV_TellOffset(pb);
	UINT32   rev;
	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Tag(pb, "moov");

	if (ptrack == 0) { //coverity 43933
		return 0;
	}
	rev = MOVWriteMVHDTag(pb, ptrack, 2);
	if (rev == 0) {
		return 0;
	}

	//for (i=0; i<mov->nb_streams; i++) {
	//    if(mov->tracks[i].entry > 0) {
	//        mov_write_trak_tag(pb, &(mov->tracks[i]));
	//     }
	//}
	if ((trackIndex & TRACK_INDEX_VIDEO) && (ptrack)) {
		MOVWriteTRAKTag(pb, ptrack);
	}
	if ((trackIndex & TRACK_INDEX_AUDIO) && (pAudtrack)) {
		MOVWriteTRAKTag(pb, pAudtrack);
	}
	if ((ptrack) && (ptrack->mode == MOVMODE_PSP)) { //coverity 43143
		MOVWriteUUIDUSMTTag(pb, ptrack);
	}
	moovSize = MOVWriteUpdateData(pb, pos);

	//#NT#2012/08/22#Hideo Lin -begin
	//#NT#For movie thumbnail
#if 1
	if (ptrack->udta_size) {
		MOVWriteUDTATag(pb, ptrack);
		moovSize = MOVWriteUpdateData(pb, pos);//2012/08/23 Meg, fixbug: udta should in moov
	}

	if (ptrack->gpstag_size) {
		MOVWriteGPSTag(pb, ptrack);
		moovSize = MOVWriteUpdateData(pb, pos);//gps tag should in moov
	}

#if 1	//add for meta -begin
	if (ptrack->metatag_size) {
		UINT32 metaidx, metaum = g_mov_meta_num[ptrack->metaid];
		for (metaidx = 0; metaidx < metaum; metaidx++) {
			MOVWriteMetaTag(pb, ptrack, metaidx);
			moovSize = MOVWriteUpdateData(pb, pos);//meta tag should in moov
		}
	}
#endif	//add for meta -end

	if (g_mov_moovlayer1size) {
		MOV_WriteBuffer(pb, (char *)g_mov_moovlayer1adr, g_mov_moovlayer1size); // handler
		moovSize = MOVWriteUpdateData(pb, pos);
		DBGD(g_mov_moovlayer1size);
		DBGD(moovSize);
	}

#else
	if (ptrack->udta_size || ptrack->uiThumbSize) {
		UINT32 uiSize;

		uiSize = MOVWriteUDTATag(pb, ptrack);
		moovSize = MOVWriteUpdateData(pb, pos);//2012/08/23 Meg, fixbug: udta should in moov
	}
#endif
	//#NT#2012/08/22#Hideo Lin -end

	return moovSize;
}

//add for sub-stream -begin
UINT32 MOV_Sub_WriteMOOVTag(char *pb, MOVREC_Track *ptrack, MOVREC_Track *pSubtrack, MOVREC_Track *pAudtrack, UINT8 trackIndex)
{
	MOV_Offset  moovSize;
	MOV_Offset  pos = MOV_TellOffset(pb);
	UINT32   rev;
	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Tag(pb, "moov");

	if (ptrack == 0) { //coverity 43933
		return 0;
	}
	rev = MOVWriteMVHDTag(pb, ptrack, 3);
	if (rev == 0) {
		rev = MOVWriteMVHDTag(pb, pSubtrack, 3);
		if (rev == 0) {
			return 0;
		}
	}

	//for (i=0; i<mov->nb_streams; i++) {
	//    if(mov->tracks[i].entry > 0) {
	//        mov_write_trak_tag(pb, &(mov->tracks[i]));
	//     }
	//}
	// trackID = 1
	if ((trackIndex & TRACK_INDEX_VIDEO) && (ptrack)) {
		if (ptrack->trackDuration != 0) {
			MOVWriteTRAKTag(pb, ptrack);
		}
	}
	// trackID = 2
	if ((trackIndex & TRACK_INDEX_VIDEO) && (pSubtrack)) {
		if (pSubtrack->trackDuration != 0) {
			MOVWriteTRAKTag(pb, pSubtrack);
		}
	}
	// trackID = 3
	if ((trackIndex & TRACK_INDEX_AUDIO) && (pAudtrack)) {
		MOVWriteTRAKTag(pb, pAudtrack);
	}
	if ((ptrack) && (ptrack->mode == MOVMODE_PSP)) { //coverity 43143
		MOVWriteUUIDUSMTTag(pb, ptrack);
	}
	moovSize = MOVWriteUpdateData(pb, pos);

	//#NT#2012/08/22#Hideo Lin -begin
	//#NT#For movie thumbnail
#if 1
	if (ptrack->udta_size) {
		MOVWriteUDTATag(pb, ptrack);
		moovSize = MOVWriteUpdateData(pb, pos);//2012/08/23 Meg, fixbug: udta should in moov
	}

	if (ptrack->gpstag_size) {
		MOVWriteGPSTag(pb, ptrack);
		moovSize = MOVWriteUpdateData(pb, pos);//gps tag should in moov
	}

#if 1	//add for meta -begin
	if (ptrack->metatag_size) {
		UINT32 metaidx, metaum = g_mov_meta_num[ptrack->metaid];
		for (metaidx = 0; metaidx < metaum; metaidx++) {
			MOVWriteMetaTag(pb, ptrack, metaidx);
			moovSize = MOVWriteUpdateData(pb, pos);//meta tag should in moov
		}
	}
#endif	//add for meta -end

	if (g_mov_moovlayer1size) {
		MOV_WriteBuffer(pb, (char *)g_mov_moovlayer1adr, g_mov_moovlayer1size); // handler
		moovSize = MOVWriteUpdateData(pb, pos);
		DBGD(g_mov_moovlayer1size);
		DBGD(moovSize);
	}

#else
	if (ptrack->udta_size || ptrack->uiThumbSize) {
		UINT32 uiSize;

		uiSize = MOVWriteUDTATag(pb, ptrack);
		moovSize = MOVWriteUpdateData(pb, pos);//2012/08/23 Meg, fixbug: udta should in moov
	}
#endif
	//#NT#2012/08/22#Hideo Lin -end

	return moovSize;
}
//add for sub-stream -end

#if 0
static UINT32 MOVWriteMP4Header(char *pb, MOV_FILE_INFO *pHeader)
{
	//MOV_Offset  pos = MOV_TellOffset(pb);
	//write ftyp, len = 0x14
	MOV_WriteB32Bits(pb, 0x1C);//size
	MOV_WriteB32Tag(pb, "ftyp");
	MOV_WriteB32Tag(pb, "MSNV");//mp4 for PSP

	MOV_WriteB32Bits(pb, 0x200);//version
	MOV_WriteB32Tag(pb, "MSNV");//mp4 for PSP
	MOV_WriteB32Tag(pb, "isom");//supporting type
	MOV_WriteB32Tag(pb, "mp42");//supporting type

	//write uuid, len = 0x94
	MOV_WriteB32Bits(pb, 0x94);//size
	MOV_WriteB32Tag(pb, "uuid");

	MOV_WriteB32Tag(pb, "PROF");
	//96-bit UUID
	MOV_WriteB32Bits(pb, 0x21D24FCE);
	MOV_WriteB32Bits(pb, 0xBB88695C);
	MOV_WriteB32Bits(pb, 0xFAC9C740);

	MOV_WriteB32Bits(pb, 0);
	MOV_WriteB32Bits(pb, 3);//sections

	//section 1
	MOV_WriteB32Bits(pb, 0x14);//size
	MOV_WriteB32Tag(pb, "FPRF");
	MOV_WriteB32Bits(pb, 0);
	MOV_WriteB32Bits(pb, 0);
	MOV_WriteB32Bits(pb, 0);

	//section 2
	MOV_WriteB32Bits(pb, 0x2C);//size
	MOV_WriteB32Tag(pb, "APRF");
	MOV_WriteB32Bits(pb, 0);
	MOV_WriteB32Bits(pb, 2);//trackID
	MOV_WriteB32Tag(pb, "mp4a");
	MOV_WriteB32Bits(pb, 0x20F);
	MOV_WriteB32Bits(pb, 0);
	MOV_WriteB32Bits(pb, (pHeader->AudioSampleRate) * 2 / 1000); //audio kbps
	MOV_WriteB32Bits(pb, (pHeader->AudioSampleRate) * 2 / 1000); //audio kbps
	MOV_WriteB32Bits(pb, pHeader->AudioSampleRate);//audio sampleRate
	MOV_WriteB32Bits(pb, pHeader->AudioChannel);//audio sampleRate

	//section 3
	MOV_WriteB32Bits(pb, 0x34);//size
	MOV_WriteB32Tag(pb, "VPRF");
	MOV_WriteB32Bits(pb, 0);
	MOV_WriteB32Bits(pb, 1);//trackID
	MOV_WriteB32Tag(pb, "mp4v");
	MOV_WriteB32Bits(pb, 0x103);
	MOV_WriteB32Bits(pb, 0);
	MOV_WriteB32Bits(pb, 0xABE);//video kbps from 613_mp4, 2750000
	MOV_WriteB32Bits(pb, 0xABE);//video kbps from 613_mp4
	MOV_WriteB32Bits(pb, 0x1DF853);//from 613_mp4, 1964115
	MOV_WriteB32Bits(pb, 0x1DF853);//from 613_mp4, 1964115
	MOV_WriteB16Bits(pb, pHeader->ImageWidth);
	MOV_WriteB16Bits(pb, pHeader->ImageHeight);
	MOV_WriteB16Bits(pb, 0x01);
	MOV_WriteB16Bits(pb, 0x01);

	return 0xB0;//0x1C+0x94 =  0xB0
}
#endif


// nidx data
// len(4) + free (4) +  len (4)+nidx (4)
// VT start(4) + VT size(4) + v1 pos (4) + v1 size (4)
// ...
// AT start(4) + AT size (4) + lastJunkPos
// free len = cluster size(g_aviRecFileObj.clusterSize)
// nidx len = 0x18 + vfn*8
static void MOVWrite_MakeNidx(MEDIAREC_NIDXINFO *pNidxInfo)
{
	UINT32 *pBuf;
	UINT32 nidxlen;
	char *pb;

	nidxlen = MP_MOV_NIDX_FIXEDSIZE + pNidxInfo->thisVideoFN * MP_MOV_SIZE_ONEENTRY; //2012/09/11 Meg Lin
	if (pNidxInfo->h264descLen) {
		nidxlen += (4 + pNidxInfo->h264descLen); //len + data
	}
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr);
	MOV_MakeAtomInit((void *)pBuf);
	pb = NULL;//given null pointer is ok
	MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, pNidxInfo->clusterSize);//size for free
	MOV_WriteB32Tag(pb, "free");
	MOV_WriteB32Bits(pb, nidxlen);//size for nidx
	MOV_WriteB32Tag(pb, "nidx");
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioStartPos); //audPos,       pos 0x10
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioSize);     //audSize,      pos 0x14
	MOV_WriteL32Bits(pb, pNidxInfo->lastNidxdataPos);   //lastjunkPos,  pos 0x18
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoStartFN);  //vidstartFN,   pos 0x1c
	MOV_WriteL32Bits(pb, pNidxInfo->videoWid);          //video width,  pos 0x20
	MOV_WriteL32Bits(pb, pNidxInfo->videoHei);          //video height, pos 0x24
	MOV_WriteL32Bits(pb, pNidxInfo->videoCodec);        //videocodec,   pos 0x28
	MOV_WriteL32Bits(pb, pNidxInfo->audioSampleRate);   //audio SR,     pos 0x2c
	MOV_WriteL32Bits(pb, pNidxInfo->audioChannel);      //audiochannel, pos 0x30
	MOV_WriteL32Bits(pb, pNidxInfo->headerSize);        //headersize,   pos 0x34

	if (pNidxInfo->versionCode == MEDIARECVY_VERSION) { //2012/09/11 Meg Lin
		MOV_WriteL32Bits(pb, MEDIARECVY_VERSION);                        //version,      pos 0x38
	}
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoFrmDataLen);//thisVidLen,  pos 0x3c

	//newAddr = MOV_TellTotalAddr(pb);
	//memcpy((UINT32 *)newAddr, (UINT32 *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);

	MOV_WriteL32Bits(pb, pNidxInfo->h264descLen);//h264 desc Len,  pos 0x38+len
	if (pNidxInfo->h264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->h264descData, pNidxInfo->h264descLen);
	}

	//#NT#2012/09/12#Meg Lin -begin
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr + pNidxInfo->clusterSize - 8);
	MOV_MakeAtomInit((void *)pBuf);
	MOV_WriteL32Bits(pb, pNidxInfo->clusterSize);//cluster size     end -8
	MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //cluster id       end -4
	//#NT#2012/09/12#Meg Lin -end

#if 0
	*pBuf++ = FOURCC_JUNK;
	*pBuf++ = g_aviRecFileObj.clusterSize - 8;
	*pBuf++ = FOURCC_NIDX;
	*pBuf++ = nidxlen;
	*pBuf++ = pNidxInfo->thisVideoStartPos;
	*pBuf++ = pNidxInfo->thisVideoSize;
	memcpy((UINT32 *)pBuf, (UINT32 *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);
	pBuf += (pNidxInfo->thisVideoFrmDataLen / 4);
	*pBuf++ = pNidxInfo->thisAudioStartPos;
	*pBuf++ = pNidxInfo->thisAudioSize;
	*pBuf++ = pNidxInfo->lastNidxdataPos;
#endif
}

// nidx data
// len(4) + free (4) +  len (4)+nidx (4)
// VT start(4) + VT size(4) + v1 pos (4) + v1 size (4)
// ...
// AT start(4) + AT size (4) + lastJunkPos
// free len = cluster size(g_aviRecFileObj.clusterSize)
// nidx len = 0x18 + vfn*8
static void MOVWrite_MakeSMNidx(SM_REC_NIDXINFO *pNidxInfo)
{
	UINT32 *pBuf;
	UINT32 nidxlen;
	char *pb;

	nidxlen = MP_MOV_NIDX_FIXEDSIZE + pNidxInfo->thisVideoFN * MP_MOV_SIZE_ONEENTRY; //2012/09/11 Meg Lin
	nidxlen += pNidxInfo->thisAudioFN * MP_MOV_SIZE_ONEENTRY; //2012/09/11 Meg Lin
	if (pNidxInfo->h264descLen) {
		nidxlen += (4 + pNidxInfo->h264descLen); //len + data
	}
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr);
	MOV_MakeAtomInit((void *)pBuf);
	pb = NULL;//given null pointer is ok
	MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, pNidxInfo->clusterSize);//size for free
	MOV_WriteB32Tag(pb, "free");
	MOV_WriteB32Bits(pb, nidxlen);//size for nidx
	MOV_WriteB32Tag(pb, "nidx");
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioStartFN);  //aud this start, pos 0x10
	MOV_WriteL32Bits(pb, pNidxInfo->audioCodec);        //aud codec,      pos 0x14
	MOV_WriteL32Bits(pb, pNidxInfo->lastNidxdataPos);   //lastjunkPos,  pos 0x18
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoStartFN);  //vidstartFN,   pos 0x1c
	MOV_WriteL32Bits(pb, pNidxInfo->videoWid);          //video width,  pos 0x20
	MOV_WriteL32Bits(pb, pNidxInfo->videoHei);          //video height, pos 0x24
	MOV_WriteL32Bits(pb, pNidxInfo->videoCodec);        //videocodec,   pos 0x28
	MOV_WriteL32Bits(pb, pNidxInfo->videoVfr);          //vidframerate  pos 0x2c
	MOV_WriteL32Bits(pb, pNidxInfo->audioSampleRate);   //audio SR,     pos 0x30
	MOV_WriteL32Bits(pb, pNidxInfo->audioChannel);      //audiochannel, pos 0x34
	MOV_WriteL32Bits(pb, pNidxInfo->headerSize);        //headersize,   pos 0x38

	//if (pNidxInfo->versionCode == MEDIARECVY_VERSION)//2012/09/11 Meg Lin
	{
		MOV_WriteL32Bits(pb, SMEDIARECVY_VERSION);      //version,      pos 0x3c
	}
	//write video
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoFN);//thisVidLen,  pos 0x40
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);

	//write audio
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioFN);
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisAudioFrameData, pNidxInfo->thisAudioFrmDataLen);

	MOV_WriteL32Bits(pb, pNidxInfo->h264descLen);//h264 desc Len,  pos 0x40+len
	if (pNidxInfo->h264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->h264descData, pNidxInfo->h264descLen);
	}

	//#NT#2012/09/12#Meg Lin -begin
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr + pNidxInfo->clusterSize - 8);
	MOV_MakeAtomInit((void *)pBuf);
	MOV_WriteL32Bits(pb, pNidxInfo->clusterSize);//cluster size     end -8
	MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //cluster id       end -4
	//#NT#2012/09/12#Meg Lin -end

#if 0
	*pBuf++ = FOURCC_JUNK;
	*pBuf++ = g_aviRecFileObj.clusterSize - 8;
	*pBuf++ = FOURCC_NIDX;
	*pBuf++ = nidxlen;
	*pBuf++ = pNidxInfo->thisVideoStartPos;
	*pBuf++ = pNidxInfo->thisVideoSize;
	memcpy((UINT32 *)pBuf, (UINT32 *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);
	pBuf += (pNidxInfo->thisVideoFrmDataLen / 4);
	*pBuf++ = pNidxInfo->thisAudioStartPos;
	*pBuf++ = pNidxInfo->thisAudioSize;
	*pBuf++ = pNidxInfo->lastNidxdataPos;
#endif
}

static void MOVWrite_MakeSMNidx_co64(SM_REC_CO64_NIDXINFO *pNidxInfo)
{
	UINT32 *pBuf;
	UINT32 nidxlen;
	char *pb;

	nidxlen = MP_MOV_NIDX_FIXEDSIZE + pNidxInfo->thisVideoFN * MP_MOV_SIZE_ONEENTRY; //2012/09/11 Meg Lin
	nidxlen += pNidxInfo->thisAudioFN * MP_MOV_SIZE_ONEENTRY; //2012/09/11 Meg Lin
	if (pNidxInfo->h264descLen) {
		nidxlen += (4 + pNidxInfo->h264descLen); //len + data
	}
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr);
	MOV_MakeAtomInit((void *)pBuf);
	pb = NULL;//given null pointer is ok
	MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, pNidxInfo->clusterSize);//size for free
	MOV_WriteB32Tag(pb, "free");
	MOV_WriteB32Bits(pb, nidxlen);//size for nidx
	MOV_WriteB32Tag(pb, "nidx");
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioStartFN);  //aud this start, pos 0x10
	//DBG_DUMP("^Y thisaFN = %d\r\n", pNidxInfo->thisAudioStartFN);
	MOV_WriteL32Bits(pb, pNidxInfo->audioCodec);        //aud codec,      pos 0x14
	MOV_WriteL64Bits(pb, pNidxInfo->lastNidxdataPos64);   //lastjunkPos,  pos 0x18
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoStartFN);  //vidstartFN,   pos 0x1c
	MOV_WriteL32Bits(pb, pNidxInfo->videoWid);          //video width,  pos 0x20
	MOV_WriteL32Bits(pb, pNidxInfo->videoHei);          //video height, pos 0x24
	//DBG_DUMP("^Y wid = %d hei = %d\r\n", pNidxInfo->videoWid, pNidxInfo->videoHei);
	MOV_WriteL32Bits(pb, pNidxInfo->videoCodec);        //videocodec,   pos 0x28
	MOV_WriteL32Bits(pb, pNidxInfo->videoVfr);          //vidframerate  pos 0x2c
	MOV_WriteL32Bits(pb, pNidxInfo->audioSampleRate);   //audio SR,     pos 0x30
	MOV_WriteL32Bits(pb, pNidxInfo->audioChannel);      //audiochannel, pos 0x34
	MOV_WriteL32Bits(pb, pNidxInfo->headerSize);        //headersize,   pos 0x38

	//if (pNidxInfo->versionCode == MEDIARECVY_VERSION)//2012/09/11 Meg Lin
	{
		MOV_WriteL32Bits(pb, SMEDIARECVY_CO64_VERSION);      //version,      pos 0x3c
	}
	//write video
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoFN);//thisVidLen,  pos 0x40
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);

	//write audio
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioFN);
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisAudioFrameData, pNidxInfo->thisAudioFrmDataLen);

	MOV_WriteL32Bits(pb, pNidxInfo->h264descLen);//h264 desc Len,  pos 0x40+len
	if (pNidxInfo->h264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->h264descData, pNidxInfo->h264descLen);
	}

	//#NT#2012/09/12#Meg Lin -begin
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr + pNidxInfo->clusterSize - 8);
	MOV_MakeAtomInit((void *)pBuf);
	MOV_WriteL32Bits(pb, pNidxInfo->clusterSize);//cluster size     end -8
	MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //cluster id       end -4
	//#NT#2012/09/12#Meg Lin -end

#if 0
	*pBuf++ = FOURCC_JUNK;
	*pBuf++ = g_aviRecFileObj.clusterSize - 8;
	*pBuf++ = FOURCC_NIDX;
	*pBuf++ = nidxlen;
	*pBuf++ = pNidxInfo->thisVideoStartPos;
	*pBuf++ = pNidxInfo->thisVideoSize;
	memcpy((UINT32 *)pBuf, (UINT32 *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);
	pBuf += (pNidxInfo->thisVideoFrmDataLen / 4);
	*pBuf++ = pNidxInfo->thisAudioStartPos;
	*pBuf++ = pNidxInfo->thisAudioSize;
	*pBuf++ = pNidxInfo->lastNidxdataPos;
#endif
}
// nidx2015 data
// len(4) + free (4) +  len (4)+nidx (4)
// VT start(4) + VT size(4) + v1 pos (4) + v1 size (4)
// ...
// AT start(4) + AT size (4) + lastJunkPos
// free len = cluster size(g_aviRecFileObj.clusterSize)
// nidx len = 0x18 + vfn*8
static void MOVWrite_Make2015Nidx(SM_REC_NIDXINFO *pNidxInfo, UINT32 id)
{
	UINT32 *pBuf;
	UINT32 nidxlen, oneentry = MP_MOV_SIZE_ONEENTRY;
	char *pb;
	nidxlen = MP_MOV_NIDX_FIXEDSIZE + pNidxInfo->thisVideoFN * oneentry; //2012/09/11 Meg Lin
	nidxlen += pNidxInfo->thisAudioFN * oneentry; //2012/09/11 Meg Lin
	if (pNidxInfo->h264descLen) {
		nidxlen += (4 + pNidxInfo->h264descLen); //len + data
	}
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr);
	MOV_MakeAtomInit((void *)pBuf);
	pb = NULL;//given null pointer is ok
	MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, pNidxInfo->clusterSize);//size for free
	MOV_WriteB32Tag(pb, "free");
	MOV_WriteB32Bits(pb, nidxlen);//size for nidx
	MOV_WriteB32Tag(pb, "nidx");
	MOV_WriteL32Bits(pb, g_mov_nidx_timeid[id]);
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioStartFN);  //aud this start, pos 0x14
	MOV_WriteL32Bits(pb, pNidxInfo->audioCodec);        //aud codec,      pos 0x18
	MOV_WriteL32Bits(pb, pNidxInfo->lastNidxdataPos);   //lastjunkPos,  pos 0x1c
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoStartFN);  //vidstartFN,   pos 0x20
	MOV_WriteL32Bits(pb, pNidxInfo->videoWid);          //video width,  pos 0x24
	MOV_WriteL32Bits(pb, pNidxInfo->videoHei);          //video height, pos 0x28
	MOV_WriteL32Bits(pb, pNidxInfo->videoCodec);        //videocodec,   pos 0x2c
	MOV_WriteL32Bits(pb, pNidxInfo->videoVfr);          //vidframerate  pos 0x30
	MOV_WriteL32Bits(pb, pNidxInfo->audioSampleRate);   //audio SR,     pos 0x34
	MOV_WriteL32Bits(pb, pNidxInfo->audioChannel);      //audiochannel, pos 0x38
	MOV_WriteL32Bits(pb, pNidxInfo->headerSize);        //headersize,   pos 0x3c

	//if (pNidxInfo->versionCode == MEDIARECVY_VERSION)//2012/09/11 Meg Lin
	{
		MOV_WriteL32Bits(pb, SM2015_RCVY_VERSION);      //version,      pos 0x40
	}
	//write video
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoFN);//thisVidLen,  pos 0x44
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);

	//write audio
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioFN);
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisAudioFrameData, pNidxInfo->thisAudioFrmDataLen);

	MOV_WriteL32Bits(pb, pNidxInfo->h264descLen);//h264 desc Len,  pos 0x44+len
	if (pNidxInfo->h264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->h264descData, pNidxInfo->h264descLen);
	}

	//#NT#2012/09/12#Meg Lin -begin
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr + pNidxInfo->clusterSize - 8);
	MOV_MakeAtomInit((void *)pBuf);
	MOV_WriteL32Bits(pb, pNidxInfo->clusterSize);//cluster size     end -8
	MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //cluster id       end -4
	//#NT#2012/09/12#Meg Lin -end

#if 0
	*pBuf++ = FOURCC_JUNK;
	*pBuf++ = g_aviRecFileObj.clusterSize - 8;
	*pBuf++ = FOURCC_NIDX;
	*pBuf++ = nidxlen;
	*pBuf++ = pNidxInfo->thisVideoStartPos;
	*pBuf++ = pNidxInfo->thisVideoSize;
	memcpy((UINT32 *)pBuf, (UINT32 *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);
	pBuf += (pNidxInfo->thisVideoFrmDataLen / 4);
	*pBuf++ = pNidxInfo->thisAudioStartPos;
	*pBuf++ = pNidxInfo->thisAudioSize;
	*pBuf++ = pNidxInfo->lastNidxdataPos;
#endif
}

static void MOVWrite_Make2015Nidx_co64(SM_REC_CO64_NIDXINFO *pNidxInfo, UINT32 id)
{
	UINT32 *pBuf;
	UINT32 nidxlen, oneentry = MP_MOV_SIZE_CO64_ONEENTRY;
	char *pb;

	nidxlen = MP_MOV_NIDX_FIXEDSIZE + pNidxInfo->thisVideoFN * oneentry; //2012/09/11 Meg Lin
	nidxlen += pNidxInfo->thisAudioFN * oneentry; //2012/09/11 Meg Lin
	if (pNidxInfo->h264descLen) {
		nidxlen += (4 + pNidxInfo->h264descLen); //len + data
	}
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr);
	MOV_MakeAtomInit((void *)pBuf);
	pb = NULL;//given null pointer is ok
	MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, pNidxInfo->clusterSize);//size for free
	MOV_WriteB32Tag(pb, "free");
	MOV_WriteB32Bits(pb, nidxlen);//size for nidx
	MOV_WriteB32Tag(pb, "nidx");
	MOV_WriteL32Bits(pb, g_mov_nidx_timeid[id]);
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioStartFN);  //aud this start, pos 0x14
	MOV_WriteL32Bits(pb, pNidxInfo->audioCodec);        //aud codec,      pos 0x18
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoStartFN);  //vidstartFN,   pos 0x1c
	MOV_WriteL64Bits(pb, pNidxInfo->lastNidxdataPos64); //lastjunkPos,  pos 0x20
	MOV_WriteL32Bits(pb, pNidxInfo->videoWid);          //video width,  pos 0x28
	MOV_WriteL32Bits(pb, pNidxInfo->videoHei);          //video height, pos 0x2c
	MOV_WriteL32Bits(pb, pNidxInfo->videoCodec);        //videocodec,   pos 0x30
	MOV_WriteL32Bits(pb, pNidxInfo->videoVfr);          //vidframerate  pos 0x34
	MOV_WriteL32Bits(pb, pNidxInfo->audioSampleRate);   //audio SR,     pos 0x38
	MOV_WriteL32Bits(pb, pNidxInfo->audioChannel);      //audiochannel, pos 0x3c
	MOV_WriteL32Bits(pb, pNidxInfo->headerSize);        //headersize,   pos 0x40

	//if (pNidxInfo->versionCode == MEDIARECVY_VERSION)//2012/09/11 Meg Lin
	MOV_WriteL32Bits(pb, SM2015_RCVY_CO64_VERSION);      //version,      pos 0x44
	//write video
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoFN);//thisVidLen,  pos 0x48
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);

	//write audio
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioFN);
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisAudioFrameData, pNidxInfo->thisAudioFrmDataLen);

	MOV_WriteL32Bits(pb, pNidxInfo->h264descLen);//h264 desc Len
	if (pNidxInfo->h264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->h264descData, pNidxInfo->h264descLen);
	}

	//#NT#2012/09/12#Meg Lin -begin
	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr + pNidxInfo->clusterSize - 8);
	MOV_MakeAtomInit((void *)pBuf);
	MOV_WriteL32Bits(pb, pNidxInfo->clusterSize);//cluster size     end -8
	MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //cluster id       end -4
	//#NT#2012/09/12#Meg Lin -end

}
// nidxDUAL data
// len(4) + free(4) + len(4) + nidx(4)
// VT start(4) + VT size(4) + v1 pos(4) + v1 size(4)
// ...
// AT start(4) + AT size(4) + lastJunkPos
// free len = cluster size(g_aviRecFileObj.clusterSize)
// nidx len = 0x18 + vfn*8
static void MOVWrite_MakeDUALNidx(SM_REC_DUAL_NIDXINFO *pNidxInfo, UINT32 id)
{
	UINT32 *pBuf;
	UINT32 nidxlen, oneentry = MP_MOV_SIZE_ONEENTRY;
	char *pb;
	nidxlen = MP_MOV_NIDX_FIXEDSIZE + pNidxInfo->thisVideoFN * oneentry;
	nidxlen += pNidxInfo->thisAudioFN * oneentry;
	if (pNidxInfo->h264descLen) {
		nidxlen += (4 + pNidxInfo->h264descLen); //len + data
	}
	//sub-stream (no use if 0)
	nidxlen += pNidxInfo->thisSubVideoFN * oneentry;
	if (pNidxInfo->Subh264descLen) {
		nidxlen += (4 + pNidxInfo->Subh264descLen); //len + data
	}

	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr);
	MOV_MakeAtomInit((void *)pBuf);
	pb = NULL;//given null pointer is ok
	MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, pNidxInfo->clusterSize);//size for free
	MOV_WriteB32Tag(pb, "free");
	MOV_WriteB32Bits(pb, nidxlen);//size for nidx
	MOV_WriteB32Tag(pb, "nidx");
	MOV_WriteL32Bits(pb, g_mov_nidx_timeid[id]);
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioStartFN);  //aud this start, pos 0x14
	MOV_WriteL32Bits(pb, pNidxInfo->audioCodec);        //aud codec,      pos 0x18
	MOV_WriteL32Bits(pb, pNidxInfo->lastNidxdataPos);   //lastjunkPos,  pos 0x1c
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoStartFN);  //vidstartFN,   pos 0x20
	MOV_WriteL32Bits(pb, pNidxInfo->videoWid);          //video width,  pos 0x24
	MOV_WriteL32Bits(pb, pNidxInfo->videoHei);          //video height, pos 0x28
	MOV_WriteL32Bits(pb, pNidxInfo->videoCodec);        //videocodec,   pos 0x2c
	MOV_WriteL32Bits(pb, pNidxInfo->videoVfr);          //vidframerate  pos 0x30
	MOV_WriteL32Bits(pb, pNidxInfo->audioSampleRate);   //audio SR,     pos 0x34
	MOV_WriteL32Bits(pb, pNidxInfo->audioChannel);      //audiochannel, pos 0x38
	MOV_WriteL32Bits(pb, pNidxInfo->headerSize);        //headersize,   pos 0x3c
	MOV_WriteL32Bits(pb, SMDUAL_RCVY_VERSION);          //version,      pos 0x40

	//write main-stream entry
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoFN);//thisVidLen,  pos 0x44
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);

	//write audio entry
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioFN);
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisAudioFrameData, pNidxInfo->thisAudioFrmDataLen);

	//write main-stream desc
	MOV_WriteL32Bits(pb, pNidxInfo->h264descLen);//h264 desc Len,  pos 0x44+len
	if (pNidxInfo->h264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->h264descData, pNidxInfo->h264descLen);
	}

	if (pNidxInfo->thisSubVideoStartFN == 0) {
		MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //real data end
	}

	//write sub-stream info (no use if 0)
	MOV_WriteL32Bits(pb, pNidxInfo->thisSubVideoStartFN);  //vidstartFN,   pos
	MOV_WriteL32Bits(pb, pNidxInfo->SubvideoWid);          //video width,  pos
	MOV_WriteL32Bits(pb, pNidxInfo->SubvideoHei);          //video height, pos
	MOV_WriteL32Bits(pb, pNidxInfo->SubvideoCodec);        //videocodec,   pos
	MOV_WriteL32Bits(pb, pNidxInfo->SubvideoVfr);          //vidframerate  pos

	//write sub-stream entry (no use if 0)
	MOV_WriteL32Bits(pb, pNidxInfo->thisSubVideoFN);//thisVidLen,
	if (pNidxInfo->thisSubVideoFrmDataLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->thisSubVideoFrameData, pNidxInfo->thisSubVideoFrmDataLen);
	}

	//write sub-stream desc
	MOV_WriteL32Bits(pb, pNidxInfo->Subh264descLen);//h264 desc Len
	if (pNidxInfo->Subh264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->Subh264descData, pNidxInfo->Subh264descLen);
	}

	if (pNidxInfo->thisSubVideoStartFN != 0) {
		MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //real data end
	}

	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr + pNidxInfo->clusterSize - 8);
	MOV_MakeAtomInit((void *)pBuf);
	MOV_WriteL32Bits(pb, pNidxInfo->clusterSize);//cluster size     end -8
	MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //cluster id       end -4
}

static void MOVWrite_MakeDUALNidx_co64(SM_REC_DUAL_CO64_NIDXINFO *pNidxInfo, UINT32 id)
{
	UINT32 *pBuf;
	UINT32 nidxlen, oneentry = MP_MOV_SIZE_CO64_ONEENTRY;
	char *pb;

	nidxlen = MP_MOV_NIDX_FIXEDSIZE + pNidxInfo->thisVideoFN * oneentry;
	nidxlen += pNidxInfo->thisAudioFN * oneentry;
	if (pNidxInfo->h264descLen) {
		nidxlen += (4 + pNidxInfo->h264descLen); //len + data
	}
	//sub-stream (no use if 0)
	nidxlen += pNidxInfo->thisSubVideoFN * oneentry;
	if (pNidxInfo->Subh264descLen) {
		nidxlen += (4 + pNidxInfo->Subh264descLen); //len + data
	}

	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr);
	MOV_MakeAtomInit((void *)pBuf);
	pb = NULL;//given null pointer is ok
	MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, pNidxInfo->clusterSize);//size for free
	MOV_WriteB32Tag(pb, "free");
	MOV_WriteB32Bits(pb, nidxlen);//size for nidx
	MOV_WriteB32Tag(pb, "nidx");
	MOV_WriteL32Bits(pb, g_mov_nidx_timeid[id]);
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioStartFN);  //aud this start, pos 0x14
	MOV_WriteL32Bits(pb, pNidxInfo->audioCodec);        //aud codec,      pos 0x18
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoStartFN);  //vidstartFN,   pos 0x1c
	MOV_WriteL64Bits(pb, pNidxInfo->lastNidxdataPos64); //lastjunkPos,  pos 0x20
	MOV_WriteL32Bits(pb, pNidxInfo->videoWid);          //video width,  pos 0x28
	MOV_WriteL32Bits(pb, pNidxInfo->videoHei);          //video height, pos 0x2c
	MOV_WriteL32Bits(pb, pNidxInfo->videoCodec);        //videocodec,   pos 0x30
	MOV_WriteL32Bits(pb, pNidxInfo->videoVfr);          //vidframerate  pos 0x34
	MOV_WriteL32Bits(pb, pNidxInfo->audioSampleRate);   //audio SR,     pos 0x38
	MOV_WriteL32Bits(pb, pNidxInfo->audioChannel);      //audiochannel, pos 0x3c
	MOV_WriteL32Bits(pb, pNidxInfo->headerSize);        //headersize,   pos 0x40
	MOV_WriteL32Bits(pb, SMDUAL_RCVY_CO64_VERSION);     //version,      pos 0x44

	//write main-stream entry
	MOV_WriteL32Bits(pb, pNidxInfo->thisVideoFN);//thisVidLen,  pos 0x48
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisVideoFrameData, pNidxInfo->thisVideoFrmDataLen);

	//write audio entry
	MOV_WriteL32Bits(pb, pNidxInfo->thisAudioFN);
	MOV_WriteBuffer(pb, (char *)pNidxInfo->thisAudioFrameData, pNidxInfo->thisAudioFrmDataLen);

	//write main-stream desc
	MOV_WriteL32Bits(pb, pNidxInfo->h264descLen);//h264 desc Len
	if (pNidxInfo->h264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->h264descData, pNidxInfo->h264descLen);
	}

	if (pNidxInfo->thisSubVideoStartFN == 0) {
		MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //real data end
	}

	//write sub-stream info (no use if 0)
	MOV_WriteL32Bits(pb, pNidxInfo->thisSubVideoStartFN);  //vidstartFN,   pos
	MOV_WriteL32Bits(pb, pNidxInfo->SubvideoWid);          //video width,  pos
	MOV_WriteL32Bits(pb, pNidxInfo->SubvideoHei);          //video height, pos
	MOV_WriteL32Bits(pb, pNidxInfo->SubvideoCodec);        //videocodec,   pos
	MOV_WriteL32Bits(pb, pNidxInfo->SubvideoVfr);          //vidframerate  pos

	//write sub-stream entry (no use if 0)
	MOV_WriteL32Bits(pb, pNidxInfo->thisSubVideoFN);//thisVidLen,
	if (pNidxInfo->thisSubVideoFrmDataLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->thisSubVideoFrameData, pNidxInfo->thisSubVideoFrmDataLen);
	}

	//write sub-stream desc
	MOV_WriteL32Bits(pb, pNidxInfo->Subh264descLen);//h264 desc Len
	if (pNidxInfo->Subh264descLen) {
		MOV_WriteBuffer(pb, (char *)pNidxInfo->Subh264descData, pNidxInfo->Subh264descLen);
	}

	if (pNidxInfo->thisSubVideoStartFN != 0) {
		MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //real data end
	}

	pBuf = (UINT32 *)(pNidxInfo->dataoutAddr + pNidxInfo->clusterSize - 8);
	MOV_MakeAtomInit((void *)pBuf);
	MOV_WriteL32Bits(pb, pNidxInfo->clusterSize);//cluster size     end -8
	MOV_WriteL32Bits(pb, MOV_NIDX_CLUR);         //cluster id       end -4
}

// gps data //2012/11/02 Meg
// len(4) + free (4) +  GPS (4) + len (4) (order follows AVI, data+len(LE))
// len = cluster size -8
// GPS  len = from project layer
static void MOVWrite_MakeGPS(MEDIAREC_GPSINFO *pGPSInfo)
{
	UINT32 *pBuf;
	char *pb;

	// note: should use 2nd API set for GPS data, otherwise it may conflict with mov header making
	pBuf = (UINT32 *)(pGPSInfo->bufAddr);
	MOV_MakeAtomInit2((void *)pBuf);
	pb = NULL;//given null pointer is ok
	MOV_WriteB32Bits2(pb, pGPSInfo->clusterSize);//size for free
	MOV_WriteB32Tag2(pb, "free");
	MOV_WriteB32Tag2(pb, "GPS ");
	MOV_WriteL32Bits2(pb, pGPSInfo->gpsDataSize);//size for GPS

	MOV_WriteBuffer2(pb, (char *)pGPSInfo->gpsDataAddr, pGPSInfo->gpsDataSize);
}
/*
static ER MOVWrite_RecvUpdateHDR(UINT32 p1, UINT32 p2, UINT32 p3)
{
    UINT32 *ptr32, count, i, max=100, mdatsize, outAddr, oldFilesize;
    MEDIAREC_RECVFNTHDR *pHdr;

    pHdr = (MEDIAREC_RECVFNTHDR *)p1;
    outAddr = pHdr->outAddr;
    oldFilesize = pHdr->oldFilesize;
    ptr32 = (UINT32 *)outAddr;
    count = (pHdr->clustersize)/4;
    if (count > max)
        count = max;
    i=0;
    while (i<count)
    {
        if (*ptr32++ == 0x7461646d)//mdat
        {
            break;
        }
        i+=1;
    }
    if (i<count)
    {
        ptr32 = (UINT32 *)(outAddr+(i-1)*4);//get len pos, =mdat pos -4
        mdatsize = oldFilesize -(i-1)*4;//len = oldfilesize - lenpos
        *ptr32 = Mov_Swap(mdatsize);
        return E_OK;
    }

    return E_SYS;

}*/

static UINT32 MOVWriteMakerTag(char *pb, char *pMaker, UINT32 len)
{
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Bits(pb, 0xA9666D74);//A9, fmt

	MOV_WriteB16Bits(pb, len);
	MOV_WriteL16Bits(pb, 0);
	MOV_WriteBytes(pb, pMaker, len);
	return MOVWriteUpdateData(pb, pos);
}

static UINT32 MOVWriteModelTag(char *pb, char *pModel, UINT32 len)
{
	MOV_Offset  pos = MOV_TellOffset(pb);
	MOV_WriteB32Bits(pb, 0);//size
	MOV_WriteB32Bits(pb, 0xA9696E66);//A9, inf

	MOV_WriteB16Bits(pb, len);
	MOV_WriteL16Bits(pb, 0);
	MOV_WriteBytes(pb, pModel, len);
	return MOVWriteUpdateData(pb, pos);
}

static ER MOVWriteCheckID(UINT32 type, UINT32 id)
{
	if (id >= MOVWRITER_COUNT) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("type=0x%x, id error %d!\r\n", type, id);
		}
		return E_PAR;
	} else {
		return E_OK;
	}
}

UINT32 MOVWrite_UserMakerModelData(MOV_USER_MAKERINFO *pMaker)
{
	//MOV_Offset  pos = 0;
	char *pb = 0;
	MOV_MakeAtomInit((void *)pMaker->ouputAddr);

	MOVWriteMakerTag(pb, (char *)pMaker->pMaker, pMaker->makerLen);
	MOVWriteModelTag(pb, (char *)pMaker->pModel, pMaker->modelLen);
	return (12 * 2 + pMaker->makerLen + pMaker->modelLen);

}


ER MovWriteLib_RegisterObjCB(void *pobj)
{
	CONTAINERMAKER *pContainer;
	pContainer = (CONTAINERMAKER *)pobj;
	if (pContainer->checkID != MOVWRITELIB_CHECKID) {
		return E_NOSPT;
	}
	gMovContainerMaker.cbWriteFile = (pContainer->cbWriteFile);
	gMovContainerMaker.cbShowErrMsg = (pContainer->cbShowErrMsg);
	gMovContainerMaker.cbReadFile = (pContainer->cbReadFile);

	//if (gMovWriteContainer.cbShowErrMsg)
	//    (gMovWriteContainer.cbShowErrMsg)("Create Obj OK\r\n");
	return E_OK;
}
#if 0
ER MovWriteLib_RegisterWriteCB(void *pobj)
{
	CONTAINERMAKER *pContainer;
	pContainer = (CONTAINERMAKER *)pobj;
	if (pContainer->checkID != MOVWRITELIB_CHECKID) {
		return E_NOSPT;
	}
	gMovContainerMaker.cbWriteFile = (pContainer->cbWriteFile);

	//if (gMovWriteContainer.cbShowErrMsg)
	//    (gMovWriteContainer.cbShowErrMsg)("Create Obj OK\r\n");
	return E_OK;
}
#endif
ER MovWriteLib_ChangeWriteCB(void *pobj)
{
	CONTAINERMAKER *pContainer;
	pContainer = (CONTAINERMAKER *)pobj;
	gMovContainerMaker.cbWriteFile = (pContainer->cbWriteFile);

	//if (gMovContainerMaker.cbShowErrMsg)
	//    (gMovContainerMaker.cbShowErrMsg)("Create Obj OK\r\n");
	//DBG_DUMP("cb=0x%lx!\r\n", (pContainer->cbWriteFile));
	return E_OK;
}

ER MovWriteLib_Initialize(UINT32 id)
{
	if (id >= MOVWRITER_COUNT) {
		return E_PAR;
	}

	gMovWriteLib_Info[id].bufAddr = 0;
	gMovWriteLib_Info[id].bufSize = 0;
	return E_OK;
}

ER MovWriteLib_SetMemBuf(UINT32 id, UINT32 startAddr, UINT32 size)
{
	if (id >= MOVWRITER_COUNT) {
		return E_PAR;
	}
	gMovWriteLib_Info[id].bufAddr = startAddr;
	gMovWriteLib_Info[id].bufSize = size;
	//g_movTrack[id].gpstag_addr =0;//initialize seems no called

	return E_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// -----------------------------------------------------------------------
// | ftyp (standard, file type)                                          |
// -----------------------------------------------------------------------
// | fre1 (customized, for data which should be located in front header) |
// -----------------------------------------------------------------------
// | thum (customized, for our own thumbnail)                            |
// -----------------------------------------------------------------------
// | mdat (standard, main video/audio data)                              |
// -----------------------------------------------------------------------
// | moov (standard, video/audio tables)                                 |
// -----------------------------------------------------------------------
//
///////////////////////////////////////////////////////////////////////////////////////////////////

ER MovWriteLib_MakeHeader(UINT32 hdrAddr, UINT32 *pSize, MEDIAREC_HDR_OBJ *pobj)
{
	//UINT32 *p32Data;
	//UINT32 tagSize =0, filepos =0;
	//UINT8 finish=0;
	UINT8   *ptr;//2010/01/29 Meg Lin
	UINT8   pChar[8]; //2010/01/29 Meg Lin
	UINT32  i, uiFre1Size = 0;//, uiThumbSize = 0;
	//#NT#2012/06/26#Hideo Lin -begin
	//#NT#To support MP4 file type
	UINT32  uiFtypSize, freasize = 0;
	UINT8   *pFtyp;
	//#NT#2012/06/26#Hideo Lin -end
	//UINT32  wordAlign4size = 0;//2012/10/22 Meg
	/* remove for 680 no need
	    if (gMovContainerMaker.cbWriteFile == NULL) {
	        if (gMovContainerMaker.cbShowErrMsg) {
	            (gMovContainerMaker.cbShowErrMsg)("ERROR!! cbWriteFile=NULL!!!\r\n");
	        }
	        return E_NOSPT;
	    }
	    */
	//if (gMovContainerMaker.cbShowErrMsg)
	//{
	//    (gMovContainerMaker.cbShowErrMsg)("Start MAKE header!!!\r\n");
	//}

	//#NT#2012/06/26#Hideo Lin -begin
	//#NT#To support MP4 file type
	if (pobj->uiFileType == MEDIA_FTYP_MP4) {
		uiFtypSize = MP4_FTYP_SIZE;
		pFtyp = g_mp4Ftyp;
	} else {
		uiFtypSize = MOV_FTYP_SIZE;
		pFtyp = g_movFtyp;
	}
	//#NT#2012/06/26#Hideo Lin -end

	//if (gMovWriteLib_Info.bufAddr== NULL)
	//{
	//    if (gMovWriteContainer.cbShowErrMsg)
	//    {
	//        (gMovWriteContainer.cbShowErrMsg)("ERROR!! bufaddr=NULL!!!\r\n");
	//    }
	//    return E_PAR;
	//}

	//#NT#2012/08/24#Hideo Lin -begin
	//#NT#Add 'thum' atom for thumbnail data (customized by ourselves)
	//#NT#2009/11/24#Meg Lin -begin
	//*pSize = 8;//len (4) + mdat (4)
	ptr = (UINT8 *)hdrAddr;
	//DBG_DUMP("hdrAddr=0x%lx!\r\n", hdrAddr);
	for (i = 0; i < uiFtypSize; i++) {
		*(ptr + i) = *(pFtyp + i);
	}

	//#NT#2010/01/29#Meg Lin# -begin
	// 'fre1' atom (for special data should be located in front header; customized by ourselves)
	ptr += uiFtypSize;
	//if (g_movTrack.fre1_size != 0)
	if (pobj->uiFre1Size != 0) {
		uiFre1Size = pobj->uiFre1Size + 8;//len + tag
		pChar[0] = (uiFre1Size >> 24) & 0xFF;
		pChar[1] = (uiFre1Size >> 16) & 0xFF;
		pChar[2] = (uiFre1Size >> 8) & 0xFF;
		pChar[3] = uiFre1Size & 0xFF;
		pChar[4] = 0x66;//f
		pChar[5] = 0x72;//r
		pChar[6] = 0x65;//e
		pChar[7] = 0x31;//1
		for (i = 0; i < 8; i++) {
			*(ptr + i) = *(pChar + i);
		}

		memcpy((void *)(ptr + 8), (void *)pobj->uiFre1Addr, pobj->uiFre1Size);
	}
	//*pSize = (uiFtypSize+8+uiFre1Size);// ftyp +mdat //marked; useless now, since front header size is fixed

	// 'thum' atom (for thumbnail data; customized by ourselves)
	ptr += uiFre1Size;
	//DBG_DUMP("movlib:uiThumbSize=%d!\r\n", pobj->uiThumbSize);
	//#NT#2019/10/15#Willy Su -begin
	//#NT#To support write 'frea'/'skip' tag without thumbnail
	#if 0
	if (pobj->uiThumbSize) {
	#endif
	{
		freasize = MOVWriteFREATag(0, pobj, (UINT32)ptr);
		pobj->uiFreaSize = freasize;
		ptr += freasize;
		g_movTrack[pobj->uiMakerID].uiFreaSize = freasize;
		pobj->uiFreaEndSize += freasize;
	}

//#NT#2019/12/20#Support front moov -begin
	pobj->uiFreaEndSize = uiFtypSize + uiFre1Size + freasize;
	DBG_IND("^M pobj->uiFreaEndSize=0x%lx\r\n", pobj->uiFreaEndSize);
//#NT#2019/12/20#Support front moov -end

#if 0
	if (g_movTrack.uiThumbSize) {
		wordAlign4size = ALIGN_CEIL_4(g_movTrack.uiThumbSize + 8);//len + tag
		//pad4 = (g_movTrack.uiThumbSize + 8)&0x3;//2012/10/24 Meg
		uiThumbSize = wordAlign4size;//2012/10/22 Meg
		pChar[0] = (uiThumbSize >> 24) & 0xFF;
		pChar[1] = (uiThumbSize >> 16) & 0xFF;
		pChar[2] = (uiThumbSize >> 8) & 0xFF;
		pChar[3] = uiThumbSize & 0xFF;
		pChar[4] = 't';
		pChar[5] = 'h';
		pChar[6] = 'm';
		pChar[7] = 'a';
		for (i = 0; i < 8; i++) {
			*(ptr + i) = *(pChar + i);
		}

		// copy thumbnail data
		memcpy((void *)(ptr + 8), (void *)g_movTrack.uiThumbAddr, g_movTrack.uiThumbSize);
		ptr += uiThumbSize;//2012/10/24 Meg
	}
	if (g_movTrack.uiScreenSize) {
		wordAlign4size = ALIGN_CEIL_4(g_movTrack.uiScreenSize + 8); //len + tag
		//pad4 = (g_movTrack.uiThumbSize + 8)&0x3;//2012/10/24 Meg
		uiThumbSize = wordAlign4size;//2012/10/22 Meg
		pChar[0] = (uiThumbSize >> 24) & 0xFF;
		pChar[1] = (uiThumbSize >> 16) & 0xFF;
		pChar[2] = (uiThumbSize >> 8) & 0xFF;
		pChar[3] = uiThumbSize & 0xFF;
		pChar[4] = 's';
		pChar[5] = 'c';
		pChar[6] = 'r';
		pChar[7] = 'a';
		for (i = 0; i < 8; i++) {
			*(ptr + i) = *(pChar + i);
		}

		// copy thumbnail data
		memcpy((void *)(ptr + 8), (void *)g_movTrack.uiScreenAddr, g_movTrack.uiScreenSize);
		ptr += uiThumbSize;//2012/10/24 Meg
	}
#endif

	ptr += 4; // jump 4 bytes (mdat size)
	*ptr++ = 0x6d;//m
	*ptr++ = 0x64;//d
	*ptr++ = 0x61;//a
	*ptr++ = 0x74;//t

	//#NT#2012/08/22#Hideo Lin -begin
	//#NT#For movie thumbnail
	//*pSize = g_movTrack.clusterSize;
	*pSize = pobj->uiHeaderSize;
	//#NT#2012/08/22#Hideo Lin -end
	//DBG_DUMP("uiHeaderSize=%d!\r\n", pobj->uiHeaderSize);

	//#NT#2010/01/29#Meg Lin# -end
	//#NT#2009/11/24#Meg Lin -end
	//#NT#2012/08/24#Hideo Lin -end
	//debug_msg("MakeHeader size = 0x%lx!\r\n", pobj->uiHeaderSize);
	if (gMovRecFileInfo[pobj->uiMakerID].uiTempHdrAdr) {
		//DBG_DUMP("^M copy dst=0x%lx, src=0x%lx, size=0x%lx\r\n",gMovRecFileInfo[pobj->uiMakerID].uiTempHdrAdr, hdrAddr, pobj->uiHeaderSize);
		//memcpy((char *)gMovRecFileInfo[pobj->uiMakerID].uiTempHdrAdr, (char *)hdrAddr, pobj->uiHeaderSize);
		gMovTempHDR[pobj->uiMakerID] = hdrAddr;
		DBG_IND("MOV [%d]tempHDR=0x%lx\r\n",pobj->uiMakerID, hdrAddr);
		gMovTempHDRsize[pobj->uiMakerID] = pobj->uiHeaderSize;
	}
	return E_OK;
}

//#NT#2012/09/19#Meg Lin -begin
ER MovWriteLib_GetInfo(MEDIAWRITE_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3)
{
	ER resultV = E_PAR;
	UINT32 fr;
	UINT32 vfn = 0, afr = 0, vMax = 0, id, temp, tempend, size_h;
	UINT32  vfr, sec, size;
	MOV_Ientry      *pEntry = 0;
	MP_ESTIMATE_VNUM *ptr;
	MEDIAREC_ENTRY_INFO *pinfo;
	SM_AUD_NIDXINFO *paudNidxinfo;
	SM_VID_NIDXINFO *pvidNidxinfo;
	ER chk_id;
	//DBG_DUMP("type = %d\r\n", type);
	//fr = gMovRecFileInfo.VideoFrameRate;
	//UINT8 p1outsize=0, p2outsize=0, p3outsize=0;
	switch (type) {
	case MEDIAWRITE_GETINFO_HEADERSIZE:
		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			return E_PAR;
		}
		fr = gMovRecFileInfo[id].VideoFrameRate;
		if ((gMovRecFileInfo[id].encAudioCodec == MOVAUDENC_AAC)
			|| (gMovRecFileInfo[id].encAudioCodec == MOVAUDENC_PPCM)) { //2015/10/16
			*pparam2 = fr * 20 + (gMovRecFileInfo[id].AudioSampleRate / 1024) * 20; //vid+aud
		} else { //PCM
			*pparam2 = fr * 20 + 16;    //vid+aud
		}
		resultV = E_OK;
		break;
	//#NT#2010/04/20#Meg Lin -begin
	//#NT#Bug: video entry buffer not enough for small resolution
	case MEDIAWRITE_GETINFO_VALID_VFNUM:
		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			return E_PAR;
		}
		temp = g_movEntryMax[id];
		*pparam2 = temp;//vid+aud
		resultV = E_OK;
		break;
	//#NT#2010/04/20#Meg Lin -end
	case MEDIAWRITE_GETINFO_VIDEOENTRY_ADDR:
		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			return E_PAR;
		}
		vfn = *pparam2;
		if (g_movEntryAddr[id] == 0) {
			resultV = E_SYS;
			break;
		}
		//debug_msg("pentry1 = 0x%lx!\r\n", g_movEntryAddr[id]);
		pEntry = (MOV_Ientry *)g_movEntryAddr[id] + vfn;
		//debug_msg("pentry2 = 0x%lx, size=0x%lx!\r\n", pEntry, pEntry->size);
		fr = gMovRecFileInfo[id].VideoFrameRate;


		pinfo = (MEDIAREC_ENTRY_INFO *)pparam3;
		pinfo->entryAddr = (UINT32)pEntry;
		pinfo->entrySize = fr * sizeof(MOV_Ientry);
		resultV = E_OK;
		break;
	case MEDIAWRITE_GETINFO_SM_AUDENTRY_ADDR:

		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			return E_PAR;
		}
		paudNidxinfo = (SM_AUD_NIDXINFO *)pparam2;
		if (g_movAudEntryAddr[id] == 0) {
			resultV = E_SYS;
			break;
		}
		pEntry = (MOV_Ientry *)g_movAudEntryAddr[id] + paudNidxinfo->nowAfn;
		fr = (paudNidxinfo->thisAfn);

		//DBG_DUMP("nowA=%d fr=%d\r\n", paudNidxinfo->nowAfn,fr);
		pinfo = (MEDIAREC_ENTRY_INFO *)pparam3;
		pinfo->entryAddr = (UINT32)pEntry;
		pinfo->entrySize = fr * sizeof(MOV_Ientry);
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_SM_VIDENTRY_ADDR:

		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		pinfo = (MEDIAREC_ENTRY_INFO *)pparam3;
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			pinfo->entrySize = 0;
			return E_PAR;
		}
		pvidNidxinfo = (SM_VID_NIDXINFO *)pparam2;
		if (g_movEntryAddr[id] == 0) {
			resultV = E_SYS;
			pinfo->entrySize = 0;
			break;
		}
		pEntry = (MOV_Ientry *)g_movEntryAddr[id] + pvidNidxinfo->nowVfn;
		fr = (pvidNidxinfo->thisVfn);

		//DBG_DUMP("nowV=%d fr=%d\r\n", pvidNidxinfo->nowVfn,fr);
		pinfo->entryAddr = (UINT32)pEntry;
		pinfo->entrySize = fr * sizeof(MOV_Ientry);
		resultV = E_OK;
		break;


	case MEDIAWRITE_GETINFO_AUDIOENTRY_ADDR:
		id = *pparam1;
		pinfo = (MEDIAREC_ENTRY_INFO *)pparam3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			pinfo->entrySize = 0;
			return E_PAR;
		}
		vfn = *pparam2;
		if (g_movAudEntryAddr[id] == 0) {
			resultV = E_SYS;
			pinfo->entrySize = 0;
			break;
		}
		//debug_msg("pentry1 = 0x%lx!\r\n", g_movEntryAddr[id]);
		pEntry = (MOV_Ientry *)g_movAudEntryAddr[id] + vfn;
		//debug_msg("pentry2 = 0x%lx, size=0x%lx!\r\n", pEntry, pEntry->size);
		pinfo->entryAddr = pEntry->pos;
		pinfo->entrySize = pEntry->size;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_SIZE_ONEENTRY:
		*pparam1 =  MP_MOV_SIZE_ONEENTRY;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_NEWIDX1_BUF:
		*pparam1 =  gMovRecvy_newMoovAddr;
		*pparam2 =  gMovRecvy_newMoovSize;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_BACK1STHEAD_BUF:
		*pparam1 =  gMovRecvy_Back1sthead;
		*pparam2 =  gMovRecFileInfo[0].hdr_revSize;//2012/10/22 Meg
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_ROUGHHDRSIZE:
		*pparam3 = (*pparam1 / 100) * 20 + *pparam2 * 20; //vid+aud
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_ROUGH_VVFNUM://rough valid video frame num
		ptr = (MP_ESTIMATE_VNUM *)pparam1;
		DBG_DUMP("ROUGH_VVFNUM:ptrisze22 = 0x%lx!\r\n", ptr->size);

		fr = MOVWrite_TransRoundVidFrameRate(ptr->vfr);//vid
		afr = ptr->afr;//aud
		if (afr > 1) { //not PCM
			//2014/06/30 fixbug: calculate MP4 maxtime wrong
			vMax = MOV_Write_CalcV(ptr->size, afr * 1024, fr); //MP_AUDENC_AAC_RAW_BLOCK
		} else {
			vMax = MOV_Write_CalcV(ptr->size, 0, fr);
		}
		*pparam2 = vMax;//2012/10/29 Meg
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_FINALFILESIZE:
		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			return E_PAR;
		}
		*pparam2 = g_movCon[id].finalsize;
		*pparam3 = g_movCon[id].finalsize >> 32;
		//DBG_DUMP("^M get 2=0x%lx, 3=0x%lx\r\n", *pparam2, *pparam3);
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_VFR_TRANS:
		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			return E_PAR;
		}
		*pparam2 = g_movCon[id].finalsize;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_SM_VID_POS:

		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_GETINFO_SM_VID_POS, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			*pparam3 = 0;
			return E_PAR;
		}
		if (g_movEntryAddr[id] == 0) {
			resultV = E_SYS;
			*pparam3 = 0;
			break;
		}
		if (pparam3 == 0) {
			resultV = E_SYS;
			break;
		}
		temp = *pparam2;
		pEntry = (MOV_Ientry *)g_movEntryAddr[id] + temp;
		tempend = pEntry->pos + pEntry->size;
		//DBG_DUMP("nowV=%d pos=0x%lx\r\n", temp,tempend);
		temp = tempend;
		*pparam3 = temp;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_SM_AUD_POS:

		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_GETINFO_SM_AUD_POS, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			*pparam3 = 0;
			return E_PAR;
		}
		if (g_movAudEntryAddr[id] == 0) {
			resultV = E_SYS;
			*pparam3 = 0;
			break;
		}
		if (pparam3 == 0) {
			resultV = E_SYS;
			break;
		}
		temp = *pparam2;
		pEntry = (MOV_Ientry *)g_movAudEntryAddr[id] + temp;
		tempend = pEntry->pos + pEntry->size;

		//DBG_DUMP("nowA=%d pos=0x%lx\r\n", temp,tempend);
		temp = tempend;
		*pparam3 = temp;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_FRAMESIZE:
		vfr = *pparam1;
		sec = *pparam2;
		size = MOV_Write_GetEntrySize(48000, vfr, sec);
		resultV = E_OK;
		*pparam3 = size;
		break;

	case MEDIAWRITE_GETINFO_TRUNCATESIZE:
		*pparam1 = (UINT32)gMovRcvy_NewTruncatesize;
		//DBG_DUMP("GETINFO_TRUNCATESIZE=0x%llx\r\n", gMovRcvy_NewTruncatesize);
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_TRUNCATESIZE_H:
		size_h = (UINT32)(gMovRcvy_NewTruncatesize >> 32);

		*pparam1 = size_h;
		DBG_DUMP("GETINFO_TRUNCATESIZE_H=0x%lx\r\n", size_h);
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_BACKHDR_SIZE:
		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_GETINFO_BACKHDR_SIZE, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			*pparam3 = 0;
			return E_PAR;
		}
		if (pparam2) {
			*pparam2 = gMovMp4_backhdrsize[id];
		}
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_SYNCPOSFROMHEAD:
		id = *pparam1;
		temp = *pparam2;
		tempend = MOV_SyncPos(id, temp, 1);
		*pparam3 = tempend;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_VIDENTRYNUM:
		id = *pparam1;
		if (g_entry_pos[id].vid_tail > g_entry_pos[id].vid_head) {
			temp = g_entry_pos[id].vid_tail - g_entry_pos[id].vid_head;
		} else {
			pEntry = (MOV_Ientry *)g_movEntryAddr[id];
			temp = g_entry_pos[id].vid_tail - pEntry;
			pEntry = (MOV_Ientry *)g_movEntryAddr[id] + g_movEntryMax[id];
			tempend = pEntry - g_entry_pos[id].vid_head + 1;
			temp = temp + tempend;
		}
		DBG_IND("^G VIDENTRYNUM %d %d\r\n", temp, g_entry_pos[id].vid_num);
		*pparam3 = temp;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_AUDENTRYNUM:
		id = *pparam1;
		if (g_entry_pos[id].aud_tail > g_entry_pos[id].aud_head) {
			temp = g_entry_pos[id].aud_tail - g_entry_pos[id].aud_head;
		} else {
			pEntry = (MOV_Ientry *)g_movAudEntryAddr[id];
			temp = g_entry_pos[id].aud_tail - pEntry;
			pEntry = (MOV_Ientry *)g_movAudEntryAddr[id] + g_movAudioEntryMax[id];
			tempend = pEntry - g_entry_pos[id].aud_head + 1;
			temp = temp + tempend;
		}
		DBG_IND("^G AUDENTRYNUM %d %d\r\n", temp, g_entry_pos[id].aud_num);
		*pparam3 = temp;
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_TIMESCALE:
		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_GETINFO_TIMESCALE, id);
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			return E_PAR;
		}
		fr = *pparam2;
		*pparam3 = MOV_Wrtie_GetTimeScale(fr);
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_SM_SUBENTRY_ADDR:

		id = *pparam1;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		pinfo = (MEDIAREC_ENTRY_INFO *)pparam3;
		if (chk_id != E_OK) {
			DBG_DUMP("ERROR ID = %d\r\n", id);
			pinfo->entrySize = 0;
			return E_PAR;
		}
		pvidNidxinfo = (SM_VID_NIDXINFO *)pparam2;
		if (g_sub_movEntryAddr[id] == 0) {
			resultV = E_SYS;
			pinfo->entrySize = 0;
			break;
		}
		pEntry = (MOV_Ientry *)g_sub_movEntryAddr[id] + pvidNidxinfo->nowVfn;
		fr = (pvidNidxinfo->thisVfn);

		//DBG_DUMP("^R nowV=%d fr=%d\r\n", pvidNidxinfo->nowVfn,fr);
		pinfo->entryAddr = (UINT32)pEntry;
		pinfo->entrySize = fr * sizeof(MOV_Ientry);
		resultV = E_OK;
		break;

	case MEDIAWRITE_GETINFO_ENTRY_DURATION:
		id = *pparam1;
		vfn = *pparam2;
		fr = *pparam3;
		*pparam3 = MOV_Wrtie_GetEntryDuration(id, vfn, fr);
		resultV = E_OK;
		break;

	default:
		break;
	}
	return resultV;
}
//#NT#2012/09/19#Meg Lin -end

ER MovWriteLib_SetInfo(MEDIAWRITE_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	MOV_Ientry *ptr;
	MEDIAREC_FLASHINFO *pFlashInfo;
	CUSTOMDATA *pCdata;
	struct tm *p_timespace;

	//MEDIAREC_MAKEAVIIDX1 *pIdx;
	UINT32 id, chk_id;
	pFlashInfo = (MEDIAREC_FLASHINFO *)param1;
#if 0//MOV_RECDEBUG
	if ((type != MEDIAWRITE_SETINFO_MOVVIDEOENTRY) && (type != MEDIAWRITE_SETINFO_MOVAUDIOENTRY)
		&& (type != MEDIAWRITE_SETINFO_MP4VIDEOENTRY)) {
		if (gMovContainerMaker.cbShowErrMsg) {
			(gMovContainerMaker.cbShowErrMsg)("type=%x, 1=0x%lx,2=0x%lx,3=0x%lx!\r\n", type, param1, param2, param3);
		}
	}
#endif
	switch (type) {
	case MEDIAWRITE_SETINFO_WID_HEI:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_WID_HEI, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].ImageWidth = param1;
		gMovRecFileInfo[id].ImageHeight = param2;
		break;
	case MEDIAWRITE_SETINFO_VID_FRAME:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_VID_FRAME, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].videoFrameCount = param1;
		break;
	case MEDIAWRITE_SETINFO_AUD_FRAME:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_AUD_FRAME, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].audioFrameCount = param1;
		break;
	case MEDIAWRITE_SETINFO_REC_DURATION:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_REC_DURATION, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].PlayDuration = param1;
		//(gMovContainerMaker.cbShowErrMsg)("type=%x, 1=0x%lx,2=0x%lx,3=0x%lx!\r\n", type, param1, param2, param3);
		//DBG_DUMP("setDUR type=%x, 1=0x%lx,2=0x%lx,3=0x%lx!\r\n", type, param1, param2, param3);
		break;
	case MEDIAWRITE_SETINFO_FRAMEINFOBUF:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_FRAMEINFOBUF, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (gMovRecFileInfo[id].encAudioCodec == MOVAUDENC_PCM) { // PCM
			MOV_Write_SetEntryAddr(param1, param2, 0, 0, id);
		} else { // others
			MOV_Write_SetEntryAddr(param1, param2, gMovRecFileInfo[id].AudioSampleRate, gMovRecFileInfo[id].VideoFrameRate, id);
		}
		//(gMovContainerMaker.cbShowErrMsg)("^G FAMEBUF  type=%x, 1=0x%lx,2=0x%lx,id=0x%lx!\r\n", type, param1, param2, param3);
		break;

	case MEDIAWRITE_SETINFO_INITENTRYPOS:
		id = param3;
		MOV_Write_InitEntryAddr(id);
		break;

	case MEDIAWRITE_SETINFO_ENTRYPOS_ENABLE:
		if (param1) {
			g_mov_sync_pos = 1;
		} else {
			g_mov_sync_pos = 0;
		}
		break;

	case MEDIAWRITE_SETINFO_VID_FR:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_VID_FR, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].VideoFrameRate = param1;
		break;
	case MEDIAWRITE_SETINFO_MOVVIDEOENTRY:
		if (param2 == 0) {
			if (gMovContainerMaker.cbShowErrMsg) {
				(gMovContainerMaker.cbShowErrMsg)("ERROR!! set video entry p1=0x%lx, p2 = 0x%lx!\r\n", param1, param2);
			}
			break;
		}
		ptr = (MOV_Ientry *)param2;
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_MOVVIDEOENTRY, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (g_mov_sync_pos) {
			MOV_SetIentry_SyncPos(id, param1, ptr);
		} else {
			MOV_SetIentry(id, param1, ptr);
		}

		break;
	case MEDIAWRITE_SETINFO_MOVAUDIOENTRY:
		if (param2 == 0) {
			if (gMovContainerMaker.cbShowErrMsg) {
				(gMovContainerMaker.cbShowErrMsg)("ERROR!! set audio entry p1=0x%lx, p2 = 0x%lx!\r\n", param1, param2);
			}
			break;
		}
		ptr = (MOV_Ientry *)param2;
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_MOVAUDIOENTRY, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (g_mov_sync_pos) {
			MOV_SetAudioIentry_SyncPos(id, param1, ptr);
		} else {
			MOV_SetAudioIentry(id, param1, ptr);
		}

		break;
	case MEDIAWRITE_SETINFO_MOVMDATSIZE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_MOVMDATSIZE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].totalMdatSize = (UINT64)param1;
		if (param2) {
			gMovRecFileInfo[id].totalMdatSize += (UINT64)param2 << 32;
			//DBG_DUMP("^M mdatsize=0x%llx\r\n", gMovRecFileInfo[id].totalMdatSize);
		}
		break;
	case MEDIAWRITE_SETINFO_TEMPMOOVADDR:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_TEMPMOOVADDR, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].ptempMoovBufAddr = (UINT8 *)param1;
		break;
	case MEDIAWRITE_SETINFO_AUDIOSIZE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_AUDIOSIZE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].audioTotalSize = param1;
		break;
	case MEDIAWRITE_SETINFO_VIDEOTYPE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_VIDEOTYPE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].encVideoCodec = param1;
		break;
	case MEDIAWRITE_SETINFO_AUD_SAMPLERATE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_AUD_SAMPLERATE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].AudioSampleRate = (UINT16)param1;
		break;
	case MEDIAWRITE_SETINFO_AUD_BITRATE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_AUD_BITRATE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].AudioBitRate = param1;
		break;
	case MEDIAWRITE_SETINFO_AUDIOTYPE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_AUDIOTYPE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].encAudioCodec = param1;
		break;
	//#NT#2009/12/21#Meg Lin -begin
	//#NT#new feature: rec stereo
	case MEDIAWRITE_SETINFO_AUD_CHS:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_AUD_CHS, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].AudioChannel = (UINT16)param1;
		break;
	//#NT#2009/12/21#Meg Lin -end
	case MEDIAWRITE_SETINTO_MOV_264DESC:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINTO_MOV_264DESC, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (param2) {
			if (gMovRecFileInfo[id].encVideoCodec == MEDIAVIDENC_H264) { //2013/12/20 fixbug: MJPG+eCos hangs
				MovWriteLib_Make264VidDesc(param1, param2, id);
			}
		}
		break;

	case MEDIAWRITE_SETINFO_CUTENTRY:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_CUTENTRY, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}

		MOV_UpdateVentry(pFlashInfo->startSec, pFlashInfo->flashSec, pFlashInfo->fstVideoPos, pFlashInfo->frameRate, id);
		MOV_UpdateAentry(pFlashInfo->startSec, pFlashInfo->flashSec, pFlashInfo->fstAudioPos, id);
		break;

	case MEDIAWRITE_SETINFO_USERDATA:
		if ((param1 == 0) && (param2 != 0)) {
			break;
		}
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_USERDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].udta_addr = param1;
		g_movTrack[id].udta_size = param2;
		DBG_MSG("^Gudta_addr 0x%X, udta_size 0x%X\r\n", g_movTrack[id].udta_addr, g_movTrack[id].udta_size);
		break;

	case MEDIAWRITE_SETINFO_CUSTOMDATA:
		if ((param1 == 0) && (param2 != 0)) {
			DBG_MSG("^GMEDIAWRITE_SETINFO_CUSTOMDATA param1 zero!\r\n");
			break;
		}
		pCdata = (CUSTOMDATA *)param1;
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_USERDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].cudta_addr = pCdata->addr;
		g_movTrack[id].cudta_size = pCdata->size;
		g_movTrack[id].cudta_tag = pCdata->tag;
		DBG_MSG("^GCUSTOM_addr 0x%X, udta_size 0x%X tag=0x%lx\r\n", g_movTrack[id].cudta_addr, g_movTrack[id].cudta_size, g_movTrack[0].cudta_tag);
		break;

	//#NT#2010/01/29#Meg Lin# -begin
	case MEDIAWRITE_SETINFO_FRE1DATA:
		if ((param1 == 0) && (param2 != 0)) {
			break;
		}
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_FRE1DATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].fre1_addr = param1;
		g_movTrack[id].fre1_size = param2;
		//DBG_DUMP("FRE1 addr=0x%lx, size=0x%lx\r\n", param1, param2);
		//move to pHdrObj->uiFre1Addr
		break;
	//#NT#2010/01/29#Meg Lin# -end
	case MEDIAWRITE_SETINFO_UPDATEVIDPOS:
		if (param2 == 0) {
			if (gMovContainerMaker.cbShowErrMsg) {
				(gMovContainerMaker.cbShowErrMsg)("ERROR!! update video entry p1=0x%lx, p2 = 0x%lx!\r\n", param1, param2);
			}
			break;
		}
		ptr = (MOV_Ientry *)param2;
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_UPDATEVIDPOS, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		MOV_UpdateVidEntryPos(id, param1, ptr);

		break;
	case MEDIAWRITE_SETINFO_RESET:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_RESET, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		MOV_ResetEntry(id);
		break;

	//#NT#2012/03/22#Hideo Lin -begin
	//#NT#To set H.264 GOP type
	case MEDIAWRITE_SETINFO_H264GOPTYPE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_H264GOPTYPE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].uiH264GopType = param1;
		break;
	//#NT#2012/03/22#Hideo Lin -end

	//#NT#2012/06/26#Hideo Lin -begin
	//#NT#To support MP4 file type
	case MEDIAWRITE_SETINFO_FILETYPE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_FILETYPE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].uiFileType = param1;
		break;
	//#NT#2012/06/26#Hideo Lin -end

	case MEDIAWRITE_SETINFO_CLUSTERSIZE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_CLUSTERSIZE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].clusterSize = param1;
		gMovRecFileInfo[id].clustersize = param1;
		break;

	case MEDIAWRITE_SETINFO_NIDXDATA:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_NIDXDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		MOVWrite_MakeNidx((MEDIAREC_NIDXINFO *)param1);//no need id
		break;


	case MEDIAWRITE_SETINFO_TIMESPECIALID:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_TIMESPECIALID, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_mov_nidx_timeid[id] = param1;
		//DBG_DUMP("[%d] timeid=0x%lx!\r\n",id,  param1);
		break;

	case MEDIAWRITE_SETINFO_SM_NIDXDATA:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_SM_NIDXDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		//CHKPNT;
		if (param2) {
			MOVWrite_MakeSMNidx_co64((SM_REC_CO64_NIDXINFO *)param1);//no need id
		} else {
			MOVWrite_MakeSMNidx((SM_REC_NIDXINFO *)param1);//no need id
		}
		//  MOVWrite_MakeSMNidx((SM_REC_NIDXINFO *)par1);//no need id
		break;

	case MEDIAWRITE_SETINFO_2015_NIDXDATA:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_2015_NIDXDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (param2) {
			//CHKPNT;
			MOVWrite_Make2015Nidx_co64((SM_REC_CO64_NIDXINFO *)param1, id);//no need id
		} else {
			//CHKPNT;
			MOVWrite_Make2015Nidx((SM_REC_NIDXINFO *)param1, id);//no need id
		}
		break;

	case MEDIAWRITE_SETINFO_DUAL_NIDXDATA:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_DUAL_NIDXDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (param2) {
			//CHKPNT;
			MOVWrite_MakeDUALNidx_co64((SM_REC_DUAL_CO64_NIDXINFO *)param1, id);//no need id
		} else {
			//CHKPNT;
			MOVWrite_MakeDUALNidx((SM_REC_DUAL_NIDXINFO *)param1, id);//no need id
		}
		break;

	case MEDIAWRITE_SETINFO_GPSDATA://2012/11/02 Meg
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_GPSDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		MOVWrite_MakeGPS((MEDIAREC_GPSINFO *)param1);//no need id
		break;

	//now used, marked 2013/03/07 Meg
	//case MEDIAWRITE_SETINFO_MAKEIDX1DATA:
	//    pIdx = (MEDIAREC_MAKEAVIIDX1 *)param1;
	//    len = MOVRcvy_MakeMoovHeader(&gMovRecFileInfo);
	//    pIdx->outsize = len;
	//    break;

	//case MEDIAWRITE_SETINFO_RECVRY_UPDATEHDR:
	//    MOVWrite_RecvUpdateHDR(param1, param2, param3);
	//    break;

	//now used, marked 2013/03/07 Meg
	//case MEDIAWRITE_SETINFO_TEMPIDX1ADDR:
	//    if (param1 == 0)
	//    {
	//        if (gMovContainerMaker.cbShowErrMsg)
	//        {
	//            (gMovContainerMaker.cbShowErrMsg)("TEMPIDX1ADDR should not zero!!!\r\n");
	//        }
	//        return E_SYS;
	//    }
	//    gMovWriteMoovTempAdr = param1;
	//    break;

	case MEDIAWRITE_SETINFO_RECVRY_FILESIZE:
		if (param1 == 0) {
			if (gMovContainerMaker.cbShowErrMsg) {
				(gMovContainerMaker.cbShowErrMsg)("recovery file size should not zero!!!\r\n");
			}
			return E_SYS;
		}
		gMovRecFileInfo[0].recv_filesize = param1;
		if (param2) {
			gMovRecFileInfo[0].recv_filesize += (UINT64)param2 << 32;
			//DBG_DUMP("^M mdatsize=0x%llx\r\n", gMovRecFileInfo[0].recv_filesize);
		}

		//DBG_DUMP("^G 33totalfilesize %llx!\r\n", param1);
		//DBG_DUMP("^G 44totalfilesize %llx!\r\n", gMovRecFileInfo[0].recv_filesize);

		break;

	case MEDIAWRITE_SETINFO_RECVRY_MEMBUF:
		if ((param1 == 0) || (param2 == 0)) {
			return E_PAR;
		}
		//add for recovery
		gMovRecFileInfo[0].bufaddr = param1;
		gMovRecFileInfo[0].bufsize = param2;
		break;

	case MEDIAWRITE_SETINFO_RECVRY_BLKSIZE:
		if ((param1 == 0)) {
			return E_PAR;
		}
		//add for recovery
		gMovRecFileInfo[0].rcvy_blksize = param1;
		break;

	case MEDIAWRITE_SETINFO_RECVRY_MAX_VFR:
		if ((param1 == 0)) {
			return E_PAR;
		}
		//add for recovery
		gMovRecFileInfo[0].max_vfr_cfg = param1;
		break;

	case MEDIAWRITE_SETINFO_RECVRY_MAX_ASR:
		if ((param1 == 0)) {
			return E_PAR;
		}
		//add for recovery
		gMovRecFileInfo[0].max_asr_cfg = param1;
		break;

	case MEDIAWRITE_SETINFO_RECVRY_MAX_DUR:
		if ((param1 == 0)) {
			return E_PAR;
		}
		//add for recovery
		gMovRecFileInfo[0].max_dur_cfg = param1;
		break;

	//#NT#2012/08/22#Hideo Lin -begin
	//#NT#For setting thumbnail data
	case MEDIAWRITE_SETINFO_THUMB_DATA:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_THUMB_DATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].uiThumbAddr = param1;
		g_movTrack[id].uiThumbSize = param2;
		DBG_MSG("^GuiThumbAddr 0x%X, uiThumbSize 0x%X\r\n", g_movTrack[id].uiThumbAddr, g_movTrack[id].uiThumbSize);
		break;
	//#NT#2012/08/22#Hideo Lin -end

	case MEDIAWRITE_SETINFO_SCREEN_DATA://2013/05/28 Meg
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_THUMB_DATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].uiScreenAddr = param1;
		g_movTrack[id].uiScreenSize = param2;
		//DBG_MSG("^GuiScreenAddr 0x%X, uiScreenSize 0x%X\r\n", g_movTrack.uiScreenAddr, g_movTrack.uiScreenSize);
		break;

	//#NT#2012/08/22#Hideo Lin -begin
	//#NT#For setting MOV front header size (should be cluster alignment to improve file writing speed as movie recording)
	case MEDIAWRITE_SETINFO_HEADER_SIZE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_HEADER_SIZE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].uiHeaderSize = param1;
		break;
	//#NT#2012/08/22#Hideo Lin -end

	//#NT#2012/08/24#Hideo Lin -begin
	//#NT#For customized user data
	case MEDIAWRITE_SETINFO_CUSTOM_UDATA:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_CUSTOM_UDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].bCustomUdata = (param1 == TRUE) ? TRUE : FALSE;
		break;
	//#NT#2012/08/24#Hideo Lin -end
	case MEDIAWRITE_SETINFO_MOVHDR_REVSIZE://2012/10/22 Meg
		gMovRecFileInfo[0].hdr_revSize = param1;
		break;

	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	case MEDIAWRITE_SETINFO_MOV_ROTATEINFO:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_CUSTOM_UDATA, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].uiRotateInfo = param1;
		break;
	//#NT#2013/04/17#Calvin Chang -end

	case MEDIAWRITE_SETINFO_DAR://2013/10/28
		id = param3;
		g_movTrack[id].uiDAR = param1;
		break;


	case MEDIAWRITE_SETINTO_MOVGPSENTRY:
		if (param2 == 0) {
			if (gMovContainerMaker.cbShowErrMsg) {
				(gMovContainerMaker.cbShowErrMsg)("ERROR!! set video entry p1=0x%lx, p2 = 0x%lx!\r\n", param1, param2);
			}
			break;
		}
		ptr = (MOV_Ientry *)param2;
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_MOVVIDEOENTRY, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		MOV_SetGPS_Entry(id, param1, ptr);

		break;
	case MEDIAWRITE_SETINFO_GPSBUFFER:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_GPSBUFFER, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		MOV_SetGPSEntryAddr(param1, param2, id);
		break;

	case MEDIAWRITE_SETINFO_GPSTAGBUFFER:
		if ((param1 == 0) && (param2 != 0)) {
			break;
		}
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_GPSTAGBUFFER, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		g_movTrack[id].gpstag_addr = param1;
		g_movTrack[id].gpstag_size = param2;
		g_movTrack[id].fileid = id;
		//DBG_DUMP("GPS addr=0x%lx, size=0x%lx\r\n", param1, param2);
		//move to pHdrObj->uiFre1Addr
		break;

	case MEDIAWRITE_SETINFO_MEDIAVERSION:
		gMediaVersionAddr = param1;
		break;

	case MEDIAWRITE_SETINFO_EN_FREABOX:
		id = param3;
		if (param1) {
			g_mov_frea_box[id] = 1;
		} else {
			g_mov_frea_box[id] = 0;
		}
		break;

	case MEDIAWRITE_SETINFO_EN_CO64:
		if (param1) {
			gMovCo64 = 1;
		} else {
			gMovCo64 = 0;
		}
		//DBG_DUMP("^R using co64=%d!\r\n",gMovCo64);
		break;

	case MEDIAWRITE_SETINFO_UTC_TIMEZONE:
		if (param1 == ATOM_USER) {
			g_mov_utc_timeoffset = param1;
		} else if (param1) {
			if (param2) { //is negative: UTC-N, need to add offset
				g_mov_utc_timeoffset = (INT32)param1 * 60 * 60;
			} else { //is positive: UTC+N, need to sub offset
				g_mov_utc_timeoffset = (~((INT32)param1 * 60 * 60) + 1);
			}
		} else {
			g_mov_utc_timeoffset = 0;
		}
		break;

	case MEDIAWRITE_SETINFO_UTC_TIMEINFO:
		if (param1 == 0) {
			DBG_MSG("^MEDIAWRITE_SETINFO_UTC_TIMESPACE param1 zero!\r\n");
			break;
		}
		p_timespace = (struct tm *)param1;
		id = param3;
		g_mov_utc_timeinfo.tm_year = p_timespace->tm_year;
		g_mov_utc_timeinfo.tm_mon  = p_timespace->tm_mon;
		g_mov_utc_timeinfo.tm_mday = p_timespace->tm_mday;
		g_mov_utc_timeinfo.tm_hour = p_timespace->tm_hour;
		g_mov_utc_timeinfo.tm_min  = p_timespace->tm_min;
		g_mov_utc_timeinfo.tm_sec  = p_timespace->tm_sec;
		DBG_IND("[%d] tm=(%d/%d/%d %d/%d/%d)\r\n", id,
			g_mov_utc_timeinfo.tm_year, g_mov_utc_timeinfo.tm_mon, g_mov_utc_timeinfo.tm_mday,
			g_mov_utc_timeinfo.tm_hour, g_mov_utc_timeinfo.tm_min, g_mov_utc_timeinfo.tm_sec);
		break;

	case MEDIAWRITE_SETINFO_MP4_TEMPHDR:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_MP4_TEMPHDR, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].uiTempHdrAdr = param1;
		break;
	case MEDIAWRITE_SETINFO_H265DESC:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_H265DESC, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (param2) {
			if (gMovRecFileInfo[id].encVideoCodec == MEDIAVIDENC_H265) {
				g_movh265len[id] = MovWriteLib_Make265VidDesc(param1, param2, id);
			}
		}
		break;
	case MEDIAWRITE_SETINFO_MOOV_LAYER1DATA:
		if (param2) {
			g_mov_moovlayer1adr = param1;
			g_mov_moovlayer1size = param2;
		} else {
			g_mov_moovlayer1adr = 0;
			g_mov_moovlayer1size = 0;
		}
		break;
	case MEDIAWRITE_SETINFO_COPYTEMPHDR:
		if (param3< MOVWRITER_COUNT)
		{
			if (gMovRecFileInfo[param3].uiTempHdrAdr) {
				UINT32 adr,siz;
			//DBG_DUMP("^M copy dst=0x%lx, src=0x%lx, size=0x%lx\r\n",gMovRecFileInfo[pobj->uiMakerID].uiTempHdrAdr, hdrAddr, pobj->uiHeaderSize);
					//memcpy((char *)gMovRecFileInfo[pobj->uiMakerID].uiTempHdrAdr, (char *)hdrAddr, pobj->uiHeaderSize);
				adr = gMovTempHDR[param3];
				siz = gMovTempHDRsize[param3];

				memcpy((char *)gMovRecFileInfo[param3].uiTempHdrAdr, (char *)adr, siz);
			}
		}
		break;
	//add for sub-stream -begin
	case MEDIAWRITE_SETINFO_SUB_ENABLE:
		id = param3;
		if (param1) {
			gMovRecFileInfo[id].SubVideoTrackEn = 1;
		} else {
			gMovRecFileInfo[id].SubVideoTrackEn = 0;
		}
		break;
	case MEDIAWRITE_SETINFO_SUB_WID_HEI:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_SUB_WID_HEI, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].SubImageWidth = param1;
		gMovRecFileInfo[id].SubImageHeight = param2;
		break;
	case MEDIAWRITE_SETINFO_SUB_VID_FRAME:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_SUB_VID_FRAME, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].SubvideoFrameCount = param1;
		break;
	case MEDIAWRITE_SETINFO_SUB_DURATION:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_SUB_DURATION, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].SubPlayDuration = param1;
		break;
	case MEDIAWRITE_SETINFO_SUB_VID_FR:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_SUB_VID_FR, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].SubVideoFrameRate = param1;
		break;
	case MEDIAWRITE_SETINFO_SUB_VIDEOTYPE:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_SUB_VIDEOTYPE, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		gMovRecFileInfo[id].SubencVideoCodec = param1;
		break;
	case MEDIAWRITE_SETINFO_SUB_DAR:
		id = param3;
		g_sub_movTrack[id].uiDAR = param1;
		break;
	case MEDIAWRITE_SETINFO_SUB_VIDEOENTRY:
		if (param2 == 0) {
			if (gMovContainerMaker.cbShowErrMsg) {
				(gMovContainerMaker.cbShowErrMsg)("ERROR!! set video entry p1=0x%lx, p2 = 0x%lx!\r\n", param1, param2);
			}
			break;
		}
		ptr = (MOV_Ientry *)param2;
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_SUB_VIDEOENTRY, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		MOV_Sub_SetIentry(id, param1, ptr);
		break;
	case MEDIAWRITE_SETINTO_SUB_H264DESC:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINTO_SUB_H264DESC, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (param2) {
			if (gMovRecFileInfo[id].SubencVideoCodec == MEDIAVIDENC_H264) { //2013/12/20 fixbug: MJPG+eCos hangs
				MOV_Sub_Make264VidDesc(param1, param2, id);
			}
		}
		break;
	case MEDIAWRITE_SETINFO_SUB_H265DESC:
		id = param3;
		chk_id = MOVWriteCheckID(MEDIAWRITE_SETINFO_SUB_H265DESC, id);
		if (chk_id != E_OK) {
			return E_PAR;
		}
		if (param2) {
			if (gMovRecFileInfo[id].SubencVideoCodec == MEDIAVIDENC_H265) {
				g_sub_movh265len[id] = MOV_Sub_Make265VidDesc(param1, param2, id);
			}
		}
		break;
	//add for sub-stream -end

#if 1	//add for meta -begin
	case MEDIAWRITE_SETINFO_METABUFFER:
		id = param3;
		MOV_SetMetaEntryAddr(param1, param2, id);
		g_movTrack[id].metatag_addr = param1;
		g_movTrack[id].metatag_size = param2;
		g_movTrack[id].metaid = id;
		break;

	case MEDIAWRITE_SETINFO_METATAGBUFFER:
		id = param3;
		g_mov_meta_num[id] = param1;
		break;

	case MEDIAWRITE_SETINTO_METAENTRY:
		if (param2 == 0) {
			break;
		}
		ptr = (MOV_Ientry *)param2;
		id = param3;
		MOV_SetMeta_Entry(id, param1, ptr);
		break;

	case MEDIAWRITE_SETINFO_METADATA:
		id = param3;
		break;

	case MEDIAWRITE_SETINFO_METASIGN:
		id = param3;
		//param2 as meta_idx
		g_mov_meta_entry_sign[id][param2] = param1;
		break;
#endif	//add for meta -end

	default:
		break;
	}
	return E_OK;
}

ER MovWriteLib_CustomizeFunc(UINT32 type, void *pobj)
{
	ER result = E_NOSPT;
	MAKEMOOV_INFO *pinfo;

	switch (type) {
	case MEDIAWRITELIB_CUSTOM_FUNC_IDXTLB_RECOVERY://fix one at the same time
		result = MOVRcvy_IndexTlb_Recovery(0);
		break;

	case MEDIAWRITELIB_CUSTOM_FUNC_MAKEMOOV:
		pinfo = (MAKEMOOV_INFO *)pobj;
		result = MOVMJPG_MakeMoov(pinfo->id, pinfo->pfileinfo);
		break;

	default:
		break;
	}
	return result;
}

UINT32 MovWriteLib_MakefrontFtyp(UINT32 outAddr, UINT32 uifType)
{
	UINT32 uiFtypSize, i;
	UINT8 *pFtyp, *ptr;
	if (uifType == MEDIA_FTYP_MP4) {
		uiFtypSize = MP4_FTYP_SIZE;
		pFtyp = g_mp4Ftyp;
	} else {
		uiFtypSize = MOV_FTYP_SIZE;
		pFtyp = g_movFtyp;
	}
	ptr = (UINT8 *)outAddr;
	for (i = 0; i < uiFtypSize; i++) {
		*(ptr + i) = *(pFtyp + i);
	}
	return uiFtypSize;
}

ER MovWriteLib_UpdateHeader(UINT32 id, void *pobj)
{
	MOVMJPG_UpdateHeader(id, &gMovRecFileInfo[id]);

	return E_OK;
}
void MovWriteLib_Make264VidDesc(UINT32 addr, UINT32 len, UINT32 id)
{
#if 0 // fixed to 1 SPS + 1 PPS

	UINT8 *pIn, *pProfile, i;
	MOV_Offset pos;

	pIn = (UINT8 *)addr;
	MOV_MakeAtomInit(g_264VidDesc);
	pos = MOV_TellOffset(0);

	MOV_WriteB32Bits(0, 0);//size
	MOV_WriteB32Tag(0, "avcC");
	MOV_WriteB8Bits(0, 0x01);//
	pIn += 4;
	pProfile = pIn;
	pIn++;
	MOV_WriteB8Bits(0, *pIn++);//
	MOV_WriteB8Bits(0, *pIn++);//
	MOV_WriteB8Bits(0, *pIn++);//
	MOV_WriteB16Bits(0, 0xFFE1);//
	MOV_WriteB16Bits(0, len - 12); //
	for (i = 0; i < len - 12; i++) {
		MOV_WriteB8Bits(0, *pProfile++);//
	}
	pProfile += 4; //jump 4 bytes
	MOV_WriteB8Bits(0, 0x01);//
	MOV_WriteB8Bits(0, 0x00);//
	MOV_WriteB8Bits(0, 0x04);//
	for (i = 0; i < 4; i++) {
		MOV_WriteB8Bits(0, *pProfile++);//
	}
	MOVWriteUpdateData(0, pos);

#elif 0 // modify to support 1 SPS + N PPS

#define H264_SPS        0x67
#define H264_PPS        0x68
#define H264_PPS_NUM    8

	UINT8       *pIn, *pSPS = 0, *pPPS[H264_PPS_NUM], *pSeekPos[H264_PPS_NUM + 1];
	UINT32      i, j;
	UINT32      uiSPSSize = 0, uiPPSNum = 0, uiPPSSize[H264_PPS_NUM] = {0}, uiPPSTotal = 0;
	UINT32      uiSeekSize[H264_PPS_NUM + 1] = {0}, uiSeekCount = 0;
	BOOL        bDataFound = FALSE;
	MOV_Offset  pos;

	pIn = (UINT8 *)addr;
	MOV_MakeAtomInit(g_264VidDesc);
	pos = MOV_TellOffset(0);

	// parsing H264 description
	i = 0;
	while (i < len) {
		// check start code (00, 00, 00, 01)
		if ((*(pIn + i) == 0) && (*(pIn + i + 1) == 0) && (*(pIn + i + 2) == 0) && (*(pIn + i + 3) == 1)) {
			i += 4;
			pSeekPos[uiSeekCount] = pIn + i; // store the position of the data after start code
			uiSeekCount++; // one start code was found
			bDataFound = TRUE;
		} else {
			i++;
			if (bDataFound) {
				uiSeekSize[uiSeekCount - 1]++; // increase the size of found data
			}
		}
	}

	for (i = 0; i < uiSeekCount; i++) {
		if (*pSeekPos[i] == H264_SPS) {
			pSPS = pSeekPos[i];
			uiSPSSize = uiSeekSize[i];
		} else if (*pSeekPos[i] == H264_PPS) {
			pPPS[uiPPSNum] = pSeekPos[i];
			uiPPSSize[uiPPSNum] = uiSeekSize[i];
			uiPPSNum++;
		}
	}

	MOV_WriteB32Bits(0, 0);             // size
	MOV_WriteB32Tag(0, "avcC");         // avcC box type
	MOV_WriteB8Bits(0, 0x01);           // version
	MOV_WriteB8Bits(0, *(pSPS + 1));    // AVC profile indication
	MOV_WriteB8Bits(0, *(pSPS + 2));    // profile compatibility
	MOV_WriteB8Bits(0, *(pSPS + 3));    // AVC level indication
	MOV_WriteB16Bits(0, 0xFFE1);        // 0xFF: NALU length, 0xE1: SPS number (lower 5-bit)
	MOV_WriteB16Bits(0, uiSPSSize);     // SPS size

	for (i = 0; i < uiSPSSize; i++) {
		MOV_WriteB8Bits(0, *(pSPS + i)); // SPS data
	}

	MOV_WriteB8Bits(0, uiPPSNum);       // PPS number

	for (i = 0; i < uiPPSNum; i++) {
		MOV_WriteB16Bits(0, uiPPSSize[i]); // PPS size

		for (j = 0; j < uiPPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pPPS[i] + j)); // PPS data
		}
	}

	MOVWriteUpdateData(0, pos);

#else // modify to support M SPS + N PPS

#define H264_SPS        0x67
#define H264_PPS        0x68
#define H264_SPS_NUM    4
#define H264_PPS_NUM    8

	UINT8       *pIn, *pSPS[H264_SPS_NUM] = {0}, *pPPS[H264_PPS_NUM] = {0}, *pSeekPos[H264_SPS_NUM + H264_PPS_NUM] = {0};
	UINT32      i, j;
	UINT32      uiSPSSize[H264_SPS_NUM] = {0}, uiSPSNum = 0;
	UINT32      uiPPSSize[H264_PPS_NUM] = {0}, uiPPSNum = 0;
	UINT32      uiSeekSize[H264_SPS_NUM + H264_PPS_NUM] = {0}, uiSeekCount = 0;
	BOOL        bDataFound = FALSE;
	MOV_Offset  pos;
	char        *pchar;

	pIn = (UINT8 *)addr;
	pchar = g_264VidDesc[id];
	MOV_MakeAtomInit((void *)pchar);
	pos = MOV_TellOffset(0);

	// parsing H264 description
	i = 0;
	while (i < len) {
		// check start code (00, 00, 00, 01)
		if ((*(pIn + i) == 0) && (*(pIn + i + 1) == 0) && (*(pIn + i + 2) == 0) && (*(pIn + i + 3) == 1)) {
			i += 4;
			pSeekPos[uiSeekCount] = pIn + i; // store the position of the data after start code
			uiSeekCount++; // one start code was found
			bDataFound = TRUE;
		} else {
			i++;
			if (bDataFound) {
				uiSeekSize[uiSeekCount - 1]++; // increase the size of found data
			}
		}
	}
	//DBGD(uiSeekCount);
	for (i = 0; i < uiSeekCount; i++) {
		if (*pSeekPos[i] == H264_SPS) {
			pSPS[uiSPSNum] = pSeekPos[i];
			uiSPSSize[uiSPSNum] = uiSeekSize[i];
			uiSPSNum++;
		} else if (*pSeekPos[i] == H264_PPS) {
			pPPS[uiPPSNum] = pSeekPos[i];
			uiPPSSize[uiPPSNum] = uiSeekSize[i];
			uiPPSNum++;
		}
	}
	if (uiSPSNum == 0) {
		DBG_IND("^R no SPS, ignore\r\n");
		return;
	}
	//DBG_DUMP("ste1ok\r\n");
	uiSPSNum &= 0x1F; // only lower 5-bit

	MOV_WriteB32Bits(0, 0);             // size
	MOV_WriteB32Tag(0, "avcC");         // avcC box type
	MOV_WriteB8Bits(0, 0x01);           // version
	MOV_WriteB8Bits(0, *(pSPS[0] + 1)); // AVC profile indication
	MOV_WriteB8Bits(0, *(pSPS[0] + 2)); // profile compatibility
	MOV_WriteB8Bits(0, *(pSPS[0] + 3)); // AVC level indication
	MOV_WriteB8Bits(0, 0xFF);           // NALU length: 4 (lower 2-bit + 1)
	MOV_WriteB8Bits(0, 0xE0 | uiSPSNum); // SPS number (lower 5-bit)

	for (i = 0; i < uiSPSNum; i++) {
		MOV_WriteB16Bits(0, uiSPSSize[i]); // SPS size

		for (j = 0; j < uiSPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pSPS[i] + j)); // SPS data
		}
	}

	MOV_WriteB8Bits(0, uiPPSNum);       // PPS number

	for (i = 0; i < uiPPSNum; i++) {
		MOV_WriteB16Bits(0, uiPPSSize[i]); // PPS size

		for (j = 0; j < uiPPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pPPS[i] + j)); // PPS data
		}
	}

	MOVWriteUpdateData(0, pos);

#endif
}

UINT32 MovWriteLib_Make265VidDesc(UINT32 addr, UINT32 len, UINT32 id)
{
// modify to support M SPS + N PPS

#define H265_VPS        0x40
#define H265_SPS        0x42
#define H265_PPS        0x44
#define H265_VPS_NUM    3
#define H265_SPS_NUM    3
#define H265_PPS_NUM    3

	UINT8       *pIn, *pVPS[H265_VPS_NUM], *pSPS[H265_SPS_NUM], *pPPS[H265_PPS_NUM], *pSeekPos[H265_VPS_NUM + H265_SPS_NUM + H265_PPS_NUM];
	UINT32      i, j;
	UINT32      uiVPSSize[H265_VPS_NUM] = {0}, uiVPSNum = 0;
	UINT32      uiSPSSize[H265_SPS_NUM] = {0}, uiSPSNum = 0;
	UINT32      uiPPSSize[H265_PPS_NUM] = {0}, uiPPSNum = 0;
	UINT32      uiSeekSize[H265_VPS_NUM + H265_SPS_NUM + H265_PPS_NUM] = {0}, uiSeekCount = 0;
	BOOL        bDataFound = FALSE;
	MOV_Offset  desclen;
	char        *pchar;

	pIn = (UINT8 *)addr;
	pchar = g_265VidDesc[id];
	if (len > MEDIAWRITE_H265_STSD_MAXLEN) {
		DBG_DUMP("^R H265 DESC len too BIG!!!0x%lx\r\n", len);
		return 0;
	}
	MOV_MakeAtomInit((void *)pchar);
	//pchar2 = (char *)addr;
	//for (i=0;i<len;i++)
	//{
	//  if (i%8==0)
	//  {
	//      DBG_DUMP("\r\n");
	//  }
	//  DBG_DUMP(" [0x%02x] ", *(pchar2+i));
	//
	//}

	// parsing H264 description
	i = 0;
	while (i < len) {
		// check start code (00, 00, 00, 01)
		if ((*(pIn + i) == 0) && (*(pIn + i + 1) == 0) && (*(pIn + i + 2) == 0) && (*(pIn + i + 3) == 1)) {
			i += 4;
			pSeekPos[uiSeekCount] = pIn + i; // store the position of the data after start code
			uiSeekCount++; // one start code was found
			bDataFound = TRUE;
		} else {
			i++;
			if (bDataFound) {
				uiSeekSize[uiSeekCount - 1]++; // increase the size of found data
			}
		}
	}
	//DBGD(uiSeekCount);
	for (i = 0; i < uiSeekCount; i++) {
		if (*pSeekPos[i] == H265_SPS) {
			pSPS[uiSPSNum] = pSeekPos[i];
			//DBG_DUMP("sps pos=%d\r\n", i);
			uiSPSSize[uiSPSNum] = uiSeekSize[i];
			uiSPSNum++;
		} else if (*pSeekPos[i] == H265_PPS) {
			pPPS[uiPPSNum] = pSeekPos[i];
			//DBG_DUMP("pps pos=%d\r\n", i);
			uiPPSSize[uiPPSNum] = uiSeekSize[i];
			uiPPSNum++;
		} else if (*pSeekPos[i] == H265_VPS) {
			pVPS[uiVPSNum] = pSeekPos[i];
			//DBG_DUMP("vps pos=%d\r\n", i);
			uiVPSSize[uiVPSNum] = uiSeekSize[i];
			uiVPSNum++;
		}
	}
	//DBG_DUMP("ste1ok, vpsnum=%d, %d, %d\r\n", uiVPSNum,uiSPSNum,uiPPSNum);
	//DBG_DUMP("len, vpslen=0x%x, %x, %x\r\n", uiVPSSize[0],uiSPSSize[0],uiPPSSize[0]);

	//now 1, MOV_WriteB16Bits(0, uiVPSNum);       // VPS number
#if 1  //H265 MULTI-TILE
	MOV_WriteB8Bits(0, 0x20); // VPS
	MOV_WriteB16Bits(0, uiVPSNum); // VPS number (lower 5-bit)
#endif //H265 MULTI-TILE

	for (i = 0; i < uiVPSNum; i++) {
		MOV_WriteB16Bits(0, uiVPSSize[i]); // VPS size

		for (j = 0; j < uiVPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pVPS[i] + j)); // VPS data
		}
	}

	MOV_WriteB8Bits(0, 0x21); // SPS
	MOV_WriteB16Bits(0, uiSPSNum); // SPS number (lower 5-bit)

	for (i = 0; i < uiSPSNum; i++) {
		MOV_WriteB16Bits(0, uiSPSSize[i]); // SPS size

		for (j = 0; j < uiSPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pSPS[i] + j)); // SPS data
		}
	}

	MOV_WriteB8Bits(0, 0x22); // PPS
	MOV_WriteB16Bits(0, uiPPSNum);       // PPS number

	for (i = 0; i < uiPPSNum; i++) {
		MOV_WriteB16Bits(0, uiPPSSize[i]); // PPS size

		for (j = 0; j < uiPPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pPPS[i] + j)); // PPS data
		}
	}
	desclen = MOV_TellOffset(0);
	//DBG_DUMP("^M 5555pos=0x%lx, len=0x%lx.\r\n", pchar, desclen);
	return desclen;

}

//add for sub-stream -begin
void MOV_Sub_Make264VidDesc(UINT32 addr, UINT32 len, UINT32 id)
{
#if 0 // fixed to 1 SPS + 1 PPS

	UINT8 *pIn, *pProfile, i;
	MOV_Offset pos;

	pIn = (UINT8 *)addr;
	MOV_MakeAtomInit(g_264VidDesc);
	pos = MOV_TellOffset(0);

	MOV_WriteB32Bits(0, 0);//size
	MOV_WriteB32Tag(0, "avcC");
	MOV_WriteB8Bits(0, 0x01);//
	pIn += 4;
	pProfile = pIn;
	pIn++;
	MOV_WriteB8Bits(0, *pIn++);//
	MOV_WriteB8Bits(0, *pIn++);//
	MOV_WriteB8Bits(0, *pIn++);//
	MOV_WriteB16Bits(0, 0xFFE1);//
	MOV_WriteB16Bits(0, len - 12); //
	for (i = 0; i < len - 12; i++) {
		MOV_WriteB8Bits(0, *pProfile++);//
	}
	pProfile += 4; //jump 4 bytes
	MOV_WriteB8Bits(0, 0x01);//
	MOV_WriteB8Bits(0, 0x00);//
	MOV_WriteB8Bits(0, 0x04);//
	for (i = 0; i < 4; i++) {
		MOV_WriteB8Bits(0, *pProfile++);//
	}
	MOVWriteUpdateData(0, pos);

#elif 0 // modify to support 1 SPS + N PPS

#define H264_SPS        0x67
#define H264_PPS        0x68
#define H264_PPS_NUM    8

	UINT8       *pIn, *pSPS = 0, *pPPS[H264_PPS_NUM], *pSeekPos[H264_PPS_NUM + 1];
	UINT32      i, j;
	UINT32      uiSPSSize = 0, uiPPSNum = 0, uiPPSSize[H264_PPS_NUM] = {0}, uiPPSTotal = 0;
	UINT32      uiSeekSize[H264_PPS_NUM + 1] = {0}, uiSeekCount = 0;
	BOOL        bDataFound = FALSE;
	MOV_Offset  pos;

	pIn = (UINT8 *)addr;
	MOV_MakeAtomInit(g_264VidDesc);
	pos = MOV_TellOffset(0);

	// parsing H264 description
	i = 0;
	while (i < len) {
		// check start code (00, 00, 00, 01)
		if ((*(pIn + i) == 0) && (*(pIn + i + 1) == 0) && (*(pIn + i + 2) == 0) && (*(pIn + i + 3) == 1)) {
			i += 4;
			pSeekPos[uiSeekCount] = pIn + i; // store the position of the data after start code
			uiSeekCount++; // one start code was found
			bDataFound = TRUE;
		} else {
			i++;
			if (bDataFound) {
				uiSeekSize[uiSeekCount - 1]++; // increase the size of found data
			}
		}
	}

	for (i = 0; i < uiSeekCount; i++) {
		if (*pSeekPos[i] == H264_SPS) {
			pSPS = pSeekPos[i];
			uiSPSSize = uiSeekSize[i];
		} else if (*pSeekPos[i] == H264_PPS) {
			pPPS[uiPPSNum] = pSeekPos[i];
			uiPPSSize[uiPPSNum] = uiSeekSize[i];
			uiPPSNum++;
		}
	}

	MOV_WriteB32Bits(0, 0);             // size
	MOV_WriteB32Tag(0, "avcC");         // avcC box type
	MOV_WriteB8Bits(0, 0x01);           // version
	MOV_WriteB8Bits(0, *(pSPS + 1));    // AVC profile indication
	MOV_WriteB8Bits(0, *(pSPS + 2));    // profile compatibility
	MOV_WriteB8Bits(0, *(pSPS + 3));    // AVC level indication
	MOV_WriteB16Bits(0, 0xFFE1);        // 0xFF: NALU length, 0xE1: SPS number (lower 5-bit)
	MOV_WriteB16Bits(0, uiSPSSize);     // SPS size

	for (i = 0; i < uiSPSSize; i++) {
		MOV_WriteB8Bits(0, *(pSPS + i)); // SPS data
	}

	MOV_WriteB8Bits(0, uiPPSNum);       // PPS number

	for (i = 0; i < uiPPSNum; i++) {
		MOV_WriteB16Bits(0, uiPPSSize[i]); // PPS size

		for (j = 0; j < uiPPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pPPS[i] + j)); // PPS data
		}
	}

	MOVWriteUpdateData(0, pos);

#else // modify to support M SPS + N PPS

#define H264_SPS        0x67
#define H264_PPS        0x68
#define H264_SPS_NUM    4
#define H264_PPS_NUM    8

	UINT8       *pIn, *pSPS[H264_SPS_NUM] = {0}, *pPPS[H264_PPS_NUM] = {0}, *pSeekPos[H264_SPS_NUM + H264_PPS_NUM] = {0};
	UINT32      i, j;
	UINT32      uiSPSSize[H264_SPS_NUM] = {0}, uiSPSNum = 0;
	UINT32      uiPPSSize[H264_PPS_NUM] = {0}, uiPPSNum = 0;
	UINT32      uiSeekSize[H264_SPS_NUM + H264_PPS_NUM] = {0}, uiSeekCount = 0;
	BOOL        bDataFound = FALSE;
	MOV_Offset  pos;
	char        *pchar;

	pIn = (UINT8 *)addr;
	pchar = g_sub_264VidDesc[id];
	MOV_MakeAtomInit((void *)pchar);
	pos = MOV_TellOffset(0);

	// parsing H264 description
	i = 0;
	while (i < len) {
		// check start code (00, 00, 00, 01)
		if ((*(pIn + i) == 0) && (*(pIn + i + 1) == 0) && (*(pIn + i + 2) == 0) && (*(pIn + i + 3) == 1)) {
			i += 4;
			pSeekPos[uiSeekCount] = pIn + i; // store the position of the data after start code
			uiSeekCount++; // one start code was found
			bDataFound = TRUE;
		} else {
			i++;
			if (bDataFound) {
				uiSeekSize[uiSeekCount - 1]++; // increase the size of found data
			}
		}
	}
	//DBGD(uiSeekCount);
	for (i = 0; i < uiSeekCount; i++) {
		if (*pSeekPos[i] == H264_SPS) {
			pSPS[uiSPSNum] = pSeekPos[i];
			uiSPSSize[uiSPSNum] = uiSeekSize[i];
			uiSPSNum++;
		} else if (*pSeekPos[i] == H264_PPS) {
			pPPS[uiPPSNum] = pSeekPos[i];
			uiPPSSize[uiPPSNum] = uiSeekSize[i];
			uiPPSNum++;
		}
	}
	if (uiSPSNum == 0) {
		DBG_IND("^R no SPS, ignore\r\n");
		return;
	}
	//DBG_DUMP("ste1ok\r\n");
	uiSPSNum &= 0x1F; // only lower 5-bit

	MOV_WriteB32Bits(0, 0);             // size
	MOV_WriteB32Tag(0, "avcC");         // avcC box type
	MOV_WriteB8Bits(0, 0x01);           // version
	MOV_WriteB8Bits(0, *(pSPS[0] + 1)); // AVC profile indication
	MOV_WriteB8Bits(0, *(pSPS[0] + 2)); // profile compatibility
	MOV_WriteB8Bits(0, *(pSPS[0] + 3)); // AVC level indication
	MOV_WriteB8Bits(0, 0xFF);           // NALU length: 4 (lower 2-bit + 1)
	MOV_WriteB8Bits(0, 0xE0 | uiSPSNum); // SPS number (lower 5-bit)

	for (i = 0; i < uiSPSNum; i++) {
		MOV_WriteB16Bits(0, uiSPSSize[i]); // SPS size

		for (j = 0; j < uiSPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pSPS[i] + j)); // SPS data
		}
	}

	MOV_WriteB8Bits(0, uiPPSNum);       // PPS number

	for (i = 0; i < uiPPSNum; i++) {
		MOV_WriteB16Bits(0, uiPPSSize[i]); // PPS size

		for (j = 0; j < uiPPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pPPS[i] + j)); // PPS data
		}
	}

	MOVWriteUpdateData(0, pos);

#endif
}

UINT32 MOV_Sub_Make265VidDesc(UINT32 addr, UINT32 len, UINT32 id)
{
// modify to support M SPS + N PPS

#define H265_VPS        0x40
#define H265_SPS        0x42
#define H265_PPS        0x44
#define H265_VPS_NUM    3
#define H265_SPS_NUM    3
#define H265_PPS_NUM    3

	UINT8       *pIn, *pVPS[H265_VPS_NUM], *pSPS[H265_SPS_NUM], *pPPS[H265_PPS_NUM], *pSeekPos[H265_VPS_NUM + H265_SPS_NUM + H265_PPS_NUM];
	UINT32      i, j;
	UINT32      uiVPSSize[H265_VPS_NUM] = {0}, uiVPSNum = 0;
	UINT32      uiSPSSize[H265_SPS_NUM] = {0}, uiSPSNum = 0;
	UINT32      uiPPSSize[H265_PPS_NUM] = {0}, uiPPSNum = 0;
	UINT32      uiSeekSize[H265_VPS_NUM + H265_SPS_NUM + H265_PPS_NUM] = {0}, uiSeekCount = 0;
	BOOL        bDataFound = FALSE;
	MOV_Offset  desclen;
	char        *pchar;

	pIn = (UINT8 *)addr;
	pchar = g_sub_265VidDesc[id];
	if (len > MEDIAWRITE_H265_STSD_MAXLEN) {
		DBG_DUMP("^R H265 DESC len too BIG!!!0x%lx\r\n", len);
		return 0;
	}
	MOV_MakeAtomInit((void *)pchar);
	//pchar2 = (char *)addr;
	//for (i=0;i<len;i++)
	//{
	//  if (i%8==0)
	//  {
	//      DBG_DUMP("\r\n");
	//  }
	//  DBG_DUMP(" [0x%02x] ", *(pchar2+i));
	//
	//}

	// parsing H264 description
	i = 0;
	while (i < len) {
		// check start code (00, 00, 00, 01)
		if ((*(pIn + i) == 0) && (*(pIn + i + 1) == 0) && (*(pIn + i + 2) == 0) && (*(pIn + i + 3) == 1)) {
			i += 4;
			pSeekPos[uiSeekCount] = pIn + i; // store the position of the data after start code
			uiSeekCount++; // one start code was found
			bDataFound = TRUE;
		} else {
			i++;
			if (bDataFound) {
				uiSeekSize[uiSeekCount - 1]++; // increase the size of found data
			}
		}
	}
	//DBGD(uiSeekCount);
	for (i = 0; i < uiSeekCount; i++) {
		if (*pSeekPos[i] == H265_SPS) {
			pSPS[uiSPSNum] = pSeekPos[i];
			//DBG_DUMP("sps pos=%d\r\n", i);
			uiSPSSize[uiSPSNum] = uiSeekSize[i];
			uiSPSNum++;
		} else if (*pSeekPos[i] == H265_PPS) {
			pPPS[uiPPSNum] = pSeekPos[i];
			//DBG_DUMP("pps pos=%d\r\n", i);
			uiPPSSize[uiPPSNum] = uiSeekSize[i];
			uiPPSNum++;
		} else if (*pSeekPos[i] == H265_VPS) {
			pVPS[uiVPSNum] = pSeekPos[i];
			//DBG_DUMP("vps pos=%d\r\n", i);
			uiVPSSize[uiVPSNum] = uiSeekSize[i];
			uiVPSNum++;
		}
	}
	//DBG_DUMP("ste1ok, vpsnum=%d, %d, %d\r\n", uiVPSNum,uiSPSNum,uiPPSNum);
	//DBG_DUMP("len, vpslen=0x%x, %x, %x\r\n", uiVPSSize[0],uiSPSSize[0],uiPPSSize[0]);

	//now 1, MOV_WriteB16Bits(0, uiVPSNum);       // VPS number
#if 1  //H265 MULTI-TILE
	MOV_WriteB8Bits(0, 0x20); // VPS
	MOV_WriteB16Bits(0, uiVPSNum); // VPS number (lower 5-bit)
#endif //H265 MULTI-TILE

	for (i = 0; i < uiVPSNum; i++) {
		MOV_WriteB16Bits(0, uiVPSSize[i]); // VPS size

		for (j = 0; j < uiVPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pVPS[i] + j)); // VPS data
		}
	}

	MOV_WriteB8Bits(0, 0x21); // SPS
	MOV_WriteB16Bits(0, uiSPSNum); // SPS number (lower 5-bit)

	for (i = 0; i < uiSPSNum; i++) {
		MOV_WriteB16Bits(0, uiSPSSize[i]); // SPS size

		for (j = 0; j < uiSPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pSPS[i] + j)); // SPS data
		}
	}

	MOV_WriteB8Bits(0, 0x22); // PPS
	MOV_WriteB16Bits(0, uiPPSNum);       // PPS number

	for (i = 0; i < uiPPSNum; i++) {
		MOV_WriteB16Bits(0, uiPPSSize[i]); // PPS size

		for (j = 0; j < uiPPSSize[i]; j++) {
			MOV_WriteB8Bits(0, *(pPPS[i] + j)); // PPS data
		}
	}
	desclen = MOV_TellOffset(0);
	//DBG_DUMP("^M 5555pos=0x%lx, len=0x%lx.\r\n", pchar, desclen);
	return desclen;

}
//add for sub-stream -end

UINT32 Mov_Swap(UINT32 value)
{
	UINT32 temp = 0;
	temp = (value & 0xff) << 24;
	temp |= (value & 0xFF00) << 8;
	temp |= (value & 0xFF0000) >> 8;
	temp |= (value & 0xFF000000) >> 24;
	return temp;

}

UINT32 MOVWrite_TransRoundVidFrameRate(UINT32 input)
{
	UINT32 roundVfr = 30;
	if (input < 1000) { //12/15/24/30/48/60/120
		roundVfr = input;
	} else if (input < 1201) {
		roundVfr = 12;
	} else if (input < 1501) {
		roundVfr = 15;
	} else if (input < 2401) {
		roundVfr = 24;
	} else if (input < 2801) {
		roundVfr = 28;
	} else if (input < 3001) {
		roundVfr = 30;
	} else if (input < 4801) {
		roundVfr = 48;
	} else if (input < 5501) {
		roundVfr = 55;
	} else if (input < 6001) {
		roundVfr = 60;
	} else if (input < 12001) {
		roundVfr = 120;
	} else if (input < 24001) {
		roundVfr = 240;
	}
	return roundVfr;
}
#if 0
UINT32 MovWriteLib_Temp_WriteLib(void)
{
	UINT32 size;
	size = MOVMJPG_Temp_WriteMoov(&gMovRecFileInfo);

	return size;
}
#endif

#if defined (__UITRON) || defined (__ECOS)
NVTVER_VERSION_ENTRY(MP_MovWriteLib, 1, 00, 033, 00)//2.00.009
#else
VOS_MODULE_VERSION(MP_MovWriteLib, 1, 00, 023, 00);
#endif

