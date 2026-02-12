/**
    @brief 		Header file of bsmux library.

    @file 		bsmux_init.h

    @ingroup 	mBsMux

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef BSMUX_INT_H
#define BSMUX_INT_H

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include <kwrap/error_no.h>
#include <kwrap/perf.h>
#include <kwrap/type.h>
#include <kwrap/cmdsys.h>
#include <kwrap/sxcmd.h>
#include <kwrap/stdio.h>
#include <kwrap/util.h>
#include <kwrap/verinfo.h>
#include <hd_gfx.h>
#include "FileSysTsk.h"
#include <time.h>
#include <comm/hwclock.h>
#include "avfile/movieinterface_def.h"
#include "avfile/MOVLib.h"
#include "avfile/MediaWriteLib.h"
#include "avfile/AVFile_MakerMov.h"
#include "avfile/AVFile_MakerTS.h"
#include "Utility/avl.h"
#include "hd_bsmux_lib.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          BSMUX
#define __DBGLVL__          2
#include "kwrap/debug.h"
extern unsigned int BSMUX_debug_level;
///////////////////////////////////////////////////////////////////////////////

// BACKWARD COMPATIBLE
#define ISF_UNIT_BSMUX                   0
#define ISF_MAX_BSMUX                    1
#define ISF_IN_BASE                      128
#define ISF_OUT_BASE                     0
#define ISF_BSMUX_IN_NUM                 32
#define ISF_BSMUX_OUT_NUM                16

#define ISF_BSMUX_MDAT_BLK_MAX           128
#define ISF_BSMUX_ENCRYPTO_BLK           256

#define ISF_OK                           E_OK
#define ISF_ERR_FAILED                   E_SYS

#define MP_AUDENC_AAC_RAW_BLOCK          1024

#define NMEDIAREC_ALIGN_CEIL             1 //round up
#define NMEDIAREC_ALIGN_ROUND            2 //round off

#define BSMUXER_CALCSEC_UNKNOWN_FSSIZE   (UINT64)0xFFFFFFFF

#define BSMUXER_BSQ_ROLLBACK_SECNEW      300 //300 GOPs = 30fps (120sec) = 120fps (30sec)
#define BSMUXER_BSQ_MAX_SECENTRY         120 //60 fps +47 aac

#define BSMUXER_FEVENT_NORMAL            HD_BSMUX_FEVENT_NORMAL
#define BSMUXER_FEVENT_EMR               HD_BSMUX_FEVENT_EMR
#define BSMUXER_FEVENT_BSINCARD          HD_BSMUX_FEVENT_BSINCARD

#define BSMUXER_FOP_NONE                 HD_BSMUX_FOP_NONE
#define BSMUXER_FOP_CREATE               HD_BSMUX_FOP_CREATE
#define BSMUXER_FOP_CLOSE                HD_BSMUX_FOP_CLOSE
#define BSMUXER_FOP_CONT_WRITE           HD_BSMUX_FOP_CONT_WRITE
#define BSMUXER_FOP_SEEK_WRITE           HD_BSMUX_FOP_SEEK_WRITE
#define BSMUXER_FOP_FLUSH                HD_BSMUX_FOP_FLUSH
#define BSMUXER_FOP_DISCARD              HD_BSMUX_FOP_DISCARD
#define BSMUXER_FOP_READ                 HD_BSMUX_FOP_READ_WRITE
#define BSMUXER_FOP_SNAPSHOT             HD_BSMUX_FOP_SNAPSHOT

#define BSMUX_REC_AUDIOINFO              HD_BSMUX_AUDIOINFO
#define BSMUX_GPS_DATA                   HD_BSMUX_GPS_DATA
#define BSMUX_USER_DATA                  HD_BSMUX_USER_DATA
#define NMEDIA_REC_EMR_INFO              HD_BSMUX_TRIG_EMR
#define BSMUX_CRYPTO                     HD_BSMUX_CRYPTO
#define BSMUX_CALC_SEC                   HD_BSMUX_CALC_SEC
#define BSMUX_CALC_SEC_SETTING           HD_BSMUX_CALC_SEC_SETTING
#define BSMUX_REC_EXTINFO                HD_BSMUX_EXTINFO

#define BSMUX_OUT_DATA                   HD_BSMUX_OUT_DATA
#define BSMUXER_OUT_BUF                  HD_BSMUX_OUT_DATA

#define BSMUXER_RESULT_NORMAL            HD_BSMUX_ERRCODE_NONE
#define BSMUXER_RESULT_SLOWMEDIA         HD_BSMUX_ERRCODE_SLOWMEDIA
#define BSMUXER_RESULT_LOOPREC_FULL      HD_BSMUX_ERRCODE_LOOPREC_FULL
//#define BSMUXER_RESULT_CARD_FULL         HD_BSMUX_ERRCODE_CARD_FULL
#define BSMUXER_RESULT_OVERTIME          HD_BSMUX_ERRCODE_OVERTIME
#define BSMUXER_RESULT_MAXSIZE           HD_BSMUX_ERRCODE_MAXSIZE
#define BSMUXER_RESULT_VANOTSYNC         HD_BSMUX_ERRCODE_VANOTSYNC
#define BSMUXER_RESULT_GOPMISMATCH       HD_BSMUX_ERRCODE_GOPMISMATCH
#define BSMUXER_RESULT_PROCDATAFAIL      HD_BSMUX_ERRCODE_PROCDATAFAIL
#define BSMUXER_RESULT                   HD_BSMUX_ERRCODE

#define BSMUXER_CBEVENT_PUTBSDONE        HD_BSMUX_CB_EVENT_PUTBSDONE
#define BSMUXER_CBEVENT_FOUTREADY        HD_BSMUX_CB_EVENT_FOUTREADY
#define BSMUXER_CBEVENT_KICKTHUMB        HD_BSMUX_CB_EVENT_KICKTHUMB
#define BSMUXER_CBEVENT_CUT_COMPLETE     HD_BSMUX_CB_EVENT_CUT_COMPLETE
#define BSMUXER_CBEVENT_CLOSE_RESULT     HD_BSMUX_CB_EVENT_CLOSE_RESULT
#define BSMUXER_CBEVENT_COPYBSBUF        HD_BSMUX_CB_EVENT_COPYBSBUF
#define BSMUXER_CBEVENT_CUT_BEGIN        HD_BSMUX_CB_EVENT_CUT_BEGIN
#define BSMUXER_CBEVENT                  HD_BSMUX_CB_EVENT

#define BSMUX_EVENT_CB                   HD_BSMUX_CALLBACK
#define BSMUXER_CBINFO                   HD_BSMUX_CBINFO
#define BSMUXER_OBJ                      HD_BSMUX_REG_CALLBACK

/**
    Video Encoding Slice Type
*/
typedef enum {
	MP_VDOENC_P_SLICE = 0,
	MP_VDOENC_B_SLICE = 1,
	MP_VDOENC_I_SLICE = 2,
	MP_VDOENC_IDR_SLICE = 3,
	MP_VDOENC_KP_SLICE = 4,
	ENUM_DUMMY4WORD(MP_VDOENC_SLICE_TYPE)
} MP_VDOENC_SLICE_TYPE;

