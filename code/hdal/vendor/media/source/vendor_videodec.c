/**
	@brief Source file of vendor media videodec.\n

	@file vendor_videodec.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#if defined(__LINUX)
#include <sys/ioctl.h>
#endif
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"
#include "vendor_videodec.h"
#include "kflow_videodec/isf_vdodec.h"
#include "videosprite/videosprite.h"
#include "hdal.h"

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
#define HD_VIDEODEC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE        ISF_UNIT_VDODEC
#define DEV_COUNT       1 //ISF_MAX_VDODEC
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        16
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       16

#define HD_DEV_BASE HD_DAL_VIDEODEC_BASE
#define HD_DEV_MAX  HD_DAL_VIDEODEC_MAX

#define KFLOW_VIDEODEC_PARAM_BEGIN     VDODEC_PARAM_START
#define KFLOW_VIDEODEC_PARAM_END        VDODEC_PARAM_MAX
#define KFLOW_VIDEODEC_PARAM_NUM      (KFLOW_VIDEODEC_PARAM_END - KFLOW_VIDEODEC_PARAM_BEGIN)

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/
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

#define _HD_CONVERT_IN_ID(in_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((in_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((in_id) >= HD_IN_BASE && (in_id) <= HD_IN_MAX) { \
			UINT32 id = (in_id) - HD_IN_BASE; \
			if(id < OUT_COUNT) { \
				(in_id) = IN_BASE + id; \
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
HD_RESULT vendor_videodec_set(HD_PATH_ID path_id, VENDOR_VIDEODEC_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO    in_id = HD_GET_IN(path_id);
	//HD_IO	out_id = HD_GET_OUT(path_id);
	//HD_IO ctrl_id  = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r=0;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};

	int isf_fd = _hd_common_get_fd();

	if (isf_fd <= 0) {
		return HD_ERR_UNINIT;
	}

	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}

	_HD_CONVERT_SELF_ID(self_id, rv);	if (rv != HD_OK) { return rv; }
	rv = HD_OK;

	switch (id) {
		case VENDOR_VIDEODEC_PARAM_IN_YUV_AUTO_DROP: {
			if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { return HD_ERR_IO; }
			_HD_CONVERT_IN_ID(in_id, rv); 	if (rv != HD_OK) { return rv; }

			VENDOR_VIDEODEC_YUV_AUTO_DROP *p_user = (VENDOR_VIDEODEC_YUV_AUTO_DROP*)p_param;

			cmd.dest = ISF_PORT(self_id, in_id);
			cmd.param = VDODEC_PARAM_YUV_AUTO_DROP;
			cmd.value = p_user->enable;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD1;
		}
		break;

		case VENDOR_VIDEODEC_PARAM_IN_RAWQUE_MAX_NUM: {
			if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { return HD_ERR_IO; }
			_HD_CONVERT_IN_ID(in_id, rv); 	if (rv != HD_OK) { return rv; }

			VENDOR_VIDEODEC_RAWQUE_MAX_NUM *p_user = (VENDOR_VIDEODEC_RAWQUE_MAX_NUM*)p_param;

			cmd.dest = ISF_PORT(self_id, in_id);
			cmd.param = VDODEC_PARAM_RAWQUE_MAX_NUM;
			cmd.value = p_user->rawque_max_num;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD1;
		}
		break;

		default:
			rv = HD_ERR_PARAM;
			printf("vendor_videoenc_set not support item %d \n", id);
			return rv;
	}

_VD1:
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

HD_RESULT vendor_videodec_get(HD_PATH_ID path_id, VENDOR_VIDEODEC_PARAM_ID id, VOID *p_param)
{
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r, isf_fd;

	_HD_CONVERT_SELF_ID(self_id, rv); if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_OUT_ID(out_id, rv); if(rv != HD_OK) {	return rv;}

	switch (id) {
		case VENDOR_VIDEODEC_PARAM_OUT_STATUS: {
			unsigned int *p_user = (unsigned int*)p_param;

			isf_fd = _hd_common_get_fd();

			cmd.dest = ISF_PORT(self_id, out_id);
			cmd.param = VDODEC_PARAM_DEC_STATUS;
			cmd.size = 0;

			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
			if (r == 0) {
				switch(cmd.rv) {
				case ISF_OK:
					rv = HD_OK;
					*p_user = cmd.value;
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
		}
		break;

		default:
			return rv;
	}
	return rv;
}

