/**
	@brief Header file of bitstream muxer module.\n
	This file contains the functions which is related to bsmux in the chip.

	@file hd_bsmux_lib.h

	@ingroup mlib

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef	_HD_BSMUX_LIB_H_
#define	_HD_BSMUX_LIB_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_common.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define HD_BSMUX_MAX_CTRL         16   ///< max count of ctrl of this device (interface)
#define HD_BSMUX_MAX_IN           64   ///< max count of input of this device (interface)
#define HD_BSMUX_MAX_OUT          64   ///< max count of output of this device (interface)
#define HD_BSMUX_MAX_DATA_TYPE     4   ///< max count of output pool of this device (interface)

/**
    @name file operation definitions (bitwise)
    @note modified from uitron
*/
#define HD_BSMUX_FOP_NONE         0x00000000 ///< Do nothing. Used for event only
#define HD_BSMUX_FOP_CREATE       0x00000001 ///< Create a new file (if old one exists, it will be truncated)
#define HD_BSMUX_FOP_CLOSE        0x00000002 ///< Close the file
#define HD_BSMUX_FOP_CONT_WRITE   0x00000004 ///< Write continously (from current position)
#define HD_BSMUX_FOP_SEEK_WRITE   0x00000008 ///< Write after seeking
#define HD_BSMUX_FOP_FLUSH        0x00000010 ///< Flush the data right away after writing
#define HD_BSMUX_FOP_DISCARD      0x00000100 ///< Discard operations which not processed yet
#define HD_BSMUX_FOP_READ_WRITE   0x00001000 ///< Read data from sysinfo and write continously (from current position)
#define HD_BSMUX_FOP_SNAPSHOT     (HD_BSMUX_FOP_CREATE | HD_BSMUX_FOP_CONT_WRITE | HD_BSMUX_FOP_CLOSE) ///< Create a single file in one operation

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/
#define HD_BSMUX_SET_COUNT(a, b)        ((a)*10)+(b)	///< ex: use HD_BSMUX_SET_COUNT(1, 5) for setting 1.5
#define HD_BSMUX_SET_MS(a, b)           ((a)*1000)+(b)	///< ex: use HD_BSMUX_SET_MS(1, 500) for setting 1.5 sec

#define HD_BSMUX_UTC_DATE(y, m, d)      ((y << 16) | ((m & 0x00ff) << 8) | (d & 0x00ff))
#define HD_BSMUX_UTC_TIME(h, m, s)      ((h << 16) | ((m & 0x00ff) << 8) | (s & 0x00ff))
#define HD_BSMUX_UTC_YEAR(d)            ((d) >> 16)
#define HD_BSMUX_UTC_MON(d)             (((d) & 0xff00) >> 8)
#define HD_BSMUX_UTC_DAY(d)             ((d) & 0x00ff)
#define HD_BSMUX_UTC_HOUR(t)            ((t) >> 16)
#define HD_BSMUX_UTC_MIN(t)             (((t) & 0xff00) >> 8)
#define HD_BSMUX_UTC_SEC(t)             ((t) & 0x00ff)

#define HD_LIB_BSMUX_COUNT	      32
#define HD_LIB_BSMUX_BASE	      0
#define HD_LIB_BSMUX(did)	      (HD_LIB_BSMUX_BASE + (did))
#define HD_LIB_BSMUX_MAX	      (HD_LIB_BSMUX_BASE + HD_LIB_BSMUX_COUNT - 1)

#define HD_BSMUX_CTRL(dev_id)           ((HD_LIB_BSMUX(dev_id) << 16) | HD_CTRL)
#define HD_BSMUX_IN(dev_id, in_id)      ((HD_LIB_BSMUX(dev_id) << 16) | (((in_id) & 0x00ff) << 8))
#define HD_BSMUX_OUT(dev_id, out_id)    ((HD_LIB_BSMUX(dev_id) << 16) | ((out_id) & 0x00ff))

