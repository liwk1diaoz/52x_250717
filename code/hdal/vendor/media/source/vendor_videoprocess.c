/**
	@brief Source file of vendor media videocapture.\n

	@file vendor_videocapture.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#if defined(__LINUX)
#include <sys/ioctl.h>
#endif
#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME VENDOR_VIDEOPROCESS

#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"
#include "kflow_videoprocess/isf_vdoprc.h"
#include "vendor_videoprocess.h"


#if defined (__FREERTOS)
#define ISF_OPEN     isf_flow_open
#define ISF_IOCTL    isf_flow_ioctl
#define ISF_CLOSE    isf_flow_close
#endif
#if defined(__LINUX)
#define ISF_OPEN     open
#define ISF_IOCTL    ioctl
#define ISF_CLOSE    close
#endif
#define DBG_ERR(fmt, args...) 	printf("%s: " fmt, __func__, ##args)
#define DBG_DUMP				printf
#define CHKPNT                  printf("\033[37mCHK: %d, %s\033[0m\r\n", __LINE__, __func__)


/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define DEV_BASE		ISF_UNIT_VDOPRC
#define DEV_COUNT		ISF_MAX_VDOPRC
#define IN_BASE		    ISF_IN_BASE
#define IN_COUNT		1
#define OUT_BASE		ISF_OUT_BASE
#define OUT_COUNT 	    16

#define HD_DEV_BASE	HD_DAL_VIDEOPROC_BASE
#define HD_DEV_MAX	HD_DAL_VIDEOPROC_MAX

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
HD_RESULT vendor_videoproc_set(UINT32 path_id, VENDOR_VIDEOPROC_PARAM_ID param_id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO   out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r = 0;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	int isf_fd = _hd_common_get_fd();

	if (isf_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}
	_HD_CONVERT_SELF_ID(self_id, rv);   if (rv != HD_OK) { return rv; }
	rv = HD_OK;
	if(ctrl_id == HD_CTRL) {
		switch(param_id) {
			case VENDOR_VIDEOPROC_PARAM_IN_DEPTH: {
				UINT32 *p_user = (UINT32 *)p_param;

				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = VDOPRC_PARAM_IN_DEPTH;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_CFG_DIS_SCALERATIO: {
				UINT32 *p_user = (UINT32 *)p_param;

				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = VDOPRC_PARAM_DIS_SCALERATIO;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_CFG_DIS_SUBSAMPLE: {
				UINT32 *p_user = (UINT32 *)p_param;

				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = VDOPRC_PARAM_DIS_SUBSAMPLE;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_PARAM_STRIP: {
				UINT32 *p_user = (UINT32 *)p_param;

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOPRC_PARAM_STRIP;
				cmd.value = *p_user;
				cmd.size = 0;

				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_PARAM_EIS_FUNC: {
				UINT32 *p_user = (UINT32 *)p_param;

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOPRC_PARAM_EIS_FUNC;
				cmd.value = *p_user;
				cmd.size = 0;

				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_PARAM_VPE_SCENE: {
				UINT32 *p_user = (UINT32 *)p_param;

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOPRC_PARAM_VPE_SCENE;
				cmd.value = *p_user;
				cmd.size = 0;

				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _VD_VPRC1;
			}
			break;
			default:
				rv = HD_ERR_PARAM;
				printf("vendor_videoproc_set not support item %d \n", param_id);
				return rv;
			break;
		}
	} else {
		switch (param_id) {
			case VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN: {
				UINT32 *p_user = (UINT32 *)p_param;

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);   if (rv != HD_OK) { return rv; }

				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = VDOPRC_PARAM_HEIGHT_ALIGN;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
	 			goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_PARAM_USER_CROP_TRIG: {
				UINT32 *p_user = (UINT32 *)p_param;

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);   if (rv != HD_OK) { return rv; }

				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = VDOPRC_PARAM_TRIG_USER_CROP;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
	 			goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_PARAM_OUT_ONEBUF_MAX: {
				UINT32 *p_user = (UINT32 *)p_param;
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);   if (rv != HD_OK) { return rv; }

				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = VDOPRC_PARAM_OUT_ONEBUF_MAX;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
	 			goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_PARAM_DIS_CROP_ALIGN: {
				UINT32 *p_user = (UINT32 *)p_param;
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);   if (rv != HD_OK) { return rv; }

				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = VDOPRC_PARAM_DIS_CROP_ALIGN;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
	 			goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_PARAM_LINEOFFSET_ALIGN: {
				UINT32 *p_user = (UINT32 *)p_param;

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);   if (rv != HD_OK) { return rv; }

                if((*p_user)%2) {
                    DBG_ERR("should multiple 2\r\n");
                    return HD_ERR_PARAM;
                }
				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = VDOPRC_PARAM_LOFF_ALIGN;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
	 			goto _VD_VPRC1;
			}
			break;
            case VENDOR_VIDEOPROC_PARAM_FUNC_CONFIG: {
				HD_VIDEOPROC_FUNC_CONFIG* p_user = (HD_VIDEOPROC_FUNC_CONFIG*)p_param;

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);   if (rv != HD_OK) { return rv; }

				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = VDOPRC_PARAM_OUT_VNDCFG_FUNC;
				cmd.value = p_user->out_func;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
	 			goto _VD_VPRC1;
			}
			break;
			case VENDOR_VIDEOPROC_PARAM_DIS_COMPENSATE: {
				UINT32 *p_user = (UINT32 *)p_param;
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_OUT_ID(out_id, rv);   if (rv != HD_OK) { return rv; }

				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = VDOPRC_PARAM_DIS_COMPENSATE;
				cmd.value = *p_user;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
	 			goto _VD_VPRC1;
			}
			break;

			default:
				rv = HD_ERR_PARAM;
				printf("vendor_videoproc_set not support item %d \n", param_id);
				return rv;
			break;
		}
	}

_VD_VPRC1:
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:  rv = HD_OK; break;
		default: rv = HD_ERR_SYS; break;
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


