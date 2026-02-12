/**
	@brief Source file of vendor media audiocapture.\n

	@file vendor_audiocapture.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#if defined(__LINUX)
#include <sys/ioctl.h>
#endif
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"
#include "vendor_audiocapture.h"
#include "kflow_audiocapture/isf_audcap.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define ISF_OPEN     isf_flow_open
#define ISF_IOCTL    isf_flow_ioctl
#define ISF_CLOSE    isf_flow_close

#define DBG_ERR(fmt, args...)
#define DBG_DUMP
#endif
#if defined(__LINUX)
#define ISF_OPEN     open
#define ISF_IOCTL    ioctl
#define ISF_CLOSE    close

#define DBG_ERR(fmt, args...) 	printf("%s: " fmt, __func__, ##args)
#define DBG_DUMP				printf
#endif


/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define HD_DEV_BASE HD_DAL_AUDIOCAP_BASE
#define HD_DEV_MAX  HD_DAL_AUDIOCAP_MAX
#define DEV_BASE        ISF_UNIT_AUDCAP
#define DEV_COUNT       ISF_MAX_AUDCAP
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       2

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
			if(id < OUT_COUNT) { \
				(out_id) = OUT_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern int _hd_common_get_fd(void);
extern HD_RESULT _hd_audiocap_kflow_param_set_by_vendor(UINT32 self_id, UINT32 out_id, UINT32 param_id, UINT32 param);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_audiocap_set(UINT32 id, VENDOR_AUDIOCAP_ITEM item, VOID *p_param)
{
	INT32 ret;
	int isf_fd;

	HD_DAL self_id = HD_GET_DEV(id);
	HD_IO out_id = HD_GET_OUT(id);
	HD_IO ctrl_id = HD_GET_CTRL(id);
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	int r;

	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}

	switch (item) {
		case VENDOR_AUDIOCAP_ITEM_EXT: {
			AUDCAP_AUD_INIT_CFG init_config = {0};
			VENDOR_AUDIOCAP_INIT_CFG* p_user = (VENDOR_AUDIOCAP_INIT_CFG*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			if (p_user->driver_name[0] != 0) {
				snprintf(init_config.driver_name, VENDOR_AUDIOCAP_NAME_LEN-1, p_user->driver_name);
				init_config.aud_init_cfg.pin_cfg.pinmux.audio_pinmux  = p_user->aud_init_cfg.pin_cfg.pinmux.audio_pinmux;
				init_config.aud_init_cfg.pin_cfg.pinmux.cmd_if_pinmux = p_user->aud_init_cfg.pin_cfg.pinmux.cmd_if_pinmux;
				init_config.aud_init_cfg.i2s_cfg.bit_width     = p_user->aud_init_cfg.i2s_cfg.bit_width;
				init_config.aud_init_cfg.i2s_cfg.bit_clk_ratio = p_user->aud_init_cfg.i2s_cfg.bit_clk_ratio;
				init_config.aud_init_cfg.i2s_cfg.op_mode       = p_user->aud_init_cfg.i2s_cfg.op_mode;
				init_config.aud_init_cfg.i2s_cfg.tdm_ch        = p_user->aud_init_cfg.i2s_cfg.tdm_ch;
			} else {
				init_config.driver_name[0] = 0;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_AUD_INIT_CFG;
			cmd.value = (UINT32)&init_config;
			cmd.size = sizeof(AUDCAP_AUD_INIT_CFG);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_DEFAULT_SETTING: {
			UINT32 setting = *(UINT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {
				return HD_ERR_IO;
			}

			_HD_CONVERT_OUT_ID(out_id, ret);
			if(ret != HD_OK) {
				return ret;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = AUDCAP_PARAM_AUD_DEFAULT_SETTING;
			cmd.value = setting;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}

			break;
		}
		case VENDOR_AUDIOCAP_ITEM_NOISEGATE_THRESHOLD: {
			INT32 threshold = *(INT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {
				return HD_ERR_IO;
			}

			_HD_CONVERT_OUT_ID(out_id, ret);
			if(ret != HD_OK) {
				return ret;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = AUDCAP_PARAM_AUD_NG_THRESHOLD;
			cmd.value = (UINT32)&threshold;
			cmd.size = sizeof(INT32);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}

			break;
		}
		case VENDOR_AUDIOCAP_ITEM_AGC_CONFIG: {
			AUDCAP_AGC_CONFIG agc_config = {0};
			VENDOR_AUDIOCAP_AGC_CONFIG* p_user = (VENDOR_AUDIOCAP_AGC_CONFIG*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {
				return HD_ERR_IO;
			}

			_HD_CONVERT_OUT_ID(out_id, ret);
			if(ret != HD_OK) {
				return ret;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			agc_config.enable       = p_user->enable;
			agc_config.target_lvl   = p_user->target_lvl;
			agc_config.ng_threshold = p_user->ng_threshold;
			//agc_config.decay_time   = p_user->decay_time;
			//agc_config.attack_time  = p_user->attack_time;
			//agc_config.target_lvl   = p_user->target_lvl;
			//agc_config.max_gain     = p_user->max_gain;
			//agc_config.min_gain     = p_user->min_gain;

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = AUDCAP_PARAM_AGC_CFG;
			cmd.value = (UINT32)&agc_config;
			cmd.size = sizeof(AUDCAP_AGC_CONFIG);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_ALC_ENABLE: {
			INT32 enable = *(INT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_AUD_ALC_EN;
			cmd.value = enable;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_REC_SRC: {
			UINT32 src = *(UINT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_REC_SRC;
			cmd.value = src;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_PREPWR_ENABLE: {
			INT32 enable = *(INT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_AUD_PREPWR_EN;
			cmd.value = enable;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_VOLUME: {
			VENDOR_AUDIOCAP_VOLUME* vol = (VENDOR_AUDIOCAP_VOLUME*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {
				return HD_ERR_IO;
			}

			_HD_CONVERT_OUT_ID(out_id, ret);
			if(ret != HD_OK) {
				return ret;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			_hd_audiocap_kflow_param_set_by_vendor(self_id, out_id, AUDCAP_PARAM_VOL_IMM, vol->volume);

			ret = HD_OK;

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = AUDCAP_PARAM_VOL_IMM;
			cmd.value = (UINT32)vol->volume;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}

			break;
		}
		case VENDOR_AUDIOCAP_ITEM_LOOPBACK_CONFIG: {
			VENDOR_AUDIOCAP_LOOPBACK_CONFIG* loopback = (VENDOR_AUDIOCAP_LOOPBACK_CONFIG*)p_param;
			AUDCAP_LB_CONFIG lb_config = {0};

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			ret = HD_OK;

			lb_config.enabled    = loopback->enabled;
			lb_config.lb_channel = loopback->lb_channel;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_LB_CONFIG;
			cmd.value = (UINT32)&lb_config;
			cmd.size = sizeof(AUDCAP_LB_CONFIG);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}

			break;
		}
		case VENDOR_AUDIOCAP_ITEM_GAIN_LEVEL: {
			UINT32 level = *(UINT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_GAIN_LEVEL;
			cmd.value = level;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_TDM_CH: {
			UINT32 tdm_ch = *(UINT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_TDM_CH;
			cmd.value = tdm_ch;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_ALC_CONFIG: {
			AUDCAP_ALC_CONFIG alc_config = {0};
			VENDOR_AUDIOCAP_ALC_CONFIG* p_user = (VENDOR_AUDIOCAP_ALC_CONFIG*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			if ((p_user->max_gain < VENDOR_AUDIOCAP_ALC_GAIN_MIN || p_user->max_gain > VENDOR_AUDIOCAP_ALC_GAIN_MAX)
				|| (p_user->min_gain < VENDOR_AUDIOCAP_ALC_GAIN_MIN || p_user->min_gain > VENDOR_AUDIOCAP_ALC_GAIN_MAX)) {
				ret = HD_ERR_INV;
				return ret;
			}

			alc_config.decay_time   = p_user->decay_time;
			alc_config.attack_time  = p_user->attack_time;
			alc_config.max_gain     = p_user->max_gain;
			alc_config.min_gain     = p_user->min_gain;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_AUD_ALC_CONFIG;
			cmd.value = (UINT32)&alc_config;
			cmd.size = sizeof(AUDCAP_ALC_CONFIG);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_MONO_EXPAND: {
			UINT32 mono_expand = *(UINT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_MONO_EXPAND;
			cmd.value = mono_expand;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_AEC_REF_BYPASS_FUNC: {
			BOOL enable = *(BOOL*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_AEC_REF_BYPASS_FUNC;
			cmd.value = enable;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_AEC_REF_BYPASS_ENABLE: {
			BOOL enable = *(BOOL*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_AEC_REF_BYPASS_ENABLE;
			cmd.value = enable;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		case VENDOR_AUDIOCAP_ITEM_AEC_PRE_INIT: {
			BOOL enable = *(BOOL*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			if (ctrl_id != HD_CTRL) {
				ret = HD_ERR_IO;
				//printf("vendor_audiocapture_set invalid id %x\n", id);
				return ret;
			}

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = AUDCAP_PARAM_AEC_PRE_INIT;
			cmd.value = enable;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

			if (r == 0) {
				switch (cmd.rv) {
				case ISF_OK:
					ret = HD_OK;
					break;
				default:
					ret = HD_ERR_SYS;
					break;
				}
			} else {
				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
					ret= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				} else {
					DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
					ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
				}
			}
			break;
		}
		default:
			ret = HD_ERR_PARAM;
			//printf("vendor_audiocapture_set not support item %d \n", item);
			break;
	}


	return ret;
}

static void _vendor_audiocap_wrap_frame(HD_AUDIO_FRAME* p_audio_frame, HD_AUDIO_FRAME* p_audio_ref_frame, ISF_DATA *p_data)
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

	p_audio_ref_frame->sign                  = MAKEFOURCC('A','F','R','M');
	p_audio_ref_frame->phy_addr[0]           = p_audframe->reserved[0];
	p_audio_ref_frame->size                  = p_audframe->reserved[1];
	p_audio_ref_frame->timestamp             = (UINT64)p_audframe->reserved[2] + (((UINT64)(p_audframe->reserved[3])) << 32);
}

HD_RESULT vendor_audiocap_get(UINT32 id, VENDOR_AUDIOCAP_ITEM item, VOID *p_param)
{
	INT32 ret;
	int isf_fd;

	HD_DAL self_id = HD_GET_DEV(id);
	HD_IO out_id = HD_GET_OUT(id);
	int r;

	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}

	switch (item) {
		case VENDOR_AUDIOCAP_ITEM_AUDIO_FRAME: {
			ret = HD_ERR_NG;
			VENDOR_AUDIOCAP_AUDIO_FRAME* p_user = (VENDOR_AUDIOCAP_AUDIO_FRAME*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			{
				ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
				ISF_DATA data = {0};
				_HD_CONVERT_SELF_ID(self_id, ret); 	if(ret != HD_OK) {	return ret;}
				_HD_CONVERT_OUT_ID(out_id, ret);	if(ret != HD_OK) {	return ret;}
				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.p_data = &data;
				cmd.async = p_user->wait_ms;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_PULL_DATA, &cmd);
				if (r == 0) {
					switch (cmd.rv) {
					case ISF_OK:
						ret = HD_OK;
						break;
					default:
						ret = HD_ERR_SYS;
						break;
					}
				} else {
					if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
						ret = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
					} else {
						//HD_AUDIOCAPTURE_ERR("system fail, rv=%d\r\n", cmd.rv);
						ret = cmd.rv; // ISF_ERR is out of range of HD_ERR
					}
				}

				//--- use ISF_DATA ---
				if (ret == HD_OK) {
					_vendor_audiocap_wrap_frame(&(p_user->audio_frame), &(p_user->audio_aec_ref_frame), &data);
				} else {
					//HD_AUDIOCAPTURE_ERR("pull out fail, r=%d, rv=%d\r\n", r, rv);
				}
			}
			break;
		}
		default:
			ret = HD_ERR_PARAM;
			//printf("vendor_audiocapture_get not support item %d \n", item);
			break;
	}
	return ret;
}

