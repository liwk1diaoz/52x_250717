/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       ImageApp_Photo.h
    @ingroup    mIImageApp

    @note       Nothing.

    @date       2017/06/01
*/

#ifndef IA_PHOTO_H
#define IA_PHOTO_H

#include "hdal.h"
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "ImageApp/ImageApp_Common.h"
#include "ImageApp_Photo_CapCbMsg.h"
#include "ImageApp_Photo_CapInfor.h"
//#include "kdrv_videoenc/kdrv_videoenc.h"

//#include "GxLib.h"

//#include "ImageStream.h"
//#include "ImageUnit_ImagePipe.h"
//#include "ImageUnit_VdoOut.h"
//#include "ImageUnit_CamFile.h"
//#include "ImageUnit_Cap.h"
//#include "NVTEvent.h"
//#include "VideoEncode.h"

/**
    @addtogroup mIImageApp
*/
//@{

#if 0
// Debug Macro
#define DBG_DUMP printf
#define DBG_WRN(fmtstr, args...) DBG_DUMP("\033[33m%s(): \033[0m" fmtstr, __func__, ##args)
#define DBG_IND(fmtstr, args...) DBG_DUMP("%s(): " fmtstr, __func__, ##args)
#define DBG_ERR(fmtstr, args...) DBG_DUMP("\033[31m%s(): \033[0m" fmtstr, __func__, ##args)
#define DBGD(x) DBG_DUMP("\033[0;35m%s=%d\033[0m\r\n", #x, x)
#define DBGH(x) DBG_DUMP("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define CHKPNT DBG_DUMP("\033[37mCHK: %d, %s\033[0m\r\n", __LINE__, __func__)
#endif
/**
     @name Configuration ID for Config().
*/
//@{
#define PHOTO_CFG_DISP_INFO          1      ///<  photo display info
#define PHOTO_CFG_STRM_INFO          2      ///<  photo streaming info
#define PHOTO_CFG_IME3DNR_INFO       3      ///<  photo IME3DNR info
#define PHOTO_CFG_STRM_TYPE          4      ///<  photo streaming type, HTTP or RTSP
#define PHOTO_CFG_FBD_POOL           5      ///<  filedb pool memory range
#define PHOTO_CFG_ROOT_PATH          6      ///<  photo write file root path
#define PHOTO_CFG_FOLDER_NAME        7      ///<  photo write file folder name
#define PHOTO_CFG_FILE_NAME_CB       8      ///<  photo write file name
#define PHOTO_CFG_DUAL_DISP          9      ///<  photo dual disp
#define PHOTO_CFG_DRAW_CB           10      ///<  photo draw callback
#define PHOTO_CFG_CBR_INFO          11      ///<  photo Vdo Enc CBR info
#define PHOTO_CFG_DISP_REG_CB       12      ///<  photo reg  disp pip ondraw callback
#define PHOTO_CFG_WIFI_REG_CB       13      ///<  photo reg  wifi pip ondraw callback
#define PHOTO_CFG_FILEDB_FILTER     14      ///<  photo filedb filter
#define PHOTO_CFG_FILEDB_MAX_NUM    15      ///<  photo filedb max file number
#define PHOTO_CFG_IPL_MIRROR        16      ///<  photo set ipl mirror/flip

#define PHOTO_CFG_SENSOR_INFO       17
#define PHOTO_CFG_VOUT_INFO         18      ///<  photo vout info
#define PHOTO_CFG_MEM_POOL_INFO     19
#define PHOTO_CFG_CAP_INFO          20
#define PHOTO_CFG_CAP_WRITEFILE_CB  21
#define PHOTO_CFG_CAP_MSG_CB        22
#define PHOTO_CFG_CAP_DETS2_CB      23
#define PHOTO_CFG_3DNR_PATH         24
#define PHOTO_CFG_DISP_IME_CROP     25      ///<  photo set disp ime crop
#define PHOTO_CFG_VCAP_PAT_GEN      26      ///<  photo set  vcap pattern generation
#define PHOTO_CFG_VCAP_OUTFUNC      27      ///<  photo set vcap func
#define PHOTO_CFG_VCAP_CTRLPORT     28      ///<  photo get vcap ctrl port
#define PHOTO_CFG_VCAP_SENSOR_SIZE  29      ///<  photo set vcap in size
#define PHOTO_CFG_VENC_BS_CB        30      ///<  photo set venc bs callback ()

