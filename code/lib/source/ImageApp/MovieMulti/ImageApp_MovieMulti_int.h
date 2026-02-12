#ifndef IMAGEAPP_MOVIEMULTI_INT_H
#define IMAGEAPP_MOVIEMULTI_INT_H

#include "ImageApp/ImageApp_MovieMulti.h"
#include "../Common/ImageApp_Common_int.h"
#include "avfile/movieinterface_def.h"
#include <kwrap/cmdsys.h>
#include <kwrap/sxcmd.h>
#include <kwrap/stdio.h>
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/util.h>
#include <kwrap/perf.h>
#include <kwrap/cpu.h>
#include <kwrap/verinfo.h>
#include <string.h>
#include "hd_bsmux_lib.h"
#include "hd_fileout_lib.h"
#include "vendor_audiocapture.h"
#include "vendor_audioenc.h"
#include "vendor_common.h"
#include "vendor_isp.h"
#include "vendor_videodec.h"
#include "vendor_videoenc.h"
#include "vendor_videoprocess.h"
#include <vf_gfx.h>
#include "SizeConvert.h"
#include "exif/Exif.h"
#include "GxImageFile.h"
#include "Utility/Color.h"
#include "ImageApp_MovieMulti_FileManage.h"
#include "UVAC.h"
#if defined(__FREERTOS)
#include "crypto.h"
#else  // defined(__FREERTOS)
#include <crypto/cryptodev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#endif // defined(__FREERTOS)

/// ========== Branch definition area ==========
#define IAMOVIE_BR_1_25                         125
#define IAMOVIE_BR_1_40                         140
#define IAMOVIE_BRANCH                          IAMOVIE_BR_1_40

/// ========== Debug macro area ==========
#define IAMOVIE_COMM_CB_TIMEOUT_CHECK           1000    // unit: ms
#define IAMOVIE_BS_CB_TIMEOUT_CHECK             100     // unit: ms
#define IAMOVIE_USER_CB_TIMEOUT_CHECK           100     // unit: ms
#define IAMOVIE_TIMESTAMP_WARNING               ENABLE
#define IAMOVIE_TIMESTAMP_WARNING_TIME          6000000 // unit: us
#define ImageApp_MovieMulti_DUMP(fmtstr, args...) do { if(g_ia_moviemulti_trace_on) DBG_DUMP(fmtstr, ##args); } while(0)

/// ========== Internal function configuation area ==========
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
#define _IAMOVIE_ONE_SEC_CB_USING_VENC          DISABLE        // Enable for using venc, else using bsmux
#else
#define _IAMOVIE_ONE_SEC_CB_USING_VENC          ENABLE         // Enable for using venc, else using bsmux
#endif

#include "FileDB.h"

#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
#include "vendor_type.h"
#endif

/// alsa function
#define USE_ALSA_LIB                            DISABLE
#define ALSA_DEBUG                              ENABLE

#if (USE_ALSA_LIB == ENABLE)
#include <errno.h>
#include <alsa/asoundlib.h>
#endif

#define BSMUX_USE_NEW_EMR_API                   ENABLE
#define BSMUX_USE_NEW_PUSH_METHOD               ENABLE

#define IAMOVIE_CRYPTO_OFFSET                   16
#define IAMOVIE_CRYPTO_SIZE                     256

#if defined(_BSP_NA51089_) || (IAMOVIE_BRANCH == IAMOVIE_BR_1_25)
#define IAMOVIE_SUPPORT_VPE                     DISABLE
#else
#define IAMOVIE_SUPPORT_VPE                     ENABLE
#endif

/// ========== Max path definition area ==========
#if defined(_BSP_NA51055_)
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
#if defined(__FREERTOS)
#define MAX_IMG_PATH                3           // na51055, 1.40, rtos (57x only)
#else
#define MAX_IMG_PATH                4           // na51055, 1.40, linux (57x & 58x)
#define MAX_IMG_PATH_NA51055        3           // 3 path max for 57x
#endif
#else
#define MAX_IMG_PATH                2           // na51055, 1.25 (customized branch), linux (57x & 58x)
#endif
#elif defined(_BSP_NA51089_)
#define MAX_IMG_PATH                3           // na51089, 1.00, rtos & linux (56x)
#elif defined(_BSP_NA51102_)
#define MAX_IMG_PATH                4           // na51102, 1.00, linux (53x)
#else
#define MAX_IMG_PATH                2           // undefined
#endif

#if defined(_BSP_NA51102_)
#define MAX_DISP_PATH               2
#else
#define MAX_DISP_PATH               1
#endif

#define MAX_WIFI_PATH               1
#define MAX_UVAC_PATH               MAX_WIFI_PATH
#define MAX_AUDCAP_PATH             1
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
#define MAX_ETHCAM_PATH             2
#else
#define MAX_ETHCAM_PATH             1
#endif
#define MAX_IMGCAP_PATH             1

#define MAX_BSMUX_PATH              12

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
#define VPROC_PER_LINK              3
#else
#define VPROC_PER_LINK              1
#endif

#define MAX_GPS_DATA_SIZE           2048
#define GPS_DATA_FRONT_RESERVED     16

#define IAMVOIE_DEFAULT_H264_PROFILE    HD_H264E_HIGH_PROFILE
#define IAMVOIE_DEFAULT_H264_LEVEL      HD_H264E_LEVEL_5_1
#define IAMVOIE_DEFAULT_H265_PROFILE    HD_H265E_MAIN_PROFILE
#define IAMVOIE_DEFAULT_H265_LEVEL      HD_H265E_LEVEL_5

/// ========== Common definition area ==========
#define MASK_EMR_MAIN              (_CFG_MAIN_EMR_BOTH  | _CFG_MAIN_EMR_PAUSE  | _CFG_MAIN_EMR_ONLY | _CFG_MAIN_EMR_LOOP)
#define MASK_EMR_CLONE             (_CFG_CLONE_EMR_BOTH | _CFG_CLONE_EMR_PAUSE | _CFG_CLONE_EMR_ONLY | _CFG_CLONE_EMR_LOOP)

enum {
	LINKTYPE_REC = 0,
	LINKTYPE_STRM,
	LINKTYPE_DISP,
	LINKTYPE_ETHCAM,
	LINKTYPE_ETHCAM_TX,
	LINKTYPE_UVAC,
	LINKTYPE_AUDCAP,
};

enum {
	IMGCAP_THUM      = MOVIE_IMGCAP_THUMB,
	IMGCAP_JSTM      = MOVIE_IMGCAP_RAWENC,
	IMGCAP_EXIF_THUM = MOVIE_IMGCAP_EXIF_THUMB,
};

enum {
	MOVIETYPE_MAIN = 0,
	MOVIETYPE_CLONE,
	MOVIETYPE_MAX,
};

typedef enum {
	IMGCAP_THUMB_W = 640,
	IMGCAP_THUMB_H = 480,
	IMGCAP_EXIF_W  = 160,
	IMGCAP_EXIF_H  = 120,
} IMGCAP_THUMB_SIZE;

typedef enum {
	CB_PATH_STATUS_INIT     = 0x0000,
	CB_PATH_STATUS_RECORD   = 0x0001,
	CB_PATH_STATUS_CUT      = 0x0002,
	CB_PATH_STATUS_CLOSE    = 0x0003,
} CB_PATH_STATUS;