typedef enum {
	AUDIO_CH_LEFT,              ///< Left
	AUDIO_CH_RIGHT,             ///< Right
	AUDIO_CH_STEREO,            ///< Stereo
	AUDIO_CH_MONO,              ///< Mono two channel. Obselete. Shall not use this option.
	AUDIO_CH_DUAL_MONO,         ///< Dual Mono Channels. Valid for record(RX) only.

	ENUM_DUMMY4WORD(AUDIO_CH)
} AUDIO_CH;

/**
    file buf.
    @note for push out to fileout
*/
typedef struct _BSMUXER_FILE_BUF {
	UINT32 				pathID;					   ///< keep path id
	UINT32              event;                     ///< BSMux event
	UINT32              fileop;                    ///< bitwise: open/close/conti_write or seek_write/flush/none(event only)/discard
	UINT32              addr;                      ///< write data address
	UINT64              size;                      ///< write data size
	UINT64              pos;                       ///< only valid if seek_write
	UINT32              type;                      ///< file type, MP4, TS, JPG, THM
} BSMUXER_FILE_BUF;

// INCLUDE PROTECTED
#include "bsmux_mdat.h"
#include "bsmux_ts.h"
#include "bsmux_cb.h"
#include "bsmux_ctrl.h"
#include "bsmux_id.h"
#include "bsmux_tsk.h"
#include "bsmux_util.h"

/*-----------------------------------------------------------------------------*/
/* Macro Function Definitions                                                  */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define BSMUX_MAX_PATH_NUM      16 //NMR_MAX_TOTALMDAT
#define BSMUX_MAX_CTRL_NUM      16 //NMR_MAX_TOTALMDAT
#define BSMUX_MAX_BSIN_NUM      64
#define BSMUX_MAX_META_NUM      10

#define BSMUX_BSQ_ROLLBACK_SEC  68 //68 GOPs = 30fps (30sec) = 120fps (7sec), from NMEDIAREC_BSQ_ROLLBACK_SECORI

#define BSMUX_FILEINFO_TOTALVF  1 //vidCopyCount //NMRBS_FILEINFO_TOTALVF
#define BSMUX_FILEINFO_TOTALAF  2 //NMRBS_FILEINFO_TOTALAF
#define BSMUX_FILEINFO_NOWSEC   3 //NMRBS_FILEINFO_NOWSEC
#define BSMUX_FILEINFO_ADDVF    4 //vidCount //NMRBS_FILEINFO_ADDVF
#define BSMUX_FILEINFO_COPYPOS  5 //copyPos
#define BSMUX_FILEINFO_TOTALGPS 6 //gpsCopyCount
#define BSMUX_FILEINFO_TOTAL    7 //copyTotalCount
#define BSMUX_FILEINFO_FRONTPUT 8 //front put
#define BSMUX_FILEINFO_EXTNUM   9
#define BSMUX_FILEINFO_EXTSEC   10
#define BSMUX_FILEINFO_BLKPUT   11
#define BSMUX_FILEINFO_SUBVF    12
#define BSMUX_FILEINFO_THUMBPUT 13
#define BSMUX_FILEINFO_EXTNSEC  14
#define BSMUX_FILEINFO_TOTALVF_DROP 15
#define BSMUX_FILEINFO_TOTALAF_DROP 16
#define BSMUX_FILEINFO_SUBVF_DROP 17
#define BSMUX_FILEINFO_CUTBLK   18
#define BSMUX_FILEINFO_PUT_VF   19
#define BSMUX_FILEINFO_PUT_AF   20
#define BSMUX_FILEINFO_PUT_SUBVF 21
#define BSMUX_FILEINFO_ROLLSEC  22
#define BSMUX_FILEINFO_VAR_RATE 23
#define BSMUX_FILEINFO_TOTALMETA 24 //meta copy count
#define BSMUX_FILEINFO_TOTALMETA2 25 //meta copy count

//mdat header info
#define BSMUX_HDRMEM_MAX        4  //4 for the same path files going to card //NMR_MDAT_HDRMEM_MAX
#define BSMUX_HDRNUM_MAX        2

#define BSMUX_HDRMEM_NONE       0 //not hdr
#define BSMUX_HDRMEM_FRONT      1 //NMR_MDAT_HDRMEM_FRONT
#define BSMUX_HDRMEM_TEMP       2 //temp for copy front header to update filepos=0 //NMR_MDAT_HDRMEM_TEMP
#define BSMUX_HDRMEM_BACK       3 //NMR_MDAT_HDRMEM_BACK

#define BSMUX_BS2CARD_2M        0x200000 //NMEDIAREC_BS2CARD_STEP2M
#define BSMUX_BS2CARD_1M        0x100000 //NMEDIAREC_BS2CARD_STEP1M
#define BSMUX_BS2CARD_500K      0x80000 //NMEDIAREC_BS2CARD_STEP0_5M
#define BSMUX_BS2CARD_NORMAL    0 //not hdr
#define BSMUX_BS2CARD_FRONT     1 //NMR_MDAT_HDRMEM_FRONT
#define BSMUX_BS2CARD_TEMP      2 //temp for copy front header to update filepos=0 //NMR_MDAT_HDRMEM_TEMP
#define BSMUX_BS2CARD_BACK      3 //NMR_MDAT_HDRMEM_BACK
#define BSMUX_BS2CARD_LAST      4
#define BSMUX_BS2CARD_MOOV      5
#define BSMUX_BS2CARD_THUMB     6
#define BSMUX_BS2STRG_WRITE     7
#define BSMUX_BS2STRG_SYNC      8
#define BSMUX_BS2STRG_FLUSH     9
#define BSMUX_BS2STRG_READ      10

#define BSMUX_RESERVED_FILESIZE 0x10000 //NMR_RESERVED_FILESIZE,
#define BSMUX_HEADER_MIN        BSMUX_RESERVED_FILESIZE //NMRMDAT_HEADER_MIN
#define BSMUX_IDX1SIZE_NOMRAL   0x200000 // 2M for 30fps 60 min //NMRMDAT_IDX1SIZE_NOMRAL
#define BSMUX_GPS_MIN           0x4000  // 16 K //NMRMDAT_GPS_MIN
#define BSMUX_MAX_NIDX_BLK      0x2000//8K //NMEDIAREC_MAX_NIDX_BLK
#define BSMUX_VIDEO_DESC_SIZE   0x200   // 512 bytes //NMTMDAT_VIDEO_DESC_SIZE

