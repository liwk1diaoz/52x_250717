/*
    Header file of mov recorder.

    Exported header file of mov fileformat writer.

    @file       movRec.h
    @ingroup    mIAVMOV
    @note       Nothing.
    @version    V1.00.000
    @date       2019/05/05
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _MP_MOVREC_H
#define _MP_MOVREC_H

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
#include "VideoEncode.h"
#else
#endif

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define MOV_RECDEBUG  		0
#if defined(_BSP_NA51000_)
#define MOVWRITER_COUNT  	16
#elif defined(_BSP_NA51023_)
#define MOVWRITER_COUNT     8
#elif defined(_BSP_NA51055_)
#define MOVWRITER_COUNT     16
#else
#define MOVWRITER_COUNT     16
#endif

#define MOV_META_COUNT      10


#define MOV_MDAT_BASETIME  0xBC191380;//base 2004 01 01 0:0:0


#define TRACK_INDEX_VIDEO   0x01
#define TRACK_INDEX_AUDIO   0x02

#define MOVMODE_MOV     1
#define MOVMODE_PSP     3  //only (1)Mpeg4 and (2)both video, audio can set

#define REC_VIDEO_TRACK 0
#define REC_AUDIO_TRACK 1
#define MOV_STATUS_OK               0
#define MOV_STATUS_INVALID_PARAM    1
#define MOV_STATUS_WRITE_ERR        2
#define MOV_STATUS_READ_ERR         3
#define MOV_STATUS_ENTRYADDRERR     4

#define MOV_MDAT_HEADER   8
#define MOV_FIRST_AUDIOPREROLL  0//2007/06/01 0x5800

//#define MOV_FTYP_SIZE    0x18 //2009/11/24 Meg Lin
#define MOV_FTYP_SIZE    0x1C //2019/07/09 Willy Su
#define MOV_FRE1_SIZE    0x18 //2010/01/29 Meg Lin
#define MP4_FTYP_SIZE    0x1C //2012/06/26 Hideo Lin for MP4 ftyp size

#define MOV_MEDIARECVRY_MAXCLUSIZE  0x40000 //max cluster size, 64K//2012/12/18 Meg
//must be MEDIAREC_AVI_HDR
#define MOV_MEDIARECVRY_MAXFRAME    (15*60*92)//15 min, 92fps(vid60+aud32)
#define MOV_NIDX        0x7864696e  // 'nidx'       //2011/02/18 Meg Lin
#define MOV_NOT_NIDX    0xFFFFFFFF
#define MOV_NIDX_CLUR   0x72756c63  //'clur' //2012/09/12 Meg Lin

#define MOV_SKIP        0x70696B73// 'skip'
#define MOV_FREA        0x61657266// 'frea' //2013/05/28 Meg
#define MOV_TIMA        0x616d6974// 'tima'
#define MOV_THMA        0x616d6874// 'thma'
#define MOV_SCRA        0x61726373// 'scra'
#define MOV_MEDIARECVRY_MAXBLKSIZE  0x200000 //max 2M
#define MOV_MEDIARECVRY_MAXBLKSIZE_BIG  0x600000 //max 6M > 6MB for UHD p50

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/
/*
    Memory struture for mov writer.

    Memory struture for mov writer.
    (for MEDIAREC_OBJ)

*/
typedef struct {
	UINT32 bufAddr;
	UINT32 bufSize;

} MOVWRITELIB_INFO;