/********************************************************************
	TYPE DEFINITION
********************************************************************/
/**
     @name file event definitions
     @note modified from uitron (should be the same as HD_FILEOUT_FEVENT_XXX)
*/
typedef enum _HD_BSMUX_FEVENT {
	HD_BSMUX_FEVENT_NORMAL              = 0x00000000,    ///< normal recording
	HD_BSMUX_FEVENT_EMR                 = 0x00000001,    ///< emergency recording
	HD_BSMUX_FEVENT_BSINCARD            = 0x00000002,    ///< stored in cards temporarily
	ENUM_DUMMY4WORD(HD_BSMUX_FEVENT)
} HD_BSMUX_FEVENT;

/**
    @name file type definitions.
    @note modified from uitron (should be the same as MEDIA_FILEFORMAT_XXX)
*/
typedef enum _HD_BSMUX_FTYPE {
	HD_BSMUX_FTYPE_MOV                  = 0x00000001,    ///< MOV file format
	HD_BSMUX_FTYPE_MP4                  = 0x00000004,    ///< MP4 file format
	HD_BSMUX_FTYPE_TS                   = 0x00000008,    ///< TS file format
	ENUM_DUMMY4WORD(HD_BSMUX_FTYPE)
} HD_BSMUX_FTYPE;

/**
    bsmux output data descriptor.
    @note modified from BSMUXER_FILE_BUF
    @CALLBACK: FOUT
*/
#define HD_BSMUX_DESC_SIZE  38 ///< size of data desc in WORDS
typedef struct _HD_BSMUX_OUT_DATA {
	UINT32              sign;                       ///< MAKEFOURCC('F','O','U','T')
	UINT32              event;                      ///< bsmux fevent
	UINT32              fileop;                     ///< bitwise: open/close/conti_write or seek_write/flush/none(event only)/discard
	UINT32              addr;                       ///< write data address
	UINT64              size;                       ///< write data size
	UINT64              pos;                        ///< only valid if seek_write
	UINT32              type;                       ///< file type, MP4, TS, JPG, THM
	UINT32              fpath_size;                 ///< Length of file path
	CHAR                *p_fpath;                   ///< Specify file path (if NULL, it will callback project to get)
	INT32               (*fp_pushed)(void *p_data); ///< callback for previous module release buffer
	INT32               ret_push;                   ///< fileop result [ER/INT32]
	UINT32              pathID;					    ///< keep path id
	UINT32              resv[23];
	VOID                *p_user_data;
} HD_BSMUX_OUT_DATA;
STATIC_ASSERT(sizeof(HD_BSMUX_OUT_DATA) / sizeof(UINT32) == HD_BSMUX_DESC_SIZE);

/**
    @name callback event type.
    @note modified from BSMUXER_CALLBACKEVENT
*/
typedef enum _HD_BSMUX_CB_EVENT {
	HD_BSMUX_CB_EVENT_PUTBSDONE         = 0x00000001,    ///< put bs done
	HD_BSMUX_CB_EVENT_FOUTREADY         = 0x00000002,    ///< fout data ready
	HD_BSMUX_CB_EVENT_KICKTHUMB         = 0x00000004,    ///< <obsolete>
	HD_BSMUX_CB_EVENT_CUT_COMPLETE      = 0x00000008,    ///< cut file complete
	HD_BSMUX_CB_EVENT_CLOSE_RESULT      = 0x00000010,    ///< result causing module close
	HD_BSMUX_CB_EVENT_COPYBSBUF         = 0x00000020,    ///< copy bs done
	HD_BSMUX_CB_EVENT_CUT_BEGIN         = 0x00000040,    ///< cut file begin
	ENUM_DUMMY4WORD(HD_BSMUX_CB_EVENT)
} HD_BSMUX_CB_EVENT;

