/**
	@brief Source code of video decoder module.\n
	This file contains the functions which is related to video decoder in the chip.

	@file hd_videodec.c

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
#define HD_MODULE_NAME HD_VIDEODEC
#include "hd_int.h"
#include "hd_logger_p.h"
#include "kflow_videodec/isf_vdodec.h"
#include "kflow_videodec/media_def.h"
#if defined(__LINUX)
#include <semaphore.h>
#else
#include "kwrap/semaphore.h"
#endif

// INCLUDE_PROTECTED
#include "dal_vdodec.h"
#include "video_decode.h"
#include "nmediaplay_vdodec.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define HD_VIDEODEC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE        ISF_UNIT_VDODEC
#define DEV_COUNT       1 //ISF_MAX_VDODEC
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        16
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       16

#define HD_DEV_BASE	HD_DAL_VIDEODEC_BASE
#define HD_DEV_MAX	HD_DAL_VIDEODEC_MAX

#define KFLOW_VIDEODEC_PARAM_BEGIN     VDODEC_PARAM_START
#define KFLOW_VIDEODEC_PARAM_END       VDODEC_PARAM_MAX
#define KFLOW_VIDEODEC_PARAM_NUM      (KFLOW_VIDEODEC_PARAM_END - KFLOW_VIDEODEC_PARAM_BEGIN)

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef enum _HD_VIDEODEC_STAT {
	HD_VIDEODEC_STATUS_UNINIT = 0,
	HD_VIDEODEC_STATUS_INIT,
	HD_VIDEODEC_STATUS_OPEN,
	HD_VIDEODEC_STATUS_START,
	HD_VIDEODEC_STATUS_STOP,
	HD_VIDEODEC_STATUS_CLOSE,
} HD_VIDEODEC_STAT;

typedef struct _VDODEC_INFO_PRIV {
	HD_VIDEODEC_STAT        status;                     ///< hd_videodec_status
	BOOL                    b_maxmem_set;
	BOOL                    b_need_update_param[KFLOW_VIDEODEC_PARAM_NUM];
} VDODEC_INFO_PRIV;

typedef struct _VDODEC_INFO_PORT {
	HD_VIDEODEC_BUFINFO       dec_buf_info;
	HD_VIDEODEC_PATH_CONFIG   dec_path_cfg;
	HD_VIDEODEC_IN            dec_in_param;
	HD_H26XDEC_DESC           dec_h26x_desc;

	//private data
	VDODEC_INFO_PRIV          priv;
} VDODEC_INFO_PORT;
	
typedef struct _VDODEC_INFO {
		VDODEC_INFO_PORT *port;
} VDODEC_INFO;

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                    */
/*-----------------------------------------------------------------------------*/
static UINT32 _max_path_count = 0;

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
			if((id < OUT_COUNT) && (id < _max_path_count)) { \
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
		} else if(((dev_id) >= ISF_UNIT_VDODEC) && ((dev_id) < ISF_UNIT_VDODEC + ISF_MAX_VDODEC)) { \
			(dev_id) = HD_DAL_VIDEODEC_BASE + (dev_id) - ISF_UNIT_VDODEC; \
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

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/
int _hd_videodec_is_init(VOID);

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
VDODEC_INFO videodec_info[DEV_COUNT] = {0};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

void _hd_videodec_cfg_max(UINT32 maxpath)
{
	if (!videodec_info[0].port) {
		_max_path_count = maxpath;
		if (_max_path_count > OUT_COUNT) {
			DBG_WRN("dts max_path=%lu is larger than built-in max_path=%lu!\r\n", _max_path_count, (UINT32)OUT_COUNT);
			_max_path_count = OUT_COUNT;
		}
		if (_max_path_count > VDODEC_MAX_PATH_NUM) {
			DBG_WRN("dts max_path=%lu is larger than built-in max_path=%lu!\r\n", _max_path_count, (UINT32)VDODEC_MAX_PATH_NUM);
			_max_path_count = VDODEC_MAX_PATH_NUM;
		}
	}
}

static UINT8 g_cfg_cpu = 0;
static UINT8 g_cfg_debug = 0;
static UINT8 g_cfg_vdec = 0;
#if defined(_BSP_NA51055_)
static UINT8 g_cfg_ai = 0;
#endif

UINT32 _hd_videodec_codec_hdal2unit(HD_VIDEO_CODEC h_codec)
{
	switch (h_codec)
	{
		case HD_CODEC_TYPE_JPEG:    return MEDIAPLAY_VIDEO_MJPG;
		case HD_CODEC_TYPE_H264:    return MEDIAPLAY_VIDEO_H264;
		case HD_CODEC_TYPE_H265:    return MEDIAPLAY_VIDEO_H265;
		default:
			DBG_ERR("unknown codec type(%d)\r\n", h_codec);
			return (-1);
	}
}

UINT32 _hd_videodec_codec_unit2hdal(UINT32 u_codec)
{
	switch (u_codec)
	{
		case MEDIAPLAY_VIDEO_MJPG:    return HD_CODEC_TYPE_JPEG;
		case MEDIAPLAY_VIDEO_H264:    return HD_CODEC_TYPE_H264;
		case MEDIAPLAY_VIDEO_H265:    return HD_CODEC_TYPE_H265;
		default:
			DBG_ERR("unknown codec type(%lu)\r\n", u_codec);
			return (-1);
	}
}

UINT32 _hd_videodec_frametype_hdal2unit(UINT32 h_ftype)
{
	switch (h_ftype)
	{
		case HD_FRAME_TYPE_IDR:    return MP_VDODEC_IDR_SLICE;
		case HD_FRAME_TYPE_I:      return MP_VDODEC_I_SLICE;
		case HD_FRAME_TYPE_P:      return MP_VDODEC_P_SLICE;
		case HD_FRAME_TYPE_KP:     return MP_VDODEC_KP_SLICE;
		default:
			DBG_ERR("unknown frame type(%lu)\r\n", h_ftype);
			return (-1);
	}
}

UINT32 _hd_videodec_frametype_unit2hdal(UINT32 u_ftype)
{
	switch (u_ftype)
	{
		case MP_VDODEC_IDR_SLICE:    return HD_FRAME_TYPE_IDR;
		case MP_VDODEC_I_SLICE:      return HD_FRAME_TYPE_I;
		case MP_VDODEC_P_SLICE:      return HD_FRAME_TYPE_P;
		case MP_VDODEC_KP_SLICE:     return HD_FRAME_TYPE_KP;
		default:
			DBG_ERR("unknown frame type(%lu)\r\n", u_ftype);
			return (-1);
	}
}

HD_RESULT _hd_videodec_convert_dev_id(HD_DAL* p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

HD_RESULT _hd_videodec_convert_in_id(HD_IO* p_in_id)
{
	HD_RESULT rv;
	_HD_CONVERT_IN_ID(p_in_id[0], rv);
	return rv;
}

int _hd_videodec_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "VIDEODEC_%d_IN_%d", dev_id - HD_DEV_BASE, in_id - HD_IN_BASE);
}

