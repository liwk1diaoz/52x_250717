/**
	@brief Source code of video output module.\n
	This file contains the functions which is related to video output in the chip.

	@file hd_videoout.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "hdal.h"
#define HD_MODULE_NAME HD_VIDEOOUT
#include "hd_int.h"
#include "hd_logger_p.h"
#include "kflow_videoout/isf_vdoout.h"

#define HD_VIDEOOUT_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))
#define DEV_BASE        ISF_UNIT_VDOOUT
#define DEV_COUNT       ISF_MAX_VDOOUT
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        9
#define OUT_BASE        ISF_OUT_BASE
#define OUT_COUNT       1

#define HD_DEV_BASE HD_DAL_VIDEOOUT_BASE
#define HD_DEV_MAX  HD_DAL_VIDEOOUT_MAX
#define HD_FB_CAP_MAX   1

typedef struct _HD_VOUT_CTX {
    UINT32 start_tag;
    HD_VIDEOOUT_MODE vdoout_mode;
    HD_VIDEOOUT_WIN_ATTR vdoout_win;
    HD_VIDEOOUT_IN vdoout_in;
    HD_VIDEO_PXLFMT fb_format[HD_FB_MAX];
    BOOL fb_en[HD_FB_MAX];
    HD_FB_ATTR fb_attr[HD_FB_MAX];
    HD_FB_PALETTE_TBL  fb_palette;//only FB0 support palette
    UINT32 palette_table[VDOOUT_PALETTE_MAX];//only FB0 support palette
    HD_FB_DIM fb_dim[HD_FB_MAX];
    HD_VIDEOOUT_FUNC_CONFIG func_cfg;
    UINT32 end_tag;
} HD_VOUT_CTX;

static UINT32 _hd_vout_csz = 0;  //context buffer size
static HD_VOUT_CTX* _hd_vout_ctx = NULL;  //context buffer addr
static UINT32 _max_dev_count = VDOOUT_MAX_NUM;

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


#if defined(__LINUX)
//#include <sys/types.h>	  /* key_t, sem_t, pid_t	  */
//#include <sys/shm.h>		  /* shmat(), IPC_RMID		  */
#include <errno.h>          /* errno, ECHILD            */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>          /* O_CREAT, O_EXEC          */

static int	  g_shmid = 0;
#define SHM_KEY HD_DAL_VIDEOOUT_BASE
static sem_t *g_sem = 0;
#define SEM_NAME "sem_vout"

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

////////////////////////////////private API///////////////////////////////////////
static INT32 _hd_fb_get_lyr(HD_FB_ID fb_id);
static INT32 _hd_fb_get_fmt(HD_FB_ID fb_id,HD_VIDEO_PXLFMT fmt);
static INT32 _hd_fb_get_blend(HD_VIDEO_PXLFMT fmt);
static UINT32 _hd_videoout_chk_vdo_dir(HD_VIDEO_DIR dir);
static UINT32 _hd_videoout_get_dir(HD_VIDEO_DIR dir);
static UINT16 _hd_fb_get_fmt_color(HD_VIDEO_PXLFMT fmt,UINT16 color,UINT32 isG);
static INT32 _hd_videoout_chk_vdo_fmt(HD_VIDEO_PXLFMT fmt);

HD_RESULT _hd_videoout_convert_dev_id(HD_DAL* p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

HD_RESULT _hd_videoout_convert_in_id(HD_IO* p_in_id)
{
	HD_RESULT rv;
	_HD_CONVERT_IN_ID(p_in_id[0], rv);
	return rv;
}

int _hd_videoout_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "VIDEOOUT_%d_IN_%d", dev_id - HD_DEV_BASE, in_id - HD_IN_BASE);
}


void _hd_videoout_cfg_max(UINT32 maxdevice)
{
	if (!_hd_vout_ctx) {
		_max_dev_count = maxdevice;
		if (_max_dev_count > VDOOUT_MAX_NUM) {
			DBG_WRN("dts max_device=%lu is larger than built-in max_device=%lu!\r\n", _max_dev_count, (UINT32)VDOOUT_MAX_NUM);
            _max_dev_count = VDOOUT_MAX_NUM;
		}
	}
}

HD_RESULT hd_videoout_init(VOID)
{
	UINT32 i;

	hdal_flow_log_p("hd_videoout_init\n");

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

	if (_hd_vout_ctx != NULL) {
		return HD_ERR_UNINIT;
	}

	if (_max_dev_count == 0) {
		DBG_ERR("_max_dev_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}
	//init lib
	_hd_vout_csz = sizeof(HD_VOUT_CTX)*_max_dev_count;
	_hd_vout_ctx = (HD_VOUT_CTX*)malloc_ctx(_hd_vout_csz);
	if (_hd_vout_ctx == NULL) {
		DBG_ERR("cannot alloc heap for _max_dev_count =%d\r\n", (int)_max_dev_count);
		return HD_ERR_HEAP;
	}

	if (_hd_common_get_pid() == 0) {

		lock_ctx();
		for(i = 0; i < _max_dev_count; i++) {
			memset(&(_hd_vout_ctx[i]), 0, sizeof(HD_VOUT_CTX));
	        _hd_vout_ctx[i].start_tag = 0xAA55AA55;
	        _hd_vout_ctx[i].end_tag = 0xAA55AA55;
		}
		unlock_ctx();
	}

	if (_hd_common_get_pid() == 0) {

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
			DBG_ERR("init fail? %d\r\n", r);
			return r;
		}
	}

	}

	return HD_OK;
}
HD_RESULT hd_videoout_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoout_bind:\n");
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
    if (_hd_videoout_is_init()== FALSE) {
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

HD_RESULT hd_videoout_unbind(HD_OUT_ID _out_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoout_unbind:\n");
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
    if (_hd_videoout_is_init()== FALSE) {
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

HD_RESULT _hd_videoout_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
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

HD_RESULT _hd_videoout_get_state(HD_OUT_ID _out_id, UINT32* p_state)
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
HD_RESULT hd_videoout_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	UINT32 dev_id = self_id - HD_DEV_BASE;

	hdal_flow_log_p("hd_videoout_open:\n");
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
    if (_hd_videoout_is_init()== FALSE) {
    	return HD_ERR_UNINIT;
    }

	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		_HD_CONVERT_SELF_ID(self_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
			hdal_flow_log_p("path_id(N/A)\n"); return rv;
		}
		if(!p_path_id) {hdal_flow_log_p("path_id(N/A)\n"); return HD_ERR_NULL_PTR;}
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

			p_path_id[0] = HD_VIDEOOUT_PATH(cur_dev, in_id, path_out_id); //osg_id + out_id
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
			p_path_id[0] = HD_VIDEOOUT_PATH(cur_dev, path_in_id, out_id); //in_id + osg_id
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
				hdal_flow_log_p("dev_id(N/A)\n"); return HD_ERR_DEV;
			}
			if(!p_path_id) {return HD_ERR_NULL_PTR;}
			p_path_id[0] = HD_VIDEOOUT_PATH(cur_dev, in_id, out_id);
		}
	}
	hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);
	//--- check if device config had been set ---
	{
		lock_ctx();
		if (_hd_vout_ctx[dev_id].vdoout_mode.output_type == 0) {
			DBG_ERR("Error: please set HD_VIDEOOUT_PARAM_MODE first !!!\r\n");
			unlock_ctx();
			return HD_ERR_NO_CONFIG;
		}
		unlock_ctx();
	}
	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_OUT_ID(out_id, rv);
		if (rv != HD_OK) {
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
	return rv;
}

