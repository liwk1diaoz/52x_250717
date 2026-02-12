/**
	@brief Source code of video capture module.\n
	This file contains the functions which is related to video capture in the chip.

	@file hd_videocapture.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME HD_VIDEOCAPTURE
#include "hd_int.h"
#include "hd_logger_p.h"
#include "kflow_videocapture/isf_vdocap.h"

#define HD_VIDEOCAP_SEN_HDR_NONE        0    ///< for not using sensor HDR
#define HD_VIDEOCAP_SEN_HDR_SET1_MAIN   1    ///< indicate this videocapture connect to the main path of sensor HDR set1
#define HD_VIDEOCAP_SEN_HDR_SET1_SUB1   2    ///< indicate this videocapture connect to the sub path 1 of sensor HDR set1
#define HD_VIDEOCAP_SEN_HDR_SET1_SUB2   3    ///< indicate this videocapture connect to the sub path 2 of sensor HDR set1
#define HD_VIDEOCAP_SEN_HDR_SET1_SUB3   4    ///< indicate this videocapture connect to the sub path 3 of sensor HDR set1
#define HD_VIDEOCAP_SEN_HDR_SET2_MAIN   5    ///< indicate this videocapture connect to the main path of sensor HDR set2
#define HD_VIDEOCAP_SEN_HDR_SET2_SUB1   6    ///< indicate this videocapture connect to the sub path 1 of sensor HDR set2

#define HD_VIDEOCAP_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define DEV_BASE		ISF_UNIT_VDOCAP
#define DEV_COUNT		ISF_MAX_VDOCAP
#define IN_BASE		ISF_IN_BASE
#define IN_COUNT		1
#define OUT_BASE		ISF_OUT_BASE
#define OUT_COUNT 	1

#define HD_DEV_BASE	HD_DAL_VIDEOCAP_BASE
#define HD_DEV_MAX	HD_DAL_VIDEOCAP_MAX

#define HD_VIDEOCAP_MAX_DEPTH   4

#define HD_VIDEOCAP_MAX_IN_COUNT   1
#define HD_VIDEOCAP_MAX_OUT_COUNT   1

#if defined(_BSP_NA51000_)
#define HD_VIDEOCAP_MAX_DEVCOUNT   8
#define HD_VIDEOCAP_MAX_SHDR_DEVCOUNT   4
static UINT32 _max_dev_count = 4;
static UINT32 _active_list = 0x0F;//default using vcap0~3
#elif defined(_BSP_NA51055_)
#define HD_VIDEOCAP_MAX_DEVCOUNT    5
#define HD_VIDEOCAP_MAX_SHDR_DEVCOUNT   5
static UINT32 _max_dev_count = 2;
static UINT32 _active_list = 0x03;//default using vcap0~1
#elif defined(_BSP_NA51089_)
#define HD_VIDEOCAP_MAX_DEVCOUNT    3
#define HD_VIDEOCAP_MAX_SHDR_DEVCOUNT   3
static UINT32 _max_dev_count = 2;
static UINT32 _active_list = 0x03;//default using vcap0~1
#endif

#define MAX_ACTIVE_LIST_CNT 8

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
#define SHM_KEY HD_DAL_VIDEOCAP_BASE
static sem_t *g_sem = 0;
#define SEM_NAME "sem_vcap"

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


typedef struct _HD_VCAP_CTX {
	HD_VIDEOCAP_DRV_CONFIG drv_cfg;
	HD_VIDEOCAP_CTRL ctl;
	HD_VIDEOCAP_IN in;
	HD_VIDEOCAP_OUT out;
	HD_VIDEOCAP_CROP crop;
	BOOL drv_configed;
	HD_VIDEOCAP_FUNC_CONFIG func_cfg;
} HD_VCAP_CTX;

static UINT32 _hd_vcap_csz = 0;  //context buffer size
static HD_VCAP_CTX* _hd_vcap_ctx = NULL; //context buffer addr

static BOOL _hd_videocap_chk_vdo_fmt(HD_VIDEO_PXLFMT fmt)
{
	switch(fmt){
	case HD_VIDEO_PXLFMT_RAW14:
	case HD_VIDEO_PXLFMT_RAW14_SHDR2:
	case HD_VIDEO_PXLFMT_RAW14_SHDR3:
	case HD_VIDEO_PXLFMT_RAW14_SHDR4:
	case HD_VIDEO_PXLFMT_NRX8:
	case HD_VIDEO_PXLFMT_NRX10:
	case HD_VIDEO_PXLFMT_NRX14:
	case HD_VIDEO_PXLFMT_NRX14_SHDR2:
	case HD_VIDEO_PXLFMT_NRX14_SHDR3:
	case HD_VIDEO_PXLFMT_NRX14_SHDR4:
	case HD_VIDEO_PXLFMT_NRX16:
		DBG_ERR("not sup fmt 0x%x\r\n",fmt);
        return FALSE;
	default:
		return TRUE;
	}
}

static UINT32 _hd_videocap_devid_to_ctxid(UINT32 dev_id)
{
	UINT32 i;
	UINT32 ctx_id = 0;

	if (dev_id > HD_VIDEOCAP_MAX_DEVCOUNT) {
		DBG_ERR("dev_id(%d) > MAX ID!\r\n", (int)dev_id);
		return 0;
	}
	if (((1 << dev_id) & _active_list) == 0) {
		DBG_ERR("VCAP[%d] is NOT active(0x%02X)\n", (int)dev_id, (unsigned int)_active_list);
		return 0;
	}
	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		if ((1 << i) & _active_list) {
			if (i == dev_id) {
				break;
			} else {
				ctx_id++;
			}
		}
	}
	//DBG_DUMP("[0x%X]dev_id %d -> ctx_id %d \r\n",(unsigned int)_active_list,(int)dev_id,(int)ctx_id);
	return ctx_id;
}
static UINT32 _hd_videocap_get_dev_num(UINT32 map)
{
	UINT32 i, temp;
	UINT32 dev_num = 0;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = map >> i;
		if (temp & 0x1) {
			dev_num++;
		}
	}
	return dev_num;
}
void _hd_videocap_cfg_active(unsigned int *p_active_list)
{
	UINT32 i;
	unsigned int active_list = 0;

	if (!_hd_vcap_ctx) {
		//check if list exceed max device
		for (i = HD_VIDEOCAP_MAX_DEVCOUNT; i < MAX_ACTIVE_LIST_CNT; i++) {
			if (&p_active_list[i] && p_active_list[i]) {
				DBG_WRN("dts vdocap_active_list is larger than built-in max device list num(%d)!\r\n", HD_VIDEOCAP_MAX_DEVCOUNT);
			}
		}
		if (i > MAX_ACTIVE_LIST_CNT) {
			DBG_ERR("vdocap_active_list[] too small!\r\n");
		}

		for (i = 0; i < HD_VIDEOCAP_MAX_DEVCOUNT; i++) {
			if (&p_active_list[i] && p_active_list[i]) {
				active_list |= (1 << i);
			}
		}
		if (active_list & ((1 << HD_VIDEOCAP_MAX_DEVCOUNT) - 1)) {
			_active_list = active_list;
			_max_dev_count = _hd_videocap_get_dev_num(_active_list);
		} else {
			DBG_ERR("No active VCAP!\r\n");
		}
	}
}

static BOOL _hd_videocap_check_dir_support(HD_VIDEO_PXLFMT pxlfmt, HD_VIDEO_DIR dir)
{
	BOOL ret = FALSE;

	if (dir == HD_VIDEO_DIR_NONE) {
		ret = TRUE;
	} else if (HD_VIDEO_PXLFMT_CLASS(pxlfmt) == HD_VIDEO_PXLFMT_CLASS_YUV) {
		if (dir == HD_VIDEO_DIR_MIRRORX || dir == HD_VIDEO_DIR_MIRRORY || dir == HD_VIDEO_DIR_MIRRORXY) {
			ret = TRUE;
		} else {
			DBG_ERR("not support dir = 0x%X\r\n", dir);
		}
	} else {
		DBG_ERR("only support ccir mirro/flip\r\n");
	}

	return ret;
}

static BOOL _hd_videocap_check_shdr_support(HD_VIDEOCAP_DRV_CONFIG *p_drv_cfg)
{
	UINT32 sensor_id = GET_HI_UINT16(p_drv_cfg->sen_cfg.shdr_map);
	UINT32 shdr_map = GET_LO_UINT16(p_drv_cfg->sen_cfg.shdr_map);


//printf("\r\n sen_cfg.shdr_map=0x%X, dev_num=%d, sensor_id=%d, shdr_map=0x%X\r\n", p_drv_cfg->sen_cfg.shdr_map,_hd_videocap_get_dev_num(shdr_map),sensor_id,shdr_map);
	if (p_drv_cfg->sen_cfg.shdr_map) {
		if ((sensor_id > HD_VIDEOCAP_HDR_SENSOR2) || (_hd_videocap_get_dev_num(shdr_map) < 2)) {
			DBG_ERR("SHDR config error(0x%lx).\r\n", p_drv_cfg->sen_cfg.shdr_map);
			return FALSE;
		}
#if defined(_BSP_NA51000_)
		if (sensor_id == HD_VIDEOCAP_HDR_SENSOR1) {
			if (shdr_map > (HD_VIDEOCAP_3 | HD_VIDEOCAP_2 | HD_VIDEOCAP_1 | HD_VIDEOCAP_0)) {
				DBG_ERR("SHDR1 only support VIDEOCAP0~3.\r\n");
				return FALSE;
			}
		} else {//HD_VIDEOCAP_HDR_SENSOR2
			if (p_drv_cfg->sen_cfg.sen_dev.if_type == HD_COMMON_VIDEO_IN_MIPI_CSI) {
				if (shdr_map != ( HD_VIDEOCAP_3 | HD_VIDEOCAP_1)) {
					DBG_ERR("SHDR2 with CSI only support VIDEOCAP1/3.\r\n");
					return FALSE;
				}
			}
		}
#else
		if (sensor_id == HD_VIDEOCAP_HDR_SENSOR1) {
			if (shdr_map & HD_VIDEOCAP_2) {
				DBG_ERR("SHDR1 only support VIDEOCAP0/1/3/4.\r\n");
				return FALSE;
			}
		} else {//HD_VIDEOCAP_HDR_SENSOR2
			if (shdr_map & ( HD_VIDEOCAP_0 | HD_VIDEOCAP_2)) {
				DBG_ERR("SHDR2 with CSI only support VIDEOCAP1/3/4.\r\n");
				return FALSE;
			}
		}
#endif
	}
	return TRUE;
}
static UINT32 _hd_videocap_shdr_main_hdl_to_isf(UINT32 shdr_map)
{
	UINT32 sensor_id = GET_HI_UINT16(shdr_map);
	UINT32 ret = HD_VIDEOCAP_SEN_HDR_NONE;

	if (shdr_map) {
		if (sensor_id == HD_VIDEOCAP_HDR_SENSOR1) {
			ret = HD_VIDEOCAP_SEN_HDR_SET1_MAIN;
		} else {
			ret = HD_VIDEOCAP_SEN_HDR_SET2_MAIN;
		}
	}
	return ret;
}
static UINT32 _hd_videocap_shdr_sub_hdl_to_isf(UINT32 shdr_map, UINT32 seq)
{
	UINT32 sensor_id = GET_HI_UINT16(shdr_map);
	UINT32 ret = HD_VIDEOCAP_SEN_HDR_NONE;

	if (shdr_map) {
		if (sensor_id == HD_VIDEOCAP_HDR_SENSOR1) {
			if (seq == 1) {
				ret = HD_VIDEOCAP_SEN_HDR_SET1_SUB1;
			} else if (seq == 2) {
				ret = HD_VIDEOCAP_SEN_HDR_SET1_SUB2;
			} else if (seq == 3) {
				ret = HD_VIDEOCAP_SEN_HDR_SET1_SUB3;
			}
		} else {
			if (seq == 1) {
				ret = HD_VIDEOCAP_SEN_HDR_SET2_SUB1;
			}
		}
	}
	return ret;
}
static BOOL _hd_videocap_is_shdr_mode(UINT32 dev_id)
{
	BOOL ret = FALSE;

	if (dev_id < HD_VIDEOCAP_MAX_SHDR_DEVCOUNT) {
		UINT32 ctx_id = _hd_videocap_devid_to_ctxid(dev_id);

		if (_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map) {
			if ((1 << dev_id) & _active_list) {
				ret = TRUE;
			}
		}
	}
	return ret;
}
static HD_RESULT hd_videocap_get_path_id(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	UINT32 cur_dev = self_id;
	HD_RESULT rv = HD_OK;

	_HD_CONVERT_SELF_ID(self_id, rv);   if(rv != HD_OK) { DBG_ERR("path_id(N/A)\n"); return rv;}
	_HD_CONVERT_SELF_ID(in_dev_id, rv); if(rv != HD_OK) { DBG_ERR("path_id(N/A)\n"); return rv;}
	if(in_dev_id != self_id) { DBG_ERR("path_id(N/A)\n"); return rv;}
	*p_path_id = HD_VIDEOCAP_PATH(cur_dev, in_id, out_id);
	return HD_OK;
}
static HD_RESULT _hd_videocap_set_shdr_sub_map(UINT32 shdr_map)
{
	ISF_FLOW_IOCTL_PARAM_ITEM sub_path_cmd = {0};
	HD_PATH_ID sub_ctrl_id = 0;
	HD_DAL sub_self_id = 0;
	HD_RESULT sub_ret = HD_OK;
	HD_RESULT rv = HD_OK;
	int r;
	UINT32 i, temp;
	UINT32 map = GET_LO_UINT16(shdr_map);
	UINT32 seq = 0;

	for (i = 0; i < HD_VIDEOCAP_MAX_SHDR_DEVCOUNT; i++) {
		temp  = map >> i;
		if ((temp & 0x1) && ((1 << i) & _active_list)) {
			//only for sub path (seq = 0 is main path)
			if (seq) {
				sub_ret = hd_videocap_open(0, HD_VIDEOCAP_CTRL(i), &sub_ctrl_id); //open this for device control
				if (sub_ret != HD_OK) {
					return sub_ret;
				}

				sub_self_id = HD_GET_DEV(sub_ctrl_id);
				_HD_CONVERT_SELF_ID(sub_self_id, rv); 	if(rv != HD_OK) {	return rv;}

				map = _hd_videocap_shdr_sub_hdl_to_isf(shdr_map, seq);
				sub_path_cmd.dest = ISF_PORT(sub_self_id, ISF_CTRL);
				sub_path_cmd.param = VDOCAP_PARAM_SHDR_MAP;
				sub_path_cmd.value = map;
				sub_path_cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &sub_path_cmd);
				if (r == 0) {
					if (sub_path_cmd.rv) {
						rv = HD_ERR_SYS;
						break;
					}
				} else {
					if(((int)sub_path_cmd.rv <= ISF_ERR_BEGIN) && ((int)sub_path_cmd.rv >= ISF_ERR_END)) {
						rv = sub_path_cmd.rv; // ISF_ERR is exactly the same with HD_ERR
					} else {
						DBG_ERR("system fail, rv=%d\r\n", sub_path_cmd.rv);
						rv = sub_path_cmd.rv; // ISF_ERR is out of range of HD_ERR
					}
					break;
				}
			}
			seq++;
		}
	}
	return rv;
}
static HD_RESULT _hd_videocap_set_shdr_sub_ctrl(UINT32 shdr_map, ISF_FLOW_IOCTL_PARAM_ITEM * p_cmd)
{
	ISF_FLOW_IOCTL_PARAM_ITEM sub_path_cmd = {0};
	HD_PATH_ID sub_ctrl_id = 0;
	HD_DAL sub_self_id = 0;
	HD_RESULT sub_ret = HD_OK;
	HD_RESULT rv = HD_OK;
	int r;
	UINT32 i, temp;
	UINT32 map = GET_LO_UINT16(shdr_map);
	UINT32 seq = 0;

	for (i = 0; i < HD_VIDEOCAP_MAX_SHDR_DEVCOUNT; i++) {
		temp  = map >> i;
		if ((temp & 0x1) && ((1 << i) & _active_list)) {
			//only for sub path (seq = 0 is main path)
			if (seq) {
				sub_ret = hd_videocap_open(0, HD_VIDEOCAP_CTRL(i), &sub_ctrl_id); //open this for device control
				if (sub_ret != HD_OK) {
					return sub_ret;
				}
				sub_self_id = HD_GET_DEV(sub_ctrl_id);
				_HD_CONVERT_SELF_ID(sub_self_id, rv); 	if(rv != HD_OK) {	return rv;}
				memcpy(&sub_path_cmd, p_cmd, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM));
				sub_path_cmd.dest = ISF_PORT(sub_self_id, ISF_CTRL);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &sub_path_cmd);
				if (r == 0) {
					if (sub_path_cmd.rv) {
						rv = HD_ERR_SYS;
						break;
					}
				} else {
					if(((int)sub_path_cmd.rv <= ISF_ERR_BEGIN) && ((int)sub_path_cmd.rv >= ISF_ERR_END)) {
						rv = sub_path_cmd.rv; // ISF_ERR is exactly the same with HD_ERR
					} else {
						DBG_ERR("system fail, rv=%d\r\n", sub_path_cmd.rv);
						rv = sub_path_cmd.rv; // ISF_ERR is out of range of HD_ERR
					}
					break;
				}
			}
			seq++;
		}
	}
	return rv;
}
static HD_RESULT _hd_videocap_set_shdr_sub_path(UINT32 shdr_map, ISF_FLOW_IOCTL_PARAM_ITEM * p_cmd)
{
	ISF_FLOW_IOCTL_PARAM_ITEM sub_path_cmd = {0};
	HD_PATH_ID sub_path_id = 0;
	HD_DAL sub_self_id = 0;
	HD_RESULT sub_ret = HD_OK;
	HD_RESULT rv = HD_OK;
	int r;
	UINT32 i, temp;
	UINT32 map = GET_LO_UINT16(shdr_map);
	UINT32 seq = 0;
	HD_IO out_id = GET_LO_UINT16(p_cmd->dest);

	for (i = 0; i < HD_VIDEOCAP_MAX_SHDR_DEVCOUNT; i++) {
		temp  = map >> i;
		if ((temp & 0x1) && ((1 << i) & _active_list)) {
			//only for sub path (seq = 0 is main path)
			if (seq) {
				sub_ret = hd_videocap_get_path_id(HD_VIDEOCAP_IN(i, 0), HD_VIDEOCAP_OUT(i, 0), &sub_path_id);
				if (sub_ret != HD_OK) {
					return sub_ret;
				}
				sub_self_id = HD_GET_DEV(sub_path_id);
				_HD_CONVERT_SELF_ID(sub_self_id, rv); 	if(rv != HD_OK) {	return rv;}
				memcpy(&sub_path_cmd, p_cmd, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM));
				sub_path_cmd.dest = ISF_PORT(sub_self_id, out_id);
				//printf("\r\nset_shdr_sub_path:dest=0x%X, param=0x%X\r\n", sub_path_cmd.dest, sub_path_cmd.param);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &sub_path_cmd);
				if (r == 0) {
					if (sub_path_cmd.rv) {
						rv = HD_ERR_SYS;
						break;
					}
				} else {
					if(((int)sub_path_cmd.rv <= ISF_ERR_BEGIN) && ((int)sub_path_cmd.rv >= ISF_ERR_END)) {
						rv = sub_path_cmd.rv; // ISF_ERR is exactly the same with HD_ERR
					} else {
						DBG_ERR("system fail, rv=%d\r\n", sub_path_cmd.rv);
						rv = sub_path_cmd.rv; // ISF_ERR is out of range of HD_ERR
					}
					break;
				}
			}
			seq++;
		}
	}
	return rv;
}
static HD_RESULT _hd_videocap_set_sub_path_state(UINT32 dev_id, UINT32 state)
{
	ISF_FLOW_IOCTL_STATE_ITEM sub_path_cmd = {0};
	HD_PATH_ID sub_path_id = 0;
	HD_DAL sub_self_id = 0;
	HD_IO sub_out_id;
	HD_RESULT rv = HD_OK;
	int r;
	UINT32 i, temp;
	UINT32 ctx_id = _hd_videocap_devid_to_ctxid(dev_id);
	UINT32 map = 0;
	UINT32 seq = 0;

	map = GET_LO_UINT16(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map);

	//printf("\r\nset_sub_path:dev_id=%d, state=%d\r\n",dev_id,state);
	for (i = 0; i < HD_VIDEOCAP_MAX_SHDR_DEVCOUNT; i++) {
		temp  = map >> i;
		if ((temp & 0x1)) {
			if (((1 << i) & _active_list) == 0) {
				DBG_ERR("VCAP[%d] is NOT active(0x%02X) for SHDR(0x%X)\n", (int)i, (unsigned int)_active_list, (unsigned int)map);
				return HD_ERR_NO_CONFIG;
			}
			//only for sub path (seq = 0 is main path)
			if (seq) {
				rv = hd_videocap_get_path_id(HD_VIDEOCAP_IN(i, 0), HD_VIDEOCAP_OUT(i, 0), &sub_path_id);
				if (rv != HD_OK)
					return rv;

				sub_self_id = HD_GET_DEV(sub_path_id);
				sub_out_id = HD_GET_OUT(sub_path_id);
				_HD_CONVERT_SELF_ID(sub_self_id, rv); 	if(rv != HD_OK) {	return rv;}
				_HD_CONVERT_OUT_ID(sub_out_id, rv);	if(rv != HD_OK) {	return rv;}
				sub_path_cmd.src = ISF_PORT(sub_self_id, sub_out_id);
				sub_path_cmd.state = state;
				//printf("\r\nset_sub_path:cmd.src=0x%X\r\n",sub_path_cmd.src);

				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_STATE, &sub_path_cmd);

				if (r == 0) {
					if (sub_path_cmd.rv) {
						rv = HD_ERR_SYS;
						break;
					}
				} else {
					if(((int)sub_path_cmd.rv <= ISF_ERR_BEGIN) && ((int)sub_path_cmd.rv >= ISF_ERR_END)) {
						rv = sub_path_cmd.rv; // ISF_ERR is exactly the same with HD_ERR
					} else {
						DBG_ERR("system fail, rv=%d\r\n", sub_path_cmd.rv);
						rv = sub_path_cmd.rv; // ISF_ERR is out of range of HD_ERR
					}
					break;
				}
			}
			seq++;
		}
	}
	return rv;
}

#if 0
static HD_RESULT _hd_videocap_get_param(HD_DAL self_id, HD_IO out_id, UINT32 param, UINT32 *value)
{
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	HD_RESULT rv = HD_ERR_NG;
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
		rv = HD_ERR_NG;
		switch(cmd.rv) {
		case ISF_OK:
			rv = HD_OK;
			*value = cmd.value;
			break;
		default: break;
		}
	} else {
		rv = HD_ERR_SYS;
	}
	return rv;
}
#endif
HD_RESULT _hd_videocap_convert_dev_id(HD_DAL* p_dev_id)
{
	HD_RESULT rv;
	_HD_CONVERT_SELF_ID(p_dev_id[0], rv);
	return rv;
}

int _hd_videocap_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len)
{
	return snprintf(p_str, str_len, "VIDEOCAP_%d_OUT_%d", dev_id - HD_DEV_BASE, out_id - HD_OUT_BASE);
}


HD_RESULT hd_videocap_init(VOID)
{
	UINT32 i;

	hdal_flow_log_p("hd_videocap_init\n");
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

	if (_hd_vcap_ctx != NULL) {
		return HD_ERR_UNINIT;
	}

	if (_max_dev_count == 0) {
		DBG_ERR("_max_dev_count =0?\r\n");
		return HD_ERR_NO_CONFIG;
	}

	//init lib
	_hd_vcap_csz = sizeof(HD_VCAP_CTX)*_max_dev_count;
	_hd_vcap_ctx = (HD_VCAP_CTX*)malloc_ctx(_hd_vcap_csz);
	if (_hd_vcap_ctx == NULL) {
		DBG_ERR("cannot alloc heap for _max_dev_count =%d\r\n", (int)_max_dev_count);
		return HD_ERR_HEAP;
	}

	if (_hd_common_get_pid() == 0) {

		lock_ctx();
		for(i = 0; i < _max_dev_count; i++) {
			memset(&(_hd_vcap_ctx[i]), 0, sizeof(HD_VCAP_CTX));
		}
		unlock_ctx();

	}

	//init drv
	if (_hd_common_get_pid() == 0) {

		int r;
		ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
		isf_dev_cmd.cmd = 1; //cmd = init
		isf_dev_cmd.p0 = ISF_PORT(DEV_BASE+0, ISF_CTRL); //always call to dev 0
		isf_dev_cmd.p1 = _active_list;
		isf_dev_cmd.p2 = 0;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_CMD, &isf_dev_cmd);
		if(r < 0) {
			DBG_ERR("init fail? %d\r\n", r);
			return r;
		}
	}

	return HD_OK;
}

HD_RESULT hd_videocap_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videocap_bind:\n");
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
	if (_hd_vcap_ctx == NULL) {
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

HD_RESULT hd_videocap_unbind(HD_OUT_ID _out_id)
{
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videocap_unbind:\n");
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
	if (_hd_vcap_ctx == NULL) {
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
HD_RESULT _hd_videocap_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id)
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


HD_RESULT _hd_videocap_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id)
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

HD_RESULT _hd_videocap_get_state(HD_OUT_ID _out_id, UINT32* p_state)
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
HD_RESULT hd_videocap_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID* p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videocap_open:\n");
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
	if (_hd_vcap_ctx == NULL) {
		return HD_ERR_UNINIT;
	}
	if((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		_HD_CONVERT_SELF_ID(self_id, rv);
		if ((1 << (self_id-DEV_BASE) & _active_list) == 0) {
			DBG_ERR("VCAP[%d] is NOT active(0x%X)\n", (int)(self_id-DEV_BASE), (unsigned int)_active_list);
			return HD_ERR_NO_CONFIG;
		}
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
			hdal_flow_log_p("path_id(N/A)\n"); return rv;
		}
		if(!p_path_id) {return HD_ERR_NULL_PTR;}
		p_path_id[0] = _out_id;
		hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);
		//do nothing
		return HD_OK;
	} else {
		UINT32 cur_dev = self_id;

		_HD_CONVERT_SELF_ID(self_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((self_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
			hdal_flow_log_p("path_id(N/A)\n"); return rv;
		}

		_HD_CONVERT_SELF_ID(in_dev_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("dev_id=%lu is larger than max=%lu!\r\n", (UINT32)((in_dev_id) - HD_DEV_BASE), (UINT32)_max_dev_count);
			hdal_flow_log_p("path_id(N/A)\n"); return rv;
		}
		if(in_dev_id != self_id) {
			DBG_ERR("dev_id of in/out is not the same!\r\n");
			hdal_flow_log_p("path_id(N/A)\n"); return HD_ERR_DEV;
		}
		if(!p_path_id) { hdal_flow_log_p("path_id(N/A)\n"); return HD_ERR_NULL_PTR;}
		p_path_id[0] = HD_VIDEOCAP_PATH(cur_dev, in_id, out_id);
		hdal_flow_log_p("path_id(0x%x)\n", p_path_id[0]);
	}

	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
		_HD_CONVERT_OUT_ID(out_id, rv);
		if(rv != HD_OK) {
			DBG_ERR("out_id=%lu is larger than max=%lu!\r\n", (UINT32)((out_id) - HD_OUT_BASE), (UINT32)OUT_COUNT);
			hdal_flow_log_p("path_id(N/A)\n"); return rv;
		}
		cmd.src = ISF_PORT(self_id, out_id);
		cmd.state = 0;
		//printf("\r\nhd_videocap_open:cmd.src=0x%X\r\n",cmd.src);

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
		if (rv != HD_OK) {
			return rv;
		}
		lock_ctx();
		if (_hd_videocap_is_shdr_mode(self_id-DEV_BASE)) {
			rv = _hd_videocap_set_sub_path_state(self_id-DEV_BASE, 0);
		}
		unlock_ctx();
	}
	return rv;
}

HD_RESULT hd_videocap_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	UINT32 ctx_id;

	hdal_flow_log_p("hd_videocap_start:\n");
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
	if (_hd_vcap_ctx == NULL) {
		return HD_ERR_UNINIT;
	}
	_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
	_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

	ctx_id = _hd_videocap_devid_to_ctxid(self_id-DEV_BASE);

	lock_ctx();
	//remind user check pull depth in SHDR mode
	if (_hd_vcap_ctx[ctx_id].out.depth && _hd_videocap_is_shdr_mode(self_id-DEV_BASE)) {
		if (_hd_vcap_ctx[ctx_id].out.depth == 1)
			DBG_WRN("depth should be a multiple of SHDR-n-frame\r\n");
	}
	unlock_ctx();
#if 0 //vendor pdaf setting can't be checked here
	//--- check if DRV_CONFIG info had been set ---
	if (_hd_vcap_ctx[ctx_id].drv_configed == FALSE) {
		DBG_ERR("FATAL ERROR: please set HD_VIDEOCAP_PARAM_DRV_CONFIG first !!!\r\n");
		return HD_ERR_NO_CONFIG;
	} else
#endif
	{
		ISF_FLOW_IOCTL_STATE_ITEM cmd = {0};
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
		if (rv != HD_OK) {
			return rv;
		}
		lock_ctx();
		if (_hd_videocap_is_shdr_mode(self_id-DEV_BASE)) {
			rv = _hd_videocap_set_sub_path_state(self_id-DEV_BASE, 1);
		}
		unlock_ctx();
	}
	return rv;
}

HD_RESULT hd_videocap_stop(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videocap_stop:\n");
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
	if (_hd_vcap_ctx == NULL) {
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
		if (rv != HD_OK) {
			return rv;
		}
		lock_ctx();
		if (_hd_videocap_is_shdr_mode(self_id-DEV_BASE)) {
			rv = _hd_videocap_set_sub_path_state(self_id-DEV_BASE, 2);
		}
		unlock_ctx();
	}
	return rv;
}

HD_RESULT hd_videocap_start_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videocap_stop_list(HD_PATH_ID *path_id, UINT num)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_videocap_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;

	hdal_flow_log_p("hd_videocap_close:\n");
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
	if (_hd_vcap_ctx == NULL) {
		return HD_ERR_UNINIT;
	}
	if(ctrl_id == HD_CTRL) {
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		//do nothing
		return HD_OK;
	}
#if 0
	//reset path setting
	lock_ctx();
	memset(&(_hd_vcap_ctx[self_id-DEV_BASE].in), 0, sizeof(HD_VIDEOCAP_IN));
	memset(&(_hd_vcap_ctx[self_id-DEV_BASE].out), 0, sizeof(HD_VIDEOCAP_OUT));
	memset(&(_hd_vcap_ctx[self_id-DEV_BASE].crop), 0, sizeof(HD_VIDEOCAP_CROP));
	unlock_ctx();
	hd_videocap_set(path_id, HD_VIDEOCAP_PARAM_IN, (VOID *) &(_hd_vcap_ctx[self_id-DEV_BASE].in));
	hd_videocap_set(path_id, HD_VIDEOCAP_PARAM_OUT, (VOID *) &(_hd_vcap_ctx[self_id-DEV_BASE].out));
	hd_videocap_set(path_id, HD_VIDEOCAP_PARAM_IN_CROP, (VOID *) &(_hd_vcap_ctx[self_id-DEV_BASE].crop));
#endif
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
		if (rv != HD_OK) {
			return rv;
		}
		lock_ctx();
		if (_hd_videocap_is_shdr_mode(self_id-DEV_BASE)) {
			rv = _hd_videocap_set_sub_path_state(self_id-DEV_BASE, 3);
		}
		unlock_ctx();
	}
	return rv;
}

static INT hd_videocap_param_cvt_name(HD_VIDEOCAP_PARAM_ID  id, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
		case HD_VIDEOCAP_PARAM_DEVCOUNT:    snprintf(p_ret_string, max_str_len, "DEVCOUNT");  break;
		case HD_VIDEOCAP_PARAM_SYSCAPS:     snprintf(p_ret_string, max_str_len, "SYSCAPS");   break;
		case HD_VIDEOCAP_PARAM_DRV_CONFIG:  snprintf(p_ret_string, max_str_len, "DRV_CONFIG");   break;
		case HD_VIDEOCAP_PARAM_IN:          snprintf(p_ret_string, max_str_len, "IN");        break;
		case HD_VIDEOCAP_PARAM_IN_CROP:
		case HD_VIDEOCAP_PARAM_OUT_CROP:
											snprintf(p_ret_string, max_str_len, "IN_CROP");  break;
		case HD_VIDEOCAP_PARAM_OUT:         snprintf(p_ret_string, max_str_len, "OUT");       break;
		case HD_VIDEOCAP_PARAM_CTRL:        snprintf(p_ret_string, max_str_len, "CTRL");      break;
		case HD_VIDEOCAP_PARAM_SYSINFO:     snprintf(p_ret_string, max_str_len, "SYSINFO");      break;
		case HD_VIDEOCAP_PARAM_FUNC_CONFIG: snprintf(p_ret_string, max_str_len, "FUNC_CONFIG");      break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown param_id(%d)\r\n", id);
			return (-1);
	}
	return 0;
}

HD_RESULT hd_videocap_get(HD_PATH_ID path_id, HD_VIDEOCAP_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	UINT32 ctx_id;
	{
		CHAR  param_name[20];
		hd_videocap_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videocap_get(%s):\n", param_name);
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
	if (_hd_vcap_ctx == NULL) {
		return HD_ERR_UNINIT;
	}
	if (p_param == 0) {
		return HD_ERR_NULL_PTR;
	}

	{
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		rv = HD_OK;

		ctx_id = _hd_videocap_devid_to_ctxid(self_id - DEV_BASE);

		lock_ctx();

		if(ctrl_id == HD_CTRL) {
			switch(id) {
			case HD_VIDEOCAP_PARAM_DEVCOUNT: {
				HD_DEVCOUNT* p_user = (HD_DEVCOUNT*)p_param;
				memset(p_user, 0, sizeof(HD_DEVCOUNT));
				p_user->max_dev_count = _max_dev_count;
				hdal_flow_log_p("max_dev_cnt(%u)\n", p_user->max_dev_count);
			}
			break;
			case HD_VIDEOCAP_PARAM_SYSCAPS:	{
				HD_VIDEOCAP_SYSCAPS *p_user = (HD_VIDEOCAP_SYSCAPS *)p_param;
				VDOCAP_SYSCAPS sys_caps;
				ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};

				memset(p_user, 0, sizeof(HD_VIDEOCAP_SYSCAPS));
				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;
				p_user->chip_id = 0;
				p_user->max_in_count = HD_VIDEOCAP_MAX_IN_COUNT;
				p_user->max_out_count = HD_VIDEOCAP_MAX_OUT_COUNT;
				p_user->dev_caps =
					HD_CAPS_DRVCONFIG
					 | HD_VIDEOCAP_DEVCAPS_AE
					 | HD_VIDEOCAP_DEVCAPS_AWB
					 | HD_VIDEOCAP_DEVCAPS_AF
					 | HD_VIDEOCAP_DEVCAPS_WDR
					 | HD_VIDEOCAP_DEVCAPS_SHDR
					 | HD_VIDEOCAP_DEVCAPS_ETH;
				p_user->in_caps[0] = HD_VIDEOCAP_INCAPS_SEN_MODE;
				p_user->out_caps[0] =
					 HD_VIDEO_CAPS_RAW
					 | HD_VIDEO_CAPS_COMPRESS
					 | HD_VIDEO_CAPS_YUV422
					 | HD_VIDEO_CAPS_CROP_AUTO
					 | HD_VIDEO_CAPS_CROP_WIN
					 | HD_VIDEO_CAPS_SCALE_DOWN
					 | HD_VIDEO_CAPS_DIR_MIRRORX
					 | HD_VIDEO_CAPS_DIR_MIRRORY;
				p_user->max_w_scaleup = 0; //680 no scale up



				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOCAP_PARAM_SYS_CAPS;
				cmd.size = sizeof(VDOCAP_SYSCAPS);
				cmd.value = (UINT32)&sys_caps;

				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
				if (r == 0) {
					switch(cmd.rv) {
					case ISF_OK:
						rv = HD_OK;
						p_user->max_dim.w = sys_caps.max_dim.w;
						p_user->max_dim.h = sys_caps.max_dim.h;
						p_user->max_frame_rate = sys_caps.max_frame_rate;
						p_user->pxlfmt = sys_caps.pxlfmt;
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


				hdal_flow_log_p("dev_id(0x%08x) chip_id(%u) max_in(%u) max_out(%u) dev_caps(0x%08x)\n", p_user->dev_id, p_user->chip_id, p_user->max_in_count,
																									p_user->max_out_count, p_user->dev_caps);
				hdal_flow_log_p("    in_caps(0x%08x) out_caps(0x%08x)\n", p_user->in_caps[0], p_user->out_caps[0]);
				hdal_flow_log_p("    max_dim(%d,%d) max_frame_rate(%d/%d) pxlfmt=0x%X\n", p_user->max_dim.w, p_user->max_dim.h,
																				GET_HI_UINT16(p_user->max_frame_rate),GET_LO_UINT16(p_user->max_frame_rate),
																				p_user->pxlfmt);
			}
			break;
			case HD_VIDEOCAP_PARAM_SYSINFO:	{
				HD_VIDEOCAP_SYSINFO* p_user = (HD_VIDEOCAP_SYSINFO *)p_param;
				VDOCAP_SYSINFO sys_info;

				memset(p_user, 0, sizeof(HD_VIDEOCAP_SYSINFO));
				memset(&sys_info, 0, sizeof(sys_info));

				p_user->dev_id = self_id - HD_DEV_BASE + DEV_BASE;

				ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOCAP_PARAM_SYS_INFO;
				cmd.size = sizeof(VDOCAP_SYSINFO);
				cmd.value = (UINT32)&sys_info;

				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
				if (r == 0) {
					switch(cmd.rv) {
					case ISF_OK:
						rv = HD_OK;
						p_user->cur_fps[0] = sys_info.cur_fps;
						p_user->vd_count = sys_info.vd_count;
						p_user->output_started = sys_info.output_started;
						if (sys_info.output_started) {
							p_user->cur_dim.w = sys_info.cur_dim.w;
							p_user->cur_dim.h = sys_info.cur_dim.h;
						}
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

				hdal_flow_log_p("dev_id(0x%08x) cur_fps[0](0x%08X) vd_count(%llu) cur_dim(%dx%d)\n", p_user->dev_id, p_user->cur_fps[0], p_user->vd_count,(int)p_user->cur_dim.w,(int)p_user->cur_dim.h);
			}
			break;
			case HD_VIDEOCAP_PARAM_DRV_CONFIG: {
				memcpy(p_param, (VOID *)&_hd_vcap_ctx[ctx_id].drv_cfg, sizeof(HD_VIDEOCAP_DRV_CONFIG));
				hdal_flow_log_p("\n");
			}
			break;
			case HD_VIDEOCAP_PARAM_CTRL: {
				memcpy(p_param, (VOID *)&_hd_vcap_ctx[ctx_id].ctl, sizeof(HD_VIDEOCAP_CTRL));
				hdal_flow_log_p("ctl.func(0x%08x)\n", _hd_vcap_ctx[ctx_id].ctl.func);
			}
			break;
			default: rv = HD_ERR_PARAM; break;
			}
		} else {
			rv = HD_OK;
			hdal_flow_log_p("\n");
			switch(id) {
			case HD_VIDEOCAP_PARAM_IN:
				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) { rv = HD_ERR_IO; goto _HD_VC2;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) { goto _HD_VC2;}

				memcpy(p_param, (VOID *)&_hd_vcap_ctx[ctx_id].in, sizeof(HD_VIDEOCAP_IN));
				break;
			case HD_VIDEOCAP_PARAM_OUT:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VC2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VC2;}

				memcpy(p_param, (VOID *)&_hd_vcap_ctx[ctx_id].out, sizeof(HD_VIDEOCAP_OUT));
				break;
			case HD_VIDEOCAP_PARAM_IN_CROP:
			case HD_VIDEOCAP_PARAM_OUT_CROP:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VC2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VC2;}

				memcpy(p_param, (VOID *)&_hd_vcap_ctx[ctx_id].crop, sizeof(HD_VIDEOCAP_CROP));
				break;
			case HD_VIDEOCAP_PARAM_FUNC_CONFIG:{
				HD_VIDEOCAP_FUNC_CONFIG* p_user = (HD_VIDEOCAP_FUNC_CONFIG *)p_param;
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VC2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VC2;}

				memset(p_user, 0, sizeof(HD_VIDEOCAP_FUNC_CONFIG));
				memcpy(p_param, (VOID *)&_hd_vcap_ctx[ctx_id].func_cfg, sizeof(HD_VIDEOCAP_FUNC_CONFIG));
				}
				break;

#if 0
			case HD_VIDEOCAP_PARAM_OUT_DIR:
				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX))  { rv = HD_ERR_IO; goto _HD_VC2;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VC2;}

				memcpy(p_param, (VOID *)&videocap_dir[dev_id], sizeof(HD_VIDEO_DIR));
				break;
#endif
			default: rv = HD_ERR_PARAM; break;
			}
		}

_HD_VC2:
		unlock_ctx();
	}
	return rv;
}

static BOOL hd_videocap_set_if_cfg(VDOCAP_SEN_INIT_IF_CFG *p_if_cfg, HD_VIDEOCAP_SENSOR_DEVICE *p_sen)
{
	BOOL ret = TRUE;

	switch (p_sen->if_type) {
	case HD_COMMON_VIDEO_IN_MIPI_CSI:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_MIPI;
		break;
	case HD_COMMON_VIDEO_IN_LVDS:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_LVDS;
		break;
	case HD_COMMON_VIDEO_IN_SLVS_EC:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_SLVSEC;
		break;
	case HD_COMMON_VIDEO_IN_P_RAW:
	case HD_COMMON_VIDEO_IN_P_AHD:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_PARALLEL;
		break;
	case HD_COMMON_VIDEO_IN_MIPI_VX1:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_MIPI;
		break;
	case HD_COMMON_VIDEO_IN_P_RAW_VX1:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_PARALLEL;
		break;

	default:
		DBG_ERR("not support if_type = %d\r\n", p_sen->if_type);
		ret = FALSE;
		break;
	}

	p_if_cfg->tge.tge_en = p_sen->if_cfg.tge.tge_en;
	p_if_cfg->tge.swap = p_sen->if_cfg.tge.swap;
	p_if_cfg->tge.sie_vd_src = p_sen->if_cfg.tge.vcap_vd_src;
	p_if_cfg->tge.sie_sync_set = p_sen->if_cfg.tge.vcap_sync_set;

	return ret;
}
static BOOL hd_videocap_set_cmd_if_cfg(VDOCAP_SEN_INIT_CMDIF_CFG *p_cmd_if_cfg, HD_VIDEOCAP_SENSOR_DEVICE *p_sen)
{
	BOOL ret = TRUE;

	p_cmd_if_cfg->vx1.en = p_sen->if_cfg.vx1.en;
	p_cmd_if_cfg->vx1.if_sel = p_sen->if_cfg.vx1.if_sel;
	p_cmd_if_cfg->vx1.ctl_sel = p_sen->if_cfg.vx1.ctl_sel;
	p_cmd_if_cfg->vx1.tx_type = p_sen->if_cfg.vx1.tx_type;

	return ret;
}
static BOOL hd_videocap_set_pin_cfg(VDOCAP_SEN_INIT_PIN_CFG *p_pin_cfg, HD_VIDEOCAP_SENSOR_DEVICE *p_sen)
{
	BOOL ret = TRUE;

	p_pin_cfg->pinmux.sensor_pinmux =  p_sen->pin_cfg.pinmux.sensor_pinmux;
	p_pin_cfg->pinmux.cmd_if_pinmux = p_sen->pin_cfg.pinmux.cmd_if_pinmux;
	p_pin_cfg->pinmux.serial_if_pinmux = p_sen->pin_cfg.pinmux.serial_if_pinmux;
	p_pin_cfg->clk_lane_sel = p_sen->pin_cfg.clk_lane_sel;
	if (VDOCAP_SEN_SER_MAX_DATALANE >= HD_VIDEOCAP_SEN_SER_MAX_DATALANE) {
		memcpy(p_pin_cfg->sen_2_serial_pin_map, p_sen->pin_cfg.sen_2_serial_pin_map, sizeof(p_pin_cfg->sen_2_serial_pin_map));
	} else {
		DBG_ERR("sen_2_serial_pin_map len not match %d,%d\r\n", VDOCAP_SEN_SER_MAX_DATALANE, HD_VIDEOCAP_SEN_SER_MAX_DATALANE);
		ret = FALSE;
	}
	p_pin_cfg->ccir_msblsb_switch = p_sen->pin_cfg.ccir_msblsb_switch;
	p_pin_cfg->ccir_vd_hd_pin = p_sen->pin_cfg.ccir_vd_hd_pin;
	p_pin_cfg->vx1_tx241_cko_pin = p_sen->pin_cfg.vx1_tx241_cko_pin;
	p_pin_cfg->vx1_tx241_cfg_2lane_mode = p_sen->pin_cfg.vx1_tx241_cfg_2lane_mode;
	return ret;
}


static void hd_videocap_dump_drv_config(HD_VIDEOCAP_DRV_CONFIG *p_drv_cfg)
{
	hdal_flow_log_p("driver_name(%s) if_type(%d) shdr_map(0x%lx)\n", p_drv_cfg->sen_cfg.sen_dev.driver_name,
															p_drv_cfg->sen_cfg.sen_dev.if_type,
															p_drv_cfg->sen_cfg.shdr_map);

	hdal_flow_log_p("    sensor_pinmux(0x%x) serial_if_pinmux(0x%x) cmd_if_pinmux(0x%x) clk_lane_sel(%d)\n",
																	p_drv_cfg->sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux,
																	p_drv_cfg->sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux,
																	p_drv_cfg->sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux,
																	p_drv_cfg->sen_cfg.sen_dev.pin_cfg.clk_lane_sel);
	hdal_flow_log_p("    sen_2_serial_pin_map[%d %d %d %d %d %d %d %d]\n",
															p_drv_cfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0],
															p_drv_cfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1],
															p_drv_cfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2],
															p_drv_cfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3],
															p_drv_cfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4],
															p_drv_cfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5],
															p_drv_cfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6],
															p_drv_cfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7]);


	hdal_flow_log_p("    ccir_msblsb_switch(%d) ccir_vd_hd_pin(%d)\n", p_drv_cfg->sen_cfg.sen_dev.pin_cfg.ccir_msblsb_switch,
																	p_drv_cfg->sen_cfg.sen_dev.pin_cfg.ccir_vd_hd_pin);

	hdal_flow_log_p("    vx1_tx241_cko_pin(%d) vx1_tx241_cfg_2lane_mode(%d)\n",
														p_drv_cfg->sen_cfg.sen_dev.pin_cfg.vx1_tx241_cko_pin,
														p_drv_cfg->sen_cfg.sen_dev.pin_cfg.vx1_tx241_cfg_2lane_mode);


	hdal_flow_log_p("    vx1_en(%d) if_sel(%d) ctl_sel(%d) tx_type(%d)\n", p_drv_cfg->sen_cfg.sen_dev.if_cfg.vx1.en,
																	p_drv_cfg->sen_cfg.sen_dev.if_cfg.vx1.if_sel,
																	p_drv_cfg->sen_cfg.sen_dev.if_cfg.vx1.ctl_sel,
																	p_drv_cfg->sen_cfg.sen_dev.if_cfg.vx1.tx_type);
	hdal_flow_log_p("    tge_en(%d) swap(%d) vcap_vd_src(%d) vcap_sync_set(0x%X)\n", p_drv_cfg->sen_cfg.sen_dev.if_cfg.tge.tge_en,
																	p_drv_cfg->sen_cfg.sen_dev.if_cfg.tge.swap,
																	p_drv_cfg->sen_cfg.sen_dev.if_cfg.tge.vcap_vd_src,
																	p_drv_cfg->sen_cfg.sen_dev.if_cfg.tge.vcap_sync_set);

	hdal_flow_log_p("    optin: en_mask(0x%x) sen_map_if(%d) if_time_out(%d)\n", p_drv_cfg->sen_cfg.sen_dev.option.en_mask,
																	p_drv_cfg->sen_cfg.sen_dev.option.sen_map_if,
																	p_drv_cfg->sen_cfg.sen_dev.option.if_time_out);
}

/*
			isf_unit_set_param(ISF_CTRL, VDOCAP_PARAM_FLOW_TYPE, VDOCAP_FLOW_SEN_IN);
ISF_UNIT_PARAM_OUT_VDOMAX
	ISF_VDO_MAX
			isf_unit_set_vdomaxsize(ISF_IN(0), VDO_SIZE(1920, 1080));
			isf_unit_set_vdoformat(ISF_IN(0), VDO_PXLFMT_RAW8); //select RAW2YUV
ISF_UNIT_PARAM_OUT_VDOFRAME
	ISF_VDO_FRAME
			isf_unit_set_vdosize(ISF_IN(0), VDO_SIZE(1920, 1080));
			isf_unit_set_vdoformat(ISF_IN(0), VDO_PXLFMT_RAW8); //select RAW2YUV
			isf_unit_set_vdodirect(ISF_IN(0), VDO_DIR_NONE);
ISF_UNIT_PARAM_OUT_VDOFPS
	ISF_VDO_FPS
			isf_unit_set_vdoframectrl(ISF_IN(0), VDO_FRC(30, 1));
ISF_UNIT_PARAM_OUT_VDOWIN
	ISF_VDO_WIN
			isf_unit_set_vdoaspect(ISF_IN(0), VDO_SIZE(0, 0)); //disable dzoom
			isf_unit_set_vdowinpose(ISF_IN(0), VDO_POSE(0, 0));
			isf_unit_set_vdowinsize(ISF_IN(0), VDO_SIZE(0, 0));
*/
HD_RESULT hd_videocap_set(HD_PATH_ID path_id, HD_VIDEOCAP_PARAM_ID id, VOID* p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	UINT32 dev_id;
	UINT32 ctx_id;

	{
		CHAR  param_name[20];
		hd_videocap_param_cvt_name(id, param_name, 20);

		hdal_flow_log_p("hd_videocap_set(%s):\n", param_name);
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
	if (_hd_vcap_ctx == NULL) {
		return HD_ERR_UNINIT;
	}
	if (p_param == 0) {
		return HD_ERR_NULL_PTR;
	}
	{
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		rv = HD_OK;
		cmd.param = id;

		dev_id = self_id - DEV_BASE;
		ctx_id = _hd_videocap_devid_to_ctxid(dev_id);
		#if 0 //CID 131695 (#1 of 1): Logically dead code (DEADCODE)
		if (dev_id >= _max_dev_count) {
			return HD_ERR_DEV;
		}
		#endif

		lock_ctx();

		//printf("hd_videocap_set:ctrl_id=0x%X, param_id=%d\r\n", ctrl_id, id);
		if(ctrl_id == HD_CTRL) {
			switch(id) {
			case HD_VIDEOCAP_PARAM_DRV_CONFIG: {
				HD_VIDEOCAP_DRV_CONFIG *p_user = (HD_VIDEOCAP_DRV_CONFIG *)p_param;
				VDOCAP_SEN_INIT_CFG sen_init;
				UINT32 map;

				hd_videocap_dump_drv_config(p_user);
				memcpy(&_hd_vcap_ctx[ctx_id].drv_cfg, p_param, sizeof(HD_VIDEOCAP_DRV_CONFIG));
				_hd_vcap_ctx[ctx_id].drv_configed = TRUE;

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);

				if (FALSE == _hd_videocap_check_shdr_support(p_user)) {
					rv = HD_ERR_INV;
					goto _HD_VC12;
				}
				map = _hd_videocap_shdr_main_hdl_to_isf(p_user->sen_cfg.shdr_map);
				//shdr mapping
				cmd.param = VDOCAP_PARAM_SHDR_MAP;
				cmd.value = map;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//printf("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);
				if (r != 0) goto _HD_VC1;

				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_map(p_user->sen_cfg.shdr_map);
					//printf("===>1-1:r=%d,rv=%08x\r\n", r, cmd.rv);
					if (r != 0) goto _HD_VC1;
				}
				//pattern gen flow
				if (strncmp(p_user->sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_PAT_GEN, VDOCAP_SEN_NAME_LEN) == 0) {
					cmd.param = VDOCAP_PARAM_FLOW_TYPE;
					cmd.value = VDOCAP_FLOW_PATGEN;
					cmd.size = 0;
					r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
					//printf("===>0:r=%d,rv=%08x\r\n", r, cmd.rv);
					if (_hd_videocap_is_shdr_mode(dev_id)) {
						r = _hd_videocap_set_shdr_sub_ctrl(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map, &cmd);
					}
				} else {
					//SEN_INIT_CFG
					memset((void *)(&sen_init), 0, sizeof(VDOCAP_SEN_INIT_CFG));

					memcpy(sen_init.driver_name, p_user->sen_cfg.sen_dev.driver_name, VDOCAP_SEN_NAME_LEN);
					if (FALSE == hd_videocap_set_if_cfg(&sen_init.sen_init_cfg.if_cfg, &p_user->sen_cfg.sen_dev))
						goto _HD_VC1;
					if (FALSE == hd_videocap_set_cmd_if_cfg(&sen_init.sen_init_cfg.cmd_if_cfg, &p_user->sen_cfg.sen_dev))
						goto _HD_VC1;
					if (FALSE == hd_videocap_set_pin_cfg(&sen_init.sen_init_cfg.pin_cfg, &p_user->sen_cfg.sen_dev))
						goto _HD_VC1;
					memcpy(&sen_init.sen_init_option, &p_user->sen_cfg.sen_dev.option, sizeof(VDOCAP_SEN_INIT_OPTION));

					cmd.param = VDOCAP_PARAM_SEN_INIT_CFG;
					cmd.value = (UINT32)&sen_init;
					cmd.size = sizeof(VDOCAP_SEN_INIT_CFG);
					r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
					//printf("===>2:r=%d,rv=%08x\r\n", r, cmd.rv);
				}
				goto _HD_VC1;
			}
			case HD_VIDEOCAP_PARAM_CTRL: {
				HD_VIDEOCAP_CTRL *p_user = (HD_VIDEOCAP_CTRL *)p_param;
				HD_VIDEOCAP_CTRLFUNC en;

				hdal_flow_log_p("func(0x%x)\n", p_user->func);
				memcpy(&_hd_vcap_ctx[ctx_id].ctl, p_param, sizeof(HD_VIDEOCAP_CTRL));

				en = p_user->func;

				if ((en & (HD_VIDEOCAP_FUNC_WDR|HD_VIDEOCAP_FUNC_SHDR)) && (en & HD_VIDEOCAP_FUNC_AF) && (en & HD_VIDEOCAP_FUNC_ETH)) {
					DBG_ERR("Only two of SHDR(or WDR), AF and ETH could be enabled at the same time!\r\n");
					rv = HD_ERR_INV;
					goto _HD_VC12;
				}

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOCAP_PARAM_ALG_FUNC;
				cmd.value = p_user->func;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//printf("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_ctrl(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map, &cmd);
				}
				goto _HD_VC1;
			}
			default: rv = HD_ERR_PARAM; break;
			}
		} else {
			switch(id) {
			case HD_VIDEOCAP_PARAM_IN: {
				VDOCAP_SEN_MODE_INFO sen_mode_info;
				HD_VIDEOCAP_IN* p_user = (HD_VIDEOCAP_IN*)p_param;

				hdal_flow_log_p("sen_mode(%d) frc(%d/%d) dim(%d,%d) pxlfmt(0x%x) out_frame_num(%d)\n", p_user->sen_mode, GET_HI_UINT16(p_user->frc),GET_LO_UINT16(p_user->frc),
																							p_user->dim.w, p_user->dim.h, p_user->pxlfmt, p_user->out_frame_num);

				memcpy(&_hd_vcap_ctx[ctx_id].in, p_param, sizeof(HD_VIDEOCAP_IN));
				memcpy(&sen_mode_info, p_param, sizeof(sen_mode_info));
				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOCAP_PARAM_SEN_MODE_INFO;
				cmd.value = (UINT32)&sen_mode_info;
				cmd.size = sizeof(sen_mode_info);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//printf("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map, &cmd);
				}
				goto _HD_VC1;
			}
			break;
			case HD_VIDEOCAP_PARAM_OUT:	{
				HD_VIDEOCAP_OUT *p_user = (HD_VIDEOCAP_OUT *)p_param;
				ISF_VDO_FRAME frame = {0};
				ISF_VDO_FPS fps = {0};
				UINT32 depth;
				//HD_VIDEOCAP_SEN_MODESEL_CCIR ccir;

				hdal_flow_log_p("dim(%d,%d) pxlfmt(0x%x) dir(0x%08x) frc(%d/%d) depth(%d)\n", p_user->dim.w, p_user->dim.h,
																						p_user->pxlfmt, p_user->dir,
																						GET_HI_UINT16(p_user->frc),GET_LO_UINT16(p_user->frc),
																						p_user->depth);
				memcpy(&_hd_vcap_ctx[ctx_id].out, p_param, sizeof(HD_VIDEOCAP_OUT));

				cmd.dest = ISF_PORT(self_id, ISF_CTRL);

				/* p_user->ccir	*/
				#if 0 //not ready
				cmd.param = ;
				cmd.value = (UINT32)&sen_init;
				cmd.size = sizeof(VDOCAP_SEN_INIT_CFG);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				#endif

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VC12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VC12;}

				cmd.dest = ISF_PORT(self_id, out_id);
				//only support rotating ccir in SIE
				if (FALSE == _hd_videocap_check_dir_support(p_user->pxlfmt, p_user->dir)) {
					rv = HD_ERR_INV;
					goto _HD_VC12;
				}
				if(FALSE == _hd_videocap_chk_vdo_fmt(p_user->pxlfmt))
                    return HD_ERR_INV;

				frame.direct = p_user->dir;
				frame.imgfmt = p_user->pxlfmt;
				frame.imgsize.w = p_user->dim.w;
				frame.imgsize.h = p_user->dim.h;
				#if defined(_BSP_NA51055_)
				if (frame.imgsize.w || frame.imgsize.h) {
					DBG_WRN("Not support scaling!\r\n");
				}
				#endif
				cmd.param = ISF_UNIT_PARAM_VDOFRAME;
				cmd.value = (UINT32)&frame;
				cmd.size = sizeof(ISF_VDO_FRAME);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map, &cmd);
					if (r != 0) goto _HD_VC1;
				}
				if (p_user->depth > HD_VIDEOCAP_MAX_DEPTH) {
					depth = HD_VIDEOCAP_MAX_DEPTH;
					HD_VIDEOOUT_WRN("The max depth is %d!.\r\n", HD_VIDEOCAP_MAX_DEPTH);
				} else {
					depth = p_user->depth;
				}
				cmd.param = VDOCAP_PARAM_OUT_DEPTH;
				cmd.value = depth;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map, &cmd);
					if (r != 0) goto _HD_VC1;
				}
				if (GET_HI_UINT16(p_user->frc) > GET_LO_UINT16(p_user->frc)) {
					DBG_WRN("Not support sampling up!\r\n");
				}
				fps.framepersecond = p_user->frc;
				cmd.param = ISF_UNIT_PARAM_VDOFPS;
				cmd.value = (UINT32)&fps;
				cmd.size = sizeof(ISF_VDO_FPS);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map, &cmd);
				}
				goto _HD_VC1;
			}
			break;
			case HD_VIDEOCAP_PARAM_IN_CROP:
			case HD_VIDEOCAP_PARAM_OUT_CROP: {
				HD_VIDEOCAP_CROP *p_user = (HD_VIDEOCAP_CROP *)p_param;
				ISF_VDO_WIN win = {0};

				hdal_flow_log_p("mode(%d) win(%d,%d,%d,%d)\n", p_user->mode, p_user->win.rect.x, p_user->win.rect.y, p_user->win.rect.w, p_user->win.rect.h);
				hdal_flow_log_p("    auto_info:ratio_w_h(0x%08x) factor(%d) sft(%d,%d)\n", p_user->auto_info.ratio_w_h, p_user->auto_info.factor, p_user->auto_info.sft.x, p_user->auto_info.sft.y);
				hdal_flow_log_p("    align(%lu,%lu)\n", p_user->align.w, p_user->align.h);
				memcpy(&_hd_vcap_ctx[ctx_id].crop, p_param, sizeof(HD_VIDEOCAP_CROP));

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VC12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VC12;}
				cmd.dest = ISF_PORT(self_id, out_id);

				if(p_user->mode == HD_CROP_AUTO) { ///< cropping size will be calculated by ratio and factor
					win.window.x = p_user->auto_info.sft.x;
					win.window.y = p_user->auto_info.sft.y;
					win.window.w = p_user->auto_info.factor;
					win.window.h = 0;
					win.imgaspect.w = (p_user->auto_info.ratio_w_h >> 16);
					win.imgaspect.h = (p_user->auto_info.ratio_w_h) & 0xffff;
				} else if(p_user->mode == HD_CROP_ON) { ///< cropping size by user
					win.window.x = p_user->win.rect.x;
					win.window.y = p_user->win.rect.y;
					win.window.w = p_user->win.rect.w;
					win.window.h = p_user->win.rect.h;
					win.imgaspect.w = 0;
					win.imgaspect.h = 0;
				} else { //HD_VIDEOCAP_CROP_FULL ///< using sensor full frame
					win.window.x = 0;
					win.window.y = 0;
					win.window.w = 0;
					win.window.h = 0;
					win.imgaspect.w = 0;
					win.imgaspect.h = 0;
				}
				cmd.param = ISF_UNIT_PARAM_VDOWIN;
				cmd.value = (UINT32)&win;
				cmd.size = sizeof(ISF_VDO_WIN);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map, &cmd);
					if (r != 0) goto _HD_VC1;
				}
				if ((p_user->align.w) % 4 || (p_user->align.h % 4)) {
					DBG_ERR("align(%lu,%lu) must be a multiple of 4.\r\n", p_user->align.w, p_user->align.h);
					rv = HD_ERR_INV;
					goto _HD_VC12;
				}
				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOCAP_PARAM_CROP_ALIGN;
				cmd.value = (UINT32)&p_user->align;
				cmd.size = sizeof(VDOCAP_SEN_INIT_CFG);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[ctx_id].drv_cfg.sen_cfg.shdr_map, &cmd);
				}
				goto _HD_VC1;
			}
			break;
			case HD_VIDEOCAP_PARAM_FUNC_CONFIG: {
				HD_VIDEOCAP_FUNC_CONFIG* p_user = (HD_VIDEOCAP_FUNC_CONFIG*)p_param;
				ISF_ATTR t_attr;

				hdal_flow_log_p("ddr_id(%d) out_func(0x%x)\n", p_user->ddr_id, p_user->out_func);

				memcpy(&_hd_vcap_ctx[ctx_id].func_cfg, p_param, sizeof(HD_VIDEOCAP_FUNC_CONFIG));

				if (p_user->ddr_id >= DDR_ID_MAX && p_user->ddr_id != DDR_ID_AUTO) {
					DBG_ERR("ddr_id(%d) should < %d\r\n", (int)p_user->ddr_id, (int)DDR_ID_MAX);
					return HD_ERR_INV;
				}
				cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				if (p_user->out_func & HD_VIDEOCAP_OUTFUNC_DIRECT) {
					if (dev_id) {
						DBG_ERR("OUT DIRECT only support vcap0!\r\n");
						rv = HD_ERR_INV;
						goto _HD_VC12;
					}
					cmd.value = VDOCAP_OUT_DEST_DIRECT;
				} else {
					cmd.value = VDOCAP_OUT_DEST_DRAM;
				}
				cmd.param = VDOCAP_PARAM_OUT_DEST;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//printf("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[dev_id].drv_cfg.sen_cfg.shdr_map, &cmd);
				}

				if (p_user->out_func & HD_VIDEOCAP_OUTFUNC_ONEBUF) {
					cmd.value = TRUE;
				} else {
					cmd.value = FALSE;
				}
				cmd.param = VDOCAP_PARAM_ONE_BUFF;
				cmd.size = 0;
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//printf("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[dev_id].drv_cfg.sen_cfg.shdr_map, &cmd);
				}

				if(!(out_id >= HD_OUT_BASE && out_id <= HD_OUT_MAX)) { rv = HD_ERR_IO; goto _HD_VC12;}
				_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) { goto _HD_VC12;}
				t_attr.attr = p_user->ddr_id;
				cmd.dest = ISF_PORT(self_id, out_id);
				cmd.param = ISF_UNIT_PARAM_ATTR;
				cmd.value = (UINT32)&t_attr;
				cmd.size = sizeof(ISF_ATTR);
				r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				if (r != 0) goto _HD_VC1;
				if (_hd_videocap_is_shdr_mode(dev_id)) {
					r = _hd_videocap_set_shdr_sub_path(_hd_vcap_ctx[dev_id].drv_cfg.sen_cfg.shdr_map, &cmd);
				}
				goto _HD_VC1;
			}
			break;
			default: rv = HD_ERR_PARAM; break;
			}
		}
		if(rv != HD_OK) {
			goto _HD_VC12;
		}
	}
