/**
    @brief Source code of audio output module.\n
    This file contains the functions which is related to audio output in the chip.

    @file hd_audioout.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "hdal.h"
#define HD_MODULE_NAME HD_AUDIOOUT
#include "hd_int.h"
#include "kflow_audioout/isf_audout.h"
#include "kflow_audiocapture/isf_audcap.h"
#include <string.h>
#include "hd_logger_p.h"

#define HD_AUDIOOUT_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE        ISF_UNIT_AUDOUT
#define DEV_COUNT       ISF_MAX_AUDOUT
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        1
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       1

#define HD_DEV_BASE HD_DAL_AUDIOOUT_BASE
#define HD_DEV_MAX  HD_DAL_AUDIOOUT_MAX

#define HD_AUDIOOUT_MAX_DEVCOUNT   1
#define HD_AUDIOOUT_MAX_IN_COUNT   1
#define HD_AUDIOOUT_MAX_OUT_COUNT  1

typedef struct _AUDOUT_INFO {
	HD_AUDIOOUT_DEV_CONFIG      dev_config;
	HD_AUDIOOUT_DRV_CONFIG      drv_config;
	HD_AUDIOOUT_IN              in[HD_AUDIOOUT_MAX_IN_COUNT];
	HD_AUDIOOUT_OUT             out[HD_AUDIOOUT_MAX_OUT_COUNT];
	HD_AUDIOOUT_VOLUME          volume;
	BOOL                        b_dev_cfg_set;
	BOOL                        b_out_cfg_set;
} AUDOUT_INFO;

#define _HD_CONVERT_SELF_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_DEV;  \
		if((dev_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) { \
			UINT32 id = (dev_id) - HD_DEV_BASE; \
			if((id < DEV_COUNT) && (id < _max_dev_count)) { \
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
			if(id < OUT_COUNT) { \
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
		if(((dev_id) >= ISF_UNIT_AUDDEC) && ((dev_id) < ISF_UNIT_AUDDEC + ISF_MAX_AUDDEC)) { \
			(dev_id) = HD_DAL_AUDIODEC_BASE + (dev_id) - ISF_UNIT_AUDDEC; \
		} else if(((dev_id) >= ISF_UNIT_AUDCAP) && ((dev_id) < ISF_UNIT_AUDCAP + ISF_MAX_AUDCAP)) { \
			(dev_id) = HD_DAL_AUDIOCAP_BASE + (dev_id) - ISF_UNIT_AUDCAP; \
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

//AUDOUT_INFO audioout_info[DEV_COUNT] = {0};
static AUDOUT_INFO* audioout_info = NULL;
static UINT32 _max_dev_count = 0;

HD_RESULT _hd_audioout_convert_dev_id(HD_DAL *p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

int _hd_audioout_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "AUDIOOUT_%d_IN_%d", dev_id - HD_DEV_BASE, in_id - HD_IN_BASE);
}

void _hd_audioout_rebuild_frame(ISF_DATA *p_data, HD_AUDIO_FRAME* p_audio_frame)
{
	AUD_FRAME * p_audframe = (AUD_FRAME *) p_data->desc;
	HD_DAL dev_id = p_audio_frame->blk >> 16;

	p_data->sign    = MAKEFOURCC('I','S','F','D');

	if((dev_id) >= HD_DAL_AUDIOCAP_BASE && (dev_id) <= HD_DAL_AUDIOCAP_MAX) {
		//check if dev_id is AUDIOCAP
		dev_id -= HD_DAL_AUDIOCAP_BASE;
		p_audframe->sign = MAKEFOURCC('A','C','A','P');
		p_data->h_data = (dev_id << 16) | (p_audio_frame->blk & 0xFFFF);
	} else if((dev_id) >= HD_DAL_AUDIODEC_BASE && (dev_id) <= HD_DAL_AUDIODEC_MAX) {
		//check if dev_id is AUDIODEC
		dev_id -= HD_DAL_AUDIODEC_BASE;
		p_audframe->sign = MAKEFOURCC('A','D','E','C');
		p_data->h_data = (dev_id << 16) | (p_audio_frame->blk & 0xFFFF);
	} else if (p_audio_frame->blk == 0) {
		p_audframe->sign = MAKEFOURCC('A','F','R','M');
		p_data->h_data = 0;
	}
}

HD_RESULT _hd_audioout_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
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
				HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}


HD_RESULT _hd_audioout_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id)
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
				HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT _hd_audioout_get_state(HD_OUT_ID _out_id, UINT32* p_state)
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
				HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

void _hd_audioout_cfg_max(UINT32 maxdevice)
{
	if (!audioout_info) {
		_max_dev_count = maxdevice;
		if (_max_dev_count > DEV_COUNT) {
			HD_AUDIOOUT_WRN("dts dev=%lu is larger than built-in max_dev=%lu!\r\n", _max_dev_count, (UINT32)DEV_COUNT);
			_max_dev_count = DEV_COUNT;
		}
	}
}

HD_RESULT hd_audioout_init(VOID)
{
	UINT32 i;

	hdal_flow_log_p("hd_audioout_init\n");

	/*for(i = 0; i < DEV_COUNT; i++) {
		memset(&audioout_info[i], 0, sizeof(AUDOUT_INFO));
		audioout_info[i].volume.volume = 100;
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
			HD_AUDIOOUT_ERR("init fail? %d\r\n", r);
			return r;
		}
		return HD_OK;
	}

	if (dev_fd <= 0) {
		return HD_ERR_INIT;
	}

	if (audioout_info != NULL) {
		return HD_ERR_UNINIT;
	}

	if (_max_dev_count == 0) {
		DBG_ERR("_max_dev_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	audioout_info = (AUDOUT_INFO*)malloc(sizeof(AUDOUT_INFO)*_max_dev_count);
	if (audioout_info == NULL) {
		DBG_ERR("cannot alloc heap for _max_dev_count =%d\r\n", (int)_max_dev_count);
		return HD_ERR_HEAP;
	}
	for(i = 0; i < _max_dev_count; i++) {
		memset(&(audioout_info[i]), 0, sizeof(AUDOUT_INFO));
		audioout_info[i].volume.volume = 100;
	}

	//init drv
	{
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = _max_dev_count;
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			free(audioout_info);
			audioout_info = NULL;
			DBG_ERR("init fail? %d\r\n", r);
			return r;
		}
	}

	return HD_OK;
}

HD_RESULT hd_audioout_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioout_open:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) in(%d) ", self_id, out_id, in_id);

	if (dev_fd <= 0) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioout_info == NULL) {
		return HD_ERR_UNINIT;
	}

	if(!p_path_id) {
		hdal_flow_log_p("path_id(N/A)\n");
		return HD_ERR_NULL_PTR;
	}

	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		_HD_CONVERT_SELF_ID(self_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
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
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_OUT_ID(check_out_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("out_id=%lu is larger than max=%lu!\r\n", (UINT32)((check_out_id) - HD_OUT_BASE), (UINT32)HD_AUDIOOUT_MAX_OUT_COUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_SELF_ID(in_dev_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((in_dev_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		_HD_CONVERT_IN_ID(check_in_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("in_id=%lu is larger than max=%lu!\r\n", (UINT32)((check_in_id) - HD_IN_BASE), (UINT32)HD_AUDIOOUT_MAX_IN_COUNT);
			hdal_flow_log_p("path_id(N/A)\n");
			return rv;
		}
		if(in_dev_id != self_id) {
			DBG_ERR("dev_id of in/out is not the same!\r\n");
			hdal_flow_log_p("path_id(N/A)\n");
			return HD_ERR_DEV;
		}
		p_path_id[0] = HD_AUDIOOUT_PATH(cur_dev, in_id, out_id);
		hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);
	}

	//--- check if DEV_CONFIG info had been set ---
	{
		UINT32 did = self_id - ISF_UNIT_AUDOUT;
		if (audioout_info[did].b_dev_cfg_set == FALSE) {
			HD_AUDIOOUT_ERR("Error: please set HD_AUDIOOUT_PARAM_DEV_CONFIG first !!!\r\n");
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
				HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_audioout_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioout_start:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioout_info == NULL) {
		return HD_ERR_UNINIT;
	}

	_HD_CONVERT_SELF_ID(self_id, rv);
	if (rv != HD_OK) {
		return rv;
	}

	//--- check if OUT info had been set ---
	{
		UINT32 did = self_id - ISF_UNIT_AUDOUT;
		if (audioout_info[did].b_out_cfg_set == FALSE) {
			HD_AUDIOOUT_ERR("Error: please set HD_AUDIOOUT_PARAM_OUT first !!!\r\n");
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
				HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_audioout_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioout_stop:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioout_info == NULL) {
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
				HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_audioout_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audioout_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audioout_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_audioout_close:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioout_info == NULL) {
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
				HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

static INT _hd_audioout_param_cvt_name(HD_AUDIOOUT_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
		case HD_AUDIOOUT_PARAM_DEVCOUNT:    snprintf(p_ret_string, max_str_len, "DEVCOUNT");   break;
		case HD_AUDIOOUT_PARAM_SYSCAPS:     snprintf(p_ret_string, max_str_len, "SYSCAPS");    break;
		case HD_AUDIOOUT_PARAM_SYSINFO:     snprintf(p_ret_string, max_str_len, "SYSINFO");    break;
		case HD_AUDIOOUT_PARAM_DEV_CONFIG:  snprintf(p_ret_string, max_str_len, "DEV_CONFIG"); break;
		case HD_AUDIOOUT_PARAM_DRV_CONFIG:  snprintf(p_ret_string, max_str_len, "DRV_CONFIG"); break;
		case HD_AUDIOOUT_PARAM_IN:          snprintf(p_ret_string, max_str_len, "IN");         break;
		case HD_AUDIOOUT_PARAM_OUT:         snprintf(p_ret_string, max_str_len, "OUT");        break;
		case HD_AUDIOOUT_PARAM_VOLUME:      snprintf(p_ret_string, max_str_len, "VOLUME");     break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", id);
			return (-1);
	}
	return 0;
}

static BOOL _hd_audioout_check_src_sr(UINT32 sr)
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

HD_RESULT hd_audioout_get(HD_PATH_ID path_id, HD_AUDIOOUT_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	{
		CHAR  param_name[20];
		_hd_audioout_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_audioout_get(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioout_info == NULL) {
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
			case HD_AUDIOOUT_PARAM_DEVCOUNT:
			{
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
				p_user->max_dev_count = _max_dev_count;
				rv = HD_OK;
				hdal_flow_log_p("max_dev_count(%u)\n", p_user->max_dev_count);
			}
			break;
			case HD_AUDIOOUT_PARAM_SYSCAPS:
			{
				HD_AUDIOOUT_SYSCAPS *p_user = (HD_AUDIOOUT_SYSCAPS *)p_param;

				memset(p_user, 0, sizeof(HD_AUDIOOUT_SYSCAPS));
				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;
				p_user->chip_id = 0;
				p_user->max_in_count = HD_AUDIOOUT_MAX_IN_COUNT;
				p_user->max_out_count = HD_AUDIOOUT_MAX_OUT_COUNT;
				p_user->dev_caps =
					HD_CAPS_DEVCONFIG
					 | HD_CAPS_DRVCONFIG
					 | HD_AUDIOOUT_DEVCAPS_SPK
					 | HD_AUDIOOUT_DEVCAPS_LINEOUT
					 | HD_AUDIOOUT_DEVCAPS_DIFF_SR
					 | HD_AUDIOOUT_DEVCAPS_VOLUME;
				p_user->in_caps[0] = HD_AUDIO_CAPS_RESAMPLE_UP;

				p_user->out_caps[0] =
					HD_AUDIO_CAPS_BITS_16
					 | HD_AUDIO_CAPS_CH_MONO
					 | HD_AUDIO_CAPS_CH_STEREO;
				p_user->support_out_sr[0] =
					HD_AUDIOOUT_SRCAPS_8000
					 | HD_AUDIOOUT_SRCAPS_11025
					 | HD_AUDIOOUT_SRCAPS_12000
					 | HD_AUDIOOUT_SRCAPS_16000
					 | HD_AUDIOOUT_SRCAPS_22050
					 | HD_AUDIOOUT_SRCAPS_24000
					 | HD_AUDIOOUT_SRCAPS_32000
					 | HD_AUDIOOUT_SRCAPS_44100
					 | HD_AUDIOOUT_SRCAPS_48000;
				p_user->support_in_sr[0] =
					HD_AUDIOOUT_SRCAPS_8000
					 | HD_AUDIOOUT_SRCAPS_11025
					 | HD_AUDIOOUT_SRCAPS_12000
					 | HD_AUDIOOUT_SRCAPS_16000
					 | HD_AUDIOOUT_SRCAPS_22050
					 | HD_AUDIOOUT_SRCAPS_24000
					 | HD_AUDIOOUT_SRCAPS_32000
					 | HD_AUDIOOUT_SRCAPS_44100
					 | HD_AUDIOOUT_SRCAPS_48000;

				rv = HD_OK;

				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip_id, p_user->max_in_count,
																									p_user->max_out_count, p_user->dev_caps);
				hdal_flow_log_p("    in_caps(0x%08x) out_caps(0x%08x)\n", p_user->in_caps[0], p_user->out_caps[0]);
				hdal_flow_log_p("    support_out_sr(0x%08x) support_in_sr(0x%08x)\n", p_user->support_out_sr[0], p_user->support_in_sr[0]);
			}
			break;
			case HD_AUDIOOUT_PARAM_SYSINFO:
			{
				HD_AUDIOOUT_SYSINFO* p_user = (HD_AUDIOOUT_SYSINFO*)p_param;
				AUDOUT_SYSINFO sys_info;
				UINT8 i;

				memset(p_user, 0, sizeof(HD_AUDIOOUT_SYSINFO));
				memset(&sys_info, 0, sizeof(sys_info));
				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;
				rv = HD_OK;

				ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = AUDOUT_PARAM_SYS_INFO;
				cmd.size = sizeof(AUDOUT_SYSINFO);
				cmd.value = (UINT32)&sys_info;

				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);

				if (r == 0) {
					switch(cmd.rv) {
					case ISF_OK:
						rv = HD_OK;
						p_user->cur_out_sample_rate = sys_info.cur_out_sample_rate;
						p_user->cur_mode = sys_info.cur_mode;
						p_user->cur_sample_bit = sys_info.cur_sample_bit;
						for (i = 0; i < HD_AUDIOOUT_MAX_IN; i++) {
							p_user->cur_in_sample_rate[i] = sys_info.cur_in_sample_rate[i];
						}
						hdal_flow_log_p("cur_out_sr(%u) cur_mode(%u) cur_sample_bit(%u) cur_in_sr(%u)\n", p_user->cur_out_sample_rate, p_user->cur_mode, p_user->cur_sample_bit, p_user->cur_in_sample_rate[0]);
						break;
					default:
						rv = HD_ERR_SYS;
						break;
					}
				} else {
					if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
						rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
					} else {
						HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
						rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
					}
				}
			}
			break;
			case HD_AUDIOOUT_PARAM_DEV_CONFIG:
			{
				HD_AUDIOOUT_DEV_CONFIG* p_user = (HD_AUDIOOUT_DEV_CONFIG*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDOUT;
				p_user[0] = audioout_info[did].dev_config;
				hdal_flow_log_p("out_max_sr(%u) out_max_sample_bit(%u) out_max_mode(%u)\n", p_user->out_max.sample_rate, p_user->out_max.sample_bit, p_user->out_max.mode);
				hdal_flow_log_p("    max_frame_sample(%u) max_frame_num(%u) in_max_sr(%u)\n", p_user->frame_sample_max, p_user->frame_num_max, p_user->in_max.sample_rate);
				rv = HD_OK;
			}
			break;
			case HD_AUDIOOUT_PARAM_DRV_CONFIG:
			{
				HD_AUDIOOUT_DRV_CONFIG* p_user = (HD_AUDIOOUT_DRV_CONFIG*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDOUT;
				p_user[0] = audioout_info[did].drv_config;
				hdal_flow_log_p("mono(%u) output(%u)\n", p_user->mono, p_user->output);
				rv = HD_OK;
			}
			break;
			case HD_AUDIOOUT_PARAM_VOLUME:
			{
				HD_AUDIOOUT_VOLUME* p_user = (HD_AUDIOOUT_VOLUME*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDOUT;
				p_user[0] = audioout_info[did].volume;
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
			case HD_AUDIOOUT_PARAM_IN:
			if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
			_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}
			{
				HD_AUDIOOUT_IN* p_user = (HD_AUDIOOUT_IN*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDOUT;
				UINT32 iid = in_id - ISF_IN_BASE;
				p_user[0] = audioout_info[did].in[iid];
				hdal_flow_log_p("in_sr(%u)\n", p_user->sample_rate);
				rv = HD_OK;
			}
			break;
			case HD_AUDIOOUT_PARAM_OUT:
			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			{
				HD_AUDIOOUT_OUT* p_user = (HD_AUDIOOUT_OUT*)p_param;
				UINT32 did = self_id - ISF_UNIT_AUDOUT;
				UINT32 oid = out_id - ISF_OUT_BASE;
				p_user[0] = audioout_info[did].out[oid];
				hdal_flow_log_p("out_sr(%u) out_sample_bit(%u) out_mode(%u)\n", p_user->sample_rate, p_user->sample_bit, p_user->mode);
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

HD_RESULT hd_audioout_set(HD_PATH_ID path_id, HD_AUDIOOUT_PARAM_ID id, VOID *p_param)
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
		_hd_audioout_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_audioout_set(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() > 0) {
		DBG_ERR("system fail, not support\r\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if (audioout_info == NULL) {
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
			case HD_AUDIOOUT_PARAM_DEV_CONFIG:
			{
				HD_AUDIOOUT_DEV_CONFIG* p_user = (HD_AUDIOOUT_DEV_CONFIG*)p_param;
				ISF_AUD_MAX max = {0};

				#if 0
				if ((p_user->frame_sample_max == 0) || (p_user->frame_sample_max % 1024 != 0)) {
					HD_AUDIOOUT_ERR("frame_sample_max must be multiple of 1024\r\n");
					r = -1;
					goto _HD_AO;
				}
				#else
				if ((p_user->frame_sample_max == 0) || (p_user->frame_sample_max % 4 != 0)) {
					HD_AUDIOOUT_ERR("frame_sample_max must be 4 bytes align\r\n");
					r = -1;
					goto _HD_AO;
				}
				#endif

				if (!_hd_audioout_check_src_sr(p_user->in_max.sample_rate)) {
					HD_AUDIOOUT_ERR("invalid in_max sample_rate(%u)\r\n", p_user->in_max.sample_rate);
					r = HD_ERR_INV;
					goto _HD_AO;
				}

				{
					UINT32 did = self_id - ISF_UNIT_AUDOUT;
					audioout_info[did].dev_config = p_user[0];
					audioout_info[did].b_dev_cfg_set = TRUE;
				}
				hdal_flow_log_p("out_max_sr(%u) out_max_sample_bit(%u) out_max_mode(%u)\n", p_user->out_max.sample_rate, p_user->out_max.sample_bit, p_user->out_max.mode);
				hdal_flow_log_p("    max_frame_sample(%u) max_frame_num(%u) in_max_sr(%u)\n", p_user->frame_sample_max, p_user->frame_num_max, p_user->in_max.sample_rate);


				max.max_frame = p_user->frame_num_max;
				max.max_bitpersample= p_user->out_max.sample_bit;
				max.max_channelcount= (p_user->out_max.mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				max.max_samplepersecond= p_user->out_max.sample_rate;
				cmd.dest = ISF_PORT(self_id, ISF_OUT(0));
				cmd.param = ISF_UNIT_PARAM_AUDMAX;
				cmd.value = (UINT32)&max;
				cmd.size = sizeof(ISF_AUD_MAX);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AO;
				}

				cmd.dest = ISF_PORT(self_id, ISF_OUT(0));
				cmd.param = AUDOUT_PARAM_MAX_FRAME_SAMPLE;
				cmd.value = p_user->frame_sample_max;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AO;
				}

				cmd.dest = ISF_PORT(self_id, ISF_OUT(0));
				cmd.param = AUDOUT_PARAM_BUF_COUNT;
				cmd.value = p_user->frame_num_max;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					goto _HD_AO;
				}

				max.max_samplepersecond= p_user->in_max.sample_rate;
				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = ISF_UNIT_PARAM_AUDMAX;
				cmd.value = (UINT32)&max;
				cmd.size = sizeof(ISF_AUD_MAX);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AO;
			}
			break;
			case HD_AUDIOOUT_PARAM_VOLUME:
			{
				HD_AUDIOOUT_VOLUME* p_user = (HD_AUDIOOUT_VOLUME*)p_param;

				{
					UINT32 did = self_id - ISF_UNIT_AUDOUT;

					audioout_info[did].volume = p_user[0];
				}

				hdal_flow_log_p("vol(%u)\n", p_user->volume);

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = AUDOUT_PARAM_VOL_IMM;
				cmd.value = p_user->volume;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AO;
			}
			break;
			case HD_AUDIOOUT_PARAM_DRV_CONFIG:
			{
				HD_AUDIOOUT_DRV_CONFIG* p_user = (HD_AUDIOOUT_DRV_CONFIG*)p_param;
				ISF_SET set[2] = {0};

				{
					UINT32 did = self_id - ISF_UNIT_AUDOUT;
					audioout_info[did].drv_config = p_user[0];
				}

				hdal_flow_log_p("mono(%u) output(%u)\n", p_user->mono, p_user->output);

				set[0].param = AUDOUT_PARAM_CHANNEL;
				set[0].value = (p_user->mono == HD_AUDIO_MONO_LEFT)? AUDOUT_MONO_LEFT : AUDOUT_MONO_RIGHT;
				set[1].param = AUDOUT_PARAM_OUT_DEV;
				switch (p_user->output) {
					case HD_AUDIOOUT_OUTPUT_LINE:
					{
						set[1].value = AUDOUT_OUTPUT_LINE;
					}
					break;
					case HD_AUDIOOUT_OUTPUT_ALL:
					{
						set[1].value = AUDOUT_OUTPUT_ALL;
					}
					break;
					case HD_AUDIOOUT_OUTPUT_I2S:
					{
						set[1].value = AUDOUT_OUTPUT_I2S;
					}
					break;
					case HD_AUDIOOUT_OUTPUT_SPK:
					default:
						set[1].value = AUDOUT_OUTPUT_SPK;
						break;
				}
				//set[1].value = (p_user->output == HD_AUDIOOUT_OUTPUT_SPK)? AUDOUT_OUTPUT_SPK : AUDOUT_OUTPUT_LINE;
				cmd.dest = ISF_PORT(self_id, ISF_OUT(0));
				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*2;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AO;
			}
			break;
			default:
				rv = HD_ERR_PARAM;
				break;
			}
		} else {
			switch (id) {
			case HD_AUDIOOUT_PARAM_IN:
			{
				HD_AUDIOOUT_IN* p_user = (HD_AUDIOOUT_IN*)p_param;
				ISF_AUD_FPS fps = {0};

				if (!_hd_audioout_check_src_sr(p_user->sample_rate)) {
					HD_AUDIOOUT_ERR("invalid in.sample_rate(%u)\r\n", p_user->sample_rate);
					r = HD_ERR_INV;
					goto _HD_AO;
				}

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

				{
					UINT32 did = self_id - ISF_UNIT_AUDOUT;
					UINT32 iid = in_id - ISF_IN_BASE;
					audioout_info[did].in[iid]= p_user[0];
				}

				hdal_flow_log_p("in_sr(%u)\n", p_user->sample_rate);

				cmd.dest = ISF_PORT(self_id, in_id);
				/*
				p_user->func
				*/
				fps.samplepersecond= p_user->sample_rate;
				cmd.param = ISF_UNIT_PARAM_AUDFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_AUD_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AO;
			}
			break;
			case HD_AUDIOOUT_PARAM_OUT:
			{
				HD_AUDIOOUT_OUT* p_user = (HD_AUDIOOUT_OUT*)p_param;
				ISF_AUD_FRAME frame = {0};
				ISF_AUD_FPS fps = {0};

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

				{
					UINT32 did = self_id - ISF_UNIT_AUDOUT;
					UINT32 oid = out_id - ISF_OUT_BASE;
					audioout_info[did].out[oid]= p_user[0];
					audioout_info[did].b_out_cfg_set = TRUE;
				}

				hdal_flow_log_p("out_sr(%u) out_sample_bit(%u) out_mode(%u)\n", p_user->sample_rate, p_user->sample_bit, p_user->mode);

				cmd.dest = ISF_PORT(self_id, out_id);
				/*
				p_user->func
				*/
				frame.bitpersample = p_user->sample_bit;
				frame.channelcount = (p_user->mode == HD_AUDIO_SOUND_MODE_MONO)? 1:2;
				cmd.param = ISF_UNIT_PARAM_AUDFRAME;
				cmd.value = (UINT32)&frame;
				cmd.size = sizeof(ISF_AUD_FRAME);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_AO;

				fps.samplepersecond = p_user->sample_rate;
				cmd.param = ISF_UNIT_PARAM_AUDFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_AUD_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_AO;
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

_HD_AO:
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
			HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}

