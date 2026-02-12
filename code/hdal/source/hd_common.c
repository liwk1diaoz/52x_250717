/**
	@brief Common source code.\n
	This file contains the functions which is related to common part in the chip.

	@file hd_common.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#if defined(__LINUX)
#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#endif
#if defined(__FREERTOS)
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#endif

#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME HD_COMMON
#include "hd_int.h"
#include "hd_version.h"
#include "hd_logger_p.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/nvtmpp_ioctl.h"
#include "comm/log.h"
#if defined(__LINUX)
#include <sys/mman.h>
#endif

/***************************/
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define MAX_KER_MBLK_NU	32

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct {
	uintptr_t	start_addr;
	uintptr_t   end_addr;
} memory_range_info_t;
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
HD_RESULT bd_dal_init(void);
HD_RESULT bd_dal_uninit(void);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
int dev_fd = -1;
int nvtmpp_hdl = -1;
static UINT32 g_cfg = 0;
static UINT32 g_cfg1 = 0;
static UINT32 g_cfg2 = 0;
static UINT32 b_get_cfg = 0;
static int g_log = 1;
pthread_t  dump_thread_id = 0;


#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define NVTMPP_OPEN(...) 0
#define NVTMPP_IOCTL nvtmpp_ioctl
#define NVTMPP_CLOSE(...)
#define LOG_OPEN(...) 0
#define LOG_IOCTL log_ioctl
#define LOG_CLOSE(...)
#endif
#if defined(__LINUX)
#define NVTMPP_OPEN  open
#define NVTMPP_IOCTL ioctl
#define NVTMPP_CLOSE close
#define LOG_OPEN     open
#define LOG_IOCTL    ioctl
#define LOG_CLOSE    close
#endif


#if 1 //defined(__LINUX)

/*
HD_COMMON_MEM_VIRT_INFO dump_buf_info = {0};
void *dump_buf_va;
UINT32 dump_buf_pa;
*/
/*
static int _hd_dump_open(void)
{
	ret = hd_common_mem_alloc("hd_dump", &dump_buf_pa, (void **)&dump_buf_va, 8192, 0);
	va1 = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, dump_buf_pa, size);
	if (va1 == 0) {
		func_ret = HD_ERR_SYS;
		goto map_err;
	}
}

static int _hd_dump_close(void)
{
	hd_common_mem_munmap((void*)va2, size);
	ret = hd_common_mem_free(pa1, (void *)va1);
}
*/
int _hd_dump_printf(const char *fmtstr, ...)
{
	ISF_FLOW_IOCTL_OUT_LOG cmd = {0};
	int len;
	int r;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(cmd.str, 1023, fmtstr, marker);
	va_end(marker);

	cmd.str[1023] = 0; //force truncate for safe
	cmd.len = len;
	r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_OUT_STR, &cmd);
	return r;
}

static ISF_FLOW_IOCTL_OUT_LOG cmd = {0};

static void *_hd_dump_thread(void *arg)
{
	int r;

	while(g_log) {
		// wait log cmd
		r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_OUT_WAI, &cmd);
		if(r < 0) {
			DBG_ERR("wait cat info begin fail?\r\n");
			break; //quit now!
		}
		switch (cmd.uid) {
		case 1:
			_hd_dump_printf("HDAL_VERSION: %08X:%08X\r\n", HDAL_VERSION, HDAL_IMPL_VERSION);
			_hd_dump_printf("\r\n");
			break;
		case ISF_UNIT_VDOCAP:
		case ISF_UNIT_VDOCAP+1:
		case ISF_UNIT_VDOCAP+2:
		case ISF_UNIT_VDOCAP+3:
		case ISF_UNIT_VDOCAP+4:
		case ISF_UNIT_VDOCAP+5:
		case ISF_UNIT_VDOCAP+6:
		case ISF_UNIT_VDOCAP+7:
			_hd_videocap_dump_info(cmd.uid - ISF_UNIT_VDOCAP);
			break;
		case ISF_UNIT_VDOPRC:
		case ISF_UNIT_VDOPRC+1:
		case ISF_UNIT_VDOPRC+2:
		case ISF_UNIT_VDOPRC+3:
		case ISF_UNIT_VDOPRC+4:
		case ISF_UNIT_VDOPRC+5:
		case ISF_UNIT_VDOPRC+6:
		case ISF_UNIT_VDOPRC+7:
		case ISF_UNIT_VDOPRC+8:
		case ISF_UNIT_VDOPRC+9:
		case ISF_UNIT_VDOPRC+10:
		case ISF_UNIT_VDOPRC+11:
		case ISF_UNIT_VDOPRC+12:
		case ISF_UNIT_VDOPRC+13:
		case ISF_UNIT_VDOPRC+14:
		case ISF_UNIT_VDOPRC+15:
			_hd_videoproc_dump_info(cmd.uid - ISF_UNIT_VDOPRC);
			break;
		case ISF_UNIT_VDOENC:
			_hd_videoenc_dump_info();
			break;
		case ISF_UNIT_VDODEC:
			_hd_videodec_dump_info();
			break;
		case ISF_UNIT_AUDCAP:
			_hd_audiocap_dump_info(cmd.uid - ISF_UNIT_AUDCAP);
			break;
		case ISF_UNIT_AUDOUT:
		case ISF_UNIT_AUDOUT+1:
			_hd_audioout_dump_info(cmd.uid - ISF_UNIT_AUDOUT);
			break;
		case ISF_UNIT_AUDENC:
			_hd_audioenc_dump_info();
			break;
		case ISF_UNIT_AUDDEC:
			_hd_audiodec_dump_info();
			break;
		case ISF_UNIT_VDOOUT:
		case ISF_UNIT_VDOOUT+1:
			_hd_videoout_dump_info(cmd.uid - ISF_UNIT_VDOOUT);

			break;
		case 0xffffffff:
			g_log = 0; //quit cat info!
			break;
		default:
			break;
		}

		if(g_log) {
			// signal log cmd
			r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_OUT_SIG, &cmd);
			if(r < 0) {
				DBG_ERR("sig cat info end fail?\r\n");
				break; //quit now!
			}
			cmd.uid = 0;
		}
	}
	return 0;
}
#else
int _hd_dump_printf(const char *fmtstr, ...)
{
    return 0;
}
#endif

