#ifndef IMAGEAPP_USBMOVIE_H
#define IMAGEAPP_USBMOVIE_H

#include "hdal.h"
#include "kflow_videoenc/media_def.h"
#include <kwrap/type.h>
#include "ImageApp/ImageApp_Common.h"
#include "UVAC.h"

/// ========== Definition area ==========
/**
     @name Image ratio.

     @note for ImageUnit_CfgAspect()
*/
//@{
#define NVT_USBMOVIE_CROPRATIO_W_16     16
#define NVT_USBMOVIE_CROPRATIO_W_4      4
#define NVT_USBMOVIE_CROPRATIO_H_9      9
#define NVT_USBMOVIE_CROPRATIO_H_3      3

/**
     @name Tagret bitrate for MJPEG. Unit is Byte/Sec.

     @note for NVT_USBMOVIE_CFG_TBR_MJPG
*/
//@{
#define NVT_USBMOVIE_TBR_MJPG_LOW       0x00200000  ///< 2M
#define NVT_USBMOVIE_TBR_MJPG_HIGH      0x01000000  ///< 16M
#define NVT_USBMOVIE_TBR_MJPG_DEFAULT   0x00800000  ///< 8M
//@}

/**
     @name Tagret bitrate for H264. Unit is Byte/Sec.

     @note for NVT_USBMOVIE_CFG_TBR_H264
*/
//@{
#define NVT_USBMOVIE_TBR_H264_LOW       0x00100000  ///< 1M
#define NVT_USBMOVIE_TBR_H264_HIGH      0x00800000  ///< 8M
#define NVT_USBMOVIE_TBR_H264_DEFAULT   0x00200000  ///< 2M
//@}

/**
     @name Usb movie mode.

     @note for NVT_USBMOVIE_CFG_MODE
*/
//@{
#define NVT_USBMOVIE_MODE_CAM             0    ///< like a PC cam
//#define NVT_USBMOVIE_MODE_MULTIMEDIA      1    ///< for multimedia system
//@}

/**
     @name Usb media format.

     Uvc payload format.

     @note for NVT_USBMOVIE_CFG_MEDIA_FMT
*/
//@{
#define NVT_USBMOVIE_MEDIA_MJPEG            0    ///< MJPEG
#define NVT_USBMOVIE_MEDIA_H264             1    ///< H264
//@}

//UVC Extension Unit Control Selectors
#define EU_CONTROL_ID_01                    0x01
#define EU_CONTROL_ID_02                    0x02
#define EU_CONTROL_ID_03                    0x03
#define EU_CONTROL_ID_04                    0x04
#define EU_CONTROL_ID_05                    0x05
#define EU_CONTROL_ID_06                    0x06
#define EU_CONTROL_ID_07                    0x07
#define EU_CONTROL_ID_08                    0x08
#define EU_CONTROL_ID_09                    0x09
#define EU_CONTROL_ID_10                    0x0A
#define EU_CONTROL_ID_11                    0x0B
#define EU_CONTROL_ID_12                    0x0C
#define EU_CONTROL_ID_13                    0x0D
#define EU_CONTROL_ID_14                    0x0E
#define EU_CONTROL_ID_15                    0x0F
#define EU_CONTROL_ID_16                    0x10

#define SUPPORT_GET_REQUEST                 0x01
#define SUPPORT_SET_REQUEST                 0x02

/// ========== Enum area ==========
enum {
	USBMOVIE_CFG_MEM_POOL_INFO  = 0xF000E000,
	USBMOVIE_CFG_SENSOR_INFO,
	USBMOVIE_CFG_CHANNEL,               ///< UVC channel number
	USBMOVIE_CFG_TBR_MJPG,              ///< tagret bitrate for MJPEG. Unit is Byte/Sec.
	USBMOVIE_CFG_TBR_H264,              ///< tagret bitrate for H264. Unit is Byte/Sec.
	USBMOVIE_CFG_SET_MOVIE_SIZE_CB,     ///< the callback is invoked while UVC is started by USB host
	USBMOVIE_CFG_MODE,                  ///< for PC cam mode or multimedia mode
	USBMOVIE_CFG_MEDIA_FMT,             ///< payload data is MJPEG or H264, for multimedia mode only
	USBMOVIE_CFG_VDOIN_INFO,
	USBMOVIE_CFG_SENSOR_MAPPING,
	USBMOVIE_CFG_DRAM,                  /// <UVAC_DRAM_CFG *> structure pointer (Set only)
};

/// ========== Data structure area ==========
typedef struct _NVT_USBMOVIE_IPL_FUNC {
	UINT32 vcap_func;
	UINT32 vproc_func;
} NVT_USBMOVIE_IPL_FUNC, *PNVT_USBMOVIE_IPL_FUNC;

typedef struct _NVT_USBMOVIE_VID_RESO {
	UVAC_VID_DEV_CNT dev_id;
	UINT32 sen_id;
	UINT32 ResoIdx;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	UVAC_VIDEO_FORMAT codec;
	UINT32 tbr;

} NVT_USBMOVIE_VID_RESO, *PNVT_USBMOVIE_VID_RESO;

typedef struct {
	UINT32                 rec_id;
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
} USBMOVIE_SENSOR_INFO;

// structure for UVAC_PARAM_IPL_USER_IMG_SIZE
typedef struct {
	USIZE size;
	UINT32 fps;
} UVAC_IPL_SIZE_INFO;

// structure for UVAC_PARAM_IMGCAP_JPG_QUALITY
typedef struct {
	UINT32 jpg_quality;                 // 1~100
	UINT32 buf_size;                    // unit: byte, real allocate buffer size will be buf_size * 1.5
} UVAC_JPG_QUALITY;