typedef struct {
	UINT8               FrameResolution;
	UINT8               VideoQuality;
	UINT8               Mode;
	UINT8               rev;
	UINT32              TimeResolution;
	UINT32              BufHeaderLen;
	UINT32              BufOffsetLen;
	UINT32              BufLen;
	UINT32              BufOffsetCnt;
	UINT32              DataPacketSize;//4K

	//VIDEOCODEC          decVideoCodec;
	//AUDIOCODEC          decAudioCodec;

	//VIDEO_CODEC         encVideoCodec;
	//AUDIO_CODEC         encAudioCodec;
	UINT32              Preroll;
	UINT16              PacketPayloadType;
	UINT16              rev2;
	UINT32              TotalAudioSampleCount;
	UINT16              AudioSampleRate;
	UINT16              AudioChannel;
	UINT32              AudioBitsPerSample;
	UINT16              ImageWidth;             ///< [Out]     : mov Image Width
	UINT16              ImageHeight;            ///< [Out]     : mov Image Height
	UINT32              VideoPayloadSize;
	UINT32              AudioPayloadSize;
	UINT32              CreationDate;
	UINT32              NowFilePos;
	UINT64              IndexEntryTimeInterval; ///< [In/Out]  : Key frame Time Inerval
	UINT64              DataPacketsCount;
	UINT64              PlayDuration;
	UINT64              FileSize;
} MOV_FILE_INFO, *PMOV_FILE_INFO;
#if 0
typedef struct {
	UINT8               FrameResolution;
	UINT8               VideoQuality;
	UINT16              rev;
	UINT32              TimeResolution;
	UINT32              BufHeaderLen;
	UINT32              BufOffsetLen;
	UINT32              BufLen;
	UINT32              BufOffsetCnt;
	UINT32              DataPacketSize;//4K

	UINT8               decVideoCodec;
	UINT8               decAudioCodec;

	UINT8               encVideoCodec;
	UINT8               encAudioCodec;//MOVAUDENC_AAC
	UINT32              Preroll;
	UINT16              PacketPayloadType;
	UINT16              VideoFrameRate;
	UINT32              TotalAudioSampleCount;
	UINT16              AudioSampleRate;
	UINT16              AudioChannel;
	UINT32              AudioBitsPerSample;
	UINT32              AudioBitRate;
	UINT16              ImageWidth;             ///< [Out]     : mov Image Width
	UINT16              ImageHeight;            ///< [Out]     : mov Image Height
	UINT32              VideoPayloadSize;
	UINT32              AudioPayloadSize;
	UINT32              CreationDate;
	//UINT32              NowFilePos;
	UINT32              fileOffset;
	UINT32              audioFrameCount;
	UINT32              videoFrameCount;
	UINT64              totalMdatSize;
	UINT8               *ptempMoovBufAddr;
	UINT32              audioTotalSize;
	UINT64              IndexEntryTimeInterval; ///< [In/Out]  : Key frame Time Inerval
	UINT64              DataPacketsCount;
	UINT64              PlayDuration;
	UINT32              uiH264GopType;  // H.264 GOP type (Hideo@120322)
	UINT32              uiFileType;     // file type (Hideo@120626)

	//add for recovery -begin
	UINT32              bufaddr;
	UINT32              bufsize;
	UINT64              recv_filesize; //filesize before recovery
	UINT32              clustersize;
	UINT32              hdr_revSize;//MEDIAREC_AVI_HDR//2012/10/22 Meg
	//add for recovery -end
	//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
	UINT32              uiVideoRotate;
	//#NT#2013/04/17#Calvin Chang -end
	//UINT64              FileSize;
	UINT32              uinidxversion;
	UINT32              uiTempHdrAdr;
} MOVMJPG_FILEINFO;
#endif
typedef struct {
	UINT16 width;
	UINT16 height;//coded_width/height

	UINT64 finalsize;//final file size (including moov)
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
} MOVREC_Context;

typedef struct {
	UINT32      uiThumbAddr;        ///< thumbnail bit-stream starting address
	UINT32      uiThumbSize;        ///< thumbnail bit-stream size
	UINT32      uiScreenAddr;        ///< thumbnail bit-stream starting address
	UINT32      uiScreenSize;        ///< thumbnail bit-stream size



} MOVREC_FREAATOM; //2013/05/28 Meg