int _hd_videodec_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "VIDEODEC_%d_OUT_%d", dev_id - HD_DEV_BASE, out_id - HD_OUT_BASE);
}

HD_RESULT _hd_vd_get_param(HD_DAL self_id, HD_IO out_id, UINT32 param, UINT32 *value)
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
		default:
			rv = HD_ERR_SYS;
			break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

HD_RESULT hd_vd_get_vui_info(HD_PATH_ID path_id, UINT32 *value)
{
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	_HD_CONVERT_SELF_ID(self_id, rv); if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_OUT_ID(out_id, rv); if(rv != HD_OK) {	return rv;}

	cmd.dest = ISF_PORT(self_id, out_id);
	cmd.param = VDODEC_PARAM_VUI_INFO;
	cmd.value = (UINT32)value;//(UINT32)&vui_info;
	cmd.size = sizeof(MP_VDODEC_VUI_INFO);

	r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:
			rv = HD_OK;
			break;
		default:
			rv = HD_ERR_SYS;
			break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

HD_RESULT hd_vd_get_jpeginfo(HD_PATH_ID path_id, UINT32 *value)
{
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	_HD_CONVERT_SELF_ID(self_id, rv); if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_OUT_ID(out_id, rv); if(rv != HD_OK) {	return rv;}

	cmd.dest = ISF_PORT(self_id, out_id);
	cmd.param = VDODEC_PARAM_JPEGINFO;
	cmd.value = (UINT32)value;
	cmd.size = sizeof(MP_VDODEC_JPGDECINFO);

	r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:
			rv = HD_OK;
			//memcpy((MP_VDODEC_JPGDECINFO *)value, (MP_VDODEC_JPGDECINFO *)(&cmd.value), sizeof(MP_VDODEC_JPGDECINFO));
			break;
		default:
			rv = HD_ERR_SYS;
			break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

void _hd_videodec_wrap_frame(HD_VIDEO_FRAME* p_video_frame, ISF_DATA *p_data)
{
	PVDO_FRAME p_vdoframe = (VDO_FRAME *) p_data->desc;
	//HD_DAL id = (p_data->h_data >> 16) + HD_DEV_BASE;

	p_video_frame->blk = p_data->h_data; //isf-data-handle
	//p_video_frame->blk = (id << 16) | (p_data->h_data & 0xFFFF); // refer to jira: NA51000-1384

	p_video_frame->sign = p_vdoframe->sign;
	p_video_frame->p_next = NULL;
	p_video_frame->ddr_id = 0;
	p_video_frame->pxlfmt = p_vdoframe->pxlfmt;
	p_video_frame->dim.w = p_vdoframe->size.w;
	p_video_frame->dim.h = p_vdoframe->size.h;
	p_video_frame->count = p_vdoframe->count;
	p_video_frame->timestamp = p_vdoframe->timestamp;
	p_video_frame->pw[HD_VIDEO_PINDEX_Y] = p_vdoframe->pw[VDO_PINDEX_Y];
	p_video_frame->pw[HD_VIDEO_PINDEX_UV] = p_vdoframe->pw[VDO_PINDEX_UV];
	p_video_frame->ph[HD_VIDEO_PINDEX_Y] = p_vdoframe->ph[VDO_PINDEX_Y];
	p_video_frame->ph[HD_VIDEO_PINDEX_UV] = p_vdoframe->ph[VDO_PINDEX_UV];
	p_video_frame->loff[HD_VIDEO_PINDEX_Y] = p_vdoframe->loff[VDO_PINDEX_Y];
	p_video_frame->loff[HD_VIDEO_PINDEX_UV] = p_vdoframe->loff[VDO_PINDEX_UV];
	p_video_frame->phy_addr[HD_VIDEO_PINDEX_Y] = p_vdoframe->phyaddr[VDO_PINDEX_Y];
	p_video_frame->phy_addr[HD_VIDEO_PINDEX_UV] = p_vdoframe->phyaddr[VDO_PINDEX_UV];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

HD_RESULT hd_videodec_init(VOID)
{
	UINT32 cfg, cfg1, cfg2;
	HD_RESULT rv;
	UINT32 dev, port;
	UINT32 i;

	hdal_flow_log_p("hd_videodec_init\n");

	if (dev_fd <= 0) {
		return HD_ERR_INIT;
	}

	if (_hd_common_get_pid() > 0) {
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = 0;
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("init fail? %d\r\n", r);
			return r;
		}
		return HD_OK;
	}

	rv = _hd_common_get_init(&cfg);
	if (rv != HD_OK) {
		return HD_ERR_NO_CONFIG;
	}
	rv = hd_common_get_sysconfig(&cfg1, &cfg2);
	if (rv != HD_OK) {
		return HD_ERR_NO_CONFIG;
	}

	g_cfg_cpu = (cfg & HD_COMMON_CFG_CPU);
	g_cfg_debug = ((cfg & HD_COMMON_CFG_DEBUG) >> 12);
	g_cfg_vdec = ((cfg1 & HD_VIDEODEC_CFG) >> 12);
#if defined(_BSP_NA51055_)
	g_cfg_ai = ((cfg2 & VENDOR_AI_CFG) >> 16);
#endif

	if (videodec_info[0].port != NULL) {
		DBG_ERR("already init\n");
		return HD_ERR_UNINIT;
	}

	if (_max_path_count == 0) {
		DBG_ERR("_max_path_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	videodec_info[0].port = (VDODEC_INFO_PORT*)malloc(sizeof(VDODEC_INFO_PORT)*_max_path_count);
	if (videodec_info[0].port == NULL) {
		DBG_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
		return HD_ERR_HEAP;
	}
	for(i = 0; i < _max_path_count; i++) {
		memset(&(videodec_info[0].port[i]), 0, sizeof(VDODEC_INFO_PORT));  // no matter what status now...reset status = [UNINIT] anyway
	}

	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			// set status
			videodec_info[dev].port[port].priv.status = HD_VIDEODEC_STATUS_INIT;  // [UNINT] -> [INIT]
		}
	}

	//init drv
	{
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = _max_path_count;
#if defined(_BSP_NA51055_)
		isf_dev_cmd.p2 = (g_cfg_cpu << 28) | (g_cfg_debug << 24) | (g_cfg_ai << 8) | (g_cfg_vdec);
#else
		isf_dev_cmd.p2 = (g_cfg_cpu << 28) | (g_cfg_debug << 24) | (g_cfg_vdec);
#endif
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("init fail? %d\r\n", r);
			return r;
		}
	}
#if 0 // [ToDo]
	for (dev = 0; dev < 1 /*DEV_COUNT*/; dev++) {
		for (port = 0; port < OUT_COUNT; port++) {
			p_dec_info = &videodec_info[dev];
			self_id = (dev +DEV_BASE);
			out_id  = (port+OUT_BASE);

			//init values
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_PATH_CONFIG, &p_enc_info->port[port].enc_path_cfg);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_IN, &p_enc_info->port[port].enc_in_dim);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &p_enc_info->port[port].enc_out_param);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_VUI, &p_enc_info->port[port].enc_vui);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_DEBLOCK, &p_enc_info->port[port].enc_deblock);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &p_enc_info->port[port].enc_rc);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_USR_QP, &p_enc_info->port[port].enc_usr_qp);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &p_enc_info->port[port].enc_slice_split);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_ENC_GDR, &p_enc_info->port[port].enc_gdr);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_ROI, &p_enc_info->port[port].enc_roi);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_ROW_RC, &p_enc_info->port[port].enc_row_rc);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_AQ, &p_enc_info->port[port].enc_aq);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &p_enc_info->port[port].request_keyframe);
			_hd_videoenc_fill_default_params(HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT, &p_enc_info->port[port].trig_snapshot);

			// set init "need update"
			_hd_videoenc_kflow_init_need_update(self_id, out_id);
		}
	}