HD_RESULT hd_videoout_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
    UINT32 dev_id = self_id - HD_DEV_BASE;

	hdal_flow_log_p("hd_videoout_start:\n");
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
    if (_hd_videoout_is_init()== FALSE) {
    	return HD_ERR_UNINIT;
    }

	{
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

	//--- check if in had been set ---
	{
		lock_ctx();
		if (_hd_vout_ctx[dev_id].vdoout_in.pxlfmt == 0) {
			DBG_ERR("Error: please set HD_VIDEOOUT_PARAM_IN first !!!\r\n");
			unlock_ctx();
			return HD_ERR_NO_CONFIG;
		}
		unlock_ctx();
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

HD_RESULT hd_videoout_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoout_stop:\n");
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
    if (_hd_videoout_is_init()== FALSE) {
    	return HD_ERR_UNINIT;
    }

	{
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

HD_RESULT hd_videoout_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoout_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoout_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videoout_close:\n");
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
    if (_hd_videoout_is_init()== FALSE) {
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
INT _hd_videoout_param_cvt_name(HD_VIDEOOUT_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
	    case HD_VIDEOOUT_PARAM_DEVCOUNT:         snprintf(p_ret_string, max_str_len, "DEVCOUNT");          break;
		case HD_VIDEOOUT_PARAM_SYSCAPS:          snprintf(p_ret_string, max_str_len, "SYSCAPS");           break;
		case HD_VIDEOOUT_PARAM_DEV_CONFIG:       snprintf(p_ret_string, max_str_len, "DEV_CONFIG");        break;
		case HD_VIDEOOUT_PARAM_MODE:             snprintf(p_ret_string, max_str_len, "MODE");              break;
		case HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE: snprintf(p_ret_string, max_str_len, "PALETTE_TABLE");     break;
		case HD_VIDEOOUT_PARAM_CLEAR_WIN:        snprintf(p_ret_string, max_str_len, "CLEAR_WIN");         break;
		case HD_VIDEOOUT_PARAM_IN:               snprintf(p_ret_string, max_str_len, "IN");                break;
		case HD_VIDEOOUT_PARAM_IN_WIN_ATTR:      snprintf(p_ret_string, max_str_len, "IN_WIN_ATTR");       break;
		case HD_VIDEOOUT_PARAM_IN_WIN_PSR_ATTR:  snprintf(p_ret_string, max_str_len, "IN_WIN_PSR_ATTR");   break;
		case HD_VIDEOOUT_PARAM_WRITEBACK_BUF:    snprintf(p_ret_string, max_str_len, "WRITEBACK_BUF");     break;
		case HD_VIDEOOUT_PARAM_FB_FMT:           snprintf(p_ret_string, max_str_len, "FB_FMT");            break;
		case HD_VIDEOOUT_PARAM_FB_ATTR:          snprintf(p_ret_string, max_str_len, "FB_ATTR");           break;
		case HD_VIDEOOUT_PARAM_FB_ENABLE:        snprintf(p_ret_string, max_str_len, "FB_ENABLE");         break;
		case HD_VIDEOOUT_PARAM_FB_LAYER_ORDER:   snprintf(p_ret_string, max_str_len, "FB_LAYER_ORDER");    break;
		case HD_VIDEOOUT_PARAM_HDMI_ABILITY:     snprintf(p_ret_string, max_str_len, "HDMI_ABILITY");      break;
		case HD_VIDEOOUT_PARAM_OUT_STAMP_BUF:    snprintf(p_ret_string, max_str_len, "OUT_STAMP_BUF");     break;
		case HD_VIDEOOUT_PARAM_OUT_STAMP_IMG:    snprintf(p_ret_string, max_str_len, "OUT_STAMP_IMG");     break;
		case HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR:   snprintf(p_ret_string, max_str_len, "OUT_STAMP_ATTR");    break;
		case HD_VIDEOOUT_PARAM_OUT_MASK_ATTR:    snprintf(p_ret_string, max_str_len, "OUT_MASK_ATTR");     break;
		case HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR:  snprintf(p_ret_string, max_str_len, "OUT_MOSAIC_ATTR");   break;
		case HD_VIDEOOUT_PARAM_FB_DIM:           snprintf(p_ret_string, max_str_len, "FB_DIM");            break;
		case HD_VIDEOOUT_PARAM_OUT_PALETTE_TABLE: snprintf(p_ret_string, max_str_len, "OUT_PALETTE_TABLE");break;
		case HD_VIDEOOUT_PARAM_FUNC_CONFIG:      snprintf(p_ret_string, max_str_len, "FUNC_CONFIG");       break;
			default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", id);
			return (-1);
	}
	return 0;
}

int _hd_videoout_type_str(HD_COMMON_VIDEO_OUT_TYPE type, CHAR *p_str, INT str_len)
{
	switch (type) {
		case HD_COMMON_VIDEO_OUT_MIPI_DSI:          snprintf(p_str, str_len, "DSI");  break;
		case HD_COMMON_VIDEO_OUT_HDMI:            	snprintf(p_str, str_len, "HDMI");  break;
		case HD_COMMON_VIDEO_OUT_CVBS:            	snprintf(p_str, str_len, "CVBS");  break;
		case HD_COMMON_VIDEO_OUT_LCD:            	snprintf(p_str, str_len, "LCD");  break;
		default:  snprintf(p_str, str_len, "(?)");
			return (-1);
	}
	return 0;
}
HD_RESULT hd_videoout_get(HD_PATH_ID path_id, HD_VIDEOOUT_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
    UINT32 dev_id = self_id - HD_DEV_BASE;

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
    if (_hd_videoout_is_init()== FALSE) {
    	return HD_ERR_UNINIT;
    }
    if(dev_id>= _max_dev_count){
        return HD_ERR_LIMIT;
    }

	lock_ctx();
    if((_hd_vout_ctx[dev_id].start_tag != 0xAA55AA55)||
    (_hd_vout_ctx[dev_id].end_tag != 0xAA55AA55)){
        DBG_ERR("get dev_id %lx id %d tag err %lx %lx \r\n",dev_id,id,_hd_vout_ctx[dev_id].start_tag,_hd_vout_ctx[dev_id].end_tag);
		unlock_ctx();
        return HD_ERR_BAD_DATA;
    }
	unlock_ctx();
    {
		CHAR  param_name[20];
		_hd_videoout_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videoout_get(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
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
			return _hd_osg_get(self_id, out_id, osg_in_id, id, p_param);
		} else if(osg_out_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_get(self_id, in_id, osg_out_id, id, p_param);
		}
	}


	//DBG_IND("path_id %lx param_id %d in_id %d\r\n",path_id,id,in_id);
	{
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		cmd.param = id;

		lock_ctx();

		if(ctrl_id == HD_CTRL) {
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			switch(id) {
			case HD_VIDEOOUT_PARAM_DEVCOUNT:
			{
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
#if defined(_BSP_NA51000_)
				p_user->max_dev_count = 2;
#else
				p_user->max_dev_count = 1;
#endif
				hdal_flow_log_p("max_dev_cnt(%u)\n", p_user->max_dev_count);

				rv = HD_OK;
			}
			break;
			case HD_VIDEOOUT_PARAM_SYSCAPS:
			{
				HD_VIDEOOUT_SYSCAPS* p_user = (HD_VIDEOOUT_SYSCAPS*)p_param;
				VDOOUT_INFO vdoout_info={0};

				cmd.param = VDOOUT_PARAM_CAPS;
				cmd.value = (UINT32)&vdoout_info;
				cmd.size = sizeof(VDOOUT_INFO);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
				//DBG_IND("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);
				if((r != 0) || (cmd.rv!=0))
					goto _HD_VOG;

				//printf("===>vdoout_info.devSize.w =%d,h=%d\r\n", vdoout_info.devSize.w,vdoout_info.devSize.h);
				memset(p_user, 0, sizeof(HD_VIDEOOUT_SYSCAPS));
				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;
				//printf("===>self_id =%x,HD_DEV_BASE=%x DEV_BASE=%x\r\n", self_id,HD_DEV_BASE,DEV_BASE);

				p_user->chip_id = 0;
				p_user->max_in_count = 1;
				p_user->max_out_count = 1;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
				p_user->dev_caps = HD_VIDEOOUT_CAPS_LCD;
#else
				p_user->dev_caps = HD_VIDEOOUT_CAPS_CVBS|HD_VIDEOOUT_CAPS_HDMI|HD_VIDEOOUT_CAPS_LCD;
#endif
				p_user->in_caps[0] =
					 HD_VIDEO_CAPS_YUV420
					 | HD_VIDEO_CAPS_YUV422
					 | HD_VIDEO_CAPS_DIR_ROTATER
					 | HD_VIDEO_CAPS_DIR_ROTATEL
                     | HD_VIDEO_CAPS_DIR_ROTATE180
					 | HD_VIDEO_CAPS_DIR_MIRRORX
					 | HD_VIDEO_CAPS_DIR_MIRRORY
					 | HD_VIDEOOUT_INCAPS_ONEBUF
					 | HD_VIDEO_CAPS_STAMP
					 | HD_VIDEO_CAPS_MASK;
				p_user->output_dim.w=vdoout_info.devSize.w;
				p_user->output_dim.h=vdoout_info.devSize.h;
				p_user->input_dim.w=vdoout_info.devSize.w;
				p_user->input_dim.h=vdoout_info.devSize.h;

				p_user->fps = vdoout_info.devfps;
				p_user->max_scale_up_w = -1 ; //no limit
				p_user->max_scale_up_h = -1 ; //no limit
				p_user->max_scale_down_w = 2 ;
				p_user->max_scale_down_h = 2 ;
				p_user->max_fb_count = HD_FB_CAP_MAX;
    			p_user->fb_caps[HD_FB0] = HD_FB_CAPS_INDEX_8|HD_FB_CAPS_ARGB_1555|HD_FB_CAPS_ARGB_4444|HD_FB_CAPS_ARGB_8888|HD_FB_CAPS_ARGB_8565;

				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip_id, p_user->max_in_count, p_user->max_out_count, p_user->dev_caps);
				//hdal_flow_log_p("    max_in_stamp(%u) max_in_stamp_ex(%u) max_in_mask(%u) max_in_mask_ex(%u)\n", p_user->max_in_stamp, p_user->max_in_stamp_ex, p_user->max_in_mask, p_user->max_in_mask_ex);

				rv = HD_OK;
			}
			break;
			case HD_VIDEOOUT_PARAM_MODE:
			{
				HD_VIDEOOUT_MODE* hd_mode = (HD_VIDEOOUT_MODE*)p_param;
                char type[16]={0};
                UINT32 output_mode =0;

				memcpy((void *)hd_mode,(void *)&_hd_vout_ctx[dev_id].vdoout_mode,sizeof(HD_VIDEOOUT_MODE));

				switch(hd_mode->output_type){
				case HD_COMMON_VIDEO_OUT_CVBS:
                    output_mode = hd_mode->output_mode.cvbs;
				break;
				case HD_COMMON_VIDEO_OUT_HDMI:
                    output_mode = hd_mode->output_mode.hdmi;
				break;
				case HD_COMMON_VIDEO_OUT_LCD:
                    output_mode = hd_mode->output_mode.lcd;
				break;
				default:
					DBG_ERR("output_type %d not sup \r\n",hd_mode->output_type);
					rv = HD_ERR_PARAM;
					goto _HD_VOG2;
				}
    			_hd_videoout_type_str(hd_mode->output_type, type, 16);

           		hdal_flow_log_p("type(%s) output_mode (%d)\n",type,output_mode);

				#if 0
				VDOOUT_MODE mode={0};

				cmd.param = VDOOUT_PARAM_MODE;
				cmd.value = (UINT32)&mode;
				cmd.size = sizeof(VDOOUT_MODE);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
				if((r != 0) || (cmd.rv!=0))
					goto _HD_VOG;

				printf("===>mode=%x %d\r\n", mode.output_type,cmd.rv);


				if(sizeof(VDOOUT_MODE)==sizeof(HD_VIDEOOUT_MODE)) {
					memcpy((void *)hd_mode,(void *)&mode,sizeof(HD_VIDEOOUT_MODE));
				} else {
					rv = HD_ERR_USER; //lib header not sync
					goto _HD_VOG2;
				}

				switch(mode.output_type){
				case VDOOUT_TV_PAL:
					hd_mode->output_type = HD_COMMON_VIDEO_OUT_CVBS;
					hd_mode->output_mode.cvbs=HD_VIDEOOUT_CVBS_PAL;
				break;
				case VDOOUT_TV_NTSC:
					hd_mode->output_type = HD_COMMON_VIDEO_OUT_CVBS;
					hd_mode->output_mode.cvbs=HD_VIDEOOUT_CVBS_NTSC;
				break;
				case VDOOUT_HDMI:
					hd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
				break;
				case VDOOUT_PANEL:
					hd_mode->output_type = HD_COMMON_VIDEO_OUT_LCD;
				break;
				default:
					DBG_ERR("output_type %d not sup \r\n",mode.output_type);
					rv = HD_ERR_PARAM;
					goto _HD_VOG2;
				}
				#endif
			}
			break;
			case HD_VIDEOOUT_PARAM_FB_FMT:
			{
				HD_FB_FMT* hd_fb = (HD_FB_FMT*)p_param;
				INT32 id = _hd_fb_get_lyr(hd_fb->fb_id);
				if(id>0) {
					hd_fb->fmt = _hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id];
				} else {
					DBG_ERR("fb_id %d\r\n",hd_fb->fb_id);
                    rv = HD_ERR_PARAM;
				}
                hdal_flow_log_p("fb_id(%d) fmt(0x%x)\n",hd_fb->fb_id,hd_fb->fmt);
			}
			break;
			case HD_VIDEOOUT_PARAM_FB_ENABLE:
			{
				HD_FB_ENABLE* hd_fb = (HD_FB_ENABLE*)p_param;
				INT32 id = _hd_fb_get_lyr(hd_fb->fb_id);
				if(id>0) {
					hd_fb->enable = _hd_vout_ctx[dev_id].fb_en[hd_fb->fb_id];
				} else {
					DBG_ERR("fb_id %d\r\n",hd_fb->fb_id);
                    rv = HD_ERR_PARAM;
				}

                hdal_flow_log_p("fb_id(%d) enable (%d)\n",hd_fb->fb_id,hd_fb->enable);

			}
			break;
			case HD_VIDEOOUT_PARAM_FB_ATTR:
			{
				HD_FB_ATTR* hd_fb = (HD_FB_ATTR*)p_param;
				INT32 id = _hd_fb_get_lyr(hd_fb->fb_id);
				if(id>0) {
					memcpy((void *)hd_fb,(void *)&_hd_vout_ctx[dev_id].fb_attr[hd_fb->fb_id],sizeof(HD_FB_ATTR));
				} else {
					DBG_ERR("fb_id %d\r\n",hd_fb->fb_id);
                    rv = HD_ERR_PARAM;
				}

                hdal_flow_log_p("fb_id(%d) alpha_blend(%d) alpha_1555(%d) colorkey(%d) r_ckey(0x%x) g_ckey(0x%x) b_ckey(0x%x)\n",hd_fb->fb_id,hd_fb->alpha_blend,hd_fb->alpha_1555,hd_fb->colorkey_en,hd_fb->r_ckey,hd_fb->g_ckey,hd_fb->b_ckey);

			}
			break;
            case HD_VIDEOOUT_PARAM_FB_DIM:
            {
				HD_FB_DIM* hd_fb = (HD_FB_DIM*)p_param;
				INT32 id = _hd_fb_get_lyr(hd_fb->fb_id);
				if(id>0) {
                    if(!_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]){
    					DBG_ERR("set fb fmt first\r\n");
                        rv = HD_ERR_NOT_ALLOW;
						goto _HD_VOG2;
                    }

					memcpy((void *)hd_fb,(void *)&_hd_vout_ctx[dev_id].fb_dim[hd_fb->fb_id],sizeof(HD_FB_DIM));
				} else {
					DBG_ERR("fb_id %d\r\n",hd_fb->fb_id);
                    rv = HD_ERR_PARAM;
				}

          		hdal_flow_log_p("fb_id(%d) w(%u) h(%u) out (%u,%u,%u,%u) fmt %x\n",hd_fb->fb_id,(UINT)hd_fb->input_dim.w,(UINT)hd_fb->input_dim.h,\
                    (UINT)hd_fb->output_rect.x,(UINT)hd_fb->output_rect.y,(UINT)hd_fb->output_rect.w,(UINT)hd_fb->output_rect.h,_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]);

			}
			break;
			case HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE:
			{
				HD_FB_PALETTE_TBL* hd_fb = (HD_FB_PALETTE_TBL*)p_param;

				if(hd_fb->fb_id!=HD_FB0){
					DBG_ERR("not sup fb_id %d \r\n",hd_fb->fb_id);
					rv = HD_ERR_PARAM;
					goto _HD_VOG2;
				}
				#if 0
				VDOOUT_PALETTE vdo_lyr = {0};

				cmd.param = VDOOUT_PARAM_PALETTE_TABLE;
				cmd.value = (UINT32)&vdo_lyr;
				cmd.size = sizeof(VDOOUT_PALETTE);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
				if((r != 0) || (cmd.rv!=0))
					goto _HD_VOG;
				hd_fb->table_size = vdo_lyr.number;

				#endif

				memcpy((void*)hd_fb,(void*)&_hd_vout_ctx[dev_id].fb_palette,sizeof(HD_FB_PALETTE_TBL));

                hdal_flow_log_p("fb_id(%d) table_size(%d) p_table(0x%x)\n",hd_fb->fb_id,hd_fb->table_size,(UINT32)hd_fb->p_table);
 			}
			break;
#if defined(_BSP_NA51000_)
            case HD_VIDEOOUT_PARAM_HDMI_ABILITY:
            {
				HD_VIDEOOUT_HDMI_ABILITY* hd_hdmi_abi = (HD_VIDEOOUT_HDMI_ABILITY*)p_param;
				VDOOUT_HDMI_ABILITY hdmi_abi = {0};

				cmd.param = VDOOUT_PARAM_HDMI_ABILITY;
				cmd.value = (UINT32)&hdmi_abi;
				cmd.size = sizeof(VDOOUT_HDMI_ABILITY);

				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);

                if(r==0)
                {
                UINT32 ii=0;
                hd_hdmi_abi->len = hdmi_abi.len;
                for (ii = 0; ii < hdmi_abi.len; ii++) {
                    //DBG_DUMP("hd Ability %ld: %ld\r\n",(INT32)ii,(INT32)hdmi_abi.video_abi[ii].video_id);
                    hd_hdmi_abi->video_id[ii] = hdmi_abi.video_abi[ii].video_id;
                }
				}
                goto _HD_VOG;
			}
            break;
#endif
			default: DBG_ERR("ctrl not sup %d,check path and param id\r\n",id);rv = HD_ERR_PARAM; break;
			}
		} else {
			switch(id) {
            case HD_VIDEOOUT_PARAM_FUNC_CONFIG:
            {
				HD_VIDEOOUT_FUNC_CONFIG* hd_cfg = (HD_VIDEOOUT_FUNC_CONFIG*)p_param;
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VOG2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VOG2;}

				memcpy((void *)hd_cfg,(void *)&_hd_vout_ctx[dev_id].func_cfg,sizeof(HD_VIDEOOUT_FUNC_CONFIG));

                hdal_flow_log_p("hd_cfg.in_func (0x%x)\n",hd_cfg->in_func);
			}
            break;
			case HD_VIDEOOUT_PARAM_IN_WIN_ATTR:
			{
				HD_VIDEOOUT_WIN_ATTR *p_user = (HD_VIDEOOUT_WIN_ATTR*)p_param;

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VOG2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VOG2;}
				memcpy((void *)p_user,(void *)&_hd_vout_ctx[dev_id].vdoout_win,sizeof(HD_VIDEOOUT_WIN_ATTR));

				hdal_flow_log_p("visible(%d) rect(%d,%d,%d,%d) layer %d\n", p_user->visible, p_user->rect.x, p_user->rect.y, p_user->rect.w, p_user->rect.h,p_user->layer);

				#if 0
				cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = VDOOUT_PARAM_WIN_ATTR;
				cmd.value = (UINT32)p_param;
				cmd.size = sizeof(HD_VIDEOOUT_WIN_ATTR);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
				goto _HD_VOG;
				#endif
			}
			break;
			case HD_VIDEOOUT_PARAM_IN:
			{
				HD_VIDEOOUT_IN *p_user = (HD_VIDEOOUT_IN*)p_param;

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VOG2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VOG2;}
				memcpy((void *)p_user,(void *)&_hd_vout_ctx[dev_id].vdoout_in,sizeof(HD_VIDEOOUT_IN));

				hdal_flow_log_p("dim(%ux%u) pxlfmt(0x%08x) dir(0x%08x)\n", p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->dir);

				#if 0
				cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = VDOOUT_PARAM_WIN_ATTR;
				cmd.value = (UINT32)p_param;
				cmd.size = sizeof(HD_VIDEOOUT_WIN_ATTR);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
				goto _HD_VOG;
				#endif
			}
			break;
			default: DBG_ERR("path not sup %d,check path and param id\r\n",id);rv = HD_ERR_PARAM; break;
			}
		}
		if(rv != HD_OK) {
			goto _HD_VOG2;
		}
		r = 0; //OK
	}
