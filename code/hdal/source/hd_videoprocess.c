/**
	@brief Source code of video process module.\n
	This file contains the functions which is related to video process in the chip.

	@file hd_videoprocess.c

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "hdal.h"
#define HD_MODULE_NAME HD_VIDEOPROC
#include "hd_int.h"
#include "hd_logger_p.h"
#include "kflow_videoprocess/isf_vdoprc.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/nvtmpp_ioctl.h"

//just a temp macro, waiting for hdal header updating
#define HD_VIDEOPROC_FUNC_DIRECT_SCALEUP 0x04000000 ///< enable scaling up in direct mode

/*
  1) ability
    [1] 313: videocap = CAP, videoprocess = VPE, it support 4 PHY_OUT, 0 OUT_EX
    [2] 680: videocap = SIE, videoprocess = IPL, it support 5 PHY_OUT, 11 OUT_EX (scale/rotate/crop)
*/

#define HD_VIDEOPROC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE		ISF_UNIT_VDOPRC
#define DEV_COUNT		ISF_MAX_VDOPRC
#define IN_BASE		ISF_IN_BASE
#define IN_COUNT		1
#define OUT_BASE		ISF_OUT_BASE
#define OUT_COUNT 	16
#define OUT_PHY_COUNT 5
#define OUT_MAX_DEPTH 15
#define OUT_VPE_COUNT 4

#define HD_DEV_BASE	HD_DAL_VIDEOPROC_BASE
#define HD_DEV_MAX	HD_DAL_VIDEOPROC_MAX

static UINT32 _max_dev_count = 0;
extern int nvtmpp_hdl;


#define _HD_CONVERT_SELF_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_DEV;	\
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
			if(id < IN_COUNT) { \
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


HD_RESULT _hd_videoproc_convert_dev_id(HD_DAL* p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

HD_RESULT _hd_videoproc_convert_in_id(HD_IO* p_in_id)
{
	HD_RESULT rv;
	_HD_CONVERT_IN_ID(p_in_id[0], rv);
	return rv;
}

int _hd_videoproc_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "VIDEOPROC_%d_IN_%d", dev_id - HD_DEV_BASE, in_id - HD_IN_BASE);
}

int _hd_videoproc_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "VIDEOPROC_%d_OUT_%d",  dev_id - HD_DEV_BASE, out_id - HD_OUT_BASE);
}

#if defined(__LINUX)
//#include <sys/types.h>      /* key_t, sem_t, pid_t      */
//#include <sys/shm.h>        /* shmat(), IPC_RMID        */
#include <errno.h>          /* errno, ECHILD            */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>          /* O_CREAT, O_EXEC          */

static int    g_shmid = 0;
#define SHM_KEY HD_DAL_VIDEOPROC_BASE
static sem_t *g_sem = 0;
#define SEM_NAME "sem_vprc"

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


typedef struct _HD_VP_CTX {
	HD_VIDEOPROC_DEV_CONFIG dev_cfg;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	HD_VIDEOPROC_LL_CONFIG ll_cfg;
	UINT32 dev_ddr_id;
	HD_VIDEOPROC_FUNC_CONFIG path_cfg[VDOPRC_MAX_OUT_NUM];
#endif
	HD_VIDEOPROC_CTRL ctrl;
	HD_VIDEOPROC_IN in[VDOPRC_MAX_IN_NUM];
	HD_VIDEOPROC_OUT out[VDOPRC_MAX_PHY_OUT_NUM];
	HD_VIDEOPROC_OUT_EX out_ex[VDOPRC_MAX_OUT_NUM-VDOPRC_MAX_PHY_OUT_NUM];
	HD_VIDEOPROC_CROP in_crop[VDOPRC_MAX_IN_NUM];
	HD_VIDEOPROC_CROP out_crop[VDOPRC_MAX_OUT_NUM];
	HD_VIDEOPROC_CROP in_crop_psr[VDOPRC_MAX_VPE_OUT_NUM];
	HD_VIDEOPROC_CROP out_crop_psr[VDOPRC_MAX_VPE_OUT_NUM];
	BOOL b_dev_cfg_set;
} HD_VP_CTX;

static UINT32 _hd_vprc_csz = 0;  //context buffer size
static HD_VP_CTX* _hd_vprc_ctx = NULL; //context buffer addr

void _hd_videoproc_cfg_max(UINT32 maxdevice)
{
	if (!_hd_vprc_ctx) {
		_max_dev_count = maxdevice;
		if (_max_dev_count > VDOPRC_MAX_NUM) {
			DBG_WRN("dts max_device=%lu is larger than built-in max_device=%lu!\r\n", _max_dev_count, (UINT32)VDOPRC_MAX_NUM);
			_max_dev_count = VDOPRC_MAX_NUM;
		}
	}
}

static UINT8 g_cfg_cpu = 0;
static UINT8 g_cfg_debug = 0;
static UINT8 g_cfg_vprc = 0;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
static UINT8 g_cfg_ai = 0;
#endif
static UINT32 g_max_out = VDOPRC_MAX_OUT_NUM;

UINT32 _hd_videoproc_get_max_out(void)
{
	return g_max_out;
}


HD_RESULT hd_videoproc_init(VOID)
{
	UINT32 cfg, cfg1, cfg2;
	HD_RESULT rv;
	UINT32 i;

	hdal_flow_log_p("hd_videoproc_init\n");

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

	g_cfg_cpu = (cfg & HD_COMMON_CFG_CPU);
	g_cfg_debug = ((cfg & HD_COMMON_CFG_DEBUG) >> 12);
	g_cfg_vprc = ((cfg1 & HD_VIDEOPROC_CFG) >> 16);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	g_cfg_ai = ((cfg2 & VENDOR_AI_CFG) >> 16);
#endif

	} else {

	g_cfg_cpu =  0;
	g_cfg_debug = 0;
	g_cfg_vprc = 0;
#if defined(_BSP_NA51055_)
	g_cfg_ai = 0;
#endif

	}

	if (_hd_vprc_ctx != NULL) {
		return HD_ERR_UNINIT;
	}

	if (_max_dev_count == 0) {
		DBG_ERR("_max_dev_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	_hd_vprc_csz = sizeof(HD_VP_CTX)*_max_dev_count;
	_hd_vprc_ctx = (HD_VP_CTX*)malloc_ctx(_hd_vprc_csz);
	if (_hd_vprc_ctx == NULL) {
		DBG_ERR("cannot alloc heap for _max_dev_count =%d\r\n", (int)_max_dev_count);
		return HD_ERR_HEAP;
	}

	if (_hd_common_get_pid() == 0) {

		lock_ctx();
		for(i = 0; i < _max_dev_count; i++) {
			memset(&(_hd_vprc_ctx[i]), 0, sizeof(HD_VP_CTX));
		}
		unlock_ctx();

	}

	//init drv
	if (_hd_common_get_pid() == 0) {
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		UINT8 ai_enable = 0;

		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = _max_dev_count;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		if (g_cfg_ai & (VENDOR_AI_CFG_ENABLE_CNN >> 16)) { //if enable ai CNN2
			ai_enable = 1;
#if 0 //check the rules with the VENDOR_VIDEOPROC_PARAM_STRIP setting, so move to _vdoprc_set_init
			UINT32 stripe_rule = g_cfg_vprc & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
			if (stripe_rule > 3) {
				stripe_rule = 0;
			}
			if ((stripe_rule == 0)||(stripe_rule == 3)) { //NOT disable GDC?
				//DBG_DUMP("already enable CNN => current vprc config stripe_rule = LV%lu, enforce to LV2!", stripe_rule+1);
				g_cfg_vprc |= (HD_VIDEOPROC_CFG_STRIP_LV2 >> 16); //disable GDC for AI
			} else {
				//DBG_DUMP("already enable CNN => current vprc config stripe_rule = LV%lu", stripe_rule+1);
			}
#endif
		}
#endif
		isf_dev_cmd.p2 = (g_cfg_cpu << 28) | (g_cfg_debug << 24) | (ai_enable << 16) | (g_cfg_vprc);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("init fail? %d\r\n", r);
			return r;
		}
	}

	//query sys caps
	{
		ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
		VDOPRC_SYS_CAPS caps = {0};
		HD_RESULT rv;
		int r;

		cmd.dest = ISF_PORT(DEV_BASE+0, ISF_CTRL);
		cmd.param = VDOPRC_PARAM_SYS_CAPS;
		cmd.value = (UINT32)&caps;
		cmd.size = sizeof(VDOPRC_SYS_CAPS);

		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				rv = HD_OK;
				//event_mask = cmd.value;
				break;
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

		if (rv == HD_OK) {
			g_max_out = caps.max_out_count;
		}
	}
	return HD_OK;
}

HD_RESULT hd_videoproc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoproc_bind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) dst(0x%x) in(%d)\n", self_id, out_id, dest_id, in_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_vprc_ctx == NULL) {
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