#endif
	return HD_OK;
}

HD_RESULT hd_videodec_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videodec_bind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) dst(0x%x) in(%d)\n", self_id, out_id, dest_id, in_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (_hd_videodec_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_BIND_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
		cmd.src = ISF_PORT(self_id, out_id);
		_HD_CONVERT_DEST_ID(dest_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
		cmd.dest = ISF_PORT(dest_id, in_id);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_BIND, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				rv = HD_OK;
				break;
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_videodec_unbind(HD_OUT_ID _out_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videodec_unbind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d)\n", self_id, out_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (_hd_videodec_is_init() == FALSE) {
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
			case ISF_OK:
				rv = HD_OK;
				break;
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT _hd_videodec_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
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
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT _hd_videodec_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id)
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
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT _hd_videodec_get_state(HD_OUT_ID _out_id, UINT32* p_state)
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
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_videodec_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videodec_open:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) in(%d) ", self_id, out_id, in_id);

	if (dev_fd <= 0) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (_hd_videodec_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		_HD_CONVERT_SELF_ID(self_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)DEV_COUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		if(!p_path_id) {return HD_ERR_NULL_PTR;}
		p_path_id[0] = _out_id;
		hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);
		//do nothing
		return HD_OK;
	} else {
		UINT32 cur_dev = self_id;
		HD_IO check_in_id = in_id;
		HD_IO check_out_id = out_id;
		_HD_CONVERT_SELF_ID(self_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)DEV_COUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_OUT_ID(check_out_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("out_id=%lu is larger than max=%lu!\r\n", (UINT32)((check_out_id) - HD_OUT_BASE), (UINT32)_max_path_count);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_SELF_ID(in_dev_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((in_dev_id) - HD_DEV_BASE), (UINT32)DEV_COUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_IN_ID(check_in_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("in_id=%lu is larger than max=%lu!\r\n", (UINT32)((check_in_id) - HD_IN_BASE), (UINT32)_max_path_count);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		#if 0  // CID 137131 (#1 of 1): Logically dead code (DEADCODE) => vdec only 1 device, so if pass up 2 line MUST (in_dev_id = self_id)
		if(in_dev_id != self_id) { hdal_flow_log_p("path_id(N/A)\n");	return HD_ERR_DEV;}
		#endif
		if(!p_path_id) { hdal_flow_log_p("path_id(N/A)\n"); return HD_ERR_NULL_PTR;}
		p_path_id[0] = HD_VIDEODEC_PATH(cur_dev, in_id, out_id);
	}

	hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_OUT_ID(out_id, rv);
		if(rv != HD_OK) {
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}

		// init variable
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				rv = HD_OK;
				break;
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}

	if (rv == HD_OK) {
		videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_VIDEODEC_STATUS_OPEN;
	}

	return rv;
}

HD_RESULT hd_videodec_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videodec_start:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (_hd_videodec_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		//--- check if MAX_MEM info had been set ---
		if (videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set == FALSE) {
			DBG_ERR("Error: please set HD_VIDEODEC_PARAM_MAXMEM_CONFIG first !!!\r\n");
			return HD_ERR_NO_CONFIG;
		}

		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 1;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				rv = HD_OK;
				break;
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}

	if (rv == HD_OK) {
		videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_VIDEODEC_STATUS_START;
	}

	{
		#if 0 // vdodec use common buffer as yuv buffer
		// get physical addr/size of decoded frame buffer
		HD_BUFINFO *p_bufinfo = &videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_buf_info.buf_info;
		_hd_vd_get_param(self_id, out_id, VDODEC_PARAM_BUFINFO_PHYADDR, &p_bufinfo->phy_addr);
		_hd_vd_get_param(self_id, out_id, VDODEC_PARAM_BUFINFO_SIZE,    &p_bufinfo->buf_size);
		DBG_DUMP("[start] decoded frame buffer: phy_addr=0x%lx, buf_size=0x%lx\r\n", p_bufinfo->phy_addr, p_bufinfo->buf_size);
		#endif
	}

	return rv;
}

