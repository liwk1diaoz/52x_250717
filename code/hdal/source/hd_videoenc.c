/**
	@brief Source code of video encoder module.\n
	This file contains the functions which is related to video encoder in the chip.

	@file hd_videoenc.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

// TODO: temp for [520] build
#ifdef _BSP_NA51000_
#undef _BSP_NA51000_
#endif
#ifndef _BSP_NA51055_
#define _BSP_NA51055_
#endif

#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME HD_VIDEOENC
#include "hd_int.h"
#include "hd_logger_p.h"
#include "kflow_videoenc/isf_vdoenc.h"
#include "kflow_videoenc/media_def.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/nvtmpp_ioctl.h"

// INCLUDE_PROTECTED
#if defined(_BSP_NA51000_)
#include "dal_vdoenc.h"
#include "h26x_def.h"
#endif
#include "video_encode.h"
#include "nmediarec_vdoenc.h"


/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define HD_VIDEOENC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE        ISF_UNIT_VDOENC
#define DEV_COUNT       1 //ISF_MAX_VDOENC
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        16
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       16

#define HD_DEV_BASE	HD_DAL_VIDEOENC_BASE
#define HD_DEV_MAX	HD_DAL_VIDEOENC_MAX

#define KFLOW_VIDEOENC_PARAM_BEGIN     VDOENC_PARAM_START
#define KFLOW_VIDEOENC_PARAM_END       __VDOENC_PARAM_MAX__
#define KFLOW_VIDEOENC_PARAM_NUM      (KFLOW_VIDEOENC_PARAM_END - KFLOW_VIDEOENC_PARAM_BEGIN)

#define FLOW_VIDEOENC_META_ALLOC_NUM   NMI_VDOENC_META_ALLOC_INFO_MAX

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
//                                            set/get             set/get/start
//                                              ^ |                  ^ |
//                                              | v                  | v
//  [UNINIT] -- init   --> [INIT] -- open  --> [OPEN]  -- start --> [START]
//          <-- uninit --        <-- close --         <-- stop  --
//
typedef enum _HD_VIDEOENC_STAT {
	HD_VIDEOENC_STATUS_UNINIT = 0,
	HD_VIDEOENC_STATUS_INIT,
	HD_VIDEOENC_STATUS_OPEN,
	HD_VIDEOENC_STATUS_START,
} HD_VIDEOENC_STAT;

typedef struct _VDOENC_INFO_PRIV_VENDOR {
	UINT32                  rec_format;
	BOOL                    b_jpg_vbr_mode;
	UINT32                  jpg_vbr_mode;
	MP_VDOENC_RDO_INFO      rdo_info;
	MP_VDOENC_JND_INFO      jnd_info;
	UINT32                  h26x_vbr_policy;
	BOOL                    b_jpg_yuv_trans;
	UINT32                  jpg_yuv_trans_en;
	UINT32                  jpg_rc_min_q;
	UINT32                  jpg_rc_max_q;
	UINT32                  h26x_rc_gop;
	UINT32                  min_i_ratio;
	UINT32                  min_p_ratio;
	UINT32                  h26x_colmv_en;
	UINT32                  h26x_comm_recfrm_en;
	UINT32                  h26x_comm_recfrm_base_en;
	UINT32                  h26x_comm_recfrm_svc_en;
	UINT32                  h26x_comm_recfrm_ltr_en;
	BOOL                    b_fit_work_memory;
	UINT32                  long_start_code;
	UINT32                  h26x_svc_weight_mode;
	BOOL                    b_bs_quick_rollback;
	BOOL                    b_quality_base_en;
	BOOL                    hw_padding_mode;
	UINT32                  br_tolerance;
	BOOL                    b_comm_srcout_en;
	BOOL                    b_comm_srcout_no_jpg_enc;
	UINT32                  comm_srcout_addr;
	UINT32                  comm_srcout_size;
	BOOL                    b_timer_trig_comp_en;
	BOOL                    b_h26x_sei_chksum;
	MP_VDOENC_GDR_INFO      gdr_info;
	BOOL                    b_h26x_low_power;
	MP_VDOENC_FRM_CROP_INFO h26x_crop_info;
	BOOL                    b_h26x_maq_diff_en;
	UINT32                  h26x_maq_diff_str;
	UINT32                  h26x_maq_diff_start_idx;
	UINT32                  h26x_maq_diff_end_idx;
	BOOL                    b_jpg_jfif;
	UINT32                  jpg_jfif_en;
	NMI_VDOENC_H26X_SKIP_FRM_INFO skip_frm_info;
	NMI_VDOENC_META_ALLOC_INFO *meta_alloc_info;
	UINT32                  meta_alloc_num;
} VDOENC_INFO_PRIV_VENDOR;

typedef struct _VDOENC_INFO_PRIV {
	HD_VIDEOENC_STAT        status;                     ///< hd_videoenc_status
	BOOL                    b_maxmem_set;
	BOOL                    b_need_update_param[KFLOW_VIDEOENC_PARAM_NUM];
	BOOL                    b_open_from_init;
	VDOENC_INFO_PRIV_VENDOR vendor_set;
} VDOENC_INFO_PRIV;

typedef struct _VDOENC_INFO_PORT {
	HD_VIDEOENC_BUFINFO       enc_buf_info;
	HD_VIDEOENC_PATH_CONFIG   enc_path_cfg;
	HD_VIDEOENC_FUNC_CONFIG   enc_func_cfg;
	HD_VIDEOENC_IN            enc_in_dim;
	HD_VIDEOENC_OUT2          enc_out_param;
	HD_H26XENC_VUI            enc_vui;
	HD_H26XENC_DEBLOCK        enc_deblock;
	HD_H26XENC_RATE_CONTROL2  enc_rc;
	HD_H26XENC_USR_QP         enc_usr_qp;
	HD_H26XENC_SLICE_SPLIT    enc_slice_split;
	HD_H26XENC_GDR            enc_gdr;
	HD_H26XENC_ROI            enc_roi;
	HD_H26XENC_ROW_RC         enc_row_rc;
	HD_H26XENC_AQ             enc_aq;
	HD_H26XENC_REQUEST_IFRAME request_keyframe;
	HD_H26XENC_TRIG_SNAPSHOT  trig_snapshot;
	HD_VIDEOENC_BS_RING 	  enc_bs_ring;

	//private data
	VDOENC_INFO_PRIV          priv;
} VDOENC_INFO_PORT;

typedef struct _VDOENC_INFO {
	VDOENC_INFO_PORT *port;
} VDOENC_INFO;

static UINT32 _hd_venc_csz = 0; //context buffer size

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                    */
/*-----------------------------------------------------------------------------*/
static UINT32 _max_path_count = 0;
extern int nvtmpp_hdl;

#define _HD_CONVERT_SELF_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_DEV;	\
		if((dev_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) { \
			UINT32 id = (dev_id) - HD_DEV_BASE; \
			if(id < DEV_COUNT) { \
				(dev_id) = DEV_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_OUT_ID(out_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((out_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((out_id) >= HD_OUT_BASE && (out_id) <= HD_OUT_MAX) { \
			UINT32 id = (out_id) - HD_OUT_BASE; \
			if((id < OUT_COUNT) && (id < _max_path_count)) { \
				(out_id) = OUT_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_DEST_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_NOT_SUPPORT; \
		if((dev_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if(((dev_id) >= HD_DAL_VIDEOENC_BASE) && ((dev_id) <= HD_DAL_VIDEOENC_MAX)) { \
			(rv) = _hd_videoenc_convert_dev_id(&(dev_id)); \
		} else if(((dev_id) >= HD_DAL_VIDEOOUT_BASE) && ((dev_id) <= HD_DAL_VIDEOOUT_MAX)) { \
			(rv) = _hd_videoout_convert_dev_id(&(dev_id)); \
		} else if(((dev_id) >= HD_DAL_VIDEOPROC_BASE) && ((dev_id) <= HD_DAL_VIDEOPROC_MAX)) { \
			(rv) = _hd_videoproc_convert_dev_id(&(dev_id)); \
		} \
	} while(0)

#define _HD_CONVERT_IN_ID(in_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((in_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((in_id) >= HD_IN_BASE && (in_id) <= HD_IN_MAX) { \
			UINT32 id = (in_id) - HD_IN_BASE; \
			if((id < IN_COUNT) && (id < _max_path_count)) { \
				(in_id) = IN_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_DEST_IN_ID(dev_id, in_id, rv) \
	do { \
		(rv) = HD_ERR_NOT_SUPPORT; \
		if((dev_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if(((dev_id) >= HD_DAL_VIDEOENC_BASE) && ((dev_id) <= HD_DAL_VIDEOENC_MAX)) { \
			(rv) = _hd_videoenc_convert_in_id(&(in_id)); \
		} else if(((dev_id) >= HD_DAL_VIDEOOUT_BASE) && ((dev_id) <= HD_DAL_VIDEOOUT_MAX)) { \
			(rv) = _hd_videoout_convert_in_id(&(in_id)); \
		} else if(((dev_id) >= HD_DAL_VIDEOPROC_BASE) && ((dev_id) <= HD_DAL_VIDEOPROC_MAX)) { \
			(rv) = _hd_videoproc_convert_in_id(&(in_id)); \
		} \
	} while(0)

#define _HD_CONVERT_OSG_ID(osg_id) \
	do { \
		if((osg_id) == 0) { \
			(osg_id) = 0; \
		} if((osg_id) >= HD_STAMP_BASE && (osg_id) <= HD_MASK_EX_MAX) { \
			if((osg_id) >= HD_STAMP_BASE && (osg_id) <= HD_STAMP_MAX) { \
				UINT32 id = (osg_id) - HD_STAMP_BASE; \
				(osg_id) = 0x00010000 | id; \
			} else if((osg_id) >= HD_STAMP_EX_BASE && (osg_id) <= HD_STAMP_EX_MAX) { \
				UINT32 id = (osg_id) - HD_STAMP_EX_BASE; \
				(osg_id) = 0x00020000 | id; \
			} else if((osg_id) >= HD_MASK_BASE && (osg_id) <= HD_MASK_MAX) { \
				UINT32 id = (osg_id) - HD_MASK_BASE; \
				(osg_id) = 0x00040000 | id; \
			} else if((osg_id) >= HD_MASK_EX_BASE && (osg_id) <= HD_MASK_EX_MAX) { \
				UINT32 id = (osg_id) - HD_MASK_EX_BASE; \
				(osg_id) = 0x00080000 | id; \
			} else { \
				(osg_id) = 0; \
			} \
		} else { \
			(osg_id) = 0; \
		} \
	} while(0)

#define _HD_REVERT_BIND_ID(dev_id) \
	do { \
		if(((dev_id) >= ISF_UNIT_VDOCAP) && ((dev_id) < ISF_UNIT_VDOCAP + ISF_MAX_VDOCAP)) { \
			(dev_id) = HD_DAL_VIDEOCAP_BASE + (dev_id) - ISF_UNIT_VDOCAP; \
		} else if(((dev_id) >= ISF_UNIT_VDOPRC) && ((dev_id) < ISF_UNIT_VDOPRC + ISF_MAX_VDOPRC)) { \
			(dev_id) = HD_DAL_VIDEOPROC_BASE + (dev_id) - ISF_UNIT_VDOPRC; \
		} else if(((dev_id) >= ISF_UNIT_VDOOUT) && ((dev_id) < ISF_UNIT_VDOOUT + ISF_MAX_VDOOUT)) { \
			(dev_id) = HD_DAL_VIDEOOUT_BASE + (dev_id) - ISF_UNIT_VDOOUT;\
		} else if(((dev_id) >= ISF_UNIT_VDOENC) && ((dev_id) < ISF_UNIT_VDOENC + ISF_MAX_VDOENC)) { \
			(dev_id) = HD_DAL_VIDEOENC_BASE + (dev_id) - ISF_UNIT_VDOENC; \
		} \
	} while(0)

#define _HD_REVERT_IN_ID(in_id) \
	do { \
		(in_id) = HD_IN_BASE + (in_id) - ISF_IN_BASE; \
	} while(0)

#define _HD_REVERT_OUT_ID(out_id) \
	do { \
		(out_id) = HD_OUT_BASE + (out_id) - ISF_OUT_BASE; \
	} while(0)

#define TIMESTAMP_TO_MS(ts)  ((ts >> 32) * 1000 + (ts & 0xFFFFFFFF) / 1000)

#if defined(__LINUX)
//#include <sys/types.h>      /* key_t, sem_t, pid_t      */
//#include <sys/shm.h>        /* shmat(), IPC_RMID        */
#include <errno.h>          /* errno, ECHILD            */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>          /* O_CREAT, O_EXEC          */

static int    g_shmid = 0;
#define SHM_KEY HD_DAL_VIDEOENC_BASE
static sem_t *g_sem = 0;
#define SEM_NAME "sem_venc"

static void* malloc_ctx(size_t buf_size)
{
	char   *g_shm = NULL;

	if (!_hd_common_is_new_flow()) {
		return malloc(buf_size);
	}

    // create the share-mem.
	if (_hd_common_get_pid() == 0) {
		if( ( g_shmid = shmget( SHM_KEY, buf_size, IPC_CREAT | 0666 ) ) < 0 ) {
			perror( "shmget" );
			exit(1);
		}
		if( ( g_shm = shmat( g_shmid, NULL, 0 ) ) == (char *)-1 ) {
			perror( "shmat" );
			exit(1);
		}
	} else {
	    if( ( g_shmid = shmget( SHM_KEY, buf_size, 0666 ) ) < 0 ) {
	        perror( "shmget" );
	        exit(1);
	    }
		if( ( g_shm = shmat( g_shmid, NULL, 0 ) ) == (char *)-1 ) {
			perror( "shmat" );
			exit(1);
		}
	}

    // create the semaphore.
	mode_t mask = umask(0);
	if (_hd_common_get_pid() == 0) {
		g_sem = sem_open (SEM_NAME, O_EXCL, 0666, 1);
		if (g_sem == 0) {
			g_sem = sem_open (SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
			if (g_sem == 0) {
				perror( "sem_open" );
				exit(1);
			}
		}
	} else {
		g_sem = sem_open (SEM_NAME, O_EXCL, 0666, 1);
		if (g_sem == 0) {
			perror( "sem_open" );
			exit(1);
		}
	}
	umask(mask);
	return g_shm;
}

static void free_ctx(void* ptr)
{
	if (!_hd_common_is_new_flow()) {
		free(ptr); return;
	}

	if (!ptr) {
		return;
	}

    // destory the semaphore.
	if (_hd_common_get_pid() == 0) {
		sem_close(g_sem);
		// sem_unlink can only be called on main process
		sem_unlink (SEM_NAME);
	} else {
		sem_close(g_sem);
	}

    // destory the share-mem.
	if (_hd_common_get_pid() == 0) {
		shmdt(ptr);
		shmctl(g_shmid, IPC_RMID, NULL);
	} else {
		shmdt(ptr);
	}
}

static void lock_ctx(void)
{
	if (!_hd_common_is_new_flow()) {
		return;
	}

	if (g_sem) {
		sem_wait (g_sem);
	}
}

static void unlock_ctx(void)
{
	if (!_hd_common_is_new_flow()) {
		return;
	}

	if (g_sem) {
		sem_post (g_sem);
	}
}

#else

static void* malloc_ctx(size_t buf_size) {return malloc(buf_size);}
static void free_ctx(void* ptr) { free(ptr); }
static void lock_ctx(void) {}
static void unlock_ctx(void) {}

#endif

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/
int _hd_videoenc_is_init(VOID);

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
VDOENC_INFO videoenc_info[DEV_COUNT] = {0};

static UINT8 g_cfg_cpu = 0;
static UINT8 g_cfg_debug = 0;
static UINT8 g_cfg_venc = 0;
static UINT8 g_cfg_vprc = 0;
#if defined(_BSP_NA51055_)
static UINT8 g_cfg_ai = 0;
#endif

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

void _hd_videoenc_cfg_max(UINT32 maxpath)
{
	if (!videoenc_info[0].port) {
		_max_path_count = maxpath;
		if (_max_path_count > OUT_COUNT) {
			DBG_WRN("dts max_path=%lu is larger than built-in max_path=%lu!\r\n", (unsigned long)(_max_path_count), (unsigned long)((UINT32)OUT_COUNT));
			_max_path_count = OUT_COUNT;
		}
		if (_max_path_count > VDOENC_MAX_PATH_NUM) {
			DBG_WRN("dts max_path=%lu is larger than built-in max_path=%lu!\r\n", (unsigned long)(_max_path_count), (unsigned long)((UINT32)VDOENC_MAX_PATH_NUM));
			_max_path_count = VDOENC_MAX_PATH_NUM;
		}
	}
}

UINT32 _hd_videoenc_codec_hdal2unit(HD_VIDEO_CODEC h_codec)
{
	switch (h_codec)
	{
		case HD_CODEC_TYPE_JPEG:    return MEDIAVIDENC_MJPG;
		case HD_CODEC_TYPE_H264:    return MEDIAVIDENC_H264;
		case HD_CODEC_TYPE_H265:    return MEDIAVIDENC_H265;
		default:
			DBG_ERR("unknown codec type(%d)\r\n", (int)(h_codec));
			return (-1);
	}
}

UINT32 _hd_videoenc_codec_unit2hdal(UINT32 u_codec)
{
	switch (u_codec)
	{
		case MEDIAVIDENC_MJPG:    return HD_CODEC_TYPE_JPEG;
		case MEDIAVIDENC_H264:    return HD_CODEC_TYPE_H264;
		case MEDIAVIDENC_H265:    return HD_CODEC_TYPE_H265;
		default:
			DBG_ERR("unknown codec type(%lu)\r\n", (unsigned long)(u_codec));
			return (-1);
	}
}

UINT32 _hd_videoenc_frametype_hdal2unit(UINT32 h_ftype)
{
	switch (h_ftype)
	{
#if defined(_BSP_NA51000_)
		case HD_FRAME_TYPE_IDR:    return IDR_SLICE;
		case HD_FRAME_TYPE_I:      return I_SLICE;
		case HD_FRAME_TYPE_P:      return P_SLICE;
		case HD_FRAME_TYPE_KP:     return 4;
#elif defined(_BSP_NA51055_)
		case HD_FRAME_TYPE_IDR:    return MP_VDOENC_IDR_SLICE;
		case HD_FRAME_TYPE_I:      return MP_VDOENC_I_SLICE;
		case HD_FRAME_TYPE_P:      return MP_VDOENC_P_SLICE;
		case HD_FRAME_TYPE_KP:     return MP_VDOENC_KP_SLICE;
#endif
		default:
			DBG_ERR("unknown frame type(%lu)\r\n", (unsigned long)(h_ftype));
			return (-1);
	}
}

UINT32 _hd_videoenc_frametype_unit2hdal(UINT32 u_ftype)
{
	switch (u_ftype)
	{
#if defined(_BSP_NA51000_)
		case IDR_SLICE:    return HD_FRAME_TYPE_IDR;
		case I_SLICE:      return HD_FRAME_TYPE_I;
		case P_SLICE:      return HD_FRAME_TYPE_P;
		case 4:            return HD_FRAME_TYPE_KP;
#elif defined(_BSP_NA51055_)
		case MP_VDOENC_IDR_SLICE:    return HD_FRAME_TYPE_IDR;
		case MP_VDOENC_I_SLICE:      return HD_FRAME_TYPE_I;
		case MP_VDOENC_P_SLICE:      return HD_FRAME_TYPE_P;
		case MP_VDOENC_KP_SLICE:     return HD_FRAME_TYPE_KP;
#endif
		default:
			DBG_ERR("unknown frame type(%lu)\r\n", (unsigned long)(u_ftype));
			return (-1);
	}
}

UINT32 _hd_videoenc_profile_hdal2unit(HD_VIDEOENC_PROFILE h_profile)
{
	switch (h_profile)
	{
		case HD_H264E_BASELINE_PROFILE:      return MP_VDOENC_PROFILE_BASELINE;

		case HD_H264E_MAIN_PROFILE:
		case HD_H265E_MAIN_PROFILE:          return MP_VDOENC_PROFILE_MAIN;

		case HD_H264E_HIGH_PROFILE:          return MP_VDOENC_PROFILE_HIGH;
		default:
			DBG_ERR("unknown profile(%d)\r\n", (int)(h_profile));
			return (-1);
	}
}

UINT32 _hd_videoenc_svc_hdal2unit(HD_VIDEOENC_SVC_LAYER h_svc)
{
	switch (h_svc)
	{
		case HD_SVC_DISABLE:      return MP_VDOENC_SVC_DISABLE;
		case HD_SVC_2X:           return MP_VDOENC_SVC_LAYER1;
		case HD_SVC_4X:           return MP_VDOENC_SVC_LAYER2;
		default:
			DBG_ERR("unknown svc(%d)\r\n", (int)(h_svc));
			return (-1);
	}
}

HD_RESULT _hd_videoenc_convert_dev_id(HD_DAL* p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

HD_RESULT _hd_videoenc_convert_in_id(HD_IO* p_in_id)
{
	HD_RESULT rv;
	_HD_CONVERT_IN_ID(p_in_id[0], rv);
	return rv;
}

int _hd_videoenc_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "VIDEOENC_%d_IN_%d", dev_id - HD_DEV_BASE, in_id - HD_IN_BASE);
}

int _hd_videoenc_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "VIDEOENC_%d_OUT_%d", dev_id - HD_DEV_BASE, out_id - HD_OUT_BASE);
}

HD_RESULT _hd_get_param(HD_DAL self_id, HD_IO out_id, UINT32 param, UINT32 *value)
{
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_RESULT rv;
	int r;

	cmd.dest = ISF_PORT(self_id, out_id);
	cmd.param = param;
	cmd.size = 0;

	r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:
			rv = HD_OK;
			*value = cmd.value;
			break;
		default: rv = HD_ERR_SYS; break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

HD_RESULT _hd_get_struct_param(HD_DAL self_id, HD_IO out_id, UINT32 param, UINT32 *value, UINT32 size)
{
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_RESULT rv;
	int r;

	cmd.dest = ISF_PORT(self_id, out_id);
	cmd.param = param;
	cmd.value = (UINT32)value;
	cmd.size = size;

	r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:
			rv = HD_OK;
			break;
		default: rv = HD_ERR_SYS; break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

void _hd_videoenc_wrap_bitstream(HD_VIDEOENC_BS* p_video_bitstream, ISF_DATA *p_data)
{
	PVDO_BITSTREAM p_vdobs = (VDO_BITSTREAM *) p_data->desc;

	p_video_bitstream->sign           = MAKEFOURCC('V','S','T','M');  // let HDAL user feel sign = 'V','S','T','M'
	p_video_bitstream->blk            = p_data->h_data;  //isf-data-handle
	p_video_bitstream->vcodec_format  = _hd_videoenc_codec_unit2hdal(p_vdobs->CodecType);
	p_video_bitstream->timestamp      = p_vdobs->timestamp;
	p_video_bitstream->svc_layer_type = p_vdobs->resv[9];  // jeah: TODO?
	p_video_bitstream->frame_type     = _hd_videoenc_frametype_unit2hdal(p_vdobs->resv[6]);
	p_video_bitstream->qp             = p_vdobs->resv[15];
	p_video_bitstream->motion_ratio   = p_vdobs->resv[16];
	p_video_bitstream->psnr_info.y_mse = p_vdobs->resv[17];
	p_video_bitstream->psnr_info.u_mse = p_vdobs->resv[18];
	p_video_bitstream->psnr_info.v_mse = p_vdobs->resv[19];

	p_video_bitstream->p_next = (HD_METADATA*)p_vdobs->resv[20]; // nalu info
	p_video_bitstream->evbr_state      = (HD_VIDEO_EVBR_STATE)p_vdobs->resv[21];

	//if (p_video_bitstream->frame_type == HD_FRAME_TYPE_IDR) {      // TODO: will I-Frame with Desc ???
	if (p_vdobs->resv[10] == 1) {  // is_key
		switch (p_video_bitstream->vcodec_format) {
			case HD_CODEC_TYPE_JPEG:
				p_video_bitstream->pack_num = 1;
				p_video_bitstream->video_pack[0].pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;
				p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[11];
				p_video_bitstream->video_pack[0].size                = p_vdobs->DataSize;
				break;
			case HD_CODEC_TYPE_H264:
				p_video_bitstream->pack_num = 3;  // sps + pps + bs
				p_video_bitstream->video_pack[0].pack_type.h264_type = H264_NALU_TYPE_SPS;
				p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[12];
				p_video_bitstream->video_pack[0].size                = p_vdobs->resv[1];
				p_video_bitstream->video_pack[1].pack_type.h264_type = H264_NALU_TYPE_PPS;
				p_video_bitstream->video_pack[1].phy_addr            = p_vdobs->resv[13];
				p_video_bitstream->video_pack[1].size                = p_vdobs->resv[3];
				p_video_bitstream->video_pack[2].pack_type.h264_type = H264_NALU_TYPE_IDR;
				p_video_bitstream->video_pack[2].phy_addr            = p_vdobs->resv[11];
				p_video_bitstream->video_pack[2].size                = p_vdobs->DataSize;
				break;
			case HD_CODEC_TYPE_H265:
				p_video_bitstream->pack_num = 4;  // vps + sps + pps + bs
				p_video_bitstream->video_pack[0].pack_type.h265_type = H265_NALU_TYPE_VPS;
				p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[14];
				p_video_bitstream->video_pack[0].size                = p_vdobs->resv[5];
				p_video_bitstream->video_pack[1].pack_type.h265_type = H265_NALU_TYPE_SPS;
				p_video_bitstream->video_pack[1].phy_addr            = p_vdobs->resv[12];
				p_video_bitstream->video_pack[1].size                = p_vdobs->resv[1];
				p_video_bitstream->video_pack[2].pack_type.h265_type = H265_NALU_TYPE_PPS;
				p_video_bitstream->video_pack[2].phy_addr            = p_vdobs->resv[13];
				p_video_bitstream->video_pack[2].size                = p_vdobs->resv[3];
				p_video_bitstream->video_pack[3].pack_type.h265_type = H265_NALU_TYPE_IDR;
				p_video_bitstream->video_pack[3].phy_addr            = p_vdobs->resv[11];
				p_video_bitstream->video_pack[3].size                = p_vdobs->DataSize;
				break;
			default:
				DBG_ERR("unknown vcodec_format(%d)\r\n", (int)(p_video_bitstream->vcodec_format));
				break;
		}
	} else {
		p_video_bitstream->pack_num = 1;
		p_video_bitstream->video_pack[0].phy_addr  = p_vdobs->resv[11];
		p_video_bitstream->video_pack[0].size      = p_vdobs->DataSize;

		switch (p_video_bitstream->vcodec_format) {
			case HD_CODEC_TYPE_JPEG:  p_video_bitstream->video_pack[0].pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;    break;
			case HD_CODEC_TYPE_H264:  p_video_bitstream->video_pack[0].pack_type.h264_type = H264_NALU_TYPE_SLICE;  break;
			case HD_CODEC_TYPE_H265:  p_video_bitstream->video_pack[0].pack_type.h265_type = H265_NALU_TYPE_SLICE;  break;
			default:
				DBG_ERR("unknown vcodec_format(%d)\r\n", (int)(p_video_bitstream->vcodec_format));
				break;
		}
	}
}

void _hd_videoenc_rebuild_bitstream(ISF_DATA *p_data, HD_VIDEOENC_BS* p_video_bitstream)
{
	PVDO_BITSTREAM p_vdobs = (VDO_BITSTREAM *) p_data->desc;

	p_data->sign   = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_bitstream->blk; //isf-data-handle

	p_vdobs->sign  = MAKEFOURCC('V','E','T','M');  // assign this specfic sign, so isf_flow can rebuild p_class for kflow_videoenc to lock/unlock
}

void _hd_videoenc_wrap_bitstream3(HD_VIDEOENC_BS3* p_video_bitstream, ISF_DATA *p_data)
{
	PVDO_BITSTREAM p_vdobs = (VDO_BITSTREAM *) p_data->desc;

	p_video_bitstream->sign           = MAKEFOURCC('V','S','T','M');  // let HDAL user feel sign = 'V','S','T','M'
	p_video_bitstream->blk            = p_data->h_data;  //isf-data-handle
	p_video_bitstream->blk2           = p_vdobs->resv[4];
	p_video_bitstream->vcodec_format  = _hd_videoenc_codec_unit2hdal(p_vdobs->CodecType);
	p_video_bitstream->timestamp      = p_vdobs->timestamp;
	p_video_bitstream->svc_layer_type = p_vdobs->resv[9];  // jeah: TODO?
	p_video_bitstream->frame_type     = _hd_videoenc_frametype_unit2hdal(p_vdobs->resv[6]);
	p_video_bitstream->qp             = p_vdobs->resv[15];
	p_video_bitstream->motion_ratio   = p_vdobs->resv[16];
	p_video_bitstream->psnr_info.y_mse = p_vdobs->resv[17];
	p_video_bitstream->psnr_info.u_mse = p_vdobs->resv[18];
	p_video_bitstream->psnr_info.v_mse = p_vdobs->resv[19];

	p_video_bitstream->p_next = (HD_METADATA*)p_vdobs->resv[20]; // nalu info
	p_video_bitstream->evbr_state      = (HD_VIDEO_EVBR_STATE)p_vdobs->resv[23];

	//if (p_video_bitstream->frame_type == HD_FRAME_TYPE_IDR) {      // TODO: will I-Frame with Desc ???
	if (p_vdobs->resv[10] == 1) {  // is_key
		switch (p_video_bitstream->vcodec_format) {
			case HD_CODEC_TYPE_JPEG:
				if (p_vdobs->resv[0] == 0 && p_vdobs->resv[2] == 0) {
					p_video_bitstream->pack_num = 1;
					p_video_bitstream->video_pack[0].pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;
					p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[11];
					p_video_bitstream->video_pack[0].size                = p_vdobs->DataSize;
				} else {
					p_video_bitstream->pack_num = 2;
					p_video_bitstream->video_pack[0].pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;
					p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[11];
					p_video_bitstream->video_pack[0].size                = p_vdobs->DataSize;
					p_video_bitstream->video_pack[1].pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;
					p_video_bitstream->video_pack[1].phy_addr            = p_vdobs->resv[0];
					p_video_bitstream->video_pack[1].size                = p_vdobs->resv[2];
				}
				break;
			case HD_CODEC_TYPE_H264: {
					if (p_vdobs->resv[0] == 0 && p_vdobs->resv[2] == 0) {
						p_video_bitstream->pack_num = 3;  // sps + pps + bs
						p_video_bitstream->video_pack[0].pack_type.h264_type = H264_NALU_TYPE_SPS;
						p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[12];
						p_video_bitstream->video_pack[0].size                = p_vdobs->resv[1];
						p_video_bitstream->video_pack[1].pack_type.h264_type = H264_NALU_TYPE_PPS;
						p_video_bitstream->video_pack[1].phy_addr            = p_vdobs->resv[13];
						p_video_bitstream->video_pack[1].size                = p_vdobs->resv[3];
						p_video_bitstream->video_pack[2].pack_type.h264_type = H264_NALU_TYPE_IDR;
						p_video_bitstream->video_pack[2].phy_addr            = p_vdobs->resv[11];
						p_video_bitstream->video_pack[2].size                = p_vdobs->DataSize;
					} else {
						p_video_bitstream->pack_num = 4;  // sps + pps + bs1+bs2
						p_video_bitstream->video_pack[0].pack_type.h264_type = H264_NALU_TYPE_SPS;
						p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[12];
						p_video_bitstream->video_pack[0].size                = p_vdobs->resv[1];
						p_video_bitstream->video_pack[1].pack_type.h264_type = H264_NALU_TYPE_PPS;
						p_video_bitstream->video_pack[1].phy_addr            = p_vdobs->resv[13];
						p_video_bitstream->video_pack[1].size                = p_vdobs->resv[3];
						p_video_bitstream->video_pack[2].pack_type.h264_type = H264_NALU_TYPE_IDR;
						p_video_bitstream->video_pack[2].phy_addr            = p_vdobs->resv[11];
						p_video_bitstream->video_pack[2].size                = p_vdobs->DataSize;
						p_video_bitstream->video_pack[3].pack_type.h264_type = H264_NALU_TYPE_IDR;
						p_video_bitstream->video_pack[3].phy_addr            = p_vdobs->resv[0];
						p_video_bitstream->video_pack[3].size                = p_vdobs->resv[2];
					}
				}
				break;
			case HD_CODEC_TYPE_H265: {
					if (p_vdobs->resv[0] == 0 && p_vdobs->resv[2] == 0) {

						p_video_bitstream->pack_num = 4;  // vps + sps + pps + bs
						p_video_bitstream->video_pack[0].pack_type.h265_type = H265_NALU_TYPE_VPS;
						p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[14];
						p_video_bitstream->video_pack[0].size                = p_vdobs->resv[5];
						p_video_bitstream->video_pack[1].pack_type.h265_type = H265_NALU_TYPE_SPS;
						p_video_bitstream->video_pack[1].phy_addr            = p_vdobs->resv[12];
						p_video_bitstream->video_pack[1].size                = p_vdobs->resv[1];
						p_video_bitstream->video_pack[2].pack_type.h265_type = H265_NALU_TYPE_PPS;
						p_video_bitstream->video_pack[2].phy_addr            = p_vdobs->resv[13];
						p_video_bitstream->video_pack[2].size                = p_vdobs->resv[3];
						p_video_bitstream->video_pack[3].pack_type.h265_type = H265_NALU_TYPE_IDR;
						p_video_bitstream->video_pack[3].phy_addr            = p_vdobs->resv[11];
						p_video_bitstream->video_pack[3].size                = p_vdobs->DataSize;
					} else {
						p_video_bitstream->pack_num = 5;  // vps + sps + pps + bs1 + bs2
						p_video_bitstream->video_pack[0].pack_type.h265_type = H265_NALU_TYPE_VPS;
						p_video_bitstream->video_pack[0].phy_addr            = p_vdobs->resv[14];
						p_video_bitstream->video_pack[0].size                = p_vdobs->resv[5];
						p_video_bitstream->video_pack[1].pack_type.h265_type = H265_NALU_TYPE_SPS;
						p_video_bitstream->video_pack[1].phy_addr            = p_vdobs->resv[12];
						p_video_bitstream->video_pack[1].size                = p_vdobs->resv[1];
						p_video_bitstream->video_pack[2].pack_type.h265_type = H265_NALU_TYPE_PPS;
						p_video_bitstream->video_pack[2].phy_addr            = p_vdobs->resv[13];
						p_video_bitstream->video_pack[2].size                = p_vdobs->resv[3];
						p_video_bitstream->video_pack[3].pack_type.h265_type = H265_NALU_TYPE_IDR;
						p_video_bitstream->video_pack[3].phy_addr            = p_vdobs->resv[11];
						p_video_bitstream->video_pack[3].size                = p_vdobs->DataSize;
						p_video_bitstream->video_pack[4].pack_type.h265_type = H265_NALU_TYPE_IDR;
						p_video_bitstream->video_pack[4].phy_addr            = p_vdobs->resv[0];
						p_video_bitstream->video_pack[4].size                = p_vdobs->resv[2];
					}
				}
				break;
			default:
				DBG_ERR("unknown vcodec_format(%d)\r\n", (int)(p_video_bitstream->vcodec_format));
				break;
		}
	} else {
		if (p_vdobs->resv[0] == 0 && p_vdobs->resv[2] == 0) {
			p_video_bitstream->pack_num = 1;
			p_video_bitstream->video_pack[0].phy_addr  = p_vdobs->resv[11];
			p_video_bitstream->video_pack[0].size      = p_vdobs->DataSize;
		} else {
			p_video_bitstream->pack_num = 2;
			p_video_bitstream->video_pack[0].phy_addr  = p_vdobs->resv[11];
			p_video_bitstream->video_pack[0].size      = p_vdobs->DataSize;
			p_video_bitstream->video_pack[1].phy_addr  = p_vdobs->resv[0];
			p_video_bitstream->video_pack[1].size      = p_vdobs->resv[2];
		}

		switch (p_video_bitstream->vcodec_format) {
			case HD_CODEC_TYPE_JPEG:
				p_video_bitstream->video_pack[0].pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;
				if (p_video_bitstream->pack_num == 2) {
					p_video_bitstream->video_pack[1].pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;
				}
				break;
			case HD_CODEC_TYPE_H264:
				p_video_bitstream->video_pack[0].pack_type.h264_type = H264_NALU_TYPE_SLICE;
				if (p_video_bitstream->pack_num == 2) {
					p_video_bitstream->video_pack[1].pack_type.h264_type = H264_NALU_TYPE_SLICE;
				}
				break;
			case HD_CODEC_TYPE_H265:
				p_video_bitstream->video_pack[0].pack_type.h265_type = H265_NALU_TYPE_SLICE;
				if (p_video_bitstream->pack_num == 2) {
					p_video_bitstream->video_pack[1].pack_type.h265_type = H265_NALU_TYPE_SLICE;
				}
				break;
			default:
				DBG_ERR("unknown vcodec_format(%d)\r\n", (int)(p_video_bitstream->vcodec_format));
				break;
		}
	}
}

void _hd_videoenc_rebuild_bitstream3(ISF_DATA *p_data, HD_VIDEOENC_BS3* p_video_bitstream)
{
	PVDO_BITSTREAM p_vdobs = (VDO_BITSTREAM *) p_data->desc;

	p_data->sign   = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_bitstream->blk; //isf-data-handle

	p_vdobs->sign  = MAKEFOURCC('V','E','T','M');  // assign this specfic sign, so isf_flow can rebuild p_class for kflow_videoenc to lock/unlock
	p_vdobs->resv[0] = p_video_bitstream->blk2;
}

static INT _hd_videoenc_assign_param_enc_out_param_1_to_2(HD_VIDEOENC_OUT2 *enc_out2, HD_VIDEOENC_OUT *enc_out1)
{
	enc_out2->codec_type = enc_out1->codec_type;

	if (enc_out1->codec_type == HD_CODEC_TYPE_JPEG) {
		memset(&enc_out2->jpeg, 0, sizeof(HD_JPEG_CONFIG2));
		memcpy(&enc_out2->jpeg, &enc_out1->jpeg, sizeof(HD_JPEG_CONFIG));
	} else {
	    memcpy(&enc_out2->h26x, &enc_out1->h26x, sizeof(HD_H26X_CONFIG));
	}

	return 0;
}

static INT _hd_videoenc_assign_param_enc_out_param_2_to_1(HD_VIDEOENC_OUT *enc_out1, HD_VIDEOENC_OUT2 *enc_out2)
{
	enc_out1->codec_type = enc_out2->codec_type;

	if (enc_out2->codec_type == HD_CODEC_TYPE_JPEG) {
		memcpy(&enc_out1->jpeg, &enc_out2->jpeg, sizeof(HD_JPEG_CONFIG));
	} else {
	    memcpy(&enc_out1->h26x, &enc_out2->h26x, sizeof(HD_H26X_CONFIG));
	}

	return 0;
}

static INT _hd_videoenc_assign_param_rc_param_1_to_2(HD_H26XENC_RATE_CONTROL2 *rc_param2, HD_H26XENC_RATE_CONTROL *rc_param1)
{
	rc_param2->rc_mode = rc_param1->rc_mode;

	switch(rc_param1->rc_mode) {
		case HD_RC_MODE_CBR:
			memset(&rc_param2->cbr, 0, sizeof(HD_H26XENC_CBR2));
			memcpy(&rc_param2->cbr, &rc_param1->cbr, sizeof(HD_H26XENC_CBR));
			break;
		case HD_RC_MODE_VBR:
			memset(&rc_param2->vbr, 0, sizeof(HD_H26XENC_VBR2));
			memcpy(&rc_param2->vbr, &rc_param1->vbr, sizeof(HD_H26XENC_VBR));
			break;
		case HD_RC_MODE_FIX_QP:
			memcpy(&rc_param2->fixqp, &rc_param1->fixqp, sizeof(HD_H26XENC_FIXQP));
			break;
		case HD_RC_MODE_EVBR:
			memset(&rc_param2->evbr, 0, sizeof(HD_H26XENC_EVBR2));
			memcpy(&rc_param2->evbr, &rc_param1->evbr, sizeof(HD_H26XENC_EVBR));
			break;
		default:
			break;
	}
	return 0;
}

static INT _hd_videoenc_assign_param_rc_param_2_to_1(HD_H26XENC_RATE_CONTROL *rc_param1, HD_H26XENC_RATE_CONTROL2 *rc_param2)
{
	rc_param1->rc_mode = rc_param2->rc_mode;

	switch(rc_param2->rc_mode) {
		case HD_RC_MODE_CBR:
			memcpy(&rc_param1->cbr, &rc_param2->cbr, sizeof(HD_H26XENC_CBR));
			break;
		case HD_RC_MODE_VBR:
			memcpy(&rc_param1->vbr, &rc_param2->vbr, sizeof(HD_H26XENC_VBR));
			break;
		case HD_RC_MODE_FIX_QP:
			memcpy(&rc_param1->fixqp, &rc_param2->fixqp, sizeof(HD_H26XENC_FIXQP));
			break;
		case HD_RC_MODE_EVBR:
			memcpy(&rc_param1->evbr, &rc_param2->evbr, sizeof(HD_H26XENC_EVBR));
			break;
		default:
			break;
	}
	return 0;
}

static INT _hd_videoenc_verify_param_HD_VIDEOENC_PATH_CONFIG(HD_VIDEOENC_PATH_CONFIG *p_enc_pathcfg, CHAR *p_ret_string, INT max_str_len)
{
	HD_VIDEOENC_MAXMEM *p_maxmem = &p_enc_pathcfg->max_mem;
	UINT32 isp_id                = p_enc_pathcfg->isp_id;
	UINT32 ipp_id                = isp_id & 0xFF;

	if (p_maxmem->codec_type != HD_CODEC_TYPE_JPEG && p_maxmem->codec_type != HD_CODEC_TYPE_H264 &&
		p_maxmem->codec_type != HD_CODEC_TYPE_H265) {
		snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_PATH_CONFIG: codec_type(%d) is not supported.", p_maxmem->codec_type);
		goto exit;
	}

	if (p_maxmem->bitrate > 128*1024*1024) {
			snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PATH_CONFIG: bitrate(%lu) is out-of-range(0~128Mbit).", p_maxmem->bitrate);
			goto exit;
	}

	if (p_maxmem->svc_layer > 2) {
			snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PATH_CONFIG: svc_layer(%d) is out-of-range(0~2).", p_maxmem->svc_layer);
			goto exit;
	}

	if (p_maxmem->ltr > 1) {
			snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PATH_CONFIG: ltr(%u) is out-of-range(0~1).", p_maxmem->ltr);
			goto exit;
	}

	if (p_maxmem->rotate > 1) {
			snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PATH_CONFIG: rotate(%u) is out-of-range(0~1).", p_maxmem->rotate);
			goto exit;
	}

	if (p_maxmem->source_output > 1) {
			snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PATH_CONFIG: source_output(%u) is out-of-range(0~1).", p_maxmem->source_output);
			goto exit;
	}

	if ((ipp_id > 4) && (isp_id != 0xffffffff)) { // ipp_id range 0~4 in 52x, range 0~7 in 68x
			snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PATH_CONFIG: isp_id(0x%08x), ipp_id(%lu) is out-of-range(0~4) and is not IGNORE(0xffffffff).", (unsigned int)isp_id, ipp_id);
			goto exit;
	}

	return 0;
exit:
	return -1;

}

static INT _hd_videoenc_verify_param_HD_VIDEOENC_FUNC_CONFIG(HD_VIDEOENC_FUNC_CONFIG *p_enc_func_cfg, CHAR *p_ret_string, INT max_str_len)
{
	UINT32 stripe_rule = g_cfg_vprc & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
	UINT32 all_support_func = (HD_VIDEOENC_INFUNC_ONEBUF|HD_VIDEOENC_INFUNC_LOWLATENCY);

	if (p_enc_func_cfg->in_func & HD_VIDEOENC_INFUNC_LOWLATENCY && stripe_rule == 3) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_FUNC_CONFIG: low layency not allow STRP_LV4\r\n");
		goto exit;
	}

	if (p_enc_func_cfg->in_func & (~all_support_func)) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_FUNC_CONFIG: in_func(0x%08x) invalid, only (0x%08x) supported !!\r\n", (UINT)p_enc_func_cfg->in_func, (UINT)all_support_func);
		goto exit;
	}

	if (p_enc_func_cfg->ddr_id >= DDR_ID_MAX && p_enc_func_cfg->ddr_id != DDR_ID_AUTO) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_FUNC_CONFIG: ddr_id(%d) is out-of-range(0~%d).\r\n", (int)p_enc_func_cfg->ddr_id, (int)(DDR_ID_MAX-1));
		goto exit;
	}

	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_VIDEOENC_IN(HD_VIDEOENC_IN *p_enc_in, CHAR *p_ret_string, INT max_str_len)
{
	// pxl_fmt
	switch(p_enc_in->pxl_fmt) {
	case HD_VIDEO_PXLFMT_Y8:
	case HD_VIDEO_PXLFMT_YUV420:
	case HD_VIDEO_PXLFMT_YUV422:
#if defined(_BSP_NA51000_)
	case HD_VIDEO_PXLFMT_YUV420_NVX1_H264:
	case HD_VIDEO_PXLFMT_YUV420_NVX1_H265:
#elif defined(_BSP_NA51055_)
	case HD_VIDEO_PXLFMT_YUV420_NVX2:
#endif
		break;
	default:
		snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_IN: pxl_fmt(%d) is not supported.\n", p_enc_in->pxl_fmt);
		goto exit;
	}

	//dir
	switch (p_enc_in->dir) {
	case HD_VIDEO_DIR_NONE:
	case HD_VIDEO_DIR_ROTATE_90:
#if defined(_BSP_NA51055_)
	case HD_VIDEO_DIR_ROTATE_180:
#endif
	case HD_VIDEO_DIR_ROTATE_270:
		break;
	default:
		snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_IN: dir(%d) is not supported.\n", p_enc_in->dir);
		goto exit;
	}
#if 0  // this check move to _hd_videoenc_check_params_range_on_start(), because H26x & JPEG have different minium dim !!
	// dim
	if (p_enc_in->dim.w < 352) {
		snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_IN: dim.w(%lu) must >= 352 (minium dim = 352x288) !! \n", p_enc_in->dim.w);
		goto exit;
	}

	if (p_enc_in->dim.h < 288) {
		snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_IN: dim.h(%lu) must >= 288 (minium dim = 352x288) !! \n", p_enc_in->dim.h);
		goto exit;
	}
#endif

	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_VIDEOENC_OUT2(HD_VIDEOENC_OUT2 *p_enc_out, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_out->codec_type != HD_CODEC_TYPE_JPEG && p_enc_out->codec_type != HD_CODEC_TYPE_H264 &&
		p_enc_out->codec_type != HD_CODEC_TYPE_H265) {
		snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_OUT: codec_type(%d) is not supported.", p_enc_out->codec_type);
		goto exit;
	}
	if (p_enc_out->codec_type == HD_CODEC_TYPE_JPEG) {
		HD_JPEG_CONFIG2  *p_jpeg = &p_enc_out->jpeg;
#if 0
		if (p_jpeg->retstart_interval < 0) {
			snprintf(p_ret_string, max_str_len,
					"HD_JPEG_CONFIG: retstart_interval(%lu) is out-of-range(0~image_size/256).",
					p_jpeg->retstart_interval);
			goto exit;
		}
#endif
		if (p_jpeg->bitrate == 0) {
			// [fix quality] bitrate = 0 , image_quality = 1~100
			if (p_jpeg->image_quality < 1 || p_jpeg->image_quality > 100) {
				snprintf(p_ret_string, max_str_len,
						"HD_JPEG_CONFIG: bitrate(0)(fix_quality_mode), image_quality(%ld) is out-of-range(1~100).", p_jpeg->image_quality);
				goto exit;
			}
		}else{
			// [MOVIE cbr] bitrate > 0 , image_quality = 1~100 (init quality)
			if (p_jpeg->image_quality > 100) {
				snprintf(p_ret_string, max_str_len,
						"HD_JPEG_CONFIG: bitrate(%ld)(movie_cbr_mode), init_image_quality(%ld) is out-of-range(1~100).", p_jpeg->bitrate, p_jpeg->image_quality);
				goto exit;
			}
			// [PHOTO cbr] bitrate > 0 , image_quality = 0
		}
		if (p_jpeg->frame_rate_base > 0 && p_jpeg->frame_rate_incr == 0) {
			snprintf(p_ret_string, max_str_len,
					"HD_JPEG_CONFIG: frame_rate_incr(0) is 0.");
			goto exit;
		}
	} else {
		HD_H26X_CONFIG  *p_h26x = &p_enc_out->h26x;
		if (p_h26x->gop_num > 4096) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26X_CONFIG: gop_num(%lu) is out-of-range(0~4096).", p_h26x->gop_num);
			goto exit;
		}

		if (p_h26x->chrm_qp_idx < -12 || p_h26x->chrm_qp_idx > 12) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26X_CONFIG: chrm_qp_idx(%d) is out-of-range(-12~12).", p_h26x->chrm_qp_idx);
			goto exit;
		}
		if (p_h26x->sec_chrm_qp_idx < -12 || p_h26x->sec_chrm_qp_idx > 12) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26X_CONFIG: sec_chrm_qp_idx(%d) is out-of-range(-12~12).", p_h26x->sec_chrm_qp_idx);
			goto exit;
		}

		if (p_h26x->ltr_interval > 4095) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26X_CONFIG: ltr_interval(%lu) is out-of-range(0~4095).", p_h26x->ltr_interval);
			goto exit;
		}
		if (p_h26x->ltr_interval != 0) {
			if (p_h26x->ltr_pre_ref > 1) {
				snprintf(p_ret_string, max_str_len,
						"HD_H26X_CONFIG: ltr_pre_ref(%d) is out-of-range(0~1).", p_h26x->ltr_pre_ref);
				goto exit;
			}
		}
		if (p_h26x->gray_en > 1) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26X_CONFIG: gray_en(%d) is out-of-range(0~1).", p_h26x->gray_en);
			goto exit;
		}

		if (p_h26x->source_output > 1) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26X_CONFIG: source_output(%d) is out-of-range(0~1).", p_h26x->source_output);
			goto exit;
		}

		// Profile check
		if (p_enc_out->codec_type == HD_CODEC_TYPE_H264) {
			switch(p_h26x->profile) {
			case HD_H264E_BASELINE_PROFILE:
			case HD_H264E_MAIN_PROFILE:
			case HD_H264E_HIGH_PROFILE:
				break;
			default:
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_PROFILE: when H264, profile(%d) is not support.", p_h26x->profile);
					goto exit;
			}
		} else if (p_enc_out->codec_type == HD_CODEC_TYPE_H265) {
			switch(p_h26x->profile) {
			case HD_H265E_MAIN_PROFILE:
				break;
			default:
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_PROFILE: when H265, profile(%d) is not support.", p_h26x->profile);
					goto exit;
			}
		} else {
			snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_PROFILE: codec_type != H264/H265 is not support.");
			goto exit;
		}

		// Level check
		if (p_enc_out->codec_type == HD_CODEC_TYPE_H264) {
			switch(p_h26x->level_idc) {
			case HD_H264E_LEVEL_1: case HD_H264E_LEVEL_1_1: case HD_H264E_LEVEL_1_2: case HD_H264E_LEVEL_1_3:
			case HD_H264E_LEVEL_2: case HD_H264E_LEVEL_2_1: case HD_H264E_LEVEL_2_2:
			case HD_H264E_LEVEL_3: case HD_H264E_LEVEL_3_1: case HD_H264E_LEVEL_3_2:
			case HD_H264E_LEVEL_4: case HD_H264E_LEVEL_4_1: case HD_H264E_LEVEL_4_2:
			case HD_H264E_LEVEL_5: case HD_H264E_LEVEL_5_1:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
						"HD_VIDEOENC_LEVEL: when H264, level_idc(%d) is not supported.", p_h26x->level_idc);
				goto exit;
			}
		} else if (p_enc_out->codec_type == HD_CODEC_TYPE_H265) {
			switch(p_h26x->level_idc) {
			case HD_H265E_LEVEL_1: case HD_H265E_LEVEL_2:   case HD_H265E_LEVEL_2_1:
			case HD_H265E_LEVEL_3: case HD_H265E_LEVEL_3_1: case HD_H265E_LEVEL_4:   case HD_H265E_LEVEL_4_1:
			case HD_H265E_LEVEL_5: case HD_H265E_LEVEL_5_1: case HD_H265E_LEVEL_5_2:
			case HD_H265E_LEVEL_6: case HD_H265E_LEVEL_6_1: case HD_H265E_LEVEL_6_2:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
						"HD_VIDEOENC_LEVEL: when H265, level_idc(%d) is not supported.", p_h26x->level_idc);
				goto exit;
			}
		} else {
			snprintf(p_ret_string, max_str_len,
						"HD_VIDEOENC_LEVEL: codec_type != H264/H265 is not support.");
			goto exit;
		}

		// Entropy check
		if (p_enc_out->codec_type == HD_CODEC_TYPE_H264) {
			switch(p_h26x->entropy_mode) {
			case HD_H264E_CAVLC_CODING:
			case HD_H264E_CABAC_CODING:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_CODING: when H264, entropy_mode(%d) is not support.", p_h26x->entropy_mode);
				goto exit;
			}
		} else if (p_enc_out->codec_type == HD_CODEC_TYPE_H265) {
			switch(p_h26x->entropy_mode) {
			case HD_H265E_CABAC_CODING:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_CODING: when H265, entropy_mode(%d) is not support.", p_h26x->entropy_mode);
				goto exit;
			}
		} else {
			snprintf(p_ret_string, max_str_len,
						"HD_VIDEOENC_CODING: codec_type != H264/H265 is not support.");
			goto exit;
		}

		if (p_h26x->svc_layer > 2) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26X_CONFIG: svc_layer(%d) is out-of-range(0~2).", p_h26x->svc_layer);
			goto exit;
		}

	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_H26XENC_VUI(HD_H26XENC_VUI *p_enc_vui, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_vui->vui_en > 1) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_VUI: vui_en(%d) is out-of-range(0~1).", p_enc_vui->vui_en);
		goto exit;
	}
	if (p_enc_vui->vui_en == 1) {
		if (p_enc_vui->sar_width > 65535) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VUI: sar_width(%lu) is out-of-range(0~65535).", p_enc_vui->sar_width);
			goto exit;
		}
		if (p_enc_vui->sar_height > 65535) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VUI: sar_height(%lu) is out-of-range(0~65535).", p_enc_vui->sar_height);
			goto exit;
		}
		if (p_enc_vui->matrix_coef > 255) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VUI: matrix_coef(%d) is out-of-range(0~255).", p_enc_vui->matrix_coef);
			goto exit;
		}
		if (p_enc_vui->transfer_characteristics > 255) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VUI: transfer_characteristics(%d) is out-of-range(0~255).", p_enc_vui->transfer_characteristics);
			goto exit;
		}
		if (p_enc_vui->colour_primaries > 255) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VUI: colour_primaries(%d) is out-of-range(0~255).", p_enc_vui->colour_primaries);
			goto exit;
		}
		if (p_enc_vui->video_format > 7) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VUI: video_format(%d) is out-of-range(0~7).", p_enc_vui->video_format);
			goto exit;
		}
		if (p_enc_vui->color_range > 1) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VUI: color_range(%d) is out-of-range(0~1).", p_enc_vui->color_range);
			goto exit;
		}
		if (p_enc_vui->timing_present_flag > 1) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VUI: timing_present_flag(%d) is out-of-range(0~1).", p_enc_vui->timing_present_flag);
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_H26XENC_DEBLOCK(HD_H26XENC_DEBLOCK *p_enc_deblock, CHAR *p_ret_string, INT max_str_len)
{
#if defined(_BSP_NA51000_)
	if (p_enc_deblock->dis_ilf_idc > 2) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_DEBLOCK: dis_ilf_idc(%d) is out-of-range(0~2).", p_enc_deblock->dis_ilf_idc);
		goto exit;
	}
