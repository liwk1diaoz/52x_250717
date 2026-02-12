#ifndef IMAGEAPP_COMMON_INT_H
#define IMAGEAPP_COMMON_INT_H

#include <string.h>
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include <kwrap/type.h>
#include <kwrap/util.h>
#include "hdal.h"
#include "hd_bsmux_lib.h"
#include "hd_fileout_lib.h"
#include "ImageApp/ImageApp_Common.h"
#include "nvtrtspd.h"
#include "LviewNvt/lviewd.h"
#include "LviewNvt/LviewNvtAPI.h"
#include "vendor_videocapture.h"
#include "vendor_videoout.h"

/// ========== Branch definition area ==========
#define IACOMMON_BR_1_25            125
#define IACOMMON_BR_1_40            140
#define IACOMMON_BRANCH             IACOMMON_BR_1_40

/// ========== Debug macro area ==========
#define IACOMMON_DEBUG_VTIME                        DISABLE
#define IACOMMON_BS_ERR_TIMEOUT_CHECK               100     // unit: ms

#define ImageApp_Common_DUMP(fmtstr, args...) do { if(g_ia_common_trace_on) DBG_DUMP(fmtstr, ##args); } while(0)

/// ========== Internal function configuation area ==========
/// fixed rtsp stream 0             default (master): DISABLE, refer to IVOT_N00017_CO-23
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
#define RTSP_FIXED_STREAM0          DISABLE
#else
#define RTSP_FIXED_STREAM0          ENABLE
#endif

/// ========== Port status definition area ==========
#define UNUSED                      0xFFFFFFFF
#define PORT_INC_IND                0x80000000
#define PORT_DEC_IND                0x40000000
#define CB_STATE_BEFORE_RUN         0
#define CB_STATE_AFTER_RUN          1
#define CB_STATE_BEFORE_STOP        2
#define CB_STATE_AFTER_STOP         3

typedef enum {
	STATE_STOP                    = 0,
	STATE_RUN                     = 1,
	STATE_PREPARE_TO_STOP         = 2,
	STATE_PREPARE_TO_RUN          = 3,
} IACOMM_PATH_STATE;

typedef HD_RESULT (*MODULE_SET_STATE)(HD_PATH_ID path_id, UINT32 state);
typedef void (*LINKUPDATESTATUS_CB)(HD_PATH_ID path_id, UINT32 state);

/// ========== HDAL function area ==========
#define HD_VIDEOPROC_PIPE_OUT_DIS   0x00000800

/// ========== RTSP definition area ==========
#define MAX_RTSP_PATH           1

/// ========== Flag id definition area ==========
#define FLGIACOMMON_VE_S0_BS_RDY    0x00000001     /// video queue create indicator
#define FLGIACOMMON_AE_S0_BS_RDY    0x00000010
#define FLGIACOMMON_VE_S0_QUE_RDY   0x00000100
#define FLGIACOMMON_AE_S0_QUE_RDY   0x00001000
#define FLGIACOMMON_VE_S0_CQUE_FREE 0x00010000
#define FLGIACOMMON_AE_S0_CQUE_FREE 0x00100000
#define FLGIACOMMON_VE_S0_RQUE_FREE 0x01000000
#define FLGIACOMMON_AE_S0_RQUE_FREE 0x10000000
#define FLGIACOMMON_RDY_FLAG        (FLGIACOMMON_VE_S0_BS_RDY | FLGIACOMMON_AE_S0_BS_RDY | FLGIACOMMON_VE_S0_QUE_RDY | FLGIACOMMON_AE_S0_QUE_RDY)
#define FLGIACOMMON_FREE_FLAG       (FLGIACOMMON_VE_S0_CQUE_FREE | FLGIACOMMON_AE_S0_CQUE_FREE | FLGIACOMMON_VE_S0_RQUE_FREE | FLGIACOMMON_AE_S0_RQUE_FREE)


