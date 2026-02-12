/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       MOVWriter.c
    @ingroup    mIAVMOV

    @brief      MOV file writer
                It's the MOV file writer, support multi/single payload now

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
//#include "FileSysTsk.h"//2012/07/13 Meg
#include "MediaWriteLib.h"
#include "AVFile_MakerMov.h"
#include "MOVLib.h"
#include "movRec.h"
#include "HwClock.h"
#include "Debug.h"
#include "MovWriteLib.h"
#include "FileSysTsk.h"
#include "HwMem.h"

#define __MODULE__          MOVWrite
#define __DBGLVL__          1  // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
#else
#include <string.h>
#include "kwrap/perf.h"
#include "kwrap/type.h"
#include "FileSysTsk.h"

#define __MODULE__          MOVWrite
#define __DBGLVL__          2
#include "kwrap/debug.h"
unsigned int MOVWrite_debug_level = NVT_DBG_WRN;

#include "avfile/MediaWriteLib.h"
#include "avfile/AVFile_MakerMov.h"
#include "avfile/MOVLib.h"

// INCLUDE PROTECTED
#include "movRec.h"
#include "MovWriteLib.h"
#include <time.h>
#include <comm/hwclock.h>

#define AVFILE_TODO ENABLE
#endif

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions & Types Declarations                             */
/*-----------------------------------------------------------------------------*/
#define MOV_WITHVIDEO  1

UINT32 gMovEndfileWay = 1;

UINT32      gmovw_mustmsg = 0;
#define NMR_MOVMUST(fmtstr, args...) do { if(gmovw_mustmsg) DBG_DUMP(fmtstr, ##args); } while(0)

//static UINT8*                       gpMOVBSOutBaseAddr  = NULL;
//#NT#2007/09/06#Meg Lin -begin
//fixbug: if video > audio, add pure audio frame
//#NT#2007/09/06#Meg Lin -end

//add for sub-stream -begin
MOVREC_Track                        g_sub_movTrack[MOVWRITER_COUNT] = {0};
MOVREC_Context                      g_sub_movCon[MOVWRITER_COUNT];
MOV_Ientry                          *gp_sub_movEntry;
UINT32                              g_sub_movEntryMax[MOVWRITER_COUNT];
UINT32                              g_sub_movEntryAddr[MOVWRITER_COUNT];
//add for sub-stream -end

MOVMJPG_FILEINFO                    gMovRecFileInfo[MOVWRITER_COUNT] = {0};
UINT32                              guiAudioPacketIndex = 0;
UINT32                              gMovTempHDR[MOVWRITER_COUNT] = {0};
UINT32                              gMovTempHDRsize[MOVWRITER_COUNT] = {0};
MOVREC_Track                        g_movTrack[MOVWRITER_COUNT] = {0}, g_movAudioTrack[MOVWRITER_COUNT] = {0};
MOVREC_Context                      g_movCon[MOVWRITER_COUNT], g_movAudioCon[MOVWRITER_COUNT];

MOV_Ientry                          *gp_movEntry;
MOV_Ientry                          *gp_movAudioEntry;
//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
MOVREC_EntryPos                     g_entry_pos[MOVWRITER_COUNT] = {0};
//#NT#2019/11/22#Willy Su -end
UINT32                              g_movEntryMax[MOVWRITER_COUNT];
UINT32                              g_movAudioEntryMax[MOVWRITER_COUNT];
UINT32                              g_movGPSEntryMax[MOVWRITER_COUNT];
UINT32                              g_movEntrySize[MOVWRITER_COUNT];
UINT32                              g_movEntryAddr[MOVWRITER_COUNT];
UINT32                              g_movAudEntryAddr[MOVWRITER_COUNT];
UINT32                              g_movGPSEntryAddr[MOVWRITER_COUNT] = {0};
UINT32                              g_movGPSEntrySize[MOVWRITER_COUNT] = {0};
UINT32                              g_movGPSEntryTotal[MOVWRITER_COUNT] = {0};
char                                g_movMJPGCodecName[0xe] = {'M', 'o', 't', 'i', 'o', 'n', ' ', 'J',
															   'P', 'E', 'G', ' ', 'A', 0

															  };
char                                g_movH264CodecName[0x5] = {'h', '2', '6', '4', 0};
char                                g_movH265CodecName[0x5] = {'h', '2', '6', '5', 0};
UINT8                               gMovSeamlessCut = 0, gMovCo64 = 0;
UINT32                              g_mov_nidx_timeid[MOVWRITER_COUNT] = {0};
UINT8                               g_mov_sync_pos = 0;
INT32                               g_mov_utc_timeoffset = 0;
struct tm                           g_mov_utc_timeinfo = {0};
BOOL                                g_mov_frea_box[MOVWRITER_COUNT] = {0};
//for mov recovery -begin
//char    gMov264VidDesc[0x40];
/*
UINT32          gMovRecvy_nidxNum     = 0;
UINT32          gMovRecvy_Back1sthead = 0;
UINT32          gMovRecvy_CluReadAddr = 0;
UINT32          gMovRecvy_entryAddr   = 0;
UINT32          gMovRecvy_entrySize   = 0;
UINT32          gMovRecvy_newMoovAddr = 0;
UINT32          gMovRecvy_newMoovSize = 0;
UINT32          gMovRecvyVidEntryMax  = 0;
UINT32          gMovRecvyAudEntryMax  = 0;
UINT32          gMovRecvy_totalVnum   = 0;
UINT32          gMovRecvy_audTotalSize = 0;
UINT32          gMovRecvy_vfr = 0, gMovRecvy_vidWid = 0;
UINT32          gMovRcvy_Hdrsize = 0;//2012/09/11 Meg Lin
*/
//for mov recovery -end

#if 1	//add for meta -begin
UINT32                              g_mov_meta_num[MOVWRITER_COUNT] = {0};
UINT32                              g_mov_meta_entry_sign[MOVWRITER_COUNT][MOV_META_COUNT] = {0};
UINT32                              g_mov_meta_entry_max[MOVWRITER_COUNT][MOV_META_COUNT] = {0};
UINT32                              g_mov_meta_entry_addr[MOVWRITER_COUNT][MOV_META_COUNT] = {0};
UINT32                              g_mov_meta_entry_size[MOVWRITER_COUNT][MOV_META_COUNT] = {0};
UINT32                              g_mov_meta_entry_total[MOVWRITER_COUNT][MOV_META_COUNT] = {0};
#endif	//add for meta -end


extern char        g_264VidDesc[MOVWRITER_COUNT][0x60];
//ER MOVRcvy_GetNidxTbl(UINT32 id, UINT32 filesize);
//ER MOVRcvy_UpdateMdatHDR(UINT32 id);

/** \addtogroup mIAVMOV */
//@{

CONTAINERMAKER gMovContainerMaker = {
	MovWriteLib_Initialize, //Initialize
	MovWriteLib_SetMemBuf,  //SetMemBuf
	MovWriteLib_MakeHeader, //MakeHeader
	MovWriteLib_UpdateHeader, //UpdateHeader
	MovWriteLib_GetInfo,    //GetInfo
	MovWriteLib_SetInfo,    //SetInfo
	MovWriteLib_CustomizeFunc, //CustomizeFunc
	NULL,                   //cbWriteFile
	NULL,                   //cbReadFile
	NULL,                   //cbShowErrMsg
	MOVWRITELIB_CHECKID,           //checkID
	0, //id
};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

void mov_ResetContainerMaker(void)
{
	UINT8 i;
	for (i = 0; i < MOVWRITER_COUNT; i++) {
		//unused
	}
}

UINT32 gmov_setConMakerID = 0;
void mov_setConMakerId(UINT32 value)
{
	gmov_setConMakerID = value;
}

PCONTAINERMAKER mov_getContainerMaker(void)
{
	PCONTAINERMAKER pConMaker;
	UINT32 id;


	pConMaker = &gMovContainerMaker;
	id = gmov_setConMakerID;
    if (id>=MOVWRITER_COUNT) {
		 DBG_DUMP("^R ERR: %d getcontainerNULL\r\n", id);
         return NULL;
    }
	//#NT#2020/09/18#Willy Su -begin
	//#NT#Changed due to one-to-one mapping between maker id and mdat id
	pConMaker->id = id;
	//#NT#2020/09/18#Willy Su -end

/*
	for (i = 0; i < MOVWRITER_COUNT; i++) {
		if (gMovWriteUsed[i] == 0) {
			pConMaker->id = i;
			gMovWriteUsed[i] = 1;
			break;
		}

	}*/

	return pConMaker;
}


UINT8  gnowFirst = 0, goldFrist;

//#NT#2007/09/06#Meg Lin -end
#if 0//使用方式
RTC_DATE Date;
RTC_TIME Time;
Date = rtc_getDate();
Time = rtc_getTime();

movTimeSeconds = MOVWriteCountTotalSeconds(Date, Time);
g_movMoovCtime = movTimeSeconds + MOV_MDAT_BASETIME;
#endif
//因為歐洲的日光節約時間, 算出來的時間可能在夏季會有一小時的誤差
UINT32 MOVWriteCountTotalSeconds(void)//RTC_DATE date, RTC_TIME time)
{
#if AVFILE_TODO
	UINT32 temp, totalSec = 0;
	//RTC_DATE Date;
	//RTC_TIME Time;
    struct tm Curr_DateTime;
	if ((g_mov_utc_timeinfo.tm_mon != 0) && (g_mov_utc_timeinfo.tm_mday != 0)) {
		memcpy((void *)&Curr_DateTime, (void *)&g_mov_utc_timeinfo, sizeof(g_mov_utc_timeinfo));
	} else {
		Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
	}
	DBG_IND("year: %d, month : %d, day:%d!\r\n", Curr_DateTime.tm_year, Curr_DateTime.tm_mon, Curr_DateTime.tm_mday);
	DBG_IND("hour: %d, min : %d, sec:%d!\r\n", Curr_DateTime.tm_hour, Curr_DateTime.tm_min, Curr_DateTime.tm_sec);
	if (Curr_DateTime.tm_year < 1904) { //2015/07/22 fixbug: 1900.1.1 recording error
		DBG_DUMP("year ERROR!! %d", Curr_DateTime.tm_year);
		Curr_DateTime.tm_year = 2000;
		Curr_DateTime.tm_mon = 1;
		Curr_DateTime.tm_mday = 1;
		Curr_DateTime.tm_hour = 0;
		Curr_DateTime.tm_min = 0;
		Curr_DateTime.tm_sec = 0;
	}
	temp = Curr_DateTime.tm_year - 1904; // base 1904 1 1 0:0:0
	while (temp >= 1) { //temp = 2005, counttotalsec of 2004
		if (temp % 4 == 1) { //leap year
			totalSec += 0x1E28500;//366*24*60*60
		} else {
			totalSec += 0x1E13380;//365*24*60*60
		}
		temp -= 1;
	}
	temp = Curr_DateTime.tm_mon-1;
	while (temp) {
		switch (temp) {
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			totalSec += 0x28DE80;//31*24*60*60
			break;
		case 2:
			if (Curr_DateTime.tm_year % 4 == 0) {
				totalSec += 0x263B80;
			} else {
				totalSec += 0x24EA00;
			}
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			totalSec += 0x278D00;
			break;
		default:
			break;
		}
		temp -= 1;

	}
	DBG_IND("totalsec=0x%lx!!\r\n", totalSec);
	temp = Curr_DateTime.tm_mday - 1;
	totalSec += temp * 0x15180; //24*60*60
	temp = Curr_DateTime.tm_hour;
	totalSec += temp * 0xE10; //60*60
	temp = Curr_DateTime.tm_min;
	totalSec += temp * 60;
	totalSec += Curr_DateTime.tm_sec;

	if (g_mov_utc_timeoffset == ATOM_USER) {
		totalSec = 0;
	} else if (g_mov_utc_timeoffset != 0) {
		totalSec = (UINT32)((INT32)totalSec + g_mov_utc_timeoffset);
	}
#else
	UINT32 totalSec = 0;
	UINT32 temp;
	UINT32 tm_year = 2000;
	UINT32 tm_mon  = 1;
	UINT32 tm_mday = 1;
	UINT32 tm_hour = 0;
	UINT32 tm_min  = 0;
	UINT32 tm_sec  = 0;

	temp = tm_year - 1904; // base 1904 1 1 0:0:0
	while (temp >= 1) { //temp = 2005, counttotalsec of 2004
		if (temp % 4 == 1) { //leap year
			totalSec += 0x1E28500;//366*24*60*60
		} else {
			totalSec += 0x1E13380;//365*24*60*60
		}
		temp -= 1;
	}
	temp = tm_mon-1;
	totalSec += temp;
	temp = tm_mday - 1;
	totalSec += temp * 0x15180; //24*60*60
	temp = tm_hour;
	totalSec += temp * 0xE10; //60*60
	temp = tm_min;
	totalSec += temp * 60;
	totalSec += tm_sec;
#endif
	return totalSec;

}