#elif defined(_BSP_NA51055_)
	if (p_enc_deblock->dis_ilf_idc > 6) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_DEBLOCK: dis_ilf_idc(%d) is out-of-range(0~6).", p_enc_deblock->dis_ilf_idc);
		goto exit;
	}
#endif
	if (p_enc_deblock->db_alpha < -12 || p_enc_deblock->db_alpha > 12) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_DEBLOCK: db_alpha(%d) is out-of-range(-12~12).", p_enc_deblock->db_alpha);
		goto exit;
	}
	if (p_enc_deblock->db_beta < -12 || p_enc_deblock->db_beta > 12) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_DEBLOCK: db_beta(%d) is out-of-range(-12~12).", p_enc_deblock->db_beta);
		goto exit;
	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_H26XENC_RATE_CONTROL2(HD_H26XENC_RATE_CONTROL2 *p_enc_rc, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_rc->rc_mode == HD_RC_MODE_CBR) {
		HD_H26XENC_CBR2     *p_cbr = &p_enc_rc->cbr;
		if (p_cbr->bitrate > 128*1024*1024) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: bitrate(%lu) is out-of-range(0~128Mbit).", p_cbr->bitrate);
			goto exit;
		}
		/* p_cbr->frame_rate_base and p_cbr->frame_rate_incr can be any value.
		 */
		if (p_cbr->init_i_qp > 51 || p_cbr->max_i_qp > 51 || p_cbr->min_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: i_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
					p_cbr->init_i_qp, p_cbr->min_i_qp, p_cbr->max_i_qp);
			goto exit;
		}
		if (p_cbr->init_p_qp > 51 || p_cbr->max_p_qp > 51 || p_cbr->min_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: p_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
					p_cbr->init_p_qp, p_cbr->min_p_qp, p_cbr->max_p_qp);
			goto exit;
		}

		if (p_cbr->static_time > 20) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: static_time(%lu) is out-of-range(0~20).", p_cbr->static_time);
			goto exit;
		}
		if (p_cbr->ip_weight < -100 || p_cbr->ip_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: ip_weight(%ld) is out-of-range(-100~100).", p_cbr->ip_weight);
			goto exit;
		}

		if (p_cbr->frame_rate_incr == 0) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: frame_rate_incr(%ld) can't be 0.", p_cbr->frame_rate_incr);
			goto exit;
		}

		if (p_cbr->key_p_period > 4096) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: key_p_period(%ld) is out-of-range(0~4096).", p_cbr->key_p_period);
			goto exit;
		}
		if (p_cbr->kp_weight < -100 || p_cbr->kp_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: kp_weight(%ld) is out-of-range(-100~100).", p_cbr->kp_weight);
			goto exit;
		}
		if (p_cbr->p2_weight < -100 || p_cbr->p2_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: p2_weight(%ld) is out-of-range(-100~100).", p_cbr->p2_weight);
			goto exit;
		}
		if (p_cbr->p3_weight < -100 || p_cbr->p3_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: p3_weight(%ld) is out-of-range(-100~100).", p_cbr->p3_weight);
			goto exit;
		}
		if (p_cbr->lt_weight < -100 || p_cbr->lt_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: lt_weight(%ld) is out-of-range(-100~100).", p_cbr->lt_weight);
			goto exit;
		}
		if (p_cbr->motion_aq_str < -15 || p_cbr->motion_aq_str > 15) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_CBR: motion_aq_str(%ld) is out-of-range(-15~15).", p_cbr->motion_aq_str);
			goto exit;
		}
	} else if (p_enc_rc->rc_mode == HD_RC_MODE_VBR) {
		HD_H26XENC_VBR2     *p_vbr = &p_enc_rc->vbr;
		if (p_vbr->bitrate > 128*1024*1024) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: bitrate(%lu) is out-of-range(0~128Mbit).", p_vbr->bitrate);
			goto exit;
		}
		/* p_vbr->frame_rate_base and p_vbr->frame_rate_incr can be any value.
		 */
		if (p_vbr->init_i_qp > 51 || p_vbr->max_i_qp > 51 || p_vbr->min_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: i_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
					p_vbr->init_i_qp, p_vbr->min_i_qp, p_vbr->max_i_qp);
			goto exit;
		}
		if (p_vbr->init_p_qp > 51 || p_vbr->max_p_qp > 51 || p_vbr->min_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: p_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
					p_vbr->init_p_qp, p_vbr->min_p_qp, p_vbr->max_p_qp);
			goto exit;
		}

		if (p_vbr->static_time > 20) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: static_time(%lu) is out-of-range(0~20).", p_vbr->static_time);
			goto exit;
		}
		if (p_vbr->ip_weight < -100 || p_vbr->ip_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: ip_weight(%ld) is out-of-range(-100~100).", p_vbr->ip_weight);
			goto exit;
		}

		if (p_vbr->frame_rate_incr == 0) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: frame_rate_incr(%ld) can't be 0.", p_vbr->frame_rate_incr);
			goto exit;
		}
		if (p_vbr->key_p_period > 4096) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: key_p_period(%ld) is out-of-range(0~4096).", p_vbr->key_p_period);
			goto exit;
		}
		if (p_vbr->kp_weight < -100 || p_vbr->kp_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: kp_weight(%ld) is out-of-range(-100~100).", p_vbr->kp_weight);
			goto exit;
		}
		if (p_vbr->p2_weight < -100 || p_vbr->p2_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: p2_weight(%ld) is out-of-range(-100~100).", p_vbr->p2_weight);
			goto exit;
		}
		if (p_vbr->p3_weight < -100 || p_vbr->p3_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: p3_weight(%ld) is out-of-range(-100~100).", p_vbr->p3_weight);
			goto exit;
		}
		if (p_vbr->lt_weight < -100 || p_vbr->lt_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: lt_weight(%ld) is out-of-range(-100~100).", p_vbr->lt_weight);
			goto exit;
		}
		if (p_vbr->motion_aq_str < -15 || p_vbr->motion_aq_str > 15) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: motion_aq_str(%ld) is out-of-range(-15~15).", p_vbr->motion_aq_str);
			goto exit;
		}
		if (p_vbr->change_pos > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_VBR: change_pos(%lu) is out-of-range(0~100).", p_vbr->change_pos);
			goto exit;
		}

	} else if (p_enc_rc->rc_mode == HD_RC_MODE_FIX_QP) {
		HD_H26XENC_FIXQP     *p_fixqp = &p_enc_rc->fixqp;
		/* p_vbr->frame_rate_base and p_vbr->frame_rate_incr can be any value.
		 */
		if (p_fixqp->fix_i_qp > 51 || p_fixqp->fix_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_FIXQP: fix_i_qp(%u) fix_p_qp(%u) is out-of-range(0~51).",
					p_fixqp->fix_i_qp, p_fixqp->fix_p_qp);
			goto exit;
		}
		if (p_fixqp->frame_rate_incr == 0) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_FIXQP: frame_rate_incr(%ld) can't be 0.", p_fixqp->frame_rate_incr);
			goto exit;
		}

	} else if (p_enc_rc->rc_mode == HD_RC_MODE_EVBR) {
		HD_H26XENC_EVBR2    *p_evbr = &p_enc_rc->evbr;
		if (p_evbr->bitrate > 128*1024*1024) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: bitrate(%lu) is out-of-range(0~128Mbit).", p_evbr->bitrate);
			goto exit;
		}
		/* p_evbr->frame_rate_base and p_evbr->frame_rate_incr can be any value.
		 */
		if (p_evbr->init_i_qp > 51 || p_evbr->max_i_qp > 51 || p_evbr->min_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: i_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
					p_evbr->init_i_qp, p_evbr->min_i_qp, p_evbr->max_i_qp);
			goto exit;
		}
		if (p_evbr->init_p_qp > 51 || p_evbr->max_p_qp > 51 || p_evbr->min_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: p_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
					p_evbr->init_p_qp, p_evbr->min_p_qp, p_evbr->max_p_qp);
			goto exit;
		}

		if (p_evbr->static_time > 20) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: static_time(%lu) is out-of-range(0~20).", p_evbr->static_time);
			goto exit;
		}
		if (p_evbr->ip_weight < -100 || p_evbr->ip_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: ip_weight(%ld) is out-of-range(-100~100).", p_evbr->ip_weight);
			goto exit;
		}
		if (p_evbr->frame_rate_incr == 0) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: frame_rate_incr(%ld) can't be 0.", p_evbr->frame_rate_incr);
			goto exit;
		}

		if (p_evbr->key_p_period > 4096) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: key_p_period(%ld) is out-of-range(0~4096).", p_evbr->key_p_period);
			goto exit;
		}
		if (p_evbr->kp_weight < -100 || p_evbr->kp_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: kp_weight(%ld) is out-of-range(-100~100).", p_evbr->kp_weight);
			goto exit;
		}
		if (p_evbr->p2_weight < -100 || p_evbr->p2_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: p2_weight(%ld) is out-of-range(-100~100).", p_evbr->p2_weight);
			goto exit;
		}
		if (p_evbr->p3_weight < -100 || p_evbr->p3_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: p3_weight(%ld) is out-of-range(-100~100).", p_evbr->p3_weight);
			goto exit;
		}
		if (p_evbr->lt_weight < -100 || p_evbr->lt_weight > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: lt_weight(%ld) is out-of-range(-100~100).", p_evbr->lt_weight);
			goto exit;
		}
		if (p_evbr->still_frame_cnd < 1 || p_evbr->still_frame_cnd > 4096) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: still_frame_cnd(%ld) is out-of-range(1~4096).", p_evbr->still_frame_cnd);
			goto exit;
		}
		if (p_evbr->motion_ratio_thd < 1 || p_evbr->motion_ratio_thd > 100) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: motion_ratio_thd(%ld) is out-of-range(1~100).", p_evbr->motion_ratio_thd);
			goto exit;
		}
		if (p_evbr->motion_aq_str < -15 || p_evbr->motion_aq_str > 15) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: motion_aq_str(%ld) is out-of-range(-15~15).", p_evbr->motion_aq_str);
			goto exit;
		}

		if (p_evbr->still_i_qp > 51 || p_evbr->still_p_qp > 51 || p_evbr->still_kp_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_EVBR: still_i_qp(%lu) still_p_qp(%lu) still_kp_qp(%lu) is out-of-range(0~51).",
					p_evbr->still_i_qp, p_evbr->still_p_qp, p_evbr->still_kp_qp);
			goto exit;
		}
	} else {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_RATE_CONTROL: rc_mode(%d) is not supported.", p_enc_rc->rc_mode);
		goto exit;
	}

	return 0;
exit:
	return -1;
}


static INT _hd_videoenc_verify_param_HD_H26XENC_USR_QP(HD_H26XENC_USR_QP *p_enc_usr_qp, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_usr_qp->enable > 1) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_USR_QP: enable(%d) is out-of-range(0~1).", p_enc_usr_qp->enable);
		goto exit;
	}
	if (p_enc_usr_qp->enable == 1) {
		if (p_enc_usr_qp->qp_map_addr == 0) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_USR_QP: qp_map_addr is NULL.");
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_H26XENC_SLICE_SPLIT(HD_H26XENC_SLICE_SPLIT *p_enc_slice_split, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_slice_split->enable > 1) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_SLICE_SPLIT: enable(%ld) is out-of-range(0~1).", p_enc_slice_split->enable);
		goto exit;
	}
	if (p_enc_slice_split->enable == 1) {
		if ((p_enc_slice_split->slice_row_num < 1)) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_SLICE_SPLIT: slice_row_num(%ld) range:(1 ~ num_of_mb/ctu_row).", p_enc_slice_split->slice_row_num);
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}


static INT _hd_videoenc_verify_param_HD_H26XENC_GDR(HD_H26XENC_GDR *p_enc_gdr, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_gdr->enable > 1) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_GDR: enable(%d) is out-of-range(0~1).", p_enc_gdr->enable);
		goto exit;
	}
	if (p_enc_gdr->enable == 1) {
		/*if (p_enc_gdr->period < 0 || p_enc_gdr->period > 0xFFFFFFFF) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_GDR: period(%ld) is out-of-range(0~0xFFFFFFFF).", p_enc_gdr->period);
			goto exit;
		}*/
		if (p_enc_gdr->number < 1) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_GDR: number(%ld) range:(1 ~ num_of_mb/ctu_row).", p_enc_gdr->number);
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_H26XENC_ROI(HD_H26XENC_ROI  *p_enc_roi, CHAR *p_ret_string, INT max_str_len)
{
	INT i;
	HD_H26XENC_ROI_WIN *p_st_roi;
#if defined(_BSP_NA51000_)
	switch (p_enc_roi->roi_qp_mode) {
	case HD_VIDEOENC_QPMODE_FIXED_QP:
	case HD_VIDEOENC_QPMODE_DELTA:
		break;
	default:
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_ROI: roi_qp_mode(%d) is not supported.\n", p_enc_roi->roi_qp_mode);
		goto exit;
	}
#endif
	for (i = 0; i < HD_H26XENC_ROI_WIN_COUNT; i++) {
		p_st_roi = &p_enc_roi->st_roi[i];
		if (p_st_roi->enable > 1) {
				snprintf(p_ret_string, max_str_len,
						"HD_H26XENC_ROI_WIN: enable(%d) is out-of-range(0~1).", p_st_roi->enable);
				goto exit;
		}

		if (p_st_roi->enable == 1) {
#if defined(_BSP_NA51000_)
			if (p_enc_roi->roi_qp_mode == HD_VIDEOENC_QPMODE_FIXED_QP) {
				if (p_st_roi->qp < 0 || p_st_roi->qp > 51) {
					snprintf(p_ret_string, max_str_len,
							"HD_H26XENC_ROI_WIN: when roi_qp_mode=%u, qp(%d) is out-of-range(0~51).",
							p_enc_roi->roi_qp_mode, p_st_roi->qp);
					goto exit;
				}
			} else {
				if (p_st_roi->qp < -26 || p_st_roi->qp > 25) {
					snprintf(p_ret_string, max_str_len,
							"HD_H26XENC_ROI_WIN: when roi_qp_mode=%d, qp(%d) is out-of-range(-26~25).",
							p_enc_roi->roi_qp_mode, p_st_roi->qp);
					goto exit;
				}
			}
#elif defined(_BSP_NA51055_)
			{
				switch(p_st_roi->mode) {
				case HD_VIDEOENC_QPMODE_DELTA:
				case HD_VIDEOENC_QPMODE_RESERVED:
				case HD_VIDEOENC_QPMODE_DISABLE_AQ:
				case HD_VIDEOENC_QPMODE_FIXED_QP:
					break;
				default:
					snprintf(p_ret_string, max_str_len,
							"HD_H26XENC_ROI_WIN: mode(%d) is not supported.", p_st_roi->mode);
					goto exit;
				}
			}
			if (p_st_roi->mode == HD_VIDEOENC_QPMODE_FIXED_QP) {
				if (p_st_roi->qp < 0 || p_st_roi->qp > 51) {
					snprintf(p_ret_string, max_str_len,
							"HD_H26XENC_ROI_WIN: when mode=%u, qp(%d) is out-of-range(0~51).",
							p_st_roi->mode, p_st_roi->qp);
					goto exit;
				}
			} else {
				if (p_st_roi->qp < -32 || p_st_roi->qp > 31) {
					snprintf(p_ret_string, max_str_len,
							"HD_H26XENC_ROI_WIN: when mode=%d, qp(%d) is out-of-range(-32~31).",
							p_st_roi->mode, p_st_roi->qp);
					goto exit;
				}
			}
#endif
		}
	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_H26XENC_ROW_RC(HD_H26XENC_ROW_RC *p_enc_row_rc, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_row_rc->enable > 1) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_ROW_RC: enable(%d) is out-of-range(0~1).", p_enc_row_rc->enable);
		goto exit;
	}
	if (p_enc_row_rc->enable == 1) {
		if (p_enc_row_rc->i_qp_range > 15) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_ROW_RC: i_qp_range(%u) is out-of-range(0~15).", p_enc_row_rc->i_qp_range);
			goto exit;
		}
		if (p_enc_row_rc->i_qp_step > 15) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_ROW_RC: i_qp_step(%u) is out-of-range(0~15).", p_enc_row_rc->i_qp_step);
			goto exit;
		}

		if (p_enc_row_rc->p_qp_range < 0 || p_enc_row_rc->p_qp_range > 15) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_ROW_RC: p_qp_range(%u) is out-of-range(0~15).", p_enc_row_rc->p_qp_range);
			goto exit;
		}
		if (p_enc_row_rc->p_qp_step < 0 || p_enc_row_rc->p_qp_step > 15) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_ROW_RC: p_qp_step(%u) is out-of-range(0~15).", p_enc_row_rc->p_qp_step);
			goto exit;
		}
		if (p_enc_row_rc->min_i_qp < 0 || p_enc_row_rc->min_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_ROW_RC: min_i_qp(%u) is out-of-range(0~51).", p_enc_row_rc->min_i_qp);
			goto exit;
		}
		if (p_enc_row_rc->max_i_qp < 0 || p_enc_row_rc->max_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_ROW_RC: max_i_qp(%u) is out-of-range(0~51).", p_enc_row_rc->max_i_qp);
			goto exit;
		}
		if (p_enc_row_rc->min_p_qp < 0 || p_enc_row_rc->min_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_ROW_RC: min_p_qp(%u) is out-of-range(0~51).", p_enc_row_rc->min_p_qp);
			goto exit;
		}
		if (p_enc_row_rc->max_p_qp < 0 || p_enc_row_rc->max_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_ROW_RC: max_p_qp(%u) is out-of-range(0~51).", p_enc_row_rc->max_p_qp);
			goto exit;
		}

	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_H26XENC_AQ(HD_H26XENC_AQ *p_enc_aq, CHAR *p_ret_string, INT max_str_len)
{
	//INT i;
	if (p_enc_aq->enable > 1) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_AQ: enable(%d) is out-of-range(0~1).", p_enc_aq->enable);
		goto exit;
	}
	if (p_enc_aq->enable == 1) {
		if (p_enc_aq->i_str < 1 || p_enc_aq->i_str > 8) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_AQ: i_str(%u) is out-of-range(1~8).", p_enc_aq->i_str);
			goto exit;
		}
		if (p_enc_aq->p_str < 1 || p_enc_aq->p_str > 8) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_AQ: p_str(%u) is out-of-range(1~8).", p_enc_aq->p_str);
			goto exit;
		}
		if (p_enc_aq->max_delta_qp < 0 || p_enc_aq->max_delta_qp > 15) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_AQ: max_delta_qp(%d) is out-of-range(0~15).", (int)p_enc_aq->max_delta_qp);
			goto exit;
		}
		if (p_enc_aq->min_delta_qp < -15 || p_enc_aq->min_delta_qp > 0) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_AQ: min_delta_qp(%d) is out-of-range(-15~0).", (int)p_enc_aq->min_delta_qp);
			goto exit;
		}
		/*
		if (p_enc_aq->depth < 0 || p_enc_aq->depth > 2) {
			snprintf(p_ret_string, max_str_len,
					"HD_H26XENC_AQ: depth(%u) is out-of-range(0~2).", p_enc_aq->depth);
			goto exit;
		}

		for (i = 0; i < HD_H26XENC_AQ_MAP_TABLE_NUM; i++) {
			INT16       value = p_enc_aq->thd_table[i];
			if (value < -512 || value > 511) {
				snprintf(p_ret_string, max_str_len,
						"HD_H26XENC_AQ: p_enc_aq->thd_table[%d](%d) is out-of-range(-512~511).", i, value);
				goto exit;
			}
		}
		*/
	}
	return 0;