/// ========== queue function common area ==========
extern void _QUEUE_EnQueueToTail(QUEUE_BUFFER_QUEUE *queue, QUEUE_BUFFER_ELEMENT *element);
extern QUEUE_BUFFER_ELEMENT* _QUEUE_DeQueueFromHead(QUEUE_BUFFER_QUEUE *queue);
extern UINT32 _QUEUE_GetQueueSize(QUEUE_BUFFER_QUEUE *queue);

/// ========== Streaming queue area ==========
#define STRM_VQUEUENUM          30
#define STRM_AQUEUENUM          30

typedef struct {
	QUEUE_BUFFER_ELEMENT q;             // should be the first element of this structure
	UINT64               timestamp;
	HD_AUDIO_CODEC       acodec_format;
	UINT32               phy_addr;
	UINT32               size;
} STRM_ABUFFER_ELEMENT;

typedef struct  {
	QUEUE_BUFFER_ELEMENT q;             // should be the first element of this structure
	UINT64               timestamp;
	HD_VIDEO_CODEC       vcodec_format;
	UINT32               pack_num;
	UINT32               phy_addr[VENC_BS_PACK_MAX];
	UINT32               size[VENC_BS_PACK_MAX];
} STRM_VBUFFER_ELEMENT;

/// ========== Live555 internal data structures area ==========
typedef struct _VIDEO_INFO {
	UINT32 id;
	HD_PATH_ID venc_p;
	STRM_VBUFFER_ELEMENT *pVBuf;
	NVTLIVE555_CODEC codec_type;
	unsigned char vps[64];
	int vps_size;
	unsigned char sps[64];
	int sps_size;
	unsigned char pps[64];
	int pps_size;
	int ref_cnt;
} VIDEO_INFO;

typedef struct _ADUIO_INFO {
	UINT32 id;
	int ref_cnt;
	STRM_ABUFFER_ELEMENT *pABuf;
	NVTLIVE555_AUDIO_INFO info;
	HD_PATH_ID aenc_p;
} AUDIO_INFO;

/// ========== hdal internal data structures area ==========
/// ===== ACAP =====
typedef struct _IACOMM_ACAP_CFG {
	HD_PATH_ID                  *p_audio_cap_ctrl;
	HD_CTRL_ID                   ctrl_id;
	HD_AUDIOCAP_DEV_CONFIG      *pdev_cfg;
	HD_AUDIOCAP_DRV_CONFIG      *pdrv_cfg;
} IACOMM_ACAP_CFG;

typedef struct _IACOMM_ACAP_PARAM {
	HD_PATH_ID                   audio_cap_path;
	HD_AUDIOCAP_IN              *pcap_param;
} IACOMM_ACAP_PARAM;

/// ===== ADEC =====
typedef struct _IACOMM_ADEC_CFG {
	HD_PATH_ID                   audio_dec_path;
	HD_AUDIODEC_PATH_CONFIG     *pcfg;
} IACOMM_ADEC_CFG;

typedef struct _IACOMM_ADEC_PARAM {
	HD_PATH_ID                   audio_dec_path;
	HD_AUDIODEC_IN              *pin_param;
} IACOMM_ADEC_PARAM;

/// ===== AENC =====
typedef struct _IACOMM_AENC_CFG {
	HD_PATH_ID                   audio_enc_path;
	HD_AUDIOENC_PATH_CONFIG2    *pcfg;
} IACOMM_AENC_CFG;

typedef struct _IACOMM_AENC_PARAM {
	HD_PATH_ID                   audio_enc_path;
	HD_AUDIOENC_IN              *pin_param;
	HD_AUDIOENC_OUT             *pout_param;
} IACOMM_AENC_PARAM;

/// ===== AOUT =====
typedef struct _IACOMM_AOUT_CFG {
	HD_PATH_ID                  *p_audio_out_ctrl;
	HD_CTRL_ID                   ctrl_id;
	HD_AUDIOOUT_DEV_CONFIG      *pdev_cfg;
	HD_AUDIOOUT_DRV_CONFIG      *pdrv_cfg;
} IACOMM_AOUT_CFG;

