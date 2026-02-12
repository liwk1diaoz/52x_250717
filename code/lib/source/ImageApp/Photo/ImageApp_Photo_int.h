#ifndef IA_PHOTO_INT_H
#define IA_PHOTO_INT_H
#include "ImageApp/ImageApp_Photo.h"


#define IME_3DNR_PATH    0
#define IME_DISPLAY_PATH    1
#define IME_STREAMING_PATH  2
#define IME_3DNR_REF_PATH   4

//#define VPRC_CAP_3DNR_PATH    0 //cap not support 3DNR
#define VPRC_CAP_MAIN_PATH     0
#define VPRC_CAP_SCR_PATH    1

#define VPRC_CAP_QV_THUMB_PATH  2  // 3 ,vprc path 3 not support yuv420, ERR:_vdoprc_check_out_fmt() -out3:fmt=520c0420 is not supported with pipe=255!

#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)

extern PHOTO_DISP_INFO  photo_disp_info[PHOTO_DISP_ID_MAX];
extern PHOTO_STRM_INFO  photo_strm_info[(PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN)];
extern PHOTO_IME3DNR_INFO  photo_ime3dnr_info[(PHOTO_IME3DNR_ID_MAX-PHOTO_IME3DNR_ID_MIN)];
extern PHOTO_IPL_MIRROR  photo_ipl_mirror[PHOTO_VID_IN_MAX];
extern PHOTO_DUAL_DISP g_photo_dual_disp;
extern PHOTO_VDO_OUT_INFO g_isf_vout_info[PHOTO_VID_OUT_MAX];
extern UINT32 g_photo_filedb_filter;
extern UINT32 g_photo_filedb_max_num;
extern PHOTO_SENSOR_INFO photo_vcap_sensor_info[PHOTO_VID_IN_MAX];
extern PHOTO_CAP_INFO photo_cap_info[PHOTO_VID_IN_MAX];
extern PHOTO_DISP_INFO g_PhotoDispLink[PHOTO_DISP_ID_MAX];
extern HD_VIDEOENC_IN  photo_cap_venc_in_param;
extern HD_VIDEOENC_OUT2 photo_cap_venc_out_param;
extern HD_VIDEOCAP_SYSCAPS  photo_vcap_syscaps[PHOTO_VID_IN_MAX];
extern HD_URECT photo_vcap_out_crop_rect;

extern PHOTO_CAP_CBMSG_FP  g_PhotoCapMsgCb;
extern UINT32 g_photo_is_disp_qv_restart_preview;
extern PHOTO_IME_CROP_INFO photo_disp_imecrop_info[PHOTO_VID_IN_MAX];
extern UINT32 photo_vcap_patgen[PHOTO_VID_IN_MAX];
extern PHOTO_VCAP_OUTFUNC g_photo_vcap_func_cfg[PHOTO_VID_IN_MAX];
extern HD_VIDEOCAP_IN g_prv_vcap_in_param[PHOTO_VID_IN_MAX];
extern PHOTO_VCAP_SENSOR_MODE_CFG g_photo_cap_sensor_mode_cfg[PHOTO_VID_IN_MAX];
extern HD_VIDEOCAP_CROP g_prv_vcap_cropout_param[PHOTO_VID_IN_MAX];
extern PhotoVdoBsCb  *photo_vdo_bs_cb[PHOTO_VID_IN_MAX];

//extern UINT32 _SECTION(".kercfg_data") PHOTO_DISP_TSK_ID;
//extern UINT32 _SECTION(".kercfg_data") PHOTO_DISP_FLG_ID;
//extern UINT32 _SECTION(".kercfg_data") PHOTO_WIFI_TSK_ID;
//extern UINT32 _SECTION(".kercfg_data") PHOTO_WIFI_FLG_ID;
extern UINT32 PHOTO_DISP_TSK_ID;
extern UINT32 PHOTO_DISP_FLG_ID;
extern UINT32 PHOTO_WIFI_TSK_ID;
extern UINT32 PHOTO_WIFI_FLG_ID;
extern UINT32 g_photo_3ndr_path;
extern UINT32 NormalJpgQueueF; //feed
extern UINT32 NormalJpgQueueR; //release

#define NORMAL_JPG_QUEUE_RESET() \
	{ \
		NormalJpgQueueF = 0; \
		NormalJpgQueueR = 0; \
	}

#define NORMAL_JPG_QUEUE_IN() \
	{ \
		NormalJpgQueueF += 1; \
	}

#define NORMAL_JPG_QUEUE_F() (NormalJpgQueueF)

#define NORMAL_JPG_QUEUE_OUT() \
	{ \
		NormalJpgQueueR += 1; \
	} \

#define NORMAL_JPG_QUEUE_R() (NormalJpgQueueR)

#define NORMAL_JPG_SET_QUEUE_R(Idx) \
	{ \
		NormalJpgQueueR = Idx; \
	}

#define NORMAL_JPG_QUEUE_NUM() (NormalJpgQueueF - NormalJpgQueueR)


typedef struct {
	UINT32 Addr;         ///< buffer address
	UINT32 Size;         ///< buffer size
	UINT32 SensorID;     ///< This image is belong to which sensor ID
} IMGCAP_FILE_INFOR;

extern ER _ImageApp_Photo_DispLinkOpen(PHOTO_DISP_INFO *p_disp_info);

extern ER _ImageApp_Photo_DispLinkClose(PHOTO_DISP_INFO *p_disp_info);

extern void _ImageApp_Photo_DispSetAspectRatio(UINT32 Path, UINT32 w, UINT32 h);

extern void _ImageApp_Photo_DispSetVdoImgSize(UINT32 Path, UINT32 w, UINT32 h);

extern void _ImageApp_Photo_DispResetPath(void);

extern ER _ImageApp_Photo_DispLinkStart(PHOTO_DISP_INFO *p_disp_info);

extern ER _ImageApp_Photo_WiFiLinkOpen(PHOTO_STRM_INFO *p_strm_info);

extern ER _ImageApp_Photo_WiFiLinkClose(PHOTO_STRM_INFO *p_strm_info);

extern void _ImageApp_Photo_WiFiSetVdoImgSize(UINT32 Path, UINT32 w, UINT32 h);

extern void _ImageApp_Photo_WiFiSetAspectRatio(UINT32 Path, UINT32 w, UINT32 h);

extern void _ImageApp_Photo_InstallCmd(void);

extern void _ImageApp_Photo_TriggerCap(void);

extern ER _ImageApp_Photo_CapLinkOpen(PHOTO_CAP_INFO *p_cap_info);

extern ER _ImageApp_Photo_CapLinkClose(PHOTO_CAP_INFO *p_cap_info);

extern ER _ImageApp_Photo_CapLinkStart(PHOTO_CAP_INFO *p_cap_info);

extern HD_RESULT _ImageApp_Photo_venc_set_param(HD_PATH_ID venc_path, HD_VIDEOENC_IN *pin_param, HD_VIDEOENC_OUT2 *pout_param);
extern HD_URECT _ImageApp_Photo_vcap_set_crop_out(UINT32 img_ratio, ISIZE sensor_info_size);

#endif//IA_PHOTO_INT_H