typedef struct {
	UINT32      mode;
	UINT32      entry;   //frameNum
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
	UINT32      frameRate;
	MOVREC_Context *context;

	UINT32      vosLen;
	UINT8       *vosData;
	MOV_Ientry  *cluster;
	UINT32      audio_vbr;
	UINT32      type;//video: mjpg, audio: wave
	UINT32      udta_addr;//addr of user data
	UINT32      udta_size;//size of user data
	UINT32      cudta_addr;//addr of custom data
	UINT32      cudta_size;//size of custom data
	UINT32      cudta_tag;//tag of custom data
	UINT32      fre1_addr;//addr of fre1 data //2010/01/29 Meg Lin
	UINT32      fre1_size;//size of fre1 data //2010/01/29 Meg Lin
	UINT32      gpstag_addr;//addr of gps tag addr
	UINT32      gpstag_size;//size of gps tag size
	UINT32      fileid;//file id to map gps tag buffer
	UINT32      clusterSize;//cluster size to make header
	UINT32      uiThumbAddr;        ///< thumbnail bit-stream starting address
	UINT32      uiThumbSize;        ///< thumbnail bit-stream size
	UINT32      uiScreenAddr;       ///< screennail bit-stream starting address//2013/05/28 Meg
	UINT32      uiScreenSize;       ///< screennail bit-stream size//2013/05/28 Meg
	UINT32      uiFreaSize;         ///< Frea size//2013/05/28 Meg
	UINT32      uiHeaderSize;       ///< MOV front header size
	UINT32      uiH264GopType;      ///< valid if Video track
	CHAR        *pH264Desc;         ///< pointer to H264 desc
	CHAR        *pH265Desc;         ///< pointer to H265 desc
	BOOL        bCustomUdata;       ///< customized user data
	UINT32      uiRotateInfo;       ///< rotation angle for transformation matrix setting (0,90,180,270) //2013/04/17 Calvin Chang
	UINT32      h265Len;
#if defined (__UITRON) || defined (__ECOS)
	MP_VDOENC_DAR  uiDAR;              ///< video display aspect ratio//2013/10/28
#else
	MEDIAREC_DAR   uiDAR;              ///< video display aspect ratio//2013/10/28
#endif
//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
	MOV_Ientry  *entry_head;
	MOV_Ientry  *entry_tail;
	MOV_Ientry  *entry_max;
//#NT#2019/11/22#Willy Su -end
#if 1	//add for meta -begin
	UINT32      metatag_addr;//addr of meta tag addr
	UINT32      metatag_size;//size of meta tag size
	UINT32      metaid;//file id to map meta tag buffer
#endif	//add for meta -end
} MOVREC_Track;

//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
typedef struct {
	MOV_Ientry  *vid_head;
	MOV_Ientry  *vid_tail;
	MOV_Ientry  *aud_head;
	MOV_Ientry  *aud_tail;
	UINT32      vid_num;
	UINT32      aud_num;
	UINT32      aud_shift;
} MOVREC_EntryPos;
//#NT#2019/11/22#Willy Su -end

typedef struct {
	UINT64 startAddr;
	UINT64 endAddr;
	UINT32 searchOk;
} MOVRCVYREAD_INFO;

/*-----------------------------------------------------------------------------*/
/* Extern Variables                                                            */
/*-----------------------------------------------------------------------------*/
extern UINT32                              gMovTempHDR[MOVWRITER_COUNT];
extern UINT32                              gMovTempHDRsize[MOVWRITER_COUNT];

