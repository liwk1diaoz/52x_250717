/**
	@brief Source code of audio encoder module.\n
	This file contains the functions which is related to audio encoder in the chip.

	@file hd_audioenc.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME HD_AUDIOENC
#include "hd_int.h"
#include "hd_logger_p.h"
#include "kflow_audioenc/isf_audenc.h"
#include "kflow_videoenc/media_def.h"  // [ToDo] should separate it to media_vdef.h and media_adef.h

// INCLUDE_PROTECTED
#include "nmediarec_audenc.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define HD_AUDIOENC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE		ISF_UNIT_AUDENC
#define DEV_COUNT		1 //ISF_MAX_AUDENC
#define IN_BASE			ISF_IN_BASE
#define IN_COUNT		16
#define OUT_BASE		ISF_OUT_BASE
#define OUT_COUNT 		16

#define HD_DEV_BASE		HD_DAL_AUDIOENC_BASE
#define HD_DEV_MAX		HD_DAL_AUDIOENC_MAX

#define AUDENC_MAX_PATH_NUM 4

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef enum _HD_AUDIOENC_STATUS {
	HD_AUDIOENC_STATUS_CLOSE = 0,
	HD_AUDIOENC_STATUS_MAXMEM_DONE,
	HD_AUDIOENC_STATUS_OPEN,
	HD_AUDIOENC_STATUS_START,
	HD_AUDIOENC_STATUS_STOP,
	HD_AUDIOENC_STATUS_UNINIT,
	HD_AUDIOENC_STATUS_INIT,
} HD_AUDIOENC_STATUS;

typedef struct _AUDENC_INFO_PRIV
{
    HD_AUDIOENC_STATUS      status;                     ///< hd_audioenc_status
    BOOL                    b_maxmem_set;
} AUDENC_INFO_PRIV;

typedef struct _AUDENC_INFO_PORT {
	HD_AUDIOENC_BUFINFO       enc_buf_info;
	HD_AUDIOENC_PATH_CONFIG   enc_path_cfg;
	HD_AUDIOENC_IN            enc_in_param;
	HD_AUDIOENC_OUT           enc_out_param;
	HD_AUDIOENC_PATH_CONFIG2  enc_path_cfg2;

	//private data
	AUDENC_INFO_PRIV priv;
} AUDENC_INFO_PORT;

typedef struct _AUDENC_INFO {
	AUDENC_INFO_PORT *port;
} AUDENC_INFO;

#define AUDENC_PHY2VIRT(bs_phy_addr) (g_audenc_bs_virt_start_addr + (bs_phy_addr - g_audenc_bs_phy_start_addr))

#define TIMESTAMP_TO_MS(ts)  ((ts >> 32) * 1000 + (ts & 0xFFFFFFFF) / 1000)

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
		} else if(((dev_id) >= HD_DAL_AUDIOOUT_BASE) && ((dev_id) <= HD_DAL_AUDIOOUT_MAX)) { \
			(rv) = _hd_audioout_convert_dev_id(&(dev_id)); \
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

#define _HD_REVERT_BIND_ID(dev_id) \
	do { \
		if(((dev_id) >= ISF_UNIT_AUDCAP) && ((dev_id) < ISF_UNIT_AUDCAP + ISF_MAX_AUDCAP)) { \
			(dev_id) = HD_DAL_AUDIOCAP_BASE + (dev_id) - ISF_UNIT_AUDCAP; \
		} else if(((dev_id) >= ISF_UNIT_AUDOUT) && ((dev_id) < ISF_UNIT_AUDOUT + ISF_MAX_AUDOUT)) { \
			(dev_id) = HD_DAL_AUDIOOUT_BASE + (dev_id) - ISF_UNIT_AUDOUT;\
		} else if(((dev_id) >= ISF_UNIT_AUDENC) && ((dev_id) < ISF_UNIT_AUDENC + ISF_MAX_AUDENC)) { \
			(dev_id) = HD_DAL_AUDIOENC_BASE + (dev_id) - ISF_UNIT_AUDENC; \
		} else if(((dev_id) >= ISF_UNIT_AUDDEC) && ((dev_id) < ISF_UNIT_AUDDEC + ISF_MAX_AUDDEC)) { \
			(dev_id) = HD_DAL_AUDIODEC_BASE + (dev_id) - ISF_UNIT_AUDDEC; \
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

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
AUDENC_INFO audioenc_info[DEV_COUNT] = {0};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

void _hd_audioenc_cfg_max(UINT32 maxpath)
{
	if (!audioenc_info[0].port) {
		_max_path_count = maxpath;
		if (_max_path_count > AUDENC_MAX_PATH_NUM) {
			DBG_WRN("dts path=%lu is larger than built-in max_path=%lu!\r\n", _max_path_count, (UINT32)AUDENC_MAX_PATH_NUM);
			_max_path_count = AUDENC_MAX_PATH_NUM;
		}
	}
}

UINT32 _hd_audioenc_codec_hdal2unit(HD_AUDIO_CODEC h_codec)
{
	switch (h_codec)
	{
		case HD_AUDIO_CODEC_PCM:	return MOVAUDENC_PCM;
		case HD_AUDIO_CODEC_AAC:	return MOVAUDENC_AAC;
		case HD_AUDIO_CODEC_ULAW:	return MOVAUDENC_ULAW;
		case HD_AUDIO_CODEC_ALAW:	return MOVAUDENC_ALAW;
		default:
			DBG_ERR("unknown codec type(%d)\r\n", h_codec);
			return (-1);
	}
}

UINT32 _hd_audioenc_codec_unit2hdal(UINT32 u_codec)
{
	switch (u_codec)
	{
		case MOVAUDENC_PCM:		return HD_AUDIO_CODEC_PCM;
		case MOVAUDENC_AAC:		return HD_AUDIO_CODEC_AAC;
		case MOVAUDENC_ULAW:    return HD_AUDIO_CODEC_ULAW;
		case MOVAUDENC_ALAW:	return HD_AUDIO_CODEC_ALAW;
		default:
			DBG_ERR("unknown codec type(%lu)\r\n", u_codec);
			return (-1);
	}
}

HD_RESULT _hd_audioenc_convert_dev_id(HD_DAL* p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

HD_RESULT _hd_ae_get_param(HD_DAL self_id, HD_IO out_id, UINT32 param, UINT32 *value)
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

void _hd_audioenc_wrap_bs(HD_AUDIO_BS* p_audio_bs, ISF_DATA *p_data)
{
	PAUD_BITSTREAM p_audbs = (AUD_BITSTREAM *) p_data->desc;
	HD_DAL id = (p_data->h_data >> 16) + HD_DEV_BASE;

	p_audio_bs->blk = p_data->h_data; //isf-data-handle

	p_audio_bs->sign                  = MAKEFOURCC('A','S','T','M');
	p_audio_bs->p_next                = NULL;
	p_audio_bs->blk                   = (id << 16) | (p_data->h_data & 0xFFFF); // refer to jira: NA51000-1384
	p_audio_bs->acodec_format         = _hd_audioenc_codec_unit2hdal(p_audbs->CodecType);
	p_audio_bs->timestamp             = p_audbs->timestamp;
	p_audio_bs->phy_addr              = p_audbs->resv[11];
	p_audio_bs->size                  = p_audbs->DataSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

HD_RESULT hd_audioenc_init(VOID)
{
	UINT32 dev, port;
	UINT32 i;

	hdal_flow_log_p("hd_audioenc_init\n");

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

	if (audioenc_info[0].port != NULL) {
		DBG_ERR("already init\n");
		return HD_ERR_UNINIT;
	}

	if (_max_path_count == 0) {
		DBG_ERR("_max_path_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	audioenc_info[0].port = (AUDENC_INFO_PORT*)malloc(sizeof(AUDENC_INFO_PORT)*_max_path_count);
	if (audioenc_info[0].port == NULL) {
		DBG_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
		return HD_ERR_HEAP;
	}
	for(i = 0; i < _max_path_count; i++) {
		memset(&(audioenc_info[0].port[i]), 0, sizeof(AUDENC_INFO_PORT));  // no matter what status now...reset status = [UNINIT] anyway
	}

	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			// set status
			audioenc_info[dev].port[port].priv.status = HD_AUDIOENC_STATUS_INIT;  // [UNINT] -> [INIT]
		}
	}

	//init drv
	{
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = _max_path_count;
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			free(audioenc_info[0].port);
			audioenc_info[0].port = NULL;
			DBG_ERR("init fail? %d\r\n", r);
			return r;
		}
	}

	return HD_OK;
}

HD_RESULT hd_audioenc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioenc_bind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) dst(0x%x) in(%d)\n", self_id, out_id, dest_id, in_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioenc_info[0].port == NULL) {
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

HD_RESULT hd_audioenc_unbind(HD_OUT_ID _out_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioenc_unbind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d)\n", self_id, out_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioenc_info[0].port == NULL) {
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

HD_RESULT _hd_audioenc_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
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


HD_RESULT _hd_audioenc_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id)
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

HD_RESULT _hd_audioenc_get_state(HD_OUT_ID _out_id, UINT32* p_state)
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

HD_RESULT hd_audioenc_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioenc_open:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) in(%d) ", self_id, out_id, in_id);

	if (dev_fd <= 0) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioenc_info[0].port == NULL) {
		return HD_ERR_UNINIT;
	}

	if(!p_path_id) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_NULL_PTR;
	}

	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		_HD_CONVERT_SELF_ID(self_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)DEV_COUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
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
		#if 0 //Logically dead code
		if(in_dev_id != self_id) { hdal_flow_log_p("path_id(N/A)\n");	return HD_ERR_DEV;}
		#endif
		p_path_id[0] = HD_AUDIOENC_PATH(cur_dev, in_id, out_id);
	}

	hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_OUT_ID(out_id, rv);
		if(rv != HD_OK) {
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
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
		//hd_audioenc_vars[out_id-OUT_BASE].status = HD_AUDIOENC_STATUS_OPEN;
		audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_AUDIOENC_STATUS_OPEN;
	}

	return rv;
}

HD_RESULT hd_audioenc_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioenc_start:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioenc_info[0].port == NULL) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		//--- check if MAX_MEM info had been set ---
		if (audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set == FALSE) {
			DBG_ERR("Error: please set HD_AUDIOENC_PARAM_MAXMEM_CONFIG first !!!\r\n");
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
		//hd_audioenc_vars[out_id-OUT_BASE].status = HD_AUDIOENC_STATUS_START;
		audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_AUDIOENC_STATUS_START;
	}

	{
		// get physical addr/size of bs buffer
		HD_BUFINFO *p_bufinfo = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_buf_info.buf_info;
		_hd_ae_get_param(self_id, out_id, AUDENC_PARAM_BUFINFO_PHYADDR, &p_bufinfo->phy_addr);
		_hd_ae_get_param(self_id, out_id, AUDENC_PARAM_BUFINFO_SIZE,    &p_bufinfo->buf_size);
		//printf("phy_addr=0x%x, buf_size=0x%x\r\n", p_bufinfo->phy_addr, p_bufinfo->buf_size);
	}

	return rv;
}

HD_RESULT hd_audioenc_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioenc_stop:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioenc_info[0].port == NULL) {
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
		//hd_audioenc_vars[out_id-OUT_BASE].status = HD_AUDIOENC_STATUS_STOP;
		audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_AUDIOENC_STATUS_STOP;
	}

	return rv;
}

HD_RESULT hd_audioenc_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audioenc_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audioenc_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioenc_close:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioenc_info[0].port == NULL) {
		return HD_ERR_UNINIT;
	}

	if(ctrl_id == HD_CTRL) {
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		//do nothing
		return HD_OK;
	}

	_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_OUT_ID(out_id, rv);		if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_IN_ID(in_id, rv);		if(rv != HD_OK) {	return rv;}

	{
		// release MAX_MEM
		{
			ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
			NMI_AUDENC_MAX_MEM_INFO max_mem = {0};

			max_mem.bRelease = TRUE;

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = AUDENC_PARAM_MAX_MEM_INFO;
			cmd.value = (UINT32)&max_mem;
			cmd.size = sizeof(NMI_AUDENC_MAX_MEM_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r == 0) {
				audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = FALSE;
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

		// reset HD_AUDIOENC_PARAM_IN
		{
			ISF_SET set[3] = {0};
			HD_AUDIOENC_IN* p_enc_in_param = &audioenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_in_param;

			p_enc_in_param->sample_rate = 0;
			p_enc_in_param->mode        = 0;
			p_enc_in_param->sample_bit  = 0;
			hdal_flow_log_p("sr(%u) ch(%u) bit(%u)\n", p_enc_in_param->sample_rate, p_enc_in_param->mode, p_enc_in_param->sample_bit);

			cmd.dest = ISF_PORT(self_id, in_id);

			set[0].param = AUDENC_PARAM_SAMPLERATE;      set[0].value = p_enc_in_param->sample_rate;
			set[1].param = AUDENC_PARAM_CHS;             set[1].value = p_enc_in_param->mode;
			set[2].param = AUDENC_PARAM_BITS;            set[2].value = p_enc_in_param->sample_bit;

			cmd.param = ISF_UNIT_PARAM_MULTI;
			cmd.value = (UINT32)set;
			cmd.size = sizeof(ISF_SET)*3;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r != 0) {
				DBG_ERR("reset HD_AUDIOENC_PARAM_IN fail\r\n");
			}
		}

		// reset HD_AUDIOENC_PARAM_OUT
		{
			ISF_SET set[4] = {0};
			HD_AUDIOENC_OUT* p_enc_out_param = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_out_param;

			p_enc_out_param->codec_type = 0;
			p_enc_out_param->aac_adts = 0;
			hdal_flow_log_p("codec(%u)\n", p_enc_out_param->codec_type);
			hdal_flow_log_p("adts(%u)\n", p_enc_out_param->aac_adts);

			cmd.dest = ISF_PORT(self_id, out_id);

			set[0].param = AUDENC_PARAM_CODEC;            set[0].value = p_enc_out_param->codec_type;
			set[1].param = AUDENC_PARAM_AAC_ADTS_HEADER;  set[1].value = p_enc_out_param->aac_adts;
			set[2].param = AUDENC_PARAM_RECFORMAT;        set[2].value = 0;
			set[3].param = AUDENC_PARAM_PORT_OUTPUT_FMT;  set[3].value = 0;

			cmd.param = ISF_UNIT_PARAM_MULTI;
			cmd.value = (UINT32)set;
			cmd.size = sizeof(ISF_SET)*4;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r != 0) {
				DBG_ERR("reset HD_AUDIOENC_PARAM_OUT\r\n");
			}
		}
	}

	if (rv == HD_OK) {
		//hd_audioenc_vars[out_id-OUT_BASE].status = HD_AUDIOENC_STATUS_CLOSE;
		audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_AUDIOENC_STATUS_CLOSE;
	}

	return rv;
}

INT _hd_audioenc_param_cvt_name(HD_AUDIOENC_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
		case HD_AUDIOENC_PARAM_DEVCOUNT:            snprintf(p_ret_string, max_str_len, "DEVCOUNT");           break;
		case HD_AUDIOENC_PARAM_SYSCAPS:             snprintf(p_ret_string, max_str_len, "SYSCAPS");            break;
		case HD_AUDIOENC_PARAM_PATH_CONFIG:         snprintf(p_ret_string, max_str_len, "PATH_CONFIG");        break;
		case HD_AUDIOENC_PARAM_BUFINFO:             snprintf(p_ret_string, max_str_len, "BUFINFO");            break;
		case HD_AUDIOENC_PARAM_IN:                  snprintf(p_ret_string, max_str_len, "IN");                 break;
		case HD_AUDIOENC_PARAM_OUT:                 snprintf(p_ret_string, max_str_len, "OUT");                break;
		case HD_AUDIOENC_PARAM_PATH_CONFIG2:        snprintf(p_ret_string, max_str_len, "PATH_CONFIG2");       break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", id);
			return (-1);
	}
	return 0;
}

HD_RESULT hd_audioenc_get(HD_PATH_ID path_id, HD_AUDIOENC_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	//HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	//int r;

	{
		CHAR  param_name[20];
		_hd_audioenc_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_audioenc_get(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() > 0) && (id != HD_AUDIOENC_PARAM_BUFINFO)){
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if ((_hd_common_get_pid() == 0) &&(audioenc_info[0].port == NULL)) {
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

			case HD_AUDIOENC_PARAM_DEVCOUNT:
			{
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
				p_user->max_dev_count = 1;

				hdal_flow_log_p("max_dev_cnt(%u)\n", p_user->max_dev_count);

				rv = HD_OK;
			}
			break;

			case HD_AUDIOENC_PARAM_SYSCAPS:
			{
				HD_AUDIOENC_SYSCAPS* p_user = (HD_AUDIOENC_SYSCAPS*)p_param;
				int  i;
				memset(p_user, 0, sizeof(HD_AUDIOENC_SYSCAPS));
				p_user->dev_id        = self_id - HD_DEV_BASE + DEV_BASE;
				p_user->chip_id       = 0;
				p_user->max_in_count  = 16;
				p_user->max_out_count = 16;
				p_user->dev_caps      = HD_CAPS_PATHCONFIG;
				for (i=0; i<16; i++) p_user->in_caps[i] =
					HD_AUDIO_CAPS_BITS_8
					 | HD_AUDIO_CAPS_BITS_16
					 | HD_AUDIO_CAPS_CH_MONO
					 | HD_AUDIO_CAPS_CH_STEREO;
				for (i=0; i<16; i++) p_user->out_caps[i] =
					HD_AUDIOENC_CAPS_PCM
					 | HD_AUDIOENC_CAPS_AAC
					 | HD_AUDIOENC_CAPS_ADPCM
					 | HD_AUDIOENC_CAPS_ULAW
					 | HD_AUDIOENC_CAPS_ALAW;

				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip_id, p_user->max_in_count, p_user->max_out_count, p_user->dev_caps);
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
			case HD_AUDIOENC_PARAM_IN:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id , rv);	if(rv != HD_OK) {	return rv;}

					HD_AUDIOENC_IN *p_user = (HD_AUDIOENC_IN*)p_param;
					HD_AUDIOENC_IN *p_bufinfo = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_in_param;
					memcpy(p_user, p_bufinfo, sizeof(HD_AUDIOENC_IN));

					hdal_flow_log_p("sr(%u) ch(%u) bit(%u)\n", p_user->sample_rate, p_user->mode, p_user->sample_bit);
				}
				break;
			case HD_AUDIOENC_PARAM_PATH_CONFIG:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_AUDIOENC_PATH_CONFIG *p_user = (HD_AUDIOENC_PATH_CONFIG*)p_param;
					HD_AUDIOENC_PATH_CONFIG *p_bufinfo = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_path_cfg;
					memcpy(p_user, p_bufinfo, sizeof(HD_AUDIOENC_PATH_CONFIG));

					hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u)\n", p_user->max_mem.codec_type, p_user->max_mem.sample_rate, p_user->max_mem.mode, p_user->max_mem.sample_bit);
				}
				break;
			case HD_AUDIOENC_PARAM_PATH_CONFIG2:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_AUDIOENC_PATH_CONFIG2 *p_user = (HD_AUDIOENC_PATH_CONFIG2*)p_param;
					HD_AUDIOENC_PATH_CONFIG2 *p_bufinfo2 = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_path_cfg2;
					memcpy(p_user, p_bufinfo2, sizeof(HD_AUDIOENC_PATH_CONFIG2));

					hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u) buf_ms(%u)\n", p_user->max_mem.codec_type, p_user->max_mem.sample_rate, p_user->max_mem.mode, p_user->max_mem.sample_bit, p_user->max_mem.buf_ms);
				}
				break;
			case HD_AUDIOENC_PARAM_BUFINFO:
				{
					HD_AUDIOENC_BUFINFO *p_user = (HD_AUDIOENC_BUFINFO*)p_param;

					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}


					if (_hd_common_get_pid() == 0) { // main process
						HD_AUDIOENC_BUFINFO *p_bufinfo = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_buf_info;

						//check OPEN status first
						if (audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status != HD_AUDIOENC_STATUS_START) {
							DBG_ERR("HD_AUDIOENC_PARAM_BUFINFO could only be get after START\r\n");
							return HD_ERR_NOT_START;
						}

						memcpy(p_user, p_bufinfo, sizeof(HD_AUDIOENC_BUFINFO));
					} else { // client process
						// query kernel to get physical addr/size of bs buffer
						HD_BUFINFO bufinfo = {0};

						_hd_ae_get_param(self_id, out_id, AUDENC_PARAM_BUFINFO_PHYADDR, &bufinfo.phy_addr);
						_hd_ae_get_param(self_id, out_id, AUDENC_PARAM_BUFINFO_SIZE,    &bufinfo.buf_size);
						memcpy(&p_user->buf_info, &bufinfo, sizeof(HD_BUFINFO));
					}
					hdal_flow_log_p("addr(0x%08x) size(%u)\n", p_user->buf_info.phy_addr, p_user->buf_info.buf_size);
				}
				break;
			case HD_AUDIOENC_PARAM_OUT:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_AUDIOENC_OUT *p_user = (HD_AUDIOENC_OUT*)p_param;
					HD_AUDIOENC_OUT *p_bufinfo = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_out_param;
					memcpy(p_user, p_bufinfo, sizeof(HD_AUDIOENC_OUT));

					{
						hdal_flow_log_p("codec(%u) ", p_user->codec_type);
						if (p_user->codec_type == HD_AUDIO_CODEC_AAC) {
							hdal_flow_log_p("adts(%u)\n", p_user->aac_adts);
						}
					}
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

HD_RESULT hd_audioenc_set(HD_PATH_ID path_id, HD_AUDIOENC_PARAM_ID id, VOID* p_param)
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
		_hd_audioenc_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_audioenc_set(%s):\n", param_name);
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

	if (audioenc_info[0].port == NULL) {
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
			default:
				rv = HD_ERR_PARAM;
				break;
			}
		} else {
			switch(id) {
			case HD_AUDIOENC_PARAM_PATH_CONFIG:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_AUDIOENC_PATH_CONFIG* p_user = (HD_AUDIOENC_PATH_CONFIG*)p_param;
				HD_AUDIOENC_PATH_CONFIG *p_enc_path_cfg = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_path_cfg;

				if ((p_user->max_mem.codec_type != p_enc_path_cfg->max_mem.codec_type) &&
					(audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status == HD_AUDIOENC_STATUS_START)) {
					DBG_WRN("update codec_type fail, please STOP first!!\r\n");
					return HD_ERR_NOT_ALLOW;
				}

				memcpy(p_enc_path_cfg, p_user, sizeof(HD_AUDIOENC_PATH_CONFIG));

				hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u)\n",
						p_user->max_mem.codec_type, p_user->max_mem.sample_rate, p_user->max_mem.mode, p_user->max_mem.sample_bit);

				cmd.dest = ISF_PORT(self_id, out_id);

				// codec
				cmd.param = AUDENC_PARAM_CODEC;
				cmd.value = _hd_audioenc_codec_hdal2unit(p_user->max_mem.codec_type);
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("set path_config codec fail, r(%d)\r\n", r);
					goto _HD_AE1;
				}

				// encode buffer length in ms
				cmd.param = AUDENC_PARAM_ENCBUF_MS;
				cmd.value = 0;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("path_config buf_ms fail, r(%d)\r\n", r);
					goto _HD_AE1;
				}

				// max_mem
				NMI_AUDENC_MAX_MEM_INFO max_mem;

				max_mem.uiAudCodec = _hd_audioenc_codec_hdal2unit(p_user->max_mem.codec_type);
				max_mem.uiAudBits = p_user->max_mem.sample_bit;
				max_mem.uiAudChannels = (p_user->max_mem.mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				max_mem.uiAudioSR = p_user->max_mem.sample_rate;
				max_mem.uiRecFormat = MEDIAREC_LIVEVIEW;
				max_mem.bRelease = FALSE;

				cmd.param = AUDENC_PARAM_MAX_MEM_INFO;
				cmd.value = (UINT32)&max_mem;
				cmd.size = sizeof(NMI_AUDENC_MAX_MEM_INFO);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

				if (r == 0) {
					audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = TRUE;
				} else {
					DBG_ERR("Set MAX memory FAIL ... !!\r\n");
				}

				goto _HD_AE1;
			}
			break;
			case HD_AUDIOENC_PARAM_PATH_CONFIG2:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_AUDIOENC_PATH_CONFIG2* p_user = (HD_AUDIOENC_PATH_CONFIG2*)p_param;
				HD_AUDIOENC_PATH_CONFIG2 *p_enc_path_cfg2 = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_path_cfg2;

				if ((p_user->max_mem.codec_type != p_enc_path_cfg2->max_mem.codec_type) &&
					(audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status == HD_AUDIOENC_STATUS_START)) {
					DBG_WRN("update codec_type fail, please STOP first!!\r\n");
					return HD_ERR_NOT_ALLOW;
				}

				memcpy(p_enc_path_cfg2, p_user, sizeof(HD_AUDIOENC_PATH_CONFIG2));

				hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u) buf_ms(%lu)\n",
						p_user->max_mem.codec_type, p_user->max_mem.sample_rate, p_user->max_mem.mode, p_user->max_mem.sample_bit, p_user->max_mem.buf_ms);

				cmd.dest = ISF_PORT(self_id, out_id);

				// codec
				cmd.param = AUDENC_PARAM_CODEC;
				cmd.value = _hd_audioenc_codec_hdal2unit(p_user->max_mem.codec_type);
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("set path_config codec fail, r(%d)\r\n", r);
					goto _HD_AE1;
				}

				// encode buffer length in ms
				cmd.param = AUDENC_PARAM_ENCBUF_MS;
				cmd.value = p_user->max_mem.buf_ms;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("path_config buf_ms fail, r(%d)\r\n", r);
					goto _HD_AE1;
				}

				// max_mem
				NMI_AUDENC_MAX_MEM_INFO max_mem;

				max_mem.uiAudCodec = _hd_audioenc_codec_hdal2unit(p_user->max_mem.codec_type);
				max_mem.uiAudBits = p_user->max_mem.sample_bit;
				max_mem.uiAudChannels = (p_user->max_mem.mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				max_mem.uiAudioSR = p_user->max_mem.sample_rate;
				max_mem.uiRecFormat = MEDIAREC_LIVEVIEW;
				max_mem.bRelease = FALSE;

				cmd.param = AUDENC_PARAM_MAX_MEM_INFO;
				cmd.value = (UINT32)&max_mem;
				cmd.size = sizeof(NMI_AUDENC_MAX_MEM_INFO);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

				if (r == 0) {
					audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = TRUE;
				} else {
					DBG_ERR("Set MAX memory FAIL ... !!\r\n");
				}

				goto _HD_AE1;
			}
			break;

			case HD_AUDIOENC_PARAM_IN:
			{
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

				ISF_SET set[3] = {0};
				HD_AUDIOENC_IN* p_user = (HD_AUDIOENC_IN*)p_param;
				HD_AUDIOENC_IN* p_enc_in_param = &audioenc_info[self_id-DEV_BASE].port[in_id-IN_BASE].enc_in_param;
				memcpy(p_enc_in_param, p_user, sizeof(HD_AUDIOENC_IN));

				hdal_flow_log_p("sr(%u) ch(%u) bit(%u)\n", p_user->sample_rate, p_user->mode, p_user->sample_bit);

				cmd.dest = ISF_PORT(self_id, in_id);

				set[0].param = AUDENC_PARAM_SAMPLERATE;      set[0].value = p_user->sample_rate;
				set[1].param = AUDENC_PARAM_CHS;             set[1].value = (p_user->mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				set[2].param = AUDENC_PARAM_BITS;            set[2].value = p_user->sample_bit;

				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*3;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("Set param_in fail, r(%d)\r\n", r);
					goto _HD_AE1;
				}

				goto _HD_AE1;
			}
			break;

			case HD_AUDIOENC_PARAM_OUT:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				ISF_SET set[4] = {0};
				HD_AUDIOENC_OUT* p_user = (HD_AUDIOENC_OUT*)p_param;
				HD_AUDIOENC_OUT* p_enc_out_param = &audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].enc_out_param;

				if ((p_user->codec_type != p_enc_out_param->codec_type) &&
					(audioenc_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status == HD_AUDIOENC_STATUS_START)) {
					DBG_WRN("update codec_type fail, please STOP first!!\r\n");
					return HD_ERR_NOT_ALLOW;
				}

				memcpy(p_enc_out_param, p_user, sizeof(HD_AUDIOENC_OUT));

				{
					hdal_flow_log_p("codec(%u)\n", p_user->codec_type);
					if (p_user->codec_type == HD_AUDIO_CODEC_AAC) {
						hdal_flow_log_p("adts(%u)\n", p_user->aac_adts);
					}
				}

				cmd.dest = ISF_PORT(self_id, out_id);

				set[0].param = AUDENC_PARAM_CODEC;           set[0].value = _hd_audioenc_codec_hdal2unit(p_user->codec_type);
				set[1].param = AUDENC_PARAM_AAC_ADTS_HEADER; set[1].value = p_user->aac_adts;
				set[2].param = AUDENC_PARAM_RECFORMAT;       set[2].value = MEDIAREC_LIVEVIEW;
				if (p_user->codec_type == HD_AUDIO_CODEC_PCM) {
					set[3].param = AUDENC_PARAM_PORT_OUTPUT_FMT;  set[3].value = AUDENC_OUTPUT_UNCOMPRESSION;
				} else {
					set[3].param = AUDENC_PARAM_PORT_OUTPUT_FMT;  set[3].value = AUDENC_OUTPUT_COMPRESSION;
				}

				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*4;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("Set param_out fail, r(%d)\r\n", r);
					goto _HD_AE1;
				}

				goto _HD_AE1;
			}
			break;

			default:
				DBG_ERR("Set invalid id(%d)\r\n", id);
				rv = HD_ERR_PARAM;
				break;
			}
		}
		if (rv != HD_OK) {
			return rv;
		}
		//r = 0;
	}
_HD_AE1:
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

void _hd_audioenc_dewrap_frame(ISF_DATA *p_data, HD_AUDIO_FRAME* p_audio_frame)
{
	AUD_FRAME * p_audframe = (AUD_FRAME *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');

	p_audframe->sign        = p_audio_frame->sign;
	//p_audframe->p_next      = p_audio_frame->p_next;  // TBD: it will cause "error: assignment from incompatible pointer type"
	//p_audframe->ddr_id      = p_audio_frame->ddr_id;  // AUD_FRAME has no member named &ddr_id
	p_audframe->bit_width   = p_audio_frame->bit_width;
	p_audframe->sound_mode  = p_audio_frame->sound_mode;
	p_audframe->sample_rate = p_audio_frame->sample_rate;
	p_audframe->count       = p_audio_frame->count;
	p_audframe->timestamp   = p_audio_frame->timestamp;
	p_audframe->phyaddr[0]  = p_audio_frame->phy_addr[0];
	p_audframe->phyaddr[1]  = p_audio_frame->phy_addr[1];
	p_audframe->size        = p_audio_frame->size;
}

HD_RESULT hd_audioenc_push_in_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME* p_audio_frame, HD_AUDIO_BS* p_user_out_audio_bs, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (audioenc_info[0].port == NULL)) {
		return HD_ERR_UNINIT;
	}

	if (p_audio_frame == NULL) {
		DBG_ERR("p_audio_frame is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	if (p_audio_frame->phy_addr[0] == 0) {
        DBG_ERR("Check HD_AUDIO_FRAME phy_addr[0] is NULL.\n");
        return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_audioenc_dewrap_frame(&data, p_audio_frame);

		cmd.dest = ISF_PORT(self_id, in_id);
		cmd.p_data = &data;
		cmd.async = wait_ms;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_PUSH_DATA, &cmd);
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

void _hd_audioenc_rebuild_bs(ISF_DATA *p_data, HD_AUDIO_BS* p_audio_bs)
{
	AUD_BITSTREAM * p_audbs = (AUD_BITSTREAM *) p_data->desc;
	HD_DAL dev_id = p_audio_bs->blk >> 16;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	//p_data->h_data = p_audio_bs->blk; //isf-data-handle

	if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) {
		//check if dev_id is AUDIOENC
		dev_id -= HD_DEV_BASE;
		p_audbs->sign = MAKEFOURCC('A','E','N','C');
		p_data->h_data = (dev_id << 16) | (p_audio_bs->blk & 0xFFFF); // refer to jira: NA51000-1384
	} else if (p_audio_bs->blk == 0) {
		p_audbs->sign = MAKEFOURCC('A','S','T','M');
		p_data->h_data = 0;
	}
}

HD_RESULT hd_audioenc_pull_out_buf(HD_PATH_ID path_id, HD_AUDIO_BS* p_audio_bs, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (audioenc_info[0].port == NULL)) {
		return HD_ERR_UNINIT;
	}

	if (p_audio_bs == NULL) {
		DBG_ERR("p_audio_bs is NULL\r\n");
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
			_hd_audioenc_wrap_bs(p_audio_bs, &data);
		}
	}
	return rv;
}

HD_RESULT hd_audioenc_release_out_buf(HD_PATH_ID path_id, HD_AUDIO_BS* p_audio_bs)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (audioenc_info[0].port == NULL)) {
		return HD_ERR_UNINIT;
	}

	if (p_audio_bs == NULL) {
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_audioenc_rebuild_bs(&data, p_audio_bs);

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

HD_RESULT hd_audioenc_poll_list(HD_AUDIOENC_POLL_LIST *p_poll, UINT32 num, INT32 wait_ms)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audioenc_recv_list(HD_AUDIOENC_RECV_LIST *p_audio_bs, UINT32 num)
{
	return HD_ERR_NOT_SUPPORT;
}


HD_RESULT hd_audioenc_uninit(VOID)
{
	HD_RESULT rv = HD_OK;
	HD_PATH_ID path_id = 0;
	UINT32 dev, port;

	hdal_flow_log_p("hd_audioenc_uninit\n");

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

	if (audioenc_info[0].port == NULL) {
		DBG_ERR("NOT init yet\n");
		return HD_ERR_INIT;
	}

	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			if (audioenc_info[dev].port[port].priv.status == HD_AUDIOENC_STATUS_START) {
				path_id = HD_AUDIOENC_PATH(HD_DAL_AUDIOENC(dev), HD_IN(port), HD_OUT(port));
				DBG_WRN("ERROR: path(%lu) is NOT stop/close yet....auto-call hd_audioenc_stop() and hd_audioenc_close() before attemp to uninit !!!\r\n", port);
				if (HD_OK != hd_audioenc_stop(path_id)) {
					DBG_ERR("auto-call hd_audioenc_stop(path%lu) error !!!\r\n", port);
					rv = HD_ERR_NG;
				}
				if (HD_OK != hd_audioenc_close(path_id)) {
					DBG_ERR("auto-call hd_audioenc_close(path%lu) error !!!\r\n", port);
					rv = HD_ERR_NG;
				}
			} else if (audioenc_info[dev].port[port].priv.status == HD_AUDIOENC_STATUS_OPEN) {
				path_id = HD_AUDIOENC_PATH(HD_DAL_AUDIOENC(dev), HD_IN(port), HD_OUT(port));
				DBG_WRN("ERROR: path(%lu) is NOT close yet....auto-call hd_audioenc_close() before attemp to uninit !!!\r\n", port);
				if (HD_OK != hd_audioenc_close(path_id)) {
					DBG_ERR("auto-call hd_audioenc_close(path%lu) error !!!\r\n", port);
					rv = HD_ERR_NG;
				}
			} else {
				// set status
				audioenc_info[dev].port[port].priv.status = HD_AUDIOENC_STATUS_UNINIT; // [INIT]/[UNINIT] -> [UNINT]
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
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("uninit fail? %d\r\n", r);
			return r;
		}
	}

	//uninit lib
	{
		free(audioenc_info[0].port);
		audioenc_info[0].port = NULL;
	}

	return rv;
}

int _hd_audioenc_is_init(VOID)
{
	return (audioenc_info[0].port != NULL);
}