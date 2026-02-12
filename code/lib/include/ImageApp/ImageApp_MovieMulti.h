#ifndef IMAGEAPP_MOVIEMULTI_H
#define IMAGEAPP_MOVIEMULTI_H

#include "hdal.h"
#include "kflow_videoenc/media_def.h"
#include <kwrap/type.h>
#include "ImageApp/ImageApp_Common.h"
#include "hd_bsmux_lib.h"
#include "hd_fileout_lib.h"
#include "vendor_videoenc.h"

/// ========== Callback function area ==========
typedef void (*MOVIE_RAWPROC_CB)(void);

/// ========== Enum area ==========
typedef enum {
	_CFG_REC_ID_1 = 0,
	_CFG_REC_ID_2,
	_CFG_REC_ID_3,
	_CFG_REC_ID_4,
	_CFG_REC_ID_MAX,
	_CFG_STRM_ID_1,
	_CFG_STRM_ID_2,
	_CFG_STRM_ID_3,
	_CFG_STRM_ID_4,
	_CFG_STRM_ID_MAX,
	_CFG_UVAC_ID_1,
	_CFG_UVAC_ID_2,
	_CFG_UVAC_ID_MAX,
	_CFG_CLONE_ID_1,
	_CFG_CLONE_ID_2,
	_CFG_CLONE_ID_3,
	_CFG_CLONE_ID_4,
	_CFG_CLONE_ID_MAX,
	_CFG_DISP_ID_1,
	_CFG_DISP_ID_2,
	_CFG_DISP_ID_MAX,
	_CFG_ETHCAM_ID_1,
	_CFG_ETHCAM_ID_2,
	_CFG_ETHCAM_ID_MAX,
	_CFG_AUDIO_ID_1,
	_CFG_AUDIO_ID_MAX,
	_CFG_CTRL_ID = 0x0000FF00,
	ENUM_DUMMY4WORD(MOVIE_CFG_REC_ID)
} MOVIE_CFG_REC_ID;
                                                                                    /// Notice:
enum {                                                                              /// <xxx> means argument value uses xxx type/structure pointer to pass information
	MOVIE_CONFIG_RECORD_INFO  = 0xF000F000,                                         /// <MOVIE_RECODE_INFO *> structure pointer (Set only)
	MOVIE_CONFIG_STREAM_INFO,                                                       /// <MOVIE_STRM_INFO *> structure pointer (Set only)
	MOVIE_CONFIG_AUDIO_INFO,                                                        /// <MOVIEMULTI_AUDIO_INFO *> structure pointer (Set only)
	MOVIE_CONFIG_DISP_INFO,                                                         /// <MOVIEMULTI_CFG_DISP_INFO *> structure pointer (Set only)
	MOVIE_CONFIG_UVAC_INFO,                                                         /// N/A
	MOVIE_CONFIG_ALG_INFO,                                                          /// <MOVIE_ALG_INFO *> structure pointer (Set only)
	MOVIE_CONFIG_ETHCAM_INFO,                                                       /// N/A
	MOVIE_CONFIG_MAX_LINK_INFO,                                                     /// <MOVIEMULTI_MAX_LINK_INFO *> structure pointer (Set only)
	MOVIE_CONFIG_MEM_POOL_INFO,                                                     /// <HD_COMMON_MEM_INIT_CONFIG *> structure pointer (Set only)
	MOVIE_CONFIG_SENSOR_INFO,                                                       /// <MOVIE_SENSOR_INFO *> structure pointer (Set only)
	MOVIE_CONFIG_SENSOR_MAPPING,                                                    /// <UINT32 *> structure pointer, point to an UINT32 array with n components (n = sensor count) (Set only)
	MOVIE_CONFIG_AD_MAP,                                                            /// <MOVIE_AD_MAP *> structure pointer (Set only)
	MOVIE_CONFIG_DRAM,                                                              /// <MOVIEMULTI_DRAM_CFG *> structure pointer (Set only)
};

// emum for MOVIEMULTI_PARAM_IPL_FORCED_IMG_SIZE
typedef enum {
	MOVIE_IPL_SIZE_AUTO = 0,
	MOVIE_IPL_SIZE_MAIN,
	MOVIE_IPL_SIZE_CLONE,
	MOVIE_IPL_SIZE_WIFI,
	MOVIE_IPL_SIZE_ALG,
	MOVIE_IPL_SIZE_DISP,
	MOVIE_IPL_SIZE_USER,
	ENUM_DUMMY4WORD(MOVIE_IPL_SIZE)
} MOVIE_IPL_SIZE;

// structure for MOVIEMULTI_PARAM_IPL_USER_IMG_SIZE
typedef struct {
	USIZE size;
	UINT32 fps;
} MOVIEMULTI_IPL_SIZE_INFO;

typedef enum {
	_CFG_VID_IN_1 = 0,
	_CFG_VID_IN_2,
	_CFG_VID_IN_3,
	_CFG_VID_IN_4,
	ENUM_DUMMY4WORD(MOVIE_CFG_VID_IN)
} MOVIE_CFG_VID_IN;

typedef enum {
	_CFG_ALG_PATH3 = 0,
	_CFG_ALG_PATH4,
	_CFG_ALG_PATH_MAX,
	ENUM_DUMMY4WORD(MOVIE_CFG_ALG_PATH)
} MOVIE_CFG_ALG_PATH;

typedef enum {
	IAMOVIE_VPRC_EX_MAIN = 0,
	IAMOVIE_VPRC_EX_CLONE,
	IAMOVIE_VPRC_EX_DISP,
	IAMOVIE_VPRC_EX_WIFI,
	IAMOVIE_VPRC_EX_ALG,
	IAMOVIE_VPRC_EX_DISP2,
	IAMOVIE_VPRC_EX_ALG2,
	IAMOVIE_VPRC_EX_MAX,
	ENUM_DUMMY4WORD(IAMOVIE_VPRC_EX_PATH)
} IAMOVIE_VPRC_EX_PATH;

typedef ISIZE        MOVIE_CFG_IMG_SIZE;

typedef UINT32       MOVIE_CFG_FRAME_RATE;

typedef UINT32       MOVIE_CFG_TARGET_RATE;

typedef ISIZE        MOVIE_CFG_IMG_RATIO;

typedef ISIZE        MOVIE_CFG_RAWENC_SIZE;

typedef UINT32       MOVIE_CFG_VCAP_FUNC;

typedef UINT32       MOVIE_CFG_VPROC_FUNC;

typedef BOOL         MOVIE_CFG_DISP_ENABLE;

typedef UINT32       MOVIE_CFG_GOP_NUM;

typedef BOOL         MOVIE_CFG_RTSP_STRM_OUT;

// emum for MOVIEMULTI_PARAM_CODEC
typedef enum {
	_CFG_CODEC_MJPG = MEDIAVIDENC_MJPG,
	_CFG_CODEC_H264 = MEDIAVIDENC_H264,
	_CFG_CODEC_H265 = MEDIAVIDENC_H265,
	ENUM_DUMMY4WORD(MOVIE_CFG_CODEC)
} MOVIE_CFG_CODEC;

typedef enum {
	_CFG_AUD_CODEC_NONE = 0,
	_CFG_AUD_CODEC_PCM,
	_CFG_AUD_CODEC_AAC,
	_CFG_AUD_CODEC_ADPCM,
	_CFG_AUD_CODEC_ULAW,
	_CFG_AUD_CODEC_ALAW,
	_CFG_AUD_CODEC_MAX,
	ENUM_DUMMY4WORD(MOVIE_CFG_AUD_CODEC)
} MOVIE_CFG_AUD_CODEC;

// emum for MOVIEMULTI_PARAM_REC_FORMAT
typedef enum {
	_CFG_REC_TYPE_AV        = MEDIAREC_AUD_VID_BOTH,
	_CFG_REC_TYPE_VID       = MEDIAREC_VID_ONLY,
	_CFG_REC_TYPE_TIMELAPSE = MEDIAREC_TIMELAPSE,
	_CFG_REC_TYPE_GOLFSHOT  = MEDIAREC_GOLFSHOT,
	ENUM_DUMMY4WORD(MOVIE_CFG_REC_MODE)
} MOVIE_CFG_REC_MODE;

// emum for MOVIEMULTI_PARAM_FILE_FORMAT
typedef enum {
	_CFG_FILE_FORMAT_MP4     = MEDIA_FILEFORMAT_MP4,
	_CFG_FILE_FORMAT_TS      = MEDIA_FILEFORMAT_TS,
	_CFG_FILE_FORMAT_MOV     = MEDIA_FILEFORMAT_MOV,
	_CFG_FILE_FORMAT_JPG     = MEDIA_FILEFORMAT_JPG,
	_CFG_FILE_FORMAT_UNKNOWN = 0,
	ENUM_DUMMY4WORD(MOVIE_CFG_FILE_FORMAT)
} MOVIE_CFG_FILE_FORMAT;

typedef enum {
	_CFG_EMR_OFF         = 0x00000000,    /// EMR off
	_CFG_MAIN_EMR_BOTH   = 0x00000001,    /// Main remains + EMR
	_CFG_MAIN_EMR_PAUSE  = 0x00000002,    /// Main paused + EMR
	_CFG_MAIN_EMR_ONLY   = 0x00000004,    /// Main EMR only
	_CFG_MAIN_EMR_LOOP   = 0x00000008,    /// Main EMR loop
	_CFG_CLONE_EMR_BOTH  = 0x00000010,    /// Clone remains + EMR
	_CFG_CLONE_EMR_PAUSE = 0x00000020,    /// Clone paused + EMR
	_CFG_CLONE_EMR_ONLY  = 0x00000040,    /// Clone EMR only
	_CFG_CLONE_EMR_LOOP  = 0x00000080,    /// Clone EMR loop
	_CFG_EMR_SET_CRASH   = 0x00000100,    /// set crash
	ENUM_DUMMY4WORD(MOVIE_CFG_FILE_OPTION)
} MOVIE_CFG_FILE_OPTION;