//@}

typedef void(PHOTO_DRAW_CB)(UINT32  yAddr, UINT32 cbAddr, UINT32 crAddr, UINT32 lineOfsY);                 ///< Callback for draw image.
//typedef void (*PHOTO_DISP_PIP_PROCESS_CB)(void);
//typedef void (*PHOTO_WIFI_PIP_PROCESS_CB)(void);
typedef void (*PHOTO_PROCESS_CB)(void);
typedef void (*PHOTO_PROCESS_PARAM_CB)(UINT32 idx);
typedef void(PHOTO_FILENAME_CB)(UINT32 id, CHAR *pFileName);
typedef INT32(*PHOTO_CAP_FSYS_FP)(UINT32 Addr, UINT32 Size, UINT32 Fmt, INT32 SensorID);
typedef void (*PHOTO_CAP_CBMSG_FP)(IMG_CAP_CBMSG Msg, void *Data);
typedef BOOL (*PHOTO_CAP_S2DET_FP)(void);
typedef void (PhotoVdoBsCb)(UINT32 id, HD_VIDEOENC_BS *pvenc_data);

typedef enum {
	PHOTO_DISP_ID_MIN = 0,
	PHOTO_DISP_ID_1 = PHOTO_DISP_ID_MIN,
	PHOTO_DISP_ID_2,
	PHOTO_DISP_ID_3,
	PHOTO_DISP_ID_4,
	PHOTO_DISP_ID_MAX,
	ENUM_DUMMY4WORD(PHOTO_DISP_ID)
} PHOTO_DISP_ID;

typedef enum {
	PHOTO_STRM_ID_MIN = PHOTO_DISP_ID_MAX,
	PHOTO_STRM_ID_1 = PHOTO_STRM_ID_MIN,
	PHOTO_STRM_ID_2,
	PHOTO_STRM_ID_3,
	PHOTO_STRM_ID_4,
	PHOTO_STRM_ID_MAX,
	ENUM_DUMMY4WORD(PHOTO_STRM_ID)
} PHOTO_STRM_ID;

typedef enum {
	PHOTO_IME3DNR_ID_MIN = PHOTO_STRM_ID_MAX,
	PHOTO_IME3DNR_ID_1 = PHOTO_IME3DNR_ID_MIN,
	PHOTO_IME3DNR_ID_2,
	PHOTO_IME3DNR_ID_MAX,
	ENUM_DUMMY4WORD(PHOTO_IME3DNR_ID)
} PHOTO_IME3DNR_ID;

typedef enum {
	PHOTO_CAP_STRM_ID = PHOTO_IME3DNR_ID_MAX,
	PHOTO_CAP_STRM_ID_MAX,
	ENUM_DUMMY4WORD(_PHOTO_CAP_STRM_ID)
} _PHOTO_CAP_STRM_ID;

typedef enum {
	PHOTO_VID_IN_1 = 0,
	PHOTO_VID_IN_2,
	PHOTO_VID_IN_3,
	PHOTO_VID_IN_4,
	PHOTO_VID_IN_MAX,
	ENUM_DUMMY4WORD(PHOTO_VID_IN)
} PHOTO_VID_IN;

typedef enum {
	PHOTO_VID_OUT_1 = 0,
	PHOTO_VID_OUT_2,
	PHOTO_VID_OUT_MAX,
	ENUM_DUMMY4WORD(PHOTO_VID_OUT)
} PHOTO_VID_OUT;


typedef enum {
	PHOTO_CAP_ID_1 = 0,
	PHOTO_CAP_ID_2,
	PHOTO_CAP_ID_3,
	PHOTO_CAP_ID_4,
	PHOTO_CAP_ID_MAX,
	ENUM_DUMMY4WORD(PHOTO_CAP_ID)
} PHOTO_CAP_ID;

typedef enum {
    PHOTO_3DNR_SHARED_PATH = 0,
    PHOTO_3DNR_REF_PATH, // reference path
    ENUM_DUMMY4WORD(PHOTO_3DNR_PATH)
} PHOTO_3DNR_PATH;

typedef enum {
	PHOTO_CODEC_MJPG = 1,
	PHOTO_CODEC_H264,
	PHOTO_CODEC_H265,
	ENUM_DUMMY4WORD(PHOTO_CODEC)
} PHOTO_CODEC;