extern MOVREC_Track                         g_movTrack[], g_movAudioTrack[];
extern MOVREC_Context                       g_movCon[];
extern MOVREC_Context                       g_movAudioCon[];
//#NT#2010/04/20#Meg Lin -begin
//#NT#Bug: video entry buffer not enough for small resolution
extern UINT32                               g_movEntryMax[];
extern UINT32                               g_movAudioEntryMax[];
extern UINT32                               g_movEntryAddr[];
extern UINT32                               g_movAudEntryAddr[];
//#NT#2010/04/20#Meg Lin -end
extern CONTAINERMAKER                       gMovContainerMaker;
extern MOVMJPG_FILEINFO                     gMovRecFileInfo[];
extern UINT32                               gMovWriteMoovTempAdr;
extern char                                 g_movMJPGCodecName[];
extern char                                 g_movH264CodecName[];
extern char                                 g_movH265CodecName[];
//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
extern MOVREC_EntryPos                      g_entry_pos[MOVWRITER_COUNT];
//#NT#2019/11/22#Willy Su -end
extern MOV_Ientry                          *gp_movEntry;
extern MOV_Ientry                          *gp_movAudioEntry;
extern char                                 g_264VidDesc[MOVWRITER_COUNT][0x60];
extern UINT32                               g_mov_nidx_timeid[];
extern UINT8                                gMovCo64;
extern UINT32                               gMovMp4_backhdrsize[];
extern char                                 g_265VidDesc[MOVWRITER_COUNT][0x200];//MEDIAWRITE_H265_STSD_MAXLEN
extern UINT32                               g_movh265len[];
extern UINT8                                g_mov_sync_pos;
extern INT32                                g_mov_utc_timeoffset;
extern struct tm                            g_mov_utc_timeinfo;
extern BOOL                                 g_mov_frea_box[];
//add for sub-stream -begin
extern MOVREC_Track                         g_sub_movTrack[];
extern MOVREC_Context                       g_sub_movCon[];
extern UINT32                               g_sub_movEntryMax[];
extern UINT32                               g_sub_movEntryAddr[];
extern MOV_Ientry                          *gp_sub_movEntry;
extern char                                 g_sub_264VidDesc[MOVWRITER_COUNT][0x60];
extern char                                 g_sub_265VidDesc[MOVWRITER_COUNT][0x200];//MEDIAWRITE_H265_STSD_MAXLEN
extern UINT32                               g_sub_movh265len[];
//add for sub-stream -end

