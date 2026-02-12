/**
	@brief Source file of vendor media audioenc.\n

	@file vendor_audiodec.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#if defined(__LINUX)
#include <sys/ioctl.h>
#endif
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"
#include "vendor_audiodec.h"
#include "kflow_audiodec/isf_auddec.h"
#include "kflow_videodec/media_def.h"
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
#define HD_DEV_BASE HD_DAL_AUDIODEC_BASE
#define HD_DEV_MAX  HD_DAL_AUDIODEC_MAX
#define DEV_BASE        ISF_UNIT_AUDDEC
#define DEV_COUNT       ISF_MAX_AUDDEC
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       2
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        2

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
HD_RESULT vendor_audiodec_set(UINT32 id, VENDOR_AUDIODEC_ITEM item, VOID *p_param)
{
	INT32 ret;
	int isf_fd;

	HD_DAL self_id = HD_GET_DEV(id);
	//HD_IO out_id = HD_GET_OUT(id);
	HD_IO in_id = HD_GET_IN(id);
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	int r;

	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}

	switch (item) {
		case VENDOR_AUDIODEC_ITEM_CODEC_HEADER: {
			UINT32 codec_header = *(UINT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {
				return HD_ERR_IO;
			}

			_HD_CONVERT_IN_ID(in_id, ret);
			if(ret != HD_OK) {
				return ret;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			cmd.dest = ISF_PORT(self_id, in_id);
			cmd.param = AUDDEC_PARAM_ADTS_EN;
			cmd.value = codec_header;
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

		case VENDOR_AUDIODEC_ITEM_RAW_BLOCK_SIZE: {
			UINT32 block_size = *(UINT32*)p_param;

			isf_fd = _hd_common_get_fd();

			if (isf_fd <= 0) {
				return HD_ERR_UNINIT;
			}

			if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {
				return HD_ERR_IO;
			}

			_HD_CONVERT_IN_ID(in_id, ret);
			if(ret != HD_OK) {
				return ret;
			}

			_HD_CONVERT_SELF_ID(self_id, ret);
			if (ret != HD_OK) {
				return ret;
			}

			ret = HD_OK;

			cmd.dest = ISF_PORT(self_id, in_id);
			cmd.param = AUDDEC_PARAM_RAW_BLOCK_SIZE;
			cmd.value = block_size;
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
			printf("vendor_audioenc_set not support item %d \n", item);
			break;
	}

	return ret;
}

HD_RESULT vendor_audiodec_get(UINT32 id, VENDOR_AUDIODEC_ITEM item, VOID *p_param)
{
	return HD_OK;
}