typedef enum {
	PHOTO_STRM_TYPE_HTTP = 0,
	PHOTO_STRM_TYPE_RTSP,
	ENUM_DUMMY4WORD(PHOTO_STRM_TYPE)
} PHOTO_STRM_TYPE;


typedef enum {
	PHOTO_MULTI_VIEW_SINGLE = 0,         ///<  single sensor view
	PHOTO_MULTI_VIEW_PIP,                ///<  pip view
	PHOTO_MULTI_VIEW_SBS_LR,             ///<  sbs Left, Right view
	PHOTO_MULTI_VIEW_SBS_UD,             ///<  sbs Up , Down view
	ENUM_DUMMY4WORD(PHOTO_MULTI_VIEW)
} PHOTO_MULTI_VIEW;

typedef enum {
	PHOTO_PORT_STATE_EN_RUN = 1,  //enable and run
	PHOTO_PORT_STATE_EN,          //enable but state off
	ENUM_DUMMY4WORD(PHOTO_PORT_STATE)
} PHOTO_PORT_STATE;

typedef enum {
    PHOTO_HDAL_VCAP_CAP_PATH = 0x00000100,
    PHOTO_HDAL_VPRC_3DNR_REF_PATH = 0x00000200,
    ENUM_DUMMY4WORD(PHOTO_HDAL_PATH)
} PHOTO_HDAL_PATH;

typedef struct {
	PHOTO_PORT_STATE                 enable;
	PHOTO_VID_IN         vid_in;
	HD_VIDEOCAP_DRV_CONFIG vcap_cfg;
	HD_VIDEOCAP_CTRL vcap_ctrl;
	HD_VIDEO_PXLFMT senout_pxlfmt;
	HD_VIDEO_PXLFMT capout_pxlfmt;
	UINT32 data_lane;
	SENSOR_PATH_INFO       ae_path;
	SENSOR_PATH_INFO       awb_path;
	SENSOR_PATH_INFO       af_path;
	SENSOR_PATH_INFO       iq_path;
	SENSOR_PATH_INFO       iqcap_path;
	SENSOR_PATH_INFO       iq_shading_path;
	SENSOR_PATH_INFO       iq_dpc_path;
	SENSOR_PATH_INFO       iq_ldc_path;
	char                   		file_path[32];

	UINT32                 fps[4];
	ISIZE                  sSize[4];
	HD_VIDEOPROC_CTRLFUNC vproc_func;

	HD_PATH_ID           vcap_p[1];

	HD_PATH_ID           vproc_p_phy[2][5]; //[2] 0 for liveview/wifi; 1 for cap; [5] 0~2 IME path

	HD_PATH_ID           vproc_p_ext[3];

	HD_PATH_ID video_cap_ctrl;
} PHOTO_SENSOR_INFO;

typedef struct {
	PHOTO_PORT_STATE                 enable;
	PHOTO_DISP_ID        disp_id;
	PHOTO_VID_IN         vid_in;
#if 1
	PHOTO_MULTI_VIEW     multi_view_type;      ///<  enable PIP view
	UINT32               width;
	UINT32               height;
	UINT32               width_ratio;
	UINT32               height_ratio;
	UINT32               rotate_dir;
	UINT32               is_merge_3dnr_path;
	PHOTO_VID_OUT        vid_out;
#endif
	HD_VIDEOOUT_SYSCAPS  vout_syscaps;
	HD_PATH_ID           vout_p_ctrl;
	HD_PATH_ID           vout_p[1];
	USIZE      		vout_ratio;

	//HD_PATH_ID vout_ctrl;
	//HD_PATH_ID vout_path;
} PHOTO_DISP_INFO;

typedef struct {
	HD_PATH_ID vout_ctrl;
	HD_PATH_ID vout_path;
	USIZE      	vout_ratio;
} PHOTO_VOUT_INFO;

typedef struct {
	PHOTO_PORT_STATE                 enable;
	PHOTO_CAP_ID         cap_id;
	PHOTO_VID_IN         vid_in;
	ISIZE                  sCapSize;
	ISIZE                  sCapMaxSize;
	UINT32			quality;
	UINT32			qv_img;
	ISIZE			qv_img_size;
	UINT32			qv_img_fmt;
	UINT32			screen_img;
	ISIZE			screen_img_size;
	UINT32			screen_fmt;
	UINT32			screen_bufsize;

	UINT32			thumb_fmt;

	SEL_DATASTAMP			datastamp;
	UINT32			picnum;
	UINT32			jpgfmt;
	UINT32			filefmt;
	UINT32                img_ratio;

	UINT32               reenctype;
	UINT32               rho_initqf;
	UINT32               rho_targetsize;
	UINT32               rho_lboundsize;
	UINT32               rho_hboundsize;
	UINT32               rho_retrycnt;
	UINT32			filebufsize;

	UINT32			raw_buff;
	UINT32			jpg_buff;
	UINT32			yuv_buff;

	HD_PATH_ID 		venc_p;

	UINT32			actflag;

} PHOTO_CAP_INFO;