//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
//return total size
UINT32 MOV_SyncPos(UINT32 id, UINT32 index, UINT32 bHead) //index as total_sec
{
	MOVREC_EntryPos *p_entry_pos = &(g_entry_pos[id]);
	MOV_Ientry *vid_entry = 0;
	MOV_Ientry *aud_entry = 0;
	UINT32 vid_num;
	UINT32 aud_num;
	UINT32 vid_frate = gMovRecFileInfo[id].VideoFrameRate;
	UINT32 aud_srate = gMovRecFileInfo[id].AudioSampleRate;
	UINT32 vid_need_num  = (index - 1) * vid_frate;
	UINT32 aud_need_num  = (index - 1) * aud_srate / 1024;
	UINT32 vid_shift_num = 0;//index * vid_frate;
	UINT32 aud_shift_num = 0;//index * aud_srate / 1024;
	#if 0
	UINT32 aud_shift_times = 1024 / (aud_srate % 1024);
	#endif
	UINT32 shift_num = 0;
	UINT64 vid_org_pos = 0, aud_org_pos = 0;
	UINT64 vid_new_pos = 0, aud_new_pos = 0;
	UINT64 org_pos = 0, new_pos = 0;
	UINT64 shift_size = 0;
	//UINT32 num;
	UINT32 i;

	DBG_IND("^G NEED vid %d aud %d\r\n", vid_need_num, aud_need_num);

	// Get vid num
	if (p_entry_pos->vid_tail > p_entry_pos->vid_head) {
		vid_num = p_entry_pos->vid_tail - p_entry_pos->vid_head;
	} else {
		MOV_Ientry *pEntry = 0;
		UINT32 temp, tempend;
		pEntry = (MOV_Ientry *)g_movEntryAddr[id];
		temp = p_entry_pos->vid_tail - pEntry;
		pEntry = (MOV_Ientry *)g_movEntryAddr[id] + g_movEntryMax[id];
		tempend = pEntry - p_entry_pos->vid_head + 1;
		vid_num = temp + tempend;
	}
	DBG_IND("^G vid num %d %d\r\n", vid_num, p_entry_pos->vid_num);

	// Get aud num
	if (p_entry_pos->aud_tail > p_entry_pos->aud_head) {
		aud_num = p_entry_pos->aud_tail - p_entry_pos->aud_head;
	} else {
		MOV_Ientry *pEntry = 0;
		UINT32 temp, tempend;
		pEntry = (MOV_Ientry *)g_movAudEntryAddr[id];
		temp = p_entry_pos->aud_tail - pEntry;
		pEntry = (MOV_Ientry *)g_movAudEntryAddr[id] + g_movAudioEntryMax[id];
		tempend = pEntry - p_entry_pos->aud_head + 1;
		aud_num = temp + tempend;
	}
	DBG_IND("^G aud num %d %d\r\n", aud_num, p_entry_pos->aud_num);

	DBG_IND("^G NOW vid %d aud %d\r\n", vid_num, aud_num);

	vid_shift_num = vid_num - vid_need_num;
	aud_shift_num = aud_num - aud_need_num;

	DBG_IND("^G SHIFT vid %d aud %d\r\n", vid_num, aud_num);

	// Get org_pos
	vid_entry = p_entry_pos->vid_head;
	aud_entry = p_entry_pos->aud_head;
	vid_org_pos = vid_entry->pos;
	aud_org_pos = aud_entry->pos;
	org_pos = (vid_org_pos > aud_org_pos) ? aud_org_pos : vid_org_pos;
	DBG_IND("^G org pos: %lld\r\n", org_pos);

	// Sync Vid Ientry
	shift_num = vid_shift_num;
	for (i = 0; i < shift_num; i++) {
		if (vid_entry == (MOV_Ientry *)g_movEntryAddr[id] + g_movEntryMax[id]) {
			vid_entry = (MOV_Ientry *)g_movEntryAddr[id];
			DBG_IND("^G VID BACK\r\n");
		} else {
			vid_entry++;
		}
		p_entry_pos->vid_num --;
	}
	p_entry_pos->vid_head = vid_entry;
	DBG_IND("^G vid head (%lld %d) at (0x%X)\r\n", vid_entry->pos, vid_entry->size, vid_entry);

	// Sync Aud Ientry
	shift_num = aud_shift_num;
	#if 0
	p_entry_pos->aud_shift++;
	if ((p_entry_pos->aud_shift % aud_shift_times) == 0) {
		shift_num += 1;
		p_entry_pos->aud_shift = 0;
	}
	#endif
	for (i = 0; i < shift_num; i++) {
		if (aud_entry == (MOV_Ientry *)g_movAudEntryAddr[id] + g_movAudioEntryMax[id]) {
			aud_entry = (MOV_Ientry *)g_movAudEntryAddr[id];
			DBG_IND("^G AUD BACK\r\n");
		} else {
			aud_entry++;
		}
		p_entry_pos->aud_num --;
	}
	p_entry_pos->aud_head = aud_entry;
	DBG_IND("^G aud head (%lld %d) at (0x%X)\r\n", aud_entry->pos, aud_entry->size, aud_entry);

	// Get new_pos
	vid_entry = p_entry_pos->vid_head;
	aud_entry = p_entry_pos->aud_head;
	vid_new_pos = vid_entry->pos;
	aud_new_pos = aud_entry->pos;
	new_pos = (vid_new_pos > aud_new_pos) ? aud_new_pos : vid_new_pos;
	DBG_IND("^G new pos: %lld\r\n", new_pos);

	// Get vid num
	if (p_entry_pos->vid_tail > p_entry_pos->vid_head) {
		vid_num = p_entry_pos->vid_tail - p_entry_pos->vid_head;
	} else {
		MOV_Ientry *pEntry = 0;
		UINT32 temp, tempend;
		pEntry = (MOV_Ientry *)g_movEntryAddr[id];
		temp = p_entry_pos->vid_tail - pEntry;
		pEntry = (MOV_Ientry *)g_movEntryAddr[id] + g_movEntryMax[id];
		tempend = pEntry - p_entry_pos->vid_head + 1;
		vid_num = temp + tempend;
	}
	DBG_IND("^G vid num %d %d\r\n", vid_num, p_entry_pos->vid_num);

	// Get aud num
	if (p_entry_pos->aud_tail > p_entry_pos->aud_head) {
		aud_num = p_entry_pos->aud_tail - p_entry_pos->aud_head;
	} else {
		MOV_Ientry *pEntry = 0;
		UINT32 temp, tempend;
		pEntry = (MOV_Ientry *)g_movAudEntryAddr[id];
		temp = p_entry_pos->aud_tail - pEntry;
		pEntry = (MOV_Ientry *)g_movAudEntryAddr[id] + g_movAudioEntryMax[id];
		tempend = pEntry - p_entry_pos->aud_head + 1;
		aud_num = temp + tempend;
	}
	DBG_IND("^G aud num %d %d\r\n", aud_num, p_entry_pos->aud_num);

	// Update Entry offset
	shift_size = (new_pos > org_pos) ? (new_pos - org_pos) : (org_pos - new_pos);
	DBG_IND("^G shift size %lld\r\n", shift_size);

	vid_entry = p_entry_pos->vid_head;
	for(i = 0; i < vid_num; i++) {
		vid_entry->pos -= shift_size;
		if (vid_entry == (MOV_Ientry *)g_movEntryAddr[id] + g_movEntryMax[id]) {
			vid_entry = (MOV_Ientry *)g_movEntryAddr[id];
		} else {
			vid_entry++;
		}
	}
	//vid_entry = p_entry_pos->vid_head;
	DBG_IND("^G vid head (%lld %d) at (0x%X)\r\n", vid_entry->pos, vid_entry->size, vid_entry);

	aud_entry = p_entry_pos->aud_head;
	for(i = 0; i < aud_num; i++) {
		aud_entry->pos -= shift_size;
		if (aud_entry == (MOV_Ientry *)g_movAudEntryAddr[id] + g_movAudioEntryMax[id]) {
			aud_entry = (MOV_Ientry *)g_movAudEntryAddr[id];
		} else {
			aud_entry++;
		}
	}
	//aud_entry = p_entry_pos->aud_head;
	DBG_IND("^G aud head (%lld %d) at (0x%X)\r\n", aud_entry->pos, aud_entry->size, aud_entry);

	return (UINT32)shift_size;
}
//#NT#2019/11/22#Willy Su -end

//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
void MOV_SetIentry_SyncPos(UINT32 id, UINT32 index, MOV_Ientry *ptr)
{
	MOVREC_EntryPos *p_entry_pos = &(g_entry_pos[id]);
	//MOV_Ientry *p_head = 0;
	MOV_Ientry *p_tail = 0;

	// Check Input
	if (NULL == ptr) {
		DBG_DUMP("[%d]SetIentry[%d] NULL\r\n", id, index);
		return ;
	}
	if (id >= MOVWRITER_COUNT) {
		DBG_DUMP("[%d]SetIentry[%d] Error\r\n", id, index);
		return ;
	}

	// Head & Tail
	//p_head = p_entry_pos->vid_head;
	p_tail = p_entry_pos->vid_tail;

	// New Entry
	if (ptr->updated) {
		p_tail->pos = ptr->pos;
		p_tail->size = ptr->size;
	} else {
		memcpy((UINT8 *)p_tail, (UINT8 *)ptr, sizeof(MOV_Ientry));
	}
	p_entry_pos->vid_num ++;

	// Update Head & Tail
	//p_entry_pos->vid_head = p_head;
	if (p_tail == (MOV_Ientry *)g_movEntryAddr[id] + g_movEntryMax[id]) {
		p_entry_pos->vid_tail = (MOV_Ientry *)g_movEntryAddr[id];
	} else {
		p_entry_pos->vid_tail = p_tail + 1;
	}

	return ;
}
//#NT#2019/11/22#Willy Su -end

//add for sub-stream -begin
void MOV_Sub_SetIentry(UINT32 id, UINT32 index, MOV_Ientry *ptr)
{
	MOV_Ientry      *pEntry = 0;

	if (g_sub_movEntryAddr[id] == 0) {
		return;
	}
	if (index > g_sub_movEntryMax[id]) {
		DBG_DUMP("id=%d index =%d big!!%ld!!\r\n", id, index, g_sub_movEntryMax[id]);
		DBG_DUMP("vid!!%ld!! sub!!%ld!!\r\n", g_movEntryMax[id], g_sub_movEntryMax[id]);
		return;
	}

	gp_sub_movEntry = (MOV_Ientry *)g_sub_movEntryAddr[id];
	pEntry = gp_sub_movEntry + index;
	if (pEntry->updated != 0) {
		pEntry->size = ptr->size;
		pEntry->key_frame = ptr->key_frame;
	} else { //if (pEntry->updated == 0)
		memcpy((UINT8 *)pEntry, (UINT8 *)ptr, sizeof(MOV_Ientry));
	}
	if (0) { //index <5)
		DBG_DUMP("^R[V%d][%d].pos=0x%llx,size =0x%lx!!\r\n", id, index, pEntry->pos, pEntry->size);
	}
}
//add for sub-stream -end

void MOV_SetIentry(UINT32 id, UINT32 index, MOV_Ientry *ptr)
{
	MOV_Ientry      *pEntry = 0;
	UINT32 fr, mod;
	if (g_movEntryAddr[id] == 0) {
		return;
	}
	if (index > g_movEntryMax[id]) {
		DBG_DUMP("id=%d index =%d big!!%ld!!\r\n", id, index, g_movEntryMax[id]);
		DBG_DUMP("vid!!%ld!! aud!!%ld!!\r\n", g_movEntryMax[id], g_movAudioEntryMax[id]);
		return;
	}

	//#NT#2011/01/07#Meg Lin -begin
	if (gMovSeamlessCut) {
		fr = gMovRecFileInfo[id].VideoFrameRate;
		mod = index % fr;
		index = gMovSeamlessCut * fr + mod;
		DBG_ERR("v%d[%d].p=0x%llx,s=0x%lx!\r\n", gMovSeamlessCut, index, ptr->pos, ptr->size);
	}
	//#NT#2011/01/07#Meg Lin -end
	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	pEntry = gp_movEntry + index;
	if (pEntry->updated != 0) {
		pEntry->size = ptr->size;
		pEntry->key_frame = ptr->key_frame;
	} else { //if (pEntry->updated == 0)
		memcpy((UINT8 *)pEntry, (UINT8 *)ptr, sizeof(MOV_Ientry));
	}
	if (0) { //index <5)
		DBG_DUMP("^R[V%d][%d].pos=0x%llx,size =0x%lx!!\r\n", id, index, pEntry->pos, pEntry->size);
	}
}

void MOV_SetIentry_old32(UINT32 id, UINT32 index, MOV_old32_Ientry *ptr)
{
	MOV_Ientry      *pEntry = 0;
	UINT32 fr, mod;
	if (g_movEntryAddr[id] == 0) {
		return;
	}
	if (index > g_movEntryMax[id]) {
		return;
	}

	//#NT#2011/01/07#Meg Lin -begin
	if (gMovSeamlessCut) {
		fr = gMovRecFileInfo[id].VideoFrameRate;
		mod = index % fr;
		index = gMovSeamlessCut * fr + mod;
		DBG_ERR("v%d[%d].p=0x%lx,s=0x%lx!\r\n", gMovSeamlessCut, index, ptr->pos, ptr->size);
	}
	if (0) { //index % 60 == 0)
		DBG_DUMP("Michael index %d, pos 0x%lx, g_movEntryAddr=0x%x\r\n", index, ptr->pos,
				 g_movEntryAddr[id]);
	}
	//#NT#2011/01/07#Meg Lin -end
	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	pEntry = gp_movEntry + index;
	if (pEntry->updated != 0) {
		pEntry->size = ptr->size;
		pEntry->key_frame = ptr->key_frame;
	} else { //if (pEntry->updated == 0)
		//memcpy((UINT8 *)pEntry, (UINT8 *)ptr, sizeof(MOV_Ientry));
		pEntry->pos = (UINT64)ptr->pos;
		pEntry->size = ptr->size;
		pEntry->key_frame = ptr->key_frame;
	}
	if (0) { //index <5)
		DBG_DUMP("^R[V%d][%d].pos=0x%llx,size =0x%lx!!\r\n", id, index, pEntry->pos, pEntry->size);
	}
}

void MOV_UpdateVidEntryPos(UINT32 id, UINT32 index, MOV_Ientry *ptr)
{
	MOV_Ientry      *pEntry = 0;
	if (g_movEntryAddr[id] == 0) {
		return;
	}
	if (index > g_movEntryMax[id]) {
		return;
	}

	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	pEntry = gp_movEntry + index;
	pEntry->pos = ptr->pos;
	pEntry->updated = 1;
	//DBG_DUMP("UV[%d].pos=0x%lx\r\n", index, pEntry->pos);

}

//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
void MOV_SetAudioIentry_SyncPos(UINT32 id, UINT32 index, MOV_Ientry *ptr)
{
	MOVREC_EntryPos *p_entry_pos = &(g_entry_pos[id]);
	//MOV_Ientry *p_head = 0;
	MOV_Ientry *p_tail = 0;

	// Check Entry
	if (NULL == ptr) {
		DBG_DUMP("[%d]SetIentry[%d] NULL\r\n", id, index);
		return ;
	}
	if (id >= MOVWRITER_COUNT) {
		DBG_DUMP("[%d]SetIentry[%d] Error\r\n", id, index);
		return ;
	}

	// Head & Tail
	//p_head = p_entry_pos->aud_head;
	p_tail = p_entry_pos->aud_tail;

	// New Entry
	if (ptr->updated) {
		p_tail->pos = ptr->pos;
		p_tail->size = ptr->size;
	} else {
		memcpy((UINT8 *)p_tail, (UINT8 *)ptr, sizeof(MOV_Ientry));
	}
	p_entry_pos->aud_num ++;

	// Update Head & Tail
	//p_entry_pos->aud_head = p_head;
	if (p_tail == (MOV_Ientry *)g_movAudEntryAddr[id] + g_movAudioEntryMax[id]) {
		p_entry_pos->aud_tail = (MOV_Ientry *)g_movAudEntryAddr[id];
	} else {
		p_entry_pos->aud_tail = p_tail + 1;
	}

	return ;
}
//#NT#2019/11/22#Willy Su -end