exit:
	return -1;

}
static INT _hd_videoenc_verify_param_HD_H26XENC_REQUEST_IFRAME(HD_H26XENC_REQUEST_IFRAME *p_enc_reqi, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_reqi->enable > 1) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_REQUEST_IFRAME: enable(%d) is out-of-range(0~1).", p_enc_reqi->enable);
		goto exit;
	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_H26XENC_TRIG_SNAPSHOT(UINT32 self_id, UINT32 out_id, HD_H26XENC_TRIG_SNAPSHOT *p_enc_trig_snapshot, CHAR *p_ret_string, INT max_str_len)
{
	if (videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.vendor_set.b_comm_srcout_no_jpg_enc) {
		return 0;
	}

	if (p_enc_trig_snapshot->phy_addr == 0) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_TRIG_SNAPSHOT: phy_addr is NULL.");
		goto exit;
	}

	if (p_enc_trig_snapshot->size == 0) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_TRIG_SNAPSHOT: size is 0.");
		goto exit;
	}

	if (p_enc_trig_snapshot->image_quality < 1 || p_enc_trig_snapshot->image_quality > 100) {
		snprintf(p_ret_string, max_str_len,
				"HD_H26XENC_TRIG_SNAPSHOT: image_quality(%ld) is out-of-range(1~100).", p_enc_trig_snapshot->image_quality);
		goto exit;
	}

	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_verify_param_HD_VIDEOENC_BS_RING(HD_VIDEOENC_BS_RING *p_enc_bs_ring, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_bs_ring->enable > 1) {
		snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_BS_RING: enable(%ld) is out-of-range(0~1).", (unsigned long)p_enc_bs_ring->enable);
		goto exit;
	}
	return 0;
exit:
	return -1;
}

static INT _hd_videoenc_check_exist_lowlatency_path(UINT32 *ll_port)
{
	UINT32 dev = 0 , port = 0;

	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			if (videoenc_info[dev].port[port].enc_func_cfg.in_func & HD_VIDEOENC_INFUNC_LOWLATENCY) {
				*ll_port = port;
				return -1;
			}
		}
	}
	return 0;
}

INT _hd_videoenc_param_cvt_name(HD_VIDEOENC_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
		case HD_VIDEOENC_PARAM_DEVCOUNT:            snprintf(p_ret_string, max_str_len, "DEVCOUNT");           break;
		case HD_VIDEOENC_PARAM_SYSCAPS:             snprintf(p_ret_string, max_str_len, "SYSCAPS");            break;
		case HD_VIDEOENC_PARAM_PATH_CONFIG:         snprintf(p_ret_string, max_str_len, "PATH_CONFIG");        break;
		case HD_VIDEOENC_PARAM_FUNC_CONFIG:         snprintf(p_ret_string, max_str_len, "FUNC_CONFIG");        break;
		case HD_VIDEOENC_PARAM_BUFINFO:             snprintf(p_ret_string, max_str_len, "BUFINFO");            break;
		case HD_VIDEOENC_PARAM_IN:                  snprintf(p_ret_string, max_str_len, "IN");                 break;
		case HD_VIDEOENC_PARAM_IN_FRC:              snprintf(p_ret_string, max_str_len, "IN_FRC");             break;
		case HD_VIDEOENC_PARAM_OUT_ENC_PARAM:       snprintf(p_ret_string, max_str_len, "OUT_ENC_PARAM");      break;
		case HD_VIDEOENC_PARAM_OUT_ENC_PARAM2:      snprintf(p_ret_string, max_str_len, "OUT_ENC_PARAM2");     break;
		case HD_VIDEOENC_PARAM_OUT_VUI:             snprintf(p_ret_string, max_str_len, "OUT_VUI");            break;
		case HD_VIDEOENC_PARAM_OUT_DEBLOCK:         snprintf(p_ret_string, max_str_len, "OUT_DEBLOCK");        break;
		case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL:    snprintf(p_ret_string, max_str_len, "OUT_RATE_CONTROL");   break;
		case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2:   snprintf(p_ret_string, max_str_len, "OUT_RATE_CONTROL2");  break;
		case HD_VIDEOENC_PARAM_OUT_USR_QP:          snprintf(p_ret_string, max_str_len, "OUT_USR_QP");         break;
		case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:     snprintf(p_ret_string, max_str_len, "OUT_SLICE_SPLIT");    break;
		case HD_VIDEOENC_PARAM_OUT_ENC_GDR:         snprintf(p_ret_string, max_str_len, "OUT_ENC_GDR");        break;
		case HD_VIDEOENC_PARAM_OUT_ROI:             snprintf(p_ret_string, max_str_len, "OUT_ROI");            break;
		case HD_VIDEOENC_PARAM_OUT_ROW_RC:          snprintf(p_ret_string, max_str_len, "OUT_ROW_RC");         break;
		case HD_VIDEOENC_PARAM_OUT_AQ:              snprintf(p_ret_string, max_str_len, "OUT_AQ");             break;
		case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME:  snprintf(p_ret_string, max_str_len, "OUT_REQUEST_IFRAME"); break;
		case HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT:   snprintf(p_ret_string, max_str_len, "OUT_TRIG_SNAPSHOT");  break;
		case HD_VIDEOENC_PARAM_IN_STAMP_BUF:        snprintf(p_ret_string, max_str_len, "IN_STAMP_BUF");       break;
		case HD_VIDEOENC_PARAM_IN_STAMP_IMG:        snprintf(p_ret_string, max_str_len, "IN_STAMP_IMG");       break;
		case HD_VIDEOENC_PARAM_IN_STAMP_ATTR:       snprintf(p_ret_string, max_str_len, "IN_STAMP_ATTR");      break;
		case HD_VIDEOENC_PARAM_IN_MASK_ATTR:        snprintf(p_ret_string, max_str_len, "IN_MASK_ATTR");       break;
		case HD_VIDEOENC_PARAM_IN_MOSAIC_ATTR:      snprintf(p_ret_string, max_str_len, "IN_MOSAIC_ATTR");     break;
		case HD_VIDEOENC_PARAM_IN_PALETTE_TABLE:    snprintf(p_ret_string, max_str_len, "IN_PALETTE_TABLE");   break;
		case HD_VIDEOENC_PARAM_BS_RING:             snprintf(p_ret_string, max_str_len, "BS_RING");            break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", (int)(id));
			return (-1);
	}
	return 0;
}

INT _hd_videoenc_check_params_state(UINT32 self_id, UINT32 out_id, HD_VIDEOENC_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	HD_VIDEOENC_STAT cur_state = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status;
	//BOOL b_maxmem_set = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set;
	CHAR  param_name[20];

	_hd_videoenc_param_cvt_name(id, param_name, 20);

	switch (id) {
#if 0
	case HD_VIDEOENC_PARAM_PATH_CONFIG:
		// static setting, also involved buffer alloc, need stop->close->open->set->start
		if (cur_state == HD_VIDEOENC_STATUS_START) {
			snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_PARAM_%s: could NOT set on START, please call stop->close->open->set->start !!", param_name);
			goto exit;
		} else if ((cur_state == HD_VIDEOENC_STATUS_OPEN) && (b_maxmem_set == TRUE)){
			snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_PARAM_%s: already set before, please call close->open->set->start to set again!!", param_name);
			goto exit;
		}
		break;

	case HD_VIDEOENC_PARAM_FUNC_CONFIG:
		// static setting, also involved buffer alloc, need stop->close->open->set->start
		if (cur_state == HD_VIDEOENC_STATUS_START) {
			snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_PARAM_%s: could NOT set on START, please call stop->close->open->set->start !!", param_name);
			goto exit;
		}
		break;

#endif
	case HD_VIDEOENC_PARAM_IN:
	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM:
	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM2:
	case HD_VIDEOENC_PARAM_OUT_VUI:
	case HD_VIDEOENC_PARAM_OUT_DEBLOCK:
	case HD_VIDEOENC_PARAM_BS_RING:
		// static setting, need stop->set->start
		if (cur_state == HD_VIDEOENC_STATUS_START) {
			snprintf(p_ret_string, max_str_len,
				"HD_VIDEOENC_PARAM_%s: could NOT set on START, please call stop->set->start !!", param_name);
			goto exit;
		}
		break;

	case HD_VIDEOENC_PARAM_OUT_ENC_GDR:
		if (videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.vendor_set.gdr_info.enable_gdr_i_frm) {
			if (cur_state == HD_VIDEOENC_STATUS_START) {
				snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PARAM_%s: could NOT set on START if gdr i-frm enable, please call close->open->set->start !!", param_name);
				goto exit;
			}

			if (cur_state == HD_VIDEOENC_STATUS_OPEN && videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_open_from_init == FALSE) {
				snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PARAM_%s: could NOT set w/o CLOSE if gdr i-frm enable, please call close->open->set->start !!", param_name);
				goto exit;
			}
		}
		break;

	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL:
	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2:
	case HD_VIDEOENC_PARAM_OUT_USR_QP:
	case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:
	case HD_VIDEOENC_PARAM_OUT_ROI:
	case HD_VIDEOENC_PARAM_OUT_ROW_RC:
	case HD_VIDEOENC_PARAM_OUT_AQ:
	case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME:
	case HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT:
		// dynamic setting, could set->start
		break;

	case HD_VIDEOENC_PARAM_IN_FRC:
		// dynamic setting, could set->start. But if involved (direct/timer) mode change, will result in BUG. JIRA : IVOT_N01004_CO-298
		if (cur_state == HD_VIDEOENC_STATUS_START) {
			HD_VIDEOENC_IN  *p_cur_frc = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_in_dim;
			HD_VIDEOENC_FRC *p_new_frc = (HD_VIDEOENC_FRC *)p_param;
			UINT32 cur_dst = GET_HI_UINT16(p_cur_frc->frc);
			UINT32 cur_src = GET_LO_UINT16(p_cur_frc->frc);
			UINT32 new_dst = GET_HI_UINT16(p_new_frc->frc);
			UINT32 new_src = GET_LO_UINT16(p_new_frc->frc);

			// check if mode change (direct/timer) on start
			if (((cur_dst <= cur_src) && (new_dst  > new_src)) ||
				((cur_dst  > cur_src) && (new_dst <= new_src))) {
				snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PARAM_%s: FATAL ERROR !! current frc(%lu/%lu) could NOT change to frc(%lu/%lu) via set->start. Please stop->set->start", param_name, cur_dst, cur_src, new_dst, new_src);
				goto exit;
			}
			// check if (timer mode -> timer mode) on start
			else if ((cur_dst  > cur_src) && (new_dst  > new_src)) {
				snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PARAM_%s: ERROR !! current frc(%lu/%lu) could NOT change to frc(%lu/%lu) via set->start. Please stop->set->start", param_name, cur_dst, cur_src, new_dst, new_src);
				goto exit;
			}
		}
		break;

	default:
		break;
	}

	return 0;
exit:
	return -1;
}

INT _hd_videoenc_check_params_range(UINT32 self_id, UINT32 out_id, HD_VIDEOENC_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	INT ret = 0;
	switch (id) {
	case HD_VIDEOENC_PARAM_PATH_CONFIG:
		ret = _hd_videoenc_verify_param_HD_VIDEOENC_PATH_CONFIG((HD_VIDEOENC_PATH_CONFIG*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_FUNC_CONFIG:
		ret = _hd_videoenc_verify_param_HD_VIDEOENC_FUNC_CONFIG((HD_VIDEOENC_FUNC_CONFIG*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_IN:
		ret = _hd_videoenc_verify_param_HD_VIDEOENC_IN((HD_VIDEOENC_IN*) p_param, p_ret_string, max_str_len);
		break;

//	case HD_VIDEOENC_PARAM_SYSINFO:
	//	break;

	case HD_VIDEOENC_PARAM_BUFINFO:
		break;

	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM:
		{
			HD_VIDEOENC_OUT2 enc_out_param2 = {0};
			_hd_videoenc_assign_param_enc_out_param_1_to_2(&enc_out_param2, (HD_VIDEOENC_OUT *)p_param);
			ret = _hd_videoenc_verify_param_HD_VIDEOENC_OUT2((HD_VIDEOENC_OUT2*) &enc_out_param2, p_ret_string, max_str_len);
		}
		break;

	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM2:
		ret = _hd_videoenc_verify_param_HD_VIDEOENC_OUT2((HD_VIDEOENC_OUT2*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_VUI:
		ret = _hd_videoenc_verify_param_HD_H26XENC_VUI((HD_H26XENC_VUI*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_DEBLOCK:
		ret = _hd_videoenc_verify_param_HD_H26XENC_DEBLOCK((HD_H26XENC_DEBLOCK*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param2 = {0};
			_hd_videoenc_assign_param_rc_param_1_to_2(&rc_param2, (HD_H26XENC_RATE_CONTROL *)p_param);
			ret = _hd_videoenc_verify_param_HD_H26XENC_RATE_CONTROL2((HD_H26XENC_RATE_CONTROL2*) &rc_param2, p_ret_string, max_str_len);
		}
		break;

	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2:
		ret = _hd_videoenc_verify_param_HD_H26XENC_RATE_CONTROL2((HD_H26XENC_RATE_CONTROL2*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_USR_QP:
		ret = _hd_videoenc_verify_param_HD_H26XENC_USR_QP((HD_H26XENC_USR_QP*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:
		ret = _hd_videoenc_verify_param_HD_H26XENC_SLICE_SPLIT((HD_H26XENC_SLICE_SPLIT*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_ENC_GDR:
		ret = _hd_videoenc_verify_param_HD_H26XENC_GDR((HD_H26XENC_GDR*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_ROI:
		ret = _hd_videoenc_verify_param_HD_H26XENC_ROI((HD_H26XENC_ROI*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_ROW_RC:
		ret = _hd_videoenc_verify_param_HD_H26XENC_ROW_RC((HD_H26XENC_ROW_RC*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_AQ:
		ret = _hd_videoenc_verify_param_HD_H26XENC_AQ((HD_H26XENC_AQ*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME:
		ret = _hd_videoenc_verify_param_HD_H26XENC_REQUEST_IFRAME((HD_H26XENC_REQUEST_IFRAME *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT:
		ret = _hd_videoenc_verify_param_HD_H26XENC_TRIG_SNAPSHOT(self_id, out_id, (HD_H26XENC_TRIG_SNAPSHOT*) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_BS_RING:
		ret = _hd_videoenc_verify_param_HD_VIDEOENC_BS_RING((HD_VIDEOENC_BS_RING*) p_param, p_ret_string, max_str_len);
		break;

	default:
		break;
	}
	return ret;
}

INT _hd_videoenc_check_params_range_on_start(UINT32 self_id, UINT32 out_id, CHAR *p_ret_string, INT max_str_len)
{
	UINT32 dev  = self_id-DEV_BASE;
	UINT32 port =  out_id-OUT_BASE;

	//check dim & codec
	{
		MP_VDOENC_HW_LMT hw_limit = {0};
		HD_VIDEO_CODEC codec = videoenc_info[dev].port[port].enc_out_param.codec_type;
		HD_DIM dim           = videoenc_info[dev].port[port].enc_in_dim.dim;
		HD_VIDEO_DIR dir     = videoenc_info[dev].port[port].enc_in_dim.dir;
		UINT32 chk_w         = ((dir == HD_VIDEO_DIR_ROTATE_90) || (dir == HD_VIDEO_DIR_ROTATE_270))? dim.h:dim.w;
		UINT32 chk_h         = ((dir == HD_VIDEO_DIR_ROTATE_90) || (dir == HD_VIDEO_DIR_ROTATE_270))? dim.w:dim.h;

		_hd_get_struct_param(self_id, out_id, VDOENC_PARAM_HW_LIMIT, (UINT32*)&hw_limit, sizeof(MP_VDOENC_HW_LMT));

		switch (codec) {
		case HD_CODEC_TYPE_H264:
			{
				if (chk_w < hw_limit.h264e.min_width) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.w(%lu) after rotate(%lu) must >= %lu for H264 codec type (minimum dim = %lux%lu) !! \n", dim.w, chk_w, hw_limit.h264e.min_width, hw_limit.h264e.min_width, hw_limit.h264e.min_height);
					goto exit;
				}

				if (chk_h < hw_limit.h264e.min_height) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.h(%lu) after rotate(%lu) must >= %lu for H264 codec type (minimum dim = %lux%lu) !! \n", dim.h, chk_h, hw_limit.h264e.min_height, hw_limit.h264e.min_width, hw_limit.h264e.min_height);
					goto exit;
				}
#if 1
				if (chk_w > hw_limit.h264e.max_width) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.w(%lu) after rotate(%lu) must <= %lu for H264 codec type (maximum dim = %lux%lu) !! \n", dim.w, chk_w, hw_limit.h264e.max_width, hw_limit.h264e.max_width, hw_limit.h264e.max_height);
					goto exit;
				}

				if (chk_h > hw_limit.h264e.max_height) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.h(%lu) after rotate(%lu) must <= %lu for H264 codec type (maximum dim = %lux%lu) !! \n", dim.h, chk_h, hw_limit.h264e.max_height, hw_limit.h264e.max_width, hw_limit.h264e.max_height);
					goto exit;
				}

				if ((dir == HD_VIDEO_DIR_ROTATE_90) || (dir == HD_VIDEO_DIR_ROTATE_270)) {
					if (dim.h > hw_limit.h264e_rotate_max_height) {
						snprintf(p_ret_string, max_str_len,
								"HD_VIDEOENC_IN: dim.h(%lu) must <= %lu for H264 codec type with rotate 90/270!!\n", dim.h, hw_limit.h264e_rotate_max_height);
						goto exit;
					}
				}

				// check crop dim
				if (videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info.frm_crop_enable == TRUE) {
					UINT32 disp_w = (UINT32)videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info.display_w;
					UINT32 disp_h = (UINT32)videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info.display_h;
					if (disp_w > dim.w) {
						snprintf(p_ret_string, max_str_len,
								"HD_VIDEOENC_IN: crop disp_w(%lu) must <= dim.w %lu for H264 codec type!!\n", disp_w, dim.w);
						goto exit;
					}

					if (disp_h > dim.h) {
						snprintf(p_ret_string, max_str_len,
								"HD_VIDEOENC_IN: crop disp_h(%lu) must <= dim.h %lu for H264 codec type!!\n", disp_h, dim.h);
						goto exit;
					}
				}
#endif
			}
			break;
		case HD_CODEC_TYPE_H265:
			{
				if (chk_w < hw_limit.h265e.min_width) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.w(%lu) after rotate(%lu) must >= %lu for H265 codec type (minimum dim = %lux%lu) !! \n", dim.w, chk_w, hw_limit.h265e.min_width, hw_limit.h265e.min_width, hw_limit.h265e.min_height);
					goto exit;
				}

				if (chk_h < hw_limit.h265e.min_height) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.h(%lu) after rotate(%lu) must >= %lu for H265 codec type (minimum dim = %lux%lu) !! \n", dim.h, chk_h, hw_limit.h265e.min_height, hw_limit.h265e.min_width, hw_limit.h265e.min_height);
					goto exit;
				}
#if 1
				if (chk_w > hw_limit.h265e.max_width) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.w(%lu) after rotate(%lu) must <= %lu for H265 codec type (maximum dim = %lux%lu) !! \n", dim.w, chk_w, hw_limit.h265e.max_width, hw_limit.h265e.max_width, hw_limit.h265e.max_height);
					goto exit;
				}

				if (chk_h > hw_limit.h265e.max_height) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.h(%lu) after rotate(%lu) must <= %lu for H265 codec type (maximum dim = %lux%lu) !! \n", dim.h, chk_h, hw_limit.h265e.max_height, hw_limit.h265e.max_width, hw_limit.h265e.max_height);
					goto exit;
				}

				// check crop dim
				if (videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info.frm_crop_enable == TRUE) {
					UINT32 disp_w = (UINT32)videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info.display_w;
					UINT32 disp_h = (UINT32)videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info.display_h;
					if (disp_w > dim.w) {
						snprintf(p_ret_string, max_str_len,
								"HD_VIDEOENC_IN: crop disp_w(%lu) must <= dim.w %lu for H265 codec type!!\n", disp_w, dim.w);
						goto exit;
					}

					if (disp_h > dim.h) {
						snprintf(p_ret_string, max_str_len,
								"HD_VIDEOENC_IN: crop disp_h(%lu) must <= dim.h %lu for H265 codec type!!\n", disp_h, dim.h);
						goto exit;
					}
				}
#endif
			}
			break;
		case HD_CODEC_TYPE_JPEG:
			{
				if (chk_w < hw_limit.jpege.min_width) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.w(%lu) must >= %lu for JPEG codec type (minimum dim = %lux%lu) !! \n", chk_w, hw_limit.jpege.min_width, hw_limit.jpege.min_width, hw_limit.jpege.min_height);
					goto exit;
				}

				if (chk_h < hw_limit.jpege.min_height) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.h(%lu) must >= %lu for JPEG codec type (minimum dim = %lux%lu) !! \n", chk_h, hw_limit.jpege.min_height, hw_limit.jpege.min_width, hw_limit.jpege.min_height);
					goto exit;
				}
#if 0
				if (chk_w > hw_limit.jpege.max_width) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.w(%lu) must <= %lu for JPEG codec type (maximum dim = %lux%lu) !! \n", chk_w, hw_limit.jpege.max_width, hw_limit.jpege.max_width, hw_limit.jpege.max_height);
					goto exit;
				}

				if (chk_h > hw_limit.jpege.max_height) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_IN: dim.h(%lu) must <= %lu for JPEG codec type (maximum dim = %lux%lu) !! \n", chk_h, hw_limit.jpege.max_height, hw_limit.jpege.max_width, hw_limit.jpege.max_height);
					goto exit;
				}
#endif
			}
			break;
		default:
			{
				snprintf(p_ret_string, max_str_len,
								"HD_VIDEOENC_OUT: codec_type(%d) is not supported.\n", codec);
				goto exit;
			}
			break;
		}
	}

	// check enc_buf_ms & min ratio
	{
		UINT32 enc_buf_ms  = videoenc_info[dev].port[port].enc_path_cfg.max_mem.enc_buf_ms;
		UINT32 min_i_ratio = videoenc_info[dev].port[port].priv.vendor_set.min_i_ratio;
		UINT32 min_p_ratio = videoenc_info[dev].port[port].priv.vendor_set.min_p_ratio;

		if (enc_buf_ms < min_i_ratio) {
			snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PARAM_PATH_CONFIG: enc_buf_ms(%lu) must >= min_i_ratio(%lu)\n", enc_buf_ms, min_i_ratio);
			goto exit;
		}
		if (enc_buf_ms < min_p_ratio) {
			snprintf(p_ret_string, max_str_len,
					"HD_VIDEOENC_PARAM_PATH_CONFIG: enc_buf_ms(%lu) must >= min_p_ratio(%lu)\n", enc_buf_ms, min_p_ratio);
			goto exit;
		}
	}

	// if vendor(fix_work_memory) is set to TRUE => check if path_config & out_enc_param & in_param have all the same settings !!
	if (videoenc_info[dev].port[port].priv.vendor_set.b_fit_work_memory == TRUE) {
		HD_VIDEOENC_PATH_CONFIG *p_enc_path_cfg  = (HD_VIDEOENC_PATH_CONFIG*)&videoenc_info[dev].port[port].enc_path_cfg;
		HD_VIDEOENC_OUT2 *p_enc_out_param = (HD_VIDEOENC_OUT2*)&videoenc_info[dev].port[port].enc_out_param;
		HD_VIDEOENC_IN *p_enc_in_dim = (HD_VIDEOENC_IN*)&videoenc_info[dev].port[port].enc_in_dim;

		if (p_enc_out_param->codec_type != p_enc_path_cfg->max_mem.codec_type) {
			snprintf(p_ret_string, max_str_len,
					"(codec_type) for HD_VIDEOENC_PARAM_PATH_CONFIG & HD_VIDEOENC_PARAM_OUT_ENC_PARAM is NOT match when vendor(FIT_WORK_MEMORY) is set !!");
			goto exit;
		}
		if ((p_enc_path_cfg->max_mem.rotate == FALSE) &&
			((p_enc_in_dim->dir == HD_VIDEO_DIR_ROTATE_90) || (p_enc_in_dim->dir == HD_VIDEO_DIR_ROTATE_270))) {
			snprintf(p_ret_string, max_str_len,
					"(rotate) for HD_VIDEOENC_PARAM_PATH_CONFIG & HD_VIDEOENC_PARAM_IN is NOT match when vendor(FIT_WORK_MEMORY) is set !!");
			goto exit;
		}
		if ((p_enc_path_cfg->max_mem.rotate == TRUE) &&
			((p_enc_in_dim->dir == HD_VIDEO_DIR_ROTATE_0) || (p_enc_in_dim->dir == HD_VIDEO_DIR_ROTATE_180))) {
			snprintf(p_ret_string, max_str_len,
					"(rotate) for HD_VIDEOENC_PARAM_PATH_CONFIG & HD_VIDEOENC_PARAM_IN is NOT match when vendor(FIT_WORK_MEMORY) is set !!");
			goto exit;
		}
		if (p_enc_in_dim->dim.w != p_enc_path_cfg->max_mem.max_dim.w) {
			snprintf(p_ret_string, max_str_len,
					"(width) for HD_VIDEOENC_PARAM_PATH_CONFIG & HD_VIDEOENC_PARAM_IN is NOT match when vendor(FIT_WORK_MEMORY) is set !!");
			goto exit;
		}
		if (p_enc_in_dim->dim.h != p_enc_path_cfg->max_mem.max_dim.h) {
			snprintf(p_ret_string, max_str_len,
					"(height) for HD_VIDEOENC_PARAM_PATH_CONFIG & HD_VIDEOENC_PARAM_IN is NOT match when vendor(FIT_WORK_MEMORY) is set !!");
			goto exit;
		}

		if ((p_enc_path_cfg->max_mem.codec_type == HD_CODEC_TYPE_H264) || (p_enc_path_cfg->max_mem.codec_type == HD_CODEC_TYPE_H265)) {
			if ((p_enc_out_param->h26x.ltr_interval == 0) != (p_enc_path_cfg->max_mem.ltr == FALSE)) {
				snprintf(p_ret_string, max_str_len,
						"(ltr) for HD_VIDEOENC_PARAM_PATH_CONFIG & HD_VIDEOENC_PARAM_OUT_ENC_PARAM is NOT match when vendor(FIT_WORK_MEMORY) is set !!");
				goto exit;
			}
			if (p_enc_out_param->h26x.svc_layer != p_enc_path_cfg->max_mem.svc_layer) {
				snprintf(p_ret_string, max_str_len,
						"(svc) for HD_VIDEOENC_PARAM_PATH_CONFIG & HD_VIDEOENC_PARAM_OUT_ENC_PARAM is NOT match when vendor(FIT_WORK_MEMORY) is set !!");
				goto exit;
			}
			if (p_enc_out_param->h26x.source_output != p_enc_path_cfg->max_mem.source_output) {
				snprintf(p_ret_string, max_str_len,
						"(source_output) for HD_VIDEOENC_PARAM_PATH_CONFIG & HD_VIDEOENC_PARAM_OUT_ENC_PARAM is NOT match when vendor(FIT_WORK_MEMORY) is set !!");
				goto exit;
			}
		}
	}

	// check rate control bitrate
	{
		HD_VIDEO_CODEC codec = videoenc_info[dev].port[port].enc_out_param.codec_type;
		HD_VIDEOENC_PATH_CONFIG *p_enc_path_cfg  = (HD_VIDEOENC_PATH_CONFIG*)&videoenc_info[dev].port[port].enc_path_cfg;
		HD_H26XENC_RATE_CONTROL2 *p_enc_rc = (HD_H26XENC_RATE_CONTROL2*)&videoenc_info[dev].port[port].enc_rc;

		if (codec != HD_CODEC_TYPE_JPEG) {
			if (p_enc_rc->rc_mode == HD_RC_MODE_CBR) {
				HD_H26XENC_CBR2 *p_cbr = &p_enc_rc->cbr;
				if (p_cbr->bitrate > p_enc_path_cfg->max_mem.bitrate) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_PARAM_PATH_CONFIG: path config bitrate(%lu) must be >= HD_H26XENC_CBR: bitrate(%lu).", p_enc_path_cfg->max_mem.bitrate, p_cbr->bitrate);
					goto exit;
				}
			} else if (p_enc_rc->rc_mode == HD_RC_MODE_VBR) {
				HD_H26XENC_VBR2 *p_vbr = &p_enc_rc->vbr;
				if (p_vbr->bitrate > p_enc_path_cfg->max_mem.bitrate) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_PARAM_PATH_CONFIG: path config bitrate(%lu) must be >= HD_H26XENC_VBR: bitrate(%lu).", p_enc_path_cfg->max_mem.bitrate, p_vbr->bitrate);
					goto exit;
				}
			} else if (p_enc_rc->rc_mode == HD_RC_MODE_EVBR) {
				HD_H26XENC_EVBR2 *p_evbr = &p_enc_rc->evbr;
				if (p_evbr->bitrate > p_enc_path_cfg->max_mem.bitrate) {
					snprintf(p_ret_string, max_str_len,
							"HD_VIDEOENC_PARAM_PATH_CONFIG: path config bitrate(%lu) must be >= HD_H26XENC_EVBR: bitrate(%lu).", p_enc_path_cfg->max_mem.bitrate, p_evbr->bitrate);
					goto exit;
				}
			}
		}
	}

	return 0;
exit:
	return -1;
}

VOID _hd_videoenc_fill_default_vendor_params(UINT32 kflow_videoenc_param, VOID *p_param)
{
	switch (kflow_videoenc_param) {
	case VDOENC_PARAM_RDO:
		{
			MP_VDOENC_RDO_INFO *p_rdo_info = (MP_VDOENC_RDO_INFO *)p_param;
			#if 0
			p_rdo_info->rdo_codec = VDOENC_RDO_CODEC_265;
			p_rdo_info->rdo_param.rdo_265.hevc_intra_32x32_cost_bias       = 0;
			p_rdo_info->rdo_param.rdo_265.hevc_intra_16x16_cost_bias       = 0;
			p_rdo_info->rdo_param.rdo_265.hevc_intra_8x8_cost_bias         = 0;
			p_rdo_info->rdo_param.rdo_265.hevc_inter_skip_cost_bias        = 0;
			p_rdo_info->rdo_param.rdo_265.hevc_inter_merge_cost_bias       = 0;
			p_rdo_info->rdo_param.rdo_265.hevc_inter_64x64_cost_bias       = 14;
			p_rdo_info->rdo_param.rdo_265.hevc_inter_64x32_32x64_cost_bias = 28;
			p_rdo_info->rdo_param.rdo_265.hevc_inter_32x32_cost_bias       = 14;
			p_rdo_info->rdo_param.rdo_265.hevc_inter_32x16_16x32_cost_bias = 28;
			p_rdo_info->rdo_param.rdo_265.hevc_inter_16x16_cost_bias       = 7;
			#else
			p_rdo_info->rdo_codec = VDOENC_RDO_CODEC_265 + 1; // means "not set" by user yet
			#endif
		}
		break;

	case VDOENC_PARAM_JND:
		{
			MP_VDOENC_JND_INFO *p_jnd_info = (MP_VDOENC_JND_INFO *)p_param;
			p_jnd_info->enable     = 0;
			p_jnd_info->str        = 10;
			p_jnd_info->level      = 12;
			p_jnd_info->threshold  = 25;
		}
	break;

	case VDOENC_PARAM_JPG_RC_MIN_QUALITY:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_JPG_RC_MAX_QUALITY:
		*(UINT32 *)p_param = 100;
		break;

	case VDOENC_PARAM_MIN_I_RATIO:
		*(UINT32 *)p_param = 1500;  // will set (1500/10) = 150 to kflow later (which is kflow default)
		break;

	case VDOENC_PARAM_MIN_P_RATIO:
		*(UINT32 *)p_param = 1000;  // will set (1000/10) = 100 to kflow later (which is kflow default)
		break;

	case VDOENC_PARAM_COLMV_ENABLE:
		*(UINT32 *)p_param = 1;
		break;

	case VDOENC_PARAM_COMM_RECFRM_ENABLE:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_COMM_BASE_RECFRM:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_COMM_SVC_RECFRM:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_COMM_LTR_RECFRM:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_MAXMEM_FIT_WORK_MEMORY:
		*(BOOL *)p_param = FALSE;
		break;

	case VDOENC_PARAM_LONG_START_CODE:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_BS_QUICK_ROLLBACK:
		*(BOOL *)p_param = FALSE;
		break;

	case VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_BR_TOLERANCE:
		*(UINT32 *)p_param = 2;
		break;

	case VDOENC_PARAM_COMM_SRCOUT_ENABLE:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_COMM_SRCOUT_ADDR:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_COMM_SRCOUT_SIZE:
		*(UINT32 *)p_param = 0;
		break;

	case VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE:
		*(BOOL *)p_param = 1;
		break;

	case VDOENC_PARAM_H26X_SEI_CHKSUM:
		*(BOOL *)p_param = FALSE;
		break;

	case VDOENC_PARAM_GDR:
		{
			MP_VDOENC_GDR_INFO *p_gdr_info = (MP_VDOENC_GDR_INFO *)p_param;
			p_gdr_info->enable_gdr_i_frm = 0;
			p_gdr_info->enable_gdr_qp    = 0;
			p_gdr_info->gdr_qp           = 0;
		}
	break;

	case VDOENC_PARAM_H26X_LOW_POWER:
		*(BOOL *)p_param = FALSE;
		break;

	case VDOENC_PARAM_H26X_CROP:
		{
			MP_VDOENC_FRM_CROP_INFO *p_h26x_crop_info = (MP_VDOENC_FRM_CROP_INFO *)p_param;
			p_h26x_crop_info->frm_crop_enable = FALSE;
			p_h26x_crop_info->display_w = 0;
			p_h26x_crop_info->display_h= 0;
		}
		break;

	case VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE:
		*(BOOL *)p_param = 0;
		break;

	case VDOENC_PARAM_H26X_SKIP_FRM:
		{
			NMI_VDOENC_H26X_SKIP_FRM_INFO *p_skip_frm_info = (NMI_VDOENC_H26X_SKIP_FRM_INFO *)p_param;
			p_skip_frm_info->enable = 0;
			p_skip_frm_info->target_fr = 0;
			p_skip_frm_info->input_frm_cnt = 0;
		}
		break;

	default:
		break;
	}
}

VOID _hd_videoenc_fill_default_params_query_kdrv(UINT32 self_id, UINT32 out_id, HD_VIDEOENC_PARAM_ID id, VOID *p_param)
{
	switch(id) {
	case HD_VIDEOENC_PARAM_OUT_AQ:
		{
			MP_VDOENC_AQ_INFO aq_info = {0};
			HD_H26XENC_AQ *p_enc_aq = (HD_H26XENC_AQ *) p_param;

			_hd_get_struct_param(self_id, out_id, VDOENC_PARAM_AQINFO, (UINT32*)&aq_info, sizeof(MP_VDOENC_AQ_INFO));

			p_enc_aq->enable       = aq_info.enable;
			p_enc_aq->i_str        = aq_info.i_str;
			p_enc_aq->p_str        = aq_info.p_str;
			p_enc_aq->max_delta_qp = aq_info.max_delta_qp;
			p_enc_aq->min_delta_qp = aq_info.min_delta_qp;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_ROW_RC:
		{
			MP_VDOENC_ROWRC_INFO rowrc_info = {0};
			HD_H26XENC_ROW_RC *p_enc_row_rc = (HD_H26XENC_ROW_RC *) p_param;

			_hd_get_struct_param(self_id, out_id, VDOENC_PARAM_ROWRC, (UINT32*)&rowrc_info, sizeof(MP_VDOENC_ROWRC_INFO));

			p_enc_row_rc->enable     = rowrc_info.enable;
			p_enc_row_rc->i_qp_range = rowrc_info.i_qp_range;
			p_enc_row_rc->i_qp_step  = rowrc_info.i_qp_step;
			p_enc_row_rc->p_qp_range = rowrc_info.p_qp_range;
			p_enc_row_rc->p_qp_step  = rowrc_info.p_qp_step;
			p_enc_row_rc->min_i_qp   = rowrc_info.min_i_qp;
			p_enc_row_rc->max_i_qp   = rowrc_info.max_i_qp;
			p_enc_row_rc->min_p_qp   = rowrc_info.min_p_qp;
			p_enc_row_rc->max_p_qp   = rowrc_info.max_p_qp;
		}
	break;
	default:
		break;
	}
}

VOID _hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_ID id, VOID *p_param)
{
	switch (id) {
	case HD_VIDEOENC_PARAM_PATH_CONFIG:
		{
			HD_VIDEOENC_PATH_CONFIG *p_path_cfg = (HD_VIDEOENC_PATH_CONFIG *)p_param;
			p_path_cfg->max_mem.codec_type    = HD_CODEC_TYPE_H265;
			p_path_cfg->max_mem.max_dim.w     = 1920;
			p_path_cfg->max_mem.max_dim.h     = 1080;
			p_path_cfg->max_mem.bitrate       = 2*1024*1024; // 2 Mb/s = 512 MB/s
			p_path_cfg->max_mem.enc_buf_ms    = 3000;
			p_path_cfg->max_mem.svc_layer     = HD_SVC_4X;
			p_path_cfg->max_mem.ltr           = TRUE;
			p_path_cfg->max_mem.rotate        = FALSE;
			p_path_cfg->max_mem.source_output = FALSE;

			p_path_cfg->isp_id                = 0; // use sensor ID 0
		}
	break;
	case HD_VIDEOENC_PARAM_FUNC_CONFIG:
		{
			HD_VIDEOENC_FUNC_CONFIG* p_func_cfg = (HD_VIDEOENC_FUNC_CONFIG *)p_param;
			p_func_cfg->in_func = 0;
			p_func_cfg->ddr_id  = 0;
		}
	break;
	case HD_VIDEOENC_PARAM_IN:
		{
			HD_VIDEOENC_IN *p_enc_in = (HD_VIDEOENC_IN *) p_param;
			p_enc_in->dim.w   = 1920;
			p_enc_in->dim.h   = 1080;
			p_enc_in->pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
			p_enc_in->dir     = HD_VIDEO_DIR_NONE;
			p_enc_in->frc     = HD_VIDEO_FRC_RATIO(1,1);
		}
	break;
	case HD_VIDEOENC_PARAM_BUFINFO:
		{
			HD_VIDEOENC_BUFINFO *p_buf_inf = (HD_VIDEOENC_BUFINFO *)p_param;
			p_buf_inf->buf_info.phy_addr = 0;
			p_buf_inf->buf_info.buf_size = 0;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM2:
		{
			HD_VIDEOENC_OUT2 *p_enc_out = (HD_VIDEOENC_OUT2 *) p_param;
			p_enc_out->codec_type           = HD_CODEC_TYPE_H264;
			p_enc_out->h26x.profile         = HD_H264E_HIGH_PROFILE;
			p_enc_out->h26x.level_idc       = HD_H264E_LEVEL_5_1;
			p_enc_out->h26x.gop_num         = 15;
			p_enc_out->h26x.ltr_interval    = 0;
			p_enc_out->h26x.ltr_pre_ref     = 0;
			p_enc_out->h26x.gray_en         = 0;
			p_enc_out->h26x.source_output   = 0;
			p_enc_out->h26x.svc_layer       = HD_SVC_DISABLE;
			p_enc_out->h26x.entropy_mode    = HD_H264E_CABAC_CODING;
			p_enc_out->h26x.chrm_qp_idx     = 0;
			p_enc_out->h26x.sec_chrm_qp_idx = 0;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_VUI:
		{
			HD_H26XENC_VUI *p_enc_vui = (HD_H26XENC_VUI *) p_param;
			p_enc_vui->vui_en                   = 0;
			p_enc_vui->sar_width                = 0;
			p_enc_vui->sar_height               = 0;
			p_enc_vui->matrix_coef              = 2;
			p_enc_vui->transfer_characteristics = 2;
			p_enc_vui->colour_primaries         = 2;
			p_enc_vui->video_format             = 5;
			p_enc_vui->color_range              = 0;
			p_enc_vui->timing_present_flag      = 0;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_DEBLOCK:
		{
			HD_H26XENC_DEBLOCK *p_enc_deblock = (HD_H26XENC_DEBLOCK *) p_param;
			p_enc_deblock->dis_ilf_idc = 0;
			p_enc_deblock->db_alpha    = 0;
			p_enc_deblock->db_beta     = 0;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2:
		{
			HD_H26XENC_RATE_CONTROL2 *p_enc_rc = (HD_H26XENC_RATE_CONTROL2 *) p_param;
			HD_H26XENC_CBR2     *p_cbr = &p_enc_rc->cbr;
			p_enc_rc->rc_mode      = HD_RC_MODE_CBR;
			p_cbr->bitrate         = 0;  // will be set to same as path_config.max_mem.bitrate later
			p_cbr->frame_rate_base = 30;
			p_cbr->frame_rate_incr = 1;
			p_cbr->init_i_qp       = 26;
			p_cbr->max_i_qp        = 45;
			p_cbr->min_i_qp        = 10;
			p_cbr->init_p_qp       = 26;
			p_cbr->max_p_qp        = 45;
			p_cbr->min_p_qp        = 10;
			p_cbr->static_time     = 4;
			p_cbr->ip_weight       = 0;
			p_cbr->key_p_period    = 0;
			p_cbr->kp_weight       = 0;
			p_cbr->p2_weight       = 0;
			p_cbr->p3_weight       = 0;
			p_cbr->lt_weight       = 0;
			p_cbr->motion_aq_str   = 0;
			p_cbr->max_frame_size  = 0;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_USR_QP:
		{
			HD_H26XENC_USR_QP *p_enc_usr_qp = (HD_H26XENC_USR_QP *) p_param;
			p_enc_usr_qp->enable      = 0;
			p_enc_usr_qp->qp_map_addr = 0;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:
		{
			HD_H26XENC_SLICE_SPLIT *p_enc_slice_split = (HD_H26XENC_SLICE_SPLIT *) p_param;
			p_enc_slice_split->enable        = 0;
			p_enc_slice_split->slice_row_num = 1;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_ENC_GDR:
		{
			HD_H26XENC_GDR *p_enc_gdr = (HD_H26XENC_GDR *) p_param;
			p_enc_gdr->enable = 0;
			p_enc_gdr->period = 0;
			p_enc_gdr->number = 1;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_ROI:
		{
			HD_H26XENC_ROI  *p_enc_roi = (HD_H26XENC_ROI *) p_param;
			HD_H26XENC_ROI_WIN  *p_st_roi;
			INT i;
			p_enc_roi->roi_qp_mode = HD_VIDEOENC_QPMODE_DELTA;
			for (i = 0; i < HD_H26XENC_ROI_WIN_COUNT; i++) {
				p_st_roi = &p_enc_roi->st_roi[i];
				memset(p_st_roi, 0, sizeof(HD_H26XENC_ROI_WIN));
			}
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_ROW_RC:
		{
			HD_H26XENC_ROW_RC *p_enc_row_rc = (HD_H26XENC_ROW_RC *) p_param;
			p_enc_row_rc->enable     = 1;
			p_enc_row_rc->i_qp_range = 2;
			p_enc_row_rc->i_qp_step  = 1;
			p_enc_row_rc->p_qp_range = 4;
			p_enc_row_rc->p_qp_step  = 1;
			p_enc_row_rc->min_i_qp   = 1;
			p_enc_row_rc->max_i_qp   = 51;
			p_enc_row_rc->min_p_qp   = 1;
			p_enc_row_rc->max_p_qp   = 51;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_AQ:
		{
			HD_H26XENC_AQ *p_enc_aq = (HD_H26XENC_AQ *) p_param;
			p_enc_aq->enable       = 0;
			p_enc_aq->i_str        = 3;
			p_enc_aq->p_str        = 1;
			p_enc_aq->max_delta_qp = 6;
			p_enc_aq->min_delta_qp = -6;
			p_enc_aq->depth        = 2;
		}
	break;
	case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME:
		{
			HD_H26XENC_REQUEST_IFRAME *p_request_keyframe = (HD_H26XENC_REQUEST_IFRAME *) p_param;
			p_request_keyframe->enable = 0;
		}
	break;

	case HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT:
		{
			HD_H26XENC_TRIG_SNAPSHOT *p_trig_snaphot = (HD_H26XENC_TRIG_SNAPSHOT *) p_param;
			p_trig_snaphot->phy_addr      = 0;
			p_trig_snaphot->size          = 0;
			p_trig_snaphot->image_quality = 70;
		}
	break;

	case HD_VIDEOENC_PARAM_BS_RING:
		{
			HD_VIDEOENC_BS_RING *p_enc_bs_ring = (HD_VIDEOENC_BS_RING *) p_param;
			p_enc_bs_ring->enable = 0;
		}
	break;
	default:
		break;
	}
	return;
}

HD_RESULT _hd_videoenc_fill_all_default_params(UINT32 self_id, UINT32 out_id)
{
	UINT32 dev  = (self_id - DEV_BASE);
	UINT32 port = (out_id  - OUT_BASE);
	VDOENC_INFO *p_enc_info = &videoenc_info[dev];

	//init values
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_PATH_CONFIG, &p_enc_info->port[port].enc_path_cfg);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_FUNC_CONFIG, &p_enc_info->port[port].enc_func_cfg);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_IN, &p_enc_info->port[port].enc_in_dim);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &p_enc_info->port[port].enc_out_param);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_VUI, &p_enc_info->port[port].enc_vui);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_DEBLOCK, &p_enc_info->port[port].enc_deblock);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &p_enc_info->port[port].enc_rc);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_USR_QP, &p_enc_info->port[port].enc_usr_qp);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &p_enc_info->port[port].enc_slice_split);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_ENC_GDR, &p_enc_info->port[port].enc_gdr);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_ROI, &p_enc_info->port[port].enc_roi);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &p_enc_info->port[port].request_keyframe);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT, &p_enc_info->port[port].trig_snapshot);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_BS_RING, &p_enc_info->port[port].enc_bs_ring);

#if defined(_BSP_NA51055_)
	_hd_videoenc_fill_default_params_query_kdrv(self_id, out_id, HD_VIDEOENC_PARAM_OUT_ROW_RC, &p_enc_info->port[port].enc_row_rc);
	_hd_videoenc_fill_default_params_query_kdrv(self_id, out_id, HD_VIDEOENC_PARAM_OUT_AQ, &p_enc_info->port[port].enc_aq);
#else
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_ROW_RC, &p_enc_info->port[port].enc_row_rc);
	_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_AQ, &p_enc_info->port[port].enc_aq);
#endif

	// init vendor values
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_RDO, &p_enc_info->port[port].priv.vendor_set.rdo_info);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_JND, &p_enc_info->port[port].priv.vendor_set.jnd_info);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_JPG_RC_MIN_QUALITY, &p_enc_info->port[port].priv.vendor_set.jpg_rc_min_q);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_JPG_RC_MAX_QUALITY, &p_enc_info->port[port].priv.vendor_set.jpg_rc_max_q);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_MIN_I_RATIO, &p_enc_info->port[port].priv.vendor_set.min_i_ratio);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_MIN_P_RATIO, &p_enc_info->port[port].priv.vendor_set.min_p_ratio);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COLMV_ENABLE, &p_enc_info->port[port].priv.vendor_set.h26x_colmv_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COMM_RECFRM_ENABLE, &p_enc_info->port[port].priv.vendor_set.h26x_comm_recfrm_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COMM_BASE_RECFRM, &p_enc_info->port[port].priv.vendor_set.h26x_comm_recfrm_base_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COMM_SVC_RECFRM, &p_enc_info->port[port].priv.vendor_set.h26x_comm_recfrm_svc_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COMM_LTR_RECFRM, &p_enc_info->port[port].priv.vendor_set.h26x_comm_recfrm_ltr_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_MAXMEM_FIT_WORK_MEMORY, &p_enc_info->port[port].priv.vendor_set.b_fit_work_memory);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_LONG_START_CODE, &p_enc_info->port[port].priv.vendor_set.long_start_code);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_BS_QUICK_ROLLBACK, &p_enc_info->port[port].priv.vendor_set.b_bs_quick_rollback);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE, &p_enc_info->port[port].priv.vendor_set.b_quality_base_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_BR_TOLERANCE, &p_enc_info->port[port].priv.vendor_set.br_tolerance);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COMM_SRCOUT_ENABLE, &p_enc_info->port[port].priv.vendor_set.b_comm_srcout_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC, &p_enc_info->port[port].priv.vendor_set.b_comm_srcout_no_jpg_enc);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COMM_SRCOUT_ADDR, &p_enc_info->port[port].priv.vendor_set.comm_srcout_addr);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_COMM_SRCOUT_SIZE, &p_enc_info->port[port].priv.vendor_set.comm_srcout_size);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE, &p_enc_info->port[port].priv.vendor_set.b_timer_trig_comp_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_H26X_SEI_CHKSUM, &p_enc_info->port[port].priv.vendor_set.b_h26x_sei_chksum);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_GDR, &p_enc_info->port[port].priv.vendor_set.gdr_info);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_H26X_LOW_POWER, &p_enc_info->port[port].priv.vendor_set.b_h26x_low_power);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_H26X_CROP, &p_enc_info->port[port].priv.vendor_set.h26x_crop_info);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE, &p_enc_info->port[port].priv.vendor_set.b_h26x_maq_diff_en);
	_hd_videoenc_fill_default_vendor_params(VDOENC_PARAM_H26X_SKIP_FRM, &p_enc_info->port[port].priv.vendor_set.skip_frm_info);
	return HD_OK;
}