HD_RESULT hd_videoproc_unbind(HD_OUT_ID _out_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoproc_unbind:\n");
	hdal_flow_log_p("    self(0x%x) out(%d)\n", self_id, out_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_vprc_ctx == NULL) {
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

UINT32 _hd_videoproc_get_3dnr_ref(HD_PATH_ID path_id)
{
	HD_RESULT rv = HD_ERR_NG;
	HD_IO out_id = HD_GET_OUT(path_id);
	_HD_CONVERT_OUT_ID(out_id, rv);
	if (path_id == 0) return 0;
	if (rv != HD_OK) return 0xffffffff;
	return (out_id - ISF_OUT_BASE);
}

UINT32 _hd_videoproc_get_src_out(HD_PATH_ID path_id)
{
	HD_RESULT rv = HD_ERR_NG;
	HD_IO out_id = HD_GET_OUT(path_id);
	_HD_CONVERT_OUT_ID(out_id, rv);
	if (path_id == 0) return 0;
	if (rv != HD_OK) return 0xffffffff;
	return (out_id - ISF_OUT_BASE);
}

HD_RESULT _hd_videoproc_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
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
			default: break;
			}
		} else {
			rv = HD_ERR_SYS;
		}
	}
	return rv;
}


HD_RESULT _hd_videoproc_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id)
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
			default: break;
			}
		} else {
			rv = HD_ERR_SYS;
		}
	}
	return rv;
}

HD_RESULT _hd_videoproc_get_state(HD_OUT_ID _out_id, UINT32* p_state)
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
			default: break;
			}
		} else {
			rv = HD_ERR_SYS;
		}
	}
	return rv;
}

HD_RESULT hd_videoproc_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoproc_open:\n");
	hdal_flow_log_p("    self(0x%x) out(%d) in(%d) ", self_id, out_id, in_id);

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
	if (_hd_vprc_ctx == NULL) {
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
			hdal_flow_log_p("path_id(N/A)\n"); return rv;
		}
		p_path_id[0] = _out_id;
		hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);
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
				DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
				hdal_flow_log_p("path_id(N/A)\n"); return rv;
			}
			_HD_CONVERT_OUT_ID(out_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("out_id=%lu is larger than max=%lu!\r\n", (UINT32)((out_id) - HD_OUT_BASE), (UINT32)OUT_COUNT);
				hdal_flow_log_p("path_id(N/A)\n"); return rv;
			}
			p_path_id[0] = HD_VIDEOPROC_PATH(cur_dev, in_id, path_out_id); //osg_id + out_id
			return _hd_osg_attach(self_id, out_id, osg_in_id);
		} else if(osg_out_id) {
			UINT32 cur_dev = in_dev_id;
			HD_IO path_in_id = in_id;
			_HD_CONVERT_SELF_ID(in_dev_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((in_dev_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
				hdal_flow_log_p("path_id(N/A)\n"); return rv;
			}
			_HD_CONVERT_IN_ID(in_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("in_id=%lu is larger than max=%lu!\r\n", (UINT32)((in_id) - HD_IN_BASE), (UINT32)IN_COUNT);
				hdal_flow_log_p("path_id(N/A)\n"); return rv;
			}
			p_path_id[0] = HD_VIDEOPROC_PATH(cur_dev, path_in_id, out_id); //in_id + osg_id
			return _hd_osg_attach(in_dev_id, in_id, osg_out_id);
		} else {
			UINT32 cur_dev = self_id;
			HD_IO check_in_id = in_id;
			HD_IO check_out_id = out_id;
			_HD_CONVERT_SELF_ID(self_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
				hdal_flow_log_p("path_id(N/A)\n"); return rv;
			}
			_HD_CONVERT_OUT_ID(check_out_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("out_id=%lu is larger than max=%lu!\r\n", (UINT32)((check_out_id) - HD_OUT_BASE), (UINT32)OUT_COUNT);
				hdal_flow_log_p("path_id(N/A)\n"); return rv;
			}
			_HD_CONVERT_SELF_ID(in_dev_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((in_dev_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
				hdal_flow_log_p("path_id(N/A)\n"); return rv;
			}
			_HD_CONVERT_IN_ID(check_in_id, rv);
			if(rv != HD_OK) {
				DBG_ERR("in_id=%lu is larger than max=%lu!\r\n", (UINT32)((check_in_id) - HD_IN_BASE), (UINT32)IN_COUNT);
				hdal_flow_log_p("path_id(N/A)\n"); return rv;
			}
			if(in_dev_id != self_id) {
				DBG_ERR("dev_id of in/out is not the same!\r\n");
				hdal_flow_log_p("path_id(N/A)\n"); return HD_ERR_DEV;
			}
			p_path_id[0] = HD_VIDEOPROC_PATH(cur_dev, in_id, out_id); //in_id + out_id
		}
	}

	hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);

	//--- check if DEV_CONFIG info had been set ---
	lock_ctx();

	//if (!_hd_common_is_new_flow()) {
	{
		UINT32 did = self_id - ISF_UNIT_VDOPRC;
		if (_hd_vprc_ctx[did].b_dev_cfg_set == FALSE) {
			DBG_ERR("Error: please set HD_VIDEOPROC_PARAM_DEV_CONFIG first !!!\r\n");
			unlock_ctx();
			return HD_ERR_NO_CONFIG;
		}
	}
	//}

	unlock_ctx();

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
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
	return rv;
}

HD_RESULT hd_videoproc_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoproc_start:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_vprc_ctx == NULL) {
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
	return rv;
}

HD_RESULT hd_videoproc_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoproc_stop:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_vprc_ctx == NULL) {
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
	return rv;
}

HD_RESULT hd_videoproc_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoproc_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoproc_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoproc_close:\n");
	hdal_flow_log_p("    path_id(0x%x)\n", path_id);

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_vprc_ctx == NULL) {
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

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
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
	return rv;
}

INT _hd_videoproc_param_cvt_name(HD_VIDEOENC_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
		case HD_VIDEOPROC_PARAM_DEVCOUNT:            snprintf(p_ret_string, max_str_len, "DEVCOUNT");           break;
		case HD_VIDEOPROC_PARAM_SYSCAPS:             snprintf(p_ret_string, max_str_len, "SYSCAPS");            break;
		case HD_VIDEOPROC_PARAM_SYSINFO:             snprintf(p_ret_string, max_str_len, "SYSINFO");            break;
		case HD_VIDEOPROC_PARAM_DEV_CONFIG:          snprintf(p_ret_string, max_str_len, "DEV_CONFIG");         break;
		case HD_VIDEOPROC_PARAM_CTRL:                snprintf(p_ret_string, max_str_len, "CTRL");               break;
		case HD_VIDEOPROC_PARAM_IN:                  snprintf(p_ret_string, max_str_len, "IN");                 break;
		case HD_VIDEOPROC_PARAM_IN_FRC:              snprintf(p_ret_string, max_str_len, "IN_FRC");             break;
		case HD_VIDEOPROC_PARAM_IN_CROP:             snprintf(p_ret_string, max_str_len, "IN_CROP");            break;
		case HD_VIDEOPROC_PARAM_IN_CROP_PSR:         snprintf(p_ret_string, max_str_len, "IN_CROP_PSR");        break;
		case HD_VIDEOPROC_PARAM_OUT:                 snprintf(p_ret_string, max_str_len, "OUT");                break;
		case HD_VIDEOPROC_PARAM_OUT_FRC:             snprintf(p_ret_string, max_str_len, "OUT_FRC");            break;
		case HD_VIDEOPROC_PARAM_OUT_CROP:            snprintf(p_ret_string, max_str_len, "OUT_CROP");           break;
		case HD_VIDEOPROC_PARAM_OUT_CROP_PSR:        snprintf(p_ret_string, max_str_len, "OUT_CROP_PSR");       break;
		case HD_VIDEOPROC_PARAM_OUT_EX:              snprintf(p_ret_string, max_str_len, "OUT_EX");             break;
		case HD_VIDEOPROC_PARAM_OUT_EX_CROP:         snprintf(p_ret_string, max_str_len, "OUT_EX_CROP");        break;
		case HD_VIDEOPROC_PARAM_IN_STAMP_BUF:        snprintf(p_ret_string, max_str_len, "IN_STAMP_BUF");       break;
		case HD_VIDEOPROC_PARAM_IN_STAMP_IMG:        snprintf(p_ret_string, max_str_len, "IN_STAMP_IMG");       break;
		case HD_VIDEOPROC_PARAM_IN_STAMP_ATTR:       snprintf(p_ret_string, max_str_len, "IN_STAMP_ATTR");      break;
		case HD_VIDEOPROC_PARAM_IN_MASK_ATTR:        snprintf(p_ret_string, max_str_len, "IN_MASK_ATTR");       break;
		case HD_VIDEOPROC_PARAM_IN_MOSAIC_ATTR:      snprintf(p_ret_string, max_str_len, "IN_MOSAIC_ATTR");     break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		case HD_VIDEOPROC_PARAM_LL_CONFIG:           snprintf(p_ret_string, max_str_len, "LL_CONFIG");          break;
		case HD_VIDEOPROC_PARAM_FUNC_CONFIG:         snprintf(p_ret_string, max_str_len, "FUNC_CONFIG");        break;
#endif
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", id);
			return (-1);
	}
	return 0;
}