void MOV_SetAudioIentry(UINT32 id, UINT32 index, MOV_Ientry *ptr)
{
	MOV_Ientry      *pEntry = 0;

	//DBG_DUMP("[A%d][%d].pos=0x%lx,size =0x%lx!!\r\n", id, index, ptr->pos, ptr->size);

	if (g_movAudEntryAddr[id] == 0) {
		DBG_DUMP("g_movAudEntryAddr[%d] =0!!\r\n", id);
		return;
	}
	if (index > g_movAudioEntryMax[id]) {
		DBG_DUMP("id=%d index =%d big!!%ld!!\r\n", id, index, g_movAudioEntryMax[id]);
		DBG_DUMP("vid!!%ld!! aud!!%ld!!\r\n", g_movEntryMax[id], g_movAudioEntryMax[id]);
		return;
	}
	gp_movAudioEntry = (MOV_Ientry *)g_movAudEntryAddr[id];
	pEntry = gp_movAudioEntry + index;
	//DBG_DUMP("11=0x%lx,22=0x%lx!!\r\n", gp_movAudioEntry, pEntry);
	memcpy((UINT8 *)pEntry, (UINT8 *)ptr, sizeof(MOV_Ientry));
	//DBG_MSG("movAudioEntry[%d].pos=0x%lx,size =0x%lx!, key=%d\r\n", index, pEntry->pos, pEntry->size, pEntry->key_frame);

}

void MOV_SetAudioIentry_old32(UINT32 id, UINT32 index, MOV_old32_Ientry *ptr)
{
	MOV_Ientry      *pEntry = 0;

	//DBG_DUMP("[A%d][%d].pos=0x%lx,size =0x%lx!!\r\n", id, index, ptr->pos, ptr->size);

	if (g_movAudEntryAddr[id] == 0) {
		DBG_DUMP("g_movAudEntryAddr[%d] =0!!\r\n", id);
		return;
	}
	if (index > g_movAudioEntryMax[id]) {
		DBG_DUMP("id = %d, index =%d big!!%d!!\r\n", id, index, g_movAudioEntryMax[id]);
		return;
	}
	gp_movAudioEntry = (MOV_Ientry *)g_movAudEntryAddr[id];
	pEntry = gp_movAudioEntry + index;
	//DBG_DUMP("11=0x%lx,22=0x%lx!!\r\n", gp_movAudioEntry, pEntry);
	//memcpy((UINT8 *)pEntry, (UINT8 *)ptr, sizeof(MOV_Ientry));
	pEntry->pos = (UINT64)ptr->pos;
	pEntry->size = ptr->size;
	pEntry->key_frame = ptr->key_frame;
	pEntry->updated   = ptr->updated;
	//DBG_MSG("movAudioEntry[%d].pos=0x%lx,size =0x%lx!, key=%d\r\n", index, pEntry->pos, pEntry->size, pEntry->key_frame);

}
void MOV_ResetEntry(UINT32 id)
{
	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	if (gp_movEntry == NULL) {
		return;
	}
	if (g_movEntrySize[id] == 0) {
		return;
	}

	//clear EntryBlock
	memset((UINT8 *)gp_movEntry, 0, g_movEntrySize[id]);
	//DBG_MSG("resetsize=0x%lx!!!!!!!!!!", g_movEntrySize);
}

void MOV_SetGPSEntryAddr(UINT32 addr, UINT32 size, UINT32 id)
{

	g_movGPSEntrySize[id] = size;
	g_movGPSEntryAddr[id] = addr;
	g_movGPSEntryMax[id] = size / (sizeof(MOV_Ientry));

	memset((UINT8 *)addr, 0, size);
#if 0//MOV_RECDEBUG
	DBG_DUMP("^R[%d] GPS addr=0x%lX, max=0x%lX\r\n", id, addr, g_movGPSEntryMax[id]);
#endif
}

void MOV_SetGPS_Entry(UINT32 id, UINT32 index, MOV_Ientry *ptr)
{
	MOV_Ientry      *pEntry = 0;
	MOV_Ientry      *pEntryNow = 0;
	if (g_movGPSEntryAddr[id] == 0) {
		DBG_DUMP("g_movGPSEntryAddr[%d]=zero\r\n", id);
		return;
	}
	if (index > g_movGPSEntryMax[id]) {
		DBG_DUMP("exceed g_movGPSEntryMax[%d]=%d\r\n", id, g_movGPSEntryMax[id]);
		return;
	}

	if ((index == 0) && (ptr->pos == 0) && (ptr->size == 0)) {
		// reset flow
		g_movGPSEntryTotal[id] = 0;
		return;
	}

	pEntry = (MOV_Ientry *)g_movGPSEntryAddr[id];
	pEntryNow = pEntry + index;
	{
		memcpy((UINT8 *)pEntryNow, (UINT8 *)ptr, sizeof(MOV_Ientry));
	}
	g_movGPSEntryTotal[id] = (index + 1);
	//DBG_DUMP("GPS entry[id %d][%d]pos=0x%lX, size=0x%lX\r\n",id, index,ptr->pos, ptr->size);

}

#if 1	//add for meta -begin
void MOV_SetMetaEntryAddr(UINT32 addr, UINT32 size, UINT32 id)
{
	UINT32 metanum = g_mov_meta_num[id];
	UINT32 metasize;
	UINT32 metaaddr;
	UINT32 metaidx = 0;
	if (metanum == 0) {
		return;
	}
	metasize = size / metanum;
	metaaddr = addr;
	for (metaidx = 0; metaidx < metanum; metaidx++) {
		g_mov_meta_entry_size[id][metaidx] = metasize;
		g_mov_meta_entry_addr[id][metaidx] = metaaddr;
		g_mov_meta_entry_max[id][metaidx] = metasize / (sizeof(MOV_Ientry));
		metaaddr += metasize;
	}

	if ((addr != 0) && (size != 0))
		memset((UINT8 *)addr, 0, size);
}

void MOV_SetMeta_Entry(UINT32 id, UINT32 index, MOV_Ientry *ptr)
{
	MOV_Ientry      *pEntry = 0;
	MOV_Ientry      *pEntryNow = 0;
	UINT8 metaidx = ptr->updated;
	if (g_mov_meta_entry_addr[id][metaidx] == 0) {
		DBG_DUMP("g_mov_meta_entry_addr[%d][%d]=zero\r\n", id, index);
		return;
	}
	if (index > g_mov_meta_entry_max[id][metaidx]) {
		DBG_DUMP("exceed g_mov_meta_entry_max[%d][%d]=%d\r\n", id, metaidx, g_mov_meta_entry_max[id][metaidx]);
		return;
	}

	if ((index == 0) && (ptr->pos == 0) && (ptr->size == 0)) {
		// reset flow
		g_mov_meta_entry_total[id][metaidx] = 0;
		return;
	}

	pEntry = (MOV_Ientry *)g_mov_meta_entry_addr[id][metaidx];
	pEntryNow = pEntry + index;
	{
		memcpy((UINT8 *)pEntryNow, (UINT8 *)ptr, sizeof(MOV_Ientry));
	}
	g_mov_meta_entry_total[id][metaidx] = (index + 1);
	//DBG_DUMP("META entry[id %d][md %d][%d]pos=0x%lX, size=0x%lX\r\n", id, metaidx, index, (unsigned long)ptr->pos, (unsigned long)ptr->size);
}
#endif	//add for meta -end

extern UINT32 gTempPacketBufBase;
UINT32 gMovMp4_backhdrsize[MOVWRITER_COUNT] = {0};
UINT32 MOVMJPG_UpdateHeader(UINT32 id, MOVMJPG_FILEINFO *pMOVInfo)
{
	UINT32  headerLen = 0, /*tt1, tt2 = 0, */ori = 0;
	UINT64  mdatSize = 0;
	UINT32  soundSize, temp, temp2, temp3, newsize = 0; //2010/01/29 Meg Lin
	UINT32  uiFtypSize, pos1 = 0, size1 = 0, pos2 = 0, size2 = 0, w_start = 0, w_len = 0;
	UINT8   pChar[16] = {0};
	UINT32  wordAlign4size = 0;//2012/10/24 Meg
	UINT32  tima_foft = 0;//2013/05/28 Meg
	VOS_TICK tt1, tt2;
	UINT32  timescale = 0;
	UINT32  trackID = 0;

	// Write out video BitStream if there is video Bitstream
	//MOVWriteWaitPreCmdFinish();//2012/07/13 Meg
	//if ( FileSys_GetDiskInfo(FST_INFO_PARTITION_TYPE) == 0x04 && gMovCo64)
	//    mdatSize = (UINT64)pMOVInfo->totalMdatSize + (MOV_MDAT_HEADER) + 8;
	//else
	mdatSize = (UINT64)pMOVInfo->totalMdatSize + (MOV_MDAT_HEADER);//AVI_lastVideoPad
	// Close file
	NMR_MOVMUST("[%d] 1=0x%lx\r\n", id, (unsigned long)pMOVInfo->ptempMoovBufAddr);
	MOV_MakeAtomInit(pMOVInfo->ptempMoovBufAddr);

	//#NT#2012/06/26#Hideo Lin -begin
	//#NT#To support MP4 file type
	if (pMOVInfo->uiFileType == MEDIA_FTYP_MP4) {
		uiFtypSize = MP4_FTYP_SIZE;
	} else {
		uiFtypSize = MOV_FTYP_SIZE;
	}
	//#NT#2012/06/26#Hideo Lin -end

	//--------------VIDEO track begin -----------------
	timescale = MOV_Wrtie_GetTimeScale(pMOVInfo->VideoFrameRate);
	NMR_MOVMUST("timescale: %d\r\n", timescale);
	g_movCon[id].codec_type = REC_VIDEO_TRACK;
	g_movCon[id].width = pMOVInfo->ImageWidth;
	g_movCon[id].height = pMOVInfo->ImageHeight;
	NMR_MOVMUST("id= %d!, encVideoCodec =%d!\r\n", id, pMOVInfo->encVideoCodec);
	if (pMOVInfo->encVideoCodec == MEDIAVIDENC_MJPG) {
		g_movCon[id].codec_name = (char *)&g_movMJPGCodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		//g_movTrack.tag = 0x6D6A7061;//mjpa (Motion JPEG A)
		g_movTrack[id].tag = 0x6A706567;//jpeg (Photo JPEG)
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
	} else if (pMOVInfo->encVideoCodec == MEDIAVIDENC_H264) {
		g_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		g_movTrack[id].tag = 0x61766331;//avc1
		//DBG_DUMP("^R AVC1\r\n");
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
	} else if (pMOVInfo->encVideoCodec == MEDIAVIDENC_H265) {
		g_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		g_movTrack[id].tag = 0x68766331;//hvc1
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
		//DBG_DUMP("^G HVC1\r\n");
	} else {
		DBG_ERR("pMOVInfo->encVideoCodec %d ERROR!!!!\r\n", (int)pMOVInfo->encVideoCodec);
		DBG_ERR("DO NOT UPDATE HEADER!!!\r\n");
		return 0;
	}
	NMR_MOVMUST("id= %d!, context = 0x%lx!\r\n", id, (UINT32)&g_movCon[id]);
	//g_movCon.rc_buffer_size = 0x2EE000;
	//g_movCon.rc_max_rate = 0xA00F00;
	if (pMOVInfo->videoFrameCount < g_movEntryMax[id]) { //MOV_ENTRY_MAX)
		g_movTrack[id].entry = pMOVInfo->videoFrameCount;
		//DBGD(pMOVInfo->videoFrameCount);
	} else {
		g_movTrack[id].entry = g_movEntryMax[id];//MOV_ENTRY_MAX;
		//DBGD(g_movEntryMax[id]);
	}
	NMR_MOVMUST("[%d] vidfrcnt=0x%lx\r\n", id, pMOVInfo->videoFrameCount);
	g_movTrack[id].mode = MOVMODE_MOV;
	g_movTrack[id].hasKeyframes = 1;
	g_movTrack[id].ctime = MOVWriteCountTotalSeconds();//base 2004 01 01 0:0:0
	g_movTrack[id].mtime = MOVWriteCountTotalSeconds();

	g_movTrack[id].timescale = timescale;//2009/11/24 Meg Lin
	NMR_MOVMUST("trackDuration=%d!!!\r\n", (int)pMOVInfo->PlayDuration);
	g_movTrack[id].trackDuration = pMOVInfo->PlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;
#if 1//add for sub-stream -begin
	if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
		if (pMOVInfo->PlayDuration == 0 && pMOVInfo->SubPlayDuration == 0) {
			DBG_DUMP("pMOVInfo->PlayDuration ZERO!!!! ERROR1!!");
			return MOV_STATUS_INVALID_PARAM;
		}
	} else {
		if (pMOVInfo->PlayDuration == 0) {
			DBG_DUMP("pMOVInfo->PlayDuration ZERO!!!! ERROR1!!");
			return MOV_STATUS_INVALID_PARAM;
		}
	}
#else
	if (pMOVInfo->PlayDuration == 0) {
		DBG_DUMP("pMOVInfo->PlayDuration ZERO!!!! ERROR1!!");
		return MOV_STATUS_INVALID_PARAM;
	}
#endif//add for sub-stream -end
	//DBG_DUMP("pMOVInfo->PlayDuration[%d] time %d!!", id, g_movTrack[id].trackDuration);
	g_movTrack[id].language = 0x00;//english
	trackID++;
	g_movTrack[id].trackID = trackID;
	g_movTrack[id].frameRate = pMOVInfo->VideoFrameRate;
	NMR_MOVMUST("^R gp_vfr %d\r\n", g_movTrack[id].frameRate);
	g_movTrack[id].uiH264GopType = pMOVInfo->uiH264GopType;

	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	NMR_MOVMUST("^R gp_movEntry=0x%lx\r\n", (unsigned long)gp_movEntry);
	if (gp_movEntry) {
		g_movTrack[id].cluster = gp_movEntry;//&g_movEntry[0];
		g_movTrack[id].entry_head = g_entry_pos[id].vid_head;
		g_movTrack[id].entry_tail = g_entry_pos[id].vid_tail;
		g_movTrack[id].entry_max = (MOV_Ientry *)g_movEntryAddr[id] + g_movEntryMax[id];
	} else {
		return MOV_STATUS_ENTRYADDRERR;
	}
	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	//g_movTrack[id].uiRotateInfo = pMOVInfo->uiVideoRotate;
	//#NT#2013/04/17#Calvin Chang -end
	//--------------VIDEO track end -----------------
	//--------------VIDEO sub track begin -----------------