/**
    @name callback error code.
    @note modified from BSMUXER_CALLBACKEVENT
*/
typedef enum _HD_BSMUX_ERRCODE {
	HD_BSMUX_ERRCODE_NONE               = 0x00000000,    ///< normal
	HD_BSMUX_ERRCODE_SLOWMEDIA          = 0x00000001,    ///< slow media to restart
	HD_BSMUX_ERRCODE_LOOPREC_FULL       = 0x00000004,    ///< <obsolete>
	HD_BSMUX_ERRCODE_OVERTIME           = 0x00000020,    ///< <obsolete>
	HD_BSMUX_ERRCODE_MAXSIZE            = 0x00000040,    ///< <obsolete>
	HD_BSMUX_ERRCODE_VANOTSYNC          = 0x00000080,    ///< vid/aud not sync
	HD_BSMUX_ERRCODE_GOPMISMATCH        = 0x00000100,    ///< gop not match setting
	HD_BSMUX_ERRCODE_PROCDATAFAIL       = 0x00000200,    ///< proc push data fail
	ENUM_DUMMY4WORD(HD_BSMUX_ERRCODE)
} HD_BSMUX_ERRCODE;

/**
    callback information.
    @note modified from BSMUXER_CALLBACK_INFO
    @CALLBACK: FOUT
*/
typedef struct _HD_BSMUX_CBINFO {
	UINT32 				id;           ///< path id
	HD_BSMUX_OUT_DATA *	out_data;     ///< output data
	HD_BSMUX_ERRCODE 	errcode;      ///< callback error (if normal be none)
	HD_BSMUX_CB_EVENT 	cb_event;     ///< callback event
} HD_BSMUX_CBINFO;

/**
    callback funcion prototype.

	@param CHAR *              p_name
    @param HD_BSMUX_CBINFO *   cbinfo
    @param UINT32 *            param
*/
typedef INT32 (*HD_BSMUX_CALLBACK)(CHAR *p_name, HD_BSMUX_CBINFO *cbinfo, UINT32 *param);

/**
    bsmux callback info.
    @note internal use
    @SET: HD_BSMUX_PARAM_REG_CALLBACK
*/
typedef struct _HD_BSMUX_REG_CALLBACK {
	HD_BSMUX_CALLBACK callbackfunc;
	UINT32 version;
} HD_BSMUX_REG_CALLBACK;

/**
    @name video codec type.
    @note modified from uitron (should be the same as MEDIAVIDENC_XXX)
*/
typedef enum _HD_BSMUX_VIDCODEC {
	HD_BSMUX_VIDCODEC_MJPG              = 0x00000001,    ///< motion jpeg
	HD_BSMUX_VIDCODEC_H264              = 0x00000002,    ///< h.264
	HD_BSMUX_VIDCODEC_H265              = 0x00000003,    ///< h.265
	ENUM_DUMMY4WORD(HD_BSMUX_VIDCODEC)
} HD_BSMUX_VIDCODEC;

/**
    video info (project setting).
    @SET/GET: HD_BSMUX_PARAM_VIDEOINFO
*/
typedef struct _HD_BSMUX_VIDEOINFO {
	UINT32 vidcodec;                  ///< vidoe codec type (using HD_BSMUX_VIDCODEC)
	UINT32 vfr;                       ///< vidoe frame rate (round)(playback)
	UINT32 width;                     ///< video width
	UINT32 height;                    ///< video height
	UINT32 descAddr;                  ///< <obsolete>
	UINT32 descSize;                  ///< <obsolete>
	UINT32 tbr;                       ///< target bitrate Bytes/sec
	UINT32 DAR;                       ///< MP_VDOENC_DAR_DEFAULT or others
	UINT32 gop;                       ///< gop
} HD_BSMUX_VIDEOINFO;

/**
    @name audio codec type.
    @note modified from uitron (should be the same as MOVAUDENC_XXX)
*/
typedef enum _HD_BSMUX_AUDCODEC {
	HD_BSMUX_AUDCODEC_PCM               = 0x00000001,    ///< PCM
	HD_BSMUX_AUDCODEC_AAC               = 0x00000002,    ///< AAC
	HD_BSMUX_AUDCODEC_PPCM              = 0x00000003,    ///< Packed PCM
	HD_BSMUX_AUDCODEC_ULAW              = 0x00000004,    ///< PCM uLaw
	HD_BSMUX_AUDCODEC_ALAW              = 0x00000005,    ///< PCM aLaw
	ENUM_DUMMY4WORD(HD_BSMUX_AUDCODEC)
} HD_BSMUX_AUDCODEC;