HD_RESULT hd_videoproc_get(HD_PATH_ID path_id, HD_VIDEOPROC_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (_hd_vprc_ctx == NULL) {
		return HD_ERR_UNINIT;
	}
	if (p_param == 0) {
		return HD_ERR_NULL_PTR;
	}

	{
		CHAR  param_name[20];
		_hd_videoproc_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videoproc_get(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	{
		HD_IO osg_in_id = in_id, osg_out_id = out_id;
		_HD_CONVERT_OSG_ID(osg_in_id);
		_HD_CONVERT_OSG_ID(osg_out_id);
		if(osg_in_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_get(self_id, out_id, osg_in_id, id, p_param);
		} else if(osg_out_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_get(self_id, in_id, osg_out_id, id, p_param);
		}
	}

	{
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		rv = HD_OK;

		lock_ctx();

		if(ctrl_id == HD_CTRL) {
			switch(id) {
			case HD_VIDEOPROC_PARAM_DEVCOUNT:
			{
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
				p_user->max_dev_count = _max_dev_count;

				hdal_flow_log_p("max_dev_count(%u)\n", p_user->max_dev_count);

				rv = HD_OK;
				goto _HD_VP2;
			}
			break;
			case HD_VIDEOPROC_PARAM_DEV_CONFIG:
			{
				HD_VIDEOPROC_DEV_CONFIG* p_user = (HD_VIDEOPROC_DEV_CONFIG*)p_param;
				UINT32 did = self_id - ISF_UNIT_VDOPRC;
				p_user[0] = _hd_vprc_ctx[did].dev_cfg;

				hdal_flow_log_p("pipe(0x%08x) isp_id(%u)\n", p_user->pipe, p_user->isp_id);
				hdal_flow_log_p("    ctrl_max func(0x%08x) ref_path_3dnr(0x%08x)\n", p_user->ctrl_max.func, p_user->ctrl_max.ref_path_3dnr);
				hdal_flow_log_p("    in_max dim(%ux%u) pxlfmt(0x%08x)\n", p_user->in_max.dim.w, p_user->in_max.dim.h, p_user->in_max.pxlfmt);

				rv = HD_OK;
				goto _HD_VP2;
			}
			break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			case HD_VIDEOPROC_PARAM_LL_CONFIG:
			{
				HD_VIDEOPROC_LL_CONFIG* p_user = (HD_VIDEOPROC_LL_CONFIG*)p_param;
				UINT32 did = self_id - ISF_UNIT_VDOPRC;
				p_user[0] = _hd_vprc_ctx[did].ll_cfg;

				hdal_flow_log_p("delay_trig_lowlatency(0x%08x)\n", p_user->delay_trig_lowlatency);

				rv = HD_OK;
				goto _HD_VP2;
			}
			break;
			case HD_VIDEOPROC_PARAM_FUNC_CONFIG:
			{
				HD_VIDEOPROC_FUNC_CONFIG* p_user = (HD_VIDEOPROC_FUNC_CONFIG*)p_param;
				UINT32 did = self_id - ISF_UNIT_VDOPRC;
				p_user->in_func = _hd_vprc_ctx[did].path_cfg[0].in_func;
				p_user->ddr_id = _hd_vprc_ctx[did].dev_ddr_id;

				hdal_flow_log_p("dev_cfg_func(0x%08x) ddr_id(%lu)\n", p_user->in_func, p_user->ddr_id);

				rv = HD_OK;
				goto _HD_VP2;
			}
			break;
#endif
			case HD_VIDEOPROC_PARAM_SYSCAPS:
			{
				HD_VIDEOPROC_SYSCAPS* p_user = (HD_VIDEOPROC_SYSCAPS*)p_param;
				int  i;
				memset(p_user, 0, sizeof(HD_VIDEOPROC_SYSCAPS));
				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;
				p_user->chip_id = 0;
				p_user->max_in_count = VDOPRC_MAX_IN_NUM;
				p_user->max_out_count = VDOPRC_MAX_OUT_NUM;  //(0~4 are physical out, 5~15 are extend out)
#if defined(_BSP_NA51000_)
				p_user->dev_caps =
					HD_CAPS_DEVCONFIG
					 | HD_VIDEOPROC_DEVCAPS_3DNR
					 | HD_VIDEOPROC_DEVCAPS_WDR
					 | HD_VIDEOPROC_DEVCAPS_SHDR
					 | HD_VIDEOPROC_DEVCAPS_DEFOG
					 | HD_VIDEOPROC_DEVCAPS_MOSAIC
					 | HD_VIDEOPROC_DEVCAPS_COLORNR;
#endif
#if defined(_BSP_NA51055_)
				p_user->dev_caps =
					HD_CAPS_DEVCONFIG
					 | HD_VIDEOPROC_DEVCAPS_3DNR
					 | HD_VIDEOPROC_DEVCAPS_WDR
					 | HD_VIDEOPROC_DEVCAPS_SHDR
					 | HD_VIDEOPROC_DEVCAPS_DEFOG
					 | HD_VIDEOPROC_DEVCAPS_MOSAIC
					 | HD_VIDEOPROC_DEVCAPS_COLORNR
					 | HD_VIDEOPROC_DEVCAPS_AF;
#endif
#if defined(_BSP_NA51089_)
				p_user->dev_caps =
					HD_CAPS_DEVCONFIG
					 | HD_VIDEOPROC_DEVCAPS_3DNR
					 | HD_VIDEOPROC_DEVCAPS_WDR
					 | HD_VIDEOPROC_DEVCAPS_SHDR
					 | HD_VIDEOPROC_DEVCAPS_MOSAIC
					 | HD_VIDEOPROC_DEVCAPS_COLORNR
					 | HD_VIDEOPROC_DEVCAPS_AF;
#endif
				p_user->in_caps[0] =
					HD_VIDEO_CAPS_AUTO_SYNC
					 | HD_VIDEO_CAPS_RAW
					 | HD_VIDEO_CAPS_COMPRESS
					 | HD_VIDEO_CAPS_YUV420
					 | HD_VIDEO_CAPS_YUV422
					 | HD_VIDEO_CAPS_CROP_WIN
					 | HD_VIDEO_CAPS_CROP_AUTO
					 | HD_VIDEO_CAPS_DIR_MIRRORX
					 | HD_VIDEO_CAPS_DIR_MIRRORY
					 | HD_VIDEO_CAPS_STAMP
					 | HD_VIDEO_CAPS_MASK;
				for (i=0; i<VDOPRC_MAX_OUT_NUM; i++) p_user->out_caps[i] =
					HD_VIDEO_CAPS_AUTO_SYNC
					 | HD_VIDEO_CAPS_COMPRESS
					 | HD_VIDEO_CAPS_YUV400
					 | HD_VIDEO_CAPS_YUV420
					 | HD_VIDEO_CAPS_YUV422
					 | HD_VIDEO_CAPS_CROP_WIN
					 | HD_VIDEO_CAPS_SCALE_UP
					 | HD_VIDEO_CAPS_SCALE_DOWN
					 | HD_VIDEO_CAPS_FRC_DOWN
					 | HD_VIDEOPROC_OUTCAPS_MD
					 | HD_VIDEOPROC_OUTCAPS_DIS;
				p_user->max_w_scaleup_ratio = 32; //for extend out, it is 16
				p_user->max_w_scaledown_ratio =  32; //for extend out, it is 16
				p_user->max_h_scaleup_ratio =  16;
				p_user->max_h_scaledown_ratio =  16;
				p_user->max_in_stamp =  4;
				p_user->max_in_stamp_ex =  0;
				p_user->max_in_mask =  8;
				p_user->max_in_mask_ex =  0;

				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip_id, p_user->max_in_count, p_user->max_out_count, p_user->dev_caps);
				hdal_flow_log_p("    max_w_scaleup(%u) max_w_scaledown(%u) max_h_scaleup(%u) max_h_scaledown(%u)\n", p_user->max_w_scaleup_ratio, p_user->max_w_scaledown_ratio, p_user->max_h_scaleup_ratio, p_user->max_h_scaledown_ratio);
				hdal_flow_log_p("    max_in_stamp(%u) max_in_stamp_ex(%u) max_in_mask(%u) max_in_mask_ex(%u)\n", p_user->max_in_stamp, p_user->max_in_stamp_ex, p_user->max_in_mask, p_user->max_in_mask_ex);
				{
					int i;
					hdal_flow_log_p("         in_caps    , out_caps =>\n");
					for (i=0; i<16; i++) {
						hdal_flow_log_p("    [%02d] 0x%08x , 0x%08x\n", i, p_user->in_caps[0], p_user->out_caps[i]);
					}
				}

				rv = HD_OK;
				goto _HD_VP2;
			}
			break;
			case HD_VIDEOPROC_PARAM_SYSINFO:
			{
				HD_VIDEOPROC_SYSINFO* p_user = (HD_VIDEOPROC_SYSINFO*)p_param;
				int  i;
				memset(p_user, 0, sizeof(HD_VIDEOPROC_SYSINFO));
				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;
				for (i=0; i<VDOPRC_MAX_IN_NUM; i++) p_user->cur_in_fps[i] = 0;
				for (i=0; i<VDOPRC_MAX_OUT_NUM; i++) p_user->cur_out_fps[i] = 0;
				rv = HD_OK;
				/*
				ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
				int r;
				if(rv != HD_OK) {
					goto _HD_VP2;
				}
				cmd.param = id;
				cmd.value = (UINT32)p_param;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
				if (r == 0) {
					switch(cmd.rv) {
					case ISF_OK: rv = HD_OK; break;
					default: rv = HD_ERR_SYS; break;
					}
				} else {
					rv = HD_ERR_SYS;
				}*/
				goto _HD_VP2;
			}
			break;
			case HD_VIDEOPROC_PARAM_CTRL:
			{
				HD_VIDEOPROC_CTRL* p_user = (HD_VIDEOPROC_CTRL*)p_param;
				UINT32 did = self_id - ISF_UNIT_VDOPRC;
				p_user[0] = _hd_vprc_ctx[did].ctrl;
				hdal_flow_log_p("func(0x%08x) ref_path_3dnr(0x%08x)\n", p_user->func, p_user->ref_path_3dnr);
				rv = HD_OK;
				goto _HD_VP2;
			}
			break;
			default:
				rv = HD_ERR_PARAM;
				goto _HD_VP2;
				break;
			}
		} else {
			switch(id) {
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			case HD_VIDEOPROC_PARAM_FUNC_CONFIG:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP2;}
				//if((out_id - OUT_BASE) >= OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP2;} //only support phy out
				{
					HD_VIDEOPROC_FUNC_CONFIG* p_user = (HD_VIDEOPROC_FUNC_CONFIG*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					p_user->in_func= _hd_vprc_ctx[did].path_cfg[0].in_func;
					p_user->out_func = _hd_vprc_ctx[did].path_cfg[oid].out_func;
					p_user->ddr_id = _hd_vprc_ctx[did].path_cfg[oid].ddr_id;
					p_user->out_order = _hd_vprc_ctx[did].path_cfg[oid].out_order;
					hdal_flow_log_p("out_cfg_func(0x%08x) ddr_id(%lu)\n", p_user->out_func, p_user->ddr_id);

					rv = HD_OK;
					goto _HD_VP2;
				}
				break;
#endif
			case HD_VIDEOPROC_PARAM_IN:
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VP2; }
				{
					HD_VIDEOPROC_IN* p_user = (HD_VIDEOPROC_IN*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 iid = in_id - ISF_IN_BASE;
					p_user[0] = _hd_vprc_ctx[did].in[iid];

					hdal_flow_log_p("dim(%ux%u) pxlfmt(0x%08x) dir(0x%08x)\n", p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->dir);

					rv = HD_OK;
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_IN_FRC:
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VP2;}
				{
					HD_VIDEOPROC_FRC* p_user = (HD_VIDEOPROC_FRC*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 iid = in_id - ISF_IN_BASE;
					p_user[0].frc = _hd_vprc_ctx[did].in[iid].frc;

					hdal_flow_log_p("frc(%u/%u)\n", GET_HI_UINT16(p_user->frc), GET_LO_UINT16(p_user->frc));

					rv = HD_OK;
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_IN_CROP:
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VP2;}
				{
					HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 iid = in_id - ISF_IN_BASE;
					p_user[0] = _hd_vprc_ctx[did].in_crop[iid];
					rv = HD_OK;

					hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_OUT:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP2;}
				if((out_id - OUT_BASE) >= OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP2;} //only support phy out
				{
					HD_VIDEOPROC_OUT* p_user = (HD_VIDEOPROC_OUT*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					p_user[0] = _hd_vprc_ctx[did].out[oid];
					rv = HD_OK;

					hdal_flow_log_p("dim(%ux%u) pxlfmt(0x%08x) dir(0x%08x) frc(%u/%u) func(%08x) depth(%d)\n", p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->dir, GET_HI_UINT16(p_user->frc), GET_LO_UINT16(p_user->frc), p_user->func, p_user->depth);
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_OUT_FRC:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP2;}
				//if((out_id - OUT_BASE) >= OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP2;} //only support phy out
				{
					HD_VIDEOPROC_FRC* p_user = (HD_VIDEOPROC_FRC*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					p_user[0].frc = _hd_vprc_ctx[did].out[oid].frc;
					rv = HD_OK;

					hdal_flow_log_p("frc(%u/%u)\n", GET_HI_UINT16(p_user->frc), GET_LO_UINT16(p_user->frc));
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_OUT_CROP:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP2;}
				if((out_id - OUT_BASE) >= OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP2;} //only support phy out
				{
					HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					p_user[0] = _hd_vprc_ctx[did].out_crop[oid];
					rv = HD_OK;

					hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_OUT_EX:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)){ rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP2;}
				if((out_id - OUT_BASE) < OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP2;} //only support ext out
				{
					HD_VIDEOPROC_OUT_EX* p_user = (HD_VIDEOPROC_OUT_EX*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					p_user[0] = _hd_vprc_ctx[did].out_ex[oid-OUT_PHY_COUNT];
					rv = HD_OK;

					hdal_flow_log_p("dim(%ux%u) pxlfmt(0x%08x) dir(0x%08x) frc(%u/%u) src_path(%08x) depth(%d)\n", p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->dir, GET_HI_UINT16(p_user->frc), GET_LO_UINT16(p_user->frc), p_user->src_path, p_user->depth);
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_OUT_EX_CROP:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)){ rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP2;}
				if((out_id - OUT_BASE) < OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP2;} //only support ext out
				{
					HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					p_user[0] = _hd_vprc_ctx[did].out_crop[oid];
					rv = HD_OK;

					hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_IN_CROP_PSR:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)){ rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP2;}
				if((out_id - OUT_BASE) >= OUT_VPE_COUNT) { rv = HD_ERR_IO; goto _HD_VP2;} //only support vpe out
				{
					HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					p_user[0] = _hd_vprc_ctx[did].in_crop_psr[oid];
					rv = HD_OK;

					hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);
					goto _HD_VP2;
				}
				break;
			case HD_VIDEOPROC_PARAM_OUT_CROP_PSR:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)){ rv = HD_ERR_IO; goto _HD_VP2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP2;}
				if((out_id - OUT_BASE) >= OUT_VPE_COUNT) { rv = HD_ERR_IO; goto _HD_VP2;} //only support vpe out
				{
					HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					p_user[0] = _hd_vprc_ctx[did].out_crop_psr[oid];
					rv = HD_OK;

					hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);
					goto _HD_VP2;
				}
				break;
			default:
				rv = HD_ERR_PARAM;
				break;
			}
		}
	}