_HD_VOG2:
	unlock_ctx();
	return rv;
_HD_VOG:
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

HD_RESULT hd_videoout_set(HD_PATH_ID path_id, HD_VIDEOOUT_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
    UINT32 dev_id = self_id - HD_DEV_BASE;

	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if (!_hd_common_is_new_flow()) {
		if (_hd_common_get_pid() > 0) {
			DBG_ERR("system fail, not support\r\n");
			return HD_ERR_NOT_SUPPORT;
		}
	}
    if (_hd_videoout_is_init()== FALSE) {
    	return HD_ERR_UNINIT;
    }
    if(dev_id>= _max_dev_count){
        return HD_ERR_LIMIT;
    }

	lock_ctx();
    if((_hd_vout_ctx[dev_id].start_tag != 0xAA55AA55)||
    (_hd_vout_ctx[dev_id].end_tag != 0xAA55AA55)){
        DBG_ERR("set dev_id %lx id %d tag err %lx %lx \r\n",dev_id,id,_hd_vout_ctx[dev_id].start_tag,_hd_vout_ctx[dev_id].end_tag);
		unlock_ctx();
        return HD_ERR_BAD_DATA;
    }
	unlock_ctx();
    {
		CHAR  param_name[20];
		_hd_videoout_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videoout_set(%s):\n", param_name);
		hdal_flow_log_p("    path_id(0x%x) ", path_id);
	}

	if (dev_fd <= 0) {
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
			return _hd_osg_set(self_id, out_id, osg_in_id, id, p_param);
		} else if(osg_out_id) {
			_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
			_HD_CONVERT_IN_ID(in_id, rv);	if(rv != HD_OK) {	return rv;}
			return _hd_osg_set(self_id, in_id, osg_out_id, id, p_param);
		}
	}

	//DBG_IND("path_id %lx param_id %d in_id %d\r\n",path_id,id,in_id);
	{
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}

		lock_ctx();

		if(ctrl_id == HD_CTRL) {
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			switch(id) {
			case HD_VIDEOOUT_PARAM_MODE:
			{
				HD_VIDEOOUT_MODE* hd_mode = (HD_VIDEOOUT_MODE*)p_param;
				VDOOUT_MODE mode={0};
                char type[16]={0};
                UINT32 output_mode =0;
				if(sizeof(VDOOUT_MODE)==sizeof(HD_VIDEOOUT_MODE)) {
					memcpy((void *)&mode,(void *)hd_mode,sizeof(HD_VIDEOOUT_MODE));
				} else {
					rv = HD_ERR_USER; //lib header not sync
					goto _HD_VOS2;
				}

				switch(hd_mode->output_type){
				case HD_COMMON_VIDEO_OUT_CVBS:
					if(hd_mode->output_mode.cvbs==HD_VIDEOOUT_CVBS_PAL)
						mode.output_type = VDOOUT_TV_PAL;
					else
						mode.output_type = VDOOUT_TV_NTSC;
                    output_mode = hd_mode->output_mode.cvbs;
				break;
				case HD_COMMON_VIDEO_OUT_HDMI:
					mode.output_type = VDOOUT_HDMI;
                    output_mode = hd_mode->output_mode.hdmi;
				break;
				case HD_COMMON_VIDEO_OUT_LCD:
					mode.output_type = VDOOUT_PANEL;
                    output_mode = hd_mode->output_mode.lcd;
				break;
				default:
					DBG_ERR("output_type %d not sup \r\n",hd_mode->output_type);
					rv = HD_ERR_PARAM;
					goto _HD_VOS2;
				}

    			_hd_videoout_type_str(hd_mode->output_type, type, 16);

           		hdal_flow_log_p("type(%s) output_mode (%d)\n",type,output_mode);

				cmd.param = VDOOUT_PARAM_MODE;
				cmd.value = (UINT32)&mode;
				cmd.size = sizeof(VDOOUT_MODE);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r==0){
                    memcpy((void *)&_hd_vout_ctx[dev_id].vdoout_mode,(void *)hd_mode,sizeof(HD_VIDEOOUT_MODE));
                }
				goto _HD_VOS;
			}
			break;
			case HD_VIDEOOUT_PARAM_FB_ENABLE:
			{
				HD_FB_ENABLE* hd_fb = (HD_FB_ENABLE*)p_param;
				VDOOUT_LYR_ENABLE vdo_lyr = {0};
				INT32 id = _hd_fb_get_lyr(hd_fb->fb_id);
				if(id>0) {
					vdo_lyr.layer_id = id;
				} else {
					rv = HD_ERR_NOT_SUPPORT;
					goto _HD_VOS2;
				}

           		hdal_flow_log_p("fb_id(%d) enable (%d)\n",hd_fb->fb_id,hd_fb->enable);

				vdo_lyr.enable= hd_fb->enable;
				cmd.param = VDOOUT_PARAM_LYR_ENABLE;
				cmd.value = (UINT32)&vdo_lyr;
				cmd.size = sizeof(VDOOUT_LYR_ENABLE);

				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r==0){
				    _hd_vout_ctx[dev_id].fb_en[hd_fb->fb_id]	= hd_fb->enable;
                }
				goto _HD_VOS;
			}
			break;
			case HD_VIDEOOUT_PARAM_FB_FMT:
			{
				HD_FB_FMT* hd_fb = (HD_FB_FMT*)p_param;
				VDOOUT_LYR_FMT vdo_lyr = {0};
				INT32 id = _hd_fb_get_lyr(hd_fb->fb_id);
				INT32 fmt = _hd_fb_get_fmt(hd_fb->fb_id,hd_fb->fmt);
				if(id>0) {
					vdo_lyr.layer_id = id;
				} else {
					rv = HD_ERR_NOT_SUPPORT;
				}

				if(fmt>0) {
					vdo_lyr.fmt = fmt;
				} else {
					rv = HD_ERR_NOT_SUPPORT;
					goto _HD_VOS2;
				}

                hdal_flow_log_p("fb_id(%d) fmt(0x%x)\n",hd_fb->fb_id,hd_fb->fmt);

				cmd.param = VDOOUT_PARAM_LYR_FMT;
				cmd.value = (UINT32)&vdo_lyr;
				cmd.size = sizeof(VDOOUT_LYR_FMT);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r==0){
				    _hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]	= hd_fb->fmt;
                }
                goto _HD_VOS;
			}
			break;

			case HD_VIDEOOUT_PARAM_FB_ATTR:
			{
				HD_FB_ATTR* hd_fb = (HD_FB_ATTR*)p_param;
				VDOOUT_LYR_BLEND vdo_lyr = {0};
				VDOOUT_LYR_COLORKEY vdo_color = {0};
                INT32 blend_type=0;
				INT32 id = _hd_fb_get_lyr(hd_fb->fb_id);
				if(id>0) {
					vdo_lyr.layer_id = id;
				} else {
					rv = HD_ERR_NOT_SUPPORT;
					goto _HD_VOS2;
				}

                if(!_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]){
					DBG_ERR("set fb fmt first\r\n");
                    return HD_ERR_NOT_ALLOW;
                }

                blend_type = _hd_fb_get_blend(_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]);
                if(blend_type>0) {
    				vdo_lyr.type = blend_type;
    			} else {
                    rv = HD_ERR_PARAM;
					goto _HD_VOS2;
    			}

				if(_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]==HD_VIDEO_PXLFMT_ARGB1555)
					vdo_lyr.global_alpha = hd_fb->alpha_blend;
				else
					vdo_lyr.global_alpha = hd_fb->alpha_1555;

                hdal_flow_log_p("fb_id(%d) alpha_blend(%d) alpha_1555(%d) colorkey(%d) r_ckey(0x%x) g_ckey(0x%x) b_ckey(0x%x)\n",hd_fb->fb_id,hd_fb->alpha_blend,hd_fb->alpha_1555,hd_fb->colorkey_en,hd_fb->r_ckey,hd_fb->g_ckey,hd_fb->b_ckey);

				cmd.param = VDOOUT_PARAM_LYR_BLEND;
				cmd.value = (UINT32)&vdo_lyr;
				cmd.size = sizeof(VDOOUT_LYR_BLEND);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if(r!=0 ||cmd.rv!=0)
					goto _HD_VOS;
				vdo_color.layer_id = id;
                if(hd_fb->colorkey_en)
    				vdo_color.colorkey_op = VDOOUT_KEY_OP_YEQUKEY;
                else
                    vdo_color.colorkey_op = VDOOUT_KEY_OP_OFF;

				vdo_color.key_r = _hd_fb_get_fmt_color(_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id],hd_fb->r_ckey,0);
				vdo_color.key_g = _hd_fb_get_fmt_color(_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id],hd_fb->g_ckey,1);
				vdo_color.key_b = _hd_fb_get_fmt_color(_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id],hd_fb->b_ckey,0);
				cmd.param = VDOOUT_PARAM_LYR_COLORKEY;
				cmd.value = (UINT32)&vdo_color;
				cmd.size = sizeof(VDOOUT_LYR_COLORKEY);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r==0){
				    memcpy((void *)&_hd_vout_ctx[dev_id].fb_attr[hd_fb->fb_id],(void *)hd_fb,sizeof(HD_FB_ATTR));
                }
				goto _HD_VOS;
			}
			break;
            case HD_VIDEOOUT_PARAM_FB_DIM:
            {
				HD_FB_DIM* hd_fb = (HD_FB_DIM*)p_param;
				VDOOUT_LYR_DIM vdo_lyr = {0};

				INT32 id = _hd_fb_get_lyr(hd_fb->fb_id);
				if(id>0) {
					vdo_lyr.layer_id = id;
				} else {
					rv = HD_ERR_NOT_SUPPORT;
					goto _HD_VOS2;
				}

                if(!_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]){
					DBG_ERR("set fb fmt first\r\n");
                    rv = HD_ERR_NOT_ALLOW;
					goto _HD_VOS2;
                }

           		hdal_flow_log_p("fb_id(%d) w(%u) h(%u) out (%u,%u,%u,%u) fmt %x\n",hd_fb->fb_id,(UINT)hd_fb->input_dim.w,(UINT)hd_fb->input_dim.h,\
                    (UINT)hd_fb->output_rect.x,(UINT)hd_fb->output_rect.y,(UINT)hd_fb->output_rect.w,(UINT)hd_fb->output_rect.h,_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]);

                if((!hd_fb->input_dim.w)||(!hd_fb->input_dim.h)){
					rv = HD_ERR_PARAM;
					goto _HD_VOS2;
                }
                if((!hd_fb->output_rect.w)||(!hd_fb->output_rect.h)) {
                    rv = HD_ERR_PARAM;
					goto _HD_VOS2;
                }

                if((hd_fb->input_dim.w>=hd_fb->output_rect.w*2)||(hd_fb->input_dim.h>=hd_fb->output_rect.h*2)) {
                    DBG_ERR("w(%d) h(%d),win (%d,%d,%d,%d) scale down >2\r\n",(int)hd_fb->input_dim.w,(int)hd_fb->input_dim.h,\
                    (int)hd_fb->output_rect.x,(int)hd_fb->output_rect.y,(int)hd_fb->output_rect.w,(int)hd_fb->output_rect.h);
                    rv = HD_ERR_PARAM;
					goto _HD_VOS2;
                }

                if((hd_fb->output_rect.x>=hd_fb->output_rect.w)||(hd_fb->output_rect.y>=hd_fb->output_rect.h)) {
                    rv = HD_ERR_PARAM;
					goto _HD_VOS2;
            	}

				vdo_lyr.in_dim.w = hd_fb->input_dim.w;
				vdo_lyr.in_dim.h = hd_fb->input_dim.h;
				vdo_lyr.out_rect.x = hd_fb->output_rect.x;
				vdo_lyr.out_rect.y = hd_fb->output_rect.y;
				vdo_lyr.out_rect.w = hd_fb->output_rect.w;
				vdo_lyr.out_rect.h = hd_fb->output_rect.h;
				vdo_lyr.fmt = _hd_fb_get_fmt(hd_fb->fb_id,_hd_vout_ctx[dev_id].fb_format[hd_fb->fb_id]);

				cmd.param = VDOOUT_PARAM_LYR_DIM;
				cmd.value = (UINT32)&vdo_lyr;
				cmd.size = sizeof(VDOOUT_LYR_DIM);

				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r==0){
				    memcpy((void *)&_hd_vout_ctx[dev_id].fb_dim[hd_fb->fb_id],(void *)hd_fb,sizeof(HD_FB_DIM));
                }
				goto _HD_VOS;
			}
            break;
			case HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE:
			{
				HD_FB_PALETTE_TBL* hd_fb = (HD_FB_PALETTE_TBL*)p_param;
				VDOOUT_PALETTE vdo_lyr = {0};
				if(hd_fb->fb_id!=HD_FB0){
					DBG_ERR("fb_id %d not sup %d\r\n",hd_fb->fb_id,id);
					rv = HD_ERR_PARAM;
					goto _HD_VOS2;
				}

                if((!hd_fb->table_size)||(hd_fb->table_size>256)) {
                    DBG_ERR("table size %d (1~256)\r\n",hd_fb->table_size);
                    rv = HD_ERR_PARAM;
					goto _HD_VOS2;
                }
                if(!hd_fb->p_table) {
                    DBG_ERR("table 0\r\n");
                    rv = HD_ERR_PARAM;
					goto _HD_VOS2;
                }
				vdo_lyr.start = 0;
				vdo_lyr.number = hd_fb->table_size;
				memcpy((void*)&vdo_lyr.p_pale_entry[0],(void*)hd_fb->p_table,sizeof(UINT32)*VDOOUT_PALETTE_MAX);
				memcpy((void*)&_hd_vout_ctx[dev_id].palette_table[0],(void*)hd_fb->p_table,sizeof(UINT32)*VDOOUT_PALETTE_MAX);
                _hd_vout_ctx[dev_id].fb_palette.p_table = _hd_vout_ctx[dev_id].palette_table;

                hdal_flow_log_p("fb_id(%d) table_size(%d) p_table(0x%x)\n",hd_fb->fb_id,hd_fb->table_size,(UINT32)hd_fb->p_table);

				cmd.param = VDOOUT_PARAM_PALETTE_TABLE;
				cmd.value = (UINT32)&vdo_lyr;
				cmd.size = sizeof(VDOOUT_PALETTE);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r==0){
				    memcpy((void*)&_hd_vout_ctx[dev_id].fb_palette,(void*)hd_fb,sizeof(HD_FB_PALETTE_TBL));
                }
				goto _HD_VOS;
			}
			break;
            case HD_VIDEOOUT_PARAM_CLEAR_WIN:
            {
                cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOOUT_PARAM_CLEAR_WIN;
				cmd.value = 1;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				goto _HD_VOS;
            }
            break;
			default: DBG_ERR("ctrl not sup %d,check path and param id\r\n",id);rv = HD_ERR_PARAM; break;

			}
		} else {
			switch(id) {
            case HD_VIDEOOUT_PARAM_FUNC_CONFIG:
            {
				HD_VIDEOOUT_FUNC_CONFIG* hd_cfg = (HD_VIDEOOUT_FUNC_CONFIG*)p_param;
				VDOOUT_FUNC_CFG vout_cfg={0};
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
                ISF_ATTR t_attr;
#endif
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VOS2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VOS2;}

           		hdal_flow_log_p("vout_cfg in_func (0x%x) ddr_id (%d)\n",hd_cfg->in_func,hd_cfg->ddr_id);

                if(sizeof(VDOOUT_FUNC_CFG)==sizeof(HD_VIDEOOUT_FUNC_CONFIG)){
    				memcpy((void *)&vout_cfg,(void *)hd_cfg,sizeof(HD_VIDEOOUT_MODE));
                } else {
                    rv = HD_ERR_USER; //lib header not sync
                    goto _HD_VOS2;
                }
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
				t_attr.attr = hd_cfg->ddr_id;
				cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = ISF_UNIT_PARAM_ATTR;
				cmd.value = (UINT32)&t_attr;
				cmd.size = sizeof(ISF_ATTR);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VOS;
#endif
                cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = VDOOUT_PARAM_CFG;
				cmd.value = (UINT32)&vout_cfg;
				cmd.size = sizeof(VDOOUT_FUNC_CFG);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r ==0 ){
				    memcpy((void *)&_hd_vout_ctx[dev_id].func_cfg,(void *)hd_cfg,sizeof(HD_VIDEOOUT_FUNC_CONFIG));
                }
				goto _HD_VOS;
			}
            break;
			case HD_VIDEOOUT_PARAM_IN_WIN_ATTR:
			{
				HD_VIDEOOUT_WIN_ATTR* p_user = (HD_VIDEOOUT_WIN_ATTR*)p_param;
				ISF_VDO_WIN win = {0};

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VOS2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VOS2;}


				hdal_flow_log_p("visible(%d) rect(%d,%d,%d,%d) layer %d\n", p_user->visible, p_user->rect.x, p_user->rect.y, p_user->rect.w, p_user->rect.h,p_user->layer);

				cmd.dest = ISF_PORT(self_id, in_id);

                if((!p_user->rect.w)||(!p_user->rect.h)) {
                    rv = HD_ERR_PARAM;
					goto _HD_VOS2;
                }
                if(p_user->layer==HD_LAYER1) {
				win.window.x = p_user->rect.x;
				win.window.y = p_user->rect.y;
				win.window.w = p_user->rect.w;
				win.window.h = p_user->rect.h;
				win.imgaspect.w = 0;
				win.imgaspect.h = 0;
				cmd.param = ISF_UNIT_PARAM_VDOWIN;
				cmd.value = (UINT32)&win;
				cmd.size = sizeof(ISF_VDO_WIN);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r==0){
				    memcpy((void *)&_hd_vout_ctx[dev_id].vdoout_win,(void *)p_param,sizeof(HD_VIDEOOUT_WIN_ATTR));
                }
    				goto _HD_VOS;
                } else if((p_user->layer > HD_LAYER1) && (p_user->layer < HD_WIN_LAYER_MAX)) {
                    VDOOUT_WND_ATTR vout_merge_cfg={0};


                    vout_merge_cfg.visible = p_user->visible;
    				vout_merge_cfg.rect.x = p_user->rect.x;
    				vout_merge_cfg.rect.y = p_user->rect.y;
    				vout_merge_cfg.rect.w = p_user->rect.w;
    				vout_merge_cfg.rect.h = p_user->rect.h;
                    vout_merge_cfg.layer = p_user->layer;

                    cmd.dest = ISF_PORT(self_id, in_id);
    				cmd.param = VDOOUT_PARAM_WIN_ATTR;
    				cmd.value = (UINT32)&vout_merge_cfg;
    				cmd.size = sizeof(VDOOUT_WND_ATTR);
    				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                    if(r ==0 ){
    				    //memcpy((void *)&_hd_vout_ctx[dev_id].func_cfg,(void *)hd_cfg,sizeof(HD_VIDEOOUT_FUNC_CONFIG));
                    }
    				goto _HD_VOS;

                } else {
                    DBG_ERR("HD_ERR_PARAM\r\n");
                    return HD_ERR_PARAM;
                }
			}
            break;
			case HD_VIDEOOUT_PARAM_IN:
			{
				HD_VIDEOOUT_IN* p_user = (HD_VIDEOOUT_IN*)p_param;
				ISF_VDO_FRAME frame = {0};

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VOS2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VOS2;}


				hdal_flow_log_p("dim(%ux%u) pxlfmt(0x%08x) dir(0x%08x)\n", p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->dir);

				cmd.dest = ISF_PORT(self_id, in_id);

                if((!p_user->dim.w)||(!p_user->dim.h)) {
                    rv = HD_ERR_PARAM;
					goto _HD_VOS2;
                }

				/*
				p_user->func
				*/
				if(_hd_videoout_chk_vdo_fmt(p_user->pxlfmt)!=0) {
                    rv = HD_ERR_NOT_SUPPORT;
					goto _HD_VOS2;
				}

				if(_hd_videoout_chk_vdo_dir(p_user->dir)!=0) {
                    rv = HD_ERR_NOT_SUPPORT;
					goto _HD_VOS2;
				}

				frame.direct = _hd_videoout_get_dir(p_user->dir);
				frame.imgfmt = p_user->pxlfmt;
				frame.imgsize.w = p_user->dim.w;
				frame.imgsize.h = p_user->dim.h;
				cmd.param = ISF_UNIT_PARAM_VDOFRAME;
				cmd.value = (UINT32)&frame;
				cmd.size = sizeof(ISF_VDO_FRAME);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
                if(r==0){
				    memcpy((void *)&_hd_vout_ctx[dev_id].vdoout_in,(void *)p_user,sizeof(HD_VIDEOOUT_IN));
                }
				goto _HD_VOS;
			}
			break;

			default: DBG_ERR("path not sup %d,check path and param id\r\n",id);rv = HD_ERR_PARAM; break;
			}
		}
		//return rv;
	}