#define BSMUX_ROLLBACKSEC_MIN   3
#define BSMUX_ROLLBACKSEC_MAX   30
#define BSMUX_KEEPSEC_MIN       5
#define BSMUX_KEEPSEC_MAX       60
#define BSMUX_RESVSEC_MIN       7
#define BSMUX_RESVSEC_MAX       30
#define BSMUX_PLAYVFR_15        15
#define BSMUX_PLAYVFR_30        30
#define BSMUX_NIDX_RESVSEC      4
#define BSMUX_LOCKNUMMIN        2 //one for our own, one for output
#define BSMUX_NORMALNUMMIN      3 //one for our own, one for padding reserved, one for output
#define BSMUX_DUR_US_MAX        6000000 //us (2s)
#define BSMUX_DUR_MS_MAX        6000    //ms (2s)

#define BSMUX_CTRL_IDX          0xF000
#define BSMUX_INVALID_IDX       0xFFFF

#define BSMUX_STEP_EMRREC      0x0001
#define BSMUX_STEP_EMRLOOP     0x0002

//TEST
#define BSMUX_TEST_FORCE_SW     0
#define BSMUX_TEST_GPS_ON       0
#define BSMUX_TEST_USER_ON      0
#define BSMUX_TEST_TS_FORMAT    0
#define BSMUX_TEST_CHK_FIRSTI   1
#define BSMUX_TEST_SET_MS       1
#define BSMUX_TEST_NALU         1
#define BSMUX_TEST_MULTI_TILE   1
#define BSMUX_TEST_RESV_BUFF    1
#define BSMUX_TEST_MOOV_RC      1
#define BSMUX_TEST_USE_HEAP     1

//DEBUG
#define BSMUX_DEBUG_CBINFO      0
#define BSMUX_DEBUG_MEMBUF      0
#define BSMUX_DEBUG_BSQ         0
#define BSMUX_DEBUG_TSKINFO     0
#define BSMUX_DEBUG_ACTION      0
#define BSMUX_DEBUG_HEADER      0
#define BSMUX_DEBUG_BITRATE     0
#define BSMUX_DEBUG_MDAT        0
#define BSMUX_DEBUG_TS          0
#define BSMUX_DEBUG_MSG         0