BOOL _hd_videoenc_kflow_param_is_single_value(UINT32 kflow_videoenc_param)
{
	switch (kflow_videoenc_param) {
		//--- single value settings ---
		case VDOENC_PARAM_CODEC:                   // 1:MJPG , 2:H264 , 3: H265
		case VDOENC_PARAM_PROFILE:                 // 0:Baseline , 1: Main , 2: High
		case VDOENC_PARAM_TARGETRATE:              // (Byte/s)
		case VDOENC_PARAM_ENCBUF_MS:
		case VDOENC_PARAM_ENCBUF_RESERVED_MS:
		case VDOENC_PARAM_FRAMERATE:
		case VDOENC_PARAM_GOPTYPE:                 // 0: I/P  , 1: I/P/B
		case VDOENC_PARAM_GOPNUM:
		case VDOENC_PARAM_RECFORMAT:               // 1: VID_ONLY ,  2: AUD_VID_BOTH 4: GOLFSHOT , 5: TIMELAPSE , 7:LIVEVIEW
		case VDOENC_PARAM_FILETYPE:                // 1: MOV , 2: AVI , 4: MP4 , 8: TS , 16: JPG
		case VDOENC_PARAM_INITQP:
		case VDOENC_PARAM_MINQP:
		case VDOENC_PARAM_MAXQP:
		case VDOENC_PARAM_MIN_I_RATIO:
		case VDOENC_PARAM_MIN_P_RATIO:
		case VDOENC_PARAM_MAX_FRAME_QUEUE:
		case VDOENC_PARAM_DAR:                     // 0= Default aspect ratio , 1= 16:9
		case VDOENC_PARAM_SKIP_FRAME:
		case VDOENC_PARAM_SVC:                     // 0: DISABLE , 1: LAYER1 , 2:LAYER2
		case VDOENC_PARAM_MULTI_TEMPORARY_LAYER:   // 0 : disable, 1: enable (H.265 Use Multi Temporal Layer)
		case VDOENC_PARAM_USRQP:                   // use user Qp (0:default, 1:enable)
		case VDOENC_PARAM_FRAME_INTERVAL:
		case VDOENC_PARAM_ENCBUF_ADDR:
		case VDOENC_PARAM_ENCBUF_SIZE:
		case VDOENC_PARAM_DIS:                     // 0 : disable, 1: enable
		case VDOENC_PARAM_TV_RANGE:                // 0 : tv range ,  1 : full range
		case VDOENC_PARAM_START_TIMER_BY_MANUAL:
		case VDOENC_PARAM_START_TIMER:
		case VDOENC_PARAM_STOP_TIMER:
		case VDOENC_PARAM_START_SMART_ROI:         // 0 : disable, 1: enable
		case VDOENC_PARAM_WAIT_SMART_ROI:          // 0 : disable, 1: enable
		case VDOENC_PARAM_RESET_IFRAME:
		case VDOENC_PARAM_RESET_SEC:
		case VDOENC_PARAM_TIMELAPSE_TIME:
		case VDOENC_PARAM_ALLOC_SNAPSHOT_BUF:
		case VDOENC_PARAM_SNAPSHOT:
		case VDOENC_PARAM_SYNCSRC:
		case VDOENC_PARAM_FPS_IMM:
		case VDOENC_PARAM_FUNC:                    // [bit0] 0: WAIT mode,  1: NOWAIT mode , [bit1] LOWLATENCY enable : 0/1
		case VDOENC_PARAM_BUFINFO_PHYADDR:
		case VDOENC_PARAM_BUFINFO_SIZE:
		case VDOENC_PARAM_ISP_ID:
		case VDOENC_PARAM_LEVEL_IDC:
		case VDOENC_PARAM_ENTROPY:
		case VDOENC_PARAM_GRAY:
		case VDOENC_PARAM_JPG_QUALITY:
		case VDOENC_PARAM_JPG_RESTART_INTERVAL:
		case VDOENC_PARAM_JPG_BITRATE:
		case VDOENC_PARAM_JPG_FRAMERATE:
		case VDOENC_PARAM_JPG_VBR_MODE:
		case VDOENC_PARAM_JPG_RC_MIN_QUALITY:
		case VDOENC_PARAM_JPG_RC_MAX_QUALITY:
		case VDOENC_PARAM_H26X_VBR_POLICY:
		case VDOENC_PARAM_GOPNUM_ONSTART:
		case VDOENC_PARAM_JPG_YUV_TRAN_ENABLE:
		case VDOENC_PARAM_COLMV_ENABLE:
		case VDOENC_PARAM_COMM_RECFRM_ENABLE:
		case VDOENC_PARAM_COMM_BASE_RECFRM:
		case VDOENC_PARAM_COMM_SVC_RECFRM:
		case VDOENC_PARAM_COMM_LTR_RECFRM:
		case VDOENC_PARAM_LONG_START_CODE:
		case VDOENC_PARAM_H26X_SVC_WEIGHT_MODE:
		case VDOENC_PARAM_BS_QUICK_ROLLBACK:
		case VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE:
		case VDOENC_PARAM_HW_PADDING_MODE:
		case VDOENC_PARAM_BR_TOLERANCE:
		case VDOENC_PARAM_COMM_SRCOUT_ENABLE:
		case VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC:
		case VDOENC_PARAM_COMM_SRCOUT_ADDR:
		case VDOENC_PARAM_COMM_SRCOUT_SIZE:
		case VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE:
		case VDOENC_PARAM_REQ_TARGET_I:
		case VDOENC_PARAM_H26X_SEI_CHKSUM:
		case VDOENC_PARAM_H26X_LOW_POWER:
		case VDOENC_PARAM_CHRM_QP_IDX:
		case VDOENC_PARAM_SEC_CHRM_QP_IDX:
		case VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE:
		case VDOENC_PARAM_H26X_MAQ_DIFF_STR:
		case VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX:
		case VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX:
		case VDOENC_PARAM_BS_RING:
		case VDOENC_PARAM_JPG_JFIF_ENABLE:
			return TRUE;

		//--- struct settings ---
		case VDOENC_PARAM_MAX_MEM_INFO:
		case VDOENC_PARAM_LTR:
		case VDOENC_PARAM_CBRINFO:
		case VDOENC_PARAM_EVBRINFO:
		case VDOENC_PARAM_VBRINFO:
		case VDOENC_PARAM_FIXQPINFO:
#if defined(_BSP_NA51055_)
		case VDOENC_PARAM_ROWRC:
		case VDOENC_PARAM_RDO:
		case VDOENC_PARAM_JND:
#endif
		case VDOENC_PARAM_AQINFO:
		case VDOENC_PARAM_ROI:
		case VDOENC_PARAM_DEBLOCK:
		case VDOENC_PARAM_GDR:
		case VDOENC_PARAM_VUI:
		case VDOENC_PARAM_SLICE_SPLIT:
		case VDOENC_PARAM_USR_QP_MAP:
		case VDOENC_PARAM_H26X_CROP:
		case VDOENC_PARAM_H26X_SKIP_FRM:
		case VDOENC_PARAM_META_ALLOC:
			return FALSE;

		//--- Function CB / Object settings ---
		case VDOENC_PARAM_ENCODER_OBJ:
		case VDOENC_PARAM_3DNR_CB:
		case VDOENC_PARAM_IMM_CB:
		case VDOENC_PARAM_EVENT_CB:
		case VDOENC_PARAM_RAW_DATA_CB:
		case VDOENC_PARAM_SRC_OUT_CB:
		case VDOENC_PARAM_PROC_CB:
		case VDOENC_PARAM_MAXMEM_FIT_WORK_MEMORY:  // no need to set to kflow individually, it's just used to pass param to PATH_CONFIG in hdal lib
		default:
			DBG_ERR("unsupport kflow param(%lu)\r\n", (unsigned long)(kflow_videoenc_param));
			return FALSE;
	}
}

UINT32 _hd_videoenc_kflow_param_get_single_value(UINT32 self_id, UINT32 out_id, UINT32 kflow_videoenc_param)
{
	UINT32 dev  = self_id-DEV_BASE;
	UINT32 port =  out_id-OUT_BASE;

	switch (kflow_videoenc_param) {
		//--- single value settings ---
		case VDOENC_PARAM_CODEC:                   // 1:MJPG , 2:H264 , 3: H265
			return _hd_videoenc_codec_hdal2unit(videoenc_info[dev].port[port].enc_out_param.codec_type);
		case VDOENC_PARAM_PROFILE:                 // 0:Baseline , 1: Main , 2: High
			return _hd_videoenc_profile_hdal2unit(videoenc_info[dev].port[port].enc_out_param.h26x.profile);
		case VDOENC_PARAM_ENCBUF_MS:
			return videoenc_info[dev].port[port].enc_path_cfg.max_mem.enc_buf_ms;
		case VDOENC_PARAM_GOPTYPE:                 // 0: I/P  , 1: I/P/B
			return 0;
		case VDOENC_PARAM_GOPNUM:
			return videoenc_info[dev].port[port].enc_out_param.h26x.gop_num;
		case VDOENC_PARAM_RECFORMAT:               // 1: VID_ONLY ,  2: AUD_VID_BOTH 4: GOLFSHOT , 5: TIMELAPSE , 7:LIVEVIEW
			return (videoenc_info[dev].port[port].priv.vendor_set.rec_format == 0)? 7:videoenc_info[dev].port[port].priv.vendor_set.rec_format;
		case VDOENC_PARAM_MULTI_TEMPORARY_LAYER:   // 0 : disable, 1: enable (H.265 Use Multi Temporal Layer)
			return 1;
		case VDOENC_PARAM_SVC:                     // 0: DISABLE , 1: LAYER1 , 2:LAYER2
			return _hd_videoenc_svc_hdal2unit(videoenc_info[dev].port[port].enc_out_param.h26x.svc_layer);

		case VDOENC_PARAM_TV_RANGE:                // 0 : tv range ,  1 : full range
			return videoenc_info[dev].port[port].enc_vui.color_range;
		case VDOENC_PARAM_RESET_IFRAME:
			return videoenc_info[dev].port[port].request_keyframe.enable;
		case VDOENC_PARAM_ALLOC_SNAPSHOT_BUF:
			return videoenc_info[dev].port[port].enc_out_param.h26x.source_output;
		case VDOENC_PARAM_FUNC:                    // [bit0] 0: WAIT mode,  1: NOWAIT mode , [bit1] LOWLATENCY enable : 0/1
#if defined(_BSP_NA51000_)
			return 0;
#elif defined(_BSP_NA51055_)
			return ((videoenc_info[dev].port[port].enc_func_cfg.in_func & HD_VIDEOENC_INFUNC_ONEBUF    )? VDOENC_FUNC_NOWAIT    :0)|
			       ((videoenc_info[dev].port[port].enc_func_cfg.in_func & HD_VIDEOENC_INFUNC_LOWLATENCY)? VDOENC_FUNC_LOWLATENCY:0);
#endif
		case VDOENC_PARAM_ISP_ID:
			return videoenc_info[dev].port[port].enc_path_cfg.isp_id;
		case VDOENC_PARAM_LEVEL_IDC:
			return videoenc_info[dev].port[port].enc_out_param.h26x.level_idc;
		case VDOENC_PARAM_ENTROPY:
			{
				switch (videoenc_info[dev].port[port].enc_out_param.h26x.entropy_mode) {
					case HD_H264E_CAVLC_CODING:
						return DAL_VDOENC_CAVLC;

					case HD_H264E_CABAC_CODING:
					case HD_H265E_CABAC_CODING:
						return DAL_VDOENC_CABAC;

					default:
						DBG_ERR("unsupport entropy_mode(%d)!!!\r\n", (int)(videoenc_info[dev].port[port].enc_out_param.h26x.entropy_mode));
						return 0;
				}
			}
			break;
		case VDOENC_PARAM_GRAY:
			return videoenc_info[dev].port[port].enc_out_param.h26x.gray_en;
		case VDOENC_PARAM_JPG_QUALITY:
			return videoenc_info[dev].port[port].enc_out_param.jpeg.image_quality;
		case VDOENC_PARAM_JPG_RESTART_INTERVAL:
			return videoenc_info[dev].port[port].enc_out_param.jpeg.retstart_interval;
		case VDOENC_PARAM_JPG_BITRATE:
			return videoenc_info[dev].port[port].enc_out_param.jpeg.bitrate;
		case VDOENC_PARAM_JPG_FRAMERATE:
		{
			if (videoenc_info[dev].port[port].enc_out_param.jpeg.frame_rate_base == 0)
				return 0;
			else
				// already error check (base > 0 , incr == 0), so it's ok
				return (videoenc_info[dev].port[port].enc_out_param.jpeg.frame_rate_base / videoenc_info[dev].port[port].enc_out_param.jpeg.frame_rate_incr);
		}
		case VDOENC_PARAM_JPG_VBR_MODE:
		{
			if (videoenc_info[dev].port[port].priv.vendor_set.b_jpg_vbr_mode == TRUE) {
				videoenc_info[dev].port[port].priv.vendor_set.b_jpg_vbr_mode = FALSE;
				return videoenc_info[dev].port[port].priv.vendor_set.jpg_vbr_mode;
			} else {
				return 0;
			}
		}

		case VDOENC_PARAM_JPG_RC_MIN_QUALITY:
			return videoenc_info[dev].port[port].priv.vendor_set.jpg_rc_min_q;

		case VDOENC_PARAM_JPG_RC_MAX_QUALITY:
			return videoenc_info[dev].port[port].priv.vendor_set.jpg_rc_max_q;

		case VDOENC_PARAM_JPG_YUV_TRAN_ENABLE:
			if (videoenc_info[dev].port[port].priv.vendor_set.b_jpg_yuv_trans == TRUE) {
				videoenc_info[dev].port[port].priv.vendor_set.b_jpg_yuv_trans = FALSE;
				return videoenc_info[dev].port[port].priv.vendor_set.jpg_yuv_trans_en;
			} else {
				return 0;
			}

		case VDOENC_PARAM_H26X_VBR_POLICY:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_vbr_policy;

		case VDOENC_PARAM_MIN_I_RATIO:
			return (videoenc_info[dev].port[port].priv.vendor_set.min_i_ratio/10);

		case VDOENC_PARAM_MIN_P_RATIO:
			return (videoenc_info[dev].port[port].priv.vendor_set.min_p_ratio/10);

		case VDOENC_PARAM_GOPNUM_ONSTART:
			return videoenc_info[dev].port[port].enc_out_param.h26x.gop_num;

		case VDOENC_PARAM_COLMV_ENABLE:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_colmv_en;

		case VDOENC_PARAM_COMM_RECFRM_ENABLE:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_en;

		case VDOENC_PARAM_COMM_BASE_RECFRM:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_base_en;

		case VDOENC_PARAM_COMM_SVC_RECFRM:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_svc_en;

		case VDOENC_PARAM_COMM_LTR_RECFRM:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_ltr_en;

		case VDOENC_PARAM_LONG_START_CODE:
			return videoenc_info[dev].port[port].priv.vendor_set.long_start_code;

		case VDOENC_PARAM_H26X_SVC_WEIGHT_MODE:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_svc_weight_mode;

		case VDOENC_PARAM_BS_QUICK_ROLLBACK:
			return videoenc_info[dev].port[port].priv.vendor_set.b_bs_quick_rollback;

		case VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE:
			return videoenc_info[dev].port[port].priv.vendor_set.b_quality_base_en;

		case VDOENC_PARAM_HW_PADDING_MODE:
			return videoenc_info[dev].port[port].priv.vendor_set.hw_padding_mode;

		case VDOENC_PARAM_BR_TOLERANCE:
			return videoenc_info[dev].port[port].priv.vendor_set.br_tolerance;

		case VDOENC_PARAM_COMM_SRCOUT_ENABLE:
			return videoenc_info[dev].port[port].priv.vendor_set.b_comm_srcout_en;

		case VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC:
			return videoenc_info[dev].port[port].priv.vendor_set.b_comm_srcout_no_jpg_enc;

		case VDOENC_PARAM_COMM_SRCOUT_ADDR:
			return videoenc_info[dev].port[port].priv.vendor_set.comm_srcout_addr;

		case VDOENC_PARAM_COMM_SRCOUT_SIZE:
			return videoenc_info[dev].port[port].priv.vendor_set.comm_srcout_size;

		case VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE:
			return videoenc_info[dev].port[port].priv.vendor_set.b_timer_trig_comp_en;

		case VDOENC_PARAM_H26X_SEI_CHKSUM:
			return videoenc_info[dev].port[port].priv.vendor_set.b_h26x_sei_chksum;

		case VDOENC_PARAM_H26X_LOW_POWER:
			return videoenc_info[dev].port[port].priv.vendor_set.b_h26x_low_power;

		case VDOENC_PARAM_CHRM_QP_IDX:
			return videoenc_info[dev].port[port].enc_out_param.h26x.chrm_qp_idx;

		case VDOENC_PARAM_SEC_CHRM_QP_IDX:
			return videoenc_info[dev].port[port].enc_out_param.h26x.sec_chrm_qp_idx;

		case VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE:
			return videoenc_info[dev].port[port].priv.vendor_set.b_h26x_maq_diff_en;

		case VDOENC_PARAM_H26X_MAQ_DIFF_STR:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_str;

		case VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_start_idx;

		case VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX:
			return videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_end_idx;

		case VDOENC_PARAM_BS_RING:
			return videoenc_info[dev].port[port].enc_bs_ring.enable;

		case VDOENC_PARAM_JPG_JFIF_ENABLE:
			if (videoenc_info[dev].port[port].priv.vendor_set.b_jpg_jfif == TRUE) {
				videoenc_info[dev].port[port].priv.vendor_set.b_jpg_jfif = FALSE;
				return videoenc_info[dev].port[port].priv.vendor_set.jpg_jfif_en;
			} else {
				return 0;
			}

		//---------------------
		case VDOENC_PARAM_DAR:                     // 0= Default aspect ratio , 1= 16:9  (only can set this to indirect set SAR)
		case VDOENC_PARAM_START_SMART_ROI:         // 0 : disable, 1: enable
		case VDOENC_PARAM_WAIT_SMART_ROI:          // 0 : disable, 1: enable
		case VDOENC_PARAM_SNAPSHOT:
		case VDOENC_PARAM_FPS_IMM:
		case VDOENC_PARAM_USRQP:                   // use user Qp (0:default, 1:enable)
			DBG_ERR("TODO: should this(%lu) support ??\r\n", (unsigned long)(kflow_videoenc_param));
			return 0;

		//--------------------
		case VDOENC_PARAM_TARGETRATE:              // (Byte/s)
		case VDOENC_PARAM_ENCBUF_RESERVED_MS:
		case VDOENC_PARAM_FRAMERATE:
		case VDOENC_PARAM_FILETYPE:                // 1: MOV , 2: AVI , 4: MP4 , 8: TS , 16: JPG
		case VDOENC_PARAM_INITQP:
		case VDOENC_PARAM_MINQP:
		case VDOENC_PARAM_MAXQP:
		case VDOENC_PARAM_MAX_FRAME_QUEUE:
		case VDOENC_PARAM_SKIP_FRAME:
		case VDOENC_PARAM_FRAME_INTERVAL:
		case VDOENC_PARAM_ENCBUF_ADDR:
		case VDOENC_PARAM_ENCBUF_SIZE:
		case VDOENC_PARAM_DIS:                     // 0 : disable, 1: enable
		case VDOENC_PARAM_START_TIMER_BY_MANUAL:
		case VDOENC_PARAM_START_TIMER:
		case VDOENC_PARAM_STOP_TIMER:
		case VDOENC_PARAM_RESET_SEC:
		case VDOENC_PARAM_TIMELAPSE_TIME:
		case VDOENC_PARAM_SYNCSRC:
		case VDOENC_PARAM_BUFINFO_PHYADDR:
		case VDOENC_PARAM_BUFINFO_SIZE:
		case VDOENC_PARAM_MAXMEM_FIT_WORK_MEMORY:  // no need to set to kflow individually, it's just used to pass param to PATH_CONFIG in hdal lib
		case VDOENC_PARAM_REQ_TARGET_I:
		default:
			DBG_ERR("unsupport kflow_videoenc_param(%lu)\r\n", (unsigned long)(kflow_videoenc_param));
			return 0;
	}
}