_HD_VOS2:
	unlock_ctx();
	return rv;
_HD_VOS:
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

HD_RESULT hd_videoout_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame, HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		DBG_ERR("Invalid dev_fd=%d\r\n", dev_fd);
		return HD_ERR_UNINIT;
	}
	if ((_hd_common_get_pid() == 0) && (_hd_videoout_is_init()== FALSE)) {
		return HD_ERR_UNINIT;
	}

	if (p_video_frame == 0) {
		DBG_ERR("p_video_frame is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	if (p_video_frame->phy_addr[0] == 0) {
        DBG_ERR("Check HD_VIDEO_FRAME phy_addr[0] is NULL.\n");
        return HD_ERR_NULL_PTR;
	}
	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};
		VDO_FRAME vdo_frm = {0};

		// set vdo_frm info
		vdo_frm.sign       = p_video_frame->sign;
    	vdo_frm.p_next = NULL;
		vdo_frm.pxlfmt     = p_video_frame->pxlfmt;
		vdo_frm.size.w     = p_video_frame->dim.w;
		vdo_frm.size.h     = p_video_frame->dim.h;
	    vdo_frm.count      = p_video_frame->count;
		vdo_frm.timestamp  = p_video_frame->timestamp;
    	vdo_frm.pw[VDO_PINDEX_Y] = p_video_frame->pw[HD_VIDEO_PINDEX_Y];
    	vdo_frm.pw[VDO_PINDEX_UV] = p_video_frame->pw[HD_VIDEO_PINDEX_UV];
    	vdo_frm.ph[VDO_PINDEX_Y] = p_video_frame->ph[HD_VIDEO_PINDEX_Y];
    	vdo_frm.ph[VDO_PINDEX_UV] = p_video_frame->ph[HD_VIDEO_PINDEX_UV];
    	vdo_frm.loff[VDO_PINDEX_Y] = p_video_frame->loff[HD_VIDEO_PINDEX_Y];
    	vdo_frm.loff[VDO_PINDEX_UV] = p_video_frame->loff[HD_VIDEO_PINDEX_UV];
    	vdo_frm.phyaddr[VDO_PINDEX_Y] = p_video_frame->phy_addr[HD_VIDEO_PINDEX_Y];
    	vdo_frm.phyaddr[VDO_PINDEX_UV] = p_video_frame->phy_addr[HD_VIDEO_PINDEX_UV];
		vdo_frm.addr[VDO_PINDEX_Y]     = 0;  // kflow_videoenc will identify ((addr == 0) && (loff != 0)) to know that it's coming from HDAL user push_in
		vdo_frm.addr[VDO_PINDEX_UV]    = 0;  //   and kflow_videoenc will call nvtmpp_sys_pa2va() in kernel to convert phyaddr to viraddr

		// set isf_data
		data.sign         = MAKEFOURCC('I','S','F','D');
		data.h_data       = p_video_frame->blk;
		memcpy(&data.desc, &vdo_frm, sizeof(VDO_FRAME));

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
    			DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
    			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
    		}
    	}
	}
	return rv;
}