typedef enum {
	BSMUXER_PARAM_MIN,
	//Basic Settings: THUMB
	BSMUXER_PARAM_THUMB_ON,     //thumb on/off
	BSMUXER_PARAM_THUMB_ADDR,   //thumb addr
	BSMUXER_PARAM_THUMB_SIZE,   //thumb size
	//Basic Settings: VID
	BSMUXER_PARAM_VID_WIDTH,    //vid wid
	BSMUXER_PARAM_VID_HEIGHT,   //vid height
	BSMUXER_PARAM_VID_VFR,      //vid frame rate
	BSMUXER_PARAM_VID_TBR,      //vid target bitrate
	BSMUXER_PARAM_VID_CODECTYPE,    //vid codectype
	BSMUXER_PARAM_VID_DESCADDR,     //vid desc addr
	BSMUXER_PARAM_VID_DESCSIZE,     //vid desc size
	BSMUXER_PARAM_VID_DAR,     //vid display aspect ratio, MP_VDOENC_DAR_DEFAULT
	BSMUXER_PARAM_VID_GOP,
	BSMUXER_PARAM_VID_NALUNUM,
	BSMUXER_PARAM_VID_VPSSIZE,
	BSMUXER_PARAM_VID_SPSSIZE,
	BSMUXER_PARAM_VID_PPSSIZE,
	BSMUXER_PARAM_VID_VAR_VFR,
	//Basic Settings: SUB VID
	BSMUXER_PARAM_VID_SUB_WIDTH,    //vid wid
	BSMUXER_PARAM_VID_SUB_HEIGHT,   //vid height
	BSMUXER_PARAM_VID_SUB_VFR,      //vid frame rate
	BSMUXER_PARAM_VID_SUB_TBR,      //vid target bitrate
	BSMUXER_PARAM_VID_SUB_CODECTYPE,    //vid codectype
	BSMUXER_PARAM_VID_SUB_DESCADDR,     //vid desc addr
	BSMUXER_PARAM_VID_SUB_DESCSIZE,     //vid desc size
	BSMUXER_PARAM_VID_SUB_DAR,     //vid display aspect ratio, MP_VDOENC_DAR_DEFAULT
	BSMUXER_PARAM_VID_SUB_GOP,
	BSMUXER_PARAM_VID_SUB_NALUNUM,
	BSMUXER_PARAM_VID_SUB_VPSSIZE,
	BSMUXER_PARAM_VID_SUB_SPSSIZE,
	BSMUXER_PARAM_VID_SUB_PPSSIZE,
	//Basic Settings: AUD
	BSMUXER_PARAM_AUD_CODECTYPE,//MOVAUDENC_AAC or others
	BSMUXER_PARAM_AUD_SR,       //audio sample rate
	BSMUXER_PARAM_AUD_CHS,      //audio channels
	BSMUXER_PARAM_AUD_ON,       //audio on/off
	BSMUXER_PARAM_AUD_EN_ADTS,    //audio BS ADTS shift bytes. default =0
	//Basic Settings: FILE
	BSMUXER_PARAM_FILE_EMRON,   //emr on
	BSMUXER_PARAM_FILE_EMRLOOP,   //emrloop on
	BSMUXER_PARAM_FILE_STRGID,  //storage id (0 or 1)
	BSMUXER_PARAM_FILE_DDR_ID,  //ddr_id
	BSMUXER_PARAM_FILE_SEAMLESSSEC,     //seamlessSec
	BSMUXER_PARAM_FILE_ROLLBACKSEC,     //rollbacksec (default:1)
	BSMUXER_PARAM_FILE_KEEPSEC, //keepsec (only for EMR)
	BSMUXER_PARAM_FILE_ENDTYPE, //endtype, MOVREC_ENDTYPE_CUTOVERLAP or others
	BSMUXER_PARAM_FILE_FILETYPE,//filetype, MEDIA_FILEFORMAT_MP4 or others
	BSMUXER_PARAM_FILE_RECFORMAT,//recformat, MEDIAREC_AUD_VID_BOTH or others
    BSMUXER_PARAM_FILE_PLAYFRAMERATE, //default as 30
	BSMUXER_PARAM_FILE_BUFRESSEC,    //buffer reserved sec, default 5 (5~30)
	BSMUXER_PARAM_FILE_OVERLAP_ON,    //overlap on/off, default OFF
	BSMUXER_PARAM_FILE_PAUSE_ON,    // for emr pause
	BSMUXER_PARAM_FILE_PAUSE_ID,    // for emr pause
	BSMUXER_PARAM_FILE_PAUSE_CNT,    // for emr pause
	BSMUXER_PARAM_FILE_SEAMLESSSEC_MS,
	BSMUXER_PARAM_FILE_ROLLBACKSEC_MS,
	BSMUXER_PARAM_FILE_KEEPSEC_MS,
	BSMUXER_PARAM_FILE_BUFRESSEC_MS,
	//Basic Settings: BUF
	BSMUXER_PARAM_BUF_VIDENC_ADDR,  // videnc bs phy_addr
	BSMUXER_PARAM_BUF_VIDENC_VIRT,  // videnc bs virt_addr
	BSMUXER_PARAM_BUF_VIDENC_SIZE,  // videnc bs buf_size
	BSMUXER_PARAM_BUF_AUDENC_ADDR,  // audenc bs phy_addr
	BSMUXER_PARAM_BUF_AUDENC_VIRT,  // videnc bs virt_addr
	BSMUXER_PARAM_BUF_AUDENC_SIZE,  // audenc bs buf_size
	BSMUXER_PARAM_BUF_SUB_VIDENC_ADDR,  // videnc bs phy_addr
	BSMUXER_PARAM_BUF_SUB_VIDENC_VIRT,  // videnc bs virt_addr
	BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE,  // videnc bs buf_size
	//Basic Settings: HDR
	BSMUXER_PARAM_HDR_FRAMEBUF_ADDR,
	BSMUXER_PARAM_HDR_FRAMEBUF_SIZE,
	BSMUXER_PARAM_HDR_FRONTHDR_ADDR,
	BSMUXER_PARAM_HDR_FRONTHDR_SIZE,
	BSMUXER_PARAM_HDR_BACKHDR_ADDR,
	BSMUXER_PARAM_HDR_BACKHDR_SIZE,
	BSMUXER_PARAM_HDR_TEMPHDR_ADDR,
	BSMUXER_PARAM_HDR_TEMPHDR_SIEZ,
	BSMUXER_PARAM_HDR_TEMPHDR_1_ADDR,
	BSMUXER_PARAM_HDR_TEMPHDR_1_SIEZ,
	BSMUXER_PARAM_HDR_NIDX_ADDR,  //nidxaddr
	BSMUXER_PARAM_HDR_GSP_ADDR,   //gpsaddr
	//Basic Settings: GPS
	BSMUXER_PARAM_GPS_ON,     //gps on/off
	BSMUXER_PARAM_GPS_RATE,
	BSMUXER_PARAM_GPS_QUEUE,
	//Basic Settings: MEM
	BSMUXER_PARAM_MEM_ADDR,         // bsmux phy_addr
	BSMUXER_PARAM_MEM_VIRT,         // bsmux virt_addr
	BSMUXER_PARAM_MEM_SIZE,         // bamux buf_size
	BSMUXER_PARAM_MEM_END,
	BSMUXER_PARAM_MEM_WRTBLK,
	BSMUXER_PARAM_PRECALC_BUFFER,    //buffer to calculat min bsmuxer second
	//Basic Settings: NIDX
	BSMUXER_PARAM_NIDX_EN,
	BSMUXER_PARAM_NIDX_VFN,
	BSMUXER_PARAM_NIDX_SUB_VFN,
	BSMUXER_PARAM_NIDX_AFN,
	//Utility: USERDATA
	BSMUXER_PARAM_USERDATA_ON,
	BSMUXER_PARAM_USERDATA_ADDR,
	BSMUXER_PARAM_USERDATA_SIZE,
	//Utility: CUSTDATA
	BSMUXER_PARAM_CUSTDATA_ADDR,
	BSMUXER_PARAM_CUSTDATA_SIZE,
	BSMUXER_PARAM_CUSTDATA_TAG,
	//Utility: WRINFO
	BSMUXER_PARAM_WRINFO_FLUSH_FREQ,
	BSMUXER_PARAM_WRINFO_WRBLK_CNT,
	BSMUXER_PARAM_WRINFO_WRBLK_SIZE,
	BSMUXER_PARAM_WRINFO_CLOSE_FLAG,
	BSMUXER_PARAM_WRINFO_WRBLK_LOCKSEC,
	BSMUXER_PARAM_WRINFO_WRBLK_TIMEOUT,
	//Utility: EXTINFO
	BSMUXER_PARAM_EXTINFO_UNIT,
	BSMUXER_PARAM_EXTINFO_MAX_NUM,
	BSMUXER_PARAM_EXTINFO_ENABLE,
	//Utility: MOOVINFO
	BSMUXER_PARAM_FRONT_MOOV,
	BSMUXER_PARAM_MOOV_ADDR,
	BSMUXER_PARAM_MOOV_SIZE,
	BSMUXER_PARAM_MOOV_FREQ,
	BSMUXER_PARAM_MOOV_TUNE,
	//Utility: DROPINFO
	BSMUXER_PARAM_EN_DROP,
	BSMUXER_PARAM_VID_DROP,
	BSMUXER_PARAM_AUD_DROP,
	BSMUXER_PARAM_SUB_DROP,
	BSMUXER_PARAM_DAT_DROP,
	BSMUXER_PARAM_VID_SET,
	BSMUXER_PARAM_AUD_SET,
	BSMUXER_PARAM_SUB_SET,
	BSMUXER_PARAM_DAT_SET,
	BSMUXER_PARAM_FULL_SET,
	//Utility: UTC
	BSMUXER_PARAM_UTC_SIGN,  //1: negative | 0: positive
	BSMUXER_PARAM_UTC_ZONE,
	BSMUXER_PARAM_UTC_TIME,
	//Utility: ALIGN
	BSMUXER_PARAM_MUXALIGN,
	BSMUXER_PARAM_MUXMETHOD,
	//Utility: META
	BSMUXER_PARAM_META_ON,     //gps on/off
	BSMUXER_PARAM_META_NUM,
	BSMUXER_PARAM_META_DATA,
	//Others
	BSMUXER_PARAM_EN_POINTTHM,
	BSMUXER_PARAM_MAX_SIZE,
	BSMUXER_PARAM_EMR_NEXTSEC,   //second after emr finish to start next emr, default 3(3~10)
	BSMUXER_PARAM_FREASIZE,
	BSMUXER_PARAM_FREAENDSIZE,
	BSMUXER_PARAM_EN_FREABOX,
	BSMUXER_PARAM_EN_FASTPUT,
	BSMUXER_PARAM_DUR_US_MAX,
	BSMUXER_PARAM_BOXTAG_SIZE,
	BSMUXER_PARAM_PTS_RESET,
	//Utility: STRGBUF
	BSMUXER_PARAM_EN_STRGBUF,
	BSMUXER_PARAM_STRGBUF_ACT,
	BSMUXER_PARAM_STRGBUF_CUT,
	BSMUXER_PARAM_STRGBUF_HDR,
	BSMUXER_PARAM_STRGBUF_VID,
	BSMUXER_PARAM_STRGBUF_VID_NUM,
	BSMUXER_PARAM_STRGBUF_AUD_NUM,
	BSMUXER_PARAM_STRGBUF_CUR_POS,
	BSMUXER_PARAM_STRGBUF_MAX_NUM,
	BSMUXER_PARAM_STRGBUF_ALLOC_SIZE,
	BSMUXER_PARAM_STRGBUF_TOTAL_SIZE,
	BSMUXER_PARAM_STRGBUF_PUT_POS,
	BSMUXER_PARAM_STRGBUF_GET_POS,
	BSMUXER_PARAM_MAX,
	BSMUXER_EVENT_MIN,

	BSMUXER_EVENT_MAX,
	ENUM_DUMMY4WORD(BSMUXER_PARAM)
} BSMUXER_PARAM;