_HD_VP2:
	unlock_ctx();
	return rv;
}

/*


			isf_unit_set_param(ISF_CTRL, VDOPRC_PARAM_IQ_ID, 0);
			isf_unit_set_param(ISF_CTRL, VDOPRC_PARAM_PIPE, VDOPRC_PIPE_ALL);
			isf_unit_set_param(ISF_CTRL, VDOPRC_PARAM_MAX_FUNC, 0);
			isf_unit_set_param(ISF_CTRL, VDOPRC_PARAM_MAX_FUNC2, 0);
			isf_unit_set_param(ISF_CTRL, VDOPRC_PARAM_FUNC, 0);
			isf_unit_set_param(ISF_CTRL, VDOPRC_PARAM_FUNC2, 0);
ISF_UNIT_PARAM_IN_VDOMAX
	ISF_VDO_MAX
			isf_unit_set_vdomaxsize(ISF_IN(0), VDO_SIZE(1920, 1080));
			isf_unit_set_vdoformat(ISF_IN(0), VDO_PXLFMT_YUV420);
ISF_UNIT_PARAM_OUT_VDOFRAME
	ISF_VDO_FRAME
			isf_unit_set_vdosize(ISF_IN(0), VDO_SIZE(1920, 1080));
			isf_unit_set_vdoformat(ISF_IN(0), VDO_PXLFMT_YUV420);
			isf_unit_set_vdodirect(ISF_IN(0), VDO_DIR_NONE);
ISF_UNIT_PARAM_OUT_VDOFPS
	ISF_VDO_FPS
			isf_unit_set_vdoframectrl(ISF_IN(0), VDO_FRC_ALL);
ISF_UNIT_PARAM_OUT_VDOWIN
	ISF_VDO_WIN
			isf_unit_set_vdowinpose(ISF_IN(0), VDO_POSE(0, 0));
			isf_unit_set_vdowinsize(ISF_IN(0), VDO_SIZE(0, 0));


*/