#if defined(__LINUX)
/*parse sys/kernel/debug/memblock/memory memory range*/
HD_RESULT _hd_get_kernel_memory(memory_range_info_t *kernel_memory_info, unsigned int *nr_of_kernel_memory_info)
{
	unsigned int i = 0;
	char line[256];
	FILE *fp;
	uintptr_t start_addr, end_addr;

	if (!kernel_memory_info || *nr_of_kernel_memory_info == 0) {
	    return HD_ERR_NULL_PTR;
	}
	fp = fopen("/sys/kernel/debug/memblock/memory", "r");
	if (!fp) {
		DBG_ERR("open /sys/kernel/debug/memblock/memory fail,errno(%d) strerror(%s)\n", errno, strerror(errno));
		return HD_ERR_FAIL;
	}
	while (fgets(line, sizeof(line), fp) != 0) {
		if (strstr(line, ":")) {
			char *token, *strptr;

			token = strstr(line, ":");
			token++;
			start_addr = strtoull(token, &strptr, 16);
			token = strstr(strptr, "..");
			token+= 2;
			end_addr = strtoull(token, NULL, 16);
			kernel_memory_info[i].start_addr = start_addr;
			kernel_memory_info[i].end_addr = end_addr;
			//DBG_DUMP("km[%d] = 0x%x ~ 0x%x\r\n", i, start_addr, end_addr);
			i++;
		    if (i >= *nr_of_kernel_memory_info) {
				HD_COMMON_ERR("memblock nr=%d >= nr_of_kernel_memory_info=%d\n" ,i, *nr_of_kernel_memory_info);
				fclose(fp);
				return HD_ERR_RESOURCE;
		    }
		}
	}
	fclose(fp);
	*nr_of_kernel_memory_info = i;
	return HD_OK;
}

HD_RESULT check_hdal_memory_with_kernel_memory(memory_range_info_t *km_info, unsigned int nr_of_memory_info)
{
	unsigned int                     km_i, j;
	NVTMPP_IOC_GET_HDAL_BASE_S       get_hdal_s = {0};
	int                              ret;
	uintptr_t                        hdal_s_addr, hdal_e_addr;
	unsigned long                    hdal_size;

	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_HDAL_BASE, &get_hdal_s);
    if (ret < 0) {
		HD_COMMON_ERR("get hdal base\r\n");
		return HD_OK;
    }
	for (km_i = 0; km_i < nr_of_memory_info; km_i++) {
		for (j =0; j < NVTMPP_DDR_MAX_NUM; j++) {
			hdal_s_addr = get_hdal_s.base[j];
			hdal_e_addr = get_hdal_s.base[j] + get_hdal_s.size[j];
			hdal_size = get_hdal_s.size[j];
			if (!hdal_size) {
				continue;
			}
			if (hdal_s_addr >= km_info[km_i].start_addr && hdal_s_addr <= km_info[km_i].end_addr) {
				goto check_fail;
			}
			if (hdal_e_addr >= km_info[km_i].start_addr && hdal_e_addr <= km_info[km_i].end_addr) {
				goto check_fail;
			}
			if (km_info[km_i].start_addr >= hdal_s_addr && km_info[km_i].start_addr <= hdal_e_addr) {
				goto check_fail;
			}
			if (km_info[km_i].end_addr >= hdal_s_addr && km_info[km_i].end_addr <= hdal_e_addr) {
				goto check_fail;
			}
		}
    }
	return HD_OK;

check_fail:
	HD_COMMON_ERR("hdal overlap with linux memory\r\n");
	DBG_DUMP("linux mem:\r\n");
	system("cat /sys/kernel/debug/memblock/memory");
	DBG_DUMP("hdal mem:\r\n");
	for (j = 0; j < NVTMPP_DDR_MAX_NUM; j++) {
		if (get_hdal_s.size[j]) {
			DBG_DUMP("%d: 0x%lx ~ 0x%lx\r\n", j, (unsigned long)get_hdal_s.base[j], (unsigned long)(get_hdal_s.base[j] + get_hdal_s.size[j]));
		}
	}
	return HD_ERR_NOT_ALLOW;
}
#endif