HD_RESULT hd_videodec_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videodec_stop:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (_hd_videodec_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 2;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				rv = HD_OK;
				break;
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}

	if (rv == HD_OK) {
		videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_VIDEODEC_STATUS_STOP;
	}

	return rv;
}

HD_RESULT hd_videodec_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videodec_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videodec_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videodec_close:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (_hd_videodec_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	if(ctrl_id == HD_CTRL) {
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		//do nothing
		return HD_OK;
	}

	if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}

	_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_OUT_ID(out_id, rv);		if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_IN_ID(in_id, rv);		if(rv != HD_OK) {	return rv;}

	{
		// release MAX_MEM
		{
			ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
			NMI_VDODEC_MAX_MEM_INFO max_mem = {0};

			max_mem.uiVdoCodec = 0;
			max_mem.uiWidth = 0;
			max_mem.uiHeight = 0;
			max_mem.bRelease = TRUE;

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = VDODEC_PARAM_MAX_MEM_INFO;
			cmd.value = (UINT32)&max_mem;
			cmd.size = sizeof(NMI_VDODEC_MAX_MEM_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r == 0) {
				videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = FALSE;
			} else {
				DBG_ERR("Release memory FAIL ... !!\r\n");
			}
		}

		// call updateport - stop
		{
			ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};

			cmd.src = ISF_PORT(self_id, out_id);
			cmd.state = 3;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
			if (r == 0) {
				switch(cmd.rv) {
				case ISF_OK:
					rv = HD_OK;
					break;
				default:
					rv = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
		}

		// reset HD_VIDEODEC_PARAM_IN
		{
			ISF_SET set[1] = {0};
			HD_VIDEODEC_IN* p_dec_in_param = &videodec_info[self_id-DEV_BASE].port[in_id-IN_BASE].dec_in_param;

			p_dec_in_param->codec_type = 0;
			hdal_flow_log_p("codec(%u)\n", p_dec_in_param->codec_type);

			cmd.dest = ISF_PORT(self_id, in_id);

			set[0].param = VDODEC_PARAM_CODEC;           set[0].value = p_dec_in_param->codec_type;

			cmd.param = ISF_UNIT_PARAM_MULTI;
			cmd.value = (UINT32)set;
			cmd.size = sizeof(ISF_SET)*1;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r != 0) {
				DBG_ERR("reset HD_VIDEODEC_PARAM_IN fail\r\n");
			}
		}

		// reset HD_VIDEODEC_PARAM_IN_DESC
		{
			HD_H26XDEC_DESC* p_in_desc = &videodec_info[self_id-DEV_BASE].port[in_id-IN_BASE].dec_h26x_desc;
			NMI_VDODEC_MEM_RANGE desc_mem = {0};

			p_in_desc->addr = 0;
			p_in_desc->len = 0;
			hdal_flow_log_p("desc_addr(0x%08x) desc_len(%u)\n", p_in_desc->addr, p_in_desc->len);

			// desc
			desc_mem.Addr = p_in_desc->addr;
			desc_mem.Size = p_in_desc->len;

			cmd.dest = ISF_PORT(self_id, in_id);

			cmd.param = VDODEC_PARAM_DESC;
			cmd.value = (UINT32)&desc_mem;
			cmd.size = sizeof(NMI_VDODEC_MEM_RANGE);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r != 0) {
				DBG_ERR("reset HD_VIDEODEC_PARAM_IN_DESC fail\r\n");
			}
		}
	}

	if (rv == HD_OK) {
		videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_VIDEODEC_STATUS_CLOSE;
	}

	return rv;
}