#define AENC_BS_RESERVED_SIZE_MP4       0
#define AENC_BS_RESERVED_SIZE_TS        21
#define VENC_BS_RESERVED_SIZE_MP4       0
#define VENC_BS_RESERVED_SIZE_TS        21
#define VENC_BS_RESERVED_SIZE_JPG       65536

/// ========== Utilities definition area ==========
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define DBGINFO_BUFSIZE()	(0x200)
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)

/// ========== Definition area ==========
#define UPDATE_FORWARD              0
#define UPDATE_REVERSE              1

/// ========== Flag id definition area ==========
//IAMOVIE_FLG_ID
#define FLGIAMOVIE_VE_M0            0x00000001      /// ImgLink0 main
#define FLGIAMOVIE_VE_C0            0x00000002      /// ImgLink0 clone
#define FLGIAMOVIE_VE_M1            0x00000004      /// ImgLink1 main
#define FLGIAMOVIE_VE_C1            0x00000008      /// ImgLink1 clone
#define FLGIAMOVIE_VE_M2            0x00000010      /// ImgLink2 main
#define FLGIAMOVIE_VE_C2            0x00000020      /// ImgLink2 clone
#define FLGIAMOVIE_VE_M3            0x00000040      /// ImgLink3 main
#define FLGIAMOVIE_VE_C3            0x00000080      /// ImgLink3 clone
#define FLGIAMOVIE_VE_S0            0x00000100      /// WifiLink
#define FLGIAMOVIE_VE_E0            0x00000200      /// EthCamLink
#define FLGIAMOVIE_VE_E1            0x00000400      /// EthCamLink
#define FLGIAMOVIE_VE_MASK          0x004007FF      /// All VE

#define FLGIAMOVIE_AE_AAC           0x00001000      /// AAC encode
#define FLGIAMOVIE_AE_MASK          0x00201000      /// All AE

#define FLGIAMOVIE_AE_S0            0x00010000
#define FLGIAMOVIE_AC_U0            0x00020000

#define FLGIAMOVIE_AC_PCM           0x00008000      /// PCM CAP
#define FLGIAMOVIE_AC_MASK          0x00108000      /// All ACap

#define FLGIAMOVIE_AC_GET           0x01000000
#define FLGIAMOVIE_AC_GET_DONE      0x02000000

#define FLGIAMOVIE_AC_ESC           0x00100000
#define FLGIAMOVIE_AE_ESC           0x00200000
#define FLGIAMOVIE_VE_ESC           0x00400000

#define FLGIAMOVIE_TSK_AC_IDLE      0x10000000      /// _ImageApp_MovieMulti_ACapPullTsk not pull out buffer
#define FLGIAMOVIE_TSK_AE_IDLE      0x20000000      /// _ImageApp_MovieMulti_AEncPullTsk not pull out buffer
#define FLGIAMOVIE_TSK_VE_IDLE      0x40000000      /// _ImageApp_MovieMulti_VEncPullTsk not pull out buffer
#define FLGIAMOVIE_TSK_IDLE_MASK    0x70000000      /// All PullTsk not pull out buffer

#define FLGIAMOVIE_FLOW_LINK_CFG    0x80000000      /// All link is eatablished

#define FLGIAMOVIE_MASK             FLGPTN_BIT_ALL

//IAMOVIE_FLG_ID2
#define FLGIAMOVIE2_JPG_M0_RDY      0x00000001      /// ImgLink0 main rawenc
#define FLGIAMOVIE2_JPG_C0_RDY      0x00000002      /// ImgLink0 clone rawenc
#define FLGIAMOVIE2_JPG_M1_RDY      0x00000004      /// ImgLink1 main rawenc
#define FLGIAMOVIE2_JPG_C1_RDY      0x00000008      /// ImgLink1 clone rawenc
#define FLGIAMOVIE2_JPG_M2_RDY      0x00000010      /// ImgLink2 main rawenc
#define FLGIAMOVIE2_JPG_C2_RDY      0x00000020      /// ImgLink2 clone rawenc
#define FLGIAMOVIE2_JPG_M3_RDY      0x00000040      /// ImgLink3 main rawenc
#define FLGIAMOVIE2_JPG_C3_RDY      0x00000080      /// ImgLink3 clone rawenc
#define FLGIAMOVIE2_THM_M0_RDY      0x00000100      /// ImgLink0 main thumbnail
#define FLGIAMOVIE2_THM_C0_RDY      0x00000200      /// ImgLink0 clone thumbnail
#define FLGIAMOVIE2_THM_M1_RDY      0x00000400      /// ImgLink1 main thumbnail
#define FLGIAMOVIE2_THM_C1_RDY      0x00000800      /// ImgLink1 clone thumbnail
#define FLGIAMOVIE2_THM_M2_RDY      0x00001000      /// ImgLink2 main thumbnail
#define FLGIAMOVIE2_THM_C2_RDY      0x00002000      /// ImgLink2 clone thumbnail
#define FLGIAMOVIE2_THM_M3_RDY      0x00004000      /// ImgLink3 main thumbnail
#define FLGIAMOVIE2_THM_C3_RDY      0x00008000      /// ImgLink3 clone thumbnail
#define FLGIAMOVIE2_JPG_E0_RDY      0x00010000      /// EthCamLink0 main rawenc
#define FLGIAMOVIE2_JPG_E1_RDY      0x00020000      /// EthCamLink1 main rawenc
#define FLGIAMOVIE2_JPG_E2_RDY      0x00040000      /// EthCamLink2 main rawenc
#define FLGIAMOVIE2_JPG_E3_RDY      0x00080000      /// EthCamLink3 main rawenc
#define FLGIAMOVIE2_THM_E0_RDY      0x00100000      /// EthCamLink0 main thumbnail
#define FLGIAMOVIE2_THM_E1_RDY      0x00200000      /// EthCamLink1 main thumbnail
#define FLGIAMOVIE2_THM_E2_RDY      0x00400000      /// EthCamLink2 main thumbnail
#define FLGIAMOVIE2_THM_E3_RDY      0x00800000      /// EthCamLink3 main thumbnail

#define FLGIAMOVIE2_RAWENC_RDY      0x00FFFFFF      /// All rawenc path
#define FLGIAMOVIE2_RAWENC_FRM_WAIT 0x40000000

#define FLGIAMOVIE2_VE_IMGCAP_ESC   0x20000000
#define FLGIAMOVIE2_VE_IMGCAP       0x80000000      /// ImgCapLink

#define FLGIAMOVIE2_MASK            FLGPTN_BIT_ALL

//IAMOVIE_FLG_BG
#define FLGIAMOVIE_BG_VE_M0         0x00000001      /// ImgLink0 main
#define FLGIAMOVIE_BG_VE_C0         0x00000002      /// ImgLink0 clone
#define FLGIAMOVIE_BG_VE_M1         0x00000004      /// ImgLink1 main
#define FLGIAMOVIE_BG_VE_C1         0x00000008      /// ImgLink1 clone
#define FLGIAMOVIE_BG_VE_M2         0x00000010      /// ImgLink2 main
#define FLGIAMOVIE_BG_VE_C2         0x00000020      /// ImgLink2 clone
#define FLGIAMOVIE_BG_VE_M3         0x00000040      /// ImgLink3 main
#define FLGIAMOVIE_BG_VE_C3         0x00000080      /// ImgLink3 clone
#define FLGIAMOVIE_BG_VE_MASK       0x000000FF      /// All VE
#define FLGIAMOVIE_BG_ESC           0x80000000      /// Escape BG Task

#define FLGIAMOVIE_BG_MASK          FLGPTN_BIT_ALL