HD_RESULT hd_common_init(UINT32 sys_config_type)
{
#if 1 //defined(__LINUX)
	int ret;
#endif
	int r;
	int log_fd = -1;
	unsigned int hdal_version = HDAL_VERSION;
	unsigned int impl_version = HDAL_IMPL_VERSION;
	ISF_FLOW_IOCTL_GET_MAX_PATH   isf_max_path = {0};
	ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};
	unsigned int cfg_cpu;

	if (dev_fd >= 0) {
		DBG_ERR("hd_common is not uninit?\r\n");
		return HD_ERR_INIT;
	}

	cfg_cpu = (sys_config_type & HD_COMMON_CFG_CPU);

	// open internal logger system
	hd_logger_init_p(cfg_cpu);

	// open common module fd
	if (cfg_cpu == 0)
		dev_fd = ISF_OPEN("/dev/isf_flow0",O_RDWR|O_SYNC);
	else if (cfg_cpu == 1)
		dev_fd = ISF_OPEN("/dev/isf_flow1",O_RDWR|O_SYNC);
	else if (cfg_cpu == 2)
		dev_fd = ISF_OPEN("/dev/isf_flow2",O_RDWR|O_SYNC);
	else if (cfg_cpu == 3)
		dev_fd = ISF_OPEN("/dev/isf_flow3",O_RDWR|O_SYNC);
	else if (cfg_cpu == 4)
		dev_fd = ISF_OPEN("/dev/isf_flow4",O_RDWR|O_SYNC);
	else if (cfg_cpu == 5)
		dev_fd = ISF_OPEN("/dev/isf_flow5",O_RDWR|O_SYNC);
	else if (cfg_cpu == 6)
		dev_fd = ISF_OPEN("/dev/isf_flow6",O_RDWR|O_SYNC);
	else if (cfg_cpu == 7)
		dev_fd = ISF_OPEN("/dev/isf_flow7",O_RDWR|O_SYNC);

	if (dev_fd < 0) {
		DBG_ERR("open hd fail?\r\n");
		return HD_ERR_NG;
	}

	//here shows version to avoid message racing with kernel
	DBG_DUMP("HDAL: Version: v%x.%x.%x\r\n",
		(HDAL_VERSION&0xF00000) >> 20,
		(HDAL_VERSION&0x0FFFF0) >> 4,
		(HDAL_VERSION&0x00000F));

	if ((log_fd = LOG_OPEN("/dev/log_vg", O_RDWR | O_SYNC)) < 0) {
		DBG_ERR("open /dev/log_vg failed.\n");
		goto _INIT_ERR1;
	}

	if (LOG_IOCTL(log_fd, IOCTL_SET_HDAL_VERSION, &hdal_version) < 0) {
		DBG_ERR("Set HDAL Version failed.\n");
		LOG_CLOSE(log_fd);
		goto _INIT_ERR2;
	}

	if (LOG_IOCTL(log_fd, IOCTL_SET_IMPL_VERSION, &impl_version) < 0) {
		DBG_ERR("Set HDAL Version failed.\n");
		LOG_CLOSE(log_fd);
		goto _INIT_ERR2;
	}

	LOG_CLOSE(log_fd);

	hdal_flow_log_p("hd_common_init\r\n");
	hdal_flow_log_p("    cfg(0x%08x)\r\n",
		sys_config_type);

	//config
	g_cfg = sys_config_type;
	g_cfg1 = 0;
	g_cfg2 = 0;
	b_get_cfg = 0;

	//common init (and reset)
	isf_dev_cmd.p0 = 0;
	isf_dev_cmd.p1 = 0;
	isf_dev_cmd.p2 = g_cfg;
	r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_INIT, &isf_dev_cmd);
	if(r < 0) {
		DBG_ERR("init fail? %d\r\n", r);
		goto _INIT_ERR1;
	}
	if ((g_cfg & HD_COMMON_CFG_CPU) == 0 && _hd_common_get_pid() <= 0) {
		_hd_osg_reset();
	}

	//load system config
	r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_GET_MAX_PATH, &isf_max_path);
	if (r < 0) {
		DBG_ERR("get max path fail! %d\r\n", r);
		goto _INIT_ERR1;
	}
	_hd_videocap_cfg_active(isf_max_path.vdocap_active_list);
	_hd_videoproc_cfg_max(isf_max_path.vdoprc_maxdevice);
	_hd_videoenc_cfg_max(isf_max_path.vdoenc_maxpath);
	_hd_videodec_cfg_max(isf_max_path.vdodec_maxpath);

	if(_hd_common_get_pid() <= 0)
		_hd_osg_cfg_max(isf_max_path.vdoprc_maxdevice,
			isf_max_path.vdoenc_maxpath,
			isf_max_path.vdoout_maxdevice,
			isf_max_path.stamp_maximg,
			isf_max_path.vdoprc_maxstamp,
			isf_max_path.vdoprc_maxmask,
			isf_max_path.vdoenc_maxstamp,
			isf_max_path.vdoenc_maxmask,
			isf_max_path.vdoout_maxstamp,
			isf_max_path.vdoout_maxmask);

	_hd_videoout_cfg_max(isf_max_path.vdoout_maxdevice);
	_hd_audiocap_cfg_max(isf_max_path.adocap_maxdevice);
	_hd_audioout_cfg_max(isf_max_path.adoout_maxdevice);
	_hd_audioenc_cfg_max(isf_max_path.adoenc_maxpath);
	_hd_audiodec_cfg_max(isf_max_path.adodec_maxpath);

	if ((g_cfg & HD_COMMON_CFG_CPU) == 0) {
		g_log = 1; //start cat info
		// create encode_thread (pull_out bitstream)
		//ret = pthread_create(&dump_thread_id, NULL, _hd_dump_thread, (void *)stream);
		ret = pthread_create(&dump_thread_id, NULL, _hd_dump_thread, (void *)NULL);
		if (ret < 0) {
			DBG_ERR("create dump thread fail? %d\r\n", r);
			goto _INIT_ERR1;
		}
		//_hd_common_reg_cb();

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#else
        {
            char thread_name[20] = {0};
            sprintf(thread_name, "_hd_dump_thread");
            ret = pthread_setname_np(dump_thread_id, thread_name);
        }

        if (ret < 0) {
            //DBG_ERR("name thread fail? %d\r\n", ret);
            return HD_OK;
        }
#endif

	}

	return HD_OK;

_INIT_ERR1:
	// close internal logger system
	hd_logger_uninit_p();

_INIT_ERR2:
	if (ISF_CLOSE(dev_fd) != 0) {
		return HD_ERR_NG;
	}

	dev_fd = -1;
	return HD_ERR_NG;
}