INT _hd_videodec_param_cvt_name(HD_VIDEODEC_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
		case HD_VIDEODEC_PARAM_DEVCOUNT:            snprintf(p_ret_string, max_str_len, "DEVCOUNT");           break;
		case HD_VIDEODEC_PARAM_SYSCAPS:             snprintf(p_ret_string, max_str_len, "SYSCAPS");            break;
		case HD_VIDEODEC_PARAM_PATH_CONFIG:         snprintf(p_ret_string, max_str_len, "PATH_CONFIG");        break;
		case HD_VIDEODEC_PARAM_BUFINFO:             snprintf(p_ret_string, max_str_len, "BUFINFO");            break;
		case HD_VIDEODEC_PARAM_IN:                  snprintf(p_ret_string, max_str_len, "IN");                 break;
		case HD_VIDEODEC_PARAM_IN_DESC:             snprintf(p_ret_string, max_str_len, "IN_DESC");            break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", id);
			return (-1);
	}
	return 0;
}

HD_RESULT hd_videodec_get(HD_PATH_ID path_id, HD_VIDEODEC_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	//HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	//int r;

	{
		CHAR  param_name[20];
		_hd_videodec_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videodec_get(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (_hd_videodec_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	if (p_param == 0) {
		return HD_ERR_NULL_PTR;
	}

	{
		//ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		rv = HD_OK;
		if(ctrl_id == HD_CTRL) {
			switch(id) {

			case HD_VIDEODEC_PARAM_DEVCOUNT:
			{
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
				p_user->max_dev_count = 1;

				hdal_flow_log_p("max_dev_cnt(%u)\n", p_user->max_dev_count);

				rv = HD_OK;
			}
			break;

			case HD_VIDEODEC_PARAM_SYSCAPS:
			{
				HD_VIDEODEC_SYSCAPS* p_user = (HD_VIDEODEC_SYSCAPS*)p_param;
				int  i;
				memset(p_user, 0, sizeof(HD_VIDEODEC_SYSCAPS));
				p_user->dev_id        = self_id - HD_DEV_BASE + DEV_BASE;
				p_user->chip          = 0;
				p_user->max_in_count  = 16;
				p_user->max_out_count = 16;
				p_user->dev_caps      = HD_CAPS_PATHCONFIG;
				for (i=0; i<16; i++) p_user->in_caps[i] =
					HD_VIDEODEC_CAPS_JPEG
					 | HD_VIDEODEC_CAPS_H264
					 | HD_VIDEODEC_CAPS_H265;
				for (i=0; i<16; i++) p_user->out_caps[i] =
					HD_VIDEO_CAPS_YUV420
					 | HD_VIDEO_CAPS_YUV422;

				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip, p_user->max_in_count, p_user->max_out_count, p_user->dev_caps);
				{
					int i;
					hdal_flow_log_p("         in_caps    , out_caps =>\n");
					for (i=0; i<16; i++) {
						hdal_flow_log_p("    [%02d] 0x%08x , 0x%08x\n", i, p_user->in_caps[i], p_user->out_caps[i]);
					}
				}

				rv = HD_OK;
			}
			break;

			default: rv = HD_ERR_PARAM; break;
			}
		} else {
			switch(id) {
			case HD_VIDEODEC_PARAM_IN:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id , rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEODEC_IN *p_user = (HD_VIDEODEC_IN*)p_param;
					HD_VIDEODEC_IN *p_bufinfo = &videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_in_param;
					memcpy(p_user, p_bufinfo, sizeof(HD_VIDEODEC_IN));

					hdal_flow_log_p("codec(%u)\n", p_user->codec_type);
				}
				break;
			case HD_VIDEODEC_PARAM_PATH_CONFIG:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_VIDEODEC_PATH_CONFIG *p_user = (HD_VIDEODEC_PATH_CONFIG*)p_param;
					HD_VIDEODEC_PATH_CONFIG *p_bufinfo = &videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_path_cfg;
					memcpy(p_user, p_bufinfo, sizeof(HD_VIDEODEC_PATH_CONFIG));

					hdal_flow_log_p("codec(%u) dim(%ux%u)\n", p_user->max_mem.codec_type, p_user->max_mem.dim.w, p_user->max_mem.dim.h);
				}
				break;
			case HD_VIDEODEC_PARAM_BUFINFO:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					//check OPEN status first
					if (videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status != HD_VIDEODEC_STATUS_START) {
						DBG_ERR("HD_VIDEODEC_PARAM_BUFINFO could only be get after START\r\n");
						return HD_ERR_NOT_START;
					}

					HD_VIDEODEC_BUFINFO *p_user = (HD_VIDEODEC_BUFINFO*)p_param;
					HD_VIDEODEC_BUFINFO *p_bufinfo = &videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_buf_info;
					memcpy(p_user, p_bufinfo, sizeof(HD_VIDEODEC_BUFINFO));

					hdal_flow_log_p("addr(0x%08x) size(%u)\n", p_user->buf_info.phy_addr, p_user->buf_info.buf_size);
				}
				break;
			default:
				DBG_ERR("Invalid id = 0x%x\r\n", id);
				rv = HD_ERR_PARAM;
				break;
			}
		}
	}
	return rv;
}