typedef UINT32       MOVIE_CFG_FILE_ENDTYPE;

typedef enum {
	//H264 profile
	_CFG_H264_PROFILE_BASELINE = HD_H264E_BASELINE_PROFILE,
	_CFG_H264_PROFILE_MAIN     = HD_H264E_MAIN_PROFILE,
	_CFG_H264_PROFILE_HIGH     = HD_H264E_HIGH_PROFILE,
	//H265 profile
	_CFG_H265_PROFILE_MAIN     = HD_H265E_MAIN_PROFILE,
	ENUM_DUMMY4WORD(MOVIE_CFG_PROFILE)
} MOVIE_CFG_PROFILE;

typedef enum _MOVIE_CFG_LEVEL {
	//H264 level
	_CFG_H264_LEVEL_1          = HD_H264E_LEVEL_1,
	_CFG_H264_LEVEL_1_1        = HD_H264E_LEVEL_1_1,
	_CFG_H264_LEVEL_1_2        = HD_H264E_LEVEL_1_2,
	_CFG_H264_LEVEL_1_3        = HD_H264E_LEVEL_1_3,
	_CFG_H264_LEVEL_2          = HD_H264E_LEVEL_2,
	_CFG_H264_LEVEL_2_1        = HD_H264E_LEVEL_2_1,
	_CFG_H264_LEVEL_2_2        = HD_H264E_LEVEL_2_2,
	_CFG_H264_LEVEL_3          = HD_H264E_LEVEL_3,
	_CFG_H264_LEVEL_3_1        = HD_H264E_LEVEL_3_1,
	_CFG_H264_LEVEL_3_2        = HD_H264E_LEVEL_3_2,
	_CFG_H264_LEVEL_4          = HD_H264E_LEVEL_4,
	_CFG_H264_LEVEL_4_1        = HD_H264E_LEVEL_4_1,
	_CFG_H264_LEVEL_4_2        = HD_H264E_LEVEL_4_2,
	_CFG_H264_LEVEL_5          = HD_H264E_LEVEL_5,
	_CFG_H264_LEVEL_5_1        = HD_H264E_LEVEL_5_1,
	//H265 level
	_CFG_H265_LEVEL_1          = HD_H265E_LEVEL_1,
	_CFG_H265_LEVEL_2          = HD_H265E_LEVEL_2,
	_CFG_H265_LEVEL_2_1        = HD_H265E_LEVEL_2_1,
	_CFG_H265_LEVEL_3          = HD_H265E_LEVEL_3,
	_CFG_H265_LEVEL_3_1        = HD_H265E_LEVEL_3_1,
	_CFG_H265_LEVEL_4          = HD_H265E_LEVEL_4,
	_CFG_H265_LEVEL_4_1        = HD_H265E_LEVEL_4_1,
	_CFG_H265_LEVEL_5          = HD_H265E_LEVEL_5,
	_CFG_H265_LEVEL_5_1        = HD_H265E_LEVEL_5_1,
	_CFG_H265_LEVEL_5_2        = HD_H265E_LEVEL_5_2,
	_CFG_H265_LEVEL_6          = HD_H265E_LEVEL_6,
	_CFG_H265_LEVEL_6_1        = HD_H265E_LEVEL_6_1,
	_CFG_H265_LEVEL_6_2        = HD_H265E_LEVEL_6_2,
	ENUM_DUMMY4WORD(MOVIE_CFG_LEVEL)
} MOVIE_CFG_LEVEL;

typedef enum {
	_CFG_DAR_DEFAULT,                      // default video display aspect ratio (the same as encoded image)
	_CFG_DAR_16_9,                         // 16:9 video display aspect ratio
	ENUM_DUMMY4WORD(MOVIE_CFG_DAR)
} MOVIE_CFG_DAR;

typedef struct {
	INT32  aq_enable;
	UINT32 aq_str;
	INT32  sraq_init_aslog2;
	INT32  max_sraq;
	INT32  min_sraq;
	INT32  sraq_const_aslog2;
} MOVIE_CFG_AQ_INFO;

typedef struct {
	UINT32 uiEnable;
	UINT32 uiStaticTime;
	UINT32 uiFrameRate;
	UINT32 uiByteRate;
	UINT32 uiGOP;
	UINT32 uiInitIQp;
	UINT32 uiMinIQp;
	UINT32 uiMaxIQp;
	UINT32 uiInitPQp;
	UINT32 uiMinPQp;
	UINT32 uiMaxPQp;
	INT32  iIPWeight;
	UINT32 uiRowRcEnalbe;
	UINT32 uiRowRcQpRange;
	UINT32 uiRowRcQpStep;
} MOVIE_CFG_CBR_INFO;

typedef enum {
	_CFG_AUDIO_CH_LEFT,              ///< Left
	_CFG_AUDIO_CH_RIGHT,             ///< Right
	_CFG_AUDIO_CH_STEREO,            ///< Stereo
	ENUM_DUMMY4WORD(MOVIE_CFG_AUDIO_CH)
} MOVIE_CFG_AUDIO_CH;

typedef struct {
	HD_PATH_ID      vout_ctrl;
	HD_PATH_ID      vout_path;
	USIZE           ratio;
	HD_VIDEO_DIR    dir;
} MOVIEMULTI_CFG_DISP_INFO;

typedef UINT32       MOVIE_CFG_FILE_SECOND;

typedef struct {
	MOVIE_CFG_REC_ID        rec_id;
	MOVIE_CFG_FILE_SECOND   seamless_sec;  // Seamless second
	MOVIE_CFG_FILE_OPTION   emr_on;        // pre-record emergency on
	MOVIE_CFG_FILE_SECOND   emr_sec;       // pre-record second for emergency (main_path)
	MOVIE_CFG_FILE_SECOND   keep_sec;      // post-record second for emergency (main path)
	MOVIE_CFG_FILE_SECOND   overlap_sec;   // overlap second
	MOVIE_CFG_FILE_ENDTYPE  end_type;      // such as MOVREC_ENDTYPE_CUTOVERLAP
	MOVIE_CFG_FILE_SECOND   rollback_sec;  // pre-record second for crash
	MOVIE_CFG_FILE_SECOND   forward_sec;   // post-record second for crash
	MOVIE_CFG_FILE_SECOND   clone_emr_sec; // pre-record second for emergency (clone path)
	MOVIE_CFG_FILE_SECOND   clone_keep_sec;// post-record second for emergency (clone path)
} MOVIE_RECODE_FILE_OPTION;

/// ========== Data structure area ==========
typedef struct {
	MOVIE_CFG_REC_ID             rec_id;
	MOVIE_CFG_VID_IN             vid_in;
	MOVIE_CFG_IMG_SIZE           size;
	MOVIE_CFG_FRAME_RATE         frame_rate;
	MOVIE_CFG_TARGET_RATE        target_bitrate;
	MOVIE_CFG_CODEC              codec;
	MOVIE_CFG_AUD_CODEC          aud_codec;
	MOVIE_CFG_REC_MODE           rec_mode;
	MOVIE_CFG_FILE_FORMAT        file_format;
	MOVIE_CFG_IMG_RATIO          ratio;
	MOVIE_CFG_RAWENC_SIZE        raw_enc_size;
	MOVIE_CFG_VCAP_FUNC          vcap_func;
	MOVIE_CFG_DISP_ENABLE        disp_enable;
	BOOL                         ipl_set_enable;
	MOVIE_CFG_DAR                dar;
	MOVIE_CFG_AQ_INFO            aq_info;
	MOVIE_CFG_CBR_INFO           cbr_info;
	BOOL                         sensor_rotate;
	MOVIE_CFG_VPROC_FUNC         vproc_func;
	MOVIE_CFG_IMG_RATIO			 disp_ratio;		// video display aspect ratio
} MOVIE_RECODE_INFO;

typedef struct {
	MOVIE_CFG_REC_ID             strm_id;
	MOVIE_CFG_VID_IN             vid_in;
	MOVIE_CFG_IMG_SIZE           size;
	MOVIE_CFG_FRAME_RATE         frame_rate;
	MOVIE_CFG_TARGET_RATE        max_bit_rate;
	MOVIE_CFG_CODEC              codec;
	MOVIE_CFG_GOP_NUM            gop_num;
	MOVIE_CFG_AUD_CODEC          aud_codec;
	MOVIE_CFG_RTSP_STRM_OUT      rstp_strm_out;
	MOVIE_CFG_AQ_INFO            aq_info;
	MOVIE_CFG_CBR_INFO           cbr_info;
	BOOL                         ipl_set_enable;
	MOVIE_CFG_IMG_RATIO          ratio;
	BOOL                         userproc_pull;
} MOVIE_STRM_INFO;

typedef struct {
	UINT32 aud_rate;
	UINT32 aud_ch;
	UINT32 aud_ch_num;
} MOVIEMULTI_AUDIO_INFO;

typedef struct {
	UINT32                   ImgFmt;
	USIZE                    ImgSize;
	URECT                    Window;
	UINT32                   ImgFps;
} MOVIE_ALG_IPL_INFO;

typedef struct {
	MOVIE_ALG_IPL_INFO      path13;
	MOVIE_ALG_IPL_INFO      path14;
	MOVIE_CFG_REC_ID        rec_id;
} MOVIE_ALG_INFO;

typedef struct {
	UINT32 MaxImgLink;
	UINT32 MaxDispLink;
	UINT32 MaxWifiLink;
	UINT32 MaxAudCapLink;
	UINT32 MaxEthCamLink;
} MOVIEMULTI_MAX_LINK_INFO;