/// ========== Link data structures area ==========
// ImgLink -> WifiLink-> ImgCapLink -> AudCapLink -> EthCamLink
// ImgLink    : / vcap / vprc(ipp*1 + [vpe*2]) / venc /      / bsmux / fileout /
// WifiLink   :                                / venc /
// ImgCapLink :                                / venc /
// AudCapLink :                                                                / acap / aenc /
// EthCamLink :        / vprc([vpe*1])         / venc / vdec / bsmux / fileout /
/// ===== AudCapLink =====
typedef struct {
	// audiocap
	HD_PATH_ID           acap_p_ctrl;
	HD_CTRL_ID           acap_ctrl;
	HD_IN_ID             acap_i[1];
	HD_OUT_ID            acap_o[1];
	HD_PATH_ID           acap_p[1];
	// audioenc
	HD_IN_ID             aenc_i[1];
	HD_OUT_ID            aenc_o[1];             // aac
	HD_PATH_ID           aenc_p[1];
} MOVIE_AUDCAP_LINK;

typedef struct {
	// audiocap
	UINT32               acap_p[1];
	// audioenc
	UINT32               aenc_p[1];             // aac
} MOVIE_AUDCAP_LINK_STATUS;

/// ===== DispLink =====
typedef struct {
	// videoout
	HD_VIDEOOUT_SYSCAPS  vout_syscaps;
	HD_PATH_ID           vout_p_ctrl;
	// VdoOut flow refine: vout is opened in project
	#if 0
	HD_IN_ID             vout_i[1];
	HD_OUT_ID            vout_o[1];
	#endif
	HD_PATH_ID           vout_p[1];
	HD_DIM               vout_ratio;
	HD_URECT             vout_win;
	HD_VIDEO_DIR         vout_dir;
} MOVIE_DISP_LINK;

typedef struct {
	// videoout
	UINT32               vout_p[1];
} MOVIE_DISP_LINK_STATUS;

/// ===== EthCamLink =====
typedef struct {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	// video process
	HD_VIDEOPROC_SYSCAPS vproc_syscaps;
	HD_CTRL_ID           vproc_ctrl;
	HD_PATH_ID           vproc_p_ctrl;
	HD_IN_ID             vproc_i[1];
	HD_OUT_ID            vproc_o_phy[1];
	HD_PATH_ID           vproc_p_phy[1];
	HD_OUT_ID            vproc_o[1];
	HD_PATH_ID           vproc_p[1];
#endif
	// video encode
	HD_IN_ID             venc_i[1];
	HD_OUT_ID            venc_o[1];
	HD_PATH_ID           venc_p[1];
	// video decode
	HD_IN_ID             vdec_i[1];
	HD_OUT_ID            vdec_o[1];
	HD_PATH_ID           vdec_p[1];
	// bsmux
	HD_IN_ID             bsmux_i[2];
	HD_OUT_ID            bsmux_o[2];
	HD_PATH_ID           bsmux_p[2];
	// fileout
	HD_IN_ID             fileout_i[2];
	HD_OUT_ID            fileout_o[2];
	HD_PATH_ID           fileout_p[2];
} MOVIE_ETHCAM_LINK;

typedef struct {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
	// video process
	UINT32               vproc_p_phy[1];
	UINT32               vproc_p[1];
#endif
	// video encode
	UINT32               venc_p[1];
	// video decode
	UINT32               vdec_p[1];
	// bsmux
	UINT32               bsmux_p[2];
	// fileout
	UINT32               fileout_p[2];
} MOVIE_ETHCAM_LINK_STATUS;

/// ===== ImgCapLink =====
typedef struct {
	// video encode
	HD_IN_ID             venc_i[1];
	HD_OUT_ID            venc_o[1];
	HD_PATH_ID           venc_p[1];
} MOVIE_IMGCAP_LINK;

typedef struct {
	// video encode
	UINT32               venc_p[1];
} MOVIE_IMGCAP_LINK_STATUS;

/// ===== ImgLink =====
typedef struct {
	// video capture
	HD_VIDEOCAP_SYSCAPS  vcap_syscaps;
	HD_CTRL_ID           vcap_ctrl;
	HD_PATH_ID           vcap_p_ctrl;
	HD_IN_ID             vcap_i[1];
	HD_OUT_ID            vcap_o[1];
	HD_PATH_ID           vcap_p[1];
	// video process
	HD_VIDEOPROC_SYSCAPS vproc_syscaps[VPROC_PER_LINK];
	HD_CTRL_ID           vproc_ctrl[VPROC_PER_LINK];
	HD_PATH_ID           vproc_p_ctrl[VPROC_PER_LINK];
	HD_IN_ID             vproc_i[VPROC_PER_LINK][1];
	HD_OUT_ID            vproc_o_phy[VPROC_PER_LINK][5];
	HD_PATH_ID           vproc_p_phy[VPROC_PER_LINK][5];
	HD_OUT_ID            vproc_o[VPROC_PER_LINK][7];            // main / clone / disp / wifi / alg / disp2 / alg(y)
	HD_PATH_ID           vproc_p[VPROC_PER_LINK][7];
	// video encode
	HD_IN_ID             venc_i[2];
	HD_OUT_ID            venc_o[2];
	HD_PATH_ID           venc_p[2];
	// bsmux
	HD_IN_ID             bsmux_i[4];
	HD_OUT_ID            bsmux_o[4];
	HD_PATH_ID           bsmux_p[4];
	// fileout
	HD_IN_ID             fileout_i[4];
	HD_OUT_ID            fileout_o[4];
	HD_PATH_ID           fileout_p[4];
} MOVIE_IMAGE_LINK;

typedef struct {
	// video capture
	UINT32               vcap_p[1];
	// video process
	UINT32               vproc_p_phy[VPROC_PER_LINK][5];
	UINT32               vproc_p[VPROC_PER_LINK][7];            // main / clone / disp / wifi / alg / disp2 / alg(y)
	// video encode
	UINT32               venc_p[2];
	// bsmux
	UINT32               bsmux_p[4];
	// fileout
	UINT32               fileout_p[4];
} MOVIE_IMAGE_LINK_STATUS;

/// ===== WifiLink =====
typedef struct {
	// video encode
	HD_IN_ID             venc_i[1];
	HD_OUT_ID            venc_o[1];
	HD_PATH_ID           venc_p[1];
} MOVIE_WIFI_LINK;

typedef struct {
	// video encode
	UINT32               venc_p[1];
} MOVIE_WIFI_LINK_STATUS;

/// ========== Internal data structures area ==========
typedef struct {
	USIZE size;
	UINT32 fmt;
	UINT32 fps;
	USIZE ratio;
} MOVIE_IMG_IME_SETTING;

typedef struct {
	CHAR path_prev[132];
	UINT32 is_crash_curr;
	UINT32 is_crash_prev;
	UINT32 is_crash_next;
	UINT32 is_crash_next_old; //is_crash_next of the last time
} MOVIE_CB_CRASH_INFO;

typedef struct {
	CB_PATH_STATUS status;
	CHAR *p_fpath;
	UINT32 duration;
} MOVIE_CB_PATH_INFO;

typedef struct {
	UINT32 idx;
	UINT32 tbl;
	UINT32 link_type;
} MOVIE_TBL_IDX;

/// ========== ImaCap queue area ==========
#define IMGCAP_JOBQUEUENUM      8