HD_RESULT hd_audioout_push_in_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_in_audio_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		HD_AUDIOOUT_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}

	if (p_in_audio_frame == NULL) {
		HD_AUDIOOUT_ERR("p_in_audio_bs is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	if ((_hd_common_get_pid() == 0) && (audioout_info == NULL)) {
		return HD_ERR_UNINIT;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};
		AUD_FRAME aud_frame = {0};

		_hd_audioout_rebuild_frame(&data, p_in_audio_frame);

		// set aud_bs info
		aud_frame.sign        = p_in_audio_frame->sign;
		aud_frame.phyaddr[0]  = p_in_audio_frame->phy_addr[0];
		aud_frame.size        = p_in_audio_frame->size;
		aud_frame.timestamp   = p_in_audio_frame->timestamp;
		aud_frame.bit_width   = p_in_audio_frame->bit_width;
		aud_frame.sound_mode  = p_in_audio_frame->sound_mode;
		aud_frame.sample_rate = p_in_audio_frame->sample_rate;

		// set isf_data
		data.size = p_in_audio_frame->size; // it could be the whole isf_data, but we only have one data bs_size
		memcpy(&data.desc, &aud_frame, sizeof(AUD_FRAME));

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
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
				HD_AUDIOOUT_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}

		/*if (rv != HD_OK) {
			HD_AUDIOOUT_ERR("push in fail, r=%d, rv=%d\r\n", r, rv);
		}*/
	}

	return rv;
}