typedef struct {
	MOVIE_CFG_REC_ID       rec_id;
	HD_VIDEOCAP_DRV_CONFIG vcap_cfg;
	HD_VIDEO_PXLFMT        senout_pxlfmt;
	HD_VIDEO_PXLFMT        capout_pxlfmt;
	UINT32                 data_lane;
	SENSOR_PATH_INFO       ae_path;
	SENSOR_PATH_INFO       awb_path;
	SENSOR_PATH_INFO       af_path;
	SENSOR_PATH_INFO       iq_path;
	SENSOR_PATH_INFO       iq_shading_path;
	SENSOR_PATH_INFO       iq_dpc_path;
	SENSOR_PATH_INFO       iq_ldc_path;
	char                   file_path[32];
} MOVIE_SENSOR_INFO;

// emum for MOVIEMULTI_PARAM_RO_DEL_TYPE
typedef enum {
	RO_DEL_TYPE_PERCENT = 0,
	RO_DEL_TYPE_NUM,
	ENUM_DUMMY4WORD(MOVIE_RO_DEL_TYPE)
} MOVIE_RO_DEL_TYPE;

// emum for MOVIEMULTI_PARAM_VCAP_CCIR_MODE
typedef enum {
	CCIR_AUTO = 0,
	CCIR_INTERLACE,
	CCIR_PROGRESSIVE,
	ENUM_DUMMY4WORD(MOVIE_CCIR_MODE)
} MOVIE_CCIR_MODE;

typedef struct {
	UINT64              givenSpace;  //in: free space
	//NMEDIAREC_CALCSEC_SETTING info[8];
} MOVIEMULTI_CALCSEC_SETTING2;

typedef struct {
	MEM_RANGE           mem_range;
	CHAR               *p_root_path;
	CHAR                drive; // 'A' ~ 'Z'
} MOVIE_CFG_FDB_INFO;

typedef struct {
	MOVIE_CFG_REC_ID    rec_id;
	CHAR*               movie;
	CHAR*               emr;
	CHAR*               snapshot;
	BOOL                file_naming;
} MOVIEMULTI_RECODE_FOLDER_NAMING;

typedef struct {
	UINT32              width;
	UINT32              height;
	UINT32              codec;
	UINT32              header_size;
	UINT8              *header;
} MOVIEMULTI_ETHCAM_DEC_INFO;

typedef struct {
	UINT32              width;
	UINT32              height;
	UINT32              vfr;
	UINT32              tbr;
	UINT32              ar;
	UINT32              gop;
	UINT32              codec;
	UINT32              aud_codec;
	UINT32              rec_mode;
	UINT32              rec_format;
	UINT32              buf_pa;
	UINT32              buf_size;
	MOVIE_CFG_AQ_INFO   aq_info;
	MOVIE_CFG_CBR_INFO  cbr_info;
} MOVIEMULTI_ETHCAM_REC_INFO;

// structure for MOVIEMULTI_PARAM_VCAP_CROP
typedef struct {
	URECT Win;
} MOVIEMULTI_VCAP_CROP_INFO;

// structure for MOVIEMULTI_PARAM_MAIN_IME_CROP / MOVIEMULTI_PARAM_CLONE_IME_CROP / MOVIEMULTI_PARAM_DISP_IME_CROP
typedef struct {
	USIZE IMESize;
	URECT IMEWin;
} MOVIEMULTI_IME_CROP_INFO;

typedef struct {
	UINT32 en;
	UINT32 cnt1;
	UINT32 cnt2;
} MOVIEMULTI_BS_DROP_CNT;

#define MOVIEMULTI_DISK_MAX_DIR_NUM   8 //default

typedef struct {
	CHAR dir_path[128];
	UINT64 dir_size;
	UINT32 dir_ratio_of_disk;
	UINT32 file_num;
	UINT64 file_avg_size;
	UINT64 falloc_size;
	BOOL is_falloc;
	BOOL is_use;
} MOVIEMULTI_DIR_INFO;

typedef struct {
	UINT64 disk_size;
	UINT64 free_size;
	MOVIEMULTI_DIR_INFO dir_info[MOVIEMULTI_DISK_MAX_DIR_NUM];
	UINT64 other_size;
	UINT32 dir_num;
	UINT32 dir_depth;
	BOOL is_format_free; //0:init 1:true 2:false
} MOVIEMULTI_DISK_INFO;

typedef struct {
	UINT32 video_cap[4];
	UINT32 video_proc[4];
	UINT32 video_encode;
	UINT32 video_decode;
	UINT32 video_out;
} MOVIEMULTI_DRAM_CFG;

typedef struct {
	CHAR   path[128];
	UINT32 duration;
} MOVIEMULTI_CLOSE_FILE_INFO;

// structure for MOVIEMULTI_PARAM_CODEC_PROFILE_LEVEL
typedef struct {
	MOVIE_CFG_CODEC          codec;
	MOVIE_CFG_PROFILE        profile;
	MOVIE_CFG_LEVEL          level;
} MOVIE_CFG_CODEC_INFO;

// structure for MOVIEMULTI_PARAM_VPRC_FPS
typedef struct {
	UINT32                   id;                 ///< IAMOVIE_VPRC_EX_PATH
	UINT32                   fps;
} MOVIEMULTI_VPRC_FPS;

// structure for MOVIEMULTI_PARAM_VPRC_OUT_SIZE
typedef struct {
	UINT32                   id;                 ///< IAMOVIE_VPRC_EX_PATH
	BOOL                     crop_update;
	URECT                    crop_win;           ///< only valid if crop_update == TRUE
	BOOL                     out_update;
	USIZE                    out_size;           ///< only valid if out_update == TRUE
} MOVIEMULTI_VPRC_SIZE;

// structure for MOVIEMULTI_PARAM_VDO_ENC_PARAM
typedef struct {
	MOVIE_CFG_CODEC          codec;
	MOVIE_CFG_PROFILE        profile;
	MOVIE_CFG_LEVEL          level;
	UINT32                   fps;
	UINT32                   tbr;
	UINT32                   gop;
} MOVIEMULTI_VDO_ENC_PARAM;

// structure for MOVIEMULTI_PARAM_DIS_FUNC
typedef struct {
	BOOL                     enable;
	UINT32                   scale_ratio;       ///< config scale-ratio of DIS func (1100, 1200, 1400)
	MOVIE_CFG_LEVEL          subsample;         ///< config sub-sample of DIS func (0, 1, 2)
} MOVIEMULTI_DIS_PARAM;

// structure for MOVIEMULTI_PARAM_DIS_CROP_ALIGN
typedef struct {
	IAMOVIE_VPRC_EX_PATH     id;                ///< path need align
	UINT32                   align;             ///< align size
} MOVIEMULTI_DIS_CROP_ALIGN_PARAM;

// structure for MOVIEMULTI_PARAM_FILE_CUST_TAG
typedef struct {
	UINT32 on;
	UINT32 tag;
	UINT32 addr;
	UINT32 size;
} MOVIEMULTI_MOV_CUSTOM_TAG;

// structure for MOVIEMULTI_PARAM_FILE_ENCRYPT
typedef enum {
	MOVIEMULTI_ENCRYPT_TYPE_NONE              = 0x00000000,
	MOVIEMULTI_ENCRYPT_TYPE_CONTAINER         = 0x00000001,
	MOVIEMULTI_ENCRYPT_TYPE_I_FRAME           = 0x00000002,
	MOVIEMULTI_ENCRYPT_TYPE_P_FRAME           = 0x00000004,
	MOVIEMULTI_ENCRYPT_TYPE_ALL_FRAME         = 0x00000006,
	MOVIEMULTI_ENCRYPT_TYPE_AUDIO             = 0x00000008,
	ENUM_DUMMY4WORD(MOVIEMULTI_ENCRYPT_TYPE)
} MOVIEMULTI_ENCRYPT_TYPE;

typedef enum {
	MOVIEMULTI_ENCRYPT_MODE_AES128            = 0,
	MOVIEMULTI_ENCRYPT_MODE_AES256,
	ENUM_DUMMY4WORD(MOVIEMULTI_ENCRYPT_MODE)
} MOVIEMULTI_ENCRYPT_MODE;

typedef struct {
	MOVIEMULTI_ENCRYPT_TYPE type;
	MOVIEMULTI_ENCRYPT_MODE mode;
	UINT8                  *key;
	UINT32                  key_len;
} MOVIEMULTI_FILE_ENCRYPT;

// structure for MOVIEMULTI_PARAM_FILE_BUFLOCK_INFO
typedef struct {
	UINT32 enable;
	UINT32 lock_sec_ms;
	UINT32 timeout_ms;
} MOVIEMULTI_FILE_BUFLOCK;

// structure for MOVIEMULTI_PARAM_IMGCAP_JPG_QUALITY
typedef struct {
	UINT32 jpg_quality;                 // 1~100
	UINT32 buf_size;                    // unit: byte, real allocate buffer size will be buf_size * 1.5
} MOVIEMULTI_JPG_QUALITY;

/// ========== Gyro data area ==========
#define GYRO_DATA_NUM                           32
#define GYRO_DATA_SIZE                         (4 + 4 * GYRO_DATA_NUM * 6)