HD_RESULT _hd_videoenc_kflow_param_set_struct(UINT32 self_id, UINT32 out_id, UINT32 kflow_videoenc_param)
{
	HD_RESULT ret = HD_OK;
	int r = 0;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	UINT32 dev  = self_id-DEV_BASE;
	UINT32 port =  out_id-OUT_BASE;

	cmd.dest = ISF_PORT(self_id, out_id);

	switch (kflow_videoenc_param) {
		/*case VDOENC_PARAM_MAX_MEM_INFO:
		{
		}
		break;
		*/
		case VDOENC_PARAM_LTR:
		{
			//--- LTR ---
			HD_VIDEOENC_OUT2* p_enc_out_param = (HD_VIDEOENC_OUT2*)&videoenc_info[dev].port[port].enc_out_param;

			MP_VDOENC_LTR_INFO ltr_info;

			// from p_enc_out_param
			ltr_info.uiLTRInterval = p_enc_out_param->h26x.ltr_interval;
			ltr_info.uiLTRPreRef   = p_enc_out_param->h26x.ltr_pre_ref;

			cmd.param = VDOENC_PARAM_LTR;
			cmd.value = (UINT32)&ltr_info;
			cmd.size = sizeof(MP_VDOENC_LTR_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_CBRINFO:
		{
			//--- CBR ---
			HD_H26XENC_RATE_CONTROL2* p_enc_rc = (HD_H26XENC_RATE_CONTROL2*)&videoenc_info[dev].port[port].enc_rc;
			UINT32 h26x_rc_gop = videoenc_info[dev].port[port].priv.vendor_set.h26x_rc_gop;

			MP_VDOENC_CBR_INFO cbr_info = {0};

			// from p_enc_rc->cbr
			cbr_info.enable         = 1;
			cbr_info.static_time    = p_enc_rc->cbr.static_time;
			cbr_info.byte_rate      = p_enc_rc->cbr.bitrate/8;
			cbr_info.frame_rate     = ((p_enc_rc->cbr.frame_rate_base * 1000) / p_enc_rc->cbr.frame_rate_incr);
			cbr_info.init_i_qp      = p_enc_rc->cbr.init_i_qp;
			cbr_info.min_i_qp       = p_enc_rc->cbr.min_i_qp;
			cbr_info.max_i_qp       = p_enc_rc->cbr.max_i_qp;
			cbr_info.init_p_qp      = p_enc_rc->cbr.init_p_qp;
			cbr_info.min_p_qp       = p_enc_rc->cbr.min_p_qp;
			cbr_info.max_p_qp       = p_enc_rc->cbr.max_p_qp;
			cbr_info.ip_weight      = p_enc_rc->cbr.ip_weight;
			cbr_info.key_p_period   = p_enc_rc->cbr.key_p_period;
			cbr_info.kp_weight      = p_enc_rc->cbr.kp_weight;
			cbr_info.p2_weight      = p_enc_rc->cbr.p2_weight;
			cbr_info.p3_weight      = p_enc_rc->cbr.p3_weight;
			cbr_info.lt_weight      = p_enc_rc->cbr.lt_weight;
			cbr_info.motion_aq_str  = p_enc_rc->cbr.motion_aq_str;
			cbr_info.max_frame_size = p_enc_rc->cbr.max_frame_size;

			// from p_enc_out_param
			cbr_info.gop            = h26x_rc_gop;

			cmd.param = VDOENC_PARAM_CBRINFO;
			cmd.value = (UINT32)&cbr_info;
			cmd.size = sizeof(MP_VDOENC_CBR_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_EVBRINFO:
		{
			//--- EVBR ---
			HD_H26XENC_RATE_CONTROL2* p_enc_rc = (HD_H26XENC_RATE_CONTROL2*)&videoenc_info[dev].port[port].enc_rc;
			UINT32 h26x_rc_gop = videoenc_info[dev].port[port].priv.vendor_set.h26x_rc_gop;

			MP_VDOENC_EVBR_INFO evbr_info = {0};

			// from p_enc_rc->evbr
			evbr_info.enable            = 1;
			evbr_info.static_time       = p_enc_rc->evbr.static_time;
			evbr_info.byte_rate         = p_enc_rc->evbr.bitrate/8;
			evbr_info.frame_rate        = ((p_enc_rc->evbr.frame_rate_base * 1000) / p_enc_rc->evbr.frame_rate_incr);
			evbr_info.init_i_qp         = p_enc_rc->evbr.init_i_qp;
			evbr_info.min_i_qp          = p_enc_rc->evbr.min_i_qp;
			evbr_info.max_i_qp          = p_enc_rc->evbr.max_i_qp;
			evbr_info.init_p_qp         = p_enc_rc->evbr.init_p_qp;
			evbr_info.min_p_qp          = p_enc_rc->evbr.min_p_qp;
			evbr_info.max_p_qp          = p_enc_rc->evbr.max_p_qp;
			evbr_info.ip_weight         = p_enc_rc->evbr.ip_weight;
			evbr_info.key_p_period      = p_enc_rc->evbr.key_p_period;
			evbr_info.kp_weight         = p_enc_rc->evbr.kp_weight;
			evbr_info.motion_aq_st      = p_enc_rc->evbr.motion_aq_str;
			evbr_info.still_frm_cnd     = p_enc_rc->evbr.still_frame_cnd;
			evbr_info.motion_ratio_thd  = p_enc_rc->evbr.motion_ratio_thd;
			evbr_info.i_psnr_cnd        = p_enc_rc->evbr.still_i_qp;    // It's correct. PSNR parameter have double meaning
			evbr_info.p_psnr_cnd        = p_enc_rc->evbr.still_p_qp;    //  PSNR >= 100 , it means PSNR
			evbr_info.kp_psnr_cnd       = p_enc_rc->evbr.still_kp_qp;   //  PSNR <  100 , it means QP
			evbr_info.p2_weight         = p_enc_rc->evbr.p2_weight;
			evbr_info.p3_weight         = p_enc_rc->evbr.p3_weight;
			evbr_info.lt_weight         = p_enc_rc->evbr.lt_weight;
			evbr_info.max_frame_size    = p_enc_rc->evbr.max_frame_size;

			// from p_enc_out_param
			evbr_info.gop               = h26x_rc_gop;

			cmd.param = VDOENC_PARAM_EVBRINFO;
			cmd.value = (UINT32)&evbr_info;
			cmd.size = sizeof(MP_VDOENC_EVBR_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_VBRINFO:
		{
			//--- VBR ---
			HD_H26XENC_RATE_CONTROL2* p_enc_rc = (HD_H26XENC_RATE_CONTROL2*)&videoenc_info[dev].port[port].enc_rc;
			UINT32 h26x_rc_gop = videoenc_info[dev].port[port].priv.vendor_set.h26x_rc_gop;

			MP_VDOENC_VBR_INFO vbr_info = {0};

			// from p_enc_rc->vbr
			vbr_info.enable         = 1;
			vbr_info.static_time    = p_enc_rc->vbr.static_time;
			vbr_info.byte_rate      = p_enc_rc->vbr.bitrate/8;
			vbr_info.frame_rate     = ((p_enc_rc->vbr.frame_rate_base * 1000) / p_enc_rc->vbr.frame_rate_incr);
			vbr_info.init_i_qp      = p_enc_rc->vbr.init_i_qp;
			vbr_info.min_i_qp       = p_enc_rc->vbr.min_i_qp;
			vbr_info.max_i_qp       = p_enc_rc->vbr.max_i_qp;
			vbr_info.init_p_qp      = p_enc_rc->vbr.init_p_qp;
			vbr_info.min_p_qp       = p_enc_rc->vbr.min_p_qp;
			vbr_info.max_p_qp       = p_enc_rc->vbr.max_p_qp;
			vbr_info.ip_weight      = p_enc_rc->vbr.ip_weight;
			vbr_info.change_pos     = p_enc_rc->vbr.change_pos;
			vbr_info.key_p_period   = p_enc_rc->vbr.key_p_period;
			vbr_info.kp_weight      = p_enc_rc->vbr.kp_weight;
			vbr_info.p2_weight      = p_enc_rc->vbr.p2_weight;
			vbr_info.p3_weight      = p_enc_rc->vbr.p3_weight;
			vbr_info.lt_weight      = p_enc_rc->vbr.lt_weight;
			vbr_info.motion_aq_str  = p_enc_rc->vbr.motion_aq_str;
			vbr_info.max_frame_size = p_enc_rc->vbr.max_frame_size;

			// from p_enc_out_param
			vbr_info.gop            = h26x_rc_gop;

			cmd.param = VDOENC_PARAM_VBRINFO;
			cmd.value = (UINT32)&vbr_info;
			cmd.size = sizeof(MP_VDOENC_VBR_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_FIXQPINFO:
		{
			//--- FIXQP ---
			HD_H26XENC_RATE_CONTROL2* p_enc_rc = (HD_H26XENC_RATE_CONTROL2*)&videoenc_info[dev].port[port].enc_rc;

			MP_VDOENC_FIXQP_INFO fixqp_info = {0};

			// from p_enc_rc->fixqp
			fixqp_info.enable     = 1;
			fixqp_info.fix_i_qp     = p_enc_rc->fixqp.fix_i_qp;
			fixqp_info.fix_p_qp     = p_enc_rc->fixqp.fix_p_qp;
			fixqp_info.frame_rate  = ((p_enc_rc->fixqp.frame_rate_base * 1000) / p_enc_rc->fixqp.frame_rate_incr);   // no use, only sync prj settings

			cmd.param = VDOENC_PARAM_FIXQPINFO;
			cmd.value = (UINT32)&fixqp_info;
			cmd.size = sizeof(MP_VDOENC_FIXQP_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_ROWRC:
		{
			//--- ROWRC ---
			HD_H26XENC_ROW_RC* p_enc_rowrc = (HD_H26XENC_ROW_RC*)&videoenc_info[dev].port[port].enc_row_rc;

			MP_VDOENC_ROWRC_INFO rowrc_info;

			// from p_enc_rowrc
			rowrc_info.enable      = p_enc_rowrc->enable;
			rowrc_info.i_qp_range  = p_enc_rowrc->i_qp_range;
			rowrc_info.i_qp_step   = p_enc_rowrc->i_qp_step;
			rowrc_info.p_qp_range  = p_enc_rowrc->p_qp_range;
			rowrc_info.p_qp_step   = p_enc_rowrc->p_qp_step;
			rowrc_info.min_i_qp    = p_enc_rowrc->min_i_qp;
			rowrc_info.max_i_qp    = p_enc_rowrc->max_i_qp;
			rowrc_info.min_p_qp    = p_enc_rowrc->min_p_qp;
			rowrc_info.max_p_qp    = p_enc_rowrc->max_p_qp;

			cmd.param = VDOENC_PARAM_ROWRC;
			cmd.value = (UINT32)&rowrc_info;
			cmd.size = sizeof(MP_VDOENC_ROWRC_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_AQINFO:
		{
			//--- AQINFO ---
			HD_H26XENC_AQ* p_enc_aq = (HD_H26XENC_AQ*)&videoenc_info[dev].port[port].enc_aq;

			MP_VDOENC_AQ_INFO aq_info;

			// from p_enc_aq
			aq_info.enable       = p_enc_aq->enable;
			aq_info.i_str        = p_enc_aq->i_str;
			aq_info.p_str        = p_enc_aq->p_str;
			aq_info.max_delta_qp = p_enc_aq->max_delta_qp;
			aq_info.min_delta_qp = p_enc_aq->min_delta_qp;

			cmd.param = VDOENC_PARAM_AQINFO;
			cmd.value = (UINT32)&aq_info;
			cmd.size = sizeof(MP_VDOENC_AQ_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_GDR:
		{
			HD_VIDEOENC_STAT cur_state = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status;
			//--- GDR ---
			HD_H26XENC_GDR* p_enc_gdr = (HD_H26XENC_GDR*)&videoenc_info[dev].port[port].enc_gdr;
			MP_VDOENC_GDR_INFO* p_enc_gdr_vendor = (MP_VDOENC_GDR_INFO *)&videoenc_info[dev].port[port].priv.vendor_set.gdr_info;

			MP_VDOENC_GDR_INFO gdr_info;

			// from p_enc_gdr
			gdr_info.enable           = p_enc_gdr->enable;
			gdr_info.period           = p_enc_gdr->period;
			gdr_info.number           = p_enc_gdr->number;
			gdr_info.enable_gdr_i_frm = p_enc_gdr_vendor->enable_gdr_i_frm;
			gdr_info.enable_gdr_qp    = p_enc_gdr_vendor->enable_gdr_qp;
			gdr_info.gdr_qp           = p_enc_gdr_vendor->gdr_qp;

			if (gdr_info.enable_gdr_i_frm) {
				if (cur_state == HD_VIDEOENC_STATUS_START) {
					DBG_ERR("HD_VIDEOENC_PARAM_GDR: could NOT set on START if gdr i-frm enable, please call close->open->set->start !!");
					return HD_ERR_STATE;
				}

				if (cur_state == HD_VIDEOENC_STATUS_OPEN && videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_open_from_init == FALSE) {
					DBG_ERR("HD_VIDEOENC_PARAM_GDR: could NOT set w/o CLOSE if gdr i-frm enable, please call close->open->set->start !!");
					return HD_ERR_STATE;
				}

			}

			cmd.param = VDOENC_PARAM_GDR;
			cmd.value = (UINT32)&gdr_info;
			cmd.size = sizeof(MP_VDOENC_GDR_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_SLICE_SPLIT:
		{
			//--- SLICE SPLIT ---
			HD_H26XENC_SLICE_SPLIT* p_enc_slice_split = (HD_H26XENC_SLICE_SPLIT*)&videoenc_info[dev].port[port].enc_slice_split;

			MP_VDOENC_SLICESPLIT_INFO slice_split_info;

			// from p_enc_slice_split
			slice_split_info.enable        = p_enc_slice_split->enable;
			slice_split_info.slice_row_num = p_enc_slice_split->slice_row_num;


			cmd.param = VDOENC_PARAM_SLICE_SPLIT;
			cmd.value = (UINT32)&slice_split_info;
			cmd.size = sizeof(MP_VDOENC_SLICESPLIT_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_USR_QP_MAP:
		{
			//--- USR QP MAP ---
			HD_H26XENC_USR_QP* p_enc_usr_qp = (HD_H26XENC_USR_QP*)&videoenc_info[dev].port[port].enc_usr_qp;
			HD_VIDEOENC_IN *p_enc_in_dim = &videoenc_info[dev].port[port].enc_in_dim;
			HD_VIDEOENC_OUT2 *p_enc_out_param = &videoenc_info[dev].port[port].enc_out_param;
			//UINT32 mb_w=0;
			UINT32 mb_h=0, loft=0;
			MP_VDOENC_QPMAP_INFO qpmap_info = {0};

			// set mb_w / mb_h / loft
			if (p_enc_out_param->codec_type == HD_CODEC_TYPE_H264) {
				//mb_w = ALIGN_CEIL_16(p_enc_in_dim->dim.w)/16;
				if (p_enc_in_dim->dir == HD_VIDEO_DIR_ROTATE_90 || p_enc_in_dim->dir == HD_VIDEO_DIR_ROTATE_270) {
					mb_h = ALIGN_CEIL_16(p_enc_in_dim->dim.w)/16;
					loft = (ALIGN_CEIL_32(p_enc_in_dim->dim.h)/16);
				} else {
					mb_h = ALIGN_CEIL_16(p_enc_in_dim->dim.h)/16;
					loft = (ALIGN_CEIL_32(p_enc_in_dim->dim.w)/16);
				}
			} else if (p_enc_out_param->codec_type == HD_CODEC_TYPE_H265) {
				//mb_w = ALIGN_CEIL_64(p_enc_in_dim->dim.w)/16;
				if (p_enc_in_dim->dir == HD_VIDEO_DIR_ROTATE_90 || p_enc_in_dim->dir == HD_VIDEO_DIR_ROTATE_270) {
					mb_h = ALIGN_CEIL_64(p_enc_in_dim->dim.w)/16;
					loft = (ALIGN_CEIL_64(p_enc_in_dim->dim.h)/16);
				} else {
					mb_h = ALIGN_CEIL_64(p_enc_in_dim->dim.h)/16;
					loft = (ALIGN_CEIL_64(p_enc_in_dim->dim.w)/16);
				}
			} else {
				//printf("unsupport codec type(%d) for userqp setting.\n", (int)(p_enc_out_param->codec_type));
				//return HD_ERR_PARAM;
				return HD_OK; // just do nothing for jpeg setting userqp
			}

			// from p_enc_usr_qp
			qpmap_info.enable      = p_enc_usr_qp->enable;
			qpmap_info.qp_map_addr = (UINT8 *)p_enc_usr_qp->qp_map_addr;
			qpmap_info.qp_map_size = (loft*mb_h) * sizeof(UINT16);
			qpmap_info.qp_map_loft = (loft) * sizeof(UINT16);

			cmd.param = VDOENC_PARAM_USR_QP_MAP;
			cmd.value = (UINT32)&qpmap_info;
			cmd.size = sizeof(MP_VDOENC_QPMAP_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_DEBLOCK:
		{
			//--- DEBLOCK ---
			HD_H26XENC_DEBLOCK* p_enc_deblock = (HD_H26XENC_DEBLOCK*)&videoenc_info[dev].port[port].enc_deblock;

			MP_VDOENC_INIT db_info;

			// from p_enc_deblock
			db_info.disable_db  = p_enc_deblock->dis_ilf_idc;
			db_info.db_alpha    = p_enc_deblock->db_alpha;
			db_info.db_beta     = p_enc_deblock->db_beta;


			cmd.param = VDOENC_PARAM_DEBLOCK;
			cmd.value = (UINT32)&db_info;
			cmd.size = sizeof(MP_VDOENC_INIT);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_VUI:
		{
			//--- VUI ---
			HD_H26XENC_VUI* p_enc_vui = (HD_H26XENC_VUI*)&videoenc_info[dev].port[port].enc_vui;

			MP_VDOENC_INIT vui_info;

			// from p_enc_vui
			vui_info.bVUIEn                   = p_enc_vui->vui_en;
			vui_info.sar_width                = p_enc_vui->sar_width;
			vui_info.sar_height               = p_enc_vui->sar_height;
			vui_info.matrix_coef              = p_enc_vui->matrix_coef;
			vui_info.transfer_characteristics = p_enc_vui->transfer_characteristics;
			vui_info.colour_primaries         = p_enc_vui->colour_primaries;
			vui_info.video_format             = p_enc_vui->video_format;
			vui_info.color_range              = p_enc_vui->color_range;
			vui_info.time_present_flag        = p_enc_vui->timing_present_flag;


			cmd.param = VDOENC_PARAM_VUI;
			cmd.value = (UINT32)&vui_info;
			cmd.size = sizeof(MP_VDOENC_INIT);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_ROI:
		{
			//--- ROI ---
			HD_H26XENC_ROI* p_enc_roi = (HD_H26XENC_ROI*)&videoenc_info[dev].port[port].enc_roi;

			MP_VDOENC_ROI_INFO roi_info;
			UINT32 i=0;

			// from p_enc_roi
			for (i = 0 ; i < HD_H26XENC_ROI_WIN_COUNT ; i++) {
				roi_info.st_roi[i].enable   = p_enc_roi->st_roi[i].enable;
				roi_info.st_roi[i].qp       = p_enc_roi->st_roi[i].qp;       // fixed: 0 ~ 51, delta: -32 ~ 31
				roi_info.st_roi[i].qp_mode  = p_enc_roi->st_roi[i].mode;
				roi_info.st_roi[i].coord_X  = p_enc_roi->st_roi[i].rect.x;
				roi_info.st_roi[i].coord_Y  = p_enc_roi->st_roi[i].rect.y;
				roi_info.st_roi[i].width    = p_enc_roi->st_roi[i].rect.w;
				roi_info.st_roi[i].height   = p_enc_roi->st_roi[i].rect.h;
			}

			cmd.param = VDOENC_PARAM_ROI;
			cmd.value = (UINT32)&roi_info;
			cmd.size = sizeof(MP_VDOENC_ROI_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_RDO:
		{
			//--- RDO ---
			MP_VDOENC_RDO_INFO *p_rdo_info = (MP_VDOENC_RDO_INFO *)&videoenc_info[dev].port[port].priv.vendor_set.rdo_info;

			cmd.param = VDOENC_PARAM_RDO;
			cmd.value = (UINT32)p_rdo_info;
			cmd.size = sizeof(MP_VDOENC_RDO_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_JND:
		{
			//--- JND ---
			MP_VDOENC_JND_INFO *p_jnd_info = (MP_VDOENC_JND_INFO *)&videoenc_info[dev].port[port].priv.vendor_set.jnd_info;

			cmd.param = VDOENC_PARAM_JND;
			cmd.value = (UINT32)p_jnd_info;
			cmd.size = sizeof(MP_VDOENC_JND_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_H26X_CROP:
		{
			//--- H26X CROP INFO ---
					MP_VDOENC_FRM_CROP_INFO *p_h26x_crop_info = (MP_VDOENC_FRM_CROP_INFO *)&videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info;

			cmd.param = VDOENC_PARAM_H26X_CROP;
			cmd.value = (UINT32)p_h26x_crop_info;
			cmd.size = sizeof(MP_VDOENC_FRM_CROP_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_H26X_SKIP_FRM:
		{
			//--- H26X SKIP FRM ---
			NMI_VDOENC_H26X_SKIP_FRM_INFO *p_skip_frm = (NMI_VDOENC_H26X_SKIP_FRM_INFO *)&videoenc_info[dev].port[port].priv.vendor_set.skip_frm_info;
			cmd.param = VDOENC_PARAM_H26X_SKIP_FRM;
			cmd.value = (UINT32)p_skip_frm;
			cmd.size = sizeof(NMI_VDOENC_H26X_SKIP_FRM_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		}
		break;
		case VDOENC_PARAM_META_ALLOC:
		{
			//--- MATA ALLOC INFO ---
			NMI_VDOENC_META_ALLOC_INFO *p_meta_alloc_info = &videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_info[0];
			UINT32 num = videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_num;

			cmd.param = VDOENC_PARAM_META_ALLOC;
			cmd.value = (UINT32)p_meta_alloc_info;
			cmd.size = sizeof(NMI_VDOENC_META_ALLOC_INFO) * num;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			free_ctx(videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_info);
			videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_info = NULL;
			videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_num = 0;
		}
		break;
		default:
			return HD_ERR_PARAM;
	}


	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:  ret = HD_OK; break;
		default: ret = HD_ERR_SYS; break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			ret = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
			ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return ret;
}

VOID _hd_videoenc_kflow_set_need_update(UINT32 self_id, UINT32 out_id, UINT32 kflow_videoenc_param, BOOL value)
{
	videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_need_update_param[kflow_videoenc_param-KFLOW_VIDEOENC_PARAM_BEGIN] = value;
}

BOOL _hd_videoenc_kflow_get_need_update(UINT32 self_id, UINT32 out_id, UINT32 kflow_videoenc_param)
{
	return videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_need_update_param[kflow_videoenc_param-KFLOW_VIDEOENC_PARAM_BEGIN];
}

HD_RESULT _hd_videoenc_kflow_param_apply_all_settings(UINT32 self_id, UINT32 out_id)
{
	HD_RESULT ret = HD_OK;
	int r = 0;
	UINT32 param_id = 0;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	ISF_SET set[KFLOW_VIDEOENC_PARAM_NUM] = {0};
	UINT32 set_num = 0;

	lock_ctx();
	for (param_id = KFLOW_VIDEOENC_PARAM_BEGIN; param_id < KFLOW_VIDEOENC_PARAM_END; param_id++) {
		if (_hd_videoenc_kflow_get_need_update(self_id, out_id, param_id) == TRUE) {
			if (_hd_videoenc_kflow_param_is_single_value(param_id) == TRUE) {
				// single value settings ... just collect those settings, but set them later
				set[set_num].param = param_id;
				set[set_num].value = _hd_videoenc_kflow_param_get_single_value(self_id, out_id, param_id);
				set_num++;
			} else {
				// struct settings (set each struct at once)
				ret = _hd_videoenc_kflow_param_set_struct(self_id, out_id, param_id);
				if (ret != HD_OK) {
					return ret;
				}
			}

			// reset need_update flag
			_hd_videoenc_kflow_set_need_update(self_id, out_id, param_id, FALSE);
		}
	}
	unlock_ctx();

	// set all single values via MULTI method
	if (set_num > 0) {
		cmd.dest = ISF_PORT(self_id, out_id);
		cmd.param = ISF_UNIT_PARAM_MULTI;
		cmd.value = (UINT32)set;
		cmd.size = sizeof(ISF_SET)*set_num;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  ret = HD_OK; break;
			default: ret = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				ret = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}

	return ret;
}

void _hd_videoenc_kflow_init_need_update(UINT32 self_id, UINT32 out_id)
{
	//set need update (default)
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GOPTYPE,   TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_RECFORMAT, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_MULTI_TEMPORARY_LAYER, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FUNC,      TRUE);

	//set default
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_LTR,       TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CODEC,     TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GOPNUM,    TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ALLOC_SNAPSHOT_BUF, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_PROFILE,   TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ENCBUF_MS, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_SVC,       TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ROI,       TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_AQINFO,    TRUE);
	//_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_USRQP,     TRUE);  //TODO: this affect H265 ( 0 : 64x64, 1 : 16x16 ) , current default = 1 in NMedia
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CHRM_QP_IDX, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_SEC_CHRM_QP_IDX, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_USR_QP_MAP,TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_LEVEL_IDC, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ENTROPY,   TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GRAY,      TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_DEBLOCK,   TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GDR,       TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VUI,       TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_SLICE_SPLIT, TRUE);

	//set CBR as default
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
#if defined(_BSP_NA51055_)
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ROWRC,     TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_RDO,       TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JND,       TRUE);
#endif
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_MIN_I_RATIO, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_MIN_P_RATIO, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COLMV_ENABLE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_RECFRM_ENABLE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_BASE_RECFRM, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SVC_RECFRM, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_LTR_RECFRM, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_LONG_START_CODE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_BS_QUICK_ROLLBACK, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_BR_TOLERANCE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SRCOUT_ENABLE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SRCOUT_ADDR, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SRCOUT_SIZE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_SEI_CHKSUM, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_LOW_POWER, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_CROP, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_BS_RING, TRUE);
	_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_SKIP_FRM, TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

HD_RESULT _hd_videoenc_kflow_param_set_by_vendor(UINT32 self_id, UINT32 out_id, UINT32 kflow_videoenc_param, UINT32 param)
{
	UINT32 dev  = self_id-DEV_BASE;
	UINT32 port =  out_id-OUT_BASE;
	HD_PATH_ID path_id = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(dev), HD_IN(port), HD_OUT(port));
	HD_VIDEOENC_STAT status;

	switch (kflow_videoenc_param) {
		case VDOENC_PARAM_RECFORMAT:               // 1: VID_ONLY ,  2: AUD_VID_BOTH 4: GOLFSHOT , 5: TIMELAPSE , 7:LIVEVIEW
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.rec_format = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_RECFORMAT, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_MIN_I_RATIO:
			hdal_flow_log_p("vendor_videoenc_set(OUT_MIN_RATIO):\n");
			hdal_flow_log_p("    path_id(0x%x) min_i_ratio(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.min_i_ratio = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_MIN_I_RATIO, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_MIN_P_RATIO:
			hdal_flow_log_p("    path_id(0x%x) min_p_ratio(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.min_p_ratio = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_MIN_P_RATIO, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_VBR_MODE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_JPG_RC):\n");
			hdal_flow_log_p("    path_id(0x%x) jpg_vbr_mode(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			videoenc_info[dev].port[port].priv.vendor_set.b_jpg_vbr_mode = TRUE;
			videoenc_info[dev].port[port].priv.vendor_set.jpg_vbr_mode = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_VBR_MODE, TRUE);
			return HD_OK;

		case VDOENC_PARAM_JPG_RC_MIN_QUALITY:
			hdal_flow_log_p("    path_id(0x%x) jpg_rc_min_q(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.jpg_rc_min_q = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_RC_MIN_QUALITY, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_RC_MAX_QUALITY:
			hdal_flow_log_p("    path_id(0x%x) jpg_rc_max_q(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.jpg_rc_max_q = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_RC_MAX_QUALITY, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_YUV_TRAN_ENABLE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_JPG_YUV_TRANS):\n");
			hdal_flow_log_p("    path_id(0x%x) jpg_yuv_trans_en(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.b_jpg_yuv_trans = TRUE;
			videoenc_info[dev].port[port].priv.vendor_set.jpg_yuv_trans_en = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_YUV_TRAN_ENABLE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_RDO: {
			MP_VDOENC_RDO_INFO *p_rdoinfo = 0;
			lock_ctx();
			p_rdoinfo = &videoenc_info[dev].port[port].priv.vendor_set.rdo_info;
			memcpy(p_rdoinfo, (void *)param, sizeof(MP_VDOENC_RDO_INFO));
			hdal_flow_log_p("vendor_videoenc_set(OUT_RDO):\n");
			if (p_rdoinfo->rdo_codec == VDOENC_RDO_CODEC_264) {
				hdal_flow_log_p("    path_id(0x%x) Set RDO (H264), intra[4x4(%u) 8x8(%u) 16x16(%u)], inter[tu4(%u) tu8(%u) skip(%u)]\r\n", (unsigned int)(path_id), (unsigned int)(p_rdoinfo->rdo_param.rdo_264.avc_intra_4x4_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_264.avc_intra_8x8_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_264.avc_intra_16x16_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_264.avc_inter_tu4_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_264.avc_inter_tu8_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_264.avc_inter_skip_cost_bias));
			} else if (p_rdoinfo->rdo_codec == VDOENC_RDO_CODEC_265) {
				hdal_flow_log_p("    path_id(0x%x) Set RDO (H265), intra[32x32(%u) 16x16(%u) 8x8(%u)], inter[skip(%d) merge(%d) 64x64(%u) 64x32_32x64(%u) 32x32(%u) 32x16_16x32(%u) 16x16(%u)]\r\n", (unsigned int)(path_id), (unsigned int)(p_rdoinfo->rdo_param.rdo_265.hevc_intra_32x32_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_265.hevc_intra_16x16_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_265.hevc_intra_8x8_cost_bias), (int)(p_rdoinfo->rdo_param.rdo_265.hevc_inter_skip_cost_bias), (int)(p_rdoinfo->rdo_param.rdo_265.hevc_inter_merge_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_265.hevc_inter_64x64_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_265.hevc_inter_64x32_32x64_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_265.hevc_inter_32x32_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_265.hevc_inter_32x16_16x32_cost_bias), (unsigned int)(p_rdoinfo->rdo_param.rdo_265.hevc_inter_16x16_cost_bias));
			} else {
				unlock_ctx();
				return HD_ERR_PARAM;
			}
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_RDO, TRUE);
			unlock_ctx();
			return HD_OK;
		}

		case VDOENC_PARAM_JND: {
			MP_VDOENC_JND_INFO *p_jndinfo = 0;
			lock_ctx();
			p_jndinfo = &videoenc_info[dev].port[port].priv.vendor_set.jnd_info;
			memcpy(p_jndinfo, (void *)param, sizeof(MP_VDOENC_JND_INFO));
			hdal_flow_log_p("vendor_videoenc_set(OUT_JND):\n");
			hdal_flow_log_p("    path_id(0x%x) Set JND, en(%u) str(%u) level(%u) threshould(%u)\r\n", (unsigned int)(path_id), (unsigned int)(p_jndinfo->enable), (unsigned int)(p_jndinfo->str), (unsigned int)(p_jndinfo->level), (unsigned int)(p_jndinfo->threshold));
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JND, TRUE);
			unlock_ctx();
			return HD_OK;
		}

		case VDOENC_PARAM_H26X_VBR_POLICY:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_VBR_POLICY):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_vbr_polocy(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_vbr_policy = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_VBR_POLICY, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_GOPNUM_ONSTART:
			lock_ctx();
			status = videoenc_info[dev].port[port].priv.status;
			unlock_ctx();
			if (status != HD_VIDEOENC_STATUS_START) {
				DBG_ERR("vendor_videoenc_set(OUT_H26X_ENC_GOP) could only be set on START !! Please call hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_ENC_PARAM) instead!!\r\n");
				return HD_ERR_STATE;
			}
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_ENC_GOP):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_enc_gop(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].enc_out_param.h26x.gop_num = param;   // update enc gop
			videoenc_info[dev].port[port].priv.vendor_set.h26x_rc_gop = param;  // update rc  gop
			videoenc_info[dev].port[port].request_keyframe.enable = TRUE;       // trigger reset-i

			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GOPNUM_ONSTART,  TRUE);
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_RESET_IFRAME,  TRUE);
			{
				// update rc_gop via set RC again
				HD_H26XENC_RATE_CONTROL2 *p_enc_rc = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_rc;

				switch (p_enc_rc->rc_mode)
				{
					case HD_RC_MODE_CBR:
					{
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
					}
					break;
					case HD_RC_MODE_VBR:
					{
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
					}
					break;
					case HD_RC_MODE_EVBR:
					{
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
					}
					break;
					case HD_RC_MODE_FIX_QP:
					{
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, TRUE);
					}
					break;
					default:
						DBG_ERR("ERROR: current rc_mode(%d) is not support\n", (int)(p_enc_rc->rc_mode));
						unlock_ctx();
						return HD_ERR_SYS;
				}
			}
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_RC_GOPNUM:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_RC_GOP):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_rc_gop(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_rc_gop = param;

			{
				// update rc_gop via set RC again
				HD_H26XENC_RATE_CONTROL2 *p_enc_rc = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_rc;

				switch (p_enc_rc->rc_mode)
				{
					case HD_RC_MODE_CBR:
					{
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
					}
					break;
					case HD_RC_MODE_VBR:
					{
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
					}
					break;
					case HD_RC_MODE_EVBR:
					{
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
					}
					break;
					case HD_RC_MODE_FIX_QP:
					{
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, TRUE);
					}
					break;
					default:
						DBG_ERR("ERROR: current rc_mode(%d) is not support\n", (int)(p_enc_rc->rc_mode));
						unlock_ctx();
						return HD_ERR_SYS;
				}
			}
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COLMV_ENABLE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COLMV_ENABLE):\n");
			hdal_flow_log_p("    path_id(0x%x) colmv_enable(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_colmv_en = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COLMV_ENABLE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_RECFRM_ENABLE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COMM_RECFRM_ENABLE):\n");
			hdal_flow_log_p("    path_id(0x%x) comm_recfrm_enable(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_en = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_RECFRM_ENABLE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_BASE_RECFRM:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COMM_BASE_RECFRM):\n");
			hdal_flow_log_p("    path_id(0x%x) comm_base_recfrm(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_base_en = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_BASE_RECFRM, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_SVC_RECFRM:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COMM_SVC_RECFRM):\n");
			hdal_flow_log_p("    path_id(0x%x) comm_svc_recfrm(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_svc_en = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SVC_RECFRM, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_LTR_RECFRM:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COMM_LTR_RECFRM):\n");
			hdal_flow_log_p("    path_id(0x%x) comm_ltr_recfrm(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_ltr_en = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_LTR_RECFRM, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_MAXMEM_FIT_WORK_MEMORY:
			hdal_flow_log_p("vendor_videoenc_set(OUT_FIT_WORK_MEMORY):\n");
			hdal_flow_log_p("    path_id(0x%x) fit_work_memory(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			if (videoenc_info[dev].port[port].priv.b_maxmem_set == TRUE) {
				hdal_flow_log_p("    -> set is FAILED !! must set before hd_videoenc_set(PATH_CONFIG)\n");
				DBG_ERR("vendor_videoenc_set(OUT_FIT_WORK_MEMORY) is FAILED !! must set before hd_videoenc_set(PATH_CONFIG)\n");
				unlock_ctx();
				return HD_ERR_STATE;
			} else {
				videoenc_info[dev].port[port].priv.vendor_set.b_fit_work_memory = (BOOL)param;
				unlock_ctx();
				return HD_OK;
			}

		case VDOENC_PARAM_LONG_START_CODE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_LONG_START_CODE):\n");
			hdal_flow_log_p("    path_id(0x%x) long_start_code(%u)\n", path_id, param);
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.long_start_code = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_LONG_START_CODE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_SVC_WEIGHT_MODE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_SVC_WEIGHT_MODE):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_svc_weight_mode(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_svc_weight_mode = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_SVC_WEIGHT_MODE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_BS_QUICK_ROLLBACK:
			hdal_flow_log_p("vendor_videoenc_set(OUT_BS_QUICK_ROLLBACK):\n");
			hdal_flow_log_p("    path_id(0x%x) bs_quick_rollback(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.b_bs_quick_rollback = (BOOL)param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_BS_QUICK_ROLLBACK, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_QUALITY_BASE_MODE_ENABLE):\n");
			hdal_flow_log_p("	 path_id(0x%x) quality_base_mode_enable(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.b_quality_base_en = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_HW_PADDING_MODE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_HW_PADDING_MODE):\n");
			hdal_flow_log_p("    path_id(0x%x) hw_padding_mode(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.hw_padding_mode = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_HW_PADDING_MODE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_BR_TOLERANCE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_BR_TOLERANCE):\n");
			hdal_flow_log_p("    path_id(0x%x) br_tolerance(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.br_tolerance = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_BR_TOLERANCE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_SRCOUT_ENABLE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COMM_SRCOUT_ENABLE):\n");
			hdal_flow_log_p("    path_id(0x%x) comm_srcout_en(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			if (videoenc_info[dev].port[port].priv.b_maxmem_set == TRUE) {
				hdal_flow_log_p("    -> set is FAILED !! must set before hd_videoenc_set(PATH_CONFIG)\n");
				DBG_ERR("vendor_videoenc_set(COMM_SRCOUT_ENABLE) is FAILED !! must set before hd_videoenc_set(PATH_CONFIG)\n");
				unlock_ctx();
				return HD_ERR_STATE;
			} else {
				if (videoenc_info[dev].port[port].priv.status == HD_VIDEOENC_STATUS_START) {
					hdal_flow_log_p("	 -> set is FAILED !! must set before hd_videoenc_start\n");
					DBG_ERR("vendor_videoenc_set(COMM_SRCOUT_ENABLE) is FAILED !! must set before hd_videoenc_start\n");
					unlock_ctx();
					return HD_ERR_STATE;
				} else {
					videoenc_info[dev].port[port].priv.vendor_set.b_comm_srcout_en = (BOOL)param;
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SRCOUT_ENABLE, TRUE);
					unlock_ctx();
					return HD_OK;
				}
			}

		case VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COMM_SRCOUT_NO_JPG_ENC):\n");
			hdal_flow_log_p("    path_id(0x%x) comm_srcout_no_jpg_enc(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			if (videoenc_info[dev].port[port].priv.b_maxmem_set == TRUE) {
				hdal_flow_log_p("    -> set is FAILED !! must set before hd_videoenc_set(PATH_CONFIG)\n");
				DBG_ERR("vendor_videoenc_set(COMM_SRCOUT_NO_JPG_ENC) is FAILED !! must set before hd_videoenc_set(PATH_CONFIG)\n");
				unlock_ctx();
				return HD_ERR_STATE;
			} else {
				if (videoenc_info[dev].port[port].priv.status == HD_VIDEOENC_STATUS_START) {
					hdal_flow_log_p("	 -> set is FAILED !! must set before hd_videoenc_start\n");
					DBG_ERR("vendor_videoenc_set(COMM_SRCOUT_NO_JPG_ENC) is FAILED !! must set before hd_videoenc_start\n");
					unlock_ctx();
					return HD_ERR_STATE;
				} else {
					videoenc_info[dev].port[port].priv.vendor_set.b_comm_srcout_no_jpg_enc = (BOOL)param;
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC, TRUE);
					unlock_ctx();
					return HD_OK;
				}
			}

		case VDOENC_PARAM_COMM_SRCOUT_ADDR:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COMM_SRCOUT_ADDR):\n");
			hdal_flow_log_p("    path_id(0x%x) comm_srcout_addr(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.comm_srcout_addr = (UINT32)param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SRCOUT_ADDR, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_SRCOUT_SIZE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_COMM_SRCOUT_SIZE):\n");
			hdal_flow_log_p("    path_id(0x%x) comm_srcout_size(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.comm_srcout_size = (UINT32)param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_COMM_SRCOUT_SIZE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_TIMER_TRIG_COMP_ENABLE):\n");
			hdal_flow_log_p("    path_id(0x%x) timer_trig_comp_en(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.b_timer_trig_comp_en = (BOOL)param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_SEI_CHKSUM:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_SEI_CHKSUM):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_sei_chksum(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.b_h26x_sei_chksum= (BOOL)param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_SEI_CHKSUM, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_GDR: {
			MP_VDOENC_GDR_INFO *p_gdrinfo = 0;
			p_gdrinfo = &videoenc_info[dev].port[port].priv.vendor_set.gdr_info;
			memcpy(p_gdrinfo, (void *)param, sizeof(MP_VDOENC_GDR_INFO));
			hdal_flow_log_p("vendor_videoenc_set(OUT_GDR):\n");
			hdal_flow_log_p("    path_id(0x%x) Set GDR, en(%u) period(%u) num(%u) en_ifrm(%u) en_qp(%u) qp(%u)\r\n", (unsigned int)(path_id), (unsigned int)(p_gdrinfo->enable), (unsigned int)(p_gdrinfo->period), (unsigned int)(p_gdrinfo->number), (unsigned int)(p_gdrinfo->enable_gdr_i_frm), (unsigned int)(p_gdrinfo->enable_gdr_qp), (unsigned int)(p_gdrinfo->gdr_qp));
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GDR, TRUE);
			return HD_OK;
		}

		case VDOENC_PARAM_H26X_LOW_POWER:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_LOW_POWER):\n");
			hdal_flow_log_p("	 path_id(0x%x) h26x_low_power(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.b_h26x_low_power = (BOOL)param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_LOW_POWER, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_CROP: {
			KDRV_VDOENC_FRM_CROP_INFO *p_h26x_crop_info = 0;
			lock_ctx();
			p_h26x_crop_info = &videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info;
			memcpy(p_h26x_crop_info, (void *)param, sizeof(KDRV_VDOENC_FRM_CROP_INFO));
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_CROP):\n");
			hdal_flow_log_p("	 path_id(0x%x) Set H26X_CROP, enable(%u) display_width(%u) display_height(%u)\r\n", (unsigned int)(path_id), (unsigned int)(p_h26x_crop_info->frm_crop_enable), (unsigned int)(p_h26x_crop_info->display_w), (unsigned int)(p_h26x_crop_info->display_h));
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_CROP, TRUE);
			unlock_ctx();
			return HD_OK;
		}

		case VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_MAQ_DIFF_ENABLE):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_maq_diff_enable(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.b_h26x_maq_diff_en = (BOOL)param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_MAQ_DIFF_STR:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_MAQ_DIFF_STR):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_maq_diff_str(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_str = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_MAQ_DIFF_STR, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_MAQ_DIFF_START_IDX):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_maq_diff_start_idx(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_start_idx = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX:
			hdal_flow_log_p("vendor_videoenc_set(OUT_H26X_MAQ_DIFF_END_IDX):\n");
			hdal_flow_log_p("    path_id(0x%x) h26x_maq_diff_end_idx(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_end_idx = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_JFIF_ENABLE:
			hdal_flow_log_p("vendor_videoenc_set(OUT_JPG_JFIF):\n");
			hdal_flow_log_p("    path_id(0x%x) jpg_jfif_en(%u)\n", (unsigned int)(path_id), (unsigned int)(param));
			lock_ctx();
			videoenc_info[dev].port[port].priv.vendor_set.b_jpg_jfif = TRUE;
			videoenc_info[dev].port[port].priv.vendor_set.jpg_jfif_en = param;
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_JFIF_ENABLE, TRUE);
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_SKIP_FRM: {
			NMI_VDOENC_H26X_SKIP_FRM_INFO *p_skipfrm = 0;
			lock_ctx();
			p_skipfrm = &videoenc_info[dev].port[port].priv.vendor_set.skip_frm_info;
			memcpy(p_skipfrm, (void *)param, sizeof(NMI_VDOENC_H26X_SKIP_FRM_INFO));
			hdal_flow_log_p("vendor_videoenc_set(OUT_VDOENC_PARAM_H26X_SKIP_FRM):\n");
			hdal_flow_log_p("	 path_id(0x%x) Set H26X_SKIP_FRM, enable(%u) target_fr(%u) input_frm_cnt(%u)\r\n", (unsigned int)(path_id), (unsigned int)(p_skipfrm->enable), (unsigned int)(p_skipfrm->target_fr), (unsigned int)(p_skipfrm->input_frm_cnt));
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_H26X_SKIP_FRM, TRUE);
			unlock_ctx();
			return HD_OK;
		}

		case VDOENC_PARAM_META_ALLOC: {
			NMI_VDOENC_META_ALLOC_INFO *p_user = (NMI_VDOENC_META_ALLOC_INFO *)param;
			NMI_VDOENC_META_ALLOC_INFO *p_meta_alloc_info = NULL;
			lock_ctx();
			{
				UINT32 num = 0;
				while (p_user != NULL) {
					if (num == FLOW_VIDEOENC_META_ALLOC_NUM) {
						DBG_WRN("meta_alloc_max=%d\r\n", FLOW_VIDEOENC_META_ALLOC_NUM);
						break;
					}
					num++;
					p_user = p_user->p_next;
				}
				if (videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_info == NULL) {
					videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_info = (NMI_VDOENC_META_ALLOC_INFO *)malloc_ctx(sizeof(NMI_VDOENC_META_ALLOC_INFO) * num);
					if (videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_info == NULL) {
						DBG_ERR("cannot alloc heap for meta_alloc_info\r\n");
					}
				}
				p_meta_alloc_info = &videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_info[0];
				memcpy(p_meta_alloc_info, (void *)param, sizeof(NMI_VDOENC_META_ALLOC_INFO)*num);
				videoenc_info[dev].port[port].priv.vendor_set.meta_alloc_num = num;
			}
			hdal_flow_log_p("vendor_videoenc_set(META_ALLOC):\n");
			hdal_flow_log_p("	 path_id(0x%x) Set META_ALLOC, pointer(%lu)\r\n", (unsigned int)(path_id), (unsigned long)(p_meta_alloc_info));
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_META_ALLOC, TRUE);
			unlock_ctx();
			return HD_OK;
		}

		default:
			DBG_ERR("unsupport kflow_videoenc_param(%lu)\r\n", (unsigned long)(kflow_videoenc_param));
			return HD_ERR_PARAM;
	}
}

HD_RESULT _hd_videoenc_kflow_param_get_by_vendor(UINT32 self_id, UINT32 out_id, UINT32 kflow_videoenc_param, UINT32 *param)
{
	UINT32 dev  = self_id-DEV_BASE;
	UINT32 port =  out_id-OUT_BASE;

	switch (kflow_videoenc_param) {
		case VDOENC_PARAM_RECFORMAT:               // 1: VID_ONLY ,  2: AUD_VID_BOTH 4: GOLFSHOT , 5: TIMELAPSE , 7:LIVEVIEW
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.rec_format;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_MIN_I_RATIO:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.min_i_ratio;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_MIN_P_RATIO:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.min_p_ratio;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_VBR_MODE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.jpg_vbr_mode;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_RC_MIN_QUALITY:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.jpg_rc_min_q;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_RC_MAX_QUALITY:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.jpg_rc_max_q;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_YUV_TRAN_ENABLE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.jpg_yuv_trans_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_RDO:
			lock_ctx();
			memcpy(param, &videoenc_info[dev].port[port].priv.vendor_set.rdo_info, sizeof(MP_VDOENC_RDO_INFO));
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JND:
			lock_ctx();
			memcpy(param, &videoenc_info[dev].port[port].priv.vendor_set.jnd_info, sizeof(MP_VDOENC_JND_INFO));
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_VBR_POLICY:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_vbr_policy;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_RC_GOPNUM:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_rc_gop;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COLMV_ENABLE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_colmv_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_RECFRM_ENABLE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_BASE_RECFRM:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_base_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_SVC_RECFRM:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_svc_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_LTR_RECFRM:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_comm_recfrm_ltr_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_MAXMEM_FIT_WORK_MEMORY:
			lock_ctx();
			*param = (UINT32)videoenc_info[dev].port[port].priv.vendor_set.b_fit_work_memory;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_LONG_START_CODE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.long_start_code;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_SVC_WEIGHT_MODE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_svc_weight_mode;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_BS_QUICK_ROLLBACK:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.b_bs_quick_rollback;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.b_quality_base_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_HW_PADDING_MODE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.hw_padding_mode;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_BR_TOLERANCE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.br_tolerance;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_SRCOUT_ENABLE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.b_comm_srcout_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.b_comm_srcout_no_jpg_enc;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_SRCOUT_ADDR:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.comm_srcout_addr;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_COMM_SRCOUT_SIZE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.comm_srcout_size;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE:
			lock_ctx();
			*param = (UINT32)videoenc_info[dev].port[port].priv.vendor_set.b_timer_trig_comp_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_SEI_CHKSUM:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.b_h26x_sei_chksum;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_GDR:
			lock_ctx();
			memcpy(param, &videoenc_info[dev].port[port].priv.vendor_set.gdr_info, sizeof(MP_VDOENC_GDR_INFO));
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_LOW_POWER:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.b_h26x_low_power;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_CROP:
			lock_ctx();
			memcpy(param, &videoenc_info[dev].port[port].priv.vendor_set.h26x_crop_info, sizeof(MP_VDOENC_FRM_CROP_INFO));
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE:
			lock_ctx();
			*param = (UINT32)videoenc_info[dev].port[port].priv.vendor_set.b_h26x_maq_diff_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_MAQ_DIFF_STR:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_str;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_start_idx;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.h26x_maq_diff_end_idx;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_JPG_JFIF_ENABLE:
			lock_ctx();
			*param = videoenc_info[dev].port[port].priv.vendor_set.jpg_jfif_en;
			unlock_ctx();
			return HD_OK;

		case VDOENC_PARAM_H26X_SKIP_FRM:
			lock_ctx();
			memcpy(param, &videoenc_info[dev].port[port].priv.vendor_set.skip_frm_info, sizeof(NMI_VDOENC_H26X_SKIP_FRM_INFO));
			unlock_ctx();
			return HD_OK;

		default:
			DBG_ERR("unsupport kflow_videoenc_param(%lu)\r\n", (unsigned long)(kflow_videoenc_param));
			return HD_ERR_PARAM;
	}
}

INT _hd_videoenc_modify_params_on_start(UINT32 self_id, UINT32 out_id, CHAR *p_ret_string, INT max_str_len)
{
	UINT32 dev  = self_id-DEV_BASE;
	UINT32 port =  out_id-OUT_BASE;

	//set JPEG bitrate same as PATH_CONFIG.bitrate , JIRA: IVOT_N00025-1489 ( user may set JPEG encode, but also set OUT_RATE_CONTROL ... this will let nmediarec use wrong min i/p size to check bs buffer )
	{
		HD_VIDEO_CODEC codec = videoenc_info[dev].port[port].enc_out_param.codec_type;
		HD_H26XENC_RATE_CONTROL2 *rc_param = &videoenc_info[dev].port[port].enc_rc;
		HD_VIDEOENC_PATH_CONFIG *path_cfg_param = &videoenc_info[dev].port[port].enc_path_cfg;

		if (codec == HD_CODEC_TYPE_JPEG) {
			// set JPEG bitrate same as PATH_CONFIG.bitrate
			rc_param->rc_mode     = HD_RC_MODE_CBR;
			rc_param->cbr.bitrate = path_cfg_param->max_mem.bitrate;
			// set CBR
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   TRUE);
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
			_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

HD_RESULT hd_videoenc_init(VOID)
{
	UINT32 dev, port;
	UINT32 self_id, out_id;
	UINT32 cfg, cfg1, cfg2;
	UINT32 gdc_first=0;
	HD_RESULT rv;
	UINT32 i;

	hdal_flow_log_p("hd_videoenc_init\n");

	if (dev_fd <= 0) {
		return HD_ERR_INIT;
	}

	if (_hd_common_get_pid() > 0) { // client process
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = 0;
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("init fail? %d\r\n", (int)(r));
			return r;
		}
		if (!_hd_common_is_new_flow()) {
			return HD_OK;
		}
	}

	if (_hd_common_get_pid() == 0) {

	rv = _hd_common_get_init(&cfg);
	if (rv != HD_OK) {
		return HD_ERR_NO_CONFIG;
	}
	rv = hd_common_get_sysconfig(&cfg1, &cfg2);
	if (rv != HD_OK) {
		return HD_ERR_NO_CONFIG;
	}

	g_cfg_cpu   = (cfg & HD_COMMON_CFG_CPU);
	g_cfg_debug = ((cfg & HD_COMMON_CFG_DEBUG) >> 12);
	g_cfg_venc  = ((cfg1 & HD_VIDEOENC_CFG) >> 8);
	g_cfg_vprc  = ((cfg1 & HD_VIDEOPROC_CFG) >> 16);
#if defined(_BSP_NA51055_)
	g_cfg_ai = ((cfg2 & VENDOR_AI_CFG) >> 16);
#endif

#if defined(_BSP_NA51055_)
	if (g_cfg_ai & (VENDOR_AI_CFG_ENABLE_CNN >> 16)) { //if enable ai CNN2
		UINT32 stripe_rule = g_cfg_vprc & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
		if (stripe_rule > 3) {
			stripe_rule = 0;
		}
		if ((stripe_rule == 0)||(stripe_rule == 3)) { //NOT disable GDC?
			//DBG_DUMP("already enable CNN => current vprc config stripe_rule = LV%lu, enforce to LV2!", (unsigned long)(stripe_rule+1));
			g_cfg_vprc |= (HD_VIDEOPROC_CFG_STRIP_LV2 >> 16); //disable GDC for AI
		} else {
			//DBG_DUMP("already enable CNN => current vprc config stripe_rule = LV%lu", (unsigned long)(stripe_rule+1));
		}
	}
	{
		UINT32 stripe_rule = g_cfg_vprc & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
		if (stripe_rule > 3) {
			stripe_rule = 0;
		}
		if ((stripe_rule == 1)||(stripe_rule == 2)) { //if vprc disable GDC
		    gdc_first = 0; //enable "low-latency first" stripe policy: tile cut on 2048
		    //DBG_DUMP("vprc config stripe_rule = LV%lu => venc config GDC off, tile cut if w>2048", (unsigned long)(stripe_rule+1));
		} else {
		    gdc_first = 1; //enable "GDC first" stripe policy: tile cut on 1280
		    //DBG_DUMP("vprc config stripe_rule = LV%lu => venc config GDC on, tile cut if w>1280", (unsigned long)(stripe_rule+1));
		}
	}
#endif

	}

	if (videoenc_info[0].port != NULL) {
		DBG_ERR("already init\n");
		return HD_ERR_UNINIT;
	}

	if (_max_path_count == 0) {
		DBG_ERR("_max_path_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	_hd_venc_csz = sizeof(VDOENC_INFO_PORT)*_max_path_count;
	videoenc_info[0].port = (VDOENC_INFO_PORT*)malloc_ctx(_hd_venc_csz);
	if (videoenc_info[0].port == NULL) {
		DBG_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
		return HD_ERR_HEAP;
	}

	if (_hd_common_get_pid() == 0) {

		//only clear 0 by pid 0

		lock_ctx();
		for(i = 0; i < _max_path_count; i++) {
			memset(&(videoenc_info[0].port[i]), 0, sizeof(VDOENC_INFO_PORT));  // no matter what status now...reset status = [UNINIT] anyway
		}
		unlock_ctx();
	}

	//init drv
	if (_hd_common_get_pid() == 0) {
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = _max_path_count;
		isf_dev_cmd.p2 = (g_cfg_cpu << 28) | (g_cfg_debug << 24) | (gdc_first << 4) | (g_cfg_venc);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("init fail? %d\r\n", (int)(r));
			return r;
		}
	}

	if (_hd_common_get_pid() == 0) {

	//only reset default by pid 0

	// init lib - default
	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {

			self_id = (dev +DEV_BASE);
			out_id  = (port+OUT_BASE);

			lock_ctx();
			// init default params
			_hd_videoenc_fill_all_default_params(self_id, out_id);

			// set init "need update"
			_hd_videoenc_kflow_init_need_update(self_id, out_id);

			// set status
			videoenc_info[dev].port[port].priv.status = HD_VIDEOENC_STATUS_INIT;  // [UNINT] -> [INIT]
			unlock_ctx();
		}
	}

	}

	return HD_OK;
}

HD_RESULT hd_videoenc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoenc_bind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) dst(0x%x) in(%d)\n", (unsigned int)(self_id), (int)(out_id), (unsigned int)(dest_id), (int)(in_id));

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_videoenc_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_BIND_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
		cmd.src = ISF_PORT(self_id, out_id);
		_HD_CONVERT_DEST_IN_ID(dest_id, in_id, rv);	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_DEST_ID(dest_id, rv); 	if(rv != HD_OK) {	return rv;}
		cmd.dest = ISF_PORT(dest_id, in_id);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_BIND, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_videoenc_unbind(HD_OUT_ID _out_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoenc_unbind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d)\n", (unsigned int)(self_id), (int)(out_id));

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_videoenc_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_BIND_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.dest = ISF_PORT_NULL;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_BIND, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT _hd_videoenc_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
{
	HD_DAL self_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if(!p_src_out_id) {
		return HD_ERR_NULL_PTR;
	}


	{
		ISF_FLOW_IOCTL_BIND_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
		cmd.src = ISF_PORT(self_id, in_id);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_BIND, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				{
					UINT32 dev_id = GET_HI_UINT16(cmd.dest);
					UINT32 out_id = GET_LO_UINT16(cmd.dest);
					_HD_REVERT_BIND_ID(dev_id);
					_HD_REVERT_OUT_ID(out_id);
					p_src_out_id[0] = (((dev_id) << 16) | ((out_id) & 0x00ff));
				}
				rv = HD_OK;
				break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}


HD_RESULT _hd_videoenc_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if(!p_dest_in_id) {
		return HD_ERR_NULL_PTR;
	}


	{
		ISF_FLOW_IOCTL_BIND_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
		cmd.src = ISF_PORT(self_id, out_id);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_BIND, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				{
					UINT32 dev_id = GET_HI_UINT16(cmd.dest);
					UINT32 in_id = GET_LO_UINT16(cmd.dest);
					_HD_REVERT_BIND_ID(dev_id);
					_HD_REVERT_IN_ID(in_id);
					p_dest_in_id[0] = (((dev_id) << 16) | (((in_id) & 0x00ff) << 8));
				}
				rv = HD_OK;
				break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT _hd_videoenc_get_state(HD_OUT_ID _out_id, UINT32* p_state)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if(!p_state) {
		return HD_ERR_NULL_PTR;
	}


	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		cmd.src = ISF_PORT(self_id, out_id);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_STATE, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				p_state[0] = cmd.state;
				rv = HD_OK;
				break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_videoenc_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	HD_VIDEOENC_STAT status;

	hdal_flow_log_p("hd_videoenc_open:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) in(%d) ", (unsigned int)(self_id), (int)(out_id), (int)(in_id));

	if (dev_fd <= 0) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_videoenc_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}
	if(!p_path_id) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_NULL_PTR;
	}

	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		_HD_CONVERT_SELF_ID(self_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((self_id) - HD_DEV_BASE)), (unsigned long)((UINT32)DEV_COUNT));
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		p_path_id[0] = _out_id;
		hdal_flow_log_p("path_id(0x%x)\n", (unsigned int)(p_path_id[0]));
		//do nothing
		return HD_OK;
	} else {
		HD_IO osg_in_id = in_id, osg_out_id = out_id;
		_HD_CONVERT_OSG_ID(osg_in_id);
		_HD_CONVERT_OSG_ID(osg_out_id);
		if(osg_in_id) {
			UINT32 cur_dev = self_id;
			HD_IO path_out_id = out_id;
			_HD_CONVERT_SELF_ID(self_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((self_id) - HD_DEV_BASE)), (unsigned long)((UINT32)DEV_COUNT));
				hdal_flow_log_p("path_id(N/A)\n");
				return rv;
			}
			_HD_CONVERT_OUT_ID(out_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("out_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((out_id) - HD_OUT_BASE)), (unsigned long)((UINT32)_max_path_count));
				hdal_flow_log_p("path_id(N/A)\n");
				return rv;
			}
			p_path_id[0] = HD_VIDEOENC_PATH(cur_dev, in_id, path_out_id); //osg_id + out_id
			hdal_flow_log_p("path_id(0x%x)\n", (unsigned int)(p_path_id[0]));
			return _hd_osg_attach(self_id, out_id, osg_in_id);
		} else if(osg_out_id) {
			UINT32 cur_dev = in_dev_id;
			HD_IO path_in_id = in_id;
			_HD_CONVERT_SELF_ID(in_dev_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((in_dev_id) - HD_DEV_BASE)), (unsigned long)((UINT32)DEV_COUNT));
				hdal_flow_log_p("path_id(N/A)\n");
				return rv;
			}
			_HD_CONVERT_IN_ID(in_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("in_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((in_id) - HD_IN_BASE)), (unsigned long)((UINT32)_max_path_count));
				hdal_flow_log_p("path_id(N/A)\n");
				return rv;
			}
			p_path_id[0] = HD_VIDEOENC_PATH(cur_dev, path_in_id, out_id); //in_id + osg_id
			hdal_flow_log_p("path_id(0x%x)\n", (unsigned int)(p_path_id[0]));
			return _hd_osg_attach(in_dev_id, in_id, osg_out_id);
		} else {
			UINT32 cur_dev = self_id;
			HD_IO check_in_id = in_id;
			HD_IO check_out_id = out_id;
			_HD_CONVERT_SELF_ID(self_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((self_id) - HD_DEV_BASE)), (unsigned long)((UINT32)DEV_COUNT));
				hdal_flow_log_p("path_id(N/A)\n");
				return rv;
			}
			_HD_CONVERT_OUT_ID(check_out_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("out_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((check_out_id) - HD_OUT_BASE)), (unsigned long)((UINT32)_max_path_count));
				hdal_flow_log_p("path_id(N/A)\n");
				return rv;
			}
			_HD_CONVERT_SELF_ID(in_dev_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((in_dev_id) - HD_DEV_BASE)), (unsigned long)((UINT32)DEV_COUNT));
				hdal_flow_log_p("path_id(N/A)\n");
				return rv;
			}
			_HD_CONVERT_IN_ID(check_in_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("in_id=%lu is larger than max=%lu!\r\n", (unsigned long)((UINT32)((check_in_id) - HD_IN_BASE)), (unsigned long)((UINT32)_max_path_count));
				hdal_flow_log_p("path_id(N/A)\n");
				return rv;
			}
			#if 0  // CID 137136 (#1 of 1): Logically dead code (DEADCODE) => venc only 1 device, so if pass up 2 _HD_CONVERT_SELF_ID() line MUST (in_dev_id = self_id)
			if(in_dev_id != self_id) { hdal_flow_log_p("path_id(N/A)\n"); return HD_ERR_DEV;}
			#endif
			p_path_id[0] = HD_VIDEOENC_PATH(cur_dev, in_id, out_id); //in_id + out_id
		}
	}


	_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { hdal_flow_log_p("path_id(N/A)\n"); return rv;}

	lock_ctx();
	status = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status;
	unlock_ctx();
	// check status
	if (status == HD_VIDEOENC_STATUS_UNINIT) {
		DBG_ERR("FATAL ERROR : module is NOT init yet, please call hd_videoenc_init() first !!!\r\n");
		p_path_id[0] = 0;
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_UNINIT;
	}
	// isf flow will check the status , so vdoenc don't need to check the status here
	/* else if ((status == HD_VIDEOENC_STATUS_OPEN) ||
	           (status == HD_VIDEOENC_STATUS_START)){
		hdal_flow_log_p("path_id(0x%x)\n", (unsigned int)(p_path_id[0]));
		DBG_ERR("module is already opened...ignore request !!\r\n");
		return HD_ERR_ALREADY_OPEN;
	}
	*/

	hdal_flow_log_p("path_id(0x%x)\n", (unsigned int)(p_path_id[0]));

	// call updateport - open
	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};

		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	if (rv == HD_OK) {
		lock_ctx();
		videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_VIDEOENC_STATUS_OPEN;   // [INIT] -> [OPEN]
		videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_open_from_init = TRUE;
		unlock_ctx();
	}
	return rv;
}

HD_RESULT hd_videoenc_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	HD_VIDEOENC_STAT status;

	hdal_flow_log_p("hd_videoenc_start:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", (unsigned int)(path_id));

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_videoenc_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	if(ctrl_id == HD_CTRL) {
		return HD_ERR_NOT_SUPPORT;
	} else {
		HD_IO osg_in_id = in_id, osg_out_id = out_id;
		_HD_CONVERT_OSG_ID(osg_in_id);
		_HD_CONVERT_OSG_ID(osg_out_id);
		if(osg_in_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_enable(self_id, out_id, osg_in_id);
		} else if(osg_out_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_enable(self_id, in_id, osg_out_id);
		}
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

		lock_ctx();
		//--- check input parameter valid(those who can only be checked on START) ---
		{
			CHAR msg_string[256];
			if (_hd_videoenc_check_params_range_on_start(self_id, out_id, msg_string, 256) < 0) {
				DBG_ERR("Wrong value. %s\n", msg_string);
				unlock_ctx();
				return HD_ERR_PARAM;
			}
		}

		//--- check if MAX_MEM info had been set ---
		if (videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set == FALSE) {
			DBG_ERR("FATAL ERROR: please set HD_VIDEOENC_PARAM_PATH_CONFIG for max_mem first !!!\r\n");
			unlock_ctx();
			return HD_ERR_NO_CONFIG;
		}
		unlock_ctx();

		lock_ctx();
		status = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status;
		unlock_ctx();
		//--- check status ---
		if (status == HD_VIDEOENC_STATUS_UNINIT) {
			DBG_ERR("FATAL ERROR : module is NOT init & open yet, please call hd_videoenc_init() and hd_videoenc_open() first !!!\r\n");
			return HD_ERR_UNINIT;
		} else if (status == HD_VIDEOENC_STATUS_INIT){
			DBG_ERR("FATAL ERROR : module is NOT open yet, please call hd_videoenc_open() first !!!\r\n");
			return HD_ERR_NOT_OPEN;
		}

		lock_ctx();
		//--- modify param on start ---
		{
			CHAR msg_string[256];
			if (_hd_videoenc_modify_params_on_start(self_id, out_id, msg_string, 256) < 0) {
				DBG_ERR("Wrong value. %s\n", msg_string);
				unlock_ctx();
				return HD_ERR_PARAM;
			}
		}
		unlock_ctx();

		//--- set IN.frc again ---   // JIRA : NA51084-279 , user may set RC via stop-set-start , let timer_mode interval be changed.. so set IN.frc again to prevent it
		if (videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status == HD_VIDEOENC_STATUS_OPEN) {
			HD_VIDEOENC_IN *p_enc_in_dim = &videoenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_in_dim;
			ISF_VDO_FPS fps = {0};
			ISF_FLOW_IOCTL_PARAM_ITEM cmd_p = {0};

			cmd_p.dest = ISF_PORT(self_id, in_id);
			fps.framepersecond = p_enc_in_dim->frc ;
			cmd_p.param = ISF_UNIT_PARAM_VDOFPS;
			cmd_p.value = (UINT32)&fps;
			cmd_p.size = sizeof(ISF_VDO_FPS);

			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd_p);
			if (r != 0) {
				DBG_ERR("auto-set in.frc again FAILED.\r\n");
			}
		}

		//--- apply all settings ---
		rv = _hd_videoenc_kflow_param_apply_all_settings(self_id, out_id);
		if(rv != HD_OK) {	return rv;}

		//--- updateport to START ---
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 1;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	lock_ctx();
	if (rv == HD_OK) {
		videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_VIDEOENC_STATUS_START;
	}

	{
		// get physical addr/size of bs buffer
		HD_BUFINFO *p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_buf_info.buf_info;
		_hd_get_param(self_id, out_id, VDOENC_PARAM_BUFINFO_PHYADDR, &p_bufinfo->phy_addr);
		_hd_get_param(self_id, out_id, VDOENC_PARAM_BUFINFO_SIZE,    &p_bufinfo->buf_size);
	}
	unlock_ctx();
	return rv;
}

HD_RESULT hd_videoenc_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	HD_VIDEOENC_STAT        status;

	hdal_flow_log_p("hd_videoenc_stop:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", (unsigned int)(path_id));

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_videoenc_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	if(ctrl_id == HD_CTRL) {
		return HD_ERR_NOT_SUPPORT;
	} else {
		HD_IO osg_in_id = in_id, osg_out_id = out_id;
		_HD_CONVERT_OSG_ID(osg_in_id);
		_HD_CONVERT_OSG_ID(osg_out_id);
		if(osg_in_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_disable(self_id, out_id, osg_in_id);
		} else if(osg_out_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_disable(self_id, in_id, osg_out_id);
		}
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		lock_ctx();
		status = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status;
		unlock_ctx();
		//--- check status ---
		if (status != HD_VIDEOENC_STATUS_START) {
			DBG_ERR("module is NOT start yet, ignore stop request.... \r\n");
			return HD_ERR_NOT_START;
		}

		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 2;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	lock_ctx();
	if (rv == HD_OK) {
		videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_VIDEOENC_STATUS_OPEN; // [START] -> [OPEN]
		videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_open_from_init = FALSE;
	}
	unlock_ctx();
	return rv;
}

HD_RESULT hd_videoenc_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoenc_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoenc_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	HD_VIDEOENC_STAT        status;

	hdal_flow_log_p("hd_videoenc_close:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", (unsigned int)(path_id));

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_videoenc_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	if(ctrl_id == HD_CTRL) {
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		//do nothing
		return HD_OK;
	} else {
		HD_IO osg_in_id = in_id, osg_out_id = out_id;
		_HD_CONVERT_OSG_ID(osg_in_id);
		_HD_CONVERT_OSG_ID(osg_out_id);
		if(osg_in_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_detach(self_id, out_id, osg_in_id);
		} else if(osg_out_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_detach(self_id, in_id, osg_out_id);
		}
	}

	_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

	lock_ctx();
	status = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status;
	unlock_ctx();
	//--- check status ---
	if (status == HD_VIDEOENC_STATUS_START) {
		DBG_WRN("module is START now, auto call hd_videoenc_stop() before attemp to close.... !! \r\n");
		if (HD_OK !=hd_videoenc_stop(path_id)) {
			DBG_ERR("auto call hd_videoenc_stop() fail ...!!\r\n");
			return HD_ERR_NG;
		}
	} else if ((status == HD_VIDEOENC_STATUS_UNINIT) ||
	           (status == HD_VIDEOENC_STATUS_INIT)) {
	    DBG_ERR("module is NOT open yet... ignore close request !! \r\n");
		return HD_ERR_NOT_OPEN;
	}

	{
		lock_ctx();
		// release MAX_MEM
		if (videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set == TRUE) {
			ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
			NMI_VDOENC_MAX_MEM_INFO max_mem = {0};

			max_mem.bRelease = TRUE;

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = VDOENC_PARAM_MAX_MEM_INFO;
			cmd.value = (UINT32)&max_mem;
			cmd.size = sizeof(NMI_VDOENC_MAX_MEM_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r == 0) {
				videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = FALSE;
			} else {
				DBG_ERR("Release memory FAIL ... !!\r\n");
			}
		}
		unlock_ctx();

		// call updateport - stop
		{
			ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};

			cmd.src = ISF_PORT(self_id, out_id);
			cmd.state = 3;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
			if (r == 0) {
				switch(cmd.rv) {
				case ISF_OK:  rv = HD_OK; break;
				default: rv = HD_ERR_SYS; break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
		}
	}
	if (rv == HD_OK) {
		lock_ctx();
		videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_VIDEOENC_STATUS_INIT;  // [OPEN] -> [INIT]

		// reset default params
		_hd_videoenc_fill_all_default_params(self_id, out_id);

		// set init "need update" , so next time start() will set those default setting to kernel
		_hd_videoenc_kflow_init_need_update(self_id, out_id);
		unlock_ctx();
	}
	return rv;
}

HD_RESULT hd_videoenc_get(HD_PATH_ID path_id, HD_VIDEOENC_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	//int r;

	{
		CHAR  param_name[20];
		_hd_videoenc_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videoenc_get(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", (unsigned int)(path_id));
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() > 0) && (id != HD_VIDEOENC_PARAM_BUFINFO)) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if ((_hd_common_get_pid() == 0) && (_hd_videoenc_is_init() == FALSE)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_videoenc_is_init() == FALSE) {
			return HD_ERR_UNINIT;
		}
	}
	if (p_param == 0) {
		return HD_ERR_NULL_PTR;
	}

	{
		HD_IO osg_in_id = in_id, osg_out_id = out_id;
		_HD_CONVERT_OSG_ID(osg_in_id);
		_HD_CONVERT_OSG_ID(osg_out_id);
		if(osg_in_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			hdal_flow_log_p("\n");
			return _hd_osg_get(self_id, out_id, osg_in_id, id, p_param);
		} else if(osg_out_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
			hdal_flow_log_p("\n");
			return _hd_osg_get(self_id, in_id, osg_out_id, id, p_param);
		}
	}

	{
		//ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		rv = HD_OK;
		if(ctrl_id == HD_CTRL) {
			switch(id) {

			case HD_VIDEOENC_PARAM_DEVCOUNT:
			{
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
				p_user->max_dev_count = 1;

				hdal_flow_log_p("max_dev_cnt(%u)\n", (unsigned int)(p_user->max_dev_count));

				rv = HD_OK;
			}
			break;
			case HD_VIDEOENC_PARAM_SYSCAPS:
			{
				HD_VIDEOENC_SYSCAPS* p_user = (HD_VIDEOENC_SYSCAPS*)p_param;
				int  i;
				memset(p_user, 0, sizeof(HD_VIDEOENC_SYSCAPS));
				p_user->dev_id        = self_id - HD_DEV_BASE + DEV_BASE;
				p_user->chip_id       = 0;
				p_user->max_in_count  = 16;
				p_user->max_out_count = 16;
				p_user->dev_caps =
					HD_CAPS_PATHCONFIG;
				for (i=0; i<16; i++) p_user->in_caps[i] =
					 HD_VIDEO_CAPS_YUV420
					 | HD_VIDEO_CAPS_YUV422
					 | HD_VIDEO_CAPS_COMPRESS
					 | HD_VIDEO_CAPS_DIR_ROTATER
					 | HD_VIDEO_CAPS_DIR_ROTATEL
#if defined(_BSP_NA51055_)
					 | HD_VIDEOENC_INCAPS_ONEBUF
					 | HD_VIDEOENC_INCAPS_LOWLATENCY
#endif
					 | HD_VIDEO_CAPS_STAMP
					 | HD_VIDEO_CAPS_MASK;
				for (i=0; i<16; i++) p_user->out_caps[i] =
					HD_VIDEOENC_CAPS_JPEG
					 | HD_VIDEOENC_CAPS_H264
					 | HD_VIDEOENC_CAPS_H265;
				p_user->max_in_stamp    = 32;
				p_user->max_in_stamp_ex = 64;
				p_user->max_in_mask     = 8;
				p_user->max_in_mask_ex  = 16;

				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", (unsigned int)(p_user->dev_id), (unsigned int)(p_user->chip_id), (unsigned int)(p_user->max_in_count), (unsigned int)(p_user->max_out_count), (unsigned int)(p_user->dev_caps));
				hdal_flow_log_p("    max_in_stamp(%u) max_in_stamp_ex(%u) max_in_mask(%u) max_in_mask_ex(%u)\n", (unsigned int)(p_user->max_in_stamp), (unsigned int)(p_user->max_in_stamp_ex), (unsigned int)(p_user->max_in_mask), (unsigned int)(p_user->max_in_mask_ex));
				{
					int i;
					hdal_flow_log_p("         in_caps    , out_caps =>\n");
					for (i=0; i<16; i++) {
						hdal_flow_log_p("    [%02d] 0x%08x , 0x%08x\n", (int)(i), (unsigned int)(p_user->in_caps[i]), (unsigned int)(p_user->out_caps[i]));
					}
				}

				rv = HD_OK;
			}
			break;
			default: rv = HD_ERR_PARAM; break;
			}
		} else {
			switch(id) {

			case HD_VIDEOENC_PARAM_IN:
				{
					if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEOENC_IN *p_user = (HD_VIDEOENC_IN*)p_param;
					HD_VIDEOENC_IN *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_in_dim;
					memcpy(p_user, p_bufinfo, sizeof(HD_VIDEOENC_IN));
					unlock_ctx();

					hdal_flow_log_p("dim(%ux%u) pxl_fmt(0x%08x) dir(0x%08x) frc(%u/%u)\n", (unsigned int)(p_user->dim.w), (unsigned int)(p_user->dim.h), (unsigned int)(p_user->pxl_fmt), (unsigned int)(p_user->dir), (unsigned int)(GET_HI_UINT16(p_user->frc)), (unsigned int)(GET_LO_UINT16(p_user->frc)));
				}
				break;

			case HD_VIDEOENC_PARAM_IN_FRC:
				{
					if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEOENC_FRC *p_user = (HD_VIDEOENC_FRC*)p_param;
					HD_VIDEOENC_IN *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_in_dim;
					p_user->frc = p_bufinfo->frc;
					unlock_ctx();

					hdal_flow_log_p("frc(%u/%u)\n", (unsigned int)(GET_HI_UINT16(p_user->frc)), (unsigned int)(GET_LO_UINT16(p_user->frc)));
				}
				break;

			case HD_VIDEOENC_PARAM_PATH_CONFIG:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEOENC_PATH_CONFIG *p_user = (HD_VIDEOENC_PATH_CONFIG*)p_param;
					HD_VIDEOENC_PATH_CONFIG *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_path_cfg;
					memcpy(p_user, p_bufinfo, sizeof(HD_VIDEOENC_PATH_CONFIG));
					unlock_ctx();

					hdal_flow_log_p("isp_id(%u) codec(%u) dim(%ux%u) bitrate(%u) enc_buf_ms(%u) svc(%u) ltr(%u) rotate(%u) sout(%u)\n", (unsigned int)(p_user->isp_id), (unsigned int)(p_user->max_mem.codec_type), (unsigned int)(p_user->max_mem.max_dim.w), (unsigned int)(p_user->max_mem.max_dim.h), (unsigned int)(p_user->max_mem.bitrate), (unsigned int)(p_user->max_mem.enc_buf_ms), (unsigned int)(p_user->max_mem.svc_layer), (unsigned int)(p_user->max_mem.ltr), (unsigned int)(p_user->max_mem.rotate), (unsigned int)(p_user->max_mem.rotate));
				}
				break;

			case HD_VIDEOENC_PARAM_FUNC_CONFIG:
				{
					if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEOENC_FUNC_CONFIG *p_user = (HD_VIDEOENC_FUNC_CONFIG*)p_param;
					HD_VIDEOENC_FUNC_CONFIG *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_func_cfg;
					memcpy(p_user, p_bufinfo, sizeof(HD_VIDEOENC_FUNC_CONFIG));
					unlock_ctx();

					hdal_flow_log_p("ddr_id(%u) in_func(%u)\n", (unsigned int)(p_user->ddr_id), (unsigned int)(p_user->in_func));
				}
				break;

			case HD_VIDEOENC_PARAM_BUFINFO:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEOENC_BUFINFO *p_user = (HD_VIDEOENC_BUFINFO*)p_param;

					if (_hd_common_get_pid() == 0) { // main process
						HD_VIDEOENC_STAT status;

						lock_ctx();
						status = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status;
						unlock_ctx();
						//check OPEN status first
						if (status != HD_VIDEOENC_STATUS_START) {
							DBG_ERR("HD_VIDEOENC_PARAM_BUFINFO could only be get after START\r\n");
							return HD_ERR_NOT_START;
						}
						HD_VIDEOENC_BUFINFO *p_bufinfo = 0;
						lock_ctx();
						p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_buf_info;
						memcpy(p_user, p_bufinfo, sizeof(HD_VIDEOENC_BUFINFO));
						unlock_ctx();
					} else { // client process
						// query kernel to get physical addr/size of bs buffer
						HD_BUFINFO bufinfo = {0};

						_hd_get_param(self_id, out_id, VDOENC_PARAM_BUFINFO_PHYADDR, &bufinfo.phy_addr);
						_hd_get_param(self_id, out_id, VDOENC_PARAM_BUFINFO_SIZE,    &bufinfo.buf_size);
						memcpy(&p_user->buf_info, &bufinfo, sizeof(HD_BUFINFO));
					}

					hdal_flow_log_p("addr(0x%08x) size(%u)\n", (unsigned int)(p_user->buf_info.phy_addr), (unsigned int)(p_user->buf_info.buf_size));
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_ENC_PARAM:
				{
					HD_VIDEOENC_OUT *p_user = (HD_VIDEOENC_OUT*)p_param;
					HD_VIDEOENC_OUT2 enc_out_param2 = {0};

					hdal_flow_log_p("\n");

					hd_videoenc_get(path_id, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, (VOID *)&enc_out_param2);
					_hd_videoenc_assign_param_enc_out_param_2_to_1(p_user, &enc_out_param2);
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_ENC_PARAM2:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEOENC_OUT2 *p_user = (HD_VIDEOENC_OUT2*)p_param;
					HD_VIDEOENC_OUT2 *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_out_param;
					memcpy(p_user, p_bufinfo, sizeof(HD_VIDEOENC_OUT2));
					unlock_ctx();

					{
						hdal_flow_log_p("codec(%u) ", (unsigned int)(p_user->codec_type));
						if (p_user->codec_type == HD_CODEC_TYPE_JPEG) {
							hdal_flow_log_p("res_int(%u) img_q(%u) bitrate(%u) fr(%u/%u)\n", (unsigned int)(p_user->jpeg.retstart_interval), (unsigned int)(p_user->jpeg.image_quality), (unsigned int)(p_user->jpeg.bitrate), (unsigned int)(p_user->jpeg.frame_rate_base), (unsigned int)(p_user->jpeg.frame_rate_incr));
						} else {
							hdal_flow_log_p("gop(%u) ltr_int(%u) ltr_ref(%u) gray(%u) src_out(%u) profile(%u) level(%u) svc(%u) entropy(%u)\n", (unsigned int)(p_user->h26x.gop_num), (unsigned int)(p_user->h26x.ltr_interval), (unsigned int)(p_user->h26x.ltr_pre_ref), (unsigned int)(p_user->h26x.gray_en), (unsigned int)(p_user->h26x.source_output), (unsigned int)(p_user->h26x.profile), (unsigned int)(p_user->h26x.level_idc), (unsigned int)(p_user->h26x.svc_layer), (unsigned int)(p_user->h26x.entropy_mode));
						}
					}
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_VUI:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_VUI *p_user = (HD_H26XENC_VUI*)p_param;
					HD_H26XENC_VUI *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_vui;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_VUI));
					unlock_ctx();

					hdal_flow_log_p("vui_en(%u) sar_w(%u) sar_h(%u) mat_c(%u) tran_c(%u) col_prim(%u) vid_fmt(%u) col_rng(%u) time_pre(%u)\n", (unsigned int)(p_user->vui_en), (unsigned int)(p_user->sar_width), (unsigned int)(p_user->sar_height), (unsigned int)(p_user->matrix_coef), (unsigned int)(p_user->transfer_characteristics), (unsigned int)(p_user->colour_primaries), (unsigned int)(p_user->video_format), (unsigned int)(p_user->color_range), (unsigned int)(p_user->timing_present_flag));
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_DEBLOCK:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_DEBLOCK *p_user = (HD_H26XENC_DEBLOCK*)p_param;
					HD_H26XENC_DEBLOCK *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_deblock;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_DEBLOCK));
					unlock_ctx();

					hdal_flow_log_p("dis_ilf_idc(%u)  alpha(%d)  beta(%d)\n", (unsigned int)(p_user->dis_ilf_idc), (int)(p_user->db_alpha), (int)(p_user->db_beta));
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL:
				{
					HD_H26XENC_RATE_CONTROL *p_user = (HD_H26XENC_RATE_CONTROL*)p_param;
					HD_H26XENC_RATE_CONTROL2 rc_param2 = {0};

					hdal_flow_log_p("\n");

					hd_videoenc_get(path_id, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, (VOID *)&rc_param2);
					_hd_videoenc_assign_param_rc_param_2_to_1(p_user, &rc_param2);
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_RATE_CONTROL2 *p_user = (HD_H26XENC_RATE_CONTROL2*)p_param;
					HD_H26XENC_RATE_CONTROL2 *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_rc;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_RATE_CONTROL2));
					unlock_ctx();

					{
						switch (p_user->rc_mode)
						{
							case HD_RC_MODE_CBR:
								hdal_flow_log_p("mode(CBR) bitrate(%u) fr(%u/%u) I(int/min/max)=(%u/%u/%u) P(int/min/max)=(%u/%u/%u) sta(%u) ip_w(%d) kp_p(%u) kp_w(%d) p2_w(%d) p3_w(%d) lt_w(%d) mas(%d) max_size(%u)\n", (unsigned int)(p_user->cbr.bitrate), (unsigned int)(p_user->cbr.frame_rate_base), (unsigned int)(p_user->cbr.frame_rate_incr), (unsigned int)(p_user->cbr.init_i_qp), (unsigned int)(p_user->cbr.min_i_qp), (unsigned int)(p_user->cbr.max_i_qp), (unsigned int)(p_user->cbr.init_p_qp), (unsigned int)(p_user->cbr.min_p_qp), (unsigned int)(p_user->cbr.max_p_qp), (unsigned int)(p_user->cbr.static_time), (int)(p_user->cbr.ip_weight), (unsigned int)(p_user->cbr.key_p_period), (int)(p_user->cbr.kp_weight), (int)(p_user->cbr.p2_weight), (int)(p_user->cbr.p3_weight), (int)(p_user->cbr.lt_weight), (int)(p_user->cbr.motion_aq_str), (unsigned int)(p_user->cbr.max_frame_size));
							break;
							case HD_RC_MODE_VBR:
								hdal_flow_log_p("mode(VBR) bitrate(%u) fr(%u/%u) I(int/min/max)=(%u/%u/%u) P(int/min/max)=(%u/%u/%u) sta(%u) ip_w(%d) kp_p(%u) kp_w(%d) p2_w(%d) p3_w(%d) lt_w(%d) mas(%d) max_size(%u) pos(%u)\n", (unsigned int)(p_user->vbr.bitrate), (unsigned int)(p_user->vbr.frame_rate_base), (unsigned int)(p_user->vbr.frame_rate_incr), (unsigned int)(p_user->vbr.init_i_qp), (unsigned int)(p_user->vbr.min_i_qp), (unsigned int)(p_user->vbr.max_i_qp), (unsigned int)(p_user->vbr.init_p_qp), (unsigned int)(p_user->vbr.min_p_qp), (unsigned int)(p_user->vbr.max_p_qp), (unsigned int)(p_user->vbr.static_time), (int)(p_user->vbr.ip_weight), (unsigned int)(p_user->vbr.key_p_period), (int)(p_user->vbr.kp_weight), (int)(p_user->vbr.p2_weight), (int)(p_user->vbr.p3_weight), (int)(p_user->vbr.lt_weight), (int)(p_user->vbr.motion_aq_str), (unsigned int)(p_user->vbr.max_frame_size), (unsigned int)(p_user->vbr.change_pos));
							break;
							case HD_RC_MODE_EVBR:
								hdal_flow_log_p("mode(EVBR) bitrate(%u) fr(%u/%u) I(int/min/max)=(%u/%u/%u) P(int/min/max)=(%u/%u/%u) sta(%u) ip_w(%d) kp_p(%u) kp_w(%d) p2_w(%d) p3_w(%d) lt_w(%d) mas(%d) max_size(%u) sfc(%u) mrt(%u) si(%u) sp(%u) skp(%u)\n", p_user->evbr.bitrate, p_user->evbr.frame_rate_base, p_user->evbr.frame_rate_incr, p_user->evbr.init_i_qp, p_user->evbr.min_i_qp, p_user->evbr.max_i_qp, p_user->evbr.init_p_qp, p_user->evbr.min_p_qp, p_user->evbr.max_p_qp, p_user->evbr.static_time, p_user->evbr.ip_weight, p_user->evbr.key_p_period, p_user->evbr.kp_weight, p_user->evbr.p2_weight, p_user->evbr.p3_weight, p_user->evbr.lt_weight, p_user->evbr.motion_aq_str, p_user->evbr.max_frame_size, p_user->evbr.still_frame_cnd, p_user->evbr.motion_ratio_thd, p_user->evbr.still_i_qp, p_user->evbr.still_p_qp, p_user->evbr.still_kp_qp);
							break;
							case HD_RC_MODE_FIX_QP:
								hdal_flow_log_p("mode(FIXQP) fr(%u/%u) i_qp(%u) p_qp(%u)\n", (unsigned int)(p_user->fixqp.frame_rate_base), (unsigned int)(p_user->fixqp.frame_rate_incr), (unsigned int)(p_user->fixqp.fix_i_qp), (unsigned int)(p_user->fixqp.fix_p_qp));
							break;
							default:
								hdal_flow_log_p("ERROR\n");
								break;
						}
					}
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_USR_QP:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_USR_QP *p_user = (HD_H26XENC_USR_QP*)p_param;
					HD_H26XENC_USR_QP *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_usr_qp;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_USR_QP));
					unlock_ctx();

					hdal_flow_log_p("en(%u) map_addr(0x%08x)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->qp_map_addr));
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_SLICE_SPLIT *p_user = (HD_H26XENC_SLICE_SPLIT*)p_param;
					HD_H26XENC_SLICE_SPLIT *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_slice_split;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_SLICE_SPLIT));
					unlock_ctx();

					hdal_flow_log_p("en(%u) row_num(%u)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->slice_row_num));
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_ENC_GDR:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_GDR *p_user = (HD_H26XENC_GDR*)p_param;
					HD_H26XENC_GDR *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_gdr;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_GDR));
					unlock_ctx();

					hdal_flow_log_p("en(%u) period(%u) row_num(%u)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->period), (unsigned int)(p_user->number));
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_ROI:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_ROI *p_user = (HD_H26XENC_ROI*)p_param;
					HD_H26XENC_ROI *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_roi;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_ROI));
					unlock_ctx();

					{
						int i;
#if defined(_BSP_NA51000_)
						hdal_flow_log_p("qp_mode(%u)\n", (unsigned int)(p_user->roi_qp_mode));
						for (i=0; i<HD_H26XENC_ROI_WIN_COUNT; i++) {
							if (p_user->st_roi[i].enable == 1)
								hdal_flow_log_p("    [%2d] en(%u) qp(%d) rect(x,y,w,h)=(%u,%u,%u,%u)\n", (int)(i), (unsigned int)(p_user->st_roi[i].enable), (int)(p_user->st_roi[i].qp), (unsigned int)(p_user->st_roi[i].rect.x), (unsigned int)(p_user->st_roi[i].rect.y), (unsigned int)(p_user->st_roi[i].rect.w), (unsigned int)(p_user->st_roi[i].rect.h));
						}
#elif defined(_BSP_NA51055_)
						hdal_flow_log_p("\n");
						for (i=0; i<HD_H26XENC_ROI_WIN_COUNT; i++) {
							if (p_user->st_roi[i].enable == 1)
								hdal_flow_log_p("  [%2d] en(%u) qp(%d) mode(%d) rect(x,y,w,h)=(%u,%u,%u,%u)\n", (int)(i), (unsigned int)(p_user->st_roi[i].enable), (int)(p_user->st_roi[i].qp), (int)(p_user->st_roi[i].mode), (unsigned int)(p_user->st_roi[i].rect.x), (unsigned int)(p_user->st_roi[i].rect.y), (unsigned int)(p_user->st_roi[i].rect.w), (unsigned int)(p_user->st_roi[i].rect.h));
						}
#endif
					}
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_ROW_RC:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_ROW_RC *p_user = (HD_H26XENC_ROW_RC*)p_param;
					HD_H26XENC_ROW_RC *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_row_rc;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_ROW_RC));
					unlock_ctx();