HD_RESULT hd_videoout_pull_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoout_release_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videoout_uninit(VOID)
{

	hdal_flow_log_p("hd_videoout_uninit\n");

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

	if (_hd_vout_ctx == NULL) {
		return HD_ERR_UNINIT;
	}

	if (_hd_common_get_pid() == 0) {

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

	}

	//uninit lib
	{
		free_ctx(_hd_vout_ctx);
		_hd_vout_ctx = NULL;
	}

	return HD_OK;
}

int _hd_videoout_is_init(VOID)
{
	return (_hd_vout_ctx != NULL);
}
//////////////////////////////////////

static INT32 _hd_fb_get_lyr(HD_FB_ID fb_id)
{
	switch(fb_id){
	case HD_FB0:
		return VDOOUT_LYR_OSD1;
	break;
#if 0
	case HD_FB1:
		return VDOOUT_LYR_FD;
	break;
	case HD_FB2:
		return VDOOUT_LYR_VDO1;
	break;
#endif
	default:
		DBG_ERR("not sup lyr FB%d\r\n",fb_id);
		return -1;
	}
}

static INT32 _hd_fb_get_fmt(HD_FB_ID fb_id,HD_VIDEO_PXLFMT fmt)
{
	switch(fmt){
	case HD_VIDEO_PXLFMT_ARGB1555:
		return VDOOUT_BUFTYPE_ARGB1555;
	break;
	case HD_VIDEO_PXLFMT_ARGB4444:
		return VDOOUT_BUFTYPE_ARGB4444;
	break;
	case HD_VIDEO_PXLFMT_ARGB8888:
		return VDOOUT_BUFTYPE_ARGB8888;
	break;
	case HD_VIDEO_PXLFMT_ARGB8565:
		return VDOOUT_BUFTYPE_ARGB8565;
	break;
	case HD_VIDEO_PXLFMT_I8:
		return VDOOUT_BUFTYPE_PAL8;
	break;
	default:
		DBG_ERR("FB%d not sup fmt %x\r\n",fb_id,fmt);
		return -1;
	}
}