typedef enum _BSMUX_CTRL_INFO {
	BSMUX_CTRL_MIN,
	//bsq addr
	BSMUX_CTRL_NOWADDR,       //nowaddr: now address in copy buffer
	BSMUX_CTRL_LAST2CARD,     //last2card: last address in copy buffer to card
	BSMUX_CTRL_REAL2CARD,     //real2card: file size has in card => change to addr
	BSMUX_CTRL_END2CARD,      //fileend_addr
	//size related
	BSMUX_CTRL_TOTAL2CARD,    //total2card: total file size has been add to FS Queue
	//wrtie block
	BSMUX_CTRL_WRITEBLOCK,    //blksize: one blksize
	BSMUX_CTRL_WRITEOFFSET,   //blkoffset: cur offset
	BSMUX_CTRL_USEDBLOCK,     //blkusednum: used blk num
	//buf range
	BSMUX_CTRL_BUFADDR,       //buf addr (va)
	BSMUX_CTRL_BUFSIZE,       //buf size
	BSMUX_CTRL_BUFEND,
	BSMUX_CTRL_USEABLE,
	BSMUX_CTRL_FREESIZE,
	BSMUX_CTRL_USEDSIZE,

	BSMUX_CTRL_MAX,
	ENUM_DUMMY4WORD(BSMUX_CTRL_INFO)
} BSMUX_CTRL_INFO;

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

//            [WAITING HIT] -- hit --> [HIT TO RUN]
//                ^                        |
//                |                        v
//  [IDLE]  -- resume                --> [RUN]
//          <-- [PUTALL] <-- suspend --
//

/**
    type: bsmux tsk status.
*/
typedef enum _BSMUX_STATUS {
	BSMUX_STATUS_MIN = 0,
	BSMUX_STATUS_IDLE,
	BSMUX_STATUS_RUN,
	//>> BSMUX_ACTION_MAKE_HEADER
	//>> BSMUX_ACTION_SAVE_ENTRY
	//>> BSMUX_ACTION_CUTFILE
	BSMUX_STATUS_SUSPEND,
	//>> BSMUX_ACTION_UPDATE_HEADER
	//>> BSMUX_ACTION_OVERLAP_1SEC
	BSMUX_STATUS_RESUME,
	//>> BSMUX_ACTION_WAITING_HIT
	//>> BSMUX_ACTION_WAITING_RUN
	BSMUX_STATUS_MAX,
	ENUM_DUMMY4WORD(BSMUX_STATUS)
} BSMUX_STATUS;

/**
    type: bsmux tsk action.
*/
typedef enum _BSMUX_ACTION {
	BSMUX_ACTION_NONE			= 0x00000000,
	//common
	BSMUX_ACTION_WAITING_HIT	= 0x00000001,
	BSMUX_ACTION_WAITING_RUN	= 0x00000002,
	BSMUX_ACTION_CUTFILE		= 0x00000004,
	BSMUX_ACTION_OVERLAP_1SEC	= 0x00000008,
	BSMUX_ACTION_WAITING_IDLE	= 0x00000010,
	BSMUX_ACTION_ADDLAST		= 0x00000020,
	BSMUX_ACTION_PUT_FRONT		= 0x00000040,
	BSMUX_ACTION_PUT_LAST		= 0x00000080,
	//mdat
	BSMUX_ACTION_MAKE_HEADER	= 0x00000100,
	BSMUX_ACTION_UPDATE_HEADER	= 0x00000200,
	BSMUX_ACTION_SAVE_ENTRY		= 0x00000400,
	BSMUX_ACTION_PAD_NIDX		= 0x00000800,
	BSMUX_ACTION_MAKE_MOOV      = 0x00001000,
	//ts
	BSMUX_ACTION_MAKE_PES		= 0x00010000,
	BSMUX_ACTION_MAKE_PAT		= 0x00020000,
	BSMUX_ACTION_MAKE_PMT		= 0x00040000,
	BSMUX_ACTION_MAKE_PCR		= 0x00080000,
	//2v1a
	BSMUX_ACTION_REORDER_1SEC	= 0x01000000,
	ENUM_DUMMY4WORD(BSMUX_ACTION)
} BSMUX_ACTION;

/**
    type: bsmux tsk ops.
*/
typedef enum _BSMUX_TSK_OPS {
	BSMUX_TSK_OPS_MAKE_HEADER  = 0x0001,
	BSMUX_TSK_OPS_COPY2BUFFER  = 0x0002,
	BSMUX_TSK_OPS_GETALL2END   = 0x0004,
	BSMUX_TSK_OPS_UPDATEHEADER = 0x0008,
	BSMUX_TSK_OPS_SUSPENDRESET = 0x0010,
	BSMUX_TSK_OPS_WAITINGHIT   = 0x0020,
	ENUM_DUMMY4WORD(BSMUX_TSK_OPS)
} BSMUX_TSK_OPS;

/**
    input data type.
    @note modified from NMEDIA_REC_BSQ_TYPE
*/
typedef enum _BSMUX_TYPE {
	BSMUX_TYPE_VIDEO     = 0x0001,
	BSMUX_TYPE_AUDIO     = 0x0002,
	BSMUX_TYPE_THUMB     = 0x0004,
	BSMUX_TYPE_RAWEN     = 0x0008,
	BSMUX_TYPE_GPSIN     = 0x0010,
	BSMUX_TYPE_USERT     = 0x0020,
	BSMUX_TYPE_USRCB     = 0x0040,
	BSMUX_TYPE_NIDXT     = 0x0100,
	BSMUX_TYPE_SUBVD     = 0x0200,
	BSMUX_TYPE_MTAIN     = 0x0400,
	ENUM_DUMMY4WORD(BSMUX_TYPE)
} BSMUX_TYPE;