#if defined(_BSP_NA51000_)
					hdal_flow_log_p("en(%u) qp_rng(%u) qp_step(%u)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->i_qp_range), (unsigned int)(p_user->i_qp_step));
#elif defined(_BSP_NA51055_)
					hdal_flow_log_p("en(%u) I(range, step, min, max) = (%u, %u, %u, %u) P(range, step, min, max) = (%u, %u, %u, %u)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->i_qp_range), (unsigned int)(p_user->i_qp_step), (unsigned int)(p_user->min_i_qp), (unsigned int)(p_user->max_i_qp), (unsigned int)(p_user->p_qp_range), (unsigned int)(p_user->p_qp_step), (unsigned int)(p_user->min_p_qp), (unsigned int)(p_user->max_p_qp));
#endif
				}
				break;
			case HD_VIDEOENC_PARAM_OUT_AQ:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_H26XENC_AQ *p_user = (HD_H26XENC_AQ*)p_param;
					HD_H26XENC_AQ *p_bufinfo = 0;
					lock_ctx();
					p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_aq;
					memcpy(p_user, p_bufinfo, sizeof(HD_H26XENC_AQ));
					unlock_ctx();

					hdal_flow_log_p("en(%u) i_str(%u) p_str(%u) max_delta(%d) min_delta(%d)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->i_str), (unsigned int)(p_user->p_str), (int)(p_user->max_delta_qp), (int)(p_user->min_delta_qp));
				}
				break;
			case HD_VIDEOENC_PARAM_BS_RING:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEOENC_BS_RING *p_user = (HD_VIDEOENC_BS_RING*)p_param;
					HD_VIDEOENC_BS_RING *p_bufinfo = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_bs_ring;
					memcpy(p_user, p_bufinfo, sizeof(HD_VIDEOENC_BS_RING));

					hdal_flow_log_p("en(%u)\n", (unsigned int)(p_user->enable));
				}
				break;
			default: rv = HD_ERR_PARAM; break;
			}
		}
		/*
		if(rv != HD_OK)
			return rv;
		cmd.param = id;
		if(p_param == NULL)
			return HD_ERR_NULL_PTR;
		cmd.value = (UINT32)p_param;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
		if (r == 0) {
			rv = HD_ERR_NG;
			switch(cmd.rv) {
			case ISF_OK: rv = HD_OK; break;
			default: break;
			}
		} else {
			rv = HD_ERR_SYS;
		}*/
	}
	return rv;
}