// =================== User Event Callback ===================
typedef enum {
	MOVIE_USER_CB_EVENT_START                 = 0x0000E300,
	// EVENT
	MOVIE_USER_CB_EVENT_VIDEO_READY           = MOVIE_USER_CB_EVENT_START,
	MOVIE_USER_CB_EVENT_STAMP_CB,                                            // value: (MP_VDOENC_YUV_SRC*)
	MOVIE_USER_CB_EVENT_REC_ONE_SECOND,
	MOVIE_USER_CB_EVENT_REC_STOP,
	MOVIE_USER_CB_EVENT_CARD_FULL,
	MOVIE_USER_CB_EVENT_REC_FILE_COMPLETED,
	MOVIE_USER_CB_EVENT_EMR_FILE_COMPLETED,
	MOVIE_USER_CB_EVENT_CARSH_FILE_COMPLETED,
	MOVIE_USER_CB_EVENT_SNAPSHOT_OK,
	MOVIE_USER_CB_EVENT_TIMELAPSE_ONESHOT,
	MOVIE_USER_CB_EVENT_FILENAMING_MOV_CB,
	MOVIE_USER_CB_EVENT_FILENAMING_EMR_CB,
	MOVIE_USER_CB_EVENT_FILENAMING_SNAPSHOT_CB,
	MOVIE_USER_CB_EVENT_KICKTHUMB,
	MOVIE_USER_CB_EVENT_CUTFILE,
	MOVIE_USER_CB_EVENT_CLOSE_RESULT,
	MOVIE_USER_CB_EVENT_OVERTIME,
	MOVIE_USER_CB_EVENT_RAWENC_PREPARED,
	MOVIE_USER_CB_EVENT_CLOSE_FILE_COMPLETED,
	MOVIE_USRR_CB_EVENT_TRIGGER_ETHCAM_THUMB,
	MOVIE_USRR_CB_EVENT_FILE_DELETED,
	MOVIE_USER_CB_EVENT_JENC_PREPARED,
	MOVIE_USER_CB_EVENT_JENC_DONE,
	MOVIE_USER_CB_EVENT_UVAC,
	MOVIE_USER_CB_EVENT_PREV_CARSH_FILE_COMPLETED,
	MOVIE_USER_CB_EVENT_SET_ENC_VUI,
	MOVIE_USER_CB_EVENT_SET_ENC_RC_INFO,
	MOVIE_USER_CB_EVENT_EXIF_DONE,

	// ERROR
	MOVIE_USER_CB_ERROR_CARD_SLOW             = 0x0000E400,
	MOVIE_USER_CB_ERROR_CARD_WR_ERR,
	MOVIE_USER_CB_ERROR_SEAMLESS_REC_FULL,
	MOVIE_USER_CB_ERROR_SNAPSHOT_ERR,
	MOVIE_USER_CB_ERROR_PROC_DATA_FAIL,
	MOVIE_USRR_CB_ERROR_FOUT_SLOW_STATUS,

	ENUM_DUMMY4WORD(MOVIE_USER_CB_EVENT)
} MOVIE_USER_CB_EVENT;

typedef enum {
	MOVIE_IMGCAP_THUMB = 0,
	MOVIE_IMGCAP_RAWENC,
	MOVIE_IMGCAP_EXIF_THUMB,
	ENUM_DUMMY4WORD(MOVIE_IMGCAP_TYPE)
} MOVIE_IMGCAP_TYPE;

// enum for MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE
typedef enum {
	MOVIE_IMGCAP_SOUT_PRIVATE = 0,
	MOVIE_IMGCAP_SOUT_COMMON,
	MOVIE_IMGCAP_SOUT_NONE,                 // not support this mode for NT9657x/NT9658x
	MOVIE_IMGCAP_SOUT_SHARED_PRIVATE,
	MOVIE_IMGCAP_SOUT_MAX,
	ENUM_DUMMY4WORD(MOVIE_IMGCAP_SOUT_TYPE)
} MOVIE_IMGCAP_SOUT_TYPE;

// enum for MOVIEMULTI_PARAM_IMGCAP_YUVSRC
typedef enum {
	MOVIE_IMGCAP_YUVSRC_MAIN = 0,
	MOVIE_IMGCAP_YUVSRC_CLONE,
	MOVIE_IMGCAP_YUVSRC_DISP,
	MOVIE_IMGCAP_YUVSRC_STRM,
	MOVIE_IMGCAP_YUVSRC_ALG,
	MOVIE_IMGCAP_YUVSRC_SOUT,
	MOVIE_IMGCAP_YUCSRC_MAX,
	ENUM_DUMMY4WORD(MOVIE_IMGCAP_YUVSRC)
} MOVIE_IMGCAP_YUVSRC;

// enum for MOVIEMULTI_PARAM_PSEUDO_REC_MODE
typedef enum {
	MOVIE_PSEUDO_REC_NONE = 0,                      /// normal recording
	MOVIE_PSEUDO_REC_MODE_1,                        /// only start vcap & vprc & fileout
	MOVIE_PSEUDO_REC_MODE_2,                        /// only start vcap & vprc & venc & fileout (acap & aenc if rec_mode==_CFG_REC_TYPE_AV)
	MOVIE_PSEUDO_REC_MODE_3,                        /// only start vcap & vprc & venc (acap & aenc if rec_mode==_CFG_REC_TYPE_AV)
	MOVIE_PSEUDO_REC_MODE_MAX,
	ENUM_DUMMY4WORD(MOVIE_PSEUDO_REC_TYPE)
} MOVIE_PSEUDO_REC_TYPE;

typedef enum {
	MOVIE_VPE_NONE        = 0x00000000,
	MOVIE_VPE_1           = 0x00000001,
	MOVIE_VPE_2           = 0x00000002,
	MOVIE_VPE_1_2         = (MOVIE_VPE_1 | MOVIE_VPE_2),
	ENUM_DUMMY4WORD(MOVIE_VPE)
} MOVIE_VPE;

// enum for MOVIEMULTI_PARAM_CODEC_TRIGGER_MODE
typedef enum {
	MOVIE_CODEC_TRIGGER_TIMER = 0,
	MOVIE_CODEC_TRIGGER_DIRECT,
	MOVIE_CODEC_TRIGGER_MAX,
	ENUM_DUMMY4WORD(MOVIE_CODEC_TRIGGER_MODE)
} MOVIE_CODEC_TRIGGER_MODE;

typedef struct {
	HD_PATH_ID        path_id;
	MOVIE_IMGCAP_TYPE type;
	USIZE             img_size;
	HD_VIDEO_FRAME*	  yuv_frame;
} MOVIE_IMGCAP_INFO;

typedef struct {
	UINT32 sign;
	UINT32 addr_pa;
	UINT32 addr_va;
	UINT32 size;
} MOVIE_JPEG_INFO;

typedef struct {
	MOVIE_CFG_REC_ID  rec_id;
	UINT32            vin_id;               /// BYTE[3]: reserved, BYTE[2]: ad_detcet_type (enum AD_DET_TYPE), BYTE[1]: chip_id, BYTE[0]: vin_id
	UINT32            vcap_id_bit;
} MOVIE_AD_MAP;

// Notice: need #include "ad.h"
#define MAKEADMAPID(chip_id, vin_id)                    (((chip_id & 0x000000ff) << 8) | (vin_id & 0x000000ff))
#define MAKEADMAPIDTYPE(chip_id, vin_id, ad_type)       (((ad_type & 0x000000ff) << 16) | ((chip_id & 0x000000ff) << 8) | (vin_id & 0x000000ff))


typedef void (MovieUserEventCb)(UINT32 id, MOVIE_USER_CB_EVENT event_id, UINT32 value);

typedef void (MovieAudCapCb)(UINT32 pa, UINT32 va, UINT32 size);

typedef void (MovieAudBsCb)(MOVIE_CFG_REC_ID id, HD_AUDIO_BS *paenc_data);

typedef void (MovieVdoBsCb)(MOVIE_CFG_REC_ID id, HD_VIDEOENC_BS *pvenc_data);

typedef void (*ETHCAM_SND_FUNC)(char* addr, int* size);

typedef enum {
	MOVIE_VPE_STATE_INIT,
	MOVIE_VPE_STATE_RUN,
	MOVIE_VPE_STATE_UNINIT,
} MOVIE_VPE_STATE;

typedef void (MovieVpeCb)(MOVIE_CFG_REC_ID id, MOVIE_VPE_STATE state, void *param);

