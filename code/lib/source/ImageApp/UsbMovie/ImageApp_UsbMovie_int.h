#ifndef IMAGEAPP_USBMOVIE_INT_H
#define IMAGEAPP_USBMOVIE_INT_H

#include "ImageApp/ImageApp_UsbMovie.h"
#include "../Common/ImageApp_Common_int.h"
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include "UVAC.h"
#include "vendor_videoenc.h"

/// ========== Debug macro area ==========
#define ImageApp_UsbMovie_DUMP(fmtstr, args...) do { if(g_ia_usbmovie_trace_on) DBG_DUMP(fmtstr, ##args); } while(0)

/// ========== Max path definition area ==========
#define MAX_USBIMG_PATH             2
#define MAX_USBAUD_PATH             1

/// ========== Utilities definition area ==========
//#define max(a,b) (((a) > (b)) ? (a) : (b))
//#define min(a,b) (((a) < (b)) ? (a) : (b))
//#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)

/// ========== Definition area ==========
#define UPDATE_FORWARD              0
#define UPDATE_REVERSE              1

/// ========== data structures area ==========
typedef struct {
	UINT32 aud_rate;
	UINT32 aud_ch;
	UINT32 aud_ch_num;
} USBMOVIE_AUDIO_INFO;

/// ========== Link data structures area ==========
/// ImgLink
typedef struct {
	// video capture
	HD_VIDEOCAP_SYSCAPS  vcap_syscaps;
	HD_CTRL_ID           vcap_ctrl;
	HD_PATH_ID           vcap_p_ctrl;
	HD_IN_ID             vcap_i[1];
	HD_OUT_ID            vcap_o[1];
	HD_PATH_ID           vcap_p[1];
	// video process
	HD_VIDEOPROC_SYSCAPS vproc_syscaps;
	HD_CTRL_ID           vproc_ctrl;
	HD_PATH_ID           vproc_p_ctrl;
	HD_IN_ID             vproc_i[1];
	HD_OUT_ID            vproc_o_phy[2];
	HD_PATH_ID           vproc_p_phy[2];
	HD_OUT_ID            vproc_o[1];
	HD_PATH_ID           vproc_p[1];
	// video encode
	HD_IN_ID             venc_i[1];
	HD_OUT_ID            venc_o[1];
	HD_PATH_ID           venc_p[1];
} USBMOVIE_IMAGE_LINK;

typedef struct {
	// video capture
	UINT32               vcap_p[1];
	// video process
	UINT32               vproc_p_phy[2];
	UINT32               vproc_p[1];
	// video encode
	UINT32               venc_p[1];
} USBMOVIE_IMAGE_LINK_STATUS;

/// AudCapLink
typedef struct {
	// audiocap
	HD_PATH_ID           acap_p_ctrl;
	HD_CTRL_ID           acap_ctrl;
	HD_IN_ID             acap_i[1];
	HD_OUT_ID            acap_o[1];
	HD_PATH_ID           acap_p[1];
} USBMOVIE_AUDCAP_LINK;

typedef struct {
	// audiocap
	UINT32               acap_p[1];
} USBMOVIE_AUDCAP_LINK_STATUS;

/// ========== Flag id definition area ==========
#define FLGIAUSBMOVIE_VE_M0         0x00000001
#define FLGIAUSBMOVIE_VE_C0         0x00000002
#define FLGIAUSBMOVIE_VE_M1         0x00000004
#define FLGIAUSBMOVIE_VE_C1         0x00000008
#define FLGIAUSBMOVIE_VE_MASK       0x0000000F      /// All VE

#define FLGIAUSBMOVIE_AC_PCM        0x00000800      /// PCM encode
#define FLGIAUSBMOVIE_AC_MASK       0x00000800      /// All ACap

#define FLGIAUSBMOVIE_TSK_AC_IDLE   0x10000000      /// _ImageApp_UsbMovie_ACapPullTsk not pull out buffer
#define FLGIAUSBMOVIE_TSK_VE_IDLE   0x40000000      /// _ImageApp_UsbMovie_VEncPullTsk not pull out buffer
#define FLGIAUSBMOVIE_FLOW_LINK_CFG 0x80000000      /// All link is eatablished

#define FLGIAUSBMOVIE_MASK          FLGPTN_BIT_ALL