typedef struct {
	PHOTO_PORT_STATE                 enable;
	PHOTO_MULTI_VIEW     multi_view_type;      ///<  enable PIP view
	PHOTO_STRM_ID        strm_id;
	PHOTO_VID_IN         vid_in;
	UINT32               width;
	UINT32               height;
	UINT32               width_ratio;
	UINT32               height_ratio;
	UINT32               frame_rate;
	UINT32               max_width;
	UINT32               max_height;
	UINT32               max_bitrate;
	UINT32               target_bitrate;
	PHOTO_CODEC          codec;
	PHOTO_STRM_TYPE      strm_type;
	UINT32               is_merge_3dnr_path;

	HD_PATH_ID           venc_p[1];

} PHOTO_STRM_INFO;

typedef struct {
    PHOTO_PORT_STATE                 enable;
	PHOTO_IME3DNR_ID     ime3dnr_id;
	PHOTO_VID_IN         vid_in;
	UINT32               width;
	UINT32               height;
	UINT32               width_ratio;
	UINT32               height_ratio;
	UINT32               max_width;
	UINT32               max_height;
	UINT32               is_merge_3dnr_path;
} PHOTO_IME3DNR_INFO;


typedef struct {
	PHOTO_CAP_ID         cap_id;
	UINT32               ipl_id;
	CHAR                *folder_path;
} PHOTO_CAP_FOLDER_NAMING;

typedef struct {
	BOOL enable;
	ISIZE disp_size;
	ISIZE disp_ar;
} PHOTO_DUAL_DISP;

typedef struct {
	UINT32 uiEnable;
	UINT32 uiStaticTime;
	UINT32 uiByteRate;
	UINT32 uiFrameRate;
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
} PHOTO_STRM_VENC_CBR_INFO;

typedef struct {
	BOOL                 enable;
	PHOTO_STRM_ID        strm_id;
	PHOTO_STRM_VENC_CBR_INFO      cbr_info;
} PHOTO_STRM_CBR_INFO;

typedef struct {
	//ISF_UNIT           *p_vdoOut;
	UINT32           *p_vdoOut;
} PHOTO_VDO_OUT_INFO;

typedef struct {
	PHOTO_VID_IN         vid;
	UINT32              mirror_type;
} PHOTO_IPL_MIRROR;

typedef struct {
	UINT32 enable;
	PHOTO_VID_IN         vid;
	USIZE IMESize;
	URECT IMEWin;
} PHOTO_IME_CROP_INFO;

typedef struct {
	PHOTO_VID_IN         vid;
	UINT32              patgen_type;
} PHOTO_VCAP_PAT_GEN;

typedef struct {
	PHOTO_VID_IN         vid;
	HD_VIDEOCAP_OUTFUNC out_func;
} PHOTO_VCAP_OUTFUNC;

typedef struct {
	PHOTO_VID_IN         vid;
	HD_DIM 			in_size;
	HD_VIDEO_FRC 	frc;
} PHOTO_VCAP_SENSOR_MODE_CFG; //change cap sensor size

/**
    Config settings for Photo mode.

    Config settings for Photo mode.

    @param[in] config_id PHOTO_CFG_DISP_INFO or others.
    @param[in] value configuration value.

    @return void

    Example:
    @code
    {
        PHOTO_DISP_INFO  dscam_disp;

        dscam_disp.enable          =  TRUE;
        dscam_disp.enable_pip      =  FALSE;
        dscam_disp.disp_id         =  PHOTO_DISP_ID_1;
        dscam_disp.vid_in          =  PHOTO_VID_IN_1;
        dscam_disp.width           =  640;
        dscam_disp.height          =  480;
        dscam_disp.width_ratio     =  4;
        dscam_disp.height_ratio    =  3;
        ImageApp_Photo_Config(PHOTO_CFG_DISP_INFO, (UINT32)&dscam_disp);
    }
    @endcode
*/
extern void ImageApp_Photo_Config(UINT32 config_id, UINT32 value);