HD_RESULT hd_common_uninit(void)
{
	int r;
	ISF_FLOW_IOCTL_CMD isf_dev_cmd = {0};

	hdal_flow_log_p("hd_common_uninit\r\n");
	// close common module fd
	if (dev_fd < 0) {
		return HD_ERR_UNINIT;
	}
    if(nvtmpp_hdl != -1) {
		DBG_WRN("mem is not uninit?\r\n");
        	return HD_ERR_UNINIT;
    }
	r = ISF_IOCTL(dev_fd, ISF_FLOW_CMD_UNINIT, &isf_dev_cmd);
	if(r < 0) {
		DBG_ERR("common uninit fail? %d\r\n", r);
		return HD_ERR_NG;
	}

	if (ISF_CLOSE(dev_fd) != 0) {
		return HD_ERR_NG;
	}

	if ((g_cfg & HD_COMMON_CFG_CPU) == 0) {
		// destroy thread
		pthread_join(dump_thread_id, NULL);
	}

	// close internal logger system
	hd_logger_uninit_p();

	dev_fd = -1;
	g_cfg1 = 0;
	g_cfg2 = 0;
	g_cfg = 0;
	return HD_OK;
}

HD_RESULT hd_common_sysconfig(UINT32 sys_config_type1, UINT32 sys_config_type2, UINT32 type1_mask, UINT32 type2_mask)
{
	UINT32 m1, m2;

	hdal_flow_log_p("hd_common_sysconfig:\r\n");
	if (dev_fd < 0) {
		DBG_ERR("Please init hd_common firstly\r\n");
		return HD_ERR_UNINIT;
	}

	hdal_flow_log_p("    cfg1(0x%08x) cfg2(0x%08x) cfg1_mask(0x%08x) cfg2_mask(0x%08x)\r\n",
		(unsigned int)sys_config_type1, (unsigned int)sys_config_type2, (unsigned int)type1_mask, (unsigned int)type2_mask);

	if (b_get_cfg == 1) {
		DBG_ERR("Too late, config is already in used, cannot be changed!\r\n");
		return HD_ERR_INIT;
	}

	m1 = (sys_config_type1 & type1_mask);
	m2 = (sys_config_type2 & type2_mask);
	if (m1 & HD_VIDEOCAP_CFG) {
		g_cfg1 &= ~(m1 & HD_VIDEOCAP_CFG);
		g_cfg1 |= (m1 & HD_VIDEOCAP_CFG);
	}
	if (m1 & HD_VIDEOOUT_CFG) {
		g_cfg1 &= ~(m1 & HD_VIDEOOUT_CFG);
		g_cfg1 |= (m1 & HD_VIDEOOUT_CFG);
	}
	if (m1 & HD_VIDEOENC_CFG) {
		g_cfg1 &= ~(m1 & HD_VIDEOENC_CFG);
		g_cfg1 |= (m1 & HD_VIDEOENC_CFG);
	}
	if (m1 & HD_VIDEODEC_CFG) {
		g_cfg1 &= ~(m1 & HD_VIDEODEC_CFG);
		g_cfg1 |= (m1 & HD_VIDEODEC_CFG);
	}
	if (m1 & HD_VIDEOPROC_CFG) {
		g_cfg1 &= ~(m1 & HD_VIDEOPROC_CFG);
		g_cfg1 |= (m1 & HD_VIDEOPROC_CFG);
	}
	if (m1 & HD_GFX_CFG) {
		g_cfg1 &= ~(m1 & HD_GFX_CFG);
		g_cfg1 |= (m1 & HD_GFX_CFG);
	}
	if (m1 & HD_OSG_CFG) {
		g_cfg1 &= ~(m1 & HD_OSG_CFG);
		g_cfg1 |= (m1 & HD_OSG_CFG);
	}
	if (m1 & VENDOR_R1_CFG) {
		g_cfg1 &= ~(m1 & VENDOR_R1_CFG);
		g_cfg1 |= (m1 & VENDOR_R1_CFG);
	}
	////////////////////////////////////////////
	if (m2 & HD_AUDIOCAP_CFG) {
		g_cfg2 &= ~(m2 & HD_AUDIOCAP_CFG);
		g_cfg2 |= (m2 & HD_AUDIOCAP_CFG);
	}
	if (m2 & HD_AUDIOOUT_CFG) {
		g_cfg2 &= ~(m2 & HD_AUDIOOUT_CFG);
		g_cfg2 |= (m2 & HD_AUDIOOUT_CFG);
	}
	if (m2 & HD_AUDIOENC_CFG) {
		g_cfg2 &= ~(m2 & HD_AUDIOENC_CFG);
		g_cfg2 |= (m2 & HD_AUDIOENC_CFG);
	}
	if (m2 & HD_AUDIODEC_CFG) {
		g_cfg2 &= ~(m2 & HD_AUDIODEC_CFG);
		g_cfg2 |= (m2 & HD_AUDIODEC_CFG);
	}
	if (m2 & VENDOR_AI_CFG) {
		g_cfg2 &= ~(m2 & VENDOR_AI_CFG);
		g_cfg2 |= (m2 & VENDOR_AI_CFG);
	}
	if (m2 & VENDOR_CV_CFG) {
		g_cfg2 &= ~(m2 & VENDOR_CV_CFG);
		g_cfg2 |= (m2 & VENDOR_CV_CFG);
	}
	if (m2 & VENDOR_DSP_CFG) {
		g_cfg2 &= ~(m2 & VENDOR_DSP_CFG);
		g_cfg2 |= (m2 & VENDOR_DSP_CFG);
	}
	if (m2 & VENDOR_ISP_CFG) {
		g_cfg2 &= ~(m2 & VENDOR_ISP_CFG);
		g_cfg2 |= (m2 & VENDOR_ISP_CFG);
	}
	return HD_OK;
}

int _hd_common_get_pid(void)
{
	//hdal_flow_log_p("_hd_common_get_init:\r\n");
	if (dev_fd < 0) {
		DBG_ERR("Please init hd_common firstly\r\n");
		return HD_ERR_UNINIT;
	}

	return (int)(g_cfg & HD_COMMON_CFG_CPU);
}

int _hd_common_is_new_flow(void)
{
	//hdal_flow_log_p("_hd_common_get_init:\r\n");
	if (dev_fd < 0) {
		DBG_ERR("Please init hd_common firstly\r\n");
		return HD_ERR_UNINIT;
	}

	return (int)(g_cfg & HD_COMMON_CFG_R4);
}