HD_RESULT hd_videodec_set(HD_PATH_ID path_id, HD_VIDEODEC_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};

	{
		CHAR  param_name[20];
		_hd_videodec_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videodec_set(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
		DBG_ERR("dev_fd <= 0\r\n");
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (_hd_videodec_is_init() == FALSE) {
		return HD_ERR_UNINIT;
	}

	{
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	DBG_ERR("SELF_ID is invalid\r\n"); return rv;}
		rv = HD_OK;
		cmd.param = id;
		if(p_param == NULL) {
			DBG_ERR("p_param is NULL\r\n");
			return HD_ERR_NULL_PTR;
		}
		if (ctrl_id == HD_CTRL) {
			switch(id) {
			default: rv = HD_ERR_PARAM; break;
			}
		} else {
			switch(id) {
			case HD_VIDEODEC_PARAM_PATH_CONFIG:
			{
				// codec (in-port)
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_VIDEODEC_PATH_CONFIG* p_user = (HD_VIDEODEC_PATH_CONFIG*)p_param;
				HD_VIDEODEC_PATH_CONFIG *p_dec_path_cfg = &videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_path_cfg;
				memcpy(p_dec_path_cfg, p_user, sizeof(HD_VIDEODEC_PATH_CONFIG));

				hdal_flow_log_p("codec(%u) dim(%ux%u)\n", p_user->max_mem.codec_type, p_user->max_mem.dim.w, p_user->max_mem.dim.h);


				if (videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status == HD_VIDEODEC_STATUS_START) {
					DBG_WRN("please STOP first!!\r\n");
					return HD_ERR_NOT_ALLOW;
				}

				// codec
				cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = VDODEC_PARAM_CODEC;
				cmd.value = _hd_videodec_codec_hdal2unit(p_user->max_mem.codec_type);
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("set path_config codec fail ... !!\r\n");
					goto _HD_VD1;
				}

				// max_mem
				NMI_VDODEC_MAX_MEM_INFO max_mem;

				max_mem.uiVdoCodec = _hd_videodec_codec_hdal2unit(p_user->max_mem.codec_type);
				max_mem.uiWidth = p_user->max_mem.dim.w;
				max_mem.uiHeight = p_user->max_mem.dim.h;
				max_mem.bRelease = FALSE;

				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = VDODEC_PARAM_MAX_MEM_INFO;
				cmd.value = (UINT32)&max_mem;
				cmd.size = sizeof(NMI_VDODEC_MAX_MEM_INFO);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r == 0) {
					videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = TRUE;
				} else {
					DBG_ERR("Set MAX memory FAIL ... !!\r\n");
					goto _HD_VD1;
				}

				// [OUT]
				cmd.dest = ISF_PORT(self_id, out_id);

				// ddr_id
				{
					ISF_ATTR t_attr = {0};

					t_attr.attr = p_user->max_mem.ddr_id;
					cmd.param = ISF_UNIT_PARAM_ATTR;
					cmd.value = (UINT32)&t_attr;
					cmd.size = sizeof(ISF_ATTR);
					r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
					if (r != 0) goto _HD_VD1;
				}
				goto _HD_VD1;
			}
			break;

			case HD_VIDEODEC_PARAM_IN:
			{
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				ISF_SET set[1] = {0};
				HD_VIDEODEC_IN* p_user = (HD_VIDEODEC_IN*)p_param;
				HD_VIDEODEC_IN* p_dec_in_param = &videodec_info[self_id-DEV_BASE].port[in_id-IN_BASE].dec_in_param;

				if (videodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status == HD_VIDEODEC_STATUS_START) {
					DBG_WRN("please STOP first!!\r\n");
					return HD_ERR_NOT_ALLOW;
				}

				memcpy(p_dec_in_param, p_user, sizeof(HD_VIDEODEC_IN));

				hdal_flow_log_p("codec(%u)\n", p_user->codec_type);

				cmd.dest = ISF_PORT(self_id, in_id);

				set[0].param = VDODEC_PARAM_CODEC;           set[0].value = _hd_videodec_codec_hdal2unit(p_user->codec_type);

				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*1;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("param_in fail (CHK %d)\r\n", __LINE__);
					goto _HD_VD1;
				}

				goto _HD_VD1;
			}
			break;

			case HD_VIDEODEC_PARAM_IN_DESC:
			{
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_H26XDEC_DESC* p_user = (HD_H26XDEC_DESC*)p_param;
				HD_H26XDEC_DESC* p_in_desc = &videodec_info[self_id-DEV_BASE].port[in_id-IN_BASE].dec_h26x_desc;
				if (p_user->addr == 0) {
					DBG_ERR("Error desc_addr is 0\r\n");
					return HD_ERR_PARAM;
				}
				if (p_user->len > NMI_VDODEC_H26X_NAL_MAXSIZE || p_user->len == 0) {
					DBG_ERR("Error desc_len(%d), cannot equal to 0 or over max size 512\r\n", (int)p_user->len);
					return HD_ERR_PARAM;
				}
				memcpy(p_in_desc, p_user, sizeof(HD_H26XDEC_DESC));

				hdal_flow_log_p("desc_addr(0x%08x) desc_len(%u)\n", p_user->addr, p_user->len);

				// desc
				NMI_VDODEC_MEM_RANGE desc_mem = {0};

				desc_mem.Addr = p_user->addr;
				desc_mem.Size = p_user->len;

				cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = VDODEC_PARAM_DESC;
				cmd.value = (UINT32)&desc_mem;
				cmd.size = sizeof(NMI_VDODEC_MEM_RANGE);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("param_in_desc fail\r\n");
					goto _HD_VD1;
				}

				goto _HD_VD1;
			}
			break;

			default:
				DBG_ERR("invalid id=%d (CHK %d)\r\n", id, __LINE__);
				rv = HD_ERR_PARAM;
				break;
			}
		}
		if (rv != HD_OK) {
			return rv;
		}
		//r = 0;
	}
_HD_VD1:
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:
			rv = HD_OK;
			break;
		default:
			rv = HD_ERR_SYS;
			break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