static INT32 _hd_fb_get_blend(HD_VIDEO_PXLFMT fmt)
{
	switch(fmt){
	case HD_VIDEO_PXLFMT_ARGB4444:
	case HD_VIDEO_PXLFMT_ARGB1555:
	case HD_VIDEO_PXLFMT_ARGB8888:
	case HD_VIDEO_PXLFMT_ARGB8565:
	case HD_VIDEO_PXLFMT_I8:
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	return VDOOUT_BLEND_TYPE_SOURCE_BACK;
#else
	return VDOOUT_BLEND_TYPE_SOURCE;
#endif
	case HD_VIDEO_PXLFMT_YUV422:
	case HD_VIDEO_PXLFMT_YUV420:
		return VDOOUT_BLEND_TYPE_GLOBAL;
	break;
	default:
		DBG_ERR("not sup fmt 0x%x\r\n",fmt);
		return -1;
	}
}

#define VOUT_DIR_SUP (HD_VIDEO_DIR_MIRRORX    \
|HD_VIDEO_DIR_MIRRORY    \
|HD_VIDEO_DIR_MIRRORXY   \
|HD_VIDEO_DIR_ROTATE_0   \
|HD_VIDEO_DIR_ROTATE_90  \
|HD_VIDEO_DIR_ROTATE_180 \
|HD_VIDEO_DIR_ROTATE_270 \
|HD_VIDEO_DIR_ROTATE_360 )