extern UINT32 ImageApp_Photo_GetConfig(UINT32 config_id, UINT32 param1);


/**
    Open photo mode.

    Open photo mode.

    @param[in] void

    @return void
*/
extern void ImageApp_Photo_Open(void);

/**
    Close photo mode.

    Close photo mode.

    @param[in] void

    @return void
*/

extern INT32 ImageApp_Photo_FileNaming_Open(void);

extern void ImageApp_Photo_FileNaming_SetSortBySN(CHAR *pDelimStr, UINT32 nDelimCount, UINT32 nNumOfSn);


extern void ImageApp_Photo_SetVdoImgSize(UINT32 Path, UINT32 w, UINT32 h);

extern void ImageApp_Photo_SetVdoAspectRatio(UINT32 Path, UINT32 w, UINT32 h);

extern void ImageApp_Photo_ResetPath(UINT32 Path);


extern void ImageApp_Photo_Close(void);

extern UINT32 ImageApp_Photo_CapStart(void);

extern void ImageApp_Photo_CapStop(void);

extern void ImageApp_Photo_CapWaitFinish(void);

extern CHAR* ImageApp_Photo_GetLastWriteFilePath(void);

extern UINT32 ImageApp_Photo_SetScreenSleep(void);

extern UINT32 ImageApp_Photo_SetScreenWakeUp(void);

extern void ImageApp_Photo_InstallID(void);

extern void ImageApp_Photo_DispConfig(UINT32 config_id, UINT32 value);

extern UINT32 ImageApp_Photo_UpdateImgLinkForDisp(PHOTO_DISP_ID disp_id, UINT32 action, BOOL allow_pull);

extern void ImageApp_Photo_DispGetVdoImgSize(UINT32 Path, UINT32* w, UINT32* h);

extern HD_RESULT ImageApp_Photo_DispPullOut(PHOTO_DISP_INFO *p_disp_info, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);

extern HD_RESULT ImageApp_Photo_DispPushIn(PHOTO_DISP_INFO *p_disp_info, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);

extern HD_RESULT ImageApp_Photo_DispReleaseOut(PHOTO_DISP_INFO *p_disp_info, HD_VIDEO_FRAME* p_video_frame);

extern UINT32 ImageApp_Photo_Disp_IsPreviewShow(void);

extern void ImageApp_Photo_Disp_SetPreviewShow(UINT32 IsShow);

extern ER PhotoWiFiTsk_Open(void);

extern ER PhotoWiFiTsk_Close(void);

extern void PhotoWiFiTsk(void);

extern void ImageApp_Photo_WiFiConfig(UINT32 config_id, UINT32 value);

extern ER ImageApp_Photo_WiFiLinkStart(PHOTO_STRM_INFO *p_strm_info);

extern ER ImageApp_Photo_WiFiLinkStop(PHOTO_STRM_ID strm_id);

extern UINT32 ImageApp_Photo_UpdateImgLinkForStrm(PHOTO_STRM_ID strm_id, UINT32 action, BOOL allow_pull);

extern HD_RESULT ImageApp_Photo_WifiPullOut(PHOTO_STRM_INFO *p_strm_info, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);

extern HD_RESULT ImageApp_Photo_WifiPushIn(PHOTO_STRM_INFO *p_strm_info, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms);

extern HD_RESULT ImageApp_Photo_WifiReleaseOut(PHOTO_STRM_INFO *p_strm_info, HD_VIDEO_FRAME* p_video_frame);

extern UINT32 ImageApp_Photo_WiFi_IsPreviewShow(void);

extern void ImageApp_Photo_WiFi_SetPreviewShow(UINT32 IsShow);

extern PHOTO_CAP_INFO* ImageApp_Photo_GetCapConfig(UINT32 idx);

extern void ImageApp_Photo_CapConfig(UINT32 config_id, UINT32 value);

extern INT32 ImageApp_Photo_SetFormatFree(BOOL is_on);

extern INT32 ImageApp_Photo_IsFormatFree(void);

extern void ImageApp_Photo_Get_Hdal_Path(PHOTO_VID_IN VID_IN, PHOTO_HDAL_PATH hd_path, UINT32 *pValue);

extern HD_PATH_ID ImageApp_Photo_GetVdoEncPort(UINT32 id);
//@}
#endif//IA_PHOTO_H