#if 1//add for sub-stream -begin
	if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
		g_sub_movCon[id].codec_type = REC_VIDEO_TRACK;
		g_sub_movCon[id].width = pMOVInfo->SubImageWidth;
		g_sub_movCon[id].height = pMOVInfo->SubImageHeight;
		NMR_MOVMUST("id= %d!, encVideoCodec =%d!\r\n", id, pMOVInfo->SubencVideoCodec);
		if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_MJPG) {
			g_sub_movCon[id].codec_name = (char *)&g_movMJPGCodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			//g_movTrack.tag = 0x6D6A7061;//mjpa (Motion JPEG A)
			g_sub_movTrack[id].tag = 0x6A706567;//jpeg (Photo JPEG)
			//g_movTrack.vosLen = 0x20;
			//g_movTrack.vosData = &g_movEsdsData05[0];
		} else if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_H264) {
			g_sub_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			g_sub_movTrack[id].tag = 0x61766331;//avc1
			//DBG_DUMP("^R AVC1\r\n");
			//g_movTrack.vosLen = 0x20;
			//g_movTrack.vosData = &g_movEsdsData05[0];
		} else if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_H265) {
			g_sub_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			g_sub_movTrack[id].tag = 0x68766331;//hvc1
			//g_movTrack.vosLen = 0x20;
			//g_movTrack.vosData = &g_movEsdsData05[0];
			//DBG_DUMP("^G HVC1\r\n");
		} else {
			DBG_ERR("pMOVInfo->SubencVideoCodec %d ERROR!!!!\r\n", (int)pMOVInfo->SubencVideoCodec);
			DBG_ERR("DO NOT UPDATE HEADER!!!\r\n");
			return 0;
		}
		NMR_MOVMUST("id= %d!, context = 0x%lx!\r\n", id, (UINT32)&g_sub_movCon[id]);
		//g_sub_movCon.rc_buffer_size = 0x2EE000;
		//g_sub_movCon.rc_max_rate = 0xA00F00;
		if (pMOVInfo->SubvideoFrameCount < g_sub_movEntryMax[id]) { //MOV_ENTRY_MAX)
			g_sub_movTrack[id].entry = pMOVInfo->SubvideoFrameCount;
			//DBGD(pMOVInfo->videoFrameCount);
		} else {
			g_sub_movTrack[id].entry = g_sub_movEntryMax[id];//MOV_ENTRY_MAX;
			//DBGD(g_movEntryMax[id]);
		}
		NMR_MOVMUST("[%d] vidfrcnt=0x%lx\r\n", id, pMOVInfo->SubvideoFrameCount);
		g_sub_movTrack[id].mode = MOVMODE_MOV;
		g_sub_movTrack[id].hasKeyframes = 1;
		g_sub_movTrack[id].ctime = MOVWriteCountTotalSeconds();//base 2004 01 01 0:0:0
		g_sub_movTrack[id].mtime = MOVWriteCountTotalSeconds();

		g_sub_movTrack[id].timescale = timescale;//2009/11/24 Meg Lin
		NMR_MOVMUST("trackDuration=%d!!!\r\n", (int)pMOVInfo->SubPlayDuration);
		g_sub_movTrack[id].trackDuration = pMOVInfo->SubPlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;
		if (pMOVInfo->PlayDuration == 0 && pMOVInfo->SubPlayDuration == 0) {
			DBG_DUMP("pMOVInfo->SubPlayDuration ZERO!!!! ERROR2!!");
			return MOV_STATUS_INVALID_PARAM;
		}
		//DBG_DUMP("pMOVInfo->PlayDuration[%d] time %d!!", id, g_sub_movTrack[id].trackDuration);
		g_sub_movTrack[id].language = 0x00;//english
		trackID++;
		g_sub_movTrack[id].trackID = trackID;
		g_sub_movTrack[id].frameRate = pMOVInfo->SubVideoFrameRate;
		NMR_MOVMUST("^R gp_vfr %d\r\n", g_sub_movTrack[id].frameRate);
		g_sub_movTrack[id].uiH264GopType = pMOVInfo->uiH264GopType;

		gp_sub_movEntry = (MOV_Ientry *)g_sub_movEntryAddr[id];
		NMR_MOVMUST("^R gp_movEntry=0x%lx\r\n", (unsigned long)gp_sub_movEntry);
		if (gp_sub_movEntry) {
			g_sub_movTrack[id].cluster = gp_sub_movEntry;
		} else {
			return MOV_STATUS_ENTRYADDRERR;
		}
	}
#endif//add for sub-stream -end
	//--------------VIDEO sub track end -----------------
	//--------------audio track begin -----------------
#if 1//2007/06/12 meg_nowave, rec_no_wave, mark this
	NMR_MOVMUST("[%d] audsize=0x%lx\r\n", id, pMOVInfo->audioTotalSize);
	NMR_MOVMUST("[%d] afn=0x%lx\r\n", id, pMOVInfo->audioFrameCount);
	if (pMOVInfo->audioTotalSize != 0) {
		g_movAudioCon[id].codec_type = REC_AUDIO_TRACK;
		g_movAudioCon[id].width = pMOVInfo->ImageWidth;
		g_movAudioCon[id].height = pMOVInfo->ImageHeight;

		if (pMOVInfo->encAudioCodec == MOVAUDENC_AAC) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_AAC;//mp4a
			g_movAudioCon[id].rc_max_rate = pMOVInfo->AudioBitRate;
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else if (pMOVInfo->encAudioCodec == MOVAUDENC_ULAW) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_ULAW;//ulaw
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else if (pMOVInfo->encAudioCodec == MOVAUDENC_ALAW) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_ALAW;//alaw
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else { // if (pMOVInfo->encAudioCodec== MOVAUDENC_PCM), default
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_PCM;//sowt
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		}

		if (pMOVInfo->encAudioCodec == MOVAUDENC_ULAW ||
			pMOVInfo->encAudioCodec == MOVAUDENC_ALAW) {
			g_movAudioCon[id].wBitsPerSample = 0x08;
		} else {
			g_movAudioCon[id].wBitsPerSample = 0x10;
		}

		//#NT#2009/12/21#Meg Lin -begin
		//#NT#new feature: rec stereo
		g_movAudioCon[id].nchannel = pMOVInfo->AudioChannel;

		g_movAudioCon[id].nAvgBytesPerSecond = (pMOVInfo->AudioSampleRate) * 2 * pMOVInfo->AudioChannel;
		//#NT#2009/12/21#Meg Lin -end
		g_movAudioCon[id].nSamplePerSecond = pMOVInfo->AudioSampleRate;
		if (g_movAudioTrack[id].entry > g_movAudioEntryMax[id]) { //MOV_ENTRY_MAX)
			DBG_DUMP("^R toomuch id=%d, %ld, %ld\r\n", id, g_movAudioTrack[id].entry, g_movAudioEntryMax[id]);
			g_movAudioTrack[id].entry = g_movAudioEntryMax[id];//MOV_ENTRY_MAX;
		}
		g_movAudioTrack[id].context = &g_movAudioCon[id];
		//g_movAudioTrack.tag = 0x6D703476;//mp4v
		g_movAudioTrack[id].vosLen = 0x20;
		g_movAudioTrack[id].mode = MOVMODE_MOV;
		g_movAudioTrack[id].hasKeyframes = 1;
		g_movAudioTrack[id].ctime = MOVWriteCountTotalSeconds();
		g_movAudioTrack[id].mtime = MOVWriteCountTotalSeconds();
		//#NT#2009/11/24#Meg Lin -begin
		g_movAudioTrack[id].timescale = timescale;
		g_movAudioTrack[id].audioSize = pMOVInfo->audioTotalSize;
		//DBG_MSG("audiosize=%lx!\r\n", g_movAudioTrack.audioSize);
		//fixbug: countsize over UINT32
#if 0
		//if (g_movAudioTrack.audioSize > 0x600000)//0xffffffff/600 =6.8M, *1000 will exceed UINT32 size
		if (g_movAudioTrack.audioSize > 0x20000) { //0xffffffff/30000 =143165 (0x22f00), *1000 will exceed UINT32 size
			temp = g_movAudioTrack.audioSize / g_movAudioCon.nAvgBytesPerSecond;
			temp2 = g_movAudioTrack.audioSize % g_movAudioCon.nAvgBytesPerSecond;
			soundSize = (temp2 * MOV_TIMESCALE) / g_movAudioCon.nAvgBytesPerSecond + temp * MOV_TIMESCALE; //2009/04/21 MegLin 60:01 bug fix
		} else {
			soundSize = (g_movAudioTrack.audioSize * MOV_TIMESCALE) / g_movAudioCon.nAvgBytesPerSecond;
		}
#else
		temp = g_movAudioTrack[id].audioSize / g_movAudioCon[id].nAvgBytesPerSecond;
		temp2 = g_movAudioTrack[id].audioSize % g_movAudioCon[id].nAvgBytesPerSecond;
		//2013/08/28 Meg fixbug
		//if (temp2 > 0x10000) { //0xffffffff/60000 =71582 (0x1179e), *1000 will exceed UINT32 size
		if (temp2 > 0xB000) { //0xffffffff/92400 =46482 (0xB592), *1000 will exceed UINT32 size
			temp3 = temp2 / 2;
			soundSize = (temp3 * timescale) / g_movAudioCon[id].nAvgBytesPerSecond * 2;
		} else {
			soundSize = (temp2 * timescale) / g_movAudioCon[id].nAvgBytesPerSecond;
		}

		soundSize += temp * timescale; //2009/04/21 MegLin 60:01 bug fix
		//DBG_DUMP("audsize=0x%lx, nAvg=0x%lx, sound=0x%lx\r\n",g_movAudioTrack[id].audioSize, g_movAudioCon[id].nAvgBytesPerSecond ,soundSize);

#endif
		//#NT#2009/11/24#Meg Lin -end

		g_movAudioTrack[id].trackDuration = (UINT32)soundSize;
		g_movAudioTrack[id].language = 0x00;//english
		trackID++;
		g_movAudioTrack[id].trackID = trackID; //2010/01/29 Meg Lin, fixbug

		gp_movAudioEntry = (MOV_Ientry *)g_movAudEntryAddr[id];

		//DBG_DUMP("3=0x%lx\r\n", gp_movAudioEntry);
		if (gp_movAudioEntry) {
			g_movAudioTrack[id].cluster = gp_movAudioEntry;
			g_movAudioTrack[id].entry_head = g_entry_pos[id].aud_head;
			g_movAudioTrack[id].entry_tail = g_entry_pos[id].aud_tail;
			g_movAudioTrack[id].entry_max = (MOV_Ientry *)g_movAudEntryAddr[id] + g_movAudioEntryMax[id];
		} else {
			return MOV_STATUS_ENTRYADDRERR;
		}
		//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
		g_movAudioTrack[id].uiRotateInfo = 0;
		//#NT#2013/04/17#Calvin Chang -end
	}
	//--------------audio track end -----------------
#endif//2007/06/12 meg_nowave
	g_movTrack[id].pH264Desc = g_264VidDesc[id];
	g_movTrack[id].pH265Desc = g_265VidDesc[id];
	g_movTrack[id].h265Len   = g_movh265len[id];
#if 1//add for sub-stream -begin
	if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
		g_sub_movTrack[id].pH264Desc = g_sub_264VidDesc[id];
		g_sub_movTrack[id].pH265Desc = g_sub_265VidDesc[id];
		g_sub_movTrack[id].h265Len   = g_sub_movh265len[id];
	}