void _hd_videodec_dewrap_frame(ISF_DATA *p_data, HD_VIDEO_FRAME* p_video_frame)
{
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_frame->blk; //isf-data-handle

	p_vdoframe->sign = p_video_frame->sign;
	p_vdoframe->p_next = NULL;
	p_vdoframe->resv1 = p_video_frame->ddr_id;
	p_vdoframe->pxlfmt = p_video_frame->pxlfmt;
	p_vdoframe->size.w = p_video_frame->dim.w;
	p_vdoframe->size.h = p_video_frame->dim.h;
	p_vdoframe->count = p_video_frame->count;
	p_vdoframe->timestamp = p_video_frame->timestamp;
	p_vdoframe->phyaddr[VDO_PINDEX_Y] = p_video_frame->phy_addr[HD_VIDEO_PINDEX_Y];
	p_vdoframe->phyaddr[VDO_PINDEX_UV] = p_video_frame->phy_addr[HD_VIDEO_PINDEX_UV];
	// user push out buf do not need those?
	//p_vdoframe->pw[VDO_PINDEX_Y] = p_video_frame->pw[HD_VIDEO_PINDEX_Y];
	//p_vdoframe->pw[VDO_PINDEX_UV] = p_video_frame->pw[HD_VIDEO_PINDEX_UV];
	//p_vdoframe->ph[VDO_PINDEX_Y] = p_video_frame->ph[HD_VIDEO_PINDEX_Y];
	//p_vdoframe->ph[VDO_PINDEX_UV] = p_video_frame->ph[HD_VIDEO_PINDEX_UV];
	//p_vdoframe->loff[VDO_PINDEX_Y] = p_video_frame->loff[HD_VIDEO_PINDEX_Y];
	//p_vdoframe->loff[VDO_PINDEX_UV] = p_video_frame->loff[HD_VIDEO_PINDEX_UV];
}

void _hd_videodec_dewrap_bs(ISF_DATA *p_data, HD_VIDEODEC_BS* p_video_bs)
{
	VDO_BITSTREAM * p_vdobs = (VDO_BITSTREAM *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_bs->blk;
	p_data->size = p_video_bs->size; // it could be the whole isf_data, but we only have one data bs_size

	// set vdo_bs info
	p_vdobs->sign      = p_video_bs->sign;
	p_vdobs->DataAddr  = p_video_bs->phy_addr;
	p_vdobs->DataSize  = p_video_bs->size;
	p_vdobs->CodecType = p_video_bs->vcodec_format;
	p_vdobs->count     = p_video_bs->count;
	p_vdobs->timestamp = p_video_bs->timestamp;
}

HD_RESULT hd_videodec_push_in_buf(HD_PATH_ID path_id, HD_VIDEODEC_BS* p_in_video_bitstream, HD_VIDEO_FRAME* p_user_out_video_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) { return rv;}

	if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { return HD_ERR_IO;}
	_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { return rv;}

	if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { return HD_ERR_IO;}
	_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (_hd_videodec_is_init() == FALSE)) {
		return HD_ERR_UNINIT;
	}

	if (p_in_video_bitstream == NULL) {
		DBG_ERR("p_in_video_bs is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	if (p_user_out_video_frame != NULL && p_user_out_video_frame->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("fourcc is not VFRM for user out frame\r\n");
		return HD_ERR_SIGN;
	}

	if (p_in_video_bitstream->sign != MAKEFOURCC('V','S','T','M')) {
		DBG_ERR("fourcc is not VSTM for video bs\r\n");
		return HD_ERR_SIGN;
	}

	{
		// push user_out_buf (fourcc: 'V','F','R','M')
		if (p_user_out_video_frame) {
			ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
			ISF_DATA data = {0};

			if (p_user_out_video_frame->blk == 0) {
				DBG_ERR("p_user_out_video_frame blk is 0\r\n");
				return HD_ERR_NULL_PTR;
			}

			if (p_user_out_video_frame->phy_addr[0] == 0) {
				DBG_ERR("p_user_out_video_frame pa is 0\r\n");
				return HD_ERR_NULL_PTR;
			}

			_hd_videodec_dewrap_frame(&data, p_user_out_video_frame);

			cmd.dest = ISF_PORT(self_id, in_id);
			cmd.p_data = &data;
			cmd.async = wait_ms;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_PUSH_DATA, &cmd); //if isf_unit return value != ISF_OK, r will be -1
			if (r == 0) {
				switch(cmd.rv) {
				case ISF_OK:
					rv = HD_OK;
					break;
				default:
					rv = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}

			if (rv != HD_OK && rv != HD_ERR_OVERRUN) {
				DBG_ERR("push in user_out_buf fail, r=%d, rv=%d\r\n", r, rv);
			}
		}

		// push video bs (fourcc: 'V','S','T','M')
		{
			ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
			ISF_DATA data = {0};

			if (p_in_video_bitstream->blk == 0) {
				DBG_ERR("p_in_video_bitstream blk is NULL\r\n");
				return HD_ERR_NULL_PTR;
			}

			_hd_videodec_dewrap_bs(&data, p_in_video_bitstream);

			cmd.dest = ISF_PORT(self_id, in_id);
			cmd.p_data = &data;
			cmd.async = wait_ms;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_PUSH_DATA, &cmd); //if isf_unit return value != ISF_OK, r will be -1
			if (r == 0) {
				switch(cmd.rv) {
				case ISF_OK:
					rv = HD_OK;
					break;
				default:
					rv = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}

			if (rv != HD_OK && rv != HD_ERR_OVERRUN) {
				DBG_ERR("push in vdo_bs fail, r=%d, rv=%d\r\n", r, rv);
			}
		}
	}

	return rv;
}