_HD_VC12:
	unlock_ctx();
	return rv;
_HD_VC1:
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

#define TIMESTAMP_TO_MS(ts)  ((ts >> 32) * 1000 + (ts & 0xFFFFFFFF) / 1000)

static void _hd_videocap_wrap_frame(HD_VIDEO_FRAME* p_video_frame, ISF_DATA *p_data)
{
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;

	p_video_frame->blk = p_data->h_data; //isf-data-handle

	p_video_frame->sign = p_vdoframe->sign;
	p_video_frame->p_next = NULL;
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

	p_video_frame->pw[HD_VIDEO_PINDEX_V] = p_vdoframe->pw[HD_VIDEO_PINDEX_V];
	p_video_frame->ph[HD_VIDEO_PINDEX_V] = p_vdoframe->ph[HD_VIDEO_PINDEX_V];
	p_video_frame->loff[HD_VIDEO_PINDEX_V] = p_vdoframe->loff[HD_VIDEO_PINDEX_V];
	p_video_frame->phy_addr[HD_VIDEO_PINDEX_V] = p_vdoframe->phyaddr[HD_VIDEO_PINDEX_V];

	p_video_frame->pw[HD_VIDEO_PINDEX_A] = p_vdoframe->pw[HD_VIDEO_PINDEX_A];
	p_video_frame->ph[HD_VIDEO_PINDEX_A] = p_vdoframe->ph[HD_VIDEO_PINDEX_A];
	p_video_frame->loff[HD_VIDEO_PINDEX_A] = p_vdoframe->loff[HD_VIDEO_PINDEX_A];
	p_video_frame->phy_addr[HD_VIDEO_PINDEX_A] = p_vdoframe->phyaddr[HD_VIDEO_PINDEX_A];

	p_video_frame->poc_info = p_vdoframe->reserved[0];
	p_video_frame->reserved[0] = p_vdoframe->reserved[1];
	p_video_frame->reserved[1] = p_vdoframe->reserved[2];
	p_video_frame->reserved[2] = p_vdoframe->reserved[3];
	p_video_frame->reserved[3] = p_vdoframe->reserved[4];
	p_video_frame->reserved[4] = p_vdoframe->reserved[5];
	p_video_frame->reserved[5] = p_vdoframe->reserved[6];
	p_video_frame->reserved[6] = p_vdoframe->reserved[7];
}
HD_RESULT hd_videocap_pull_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if ((_hd_common_get_pid() == 0) && (_hd_vcap_ctx == NULL)) {
		return HD_ERR_UNINIT;
	}
	if (p_video_frame == 0) {
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};
		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}