/**
    input data.
    @note modified from BSMUXER_MDATDATA
    @PUSH IN: VIDEO / AUDIO / THUMB / RAW / GPS / USER
*/
typedef struct _BSMUXER_DATA {
	UINT32 type;                    ///< signature type
	UINT32 bSMemAddr;               ///< physical address of encoded data
	UINT32 bSSize;                  ///< size of encoded data
	UINT32 isKey;                   ///< if video frame type I-frame
	UINT32 bSVirAddr;               ///< virtual address of encoded data
	UINT32 bufid;                   ///< memory block
	UINT32 naluaddr;
	UINT32 nalusize;
	UINT64 bSTimeStamp;
	UINT32 naluVirAddr;
	UINT32 naluVPSSize;
	UINT32 naluSPSSize;
	UINT32 naluPPSSize;
	UINT32 naluNum;        //tile
	UINT32 naluOftMemAddr; //tile
	UINT32 naluOftVirAddr; //tile
	VOID * user_data;
	UINT32 meta_data_num;
	UINT32 meta_data_mem_addr;
	UINT32 meta_data_vir_addr;
} BSMUXER_DATA;


/**
    struct: bsmux tsk ctrl obj.
    @note modified from NMEDIA_REC_MDAT_OBJ, NMEDIA_TS_OBJ
*/
typedef struct _BSMUX_TSK_CTRL_OBJ {
	BSMUX_STATUS Status;
	BSMUX_ACTION Action;
} BSMUX_TSK_CTRL_OBJ;

/**
    struct: bsmux tsk ctrl info.
*/
typedef struct _BSMUX_TSK_CTRL_INFO {
	UINT32 vid_copy_cnt;
	UINT32 aud_copy_cnt;
} BSMUX_TSK_CTRL_INFO;

/**
    struct: bsmux mem buf.
    @note modified from NMEDIASAVEQ_MEMBUF, NMEDIA_TS_BUF, NMEDIA_TS_FILE_INFO
*/
typedef struct _BSMUX_MEM_BUF {
	//mem
	UINT32 addr;	//&&NMEDIA_TS_BUF
	UINT32 size;	//&&NMEDIA_TS_BUF
	UINT32 end;
	UINT32 max;
	//bsq addr
	UINT32 nowaddr;		// && NMEDIA_TS_BUF
	UINT32 last2card;   //last address in copy buffer to card && NMEDIA_TS_BUF
	UINT32 real2card;   //file size has in card && NMEDIA_TS_BUF => change to addr
	UINT32 end2card;    //end of file / fileend_addr && NMEDIA_TS_BUF
	//size related
	UINT32 total2card;  //total file size has been add to FS Queue && NMEDIA_TS_FILE_INFO
	UINT32 allowmaxsize;   //size in FS queue but not in card
	//wrtie block
	UINT32 blksize;     //one blksize
	UINT32 blkoffset;   //cur offset
	UINT32 blkusednum;
	//buf usage
	UINT32 free_size;
	UINT32 used_size;
} BSMUX_MEM_BUF;

/**
    struct: bsmux mux info.
*/
typedef struct _BSMUX_MUX_INFO {
	UINT32 phy_addr;
	UINT32 vrt_addr;
} BSMUX_MUX_INFO;

/**
    struct: bsmux saveq pos info.
    @note modified from NMEDIASAVEQ_INFO
*/
typedef struct _BSMUX_SAVEQ_POS_INFO {
	UINT32 front;
	UINT32 rear;
	UINT32 full;
	UINT32 max;
} BSMUX_SAVEQ_POS_INFO;

/**
    struct: bsmux saveq bs info.
    @note modified from NMEDIA_REC_BSQ_INFO
*/
#define BSMUX_SAVEQ_BS_INFO_SIZE 12
typedef struct _BSMUX_SAVEQ_BS_INFO {
	UINT32 uiType;                  ///< signature type
	UINT32 uiBSMemAddr;             ///< physical address of encoded data
	UINT32 uiBSSize;                ///< size of encoded data
	UINT32 uiIsKey;                 ///< if video frame type I-frame
	UINT32 uiBSVirAddr;             ///< virtual address of encoded data
	UINT32 uibufid;                 ///< memory block (if meta as memory block index)
	UINT32 uiPortID;                ///< output port id
	UINT32 uiCountSametype;
	UINT64 uiTimeStamp;
	UINT32 uiTOftMemAddr;
	UINT32 uiTOftVirAddr;
} BSMUX_SAVEQ_BS_INFO;
STATIC_ASSERT(sizeof(BSMUX_SAVEQ_BS_INFO) / sizeof(UINT32) == BSMUX_SAVEQ_BS_INFO_SIZE);

/**
    struct: bsmux saveq file info.
    @note modified from NMEDIABSQ_FILEINFO, NMEDIA_TS_FILE_INFO
*/
typedef struct _BSMUX_SAVEQ_FILE_INFO {
	UINT32 vidCount;        //vid count for entry (has added)
	UINT32 copyTotalCount;  //total count for entry (has copyed)
	UINT32 vidCopyCount;    //vid count for entry (has copyed, for moov) && NMEDIA_TS_FILE_INFO
	UINT32 audCopyCount;    //aud count for entry (has copyed, for moov) && NMEDIA_TS_FILE_INFO
	UINT64 nowFileOft;      //BS add,  file offset
	UINT64 copyPos;         //copy ok, file position
	UINT32 gpsCopyCount;    //gps count for entry (has copyed, for moov)
	UINT32 vidSec;          //rec sec, count from vidFrameRate
	UINT32 headerbyte;      //hdr byte per second
	UINT32 vidCopySec;      //rec copy sec (has copyed, for moov)
	UINT32 headerok;        //headerok
	UINT32 thumbok;         //thumbok
	UINT32 extNum;
	UINT32 extSec;
	UINT32 blkPut;
	UINT32 subvidCopyCount;
	UINT32 extNextSec;
	UINT32 vidDropCount;
	UINT32 audDropCount;
	UINT32 subvidDropCount;
	UINT32 cut_blk_count;
	UINT32 vid_put_count;
	UINT32 aud_put_count;
	UINT32 sub_put_count;
	UINT32 rollack_sec;
	UINT32 variable_rate;
	UINT32 meta_copy_count[BSMUX_MAX_META_NUM];
} BSMUX_SAVEQ_FILE_INFO;

/**
    video info (project setting).
*/
typedef struct _BSMUX_REC_VIDEOINFO {
	UINT32 vidcodec;                  ///< vidoe codec type (using HD_BSMUX_VIDCODEC)
	UINT32 vfr;                       ///< vidoe frame rate (round)(playback)
	UINT32 width;                     ///< video width
	UINT32 height;                    ///< video height
	UINT32 descAddr;                  ///<
	UINT32 descSize;                  ///<
	UINT32 tbr;                       ///< target bitrate Bytes/sec
	UINT32 DAR;                       ///< MP_VDOENC_DAR_DEFAULT or others
	UINT32 gop;                       ///< gop
	UINT32 naluNum;
	UINT32 vpsSize;
	UINT32 spsSize;
	UINT32 ppsSize;
	UINT32 var_vfr;
} BSMUX_REC_VIDEOINFO;