void _hd_videodec_rebuild_frame(ISF_DATA *p_data, HD_VIDEO_FRAME* p_video_frame)
{
#if 0
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;
	HD_DAL dev_id = p_video_frame->blk >> 16;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	//p_data->h_data = p_video_frame->blk; //isf-data-handle

	if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) {
		//check if dev_id is VIDEODEC
		dev_id -= HD_DEV_BASE;
		p_vdoframe->sign = MAKEFOURCC('V','D','E','C');
		p_data->h_data = (dev_id << 16) | (p_video_frame->blk & 0xFFFF); // refer to jira: NA51000-1384
	} else if (p_video_frame->blk == 0) {
		p_vdoframe->sign = MAKEFOURCC('V','F','R','M');
		p_data->h_data = 0;
	}
#else
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_frame->blk; //isf-data-handle

	p_vdoframe->sign       = p_video_frame->sign;
#endif
}

HD_RESULT hd_videodec_pull_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (_hd_videodec_is_init() == FALSE)) {
		return HD_ERR_UNINIT;
	}

	if (p_video_frame == NULL) {
		DBG_ERR("p_video_frame is NULL\r\n");
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
			case ISF_OK:
				rv = HD_OK;
				break;
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}

		//--- use ISF_DATA ---
		if (rv == HD_OK) {
			_hd_videodec_wrap_frame(p_video_frame, &data);
		} else {
			//DBG_ERR("pull out fail, r=%d, rv=%d\r\n", r, rv);
		}
	}
	return rv;
}

HD_RESULT hd_videodec_release_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (_hd_videodec_is_init() == FALSE)) {
		return HD_ERR_UNINIT;
	}

	if (p_video_frame == 0) {
		DBG_ERR("p_video_frame is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_videodec_rebuild_frame(&data, p_video_frame);

		cmd.dest = ISF_PORT(self_id, out_id);
		cmd.p_data = &data;
		cmd.async = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_RELEASE_DATA, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				rv = HD_OK;
				break;
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_videodec_send_list(HD_VIDEODEC_SEND_LIST *p_videodec_bs, UINT32 num, INT32 wait_ms)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videodec_uninit(VOID)
{
	UINT32 cfg, cfg1, cfg2;
	HD_RESULT rv = HD_OK;
	HD_PATH_ID path_id = 0;
	UINT32 dev, port;

	hdal_flow_log_p("hd_videodec_uninit\n");

	if (dev_fd <= 0) {
		return HD_ERR_INIT;
	}

	if (_hd_common_get_pid() > 0) {
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 0; //cmd = uninit
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = 0;
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("uninit fail? %d\r\n", r);
			return r;
		}
		return HD_OK;
	}

	if (videodec_info[0].port == NULL) {
		DBG_ERR("NOT init yet\n");
		return HD_ERR_INIT;
	}

	rv = _hd_common_get_init(&cfg);
	if (rv != HD_OK) {
		return HD_ERR_NO_CONFIG;
	}
	rv = hd_common_get_sysconfig(&cfg1, &cfg2);
	if (rv != HD_OK) {
		return HD_ERR_NO_CONFIG;
	}
	
	g_cfg_cpu = (cfg & HD_COMMON_CFG_CPU);
	g_cfg_debug = ((cfg & HD_COMMON_CFG_DEBUG) >> 12);
	g_cfg_vdec = ((cfg1 & HD_VIDEODEC_CFG) >> 12);
#if defined(_BSP_NA51055_)
	g_cfg_ai = ((cfg2 & VENDOR_AI_CFG) >> 16);
#endif

	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			if (videodec_info[dev].port[port].priv.status == HD_VIDEODEC_STATUS_START) {
				path_id = HD_VIDEODEC_PATH(HD_DAL_VIDEODEC(dev), HD_IN(port), HD_OUT(port));
				DBG_WRN("ERROR: path(%lu) is NOT stop/close yet....auto-call hd_videodec_stop() and hd_videodec_close() before attemp to uninit !!!\r\n", port);
				if (HD_OK != hd_videodec_stop(path_id)) {
					DBG_ERR("auto-call hd_videodec_stop(path%lu) error !!!\r\n", port);
					rv = HD_ERR_NG;
				}
				if (HD_OK != hd_videodec_close(path_id)) {
					DBG_ERR("auto-call hd_videodec_close(path%lu) error !!!\r\n", port);
					rv = HD_ERR_NG;
				}
				} else if (videodec_info[dev].port[port].priv.status == HD_VIDEODEC_STATUS_OPEN) {
					path_id = HD_VIDEODEC_PATH(HD_DAL_VIDEODEC(dev), HD_IN(port), HD_OUT(port));
					DBG_WRN("ERROR: path(%lu) is NOT close yet....auto-call hd_videodec_close() before attemp to uninit !!!\r\n", port);
					if (HD_OK != hd_videodec_close(path_id)) {
						DBG_ERR("auto-call hd_videodec_close(path%lu) error !!!\r\n", port);
						rv = HD_ERR_NG;
					}
				} else {
					// set status
					videodec_info[dev].port[port].priv.status = HD_VIDEODEC_STATUS_UNINIT; // [INIT]/[UNINIT] -> [UNINT]
				}
			}
		}

	//uninit drv
	{
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 0; //cmd = uninit
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = 0;
#if defined(_BSP_NA51055_)
		isf_dev_cmd.p2 = (g_cfg_cpu << 28) | (g_cfg_debug << 24) | (g_cfg_ai << 8) | (g_cfg_vdec);
#else
		isf_dev_cmd.p2 = (g_cfg_cpu << 28) | (g_cfg_debug << 24) | (g_cfg_vdec);
#endif
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("uninit fail? %d\r\n", r);
			return r;
		}
	}

	//uninit lib
	{
		free(videodec_info[0].port);
		videodec_info[0].port = NULL;
	}

	return rv;
}

int _hd_videodec_is_init(VOID)
{
	return (videodec_info[0].port != NULL);
}