/**
    audio info (project setting).
    @SET/GET: HD_BSMUX_PARAM_AUDIOINFO
*/
typedef struct _HD_BSMUX_AUDIOINFO {
	UINT32 codectype;                 ///< audio codec type (using HD_BSMUX_AUDCODEC)
	UINT32 chs;                       ///< channels
	UINT32 asr;                       ///< audio samplerate
	UINT32 aud_en;                    ///< audio enable
	UINT32 adts_bytes;                ///< audio adts bytes
} HD_BSMUX_AUDIOINFO;

/**
    @name recoding type.
    @note modified from uitron (should be the same as MEDIAREC_XXX)
*/
typedef enum _HD_BSMUX_RECFORMAT {
	HD_BSMUX_RECFORMAT_VID_ONLY         = 0x00000001,    ///< video only, normal speed
	HD_BSMUX_RECFORMAT_AUD_VID_BOTH     = 0x00000002,    ///< video/audio sync, normal speed
	HD_BSMUX_RECFORMAT_GOLFSHOT         = 0x00000004,    ///< golfshot mode(rec n fps, play m fps, n > m, n % m = 0), no audio
	HD_BSMUX_RECFORMAT_TIMELAPSE        = 0x00000005,    ///< timelapse mode(rec n fps, play m fps, n < m, m % n = 0), no audio
	ENUM_DUMMY4WORD(HD_BSMUX_RECFORMAT)
} HD_BSMUX_RECFORMAT;

/**
    file info (project setting).
    @SET/GET: HD_BSMUX_PARAM_FILEINFO
*/
typedef struct _HD_BSMUX_FILEINFO {
	UINT32 emron;                     ///< emr on/off
	UINT32 emrloop;                   ///< emrloop on/off
	UINT32 strgid;                    ///< <obsolete>
	UINT32 ddr_id;                    ///< DDR ID
	UINT32 seamlessSec;               ///< seamless sec
	UINT32 rollbacksec;               ///< rollback sec
	UINT32 keepsec;                   ///< keep sec of emerengy recording
	UINT32 endtype;                   ///< <obsolete>
	UINT32 filetype;                  ///< file type (using HD_BSMUX_FTYPE)
	UINT32 recformat;                 ///< recoding type (using HD_BSMUX_RECFORMAT)
	UINT32 playvfr;                   ///< playback config for timelapse/golfshot mode
	UINT32 revsec;                    ///< buffer reserved sec
	UINT32 overlop_on;                ///< overlap on/off
	UINT32 seamlessSec_ms;            ///< seamless sec (ms)
	UINT32 rollbacksec_ms;            ///< rollback sec (ms)
	UINT32 keepsec_ms;                ///< keep sec of emerengy recording (ms)
	UINT32 revsec_ms;                 ///< buffer reserved sec (ms)
} HD_BSMUX_FILEINFO;

/**
    buf info (project setting).
    @note for bs data of videnc/audenc
    @SET: HD_BSMUX_PARAM_BUFINFO
*/
typedef struct _HD_BSMUX_BUFINFO {
	HD_BUFINFO videnc;
	HD_BUFINFO audenc;
} HD_BSMUX_BUFINFO;

/**
    wrinfo (project setting).
    @SET/GET: HD_BSMUX_PARAM_WRINFO
*/
typedef struct _HD_BSMUX_WRINFO {
	UINT32 flush_freq;                ///< flush frequency
	UINT32 wrblk_size;                ///< write block size
} HD_BSMUX_WRINFO;

/**
    extinfo (project setting).
    @SET/GET: HD_BSMUX_PARAM_EXTINFO
*/
typedef struct _HD_BSMUX_EXTINFO {
	UINT32 unit;                       ///< extend unit sec
	UINT32 max_num;                    ///< extend max num
	UINT32 enable;                     ///< extend on/off
} HD_BSMUX_EXTINFO;