// =================== Parameters ===================
enum {                                                                              /// Notice:
	MOVIEMULTI_PARAM_START = 0x0000ED000,                                           /// <xxx> means argument value uses xxx type/structure pointer to pass information
	// ipl group
	MOVIEMULTI_PARAM_IPL_GROUP_BEGIN = MOVIEMULTI_PARAM_START,
	MOVIEMULTI_PARAM_IPL_MIRROR = MOVIEMULTI_PARAM_IPL_GROUP_BEGIN,                 /// <UINT32> of <HD_VIDEO_DIR_XXX> definition (Set only)
	MOVIEMULTI_PARAM_VCAP_OUTFUNC,                                                  /// <UINT32> of <HD_VIDEOCAP_CTRLFUNC> enum combination (Set only)
	MOVIEMULTI_PARAM_IPL_FORCED_IMG_SIZE,                                           /// <UINT32> of <MOVIE_IPL_SIZE> enum (Set only)
	MOVIEMULTI_PARAM_IPL_USER_IMG_SIZE,                                             /// <MOVIEMULTI_IPL_SIZE_INFO *> structure pointer (Set only)
	MOVIEMULTI_PARAM_VCAP_CCIR_FMT,                                                 /// <UINT32> of <VENDOR_VIDEOCAP_CCIR_FMT_SEL> emum (Set only)
	MOVIEMULTI_PARAM_VCAP_CCIR_MIRROR_FLIP,                                         /// <UINT32> of <HD_VIDEO_DIR_XXX> definition, ccir sensor only (Set only)
	MOVIEMULTI_PARAM_VCAP_CCIR_MODE,                                                /// <UINT32> of <MOVIE_CCIR_MODE> enum, ccir sensor only (Set only)
	MOVIEMULTI_PARAM_VCAP_CROP,                                                     /// <MOVIEMULTI_VCAP_CROP_INFO *> structure pointer (Set only)
	MOVIEMULTI_PARAM_FORCED_UNIQUE_3DNR_PATH,                                       /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_FORCED_USE_YUVAUX,                                             /// <UINT32> of boolean, ccir sensor only, default: disable (Set only)
	MOVIEMULTI_PARAM_VCAP_OUT_SIZE,                                                 /// <USIZE *> structure pointer, sie scaling down setting, 9657x/9658x is not supported (Set only)
	MOVIEMULTI_PARAM_VCAP_CCIR_DET_LOOP,                                            /// <UINT32>, one loop will add extra 50ms delay (Set only)
	MOVIEMULTI_PARAM_VCAP_PAT_GEN,                                                  /// <UINT32> of <HD_VIDEOCAP_SEN_PATGEN_SEL> enum (Set only)
	MOVIEMULTI_PARAM_VPRC_FPS,                                                      /// <<<MOVIEMULTI_VPRC_FPS *>>> structure pointer, wifi path only (Set only)
	MOVIEMULTI_PARAM_DIS_FUNC,                                                      /// <MOVIEMULTI_DIS_PARAM *> structure pointer (Set only)
	MOVIEMULTI_PARAM_VPRC_OUT_SIZE,                                                 /// <MOVIEMULTI_VPRC_SIZE *> structure pointer (Set only)
	MOVIEMULTI_PARAM_DIS_CROP_ALIGN,                                                /// <MOVIEMULTI_DIS_CROP_ALIGN_PARAM *> structure pointer (Set only, IAMOVIE_VPRC_EX_DISP only)
	MOVIEMULTI_PARAM_SIE_REMAP,                                                     /// <UINT32>, enable to remap csi1 to sie2, default: disable (Set only)
	MOVIEMULTI_PARAM_VPE_EN,                                                        /// <UINT32> of <MOVIE_VPE> enum (Set only)
	MOVIEMULTI_PARAM_VPE_REG_CB,                                                    /// <MovieVpeCb *> callback function (Set only)
	MOVIEMULTI_PARAM_VPE_SCENE,                                                     /// <UINT32> of <VENDOR_VIDEOPROC_VPE_SCENE> enum (Set only)
	MOVIEMULTI_PARAM_FORCED_USE_VPE,                                                /// <UINT32> of boolean, NT98580/NT98529 only, ccir sensor only, default: disable
	MOVIEMULTI_PARAM_VCAP_GYRO_FUNC_EN,                                             /// <UINT32> of boolean, enable vcap gyro function, default: disable
	MOVIEMULTI_PARAM_VPRC_EIS_FUNC_EN,                                              /// <UINT32> of boolean, enable vprc eis function, default: disable
	MOVIEMULTI_PARAM_VPRC_QUEUE_DEPTH,                                              /// <UINT32> (Set only)
	MOVIEMULTI_PARAM_VPRC_EIS_FUNC_USE_VPE_LITE,                                    /// <UINT32> of boolean, use vpe_lite to process eis, default: disable (530 only)
	MOVIEMULTI_PARAM_VPRC_EIS_YUV_COMPRESS,                                         /// <UINT32> of boolean, yuv compress for vprc eis function, default: disable (530 only)
	MOVIEMULTI_PARAM_FORCED_USE_PIPE_SCALE,                                         /// <UINT32> of boolean, ccir sensor only, default: disable (Set only)
	MOVIEMULTI_PARAM_IPL_GROUP_END,
	// image group
	MOVIEMULTI_PARAM_IMG_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0100,
	MOVIEMULTI_PARAM_MAIN_IME_CROP,                                                 /// <MOVIEMULTI_IME_CROP_INFO *> structure pointer (Set only)
	MOVIEMULTI_PARAM_CLONE_IME_CROP,                                                /// <MOVIEMULTI_IME_CROP_INFO *> structure pointer (Set only)
	MOVIEMULTI_PARAM_VPRC_VPE_LITE_CROP,                                            /// <MOVIEMULTI_IME_CROP_INFO *> structure pointer (Set only) (530 only)
	MOVIEMULTI_PARAM_VPRC_NO_EXT,                                                   /// <UINT32> of boolean, not use vprc extension path, default: false
	MOVIEMULTI_PARAM_IMG_GROUP_END,
	// video codec group
	MOVIEMULTI_PARAM_CODEC_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0200,
	MOVIEMULTI_PARAM_CODEC = MOVIEMULTI_PARAM_CODEC_GROUP_BEGIN,                    /// <UINT32> of <MOVIE_CFG_CODEC> enum (Set / Get)
	MOVIEMULTI_PARAM_TIMELAPSE_TIME,                                                /// <UINT32>, timelapse time in unit ms (Set only)
	MOVIEMULTI_PARAM_ENCBUF_MS,                                                     /// <UINT32>, venc buffer time in unit ms (Set only)
	MOVIEMULTI_PARAM_VDO_ENC_REQUEST_I,                                             /// <UINT32>, always set 0 (Set only)
	MOVIEMULTI_PARAM_VDO_ENC_REG_CB,                                                /// <MovieVdoBsCb *> callback function (Set only)
	MOVIEMULTI_PARAM_VDO_QUALITY_BASE_MODE_EN,                                      /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_TARGETRATE,                                                    /// <UINT32>, tbr in unit byte (Set only)
	MOVIEMULTI_PARAM_TIMER_TRIGGER_COMPENSATION,                                    /// <UINT32> of boolean, default: enable (Set only)
	MOVIEMULTI_PARAM_VUI_EN,                                                        /// <UINT32> of boolean, default: enable (Set only)
	MOVIEMULTI_PARAM_VDO_ENC_MAX_FRAME_SIZE,                                        /// <UINT32>, size in unit byte (Set only)
	MOVIEMULTI_PARAM_CODEC_PROFILE_LEVEL,                                           /// <MOVIE_CFG_CODEC_INFO *> structure pointer, (Set / Get)
	MOVIEMULTI_PARAM_VDO_ENC_PARAM,                                                 /// <MOVIEMULTI_VDO_ENC_PARAM *> structure pointer, wifi path only (Set only)
	MOVIEMULTI_PARAM_CODEC_XVR_APP,                                                 /// <UINT32> of <VENDOR_VDOENC_XVR_APP> enum (Set only)
	MOVIEMULTI_PARAM_CODEC_SKIP_FRAME_MODE,                                         /// <UINT32>, HI16 for target fps, LO16 for src fps. Set 0 to disble (Set only)
	MOVIEMULTI_PARAM_CODEC_SVC,                                                     /// <UINT32>, of <HD_VIDEOENC_SVC_LAYER> enum
	MOVIEMULTI_PARAM_CODEC_LOW_POWER_MODE,                                          /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_CODEC_META_DATA_CFG,                                           /// <VENDOR_META_ALLOC *> structure pointer (Set only)
	MOVIEMULTI_PARAM_CODEC_TRIGGER_MODE,                                            /// <UINT32> of <MOVIE_CODEC_TRIGGER_MODE> enum, default: MOVIE_CODEC_TRIGGER_TIMER for all record path, MOVIE_CODEC_TRIGGER_DIRECT for streaming path (Set only)
	MOVIEMULTI_PARAM_CODEC_RRC,                                                     /// <HD_H26XENC_ROW_RC *> structure pointer, for row RC setting
	MOVIEMULTI_PARAM_CODEC_AQ,                                                      /// <HD_H26XENC_AQ *> structure pointer, for AQ setting
	MOVIEMULTI_PARAM_CODEC_MAXMEM,                                                  /// <HD_VIDEOENC_MAXMEM *> structure pointer, for venc max mem setting
	MOVIEMULTI_PARAM_CODEC_GROUP_END,
	// imgcap group
	MOVIEMULTI_PARAM_IMGCAP_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0300,
	MOVIEMULTI_PARAM_IMGCAP_EXIF_EN = MOVIEMULTI_PARAM_IMGCAP_GROUP_BEGIN,          /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_IMGCAP_THUM_SIZE,                                              /// <USIZE *> structure pointer (Set only)
	MOVIEMULTI_PARAM_IMGCAP_USE_VENC_BUF,                                           /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE,                                          /// <UINT32> of <MOVIE_IMGCAP_SOUT_TYPE> enum
	MOVIEMULTI_PARAM_IMGCAP_YUVSRC,                                                 /// <UINT32> of <MOVIE_IMGCAP_YUVSRC> enum (Set only)
	MOVIEMULTI_PARAM_IMGCAP_THUM_WITH_EXIF,                                         /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_IMGCAP_THUM_AUTO_SCALING_EN,                                   /// <UINT32> of boolean, enable this will re-calculated MP4 thumbnail size with aspect ratio, default: enable (Set only)
	MOVIEMULTI_PARAM_IMGCAP_EXIF_THUM_AUTO_SCALING_EN,                              /// <UINT32> of boolean, enable this will re-calculated EXIF thumbnail size with aspect ratio, default: enable (Set only)
	MOVIEMULTI_PARAM_IMGCAP_JPG_QUALITY,                                            /// <MOVIEMULTI_JPG_QUALITY *> structure pointer, set before ImageApp_MovieMulti_Open (Set only)
	MOVIEMULTI_PARAM_IMGCAP_SOUT_SHARED_BUF_SIZE,                                   /// <UINT32> of source out shared buffer size, only available when config MOVIE_IMGCAP_SOUT_SHARED_PRIVATE
	MOVIEMULTI_PARAM_IMGCAP_GROUP_END,
	// audio codec group
	MOVIEMULTI_PARAM_AUD_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0400,
	MOVIEMULTI_PARAM_AUD_MUTE_ENC = MOVIEMULTI_PARAM_AUD_GROUP_BEGIN,               /// <UINT32> of boolean, mute from acap if MOVIEMULTI_PARAM_AUD_MUTE_ENC_FUNC_EN is not set, default: disable (Set only)
	NOVIEMULTI_PARAM_AUD_ACAP_BY_HDAL,                                              /// <UINT32> of boolean, Linux SDK only, default: enable (Set only)
	MOVIEMULTI_PARAM_AUD_MUTE_ENC_FUNC_EN,                                          /// <UINT32> of boolean, keep acap but only mute aenc, default: disable (Set only)
	MOVIEMULTI_PARAM_AUD_CAP_REG_CB,                                                /// <MovieAudCapCb *> callback function (Set only)
	MOVIEMULTI_PARAM_AUD_ACAP_VOL,                                                  /// <UINT32>, 0~100 (Set / Get)
	MOVIEMULTI_PARAM_AUD_ACAP_ALC_EN,                                               /// <UINT32> of boolean, default: enable (Set only)
	MOVIEMULTI_PARAM_AUD_ENC_REG_CB,                                                /// <MovieAudBsCb *> callback function (Set only)
	MOVIEMULTI_PARAM_AUD_ACAP_ANR,                                                  /// <HD_AUDIOCAP_ANR *> structure pointer, set before ImageApp_MovieMulti_Open (Set only)
	MOVIEMULTI_PARAM_AUD_ACAP_DEFAULT_SETTING,                                      /// <UINT32> of <VENDOR_AUDIOCAP_DEFAULT_SETTING> enum, set after ImageApp_MovieMulti_Open and before audio start (Set only)
	MOVIEMULTI_PARAM_AUD_ACAP_NOISEGATE_THRESHOLD,                                  /// <INT32>, set after ImageApp_MovieMulti_Open and before audio start. -76 ~ -31 (Set only)
	MOVIEMULTI_PARAM_AUD_ENCBUF_MS,                                                 /// <UINT32>, aenc buffer time in unit ms (Set / Get)
	MOVIEMULTI_PARAM_AUD_ACAP_PREPWR_EN,                                            /// <UINT32> of boolean, default: enable (Set only)
	MOVIEMULTI_PARAM_AUD_CAPBUF_MS,                                                 /// <UINT32>, acap buffer time in unit ms (Set / Get)
	MOVIEMULTI_PARAM_AUD_PAUSE_PUSH,                                                /// <UINT32> of boolean, pause push audio bitstream to BsMux, default: false (Set / Get)
	MOVIEMULTI_PARAM_AUD_CODEC,                                                     /// <MOVIE_CFG_AUD_CODEC> audio codec, default: _CFG_AUD_CODEC_AAC (Set / Get)
	MOVIEMULTI_PARAM_AUD_ACAP_AEC,                                                  /// <HD_AUDIOCAP_AEC *> structure pointer, set before ImageApp_MovieMulti_Open (Set only)
	MOVIEMULTI_PARAM_AUD_GROUP_END,
	// file group
	MOVIEMULTI_PARAM_FILE_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0500,
	MOVIEMULTI_PARAM_FILE_FORMAT = MOVIEMULTI_PARAM_FILE_GROUP_BEGIN,               /// <UINT32> of <MOVIE_CFG_FILE_FORMAT> enum (Set only)
	MOVIEMULTI_PARAM_REC_FORMAT,                                                    /// <UINT32> of <MOVIE_CFG_REC_MODE> enum (Set only)
	MOVIEMULTI_PARAM_FILE_SEAMLESSSEC,                                              /// <UINT32>, seamless time in unit sec (Set only)
	MOVIEMULTI_PARAM_FILE_ENDTYPE,                                                  /// <UINT32> of <MOVREC_ENDTYPE_XXX> definition (Set only)
	MOVIEMULTI_PARAM_FILE_PLAYFRAMERATE,                                            /// <UINT32> (Set only)
	MOVIEMULTI_PARAM_FILE_BUFRESSEC,                                                /// <UINT32>, bsmux buffer (normal and emr path) in unit sec (Set only)
	MOVIEMULTI_PARAM_FILE_ROLLBACKSEC,                                              /// <UINT32>, EMR roll back time in unit sec (Set only)
	MOVIEMULTI_PARAM_FILE_KEEPSEC,                                                  /// <UINT32>, EMR keep time in unit sec (Set only)
	MOVIEMULTI_PARAM_FILE_EMRON,                                                    /// <UINT32> of <MOVIE_CFG_FILE_OPTION> enum combination (Set only)
	MOVIEMULTI_PARAM_FILE_FLUSH_SEC,                                                /// <UINT32>, bsmux flush time in unit sec (Set only)
	MOVIEMULTI_PARAM_FILE_WRITE_BLKSIZE,                                            /// <UINT32>, bsmux flush block size in unit byte, the value should aligned to MB (Set only)
	MOVIEMULTI_PARAM_FILE_GPS_DATA,                                                 /// <MEM_RANGE *> structure pointer (Set only)
	MOVIEMULTI_PARAM_FILE_FRONT_MOOV,                                               /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_FILE_FRONT_MOOV_FLUSH_SEC,                                     /// <UINT32>, bsmux flush time (front moov) in unit sec (Set only)
	MOVIEMULTI_PARAM_FILE_2V1A_MODE,                                                /// <UINT32> of boolean, change mode is required, default: disable (Set / Get)
	MOVIEMULTI_PARAM_FILE_BSMUX_FAST_PUT,                                           /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_FILE_DROP_FRAME_WITHOUT_SLOW_CB,                               /// <UINT32> of boolean, default: disable (Set to _CFG_CTRL_ID path only)
	MOVIEMULTI_PARAM_RO_DEL_TYPE,                                                   /// <UINT32> of <MOVIE_RO_DEL_TYPE> enum (Set only)
	MOVIEMULTI_PARAM_RO_DEL_PERCENT,                                                /// <UINT32>, RO delete percentage (Set only)
	MOVIEMULTI_PARAM_RO_DEL_NUM,                                                    /// <UINT32>, RO delete number (Set only)
	MOVIEMULTI_PARAM_FILE_GPS_EN,                                                   /// <UINT32> of boolean, default: enable (Set only)
	MOVIEMULTI_PARAM_FILE_GPS_RATE,                                                 /// <UINT32> (Set only)
	MOVIEMULTI_PARAM_FILE_MAX_POP_SIZE,                                             /// <UINT32>, file pop size in unit byte, the value should aligned to MB (Set only)
	MOVIEMULTI_PARAM_EMR_BSQ_INCARD,                                                /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_FILE_RECOVERY_FUNC_EN,                                         /// <UINT32> of boolean, enable/disable file recovery function, default: enable (Set to _CFG_CTRL_ID path only)
	MOVIEMULTI_PARAM_FILE_BITSTREAM_ALIGN_EN,                                       /// <UINT32> of boolean, enable/disable bitstream data align function, default: enable (Set to _CFG_CTRL_ID path only)
	MOVIEMULTI_PARAM_FILE_GPS_BUF_SIZE,                                             /// <UINT32>, gps data buffer size in unit byte (Set only)
	MOVIEMULTI_PARAM_FILE_CUST_TAG,                                                 /// <MOVIEMULTI_MOV_CUSTOM_TAG *> structure pointer, set after ImageApp_MovieMulti_Open (Set only)
	MOVIEMULTI_PARAM_FILE_ENCRYPT,                                                  /// <MOVIEMULTI_FILE_ENCRYPT *> structure pointer, set before ImageApp_MovieMulti_Open (Set only)
	MOVIEMULTI_PARAM_FILE_FREA_EN,                                                  /// <UINT32> of boolean, use "frea" (enable) or "skip" (disable) atom to save private info. default: disable (Set to _CFG_CTRL_ID path only)
	MOVIEMULTI_PARAM_FILE_UTC_AUTO_EN,                                              /// <UINT32> of boolean, enable it will not write time info into mp4 file. default: disable (Set to _CFG_CTRL_ID path only)
	MOVIEMULTI_PARAM_FILE_VAR_RATE,                                                 /// <UINT32>, play vfr, if set, the video file will become video only (Set only)
	MOVIEMULTI_PARAM_FILE_USE_FILEDB,                                               /// <UINT32> of boolean, use fileDB to manage files. defualt enable for ver. 1.00.xxx, but disable for ver. 1.00.xxxa (Set only)
	MOVIEMULTI_PARAM_FILE_CB_CLOSED_FILE_INFO,                                      /// <UINT32> of boolean, pass MOVIEMULTI_CLOSE_FILE_INFO when MOVIE_USER_CB_EVENT_CLOSE_FILE_COMPLETED callback. defualt disable for ver. 1.00.xxx, but enable for ver. 1.00.xxxa (Set only)
	MOVIEMULTI_PARAM_FILE_BUFRESSEC_EMR,                                            /// <UINT32>, bsmux buffer (emr path) in unit sec (Set only)
	MOVIEMULTI_PARAM_FILE_BUFLOCK_INFO,                                             /// <MOVIEMULTI_FILE_BUFLOCK *> structure pointer, the value of buf_sec_ms should < BUFRESSEC (Set only)
	MOVIEMULTI_PARAM_FILE_ROLLBACKSEC_CRASH,                                        /// <UINT32>, pre-record second for crash (update rollback_sec of MOVIE_RECODE_FILE_OPTION)
	MOVIEMULTI_PARAM_FILE_KEEPSEC_CRASH,                                            /// <UINT32>, post-record second for crash (update forward_sec of MOVIE_RECODE_FILE_OPTION)
	MOVIEMULTI_PARAM_FILE_SETCRASH_NOT_RO,                                          /// <UINT32>, of boolean, enable it will not set ro attrib on crash file. default: disable (Set only)
	MOVIEMULTI_PARAM_FILE_EMR_NOT_RO,                                               /// <UINT32>, of boolean, enable it will not set ro attrib on emr file. default: disable (Set only)
	MOVIEMULTI_PARAM_FILE_BSMUX_DDR_ID,                                             /// <UINT32>, ddr_id of bsmux buffer, default is DDR_ID0 (Set only)
	MOVIEMULTI_PARAM_FILE_META_DATA_CFG,                                            /// <HD_BSMUX_META_ALLOC *> structure pointer (Set only)
	MOVIEMULTI_PARAM_FILE_MAX_FILE_SIZE,                                            /// <UINT32>, file max size in unit byte (Set only)
	MOVIEMULTI_PARAM_FILE_CO64_EN,                                                  /// <UINT32> of boolean, enable it will use co64 instead of stco atom in mp4 file. default: enable (Set to _CFG_CTRL_ID path only)
	MOVIEMULTI_PARAM_FILE_GROUP_END,
	// display group
	MOVIEMULTI_PARAM_DISP_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0600,
	MOVIEMULTI_PARAM_DISP_REG_CB = MOVIEMULTI_PARAM_DISP_GROUP_BEGIN,               /// <MOVIE_RAWPROC_CB *> callback function (Set only)
	MOVIEMULTI_PARAM_DISP_IME_CROP,                                                 /// <MOVIEMULTI_IME_CROP_INFO *> structure pointer (Set only)
	MOVIEMULTI_PARAM_DISP_MIRROR_FLIP,                                              /// <UINT32> of <HD_VIDEO_DIR_XXX> definition (Set only)
	MOVIEMULTI_PARAM_DISP_IME_CROP_AUTO_SCALING_EN,                                 /// <UINT32> of boolean, enable this will re-calculated image size with aspect ratio, default: enable (Set only)
	MOVIEMULTI_PARAM_DISP_WINDOW,                                                   /// <HD_URECT *> structure pointer (Set only)
	MOVIEMULTI_PARAM_DISP_FPS,                                                      /// <UINT32> (Set only)
	MOVIEMULTI_PARAM_DISP_IME_CROP_FIXED_OUT_SIZE_EN,                               /// <UINT32> of boolean, enable this setting allow crop window size != display size, default: disable (Set only)
	MOVIEMULTI_PARAM_DISP_QUEUE_DEPTH,                                              /// <UINT32> (Set only)
	MOVIEMULTI_PARAM_DISP_GROUP_END,
	// wifi group
	MOVIEMULTI_PARAM_WIFI_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0700,
	MOVIEMULTI_PARAM_WIFI_REG_CB = MOVIEMULTI_PARAM_WIFI_GROUP_BEGIN,               /// <MOVIE_RAWPROC_CB *> callback function (Set only)
	MOVIEMULTI_PRARM_UVAC_FUNC,                                                     /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_UVAC_VEND_DEV_DESC,                                            /// <UVAC_VEND_DEV_DESC *> structure pointer (Set only)
	MOVIEMULTI_PARAM_UVAC_VID_RESO_ARY,                                             /// <UVAC_VID_RESO_ARY *> structure pointer (Set only)
	MOVIEMULTI_PARAM_UVAC_AUD_SAMPLERATE_ARY,                                       /// <UVAC_AUD_SAMPLERATE_ARY *> structure pointer (Set only)
	MOVIEMULTI_PARAM_UVAC_VID_FMT_TYPE,                                             /// <UINT32> of <UVAC_VIDEO_FORMAT_TYPE> enum (Set only)
	MOVIEMULTI_PARAM_UVAC_CDC_EN,                                                   /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_UVAC_CDC2_EN,                                                  /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_UVAC_CDC_PSTN_REQ_CB,                                          /// <UVAC_CDC_PSTN_REQUEST_CB *> callback function (Set only)
	MOVIEMULTI_PARAM_WIFI_VENC_TIMER_MODE,                                          /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_WIFI_GROUP_END,
	// misc group
	MOVIEMULTI_PARAM_MISC_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0800,
	MOVIEMULTI_PARAM_FILEDB_FILTER = MOVIEMULTI_PARAM_MISC_GROUP_BEGIN,             /// <UINT32> of <FILEDB_FMT_XXX> definition combination (Set only)
	MOVIEMULTI_PRARM_FILEDB_MAX_MUM,                                                /// <UINT32> (Set only)
	MOVIEMULTI_PARAM_YUV_COMPRESS,                                                  /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PRARM_DELETE_FILTER,                                                 /// <UINT32> of <FILEDB_FMT_XXX> definition combination (Set only)
	MOVIEMULTI_PARAM_PSEUDO_REC_MODE,                                               /// <UINT32> of <MOVIE_PSEUDO_REC_TYPE> enum (Set / Get)
	MOVIEMULTI_PARAM_TV_RANGE,                                                      /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_MANUAL_PUSH_VDO_FRAME,                                         /// <UINT32> of boolean, bind vprc->venc if set disable, default: disable (Set only)
	MOVIEMULTI_PARAM_MANUAL_PUSH_RAW_FRAME,                                         /// <UINT32> of boolean, bind vcap->vprc if set disable, default: disable (Set only)
	MOVIEMULTI_PARAM_MANUAL_PUSH_VPE_FRAME,                                         /// <UINT32> of boolean, bind vprc->vprc if set disable, for eis only, default: disable (Set only)
	MOVIEMULTI_PARAM_FLOW_FOR_DUMPED_MODE_FAST_BOOT,                                /// <UINT32> of boolean, flow for dumped mode fast boot, default: disable (Set only)
	MOVIEMULTI_PARAM_MISC_GROUP_END,
	// ethcam group
	MOVIEMULTI_PARAM_ETHCAM_GROUP_BEGIN = MOVIEMULTI_PARAM_START + 0x0900,
	MOVIEMULTI_RARAM_ETHCAM_RX_FUNC_EN = MOVIEMULTI_PARAM_ETHCAM_GROUP_BEGIN,       /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PRARM_ETHCAM_DEC_INFO,                                               /// <MOVIEMULTI_ETHCAM_DEC_INFO *> structure pointer (Set only)
	MOVIEMULTI_PRARM_ETHCAM_REC_INFO,                                               /// <MOVIEMULTI_ETHCAM_REC_INFO *> structure pointer (Set only)
	MOVIEMULTI_RARAM_ETHCAM_TX_CLONE_DIRECTTRIGGER_EN,                              /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PRARM_ETHCAM_JPG_ENC_ON_RX,                                          /// <UINT32> of boolean, default: disable (Set only)
	MOVIEMULTI_PARAM_ETHCAM_GROUP_END,
};