HD_RESULT hd_videoproc_set(HD_PATH_ID path_id, HD_VIDEOPROC_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r = 0;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};

	{
		CHAR  param_name[20];
		_hd_videoproc_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videoproc_set(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
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
	if (_hd_vprc_ctx == NULL) {
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

	{
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		rv = HD_OK;
		cmd.param = id;

		lock_ctx();

		if(ctrl_id == HD_CTRL) {
			switch(id) {
			case HD_VIDEOPROC_PARAM_DEV_CONFIG:
			{
				HD_VIDEOPROC_DEV_CONFIG* p_user = (HD_VIDEOPROC_DEV_CONFIG*)p_param;
				ISF_VDO_MAX max = {0};
				#if _TODO
				ISF_SET set[4] = {0};
				#else
				ISF_SET set[3] = {0};
				#endif

				hdal_flow_log_p("pipe(0x%08x) isp_id(%u)\n", p_user->pipe, p_user->isp_id);
				hdal_flow_log_p("    ctrl_max func(0x%08x) ref_path_3dnr(0x%08x)\n", p_user->ctrl_max.func, p_user->ctrl_max.ref_path_3dnr);
				hdal_flow_log_p("    in_max dim(%ux%u) pxlfmt(0x%08x)\n", p_user->in_max.dim.w, p_user->in_max.dim.h, p_user->in_max.pxlfmt);
				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					_hd_vprc_ctx[did].dev_cfg = p_user[0];
					_hd_vprc_ctx[did].b_dev_cfg_set = TRUE;
				}

				/*
				ISF_SET set[2] = {0};
				set[0].param = VDOPRC_PARAM_FUNC;	set[0].value = p_user->func;
				set[1].param = VDOPRC_PARAM_FUNC2;	set[1].value = p_user->func;
				*/
				/*
				p_user->in_max.frc;
				*/
				max.max_frame = 0; //keep 0 for using default
				max.max_imgfmt = p_user->in_max.pxlfmt;
				max.max_imgsize.w = p_user->in_max.dim.w;
				max.max_imgsize.h = p_user->in_max.dim.h;
				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = ISF_UNIT_PARAM_VDOMAX;
				cmd.value = (UINT32)&max;
				cmd.size = sizeof(ISF_VDO_MAX);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("set %d => r=%d,rv=%08x\r\n", id, r, cmd.rv);
					goto _HD_VP1;
				}

				set[0].param = VDOPRC_PARAM_PIPE;	set[0].value = p_user->pipe;
				set[1].param = VDOPRC_PARAM_IQ_ID;	set[1].value = p_user->isp_id;
				set[2].param = VDOPRC_PARAM_MAX_FUNC;
				if(p_user->ctrl_max.func & HD_VIDEOPROC_FUNC_3DNR)
					set[2].value |= VDOPRC_FUNC_3DNR;
				if(p_user->ctrl_max.func & HD_VIDEOPROC_FUNC_WDR)
					set[2].value |= VDOPRC_FUNC_WDR;
				if(p_user->ctrl_max.func & HD_VIDEOPROC_FUNC_SHDR)
					set[2].value |= VDOPRC_FUNC_SHDR;
				if(p_user->ctrl_max.func & HD_VIDEOPROC_FUNC_DEFOG)
					set[2].value |= VDOPRC_FUNC_DEFOG;
				if(p_user->ctrl_max.func & HD_VIDEOPROC_FUNC_MOSAIC) {
					set[2].value |= VDOPRC_FUNC_PM_PIXELIZTION;
					set[2].value |= VDOPRC_FUNC_YUV_SUBOUT;
				}
				if(p_user->ctrl_max.func & HD_VIDEOPROC_FUNC_COLORNR) {
					set[2].value |= VDOPRC_FUNC_YUV_SUBOUT;
				}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
				if(p_user->ctrl_max.func & HD_VIDEOPROC_FUNC_AF) {
					set[2].value |= VDOPRC_FUNC_VA_SUBOUT;
				}
				if(p_user->ctrl_max.func & HD_VIDEOPROC_FUNC_3DNR_STA) {
					set[2].value |= VDOPRC_FUNC_3DNR_STA;
				}
#endif

				#if _TODO
				set[3].param = VDOPRC_PARAM_MAX_FUNC2;	set[3].value = p_user->in_max.func;
				#endif
				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				#if _TODO
				cmd.size = sizeof(ISF_SET)*4;
				#else
				cmd.size = sizeof(ISF_SET)*3;
				#endif
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("set %d => r=%d,rv=%08x\r\n", id, r, cmd.rv);
					goto _HD_VP1;
				}

			}
			break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			case HD_VIDEOPROC_PARAM_FUNC_CONFIG:
			{
				HD_VIDEOPROC_FUNC_CONFIG* p_user = (HD_VIDEOPROC_FUNC_CONFIG*)p_param;
				ISF_SET set[1] = {0};
				ISF_ATTR t_attr;

				hdal_flow_log_p("in_cfg_func(0x%08x) ddr_id(%lu)\n", p_user->in_func, p_user->ddr_id);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					_hd_vprc_ctx[did].path_cfg[0].in_func = p_user->in_func;
					_hd_vprc_ctx[did].dev_ddr_id = p_user->ddr_id;
				}

				t_attr.attr = p_user->ddr_id;
				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = ISF_UNIT_PARAM_ATTR;
				cmd.value = (UINT32)&t_attr;
				cmd.size = sizeof(ISF_ATTR);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;

				set[0].param = VDOPRC_PARAM_IN_CFG_FUNC;
				set[0].value = p_user->in_func;
				cmd.dest = ISF_PORT(self_id, ISF_IN(0));
				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;
			}
			break;
#endif
			case HD_VIDEOPROC_PARAM_CTRL:
			{
				HD_VIDEOPROC_CTRL* p_user = (HD_VIDEOPROC_CTRL*)p_param;
				ISF_SET set[2] = {0};

				hdal_flow_log_p("func(0x%08x) ref_path_3dnr(0x%08x)\n", p_user->func, p_user->ref_path_3dnr);
				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					_hd_vprc_ctx[did].ctrl = p_user[0];
				}

				set[0].param = VDOPRC_PARAM_FUNC;
				set[0].value = 0;
				if(p_user->func & HD_VIDEOPROC_FUNC_3DNR)
					set[0].value |= VDOPRC_FUNC_3DNR;
				if(p_user->func & HD_VIDEOPROC_FUNC_WDR)
					set[0].value |= VDOPRC_FUNC_WDR;
				if(p_user->func & HD_VIDEOPROC_FUNC_SHDR)
					set[0].value |= VDOPRC_FUNC_SHDR;
				if(p_user->func & HD_VIDEOPROC_FUNC_DEFOG)
					set[0].value |= VDOPRC_FUNC_DEFOG;
				if(p_user->func & HD_VIDEOPROC_FUNC_MOSAIC) {
					set[0].value |= VDOPRC_FUNC_PM_PIXELIZTION;
					set[0].value |= VDOPRC_FUNC_YUV_SUBOUT;
				}
				if(p_user->func & HD_VIDEOPROC_FUNC_COLORNR) {
					set[0].value |= VDOPRC_FUNC_YUV_SUBOUT;
				}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
				if(p_user->func & HD_VIDEOPROC_FUNC_AF) {
					set[0].value |= VDOPRC_FUNC_VA_SUBOUT;
				}
				if(p_user->func & HD_VIDEOPROC_FUNC_3DNR_STA) {
					set[0].value |= VDOPRC_FUNC_3DNR_STA;
				}
#endif
				if(p_user->func & HD_VIDEOPROC_FUNC_DIRECT_SCALEUP) {
					set[0].value |= VDOPRC_FUNC_DIRECT_SCALEUP;
				}

				set[1].param = VDOPRC_PARAM_3DNR_REFPATH;
				set[1].value = 0;
				if(p_user->func & HD_VIDEOPROC_FUNC_3DNR) {
					HD_IO out_id = HD_GET_OUT(p_user->ref_path_3dnr);
					if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
					set[1].value = (out_id - HD_OUT_BASE);
				}

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*2;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("set %d => r=%d,rv=%08x\r\n", id, r, cmd.rv);
					goto _HD_VP1;
				}
			}
			break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			case HD_VIDEOPROC_PARAM_LL_CONFIG:
			{
				HD_VIDEOPROC_LL_CONFIG* p_user = (HD_VIDEOPROC_LL_CONFIG*)p_param;
				ISF_SET set[1] = {0};
				hdal_flow_log_p("delay_trig_lowlatency(0x%08x)\n", p_user->delay_trig_lowlatency);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					_hd_vprc_ctx[did].ll_cfg = p_user[0];
				}

				set[0].param = VDOPRC_PARAM_LOWLATENCY_TRIG;
				set[0].value = p_user->delay_trig_lowlatency;

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*1;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) {
					DBG_ERR("set %d => r=%d,rv=%08x\r\n", id, r, cmd.rv);
					goto _HD_VP1;
				}
			}
			break;