/**
    gps data.
    @SET: HD_BSMUX_PARAM_GPS_DATA
*/
typedef struct _HD_BSMUX_GPS_DATA {
	UINT32 gpson;
	UINT32 gpsdataadr;
	UINT32 gpsdatasize;
} HD_BSMUX_GPS_DATA;

/**
    user data.
    @SET: HD_BSMUX_PARAM_USER_DATA
*/
typedef struct _HD_BSMUX_USER_DATA {
	UINT32 useron;
	UINT32 userdataadr;
	UINT32 userdatasize;
} HD_BSMUX_USER_DATA;

/**
    customized data.
    @SET: HD_BSMUX_PARAM_CUST_DATA
*/
typedef struct _HD_BSMUX_CUST_DATA {
	UINT32 custon;
	UINT32 custtag;
	UINT32 custaddr;
	UINT32 custsize;
} HD_BSMUX_CUST_DATA;

/**
    @name put data type.
*/
typedef enum _HD_BSMUX_PUT_DATA_TYPE {
	HD_BSMUX_PUT_DATA_TYPE_MIN,
	HD_BSMUX_PUT_DATA_TYPE_GPS          = 0x00000001,   ///< put gps data
	HD_BSMUX_PUT_DATA_TYPE_USER         = 0x00000002,   ///< put user data (moov only)
	HD_BSMUX_PUT_DATA_TYPE_CUST         = 0x00000004,   ///< put customized data (with specific tag)
	HD_BSMUX_PUT_DATA_TYPE_THUMB        = 0x00000010,   ///< put thumbnail data
	HD_BSMUX_PUT_DATA_TYPE_MAX,
	ENUM_DUMMY4WORD(HD_BSMUX_PUT_DATA_TYPE)
} HD_BSMUX_PUT_DATA_TYPE;

/**
    put data info (event).
    @SET: HD_BSMUX_PARAM_PUT_DATA
*/
typedef struct _HD_BSMUX_PUT_DATA {
	HD_BSMUX_PUT_DATA_TYPE type;        ///< put data type
	UINT32 phy_addr;                    ///< physical address of data buffer
	UINT32 vir_addr;                    ///< virtual address of data buffer
	UINT32 size;                        ///< size of data buffer
	UINT32 resv;                        ///< reserved for specific info
} HD_BSMUX_PUT_DATA;

/**
    trig emr (event).
    @SET: HD_BSMUX_PARAM_TRIG_EMR
*/
typedef struct _HD_BSMUX_TRIG_EMR {
	UINT32 emr_on;                     ///< emergency on/off
	UINT32 pause_on;                   ///< pause on/off
	UINT32 pause_id;                   ///< pause id
} HD_BSMUX_TRIG_EMR;

/**
    @name trigger event type.
*/
typedef enum _HD_BSMUX_TRIG_EVENT_TYPE {
	HD_BSMUX_TRIG_EVENT_MIN,
	HD_BSMUX_TRIG_EVENT_CUTNOW          = 0x00000001,    ///< trigger cut now event of output path
	HD_BSMUX_TRIG_EVENT_EXTEND          = 0x00000002,    ///< trigger extend event of output path
	HD_BSMUX_TRIG_EVENT_PAUSE           = 0x00000004,    ///< trigger pause event of output path
	HD_BSMUX_TRIG_EVENT_RESUME          = 0x00000008,    ///< trigger resume event of output path
	HD_BSMUX_TRIG_EVENT_EMERGENCY       = 0x00000010,    ///< trigger emergency of output path
	HD_BSMUX_TRIG_EVENT_MAX,
	ENUM_DUMMY4WORD(HD_BSMUX_TRIG_EVENT_TYPE)
} HD_BSMUX_TRIG_EVENT_TYPE;

/**
    trigger event info (event).
    @SET: HD_BSMUX_PARAM_TRIG_EVENT
*/
typedef struct _HD_BSMUX_TRIG_EVENT {
	HD_BSMUX_TRIG_EVENT_TYPE type;      ///< trigger event type
	UINT32 ret_val;                     ///< return value
} HD_BSMUX_TRIG_EVENT;