HD_RESULT hd_videoenc_set(HD_PATH_ID path_id, HD_VIDEOENC_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r=0;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};

	{
		CHAR  param_name[20];
		_hd_videoenc_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videoenc_set(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", (unsigned int)(path_id));
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_videoenc_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}
	if (p_param == 0) {
		return HD_ERR_NULL_PTR;
	}

	{
		HD_IO osg_in_id = in_id, osg_out_id = out_id;
		_HD_CONVERT_OSG_ID(osg_in_id);
		_HD_CONVERT_OSG_ID(osg_out_id);
		if(osg_in_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			hdal_flow_log_p("\n");
			return _hd_osg_set(self_id, out_id, osg_in_id, id, p_param);
		} else if(osg_out_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
			hdal_flow_log_p("\n");
			return _hd_osg_set(self_id, in_id, osg_out_id, id, p_param);
		}
	}

	// check input parameter valid
	{
		CHAR msg_string[256];
		UINT32 chk_self_id = self_id;
		UINT32 chk_out_id  = out_id;

		_HD_CONVERT_SELF_ID(chk_self_id, rv);   if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(chk_out_id, rv);     if(rv != HD_OK) {	return rv;}

		if (_hd_videoenc_check_params_range(chk_self_id, chk_out_id, id, p_param, msg_string, 256) < 0) {
			DBG_ERR("Wrong value. %s, id=%d\n", msg_string, (int)(id));
			return HD_ERR_PARAM;
		}
	}

	// check input parameter state
	{
		CHAR msg_string[256];
		UINT32 chk_self_id = self_id;
		UINT32 chk_out_id  = out_id;

		_HD_CONVERT_SELF_ID(chk_self_id, rv);   if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(chk_out_id, rv);     if(rv != HD_OK) {	return rv;}

		lock_ctx();
		if (_hd_videoenc_check_params_state(chk_self_id, chk_out_id, id, p_param, msg_string, 256) < 0) {
			DBG_ERR("Wrong state. %s, id=%d\n", msg_string, (int)(id));
			unlock_ctx();
			return HD_ERR_STATE;
		}
		unlock_ctx();
	}

	{
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		rv = HD_OK;
		cmd.param = id;

		if(ctrl_id == HD_CTRL) {
			switch(id) {
			/*
			case HD_VIDEOPROCESS_PARAM_CONFIG:
			{
				HD_VIDEOPROCESS_CONFIG* p_user = (HD_VIDEOPROCESS_CONFIG*)p_param;
				ISF_VDO_MAX max = {0};
				ISF_SET set[4] = {0};

				max.max_frame = 1;
				max.max_imgfmt = p_user->in_max.pxlfmt;
				max.max_imgsize.w = p_user->in_max.size.w;
				max.max_imgsize.h = p_user->in_max.size.h;
				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = ISF_UNIT_PARAM_VDOMAX;
				cmd.value = (UINT32)&max;
				cmd.size = sizeof(ISF_VDO_MAX);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				printf("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);
				if (r != 0) goto _HD_VP1;

				set[0].param = VDOPRC_PARAM_PIPE;	set[0].value = p_user->pipe;
				set[1].param = VDOPRC_PARAM_IQ_ID;	set[1].value = p_user->iq_id;
				set[2].param = VDOPRC_PARAM_MAX_FUNC;	set[2].value = p_user->ctrl_max.func;
				set[3].param = VDOPRC_PARAM_MAX_FUNC2;	set[3].value = p_user->in_max.func;
				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*4;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				printf("===>2:r=%d,rv=%08x\r\n", r, cmd.rv);
				goto _HD_VE1;

			}
			break;
			*/
			default: rv = HD_ERR_PARAM; break;
			}
		} else {
			switch(id) {
			case HD_VIDEOENC_PARAM_IN:
			{
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

				HD_VIDEOENC_IN* p_user = (HD_VIDEOENC_IN*)p_param;
				HD_VIDEOENC_IN *p_enc_in_dim = 0;
				lock_ctx();
				p_enc_in_dim = &videoenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_in_dim;
				memcpy(p_enc_in_dim, p_user, sizeof(HD_VIDEOENC_IN));
				unlock_ctx();

				hdal_flow_log_p("dim(%ux%u) pxl_fmt(0x%08x) dir(0x%08x) frc(%u/%u)\n", (unsigned int)(p_user->dim.w), (unsigned int)(p_user->dim.h), (unsigned int)(p_user->pxl_fmt), (unsigned int)(p_user->dir), (unsigned int)(GET_HI_UINT16(p_user->frc)), (unsigned int)(GET_LO_UINT16(p_user->frc)));

				ISF_VDO_FRAME frame = {0};
				ISF_VDO_FPS fps = {0};

				cmd.dest = ISF_PORT(self_id, in_id);

				frame.direct    = p_user->dir;	 // TODO: need hdal2unit func ?
				frame.imgfmt    = p_user->pxl_fmt;
				frame.imgsize.w = p_user->dim.w;
				frame.imgsize.h = p_user->dim.h;
				cmd.param = ISF_UNIT_PARAM_VDOFRAME;
				cmd.value = (UINT32)&frame;
				cmd.size = sizeof(ISF_VDO_FRAME);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VE1;

				fps.framepersecond = p_user->frc ;
				cmd.param = ISF_UNIT_PARAM_VDOFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_VDO_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VE1;
			}
			break;

			case HD_VIDEOENC_PARAM_IN_FRC:
			{
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

				HD_VIDEOENC_FRC* p_user = (HD_VIDEOENC_FRC*)p_param;
				HD_VIDEOENC_IN *p_enc_in_dim = 0;
				lock_ctx();
				p_enc_in_dim = &videoenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_in_dim;
				p_enc_in_dim->frc = p_user->frc;
				unlock_ctx();

				hdal_flow_log_p("frc(%u/%u)\n", (unsigned int)(GET_HI_UINT16(p_user->frc)), (unsigned int)(GET_LO_UINT16(p_user->frc)));

				ISF_VDO_FPS fps = {0};

				cmd.dest = ISF_PORT(self_id, in_id);

				fps.framepersecond = p_user->frc ;
				cmd.param = ISF_UNIT_PARAM_VDOFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_VDO_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VE1;
			}
			break;

			case HD_VIDEOENC_PARAM_PATH_CONFIG:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_VIDEOENC_PATH_CONFIG *p_user = (HD_VIDEOENC_PATH_CONFIG *)p_param;
				HD_VIDEOENC_PATH_CONFIG *p_enc_path_cfg = 0;
				VDOENC_INFO *p_enc_info = 0;
				lock_ctx();
				p_enc_path_cfg = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_path_cfg;
				p_enc_info = &videoenc_info[self_id - DEV_BASE];
				memcpy(p_enc_path_cfg, p_user, sizeof(HD_VIDEOENC_PATH_CONFIG));
				unlock_ctx();

				hdal_flow_log_p("isp_id(%u) codec(%u) dim(%ux%u) bitrate(%u) enc_buf_ms(%u) svc(%u) ltr(%u) rotate(%u) sout(%u)\n", (unsigned int)(p_user->isp_id), (unsigned int)(p_user->max_mem.codec_type), (unsigned int)(p_user->max_mem.max_dim.w), (unsigned int)(p_user->max_mem.max_dim.h), (unsigned int)(p_user->max_mem.bitrate), (unsigned int)(p_user->max_mem.enc_buf_ms), (unsigned int)(p_user->max_mem.svc_layer), (unsigned int)(p_user->max_mem.ltr), (unsigned int)(p_user->max_mem.rotate), (unsigned int)(p_user->max_mem.source_output));

				// [OUT]
				cmd.dest = ISF_PORT(self_id, out_id);

				// codec
				cmd.param = VDOENC_PARAM_CODEC;
				cmd.value = _hd_videoenc_codec_hdal2unit(p_user->max_mem.codec_type);
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VE1;

				// enc_buf_ms (set same as MAX_MEM)
				cmd.param = VDOENC_PARAM_ENCBUF_MS;
				cmd.value = p_user->max_mem.enc_buf_ms;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VE1;

				// isp_id
				cmd.param = VDOENC_PARAM_ISP_ID;
				cmd.value = p_user->isp_id;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VE1;

				lock_ctx();
				// target bitrate (set same as MAX_MEM first)
				videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_rc.cbr.bitrate = p_user->max_mem.bitrate;
				unlock_ctx();

				// max_mem
				NMI_VDOENC_MAX_MEM_INFO max_mem;

				max_mem.uiCodec           = _hd_videoenc_codec_hdal2unit(p_user->max_mem.codec_type);
				max_mem.uiWidth           = p_user->max_mem.max_dim.w;
				max_mem.uiHeight          = p_user->max_mem.max_dim.h;
				max_mem.uiTargetByterate  = p_user->max_mem.bitrate/8;
				max_mem.uiEncBufMs        = p_user->max_mem.enc_buf_ms;
				max_mem.uiRecFormat       = MEDIAREC_LIVEVIEW;
				max_mem.uiSVCLayer        = p_user->max_mem.svc_layer;
				max_mem.uiLTRInterval     = p_user->max_mem.ltr;
				max_mem.uiRotate          = p_user->max_mem.rotate;
				max_mem.bAllocSnapshotBuf = p_user->max_mem.source_output;
				max_mem.bRelease          = FALSE;
				lock_ctx();
				max_mem.bColmvEn          = p_enc_info->port[out_id-OUT_BASE].priv.vendor_set.h26x_colmv_en;
				max_mem.bCommRecFrmEn     = p_enc_info->port[out_id-OUT_BASE].priv.vendor_set.h26x_comm_recfrm_en;
				max_mem.bFitWorkMemory    = p_enc_info->port[out_id-OUT_BASE].priv.vendor_set.b_fit_work_memory;
				max_mem.bQualityBaseMode  = p_enc_info->port[out_id-OUT_BASE].priv.vendor_set.b_quality_base_en;
				max_mem.bCommSnapShot     = p_enc_info->port[out_id-OUT_BASE].priv.vendor_set.b_comm_srcout_en;
				unlock_ctx();

				cmd.param = VDOENC_PARAM_MAX_MEM_INFO;
				cmd.value = (UINT32)&max_mem;
				cmd.size = sizeof(NMI_VDOENC_MAX_MEM_INFO);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

				lock_ctx();
				if (r == 0) {
					videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = TRUE;
				} else {
					DBG_ERR("Set MAX memory FAIL ... !!\r\n");
				}
				unlock_ctx();
				goto _HD_VE1;
			}
			break;
			case HD_VIDEOENC_PARAM_FUNC_CONFIG:
			{
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_VIDEOENC_FUNC_CONFIG *p_user = (HD_VIDEOENC_FUNC_CONFIG *)p_param;
				if (p_user->in_func & HD_VIDEOENC_INFUNC_LOWLATENCY) {
					UINT32 exist_ll_path = 0;
					if (_hd_videoenc_check_exist_lowlatency_path(&exist_ll_path) < 0) {
						DBG_ERR("duplicate low-latency path %lu, cur=%lu\n", (unsigned long)((UINT32)(in_id-IN_BASE)), (unsigned long)(exist_ll_path));
						return HD_ERR_INV;
					}
				}
				HD_VIDEOENC_FUNC_CONFIG *p_enc_func_cfg = 0;
				lock_ctx();
				p_enc_func_cfg = &videoenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_func_cfg;
				memcpy(p_enc_func_cfg, p_user, sizeof(HD_VIDEOENC_FUNC_CONFIG));
				unlock_ctx();

				hdal_flow_log_p("ddr_id(%u) in_func(%u)\n", (unsigned int)(p_user->ddr_id), (unsigned int)(p_user->in_func));

				// [IN]
				cmd.dest = ISF_PORT(self_id, in_id);

				// buffer count
				{
					ISF_VDO_MAX max = {0};
#if defined(_BSP_NA51000_)
					max.max_frame = 2;
#elif defined(_BSP_NA51055_)
					max.max_frame = ((p_user->in_func & HD_VIDEOENC_INFUNC_ONEBUF) || (p_user->in_func & HD_VIDEOENC_INFUNC_LOWLATENCY))? 1:2;
#endif
					cmd.param = ISF_UNIT_PARAM_VDOMAX;
					cmd.value = (UINT32)&max;
					cmd.size = sizeof(ISF_VDO_MAX);
					r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
					if (r != 0) goto _HD_VE1;
				}

				// [OUT]
				cmd.dest = ISF_PORT(self_id, out_id);

				// ddr_id
				{
					ISF_ATTR t_attr = {0};

					t_attr.attr = p_user->ddr_id;

					cmd.param = ISF_UNIT_PARAM_ATTR;
					cmd.value = (UINT32)&t_attr;
					cmd.size = sizeof(ISF_ATTR);
					r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
					if (r != 0) goto _HD_VE1;
				}

				lock_ctx();
				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FUNC, TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_ENC_PARAM:
			{
				HD_VIDEOENC_OUT2 enc_out_param2 = {0};
				HD_VIDEOENC_OUT* p_user = (HD_VIDEOENC_OUT*)p_param;

				hdal_flow_log_p("\n");

				_hd_videoenc_assign_param_enc_out_param_1_to_2(&enc_out_param2, p_user);
				return hd_videoenc_set(path_id, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, (VOID *)&enc_out_param2);
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_ENC_PARAM2:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_VIDEOENC_OUT2* p_user = (HD_VIDEOENC_OUT2*)p_param;
				HD_VIDEOENC_OUT2 *p_enc_out_param = 0;
				lock_ctx();
				p_enc_out_param = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_out_param;
				memcpy(p_enc_out_param, p_user, sizeof(HD_VIDEOENC_OUT2));

				hdal_flow_log_p("codec(%u) ", (unsigned int)(p_user->codec_type));

				if (p_user->codec_type == HD_CODEC_TYPE_JPEG) {
					//---- JPEG ----
					hdal_flow_log_p("res_int(%u) img_q(%u) bitrate(%u) fr(%u/%u)\n", (unsigned int)(p_user->jpeg.retstart_interval), (unsigned int)(p_user->jpeg.image_quality), (unsigned int)(p_user->jpeg.bitrate), (unsigned int)(p_user->jpeg.frame_rate_base), (unsigned int)(p_user->jpeg.frame_rate_incr));

					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CODEC, TRUE);

					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_LTR, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GOPNUM, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ALLOC_SNAPSHOT_BUF, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_PROFILE, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_SVC, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_LEVEL_IDC, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ENTROPY, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GRAY, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CHRM_QP_IDX, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_SEC_CHRM_QP_IDX, FALSE);

					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_QUALITY, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_RESTART_INTERVAL, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_BITRATE, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_FRAMERATE, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_VBR_MODE, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_RC_MIN_QUALITY, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_RC_MAX_QUALITY, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_YUV_TRAN_ENABLE, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_JFIF_ENABLE, TRUE);
				} else {
					//---- H264 & H265 ----
					hdal_flow_log_p("gop(%u) ltr_int(%u) ltr_ref(%u) gray(%u) src_out(%u) profile(%u) level(%u) svc(%u) entropy(%u)\n", (unsigned int)(p_user->h26x.gop_num), (unsigned int)(p_user->h26x.ltr_interval), (unsigned int)(p_user->h26x.ltr_pre_ref), (unsigned int)(p_user->h26x.gray_en), (unsigned int)(p_user->h26x.source_output), (unsigned int)(p_user->h26x.profile), (unsigned int)(p_user->h26x.level_idc), (unsigned int)(p_user->h26x.svc_layer), (unsigned int)(p_user->h26x.entropy_mode));

					// set (rc_gop = encode_gop) first, user may update rc_gop via vendor command later
					videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.vendor_set.h26x_rc_gop = p_enc_out_param->h26x.gop_num;

					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CODEC, TRUE);

					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_LTR, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GOPNUM, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ALLOC_SNAPSHOT_BUF, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_PROFILE, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_SVC, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_LEVEL_IDC, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ENTROPY, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GRAY, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CHRM_QP_IDX, TRUE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_SEC_CHRM_QP_IDX, TRUE);

					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_QUALITY, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_RESTART_INTERVAL, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_BITRATE, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_FRAMERATE, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_VBR_MODE, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_RC_MIN_QUALITY, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_RC_MAX_QUALITY, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_YUV_TRAN_ENABLE, FALSE);
					_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_JPG_JFIF_ENABLE, FALSE);
					{
						// GOP can only be update via RC after init, so just call set RC again to set GOP
						HD_H26XENC_RATE_CONTROL2 *p_enc_rc = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_rc;

						switch (p_enc_rc->rc_mode)
						{
							case HD_RC_MODE_CBR:
							{
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   TRUE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
							}
							break;
							case HD_RC_MODE_VBR:
							{
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   TRUE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
							}
							break;
							case HD_RC_MODE_EVBR:
							{
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  TRUE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
							}
							break;
							case HD_RC_MODE_FIX_QP:
							{
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
								_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, TRUE);
							}
							break;
							default:
								DBG_ERR("ERROR: current rc_mode(%d) is not support\n", (int)(p_enc_rc->rc_mode));
								break;
						}
					}
				}
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_VUI:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_VUI* p_user = (HD_H26XENC_VUI*)p_param;
				HD_H26XENC_VUI *p_enc_vui = 0;
				lock_ctx();
				p_enc_vui = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_vui;
				memcpy(p_enc_vui, p_user, sizeof(HD_H26XENC_VUI));

				hdal_flow_log_p("vui_en(%u) sar_w(%u) sar_h(%u) mat_c(%u) tran_c(%u) col_prim(%u) vid_fmt(%u) col_rng(%u) time_pre(%u)\n", (unsigned int)(p_user->vui_en), (unsigned int)(p_user->sar_width), (unsigned int)(p_user->sar_height), (unsigned int)(p_user->matrix_coef), (unsigned int)(p_user->transfer_characteristics), (unsigned int)(p_user->colour_primaries), (unsigned int)(p_user->video_format), (unsigned int)(p_user->color_range), (unsigned int)(p_user->timing_present_flag));

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VUI, TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_DEBLOCK:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_DEBLOCK* p_user = (HD_H26XENC_DEBLOCK*)p_param;
				HD_H26XENC_DEBLOCK *p_enc_deblock = 0;
				lock_ctx();
				p_enc_deblock = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_deblock;
				memcpy(p_enc_deblock, p_user, sizeof(HD_H26XENC_DEBLOCK));

				hdal_flow_log_p("dis_ilf_idc(%u)  alpha(%d)  beta(%d)\n", (unsigned int)(p_user->dis_ilf_idc), (int)(p_user->db_alpha), (int)(p_user->db_beta));

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_DEBLOCK,  TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL:
			{
				HD_H26XENC_RATE_CONTROL2 rc_param2 = {0};
				HD_H26XENC_RATE_CONTROL* p_user = (HD_H26XENC_RATE_CONTROL*)p_param;

				hdal_flow_log_p("\n");

				_hd_videoenc_assign_param_rc_param_1_to_2(&rc_param2, p_user);
				return hd_videoenc_set(path_id, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, (VOID *)&rc_param2);
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_RATE_CONTROL2* p_user = (HD_H26XENC_RATE_CONTROL2*)p_param;
				HD_H26XENC_RATE_CONTROL2 *p_enc_rc = 0;
				lock_ctx();
				p_enc_rc = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_rc;
				memcpy(p_enc_rc, p_user, sizeof(HD_H26XENC_RATE_CONTROL2));

				switch (p_user->rc_mode)
				{
					case HD_RC_MODE_CBR:
					{
						hdal_flow_log_p("mode(CBR) bitrate(%u) fr(%u/%u) I(int/min/max)=(%u/%u/%u) P(int/min/max)=(%u/%u/%u) sta(%u) ip_w(%d) kp_p(%u) kp_w(%d) p2_w(%d) p3_w(%d) lt_w(%d) mas(%d) max_size(%u)\n", (unsigned int)(p_user->cbr.bitrate), (unsigned int)(p_user->cbr.frame_rate_base), (unsigned int)(p_user->cbr.frame_rate_incr), (unsigned int)(p_user->cbr.init_i_qp), (unsigned int)(p_user->cbr.min_i_qp), (unsigned int)(p_user->cbr.max_i_qp), (unsigned int)(p_user->cbr.init_p_qp), (unsigned int)(p_user->cbr.min_p_qp), (unsigned int)(p_user->cbr.max_p_qp), (unsigned int)(p_user->cbr.static_time), (int)(p_user->cbr.ip_weight), (unsigned int)(p_user->cbr.key_p_period), (int)(p_user->cbr.kp_weight), (int)(p_user->cbr.p2_weight), (int)(p_user->cbr.p3_weight), (int)(p_user->cbr.lt_weight), (int)(p_user->cbr.motion_aq_str), (unsigned int)(p_user->cbr.max_frame_size));

						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);

					}
					break;
					case HD_RC_MODE_VBR:
					{
						hdal_flow_log_p("mode(VBR) bitrate(%u) fr(%u/%u) I(int/min/max)=(%u/%u/%u) P(int/min/max)=(%u/%u/%u) sta(%u) ip_w(%d) kp_p(%u) kp_w(%d) p2_w(%d) p3_w(%d) lt_w(%d) mas(%d) max_size(%u) pos(%u)\n", (unsigned int)(p_user->vbr.bitrate), (unsigned int)(p_user->vbr.frame_rate_base), (unsigned int)(p_user->vbr.frame_rate_incr), (unsigned int)(p_user->vbr.init_i_qp), (unsigned int)(p_user->vbr.min_i_qp), (unsigned int)(p_user->vbr.max_i_qp), (unsigned int)(p_user->vbr.init_p_qp), (unsigned int)(p_user->vbr.min_p_qp), (unsigned int)(p_user->vbr.max_p_qp), (unsigned int)(p_user->vbr.static_time), (int)(p_user->vbr.ip_weight), (unsigned int)(p_user->vbr.key_p_period), (int)(p_user->vbr.kp_weight), (int)(p_user->vbr.p2_weight), (int)(p_user->vbr.p3_weight), (int)(p_user->vbr.lt_weight), (int)(p_user->vbr.motion_aq_str), (unsigned int)(p_user->vbr.max_frame_size), (unsigned int)(p_user->vbr.change_pos));

						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
					}
					break;
					case HD_RC_MODE_EVBR:
					{
						hdal_flow_log_p("mode(EVBR) bitrate(%u) fr(%u/%u) I(int/min/max)=(%u/%u/%u) P(int/min/max)=(%u/%u/%u) sta(%u) ip_w(%d) kp_p(%u) kp_w(%d) p2_w(%d) p3_w(%d) lt_w(%d) mas(%d) max_size(%u) sfc(%u) mrt(%u) si(%u) sp(%u) skp(%u)\n", p_user->evbr.bitrate, p_user->evbr.frame_rate_base, p_user->evbr.frame_rate_incr, p_user->evbr.init_i_qp, p_user->evbr.min_i_qp, p_user->evbr.max_i_qp, p_user->evbr.init_p_qp, p_user->evbr.min_p_qp, p_user->evbr.max_p_qp, p_user->evbr.static_time, p_user->evbr.ip_weight, p_user->evbr.key_p_period, p_user->evbr.kp_weight, p_user->evbr.p2_weight, p_user->evbr.p3_weight, p_user->evbr.lt_weight, p_user->evbr.motion_aq_str, p_user->evbr.max_frame_size, p_user->evbr.still_frame_cnd, p_user->evbr.motion_ratio_thd, p_user->evbr.still_i_qp, p_user->evbr.still_p_qp, p_user->evbr.still_kp_qp);

						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  TRUE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
					}
					break;
					case HD_RC_MODE_FIX_QP:
					{
						hdal_flow_log_p("mode(FIXQP) fr(%u/%u) i_qp(%u) p_qp(%u)\n", (unsigned int)(p_user->fixqp.frame_rate_base), (unsigned int)(p_user->fixqp.frame_rate_incr), (unsigned int)(p_user->fixqp.fix_i_qp), (unsigned int)(p_user->fixqp.fix_p_qp));

						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
						_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, TRUE);
					}
					break;
					default: rv = HD_ERR_PARAM; break;
				}
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_USR_QP:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_USR_QP* p_user = (HD_H26XENC_USR_QP*)p_param;
				HD_H26XENC_USR_QP *p_enc_usr_qp = 0;
				lock_ctx();
				p_enc_usr_qp = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_usr_qp;
				memcpy(p_enc_usr_qp, p_user, sizeof(HD_H26XENC_USR_QP));

				hdal_flow_log_p("en(%u) map_addr(0x%08x)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->qp_map_addr));

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_USR_QP_MAP, TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_SLICE_SPLIT* p_user = (HD_H26XENC_SLICE_SPLIT*)p_param;
				HD_H26XENC_SLICE_SPLIT *p_enc_slice_split = 0;
				lock_ctx();
				p_enc_slice_split = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_slice_split;
				memcpy(p_enc_slice_split, p_user, sizeof(HD_H26XENC_SLICE_SPLIT));

				hdal_flow_log_p("en(%u) row_num(%u)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->slice_row_num));

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_SLICE_SPLIT, TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_ENC_GDR:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_GDR* p_user = (HD_H26XENC_GDR*)p_param;
				HD_H26XENC_GDR *p_enc_gdr = 0;
				lock_ctx();
				p_enc_gdr = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_gdr;
				memcpy(p_enc_gdr, p_user, sizeof(HD_H26XENC_GDR));

				hdal_flow_log_p("en(%u) period(%u) row_num(%u)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->period), (unsigned int)(p_user->number));

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_GDR, TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_ROI:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_ROI* p_user = (HD_H26XENC_ROI*)p_param;
				HD_H26XENC_ROI *p_enc_roi = 0;
				lock_ctx();
				p_enc_roi = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_roi;
				memcpy(p_enc_roi, p_user, sizeof(HD_H26XENC_ROI));

				{
					int i;
#if defined(_BSP_NA51000_)
					hdal_flow_log_p("qp_mode(%u)\n", (unsigned int)(p_user->roi_qp_mode));
					for (i=0; i<HD_H26XENC_ROI_WIN_COUNT; i++) {
						if (p_user->st_roi[i].enable == 1)
							hdal_flow_log_p("    [%2d] en(%u) qp(%d) rect(x,y,w,h)=(%u,%u,%u,%u)\n", (int)(i), (unsigned int)(p_user->st_roi[i].enable), (int)(p_user->st_roi[i].qp), (unsigned int)(p_user->st_roi[i].rect.x), (unsigned int)(p_user->st_roi[i].rect.y), (unsigned int)(p_user->st_roi[i].rect.w), (unsigned int)(p_user->st_roi[i].rect.h));
					}
#elif defined(_BSP_NA51055_)
					hdal_flow_log_p("\n");
					for (i=0; i<HD_H26XENC_ROI_WIN_COUNT; i++) {
						if (p_user->st_roi[i].enable == 1)
							hdal_flow_log_p("  [%2d] en(%u) qp(%d) mode(%d) rect(x,y,w,h)=(%u,%u,%u,%u)\n", (int)(i), (unsigned int)(p_user->st_roi[i].enable), (int)(p_user->st_roi[i].qp), (int)(p_user->st_roi[i].mode), (unsigned int)(p_user->st_roi[i].rect.x), (unsigned int)(p_user->st_roi[i].rect.y), (unsigned int)(p_user->st_roi[i].rect.w), (unsigned int)(p_user->st_roi[i].rect.h));
					}
#endif
				}

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ROI, TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_ROW_RC:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_ROW_RC* p_user = (HD_H26XENC_ROW_RC*)p_param;
				HD_H26XENC_ROW_RC *p_enc_row_rc = 0;
				lock_ctx();
				p_enc_row_rc = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_row_rc;
				memcpy(p_enc_row_rc, p_user, sizeof(HD_H26XENC_ROW_RC));
				unlock_ctx();
#if defined(_BSP_NA51000_)
				hdal_flow_log_p("en(%u) qp_rng(%u) qp_step(%u)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->i_qp_range), (unsigned int)(p_user->i_qp_step));

				//TODO: (1) need to add VDOENC_PARAM_ROW_RC to set MP_VDOENC_SETINFO_ROWRC
				//      (2) or by setting via CBR/VBR/EVBR

				// use way (2), depends on current RC mode, set it again
				lock_ctx();
				{
					HD_H26XENC_RATE_CONTROL2 *p_enc_rc = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_rc;

					switch (p_enc_rc->rc_mode) {
						case HD_RC_MODE_CBR:
						{
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   TRUE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);

						}
						break;
						case HD_RC_MODE_VBR:
						{
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   TRUE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  FALSE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
						}
						break;
						case HD_RC_MODE_EVBR:
						{
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_CBRINFO,   FALSE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_VBRINFO,   FALSE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_EVBRINFO,  TRUE);
							_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_FIXQPINFO, FALSE);
						}
						break;

						case HD_RC_MODE_FIX_QP:
							DBG_ERR("FIX_QP mode could does NOT support ROW_RC mode !!\r\n");
							rv = HD_ERR_PARAM;
							break;

						default:
							DBG_ERR("unknown RC mode(%d)\r\n", (int)(p_enc_rc->rc_mode));
							rv = HD_ERR_PARAM;
							break;
					}
				}
				unlock_ctx();
#elif defined(_BSP_NA51055_)
				hdal_flow_log_p("en(%u) I(range, step, min, max) = (%u, %u, %u, %u) P(range, step, min, max) = (%u, %u, %u, %u)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->i_qp_range), (unsigned int)(p_user->i_qp_step), (unsigned int)(p_user->min_i_qp), (unsigned int)(p_user->max_i_qp), (unsigned int)(p_user->p_qp_range), (unsigned int)(p_user->p_qp_step), (unsigned int)(p_user->min_p_qp), (unsigned int)(p_user->max_p_qp));

				lock_ctx();
				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_ROWRC,  TRUE);
				unlock_ctx();
#endif
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_AQ:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_AQ* p_user = (HD_H26XENC_AQ*)p_param;
				HD_H26XENC_AQ *p_enc_aq = 0;
				lock_ctx();
				p_enc_aq = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_aq;
				memcpy(p_enc_aq, p_user, sizeof(HD_H26XENC_AQ));

				hdal_flow_log_p("en(%u) i_str(%u) p_str(%u) max_delta(%d) min_delta(%d)\n", (unsigned int)(p_user->enable), (unsigned int)(p_user->i_str), (unsigned int)(p_user->p_str), (int)(p_user->max_delta_qp), (int)(p_user->min_delta_qp));

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_AQINFO,  TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_REQUEST_IFRAME* p_user = (HD_H26XENC_REQUEST_IFRAME*)p_param;
				HD_H26XENC_REQUEST_IFRAME *p_req_keyframe = 0;
				lock_ctx();
				p_req_keyframe = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].request_keyframe;
				memcpy(p_req_keyframe, p_user, sizeof(HD_H26XENC_REQUEST_IFRAME));

				hdal_flow_log_p("en(%u)\n", (unsigned int)(p_user->enable));

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_RESET_IFRAME,  TRUE);
				unlock_ctx();
			}
			break;
			case HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XENC_TRIG_SNAPSHOT* p_user = (HD_H26XENC_TRIG_SNAPSHOT*)p_param;
				HD_H26XENC_TRIG_SNAPSHOT *p_trig_snapshot = 0;
				HD_VIDEOENC_STAT status;

				lock_ctx();
				p_trig_snapshot = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].trig_snapshot;
				memcpy(p_trig_snapshot, p_user, sizeof(HD_H26XENC_TRIG_SNAPSHOT));
				unlock_ctx();

				hdal_flow_log_p("addr(0x%08x) size(%u) img_q(%u)\n", (unsigned int)(p_user->phy_addr), (unsigned int)(p_user->size), (unsigned int)(p_user->image_quality));

				lock_ctx();
				status = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status;
				unlock_ctx();

				if (status != HD_VIDEOENC_STATUS_START) {
					DBG_ERR("TRIG_SNAPSHOT could only be called after START, please call hd_videoenc_start() first\r\n");
					return HD_ERR_NOT_START;
				}

				lock_ctx();
				{
					HD_VIDEO_CODEC codec = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_out_param.codec_type;
					BOOL maxmem_snapshot = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_path_cfg.max_mem.source_output;
					BOOL encout_snapshot = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_out_param.h26x.source_output;

					if ((codec != HD_CODEC_TYPE_H264) && (codec != HD_CODEC_TYPE_H265)) {
						DBG_ERR("TRIG_SNAPSHOT is only available for H264/H265 codec !!\r\n");
						unlock_ctx();
						return HD_ERR_NOT_SUPPORT;
					}

					if (maxmem_snapshot == FALSE) {
						DBG_ERR("TRIG_SNAPSHOP require to alloc extra snapshot buffer(HD_VIDEOENC_PARAM_PATH_CONFIG, for max_mem.source_output = TRUE), current setting = FALSE !!\r\n");
						unlock_ctx();
						return HD_ERR_NOMEM;
					}

					if (encout_snapshot == FALSE) {
						DBG_ERR("TRIG_SNAPSHOP require to use extra-alloc snapshot buffer(HD_VIDEOENC_PARAM_OUT_ENC_PARAM, for h26x.source_output = TRUE), current setting = FALSE !!\r\n");
						unlock_ctx();
						return HD_ERR_USER;
					}
				}

				{
					UINT32 width  = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_in_dim.dim.w;
					UINT32 height = videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_in_dim.dim.h;
					UINT32 min_size = (width*height*3/2)/10;  // YUV420, compression ratio = 1/10

					HD_H26XENC_TRIG_SNAPSHOT* p_user = (HD_H26XENC_TRIG_SNAPSHOT *)p_param;

					if (!videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.vendor_set.b_comm_srcout_no_jpg_enc) {
						if (p_user->phy_addr == 0) {
							DBG_ERR("phyaddr is NULL\r\n");
							unlock_ctx();
							return HD_ERR_NULL_PTR;
						}
						if (p_user->size < min_size) {
							DBG_ERR("size(%lu) is too small, need at least (%lu)\r\n", (unsigned long)(p_user->size), (unsigned long)(min_size));
							unlock_ctx();
							return HD_ERR_PARAM;
						}
					}
					unlock_ctx();

					//trigger snapshot (send bs buffer to kernel)
					VDOENC_SNAP_INFO snap_info;
					snap_info.phyaddr       = p_user->phy_addr;
					snap_info.size          = p_user->size;
					snap_info.image_quality = p_user->image_quality;
					snap_info.timeout       = -1;  // NOT implement yet

					cmd.dest = ISF_PORT(self_id, out_id);
					cmd.param = VDOENC_PARAM_SNAPSHOT_PURE_LINUX;
					cmd.value = (UINT32)&snap_info;
					cmd.size = sizeof(VDOENC_SNAP_INFO);
					r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
					if (r != 0) {
						p_user->size = 0;
						goto _HD_VE1;
					}

					//query bs size
					_hd_get_param(self_id, out_id, VDOENC_PARAM_SNAPSHOT_PURE_LINUX, &p_user->size);
				}
			}
			break;
			case HD_VIDEOENC_PARAM_BS_RING:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_VIDEOENC_BS_RING* p_user = (HD_VIDEOENC_BS_RING*)p_param;
				HD_VIDEOENC_BS_RING *p_enc_bs_ring = &videoenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_bs_ring;
				memcpy(p_enc_bs_ring, p_user, sizeof(HD_VIDEOENC_BS_RING));

				hdal_flow_log_p("en(%u)\n", (unsigned int)(p_user->enable));

				_hd_videoenc_kflow_set_need_update(self_id, out_id, VDOENC_PARAM_BS_RING, TRUE);
			}
			break;
			default: rv = HD_ERR_PARAM; break;
			}
		}
		if(rv != HD_OK)
			return rv;
		r = 0; //OK
	}