#endif
			default:
				rv = HD_ERR_PARAM;
				goto _HD_VP12;
				break;
			}
		} else {
			switch(id) {
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			case HD_VIDEOPROC_PARAM_FUNC_CONFIG:
			{
				HD_VIDEOPROC_FUNC_CONFIG* p_user = (HD_VIDEOPROC_FUNC_CONFIG*)p_param;
				ISF_SET set[2] = {0};
				ISF_ATTR t_attr;

				hdal_flow_log_p("out_cfg_func(0x%08x) ddr_id(%lu)\n", p_user->out_func, p_user->ddr_id);

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				//if((out_id - OUT_BASE) >= OUT_PHY_COUNT) {rv = HD_ERR_IO; goto _HD_VP12;} //only support phy out

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					UINT32 ctrl_func = (_hd_vprc_ctx[did].path_cfg[0].in_func & HD_VIDEOPROC_INFUNC_DIRECT); //get ctrl func
					_hd_vprc_ctx[did].path_cfg[0].in_func= (ctrl_func | p_user->in_func); //keep ctrl func
					_hd_vprc_ctx[did].path_cfg[oid].out_func= p_user->out_func;
					_hd_vprc_ctx[did].path_cfg[oid].ddr_id = p_user->ddr_id;
					_hd_vprc_ctx[did].path_cfg[oid].out_order = p_user->out_order;
				}


				t_attr.attr = p_user->ddr_id;
				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = ISF_UNIT_PARAM_ATTR;
				cmd.value = (UINT32)&t_attr;
				cmd.size = sizeof(ISF_ATTR);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;

				set[0].param = VDOPRC_PARAM_OUT_CFG_FUNC;
				set[0].value = p_user->out_func;
				set[1].param = VDOPRC_PARAM_OUT_ORDER;
				set[1].value = p_user->out_order;
				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = ISF_UNIT_PARAM_MULTI;
				cmd.value = (UINT32)set;
				cmd.size = sizeof(ISF_SET)*2;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;
			}
			break;
#endif
			case HD_VIDEOPROC_PARAM_IN:
			{
				HD_VIDEOPROC_IN* p_user = (HD_VIDEOPROC_IN*)p_param;
				ISF_VDO_FRAME frame = {0};
				ISF_VDO_FPS fps = {0};

				hdal_flow_log_p("dim(%ux%u) pxlfmt(0x%08x) dir(0x%08x)\n", p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->dir);

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VP12;}
				cmd.dest = ISF_PORT(self_id, in_id);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 iid = in_id - ISF_IN_BASE;
					_hd_vprc_ctx[did].in[iid] = p_user[0];
				}

				/*
				TODO: p_user->func & HD_VIDEOPROC_INFUNC_DIRECT
				*/
				frame.direct = p_user->dir;
				frame.imgfmt = p_user->pxlfmt;
				frame.imgsize.w = p_user->dim.w;
				frame.imgsize.h = p_user->dim.h;
				cmd.param = ISF_UNIT_PARAM_VDOFRAME;
				cmd.value = (UINT32)&frame;
				cmd.size = sizeof(ISF_VDO_FRAME);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;

				fps.framepersecond = p_user->frc;
				cmd.param = ISF_UNIT_PARAM_VDOFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_VDO_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_IN_FRC:
			{
				HD_VIDEOPROC_FRC* p_user = (HD_VIDEOPROC_FRC*)p_param;
				ISF_VDO_FPS fps = {0};

				hdal_flow_log_p("frc(%u/%u)\n", GET_HI_UINT16(p_user->frc), GET_LO_UINT16(p_user->frc));

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VP12;}
				cmd.dest = ISF_PORT(self_id, in_id);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 iid = in_id - ISF_IN_BASE;
					_hd_vprc_ctx[did].in[iid].frc = p_user[0].frc;
				}

				fps.framepersecond = p_user->frc;
				cmd.param = ISF_UNIT_PARAM_VDOFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_VDO_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_IN_CROP:
			{
				HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
				ISF_VDO_WIN win = {0};

				hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VP12;}
				cmd.dest = ISF_PORT(self_id, in_id);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 iid = in_id - ISF_IN_BASE;
					_hd_vprc_ctx[did].in_crop[iid] = p_user[0];
				}

				if(p_user->mode == HD_CROP_OFF) { ///< disable crop
					win.window.x = 0;
					win.window.y = 0;
					win.window.w = 0;
					win.window.h = 0;
					win.imgaspect.w = 0;
					win.imgaspect.h = 0;
				} else if(p_user->mode == HD_CROP_ON) { ///< enable crop
					win.window.x = p_user->win.rect.x;
					win.window.y = p_user->win.rect.y;
					win.window.w = p_user->win.rect.w;
					win.window.h = p_user->win.rect.h;
					win.imgaspect.w = 0;
					win.imgaspect.h = 0;
				} else if(p_user->mode == HD_CROP_AUTO) { ///< auto calculate crop by DZOOM
					win.window.x = 0;
					win.window.y = 0;
					win.window.w = 0;
					win.window.h = 0;
					win.imgaspect.w = 1;
					win.imgaspect.h = 1;
				} else {
					r = HD_ERR_PARAM;
					goto _HD_VP1;
				}
				cmd.param = ISF_UNIT_PARAM_VDOWIN;
				cmd.value = (UINT32)&win;
				cmd.size = sizeof(ISF_VDO_WIN);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_OUT:
			{
				HD_VIDEOPROC_OUT* p_user = (HD_VIDEOPROC_OUT*)p_param;
				ISF_VDO_MAX max = {0};
				ISF_VDO_FRAME frame = {0};
				ISF_VDO_FPS fps = {0};
				ISF_VDO_WIN win = {0};

				hdal_flow_log_p("dim(%ux%u) pxlfmt(0x%08x) dir(0x%08x) frc(%u/%u) func(%08x) depth(%d)\n", p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->dir, GET_HI_UINT16(p_user->frc), GET_LO_UINT16(p_user->frc), p_user->func, p_user->depth);

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				if((out_id - OUT_BASE) >= OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP12;} //only support phy out
				cmd.dest = ISF_PORT(self_id, out_id);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					_hd_vprc_ctx[did].out[oid] = p_user[0];
				}

				max.max_frame = p_user->depth;
				max.max_imgfmt = 0;
				max.max_imgsize.w = 0;
				max.max_imgsize.h = 0;
				if (max.max_frame > OUT_MAX_DEPTH) {
					//DBG_WRN("The max depth is %d!.\r\n", OUT_MAX_DEPTH);
					max.max_frame = OUT_MAX_DEPTH;
				}
				cmd.param = ISF_UNIT_PARAM_VDOMAX;
				cmd.value = (UINT32)&max;
				cmd.size = sizeof(ISF_VDO_MAX);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;

				/*
				TODO: p_user->func & HD_VIDEOPROC_OUTFUNC_MD;
				TODO: p_user->func & HD_VIDEOPROC_OUTFUNC_DIS;
				*/
				frame.direct = p_user->dir;
				frame.imgfmt = p_user->pxlfmt;
				frame.imgsize.w = p_user->dim.w;
				frame.imgsize.h = p_user->dim.h;
				cmd.param = ISF_UNIT_PARAM_VDOFRAME;
				cmd.value = (UINT32)&frame;
				cmd.size = sizeof(ISF_VDO_FRAME);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;

				fps.framepersecond = p_user->frc;
				cmd.param = ISF_UNIT_PARAM_VDOFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_VDO_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);


				if (p_user->dim.w == 0 && p_user->dim.h == 0 && p_user->bg.w && p_user->bg.h && p_user->rect.w && p_user->rect.h) {
					win.window.x = p_user->rect.x;
					win.window.y = p_user->rect.y;
					win.window.w = p_user->rect.w;
					win.window.h = p_user->rect.h;
					win.imgaspect.w = p_user->bg.w;
					win.imgaspect.h = p_user->bg.h;
				}
				cmd.param = VDOPRC_PARAM_OUT_REGION;
				cmd.value = (UINT32)&win;
				cmd.size = sizeof(ISF_VDO_WIN);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);

				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_OUT_FRC:
			{
				HD_VIDEOPROC_FRC* p_user = (HD_VIDEOPROC_FRC*)p_param;
				ISF_VDO_FPS fps = {0};

				hdal_flow_log_p("frc(%u/%u)\n", GET_HI_UINT16(p_user->frc), GET_LO_UINT16(p_user->frc));

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				//if((out_id - OUT_BASE) >= OUT_PHY_COUNT) {rv = HD_ERR_IO; goto _HD_VP12;} //only support phy out
				cmd.dest = ISF_PORT(self_id, out_id);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					_hd_vprc_ctx[did].out[oid].frc = p_user[0].frc;
				}

				fps.framepersecond = p_user->frc;
				cmd.param = ISF_UNIT_PARAM_VDOFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_VDO_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_OUT_CROP:
			{
				HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
				ISF_VDO_WIN win = {0};

				hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				if((out_id - OUT_BASE) >= OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP12;} //only support phy out
				cmd.dest = ISF_PORT(self_id, out_id);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					_hd_vprc_ctx[did].out_crop[oid] = p_user[0];
				}

				if(p_user->mode == HD_CROP_OFF) { ///< disable crop
					win.window.x = 0;
					win.window.y = 0;
					win.window.w = 0;
					win.window.h = 0;
					win.imgaspect.w = 0;
					win.imgaspect.h = 0;
				} else if(p_user->mode == HD_CROP_ON) { ///< enable crop
					win.window.x = p_user->win.rect.x;
					win.window.y = p_user->win.rect.y;
					win.window.w = p_user->win.rect.w;
					win.window.h = p_user->win.rect.h;
					win.imgaspect.w = 0;
					win.imgaspect.h = 0;
				} else {
					r = HD_ERR_PARAM;
					goto _HD_VP1;
				}
				cmd.param = ISF_UNIT_PARAM_VDOWIN;
				cmd.value = (UINT32)&win;
				cmd.size = sizeof(ISF_VDO_WIN);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_OUT_EX:
			{
				HD_VIDEOPROC_OUT_EX* p_user = (HD_VIDEOPROC_OUT_EX*)p_param;
				ISF_VDO_MAX max = {0};
				ISF_VDO_FUNC func = {0};
				ISF_VDO_FRAME frame = {0};
				ISF_VDO_FPS fps = {0};
				HD_IO src_out_id = HD_GET_OUT(p_user->src_path);

				hdal_flow_log_p("dim(%ux%u) pxlfmt(0x%08x) dir(0x%08x) frc(%u/%u) src_path(%08x) depth(%d)\n", p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->dir, GET_HI_UINT16(p_user->frc), GET_LO_UINT16(p_user->frc), p_user->src_path, p_user->depth);

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				if((out_id - OUT_BASE) < OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP12;} //only support ext out
				cmd.dest = ISF_PORT(self_id, out_id);

				_HD_CONVERT_OUT_ID(src_out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				if((src_out_id - OUT_BASE) >= OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP12;} //only support phy out

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					_hd_vprc_ctx[did].out_ex[oid-OUT_PHY_COUNT] = p_user[0];
				}

				func.src = src_out_id;
				func.func = 0;
				cmd.param = ISF_UNIT_PARAM_VDOFUNC;
				cmd.value = (UINT32)&func;
				cmd.size = sizeof(ISF_VDO_FUNC);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;

				max.max_frame = p_user->depth;
				max.max_imgfmt = 0;
				max.max_imgsize.w = 0;
				max.max_imgsize.h = 0;
				if (max.max_frame > OUT_MAX_DEPTH) {
					//DBG_WRN("The max depth is %d!.\r\n", OUT_MAX_DEPTH);
					max.max_frame = OUT_MAX_DEPTH;
				}
				cmd.param = ISF_UNIT_PARAM_VDOMAX;
				cmd.value = (UINT32)&max;
				cmd.size = sizeof(ISF_VDO_MAX);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;

				frame.direct = p_user->dir;
				frame.imgfmt = p_user->pxlfmt;
				frame.imgsize.w = p_user->dim.w;
				frame.imgsize.h = p_user->dim.h;
				cmd.param = ISF_UNIT_PARAM_VDOFRAME;
				cmd.value = (UINT32)&frame;
				cmd.size = sizeof(ISF_VDO_FRAME);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VP1;

				fps.framepersecond = p_user->frc;
				cmd.param = ISF_UNIT_PARAM_VDOFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_VDO_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_OUT_EX_CROP:
			{
				HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
				ISF_VDO_WIN win = {0};

				hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				if((out_id - OUT_BASE) < OUT_PHY_COUNT) { rv = HD_ERR_IO; goto _HD_VP12;} //only support ext out
				cmd.dest = ISF_PORT(self_id, out_id);

				//keep for get
				{
					UINT32 did = self_id - ISF_UNIT_VDOPRC;
					UINT32 oid = out_id - ISF_OUT_BASE;
					_hd_vprc_ctx[did].out_crop[oid] = p_user[0];
				}

				if(p_user->mode == HD_CROP_OFF) { ///< disable crop
					win.window.x = 0;
					win.window.y = 0;
					win.window.w = 0;
					win.window.h = 0;
					win.imgaspect.w = 0;
					win.imgaspect.h = 0;
				} else if(p_user->mode == HD_CROP_ON) { ///< enable crop
					win.window.x = p_user->win.rect.x;
					win.window.y = p_user->win.rect.y;
					win.window.w = p_user->win.rect.w;
					win.window.h = p_user->win.rect.h;
					win.imgaspect.w = 0;
					win.imgaspect.h = 0;
				} else {
					r = HD_ERR_PARAM;
					goto _HD_VP1;
				}
				cmd.param = ISF_UNIT_PARAM_VDOWIN;
				cmd.value = (UINT32)&win;
				cmd.size = sizeof(ISF_VDO_WIN);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_IN_CROP_PSR:
			{
				HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
				URECT win = {0};
				UINT32 did = self_id - ISF_UNIT_VDOPRC;

				hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);

				if(_hd_vprc_ctx[did].dev_cfg.pipe != HD_VIDEOPROC_PIPE_VPE) {
					DBG_ERR("only support VPE mode!\r\n");
					r = HD_ERR_NOT_ALLOW;
					goto _HD_VP1;
				}

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				if((out_id - OUT_BASE) >= OUT_VPE_COUNT) { rv = HD_ERR_IO; goto _HD_VP12;} //only support vpe out
				cmd.dest = ISF_PORT(self_id, out_id);

				//keep for get
				{
					UINT32 oid = out_id - ISF_OUT_BASE;
					_hd_vprc_ctx[did].in_crop_psr[oid] = p_user[0];
				}

				if (p_user->mode != HD_CROP_OFF) {
					win.x = p_user->win.rect.x;
					win.y = p_user->win.rect.y;
					win.w = p_user->win.rect.w;
					win.h = p_user->win.rect.h;
				}

				cmd.param = VDOPRC_PARAM_VPE_PRE_SCL_CROP;
				cmd.value = (UINT32)&win;
				cmd.size = sizeof(URECT);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			case HD_VIDEOPROC_PARAM_OUT_CROP_PSR:
			{
				HD_VIDEOPROC_CROP* p_user = (HD_VIDEOPROC_CROP*)p_param;
				URECT win = {0};
				UINT32 did = self_id - ISF_UNIT_VDOPRC;

				hdal_flow_log_p("mode(%d) rect(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);

				if(_hd_vprc_ctx[did].dev_cfg.pipe != HD_VIDEOPROC_PIPE_VPE) {
					DBG_ERR("only support VPE mode!\r\n");
					r = HD_ERR_NOT_ALLOW;
					goto _HD_VP1;
				}

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VP12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VP12;}
				if((out_id - OUT_BASE) >= OUT_VPE_COUNT) { rv = HD_ERR_IO; goto _HD_VP12;} //only support vpe out
				cmd.dest = ISF_PORT(self_id, out_id);

				//keep for get
				{
					UINT32 oid = out_id - ISF_OUT_BASE;
					_hd_vprc_ctx[did].out_crop_psr[oid] = p_user[0];
				}

				if (p_user->mode != HD_CROP_OFF) {
					win.x = p_user->win.rect.x;
					win.y = p_user->win.rect.y;
					win.w = p_user->win.rect.w;
					win.h = p_user->win.rect.h;
				}

				cmd.param = VDOPRC_PARAM_VPE_HOLE_REGION;
				cmd.value = (UINT32)&win;
				cmd.size = sizeof(URECT);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VP1;
			}
			break;
			default:
				rv = HD_ERR_PARAM;
				goto _HD_VP12;
				break;
			}
		}
		goto _HD_VP12;
	}