/**
    @name uitlity type.
*/
typedef enum _HD_BSMUX_EN_UTIL_TYPE {
	HD_BSMUX_EN_UTIL_MIN,
	HD_BSMUX_EN_UTIL_SW_METHOD          = 0x00000001,    ///< buffer copy method of mov container of output path. ts is unsupported.
	HD_BSMUX_EN_UTIL_PTS_RESET          = 0x00000002,    ///< reset pts each file for ts flow. mov/mp4 is unsupported.
	HD_BSMUX_EN_UTIL_FRONTMOOV          = 0x00000010,    ///< front 'moov' box of mov container of output path.
	HD_BSMUX_EN_UTIL_STRG_BUF           = 0x00000020,    ///< storage buffer usgae of mov format of output path.
	HD_BSMUX_EN_UTIL_RECOVERY           = 0x00000040,    ///< file recovery mechanism for mov container of output path.
	HD_BSMUX_EN_UTIL_EMERGENCY          = 0x00000080,    ///< emergency once information of output path.
	HD_BSMUX_EN_UTIL_FREA_BOX           = 0x00000100,    ///< 'frea' box on mov container of output path. if true 'skip' box will be replaced.
	HD_BSMUX_EN_UTIL_DUR_LIMIT          = 0x00000200,    ///< timestamp/duration limit of input bitstream of output path.
	HD_BSMUX_EN_UTIL_MUXALIGN           = 0x00000400,    ///< buffer copy alignment of mov container of output path.
	HD_BSMUX_EN_UTIL_BUFLOCK            = 0x00000800,    ///< buffer lock (ms) if output resource is blocked of output path.
	HD_BSMUX_EN_UTIL_FAST_PUT           = 0x00001000,    ///< fast put first writeblock for mov container of output path.
	HD_BSMUX_EN_UTIL_EN_DROP            = 0x00002000,    ///< drop invalid bitstream for mov container of output path.
	HD_BSMUX_EN_UTIL_GPS_DATA           = 0x00004000,    ///< gps input data configuration of output path.
	HD_BSMUX_EN_UTIL_UTC_AUTO           = 0x00008000,    ///< utc time zone mode for mov container of output path.
	HD_BSMUX_EN_UTIL_BUF_USAGE          = 0x00010000,    ///< buffer usage of bitstream copy of output path.
	HD_BSMUX_EN_UTIL_UTC_TIME           = 0x00020000,    ///< utc time configuration for mov container of output path.
	HD_BSMUX_EN_UTIL_VAR_RATE           = 0x00040000,    ///< variable rate-control configuration for mov container of output path.
	HD_BSMUX_EN_UTIL_BTAG_SIZE          = 0x00080000,    ///< 'mdat'/'stco' box tag size on mov container of output path. valid 32(bit) or 64(bit).
	HD_BSMUX_EN_UTIL_MAX,
	ENUM_DUMMY4WORD(HD_BSMUX_EN_UTIL_TYPE)
} HD_BSMUX_EN_UTIL_TYPE;

/**
    enable uitlity info (util).
    @SET: HD_BSMUX_PARAM_TRIG_EVENT
*/
typedef struct _HD_BSMUX_EN_UTIL {
	HD_BSMUX_EN_UTIL_TYPE type;       ///< uitlity type
	UINT32 enable;                    ///< on/off
	UINT32 resv[2];                   ///< reserved
} HD_BSMUX_EN_UTIL;

/**
    util calc sec info.
    @GET: HD_BSMUX_PARAM_CALC_SEC
*/
typedef struct _HD_BSMUX_CALC_SEC_SETTING {
	UINT64 nowFSFreeSpace;             ///< in: free space
	UINT32 vidfps;                     ///< in: video frame per second
	UINT32 vidTBR;                     ///< in: now video target bit rate
	UINT32 audSampleRate;              ///< in: audio sample rate
	UINT32 audChs;                     ///< in: ahdio channels
	UINT32 gpson;                      ///< in: if gps on
	UINT32 nidxon;                     ///< in: if nidx on
} HD_BSMUX_CALC_SEC_SETTING;