#endif//add for sub-stream -end

	//DBG_DUMP("[%d] dur=0x%lx\r\n", id, g_movTrack[id].trackDuration);
	//DBG_DUMP("^G pH264Desc=0x%lx\r\n", &g_264VidDesc[0]);
	//DBG_DUMP("^G g_movTrack[0]0x%lx 0x%lx\r\n", g_movTrack[0].entry, g_movTrack[1].entry);
	if ((pMOVInfo->audioTotalSize != 0)) { //&&(gMovRecFileInfo.encVideoCodec==MEDIAVIDENC_MJPG)) //FAKE_264
		//DBG_DUMP("vidtrcAA[%d] =0x%lx!\r\n",id,  &g_movTrack[id]);

#if 1//add for sub-stream -begin
		if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
			headerLen = MOV_Sub_WriteMOOVTag(0, &g_movTrack[id], &g_sub_movTrack[id], &g_movAudioTrack[id], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
		} else {
			headerLen = MOVWriteMOOVTag(0, &g_movTrack[id], &g_movAudioTrack[id], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
		}
#else
		headerLen = MOVWriteMOOVTag(0, &g_movTrack[id], &g_movAudioTrack[id], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
#endif//add for sub-stream -end
	} else {
		//DBG_DUMP("vidtrcBB[%d] =0x%lx!\r\n",id,  &g_movTrack[id]);
		//debug_dumpmem(0x807de260, 0x50);//h264 0xc0a10000
#if 1//add for sub-stream -begin
		if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
			headerLen = MOV_Sub_WriteMOOVTag(0, &g_movTrack[id], &g_sub_movTrack[id], 0, TRACK_INDEX_VIDEO);
		} else {
			headerLen = MOVWriteMOOVTag(0, &g_movTrack[id], 0, TRACK_INDEX_VIDEO);
		}
#else
		headerLen = MOVWriteMOOVTag(0, &g_movTrack[id], 0, TRACK_INDEX_VIDEO);
#endif//add for sub-stream -end
	}
	if (headerLen == 0) {
		DBG_DUMP("^Rmov update header fails!!!");
		return MOV_STATUS_INVALID_PARAM;
	}
	headerLen += MOVWriteCUSTOMDataTag(0, &g_movTrack[id]);
	//rec_no_wave, use this --headerLen = MOVWriteMOOVTag(0, &g_movTrack, TRACK_INDEX_VIDEO);
	NMR_MOVMUST("[%d] headerlen=0x%lx\r\n", id, headerLen);
	gMovMp4_backhdrsize[id] = headerLen;

	//if (gMovRecFileInfo.audioTotalSize != 0)//FAKE_264

	{
		//MOVWriteFile(pMOVInfo->ptempMoovBufAddr, headerLen);
		//#NT#2012/07/13#Meg Lin -begin
		if (gMovContainerMaker.cbWriteFile) {
			DBG_DUMP("[MOV] write addr 0x%lx, size=0x%lx, pos=0x%llx!\r\n", (UINT32)pMOVInfo->ptempMoovBufAddr, headerLen, mdatSize);
			DBG_DUMP("[MOV] write1 size=0x%lx, pos=0x%llx!\r\n", headerLen, mdatSize);
			(gMovContainerMaker.cbWriteFile)(0,
											 mdatSize,//pos
											 headerLen, //size
											 (UINT32)pMOVInfo->ptempMoovBufAddr);//addr

		}
		//#NT#2012/07/13#Meg Lin -end
		{
			UINT32 clu;

			clu = gMovRecFileInfo[id].clustersize;
			g_movCon[id].finalsize = ((mdatSize + headerLen + clu) / clu) * clu;
		}
		//DBG_DUMP("file[%d]size= 0x%lx \r\n", id, g_movCon[id].finalsize);
		mdatSize -= uiFtypSize;//2009/11/24 Meg Lin
		//#NT#2010/01/29#Meg Lin# -begin
		if (g_movTrack[id].fre1_size != 0) {
			mdatSize -= (8 + g_movTrack[id].fre1_size);
			tima_foft += (8 + g_movTrack[id].fre1_size);
		}
		//#NT#2010/01/29#Meg Lin# -end

		//#NT#2012/08/24#Hideo Lin -begin
		//#NT#Add 'thum' atom for thumbnail data (customized by ourselves)
		//#NT#2019/10/15#Willy Su -begin
		//#NT#To support write 'frea'/'skip' tag without thumbnail
		#if 0
		if (g_movTrack[id].uiThumbSize != 0) {
		#endif
		{
			wordAlign4size = ALIGN_CEIL_4(g_movTrack[id].uiFreaSize);//2013/05/28 Meg

			mdatSize -= wordAlign4size;//2012/10/24 Meg
		}
		//#NT#2019/10/15#Willy Su -end
		//#NT#2012/08/24#Hideo Lin -end

		//DBG_MSG("write moov size=%lx, size=%lx!\r\n", mdatSize+8, headerLen);
		if (gMovCo64) {
			NMR_MOVMUST("use 64 bits mdat atom\r\n");

			pChar[0] = 0x00;
			pChar[1] = 0x00;
			pChar[2] = 0x00;
			pChar[3] = 0x01;
			pChar[4] = 0x6D;//m
			pChar[5] = 0x64;//d
			pChar[6] = 0x61;//a
			pChar[7] = 0x74;//t

			pChar[8] = (mdatSize >> 56) & 0xFF;
			pChar[9] = (mdatSize >> 48) & 0xFF;
			pChar[10] = (mdatSize >> 40) & 0xFF;
			pChar[11] = (mdatSize >> 32) & 0xFF;
			pChar[12] = (mdatSize >> 24) & 0xFF;
			pChar[13] = (mdatSize >> 16) & 0xFF;
			pChar[14] = (mdatSize >> 8) & 0xFF;
			pChar[15] = (mdatSize) & 0xFF;
		} else {
			NMR_MOVMUST("use 32 bits mdat atom\r\n");

			pChar[0] = (mdatSize >> 24) & 0xFF;
			pChar[1] = (mdatSize >> 16) & 0xFF;
			pChar[2] = (mdatSize >> 8) & 0xFF;
			pChar[3] = mdatSize & 0xFF;
			pChar[4] = 0x6D;//m
			pChar[5] = 0x64;//d
			pChar[6] = 0x61;//a
			pChar[7] = 0x74;//t
		}

		//#NT#2010/01/29#Meg Lin# -begin
		if (g_movTrack[id].fre1_size != 0) {
			newsize += g_movTrack[id].fre1_size + 8;//len + tag
			//DBG_DUMP("fre1_size=0x%lx\r\n", g_movTrack[id].fre1_size);
		}
		//#NT#2012/07/13#Meg Lin -begin

		//#NT#2012/08/24#Hideo Lin -begin
		//#NT#Add 'thum' atom for thumbnail data (customized by ourselves)
		//#NT#2019/10/15#Willy Su -begin
		//#NT#To support write 'frea'/'skip' tag without thumbnail
		#if 0
		if (g_movTrack[id].uiThumbSize != 0) { //2012/10/24 Meg
		#endif
		{
			newsize += wordAlign4size;//g_movTrack.uiThumbSize + 8;//len + tag
			//DBG_DUMP("wordAlign4size=0x%lx\r\n", wordAlign4size);
			//DBG_DUMP("uiThumbSize=0x%lx\r\n", g_movTrack[id].uiThumbSize );
		}
		//#NT#2019/10/15#Willy Su -end
		//#NT#2012/08/24#Hideo Lin -end
		//DBG_DUMP("^R uiFtypSize=0x%lx, newsize=0x%lx!\r\n",uiFtypSize, newsize);
		//coverity tt1 = Perf_GetCurrent();
		ori = gMovRecFileInfo[id].uiTempHdrAdr;
		if ((gMovEndfileWay) && (ori)) {
			UINT32 newpos11 = 0;

			//DBG_DUMP("READ adr=0x%lx\r\n", ori);
			newpos11 = (UINT32)ori + uiFtypSize + newsize;
			pos1 = uiFtypSize + newsize;
			size1 = 8 + 8;
			memcpy((char *)newpos11, (char *)pChar, 8 + 8);
			//DBG_DUMP("copy to=0x%lx, from=0x%lx\r\n", newpos11, pChar);
		} else {
			if (gMovContainerMaker.cbWriteFile) {
				//DBG_DUMP("[MOV] write addr 0x%lx, size=0x%lx, pos=0x%lx!\r\n", (UINT32)pChar, 8, uiFtypSize+newsize);
				//DBG_DUMP("[MOV] write2 size=0x%lx, pos=0x%lx!\r\n", 8, uiFtypSize+newsize);
				//DBG_DUMP("uiFtypSize 0x%lx, newi=0x%lx\r\n", uiFtypSize,newsize);
				(gMovContainerMaker.cbWriteFile)(0,
												 uiFtypSize + newsize, //pos
												 8 + 8, //size
												 (UINT32)pChar);//addr

			}
		}
		//tt2 = Perf_GetCurrent();
		vos_perf_mark(&tt2);
		//DBG_DUMP("step1=%d ms\r\n", (tt2-tt1)/1000);

		mdatSize = g_movTrack[id].trackDuration / timescale; //duration in sec
		pChar[0] = (mdatSize >> 24) & 0xFF;
		pChar[1] = (mdatSize >> 16) & 0xFF;
		pChar[2] = (mdatSize >> 8) & 0xFF;
		pChar[3] = mdatSize & 0xFF;
		tima_foft += (8 + 8); //len+frea+len+tima
		//DBG_DUMP("^R tima_foft=0x%lx!\r\n", tima_foft);
		if ((gMovEndfileWay) && (ori)) {
			UINT32 newpos11 = 0;
			//DBG_DUMP("^RgMovEndfileWay=1\r\n");
			//DBG_DUMP("[MOV] write addr 0x%lx, size=0x%lx, pos=0x%lx!\r\n", (UINT32)pChar, 8, uiFtypSize+newsize);
			//DBG_DUMP("[MOV] write2 size=0x%lx, pos=0x%lx!\r\n", 8, uiFtypSize+newsize);
			//DBG_DUMP("uiFtypSize 0x%lx, newi=0x%lx\r\n", uiFtypSize,newsize);
			newpos11 = (UINT32)ori + uiFtypSize + tima_foft;
			memcpy((char *)newpos11, (char *)pChar, 4);
			pos2 = uiFtypSize + tima_foft;
			size2 = 4;
			if (pos2 < pos1) {
				w_start = pos2;
				w_len = (pos1 + size1 - pos2);
			} else {
				w_start = pos1;
				w_len = (pos2 + size2 - pos1);
			}
			//DBG_DUMP("copy to22=0x%lx, from=0x%lx\r\n", newpos11, pChar);
			if (gMovContainerMaker.cbWriteFile) { //2013/06/06 Meg, fix: fre1
				//DBG_DUMP("[MOV] write3 size=0x%lx, pos=0x%lx!\r\n", 4, uiFtypSize+tima_foft);
				//DBG_DUMP("[MOV]W adrr 0x%lx, size=0x%lx, pos=0x%lx!\r\n", (UINT32)ori+w_start, w_len, 0);
				(gMovContainerMaker.cbWriteFile)(0, //type
												 0 + w_start, //filepos
												 w_len,  //writesize
												 (UINT32)ori + w_start); //addr

			}
		} else {
			//DBG_DUMP("^GgMovEndfileWay=0\r\n");
			if (gMovContainerMaker.cbWriteFile) { //2013/06/06 Meg, fix: fre1
				//DBG_DUMP("[MOV] write3 size=0x%lx, pos=0x%lx!\r\n", 4, uiFtypSize+tima_foft);
				(gMovContainerMaker.cbWriteFile)(0, //type
												 uiFtypSize + tima_foft, //filepos
												 4,  //writesize
												 (UINT32)pChar);//addr

				//DBG_DUMP("[MOV]W adrr 0x%lx, size=0x%lx, pos=0x%lx!\r\n", (UINT32)pChar, 4, uiFtypSize+tima_foft);
			}
		}
		//tt1 = Perf_GetCurrent();
		vos_perf_mark(&tt1);
		//if ((tt1 - tt2) > 500000) { //500 ms
		//	DBG_DUMP("updateHDR: step2=%d ms\r\n", (tt1 - tt2) / 1000);
		//}
		if (vos_perf_duration(tt2, tt1) > 500000) { //500 ms
			DBG_DUMP("updateHDR: step2=%d ms\r\n", vos_perf_duration(tt2, tt1) / 1000);
		}

		//#NT#2010/01/29#Meg Lin# -end
		//#NT#2012/07/13#Meg Lin -end
	}

	return MOV_STATUS_OK;
}

//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
void MOV_Write_InitEntryAddr(UINT32 id)
{
	if (g_movEntrySize[id] == 0) {
		return;
	}

	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	if (gp_movEntry == NULL) {
		return;
	}
	memset((UINT8 *)gp_movEntry, 0, g_movEntrySize[id]);
	{
		g_entry_pos[id].vid_head = (MOV_Ientry *)g_movEntryAddr[id];
		g_entry_pos[id].vid_tail = (MOV_Ientry *)g_movEntryAddr[id];
		g_entry_pos[id].vid_num  = 0;
	}
	{
		g_entry_pos[id].aud_head = (MOV_Ientry *)g_movAudEntryAddr[id];
		g_entry_pos[id].aud_tail = (MOV_Ientry *)g_movAudEntryAddr[id];
		g_entry_pos[id].aud_num  = 0;
		g_entry_pos[id].aud_shift= 0;
	}
	return ;
}
//#NT#2019/11/22#Willy Su -end

//#NT#2012/09/19#Meg Lin -begin
void MOV_Write_SetEntryAddr(UINT32 addr, UINT32 size, UINT32 sampleRate, UINT32 vidFR, UINT32 id)
{
	UINT32  entryMax = 0;
	UINT32  Tmp;
	UINT32  vidR;

	g_movEntrySize[id] = size;
	if (sampleRate == 0) { //PCM. vid:aud = 10:1
		vidR = 10;
#if 1//MOV_RECDEBUG
		//DBG_DUMP("PCM\r\n");
#endif
	} else {
		vidR = MOVWrite_TransRoundVidFrameRate(vidFR);
#if 0//MOV_RECDEBUG
		DBG_DUMP("AAC %d, %d\r\n", vidR, audR);
#endif
	}
	//add for sub-stream -begin
	if (gMovRecFileInfo[id].SubVideoFrameRate) {
		// video
		g_movEntryAddr[id] = addr;
		gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
		entryMax = size / sizeof(MOV_Ientry); //2012/12/18 Meg, fixbug
		//DBG_DUMP("size = 0x%lx, max = 0x%lx\r\n", size, entryMax);
		g_movEntryMax[id] = MOV_Write_CalcMainV(size, sampleRate, vidR, gMovRecFileInfo[id].SubVideoFrameRate);
		Tmp = addr + (g_movEntryMax[id] + 1) * sizeof(MOV_Ientry);
		// video sub-stream
		g_sub_movEntryAddr[id] = Tmp;
		gp_sub_movEntry = (MOV_Ientry *)g_sub_movEntryAddr[id];
		g_sub_movEntryMax[id] = MOV_Write_CalcSubV(size, sampleRate, vidR, gMovRecFileInfo[id].SubVideoFrameRate);
		Tmp = g_sub_movEntryAddr[id] + (g_sub_movEntryMax[id] + 1) * sizeof(MOV_Ientry);
		// audio
		g_movAudEntryAddr[id] = Tmp;
		gp_movAudioEntry = (MOV_Ientry *)g_movAudEntryAddr[id];
		g_movAudioEntryMax[id] = entryMax - g_movEntryMax[id] - 2;
	} else {
		// video
		g_movEntryAddr[id] = addr;
		gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
		entryMax = size / sizeof(MOV_Ientry); //2012/12/18 Meg, fixbug
		//DBG_DUMP("size = 0x%lx, max = 0x%lx\r\n", size, entryMax);
		g_movEntryMax[id] = MOV_Write_CalcV(size, sampleRate, vidR);
		Tmp = addr + (g_movEntryMax[id] + 1) * sizeof(MOV_Ientry);
		// audio
		g_movAudEntryAddr[id] = Tmp;
		gp_movAudioEntry = (MOV_Ientry *)g_movAudEntryAddr[id];
		g_movAudioEntryMax[id] = entryMax - g_movEntryMax[id] - 2;
	//VID
	{
		g_entry_pos[id].vid_head = (MOV_Ientry *)g_movEntryAddr[id];
		g_entry_pos[id].vid_tail = (MOV_Ientry *)g_movEntryAddr[id];
	}
	//AUD
	{
		g_entry_pos[id].aud_head = (MOV_Ientry *)g_movAudEntryAddr[id];
		g_entry_pos[id].aud_tail = (MOV_Ientry *)g_movAudEntryAddr[id];
	}

	}
	//add for sub-stream -end
	#if 0
	memset((UINT8 *)gp_movEntry, 0, size);
	#else
#if defined (__UITRON) || defined (__ECOS)
	hwmem_open();
	hwmem_memset(addr,0, size);
	hwmem_close();
#else
	memset((void *)addr, 0, size);
#endif
	#endif
#if MOV_RECDEBUG
	DBG_DUMP("[%d] videoMax=0x%lX, audioMax=0x%lX\r\n", id, g_movEntryMax[id], g_movAudioEntryMax[id]);
	//DBG_DUMP("^G [%d]g_movEntryAddr=0x%lx.\r\n",id, g_movEntryAddr[id]);
	//DBG_DUMP("[%d] videoMax=0x%lX, audioMax=0x%lX\r\n",id, gp_movEntry,gp_movAudioEntry);
#endif
}

UINT32 MOV_Write_GetEntrySize(UINT32 sampleRate, UINT32 vidFR, UINT32 sec)
{
	UINT32  entrysize = 0;
	UINT32  vidR, audR;
	if (sampleRate == 0) { //PCM. vid:aud = 10:1
		vidR = 10;
		audR = 1;
#if 1//MOV_RECDEBUG
		//DBG_DUMP("PCM\r\n");
#endif
	} else {
		vidR = MOVWrite_TransRoundVidFrameRate(vidFR);
		audR = (sampleRate / 1024) + 1; //ex: 8000/1024 +1 = 8, 48000/1024+1 = 47
#if 0//MOV_RECDEBUG
		DBG_DUMP("AAC %d, %d\r\n", vidR, audR);
#endif
	}
	entrysize = (vidR + audR) * sizeof(MOV_Ientry); //2012/12/18 Meg, fixbug
	return (entrysize * (sec + 2));
}


UINT32 MOV_Write_CalcV(UINT32 size, UINT32 sampleRate, UINT32 vidFR)
{
	UINT32  entryMax = 0, vMax;
	UINT32  vidR, audR;

	if (sampleRate == 0) { //PCM. vid:aud = 10:1
		vidR = 10;
		audR = 1;
	} else {
		vidR = vidFR;
		audR = (sampleRate / 1024) + 1; //ex: 8000/1024 +1 = 8, 48000/1024+1 = 47
	}
	entryMax = size / (sizeof(MOV_Ientry));
	vMax = entryMax * vidR / (vidR + audR);
	return vMax;
}
//#NT#2012/09/19#Meg Lin -end

//add for sub-stream -begin
UINT32 MOV_Write_CalcMainV(UINT32 size, UINT32 sampleRate, UINT32 vidFR, UINT32 subFR)
{
	UINT32  entryMax = 0, vMax;
	UINT32  vidR, audR;

	if (sampleRate == 0) { //PCM. vid:aud = 10:1
		vidR = 10;
		audR = 1;
	} else {
		vidR = vidFR;
		audR = (sampleRate / 1024) + 1; //ex: 8000/1024 +1 = 8, 48000/1024+1 = 47
	}
	entryMax = size / (sizeof(MOV_Ientry));
	vMax = entryMax * vidR / (vidR + audR + subFR);
	return vMax;
}

UINT32 MOV_Write_CalcSubV(UINT32 size, UINT32 sampleRate, UINT32 vidFR, UINT32 subFR)
{
	UINT32  entryMax = 0, vMax;
	UINT32  vidR, audR;

	if (sampleRate == 0) { //PCM. vid:aud = 10:1
		vidR = 10;
		audR = 1;
	} else {
		vidR = vidFR;
		audR = (sampleRate / 1024) + 1; //ex: 8000/1024 +1 = 8, 48000/1024+1 = 47
	}
	entryMax = size / (sizeof(MOV_Ientry));
	vMax = entryMax * subFR / (vidR + audR + subFR);
	return vMax;
}
//add for sub-stream -end

UINT32 MOV_Wrtie_GetTimeScale(UINT32 vfr)
{
#if 0
	return MOV_TIMESCALE;
#else
	UINT32 timescale = 0;
	UINT32 tmp1, tmp2;

	if (vfr == 0) {
		DBG_ERR("vfr zero\r\n");
		return MOV_TIMESCALE;
	}

	tmp1 = MOV_TIMESCALE / vfr;
	tmp2 = MOV_TIMESCALE % vfr;

	if (tmp2) {
		if (tmp1 / 1000) {
			timescale = ((tmp1 / 1000) * 1000) * vfr;
		} else if (tmp1 / 100) {
			timescale = ((tmp1 / 100) * 100) * vfr;
		} else if (tmp1 / 10) {
			timescale = ((tmp1 / 10) * 10) * vfr;
		} else {
			timescale = tmp1 * vfr;
		}
	} else {
		timescale = MOV_TIMESCALE;
	}

	return timescale;
#endif
}

UINT32 MOV_Wrtie_GetDuration(UINT32 vfn, UINT32 vfr)
{
	return vfn * (MOV_Wrtie_GetTimeScale(vfr) / vfr);
}

UINT32 MOV_Wrtie_GetEntryDuration(UINT32 id, UINT32 vfn, UINT32 vfr)
{
	MOV_Ientry *pEntry = 0;
	UINT32 index;
	UINT32 duration = 0, total_duration = 0;
	UINT32 curr_rate = 0, entry_count = 0;
	if (g_movEntryAddr[id] == 0) {
		return 0;
	}
	if (vfn > g_movEntryMax[id]) {
		return 0;
	}
	DBG_IND("MOV_Wrtie_GetEntryDuration[%d]: vfn=%d\r\n", id, vfn);
	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	for (index = 0; index < vfn; index++) {
		pEntry = gp_movEntry + index;
		entry_count++;
		if (pEntry->key_frame) {
			if ((curr_rate != pEntry->rev) && (curr_rate != 0)) {
				//1. next group, calculate duration and update
				duration = (entry_count - 1) * (MOV_Wrtie_GetTimeScale(curr_rate) / curr_rate);
				DBG_IND("[%d] entry_count(%d) curr_rate(%d) duration(%d)\r\n", id, entry_count, curr_rate, duration);
				total_duration += duration;
				curr_rate = pEntry->rev;
				entry_count = 1;
			} else {
				//1. next_rate == 0, first keyframe => keep counting
				//2. next_rate = cur_rate => keep counting
				curr_rate = pEntry->rev;
			}
		} else {
			//1. p-frame => keep counting
		}
	}
	if (index == vfn) {
		//1, last group, calculate duration and update
		duration = entry_count * (MOV_Wrtie_GetTimeScale(curr_rate) / curr_rate);
		total_duration += duration;
	}
	DBG_IND("[%d] total_duration=%d\r\n", id, total_duration);
	return total_duration;
}

UINT32 g_movMinusSize = 0;
MOV_Ientry  *gp_movWriteEntry;
MOV_Ientry  *gp_movWriteAudioEntry;
UINT32 g_movWriteEntryMax;
UINT32 g_movWriteAudioEntryMax;

void MOV_RollbackVentry(UINT32 sec, UINT32 flashsec, UINT32 fstVideoPos, UINT32 fr, UINT32 id);

void MOV_UpdateVentry(UINT32 sec, UINT32 flashsec, UINT32 fstVideoPos, UINT32 fr, UINT32 id)
{
	MOV_Ientry      *pEntry = 0;
	MOV_Ientry      *pOld = 0;
	UINT32 index = 0, i, j, filepos = 0;
	UINT8 showmsg = 0;

	gMovSeamlessCut = flashsec;
	showmsg = 1;
	index = sec * fr;

	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	pEntry = gp_movEntry + index;
	filepos = pEntry->pos;
	for (i = 0; i < (flashsec + 1); i++) {
		index = (sec + i) * fr;
		for (j = 0; j < fr; j++) {
			pEntry = gp_movEntry + index + j;
			//DBG_MSG("oldpos = 0x%lx ",pEntry->pos);
			if (pEntry->pos != 0) {
				pEntry->pos -= (filepos - fstVideoPos);
			}
			pOld = gp_movEntry + i * fr + j;
			memcpy(pOld, pEntry, sizeof(MOV_Ientry));
			//DBG_DUMP("newpos = 0x%lx \r\n",pEntry->pos);
		}
	}

	//clear old data
	for (i = (flashsec + 1); i < (sec + flashsec + 2); i++) {
		index = i * fr;
		for (j = 0; j < fr; j++) {
			pEntry = gp_movEntry + index + j;
			if (showmsg == 0) {
				DBG_DUMP("clr v[%d]!\r\n ", index + j);
				showmsg = 1;
			}
			pEntry->pos = 0;
		}
	}
	gMovSeamlessCut = 0;
	return;
}

void MOV_RollbackVentry(UINT32 sec, UINT32 flashsec, UINT32 fstVideoPos, UINT32 fr, UINT32 id)
{
	MOV_Ientry      *pEntry = 0;
	UINT32 index = 0, i, j, filepos = 0;

	gMovSeamlessCut = flashsec;
	index = sec * fr;
	//DBG_DUMP("^G old pEntry=0x%lx!\r\n", g_movEntryAddr[id]);
	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	pEntry = gp_movEntry + index;

	//set new Entry
	g_movEntryAddr[id] = (UINT32)pEntry;
	//DBG_DUMP("^G pEntry=0x%lx!\r\n", pEntry);


	filepos = pEntry->pos;
	for (i = 0; i < (flashsec + 1); i++) {
		index = (sec + i) * fr;
		for (j = 0; j < fr; j++) {
			pEntry = gp_movEntry + index + j;
			//DBG_MSG("oldpos = 0x%lx ",pEntry->pos);
			if (pEntry->pos != 0) {
				pEntry->pos -= (filepos - fstVideoPos);
				//DBG_DUMP("^R[%d]",index+j);
			}
			//pOld = gp_movEntry+i*fr+j;
			//memcpy(pOld, pEntry, sizeof(MOV_Ientry));
			//DBG_MSG("newpos = 0x%lx \r\n",pEntry->pos);
		}
	}

	//clear old data
	//donothing
	gMovSeamlessCut = 0;
	//coverity pEntry = gp_movEntry;


	return;
}
void MOV_UpdateAentry(UINT32 sec, UINT32 flashsec, UINT32 fstAudPos, UINT32 id)
{
	MOV_Ientry      *pEntry = 0;
	MOV_Ientry      *pOld = 0;
	UINT32 index = 0, i, filepos = 0;
	index = sec;//1st GOP less 2 B frame

	gp_movAudioEntry = (MOV_Ientry *)g_movAudEntryAddr[id];

	pEntry = gp_movAudioEntry + index;
	filepos = pEntry->pos;

	for (i = 0; i < flashsec; i++) {
		pEntry = gp_movAudioEntry + index + i;
		pEntry->pos -= (filepos - fstAudPos);
		pOld = gp_movAudioEntry + i;
		memcpy(pOld, pEntry, sizeof(MOV_Ientry));
	}
	for (i = 0; i < flashsec; i++) {
		pEntry = gp_movAudioEntry + i;
		//DBG_MSG("audio[%d] pos=0x%lx, size=0x%lx!\r\n", i, pEntry->pos, pEntry->size);
	}
	//clear old data
	for (i = (flashsec + 1); i < (sec + flashsec + 2); i++)
		//for (i=(flashsec+2);i<(flashsec+3);i++)
	{
		pEntry = gp_movAudioEntry + i;
		pEntry->pos = 0;
	}
	//coverity pEntry = gp_movAudioEntry;

	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// -----------------------------------------------------------------------
// | ftyp (standard, file type)                                          |
// -----------------------------------------------------------------------
// | skip (customized, for our own thumbnail 'frea')                     |
// -----------------------------------------------------------------------
// | moov (standard, video/audio tables)                                 |
// -----------------------------------------------------------------------
// | skip (customized, reserved)                                         |
// -----------------------------------------------------------------------
// | mdat (standard, main video/audio data)                              |
// -----------------------------------------------------------------------
//
///////////////////////////////////////////////////////////////////////////////////////////////////

UINT32 MOVMJPG_MakeMoov(UINT32 id, MOVMJPG_FILEINFO *pMOVInfo)
{
	UINT32  uiFreaEndSize = 0; //'ftyp' + 'frea'
	UINT32  uiFtypSize = 0;    //'ftyp'
	UINT32  uiMoovSize = 0;    //'moov'
	UINT32  uiSkipSize = 0;    //'skip'
	UINT64  uiMdatSize = 0;    //'mdat'
	UINT32  soundSize, temp, temp2, temp3;
	UINT32  totalmoovori, mdatTagSize;
	UINT32  timescale = 0;
	UINT32  trackID = 0;
	UINT32  sec, oft = 0;
	UINT8   *ptr;

	NMR_MOVMUST("MakeMoov0917 [%d]0x%lx\r\n", id, (unsigned long)pMOVInfo->ptempMoovBufAddr);
	ptr = (UINT8 *)pMOVInfo->ptempMoovBufAddr;
	uiMdatSize = (UINT64)pMOVInfo->totalMdatSize;
	totalmoovori = pMOVInfo->outMoovSize;//shift+moov+skip=bufsize
	pMOVInfo->outMoovSize = 0;//init
	timescale = MOV_Wrtie_GetTimeScale(pMOVInfo->VideoFrameRate);

	// 'ftyp' atom + 'frea' atom => uiFreaEndSize
	uiFreaEndSize = pMOVInfo->shiftfrea;
	if (uiFreaEndSize == 0) {
		uiFtypSize = MovWriteLib_MakefrontFtyp((UINT32)ptr, pMOVInfo->uiFileType);
		DBG_WRN("shiftfrea zero, add uiFtypSize 0x%x\r\n", uiFtypSize);
		uiFreaEndSize += uiFtypSize;
	} else {
		// 'tima' atom: add duration in sec (optional)
		sec = pMOVInfo->PlayDuration / timescale; //duration in sec
		if (pMOVInfo->uiFileType == MEDIA_FTYP_MP4) {
			oft += ((MP4_FTYP_SIZE) + (8 + 8)); //ftyp+len+tima (tima oft)
		} else {
			oft += ((MOV_FTYP_SIZE) + (8 + 8)); //ftyp+len+tima (tima oft)
		}
		MOV_MakeAtomInit(ptr);
		MOV_SeekOffset(0, oft, KFS_SEEK_SET);
		MOV_WriteB32Bits(0, sec);
	}
	ptr += pMOVInfo->shiftfrea;
	uiMdatSize -= (UINT64)uiFreaEndSize;
	MOV_MakeAtomInit(ptr);
	memset((void *)ptr, 0, totalmoovori);//init bufsize

	// 'moov' atom => uiMoovSize
	g_movTrack[id].timescale = timescale;
	g_movTrack[id].uiRotateInfo = 0;
	NMR_MOVMUST("vfn %d afn %d\r\n", pMOVInfo->videoFrameCount, pMOVInfo->audioFrameCount);
	//--------------VIDEO track begin -----------------
	g_movCon[id].codec_type = REC_VIDEO_TRACK;
	g_movCon[id].width = pMOVInfo->ImageWidth;
	g_movCon[id].height = pMOVInfo->ImageHeight;
	NMR_MOVMUST("id= %d!, encVideoCodec =%d!\r\n", id, pMOVInfo->encVideoCodec);
	if (pMOVInfo->encVideoCodec == MEDIAVIDENC_MJPG) {
		g_movCon[id].codec_name = (char *)&g_movMJPGCodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		//g_movTrack.tag = 0x6D6A7061;//mjpa (Motion JPEG A)
		g_movTrack[id].tag = 0x6A706567;//jpeg (Photo JPEG)
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
	} else if (pMOVInfo->encVideoCodec == MEDIAVIDENC_H264) {
		g_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		g_movTrack[id].tag = 0x61766331;//avc1
		//DBG_DUMP("^R AVC1\r\n");
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
	} else if (pMOVInfo->encVideoCodec == MEDIAVIDENC_H265) {
		g_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
		g_movTrack[id].context = &g_movCon[id];
		g_movTrack[id].tag = 0x68766331;//hvc1
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
		//DBG_DUMP("^G HVC1\r\n");
	} else {
		DBG_ERR("pMOVInfo->encVideoCodec %d ERROR!!!!\r\n", (int)pMOVInfo->encVideoCodec);
		DBG_ERR("DO NOT UPDATE HEADER!!!\r\n");
		return 0;
	}
	NMR_MOVMUST("id= %d!, context = 0x%lx!\r\n", id, (UINT32)&g_movCon[id]);
	//g_movCon.rc_buffer_size = 0x2EE000;
	//g_movCon.rc_max_rate = 0xA00F00;
	if (pMOVInfo->videoFrameCount < g_movEntryMax[id]) { //MOV_ENTRY_MAX)
		g_movTrack[id].entry = pMOVInfo->videoFrameCount;
		//DBGD(pMOVInfo->videoFrameCount);
	} else {
		g_movTrack[id].entry = g_movEntryMax[id];//MOV_ENTRY_MAX;
		//DBGD(g_movEntryMax[id]);
	}
	NMR_MOVMUST("[%d] vidfrcnt=0x%lx\r\n", id, pMOVInfo->videoFrameCount);
	g_movTrack[id].mode = MOVMODE_MOV;
	g_movTrack[id].hasKeyframes = 1;
	g_movTrack[id].ctime = MOVWriteCountTotalSeconds();//base 2004 01 01 0:0:0
	g_movTrack[id].mtime = MOVWriteCountTotalSeconds();
	g_movTrack[id].timescale = timescale;//2009/11/24 Meg Lin
	NMR_MOVMUST("trackDuration=%d!!!\r\n", (int)pMOVInfo->PlayDuration);
	g_movTrack[id].trackDuration = pMOVInfo->PlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;
	if (pMOVInfo->PlayDuration == 0) {
		DBG_DUMP("pMOVInfo->PlayDuration ZERO!!!! ERROR1!!");
		return MOV_STATUS_INVALID_PARAM;
	}
	//DBG_DUMP("pMOVInfo->PlayDuration[%d] time %d!!", id, g_movTrack[id].trackDuration);
	g_movTrack[id].language = 0x00;//english
	trackID++;
	g_movTrack[id].trackID = trackID;
	g_movTrack[id].frameRate = pMOVInfo->VideoFrameRate;
	NMR_MOVMUST("^R gp_vfr %d\r\n", g_movTrack[id].frameRate);
	g_movTrack[id].uiH264GopType = pMOVInfo->uiH264GopType;
	gp_movEntry = (MOV_Ientry *)g_movEntryAddr[id];
	NMR_MOVMUST("^R gp_movEntry=0x%lx\r\n", (unsigned long)gp_movEntry);
	if (gp_movEntry) {
		g_movTrack[id].cluster = gp_movEntry;//&g_movEntry[0];
		g_movTrack[id].entry_head = g_entry_pos[id].vid_head;
		g_movTrack[id].entry_tail = g_entry_pos[id].vid_tail;
		g_movTrack[id].entry_max = (MOV_Ientry *)g_movEntryAddr[id] + g_movEntryMax[id];
	} else {
		return MOV_STATUS_ENTRYADDRERR;
	}
	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	//g_movTrack[id].uiRotateInfo = pMOVInfo->uiVideoRotate;
	//#NT#2013/04/17#Calvin Chang -end
	//--------------VIDEO track end -----------------
	//--------------VIDEO sub track begin -----------------
#if 1//add for sub-stream -begin
	if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
		g_sub_movCon[id].codec_type = REC_VIDEO_TRACK;
		g_sub_movCon[id].width = pMOVInfo->SubImageWidth;
		g_sub_movCon[id].height = pMOVInfo->SubImageHeight;
		NMR_MOVMUST("id= %d!, encVideoCodec =%d!\r\n", id, pMOVInfo->SubencVideoCodec);
		if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_MJPG) {
			g_sub_movCon[id].codec_name = (char *)&g_movMJPGCodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			//g_sub_movTrack.tag = 0x6D6A7061;//mjpa (Motion JPEG A)
			g_sub_movTrack[id].tag = 0x6A706567;//jpeg (Photo JPEG)
			//g_sub_movTrack.vosLen = 0x20;
			//g_sub_movTrack.vosData = &g_movEsdsData05[0];
		} else if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_H264) {
			g_sub_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			g_sub_movTrack[id].tag = 0x61766331;//avc1
			//DBG_DUMP("^R AVC1\r\n");
			//g_sub_movTrack.vosLen = 0x20;
			//g_sub_movTrack.vosData = &g_movEsdsData05[0];
		} else if (pMOVInfo->SubencVideoCodec == MEDIAVIDENC_H265) {
			g_sub_movCon[id].codec_name = (char *)&g_movH264CodecName[0];
			g_sub_movTrack[id].context = &g_sub_movCon[id];
			g_sub_movTrack[id].tag = 0x68766331;//hvc1
			//g_sub_movTrack.vosLen = 0x20;
			//g_sub_movTrack.vosData = &g_movEsdsData05[0];
			//DBG_DUMP("^G HVC1\r\n");
		} else {
			DBG_ERR("pMOVInfo->SubencVideoCodec %d ERROR!!!!\r\n", (int)pMOVInfo->SubencVideoCodec);
			DBG_ERR("DO NOT UPDATE HEADER!!!\r\n");
			return 0;
		}
		NMR_MOVMUST("id= %d!, context = 0x%lx!\r\n", id, (UINT32)&g_sub_movCon[id]);
		//g_sub_movCon.rc_buffer_size = 0x2EE000;
		//g_sub_movCon.rc_max_rate = 0xA00F00;
		if (pMOVInfo->SubvideoFrameCount < g_sub_movEntryMax[id]) { //MOV_ENTRY_MAX)
			g_sub_movTrack[id].entry = pMOVInfo->SubvideoFrameCount;
			//DBGD(pMOVInfo->SubvideoFrameCount);
		} else {
			g_sub_movTrack[id].entry = g_sub_movEntryMax[id];//MOV_ENTRY_MAX;
			//DBGD(g_sub_movEntryMax[id]);
		}
		NMR_MOVMUST("[%d] vidfrcnt=0x%lx\r\n", id, pMOVInfo->SubvideoFrameCount);
		g_sub_movTrack[id].mode = MOVMODE_MOV;
		g_sub_movTrack[id].hasKeyframes = 1;
		g_sub_movTrack[id].ctime = MOVWriteCountTotalSeconds();//base 2004 01 01 0:0:0
		g_sub_movTrack[id].mtime = MOVWriteCountTotalSeconds();
		g_sub_movTrack[id].timescale = timescale;//2009/11/24 Meg Lin
		NMR_MOVMUST("trackDuration=%d!!!\r\n", (int)pMOVInfo->SubPlayDuration);
		g_sub_movTrack[id].trackDuration = pMOVInfo->SubPlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;
		if (pMOVInfo->SubPlayDuration == 0) {
			DBG_DUMP("pMOVInfo->SubPlayDuration ZERO!!!! ERROR1!!");
			return MOV_STATUS_INVALID_PARAM;
		}
		//DBG_DUMP("pMOVInfo->SubPlayDuration[%d] time %d!!", id, g_sub_movTrack[id].trackDuration);
		g_sub_movTrack[id].language = 0x00;//english
		trackID++;
		g_sub_movTrack[id].trackID = trackID;
		g_sub_movTrack[id].frameRate = pMOVInfo->SubVideoFrameRate;
		NMR_MOVMUST("^R gp_vfr %d\r\n", g_sub_movTrack[id].frameRate);
		g_sub_movTrack[id].uiH264GopType = pMOVInfo->uiH264GopType; ///???
		gp_sub_movEntry = (MOV_Ientry *)g_sub_movEntryAddr[id];
		if (gp_sub_movEntry) {
			g_sub_movTrack[id].cluster = gp_sub_movEntry;//&g_movEntry[0];
		} else {
			return MOV_STATUS_ENTRYADDRERR;
		}
	}
#endif//add for sub-stream -end
	//--------------VIDEO sub track end -----------------
	//--------------audio track begin -----------------
#if 1//2007/06/12 meg_nowave, rec_no_wave, mark this
	NMR_MOVMUST("[%d] audsize=0x%lx\r\n", id, pMOVInfo->audioTotalSize);
	NMR_MOVMUST("[%d] afn=0x%lx\r\n", id, pMOVInfo->audioFrameCount);
	if (pMOVInfo->audioTotalSize != 0) {
		g_movAudioCon[id].codec_type = REC_AUDIO_TRACK;
		g_movAudioCon[id].width = pMOVInfo->ImageWidth;
		g_movAudioCon[id].height = pMOVInfo->ImageHeight;
		if (pMOVInfo->encAudioCodec == MOVAUDENC_AAC) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_AAC;//mp4a
			g_movAudioCon[id].rc_max_rate = pMOVInfo->AudioBitRate;
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else if (pMOVInfo->encAudioCodec == MOVAUDENC_ULAW) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_ULAW;//ulaw
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else if (pMOVInfo->encAudioCodec == MOVAUDENC_ALAW) {
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_ALAW;//alaw
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		} else { // if (pMOVInfo->encAudioCodec== MOVAUDENC_PCM), default
			g_movAudioTrack[id].context = &g_movAudioCon[id];
			g_movAudioTrack[id].tag = MOV_AUDFMT_PCM;//sowt
			g_movAudioTrack[id].entry = pMOVInfo->audioFrameCount;
		}
		if (pMOVInfo->encAudioCodec == MOVAUDENC_ULAW ||
			pMOVInfo->encAudioCodec == MOVAUDENC_ALAW) {
			g_movAudioCon[id].wBitsPerSample = 0x08;
		} else {
			g_movAudioCon[id].wBitsPerSample = 0x10;
		}
		//#NT#2009/12/21#Meg Lin -begin
		//#NT#new feature: rec stereo
		g_movAudioCon[id].nchannel = pMOVInfo->AudioChannel;
		g_movAudioCon[id].nAvgBytesPerSecond = (pMOVInfo->AudioSampleRate) * 2 * pMOVInfo->AudioChannel;
		//#NT#2009/12/21#Meg Lin -end
		g_movAudioCon[id].nSamplePerSecond = pMOVInfo->AudioSampleRate;
		if (g_movAudioTrack[id].entry > g_movAudioEntryMax[id]) { //MOV_ENTRY_MAX)
			DBG_DUMP("^R toomuch id=%d, %ld, %ld\r\n", id, g_movAudioTrack[id].entry, g_movAudioEntryMax[id]);
			g_movAudioTrack[id].entry = g_movAudioEntryMax[id];//MOV_ENTRY_MAX;
		}
		g_movAudioTrack[id].context = &g_movAudioCon[id];
		//g_movAudioTrack.tag = 0x6D703476;//mp4v
		g_movAudioTrack[id].vosLen = 0x20;
		g_movAudioTrack[id].mode = MOVMODE_MOV;
		g_movAudioTrack[id].hasKeyframes = 1;
		g_movAudioTrack[id].ctime = MOVWriteCountTotalSeconds();
		g_movAudioTrack[id].mtime = MOVWriteCountTotalSeconds();
		//#NT#2009/11/24#Meg Lin -begin
		g_movAudioTrack[id].timescale = timescale;
		g_movAudioTrack[id].audioSize = pMOVInfo->audioTotalSize;
		//DBG_MSG("audiosize=%lx!\r\n", g_movAudioTrack.audioSize);
		//fixbug: countsize over UINT32
#if 0
		//if (g_movAudioTrack.audioSize > 0x600000)//0xffffffff/600 =6.8M, *1000 will exceed UINT32 size
		if (g_movAudioTrack.audioSize > 0x20000) { //0xffffffff/30000 =143165 (0x22f00), *1000 will exceed UINT32 size
			temp = g_movAudioTrack.audioSize / g_movAudioCon.nAvgBytesPerSecond;
			temp2 = g_movAudioTrack.audioSize % g_movAudioCon.nAvgBytesPerSecond;
			soundSize = (temp2 * MOV_TIMESCALE) / g_movAudioCon.nAvgBytesPerSecond + temp * MOV_TIMESCALE; //2009/04/21 MegLin 60:01 bug fix
		} else {
			soundSize = (g_movAudioTrack.audioSize * MOV_TIMESCALE) / g_movAudioCon.nAvgBytesPerSecond;
		}
#else
		temp = g_movAudioTrack[id].audioSize / g_movAudioCon[id].nAvgBytesPerSecond;
		temp2 = g_movAudioTrack[id].audioSize % g_movAudioCon[id].nAvgBytesPerSecond;
		//2013/08/28 Meg fixbug
		//if (temp2 > 0x10000) { //0xffffffff/60000 =71582 (0x1179e), *1000 will exceed UINT32 size
		if (temp2 > 0xB000) { //0xffffffff/92400 =46482 (0xB592), *1000 will exceed UINT32 size
			temp3 = temp2 / 2;
			soundSize = (temp3 * timescale) / g_movAudioCon[id].nAvgBytesPerSecond * 2;
		} else {
			soundSize = (temp2 * timescale) / g_movAudioCon[id].nAvgBytesPerSecond;
		}
		soundSize += temp * timescale; //2009/04/21 MegLin 60:01 bug fix
		//DBG_DUMP("audsize=0x%lx, nAvg=0x%lx, sound=0x%lx\r\n",g_movAudioTrack[id].audioSize, g_movAudioCon[id].nAvgBytesPerSecond ,soundSize);
#endif
		//#NT#2009/11/24#Meg Lin -end
		g_movAudioTrack[id].trackDuration = (UINT32)soundSize;
		g_movAudioTrack[id].language = 0x00;//english
		trackID++;
		g_movAudioTrack[id].trackID = trackID; //2010/01/29 Meg Lin, fixbug
		gp_movAudioEntry = (MOV_Ientry *)g_movAudEntryAddr[id];
		//DBG_DUMP("3=0x%lx\r\n", gp_movAudioEntry);
		if (gp_movAudioEntry) {
			g_movAudioTrack[id].cluster = gp_movAudioEntry;
			g_movAudioTrack[id].entry_head = g_entry_pos[id].aud_head;
			g_movAudioTrack[id].entry_tail = g_entry_pos[id].aud_tail;
			g_movAudioTrack[id].entry_max = (MOV_Ientry *)g_movAudEntryAddr[id] + g_movAudioEntryMax[id];
		} else {
			return MOV_STATUS_ENTRYADDRERR;
		}
		//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
		g_movAudioTrack[id].uiRotateInfo = 0;
		//#NT#2013/04/17#Calvin Chang -end
	}
	//--------------audio track end -----------------
#endif//2007/06/12 meg_nowave
#if 0  // remove old desc flow
	MovWriteLib_Make264VidDesc(pMOVInfo->vidDescAdr, pMOVInfo->vidDescSize, id);
#endif // remove old desc flow

	MOV_MakeAtomInit(ptr);
	MOV_SeekOffset(0, uiFtypSize, KFS_SEEK_SET);

	g_movTrack[id].pH264Desc = g_264VidDesc[id];
	g_movTrack[id].pH265Desc = g_265VidDesc[id];
	g_movTrack[id].h265Len   = g_movh265len[id];
#if 1//add for sub-stream -begin
	if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
		g_sub_movTrack[id].pH264Desc = g_sub_264VidDesc[id];
		g_sub_movTrack[id].pH265Desc = g_sub_265VidDesc[id];
		g_sub_movTrack[id].h265Len   = g_sub_movh265len[id];
	}