/// ========== Function Prototype area ==========
/// ImageApp_MovieMulti
extern ER ImageApp_MovieMulti_Open(void);
extern ER ImageApp_MovieMulti_Close(void);
extern ER ImageApp_MovieMulti_Config(UINT32 config_id, UINT32 value);
extern ER ImageApp_MovieMulti_FileOption(MOVIE_RECODE_FILE_OPTION *prec_option);
extern UINT32 ImageApp_MovieMulti_Recid2ImgLink(MOVIE_CFG_REC_ID id);
extern UINT32 ImageApp_MovieMulti_Recid2BsPort(MOVIE_CFG_REC_ID id);
extern UINT32 ImageApp_MovieMulti_BsPort2Recid(UINT32 port);
extern UINT32 ImageApp_MovieMulti_BsPort2Imglink(UINT32 port);
extern UINT32 ImageApp_MovieMulti_BsPort2EthCamlink(UINT32 port);
extern UINT32 ImageApp_MovieMulti_VePort2Imglink(UINT32 port);
extern HD_PATH_ID ImageApp_MovieMulti_GetVcapCtrlPort(MOVIE_CFG_REC_ID id);
extern HD_PATH_ID ImageApp_MovieMulti_GetVcapPort(MOVIE_CFG_REC_ID id);
extern HD_PATH_ID ImageApp_MovieMulti_GetVprcInPort(MOVIE_CFG_REC_ID id);
extern HD_PATH_ID ImageApp_MovieMulti_GetVprcSplitterInPort(MOVIE_CFG_REC_ID id);
extern HD_PATH_ID ImageApp_MovieMulti_GetVprc3DNRPort(MOVIE_CFG_REC_ID id);
extern HD_PATH_ID ImageApp_MovieMulti_GetVdoEncPort(MOVIE_CFG_REC_ID id);
extern HD_PATH_ID ImageApp_MovieMulti_GetRawEncPort(MOVIE_CFG_REC_ID id);
extern HD_PATH_ID ImageApp_MovieMulti_GetVprcPort(MOVIE_CFG_REC_ID id, IAMOVIE_VPRC_EX_PATH path);
extern HD_PATH_ID ImageApp_MovieMulti_GetVprcPhyPort(MOVIE_CFG_REC_ID id, IAMOVIE_VPRC_EX_PATH path);
extern HD_PATH_ID ImageApp_MovieMulti_GetAlgDataPort(MOVIE_CFG_REC_ID id, MOVIE_CFG_ALG_PATH path);
extern HD_PATH_ID ImageApp_MovieMulti_GetAudEncPort(MOVIE_CFG_REC_ID id);
extern void ImageApp_MovieMulti_Root_Path(CHAR *prootpath, UINT32 rec_id);
extern UINT32 ImageApp_MovieMulti_Folder_Naming(MOVIEMULTI_RECODE_FOLDER_NAMING *pfolder_naming);
extern BOOL ImageApp_MovieMulti_isFileNaming(MOVIE_CFG_REC_ID rec_id);
/// ImageApp_MovieMulti_AudCapLink
extern ER ImageApp_MovieMulti_AudCapStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_AudCapStop(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_AudCapGetFrame(MOVIE_CFG_REC_ID id, UINT8 *p_buf, UINT32 *p_len, INT32 wait_ms);
extern ER ImageApp_MovieMulti_AudTranscodeStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_AudTranscodeStop(MOVIE_CFG_REC_ID id);
/// ImageApp_MovieMulti_CB
extern ER ImageApp_MovieMulti_RegUserCB(MovieUserEventCb *fncb);
extern ER ImageApp_MovieMulti_InitCrash(MOVIE_CFG_REC_ID rec_id);
extern ER ImageApp_MovieMulti_SetCrash(MOVIE_CFG_REC_ID rec_id, UINT32 is_set);
/// ImageApp_MovieMulti_DispLink
extern ER ImageApp_MovieMulti_DispStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_DispStop(MOVIE_CFG_REC_ID id);
extern HD_RESULT ImageApp_MovieMulti_DispPullOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);
extern HD_RESULT ImageApp_MovieMulti_DispPushIn(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);
extern HD_RESULT ImageApp_MovieMulti_DispReleaseOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame);
extern HD_RESULT ImageApp_MovieMulti_DispWindowEnable(MOVIE_CFG_REC_ID id, UINT32 En);
/// ImageApp_MovieMulti_EthCamLink
extern ER ImageApp_MovieMulti_EthCamLinkForDisp(MOVIE_CFG_REC_ID id, UINT32 action, BOOL allow_pull);
extern UINT32 ImageApp_MovieMulti_EthCamLinkForDispStatus(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_EthCamLinkPushVdoBsToDecoder(MOVIE_CFG_REC_ID id, HD_VIDEODEC_BS *p_video_bs);
extern ER ImageApp_MovieMulti_EthCamLinkPushVdoBsToBsMux(MOVIE_CFG_REC_ID id, HD_VIDEOENC_BS *p_video_bs);
extern ER ImageApp_MovieMulti_EthCamLinkPushJpgToFileOut(MOVIE_CFG_REC_ID id, HD_FILEOUT_BUF *p_jpg);
extern ER ImageApp_MovieMulti_EthCamLinkPushThumbToBsMux(MOVIE_CFG_REC_ID id, HD_BSMUX_PUT_DATA *p_thumb_info, UINT32 is_emr);
/// ImageApp_MovieMulti_FileManage
extern INT32 ImageApp_MovieMulti_FMAllocate(CHAR drive);
extern INT32 ImageApp_MovieMulti_FMScan(CHAR drive, MOVIEMULTI_DISK_INFO* p_disk_info);
extern INT32 ImageApp_MovieMulti_FMDump(CHAR drive);
extern INT32 ImageApp_MovieMulti_FMOpen(CHAR drive);
extern INT32 ImageApp_MovieMulti_FMClose(CHAR drive);
extern INT32 ImageApp_MovieMulti_FMRefresh(CHAR drive);
extern INT32 ImageApp_MovieMulti_FMConfig(MOVIE_CFG_FDB_INFO *p_cfg);
extern void ImageApp_MovieMulti_FMSetSortBySN(CHAR *pDelimStr, UINT32 nDelimCount, UINT32 nNumOfSn);
/// ImageApp_MovieMulti_ImgCapLink
extern ER ImageApp_MovieMulti_ImgCapLink_Pending(BOOL status);
/// ImageApp_MovieMulti_ImgLink
extern HD_RESULT ImageApp_MovieMulti_RecPullOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);         // pull out vdofrm from vprc
extern HD_RESULT ImageApp_MovieMulti_RecPushIn(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);          // push in vdofrm to venc
extern HD_RESULT ImageApp_MovieMulti_RecReleaseOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame);                     // release vdofrm from vprc
extern HD_RESULT ImageApp_MovieMulti_RawFramePullOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);    // pull out rawfrm from vcap
extern HD_RESULT ImageApp_MovieMulti_RawFramePushIn(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);     // push in rawfrm to vprc
extern HD_RESULT ImageApp_MovieMulti_RawFrameReleaseOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame);                // release rawfrm from vcap
/// ImageApp_MovieMulti_Operation
extern ER ImageApp_MovieMulti_ImgLinkForDisp(MOVIE_CFG_REC_ID id, UINT32 action, BOOL allow_pull);
extern UINT32 ImageApp_MovieMulti_ImgLinkForDispStatus(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_ImgLinkForStreaming(MOVIE_CFG_REC_ID id, UINT32 action, BOOL allow_pull);
extern UINT32 ImageApp_MovieMulti_ImgLinkForStreamingStatus(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_ImgLinkForUVAC(MOVIE_CFG_REC_ID id, UINT32 action, BOOL allow_pull);
extern UINT32 ImageApp_MovieMulti_ImgLinkForUVACStatus(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_ImgLinkForAlg(MOVIE_CFG_REC_ID id, MOVIE_CFG_ALG_PATH path, UINT32 action, BOOL allow_pull);
extern UINT32 ImageApp_MovieMulti_ImgLinkForAlgStatus(MOVIE_CFG_REC_ID id, MOVIE_CFG_ALG_PATH path);
extern ER ImageApp_MovieMulti_RecStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_RecStop(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_TriggerSnapshot(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_TrigEMR(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_TrigOnce(MOVIE_CFG_REC_ID id, UINT32 sec);
extern ER ImageApp_MovieMulti_EthCamTxStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_EthCamTxStop(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_EthCam_TxTriggerThumb(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_TranscodeStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_TranscodeStop(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_ImgLinkPreStart(MOVIE_CFG_REC_ID id, IAMOVIE_VPRC_EX_PATH path);
extern ER ImageApp_MovieMulti_ImgLinkPostStop(MOVIE_CFG_REC_ID id, IAMOVIE_VPRC_EX_PATH path);
extern ER ImageApp_MovieMulti_Mode3_RecStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_Mode3_RecStop(MOVIE_CFG_REC_ID id);
/// ImageApp_MovieMulti_Param
extern ER ImageApp_MovieMulti_SetParam(MOVIE_CFG_REC_ID id, UINT32 param, UINT32 value);
extern ER ImageApp_MovieMulti_GetParam(MOVIE_CFG_REC_ID id, UINT32 param, UINT32* value);
/// ImageApp_MovieMulti_Utilities
extern UINT32 ImageApp_MovieMulti_IsStreamRunning(MOVIE_CFG_REC_ID id);
extern UINT32 ImageApp_MovieMulti_GetFreeRec(HD_BSMUX_CALC_SEC *pSetting);
extern UINT32 ImageApp_MovieMulti_GetRemainSize(CHAR drive, UINT64 *p_size);
extern UINT32 ImageApp_MovieMulti_SetReservedSize(CHAR drive, UINT64 size, UINT32 is_loop);
extern UINT32 ImageApp_MovieMulti_SetExtendCrashInfo(MOVIE_CFG_REC_ID id, UINT32 unit, UINT32 max_num, UINT32 enable);
extern UINT32 ImageApp_MovieMulti_IsExtendCrash(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_SetExtendCrash(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_SetCutNow(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_SetCutNowWithPeriod(MOVIE_CFG_REC_ID id, UINT32 sec, UINT32 protected);
extern ER ImageApp_MovieMulti_GetDropCnt(MOVIE_CFG_REC_ID id, MOVIEMULTI_BS_DROP_CNT *pcnt);
extern ER ImageApp_MovieMulti_GetAudEncBufInfo(MOVIE_CFG_REC_ID id, HD_AUDIOENC_BUFINFO *pinfo);
extern ER ImageApp_MovieMulti_GetVdoEncBufInfo(MOVIE_CFG_REC_ID id, HD_VIDEOENC_BUFINFO *pinfo);
extern ER ImageApp_MovieMulti_GetVcapSysInfo(MOVIE_CFG_REC_ID id, HD_VIDEOCAP_SYSINFO *pinfo);
extern UINT32 ImageApp_MovieMulti_DetBufUsage(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_SetUTCTimeInfo(MOVIE_CFG_REC_ID id, struct tm *p_timeinfo);
extern INT32 ImageApp_MovieMulti_SetFormatFree(CHAR drive, BOOL is_on);
extern INT32 ImageApp_MovieMulti_SetFolderInfo(CHAR *p_path, UINT32 ratio, UINT64 f_size);
extern INT32 ImageApp_MovieMulti_GetFolderInfo(CHAR *p_path, UINT32 *ratio, UINT64 *f_size);
extern INT32 ImageApp_MovieMulti_SetCyclicRec(CHAR *p_path, BOOL is_on);
extern INT32 ImageApp_MovieMulti_SetExtStrType(CHAR drive, UINT32 type);
extern INT32 ImageApp_MovieMulti_SetUseHiddenFirst(CHAR drive, BOOL value);
extern INT32 ImageApp_MovieMulti_SetSyncTime(CHAR drive, BOOL value);
extern INT32 ImageApp_MovieMulti_SetFileUnit(CHAR *p_path, UINT32 unit);
extern INT32 ImageApp_MovieMulti_SetSkipReadOnly(CHAR drive, BOOL value);
extern ER ImageApp_MovieMulti_GetDesc(MOVIE_CFG_REC_ID id, UINT32 codec, PMEM_RANGE desc, PMEM_RANGE sps, PMEM_RANGE pps, PMEM_RANGE vps);
/// ImageApp_MovieMulti_WifiLink
extern ER ImageApp_MovieMulti_StreamingStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_StreamingStop(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_UvacStart(MOVIE_CFG_REC_ID id);
extern ER ImageApp_MovieMulti_UvacStop(MOVIE_CFG_REC_ID id);
extern HD_RESULT ImageApp_MovieMulti_WifiPullOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);
extern HD_RESULT ImageApp_MovieMulti_WifiPushIn(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);
extern HD_RESULT ImageApp_MovieMulti_WifiReleaseOut(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame);

extern ER ImageApp_MovieMulti_EthCamTxRecId1_CBLink(HD_VIDEOENC_BS *pData, UINT32 SndFunc);
extern ER ImageApp_MovieMulti_EthCamTxCloneId1_CBLink(HD_VIDEOENC_BS *pvenc_data, UINT32 SndFunc);
extern ER ImageApp_MovieMulti_EthCamTxRecId1_DumpBsInfo(void);
extern ER ImageApp_MovieMulti_EthCamTxCloneId1_DumpBsInfo(void);
extern ER ImageApp_MovieMulti_EthCamTxRecId1_SetRawEncodeBuff(UINT32 WorkAddr, UINT WorkSize);
extern ER ImageApp_MovieMulti_EthCamTxRecId1_TriggerRawEncode(UINT32 addr_va, UINT32 size);
extern ER ImageApp_MovieMulti_EthCamTxRecId1_SetThumbBuff(UINT32 WorkAddr, UINT WorkSize);
extern ER ImageApp_MovieMulti_EthCamTxRecId1_TriggerThumb(UINT32 addr_va, UINT32 size);

extern ER ImageApp_MovieMulti_EthCamTxCloneId1_SetRawEncodeBuff(UINT32 WorkAddr, UINT WorkSize);
extern ER ImageApp_MovieMulti_EthCamTxCloneId1_SetVdoEncVaAddr(UINT32 Pathid);
extern ER ImageApp_MovieMulti_EthCamTxRecId1_SetVdoEncVaAddr(UINT32 Pathid);

// =================== Special definition area start ===================
#define ETHCAM_TX_MAGIC_KEY        0x67100000

#endif//IMAGEAPP_MOVIEMULTI_H