typedef struct {
	UINT32 working_buf;
	UINT32 video_cap[2];
	UINT32 video_proc[2];
	UINT32 video_encode;
} UVAC_DRAM_CFG;

/// ========== Callback function area ==========
/**
    @name Callback prototype for NVT_USBMOVIE_CFG_SET_MOVIE_SIZE_CB

    @param[in] pVidReso1 pointer to video resolution of UVC channel 1 , refering to #_NVT_USBMOVIE_VID_RESO
    @param[in] pVidReso2 pointer to video resolution of UVC channel 2 , refering to #_NVT_USBMOVIE_VID_RESO

    @return void
*/
//@{
typedef void (*IMAGAPP_UVAC_SET_VIDRESO_CB)(PNVT_USBMOVIE_VID_RESO pVidReso1);
//@}

// =================== Parameters ===================
enum {
	UVAC_PARAM_START = 0x00003000,
	UVAC_PARAM_VCAP_FUNC,
	UVAC_PARAM_VPROC_FUNC,
	UVAC_PARAM_UVAC_INFO,           ///< Information needed for UVAC module.
	UVAC_PARAM_UVAC_VEND_DEV_DESC,  ///< Device descriptor, refering to #_UVAC_VEND_DEV_DESC.
	UVAC_PARAM_VID_RESO_ARY,        ///< Video resolution array, refering to #_UVAC_VID_RESO_ARY.
	UVAC_PARAM_AUD_SAMPLERATE_ARY,  ///< Audio sample array, refering to #_UVAC_AUD_SAMPLERATE_ARY_.
	UVAC_PARAM_UVAC_TBR_MJPG,       ///< Motion JPEG stream target size(Byte/Sec). Default value is 0x300000.
	UVAC_PARAM_UVAC_TBR_H264,       ///< H.264 stream target size(Byte/Sec). Default value is 0x100000.
	UVAC_PARAM_BS_START,            ///< Resume  sending stream to USB host.
	UVAC_PARAM_BS_STOP,             ///< Suspend sending stream to USB host.
	UVAC_PARAM_UVAC_CAP_M3,         ///< Enable UVC capture method 3.
	UVAC_PARAM_EU_VENDCMDCB_ID01,   ///> Extension unit callback with CS=1.
	UVAC_PARAM_EU_VENDCMDCB_ID02,   ///> Extension unit callback with CS=2.
	UVAC_PARAM_EU_VENDCMDCB_ID03,   ///> Extension unit callback with CS=3.
	UVAC_PARAM_EU_VENDCMDCB_ID04,   ///> Extension unit callback with CS=4.
	UVAC_PARAM_EU_VENDCMDCB_ID05,   ///> Extension unit callback with CS=5.
	UVAC_PARAM_EU_VENDCMDCB_ID06,   ///> Extension unit callback with CS=6.
	UVAC_PARAM_EU_VENDCMDCB_ID07,   ///> Extension unit callback with CS=7.
	UVAC_PARAM_EU_VENDCMDCB_ID08,   ///> Extension unit callback with CS=8.
	UVAC_PARAM_WINUSB_ENABLE,       ///< Enable WINUSB.
	UVAC_PARAM_WINUSB_CB,           ///< The callback function for WINUSB
	UVAC_PARAM_BULK_DATA,           ///< Obsolete
	UVAC_PARAM_VID_FMT_TYPE,        ///< Supported video format by UVC device.
	UVAC_PARAM_CDC_ENABLE,          ///< Enable CDC ADM.
	UVAC_PARAM_CDC_PSTN_REQUEST_CB, ///< The callback function for CDC PSTN subclass specific request
	UVAC_PARAM_AUD_FMT_TYPE,        ///< Audio format, refering to #_UVAC_AUD_FMT_TYPE
	UVAC_PARAM_MAX_FRAME_SIZE,      ///< Set higher frame size in MJPEG format.
	UVAC_PARAM_SUSPEND_CLOSE_IMM,	///< Suspend close unit.
	UVAC_PARAM_EU_CTRL,				///< UVC extension unit capability.
	UVAC_PARAM_VCAP_OUTFUNC,
	UVAC_PARAM_UVC2_MJPG_FRM_INFO,
	UVAC_PARAM_UVC2_H264_FRM_INFO,
	UVAC_PARAM_VCAP_PAT_GEN,
	UVAC_PARAM_IPL_MIRROR,
	UVAC_PARAM_IPL_USER_IMG_SIZE,
	UVAC_PARAM_VCAP_OUT_SIZE,
	UVAC_PARAM_IMG_JPG_QUALITY,
	UVAC_PARAM_SIE_REMAP,
	UVAC_PARAM_CT_CONTROLS,
	UVAC_PARAM_PU_CONTROLS,
	UVAC_PARAM_CT_CB,
	UVAC_PARAM_PU_CB,
	UVAC_PARAM_UVAC_TBR_MAX,        ///< Max TBR (Byte/Sec)
	UVAC_PARAM_ENCBUF_MS,           ///< venc buffer time in unit ms
};

/// ========== Function Prototype area ==========
/// ImageApp_UsbMovie
extern ER ImageApp_UsbMovie_Open(void);
extern ER ImageApp_UsbMovie_Close(void);
extern ER ImageApp_UsbMovie_Config(UINT32 config_id, UINT32 value);
/// ImageApp_UsbMovie_Param
extern ER ImageApp_UsbMovie_SetParam(UINT32 id, UINT32 param, UINT32 value);
extern ER ImageApp_UsbMovie_GetParam(UINT32 id, UINT32 param, UINT32* value);

#endif//IMAGEAPP_USBMOVIE_H