typedef struct {
	QUEUE_BUFFER_ELEMENT q;             // should be the first element of this structure
	UINT32               type;
	UINT32               exif;
	HD_PATH_ID           path;
	UINT32               is_emr;        // for thumb only, to record normal or emr path
} IMGCAP_JOB_ELEMENT;

/// ========== Internal cross file variable area ==========
/// ImageApp_MovieMulti
extern UINT32 img_flow_for_dumped_fast_boot;
extern UINT32 IsMovieMultiOpened;
extern MOVIEMULTI_MAX_LINK_INFO MaxLinkInfo;
extern UINT32 iamovie_need_reopen;
extern MOVIEMULTI_DRAM_CFG dram_cfg;
extern UINT32 iamovie_use_filedb_and_naming;
// Important notice!!! If enlarge MAX_IMG_PATH, following variables shoule also update initial value too !!!
extern UINT32 img_vcap_capout_dir[MAX_IMG_PATH];
extern UINT32 img_sensor_mapping[MAX_IMG_PATH];
extern UINT32 img_bsmux_bufsec[MAX_IMG_PATH][4];
extern UINT32 ImgLinkVdoEncBufMs[MAX_IMG_PATH][2];
extern MOVIE_AD_MAP ad_sensor_map[MAX_IMG_PATH];
/// ImageApp_MovieMulti_AudCapLink
extern MOVIEMULTI_AUDIO_INFO g_AudioInfo;
extern UINT32 IsAudCapLinkOpened[MAX_AUDCAP_PATH];
extern MOVIE_AUDCAP_LINK AudCapLink[MAX_AUDCAP_PATH];
extern MOVIE_AUDCAP_LINK_STATUS AudCapLinkStatus[MAX_AUDCAP_PATH];
extern HD_AUDIOCAP_DEV_CONFIG aud_acap_dev_cfg[MAX_AUDCAP_PATH];
extern HD_AUDIOCAP_DRV_CONFIG aud_acap_drv_cfg[MAX_AUDCAP_PATH];
extern HD_AUDIOCAP_IN aud_acap_param[MAX_AUDCAP_PATH];
extern HD_AUDIOCAP_ANR aud_acap_anr_param[MAX_AUDCAP_PATH];
extern HD_AUDIOCAP_AEC aud_acap_aec_param[MAX_AUDCAP_PATH];
extern HD_AUDIOENC_PATH_CONFIG2 aud_aenc_cfg[MAX_AUDCAP_PATH];
extern HD_AUDIOENC_IN aud_aenc_in_param[MAX_AUDCAP_PATH];
extern HD_AUDIOENC_OUT aud_aenc_out_param[MAX_AUDCAP_PATH];
extern HD_AUDIOCAP_BUFINFO aud_acap_bs_buf[MAX_AUDCAP_PATH];
extern HD_AUDIOENC_BUFINFO aud_aenc_bs_buf[MAX_AUDCAP_PATH];
extern UINT32 aud_aenc_bs_buf_va[MAX_AUDCAP_PATH];
extern UINT32 AudCapBufMs[MAX_AUDCAP_PATH];
extern UINT32 AudEncBufMs[MAX_AUDCAP_PATH];
extern MOVIE_CFG_AUD_CODEC AudEncCodec[MAX_AUDCAP_PATH];
extern UINT32 aud_cap_volume;
extern UINT32 aud_cap_alc_en;
extern UINT32 aud_cap_prepwr_en;
extern BOOL aud_mute_enc_func;
extern BOOL aud_mute_enc;
extern UINT32 aud_mute_enc_buf_pa, aud_mute_enc_buf_va, aud_mute_enc_buf_size;
extern MovieAudCapCb *aud_cap_cb;
extern BOOL aud_acap_by_hdal;
#if (USE_ALSA_LIB == ENABLE)
extern snd_pcm_t *alsa_hdl;
extern UINT32 alsabuf_pa, alsabuf_va, alsabuf_size;
#endif
extern HD_AUDIOCAP_BUFINFO aud_acap_bs_buf_info;
extern UINT32 aud_acap_frame_va;
/// ImageApp_MovieMulti_CB
extern UINT32 iamovie_movie_path_info;
extern MovieUserEventCb *g_ia_moviemulti_usercb;
extern UINT32 g_vdo_rec_sec;
/// ImageApp_MovieMulti_DispLink
extern UINT32 IsDispLinkOpened[MAX_DISP_PATH];
extern UINT32 IsDispLinkStarted[MAX_DISP_PATH];
extern MOVIE_DISP_LINK DispLink[MAX_DISP_PATH];
extern MOVIE_DISP_LINK_STATUS DispLinkStatus[MAX_DISP_PATH];
extern UINT32 g_DispFps[MAX_DISP_PATH];
extern UINT32 g_DispMirrorFlip[MAX_DISP_PATH];
extern MOVIE_RAWPROC_CB  g_DispCb;
extern HD_URECT user_disp_win[MAX_DISP_PATH];
extern HD_VIDEOOUT_FUNC_CONFIG disp_func_cfg[MAX_DISP_PATH];
extern UINT32 disp_queue_depth[MAX_DISP_PATH];
/// ImageApp_MovieMulti_EthCamLink
extern UINT32 EthCamRxFuncEn;
extern MOVIE_ETHCAM_LINK EthCamLink[MAX_ETHCAM_PATH];
extern MOVIE_ETHCAM_LINK_STATUS EthCamLinkStatus[MAX_ETHCAM_PATH];
extern UINT32 is_ethcamlink_opened[MAX_ETHCAM_PATH];
extern UINT32 is_ethcamlink_for_disp_enabled[MAX_ETHCAM_PATH][MAX_DISP_PATH];
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
extern UINT32 ethcam_vproc_vpe_en[MAX_ETHCAM_PATH];
extern MovieVpeCb *ethcam_vproc_vpe_cb[MAX_ETHCAM_PATH];
extern HD_VIDEOPROC_DEV_CONFIG ethcam_vproc_cfg[MAX_ETHCAM_PATH];
extern HD_VIDEOPROC_CTRL ethcam_vproc_ctrl[MAX_ETHCAM_PATH];
extern HD_VIDEOPROC_IN ethcam_vproc_in_param[MAX_ETHCAM_PATH];
extern HD_VIDEOPROC_FUNC_CONFIG ethcam_vproc_func_cfg[MAX_ETHCAM_PATH];
extern UINT32 ethcam_vproc_vpe_scene[MAX_ETHCAM_PATH];
extern UINT32 ethcam_vproc_queue_depth[MAX_ETHCAM_PATH];
extern UINT32 ethcam_vproc_no_ext[MAX_ETHCAM_PATH];
#endif
extern HD_VIDEOENC_PATH_CONFIG ethcam_venc_path_cfg[MAX_ETHCAM_PATH];
extern HD_VIDEOENC_IN  ethcam_venc_in_param[MAX_ETHCAM_PATH];
extern HD_VIDEOENC_OUT2 ethcam_venc_out_param[MAX_ETHCAM_PATH];
extern HD_H26XENC_RATE_CONTROL2 ethcam_venc_rc_param[MAX_ETHCAM_PATH];
extern HD_VIDEOENC_BUFINFO ethcam_venc_bs_buf[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_bs_buf_va[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_quality_base_mode_en[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_sout_type[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_sout_pa[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_sout_va[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_sout_size[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_vui_disable[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_vui_color_tv_range[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_max_frame_size[MAX_ETHCAM_PATH];
extern HD_COMMON_MEM_VB_BLK ethcam_venc_sout_blk[MAX_ETHCAM_PATH];
extern MOVIE_CFG_PROFILE ethcam_venc_h264_profile[MAX_ETHCAM_PATH];
extern MOVIE_CFG_LEVEL ethcam_venc_h264_level[MAX_ETHCAM_PATH];
extern MOVIE_CFG_PROFILE ethcam_venc_h265_profile[MAX_ETHCAM_PATH];
extern MOVIE_CFG_LEVEL ethcam_venc_h265_level[MAX_ETHCAM_PATH];
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
extern VENDOR_VDOENC_XVR_APP ethcam_venc_xvr_app[MAX_ETHCAM_PATH];
#endif
extern UINT32 ethcam_venc_skip_frame_cfg[MAX_ETHCAM_PATH];
extern HD_VIDEOENC_SVC_LAYER ethcam_venc_svc[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_low_power_mode[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_trigger_mode[MAX_ETHCAM_PATH];
extern UINT64 ethcam_venc_timestamp[MAX_ETHCAM_PATH];
extern UINT32 EthCamLinkVdoEncBufMs[MAX_ETHCAM_PATH];
extern HD_VIDEOENC_MAXMEM ethcam_venc_maxmem[MAX_ETHCAM_PATH];
extern HD_VIDEODEC_PATH_CONFIG ethcam_vdec_cfg[MAX_ETHCAM_PATH][1];
extern HD_VIDEODEC_IN ethcam_vdec_in_param[MAX_ETHCAM_PATH][1];
extern HD_H26XDEC_DESC ethcam_vdec_desc[MAX_ETHCAM_PATH][1];
extern MOVIEMULTI_ETHCAM_REC_INFO EthCamLinkRecInfo[MAX_ETHCAM_PATH];
extern UINT32 EthCamLinkTimelapseTime[MAX_ETHCAM_PATH];
extern HD_BSMUX_VIDEOINFO ethcam_bsmux_vinfo[MAX_ETHCAM_PATH][2];
extern HD_BSMUX_AUDIOINFO ethcam_bsmux_ainfo[MAX_ETHCAM_PATH][2];
extern HD_BSMUX_FILEINFO ethcam_bsmux_finfo[MAX_ETHCAM_PATH][2];
extern UINT32 ethcam_bsmux_front_moov_en[MAX_ETHCAM_PATH][2];
extern UINT32 ethcam_bsmux_front_moov_flush_sec[MAX_ETHCAM_PATH][2];
extern UINT32 ethcam_bsmux_meta_data_en[MAX_ETHCAM_PATH][2];
extern UINT32 ethcam_bsmux_audio_pause[MAX_ETHCAM_PATH][2];
extern UINT32 ethcam_bsmux_bufsec[MAX_ETHCAM_PATH][2];
extern UINT32 ethcam_bsmux_cutnow_with_period_cnt[MAX_ETHCAM_PATH][2];
extern UINT32 ethcam_emr_loop_start[MAX_ETHCAM_PATH][1];
extern USIZE ethcam_venc_imgcap_thum_size[MAX_ETHCAM_PATH];
extern UINT32 ethcam_venc_imgcap_on_rx[MAX_ETHCAM_PATH];
/// ImageApp_MovieMulti_id
extern ID IAMOVIE_FLG_ID, IAMOVIE_FLG_ID2, IAMOVIE_FLG_BSMUX_ID;
extern ID IAMOVIE_FLG_BG_ID;
extern ID IAMOVIE_SEMID_FM;
extern ID IAMOVIE_SEMID_CRYPTO;
extern ID IAMOVIE_SEMID_OPERATION;
extern ID IAMOVIE_SEMID_ACAP;
extern MovieAudBsCb  *aud_bs_cb[MAX_AUDCAP_PATH];
extern MovieVdoBsCb  *vdo_bs_rec_cb[MAX_IMG_PATH][2];
extern MovieVdoBsCb  *vdo_bs_strm_cb[MAX_WIFI_PATH];
extern MovieVdoBsCb  *vdo_bs_ethcam_cb[MAX_ETHCAM_PATH];
/// ImageApp_MovieMulti_ImgCapLink
extern UINT32 IsImgCapLinkOpened[MAX_IMGCAP_PATH];
extern MOVIE_IMGCAP_LINK ImgCapLink[MAX_IMGCAP_PATH];
extern MOVIE_IMGCAP_LINK_STATUS ImgCapLinkStatus[MAX_IMGCAP_PATH];
extern HD_VIDEOENC_PATH_CONFIG imgcap_venc_path_cfg[MAX_IMGCAP_PATH];
extern HD_VIDEOENC_IN  imgcap_venc_in_param[MAX_IMGCAP_PATH];
extern HD_VIDEOENC_OUT2 imgcap_venc_out_param[MAX_IMGCAP_PATH];
extern HD_VIDEOENC_BUFINFO imgcap_venc_bs_buf[MAX_IMGCAP_PATH];
extern UINT32 imgcap_venc_bs_buf_va[MAX_IMGCAP_PATH];
extern QUEUE_BUFFER_QUEUE ImgCapJobQueueClear;
extern QUEUE_BUFFER_QUEUE ImgCapJobQueueReady;
extern UINT32 ImgCapExifFuncEn;
extern UINT32 imgcap_thum_with_exif;
extern UINT32 imgcap_thum_no_auto_scaling[MAX_IMG_PATH][2];
extern UINT32 imgcap_rawenc_pa[_CFG_ETHCAM_ID_MAX], imgcap_rawenc_va[_CFG_ETHCAM_ID_MAX], imgcap_rawenc_size[_CFG_ETHCAM_ID_MAX];
extern UINT32 imgcap_use_venc_buf;
extern UINT32 imgcap_venc_shared_sout_pa;
extern UINT32 imgcap_venc_shared_sout_va;
extern UINT32 imgcap_venc_shared_sout_size;
extern UINT32 imgcap_venc_shared_sout_ddr_id;
extern UINT32 imgcap_venc_shared_sout_count;
extern MOVIEMULTI_JPG_QUALITY imgcap_jpg_quality;
/// ImageApp_MovieMulti_ImgLink
extern UINT32 IsImgLinkOpened[MAX_IMG_PATH];
extern MOVIE_IMAGE_LINK ImgLink[MAX_IMG_PATH];
extern MOVIE_IMAGE_LINK_STATUS ImgLinkStatus[MAX_IMG_PATH];
extern UINT32 gImgLinkAlgFuncEn[MAX_IMG_PATH];
extern UINT32 gImeMaxOutPath[MAX_IMG_PATH];
extern UINT32 gImgLinkForcedSizePath[MAX_IMG_PATH];
extern UINT32 gSwitchLink[MAX_IMG_PATH][6];
extern UINT32 gUserProcMap[MAX_IMG_PATH][4];
extern UINT32 fps_vcap_out[MAX_IMG_PATH], fps_vprc_p_out[MAX_IMG_PATH][3], fps_vprc_e_out[MAX_IMG_PATH][6];
extern MOVIEMULTI_IPL_SIZE_INFO IPLUserSize[MAX_IMG_PATH];
extern UINT32 IsSetImgLinkRecInfo[MAX_IMG_PATH][2];
extern MOVIE_RECODE_INFO ImgLinkRecInfo[MAX_IMG_PATH][2];
extern UINT32 ImgLinkTimelapseTime[MAX_IMG_PATH][2];
extern UINT32 ImgLinkTimelapsePlayRate;
extern UINT32 IsSetImgLinkAlgInfo[MAX_IMG_PATH];
extern MOVIE_ALG_INFO ImgLinkAlgInfo[MAX_IMG_PATH];
extern MOVIEMULTI_IME_CROP_INFO DispIMECropInfo[MAX_IMG_PATH];
extern MOVIEMULTI_IME_CROP_INFO MainIMECropInfo[MAX_IMG_PATH];
extern MOVIEMULTI_IME_CROP_INFO CloneIMECropInfo[MAX_IMG_PATH];
extern UINT32 DispIMECropNoAutoScaling[MAX_IMG_PATH];
extern UINT32 DispIMECropFixedOutSize[MAX_IMG_PATH];
extern UINT32 IsImgLinkForEthCamTxEnabled[MAX_IMG_PATH][2];
extern HD_VIDEOCAP_DRV_CONFIG img_vcap_cfg[MAX_IMG_PATH];
extern HD_VIDEO_PXLFMT img_vcap_senout_pxlfmt[MAX_IMG_PATH];
extern HD_VIDEO_PXLFMT img_vcap_capout_pxlfmt[MAX_IMG_PATH];
extern UINT32 img_vcap_data_lane[MAX_IMG_PATH];
extern HD_VIDEOCAP_CTRL img_vcap_ctrl[MAX_IMG_PATH];
extern HD_VIDEOCAP_IN img_vcap_in_param[MAX_IMG_PATH];
extern HD_VIDEOCAP_CROP img_vcap_crop_param[MAX_IMG_PATH];
extern HD_VIDEOCAP_OUT img_vcap_out_param[MAX_IMG_PATH];
extern HD_VIDEOCAP_FUNC_CONFIG img_vcap_func_cfg[MAX_IMG_PATH];
extern VENDOR_VIDEOCAP_CCIR_FMT_SEL img_vcap_ccir_fmt[MAX_IMG_PATH];
extern UINT32 img_vcap_ccir_mode[MAX_IMG_PATH];
extern UINT32 img_vcap_detect_loop[MAX_IMG_PATH];
extern UINT32 img_vcap_patgen[MAX_IMG_PATH];
extern MOVIEMULTI_VCAP_CROP_INFO img_vcap_crop_win[MAX_IMG_PATH];
extern USIZE img_vcap_out_size[MAX_IMG_PATH];
#if defined(_BSP_NA51089_)
extern UINT32 img_vcap_sie_remap[MAX_IMG_PATH];
#endif
extern UINT32 img_vcap_gyro_func[MAX_IMG_PATH];
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
extern UINT32 img_vproc_vpe_en[MAX_IMG_PATH];
extern MovieVpeCb *img_vproc_vpe_cb[MAX_IMG_PATH];
extern UINT32 img_vproc_vpe_scene[MAX_IMG_PATH];
#endif
extern UINT32 img_vproc_splitter[MAX_IMG_PATH];
extern HD_VIDEOPROC_DEV_CONFIG img_vproc_cfg[MAX_IMG_PATH];
extern HD_VIDEOPROC_CTRL img_vproc_ctrl[MAX_IMG_PATH];
extern HD_VIDEOPROC_IN img_vproc_in_param[MAX_IMG_PATH];
extern HD_VIDEOPROC_FUNC_CONFIG img_vproc_func_cfg[MAX_IMG_PATH];
extern UINT32 img_vproc_yuv_compress[MAX_IMG_PATH];
extern UINT32 img_vproc_3dnr_path[MAX_IMG_PATH];
extern UINT32 img_vproc_forced_use_pipe[MAX_IMG_PATH];
extern UINT32 img_vproc_dis_func[MAX_IMG_PATH];
extern UINT32 img_vproc_eis_func[MAX_IMG_PATH];
extern UINT32 img_vproc_queue_depth[MAX_IMG_PATH];
extern UINT32 img_vproc_no_ext[MAX_IMG_PATH];
extern HD_VIDEOENC_PATH_CONFIG img_venc_path_cfg[MAX_IMG_PATH][2];
extern HD_VIDEOENC_IN  img_venc_in_param[MAX_IMG_PATH][2];
extern HD_VIDEOENC_OUT2 img_venc_out_param[MAX_IMG_PATH][2];
extern HD_H26XENC_RATE_CONTROL2 img_venc_rc_param[MAX_IMG_PATH][2];
extern HD_VIDEOENC_BUFINFO img_venc_bs_buf[MAX_IMG_PATH][2];
extern UINT32 img_venc_bs_buf_va[MAX_IMG_PATH][2];
extern UINT32 img_venc_quality_base_mode_en[MAX_IMG_PATH][2];
extern UINT32 img_venc_sout_type[MAX_IMG_PATH][2];
extern UINT32 img_venc_sout_pa[MAX_IMG_PATH][2];
extern UINT32 img_venc_sout_va[MAX_IMG_PATH][2];
extern UINT32 img_venc_sout_size[MAX_IMG_PATH][2];
extern UINT32 img_venc_vui_disable[MAX_IMG_PATH][2];
extern UINT32 img_venc_vui_color_tv_range[MAX_IMG_PATH][2];
extern UINT32 img_venc_max_frame_size[MAX_IMG_PATH][2];
extern UINT32 img_venc_imgcap_yuvsrc[MAX_IMG_PATH][2];
extern HD_COMMON_MEM_VB_BLK img_venc_sout_blk[MAX_IMG_PATH][2];
extern MOVIE_CFG_PROFILE img_venc_h264_profile[MAX_IMG_PATH][2];
extern MOVIE_CFG_LEVEL img_venc_h264_level[MAX_IMG_PATH][2];
extern MOVIE_CFG_PROFILE img_venc_h265_profile[MAX_IMG_PATH][2];
extern MOVIE_CFG_LEVEL img_venc_h265_level[MAX_IMG_PATH][2];
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
extern VENDOR_VDOENC_XVR_APP img_venc_xvr_app[MAX_IMG_PATH][2];
#endif
extern UINT32 img_venc_skip_frame_cfg[MAX_IMG_PATH][2];
extern HD_VIDEOENC_SVC_LAYER img_venc_svc[MAX_IMG_PATH][2];
extern UINT32 img_venc_low_power_mode[MAX_IMG_PATH][2];
extern UINT32 img_venc_trigger_mode[MAX_IMG_PATH][2];
extern UINT64 img_venc_timestamp[MAX_IMG_PATH][2];
extern HD_VIDEOENC_MAXMEM img_venc_maxmem[MAX_IMG_PATH][2];
extern HD_BSMUX_VIDEOINFO img_bsmux_vinfo[MAX_IMG_PATH][4];
extern HD_BSMUX_AUDIOINFO img_bsmux_ainfo[MAX_IMG_PATH][4];
extern HD_BSMUX_FILEINFO img_bsmux_finfo[MAX_IMG_PATH][4];
extern UINT32 img_bsmux_front_moov_en[MAX_IMG_PATH][4];
extern UINT32 img_bsmux_front_moov_flush_sec[MAX_IMG_PATH][4];
extern UINT32 img_bsmux_2v1a_mode[MAX_IMG_PATH];
extern UINT32 img_bsmux_meta_data_en[MAX_IMG_PATH][4];
extern UINT32 img_bsmux_audio_pause[MAX_IMG_PATH][4];
extern UINT32 img_bsmux_cutnow_with_period_cnt[MAX_IMG_PATH][4];
extern MOVIE_RECODE_FILE_OPTION ImgLinkFileInfo;
extern UINT32 forced_use_unique_3dnr_path[MAX_IMG_PATH];
extern UINT32 use_unique_3dnr_path[MAX_IMG_PATH];
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == ENABLE)
extern UINT32 img_venc_frame_cnt[MAX_IMG_PATH][2];
extern UINT32 img_venc_frame_cnt_clr[MAX_IMG_PATH][2];
extern UINT32 img_venc_1st_path_idx;
extern UINT32 img_venc_1st_path_ch;
#endif
extern UINT32 trig_once_limited[MAX_IMG_PATH][2];
extern UINT32 trig_once_cnt[MAX_IMG_PATH][2];
extern UINT32 trig_once_first_i[MAX_IMG_PATH][2];
extern UINT32 trig_once_2v1a_stop_path[MAX_IMG_PATH][2];
extern UINT32 gpsdata_pa, gpsdata_va, gpsdata_size, gpsdata_offset;
extern UINT32 gpsdata_rec_pa[MAX_IMG_PATH][2], gpsdata_rec_va[MAX_IMG_PATH][2], gpsdata_rec_size[MAX_IMG_PATH][2], gpsdata_rec_offset[MAX_IMG_PATH][2];
extern UINT32 gpsdata_eth_pa[MAX_ETHCAM_PATH][1], gpsdata_eth_va[MAX_ETHCAM_PATH][1], gpsdata_eth_size[MAX_ETHCAM_PATH][1], gpsdata_eth_offset[MAX_ETHCAM_PATH][1];
extern UINT32 gpsdata_rec_disable[MAX_IMG_PATH][2], gpsdata_rec_rate[MAX_IMG_PATH][2];
extern UINT32 gpsdata_eth_disable[MAX_ETHCAM_PATH][1], gpsdata_eth_rate[MAX_ETHCAM_PATH][1];
extern UINT32 img_pseudo_rec_mode[MAX_IMG_PATH][2];
extern UINT32 img_manual_push_vdo_frame[MAX_IMG_PATH][2];
extern UINT32 img_manual_push_raw_frame[MAX_IMG_PATH];
extern UINT32 img_manual_push_vpe_frame[MAX_IMG_PATH];
extern UINT32 img_to_imgcap_id;
extern UINT32 img_to_imgcap_path;
extern HD_VIDEO_FRAME *p_img_to_imgcap_frame;
extern UINT32 img_emr_loop_start[MAX_IMG_PATH][2];
extern USIZE img_venc_imgcap_thum_size[MAX_IMG_PATH][2];
/// ImageApp_MovieMulti_Param
extern UINT32 iamovie_gps_buffer_size;
extern UINT32 iamovie_encrypt_type;
extern UINT32 iamovie_encrypt_mode;
extern UINT8  iamovie_encrypt_key[32];
/// ImageApp_MovieMulti_Operation
extern UINT32 IsImgLinkForDispEnabled[MAX_IMG_PATH][MAX_DISP_PATH];
extern UINT32 IsImgLinkForStreamingEnabled[MAX_IMG_PATH][MAX_WIFI_PATH];
extern UINT32 IsImgLinkForUVACEnabled[MAX_IMG_PATH][MAX_WIFI_PATH];
extern UINT32 IsImgLinkForAlgEnabled[MAX_IMG_PATH][_CFG_ALG_PATH_MAX];
extern UINT32 IsImgLinkTranscodeMode[MAX_IMG_PATH][2];
extern UINT32 IsImgLinkMode3RecStart[MAX_IMG_PATH][4];
/// ImageApp_MovieMulti_Param
extern UINT32 g_ia_moviemulti_filedb_filter;
extern UINT32 g_ia_moviemulti_filedb_max_num;
extern UINT32 g_ia_moviemulti_delete_filter;
extern UINT32 g_ia_moviemulti_crash_not_ro;
extern UINT32 g_ia_moviemulti_emr_not_ro;
/// ImageApp_MovieMulti_SxCmd
extern BOOL g_ia_moviemulti_trace_on;
/// ImageApp_MovieMulti_Utilities
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
extern UINT32 FirstBsMuxPath;
#endif
extern UINT32 BsMuxFrmCnt[MAX_BSMUX_PATH];
/// ImageApp_MovieMulti_WifiLink
extern MOVIE_STRM_INFO WifiLinkStrmInfo[MAX_WIFI_PATH];
extern UINT32 IsWifiLinkOpened[MAX_WIFI_PATH];
extern UINT32 IsWifiLinkStarted[MAX_WIFI_PATH];
extern UINT32 IsUvacLinkStarted[MAX_WIFI_PATH];
extern MOVIE_WIFI_LINK WifiLink[MAX_WIFI_PATH];
extern MOVIE_WIFI_LINK_STATUS WifiLinkStatus[MAX_WIFI_PATH];
extern HD_VIDEOENC_PATH_CONFIG wifi_venc_path_cfg[MAX_WIFI_PATH];
extern HD_VIDEOENC_IN  wifi_venc_in_param[MAX_WIFI_PATH];
extern HD_VIDEOENC_OUT2 wifi_venc_out_param[MAX_WIFI_PATH];
extern HD_H26XENC_RATE_CONTROL2 wifi_venc_rc_param[MAX_WIFI_PATH];
extern UINT32 wifi_venc_vui_disable[MAX_WIFI_PATH];
extern UINT32 wifi_venc_vui_color_tv_range[MAX_WIFI_PATH];
extern UINT32 wifi_venc_max_frame_size[MAX_WIFI_PATH];
extern MOVIE_CFG_PROFILE wifi_venc_h264_profile[MAX_WIFI_PATH];
extern MOVIE_CFG_LEVEL wifi_venc_h264_level[MAX_WIFI_PATH];
extern MOVIE_CFG_PROFILE wifi_venc_h265_profile[MAX_WIFI_PATH];
extern MOVIE_CFG_LEVEL wifi_venc_h265_level[MAX_WIFI_PATH];
#if (defined(_BSP_NA51089_) || defined(_BSP_NA51102_))
extern VENDOR_VDOENC_XVR_APP wifi_venc_xvr_app[MAX_WIFI_PATH];
#endif
extern HD_VIDEOENC_SVC_LAYER wifi_venc_svc[MAX_WIFI_PATH];
extern UINT32 wifi_venc_low_power_mode[MAX_WIFI_PATH];
extern UINT64 wifi_venc_timestamp[MAX_WIFI_PATH];
extern UINT32 WifiLinkVdoEncBufMs[MAX_WIFI_PATH];
extern HD_VIDEOENC_MAXMEM wifi_venc_maxmem[MAX_WIFI_PATH];
extern MOVIE_RAWPROC_CB  g_WifiCb;
extern UINT32 wifi_movie_uvac_func_en;
extern UVAC_INFO gWifiMovieUvacInfo;
extern UVAC_VEND_DEV_DESC *gpWifiMovieUvacVendDevDesc;
extern UINT32 wifi_movie_uvac_pa, wifi_movie_uvac_va, wifi_movie_uvac_size;
extern HD_VIDEOENC_BUFINFO wifi_venc_bs_buf[MAX_WIFI_PATH];
extern UINT32 wifi_venc_bs_buf_va[MAX_WIFI_PATH];
extern UINT32 wifi_venc_trigger_mode[MAX_WIFI_PATH];

/// ========== Internal function area ==========
/// ImageApp_MovieMulti
extern ER _ImageApp_MovieMulti_LinkCfg(void);
extern ER _ImageApp_MovieMulti_RecidGetTableIndex(MOVIE_CFG_REC_ID id, MOVIE_TBL_IDX *p);
extern ER _ImageApp_MovieMulti_BsPortGetTableIndex(UINT32 id, MOVIE_TBL_IDX *p);
extern UINT32 _ImageApp_MovieMulti_GetFreeVprc(void);
extern ER _ImageApp_MovieMulti_CheckEMRMode(void);
/// ImageApp_MovieMulti_AudCapLink
extern ER _ImageApp_MovieMulti_AudCapLinkCfg(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_AudSetAEncParam(UINT32 idx);
extern ER _ImageApp_MovieMulti_AudReopenAEnc(UINT32 idx);
extern ER _ImageApp_MovieMulti_AudCapLinkOpen(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_AudCapLinkClose(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_AudCapLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction);
extern ER _ImageApp_MovieMulti_AudRecStart(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_AudRecStop(MOVIE_CFG_REC_ID id);
extern void _ImageApp_MovieMulti_AudCapFrameProc(HD_AUDIO_FRAME *pdata);
/// ImageApp_MovieMulti_CB
extern MOVIE_CB_CRASH_INFO *_MovieMulti_Get_CrashInfo(UINT32 idx);
extern INT32 _ImageApp_MovieMulti_BsMux_CB(CHAR *p_name, HD_BSMUX_CBINFO *cbinfo, UINT32 *param);
extern INT32 _ImageApp_MovieMulti_FileOut_CB(CHAR *p_name, HD_FILEOUT_CBINFO *cbinfo, UINT32 *param);
/// ImageApp_MovieMulti_DispLink
extern ER _ImageApp_MovieMulti_DispLinkCfg(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_DispLinkOpen(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_DispLinkClose(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_DispLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction);
extern ER _ImageApp_MovieMulti_DispGetSize(MOVIE_CFG_REC_ID id, USIZE *sz);
/// ImageApp_MovieMulti_EthCamLink
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
extern ER _ImageApp_MovieMulti_EthCamReopenVPrc(UINT32 idx);
#endif
extern ER _ImageApp_MovieMulti_EthCamSetVEncParam(UINT32 idx);
extern ER _ImageApp_MovieMulti_EthCamReopenVEnc(UINT32 idx);
extern ER _ImageApp_MovieMulti_EthCamLinkCfg(MOVIE_CFG_REC_ID id, UINT32 submit);
extern ER _ImageApp_MovieMulti_EthCamLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction);
extern ER _ImageApp_MovieMulti_EthCamLinkOpen(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_EthCamLinkClose(MOVIE_CFG_REC_ID id);
/// ImageApp_MovieMulti_id
extern BOOL _ImageApp_MovieMulti_IsBGTskRunning(void);
extern ER _ImageApp_MovieMulti_CreateBGTsk(void);
extern ER _ImageApp_MovieMulti_InstallID(void);
extern ER _ImageApp_MovieMulti_UninstallID(void);
/// ImageApp_MovieMulti_ImgCapLink
extern ER _ImageApp_MovieMulti_AllocSharedSrcOutBuf(UINT32 buf_ddr_id, UINT32 buf_size, UINT32 *buf_pa, UINT32 *buf_va);
extern ER _ImageApp_MovieMulti_FreeSharedSrcOutBuf(void);
extern ER _ImageApp_MovieMulti_ImgCapLinkCfg(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_ImgCapLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction);
extern ER _ImageApp_MovieMulti_ImgCapLinkOpen(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_ImgCapLinkClose(MOVIE_CFG_REC_ID id);
/// ImageApp_MovieMulti_ImgLink
extern ER _ImageApp_MovieMulti_ImgLinkCfg(MOVIE_CFG_REC_ID id, UINT32 submit);
extern ER _ImageApp_MovieMulti_ImgSetVEncParam(UINT32 idx, UINT32 tbl);
extern ER _ImageApp_MovieMulti_ImgReopenVEnc(UINT32 idx, UINT32 tbl);
extern ER _ImageApp_MovieMulti_ImgLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction);
extern ER _ImageApp_MovieMulti_ImgLinkOpen(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_ImgLinkClose(MOVIE_CFG_REC_ID id);
extern void _ImageApp_MovieMulti_GetVideoFrameForImgCap(UINT32 id, UINT32 path, HD_VIDEO_FRAME *p_frame);
/// ImageApp_MovieMulti_Operation
extern ER _ImageApp_MovieMulti_TriggerThumb(MOVIE_CFG_REC_ID id, UINT32 is_emr);
/// ImageApp_MovieMulti_Param
extern ER _ImageApp_MovieMulti_SetupAudioParam(void);
extern ER _ImageApp_MovieMulti_SetupRecParam(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_SetupFileOption(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_SetupStrmParam(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_SetupImgCapParam(MOVIE_CFG_REC_ID id);
/// ImageApp_MovieMulti_Utilities
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
extern UINT32 _ImageApp_MovieMulti_GetFirstBsMuxPort(void);
#endif
extern UINT32 _ImageApp_MovieMulti_IsRecording(void);
extern UINT32 _ImageApp_MovieMulti_frc(UINT32 dst_fr, UINT32 src_fr);
extern BOOL _ImageApp_MovieMulti_CheckProfileLevelValid(MOVIE_CFG_CODEC_INFO *pinfo);
extern ER _ImageApp_MovieMulti_CheckOpenSetting(void);
extern UINT32 _ImageApp_MovieMulti_GetMaxImgPath(void);
extern ER _ImageApp_MovieMulti_Crypto_Init(void);
extern ER _ImageApp_MovieMulti_Crypto_Uninit(void);
extern ER _ImageApp_MovieMulti_Crypto(UINT32 addr, UINT32 size);
extern HD_RESULT _ImageApp_MovieMulti_UpdateVprcDepth(HD_PATH_ID path, UINT32 depth);
/// ImageApp_MovieMulti_WifiLink
extern ER _ImageApp_MovieMulti_WifiLinkCfg(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_WifiSetVEncParam(UINT32 idx);
extern ER _ImageApp_MovieMulti_WifiReopenVEnc(UINT32 idx);
extern ER _ImageApp_MovieMulti_WifiLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction);
extern ER _ImageApp_MovieMulti_WifiLinkOpen(MOVIE_CFG_REC_ID id);
extern ER _ImageApp_MovieMulti_WifiLinkClose(MOVIE_CFG_REC_ID id);

/// ========== Switch sorting function area ==========
#define SWITCH_IN_MAX       6
#define SWITCH_OUT_MAX      6

typedef struct _SW_PORT_SPEC {
	UINT32 w;
	UINT32 h;
	UINT32 fmt;
	UINT32 frc;
} SW_PORT_SPEC;

typedef struct _SW_PORT_TABLE {
	UINT32 BeginCollect;
	UINT32 AutoInNum;
	SW_PORT_SPEC OUT[SWITCH_OUT_MAX];
	SW_PORT_SPEC IN[SWITCH_IN_MAX];
	UINT32 InStart;
	UINT32 InEnd;
} SW_PORT_TABLE;

extern ER _Switch_Auto_Begin(SW_PORT_TABLE *tbl);
extern ER _Switch_Auto_End(SW_PORT_TABLE *tbl);
extern UINT32 _Switch_Auto_GetConnect(SW_PORT_TABLE *tbl, UINT32 o);

#endif//IMAGEAPP_MOVIEMULTI_INT_H
