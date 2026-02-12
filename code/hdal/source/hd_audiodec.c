/**
	@brief Source code of audio decoder module.\n
	This file contains the functions which is related to audio decoder in the chip.

	@file hd_audiodec.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME HD_AUDIODEC
#include "hd_int.h"
#include "hd_logger_p.h"
#include "kflow_audiodec/isf_auddec.h"
#include "kflow_videoenc/media_def.h"  // [ToDo] should separate it to media_vdef.h and media_adef.h

// INCLUDE_PROTECTED
#include "nmediaplay_auddec.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define HD_AUDIODEC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE		ISF_UNIT_AUDDEC
#define DEV_COUNT		1
#define IN_BASE			ISF_IN_BASE
#define IN_COUNT		16
#define OUT_BASE		ISF_OUT_BASE
#define OUT_COUNT 		16

#define HD_DEV_BASE		HD_DAL_AUDIODEC_BASE
#define HD_DEV_MAX		HD_DAL_AUDIODEC_MAX

#define AUDDEC_MAX_PATH_NUM 4

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef enum _HD_AUDIODEC_STATUS {
	HD_AUDIODEC_STATUS_CLOSE = 0,
	HD_AUDIODEC_STATUS_MAXMEM_DONE,
	HD_AUDIODEC_STATUS_OPEN,
	HD_AUDIODEC_STATUS_START,
	HD_AUDIODEC_STATUS_STOP,
	HD_AUDIODEC_STATUS_UNINIT,
	HD_AUDIODEC_STATUS_INIT,
} HD_AUDIODEC_STATUS;

typedef struct _AUDDEC_INFO_PRIV
{
    HD_AUDIODEC_STATUS      status;                     ///< hd_audiodec_status
    BOOL                    b_maxmem_set;
} AUDDEC_INFO_PRIV;

typedef struct _AUDDEC_INFO_PORT {
	HD_AUDIODEC_BUFINFO       dec_buf_info;
	HD_AUDIODEC_PATH_CONFIG   dec_path_cfg;
	HD_AUDIODEC_IN            dec_in_param;

	//private data
	AUDDEC_INFO_PRIV priv;
} AUDDEC_INFO_PORT;

typedef struct _AUDDEC_INFO {
	AUDDEC_INFO_PORT *port;
} AUDDEC_INFO;

#define AUDDEC_PHY2VIRT(bs_phy_addr) (g_auddec_bs_virt_start_addr + (bs_phy_addr - g_auddec_bs_phy_start_addr))

#define TIMESTAMP_TO_MS(ts)  ((ts >> 32) * 1000 + (ts & 0xFFFFFFFF) / 1000)

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
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
AUDDEC_INFO audiodec_info[DEV_COUNT] = {0};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

void _hd_audiodec_cfg_max(UINT32 maxpath)
{
	if (!audiodec_info[0].port) {
		_max_path_count = maxpath;
		if (_max_path_count > AUDDEC_MAX_PATH_NUM) {
			DBG_WRN("dts path=%lu is larger than built-in max_path=%lu!\r\n", _max_path_count, (UINT32)AUDDEC_MAX_PATH_NUM);
			_max_path_count = AUDDEC_MAX_PATH_NUM;
		}
	}
}

UINT32 _hd_audiodec_codec_hdal2unit(HD_AUDIO_CODEC h_codec)
{
	switch (h_codec)
	{
		case HD_AUDIO_CODEC_PCM:	return AUDDEC_DECODER_PCM;
		case HD_AUDIO_CODEC_AAC:	return AUDDEC_DECODER_AAC;
		case HD_AUDIO_CODEC_ULAW:	return AUDDEC_DECODER_ULAW;
		case HD_AUDIO_CODEC_ALAW:	return AUDDEC_DECODER_ALAW;
		default:
			DBG_ERR("unknown codec type(%d)\r\n", h_codec);
			return (-1);
	}
}

UINT32 _hd_audiodec_codec_unit2hdal(UINT32 u_codec)
{
	switch (u_codec)
	{
		case AUDDEC_DECODER_PCM:	return HD_AUDIO_CODEC_PCM;
		case AUDDEC_DECODER_AAC:	return HD_AUDIO_CODEC_AAC;
		case AUDDEC_DECODER_ULAW:	return HD_AUDIO_CODEC_ULAW;
		case AUDDEC_DECODER_ALAW:	return HD_AUDIO_CODEC_ALAW;
		default:
			DBG_ERR("unknown codec type(%lu)\r\n", u_codec);
			return (-1);
	}
}

HD_RESULT _hd_audiodec_convert_dev_id(HD_DAL* p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

HD_RESULT _hd_ad_get_param(HD_DAL self_id, HD_IO out_id, UINT32 param, UINT32 *value)
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

void _hd_audiodec_wrap_frm(HD_AUDIO_FRAME* p_audio_frm, ISF_DATA *p_data)
{
	PAUD_FRAME p_audfrm = (AUD_FRAME *) p_data->desc;
	HD_DAL id = (p_data->h_data >> 16) + HD_DEV_BASE;

	p_audio_frm->blk = p_data->h_data; //isf-data-handle

	p_audio_frm->sign                  = MAKEFOURCC('A','F','R','M');
	p_audio_frm->blk                   = (id << 16) | (p_data->h_data & 0xFFFF); // refer to jira: NA51000-1384
	p_audio_frm->p_next                = NULL;
	p_audio_frm->timestamp             = p_audfrm->timestamp;
	p_audio_frm->phy_addr[0]           = p_audfrm->phyaddr[0];
	p_audio_frm->phy_addr[1]           = p_audfrm->phyaddr[1]; // [ToDo]
	p_audio_frm->size                  = p_audfrm->size;
}

HD_RESULT hd_audiodec_init(VOID)
{
	UINT32 dev, port;
	UINT32 i;

	hdal_flow_log_p("hd_audiodec_init\n");

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

	if (audiodec_info[0].port != NULL) {
		DBG_ERR("already init\n");
		return HD_ERR_UNINIT;
	}

	if (_max_path_count == 0) {
		DBG_ERR("_max_path_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	audiodec_info[0].port = (AUDDEC_INFO_PORT*)malloc(sizeof(AUDDEC_INFO_PORT)*_max_path_count);
	if (audiodec_info[0].port == NULL) {
		DBG_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
		return HD_ERR_HEAP;
	}
	for(i = 0; i < _max_path_count; i++) {
		memset(&(audiodec_info[0].port[i]), 0, sizeof(AUDDEC_INFO_PORT));  // no matter what status now...reset status = [UNINIT] anyway
	}

	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			// set status
			audiodec_info[dev].port[port].priv.status = HD_AUDIODEC_STATUS_INIT;  // [UNINT] -> [INIT]
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
			free(audiodec_info[0].port);
			audiodec_info[0].port = NULL;
			DBG_ERR("init fail? %d\r\n", r);
			return r;
		}
	}

	return HD_OK;
}

HD_RESULT hd_audiodec_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiodec_bind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) dst(0x%x) in(%d)\n", self_id, out_id, dest_id, in_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiodec_info[0].port == NULL) {
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

HD_RESULT hd_audiodec_unbind(HD_OUT_ID _out_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiodec_unbind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d)\n", self_id, out_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiodec_info[0].port == NULL) {
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

HD_RESULT _hd_audiodec_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
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


HD_RESULT _hd_audiodec_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id)
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

HD_RESULT _hd_audiodec_get_state(HD_OUT_ID _out_id, UINT32* p_state)
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

HD_RESULT hd_audiodec_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiodec_open:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) in(%d) ", self_id, out_id, in_id);

	if (dev_fd <= 0) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiodec_info[0].port == NULL) {
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
		p_path_id[0] = HD_AUDIODEC_PATH(cur_dev, in_id, out_id);
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
		audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_AUDIODEC_STATUS_OPEN;
	}

	return rv;
}

HD_RESULT hd_audiodec_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiodec_start:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiodec_info[0].port == NULL) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		//--- check if MAX_MEM info had been set ---
		if (audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set == FALSE) {
			DBG_ERR("Error: please set HD_AUDIODEC_PARAM_MAXMEM_CONFIG first !!!\r\n");
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
		audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_AUDIODEC_STATUS_START;
	}

	{
		// get physical addr/size of decoded frame buffer
		HD_BUFINFO *p_bufinfo = &audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_buf_info.buf_info;
		_hd_ad_get_param(self_id, out_id, AUDDEC_PARAM_BUFINFO_PHYADDR, &p_bufinfo->phy_addr);
		_hd_ad_get_param(self_id, out_id, AUDDEC_PARAM_BUFINFO_SIZE,    &p_bufinfo->buf_size);
		//DBG_DUMP("[start] decoded frame buffer: phy_addr=0x%lx, buf_size=0x%lx\r\n", p_bufinfo->phy_addr, p_bufinfo->buf_size);
	}

	return rv;
}

HD_RESULT hd_audiodec_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiodec_stop:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiodec_info[0].port == NULL) {
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
		audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_AUDIODEC_STATUS_STOP;
	}

	return rv;
}

HD_RESULT hd_audiodec_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audiodec_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audiodec_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiodec_close:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiodec_info[0].port == NULL) {
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
			NMI_AUDDEC_MAX_MEM_INFO max_mem = {0};

			max_mem.bRelease = TRUE;

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = AUDDEC_PARAM_MAX_MEM_INFO;
			cmd.value = (UINT32)&max_mem;
			cmd.size = sizeof(NMI_AUDDEC_MAX_MEM_INFO);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r == 0) {
				audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = FALSE;
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

		// reset HD_AUDIODEC_PARAM_IN:
		{
			ISF_SET set[4] = {0};
			HD_AUDIODEC_IN* p_dec_in_param = &audiodec_info[self_id-DEV_BASE].port[in_id-IN_BASE].dec_in_param;

			p_dec_in_param->codec_type  = 0;
			p_dec_in_param->sample_rate = 0;
			p_dec_in_param->mode        = 0;
			p_dec_in_param->sample_bit  = 0;
			hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u)\n", p_dec_in_param->codec_type, p_dec_in_param->sample_rate, p_dec_in_param->mode, p_dec_in_param->sample_bit);

			cmd.dest = ISF_PORT(self_id, in_id);

			set[0].param = AUDDEC_PARAM_CODEC;           set[0].value = p_dec_in_param->codec_type;
			set[1].param = AUDDEC_PARAM_SAMPLERATE;      set[1].value = p_dec_in_param->sample_rate;
			set[2].param = AUDDEC_PARAM_CHANNELS;        set[2].value = p_dec_in_param->mode;
			set[3].param = AUDDEC_PARAM_BITS;            set[3].value = p_dec_in_param->sample_bit;

			cmd.param = ISF_UNIT_PARAM_MULTI;
			cmd.value = (UINT32)set;
			cmd.size = sizeof(ISF_SET)*4;
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r != 0) {
				DBG_ERR("reset HD_AUDIODEC_PARAM_IN fail\r\n");
			}
		}
	}

	if (rv == HD_OK) {
		audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status = HD_AUDIODEC_STATUS_CLOSE;
	}

	return rv;
}

INT _hd_audiodec_param_cvt_name(HD_AUDIODEC_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
		case HD_AUDIODEC_PARAM_DEVCOUNT:            snprintf(p_ret_string, max_str_len, "DEVCOUNT");           break;
		case HD_AUDIODEC_PARAM_SYSCAPS:             snprintf(p_ret_string, max_str_len, "SYSCAPS");            break;
		case HD_AUDIODEC_PARAM_PATH_CONFIG:         snprintf(p_ret_string, max_str_len, "PATH_CONFIG");        break;
		case HD_AUDIODEC_PARAM_BUFINFO:             snprintf(p_ret_string, max_str_len, "BUFINFO");            break;
		case HD_AUDIODEC_PARAM_IN:                  snprintf(p_ret_string, max_str_len, "IN");                 break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", id);
			return (-1);
	}
	return 0;
}

HD_RESULT hd_audiodec_get(HD_PATH_ID path_id, HD_AUDIODEC_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	//HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	//int r;

	{
		CHAR  param_name[20];
		_hd_audiodec_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_audiodec_get(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() > 0) && (id != HD_AUDIODEC_PARAM_BUFINFO)){
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if ((_hd_common_get_pid() == 0) &&(audiodec_info[0].port == NULL)) {
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

			case HD_AUDIODEC_PARAM_DEVCOUNT:
			{
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
				p_user->max_dev_count = 1;

				hdal_flow_log_p("max_dev_cnt(%u)\n", p_user->max_dev_count);

				rv = HD_OK;
			}
			break;

			case HD_AUDIODEC_PARAM_SYSCAPS:
			{
				HD_AUDIODEC_SYSCAPS* p_user = (HD_AUDIODEC_SYSCAPS*)p_param;
				int  i;
				memset(p_user, 0, sizeof(HD_AUDIODEC_SYSCAPS));
				p_user->dev_id        = self_id - HD_DEV_BASE + DEV_BASE;
				p_user->chip_id       = 0;
				p_user->max_in_count  = 16;
				p_user->max_out_count = 16;
				p_user->dev_caps      = HD_CAPS_PATHCONFIG;
				for (i=0; i<16; i++) p_user->in_caps[i] =
					HD_AUDIODEC_CAPS_PCM
					 | HD_AUDIODEC_CAPS_AAC
					 | HD_AUDIODEC_CAPS_ADPCM
					 | HD_AUDIODEC_CAPS_ULAW
					 | HD_AUDIODEC_CAPS_ALAW;

				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip_id, p_user->max_in_count, p_user->max_out_count, p_user->dev_caps);
				{
					int i;
					hdal_flow_log_p("         in_caps =>\n");
					for (i=0; i<16; i++) {
						hdal_flow_log_p("    [%02d] 0x%08x\n", i, p_user->in_caps[i]);
					}
				}

				rv = HD_OK;
			}
			break;

			default: rv = HD_ERR_PARAM; break;
			}
		} else {
			switch(id) {
			case HD_AUDIODEC_PARAM_IN:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id , rv);	if(rv != HD_OK) {	return rv;}

					HD_AUDIODEC_IN *p_user = (HD_AUDIODEC_IN*)p_param;
					HD_AUDIODEC_IN *p_bufinfo = &audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_in_param;
					memcpy(p_user, p_bufinfo, sizeof(HD_AUDIODEC_IN));

					hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u)\n", p_user->codec_type, p_user->sample_rate, p_user->mode, p_user->sample_bit);
				}
				break;
			case HD_AUDIODEC_PARAM_PATH_CONFIG:
				{
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					HD_AUDIODEC_PATH_CONFIG *p_user = (HD_AUDIODEC_PATH_CONFIG*)p_param;
					HD_AUDIODEC_PATH_CONFIG *p_bufinfo = &audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_path_cfg;
					memcpy(p_user, p_bufinfo, sizeof(HD_AUDIODEC_PATH_CONFIG));

					hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u)\n", p_user->max_mem.codec_type, p_user->max_mem.sample_rate, p_user->max_mem.mode, p_user->max_mem.sample_bit);
				}
				break;
			case HD_AUDIODEC_PARAM_BUFINFO:
				{
					HD_AUDIODEC_BUFINFO *p_user = (HD_AUDIODEC_BUFINFO*)p_param;

					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
					_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

					if (_hd_common_get_pid() == 0) { // main process
						HD_AUDIODEC_BUFINFO *p_bufinfo = &audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_buf_info;

						//check OPEN status first
						if (audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status != HD_AUDIODEC_STATUS_START) {
							DBG_ERR("HD_AUDIODEC_PARAM_BUFINFO could only be get after START\r\n");
							return HD_ERR_NOT_START;
						}

						memcpy(p_user, p_bufinfo, sizeof(HD_AUDIODEC_BUFINFO));
					} else { // client process
						// query kernel to get physical addr/size of bs buffer
						HD_BUFINFO bufinfo = {0};

						_hd_ad_get_param(self_id, out_id, AUDDEC_PARAM_BUFINFO_PHYADDR, &bufinfo.phy_addr);
						_hd_ad_get_param(self_id, out_id, AUDDEC_PARAM_BUFINFO_SIZE,    &bufinfo.buf_size);
						memcpy(&p_user->buf_info, &bufinfo, sizeof(HD_BUFINFO));
					}
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

HD_RESULT hd_audiodec_set(HD_PATH_ID path_id, HD_AUDIODEC_PARAM_ID id, VOID* p_param)
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
		_hd_audiodec_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_audiodec_set(%s):\n", param_name);
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

	if (audiodec_info[0].port == NULL) {
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
			case HD_AUDIODEC_PARAM_PATH_CONFIG:
			{
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				HD_AUDIODEC_PATH_CONFIG* p_user = (HD_AUDIODEC_PATH_CONFIG*)p_param;
				HD_AUDIODEC_PATH_CONFIG *p_dec_path_cfg = &audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].dec_path_cfg;
				memcpy(p_dec_path_cfg, p_user, sizeof(HD_AUDIODEC_PATH_CONFIG));

				hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u)\n", p_user->max_mem.codec_type, p_user->max_mem.sample_rate, p_user->max_mem.mode, p_user->max_mem.sample_bit);

				// max_mem
				NMI_AUDDEC_MAX_MEM_INFO max_mem;

				max_mem.uiAudCodec = _hd_audiodec_codec_hdal2unit(p_user->max_mem.codec_type);
				max_mem.uiAudChannels = (p_user->max_mem.mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				max_mem.uiAudioSR = p_user->max_mem.sample_rate;
				max_mem.uiAudioBits = p_user->max_mem.sample_bit;
				max_mem.bRelease = FALSE;

				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = AUDDEC_PARAM_MAX_MEM_INFO;
				cmd.value = (UINT32)&max_mem;
				cmd.size = sizeof(NMI_AUDDEC_MAX_MEM_INFO);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r == 0) {
					audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.b_maxmem_set = TRUE;
				} else {
					DBG_ERR("Set MAX memory FAIL ... !!\r\n");
					goto _HD_AD1;
				}

				// codec (in-port)
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

				if (audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status == HD_AUDIODEC_STATUS_START) {
					DBG_WRN("update codec_type fail, please STOP first!!\r\n");
					return HD_ERR_NOT_ALLOW;
				}

				cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = AUDDEC_PARAM_CODEC;
				cmd.value = _hd_audiodec_codec_hdal2unit(p_user->max_mem.codec_type);
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("set path_config codec fail ... !!\r\n");
					goto _HD_AD1;
				}

				goto _HD_AD1;
			}
			break;

			case HD_AUDIODEC_PARAM_IN:
			{
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				ISF_SET set[5] = {0};
				HD_AUDIODEC_IN* p_user = (HD_AUDIODEC_IN*)p_param;
				HD_AUDIODEC_IN* p_dec_in_param = &audiodec_info[self_id-DEV_BASE].port[in_id-IN_BASE].dec_in_param;

				if ((p_user->codec_type != p_dec_in_param->codec_type) &&
					(audiodec_info[self_id-DEV_BASE].port[out_id-OUT_BASE].priv.status == HD_AUDIODEC_STATUS_START)) {
					DBG_WRN("update codec_type fail, please STOP first!!\r\n");
					return HD_ERR_NOT_ALLOW;
				}

				memcpy(p_dec_in_param, p_user, sizeof(HD_AUDIODEC_IN));

				hdal_flow_log_p("codec(%u) sr(%u) ch(%u) bit(%u)\n", p_user->codec_type, p_user->sample_rate, p_user->mode, p_user->sample_bit);

				cmd.dest = ISF_PORT(self_id, in_id);

				set[0].param = AUDDEC_PARAM_CODEC;           set[0].value = _hd_audiodec_codec_hdal2unit(p_user->codec_type);
				set[1].param = AUDDEC_PARAM_SAMPLERATE;      set[1].value = p_user->sample_rate;
				set[2].param = AUDDEC_PARAM_CHANNELS;        set[2].value = (p_user->mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				set[3].param = AUDDEC_PARAM_BITS;            set[3].value = p_user->sample_bit;
				set[4].param = AUDDEC_PARAM_ADTS_EN;         set[4].value = TRUE;

				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*5;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("param_in fail (CHK %d)\r\n", __LINE__);
					goto _HD_AD1;
				}

				goto _HD_AD1;
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
_HD_AD1:
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

void _hd_audiodec_dewrap_bs(ISF_DATA *p_data, HD_AUDIO_BS* p_audio_bs)
{
	AUD_BITSTREAM * p_audbs = (AUD_BITSTREAM *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_audio_bs->blk;
	p_data->size = p_audio_bs->size; // it could be the whole isf_data, but we only have one data bs_size

	// set aud_bs info
	p_audbs->sign      = p_audio_bs->sign;
	p_audbs->DataAddr  = p_audio_bs->phy_addr;
	p_audbs->DataSize  = p_audio_bs->size;
	p_audbs->CodecType = p_audio_bs->acodec_format;
	p_audbs->timestamp = p_audio_bs->timestamp;
}

HD_RESULT hd_audiodec_push_in_buf(HD_PATH_ID path_id, HD_AUDIO_BS* p_in_audio_bs, HD_AUDIO_FRAME* p_user_out_audio_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (audiodec_info[0].port == NULL)) {
		return HD_ERR_UNINIT;
	}

	if (p_in_audio_bs == NULL) {
		DBG_ERR("p_in_audio_bs is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_audiodec_dewrap_bs(&data, p_in_audio_bs);

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
	}

	return rv;
}

void _hd_audiodec_rebuild_frame(ISF_DATA *p_data, HD_AUDIO_FRAME* p_audio_frame)
{
	AUD_FRAME * p_audframe = (AUD_FRAME *) p_data->desc;
	HD_DAL dev_id = p_audio_frame->blk >> 16;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	//p_data->h_data = p_audio_frame->blk; //isf-data-handle

	if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) {
		//check if dev_id is AUDIODEC
		dev_id -= HD_DEV_BASE;
		p_audframe->sign = MAKEFOURCC('A','D','E','C');
		p_data->h_data = (dev_id << 16) | (p_audio_frame->blk & 0xFFFF); // refer to jira: NA51000-1384
	} else if (p_audio_frame->blk == 0) {
		p_audframe->sign = MAKEFOURCC('A','F','R','M');
		p_data->h_data = 0;
	}
}

HD_RESULT hd_audiodec_pull_out_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME* p_audio_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (audiodec_info[0].port == NULL)) {
		return HD_ERR_UNINIT;
	}

	if (p_audio_frame == NULL) {
		DBG_ERR("p_audio_frame is NULL\r\n");
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
			_hd_audiodec_wrap_frm(p_audio_frame, &data);
		}
	}
	return rv;
}

HD_RESULT hd_audiodec_release_out_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME* p_audio_frame)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (audiodec_info[0].port == NULL)) {
		return HD_ERR_UNINIT;
	}

	if (p_audio_frame == NULL) {
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_audiodec_rebuild_frame(&data, p_audio_frame);

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

HD_RESULT hd_audiodec_send_list(HD_AUDIODEC_SEND_LIST *p_audio_bs, UINT32 num, INT32 wait_ms)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audiodec_uninit(VOID)
{
	HD_RESULT rv = HD_OK;
	HD_PATH_ID path_id = 0;
	UINT32 dev, port;

	hdal_flow_log_p("hd_audiodec_uninit\n");

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

	if (audiodec_info[0].port == NULL) {
		DBG_ERR("NOT init yet\n");
		return HD_ERR_INIT;
	}

	for (dev = 0; dev < DEV_COUNT; dev++) {
		for (port = 0; port < _max_path_count; port++) {
			if (audiodec_info[dev].port[port].priv.status == HD_AUDIODEC_STATUS_START) {
				path_id = HD_AUDIODEC_PATH(HD_DAL_AUDIODEC(dev), HD_IN(port), HD_OUT(port));
				DBG_WRN("ERROR: path(%lu) is NOT stop/close yet....auto-call hd_audiodec_stop() and hd_audiodec_close() before attemp to uninit !!!\r\n", port);
				if (HD_OK != hd_audiodec_stop(path_id)) {
					DBG_ERR("auto-call hd_audiodec_stop(path%lu) error !!!\r\n", port);
					rv = HD_ERR_NG;
				}
				if (HD_OK != hd_audiodec_close(path_id)) {
					DBG_ERR("auto-call hd_audiodec_close(path%lu) error !!!\r\n", port);
					rv = HD_ERR_NG;
				}
			} else if (audiodec_info[dev].port[port].priv.status == HD_AUDIODEC_STATUS_OPEN) {
				path_id = HD_AUDIODEC_PATH(HD_DAL_AUDIODEC(dev), HD_IN(port), HD_OUT(port));
				DBG_WRN("ERROR: path(%lu) is NOT close yet....auto-call hd_audiodec_close() before attemp to uninit !!!\r\n", port);
				if (HD_OK != hd_audiodec_close(path_id)) {
					DBG_ERR("auto-call hd_audiodec_close(path%lu) error !!!\r\n", port);
					rv = HD_ERR_NG;
				}
			} else {
				// set status
				audiodec_info[dev].port[port].priv.status = HD_AUDIODEC_STATUS_UNINIT; // [INIT]/[UNINIT] -> [UNINT]
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
		free(audiodec_info[0].port);
		audiodec_info[0].port = NULL;
	}

	return rv;
}

int _hd_audiodec_is_init(VOID)
{
	return (audiodec_info[0].port != NULL);
}