typedef struct _IACOMM_AOUT_PARAM {
	HD_PATH_ID                   audio_out_ctrl;
	HD_PATH_ID                   audio_out_path;
	HD_AUDIOOUT_IN              *pin_param;
	HD_AUDIOOUT_OUT             *pout_param;
	HD_AUDIOOUT_VOLUME          *p_vol;
} IACOMM_AOUT_PARAM;

/// ===== BSMUX =====
typedef struct _IACOMM_BSMUX_CFG {
	HD_PATH_ID                   path_id;
	HD_BSMUX_PARAM_ID            id;
	VOID                        *p_param;
} IACOMM_BSMUX_CFG;

typedef struct _IACOMM_BSMUX_PARAM {
	HD_PATH_ID                   path_id;
	HD_BSMUX_PARAM_ID            id;
	VOID                        *p_param;
} IACOMM_BSMUX_PARAM;

/// ===== FILEOUT =====
typedef struct _IACOMM_FILEOUT_CFG {
	HD_PATH_ID                   path_id;
	HD_FILEOUT_PARAM_ID          id;
	VOID                        *p_param;
} IACOMM_FILEOUT_CFG;

typedef struct _IACOMM_FILEOUT_PARAM {
	HD_PATH_ID                   path_id;
	HD_FILEOUT_PARAM_ID          id;
	VOID                        *p_param;
} IACOMM_FILEOUT_PARAM;

/// ===== VCAP =====
typedef struct _IACOMM_VCAP_CFG {
	HD_PATH_ID                  *p_video_cap_ctrl;
	HD_CTRL_ID                   ctrl_id;
	HD_VIDEOCAP_DRV_CONFIG      *pcfg;
	HD_VIDEOCAP_CTRL            *pctrl;
} IACOMM_VCAP_CFG;

typedef struct _IACOMM_VCAP_PARAM {
	HD_PATH_ID                   video_cap_path;
	UINT32                       data_lane;
	HD_VIDEOCAP_IN              *pin_param;
	HD_VIDEOCAP_CROP            *pcrop_param;
	HD_VIDEOCAP_OUT             *pout_param;
	HD_VIDEOCAP_FUNC_CONFIG     *pfunc_cfg;
} IACOMM_VCAP_PARAM;

/// ===== VDEC =====
typedef struct _IACOMM_VDEC_CFG {
	HD_PATH_ID                   video_dec_path;
	HD_VIDEODEC_PATH_CONFIG     *pcfg;
} IACOMM_VDEC_CFG;

typedef struct _IACOMM_VDEC_PARAM {
	HD_PATH_ID                   video_dec_path;
	HD_VIDEODEC_IN              *pin;
	HD_H26XDEC_DESC             *pdesc;
} IACOMM_VDEC_PARAM;

/// ===== VENC =====
typedef struct _IACOMM_VENC_CFG {
	HD_PATH_ID                   video_enc_path;
	HD_VIDEOENC_PATH_CONFIG     *ppath_cfg;
	HD_VIDEOENC_FUNC_CONFIG     *pfunc_cfg;
} IACOMM_VENC_CFG;

typedef struct _IACOMM_VENC_PARAM {
	HD_PATH_ID                   video_enc_path;
	HD_VIDEOENC_IN              *pin;
	HD_VIDEOENC_OUT2            *pout;
	HD_H26XENC_RATE_CONTROL2    *prc;
} IACOMM_VENC_PARAM;

/// ===== VOUT =====
typedef struct _IACOMM_VOUT_CFG {
	HD_PATH_ID                  *p_video_out_ctrl;
	UINT32                       out_type;
	HD_VIDEOOUT_HDMI_ID          hdmi_id;
} IACOMM_VOUT_CFG;

typedef struct _IACOMM_VOUT_PARAM {
	HD_PATH_ID                   video_out_path;
	HD_URECT                    *p_rect;
	BOOL                         enable;
	HD_VIDEO_DIR                 dir;
	HD_VIDEOOUT_FUNC_CONFIG     *pfunc_cfg;
} IACOMM_VOUT_PARAM;

