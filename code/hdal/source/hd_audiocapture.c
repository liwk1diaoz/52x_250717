/**
    @brief Source code of audio capture module.\n
    This file contains the functions which is related to audio capture in the chip.

    @file hd_audiocapture.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "hdal.h"
#define HD_MODULE_NAME HD_AUDIOCAP
#include "hd_int.h"
#include "kflow_audiocapture/isf_audcap.h"
#include <string.h>
#include "hd_logger_p.h"

#define HD_AUDIOCAP_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE        ISF_UNIT_AUDCAP
#define DEV_COUNT       ISF_MAX_AUDCAP
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        1
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       2

#define HD_DEV_BASE HD_DAL_AUDIOCAP_BASE
#define HD_DEV_MAX  HD_DAL_AUDIOCAP_MAX

#define HD_AUDIOCAP_MAX_DEVCOUNT   1
#define HD_AUDIOCAP_MAX_IN_COUNT   IN_COUNT
#define HD_AUDIOCAP_MAX_OUT_COUNT  OUT_COUNT



typedef struct _AUDCAP_INFO {
	struct {
		HD_AUDIOCAP_BUFINFO     cap_buf_info;
	} port[HD_AUDIOCAP_MAX_OUT_COUNT];

	HD_AUDIOCAP_DEV_CONFIG      dev_config;
	HD_AUDIOCAP_DRV_CONFIG      drv_config;
	HD_AUDIOCAP_IN              in[HD_AUDIOCAP_MAX_IN_COUNT];
	HD_AUDIOCAP_OUT             out[HD_AUDIOCAP_MAX_OUT_COUNT];
	HD_AUDIOCAP_AEC             aec[HD_AUDIOCAP_MAX_OUT_COUNT];
	HD_AUDIOCAP_ANR             anr[HD_AUDIOCAP_MAX_OUT_COUNT];
	HD_AUDIOCAP_VOLUME          volume[HD_AUDIOCAP_MAX_OUT_COUNT];
	BOOL                        b_dev_cfg_set;
	BOOL                        b_in_cfg_set;
} AUDCAP_INFO;

static UINT32 _max_path_count = 0;

#define TIMESTAMP_TO_MS(ts)  ((ts >> 32) * 1000 + (ts & 0xFFFFFFFF) / 1000)

#define _HD_CONVERT_SELF_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_DEV;  \
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
		} else if(((dev_id) >= HD_DAL_AUDIOENC_BASE) && ((dev_id) <= HD_DAL_AUDIOENC_MAX)) { \
			(rv) = _hd_audioenc_convert_dev_id(&(dev_id)); \
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
			if(id < IN_COUNT) { \
				(in_id) = IN_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_REVERT_BIND_ID(dev_id) \
	do { \
		if(((dev_id) >= ISF_UNIT_AUDENC) && ((dev_id) < ISF_UNIT_AUDENC + ISF_MAX_AUDENC)) { \
			(dev_id) = HD_DAL_AUDIOENC_BASE + (dev_id) - ISF_UNIT_AUDENC; \
		} else if(((dev_id) >= ISF_UNIT_AUDOUT) && ((dev_id) < ISF_UNIT_AUDOUT + ISF_MAX_AUDOUT)) { \
			(dev_id) = HD_DAL_AUDIOOUT_BASE + (dev_id) - ISF_UNIT_AUDOUT; \
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

//AUDCAP_INFO audiocap_info[DEV_COUNT] = {0};
static AUDCAP_INFO* audiocap_info = NULL;

HD_RESULT _hd_audiocap_convert_dev_id(HD_DAL *p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

int _hd_audiocap_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "AUDIOCAP_%d_OUT_%d",  dev_id - HD_DEV_BASE, out_id - HD_OUT_BASE);
}

void _hd_audiocap_wrap_frame(HD_AUDIO_FRAME* p_audio_frame, ISF_DATA *p_data)
{
	PAUD_FRAME p_audframe = (AUD_FRAME *) p_data->desc;
	HD_DAL id = (p_data->h_data >> 16) + HD_DEV_BASE;

	p_audio_frame->sign                  = MAKEFOURCC('A','F','R','M');
	p_audio_frame->blk                   = (id << 16) | (p_data->h_data & 0xFFFF);
	p_audio_frame->timestamp             = p_audframe->timestamp;
	p_audio_frame->phy_addr[0]           = p_audframe->phyaddr[0];
	p_audio_frame->phy_addr[1]           = p_audframe->phyaddr[1];
	p_audio_frame->size                  = p_audframe->size;
	p_audio_frame->sample_rate           = p_audframe->sample_rate;
	p_audio_frame->sound_mode            = (p_audframe->sound_mode == 1)? HD_AUDIO_SOUND_MODE_MONO : HD_AUDIO_SOUND_MODE_STEREO;
	p_audio_frame->bit_width             = p_audframe->bit_width;
}

void _hd_audiocap_rebuild_frame(ISF_DATA *p_data, HD_AUDIO_FRAME* p_audio_frame)
{
	AUD_FRAME * p_audframe = (AUD_FRAME *) p_data->desc;
	HD_DAL dev_id = p_audio_frame->blk >> 16;

	p_data->sign    = MAKEFOURCC('I','S','F','D');
	//p_data->h_data  = p_audio_frame->blk; //isf-data-handle

	if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) {
		//check if dev_id is AUDIOCAP
		dev_id -= HD_DEV_BASE;
		p_audframe->sign = MAKEFOURCC('A','C','A','P');
		p_data->h_data = (dev_id << 16) | (p_audio_frame->blk & 0xFFFF);
	} else if (p_audio_frame->blk == 0) {
		p_audframe->sign = MAKEFOURCC('A','F','R','M');
		p_data->h_data = 0;
	}
}

HD_RESULT _hd_audiocap_get_param(HD_DAL self_id, HD_IO out_id, UINT32 param, UINT32 *value)
{
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_RESULT rv;
	int r;

	if (out_id == HD_CTRL) {
		cmd.dest = ISF_PORT(self_id, ISF_CTRL);
	} else {
		cmd.dest = ISF_PORT(self_id, out_id);
	}
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
			HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

void _hd_audiocap_cfg_max(UINT32 maxpath)
{
	if (!audiocap_info) {
		_max_path_count = maxpath;
		if (_max_path_count > HD_AUDIOCAP_MAX_OUT_COUNT) {
			HD_AUDIOCAPTURE_WRN("dts path=%lu is larger than built-in max_path=%lu!\r\n", _max_path_count, (UINT32)HD_AUDIOCAP_MAX_OUT_COUNT);
			_max_path_count = HD_AUDIOCAP_MAX_OUT_COUNT;
		}
	}
}

HD_RESULT hd_audiocap_init(VOID)
{
	UINT32 i, j;

	hdal_flow_log_p("hd_audiocap_init\n");

	/*for(i = 0; i < DEV_COUNT; i++) {
		memset(&audiocap_info[i], 0, sizeof(AUDCAP_INFO));
		audiocap_info[i].volume.volume = 100;
	}*/

	if (_hd_common_get_pid() > 0) {
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = 0;
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			HD_AUDIOCAPTURE_ERR("init fail? %d\r\n", r);
			return r;
		}
		return HD_OK;
	}

	if (dev_fd <= 0) {
		return HD_ERR_INIT;
	}

	if (audiocap_info != NULL) {
		return HD_ERR_UNINIT;
	}

	if (_max_path_count == 0) {
		HD_AUDIOCAPTURE_ERR("_max_path_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	audiocap_info = (AUDCAP_INFO*)malloc(sizeof(AUDCAP_INFO)*_max_path_count);
	if (audiocap_info == NULL) {
		HD_AUDIOCAPTURE_ERR("cannot alloc heap for _max_path_count =%d\r\n", (int)_max_path_count);
		return HD_ERR_HEAP;
	}
	for(i = 0; i < _max_path_count; i++) {
		memset(&(audiocap_info[i]), 0, sizeof(AUDCAP_INFO));
		for(j = 0; j < HD_AUDIOCAP_MAX_OUT_COUNT; j++) {
			audiocap_info[i].volume[j].volume = 100;
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
			free(audiocap_info);
			audiocap_info = NULL;
			HD_AUDIOCAPTURE_ERR("init fail? %d\r\n", r);
			return r;
		}
	}

	return HD_OK;
}

HD_RESULT hd_audiocap_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiocap_bind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) dst(0x%x) in(%d)\n", self_id, out_id, dest_id, in_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		HD_AUDIOCAPTURE_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiocap_info == NULL) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_BIND_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		_HD_CONVERT_OUT_ID(out_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		cmd.src = ISF_PORT(self_id, out_id);
		_HD_CONVERT_DEST_ID(dest_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		_HD_CONVERT_IN_ID(in_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_audiocap_unbind(HD_OUT_ID _out_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiocap_unbind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d)\n", self_id, out_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		HD_AUDIOCAPTURE_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiocap_info == NULL) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_BIND_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		_HD_CONVERT_OUT_ID(out_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT _hd_audiocap_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}


HD_RESULT _hd_audiocap_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id)
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT _hd_audiocap_get_state(HD_OUT_ID _out_id, UINT32* p_state)
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_audiocap_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiocap_open:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) in(%d) ", self_id, out_id, in_id);

	if (dev_fd <= 0) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		HD_AUDIOCAPTURE_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiocap_info == NULL) {
		return HD_ERR_UNINIT;
	}

	if(!p_path_id) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_NULL_PTR;
	}

	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		_HD_CONVERT_SELF_ID(self_id, rv);
		if(rv != HD_OK) {
			HD_AUDIOCAPTURE_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)HD_AUDIOCAP_MAX_DEVCOUNT);
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
			HD_AUDIOCAPTURE_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)HD_AUDIOCAP_MAX_DEVCOUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_OUT_ID(check_out_id, rv);
		if(rv != HD_OK) {
			HD_AUDIOCAPTURE_ERR("out_id=%lu is larger than max=%lu!\r\n", (UINT32)((check_out_id) - HD_OUT_BASE), (UINT32)_max_path_count);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_SELF_ID(in_dev_id, rv);
		if(rv != HD_OK) {
			HD_AUDIOCAPTURE_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((in_dev_id) - HD_DEV_BASE), (UINT32)HD_AUDIOCAP_MAX_DEVCOUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_IN_ID(check_in_id, rv);
		if(rv != HD_OK) {
			HD_AUDIOCAPTURE_ERR("in_id=%lu is larger than max=%lu!\r\n", (UINT32)((check_in_id) - HD_IN_BASE), (UINT32)HD_AUDIOCAP_MAX_IN_COUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		if(in_dev_id != self_id) {
			HD_AUDIOCAPTURE_ERR("dev_id of in/out is not the same!\r\n");
			hdal_flow_log_p("path_id(N/A)\n");
			return HD_ERR_DEV;
		}
		p_path_id[0] = HD_AUDIOCAP_PATH(cur_dev, in_id, out_id);
		hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);
	}

	//--- check if DEV_CONFIG info had been set ---
	{
		UINT32 did = self_id - ISF_UNIT_AUDCAP;
		if (audiocap_info[did].b_dev_cfg_set == FALSE) {
			HD_AUDIOCAPTURE_ERR("Error: please set HD_AUDIOCAP_PARAM_DEV_CONFIG first !!!\r\n");
			return HD_ERR_NO_CONFIG;
		}
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_OUT_ID(out_id, rv);
		if (rv != HD_OK) {
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch (cmd.rv) {
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}

	// get physical addr/size of bs buffer
	HD_BUFINFO *p_bufinfo = &audiocap_info[0].port[0].cap_buf_info.buf_info;
	_hd_audiocap_get_param(self_id, HD_CTRL, AUDCAP_PARAM_BUFINFO_PHYADDR, &p_bufinfo->phy_addr);
	_hd_audiocap_get_param(self_id, HD_CTRL, AUDCAP_PARAM_BUFINFO_SIZE,    &p_bufinfo->buf_size);

	return rv;
}

HD_RESULT hd_audiocap_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiocap_start:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		HD_AUDIOCAPTURE_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiocap_info == NULL) {
		return HD_ERR_UNINIT;
	}

	_HD_CONVERT_SELF_ID(self_id, rv);
	if (rv != HD_OK) {
		return rv;
	}

	//--- check if IN info had been set ---
	{
		UINT32 did = self_id - ISF_UNIT_AUDCAP;
		if (audiocap_info[did].b_in_cfg_set == FALSE) {
			HD_AUDIOCAPTURE_ERR("Error: please set HD_AUDIOCAP_PARAM_IN first !!!\r\n");
			return HD_ERR_NO_CONFIG;
		}
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};

		_HD_CONVERT_OUT_ID(out_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 1;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch (cmd.rv) {
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_audiocap_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiocap_stop:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		HD_AUDIOCAPTURE_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiocap_info == NULL) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		_HD_CONVERT_OUT_ID(out_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 2;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch (cmd.rv) {
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_audiocap_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audiocap_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audiocap_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audiocap_close:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		HD_AUDIOCAPTURE_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiocap_info == NULL) {
		return HD_ERR_UNINIT;
	}

	if(ctrl_id == HD_CTRL) {
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		//do nothing
		return HD_OK;
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		_HD_CONVERT_OUT_ID(out_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 3;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &cmd);
		if (r == 0) {
			switch (cmd.rv) {
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

static INT _hd_audiocap_param_cvt_name(HD_AUDIOCAP_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
		case HD_AUDIOCAP_PARAM_DEVCOUNT:    snprintf(p_ret_string, max_str_len, "DEVCOUNT");   break;
		case HD_AUDIOCAP_PARAM_SYSCAPS:     snprintf(p_ret_string, max_str_len, "SYSCAPS");    break;
		case HD_AUDIOCAP_PARAM_SYSINFO:     snprintf(p_ret_string, max_str_len, "SYSINFO");    break;
		case HD_AUDIOCAP_PARAM_DEV_CONFIG:  snprintf(p_ret_string, max_str_len, "DEV_CONFIG"); break;
		case HD_AUDIOCAP_PARAM_DRV_CONFIG:  snprintf(p_ret_string, max_str_len, "DRV_CONFIG"); break;
		case HD_AUDIOCAP_PARAM_IN:          snprintf(p_ret_string, max_str_len, "IN");         break;
		case HD_AUDIOCAP_PARAM_OUT:         snprintf(p_ret_string, max_str_len, "OUT");        break;
		case HD_AUDIOCAP_PARAM_OUT_AEC:     snprintf(p_ret_string, max_str_len, "OUT_AEC");    break;
		case HD_AUDIOCAP_PARAM_OUT_ANR:     snprintf(p_ret_string, max_str_len, "OUT_ANR");    break;
		case HD_AUDIOCAP_PARAM_VOLUME:      snprintf(p_ret_string, max_str_len, "VOLUME");     break;
		case HD_AUDIOCAP_PARAM_BUFINFO:     snprintf(p_ret_string, max_str_len, "BUFINFO");    break;

		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", id);
			return (-1);
	}
	return 0;
}

static BOOL _hd_audiocap_check_src_sr(UINT32 sr)
{
	BOOL ret;

	switch (sr) {
		case 0:
		case HD_AUDIO_SR_8000:
		case HD_AUDIO_SR_11025:
		case HD_AUDIO_SR_12000:
		case HD_AUDIO_SR_16000:
		case HD_AUDIO_SR_22050:
		case HD_AUDIO_SR_24000:
		case HD_AUDIO_SR_32000:
		case HD_AUDIO_SR_44100:
		case HD_AUDIO_SR_48000:
			ret= TRUE;
			break;
		default:
			ret= FALSE;
			break;
	}
	return ret;
}

static BOOL _hd_audiocap_check_aec(HD_AUDIOCAP_AEC* aec_param)
{
	BOOL ret;

	if ((aec_param->enabled != TRUE) && (aec_param->enabled != FALSE)) {
		ret = FALSE;
		return ret;
	}

	if (aec_param->enabled == FALSE) {
		return TRUE;
	}

	if ((aec_param->leak_estimate_enabled != TRUE) && (aec_param->leak_estimate_enabled != FALSE)) {
		ret = FALSE;
		return ret;
	}

	if ((aec_param->leak_estimate_value < 25) || (aec_param->leak_estimate_value > 99)) {
		ret = FALSE;
		return ret;
	}

	if ((aec_param->noise_cancel_level < -40) || (aec_param->noise_cancel_level > -3)) {
		ret = FALSE;
		return ret;
	}

	if ((aec_param->echo_cancel_level < -60) || (aec_param->echo_cancel_level > -30)) {
		ret = FALSE;
		return ret;
	}

	if (aec_param->filter_length == 0) {
		ret = FALSE;
		return ret;
	}

	if (aec_param->frame_size == 0) {
		ret = FALSE;
		return ret;
	}

	if ((aec_param->notch_radius < 0) || (aec_param->echo_cancel_level > 1000)) {
		ret = FALSE;
		return ret;
	}

	switch (aec_param->lb_channel) {
		case HD_AUDIOCAP_LB_CH_LEFT:
		case HD_AUDIOCAP_LB_CH_RIGHT:
		case HD_AUDIOCAP_LB_CH_STEREO:
		case HD_AUDIOCAP_LB_CH_0:
		case HD_AUDIOCAP_LB_CH_1:
		case HD_AUDIOCAP_LB_CH_2:
		case HD_AUDIOCAP_LB_CH_3:
		case HD_AUDIOCAP_LB_CH_4:
		case HD_AUDIOCAP_LB_CH_5:
		case HD_AUDIOCAP_LB_CH_6:
		case HD_AUDIOCAP_LB_CH_7:
			ret = TRUE;
			break;
		default:
			ret = FALSE;
			break;
	}

	return ret;
}

static BOOL _hd_audiocap_check_anr(HD_AUDIOCAP_ANR* anr_param)
{
	BOOL ret = TRUE;

	if ((anr_param->enabled != TRUE) && (anr_param->enabled != FALSE)) {
		ret = FALSE;
		return ret;
	}

	if (anr_param->enabled == FALSE) {
		return TRUE;
	}

	if ((anr_param->suppress_level < 3) || (anr_param->suppress_level > 35)) {
		ret = FALSE;
		return ret;
	}

	if (anr_param->hpf_cut_off_freq < 0) {
		ret = FALSE;
		return ret;
	}

	if ((anr_param->bias_sensitive < 1) || (anr_param->bias_sensitive > 9)) {
		ret = FALSE;
		return ret;
	}

	return ret;
}

HD_RESULT hd_audiocap_get(HD_PATH_ID path_id, HD_AUDIOCAP_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	{
		CHAR  param_name[20];
		_hd_audiocap_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_audiocap_get(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() > 0) && (id != HD_AUDIOCAP_PARAM_BUFINFO)){
		HD_AUDIOCAPTURE_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}


	if ((_hd_common_get_pid() == 0) && (audiocap_info == NULL)) {
		return HD_ERR_UNINIT;
	}

	if (p_param == 0) {
		return HD_ERR_NULL_PTR;
	}

	{
		_HD_CONVERT_SELF_ID(self_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		rv = HD_OK;

		if(ctrl_id == HD_CTRL) {
			switch(id) {
			case HD_AUDIOCAP_PARAM_BUFINFO:
			{
				HD_AUDIOCAP_BUFINFO *p_user = (HD_AUDIOCAP_BUFINFO*)p_param;

				if (_hd_common_get_pid() == 0) { // main process
					HD_AUDIOCAP_BUFINFO *p_bufinfo = &audiocap_info[0].port[0].cap_buf_info;
					memcpy(p_user, p_bufinfo, sizeof(HD_AUDIOCAP_BUFINFO));
				} else {
					HD_BUFINFO bufinfo = {0};

					_hd_audiocap_get_param(self_id, HD_CTRL, AUDCAP_PARAM_BUFINFO_PHYADDR, &bufinfo.phy_addr);
					_hd_audiocap_get_param(self_id, HD_CTRL, AUDCAP_PARAM_BUFINFO_SIZE,    &bufinfo.buf_size);
					memcpy(&p_user->buf_info, &bufinfo, sizeof(HD_BUFINFO));
				}

				hdal_flow_log_p("buf.addr(%x) buf.size(%x)\n", p_user->buf_info.phy_addr, p_user->buf_info.buf_size);
			}
			break;
			case HD_AUDIOCAP_PARAM_DEVCOUNT:
			{
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
				p_user->max_dev_count = HD_AUDIOCAP_MAX_DEVCOUNT;
				rv = HD_OK;
				hdal_flow_log_p("max_dev_cnt(%u)\n", p_user->max_dev_count);
			}
			break;
			case HD_AUDIOCAP_PARAM_SYSCAPS:
			{
				HD_AUDIOCAP_SYSCAPS *p_user = (HD_AUDIOCAP_SYSCAPS *)p_param;

				memset(p_user, 0, sizeof(HD_AUDIOCAP_SYSCAPS));
				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;
				p_user->chip_id = 0;
				p_user->max_in_count = HD_AUDIOCAP_MAX_IN_COUNT;
				p_user->max_out_count = HD_AUDIOCAP_MAX_OUT_COUNT;
				p_user->dev_caps =
					HD_CAPS_DEVCONFIG
					 | HD_CAPS_DRVCONFIG
					 | HD_AUDIOCAP_DEVCAPS_MIC
					 | HD_AUDIOCAP_DEVCAPS_DIFF_SR
					 | HD_AUDIOCAP_DEVCAPS_AEC
					 | HD_AUDIOCAP_DEVCAPS_ANR
					 | HD_AUDIOCAP_DEVCAPS_VOLUME;
				p_user->in_caps[0] =
					HD_AUDIO_CAPS_BITS_16
					 | HD_AUDIO_CAPS_CH_MONO
					 | HD_AUDIO_CAPS_CH_STEREO;
				p_user->out_caps[0] = HD_AUDIO_CAPS_RESAMPLE_DOWN;
				p_user->support_in_sr[0] =
					HD_AUDIOCAP_SRCAPS_8000
					 | HD_AUDIOCAP_SRCAPS_11025
					 | HD_AUDIOCAP_SRCAPS_12000
					 | HD_AUDIOCAP_SRCAPS_16000
					 | HD_AUDIOCAP_SRCAPS_22050
					 | HD_AUDIOCAP_SRCAPS_24000
					 | HD_AUDIOCAP_SRCAPS_32000
					 | HD_AUDIOCAP_SRCAPS_44100
					 | HD_AUDIOCAP_SRCAPS_48000;
				p_user->support_out_sr[0] =
					HD_AUDIOCAP_SRCAPS_8000
					 | HD_AUDIOCAP_SRCAPS_11025
					 | HD_AUDIOCAP_SRCAPS_12000
					 | HD_AUDIOCAP_SRCAPS_16000
					 | HD_AUDIOCAP_SRCAPS_22050
					 | HD_AUDIOCAP_SRCAPS_24000
					 | HD_AUDIOCAP_SRCAPS_32000
					 | HD_AUDIOCAP_SRCAPS_44100
					 | HD_AUDIOCAP_SRCAPS_48000;

				rv = HD_OK;

				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip_id, p_user->max_in_count,
																									p_user->max_out_count, p_user->dev_caps);
				hdal_flow_log_p("    in_caps(0x%08x) out_caps(0x%08x)\n", p_user->in_caps[0], p_user->out_caps[0]);
				hdal_flow_log_p("    support_in_sr(0x%08x) support_out_sr(0x%08x)\n", p_user->support_in_sr[0], p_user->support_out_sr[0]);
			}
			break;
			case HD_AUDIOCAP_PARAM_SYSINFO:
			{
				HD_AUDIOCAP_SYSINFO* p_user = (HD_AUDIOCAP_SYSINFO*)p_param;
				AUDCAP_SYSINFO sys_info;
				UINT8 i;

				memset(p_user, 0, sizeof(HD_AUDIOCAP_SYSINFO));
				memset(&sys_info, 0, sizeof(sys_info));
				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;
				rv = HD_OK;

				ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = AUDCAP_PARAM_SYS_INFO;
				cmd.size = sizeof(AUDCAP_SYSINFO);
				cmd.value = (UINT32)&sys_info;

				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);

				if (r == 0) {
					switch(cmd.rv) {
					case ISF_OK:
						rv = HD_OK;
						p_user->cur_in_sample_rate = sys_info.cur_in_sample_rate;
						p_user->cur_mode = sys_info.cur_mode;
						p_user->cur_sample_bit = sys_info.cur_sample_bit;
						for (i = 0; i < HD_AUDIOCAP_MAX_OUT; i++) {
							p_user->cur_out_sample_rate[i] = sys_info.cur_out_sample_rate[i];
						}
						hdal_flow_log_p("cur_in_sr(%u) cur_mode(%u) cur_sample_bit(%u) cur_out_sr[0](%u)\n", p_user->cur_in_sample_rate, p_user->cur_mode, p_user->cur_sample_bit, p_user->cur_out_sample_rate[0]);
						break;
					default:
						rv = HD_ERR_SYS;
						break;
					}
				} else {
					if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
						rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
					} else {
						HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
						rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
					}
				}
			}
			break;
			case HD_AUDIOCAP_PARAM_DEV_CONFIG:
			{
				HD_AUDIOCAP_DEV_CONFIG* p_user = (HD_AUDIOCAP_DEV_CONFIG*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDCAP;
				p_user[0] = audiocap_info[did].dev_config;

				hdal_flow_log_p("in_max.sr(%u) in_max.sample_bit(%u) in_max.mode(%u) in_max.frame_sample(%u) max_frame_num(%u) out_max.sr(%u)\n",
					p_user->in_max.sample_rate, p_user->in_max.sample_bit, p_user->in_max.mode, p_user->in_max.frame_sample, p_user->frame_num_max, p_user->out_max.sample_rate);
				hdal_flow_log_p("    aec_max.en(%u) aec_max.leak_est_en(%u) aec_max.leak_est(%u) aec_max.noise_lvl(%u) aec_max.echo_lvl(%u) aec_max.filter_len(%u) aec_max.frm_size(%u) aec_max.notch_radius(%u)\n",
					p_user->aec_max.enabled, p_user->aec_max.leak_estimate_enabled, p_user->aec_max.leak_estimate_value, p_user->aec_max.noise_cancel_level, p_user->aec_max.echo_cancel_level,
					p_user->aec_max.filter_length, p_user->aec_max.frame_size, p_user->aec_max.notch_radius);
				hdal_flow_log_p("    anr_max.en(%u) anr_max.suppress_level(%u) anr_max.hpf_freq(%u) anr_max.bias_sensitive(%u)\n",
					p_user->anr_max.enabled, p_user->anr_max.suppress_level, p_user->anr_max.hpf_cut_off_freq, p_user->anr_max.bias_sensitive);

				rv = HD_OK;
			}
			break;
			case HD_AUDIOCAP_PARAM_DRV_CONFIG:
			{
				HD_AUDIOCAP_DRV_CONFIG* p_user = (HD_AUDIOCAP_DRV_CONFIG*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDCAP;
				p_user[0] = audiocap_info[did].drv_config;
				hdal_flow_log_p("mono(%u)\n", p_user->mono);
				rv = HD_OK;
			}
			break;
			case HD_AUDIOCAP_PARAM_VOLUME:
			{
				HD_AUDIOCAP_VOLUME* p_user = (HD_AUDIOCAP_VOLUME*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDCAP;
				p_user[0] = audiocap_info[did].volume[0];
				hdal_flow_log_p("vol(%u)\n", p_user->volume);
				rv = HD_OK;
			}
			break;
			default:
				rv = HD_ERR_PARAM;
				break;
			}
		} else {
			switch(id) {
			case HD_AUDIOCAP_PARAM_IN:
			if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
			_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}
			{
				HD_AUDIOCAP_IN* p_user = (HD_AUDIOCAP_IN*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDCAP;
				UINT32 iid = in_id - ISF_IN_BASE;
				p_user[0] = audiocap_info[did].in[iid];
				hdal_flow_log_p("in.sr(%u) in.sample_bit(%u) in.mode(%u) in.frame_sample(%u)\n",
					p_user->sample_rate, p_user->sample_bit, p_user->mode, p_user->frame_sample);
				rv = HD_OK;
			}
			break;
			case HD_AUDIOCAP_PARAM_OUT:
			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			{
				HD_AUDIOCAP_OUT* p_user = (HD_AUDIOCAP_OUT*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDCAP;
				UINT32 oid = out_id - ISF_OUT_BASE;
				p_user[0] = audiocap_info[did].out[oid];
				hdal_flow_log_p("out.sr(%u)\n", p_user->sample_rate);
				rv = HD_OK;
			}
			break;
			case HD_AUDIOCAP_PARAM_OUT_AEC:
			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			{
				HD_AUDIOCAP_AEC* p_user = (HD_AUDIOCAP_AEC*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDCAP;
				UINT32 oid = out_id - ISF_OUT_BASE;
				p_user[0] = audiocap_info[did].aec[oid];
				hdal_flow_log_p("aec.en(%u) aec.leak_est_en(%u) aec.leak_est(%u) aec.noise_lvl(%u) aec.echo_lvl(%u) aec.filter_len(%u) aec.frm_size(%u) aec.notch_radius(%u)\n",
					p_user->enabled, p_user->leak_estimate_enabled, p_user->leak_estimate_value, p_user->noise_cancel_level, p_user->echo_cancel_level,
					p_user->filter_length, p_user->frame_size, p_user->notch_radius);

				rv = HD_OK;
			}
			break;
			case HD_AUDIOCAP_PARAM_OUT_ANR:
			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			{
				HD_AUDIOCAP_ANR* p_user = (HD_AUDIOCAP_ANR*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDCAP;
				UINT32 oid = out_id - ISF_OUT_BASE;
				p_user[0] = audiocap_info[did].anr[oid];
				hdal_flow_log_p("anr.en(%u) anr.suppress_level(%u) anr.hpf_freq(%u) anr.bias_sensitive(%u)\n",
					p_user->enabled, p_user->suppress_level, p_user->hpf_cut_off_freq, p_user->bias_sensitive);
				rv = HD_OK;
			}
			break;
			case HD_AUDIOCAP_PARAM_VOLUME:
			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			{
				HD_AUDIOCAP_VOLUME* p_user = (HD_AUDIOCAP_VOLUME*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDCAP;
				UINT32 oid = out_id - ISF_OUT_BASE;
				p_user[0] = audiocap_info[did].volume[oid];
				hdal_flow_log_p("vol(%u)\n", p_user->volume);
				rv = HD_OK;
			}
			break;
			default:
				rv = HD_ERR_PARAM;
				break;
			}
		}
	}
	return rv;
}

HD_RESULT hd_audiocap_set(HD_PATH_ID path_id, HD_AUDIOCAP_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	int r;

	{
		CHAR  param_name[20];
		_hd_audiocap_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_audiocap_set(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		HD_AUDIOCAPTURE_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audiocap_info == NULL) {
		return HD_ERR_UNINIT;
	}

	{
		_HD_CONVERT_SELF_ID(self_id, rv);
		if (rv != HD_OK) {
			return rv;
		}
		rv = HD_OK;
		//cmd.param = id;
		if (p_param == NULL) {
			return HD_ERR_NULL_PTR;
		}
		if (ctrl_id == HD_CTRL) {
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			switch (id) {
			case HD_AUDIOCAP_PARAM_DEV_CONFIG:
			{
				HD_AUDIOCAP_DEV_CONFIG* p_user = (HD_AUDIOCAP_DEV_CONFIG*)p_param;
				ISF_AUD_MAX max = {0};
				AUDCAP_AEC_CONFIG aec_config = {0};
				AUDCAP_ANR_CONFIG anr_config = {0};

				#if 0
				if ((p_user->in_max.frame_sample == 0) || (p_user->in_max.frame_sample % 1024 != 0)) {
					HD_AUDIOCAPTURE_ERR("in_max frame_sample must be multiple of 1024\r\n");
					r = -1;
					goto _HD_AC;
				}
				#else
				if ((p_user->in_max.frame_sample == 0) || (p_user->in_max.frame_sample % 4 != 0)) {
					HD_AUDIOCAPTURE_ERR("in_max frame_sample must be 4 bytes align\r\n");
					r = -1;
					goto _HD_AC;
				}
				#endif

				if (!_hd_audiocap_check_src_sr(p_user->out_max.sample_rate)) {
					HD_AUDIOCAPTURE_ERR("invalid out_max sample_rate(%u)\r\n", p_user->out_max.sample_rate);
					r = HD_ERR_INV;
					goto _HD_AC;
				}

				if (!_hd_audiocap_check_aec(&(p_user->aec_max))) {
					HD_AUDIOCAPTURE_ERR("invalid aec_max\r\n");
					r = HD_ERR_INV;
					goto _HD_AC;
				}

				if (!_hd_audiocap_check_anr(&(p_user->anr_max))) {
					HD_AUDIOCAPTURE_ERR("invalid anr_max\r\n");
					r = HD_ERR_INV;
					goto _HD_AC;
				}

				{
					UINT32 did = self_id - ISF_UNIT_AUDCAP;
					audiocap_info[did].dev_config = p_user[0];
					audiocap_info[did].b_dev_cfg_set = TRUE;
				}

				hdal_flow_log_p("in_max.sr(%u) in_max.sample_bit(%u) in_max.mode(%u) in_max.frame_sample(%u) max_frame_num(%u) out_max.sr(%u)\n",
					p_user->in_max.sample_rate, p_user->in_max.sample_bit, p_user->in_max.mode, p_user->in_max.frame_sample, p_user->frame_num_max, p_user->out_max.sample_rate);
				hdal_flow_log_p("    aec_max.en(%u) aec_max.leak_est_en(%u) aec_max.leak_est(%u) aec_max.noise_lvl(%u) aec_max.echo_lvl(%u) aec_max.filter_len(%u) aec_max.frm_size(%u) aec_max.notch_radius(%u)\n",
					p_user->aec_max.enabled, p_user->aec_max.leak_estimate_enabled, p_user->aec_max.leak_estimate_value, p_user->aec_max.noise_cancel_level, p_user->aec_max.echo_cancel_level,
					p_user->aec_max.filter_length, p_user->aec_max.frame_size, p_user->aec_max.notch_radius);
				hdal_flow_log_p("    anr_max.en(%u) anr_max.suppress_level(%u) anr_max.hpf_freq(%u) anr_max.bias_sensitive(%u)\n",
					p_user->anr_max.enabled, p_user->anr_max.suppress_level, p_user->anr_max.hpf_cut_off_freq, p_user->anr_max.bias_sensitive);

				max.max_frame = p_user->frame_num_max;
				max.max_bitpersample= p_user->in_max.sample_bit;
				max.max_channelcount= (p_user->in_max.mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				max.max_samplepersecond= p_user->in_max.sample_rate;
				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = ISF_UNIT_PARAM_AUDMAX;
				cmd.value = (UINT32)&max;
				cmd.size = sizeof(ISF_AUD_MAX);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AC;
				}
				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = AUDCAP_PARAM_MAX_FRAME_SAMPLE;
				cmd.value = p_user->in_max.frame_sample;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AC;
				}

				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = AUDCAP_PARAM_BUF_COUNT;
				cmd.value = p_user->frame_num_max;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AC;
				}


				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = AUDCAP_PARAM_DUAL_MONO;
				if (p_user->in_max.mode == HD_AUDIO_SOUND_MODE_STEREO_PLANAR) {
					cmd.value = TRUE;
				} else {
					cmd.value = FALSE;
				}
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AC;
				}

				cmd.dest = ISF_PORT(self_id, ISF_OUT(0));
				aec_config.enabled               = p_user->aec_max.enabled;
				if (aec_config.enabled) {
					aec_config.leak_estimate_enabled = p_user->aec_max.leak_estimate_enabled;
					aec_config.leak_estimate_value   = p_user->aec_max.leak_estimate_value;
					aec_config.noise_cancel_level    = p_user->aec_max.noise_cancel_level;
					aec_config.echo_cancel_level     = p_user->aec_max.echo_cancel_level;
					aec_config.filter_length         = p_user->aec_max.filter_length;
					aec_config.frame_size            = p_user->aec_max.frame_size;
					aec_config.notch_radius          = p_user->aec_max.notch_radius;
					aec_config.lb_channel            = p_user->aec_max.lb_channel;
				}

				cmd.param = AUDCAP_PARAM_AEC_MAX_CONFIG;
				cmd.value = (UINT32)&aec_config;
				cmd.size = sizeof(AUDCAP_AEC_CONFIG);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AC;
				}

				cmd.dest = ISF_PORT(self_id, ISF_OUT(0));
				anr_config.enabled          = p_user->anr_max.enabled;
				if (anr_config.enabled) {
					anr_config.suppress_level   = p_user->anr_max.suppress_level;
					anr_config.hpf_cut_off_freq = p_user->anr_max.hpf_cut_off_freq;
					anr_config.bias_sensitive   = p_user->anr_max.bias_sensitive;
				}

				cmd.param = AUDCAP_PARAM_ANR_MAX_CONFIG;
				cmd.value = (UINT32)&anr_config;
				cmd.size = sizeof(AUDCAP_ANR_CONFIG);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AC;
				}

				max.max_samplepersecond= p_user->out_max.sample_rate;
				cmd.dest = ISF_PORT(self_id, ISF_OUT(0));
				cmd.param = ISF_UNIT_PARAM_AUDMAX;
				cmd.value = (UINT32)&max;
				cmd.size = sizeof(ISF_AUD_MAX);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AC;
			}
			break;
			case HD_AUDIOCAP_PARAM_DRV_CONFIG:
			{
				HD_AUDIOCAP_DRV_CONFIG* p_user = (HD_AUDIOCAP_DRV_CONFIG*)p_param;

				{
					UINT32 did = self_id - ISF_UNIT_AUDCAP;
					audiocap_info[did].drv_config= p_user[0];
				}

				hdal_flow_log_p("mono(%u)\n", p_user->mono);

				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = AUDCAP_PARAM_CHANNEL;
				cmd.value = (p_user->mono == HD_AUDIO_MONO_LEFT)? AUDCAP_MONO_LEFT : AUDCAP_MONO_RIGHT;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AC;
			}
			break;
			case HD_AUDIOCAP_PARAM_VOLUME:
			{
				HD_AUDIOCAP_VOLUME* p_user = (HD_AUDIOCAP_VOLUME*)p_param;

				{
					UINT32 did = self_id - ISF_UNIT_AUDCAP;
					audiocap_info[did].volume[0] = p_user[0];
				}

				hdal_flow_log_p("vol(%u)\n", p_user->volume);

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = AUDCAP_PARAM_VOL_IMM;
				cmd.value = p_user->volume;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AC;
			}
			break;
			default:
				rv = HD_ERR_PARAM;
				break;
			}
		} else {
			switch (id) {
			case HD_AUDIOCAP_PARAM_IN:
			{
				HD_AUDIOCAP_IN* p_user = (HD_AUDIOCAP_IN*)p_param;
				ISF_AUD_FRAME frame = {0};
				ISF_AUD_FPS fps = {0};

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

				{
					UINT32 did = self_id - ISF_UNIT_AUDCAP;
					UINT32 iid = in_id - ISF_IN_BASE;
					audiocap_info[did].in[iid]= p_user[0];
					audiocap_info[did].b_in_cfg_set = TRUE;
				}

				hdal_flow_log_p("in.sr(%u) in.sample_bit(%u) in.mode(%u) in.frame_sample(%u)\n",
					p_user->sample_rate, p_user->sample_bit, p_user->mode, p_user->frame_sample);

				cmd.dest = ISF_PORT(self_id, in_id);
				/*
				p_user->func
				*/
				frame.bitpersample= p_user->sample_bit;
				frame.channelcount= (p_user->mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				cmd.param = ISF_UNIT_PARAM_AUDFRAME;
				cmd.value = (UINT32)&frame;
				cmd.size = sizeof(ISF_AUD_FRAME);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_AC;

				fps.samplepersecond= p_user->sample_rate;
				cmd.param = ISF_UNIT_PARAM_AUDFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_AUD_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AC;
				}

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = AUDCAP_PARAM_FRAME_SAMPLE;
				cmd.value = p_user->frame_sample;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AC;
				}

				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = AUDCAP_PARAM_DUAL_MONO;
				if (p_user->mode == HD_AUDIO_SOUND_MODE_STEREO_PLANAR) {
					cmd.value = TRUE;
				} else {
					cmd.value = FALSE;
				}
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AC;
			}
			break;
			case HD_AUDIOCAP_PARAM_OUT:
			{
				HD_AUDIOCAP_OUT* p_user = (HD_AUDIOCAP_OUT*)p_param;
				ISF_AUD_FPS fps = {0};

				if (!_hd_audiocap_check_src_sr(p_user->sample_rate)) {
					HD_AUDIOCAPTURE_ERR("invalid out.sample_rate(%u)\r\n", p_user->sample_rate);
					r = HD_ERR_INV;
					goto _HD_AC;
				}

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				{
					UINT32 did = self_id - ISF_UNIT_AUDCAP;
					UINT32 oid = out_id - ISF_OUT_BASE;
					audiocap_info[did].out[oid]= p_user[0];
				}

				hdal_flow_log_p("out.sr(%u)\n", p_user->sample_rate);

				cmd.dest = ISF_PORT(self_id, out_id);
				/*
				p_user->func
				*/
				fps.samplepersecond= p_user->sample_rate;
				cmd.param = ISF_UNIT_PARAM_AUDFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_AUD_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AC;
			}
			break;
			case HD_AUDIOCAP_PARAM_OUT_AEC:
			{
				HD_AUDIOCAP_AEC* p_user = (HD_AUDIOCAP_AEC*)p_param;
				AUDCAP_AEC_CONFIG aec_config = {0};

				if (!_hd_audiocap_check_aec(p_user)) {
					HD_AUDIOCAPTURE_ERR("invalid aec\r\n");
					r = HD_ERR_INV;
					goto _HD_AC;
				}


				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				{
					UINT32 did = self_id - ISF_UNIT_AUDCAP;
					UINT32 oid = out_id - ISF_OUT_BASE;
					audiocap_info[did].aec[oid]= p_user[0];
				}

				hdal_flow_log_p("aec.en(%u) aec.leak_est_en(%u) aec.leak_est(%u) aec.noise_lvl(%u) aec.echo_lvl(%u) aec.filter_len(%u) aec.frm_size(%u) aec.notch_radius(%u)\n",
					p_user->enabled, p_user->leak_estimate_enabled, p_user->leak_estimate_value, p_user->noise_cancel_level, p_user->echo_cancel_level,
					p_user->filter_length, p_user->frame_size, p_user->notch_radius);

				cmd.dest = ISF_PORT(self_id, out_id);

				aec_config.enabled               = p_user->enabled;
				aec_config.leak_estimate_enabled = p_user->leak_estimate_enabled;
				aec_config.leak_estimate_value   = p_user->leak_estimate_value;
				aec_config.noise_cancel_level    = p_user->noise_cancel_level;
				aec_config.echo_cancel_level     = p_user->echo_cancel_level;
				aec_config.filter_length         = p_user->filter_length;
				aec_config.frame_size            = p_user->frame_size;
				aec_config.notch_radius          = p_user->notch_radius;
				aec_config.lb_channel            = p_user->lb_channel;

				cmd.param = AUDCAP_PARAM_AEC_CONFIG;
				cmd.value = (UINT32)&aec_config;
				cmd.size = sizeof(AUDCAP_AEC_CONFIG);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AC;

			}
			break;
			case HD_AUDIOCAP_PARAM_OUT_ANR:
			{
				HD_AUDIOCAP_ANR* p_user = (HD_AUDIOCAP_ANR*)p_param;
				AUDCAP_ANR_CONFIG anr_config = {0};

				if (!_hd_audiocap_check_anr(p_user)) {
					HD_AUDIOCAPTURE_ERR("invalid anr\r\n");
					r = HD_ERR_INV;
					goto _HD_AC;
				}

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				{
					UINT32 did = self_id - ISF_UNIT_AUDCAP;
					UINT32 oid = out_id - ISF_OUT_BASE;
					audiocap_info[did].anr[oid]= p_user[0];
				}

				hdal_flow_log_p("anr.en(%u) anr.suppress_level(%u) anr.hpf_freq(%u) anr.bias_sensitive(%u)\n",
					p_user->enabled, p_user->suppress_level, p_user->hpf_cut_off_freq, p_user->bias_sensitive);

				cmd.dest = ISF_PORT(self_id, out_id);

				anr_config.enabled          = p_user->enabled;
				anr_config.suppress_level   = p_user->suppress_level;
				anr_config.hpf_cut_off_freq = p_user->hpf_cut_off_freq;
				anr_config.bias_sensitive   = p_user->bias_sensitive;

				cmd.param = AUDCAP_PARAM_ANR_CONFIG;
				cmd.value = (UINT32)&anr_config;
				cmd.size = sizeof(AUDCAP_ANR_CONFIG);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AC;

			}
			break;
			default:
				rv = HD_ERR_PARAM;
				break;
			}
		}
		if (rv != HD_OK) {
			return rv;
		}
	}
_HD_AC:
	if (r == 0) {
		switch (cmd.rv) {
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
			HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

HD_RESULT hd_audiocap_pull_out_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		HD_AUDIOCAPTURE_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (audiocap_info == NULL)) {
		return HD_ERR_UNINIT;
	}

	if (p_audio_frame == NULL) {
		HD_AUDIOCAPTURE_ERR("p_audio_frame is NULL\r\n");
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
			switch (cmd.rv) {
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}

		//--- use ISF_DATA ---
		if (rv == HD_OK) {
			_hd_audiocap_wrap_frame(p_audio_frame, &data);
		} else {
			//HD_AUDIOCAPTURE_ERR("pull out fail, r=%d, rv=%d\r\n", r, rv);
		}
	}
	return rv;

}

HD_RESULT hd_audiocap_release_out_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if ((_hd_common_get_pid() == 0) && (audiocap_info == NULL)) {
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

		_hd_audiocap_rebuild_frame(&data, p_audio_frame);

		cmd.dest = ISF_PORT(self_id, out_id);
		cmd.p_data = &data;
		cmd.async = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_RELEASE_DATA, &cmd);
		if (r == 0) {
			switch (cmd.rv) {
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
				HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_audiocap_uninit(VOID)
{
	hdal_flow_log_p("hd_audiocap_uninit\n");

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
			HD_AUDIOCAPTURE_ERR("uninit fail? %d\r\n", r);
			return r;
		}
		return HD_OK;
	}

	if (audiocap_info == NULL) {
		return HD_ERR_INIT;
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
			HD_AUDIOCAPTURE_ERR("uninit fail? %d\r\n", r);
			return r;
		}
	}

	//uninit lib
	{
		free(audiocap_info);
		audiocap_info = NULL;
	}

	return HD_OK;
}

int _hd_audiocap_is_init(VOID)
{
	return (audiocap_info != NULL);
}

HD_RESULT _hd_audiocap_kflow_param_set_by_vendor(UINT32 self_id, UINT32 out_id, UINT32 param_id, UINT32 param)
{
	UINT32 did  = self_id-DEV_BASE;
	UINT32 port = out_id-OUT_BASE;

	if (audiocap_info == NULL) {
		return HD_ERR_INIT;
	}

	if (did >= _max_path_count) {
		return HD_ERR_DEV;
	}

	if (port >= HD_AUDIOCAP_MAX_OUT_COUNT) {
		return HD_ERR_PATH;
	}

	switch (param_id) {
		case AUDCAP_PARAM_VOL_IMM:
			audiocap_info[did].volume[port].volume = param;
			return HD_OK;
		default:
			HD_AUDIOCAPTURE_ERR("unsupport param(%lu)\r\n", param_id);
			return HD_ERR_PARAM;
	}
}