_HD_VE1:
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:  rv = HD_OK; break;
		default: rv = HD_ERR_SYS; break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

void _hd_videoenc_dewrap_frame(ISF_DATA *p_data, HD_VIDEO_FRAME* p_video_frame)
{
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_frame->blk; //isf-data-handle

	p_vdoframe->sign = p_video_frame->sign;
	if (p_video_frame->p_next) {
		NVTMPP_IOC_VB_PA_TO_VA_S msg;
		int                      ret;

		msg.pa = (UINT32)p_video_frame->p_next;
		ret = ISF_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_PA_TO_VA , &msg);
		if (0 == ret) {
			p_vdoframe->p_next = (VDO_METADATA *)msg.va;
		} else {
			p_vdoframe->p_next = NULL;
		}
		//DBG_IND("p_next pa= 0x%x, va = 0x%x\r\n", (unsigned int)msg.pa, (unsigned int)msg.va);
	} else {
		p_vdoframe->p_next = NULL;
	}
	//p_vdoframe->ddr_id = 0; //TBD
	p_vdoframe->pxlfmt = p_video_frame->pxlfmt;
	p_vdoframe->size.w = p_video_frame->dim.w;
	p_vdoframe->size.h = p_video_frame->dim.h;
	p_vdoframe->count = p_video_frame->count;
	p_vdoframe->timestamp = p_video_frame->timestamp;
	p_vdoframe->pw[VDO_PINDEX_Y] = p_video_frame->pw[HD_VIDEO_PINDEX_Y];
	p_vdoframe->pw[VDO_PINDEX_UV] = p_video_frame->pw[HD_VIDEO_PINDEX_UV];
	p_vdoframe->ph[VDO_PINDEX_Y] = p_video_frame->ph[HD_VIDEO_PINDEX_Y];
	p_vdoframe->ph[VDO_PINDEX_UV] = p_video_frame->ph[HD_VIDEO_PINDEX_UV];
	p_vdoframe->loff[VDO_PINDEX_Y] = p_video_frame->loff[HD_VIDEO_PINDEX_Y];
	p_vdoframe->loff[VDO_PINDEX_UV] = p_video_frame->loff[HD_VIDEO_PINDEX_UV];
	p_vdoframe->phyaddr[VDO_PINDEX_Y] = p_video_frame->phy_addr[HD_VIDEO_PINDEX_Y];
	p_vdoframe->phyaddr[VDO_PINDEX_UV] = p_video_frame->phy_addr[HD_VIDEO_PINDEX_UV];
	p_vdoframe->reserved[0] = p_video_frame->poc_info;
	p_vdoframe->reserved[1] = p_video_frame->reserved[0];
	p_vdoframe->reserved[2] = p_video_frame->reserved[1];
	p_vdoframe->reserved[3] = p_video_frame->reserved[2];
	p_vdoframe->reserved[4] = p_video_frame->reserved[3];
	p_vdoframe->reserved[5] = p_video_frame->reserved[4];
	p_vdoframe->reserved[6] = p_video_frame->reserved[5];
	p_vdoframe->reserved[7] = p_video_frame->reserved[6];
	//p_vdoframe->addr[VDO_PINDEX_Y] = 0;  // kflow_videoenc will identify ((addr == 0) && (phyaddr != 0) && (loff != 0)) to know that it's coming from HDAL user push_in
	//p_vdoframe->addr[VDO_PINDEX_UV] = 0;  //    and then call nvtmpp_sys_pa2va() in kernel to convert phyaddr to viraddr
}

HD_RESULT hd_videoenc_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame, HD_VIDEOENC_BS* p_usr_out_video_bitstream, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", (int)(dev_fd));
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_videoenc_is_init() == FALSE)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_videoenc_is_init() == FALSE) {
			return HD_ERR_UNINIT;
		}
	}

	if (p_video_frame == NULL) {
		DBG_ERR("p_video_frame is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	if (p_video_frame->phy_addr[0] == 0) {
        DBG_ERR("Check HD_VIDEO_FRAME phy_addr[0] is NULL.\n");
        return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_videoenc_dewrap_frame(&data, p_video_frame);

		cmd.dest = ISF_PORT(self_id, in_id);
		cmd.p_data = &data;
		cmd.async = wait_ms;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_PUSH_DATA, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}

		if (rv != HD_OK) {
			//DBG_ERR("push in fail, r=%d, rv=%d\r\n", (int)(r), (int)(rv));
		}
	}

	return rv;
}

HD_RESULT hd_videoenc_pull_out_buf(HD_PATH_ID path_id, HD_VIDEOENC_BS* p_video_bitstream, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_videoenc_is_init() == FALSE)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_videoenc_is_init() == FALSE) {
			return HD_ERR_UNINIT;
		}
	}

	if (p_video_bitstream == NULL) {
		DBG_ERR("Check HD_VIDEOENC_BS struct is NULL.\n");
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		{
			UINT32 bs_ring = 0;
			_hd_get_param(self_id, out_id, VDOENC_PARAM_BS_RING, (VOID *)&bs_ring);
			if (bs_ring) {
				DBG_ERR("Not support hd_videoenc_pull_out_buf if HD_VIDEOENC_PARAM_BS_RING on.\n");
				return HD_ERR_NOT_ALLOW;
			}
		}

		cmd.dest = ISF_PORT(self_id, out_id);
		cmd.p_data = &data;
		cmd.async = wait_ms;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_PULL_DATA, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}

		//--- use ISF_DATA ---
		if ((rv == HD_OK) || (rv == HD_ERR_INTERMEDIATE)) {
			_hd_videoenc_wrap_bitstream(p_video_bitstream, &data);
		} else {
			//DBG_ERR("pull out fail !!\r\n");
		}
	}
	return rv;
}

HD_RESULT hd_videoenc_release_out_buf(HD_PATH_ID path_id, HD_VIDEOENC_BS* p_video_bitstream)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_videoenc_is_init() == FALSE)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_videoenc_is_init() == FALSE) {
			return HD_ERR_UNINIT;
		}
	}

	if (p_video_bitstream == NULL) {
		DBG_ERR("Check HD_VIDEOENC_BS struct is NULL.\n");
		return HD_ERR_NULL_PTR;
	}

	if (p_video_bitstream->blk == 0) {
		DBG_ERR("blk is NULL !!\r\n");
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		{
			UINT32 bs_ring = 0;
			_hd_get_param(self_id, out_id, VDOENC_PARAM_BS_RING, (VOID *)&bs_ring);
			if (bs_ring) {
				DBG_ERR("Not support hd_videoenc_release_out_buf if HD_VIDEOENC_PARAM_BS_RING on.\n");
				return HD_ERR_NOT_ALLOW;
			}
		}

		_hd_videoenc_rebuild_bitstream(&data, p_video_bitstream);

		cmd.dest = ISF_PORT(self_id, out_id);
		cmd.p_data = &data;
		cmd.async = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_RELEASE_DATA, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}

	}
	return rv;
}

HD_RESULT hd_videoenc_pull_out_buf3(HD_PATH_ID path_id, HD_VIDEOENC_BS3* p_video_bitstream, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_videoenc_is_init() == FALSE)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_videoenc_is_init() == FALSE) {
			return HD_ERR_UNINIT;
		}
	}

	if (p_video_bitstream == NULL) {
		DBG_ERR("Check HD_VIDEOENC_BS3 struct is NULL.\n");
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		cmd.dest = ISF_PORT(self_id, out_id);
		cmd.p_data = &data;
		cmd.async = wait_ms;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_PULL_DATA, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}

		//--- use ISF_DATA ---
		if ((rv == HD_OK) || (rv == HD_ERR_INTERMEDIATE)) {
			_hd_videoenc_wrap_bitstream3(p_video_bitstream, &data);
			if (p_video_bitstream->video_pack_num < p_video_bitstream->pack_num) {
				DBG_ERR("user provide video pack number video_pack_num(%u) should >= pull out bs pack number pack_num(%u)\r\n", (unsigned int)p_video_bitstream->video_pack_num, (unsigned int)p_video_bitstream->pack_num);
				rv = HD_ERR_NOT_ALLOW;
			}
		} else {
			//DBG_ERR("pull out fail !!\r\n");
		}
	}
	return rv;
}

HD_RESULT hd_videoenc_release_out_buf3(HD_PATH_ID path_id, HD_VIDEOENC_BS3* p_video_bitstream)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_videoenc_is_init() == FALSE)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_videoenc_is_init() == FALSE) {
			return HD_ERR_UNINIT;
		}
	}

	if (p_video_bitstream == NULL) {
		DBG_ERR("Check HD_VIDEOENC_BS3 struct is NULL.\n");
		return HD_ERR_NULL_PTR;
	}

	if (p_video_bitstream->blk == 0) {
		DBG_ERR("blk is NULL !!\r\n");
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_videoenc_rebuild_bitstream3(&data, p_video_bitstream);

		cmd.dest = ISF_PORT(self_id, out_id);
		cmd.p_data = &data;
		cmd.async = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_RELEASE_DATA, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:  rv = HD_OK; break;
			default: rv = HD_ERR_SYS; break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}

	}
	return rv;
}

HD_RESULT hd_videoenc_poll_list(HD_VIDEOENC_POLL_LIST *p_poll, UINT32 num, INT32 wait_ms)
{
	HD_DAL self_id;
	HD_PATH_ID path_id;
	HD_IO out_id;
	HD_RESULT rv = HD_ERR_NG;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	VDOENC_POLL_LIST_BS_INFO poll_bs_info;
	UINT32 i;
	int r;

	///hdal_flow_log_p("hd_videoenc_poll_list:\n    ");

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_videoenc_is_init() == FALSE)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_videoenc_is_init() == FALSE) {
			return HD_ERR_UNINIT;
		}
	}

	if (num == 0) {
		DBG_ERR("num is 0\n");
		return HD_ERR_PARAM;
	}

	// set to kernel to wait for bs available
	memset(&poll_bs_info, 0, sizeof(VDOENC_POLL_LIST_BS_INFO));

	for (i=0; i< num; i++) {
		path_id = p_poll[i].path_id;
		self_id = HD_GET_DEV(path_id);
		out_id = HD_GET_OUT(path_id);

		///hdal_flow_log_p("path_id(0x%08x) ", (unsigned int)(path_id));

		if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { return HD_ERR_IO;}
		_HD_CONVERT_SELF_ID(self_id, rv);    if(rv != HD_OK) { return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);      if(rv != HD_OK) { return rv;}

		poll_bs_info.path_mask |= (1 << out_id);
	}

	///hdal_flow_log_p("\n");

	poll_bs_info.wait_ms = wait_ms;

	{
		cmd.dest = ISF_PORT(self_id, ISF_CTRL);
		cmd.param = VDOENC_PARAM_POLL_LIST_BS;
		cmd.value = (UINT32)&poll_bs_info;
		cmd.size = sizeof(VDOENC_POLL_LIST_BS_INFO);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		if (r != 0) goto _HD_VE_POLL;
	}

	// return event_mask to user
	{
		UINT32 event_mask = 0;
		_hd_get_param(self_id, ISF_CTRL, VDOENC_PARAM_POLL_LIST_BS, &event_mask);
		///hdal_flow_log_p("    event_mask(0x%08x)\n", (unsigned int)(event_mask));

		for (i=0; i< num; i++) {
			path_id = p_poll[i].path_id;
			out_id = HD_GET_OUT(path_id);

			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { return HD_ERR_IO;}
			_HD_CONVERT_OUT_ID(out_id, rv);      if(rv != HD_OK) { return rv;}

			p_poll[i].revent.event = (event_mask & (1<<out_id))? TRUE:FALSE;
		}
	}

_HD_VE_POLL:
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:  rv = HD_OK; break;
		default: rv = HD_ERR_SYS; break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", (int)(cmd.rv));
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}

	return rv;
}

HD_RESULT hd_videoenc_recv_list(HD_VIDEOENC_RECV_LIST *p_video_bs, UINT32 num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoenc_uninit(VOID)
{
	HD_RESULT rv = HD_OK;
	UINT32 cfg, cfg1, cfg2;
	UINT32 gdc_first = 0;
	HD_PATH_ID path_id = 0;
	UINT32 dev, port;

	hdal_flow_log_p("hd_videoenc_uninit\n");

	if (dev_fd <= 0) {
		return HD_ERR_INIT;
	}
	if (_hd_common_get_pid() > 0) { // client process
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 0; //cmd = uninit
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = 0;
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("uninit fail? %d\r\n", (int)(r));
			return r;
		}
		if (!_hd_common_is_new_flow()) {
			return HD_OK;
		}
	}

	rv = _hd_common_get_init(&cfg);
	if (rv != HD_OK) {
		return HD_ERR_NO_CONFIG;
	}
	rv = hd_common_get_sysconfig(&cfg1, &cfg2);
	if (rv != HD_OK) {
		return HD_ERR_NO_CONFIG;
	}

	if (_hd_common_get_pid() == 0) {

	g_cfg_cpu   = (cfg & HD_COMMON_CFG_CPU);
	g_cfg_debug = ((cfg & HD_COMMON_CFG_DEBUG) >> 12);
	g_cfg_venc  = ((cfg1 & HD_VIDEOENC_CFG) >> 8);
	g_cfg_vprc  = ((cfg1 & HD_VIDEOPROC_CFG) >> 16);
#if defined(_BSP_NA51055_)
	g_cfg_ai = ((cfg2 & VENDOR_AI_CFG) >> 16);
#endif

#if defined(_BSP_NA51055_)
	if (g_cfg_ai & (VENDOR_AI_CFG_ENABLE_CNN >> 16)) { //if enable ai CNN2
		UINT32 stripe_rule = g_cfg_vprc & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
		if (stripe_rule > 3) {
			stripe_rule = 0;
		}
		if ((stripe_rule == 0)||(stripe_rule == 3)) { //NOT disable GDC?
			//DBG_DUMP("already enable CNN => current vprc config stripe_rule = LV%lu, enforce to LV2!", (unsigned long)(stripe_rule+1));
			g_cfg_vprc |= (HD_VIDEOPROC_CFG_STRIP_LV2 >> 16); //disable GDC for AI
		} else {
			//DBG_DUMP("already enable CNN => current vprc config stripe_rule = LV%lu", (unsigned long)(stripe_rule+1));
		}
	}
	{
		UINT32 stripe_rule = g_cfg_vprc & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
		if (stripe_rule > 3) {
			stripe_rule = 0;
		}
		if ((stripe_rule == 1)||(stripe_rule == 2)) { //if vprc disable GDC
		    gdc_first = 0; //enable "low-latency first" stripe policy: tile cut on 2048
		    //DBG_DUMP("vprc config stripe_rule = LV%lu => venc config GDC off, tile cut if w>2048", (unsigned long)(stripe_rule+1));
		} else {
		    gdc_first = 1; //enable "GDC first" stripe policy: tile cut on 1280
		    //DBG_DUMP("vprc config stripe_rule = LV%lu => venc config GDC on, tile cut if w>1280", (unsigned long)(stripe_rule+1));
		}
	}
#endif

	}

	if (videoenc_info[0].port == NULL) {
		DBG_ERR("NOT init yet\n");
		return HD_ERR_INIT;
	}

	if (_hd_common_get_pid() == 0) {

	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			HD_VIDEOENC_STAT status;
			lock_ctx();
			status = videoenc_info[dev].port[port].priv.status;
			unlock_ctx();
			if (status == HD_VIDEOENC_STATUS_START) {
				path_id = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(dev), HD_IN(port), HD_OUT(port));
				DBG_WRN("ERROR: path(%lu) is NOT stop/close yet....auto-call hd_videoenc_stop() and hd_videoenc_close() before attemp to uninit !!!\r\n", (unsigned long)(port));
				if (HD_OK != hd_videoenc_stop(path_id)) {
					DBG_ERR("auto-call hd_videoenc_stop(path%lu) error !!!\r\n", (unsigned long)(port));
					rv = HD_ERR_NG;
				}
				if (HD_OK != hd_videoenc_close(path_id)) {
					DBG_ERR("auto-call hd_videoenc_close(path%lu) error !!!\r\n", (unsigned long)(port));
					rv = HD_ERR_NG;
				}
			} else if (status == HD_VIDEOENC_STATUS_OPEN) {
				path_id = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(dev), HD_IN(port), HD_OUT(port));
				DBG_WRN("ERROR: path(%lu) is NOT close yet....auto-call hd_videoenc_close() before attemp to uninit !!!\r\n", (unsigned long)(port));
				if (HD_OK != hd_videoenc_close(path_id)) {
					DBG_ERR("auto-call hd_videoenc_close(path%lu) error !!!\r\n", (unsigned long)(port));
					rv = HD_ERR_NG;
				}
			} else {
				lock_ctx();
				// set status
				videoenc_info[dev].port[port].priv.status = HD_VIDEOENC_STATUS_UNINIT; // [INIT]/[UNINIT] -> [UNINT]
				unlock_ctx();
			}
		}
	}

	}

	//uninit drv
	if (_hd_common_get_pid() == 0) {
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 0; //cmd = uninit
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = 0;
		isf_dev_cmd.p2 = (g_cfg_cpu << 28) | (g_cfg_debug << 24) | (gdc_first << 4) | (g_cfg_venc);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("uninit fail? %d\r\n", (int)(r));
			return r;
		}
	}

	//uninit lib
	{
		free_ctx(videoenc_info[0].port);
		videoenc_info[0].port = NULL;
	}

	return rv;
}

int _hd_videoenc_is_init(VOID)
{
	return (videoenc_info[0].port != NULL);
}