static UINT32 _hd_videoout_chk_vdo_dir(HD_VIDEO_DIR dir)
{
    if(dir & (~VOUT_DIR_SUP)) {
        DBG_ERR("dir %x not sup\r\n",(dir & (~VOUT_DIR_SUP)));
        return -1;
    } else {
        return 0;
    }
}
static UINT32 _hd_videoout_get_dir(HD_VIDEO_DIR dir)
{
	UINT32 vdoout_dir = 0;
    UINT32 tmp_dir=0;
    if(!dir)
        return VDOOUT_DIR_0;

    tmp_dir = dir&~HD_VIDEO_DIR_ROTATE_MASK;
    if(tmp_dir==HD_VIDEO_DIR_MIRRORX) {
        vdoout_dir|= VDOOUT_DIR_HORZ;
	} else if(tmp_dir==HD_VIDEO_DIR_MIRRORY) {
        vdoout_dir|= VDOOUT_DIR_VERT;
	} else if(tmp_dir==HD_VIDEO_DIR_MIRRORXY) {
	    vdoout_dir|= VDOOUT_DIR_180;
    }

    tmp_dir = dir&HD_VIDEO_DIR_ROTATE_MASK;
	if(tmp_dir==HD_VIDEO_DIR_ROTATE_90) {
        vdoout_dir|= VDOOUT_DIR_90;
	} else if(tmp_dir==HD_VIDEO_DIR_ROTATE_270) {
	    vdoout_dir|= VDOOUT_DIR_270;
    } else if(tmp_dir==HD_VIDEO_DIR_ROTATE_180) {
        vdoout_dir|= VDOOUT_DIR_180;
    }

	return vdoout_dir;
}
UINT16 _hd_fb_get_fmt_color(HD_VIDEO_PXLFMT fmt,UINT16 color,UINT32 isG)
{
	switch(fmt){
	case HD_VIDEO_PXLFMT_ARGB4444:
        return (color << 4);
	case HD_VIDEO_PXLFMT_ARGB1555:
        return (color << 3);
	case HD_VIDEO_PXLFMT_ARGB8888:
	case HD_VIDEO_PXLFMT_I8:
        return color;
	case HD_VIDEO_PXLFMT_ARGB8565:
        if(isG)
            return (color<<2);
        else
            return (color<<3);

	break;
	default:
		DBG_ERR("not sup fmt 0x%x\r\n",fmt);
		return 0;
	}
}
static INT32 _hd_videoout_chk_vdo_fmt(HD_VIDEO_PXLFMT fmt)
{
	switch(fmt){
	case HD_VIDEO_PXLFMT_YUV420:
	case HD_VIDEO_PXLFMT_YUV422:
	case HD_VIDEO_PXLFMT_YUV444:
        return 0;

	default:
		DBG_ERR("not sup fmt 0x%x\r\n",fmt);
		return -1;
	}
}