/**
    file info.
    @note modified from MDAT_FILEINFO
*/
typedef struct _BSMUX_REC_FILEINFO {
	BSMUX_REC_VIDEOINFO vid;
	BSMUX_REC_VIDEOINFO sub_vid;
	BSMUX_REC_AUDIOINFO aud;
	UINT32 emron;
	UINT32 emrloop;
	UINT32 strgid;
	UINT32 ddr_id;
	UINT32 seamlessSec;
	UINT32 rollbacksec;	//video rollback sec
	UINT32 keepsec;		//emr keep sec
	UINT32 endtype;		//MOVREC_ENDTYPE_CUTOVERLAP
	UINT32 filetype;	//MEDIA_FILEFORMAT_MP4
	UINT32 recformat;	//MEDIAREC_AUD_VID_BOTH as default
	UINT32 playvfr;  	//30 as default, only works in MEDIAREC_TIMELAPSE/MEDIAREC_GOLFSHOT
	UINT32 revsec;  // buffer reserved sec [gNMR_bsm_revsec]
	UINT32 overlop_on; //overlap on/off [gNMR_rollbackOverlap]
	UINT32 pause_on;   // for trig emr pause
	UINT32 pause_id;   // for trig emr pause
	UINT32 pause_cnt;   // for trig emr pause
	UINT32 seamlessSec_ms;
	UINT32 rollbacksec_ms;
	UINT32 keepsec_ms;
	UINT32 revsec_ms;
} BSMUX_REC_FILEINFO;

/**
    bufinfo.
*/
typedef struct _BSMUX_REC_BUFINFO {
	UINT32 videnc_phy_addr;
	UINT32 videnc_virt_addr;
	UINT32 videnc_size;
	UINT32 audenc_phy_addr;
	UINT32 audenc_virt_addr;
	UINT32 audenc_size;
	UINT32 sub_videnc_phy_addr;
	UINT32 sub_videnc_virt_addr;
	UINT32 sub_videnc_size;
} BSMUX_REC_BUFINFO;

/**
    gps info.
    @note modified from MDAT_GPSINFO
*/
typedef struct _BSMUX_REC_GPSINFO {
	UINT32 gpson;
	UINT32 gpsdataadr;
	UINT32 gpsdatasize;
	UINT32 gps_rate;
	UINT32 gps_queue;
} BSMUX_REC_GPSINFO;

/**
    thumb info.
    @note modified from MDAT_THUMBINFO
*/
typedef struct _BSMUX_REC_THUMBINFO {
	UINT32 thumbon;
	UINT32 thumbadr;
	UINT32 thumbsize;
} BSMUX_REC_THUMBINFO;

/**
    mem info.
    @note modified from MDAT_MEMINFO & BSMUXER_MEM_RANGE
*/
typedef struct _BSMUX_REC_MEMINFO {
	UINT32 addr;
	UINT32 phy_addr;
	UINT32 virt_addr;
	UINT32 size;
	UINT32 end;
	UINT32 calc_buf; // [SET PRECALC_BUFFER] [GET ALLOC_SIZE]
} BSMUX_REC_MEMINFO;

/**
    header info.
    @note modified from MDAT_HDRINFO
*/
typedef struct _BSMUX_REC_HDRINFO {
	UINT32 frontheader;
	UINT32 backheader;
	UINT32 backsize;
	UINT32 framebuf;
	UINT32 framesize;
	UINT32 nidxaddr;
	UINT32 gpsaddr;//gps data in mdat
	UINT32 gpsbuffer;//gps entry
	UINT32 gpsbufsize;
	UINT32 gpstagaddr;//gps atom in moov
	UINT32 gpstagsize;
	UINT32 frontheader_2;
	UINT32 frontheader_1;
} BSMUX_REC_HDRINFO;

/**
    nidx info.
    @note modified from MDAT_NIDXINFO
*/
typedef struct {
	UINT32 nidx_en;
	UINT32 nidxok_vfn;
	UINT32 nidxok_sub_vfn;
	UINT32 nidxok_afn;
} BSMUX_REC_NIDXINFO;

/**
    userdata info.
    @note modified from NMR_MDATFILE_INFO
*/
typedef struct _BSMUX_REC_USERINFO {
	UINT32 on;
	UINT32 addr;
	UINT32 size;
} BSMUX_REC_USERINFO;

/**
    wrinfo.
*/
typedef struct _BSMUX_REC_WRINFO {
	UINT32 flush_freq;
	UINT32 wrblk_count;
	UINT32 wrblk_size;
	UINT32 close_flag;
	UINT32 wrblk_locksec;
	UINT32 wrblk_timeout;
} BSMUX_REC_WRINFO;

/**
    moovinfo.
*/
typedef struct _BSMUX_REC_MOOVINFO {
	UINT32 fmoov_on;
	UINT32 moov_addr;
	UINT32 moov_size;
	UINT32 moov_freq;
	UINT32 moov_tune;
} BSMUX_REC_MOOVINFO;

/**
    dropinfo.
*/
typedef struct _BSMUX_REC_DROPINFO {
	UINT32 enable;
	UINT32 vidcount;
	UINT32 audcount;
	UINT32 subcount;
	UINT32 datcount;
	UINT32 vid_drop_set;
	UINT32 aud_drop_set;
	UINT32 sub_drop_set;
	UINT32 dat_drop_set;
	UINT32 full_drop_set;
} BSMUX_REC_DROPINFO;

/**
    zone-utcinfo.
*/
typedef struct _BSMUX_REC_ZONEINFO {
	UINT32 utc_sign;
	UINT32 utc_zone;
} BSMUX_REC_ZONEINFO;

/**
    meta info.
*/
typedef struct _BSMUX_REC_METADATA {
	UINT32 meta_sign;
	UINT32 meta_rate;
	UINT32 meta_queue;
	UINT32 meta_index; //0 ~ (BSMUX_MAX_META_NUM-1)
} BSMUX_REC_METADATA;
typedef struct _BSMUX_REC_METAINFO {
	UINT32 meta_on;
	UINT32 meta_num;
	BSMUX_REC_METADATA meta_data[BSMUX_MAX_META_NUM];
} BSMUX_REC_METAINFO;