/// ========== Internal cross file variable area ==========
/// ImageApp_UsbMovie
extern BOOL g_ia_usbmovie_trace_on;
extern UVAC_INFO gUvacInfo;
extern UVAC_VEND_DEV_DESC *gpUvacVendDevDesc;
extern IMAGAPP_UVAC_SET_VIDRESO_CB gUvacSetVidResoCB;
extern UINT32 MaxUsbImgLink;
extern UVAC_DRAM_CFG usbimg_dram_cfg;
extern UINT32 gUvacMaxFrameSize;
/// ImageApp_UsbMovie_AudCapLink
extern USBMOVIE_AUDIO_INFO g_UsbAudioInfo;
extern USBMOVIE_AUDCAP_LINK UsbAudCapLink[MAX_USBAUD_PATH];
extern USBMOVIE_AUDCAP_LINK_STATUS UsbAudCapLinkStatus[MAX_USBAUD_PATH];
extern HD_AUDIOCAP_DEV_CONFIG usbaud_acap_dev_cfg[MAX_USBAUD_PATH];
extern HD_AUDIOCAP_DRV_CONFIG usbaud_acap_drv_cfg[MAX_USBAUD_PATH];
extern HD_AUDIOCAP_IN usbaud_acap_param[MAX_USBAUD_PATH];
extern HD_AUDIOCAP_BUFINFO usbaud_acap_bs_buf[MAX_USBAUD_PATH];
extern UINT32 usbaud_acap_bs_buf_va[MAX_USBAUD_PATH];
extern UINT32 usbaud_cap_volume;
/// ImageApp_UsbMovie_id
extern ID IAUSBMOVIE_FLG_ID;
/// ImageApp_UsbMovie_ImgLink
extern UINT32 IsUsbImgLinkOpened[MAX_USBIMG_PATH];
extern USBMOVIE_IMAGE_LINK UsbImgLink[MAX_USBIMG_PATH];
extern USBMOVIE_IMAGE_LINK_STATUS UsbImgLinkStatus[MAX_USBIMG_PATH];
extern USIZE usbimg_vcap_out_size[MAX_USBIMG_PATH];
#if defined(_BSP_NA51089_)
extern UINT32 usbimg_vcap_sie_remap[MAX_USBIMG_PATH];
#endif
extern UVAC_IPL_SIZE_INFO UsbImgIPLUserSize[MAX_USBIMG_PATH];
extern HD_DIM usbimg_max_imgsize[MAX_USBIMG_PATH];
extern HD_VIDEOCAP_CTRL usbimg_vcap_ctrl[MAX_USBIMG_PATH];
extern HD_VIDEOCAP_DRV_CONFIG usbimg_vcap_cfg[MAX_USBIMG_PATH];
extern HD_VIDEOCAP_IN usbimg_vcap_in_param[MAX_USBIMG_PATH];
extern HD_VIDEOCAP_CROP usbimg_vcap_crop_param[MAX_USBIMG_PATH];
extern HD_VIDEOCAP_OUT usbimg_vcap_out_param[MAX_USBIMG_PATH];
extern HD_VIDEOCAP_FUNC_CONFIG usbimg_vcap_func_cfg[MAX_USBIMG_PATH];
extern HD_VIDEO_PXLFMT usbimg_vcap_senout_pxlfmt[MAX_USBIMG_PATH];
extern HD_VIDEO_PXLFMT usbimg_vcap_capout_pxlfmt[MAX_USBIMG_PATH];
extern UINT32 usbimg_vcap_data_lane[MAX_USBIMG_PATH];
extern UINT32 usbimg_vcap_func[MAX_USBIMG_PATH];
extern UINT32 usbimg_vcap_patgen[MAX_USBIMG_PATH];
extern HD_VIDEOPROC_DEV_CONFIG usbimg_vproc_cfg[MAX_USBIMG_PATH];
extern HD_VIDEOPROC_CTRL usbimg_vproc_ctrl[MAX_USBIMG_PATH];
extern HD_VIDEOPROC_FUNC_CONFIG usbimg_vproc_func_cfg[MAX_USBIMG_PATH];
extern HD_VIDEOPROC_IN usbimg_vproc_in_param[MAX_USBIMG_PATH];
extern HD_VIDEOPROC_OUT usbimg_vproc_param[MAX_USBIMG_PATH][2];
extern HD_VIDEOPROC_OUT_EX usbimg_vproc_param_ex[MAX_USBIMG_PATH][1];
extern UINT32 usbimg_vproc_func[MAX_USBIMG_PATH];
extern HD_VIDEOENC_PATH_CONFIG usbimg_venc_path_cfg[MAX_USBIMG_PATH][1];
extern HD_VIDEOENC_IN  usbimg_venc_in_param[MAX_USBIMG_PATH][1];
extern HD_VIDEOENC_OUT2 usbimg_venc_out_param[MAX_USBIMG_PATH][1];
extern HD_H26XENC_RATE_CONTROL2 usbimg_venc_rc_param[MAX_USBIMG_PATH][1];
extern HD_VIDEOENC_BUFINFO usbimg_venc_bs_buf[MAX_USBIMG_PATH][1];
extern UINT32 usbimg_venc_bs_buf_va[MAX_USBIMG_PATH][1];
extern UINT32 g_UsbMovie_TBR_MJPG;
extern UINT32 g_UsbMovie_TBR_H264;
extern UINT32 g_UsbMovie_TBR_Max[MAX_USBIMG_PATH][1];
extern UINT32 usbimg_venc_buf_ms[MAX_USBIMG_PATH][1];
extern UVAC_JPG_QUALITY usbimg_jpg_quality;
// Important notice!!! If enlarge MAX_USBIMG_PATH, following variable shoule also update initial value too !!!
extern UINT32 usbimg_sensor_mapping[MAX_USBIMG_PATH];