typedef struct _HD_BSMUX_CALC_SEC {
	UINT64 givenSpace;                 ///< in: free space
	HD_BSMUX_CALC_SEC_SETTING info[HD_BSMUX_MAX_CTRL];
	UINT32 calc_sec;                   ///< out: free sec
} HD_BSMUX_CALC_SEC;

/**
    @name data type.
*/
typedef enum _HD_BSMUX_TYPE {
	HD_BSMUX_TYPE_VIDEO                 = 0x00000001,   ///< video bitstream data (main)
	HD_BSMUX_TYPE_AUDIO                 = 0x00000002,   ///< audio bitstream data
	HD_BSMUX_TYPE_USRCB                 = 0x00000040,   ///< user callback data
	HD_BSMUX_TYPE_SUBVD                 = 0x00000200,   ///< video bitstream data (sub)
	HD_BSMUX_TYPE_META                  = 0x00001000,   ///< meta data
	ENUM_DUMMY4WORD(HD_BSMUX_TYPE)
} HD_BSMUX_TYPE;

/**
	bitstream muxer data descriptor.
*/
#define BSMUX_BS_PACK_MAX                4

typedef struct _HD_BSMUX_BS_DATA {
	UINT32 phy_addr;                    ///< physical address of encoded data
	UINT32 vrt_addr;                    ///< virtual address of encoded data
	UINT32 size;                        ///< size of encoded data
} HD_BSMUX_BS_DATA;

typedef struct _HD_BSMUX_BS {
	UINT32 sign;                        ///< signature = MAKEFOURCC('B','S','M','X')
	HD_METADATA* p_next;                ///< pointer to next meta
	HD_BSMUX_TYPE data_type;            ///< data type
	UINT64 timestamp;                   ///< timestamp (unit: microsecond)
	HD_BSMUX_BS_DATA data_pack[BSMUX_BS_PACK_MAX];      ///< Pack array of encoded data
	VOID * p_user_bs;                   ///< for bitstream data descriptor
	VOID * p_user_data;                 ///< for user callback data
} HD_BSMUX_BS;

/**
	meta muxer data descriptor.
*/
#define HD_BSMUX_META_HEADER_SIZE  64  ///< size of signature header in BYTES.

typedef struct _HD_BSMUX_COMM_META {
	UINT32 sign;                        ///< signature = MAKEFOURCC('D','U','M','Y')
	HD_METADATA* p_next;                ///< pointer to next meta
	UINT32 header_size;                 ///< signature header size of meta data
	UINT32 buffer_size;                 ///< buffer size of meta data
	UINT32 reserved[12];                ///< reserved for 64-align
} HD_BSMUX_COMM_META;
STATIC_ASSERT(sizeof(HD_BSMUX_COMM_META) == HD_BSMUX_META_HEADER_SIZE);

typedef struct _HD_BSMUX_META_ALLOC {
	UINT32              sign;             ///< signature for meta alloc
	struct _HD_BSMUX_META_ALLOC *p_next;  ///< pointer to next meta alloc
	UINT32              data_size;        ///< data size for meta alloc
	UINT32              data_rate;        ///< data rate for meta alloc
} HD_BSMUX_META_ALLOC;