/*
		INT32 dev_id = self_id - DEV_BASE;
		UINT32 ctx_id = _hd_videocap_devid_to_ctxid(dev_id);
		lock_ctx();
		if (_hd_vcap_ctx[ctx_id].out.depth == 0) {
			DBG_ERR("HD_VIDEOCAP_OUT.depth should set larger than 0 to allow pull_out\r\n");
			unlock_ctx();
			return HD_ERR_SYS;
		}
		unlock_ctx();
*/
		cmd.dest = ISF_PORT(self_id, out_id);
		cmd.p_data = &data;
		cmd.async = wait_ms;
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_PULL_DATA, &cmd);
		if (r == 0) {
			switch(cmd.rv) {
			case ISF_OK:
				_hd_videocap_wrap_frame(p_video_frame, &data);
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
void _hd_videocap_rebuild_frame(ISF_DATA *p_data, HD_VIDEO_FRAME* p_video_frame)
{
	VDO_FRAME * p_vdoframe = (VDO_FRAME *) p_data->desc;

	p_data->sign = MAKEFOURCC('I','S','F','D');
	p_data->h_data = p_video_frame->blk; //isf-data-handle

	p_vdoframe->sign       = p_video_frame->sign;
}
HD_RESULT hd_videocap_release_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME* p_video_frame)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_RESULT rv = HD_ERR_NG;
	int r;
	if (dev_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	if ((_hd_common_get_pid() == 0) && (_hd_vcap_ctx == NULL)) {
		return HD_ERR_UNINIT;
	}
	if (p_video_frame == 0) {
		return HD_ERR_NULL_PTR;
	}

	{
		ISF_FLOW_IOCTL_DATA_ITEM cmd = {0};
		ISF_DATA data = {0};

		_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
		_HD_CONVERT_OUT_ID(out_id, rv);	if(rv != HD_OK) {	return rv;}

		_hd_videocap_rebuild_frame(&data, p_video_frame);

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


HD_RESULT hd_videocap_uninit(VOID)
{
	hdal_flow_log_p("hd_videocap_uninit\n");
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

	if (_hd_vcap_ctx == NULL) {
		return HD_ERR_INIT;
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
		free_ctx(_hd_vcap_ctx);
		_hd_vcap_ctx = NULL;
	}
	return HD_OK;
}

int _hd_videocap_is_init(VOID)
{
	return (_hd_vcap_ctx != NULL);
}