#endif//add for sub-stream -end

	if ((pMOVInfo->audioTotalSize != 0)) {
#if 1//add for sub-stream -begin
		if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
			uiMoovSize = MOV_Sub_WriteMOOVTag(0, &g_movTrack[id], &g_sub_movTrack[id], &g_movAudioTrack[id], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
		} else {
			uiMoovSize = MOVWriteMOOVTag(0, &g_movTrack[id], &g_movAudioTrack[id], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
		}
#else
		uiMoovSize = MOVWriteMOOVTag(0, &g_movTrack[id], &g_movAudioTrack[id], TRACK_INDEX_VIDEO | TRACK_INDEX_AUDIO);
#endif//add for sub-stream -end
	} else {
#if 1//add for sub-stream -begin
		if (pMOVInfo->SubVideoTrackEn && pMOVInfo->SubvideoFrameCount) {
			uiMoovSize = MOV_Sub_WriteMOOVTag(0, &g_movTrack[id], &g_sub_movTrack[id], 0, TRACK_INDEX_VIDEO);
		} else {
			uiMoovSize = MOVWriteMOOVTag(0, &g_movTrack[id], 0, TRACK_INDEX_VIDEO);
		}
#else
		uiMoovSize = MOVWriteMOOVTag(0, &g_movTrack[id], 0, TRACK_INDEX_VIDEO);
#endif//add for sub-stream -end
	}
	if (uiMoovSize == 0) {
		DBG_DUMP("^Rmov update header fails!!!");
		return MOV_STATUS_INVALID_PARAM;
	}
	#if 0
	headerLen += MOVWriteCUSTOMDataTag(0, &g_movTrack[id]);
	#endif
	uiMdatSize -= (UINT64)uiMoovSize;
	NMR_MOVMUST("1=0x%lx 0x%lx 0x%lx\r\n", totalmoovori, uiFtypSize, uiMoovSize);

	// 'skip' atom => uiSkipSize
	if (gMovCo64) {
		mdatTagSize = 16;
	} else {
		mdatTagSize = 8;
	}
	uiSkipSize = totalmoovori - uiFtypSize - uiMoovSize - mdatTagSize; // for size+"mdat"
	MOV_WriteB32Bits(0, uiSkipSize);
	MOV_WriteB32Tag(0, "skip");
	MOV_SeekOffset(0, totalmoovori - mdatTagSize, KFS_SEEK_SET);
	uiMdatSize -= (UINT64)uiSkipSize;

	// 'mdat' atim => uiMdatSize
	if (gMovCo64) {
		NMR_MOVMUST("use 64 bits mdat atom\r\n");
		MOV_WriteB32Bits(0, 1);
		MOV_WriteB32Tag(0, "mdat");
		MOV_WriteB64Bits(0, uiMdatSize);
	} else {
		NMR_MOVMUST("use 32 bits mdat atom\r\n");
		MOV_WriteB32Bits(0, (UINT32)uiMdatSize);//8 for size+"mdat"
		MOV_WriteB32Tag(0, "mdat");
	}

	NMR_MOVMUST("[%d] headerlen=0x%lx 0x%lx\r\n", id, uiMoovSize, totalmoovori);
	NMR_MOVMUST("freashift 0x%X moov 0x%X skip 0x%X mdat 0x%X\r\n",
		uiFreaEndSize, uiMoovSize, uiSkipSize, (UINT32)uiMdatSize);
	pMOVInfo->outMoovSize = totalmoovori;
	return MOV_STATUS_OK;
}