/// ===== VPROC =====
typedef struct _IACOMM_VPROC_CFG {
	HD_PATH_ID *p_video_proc_ctrl;
	HD_CTRL_ID ctrl_id;
	HD_VIDEOPROC_DEV_CONFIG *pcfg;
	HD_VIDEOPROC_CTRL *pctrl;
	HD_VIDEOPROC_FUNC_CONFIG *pfunc;
} IACOMM_VPROC_CFG;

typedef struct _IACOMM_VPROC_PARAM_IN {
	HD_PATH_ID video_proc_path;
	HD_VIDEOPROC_IN *in_param;
} IACOMM_VPROC_PARAM_IN;

typedef struct _IACOMM_VPROC_PARAM {
	HD_PATH_ID video_proc_path;
	HD_DIM* p_dim;
	HD_VIDEO_PXLFMT pxlfmt;
	UINT32 frc;
	HD_VIDEOPROC_FUNC_CONFIG *pfunc;
	HD_VIDEOPROC_CROP video_proc_crop_param;
} IACOMM_VPROC_PARAM;

typedef struct _IACOMM_VPROC_PARAM_EX {
	HD_PATH_ID video_proc_path_ex;
	HD_PATH_ID video_proc_path_src;
	HD_DIM* p_dim;
	HD_VIDEO_PXLFMT pxlfmt;
	HD_VIDEO_DIR dir;
	UINT32 frc;
	UINT32 depth;
	HD_VIDEOPROC_CROP video_proc_crop_param;
	HD_VIDEOPROC_FUNC_CONFIG *pfunc;
} IACOMM_VPROC_PARAM_EX;

/// ========== enum area ==========
enum {
	IACOMMON_STREAMING_MODE_LIVE555,
	IACOMMON_STREAMING_MODE_LVIEWNVT,
};

/// ========== Internal cross file variable area ==========
/// ImageApp_Common
extern BOOL g_ia_common_trace_on;
extern ID IACOMMON_FLG_ID;
extern ID IACOMMON_SEMID_QUE;
extern ID IACOMMON_SEMID_LOCK_VIDEO;
extern ID IACOMMON_SEMID_PORT_STATUS;
extern UINT32 is_iacomm_init;
extern UINT32 is_iacomm_rtsp_start;
extern UINT32 iacomm_streaming_mode;
extern UINT32 iacomm_keep_sem_flag;
extern UINT32 iacomm_reinited;
extern UINT32 iacomm_use_external_streaming;
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
extern UINT32 iacomm_LviewNvt_portNum;
extern UINT32 iacomm_LviewNvt_threadPriority;
extern UINT32 iacomm_LviewNvt_frameRate;
extern UINT32 iacomm_LviewNvt_sockbufSize;
extern UINT32 iacomm_LviewNvt_timeoutCnt;
extern UINT32 iacomm_LviewNvt_tos;
#endif
/// ImageApp_Common_CB
extern IACommonUserEventCb *g_ia_common_usercb;
extern IACommonRtspParseUrlCb *g_ia_common_rtsp_parse_url_cb;
/// ImageApp_Common_Param
extern UINT32 live555_support_rtcp;
extern UINT32 live555_rtsp_fixed_stream0;
/// iacomm_live555
extern QUEUE_BUFFER_QUEUE VBufferQueueClear[MAX_RTSP_PATH];
extern QUEUE_BUFFER_QUEUE VBufferQueueReady[MAX_RTSP_PATH];
extern QUEUE_BUFFER_QUEUE ABufferQueueClear[MAX_RTSP_PATH];
extern QUEUE_BUFFER_QUEUE ABufferQueueReady[MAX_RTSP_PATH];

/// ========== Internal function area ==========
extern void _iacommp_set_streaming_mode(UINT32 mode);
extern ER iacomm_flag_clr(ID flgid, FLGPTN clrptn);
extern ER iacomm_flag_set(ID flgid, FLGPTN setptn);
extern ER iacomm_flag_wait(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode);
extern ER iacomm_flag_wait_timeout(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode, int timeout_tick);
extern FLGPTN iacomm_flag_chk(ID flgid, FLGPTN chkptn);