HD_RESULT _hd_common_get_init(UINT32 *p_sys_config_type)
{
	//hdal_flow_log_p("_hd_common_get_init:\r\n");
	if (dev_fd < 0) {
		DBG_ERR("Please init hd_common firstly\r\n");
		return HD_ERR_UNINIT;
	}

	if (p_sys_config_type != 0) p_sys_config_type[0] = g_cfg;
	return HD_OK;
}

HD_RESULT hd_common_get_sysconfig(UINT32 *p_sys_config_type1, UINT32 *p_sys_config_type2)
{
	hdal_flow_log_p("hd_common_get_sysconfig:\r\n");
	if (dev_fd < 0) {
		DBG_ERR("Please init hd_common firstly\r\n");
		return HD_ERR_UNINIT;
	}

	b_get_cfg = 1;

	if (p_sys_config_type1 != 0) p_sys_config_type1[0] = g_cfg1;
	if (p_sys_config_type2 != 0) p_sys_config_type2[0] = g_cfg2;
	return HD_OK;
}

HD_RESULT hd_common_mem_init(HD_COMMON_MEM_INIT_CONFIG *p_mem_config)
{
	NVTMPP_IOC_VB_CONF_S  vb_conf;
	NVTMPP_IOC_VB_INIT_S  init_s;
	UINT32                i, pool_cnt = 0;
	int                   ret;
	#if defined(__LINUX)
	memory_range_info_t kernel_memory[MAX_KER_MBLK_NU];
	unsigned int nr_of_array = MAX_KER_MBLK_NU;
	#endif

	hdal_flow_log_p("hd_common_mem_init:\r\n");
	/* p_mem_config is NULL is to support second process init hd_common_mem */
	if (p_mem_config != NULL && dev_fd < 0) {
		HD_COMMON_ERR("Please init hd_common firstly\r\n");
		return HD_ERR_INIT;
	}
	if (nvtmpp_hdl >= 0) {
		return HD_ERR_INIT;
	}
	nvtmpp_hdl = NVTMPP_OPEN("/dev/nvtmpp", O_RDWR|O_SYNC);
	if (nvtmpp_hdl < 0) {
		HD_COMMON_ERR("open /dev/nvtmpp error\r\n");
		return HD_ERR_SYS;
	}
	#if defined(__LINUX)
	if (_hd_get_kernel_memory(kernel_memory, &nr_of_array) != HD_OK) {
		HD_COMMON_ERR("Error:_hd_get_kernel_memory fail\r\n");
		return HD_ERR_SYS;
	}
	if (check_hdal_memory_with_kernel_memory(kernel_memory, nr_of_array) != HD_OK) {
		return HD_ERR_SYS;
	}
	#endif
	if (p_mem_config == NULL) {
		return HD_OK;
	}
	memset(&vb_conf, 0x00, sizeof(vb_conf));
	vb_conf.max_pool_cnt = NVTMPP_VB_DEF_POOLS_CNT;
	pool_cnt = 0;
	for (i = 0; i < HD_COMMON_MEM_MAX_POOL_NUM; i++) {
		if (pool_cnt >= NVTMPP_VB_MAX_COMM_POOLS) {
			HD_COMMON_ERR("exceeds vb pool limit %d\r\n", NVTMPP_VB_MAX_COMM_POOLS);
			ret = HD_ERR_LIMIT;
			goto mem_init_fail;
		}
		if (p_mem_config->pool_info[i].blk_size == 0) {
			continue;
		}
		vb_conf.common_pool[pool_cnt].type = p_mem_config->pool_info[i].type;
		vb_conf.common_pool[pool_cnt].ddr = p_mem_config->pool_info[i].ddr_id;
		vb_conf.common_pool[pool_cnt].blk_cnt = p_mem_config->pool_info[i].blk_cnt;
		vb_conf.common_pool[pool_cnt].blk_size = p_mem_config->pool_info[i].blk_size;
		hdal_flow_log_p("    mem type(0x%08x) ddr(%d) blk_cnt(%d) blk_size(0x%08x)\r\n", vb_conf.common_pool[pool_cnt].type, vb_conf.common_pool[pool_cnt].ddr,
			             vb_conf.common_pool[pool_cnt].blk_cnt, vb_conf.common_pool[pool_cnt].blk_size);
		pool_cnt++;
	}
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_CONF_SET, &vb_conf);
    if (ret < 0) {
        ret = HD_ERR_SYS;
		goto mem_init_fail;
    }
	if (vb_conf.rtn < 0) {
		ret = HD_ERR_PARAM;
		goto mem_init_fail;
	}
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_INIT, &init_s);
    if (ret < 0) {
        ret = HD_ERR_SYS;
		goto mem_init_fail;
    }
	if (init_s.rtn < 0) {
		ret = HD_ERR_PARAM;
		goto mem_init_fail;
	}
	return HD_OK;
mem_init_fail:
	NVTMPP_CLOSE(nvtmpp_hdl);
	nvtmpp_hdl = -1;
	return ret;
}