void MOV_EndFileWay(UINT32 value)
{
	gMovEndfileWay = value;
}
#if 0
UINT32 MOVMJPG_Temp_WriteMoov(MOVMJPG_FILEINFO *pMOVInfo)
{
	UINT32  headerLen = 0;
	UINT32  mdatSize = 0;
	UINT32  soundSize, temp, temp2;
	//UINT8   pChar[8];
	// Write out video BitStream if there is video Bitstream
	//MOVWriteWaitPreCmdFinish();//2012/07/13 Meg
	mdatSize = (UINT32)pMOVInfo->totalMdatSize + (MOV_MDAT_HEADER);//AVI_lastVideoPad
	// Close file
	DBG_IND("1=0x%lx\r\n", pMOVInfo->ptempMoovBufAddr);
	MOV_MakeAtomInit(pMOVInfo->ptempMoovBufAddr);


	//--------------VIDEO track begin -----------------
	g_movCon.codec_type = REC_VIDEO_TRACK;
	g_movCon.width = pMOVInfo->ImageWidth;
	g_movCon.height = pMOVInfo->ImageHeight;
	if (pMOVInfo->encVideoCodec == MEDIAVIDENC_MJPG) {
		g_movCon.codec_name = (char *)&g_movMJPGCodecName[0];
		g_movTrack.context = &g_movCon;
		//g_movTrack.tag = 0x6D6A7061;//mjpa (Motion JPEG A)
		g_movTrack.tag = 0x6A706567;//jpeg (Photo JPEG)
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
	} else if (pMOVInfo->encVideoCodec == MEDIAVIDENC_H264) {
		g_movCon.codec_name = (char *)&g_movH264CodecName[0];
		g_movTrack.context = &g_movCon;
		g_movTrack.tag = 0x61766331;//avc1
		//g_movTrack.vosLen = 0x20;
		//g_movTrack.vosData = &g_movEsdsData05[0];
	}

	//g_movCon.rc_buffer_size = 0x2EE000;
	//g_movCon.rc_max_rate = 0xA00F00;

	if (pMOVInfo->videoFrameCount < g_movEntryMax) { //MOV_ENTRY_MAX)
		g_movTrack.entry = pMOVInfo->videoFrameCount;
	} else {
		g_movTrack.entry = g_movEntryMax;//MOV_ENTRY_MAX;
	}
	g_movTrack.mode = MOVMODE_MOV;
	g_movTrack.hasKeyframes = 1;
	g_movTrack.ctime = MOVWriteCountTotalSeconds();//0xBC191380;//base 2004 01 01 0:0:0
	g_movTrack.mtime = MOVWriteCountTotalSeconds();//0xBC191380;

	g_movTrack.timescale = MOV_TIMESCALE;
	g_movTrack.trackDuration = pMOVInfo->PlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;
	g_movTrack.language = 0x00;//english
	g_movTrack.trackID = 1;
	DBG_IND("2=0x%lx\r\n", gp_movEntry);
	if (gp_movEntry) {
		g_movTrack.cluster = gp_movEntry;//&g_movEntry[0];
	} else {
		return MOV_STATUS_ENTRYADDRERR;
	}
	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	g_movTrack.uiRotateInfo = pMOVInfo->uiVideoRotate;
	//#NT#2013/04/17#Calvin Chang -end
	//--------------VIDEO track end -----------------

	//--------------audio track begin -----------------
	if (gMovRecFileInfo.audioTotalSize != 0) {

		g_movAudioCon.codec_type = REC_AUDIO_TRACK;
		g_movAudioCon.width = pMOVInfo->ImageWidth;
		g_movAudioCon.height = pMOVInfo->ImageHeight;
		if (pMOVInfo->encAudioCodec == MOVAUDENC_AAC) {
			//g_movCon.codec_name = (char *)&g_movMJPGCodecName[0];
			g_movAudioTrack.context = &g_movAudioCon;
			g_movAudioTrack.tag = 0x6D706461;//mp4a
			g_movAudioCon.rc_max_rate = pMOVInfo->AudioBitRate;
			g_movAudioCon.nchannel = pMOVInfo->AudioChannel;
			g_movAudioTrack.trackDuration = pMOVInfo->PlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;

			//g_movTrack.vosLen = 0x20;
			//g_movTrack.vosData = &g_movEsdsData05[0];
		} else { // if (pMOVInfo->encAudioCodec== MOVAUDENC_PCM), default
			//g_movCon.codec_name = (char *)&g_movH264CodecName[0];
			g_movAudioTrack.context = &g_movAudioCon;
			g_movAudioTrack.tag = 0x736f7774;//sowt
			//g_movTrack.vosLen = 0x20;
			//g_movTrack.vosData = &g_movEsdsData05[0];
			g_movAudioCon.nchannel = pMOVInfo->AudioChannel;
			g_movAudioTrack.trackDuration = pMOVInfo->PlayDuration;// (g_movTrack.entry * g_movTrack.timescale)/30;
		}

		g_movAudioCon.wBitsPerSample = 0x10;
		//#NT#2009/12/21#Meg Lin -begin
		//#NT#new feature: rec stereo
		g_movAudioCon.nchannel = pMOVInfo->AudioChannel;
		g_movAudioCon.nAvgBytesPerSecond = (pMOVInfo->AudioSampleRate) * 2 * pMOVInfo->AudioChannel;
		//#NT#2009/12/21#Meg Lin -end

		g_movAudioCon.nSamplePerSecond = pMOVInfo->AudioSampleRate;
		if (pMOVInfo->audioFrameCount < g_movAudioEntryMax) { //MOV_ENTRY_MAX)
			g_movAudioTrack.entry = pMOVInfo->audioFrameCount;
		} else {
			g_movAudioTrack.entry = g_movAudioEntryMax;//MOV_ENTRY_MAX;
		}
		g_movAudioTrack.context = &g_movAudioCon;
		//g_movAudioTrack.tag = 0x6D703476;//mp4v
		g_movAudioTrack.vosLen = 0x20;
		g_movAudioTrack.mode = MOVMODE_MOV;
		g_movAudioTrack.hasKeyframes = 1;
		g_movAudioTrack.ctime = MOVWriteCountTotalSeconds();//0xBC191380;
		g_movAudioTrack.mtime = MOVWriteCountTotalSeconds();//0xBC191380;

		g_movAudioTrack.timescale = MOV_TIMESCALE;
		g_movAudioTrack.audioSize = pMOVInfo->audioTotalSize;
		DBG_IND("audiosize=%lx!\r\n", g_movAudioTrack.audioSize);
		//fixbug: countsize over UINT32
		if (pMOVInfo->encAudioCodec == MOVAUDENC_PCM) { //2010/01/11 need or not??
			temp = g_movAudioTrack.audioSize / g_movAudioCon.nAvgBytesPerSecond;
			temp2 = g_movAudioTrack.audioSize % g_movAudioCon.nAvgBytesPerSecond;
			soundSize = (temp2 * MOV_TIMESCALE) / g_movAudioCon.nAvgBytesPerSecond + temp * MOV_TIMESCALE; //2009/04/21 MegLin 60:01 bug fix
			g_movAudioTrack.trackDuration = (UINT32)soundSize;
		}
		g_movAudioTrack.language = 0x00;//english
		g_movAudioTrack.trackID = 1;
		DBG_IND("3=0x%lx\r\n", gp_movAudioEntry);
		if (gp_movAudioEntry) {
			g_movAudioTrack.cluster = gp_movAudioEntry;
		} else {
			return MOV_STATUS_ENTRYADDRERR;
		}
		//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
		g_movAudioTrack.uiRotateInfo = 0;
		//#NT#2013/04/17#Calvin Chang -end
	}
	//--------------audio track end -----------------
	if ((gMovRecFileInfo.audioTotalSize != 0)) { //&&(gMovRecFileInfo.encVideoCodec==MEDIAVIDENC_MJPG)) //FAKE_264
		headerLen = MOVWriteMOOVTag(0, &g_movTrack, TRACK_INDEX_AUDIO);
	} else {
		headerLen = MOVWriteMOOVTag(0, &g_movTrack, TRACK_INDEX_VIDEO);
	}
	//rec_no_wave, use this --headerLen = MOVWriteMOOVTag(0, &g_movTrack, TRACK_INDEX_VIDEO);
	DBG_IND("headerLen=0x%lx\r\n", headerLen);
	return headerLen;
}
#endif

//@}