/// ========== unit operation function area ==========
// ImageApp_Common_Utilities
extern ER _LinkPortCntInc(UINT32 *pOrgCnt);
extern ER _LinkPortCntDec(UINT32 *pOrgCnt);
extern ER _LinkUpdateStatus(MODULE_SET_STATE fp, HD_PATH_ID path_id, UINT32 *pTargetStatus, LINKUPDATESTATUS_CB fp_cb);
extern UINT32 _LinkCheckStatus(UINT32 status);
extern UINT32 _LinkCheckStatus2(UINT32 status);

/// ========== unit operation function area ==========
// iacomm_audiocap
extern HD_RESULT acap_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT acap_close_ch(HD_PATH_ID path_id);
extern HD_RESULT acap_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id);
extern HD_RESULT acap_unbind(HD_OUT_ID _out_id);
extern HD_RESULT acap_set_cfg(IACOMM_ACAP_CFG *pacap_cfg);
extern HD_RESULT acap_set_param(IACOMM_ACAP_PARAM *pacap_param);
extern HD_RESULT acap_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT acap_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
// iacomm_audiodec
extern HD_RESULT adec_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT adec_close_ch(HD_PATH_ID path_id);
extern HD_RESULT adec_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id);
extern HD_RESULT adec_unbind(HD_OUT_ID _out_id);
extern HD_RESULT adec_set_cfg(IACOMM_ADEC_CFG *padec_cfg);
extern HD_RESULT adec_set_param(IACOMM_ADEC_PARAM *padec_param);
extern HD_RESULT adec_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT adec_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
// iacomm_audioenc
extern HD_RESULT aenc_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT aenc_close_ch(HD_PATH_ID path_id);
extern HD_RESULT aenc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id);
extern HD_RESULT aenc_unbind(HD_OUT_ID _out_id);
extern HD_RESULT aenc_set_cfg(IACOMM_AENC_CFG *paenc_cfg);
extern HD_RESULT aenc_set_param(IACOMM_AENC_PARAM *paenc_param);
extern HD_RESULT aenc_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT aenc_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
// iacomm_audioout
extern HD_RESULT aout_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT aout_close_ch(HD_PATH_ID path_id);
extern HD_RESULT aout_set_cfg(IACOMM_AOUT_CFG *paout_cfg);
extern HD_RESULT aout_set_param(IACOMM_AOUT_PARAM *paout_param);
extern HD_RESULT aout_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT aout_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
// iacomm_bsmux
extern HD_RESULT bsmux_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT bsmux_close_ch(HD_PATH_ID path_id);
extern HD_RESULT bsmux_set_cfg(IACOMM_BSMUX_CFG *pbsmux_cfg);
extern HD_RESULT bsmux_set_param(IACOMM_BSMUX_PARAM *pbsmux_param);
extern HD_RESULT bsmux_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT bsmux_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
// iacomm_fileout
extern HD_RESULT fileout_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT fileout_close_ch(HD_PATH_ID path_id);
extern HD_RESULT fileout_set_cfg(IACOMM_FILEOUT_CFG *pfileout_cfg);
extern HD_RESULT fileout_set_param(IACOMM_FILEOUT_PARAM *pfileout_param);
extern HD_RESULT fileout_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT fileout_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
// iacomm_live555
extern ER live555_init(UINT32 id);
extern ER live555_reset_Aqueue(UINT32 id);
extern ER live555_reset_Vqueue(UINT32 id);
extern UINT32 live555_mmap_venc_bs(UINT32 id, HD_PATH_ID path_id);
extern UINT32 live555_munmap_venc_bs(UINT32 id);
extern int live555_refresh_video_info(UINT32 id);
extern int live555_set_audio_info(UINT32 id, NVTLIVE555_AUDIO_INFO *info);
extern int live555_on_open_video(int channel);
extern int live555_on_close_video(int handle);
extern int live555_on_get_video_info(int handle, int timeout_ms, NVTLIVE555_VIDEO_INFO *p_info);
extern int live555_on_lock_video(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm);
extern int live555_on_unlock_video(int handle);
extern UINT32 live555_mmap_aenc_bs(UINT32 id, HD_PATH_ID path_id);
extern UINT32 live555_munmap_aenc_bs(UINT32 id);
extern int live555_on_open_audio(int channel);
extern int live555_on_close_audio(int handle);
extern int live555_on_get_audio_info(int handle, int timeout_ms, NVTLIVE555_AUDIO_INFO *p_info);
extern int live555_on_lock_audio(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm);
extern int live555_on_unlock_audio(int handle);
// iacomm_lviewnvt
extern ER lviewnvt_start_daemon(LVIEWNVT_DAEMON_INFO *p_open);
extern ER lviewnvt_stop_daemon(void);
extern ER lviewnvt_push_frame(LVIEWNVT_FRAME_INFO *frame_info);
// iacomm_videocap
extern HD_RESULT vcap_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT vcap_close_ch(HD_PATH_ID path_id);
extern HD_RESULT vcap_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id);
extern HD_RESULT vcap_unbind(HD_OUT_ID _out_id);
extern HD_RESULT vcap_set_cfg(IACOMM_VCAP_CFG *pvcap_cfg);
extern HD_RESULT vcap_set_param(IACOMM_VCAP_PARAM *pvcap_param);
extern HD_RESULT vcap_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT vcap_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
extern HD_RESULT vcap_get_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps);
// iacomm_videodec
extern HD_RESULT vdec_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT vdec_close_ch(HD_PATH_ID path_id);
extern HD_RESULT vdec_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id);
extern HD_RESULT vdec_unbind(HD_OUT_ID _out_id);
extern HD_RESULT vdec_set_cfg(IACOMM_VDEC_CFG *pvdec_cfg);
extern HD_RESULT vdec_set_param(IACOMM_VDEC_PARAM *pvdec_param);
extern HD_RESULT vdec_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT vdec_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
// iacomm_videoenc
extern HD_RESULT venc_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT venc_close_ch(HD_PATH_ID path_id);
extern HD_RESULT venc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id);
extern HD_RESULT venc_unbind(HD_OUT_ID _out_id);
extern HD_RESULT venc_set_cfg(IACOMM_VENC_CFG *pvenc_cfg);
extern HD_RESULT venc_set_param(IACOMM_VENC_PARAM *pvenc_param);
extern HD_RESULT venc_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT venc_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
// iacomm_videoout
extern HD_RESULT vout_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT vout_close_ch(HD_PATH_ID path_id);
extern HD_RESULT vout_set_cfg(IACOMM_VOUT_CFG *pvout_cfg);
extern HD_RESULT vout_set_param(IACOMM_VOUT_PARAM *pvout_param);
extern HD_RESULT vout_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT vout_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);
extern HD_RESULT vout_get_caps(HD_PATH_ID video_out_ctrl,HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps);
// iacomm_videoprocess
extern HD_RESULT vproc_open_ch(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id);
extern HD_RESULT vproc_close_ch(HD_PATH_ID path_id);
extern HD_RESULT vproc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id);
extern HD_RESULT vproc_unbind(HD_OUT_ID _out_id);
extern HD_RESULT vproc_set_cfg(IACOMM_VPROC_CFG *pvproc_cfg);
extern HD_RESULT vproc_set_param_in(IACOMM_VPROC_PARAM_IN *pvproc_param_in);
extern HD_RESULT vproc_set_param(IACOMM_VPROC_PARAM *pvproc_param);
extern HD_RESULT vproc_set_param_ex(IACOMM_VPROC_PARAM_EX *pvproc_param_ex);
extern HD_RESULT vproc_set_state(HD_PATH_ID path_id, UINT32 state);
extern HD_RESULT vproc_get_ctrl_path(UINT32 id, HD_PATH_ID *p_ctrl_path);

#endif//IMAGEAPP_COMMON_INT_H