UINT32 hd_common_mem_calc_vdo_buf_size_p(HD_VIDEO_FRAME *p_frame)
{
	UINT32 buf_size;

	switch (p_frame->pxlfmt) {
	case HD_VIDEO_PXLFMT_I1:
	case HD_VIDEO_PXLFMT_I2:
	case HD_VIDEO_PXLFMT_I4:
		buf_size = p_frame->dim.h * p_frame->loff[0];
		break;

	case HD_VIDEO_PXLFMT_YUV444:
		buf_size = p_frame->dim.h * (p_frame->loff[0] + p_frame->loff[1] + p_frame->loff[2]);
		break;

	case HD_VIDEO_PXLFMT_YUV444_PLANAR:
		buf_size = p_frame->dim.h * (p_frame->loff[0] + p_frame->loff[1] + p_frame->loff[2]);
		break;

	case HD_VIDEO_PXLFMT_YUV422:
		buf_size = p_frame->dim.h * p_frame->loff[0] + p_frame->dim.h * p_frame->loff[1];
		break;

	case HD_VIDEO_PXLFMT_YUV420:
		buf_size = p_frame->dim.h * p_frame->loff[0] + (p_frame->dim.h >> 1) * p_frame->loff[1];
		break;

	case HD_VIDEO_PXLFMT_YUV422_ONE:
	case HD_VIDEO_PXLFMT_YUV420_PLANAR:
		buf_size = p_frame->dim.h * p_frame->loff[0] * 2;
		break;

	case HD_VIDEO_PXLFMT_Y8:
	case HD_VIDEO_PXLFMT_I8:
		buf_size = p_frame->dim.h * p_frame->loff[0];
		break;

	case HD_VIDEO_PXLFMT_RGB888:
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		buf_size = p_frame->dim.h * p_frame->loff[0]* 3;
		break;

	case HD_VIDEO_PXLFMT_ARGB8888:
		buf_size = p_frame->dim.h * p_frame->loff[0]* 4;
		break;

	case HD_VIDEO_PXLFMT_RGB565:
	case HD_VIDEO_PXLFMT_RGBA5551:
	case HD_VIDEO_PXLFMT_ARGB1555:
	case HD_VIDEO_PXLFMT_ARGB4444:
	case HD_VIDEO_PXLFMT_RAW16:
		if(p_frame->loff[0])
			buf_size = p_frame->dim.h * p_frame->loff[0]* 2;
		else{
			if(p_frame->dim.w & 0x01){
				DBG_ERR("osg buf width(%u) can't be odd\n", (unsigned int)p_frame->dim.w);
				return 0;
			}
			if(p_frame->dim.h & 0x01){
				DBG_ERR("osg buf height(%u) can't be odd\n", (unsigned int)p_frame->dim.h);
				return 0;
			}
			buf_size = p_frame->dim.h * p_frame->dim.w * 2;
		}
		break;

	default:
		DBG_ERR("Not Support pxlfmt 0x%x\r\n", p_frame->pxlfmt);
		buf_size = 0;
	}
	return buf_size;
}

UINT32 hd_common_mem_calc_buf_size(void *p_hd_data)
{
	HD_VIDEO_FRAME *p_frame = (HD_VIDEO_FRAME *)p_hd_data;

	if (p_frame->sign == MAKEFOURCC('V','F','R','M')) {
		return hd_common_mem_calc_vdo_buf_size_p(p_frame);
	} else if(p_frame->sign == MAKEFOURCC('O','S','G','P')) {
		return hd_common_mem_calc_vdo_buf_size_p(p_frame);
	} else {
		DBG_ERR("Not Support sign 0x%x\r\n", (int)p_frame->sign);
		return 0;
	}
}

void* hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE mem_type, UINT32 phy_addr, UINT32 size)
{
	void        *map_addr;

    if (nvtmpp_hdl < 0) {
		DBG_ERR("hd_common_mem not init\r\n");
		return NULL;
	}
	#if 0
	if (HD_COMMON_MEM_MEM_TYPE_NONCACHE == mem_type) {
		DBG_ERR("Not Support noncache map\r\n");
        return NULL;
	}
	#endif
	if (0 == size) {
		DBG_ERR("size is 0\r\n");
        return NULL;
	}
	hdal_flow_log_p("hd_common_mem_mmap:\r\n    mem_type(%d), phy_addr(0x%08x), size(0x%08x)\r\n", mem_type, (int)phy_addr, (int)size);
	#if defined(__LINUX)
	if ((phy_addr & (sysconf(_SC_PAGE_SIZE) - 1)) != 0) {
		DBG_ERR("phy_addr 0x%x not page align\r\n", (int)phy_addr);
        return NULL;
	}
	if (HD_COMMON_MEM_MEM_TYPE_NONCACHE == mem_type) {
		#define NONCACHE_FLAG        0x80000000

		phy_addr |= NONCACHE_FLAG;
	}
	map_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, nvtmpp_hdl, phy_addr);
	if (map_addr == MAP_FAILED) {
		DBG_ERR("mmap fail phy_addr 0x%x, size 0x%x\r\n", (int)phy_addr, (int)size);
		goto map_err;
	}
	#else
	map_addr = (void *)nvtmpp_sys_pa2va(phy_addr);
	if (0xFFFFFFFF == (UINT32)map_addr) {
		DBG_ERR("mmap fail phy_addr 0x%x, size 0x%x\r\n", (int)phy_addr, (int)size);
		goto map_err;
	}
	#endif
	hdal_flow_log_p("    map_addr(0x%08x)\r\n", (int)map_addr);
	return map_addr;
map_err:
    return NULL;
}



HD_RESULT hd_common_mem_munmap(void* virt_addr, unsigned int size)
{
	#if defined(__LINUX)
	int ret;
	#endif

	if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	if (0 == size) {
		DBG_ERR("size is 0\r\n");
		return HD_ERR_INV;
	}
	hdal_flow_log_p("hd_common_mem_munmap:\r\n    virt_addr(0x%08x), size(0x%08x)\r\n", (int)virt_addr, (int)size);
	#if defined(__LINUX)
	ret = munmap((void*)virt_addr, size);
	if (ret == 0) {
		return HD_OK;
	} else {
		return HD_ERR_INV_PTR;
	}
	#else
	return HD_OK;
	#endif
}