_HD_VP12:
	unlock_ctx();
	return rv;
_HD_VP1:
	unlock_ctx();
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

void _hd_videoproc_dewrap_frame(ISF_DATA *p_data, HD_VIDEO_FRAME* p_video_frame)
{
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_frame->blk; //isf-data-handle

	p_vdoframe->sign = p_video_frame->sign;
	p_vdoframe->p_next = NULL;
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

	//for planer pxlfmt
	p_vdoframe->pw[VDO_PINDEX_V] = p_video_frame->pw[VDO_PINDEX_V];
	p_vdoframe->ph[VDO_PINDEX_V] = p_video_frame->ph[VDO_PINDEX_V];
	p_vdoframe->loff[VDO_PINDEX_V] = p_video_frame->loff[VDO_PINDEX_V];
	p_vdoframe->phyaddr[VDO_PINDEX_V] = p_video_frame->phy_addr[VDO_PINDEX_V];

	//for other internal info
	p_vdoframe->pw[VDO_PINDEX_A] = p_video_frame->pw[VDO_PINDEX_A];
	p_vdoframe->ph[VDO_PINDEX_A] = p_video_frame->ph[VDO_PINDEX_A];
	p_vdoframe->loff[VDO_PINDEX_A] = p_video_frame->loff[VDO_PINDEX_A];
	p_vdoframe->phyaddr[VDO_PINDEX_A] = p_video_frame->phy_addr[VDO_PINDEX_A];

	p_vdoframe->reserved[0] = p_video_frame->poc_info;
	p_vdoframe->reserved[1] = p_video_frame->reserved[0];
	p_vdoframe->reserved[2] = p_video_frame->reserved[1];
	p_vdoframe->reserved[3] = p_video_frame->reserved[2];
	p_vdoframe->reserved[4] = p_video_frame->reserved[3];
	p_vdoframe->reserved[5] = p_video_frame->reserved[4];
	p_vdoframe->reserved[6] = p_video_frame->reserved[5];
	p_vdoframe->reserved[7] = p_video_frame->reserved[6];
}

HD_RESULT hd_videoproc_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame, HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_vprc_ctx == NULL)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_vprc_ctx == NULL) {
			return HD_ERR_UNINIT;
		}
	}

	if (p_video_frame == 0) {
		return HD_ERR_NULL_PTR;
	}

	if (p_video_frame->sign != MAKEFOURCC('V','F','R','M')) {
		return HD_ERR_SIGN;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}

		//user specified output buffer
		if (p_user_out_video_frame) {
			ISF_FLOW_IOCTL_PARAM_ITEM _cmd = {0};
			HD_VIDEO_FRAME *p_out_frm = p_user_out_video_frame;
			UINT32 i;
			VDOPRC_USER_OUT_BUF user_out_blk = {0};

			for (i = 0; i < VDOPRC_MAX_OUT_NUM; i++) {
				if (p_out_frm->sign == MAKEFOURCC('V','F','R','M')) {
					user_out_blk.blk[i] = p_out_frm->blk;
					user_out_blk.size[i] = p_out_frm->reserved[0];
					if (p_out_frm->p_next) {
						p_out_frm = (HD_VIDEO_FRAME *)p_out_frm->p_next;
						continue;
					}
				}
				break;
			}
			_cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			_cmd.param = VDOPRC_PARAM_USER_OUT_BUF;
			_cmd.value = (UINT32)&user_out_blk;
			_cmd.size = sizeof(user_out_blk);
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &_cmd);
			if (r || _cmd.rv) {
				DBG_ERR("VDOPRC_PARAM_USER_OUT_BUF fail, r=%d rv=%d\r\n", r, _cmd.rv);
				return cmd.rv;
			}
		}

		_hd_videoproc_dewrap_frame(&data, p_video_frame);

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