/// ========== Internal function area ==========
/// ImageApp_UsbMovie
extern ER _ImageApp_UsbMovie_LinkCfg(void);
extern UINT32 _ImageApp_UsbMovie_frc(UINT32 dst_fr, UINT32 src_fr);
extern ER _ImageApp_UsbMovie_ImgLinkOpen(UINT32 id);
extern ER _ImageApp_UsbMovie_ImgLinkClose(UINT32 id);
/// ImageApp_UsbMovie_AudCapLink
extern UINT32 IsUsbAudCapLinkOpened[MAX_USBAUD_PATH];
extern ER _ImageApp_UsbMovie_AudCapLinkCfg(UINT32 id);
extern ER _ImageApp_UsbMovie_AudCapLinkStatusUpdate(UINT32 id, UINT32 direction);
extern ER _ImageApp_UsbMovie_AudCapLinkOpen(UINT32 id);
extern ER _ImageApp_UsbMovie_AudCapLinkClose(UINT32 id);
extern ER _ImageApp_UsbMovie_AudCapStart(UINT32 id);
extern ER _ImageApp_UsbMovie_AudCapStop(UINT32 id);
/// ImageApp_UsbMovie_Cb
extern UINT32 _ImageApp_UsbMovie_StartVideoCB(UVAC_VID_DEV_CNT vidDevIdx, UVAC_STRM_INFO *pStrmInfo);
extern void _ImageApp_UsbMovie_StopVideoCB(UVAC_VID_DEV_CNT vidDevIdx);
extern UINT32 _ImageApp_UsbMovie_SetVolumeCB(UINT32 volume);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
extern void _ImageApp_UsbMovie_memcpy(UINT32 uiDst, UINT32 uiSrc, UINT32 uiSize);
#endif
/// ImageApp_UsbMovie_id
extern ER _ImageApp_UsbMovie_InstallID(void);
extern ER _ImageApp_UsbMovie_UninstallID(void);
/// ImageApp_UsbMovie_ImgLink
extern ER _ImageApp_UsbMovie_ImgLinkCfg(UINT32 id);
extern ER _ImageApp_UsbMovie_ImgLinkStatusUpdate(UINT32 id, UINT32 direction);
extern ER _ImageApp_UsbMovie_ImgLinkOpen(UINT32 id);
extern ER _ImageApp_UsbMovie_ImgLinkClose(UINT32 id);
/// ImageApp_UsbMovie_Operation
extern ER _ImageApp_UsbMovie_UvacStart(UINT32 id, UINT32 path);
extern ER _ImageApp_UsbMovie_UvacStop(UINT32 id, UINT32 path);
/// ImageApp_UsbMovie_Param
extern ER _ImageApp_UsbMovie_SetupRecParam(UINT32 id);
extern ER _ImageApp_UsbMovie_SetupAudioParam(void);

#endif//IMAGEAPP_USBMOVIE_INT_H