HD_RESULT hd_common_mem_flush_cache(void* virt_addr, unsigned int size)
{
	NVTMPP_IOC_VB_CACHE_SYNC_S    msg;
	int                           ret;

    if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	if (0 == size) {
		DBG_ERR("size is 0\r\n");
		return HD_ERR_INV;
	}
	msg.virt_addr = virt_addr;
	msg.size = size;
	msg.dma_dir = NVTMPP_IOC_DMA_BIDIRECTIONAL;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_CACHE_SYNC, &msg);
    if (ret < 0) {
        return HD_ERR_SYS;
    }
    return HD_OK;
}


HD_COMMON_MEM_VB_BLK hd_common_mem_get_block(HD_COMMON_MEM_POOL_TYPE pool_type, UINT32 blk_size, HD_COMMON_MEM_DDR_ID ddr)
{
	NVTMPP_IOC_VB_GET_BLK_S       msg;
	int                           ret;

    if (nvtmpp_hdl < 0) {
		return HD_COMMON_MEM_VB_INVALID_BLK;
	}
	if (0 == blk_size) {
		DBG_ERR("blk_size is 0\r\n");
		return HD_COMMON_MEM_VB_INVALID_BLK;
	}
	if (pool_type == HD_COMMON_MEM_COMMON_POOL) {
		msg.pool = NVTMPP_VB_INVALID_POOL;
	} else {
		msg.pool = pool_type;
	}
	msg.blk_size = blk_size;
	msg.ddr = ddr;
    ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_BLK, &msg);
    if (ret < 0) {
        return HD_COMMON_MEM_VB_INVALID_BLK;
    }
	return msg.rtn;
}

HD_RESULT hd_common_mem_release_block(HD_COMMON_MEM_VB_BLK blk)
{
	NVTMPP_IOC_VB_REL_BLK_S   msg;
    int                       ret;

    if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	msg.blk = blk;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_REL_BLK, &msg);
    if (ret < 0) {
        return HD_ERR_SYS;
    }
	if (msg.rtn < 0) {
		return HD_ERR_INV;
	}
    return HD_OK;
}

UINT32 hd_common_mem_blk2pa(HD_COMMON_MEM_VB_BLK blk)
{
	NVTMPP_IOC_VB_GET_BLK_PA_S   msg;
    int                          ret;

    if (nvtmpp_hdl < 0) {
		DBG_ERR("hd_common_mem not init\r\n");
		return 0;
	}
	msg.blk = blk;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_BLK_PA, &msg);
    if (ret < 0) {
		return 0;
    }
	if (msg.rtn <= 0) {
		return 0;
	}
	return msg.rtn;
}


HD_RESULT hd_common_mem_alloc(CHAR* name, UINT32 *phy_addr, void **virt_addr, UINT32 size, HD_COMMON_MEM_DDR_ID ddr)
{
	NVTMPP_IOC_VB_CREATE_POOL_S   cre_pool_s = {0};
	NVTMPP_IOC_VB_DESTROY_POOL_S  des_pool_s = {0};
	NVTMPP_IOC_VB_GET_BLK_S       get_blk_s = {0};
	HD_COMMON_MEM_VB_BLK          blk;
	NVTMPP_VB_POOL                pool;
	UINT32                        pa;
	void                         *va;
    int                           ret;

	if (name == NULL) {
		hdal_flow_log_p("hd_common_mem_alloc:\r\n    ddr(%d), size(0x%08x), name(NULL), phy_addr(0x%08x), virt_addr(0x%08x)\r\n", ddr, size, (int)phy_addr, (int)virt_addr);
	} else {
		hdal_flow_log_p("hd_common_mem_alloc:\r\n    ddr(%d), size(0x%08x), name(%s), phy_addr(0x%08x), virt_addr(0x%08x)\r\n", ddr, size, name, (int)phy_addr, (int)virt_addr);
	}
    if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	if (phy_addr == NULL) {
		DBG_ERR("phy_addr is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
	if (virt_addr == NULL) {
		DBG_ERR("virt_addr is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
	if (size == 0) {
		DBG_ERR("size is 0\r\n");
		return HD_ERR_INV;
	}
	if (name == NULL) {
		strncpy(cre_pool_s.pool_name, "user", sizeof(cre_pool_s.pool_name)-1);
	} else {
		strncpy(cre_pool_s.pool_name, name, sizeof(cre_pool_s.pool_name)-1);
	}
	cre_pool_s.blk_cnt = 1;
	cre_pool_s.blk_size = size;
	cre_pool_s.ddr = ddr;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_CREATE_POOL, &cre_pool_s);
    if (ret < 0) {
		hdal_flow_log_p("    ioctl create pool fail\r\n");
		DBG_ERR("cre_pool_fail\r\n");
		goto cre_pool_fail;
    }
	if (cre_pool_s.rtn == NVTMPP_VB_INVALID_POOL) {
		hdal_flow_log_p("    create pool fail\r\n");
		DBG_ERR("cre_pool_fail\r\n");
		goto cre_pool_fail;
	}
	pool = cre_pool_s.rtn;
    get_blk_s.pool = pool;
	get_blk_s.blk_size = size;
	get_blk_s.ddr = ddr;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_BLK, &get_blk_s);
    if (ret < 0) {
		hdal_flow_log_p("    ioctl get blk fail\r\n");
		DBG_ERR("get_blk_fail\r\n");
		goto get_blk_fail;
    }
	blk = get_blk_s.rtn;
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		DBG_ERR("blk2pa fail, blk = 0x%x\r\n", (int)blk);
		goto get_blk_fail;
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);
	if (va == 0) {
		DBG_ERR("mmap fail, pa = 0x%x, size = 0x%x\r\n", (int)pa, (int)size);
		goto get_blk_fail;
	}
	*phy_addr = pa;
	*virt_addr = va;
	hdal_flow_log_p("    pa(0x%08x) va(0x%08x) \r\n", pa, (int)va);
	return HD_OK;
get_blk_fail:
	des_pool_s.pool = pool;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_DESTROY_POOL, &des_pool_s);
cre_pool_fail:
	*phy_addr = 0;
	*virt_addr = 0;
	return HD_ERR_NOMEM;
}