/*-----------------------------------------------------------------------------*/
/* Extern Functions                                                            */
/*-----------------------------------------------------------------------------*/
extern UINT32 Mov_Swap(UINT32 value);
extern UINT32 MOVWriteSTCOTag(char *pb, MOVREC_Track *ptrack);
extern UINT32 MOVWriteSTSDTag(char *pb, MOVREC_Track *ptrack);
extern UINT32 MOVWriteSTSSTag(char *pb, MOVREC_Track *ptrack);
extern UINT32 MOVWriteMOOVTag(char *pb, MOVREC_Track *ptrack, MOVREC_Track *pAudtrack, UINT8 trackIndex);
extern UINT32 MOVMJPG_UpdateHeader(UINT32 id, MOVMJPG_FILEINFO *pMOVInfo);
//extern UINT32 MOVMJPG_UpdateHeaderByName(MOVMJPG_FILEINFO *pMOVInfo, char *pName);//2012/07/13 Meg
extern UINT32 MOVMJPG_Temp_WriteMoov(MOVMJPG_FILEINFO *pMOVInfo);
extern void MOV_SetIentry(UINT32 id, UINT32 index, MOV_Ientry *ptr);
extern void MOV_SetAudioIentry(UINT32 id, UINT32 index, MOV_Ientry *ptr);
extern void MOV_Write_SetEntryAddr(UINT32 addr, UINT32 size, UINT32 sampleRate, UINT32 vidFR, UINT32 id);
extern UINT32 MOVMJPG_GetMoovAtomSize(UINT32 sec);
extern void MOVMJPG_SetEditOffset(UINT32 minusSize);
extern void MOVMJPG_CopyVideoEntry(UINT32 startEntry, UINT32 total);
extern void MOVMJPG_CopyAudioEntry(UINT32 startEntry, UINT32 total);
extern void MOVMJPG_ResetEntry(void);//only for writing a new file after read
extern void MOVMJPG_SetVideoEntryNum(UINT32 total);
extern void MOVMJPG_SetAudioEntryNum(UINT32 total);
extern void MOV_MakeAtomInit(void *pMovBaseAddr);
extern void MOV_UpdateVentry(UINT32 sec, UINT32 flashsec, UINT32 fstVideoPos, UINT32 fr, UINT32 id);
extern void MOV_UpdateAentry(UINT32 sec, UINT32 flashsec, UINT32 fstAudPos, UINT32 id);
extern void MOV_UpdateVidEntryPos(UINT32 id, UINT32 index, MOV_Ientry *ptr);
extern void MOV_UpdateAudEntryPos(UINT32 index, MOV_Ientry *ptr);
extern void MOV_ResetEntry(UINT32 id);
extern UINT32 MOVRcvy_MakeMoovHeader(MOVMJPG_FILEINFO *pMOVInfo);//for recovery, same with MOVMJPG_UpdateHeader
extern ER MOVRcvy_IndexTlb_Recovery(UINT32 id);
extern void MovWriteLib_Make264VidDesc(UINT32 addr, UINT32 len, UINT32 id);
extern UINT32 MovWriteLib_Make265VidDesc(UINT32 addr, UINT32 len, UINT32 id);
extern UINT32 MOV_Write_CalcV(UINT32 size, UINT32 sampleRate, UINT32 vidFR);
extern UINT32 MOVWriteFREATag(char *pb, MEDIAREC_HDR_OBJ *pHdr, UINT32 outAddr);//2013/05/28 Meg
extern UINT32 MOVWriteCUSTOMDataTag(char *pb, MOVREC_Track *ptrack);
extern void MOV_SetGPSEntryAddr(UINT32 addr, UINT32 size, UINT32 id);
extern UINT32 MOV_Write_GetEntrySize(UINT32 sampleRate, UINT32 vidFR, UINT32 sec);
extern void MOV_SetIentry_old32(UINT32 id, UINT32 index, MOV_old32_Ientry *ptr);
extern void MOV_SetAudioIentry_old32(UINT32 id, UINT32 index, MOV_old32_Ientry *ptr);
//#NT#2019/11/22#Willy Su -begin
//#NT#To support entry sync pos
extern UINT32 MOV_SyncPos(UINT32 id, UINT32 index, UINT32 bHead);
extern void MOV_Write_InitEntryAddr(UINT32 id);
extern void MOV_SetIentry_SyncPos(UINT32 id, UINT32 index, MOV_Ientry *ptr);
extern void MOV_SetAudioIentry_SyncPos(UINT32 id, UINT32 index, MOV_Ientry *ptr);
//#NT#2019/11/22#Willy Su -end
extern UINT32 MOV_Wrtie_GetTimeScale(UINT32 vfr);
extern UINT32 MOV_Wrtie_GetDuration(UINT32 vfn, UINT32 vfr);
extern UINT32 MOV_Wrtie_GetEntryDuration(UINT32 id, UINT32 vfn, UINT32 vfr);
extern UINT32 MOVMJPG_MakeMoov(UINT32 id, MOVMJPG_FILEINFO *pMOVInfo);
//add for sub-stream -begin
extern void   MOV_Sub_SetIentry(UINT32 id, UINT32 index, MOV_Ientry *ptr);
extern void   MOV_Sub_Make264VidDesc(UINT32 addr, UINT32 len, UINT32 id);
extern UINT32 MOV_Sub_Make265VidDesc(UINT32 addr, UINT32 len, UINT32 id);
extern UINT32 MOV_Sub_WriteMOOVTag(char *pb, MOVREC_Track *ptrack, MOVREC_Track *pSubtrack, MOVREC_Track *pAudtrack, UINT8 trackIndex);
extern UINT32 MOV_Write_CalcMainV(UINT32 size, UINT32 sampleRate, UINT32 vidFR, UINT32 subFR);
extern UINT32 MOV_Write_CalcSubV(UINT32 size, UINT32 sampleRate, UINT32 vidFR, UINT32 subFR);
//add for sub-stream -end
#if 1	//add for meta -begin
extern void MOV_SetMetaEntryAddr(UINT32 addr, UINT32 size, UINT32 id);
#endif	//add for meta -end
#endif

