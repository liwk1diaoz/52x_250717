/**
	@brief Source file of vendor media common.

	@file vendor_common.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#if defined(__LINUX)
#define _GNU_SOURCE
#include <sched.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(__LINUX)
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#include "hdal.h"
//#include "hd_logger_p.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/nvtmpp_ioctl.h"
#include "vendor_common.h"
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define NVTMPP_OPEN(...) 0
#define NVTMPP_IOCTL nvtmpp_ioctl
#define NVTMPP_CLOSE(...)
#endif
#if defined(__LINUX)
#define NVTMPP_OPEN  open
#define NVTMPP_IOCTL ioctl
#define NVTMPP_CLOSE close
#endif

#define CHKPNT                    printf("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__) ///< Show a color sting of line count and function name in your insert codes
#define DBG_ERR(fmtstr, args...)  printf("\033[31mvendor_comm ERR:%s(): \033[0m" fmtstr,__func__, ##args)
#define DBG_WRN(fmtstr, args...)  printf("\033[33mvendor_comm WRN:%s(): \033[0m" fmtstr,__func__, ##args)
#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)


extern BOOL g_vout_keep_open;
extern BOOL g_aout_keep_open;
extern int nvtmpp_hdl;
extern void hdal_flow_log_p(const char *fmt, ...);

#if defined(__LINUX)
static void sysfs_write_p(char *path, char *s)
{
	char buf[80];
	int len;
	int fd = open(path, O_WRONLY);

	if (fd < 0) {
		if (strerror_r(errno, buf, sizeof(buf)) == 0) {
			printf("err: %s\r\n" ,buf);
		}
		return;
	}

	len = write(fd, s, strlen(s) + 1);
	if (len < 0) {
		if (strerror_r(errno, buf, sizeof(buf)) == 0) {
			printf("err: %s\r\n" ,buf);
		}
	}
	close(fd);
}

static ssize_t sysfs_read_p(char *path, char *s, int num_bytes)
{
	char buf[80];
	ssize_t count = 0;
	int fd = open(path, O_RDONLY);

	if (fd < 0) {
		if (strerror_r(errno, buf, sizeof(buf)) == 0) {
			printf("err: %s\r\n" ,buf);
		}
		return -1;
	}

	if ((count = read(fd, s, (num_bytes -1))) < 0) {
		if (strerror_r(errno, buf, sizeof(buf)) == 0) {
			printf("err: %s\r\n" ,buf);
		}
	} else {
		if ((count >= 1) && (s[count-1] == '\n')) {
			s[count-1] = '\0';
		} else {
			s[count] = '\0';
		}
	}
	close(fd);
	return count;
}

static int hotplug_align_size_p(void)
{
	char buf[15] = {0};
	int ret = 0;

	ret = sysfs_read_p("/sys/devices/system/memory/block_size_bytes", buf, 15);
	if (ret < 0) {
		printf("%s: can't get alignment size\r\n", __func__);
		return HD_ERR_DRV;
	}

	ret = (int)strtol(buf, NULL, 16);
	if (ret <= 0)
		return HD_ERR_DRV;
	else
		return ret;
}

static int hotplug_set_p(unsigned long mem_addr, unsigned long mem_size)
{
	char buf[60] = {0};
	int block_size_bytes = 0;
	unsigned long size = 0;

	block_size_bytes = hotplug_align_size_p();
	if (block_size_bytes < 0)
		return HD_ERR_DRV;

	if ((mem_addr % block_size_bytes) != 0 ||
		(mem_size % block_size_bytes) != 0) {
		printf("%s: Your parameters are not aligned.  \
			Addr: 0x%x size: 0x%x aligned size: 0x%x\n", __func__, (int)mem_addr, (int)mem_size, (int)block_size_bytes);
		return HD_ERR_INV;
	}

	// check if mem has plugged before
	struct stat sb;
	sprintf(buf, "/sys/devices/system/memory/memory%d", (int)(mem_addr/block_size_bytes));
	if (stat(buf, &sb) == 0 && S_ISDIR(sb.st_mode)) {
		//printf("mem has plugged.\n");
		return HD_OK;
	}

	// Add memblock probe
	size = mem_size;
	while (size > 0) {
		size -= block_size_bytes;
		sprintf(buf, "0x%08lx", mem_addr + size);
		sysfs_write_p("/sys/devices/system/memory/probe", buf);
		sprintf(buf, "/sys/devices/system/memory/memory%u/state", (unsigned int)((mem_addr + size) / block_size_bytes));
		sysfs_write_p(buf, "online_movable");
	}

	return HD_OK;
}
#endif




HD_RESULT vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM item, VOID *p_param)
{
	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}
	switch (item) {
	#if defined(__LINUX)
	case VENDOR_COMMON_MEM_ITEM_LINUX_HOT_PLUG: {
			VENDOR_LINUX_MEM_HOT_PLUG *p_mem_hotplug;

			p_mem_hotplug = (VENDOR_LINUX_MEM_HOT_PLUG *)p_param;
			return hotplug_set_p(p_mem_hotplug->start_addr, p_mem_hotplug->size);
		}
	case VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_START: {
			int ret;
			VENDOR_COMM_DDR_MONITOR_ID   *p_ddr;
			NVTMPP_DDR_MONITOR_ID_S    msg = {0};
			p_ddr = (VENDOR_COMM_DDR_MONITOR_ID *)p_param;
			msg.ddr = p_ddr->ddr;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_DDR_MONITOR_START, &msg);
		    if (ret < 0) {
				return HD_ERR_FAIL;
		    }
		}
		break;
	case VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_STOP: {
			int ret;
			VENDOR_COMM_DDR_MONITOR_ID  *p_ddr;
			NVTMPP_DDR_MONITOR_ID_S    msg = {0};
			p_ddr = (VENDOR_COMM_DDR_MONITOR_ID *)p_param;
			msg.ddr = p_ddr->ddr;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_DDR_MONITOR_STOP, &msg);
		    if (ret < 0) {
				return HD_ERR_FAIL;
		    }
		}
		break;
	case VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_RESET: {
			int ret;
			VENDOR_COMM_DDR_MONITOR_ID   *p_ddr;
			NVTMPP_DDR_MONITOR_ID_S     msg = {0};
			p_ddr = (VENDOR_COMM_DDR_MONITOR_ID *)p_param;
			msg.ddr = p_ddr->ddr;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_DDR_MONITOR_RESET, &msg);
		    if (ret < 0) {
				return HD_ERR_FAIL;
		    }
		}
		break;
	#endif
	default:
		printf("%s Not Support parm item 0x%x\r\n", __func__,item);
		return HD_ERR_NOT_SUPPORT;
	}
	return HD_OK;

}

HD_RESULT vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM item, VOID *p_param)
{
	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}
	switch (item) {
	case VENDOR_COMMON_MEM_ITEM_BRIDGE_MEM: {
			int ret;
			VENDOR_COMM_BRIDGE_MEM *p_bridge_mem = {0};
			NVTMPP_GET_SYSMEM_REGION_S sysmem_region = {0};
			p_bridge_mem = (VENDOR_COMM_BRIDGE_MEM *)p_param;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_BRIDGE_MEM, &sysmem_region);
		    if (ret < 0 || sysmem_region.rtn != 0) {
				DBG_ERR("get bridge mem failed.\r\n");
				return HD_ERR_FAIL;
		    }
			p_bridge_mem->phys_addr = sysmem_region.phys_addr;
			p_bridge_mem->size = sysmem_region.size;
		}
		break;
	case VENDOR_COMMON_MEM_ITEM_MAX_FREE_BLOCK_SIZE: {
			int ret;
			VENDOR_COMM_MAX_FREE_BLOCK    *p_maxfreeblk = {0};
			NVTMPP_IOC_VB_GET_MAX_FREE_S  free_s = {0};

			p_maxfreeblk = (VENDOR_COMM_MAX_FREE_BLOCK *)p_param;
			free_s.ddr = p_maxfreeblk->ddr;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_MAX_FREE_BLK_SZ, &free_s);
		    if (ret < 0) {
				DBG_ERR("get maxfreeblk failed.\r\n");
				return HD_ERR_FAIL;
		    }
			p_maxfreeblk->size = free_s.size;
		}
		break;
	case VENDOR_COMMON_MEM_ITEM_COMM_POOL_RANGE: {
			int                         ret;
			VENDOR_COMM_POOL_RANGE      *p_range = {0};
			NVTMPP_GET_COMM_POOL_RANGE_S msg = {0};

			p_range = (VENDOR_COMM_POOL_RANGE *)p_param;
			msg.ddr = p_range->ddr;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_COMM_POOL_RANGE, &msg);
		    if (ret < 0 || msg.rtn != 0) {
				DBG_ERR("get comm pool range failed.\r\n");
				return HD_ERR_FAIL;
		    }
			p_range->phys_addr = msg.phys_addr;
			p_range->size = msg.size;
		}
		break;
	case VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_DATA: {
			int ret;
			VENDOR_COMM_DDR_MONITOR_DATA *p_data = {0};
			NVTMPP_DDR_MONITOR_DATA_S      msg = {0};
			p_data = (VENDOR_COMM_DDR_MONITOR_DATA *)p_param;
			msg.ddr = p_data->ddr;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_DDR_MONITOR_DATA, &msg);
		    if (ret < 0) {
				return HD_ERR_FAIL;
		    }
			p_data->cnt = msg.cnt;
			p_data->byte = msg.byte;
		}
		break;
	case VENDOR_COMMON_MEM_ITEM_FREE_SIZE: {
			int ret;
			VENDOR_COMM_FREE_SIZE        *p_free = {0};
			NVTMPP_IOC_VB_GET_FREE_S      free_s = {0};

			p_free = (VENDOR_COMM_FREE_SIZE *)p_param;
			free_s.ddr = p_free->ddr;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_GET_FREE_SZ, &free_s);
		    if (ret < 0) {
				DBG_ERR("get maxfreeblk failed.\r\n");
				return HD_ERR_FAIL;
		    }
			p_free->size = free_s.size;
		}
		break;
	case VENDOR_COMMON_MEM_ITEM_PA_TO_BLK: {
			int ret;
			VENDOR_COMM_PA_TO_BLK        *p_msg = {0};
			NVTMPP_IOC_VB_PA_TO_BLK_S     msg_s = {0};

			p_msg = (VENDOR_COMM_PA_TO_BLK *)p_param;
			msg_s.pa = p_msg->pa;
			ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_PA_TO_BLK, &msg_s);
		    if (ret < 0) {
				DBG_ERR("blktopa failed.\r\n");
				return HD_ERR_FAIL;
		    }
			p_msg->blk = msg_s.blk;
		}
		break;

	default:
		printf("%s Not Support parm item 0x%x\r\n", __func__,item);
		return HD_ERR_NOT_SUPPORT;
	}
	return HD_OK;

}

static void* vendor_common_mem_mmap(int fd, HD_COMMON_MEM_MEM_TYPE mem_type, UINT32 phy_addr, UINT32 size)
{
	void        *map_addr;

	if (HD_COMMON_MEM_MEM_TYPE_NONCACHE == mem_type) {
		DBG_ERR("Not Support noncache map\r\n");
        return NULL;
	}
	if (0 == size) {
		DBG_ERR("size is 0\r\n");
        return NULL;
	}
	#if defined(__LINUX)
	if ((phy_addr & (sysconf(_SC_PAGE_SIZE) - 1)) != 0) {
		DBG_ERR("phy_addr 0x%x not page align\r\n", (int)phy_addr);
        return NULL;
	}
	map_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phy_addr);
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
	return map_addr;
map_err:
    return NULL;
}

HD_RESULT vendor_common_mem_alloc_fixed_pool(CHAR* name, UINT32 *phy_addr, void **virt_addr, UINT32 size, HD_COMMON_MEM_DDR_ID ddr)
{
	NVTMPP_IOC_VB_CREATE_FIXPOOL_S   cre_fixpool_s = {0} ;
	UINT32                           pa;
	void                            *va;
    int                              ret;
	int                              fd;

	if (name == NULL) {
		DBG_ERR("name is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
	if (phy_addr == NULL) {
		DBG_ERR("phy_addr is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
	if (virt_addr == NULL) {
		DBG_ERR("virt_addr is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
	fd = NVTMPP_OPEN("/dev/nvtmpp", O_RDWR|O_SYNC);
	if (fd < 0) {
		DBG_ERR("open /dev/nvtmpp error\r\n");
		return HD_ERR_SYS;
	}
	strncpy(cre_fixpool_s.pool_name, name, sizeof(cre_fixpool_s.pool_name)-1);
	cre_fixpool_s.blk_cnt = 1;
	cre_fixpool_s.blk_size = size;
	cre_fixpool_s.ddr = ddr;
	ret = NVTMPP_IOCTL(fd, NVTMPP_IOC_VB_CREATE_FIXED_POOL, &cre_fixpool_s);
	pa = cre_fixpool_s.rtn;
    if (ret < 0 || pa == 0) {
		DBG_ERR("cre_fixedpool_fail\r\n");
		goto cre_fixpool_fail;
    }
	va = vendor_common_mem_mmap(fd, HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);
	if (va == 0) {
		DBG_ERR("mmap fail, pa = 0x%x, size = 0x%x\r\n", (int)pa, (int)size);
		goto cre_fixpool_fail;
	}
	*phy_addr = pa;
	*virt_addr = va;
	NVTMPP_CLOSE(fd);
	return HD_OK;
cre_fixpool_fail:
	*phy_addr = 0;
	*virt_addr = 0;
	NVTMPP_CLOSE(fd);
	return HD_ERR_NOMEM;
}

HD_RESULT vendor_common_mem_relayout(HD_COMMON_MEM_INIT_CONFIG *p_mem_config)
{
	NVTMPP_IOC_VB_CONF_S  vb_conf;
	NVTMPP_IOC_VB_INIT_S  init_s;
	UINT32                i, pool_cnt = 0;
	int                   ret;

	//hdal_flow_log_p("hd_common_mem_relayout:\r\n");
	if (nvtmpp_hdl < 0) {
		DBG_ERR("Please init hd_common_mem firstly\r\n");
		return HD_ERR_INIT;
	}
	if (p_mem_config == NULL) {
		ret = HD_ERR_NULL_PTR;
		goto mem_init_fail;
	}
	memset(&vb_conf, 0x00, sizeof(vb_conf));
	vb_conf.max_pool_cnt = NVTMPP_VB_MAX_COMM_POOLS;
	pool_cnt = 0;
	for (i = 0; i < HD_COMMON_MEM_MAX_POOL_NUM; i++) {
		if (pool_cnt >= NVTMPP_VB_MAX_COMM_POOLS) {
			DBG_ERR("exceeds vb pool limit %d\r\n", NVTMPP_VB_MAX_COMM_POOLS);
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
		//hdal_flow_log_p("    mem type(0x%08x) ddr(%d) blk_cnt(%d) blk_size(0x%08x)\r\n", vb_conf.common_pool[pool_cnt].type, vb_conf.common_pool[pool_cnt].ddr,
		//	             vb_conf.common_pool[pool_cnt].blk_cnt, vb_conf.common_pool[pool_cnt].blk_size);
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
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_RELAYOUT, &init_s);
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


HD_RESULT vendor_common_mem_cache_sync(void* virt_addr, unsigned int size, VENDOR_COMMON_MEM_DMA_DIR dir)
{
	NVTMPP_IOC_VB_CACHE_SYNC_S    msg;
	int                           ret;

	if (nvtmpp_hdl < 0) {
		DBG_ERR("Please init hd_common_mem firstly\r\n");
		return HD_ERR_INIT;
	}
	if (0 == size) {
		DBG_ERR("size is 0\r\n");
		return HD_ERR_INV;
	}
	msg.virt_addr = virt_addr;
	msg.size = size;
	msg.dma_dir = dir;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_CACHE_SYNC, &msg);
    if (ret < 0) {
        return HD_ERR_SYS;
    }
    return HD_OK;
}

HD_RESULT vendor_common_mem_cache_sync_all(void* virt_addr, unsigned int size, VENDOR_COMMON_MEM_DMA_DIR dir)
{
	#if defined(__LINUX)
	NVTMPP_IOC_VB_CACHE_SYNC_BY_CPU_S    msg;
	int                                  ret;
	cpu_set_t                            mask;

	if (nvtmpp_hdl < 0) {
		DBG_ERR("Please init hd_common_mem firstly\r\n");
		return HD_ERR_INIT;
	}
	if (0 == size) {
		DBG_ERR("size is 0\r\n");
		return HD_ERR_INV;
	}
	if (sched_getaffinity(0, sizeof(mask), &mask) == -1)  {
		DBG_ERR("sched_getaffinity fail %d\r\n");
		return HD_ERR_SYS;
	}
	msg.virt_addr = virt_addr;
	msg.size = size;
	msg.dma_dir = dir;
	msg.cpu_count = CPU_COUNT(&mask);
	if (CPU_ISSET(1, &mask)) {
		msg.cpu_id = 1;
	} else {
		msg.cpu_id = 0;
	}
	//printf("msg.cpu_count = %d, cpu_id = %d\r\n", msg.cpu_count, msg.cpu_id);
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_CACHE_SYNC_BY_CPU, &msg);
    if (ret < 0) {
        return HD_ERR_SYS;
    }
    return HD_OK;
	#else
	return HD_ERR_SYS;
	#endif
}

VENDOR_COMMON_MEM_VB_POOL vendor_common_mem_create_pool(CHAR *pool_name, UINT32 blk_size, UINT32 blk_cnt, HD_COMMON_MEM_DDR_ID ddr)
{
	NVTMPP_IOC_VB_CREATE_POOL_S   cre_pool_s = {0};
	NVTMPP_VB_POOL                pool;
	int                           ret;

	if (pool_name == NULL) {
		hdal_flow_log_p("vendor_common_mem_create_pool:\r\n    ddr(%d), blk_size(0x%08x), blk_cnt(%d), name(NULL)\r\n",
						ddr, blk_size, blk_cnt);
	} else {
		hdal_flow_log_p("vendor_common_mem_create_pool:\r\n    ddr(%d), blk_size(0x%08x), blk_cnt(%d), name(%s)\r\n",
						ddr, blk_size, blk_cnt, pool_name);
	}
    if (nvtmpp_hdl < 0) {
		return VENDOR_COMMON_MEM_VB_INVALID_POOL;
	}
	if (blk_size == 0) {
		DBG_ERR("blk_size is 0\r\n");
		return VENDOR_COMMON_MEM_VB_INVALID_POOL;
	}
	if (pool_name == NULL) {
		strncpy(cre_pool_s.pool_name, "user", sizeof(cre_pool_s.pool_name)-1);
	} else {
		strncpy(cre_pool_s.pool_name, pool_name, sizeof(cre_pool_s.pool_name)-1);
	}
	cre_pool_s.blk_cnt = blk_cnt;
	cre_pool_s.blk_size = blk_size;
	cre_pool_s.ddr = ddr;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_CREATE_POOL, &cre_pool_s);
	if (ret < 0) {
		hdal_flow_log_p("    ioctl create pool fail\r\n");
		DBG_ERR("cre_pool_fail\r\n");
		goto cre_pool_fail;
	}
	if (cre_pool_s.rtn == VENDOR_COMMON_MEM_VB_INVALID_POOL) {
		hdal_flow_log_p("    create pool fail\r\n");
		DBG_ERR("cre_pool_fail\r\n");
		goto cre_pool_fail;
	}
	pool = cre_pool_s.rtn;
	return pool;
cre_pool_fail:
	return VENDOR_COMMON_MEM_VB_INVALID_POOL;
}


HD_RESULT vendor_common_mem_destroy_pool(VENDOR_COMMON_MEM_VB_POOL pool)
{
	NVTMPP_IOC_VB_DESTROY_POOL_S  des_pool_s;
    int                           ret;

	hdal_flow_log_p("vendor_common_mem_destroy_pool:\r\n    pool(0x%08x)\r\n", (int)pool);
    if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	des_pool_s.pool = pool;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_DESTROY_POOL, &des_pool_s);
	if (ret < 0) {
		DBG_ERR("destroy pool fail, pool = 0x%x\r\n", (int)pool);
		return HD_ERR_SYS;
	}
	if (des_pool_s.rtn < 0) {
		return HD_ERR_INV_PTR;
	}
	return HD_OK;
}


HD_RESULT vendor_common_mem_lock_block(HD_COMMON_MEM_VB_BLK blk)
{
	NVTMPP_IOC_VB_LOCK_BLK_S   msg;
    int                        ret;

    if (nvtmpp_hdl < 0) {
		return HD_ERR_UNINIT;
	}
	msg.blk = blk;
	ret = NVTMPP_IOCTL(nvtmpp_hdl, NVTMPP_IOC_VB_LOCK_BLK, &msg);
    if (ret < 0) {
        return HD_ERR_SYS;
    }
	if (msg.rtn < 0) {
		return HD_ERR_INV;
	}
    return HD_OK;
}