HD_RESULT hd_common_mem_free(UINT32 phy_addr, void *virt_addr)
{
	NVTMPP_IOC_VB_PA_TO_POOL_S    pa2pool_s;
	NVTMPP_IOC_VB_DESTROY_POOL_S  des_pool_s;
    int                           ret;
	NVTMPP_VB_POOL                pool;
	UINT32                        size;

	hdal_flow_log_p("hd_common_mem_free:\r\n    pa(0x%08x) va(0x%08x) \r\n", phy_addr, (int)virt_addr);
    if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	pa2pool_s.blk_pa = phy_addr;
	//DBG_IND("pa2pool phy_addr = 0x%x\r\n", (int)phy_addr);
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_PA_TO_POOL, &pa2pool_s);
    if (ret < 0) {
		return HD_ERR_SYS;
    }
	if (NVTMPP_VB_INVALID_POOL == pa2pool_s.pool) {
		return HD_ERR_INV_PTR;
	}
	pool = pa2pool_s.pool;
	size = pa2pool_s.size;
	des_pool_s.pool = pool;
	//DBG_IND("destroy pool = 0x%x, size = 0x%x\r\n", (int)pool, (int)size);
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_DESTROY_POOL, &des_pool_s);
    if (ret < 0) {
		DBG_ERR("destroy pool fail, pool = 0x%x\r\n", (int)pool);
        return HD_ERR_SYS;
    }
	if (des_pool_s.rtn < 0) {
		return HD_ERR_INV_PTR;
	}
	return hd_common_mem_munmap(virt_addr, size);
}

static HD_RESULT hd_common_mem_get_va_info_p(HD_COMMON_MEM_VIRT_INFO  *p_vir_info)
{
	NVTMPP_IOC_VB_GET_USER_VA_INFO_S   msg;
    int                                ret;

    if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	if (NULL == p_vir_info ) {
		return HD_ERR_NULL_PTR;
	}
	msg.va = p_vir_info->va;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_USER_VA_INFO, &msg);
    if (ret < 0) {
        return HD_ERR_SYS;
    }
	if (msg.rtn < 0) {
        return HD_ERR_INV_PTR;
    }
	p_vir_info->pa = msg.pa;
	p_vir_info->cached = msg.cached;
	return HD_OK;
}

HD_RESULT hd_common_mem_get(HD_COMMON_MEM_PARAM_ID id, VOID *p_param)
{
	if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	switch (id) {
	case HD_COMMON_MEM_PARAM_VIRT_INFO:
		return hd_common_mem_get_va_info_p((HD_COMMON_MEM_VIRT_INFO *)p_param);
	default:
		DBG_ERR("Not Support parm id 0x%x\r\n", id);
		return HD_ERR_NOT_SUPPORT;
	}
	return HD_OK;
}

HD_RESULT hd_common_dmacopy(HD_COMMON_MEM_DDR_ID src_ddr, UINT32 src_phy_addr,
                            HD_COMMON_MEM_DDR_ID dst_ddr, UINT32 dst_phy_addr,
                            UINT32 len)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_common_mem_uninit(void)
{
	NVTMPP_IOC_VB_EXIT_S   msg;
    int                    ret;

	hdal_flow_log_p("hd_common_mem_uninit\r\n");
	if (nvtmpp_hdl < 0) {
		DBG_ERR("hd_common_mem not init?\r\n");
		return HD_ERR_UNINIT;
	}
	if (_hd_common_get_pid() == 0) {
		if (_hd_videocap_is_init()) {
			DBG_ERR("hd_videocap is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_videoproc_is_init()) {
			DBG_ERR("hd_videoproc is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_videoout_is_init()) {
			DBG_ERR("hd_videoout is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_videoenc_is_init()) {
			DBG_ERR("hd_videoenc is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_videodec_is_init()) {
			DBG_ERR("hd_videodec is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_audiocap_is_init()) {
			DBG_ERR("hd_audiocap is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_audioout_is_init()) {
			DBG_ERR("hd_audioout is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_audioenc_is_init()) {
			DBG_ERR("hd_audioenc is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_audiodec_is_init()) {
			DBG_ERR("hd_audiodec is not uninit?\r\n");
			return HD_ERR_UNINIT;
		}
		if (_hd_osg_is_init() && _hd_common_get_pid() <= 0) {
			extern void _hd_osg_uninit(void);
			_hd_osg_uninit();
		}

	    ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_EXIT, &msg);
	    if (ret < 0) {
	        return HD_ERR_SYS;
	    }
	}
	NVTMPP_CLOSE(nvtmpp_hdl);
	nvtmpp_hdl = -1;
	return HD_OK;
}

HD_RESULT hd_common_pcie_init(void)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_common_pcie_open(char *name, HD_COMMON_PCIE_CHIP chip_id, int chan_id, int mode)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_common_pcie_close(HD_COMMON_PCIE_CHIP chip_id, int chan_id)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_common_pcie_get(HD_COMMON_PCIE_CHIP chip_id, int chan_id, HD_COMMON_PCIE_PARAM_ID id, void *parm)
{
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_common_pcie_set(HD_COMMON_PCIE_CHIP chip_id, int chan_id, HD_COMMON_PCIE_PARAM_ID id, void *parm)
{
	return HD_ERR_NOT_SUPPORT;
}

int hd_common_pcie_recv(HD_COMMON_PCIE_CHIP chip_id, int chan_id, unsigned char *pBuf, unsigned int len)
{
	return 0;
}

int hd_common_pcie_send(HD_COMMON_PCIE_CHIP chip_id, int chan_id, unsigned char *pBuf, unsigned int len)
{
	return 0;
}

HD_RESULT hd_common_pcie_uninit(void)
{
	return HD_ERR_NOT_SUPPORT;
}

int _hd_common_get_fd(void)
{
	return dev_fd;
}