/**
    @name bsmux param id.
*/
typedef enum _HD_BSMUX_PARAM_ID {
	HD_BSMUX_PARAM_MIN      = 0,
	HD_BSMUX_PARAM_REG_CALLBACK,        ///< support set with ctrl path, using HD_BSMUX_CALLBACK
	HD_BSMUX_PARAM_VIDEOINFO,           ///< support get/set with i/o path, using HD_BSMUX_VIDEOINFO struct
	HD_BSMUX_PARAM_AUDIOINFO,           ///< support get/set with i/o path, using HD_BSMUX_AUDIOINFO struct
	HD_BSMUX_PARAM_FILEINFO,            ///< support get/set with i/o path, using HD_BSMUX_FILEINFO struct
	HD_BSMUX_PARAM_BUFINFO,             ///< support get/set with i/o path, using HD_BSMUX_BUFINFO struct
	HD_BSMUX_PARAM_WRINFO,              ///< support get/set with i/o path, using HD_BSMUX_WRINFO struct
	HD_BSMUX_PARAM_EXTINFO,             ///< support get/set with i/o path, using HD_BSMUX_EXTINFO struct
	HD_BSMUX_PARAM_GPS_DATA,            ///< <obsolete> use HD_BSMUX_PARAM_PUT_DATA with HD_BSMUX_PUT_DATA_TYPE_GPS
	HD_BSMUX_PARAM_USER_DATA,           ///< <obsolete> use HD_BSMUX_PARAM_PUT_DATA with HD_BSMUX_PUT_DATA_TYPE_USER
	HD_BSMUX_PARAM_CUST_DATA,           ///< <obsolete> use HD_BSMUX_PARAM_PUT_DATA with HD_BSMUX_PUT_DATA_TYPE_CUST
	HD_BSMUX_PARAM_PUT_DATA,            ///< support set with i/o path, using HD_BSMUX_PUT_DATA struct
	HD_BSMUX_PARAM_TRIG_EMR,            ///< support set with i/o path, using HD_BSMUX_TRIG_EMR struct
	HD_BSMUX_PARAM_CUT_NOW,             ///< <obsolete> use HD_BSMUX_PARAM_TRIG_EVENT with HD_BSMUX_TRIG_EVENT_CUTNOW
	HD_BSMUX_PARAM_EXTEND,              ///< <obsolete> use HD_BSMUX_PARAM_TRIG_EVENT with HD_BSMUX_TRIG_EVENT_EXTEND
	HD_BSMUX_PARAM_PAUSE,               ///< <obsolete> use HD_BSMUX_PARAM_TRIG_EVENT with HD_BSMUX_TRIG_EVENT_PAUSE
	HD_BSMUX_PARAM_RESUME,              ///< <obsolete> use HD_BSMUX_PARAM_TRIG_EVENT [func: trig emr/emrloop/emroncard]
	HD_BSMUX_PARAM_TRIG_EVENT,          ///< support set with i/o path, using HD_BSMUX_TRIG_EVENT struct
	HD_BSMUX_PARAM_EN_UTIL,             ///< support set with i/o path, using HD_BSMUX_EN_UTIL struct
	HD_BSMUX_PARAM_META_ALLOC,          ///< support set with i/o path, using HD_BSMUX_META_ALLOC struct
	HD_BSMUX_PARAM_CALC_SEC,            ///< support get with ctrl path, using HD_BSMUX_CALC_SEC struct
	HD_BSMUX_PARAM_MAX,
	ENUM_DUMMY4WORD(HD_BSMUX_PARAM_ID)
} HD_BSMUX_PARAM_ID;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT hd_bsmux_init(VOID);
HD_RESULT hd_bsmux_open(HD_IN_ID in_id, HD_OUT_ID out_id, HD_PATH_ID* p_path_id);
HD_RESULT hd_bsmux_start(HD_PATH_ID path_id);
HD_RESULT hd_bsmux_stop(HD_PATH_ID path_id);
HD_RESULT hd_bsmux_close(HD_PATH_ID path_id);
HD_RESULT hd_bsmux_get(HD_PATH_ID path_id, HD_BSMUX_PARAM_ID id, VOID *p_param);
HD_RESULT hd_bsmux_set(HD_PATH_ID path_id, HD_BSMUX_PARAM_ID id, VOID *p_param);
HD_RESULT hd_bsmux_push_in_buf_video(HD_PATH_ID path_id, HD_VIDEOENC_BS* p_user_in_video_bs, INT32 wait_ms);
HD_RESULT hd_bsmux_push_in_buf_audio(HD_PATH_ID path_id, HD_AUDIO_BS* p_user_in_audio_bs, INT32 wait_ms);
HD_RESULT hd_bsmux_push_in_buf_struct(HD_PATH_ID path_id, HD_BSMUX_BS* p_user_in_bsmux_bs, INT32 wait_ms);
HD_RESULT hd_bsmux_uninit(VOID);

#endif //_HD_BSMUX_LIB_H_