#define TIMESTAMP_TO_MS(ts)  ((ts >> 32) * 1000 + (ts & 0xFFFFFFFF) / 1000)

void _hd_videoproc_wrap_frame(HD_VIDEO_FRAME* p_video_frame, ISF_DATA *p_data)
{
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;

	p_video_frame->blk = p_data->h_data; //isf-data-handle

	p_video_frame->sign = p_vdoframe->sign;
	if (p_vdoframe->p_next) {
		NVTMPP_IOC_VB_VA_TO_PA_S msg;
		int                      ret;

		msg.va = (UINT32)p_vdoframe->p_next;
		ret = ISF_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_VA_TO_PA , &msg);
		if (0 == ret) {
			p_video_frame->p_next = (HD_METADATA *)msg.pa;
		} else {
			p_video_frame->p_next = NULL;
		}
		//DBG_IND("p_next va = 0x%x, pa= 0x%x\r\n", (int)msg.va, (int)msg.pa);
	} else {
		p_video_frame->p_next = NULL;
	}
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
	//for planer pxlfmt
	p_video_frame->pw[VDO_PINDEX_V] = p_vdoframe->pw[VDO_PINDEX_V];
	p_video_frame->ph[VDO_PINDEX_V] = p_vdoframe->ph[VDO_PINDEX_V];
	p_video_frame->loff[VDO_PINDEX_V] = p_vdoframe->loff[VDO_PINDEX_V];
	p_video_frame->phy_addr[VDO_PINDEX_V] = p_vdoframe->phyaddr[VDO_PINDEX_V];
	p_video_frame->poc_info = p_vdoframe->reserved[0];
	p_video_frame->reserved[0] = p_vdoframe->reserved[1];
	p_video_frame->reserved[1] = p_vdoframe->reserved[2];
	p_video_frame->reserved[2] = p_vdoframe->reserved[3];
	p_video_frame->reserved[3] = p_vdoframe->reserved[4];
	p_video_frame->reserved[4] = p_vdoframe->reserved[5];
	p_video_frame->reserved[5] = p_vdoframe->reserved[6];
	p_video_frame->reserved[6] = p_vdoframe->reserved[7];
}

void _hd_videoproc_rebuild_frame(ISF_DATA *p_data, HD_VIDEO_FRAME* p_video_frame)
{
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_frame->blk; //isf-data-handle

	p_vdoframe->sign       = p_video_frame->sign;
}

HD_RESULT hd_videoproc_pull_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_vprc_ctx == NULL)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_vprc_ctx == NULL) {
			return HD_ERR_UNINIT;
		}
	}

	if (p_video_frame == 0) {
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
				_hd_videoproc_wrap_frame(p_video_frame, &data);
				//p_video_frame->reserved[1] = p_vdoframe->reserved[2];
				if (p_video_frame->reserved[1] == MAKEFOURCC('D', 'R', 'O', 'P')) {
					HD_RESULT ret;

					rv = HD_ERR_BAD_DATA;
					ret = hd_videoproc_release_out_buf(path_id, p_video_frame);
					if (ret != HD_OK) {
						printf("BAD DATA release error=%d !!\r\n\r\n", ret);
					}
				} else {
					rv = HD_OK;
				}
				break;
			default:
				rv = HD_ERR_SYS;
				break;
			}
		} else {
			if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
				rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
				//p_video_frame->blk = data.h_data; //isf-data-handle
			} else {
				DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
				rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
			}
		}
	}
	return rv;
}

HD_RESULT hd_videoproc_release_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_vprc_ctx == NULL)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_vprc_ctx == NULL) {
			return HD_ERR_UNINIT;
		}
	}

	if (p_video_frame == 0) {
		return HD_ERR_NULL_PTR;
	}

	if (p_video_frame->sign != MAKEFOURCC('V','F','R','M')) {
		return HD_ERR_SIGN;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_videoproc_rebuild_frame(&data, p_video_frame);

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

HD_RESULT hd_videoproc_poll_list(HD_VIDEOPROC_POLL_LIST *p_poll, UINT32 num, INT32 wait_ms)
{
	HD_DAL dev_id;
	HD_PATH_ID path_id;
	HD_IO out_id;
	HD_RESULT rv = HD_ERR_NG;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	VDOPRC_POLL_LIST poll_list;
	VDOPRC_POLL_LIST poll_list_ret;
	UINT32 i;
	int r;

	///hdal_flow_log_p("hd_videoproc_poll_list:\n    ");

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if ((_hd_common_get_pid() == 0) && (_hd_vprc_ctx == NULL)) {
			return HD_ERR_UNINIT;
		}
	} else {
		if (_hd_vprc_ctx == NULL) {
			return HD_ERR_UNINIT;
		}
	}

	if (num == 0) {
		DBG_ERR("num is 0\n");
		return HD_ERR_PARAM;
	}

	// set to kernel to wait for bs available
	memset(&poll_list, 0, sizeof(VDOPRC_POLL_LIST));

	for (i=0; i< num; i++) {
		path_id = p_poll[i].path_id;
		dev_id = HD_GET_DEV(path_id);
		out_id = HD_GET_OUT(path_id);

		///hdal_flow_log_p("path_id(0x%08x) ", path_id);

		if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { return HD_ERR_IO;}
		_HD_CONVERT_SELF_ID(dev_id, rv); if(rv != HD_OK) { return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv); if(rv != HD_OK) { return rv;}

		poll_list.path_mask[dev_id-DEV_BASE] |= (1 << out_id);
	}

	///hdal_flow_log_p("\n");

	poll_list.wait_ms = wait_ms;

	{
		cmd.dest = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		cmd.param = VDOPRC_PARAM_POLL_LIST;
		cmd.value = (UINT32)&poll_list;
		cmd.size = sizeof(VDOPRC_POLL_LIST);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
		if (r != 0) goto _HD_VP_POLL;
	}

	// return event_mask to user
	{
		//UINT32 event_mask = 0;
		ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
		HD_RESULT rv;
		int r;

		cmd.dest = ISF_PORT(DEV_BASE+0, ISF_CTRL);
		cmd.param = VDOPRC_PARAM_POLL_LIST;
		cmd.value = (UINT32)&poll_list_ret;
		cmd.size = sizeof(VDOPRC_POLL_LIST);

		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				rv = HD_OK;
				//event_mask = cmd.value;
				break;
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
		///hdal_flow_log_p("    event_mask(0x%08x)\n", event_mask);

		if (rv == HD_OK) {
			for (i=0; i< num; i++) {
				path_id = p_poll[i].path_id;
				dev_id = HD_GET_DEV(path_id);
				out_id = HD_GET_OUT(path_id);

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { return HD_ERR_IO;}
				_HD_CONVERT_SELF_ID(dev_id, rv); if(rv != HD_OK) { return rv;}
				_HD_CONVERT_OUT_ID(out_id, rv); if(rv != HD_OK) { return rv;}

				p_poll[i].revent.event = (poll_list_ret.path_mask[dev_id-DEV_BASE] & (1<<out_id))? TRUE:FALSE;
			}
		}
	}

_HD_VP_POLL:
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


HD_RESULT hd_videoproc_uninit(VOID)
{
	UINT32 cfg, cfg1, cfg2;
	HD_RESULT rv;

	hdal_flow_log_p("hd_videoproc_uninit\n");

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

		if (!_hd_common_is_new_flow()) {
			return HD_OK;
		}
	}

	if (_hd_vprc_ctx == NULL) {
		return HD_ERR_INIT;
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

	g_cfg_cpu = (cfg & HD_COMMON_CFG_CPU);
	g_cfg_debug = ((cfg & HD_COMMON_CFG_DEBUG) >> 12);
	g_cfg_vprc = ((cfg1 & HD_VIDEOPROC_CFG) >> 16);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	g_cfg_ai = ((cfg2 & VENDOR_AI_CFG) >> 16);
#endif

	//uninit drv
	{
		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 0; //cmd = uninit
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = 0;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		if (g_cfg_ai & (VENDOR_AI_CFG_ENABLE_CNN >> 16)) { //if enable ai CNN2
			UINT32 stripe_rule = g_cfg_vprc & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
			if (stripe_rule > 3) {
				stripe_rule = 0;
			}
			if ((stripe_rule == 0)||(stripe_rule == 3)) { //NOT disable GDC?
				//DBG_DUMP("already enable CNN => current vprc config stripe_rule = LV%lu, enforce to LV2!", stripe_rule+1);
				g_cfg_vprc |= (HD_VIDEOPROC_CFG_STRIP_LV2 >> 16); //disable GDC for AI
			} else {
				//DBG_DUMP("already enable CNN => current vprc config stripe_rule = LV%lu", stripe_rule+1);
			}
		}
#endif
		isf_dev_cmd.p2 = (g_cfg_cpu << 28) | (g_cfg_debug << 24) | (g_cfg_vprc);
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("uninit fail? %d\r\n", r);
			return r;
		}
	}

	}

	//uninit lib
	{
		free_ctx(_hd_vprc_ctx);
		_hd_vprc_ctx = NULL;
	}

	return HD_OK;
}

int _hd_videoproc_is_init(VOID)
{
	return (_hd_vprc_ctx != NULL);
}