HD_RESULT hd_audioout_uninit(VOID)
{
	hdal_flow_log_p("hd_audioout_uninit\n");

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
			HD_AUDIOOUT_ERR("uninit fail? %d\r\n", r);
			return r;
		}
		return HD_OK;
	}

	if (audioout_info == NULL) {
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
			DBG_ERR("uninit fail? %d\r\n", r);
			return r;
		}
	}

	//uninit lib
	{
		free(audioout_info);
		audioout_info = NULL;
	}

	return HD_OK;
}

int _hd_audioout_is_init(VOID)
{
	return (audioout_info != NULL);
}

HD_RESULT _hd_audioout_kflow_param_set_by_vendor(UINT32 self_id, UINT32 out_id, UINT32 param_id, UINT32 param)
{
	UINT32 did  = self_id-DEV_BASE;

	if (audioout_info == NULL) {
		return HD_ERR_INIT;
	}

	if (did >= _max_dev_count) {
		return HD_ERR_DEV;
	}

	switch (param_id) {
		case AUDOUT_PARAM_VOL_IMM:
			audioout_info[did].volume.volume = param;
			return HD_OK;
		default:
			HD_AUDIOCAPTURE_ERR("unsupport param(%lu)\r\n", param_id);
			return HD_ERR_PARAM;
	}
}