/**
    storage-buffer-info.
*/
typedef struct _BSMUX_STRGBUF_INFO {
	UINT32 en;                  //function enable
	// status
	UINT32 active;              //function active or not
	UINT32 cut;                 //need to cut and use bs buffer
	UINT32 is_hdr;              //header made
	UINT32 vid_ok;              //video data ready
	// frame entry
	UINT32 vid_entry_num;       //video entry num in card : as vidCopyCount
	UINT32 aud_entry_num;       //audio entry num in card : ad audCopyCount
	UINT64 cur_entry_pos;       //video entry pos in card : as copyPos
	UINT32 max_entry_num;       //max entry num => sec * vfr
	// card space
	UINT32 alloc_size;          //fallocate space size => (sec + resv) * tbr
	UINT32 total_size;          //total entry size (alignment) : as total2card
	UINT32 put_pos;             //last pos to put bs (if use, need move to blk align)
	UINT32 get_pos;             //first pos to get bs
} BSMUX_STRGBUF_INFO;

/**
    record setting info.
    @note modified from MDAT_ONEINFO
*/
typedef struct _BSMUX_REC_INFO {
	BSMUX_REC_FILEINFO  file;
	BSMUX_REC_BUFINFO   buf;
	BSMUX_REC_GPSINFO   gps;
	BSMUX_REC_THUMBINFO thumb;
	BSMUX_REC_HDRINFO   hdr;
	BSMUX_REC_MEMINFO   mem;
	BSMUX_REC_NIDXINFO  nidxinfo;
	BSMUX_REC_USERINFO  userdata;
	CUSTOMDATA          custdata;
	BSMUX_REC_WRINFO    wrinfo;
	BSMUX_REC_EXTINFO   extinfo;
	BSMUX_REC_MOOVINFO  moov;
	BSMUX_REC_DROPINFO  drop;
	BSMUX_REC_ZONEINFO  zone;
	BSMUX_REC_METAINFO  meta;
	UINT32              freasize;//out freasize
	UINT32              freaendsize;//out freasize
	UINT32              start;//starting
	UINT32              freabox_en;
	UINT32              fastput_en;
	UINT32              drop_en;
	UINT32              dur_us_max;
	UINT32              mux_align;
	UINT32              mux_method;
	UINT32              boxtag_size;
	UINT32              pts_reset;
	UINT32              padding_size;	////fast put flow ver3
	BSMUX_STRGBUF_INFO  strgbuf_info;
} BSMUX_REC_INFO;

/**
    block info.
    @note modified from NMR_BSMUX_BLOCK_INFO
    @note for make header use (mdat)
*/
typedef struct _BSMUX_BLOCK_INFO {
	UINT32 dataAddr;
	UINT32 dataSize;
	UINT32 uiAudio; //is audio or not
	UINT32 padSize;
	UINT32 ifUpdateFlashQ; //if update flash Q
	UINT32 ifNidx;
	UINT32 givenYes;//give a specific fileoffset to write! 0 means continue
	UINT32 gettemp;
	UINT64 givenPos;//pos if giveYes =1
} BSMUX_BLOCK_INFO;

/**
    header mem info.
    @note modified from NMR_MDAT_HDRMEM_INFO
    @note for make header use (mdat)
*/
typedef struct _BSMUX_HDRMEM_INFO {
	UINT32 hdrtype;
	UINT32 hdraddr;
	UINT32 hdrsize;
	UINT32 hdrused;
} BSMUX_HDRMEM_INFO;

/**
	bs timestamp sync info
	@not for v/a entry sync
*/
typedef struct _BSMUX_SYNC_INFO {
	UINT64 BsTimeStamp;
	UINT64 BsDuration;
	UINT32 FrmNum;
	UINT16 TimeSyncAlm;
	UINT16 FrmSyncAlm;
	UINT16 Tolerance;
} BSMUX_SYNC_INFO;

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/
//common (15)
typedef ER BSMUX_DBG(UINT32 value);
typedef ER BSMUX_TSKOBJ_INIT(UINT32 id, UINT32 *action);
typedef ER BSMUX_ENG_OPEN(VOID);
typedef ER BSMUX_ENG_CLOSE(VOID);
typedef UINT32 BSMUX_ENG_COPY(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method);
typedef ER BSMUX_MEM_GETSIZE(UINT32 id, UINT32 *p_size);
typedef ER BSMUX_MEM_SETSIZE(UINT32 id, UINT32 addr, UINT32 *p_size);
typedef ER BSMUX_CLEAN(UINT32 id);
typedef ER BSMUX_UPDATE_VIDINFO(UINT32 id, UINT32 p1, void *p_bsq);
typedef ER BSMUX_RELEASE_BUF(void *pBuf);
typedef ER BSMUX_ADD_GPS(UINT32 id, void *p_bsq);
typedef ER BSMUX_ADD_THUMB(UINT32 id, void *p_bsq);
typedef ER BSMUX_ADD_LAST(UINT32 id);
typedef ER BSMUX_PUT_LAST(UINT32 id);
typedef ER BSMUX_ADD_META(UINT32 id, void *p_bsq);
//sepcific (7)
typedef ER BSMUX_SAVE_ENTRY(UINT32 id, void *p_bsq);
typedef ER BSMUX_NIDX_PAD(UINT32 id);
typedef ER BSMUX_MAKE_HEADER(UINT32 id, void *p_maker);
typedef ER BSMUX_UPDATE_HEADER(UINT32 id, void *p_maker);
typedef ER BSMUX_MAKE_PES(UINT32 id, void *p_bsq);
typedef ER BSMUX_MAKE_PAT(UINT32 id, UINT32 addr);
typedef ER BSMUX_MAKE_MOOV(UINT32 id, void *p_maker, UINT32 minus1sec);

typedef struct _BSMUX_OPS {
	UINT32                Type;
	//common (15)
	BSMUX_DBG             *Dbg;
	BSMUX_TSKOBJ_INIT     *TskObjInit;
	BSMUX_ENG_OPEN        *EngOpen;
	BSMUX_ENG_CLOSE       *EngClose;
	BSMUX_ENG_COPY        *EngCpy;
	BSMUX_MEM_GETSIZE     *MemGetSize;
	BSMUX_MEM_SETSIZE     *MemSetSize;
	BSMUX_CLEAN           *Clean;
	BSMUX_UPDATE_VIDINFO  *UpdateVidInfo;
	BSMUX_RELEASE_BUF     *ReleaseBuf;
	BSMUX_ADD_GPS         *AddGPS;
	BSMUX_ADD_THUMB       *AddThumb;
	BSMUX_ADD_LAST        *AddLast;
	BSMUX_PUT_LAST        *PutLast;
	BSMUX_ADD_META        *AddMeta;
	//sepcific (7)
	BSMUX_SAVE_ENTRY      *SaveEntry;
	BSMUX_NIDX_PAD        *NidxPad;
	BSMUX_MAKE_HEADER     *MakeHeader;
	BSMUX_UPDATE_HEADER   *UpdateHeader;
	BSMUX_MAKE_PES        *MakePES;
	BSMUX_MAKE_PAT        *MakePAT;
	BSMUX_MAKE_MOOV       *MakeMoov;
} BSMUX_OPS;

#endif //BSMUX_INT_H
